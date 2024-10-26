/* route.c
 *
 * Copyright (c) 2019-2024 Apple Inc. All rights reserved.
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
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * This file contains an implementation of Thread Border Router routing.
 * The state of the Thread network is tracked, the state of the infrastructure
 * network is tracked, and policy decisions are made as to what to advertise
 * on both networks.
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
#if !USE_SYSCTL_COMMAND_TO_ENABLE_FORWARDING
#ifndef LINUX
#include <sys/sysctl.h>
#endif // LINUX
#endif // !USE_SYSCTL_COMMAND_TO_ENABLE_FORWARDING
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
#endif // IOLOOP_MACOS

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-crypto.h"
#include "srp-gw.h"
#include "srp-mdns-proxy.h"
#include "adv-ctl-server.h"
#include "srp-replication.h"


# define THREAD_DATA_DIR "/var/lib/openthread"
# define THREAD_ULA_FILE THREAD_DATA_DIR "/thread-mesh-ula"

#if STUB_ROUTER // Stub Router is true if we're building a Thread Border router or an RA tester.
#ifdef THREAD_BORDER_ROUTER
#include "cti-services.h"
#endif
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#if SRP_FEATURE_NAT64
#include "nat64-macos.h"
#endif
#include "srp-proxy.h"
#include "route.h"
#include "nat64.h"

#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"
#include "omr-watcher.h"
#include "omr-publisher.h"
#include "route-tracker.h"
#include "icmp.h"


#ifdef LINUX
#define CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IFCONFIG 1
#endif

#ifdef LINUX
struct in6_addr in6addr_linklocal_allnodes = {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}};
struct in6_addr in6addr_linklocal_allrouters = {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }}};
#endif

route_state_t *route_states;

#define CONFIGURE_STATIC_INTERFACE_ADDRESSES 1

#define interface_create(route_state, name, iface) interface_create_(route_state, name, iface, __FILE__, __LINE__)
interface_t *NULLABLE interface_create_(route_state_t *NONNULL route_state, const char *NONNULL name, int ifindex,
                                        const char *NONNULL file, int line);

static void interface_beacon_schedule(interface_t *NONNULL interface, unsigned when);
static void interface_prefix_configure(struct in6_addr prefix, interface_t *NONNULL interface);
static void interface_prefix_evaluate(interface_t *interface);
static void start_router_solicit(interface_t *interface);
#ifndef RA_TESTER
static void attempt_wpan_reconnect(void *context);
static void routing_policy_evaluate_all_interfaces(route_state_t *route_state, bool assume_changed);
#endif
static void routing_policy_evaluate(interface_t *interface, bool assume_changed);
static void post_solicit_policy_evaluate(void *context);
static void schedule_next_router_probe(interface_t *interface);

#ifndef RA_TESTER
static void thread_network_startup(route_state_t *route_state);
static void thread_network_shutdown(route_state_t *route_state);
static void thread_network_shutdown_start(route_state_t *route_state);
static void partition_state_reset(route_state_t *route_state);
static void partition_utun0_address_changed(route_state_t *route_state, const struct in6_addr *NONNULL addr, enum interface_address_change change);
static void partition_utun0_pick_listener_address(route_state_t *route_state);
static void partition_got_tunnel_name(route_state_t *route_state);
static void partition_remove_service_done(void *UNUSED NULLABLE context, cti_status_t status);
static void partition_stop_advertising_service(route_state_t *route_state);
static void partition_proxy_listener_ready(void *UNUSED NULLABLE context, uint16_t port);
static void partition_maybe_advertise_service(route_state_t *route_state);
static void partition_service_set_changed(void *context);
static void partition_maybe_enable_services(route_state_t *route_state);
static void partition_disable_service(route_state_t *route_state);
void partition_discontinue_srp_service(route_state_t *route_state);
static void partition_schedule_service_add_wakeup(route_state_t *route_state);
static void partition_schedule_anycast_service_add_wakeup(route_state_t *route_state);
#endif

route_state_t *
route_state_create(srp_server_t *server_state, const char *name)
{
    route_state_t *new_route_state = calloc(1, sizeof(*new_route_state));
    if (new_route_state == NULL || (new_route_state->name = strdup(name)) == NULL) {
        free(new_route_state);
        ERROR("no memory for route state.");
        return NULL;
    }
#if !defined(RA_TESTER)
    new_route_state->thread_network_running = false;
    new_route_state->partition_may_offer_service = false;
    new_route_state->partition_settle_satisfied = true;
    new_route_state->current_thread_state = kCTI_NCPState_Uninitialized;
#endif
    new_route_state->have_non_thread_interface = false;
    new_route_state->ula_serial = 1;
    new_route_state->have_xpanid_prefix = false;
    new_route_state->have_thread_prefix = false;
    new_route_state->config_enable_dhcpv6_prefixes = false;
    new_route_state->srp_server = server_state; // temporarily communicate the server_state object to route.c with a static assignment.
    new_route_state->next = route_states;
    route_states = new_route_state;
    return new_route_state;
}

static void
interface_finalize(void *context)
{
    interface_t *interface = context;
    if (interface->name != NULL) {
        free(interface->name);
    }
    if (interface->beacon_wakeup != NULL) {
        ioloop_wakeup_release(interface->beacon_wakeup);
    }
    if (interface->post_solicit_wakeup != NULL) {
        ioloop_wakeup_release(interface->post_solicit_wakeup);
    }
    if (interface->stale_evaluation_wakeup != NULL) {
        ioloop_wakeup_release(interface->stale_evaluation_wakeup);
    }
    if (interface->router_solicit_wakeup != NULL) {
        ioloop_wakeup_release(interface->router_solicit_wakeup);
    }
    if (interface->deconfigure_wakeup != NULL) {
        ioloop_wakeup_release(interface->deconfigure_wakeup);
    }
    if (interface->neighbor_solicit_wakeup != NULL) {
        ioloop_wakeup_release(interface->neighbor_solicit_wakeup);
    }
    if (interface->router_probe_wakeup != NULL) {
        ioloop_wakeup_release(interface->router_probe_wakeup);
    }
    free(interface);
}

interface_t *
interface_create_(route_state_t *route_state, const char *name, int ifindex, const char *file, int line)
{
    interface_t *ret;

    if (name == NULL) {
        ERROR("interface_create: missing name");
        return NULL;
    }

    ret = calloc(1, sizeof(*ret));
    if (ret) {
        RETAIN(ret, interface);
        ret->name = strdup(name);
        if (ret->name == NULL) {
            ERROR("interface_create: no memory for name");
            RELEASE(ret, interface);
            return NULL;
        }
        ret->deconfigure_wakeup = ioloop_wakeup_create();
        if (ret->deconfigure_wakeup == NULL) {
            ERROR("No memory for interface deconfigure wakeup on " PUB_S_SRP ".", ret->name);
            RELEASE(ret, interface);
            return NULL;
        }

        ret->route_state = route_state;
        ret->index = ifindex;
        ret->previously_inactive = true;
        ret->inactive = true;
        ret->previously_ineligible = true;
        if (!strcmp(name, "lo") || !strcmp(name, "wpan0")) {
            ret->ineligible = true;
        } else {
            ret->ineligible = false;
        }
    }
    return ret;
}

void interface_retain_(interface_t *NONNULL interface, const char *file, int line)
{
    RETAIN(interface, interface);
}

void interface_release_(interface_t *NONNULL interface, const char *file, int line)
{
    RELEASE(interface, interface);
}

#ifndef RA_TESTER
#endif // RA_TESTER

static void
interface_prefix_deconfigure(void *context)
{
    interface_t *interface = context;
    INFO("post solicit wakeup.");

    if (interface->preferred_lifetime != 0) {
        INFO("PUT PREFIX DECONFIGURE CODE HERE!!");
        interface->valid_lifetime = 0;
    }
    interface->deprecate_deadline = 0;
}

static bool
want_routing(route_state_t *route_state)
{
#ifdef RA_TESTER
    (void)route_state;
    return true;
#else
    return (route_state->partition_can_provide_routing &&
            route_state->partition_has_xpanid);
#endif
}

static void
interface_beacon_send(interface_t *interface, const struct in6_addr *destination)
{
    uint64_t now = ioloop_timenow();
#ifndef RA_TESTER
    route_state_t *route_state = interface->route_state;
#endif

    INFO(PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP,
         interface->deprecate_deadline > now ? " ddl>now" : "",
#ifdef RA_TESTER
         "", "", "",
#else
         route_state->partition_can_provide_routing ? " canpr" : " !canpr",
         route_state->partition_has_xpanid ? " havexp" : " !havexp",
         interface->suppress_ipv6_prefix ? " suppress" : " !suppress",
#endif
         interface->our_prefix_advertised ? " advert" : " !advert",
         interface->sent_first_beacon ? "" : " first beacon");

    if (interface->deprecate_deadline > now) {
        // The remaining valid and preferred lifetimes is the time left until the deadline.
        interface->valid_lifetime = (uint32_t)((interface->deprecate_deadline - now) / 1000);
        interface->preferred_lifetime = 0;
        if (interface->valid_lifetime < icmp_listener.unsolicited_interval / 1000) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf);
            INFO("prefix valid life time is less than the unsolicited interval, stop advertising it "
                 "and prepare to deconfigure the prefix - ifname: " PUB_S_SRP "prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP
                 ", preferred time: %" PRIu32 ", valid time: %" PRIu32 ", unsolicited interval: %" PRIu32,
                 interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf),
                 interface->preferred_lifetime, interface->valid_lifetime, icmp_listener.unsolicited_interval / 1000);
            interface->our_prefix_advertised = false;
            ioloop_add_wake_event(interface->deconfigure_wakeup,
                                  interface, interface_prefix_deconfigure,
                                  NULL, interface->valid_lifetime * 1000);
        }
    }

#ifndef RA_TESTER
    // If we have been beaconing, and router mode has been disabled, and we don't have
    // an on-link prefix to advertise, discontinue beaconing.
    if (want_routing(route_state) || interface->our_prefix_advertised) {
#endif

    // Send an RA.
        router_advertisement_send(interface, destination);
        if (destination == &in6addr_linklocal_allnodes) {
            interface->sent_first_beacon = true;
            interface->last_beacon = ioloop_timenow();;
        }
#ifndef CONTINUE_ADVERTISING_DURING_DEPRECATION
        // If we are deprecating, just send the initial deprecation to shorten the preferred lifetime, and then go silent.
        if (interface->deprecate_deadline > now && !interface->suppress_ipv6_prefix) {
            INFO("suppressing ipv6 prefix on " PUB_S_SRP, interface->name);
            interface->suppress_ipv6_prefix = true;
        }
#endif

#ifndef RA_TESTER
    } else {
        INFO("didn't send: " PUB_S_SRP PUB_S_SRP PUB_S_SRP,
             route_state->partition_can_provide_routing ? "canpr" : "!canpr",
             route_state->partition_has_xpanid ? " route_state->xpanid" : " !route_state->xpanid",
             interface->our_prefix_advertised ? " advert" : " !advert");
    }
#endif
    if (destination == &in6addr_linklocal_allnodes) {
        if (interface->num_beacons_sent < MAX_RA_RETRANSMISSION - 1) {
            // Schedule a beacon for between 8 and 16 seconds in the future (<MAX_INITIAL_RTR_ADVERT_INTERVAL)
            interface_beacon_schedule(interface, 8000 + srp_random16() % 8000);
        } else {
            interface_beacon_schedule(interface, icmp_listener.unsolicited_interval);
        }
        interface->num_beacons_sent++;
    }
}

static void
interface_beacon(void *context)
{
    interface_t *interface = context;
    interface_beacon_send(interface, &in6addr_linklocal_allnodes);
}

static void
interface_beacon_schedule(interface_t *interface, unsigned when)
{
    uint64_t now = ioloop_timenow();
    unsigned interval;


    // Make sure we haven't send an RA too recently.
    if (when < MIN_DELAY_BETWEEN_RAS && now - interface->last_beacon < MIN_DELAY_BETWEEN_RAS) {
        when = MIN_DELAY_BETWEEN_RAS;
    }
    // Add up to a second of jitter.
    when += srp_random16() % 1024;
    interface->next_beacon = now + when;
    if (interface->beacon_wakeup == NULL) {
        interface->beacon_wakeup = ioloop_wakeup_create();
        if (interface->beacon_wakeup == NULL) {
            ERROR("Unable to allocate beacon wakeup for " PUB_S_SRP, interface->name);
            return;
        }
    } else {
        // We can reschedule a beacon for sooner if we get a router solicit; in this case, we
        // need to cancel the existing beacon wakeup, and if there is none scheduled, this will
        // be a no-op.
        ioloop_cancel_wake_event(interface->beacon_wakeup);
    }
    if (interface->next_beacon - now > UINT_MAX) {
        interval = UINT_MAX;
    } else {
        interval = (unsigned)(interface->next_beacon - now);
    }
    INFO("Scheduling " PUB_S_SRP "beacon on " PUB_S_SRP " for %u milliseconds in the future",
         interface->sent_first_beacon ? "" : "first ", interface->name, interval);
    ioloop_add_wake_event(interface->beacon_wakeup, interface, interface_beacon, NULL, interval);
}

static void
router_discovery_start(interface_t *interface)
{
    INFO("Starting router discovery on " PUB_S_SRP, interface->name);

    // Immediately when an interface shows up, start doing router solicits.
    start_router_solicit(interface);

    if (interface->post_solicit_wakeup == NULL) {
        interface->post_solicit_wakeup = ioloop_wakeup_create();
        if (interface->post_solicit_wakeup == NULL) {
            ERROR("No memory for post-solicit RA wakeup on " PUB_S_SRP ".", interface->name);
        }
    } else {
        ioloop_cancel_wake_event(interface->post_solicit_wakeup);
    }

    // In 20 seconds, check the results of router discovery and update policy as needed.
    if (interface->post_solicit_wakeup) {
        ioloop_add_wake_event(interface->post_solicit_wakeup, interface, post_solicit_policy_evaluate,
                              NULL, 20 * 1000);
    }
    interface->router_discovery_in_progress = true;
    interface->router_discovery_started = true;
}

static void
flush_routers(interface_t *interface, uint64_t now)
{
    icmp_message_t *router, **p_router;

    // Flush stale routers (or all routers).
    for (p_router = &interface->routers; *p_router != NULL; ) {
        router = *p_router;
        if (now == 0 || now - router->received_time > MAX_ROUTER_RECEIVED_TIME_GAP_BEFORE_STALE)  {
            *p_router = router->next;
            SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, __router_src_addr_buf);
            INFO("flushing stale router - ifname: " PUB_S_SRP
                 ", router src: " PRI_SEGMENTED_IPv6_ADDR_SRP, interface->name,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, __router_src_addr_buf));
            icmp_message_free(router);
        } else {
            p_router = &(*p_router)->next;
        }
    }
}

static void
router_discovery_cancel(interface_t *interface)
{
    if (interface->router_solicit_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->router_solicit_wakeup);
    }
    if (interface->post_solicit_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->post_solicit_wakeup);
    }
#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
    if (interface->vicarious_discovery_complete != NULL) {
        ioloop_cancel_wake_event(interface->vicarious_discovery_complete);
        INFO("stopping vicarious router discovery on " PUB_S_SRP, interface->name);
    }
    interface->vicarious_router_discovery_in_progress = false;
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
}

static void
router_discovery_stop(interface_t *interface, uint64_t now)
{
    if (!interface->router_discovery_started) {
        INFO("router discovery not yet started.");
        return;
    }
    if (!interface->router_discovery_complete) {
        INFO("stopping router discovery on " PUB_S_SRP, interface->name);
    }
    router_discovery_cancel(interface);
    interface->router_discovery_complete = true;
    interface->router_discovery_in_progress = false;
    // clear out need_reconfigure_prefix when router_discovery_complete is set back to true.
    interface->need_reconfigure_prefix = false;
#ifdef FLUSH_STALE_ROUTERS
    flush_routers(interface, now);
#else
    (void)now;
#endif // FLUSH_STALE_ROUTERS

    // See if we need a new prefix on the interface.
    interface_prefix_evaluate(interface);
}

#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
static void
adjust_router_received_time(interface_t *const interface, const uint64_t now, const int64_t time_adjusted)
{
    icmp_message_t *router;

    if (interface->routers == NULL) {
        if (interface->our_prefix_advertised) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, __ipv6_prefix);
            INFO("No router information available for the interface - "
                 "ifname: " PUB_S_SRP ", prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __ipv6_prefix));
        } else {
            INFO("No router information available for the interface - "
                 "ifname: " PUB_S_SRP, interface->name);
        }

        goto exit;
    }

    for (router = interface->routers; router != NULL; router = router->next) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, __router_src_addr_buf);
        // Only adjust the received time once.
        if (router->received_time_already_adjusted) {
            INFO("received time already adjusted - remaining time: %llu, "
                  "router src: " PRI_SEGMENTED_IPv6_ADDR_SRP, (now - router->received_time) / MSEC_PER_SEC,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, __router_src_addr_buf));
            continue;
        }
        require_action_quiet(
            (time_adjusted > 0 && (UINT64_MAX - now) > (uint64_t)time_adjusted) ||
            (time_adjusted < 0 && now > ((uint64_t)-time_adjusted)), exit,
            ERROR("adjust_router_received_time: invalid adjusted values is causing overflow - "
                  "now: %" PRIu64 ", time_adjusted: %" PRId64, now, time_adjusted));
        router->received_time = now + time_adjusted;
        router->received_time_already_adjusted = true; // Only adjust the icmp message received time once.
        INFO("router received time is adjusted - router src: " PRI_SEGMENTED_IPv6_ADDR_SRP
              ", adjusted value: %" PRId64,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, __router_src_addr_buf), time_adjusted);
    }

exit:
    return;
}

static void
make_all_routers_nearly_stale(interface_t *interface, uint64_t now)
{
    // Make every router go stale in 19.999 seconds.   This means that if we don't get a response
    // to our solicit in 20 seconds, then when the timeout callback is called, there will be no
    // routers on the interface that aren't stale, which will trigger router discovery.
    adjust_router_received_time(interface, now, 19999 - 600 * MSEC_PER_SEC);
}

static void
vicarious_discovery_callback(void *context)
{
    interface_t *interface = context;
    INFO("Vicarious router discovery finished on " PUB_S_SRP ".", interface->name);
    interface->vicarious_router_discovery_in_progress = false;
    // At this point, policy evaluate will show all the routes that were present before vicarious
    // discovery as stale, so policy_evaluate will start router discovery if we didn't get any
    // RAs containing on-link prefixes.
    routing_policy_evaluate(interface, false);
}
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY

#ifndef RA_TESTER
static void
routing_policy_evaluate_all_interfaces(route_state_t *route_state, bool assume_changed)
{
    interface_t *interface;

    for (interface = route_state->interfaces; interface; interface = interface->next) {
        routing_policy_evaluate(interface, assume_changed);
    }
}
#endif

#ifdef FLUSH_STALE_ROUTERS
static void
stale_router_policy_evaluate(void *context)
{
    interface_t *interface = context;
    INFO("Evaluating stale routers on " PUB_S_SRP, interface->name);

    flush_routers(interface, ioloop_timenow());

    // See if we need a new prefix on the interface.
    interface_prefix_evaluate(interface);

    routing_policy_evaluate(interface, true);
}
#endif // FLUSH_STALE_ROUTERS

static bool
prefix_usable(interface_t *interface, route_state_t *route_state, icmp_message_t *router, prefix_information_t *prefix)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_addr_buf);
    // It needs to be on link, autoconfiguration enabled, or have the managed flag set and we are allowing DHCPv6-only
    // prefixes (not by default). And the preferred lifetime needs to be >0 (maybe should be >= 1800?)
    if (!((prefix->flags & ND_OPT_PI_FLAG_ONLINK) &&
          ((prefix->flags & ND_OPT_PI_FLAG_AUTO) ||
           (route_state->config_enable_dhcpv6_prefixes && (router->flags & ND_RA_FLAG_MANAGED))) &&
          prefix->preferred_lifetime > 300))
    {
        INFO("Router " PRI_SEGMENTED_IPv6_ADDR_SRP
             " is advertising prefix " PRI_SEGMENTED_IPv6_ADDR_SRP ": %sonlink, %sautoconf, %sdhcp, preferred = %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_addr_buf),
             bool_str(prefix->flags & ND_OPT_PI_FLAG_ONLINK),
             bool_str(prefix->flags & ND_OPT_PI_FLAG_AUTO),
             bool_str(route_state->config_enable_dhcpv6_prefixes && (router->flags & ND_RA_FLAG_MANAGED)),
             prefix->preferred_lifetime);
        return false;
    }
    int cmp = in6prefix_compare(&prefix->prefix, &route_state->xpanid_prefix, 8);
    if (!cmp)
    {
        INFO("Router " PRI_SEGMENTED_IPv6_ADDR_SRP
             " is advertising xpanid prefix " PRI_SEGMENTED_IPv6_ADDR_SRP ": not considering it usable",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_addr_buf));
        // If it's the xpanid prefix, we will also advertise the xpanid prefix
        return false;
    }

    // If this is a stub router, and we are advertising our own prefix, and the PIO it is advertising is greater than
    // the one we are advertising, then we keep advertising ours.
    if (interface->our_prefix_advertised && router->stub_router && cmp > 0) {
        INFO("Router " PRI_SEGMENTED_IPv6_ADDR_SRP
             " is a stub router advertising prefix " PRI_SEGMENTED_IPv6_ADDR_SRP ", which loses the election and is not usable",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_addr_buf));
        return false;
    }
    INFO("Router " PRI_SEGMENTED_IPv6_ADDR_SRP
         " is " PUB_S_SRP "advertising prefix " PRI_SEGMENTED_IPv6_ADDR_SRP ", which is usable",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
         router->stub_router ? "a stub router " : "",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_addr_buf));
    return true;
}

static void
routing_policy_evaluate(interface_t *interface, bool assume_changed)
{
    icmp_message_t *router;
    bool new_prefix = false;    // new prefix means that srp-mdns-proxy received a new prefix from the wire, which it
                                // did not know before.
    bool on_link_prefix_present = false;
    bool something_changed = assume_changed;
    uint64_t now = ioloop_timenow();
    bool stale_routers_exist = false;
    uint64_t stale_refresh_time = 0;
    route_state_t *route_state = interface->route_state;

    // No action on interfaces that aren't eligible for routing or that isn't currently active.
    if (interface->ineligible || interface->inactive) {
        INFO("not evaluating policy on " PUB_S_SRP " because it's " PUB_S_SRP, interface->name,
             interface->ineligible ? "ineligible" : "inactive");
        return;
    }

    // We can't tell whether any particular prefix is usable until we've gotten the xpanid.
    if (route_state->have_xpanid_prefix) {
        // Look at all the router advertisements we've seen to see if any contain a usable prefix which is not the
        // prefix we'd advertise. Routers advertising that prefix are all Thread BRs, and it's fine for more than
        // one router to advertise a prefix, so we will also advertise it for redundancy.
        for (router = interface->routers; router; router = router->next) {
            icmp_option_t *option = router->options;
            int i;
            bool usable = false;
            for (i = 0; i < router->num_options; i++, option++) {
                if (option->type == icmp_option_prefix_information) {
                    prefix_information_t *prefix = &option->option.prefix_information;
#ifndef RA_TESTER
                    omr_publisher_check_prefix(route_state->omr_publisher, &prefix->prefix, prefix->length);
#endif
                    if (prefix_usable(interface, route_state, router, prefix)) {
                        // We don't consider the prefix we would advertise to be infrastructure-provided if we see it
                        // advertised by another router, because that router is also a Thread BR, and we don't want
                        // to get into dueling prefixes with it.
                        if (in6prefix_compare(&option->option.prefix_information.prefix, &route_state->xpanid_prefix, 8))
                        {
                            uint32_t preferred_lifetime_offset = MAX_ROUTER_RECEIVED_TIME_GAP_BEFORE_STALE / MSEC_PER_SEC;
                            uint32_t preferred_lifetime = prefix->preferred_lifetime;

                            // Infinite preferred lifetime. Bogus.
                            if (preferred_lifetime == UINT32_MAX) {
                                preferred_lifetime = 60 * 60;   // One hour
                            }

                            // If the remaining time on this prefix is less than the stale time gap, use an offset that's the
                            // valid lifetime minus sixty seconds so that we have time if the prefix expires.
                            if (preferred_lifetime < preferred_lifetime_offset + 60) {
                                // If the preferred lifetime is less than a minute, we're not going to count this as a valid
                                // on-link prefix.
                                if (preferred_lifetime < 60) {
                                    SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
                                    SEGMENTED_IPv6_ADDR_GEN_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf);
                                    INFO("router " PRI_SEGMENTED_IPv6_ADDR_SRP " advertising " PRI_SEGMENTED_IPv6_ADDR_SRP
                                         " has a preferred lifetime of %d, which is not enough to count as usable.",
                                         SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
                                         SEGMENTED_IPv6_ADDR_PARAM_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf),
                                         preferred_lifetime);
                                    continue;
                                }
                                preferred_lifetime_offset = preferred_lifetime - 60;
                            }

                            // Lifetimes are in seconds, but henceforth we will compare with clock times, which are in ms.
                            preferred_lifetime_offset *= MSEC_PER_SEC;

                            // If the prefix' preferred lifetime plus the time received is in the past, the prefix doesn't
                            // count as an on-link prefix that's present.
                            if (router->received_time + preferred_lifetime_offset < now) {
                                SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
                                SEGMENTED_IPv6_ADDR_GEN_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf);
                                INFO("router " PRI_SEGMENTED_IPv6_ADDR_SRP " advertising " PRI_SEGMENTED_IPv6_ADDR_SRP
                                     " was received %d seconds ago with a preferred lifetime of %d.",
                                     SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
                                     SEGMENTED_IPv6_ADDR_PARAM_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf),
                                     (int)((now - router->received_time) / 1000), preferred_lifetime);

                                continue;
                            }

                            // This prefix is in principle usable. It may not actually be usable if it is stale, but we mark it usable so it
                            // will continue to be probed.
                            usable = true;

                            // router->reachable will be true immediately after receiving a router advertisement until we do a
                            // probe and don't get a response. It will become true again if, during a later probe, we get a
                            // response.
                            if (!router->reachable) {
                                SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
                                SEGMENTED_IPv6_ADDR_GEN_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf);
                                INFO("router %p " PRI_SEGMENTED_IPv6_ADDR_SRP " advertising %d %p " PRI_SEGMENTED_IPv6_ADDR_SRP
                                     " was last known to be reachable %d seconds ago.",
                                     router,
                                     SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
                                     i, option,
                                     SEGMENTED_IPv6_ADDR_PARAM_SRP(option->option.prefix_information.prefix.s6_addr, pref_buf),
                                     (int)((now - router->latest_na) / 1000));
                                continue;
                            }

                            // Otherwise, if this router's on-link prefix will expire later than any other we've seen
                            if (stale_refresh_time < router->received_time + preferred_lifetime_offset) {
                                stale_refresh_time = router->received_time + preferred_lifetime_offset;
                            }

                            // If this is a new icmp_message received now and contains PIO.
                            if (router->new_router) {
                                new_prefix = true;
                                router->new_router = false; // clear the bit since srp-mdns-proxy already processed it.
                            }

                            // This router has a usable prefix.
                            usable = true;

                            // Right now all we need is to see if there is an on-link prefix.
                            on_link_prefix_present = true;
                            SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, __router_src_add_buf);
                            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, __pio_prefix_buf);
                            INFO("router has usable PIO - ifname: " PUB_S_SRP ", router src: " PRI_SEGMENTED_IPv6_ADDR_SRP
                                 ", prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                                 interface->name,
                                 SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, __router_src_add_buf),
                                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, __pio_prefix_buf));
                        } else {
                            SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
                            INFO("Router " PRI_SEGMENTED_IPv6_ADDR_SRP
                                 " is advertising the xpanid prefix: not counting as usable ",
                                 SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf));
                        }
                    } else {
                        SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, __router_src_add_buf);
                        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, __pio_prefix_buf);
                        INFO("router has unusable PIO - ifname: " PUB_S_SRP ", router src: " PRI_SEGMENTED_IPv6_ADDR_SRP
                             ", prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                             interface->name,
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, __router_src_add_buf),
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, __pio_prefix_buf));
                    }
                }
            }
            // Remember whether or not this router has a usable prefix.
            router->usable = usable;
        }
    }

    INFO("policy on " PUB_S_SRP ": " PUB_S_SRP "stale " /* stale_routers_exist ? */
         PUB_S_SRP "started " /* interface->router_discovery_started ? */
         PUB_S_SRP "disco " /* interface->router_discovery_complete ? */
         PUB_S_SRP "present " /* on_link_prefix_present ? */
         PUB_S_SRP "advert " /* interface->our_prefix_advertised ? */
         PUB_S_SRP "conf " /* interface->on_link_prefix_configured ? */
         PUB_S_SRP "new_prefix " /* new_prefix ? */
         "preferred = %" PRIu32 " valid = %" PRIu32 " deadline = %" PRIu64,
         interface->name, stale_routers_exist ? "" : "!", interface->router_discovery_started ? "" : "!",
         interface->router_discovery_complete ? "" : "!",
         on_link_prefix_present ? "" : "!", interface->our_prefix_advertised ? "" : "!",
         interface->on_link_prefix_configured ? "" : "!", new_prefix ? "" : "!",
         interface->preferred_lifetime, interface->valid_lifetime, interface->deprecate_deadline);

    // If there are stale routers, start doing router discovery again to see if we can get them to respond.
    // Note that doing router discover just because we haven't seen an RA is actually not allowed in RFC 4861,
    // so this shouldn't be enabled.
    // Also, if we have not yet done router discovery, do it now.
    if ((!interface->router_discovery_started || !interface->router_discovery_complete
#if SRP_FEATURE_STALE_ROUTER_DISCOVERY
         || stale_routers_exist
#endif //SRP_FEATURE_STALE_ROUTER_DISCOVERY
            ) && !on_link_prefix_present) {
        if (!interface->router_discovery_in_progress) {
            // Start router discovery.
            INFO("starting router discovery");
            router_discovery_start(interface);
        } else {
            INFO("router discovery in progress");
        }
    }
    // If we are advertising a prefix and there's another on-link prefix, deprecate the one we are
    // advertising.
    else if (interface->our_prefix_advertised && on_link_prefix_present) {
        // If we have been advertising a preferred prefix, deprecate it.
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf);
        if (interface->preferred_lifetime == BR_PREFIX_LIFETIME) {
            INFO("routing_policy_evaluate: deprecating interface prefix in 30 minutes - prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf));
            interface->deprecate_deadline = now + BR_PREFIX_LIFETIME * 1000;
            something_changed = true;
            interface->preferred_lifetime = 0;
        } else {
            INFO("prefix deprecating in progress - prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf));
        }
    }
    // If there is no on-link prefix and we aren't advertising, or have deprecated, start advertising
    // again (or for the first time).
    else if (!on_link_prefix_present && interface->router_discovery_complete && route_state->have_xpanid_prefix &&
             (!interface->our_prefix_advertised || interface->deprecate_deadline != 0 ||
              interface->preferred_lifetime == 0)) {

        SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf);
        INFO("advertising prefix again - ifname: " PUB_S_SRP
             ", prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP, interface->name,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf));

        // If we were deprecating, stop.
        ioloop_cancel_wake_event(interface->deconfigure_wakeup);
        interface->deprecate_deadline = 0;

        // Start advertising immediately, 30 minutes.
        interface->preferred_lifetime = interface->valid_lifetime = BR_PREFIX_LIFETIME;

        // If the on-link prefix isn't configured on the interface, do that.
        if (!interface->on_link_prefix_configured) {
#ifndef RA_TESTER
            if (!interface->is_thread) {
#endif
                interface_prefix_configure(interface->ipv6_prefix, interface);
#ifndef RA_TESTER
            } else {
                INFO("Not setting up " PUB_S_SRP " because it is the thread interface", interface->name);
            }
#endif
        } else {
            // Configuring the on-link prefix takes a while, so we want to re-evaluate after it's finished.
            interface->our_prefix_advertised = true;
            something_changed = true;
        }
    }
    // If there is no on-link prefix present, and srp-mdns-proxy itself is advertising the prefix, and it has configured
    // an on-link prefix, and the interface is not thread interface, and it just got an interface address removal event,
    // it is possible that the IPv6 routing has been flushed due to loss of address in configd, so here we explicitly
    // reconfigure the IPv6 prefix and the routing.
    else if (interface->need_reconfigure_prefix && !on_link_prefix_present && interface->our_prefix_advertised &&
             interface->on_link_prefix_configured && !interface->is_thread) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf);
        INFO("reconfigure ipv6 prefix due to possible network changes -"
             " prefix: " PRI_SEGMENTED_IPv6_ADDR_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, __prefix_buf));
        interface_prefix_configure(interface->ipv6_prefix, interface);
        interface->need_reconfigure_prefix = false;
    }

    // If the on-link prefix goes away, stop suppressing the one we've been advertising (if it's still valid).
    if (!on_link_prefix_present && interface->suppress_ipv6_prefix) {
        INFO("un-suppressing ipv6 prefix.");
        interface->suppress_ipv6_prefix = false;
    }

    // If we've been looking to see if there's an on-link prefix, and we got one from the new router advertisement,
    // stop looking for new one.
    if (new_prefix) {
        router_discovery_stop(interface, now);
    }

    // If anything changed, do an immediate beacon; otherwise wait until the next one.
    // Also when something changed, set the number of transmissions back to zero so that
    // we send a few initial beacons quickly for reliability.
    if (something_changed) {
        INFO("change on " PUB_S_SRP ": " PUB_S_SRP "started " PUB_S_SRP "disco " PUB_S_SRP "present " PUB_S_SRP "advert " PUB_S_SRP
             "conf preferred = %" PRIu32 " valid = %" PRIu32 " deadline = %" PRIu64,
             interface->name, interface->router_discovery_started ? "" : "!",
             interface->router_discovery_complete ? "" : "!", on_link_prefix_present ? "" : "!",
             interface->our_prefix_advertised ? "" : "!", interface->on_link_prefix_configured ? "" : "!",
             interface->preferred_lifetime,
             interface->valid_lifetime, interface->deprecate_deadline);
        interface->num_beacons_sent = 0;
        interface_beacon_schedule(interface, 0);
    }

    // It's possible for us to start configuring the interface because there's no on-link prefix, and then see
    // an advertisement for an on-link prefix before interface configuration completes. When this happens, we
    // need to delete the address we just configured, because we're not going to be advertising it. We always
    // get a policy re-evaluation event when interface configuration completes, so this will happen immediately.
    // At this point we have not yet sent a router advertisement with the prefix, so even though it has a preferred
    // lifetime of about 1800 seconds here, we can safely set it to zero without leaving stale information
    // in any host's routing table.
    if (!interface->our_prefix_advertised && interface->on_link_prefix_configured) {
        INFO("on-link prefix appeared during interface configuration. removing");
        interface->preferred_lifetime = 0;
        interface_prefix_deconfigure(interface);
    }

