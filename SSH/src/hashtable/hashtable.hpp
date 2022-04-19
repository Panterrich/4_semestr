#include "hashtable.h"

int hashtable_ctor(struct hashtable* table, size_t size, unsigned int (*hash)(const void*))
{
    assert(table);
    
    if (!hash) return 1;

    table->size = size;
    table->hash = hash;
    table->keys = (struct list_pointer*) calloc(table->size, sizeof(struct list_pointer));

    for (size_t i = 0; i < table->size; ++i)
    {
        list_pointer_ctor(table->keys + i, "hashtable");
    }

    return 0;
}

int hashtable_dtor(struct hashtable* table)
{
    assert(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        list_pointer_dtor(table->keys + i);
    }

    free(table->keys);

    table->size = 0;
    table->keys = NULL;
    table->hash = NULL;

    return 0;
}

hvalue_t hashtable_get_value(struct hashtable* table, const hkey_t key)
{
    assert(table);

    if (table->size == 0) return value_poison;

    struct list_pointer_t* element = list_pointer_find_value(&table->keys[table->hash(&key) % table->size], key);

    return (element != NULL) ? element->value : value_poison;
}

unsigned int hashtable_set_value(struct hashtable* table, const hkey_t key, hvalue_t data)
{
    if (table->size == 0) return 0;

    unsigned int hash_key = table->hash(key); 

    list_pointer_insert_end(&table->keys[hash_key % table->size], key, data);

    return hash_key;
}

void hashtable_remove(struct hashtable* table, const hkey_t key)
{
    if (table->size == 0) return;

    size_t index = table->hash(key) % table->size;
    list_pointer_delete(table->keys + index, list_pointer_find_value(table->keys + index, key));
}

void hashtable_clear(struct hashtable* table)
{
    assert(table);

    for (size_t index = 0; index < table->size; ++index)
    {
        list_pointer_clear(table->keys + index);
    }
}

#define MY_STRLEN(key) (key == NULL) ? 0 : strlen(key)

unsigned int MurmurHash2 (const void* key)
{
    size_t len = MY_STRLEN(key);

    const unsigned int m = 0x5bd1e995;
    const unsigned int seed = 0;
    const int r = 24;

    unsigned int h = seed ^ len;

    const unsigned char* data = (const unsigned char*)key;
    unsigned int k;

    while (len >= 4)
    {
        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len)
    {
        case 3:
        h ^= data[2] << 16;
        case 2:
        h ^= data[1] << 8;
        case 1:
        h ^= data[0];
        h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}