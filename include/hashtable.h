/*
 * An open source hash table implementation
 * http://git.loftor.com/study/c.git/tree/HashTable
 */

#ifndef _hashtable_h
#define _hashtable_h

typedef struct HashNode {
    short key;
    void* value;
    struct HashNode* next;
} HashNode;

typedef struct HashTable {
    int size;
    int count;
    HashNode* head;
} HashTable;

HashTable* InitHashTable(int size);

void PutItemInHashTable(HashTable* table, short key, void* value);

void* GetItemFromHashTable(HashTable* table, short key);

void RemoveItemFromHashTable(HashTable* table, short key);

void DestroyHashTable(HashTable* table);

short Hash(HashTable* table, short key);

#endif