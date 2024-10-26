/* omr-publisher.c
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
 * This file contains an implementation of the Off-Mesh-Routable (OMR)
 * prefix publisher state machine.
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
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "route.h"
#include "dnssd-proxy.h"


#define STATE_MACHINE_IMPLEMENTATION 1
typedef enum {
    omr_publisher_state_invalid,
    omr_publisher_state_startup,
    omr_publisher_state_check_for_dhcp,
    omr_publisher_state_not_publishing,
    omr_publisher_state_publishing_dhcp,
    omr_publisher_state_publishing_ula,
} state_machine_state_t;
#define state_machine_state_invalid omr_publisher_state_invalid

#include "state-machine.h"
#include "service-publisher.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "omr-publisher.h"
#include "route-tracker.h"

typedef struct omr_publisher {
    int ref_count;
    state_machine_header_t state_header;

    route_state_t *route_state;
    wakeup_t *NULLABLE wakeup_timer;
    void *dhcp_client;
    omr_watcher_callback_t *omr_watcher_callback;
    omr_watcher_t *omr_watcher;
    void (*reconnect_callback)(void *context);

    omr_prefix_t *published_prefix;
    omr_prefix_t *publication_queue;
    interface_t *dhcp_interface;
    struct in6_addr ula_prefix;
    struct in6_addr dhcp_prefix;
    struct in6_addr new_dhcp_prefix;
    int dhcp_prefix_length, new_dhcp_prefix_length;
    uint32_t dhcp_preferred_lifetime, new_dhcp_preferred_lifetime;
    int min_start;
    omr_prefix_priority_t omr_priority, force_priority;
    bool ula_prefix_published, dhcp_prefix_published;
    bool dhcp_wanted;
    bool dhcp_blocked;
    bool first_time;
    bool force_publication;
} omr_publisher_t;

                                                                                                                       \
#define STATE_MACHINE_HEADER_TO_PUBLISHER(state_header)                                                                \
    if (state_header->state_machine_type != state_machine_type_omr_publisher) {                                        \
        ERROR("state header type isn't omr_publisher: %d", state_header->state_machine_type);                          \
        return omr_publisher_state_invalid;                                                                            \
    }                                                                                                                  \
    omr_publisher_t *publisher = state_header->state_object

static state_machine_state_t omr_publisher_action_startup(state_machine_header_t *state_header,
                                                          state_machine_event_t *event);
static state_machine_state_t omr_publisher_action_check_for_dhcp(state_machine_header_t *state_header,
                                                                 state_machine_event_t *event);
static state_machine_state_t omr_publisher_action_not_publishing(state_machine_header_t *state_header,
                                                                 state_machine_event_t *event);
static state_machine_state_t omr_publisher_action_publishing_dhcp(state_machine_header_t *state_header,
                                                                  state_machine_event_t *event);
static state_machine_state_t omr_publisher_action_publishing_ula(state_machine_header_t *state_header,
                                                                 state_machine_event_t *event);

#define OMR_PUB_NAME_DECL(name) omr_publisher_state_##name, #name
static state_machine_decl_t omr_publisher_states[] = {
    { OMR_PUB_NAME_DECL(invalid),                            NULL },
    { OMR_PUB_NAME_DECL(startup),                            omr_publisher_action_startup },
    { OMR_PUB_NAME_DECL(check_for_dhcp),                     omr_publisher_action_check_for_dhcp },
    { OMR_PUB_NAME_DECL(not_publishing),                     omr_publisher_action_not_publishing },
    { OMR_PUB_NAME_DECL(publishing_dhcp),                    omr_publisher_action_publishing_dhcp },
    { OMR_PUB_NAME_DECL(publishing_ula),                     omr_publisher_action_publishing_ula },
};
#define OMR_PUBLISHER_NUM_CONNECTION_STATES ((sizeof(omr_publisher_states)) / (sizeof(state_machine_decl_t)))

static void omr_publisher_discontinue_dhcp(omr_publisher_t *publisher);
static void omr_publisher_queue_prefix_update(omr_publisher_t *publisher, struct in6_addr *prefix_address,
                                              omr_prefix_priority_t priority, bool preferred,
                                              thread_service_publication_state_t initial_state);

static void
omr_publisher_finalize(omr_publisher_t *publisher)
{
    free(publisher->state_header.name);
    free(publisher);
}
RELEASE_RETAIN_FUNCS(omr_publisher);

static void
omr_publisher_context_release(route_state_t *UNUSED route_state, void *context)
{
    omr_publisher_t *publisher = context;
    RELEASE_HERE(publisher, omr_publisher);
}

static void
omr_publisher_event_prefix_update_finished_finalize(state_machine_event_t *event)
{
    if (event->thread_prefixes != NULL) {
        omr_prefix_release(event->thread_prefixes);
        event->thread_prefixes = NULL;
    }
}

static void
omr_publisher_watcher_callback(route_state_t *UNUSED NONNULL route_state, void *NULLABLE context, omr_watcher_event_type_t event_type,
                               omr_prefix_t *NULLABLE prefixes, omr_prefix_t *NULLABLE prefix)
{
    omr_publisher_t *publisher = context;

    // On startup, notice any prefixes with the user flag set: we didn't publish these, so they must be left over from
    // a crash or restart.
    if (publisher->first_time && event_type == omr_watcher_event_prefix_added && prefix != NULL && prefix->user) {
        // We might actually publish our own prefix before we go through this loop for the first time. If that's the case,
        // don't unpublish it!
        if (publisher->published_prefix == NULL ||
            in6addr_compare(&prefix->prefix, &publisher->published_prefix->prefix))
        {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("removing stale prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            omr_publisher_queue_prefix_update(publisher, &prefix->prefix, omr_prefix_priority_low, false, want_delete);
        }
    }

    // We don't otherwise care about "prefix appeared" and "prefix disappeared" events, nor about flag change events--
    // just about the final state
    if (event_type == omr_watcher_event_prefix_update_finished) {
        state_machine_event_t *event = state_machine_event_create(state_machine_event_type_prefix,
                                                                  omr_publisher_event_prefix_update_finished_finalize);
        if (event == NULL) {
            ERROR("unable to allocate event to deliver");
            return;
        }
        event->thread_prefixes = prefixes;
        if (prefixes != NULL) {
            omr_prefix_retain(event->thread_prefixes);
        }
        state_machine_event_deliver(&publisher->state_header, event);
        RELEASE_HERE(event, state_machine_event);

        publisher->first_time = false;
    }
}

void
omr_publisher_set_omr_watcher(omr_publisher_t *publisher, omr_watcher_t *watcher)
{
    if (watcher != NULL) {
        publisher->omr_watcher = watcher;
        omr_watcher_retain(publisher->omr_watcher);
        publisher->omr_watcher_callback = omr_watcher_callback_add(publisher->omr_watcher,
                                                                   omr_publisher_watcher_callback,
                                                                   omr_publisher_context_release, publisher);
        RETAIN_HERE(publisher, omr_publisher); // The omr watcher callback holds a reference to the publisher
    }
}

void omr_publisher_set_reconnect_callback(omr_publisher_t *NONNULL publisher,
                                          void (*NULLABLE reconnect_callback)(void *NULLABLE context))
{
    publisher->reconnect_callback = reconnect_callback;
}

void
omr_publisher_cancel(omr_publisher_t *publisher)
{
    if (publisher->wakeup_timer != NULL) {
        ioloop_cancel_wake_event(publisher->wakeup_timer);
        ioloop_wakeup_release(publisher->wakeup_timer);
        publisher->wakeup_timer = NULL;
    }
    if (publisher->omr_watcher_callback != NULL) {
        omr_watcher_callback_cancel(publisher->omr_watcher, publisher->omr_watcher_callback);
        publisher->omr_watcher_callback = NULL;
    }
    if (publisher->omr_watcher != NULL) {
        omr_watcher_release(publisher->omr_watcher);
        publisher->omr_watcher = NULL;
    }
    if (publisher->published_prefix != NULL) {
        omr_publisher_unpublish_prefix(publisher);
    }
    if (publisher->dhcp_client) {
        omr_publisher_discontinue_dhcp(publisher);
    }
    if (publisher->dhcp_interface != NULL) {
        interface_release(publisher->dhcp_interface);
        publisher->dhcp_interface = NULL;
    }
    state_machine_cancel(&publisher->state_header);
}

omr_publisher_t *
omr_publisher_create(route_state_t *route_state, const char *name)
{
    omr_publisher_t *ret = NULL, *publisher = calloc(1, sizeof(*publisher));
    if (publisher == NULL) {
        return publisher;
    }
    RETAIN_HERE(publisher, omr_publisher);
    publisher->wakeup_timer = ioloop_wakeup_create();
    if (publisher->wakeup_timer == NULL) {
        ERROR("wakeup timer alloc failed");
        goto out;
    }

    if (!state_machine_header_setup(&publisher->state_header,
                                    publisher, name,
                                    state_machine_type_omr_publisher,
                                    omr_publisher_states,
                                    OMR_PUBLISHER_NUM_CONNECTION_STATES)) {
        ERROR("header setup failed");
        goto out;
    }

    publisher->route_state = route_state;

    // Set the first_time flag so that we'll know to remove any locally-published on-mesh prefixes.
    publisher->first_time = true;
    publisher->min_start = OMR_PUBLISHER_MIN_START;

    ret = publisher;
    publisher = NULL;
out:
    if (publisher != NULL) {
        RELEASE_HERE(publisher, omr_publisher);
    }
    return ret;
}

void
omr_publisher_start(omr_publisher_t *publisher)
{
    state_machine_next_state(&publisher->state_header, omr_publisher_state_startup);
}

void
omr_publisher_force_publication(omr_publisher_t *publisher, omr_prefix_priority_t priority)
{
    publisher->force_publication = true;
    if (publisher->state_header.state == omr_publisher_state_publishing_dhcp ||
        publisher->state_header.state == omr_publisher_state_publishing_ula)
    {
        ERROR("already publishing");
        return;
    } else {
        INFO("forcing publication");
    }
    publisher->force_publication = true;
    publisher->force_priority = priority;
    state_machine_next_state(&publisher->state_header, omr_publisher_state_publishing_ula);
}

omr_prefix_t *NULLABLE
omr_publisher_published_prefix_get(omr_publisher_t *publisher)
{
    return publisher->published_prefix;
}

static void
omr_publisher_wait_expired(void *context)
{
    omr_publisher_t *publisher = context;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_timeout, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
omr_publisher_wakeup_release(void *context)
{
    omr_publisher_t *publisher = context;
    RELEASE_HERE(publisher, omr_publisher);
}

static void
omr_publisher_send_dhcp_event(omr_publisher_t *publisher, struct in6_addr *prefix, int prefix_length, uint32_t preferred_lifetime)
{
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_dhcp, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    if (prefix == NULL) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&publisher->dhcp_prefix.s6_addr, prefix_buf);
        INFO("DHCPv6 prefix withdrawn " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d, lifetime = %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&publisher->dhcp_prefix.s6_addr, prefix_buf),
             publisher->dhcp_prefix_length, preferred_lifetime);
        in6addr_zero(&publisher->new_dhcp_prefix);
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->s6_addr, prefix_buf);
        INFO("received DHCPv6 prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d, lifetime = %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->s6_addr, prefix_buf), prefix_length, preferred_lifetime);
        publisher->new_dhcp_prefix = *prefix;
    }
    publisher->new_dhcp_prefix_length = prefix_length;
    publisher->new_dhcp_preferred_lifetime = preferred_lifetime;
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}


static void
omr_publisher_initiate_dhcp(omr_publisher_t *publisher)
{
    if (!publisher->dhcp_blocked) {
        publisher->dhcp_wanted = true;
    }
    publisher->dhcp_client = (void *)-1;
}

void
omr_publisher_interface_configuration_changed(omr_publisher_t *publisher)
{
    // Check to see if DHCP interface became inactive/ineligible
    if (publisher->dhcp_interface != NULL) {
        if (publisher->dhcp_interface->inactive || publisher->dhcp_interface->ineligible) {
            // If we have a DHCPv6 client running, we need to discontinue it.
            if (publisher->dhcp_client != NULL) {
                omr_publisher_dhcp_client_deactivate(publisher, (intptr_t)publisher->dhcp_client);
            }
            interface_release(publisher->dhcp_interface);
            publisher->dhcp_interface = NULL;
        }
    }
    if (publisher->dhcp_wanted && publisher->dhcp_client == NULL) {
        // Start DHCP on the new interface (if there is one)
        omr_publisher_initiate_dhcp(publisher);
    }
}

static void
omr_publisher_discontinue_dhcp(omr_publisher_t *publisher)
{
    INFO("discontinuing DHCP PD client");
    omr_publisher_dhcp_client_deactivate(publisher, (intptr_t)publisher->dhcp_client);
    publisher->dhcp_wanted = false;
}

static bool
omr_publisher_dhcp_prefix_available(omr_publisher_t *publisher)
{
    if (publisher->new_dhcp_preferred_lifetime != 0 && publisher->new_dhcp_prefix_length <= 64) {
        return true;
    }
    return false;
}

static bool
omr_publisher_dhcp_prefix_changed(omr_publisher_t *publisher)
{
    if (publisher->new_dhcp_prefix_length != publisher->dhcp_prefix_length ||
        publisher->new_dhcp_preferred_lifetime != publisher->dhcp_preferred_lifetime ||
        in6addr_compare(&publisher->new_dhcp_prefix, &publisher->dhcp_prefix))
    {
        return true;
    }
    return false;
}

static bool
omr_publisher_dhcp_prefix_lost(omr_publisher_t *publisher)
{
    if (publisher->new_dhcp_prefix_length == 0) {
        return true;
    }
    return false;
}

static void
omr_publisher_install_new_dhcp_prefix(omr_publisher_t *publisher)
{
    publisher->dhcp_prefix = publisher->new_dhcp_prefix;
    publisher->dhcp_prefix_length = publisher->new_dhcp_prefix_length;
    publisher->dhcp_preferred_lifetime = publisher->new_dhcp_preferred_lifetime;
}

static void
omr_publisher_prefix_init_from_published(omr_publisher_t *publisher, struct in6_addr *prefix, int *prefix_length)
{
    if (publisher->published_prefix != NULL) {
        in6addr_copy(prefix, &publisher->published_prefix->prefix);
        *prefix_length = publisher->published_prefix->prefix_length;
    } else {
        in6addr_zero(prefix);
        *prefix_length = 64;
    }
}

static bool
omr_publisher_prefix_present(omr_publisher_t *publisher, omr_prefix_priority_t priority)
{
    struct in6_addr prefix;
    int prefix_length;
    if (publisher->omr_watcher == NULL) {
        FAULT("expecting an omr_watcher to be on the publisher");
        return true; // Saying yes because we have no watcher and hence can't behave correctly.
    }
    omr_publisher_prefix_init_from_published(publisher, &prefix, &prefix_length);
    return omr_watcher_prefix_present(publisher->omr_watcher, priority, &prefix, prefix_length);
}

static bool
omr_publisher_high_prefix_present(omr_publisher_t *publisher)
{
    bool ret = omr_publisher_prefix_present(publisher, omr_prefix_priority_high);
    if (ret) {
        INFO("setting publisher->omr_priority to high");
        publisher->omr_priority = omr_prefix_priority_high;
    }
    return ret;
}

static bool
omr_publisher_medium_prefix_present(omr_publisher_t *publisher)
{
    bool ret = omr_publisher_prefix_present(publisher, omr_prefix_priority_medium);
    if (ret) {
        INFO("setting publisher->omr_priority to medium");
        publisher->omr_priority = omr_prefix_priority_medium;
    }
    return ret;
}

static bool
omr_publisher_medium_or_high_prefix_present(omr_publisher_t *publisher)
{
    return omr_publisher_medium_prefix_present(publisher) || omr_publisher_high_prefix_present(publisher);
}

static bool
omr_publisher_low_prefix_present(omr_publisher_t *publisher)
{
    return omr_publisher_prefix_present(publisher, omr_prefix_priority_low);
}

static bool
omr_publisher_prefix_wins(omr_publisher_t *publisher, omr_prefix_priority_t priority)
{
    struct in6_addr prefix;
    int prefix_length;
    if (publisher->omr_watcher == NULL) {
        FAULT("expecting an omr_watcher to be on the publisher");
        return true; // Saying yes because we have no watcher and hence can't behave correctly.
    }
    omr_publisher_prefix_init_from_published(publisher, &prefix, &prefix_length);
    return omr_watcher_prefix_wins(publisher->omr_watcher, priority, &prefix, prefix_length);
}

static bool
omr_publisher_medium_prefix_wins(omr_publisher_t *publisher)
{
    return omr_publisher_prefix_wins(publisher, omr_prefix_priority_medium);
}

static bool
omr_publisher_low_prefix_wins(omr_publisher_t *publisher)
{
    return omr_publisher_prefix_wins(publisher, omr_prefix_priority_low);
}

bool
omr_publisher_publishing_dhcp(omr_publisher_t *publisher)
{
    if (publisher->state_header.state == omr_publisher_state_publishing_dhcp)
    {
        return true;
    }
    return false;
}

bool
omr_publisher_publishing_ula(omr_publisher_t *publisher)
{
    if (publisher->state_header.state == omr_publisher_state_publishing_ula)
    {
        return true;
    }
    return false;
}

bool
omr_publisher_publishing_prefix(omr_publisher_t *publisher)
{
    return omr_publisher_publishing_dhcp(publisher) ||
           omr_publisher_publishing_ula(publisher);
}

static void omr_publisher_queue_run(omr_publisher_t *publisher);

static void
omr_publisher_prefix_update_callback(void *context, cti_status_t status)
{
    omr_publisher_t *publisher = context;
    omr_prefix_t *prefix = publisher->publication_queue;

    if (prefix == NULL) {
        ERROR("no pending prefix update");
        return;
    }
    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
    INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d is in state " PUB_S_SRP ", status = %d",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length,
         thread_service_publication_state_name_get(prefix->publication_state), status);
    if (status == kCTIStatus_NoError) {
        if (prefix->publication_state == add_pending) {
            prefix->publication_state = add_complete;
            if (publisher->omr_watcher != NULL) {
                omr_watcher_prefix_add(publisher->omr_watcher, &prefix->prefix, prefix->prefix_length, prefix->priority);
            }
        } else if (prefix->publication_state == delete_pending) {
            prefix->publication_state = delete_complete;
            if (publisher->omr_watcher != NULL) {
                omr_watcher_prefix_remove(publisher->omr_watcher, &prefix->prefix, prefix->prefix_length);
            }
        }
    } else {
        if (prefix->publication_state == add_pending) {
            prefix->publication_state = add_failed;
        } else if (prefix->publication_state == delete_pending) {
            prefix->publication_state = delete_failed;
        }
    }
    publisher->publication_queue = prefix->next;
    omr_prefix_release(prefix);
    omr_publisher_queue_run(publisher);
    RELEASE_HERE(publisher, omr_publisher);
}

static void
omr_publisher_queue_run(omr_publisher_t *publisher)
{
    omr_prefix_t *prefix = publisher->publication_queue;
    if (prefix == NULL) {
        INFO("the queue is empty.");
        // The queue just became empty, so release its reference to the publisher.
        RELEASE_HERE(publisher, omr_publisher);
        return;
    }
    if (prefix->publication_state == delete_pending || prefix->publication_state == add_pending) {
        INFO("there is a pending update at the head of the queue.");
        return;
    }
    if (prefix->publication_state == want_delete) {
        cti_status_t status = cti_remove_prefix(publisher->route_state->srp_server, publisher,
                                                omr_publisher_prefix_update_callback, NULL, &prefix->prefix,
                                                prefix->prefix_length);
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, prefix_buf);
        INFO("removing prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
        if (status != kCTIStatus_NoError) {
            ERROR("cti_remove_prefix failed: %d", status);
            // For removes, we'll leave it on the queue
        } else {
            prefix->publication_state = delete_pending;
            RETAIN_HERE(publisher, omr_publisher); // for the callback
        }
    } else if (prefix->publication_state == want_add) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, prefix_buf);
        INFO("adding prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
        cti_status_t status = cti_add_prefix(publisher->route_state->srp_server, publisher,
                                             omr_publisher_prefix_update_callback, NULL, &prefix->prefix,
                                             prefix->prefix_length, prefix->onmesh, prefix->preferred, prefix->slaac,
                                             prefix->stable, omr_prefix_priority_to_int(prefix->priority));
        if (status != kCTIStatus_NoError) {
            ERROR("cti_add_prefix failed: %d", status);
            publisher->publication_queue = prefix->next;
            omr_prefix_release(prefix);
        } else {
            prefix->publication_state = add_pending;
            RETAIN_HERE(publisher, omr_publisher); // for the callback
        }
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, prefix_buf);
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d is in unexpected state " PUB_S_SRP " on the publication queue",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length,
             thread_service_publication_state_name_get(prefix->publication_state));
        publisher->publication_queue = prefix->next;
        omr_prefix_release(prefix);
    }
}

static void
omr_publisher_queue_prefix_update(omr_publisher_t *publisher, struct in6_addr *prefix_address,
                                  omr_prefix_priority_t priority, bool preferred,
                                  thread_service_publication_state_t initial_state)
{
    const int mandatory_subnet_prefix_length = 64;
    omr_prefix_t *prefix, **ppref, *old_queue = publisher->publication_queue;
    int flags = omr_prefix_flags_generate(true /* onmesh */, preferred, true /* slaac */, priority);

    if (publisher->published_prefix != NULL) {
        FAULT("published prefix still present");
    }
    prefix = omr_prefix_create(prefix_address, mandatory_subnet_prefix_length,
                               0 /* metric */, flags, 0 /* rloc */, true /* stable */, false /* ncp */);
    if (prefix == NULL) {
        ERROR("no memory to remember published prefix!");
        return;
    }
    prefix->publication_state = initial_state;
    if (initial_state == want_add) {
        publisher->published_prefix = prefix;
        omr_prefix_retain(publisher->published_prefix);
    }
    // Find the end of the queue
    for (ppref = &publisher->publication_queue; *ppref != NULL; ppref = &(*ppref)->next)
        ;
    *ppref = prefix;
    // The prefix on the queue is retained by relying on the create/copy rule. When adding a prefix we also retain the
    // prefix as publisher->published_prefix, so that retain is always explicit and this retain is always implicit.
    // omr_prefix_retain(*ppref);

    // If there is anything in the queue, the queue holds a reference to the publisher, so that it will continue to
    // run until it's complete.
    if (old_queue == NULL && publisher->publication_queue != NULL) {
        RETAIN_HERE(publisher, omr_publisher);
    }
    omr_publisher_queue_run(publisher);
}

