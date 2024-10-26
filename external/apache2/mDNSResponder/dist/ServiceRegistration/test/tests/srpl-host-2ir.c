/* srpl-host-2ir.c
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
 * Test: Single host registration, two instances
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
test_srpl_host_2ir_advertise_finished(test_state_t *state)
{
    // dns_service_dump_unexpected_events(state, state->primary);

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

    // We shouldn't see a register for the first instance, because it was removed subsequently.

    // We should see a register for the second instance.
    dns_service_event_t *reg2 =
        dns_service_find_first_register_event_by_name_and_type(state->primary, TEST_INSTANCE_NAME_2, TEST_SERVICE_TYPE);
    TEST_FAIL_CHECK(state, reg2 != NULL, "didn't find second register event");
    TEST_FAIL_CHECK(state, reg2->status == kDNSServiceErr_NoError,
                    "second DNSServiceRegister failed when it should have succeeded.");
    reg2->consumed = true;

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

    dns_service_event_t *regcb2 = dns_service_find_callback_for_registration(state->primary, reg2);
    TEST_FAIL_CHECK(state, regcb2 != NULL, "no register callback for second registered service");
    TEST_FAIL_CHECK(state, regcb2->status == kDNSServiceErr_NoError,
                    "second DNSServiceRegister callback got an error when it should not have.");
    regcb2->consumed = true;

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, state->primary), "unexpected dnssd transactions remain");

    // If so, the test has passed.
    TEST_PASSED(state);
}

void
test_srpl_host_2ir(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to generate two DNS Update messages, both for the \n"
        "  same host. The first contains two instances. The second contains a remove for\n"
        "  one of the instances. The test then delivers both messages through the SRP\n"
        "  replication input path. We then intercept the update finished event and check\n"
        "  the results.";

    test_state_t *state = test_state_create(srp_servers, "SRPL two instance add, one instance remove test",
                                            NULL, description, NULL);
    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    srp_servers->srp_replication_enabled = true;
    state->next = next_test;

	// Set up a SRP client state so that we can generate packets.
	state->test_packet_state = test_packet_state_create(state, test_srpl_host_2ir_advertise_finished);

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

	// Create a service to add to the packet.
    DNSServiceRef ref2;
    char txt_buf2[128];
    TXTRecordRef txt2;

    TXTRecordCreate(&txt2, sizeof(txt_buf2), txt_buf2);
    TXTRecordSetValue(&txt2, "blaznorf", 1, "1");
    TXTRecordSetValue(&txt2, "fnord", 3, "1.1");
    const char *txt_data2 = TXTRecordGetBytesPtr(&txt2);
    int txt_len2 = TXTRecordGetLength(&txt2);

    // Create a DNSSD client
    ret = srp_client_register(&ref2, 0 /* flags */, 0 /* interfaceIndex */,
                              TEST_INSTANCE_NAME_2 /* name */, TEST_SERVICE_TYPE /* regType */,
                              NULL /* domain */, TEST_HOST_NAME /* host */, 1234, txt_len2, txt_data2,
                              (DNSServiceRegisterReply)1, state);
    TEST_FAIL_CHECK_STATUS(state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    test_packet_generate(state, 10, 10, false, false);

	// Now remove a single instance.
	srp_deregister_instance(ref);
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
