/*
 * Concurrent user control
 */

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "helper.h"
#include "user.h"

#define hash_id(id) hash_64((uint64_t)(id), USER_HASH_BITS)

struct user_table {
	struct hlist_head head;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

static struct user_table user_table[1 << USER_HASH_BITS] = {
	[0 ... ((1 << USER_HASH_BITS) - 1)]	= {
		HLIST_HEAD_INIT,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_COND_INITIALIZER,
	}
};

static inline int find_user(uint64_t id, struct hlist_head *head)
{
	struct hlist_node *pos;
	hlist_for_each(pos, head) {
		struct user *user = hlist_entry(pos, struct user, node);
		if (user->id == id)
			return 1;
	}
	return 0;
}

struct user *get_user(uint64_t id)
{
	uint64_t i = hash_id(id);
	struct user_table *entry = &user_table[i];
	struct user *user;

	user = malloc(sizeof(*user));
	if (user == NULL)
		return NULL;

	user->id = id;
	INIT_HLIST_NODE(&user->node);

	ensure_pthread_mutex_lock(&entry->mutex);

	while (find_user(id, &entry->head) == 1)
		ensure_pthread_cond_wait(&entry->cond, &entry->mutex);
	hlist_add_head(&user->node, &entry->head);

	ensure_pthread_mutex_unlock(&entry->mutex);

	return user;
}

void put_user(struct user *user)
{
	uint64_t i = hash_id(user->id);
	struct user_table *entry = &user_table[i];

	ensure_pthread_mutex_lock(&entry->mutex);

	hlist_del(&user->node);
	ensure_pthread_mutex_unlock(&entry->mutex);

	ensure_pthread_cond_broadcast(&entry->cond);

	free(user);
}
