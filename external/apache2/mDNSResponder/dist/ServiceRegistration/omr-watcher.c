/* omr-watcher.c
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
 * This file contains the implementation of the omr_watcher_t object, which tracks off-mesh-routable prefixes on the
 * Thread network.
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

#include "cti-services.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "adv-ctl-server.h"
#include "dnssd-proxy.h"
#include "srp-proxy.h"
#include "route.h"
#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "nat64.h"
#include "route-tracker.h"

struct omr_watcher_callback {
    omr_watcher_callback_t *next;
    omr_watcher_event_callback_t callback;
    omr_watcher_context_release_callback_t context_release_callback;
    void *context;
    bool canceled;
};

struct omr_watcher {
    int ref_count;
    route_state_t *route_state;
    omr_watcher_callback_t *callbacks;
    cti_connection_t route_connection;
    cti_connection_t prefix_connection;
    wakeup_t *prefix_recheck_wakeup;
    omr_prefix_t *prefixes;
    void (*disconnect_callback)(void *context);
    uint16_t prefix_connections_pending;
    bool purge_pending;
    bool first_time;
    bool prefix_recheck_pending;
    bool awaiting_unpublication;
};

static void
omr_prefix_metadata_set(omr_prefix_t *prefix, int metric, int flags, int rloc, bool stable, bool ncp)
{
    prefix->metric        = metric;
    prefix->flags         = flags;
    prefix->rloc          = rloc;
    prefix->stable        = stable;
    prefix->ncp           = ncp;
    prefix->user          = !ncp;
    prefix->onmesh        = CTI_PREFIX_FLAGS_ON_MESH(flags);
    prefix->slaac         = CTI_PREFIX_FLAGS_SLAAC(flags);
    prefix->dhcp          = CTI_PREFIX_FLAGS_DHCP(flags);
    prefix->preferred     = CTI_PREFIX_FLAGS_PREFERRED(flags);
    int priority       = CTI_PREFIX_FLAGS_PRIORITY(flags);
    switch(priority) {
    case kCTIPriorityMedium:
        prefix->priority = omr_prefix_priority_medium;
        break;
    case kCTIPriorityHigh:
        prefix->priority = omr_prefix_priority_high;
        break;
    default:
    case kCTIPriorityReserved:
        prefix->priority = omr_prefix_priority_invalid;
        break;
    case kCTIPriorityLow:
        prefix->priority = omr_prefix_priority_low;
        break;
    }
}

omr_prefix_t *
omr_prefix_create(struct in6_addr *prefix, int prefix_length, int metric, int flags, int rloc, bool stable, bool ncp)
{
    omr_prefix_t *ret = calloc(1, sizeof(*ret));
    if (ret != NULL) {
        RETAIN_HERE(ret, omr_prefix);
        ret->prefix        = *prefix;
        ret->prefix_length = prefix_length;
        omr_prefix_metadata_set(ret, metric, flags, rloc, stable, ncp);
    }
    return ret;
}

int
omr_prefix_flags_generate(bool on_mesh, bool preferred, bool slaac, omr_prefix_priority_t priority)
{
    int flags = 0;
    if (on_mesh) {
        CTI_PREFIX_FLAGS_ON_MESH_SET(flags, 1);
    }
    if (preferred) {
        CTI_PREFIX_FLAGS_PREFERRED_SET(flags, 1);
    }
    if (slaac) {
        CTI_PREFIX_FLAGS_SLAAC_SET(flags, 1);
    }
    if (priority) {
        CTI_PREFIX_FLAGS_PRIORITY_SET(flags, omr_prefix_priority_to_bits(priority));
    }
    return flags;
}

int
omr_prefix_priority_to_bits(omr_prefix_priority_t priority)
{
    switch(priority) {
    case omr_prefix_priority_invalid:
        return 2;
        break;
    case omr_prefix_priority_low:
        return 3;
        break;
    case omr_prefix_priority_medium:
        return 0;
        break;
    case omr_prefix_priority_high:
        return 1;
        break;
    }
}

int
omr_prefix_priority_to_int(omr_prefix_priority_t priority)
{
    switch(priority) {
    case omr_prefix_priority_invalid:
        // We should never be asked for an invalid priority, but if we are, low is good.
        return -1;
        break;
    case omr_prefix_priority_low:
        return -1;
        break;
    case omr_prefix_priority_medium:
        return 0;
        break;
    case omr_prefix_priority_high:
        return 1;
        break;
    }
}

static void
omr_prefix_finalize(omr_prefix_t *prefix)
{
    free(prefix);
}

static void
omr_watcher_finalize(omr_watcher_t *omw)
{
    omr_prefix_t *next;

    if (omw->prefix_recheck_wakeup != NULL) {
        ioloop_cancel_wake_event(omw->prefix_recheck_wakeup);
        ioloop_wakeup_release(omw->prefix_recheck_wakeup);
        omw->prefix_recheck_wakeup = NULL;
    }

    for (omr_prefix_t *prefix = omw->prefixes; prefix != NULL; prefix = next) {
        next = prefix->next;
        RELEASE_HERE(prefix, omr_prefix);
    }

    // The omr_watcher_t can have a route_connection and a prefix_connection, but each of these will retain
    // a reference to the omr_watcher, so we can't get here while these connections are still alive. Hence,
    // we do not need to free them here.
    free(omw);
}

static void
omr_watcher_callback_finalize(omr_watcher_t *omw, omr_watcher_callback_t *callback)
{
    if (callback->context != 0 && callback->context_release_callback) {
        callback->context_release_callback(omw->route_state, callback->context);
    }
    free(callback);
}

static void
omr_watcher_purge_canceled_callbacks(void *context)
{
    omr_watcher_callback_t *cb, **pcb;
    omr_watcher_t *omw = context;

    for (pcb = &omw->callbacks; *pcb; ) {
        cb = *pcb;
        if (cb->canceled) {
            *pcb = cb->next;
            omr_watcher_callback_finalize(omw, cb);
        } else {
            pcb = &((*pcb)->next);
        }
    }
    RELEASE_HERE(omw, omr_watcher);
}

static void
omr_watcher_send_prefix_event(omr_watcher_t *omw, omr_watcher_event_type_t event_type,
                      omr_prefix_t *NULLABLE prefixes, omr_prefix_t *NULLABLE prefix)
{
    omr_watcher_callback_t *cb;

    for (cb = omw->callbacks; cb; cb = cb->next) {
        if (!cb->canceled) {
            cb->callback(omw->route_state, cb->context, event_type, prefixes, prefix);
        }
    }
}

static void
omr_watcher_prefix_list_callback(void *context, cti_prefix_vec_t *prefixes, cti_status_t status)
{
    omr_watcher_t *omw = context;
    size_t i;
    omr_prefix_t **ppref = &omw->prefixes, *prefix = NULL, **new = NULL;
    bool something_changed = false;
    bool user_prefix_seen = false;

    INFO("status: %d  prefixes: %p  count: %d", status, prefixes, prefixes == NULL ? -1 : (int)prefixes->num);

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        omw->disconnect_callback(omw->route_state);
        goto out;
    }

    if (status != kCTIStatus_NoError) {
        ERROR("unhandled error %d", status);
        goto out;
    }

    // Delete any prefixes that are not in the list provided by Thread.
    while (*ppref != NULL) {
        prefix = *ppref;

        for (i = 0; i < prefixes->num; i++) {
            cti_prefix_t *cti_prefix = prefixes->prefixes[i];

            // Is this prefix still present?
            if (!in6prefix_compare(&prefix->prefix, &cti_prefix->prefix, 8)) {
                break;
            }
        }
        if (i == prefixes->num) {
            omr_watcher_send_prefix_event(omw, omr_watcher_event_prefix_withdrawn, NULL, prefix);
            *ppref = prefix->next;
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d went away" PUB_S_SRP PUB_S_SRP PUB_S_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length,
                 prefix->user ? " (user)" : "", prefix->ncp ? " (ncp)": "", prefix->stable ? " (stable)" : "");
            RELEASE_HERE(prefix, omr_prefix);
            something_changed = true;
        } else {
            // We'll re-initialize these flags from the prefix list when we check for duplicates.
            prefix->previous_user = prefix->user;
            prefix->previous_ncp = prefix->ncp;
            prefix->previous_stable = prefix->stable;
            prefix->user = false;
            prefix->stable = false;
            prefix->ncp = false;
            ppref = &prefix->next;
            prefix->removed = false;
            prefix->added = false;
            prefix->ignore = false;
        }
    }

    // On exit, ppref is pointing to the end-of-list pointer. If after we scan the cti prefix list a second time,
    // we discover new prefixes, the first new prefix will be pointed to by *new.
    new = ppref;

    // Add any prefixes that are not present.
    for (i = 0; i < prefixes->num; i++) {
        cti_prefix_t *cti_prefix = prefixes->prefixes[i];
        SEGMENTED_IPv6_ADDR_GEN_SRP(cti_prefix->prefix.s6_addr, prefix_buf);
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d is in thread-supplied prefix list",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(cti_prefix->prefix.s6_addr, prefix_buf), cti_prefix->prefix_length);
        for (prefix = omw->prefixes; prefix != NULL; prefix = prefix->next) {
            if (!in6addr_compare(&prefix->prefix, &cti_prefix->prefix)) {
                INFO("present");
                break;
            }
        }
        if (prefix == NULL) {
            INFO("not present");
            prefix = omr_prefix_create(&cti_prefix->prefix, cti_prefix->prefix_length, cti_prefix->metric,
                                       cti_prefix->flags, cti_prefix->rloc, cti_prefix->stable, cti_prefix->ncp);
            if (prefix == NULL) {
                ERROR("no memory for prefix.");
            } else {
                *ppref = prefix;
                ppref = &prefix->next;
            }
        }
        // Also, since we're combing the list, update ncp, user and stable flags.   Note that a prefix can
        // appear more than once in the thread prefix list. Also look for a mismatch between the priority: if
        // we see a "user" priority of low and a "ncp" priority of high, this is a bug in the
        if (prefix != NULL) {
            if (cti_prefix->ncp) {
                prefix->ncp = true;
            } else {
                user_prefix_seen = true;
                prefix->user = true;
            }
            if (cti_prefix->stable) {
                prefix->stable = true;
            }
        }
    }
    for (prefix = omw->prefixes; prefix != NULL && prefix != *new; prefix = prefix->next) {
        if (prefix->user != prefix->previous_user || prefix->ncp != prefix->previous_ncp ||
            prefix->previous_stable != prefix->stable)
        {
            omr_watcher_send_prefix_event(omw, omr_watcher_event_prefix_flags_changed, NULL, prefix);
            something_changed = true;
        }
    }
    for (prefix = *new; prefix; prefix = prefix->next) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d showed up" PUB_S_SRP PUB_S_SRP PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length,
             prefix->user ? " (user)" : "", prefix->ncp ? " (ncp)": "", prefix->stable ? " (stable)" : "");
        omr_watcher_send_prefix_event(omw, omr_watcher_event_prefix_added, NULL, prefix);
        something_changed = true;
    }
    INFO("omw->prefixes = %p", omw->prefixes);
    for (prefix = omw->prefixes; prefix != NULL; prefix = prefix->next) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d is currently in the list " PUB_S_SRP PUB_S_SRP PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length,
             prefix->user ? " (user)" : "", prefix->ncp ? " (ncp)": "", prefix->stable ? " (stable)" : "");
    }
    if (something_changed || omw->first_time) {
        omr_watcher_send_prefix_event(omw, omr_watcher_event_prefix_update_finished, omw->prefixes, NULL);
        omw->first_time = false;
    }
    if (!user_prefix_seen && omw->route_state->srp_server->awaiting_prefix_removal) {
        omw->route_state->srp_server->awaiting_prefix_removal = false;
        adv_ctl_thread_shutdown_status_check(omw->route_state->srp_server);
    }
out:
    // Discontinue events (currently we'll only get one callback: this just dereferences the object so it can be freed.)
    INFO("prefix_connections_pending = %d", omw->prefix_connections_pending);
    omw->prefix_connections_pending--;
    if (omw->prefix_connection != NULL) {
        cti_events_discontinue(omw->prefix_connection);
        omw->prefix_connection = NULL;
        RELEASE_HERE(omw, omr_watcher); // We aren't going to get another callback.
    }
}

omr_watcher_callback_t *
omr_watcher_callback_add_(omr_watcher_t *omw, omr_watcher_event_callback_t callback,
                          omr_watcher_context_release_callback_t context_release, void *context,
                          const char *UNUSED file, int UNUSED line)
{
    omr_watcher_callback_t *ret = calloc(1, sizeof (*ret));
    if (ret != NULL) {
        omr_watcher_callback_t **cpp = &omw->callbacks;
        ret->callback = callback;
        ret->context = context;
        ret->context_release_callback = context_release;
        while (*cpp) {
            cpp = &((*cpp)->next);
        }
        *cpp = ret;
    }
    return ret;
}

void
omr_watcher_callback_cancel(omr_watcher_t *omw, omr_watcher_callback_t *callback)
{
    if (omw->prefix_recheck_wakeup != NULL) {
        ioloop_cancel_wake_event(omw->prefix_recheck_wakeup);
        ioloop_wakeup_release(omw->prefix_recheck_wakeup);
        omw->prefix_recheck_wakeup = NULL;
    }
    for (omr_watcher_callback_t *cb = omw->callbacks; cb != NULL; cb = cb->next) {
        if (cb == callback) {
            // Because a callback might be removed during a callback, and we don't want to have to worry about the callback
            // list being modified while we're traversing it, we just mark the callback canceled so it won't be called again
            // and schedule omr_watcher_purge_canceled_callbacks to run after we return to the event loop, where it will free any
            // callbacks marked canceled. We retain omw here in case one of the callbacks releases the last reference to it.
            cb->canceled = true;
            if (!omw->purge_pending) {
                omw->purge_pending = true;
                RETAIN_HERE(omw, omr_watcher);
                ioloop_run_async(omr_watcher_purge_canceled_callbacks, omw);
            }
        }
    }
}

omr_watcher_t *
omr_watcher_create_(route_state_t *route_state, void (*disconnect_callback)(void *), const char *UNUSED file, int UNUSED line)
{
    omr_watcher_t *omw = calloc(1, sizeof (*omw));
    RETAIN_HERE(omw, omr_watcher);
    omw->route_state = route_state;
    omw->disconnect_callback = disconnect_callback;
    omw->first_time = true;
    return omw;
}

static void
omr_watcher_offmesh_route_list_callback(void *context, cti_route_vec_t *vec, cti_status_t status)
{
    omr_watcher_t *omw = context;
#if SRP_FEATURE_NAT64
    if (omw->route_state->nat64 != NULL) {
        nat64_offmesh_route_list_callback(omw->route_state, vec, status);
    }
#endif
    if (status == kCTIStatus_NoError) {
        if (omw->route_state->route_tracker != NULL) {
            route_tracker_monitor_mesh_routes(omw->route_state->route_tracker, vec);
        }
    }
    // Release the context if it hasn't already been released.
    if (omw->route_state->thread_route_context) {
        cti_events_discontinue(omw->route_state->thread_route_context);
        omw->route_state->thread_route_context = NULL;
        RELEASE_HERE(omw, omr_watcher); // we won't get any more callbacks on this connection.
    }
}

static void
omr_watcher_wakeup_release(void *context)
{
    omr_watcher_t *watcher = context;
    RELEASE_HERE(watcher, omr_watcher);
}

static void omr_watcher_prefix_list_fetch(omr_watcher_t *watcher);

static void
omr_watcher_prefix_recheck_wakeup(void *context)
{
    omr_watcher_t *watcher = context;
    watcher->prefix_recheck_pending = false;
    bool need_recheck = false;

    // See if there are any prefixes on the list that should have been updated but haven't been
    for (omr_prefix_t *prefix = watcher->prefixes; prefix != NULL; prefix = prefix->next) {
        if (prefix->added || prefix->removed) {
            need_recheck = true;
        }
    }
    if (need_recheck) {
        INFO("prefixes expected to be refreshed were not.");
        omr_watcher_prefix_list_fetch(watcher);
    }
}

static void
omr_watcher_prefix_list_fetch(omr_watcher_t *watcher)
{
    // Postpone any recheck, since we're checking now.
    if (watcher->prefix_recheck_pending && watcher->prefix_recheck_wakeup != NULL) {
        ioloop_add_wake_event(watcher->prefix_recheck_wakeup, watcher, omr_watcher_prefix_recheck_wakeup,
                              omr_watcher_wakeup_release, 15 * MSEC_PER_SEC);
        RETAIN_HERE(watcher, omr_watcher); // for wake event
        watcher->prefix_recheck_pending = true;
    }

    int rv = cti_get_onmesh_prefix_list(watcher->route_state->srp_server, &watcher->prefix_connection,
                                        watcher, omr_watcher_prefix_list_callback, NULL);
    if (rv != kCTIStatus_NoError) {
        ERROR("can't get onmesh prefix list: %d", rv);
        return;
    }
    INFO("prefix_connections_pending = %d", watcher->prefix_connections_pending);
    watcher->prefix_connections_pending++;
    RETAIN_HERE(watcher, omr_watcher); // For the callback
}

// For now, the onmesh prefix property doesn't support change events, so we track the IPv6:Routes property, which does.
static void
omr_watcher_route_update_callback(void *context, cti_prefix_vec_t *UNUSED prefixes, cti_status_t status)
{
    omr_watcher_t *omw = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        // Note: this will cancel and release route_state->omr_watcher, which will result in omw->route_connection being NULL
        // when we exit.
        omw->disconnect_callback(omw->route_state);
        goto fail;
    }

    if (status != kCTIStatus_NoError) {
        ERROR("unhandled error %d", status);
        goto fail;
    }

    // Release the context if there's one ongoing.
    if (omw->prefix_connection != NULL) {
        omw->prefix_connections_pending--;
        cti_events_discontinue(omw->prefix_connection);
        omw->prefix_connection = NULL;
        RELEASE_HERE(omw, omr_watcher); // we won't get any more callbacks on this connection.
    }

    omr_watcher_prefix_list_fetch(omw);

    // check offmesh routes
    INFO("prefix_list finished, start to get offmesh route list");
    // Release the context if there's one ongoing.
    if (omw->route_state->thread_route_context) {
        cti_events_discontinue(omw->route_state->thread_route_context);
        omw->route_state->thread_route_context = NULL;
        RELEASE_HERE(omw, omr_watcher); // we won't get any more callbacks on this connection.
    }
    int rv = cti_get_offmesh_route_list(omw->route_state->srp_server, &omw->route_state->thread_route_context,
                                        omw, omr_watcher_offmesh_route_list_callback, NULL);
    if (rv != kCTIStatus_NoError) {
        ERROR("can't get offmesh route: %d", status);
        return;
    }
    RETAIN_HERE(omw, omr_watcher); // For the callback
    // We can expect further events.
    return;

fail:
    // We don't want any more events.
    if (omw->route_connection) {
        cti_events_discontinue(omw->route_connection);
        omw->route_connection = NULL;
        RELEASE_HERE(omw, omr_watcher); // We won't get any more callbacks, so release the omr_watcher_t.
    }
}

bool
omr_watcher_start(omr_watcher_t *omw)
{
    int status = cti_get_prefix_list(omw->route_state->srp_server, &omw->route_connection,
                                     omw, omr_watcher_route_update_callback, NULL);
    if (status == kCTIStatus_NoError) {
        RETAIN_HERE(omw, omr_watcher); // for the callback
        return true;
    }
    return false;
}

void
omr_watcher_cancel(omr_watcher_t *omw)
{
    // In case the only remaining reference(s) are held by the callbacks (which should never be the case).
    RETAIN_HERE(omw, omr_watcher);

    INFO("prefix_connections_pending = %d", omw->prefix_connections_pending);
    if (omw->prefix_connection != NULL) {
        omw->prefix_connections_pending--;
        cti_events_discontinue(omw->prefix_connection);
        omw->prefix_connection = NULL;
        RELEASE_HERE(omw, omr_watcher);
    }
    if (omw->route_connection != NULL) {
        cti_events_discontinue(omw->route_connection);
        omw->route_connection = NULL;
        RELEASE_HERE(omw, omr_watcher);
    }

    RELEASE_HERE(omw, omr_watcher);
}

bool
omr_watcher_prefix_present(omr_watcher_t *watcher, omr_prefix_priority_t priority,
                           struct in6_addr *ignore_prefix, int ignore_prefix_length)
{
    static struct in6_addr in6addr_zero;
    SEGMENTED_IPv6_ADDR_GEN_SRP(ignore_prefix, ignore_buf);
    if (in6addr_compare(ignore_prefix, &in6addr_zero)) {
        INFO("prefix to ignore: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(ignore_prefix->s6_addr, ignore_buf), ignore_prefix_length);
    }
    for (omr_prefix_t *prefix = watcher->prefixes; prefix != NULL; prefix = prefix->next) {
        if (prefix->prefix_length == ignore_prefix_length && !in6addr_compare(&prefix->prefix, ignore_prefix)) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("ignoring prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            continue;
        }
        if (prefix->priority == priority) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("matched prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            return true;
        }
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
        INFO("didn't match prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
    }
    INFO("returning false");
    return false;
}

bool
omr_watcher_non_ula_prefix_present(omr_watcher_t *watcher)
{
    for (omr_prefix_t *prefix = watcher->prefixes; prefix != NULL; prefix = prefix->next) {
        if (omr_watcher_prefix_is_non_ula_prefix(prefix)) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("matched prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            return true;
        }
    }
    INFO("returning false");
    return false;
}

bool
omr_watcher_prefix_exists(omr_watcher_t *watcher, const struct in6_addr *address, int prefix_length)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(address, target_buf);
    INFO("address: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(address->s6_addr, target_buf), prefix_length);
    for (omr_prefix_t *prefix = watcher->prefixes; prefix != NULL; prefix = prefix->next) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
        if (prefix->prefix_length == prefix_length &&
            !in6prefix_compare(&prefix->prefix, address, (prefix_length + 7) /8))
        {
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d matches!",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            return true;
        }
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d doesn't match!",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
    }
    return false;
}

bool
omr_watcher_prefix_wins(omr_watcher_t *watcher, omr_prefix_priority_t priority,
                        struct in6_addr *my_prefix, int my_prefix_length)
{
    for (omr_prefix_t *prefix = watcher->prefixes; prefix != NULL; prefix = prefix->next) {
        if (prefix->priority != priority) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("ignoring prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            continue;
        }
        if (prefix->prefix_length == my_prefix_length && in6addr_compare(&prefix->prefix, my_prefix) > 0) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d won",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
            return true;
        }
        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d didn't win",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf), prefix->prefix_length);
    }
    INFO("returning false");
    return false;
}

omr_prefix_t *
omr_watcher_prefixes_get(omr_watcher_t *watcher)
{
    return watcher->prefixes;
}

static bool
omr_watcher_prefix_add_remove(omr_watcher_t *watcher, const void *prefix_bits, int prefix_length,
                              omr_prefix_priority_t priority, bool remove)
{
    int flags = omr_prefix_priority_to_bits(priority) << kCTIPriorityShift;
    omr_prefix_t **opl, *prefix = NULL;

    // If we already have this prefix, update the metadata.
    for (opl = &watcher->prefixes; *opl != NULL; opl = &(*opl)->next) {
        prefix = *opl;

        if (prefix->prefix_length == prefix_length &&
            !in6prefix_compare(&prefix->prefix, prefix_bits, (prefix_length + 7) / 8))
        {
            omr_prefix_metadata_set(prefix, 0, flags, 0, true, true);
            goto out;
        }
    }

    // Otherwise allocate a new one.
    prefix = omr_prefix_create((struct in6_addr *)prefix_bits, prefix_length, 0, flags, 0, true, true);
    if (prefix == NULL) {
        goto out;
    }
    *opl = prefix;
out:
    if (prefix != NULL) {
        if (remove) {
            prefix->added = false;
            prefix->removed = true;
            prefix->ignore = true;
        } else {
            prefix->added = true;
            prefix->removed = false;
            prefix->ignore = false;
        }
        if (watcher->prefix_recheck_wakeup == NULL) {
            watcher->prefix_recheck_wakeup = ioloop_wakeup_create();
        }
        if (watcher->prefix_recheck_wakeup != NULL) {
            ioloop_add_wake_event(watcher->prefix_recheck_wakeup, watcher, omr_watcher_prefix_recheck_wakeup,
                                  omr_watcher_wakeup_release, 15 * MSEC_PER_SEC);
            RETAIN_HERE(watcher, omr_watcher); // for wake event
            watcher->prefix_recheck_pending = true;
        }
    }
    return prefix != NULL;
}

bool
omr_watcher_prefix_add(omr_watcher_t *watcher, const void *prefix_bits, int prefix_length, omr_prefix_priority_t priority)
{
    return omr_watcher_prefix_add_remove(watcher, prefix_bits, prefix_length, priority, false);
}

bool
omr_watcher_prefix_remove(omr_watcher_t *watcher, const void *prefix_bits, int prefix_length)
{
    return omr_watcher_prefix_add_remove(watcher, prefix_bits, prefix_length, omr_prefix_priority_invalid, true);
}

RELEASE_RETAIN_FUNCS(omr_watcher);
RELEASE_RETAIN_FUNCS(omr_prefix);

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
