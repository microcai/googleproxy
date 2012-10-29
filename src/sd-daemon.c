/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  Copyright 2010 Lennart Poettering

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef __BIONIC__
#include <linux/fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

#if defined(__linux__)
#include <mqueue.h>
#endif

#include "sd-daemon.h"

#if (__GNUC__ >= 4)
#ifdef SD_EXPORT_SYMBOLS
/* Export symbols */
#define _sd_export_ __attribute__ ((visibility("default")))
#else
/* Don't export the symbols */
#define _sd_export_ __attribute__ ((visibility("hidden")))
#endif
#else
#define _sd_export_
#endif

_sd_export_ int sd_listen_fds(int unset_environment) {

#if defined(DISABLE_SYSTEMD) || !defined(__linux__)
        return 0;
#else
        int r, fd;
        const char *e;
        char *p = NULL;
        unsigned long l;

        if (!(e = getenv("LISTEN_PID"))) {
                r = 0;
                goto finish;
        }

        errno = 0;
        l = strtoul(e, &p, 10);

        if (errno != 0) {
                r = -errno;
                goto finish;
        }

        if (!p || *p || l <= 0) {
                r = -EINVAL;
                goto finish;
        }

        /* Is this for us? */
        if (getpid() != (pid_t) l) {
                r = 0;
                goto finish;
        }

        if (!(e = getenv("LISTEN_FDS"))) {
                r = 0;
                goto finish;
        }

        errno = 0;
        l = strtoul(e, &p, 10);

        if (errno != 0) {
                r = -errno;
                goto finish;
        }

        if (!p || *p) {
                r = -EINVAL;
                goto finish;
        }

        for (fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START + (int) l; fd ++) {
                int flags;

                if ((flags = fcntl(fd, F_GETFD)) < 0) {
                        r = -errno;
                        goto finish;
                }

                if (flags & FD_CLOEXEC)
                        continue;

                if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
                        r = -errno;
                        goto finish;
                }
        }

        r = (int) l;

finish:
        if (unset_environment) {
                unsetenv("LISTEN_PID");
                unsetenv("LISTEN_FDS");
        }

        return r;
#endif
}

_sd_export_ int sd_is_fifo(int fd, const char *path) {
        struct stat st_fd;

        if (fd < 0)
                return -EINVAL;

        if (fstat(fd, &st_fd) < 0)
                return -errno;

        if (!S_ISFIFO(st_fd.st_mode))
                return 0;

        if (path) {
                struct stat st_path;

                if (stat(path, &st_path) < 0) {

                        if (errno == ENOENT || errno == ENOTDIR)
                                return 0;

                        return -errno;
                }

                return
                        st_path.st_dev == st_fd.st_dev &&
                        st_path.st_ino == st_fd.st_ino;
        }

        return 1;
}

_sd_export_ int sd_is_special(int fd, const char *path) {
        struct stat st_fd;

        if (fd < 0)
                return -EINVAL;

        if (fstat(fd, &st_fd) < 0)
                return -errno;

        if (!S_ISREG(st_fd.st_mode) && !S_ISCHR(st_fd.st_mode))
                return 0;

        if (path) {
                struct stat st_path;

                if (stat(path, &st_path) < 0) {

                        if (errno == ENOENT || errno == ENOTDIR)
                                return 0;

                        return -errno;
                }

                if (S_ISREG(st_fd.st_mode) && S_ISREG(st_path.st_mode))
                        return
                                st_path.st_dev == st_fd.st_dev &&
                                st_path.st_ino == st_fd.st_ino;
                else if (S_ISCHR(st_fd.st_mode) && S_ISCHR(st_path.st_mode))
                        return st_path.st_rdev == st_fd.st_rdev;
                else
                        return 0;
        }

        return 1;
}

static int sd_is_socket_internal(int fd, int type, int listening) {
        struct stat st_fd;

        if (fd < 0 || type < 0)
                return -EINVAL;

        if (fstat(fd, &st_fd) < 0)
                return -errno;

        if (!S_ISSOCK(st_fd.st_mode))
                return 0;

        if (type != 0) {
                int other_type = 0;
                socklen_t l = sizeof(other_type);

                if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &other_type, &l) < 0)
                        return -errno;

                if (l != sizeof(other_type))
                        return -EINVAL;

                if (other_type != type)
                        return 0;
        }

        if (listening >= 0) {
                int accepting = 0;
                socklen_t l = sizeof(accepting);

                if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &accepting, &l) < 0)
                        return -errno;

                if (l != sizeof(accepting))
                        return -EINVAL;

                if (!accepting != !listening)
                        return 0;
        }

        return 1;
}

union sockaddr_union {
        struct sockaddr sa;
        struct sockaddr_in in4;
        struct sockaddr_in6 in6;
        struct sockaddr_un un;
        struct sockaddr_storage storage;
};

_sd_export_ int sd_is_socket(int fd, int family, int type, int listening) {
        int r;

        if (family < 0)
                return -EINVAL;

        if ((r = sd_is_socket_internal(fd, type, listening)) <= 0)
                return r;

        if (family > 0) {
                union sockaddr_union sockaddr;
                socklen_t l;

                memset(&sockaddr, 0, sizeof(sockaddr));
                l = sizeof(sockaddr);

                if (getsockname(fd, &sockaddr.sa, &l) < 0)
                        return -errno;

                if (l < sizeof(sa_family_t))
                        return -EINVAL;

                return sockaddr.sa.sa_family == family;
        }

        return 1;
}

_sd_export_ int sd_is_socket_inet(int fd, int family, int type, int listening, uint16_t port) {
        union sockaddr_union sockaddr;
        socklen_t l;
        int r;

        if (family != 0 && family != AF_INET && family != AF_INET6)
                return -EINVAL;

        if ((r = sd_is_socket_internal(fd, type, listening)) <= 0)
                return r;

        memset(&sockaddr, 0, sizeof(sockaddr));
        l = sizeof(sockaddr);

        if (getsockname(fd, &sockaddr.sa, &l) < 0)
                return -errno;

        if (l < sizeof(sa_family_t))
                return -EINVAL;

        if (sockaddr.sa.sa_family != AF_INET &&
            sockaddr.sa.sa_family != AF_INET6)
                return 0;

        if (family > 0)
                if (sockaddr.sa.sa_family != family)
                        return 0;

        if (port > 0) {
                if (sockaddr.sa.sa_family == AF_INET) {
                        if (l < sizeof(struct sockaddr_in))
                                return -EINVAL;

                        return htons(port) == sockaddr.in4.sin_port;
                } else {
                        if (l < sizeof(struct sockaddr_in6))
                                return -EINVAL;

                        return htons(port) == sockaddr.in6.sin6_port;
                }
        }

        return 1;
}
