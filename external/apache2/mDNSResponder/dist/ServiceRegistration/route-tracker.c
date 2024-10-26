/* route-tracker.c
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
 * This file contains the implementation for a route tracker for tracking prefixes and routes on infrastructure so that
 * they can be published on the Thread network.
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
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "cti-services.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "route.h"
#include "nat64.h"
#include "nat64-macos.h"
#include "adv-ctl-server.h"
#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "omr-publisher.h"
#include "route-tracker.h"

#ifdef BUILD_TEST_ENTRY_POINTS
#undef cti_remove_route
#undef cti_add_route
#define cti_remove_route cti_remove_route_test
#define cti_add_route cti_add_route_test

static int cti_add_route_test(srp_server_t *NULLABLE UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                              run_context_t NULLABLE UNUSED client_queue, struct in6_addr *NONNULL prefix,
                              int UNUSED prefix_length, int UNUSED priority, int UNUSED domain_id, bool UNUSED stable,
                              bool UNUSED nat64);
static int cti_remove_route_test(srp_server_t *NULLABLE UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                                 run_context_t NULLABLE UNUSED client_queue, struct in6_addr *NONNULL prefix,
                                 int UNUSED prefix_length, int UNUSED priority);
#endif

typedef struct prefix_tracker prefix_tracker_t;
struct prefix_tracker {
    int ref_count;
    prefix_tracker_t *next; // This is for the prefix advertise queue
    struct in6_addr prefix;
    int prefix_length;
    uint32_t preferred_lifetime, valid_lifetime;
    int num_routers;
    int new_num_routers;
    bool pending;
};

// The route tracker keeps a set of prefixes that it's tracking. These prefixes are what's published on the
// Thread mesh.
struct route_tracker {
    int ref_count;
    int max_prefixes;
    void (*reconnect_callback)(void *);
    route_state_t *route_state;
    char *name;
    prefix_tracker_t **prefixes;
    prefix_tracker_t *update_queue;
    interface_t *infrastructure;
    bool canceled;
    bool user_route_seen;
    bool blocked;
#ifdef BUILD_TEST_ENTRY_POINTS
    uint32_t current_mask, add_mask, remove_mask, intended_mask;
    cti_reply_t callback;
    int iterations;
#endif
};


static void route_tracker_add_prefix_to_queue(route_tracker_t *tracker, prefix_tracker_t *prefix);

#ifdef BUILD_TEST_ENTRY_POINTS
static void route_tracker_test_update_queue_empty(route_tracker_t *tracker);
#endif

static void
prefix_tracker_finalize(prefix_tracker_t *prefix)
{
    free(prefix);
}

static prefix_tracker_t *
prefix_tracker_create(struct in6_addr *prefix_bits, int prefix_length, uint32_t preferred_lifetime, uint32_t valid_lifetime)
{
    prefix_tracker_t *prefix = calloc(1, sizeof (*prefix));
    if (prefix == NULL) {
        ERROR("no memory for prefix");
        return NULL;
    }
    RETAIN_HERE(prefix, prefix_tracker);
    prefix->prefix = *prefix_bits;
    prefix->prefix_length = prefix_length;
    prefix->preferred_lifetime = preferred_lifetime;
    prefix->valid_lifetime = valid_lifetime;
    return prefix;
}

static void
route_tracker_finalize(route_tracker_t *tracker)
{
    free(tracker->prefixes);
    free(tracker->name);
    free(tracker);
}

#ifndef BUILD_TEST_ENTRY_POINTS
RELEASE_RETAIN_FUNCS(route_tracker);
#endif // BUILD_TEST_ENTRY_POINTS

void
route_tracker_cancel(route_tracker_t *tracker)
{
    if (tracker->prefixes != NULL) {
        for (int i = 0; i < tracker->max_prefixes; i++) {
            prefix_tracker_t *prefix = tracker->prefixes[i];
            if (prefix != NULL) {
                tracker->prefixes[i] = NULL;
                // If we have published a route to this prefix, queue it for removal.
                if (prefix->num_routers != 0) {
                    prefix->num_routers = 0;
                    route_tracker_add_prefix_to_queue(tracker, prefix);
                }
                RELEASE_HERE(prefix, prefix_tracker);
            }
        }
    }
#ifndef BUILD_TEST_ENTRY_POINTS
    if (tracker->infrastructure) {
        interface_release(tracker->infrastructure);
        tracker->infrastructure = NULL;
    }
#endif // BUILD_TEST_ENTRY_POINTS
    tracker->canceled = true;
}

route_tracker_t *
route_tracker_create(route_state_t *NONNULL route_state, const char *NONNULL name)
{
    route_tracker_t *ret = NULL, *tracker = calloc(1, sizeof(*tracker));
    if (tracker == NULL) {
        INFO("no memory for tracker");
        return tracker;
    }
    RETAIN_HERE(tracker, route_tracker);
    tracker->route_state = route_state;
    tracker->name = strdup(name);
    if (tracker->name == NULL) {
        goto out;
    }
    tracker->max_prefixes = 10;
    tracker->prefixes = calloc(tracker->max_prefixes, sizeof (*tracker->prefixes));
    if (tracker->prefixes == NULL) {
        INFO("no memory for prefix vector");
        goto out;
    }

    ret = tracker;
    tracker = NULL;
out:
    if (tracker != NULL) {
        RELEASE_HERE(tracker, route_tracker);
    }
    return ret;
}

static bool
route_tracker_add_prefix(route_tracker_t *tracker, prefix_tracker_t *prefix)
{
    int open_slot = -1;
    for (int i = 0; i < tracker->max_prefixes; i++) {
        if (tracker->prefixes[i] == NULL && open_slot == -1) {
            open_slot = i;
        }
        if (tracker->prefixes[i] == prefix) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d is already present",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf), prefix->prefix_length);
            return true;
        }
    }
    if (open_slot != -1) {
        tracker->prefixes[open_slot] = prefix;
        RETAIN_HERE(tracker->prefixes[open_slot], prefix_tracker);
        return true;
    }

    int new_max = tracker->max_prefixes * 2;
    prefix_tracker_t **prefixes = calloc(new_max, sizeof (*prefixes));
    if (prefixes == NULL) {
        INFO("no memory to add prefix");
        return false;
    }
    memcpy(prefixes, tracker->prefixes, tracker->max_prefixes * sizeof(*tracker->prefixes));
    free(tracker->prefixes);
    tracker->prefixes = prefixes;
    tracker->prefixes[tracker->max_prefixes] = prefix;
    RETAIN_HERE(tracker->prefixes[tracker->max_prefixes], prefix_tracker);
    tracker->max_prefixes = new_max;
    return true;
}


void
route_tracker_set_reconnect_callback(route_tracker_t *tracker, void (*reconnect_callback)(void *context))
{
    tracker->reconnect_callback = reconnect_callback;
}

void
route_tracker_start(route_tracker_t *tracker)
{
    INFO("starting tracker " PUB_S_SRP, tracker->name);
    // Immediately check to see if we can advertise a prefix.
#ifndef BUILD_TEST_ENTRY_POINTS
    route_tracker_interface_configuration_changed(tracker);
#endif
    return;
}

static void route_tracker_start_next_update(route_tracker_t *tracker);

static void
route_tracker_remove_prefix(route_tracker_t *tracker, prefix_tracker_t *prefix)
{
    for (int i = 0; i < tracker->max_prefixes; i++) {
        if (tracker->prefixes[i] == prefix) {
            RELEASE_HERE(tracker->prefixes[i], prefix_tracker);
            tracker->prefixes[i] = NULL;
            return;
        }
    }
}

static void
route_tracker_update_callback(void *context, cti_status_t status)
{
    route_tracker_t *tracker = context;
    prefix_tracker_t *prefix = tracker->update_queue;
    INFO("status %d", status);
    if (tracker->update_queue == NULL) {
        ERROR("update seems to have disappeared");
#ifdef BUILD_TEST_ENTRY_POINTS
        route_tracker_test_update_queue_empty(tracker);
#endif
        return;
    }
    if (prefix->pending) {
        prefix->pending = false;
        tracker->update_queue = prefix->next;
        prefix->next = NULL;
        if (prefix->num_routers == 0) {
            route_tracker_remove_prefix(tracker, prefix);
        }
        RELEASE_HERE(prefix, prefix_tracker);
    }
    if (tracker->update_queue != NULL) {
        route_tracker_start_next_update(tracker);
    } else {
        // The update queue holds a reference to the tracker when there is something on the
        // queue.
        RELEASE_HERE(tracker, route_tracker);
#ifdef BUILD_TEST_ENTRY_POINTS
        route_tracker_test_update_queue_empty(tracker);
#endif
    }
}

static void
route_tracker_start_next_update(route_tracker_t *tracker)
{
    prefix_tracker_t *prefix = tracker->update_queue;
    if (prefix == NULL) {
        ERROR("start_next_update called with no update");
        return;
    }

    cti_status_t status;

    // If num_routers is zero, remove the prefix.
    if (prefix->num_routers == 0) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
        INFO("removing route: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf), prefix->prefix_length);
        status = cti_remove_route(tracker->route_state->srp_server, tracker, route_tracker_update_callback,
                                  NULL, &prefix->prefix, prefix->prefix_length, 0);
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
        INFO("  adding route: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf), prefix->prefix_length);
        status = cti_add_route(tracker->route_state->srp_server, tracker, route_tracker_update_callback,
                               NULL, &prefix->prefix, prefix->prefix_length,
                               offmesh_route_preference_medium, 0, true, false);
    }
    if (status != kCTIStatus_NoError) {
        ERROR("route update failed: %d", status);
    } else {
        prefix->pending = true;
    }
}

static void
route_tracker_add_prefix_to_queue(route_tracker_t *tracker, prefix_tracker_t *prefix)
{
    prefix_tracker_t **ptp, *old_queue = tracker->update_queue;
    // Find the prefix on the queue, or find the end of the queue.
    for (ptp = &tracker->update_queue; *ptp != NULL && *ptp != prefix; ptp = &(*ptp)->next)
        ;
    // Not on the queue...
    if (*ptp == NULL) {
        *ptp = prefix;
        RETAIN_HERE(prefix, prefix_tracker);
        // Turns out we added it to the beginning of the queue.
        if (tracker->update_queue == prefix) {
            route_tracker_start_next_update(tracker);
        }
        goto out;
    }
    // We have started to update the prefix, but haven't gotten the callback yet. Since we have put the prefix
    // back on the update queue, and it's at the beginning, mark it not pending so that when we get the callback
    // from the update function, we update this route again rather than going on to the next.
    if (prefix == tracker->update_queue) {
        prefix->pending = false;
    }
    // If we get to here, the prefix is already on the update queue and its update hasn't started yet, so we can just leave it.
out:
    // As long as there is anything in the queue, the queue needs to hold a reference to the tracker, so that if it's
    // canceled and released, we finish running the queue before stopping.
    if (old_queue == NULL && tracker->update_queue != NULL) {
        RETAIN_HERE(tracker, route_tracker);
    }
}

static void
route_tracker_track_prefix(route_tracker_t *tracker, struct in6_addr *prefix_bits, int prefix_length,
                           uint32_t preferred_lifetime, uint32_t valid_lifetime, bool count)
{
    prefix_tracker_t *prefix = NULL;
    int i;
    for (i = 0; i < tracker->max_prefixes; i++) {
        prefix = tracker->prefixes[i];
        if (prefix == NULL) {
            continue;
        }
        if (prefix->prefix_length == prefix_length && !in6addr_compare(&prefix->prefix, prefix_bits)) {
            break;
        }
    }
    if (i == tracker->max_prefixes) {
        prefix = prefix_tracker_create(prefix_bits, prefix_length, preferred_lifetime, valid_lifetime);
        if (prefix == NULL) {
            return;
        }
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix_bits, prefix_buf);
        INFO("adding prefix " PRI_SEGMENTED_IPv6_ADDR_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix_bits, prefix_buf));
        if (!route_tracker_add_prefix(tracker, prefix)) {
            // Weren't able to add it.
            RELEASE_HERE(prefix, prefix_tracker);
            return;
        }
        if (count) {
            prefix->new_num_routers++;
        }
        // To avoid confusion, route_tracker_add_prefix retains the prefix. That means the reference we got from
        // creating it is still held, so release it.
        RELEASE_HERE(prefix, prefix_tracker);
    } else {
        if (count && prefix != NULL) {
            prefix->new_num_routers++;
        }
    }
}

#ifndef BUILD_TEST_ENTRY_POINTS
#if SRP_FEATURE_PUBLISH_SPECIFIC_ROUTES
static void
route_tracker_count_prefixes(route_tracker_t *tracker, icmp_message_t *router, bool have_routable_omr_prefix)
{
    icmp_option_t *router_option = router->options;
    for (int i = 0; i < router->num_options; i++, router_option++) {
        // Always track PIO if it's on-link
        if (router_option->type == icmp_option_prefix_information &&
            (router_option->option.route_information.flags & ND_OPT_PI_FLAG_ONLINK))
        {
            route_tracker_track_prefix(tracker,
                                       &router_option->option.prefix_information.prefix,
                                       router_option->option.prefix_information.length,
                                       router_option->option.prefix_information.preferred_lifetime,
                                       router_option->option.prefix_information.valid_lifetime, true);
        } else if (have_routable_omr_prefix && router_option->type == icmp_option_prefix_information) {
            route_tracker_track_prefix(tracker,
                                       &router_option->option.route_information.prefix,
                                       router_option->option.route_information.length,
                                       router_option->option.route_information.route_lifetime,
                                       router_option->option.route_information.route_lifetime, true);
        }
    }
}
#endif
#endif // BUILD_TEST_ENTRY_POINTS

static void
route_tracker_reset_counts(route_tracker_t *tracker)
{
    // Set num_routers to zero on each prefix
    for (int i = 0; i < tracker->max_prefixes; i++) {
        prefix_tracker_t *prefix = tracker->prefixes[i];
        if (prefix != NULL) {
            prefix->new_num_routers = 0;
        }
    }
}

static void
route_tracker_publish_changes(route_tracker_t *tracker)
{
    // Now go through the prefixes and queue updates for anything that changed.
    for (int i = 0; i < tracker->max_prefixes; i++) {
        prefix_tracker_t *prefix = tracker->prefixes[i];
        if (prefix != NULL) {
            // If the number of routers advertising the prefix changed, and the total number of routers either went to zero
            // or was previously zero, put the prefix on the queue to publish.
            if (prefix->new_num_routers != prefix->num_routers &&
                (prefix->new_num_routers == 0 || prefix->num_routers == 0))
            {
                prefix->num_routers = prefix->new_num_routers;
                route_tracker_add_prefix_to_queue(tracker, prefix);
                SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
                INFO("(not) " PUB_S_SRP " prefix " PRI_SEGMENTED_IPv6_ADDR_SRP, prefix->num_routers ? "adding" : "removing",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf));
            } else {
                // Update the number of routers, but don't do anything else.
                prefix->num_routers = prefix->new_num_routers;
            }
        }
    }
}

bool
route_tracker_check_for_gua_prefixes_on_infrastructure(route_tracker_t *tracker)
{
    bool present = false;
    interface_t *interface = tracker->infrastructure;
    if (tracker == NULL || interface == NULL) {
        goto out;
    }
    for (icmp_message_t *router = interface->routers; router != NULL; router = router->next) {
        for (int i = 0; i < router->num_options; i++) {
            icmp_option_t *option = &router->options[i];
            if (option->type == icmp_option_prefix_information) {
                prefix_information_t *prefix = &option->option.prefix_information;
                if (omr_watcher_prefix_is_non_ula_prefix(prefix)) {
                    present = true;
                    goto done;
                }
            } else if (option->type == icmp_option_route_information) {
                route_information_t *rio = &option->option.route_information;
                if (omr_watcher_prefix_is_non_ula_prefix(rio)) {
                    present = true;
                    goto done;
                }
            }
        }
    }
done:
    INFO("interface " PUB_S_SRP ": checked for GUAs on infrastructure: " PUB_S_SRP "present",
         interface->name, present ? "" : "not ");
out:
    return present;
}


#ifndef BUILD_TEST_ENTRY_POINTS
void
route_tracker_route_state_changed(route_tracker_t *tracker, interface_t *interface)
{
    if (tracker->blocked) {
        INFO("tracker is blocked");
        return;
    }
    if (tracker->route_state == NULL) {
        ERROR("tracker has no route_state");
        return;
    }
    route_state_t *route_state = tracker->route_state;

    bool have_routable_omr_prefix;
    if (route_state->omr_publisher != NULL && omr_publisher_have_routable_prefix(route_state->omr_publisher)) {
        have_routable_omr_prefix = true;
    } else {
        have_routable_omr_prefix = false;
    }
    if (interface != tracker->infrastructure) {
        return;
    }
    bool need_default_route = have_routable_omr_prefix;
    // We need a default route if there is a routable omr prefix, but also if there are GUA prefixes on the adjacent
    // infrastructure link or reachable via the adjacent infrastructure link's router(s).
    if (interface != NULL && !need_default_route) {
        need_default_route = route_tracker_check_for_gua_prefixes_on_infrastructure(tracker);
    }
    INFO("interface: " PUB_S_SRP PUB_S_SRP " need default route, " PUB_S_SRP " routable OMR prefix",
         interface != NULL ? interface->name : "(no interface)", need_default_route ? "" : " don't",
         have_routable_omr_prefix ? "have" : "don't have");

    route_tracker_reset_counts(tracker);

    // If we have no interface, then all we really care about is that any routes we're publishing should be
    // removed.
    if (interface != NULL) {
        static struct in6_addr prefix;
        int width = 0;
        if (need_default_route) {
            ((uint8_t *)&prefix)[0] = 0;
        } else {
            ((uint8_t *)&prefix)[0] = 0xfc;
            width = 7;
        }
        route_tracker_track_prefix(tracker, &prefix, width, 1800, 1800, true);
    }
#if SRP_FEATURE_NAT64
    nat64_omr_route_update(route_state->nat64, have_routable_omr_prefix);
#endif
    route_tracker_publish_changes(tracker);
}

void
route_tracker_interface_configuration_changed(route_tracker_t *tracker)
{
    interface_t *preferred = NULL;
    if (tracker->blocked) {
        INFO("tracker is blocked");
        return;
    }
    if (tracker->route_state == NULL) {
        ERROR("tracker has no route_state");
        return;
    }
    route_state_t *route_state = tracker->route_state;

    for (interface_t *interface = route_state->interfaces; interface; interface = interface->next) {
        if (interface->ip_configuration_service != NULL) {
            preferred = interface;
            break;
        }
        if (!interface->inactive && !interface->ineligible) {
            if (tracker->infrastructure == interface) {
                preferred = interface;
                break;
            }
            if (preferred != NULL) {
                FAULT("more than one infra interface: " PUB_S_SRP " and " PUB_S_SRP " (picked)", interface->name, preferred->name);
            } else {
                preferred = interface;
            }
        }
    }
    if (preferred == NULL) {
        if (tracker->infrastructure != NULL) {
            interface_release(tracker->infrastructure);
            tracker->infrastructure = NULL;
            INFO("no infrastructure");
            route_tracker_route_state_changed(tracker, NULL);
        }
    } else {
        if (tracker->infrastructure != preferred) {
            if (tracker->infrastructure != NULL) {
                interface_release(tracker->infrastructure);
                tracker->infrastructure = NULL;
            }
            INFO("preferred infrastructure interface is now " PUB_S_SRP, preferred->name);
#if SRP_FEATURE_NAT64
            nat64_pass_all_pf_rule_set(preferred->name);
#endif // SRP_FEATURE_NAT64
            tracker->infrastructure = preferred;
            interface_retain(tracker->infrastructure);
            route_tracker_route_state_changed(tracker, tracker->infrastructure);
        }
    }
}

void
route_tracker_monitor_mesh_routes(route_tracker_t *tracker, cti_route_vec_t *routes)
{
    tracker->user_route_seen = false;
    for (size_t i = 0; i < routes->num; i++) {
        cti_route_t *route = routes->routes[i];
        if (route && route->origin == offmesh_route_origin_user) {
            route_tracker_track_prefix(tracker, &route->prefix, route->prefix_length, 100, 100, false);
            tracker->user_route_seen = true;
        }
    }
    if (!tracker->user_route_seen && tracker->route_state->srp_server->awaiting_route_removal) {
        tracker->route_state->srp_server->awaiting_route_removal = false;
        adv_ctl_thread_shutdown_status_check(tracker->route_state->srp_server);
    }
}

bool
route_tracker_local_routes_seen(route_tracker_t *tracker)
{
    if (tracker != NULL) {
        return tracker->user_route_seen;
    }
    return false;
}

void
route_tracker_shutdown(route_state_t *route_state)
{
    if (route_state == NULL || route_state->route_tracker == NULL) {
        return;
    }
    route_tracker_reset_counts(route_state->route_tracker);
    route_tracker_publish_changes(route_state->route_tracker);
    route_state->route_tracker->blocked = true;
}
#else // !defined(BUILD_TEST_ENTRY_POINTS)

static void
route_tracker_remove_callback(void *context)
{
    route_tracker_update_callback(context, 0);
}

static void
route_tracker_test_route_update(void *context, struct in6_addr *prefix, cti_reply_t callback, bool remove)
{
    route_tracker_t *tracker = context;

    for (int i = 0; i < 4; i++) {
        if (prefix->s6_addr[i] != 0) {
            for (int j = 0; j < 8; j++) {
                if (prefix->s6_addr[i] == (1 << j)) {
                    int bit = i * 8 + j;
                    if (remove) {
                        tracker->remove_mask |= (1 << bit);
                        INFO("bit %d removed", bit);
                    } else {
                        tracker->add_mask |= (1 << bit);
                        INFO("bit %d added", bit);
                    }
                    tracker->callback = callback;
                    ioloop_run_async(route_tracker_remove_callback, tracker);
                    return;
                }
            }
        }
    }
    INFO("no bit");
}

int
cti_add_route_test(srp_server_t *NULLABLE UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                   run_context_t NULLABLE UNUSED client_queue, struct in6_addr *NONNULL prefix,
                   int UNUSED prefix_length, int UNUSED priority, int UNUSED domain_id, bool UNUSED stable,
                   bool UNUSED nat64)
{
    route_tracker_test_route_update(context, prefix, callback, false);
    return 0;
}

int
cti_remove_route_test(srp_server_t *NULLABLE UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                      run_context_t NULLABLE UNUSED client_queue, struct in6_addr *NONNULL prefix,
                      int UNUSED prefix_length, int UNUSED priority)
{
    route_tracker_test_route_update(context, prefix, callback, true);
    return 0;
}

static void
route_tracker_test_announce_prefixes_in_mask(route_tracker_t *tracker, uint32_t mask)
{
    struct in6_addr prefix;

    tracker->intended_mask = mask;
    route_tracker_reset_counts(tracker);

    for (int i = 0; i < 32; i++) {
        if ((mask & (1 << i)) != 0) {
            INFO("%x   is in   %x", 1 << i, mask);
            int byte = i / 8;
            uint8_t bit = (uint8_t)(1 << (i & 7));

            in6addr_zero(&prefix);
            prefix.s6_addr[byte] = bit;

            SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix, prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " byte %d bit %x i %d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix, prefix_buf), byte, bit, i);

            route_tracker_track_prefix(tracker, &prefix, 64, 100, 100, true);
        } else {
            INFO("%x is not in %x", 1 << i, mask);
        }
    }
    route_tracker_publish_changes(tracker);
    // If we get the same mask twice in a row, there will be no updates, so we won't get any callbacks.
    if (tracker->update_queue == NULL) {
        route_tracker_test_update_queue_empty(tracker);
    }
}

static void
route_tracker_test_iterate(route_tracker_t *tracker)
{
    tracker->intended_mask = srp_random32();
    route_tracker_test_announce_prefixes_in_mask(tracker, tracker->intended_mask);
}

route_state_t *test_route_state;
srp_server_t *test_srp_server;

void
route_tracker_test_start(int iterations)
{
	test_srp_server = calloc(1, sizeof(*test_srp_server));
	if (test_srp_server == NULL) {
		ERROR("no memory for srp state");
		exit(1);
	}
	test_route_state = calloc(1, sizeof(*test_route_state));
	if (test_route_state == NULL) {
		ERROR("no memory for route state");
		exit(1);
	}
    test_route_state->srp_server = test_srp_server;
	test_route_state->route_tracker = route_tracker_create(test_route_state, "test");
    test_route_state->route_tracker->iterations = iterations;
    route_tracker_start(test_route_state->route_tracker);
    route_tracker_test_iterate(test_route_state->route_tracker);
}

static void
route_tracker_test_update_queue_empty(route_tracker_t *tracker)
{
    uint32_t result = ((tracker->current_mask & ~tracker->remove_mask)) | tracker->add_mask;
    INFO("current: %x intended: %x  result: %x  add: %x  remove: %x",
         tracker->current_mask, tracker->intended_mask, result, tracker->add_mask, tracker->remove_mask);
    if (tracker->intended_mask != result) {
        ERROR("test failed.");
        exit(1);
    }
    tracker->current_mask = tracker->intended_mask;
    tracker->add_mask = 0;
    tracker->remove_mask = 0;
    tracker->intended_mask = 0;
    if (tracker->iterations != 0) {
        tracker->iterations--;
        route_tracker_test_iterate(tracker);
    } else {
        INFO("test completed");
        exit(0);
    }
}
#endif //  BUILD_TEST_ENTRY_POINTS

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
