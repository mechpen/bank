#ifndef _BANK_API_H
#define _BANK_API_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>

/* requests */

#define MAX_REQ_LEN   200

#define NEW_ACC       "OPEN\n"
#define DEPOSIT       "DEPOSIT %lu TO %lu\n"
#define WITHDRAW      "WITHDRAW %lu FROM %lu\n"
#define TRANSFER      "TRANSFER %lu FROM %lu TO %lu\n"

/* responses */

#define MAX_RES_LEN   200

#define NEW_ACC_OK    "200 account number %lu\n"
#define UPDATE_OK     "200 old amount %lu new amount %lu\n"

#define SERVER_ERROR  "500 Server Error\n"
#define REQUEST_ERROR "400 Bad Request\n"
#define INSUFF_ERROR  "410 Insufficient Money\n"
#define OVERFL_ERROR  "420 Too Much Money\n"
#define NO_ACC_ERROR  "430 Account Does NOT Exist\n"

#define reply(fd, fmt, args...) ({                     \
	char __buf[MAX_RES_LEN];		                   \
	snprintf(__buf, sizeof(__buf), fmt, ##args);       \
	send(fd, __buf, strlen(__buf), 0);                 \
})

#endif
