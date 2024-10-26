/* change-text-record.c
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

static void
test_change_text_record_test_evaluate(test_state_t *state)
{
    dns_service_event_t *register_event = dns_service_find_first_register_event_by_name_and_type(state->primary,
                                                                                                 TEST_INSTANCE_NAME,
                                                                                                 TEST_SERVICE_TYPE);
    TEST_FAIL_CHECK(state, register_event != NULL, "failed to initially register service");
    dns_service_event_t *update_event = dns_service_find_update_for_register_event(state->primary,
                                                                                   register_event, NULL);
    TEST_FAIL_CHECK(state, update_event != NULL, "failed to correctly update service");
    // An update event with a NULL rdata is a TSR update, which isn't what we're looking for.
    while (update_event != NULL && update_event->rdata == NULL) {
        INFO("skipping TSR update");
        update_event = dns_service_find_update_for_register_event(state->primary, register_event, update_event);
    }
    TEST_FAIL_CHECK(state, update_event != NULL, "failed to correctly update service");
    TEST_PASSED(state);
}

static void
test_change_text_record_callback(DNSServiceRef sdref, DNSServiceFlags UNUSED flags, DNSServiceErrorType errorCode,
                                 const char *name, const char *regtype, const char *UNUSED domain, void *context)
{
    srp_server_t *server_state = context;
    test_state_t *state = server_state->test_state;
    static bool updated_text_record = false;

    INFO("Register Reply for %s . %s: %d", name, regtype, errorCode);
    INFO("state = %p", state);

    if (errorCode != kDNSServiceErr_NoError) {
        if (updated_text_record) {
            TEST_FAIL_STATUS(state, "text record update failed: srp_client_register callback returned %d", errorCode);
        } else {
            TEST_FAIL_STATUS(state, "initial registration failed: srp_client_register callback returned %d", errorCode);
        }
    }

    // if we have already updated text record, there's nothing to do;
    // otherwise, schedule a txt update.
    if (updated_text_record) {
        return;
    }

    // Allow time for the mDNS registration to finish
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2), dispatch_get_main_queue(), ^{
            char txt_buf[128];
            TXTRecordRef txt;

            TXTRecordCreate(&txt, sizeof(txt_buf), txt_buf);
            TXTRecordSetValue(&txt, "foo", 1, "1");
            TXTRecordSetValue(&txt, "bar", 3, "2.1");
            const char *txt_data = TXTRecordGetBytesPtr(&txt);
            int txt_len = TXTRecordGetLength(&txt);

            int ret = srp_client_update_record(sdref, NULL, 0, txt_len, txt_data, 0);
            TEST_FAIL_CHECK_STATUS(state, ret == kDNSServiceErr_NoError,
                                   "text record update failed: srp_client_update_record returned %d", ret);
            srp_network_state_stable(NULL);
            updated_text_record = true;

            // We will not get another callback because of a TXT record update unless it produces a conflict.
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2), dispatch_get_main_queue(), ^{
                    srp_client_ref_deallocate(sdref);
                    srp_network_state_stable(NULL); // Discontinue SRP client
                    // Allow any cleanup to happen and then evaluate results.
                    dispatch_async(dispatch_get_main_queue(), ^{
                            test_change_text_record_test_evaluate(state);
                        });
                });
        });
}

static void
test_change_text_record_ready(void *context, uint16_t UNUSED port)
{
    srp_server_t *state = context;

    int ret = srp_host_init(state);
    TEST_FAIL_CHECK(state->test_state, ret == kDNSServiceErr_NoError, "srp_host_init failed");

    DNSServiceRef ref;
    INFO("Change text record test");
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
                              test_change_text_record_callback, state);
    TEST_FAIL_CHECK_STATUS(state->test_state, ret == kDNSServiceErr_NoError, "srp_client_register returned %d", ret);
    srp_test_network_localhost_start(state->test_state);
}

void
test_change_text_record_start(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to see that when we add a service with SRP and then update the\n"
        "  service to change its text record, srp-mdns-proxy calls DNSServiceUpdateRecord rather than\n"
        "  calling DNSServiceRefDeallocate followed by DNSServiceRegister.";
    test_state_t *state = test_state_create(srp_servers, "Change Text Record test", NULL, description, NULL);

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->srp_listener = srp_proxy_listen(NULL, 0, NULL, test_change_text_record_ready, NULL, NULL, NULL, state->primary);
    TEST_FAIL_CHECK(state, state->srp_listener != NULL, "listener create failed");
    state->next = next_test;

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
