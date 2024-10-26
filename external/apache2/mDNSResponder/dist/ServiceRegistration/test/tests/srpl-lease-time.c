/* srpl-lease-time.c
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

#define LEASE_TIME 15

static void
test_srpl_lease_time_replication_evaluate(test_state_t *state, srp_server_t *server)
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

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, server), "unexpected dnssd transactions remain");

    // If so, the test has passed.
    TEST_PASSED(state);
}

static void
test_srpl_lease_time_replication_finished(srpl_connection_t *connection)
{
    test_state_t *state = connection->test_state;
    srp_server_t *server = connection->instance->domain->server_state;

    INFO("reach test_srpl_lease_time_replication_finished");
    // Wait for another LEASE_TIME to verify that the registration has the updated lease time
    // and does not expire immediately.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * LEASE_TIME), dispatch_get_main_queue(), ^{
        test_srpl_lease_time_replication_evaluate(state, server);
    });
}

static void
test_srpl_lease_time_primary_evaluate(test_state_t *state)
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
test_srpl_lease_time_callback(DNSServiceRef sdref, DNSServiceFlags UNUSED flags, DNSServiceErrorType errorCode,
                              const char *name, const char *regtype, const char *UNUSED domain, void *context)
{
    srp_server_t *server_state = context;
    test_state_t *state = server_state->test_state;
    static bool initial = true;

    INFO("Register Reply for %s . %s: %d", name, regtype, errorCode);
    INFO("state = %p", state);

    if (errorCode != kDNSServiceErr_NoError) {
        TEST_FAIL_STATUS(state, "registration failed: srp_client_register callback returned %d", errorCode);
    }
    // We only check the registration on the primary once and start the replication then.
    if (initial) {
        // Allow time for the mDNS registration to finish
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2), dispatch_get_main_queue(), ^{
                    // Verify the registration on the primary server.
                    test_srpl_lease_time_primary_evaluate(state);
                    // Change the lease time to 3600 seconds
                    srp_set_lease_times(3600, 3600);
                    // Wait for client to renew.
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * LEASE_TIME), dispatch_get_main_queue(), ^{
                                // After client renews, start another replication peer.
                                srp_server_t *second = test_srpl_add_server(state);
                                // create an outgoing connection from second to primary
                                srpl_connection_t *connection = test_srpl_connection_create(state, state->primary, second);
                                connection->srpl_advertise_finished_callback = test_srpl_lease_time_replication_finished;
                                // Start replication.
                                test_srpl_start_replication(state->primary, 12345);
                                srp_client_ref_deallocate(sdref);
                                srp_network_state_stable(NULL); // Discontinue SRP client
                    });
        });
        initial = false;
    }
}

static void
test_srpl_lease_time_ready(void *context, uint16_t UNUSED port)
{
    srp_server_t *state = context;

    int ret = srp_host_init(state);
    srp_set_lease_times(LEASE_TIME, LEASE_TIME);
    TEST_FAIL_CHECK(state->test_state, ret == kDNSServiceErr_NoError, "srp_host_init failed");

    DNSServiceRef ref;
    INFO("SRPL lease time test");
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
                              test_srpl_lease_time_callback, state);
    TEST_FAIL_CHECK_STATUS(state->test_state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    srp_test_network_localhost_start(state->test_state);
}

void
test_srpl_lease_time(test_state_t *next_state)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "    The goal of this test is to validate that lease time is processed correctly for a renewal.\n"
        "    The test first sets the lease time to 15 seconds and let the client register a host and\n"
        "    service with a srp server. It then updates the lease time on the client to 3600 seconds\n"
        "    and wait for the client to renew. It then starts a second SRP replication server and wait\n"
        "    for the synchronization to complete. It checks that the lease on the replicated peer does\n"
        "    not expire immediately.";
    test_state_t *state = test_state_create(srp_servers, "Replication lease time test", NULL, description, NULL);

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->primary->min_lease_time = LEASE_TIME;
    state->srp_listener = srp_proxy_listen(NULL, 0, NULL, test_srpl_lease_time_ready, NULL, NULL, NULL, state->primary);
    TEST_FAIL_CHECK(state, state->srp_listener != NULL, "listener create failed");

    state->next = next_state;

    // Test should not take longer than 3*LEASE_TIME.
    srp_test_state_add_timeout(state, 3 * LEASE_TIME);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
