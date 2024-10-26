/* srpl-cycle-through-peers.c
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
#include "srp-replication.h"
#include "test-packet.h"
#include "test-srpl.h"
#include "srp-proxy.h"
#include "dns-msg.h"
#include "srp-tls.h"

#define LEASE_TIME 40
#define NUM_SRP_SERVERS 4

static int num_srp_servers = 1;

static void
test_srpl_cycle_through_peers_evaluate(test_state_t *state, srp_server_t *server)
{
    // Check for a DNSServiceRegosterRecord for both an AAAA record and a KEY record, we don't care which order.
    dns_service_event_t *regrec_1 =
        dns_service_find_first_register_record_event_by_name(server, TEST_HOST_NAME_REGISTERED);
    TEST_FAIL_CHECK(state, regrec_1 != NULL, "replication:found zero register record events");
    regrec_1->consumed = true;
    dns_service_event_t *regrec_2 =
        dns_service_find_first_register_record_event_by_name(server, TEST_HOST_NAME_REGISTERED);
    TEST_FAIL_CHECK(state, regrec_1 != NULL, "found only one register record event");
    regrec_2->consumed = true;
    TEST_FAIL_CHECK(state,
                    ((regrec_1->rrtype == dns_rrtype_aaaa && regrec_2->rrtype == dns_rrtype_key) ||
                     (regrec_2->rrtype == dns_rrtype_aaaa && regrec_1->rrtype == dns_rrtype_key)),
                    "didn't find a KEY and an AAAA record register event");
    TEST_FAIL_CHECK(state, regrec_1->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord failed when it should have succeeded.");
    TEST_FAIL_CHECK(state, regrec_2->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord failed when it should have succeeded.");

    // Check for a DNSServiceRegister on the test service instance name and service type.
    dns_service_event_t *reg =
        dns_service_find_first_register_event_by_name_and_type(server, TEST_INSTANCE_NAME, TEST_SERVICE_TYPE);
    TEST_FAIL_CHECK(state, reg != NULL, "didn't find a register event");
    TEST_FAIL_CHECK(state, reg->status == kDNSServiceErr_NoError,
                    "DNSServiceRegister failed when it should have succeeded.");
    reg->consumed = true;

    dns_service_event_t *rrcb_1 = dns_service_find_callback_for_registration(server, regrec_1);
    TEST_FAIL_CHECK(state, rrcb_1 != NULL, "no register record callback for first register record");
    TEST_FAIL_CHECK(state, rrcb_1->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord callback got an error when it should not have.");
    rrcb_1->consumed = true;
    dns_service_event_t *rrcb_2 = dns_service_find_callback_for_registration(server, regrec_2);
    TEST_FAIL_CHECK(state, rrcb_2 != NULL, "no register record callback for second register record");
    TEST_FAIL_CHECK(state, rrcb_2->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord callback got an error when it should not have.");
    rrcb_2->consumed = true;

    dns_service_event_t *regcb = dns_service_find_callback_for_registration(server, reg);
    TEST_FAIL_CHECK(state, regcb != NULL, "no register callback for registered service");
    TEST_FAIL_CHECK(state, regcb->status == kDNSServiceErr_NoError,
                    "DNSServiceRegister callback got an error when it should not have.");
    regcb->consumed = true;

    // Check for remove record
    dns_service_event_t *remove_rec1 = dns_service_find_remove_for_register_event(server,
                                                                                  regrec_1, NULL);
    TEST_FAIL_CHECK(state, remove_rec1 != NULL, "found zero remove record events");
    remove_rec1->consumed = true;
    dns_service_event_t *remove_rec2 = dns_service_find_remove_for_register_event(server,
                                                                                  regrec_2, NULL);
    TEST_FAIL_CHECK(state, remove_rec2 != NULL, "found one remove record events");
    remove_rec2->consumed = true;

    dns_service_event_t *remove_instance = dns_service_find_ref_deallocate_event(server);
    TEST_FAIL_CHECK(state, remove_instance != NULL, "found zero remove instance events");
    remove_instance->consumed = true;

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, server), "unexpected dnssd transactions remain");

    // If so, the test has passed.
    TEST_PASSED(state);
}

static void
test_srpl_cycle_through_peers_replication_finished(srpl_connection_t *connection);

static void
test_srpl_next_srp_server(test_state_t *state, srp_server_t *prev)
{
    srp_server_t *new = test_srpl_add_server(state);
    // create an outgoing connection from new server to previous
    srpl_connection_t *connection = test_srpl_connection_create(state, prev, new);
    INFO("create connectionn from new %p to prev %p", new, prev);
    connection->srpl_advertise_finished_callback = test_srpl_cycle_through_peers_replication_finished;
    // Start replication on the server side (start listener etc).
    test_srpl_start_replication(prev, 12345 + num_srp_servers);
    num_srp_servers++;
    INFO("%d srp servers started", num_srp_servers);
}

static void
test_srpl_cycle_through_peers_replication_finished(srpl_connection_t *connection)
{
    test_state_t *state = connection->test_state;
    srp_server_t *client = connection->instance->domain->server_state;
    srp_server_t *server = connection->server;

    if (num_srp_servers < NUM_SRP_SERVERS) {
        // stop listener on the server of the srpl connection
        ioloop_comm_cancel(server->srpl_listener);
        // after finish synchronizing, discontinue the old peer.
        srpl_shutdown(client);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 10), dispatch_get_main_queue(), ^{
                    // add a new srp server in 10 seconds
                    test_srpl_next_srp_server(state, client);
        });
    } else {
        // if this is the last srp server, schedule to check the results in 10 seconds
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 10), dispatch_get_main_queue(), ^{
                    test_srpl_cycle_through_peers_evaluate(state, client);
        });
    }
}

static void
test_srpl_cycle_through_peers_primary_evaluate(test_state_t *state)
{
    // Check for a DNSServiceRegosterRecord for both an AAAA record and a KEY record, we don't care which order.
    dns_service_event_t *regrec_1 =
        dns_service_find_first_register_record_event_by_name(state->primary, TEST_HOST_NAME_REGISTERED);
    TEST_FAIL_CHECK(state, regrec_1 != NULL, "found zero register record events");
    regrec_1->consumed = true;
    dns_service_event_t *regrec_2 =
        dns_service_find_first_register_record_event_by_name(state->primary, TEST_HOST_NAME_REGISTERED);
    TEST_FAIL_CHECK(state, regrec_1 != NULL, "found only one register record event");
    regrec_2->consumed = true;
    TEST_FAIL_CHECK(state,
                    ((regrec_1->rrtype == dns_rrtype_aaaa && regrec_2->rrtype == dns_rrtype_key) ||
                     (regrec_2->rrtype == dns_rrtype_aaaa && regrec_1->rrtype == dns_rrtype_key)),
                    "didn't find a KEY and an AAAA record register event");
    TEST_FAIL_CHECK(state, regrec_1->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord failed when it should have succeeded.");
    TEST_FAIL_CHECK(state, regrec_2->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord failed when it should have succeeded.");

    // Check for a DNSServiceRegister on the test service instance name and service type.
    dns_service_event_t *reg =
        dns_service_find_first_register_event_by_name_and_type(state->primary, TEST_INSTANCE_NAME, TEST_SERVICE_TYPE);
    TEST_FAIL_CHECK(state, reg != NULL, "didn't find a register event");
    TEST_FAIL_CHECK(state, reg->status == kDNSServiceErr_NoError,
                    "DNSServiceRegister failed when it should have succeeded.");
    reg->consumed = true;

    dns_service_event_t *rrcb_1 = dns_service_find_callback_for_registration(state->primary, regrec_1);
    TEST_FAIL_CHECK(state, rrcb_1 != NULL, "no register record callback for first register record");
    TEST_FAIL_CHECK(state, rrcb_1->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord callback got an error when it should not have.");
    rrcb_1->consumed = true;
    dns_service_event_t *rrcb_2 = dns_service_find_callback_for_registration(state->primary, regrec_2);
    TEST_FAIL_CHECK(state, rrcb_2 != NULL, "no register record callback for second register record");
    TEST_FAIL_CHECK(state, rrcb_2->status == kDNSServiceErr_NoError,
                    "DNSServiceRegisterRecord callback got an error when it should not have.");
    rrcb_2->consumed = true;

    dns_service_event_t *regcb = dns_service_find_callback_for_registration(state->primary, reg);
    TEST_FAIL_CHECK(state, regcb != NULL, "no register callback for registered service");
    TEST_FAIL_CHECK(state, regcb->status == kDNSServiceErr_NoError,
                    "DNSServiceRegister callback got an error when it should not have.");
    regcb->consumed = true;

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, state->primary), "unexpected dnssd transactions remain");
}

static void
test_srpl_cycle_through_peers_callback(DNSServiceRef sdref, DNSServiceFlags UNUSED flags, DNSServiceErrorType errorCode,
                                       const char *name, const char *regtype, const char *UNUSED domain, void *context)
{
    srp_server_t *server_state = context;
    test_state_t *state = server_state->test_state;

    INFO("Register Reply for %s . %s: %d", name, regtype, errorCode);
    INFO("state = %p", state);

    if (errorCode != kDNSServiceErr_NoError) {
        TEST_FAIL_STATUS(state, "registration failed: srp_client_register callback returned %d", errorCode);
    }
    // Allow time for the mDNS registration to finish
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2), dispatch_get_main_queue(), ^{
                // Verify the registration on the primary server.
                test_srpl_cycle_through_peers_primary_evaluate(state);
                void *io_context = state->current_io_context;
                TEST_FAIL_CHECK(state, io_context != NULL, "NULL io_context");
                srp_cancel_wakeup(server_state, io_context);// cancel the lease renewal
                srp_client_ref_deallocate(sdref);
                // Wait to start a new replication server.
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 10), dispatch_get_main_queue(), ^{
                            test_srpl_next_srp_server(state, state->primary);
                });
    });
}

static void
test_srpl_cycle_through_peers_ready(void *context, uint16_t UNUSED port)
{
    srp_server_t *state = context;

    int ret = srp_host_init(state);
    srp_set_lease_times(LEASE_TIME, LEASE_TIME);
    TEST_FAIL_CHECK(state->test_state, ret == kDNSServiceErr_NoError, "srp_host_init failed");

    DNSServiceRef ref;
    INFO("SRPL cycle through peers test");
    char txt_buf[128];
    TXTRecordRef txt;

    TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
    TXTRecordSetValue(&txt, "foo", 1, "1");
    TXTRecordSetValue(&txt, "bar", 3, "1.1");
    const char *txt_data = TXTRecordGetBytesPtr(&txt);
    int txt_len = TXTRecordGetLength(&txt);

    // Create a DNSSD client
    ret = srp_client_register(&ref, 0 /* flags */, 0 /* interfaceIndex */,
                              TEST_INSTANCE_NAME /* name */, TEST_SERVICE_TYPE /* regType */,
                              NULL /* domain */, TEST_HOST_NAME /* host */, 1234, txt_len, txt_data,
                              test_srpl_cycle_through_peers_callback, state);
    TEST_FAIL_CHECK_STATUS(state->test_state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    srp_test_network_localhost_start(state->test_state);
}

void
test_srpl_cycle_through_peers(test_state_t *next_state)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "    The goal of this test is to validate that lease time is not accidentally extended because\n"
        "    of replication. The test first starts a single SRP server and registers a host and service\n"
        "    with a lease time of 40 seconds. Then every 10 seconds, it starts a new SRP server and after\n"
        "    the new server finsihes synchronization it discontinue the previous server. After the lease\n"
        "    time, it checks that the registration is no longer present on the last SRP server.";
    test_state_t *state = test_state_create(srp_servers, "Replication cycle through peers test", NULL, description, NULL);

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->primary->min_lease_time = LEASE_TIME;
    state->srp_listener = srp_proxy_listen(NULL, 0, NULL, test_srpl_cycle_through_peers_ready, NULL, NULL, NULL, state->primary);
    TEST_FAIL_CHECK(state, state->srp_listener != NULL, "listener create failed");

    state->next = next_state;

    // Test should not take longer than LEASE_TIME + 20.
    srp_test_state_add_timeout(state, LEASE_TIME + 20);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
