#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"

// data structures
typedef struct Block {
    int id;
    char *text;
    int cursor_index;
    struct Block *next;
} Block;

typedef struct {
    Block *start;
    Block *end;
    int id_counter;
} Document;

// memory management
Document* create_document() {
    Document *doc = (Document*) malloc(sizeof(Document));
    if (doc == NULL) return NULL;
    doc->start = NULL;
    doc->end = NULL;
    doc->id_counter = 0;
    return doc;
}

Block* create_block(int id, char *text_content) {
    Block *new_block = (Block*) malloc(sizeof(Block));
    new_block->id = id;
    new_block->text = strdup(text_content);
    new_block->next = NULL;
    new_block->cursor_index = strlen(text_content);
    return new_block;
}

void add_block(Document *doc, char *text) {
    doc->id_counter++;
    Block *new_b = create_block(doc->id_counter, text);
    if (doc->start == NULL) {
        doc->start = new_b;
        doc->end = new_b;
    } else {
        doc->end->next = new_b;
        doc->end = new_b;
    }
}

void delete_block(Document *doc, Block *target) {
    if (doc == NULL || target == NULL) return;

    if (target == doc->start) {
        doc->start = target->next;
        if (doc->start == NULL) {
            doc->end = NULL;
        }
    } 

    else {
        Block *prev = doc->start;
        while (prev != NULL && prev->next != target) {
            prev = prev->next;
        }

        if (prev != NULL) {
            prev->next = target->next;
            if (target == doc->end) {
                doc->end = prev;
            }
        }
    }

    free(target->text);
    free(target);
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

// typing logic
Block* update_typing(Document *doc, Block *b){
    double now = GetTime();
    int maxWidth = 680;

    // horizontal move timers
    static double next_horiz_time = 0; 
    bool move_l = false, move_r = false;

    if (IsKeyPressed(KEY_RIGHT)) { move_r = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_RIGHT) && now > next_horiz_time) { move_r = true; next_horiz_time = now + 0.04; }

    if (IsKeyPressed(KEY_LEFT)) { move_l = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_LEFT) && now > next_horiz_time) { move_l = true; next_horiz_time = now + 0.04; }

    if (move_r && b->cursor_index < (int)strlen(b->text)) b->cursor_index++;
    if (move_l && b->cursor_index > 0) b->cursor_index--;

    // character insertion
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125)) {
            int len = strlen(b->text);
            b->text = (char*) realloc(b->text, len + 2);
            memmove(&b->text[b->cursor_index + 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
            b->text[b->cursor_index] = (char)key;
            b->cursor_index++;
        }
        key = GetCharPressed();
    }

    // line break
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
        int len = strlen(b->text);
        b->text = (char*) realloc(b->text, len + 2);
        memmove(&b->text[b->cursor_index + 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
        b->text[b->cursor_index] = '\n';
        b->cursor_index++;
    }

    // backspace and delete
    static double next_del_time = 0;
    bool do_back = false, do_del = false;

    if (IsKeyPressed(KEY_BACKSPACE)) { do_back = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_BACKSPACE) && now > next_del_time) { do_back = true; next_del_time = now + 0.05; }

    if (IsKeyPressed(KEY_DELETE)) { do_del = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_DELETE) && now > next_del_time) { do_del = true; next_del_time = now + 0.05; }

    if (do_back) {
        if (b->cursor_index > 0) {
            int len = strlen(b->text);
            memmove(&b->text[b->cursor_index - 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
            b->cursor_index--;
        } 
        else if (b->cursor_index == 0 && b != doc->start) {
            Block *prev = doc->start;
            while (prev->next != b) {
                prev = prev->next;
            }

            int prev_len = strlen(prev->text);
            int curr_len = strlen(b->text);
            
            prev->text = (char*) realloc(prev->text, prev_len + curr_len + 1);
            
            strcat(prev->text, b->text);

            delete_block(doc, b);

            prev->cursor_index = prev_len;
            return prev; 
        }
    }

    if (do_del && b->cursor_index < (int)strlen(b->text)) {
        int len = strlen(b->text);
        memmove(&b->text[b->cursor_index], &b->text[b->cursor_index + 1], len - b->cursor_index);
    }

    // vertical navigation
    static double next_vert_time = 0;
    int dir = 0; bool proc_y = false;

    if (IsKeyPressed(KEY_UP)) { dir = -1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_UP) && now > next_vert_time) { dir = -1; proc_y = true; next_vert_time = now + 0.05; }

    if (IsKeyPressed(KEY_DOWN)) { dir = 1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_DOWN) && now > next_vert_time) { dir = 1; proc_y = true; next_vert_time = now + 0.05; }

    if (proc_y) {
        int cursor_line = 0; float cursor_x = 0;
        for (int i = 0; i < b->cursor_index; i++) {
            float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
            if (b->text[i] == '\n' || cursor_x + w > maxWidth) { cursor_line++; cursor_x = 0; if (b->text[i] == '\n') continue; }
            cursor_x += w + 1.0f;
        }
        int target_line = cursor_line + dir;
        if (target_line >= 0) {
            int best_index = b->cursor_index; float min_dist = 9999.0f;
            int scan_line = 0; float scan_x = 0;
            for (int i = 0; i <= (int)strlen(b->text); i++) {
                if (scan_line == target_line) {
                    float dist = (cursor_x > scan_x) ? (cursor_x - scan_x) : (scan_x - cursor_x);
                    if (dist < min_dist) { min_dist = dist; best_index = i; }
                }
                if (b->text[i] == '\0') break;
                float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
                if (b->text[i] == '\n' || scan_x + w > maxWidth) { scan_line++; scan_x = 0; if (b->text[i] == '\n') continue; }
                scan_x += w + 1.0f;
            }
            b->cursor_index = best_index;
        }
    }
    return b;
}