#ifdef FLUSH_STALE_ROUTERS
    // If we have an on-link prefix, schedule a policy re-evaluation at the stale router interval.
    if (on_link_prefix_present) {
        if (stale_refresh_time < now) {
            ERROR("Stale refresh time is in the past: %" PRIu64 "!", stale_refresh_time);
        } else {
            // The math used to compute refresh timeout guarantees that refresh_timeout will be <10 minutes.
            int refresh_timeout = (int)(stale_refresh_time - now);

            if (interface->stale_evaluation_wakeup == NULL) {
                interface->stale_evaluation_wakeup = ioloop_wakeup_create();
                if (interface->stale_evaluation_wakeup == NULL) {
                    ERROR("No memory for stale router evaluation wakeup on " PUB_S_SRP ".", interface->name);
                }
            } else {
                ioloop_cancel_wake_event(interface->stale_evaluation_wakeup);
            }
            ioloop_add_wake_event(interface->stale_evaluation_wakeup,
                                  interface, stale_router_policy_evaluate, NULL, refresh_timeout);
        }
    }
#endif // FLUSH_STALE_ROUTERS

    // Once router discovery is complete, start doing aliveness checks on whatever we discovered (if anything).
    if (interface->last_router_probe == 0 && interface->router_discovery_started && interface->router_discovery_complete) {
        schedule_next_router_probe(interface);
    }

