#include "../include/fscache.h"
#include <stdlib.h>

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

CacheNode* PutItemInCache(Cache* cache, int key, void* value) {
    CacheNode* node = GetItemFromHashTable(cache->table, key);

    if (node != NULL) {
        node->value = value;

        if (node != cache->head) {
            RemoveNode(cache, node);
            SetHead(cache, node);
        }
    } else {
        node = InitCacheNode(key, value);
        PutItemInHashTable(cache->table, key, (void*)node);

        if (cache->len < cache->capacity) {
            SetHead(cache, node);
            ++cache->len;
        } else {
            CacheNode* tail = cache->tail;
            RemoveNode(cache, tail);
            RemoveItemFromHashTable(cache->table, tail->key);
            SetHead(cache, node);

            if (tail->dirty) {
                return tail;
            }

            free(tail);
        }
    }

    return NULL;
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

void SetDirty(Cache* cache, int key) {
    CacheNode* node = GetItemFromHashTable(cache->table, key);

    if (node != NULL) {
        node->dirty = true;

        if (cache->head != node) {
            RemoveNode(cache, node);
            SetHead(cache, node);
        }
    }
}

/* init cache */
inode_cache = InitCache(INODE_CACHESIZE);
block_cache = InitCache(BLOCK_CACHESIZE);

void WriteBackInode(CacheNode* inode) {
    /* Get block number */
    int bnum = GetBlockNumFromInodeNum(inode->key);

    void* block = GetItemFromCache(block_cache, bnum);
    if (block == NULL) {
        int code = ReadSector(bnum, block);
        if (code == ERROR) {
            /* Maybe it needs to do other things here */
            printf("Read Sector #%d failed\n", bnum);
            free(inode);
            return;
        }

        /* Cache block */
        CacheNode* block_cache_node = PutItemInCache(block_cache, bnum, block);
        if (block_cache_node != NULL) {
            WriteBackBlock(block_cache_node);
        }
    }

    /* Save inode in the block and set dirty bit for that block */
    int num_inode_per_block = BLOCKSIZE / INODESIZE;
    memcpy((struct inode*)block + inode->key % num_inode_per_block, (struct inode*)(inode->value), sizeof(struct inode));
    SetDirty(block_cache, bnum);
    free(inode);
}

void WriteBackBlock(CacheNode* block) {
    int code = WriteSector(block->key, block->value);
    free(block);

    /* Maybe it needs to do other things here */
    if (code == ERROR) {
        printf("Write Sector #%d failed\n", block->key);
    }
}

int GetBlockNumFromInodeNum(int inum) {
    int num_inode_per_block = BLOCKSIZE / INODESIZE;
    return 1 + inum / num_inode_per_block;
}