static void
omr_publisher_publish_prefix(omr_publisher_t *publisher,
                             struct in6_addr *prefix_address, omr_prefix_priority_t priority, bool preferred)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix_address, prefix_buf);
    INFO("publishing prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/64",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix_address, prefix_buf));
    omr_publisher_queue_prefix_update(publisher, prefix_address, priority, preferred, want_add);
    INFO("setting publisher->omr_priority to %d, " PUB_S_SRP "preferred", omr_prefix_priority_to_int(priority),
         preferred ? "" : "not ");
    publisher->omr_priority = priority;
}

void
omr_publisher_unpublish_prefix(omr_publisher_t *publisher)
{
    omr_prefix_t *prefix;

    prefix = publisher->published_prefix;
    publisher->published_prefix = NULL;
    if (prefix == NULL) {
        ERROR("request to unpublished prefix that's not present");
        return;
    }
    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, prefix_buf);
    INFO("unpublishing prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
    omr_publisher_queue_prefix_update(publisher, &prefix->prefix, prefix->priority, false, want_delete);
    omr_prefix_release(prefix);
}

static void
omr_publisher_publish_dhcp_prefix(omr_publisher_t *publisher)
{
    omr_publisher_install_new_dhcp_prefix(publisher);
    omr_publisher_publish_prefix(publisher, &publisher->dhcp_prefix, omr_prefix_priority_medium,
                                 publisher->dhcp_preferred_lifetime == 0 ? false : true);
    publisher->dhcp_prefix_published = true;
    return;
}