#ifndef RA_TESTER
    if (route_state->route_tracker != NULL) {
        route_tracker_route_state_changed(route_state->route_tracker, interface);
    }
#endif
}

#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
static void
start_vicarious_router_discovery_if_appropriate(interface_t *const interface)
{
    if (!interface->our_prefix_advertised &&
        !interface->vicarious_router_discovery_in_progress && !interface->router_discovery_in_progress)
    {
        if (interface->vicarious_discovery_complete == NULL) {
            interface->vicarious_discovery_complete = ioloop_wakeup_create();
        } else {
            ioloop_cancel_wake_event(interface->vicarious_discovery_complete);
        }
        if (interface->vicarious_discovery_complete != NULL) {
            ioloop_add_wake_event(interface->vicarious_discovery_complete,
                                  interface, vicarious_discovery_callback, NULL, 20 * 1000);
            interface->vicarious_router_discovery_in_progress = true;
        }
        // In order for vicarious router discovery to be useful, we need all of the routers
        // that were present when the first solicit was received to be stale when we give up
        // on vicarious discovery.  If we got any router advertisements, these will not be
        // stale, and that means vicarious discovery succeeded.
        make_all_routers_nearly_stale(interface, ioloop_timenow());
        INFO("Starting vicarious router discovery on " PUB_S_SRP,
             interface->name);
    }
}
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY

static void
retransmit_unicast_beacon(void *context)
{
    icmp_message_t *message = context;

    // Schedule retranmsission
    interface_beacon_send(message->interface, &message->source);
    ioloop_add_wake_event(message->wakeup, message, retransmit_unicast_beacon, NULL,
                          MIN_DELAY_BETWEEN_RAS + srp_random16() % RA_FUZZ_TIME);

    // Discontinue retransmission after the third we've sent.
    if (message->messages_sent++ > 2) {
        icmp_message_t **sp = &message->interface->solicits;
        while (*sp != NULL) {
            if (*sp == message) {
                *sp = message->next;
                icmp_message_free(message);
                break;
            } else {
                sp = &(*sp)->next;
            }
        }
    }
}

// This gets called to check to see if any of the usable routers are still responding. It gets called whenever
// we get a router solicit, to ensure that the solicit gets a quick response, and also gets called once every
// minute so that we quickly notice when a router becomes unreachable.

static void
send_router_probes(void *context)
{
    interface_t *interface = context;

    // After sending three probes, do a policy evaluation.
    if (interface->num_solicits_sent++ > MAX_NS_RETRANSMISSIONS - 1) {
        // Mark routers from which we received neighbor advertises during the probe as reachable. Routers
        // that did not respond are no longer reachable.
        for (icmp_message_t *router = interface->routers; router != NULL; router = router->next) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(router->source.s6_addr, router_src_addr_buf);
            INFO("router (%p) " PRI_SEGMENTED_IPv6_ADDR_SRP " was " PUB_S_SRP "reached during probing.", router,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(router->source.s6_addr, router_src_addr_buf),
                 router->reached ? "" : "not ");
            router->reachable = router->reached;
        }
        routing_policy_evaluate(interface, false);
        schedule_next_router_probe(interface);
        return;
    }

    // Send Neighbor Solicits to any usable routers that haven't responded yet and schedule the next call to
    // send_router_probes...
    for (icmp_message_t *router = interface->routers; router != NULL; router = router->next) {
        // Don't probe routers that aren't usable, and don't re-probe a router that's already responded in this probe cycle.
        if (!router->usable || router->reached) {
            continue;
        }
        neighbor_solicit_send(router->interface, &router->source);
    }
    ioloop_add_wake_event(interface->neighbor_solicit_wakeup, interface, send_router_probes, NULL,
                          MIN_DELAY_BETWEEN_RAS + srp_random16() % RA_FUZZ_TIME);
}

static void
check_router_aliveness(void *context)
{
    interface_t *interface = context;

    if (!interface->probing) {
        interface->probing = true;
        if (interface->neighbor_solicit_wakeup == NULL) {
            interface->neighbor_solicit_wakeup = ioloop_wakeup_create();
        }
        if (interface->neighbor_solicit_wakeup != NULL) {
            interface->num_solicits_sent = 0;
            // Clear the reached flag on all routers
            for (icmp_message_t *router = interface->routers; router != NULL; router = router->next) {
                router->reached = false;
            }
            send_router_probes(interface);
        }
    }
}

static void
schedule_next_router_probe(interface_t *interface)
{
    if (interface->router_probe_wakeup == NULL) {
        interface->router_probe_wakeup = ioloop_wakeup_create();
    }
    if (interface->router_probe_wakeup != NULL) {
        INFO("scheduling router probe in 60 seconds.");
        ioloop_add_wake_event(interface->router_probe_wakeup, interface, check_router_aliveness, NULL, 60 * 1000);
        interface->probing = false;
        interface->last_router_probe = ioloop_timenow();
    }
}

void
router_solicit(icmp_message_t *message)
{
    interface_t *iface, *interface;
    bool is_retransmission = false;


    // Further validate the message
    if (message->hop_limit != 255 || message->code != 0) {
        ERROR("Invalid router solicitation, hop limit = %d, code = %d", message->hop_limit, message->code);
        goto out;
    }
    if (IN6_IS_ADDR_UNSPECIFIED(&message->source)) {
        icmp_option_t *option = message->options;
        int i;
        for (i = 0; i < message->num_options; i++) {
            if (option->type == icmp_option_source_link_layer_address) {
                ERROR("source link layer address in router solicitation from unspecified IP address");
                goto out;
            }
            option++;
        }
    } else {
        // Make sure it's not from this host
        for (iface = message->route_state->interfaces; iface; iface = iface->next) {
            if (iface->have_link_layer_address && !in6addr_compare(&message->source, &iface->link_local)) {
                INFO("dropping router solicitation sent from this host.");
                goto out;
            }
        }
    }
    interface = message->interface;

    SEGMENTED_IPv6_ADDR_GEN_SRP(message->source.s6_addr, source_buf);
    INFO(PUB_S_SRP " solicit on " PUB_S_SRP ": source address is " PRI_SEGMENTED_IPv6_ADDR_SRP,
         is_retransmission ? "retransmitted" : "initial",
         message->interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(message->source.s6_addr, source_buf));

    // See if this is a retransmission...
    icmp_message_t **sp;
    sp = &interface->solicits;
    while (*sp != NULL) {
        icmp_message_t *solicit = *sp;
        // Same source? Not already found?
        if (!is_retransmission && !in6addr_compare(&message->source, &solicit->source)) {
            uint64_t now = ioloop_timenow();
            // RFC 4861 limits RS transmissions to 3, separated by four seconds. Allowing for a bit of slop,
            // if it was received in the past 15 seconds, this is a retransmission.
            if (now - solicit->received_time > 15 * 1000) {
                *sp = solicit->next;
                icmp_message_free(solicit);
            } else {
                solicit->retransmissions_received++;
                is_retransmission = true;

                // Since this is a retransmission, that hints that there might not be any live routers
                // on this link, so check to see if the routers we are aware of are alive.
                check_router_aliveness(interface);

                sp = &(*sp)->next;
            }
        } else {
            sp = &(*sp)->next;
        }
    }

    // Schedule an immediate send. If this is a retransmission, just let our retransmission schedule
    // dictate when to send the next one.
    if (!is_retransmission && !interface->ineligible && !interface->inactive) {
        message->wakeup = ioloop_wakeup_create();
        if (message->wakeup == NULL) {
            ERROR("no memory for solicit wakeup.");
        } else {
            // Save the message for later
            *sp = message;
            // Start the unicast RA transmission train for this RS.
            retransmit_unicast_beacon(message);
            message = NULL;
        }
    } else {
        INFO("not sending a router advertisement.");
    }

#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
    // When we receive a router solicit, it means that a host is looking for a router.   We should
    // expect to hear replies if they are multicast.   If we hear no replies, it could mean there is
    // no on-link prefix.   In this case, we restart our own router discovery process.  There is no
    // need to do this if we are the one advertising a prefix.
    start_vicarious_router_discovery_if_appropriate(interface);
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
out:
    if (message != NULL) {
        icmp_message_free(message);
    }
}

void
router_advertisement(icmp_message_t *message)
{
    interface_t *iface;
    icmp_message_t *router, **rp;
    if (message->hop_limit != 255 || message->code != 0 || !IN6_IS_ADDR_LINKLOCAL(&message->source)) {
        ERROR("Invalid router advertisement, hop limit = %d, code = %d", message->hop_limit, message->code);
        icmp_message_free(message);
        return;
    }
    for (iface = message->route_state->interfaces; iface != NULL; iface = iface->next) {
        if (iface->have_link_layer_address && !in6addr_compare(&message->source, &iface->link_local)) {
            INFO("dropping router advertisement sent from this host.");
            icmp_message_free(message);
            return;
        }
    }

    // See if we've had a previous advertisement from this router. Note that routers can send more than one
    // RA to advertise more data than will fit in one RA, but in practice routers tend not to do this, and how
    // this is supposed to work is not clearly specified. From RFC4861:
    //
    //    If including all options causes the size of an advertisement to
    //    exceed the link MTU, multiple advertisements can be sent, each
    //    containing a subset of the options.
    //
    // If this happens, we're going to wind up using the last RA in the sequence. Ideally we'd do some work to marshal
    // RA trains. This is too much work to do in a current milestone. The issue is tracked in rdar://105200987
    // (Restructure handling of incoming router advertisements so as to marshal the data in case we get more than one RA
    // from the same router with different data.)
    for (rp = &message->interface->routers; *rp != NULL; rp = &(*rp)->next) {
        router = *rp;
        // The new RA is from the same router as this previous RA.
        if (!in6addr_compare(&router->source, &message->source)) {
            message->next = router->next;
            *rp = message;
            icmp_message_free(router);
            break;
        }
    }
    // If we got rid of the old RA, *rp will be non-NULL. If we didn't find a match for the old RA, or if we
    // need to keep the old RA, then *rp will be NULL, meaning that we should keep the new RA.
    if (*rp == NULL) {
        *rp = message;
    }

    // When we receive an RA, we can assume that the router is reachable, and skip immediately probing with a
    // neighbor solicit.
    message->latest_na = message->received_time;
    message->reachable = true;
    message->reached = true;

    // Check for the stub router flag here so that we have it when scanning PIOs for usability.
    for (int i = 0; i < message->num_options; i++) {
        icmp_option_t *option = &message->options[i];
        if (option->type == icmp_option_ra_flags_extension) {
            if (option->option.ra_flags_extension[0] & RA_FLAGS1_STUB_ROUTER) {
                message->stub_router = true;
            }
        }
    }
    // Something may have changed, so do a policy recalculation for this interface
    routing_policy_evaluate(message->interface, false);
}

void
neighbor_advertisement(icmp_message_t *message)
{
    if (message->hop_limit != 255 || message->code != 0) {
        ERROR("Invalid neighbor advertisement, hop limit = %d, code = %d", message->hop_limit, message->code);
        return;
    }

    // If this NA matches a router that has advertised a usable prefix, mark the router as alive by setting the
    // "latest_na" value to the current time. We don't care about NAs for routers that are not advertising a usable
    // prefix.
    for (icmp_message_t *router = message->interface->routers; router != NULL; router = router->next) {
        if (!in6addr_compare(&message->source, &router->source)) {
            // Only log for usable routers, to avoid a lot of extra noise. However, we don't actually probe routers that
            // aren't usable, so generally speaking this test will always be true.
            if (router->usable) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(message->source.s6_addr, source_buf);
                INFO("usable neighbor advertisement recieved on " PUB_S_SRP " from " PRI_SEGMENTED_IPv6_ADDR_SRP,
                     message->interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(message->source.s6_addr, source_buf));
            }
            router->latest_na = ioloop_timenow();
            router->reached = true;
            router->reachable = true;
        }
    }
    return;
}

#if   defined(CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IPCONFIG) || \
      defined(CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IFCONFIG)
static void
link_route_done(void *context, int status, const char *error)
{
    interface_t *interface = context;

    if (error != NULL) {
        ERROR("link_route_done on " PUB_S_SRP ": " PUB_S_SRP, interface->name, error);
    } else {
        INFO("link_route_done on " PUB_S_SRP ": %d.", interface->name, status);
    }
    ioloop_subproc_release(interface->link_route_adder_process);
    interface->link_route_adder_process = NULL;
    // Now that the on-link prefix is configured, time for a policy re-evaluation.
    interface->on_link_prefix_configured = true;
    routing_policy_evaluate(interface, true);
}
#endif

