#ifndef _BANK_HELPER_H
#define _BANK_HELPER_H

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "log.h"

#define ensure_pthread_mutex_lock(m) ({                        \
    int __ret;                                                 \
    if ((__ret = pthread_mutex_lock(m)) != 0)                  \
        ERROR_EXIT("mutex_lock %s", strerror(__ret));          \
})

#define ensure_pthread_mutex_unlock(m) ({                      \
    int __ret;                                                 \
    if ((__ret = pthread_mutex_unlock(m)) != 0)                \
        ERROR_EXIT("mutex_unlock%s", strerror(__ret));         \
})

#define ensure_pthread_cond_wait(c, m) ({                      \
    int __ret;                                                 \
    if ((__ret = pthread_cond_wait((c), (m))) != 0)            \
        ERROR_EXIT("cond_wait %s", strerror(__ret));           \
})

#define ensure_pthread_cond_broadcast(c) ({                    \
    int __ret;                                                 \
    if ((__ret = pthread_cond_broadcast(c)) != 0)              \
        ERROR_EXIT("cond_broadcast%s", strerror(__ret));       \
})

#define ensure_fallocate(fd, off, len) ({                      \
    int __ret;                                                 \
    if ((__ret = posix_fallocate(fd, off, len)) != 0)          \
        ERROR_EXIT("fallocate %s", strerror(__ret));           \
})

#define ensure_fsync(fd) ({                                    \
    if (fsync(fd) < 0)                                         \
        ERROR_EXIT("fsync %s", strerror(errno));               \
})

#define ensure_fdatasync(fd) ({                                \
    if (fdatasync(fd) < 0)                                     \
        ERROR_EXIT("fdatasync %s", strerror(errno));           \
})

static inline void ensure_pread(int fd, void *buf, size_t cnt, off_t off)
{
    size_t num, ret;
    for (num = 0; num < cnt; num += ret) {
        ret = pread(fd, buf+num, cnt-num, off+num);
        if (ret < 0)
            ERROR_EXIT("pread %s", strerror(errno));
        if (ret == 0)
            break;
    }
    if (num == 0)
        memset(buf, 0, cnt);
    else if (num < cnt)
        ERROR_EXIT("partial read");
}

static inline void ensure_pwrite(int fd, void *buf, size_t cnt, off_t off)
{
    size_t num, ret;
    for (num = 0; num < cnt; num += ret) {
        ret = pwrite(fd, buf+num, cnt-num, off+num);
        if (ret < 0)
            ERROR_EXIT("pwrite %s", strerror(errno));
        if (ret == 0)
            break;
    }
    if (num < cnt)
        ERROR_EXIT("partial write");
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

#endif