static void
omr_publisher_unpublish_dhcp_prefix(omr_publisher_t *publisher)
{
    omr_publisher_unpublish_prefix(publisher);
    publisher->dhcp_prefix_published = false;
    return;
}

static void
omr_publisher_publish_ula_prefix(omr_publisher_t *publisher)
{
    if (publisher->route_state == NULL || !publisher->route_state->have_thread_prefix) {
        ERROR("don't have a thread prefix to publish!");
        return;
    }
    omr_prefix_priority_t priority;
    if (publisher->force_publication) {
        priority = publisher->force_priority;
    } else {
        priority = omr_prefix_priority_low;
    }
    omr_publisher_publish_prefix(publisher, &publisher->route_state->my_thread_ula_prefix, priority, true);
    publisher->ula_prefix_published = true;
    return;
}

static void
omr_publisher_unpublish_ula_prefix(omr_publisher_t *publisher)
{
    omr_publisher_unpublish_prefix(publisher);
    publisher->ula_prefix_published = false;
    return;
}

bool
omr_publisher_have_routable_prefix(omr_publisher_t *publisher)
{
    if ((publisher->published_prefix != NULL && omr_watcher_prefix_is_non_ula_prefix(publisher->published_prefix)) ||
        (publisher->omr_watcher != NULL && omr_watcher_non_ula_prefix_present(publisher->omr_watcher)))
    {
        INFO("we have a routable prefix");
        return true;
    }
    INFO("we do not have a routable prefix");
    return false;
}