static void
interface_prefix_configure(struct in6_addr prefix, interface_t *interface)
{
    int sock;
    route_state_t *route_state = interface->route_state;

    sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        ERROR("interface_prefix_configure: socket(PF_INET6, SOCK_DGRAM, 0) failed " PUB_S_SRP ": " PUB_S_SRP,
              interface->name, strerror(errno));
        return;
    }
#ifdef CONFIGURE_STATIC_INTERFACE_ADDRESSES
    struct in6_addr interface_address = prefix;
    char addrbuf[INET6_ADDRSTRLEN + 4];
    // Use our ULA prefix as the host identifier.
    memcpy(&interface_address.s6_addr[10], &route_state->srp_server->ula_prefix.s6_addr[0], 6);
    interface_address.s6_addr[8] = (interface->index >> 8) & 255;
    interface_address.s6_addr[9] = interface->index & 255;
    inet_ntop(AF_INET6, &interface_address, addrbuf, INET6_ADDRSTRLEN);
#if   defined(CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IPCONFIG)
    char *args[] = { "set", interface->name, "MANUAL-V6", addrbuf, "64" };

    if (interface->link_route_adder_process != NULL) {
        ERROR("interface_prefix_configure: " PUB_S_SRP " already configuring the route.", interface->name);
        return;
    }
    INFO("/sbin/ipconfig " PUB_S_SRP " " PUB_S_SRP " " PUB_S_SRP " " PUB_S_SRP " "
         PUB_S_SRP, args[0], args[1], args[2], args[3], args[4]);
    interface->link_route_adder_process = ioloop_subproc("/usr/sbin/ipconfig", args, 5, link_route_done, interface, NULL);
    if (interface->link_route_adder_process == NULL) {
        ERROR("interface_prefix_configure: unable to set interface address for %s to %s.", interface->name, addrbuf);
    }
#elif defined(CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IFCONFIG)
    char *eos = addrbuf + strlen(addrbuf);
    if (sizeof(addrbuf) - (eos - addrbuf) < 4) {
        ERROR("interface_prefix_configure: this shouldn't happen: no space in addrbuf");
        return;
    }
    strcpy(eos, "/64");
    char *args[] = { interface->name, "add", addrbuf };

    if (interface->link_route_adder_process != NULL) {
        ERROR("interface_prefix_configure: " PUB_S_SRP " already configuring the route.", interface->name);
        return;
    }
    INFO("/sbin/ifconfig %s %s %s", args[0], args[1], args[2]);
    interface->link_route_adder_process = ioloop_subproc("/sbin/ifconfig", args, 3, link_route_done, NULL, interface);
    if (interface->link_route_adder_process == NULL) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface_address.s6_addr, if_addr_buf);
        ERROR("interface_prefix_configure: unable to set interface address for " PUB_S_SRP " to "
              PRI_SEGMENTED_IPv6_ADDR_SRP ".", interface->name,
              SEGMENTED_IPv6_ADDR_PARAM_SRP(interface_address.s6_addr, if_addr_buf));
    }
#else
    struct in6_aliasreq alias_request;
    int ret;

    memset(&alias_request, 0, sizeof(alias_request));
    strlcpy(alias_request.ifra_name, interface->name, IFNAMSIZ);
    alias_request.ifra_addr.sin6_family = AF_INET6;
    alias_request.ifra_addr.sin6_len = sizeof(alias_request.ifra_addr);
    memcpy(&alias_request.ifra_addr.sin6_addr, &interface_address, sizeof(alias_request.ifra_addr.sin6_addr));
    alias_request.ifra_prefixmask.sin6_len = sizeof(alias_request.ifra_addr);
    alias_request.ifra_prefixmask.sin6_family = AF_INET6;
    memset(&alias_request.ifra_prefixmask.sin6_addr, 0xff, 8); // /64.
    alias_request.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME; // seconds, I hope?
    alias_request.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME; // seconds, I hope?

    ret = ioctl(sock, SIOCAIFADDR_IN6, &alias_request);
    if (ret < 0) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface_address.s6_addr, if_addr_buf);
        ERROR("interface_prefix_configure: can't configure static address " PRI_SEGMENTED_IPv6_ADDR_SRP " on " PUB_S_SRP
              ": " PUB_S_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(interface_address.s6_addr, if_addr_buf), interface->name,
              strerror(errno));
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface_address.s6_addr, if_addr_buf);
        INFO("added address " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(interface_address.s6_addr, if_addr_buf), interface->name);
    }
#endif // CONFIGURE_STATIC_INTERFACE_ADDRESSES_WITH_IPCONFIG
#else
    (void)prefix;
#endif // CONFIGURE_STATIC_INTERFACE_ADDRESSES
    close(sock);
}

#ifndef RA_TESTER
static void
set_thread_forwarding(void)
{
#ifdef LINUX
    const char *procfile = "/proc/sys/net/ipv6/conf/all/forwarding";
    int fd = open(procfile, O_WRONLY);
    if (fd < 0) {
        ERROR("%s: %s", procfile, strerror(errno));
    } else {
        ssize_t ret = write(fd, "1", 1);
        if (ret < 0) {
            ERROR("write: %s", strerror(errno));
        } else if (ret != 1) {
            ERROR("invalid write: %zd", ret);
        }
        close(fd);
    }
#else
    int wun = 1;
    int ret = sysctlbyname("net.inet6.ip6.forwarding", NULL, 0, &wun, sizeof(wun));
    if (ret < 0) {
        ERROR(PUB_S_SRP, strerror(errno));
    } else {
        INFO("Enabled IPv6 forwarding.");
    }
#endif
}
#endif // RA_TESTER

#ifdef NEED_THREAD_RTI_SETTER
static void
thread_rti_done(void *UNUSED context, int status, const char *error)
{
    route_state_t *route_state = context;

    if (error != NULL) {
        ERROR("thread_rti_done: " PUB_S_SRP, error);
    } else {
        INFO("%d.", status);
    }
    ioloop_subproc_release(route_state->thread_rti_setter_process);
    route_state->thread_rti_setter_process = NULL;
}

static void
set_thread_rti(route_state_t *route_state)
{
    char *args[] = { "-w", "net.inet6.icmp6.nd6_process_rti=1" };
    route_state->thread_rti_setter_process = ioloop_subproc("/usr/sbin/sysctl", args, 2, thread_rti_done,
                                               NULL, route_state);
    if (route_state->thread_rti_setter_process == NULL) {
        ERROR("Unable to set thread rti enabled.");
    }
}
#endif

#if defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)
#ifdef ADD_PREFIX_WITH_WPANCTL
static void
thread_prefix_done(void *context, int status, const char *error)
{
    route_state_t *route_state = context;

    if (error != NULL) {
        ERROR("thread_prefix_done: " PUB_S_SRP, error);
    } else {
        interface_t *interface;

        INFO("%d.", status);
        for (interface = route_state->interfaces; interface; interface = interface->next) {
            if (!interface->inactive) {
                interface_beacon_schedule(interface, 0);
            }
        }
    }
    ioloop_subproc_release(route_state->thread_prefix_adder_process);
    route_state->thread_prefix_adder_process = NULL;
}
#endif
#endif // THREAD_BORDER_ROUTRER && !RA_TESTER

static void
post_solicit_policy_evaluate(void *context)
{
    interface_t *interface = context;
    INFO("Done waiting for router discovery to finish on " PUB_S_SRP, interface->name);
    interface->router_discovery_complete = true;
    interface->router_discovery_in_progress = false;
#ifdef FLUSH_STALE_ROUTERS
    flush_routers(interface, ioloop_timenow());
#endif // FLUSH_STALE_ROUTERS

    // See if we need a new prefix on the interface.
    interface_prefix_evaluate(interface);

    routing_policy_evaluate(interface, true);
    // Always clear out need_reconfigure_prefix when router_discovery_complete is set to true.
    interface->need_reconfigure_prefix = false;
}

static void
ula_record(const char *ula_printable)
{
    size_t len = strlen(ula_printable);
    if (access(THREAD_DATA_DIR, F_OK) < 0) {
        if (mkdir(THREAD_DATA_DIR, 0700) < 0) {
            ERROR("ula_record: " THREAD_DATA_DIR " not present and can't be created: %s", strerror(errno));
            return;
        }
    }
    srp_store_file_data(NULL, THREAD_ULA_FILE, (uint8_t *)ula_printable, len);
}

void
route_ula_generate(route_state_t *route_state)
{
    char ula_prefix_buffer[INET6_ADDRSTRLEN];
    struct in6_addr ula_prefix, old_ula_prefix;
    bool prefix_changed;

    // Already have a prefix?
    if (route_state->srp_server->ula_prefix.s6_addr[0] == 0xfd) {
        old_ula_prefix = route_state->srp_server->ula_prefix;
        prefix_changed = true;
    } else {
        prefix_changed = false;
    }

    in6addr_zero(&ula_prefix);
    srp_randombytes(&ula_prefix.s6_addr[1], 5);
    ula_prefix.s6_addr[0] = 0xfd;

    inet_ntop(AF_INET6, &ula_prefix, ula_prefix_buffer, sizeof ula_prefix_buffer);

    ula_record(ula_prefix_buffer);
    if (prefix_changed) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(old_ula_prefix.s6_addr, old_prefix_buf);
        SEGMENTED_IPv6_ADDR_GEN_SRP(ula_prefix.s6_addr, new_prefix_buf);
        INFO("ula-generate: prefix changed from " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(old_ula_prefix.s6_addr, old_prefix_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(ula_prefix.s6_addr, new_prefix_buf));
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(ula_prefix.s6_addr, new_prefix_buf);
        INFO("ula-generate: generated ULA prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(ula_prefix.s6_addr, new_prefix_buf));
    }

    // Set up the thread prefix.
    route_state->my_thread_ula_prefix = ula_prefix;
    route_state->srp_server->ula_prefix = ula_prefix;
    route_state->have_thread_prefix = true;
#if SRP_FEATURE_NAT64
    if (route_state->srp_server->srp_nat64_enabled) {
        nat64_set_ula_prefix(&ula_prefix);
    }
#endif
}

void
route_ula_setup(route_state_t *route_state)
{
    bool have_stored_ula_prefix = false;

    char ula_buf[INET6_ADDRSTRLEN];
    uint16_t length;
    if (srp_load_file_data(NULL, THREAD_ULA_FILE, (uint8_t *)ula_buf, &length, sizeof(ula_buf) - 1)) {
        ula_buf[length] = 0;
        if (inet_pton(AF_INET6, ula_buf, &route_state->srp_server->ula_prefix)) {
            have_stored_ula_prefix = true;
        } else {
            INFO("ula prefix %.*s is not valid", length, ula_buf);
        }
    } else {
        INFO("Couldn't open ULA file " THREAD_ULA_FILE ".");
    }

    // If we didn't already successfully fetch a stored prefix, try to store one.
    if (!have_stored_ula_prefix) {
        route_ula_generate(route_state);
    } else {
        // Set up the thread prefix.
        route_state->my_thread_ula_prefix = route_state->srp_server->ula_prefix;
        route_state->have_thread_prefix = true;
#if SRP_FEATURE_NAT64
        if (route_state->srp_server->srp_nat64_enabled) {
            nat64_set_ula_prefix(&route_state->srp_server->ula_prefix);
        }
#endif
    }
}

static void
router_solicit_callback(void *context)
{
    interface_t *interface = context;
    if (interface->is_thread) {
        INFO("discontinuing router solicitations on thread interface " PUB_S_SRP, interface->name);
        return;
    }
    if (interface->num_solicits_sent >= 3) {
        INFO("Done sending router solicitations on " PUB_S_SRP ".", interface->name);
        return;
    }
    INFO("sending router solicitation on " PUB_S_SRP , interface->name);
    router_solicit_send(interface);

    interface->num_solicits_sent++;
    ioloop_add_wake_event(interface->router_solicit_wakeup,
                          interface, router_solicit_callback, NULL,
                          RTR_SOLICITATION_INTERVAL * 1000 + srp_random16() % 1024);
}

static void
start_router_solicit(interface_t *interface)
{
    if (interface->router_solicit_wakeup == NULL) {
        interface->router_solicit_wakeup = ioloop_wakeup_create();
        if (interface->router_solicit_wakeup == 0) {
            ERROR("No memory for router solicit wakeup on " PUB_S_SRP ".", interface->name);
            return;
        }
    } else {
        ioloop_cancel_wake_event(interface->router_solicit_wakeup);
    }
    interface->num_solicits_sent = 0;
    ioloop_add_wake_event(interface->router_solicit_wakeup, interface, router_solicit_callback,
                          NULL, 128 + srp_random16() % 896);
}

static interface_t *
find_interface(route_state_t *route_state, const char *name, int ifindex)
{
    interface_t **p_interface, *interface = NULL;

    for (p_interface = &route_state->interfaces; *p_interface; p_interface = &(*p_interface)->next) {
        interface = *p_interface;
        if (!strcmp(name, interface->name)) {
            if (ifindex != -1 && interface->index != ifindex) {
                INFO("interface name " PUB_S_SRP " index changed from %d to %d", name, interface->index, ifindex);
                interface->index = ifindex;
            }
            break;
        }
    }

    // If it's a new interface, make a structure.
    // We could do a callback, but don't have a use case
    if (*p_interface == NULL) {
        interface = interface_create(route_state, name, ifindex);
        if (interface != NULL) {
            if (route_state->thread_interface_name != NULL && !strcmp(name, route_state->thread_interface_name)) {
                interface->is_thread = true;
            }
            *p_interface = interface;
        }
    }
    return interface;
}


static void
interface_shutdown(interface_t *interface)
{
    icmp_message_t *router, *next;
    INFO("Interface " PUB_S_SRP " went away.", interface->name);
    if (interface->beacon_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->beacon_wakeup);
    }
    if (interface->post_solicit_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->post_solicit_wakeup);
    }
    if (interface->stale_evaluation_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->stale_evaluation_wakeup);
    }
    if (interface->router_solicit_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->router_solicit_wakeup);
    }
    if (interface->deconfigure_wakeup != NULL) {
        ioloop_cancel_wake_event(interface->deconfigure_wakeup);
    }
#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
    if (interface->vicarious_discovery_complete != NULL) {
        ioloop_cancel_wake_event(interface->vicarious_discovery_complete);
    }
    interface->vicarious_router_discovery_in_progress = false;
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
    for (router = interface->routers; router; router = next) {
        next = router->next;
        icmp_message_free(router);
    }
    interface->routers = NULL;
    interface->last_beacon = interface->next_beacon = 0;
    interface->deprecate_deadline = 0;
    interface->preferred_lifetime = interface->valid_lifetime = 0;
    interface->num_solicits_sent = 0;
    interface->inactive = true;
    interface->ineligible = true;
    interface->our_prefix_advertised = false;
    interface->suppress_ipv6_prefix = false;
    interface->have_link_layer_address = false;
    interface->on_link_prefix_configured = false;
    interface->sent_first_beacon = false;
    interface->num_beacons_sent = 0;
    interface->router_discovery_started = false;
    interface->router_discovery_complete = false;
    interface->router_discovery_in_progress = false;
    interface->need_reconfigure_prefix = false;
}

static void
interface_prefix_evaluate(interface_t *interface)
{
    route_state_t *route_state = interface->route_state;
    // Set up the interface prefix using the prefix number for the link.
    interface->ipv6_prefix = route_state->xpanid_prefix;
}


#ifndef RA_TESTER
static bool
router_is_advertising(icmp_message_t *router, const struct in6_addr *prefix, int preflen)
{
    for (int i = 0; i < router->num_options; i++) {
        icmp_option_t *option = &router->options[i];
        if (option->type == icmp_option_prefix_information) {
            prefix_information_t *pio = &option->option.prefix_information;
            if (pio->length != 64) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&pio->prefix, prefix_buf);
                INFO("invalid IP address prefix length: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&pio->prefix, prefix_buf), preflen);
                continue;
            }
            if (!in6prefix_compare(prefix, &pio->prefix, 8)) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&pio->prefix, prefix_buf);
                SEGMENTED_IPv6_ADDR_GEN_SRP(&router->source, router_buf);
                INFO("router at " PRI_SEGMENTED_IPv6_ADDR_SRP " advertised prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&router->source, router_buf),
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&pio->prefix, prefix_buf), preflen);
                return true;
            }
        }
    }
    return false;
}

