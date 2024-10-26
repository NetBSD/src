/* dnssd-client.c
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
 * This file contains code to queue and send updates for Thread services.
 */

#ifndef LINUX
#include <netinet/in.h>
#include <net/if.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <net/if_media.h>
#include <sys/stat.h>
#else
#define _GNU_SOURCE
#include <netinet/in.h>
#include <fcntl.h>
#include <bsd/stdlib.h>
#include <net/if.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <netinet/icmp6.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stddef.h>
#include <dns_sd.h>
#include <inttypes.h>
#include <signal.h>

#ifdef IOLOOP_MACOS
#include <xpc/xpc.h>

#include <TargetConditionals.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCPrivate.h>
#include <SystemConfiguration/SCNetworkConfigurationPrivate.h>
#include <SystemConfiguration/SCNetworkSignature.h>
#include <network_information.h>

#include <CoreUtils/CoreUtils.h>
#include <mrc/private.h>
#endif // IOLOOP_MACOS

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-crypto.h"

#include "cti-services.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "adv-ctl-server.h"
#include "dnssd-proxy.h"
#include "srp-proxy.h"
#include "route.h"

#define STATE_MACHINE_IMPLEMENTATION 1
typedef enum {
    dnssd_client_state_invalid,
    dnssd_client_state_startup,
    dnssd_client_state_not_client,
    dnssd_client_state_probing,
    dnssd_client_state_client,
} state_machine_state_t;
#define state_machine_state_invalid dnssd_client_state_invalid

#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"
#include "service-publisher.h"

#include "dnssd-client.h"
#include "thread-tracker.h"
#include "probe-srp.h"

typedef enum {
    dnssd_client_dns_service_type_invalid   = 0,
    dnssd_client_dns_service_type_do53      = 1,
    dnssd_client_dns_service_type_push      = 2,
} dnssd_client_dns_service_type_t;

struct dnssd_client {
    int ref_count;
    state_machine_header_t state_header;
    char *id;
    srp_server_t *server_state;
    wakeup_t *wakeup_timer;
    cti_connection_t active_data_set_connection;
    struct in6_addr mesh_local_prefix;
    bool have_mesh_local_prefix;
    bool first_time;
    bool canceled;
    dnssd_txn_t *shared_txn;
    thread_service_t *published_service;
    DNSServiceRef shared_connection;
    mrc_dns_service_registration_t dns_service_registration;        // DNS service registration handler.
    dnssd_client_dns_service_type_t dns_service_registration_type;  // The type of the registered DNS service.
    int interface_index;
    uint16_t dns_service_registration_port;                         // The port of the registered DNS service used.
};

static uint64_t dnssd_client_serial_number;

static void
dnssd_client_finalize(dnssd_client_t *client)
{
    thread_service_release(client->published_service);
    ioloop_wakeup_release(client->wakeup_timer);
    free(client->state_header.name);
    free(client->id);
    free(client);
}

RELEASE_RETAIN_FUNCS(dnssd_client);

static void
dnssd_client_context_release(void *context)
{
    dnssd_client_t *client = context;
    RELEASE_HERE(client, dnssd_client);
}

