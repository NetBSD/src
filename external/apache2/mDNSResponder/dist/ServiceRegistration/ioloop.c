/* ioloop.c
 *
 * Copyright (c) 2018-2023 Apple, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Simple event dispatcher for DNS.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#ifdef USE_KQUEUE
#include <sys/event.h>
#endif
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <spawn.h>

#include "dns_sd.h"

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#ifndef EXCLUDE_TLS
#include "srp-tls.h"
#endif
#include "ifpermit.h"

#ifndef IOLOOP_MACOS

typedef struct async_event {
    struct async_event *next;
    async_callback_t callback;
    void *context;
} async_event_t;

io_t *ios;
wakeup_t *wakeups;
subproc_t *subprocesses;
async_event_t *async_events;
int64_t ioloop_now;

#ifdef USE_KQUEUE
int kq;
#endif
static void subproc_finalize(subproc_t *subproc);

int
getipaddr(addr_t *addr, const char *p)
{
    if (inet_pton(AF_INET, p, &addr->sin.sin_addr)) {
        addr->sa.sa_family = AF_INET;
#ifndef NOT_HAVE_SA_LEN
        addr->sa.sa_len = sizeof addr->sin;
#endif
        return sizeof addr->sin;
    }  else if (inet_pton(AF_INET6, p, &addr->sin6.sin6_addr)) {
        addr->sa.sa_family = AF_INET6;
#ifndef NOT_HAVE_SA_LEN
        addr->sa.sa_len = sizeof addr->sin6;
#endif
        return sizeof addr->sin6;
    } else {
        return 0;
    }
}

int64_t
ioloop_timenow()
{
    int64_t now;
    struct timeval tv;
    gettimeofday(&tv, 0);
    now = (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
    return now;
}

static void
message_finalize(message_t *message)
{
    free(message);
}

void
ioloop_message_retain_(message_t *message, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(message, message);
}

void
ioloop_message_release_(message_t *message, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(message, message);
}

void
ioloop_close(io_t *io)
{
    close(io->fd);
    io->fd = -1;
}

static void
add_io(io_t *io)
{
    io_t **iop;

    // Add the new reader to the end of the list if it's not on the list.
    for (iop = &ios; *iop != NULL && *iop != io; iop = &((*iop)->next))
        ;
    if (*iop == NULL) {
        *iop = io;
        io->next = NULL;
        RETAIN_HERE(io, io);
    }
}

void
ioloop_add_reader(io_t *io, io_callback_t callback)
{
    add_io(io);

    io->read_callback = callback;
#ifdef USE_SELECT
    io->want_read = true;
#endif
#ifdef USE_EPOLL
#endif
#ifdef USE_KQUEUE
    struct kevent ev;
    int rv;
    EV_SET(&ev, io->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, io);
    rv = kevent(kq, &ev, 1, NULL, 0, NULL);
    if (rv < 0) {
        ERROR("kevent add: %s", strerror(errno));
        return;
    }
#endif // USE_EPOLL
}

void
ioloop_add_writer(io_t *io, io_callback_t callback)
{
    add_io(io);

    io->write_callback = callback;
#ifdef USE_SELECT
    io->want_write = true;
#endif
#ifdef USE_EPOLL
#endif
#ifdef USE_KQUEUE
    struct kevent ev;
    int rv;
    EV_SET(&ev, io->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, io);
    rv = kevent(kq, &ev, 1, NULL, 0, NULL);
    if (rv < 0) {
        ERROR("kevent add: %s", strerror(errno));
        return;
    }
#endif // USE_EPOLL
}

void
drop_writer(io_t *io)
{
#ifdef USE_SELECT
    io->want_write = false;
#endif
#ifdef USE_EPOLL
#endif
#ifdef USE_KQUEUE
    struct kevent ev;
    int rv;
    EV_SET(&ev, io->fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, io);
    rv = kevent(kq, &ev, 1, NULL, 0, NULL);
    if (rv < 0) {
        ERROR("kevent add: %s", strerror(errno));
        return;
    }
#endif // USE_EPOLL
}

static void
add_remove_wakeup(wakeup_t *wakeup, bool remove)
{
    wakeup_t **p_wakeups;

    // Add the new reader to the end of the list if it's not on the list.
    for (p_wakeups = &wakeups; *p_wakeups != NULL && *p_wakeups != wakeup; p_wakeups = &((*p_wakeups)->next))
        ;
    if (remove) {
        void *wakeup_context = wakeup->context;
        finalize_callback_t finalize = wakeup->finalize;
        wakeup->context = NULL;
        if (wakeup->finalize != NULL) {
            wakeup->finalize = NULL;
            wakeup_finalize(wakeup_context);
        }
        if (*p_wakeups != NULL) {
            *p_wakeups = wakeup->next;
            wakeup->next = NULL;
        }
    } else {
        if (*p_wakeups == NULL) {
            *p_wakeups = wakeup;
            wakeup->next = NULL;
        }
    }
}

static void
wakeup_finalize(void *context)
{
    wakeup_t *wakeup = context;
    add_remove_wakeup(wakeup, true);
    free(wakeup);
}

void
ioloop_wakeup_retain_(wakeup_t *wakeup, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(wakeup, wakeup);
}

void
ioloop_wakeup_release_(wakeup_t *wakeup, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(wakeup, wakeup);
}

wakeup_t *
ioloop_wakeup_create_(const char *file, int line)
{
    wakeup_t *ret = calloc(1, sizeof(*ret));
    if (ret) {
        RETAIN(ret, wakeup);
    }
    return ret;
}

bool
ioloop_add_wake_event(wakeup_t *wakeup, void *context, wakeup_callback_t callback, wakeup_callback_t finalize, int milliseconds)
{
    if (callback == NULL) {
        ERROR("ioloop_add_wake_event called with null callback");
        return false;
    }
    if (milliseconds < 0) {
        ERROR("ioloop_add_wake_event called with negative timeout");
        return false;
    }
    INFO("%p %p %d", wakeup, context, milliseconds);
    add_remove_wakeup(wakeup, true);
    add_remove_wakeup(wakeup, false);
    wakeup->wakeup_time = ioloop_timenow() + milliseconds;
    wakeup->finalize = finalize;
    wakeup->wakeup = callback;
    wakeup->context = context;
    return true;
}

void
ioloop_cancel_wake_event(wakeup_t *wakeup)
{
    add_remove_wakeup(wakeup, true);
    wakeup->wakeup_time = 0;
}

bool
ioloop_init(void)
{
    signal(SIGPIPE, SIG_IGN); // because why ever?
#ifdef USE_KQUEUE
    kq = kqueue();
    if (kq < 0) {
        ERROR("kqueue(): %s", strerror(errno));
        return false;
    }
#endif
    return true;
}

static void
ioloop_io_finalize(io_t *io)
{
    if (io->io_finalize) {
        io->io_finalize(io);
    } else {
        free(io);
    }
}

int
ioloop_events(int64_t timeout_when)
{
    io_t *io, **iop;
    wakeup_t *wakeup, **p_wakeup;
    int nev = 0, rv;
    int64_t now = ioloop_timenow();
    int64_t next_event;
    int64_t timeout = 0;

    if (ioloop_now != 0) {
        INFO("%lld.%03lld seconds have passed on entry to ioloop_events",
             (long long)((now - ioloop_now) / 1000), (long long)((now - ioloop_now) % 1000));
    }
    ioloop_now = now;

#ifdef USE_SELECT
    int nfds = 0;
    fd_set reads, writes, errors;
    struct timeval tv;

    FD_ZERO(&reads);
    FD_ZERO(&writes);
    FD_ZERO(&errors);
#endif
#ifdef USE_KQUEUE
    struct timespec ts;
#endif

start_over:
    p_wakeup = &wakeups;

    // A timeout of zero means don't time out.
    if (timeout_when == 0) {
        next_event = INT64_MAX;
    } else {
        next_event = timeout_when;
    }

    // Cycle through the list of timeouts.
    while (*p_wakeup) {
        wakeup = *p_wakeup;
        if (wakeup->wakeup_time != 0) {
            if (wakeup->wakeup_time <= ioloop_now) {
                *p_wakeup = wakeup->next;
                wakeup->wakeup_time = 0;
                void *wakeup_context = wakeup->context;
                finalize_callback_t wakeup_finalize = wakeup->finalize;
                wakeup->finalize = NULL;
                wakeup->context = NULL;
                wakeup->wakeup(wakeup_context);
                if (wakeup_finalize != NULL && wakeup_context != NULL) {
                    wakeup_finalize(wakeup_context);
                }
                ++nev;

                // In case either wakeup has been freed, or a new wakeup has been added, we need to start
                // at the beginning again. This wakeup will never still be on the list unless it's been
                // re-added with a later time, so this should always have the effect that every wakeup that's
                // ready gets its callback called, and when all wakeups that are ready have been called,
                // there are no wakeups that are ready remaining on the list, so our loop exits.
                goto start_over;
            } else {
                p_wakeup = &wakeup->next;
            }
            if (wakeup->wakeup_time < next_event && wakeup->wakeup_time != 0) {
                next_event = wakeup->wakeup_time;
            }
        } else {
            *p_wakeup = wakeup->next;
        }
    }

    // Deliver and consume any asynchronous events
    while (async_events != NULL) {
        async_event_t *event = async_events;
        async_events = event->next;
        event->callback(event->context);
        free(event);
    }

    iop = &ios;
    while (*iop) {
        io = *iop;
        // If the I/O is dead, finalize or free it.
        if (io->fd == -1) {
            *iop = io->next;
            RELEASE_HERE(io, io);
            continue;
        }

        // One-time callback, used to call the listener ready callback after ioloop_listener_create() has
        // returned;
        if (io->ready != NULL) {
            io->ready(io, io->context);
            io->ready = NULL;
        }

        iop = &io->next;
    }

    INFO("now: %" PRIu64 " next_event %" PRIu64, ioloop_now, next_event);

    // If we were given a timeout in the future, or told to wait indefinitely, wait until the next event.
    if (timeout_when == 0 || timeout_when > ioloop_now) {
        timeout = next_event - ioloop_now;
        // Don't choose a time so far in the future that it might overflow some math in the kernel.
        if (timeout > IOLOOP_DAY * 100) {
            timeout = IOLOOP_DAY * 100;
        }
#ifdef USE_SELECT
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
#endif
#ifdef USE_KQUEUE
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
#endif
    }

    while (subprocesses != NULL) {
        int status;
        pid_t pid;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            break;
        }
        subproc_t **sp, *subproc;
        for (sp = &subprocesses; (*sp) != NULL; sp = &(*sp)->next) {
            subproc = *sp;
            if (subproc->pid == pid) {
                if (!WIFSTOPPED(status)) {
                    *sp = subproc->next;
                }
                subproc->callback(subproc->context, status, NULL);
                if (!WIFSTOPPED(status)) {
                    subproc->finished = true;
                    RELEASE_HERE(subproc, subproc);
                    break;
                }
            }
        }
    }

#ifdef USE_SELECT
    for (io = ios; io; io = io->next) {
        if (io->fd != -1 && (io->want_read || io->want_write)) {
            if (io->fd >= nfds) {
                nfds = io->fd + 1;
            }
            if (io->want_read) {
                FD_SET(io->fd, &reads);
            }
            if (io->want_write) {
                FD_SET(io->fd, &writes);
            }
        }
    }
#endif

#ifdef USE_SELECT
    INFO("waiting %lld %lld seconds", (long long)tv.tv_sec, (long long)tv.tv_usec);
    rv = select(nfds, &reads, &writes, &errors, &tv);
    if (rv < 0) {
        ERROR("select: %s", strerror(errno));
        exit(1);
    }
    now = ioloop_timenow();
    INFO("%lld.%03lld seconds passed waiting, got %d events", (long long)((now - ioloop_now) / 1000),
         (long long)((now - ioloop_now) % 1000), rv);
    ioloop_now = now;
    for (io = ios; io; io = io->next) {
        if (io->fd != -1) {
            if (FD_ISSET(io->fd, &reads)) {
                if (io->read_callback != NULL) {
                    io->read_callback(io, io->context);
                }
            } else if (FD_ISSET(io->fd, &writes)) {
                if (io->write_callback != NULL) {
                    io->write_callback(io, io->context);
                }
            }
        }
    }
    nev += rv;
#endif // USE_SELECT
#ifdef USE_KQUEUE
#define KEV_MAX 20
    struct kevent evs[KEV_MAX];
    int i;

    INFO("waiting %lld/%lld seconds", (long long)ts.tv_sec, (long long)ts.tv_nsec);
    do {
        rv = kevent(kq, NULL, 0, evs, KEV_MAX, &ts);
        now = ioloop_timenow();
        INFO("%lld.%03lld seconds passed waiting, got %d events", (long long)((now - ioloop_now) / 1000),
             (long long)((now - ioloop_now) % 1000), rv);
        ioloop_now = now;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        if (rv < 0) {
            if (errno == EINTR) {
                rv = 0;
            } else {
                ERROR("kevent poll: %s", strerror(errno));
                exit(1);
            }
        }
        for (i = 0; i < rv; i++) {
            io = evs[i].udata;
            if (evs[i].filter == EVFILT_WRITE) {
                io->write_callback(io, io->context);
            } else if (evs[i].filter == EVFILT_READ) {
                io->read_callback(io, io->context);
            }
        }
        nev += rv;
    } while (rv == KEV_MAX);
#endif
    return nev;
}

int
ioloop(void)
{
    int nev;
    do {
        nev = ioloop_events(0);
        INFO("%d", nev);
    } while (nev >= 0);
    ERROR("ioloop returned %d.", nev);
    return -1;
}
#endif // !defined(IOLOOP_MACOS)

static void
ioloop_normalize_address(addr_t *normalized, addr_t *original)
{
    uint16_t *sinp = (uint16_t *)&original->sin6.sin6_addr;
    // Check for ::ffff:xxxx:xxxx, which is an ipv4mapped address
    if (sinp[0] == 0 && sinp[1] == 0 && sinp[2] == 0 && sinp[3] == 0 && sinp[4] == 0 && sinp[5] == 0xffff) {
        normalized->sin.sin_family = AF_INET;
        memcpy(&normalized->sin.sin_addr, &sinp[6], sizeof(struct in_addr));
        normalized->sin.sin_port = original->sin6.sin6_port;
    } else {
        *normalized = *original;
    }
}

void
ioloop_udp_read_callback(io_t *io, void *context)
{
    comm_t *connection = (comm_t *)context;
    addr_t src;
    ssize_t rv;
    struct msghdr msg;
    struct iovec bufp;
    uint8_t msgbuf[DNS_MAX_UDP_PAYLOAD];
    char cmsgbuf[128];
    struct cmsghdr *cmh;
    message_t *message;
    (void)context;

    bufp.iov_base = msgbuf;
    bufp.iov_len = DNS_MAX_UDP_PAYLOAD;
    msg.msg_iov = &bufp;
    msg.msg_iovlen = 1;
    msg.msg_name = &src;
    msg.msg_namelen = sizeof src;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof cmsgbuf;

    rv = recvmsg(io->fd, &msg, 0);
    if (rv < 0) {
        ERROR("%s", strerror(errno));
        return;
    }
    message = ioloop_message_create(rv);
    if (!message) {
        ERROR("out of memory");
        return;
    }
    memcpy(&message->src, &src, sizeof src);
    if (rv > UINT16_MAX) {
        ERROR("message is surprisingly large: %zd", rv);
        return;
    }
    message->length = (uint16_t)rv;
    memcpy(&message->wire, msgbuf, rv);

    // For UDP, we use the interface index as part of the validation strategy, so go get
    // the interface index.
    bool set_local = false;
    for (cmh = CMSG_FIRSTHDR(&msg); cmh; cmh = CMSG_NXTHDR(&msg, cmh)) {
        addr_t source_address, local_address;

        if (cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_PKTINFO) {
            struct in6_pktinfo pktinfo;

            memcpy(&pktinfo, CMSG_DATA(cmh), sizeof pktinfo);
            message->ifindex = pktinfo.ipi6_ifindex;

            /* Get address to which the message was sent, for use when replying. */
            message->local.sin6.sin6_family = AF_INET6;
            message->local.sin6.sin6_port = htons(connection->listen_port);
            message->local.sin6.sin6_addr = pktinfo.ipi6_addr;
