/*
 * Write ahead log
 *
 * file format (fields and size):
 *
 *  +-------------+-------------+-------------+-- ... --+
 *  | header (64) | record (64) | record (64) |   ...   |
 *  +-------------+-------------+-------------+-- ... --+
 *
 * header:
 *
 *  +-------------------+---------------+---------------+
 *  | magic_version (8) | start_lsn (8) | reserved (48) |
 *  +-------------------+---------------+---------------+
 *
 * record:
 *
 *  +------------------+------------------+
 *  |     time (8)     |   reserved (8)   |
 *  +------------------+------------------+------------------+
 *  |    src_id (8)    |  src_amount (8)  | src_last_lsn (8) |
 *  +------------------+------------------+------------------+
 *  |    dst_id (8)    |  dst_amount (8)  | dst_last_lsn (8) |
 *  +------------------+------------------+------------------+
 */

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "wal.h"
#include "accdb.h"
#include "helper.h"
#include "config.h"
#include "log.h"

extern uint64_t accdb_synced_lsn;
extern uint64_t wal_next_lsn;

struct wal_header {
    uint64_t magic_version;
    uint64_t start_lsn;
    char reserved[48];
} __attribute__((packed));

uint64_t wal_next_lsn;

static int wal_fd;
static size_t wal_file_size;
static size_t wal_synced_lsn;
static pthread_mutex_t wal_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wal_cond = PTHREAD_COND_INITIALIZER;

/* lsn starts from 1 */
#define wal_pos(lsn)    \
    (((lsn) - 1) * sizeof(struct wal_record) + sizeof(struct wal_header))

#define NS_PER_S  (1000 * 1000 * 1000)

static inline void delayed_fsync(uint64_t delay_ns, uint64_t lsn,
                                 pthread_mutex_t *pmutex)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += delay_ns;
    while (ts.tv_nsec >= NS_PER_S) {
        ts.tv_nsec -= NS_PER_S;
        ts.tv_sec += 1;
    }

    while (wal_synced_lsn < lsn) {
        int ret = pthread_cond_timedwait(&wal_cond, pmutex, &ts);
        if (ret == 0)
            continue;
        if (ret != ETIMEDOUT)
            ERROR_EXIT("pthread_cond_timedwait %s", strerror(ret));
        /* timeout may come instead of cond notify, so we need to
           re-check after timeout. */
        if (wal_synced_lsn >= lsn)
            break;
        DBG("delayed_fsync lsn %ld next_lsn %ld synced_lsn %ld",
            lsn, wal_next_lsn, wal_synced_lsn);
        ensure_fdatasync(wal_fd);
        wal_synced_lsn = wal_next_lsn - 1;
        ensure_pthread_cond_broadcast(&wal_cond);
        break;
    }
}

uint64_t
wal_transfer(uint64_t src_id, uint64_t src_amount, uint64_t src_last_lsn,
             uint64_t dst_id, uint64_t dst_amount, uint64_t dst_last_lsn)
{
    uint64_t lsn, pos;
    struct wal_record record;
    struct timeval tv;

    memset(&tv, 0, sizeof(tv));
    gettimeofday(&tv, NULL);

    memset(&record, 0, sizeof(record));
    record.time = tv.tv_sec;
    record.src_id = src_id;
    record.src_amount = src_amount;
    record.src_last_lsn = src_last_lsn;
    if (dst_id != 0) {
        record.dst_id = dst_id;
        record.dst_amount = dst_amount;
        record.dst_last_lsn = dst_last_lsn;
    }

    ensure_pthread_mutex_lock(&wal_mutex);

    lsn = wal_next_lsn++;
    if (lsn == 0)
        ERROR_EXIT("lsn overflow");
    pos = wal_pos(lsn);
    if (pos == wal_file_size) {
        ensure_fallocate(wal_fd, pos, config_wal_prealloc_size);
        wal_file_size += config_wal_prealloc_size;
        DBG("lsn %ld bumped wal file size to %ld", lsn, wal_file_size);
    }
    ensure_pwrite(wal_fd, &record, sizeof(record), pos);
    delayed_fsync(config_wal_sync_delay_ns, lsn, &wal_mutex);

    ensure_pthread_mutex_unlock(&wal_mutex);

    return lsn;
}

void wal_get_entry(int32_t lsn, struct wal_record *record)
{
    ensure_pread(wal_fd, record, sizeof(*record), wal_pos(lsn));
}

static uint64_t wal_create(const char *path, int32_t start_lsn)
{
    int ret;
    struct wal_header header;

    ret = open(path, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    if (ret < 0)
        ERROR_EXIT("%s", strerror(errno));
    wal_fd = ret;

    ensure_fallocate(wal_fd, 0, config_wal_prealloc_size);
    wal_file_size = config_wal_prealloc_size;

    memset(&header, 0, sizeof(header));
    header.magic_version = WAL_MAGIC_VERSION;
    header.start_lsn = start_lsn;
    ensure_pwrite(wal_fd, &header, sizeof(header), 0);

    ensure_fsync(wal_fd);

    return start_lsn;
}

static uint64_t wal_open(void)
{
    int ret;
    char path[PATH_MAX];
    struct wal_header header;
    struct stat stat;

    ret = snprintf(path, PATH_MAX, "%s/%s", config_root_db_dir, WAL_FILE_NAME);
    if (ret >= PATH_MAX)
        ERROR_EXIT("wal path too long");

    ret = open(path, O_RDWR);
    if (ret < 0 && errno == ENOENT)
        return wal_create(path, 1);
    if (ret < 0)
        ERROR_EXIT("%s", strerror(errno));
    wal_fd = ret;

    ensure_pread(wal_fd, &header, sizeof(header), 0);
    if (header.magic_version != WAL_MAGIC_VERSION)
        ERROR_EXIT("invalid wal file magic version in %s", path);
    if (header.start_lsn == 0)
        ERROR_EXIT("invalid wal file start lsn in %s", path);

    if (fstat(wal_fd, &stat) < 0)
        ERROR_EXIT("fstat %s", strerror(errno));
    wal_file_size = stat.st_size;
    if (wal_file_size % config_wal_prealloc_size != 0)
        ERROR_EXIT("invalid wal file size %lu", wal_file_size);

    return header.start_lsn;
}

void open_and_replay_wal(void)
{
    uint64_t lsn = accdb_synced_lsn;
    struct wal_record wal_record;
    struct accdb_record acc_record;

    wal_open();
    for (;;) {
        lsn++;
        if (lsn == 0)
            ERROR_EXIT("lsn overflow");

        wal_get_entry(lsn, &wal_record);
        if (wal_record.src_id == 0)
            break;

        acc_record.amount = wal_record.src_amount;
        acc_record.last_lsn = lsn;
        accdb_set_account(wal_record.src_id, &acc_record);
        if (wal_record.dst_id != 0) {
            acc_record.amount = wal_record.dst_amount;
            acc_record.last_lsn = lsn;
            accdb_set_account(wal_record.dst_id, &acc_record);
        }
    }

    wal_next_lsn = lsn;
}
