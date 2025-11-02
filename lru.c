#include "lru.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct Node {
    char* key;
    char* value;
    struct Node* prev;
    struct Node* next;
} Node;

struct LRUCache {
    size_t capacity;
    size_t size;
    Node* head;
    Node* tail;
    pthread_mutex_t lock;
};

static Node* node_create(const char* key, const char* value) {
    Node* n = malloc(sizeof(Node));
    n->key = strdup(key ? key : "");
    n->value = strdup(value ? value : "");
    n->prev = n->next = NULL;
    return n;
}

static void move_to_head(LRUCache* c, Node* n) {
    if (!c || !n) return;
    if (c->head == n) return;

    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    if (c->tail == n) c->tail = n->prev;

    n->prev = NULL;
    n->next = c->head;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (!c->tail) c->tail = n;
}

LRUCache* lru_create(size_t capacity) {
    LRUCache* c = malloc(sizeof(LRUCache));
    c->capacity = capacity;
    c->size = 0;
    c->head = c->tail = NULL;
    pthread_mutex_init(&c->lock, NULL);
    return c;
}

const char* lru_get(LRUCache* c, const char* key) {
    if (!key) return NULL;
    pthread_mutex_lock(&c->lock);
    Node* n = c->head;
    while (n) {
        if (strcmp(n->key, key) == 0) {
            move_to_head(c, n);
            const char* v = n->value;
            pthread_mutex_unlock(&c->lock);
            return v;
        }
        n = n->next;
    }
    pthread_mutex_unlock(&c->lock);
    return NULL;
}

void lru_put(LRUCache* c, const char* key, const char* value) {
    if (!key || !value) return;
    pthread_mutex_lock(&c->lock);
    Node* n = c->head;
    while (n) {
        if (strcmp(n->key, key) == 0) {
            free(n->value);
            n->value = strdup(value);
            move_to_head(c, n);
            pthread_mutex_unlock(&c->lock);
            return;
        }
        n = n->next;
    }

    Node* new_node = node_create(key, value);
    new_node->next = c->head;
    if (c->head) c->head->prev = new_node;
    c->head = new_node;
    if (!c->tail) c->tail = new_node;
    c->size++;

    if (c->size > c->capacity) {
        Node* tail = c->tail;
        c->tail = tail->prev;
        if (c->tail) c->tail->next = NULL;
        free(tail->key);
        free(tail->value);
        free(tail);
        c->size--;
    }
    pthread_mutex_unlock(&c->lock);
}

char* lru_to_json(LRUCache* c) {
    pthread_mutex_lock(&c->lock);
    Node* n = c->head;
    size_t bufsize = 2048;
    char* buf = malloc(bufsize);
    if (!buf) { pthread_mutex_unlock(&c->lock); return strdup("[]"); }
    strcpy(buf, "[");

    while (n) {
        char entry[256];
        snprintf(entry, sizeof(entry),
                 "{\"key\":\"%s\",\"value\":\"%s\"}%s",
                 n->key, n->value, n->next ? "," : "");
        if (strlen(buf) + strlen(entry) + 2 > bufsize) {
            bufsize *= 2;
            buf = realloc(buf, bufsize);
        }
        strcat(buf, entry);
        n = n->next;
    }
    strcat(buf, "]");
    pthread_mutex_unlock(&c->lock);
    return buf;
}

void lru_free(LRUCache* c) {
    Node* n = c->head;
    while (n) {
        Node* next = n->next;
        free(n->key);
        free(n->value);
        free(n);
        n = next;
    }
    pthread_mutex_destroy(&c->lock);
    free(c);
}

