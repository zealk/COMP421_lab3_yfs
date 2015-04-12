#include <stdlib.h>
#include <stdbool.h>
#include "../include/hashtable.h"

HashTable* InitHashTable(int size) {
    HashTable* hashtable = (HashTable*)calloc(1, sizeof(HashTable));
    hashtable->size = size;
    hashtable->count = 0;
    HashNode* head = (HashNode*)calloc(size, sizeof(HashNode));
    int i;
    for (i = 0; i < size; ++i) {
        HashNode* node = head + i;
        node->key = -1;
    }
    hashtable->head = head;

    return hashtable;
}

void PutItemInHashTable(HashTable* table, int key, void* value) {
    int index = Hash(table, key);
    HashNode* node = table->head + index;

    while (true) {
        if (node->key == -1 || node->key == key) {
            if (node->key == -1) {
                node->key = key;
                ++table->count;
            }

            node->value = value;
            return;
        }

        if (node->next != NULL) {
            node = node->next;
        } else {
            HashNode* new_node = (HashNode*)calloc(1, sizeof(HashNode));
            new_node->key = key;
            new_node->value = value;
            node->next = new_node;
            ++table->count;
            return;
        }
    }
}

void* GetItemFromHashTable(HashTable* table, int key) {
    int index = Hash(table, key);
    HashNode* node = table->head + index;

    while (node != NULL) {
        if (node->key == key) {
            return node->value;
        }

        node = node->next;
    }

    return NULL;
}

void RemoveItemFromHashTable(HashTable* table, int key) {
    int index = Hash(table, key);
    HashNode* node = table->head + index;
    HashNode* temp = node;

    while (node != NULL) {
        if (node->key == key) {
            if ((table->head + index) == node) {
                node->key = -1;
                node->value = NULL;
            } else {
                temp->next = node->next;
                free(node);
            }

            --table->count;
            return;
        }
        temp = node;
        node = node->next;
    }
}

void DestroyHashTable(HashTable* table) {
    HashNode* head = table->head;
    int i;
    for (i = 0; i < table->size; ++i) {
        HashNode* current = head->next;
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
        ++head;
    }

    free(table->head);
    free(table);
}

int Hash(HashTable* table, int key) {
    return key % table->size;
}