static void
dnssd_client_wait_expired(void *context)
{
    dnssd_client_t *client = context;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_timeout, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&client->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
dnssd_client_service_tracker_callback(void *context)
{
    dnssd_client_t *client = context;

    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_service_list_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&client->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
dnssd_client_probe_callback(thread_service_t *UNUSED service, void *context, bool UNUSED succeeded)
{
    dnssd_client_t *client = context;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_probe_completed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&client->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
dnssd_client_get_mesh_local_prefix_callback(void *context, const char *prefix_string, int status)
{
    dnssd_client_t *client = context;
    INFO(PUB_S_SRP " %d", prefix_string != NULL ? prefix_string : "<null>", status);
    if (status != kCTIStatus_NoError || prefix_string == NULL) {
        goto fail;
    }

    char prefix_buf[INET6_ADDRSTRLEN];

    const char *prefix_addr_string;
    char *slash = strchr(prefix_string, '/');
    if (slash != NULL) {
        size_t len = slash - prefix_string;
        if (len == 0) {
            ERROR("bogus prefix: " PRI_S_SRP, prefix_string);
            goto fail;
        }
        if (len - 1 > sizeof(prefix_buf)) {
            ERROR("prefix too long: " PRI_S_SRP, prefix_string);
            goto fail;
        }
        memcpy(prefix_buf, prefix_string, len);
        prefix_buf[len] = 0;
        prefix_addr_string = prefix_buf;
    } else {
        prefix_addr_string = prefix_string;
    }
    if (!inet_pton(AF_INET6, prefix_addr_string, &client->mesh_local_prefix)) {
        ERROR("prefix syntax incorrect: " PRI_S_SRP, prefix_addr_string);
        goto fail;
    }
    client->have_mesh_local_prefix = true;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_got_mesh_local_prefix, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&client->state_header, event);
    RELEASE_HERE(event, state_machine_event);
fail:
    RELEASE_HERE(client, dnssd_client); // was retained for callback, can only get one callback
    return;
}

static state_machine_state_t
dnssd_client_remove_published_service(dnssd_client_t *client)
{
    if (client->dns_service_registration != NULL) {
        mrc_dns_service_registration_forget(&client->dns_service_registration);
    }
    return dnssd_client_state_not_client;
}

static state_machine_state_t
dnssd_client_service_unpublish(dnssd_client_t *client)
{
    if (client->dns_service_registration != NULL) {
        thread_service_note(client->id, client->published_service, "unpublishing service");
    }
    dnssd_client_remove_published_service(client);

    return dnssd_client_state_not_client;
}

static void
dnssd_client_dns_service_event_handler(const mrc_dns_service_registration_event_t mrc_event, const OSStatus event_err,
                                       dnssd_client_t *client)
{
    state_machine_event_t *event = NULL;
    bool want_event = false;
    switch(mrc_event)
    {
    case mrc_dns_service_registration_event_started:
        INFO("DNS service registration started");
        break;
    case mrc_dns_service_registration_event_interruption:
        INFO("DNS service registration interrupted" );
        break;

    case mrc_dns_service_registration_event_invalidation:
        want_event = true;
        if (event_err) {
            ERROR("DNS service registration invalidated with error: %d", (int)event_err);
        } else {
            INFO("DNS service registration gracefully invalidated");
        }
        event = state_machine_event_create(state_machine_event_type_dns_registration_invalidated, NULL);
        break;
    case mrc_dns_service_registration_event_connection_error:
        want_event = true;
        ERROR("Registered DNS Push connection failed on server with error: %d", (int)event_err);
        event = state_machine_event_create(state_machine_event_type_dns_registration_bad_service, NULL);
        break;
    }

    if (want_event) {
        if (event == NULL) {
            ERROR("unable to allocate event to deliver");
        } else {
            state_machine_event_deliver(&client->state_header, event);
            RELEASE_HERE(event, state_machine_event);
        }
        // Either event should be the last event we get.
        RELEASE_HERE(client, dnssd_client);
    }
}


static state_machine_state_t
dnssd_client_service_publish(dnssd_client_t *client)
{
    dnssd_client_service_unpublish(client);
    state_machine_state_t ret = dnssd_client_state_not_client;

    OSStatus err;
    mdns_dns_service_definition_t do53_definition = NULL;
    mdns_dns_push_service_definition_t push_definition = NULL;
    mdns_domain_name_t domain_name = NULL;
    mdns_address_t server_address = NULL;

    domain_name = mdns_domain_name_create("default.service.arpa.", mdns_domain_name_create_opts_none, &err);
    require_noerr_action(err, out, ERROR("failed to create default.service.arpa.: %{darwin.errno}d", (int)err));

    const uint8_t *aaaa_data = NULL;
    uint16_t port = 0;
    // @TODO: Adds SRV service probing to determine the DoT port instead the default dns_service_registration_port.
    if (client->published_service->service_type == unicast_service) {
        aaaa_data = &client->published_service->u.unicast.address.s6_addr[0];
    } else if (client->published_service->service_type == anycast_service) {
        aaaa_data = &client->published_service->u.anycast.address.s6_addr[0];
    }
    port = client->dns_service_registration_port;
    require_action(aaaa_data != NULL, out, ERROR("failed to get service address"));

    SEGMENTED_IPv6_ADDR_GEN_SRP(aaaa_data, ipv6_addr_buf);
    server_address = mdns_address_create_ipv6(aaaa_data, port, 0);
    if (server_address == NULL) {
        ERROR("failed to create address object -- address: " PRI_SEGMENTED_IPv6_ADDR_SRP,
              SEGMENTED_IPv6_ADDR_PARAM_SRP(aaaa_data, ipv6_addr_buf));
        goto out;
    }
    if (client->dns_service_registration_type == dnssd_client_dns_service_type_push) {
        push_definition = mdns_dns_push_service_definition_create();
        require_action(push_definition, out, ERROR("unable to allocate mdns_dns_push_service_definition object"));

        err = mdns_dns_push_service_definition_add_domain(push_definition, domain_name);
        require_noerr(err, out);

        mdns_dns_push_service_definition_append_server_address(push_definition, server_address);

        client->dns_service_registration = mrc_dns_service_registration_create_push(push_definition);
        if (client->dns_service_registration) {
            mrc_dns_service_registration_set_reports_connection_errors(client->dns_service_registration,
                true);
        }
    } else if (client->dns_service_registration_type == dnssd_client_dns_service_type_do53) {
        do53_definition = mdns_dns_service_definition_create();
        require_action(do53_definition, out, ERROR("unable to allocate mdns_dns_service_definition object"));

        err = mdns_dns_service_definition_add_domain(do53_definition, domain_name);
        require_noerr(err, out);

        err = mdns_dns_service_definition_append_server_address(do53_definition, server_address);
        require_noerr(err, out);

        client->dns_service_registration = mrc_dns_service_registration_create(do53_definition);
    } else {
        ERROR("Unknown DNS service registration type: %d", client->dns_service_registration_type);
        goto out;
    }
    require_action(client->dns_service_registration != NULL, out, ERROR("failed to create DNS service registration"));
    mrc_dns_service_registration_set_queue(client->dns_service_registration, dispatch_get_main_queue());

    RETAIN_HERE(client, dnssd_client); // For the handler
    mrc_dns_service_registration_set_event_handler(client->dns_service_registration,
    ^(const mrc_dns_service_registration_event_t event, const OSStatus event_err)
    {
        dnssd_client_dns_service_event_handler(event, event_err, client);
    });
    INFO("Publishing dnssd client service -- domain: " PRI_DNS_NAME_SRP ", address: " PRI_SEGMENTED_IPv6_ADDR_SRP,
        mdns_domain_name_get_presentation(domain_name), SEGMENTED_IPv6_ADDR_PARAM_SRP(aaaa_data, ipv6_addr_buf));
    mrc_dns_service_registration_activate(client->dns_service_registration);

    ret = dnssd_client_state_invalid;
out:
    mdns_forget(&do53_definition);
    mdns_forget(&push_definition);
    mdns_forget(&domain_name);
    mdns_forget(&server_address);
    return ret;
}

static state_machine_state_t dnssd_client_action_startup(state_machine_header_t *state_header,
                                                         state_machine_event_t *event);
static state_machine_state_t dnssd_client_action_not_client(state_machine_header_t *state_header,
                                                            state_machine_event_t *event);
static state_machine_state_t dnssd_client_action_probing(state_machine_header_t *state_header,
                                                         state_machine_event_t *event);
static state_machine_state_t dnssd_client_action_client(state_machine_header_t *state_header,
                                                        state_machine_event_t *event);

// States:
// -- not_client: Not a client because server or because no service (in which case hopefully we are becoming server)
// --     client: Found a working service, configured as client

#define SERVICE_PUB_NAME_DECL(name) dnssd_client_state_##name, #name
static state_machine_decl_t dnssd_client_states[] = {
    { SERVICE_PUB_NAME_DECL(invalid),                            NULL },
    { SERVICE_PUB_NAME_DECL(startup),                            dnssd_client_action_startup },
    { SERVICE_PUB_NAME_DECL(not_client),                         dnssd_client_action_not_client },
    { SERVICE_PUB_NAME_DECL(probing),                            dnssd_client_action_probing },
    { SERVICE_PUB_NAME_DECL(client),                             dnssd_client_action_client },
};
#define DNSSD_CLIENT_NUM_STATES ((sizeof(dnssd_client_states)) / (sizeof(state_machine_decl_t)))

#define STATE_MACHINE_HEADER_TO_CLIENT(state_header)                                       \
    if (state_header->state_machine_type != state_machine_type_dnssd_client) {             \
        ERROR("state header type isn't omr_client: %d", state_header->state_machine_type); \
        return dnssd_client_state_invalid;                                                 \
    }                                                                                      \
    dnssd_client_t *client = state_header->state_object

static bool
dnssd_client_should_be_client(dnssd_client_t *client)
{
    bool might_publish = false;
    bool associated = true;
    bool should_be_client = false;
    srp_server_t *server_state = client->server_state;

    if (!service_publisher_could_publish(server_state->service_publisher)) {
        should_be_client = true;
        might_publish = true;
    }

    if (!thread_tracker_associated_get(server_state->thread_tracker, false)) {
        associated = false;
        should_be_client = false;
    }

    INFO(PUB_S_SRP PUB_S_SRP PUB_S_SRP,
         should_be_client ?                "could be client" : "can't be client",
         might_publish ?                    " might publish" : " won't publish",
         associated ?                                     "" : " not associated ");
    return should_be_client;
}

// We start in this state and remain here until we get a mesh-local prefix.
static state_machine_state_t
dnssd_client_action_startup(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_CLIENT(state_header);
    BR_STATE_ANNOUNCE(client, event);

    if (client->have_mesh_local_prefix) {
        return dnssd_client_state_not_client;
    }
    return dnssd_client_state_invalid;
}

// We get into this state when there is no SRP service published on the Thread network other than our own.
// If we stop publishing (because a BR-based service showed up), then we go to the probe state.
static state_machine_state_t
dnssd_client_action_not_client(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_CLIENT(state_header);
    BR_STATE_ANNOUNCE(client, event);

    if (event == NULL) {
        if (client->published_service != NULL) {
            dnssd_client_service_unpublish(client);
        }
        return dnssd_client_state_invalid;
    }

    // We do the same thing here for any event we get, because we're just waiting for conditions to be right.
    if (dnssd_client_should_be_client(client)) {
        return dnssd_client_state_probing;
    }
    return dnssd_client_state_invalid;
}

// We get into this state when we've determined we should be a client
static state_machine_state_t
dnssd_client_action_probing(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_CLIENT(state_header);
    BR_STATE_ANNOUNCE(client, event);

    thread_service_t *service;
    srp_server_t *server_state = client->server_state;

    // On arrival in this state, we start a timer. If we get to the publishing state, we cancel the timer. If the
    // timer goes off when we're still probing, we tell the service publisher to publish any relevant cached
    // services.  The point of this is that it's fairly common at least in testing that we want to be able to
    // control an accessory after joining the Thread network /because/ the BR went down, and in this case the BR's
    // published service may take 120 seconds to time out from the thread network data. Publishing the cached
    // services can potentially help with this.
    if (event == NULL) {
        ioloop_add_wake_event(client->wakeup_timer, client, dnssd_client_wait_expired,
                              dnssd_client_context_release, 5 * MSEC_PER_SEC); // Wait five seconds for probe to succeed.
        RETAIN_HERE(client, dnssd_client); // for the wake event
    }

    // If we have been waiting for a second since we started probing and haven't succeeded, tell the service publisher
    // to publish services.
    else if (event->type == state_machine_event_type_timeout) {
        if (server_state->service_publisher != NULL) {
            INFO("server probe startup timeout expired--publishing cached data.");
            service_publisher_re_advertise_matching(server_state->service_publisher);
        }
        return dnssd_client_state_invalid;
    }

    // If we no longer need to be a client, stop trying.
    else {
        if (!dnssd_client_should_be_client(client)) {
            ioloop_cancel_wake_event(client->wakeup_timer);
            return dnssd_client_state_not_client;
        }
    }

    // See if we now have a service that's been successfully probed; if so, publish it.
    service = service_tracker_verified_service_get(client->server_state->service_tracker);
    if (service != NULL) {
        if (client->published_service != NULL) {
            thread_service_release(client->published_service);
        }
        client->published_service = service;
        thread_service_retain(client->published_service);

        // Tell the service publisher that we successfully probed a service.
        INFO("server probe succeeded--unpublishing cached data.");
        service_publisher_unadvertise_all(server_state->service_publisher);

        // We no longer want a wakeup.  Note that this is the only way out of the probing state, so it's not
        // possible to exit the state without canceling the timer here.
        ioloop_cancel_wake_event(client->wakeup_timer);

        // Publish DNS Push service pointing to probed service.
        return dnssd_client_state_client;
    }

    // Check to see if we can start a new probe
    service = service_tracker_unverified_service_get(client->server_state->service_tracker, unicast_service);
    if (service != NULL) {
        if (service->checking) {
            service_tracker_thread_service_note(client->server_state->service_tracker, service,
                                                " is still being probed");
            return dnssd_client_state_invalid;
        }
        if (service->service_type == anycast_service) {
            memcpy(&service->u.anycast.address, &client->mesh_local_prefix, 8);
            memcpy(&service->u.anycast.address.s6_addr[8], thread_rloc_preamble, 6);
            service->u.anycast.address.s6_addr[14] = service->rloc16 >> 8;
            service->u.anycast.address.s6_addr[15] = service->rloc16 & 255;
        }
        RETAIN_HERE(client, dnssd_client); // For the probe
        probe_srp_service(service, client, dnssd_client_probe_callback, dnssd_client_context_release);
        return dnssd_client_state_invalid;
    }

    // This should only ever happen if we get the service list update before the service publisher gets it.
    INFO("no service to publish");
    return dnssd_client_state_invalid;
}


// We get into this state when we've published a service.
static state_machine_state_t
dnssd_client_action_client(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_CLIENT(state_header);
    BR_STATE_ANNOUNCE(client, event);

    if (event == NULL) {
        // Publish the service.
        dnssd_client_service_publish(client);
        return dnssd_client_state_invalid;
    }

    if (event->type == state_machine_event_type_dns_registration_bad_service) {
        if (client->published_service == NULL) {
            INFO("bad service event received with no published service.");
        } else {
            client->published_service->ignore = true;  // Don't consider this service when deciding what to advertise
            client->published_service->responding = false;
            thread_service_release(client->published_service);
            client->published_service = NULL;
            return dnssd_client_state_probing; // Go back to probing.
        }
    }

    // This can happen if the daemon disconnects.
    if (client->published_service == NULL) {
        return dnssd_client_state_not_client;
    }

    // If we should no longer be client, unpublish the service and wait
    if (!dnssd_client_should_be_client(client)) {
        return dnssd_client_state_not_client;
    }

    if (service_tracker_verified_service_still_exists(client->server_state->service_tracker,
                                                      client->published_service)) {
        return dnssd_client_state_invalid;
    }

    // If we get here, the service we have been publishing is no longer present.
    return dnssd_client_state_not_client;
}

void
dnssd_client_cancel(dnssd_client_t *client)
{
    if (client->server_state->service_tracker != NULL) {
        service_tracker_cancel_probes(client->server_state->service_tracker);
        service_tracker_callback_cancel(client->server_state->service_tracker, client);
    }
    if (client->server_state->thread_tracker != NULL) {
        thread_tracker_callback_cancel(client->server_state->thread_tracker, client);
    }
    ioloop_cancel_wake_event(client->wakeup_timer);
    if (client->active_data_set_connection != NULL) {
        cti_events_discontinue(client->active_data_set_connection);
        RELEASE_HERE(client, dnssd_client); // callback held reference
        client->active_data_set_connection = NULL;
    }
    dnssd_client_remove_published_service(client);
    state_machine_cancel(&client->state_header);
}

dnssd_client_t *
dnssd_client_create(srp_server_t *server_state)
{
    dnssd_client_t *ret = NULL, *client = calloc(1, sizeof(*client));
    if (client == NULL) {
        return client;
    }
    RETAIN_HERE(client, dnssd_client);
    client->wakeup_timer = ioloop_wakeup_create();
    if (client->wakeup_timer == NULL) {
        ERROR("wakeup timer alloc failed");
        goto out;
    }

    char client_id_buf[100];
    snprintf(client_id_buf, sizeof(client_id_buf), "[DC%lld]", ++dnssd_client_serial_number);
    client->id = strdup(client_id_buf);
    if (client->id == NULL) {
        ERROR("no memory for client ID");
        goto out;
    }
    client->interface_index = -1;
    client->dns_service_registration_type = dnssd_client_dns_service_type_push;
    client->dns_service_registration_port = DNS_OVER_TLS_DEFAULT_PORT;

    if (!state_machine_header_setup(&client->state_header,
                                    client, client->id,
                                    state_machine_type_dnssd_client,
                                    dnssd_client_states,
                                    DNSSD_CLIENT_NUM_STATES)) {
        ERROR("header setup failed");
        goto out;
    }

    client->server_state = server_state;
    if (!service_tracker_callback_add(server_state->service_tracker, dnssd_client_service_tracker_callback,
                                      dnssd_client_context_release, client))
    {
        goto out;
    }
    RETAIN_HERE(client, dnssd_client); // for service tracker

    ret = client;
    client = NULL;
out:
    if (client != NULL) {
        RELEASE_HERE(client, dnssd_client);
    }
    return ret;
}

static void
dnssd_client_get_tunnel_name_callback(void *context, const char *name, cti_status_t status)
{
    dnssd_client_t *client = context;
    if (status != kCTIStatus_NoError) {
        INFO("didn't get tunnel name, error code %d", status);
        goto fail;
    }
    client->interface_index = if_nametoindex(name);
fail:
    RELEASE_HERE(client, dnssd_client); // was retained for callback, can only get one callback
}

static void
dnssd_client_active_data_set_changed_callback(void *context, cti_status_t status)
{
    dnssd_client_t *client = context;

    if (status != kCTIStatus_NoError) {
        ERROR("error %d", status);
        RELEASE_HERE(client, dnssd_client); // no more callbacks
        cti_events_discontinue(client->active_data_set_connection);
        client->active_data_set_connection = NULL;
        return;
    }

    status = cti_get_mesh_local_prefix(client->server_state, client, dnssd_client_get_mesh_local_prefix_callback, NULL);
    if (status != kCTIStatus_NoError) {
        ERROR("cti_get_mesh_local_prefix failed with status %d", status);
    } else {
        RETAIN_HERE(client, dnssd_client); // for mesh-local callback
    }
    status = cti_get_tunnel_name(client->server_state, client, dnssd_client_get_tunnel_name_callback, NULL);
    if (status != kCTIStatus_NoError) {
        ERROR("cti_get_tunnel_name failed with status %d", status);
    } else {
        RETAIN_HERE(client, dnssd_client); // for tunnel name callback
    }
}

void
dnssd_client_start(dnssd_client_t *client)
{
    cti_track_active_data_set(client->server_state, &client->active_data_set_connection,
                              client, dnssd_client_active_data_set_changed_callback, NULL);
    RETAIN_HERE(client, dnssd_client); // for callback
    dnssd_client_active_data_set_changed_callback(client, kCTIStatus_NoError); // Get the initial state.
    state_machine_next_state(&client->state_header, dnssd_client_state_startup);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
