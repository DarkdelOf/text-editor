#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(){
}
