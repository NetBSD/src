/* nat64.c
 *
 * Copyright (c) 2022-2023 Apple Inc. All rights reserved.
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
 */

#include <netinet/in.h>
#include "srp-log.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-mdns-proxy.h"
#include "nat64.h"
#include "nat64-macos.h"
#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "omr-publisher.h"

#if SRP_FEATURE_NAT64
static void nat64_infra_prefix_publisher_event_init(nat64_infra_prefix_publisher_event_t *event, nat64_infra_prefix_publisher_event_type_t event_type);
static void nat64_infra_prefix_publisher_event_deliver(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event);
static void nat64_br_prefix_publisher_event_init(nat64_br_prefix_publisher_event_t *event, nat64_br_prefix_publisher_event_type_t event_type);
static void nat64_br_prefix_publisher_event_deliver(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t *event);
static void nat64_add_prefix_to_update_queue(nat64_t *nat64, nat64_prefix_t *prefix, nat64_prefix_action action);
static void nat64_prefix_start_next_update(nat64_t *nat64);
static bool nat64_query_prefix_on_infra(nat64_infra_prefix_monitor_t *state_machine);

static void
nat64_prefix_finalize(nat64_prefix_t *prefix)
{
    free(prefix);
}

static void
nat64_finalize(nat64_t *nat64)
{
    free(nat64);
}

static nat64_ipv4_default_route_monitor_t *
nat64_ipv4_default_route_monitor_create(nat64_t *nat64)
{
    nat64_ipv4_default_route_monitor_t *monitor = calloc(1, sizeof(*monitor));
    if (monitor == NULL) {
        return monitor;
    }
    RETAIN_HERE(monitor, nat64_ipv4_default_route_monitor);
    monitor->nat64 = nat64;
    RETAIN_HERE(monitor->nat64, nat64);
    return monitor;
}

static void
nat64_ipv4_default_route_monitor_cancel(nat64_ipv4_default_route_monitor_t *monitor)
{
    if (monitor != NULL) {
        monitor->has_ipv4_default_route = false;
        if (monitor->nat64 != NULL) {
            RELEASE_HERE(monitor->nat64, nat64);
            monitor->nat64 = NULL;
        }
    }
}

static void
nat64_ipv4_default_route_monitor_finalize(nat64_ipv4_default_route_monitor_t *monitor)
{
    free(monitor);
}

static void
nat64_infra_prefix_monitor_finalize(nat64_infra_prefix_monitor_t *monitor)
{
    free(monitor);
}

static nat64_infra_prefix_monitor_t *
nat64_infra_prefix_monitor_create(nat64_t *nat64)
{
    nat64_infra_prefix_monitor_t *monitor = calloc(1, sizeof(*monitor));
    if (monitor == NULL) {
        return monitor;
    }
    RETAIN_HERE(monitor, nat64_infra_prefix_monitor);
    monitor->nat64 = nat64;
    RETAIN_HERE(monitor->nat64, nat64);
    return monitor;
}

static void
nat64_infra_prefix_monitor_cancel(nat64_infra_prefix_monitor_t *monitor)
{
    if (monitor != NULL) {
        nat64_prefix_t *next;
        for (nat64_prefix_t *prefix = monitor->infra_nat64_prefixes; prefix != NULL; prefix = next) {
            next = prefix->next;
            prefix->next = NULL;
            RELEASE_HERE(prefix, nat64_prefix);
        }
        monitor->infra_nat64_prefixes = NULL;
        if (monitor->sdRef != NULL) {
            DNSServiceRefDeallocate(monitor->sdRef);
            monitor->sdRef = NULL;
            RELEASE_HERE(monitor, nat64_infra_prefix_monitor);
        }
        if (monitor->nat64 != NULL) {
            RELEASE_HERE(monitor->nat64, nat64);
            monitor->nat64 = NULL;
        }
        monitor->canceled = true;
    }
}

static nat64_thread_prefix_monitor_t *
nat64_thread_prefix_monitor_create(nat64_t *nat64)
{
    nat64_thread_prefix_monitor_t *monitor = calloc(1, sizeof(*monitor));
    if (monitor == NULL) {
        return monitor;
    }
    RETAIN_HERE(monitor, nat64_thread_prefix_monitor);
    monitor->nat64 = nat64;
    RETAIN_HERE(monitor->nat64, nat64);
    return monitor;
}

static void
nat64_thread_prefix_monitor_cancel(nat64_thread_prefix_monitor_t *monitor)
{
    if (monitor != NULL) {
        nat64_prefix_t *next;
        for (nat64_prefix_t *prefix = monitor->thread_nat64_prefixes; prefix != NULL; prefix = next) {
            next = prefix->next;
            prefix->next = NULL;
            RELEASE_HERE(prefix, nat64_prefix);
        }
        monitor->thread_nat64_prefixes = NULL;
        if (monitor->timer != NULL) {
            ioloop_cancel_wake_event(monitor->timer);
            ioloop_wakeup_release(monitor->timer);
            monitor->timer = NULL;
        }
        if (monitor->nat64 != NULL) {
            RELEASE_HERE(monitor->nat64, nat64);
            monitor->nat64 = NULL;
        }
    }
}

static void
nat64_thread_prefix_monitor_finalize(nat64_thread_prefix_monitor_t *monitor)
{
    free(monitor);
}

static nat64_infra_prefix_publisher_t *
nat64_infra_prefix_publisher_create(nat64_t *nat64)
{
    nat64_infra_prefix_publisher_t *publisher = calloc(1, sizeof(*publisher));
    if (publisher == NULL) {
        return publisher;
    }
    RETAIN_HERE(publisher, nat64_infra_prefix_publisher);
    publisher->nat64 = nat64;
    RETAIN_HERE(publisher->nat64, nat64);
    return publisher;
}

static void
nat64_infra_prefix_publisher_finalize(nat64_infra_prefix_publisher_t *publisher)
{
    free(publisher);
}

static void
nat64_infra_prefix_publisher_cancel(nat64_infra_prefix_publisher_t *publisher)
{
    if (publisher != NULL) {
        if (publisher->state == nat64_infra_prefix_publisher_state_publishing) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(publisher->proposed_prefix->prefix.s6_addr, nat64_prefix_buf);
            INFO("thread network shutdown, unpublishing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(publisher->proposed_prefix->prefix.s6_addr, nat64_prefix_buf));
            nat64_add_prefix_to_update_queue(publisher->nat64, publisher->proposed_prefix, nat64_prefix_action_remove);
        }
        if (publisher->proposed_prefix != NULL) {
            RELEASE_HERE(publisher->proposed_prefix, nat64_prefix);
            publisher->proposed_prefix = NULL;
        }
        if (publisher->nat64 != NULL) {
            RELEASE_HERE(publisher->nat64, nat64);
            publisher->nat64 = NULL;
        }
    }
}

static nat64_br_prefix_publisher_t *
nat64_br_prefix_publisher_create(nat64_t *nat64)
{
    nat64_br_prefix_publisher_t *publisher = calloc(1, sizeof(*publisher));
    if (publisher == NULL) {
        return publisher;
    }
    RETAIN_HERE(publisher, nat64_br_prefix_publisher);
    publisher->nat64 = nat64;
    RETAIN_HERE(publisher->nat64, nat64);
    return publisher;
}

static void
nat64_br_prefix_publisher_finalize(nat64_br_prefix_publisher_t *publisher)
{
    free(publisher);
}

static void
nat64_br_prefix_publisher_cancel(nat64_br_prefix_publisher_t *publisher)
{
    if (publisher != NULL) {
        if (publisher->state == nat64_br_prefix_publisher_state_publishing) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(publisher->br_prefix->prefix.s6_addr, nat64_prefix_buf);
            INFO("thread network shutdown, unpublishing br prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(publisher->br_prefix->prefix.s6_addr, nat64_prefix_buf));
            nat64_add_prefix_to_update_queue(publisher->nat64, publisher->br_prefix, nat64_prefix_action_remove);
        }
        if (publisher->br_prefix != NULL) {
            RELEASE_HERE(publisher->br_prefix, nat64_prefix);
            publisher->br_prefix = NULL;
        }
        if (publisher->timer != NULL) {
            ioloop_cancel_wake_event(publisher->timer);
            ioloop_wakeup_release(publisher->timer);
            publisher->timer = NULL;
        }
        if (publisher->nat64 != NULL) {
            RELEASE_HERE(publisher->nat64, nat64);
            publisher->nat64 = NULL;
        }
    }
}

static void
nat64_cancel(nat64_t *nat64)
{
    if (nat64->ipv4_monitor) {
        INFO("discontinuing nat64 ipv4 default route monitor");
        nat64_ipv4_default_route_monitor_cancel(nat64->ipv4_monitor);
        RELEASE_HERE(nat64->ipv4_monitor, nat64_ipv4_default_route_monitor);
        nat64->ipv4_monitor = NULL;
    }
    if (nat64->thread_monitor) {
        INFO("discontinuing nat64 thread monitor");
        nat64_thread_prefix_monitor_cancel(nat64->thread_monitor);
        RELEASE_HERE(nat64->thread_monitor, nat64_thread_prefix_monitor);
        nat64->thread_monitor = NULL;
    }
    if (nat64->infra_monitor) {
        INFO("discontinuing nat64 infra monitor");
        nat64_infra_prefix_monitor_cancel(nat64->infra_monitor);
        RELEASE_HERE(nat64->infra_monitor, nat64_infra_prefix_monitor);
        nat64->infra_monitor = NULL;
    }
    if (nat64->nat64_infra_prefix_publisher) {
        INFO("discontinuing nat64 infra prefix publisher");
        nat64_infra_prefix_publisher_cancel(nat64->nat64_infra_prefix_publisher);
        RELEASE_HERE(nat64->nat64_infra_prefix_publisher, nat64_infra_prefix_publisher);
        nat64->nat64_infra_prefix_publisher = NULL;
    }
    if (nat64->nat64_br_prefix_publisher) {
        INFO("discontinuing nat64 br prefix publisher");
        nat64_br_prefix_publisher_cancel(nat64->nat64_br_prefix_publisher);
        RELEASE_HERE(nat64->nat64_br_prefix_publisher, nat64_br_prefix_publisher);
        nat64->nat64_br_prefix_publisher = NULL;
    }
}

