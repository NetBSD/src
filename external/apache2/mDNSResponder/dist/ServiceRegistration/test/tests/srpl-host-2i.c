/* srpl-host-2i.c
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
 * Test: Single host registration, two instances, with variants
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

static DNSServiceRef
test_srpl_host_2i_add_instance(test_state_t *state, int num)
{
	// Create a service to add to the packet.
    DNSServiceRef ref;
    char txt_buf[128];
    TXTRecordRef txt;

    TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
    TXTRecordSetValue(&txt, "blaznorf", 1, "1");
    TXTRecordSetValue(&txt, "fnord", 3, "1.1");
    const char *txt_data = TXTRecordGetBytesPtr(&txt);
    int txt_len = TXTRecordGetLength(&txt);

    const char *instance_name = num ? num == 1 ? TEST_INSTANCE_NAME_2 : TEST_INSTANCE_NAME_3 : TEST_INSTANCE_NAME;

    // Create a DNSSD client
    int ret = srp_client_register(&ref, 0 /* flags */, 0 /* interfaceIndex */,
                                  instance_name /* name */, TEST_SERVICE_TYPE /* regType */,
                                  NULL /* domain */, TEST_HOST_NAME /* host */, 1234, txt_len, txt_data,
                                  (DNSServiceRegisterReply)1, state);
    TEST_FAIL_CHECK_STATUS(state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    return ref;
}

static void
test_srpl_host_2i_check_instance(test_state_t *state, const char *instance_name)
{
    dns_service_event_t *reg =
        dns_service_find_first_register_event_by_name_and_type(state->primary, instance_name, TEST_SERVICE_TYPE);
    TEST_FAIL_CHECK_STATUS(state, reg != NULL, "didn't find register event for " PUB_S_SRP, instance_name);
    TEST_FAIL_CHECK_STATUS(state, reg->status == kDNSServiceErr_NoError,
                           "DNSServiceRegister for " PUB_S_SRP " failed when it should have succeeded.", instance_name);
    reg->consumed = true;

    dns_service_event_t *regcb = dns_service_find_callback_for_registration(state->primary, reg);
    TEST_FAIL_CHECK_STATUS(state, regcb != NULL,
                           "no register callback for registered service " PUB_S_SRP, instance_name);
    TEST_FAIL_CHECK_STATUS(state, regcb->status == kDNSServiceErr_NoError,
                           "DNSServiceRegister callback for " PUB_S_SRP " got an error when it should not have.",
                           instance_name);
    regcb->consumed = true;
}

static void
test_srpl_host_2i_advertise_finished(test_state_t *state)
{
    static bool once = false;
    static bool twice = false;
    // dns_service_dump_unexpected_events(state, state->primary);

    if (!once) {
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

        // Check for the two instances.
        test_srpl_host_2i_check_instance(state, TEST_INSTANCE_NAME);
        test_srpl_host_2i_check_instance(state, TEST_INSTANCE_NAME_2);
    } else if (twice && (state->variant == DUP_TEST_VARIANT_ADD_FIRST ||
                         state->variant == DUP_TEST_VARIANT_ADD_LAST))
    {
        // Check for a DNSServiceRegister on the third test service instance name and service type.
        test_srpl_host_2i_check_instance(state, TEST_INSTANCE_NAME_3);
        twice = false;
    }

    // We should now have consumed all of the events.
    TEST_FAIL_CHECK(state, dns_service_dump_unexpected_events(state, state->primary), "unexpected dnssd transactions remain");

    if (state->variant != DUP_TEST_VARIANT_NO_DUP && !once) {
        once = true;
        switch(state->variant) {
        case DUP_TEST_VARIANT_TWO_KEYS:
        case DUP_TEST_VARIANT_BOTH:
            break;
        case DUP_TEST_VARIANT_FIRST:
            test_packet_message_delete(state, 1);
            break;
        case DUP_TEST_VARIANT_LAST:
            test_packet_message_delete(state, 0);
            break;
        case DUP_TEST_VARIANT_ADD_FIRST:
            test_srpl_host_2i_add_instance(state, 2);
            test_packet_generate(state, 10, 10, false, true);
            twice = true;
            break;
        case DUP_TEST_VARIANT_ADD_LAST:
            test_srpl_host_2i_add_instance(state, 2);
            test_packet_generate(state, 10, 10, false, true);
            twice = true;
            break;
        }
        test_packet_start(state, false);
        return;
    }

    // If so, the test has passed.
    TEST_PASSED(state);
}

void
test_srpl_host_2i(test_state_t *next_test, int variant)
{
    extern srp_server_t *srp_servers;
    const char *variant_info = NULL;
    const char *variant_name = NULL;
    switch(variant) {
    case DUP_TEST_VARIANT_TWO_KEYS:
        variant_name = "two keys";
        variant_info = "Test that if the two updates are signed with different keys, they are rejected.";
        break;
    case DUP_TEST_VARIANT_BOTH:
        variant_name = "both";
        variant_info = "Test that if the two updates contain the same messages, the second set are ignored.";
        break;
    case DUP_TEST_VARIANT_NO_DUP:
        variant_name = "no dup";
        variant_info = "Test without a duplicate update.";
        break;
    case DUP_TEST_VARIANT_FIRST:
        variant_name = "first";
        variant_info = "Test that if the second updates contain only the first message, it is ignored.";
        break;
    case DUP_TEST_VARIANT_LAST:
        variant_name = "last";
        variant_info = "Test that if the second updates contain only the last message, it is ignored.";
        break;
    case DUP_TEST_VARIANT_ADD_FIRST:
        variant_name = "third instance first";
        variant_info = "Test that if the second update starts with a new third instance, it is added.";
        break;
    case DUP_TEST_VARIANT_ADD_LAST:
        variant_name = "third instance last";
        variant_info = "Test that if the second update ends with a new third instance, it is added.";
        break;
    default:
        TEST_FAIL_STATUS(NULL, "invalid variant: %d", variant);
    }
    const char *explanation =
        "  The goal of this test is to generate two DNS Update messages, both for the \n"
        "  same host, each containing a new registration for a different instance, and \n"
        "  deliver it through the SRP replication input path. Having passed the update set once\n"
        "  we then deliver it a second time to see that it is correctly ignored.";
	test_state_t *state = test_state_create(srp_servers, "SRPL two-instance double-replication test",
                                            variant_name, explanation, variant_info);

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->variant = variant;
    srp_servers->srp_replication_enabled = true;
    state->next = next_test;

	// Set up a SRP client state so that we can generate packets.
	state->test_packet_state = test_packet_state_create(state, test_srpl_host_2i_advertise_finished);

    DNSServiceRef ref = test_srpl_host_2i_add_instance(state, 0);
    test_packet_generate(state, 10, 10, false, false);

    // This will mean that the subsequent update no longer contains the first instance. The instance is
    // not actually deleted as a result of this.
    srp_client_ref_deallocate(ref);

    // If we want to test that updates with different keys are rejected
    bool expect_fail = false;
    if (variant == DUP_TEST_VARIANT_TWO_KEYS) {
        test_packet_reset_key(state);
        expect_fail = true;
    }

	// Create a service to add to the packet.
    test_srpl_host_2i_add_instance(state, 1);;
    test_packet_generate(state, 10, 10, false, false);

    test_packet_start(state, expect_fail);

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
