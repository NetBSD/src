/* multi-host-record.c
 *
 * Copyright (c) 2023 Apple Inc. All rights reserved.
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
 * This file contains a test for validating the correct propagation of multiple host records in srp replication.
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
test_multi_host_record_continue(test_state_t *state)
{
    TEST_PASSED(state);
}

void
test_multi_host_record_start(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to see that when we add a service with SRP and then update the\n"
        "  service to change its text record, the resulting state is accurately replicated to an SRP\n"
        "  replication peer. The test succeeds if, on the peer, we see that the host and the instance\n"
        "  refer to different records.";
    test_state_t *state = test_state_create(srp_servers,
                                            "Multiple Host Record Replication test", NULL, description, NULL);

    state->next = next_test;
    state->continue_testing = test_multi_host_record_continue;
    test_change_text_record_start(state);

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
