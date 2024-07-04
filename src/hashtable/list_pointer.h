#ifndef LISTS_POINTER_H
#define LISTS_POINTER_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

//======================================================================================================================

struct list_pointer_t
{
    hkey_t   key;
    hvalue_t value;

    struct list_pointer_t* next;
    struct list_pointer_t* prev;
};

struct list_pointer
{
    const char* name;

    size_t size;

    struct list_pointer_t* head;
    struct list_pointer_t* tail;
};

int list_pointer_ctor(struct list_pointer* list, const char* name);
int list_pointer_dtor(struct list_pointer* list);

void list_pointer_clear(struct list_pointer* list);

struct list_pointer_t* list_pointer_find_element( struct list_pointer* list, size_t number);
struct list_pointer_t* list_pointer_find_value(   struct list_pointer* list, const hkey_t key);
struct list_pointer_t* list_pointer_insert_first( struct list_pointer* list, const hkey_t key, hvalue_t value);
struct list_pointer_t* list_pointer_insert_before(struct list_pointer* list, size_t number, const hkey_t key, hvalue_t value);
struct list_pointer_t* list_pointer_insert_after( struct list_pointer* list, size_t number, const hkey_t key, hvalue_t value);
struct list_pointer_t* list_pointer_insert_begin( struct list_pointer* list, const hkey_t key, hvalue_t value);
struct list_pointer_t* list_pointer_insert_end(   struct list_pointer* list, const hkey_t key, hvalue_t value);

void list_pointer_delete(struct list_pointer* list, struct list_pointer_t* element);
void list_pointer_delete_element(struct list_pointer* list, size_t number);
void list_pointer_graph(struct list_pointer* list);

#endif