nat64_t *
nat64_create(route_state_t *route_state)
{
    nat64_t *new_nat64 = calloc(1, sizeof(*new_nat64));
    if (new_nat64 == NULL) {
        ERROR("no memory for nat64_t.");
        return NULL;
    }
    RETAIN_HERE(new_nat64, nat64);
    new_nat64->ipv4_monitor = nat64_ipv4_default_route_monitor_create(new_nat64);
    new_nat64->infra_monitor = nat64_infra_prefix_monitor_create(new_nat64);
    new_nat64->thread_monitor = nat64_thread_prefix_monitor_create(new_nat64);
    new_nat64->nat64_infra_prefix_publisher = nat64_infra_prefix_publisher_create(new_nat64);
    new_nat64->nat64_br_prefix_publisher = nat64_br_prefix_publisher_create(new_nat64);

    if (new_nat64->ipv4_monitor == NULL || new_nat64->infra_monitor == NULL ||
        new_nat64->thread_monitor == NULL || new_nat64->nat64_infra_prefix_publisher == NULL ||
        new_nat64->nat64_br_prefix_publisher == NULL) {
        ERROR("no memory for nat64 state machines.");
        nat64_cancel(new_nat64);
        return NULL;
    }
    new_nat64->route_state = route_state;

    return new_nat64;
}

nat64_prefix_t *
nat64_prefix_create(struct in6_addr *address, int prefix_length, nat64_preference pref, int rloc)
{
    nat64_prefix_t *prefix;

    prefix = calloc(1, (sizeof *prefix));
    if (prefix == NULL) {
        ERROR("no memory when create nat64 prefix");
        return NULL;
    }
    in6prefix_copy(&prefix->prefix, address, NAT64_PREFIX_SLASH_96_BYTES);
    prefix->prefix_len = prefix_length;
    prefix->priority = pref;
    prefix->rloc = rloc;
    RETAIN_HERE(prefix, nat64_prefix);
    return prefix;
}

static nat64_prefix_t *
nat64_prefix_dup(nat64_prefix_t *src)
{
    return nat64_prefix_create(&src->prefix, src->prefix_len, src->priority, src->rloc);
}

static bool
nat64_preference_has_higher_priority(const nat64_preference higher, const nat64_preference lower)
{
    // smaller value means higher priority
    if (higher < lower) {
        return true;
    } else {
        return false;
    }
}

static bool
nat64_thread_has_routable_prefix(const route_state_t * const route_state)
{
    bool have_routable_omr_prefix;
    if (route_state->omr_publisher != NULL && omr_publisher_have_routable_prefix(route_state->omr_publisher)) {
        have_routable_omr_prefix = true;
    } else {
        have_routable_omr_prefix = false;
    }
    return have_routable_omr_prefix;
}

#define NAT64_EVENT_ANNOUNCE(state_machine, event)                                       \
    do {                                                                                 \
        INFO("event " PUB_S_SRP " generated in state " PUB_S_SRP,                        \
              event.name, state_machine->state_name);                                    \
    } while (false)

#define NAT64_STATE_ANNOUNCE(state_machine, event)                                       \
    do {                                                                                 \
        if (event != NULL) {                                                             \
            INFO("event " PUB_S_SRP " received in state " PUB_S_SRP,                     \
                 event->name, state_machine->state_name);                                \
        } else {                                                                         \
            INFO("entering state " PUB_S_SRP,                                            \
            state_machine->state_name);                                                  \
        }                                                                                \
    } while (false)

#define NAT64_UNEXPECTED_EVENT(state_machine, event)                                     \
    do {                                                                                 \
        if (event != NULL) {                                                             \
            INFO("unexpected event " PUB_S_SRP " received in state " PUB_S_SRP,          \
                 event->name, state_machine->state_name);                                \
        } else {                                                                         \
            INFO("unexpected NULL event received in state " PUB_S_SRP,                   \
            state_machine->state_name);                                                  \
        }                                                                                \
    } while (false)

