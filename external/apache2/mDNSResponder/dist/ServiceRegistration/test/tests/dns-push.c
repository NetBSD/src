/* dns-push.c
 *
 * Copyright (c) 2024 Apple Inc. All rights reserved.
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
 * This file contains the SRP server test runner.
 */

#include "srp.h"
#include <dns_sd.h>
#include <arpa/inet.h>
#include "srp-test-runner.h"
#include "srp-api.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-mdns-proxy.h"
#include "test-api.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "test-dnssd.h"
#include "test.h"
#include "dnssd-proxy.h"
#define DNSMessageHeader dns_wire_t
#include "dso.h"
#include "dso-utils.h"

#define SUBSCRIBE_LIMIT 16 // SOA + SRV + A + AAAA || PTR + SRV + A + AAAA
#define TXN_LIMIT SUBSCRIBE_LIMIT * 4
typedef struct push_test_state push_test_state_t;
struct push_test_state {
    test_state_t *test_state;
    comm_t *dso_connection;
    wakeup_t *wait_for_remote_disconnect;
    dso_state_t *disconnect_expected;
    DNSServiceRef register_ref, ptr_sdref;
    DNSServiceRef txns[TXN_LIMIT];
    int num_txns;
    uint16_t subscribe_xids[SUBSCRIBE_LIMIT];
    int num_subscribe_xids, soa_index, ds_index[2];
    char *hostname;
    char *srv_name;
    int num_service_adds_pre, num_service_removes, num_service_adds_post;
    int num_address_adds_pre, num_address_removes, num_address_adds_post;
    uint16_t keepalive_xid;
    int variant, num_a_records, num_aaaa_records;
    int num_txt_records, num_srv_records;
    bool push_send_bogus_keepalive, push_unsubscribe;
    bool push_subscribe_sent, have_address_records;
    bool server_was_crashed, server_is_being_crashed;
    bool test_dns_push, have_keepalive_response, need_service;
};

static void test_dns_push_send_push_subscribe(push_test_state_t *push_state, const char *name, int rrtype);

static void
test_dns_push_dso_message_finished(void *context, message_t *UNUSED message, dso_state_t *dso)
{
    push_test_state_t *push_state = context;

    if (dso->primary.opcode == kDSOType_DNSPushUnsubscribe) {
        if (dso->activities == NULL) {
            dispatch_async(dispatch_get_main_queue(), ^{
                    TEST_PASSED(push_state->test_state);
                });
        }
    }
}

static void
test_dns_push_send_push_unsubscribe(push_test_state_t *push_state, int index)
{
    if (push_state->subscribe_xids[index] != 0) {
        struct iovec iov;
        dns_wire_t dns_message;
        uint8_t *buffer = (uint8_t *)&dns_message;
        dns_towire_state_t towire;
        dso_message_t message;

        INFO("unsubscribe %x %d", push_state->subscribe_xids[index], index);
        dso_make_message(&message, buffer, sizeof(dns_message), push_state->dso_connection->dso, true, false, 0, 0, NULL);
        memset(&towire, 0, sizeof(towire));
        towire.p = &buffer[DNS_HEADER_SIZE];
        towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
        towire.message = &dns_message;
        dns_u16_to_wire(&towire, kDSOType_DNSPushUnsubscribe);
        dns_rdlength_begin(&towire);
        dns_u16_to_wire(&towire, push_state->subscribe_xids[index]);
        dns_rdlength_end(&towire);

        memset(&iov, 0, sizeof(iov));
        iov.iov_len = towire.p - buffer;
        iov.iov_base = buffer;
        ioloop_send_message(push_state->dso_connection, NULL, &iov, 1);
        push_state->subscribe_xids[index] = 0; // Don't unsubscribe again.
    }
}