#ifndef NOT_HAVE_SA_LEN
            message->local.sin6.sin6_len = sizeof message->local;
#endif
            set_local = true;
        } else if (cmh->cmsg_level == IPPROTO_IP && cmh->cmsg_type == IP_PKTINFO) {
            struct in_pktinfo pktinfo;

            memcpy(&pktinfo, CMSG_DATA(cmh), sizeof pktinfo);
            message->ifindex = pktinfo.ipi_ifindex;

            message->local.sin.sin_family = AF_INET;
            message->local.sin.sin_addr = pktinfo.ipi_addr;
#ifndef NOT_HAVE_SA_LEN
            message->local.sin.sin_len = sizeof message->local;
#endif
            message->local.sin.sin_port = htons(connection->listen_port);
            set_local = true;
        }
        if (set_local) {
            ioloop_normalize_address(&source_address, &src);
            ioloop_normalize_address(&local_address, &message->local);
            if (source_address.sa.sa_family == AF_INET6) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&source_address.sin6.sin6_addr, src_addr_buf);
                SEGMENTED_IPv6_ADDR_GEN_SRP(&local_address.sin6.sin6_addr, dest_addr_buf);
                INFO("received %zd byte UDP message on index %d to " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d from "
                     PRI_SEGMENTED_IPv6_ADDR_SRP "#%d", rv, message->ifindex,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&local_address.sin6.sin6_addr,  dest_addr_buf),
                     ntohs(local_address.sin6.sin6_port),
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&source_address.sin6.sin6_addr, src_addr_buf),
                     ntohs(source_address.sin6.sin6_port));
            } else {
                IPv4_ADDR_GEN_SRP(&source_address.sin.sin_addr.s_addr, src_addr_buf);
                IPv4_ADDR_GEN_SRP(&local_address.sin.sin_addr.s_addr, dest_addr_buf);
                INFO("received %zd byte UDP message on index %d to " PRI_IPv4_ADDR_SRP "#%d from " PRI_IPv4_ADDR_SRP "#%d", rv,
                     message->ifindex, IPv4_ADDR_PARAM_SRP(&local_address.sin.sin_addr.s_addr, dest_addr_buf),
                     ntohs(local_address.sin.sin_port),
                     IPv4_ADDR_PARAM_SRP(&local_address.sin.sin_addr.s_addr, src_addr_buf),
                     ntohs(source_address.sin.sin_port));
            }
        }
    }

    // The first packet we get via inetd will not have the PKTINFO sockopt set, since we can only set that after we've
    // started. We can expect a retransmission, so just drop it rather than trying to do something clever.
    if (set_local) {
        connection->datagram_callback(connection, message, connection->context);
    } else {
        ERROR("dropping incoming packet because we didn't get a destination address.");
    }
    ioloop_message_release(message);
}

