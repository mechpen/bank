## Build and Run

To build:

    $ cd ~/bank && make

To start the server.  By default the server listen at 127.0.0.1:7890.

    $ cd ~/bank && src/bank &

Start a client session with `nc`.  Please note that lines starting
with "200" or "210" are server responses.

    $ nc localhost 7890
    OPEN
    200 - ACCOUNT NUMBER 1
    DEPOSIT 100 TO 1
    200 - OLD AMOUNT 0 NEW AMOUNT 100
    WITHDRAW 50 FROM 1
    200 - OLD AMOUNT 100 NEW AMOUNT 50
    OPEN
    200 - ACCOUNT NUMBER 2
    TRANSFER 50 FROM 1 TO 2
    200 - OLD AMOUNT 50 NEW AMOUNT 0
    HISTORY 1
    210 - CURRENT 0
    210 - AT 1455036422 TRANSFERRED 50 TO 2
    210 - AT 1455036402 WITHDREW 50
    210 - AT 1455036379 DEPOSITED 100
    210 - AT 1455036364 OPENED
    210 - 

Server command options:

    Usage: bank [ -r root_dir ] [ -a addr ] [ -p port ] [ -v ] [ -h ]
                [ -s wal_block_size ] [ -d max_lsn_drift ]
                [ -b user_hash_bits ]

        -r  set db root dir to <root_dir>, default is "."
        -a  bind to address <addr>, default is "127.0.0.1"
        -p  bind to port <port>, default is "7890"
        -s, -d, -b  don't use, testing only
        -v  verbose output
        -h  print this message and quit

## Assumption for Durability

Writing to a hard disk sector must be atomic.  A sector write either
succeeded or did never happen.  Durability is implemented using WAL
(write ahead lock).

## Tests

To run automated testing:

    $ cd ~/bank && make test

The automated tests include:

  - functional test
  - durability test
  - concurrency test

The durability test is done by starting the server with an older
version of the database file, then check that all missed transactions
are restored.

## Performance

The bottleneck of the system is at appending to the WAL file, which
depends on hard disk IO speed.  On the server it can get 1316.93
transaction per second with default options.

## Future Improvements

  1. security -- currently no authentication nor authorization,
  2. asyncio -- bottleneck is not here, but asyncio is cool,
  3. dynamically resizing user hash table based on number of conflicts,
  4. user session control -- timeout connection and etc,
  5. WAL rotation