static void
route_remove_routers_advertising_prefix(interface_t *interface, const struct in6_addr *prefix, int preflen)
{
    if (preflen != 64) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, prefix_buf);
        INFO("invalid IP address prefix length: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix, prefix_buf), preflen);
        return;
    }
    for (icmp_message_t **rp = &interface->routers; *rp != NULL; ) {
        icmp_message_t *router = *rp;
        if (router_is_advertising(router, prefix, preflen)) {
            *rp = router->next;
            router->next = NULL;
            icmp_message_free(router);
        } else {
            rp = &router->next;
        }
    }
}
#endif // RA_TESTER

static void
ifaddr_callback(srp_server_t *server_state, void *context, const char *name, const addr_t *address,
                const addr_t *mask, unsigned flags, enum interface_address_change change)
{
    char addrbuf[INET6_ADDRSTRLEN];
    const uint8_t *addrbytes, *maskbytes, *prefp;
    int preflen, i;
    interface_t *interface;
    route_state_t *route_state = context;

#ifndef POSIX_BUILD
    interface = find_interface(route_state, name, -1);
#else
    interface = find_interface(route_state, name, if_nametoindex(name));
#endif
    if (interface == NULL) {
        ERROR("find_interface returned NULL for " PUB_S_SRP, name);
        return;
    }

    const bool is_thread_interface = interface->is_thread;

    if (address->sa.sa_family == AF_INET) {
        addrbytes = (uint8_t *)&address->sin.sin_addr;
        maskbytes = (uint8_t *)&mask->sin.sin_addr;
        prefp = maskbytes + 3;
        preflen = 32;
        if (change == interface_address_added) {
            // Just got an IPv4 address?
            if (!interface->num_ipv4_addresses) {
                if (!(flags & (IFF_LOOPBACK | IFF_POINTOPOINT))) {
                    interface_prefix_evaluate(interface);
                }
            }
            interface->num_ipv4_addresses++;
        } else if (change == interface_address_deleted) {
            interface->num_ipv4_addresses--;
            // Just lost our last IPv4 address?
            if (!(flags & (IFF_LOOPBACK | IFF_POINTOPOINT))) {
                if (!interface->num_ipv4_addresses) {
                    interface_prefix_evaluate(interface);
                }
            }
        }
    } else if (address->sa.sa_family == AF_INET6) {
        if (change == interface_address_added) {
            interface->num_ipv6_addresses++;
        } else if (change == interface_address_deleted) {
            interface->num_ipv6_addresses--;
        }
        addrbytes = (uint8_t *)&address->sin6.sin6_addr;
        maskbytes = (uint8_t *)&mask->sin6.sin6_addr;
        prefp = maskbytes + 15;
        preflen = 128;
#ifndef LINUX
    } else if (address->sa.sa_family == AF_LINK) {
        snprintf(addrbuf, sizeof addrbuf, "%02x:%02x:%02x:%02x:%02x:%02x",
                 address->ether_addr.addr[0], address->ether_addr.addr[1],
                 address->ether_addr.addr[2], address->ether_addr.addr[3],
                 address->ether_addr.addr[4], address->ether_addr.addr[5]);
        prefp = (uint8_t *)&addrbuf[0]; maskbytes = prefp + 1; // Skip prefix length calculation
        preflen = 0;
        addrbytes = NULL;
#endif
    } else {
        INFO("Unknown address type %d", address->sa.sa_family);
        return;
    }

    if (change != interface_address_unchanged) {
#ifndef LINUX
        if (address->sa.sa_family == AF_LINK) {
            if (!interface->ineligible) {
                INFO("interface " PUB_S_SRP PUB_S_SRP " " PUB_S_SRP " " PRI_MAC_ADDR_SRP " flags %x",
                     name, is_thread_interface ? " (thread)" : "",
                     change == interface_address_added ? "added" : "removed",
                     MAC_ADDR_PARAM_SRP(address->ether_addr.addr), flags);
            }
        } else {
#endif
            for (; prefp >= maskbytes; prefp--) {
                if (*prefp) {
                    break;
                }
                preflen -= 8;
            }
            for (i = 0; i < 8; i++) {
                if (*prefp & (1<<i)) {
                    break;
                }
                --preflen;
            }
            inet_ntop(address->sa.sa_family, addrbytes, addrbuf, sizeof addrbuf);
            if (!interface->ineligible) {
                if (address->sa.sa_family == AF_INET) {
                    IPv4_ADDR_GEN_SRP(addrbytes, addr_buf);
                    INFO("interface " PUB_S_SRP PUB_S_SRP " " PUB_S_SRP " " PRI_IPv4_ADDR_SRP
                         "/%d flags %x", name, is_thread_interface ? " (thread)" : "",
                         change == interface_address_added ? "added" : "removed",
                         IPv4_ADDR_PARAM_SRP(addrbytes, addr_buf), preflen, flags);
                } else if (address->sa.sa_family == AF_INET6) {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(addrbytes, addr_buf);
                    INFO("interface " PUB_S_SRP PUB_S_SRP " " PUB_S_SRP " " PRI_SEGMENTED_IPv6_ADDR_SRP
                         "/%d flags %x", name, is_thread_interface ? " (thread)" : "",
                         change == interface_address_added ? "added" : "removed",
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(addrbytes, addr_buf), preflen, flags);
#ifndef RA_TESTER
                    if (change == interface_address_deleted) {
                        route_remove_routers_advertising_prefix(interface, &address->sin6.sin6_addr, preflen);
                        if (route_state->route_tracker != NULL) {
                            route_tracker_route_state_changed(route_state->route_tracker, interface);
                        }
                    }
#endif
                } else {
                    INFO("invalid sa_family: %d", address->sa.sa_family);
                }

                // Only notify dnssd-proxy when srp-mdns-proxy and dnssd-proxy is combined together.
#if !defined(RA_TESTER) && (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
                // Notify dnssd-proxy that address is added or removed.
                if (!is_thread_interface) {
                    if (change == interface_address_added) {
                        if (!interface->inactive) {
                            dnssd_proxy_ifaddr_callback(server_state, context, name, address, mask, flags, change);
                        }
                    } else { // change == interface_address_removed
                        dnssd_proxy_ifaddr_callback(server_state, context, name, address, mask, flags, change);
                    }
                }
#endif // #if !defined(RA_TESTER) && (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)

                // When new IP address is removed, it is possible that the existing router information, such as
                // PIO and RIO is no longer valid since srp-mdns-proxy is losing its IP address. In order to let it to
                // flush the stale router information as soon as possible, we mark all the router as stale immediately,
                // by setting the router received time to a value which is 601s ago (router will be stale if the router
                // information is received for more than 600s). And then do router discovery for 20s, so we can ensure
                // that all the stale router information will be updated during the discovery, or flushed away. If all
                // routers are flushed, then srp-mdns-proxy will advertise its own prefix and configure the new IPv6
                // address.
                if (address->sa.sa_family == AF_INET6 &&                                       // An IPv6 address
                    change == interface_address_deleted &&                                     // went away
                    in6prefix_compare(&address->sin6.sin6_addr, &interface->ipv6_prefix, 8) && // not one of ours
                    !is_thread_mesh_synthetic_or_link_local(&address->sin6.sin6_addr))         // not link-local
                {

                    INFO("clearing router discovery complete flag because address deleted.");
#ifdef VICARIOUS_ROUTER_DISCOVERY
                    INFO("making all routers stale and start router discovery due to removed address");
                    adjust_router_received_time(interface, ioloop_timenow(),
                                                -(MAX_ROUTER_RECEIVED_TIME_GAP_BEFORE_STALE + MSEC_PER_SEC));
#endif
                    // Explicitly set router_discovery_complete to false so we can ensure that srp-mdns-proxy will start
                    // the router discovery immediately.
                    interface->router_discovery_complete = false;
                    interface->router_discovery_started = false;
                    // Set need_reconfigure_prefix to true to let routing_policy_evaluate know that the router discovery
                    // is caused by interface removal event, so when the router discovery finished and nothing changes,
                    // it can reconfigure the IPv6 routing in case configured does not handle it correctly.
                    interface->need_reconfigure_prefix = true;
                    routing_policy_evaluate(interface, false);
                }
            }
#ifndef LINUX
        }
#endif
    }

    // Not a broadcast interface
    if (flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) {
        // Not the thread interface
        if (!is_thread_interface) {
            return;
        }
    }

    // 169.254.*
    if (address->sa.sa_family == AF_INET && IN_LINKLOCAL(address->sin.sin_addr.s_addr)) {
        return;
    }

    if (interface->index == -1) {
        interface->index = address->ether_addr.index;
    }

#if defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)
    if (is_thread_interface && address->sa.sa_family == AF_INET6) {
        partition_utun0_address_changed(route_state, &address->sin6.sin6_addr, change);
    }
#endif

    if (address->sa.sa_family == AF_INET) {
    } else if (address->sa.sa_family == AF_INET6) {
        if (IN6_IS_ADDR_LINKLOCAL(&address->sin6.sin6_addr)) {
            interface->link_local = address->sin6.sin6_addr;
        }
#ifndef LINUX
    } else if (address->sa.sa_family == AF_LINK) {
        if (address->ether_addr.len == 6) {
            if (change != interface_address_deleted) {
                memcpy(interface->link_layer, address->ether_addr.addr, 6);
                INFO("setting link layer address for " PUB_S_SRP " to " PRI_MAC_ADDR_SRP, interface->name,
                     MAC_ADDR_PARAM_SRP(interface->link_layer));
                interface->have_link_layer_address = true;
            } else {
                INFO("resetting link layer address for " PUB_S_SRP " (was " PRI_MAC_ADDR_SRP ")", interface->name,
                     MAC_ADDR_PARAM_SRP(interface->link_layer));
                memset(interface->link_layer, 0, 6);
                interface->have_link_layer_address = false;
            }
        }
#endif
    }
#if defined(POSIX_BUILD)
    interface_active_state_evaluate(interface, true, true);
#endif
}

#ifndef RA_TESTER
static void
route_get_mesh_local_prefix_callback(void *context, const char *prefix_string, cti_status_t status)
{
    route_state_t *route_state = context;
    char prefix_buf[INET6_ADDRSTRLEN];

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        attempt_wpan_reconnect(route_state);
        goto fail;
    }

    INFO(PRI_S_SRP " %d", prefix_string != NULL ? prefix_string : "<null>", status);
    if (status != kCTIStatus_NoError) {
        INFO("error %d", status);
    }
    if (prefix_string == NULL) {
        INFO("NULL prefix string");
        goto fail;
    }

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
    if (!inet_pton(AF_INET6, prefix_addr_string, &route_state->thread_mesh_local_prefix)) {
        ERROR("prefix syntax incorrect: " PRI_S_SRP, prefix_addr_string);
        goto fail;
    }
    SEGMENTED_IPv6_ADDR_GEN_SRP(route_state->thread_mesh_local_prefix.s6_addr, ml_prefix_buf);
    INFO(PRI_SEGMENTED_IPv6_ADDR_SRP PUB_S_SRP,
         SEGMENTED_IPv6_ADDR_PARAM_SRP(route_state->thread_mesh_local_prefix.s6_addr, ml_prefix_buf),
         slash ? slash : "");
    route_state->have_mesh_local_prefix = true;
    return;
fail:
    route_state->have_mesh_local_prefix = false;
    return;
}
#endif // RA_TESTER

void
route_refresh_interface_list(route_state_t *route_state)
{
    interface_t *interface;
    bool UNUSED have_active = false;
    // We sometimes do not get "interface down" notifications when moving from one WiFi SSID to the next. To detect that
    // this has happened, see if we go from nonzero IPv6 addresses to zero after scanning the interface addresses
    for (interface = route_state->interfaces; interface != NULL; interface = interface->next) {
        interface->old_num_ipv6_addresses = interface->num_ipv6_addresses;
    }
    ioloop_map_interface_addresses_here(route_state->srp_server, &route_state->interface_addresses, NULL, route_state, ifaddr_callback);

    for (interface = route_state->interfaces; interface; interface = interface->next) {
#if defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)
        if (interface->is_thread) {
            partition_utun0_pick_listener_address(route_state);
        }
#endif
        if (!interface->ineligible && !interface->inactive) {
            have_active = true;
        }

        if (!interface->ineligible && !interface->inactive &&
            interface->num_ipv6_addresses == 0 && interface->old_num_ipv6_addresses != 0)
        {
            flush_routers(interface, 0);
        }
    }

#ifndef RA_TESTER
    // Notice if we have lost or gained infrastructure.
    if (have_active && !route_state->have_non_thread_interface) {
        INFO("we have an active interface");
        route_state->have_non_thread_interface = true;
        route_state->partition_can_advertise_service = true;
        partition_maybe_advertise_anycast_service(route_state);
    } else if (!have_active && route_state->have_non_thread_interface) {
        INFO("we no longer have an active interface");
        route_state->have_non_thread_interface = false;
        route_state->partition_can_advertise_service = false;
        // Stop advertising the service, if we are doing so.
        partition_discontinue_all_srp_service(route_state);
    }
#endif // RA_TESTER
}


#if defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)
#if defined(POSIX_BUILD)
static void
wpan_reconnect_wakeup_callback(void *context)
{
    route_state_t *route_state = context;
    if (route_state->wpan_reconnect_wakeup != NULL) {
        ioloop_wakeup_release(route_state->wpan_reconnect_wakeup);
        route_state->wpan_reconnect_wakeup = NULL;
    }
    // Attempt to restart the thread network...
    infrastructure_network_startup(context);
}
#endif // POSIX_BUILD

static void
attempt_wpan_reconnect(void *context)
{
    route_state_t *route_state = context;
#if defined(POSIX_BUILD)
    if (route_state->wpan_reconnect_wakeup == NULL) {
        route_state->wpan_reconnect_wakeup = ioloop_wakeup_create();
        if (route_state->wpan_reconnect_wakeup == NULL) {
            ERROR("can't allocate wpan reconnect wait wakeup.");
            return;
        }
        INFO("delaying for ten seconds before attempt to reconnect to thread daemon.");
        ioloop_add_wake_event(route_state->wpan_reconnect_wakeup, NULL,
                              wpan_reconnect_wakeup_callback, NULL, 10 * 1000);
        partition_state_reset(route_state);
#endif
    }
}

static void
cti_get_tunnel_name_callback(void *context, const char *name, cti_status_t status)
{
    route_state_t *route_state = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        attempt_wpan_reconnect(route_state);
        return;
    }

    INFO(PUB_S_SRP " %d", name != NULL ? name : "<null>", status);
    if (status != kCTIStatus_NoError) {
        return;
    }
    route_state->num_thread_interfaces = 1;
    if (route_state->thread_interface_name != NULL) {
        free(route_state->thread_interface_name);
    }
    route_state->thread_interface_name = strdup(name);
    if (route_state->thread_interface_name == NULL) {
        ERROR("No memory to save thread interface name " PUB_S_SRP, name);
        return;
    }
    INFO("Thread interface at " PUB_S_SRP, route_state->thread_interface_name);
    partition_got_tunnel_name(route_state);
}

static void
cti_get_role_callback(void *context, cti_network_node_type_t role, cti_status_t status)
{
    route_state_t *route_state = context;
    bool am_thread_router = false;

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        attempt_wpan_reconnect(route_state);
        return;
    }

    if (status == kCTIStatus_NoError) {
        route_state->partition_last_role_change = ioloop_timenow();

        if (role == kCTI_NetworkNodeType_Router || role == kCTI_NetworkNodeType_Leader) {
            am_thread_router = true;
        }

        INFO("role is: " PUB_S_SRP " (%d)\n ", am_thread_router ? "router" : "not router", role);
    } else {
        ERROR("cti_get_role_callback: nonzero status %d", status);
    }

    // Our thread role doesn't actually matter, but it's useful to report it in the logs.
}

static void
cti_get_state_callback(void *context, cti_network_state_t state, cti_status_t status)
{
    route_state_t *route_state = context;
    bool associated = false;

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        attempt_wpan_reconnect(context);
        return;
    }

    route_state->partition_last_state_change = ioloop_timenow();

    if (status == kCTIStatus_NoError) {
        if ((state == kCTI_NCPState_Associated)     || (state == kCTI_NCPState_Isolated) ||
            (state == kCTI_NCPState_NetWake_Asleep) || (state == kCTI_NCPState_NetWake_Waking))
        {
            associated = true;
        }

        INFO("state is: " PUB_S_SRP " (%d)\n ", associated ? "associated" : "not associated", state);
    } else {
        ERROR("cti_get_state_callback: nonzero status %d", status);
    }

    if (route_state->current_thread_state != state) {
        if (associated) {
            route_state->current_thread_state = state;
            partition_maybe_enable_services(route_state); // but probably not
        } else {
            route_state->current_thread_state = state;
            partition_disable_service(route_state);
        }
    }
}