#define DECLARE_NAT64_STATE_GET(type, total)                                              \
static type ## _state_t *                                                                 \
type ## _state_get(type ## _state_type_t state)                                           \
{                                                                                         \
    static bool once = false;                                                             \
    if (!once) {                                                                          \
        for (unsigned i = 0; i < total ## _NUM_STATES; i++) {                             \
            if (type ## _states[i].state != (type ## _state_type_t)i) {                   \
                ERROR("type ## states %d doesn't match " PUB_S_SRP, i, type ## _states[i].name);    \
                return NULL;                                                              \
            }                                                                             \
        }                                                                                 \
        once = true;                                                                      \
    }                                                                                     \
    if (state < 0 || state > total ## _NUM_STATES) {                                      \
        return NULL;                                                                      \
    }                                                                                     \
    return & type ## _states[state];                                                      \
}

#define DECLARE_NAT64_NEXT_STATE(type)                                                     \
static void                                                                                \
type ## _next_state(type ## _t *state_machine, type ## _state_type_t state)                \
{                                                                                          \
    type ## _state_type_t next_state = state;                                              \
    do {                                                                                   \
        type ## _state_t *new_state = type ## _state_get(next_state);                      \
        if (new_state == NULL) {                                                           \
            ERROR("next state is invalid: %d", next_state);                                \
            return;                                                                        \
        }                                                                                  \
        state_machine->state = next_state;                                                 \
        state_machine->state_name = new_state->name;                                       \
        type ## _action_t action = new_state->action;                                      \
        if (action != NULL) {                                                              \
            next_state = action(state_machine, NULL);                                      \
        }                                                                                  \
    } while (next_state != type ## _state_invalid);                                        \
}

#define DECLARE_NAT64_EVENT_CONFIGURATION_GET(type, total)                                  \
static type ## _configuration_t *                                                           \
type ## _configuration_get(type ## _type_t event)                                           \
{                                                                                           \
    static bool once = false;                                                               \
    if (!once) {                                                                            \
        for (unsigned i = 0; i < total ## _NUM_EVENT_TYPES; i++) {                          \
            if (type ## _configurations[i].event_type != (type ## _type_t)i) {              \
                ERROR("type ## event %d doesn't match " PUB_S_SRP, i, type ## _configurations[i].name);   \
                return NULL;                                                                \
            }                                                                               \
        }                                                                                   \
        once = true;                                                                        \
    }                                                                                       \
    if (event < 0 || event > total ## _NUM_EVENT_TYPES) {                                   \
        return NULL;                                                                        \
    }                                                                                       \
    return & type ## _configurations[event];                                                \
}

#define NAT64_EVENT_NAME_DECL(name) { nat64_event_##name, #name }

#define DECLARE_NAT64_EVENT_INIT(type)                                                     \
static void                                                                                \
type ## _init(type ## _t *event, type ## _type_t event_type)                               \
{                                                                                          \
    memset(event, 0, sizeof(*event));                                                      \
    type ## _configuration_t *event_config = type ## _configuration_get(event_type);       \
    if (event_config == NULL) {                                                            \
        ERROR("invalid event type %d", event_type);                                        \
        return;                                                                            \
    }                                                                                      \
    event->event_type = event_type;                                                        \
    event->name = event_config->name;                                                      \
}

#define DECLARE_NAT64_EVENT_DELIVER(type)                                                                      \
static void                                                                                                    \
type ## _event_deliver(type ## _t *state_machine, type ## _event_t *event)                                     \
{                                                                                                              \
    type ## _state_t *state = type ## _state_get(state_machine->state);                                        \
    if (state == NULL) {                                                                                       \
        ERROR("event " PUB_S_SRP " received in invalid state %d", event->name, state_machine->state);          \
        return;                                                                                                \
    }                                                                                                          \
    if (state->action == NULL) {                                                                               \
        FAULT("event " PUB_S_SRP " received in state " PUB_S_SRP " with NULL action", event->name, state->name);  \
        return;                                                                                                \
    }                                                                                                          \
    type ## _state_type_t next_state = state->action(state_machine, event);                                    \
    if (next_state != type ## _state_invalid) {                                                                \
        type ## _next_state(state_machine, next_state);                                                        \
    }                                                                                                          \
}

// ipv4 default route state machine start
static void nat64_ipv4_default_route_monitor_event_init(nat64_ipv4_default_route_monitor_event_t *event, nat64_ipv4_default_route_monitor_event_type_t event_type);

typedef nat64_ipv4_default_route_monitor_state_type_t (*nat64_ipv4_default_route_monitor_action_t)(nat64_ipv4_default_route_monitor_t *NONNULL state_machine, nat64_ipv4_default_route_monitor_event_t *NULLABLE event);

typedef struct {
    nat64_ipv4_default_route_monitor_state_type_t state;
    char *name;
    nat64_ipv4_default_route_monitor_action_t action;
} nat64_ipv4_default_route_monitor_state_t;

static nat64_ipv4_default_route_monitor_state_type_t
nat64_ipv4_default_route_monitor_init_action(nat64_ipv4_default_route_monitor_t *state_machine, nat64_ipv4_default_route_monitor_event_t *UNUSED event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    state_machine->has_ipv4_default_route = false;
    return nat64_ipv4_default_route_monitor_state_wait_for_event;
}

static nat64_ipv4_default_route_monitor_state_type_t
nat64_ipv4_default_route_monitor_wait_action(nat64_ipv4_default_route_monitor_t *state_machine, nat64_ipv4_default_route_monitor_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_ipv4_default_route_monitor_state_invalid;
    } else if (event->event_type == nat64_event_ipv4_default_route_update) {
        nat64_br_prefix_publisher_event_t out_event;
        if (event->has_ipv4_connectivity == false) {
            state_machine->has_ipv4_default_route = false;
            nat64_br_prefix_publisher_event_init(&out_event, nat64_event_nat64_br_prefix_publisher_ipv4_default_route_went_away);
        } else {
            state_machine->has_ipv4_default_route = true;
            nat64_br_prefix_publisher_event_init(&out_event, nat64_event_nat64_br_prefix_publisher_ipv4_default_route_showed_up);
        }
        NAT64_EVENT_ANNOUNCE(state_machine, out_event);
        // Deliver out_event to BR prefix publisher
        nat64_br_prefix_publisher_event_deliver(state_machine->nat64->nat64_br_prefix_publisher, &out_event);
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_ipv4_default_route_monitor_state_invalid;
}

#define IPV4_STATE_NAME_DECL(name) nat64_ipv4_default_route_monitor_state_##name, #name
static nat64_ipv4_default_route_monitor_state_t
nat64_ipv4_default_route_monitor_states[] = {
    { IPV4_STATE_NAME_DECL(invalid),                  NULL },
    { IPV4_STATE_NAME_DECL(init),                     nat64_ipv4_default_route_monitor_init_action },
    { IPV4_STATE_NAME_DECL(wait_for_event),           nat64_ipv4_default_route_monitor_wait_action },
};
#define IPV4_DEFAULT_ROUTE_MONITOR_NUM_STATES (sizeof(nat64_ipv4_default_route_monitor_states) / sizeof(nat64_ipv4_default_route_monitor_state_t))

DECLARE_NAT64_STATE_GET(nat64_ipv4_default_route_monitor, IPV4_DEFAULT_ROUTE_MONITOR);
DECLARE_NAT64_NEXT_STATE(nat64_ipv4_default_route_monitor);

// ipv4 default route monitor event functions
typedef struct {
    nat64_ipv4_default_route_monitor_event_type_t event_type;
    char *name;
} nat64_ipv4_default_route_monitor_event_configuration_t;

nat64_ipv4_default_route_monitor_event_configuration_t nat64_ipv4_default_route_monitor_event_configurations[] = {
    NAT64_EVENT_NAME_DECL(ipv4_default_route_invalid),
    NAT64_EVENT_NAME_DECL(ipv4_default_route_update),
    NAT64_EVENT_NAME_DECL(ipv4_default_route_showed_up),
    NAT64_EVENT_NAME_DECL(ipv4_default_route_went_away),
};
#define IPV4_DEFAULT_ROUTE_MONITOR_NUM_EVENT_TYPES (sizeof(nat64_ipv4_default_route_monitor_event_configurations) / sizeof(nat64_ipv4_default_route_monitor_event_configuration_t))

DECLARE_NAT64_EVENT_CONFIGURATION_GET(nat64_ipv4_default_route_monitor_event, IPV4_DEFAULT_ROUTE_MONITOR);
DECLARE_NAT64_EVENT_INIT(nat64_ipv4_default_route_monitor_event);
DECLARE_NAT64_EVENT_DELIVER(nat64_ipv4_default_route_monitor);

void
nat64_default_route_update(nat64_t *NONNULL nat64, bool has_ipv4_connectivity)
{
    if (has_ipv4_connectivity != nat64->ipv4_monitor->has_ipv4_default_route){
        nat64_ipv4_default_route_monitor_event_t event;
        nat64_ipv4_default_route_monitor_event_init(&event, nat64_event_ipv4_default_route_update);
        event.has_ipv4_connectivity = has_ipv4_connectivity;
        nat64_ipv4_default_route_monitor_event_deliver(nat64->ipv4_monitor, &event);
    }
}
// ipv4 default route state machine end

// Infrastructure nat64 prefix monitor state machine start
static void nat64_infra_prefix_monitor_event_init(nat64_infra_prefix_monitor_event_t *event, nat64_infra_prefix_monitor_event_type_t event_type);
static void nat64_infra_prefix_monitor_event_deliver(nat64_infra_prefix_monitor_t *state_machine, nat64_infra_prefix_monitor_event_t *event);
typedef nat64_infra_prefix_monitor_state_type_t (*nat64_infra_prefix_monitor_action_t)(nat64_infra_prefix_monitor_t *NONNULL sm, nat64_infra_prefix_monitor_event_t *NULLABLE event);

typedef struct {
    nat64_infra_prefix_monitor_state_type_t state;
    char *name;
    nat64_infra_prefix_monitor_action_t action;
} nat64_infra_prefix_monitor_state_t;

static void
nat64_query_infra_callback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                           DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                           uint16_t rdlen, const void *rdata, uint32_t ttl, void *context)
{
    (void)(sdRef);
    (void)(interfaceIndex);

    nat64_infra_prefix_monitor_t *state_machine = context;

    if (errorCode == kDNSServiceErr_NoError) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(rdata, ipv6_rdata_buf);
        INFO("LLQ " PRI_S_SRP PRI_SEGMENTED_IPv6_ADDR_SRP
             "name: " PRI_S_SRP ", rrtype: %u, rrclass: %u, rdlen: %u, ttl: %u.",
             (flags & kDNSServiceFlagsAdd) ? "adding " : "removing ",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(rdata, ipv6_rdata_buf), fullname, rrtype, rrclass, rdlen, ttl);
        nat64_infra_prefix_monitor_event_t event;
        nat64_infra_prefix_monitor_event_init(&event, nat64_event_infra_prefix_update);
        event.flags = flags;
        event.rdata = rdata;
        NAT64_EVENT_ANNOUNCE(state_machine, event);
        nat64_infra_prefix_monitor_event_deliver(state_machine, &event);
    } else {
        if (errorCode == kDNSServiceErr_NoSuchRecord) {
            // This should never happen.
            INFO("No such record for " PRI_S_SRP , NAT64_PREFIX_LLQ_QUERY_DOMAIN);
        } else if (errorCode == kDNSServiceErr_ServiceNotRunning) {
            INFO("daemon disconnected (probably daemon crash).");
        } else {
            INFO("Got error code %d when query " PRI_S_SRP , errorCode, NAT64_PREFIX_LLQ_QUERY_DOMAIN);
        }
        DNSServiceRefDeallocate(state_machine->sdRef);
        state_machine->sdRef = NULL;

        // We enter with a reference held on the state machine object. If there is no error, that means we got some kind
        // of result, and so we don't release the reference because we can still get more results.  If, on the other hand,
        // we get an error, we will restart the query after a delay. This means that the reference we were passed is
        // still needed for the duration of the dispatch_after call. When that timer expires, if the state machine hasn't
        // been canceled in the meantime, we restart the query.
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC),
                       dispatch_get_main_queue(), ^(void) {
                           if (!state_machine->canceled) {
                               nat64_query_prefix_on_infra(state_machine);
                           }
                           RELEASE_HERE(state_machine, nat64_infra_prefix_monitor);
                       });
    }
}

static bool
nat64_query_prefix_on_infra(nat64_infra_prefix_monitor_t *state_machine)
{
    OSStatus err;

    err = DNSServiceQueryRecord(&state_machine->sdRef, kDNSServiceFlagsLongLivedQuery, kDNSServiceInterfaceIndexAny, NAT64_PREFIX_LLQ_QUERY_DOMAIN, kDNSServiceType_AAAA, kDNSServiceClass_IN, nat64_query_infra_callback, state_machine);
    if (err != kDNSServiceErr_NoError) {
        ERROR("DNSServiceQueryRecord failed for " PRI_S_SRP ": %d", NAT64_PREFIX_LLQ_QUERY_DOMAIN, (int)err);
        return false;
    }
    RETAIN_HERE(state_machine, nat64_infra_prefix_monitor); // For the callback.
    err = DNSServiceSetDispatchQueue(state_machine->sdRef, dispatch_get_main_queue());
    if (err != kDNSServiceErr_NoError) {
        ERROR("DNSServiceSetDispatchQueue failed for " PRI_S_SRP ": %d", NAT64_PREFIX_LLQ_QUERY_DOMAIN, (int)err);
        return false;
    }
    return true;
}

static nat64_infra_prefix_monitor_state_type_t
nat64_infra_prefix_monitor_init_action(nat64_infra_prefix_monitor_t *state_machine, nat64_infra_prefix_monitor_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Init action: start LLQ.
    if (!nat64_query_prefix_on_infra(state_machine)) {
         return nat64_infra_prefix_monitor_state_invalid;
    }
    // Switch to next state.
    return nat64_infra_prefix_monitor_state_wait_for_change;
}

static nat64_infra_prefix_monitor_state_type_t
nat64_infra_prefix_monitor_wait_action(nat64_infra_prefix_monitor_t *state_machine, nat64_infra_prefix_monitor_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_infra_prefix_monitor_state_invalid;
    } else if (event->event_type == nat64_event_infra_prefix_update){
        bool changed = false;
        if (event->flags & kDNSServiceFlagsAdd) {
            nat64_prefix_t **ppref = &state_machine->infra_nat64_prefixes, *prefix = NULL;
            while (*ppref != NULL) {
                prefix = *ppref;
                if (!memcmp(&prefix->prefix, event->rdata, NAT64_PREFIX_SLASH_96_BYTES)) {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, nat64_prefix_buf);
                    INFO("ignore dup infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, nat64_prefix_buf));
                    break;
                } else {
                    ppref = &prefix->next;
                }
            }
            if (*ppref == NULL) {
                nat64_prefix_t * new_prefix = nat64_prefix_create((struct in6_addr *)event->rdata, NAT64_PREFIX_SLASH_96_BYTES, nat64_preference_medium, state_machine->nat64->route_state->srp_server->rloc16);
                if (new_prefix == NULL) {
                    ERROR("no memory for nat64 prefix.");
                    return nat64_infra_prefix_monitor_state_invalid;
                }
                SEGMENTED_IPv6_ADDR_GEN_SRP(new_prefix->prefix.s6_addr, nat64_prefix_buf);
                INFO("adding infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " to list",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(new_prefix->prefix.s6_addr, nat64_prefix_buf));
                new_prefix->next = state_machine->infra_nat64_prefixes;
                state_machine->infra_nat64_prefixes = new_prefix;
                changed = true;
            }
        } else {
            nat64_prefix_t **ppref = &state_machine->infra_nat64_prefixes, *prefix = NULL;
            while (*ppref != NULL) {
                prefix = *ppref;
                if (!memcmp(&prefix->prefix, event->rdata, NAT64_PREFIX_SLASH_96_BYTES)) {
                    *ppref = prefix->next;
                    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, nat64_prefix_buf);
                    INFO("removing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " from list",
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, nat64_prefix_buf));
                    RELEASE_HERE(prefix, nat64_prefix);
                    changed = true;
                } else {
                    ppref = &prefix->next;
                }
            }
        }
        if (changed){
            return nat64_infra_prefix_monitor_state_change_occurred;
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_infra_prefix_monitor_state_invalid;
}

