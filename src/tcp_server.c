#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "api.h"
#include "log.h"

extern void handle_request(int fd, const char *req);

void *worker(void *arg)
{
	int fd = (int)(long)arg;
	FILE *stream;

	stream = fdopen(fd, "r");
	if (stream == NULL) {
		ERROR("%s", strerror(errno));
		reply(fd, SERVER_ERROR);
		close(fd);
		return NULL;
	}
	for (;;) {
		char buf[MAX_REQ_LEN];
		if (fgets(buf, sizeof(buf), stream) == NULL) {
			fclose(stream);
			return NULL;
		}
		handle_request(fd, buf);
	}
}

int tcp_listen(const char *addr_str, const char *port_str)
{
	int sock;
	const int on = 1;
	struct in_addr addr;
	in_port_t port;
	struct sockaddr_in saddr;

	if (inet_aton(addr_str, &addr) == 0)
		ERROR_EXIT("Invalid addr %s", addr_str);

	port = atoi(port_str);
	if (port == 0 || port & 0xffff0000)
		ERROR_EXIT("Invalid port %s", port_str);

	saddr.sin_family = AF_INET;
	saddr.sin_addr = addr;
	saddr.sin_port = htons(port);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		ERROR_EXIT("%s", strerror(errno));
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERROR_EXIT("%s", strerror(errno));

	if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
		ERROR_EXIT("%s", strerror(errno));
	if (listen(sock, 5) < 0)
		ERROR_EXIT("%s", strerror(errno));
	return sock;
}

void start_tcp_server(const char *addr_str, const char *port_str)
{
	int listenfd, fd, ret;
	pthread_t tid;

	listenfd = tcp_listen(addr_str, port_str);
	INFO("server listening on %s:%s", addr_str, port_str);
	for (;;) {
		fd = accept(listenfd, NULL, NULL);
		if (fd < 0) {
			ERROR("%s", strerror(errno));
			continue;
		}
		ret = pthread_create(&tid, NULL, &worker, (void *)(long)fd);
		if (ret != 0) {
			ERROR("%s", strerror(ret));
			reply(fd, SERVER_ERROR);
			close(fd);
		}
	}
}
