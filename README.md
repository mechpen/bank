## Assumptions:

  1. fsync return success -> data written to disk
  2. writing to a HDD sector is atomic
  3. appending to a file is atomic

## Improvements:

  1. security
  2. asyncio
  3. dynamic hash table size
  4. user session control
  5. log rotation

## Synchronization of Shared Variables

accdb_synced_lsn
wal_next_lsn