static nat64_infra_prefix_monitor_state_type_t
nat64_infra_prefix_monitor_change_occurred_action(nat64_infra_prefix_monitor_t *state_machine, nat64_infra_prefix_monitor_event_t * event)
{
    nat64_infra_prefix_publisher_event_t out_event_to_nat64_infra_prefix_publisher;
    nat64_br_prefix_publisher_event_t out_event_to_nat64_br_prefix_publisher;

    NAT64_STATE_ANNOUNCE(state_machine, event);
    nat64_infra_prefix_publisher_event_init(&out_event_to_nat64_infra_prefix_publisher, nat64_event_nat64_infra_prefix_publisher_infra_prefix_changed);
    out_event_to_nat64_infra_prefix_publisher.prefix = state_machine->infra_nat64_prefixes;
    NAT64_EVENT_ANNOUNCE(state_machine, out_event_to_nat64_infra_prefix_publisher);
    // Deliver this event to infra prefix publisher.
    nat64_infra_prefix_publisher_event_deliver(state_machine->nat64->nat64_infra_prefix_publisher, &out_event_to_nat64_infra_prefix_publisher);

    nat64_br_prefix_publisher_event_init(&out_event_to_nat64_br_prefix_publisher, nat64_event_nat64_br_prefix_publisher_infra_prefix_changed);
    out_event_to_nat64_br_prefix_publisher.prefix = state_machine->infra_nat64_prefixes;
    NAT64_EVENT_ANNOUNCE(state_machine, out_event_to_nat64_br_prefix_publisher);
    // Deliver this event to BR prefix publisher.
    nat64_br_prefix_publisher_event_deliver(state_machine->nat64->nat64_br_prefix_publisher, &out_event_to_nat64_br_prefix_publisher);

    return nat64_infra_prefix_monitor_state_wait_for_change;
}

#define INFRA_STATE_NAME_DECL(name) nat64_infra_prefix_monitor_state_##name, #name
static nat64_infra_prefix_monitor_state_t
nat64_infra_prefix_monitor_states[] = {
    { INFRA_STATE_NAME_DECL(invalid),                     NULL },
    { INFRA_STATE_NAME_DECL(init),                        nat64_infra_prefix_monitor_init_action },
    { INFRA_STATE_NAME_DECL(wait_for_change),             nat64_infra_prefix_monitor_wait_action },
    { INFRA_STATE_NAME_DECL(change_occurred),             nat64_infra_prefix_monitor_change_occurred_action },
};
#define INFRA_PREFIX_MONITOR_NUM_STATES (sizeof(nat64_infra_prefix_monitor_states) / sizeof(nat64_infra_prefix_monitor_state_t))

DECLARE_NAT64_STATE_GET(nat64_infra_prefix_monitor, INFRA_PREFIX_MONITOR);
DECLARE_NAT64_NEXT_STATE(nat64_infra_prefix_monitor);

// Infra prefix monitor event functions
typedef struct {
    nat64_infra_prefix_monitor_event_type_t event_type;
    char *name;
} nat64_infra_prefix_monitor_event_configuration_t;

nat64_infra_prefix_monitor_event_configuration_t nat64_infra_prefix_monitor_event_configurations[] = {
    NAT64_EVENT_NAME_DECL(infra_prefix_invalid),
    NAT64_EVENT_NAME_DECL(infra_prefix_update),
};
#define INFRA_PREFIX_MONITOR_NUM_EVENT_TYPES (sizeof(nat64_infra_prefix_monitor_event_configurations) / sizeof(nat64_infra_prefix_monitor_event_configuration_t))

DECLARE_NAT64_EVENT_CONFIGURATION_GET(nat64_infra_prefix_monitor_event, INFRA_PREFIX_MONITOR);
DECLARE_NAT64_EVENT_INIT(nat64_infra_prefix_monitor_event);
DECLARE_NAT64_EVENT_DELIVER(nat64_infra_prefix_monitor);

// Infrastructure nat64 prefix monitor state machine end



// Thread nat64 prefix monitor state machine start
static void nat64_thread_prefix_monitor_event_init(nat64_thread_prefix_monitor_event_t *event, nat64_thread_prefix_monitor_event_type_t event_type);
static void nat64_thread_prefix_monitor_event_deliver(nat64_thread_prefix_monitor_t *state_machine, nat64_thread_prefix_monitor_event_t *event);
typedef nat64_thread_prefix_monitor_state_type_t (*nat64_thread_prefix_monitor_action_t)(nat64_thread_prefix_monitor_t *NONNULL sm, nat64_thread_prefix_monitor_event_t *NULLABLE event);

typedef struct {
    nat64_thread_prefix_monitor_state_type_t state;
    char *name;
    nat64_thread_prefix_monitor_action_t action;
} nat64_thread_prefix_monitor_state_t;

static void
nat64_thread_prefix_monitor_context_release(void *context)
{
    nat64_thread_prefix_monitor_t *state_machine = context;
    RELEASE_HERE(state_machine, nat64_thread_prefix_monitor);
}

static void
nat64_thread_prefix_monitor_wakeup(void *context)
{
    nat64_thread_prefix_monitor_t *state_machine = context;
    nat64_thread_prefix_monitor_event_t out_event;
    nat64_thread_prefix_monitor_event_init(&out_event, nat64_event_thread_prefix_init_wait_ended);
    NAT64_EVENT_ANNOUNCE(state_machine, out_event);
    nat64_thread_prefix_monitor_event_deliver(state_machine, &out_event);
}

static nat64_thread_prefix_monitor_state_type_t
nat64_thread_prefix_monitor_init_action(nat64_thread_prefix_monitor_t *state_machine, nat64_thread_prefix_monitor_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Init action: start timer.
    if (state_machine->timer == NULL) {
        state_machine->timer = ioloop_wakeup_create();
        if (state_machine->timer == NULL) {
            ERROR("no memory when create timer");
            return nat64_thread_prefix_monitor_state_invalid;
        }
        RETAIN_HERE(state_machine, nat64_thread_prefix_monitor);
        // wait rand(0,10) seconds
        ioloop_add_wake_event(state_machine->timer, state_machine, nat64_thread_prefix_monitor_wakeup, nat64_thread_prefix_monitor_context_release, srp_random16() % (NAT64_THREAD_PREFIX_SETTLING_TIME * IOLOOP_SECOND));
    } else {
        INFO("thread prefix monitor timer already started");
    }
    // Switch to next state.
    return nat64_thread_prefix_monitor_state_wait_for_settling;
}

static nat64_thread_prefix_monitor_state_type_t
nat64_thread_prefix_monitor_wait_for_settling_action(nat64_thread_prefix_monitor_t *state_machine, nat64_thread_prefix_monitor_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_thread_prefix_monitor_state_invalid;
    } else if (event->event_type == nat64_event_thread_prefix_init_wait_ended){
        // Switch to next state.
        return nat64_thread_prefix_monitor_state_wait_for_change;
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
        return nat64_thread_prefix_monitor_state_invalid;
    }
}

static nat64_preference
route_pref_to_nat64_pref(offmesh_route_preference_t route_pref)
{
    if (route_pref == offmesh_route_preference_low) {
        return nat64_preference_low;
    } else if (route_pref == offmesh_route_preference_high) {
        return nat64_preference_high;
    } else if (route_pref == offmesh_route_preference_medium) {
        return nat64_preference_medium;
    } else {
        ERROR("Unknown route prefix preference %d", route_pref);
        return nat64_preference_reserved;
    }
}

static char *
get_nat64_prefix_pref_name(nat64_preference pref)
{
    if (pref == nat64_preference_low) {
        return "low";
    } else if (pref == nat64_preference_high) {
        return "high";
    } else if (pref == nat64_preference_medium) {
        return "medium";
    } else {
        ERROR("Unknown nat64 prefix preference %d", pref);
        return "unknown";
    }
}

static nat64_thread_prefix_monitor_state_type_t
nat64_thread_prefix_monitor_wait_for_change_action(nat64_thread_prefix_monitor_t *state_machine, nat64_thread_prefix_monitor_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_thread_prefix_monitor_state_invalid;
    } else if (event->event_type == nat64_event_thread_prefix_update){
        size_t i;
        nat64_prefix_t **ppref = &state_machine->thread_nat64_prefixes, *prefix = NULL;
        bool changed = false;
        // Delete any NAT64 prefixes that are not in the list provided by Thread.
        while (*ppref != NULL) {
            prefix = *ppref;
            for (i = 0; i < event->routes->num; i++) {
                cti_route_t *route = event->routes->routes[i];
                if (route->nat64 && route->origin == offmesh_route_origin_ncp){
                    if (!in6prefix_compare(&prefix->prefix, &route->prefix, NAT64_PREFIX_SLASH_96_BYTES)) {
                        break;
                    }
                }
            }
            if (i == event->routes->num) {
                *ppref = prefix->next;
                SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, nat64_prefix_buf);
                INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " with pref " PRI_S_SRP " went away",
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, nat64_prefix_buf),
                     get_nat64_prefix_pref_name(prefix->priority));
                RELEASE_HERE(prefix, nat64_prefix);
                changed = true;
            } else {
                ppref = &prefix->next;
            }
        }
        // Add any NAT64 prefixes that are not present.
        for (i = 0; i < event->routes->num; i++) {
            cti_route_t *route = event->routes->routes[i];
            if (route->nat64 && route->origin == offmesh_route_origin_ncp) {
                for(prefix = state_machine->thread_nat64_prefixes; prefix != NULL; prefix = prefix->next){
                    if (!in6prefix_compare(&prefix->prefix, &route->prefix, NAT64_PREFIX_SLASH_96_BYTES)) {
                        break;
                    }
                }
                if (prefix == NULL) {
                    prefix = nat64_prefix_create(&route->prefix, NAT64_PREFIX_SLASH_96_BYTES, route_pref_to_nat64_pref(route->preference), route->rloc);
                    if (prefix == NULL) {
                        ERROR("no memory for nat64 prefix.");
                        return nat64_thread_prefix_monitor_state_invalid;
                    } else {
                        SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, nat64_prefix_buf);
                        INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " with pref " PRI_S_SRP " showed up",
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, nat64_prefix_buf),
                             get_nat64_prefix_pref_name(prefix->priority));
                        *ppref = prefix;
                        ppref = &prefix->next;
                        changed = true;
                    }
                }
            }
        }
        if (changed) {
        // Switch to next state.
            return nat64_thread_prefix_monitor_state_change_occurred;
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_thread_prefix_monitor_state_invalid;
}