static void
test_dns_push_unsubscribe_all(push_test_state_t *push_state)
{
    struct iovec iov;
    INFO("unsubscribe");
    dns_wire_t dns_message;
    uint8_t *buffer = (uint8_t *)&dns_message;
    dns_towire_state_t towire;
    dso_message_t message;
    if (!push_state->push_send_bogus_keepalive) {
        for (int i = 0; i < push_state->num_subscribe_xids; i++) {
            test_dns_push_send_push_unsubscribe(push_state, i);
        }
    }

    // Send a keepalive message so that we can get the response, since the unsubscribe is not a response-requiring request.
    dso_make_message(&message, buffer, sizeof(dns_message), push_state->dso_connection->dso, false, false, 0, 0, NULL);
    memset(&towire, 0, sizeof(towire));
    towire.p = &buffer[DNS_HEADER_SIZE];
    towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
    towire.message = &dns_message;
    dns_u16_to_wire(&towire, kDSOType_Keepalive);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, 600);
    dns_u32_to_wire(&towire, 600);
    dns_rdlength_end(&towire);
    if (push_state->push_send_bogus_keepalive) {
        INFO("sending bogus keepalive");
        // Send a badly formatted message.
        dns_u32_to_wire(&towire, 0x12345678);
    }
    push_state->keepalive_xid = dns_message.id;
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - buffer;
    iov.iov_base = buffer;
    ioloop_send_message(push_state->dso_connection, NULL, &iov, 1);
}

static void
test_dns_push_remote_disconnect_didnt_happen(void *context)
{
    push_test_state_t *push_state = context;
    TEST_FAIL(push_state->test_state, "remote disconnect didn't happen");
}

static void
test_dns_push_handle_retry_delay(push_test_state_t *push_state, dso_state_t *dso, uint32_t delay)
{
    INFO("Got our retry delay, %ums...", delay);
    push_state->wait_for_remote_disconnect = ioloop_wakeup_create();

    TEST_FAIL_CHECK(push_state->test_state, push_state->wait_for_remote_disconnect != NULL, "can't wait for remote disconnect.");

    // Wait six seconds for remote disconnect, which should happen in five.
    ioloop_add_wake_event(push_state->wait_for_remote_disconnect, push_state, test_dns_push_remote_disconnect_didnt_happen, NULL, 6 * 1000);
    push_state->disconnect_expected = dso;
}

static void
test_dns_push_address_update(push_test_state_t *push_state, dns_rr_t *rr, const char *name)
{
    const char *record_name = "AAAA";
    char ntop[INET6_ADDRSTRLEN];
    int num;

    if (rr->type == dns_rrtype_a) {
        num = push_state->num_a_records;
        if (rr->ttl != 0xffffffff) {
            ++push_state->num_a_records;
        }
        inet_ntop(AF_INET, &rr->data.a, ntop, sizeof(ntop));
        record_name = "A";
    } else {
        num = push_state->num_aaaa_records;
        if (rr->ttl != 0xffffffff) {
            ++push_state->num_aaaa_records;
        }
        inet_ntop(AF_INET6, &rr->data.aaaa, ntop, sizeof(ntop));
    }
    INFO("%s: %s %s record #%d: %s", name, rr->ttl == 0xffffffff ? "removed" : "added", record_name, num, ntop);
}

