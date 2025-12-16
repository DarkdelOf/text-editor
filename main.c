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

int main() {
    Document *my_doc = create_document();
    
    if (my_doc == NULL) {
        printf("Erro fatal: Sem memória RAM.\n");
        return 1;
    }
    char digitacao[100];
    fgets(digitacao, sizeof(digitacao), stdin);
    char digitacao2[100];
    fgets(digitacao2, sizeof(digitacao2), stdin);

    add_block(my_doc, digitacao);
    add_block(my_doc, digitacao2);
    
    print_document(my_doc);
    
    free_document(my_doc);
    return 0;
}