#ifndef IOLOOP_MACOS
static void
tcp_read_callback(io_t *io, void *context)
{
    uint8_t *read_ptr;
    size_t read_len;
    comm_t *connection = (comm_t *)io;
    ssize_t rv;
    (void)context;
    if (connection->message_length_len < 2) {
        read_ptr = connection->message_length_bytes;
        read_len = 2 - connection->message_length_len;
    } else {
        read_ptr = &connection->buf[connection->message_cur];
        read_len = connection->message_length - connection->message_cur;
    }

    if (connection->tls_context != NULL) {
#ifndef EXCLUDE_TLS
        rv = srp_tls_read(connection, read_ptr, read_len);
        if (rv == 0) {
            // This isn't an EOF: that's returned as an error status.   This just means that
            // whatever data was available to be read was consumed by the TLS protocol without
            // producing anything to read at the app layer.
            return;
        } else if (rv < 0) {
            ERROR("TLS return that we can't handle.");
            close(connection->io.fd);
            connection->io.fd = -1;
            srp_tls_context_free(connection);
            return;
        }
#else
        ERROR("tls context with TLS excluded in tcp_read_callback.");
        return;
#endif
    } else {
        rv = read(connection->io.fd, read_ptr, read_len);

        if (rv < 0) {
            ERROR("tcp_read_callback: %s", strerror(errno));
            close(connection->io.fd);
            connection->io.fd = -1;
            // connection->io.finalize() will be called from the io loop.
            return;
        }

        // If we read zero here, the remote endpoint has closed or shutdown the connection.  Either case is
        // effectively the same--if we are sensitive to read events, that means that we are done processing
        // the previous message.
        if (rv == 0) {
            ERROR("tcp_read_callback: remote end (%s) closed connection on %d", connection->name, connection->io.fd);
            close(connection->io.fd);
            connection->io.fd = -1;
            if (connection->disconnected) {
                connection->disconnected(connection, connection->context, 0);
            }
            // connection->io.finalize() will be called from the io loop.
            return;
        }
    }
    if (connection->message_length_len < 2) {
        connection->message_length_len += rv;
        if (connection->message_length_len == 2) {
            connection->message_length = (((uint16_t)connection->message_length_bytes[0] << 8) |
                                          ((uint16_t)connection->message_length_bytes[1]));

            if (connection->message == NULL) {
                connection->message = ioloop_message_create(connection->message_length);
                if (!connection->message) {
                    ERROR("udp_read_callback: out of memory");
                    return;
                }
                connection->buf = (uint8_t *)&connection->message->wire;
                connection->message->length = connection->message_length;
                memset(&connection->message->src, 0, sizeof connection->message->src);
            }
        }
    } else {
        connection->message_cur += rv;
        if (connection->message_cur == connection->message_length) {
            connection->message_cur = 0;
            connection->datagram_callback(connection, connection->message, connection->context);
            // The callback may retain the message; we need to make way for the next one.
            ioloop_message_release(connection->message);
            connection->message = NULL;
            connection->message_length = connection->message_length_len = 0;
        }
    }
}