static void
test_dns_push_update(push_test_state_t *push_state, dns_rr_t *rr)
{
    char name[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    dns_name_print(rr->name, name, sizeof(name));

    if (rr->type == dns_rrtype_soa) {
        TEST_FAIL_CHECK_STATUS(push_state->test_state, push_state->variant == PUSH_TEST_VARIANT_HARDWIRED,
                               "SOA received in wrong variant: %s", name);
        char soaname[DNS_MAX_NAME_SIZE_ESCAPED + 1];
        TEST_FAIL_CHECK_STATUS(push_state->test_state, !strcmp(name, "default.service.arpa."), "bad name for SOA: %s", name);
        dns_name_print(rr->data.soa.mname, soaname, sizeof(soaname));
        INFO("%s in SOA %s ...", name, soaname);
        // Look up the SRV record for _dns-push-tls._tcp.default.service.arpa, so that we can get the hostname but also
        // validate that the SRV record is being advertised.
        push_state->srv_name = strdup("_dns-push-tls._tcp.default.service.arpa.");
        test_dns_push_send_push_subscribe(push_state, push_state->srv_name, dns_rrtype_srv);
        test_dns_push_send_push_unsubscribe(push_state, push_state->soa_index);
    } else if (rr->type == dns_rrtype_a || rr->type == dns_rrtype_aaaa) {
        test_dns_push_address_update(push_state, rr, name);
        if (push_state->server_was_crashed) {
            if (rr->ttl == 0xffffffff) {
                push_state->num_address_removes++;
            } else {
                push_state->num_address_adds_post++;
            }
        } else {
            push_state->num_address_adds_pre++;
        }
    } else if (rr->type == dns_rrtype_ptr) {
        TEST_FAIL_CHECK_STATUS(push_state->test_state, push_state->variant != PUSH_TEST_VARIANT_HARDWIRED,
                               "PTR received in wrong variant: %s", name);
        TEST_FAIL_CHECK_STATUS(push_state->test_state, !strcmp(name, "_example._tcp.default.service.arpa."),
                               "bad name for PTR: %s", name);
        char ptrname[DNS_MAX_NAME_SIZE_ESCAPED + 1];
        dns_name_print(rr->data.ptr.name, ptrname, sizeof(ptrname));
        INFO("%s %s IN PTR %s", rr->ttl == 0xffffffff ? "removed" : "added", name, ptrname);
        if (push_state->srv_name == NULL) {
            push_state->srv_name = strdup(ptrname);
            test_dns_push_send_push_subscribe(push_state, push_state->srv_name, dns_rrtype_srv);
        }
        if (push_state->server_was_crashed) {
            if (rr->ttl == 0xffffffff) {
                push_state->num_service_removes++;
            } else {
                push_state->num_service_adds_post++;
            }
        } else {
            push_state->num_service_adds_pre++;
        }
    } else if (rr->type == dns_rrtype_srv) {
        char hnbuf[DNS_MAX_NAME_SIZE_ESCAPED + 1];
        dns_name_print(rr->data.ptr.name, hnbuf, sizeof(hnbuf));
        INFO("%s IN SRV %s ...", name, hnbuf);
        TEST_FAIL_CHECK_STATUS(push_state->test_state, !strcmp(name, push_state->srv_name), "bad name for SRV: %s", name);
        // Look up address records for SOA name server name.
        if (push_state->hostname == NULL) {
            push_state->hostname = strdup(hnbuf);
            TEST_FAIL_CHECK_STATUS(push_state->test_state, push_state->hostname != NULL, "no memory for %s", hnbuf);
            dispatch_async(dispatch_get_main_queue(), ^{
                test_dns_push_send_push_subscribe(push_state, push_state->hostname, dns_rrtype_a);
                });
            dispatch_async(dispatch_get_main_queue(), ^{
                test_dns_push_send_push_subscribe(push_state, push_state->hostname, dns_rrtype_aaaa);
            });
            // At this point the DS queries should have been started, so we can remove them and make sure that works.
            if (push_state->variant == PUSH_TEST_VARIANT_HARDWIRED) {
                test_dns_push_send_push_unsubscribe(push_state, push_state->ds_index[0]);
                test_dns_push_send_push_unsubscribe(push_state, push_state->ds_index[1]);
            }
        }
        push_state->num_srv_records++;
    } else if (rr->type == dns_rrtype_txt) {
        char txt_buf[DNS_DATA_SIZE];
        dns_txt_data_print(txt_buf, DNS_DATA_SIZE, rr->data.txt.len, rr->data.txt.data);
        INFO("%s IN TXT %s ...", name, txt_buf);
        push_state->num_txt_records++;
    } else {
        INFO("unexpected rrtype for %s in push update: %d", name, rr->type);
    }
}

static void
test_dns_push_send_appropriate_subscribe(push_test_state_t *push_state)
{
    if (push_state->variant == PUSH_TEST_VARIANT_HARDWIRED) {
        push_state->soa_index = push_state->num_subscribe_xids;
        test_dns_push_send_push_subscribe(push_state, "default.service.arpa", dns_rrtype_soa);
        push_state->ds_index[0] = push_state->num_subscribe_xids;
        test_dns_push_send_push_subscribe(push_state, "default.service.arpa", dns_rrtype_ds);
        push_state->ds_index[1] = push_state->num_subscribe_xids;
        test_dns_push_send_push_subscribe(push_state, "default.service.arpa", dns_rrtype_ds);
    } else {
        test_dns_push_send_push_subscribe(push_state, "_example._tcp.default.service.arpa", dns_rrtype_ptr);
    }
    push_state->push_subscribe_sent = true;
}

static void
test_dns_push_satisfied_check(push_test_state_t *push_state)
{
    // Check to see if we have all the address records we wanted.
    if (!push_state->have_address_records) {
        bool satisfied;
        switch(push_state->variant) {
        case PUSH_TEST_VARIANT_HARDWIRED:
            satisfied = push_state->num_a_records != 0 && push_state->num_aaaa_records != 0;
            break;
        case PUSH_TEST_VARIANT_DAEMON_CRASH:
        case PUSH_TEST_VARIANT_MDNS:
            satisfied = push_state->num_a_records != 0 || push_state->num_aaaa_records != 0;
            break;
        case PUSH_TEST_VARIANT_TWO_QUESTIONS:
            satisfied = push_state->num_srv_records > 0 && push_state->num_txt_records > 0;
            break;
        default:
            satisfied = false;
        }
        if (satisfied && push_state->variant == PUSH_TEST_VARIANT_DAEMON_CRASH) {
            if (!push_state->server_was_crashed) {
                // If the server hasn't been crashed yet
                // And we haven't already queued up a crash event
                if (!push_state->server_is_being_crashed) {
                    push_state->server_is_being_crashed = true;
                    // Queue up a crash event
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2),
                                   dispatch_get_main_queue(), ^{
                        push_state->server_was_crashed = true;
                        push_state->num_a_records = push_state->num_aaaa_records = 0;
                        dns_service_ref_t *ref = push_state->ptr_sdref;
                        TEST_FAIL_CHECK(push_state->test_state, ref != NULL, "ptr sdref is gone!");
                        for (int i = 0; i < push_state->num_txns; i++) {
                            DNSServiceRefDeallocate(push_state->txns[i]);
                        }
                        DNSServiceQueryRecordReply callback = ref->callback.query_record_reply;
                        callback(ref, 0, 0, kDNSServiceErr_ServiceNotRunning,
                                 NULL, dns_rrtype_ptr, dns_qclass_in, 0, 0, 0, ref->context);
                    });
                }
                satisfied = false;
            } else {
                if (push_state->num_address_removes != push_state->num_address_adds_pre) {
                    satisfied = false;
                }
                if (push_state->num_address_adds_pre != push_state->num_address_adds_post) {
                    satisfied = false;
                }
                INFO("address removes: %d address adds pre: %d address adds post: %d",
                     push_state->num_address_removes, push_state->num_address_adds_pre,
                     push_state->num_address_adds_post);
                if (push_state->num_service_removes != push_state->num_service_adds_pre) {
                    satisfied = false;
                }
                INFO("service removes: %d service adds pre: %d service adds post: %d",
                     push_state->num_service_removes, push_state->num_service_adds_pre,
                     push_state->num_service_adds_post);
            }
        }
        if (satisfied) {
            push_state->have_address_records = true;
            // If we've been asked to unsubscribe, do that.
            if (push_state->push_unsubscribe) {
                test_dns_push_unsubscribe_all(push_state);
            } else {
                // Finish any ongoing activities first...
                dispatch_async(dispatch_get_main_queue(), ^{
                    TEST_PASSED(push_state->test_state);
                });
            }
        }
    }
}

