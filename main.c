#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"

// --- ESTRUTURAS DE DADOS ---

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

// --- GERENCIAMENTO DE MEMÓRIA ---

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

// --- LÓGICA DE INPUT (TECLADO) ---

void atualizar_texto_digitado(Block *b) {
    double now = GetTime();

    // 1. MOVIMENTAÇÃO HORIZONTAL (Acelerada)
    static double next_horiz_time = 0; 
    bool move_l = false, move_r = false;

    if (IsKeyPressed(KEY_RIGHT)) { move_r = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_RIGHT) && now > next_horiz_time) { move_r = true; next_horiz_time = now + 0.04; }

    if (IsKeyPressed(KEY_LEFT)) { move_l = true; next_horiz_time = now + 0.4; }
    else if (IsKeyDown(KEY_LEFT) && now > next_horiz_time) { move_l = true; next_horiz_time = now + 0.04; }

    if (move_r && b->cursor_index < (int)strlen(b->text)) b->cursor_index++;
    if (move_l && b->cursor_index > 0) b->cursor_index--;

    // 2. ESCRITA E INSERÇÃO
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

    // 3. QUEBRA DE LINHA (Shift + Enter)
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
        int len = strlen(b->text);
        b->text = (char*) realloc(b->text, len + 2);
        memmove(&b->text[b->cursor_index + 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
        b->text[b->cursor_index] = '\n';
        b->cursor_index++;
    }

    // 4. BACKSPACE E DELETE (Acelerados)
    static double next_del_time = 0;
    bool do_back = false, do_del = false;

    if (IsKeyPressed(KEY_BACKSPACE)) { do_back = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_BACKSPACE) && now > next_del_time) { do_back = true; next_del_time = now + 0.05; }

    if (IsKeyPressed(KEY_DELETE)) { do_del = true; next_del_time = now + 0.5; }
    else if (IsKeyDown(KEY_DELETE) && now > next_del_time) { do_del = true; next_del_time = now + 0.05; }

    if (do_back && b->cursor_index > 0) {
        int len = strlen(b->text);
        memmove(&b->text[b->cursor_index - 1], &b->text[b->cursor_index], len - b->cursor_index + 1);
        b->cursor_index--;
    }
    if (do_del && b->cursor_index < (int)strlen(b->text)) {
        int len = strlen(b->text);
        memmove(&b->text[b->cursor_index], &b->text[b->cursor_index + 1], len - b->cursor_index);
    }

    // 5. MOVIMENTAÇÃO VERTICAL (Inteligente/Acelerada)
    static double next_vert_time = 0;
    int dir = 0;
    bool proc_y = false;

    if (IsKeyPressed(KEY_UP)) { dir = -1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_UP) && now > next_vert_time) { dir = -1; proc_y = true; next_vert_time = now + 0.05; }

    if (IsKeyPressed(KEY_DOWN)) { dir = 1; proc_y = true; next_vert_time = now + 0.4; }
    else if (IsKeyDown(KEY_DOWN) && now > next_vert_time) { dir = 1; proc_y = true; next_vert_time = now + 0.05; }

    if (proc_y) {
        int linha_at = 0; float x_at = 0;
        for (int i = 0; i < b->cursor_index; i++) {
            float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
            if (b->text[i] == '\n' || x_at + w > 685) { linha_at++; x_at = 0; if (b->text[i] == '\n') continue; }
            x_at += w + 1.0f;
        }

        int target = linha_at + dir;
        if (target >= 0) {
            int melhor_i = b->cursor_index; float menor_d = 9999.0f;
            int l_scan = 0; float x_scan = 0;
            for (int i = 0; i <= (int)strlen(b->text); i++) {
                if (l_scan == target) {
                    float d = (x_at > x_scan) ? (x_at - x_scan) : (x_scan - x_at);
                    if (d < menor_d) { menor_d = d; melhor_i = i; }
                }
                if (b->text[i] == '\0') break;
                float w = MeasureTextEx(GetFontDefault(), (char[2]){b->text[i], '\0'}, 20, 1.0f).x;
                if (b->text[i] == '\n' || x_scan + w > 685) { l_scan++; x_scan = 0; if (b->text[i] == '\n') continue; }
                x_scan += w + 1.0f;
            }
            b->cursor_index = melhor_i;
        }
    }
}