static state_machine_state_t
omr_publisher_dhcp_event(omr_publisher_t *publisher, state_machine_event_t *UNUSED event)
{
    // If we got a DHCP prefix
    if (omr_publisher_dhcp_prefix_available(publisher)) {
        if (!omr_publisher_medium_or_high_prefix_present(publisher)) {
            INFO("got a DHCP prefix, no competing prefixes present");
            return omr_publisher_state_publishing_dhcp;
        } else {
            // If there's a medium or high priority prefix already published, don't publish another.
            // This shouldn't happen, but if it does, discontinue DHCP (it should already have been discontinued).
            INFO("medium- or high-priority prefix present, so discontinuing DHCP");
            omr_publisher_discontinue_dhcp(publisher);
        }
    }
    return omr_publisher_state_invalid;
}

// In the startup state, we wait to learn about prefixes, or for a timeout to occur. If a prefix shows up that's of medium or
// high priority, we don't need to do DHCP, so go to not_publishing. If it's a low priority prefix, we start DHCP, but won't publish
// anything other than a DHCP prefix.
static state_machine_state_t
omr_publisher_action_startup(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        publisher->omr_priority = omr_prefix_priority_invalid;
        if (publisher->wakeup_timer == NULL) {
            publisher->wakeup_timer = ioloop_wakeup_create();
        }
        if (publisher->wakeup_timer == NULL) {
            ERROR("unable to allocate a wakeup timer");
            return omr_publisher_state_invalid;
        }
        ioloop_add_wake_event(publisher->wakeup_timer, publisher, omr_publisher_wait_expired,
                              omr_publisher_wakeup_release,
                              publisher->min_start + srp_random16() % OMR_PUBLISHER_START_WAIT);
        publisher->min_start = 0; // Only need mandatory 3-second startup delay when first joining Thread network.
        RETAIN_HERE(publisher, omr_publisher); // For wakeup
        return omr_publisher_state_invalid;
    }
    // The only way out of the startup state is for the timer to expire--we don't care about prefixes showing up or
    // going away.
    if (event->type == state_machine_event_type_timeout) {
        INFO("startup timeout");
        return omr_publisher_state_check_for_dhcp;
    } else {
        BR_UNEXPECTED_EVENT(publisher, event);
    }
}

