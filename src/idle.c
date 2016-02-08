#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "accdb.h"
#include "config.h"
#include "log.h"

extern uint64_t accdb_synced_lsn;
extern uint64_t wal_next_lsn;

void *idle(void *arg)
{
	uint64_t lsn;

	for (;;) {
		lsn = wal_next_lsn - 1;
		if (lsn < accdb_synced_lsn)
			ERROR_EXIT("wal lsn %lu < synced lsn %lu", lsn, accdb_synced_lsn);

		if (lsn - accdb_synced_lsn >= MAX_LSN_DRIFT) {
			accdb_sync_lsn(lsn);
			accdb_synced_lsn = lsn;
		}

		sleep(1);
	}
}

void start_idle_thread(void)
{
	int ret;
	pthread_t tid;

	ret = pthread_create(&tid, NULL, &idle, NULL);
	if (ret != 0)
		ERROR_EXIT("%s", strerror(ret));
}
