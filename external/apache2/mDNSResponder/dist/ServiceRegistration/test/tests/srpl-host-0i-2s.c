/* srpl-host-0i-2s.c
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

static void
test_host_0i2s_test_finished(srpl_connection_t *connection)
{
    test_state_t *state = connection->test_state;
    srp_server_t *server = connection->instance->domain->server_state;
    // Check for a DNSServiceRegosterRecord for both an AAAA record and a KEY record, we don't care which order.
    dns_service_event_t *regrec_1 =
        dns_service_find_first_register_record_event_by_name(server, TEST_HOST_NAME_REGISTERED);
    TEST_FAIL_CHECK(state, regrec_1 != NULL, "found zero register record events");
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

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, server), "unexpected dnssd transactions remain");

    // If so, the test has passed.
    TEST_PASSED(state);
}

static void
test_srpl_host_0i2s_primary_evaluate(test_state_t *state)
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

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, state->primary), "unexpected dnssd transactions remain");
}

static void
test_srpl_host_0i2s_ready(void *context, uint16_t UNUSED port)
{
    srp_server_t *state = context;
    int ret = srp_host_init(state);
    TEST_FAIL_CHECK(state->test_state, ret == kDNSServiceErr_NoError, "srp_host_init failed");
    srp_set_hostname(TEST_HOST_NAME, NULL);
    srp_test_network_localhost_start(state->test_state);
    // Allow time for the client to register
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2), dispatch_get_main_queue(), ^{
        test_srpl_host_0i2s_primary_evaluate(state->test_state);
        test_srpl_start_replication(state, 12345);
    });
}


void
test_srpl_host_0i2s(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to verify that a host registration with no instance doesn’t\n"
        "  cause a problem either initially or when replicated. A SRP client first registers a\n"
        "  host without instances with the primary SRP server. The update message is then delivered\n"
        "  to the other server through SRPL connection and the host is then registered on that server.";
    test_state_t *state = test_state_create(srp_servers, "SRPL zero-instance test", NULL, description, NULL);
    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    srp_servers->srp_replication_enabled = true;
    state->next = next_test;
    srp_servers->server_id = 0;
    srp_server_t *second = test_srpl_add_server(state);
    // create an outgoing connection from secondary to primary
    srpl_connection_t *connection = test_srpl_connection_create(state, state->primary, second);
    connection->srpl_advertise_finished_callback = test_host_0i2s_test_finished;

    state->srp_listener = srp_proxy_listen(NULL, 0, NULL, test_srpl_host_0i2s_ready, NULL, NULL, NULL, state->primary);
    TEST_FAIL_CHECK(state, state->srp_listener != NULL, "listener create failed");
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