// In this state we haven't seen an OMR prefix yet, but we want to give DHCPv6 PD a chance to succeed before
// configuring an OMR prefix so that we don't push and then remove an OMR prefix. We also land in this state if
// we have seen a low-priority prefix; if we get a DHCP response, we'll publish that, and that should supersede
// the low-priority prefix.
static state_machine_state_t
omr_publisher_action_check_for_dhcp(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        if (publisher->wakeup_timer != NULL) {
            ioloop_cancel_wake_event(publisher->wakeup_timer);
        } else {
            publisher->wakeup_timer = ioloop_wakeup_create();
        }
        if (publisher->wakeup_timer == NULL) {
            ERROR("unable to allocate a wakeup timer");
            return omr_publisher_state_invalid;
        }
        ioloop_add_wake_event(publisher->wakeup_timer, publisher, omr_publisher_wait_expired,
                              omr_publisher_wakeup_release, OMR_PUBLISHER_DHCP_SUCCESS_WAIT);
        RETAIN_HERE(publisher, omr_publisher); // for the wakeup timer
        if (publisher->dhcp_client == NULL) {
            omr_publisher_initiate_dhcp(publisher);
        }
        return omr_publisher_state_invalid;
    }
    if (event->type == state_machine_event_type_timeout) {
        // We didn't get a DHCPv6 prefix quickly (might still get one later)
        INFO("timed out waiting for DHCP");
        if (omr_publisher_low_prefix_present(publisher)) {
            INFO("competing low priority prefix present");
            publisher->omr_priority = omr_prefix_priority_low;
            return omr_publisher_state_not_publishing;
        } else if (omr_publisher_medium_or_high_prefix_present(publisher)) {
            INFO("competing medium or high priority prefix present");
            omr_publisher_discontinue_dhcp(publisher);
            return omr_publisher_state_not_publishing;
        }
        return omr_publisher_state_publishing_ula;
    } else if (event->type == state_machine_event_type_prefix) {
        if (omr_publisher_medium_or_high_prefix_present(publisher)) {
            INFO("competing medium- or high priority prefix showed up");
            // We are trying to get a DHCPv6 prefix, so we need to stop.
            if (publisher->dhcp_client != NULL) {
                omr_publisher_discontinue_dhcp(publisher);
            }
            return omr_publisher_state_not_publishing;
        } else if (omr_publisher_low_prefix_present(publisher)) {
            INFO("competing low priority prefix showed up");
            publisher->omr_priority = omr_prefix_priority_low;
            return omr_publisher_state_not_publishing;
        } else {
            return omr_publisher_state_invalid;
        }
    } else if (event->type == state_machine_event_type_dhcp) {
        return omr_publisher_dhcp_event(publisher, event);
    } else {
        BR_UNEXPECTED_EVENT(publisher, event);
    }
}

