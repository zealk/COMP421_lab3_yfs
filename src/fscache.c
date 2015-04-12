#include "../include/fscache.h"

Cache* InitCache(int capacity) {
    Cache* cache = calloc(1, sizeof(Cache));
    cache->capacity = capacity;
    cache->table = InitHashTable(capacity);

    return cache;
}

CacheNode* InitCacheNode(int key, void* val) {
    CacheNode* node = calloc(1, sizeof(CacheNode));
    node->key = key;
    node->value = val;

    return node;
}

void PutItemInCache(Cache* cache, int key, void* value) {
    CacheNode* node = GetItemFromHashTable(cache->table, key);

    if (node != NULL) {
        node->value = value;

        if (node != cache->head) {

        }
    } else {
        node = InitCacheNode(key, value);
        PutItemInHashTable(cache->table, key, (void*)node);

        if (cache->len < cache->capacity) {
            SetHead(cache, node);
            ++cache->len;
        } else {
            int remove_key = cache->tail->key;
            RemoveNode(cache, cache->tail);
            RemoveItemFromHashTable(cache->table, remove_key);
            SetHead(cache, node);
        }
    }
}

void* GetItemFromCache(Cache* cache, int key) {
    CacheNode* node = GetItemFromHashTable(cache->table, key);
    if (node == NULL) {
        return NULL;
    }
    
    if (cache->head != node) {
        RemoveNode(cache, node);
        SetHead(cache, node);
    }

    return node->value;
}

void SetHead(Cache* cache, CacheNode* node) {
    node->next = cache->head;
    node->prev = NULL;

    if (cache->head != NULL) {
        cache->head->prev = node;
    }

    cache->head = node;

    if (cache->tail == NULL) {
        cache->tail = node;
    }
}

void RemoveNode(Cache* cache, CacheNode* node) {
    CacheNode* prev = node->prev;
    CacheNode* next = node->next;

    if (prev != NULL) {
        prev->next = NULL;
    } else {
        cache->head = next;
    }

    if (next != NULL) {
        next->prev = prev;
    } else {
        cache->tail = prev;
    }
}