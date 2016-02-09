/*
 * Account database file
 *
 * file format (fields and size):
 *
 *  +-------------+-------------+-------------+-- ... --+
 *  | header (32) | record (16) | record (16) |   ...   |
 *  +-------------+-------------+-------------+-- ... --+
 *
 * header:
 *
 *  +-------------------+---------------+--------------+--------------+
 *  | magic_version (8) | syncd_lsn (8) |  max_id (8)  | reserved (8) |
 *  +-------------------+---------------+--------------+--------------+
 *
 * record:
 *
 *  +-------------+-------------+
 *  |  amount (8) | last_lsn(8) |
 *  +-------------+-------------+
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>

#include "accdb.h"
#include "helper.h"
#include "config.h"
#include "log.h"

extern char *config_root_db_dir;

struct accdb_header {
	uint64_t magic_version;
	uint64_t synced_lsn;
	uint64_t max_id;
	char reserved[8];
} __attribute__((packed));

/* FIXME: atomicity */
uint64_t accdb_max_id;
uint64_t accdb_synced_lsn;

static int accdb_fd;

/* id starts from 1 */
#define accdb_pos(id)	\
	(((id) - 1) * sizeof(struct accdb_record) + sizeof(struct accdb_header))

void accdb_get_account(uint64_t id, struct accdb_record *record)
{
	ensure_pread(accdb_fd, record, sizeof(*record), accdb_pos(id));
	if (record->last_lsn == 0)
		ERROR_EXIT("last_lsn of %lu is 0", id);
}

void accdb_set_account(uint64_t id, struct accdb_record *record)
{
	ensure_pwrite(accdb_fd, record, sizeof(*record), accdb_pos(id));
}

void accdb_set_max_id(uint64_t id)
{
	ensure_pwrite(accdb_fd, &id, sizeof(id),
				  offsetof(struct accdb_header, max_id));
}

void accdb_sync_lsn(uint64_t lsn)
{
	ensure_fsync(accdb_fd);
	ensure_pwrite(accdb_fd, &lsn, sizeof(lsn),
				  offsetof(struct accdb_header, synced_lsn));
}

static void accdb_create(const char *path)
{
	int ret;
	struct accdb_header header;

	ret = open(path, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
	if (ret < 0)
		ERROR_EXIT("%s", strerror(errno));
	accdb_fd = ret;

	memset(&header, 0, sizeof(header));
	header.magic_version = ACCDB_MAGIC_VERSION;
	ensure_pwrite(accdb_fd, &header, sizeof(header), 0);
	ensure_fsync(accdb_fd);

	accdb_synced_lsn = 0;
	accdb_max_id = 0;
}

void accdb_open(void)
{
	int ret;
	static char path[PATH_MAX];
	struct accdb_header header;

	ret = snprintf(path, PATH_MAX, "%s/%s", config_root_db_dir, ACCDB_FILE_NAME);
	if (ret >= PATH_MAX)
		ERROR_EXIT("accdb path too long");

	ret = open(path, O_RDWR);
	if (ret < 0 && errno == ENOENT)
		return accdb_create(path);
	if (ret < 0)
		ERROR_EXIT("%s", strerror(errno));
	accdb_fd = ret;

	ensure_pread(accdb_fd, &header, sizeof(header), 0);
	if (header.magic_version != ACCDB_MAGIC_VERSION)
		ERROR_EXIT("Invalid accdb magic version in %s", path);

	accdb_max_id = header.max_id;
	accdb_synced_lsn = header.synced_lsn;
}