static state_machine_state_t
omr_publisher_action_not_publishing(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        if (publisher->wakeup_timer != NULL) {
            ioloop_cancel_wake_event(publisher->wakeup_timer);
        }
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_prefix) {
        if (!omr_publisher_medium_or_high_prefix_present(publisher)) {
            if (publisher->dhcp_client == NULL) {
                INFO("lost competing medium or high-priority prefix");
                return omr_publisher_state_startup;
            } else if (!omr_publisher_low_prefix_present(publisher)) {
                INFO("lost competing low-priority prefix.");
                return omr_publisher_state_startup;
            }
        } else {
            // If we were looking for DHCP, stop looking.
            if (publisher->dhcp_client != NULL) {
                omr_publisher_discontinue_dhcp(publisher);
            }
        }
        // If we get to here, there is some kind of OMR prefix present, so we don't need to publish one.
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_dhcp) {
        return omr_publisher_dhcp_event(publisher, event);
    } else {
        BR_UNEXPECTED_EVENT(publisher, event);
    }
}

// We enter this state when we decide to publish a prefix we got from DHCP. On entry, we publish the prefix. If a high priority
// prefix shows up, we unpublish it and go to the not_publishing state. If a medium priority prefix shows up, we do an election, since
// that's expected to be a DHCP prefix also. If ours loses, we unpublish and go to not_publishing, otherwise we remain in this state and
// do nothing. If our DHCP prefix goes away and is replaced with a different prefix, we unpublish the old and publish the new. If it just
// goes away and no new prefix is given, then we unpublish the old prefix and go to the publishing_ula state.
static state_machine_state_t
omr_publisher_action_publishing_dhcp(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);
    if (event == NULL) {
        // Publish the DHCPv6 prefix
        omr_publisher_publish_dhcp_prefix(publisher);
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_prefix) {
        if (omr_publisher_high_prefix_present(publisher)) {
            INFO("deferring to high-priority prefix");
            omr_publisher_unpublish_dhcp_prefix(publisher);
            omr_publisher_discontinue_dhcp(publisher);
            return omr_publisher_state_not_publishing;
        } else if (omr_publisher_medium_prefix_present(publisher)) {
            if (!omr_publisher_medium_prefix_wins(publisher)) {
                INFO("deferring to winning medium-priority prefix");
                omr_publisher_unpublish_dhcp_prefix(publisher);
                omr_publisher_discontinue_dhcp(publisher);
                return omr_publisher_state_not_publishing;
            }
            return omr_publisher_state_invalid;
        }
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_dhcp) {
        if (omr_publisher_dhcp_prefix_lost(publisher)) {
            INFO("DHCP prefix withdrawn");
            omr_publisher_unpublish_dhcp_prefix(publisher);
            return omr_publisher_state_publishing_ula;
        } else if (omr_publisher_dhcp_prefix_changed(publisher)) {
            INFO("new prefix from DHCP server");
            omr_publisher_unpublish_dhcp_prefix(publisher);
            omr_publisher_publish_dhcp_prefix(publisher);
            return omr_publisher_state_invalid;
        }
        return omr_publisher_state_invalid;
    } else {
        BR_UNEXPECTED_EVENT(publisher, event);
    }
}