static void
re_evaluate_interfaces(route_state_t *route_state)
{
    for (interface_t *interface = route_state->interfaces; interface != NULL; interface = interface->next) {
        interface_prefix_evaluate(interface);
    }

    partition_maybe_enable_services(route_state);
}

static void
route_get_xpanid_callback(void *context, uint64_t new_xpanid, cti_status_t status)
{
    route_state_t *route_state = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        attempt_wpan_reconnect(route_state);
        return;
    }

    if (status == kCTIStatus_NoError) {
        if (route_state->partition_has_xpanid) {
            ERROR("Unexpected change to XPANID from %" PRIu64 " to %" PRIu64,
                  route_state->srp_server->xpanid, new_xpanid);
        } else {
            INFO("XPANID is now %" PRIu64, new_xpanid);
        }
    } else {
        ERROR("nonzero status %d", status);
        return;
    }

    route_state->srp_server->xpanid = new_xpanid;
    route_state->partition_has_xpanid = true;
    in6addr_zero(&route_state->xpanid_prefix);
    route_state->xpanid_prefix.s6_addr[0] = 0xfd;
    for (int i = 1; i < 8; i++) {
        route_state->xpanid_prefix.s6_addr[i] = ((route_state->srp_server->xpanid >> ((8 - i) * 8)) & 0xFFU);
    }
    route_state->have_xpanid_prefix = true;

#if SRP_FEATURE_REPLICATION
    if (route_state->srp_server->srp_replication_enabled) {
        INFO("start srp replication.");
        srpl_startup(route_state->srp_server);
    }
#endif // SRP_FEATURE_REPLICATION

    re_evaluate_interfaces(route_state);
}

void
adv_ctl_add_prefix(route_state_t *route_state, const uint8_t *const data)
{
    if (route_state->omr_watcher != NULL) {
        omr_prefix_t *thread_prefixes = omr_watcher_prefixes_get(route_state->omr_watcher);
        omr_prefix_t *prefix = NULL;

        for (prefix = thread_prefixes; prefix != NULL; prefix = prefix->next) {
            if (!memcmp(&prefix->prefix, data, BR_PREFIX_SLASH_64_BYTES)) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
                INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " already there",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf));
                break;
            }
        }
        if (prefix == NULL) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("adding prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf));
            if (!omr_watcher_prefix_add(route_state->omr_watcher, (struct in6_addr *)data, BR_PREFIX_SLASH_64_BYTES, omr_prefix_priority_low)) {
                INFO("failed");
            }
        }
    }
}

void
adv_ctl_remove_prefix(route_state_t *route_state, const uint8_t *const data)
{
    if (route_state->omr_watcher != NULL) {
        omr_prefix_t *thread_prefixes = omr_watcher_prefixes_get(route_state->omr_watcher);
        omr_prefix_t *prefix = NULL;

        for (prefix = thread_prefixes; prefix != NULL; prefix = prefix->next) {
            if (!memcmp(&prefix->prefix, data, BR_PREFIX_SLASH_64_BYTES)) {
                break;
            }
        }
        if (prefix == NULL) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(data, prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " not present",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(data, prefix_buf));
        } else {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("removing prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf));
            if (!omr_watcher_prefix_remove(route_state->omr_watcher, data, BR_PREFIX_SLASH_64_BYTES)) {
                INFO("no prefix removed.");
            }
        }
    }
}

static void
route_rloc16_callback(void *context, uint16_t rloc16, cti_status_t status)
{
    route_state_t *route_state = context;

    if (status != kCTIStatus_NoError) {
        ERROR("%d", status);
    } else {
        route_state->srp_server->rloc16 = rloc16;
        route_state->have_rloc16 = true;
        INFO("server_state->rloc16 updated to %d", route_state->srp_server->rloc16);
        // whenever the local rloc16 is updated, we should re-evaluate if anycast
        // service should be advertised.
        partition_maybe_advertise_anycast_service(route_state);
    }
}
#endif // THREAD_BORDER_ROUTER && !RA_TESTER

void
infrastructure_network_startup(route_state_t *route_state)
{
    INFO("Thread network started.");

//    ioloop_network_watcher_start(network_watch_event);
#ifndef RA_TESTER
    set_thread_forwarding();
#endif
}

#if defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)
static void
route_omr_watcher_event(route_state_t *route_state, void *UNUSED context, omr_watcher_event_type_t event_type,
                        omr_prefix_t *UNUSED prefixes, omr_prefix_t *UNUSED prefix)
{
    // Whenever we get an update to the prefix list, we should check our interface addresses.
    if (event_type == omr_watcher_event_prefix_update_finished) {
        route_refresh_interface_list(route_state);
        int num_prefixes = 0;
        for (omr_prefix_t *prf = prefixes; prf != NULL; prf = prf->next) {
            num_prefixes++;
        }
        if (num_prefixes != route_state->num_thread_prefixes) {
            int old_num_prefixes = route_state->num_thread_prefixes;
            INFO("%d prefixes instead of %d, evaluating policy", num_prefixes, route_state->num_thread_prefixes);
            routing_policy_evaluate_all_interfaces(route_state, true);
            route_state->num_thread_prefixes = num_prefixes;
            if (old_num_prefixes == 0 && num_prefixes > 0) {
                INFO("thread prefix available, may advertise anycast");
                partition_maybe_advertise_anycast_service(route_state);
            }
            if (old_num_prefixes > 0 && num_prefixes == 0) {
                INFO("all thread prefixes are gone, stop advertising anycast service");
                partition_stop_advertising_anycast_service(route_state, route_state->thread_sequence_number);
            }
        }
    }
}

static void
thread_network_startup(route_state_t *route_state)
{
    if (route_state->thread_network_shutting_down) {
        INFO("thread network still shutting down--canceling");
        ioloop_cancel_wake_event(route_state->thread_network_shutdown_wakeup);
        route_state->thread_network_shutting_down = false;
        return;
    }
    int status = cti_get_state(route_state->srp_server, &route_state->thread_state_context, route_state,
                               cti_get_state_callback, NULL);
    if (status == kCTIStatus_NoError) {
        status = cti_get_network_node_type(route_state->srp_server, &route_state->thread_role_context, route_state,
                                           cti_get_role_callback, NULL);
    }
    srp_server_t *server_state = route_state->srp_server;
    server_state->service_tracker = service_tracker_create(server_state);
    if (server_state->service_tracker != NULL) {
        service_tracker_callback_add(server_state->service_tracker,
                                     partition_service_set_changed, NULL, route_state);
        service_tracker_start(server_state->service_tracker);
    }
    if (status == kCTIStatus_NoError) {
        status = cti_get_tunnel_name(route_state->srp_server, route_state, cti_get_tunnel_name_callback, NULL);
    }
    if (status == kCTIStatus_NoError) {
        status = cti_get_extended_pan_id(route_state->srp_server, &route_state->thread_xpanid_context, route_state,
                                         route_get_xpanid_callback, NULL);
    }
    if (status == kCTIStatus_NoError) {
        status = cti_get_rloc16(route_state->srp_server, &route_state->thread_rloc16_context, route_state,
                                route_rloc16_callback, NULL);
    }
    if (status == kCTIStatus_NoError) {
        status = cti_get_mesh_local_prefix(route_state->srp_server, route_state,
                                           route_get_mesh_local_prefix_callback, NULL);
    }
    if (status != kCTIStatus_NoError) {
        if (status == kCTIStatus_DaemonNotRunning) {
            attempt_wpan_reconnect(route_state);
        } else {
            ERROR("initial network setup failed");
        }
    }
#if SRP_FEATURE_NAT64
    INFO("start nat64.");
    nat64_start(route_state);
#endif
    route_state->omr_watcher = omr_watcher_create(route_state, attempt_wpan_reconnect);
    if (route_state->omr_watcher == NULL) {
        ERROR("omr_watcher create failed");
        return;
    }
    route_state->omr_watcher_callback = omr_watcher_callback_add(route_state->omr_watcher,
                                                                 route_omr_watcher_event, NULL, route_state);
    if (route_state->omr_watcher_callback == NULL) {
        ERROR("omr_watcher_callback add failed");
        return;
    }
    route_state->omr_publisher = omr_publisher_create(route_state, "main");
    if (route_state->omr_publisher == NULL) {
        ERROR("omr_publisher create failed");
        return;
    }
    omr_publisher_set_omr_watcher(route_state->omr_publisher, route_state->omr_watcher);
    omr_publisher_set_reconnect_callback(route_state->omr_publisher, attempt_wpan_reconnect);
    omr_publisher_start(route_state->omr_publisher);
    omr_watcher_start(route_state->omr_watcher);
    route_state->thread_network_running = true;
}
#endif //  defined(THREAD_BORDER_ROUTER) && !defined(RA_TESTER)

#ifndef RA_TESTER
static void
thread_network_shutdown_wakeup_callback(void *context)
{
    route_state_t *route_state = context;
    INFO("shutdown timer expired, shutting down.");
    thread_network_shutdown(route_state);
}

static void
thread_network_shutdown_start(route_state_t *route_state)
{
    if (route_state->thread_network_shutdown_wakeup == NULL) {
        route_state->thread_network_shutdown_wakeup = ioloop_wakeup_create();
    }
    if (route_state->thread_network_shutdown_wakeup == NULL) {
        INFO("no memory for wakeup object");
        thread_network_shutdown(route_state);
    } else {
        INFO("scheduling shutdown in ten seconds");
        route_state->thread_network_shutting_down = true;
        ioloop_add_wake_event(route_state->thread_network_shutdown_wakeup,
                              route_state, thread_network_shutdown_wakeup_callback, NULL, 10 * 1000);
    }
}

static void
thread_network_shutdown(route_state_t *route_state)
{
    // If we get an explicit shutdown after getting a "shut down if nothing improves", cancel the scheduled shutdown.
    // This code also runs when we're called from the shutdown wakeup callback and serves to cancel that state.
    if (route_state->thread_network_shutdown_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->thread_network_shutdown_wakeup);
    }
    route_state->thread_network_shutting_down = false;
    if (route_state->thread_state_context != NULL) {
        INFO("discontinuing state events");
        cti_events_discontinue(route_state->thread_state_context);
        route_state->thread_state_context = NULL;
    }
    if (route_state->thread_role_context != NULL) {
        INFO("discontinuing role events");
        cti_events_discontinue(route_state->thread_role_context);
        route_state->thread_role_context = NULL;
    }
    if (route_state->thread_route_context != NULL) {
        INFO("discontinuing route events");
        cti_events_discontinue(route_state->thread_route_context);
        route_state->thread_route_context = NULL;
    }
    if (route_state->thread_xpanid_context != NULL) {
        INFO("discontinuing xpanid events");
        cti_events_discontinue(route_state->thread_xpanid_context);
        route_state->thread_xpanid_context = NULL;
    }
    if (route_state->thread_rloc16_context != NULL) {
        INFO("discontinuing rloc16 events");
        cti_events_discontinue(route_state->thread_rloc16_context);
        route_state->thread_rloc16_context = NULL;
    }
    if (route_state->thread_ml_prefix_connection != NULL) {
        INFO("discontinuing route events");
        cti_events_discontinue(route_state->thread_ml_prefix_connection);
        route_state->thread_ml_prefix_connection = NULL;
    }
    srp_mdns_flush(route_state->srp_server);
#if SRP_FEATURE_REPLICATION
    INFO("stop srp replication.");
    srpl_shutdown(route_state->srp_server);
#endif
#if SRP_FEATURE_NAT64
    INFO("stop nat64.");
    nat64_stop(route_state);
#endif
    partition_state_reset(route_state);
    route_state->thread_network_running = false;
}
#endif // RA_TESTER

void
infrastructure_network_shutdown(route_state_t *route_state)
{
    interface_t *interface;

#ifndef RA_TESTER
    if (route_state->thread_network_running) {
        thread_network_shutdown(route_state);
    }
#endif
    INFO("Infrastructure network shutdown.");
    // Stop all activity on interfaces.
    for (interface = route_state->interfaces; interface; interface = interface->next) {
        interface_shutdown(interface);
    }
    // Whatever non-thread interface we may have had we just shut down, so mark it down so that we can
    // start it up later.
    route_state->have_non_thread_interface = false;
}

#ifndef RA_TESTER
static void
partition_state_reset(route_state_t *route_state)
{
    if (route_state->omr_watcher) {
        if (route_state->omr_watcher_callback != NULL) {
            INFO("canceling omr watcher callback");
            omr_watcher_callback_cancel(route_state->omr_watcher, route_state->omr_watcher_callback);
            route_state->omr_watcher_callback = NULL;
        }
        INFO("discontinuing omr watcher");
        omr_watcher_cancel(route_state->omr_watcher);
        omr_watcher_release(route_state->omr_watcher);

        route_state->omr_watcher = NULL;
    }
    if (route_state->omr_publisher) {
        INFO("discontinuing omr publisher");
        omr_publisher_cancel(route_state->omr_publisher);
        omr_publisher_release(route_state->omr_publisher);
        route_state->omr_publisher = NULL;
    }
    if (route_state->route_tracker) {
        INFO("discontinuing route tracker");
        route_tracker_cancel(route_state->route_tracker);
        route_tracker_release(route_state->route_tracker);
        route_state->route_tracker = NULL;
    }
    srp_server_t *server_state = route_state->srp_server;
    if (server_state->service_tracker != NULL) {
        service_tracker_cancel(server_state->service_tracker);
        service_tracker_release(server_state->service_tracker);
        server_state->service_tracker = NULL;
    }

    route_state->current_thread_state = kCTI_NCPState_Uninitialized;
    route_state->partition_last_prefix_set_change = 0;
    route_state->partition_last_pref_id_set_change = 0;
    route_state->partition_last_role_change = 0;
    route_state->partition_last_state_change = 0;
    route_state->partition_settle_start = 0;
    route_state->partition_service_last_add_time = 0;
    route_state->partition_have_prefix_list = false;
    route_state->partition_have_pref_id_list = false;
    route_state->partition_tunnel_name_is_known = false;
    route_state->partition_can_advertise_service = false;
    route_state->partition_can_advertise_anycast_service = false;
    route_state->srp_server->srp_anycast_service_blocked = false;
    route_state->srp_server->srp_unicast_service_blocked = false;
    route_state->partition_can_provide_routing = false;
    route_state->partition_has_xpanid = false;
    route_state->partition_may_offer_service = false;
    route_state->partition_settle_satisfied = true;
    route_state->have_rloc16 = false;
    route_state->advertising_srp_anycast_service = false;

    if (route_state->partition_settle_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->partition_settle_wakeup);
    }

    if (route_state->partition_post_partition_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->partition_post_partition_wakeup);
    }

    if (route_state->partition_pref_id_wait_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->partition_pref_id_wait_wakeup);
    }

    if (route_state->partition_service_add_pending_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->partition_service_add_pending_wakeup);
    }

    if (route_state->partition_anycast_service_add_pending_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->partition_service_add_pending_wakeup);
    }

    if (route_state->service_set_changed_wakeup != NULL) {
        ioloop_cancel_wake_event(route_state->service_set_changed_wakeup);
    }
}

static void
partition_proxy_listener_ready(void *context, uint16_t port)
{
    srp_server_t *server_state = context;
    route_state_t *route_state = server_state->route_state;

    INFO("listening on port %d", port);
    route_state->srp_service_listen_port = port;
    if (route_state->have_non_thread_interface) {
        route_state->partition_can_advertise_service = true;
        partition_maybe_advertise_service(route_state);
    } else {
        partition_discontinue_srp_service(route_state);
    }
}

static void
partition_srp_listener_canceled(comm_t *listener, void *context)
{
    srp_server_t *server_state = context;
    route_state_t *route_state = server_state->route_state;

    INFO("listener is %p", listener);
    if (route_state->srp_listener == listener) {
        ioloop_comm_release(route_state->srp_listener);
        route_state->srp_listener = NULL;

        if (!server_state->srp_unicast_service_blocked) {
            partition_discontinue_srp_service(route_state);
        }
    }
}

