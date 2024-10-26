/* macos-ioloop.c
 *
 * Copyright (c) 2018-2024 Apple Inc. All rights reserved.
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
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <dns_sd.h>

#include <CoreUtils/SystemUtils.h>  // For `IsAppleTV()`.
#include <dispatch/dispatch.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "tls-macos.h"
#include "tls-keychain.h"
#include "srp-dnssd.h"
#include "ifpermit.h"

dispatch_queue_t ioloop_main_queue;
static int cur_connection_serial;

// Forward references
static void ioloop_tcp_input_start(comm_t *NONNULL connection);
static void listener_finalize(comm_t *listener);
static bool connection_write_now(comm_t *NONNULL connection);
static bool ioloop_listener_connection_ready(comm_t *connection);

#define DSCP_CS5 0x28

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
ioloop_timenow(void)
{
    int64_t now;
    struct timeval tv;
    gettimeofday(&tv, 0);
    now = (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
    return now;
}

static void
wakeup_event(void *context)
{
    wakeup_t *wakeup = context;
    void *wakeup_context = wakeup->context;
    finalize_callback_t wakeup_finalize = wakeup->finalize;
    wakeup->context = NULL;
    wakeup->finalize = NULL;

    // All ioloop wakeups are one-shot.
    ioloop_cancel_wake_event(wakeup);

    // Call the callback, which mustn't be null.
    wakeup->wakeup(wakeup_context);

    // We have to call the finalize callback after the event has been delivered, in case we hold the only reference
    // on the object.
    if (wakeup_context != NULL && wakeup_finalize != NULL) {
        wakeup_finalize(wakeup_context);
    }
}

static void
wakeup_finalize(void *context)
{
    wakeup_t *wakeup = context;
    if (wakeup->ref_count == 0) {
        if (wakeup->dispatch_source != NULL) {
            dispatch_release(wakeup->dispatch_source);
            wakeup->dispatch_source = NULL;
        }
        void *wakeup_context = wakeup->context;
        finalize_callback_t wakeup_finalize = wakeup->finalize;
        wakeup->finalize = NULL;
        wakeup->context = NULL;
        if (wakeup_finalize != NULL && wakeup_context != NULL) {
            wakeup_finalize(wakeup_context);
        }
        free(wakeup);
    }
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
ioloop_add_wake_event(wakeup_t *wakeup, void *context, wakeup_callback_t callback, wakeup_callback_t finalize,
                      int32_t milliseconds)
{
    if (callback == NULL) {
        ERROR("ioloop_add_wake_event called with null callback");
        return false;
    }
    if (milliseconds < 0) {
        ERROR("ioloop_add_wake_event called with negative timeout");
        return false;
    }
    if (wakeup->dispatch_source != NULL) {
        ioloop_cancel_wake_event(wakeup);
    }
    wakeup->wakeup = callback;
    wakeup->context = context;
    wakeup->finalize = finalize;

    wakeup->dispatch_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, ioloop_main_queue);
    if (wakeup->dispatch_source == NULL) {
        ERROR("dispatch_source_create failed in ioloop_add_wake_event().");
        return false;
    }
    dispatch_source_set_event_handler_f(wakeup->dispatch_source, wakeup_event);
    dispatch_set_context(wakeup->dispatch_source, wakeup);

    // libdispatch doesn't allow events that are scheduled to happen right now. But it is actually useful to be
    // able to trigger an event to happen immediately, and this is the easiest way to do it from ioloop-we
    // can't rely on just scheduling an asynchronous event on an event loop because that's specific to Mac.
    if (milliseconds <= 0) {
        ERROR("ioloop_add_wake_event: milliseconds = %d", milliseconds);
        milliseconds = 10;
    }
    dispatch_source_set_timer(wakeup->dispatch_source,
                              dispatch_time(DISPATCH_TIME_NOW, milliseconds * NSEC_PER_SEC / 1000),
                              milliseconds * NSEC_PER_SEC / 1000, NSEC_PER_SEC / 100);
    dispatch_resume(wakeup->dispatch_source);

    return true;
}

void
ioloop_cancel_wake_event(wakeup_t *wakeup)
{
    if (wakeup != NULL) {
        if (wakeup->dispatch_source != NULL) {
            dispatch_source_cancel(wakeup->dispatch_source);
            dispatch_release(wakeup->dispatch_source);
            wakeup->dispatch_source = NULL;
        }
        if (wakeup->context != NULL) {
            void *wakeup_context = wakeup->context;
            finalize_callback_t wakeup_finalize = wakeup->finalize;
            wakeup->context = NULL;
            wakeup->finalize = NULL;
            if (wakeup_finalize != NULL && wakeup_context != NULL) {
                wakeup_finalize(wakeup_context);
            }
        }
    }
}

bool
ioloop_init(void)
{
    ioloop_main_queue = dispatch_get_main_queue();
    dispatch_retain(ioloop_main_queue);
    return true;
}

int
ioloop(void)
{
    dispatch_main();
    return 0;
}

#define connection_cancel(comm, conn) connection_cancel_(comm, conn, __FILE__, __LINE__)
static void
connection_cancel_(comm_t *comm, nw_connection_t connection, const char *file, int line)
{
    if (connection == NULL) {
        INFO("null connection at " PUB_S_SRP ":%d", file, line);
    } else {
        INFO("%p: " PUB_S_SRP " " PUB_S_SRP ":%d" , connection, comm->canceled ? " (already canceled)" : "", file, line);
        if (!comm->canceled) {
            nw_connection_cancel(connection);
            comm->canceled = true;
        }
    }
}

static void
comm_finalize(comm_t *comm)
{
    ERROR("comm_finalize");
    if (comm->connection != NULL) {
        nw_release(comm->connection);
        nw_connection_finalized++;
        comm->connection = NULL;
    }
    if (comm->listener != NULL) {
        nw_release(comm->listener);
        nw_listener_finalized++;
        comm->listener = NULL;
    }
    if (comm->parameters) {
        nw_release(comm->parameters);
        comm->parameters = NULL;
    }
    if (comm->pending_write != NULL) {
        dispatch_release(comm->pending_write);
        comm->pending_write = NULL;
    }

    if (comm->listener_state != NULL) {
        RELEASE_HERE(comm->listener_state, listener);
        comm->listener_state = NULL;
    }
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    if (comm->content_context != NULL) {
        nw_release(comm->content_context);
        comm->content_context = NULL;
    }
#endif

    // If there is an nw_connection_t or nw_listener_t outstanding, then we will get an asynchronous callback
    // later on.  So we can't actually free the data structure yet, but the good news is that comm_finalize() will
    // be called again later when the last outstanding asynchronous cancel is done, and then all of the stuff
    // that follows this will happen.
#ifndef __clang_analyzer__
    if (comm->ref_count > 0) {
        return;
    }
#endif
    if (comm->idle_timer != NULL) {
        ioloop_cancel_wake_event(comm->idle_timer);
        RELEASE_HERE(comm->idle_timer, wakeup);
    }
    if (comm->name != NULL) {
        free(comm->name);
    }
    if (comm->finalize != NULL) {
        comm->finalize(comm->context);
    }
    free(comm);
}

void
ioloop_comm_retain_(comm_t *comm, const char *file, int line)
{
    (void)file; (void)line;
    RETAIN(comm, comm);
}

void
ioloop_comm_release_(comm_t *comm, const char *file, int line)
{
    (void)file; (void)line;
    RELEASE(comm, comm);
}

void
ioloop_comm_cancel(comm_t *connection)
{
    if (connection->connection != NULL) {
        INFO("%p %p", connection, connection->connection);
        connection_cancel(connection, connection->connection);
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    } else if (connection->connection_group != NULL) {
        INFO("%p %p", connection, connection->connection_group);
        nw_connection_group_cancel(connection->connection_group);
#else
    }
    if (!connection->tcp_stream && connection->connection == NULL) {
        int fd = connection->io.fd;
        if (fd != -1) {
            ioloop_close(&connection->io);
            if (connection->cancel != NULL) {
                RETAIN_HERE(connection, listener);
                dispatch_async(ioloop_main_queue, ^{
                        if (connection->cancel != NULL) {
                            connection->cancel(connection, connection->context);
                        }
                        RELEASE_HERE(connection, listener);
                    });
            }
        }
#endif // UDP_LISTENER_USES_CONNECTION_GROUPS
    }
    if (connection->idle_timer != NULL) {
        ioloop_cancel_wake_event(connection->idle_timer);
    }
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

static bool
ioloop_send_message_inner(comm_t *connection, message_t *responding_to,
                          struct iovec *iov, int iov_len, bool final, bool send_length)
{
    dispatch_data_t data = NULL, new_data, combined;
    int i;
    uint16_t len = 0;

#ifdef SRP_TEST_SERVER
    if (connection->test_send_intercept != NULL) {
        return connection->test_send_intercept(connection, responding_to, iov, iov_len, final, send_length);
    }
#endif

    // Not needed on OSX because UDP conversations are treated as "connections."
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    (void)responding_to;
#else
    if (!connection->tcp_stream && connection->connection == NULL) {
        if (connection->io.fd != -1) {
            return ioloop_udp_send_message(connection, &responding_to->local, &responding_to->src, responding_to->ifindex, iov, iov_len);
        }
        return false;
    }
#endif

    if (connection->connection == NULL
#if UDP_LISTENER_USES_CONNECTION_GROUPS
        && connection->content_context == NULL
#endif
        ) {
        ERROR("no connection");
        return false;
    }

    // Create a dispatch_data_t object that contains the data in the iov.
    for (i = 0; i < iov_len; i++) {
        new_data = dispatch_data_create(iov[i].iov_base, iov[i].iov_len,
                                        ioloop_main_queue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        len += iov[i].iov_len;
        if (data != NULL) {
            if (new_data != NULL) {
                // Subsequent times through
                combined = dispatch_data_create_concat(data, new_data);
                dispatch_release(data);
                dispatch_release(new_data);
                data = combined;
            } else {
                // Fail
                dispatch_release(data);
                data = NULL;
            }
        } else {
            // First time through
            data = new_data;
        }
        if (data == NULL) {
            ERROR("ioloop_send_message: no memory.");
            return false;
        }
    }

    if (len == 0) {
        if (data) {
            dispatch_release(data);
        }
        ERROR("zero length");
        return false;
    }

    // TCP requires a length as well as the payload.
    if (send_length && connection->tcp_stream) {
        len = htons(len);
        new_data = dispatch_data_create(&len, sizeof (len), ioloop_main_queue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        if (new_data == NULL) {
            if (data != NULL) {
                dispatch_release(data);
            }
            ERROR("no memory for new_data");
            return false;
        }
        // Length is at beginning.
        combined = dispatch_data_create_concat(new_data, data);
        dispatch_release(data);
        dispatch_release(new_data);
        if (combined == NULL) {
            ERROR("no memory for combined");
            return false;
        }
        data = combined;
    }

    if (connection->pending_write != NULL) {
        ERROR("Dropping pending write on " PRI_S_SRP, connection->name ? connection->name : "<null>");
    }
    connection->pending_write = data;
    connection->final_data = final;
    if (connection->connection_ready) {
        return connection_write_now(connection);
    }
    return true;
}

bool
ioloop_send_message(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    return ioloop_send_message_inner(connection, responding_to, iov, iov_len, false, true);
}

bool
ioloop_send_final_message(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    return ioloop_send_message_inner(connection, responding_to, iov, iov_len, true, true);
}

bool
ioloop_send_data(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    return ioloop_send_message_inner(connection, responding_to, iov, iov_len, false, false);
}

bool
ioloop_send_final_data(comm_t *connection, message_t *responding_to, struct iovec *iov, int iov_len)
{
    return ioloop_send_message_inner(connection, responding_to, iov, iov_len, true, false);
}

#if UDP_LISTENER_USES_CONNECTION_GROUPS
// For UDP messages, the context is only going to be used for one reply, so when the reply is sent, call the
// disconnected callback.
static void
ioloop_disconnect_content_context(void *context)
{
    comm_t *connection = context;

    if (connection->disconnected != NULL) {
        connection->disconnected(connection, connection->context, 0);
    }
    RELEASE_HERE(connection, comm);
}
#endif // UDP_LISTENER_USES_CONNECTION_GROUPS

static bool
connection_write_now(comm_t *connection)
{
    if (false) {
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    } else if (connection->content_context != NULL) {
        nw_connection_group_reply(connection->listener_state->connection_group, connection->content_context,
                                  NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, connection->pending_write);
        if (connection->disconnected != NULL) {
            RETAIN_HERE(connection, comm);
            ioloop_run_async(ioloop_disconnect_content_context, connection);
        }
#endif
    } else {
        // Retain the connection once for each write that's pending, so that it's never finalized while
        // there's a write in progress.
        connection->writes_pending++;
        RETAIN_HERE(connection, comm);
        nw_connection_send(connection->connection, connection->pending_write,
                           (connection->final_data
                            ? NW_CONNECTION_FINAL_MESSAGE_CONTEXT
                            : NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT), true,
                           ^(nw_error_t  _Nullable error) {
                               if (error != NULL) {
                                   ERROR("ioloop_send_message: write failed: " PUB_S_SRP,
                                         strerror(nw_error_get_error_code(error)));
                                   connection_cancel(connection, connection->connection);
                               }
                               if (connection->writes_pending > 0) {
                                   connection->writes_pending--;
                               } else {
                                   ERROR("ioloop_send_message: write callback reached with no writes marked pending.");
                               }
                               RELEASE_HERE(connection, comm);
                           });
    }
    // nw_connection_send should retain this, so let go of our reference to it.
    dispatch_release(connection->pending_write);
    connection->pending_write = NULL;
    return true;
}

static bool
datagram_read(comm_t *connection, size_t length, dispatch_data_t content, nw_error_t error)
{
    message_t *message = NULL;
    bool ret = true, *retp = &ret;

    if (error != NULL) {
        ERROR(PUB_S_SRP, strerror(nw_error_get_error_code(error)));
        ret = false;
        goto out;
    }
    if (length > UINT16_MAX) {
        ERROR("oversized datagram length %zd", length);
        ret = false;
        goto out;
    }
    message = ioloop_message_create(length);
    if (message == NULL) {
        ERROR("unable to allocate message.");
        ret = false;
        goto out;
    }
    message->length = (uint16_t)length;
    dispatch_data_apply(content,
                        ^bool (dispatch_data_t __unused region, size_t offset, const void *buffer, size_t size) {
            if (message->length < offset + size) {
                ERROR("data region %zd:%zd is out of range for message length %d",
                      offset, size, message->length);
                *retp = false;
                return false;
            }
            memcpy(((uint8_t *)&message->wire) + offset, buffer, size);
            return true;
        });
    if (ret == true) {
        // Set the local address
        message->local = connection->local;

#ifdef HEXDUMP_INCOMING_DATAGRAMS
        uint16_t length = message->length > 8192 ? 8192 : message->length; // Don't dump really big messages
        for (uint16_t i = 0; i < length; i += 32) {
            char obuf[256];
            char *obp = obuf;
            int left = sizeof(obp) - 1;
            uint16_t max = message->length - i;
            if (max > 32) {
                max = 32;
            }
            for (uint16_t j = 0; j < max && left > 0; j += 8) {
                uint16_t submax = max - j;
                if (submax > 8) {
                    submax = 8;
                }
                for (uint16_t k = 0; k < submax; k++) {
                    snprintf(obp, left, "%02x", ((uint8_t *)&message->wire)[i + j + k]);
                    obp += 2;
                    *obp++ = ' ';
                    left -= 3;
                }
                *obp++ = ' ';
                left--;
            }
            *obp = 0;
            INFO("%03d " PUB_S_SRP, i, obuf);
        }
#endif
        // Process the message.
        if (connection->listener_state != NULL) {
            connection->listener_state->datagram_callback(connection, message, connection->listener_state->context);
        } else {
            connection->datagram_callback(connection, message, connection->context);
        }
    }

    out:
    if (message != NULL) {
        ioloop_message_release(message);
    }
    if (!ret && connection->connection != NULL) {
        connection_cancel(connection, connection->connection);
    }
    return ret;
}

static void
connection_error_to_string(nw_error_t error, char *errbuf, size_t errbuf_size)
{
    CFErrorRef cfe = NULL;
    CFStringRef errString = NULL;
    errbuf[0] = 0;
    if (error != NULL) {
        cfe = nw_error_copy_cf_error(error);
        if (cfe != NULL) {
            errString = CFErrorCopyDescription(cfe);
            if (errString != NULL) {
                CFStringGetCString(errString, errbuf, errbuf_size, kCFStringEncodingUTF8);
                CFRelease(errString);
            }
            CFRelease(cfe);
        }
    }
    if (errbuf[0] == 0) {
        memcpy(errbuf, "<NULL>", 7);
    }
}

static bool
check_fail(comm_t *connection, size_t length, dispatch_data_t content, nw_error_t error, const char *source)
{
    bool fail = false;
    INFO(PRI_S_SRP ": length %zd, content %p, content_length %ld, error %p, source %s",
         connection->name, length, content, content == NULL ? -1 : (long)dispatch_data_get_size(content), error, source);
    if (error != NULL) {
        fail = true;
    } else if (connection->connection == NULL) {
        fail = true;
    } else if (content == NULL) {
        ERROR("no content returned in " PUB_S_SRP ": connection must have dropped unexpectedly for " PRI_S_SRP,
              source, connection->name);
        fail = true;
    } else if (dispatch_data_get_size(content) != length) {
        ERROR("short content returned in " PUB_S_SRP ": %zd != %zd: connection must have dropped unexpectedly for " PRI_S_SRP,
              source, length, dispatch_data_get_size(content), connection->name);
        fail = true;
    }
    if (fail) {
        if (connection->connection != NULL) {
            connection_cancel(connection, connection->connection);
        }
    }
    return fail;
}

static void
tcp_read(comm_t *connection, size_t length, dispatch_data_t content, nw_error_t error)
{
    if (check_fail(connection, length, content, error, "tcp_read")) {
        return;
    }
    if (datagram_read(connection, length, content, error)) {
        // Wait for the next frame
        ioloop_tcp_input_start(connection);
    }
}

static void
tcp_read_length(comm_t *connection, dispatch_data_t content, nw_error_t error)
{
    size_t length;
    uint32_t bytes_to_read;
    const uint8_t *lenbuf;
    dispatch_data_t map;

    if (check_fail(connection, 2, content, error, "tcp_read_length")) {
        return;
    }

    map = dispatch_data_create_map(content, (const void **)&lenbuf, &length);
    if (map == NULL) {
        ERROR("tcp_read_length: map create failed");
        connection_cancel(connection, connection->connection);
        return;
    }
    dispatch_release(map);
    bytes_to_read = ((unsigned)(lenbuf[0]) << 8) | ((unsigned)lenbuf[1]);
    RETAIN_HERE(connection, comm);
    nw_connection_receive(connection->connection, bytes_to_read, bytes_to_read,
                          ^(dispatch_data_t new_content, nw_content_context_t __unused new_context,
                            bool __unused is_complete, nw_error_t new_error) {
                              if (new_error) {
                                  char errbuf[512];
                                  connection_error_to_string(new_error, errbuf, sizeof(errbuf));
                                  INFO("%p: " PUB_S_SRP, connection, errbuf);
                                  goto out;
                              }
                              tcp_read(connection, bytes_to_read, new_content, new_error);
                          out:
                              RELEASE_HERE(connection, comm);
                          });
}

static bool
ioloop_connection_input_badness_check(comm_t *connection, dispatch_data_t content, bool is_complete, nw_error_t error)
{
    if (error) {
        char errbuf[512];
        connection_error_to_string(error, errbuf, sizeof(errbuf));
        INFO("%p: " PUB_S_SRP, connection, errbuf);
        return true;
    }

    // For TCP connections, is_complete means the other end closed the connection.
    if (connection->tcp_stream && is_complete) {
        INFO("remote end closed connection.");
        connection_cancel(connection, connection->connection);
        return true;
    }

    if (content == NULL) {
        INFO("remote end closed connection.");
        connection_cancel(connection, connection->connection);
        return true;
    }
    return false;
}

static void
ioloop_tcp_input_start(comm_t *connection)
{
    if (connection->connection == NULL) {
        return;
    }

    RETAIN_HERE(connection, comm); // nw_connection_receive callback retains connection
    nw_connection_receive(connection->connection, 2, 2,
                          ^(dispatch_data_t content, nw_content_context_t __unused context,
                            bool is_complete, nw_error_t error) {
                              if (!ioloop_connection_input_badness_check(connection, content, is_complete, error)) {
                                  tcp_read_length(connection, content, error);
                              }
                              RELEASE_HERE(connection, comm);
                          });
}

static void
ioloop_udp_input_start(comm_t *connection)
{
    RETAIN_HERE(connection, comm); // nw_connection_receive callback retains connection
    nw_connection_receive_message(connection->connection,
                                  ^(dispatch_data_t content, nw_content_context_t __unused context,
                                    bool __unused is_complete, nw_error_t error) {
                                      if (!ioloop_connection_input_badness_check(connection, content, is_complete, error)) {
                                          if (datagram_read(connection, dispatch_data_get_size(content), content, error)) {
                                              ioloop_udp_input_start(connection);
                                          }
                                      }
                                      RELEASE_HERE(connection, comm);
                                  });
}

static void
ioloop_connection_state_changed(comm_t *connection, nw_connection_state_t state, nw_error_t error)
{
    char errbuf[512];
    connection_error_to_string(error, errbuf, sizeof(errbuf));

    if (state == nw_connection_state_ready) {
        if (connection->server) {
            if (!ioloop_listener_connection_ready(connection)) {
                ioloop_comm_cancel(connection);
                return;
            }
        }
        INFO(PRI_S_SRP " (%p %p) state is ready; error = " PUB_S_SRP,
             connection->name != NULL ? connection->name : "<no name>", connection, connection->connection, errbuf);
        // Set up a reader.
        if (connection->tcp_stream) {
            ioloop_tcp_input_start(connection);
        } else {
            ioloop_udp_input_start(connection);
        }
        connection->connection_ready = true;
        // If there's a write pending, send it now.
        if (connection->pending_write) {
            connection_write_now(connection);
        }
        if (connection->connected != NULL) {
            connection->connected(connection, connection->context);
        }
    } else if (state == nw_connection_state_failed || state == nw_connection_state_waiting) {
        // Waiting is equivalent to failed because we are not giving libnetcore enough information to
        // actually succeed when there is a problem connecting (e.g. "EHOSTUNREACH").
        INFO(PRI_S_SRP " (%p %p) state is " PUB_S_SRP "; error = " PUB_S_SRP,
             connection->name != NULL ? connection->name : "<no name>", connection, connection->connection,
             state == nw_connection_state_failed ? "failed" : "waiting", errbuf);
        connection_cancel(connection, connection->connection);
    } else if (state == nw_connection_state_cancelled) {
        INFO(PRI_S_SRP " (%p %p) state is canceled; error = " PUB_S_SRP,
             connection->name != NULL ? connection->name : "<no name>", connection, connection->connection, errbuf);
        if (connection->disconnected != NULL) {
            connection->disconnected(connection, connection->context, 0);
        }
        // This releases the final reference to the connection object, which was held by the nw_connection_t.
        RELEASE_HERE(connection, comm);
    } else {
        if (error != NULL) {
            // We can get here if e.g. the TLS handshake fails.
            connection_cancel(connection, connection->connection);
        }
        INFO(PRI_S_SRP " (%p %p) state is %d; error = " PUB_S_SRP,
             connection->name != NULL ? connection->name : "<no name>", connection, connection->connection, state, errbuf);
    }
}

static void
ioloop_connection_get_address_from_endpoint(addr_t *addr, nw_endpoint_t endpoint)
{
    nw_endpoint_type_t endpoint_type = nw_endpoint_get_type(endpoint);
    if (endpoint_type == nw_endpoint_type_address) {
        char *address_string = nw_endpoint_copy_address_string(endpoint);
        if (address_string == NULL) {
            ERROR("unable to get description of new connection.");
        } else {
            getipaddr(addr, address_string);
            if (addr->sa.sa_family == AF_INET6) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&addr->sin6.sin6_addr, rdata_buf);
                INFO("parsed connection local IPv6 address is: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&addr->sin6.sin6_addr, rdata_buf));
            } else {
                IPv4_ADDR_GEN_SRP(&addr->sin.sin_addr, rdata_buf);
                INFO("parsed connection local IPv4 address is: " PRI_IPv4_ADDR_SRP,
                     IPv4_ADDR_PARAM_SRP(&addr->sin.sin_addr, rdata_buf));
            }
        }
        free(address_string);
    }
}

static void
ioloop_connection_set_name_from_endpoint(comm_t *connection, nw_endpoint_t endpoint)
{
    nw_endpoint_type_t endpoint_type = nw_endpoint_get_type(endpoint);
    if (endpoint_type == nw_endpoint_type_address) {
        char *port_string = nw_endpoint_copy_port_string(endpoint);
        char *address_string = nw_endpoint_copy_address_string(endpoint);
        if (port_string == NULL || address_string == NULL) {
            ERROR("Unable to get description of new connection.");
        } else {
            const char *listener_name = connection->name == NULL ? "bogus" : connection->name;
            char *free_name = connection->name;
            connection->name = NULL;
            asprintf(&connection->name, "%s connection from %s/%s", listener_name, address_string, port_string);
            if (free_name != NULL) {
                free(free_name);
                free_name = NULL;
                listener_name = NULL;
            }
            getipaddr(&connection->address, address_string);
            if (connection->address.sa.sa_family == AF_INET6) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&connection->address.sin6.sin6_addr, rdata_buf);
                INFO("parsed connection remote IPv6 address is: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&connection->address.sin6.sin6_addr, rdata_buf));
            } else {
                IPv4_ADDR_GEN_SRP(&connection->address.sin.sin_addr, rdata_buf);
                INFO("parsed connection remote IPv4 address is: " PRI_IPv4_ADDR_SRP,
                     IPv4_ADDR_PARAM_SRP(&connection->address.sin.sin_addr, rdata_buf));
            }
        }
        free(port_string);
        free(address_string);
    } else {
        if (connection->name == NULL) {
            connection->name = nw_connection_copy_description(connection->connection);
        }
        ERROR("incoming connection " PRI_S_SRP " is of unexpected type %d", connection->name, endpoint_type);
    }
}

#if UDP_LISTENER_USES_CONNECTION_GROUPS
static void
ioloop_udp_receive(comm_t *listener, dispatch_data_t content, nw_content_context_t context, bool UNUSED is_complete)
{
    bool proceed = true;

    if (content != NULL) {
        comm_t *response_state = calloc(1, sizeof (*response_state));
        if (response_state == NULL) {
            ERROR("%p: " PRI_S_SRP ": no memory for response state.", listener, listener->name);
            return;
        }
        response_state->serial = ++cur_connection_serial;
        RETAIN_HERE(response_state, comm);
        response_state->listener_state = listener;
        RETAIN_HERE(response_state->listener_state, listener);
        response_state->datagram_callback = listener->datagram_callback;
        response_state->content_context = context;
        nw_retain(response_state->content_context);
        response_state->connection_ready = true;
        const char *identifier = nw_content_context_get_identifier(context);
        response_state->name = strdup(identifier);
        proceed = datagram_read(response_state, dispatch_data_get_size(content), content, NULL);
        RELEASE_HERE(response_state, comm);
    }
}
#else
#endif


static bool
ioloop_listener_connection_ready(comm_t *connection)
{

    nw_endpoint_t endpoint = nw_connection_copy_endpoint(connection->connection);
    if (endpoint != NULL) {
        ioloop_connection_set_name_from_endpoint(connection, endpoint);
        nw_release(endpoint);
    }
    if (connection->name != NULL) {
        INFO("Received connection from " PRI_S_SRP, connection->name);
    } else {
        ERROR("Unable to get description of new connection.");
        connection->name = strdup("unidentified");
    }

    // Best effort
    nw_endpoint_t local_endpoint = nw_connection_copy_connected_local_endpoint(connection->connection);
    if (local_endpoint != NULL) {
        ioloop_connection_get_address_from_endpoint(&connection->local, endpoint);
        nw_release(local_endpoint);
    }

    if (connection->connected != NULL) {
        connection->connected(connection, connection->context);
    }
    return true;
}

static void
ioloop_listener_connection_callback(comm_t *listener, nw_connection_t new_connection)
{
    nw_connection_set_queue(new_connection, ioloop_main_queue);
    nw_connection_start(new_connection);

    comm_t *connection = calloc(1, sizeof *connection);
    if (connection == NULL) {
        ERROR("Unable to receive connection: no memory.");
        nw_connection_cancel(new_connection);
        return;
    }
    connection->serial = ++cur_connection_serial;

    connection->connection = new_connection;
    nw_retain(connection->connection);
    nw_connection_created++;

    connection->name = strdup(listener->name);
    connection->datagram_callback = listener->datagram_callback;
    connection->tcp_stream = listener->tcp_stream;
    connection->server = true;
    connection->context = listener->context;
    connection->connected = listener->connected;
    RETAIN_HERE(connection, comm); // The connection state changed handler has a reference to the connection.
    nw_connection_set_state_changed_handler(connection->connection,
                                            ^(nw_connection_state_t state, nw_error_t error)
                                            { ioloop_connection_state_changed(connection, state, error); });
    INFO("started " PRI_S_SRP, connection->name);
}

static void
listener_finalize(comm_t *listener)
{
    if (listener->listener != NULL) {
        nw_release(listener->listener);
        nw_listener_finalized++;
        listener->listener = NULL;
    }
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    if (listener->connection_group) {
        nw_release(listener->connection_group);
        listener->connection_group = NULL;
    }
#endif
    if (listener->name != NULL) {
        free(listener->name);
    }
    if (listener->parameters) {
        nw_release(listener->parameters);
    }
    if (listener->avoid_ports != NULL) {
        free(listener->avoid_ports);
    }
    if (listener->finalize) {
        listener->finalize(listener->context);
    }
    free(listener);
}

void
ioloop_listener_retain_(comm_t *listener, const char *file, int line)
{
    RETAIN(listener, listener);
}

void
ioloop_listener_release_(comm_t *listener, const char *file, int line)
{
    RELEASE(listener, listener);
}

static void ioloop_listener_context_release(void *context)
{
    comm_t *listener = context;
    RELEASE_HERE(listener, listener);
}

void
ioloop_listener_cancel(comm_t *connection)
{
    // Only need to do it once.
    if (connection->canceled) {
        FAULT("cancel on canceled connection " PRI_S_SRP, connection->name);
        return;
    }
    connection->canceled = true;
    if (connection->listener != NULL) {
        nw_listener_cancel(connection->listener);
        // connection->listener will be released in ioloop_listener_state_changed_handler: nw_listener_state_cancelled.
    }
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    if (connection->connection_group != NULL) {
        INFO("%p %p", connection, connection->connection_group);
        nw_connection_group_cancel(connection->connection_group);
    }
#else
    if (!connection->tcp_stream && connection->connection == NULL) {
        int fd = connection->io.fd;
        if (fd != -1) {
            ioloop_close(&connection->io);
            if (connection->cancel != NULL) {
                RETAIN_HERE(connection, listener);
                dispatch_async(ioloop_main_queue, ^{
                        if (connection->cancel != NULL) {
                            connection->cancel(connection, connection->context);
                        }
                        RELEASE_HERE(connection, listener);
                    });
            }
        }
    }
#endif
}

#if UDP_LISTENER_USES_CONNECTION_GROUPS
static bool ioloop_udp_listener_setup(comm_t *listener);

static void
ioloop_udp_listener_state_changed_handler(comm_t *listener, nw_connection_group_state_t state, nw_error_t error)
{
    int i;

#ifdef DEBUG_VERBOSE
    if (listener->connection_group == NULL) {
        if (state == nw_listener_state_cancelled) {
            INFO("nw_connection_group gets released before the final nw_connection_group_state_cancelled event - name: " PRI_S_SRP,
                 listener->name);
        } else {
            ERROR("nw_connection_group gets released before the connection_group is canceled - name: " PRI_S_SRP ", state: %d",
                  listener->name, state);
        }
    }
#endif // DEBUG_VERBOSE

    // Should never happen.
    if (listener->connection_group == NULL && state != nw_connection_group_state_cancelled) {
        return;
    }

    if (error != NULL) {
        char errbuf[512];
        connection_error_to_string(error, errbuf, sizeof(errbuf));
        INFO("state changed: " PUB_S_SRP, errbuf);
        if (listener->connection_group != NULL) {
            nw_connection_group_cancel(listener->connection_group);
        }
    } else {
        if (state == nw_connection_group_state_waiting) {
            INFO("waiting");
            return;
        } else if (state == nw_connection_group_state_failed) {
            INFO("failed");
            nw_connection_group_cancel(listener->connection_group);
        } else if (state == nw_connection_group_state_ready) {
            // It's possible that we might schedule the ready event but then before we return to the run loop
            // the listener gets canceled, in which case we don't want to deliver the ready event.
            if (listener->canceled) {
                INFO("ready but canceled");
                return;
            }
            INFO("ready");
            if (listener->avoiding) {
                listener->listen_port = nw_connection_group_get_port(listener->connection_group);
                if (listener->avoid_ports != NULL) {
                    for (i = 0; i < listener->num_avoid_ports; i++) {
                        if (listener->avoid_ports[i] == listener->listen_port) {
                            INFO("Got port %d, which we are avoiding.",
                                 listener->listen_port);
                            listener->avoiding = true;
                            listener->listen_port = 0;
                            nw_connection_group_cancel(listener->connection_group);
                            return;
                        }
                    }
                }
                INFO("Got port %d.", listener->listen_port);
                listener->avoiding = false;
                if (listener->ready) {
                    listener->ready(listener->context, listener->listen_port);
                }
            }
        } else if (state == nw_connection_group_state_cancelled) {
            INFO("cancelled");
            nw_release(listener->connection_group);
            nw_listener_finalized++;
            listener->connection_group = NULL;
            if (listener->avoiding) {
                if (!ioloop_udp_listener_setup(listener)) {
                    ERROR("ioloop_listener_state_changed_handler: Unable to recreate listener.");
                    goto cancel;
                } else {
                    nw_listener_created++;
                }
            } else {
                ;
            cancel:
                if (listener->cancel) {
                    listener->cancel(listener, listener->context);
                }
                RELEASE_HERE(listener, listener);
            }
        }
    }
}
#endif // UDP_LISTENER_USES_CONNECTION_GROUPS

static void
ioloop_listener_state_changed_handler(comm_t *listener, nw_listener_state_t state, nw_error_t error)
{
#ifdef DEBUG_VERBOSE
    if (listener->listener == NULL) {
        if (state == nw_listener_state_cancelled) {
            INFO("nw_listener gets released before the final nw_listener_state_cancelled event - name: " PRI_S_SRP,
                 listener->name);
        } else {
            ERROR("nw_listener gets released before the listener is canceled - name: " PRI_S_SRP ", state: %d",
                  listener->name, state);
        }
    }
#endif // DEBUG_VERBOSE

    INFO("%p %p " PUB_S_SRP " %d", listener, listener->listener, listener->name, state);

    // Should never happen.
    if (listener->listener == NULL && state != nw_listener_state_cancelled) {
        return;
    }

    if (error != NULL) {
        char errbuf[512];
        connection_error_to_string(error, errbuf, sizeof(errbuf));
        INFO("state changed: " PUB_S_SRP, errbuf);
        if (listener->listener != NULL) {
            nw_listener_cancel(listener->listener);
        }
    } else {
        if (state == nw_listener_state_waiting) {
            INFO("waiting");
            return;
        } else if (state == nw_listener_state_failed) {
            INFO("failed");
            nw_listener_cancel(listener->listener);
        } else if (state == nw_listener_state_ready) {
            INFO("ready");
            if (listener->ready != NULL) {
                listener->ready(listener->context, listener->listen_port);
            }
        } else if (state == nw_listener_state_cancelled) {
            INFO("cancelled");
            nw_release(listener->listener);
            nw_listener_finalized++;
            listener->listener = NULL;
            if (listener->cancel != NULL) {
                listener->cancel(listener, listener->context);
            }
            RELEASE_HERE(listener, listener); // Release the nw_listener handler function's reference to the ioloop listener object.
        } else {
            INFO("something else");
        }
    }
}

#if UDP_LISTENER_USES_CONNECTION_GROUPS
static bool
ioloop_udp_listener_setup(comm_t *listener)
{
    listener->connection_group = nw_connection_group_create_with_parameters(listener->parameters);
    if (listener->connection_group == NULL) {
        return false;
    }
    nw_connection_group_set_state_changed_handler(listener->connection_group,
                                                  ^(nw_connection_group_state_t state, nw_error_t error) {
            ioloop_udp_listener_state_changed_handler(listener, state, error);
        });
    nw_connection_group_set_receive_handler(listener->connection_group, DNS_MAX_UDP_PAYLOAD, true,
                                            ^(dispatch_data_t  _Nullable content,
                                              nw_content_context_t  _Nonnull receive_context, bool is_complete) {
                                                ioloop_udp_receive(listener, content, receive_context, is_complete);
                                            });
    RETAIN_HERE(listener, listener); // For the handlers.

    // Start the connection group listener
    nw_connection_group_set_queue(listener->connection_group, ioloop_main_queue);
    nw_connection_group_start(listener->connection_group);
    return true;
}
#else
static comm_t *
ioloop_udp_listener_setup(comm_t *listener, const addr_t *ip_address, uint16_t port, const char *launchd_name, int ifindex)
{
    sa_family_t family = (ip_address != NULL) ? ip_address->sa.sa_family : AF_UNSPEC;
    sa_family_t real_family = family == AF_UNSPEC ? AF_INET6 : family;
    int true_flag = 1;
    addr_t sockname;
    socklen_t sl;
    int rv;

    listener->address.sa.sa_family = real_family;
    listener->address.sa.sa_len = (real_family == AF_INET
                                   ? sizeof(listener->address.sin)
                                   : sizeof(listener->address.sin6));
    if (real_family == AF_INET6) {
        listener->address.sin6.sin6_port = htons(port);
    } else {
        listener->address.sin.sin_port = htons(port);
    }

    listener->io.fd = -1;
#ifndef SRP_TEST_SERVER
    if (launchd_name != NULL) {
        int *fds;
        size_t cnt;
        int ret = launch_activate_socket(launchd_name, &fds, &cnt);
        if (ret != 0) {
            FAULT("launchd_activate_socket failed for " PUB_S_SRP ": " PUB_S_SRP, launchd_name, strerror(ret));
            listener->io.fd = -1;
        } else if (cnt == 0) {
            FAULT("too few sockets returned from launchd_active_socket for " PUB_S_SRP" : %zd", launchd_name, cnt);
            listener->io.fd = -1;
        } else if (cnt != 1) {
            FAULT("too many sockets returned from launchd_active_socket for " PUB_S_SRP" : %zd", launchd_name, cnt);
            for (size_t i = 0; i < cnt; i++) {
                close(fds[i]);
            }
            free(fds);
        } else {
            listener->io.fd = fds[0];
            free(fds);
        }
    }
#endif
    if (listener->io.fd == -1) {
        listener->io.fd = socket(real_family, SOCK_DGRAM, IPPROTO_UDP);
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

        // shift the DSCP value to the left by 2 bits to make the 8-bit field
        int dscp = DSCP_CS5 << 2;
        if (real_family == AF_INET6) {
            // IPV6_TCLASS.
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_TCLASS, &dscp, sizeof(dscp));
            if (rv < 0) {
                ERROR("IPV6_TCLASS failed: %s", strerror(errno));
                goto out;
            }
        } else {
            // IP_TOS
            rv = setsockopt(listener->io.fd, IPPROTO_IP, IP_TOS, &dscp, sizeof(dscp));
            if (rv < 0) {
                ERROR("IP_TOS failed: %s", strerror(errno));
                goto out;
            }
        }
        // skipping multicast support for now

        if (family == AF_INET6) {
            // Don't use a dual-stack socket.
            rv = setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_V6ONLY, &true_flag, sizeof true_flag);
            if (rv < 0) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
                ERROR("Unable to set IPv6-only flag on UDP socket for " PRI_SEGMENTED_IPv6_ADDR_SRP,
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf));
                goto out;
            }
            SEGMENTED_IPv6_ADDR_GEN_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
            ERROR("Successfully set IPv6-only flag on UDP socket for " PRI_SEGMENTED_IPv6_ADDR_SRP,
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf));
        }

        sl = listener->address.sa.sa_len;
        if (bind(listener->io.fd, &listener->address.sa, sl) < 0) {
            if (family == AF_INET) {
                IPv4_ADDR_GEN_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf);
                ERROR("Can't bind to " PRI_IPv4_ADDR_SRP "#%d: %s",
                      IPv4_ADDR_PARAM_SRP(&listener->address.sin.sin_addr.s_addr, ipv4_addr_buf), ntohs(port),
                      strerror(errno));
            } else {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf);
                ERROR("Can't bind to " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d: %s",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(&listener->address.sin6.sin6_addr.s6_addr, ipv6_addr_buf), ntohs(port),
                      strerror(errno));
            }
        out:
            close(listener->io.fd);
            listener->io.fd = -1;
            RELEASE_HERE(listener, listener);
            return NULL;
        }
    }

    if (fcntl(listener->io.fd, F_SETFL, O_NONBLOCK) < 0) {
        ERROR("%s: Can't set O_NONBLOCK: %s", listener->name, strerror(errno));
        goto out;
    }

    // We may have bound to an unspecified port, so fetch the port we got. Or we may have got the port from
    // launchd, in which case let's make sure we got the right port.
    if (launchd_name != NULL || port == 0) {
        sl = sizeof(sockname);
        if (getsockname(listener->io.fd, (struct sockaddr *)&sockname, &sl) < 0) {
            ERROR("getsockname: %s", strerror(errno));
            goto out;
        }
        listener->listen_port = ntohs(real_family == AF_INET6 ? sockname.sin6.sin6_port : sockname.sin.sin_port);
        if (launchd_name != NULL && listener->listen_port != port) {
            ERROR("launchd port mismatch: %u %u", port, listener->listen_port);
        }
    } else {
        listener->listen_port = port;
    }
    INFO("port is %d", listener->listen_port);

    if (ifindex != 0) {
        setsockopt(listener->io.fd, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
        setsockopt(listener->io.fd, IPPROTO_IPV6, IPV6_BOUND_IF, &ifindex, sizeof(ifindex));
    }
    rv = setsockopt(listener->io.fd, family == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                    family == AF_INET ? IP_PKTINFO : IPV6_RECVPKTINFO, &true_flag, sizeof true_flag);
    if (rv < 0) {
        ERROR("Can't set %s: %s.", family == AF_INET ? "IP_PKTINFO" : "IPV6_RECVPKTINFO",
              strerror(errno));
        goto out;
    }
    ioloop_add_reader(&listener->io, ioloop_udp_read_callback);
    RETAIN_HERE(listener, listener); // For the reader
    listener->io.context = listener;
    listener->io.is_static = true;
    listener->io.context_release = ioloop_listener_context_release;

    // If there's a ready callback, call it.
    if (listener->ready != NULL) {
        RETAIN_HERE(listener, listener); // For the ready callback
        dispatch_async(ioloop_main_queue, ^{
                // It's possible that we might schedule the ready event but then before we return to the run loop
                // the listener gets canceled, in which case we don't want to deliver the ready event.
                if (listener->canceled) {
                    INFO("ready but canceled");
                } else {
                    if (listener->ready != NULL) {
                        listener->ready(listener->context, listener->listen_port);
                    }
                }
                RELEASE_HERE(listener, listener);
            });
    }
    return listener;
}
#endif // UDP_LISTENER_USES_CONNECTION_GROUPS

comm_t *
ioloop_listener_create(bool stream, bool tls, bool launchd, uint16_t *avoid_ports, int num_avoid_ports,
                       const addr_t *ip_address, const char *multicast, const char *name,
                       datagram_callback_t datagram_callback, connect_callback_t connected, cancel_callback_t cancel,
                       ready_callback_t ready, finalize_callback_t finalize, tls_config_callback_t tls_config,
                       unsigned ifindex, void *context)
{
    comm_t *listener;
    int family = (ip_address != NULL) ? ip_address->sa.sa_family : AF_UNSPEC;
    uint16_t port;
    char portbuf[10];
    nw_endpoint_t endpoint;

    if (ip_address == NULL) {
        port = 0;
    } else {
        port = (family == AF_INET) ? ntohs(ip_address->sin.sin_port) : ntohs(ip_address->sin6.sin6_port);
    }

    if (multicast != NULL) {
        ERROR("ioloop_setup_listener: multicast not supported.");
        return NULL;
    }

    if (datagram_callback == NULL) {
        ERROR("ioloop_setup: no datagram callback provided.");
        return NULL;
    }

    snprintf(portbuf, sizeof(portbuf), "%d", port);
    listener = calloc(1, sizeof(*listener));
    if (listener == NULL) {
        if (ip_address == NULL) {
            ERROR("No memory for listener on <NULL>#%d", port);
        } else if (family == AF_INET) {
            IPv4_ADDR_GEN_SRP(&ip_address->sin.sin_addr.s_addr, ipv4_addr_buf);
            ERROR("No memory for listener on " PRI_IPv4_ADDR_SRP "#%d",
                  IPv4_ADDR_PARAM_SRP(&ip_address->sin.sin_addr.s_addr, ipv4_addr_buf), port);
        } else if (family == AF_INET6) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(ip_address->sin6.sin6_addr.s6_addr, ipv6_addr_buf);
            ERROR("No memory for listener on " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d",
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(ip_address->sin6.sin6_addr.s6_addr, ipv6_addr_buf), port);
        } else {
            ERROR("No memory for listener on <family address other than AF_INET or AF_INET6: %d>#%d", family, port);
        }
        return NULL;
    }
    listener->serial = ++cur_connection_serial;
    if (avoid_ports != NULL) {
        listener->avoid_ports = malloc(num_avoid_ports * sizeof(uint16_t));
        if (listener->avoid_ports == NULL) {
            if (ip_address == NULL) {
                ERROR("No memory for listener avoid_ports on <NULL>#%d", port);
            } else if (family == AF_INET) {
                IPv4_ADDR_GEN_SRP(&ip_address->sin.sin_addr.s_addr, ipv4_addr_buf);
                ERROR("No memory for listener avoid_ports on " PRI_IPv4_ADDR_SRP "#%d",
                      IPv4_ADDR_PARAM_SRP(&ip_address->sin.sin_addr.s_addr, ipv4_addr_buf), port);
            } else if (family == AF_INET6) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(ip_address->sin6.sin6_addr.s6_addr, ipv6_addr_buf);
                ERROR("No memory for listener avoid_ports on " PRI_SEGMENTED_IPv6_ADDR_SRP "#%d",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(ip_address->sin6.sin6_addr.s6_addr, ipv6_addr_buf), port);
            } else {
                ERROR("No memory for listener avoid_ports on <family address other than AF_INET or AF_INET6: %d>#%d",
                      family, port);
            }
            free(listener);
            return NULL;
        }
        listener->num_avoid_ports = num_avoid_ports;
        listener->avoiding = true;
    }
    RETAIN_HERE(listener, listener);
    listener->name = strdup(name);
    if (listener->name == NULL) {
        ERROR("no memory for listener name.");
        RELEASE_HERE(listener, listener);
        return NULL;
    }
    listener->ready = ready;
    listener->context = context;
    listener->tcp_stream = stream;
    listener->is_listener = true;

#if !UDP_LISTENER_USES_CONNECTION_GROUPS
    if (stream == FALSE) {
        comm_t *ret = ioloop_udp_listener_setup(listener, ip_address, port, launchd ? name : NULL, ifindex);
        if (ret == NULL) {
            return ret;
        }
    }
#endif

    listener->datagram_callback = datagram_callback;
    listener->cancel = cancel;
    listener->finalize = finalize;
    listener->connected = connected;

#if !UDP_LISTENER_USES_CONNECTION_GROUPS
    if (stream == FALSE) {
        return listener;
    }
#endif
    if (port == 0) {
        endpoint = NULL;
        // Even though we don't have any ports to avoid, we still want the "avoiding" behavior in this case, since that
        // is what triggers a call to the ready handler, which passes the port number that we got to it.
        listener->avoiding = true;
    } else {
        listener->listen_port = port;
        char ip_address_str[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
        if (ip_address == NULL || family == AF_UNSPEC) {
            if (family == AF_INET) {
                snprintf(ip_address_str, sizeof(ip_address_str), "0.0.0.0");
            } else {
                // AF_INET6 or AF_UNSPEC
                snprintf(ip_address_str, sizeof(ip_address_str), "::");
            }
        } else {
            if (family == AF_INET) {
                inet_ntop(family, &ip_address->sin.sin_addr, ip_address_str, sizeof(ip_address_str));
            } else {
                inet_ntop(family, &ip_address->sin6.sin6_addr, ip_address_str, sizeof(ip_address_str));
            }
        }
        endpoint = nw_endpoint_create_host(ip_address_str, portbuf);
        if (endpoint == NULL) {
            ERROR("No memory for listener endpoint.");
            RELEASE_HERE(listener, listener);
            return NULL;
        }
    }
    if (stream) {
        nw_parameters_configure_protocol_block_t configure_tls_block = NW_PARAMETERS_DISABLE_PROTOCOL;
        if (tls && tls_config != NULL) {
            configure_tls_block = ^(nw_protocol_options_t tls_options) {
                tls_config_context_t tls_context = {tls_options, ioloop_main_queue};
                tls_config((void *)&tls_context);
            };
        }

        listener->parameters = nw_parameters_create_secure_tcp(configure_tls_block, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    } else {
        if (tls) {
            ERROR("DTLS support not implemented.");
            nw_release(endpoint);
            RELEASE_HERE(listener, listener);
            return NULL;
        }
#if UDP_LISTENER_USES_CONNECTION_GROUPS
        listener->parameters = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL,
                                                               NW_PARAMETERS_DEFAULT_CONFIGURATION);
#endif
    }
    if (listener->parameters == NULL) {
        ERROR("No memory for listener parameters.");
        nw_release(endpoint);
        RELEASE_HERE(listener, listener);
        return NULL;
    }

    if (endpoint != NULL) {
        nw_parameters_set_local_endpoint(listener->parameters, endpoint);
        nw_release(endpoint);
    }

    // Set SO_REUSEADDR.
    nw_parameters_set_reuse_local_address(listener->parameters, true);


    if (stream) {
        // Create the nw_listener_t.
        listener->listener = NULL;
#ifndef SRP_TEST_SERVER
        if (launchd && name != NULL) {
            listener->listener = nw_listener_create_with_launchd_key(listener->parameters, name);
            if (listener->listener == NULL) {
                ERROR("launchd listener create failed, trying to create it without relying on launchd.");
            }
        }
#endif
        if (listener->listener == NULL) {
            listener->listener = nw_listener_create(listener->parameters);
        }
        if (listener->listener == NULL) {
            ERROR("no memory for nw_listener object");
            RELEASE_HERE(listener, listener);
            return NULL;
        }
        nw_listener_created++;
        nw_listener_set_new_connection_handler(listener->listener,
                                               ^(nw_connection_t connection) {
                                                   ioloop_listener_connection_callback(listener, connection);
                                               });

        nw_listener_set_state_changed_handler(listener->listener, ^(nw_listener_state_t state, nw_error_t error) {
            ioloop_listener_state_changed_handler(listener, state, error);
        });
        RETAIN_HERE(listener, listener); // for the nw_listener_t state change handler callback
        nw_listener_set_queue(listener->listener, ioloop_main_queue);
        nw_listener_start(listener->listener);
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    } else {
        if (launchd) {
            FAULT("launchd not yet supported for connection groups");
            return NULL;
        }
        if (!ioloop_udp_listener_setup(listener)) {
            RELEASE_HERE(listener, listener);
            return NULL;
        }
#endif // UDP_LISTENER_USES_CONNECTION_GROUPS
    }

    // Listener has one refcount
    return listener;
}

comm_t *
ioloop_connection_create(addr_t *NONNULL remote_address, bool tls, bool stream, bool stable, bool opportunistic,
                         datagram_callback_t datagram_callback, connect_callback_t connected,
                         disconnect_callback_t disconnected, finalize_callback_t finalize, void *context)
{
    comm_t *connection;
    char portbuf[10];
    nw_parameters_t parameters;
    nw_endpoint_t endpoint;
    char addrbuf[INET6_ADDRSTRLEN];

    inet_ntop(remote_address->sa.sa_family, (remote_address->sa.sa_family == AF_INET
                                             ? (void *)&remote_address->sin.sin_addr
                                             : (void *)&remote_address->sin6.sin6_addr), addrbuf, sizeof addrbuf);
    snprintf(portbuf, sizeof(portbuf), "%d", (remote_address->sa.sa_family == AF_INET
                            ? ntohs(remote_address->sin.sin_port)
                            : ntohs(remote_address->sin6.sin6_port)));
    connection = calloc(1, sizeof(*connection));
    if (connection == NULL) {
        ERROR("No memory for connection");
        return NULL;
    }
    connection->serial = ++cur_connection_serial;

    // If we don't release this because of an error, this is the caller's reference to the comm_t.
    RETAIN_HERE(connection, comm);
    endpoint = nw_endpoint_create_host(addrbuf, portbuf);
    if (endpoint == NULL) {
        ERROR("No memory for connection endpoint.");
        RELEASE_HERE(connection, comm);
        return NULL;
    }

    if (stream) {
        nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;
        if (tls) {
            // This sets up a block that's called when we get a TLS connection and want to verify
            // the cert.   Right now we only support opportunistic security, which means we have
            // no way to validate the cert.   Future work: add support for validating the cert
            // using a TLSA record if one is present.
            configure_tls = ^(nw_protocol_options_t tls_options) {
                sec_protocol_options_t sec_options = nw_tls_copy_sec_protocol_options(tls_options);
                sec_protocol_options_set_verify_block(sec_options,
                                                      ^(sec_protocol_metadata_t metadata, sec_trust_t trust_ref,
                                                        sec_protocol_verify_complete_t complete) {
                                                          (void) metadata;
                                                          (void) trust_ref;
                                                          const bool valid = true;
                                                          complete(valid);
                                                      }, ioloop_main_queue);
                nw_release(sec_options);
            };
        }

        parameters = nw_parameters_create_secure_tcp(configure_tls, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    } else {
        if (tls) {
            ERROR("DTLS support not implemented.");
            nw_release(endpoint);
            RELEASE_HERE(connection, comm);
            return NULL;
        }
        parameters = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL,
                                                     NW_PARAMETERS_DEFAULT_CONFIGURATION);
    }
    if (parameters == NULL) {
        ERROR("No memory for connection parameters.");
        nw_release(endpoint);
        RELEASE_HERE(connection, comm);
        return NULL;
    }

    nw_protocol_stack_t protocol_stack = nw_parameters_copy_default_protocol_stack(parameters);

    // If user asked for a stable address, set that option.
    if (stable) {
        nw_protocol_options_t ip_options = nw_protocol_stack_copy_internet_protocol(protocol_stack);
        nw_ip_options_set_local_address_preference(ip_options, nw_ip_local_address_preference_stable);
        nw_release(ip_options);
    }

    // Only set TCP options for TCP connections.
    if (stream) {
        nw_protocol_options_t tcp_options = nw_protocol_stack_copy_transport_protocol(protocol_stack);
        nw_tcp_options_set_no_delay(tcp_options, true);
        nw_tcp_options_set_enable_keepalive(tcp_options, true);
        nw_release(tcp_options);
    }
    nw_release(protocol_stack);

    connection->name = strdup(addrbuf);

    // Create the nw_connection_t.
    connection->connection = nw_connection_create(endpoint, parameters);
    nw_connection_created++;
    nw_release(endpoint);
    nw_release(parameters);
    if (connection->connection == NULL) {
        ERROR("no memory for nw_connection object");
        RELEASE_HERE(connection, comm);
        return NULL;
    }

    connection->datagram_callback = datagram_callback;
    connection->connected = connected;
    connection->disconnected = disconnected;
    connection->finalize = finalize;
    connection->tcp_stream = stream;
    connection->opportunistic = opportunistic;
    connection->context = context;
    RETAIN_HERE(connection, comm); // The connection state changed handler has a reference to the connection.
    nw_connection_set_state_changed_handler(connection->connection,
                                            ^(nw_connection_state_t state, nw_error_t error)
                                            { ioloop_connection_state_changed(connection, state, error); });
    nw_connection_set_queue(connection->connection, ioloop_main_queue);
    nw_connection_start(connection->connection);
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
    if (subproc->dispatch_source != NULL) {
        dispatch_release(subproc->dispatch_source);
    }
    if (subproc->output_fd != NULL) {
        ioloop_file_descriptor_release(subproc->output_fd);
    }
    if (subproc->finalize != NULL) {
        subproc->finalize(subproc->context);
    }
    free(subproc);
}

static void subproc_cancel(void *context)
{
    subproc_t *subproc = context;
    subproc->dispatch_source = NULL;
    RELEASE_HERE(subproc, subproc);
}

static void
subproc_event(void *context)
{
    subproc_t *subproc = context;
    pid_t pid;
    int status;

    pid = waitpid(subproc->pid, &status, WNOHANG);
    if (pid <= 0) {
        return;
    }
    subproc->callback(subproc, status, NULL);
    if (!WIFSTOPPED(status)) {
        dispatch_source_cancel(subproc->dispatch_source);
    }
}

static void
subproc_output_finalize(void *context)
{
    subproc_t *subproc = context;
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
ioloop_subproc(const char *exepath, char *NULLABLE *argv, int argc,
               subproc_callback_t callback, io_callback_t output_callback, void *context)
{
    subproc_t *subproc;
    int i, rv;
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attrs;

    if (callback == NULL) {
        ERROR("ioloop_add_wake_event called with null callback");
        return NULL;
    }

    if (argc > MAX_SUBPROC_ARGS) {
        callback(NULL, 0, "too many subproc args");
        return NULL;
    }

    subproc = calloc(1, sizeof *subproc);
    if (subproc == NULL) {
        callback(NULL, 0, "out of memory");
        return NULL;
    }
    RETAIN_HERE(subproc, subproc); // For the create rule
    if (output_callback != NULL) {
        rv = pipe(subproc->pipe_fds);
        if (rv < 0) {
            callback(NULL, 0, "unable to create pipe.");
            RELEASE_HERE(subproc, subproc);
            return NULL;
        }
        subproc->output_fd = ioloop_file_descriptor_create(subproc->pipe_fds[0], subproc, subproc_output_finalize);
        RETAIN_HERE(subproc, subproc); // For the file descriptor
        if (subproc->output_fd == NULL) {
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
    if (rv < 0) {
        ERROR("posix_spawn failed for " PUB_S_SRP ": " PUB_S_SRP, subproc->argv[0], strerror(errno));
        callback(subproc, 0, strerror(errno));
        RELEASE_HERE(subproc, subproc);
        return NULL;
    }
    subproc->callback = callback;
    subproc->context = context;

    subproc->dispatch_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, subproc->pid, DISPATCH_PROC_EXIT,
                                                      ioloop_main_queue);
    if (subproc->dispatch_source == NULL) {
        ERROR("dispatch_source_create failed in ioloop_add_wake_event().");
        return false;
    }
    dispatch_retain(subproc->dispatch_source);
    dispatch_source_set_event_handler_f(subproc->dispatch_source, subproc_event);
    dispatch_source_set_cancel_handler_f(subproc->dispatch_source, subproc_cancel);
    dispatch_set_context(subproc->dispatch_source, subproc);
    dispatch_activate(subproc->dispatch_source);
    RETAIN_HERE(subproc, subproc); // Dispatch has a reference

    // Now that we have a viable subprocess, add the reader callback.
    if (output_callback != NULL && subproc->output_fd != NULL) {
        close(subproc->pipe_fds[1]);
        ioloop_add_reader(subproc->output_fd, output_callback);
    }
    return subproc;
}

#ifdef SRP_TEST_SERVER
void
ioloop_dnssd_txn_cancel_srp(void *srp_server, dnssd_txn_t *txn)
{
    if (txn->sdref != NULL) {
        INFO("txn %p serviceref %p", txn, txn->sdref);
        if (srp_server != NULL) {
            dns_service_ref_deallocate(srp_server, txn->sdref);
        } else {
            DNSServiceRefDeallocate(txn->sdref);
        }
        txn->sdref = NULL;
    } else {
        INFO("dead transaction.");
    }
}
#endif

void
ioloop_dnssd_txn_cancel(dnssd_txn_t *txn)
{
    if (txn->sdref != NULL) {
        INFO("txn %p serviceref %p", txn, txn->sdref);
        DNSServiceRefDeallocate(txn->sdref);
        txn->sdref = NULL;
    } else {
        INFO("dead transaction.");
    }
}

static void
dnssd_txn_finalize(dnssd_txn_t *txn)
{
    if (txn->sdref != NULL) {
        ioloop_dnssd_txn_cancel(txn);
    }
    if (txn->finalize_callback) {
        txn->finalize_callback(txn->context);
    }
    free(txn);
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
ioloop_dnssd_txn_add_subordinate_(DNSServiceRef ref, void *context, dnssd_txn_finalize_callback_t finalize_callback,
                                  dnssd_txn_failure_callback_t failure_callback,
                                  const char *file, int line)
{
    dnssd_txn_t *txn = calloc(1, sizeof(*txn));
    (void)file; (void)line;
    (void)failure_callback;

    if (txn != NULL) {
        RETAIN(txn, dnssd_txn);
        txn->sdref = ref;
        INFO("txn %p serviceref %p", txn, ref);
        txn->context = context;
        txn->finalize_callback = finalize_callback;
    }
    return txn;
}

dnssd_txn_t *
ioloop_dnssd_txn_add_(DNSServiceRef ref, void *context, dnssd_txn_finalize_callback_t finalize_callback,
                      dnssd_txn_failure_callback_t failure_callback,
                      const char *file, int line)
{
    dnssd_txn_t *txn = ioloop_dnssd_txn_add_subordinate_(ref, context, finalize_callback, failure_callback, file, line);
    if (txn != NULL) {
        DNSServiceSetDispatchQueue(ref, ioloop_main_queue);
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


static void
file_descriptor_finalize(void *context)
{
    io_t *file_descriptor = context;
    if (file_descriptor->ref_count == 0) {
        if (file_descriptor->finalize) {
            file_descriptor->finalize(file_descriptor->context);
        }
        free(file_descriptor);
    }
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
        RETAIN(ret, file_descriptor);
    }
    return ret;
}

static void
ioloop_read_source_finalize(void *context)
{
    io_t *io = context;

    INFO("io %p fd %d, read source %p, write_source %p", io, io->fd, io->read_source, io->write_source);

    // Release the reference count that dispatch was holding.
    if (io->is_static) {
        if (io->context_release != NULL) {
            io->context_release(io->context);
        }
        FINALIZED(file_descriptor_finalized);
    } else {
        RELEASE_HERE(io, file_descriptor);
    }
}

static void
ioloop_read_source_cancel_callback(void *context)
{
    io_t *io = context;

    INFO("io %p fd %d, read source %p, write_source %p", io, io->fd, io->read_source, io->write_source);
    if (io->read_source != NULL) {
        dispatch_release(io->read_source);
        io->read_source = NULL;
        if (io->fd != -1) {
            close(io->fd);
            io->fd = -1;
        } else {
            FAULT("io->fd has been set to -1 too early");
        }
    }
}

static void
ioloop_read_event(void *context)
{
    io_t *io = context;

    if (io->read_callback != NULL) {
        io->read_callback(io, io->context);
    }
}

void
ioloop_close(io_t *io)
{
    INFO("io %p fd %d, read source %p, write_source %p", io, io->fd, io->read_source, io->write_source);
    if (io->read_source != NULL) {
        dispatch_cancel(io->read_source);
    }
    if (io->write_source != NULL) {
        dispatch_cancel(io->write_source);
    }
}

void
ioloop_add_reader(io_t *NONNULL io, io_callback_t NONNULL callback)
{
    io->read_callback = callback;
    if (io->read_source == NULL) {
        io->read_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, io->fd, 0, ioloop_main_queue);
    }
    if (io->read_source == NULL) {
        ERROR("dispatch_source_create: unable to create read dispatch source.");
        return;
    }
    dispatch_source_set_event_handler_f(io->read_source, ioloop_read_event);
    dispatch_source_set_cancel_handler_f(io->read_source, ioloop_read_source_cancel_callback);
    dispatch_set_finalizer_f(io->read_source, ioloop_read_source_finalize);
    dispatch_set_context(io->read_source, io);
    RETAIN_HERE(io, file_descriptor); // Dispatch will hold a reference.
    dispatch_resume(io->read_source);
    INFO("io %p fd %d, read source %p, write_source %p", io, io->fd, io->read_source, io->write_source);
}

void
ioloop_run_async(async_callback_t callback, void *context)
{
    dispatch_async(ioloop_main_queue, ^{
            callback(context);
        });
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

bool
ioloop_is_device_apple_tv(void)
{
    return IsAppleTV();
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
