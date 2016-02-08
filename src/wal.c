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
#include <linux/limits.h>

#include "wal.h"
#include "helper.h"
#include "config.h"
#include "log.h"

extern char *bank_root_dir;

struct wal_header {
	uint64_t magic_version;
	uint64_t start_lsn;
	char reserved[48];
} __attribute__((packed));

uint64_t wal_next_lsn;

static int wal_fd;
static pthread_mutex_t wal_mutex = PTHREAD_MUTEX_INITIALIZER;

/* lsn starts from 1 */
#define wal_pos(lsn)	\
	(((lsn) - 1) * sizeof(struct wal_record) + sizeof(struct wal_header))

uint64_t
wal_transfer(uint64_t src_id, uint64_t src_amount, uint64_t src_last_lsn,
			 uint64_t dst_id, uint64_t dst_amount, uint64_t dst_last_lsn)
{
	uint64_t lsn;
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
	ensure_pwrite(wal_fd, &record, sizeof(record), wal_pos(lsn));
	ensure_fsync(wal_fd);  /* FIXME: move out of mutex ? */

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

	memset(&header, 0, sizeof(header));
	header.magic_version = WAL_MAGIC_VERSION;
	header.start_lsn = start_lsn;
	ensure_pwrite(wal_fd, &header, sizeof(header), 0);

	ret = fsync(wal_fd);
	if (ret < 0)
		ERROR_EXIT("%s", strerror(errno));

	return start_lsn;
}

uint64_t wal_open(void)
{
	int ret;
	char path[PATH_MAX];
	struct wal_header header;

	ret = snprintf(path, PATH_MAX, "%s/%s", bank_root_dir, WAL_FILE_NAME);
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
		ERROR_EXIT("Invalid wal magic version in %s", path);
	if (header.start_lsn == 0)
		ERROR_EXIT("Invalid wal start lsn in %s", path);

	return header.start_lsn;
}
