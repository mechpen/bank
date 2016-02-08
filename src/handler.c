#include <stdint.h>

#include "api.h"
#include "wal.h"
#include "accdb.h"
#include "user.h"
#include "helper.h"

extern uint64_t accdb_max_id;

static pthread_mutex_t new_acc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_new_acc(int fd, uint64_t UNUSED(a1),
						   uint64_t UNUSED(a2), uint64_t UNUSED(a3))
{
	uint64_t id;
	struct accdb_record record;

	ensure_pthread_mutex_lock(&new_acc_mutex);

	id = accdb_max_id + 1;
	if (id == 0)
		ERROR_EXIT("id overflow");
	record.amount = 0;
	record.last_lsn = wal_update(id, record.amount, 0);
	accdb_set_account(id, &record);
	accdb_set_max_id(id);
	accdb_max_id = id;

	ensure_pthread_mutex_unlock(&new_acc_mutex);

	reply(fd, NEW_ACC_OK, id);
}

#define invalid_id(id) ((id) > accdb_max_id || (id) <= 0)

static void handle_deposit(int fd, uint64_t amount, uint64_t id,
						   uint64_t UNUSED(a3))
{
	struct user *user;
	struct accdb_record record;

	if (invalid_id(id) || amount < 0) {
		reply(fd, REQUEST_ERROR);
		return;
	}

	user = get_user(id);
	if (user == NULL) {
		reply(fd, SERVER_ERROR);
		return;
	}

	accdb_get_account(id, &record);
	if (record.amount > UINT64_MAX - amount) {
		put_user(user);
		reply(fd, OVERFL_ERROR);
	} else {
		uint64_t old = record.amount;
		record.amount += amount;
		record.last_lsn = wal_update(id, record.amount, record.last_lsn);
		accdb_set_account(id, &record);

		put_user(user);
		reply(fd, UPDATE_OK, old, record.amount);
	}
}

static void handle_withdraw(int fd, uint64_t amount, uint64_t id,
							uint64_t UNUSED(a3))
{
	struct user *user;
	struct accdb_record record;

	if (invalid_id(id) || amount < 0) {
		reply(fd, REQUEST_ERROR);
		return;
	}

	user = get_user(id);
	if (user == NULL) {
		reply(fd, SERVER_ERROR);
		return;
	}

	accdb_get_account(id, &record);
	if (record.amount < amount) {
		put_user(user);
		reply(fd, INSUFF_ERROR);
	} else {
		uint64_t old = record.amount;
		record.amount -= amount;
		record.last_lsn = wal_update(id, record.amount, record.last_lsn);
		accdb_set_account(id, &record);

		put_user(user);
		reply(fd, UPDATE_OK, old, record.amount);
	}
}

static void handle_transfer(int fd, uint64_t amount,
							uint64_t src_id, uint64_t dst_id)
{
	struct user *src_user, *dst_user;
	struct accdb_record src_record, dst_record;

	if (invalid_id(src_id) || invalid_id(dst_id) || amount < 0) {
		reply(fd, REQUEST_ERROR);
		return;
	}

	/* lock order to avoid deadlock */
	if (src_id < dst_id) {
		src_user = get_user(src_id);
		dst_user = get_user(dst_id);
	} else {
		dst_user = get_user(dst_id);
		src_user = get_user(src_id);
	}
	if (src_user == NULL || dst_user == NULL) {
		reply(fd, SERVER_ERROR);
		return;
	}

	accdb_get_account(src_id, &src_record);
	accdb_get_account(dst_id, &dst_record);
	if (src_record.amount < amount) {
		put_user(src_user);
		put_user(dst_user);
		reply(fd, INSUFF_ERROR);
	} else if (dst_record.amount > UINT64_MAX - amount) {
		put_user(src_user);
		put_user(dst_user);
		reply(fd, OVERFL_ERROR);
	} else {
		uint64_t lsn, old;
		old = src_record.amount;
		src_record.amount -= amount;
		dst_record.amount += amount;
		lsn = wal_transfer(src_id, src_record.amount, src_record.last_lsn,
						   dst_id, dst_record.amount, dst_record.last_lsn);
		src_record.last_lsn = lsn;
		dst_record.last_lsn = lsn;	
		accdb_set_account(src_id, &src_record);
		accdb_set_account(dst_id, &dst_record);

		put_user(src_user);
		put_user(dst_user);
		reply(fd, UPDATE_OK, old, src_record.amount);
	}
}

static struct {
	const char *fmtstr;
	void (*handle) (int fd, uint64_t arg1, uint64_t arg2, uint64_t arg3);
} handlers[] = {
	{NEW_ACC,  handle_new_acc},
	{DEPOSIT,  handle_deposit},
	{WITHDRAW, handle_withdraw},
	{TRANSFER, handle_transfer},
};

void handle_request(int fd, const char *req)
{
	int i;
	char buf[MAX_REQ_LEN];

	for (i = 0; i < ARRAY_SIZE(handlers); i++) {
		uint64_t arg1, arg2, arg3;

		sscanf(req, handlers[i].fmtstr, &arg1, &arg2, &arg3);
		snprintf(buf, sizeof(buf), handlers[i].fmtstr, arg1, arg2, arg3);
		if (strcmp(req, buf) == 0) {
			handlers[i].handle(fd, arg1, arg2, arg3);
			break;
		}
	}
	if (i == ARRAY_SIZE(handlers))
		reply(fd, REQUEST_ERROR);
}
