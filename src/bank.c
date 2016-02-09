#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "wal.h"
#include "user.h"
#include "accdb.h"
#include "config.h"
#include "log.h"

int log_verbose = 0;

char *config_root_db_dir = ROOT_DB_DIR;
int config_wal_prealloc_size = WAL_PREALLOC_SIZE;
int config_max_lsn_drift = MAX_LSN_DRIFT;
int config_user_hash_bits = USER_HASH_BITS;

extern void start_idle_thread(void);
extern void start_tcp_server(const char *addr_str, const char *port_str);

static void usage(void)
{
    fprintf(stderr,
            "Usage: bank [ -r root_dir ] [ -a addr ] [ -p port ] [ -v ] [ -h ]\n"
            "            [ -s wal_block_size ] [ -d max_lsn_drift ]\n"
            "            [ -b user_hash_bits ]\n\n"
            "    -r  set db root dir to <root_dir>, default is \".\"\n"
            "    -a  bind to address <addr>, default is \"127.0.0.1\"\n"
            "    -p  bind to port <port>, default is \"7890\"\n"
            "    -s, -d, -b  don't use, testing only\n"
            "    -v  verbose output\n"
            "    -h  print this message and quit\n\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int c;
    char *addr = "127.0.0.1", *port = "7890";

    while ((c = getopt(argc, argv, "r:a:p:vhs:d:b:")) != -1) {
        switch (c) {
        case 'r':
            if (strlen(optarg) >= PATH_MAX)
                usage();
            config_root_db_dir = optarg;
            break;
        case 'a':
            addr = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'v':
            log_verbose = 1;
            break;
        case 's':
            config_wal_prealloc_size = atoi(optarg);
            if (config_wal_prealloc_size % HDD_BLOCK_SIZE != 0)
                usage();
            break;
        case 'd':
            config_max_lsn_drift = atoi(optarg);
            break;
        case 'b':
            config_user_hash_bits = atoi(optarg);
            break;
        case 'h':
        case '?':
        default:
            usage();
        }
    }
    if (argc != optind)
        usage();

    if (strcmp(config_root_db_dir, ROOT_DB_DIR) != 0)
        INFO("use root_db_dir %s", config_root_db_dir);
    if (config_wal_prealloc_size != WAL_PREALLOC_SIZE)
        INFO("use alternative wal_prealloc_size %d", config_wal_prealloc_size);
    if (config_max_lsn_drift != MAX_LSN_DRIFT)
        INFO("use alternative max_lsn_drift %d", config_max_lsn_drift);
    if (config_user_hash_bits != USER_HASH_BITS)
        INFO("use alternative user_hash_bits %d", config_user_hash_bits);

    user_init();
    accdb_open();
    open_and_replay_wal();
    start_idle_thread();
    start_tcp_server(addr, port);

    return 0;
}