// We enter this state when no prefix showed up while we were waiting, and no DHCP prefix was given, or when we've unpublished
// our own DHCP prefix because it was withdrawn or expired. In order for this to happen, it has to be the case that no competing
// prefix is being published, so we're going to publish our prefix unconditionally. It's possible that some other BR will publish
// a DHCP prefix or a priority prefix, in which case we unpublish ours and go to not_publishing. It's also possible that another BR
// will publish a ULA prefix (low priority). In this case, we do an election, and if we lose, we unpublish ours and go to
// not_publishing.
static state_machine_state_t
omr_publisher_action_publishing_ula(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);
    if (event == NULL) {
        omr_publisher_publish_ula_prefix(publisher);
        if (publisher->dhcp_client == NULL) {
            FAULT("no DHCP client running!");
            omr_publisher_initiate_dhcp(publisher);
            publisher->omr_priority = omr_prefix_priority_low;
        }
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_prefix) {
        if (publisher->force_publication) {
            INFO("ignoring potentially competing prefix");
            return omr_publisher_state_invalid;
        }
        if (omr_publisher_medium_or_high_prefix_present(publisher)) {
            INFO("deferring to medium- or high-priority prefix");
            omr_publisher_unpublish_ula_prefix(publisher);
            return omr_publisher_state_not_publishing;
        } else if (omr_publisher_low_prefix_present(publisher)) {
            if (!omr_publisher_low_prefix_wins(publisher)) {
                INFO("deferring to winning low-priority prefix");
                omr_publisher_unpublish_ula_prefix(publisher);
                publisher->omr_priority = omr_prefix_priority_low;
                return omr_publisher_state_not_publishing;
            }
            return omr_publisher_state_invalid;
        }
        return omr_publisher_state_invalid;
    } else if (event->type == state_machine_event_type_dhcp) {
        if (publisher->force_publication) {
            INFO("ignoring dhcp prefix");
            return omr_publisher_state_invalid;
        } else if (omr_publisher_dhcp_prefix_available(publisher)) {
            INFO("switching to DHCP prefix");
            omr_publisher_unpublish_ula_prefix(publisher);
            return omr_publisher_state_publishing_dhcp;
        }
        return omr_publisher_state_invalid;
    } else {
        BR_UNEXPECTED_EVENT(publisher, event);
    }
}

void
omr_publisher_check_prefix(omr_publisher_t *publisher, struct in6_addr *prefix, int UNUSED len)
{
    if (publisher == NULL) {
        return;
    }
    if (publisher->published_prefix == NULL) {
        return;
    }
    // Make sure that this prefix, which we are seeing advetised on infrastructure, is not published as the OMR prefix.
    if (!in6prefix_compare(&publisher->published_prefix->prefix, prefix, 8)) {
        if (!in6prefix_compare(&publisher->ula_prefix, prefix, 8)) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->s6_addr, prefix_buf);
            FAULT("ULA prefix is being advertised on infrastructure: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->s6_addr, prefix_buf));
        } else {
            // If we get here it means that our DHCP prefix is bogus and we can't use it. So we're going to block DHCP, and treat this as
            // a DHCP prefix loss.
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->s6_addr, prefix_buf);
            ERROR("DHCP prefix is being advertised on infrastructure: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->s6_addr, prefix_buf));

            publisher->dhcp_wanted = false;
            publisher->dhcp_blocked = true;
            omr_publisher_dhcp_client_deactivate(publisher, (intptr_t)publisher->dhcp_client);
            omr_publisher_send_dhcp_event(publisher, NULL, 0, 0);
        }
    }
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