static nat64_thread_prefix_monitor_state_type_t
nat64_thread_prefix_monitor_change_occurred_action(nat64_thread_prefix_monitor_t *state_machine, nat64_thread_prefix_monitor_event_t * event)
{
    nat64_infra_prefix_publisher_event_t out_event_to_nat64_infra_prefix_publisher;
    nat64_br_prefix_publisher_event_t out_event_to_nat64_br_prefix_publisher;

    NAT64_STATE_ANNOUNCE(state_machine, event);
    nat64_infra_prefix_publisher_event_init(&out_event_to_nat64_infra_prefix_publisher, nat64_event_nat64_infra_prefix_publisher_thread_prefix_changed);
    out_event_to_nat64_infra_prefix_publisher.prefix = state_machine->thread_nat64_prefixes;
    NAT64_EVENT_ANNOUNCE(state_machine, out_event_to_nat64_infra_prefix_publisher);
    // Deliver this event to infra prefix publisher.
    nat64_infra_prefix_publisher_event_deliver(state_machine->nat64->nat64_infra_prefix_publisher, &out_event_to_nat64_infra_prefix_publisher);

    nat64_br_prefix_publisher_event_init(&out_event_to_nat64_br_prefix_publisher, nat64_event_nat64_br_prefix_publisher_thread_prefix_changed);
    out_event_to_nat64_br_prefix_publisher.prefix = state_machine->thread_nat64_prefixes;
    NAT64_EVENT_ANNOUNCE(state_machine, out_event_to_nat64_br_prefix_publisher);
    // Deliver this event to BR prefix publisher.
    nat64_br_prefix_publisher_event_deliver(state_machine->nat64->nat64_br_prefix_publisher, &out_event_to_nat64_br_prefix_publisher);

    return nat64_thread_prefix_monitor_state_wait_for_change;
}

#define THREAD_STATE_NAME_DECL(name) nat64_thread_prefix_monitor_state_##name, #name
static nat64_thread_prefix_monitor_state_t
nat64_thread_prefix_monitor_states[] = {
    { THREAD_STATE_NAME_DECL(invalid),                     NULL },
    { THREAD_STATE_NAME_DECL(init),                        nat64_thread_prefix_monitor_init_action },
    { THREAD_STATE_NAME_DECL(wait_for_settling),           nat64_thread_prefix_monitor_wait_for_settling_action },
    { THREAD_STATE_NAME_DECL(wait_for_change),             nat64_thread_prefix_monitor_wait_for_change_action },
    { THREAD_STATE_NAME_DECL(change_occurred),             nat64_thread_prefix_monitor_change_occurred_action },
};
#define THREAD_PREFIX_MONITOR_NUM_STATES (sizeof(nat64_thread_prefix_monitor_states) / sizeof(nat64_thread_prefix_monitor_state_t))

DECLARE_NAT64_STATE_GET(nat64_thread_prefix_monitor, THREAD_PREFIX_MONITOR);
DECLARE_NAT64_NEXT_STATE(nat64_thread_prefix_monitor);

// Thread prefix monitor event functions
typedef struct {
    nat64_thread_prefix_monitor_event_type_t event_type;
    char *name;
} nat64_thread_prefix_monitor_event_configuration_t;

nat64_thread_prefix_monitor_event_configuration_t nat64_thread_prefix_monitor_event_configurations[] = {
    NAT64_EVENT_NAME_DECL(thread_prefix_invalid),
    NAT64_EVENT_NAME_DECL(thread_prefix_init_wait_ended),
    NAT64_EVENT_NAME_DECL(thread_prefix_update),
};
#define THREAD_PREFIX_MONITOR_NUM_EVENT_TYPES (sizeof(nat64_thread_prefix_monitor_event_configurations) / sizeof(nat64_thread_prefix_monitor_event_configuration_t))

DECLARE_NAT64_EVENT_CONFIGURATION_GET(nat64_thread_prefix_monitor_event, THREAD_PREFIX_MONITOR);
DECLARE_NAT64_EVENT_INIT(nat64_thread_prefix_monitor_event);
DECLARE_NAT64_EVENT_DELIVER(nat64_thread_prefix_monitor);

// Thread nat64 prefix monitor state machine end


// Infrastructure nat64 prefix publisher state machine start
typedef nat64_infra_prefix_publisher_state_type_t (*nat64_infra_prefix_publisher_action_t)(nat64_infra_prefix_publisher_t *NONNULL sm, nat64_infra_prefix_publisher_event_t *NULLABLE event);

typedef struct {
    nat64_infra_prefix_publisher_state_type_t state;
    char *name;
    nat64_infra_prefix_publisher_action_t action;
} nat64_infra_prefix_publisher_state_t;

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_init_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Init action

    // Switch to next state.
    return nat64_infra_prefix_publisher_state_wait;
}

static int
nat64_num_infra_prefix(const nat64_prefix_t *prefix)
{
    int num_infra_prefix = 0;

    for (; prefix != NULL; prefix = prefix->next) {
        if (nat64_preference_has_higher_priority(prefix->priority, nat64_preference_low)) {
            num_infra_prefix++;
        }
    }
    INFO("%d infra nat64 prefixes", num_infra_prefix);
    return num_infra_prefix;
}

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_wait_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_infra_prefix_publisher_state_invalid;
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_thread_prefix_changed) {
        if (nat64_num_infra_prefix(event->prefix) >= NAT64_INFRA_PREFIX_LIMIT) {
            INFO("more than %d infra nat64 prefix present on thread network, ignore", NAT64_INFRA_PREFIX_LIMIT);
            return nat64_infra_prefix_publisher_state_ignore;
        } else {
            return nat64_infra_prefix_publisher_state_check;
        }
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_infra_prefix_changed) {
        // Check to see if it's appropriate to publish infra nat64 prefix
        if (event->prefix) {
            return nat64_infra_prefix_publisher_state_check;
        }
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_showed_up) {
        // Routable OMR prefix showed up, check to see if we should publish infra nat64 prefix
        return nat64_infra_prefix_publisher_state_check;
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_infra_prefix_publisher_state_invalid;
}

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_ignore_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_infra_prefix_publisher_state_invalid;
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_thread_prefix_changed) {
        if (nat64_num_infra_prefix(event->prefix) >= NAT64_INFRA_PREFIX_LIMIT) {
            INFO("more than %d infra nat64 prefixes present, ignore", NAT64_INFRA_PREFIX_LIMIT);
            return nat64_infra_prefix_publisher_state_invalid;
        } else {
            return nat64_infra_prefix_publisher_state_check;
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_infra_prefix_publisher_state_invalid;
}

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_check_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Go to publish state when all of the following conditions are met:
    // 1. We have infra prefix
    // 2. We have routable OMR prefix
    // 3. The number of infra prefixes on thread network is less than NAT64_INFRA_PREFIX_LIMIT
    nat64_prefix_t *infra_prefix = state_machine->nat64->infra_monitor->infra_nat64_prefixes;
    if (infra_prefix && nat64_thread_has_routable_prefix(state_machine->nat64->route_state)
        && nat64_num_infra_prefix(state_machine->nat64->thread_monitor->thread_nat64_prefixes) < NAT64_INFRA_PREFIX_LIMIT) {
        state_machine->proposed_prefix = nat64_prefix_dup(infra_prefix);
        if (state_machine->proposed_prefix == NULL) {
            return nat64_infra_prefix_publisher_state_wait;
        }
        return nat64_infra_prefix_publisher_state_publish;
    } else {
        return nat64_infra_prefix_publisher_state_wait;
    }
}

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_publish_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf);
    INFO("publishing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
          SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf));
    nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->proposed_prefix,
                                     nat64_prefix_action_add);
    return nat64_infra_prefix_publisher_state_publishing;
}

static void
nat64_remove_prefix_from_thread_monitor(nat64_t *nat64, nat64_prefix_t *deprecated_prefix)
{
    nat64_prefix_t **ppref = &nat64->thread_monitor->thread_nat64_prefixes, *prefix = NULL;
    while (*ppref != NULL) {
        prefix = *ppref;
        if (!in6prefix_compare(&prefix->prefix, &deprecated_prefix->prefix, NAT64_PREFIX_SLASH_96_BYTES)
            && prefix->rloc == deprecated_prefix->rloc)
        {
            *ppref = prefix->next;
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, nat64_prefix_buf);
            INFO("prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " with pref " PRI_S_SRP " went away",
                  SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, nat64_prefix_buf),
                  get_nat64_prefix_pref_name(prefix->priority));
            RELEASE_HERE(prefix, nat64_prefix);
        } else {
            ppref = &prefix->next;
        }
    }
}

