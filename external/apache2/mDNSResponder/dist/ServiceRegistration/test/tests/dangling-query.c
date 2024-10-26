/* dangling-query.c
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
#include "dnssd-proxy.h"

static bool
test_dns_dangling_query_response_intercept(comm_t *UNUSED connection, message_t *UNUSED responding_to,
                                           struct iovec *UNUSED iov, int UNUSED iov_len, bool UNUSED final,
                                           bool UNUSED send_length)
{
    test_state_t *state = connection->test_context;
    if (state->counter == 0) {
        state->counter = 1;
        TEST_FAIL_CHECK(state, state->context != NULL && state->context != last_freed_domain,
                        "domain was freed after the first query finished.");
        return true;
    }
    dispatch_async(dispatch_get_main_queue(), ^{
            TEST_FAIL_CHECK(state, state->context != NULL && state->context == last_freed_domain,
                            "domain wasn't freed last.");
            TEST_PASSED(state);
        });
    return true;
}

static void
test_dns_dangling_query_ready(void *context, uint16_t UNUSED port)
{
    test_state_t *state = context;

    // Hook our callback in to the srp input connection.
    state->srp_listener->test_send_intercept = test_dns_dangling_query_response_intercept;
    state->srp_listener->test_context = state;

    // Generate a DNS query that's in the infrastructure pseudo interface domain.
    message_t *message = ioloop_message_create(DNS_HEADER_SIZE + 256); // should be plenty of room
    TEST_FAIL_CHECK(state, message != NULL, "no memory for DNS query");
    dns_wire_t *wire = &message->wire;
    dns_towire_state_t towire;
    memset(&towire, 0, sizeof(towire));

    towire.p = &wire->data[0];               // We start storing RR data here.
    towire.lim = &wire->data[DNS_DATA_SIZE]; // This is the limit to how much we can store.
    towire.message = wire;

    wire->id = srp_random16();
    wire->bitfield = 0;
    dns_qr_set(wire, dns_qr_query);
    dns_opcode_set(wire, dns_opcode_query);
    wire->qdcount = htons(1);

    // A browse query that probably will get at least one answer
    dns_full_name_to_wire(NULL, &towire, "_airplay._tcp.local.");
    dns_u16_to_wire(&towire, dns_rrtype_ptr);
    dns_u16_to_wire(&towire, dns_qclass_in);
    // No ttl or length or rdata because question

    dns_proxy_input_for_server(state->srp_listener, state->primary, message, NULL);

    // second query
    towire.p = &wire->data[0];               // We start storing RR data here.
    towire.lim = &wire->data[DNS_DATA_SIZE]; // This is the limit to how much we can store.
    towire.message = wire;

    wire->id = srp_random16();
    wire->bitfield = 0;
    // A browse query that probably will get at least one answer
    dns_full_name_to_wire(NULL, &towire, "_companion-link._tcp.local.");
    dns_u16_to_wire(&towire, dns_rrtype_ptr);
    dns_u16_to_wire(&towire, dns_qclass_in);
    // No ttl or length or rdata because question

    dns_proxy_input_for_server(state->srp_listener, state->primary, message, NULL);

    state->context = delete_served_domain_by_interface_name(INFRASTRUCTURE_PSEUDO_INTERFACE);
    TEST_FAIL_CHECK(state, state->context != NULL && state->context != last_freed_domain,
                    "domain was freed immediately (way too soon).");
}

static bool
test_listen_dangling_dnssd_proxy_configure(void)
{
    dnssd_proxy_udp_port= 5300;
    dnssd_proxy_tcp_port = 5300;
    dnssd_proxy_tls_port = 8530;
    return true;
}

void
test_dns_dangling_query(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    const char *description =
        "  The goal of this test is to create an interface-specific served domain, start two queries in that\n"
		"  domain, then delete the domain, then wait for the query to finish. This should not (obvsly)\n"
        "  crash. In addition, when the queries finish, the test domain should be freed.";
    test_state_t *state = test_state_create(srp_servers, "Dangling Query test", NULL, description, NULL);
    state->next = next_test;

    srp_proxy_init("local");
    srp_test_enable_stub_router(state, srp_servers);
    state->dnssd_proxy_configurer = test_listen_dangling_dnssd_proxy_configure;
    TEST_FAIL_CHECK(state, init_dnssd_proxy(srp_servers), "failed to setup dnssd-proxy");

    // Start the srp listener.
    state->srp_listener = srp_proxy_listen(NULL, 0, NULL, test_dns_dangling_query_ready, NULL, NULL, NULL, state);

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