// --- SISTEMA PRINCIPAL ---

int main() {
    InitWindow(800, 600, "texto editor em C");
    SetTargetFPS(60);

    Document *my_doc = create_document();
    add_block(my_doc, "Clique aqui para editar...");
    Block *block_focus = NULL;

    while (!WindowShouldClose()) {
        
        // Lógica de Documento
        if (block_focus != NULL) {
            atualizar_texto_digitado(block_focus);

            if (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                add_block(my_doc, ""); 
                block_focus = my_doc->end;
            }
        }

        // --- RENDERIZAÇÃO (PARTE GRÁFICA) ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Block *current = my_doc->start;
        int y = 20;
        int fontSize = 20, lineHeight = 24, maxWidth = 680; 

        while (current != NULL) {
            // 1. Cálculo de Layout (Altura da Box)
            int vis_lines = 1; float x_c = 0;
            for (int i = 0; current->text[i] != '\0'; i++) {
                float w = MeasureTextEx(GetFontDefault(), (char[2]){current->text[i], '\0'}, (float)fontSize, 1.0f).x;
                if (current->text[i] == '\n' || x_c + w > maxWidth) { vis_lines++; x_c = 0; if (current->text[i] == '\n') continue; }
                x_c += w + 1.0f; 
            }

            int b_height = (vis_lines * lineHeight) + 20;
            Rectangle b_area = {50, (float)y, 700, (float)b_height};

            // 2. HITBOX E HOVER
            bool mouse_above = CheckCollisionPointRec(GetMousePosition(), b_area);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_above) {
                block_focus = current;
            }

            // 3. DESENHO DO FUNDO (SÓ NO HOVER)
            if (mouse_above) {
                DrawRectangleRec(b_area, (Color){ 235, 235, 235, 255 }); 
            }

            if (current == block_focus && !mouse_above) {
            }

            // 4. Desenho do Texto e Cursor
            int c_line = 0; float x_off = 0;
            Vector2 cur_pos = { 60, (float)y + 10 };
            
            for (int i = 0; current->text[i] != '\0'; i++) {
                if (i == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_off)), (float)((int)(y + 10 + (c_line * lineHeight))) };
                }

                char c = current->text[i];
                float w = MeasureTextEx(GetFontDefault(), (char[2]){c, '\0'}, (float)fontSize, 1.0f).x;

                if (c == '\n' || x_off + w > maxWidth) {
                    c_line++;
                    x_off = 0;
                    if (c == '\n') {
                        if (i + 1 == current->cursor_index) {
                            cur_pos = (Vector2){ 60, (float)((int)(y + 10 + (c_line * lineHeight))) };
                        }
                        continue; 
                    }
                }

                Vector2 pos = { (float)((int)(60 + x_off)), (float)((int)(y + 10 + (c_line * lineHeight))) };
                DrawTextEx(GetFontDefault(), (char[2]){c, '\0'}, pos, (float)fontSize, 1.0f, BLACK);
                x_off += w + 1.0f;

                if (i + 1 == current->cursor_index) {
                    cur_pos = (Vector2){ (float)((int)(60 + x_off)), (float)((int)(y + 10 + (c_line * lineHeight))) };
                }
            }

            // 5. Cursor Piscante
            if (current == block_focus && (int)(GetTime() * 2) % 2 == 0) {
                DrawRectangle((int)cur_pos.x, (int)cur_pos.y, 2, fontSize, BLACK);
            }

            y += b_height + 10;
            current = current->next;
        }
        EndDrawing();
    }

    free_document(my_doc);
    CloseWindow();
    return 0;
}