static nat64_infra_prefix_publisher_state_type_t
nat64_infra_prefix_publisher_publishing_action(nat64_infra_prefix_publisher_t *state_machine, nat64_infra_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_infra_prefix_publisher_state_invalid;
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_infra_prefix_changed ||
               event->event_type == nat64_event_nat64_infra_prefix_publisher_shutdown)
    {
        nat64_prefix_t *infra_prefix;
        for (infra_prefix = event->prefix; infra_prefix; infra_prefix = infra_prefix->next) {
            if (!in6prefix_compare(&infra_prefix->prefix, &state_machine->proposed_prefix->prefix, NAT64_PREFIX_SLASH_96_BYTES)) {
                // The proposed prefix is still there, do nothing
                return nat64_infra_prefix_publisher_state_invalid;
            }
        }
        // The proposed infra prefix is gone
        SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf);
        INFO("The proposed infra prefix is gone, unpublishing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf));
        nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->proposed_prefix, nat64_prefix_action_remove);
        // Remove it from thread_monitor prefix database
        nat64_remove_prefix_from_thread_monitor(state_machine->nat64, state_machine->proposed_prefix);
        RELEASE_HERE(state_machine->proposed_prefix, nat64_prefix);
        state_machine->proposed_prefix = NULL;
        // Is there a different infra NAT64 prefix?
        if (event->prefix) {
            state_machine->proposed_prefix = nat64_prefix_dup(event->prefix);
            if (state_machine->proposed_prefix == NULL) {
                return nat64_infra_prefix_publisher_state_check;
            }
            return nat64_infra_prefix_publisher_state_publish;
        } else {
            INFO("no longer publishing infra prefix.");
            return nat64_infra_prefix_publisher_state_wait;
        }
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_went_away ||
               event->event_type == nat64_event_nat64_infra_prefix_publisher_shutdown)
    {
        // Routable OMR prefix is gone
        SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf);
        INFO("Routable OMR prefix is gone, unpublishing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf));
        nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->proposed_prefix, nat64_prefix_action_remove);
        // Remove it from thread_monitor prefix database
        nat64_remove_prefix_from_thread_monitor(state_machine->nat64, state_machine->proposed_prefix);
        RELEASE_HERE(state_machine->proposed_prefix, nat64_prefix);
        state_machine->proposed_prefix = NULL;
        INFO("no longer publishing infra prefix.");
        return nat64_infra_prefix_publisher_state_wait;
    } else if (event->event_type == nat64_event_nat64_infra_prefix_publisher_thread_prefix_changed) {
        nat64_prefix_t *thread_prefix;
        int num_infra_prefix = 0;
        for (thread_prefix = event->prefix; thread_prefix; thread_prefix = thread_prefix->next) {
            if (nat64_preference_has_higher_priority(thread_prefix->priority, nat64_preference_low)) {
                num_infra_prefix++;
            }
        }
        INFO("%d infra nat64 prefixes present on thread network", num_infra_prefix);
        // If more than 3 infra prefixes show on thread network, BR with highest rloc should withdraw
        if (num_infra_prefix > NAT64_INFRA_PREFIX_LIMIT) {
            int max_rloc = event->prefix->rloc;
            for (thread_prefix = event->prefix; thread_prefix; thread_prefix = thread_prefix->next) {
                if (thread_prefix->rloc > max_rloc) {
                    max_rloc = thread_prefix->rloc;
                }
            }
            INFO("%d infra nat64 prefixes present on thread network with max_rloc[%d]", num_infra_prefix, max_rloc);
            if (max_rloc == state_machine->nat64->route_state->srp_server->rloc16) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf);
                INFO("BR has highest rloc, unpublishing infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->proposed_prefix->prefix.s6_addr, nat64_prefix_buf));
                nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->proposed_prefix, nat64_prefix_action_remove);
                // Remove it from thread_monitor prefix database
                nat64_remove_prefix_from_thread_monitor(state_machine->nat64, state_machine->proposed_prefix);
                RELEASE_HERE(state_machine->proposed_prefix, nat64_prefix);
                state_machine->proposed_prefix = NULL;
                INFO("no longer publishing infra prefix.");
                return nat64_infra_prefix_publisher_state_wait;
            }
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_infra_prefix_publisher_state_invalid;
}

#define INFRA_PUBLISHER_STATE_NAME_DECL(name) nat64_infra_prefix_publisher_state_##name, #name
static nat64_infra_prefix_publisher_state_t
nat64_infra_prefix_publisher_states[] = {
    { INFRA_PUBLISHER_STATE_NAME_DECL(invalid),            NULL },
    { INFRA_PUBLISHER_STATE_NAME_DECL(init),               nat64_infra_prefix_publisher_init_action },
    { INFRA_PUBLISHER_STATE_NAME_DECL(wait),               nat64_infra_prefix_publisher_wait_action },
    { INFRA_PUBLISHER_STATE_NAME_DECL(ignore),             nat64_infra_prefix_publisher_ignore_action },
    { INFRA_PUBLISHER_STATE_NAME_DECL(check),              nat64_infra_prefix_publisher_check_action },
    { INFRA_PUBLISHER_STATE_NAME_DECL(publish),            nat64_infra_prefix_publisher_publish_action },
    { INFRA_PUBLISHER_STATE_NAME_DECL(publishing),         nat64_infra_prefix_publisher_publishing_action },
};
#define INFRA_PREFIX_PUBLISHER_NUM_STATES (sizeof(nat64_infra_prefix_publisher_states) / sizeof(nat64_infra_prefix_publisher_state_t))

DECLARE_NAT64_STATE_GET(nat64_infra_prefix_publisher, INFRA_PREFIX_PUBLISHER);
DECLARE_NAT64_NEXT_STATE(nat64_infra_prefix_publisher);

// Infra prefix publisher event functions
typedef struct {
    nat64_infra_prefix_publisher_event_type_t event_type;
    char *name;
} nat64_infra_prefix_publisher_event_configuration_t;

nat64_infra_prefix_publisher_event_configuration_t nat64_infra_prefix_publisher_event_configurations[] = {
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_invalid),
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_thread_prefix_changed),
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_infra_prefix_changed),
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_routable_omr_prefix_went_away),
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_routable_omr_prefix_showed_up),
    NAT64_EVENT_NAME_DECL(nat64_infra_prefix_publisher_shutdown),
};
#define INFRA_PREFIX_PUBLISHER_NUM_EVENT_TYPES (sizeof(nat64_infra_prefix_publisher_event_configurations) / sizeof(nat64_infra_prefix_publisher_event_configuration_t))

DECLARE_NAT64_EVENT_CONFIGURATION_GET(nat64_infra_prefix_publisher_event, INFRA_PREFIX_PUBLISHER);
DECLARE_NAT64_EVENT_INIT(nat64_infra_prefix_publisher_event);
DECLARE_NAT64_EVENT_DELIVER(nat64_infra_prefix_publisher);

void
nat64_omr_route_update(nat64_t *NONNULL nat64, bool has_routable_omr_prefix)
{
    if (!has_routable_omr_prefix && nat64->nat64_infra_prefix_publisher->routable_omr_prefix_present){
        nat64_infra_prefix_publisher_event_t event;

        nat64->nat64_infra_prefix_publisher->routable_omr_prefix_present = false;
        nat64_infra_prefix_publisher_event_init(&event, nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_went_away);
        NAT64_EVENT_ANNOUNCE(nat64->nat64_infra_prefix_publisher, event);
        nat64_infra_prefix_publisher_event_deliver(nat64->nat64_infra_prefix_publisher, &event);
    } else if (has_routable_omr_prefix && !nat64->nat64_infra_prefix_publisher->routable_omr_prefix_present){
        nat64_infra_prefix_publisher_event_t event;

        nat64->nat64_infra_prefix_publisher->routable_omr_prefix_present = true;
        nat64_infra_prefix_publisher_event_init(&event, nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_showed_up);
        NAT64_EVENT_ANNOUNCE(nat64->nat64_infra_prefix_publisher, event);
        nat64_infra_prefix_publisher_event_deliver(nat64->nat64_infra_prefix_publisher, &event);
    }
}
// Infrastructure nat64 prefix publisher state machine end


// BR nat64 prefix publisher state machine start
typedef nat64_br_prefix_publisher_state_type_t (*nat64_br_prefix_publisher_action_t)(nat64_br_prefix_publisher_t *NONNULL sm, nat64_br_prefix_publisher_event_t *NULLABLE event);

typedef struct {
    nat64_br_prefix_publisher_state_type_t state;
    char *name;
    nat64_br_prefix_publisher_action_t action;
} nat64_br_prefix_publisher_state_t;

static nat64_br_prefix_publisher_state_type_t
nat64_br_prefix_publisher_init_action(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Setup BR nat64 prefix
    state_machine->br_prefix = nat64_prefix_create(&state_machine->nat64->route_state->srp_server->ula_prefix, NAT64_PREFIX_SLASH_96_BYTES, nat64_preference_low, state_machine->nat64->route_state->srp_server->rloc16);
    if (state_machine->br_prefix == NULL) {
        ERROR("no memory when create br prefix");
        return nat64_br_prefix_publisher_state_invalid;
    }
    // Add 0xFFFF to make it different from OMR prefix
    memset(&state_machine->br_prefix->prefix.s6_addr[6], 0xFF, 2);
    SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf);
    INFO("set br prefix to " PRI_SEGMENTED_IPv6_ADDR_SRP,
         SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf));
    // Switch to next state.
    return nat64_br_prefix_publisher_state_start_timer;
}

static void
nat64_br_prefix_publisher_context_release(void *context)
{
    nat64_br_prefix_publisher_t *state_machine = context;
    RELEASE_HERE(state_machine, nat64_br_prefix_publisher);
}

static void
nat64_br_prefix_publisher_wakeup(void *context)
{
    nat64_br_prefix_publisher_t *state_machine = context;
    nat64_br_prefix_publisher_event_t out_event;

    state_machine->wait_finished = true;
    nat64_br_prefix_publisher_event_init(&out_event, nat64_event_nat64_br_prefix_publisher_okay_to_publish);
    NAT64_EVENT_ANNOUNCE(state_machine, out_event);
    nat64_br_prefix_publisher_event_deliver(state_machine, &out_event);
}

static nat64_br_prefix_publisher_state_type_t
nat64_br_prefix_publisher_start_timer_action(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t * event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (state_machine->timer == NULL) {
        state_machine->timer = ioloop_wakeup_create();
        if (state_machine->timer == NULL) {
            ERROR("no memory when create timer");
            return nat64_br_prefix_publisher_state_invalid;
        }
        RETAIN_HERE(state_machine, nat64_br_prefix_publisher);
        // wait rand(10,30) seconds, will start after thread monitor is settled
        ioloop_add_wake_event(state_machine->timer, state_machine, nat64_br_prefix_publisher_wakeup, nat64_br_prefix_publisher_context_release,
                              NAT64_THREAD_PREFIX_SETTLING_TIME * IOLOOP_SECOND + srp_random16() % (NAT64_BR_PREFIX_PUBLISHER_WAIT_TIME * IOLOOP_SECOND));
        state_machine->wait_finished = false;
    } else {
        INFO("thread prefix monitor timer already started");
    }
    // Switch to next state.
    return nat64_br_prefix_publisher_state_wait_for_anything;
}