static void
test_dns_push_dns_response(push_test_state_t *push_state, message_t *message)
{
    unsigned offset, max;
    dns_rr_t rr;
    uint8_t *message_bytes;
    bool question = true;
    int rdata_num = 0;
    int num_answers = ntohs(message->wire.ancount);

    message_bytes = (uint8_t *)message->wire.data;
    offset = 0;
    max = message->length - DNS_HEADER_SIZE;
    int rr_index = 0;
    int qdcount = ntohs(message->wire.qdcount);
    while (offset < max) {
        INFO("%d %d", offset, max);
        if (rr_index >= qdcount) {
            question = false;
        }
        if (!dns_rr_parse(&rr, message_bytes, max, &offset, !question, true)) {
            TEST_FAIL_STATUS(push_state->test_state, "dns RR parse failed on rr %d", rr_index);
            break;
        }
        if (!question) {
            if (rdata_num < num_answers) {
                test_dns_push_update(push_state, &rr);
                rdata_num++;
            }
        }
        dns_name_free(rr.name);
        rr.name = NULL;
        dns_rrdata_free(&rr);
        rr_index++;
    }
    test_dns_push_satisfied_check(push_state);
}

static void
test_dns_push_dso_message(push_test_state_t *push_state, message_t *message, dso_state_t *dso, bool response)
{
    unsigned offset, max;
    dns_rr_t rr;
    uint8_t *message_bytes;

    switch(dso->primary.opcode) {
    case kDSOType_RetryDelay:
        if (response) {
            TEST_FAIL(push_state->test_state, "server sent a retry delay TLV as a response.");
        }
        dso_retry_delay(dso, &message->wire);
        break;

    case kDSOType_Keepalive:
        if (response) {
            TEST_FAIL_STATUS(push_state->test_state, "keepalive response from server, rcode = %d", dns_rcode_get(&message->wire));
        } else {
            INFO("Keepalive from server");
        }

        // We need to wait for the first keepalive response before sending a DNS push subscribe, since until we get
        // it we don't have a session. So this actually kicks off the first (possibly only) DNS Push subscribe in the
        // test.
        if (!push_state->push_subscribe_sent) {
            push_state->have_keepalive_response = true;
            if (!push_state->need_service) {
                test_dns_push_send_appropriate_subscribe(push_state);
            }
        }
        break;

    case kDSOType_DNSPushSubscribe:
        if (response) {
            // This is a protocol error--the response isn't supposed to contain a primary TLV.
            TEST_FAIL_STATUS(push_state->test_state,
                             "DNS Push response from server, rcode = %d", dns_rcode_get(&message->wire));
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
            test_dns_push_update(push_state, &rr);
            dns_name_free(rr.name);
            rr.name = NULL;
            dns_rrdata_free(&rr);
        }

        test_dns_push_satisfied_check(push_state);
        break;

    case kDSOType_NoPrimaryTLV: // No Primary TLV
        if (response) {
            bool subscribe_acked = false;
            for (int i = 0; i < push_state->num_subscribe_xids; i++) {
                if (message->wire.id == htons(push_state->subscribe_xids[i])) {
                    int rcode = dns_rcode_get(&message->wire);
                    INFO("DNS Push Subscribe response from server, rcode = %d", rcode);
                    if (rcode != dns_rcode_noerror) {
                        TEST_FAIL_STATUS(push_state->test_state, "subscribe for %x failed",
                                         push_state->subscribe_xids[i]);
                    }
                    subscribe_acked = true;
                }
            }
            if (subscribe_acked) {
            } else if (message->wire.id == push_state->keepalive_xid) {
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
        dso_simple_response(push_state->dso_connection, NULL, &message->wire, dns_rcode_dsotypeni);
        break;
    }
}

static void
test_dns_push_dso_event_callback(void *context, void *event_context, dso_state_t *dso, dso_event_type_t eventType)
{
    push_test_state_t *push_state = context;

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
        test_dns_push_dso_message(push_state, message, dso, false);
        break;
    case kDSOEventType_DSOResponse:
        INFO("DSO Response (Primary TLV=%d) received from " PRI_S_SRP,
               dso->primary.opcode, dso->remote_name);
        response_context = event_context;
        message = response_context->message_context;
        test_dns_push_dso_message(push_state, message, dso, true);
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
        if (dso == push_state->disconnect_expected) {
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
        if (!push_state->push_subscribe_sent && !push_state->need_service) {
            test_dns_push_send_appropriate_subscribe(push_state);
        } else {
            push_state->have_keepalive_response = true;
        }
        INFO("keepalive received.");
        break;
    case kDSOEventType_RetryDelay:
        disconnect_context = event_context;
        INFO("retry delay received, %d seconds", disconnect_context->reconnect_delay);
        test_dns_push_handle_retry_delay(push_state, dso, disconnect_context->reconnect_delay);
        break;
    }
}

static void
test_dns_push_send_push_subscribe(push_test_state_t *push_state, const char *name, int rrtype)
{
    struct iovec iov;

    dns_wire_t dns_message;
    uint8_t *buffer = (uint8_t *)&dns_message;
    dns_towire_state_t towire;
    dso_message_t message;
    int i = push_state->num_subscribe_xids;
    if (i >= SUBSCRIBE_LIMIT) {
        TEST_FAIL_STATUS(push_state->test_state, "subscribe xid limit reached: %d", i);
    }
    push_state->num_subscribe_xids++;

    if (push_state->test_dns_push) {
        // DNS Push subscription
        dso_make_message(&message, buffer, sizeof(dns_message), push_state->dso_connection->dso, false, false, 0, 0, NULL);

        push_state->subscribe_xids[i] = ntohs(dns_message.id);
        INFO("push subscribe for %s, rrtype %d, xid %x, num %d", name, rrtype, push_state->subscribe_xids[i], i);
        memset(&towire, 0, sizeof(towire));
        towire.p = &buffer[DNS_HEADER_SIZE];
        towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
        towire.message = &dns_message;
        dns_u16_to_wire(&towire, kDSOType_DNSPushSubscribe);
        dns_rdlength_begin(&towire);
        dns_full_name_to_wire(NULL, &towire, name);
        dns_u16_to_wire(&towire, rrtype);
        dns_u16_to_wire(&towire, dns_qclass_in);
        dns_rdlength_end(&towire);
    } else {
        // Regular DNS query
        memset(&dns_message, 0, sizeof(dns_message));
        dns_message.id = htons((uint16_t)srp_random16());
        dns_qr_set(&dns_message, 0); // query
        dns_opcode_set(&dns_message, dns_opcode_query);
        int num_questions;
        if (rrtype == dns_rrtype_srv && push_state->variant == PUSH_TEST_VARIANT_TWO_QUESTIONS) {
            num_questions = 2;
        } else {
            num_questions = 1;
        }
        dns_message.qdcount = htons(num_questions);
        memset(&towire, 0, sizeof(towire));
        towire.p = &buffer[DNS_HEADER_SIZE];
        towire.lim = towire.p + (sizeof(dns_message) - DNS_HEADER_SIZE);
        towire.message = &dns_message;
        dns_name_pointer_t np;
        dns_full_name_to_wire(&np, &towire, name);
        dns_u16_to_wire(&towire, rrtype);
        dns_u16_to_wire(&towire, dns_qclass_in);
        if (num_questions == 2) {
            dns_pointer_to_wire(NULL, &towire, &np);
            dns_u16_to_wire(&towire, dns_rrtype_txt);
            dns_u16_to_wire(&towire, dns_qclass_in);
        }
        push_state->subscribe_xids[i] = ntohs(dns_message.id);
        INFO("DNS query for %s, rrtype %d, xid %x, num %d", name, rrtype, push_state->subscribe_xids[i], i);
    }

    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - buffer;
    iov.iov_base = buffer;
    ioloop_send_message(push_state->dso_connection, NULL, &iov, 1);
}

static void
test_dns_push_connected(comm_t *connection, void *context)
{
    push_test_state_t *push_state = context;
    struct iovec iov;
    INFO("connected");
    connection->dso = dso_state_create(false, 3, connection->name, test_dns_push_dso_event_callback,
                                       push_state, NULL, push_state->dso_connection);
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
    ioloop_send_message(push_state->dso_connection, NULL, &iov, 1);
}

static void
test_dns_push_disconnected(comm_t *UNUSED connection, void *context, int UNUSED error)
{
    push_test_state_t *push_state = context;
    TEST_FAIL(push_state->test_state, "push server disconnect.");
}

static void
test_dns_push_datagram_callback(comm_t *connection, message_t *message, void *context)
{
    push_test_state_t *push_state = context;

    // If this is a DSO message, see if we have a session yet.
    switch(dns_opcode_get(&message->wire)) {
    case dns_opcode_query:
        test_dns_push_dns_response(push_state, message);
        return;
    case dns_opcode_dso:
        if (connection->dso == NULL) {
            INFO("dso message received with no DSO object on connection " PRI_S_SRP, connection->name);
            exit(1);
        }
        dso_message_received(connection->dso, (uint8_t *)&message->wire, message->length, message);
        return;
    }
    INFO("datagram on connection " PRI_S_SRP " not handled, type = %d.",
         connection->name, dns_opcode_get(&message->wire));
}

static void
test_dns_push_ready(void *context, uint16_t UNUSED port)
{
    push_test_state_t *push_state = context;
    test_state_t *state = push_state->test_state;

    addr_t address;
    memset(&address, 0, sizeof(address));
    address.sa.sa_family = AF_INET;
    address.sin.sin_port = htons(8530);
    address.sin.sin_addr.s_addr = htonl(0x7f000001);  // localhost.
                                                      // tls, stream, stable, opportunistic
    push_state->dso_connection = ioloop_connection_create(&address, true,   true,   true, true,
                                                          test_dns_push_datagram_callback, test_dns_push_connected,
                                                          test_dns_push_disconnected, NULL, push_state);
    TEST_FAIL_CHECK(state, push_state->dso_connection != NULL, "Unable to create dso connection.");
}

static bool
test_listen_longevity_dnssd_proxy_configure(void)
{
    dnssd_proxy_udp_port= 53000;
    dnssd_proxy_tcp_port = 53000;
    dnssd_proxy_tls_port = 8530;
    return true;
}

static bool
test_dns_push_query_callback_intercept(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                       DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype,
                                       uint16_t rrclass, uint16_t rdlen, const void *vrdata, uint32_t ttl, void *context)
{
    dns_service_ref_t *ref = sdRef;
    const uint8_t *rdata = vrdata;
    const uint8_t *new_rdata = rdata;
    uint8_t rdbuf[16];

    if (rrtype == dns_rrtype_a) {
        if (rdata[0] == 169 && rdata[1] == 254) {
            new_rdata = rdbuf;
            rdbuf[0] = 10;
            rdbuf[1] = 255;
            rdbuf[2] = rdata[2];
            rdbuf[3] = rdata[3];
        }
    } else if (rrtype == dns_rrtype_aaaa) {
        if (rdata[0] == 0xfe && rdata[1] == 0x80) {
            new_rdata = rdbuf;
            rdbuf[0] = 0xfc; // Change to unused 0xFC address space
            memcpy(&rdbuf[1], rdata + 1, 15); // Keep the rest.
        }
    }
    DNSServiceQueryRecordReply callback = ref->callback.query_record_reply;
    callback(ref, flags, interfaceIndex, errorCode, fullname, rrtype, rrclass, rdlen, new_rdata, ttl, context);
    return false;
}

static void test_dns_push_register_example_service(push_test_state_t *push_state);

static void
test_dns_push_register_callback(const DNSServiceRef UNUSED sd_ref, const DNSServiceFlags UNUSED flags,
                           const DNSServiceErrorType error, const char *const name, const char *const reg_type,
                           const char *const domain, void *const context)
{
    push_test_state_t *push_state = context;
    test_state_t *state = push_state->test_state;

    if (error == kDNSServiceErr_ServiceNotRunning) {
        INFO("example service deregistered due to server exit");
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 10), // 100ms
                       dispatch_get_main_queue(), ^{
            test_dns_push_register_example_service(push_state);
                       });
    } else {
        TEST_FAIL_CHECK_STATUS(state, error == kDNSServiceErr_NoError, "example service registration failed: %d", error);
        INFO("example service registered successfully -- %s.%s%s", name, reg_type, domain);
        if (push_state->need_service) {
            push_state->need_service = false;
            if (push_state->have_keepalive_response) {
                test_dns_push_send_appropriate_subscribe(push_state);
            }
        }
    }
}