static bool
tcp_send_response(comm_t *comm, message_t *responding_to, struct iovec *iov, int iov_len, bool send_length)
{
    struct msghdr mh;
    struct iovec iovec[4];
    char lenbuf[2];
    ssize_t status;
    size_t payload_length = 0;
    int i;

    // We don't anticipate ever needing more than four hunks, but if we get more, handle then?
    if (iov_len > 3) {
        ERROR("tcp_send_response: too many io buffers");
        close(comm->io.fd);
        comm->io.fd = -1;
        return false;
    }

    i = 0;
    if (send_length) {
        i++;
    }
    for (i = 0; i < iov_len; i++) {
        iovec[i + 1] = iov[i];
        payload_length += iov[i].iov_len;
    }
    if (send_length) {
        iovec[0].iov_base = &lenbuf[0];
        iovec[0].iov_len = 2;

        lenbuf[0] = payload_length / 256;
        lenbuf[1] = payload_length & 0xff;

        payload_length += 2;
    }

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
    if (comm->tls_context != NULL) {
#ifndef EXCLUDE_TLS
        status = srp_tls_write(comm, iovec, iov_len + 1);
#else
        ERROR("TLS context not null with TLS excluded.");
        status = -1;
        errno = ENOTSUP;
        return false;
#endif
    } else {
        memset(&mh, 0, sizeof mh);
        mh.msg_iov = &iovec[0];
        mh.msg_iovlen = iov_len + 1;
        mh.msg_name = 0;

        status = sendmsg(comm->io.fd, &mh, MSG_NOSIGNAL);
    }
    if (status < 0 || status != payload_length) {
        if (status < 0) {
            ERROR("tcp_send_response: write failed: %s", strerror(errno));
        } else {
            ERROR("tcp_send_response: short write (%zd out of %zu bytes)", status, payload_length);
        }
        close(comm->io.fd);
        comm->io.fd = -1;
        return false;
    }
    return true;
}
#endif // !IOLOOP_MACOS

#if !defined(IOLOOP_MACOS) || !UDP_LISTENER_USES_CONNECTION_GROUPS
bool
ioloop_udp_send_message(comm_t *comm, addr_t *source, addr_t *dest, int ifindex, struct iovec *iov, int iov_len)
{
    struct msghdr mh;
    uint8_t cmsg_buf[128];
    struct cmsghdr *cmsg;
    ssize_t status;

    memset(&mh, 0, sizeof mh);
    mh.msg_iov = iov;
    mh.msg_iovlen = iov_len;
    mh.msg_name = dest;
    mh.msg_control = cmsg_buf;
    if (source == NULL) {
        mh.msg_controllen = 0;
    } else {
        mh.msg_controllen = sizeof cmsg_buf;
        cmsg = CMSG_FIRSTHDR(&mh);

        if (source->sa.sa_family == AF_INET) {
            struct in_pktinfo *inp;
            mh.msg_namelen = sizeof (struct sockaddr_in);
            mh.msg_controllen = CMSG_SPACE(sizeof *inp);
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof *inp);
            inp = (struct in_pktinfo *)CMSG_DATA(cmsg);
            memset(inp, 0, sizeof *inp);
            inp->ipi_ifindex = ifindex;
            inp->ipi_spec_dst = source->sin.sin_addr;
            inp->ipi_addr = source->sin.sin_addr;
        } else if (source->sa.sa_family == AF_INET6) {
            struct in6_pktinfo *inp;
            mh.msg_namelen = sizeof (struct sockaddr_in6);
            mh.msg_controllen = CMSG_SPACE(sizeof *inp);
            cmsg->cmsg_level = IPPROTO_IPV6;
            cmsg->cmsg_type = IPV6_PKTINFO;
            cmsg->cmsg_len = CMSG_LEN(sizeof *inp);
            inp = (struct in6_pktinfo *)CMSG_DATA(cmsg);
            memset(inp, 0, sizeof *inp);
            inp->ipi6_ifindex = ifindex;
            inp->ipi6_addr = source->sin6.sin6_addr;
        } else {
            ERROR("unknown family %d", source->sa.sa_family);
            abort();
        }
    }
    size_t len = 0;
    for (int i = 0; i < iov_len; i++) {
        len += iov[i].iov_len;
    }
    addr_t dest_addr, source_addr;
    ioloop_normalize_address(&dest_addr, dest);
    if (source != NULL) {
        ioloop_normalize_address(&source_addr, source);
    } else {
        memset(&source_addr, 0, sizeof(source_addr));
        source_addr.sa.sa_family = dest_addr.sa.sa_family;
    }
    if (dest_addr.sa.sa_family == AF_INET) {
        IPv4_ADDR_GEN_SRP(&source_addr.sin.sin_addr.s_addr, ipv4_src_buf);
        IPv4_ADDR_GEN_SRP(&dest_addr.sin.sin_addr.s_addr, ipv4_dest_buf);
        INFO("sending %zd byte UDP response from " PRI_IPv4_ADDR_SRP " port %d index %d to " PRI_IPv4_ADDR_SRP "#%d",
             len, IPv4_ADDR_PARAM_SRP(&source_addr.sin.sin_addr.s_addr, ipv4_src_buf),
             ifindex, ntohs(source_addr.sin.sin_port),
             IPv4_ADDR_PARAM_SRP(&dest_addr.sin.sin_addr.s_addr, ipv4_dest_buf), ntohs(dest_addr.sin.sin_port));
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&source_addr.sin6.sin6_addr.s6_addr, ipv6_src_buf);
        SEGMENTED_IPv6_ADDR_GEN_SRP(&dest_addr.sin6.sin6_addr.s6_addr, ipv6_dest_buf);
        INFO("sending %zd byte UDP response from "
             PRI_SEGMENTED_IPv6_ADDR_SRP " port %d index %d to " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d",
             len, SEGMENTED_IPv6_ADDR_PARAM_SRP(&source_addr.sin6.sin6_addr.s6_addr, ipv6_src_buf),
             ntohs(source_addr.sin6.sin6_port), ifindex,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&dest_addr.sin6.sin6_addr.s6_addr, ipv6_dest_buf),
             ntohs(dest_addr.sin6.sin6_port));
    }
    status = sendmsg(comm->io.fd, &mh, 0);
    if (status < 0) {
        ERROR("%s", strerror(errno));
        return false;
    }
    return true;
}
#endif // !defined(IOLOOP_MACOS) || !UDP_LISTENER_USES_CONNECTION_GROUPS

#ifndef IOLOOP_MACOS
static bool
udp_send_response(comm_t *comm, message_t *responding_to, struct iovec *iov, int iov_len)
{
    return udp_send_message(comm, &responding_to->local, &responding_to->src, responding_to->ifindex, iov, iov_len);
}

bool
ioloop_send_multicast(comm_t *comm, int ifindex, struct iovec *iov, int iov_len)
{
    return udp_send_message(comm, &comm->multicast, ifindex, iov, iov_len);
}

static bool
udp_send_connected_response(comm_t *comm, message_t *responding_to, struct iovec *iov, int iov_len)
{
    int status = writev(comm->io.fd, iov, iov_len);
    (void)responding_to;
    if (status < 0) {
        ERROR("udp_send_connected: %s", strerror(errno));
        return false;
    }
    return true;
}

bool
ioloop_send_message(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    if (connection->tcp_stream) {
        return tcp_send_response(connection, responding_to, iov, iov_len, true);
    } else {
        if (connection->is_connected) {
            return udp_send_connected_response(connection, responding_to, iov, iov_len);
        } else if (connection->is_multicast) {
            ERROR("ioloop_send_message: multicast send must use ioloop_send_multicast!");
            return false;
        } else if (responding_to == NULL) {
            ERROR("ioloop_send_message: not connected and no responding_to message.");
            return false;
        } else {
            return udp_send_response(connection, responding_to, iov, iov_len);
        }
    }
}

