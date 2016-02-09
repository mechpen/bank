#ifndef _BANK_ACCDB_H
#define _BANK_ACCDB_H

#include <stdint.h>

struct accdb_record {
    uint64_t amount;
    uint64_t last_lsn;
} __attribute__((packed));

void accdb_open(void);
void accdb_sync_lsn(uint64_t lsn);
void accdb_set_max_id(uint64_t id);
void accdb_set_account(uint64_t id, struct accdb_record *record);
void accdb_get_account(uint64_t id, struct accdb_record *record);

#endif