static nat64_br_prefix_publisher_state_type_t
nat64_br_prefix_publisher_wait_for_anything_action(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_br_prefix_publisher_state_invalid;
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_ipv4_default_route_showed_up) {
        if (state_machine->nat64->thread_monitor->thread_nat64_prefixes == NULL
            && state_machine->nat64->nat64_infra_prefix_publisher->proposed_prefix == NULL
            && state_machine->wait_finished) {
            return nat64_br_prefix_publisher_state_publish;
        }
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_thread_prefix_changed) {
        if (event->prefix == NULL
            && state_machine->nat64->nat64_infra_prefix_publisher->proposed_prefix == NULL
            && state_machine->wait_finished
            && state_machine->nat64->ipv4_monitor->has_ipv4_default_route) {
            return nat64_br_prefix_publisher_state_publish;
        }
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_okay_to_publish) {
        if (state_machine->nat64->thread_monitor->thread_nat64_prefixes == NULL
            && state_machine->nat64->nat64_infra_prefix_publisher->proposed_prefix == NULL
            && state_machine->nat64->ipv4_monitor->has_ipv4_default_route) {
            return nat64_br_prefix_publisher_state_publish;
        }
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_infra_prefix_changed) {
        if (event->prefix == NULL
            && state_machine->nat64->thread_monitor->thread_nat64_prefixes == NULL
            && state_machine->wait_finished
            && state_machine->nat64->ipv4_monitor->has_ipv4_default_route) {
            return nat64_br_prefix_publisher_state_publish;
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_br_prefix_publisher_state_invalid;
}

static nat64_br_prefix_publisher_state_type_t
nat64_br_prefix_publisher_publish_action(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    // Enable NAT64 translation
    INFO("starting NAT64 translation on BR");
    nat64_start_translation(dispatch_get_main_queue());
    SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf);
    INFO("publishing br prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
          SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf));
    nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->br_prefix,
                                     nat64_prefix_action_add);
    return nat64_br_prefix_publisher_state_publishing;
}

static void
nat64_unpublish_br_prefix(nat64_br_prefix_publisher_t *state_machine)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf);
    INFO("unpublishing br prefix " PRI_SEGMENTED_IPv6_ADDR_SRP,
         SEGMENTED_IPv6_ADDR_PARAM_SRP(state_machine->br_prefix->prefix.s6_addr, nat64_prefix_buf));
    nat64_add_prefix_to_update_queue(state_machine->nat64, state_machine->br_prefix,
                                     nat64_prefix_action_remove);
    // Remove it from thread_monitor prefix database
    nat64_remove_prefix_from_thread_monitor(state_machine->nat64, state_machine->br_prefix);
    INFO("stopping NAT64 translation on BR");
    nat64_stop_translation();
}

static nat64_br_prefix_publisher_state_type_t
nat64_br_prefix_publisher_publishing_action(nat64_br_prefix_publisher_t *state_machine, nat64_br_prefix_publisher_event_t *event)
{
    NAT64_STATE_ANNOUNCE(state_machine, event);
    if (event == NULL) {
        return nat64_br_prefix_publisher_state_invalid;
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_thread_prefix_changed) {
        nat64_prefix_t *thread_prefix;
        for (thread_prefix = event->prefix; thread_prefix; thread_prefix = thread_prefix->next) {
            if (nat64_preference_has_higher_priority(thread_prefix->priority, nat64_preference_low)) {
                // The thread prefix has higher preference
                nat64_unpublish_br_prefix(state_machine);
                return nat64_br_prefix_publisher_state_wait_for_anything;
            } else if (thread_prefix->priority == nat64_preference_low) {
                if (in6addr_compare(&state_machine->br_prefix->prefix, &thread_prefix->prefix) > 0) {
                    nat64_unpublish_br_prefix(state_machine);
                    return nat64_br_prefix_publisher_state_wait_for_anything;
                }
            }
        }
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_ipv4_default_route_went_away ||
               event->event_type == nat64_event_nat64_br_prefix_publisher_shutdown)
    {
        nat64_unpublish_br_prefix(state_machine);
        return nat64_br_prefix_publisher_state_wait_for_anything;
    } else if (event->event_type == nat64_event_nat64_br_prefix_publisher_infra_prefix_changed) {
        // Only unpublish br prefix if there is infra prefix and routable OMR prefix
        if (event->prefix && nat64_thread_has_routable_prefix(state_machine->nat64->route_state)) {
            nat64_unpublish_br_prefix(state_machine);
            return nat64_br_prefix_publisher_state_wait_for_anything;
        }
    } else {
        NAT64_UNEXPECTED_EVENT(state_machine, event);
    }
    return nat64_br_prefix_publisher_state_invalid;
}

#define BR_PUBLISHER_STATE_NAME_DECL(name) nat64_br_prefix_publisher_state_##name, #name
static nat64_br_prefix_publisher_state_t
nat64_br_prefix_publisher_states[] = {
    { BR_PUBLISHER_STATE_NAME_DECL(invalid),                 NULL },
    { BR_PUBLISHER_STATE_NAME_DECL(init),                    nat64_br_prefix_publisher_init_action },
    { BR_PUBLISHER_STATE_NAME_DECL(start_timer),             nat64_br_prefix_publisher_start_timer_action },
    { BR_PUBLISHER_STATE_NAME_DECL(wait_for_anything),       nat64_br_prefix_publisher_wait_for_anything_action },
    { BR_PUBLISHER_STATE_NAME_DECL(publish),                 nat64_br_prefix_publisher_publish_action },
    { BR_PUBLISHER_STATE_NAME_DECL(publishing),              nat64_br_prefix_publisher_publishing_action },
};
#define BR_PREFIX_PUBLISHER_NUM_STATES (sizeof(nat64_br_prefix_publisher_states) / sizeof(nat64_br_prefix_publisher_state_t))

DECLARE_NAT64_STATE_GET(nat64_br_prefix_publisher, BR_PREFIX_PUBLISHER);
DECLARE_NAT64_NEXT_STATE(nat64_br_prefix_publisher);

// BR prefix publisher event functions
typedef struct {
    nat64_br_prefix_publisher_event_type_t event_type;
    char *name;
} nat64_br_prefix_publisher_event_configuration_t;

nat64_br_prefix_publisher_event_configuration_t nat64_br_prefix_publisher_event_configurations[] = {
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_invalid),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_okay_to_publish),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_ipv4_default_route_showed_up),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_ipv4_default_route_went_away),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_thread_prefix_changed),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_infra_prefix_changed),
    NAT64_EVENT_NAME_DECL(nat64_br_prefix_publisher_shutdown),
};
#define BR_PREFIX_PUBLISHER_NUM_EVENT_TYPES (sizeof(nat64_br_prefix_publisher_event_configurations) / sizeof(nat64_br_prefix_publisher_event_configuration_t))

DECLARE_NAT64_EVENT_CONFIGURATION_GET(nat64_br_prefix_publisher_event, BR_PREFIX_PUBLISHER);
DECLARE_NAT64_EVENT_INIT(nat64_br_prefix_publisher_event);
DECLARE_NAT64_EVENT_DELIVER(nat64_br_prefix_publisher);

// BR nat64 prefix publisher state machine end

void
nat64_init(route_state_t *NONNULL route_state)
{
    INFO("nat64_init");
    // Start state machines
    nat64_ipv4_default_route_monitor_next_state(route_state->nat64->ipv4_monitor, nat64_ipv4_default_route_monitor_state_init);
    nat64_infra_prefix_monitor_next_state(route_state->nat64->infra_monitor, nat64_infra_prefix_monitor_state_init);
    nat64_thread_prefix_monitor_next_state(route_state->nat64->thread_monitor, nat64_thread_prefix_monitor_state_init);
    nat64_infra_prefix_publisher_next_state(route_state->nat64->nat64_infra_prefix_publisher, nat64_infra_prefix_publisher_state_init);
    nat64_br_prefix_publisher_next_state(route_state->nat64->nat64_br_prefix_publisher, nat64_br_prefix_publisher_state_init);
}

void
nat64_stop(route_state_t *NONNULL route_state)
{
    if (route_state->nat64) {
        INFO("stopping nat64.");
        nat64_cancel(route_state->nat64);
        RELEASE_HERE(route_state->nat64, nat64);
        route_state->nat64 = NULL;
    }
}

void
nat64_start(route_state_t *NONNULL route_state)
{
    route_state->nat64 = nat64_create(route_state);
    if (route_state->nat64 == NULL) {
        ERROR("nat64 create failed");
        return;
    }
    nat64_init(route_state);
}

static void
nat64_add_route_callback(void *context, cti_status_t status)
{
    (void)context;
    INFO("%d", status);
}

void
nat64_add_prefix(route_state_t *route_state, const uint8_t *const data, offmesh_route_preference_t route_pref)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(data, nat64_prefix_buf);
    INFO("nat64_add_prefix(" PRI_SEGMENTED_IPv6_ADDR_SRP ")",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(data, nat64_prefix_buf));
    int status = cti_add_route(route_state->srp_server, route_state, nat64_add_route_callback, NULL,
                               (struct in6_addr *)data, NAT64_PREFIX_SLASH_96_BYTES * 8,
                               route_pref, 0, true, true);
    if (status != kCTIStatus_NoError) {
        ERROR("Unable to add nat64 prefix.");
    }
}

static void
nat64_remove_route_callback(void *context, cti_status_t status)
{
    (void)context;
    INFO("%d", status);
}

void
nat64_remove_prefix(route_state_t *route_state, const uint8_t *const data)
{
    SEGMENTED_IPv6_ADDR_GEN_SRP(data, nat64_prefix_buf);
    INFO("nat64_remove_prefix(" PRI_SEGMENTED_IPv6_ADDR_SRP ")",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(data, nat64_prefix_buf));
    int status = cti_remove_route(route_state->srp_server, route_state, nat64_remove_route_callback, NULL,
                                  (struct in6_addr *)data, NAT64_PREFIX_SLASH_96_BYTES * 8, 0);
    if (status != kCTIStatus_NoError) {
        ERROR("Unable to remove nat64 prefix.");
    }
}

