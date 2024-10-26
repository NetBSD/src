/* single-srp-update.c
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
#include "srp-proxy.h"
#include "dns-msg.h"

static void
test_single_srpl_update_advertise_finished(test_state_t *state)
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

    // If so, the test has passed.
    TEST_PASSED(state);
}

void
test_single_srpl_update(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to generate a DNS Update message and deliver it through the SRP replication\n"
        "  input path. We then intercept the update finished event and check the results.";
	test_state_t *state = test_state_create(srp_servers, "Single SRPL Update test", NULL, description, NULL);

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    srp_servers->srp_replication_enabled = true;
    state->next = next_test;

	// Set up a SRP client state so that we can generate packets.
	state->test_packet_state = test_packet_state_create(state, test_single_srpl_update_advertise_finished);

	// Create a service to add to the packet.
    DNSServiceRef ref;
    char txt_buf[128];
    TXTRecordRef txt;

    TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
    TXTRecordSetValue(&txt, "foo", 1, "1");
    TXTRecordSetValue(&txt, "bar", 3, "1.1");
    const char *txt_data = TXTRecordGetBytesPtr(&txt);
    int txt_len = TXTRecordGetLength(&txt);

    // Create a DNSSD client
    int ret = srp_client_register(&ref, 0 /* flags */, 0 /* interfaceIndex */,
                                  TEST_INSTANCE_NAME /* name */, TEST_SERVICE_TYPE /* regType */,
                                  NULL /* domain */, TEST_HOST_NAME /* host */, 1234, txt_len, txt_data,
                                  (DNSServiceRegisterReply)1, state);
    TEST_FAIL_CHECK_STATUS(state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    test_packet_generate(state, 10, 10, false, false);

    test_packet_start(state, false);

    // Test should not take longer than ten seconds.
    srp_test_state_add_timeout(state, 10);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