static void
partition_stop_srp_listener(route_state_t *route_state)
{
    if (route_state->srp_listener != NULL) {
        INFO("discontinuing SRP service on port %d", route_state->srp_service_listen_port);
        ioloop_listener_cancel(route_state->srp_listener);
        ioloop_comm_release(route_state->srp_listener);
        route_state->srp_listener = NULL;
    }
}

void
partition_start_srp_listener(route_state_t *route_state)
{
#define max_avoid_ports 100
    uint16_t avoid_ports[max_avoid_ports];
    int num_avoid_ports = 0;
    thread_service_t *service;

    for (service = service_tracker_services_get(route_state->srp_server->service_tracker);
         service != NULL; service = service->next)
    {
        if (service->service_type == unicast_service) {
            // Track the port regardless.
            if (num_avoid_ports < max_avoid_ports) {
                avoid_ports[num_avoid_ports] = (service->u.unicast.port[0] << 8) | (service->u.unicast.port[1]);
                num_avoid_ports++;
            }
        }
    }

    // Make sure we don't overwrite the listener without stopping it.
    partition_stop_srp_listener(route_state);

    INFO("starting listener.");
    route_state->srp_listener = srp_proxy_listen(avoid_ports, num_avoid_ports, NULL, partition_proxy_listener_ready,
                                                 partition_srp_listener_canceled, NULL, NULL, route_state->srp_server);
    if (route_state->srp_listener == NULL) {
        ERROR("Unable to start SRP listener, so can't advertise it");
        return;
    }
}

void
partition_discontinue_srp_service(route_state_t *route_state)
{
    partition_stop_srp_listener(route_state);

    // Won't match
    in6addr_zero(&route_state->srp_listener_ip_address);
    route_state->srp_service_listen_port = 0;

    // Stop advertising the service, if we are doing so.
    partition_stop_advertising_service(route_state);
}

void
partition_discontinue_all_srp_service(route_state_t *route_state)
{
    partition_discontinue_srp_service(route_state);
    partition_stop_advertising_anycast_service(route_state, route_state->thread_sequence_number);
}

// An address on utun0 has changed.  Evaluate what to do with our listener service.
// This gets called from ifaddr_callback().   If we don't yet have a thread service configured,
// it should be called for unchanged addresses as well as changed.
static void
partition_utun0_address_changed(route_state_t *route_state, const struct in6_addr *addr,
                                enum interface_address_change change)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(addr, addr_buf);

    // Is this the address we are currently using?
    if (!in6addr_compare(&route_state->srp_listener_ip_address, addr)) {
        route_state->seen_listener_address = true;

        // Did it go away?   If so, drop the listener.
        if (change == interface_address_deleted) {
            INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": listener address removed.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            if (route_state->srp_listener != NULL) {
                INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": canceling listener on removed address.",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
                partition_discontinue_srp_service(route_state);
            }
        } else {
            // This should never happen.
            if (change == interface_address_added) {
                ERROR(PRI_SEGMENTED_IPv6_ADDR_SRP ": address we're listening on was added.",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            } else {
                INFO("still listening on " PRI_SEGMENTED_IPv6_ADDR_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            }
        }

        // Nothing more to do for this address.
        return;
    }

    // No point in looking at addresses if we don't have prerequisites.
    if (!route_state->have_mesh_local_prefix || !route_state->have_non_thread_interface) {
        return;
    }

    // Otherwise, we don't care about deleted addresses, but added and existing addresses matter.
    if (change != interface_address_deleted) {
        // If this address isn't an address we're already listening on, check if it's an anycast address; if so,
        // skip it as a candidate to listen on.
        if (is_thread_mesh_synthetic_address(addr)) {
            INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": thread anycast address.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
        }

        // If it's not an anycast address, do an election on it against the previously-seen addresses in this
        // iteration of ioloop_map_interface_addresses(). Numerically lowest address wins. Note that this means
        // that a link-local address will always lose, and if we have any anycast address we at least have a
        // mesh-local address, and that's fine to use if it happens to win. If we could figure out what our
        // mesh-local prefix was, we'd actually prefer this address since it never changes.
        else {
            // Don't use the mesh-local prefix
            if (!in6prefix_compare(addr, &route_state->thread_mesh_local_prefix, 8)) {
                INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": is our mesh-local address, skipping",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            }
            // RFC4297: Link-Scoped Unicast address range FE80::/10
            else if (addr->s6_addr[0] == 0xfe && (addr->s6_addr[1] & 0xc0) == 0x80) {
                INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": is our link-local address, skipping",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            }
            // If the address is not on the list of prefixes we know about, let's not use it.
            else if (route_state->omr_watcher == NULL ||
                     !omr_watcher_prefix_exists(route_state->omr_watcher, addr, 64))
            {
                INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": is unknown, skipping",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
            }
            // Otherwise it's legit and we can use it
            else {
                if (!route_state->have_proposed_srp_listener_address ||
                    in6addr_compare(&route_state->proposed_srp_listener_address, addr) > 0)
                {
                    if (route_state->have_proposed_srp_listener_address) {
                        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": wins over previous winner.",
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
                    } else {
                        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": wins by being first.",
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
                    }
                    in6addr_copy(&route_state->proposed_srp_listener_address, addr);
                    route_state->have_proposed_srp_listener_address = true;
                }
            }
        }
    } else {
        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": removed.", SEGMENTED_IPv6_ADDR_PARAM_SRP(addr, addr_buf));
    }
}

static void
partition_utun0_pick_listener_address(route_state_t *route_state)
{
    if (route_state->have_mesh_local_prefix && route_state->advertising_srp_anycast_service &&
        route_state->have_non_thread_interface && route_state->have_proposed_srp_listener_address)
    {
        if (route_state->srp_listener == NULL) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->proposed_srp_listener_address, addr_buf);
            INFO("starting listener on" PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->proposed_srp_listener_address, addr_buf));

            // Copy the winning proposed listener IP address to the listener IP address
            in6addr_copy(&route_state->srp_listener_ip_address, &route_state->proposed_srp_listener_address);

            // Set up a listener.
            route_state->srp_service_listen_port = 0;
            partition_start_srp_listener(route_state);
        } else {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->srp_listener_ip_address, addr_buf);
            if (!route_state->seen_listener_address) {
                FAULT("didn't see listener address " PRI_SEGMENTED_IPv6_ADDR_SRP,
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf));
            } else {
                INFO("already listening on" PRI_SEGMENTED_IPv6_ADDR_SRP,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf));
            }
        }
    } else {
        INFO(PUB_S_SRP "advertising anycast service; " PUB_S_SRP " proposed listener address; "
             PUB_S_SRP " non-thread interface; " PUB_S_SRP " mesh-local prefix; " PUB_S_SRP " listener",
             route_state->advertising_srp_anycast_service    ? "" : "not ",
             route_state->have_proposed_srp_listener_address ? "have" : "no",
             route_state->have_non_thread_interface          ? "have" : "no",
             route_state->have_mesh_local_prefix             ? "have" : "no",
             route_state->srp_listener != NULL               ? "have" : "no");
        // In common cases, if we are not advertising anycast service due to replication failure,
        // we can not advertise unicast either. One exception is that we manually block the anycast
        // service for testing purpose. Unicast service should not be affected in this case.
        if (route_state->srp_listener != NULL && !route_state->advertising_srp_anycast_service &&
            !route_state->srp_server->srp_anycast_service_blocked) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->srp_listener_ip_address, addr_buf);
            INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": canceling listener.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf));
            partition_discontinue_srp_service(route_state);
        }
    }

    // Clear all of the election state.
    route_state->seen_listener_address = false;
    route_state->have_proposed_srp_listener_address = false;
    in6addr_zero(&route_state->proposed_srp_listener_address);
}

static void
partition_got_tunnel_name(route_state_t *route_state)
{
    route_state->partition_tunnel_name_is_known = true;
    for (interface_t *interface = route_state->interfaces; interface; interface = interface->next) {
        if (!strcmp(interface->name, route_state->thread_interface_name)) {
            interface->is_thread = true;
            break;
        }
    }
    route_refresh_interface_list(route_state);
}

static void
partition_remove_service_done(void *context, cti_status_t status)
{
    route_state_t *route_state = context;
    INFO("%d", status);

    // Flush any advertisements we're currently doing, since the accessories that advertised them will
    // notice the service is gone and start advertising with a different service.
#if defined(SRP_FEATURE_REPLICATION)
    if (!route_state->srp_server->srp_replication_enabled) {
#endif
        srp_mdns_flush(route_state->srp_server);
#if defined(SRP_FEATURE_REPLICATION)
    }
#endif
}

static bool
route_maybe_restart_service_tracker(route_state_t *route_state, int *reset, int *increment)
{
    *reset = 0;
    (*increment)++;
    if (*increment > 5) {
        if (route_state->srp_server->service_tracker != NULL) {
            service_tracker_start(route_state->srp_server->service_tracker);
        } else {
            FAULT("service tracker not present when restarting.");
        }
        *increment = 0;
        return true;
    }
    return false;
}

static void
partition_stop_advertising_service(route_state_t *route_state)
{
    // This should remove any copy of the service that this BR is advertising.
    INFO("%" PRIu64 "/%x", THREAD_ENTERPRISE_NUMBER, THREAD_SRP_SERVER_OPTION);
    uint8_t service_info[] = { 0, 0, 0, 1 };
    int status;

    if (route_maybe_restart_service_tracker(route_state, &route_state->times_advertised_unicast, &route_state->times_unadvertised_unicast)) {
        INFO("restarted service tracker.");
    }
    service_info[0] = THREAD_SRP_SERVER_OPTION & 255;
    status = cti_remove_service(route_state->srp_server, route_state, partition_remove_service_done, NULL,
                                THREAD_ENTERPRISE_NUMBER, service_info, 1);
    if (status != kCTIStatus_NoError) {
        INFO("status %d", status);
    }
    route_state->advertising_srp_unicast_service = false;
}

void
partition_stop_advertising_anycast_service(route_state_t *route_state, uint8_t sequence_number)
{
    // This should remove any copy of the service that this BR is advertising.
    INFO("%" PRIu64 "/%x %x", THREAD_ENTERPRISE_NUMBER, THREAD_SRP_SERVER_ANYCAST_OPTION, sequence_number);
    uint8_t service_info[] = { 0, 0, 0, 1 };
    int status;

    if (route_maybe_restart_service_tracker(route_state, &route_state->times_advertised_anycast, &route_state->times_unadvertised_anycast)) {
        INFO("restarted service tracker.");
    }
    service_info[0] = THREAD_SRP_SERVER_ANYCAST_OPTION & 255;
    service_info[1] = sequence_number;
    status = cti_remove_service(route_state->srp_server, route_state, partition_remove_service_done, NULL,
                                THREAD_ENTERPRISE_NUMBER, service_info, 2);
    if (status != kCTIStatus_NoError) {
        INFO("status %d", status);
    }
    route_state->advertising_srp_anycast_service = false;
    if (route_state->route_tracker != NULL) {
        INFO("discontinuing route tracker");
        route_tracker_cancel(route_state->route_tracker);
        route_tracker_release(route_state->route_tracker);
        route_state->route_tracker = NULL;
    }
    route_refresh_interface_list(route_state);
}

static void
partition_add_service_callback(void *context, cti_status_t status)
{
    route_state_t *UNUSED route_state = context;
    if (status != kCTIStatus_NoError) {
        INFO("status = %d", status);
    } else {
        INFO("status = %d", status);
    }
}

static void
partition_start_advertising_service(route_state_t *route_state)
{
    uint8_t service_info[] = {0, 0, 0, 1};
    uint8_t server_info[18];
    int ret;

    if (route_maybe_restart_service_tracker(route_state, &route_state->times_unadvertised_unicast, &route_state->times_advertised_unicast)) {
        INFO("restarted service tracker.");
    }
    memcpy(&server_info, &route_state->srp_listener_ip_address, 16);
    server_info[16] = (route_state->srp_service_listen_port >> 8) & 255;
    server_info[17] = route_state->srp_service_listen_port & 255;

    SEGMENTED_IPv6_ADDR_GEN_SRP(route_state->srp_listener_ip_address.s6_addr, server_ip_buf);
    service_info[0] = THREAD_SRP_SERVER_OPTION & 255;
    INFO("%" PRIu64 "/%02x/" PRI_SEGMENTED_IPv6_ADDR_SRP ":%d" ,
         THREAD_ENTERPRISE_NUMBER, service_info[0],
         SEGMENTED_IPv6_ADDR_PARAM_SRP(route_state->srp_listener_ip_address.s6_addr, server_ip_buf),
         route_state->srp_service_listen_port);

    ret = cti_add_service(route_state->srp_server, route_state, partition_add_service_callback, NULL,
                          THREAD_ENTERPRISE_NUMBER, service_info, 1, server_info, sizeof server_info);
    if (ret != kCTIStatus_NoError) {
        INFO("status %d", ret);
    }

    // Wait a while for the service add to be reflected in an event.
    partition_schedule_service_add_wakeup(route_state);
    route_state->advertising_srp_unicast_service = true;
}

static void
partition_start_advertising_anycast_service(route_state_t *route_state)
{
    uint8_t service_info[] = {0, 0, 0, 1};
    int ret;

    if (route_maybe_restart_service_tracker(route_state, &route_state->times_unadvertised_anycast, &route_state->times_advertised_anycast)) {
        INFO("restarted service tracker.");
    }
    service_info[0] = THREAD_SRP_SERVER_ANYCAST_OPTION & 255;
    service_info[1] = route_state->thread_sequence_number;
    INFO("%" PRIu64 "/%02x/ %x", THREAD_ENTERPRISE_NUMBER, service_info[0], route_state->thread_sequence_number);

    ret = cti_add_service(route_state->srp_server, route_state, partition_add_service_callback, NULL,
                          THREAD_ENTERPRISE_NUMBER, service_info, 2, NULL, 0);
    if (ret != kCTIStatus_NoError) {
        INFO("status %d", ret);
    }

    // Wait a while for the service add to be reflected in an event.
    partition_schedule_anycast_service_add_wakeup(route_state);
    route_state->advertising_srp_anycast_service = true;
    route_refresh_interface_list(route_state);

    if (route_state->route_tracker == NULL) {
        route_state->route_tracker = route_tracker_create(route_state, "main");
        if (route_state->route_tracker == NULL) {
            ERROR("route_tracker create failed");
            return;
        }
        route_tracker_set_reconnect_callback(route_state->route_tracker, attempt_wpan_reconnect);
        route_tracker_start(route_state->route_tracker);
    } else {
        INFO("route tracker already running.");
    }
}

static void
partition_service_add_wakeup(void *context)
{
    route_state_t *route_state = context;
    route_state->partition_service_last_add_time = 0;
    partition_maybe_advertise_service(route_state);
}

static void
partition_anycast_service_add_wakeup(void *context)
{
    route_state_t *route_state = context;
    route_state->partition_service_last_add_time = 0;
    partition_maybe_advertise_anycast_service(route_state);
}

static void
partition_schedule_service_add_wakeup(route_state_t *route_state)
{
    if (route_state->partition_service_add_pending_wakeup == NULL) {
        route_state->partition_service_add_pending_wakeup = ioloop_wakeup_create();
        if (route_state->partition_service_add_pending_wakeup == NULL) {
            ERROR("Can't schedule service add pending wakeup: no memory!");
            return;
        }
    } else {
        ioloop_cancel_wake_event(route_state->partition_service_add_pending_wakeup);
    }
    // Wait thirty seconds.
    ioloop_add_wake_event(route_state->partition_service_add_pending_wakeup, route_state,
                          partition_service_add_wakeup, NULL, 30 * 1000);
}

static void
partition_schedule_anycast_service_add_wakeup(route_state_t *route_state)
{
    if (route_state->partition_anycast_service_add_pending_wakeup == NULL) {
        route_state->partition_anycast_service_add_pending_wakeup = ioloop_wakeup_create();
        if (route_state->partition_anycast_service_add_pending_wakeup == NULL) {
            ERROR("Can't schedule anycast service add pending wakeup: no memory!");
            return;
        }
    } else {
        ioloop_cancel_wake_event(route_state->partition_anycast_service_add_pending_wakeup);
    }
    // Wait thirty seconds.
    ioloop_add_wake_event(route_state->partition_anycast_service_add_pending_wakeup, route_state,
                          partition_anycast_service_add_wakeup, NULL, 30 * 1000);
}

