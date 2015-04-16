#ifndef __FSCACHE_H__
#define __FSCACHE_H__

#include <stdbool.h>
#include "hashtable.h"

typedef struct CacheNode {
    int key;
    void* value;
    struct CacheNode* prev;
    struct CacheNode* next;
    bool dirty;
} CacheNode;

typedef struct Cache {
    CacheNode* head;
    CacheNode* tail;
    int capacity;
    int len;
    HashTable* table;
} Cache;

Cache* InitCache(int capacity);

CacheNode* InitCacheNode(int key, void* val);

CacheNode* PutItemInCache(Cache* cache, int key, void* value);

void* GetItemFromCache(Cache* cache, int key);

void SetHead(Cache* cache, CacheNode* node);

void RemoveNode(Cache* cache, CacheNode* node);

void SetDirty(Cache* cache, int key);

#endif