static void
test_dns_push_register_example_service(push_test_state_t *push_state)
{
    uint8_t txt_data[] = {
        0x08, 0x53, 0x49, 0x49, 0x3D, 0x35, 0x30, 0x30, 0x30, 0x07,
        0x53, 0x41, 0x49, 0x3D, 0x33, 0x30, 0x30, 0x03, 0x54, 0x3D, 0x30 };

    int ret = DNSServiceRegister(&push_state->register_ref, 0, kDNSServiceInterfaceIndexAny, NULL,
                                 "_example._tcp", NULL, NULL, 12345, sizeof(txt_data), txt_data,
                                 test_dns_push_register_callback, push_state);
    TEST_FAIL_CHECK(push_state->test_state, ret == kDNSServiceErr_NoError, "failed to register example service.");
    DNSServiceSetDispatchQueue(push_state->register_ref, dispatch_get_main_queue());
}

static DNSServiceErrorType
test_dns_push_crash_intercept(test_state_t *state, DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                         const char *fullname, uint16_t rrtype, uint16_t rrclass, DNSServiceAttribute const *attr,
                         DNSServiceQueryRecordReply callBack, void UNUSED *context)
{
    dns_service_ref_t *ref = context;
    DNSServiceErrorType status = DNSServiceQueryRecordWithAttribute(sdRef, flags, interfaceIndex, fullname,
                                                                    rrtype, rrclass, attr, callBack, ref);
    // We're going to signal the daemon crash on the PTR query.
    push_test_state_t *push_state = state->context;
    if (status == kDNSServiceErr_NoError && rrtype == dns_rrtype_ptr) {
        push_state->ptr_sdref = ref;
    } else {
        if (push_state->num_txns < TXN_LIMIT) {
            push_state->txns[push_state->num_txns++] = ref;
        }
    }
    return status;
}

