/**
 * text editor in c (raylib)
 * -------------------------
 * organization:
 * 1. includes & structs
 * 2. prototypes
 * 3. selection logic (core)
 * 4. memory management
 * 5. input processing
 * 6. selection state
 * 7. main loop & rendering
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"

// ============================================================================
// 1. data & prototypes
// ============================================================================

typedef struct Block {
    int id;
    char *text;
    int cursor_index;
    struct Block *next; 
    
    // selection state (-1 = none)
    int sel_start;
    int sel_len;        
} Block;

typedef struct {
    Block *start;
    Block *end;
    int id_counter;
} Document;

Block *anchor_block = NULL; 
int anchor_index = 0;

void update_selection_range(Document *doc, Block *current_hover, int current_index);
Block* delete_selected_text(Document *doc);

// ============================================================================
// 2. core logic: selection & memory surgery
// ============================================================================

// deletes selected range. returns surviving block to update focus.
Block* delete_selected_text(Document *doc) {
    Block *b = doc->start;
    Block *first = NULL;
    Block *last = NULL;
    bool has_selection = false;

    // find selection bounds
    while (b != NULL) {
        if (b->sel_start != -1) {
            if (first == NULL) first = b;
            last = b;
            if (b->sel_len > 0) has_selection = true;
        }
        b = b->next;
    }

    // ignore 0-length selection (cursor only)
    if (first != NULL && first == last && first->sel_len == 0) return NULL;
    if (first != last && first != NULL) has_selection = true;
    if (!has_selection) return NULL;

    // scenario: single block (simple memmove)
    if (first == last) {
        int text_len = strlen(first->text);
        int remove_len = first->sel_len;
        
        // safety clamp
        if (first->sel_start + remove_len > text_len) {
            remove_len = text_len - first->sel_start;
        }

        memmove(&first->text[first->sel_start], 
                &first->text[first->sel_start + remove_len], 
                text_len - (first->sel_start + remove_len) + 1);
        
        first->cursor_index = first->sel_start;
        first->sel_start = -1; 
        first->sel_len = 0;
        return first; 
    }

    // scenario: multi-block (complex merge)
    
    // 1. save tail from last block
    int tail_start = last->sel_len;
    if (tail_start > (int)strlen(last->text)) tail_start = strlen(last->text);
    char *tail_text = strdup(&last->text[tail_start]);

    // 2. delete intermediate nodes manually
    Block *block_after_selection = last->next;
    Block *curr = first->next;
    
    while (curr != NULL && curr != block_after_selection) {
        Block *next_node = curr->next;
        free(curr->text);
        free(curr);
        curr = next_node;
    }

    // 3. merge tail into first block
    first->text[first->sel_start] = '\0';
    int first_len = strlen(first->text);
    int tail_len = strlen(tail_text);
    
    first->text = (char*)realloc(first->text, first_len + tail_len + 1);
    strcat(first->text, tail_text);
    free(tail_text);

    // 4. reconnect list
    first->next = block_after_selection;
    if (block_after_selection == NULL) doc->end = first;

    first->cursor_index = first->sel_start;
    first->sel_start = -1; 
    first->sel_len = 0;
    return first;
}

// ============================================================================
// 3. list management
// ============================================================================

Document* create_document() {
    Document *doc = (Document*)malloc(sizeof(Document));
    if (doc == NULL) return NULL;
    doc->start = NULL;
    doc->end = NULL;
    doc->id_counter = 0;
    return doc;
}

Block* create_block(int id, char *text_content) {
    Block *new_block = (Block*)malloc(sizeof(Block));
    new_block->id = id;
    new_block->text = strdup(text_content);
    new_block->next = NULL;
    new_block->cursor_index = strlen(text_content);
    new_block->sel_start = -1;
    new_block->sel_len = 0;
    return new_block;
}

void add_block(Document *doc, char *text) {
    doc->id_counter++;
    Block *new_block = create_block(doc->id_counter, text);
    if (doc->start == NULL) {
        doc->start = new_block;
        doc->end = new_block;
    } else {
        doc->end->next = new_block;
        doc->end = new_block;
    }
}

void insert_block_after(Document *doc, Block *prev_block, char *text) {
    doc->id_counter++;
    Block *new_block = create_block(doc->id_counter, text);

    if (prev_block == NULL) {
        new_block->next = doc->start;
        doc->start = new_block;
        if (doc->end == NULL) doc->end = new_block;
    } else {
        new_block->next = prev_block->next;
        prev_block->next = new_block;
        if (prev_block == doc->end) doc->end = new_block;
    }
}

void free_document(Document *doc) {
    Block *current = doc->start;
    while (current != NULL) {
        Block *temp_next = current->next; 
        free(current->text); 
        free(current);       
        current = temp_next;
    }
    free(doc);
}

// ============================================================================
// 4. input logic
// ============================================================================

Block* update_typing(Document *doc, Block *b, double *last_action_time){ 
    double now = GetTime();
    int maxWidth = 680;

    // reset blink on any interaction
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || 
        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || 
        IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE) ||
        IsKeyDown(KEY_ENTER)) {
        *last_action_time = now;
    }

    // --- SHIFT STATE ---
    bool is_shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool moved = false; // flag to trigger selection update

    // 1. Setup Anchor (Start Selection)
    if (is_shift && anchor_block == NULL) {
        anchor_block = b;
        anchor_index = b->cursor_index;
    }

    // 2. Clear Selection (If moving without Shift)
    // We check this inside the movement blocks to avoid clearing just by pressing keys without moving
    
    // ------------------------------------------------------------------------
    // Horizontal Navigation
    // ------------------------------------------------------------------------
    static double next_horiz_time = 0; 
    bool move_r = false;
    bool move_l = false;

    if (IsKeyPressed(KEY_RIGHT)) { move_r = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_RIGHT) && now > next_horiz_time) { move_r = true; next_horiz_time = now + 0.04; }

    if (IsKeyPressed(KEY_LEFT)) { move_l = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_LEFT) && now > next_horiz_time) { move_l = true; next_horiz_time = now + 0.04; }
    
    if (move_r || move_l) {
        if (!is_shift) { // Clear selection if moving without shift
            anchor_block = NULL;
            for (Block *t = doc->start; t; t = t->next) { t->sel_start = -1; t->sel_len = 0; }
        }
        
        if (move_r && b->cursor_index < (int)strlen(b->text)) b->cursor_index++;
        if (move_l && b->cursor_index > 0) b->cursor_index--;
        
        moved = true;
    }

    // ------------------------------------------------------------------------
    // Character Insertion (Type to replace selection)
    // ------------------------------------------------------------------------
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            // If text is selected, delete it before typing
            Block *survivor = delete_selected_text(doc);
            if (survivor) b = survivor;

            int len = strlen(b->text);
            b->text = (char*)realloc(b->text, len + 2);
            memmove(&b->text[b->cursor_index + 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
            b->text[b->cursor_index] = (char)key;
            b->cursor_index++;
            *last_action_time = now;
            
            // Clear anchor after typing
            anchor_block = NULL;
        }
        key = GetCharPressed();
    }

    // ------------------------------------------------------------------------
    // Shift+Enter (Soft Break)
    // ------------------------------------------------------------------------
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
        // Delete selection if exists
        Block *survivor = delete_selected_text(doc);
        if (survivor) b = survivor;

        int len = strlen(b->text);
        b->text = (char*)realloc(b->text, len + 2);
        memmove(&b->text[b->cursor_index + 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
        b->text[b->cursor_index] = '\n';
        b->cursor_index++;
        anchor_block = NULL;
    }

    // ------------------------------------------------------------------------
    // Backspace / Delete
    // ------------------------------------------------------------------------
    static double next_del_time = 0;
    bool do_back = false; 
    bool do_del = false;

    if (IsKeyPressed(KEY_BACKSPACE)) { do_back = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_BACKSPACE) && now > next_del_time) { do_back = true; next_del_time = now + 0.05; }

    if (IsKeyPressed(KEY_DELETE)) { do_del = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_DELETE) && now > next_del_time) { do_del = true; next_del_time = now + 0.05; }

    // try selection delete first
    if (do_back || do_del) {
        Block *survivor = delete_selected_text(doc);
        if (survivor != NULL) {
            *last_action_time = now;
            anchor_block = NULL; // Clear anchor after delete
            return survivor; 
        }
    }

    // fallback: normal backspace
    if (do_back) {
        if (b->cursor_index > 0) {
            int len = strlen(b->text);
            memmove(&b->text[b->cursor_index - 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
            b->cursor_index--;
        } 
        else if (b->cursor_index == 0 && b != doc->start) {
            // merge with previous block
            Block *prev = doc->start;
            while (prev->next != b) prev = prev->next;
            
            int prev_len = strlen(prev->text);
            prev->text = (char*)realloc(prev->text, prev_len + strlen(b->text) + 1);
            strcat(prev->text, b->text);
            
            prev->next = b->next;
            if (b == doc->end) doc->end = prev;
            free(b->text); 
            free(b);
            
            prev->cursor_index = prev_len;
            return prev;
        }
    }

    if (do_del && b->cursor_index < (int)strlen(b->text)) {
        int len = strlen(b->text);
        memmove(&b->text[b->cursor_index], &b->text[b->cursor_index + 1], len - b->cursor_index);
    }

    // ------------------------------------------------------------------------
    // Vertical Navigation
    // ------------------------------------------------------------------------
    static double next_vert_time = 0;
    int dir = 0; 
    bool proc_y = false;

    if (IsKeyPressed(KEY_UP)) { dir = -1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_UP) && now > next_vert_time) { dir = -1; proc_y = true; next_vert_time = now + 0.05; }

    if (IsKeyPressed(KEY_DOWN)) { dir = 1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_DOWN) && now > next_vert_time) { dir = 1; proc_y = true; next_vert_time = now + 0.05; }

    if (proc_y) {
        if (!is_shift) { // Clear selection if moving without shift
            anchor_block = NULL;
            for (Block *t = doc->start; t; t = t->next) { t->sel_start = -1; t->sel_len = 0; }
        }
        
        moved = true; // Mark as moved to update selection later

        int current_line = 0; 
        float desired_x = 0; 
        float scan_x = 0;
        int scan_line = 0;
        
        // find current visual position
        for (int i = 0; i < b->cursor_index; i++) {
            float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
            if (b->text[i] == '\n' || scan_x + w > maxWidth) { 
                scan_line++; 
                scan_x = 0; 
                if (b->text[i] == '\n') continue; 
            }
            scan_x += w + 1.0f;
        }
        current_line = scan_line;
        desired_x = scan_x;

        // calc total lines in block
        int total_lines_in_block = 0;
        {
            float tx = 0; int tl = 0;
            for (int i = 0; b->text[i] != '\0'; i++) {
                float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
                if (b->text[i] == '\n' || tx + w > maxWidth) { tl++; tx = 0; if (b->text[i] == '\n') continue; }
                tx += w + 1.0f;
            }
            total_lines_in_block = tl;
        }

        int target_line = current_line + dir;

        // handle block switching
        if (target_line < 0) {
            Block *prev = NULL;
            if (b != doc->start) {
                prev = doc->start;
                while (prev->next != b) prev = prev->next;
            }

            if (prev != NULL) {
                b = prev; 
                int prev_lines = 0;
                float tx = 0; 
                for (int i = 0; b->text[i] != '\0'; i++) {
                    float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
                    if (b->text[i] == '\n' || tx + w > maxWidth) { prev_lines++; tx = 0; if (b->text[i] == '\n') continue; }
                    tx += w + 1.0f;
                }
                target_line = prev_lines; 
            } else {
                target_line = 0; 
            }
        }
        else if (target_line > total_lines_in_block) {
            if (b->next != NULL) {
                b = b->next; 
                target_line = 0; 
            } else {
                target_line = total_lines_in_block; 
            }
        }

        // apply position
        int best_index = b->cursor_index; 
        float min_dist = 100000.0f;
        scan_line = 0; 
        scan_x = 0;
        
        if (strlen(b->text) == 0) best_index = 0;
        else {
            bool found_line = false;
            int len = strlen(b->text);
            for (int i = 0; i <= len; i++) {
                if (scan_line == target_line) {
                    found_line = true;
                    float dist = (desired_x > scan_x) ? (desired_x - scan_x) : (scan_x - desired_x);
                    if (dist < min_dist) { min_dist = dist; best_index = i; }
                }
                else if (scan_line > target_line) break;

                if (i < len) {
                    char c = b->text[i];
                    float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, 20, 1.0f).x;
                    if (c == '\n' || scan_x + w > maxWidth) { 
                        scan_line++; 
                        scan_x = 0; 
                        if (c == '\n') continue; 
                    }
                    scan_x += w + 1.0f;
                }
            }
            if (!found_line && target_line >= scan_line) best_index = len;
        }
        b->cursor_index = best_index;
        *last_action_time = now;
    }

    // --- UPDATE SELECTION IF SHIFT IS HELD ---
    if (is_shift && moved && anchor_block != NULL) {
        update_selection_range(doc, b, b->cursor_index);
    }

    return b;
}

// ============================================================================
// 5. selection state
// ============================================================================

void update_selection_range(Document *doc, Block *current_hover, int current_index) {
    if (anchor_block == NULL || current_hover == NULL) return;

    // reset all
    Block *b = doc->start;
    while (b != NULL) {
        b->sel_start = -1;
        b->sel_len = 0;
        b = b->next;
    }

    // define order (start -> end)
    Block *start_b = anchor_block;
    int start_i = anchor_index;
    Block *end_b = current_hover;
    int end_i = current_index;

    // check inversion (backwards drag)
    if (start_b != end_b) {
        Block *check = doc->start;
        bool found_end_first = false;
        while (check != NULL) {
            if (check == end_b) { found_end_first = true; break; }
            if (check == start_b) break;
            check = check->next;
        }
        if (found_end_first) {
            start_b = current_hover; start_i = current_index;
            end_b = anchor_block;    end_i = anchor_index;
        }
    } else {
        if (start_i > end_i) { int t = start_i; start_i = end_i; end_i = t; }
    }

    // apply range
    Block *curr = start_b;
    bool finished = false;
    
    while (curr != NULL && !finished) {
        int len = strlen(curr->text);
        
        if (curr == end_b) finished = true;

        if (curr == start_b && curr == end_b) {
            curr->sel_start = start_i;
            curr->sel_len = end_i - start_i;
        } 
        else if (curr == start_b) {
            curr->sel_start = start_i;
            curr->sel_len = len - start_i;
        } 
        else if (curr == end_b) {
            curr->sel_start = 0;
            curr->sel_len = end_i;
        } 
        else {
            curr->sel_start = 0;
            curr->sel_len = len;
        }
        curr = curr->next;
    }
}

// ============================================================================
// 6. main loop & rendering
// ============================================================================

int main() {
    InitWindow(800, 600, "text editor in c");
    SetTargetFPS(60);

    Document *my_doc = create_document();
    add_block(my_doc, "click here to edit...");
    Block *block_focus = NULL;

    double last_action_time = GetTime();

    while (!WindowShouldClose()) {
        if (block_focus != NULL) {
            block_focus = update_typing(my_doc, block_focus, &last_action_time);
            
            // HARD ENTER: Split block logic
            if (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                
                // 1. clean selection
                Block *survivor = delete_selected_text(my_doc);
                if (survivor != NULL) block_focus = survivor;

                // 2. prepare text move
                int split_index = block_focus->cursor_index;
                char *text_moving_down = &block_focus->text[split_index];

                // 3. insert new block
                insert_block_after(my_doc, block_focus, text_moving_down);

                // 4. cut current block
                block_focus->text[split_index] = '\0';
                
                // 5. move focus
                block_focus = block_focus->next;
                block_focus->cursor_index = 0; 
                last_action_time = GetTime();
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        Block *current = my_doc->start;
        int y = 20;
        int fontSize = 20, lineHeight = 24, maxWidth = 680; 

        while (current != NULL) {
            int pad = 4;
            int gap = 2;

            // ----------------------------------------------------------------
            // a. height calculation (simulation)
            // ----------------------------------------------------------------
            int vis_lines = 1; float x_count = 0;
            for (int i = 0; current->text[i] != '\0'; i++) {
                char c = current->text[i];
                if (c == '\n') { vis_lines++; x_count = 0; continue; } // skip \n measurement

                float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, (float)fontSize, 1.0f).x;
                if (x_count + w > maxWidth) { vis_lines++; x_count = 0; } 
                x_count += w + 1.0f; 
            }
            int b_height = (vis_lines * lineHeight) + (pad * 2);
            Rectangle b_area = {0, (float)y, 800, (float)(b_height + gap)};
            bool mouse_above = CheckCollisionPointRec(GetMousePosition(), b_area);

            // ----------------------------------------------------------------
            // b. mouse interaction
            // ----------------------------------------------------------------
            Vector2 mouse = GetMousePosition();
            float local_mouse_x = mouse.x - 60;
            float local_mouse_y = mouse.y - (y + pad);
            int char_index_under_mouse = 0;

            // find char index under mouse
            {
                int sim_line = 0; float sim_x = 0;
                int text_len = strlen(current->text);
                float min_dist = 100000.0f;
                bool is_left_margin = (mouse.x < 60);

                for (int i = 0; i <= text_len; i++) {
                    float line_top = sim_line * lineHeight;
                    float line_bottom = line_top + lineHeight;
                    bool mouse_on_this_line = (local_mouse_y >= line_top && local_mouse_y < line_bottom);
                    if (i == text_len && local_mouse_y >= line_bottom) mouse_on_this_line = true;

                    if (mouse_on_this_line) {
                        if (is_left_margin && sim_x == 0) { char_index_under_mouse = i; break; }
                        float dist = (local_mouse_x > sim_x) ? (local_mouse_x - sim_x) : (sim_x - local_mouse_x);
                        if (dist < min_dist) { min_dist = dist; char_index_under_mouse = i; }
                    }

                    if (i < text_len) {
                        char c = current->text[i];
                        if (c == '\n') { sim_line++; sim_x = 0; continue; } 
                        
                        float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, (float)fontSize, 1.0f).x;
                        if (sim_x + w > maxWidth) { sim_line++; sim_x = 0; }
                        sim_x += w + 1.0f;
                    }
                }
            }

            // handle clicks & drags
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_above) {
                block_focus = current;
                last_action_time = GetTime();
                current->cursor_index = char_index_under_mouse;
                anchor_block = current;
                anchor_index = char_index_under_mouse;
                update_selection_range(my_doc, current, char_index_under_mouse);
            }
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && anchor_block != NULL) {
                 if (mouse_above) {
                    current->cursor_index = char_index_under_mouse; 
                    update_selection_range(my_doc, current, char_index_under_mouse);
                 }
            }

            // ----------------------------------------------------------------
            // c. rendering (text & selection)
            // ----------------------------------------------------------------
            int current_line = 0; float x_offset = 0;
            Vector2 cur_pos = { 60, (float)y + pad };
            
            // fix for empty block selection
            if (strlen(current->text) == 0) {
                if (current->sel_start != -1) DrawRectangle(60, y + pad, 10, lineHeight, (Color){100, 200, 255, 150});
                cur_pos = (Vector2){ 60, (float)(y + pad) };
            }

            for (int i = 0; current->text[i] != '\0'; i++) {
                if (i == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };
                }

                char c = current->text[i];

                // handle newline (skip measurement)
                if (c == '\n') {
                    if (current->sel_start != -1 && i >= current->sel_start && i < current->sel_start + current->sel_len) {
                            DrawRectangle(60 + x_offset, y + pad + (current_line * lineHeight), 5, lineHeight, (Color){100, 200, 255, 150});
                    }
                    current_line++; 
                    x_offset = 0;
                    if (i + 1 == current->cursor_index) {
                        cur_pos = (Vector2){ 60, (float)((int)(y + pad + (current_line * lineHeight))) };
                    }
                    continue; 
                }

                // handle normal char
                float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, (float)fontSize, 1.0f).x;
                if (x_offset + w > maxWidth) {
                    current_line++; 
                    x_offset = 0;
                }

                Vector2 pos = { (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };

                // selection bg
                if (current->sel_start != -1) {
                    if (i >= current->sel_start && i < current->sel_start + current->sel_len) {
                        DrawRectangle((int)pos.x, (int)pos.y, (int)w + 1, lineHeight, (Color){100, 200, 255, 150});
                    }
                }

                // draw char
                DrawTextEx(GetFontDefault(), (char[2]){c, '\0'}, pos, (float)fontSize, 1.0f, BLACK);
                x_offset += w + 1.0f;

                if (i + 1 == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };
                }
            }

            // draw blinking cursor
            if (current == block_focus) {
                double time_since_action = GetTime() - last_action_time;
                bool show_cursor = (time_since_action < 0.6) || ((int)(GetTime() * 2) % 2 == 0);
                if (show_cursor) DrawRectangle((int)cur_pos.x, (int)cur_pos.y, 2, fontSize, BLACK);
            }

            // next block jump
            y += b_height + gap;
            current = current->next;
        }
        EndDrawing();
    }
    free_document(my_doc);
    CloseWindow();
    return 0;
}