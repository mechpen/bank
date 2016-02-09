/*
 * Concurrent user control
 */

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "helper.h"
#include "user.h"

extern int config_user_hash_bits;

#define hash_id(id) hash_64((uint64_t)(id), config_user_hash_bits)

struct user_table {
	struct hlist_head head;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

static struct user_table *user_table;

void user_init(void)
{
	int i, size = 1 << config_user_hash_bits;

	user_table = (struct user_table *)calloc(sizeof(struct user_table), size);
	for (i = 0; i < size; i++) {
		int ret;
		struct user_table *entry = &user_table[i];

		INIT_HLIST_HEAD(&entry->head);
		if ((ret = pthread_mutex_init(&entry->mutex, NULL)) != 0)
			ERROR_EXIT("pthread_mutex_init %s", strerror(ret));
		if ((ret = pthread_cond_init(&entry->cond, NULL)) != 0)
			ERROR_EXIT("pthread_cond_init %s", strerror(ret));
	}
}

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

	user = (struct user *)malloc(sizeof(struct user));
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
