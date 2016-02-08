#include <stdint.h>

#include "wal.h"
#include "accdb.h"
#include "log.h"

extern uint64_t accdb_synced_lsn;
extern uint64_t wal_next_lsn;

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
