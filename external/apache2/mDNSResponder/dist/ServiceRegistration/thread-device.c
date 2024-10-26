/* thread-device.c
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
 * Functionality required to get a Thread device that is already connected to a Thread
 * mesh to act as an SRP server.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <dns_sd.h>
#include <net/if.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <netinet/icmp6.h>
#include <os/feature_private.h>
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "thread-device.h"
#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"
#include "service-publisher.h"
#include "thread-tracker.h"
#include "node-type-tracker.h"
#include "dnssd-client.h"

static void
thread_device_rloc16_callback(void *context, uint16_t rloc16, cti_status_t status)
{
    srp_server_t *server_state = context;

    if (status != kCTIStatus_NoError) {
        ERROR("rloc16 get failed with status %d", status);
    } else {
        bool start = false;
        server_state->rloc16 = rloc16;
        INFO("server_state->rloc16 updated to %d", server_state->rloc16);

        server_state->srp_on_demand = os_feature_enabled(mDNSResponder, srp_on_demand);
        INFO("srp on demand is " PUB_S_SRP, server_state->srp_on_demand ? "enabled" : "disabled");

        // Now we can start.
        if (server_state->service_tracker == NULL) {
            server_state->service_tracker = service_tracker_create(server_state);
            if (server_state->service_tracker == NULL) {
                FAULT("can't create service tracker.");
                return;
            }
            start = true;
        }
        if (server_state->thread_tracker == NULL) {
            server_state->thread_tracker = thread_tracker_create(server_state);
            if (server_state->thread_tracker == NULL) {
                FAULT("can't create thread tracker.");
                return;
            }
            start = true;
        }
        if (server_state->node_type_tracker == NULL) {
            server_state->node_type_tracker = node_type_tracker_create(server_state);
            if (server_state->node_type_tracker == NULL) {
                FAULT("can't create node type tracker.");
                return;
            }
            start = true;
        }
        if (server_state->service_publisher == NULL) {
            server_state->service_publisher = service_publisher_create(server_state);
            if (server_state->service_publisher == NULL) {
                FAULT("can't create service publisher.");
                return;
            }
            start = true;
        }
        if (server_state->dnssd_client == NULL) {
            server_state->dnssd_client = dnssd_client_create(server_state);
            if (server_state->dnssd_client == NULL) {
                FAULT("can't create dnssd client");
                return;
            }
            start = true;
        }
        if (start) {
            thread_tracker_start(server_state->thread_tracker);
            node_type_tracker_start(server_state->node_type_tracker);
            service_publisher_start(server_state->service_publisher);
            dnssd_client_start(server_state->dnssd_client);
        }
    }
}

// Start browsing for SRP service, and, if it makes sense, advertise as an SRP server.
void
thread_device_startup(srp_server_t *NONNULL server_state)
{
    // Just in case we get called without a shutdown having happened, before starting up again, do the
    // shutdown. This will be a no-op if it has already been done. Don't flush missing services, since we might
    // get a request for a missing service before we get the thread startup command.
    thread_device_shutdown(server_state);

    INFO("starting up");

    // Before we can actually do anything, we need our RLOC16.
    int status = cti_get_rloc16(server_state, &server_state->thread_rloc16_context, server_state,
                                thread_device_rloc16_callback, NULL);
    if (status != kCTIStatus_NoError) {
        FAULT("can't get rloc16: %d", status);
    }
}

void
thread_device_stop(srp_server_t *NONNULL server_state)
{
    INFO("stopping");
    if (server_state->service_tracker != NULL) {
        service_tracker_cancel_probes(server_state->service_tracker);
        service_tracker_cancel(server_state->service_tracker);
        service_tracker_release(server_state->service_tracker);
        server_state->service_tracker = NULL;
    }
    if (server_state->thread_tracker != NULL) {
        thread_tracker_cancel(server_state->thread_tracker);
        thread_tracker_release(server_state->thread_tracker);
        server_state->thread_tracker = NULL;
    }
    if (server_state->node_type_tracker != NULL) {
        node_type_tracker_cancel(server_state->node_type_tracker);
        node_type_tracker_release(server_state->node_type_tracker);
        server_state->node_type_tracker = NULL;
    }
    if (server_state->service_publisher != NULL) {
        service_publisher_cancel(server_state->service_publisher);
        service_publisher_release(server_state->service_publisher);
        server_state->service_publisher = NULL;
    }
    if (server_state->dnssd_client != NULL) {
        dnssd_client_cancel(server_state->dnssd_client);
        dnssd_client_release(server_state->dnssd_client);
        server_state->dnssd_client = NULL;
    }
}

void
thread_device_shutdown(srp_server_t *NONNULL server_state)
{
    INFO("shutting down");
    if (server_state->thread_rloc16_context != NULL) {
        cti_events_discontinue(server_state->thread_rloc16_context);
        server_state->thread_rloc16_context = NULL;
    }
    if (server_state->wed_tracker != NULL) {
        cti_events_discontinue(server_state->wed_tracker);
        server_state->wed_tracker = NULL;
    }
    thread_device_stop(server_state);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
