/* test-srpl.c
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
 * This file contains tools for SRP replication.
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
#include "test-srpl.h"
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

static bool tls_init = false;

static void
test_srpl_input(comm_t *comm, message_t *message, void *context)
{
    srp_server_t *server_state = context;
    dns_proxy_input_for_server(comm, server_state, message, NULL);
}

static void
test_srpl_listener_ready(void *context, uint16_t port)
{
    srp_server_t *server = context;
    srpl_connection_t *srpl_connection;
    srpl_instance_service_t *service;
    address_query_t *address_query;

    // listener is ready, so we start to connect on the outgoing connections
    for (srpl_connection = server->connections; srpl_connection != NULL; srpl_connection = srpl_connection->next)
    {
        srpl_instance_t *instance = srpl_connection->instance;
        service = calloc(1, sizeof(*service));
        RETAIN_HERE(service, srpl_instance_service);
        service->outgoing_port = port;
        address_query = calloc(1, sizeof(*address_query));
        RETAIN_HERE(address_query, address_query);
        inet_pton(AF_INET, "127.0.0.1", &address_query->addresses[0].sin.sin_addr);
        address_query->addresses[0].sa.sa_family = AF_INET;
        address_query->num_addresses = 1;
        address_query->cur_address = -1;
        service->address_query = address_query;
        service->instance = instance;
        RETAIN_HERE(instance, srpl_instance);
        service->domain = instance->domain;
        RETAIN_HERE(service->domain, srpl_domain);
        instance->services = service;
        srpl_connection_next_state(srpl_connection, srpl_state_next_address_get);
        address_query->cur_address = -1;
    }
}

static void
test_srpl_connection_connected(comm_t *connection, void *context)
{
    srp_server_t *server = context;
    connection->srp_server = server;
}

srp_server_t *
test_srpl_add_server(test_state_t *state)
{
    static int server_id = 1;
    srp_server_t *server = server_state_create("srp-mdns-proxy-2",
                                               3600 * 27,     // max lease time one day plus 20%
                                               30,            // min lease time 30 seconds
                                               3600 * 24 * 7, // max key lease 7 days
                                               30);           // min key lease time 30s
    server->advertise_interface = if_nametoindex("lo0"); // Test is local to the device.
    server->test_state = state;
    server->srp_replication_enabled = true;
    srp_test_enable_stub_router(state, server);
    server->server_id = server_id;
    server_id++;
    return server;
}

static srpl_instance_t*
test_srpl_instance_create(test_state_t *state, srp_server_t *server)
{
    srpl_instance_t *instance = calloc(1, sizeof (*instance));
    TEST_FAIL_CHECK(state, instance != NULL, "no memory for instance");
    instance->instance_name = strdup("single-srpl-instance");
    TEST_FAIL_CHECK(state, instance->instance_name != NULL, "no memory for instance name");
    instance->domain = srpl_domain_create_or_copy(server, "openthread.thread.home.arpa");
    TEST_FAIL_CHECK(state, instance->domain != NULL, "no domain created");
    instance->domain->srpl_opstate = SRPL_OPSTATE_ROUTINE;
    instance->have_partner_id = true;
    server->current_thread_domain_name = strdup(instance->domain->name);
    instance->domain->instances = instance;
    RETAIN_HERE(instance->domain, srpl_domain);
    RETAIN_HERE(instance->domain->instances, srpl_instance);
    return instance;
}

static srpl_connection_t *
test_srpl_instance_connection_create(test_state_t *state, srp_server_t *server)
{
    srpl_instance_t *instance = test_srpl_instance_create(state, server);
    TEST_FAIL_CHECK(state, instance != NULL, "no memory for instance");

    srpl_connection_t *srpl_connection = srpl_connection_create(instance, true);
    TEST_FAIL_CHECK(state, srpl_connection != NULL, "srpl_connection_create failed");
    srpl_connection->state = srpl_state_connecting;
    instance->connection = srpl_connection;
    RETAIN_HERE(instance->connection, srpl_connection);

    srpl_connection->test_state = state;
    return srpl_connection;
}

// create outgoing connection from client to server. It'll connect once the listener
// on the server side is ready.
srpl_connection_t *
test_srpl_connection_create(test_state_t *state, srp_server_t *server, srp_server_t *client)
{
    srpl_connection_t *connection = test_srpl_instance_connection_create(state, client);
    connection->next = server->connections;
    connection->server = server;
    server->connections = connection;
    return connection;
}

// server starts listener and gets ready for srpl connection
void
test_srpl_start_replication(srp_server_t *server, int16_t port)
{
    test_state_t *state = server->test_state;
    // create a domain, and an instance and tie them together. Also create a address_query so that
    // the incoming connection can be recognized.
    srpl_instance_t *instance = test_srpl_instance_create(state, server);
    TEST_FAIL_CHECK(state, instance != NULL, "no memory for instance");
    srpl_instance_service_t *service = calloc(1, sizeof (*service));
    TEST_FAIL_CHECK(state, service != NULL, "no memory for service");
    service->instance = instance;
    RETAIN_HERE(instance, srpl_instance);
    instance->services = service;
    RETAIN_HERE(service, srpl_instance_service);

    // create an address_query for local loopback address so that the incoming
    // connection can be recognized.
    addr_t addr;
    memset(&addr, 0, sizeof(addr));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin.sin_addr);
    addr.sa.sa_family = AF_INET;
    addr.sin.sin_port = htons(port);
    addr.sa.sa_len = sizeof(addr.sin);
    address_query_t *address_query = calloc(1, sizeof (*address_query));
    TEST_FAIL_CHECK(state, address_query != NULL, "no memory for address query");
    address_query->num_addresses = 1;
    address_query->cur_address = 0;
    memcpy(&address_query->addresses[0], &addr, sizeof(addr));
    service->address_query = address_query;
    RETAIN_HERE(address_query, address_query);
    instance->domain->srpl_opstate = SRPL_OPSTATE_ROUTINE;
    // we intentionaly pick the smallest partner id so that it would not reject
    // any incoming connection.
    instance->domain->partner_id = 0;

    if (!tls_init) {
        bool succeeded = srp_tls_init();
        TEST_FAIL_CHECK(state, succeeded, "srp_tls_init failed");
        tls_init = true;
    }
    server->srpl_listener = ioloop_listener_create(true, true, false, NULL, 0, &addr, NULL, "SRPL Listener",
                                                   test_srpl_input, test_srpl_connection_connected, NULL,
                                                   test_srpl_listener_ready, NULL, srp_tls_configure, 0, server);
    TEST_FAIL_CHECK(state, server->srpl_listener != NULL, "no memory for listener");
    server->srpl_listener->srp_server = server;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
