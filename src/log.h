#ifndef _BANK_LOG_H
#define _BANK_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

extern int log_verbose;

#define DBG(fmt, args...) ({                                \
    if (log_verbose == 1) {                                 \
        struct timeval tv;                                  \
        gettimeofday(&tv, NULL);                            \
        fprintf(stderr, "%ld.%02ld [dbg] %s: " fmt "\n",    \
                tv.tv_sec, tv.tv_usec/10000,                \
                __func__, ##args);                          \
    }                                                       \
})

#define INFO(fmt, args...) ({                               \
    struct timeval tv;                                      \
    gettimeofday(&tv, NULL);                                \
    fprintf(stderr, "%ld.%02ld [info] " fmt "\n",           \
        tv.tv_sec, tv.tv_usec/10000,                        \
        ##args);                                            \
})

#define ERROR(fmt, args...) ({                              \
    struct timeval tv;                                      \
    gettimeofday(&tv, NULL);                                \
    fprintf(stderr, "%ld.%02ld [error] %s: " fmt "\n",      \
        tv.tv_sec, tv.tv_usec/10000,                        \
        __func__, ##args);                                  \
})

#define ERROR_EXIT(fmt, args...) ({                         \
    ERROR(fmt, ##args);                                     \
    exit(-1);                                               \
})

#endif
