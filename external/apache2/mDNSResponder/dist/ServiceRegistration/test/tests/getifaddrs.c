/* getifaddrs.c
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
 * This file contains unit tests for ioloop_map_interface_addresses().
 */

#include "srp.h"
#include <dns_sd.h>
#include <arpa/inet.h>
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
#define DNSMessageHeader dns_wire_t
#include "dso.h"
#include "dso-utils.h"

#define SUBSCRIBE_LIMIT 4 // SOA + SRV + A + AAAA || PTR + SRV + A + AAAA
typedef struct ifaddr_test_state ifaddr_test_state_t;
struct ifaddr_test_state {
    test_state_t *test_state;
    struct ifaddrs *real_ifaddrs;
    bool lladdr_removed, lladdr_added;
    interface_address_state_t *ifaddr_state;
};

static void
test_ifaddrs_changed_l2addr(srp_server_t UNUSED *server_state, void *context, const char UNUSED *name,
                            const addr_t *address, const addr_t UNUSED *netmask, uint32_t UNUSED flags,
                            enum interface_address_change event_type)
{
    ifaddr_test_state_t *its = context;
    if (address->sa.sa_family == AF_LINK) {
        if (event_type == interface_address_deleted) {
            its->lladdr_removed = true;
        } else if (event_type == interface_address_added) {
            its->lladdr_added = true;

        }
    }
}

static void
test_ifaddrs_ignore(srp_server_t UNUSED *server_state, void UNUSED *context, const char UNUSED *name,
                    const addr_t UNUSED *address, const addr_t UNUSED *netmask, uint32_t UNUSED flags,
                    enum interface_address_change UNUSED event_type)
{
}

static int
test_ifaddrs_get(srp_server_t UNUSED *server_state, struct ifaddrs **ifaddrs, void *context)
{
    ifaddr_test_state_t *its = context;
    *ifaddrs = its->real_ifaddrs;
    return 0;
}

static void
test_ifaddrs_free(srp_server_t UNUSED *server_state, struct ifaddrs UNUSED *ifaddrs, void UNUSED *context)
{
}

void
test_ifaddrs_start(test_state_t *next_test)
{
    extern srp_server_t *srp_servers;
    test_state_t *state = NULL;
    const char *summary =
        "  The goal of this test is to exercise the various behaviors of ioloop_map_interface_addresses when\n"
        "  used to track the coming and going of interfaces and addresses on interfaces.\n";
    state = test_state_create(srp_servers, "ioloop_map_interface_addresses test", NULL, summary, NULL);
    state->next = next_test;
    state->getifaddrs = test_ifaddrs_get;
    state->freeifaddrs = test_ifaddrs_free;

    ifaddr_test_state_t *its = calloc(1, sizeof (*its));
    TEST_FAIL_CHECK(state, its != NULL, "no memory for test-specific state.");
    its->test_state = state;

    srp_proxy_init("local");

    TEST_FAIL_CHECK_STATUS(state, getifaddrs(&its->real_ifaddrs) == 0, "getifaddrs failed: %s", strerror(errno));
    TEST_FAIL_CHECK(state, ioloop_map_interface_addresses_here(srp_servers, &its->ifaddr_state, NULL, its, test_ifaddrs_ignore), "initial map call failed");
    for (struct ifaddrs *ifp = its->real_ifaddrs; ifp != NULL; ifp = ifp->ifa_next) {
        if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifp->ifa_addr;
            LLADDR(sdl)[0] = ~LLADDR(sdl)[0];
        }
    }
    TEST_FAIL_CHECK(state, ioloop_map_interface_addresses_here(srp_servers, &its->ifaddr_state, NULL, its, test_ifaddrs_changed_l2addr), "changed_l2addr map call failed");
    TEST_FAIL_CHECK(state, its->lladdr_removed && its->lladdr_added, "link local address wasn't updated");
    TEST_PASSED(state);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
