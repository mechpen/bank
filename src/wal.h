#ifndef _BANK_WAL_H
#define _BANK_WAL_H

#include <stdint.h>

struct wal_record {
    uint64_t time;
    uint64_t reserved;
    uint64_t src_id;
    uint64_t src_amount;
    uint64_t src_last_lsn;
    uint64_t dst_id;
    uint64_t dst_amount;
    uint64_t dst_last_lsn;
} __attribute__((packed));

void open_and_replay_wal(void);
void wal_get_entry(int32_t lsn, struct wal_record *record);

uint64_t
wal_transfer(uint64_t src_id, uint64_t src_amount, uint64_t src_last_lsn,
             uint64_t dst_id, uint64_t dst_amount, uint64_t dst_last_lsn);

static inline uint64_t wal_update(uint64_t id, uint64_t amount, uint64_t last_lsn)
{
    return wal_transfer(id, amount, last_lsn, 0, 0, 0);
}

#endif
