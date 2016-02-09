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
#define HISTORY       "HISTORY %lu\n"

/* responses */

#define MAX_RES_LEN   200

#define NEW_ACC_OK    "200 - ACCOUNT NUMBER %lu\n"
#define UPDATE_OK     "200 - OLD AMOUNT %lu NEW AMOUNT %lu\n"

#define HISTORY_NEW_ACC         "210 - AT %lu OPENED\n"
#define HISTORY_DEPOSIT         "210 - AT %lu DEPOSITED %lu\n"
#define HISTORY_WITHDRAW        "210 - AT %lu WITHDREW %lu\n"
#define HISTORY_TRANSFER_FROM   "210 - AT %lu TRANSFERED %lu FROM %lu\n"
#define HISTORY_TRANSFER_TO     "210 - AT %lu TRANSFERED %lu TO %lu\n"
#define HISTORY_CURRENT         "210 - CURRENT %lu\n"
#define MULTI_LINE_END          "210 - \n"

#define SERVER_ERROR  "500 - SERVER ERROR\n"
#define REQUEST_ERROR "400 - BAD REQUEST\n"
#define INSUFF_ERROR  "410 - INSUFFICIENT MONEY\n"
#define OVERFL_ERROR  "420 - TOO MUCH MONEY\n"

#define reply(fd, fmt, args...) ({                     \
    char __buf[MAX_RES_LEN];                           \
    snprintf(__buf, sizeof(__buf), fmt, ##args);       \
    send(fd, __buf, strlen(__buf), 0);                 \
})

#endif
