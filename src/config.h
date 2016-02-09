#ifndef _BANK_CONFIG_H
#define _BANK_CONFIG_H

#define ROOT_DB_DIR            "."

#define HDD_BLOCK_SIZE         4096

#define WAL_FILE_NAME          "wal"
#define WAL_MAGIC_VERSION      0x0000000100766587
#define WAL_PREALLOC_SIZE      4096 * HDD_BLOCK_SIZE

#define ACCDB_FILE_NAME        "accdb"
#define ACCDB_MAGIC_VERSION    0x0000000100676765

#define MAX_LSN_DRIFT          10000

#define USER_HASH_BITS         10

#define MAX_HISTORY_RECORD     10

extern char *config_root_db_dir;
extern int config_wal_prealloc_size;
extern int config_max_lsn_drift;
extern int config_user_hash_bits;

#endif
