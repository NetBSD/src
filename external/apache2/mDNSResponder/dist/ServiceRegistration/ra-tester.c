/* ra-tester.c
 *
 * Copyright (c) 2020-2022 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * This is a standalone tester for the Thread Border Router code that configures and advertises routes. The
 * idea is to be able to test the configuration/control functionality without setting up a Thread network.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <dns_sd.h>
#include <net/if.h>

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "dnssd-proxy.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "config-parse.h"
#include "route.h"

static void
usage(void)
{
    ERROR("ra-tester -t <thread interface name> --h <home interface name>");
    exit(1);
}

#ifdef FUZZING
#define main ra_tester_main
#endif

int
main(int argc, char **argv)
{
    int i;
    bool log_stderr = true;

    srp_server_t *server_state = calloc(1, sizeof(*server_state));
    if (server_state == NULL) {
        ERROR("no memory for server_state");
        return 1;
    }
    server_state->name = strdup("ra-tester");
    server_state->route_state = route_state_create(server_state, "ra-tester");
    if (server_state->route_state == NULL) {
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t")) {
            if (i + 1 == argc) {
                usage();
            }
            server_state->route_state->thread_interface_name = argv[i + 1];
            i++;
        } else if (!strcmp(argv[i], "-h")) {
            if (i + 1 == argc) {
                usage();
            }
            server_state->route_state->home_interface_name = argv[i + 1];
            i++;
        } else {
            usage();
        }
    }

    if (server_state->route_state->thread_interface_name == NULL) {
        INFO("thread interface name required.");
        usage();
    }
    if (server_state->route_state->home_interface_name == NULL) {
        INFO("home interface name required.");
        usage();
    }
    OPENLOG("ra-tester", log_stderr);

    if (!ioloop_init()) {
        return 1;
    }

    if (!start_icmp_listener()) {
        return 1;
    }

    infrastructure_network_startup(server_state->route_state);

    do {
        int something = 0;
        ioloop();
        INFO("dispatched %d events.", something);
    } while (1);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