bool
ioloop_send_final_message(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    bool ret = ioloop_send_message(connection, responding_to, iov, iov_len);
    if (ret) {
        shutdown(connection->io.fd, SHUT_WR);
    }
    return ret;
}

bool
ioloop_send_data(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    if (connection->tcp_stream) {
        return tcp_send_response(connection, responding_to, iov, iov_len, false);
    }
    return ioloop_send_message(connection, responding_to, iov, iov_len);
}

bool
ioloop_send_final_data(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    if (connection->tcp_stream) {
        bool ret = tcp_send_response(connection, responding_to, iov, iov_len, false);
        if (ret) {
            shutdown(connection->io.fd, SHUT_WR);
        }
        return ret;
    }
    return ioloop_send_message(connection, responding_to, iov, iov_len);
}

static void
io_finalize(io_t *io)
{
    io_t **iop;
    for (iop = &ios; *iop; iop = &(*iop)->next) {
        if (*iop == io) {
            *iop = io->next;
            break;
        }
    }
    free(io);
}

// When a communication is closed, scan the io event list to see if any other ios are referencing this one.
static void
comm_finalize(io_t *io)
{
    comm_t *comm = (comm_t *)io;
    ERROR("comm_finalize");
    if (comm->name != NULL) {
        free(comm->name);
    }
    if (comm->finalize != NULL) {
        comm->finalize(comm->context);
    }
    if (comm->message != NULL) {
        RELEASE_HERE(comm->message, message);
    }
    io_finalize(&comm->io);
}

void
ioloop_comm_retain_(comm_t *comm, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(&comm->io, comm);
}

void
ioloop_comm_release_(comm_t *comm, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(&comm->io, comm);
}

void
ioloop_comm_cancel(comm_t *comm)
{
    close(comm->io.fd);
    comm->io.fd = -1;
}

void
ioloop_comm_context_set(comm_t *comm, void *context, finalize_callback_t callback)
{
    if (comm->context != NULL && comm->finalize != NULL) {
        comm->finalize(comm->context);
    }
    comm->finalize = callback;
    comm->context = context;
}

void
ioloop_comm_connect_callback_set(comm_t *comm, connect_callback_t callback)
{
    comm->connected = callback;
}

void
ioloop_comm_disconnect_callback_set(comm_t *comm, disconnect_callback_t callback)
{
    comm->disconnected = callback;
}

void
ioloop_listener_retain_(comm_t *listener, const char *file, int line)
{
    RETAIN(&listener->io, comm);
}

void
ioloop_listener_release_(comm_t *listener, const char *file, int line)
{
    RELEASE(&listener->io, comm);
}

void
ioloop_listener_cancel(comm_t *connection)
{
    if (connection->io.fd != -1) {
        close(connection->io.fd);
        connection->io.fd = -1;
    }
}

