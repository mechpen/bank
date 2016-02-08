#ifndef _BANK_HLIST_H
#define _BANK_HLIST_H

#include <stdint.h>
#include <stddef.h>

/* borrowed from kernel hash list */

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }

static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define hlist_entry(ptr, type, member) container_of(ptr, type, member)

#define hlist_for_each(pos, head) 	\
	for (pos = (head)->first; pos ; pos = pos->next)

#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

static inline uint64_t hash_64(uint64_t val, unsigned int bits) {
	uint64_t hash = val;
	hash = hash * GOLDEN_RATIO_PRIME_64;
	return hash >> (64 - bits);
};

#endif