// main system
int main() {
    InitWindow(800, 600, "text editor in c");
    SetTargetFPS(60);

    Document *my_doc = create_document();
    add_block(my_doc, "click here to edit...");
    Block *block_focus = NULL;

    while (!WindowShouldClose()) {
        if (block_focus != NULL) {
            block_focus = update_typing(my_doc, block_focus);
            if (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                add_block(my_doc, ""); 
                block_focus = my_doc->end;
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

            // 1. calculate block height
            int vis_lines = 1; float x_count = 0;
            for (int i = 0; current->text[i] != '\0'; i++) {
                float w = MeasureTextEx(GetFontDefault(), (char[2]){current->text[i], '\0'}, (float)fontSize, 1.0f).x;
                if (current->text[i] == '\n' || x_count + w > maxWidth) { vis_lines++; x_count = 0; if (current->text[i] == '\n') continue; }
                x_count += w + 1.0f; 
            }

            int b_height = (vis_lines * lineHeight) + (pad * 2);
            Rectangle b_area = {50, (float)y, 700, (float)b_height};
            bool mouse_above = CheckCollisionPointRec(GetMousePosition(), b_area);

            // 2. interaction and hover
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_above) block_focus = current;
            if (mouse_above) DrawRectangleRec(b_area, (Color){ 235, 235, 235, 255 }); 

            // 3. render text and cursor position
            int current_line = 0; float x_offset = 0;
            Vector2 cur_pos = { 60, (float)y + pad };
            
            for (int i = 0; current->text[i] != '\0'; i++) {
                // set cursor pos if match
                if (i == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };
                }

                char c = current->text[i];
                float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, (float)fontSize, 1.0f).x;

                // wrap logic
                if (c == '\n' || x_offset + w > maxWidth) {
                    current_line++; x_offset = 0;
                    if (c == '\n') {
                        if (i + 1 == current->cursor_index) cur_pos = (Vector2){ 60, (float)((int)(y + pad + (current_line * lineHeight))) };
                        continue; 
                    }
                }

                // draw char
                Vector2 pos = { (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };
                DrawTextEx(GetFontDefault(), (char[2]){c, '\0'}, pos, (float)fontSize, 1.0f, BLACK);
                x_offset += w + 1.0f;

                // handle cursor at the very end
                if (i + 1 == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_offset)), (float)((int)(y + pad + (current_line * lineHeight))) };
                }
            }

            // 4. draw blinking cursor
            if (current == block_focus && (int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle((int)cur_pos.x, (int)cur_pos.y, 2, fontSize, BLACK);
            }

            // 5. next block jump
            y += b_height + gap;
            current = current->next;
        }
        EndDrawing();
    }
    free_document(my_doc);
    CloseWindow();
    return 0;
}