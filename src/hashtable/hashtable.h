#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "list_pointer.h"

struct hashtable
{
    struct list_pointer* keys;
    size_t size;

    unsigned int (*hash)(const void*);
};

int          hashtable_ctor(     struct hashtable* table, size_t size, unsigned int (*hash)(const void*));
int          hashtable_dtor(     struct hashtable* table);
hvalue_t     hashtable_get_value(struct hashtable* table, const hkey_t key);
unsigned int hashtable_set_value(struct hashtable* table, const hkey_t key, hvalue_t data);
void         hashtable_remove(   struct hashtable* table, const hkey_t key);
void         hashtable_clear(    struct hashtable* table);


unsigned int MurmurHash2 (const void* key);


#endif