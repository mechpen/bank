#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "accdb.h"

char *bank_root_dir = ".";
int log_verbose = 0;

extern void open_and_replay_wal(void);
extern void start_idle_thread(void);
extern void start_tcp_server(const char *addr_str, const char *port_str);

static void usage(void)
{
	fprintf(stderr,
			"Usage: bank [ -r root_dir ] [ -a addr ] [ -p port ] [ -v ] [ -h ]\n\n"
			"    -r  set db root dir to <root_dir>, default is \".\"\n"
			"    -a  bind to address <addr>, default is \"127.0.0.1\"\n"
			"    -p  bind to port <port>, default is \"7890\"\n"
			"    -v  verbose output\n"
			"    -h  print this message and quit\n\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int c;
	char *addr = "127.0.0.1", *port = "7890";

	while ((c = getopt(argc, argv, "r:a:p:vh")) != -1) {
		switch (c) {
		case 'r':
			if (strlen(optarg) >= PATH_MAX)
				usage();
			bank_root_dir = optarg;
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
		case 'h':
		case '?':
		default:
			usage();
		}
	}
	if (argc != optind)
		usage();

	accdb_open();
	open_and_replay_wal();
	start_idle_thread();
	start_tcp_server(addr, port);

	return 0;
}
