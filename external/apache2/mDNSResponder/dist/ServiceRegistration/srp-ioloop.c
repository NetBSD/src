/* srp-ioloop.c
 *
 * Copyright (c) 2019-2024 Apple Inc. All rights reserved.
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
 * srp host API implementation for Posix using ioloop primitives.
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

static int lease_time = 0;
static bool random_leases = false;
static bool delete_registrations = false;
static bool use_thread_services = false;
static bool change_txt_record = false;
static bool new_ip_dup = false;
static bool dup_instance_name = false;
static bool random_txt_record = false;
static bool remove_added_service = false;
static bool let_added_service_expire = false;
static bool expecting_second_add = true;
static int num_clients = 1;
static int bogusify_signatures = false;
static int bogus_remove = false;
static int push_hardwired = false;
static int push_query = false;
static int push_unsubscribe = false;
static bool push_subscribe_sent = false;
static uint16_t dns_push_subscribe_xid;
static int push_send_bogus_keepalive = false;
static int push_exhaust = false;
static bool test_subtypes = false;
static bool test_renew_subtypes = false;
static bool test_diff_subtypes = false;
static uint8_t first_bogus_address[] = { 198, 51, 100, 1 }; // RFC 5737 documentation prefix TEST-NET-2
static uint8_t second_bogus_address[] = { 203, 0, 113, 1 }; // RFC 5737 documentation prefix TEST-NET-3
static bool host_only = false;
extern bool zero_addresses;
static bool expire_instance = false;
static bool test_bad_sig_time = false;
static bool do_srp = true;

const uint64_t thread_enterprise_number = 52627;

cti_connection_t thread_service_context;

static const char *interface_name = NULL;
static wakeup_t *wait_for_remote_disconnect = NULL;
static dso_state_t *disconnect_expected = NULL;
os_log_t global_os_log;

#define SRP_IO_CONTEXT_MAGIC 0xFEEDFACEFADEBEEFULL  // BEES!   Everybody gets BEES!
typedef struct io_context {
    uint64_t magic_cookie1;
    wakeup_t *wakeup;
    void *NONNULL srp_context;
    comm_t *NULLABLE connection;
    srp_wakeup_callback_t wakeup_callback;
    srp_datagram_callback_t datagram_callback;
    bool deactivated, closed;
    uint64_t magic_cookie2;
} io_context_t;
wakeup_t *remove_wakeup;

typedef struct srp_client {
    DNSServiceRef sdref;
    int index;
    wakeup_t *wakeup;
    char *name;
    bool updated_txt_record;
} srp_client_t;

static void start_push_query(void);
static void send_push_unsubscribe(void);
static void send_push_subscribe(void);

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

static void
datagram_callback(comm_t *connection, message_t *message, void *context)
{
    (void)connection;
    io_context_t *io_context = context;
    if (!io_context->deactivated) {
        io_context->datagram_callback(io_context->srp_context,
                                      &message->wire, message->length);
    }
}

static void
wakeup_callback(void *context)
{
    io_context_t *io_context;
    if (validate_io_context(&io_context, context) == kDNSServiceErr_NoError) {
        INFO("wakeup on context %p srp_context %p", io_context, io_context->srp_context);
        if (!io_context->deactivated) {
            io_context->wakeup_callback(io_context->srp_context);
        }
    } else {
        INFO("wakeup with invalid context: %p", context);
    }
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
        if (io_context->connection) {
            ioloop_comm_cancel(io_context->connection);
        }
        io_context->closed = true;
    }
    return err;
}

static void
srp_udp_context_canceled(comm_t *UNUSED NONNULL comm, void *NULLABLE context, int UNUSED error)
{
    io_context_t *io_context = context;

    if (io_context->connection) {
        ioloop_comm_release(io_context->connection);
        io_context->connection = NULL;
    }
    if (io_context->deactivated) {
        free(io_context);
    }
}

int
srp_connect_udp(void *context, const uint8_t *port, uint16_t address_type, const uint8_t *address, uint16_t addrlen)
{
    io_context_t *io_context;
    addr_t remote;
    int err;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->connection) {
            ERROR("srp_connect_udp called with non-null I/O context.");
            return kDNSServiceErr_Invalid;
        }

        if (address_type == dns_rrtype_a) {
            if (addrlen != 4) {
                return kDNSServiceErr_Invalid;
            }
            remote.sa.sa_family = AF_INET;
            memcpy(&remote.sin.sin_addr, address, addrlen);
#ifndef NOT_HAVE_SA_LEN
            remote.sa.sa_len = sizeof remote.sin;
#endif
            memcpy(&remote.sin.sin_port, port, 2);
        } else {
            if (addrlen != 16) {
                return kDNSServiceErr_Invalid;
            }
            remote.sa.sa_family = AF_INET6;
            memcpy(&remote.sin6.sin6_addr, address, addrlen);
#ifndef NOT_HAVE_SA_LEN
            remote.sa.sa_len = sizeof remote.sin;
#endif
            memcpy(&remote.sin6.sin6_port, port, 2);
        }

        io_context->connection = ioloop_connection_create(&remote, false, false, false, true, datagram_callback,
                                                          NULL, srp_udp_context_canceled, NULL, io_context);
        if (io_context->connection == NULL) {
            return kDNSServiceErr_NoMemory;
        }
    }
    return err;
}

int
srp_make_udp_context(void *host_context, void **p_context, srp_datagram_callback_t callback, void *context)
{
    (void)host_context;

    io_context_t *io_context = calloc(1, sizeof *io_context);
    if (io_context == NULL) {
        return kDNSServiceErr_NoMemory;
    }
    io_context->magic_cookie1 = io_context->magic_cookie2 = SRP_IO_CONTEXT_MAGIC;
    io_context->datagram_callback = callback;
    io_context->srp_context = context;

    io_context->wakeup = ioloop_wakeup_create();
    if (io_context->wakeup == NULL) {
        free(io_context);
        return kDNSServiceErr_NoMemory;
    }

    *p_context = io_context;
    return kDNSServiceErr_NoError;
}

int
srp_set_wakeup(void *host_context, void *context, int milliseconds, srp_wakeup_callback_t callback)
{
    int err;
    io_context_t *io_context;
    (void)host_context;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        io_context->wakeup_callback = callback;
        INFO("srp_set_wakeup on context %p, srp_context %p", io_context, io_context->srp_context);
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
    struct iovec iov;
    io_context_t *io_context;
    (void)host_context;

    memset(&iov, 0, sizeof iov);
    iov.iov_base = message;
    iov.iov_len = message_length;

    if (bogusify_signatures) {
        ((uint8_t *)message)[message_length - 10] = ~(((uint8_t *)message)[message_length - 10]);
    }

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        if (io_context->connection == NULL) {
            return kDNSServiceErr_DefunctConnection;
        }
        if (!ioloop_send_message(io_context->connection, message, &iov, 1)) {
            return kDNSServiceErr_Unknown;
        }
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

static void
interface_callback(srp_server_t *UNUSED NULLABLE server_state, void *context, const char *NONNULL name,
                   const addr_t *NONNULL address, const addr_t *NONNULL netmask,
                   uint32_t flags, enum interface_address_change event_type)
{
    bool drop = false;
    uint8_t *rdata;
    uint16_t rdlen;
    uint16_t rrtype;
    cti_service_vec_t *cti_services = context;

    (void)netmask;
    (void)index;
    (void)event_type;

    if (address->sa.sa_family == AF_INET) {
        rrtype = dns_rrtype_a;
        rdata = (uint8_t *)&address->sin.sin_addr;
        rdlen = 4;

        // Should use IN_LINKLOCAL and IN_LOOPBACK macros here, but for some reason they are not present on
        // OpenWRT.
        if (rdata[0] == 127) {
            drop = true;
        } else if (rdata[0] == 169 && rdata[1] == 254) {
            drop = true;
        }
    } else if (address->sa.sa_family == AF_INET6) {
        rrtype = dns_rrtype_aaaa;
        rdata = (uint8_t *)&address->sin6.sin6_addr;
        rdlen = 16;
        if (IN6_IS_ADDR_LOOPBACK(&address->sin6.sin6_addr)) {
            drop = true;
        } else if (IN6_IS_ADDR_LINKLOCAL(&address->sin6.sin6_addr)) {
            drop = true;
        }
    } else {
        return;
    }
    if (drop) {
        if (address->sa.sa_family == AF_INET) {
            IPv4_ADDR_GEN_SRP(rdata, ipv4_rdata_buf);
            DEBUG("interface_callback: ignoring " PUB_S_SRP " " PRI_IPv4_ADDR_SRP, name,
                  IPv4_ADDR_PARAM_SRP(rdata, ipv4_rdata_buf));
        } else if (address->sa.sa_family == AF_INET6) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(rdata, ipv6_rdata_buf);
            DEBUG("interface_callback: ignoring " PUB_S_SRP " " PRI_SEGMENTED_IPv6_ADDR_SRP, name,
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(rdata, ipv6_rdata_buf));
        } else {
            INFO("ignoring with non-v4/v6 address" PUB_S_SRP, name);
        }
        return;
    }

    if (address->sa.sa_family == AF_INET) {
        IPv4_ADDR_GEN_SRP(rdata, ipv4_rdata_buf);
        DEBUG("interface_callback: " PUB_S_SRP " " PRI_IPv4_ADDR_SRP " %x", name,
              IPv4_ADDR_PARAM_SRP(rdata, ipv4_rdata_buf), flags);
    } else if (address->sa.sa_family == AF_INET6) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(rdata, ipv6_rdata_buf);
        DEBUG("interface_callback: " PUB_S_SRP " " PRI_SEGMENTED_IPv6_ADDR_SRP " %x", name,
              SEGMENTED_IPv6_ADDR_PARAM_SRP(rdata, ipv6_rdata_buf), flags);
    } else {
        DEBUG("interface_callback: " PUB_S_SRP "<none IPv4/IPv6 address> %x", name, flags);
    }

    // This is a workaround for a bug in the utun0 code, where packets sent to the IP address of the local
    // thread interface are dropped and do not reach the SRP server. To address this, if we find a service
    // that is on a local IPv6 address, we replace the address with ::1.
    if (cti_services != NULL && rrtype == dns_rrtype_aaaa) {
        size_t i;
        for (i = 0; i < cti_services->num; i++) {
            cti_service_t *cti_service = cti_services->services[i];
            // Look for SRP service advertisements only.
            if (IS_SRP_SERVICE(cti_service)) {
                // Local IP address?
                if (!memcmp(cti_service->server, rdata, 16)) {
                    // ::
                    memset(cti_service->server, 0, 15);
                    // 1
                    cti_service->server[15] = 1;
                }
            }
        }
    }

    srp_add_interface_address(rrtype, rdata, rdlen);
}

static void
remove_callback(void *context)
{
    srp_client_t *client = context;
    if (bogus_remove) {
        srp_set_hostname("bogus-api-test", NULL);
    }
    srp_deregister(client);
}

static void
second_register_callback(DNSServiceRef sdref, DNSServiceFlags flags, DNSServiceErrorType errorCode,
                         const char *name, const char *regtype, const char *domain, void *context)
{
    srp_client_t *client = context;

    (void)regtype;
    (void)flags;
    (void)name;
    (void)regtype;
    (void)domain;
    INFO("Second Register Reply for %s: %d", client->name, errorCode);

    if (errorCode == kDNSServiceErr_NoError) {
        if (expecting_second_add) {
            expecting_second_add = false;
            if (remove_added_service) {
                srp_deregister_instance(sdref);
                srp_network_state_stable(NULL);
            } else if (let_added_service_expire) {
                DNSServiceRefDeallocate(sdref);
            }
        } else {
            // Test succeeded
            exit(0);
        }
    } else {
        // Test failed
        exit(1);
    }
}


static void
register_callback(DNSServiceRef sdref, DNSServiceFlags flags, DNSServiceErrorType errorCode,
                  const char *name, const char *regtype, const char *domain, void *context)
{
    srp_client_t *client = context;

    (void)regtype;
    (void)flags;
    (void)name;
    (void)regtype;
    (void)domain;
    INFO("Register Reply for %s: %d", client->name, errorCode);

    if (errorCode == kDNSServiceErr_NoError) {
        if ((change_txt_record  && !client->updated_txt_record) || expire_instance) {
            TXTRecordRef txt;
            const void *txt_data = NULL;
            uint16_t txt_len = 0;
            char txt_buf[128];

            TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
            TXTRecordSetValue(&txt, "foo", 1, "1");
            TXTRecordSetValue(&txt, "bar", 3, "1.1");
            txt_data = TXTRecordGetBytesPtr(&txt);
            txt_len = TXTRecordGetLength(&txt);

            if (expire_instance) {
                char hnbuf[128];
                ioloop_strcpy(hnbuf, name, sizeof(hnbuf));

                // silently let the first expire. This is just going to leak the data, but since this is a one-shot test
                // that's not an actual problem.
                DNSServiceRefDeallocate(sdref);

                DNSServiceRef nsdref;
                expecting_second_add = true;
                // register a second instance
                int err = DNSServiceRegister(&nsdref, 0, 0, hnbuf, "_second._tcp,foo", 0, 0, htons(1234),
                                             txt_len, txt_data, second_register_callback, client);
                if (err != kDNSServiceErr_NoError) {
                    ERROR("second DNSServiceRegister failed: %d", err);
                    exit(1);
                }
            } else {
                (void)DNSServiceUpdateRecord(sdref, NULL, 0, txt_len, txt_data, 0);
                client->updated_txt_record = true;
            }
            srp_network_state_stable(NULL);
            return;
        }

        if (delete_registrations) {
            client->wakeup = ioloop_wakeup_create();
            if (client->wakeup == NULL) {
                ERROR("Unable to allocate a wakeup for %s.", client->name);
                exit(1);
            }

            // Do a remove in five seconds.
            ioloop_add_wake_event(client->wakeup, client, remove_callback, NULL, 5000);
            return;
        }

        if (test_renew_subtypes) {
            int err = srp_update_service_type(sdref, "_ipps._tcp,othersub", second_register_callback, client);
            if (err != kDNSServiceErr_NoError) {
                ERROR("srp_update_service_type failed: %d", err);
                exit(1);
            }
            srp_network_state_stable(NULL);
            return;
        }
    }

    // We get this with the duplicate instance name. In this case, we change the host IP address. This validates
    // the bit of code in srp-mdns-proxy that removes the added address when the update fails--when we look up the
    // registered address, we should see only the second bogus address, not the first.
    if (errorCode == kDNSServiceErr_NameConflict) {
        char nnbuf[128];
        char rtbuf[128];
        srp_delete_interface_address(dns_rrtype_a, first_bogus_address, sizeof(first_bogus_address));
        srp_add_interface_address(dns_rrtype_a, second_bogus_address, sizeof(second_bogus_address));
        ioloop_strcpy(nnbuf, name, sizeof(nnbuf));
        ioloop_strcpy(rtbuf, regtype, sizeof(rtbuf));
        nnbuf[0] = 'a';
        INFO("changing service instance name from " PRI_S_SRP " to " PRI_S_SRP " type " PRI_S_SRP, name, nnbuf, rtbuf);
        DNSServiceRefDeallocate(sdref);
        int err = DNSServiceRegister(&sdref, kDNSServiceFlagsNoAutoRename, 0, nnbuf, rtbuf, 0, 0, htons(1234),
                                     0, NULL, register_callback, client);
        if (err != kDNSServiceErr_NoError) {
            ERROR("DNSServiceRegister rename failed: %d", err);
            exit(1);
        }
        srp_network_state_stable(NULL);
    }
}

comm_t *dso_connection;
typedef struct connection_list connection_list_t;
struct connection_list {
    connection_list_t *next;
    comm_t *connection;
};
connection_list_t *dso_connection_list;
uint16_t subscribe_xid;
uint16_t keepalive_xid;

static void
send_push_unsubscribe(void)
{
    struct iovec iov;
    INFO("unsubscribe");
    dns_wire_t dns_message;
    uint8_t *buffer = (uint8_t *)&dns_message;
    dns_towire_state_t towire;
    dso_message_t message;
    if (!push_send_bogus_keepalive) {
        INFO("unsubscribe");
        dso_make_message(&message, buffer, sizeof(dns_message), dso_connection->dso, true, false, 0, 0, NULL);
        memset(&towire, 0, sizeof(towire));
        towire.p = &buffer[DNS_HEADER_SIZE];
        towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
        towire.message = &dns_message;
        dns_u16_to_wire(&towire, kDSOType_DNSPushUnsubscribe);
        dns_rdlength_begin(&towire);
        dns_u16_to_wire(&towire, dns_push_subscribe_xid);
        dns_rdlength_end(&towire);

        memset(&iov, 0, sizeof(iov));
        iov.iov_len = towire.p - buffer;
        iov.iov_base = buffer;
        ioloop_send_message(dso_connection, NULL, &iov, 1);
        subscribe_xid = dns_message.id; // We need this to identify the response.
    }

    // Send a keepalive message so that we can get the response, since the unsubscribe is not a response-requiring request.
    dso_make_message(&message, buffer, sizeof(dns_message), dso_connection->dso, false, false, 0, 0, NULL);
    memset(&towire, 0, sizeof(towire));
    towire.p = &buffer[DNS_HEADER_SIZE];
    towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
    towire.message = &dns_message;
    dns_u16_to_wire(&towire, kDSOType_Keepalive);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, 600);
    dns_u32_to_wire(&towire, 600);
    dns_rdlength_end(&towire);
    if (push_send_bogus_keepalive) {
        INFO("sending bogus keepalive");
        // Send a badly formatted message.
        dns_u32_to_wire(&towire, 0x12345678);
    }
    keepalive_xid = dns_message.id;
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - buffer;
    iov.iov_base = buffer;
    ioloop_send_message(dso_connection, NULL, &iov, 1);
}

static void
dso_remote_disconnect_didnt_happen(void *UNUSED context)
{
    INFO("remote disconnect didn't happen");
    exit(1);
}

static void
handle_retry_delay(dso_state_t *dso, uint32_t delay)
{
    INFO("Got our retry delay, %ums...", delay);
    wait_for_remote_disconnect = ioloop_wakeup_create();
    if (!wait_for_remote_disconnect) {
        INFO("can't wait for remote disconnect.");
        exit(1);
    }
    // Wait six seconds for remote disconnect, which should happen in five.
    ioloop_add_wake_event(wait_for_remote_disconnect, NULL, dso_remote_disconnect_didnt_happen, NULL, 6 * 1000);
    disconnect_expected = dso;
}

static void
dso_message(message_t *message, dso_state_t *dso, bool response)
{
#if PRINT_TO_STDERR
    char name[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    char ptrname[DNS_MAX_NAME_SIZE_ESCAPED + 1];
#endif
    unsigned offset, max;
    dns_rr_t rr;
    uint8_t *message_bytes;

    switch(dso->primary.opcode) {
    case kDSOType_RetryDelay:
        if (response) {
            INFO("server sent a retry delay TLV as a response.");
            exit(1);
        }
        dso_retry_delay(dso, &message->wire);
        break;

    case kDSOType_Keepalive:
        if (response) {
            INFO("Keepalive response from server, rcode = %d", dns_rcode_get(&message->wire));
            exit(0);
        } else {
            INFO("Keepalive from server");
        }
        if (!push_subscribe_sent) {
            send_push_subscribe();
            push_subscribe_sent = true;
        }
        break;

    case kDSOType_DNSPushSubscribe:
        if (response) {
            // This is a protocol error--the response isn't supposed to contain a primary TLV.
            INFO("DNS Push response from server, rcode = %d", dns_rcode_get(&message->wire));
            exit(1);
        } else {
            INFO("Unexpected DNS Push request from server, rcode = %d", dns_rcode_get(&message->wire));
        }
        break;

    case kDSOType_DNSPushUpdate:
        // DNS Push Updates are never responses.
        // DNS Push updates are compressed, so we can't just parse data out of the primary--we need to align
        // our parse with the start of the message data.
        message_bytes = (uint8_t *)message->wire.data;
        offset = (unsigned)(dso->primary.payload - message_bytes); // difference can never be greater than sizeof(message->wire).
        max = offset + dso->primary.length;
        while (offset < max) {
            if (!dns_rr_parse(&rr, message_bytes, max, &offset, true, true)) {
                // Should have emitted an error earlier
                break;
            }
#if PRINT_TO_STDERR
            dns_name_print(rr.name, name, sizeof(name));
            if (rr.type != dns_rrtype_ptr) {
                fprintf(stderr, "%s: type %u class %u ttl %" PRIu32 "\n", name, rr.type, rr.qclass, rr.ttl);
            } else {
                dns_name_print(rr.data.ptr.name, ptrname, sizeof(ptrname));
                fprintf(stderr, "%s IN PTR %s\n", name, ptrname);
            }
#endif
        }
        if (push_unsubscribe) {
            send_push_unsubscribe();
        } else if (!push_exhaust) {
            exit(0);
        }
        break;

    case kDSOType_NoPrimaryTLV: // No Primary TLV
        if (response) {
            if (message->wire.id == subscribe_xid) {
                int rcode = dns_rcode_get(&message->wire);
                INFO("DNS Push Subscribe response from server, rcode = %d", rcode);
                if (rcode != dns_rcode_noerror) {
                    exit(0);
                }
                if (push_exhaust) {
                    start_push_query();
                }
            } else if (message->wire.id == keepalive_xid) {
                int rcode = dns_rcode_get(&message->wire);
                INFO("DNS Keepalive response from server, rcode = %d", rcode);
                exit(0);
            } else {
                int rcode = dns_rcode_get(&message->wire);
                INFO("Unexpected DSO response from server, rcode = %d", rcode);
            }
        } else {
            INFO("DSO request with no primary TLV.");
            exit(1);
        }
        break;

    default:
        INFO("dso_message: unexpected primary TLV %d", dso->primary.opcode);
        dso_simple_response(dso_connection, NULL, &message->wire, dns_rcode_dsotypeni);
        break;
    }
}

static void
dso_event_callback(void *UNUSED context, void *event_context, dso_state_t *dso, dso_event_type_t eventType)
{
    message_t *message;
    dso_query_receive_context_t *response_context;
    dso_disconnect_context_t *disconnect_context;

    switch(eventType)
    {
    case kDSOEventType_DNSMessage:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("DNS Message (opcode=%d) received from " PRI_S_SRP, dns_opcode_get(&message->wire),
             dso->remote_name);
        break;
    case kDSOEventType_DNSResponse:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("DNS Response (opcode=%d) received from " PRI_S_SRP, dns_opcode_get(&message->wire),
             dso->remote_name);
        break;
    case kDSOEventType_DSOMessage:
        INFO("DSO Message (Primary TLV=%d) received from " PRI_S_SRP,
               dso->primary.opcode, dso->remote_name);
        message = event_context;
        dso_message(message, dso, false);
        break;
    case kDSOEventType_DSOResponse:
        INFO("DSO Response (Primary TLV=%d) received from " PRI_S_SRP,
               dso->primary.opcode, dso->remote_name);
        response_context = event_context;
        message = response_context->message_context;
        dso_message(message, dso, true);
        break;

    case kDSOEventType_Finalize:
        INFO("Finalize");
        break;

    case kDSOEventType_Connected:
        INFO("Connected to " PRI_S_SRP, dso->remote_name);
        break;

    case kDSOEventType_ConnectFailed:
        INFO("Connection to " PRI_S_SRP " failed", dso->remote_name);
        break;

    case kDSOEventType_Disconnected:
        INFO("Connection to " PRI_S_SRP " disconnected", dso->remote_name);
        if (dso == disconnect_expected) {
            INFO("remote end disconnected as expected.");
            exit(0);
        }
        break;
    case kDSOEventType_ShouldReconnect:
        INFO("Connection to " PRI_S_SRP " should reconnect (not for a server)", dso->remote_name);
        break;
    case kDSOEventType_Inactive:
        INFO("Inactivity timer went off, closing connection.");
        break;
    case kDSOEventType_Keepalive:
        INFO("should send a keepalive now.");
        break;
    case kDSOEventType_KeepaliveRcvd:
        if (!push_subscribe_sent) {
            send_push_subscribe();
            push_subscribe_sent = true;
        }
        INFO("keepalive received.");
        break;
    case kDSOEventType_RetryDelay:
        disconnect_context = event_context;
        INFO("retry delay received, %d seconds", disconnect_context->reconnect_delay);
        handle_retry_delay(dso, disconnect_context->reconnect_delay);
        break;
    }
}

static void
send_push_subscribe(void)
{
    struct iovec iov;
    INFO("have session");
    dns_wire_t dns_message;
    uint8_t *buffer = (uint8_t *)&dns_message;
    dns_towire_state_t towire;
    dso_message_t message;
    dso_make_message(&message, buffer, sizeof(dns_message), dso_connection->dso, false, false, 0, 0, NULL);
    dns_push_subscribe_xid = ntohs(dns_message.id);
    memset(&towire, 0, sizeof(towire));
    towire.p = &buffer[DNS_HEADER_SIZE];
    towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
    towire.message = &dns_message;
    dns_u16_to_wire(&towire, kDSOType_DNSPushSubscribe);
    dns_rdlength_begin(&towire);
    if (push_hardwired) {
        dns_full_name_to_wire(NULL, &towire, "default.service.arpa");
        dns_u16_to_wire(&towire, dns_rrtype_soa);
    } else {
        dns_full_name_to_wire(NULL, &towire, "_airplay._tcp.default.service.arpa");
        dns_u16_to_wire(&towire, dns_rrtype_ptr);
    }
    dns_u16_to_wire(&towire, dns_qclass_in);
    dns_rdlength_end(&towire);

    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - buffer;
    iov.iov_base = buffer;
    ioloop_send_message(dso_connection, NULL, &iov, 1);
    subscribe_xid = dns_message.id; // We need this to identify the response.
}

static void
dso_connected(comm_t *connection, void *UNUSED context)
{
    struct iovec iov;
    INFO("connected");
    connection->dso = dso_state_create(false, 1, connection->name, dso_event_callback,
                                       dso_connection, NULL, dso_connection);
    if (connection->dso == NULL) {
        ERROR("can't create dso state object.");
        exit(1);
    }
    dns_wire_t dns_message;
    uint8_t *buffer = (uint8_t *)&dns_message;
    dns_towire_state_t towire;
    dso_message_t message;
    dso_make_message(&message, buffer, sizeof(dns_message), connection->dso, false, false, 0, 0, NULL);
    memset(&towire, 0, sizeof(towire));
    towire.p = &buffer[DNS_HEADER_SIZE];
    towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
    towire.message = &dns_message;
    dns_u16_to_wire(&towire, kDSOType_Keepalive);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, 100); // Inactivity timeout
    dns_u32_to_wire(&towire, 100); // Keepalive interval
    dns_rdlength_end(&towire);

    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - buffer;
    iov.iov_base = buffer;
    ioloop_send_message(dso_connection, NULL, &iov, 1);
}

static void
dso_disconnected(comm_t *UNUSED connection, void *UNUSED context, int UNUSED error)
{
    fprintf(stderr, "disconnected.");
    exit(0);
}

static void
dso_datagram_callback(comm_t *connection, message_t *message, void *UNUSED context)
{
    // If this is a DSO message, see if we have a session yet.
    switch(dns_opcode_get(&message->wire)) {
    case dns_opcode_dso:
        if (connection->dso == NULL) {
            INFO("dso message received with no DSO object on connection " PRI_S_SRP, connection->name);
            exit(1);
        }
        dso_message_received(connection->dso, (uint8_t *)&message->wire, message->length, message);
        return;
        break;
    }
    INFO("datagram on connection " PRI_S_SRP " not handled, type = %d.",
         connection->name, dns_opcode_get(&message->wire));
}

static void
start_push_query(void)
{
    // If we can (should always be able to) remember the list of connections we've created.
    if (dso_connection != NULL) {
        connection_list_t *connection = calloc(1, sizeof (*connection));
        if (connection != NULL) {
            connection->connection = dso_connection;
            connection->next = dso_connection_list;
            dso_connection_list = connection;
        }
    }

    addr_t address;
    memset(&address, 0, sizeof(address));
    address.sa.sa_family = AF_INET;
    address.sin.sin_port = htons(853);
    address.sin.sin_addr.s_addr = htonl(0x7f000001);  // localhost.
                                                      // tls, stream, stable, opportunistic
    dso_connection = ioloop_connection_create(&address, true,   true,   true, true,
                                              dso_datagram_callback, dso_connected, dso_disconnected, NULL, NULL);
    if (dso_connection == NULL) {
        ERROR("Unable to create dso connection.");
        exit(1);
    }
}

static void
usage(void)
{
    fprintf(stderr,
            "srp-client [--lease-time <seconds>] [--client-count <client count>] [--server <address>%%<port>]\n"
            "           [--push-query] [--push-unsubscribe]\n"
            "           [--bogus-server-test] [--change-txt-record] [--service-type] [--test-renew-subtypes]\n"
            "           [--random-leases] [--delete-registrations] [--use-thread-services] [--log-stderr]\n"
            "           [--interface <interface name>] [--bogusify-signatures] [--remove-added-service]\n"
            "           [--dup-instance-name] [--service-port <port number>] [--expire-added-service]\n"
            "           [--random-txt-record] [--bogus-remove] [--test-subtypes] [--test-diff-subtypes]\n"
            "           [--new-ip-dup] [--push-exhaust] [--test-bad-sig-time] [--zero-addresses]\n"
            "           [--host-only] [--expire-instance]");
    exit(1);
}


static void
cti_service_list_callback(void *UNUSED context, cti_service_vec_t *services, cti_status_t status)
{
    size_t i;

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        exit(1);
    }

    if (!new_ip_dup && !zero_addresses) {
        srp_start_address_refresh();
        ioloop_map_interface_addresses(NULL, interface_name, services, interface_callback);
    }
    for (i = 0; i < services->num; i++) {
        cti_service_t *cti_service = services->services[i];
        // Look for SRP service advertisements only.
        if (IS_SRP_SERVICE(cti_service)) {
            srp_add_server_address(&cti_service->server[16], dns_rrtype_aaaa, cti_service->server, 16);
        }
    }
    if (!new_ip_dup && !zero_addresses) {
        srp_finish_address_refresh(NULL);
    }
    srp_network_state_stable(NULL);
}

int
main(int argc, char **argv)
{

    uint8_t server_address[16] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 };
    uint8_t bogus_address[16] = { 0xfc,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 };
        // { 0x26, 0x20, 0x01, 0x49, 0x00, 0x0f, 0x1a, 0x4d, 0x04, 0xff, 0x61, 0x5a, 0xa2, 0x2a, 0xab, 0xe8 };
    int err;
    DNSServiceRef sdref;
    int *nump;
    char *end;
    (void)argc;
    (void)argv;
    int i;
    bool have_server_address = false;
    bool log_stderr = false;
    char instance_name[128];
    const char *service_type = "_ipps._tcp";
    uint16_t service_port = 0;
    bool bogus_server = false;

    ioloop_init();

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--lease-time")) {
            nump = &lease_time;
        number:
            if (i + 1 == argc) {
                usage();
            }
            *nump = (uint32_t)strtoul(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || end[0] != 0) {
                usage();
            }
            i++;
        } else if (!strcmp(argv[i], "--client-count")) {
            nump = &num_clients;
            goto number;
        } else if (!strcmp(argv[i], "--interface")) {
            if (i + 1 == argc) {
                usage();
            }
            interface_name = argv[i + 1];
            i++;
        } else if (!strcmp(argv[i], "--server")) {
            char *percent;
            uint8_t addrbuf[16];
            uint16_t addrtype = dns_rrtype_aaaa;
            int addrlen = 16;

            if (i + 1 == argc) {
                usage();
            }
            percent = strchr(argv[i + 1], '%');
            if (percent == NULL || percent[1] == 0) {
                usage();
            }
            *percent = 0;
            percent++;

            const unsigned long in_server_port = strtoul(percent, &end, 10);
            if (in_server_port > UINT16_MAX || end == percent || end[0] != 0) {
                usage();
            }
            uint8_t server_port[2];
            server_port[0] = ((uint16_t)in_server_port) >> 8;
            server_port[1] = ((uint16_t)in_server_port) & 255;

            if (inet_pton(AF_INET6, argv[i + 1], addrbuf) < 1) {
                if (inet_pton(AF_INET, argv[i + 1], addrbuf) < 1) {
                    usage();
                } else {
                    addrtype = dns_rrtype_a;
                    addrlen = 4;
                }
            }
            srp_add_server_address(server_port, addrtype, addrbuf, addrlen);
            have_server_address = true;
            i++;
        } else if (!strcmp(argv[i], "--random-leases")) {
            random_leases = true;
        } else if (!strcmp(argv[i], "--delete-registrations")) {
            delete_registrations = true;
        } else if (!strcmp(argv[i], "--use-thread-services")) {
            use_thread_services = true;
        } else if (!strcmp(argv[i], "--dup-instance-name")) {
            dup_instance_name = true;
        } else if (!strcmp(argv[i], "--new-ip-dup")) {
            new_ip_dup = true;
        } else if (!strcmp(argv[i], "--log-stderr")) {
            log_stderr = true;
            OPENLOG("srp-client", true);
        } else if (!strcmp(argv[i], "--change-txt-record")) {
            change_txt_record = true;
        } else if (!strcmp(argv[i], "--random-txt-record")) {
            random_txt_record = true;
        } else if (!strcmp(argv[i], "--remove-added-service")) {
            remove_added_service = true;
        } else if (!strcmp(argv[i], "--expire-added-service")) {
            let_added_service_expire = true;
        } else if (!strcmp(argv[1], "--bogus-server-test")) {
            bogus_server = true;
        } else if (!strcmp(argv[i], "--bogusify-signatures")) {
            bogusify_signatures = true;
        } else if (!strcmp(argv[i], "--bogus-remove")) {
            bogus_remove = true;
        } else if (!strcmp(argv[i], "--push-query")) {
            push_query = true;
            do_srp = false;
        } else if (!strcmp(argv[i], "--push-hardwired")) {
            push_query = true;
            push_hardwired = true;
            push_unsubscribe = true;
            do_srp = false;
        } else if (!strcmp(argv[i], "--push-unsubscribe")) {
            push_query = true;
            push_unsubscribe = true;
            do_srp = false;
        } else if (!strcmp(argv[i], "--push-send-bogus-keepalive")) {
            push_query = true;
            push_unsubscribe = true;
            push_send_bogus_keepalive = true;
            do_srp = false;
        } else if (!strcmp(argv[i], "--push-exhaust")) {
            push_exhaust = true;
            do_srp = false;
        } else if (!strcmp(argv[i], "--test-subtypes")) {
            test_subtypes = true;
        } else if (!strcmp(argv[i], "--test-diff-subtypes")) {
            test_diff_subtypes = true;
        } else if (!strcmp(argv[i], "--test-renew-subtypes")) {
            test_renew_subtypes = true;
        } else if (!strcmp(argv[i], "--host-only")) {
            host_only = true;
        } else if (!strcmp(argv[i], "--zero-addresses")) {
            zero_addresses = true;
        } else if (!strcmp(argv[i], "--expire-instance")) {
            expire_instance = true;
        } else if (!strcmp(argv[i], "--test-bad-sig-time")) {
            test_bad_sig_time = true;
        } else if (!strcmp(argv[i], "--service-type")) {
            if (i + 1 == argc) {
                usage();
            }
            service_type = argv[i + 1];
            i++;
        } else if (!strcmp(argv[i], "--service-port")) {
            if (i + 1 == argc) {
                usage();
            }

            const int in_service_port = atoi(argv[i + 1]);
            if (in_service_port == 0 || in_service_port > UINT16_MAX) {
                fprintf(stderr, "Service port number %d is out of range or invalid, should be in (0, 65535].\n",
                        in_service_port);
                usage();
            }

            service_port = (uint16_t)in_service_port;
            i++;
        } else {
            usage();
        }
    }

    if (!log_stderr) {
        OPENLOG("srp-client", false);
    }

    // If we're asked to do a push query, we're not actually going to act as an SRP client, just do the push query.
    if (push_query || push_exhaust) {
        start_push_query();
        ioloop();
        exit(1);
    }

    if (!use_thread_services && !new_ip_dup && !zero_addresses) {
        ioloop_map_interface_addresses(NULL, interface_name, NULL, interface_callback);
    }

    if (!have_server_address && !use_thread_services) {
        uint8_t port[] = { 0, 53 };
        if (bogus_server) {
            srp_add_server_address(port, dns_rrtype_aaaa, bogus_address, 16);
        }
        srp_add_server_address(port, dns_rrtype_aaaa, server_address, 16);
    }

    if (dup_instance_name) {
        num_clients = 2;
        ioloop_strcpy(instance_name, "dup-name-test", sizeof(instance_name));
    }
    if (new_ip_dup || zero_addresses) {
        // Set up the test to validate the "failed update removes address" code in srp-mdns-proxy.
        srp_add_interface_address(dns_rrtype_a, first_bogus_address, sizeof(first_bogus_address));
        if (zero_addresses) {
            srp_delete_interface_address(dns_rrtype_a, first_bogus_address, sizeof(first_bogus_address));
        }
    }

    for (i = 0; i < num_clients; i++) {
        srp_client_t *client;
        char hnbuf[128];
        TXTRecordRef txt;
        const void *txt_data = NULL;
        uint16_t txt_len = 0;
        char txt_buf[128];

        client = calloc(1, sizeof(*client));
        if (client == NULL) {
            ERROR("no memory for client %d", i);
            exit(1);
        }

        if (num_clients == 1) {
            strcpy(hnbuf, "srp-api-test");
        } else {
            snprintf(hnbuf, sizeof(hnbuf), "srp-api-test-%d", i);
        }
        client->name = strdup(hnbuf);
        if (client->name == NULL) {
            ERROR("No memory for client name %s", hnbuf);
            exit(1);
        }
        client->index = i;

        srp_host_init(client);
        srp_set_hostname(hnbuf, NULL);

        if (random_leases) {
            int random_lease_time = 30 + srp_random16() % 1800; // random
            INFO("Client %d, lease time = %d", i, random_lease_time);
            srp_set_lease_times(random_lease_time, 7 * 24 * 3600); // random host lease, 7 day key lease
        } else if (lease_time > 0) {
            srp_set_lease_times(lease_time, 7 * 24 * 3600); // specified host lease, 7 day key lease
        } else if (let_added_service_expire) {
            srp_set_lease_times(30, 30); // Use short lease times so the lease expires quickly.
        }

        if (change_txt_record) {
            TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
            TXTRecordSetValue(&txt, "foo", 1, "0");
            TXTRecordSetValue(&txt, "bar", 3, "1.1");
            txt_data = TXTRecordGetBytesPtr(&txt);
            txt_len = TXTRecordGetLength(&txt);
        }
        if (random_txt_record) {
            char rbuf[6];
            snprintf(rbuf, sizeof(rbuf), "%u", srp_random16());
            TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
            TXTRecordSetValue(&txt, "foo", strlen(rbuf), rbuf);
            INFO("TXTRecordSetValue(..., \"foo\", %zd, %s)", strlen(rbuf), rbuf);
            txt_data = TXTRecordGetBytesPtr(&txt);
            txt_len = TXTRecordGetLength(&txt);
        }

        if (service_port == 0) {
            // If no service port is specified (0 indicates that port is unspecified), the index i will be used to
            // generate the port number.
            service_port = (i % UINT16_MAX) == 0 ? 1 : (i % UINT16_MAX);
        }

        if (!test_subtypes && !test_diff_subtypes && !test_renew_subtypes && !host_only) {
            err = DNSServiceRegister(&sdref, new_ip_dup ? kDNSServiceFlagsNoAutoRename : 0, 0,
                                     dup_instance_name ? instance_name : hnbuf, service_type, 0, 0, htons(service_port),
                                     txt_len, txt_data, register_callback, client);
            if (err != kDNSServiceErr_NoError) {
                ERROR("DNSServiceRegister failed: %d", err);
                exit(1);
            }
        }
        if (remove_added_service || let_added_service_expire) {
            expecting_second_add = true;
            err = DNSServiceRegister(&sdref, 0, 0, hnbuf, "_second._tcp,foo", 0, 0, htons(service_port),
                                     txt_len, txt_data, second_register_callback, client);
            if (err != kDNSServiceErr_NoError) {
                ERROR("second DNSServiceRegister failed: %d", err);
                exit(1);
            }
        }
        // Here we register two services with subtypes. The idea is to see that the srp parsing code does not
        // associate the second subtype with the first service instance and report an error. In order to
        // attempt to trigger the error, we need the service instance name of the second service instance
        // to be different.
        if (test_subtypes || test_diff_subtypes || test_renew_subtypes) {
            expecting_second_add = true;
            err = DNSServiceRegister(&sdref, 0, 0, hnbuf, "_ipps._tcp,subtype",
                                     0, 0, htons(service_port), txt_len, txt_data, register_callback, client);
            if (err != kDNSServiceErr_NoError) {
                ERROR("DNSServiceRegister failed: %d", err);
                exit(1);
            }
            if (test_diff_subtypes) {
                err = DNSServiceRegister(&sdref, 0, 0, hnbuf, "_second._tcp,othersub",
                                         0, 0, htons(service_port), txt_len, txt_data, second_register_callback, client);
                if (err != kDNSServiceErr_NoError) {
                    ERROR("DNSServiceRegister failed: %d", err);
                    exit(1);
                }
            } else if (!test_renew_subtypes) {
                char shnbuf[132];
                snprintf(shnbuf, sizeof(shnbuf), "foo-%s", hnbuf);
                err = DNSServiceRegister(&sdref, 0, 0, shnbuf, "_ipps._tcp,othersub",
                                         0, 0, htons(service_port), txt_len, txt_data, second_register_callback, client);
                if (err != kDNSServiceErr_NoError) {
                    ERROR("DNSServiceRegister failed: %d", err);
                    exit(1);
                }
            }
        }
    }

    if (do_srp) {
        if (use_thread_services) {
            cti_get_service_list(NULL, &thread_service_context, NULL, cti_service_list_callback, NULL);
        } else {
            srp_network_state_stable(NULL);
        }
    }
    ioloop();
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
