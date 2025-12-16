#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"

typedef struct Block {
    int id;
    char *text;
    struct Block *next;
} Block;

typedef struct {
    Block *start;
    Block *end;
    int id_counter;
} Document;

Document* create_document() {
    Document *doc = (Document*) malloc(sizeof(Document));
    if (doc == NULL) {
        return NULL; // em caso de faltar memória.
    }
    doc->start = NULL;
    doc->end = NULL;
    doc->id_counter = 0;
    return doc;
}

Block* create_block(int id, char *text_content) {
    // Aloca o bloco
    Block *new_block = (Block*) malloc(sizeof(Block));
    // Preenche os dados
    new_block->id = id;
    new_block->text = strdup(text_content);
    new_block->next = NULL;
    return new_block;
}

void add_block(Document *doc, char *text) {
    // Incrementa o contador do documento para gerar ID único
    doc->id_counter++;
    // Cria o bloco novo
    Block *new_b = create_block(doc->id_counter, text);
    
    // Lógica da lista encadeada:
    if (doc->start == NULL) {
        doc->start = new_b;
        doc->end = new_b;
    } else {
        doc->end->next = new_b;
        doc->end = new_b;
    }
}
// visualizar o documento no terminal
void print_document(Document *doc) {
    printf("\n--- VISUALIZADOR DO DOCUMENTO ---\n");
    
    Block *current = doc->start;
    
    while(current != NULL) {
        printf("[ID %d] %s\n", current->id, current->text);
        current = current->next;
    }
    printf("---------------------------------\n");
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

// inserir caracteres no texto , função auxiliar
void append_char(Block *b, char c) {
    int len = strlen(b->text);
    b->text = (char*) realloc(b->text, len + 2); // +1 pro char, +1 pro \0
    b->text[len] = c;
    b->text[len + 1] = '\0';
}

void atualizar_texto_digitado(Block *b) {
// write
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125)) {
            int len = strlen(b->text);
            b->text = (char*) realloc(b->text, len + 2);
            b->text[len] = (char)key;
            b->text[len + 1] = '\0';
        }
        key = GetCharPressed();
    }
// enter
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
        int len = strlen(b->text);
        b->text = (char*) realloc(b->text, len + 2);
        b->text[len] = '\n';
        b->text[len + 1] = '\0';
    }
// backspace
    static double after_clicking_enter = 0; 
    double enter_delta_time = GetTime();

    bool delete = false;

    if (IsKeyPressed(KEY_BACKSPACE)) {
        delete = true;
        after_clicking_enter = enter_delta_time + 0.5;
    }

    else if (IsKeyDown(KEY_BACKSPACE)) {
        if (enter_delta_time > after_clicking_enter) {
            delete = true;
            after_clicking_enter = enter_delta_time + 0.04;
        }
    }

    if (delete) {
        int len = strlen(b->text);
        if (len > 0) {
            b->text[len - 1] = '\0';
        }
    }
}

int main() {
    InitWindow(800, 600, "texto editor em C");
    SetTargetFPS(60);

    Document *my_doc = create_document();
    add_block(my_doc, "Clique aqui para editar...");
    
    Block *block_focus = NULL;

    while (!WindowShouldClose()) {
        
        if (block_focus != NULL) {
            atualizar_texto_digitado(block_focus);

            if (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                add_block(my_doc, "");     // cria um bloco novo
                block_focus = my_doc->end;  // muda o foco para esse bloco
            }
        }

// drawing (parte gráfica)
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Block *current = my_doc->start;
        int y = 20;

        while (current != NULL) {
            // --- 1. CÁLCULO DE ALTURA DINÂMICA ---
            // Conta quantos "Shift+Enter" (\n) existem no texto
            int linhas = 1;
            for(int i=0; current->text[i] != '\0'; i++) {
                if(current->text[i] == '\n') linhas++;
            }
            
            // Cada linha ocupa 20px. Adicionamos 20px de margem (padding)
            int altura_real = (linhas * 20) + 20;

            // Usamos a altura calculada na hitbox
            Rectangle area_bloco = {50, y, 700, altura_real};
            
            // --- DETECTAR CLIQUE ---
            if (CheckCollisionPointRec(GetMousePosition(), area_bloco)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    block_focus = current;
                }
            }

            // --- DESENHAR ---
            if (current == block_focus) {
                DrawRectangleLinesEx(area_bloco, 2, BLUE);
            } else {
                DrawRectangleLines(50, y, 700, altura_real, LIGHTGRAY);
            }

            DrawText(current->text, 60, y + 10, 20, BLACK);
            
            // --- CURSOR (Atenção aqui) ---
            if (current == block_focus) {
                // BUG DO CURSOR EM MULTILINHA:
                // O MeasureText mede a string inteira. Se tiver quebra de linha, 
                // o cursor vai parar lá longe na direita.
                // Para consertar perfeito daria muito trabalho agora. 
                // Por enquanto, ele vai aparecer, mas pode ficar posicionado estranho em multilinhas.
                
                int largura_texto = MeasureText(current->text, 20);
                
                if ((int)(GetTime() * 2) % 2 == 0) {
                     // Nota: Se for multiline, o cursor vai aparecer na primeira linha lá longe.
                     // Para um projeto V1, isso é aceitável.
                    DrawRectangle(60 + largura_texto + 2, y + 10, 2, 20, BLACK);
                }
            }

            // --- PULO DINÂMICO ---
            // O próximo bloco começa onde este termina + 10px de respiro
            y += altura_real + 10;
            
            current = current->next;
        }

        EndDrawing();
    }
    
    free_document(my_doc);
    CloseWindow();
    return 0;
}