static void
listen_callback(io_t *io, void *context)
{
    comm_t *listener = (comm_t *)io;
    int rv;
    addr_t addr;
    socklen_t addr_len = sizeof addr;
    comm_t *comm;
    char addrbuf[INET6_ADDRSTRLEN + 7];
    int addrlen;
    (void)context;

    rv = accept(listener->io.fd, &addr.sa, &addr_len);
    if (rv < 0) {
        ERROR("accept: %s", strerror(errno));
        close(listener->io.fd);
        listener->io.fd = -1;
        return;
    }
    inet_ntop(addr.sa.sa_family, (addr.sa.sa_family == AF_INET
                                  ? (void *)&addr.sin.sin_addr
                                  : (void *)&addr.sin6.sin6_addr), addrbuf, sizeof addrbuf);
    addrlen = strlen(addrbuf);
    snprintf(&addrbuf[addrlen], (sizeof addrbuf) - addrlen, "%%%d",
             ntohs((addr.sa.sa_family == AF_INET ? addr.sin.sin_port : addr.sin6.sin6_port)));
    comm = calloc(1, sizeof *comm);
    comm->name = strdup(addrbuf);
    comm->io.fd = rv;
    comm->address = addr;
    comm->datagram_callback = listener->datagram_callback;
    comm->tcp_stream = true;
    comm->context = listener->context;

    if (listener->tls_context == (tls_context_t *)-1) {
#ifndef EXCLUDE_TLS
        if (!srp_tls_listen_callback(comm)) {
            ERROR("TLS  setup failed.");
            close(comm->io.fd);
            free(comm);
            return;
        }
#else
        ERROR("TLS context not null in listen_callback when TLS excluded.");
        return;
#endif
    }
    if (listener->connected) {
        listener->connected(comm, listener->context);
    }
    ioloop_add_reader(&comm->io, tcp_read_callback);

#ifdef SO_NOSIGPIPE
    int one = 1;
    rv = setsockopt(comm->io.fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
    if (rv < 0) {
        ERROR("SO_NOSIGPIPE failed: %s", strerror(errno));
    }
#endif
}

static void
listener_ready_callback(io_t *io, void *context)
{
    comm_t *listener = (comm_t *)io;
    if (listener->ready) {
        listener->ready(listener->context, listener->listen_port);
    }
}

comm_t *
ioloop_listener_create(bool stream, bool tls, bool inetd, uint16_t *UNUSED avoid_ports, int UNUSED num_avoid_ports,
                       const addr_t *ip_address, const char *multicast, const char *name,
                       datagram_callback_t datagram_callback, connect_callback_t connected,
                       cancel_callback_t UNUSED cancel, ready_callback_t ready, finalize_callback_t finalize,
                       tls_config_callback_t UNUSED tls_config, unsigned UNUSED ifindex, void *context)
{
    comm_t *listener;
    socklen_t sl;
    int rv;
    int false_flag = 0;
    int true_flag = 1;
    uint16_t port;
    int family = (ip_address != NULL) ? ip_address->sa.sa_family : AF_UNSPEC;
    int real_family = family == AF_UNSPEC ? AF_INET6 : family;
    addr_t sockname;

    listener = calloc(1, sizeof *listener);
    if (listener == NULL) {
        return NULL;
    }
    RETAIN_HERE(&listener->io, comm);
    listener->name = strdup(name);
    if (!listener->name) {
        RELEASE_HERE(&listener->io, comm);
        return NULL;
    }
    listener->io.fd = socket(real_family, stream ? SOCK_STREAM : SOCK_DGRAM, stream ? IPPROTO_TCP : IPPROTO_UDP);
    if (listener->io.fd < 0) {
        ERROR("Can't get socket: %s", strerror(errno));
        goto out;
    }
    rv = setsockopt(listener->io.fd, SOL_SOCKET, SO_REUSEADDR, &true_flag, sizeof true_flag);
    if (rv < 0) {
        ERROR("SO_REUSEADDR failed: %s", strerror(errno));
        goto out;
    }

    rv = setsockopt(listener->io.fd, SOL_SOCKET, SO_REUSEPORT, &true_flag, sizeof true_flag);
    if (rv < 0) {
        ERROR("SO_REUSEPORT failed: %s", strerror(errno));
        goto out;
    }

    if (ip_address == NULL || family == AF_LOCAL) {
        port = 0;
    } else {
        port = (family == AF_INET) ? ip_address->sin.sin_port : ip_address->sin6.sin6_port;
        listener->address = *ip_address;
    }
    listener->address.sa.sa_family = real_family;

    if (multicast != 0) {
        if (stream) {
            ERROR("Unable to do non-datagram multicast.");
            goto out;
        }
        if (family == AF_LOCAL) {
            ERROR("Multicast not supported on local sockets.");
            goto out;
        }
        sl = getipaddr(&listener->multicast, multicast);
        if (sl == 0) {
            goto out;
        }
        if (listener->multicast.sa.sa_family != family) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
            ERROR("multicast address %s from different family than listen address " PRI_SEGMENTED_IPv6_ADDR_SRP ".",
                  multicast, SEGMENTED_IPv6_ADDR_PARAM_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf));
            goto out;
        }
        listener->is_multicast = true;

        if (family == AF_INET) {
            struct ip_mreq im;
            int ttl = 255;
            im.imr_multiaddr = listener->multicast.sin.sin_addr;
            im.imr_interface.s_addr = 0;
            rv = setsockopt(listener->io.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &im, sizeof im);
            if (rv < 0) {
                ERROR("Unable to join %s multicast group: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof ttl);
            if (rv < 0) {
                ERROR("Unable to set IP multicast TTL to 255 for %s: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IP, IP_TTL, &ttl, sizeof ttl);
            if (rv < 0) {
                ERROR("Unable to set IP TTL to 255 for %s: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IP, IP_MULTICAST_LOOP, &false_flag, sizeof false_flag);
            if (rv < 0) {
                ERROR("Unable to set IP Multcast loopback to false for %s: %s", multicast, strerror(errno));
                goto out;
            }
        } else {
            struct ipv6_mreq im;
            int hops = 255;
            im.ipv6mr_multiaddr = listener->multicast.sin6.sin6_addr;
            im.ipv6mr_interface = 0;
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &im, sizeof im);
            if (rv < 0) {
                ERROR("Unable to join %s multicast group: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof hops);
            if (rv < 0) {
                ERROR("Unable to set IPv6 multicast hops to 255 for %s: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof hops);
            if (rv < 0) {
                ERROR("Unable to set IPv6 hops to 255 for %s: %s", multicast, strerror(errno));
                goto out;
            }
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &false_flag, sizeof false_flag);
            if (rv < 0) {
                ERROR("Unable to set IPv6 Multcast loopback to false for %s: %s", multicast, strerror(errno));
                goto out;
            }
        }
    }

    if (family == AF_INET6) {
        // Don't use a dual-stack socket.
        rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_V6ONLY, &true_flag, sizeof true_flag);
        if (rv < 0) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
            ERROR("Unable to set IPv6-only flag on %s socket for " PRI_SEGMENTED_IPv6_ADDR_SRP,
                  tls ? "TLS" : (stream ? "TCP" : "UDP"),
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf));
            goto out;
        }
    }

#ifndef NOT_HAVE_SA_LEN
    sl = listener->address.sa.sa_len;
#else
    sl = real_family == AF_INET ? sizeof(listener->address.sin) : sizeof(listener->address.sin6);
#endif
    if (bind(listener->io.fd, &listener->address.sa, sl) < 0) {
        if (family == AF_INET) {
            IPv4_ADDR_GEN_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf);
            ERROR("Can't bind to " PRI_IPv4_ADDR_SRP "#%d/%s: %s",
                  IPv4_ADDR_PARAM_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf), ntohs(port),
                  tls ? "tlsv4" : "tcpv4", strerror(errno));
        } else {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
            ERROR("Can't bind to " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d/%s: %s",
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf), ntohs(port),
                  tls ? "tlsv6" : "tcpv6", strerror(errno));
        }
    out:
        close(listener->io.fd);
        listener->io.fd = -1;
        RELEASE_HERE(&listener->io, comm);
        return NULL;
    }

    // We may have bound to an unspecified port, so fetch the port we got.
    if (port == 0 && family != AF_LOCAL) {
        if (getsockname(listener->io.fd, (struct sockaddr *)&sockname, &sl) < 0) {
            ERROR("ioloop_listener_create: getsockname: %s", strerror(errno));
            goto out;
        }
        port = ntohs(real_family == AF_INET6 ? sockname.sin6.sin6_port : sockname.sin.sin_port);
    }
    listener->listen_port = port;

    if (tls) {
#ifndef EXCLUDE_TLS
        if (!stream) {
            ERROR("Asked to do TLS over UDP, which we don't do yet.");
            goto out;
        }
        listener->tls_context = (tls_context_t *)-1;
#else
        ERROR("TLS requested when TLS is excluded.");
        goto out;
#endif
    }

    if (stream) {
        if (listen(listener->io.fd, 5 /* xxx */) < 0) {
            if (family == AF_INET) {
                IPv4_ADDR_GEN_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf);
                ERROR("Can't listen on " PRI_IPv4_ADDR_SRP "#%d/%s: %s",
                      IPv4_ADDR_PARAM_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf), ntohs(port),
                      tls ? "tlsv4" : "tcpv4", strerror(errno));
            } else {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
                ERROR("Can't listen on " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d/%s: %s",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf), ntohs(port),
                      tls ? "tlsv6" : "tcpv6", strerror(errno));
            }
            goto out;
        }
        listener->finalize = finalize;
        ioloop_add_reader(&listener->io, listen_callback);
        listener->tcp_stream = true;
    } else {
        rv = setsockopt(listener->io.fd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                        family == AF_INET ? IP_PKTINFO : IPV6_RECVPKTINFO, &true_flag, sizeof true_flag);
        if (rv < 0) {
            ERROR("Can't set %s: %s.", family == AF_INET ? "IP_PKTINFO" : "IPV6_RECVPKTINFO",
                    strerror(errno));
            goto out;
        }
        ioloop_add_reader(&listener->io, udp_read_callback);
    }
    listener->datagram_callback = datagram_callback;
    listener->connected = connected;
    listener->context = context;
    listener->ready = ready;
    listener->io.ready = listener_ready_callback;
    listener->io.context = listener;
    listener->is_listener = true;
    return listener;
}

// This is the callback for when we complete the handshake when connecting to a remote listener.
static void
connect_callback(io_t *io, void *context)
{
    int result;
    socklen_t len = sizeof result;
    comm_t *connection = (comm_t *)io;
    bool getsockopt_failed = false;
    (void)context;

    // If connect failed, indicate that it failed.
    if (getsockopt(io->fd, SOL_SOCKET, SO_ERROR, &result, &len) < 0) {
        result = errno;
        getsockopt_failed = true;
    }
    if (result != 0) {
        ERROR("connect_callback: %ssocket %d: Error %d (%s)", getsockopt_failed ? "getsockopt " : "",
              io->fd, result, strerror(result));
        connection->disconnected(connection, connection->context, result);
        ioloop_comm_cancel(connection);
        return;
    }

    // If this is a TLS connection, set up TLS.
    if (connection->tls_context == (tls_context_t *)-1) {
#ifndef EXCLUDE_TLS
        if (!srp_tls_connect_callback(connection)) {
            connection->disconnected(connection, connection->context, 0);
            ioloop_comm_cancel(connection);
            return;
        }
#else
        ERROR("connect_callback: tls_context triggered with TLS excluded.");
        connection->disconnected(connection, connection->context, 0);
        ioloop_comm_cancel(connection);
        return;
#endif
    }

    // We don't want to say we're connected until the TLS handshake is complete.
    if (!connection->tls_handshake_incomplete) {
        connection->connected(connection, connection->context);
    }
    drop_writer(&connection->io);
    ioloop_add_reader(&connection->io, tcp_read_callback);
}

