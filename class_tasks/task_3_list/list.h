#ifndef LIST_H
#define LIST_H

typedef struct plist_t 
{
    struct plist_t* next;
    struct plist_t* prev;
} plist_t;

#define LIST_POISON1 (plist_t*) 0xBADDED
#define LIST_POISON2 (plist_t*) 0xBADBEEF


#define PLIST_HEAD_INIT(name) { &(name), &(name) }
#define PLIST_HEAD(name) plist_t name = PLIST_HEAD_INIT(name)

static inline void INIT_PLIST_HEAD(plist_t* list)
{
    list->next = list;
    list->prev = list;
}

static inline void plist_add(plist_t* item, plist_t* prev, plist_t* next)
{   
    next->prev = item;
    item->next = next;
    item->prev = prev;
    prev->next = item;
}

static inline void plist_add_head(plist_t* item, plist* head)
{
    plist_add(item, head, head->next);
}

static inline void plist_add_tail(plist_t* item, plist_t* head)
{
    plist_add(item, head->prev, head);
}

static inline void plist_del_entry(plist_t* item)
{
    item->next->prev = item->prev;
    item->prev->next = item->next;
}

static inline void plist_del(plist_t* item)
{
    plist_del_entry(item);
    item->next = LIST_POISON1;
    item->prev = LIST_POISON2;
}

static inline void plist_replace(plist_t* old_item, plist_t* new_item)
{
	new_item->next = old_item->next;
	new_item->prev = old_item->prev;
	new_item->next->prev = new_item;
	new_item->prev->next = new_item;
}

static inline void plist_replace_init(plist_t* old_item, plist_t* new_item)
{
	plist_replace(old_item, new_item);
	INIT_PLIST_HEAD(old_item);
}

static inline void plist_swap(plist_t* entry1, plist_t* entry2)
{
	plist_t* pos = entry2->prev;

	plist_del(entry2);
	plist_replace(entry1, entry2);

	if (pos == entry1) pos = entry2;

	plist_add_head(entry1, pos);
}

static inline void plist_del_init(plist_t* item)
{
    plist_del_entry(item);
	INIT_PLIST_HEAD(item);
}

static inline void plist_move(plist_t* list, plist_t* head)
{
	plist_del_entry(list);
	plist_add_head(list, head);
}

#endif //! LIST_H