/* test-packet.c
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
 * This file contains tools for generating SRP update packets that can be fed directly into the
 * parser (srp-parse.c) rather than involving connections, using the pathway that's used by SRP
 * replication. This is useful for unit tests generally, and particularly for testing the multi-packet
 * functionality used by SRP replication.
 */

#include <dns_sd.h>
#include "srp.h"
#include "srp-test-runner.h"
#include "srp-api.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-proxy.h"
#include "srp-dnssd.h"
#include "srp-mdns-proxy.h"
#include "test-api.h"
#include "srp-replication.h"
#include "test-packet.h"
#include "test-dnssd.h"
#include "dnssd-proxy.h"
#include "srp-tls.h"
#include <arpa/inet.h>

#define MAX_PACKETS 10
typedef struct test_packet_state test_packet_state_t;
struct test_packet_state {
    test_state_t *test_state;
    client_state_t *current_client;
    srpl_connection_t *srpl_connection;
    message_t *packets[MAX_PACKETS];
    int num_packets;
};

test_packet_state_t *
test_packet_state_create(test_state_t *state, void (*advertise_finished_callback)(test_state_t *state))
{
    test_packet_state_t *packet_state = calloc(1, sizeof(*packet_state));
    TEST_FAIL_CHECK(state, state != NULL, "unable to allocate test packet state");
    packet_state->test_state = state;
    srp_host_init(state);
    packet_state->current_client = srp_client_get_current();

    // We need to add at least one address record.
    srp_test_set_local_example_address(state);

    // We need an SRPL connection to capture the advertise_finished event. It needs to look real enough that
    // the event gets delivered, so we have to create a domain and an instance and tie them together, and then
    // hang the connection off of the instance.
    srpl_instance_t *instance = calloc(1, sizeof (*instance));
    TEST_FAIL_CHECK(state, instance != NULL, "no memory for instance");
    instance->instance_name = strdup("single-srpl-instance");
    TEST_FAIL_CHECK(state, instance->instance_name != NULL, "no memory for instance name");
    instance->domain = srpl_domain_create_or_copy(state->primary, "openthread.thread.home.arpa");
    instance->domain->srpl_opstate = SRPL_OPSTATE_ROUTINE;
    TEST_FAIL_CHECK(state, instance->domain != NULL, "no domain created");
    instance->domain->instances = instance;
    RETAIN_HERE(instance->domain, srpl_domain);
    RETAIN_HERE(instance->domain->instances, srpl_instance);

    srpl_connection_t *srpl_connection = srpl_connection_create(instance, false);
    TEST_FAIL_CHECK(state, srpl_connection != NULL, "srpl_connection_create failed");
    srpl_connection->state = srpl_state_test_event_intercept;
    instance->connection = srpl_connection;
    RETAIN_HERE(instance->connection, srpl_connection);

    srpl_connection->test_state = state;
    srpl_connection->advertise_finished_callback = advertise_finished_callback;
    packet_state->srpl_connection = srpl_connection;
    RETAIN_HERE(packet_state->srpl_connection, srpl_connection);

    return packet_state;
}

void
test_packet_generate(test_state_t *state, uint32_t host_lease, uint32_t key_lease, bool removing, bool prepend)
{
    test_packet_state_t *packet_state = state->test_packet_state;
    message_t *message = ioloop_message_create(sizeof(dns_wire_t));
    size_t length = message->length;
    message->received_time = srp_time();
    message->lease = host_lease;
    message->key_lease = key_lease;
    dns_wire_t *ret = srp_client_generate_update(packet_state->current_client, host_lease, key_lease,
                                                 &length, &message->wire, 9999, removing);
    TEST_FAIL_CHECK(state, length <= message->length, "srp_client_generate overflowed message length");
    TEST_FAIL_CHECK(state, ret != NULL, "srp_client_generate returned NULL");
    TEST_FAIL_CHECK(state, packet_state->num_packets < MAX_PACKETS, "more than maximum number of packets");
    if (prepend) {
        for (int i = packet_state->num_packets; i > 0; i--) {
            packet_state->packets[i] = packet_state->packets[i - 1];
        }
        packet_state->packets[0] = message;
    } else {
        packet_state->packets[packet_state->num_packets] = message;
    }
    packet_state->num_packets++;
}

void
test_packet_reset_key(test_state_t *state)
{
    test_packet_state_t *packet_state = state->test_packet_state;
    srp_host_key_reset_for_client(packet_state->current_client);
}

void
test_packet_start(test_state_t *state, bool expect_fail)
{
    test_packet_state_t *packet_state = state->test_packet_state;
    bool success = srp_parse_host_messages_evaluate(state->primary, packet_state->srpl_connection,
                                                    packet_state->packets, packet_state->num_packets);
    TEST_FAIL_CHECK_STATUS(state, expect_fail != success, "srp_parse_host_messages_evaluate returned " PUB_S_SRP,
                           success ? "true" : "false");
    if (expect_fail) {
        TEST_PASSED(state);
    }
}

bool
test_packet_srpl_intercept(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    if (event->event_type == srpl_event_advertise_finished) {
        test_state_t *state = srpl_connection->test_state;
        srpl_connection->advertise_finished_callback(state);
    }
    return false;
}

void
test_packet_message_delete(test_state_t *test_state, int index)
{
    test_packet_state_t *state = test_state->test_packet_state;
    if (index < state->num_packets) {
        ioloop_message_release(state->packets[index]);
        int j = index;
        for (int i = index + 1; i < state->num_packets; i++) {
            state->packets[j++] = state->packets[i];
        }
        state->num_packets--;
    }
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