// Currently we don't do DNS lookups, despite the host identifier being an IP address.
comm_t *NULLABLE
ioloop_connection_create(addr_t *remote_address, bool tls, bool stream, bool stable, bool opportunistic,
                         datagram_callback_t datagram_callback, connect_callback_t connected,
                         disconnect_callback_t disconnected, finalize_callback_t finalize,
                         void * context)
{
    comm_t *connection;
    socklen_t sl;
    char buf[INET6_ADDRSTRLEN + 7];
    char *s;

    if (!stream && (connected != NULL || disconnected != NULL)) {
        ERROR("connected and disconnected callbacks not valid for datagram connections");
        return NULL;
    }
    if (stream && (connected == NULL || disconnected == NULL)) {
        ERROR("connected and disconnected callbacks are required for stream connections");
        return NULL;
    }
    connection = calloc(1, sizeof *connection);
    if (connection == NULL) {
        ERROR("No memory for connection structure.");
        return NULL;
    }
    RETAIN_HERE(&connection->io, comm);
    if (inet_ntop(remote_address->sa.sa_family, (remote_address->sa.sa_family == AF_INET
                                                 ? (void *)&remote_address->sin.sin_addr
                                                 : (void *)&remote_address->sin6.sin6_addr), buf,
                  INET6_ADDRSTRLEN) == NULL) {
        ERROR("inet_ntop failed to convert remote address: %s", strerror(errno));
        RELEASE_HERE(&connection->io, comm);
        return NULL;
    }
    s = buf + strlen(buf);
    sprintf(s, "%%%hu", ntohs(remote_address->sa.sa_family == AF_INET
                              ? remote_address->sin.sin_port
                              : remote_address->sin6.sin6_port));
    connection->name = strdup(buf);
    if (!connection->name) {
        RELEASE_HERE(&connection->io, comm);
        return NULL;
    }
    connection->io.fd = socket(remote_address->sa.sa_family,
                                 stream ? SOCK_STREAM : SOCK_DGRAM, stream ? IPPROTO_TCP : IPPROTO_UDP);
    if (connection->io.fd < 0) {
        ERROR("Can't get socket: %s", strerror(errno));
        RELEASE_HERE(&connection->io, comm);
        return NULL;
    }
    connection->address = *remote_address;
    if (fcntl(connection->io.fd, F_SETFL, O_NONBLOCK) < 0) {
        ERROR("connect_to_host: %s: Can't set O_NONBLOCK: %s", connection->name, strerror(errno));
        RELEASE_HERE(&connection->io, comm);
        return NULL;
    }
    // If a stable address has been requested, request a public address in source address selection.
    if (stable && remote_address->sa.sa_family == AF_INET6) {
// Linux doesn't currently follow RFC5014. These values are defined in linux/in6.h, but this can't be
// safely included because it's incompatible with netinet/in.h. So until this is fixed, these values
// are just copied out of the header; when it is fixed, the #if condition will evaluate to false.
#if defined(LINUX)
#  if !defined(IPV6_PREFER_SRC_PUBLIC)
#    define IPV6_PREFER_SRC_TMP            0x0001
#    define IPV6_PREFER_SRC_PUBLIC         0x0002
#    define IPV6_PREFER_SRC_PUBTMP_DEFAULT 0x0100
#  endif
        int value = IPV6_PREFER_SRC_PUBLIC;
        if (setsockopt(connection->io.fd, IPPROTO_IPV6, IPV6_ADDR_PREFERENCES, &value, sizeof(value)) < 0) {
            ERROR("unable to request stable (public) address: %s", strerror(errno));
            return NULL;
        }
#else // Assume BSD
// BSD doesn't follow RFC5014 either (at least xnu).
        int value = 0;
        if (setsockopt(connection->io.fd, IPPROTO_IPV6, IPV6_PREFER_TEMPADDR, &value, sizeof(value)) < 0) {
            ERROR("unable to request stable (public) address.");
            return NULL;
        }
#endif // LINUX
    }
#ifdef NOT_HAVE_SA_LEN
    sl = (remote_address->sa.sa_family == AF_INET
          ? sizeof remote_address->sin
          : sizeof remote_address->sin6);
#else
    sl = remote_address->sa.sa_len;
#endif
    // Connect to the host
    if (connect(connection->io.fd, &connection->address.sa, sl) < 0) {
        if (errno != EINPROGRESS && errno != EAGAIN) {
            ERROR("Can't connect to %s: %s", connection->name, strerror(errno));
            RELEASE_HERE(&connection->io, comm);
            return NULL;
        }
    }
    // At this point if we are doing TCP, we do not yet have a connection, but the connection should be in
    // progress, and we should get a write select event when the connection succeeds or fails.
    // UDP is connectionless, so the connect() call just sets the default destination for send() on
    // the socket.

    if (tls) {
#ifndef TLS_EXCLUDED
        connection->tls_context = (tls_context_t *)-1;
#else
        ERROR("connect_to_host: tls requested when excluded.");
        RELEASE_HERE(&connection->io, comm);
        return NULL;
#endif
    }

    connection->connected = connected;
    connection->disconnected = disconnected;
    connection->datagram_callback = datagram_callback;
    connection->context = context;
    connection->finalize = finalize;
    connection->opportunistic = opportunistic;
    if (!stream) {
        connection->is_connected = true;
        connection->tcp_stream = false;
        ioloop_add_reader(&connection->io, udp_read_callback);
    } else {
        connection->tcp_stream = true;
        ioloop_add_writer(&connection->io, connect_callback);
    }

    return connection;
}

static void
subproc_finalize(subproc_t *subproc)
{
    int i;
    for (i = 0; i < subproc->argc; i++) {
        if (subproc->argv[i] != NULL) {
            free(subproc->argv[i]);
            subproc->argv[i] = NULL;
        }
    }
    if (subproc->output_fd != NULL) {
        ioloop_file_descriptor_release(subproc->output_fd);
    }
    if (subproc->finalize != NULL) {
        subproc->finalize(subproc->context);
    }
    free(subproc);
}

static void
subproc_output_finalize(void *context)
{
    subproc_t *subproc = context;
    if (subproc->output_fd) {
        subproc->output_fd = NULL;
    }
    RELEASE_HERE(subproc, subproc);
}

void
ioloop_subproc_release_(subproc_t *subproc, const char *file, int line)
{
    RELEASE(subproc, subproc);
}

