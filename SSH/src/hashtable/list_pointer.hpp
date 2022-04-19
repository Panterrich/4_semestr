#include "list_pointer.h"

int list_pointer_ctor(struct list_pointer* list, const char* name)
{
    assert(list);

    list->head = NULL;
    list->tail = NULL;

    list->size = 0;

    list->name = name;

    return 0;
}

int list_pointer_dtor(struct list_pointer* list)
{
    assert(list);

    list_pointer_clear(list);

    list->head = NULL;
    list->tail = NULL;

    list->size = 0;
    list->name = NULL;

    return 0;
}

void list_pointer_clear(struct list_pointer* list)
{
    assert(list);

    struct list_pointer_t* current_element = list->tail;
    struct list_pointer_t* next_element    = NULL;

    for (size_t index = 0; index < list->size; ++index)
    {
        next_element = current_element->prev;

        current_element->prev  = NULL;
        current_element->next  = NULL;

        value_dtor(&current_element->value);
        current_element->value = value_poison;

        key_dtor(&current_element->key);
        current_element->key   = key_poison;

        free(current_element);

        current_element = next_element;
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
}

struct list_pointer_t* list_pointer_find_element(struct list_pointer* list, size_t number)
{
    assert(list);

    struct list_pointer_t* current_element = list->head;

    for (size_t index = 1; index < number && index < list->size; ++index)
    {
        current_element = current_element->next;
    }

    return current_element;
}

struct list_pointer_t* list_pointer_find_value(struct list_pointer* list, const hkey_t key)
{
    assert(list);

    struct list_pointer_t* current_element = list->head;

    for (size_t index = 0; index < list->size; ++index)
    {
        if (key_cmp(&current_element->key, &key) == 0)
        {
            return current_element;
        }

        current_element = current_element->next;
    }

    return NULL;
}

struct list_pointer_t* list_pointer_insert_first(struct list_pointer* list, const hkey_t key, hvalue_t value)
{
    assert(list);

    struct list_pointer_t* new_element = (struct list_pointer_t*) calloc(1, sizeof(struct list_pointer_t));
    assert(new_element);

    new_element->value = value;
    new_element->key   = key;
    new_element->prev  = NULL;
    new_element->next  = NULL;

    list->size = 1;
    list->head = new_element;
    list->tail = new_element; 

    return new_element;
}

struct list_pointer_t* list_pointer_insert_before(struct list_pointer* list, size_t number, const hkey_t key, hvalue_t value)
{
    assert(list);

    if (list->size == 0)
    {
        return list_pointer_insert_first(list, key, value);
    }

    if (number == 1)
    {
        return list_pointer_insert_begin(list, key, value);
    }

    struct list_pointer_t* current_element = list_pointer_find_element(list, number);
    struct list_pointer_t* new_element = (struct list_pointer_t*) calloc (1, sizeof(struct list_pointer_t));

    new_element->prev  = current_element->prev;
    new_element->next  = current_element;
    new_element->value = value;
    new_element->key   = key;

    (current_element->prev)->next = new_element;
    current_element->prev = new_element;

    ++list->size;
    
    return new_element;
}

struct list_pointer_t* list_pointer_insert_after(struct list_pointer* list, size_t number, const hkey_t key, hvalue_t value)
{
    assert(list);

    if (list->size == 0)
    {
        return list_pointer_insert_first(list, key, value);
    }

    if (number == list->size)
    {
        return list_pointer_insert_end(list, key, value);
    }

    struct list_pointer_t* current_element = list_pointer_find_element(list, number);
    struct list_pointer_t* new_element = (struct list_pointer_t*) calloc (1, sizeof(struct list_pointer_t));

    new_element->next  = current_element->next;
    new_element->prev  = current_element;
    new_element->value = value;
    new_element->key   = key;

    (current_element->next)->prev = new_element;
    current_element->next = new_element;

    ++list->size;
    
    return new_element;
}

struct list_pointer_t* list_pointer_insert_begin(struct list_pointer* list, const hkey_t key, hvalue_t value)
{
    assert(list);
    
    if (list->size == 0)
    {
        return list_pointer_insert_first(list, key, value);
    }

    struct list_pointer_t* current_element = list->head;
    struct list_pointer_t* new_element = (struct list_pointer_t*) calloc (1, sizeof(struct list_pointer_t));

    new_element->prev  = NULL;
    new_element->next  = current_element;
    new_element->value = value;
    new_element->key   = key;

    current_element->prev = new_element;

    list->head = new_element;
    ++list->size;
    
    return new_element;
}

struct list_pointer_t* list_pointer_insert_end(struct list_pointer* list, const hkey_t key, hvalue_t value)
{
    assert(list);

    if (list->size == 0)
    {
        return list_pointer_insert_first(list, key, value);
    }

    struct list_pointer_t* current_element = list->tail;
    struct list_pointer_t* new_element = (struct list_pointer_t*) calloc (1, sizeof(struct list_pointer_t));

    new_element->prev  = current_element;
    new_element->next  = NULL;
    new_element->value = value;
    new_element->key   = key;

    current_element->next = new_element;

    list->tail = new_element;
    ++list->size;

    return new_element;
}

void list_pointer_delete_element(struct list_pointer* list, size_t number)
{
    assert(list);

    struct list_pointer_t* current_element = list_pointer_find_element(list, number);

    if (list->size == 1)
    {
        list->head = NULL;
        list->tail = NULL;
    }

    else if (current_element == list->head)
    {
        list->head = current_element->next;
        (current_element->next)->prev = NULL;
    }

    else if (current_element == list->tail)
    {
        list->tail = current_element->prev;
        (current_element->prev)->next = NULL;
    } 

    else
    {
        (current_element->prev)->next = current_element->next;
        (current_element->next)->prev = current_element->prev;
    }
    
    current_element->next  = NULL;
    current_element->prev  = NULL;
    value_dtor(&current_element->value);
    current_element->value = value_poison;

    key_dtor(&current_element->key);
    current_element->key   = key_poison;

    free(current_element);

    --list->size;
}

void list_pointer_delete(struct list_pointer* list, struct list_pointer_t* element)
{
    assert(list);

    if (list->size == 1)
    {
        list->head = NULL;
        list->tail = NULL;
    }

    else if (element == list->head)
    {
        list->head = element->next;
        (element->next)->prev = NULL;
    }

    else if (element == list->tail)
    {
        list->tail = element->prev;
        (element->prev)->next = NULL;
    } 

    else
    {
        (element->prev)->next = element->next;
        (element->next)->prev = element->prev;
    }
    
    element->next  = NULL;
    element->prev  = NULL;

    value_dtor(&element->value);
    element->value = value_poison;

    key_dtor(&element->key);
    element->key   = key_poison;

    free(element);

    --list->size;
}

void list_pointer_graph(struct list_pointer* list)
{
    assert(list);

    FILE* text = fopen("/tmp/graph_pointer.dot", "a");

    fprintf(text, "digraph G {\n"            
                  "graph [bgcolor = Snow2]\n" 
                  "rankdir = LR;\n");

    struct list_pointer_t* current_element = list->head;
   
    for (size_t index = 1; index <= list->size; ++index)
    {   
        if (key_cmp(&current_element->key, &key_poison) != 0)
        {

            printf("12\n");
           fprintf(text, "\t%lu [shape = plaintext, label =<\n"
                          "\t<table>\n"
                          "\t<tr>\n" 
                          "\t\t<td colspan=\"3\"> ", index); 

            print_key_value(text, current_element->key, current_element->value);

            fprintf(text, " </td>\n"
                          "\t </tr>\n"
                          "\t <tr>\n"
                          "\t\t <td> %p </td>\n"
                          "\t\t <td> %p </td>\n"
                          "\t\t <td> %p </td>\n"
                          "\t </tr>\n"
                          "\t</table>>, style = filled, color = deepskyblue2, fillcolor = lightskyblue]\n", 
                    current_element->prev, current_element, current_element->next);
        }

        else
        {
            printf("13\n");
            fprintf(text, "\t%lu [shape = plaintext, label =<\n"
                          "\t<table>\n"
                          "\t<tr>\n" 
                          "\t\t<td colspan=\"3\">", index);

            print_key_value(text, current_element->key, current_element->value);
    
            fprintf(text, " </td>\n"
                          "\t </tr>\n"
                          "\t <tr>\n"
                          "\t\t <td> %p </td>\n"
                          "\t\t <td> %p </td>\n"
                          "\t\t <td> %p </td>\n"
                          "\t </tr>\n"
                          "\t</table>>, style = filled, color = red,  fillcolor = lightcoral]\n", 
                    current_element->prev, current_element, current_element->next);
        }

        current_element = current_element->next;
    }
    
    current_element = list->head;

    for (size_t index = 1; index < list->size; ++index)
    {   
        if (key_cmp(&current_element->key, &key_poison) != 0)
        {
            if (key_cmp(&current_element->key, &key_poison) != 0 && (current_element->next)->prev == current_element)
            {
                fprintf(text, "\t%ld -> %ld [color = deepskyblue2]; \n", index, index + 1);
                fprintf(text, "\t%ld -> %ld [color = deepskyblue2]; \n", index + 1, index);
            }

            else if ((current_element->next == NULL) && (current_element == list->tail))
            {
                ;
            }

            else 
            {
                fprintf(text, "\t%ld -> %ld [color = darkred]; \n", index, index + 1);
                fprintf(text, "\t%ld -> %ld [color = darkred]; \n", index + 1, index);
            }
        }

        else 
        {
            fprintf(text, "\t%ld -> %ld [color = darkred]; \n", index, index + 1);
            fprintf(text, "\t%ld -> %ld [color = darkred]; \n", index + 1, index);
        }

        current_element = current_element->next;
    }

    fprintf(text, "\tHEAD [style = filled, color = black, fillcolor = salmon1]; \n");
    fprintf(text, "\tTAIL [style = filled, color = black, fillcolor = salmon1]; \n");
    
    fprintf(text, "\tHEAD -> %d [color = deeppink]; \n",  list->size != 0);
    fprintf(text, "\tTAIL -> %ld [color = deeppink]; \n", list->size);

    fprintf(text, "}\n");

    fclose(text);
}