#ifndef _BANK_USER_H
#define _BANK_USER_H

#include <stdint.h>

#include "hlist.h"

struct user {
	struct hlist_node node;
	uint64_t id;
};

struct user *get_user(uint64_t id);
void put_user(struct user *user);

#endif
