#ifndef __FSCACHE_H__
#define __FSCACHE_H__

#include <stdbool.h>
#include "hashtable.h"

typedef struct CacheNode {
    short key;
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

CacheNode* InitCacheNode(short key, void* val);

CacheNode* PutItemInCache(Cache* cache, short key, void* value);

void* GetItemFromCache(Cache* cache, short key);

void SetHead(Cache* cache, CacheNode* node);

void RemoveNode(Cache* cache, CacheNode* node);

void SetDirty(Cache* cache, short key);

#endif
