#ifndef LRU_H
#define LRU_H

#include <stddef.h>
#include <pthread.h>

typedef struct LRUCache LRUCache;

LRUCache* lru_create(size_t capacity);
void lru_put(LRUCache* cache, const char* key, const char* value);
const char* lru_get(LRUCache* cache, const char* key);
char* lru_to_json(LRUCache* cache); // returns malloc'd JSON string
void lru_free(LRUCache* cache);

#endif

