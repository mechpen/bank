#include <stdint.h>

#include "api.h"
#include "wal.h"
#include "accdb.h"
#include "user.h"
#include "config.h"
#include "helper.h"

extern uint64_t accdb_max_id;

static pthread_mutex_t new_acc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_new_acc(int fd, uint64_t UNUSED(a1),
                           uint64_t UNUSED(a2), uint64_t UNUSED(a3))
{
    uint64_t id;
    struct user *user;
    struct accdb_record record;

    ensure_pthread_mutex_lock(&new_acc_mutex);

    id = accdb_max_id + 1;
    if (id == 0)
        ERROR_EXIT("id overflow");

    user = get_user(id);
    if (user == NULL) {
        ensure_pthread_mutex_unlock(&new_acc_mutex);
        reply(fd, SERVER_ERROR);
        return;
    }
    accdb_max_id = id;

    ensure_pthread_mutex_unlock(&new_acc_mutex);

    record.amount = 0;
    record.last_lsn = wal_update(id, record.amount, 0);
    accdb_set_account(id, &record);
    accdb_set_max_id(id);

    put_user(user);
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

    if (invalid_id(src_id) || invalid_id(dst_id) ||
        src_id == dst_id || amount < 0) {
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
        put_user(src_user);
        put_user(dst_user);
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

#define assert_same(v1, v2, name) ({                                 \
    uint64_t __v1 = (v1), __v2 = (v2);                               \
    if (__v1 != __v2)                                                \
        ERROR_EXIT("wal mismatched %s %lu %lu", name, __v1, __v2);   \
})

static inline uint64_t get_amount(const uint64_t id,
                                   const struct wal_record *record) {
    if (id == record->src_id)
        return record->src_amount;
    assert_same(id, record->dst_id, "id");
    return record->dst_amount;
}

static inline uint64_t get_last_lsn(const uint64_t id,
                                     const struct wal_record *record) {
    if (id == record->src_id)
        return record->src_last_lsn;
    assert_same(id, record->dst_id, "id");
    return record->dst_last_lsn;
}

static inline const char *get_transfer_fmt(const uint64_t id,
                                           const struct wal_record *record) {
    if (id == record->src_id)
        return HISTORY_TRANSFER_TO;
    assert_same(id, record->dst_id, "id");
    return HISTORY_TRANSFER_FROM;
}

static inline uint64_t get_transfer_pid(const uint64_t id,
                                        const struct wal_record *record) {
    if (id == record->src_id)
        return record->dst_id;
    assert_same(id, record->dst_id, "id");
    return record->src_id;
}

static void handle_history(int fd, uint64_t id,
                           uint64_t UNUSED(a2), uint64_t UNUSED(a3))
{
    int i;
    uint64_t lsn, amount;
    struct user *user;
    struct accdb_record acc_record;
    struct wal_record wal_record;

    if (invalid_id(id)) {
        reply(fd, REQUEST_ERROR);
        return;
    }

    user = get_user(id);
    if (user == NULL) {
        reply(fd, SERVER_ERROR);
        return;
    }
    accdb_get_account(id, &acc_record);
    put_user(user);

    reply(fd, HISTORY_CURRENT, acc_record.amount);
    lsn = acc_record.last_lsn;
    amount = acc_record.amount;
    wal_get_entry(lsn, &wal_record);
    assert_same(amount, get_amount(id, &wal_record), "amount");

    for (i = 0; i < MAX_HISTORY_RECORD && wal_record.src_id != 0; i++) {
        const char *fmt = NULL;
        uint64_t time, diff, pid = 0;

        time = wal_record.time;
        if (wal_record.dst_id != 0) {
            fmt = get_transfer_fmt(id, &wal_record);
            pid = get_transfer_pid(id, &wal_record);
        }

        lsn = get_last_lsn(id, &wal_record);
        if (lsn == 0) {
            reply(fd, HISTORY_NEW_ACC, wal_record.time);
            break;
        }

        wal_get_entry(lsn, &wal_record);
        diff = get_amount(id, &wal_record);
        if (amount >= diff) {
            diff = amount - diff;
            if (fmt == NULL)
                fmt = HISTORY_DEPOSIT;
        } else {
            diff = diff - amount;
            if (fmt == NULL)
                fmt = HISTORY_WITHDRAW;
        }
        reply(fd, fmt, time, diff, pid);
        amount = get_amount(id, &wal_record);
    }
    reply(fd, MULTI_LINE_END);
}

static struct {
    const char *fmtstr;
    void (*handle) (int fd, uint64_t arg1, uint64_t arg2, uint64_t arg3);
} handlers[] = {
    {NEW_ACC,  handle_new_acc},
    {DEPOSIT,  handle_deposit},
    {WITHDRAW, handle_withdraw},
    {TRANSFER, handle_transfer},
    {HISTORY,  handle_history},
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
