#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdio.h>
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

static inline void plist_add_head(plist_t* item, plist_t* head)
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

static inline void plist_move_add(plist_t* list, plist_t* head)
{
	plist_del_entry(list);
	plist_add_head(list, head);
}

static inline void plist_move_tail(plist_t* list, plist_t* head)
{
	plist_del_entry(list);
	plist_add_tail(list, head);
}

static inline void plist_bulk_move_tail(plist_t* head, plist_t* first, plist_t* last)
{
	first->prev->next = last->next;
	last->next->prev  = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

static inline int plist_is_first(const plist_t* list, const plist_t* head)
{
	return list->prev == head;
}

static inline int plist_is_last(const plist_t* list, const plist_t* head)
{
	return list->next == head;
}

static inline int plist_is_head(const plist_t* list, const plist_t* head)
{
	return list == head;
}

static inline int plist_empty(const plist_t* head)
{
	return head->next == head;
}

static inline int plist_is_singular(const plist_t* head)
{
	return !plist_empty(head) && (head->next == head->prev);
}

static inline void plist_rotate_left(plist_t* head)
{
	plist_t* first = NULL;

	if (!plist_empty(head)) 
    {
		first = head->next;
		plist_move_tail(first, head);
	}
}

#define container_of(ptr, type, member)                                 \
    ({                                                                  \
        void* tmp = (void*) ptr;                                        \
        ((type*)((char*)(tmp) - offsetof(type, member))); \
    })


#define plist_entry(ptr, type, member)          \
	container_of(ptr, type, member)

#define plist_first_entry(ptr, type, member)    \
	plist_entry((ptr)->next, type, member)

#define plist_last_entry(ptr, type, member)     \
	plist_entry((ptr)->prev, type, member)

#define plist_entry_is_head(pos, head, member)  \
	(&pos->member == (head))

#define plist_next_entry(pos, member)           \
	plist_entry((pos)->member.next, typeof(*(pos)), member)

#define plist_prev_entry(pos, member)           \
	plist_entry((pos)->member.prev, typeof(*(pos)), member)

#define plist_for_each(pos, head)                                           \
	for (pos = (head)->next; !plist_is_head(pos, (head)); pos = pos->next)

#define plist_for_each_prev(pos, head)                                      \
	for (pos = (head)->prev; !plist_is_head(pos, (head)); pos = pos->prev)


#define plist_for_each_entry(pos, head, member)			        \
	for (pos = plist_first_entry(head, typeof(*pos), member);	\
	     !plist_entry_is_head(pos, head, member);			    \
	     pos = plist_next_entry(pos, member))

#define plist_for_each_entry_reverse(pos, head, member)			\
	for (pos = plist_last_entry(head, typeof(*pos), member);	\
	     !plist_entry_is_head(pos, head, member); 			    \
	     pos = plist_prev_entry(pos, member))

#define plist_for_each_entry_from(pos, head, member) 		    \
	for (; !plist_entry_is_head(pos, head, member);			    \
	     pos = plist_next_entry(pos, member))

#define list_for_each_entry_from_reverse(pos, head, member)		\
	for (; !list_entry_is_head(pos, head, member);			    \
	     pos = list_prev_entry(pos, member))


#endif //! LIST_H