void
test_dns_push(test_state_t *next_test, int variant)
{
    extern srp_server_t *srp_servers;
    test_state_t *state = NULL;
    bool register_example_service = false;
    bool push = true;
    if (variant == PUSH_TEST_VARIANT_HARDWIRED) {
        const char *hardwired =
            "  The goal of this test is to create DNS Push connection to the test server and attempt to\n"
            "  look up a name that goes through the hardwired query path. If we get a response, the test\n"
            "  succeeded.";
        state = test_state_create(srp_servers, "DNS Push Hardwired test", NULL, hardwired, NULL);
    } else if (variant == PUSH_TEST_VARIANT_MDNS) {
        const char *mdns =
            "  The goal of this test is to create DNS Push connection to the test server and attempt to\n"
            "  look up a name that goes through the local (mDNS) query path. If we get a response, the test\n"
            "  succeeded.";
        state = test_state_create(srp_servers, "DNS Push Local test", NULL, mdns, NULL);
        register_example_service = true;
    } else if (variant == PUSH_TEST_VARIANT_DNS_MDNS) {
        const char *mdns =
            "  The goal of this test is to create DSO connection to the test server and send a DNS query to\n"
            "  look up a name that goes through the local (mDNS) query path. If we get a response, the test\n"
            "  succeeded.";
        state = test_state_create(srp_servers, "DNS Local test", NULL, mdns, NULL);
        register_example_service = true;
        push = false;
        variant = PUSH_TEST_VARIANT_MDNS;
    } else if (variant == PUSH_TEST_VARIANT_TWO_QUESTIONS) {
        const char *two =
            "  The goal of this test is to create DSO connection to the test server and send a DNS query with\n"
            "  two questions on the same name, for a TXT and an SRV record. We will register a matter service to\n"
            "  discover.  If we get a well-formed response with answers for both service types, the test\n"
            "  succeeded.";
        state = test_state_create(srp_servers, "DNS Local two-question test", NULL, two, NULL);
        register_example_service = true;
        push = false;
        variant = PUSH_TEST_VARIANT_TWO_QUESTIONS;
    } else if (variant == PUSH_TEST_VARIANT_DNS_HARDWIRED) {
        const char *hardwired =
            "  The goal of this test is to create DSO connection to the test server and send a DNS query that\n"
            "  looks up a name that goes through the hardwired query path. If we get a response, the test\n"
            "  succeeded.";
        state = test_state_create(srp_servers, "DNS query Hardwired test", NULL, hardwired, NULL);
        push = false;
        variant = PUSH_TEST_VARIANT_HARDWIRED;
    } else if (variant == PUSH_TEST_VARIANT_DNS_CRASH) {
        const char *crash =
        "  The goal of this test is to create DSO connection to the test server and attempt to\n"
        "  look up a name that goes through the local (mDNS) query path. Once we have a result, we fake\n"
        "  an mDNSResponder crash and make sure the query is successfully restarted.";
        state = test_state_create(srp_servers, "DNS query daemon crash test", NULL, crash, NULL);
        state->query_record_intercept = test_dns_push_crash_intercept;
        register_example_service = true;
        variant = PUSH_TEST_VARIANT_DAEMON_CRASH;
        push = false;
    } else if (variant == PUSH_TEST_VARIANT_DAEMON_CRASH) {
        const char *crash =
            "  The goal of this test is to create DNS Push connection to the test server and attempt to\n"
            "  look up a name that goes through the local (mDNS) query path. Once we have a result, we fake\n"
            "  an mDNSResponder crash and make sure the query is successfully restarted.";
        state = test_state_create(srp_servers, "DNS Push daemon crash test", NULL, crash, NULL);
        state->query_record_intercept = test_dns_push_crash_intercept;
        register_example_service = true;
    }
    state->next = next_test;
    push_test_state_t *push_state = calloc(1, sizeof (*push_state));
    TEST_FAIL_CHECK(state, push_state != NULL, "no memory for test-specific state.");
    state->context = push_state;
    push_state->test_state = state;
    push_state->variant = variant;
    push_state->test_dns_push = push;

    // Might as well always test
    push_state->push_unsubscribe = true;

    srp_test_dnssd_tls_listener_ready = test_dns_push_ready;
    srp_test_tls_listener_context = push_state;
    srp_test_dso_message_finished = test_dns_push_dso_message_finished;

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->dns_service_query_callback_intercept = test_dns_push_query_callback_intercept;
    state->dnssd_proxy_configurer = test_listen_longevity_dnssd_proxy_configure;
    TEST_FAIL_CHECK(state, init_dnssd_proxy(srp_servers), "failed to setup dnssd-proxy");

    if (register_example_service) {
        push_state->need_service = true;
        test_dns_push_register_example_service(push_state);
    }

    // Test should not take longer than ten seconds.
    srp_test_state_add_timeout(state, 20);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