static offmesh_route_preference_t
nat64_pref_to_route_pref(nat64_preference nat64_pref)
{
    if (nat64_pref == nat64_preference_low) {
        return offmesh_route_preference_low;
    } else if (nat64_pref == nat64_preference_high) {
        return offmesh_route_preference_high;
    } else if (nat64_pref == nat64_preference_medium) {
        return offmesh_route_preference_medium;
    } else {
        ERROR("Unknown nat64 prefix preference %d", nat64_pref);
        return offmesh_route_preference_low;
    }
}

static void
nat64_prefix_update_callback(void *context, cti_status_t status)
{
    nat64_t *nat64 = context;
    nat64_prefix_t *prefix = nat64->update_queue;
    INFO("status %d", status);
    if (prefix == NULL) {
        ERROR("update seems to have disappeared");
        return;
    }
    SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
    INFO(PRI_SEGMENTED_IPv6_ADDR_SRP " was " PUB_S_SRP,
         SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf),
         prefix->action == nat64_prefix_action_add ? "added" : "removed");
    // The pending flag was set to true in nat64_prefix_start_next_update(), meaning that
    // we sent the request to threadradiod, but it's not finished yet, so the status is pending.
    // when this callback function is called, the status is not pending anymore.
    if (prefix->pending) {
        prefix->pending = false;
        nat64->update_queue = prefix->next;
        prefix->next = NULL;
        RELEASE_HERE(prefix, nat64_prefix);
    }
    // Start next update
    if (nat64->update_queue != NULL) {
        nat64_prefix_start_next_update(nat64);
    } else {
        // The update queue holds a reference to nat64 when there is something on the queue.
        // Release here if there is nothing on the queue.
        RELEASE_HERE(nat64, nat64);
    }
}

static void
nat64_prefix_start_next_update(nat64_t *nat64)
{
    cti_status_t status;
    nat64_prefix_t *prefix = nat64->update_queue;
    if (prefix == NULL) {
        ERROR("nat64_prefix_start_next_update called with no update");
        return;
    }
    route_state_t *route_state = nat64->route_state;
    srp_server_t *server_state = route_state->srp_server;

    SEGMENTED_IPv6_ADDR_GEN_SRP(&prefix->prefix, prefix_buf);
    if (prefix->action == nat64_prefix_action_remove) {
        INFO("removing: " PRI_SEGMENTED_IPv6_ADDR_SRP ,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf));
        status = cti_remove_route(server_state, nat64, nat64_prefix_update_callback, NULL,
                                  &prefix->prefix, NAT64_PREFIX_SLASH_96_BYTES * 8, 0);
    } else if (prefix->action == nat64_prefix_action_add){
        INFO("adding: " PRI_SEGMENTED_IPv6_ADDR_SRP ,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf));
        status = cti_add_route(server_state, nat64, nat64_prefix_update_callback, NULL,
                               &prefix->prefix, NAT64_PREFIX_SLASH_96_BYTES * 8,
                               nat64_pref_to_route_pref(prefix->priority), 0, true, true);
    } else {
        ERROR("updating: " PRI_SEGMENTED_IPv6_ADDR_SRP " with action %d",
              SEGMENTED_IPv6_ADDR_PARAM_SRP(&prefix->prefix, prefix_buf), prefix->action);
        nat64->update_queue = prefix->next;
        prefix->next = NULL;
        RELEASE_HERE(prefix, nat64_prefix);
        return;
    }
    if (status != kCTIStatus_NoError) {
        ERROR("route update failed: %d", status);
    } else {
        prefix->pending = true;
    }
}

static void
nat64_add_prefix_to_update_queue(nat64_t *nat64, nat64_prefix_t *prefix, nat64_prefix_action action)
{
    nat64_prefix_t **ppref, *old_queue = nat64->update_queue;
    // Find the prefix on the queue, or find the end of the queue.
    for (ppref = &nat64->update_queue; *ppref != NULL && *ppref != prefix; ppref = &(*ppref)->next);
    // Not on the queue
    if (*ppref == NULL) {
        nat64_prefix_t * new_prefix = nat64_prefix_dup(prefix);
        if (new_prefix == NULL) {
            ERROR("no memory for nat64 prefix.");
            return;
        }
        new_prefix->action = action;
        // The pending flag will be set to true in nat64_prefix_start_next_update()
        // when we send the request to threadradiod.
        new_prefix->pending = false;
        *ppref = new_prefix;
        // Turns out we added it to the beginning of the queue.
        if (nat64->update_queue == new_prefix) {
            nat64_prefix_start_next_update(nat64);
        }
        goto out;
    }
    // We have started to update the prefix, but haven't gotten the callback yet. Since we have put the prefix
    // back on the update queue, and it's at the beginning, mark it not pending so that when we get the callback
    // from the update function, we update this route again rather than going on to the next.
    if (prefix == nat64->update_queue) {
        prefix->pending = false;
    }
out:
    // As long as there is anything in the queue, the queue needs to hold a reference to nat64,
    // so that if it's canceled and released, we finish running the queue before stopping.
    if (old_queue == NULL && nat64->update_queue != NULL) {
        RETAIN_HERE(nat64, nat64);
    }
}

// Check stale prefix on thread network.
// For example, prefix that belongs to current BR, but current BR is not in publishing state, this can happen when srp-mdns-proxy daemon restarted.
// Remove such prefix from thread network.
static void
nat64_check_stale_prefix(route_state_t *route_state, const cti_route_vec_t *const routes)
{
    size_t i;
    for (i = 0; i < routes->num; i++) {
        cti_route_t *route = routes->routes[i];
        // This is nat64 prefix published by us
        if (route->nat64 && route->origin == offmesh_route_origin_ncp
            && route->rloc == route_state->srp_server->rloc16) {
            // br generated nat64 prefix
            if (route->preference == offmesh_route_preference_low) {
                nat64_prefix_t *prefix = route_state->nat64->nat64_br_prefix_publisher->br_prefix;
                // If we are not publishing or
                // we are publishing but the prefix is different, this can happen when ula changed
                if ((route_state->nat64->nat64_br_prefix_publisher->state != nat64_br_prefix_publisher_state_publishing) ||
                    (prefix && in6prefix_compare(&prefix->prefix, &route->prefix, NAT64_PREFIX_SLASH_96_BYTES))) {
                    nat64_prefix_t *tmp = nat64_prefix_create(&route->prefix, NAT64_PREFIX_SLASH_96_BYTES,
                                                              route_pref_to_nat64_pref(route->preference), route->rloc);
                    SEGMENTED_IPv6_ADDR_GEN_SRP(tmp->prefix.s6_addr, nat64_prefix_buf);
                    INFO("stale br prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " removing",
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(tmp->prefix.s6_addr, nat64_prefix_buf));
                    nat64_add_prefix_to_update_queue(route_state->nat64, tmp, nat64_prefix_action_remove);
                    RELEASE_HERE(tmp, nat64_prefix);
                }
            // prefix from infrastructure
            } else if (route->preference == offmesh_route_preference_medium) {
                nat64_prefix_t *prefix = route_state->nat64->nat64_infra_prefix_publisher->proposed_prefix;
                // If we are not publishing or
                // we are publishing but the prefix is different, this can happen when infra prefix changed
                if ((route_state->nat64->nat64_infra_prefix_publisher->state != nat64_infra_prefix_publisher_state_publishing) ||
                    (prefix && in6prefix_compare(&prefix->prefix, &route->prefix, NAT64_PREFIX_SLASH_96_BYTES))) {
                    nat64_prefix_t *tmp = nat64_prefix_create(&route->prefix, NAT64_PREFIX_SLASH_96_BYTES,
                                                              route_pref_to_nat64_pref(route->preference), route->rloc);
                    SEGMENTED_IPv6_ADDR_GEN_SRP(tmp->prefix.s6_addr, nat64_prefix_buf);
                    INFO("stale infra prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " removing",
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(tmp->prefix.s6_addr, nat64_prefix_buf));
                    nat64_add_prefix_to_update_queue(route_state->nat64, tmp, nat64_prefix_action_remove);
                    RELEASE_HERE(tmp, nat64_prefix);
                }
            }
        }
    }
}

void
nat64_offmesh_route_list_callback(route_state_t *route_state, cti_route_vec_t *routes, cti_status_t status)
{
    if (status != kCTIStatus_NoError) {
        ERROR("status %d", status);
    } else {
        INFO("got %zu offmesh routes", routes->num);
        nat64_check_stale_prefix(route_state, routes);
        nat64_thread_prefix_monitor_t *state_machine = route_state->nat64->thread_monitor;
        nat64_thread_prefix_monitor_event_t out_event;
        nat64_thread_prefix_monitor_event_init(&out_event, nat64_event_thread_prefix_update);
        out_event.routes = routes;
        NAT64_EVENT_ANNOUNCE(state_machine, out_event);
        nat64_thread_prefix_monitor_event_deliver(state_machine, &out_event);
    }
}

void
nat64_thread_shutdown(route_state_t *route_state)
{
    nat64_t *nat64 = route_state->nat64;
    if (nat64->nat64_infra_prefix_publisher != NULL) {
        nat64_infra_prefix_publisher_event_t infra_event;
        nat64_infra_prefix_publisher_event_init(&infra_event, nat64_event_nat64_infra_prefix_publisher_shutdown);
        nat64_infra_prefix_publisher_event_deliver(nat64->nat64_infra_prefix_publisher, &infra_event);
    }
    if (nat64->nat64_br_prefix_publisher != NULL) {
        nat64_br_prefix_publisher_event_t br_event;
        nat64_br_prefix_publisher_event_init(&br_event, nat64_event_nat64_br_prefix_publisher_shutdown);
        nat64_br_prefix_publisher_event_deliver(nat64->nat64_br_prefix_publisher, &br_event);
    }
}
#endif

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