// Invoke the specified executable with the specified arguments.   Call callback when it exits.
// All failures are reported through the callback.
subproc_t *
ioloop_subproc(const char *exepath, char **argv, int argc, subproc_callback_t callback,
               io_callback_t output_callback, void *context)
{
    subproc_t *subproc;
    int i, rv;
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attrs;

    if (callback == NULL) {
        ERROR("ioloop_subproc called with null callback");
        return NULL;
    }

    if (argc > MAX_SUBPROC_ARGS) {
        callback(NULL, 0, "too many subproc args");
        return NULL;
    }

    subproc = calloc(1, sizeof(*subproc));
    if (subproc == NULL) {
        callback(NULL, 0, "out of memory");
        return NULL;
    }
    RETAIN_HERE(subproc, subproc);
    if (output_callback != NULL) {
        rv = pipe(subproc->pipe_fds);
        if (rv < 0) {
            callback(NULL, 0, "unable to create pipe.");
            RELEASE_HERE(subproc, subproc);
            return NULL;
        }
        subproc->output_fd = ioloop_file_descriptor_create(subproc->pipe_fds[0], subproc, subproc_output_finalize);
        if (subproc->output_fd == NULL) {
            // subproc->output_fd holds a reference to subproc.
            RETAIN_HERE(subproc, subproc);
            callback(NULL, 0, "out of memory.");
            close(subproc->pipe_fds[0]);
            close(subproc->pipe_fds[1]);
            RELEASE_HERE(subproc, subproc);
            return NULL;
        }
    }

    subproc->argv[0] = strdup(exepath);
    if (subproc->argv[0] == NULL) {
        RELEASE_HERE(subproc, subproc);
        callback(NULL, 0, "out of memory");
        return NULL;
    }
    subproc->argc++;
    for (i = 0; i < argc; i++) {
        subproc->argv[i + 1] = strdup(argv[i]);
        if (subproc->argv[i + 1] == NULL) {
            RELEASE_HERE(subproc, subproc);
            callback(NULL, 0, "out of memory");
            return NULL;
        }
        subproc->argc++;
    }

    // Set up for posix_spawn
    posix_spawn_file_actions_init(&actions);
    if (output_callback != NULL) {
        posix_spawn_file_actions_adddup2(&actions, subproc->pipe_fds[1], STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, subproc->pipe_fds[0]);
        posix_spawn_file_actions_addclose(&actions, subproc->pipe_fds[1]);
    }
    posix_spawnattr_init(&attrs);
    extern char **environ;
    rv = posix_spawn(&subproc->pid, exepath, &actions, &attrs, subproc->argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attrs);
    if (rv != 0) {
        int err = rv < 0 ? errno : rv;
        ERROR("posix_spawn failed for %s: %s", subproc->argv[0], strerror(err));
        callback(subproc, 0, strerror(err));
        RELEASE_HERE(subproc, subproc);
        return NULL;
    }
    subproc->callback = callback;
    subproc->context = context;
    subproc->next = subprocesses;
    subprocesses = subproc;
    RETAIN_HERE(subproc, subproc);

    // Now that we have a viable subprocess, add the reader callback.
    if (output_callback != NULL && subproc->output_fd != NULL) {
        close(subproc->pipe_fds[1]);
        ioloop_add_reader(subproc->output_fd, output_callback);
    }
    return subproc;
}

void
ioloop_subproc_run_sync(subproc_t *subproc)
{
    int nev;
    RETAIN_HERE(subproc, subproc);
    do {
        nev = ioloop_events(0);
        INFO("%d events", nev);
        if (subproc->finished) {
            RELEASE_HERE(subproc, subproc);
            return;
        }
    } while (nev >= 0);
    ERROR("ioloop returned %d.", nev);
}

#ifndef EXCLUDE_DNSSD_TXN_SUPPORT
static void
dnssd_txn_callback(io_t *io, void *context)
{
    dnssd_txn_t *txn = (dnssd_txn_t *)context;
    // It's only safe to process the I/O if the DNSServiceRef hasn't been deallocated.
    if (txn->sdref != NULL) {
        int status = DNSServiceProcessResult(txn->sdref);
        if (status != kDNSServiceErr_NoError) {
            if (txn->failure_callback != NULL) {
                txn->failure_callback(txn->context, status);
            } else {
                INFO("status %d", status);
            }
            ioloop_dnssd_txn_cancel(txn);
        }
    }
}

void
dnssd_txn_finalize(dnssd_txn_t *txn)
{
    if (txn->sdref != NULL) {
        ioloop_dnssd_txn_cancel(txn);
    }
    if (txn->finalize_callback) {
        txn->finalize_callback(txn->context);
    }
}

void
dnssd_txn_io_finalize(void *context)
{
    dnssd_txn_t *txn = context;
    txn->io = NULL;
    RELEASE_HERE(txn, dnssd_txn);
}

void
ioloop_dnssd_txn_cancel(dnssd_txn_t *txn)
{
    if (txn->sdref != NULL) {
        DNSServiceRefDeallocate(txn->sdref);
        txn->sdref = NULL;
    } else {
        INFO("dead transaction.");
    }
    if (txn->io != NULL) {
        txn->io->fd = -1;
        RELEASE_HERE(txn->io, file_descriptor);
    }
}

void
ioloop_dnssd_txn_retain_(dnssd_txn_t *dnssd_txn, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(dnssd_txn, dnssd_txn);
}

void
ioloop_dnssd_txn_release_(dnssd_txn_t *dnssd_txn, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(dnssd_txn, dnssd_txn);
}

dnssd_txn_t *
ioloop_dnssd_txn_add_subordinate_(DNSServiceRef ref, void *context,
                                  dnssd_txn_finalize_callback_t callback, dnssd_txn_failure_callback_t failure_callback,
                                  const char *file, int line)
{
    dnssd_txn_t *txn = calloc(1, sizeof(*txn));
    if (txn != NULL) {
        RETAIN(txn, dnssd_txn);
        txn->sdref = ref;
        txn->finalize_callback = callback;
        txn->failure_callback = failure_callback;
        txn->context = context;
    }
    return txn;
}

dnssd_txn_t *
ioloop_dnssd_txn_add_(DNSServiceRef ref, void *context,
                      dnssd_txn_finalize_callback_t callback, dnssd_txn_failure_callback_t failure_callback,
                      const char *file, int line)
{
    dnssd_txn_t *txn = ioloop_dnssd_txn_add_subordinate_(ref, context, callback, failure_callback, file, line);
    if (txn != NULL) {
        txn->io = ioloop_file_descriptor_create(DNSServiceRefSockFD(txn->sdref), txn, dnssd_txn_io_finalize);
        if (txn->io == NULL) {
            RELEASE_HERE(txn, dnssd_txn);
            return NULL;
        }
        // io holds a reference to txn
        RETAIN_HERE(txn, dnssd_txn);
        ioloop_add_reader(txn->io, dnssd_txn_callback);
    }
    return txn;
}

void
ioloop_dnssd_txn_set_aux_pointer(dnssd_txn_t *NONNULL txn, void *aux_pointer)
{
    txn->aux_pointer = aux_pointer;
}

void *
ioloop_dnssd_txn_get_aux_pointer(dnssd_txn_t *NONNULL txn)
{
    return txn->aux_pointer;
}

void *
ioloop_dnssd_txn_get_context(dnssd_txn_t *NONNULL txn)
{
    return txn->context;
}
#endif // EXCLUDE_DNSSD_TXN_SUPPORT

static void
file_descriptor_finalize(void *context)
{
    io_t *file_descriptor = context;
    if (file_descriptor->finalize) {
        file_descriptor->finalize(file_descriptor->context);
    }
    if (file_descriptor->fd != -1) {
        close(file_descriptor->fd);
    }
    free(file_descriptor);
}

void
ioloop_file_descriptor_retain_(io_t *file_descriptor, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(file_descriptor, file_descriptor);
}

void
ioloop_file_descriptor_release_(io_t *file_descriptor, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(file_descriptor, file_descriptor);
}

io_t *
ioloop_file_descriptor_create_(int fd, void *context, finalize_callback_t finalize, const char *file, int line)
{
    io_t *ret;
    ret = calloc(1, sizeof(*ret));
    if (ret) {
        ret->fd = fd;
        ret->context = context;
        ret->finalize = finalize;
        ret->io_finalize = file_descriptor_finalize;
        RETAIN(ret, file_descriptor);
    }
    return ret;
}

void
ioloop_run_async(async_callback_t callback, void *context)
{
    async_event_t **epp, *event = calloc(1, sizeof(*event));
    if (event == NULL) {
        ERROR("no memory for async callback to %p, context %p", callback, context);
    }

    event->callback = callback;
    event->context = context;

    epp = &async_events;
    while (*epp) {
        epp = &(*epp)->next;
    }

    *epp = event;
}

const struct sockaddr *
connection_get_local_address(message_t *message)
{
    if (message == NULL) {
        ERROR("message is NULL.");
        return NULL;
    }
    return &message->local.sa;
}
#endif // !defined(IOLOOP_MACOS)

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