static void
partition_maybe_advertise_service(route_state_t *route_state)
{
    thread_service_t *service;
    int num_lower_services = 0;
    int num_other_services = 0;
    int num_legacy_services = 0;
    int i;
    int64_t last_add_time;
    bool advertising_service = false;

    // If we aren't ready to advertise a service, there's nothing to do.
    if (!route_state->partition_can_advertise_service) {
        INFO("no service to advertise yet.");
        return;
    }

    if (route_state->srp_server->srp_unicast_service_blocked) {
        INFO("service advertising is disabled.");
        return;
    }

    for (i = 0; i < 16; i++) {
        if (route_state->srp_listener_ip_address.s6_addr[i] != 0) {
            break;
        }
    }
    if (i == 16) {
        INFO("no listener.");
        return;
    }

    // The add service function requires a remove prior to the add, so if we are doing an add, we need to wait
    // for things to stabilize before allowing the removal of a service to trigger a re-evaluation.
    // Therefore, if we've done an add in the past ten seconds, wait ten seconds before trying another add.
    last_add_time = ioloop_timenow() - route_state->partition_service_last_add_time;
    INFO("last_add_time = %" PRId64, last_add_time);
    if (last_add_time < 10 * 1000) {
    schedule_wakeup:
        partition_schedule_service_add_wakeup(route_state);
        return;
    }

    // Count how many services are numerically lower than the listener address
    for (service = service_tracker_services_get(route_state->srp_server->service_tracker);
         service; service = service->next)
    {
        if (service->ignore || service->service_type != unicast_service) {
            continue;
        }

        if ((service->user || (route_state->have_rloc16 && service->rloc16 == route_state->srp_server->rloc16)) &&
            (in6addr_compare(&service->u.unicast.address, &route_state->srp_listener_ip_address) ||
             ((service->u.unicast.port[0] << 8) | service->u.unicast.port[1]) != route_state->srp_service_listen_port))
        {
            thread_service_note("Rtr0", service, "is ours, but stale");
            partition_stop_advertising_service(route_state);
            goto schedule_wakeup;
        }

        // See if host advertising this unicast service is also advertising an anycast service; if not, then this
        // unicast service doesn't count (much).
        bool anycast_present = false;
        for (thread_service_t *aservice = service_tracker_services_get(route_state->srp_server->service_tracker);
             aservice != NULL; aservice = aservice->next)
        {
            if (aservice->ignore || aservice->service_type != anycast_service) {
                continue;
            }
            if (service->rloc16 == aservice->rloc16) {
                anycast_present = true;
                break;
            }
        }
        if (!anycast_present) {
            num_legacy_services++;
            route_state->seen_legacy_service = true;
            continue;
        }

        int cmp = in6addr_compare(&service->u.unicast.address, &route_state->srp_listener_ip_address);
        SEGMENTED_IPv6_ADDR_GEN_SRP(&service->u.unicast.address, addr_buf);
        INFO(PUB_S_SRP PRI_SEGMENTED_IPv6_ADDR_SRP ": is " PUB_S_SRP " listener address.",
             anycast_present ? "legacy service " : "service ",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&service->u.unicast.address, addr_buf),
             cmp < 0 ? "less than" : cmp > 0 ? "greater than" : "equal to");
        if (cmp < 0) {
            num_lower_services++;
        } else if (cmp == 0) {
            advertising_service = true;
        } else {
            num_other_services++;
        }
    }

    // We only want to advertise our service if there are no services being advertised on addresses that are lower than
    // ours. Also, if we notice that a service is being advertised by our rloc16 with a different IP address than the
    // listener address, it's a stale address, so remove it. If we have seen a legacy service, and there is only one
    // other non-legacy service, continue to advertise a second service, since cooperating services are preferable.
    if ((num_lower_services > 0  && !route_state->seen_legacy_service) || num_lower_services > 1) {
        if (advertising_service) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->srp_listener_ip_address, addr_buf);
            INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": stopping advertising unicast service.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf));
            partition_stop_advertising_service(route_state);
            route_state->partition_service_last_add_time = ioloop_timenow();
        } else {
            INFO("not advertising unicast service.");
        }
    }
    // If there is not some other service published, and we are not publishing, publish.  If there is a legacy (no
    // anycast) service published, publish a second service so as to encourage the legacy service to withdraw, because
    // cooperating services are preferable to competing services.
    else if (num_other_services < 1 || (num_other_services < 2 && route_state->seen_legacy_service)) {
        if (num_legacy_services > 1 && num_other_services > 1) {
            ERROR("%d legacy services present!", num_legacy_services);
        } else if (!advertising_service) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->srp_listener_ip_address, addr_buf);
            INFO(PRI_SEGMENTED_IPv6_ADDR_SRP ": starting advertising unicast service.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf));
            partition_start_advertising_service(route_state);
            route_state->partition_service_last_add_time = ioloop_timenow();
        } else {
            INFO("already advertising unicast service.");
        }
    }
    // There is some other service published.
    else {
        INFO("another service is present, no need to advertise.");
    }
}

void
partition_maybe_advertise_anycast_service(route_state_t *route_state)
{
    int64_t last_add_time;
    bool publish = false;

    // If we aren't ready to advertise a service, there's nothing to do.
    if (!route_state->have_non_thread_interface) {
        INFO("no active interface.");
        return;
    }

    if (!route_state->partition_can_advertise_service) {
        INFO("service advertisements are blocked.");
        return;
    }

    if (!route_state->partition_can_advertise_anycast_service) {
        INFO("no service to advertise yet.");
        return;
    }

    if (route_state->srp_server->srp_anycast_service_blocked) {
        INFO("service advertising is disabled.");
        return;
    }

    if (route_state->num_thread_prefixes == 0) {
        INFO("OMR prefix is not yet advertised.");
        return;
    }

    // The add service function requires a remove prior to the add, so if we are doing an add, we need to wait
    // for things to stabilize before allowing the removal of a service to trigger a re-evaluation.
    // Therefore, if we've done an add in the past ten seconds, wait ten seconds before trying another add.
    last_add_time = ioloop_timenow() - route_state->partition_service_last_add_time;
    INFO("last_add_time = %" PRId64, last_add_time);
    if (last_add_time < 10 * 1000) {
    schedule_wakeup:
        partition_schedule_anycast_service_add_wakeup(route_state);
        return;
    }

    // Find the highest (two's complement math) sequence number
    uint8_t winning_seq = route_state->thread_sequence_number;
    for (thread_service_t *service = service_tracker_services_get(route_state->srp_server->service_tracker); service;
         service = service->next)
    {
        if (service->service_type != anycast_service) {
            continue;
        }
        struct thread_anycast_service *aservice = &service->u.anycast;
        // Eliminate stale services before doing anything else.
        if ((service->user || (route_state->have_rloc16 && service->rloc16 == route_state->srp_server->rloc16)) &&
            (aservice->sequence_number != route_state->thread_sequence_number ||
            !route_state->advertising_srp_anycast_service))
        {
            thread_service_note("Rtr0", service, "is ours, but stale");
            partition_stop_advertising_anycast_service(route_state, aservice->sequence_number);
            goto schedule_wakeup;
        }
#ifdef PING_ANYCAST_SERVICE
        if (service->ignore) {
            continue;
        }
        route_ping_aservice(route_state, service);
#endif
        uint8_t service_seq = aservice->sequence_number;
        int8_t distance = service_seq - winning_seq;
        if (distance > 0) {
            winning_seq = service_seq;
        }
        if (distance == -128) {
            if ((int8_t)service_seq > (int8_t)winning_seq) {
                winning_seq = service_seq;
            }
        }
    }

    if (winning_seq != route_state->thread_sequence_number) {
        INFO("our sequence number (0x%02x) loses the election to 0x%02x", route_state->thread_sequence_number, winning_seq);
        goto publication;
    }

    // Count how many services on our anycast sequence number have lower RLOCs
    int num_less = 0;
    for (thread_service_t *service = service_tracker_services_get(route_state->srp_server->service_tracker); service;
         service = service->next)
    {
        struct thread_anycast_service *aservice = &service->u.anycast;
        if (aservice->sequence_number == winning_seq) {
            if (service->rloc16 < route_state->srp_server->rloc16) {
                num_less++;
            }
        }
    }

    if (num_less >= MAX_ANYCAST_NUM) {
        INFO("our sequence number (0x%02x) wins, but there are %d other services published already",
             route_state->thread_sequence_number, num_less);
        goto publication;
    }

    INFO("our sequence number (0x%02x) wins, and there are %d (<5) other services published already",
         route_state->thread_sequence_number, num_less);
    publish = true;
publication:
    if (publish) {
        if (!route_state->advertising_srp_anycast_service) {
            INFO("advertising our anycast service with sequence number 0x%02x", route_state->thread_sequence_number);
            partition_start_advertising_anycast_service(route_state);
            route_state->partition_service_last_add_time = ioloop_timenow();
        } else {
            INFO("already advertising our anycast service with sequence number 0x%x",
                 route_state->thread_sequence_number);
        }
    } else {
        if (!route_state->advertising_srp_anycast_service) {
            INFO("not advertising our anycast service with sequence number 0x%02x",
                 route_state->thread_sequence_number);
        } else {
            INFO("withdrawing our anycast service advertisement with sequence number 0x%02x",
                 route_state->thread_sequence_number);
            partition_stop_advertising_anycast_service(route_state, route_state->thread_sequence_number);
            route_state->partition_service_last_add_time = ioloop_timenow();
        }
    }
}

static void
partition_service_set_changed_callback(void *context)
{
    route_state_t *route_state = context;
    service_tracker_t *tracker = route_state->srp_server->service_tracker;

    // If we discover an advertised service with our rloc, but listening address or port does not
    // match, we need to remove it.
    for (thread_service_t *service = service_tracker_services_get(tracker); service != NULL; service = service->next) {
        if (service->ignore) {
            continue;
        }
        if (service->service_type == unicast_service) {
            if (service->user || (route_state->srp_server->have_rloc16 && service->rloc16 == route_state->srp_server->rloc16)) {
                uint16_t port = (service->u.unicast.port[0] << 8) | service->u.unicast.port[1];
                if (in6addr_compare(&service->u.unicast.address, &route_state->srp_listener_ip_address) ||
                    port != route_state->srp_service_listen_port)
                {
                    service_tracker_thread_service_note(tracker, service, "is ours, but stale");
                    partition_stop_advertising_service(route_state);
                    service->ignore = true;
                }
            }
        } else if (service->service_type == anycast_service) {
            // If we discover an advertised service with our rloc, and either we aren't advertising an anycast service,
            // or the sequence number isn't the one we're supposed to be advertising, we need to remove it.
            if ((service->user || (route_state->srp_server->have_rloc16 && service->rloc16 == route_state->srp_server->rloc16)) &&
                (!route_state->advertising_srp_anycast_service ||
                 service->u.anycast.sequence_number != route_state->thread_sequence_number))
            {
                service_tracker_thread_service_note(tracker, service, "is ours, but stale");
                partition_stop_advertising_anycast_service(route_state, service->u.anycast.sequence_number);
                service->ignore = true;
            }
        }
    }

    partition_maybe_advertise_service(route_state);
    partition_maybe_advertise_anycast_service(route_state);
}

static void
partition_service_set_changed(void *context)
{
    route_state_t *route_state = context;
    if (route_state->service_set_changed_wakeup == NULL) {
        route_state->service_set_changed_wakeup = ioloop_wakeup_create();
        if (route_state->service_set_changed_wakeup == NULL) {
            ERROR("Can't schedule service list change wakeup: no memory!");
            return;
        }
    } else {
        ioloop_cancel_wake_event(route_state->service_set_changed_wakeup);
    }
    int timeout = srp_random16() % 20000; // Randomly wait between zero and twenty seconds
    INFO("waiting %d milliseconds before processing service state change.", timeout);
    ioloop_add_wake_event(route_state->service_set_changed_wakeup, route_state,
                          partition_service_set_changed_callback, NULL, timeout);
}

static void partition_maybe_enable_services(route_state_t *route_state)
{
    bool am_associated = route_state->current_thread_state == kCTI_NCPState_Associated;
    if (am_associated) {
        bool restart = false;
        INFO("Enabling service, which was disabled because of the thread role or state.");
        route_state->partition_may_offer_service = true;
        route_state->partition_can_provide_routing = true;
        route_refresh_interface_list(route_state);
        routing_policy_evaluate_all_interfaces(route_state, true);
        if (route_state->omr_watcher == NULL) {
            if (route_state->omr_publisher != NULL) {
                omr_publisher_cancel(route_state->omr_publisher);
                omr_publisher_release(route_state->omr_publisher);
                route_state->omr_publisher = NULL;
            }
            route_state->omr_watcher = omr_watcher_create(route_state, attempt_wpan_reconnect);
            if (route_state->omr_watcher == NULL) {
                ERROR("omr_watcher create failed");
                return;
            }
            route_state->omr_watcher_callback = omr_watcher_callback_add(route_state->omr_watcher,
                                                                         route_omr_watcher_event, NULL, route_state);
            if (route_state->omr_watcher_callback == NULL) {
                ERROR("omr_watcher_callback add failed");
                return;
            }
            restart = true;
        }
        if (route_state->omr_publisher == NULL) {
            route_state->omr_publisher = omr_publisher_create(route_state, "main");
            if (route_state->omr_publisher == NULL) {
                ERROR("omr_publisher create failed");
                return;
            }
            omr_publisher_set_omr_watcher(route_state->omr_publisher, route_state->omr_watcher);
            omr_publisher_set_reconnect_callback(route_state->omr_publisher, attempt_wpan_reconnect);
            restart = true;
        }
        if (restart) {
            omr_publisher_start(route_state->omr_publisher);
            omr_watcher_start(route_state->omr_watcher);
        }
    } else {
        INFO("Not enabling service: " PUB_S_SRP,
             am_associated ? "associated" : "!associated");
    }
}

static void partition_disable_service(route_state_t *route_state)
{
    bool done_something = false;

    // When our node type or state is such that we should no longer be publishing a prefix, the NCP will
    // automatically remove the published prefix.  In case this happens, we do not want to remember the
    // prefix as already having been published.  So drop our recollection of the published
    // prefix; this will get cleaned up when the network comes back if there's an inconsistency.
    if (route_state->omr_publisher != NULL) {
        omr_publisher_cancel(route_state->omr_publisher);
        omr_publisher_release(route_state->omr_publisher);
        route_state->omr_publisher = NULL;
        done_something = true;
    }
    if (route_state->omr_watcher != NULL) {
        if (route_state->omr_watcher_callback != NULL) {
            omr_watcher_callback_cancel(route_state->omr_watcher, route_state->omr_watcher_callback);
            route_state->omr_watcher_callback = NULL;
        }
        omr_watcher_cancel(route_state->omr_watcher);
        omr_watcher_release(route_state->omr_watcher);
        route_state->omr_watcher = NULL;
        done_something = true;
    }

    // We want to always say something when we pass through this state.
    if (done_something) {
        INFO("did something");
    } else {
        INFO("did nothing.");
    }

    route_state->partition_may_offer_service = false;
    route_state->partition_can_provide_routing = false;
}

void partition_block_anycast_service(route_state_t *route_state, bool block)
{
    if (block) {
        if (!route_state->srp_server->srp_anycast_service_blocked) {
            route_state->srp_server->srp_anycast_service_blocked = block;
            partition_stop_advertising_anycast_service(route_state, route_state->thread_sequence_number);
        }
    } else {
        if (route_state->srp_server->srp_anycast_service_blocked) {
            route_state->srp_server->srp_anycast_service_blocked = block;
            partition_maybe_advertise_anycast_service(route_state);
        }
    }
}

#endif // RA_TESTER

#if SRP_FEATURE_LOCAL_DISCOVERY
int
route_get_current_infra_interface_index(void)
{
    extern srp_server_t *srp_servers;
    if (srp_servers == NULL) {
        INFO("no SRP servers");
        return -1;
    }
    route_state_t *route_state = srp_servers->route_state;
    if (route_state == NULL) {
        INFO("no route state");
        return -1;
    }
    for (interface_t *interface = route_state->interfaces; interface != NULL; interface = interface->next) {
        if (!interface->inactive && !interface->is_thread) {
            return (int)interface->index; // real interface indexes are always positive integers
        }
    }
    return -1;
}
#endif // SRP_FEATURE_LOCAL_DISCOVERY
#endif // STUB_ROUTER

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
