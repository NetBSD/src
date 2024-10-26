/* test-api.c
 *
 * Copyright (c) 2023-2024 Apple Inc. All rights reserved.
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
 * srp host API test harness
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dns_sd.h>
#include <errno.h>
#include <fcntl.h>

#include "srp.h"
#include "srp-api.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "dso-utils.h"
#include "dso.h"

#include "cti-services.h"
#include "test-api.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "route.h"

#define SRP_IO_CONTEXT_MAGIC 0xFEEDFACEFADEBEEFULL  // BEES!   Everybody gets BEES!
typedef struct io_context {
    uint64_t magic_cookie1;
    wakeup_t *wakeup;
    void *NONNULL srp_context;
    void *NONNULL host_context;
    comm_t *NULLABLE connection;
    srp_wakeup_callback_t wakeup_callback;
    srp_datagram_callback_t datagram_callback;
    bool deactivated, closed;
    uint64_t magic_cookie2;
} io_context_t;

// For testing signature with a time that's out of range.
static bool test_bad_sig_time;
// For testing a signature that doesn't validate
static bool invalidate_signature;

bool
configure_dnssd_proxy(void)
{
    extern srp_server_t *srp_servers;
    if (srp_servers->test_state != NULL && srp_servers->test_state->dnssd_proxy_configurer != NULL) {
        return srp_servers->test_state->dnssd_proxy_configurer();
    } else {
        dnssd_proxy_udp_port= 53;
        dnssd_proxy_tcp_port = 53;
        dnssd_proxy_tls_port = 853;
        return true;
    }
}

int
srp_test_getifaddrs(srp_server_t *server_state, struct ifaddrs **ifaddrs, void *context)
{
    if (server_state->test_state != NULL && server_state->test_state->getifaddrs != NULL) {
        return server_state->test_state->getifaddrs(server_state, ifaddrs, context);
    }
    return getifaddrs(ifaddrs);
}

void
srp_test_freeifaddrs(srp_server_t *server_state, struct ifaddrs *ifaddrs, void *context)
{
    if (server_state->test_state != NULL && server_state->test_state->freeifaddrs != NULL) {
        server_state->test_state->freeifaddrs(server_state, ifaddrs, context);
        return;
    }
    freeifaddrs(ifaddrs);
}

void
srp_test_state_add_timeout(test_state_t *state, int timeout)
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * timeout), dispatch_get_main_queue(), ^{
            if (!state->test_complete) {
                TEST_FAIL(state, "test failed: timeout");
                exit(1);
            }
        });
}

void
srp_test_state_next(test_state_t *state)
{
    if (state->next != NULL) {
        test_state_t *next_state = state->next;
        next_state->test_complete = true;
        next_state->finished_tests = state;
        if (next_state->continue_testing == NULL) {
            TEST_FAIL(next_state, "no continue function");
        }
        next_state->continue_testing(next_state);
    } else {
        exit(0);
    }
}

void
srp_test_state_explain(test_state_t *state)
{
    if (state != NULL) {
        if (state->variant_title != NULL) {
            fprintf(stderr, "\n%s (%s variant)\n", state->title, state->variant_title);
        } else {
            fprintf(stderr, "\n%s\n", state->title);
        }
        fprintf(stderr, "\n%s\n\n", state->explanation);
        if (state->variant_info != NULL) {
            fprintf(stderr, "Variant: %s\n\n", state->variant_info);
        }
    }
}

test_state_t *
test_state_create(srp_server_t *primary, const char *title, const char *variant_title,
                  const char *explanation, const char *variant_info)
{
    test_state_t *ret = calloc(1, sizeof(*ret));
    TEST_FAIL_CHECK(NULL, ret != NULL, "no memory for test state");
    ret->primary = primary;
    primary->test_state = ret;
    ret->title = title;
    ret->variant_title = variant_title;
    ret->explanation = explanation; // Explanation is assumed to be a compile-time constant string.
    ret->variant_info = variant_info;
    return ret;
}

void
srp_test_set_local_example_address(test_state_t *UNUSED state)
{
    static const uint8_t ifaddr[] = {
        0x20, 1, 0xd, 0xb8, // 2001:0db8:
        0, 0, 0, 0, // /64 prefix
        0, 0, 0, 0,
        0, 0, 0, 1, // 2001:db8::1
    };
    srp_add_interface_address(dns_rrtype_aaaa, ifaddr, sizeof(ifaddr));
}

void
srp_test_network_localhost_start(test_state_t *UNUSED state)
{
    static const uint8_t localhost[] = {
        0, 0, 0, 0,
        0, 0, 0, 0, // /64 prefix
        0, 0, 0, 0,
        0, 0, 0, 1, // ::1
    };
    static const uint8_t port[] = { 0, 53 };

    srp_add_server_address(port, dns_rrtype_aaaa, localhost, sizeof(localhost));
    srp_test_set_local_example_address(state);

    srp_network_state_stable(NULL);
}

bool
srp_get_last_server(uint16_t *NONNULL UNUSED rrtype, uint8_t *NONNULL UNUSED rdata, uint16_t UNUSED rdlim,
                    uint8_t *NONNULL UNUSED port, void *NULLABLE UNUSED host_context)
{
    return false;
}

bool
srp_save_last_server(uint16_t UNUSED rrtype, uint8_t *UNUSED rdata, uint16_t UNUSED rdlength,
                     uint8_t *UNUSED port, void *UNUSED host_context)
{
    return false;
}

static int
validate_io_context(io_context_t **dest, void *src)
{
    io_context_t *context = src;
    if (context->magic_cookie1 == SRP_IO_CONTEXT_MAGIC &&
        context->magic_cookie2 == SRP_IO_CONTEXT_MAGIC)
   {
        *dest = context;
        return kDNSServiceErr_NoError;
    }
    return kDNSServiceErr_BadState;
}

int
srp_deactivate_udp_context(void *host_context, void *in_context)
{
    io_context_t *io_context;
    int err;
    (void)host_context;

    err = validate_io_context(&io_context, in_context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->wakeup != NULL) {
            ioloop_cancel_wake_event(io_context->wakeup);
            ioloop_wakeup_release(io_context->wakeup);
        }
        // Deactivate can be called with a connection still active; in this case, we need to wait for the
        // cancel event before freeing the structure. Otherwise, we can free it immediately.
        if (io_context->connection != NULL) {
            ioloop_comm_cancel(io_context->connection);
            io_context->deactivated = true;
            io_context->closed = true;
        } else {
            free(io_context);
        }
    }
    return err;
}

int
srp_disconnect_udp(void *context)
{
    io_context_t *io_context;
    int err;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->wakeup != NULL) {
            ioloop_cancel_wake_event(io_context->wakeup);
        }
        if (io_context->connection) {
            io_context->connection = NULL;
        }
        io_context->closed = true;
    }
    return err;
}

static bool
srp_test_send_intercept(comm_t *connection, message_t *UNUSED responding_to,
                        struct iovec *iov, int iov_len, bool UNUSED final, bool send_length)
{
    bool send_length_real = send_length;
    srp_server_t *srp_server = connection->test_context;
    test_state_t *test_state = srp_server->test_state;
    io_context_t *current_io_context = test_state->current_io_context;
    TEST_FAIL_CHECK(test_state, test_state != NULL, "no test state");
    TEST_FAIL_CHECK(test_state, current_io_context != NULL, "no I/O state");
    TEST_FAIL_CHECK(test_state, current_io_context->datagram_callback != NULL, "no datagram callback");

    // Don't copy if we don't have to.
    if (!send_length && iov_len == 1) {
        size_t len = iov[0].iov_len;
        uint8_t *data = calloc(1, len);
        memcpy(data, iov[0].iov_base, len);
        dispatch_async(dispatch_get_main_queue(), ^{
                current_io_context->datagram_callback(current_io_context, data, len);
                free(data);
            });
        return true;
    }

    // send_length indicates whether we should send a length over TCP, not whether we should send a length.
    if (!connection->tcp_stream) {
        send_length_real = false;
    }

    // We have an actual iov, or need to prepend a length, so we have to allocate and copy.
    uint8_t *message;
    size_t length = send_length_real ? 2 : 0;
    uint8_t *mp;
    for (int i = 0; i < iov_len; i++) {
        length += iov[i].iov_len;
    }

    message = malloc(length);
    TEST_FAIL_CHECK(test_state, message != NULL, "no memory for message");
    mp = message;
    // Marshal all the data into a single buffer.
    if (send_length_real) {
        *mp++ = length >> 8;
        *mp++ = length & 0xff;
    }
    for (int i = 0; i < iov_len; i++) {
        memcpy(mp, iov[i].iov_base, iov[i].iov_len);
        mp += iov[i].iov_len;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
            current_io_context->datagram_callback(current_io_context->srp_context, message, length);
        });
    return true;
}

int
srp_connect_udp(void *context, const uint8_t *UNUSED port, uint16_t UNUSED address_type,
                const uint8_t *UNUSED address, uint16_t UNUSED addrlen)
{
    io_context_t *io_context;
    int err;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->connection) {
            ERROR("srp_connect_udp called with non-null I/O context.");
            return kDNSServiceErr_Invalid;
        }

        srp_server_t *server_state = io_context->host_context;
        test_state_t *test_state = server_state->test_state;
        if (test_state == NULL || test_state->srp_listener == NULL) {
            return kDNSServiceErr_NotInitialized;
        }
        io_context->connection = test_state->srp_listener;
        test_state->current_io_context = io_context;
        io_context->connection->test_send_intercept = srp_test_send_intercept;
        io_context->connection->test_context = server_state;
    }
    return err;
}

int
srp_make_udp_context(void *host_context, void **p_context, srp_datagram_callback_t callback, void *context)
{
    io_context_t *io_context = calloc(1, sizeof *io_context);
    if (io_context == NULL) {
        return kDNSServiceErr_NoMemory;
    }
    io_context->magic_cookie1 = io_context->magic_cookie2 = SRP_IO_CONTEXT_MAGIC;
    io_context->datagram_callback = callback;
    io_context->srp_context = context;
    io_context->host_context = host_context;

    io_context->wakeup = ioloop_wakeup_create();
    if (io_context->wakeup == NULL) {
        free(io_context);
        return kDNSServiceErr_NoMemory;
    }

    *p_context = io_context;
    return kDNSServiceErr_NoError;
}

static void
wakeup_callback(void *context)
{
    io_context_t *io_context;
    if (validate_io_context(&io_context, context) == kDNSServiceErr_NoError) {
        INFO("wakeup on context %p srp_context %p", io_context, io_context->srp_context);
        INFO("setting wakeup callback %p wakeup %p", io_context->wakeup_callback, io_context->wakeup);
        if (!io_context->deactivated) {
            io_context->wakeup_callback(io_context->srp_context);
        }
    } else {
        INFO("wakeup with invalid context: %p", context);
    }
}

int
srp_set_wakeup(void *host_context, void *context, int milliseconds, srp_wakeup_callback_t callback)
{
    int err;
    io_context_t *io_context;
    (void)host_context;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        INFO("setting wakeup callback %p wakeup %p", callback, io_context->wakeup);
        io_context->wakeup_callback = callback;
        ioloop_add_wake_event(io_context->wakeup, io_context, wakeup_callback, NULL, milliseconds);
    }
    return err;
}

int
srp_cancel_wakeup(void *host_context, void *context)
{
    int err;
    io_context_t *io_context;
    (void)host_context;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        ioloop_cancel_wake_event(io_context->wakeup);
    }
    return err;
}

int
srp_send_datagram(void *host_context, void *context, void *message, size_t message_length)
{
    int err;
    io_context_t *io_context;
    srp_server_t *srp_server = host_context;
    test_state_t *test_state = srp_server->test_state;

    if (invalidate_signature) {
        ((uint8_t *)message)[message_length - 10] = ~(((uint8_t *)message)[message_length - 10]);
    }

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->connection == NULL) {
            return kDNSServiceErr_DefunctConnection;
        }
        TEST_FAIL_CHECK(test_state, io_context->connection->datagram_callback != NULL, "srp listener has no datagram callback");
        message_t *actual = ioloop_message_create(message_length);
        TEST_FAIL_CHECK(test_state, actual != NULL, "no memory for message");
        memcpy(&actual->wire, message, message_length);
        io_context->connection->datagram_callback(io_context->connection, actual, io_context->connection->context);
        ioloop_message_release(actual);
    }
    return err;
}

uint32_t
srp_timenow(void)
{
    time_t now = time(NULL);
    if (test_bad_sig_time) {
        return (uint32_t)(now - 10000);
    }
    return (uint32_t)now;
}

void
srp_test_enable_stub_router(test_state_t *state, srp_server_t *NONNULL server_state)
{
    server_state->route_state = route_state_create(server_state, "srp-mdns-proxy");
    TEST_FAIL_CHECK(state, server_state->route_state != NULL, "no memory for route state");
    server_state->stub_router_enabled = true;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
