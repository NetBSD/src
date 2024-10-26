/* nat64-macos.h
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

#ifndef NAT64_H
#define NAT64_H

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "dnssd-proxy.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "route.h"
#define DNSMessageHeader dns_wire_t
#include "ioloop-common.h" // for service_connection_t

#define NAT64_PREFIX_LLQ_QUERY_DOMAIN        "ipv4only.arpa"
#define NAT64_PREFIX_SLASH_96_BYTES          12                // Thread spec limit NAT64 prefix to /96
#define NAT64_THREAD_PREFIX_SETTLING_TIME    10                // In seconds
#define NAT64_BR_PREFIX_PUBLISHER_WAIT_TIME  30                // In seconds
#define NAT64_INFRA_PREFIX_LIMIT             3                 // Max number of allowed nat64 prefixes from infra on thread network


// Refer to https://www.rfc-editor.org/rfc/rfc4191
typedef enum {
    nat64_preference_medium     = 0,
    nat64_preference_high       = 1,
    nat64_preference_low        = 3,
    nat64_preference_reserved   = 2,
} nat64_preference;

typedef enum {
    nat64_prefix_action_none      = 0,
    nat64_prefix_action_add       = 1,
    nat64_prefix_action_remove    = 2,
} nat64_prefix_action;

typedef struct nat64_prefix nat64_prefix_t;
typedef struct nat64 nat64_t;

struct nat64_prefix {
    uint32_t ref_count;
    nat64_prefix_t *NULLABLE next;
    struct in6_addr prefix;
    int prefix_len;
    nat64_preference priority;
    nat64_prefix_action action;
    int rloc;
    bool pending;
};

// ipv4 default route monitor
typedef enum nat64_ipv4_default_route_monitor_event_type {
    nat64_event_ipv4_default_route_invalid = 0,
    nat64_event_ipv4_default_route_update,
    nat64_event_ipv4_default_route_showed_up,
    nat64_event_ipv4_default_route_went_away,
} nat64_ipv4_default_route_monitor_event_type_t;

typedef enum {
    nat64_ipv4_default_route_monitor_state_invalid = 0,
    nat64_ipv4_default_route_monitor_state_init,
    nat64_ipv4_default_route_monitor_state_wait_for_event,
} nat64_ipv4_default_route_monitor_state_type_t;

typedef struct nat64_ipv4_default_route_monitor_event {
    char *NONNULL name;
    union {
        bool has_ipv4_connectivity;
        bool has_ipv4_default_route;
    };
    nat64_ipv4_default_route_monitor_event_type_t event_type;
} nat64_ipv4_default_route_monitor_event_t;

typedef struct {
    uint32_t ref_count;
    nat64_ipv4_default_route_monitor_state_type_t state;
    bool has_ipv4_default_route;
    char *NONNULL state_name;
    nat64_t *NONNULL nat64;
} nat64_ipv4_default_route_monitor_t;

// Infrastructure prefix monitor
typedef enum nat64_infra_prefix_monitor_event_type {
    nat64_event_infra_prefix_invalid = 0,
    nat64_event_infra_prefix_update,
} nat64_infra_prefix_monitor_event_type_t;

typedef struct nat64_infra_prefix_monitor_event {
    char *NONNULL name;
    DNSServiceFlags flags;
    union {
        const void *NULLABLE rdata;
        nat64_prefix_t *NULLABLE prefix;
    };
    nat64_infra_prefix_monitor_event_type_t event_type;
} nat64_infra_prefix_monitor_event_t;

typedef enum {
    nat64_infra_prefix_monitor_state_invalid = 0,
    nat64_infra_prefix_monitor_state_init,
    nat64_infra_prefix_monitor_state_wait_for_change,
    nat64_infra_prefix_monitor_state_change_occurred,
} nat64_infra_prefix_monitor_state_type_t;

typedef struct {
    uint32_t ref_count;
    nat64_infra_prefix_monitor_state_type_t state;
    char *NONNULL state_name;
    DNSServiceRef NULLABLE sdRef;    // LLQ sdRef
    nat64_prefix_t *NULLABLE infra_nat64_prefixes;
    nat64_t *NONNULL nat64;
    bool canceled;
} nat64_infra_prefix_monitor_t;


// Thread prefix monitor
typedef enum nat64_thread_prefix_monitor_event_type {
    nat64_event_thread_prefix_invalid = 0,
    nat64_event_thread_prefix_init_wait_ended,
    nat64_event_thread_prefix_update,
} nat64_thread_prefix_monitor_event_type_t;

typedef struct nat64_thread_prefix_monitor_event {
    char *NONNULL name;
    union {
        nat64_prefix_t *NULLABLE prefix;
        cti_route_vec_t *NULLABLE routes;
    };
    nat64_thread_prefix_monitor_event_type_t event_type;
} nat64_thread_prefix_monitor_event_t;

typedef enum {
    nat64_thread_prefix_monitor_state_invalid = 0,
    nat64_thread_prefix_monitor_state_init,
    nat64_thread_prefix_monitor_state_wait_for_settling,
    nat64_thread_prefix_monitor_state_wait_for_change,
    nat64_thread_prefix_monitor_state_change_occurred,
} nat64_thread_prefix_monitor_state_type_t;

typedef struct {
    uint32_t ref_count;
    nat64_thread_prefix_monitor_state_type_t state;
    char *NONNULL state_name;
    nat64_prefix_t *NULLABLE thread_nat64_prefixes;
    wakeup_t *NULLABLE timer;
    nat64_t *NONNULL nat64;
} nat64_thread_prefix_monitor_t;

// Infra nat64 prefix publisher
typedef enum nat64_infra_prefix_publisher_event_type {
    nat64_event_nat64_infra_prefix_publisher_invalid = 0,
    nat64_event_nat64_infra_prefix_publisher_thread_prefix_changed,
    nat64_event_nat64_infra_prefix_publisher_infra_prefix_changed,
    nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_went_away,
    nat64_event_nat64_infra_prefix_publisher_routable_omr_prefix_showed_up,
    nat64_event_nat64_infra_prefix_publisher_shutdown,
} nat64_infra_prefix_publisher_event_type_t;

typedef struct nat64_infra_prefix_publisher_event {
    char *NONNULL name;
    nat64_prefix_t *NULLABLE prefix;
    nat64_infra_prefix_publisher_event_type_t event_type;
} nat64_infra_prefix_publisher_event_t;

typedef enum {
    nat64_infra_prefix_publisher_state_invalid = 0,
    nat64_infra_prefix_publisher_state_init,
    nat64_infra_prefix_publisher_state_wait,        // Wait for infra prefix
    nat64_infra_prefix_publisher_state_ignore,      // Ignore infra prefix
    nat64_infra_prefix_publisher_state_check,       // Check infra prefix
    nat64_infra_prefix_publisher_state_publish,     // Publish infra prefix
    nat64_infra_prefix_publisher_state_publishing,  // Publishing infra prefix
} nat64_infra_prefix_publisher_state_type_t;

typedef struct {
    uint32_t ref_count;
    nat64_infra_prefix_publisher_state_type_t state;
    char *NONNULL state_name;
    bool routable_omr_prefix_present;
    nat64_prefix_t *NULLABLE proposed_prefix;
    nat64_t *NONNULL nat64;
} nat64_infra_prefix_publisher_t;

// BR nat64 prefix publisher
typedef enum nat64_br_prefix_publisher_event_type {
    nat64_event_nat64_br_prefix_publisher_invalid = 0,
    nat64_event_nat64_br_prefix_publisher_okay_to_publish,
    nat64_event_nat64_br_prefix_publisher_ipv4_default_route_showed_up,
    nat64_event_nat64_br_prefix_publisher_ipv4_default_route_went_away,
    nat64_event_nat64_br_prefix_publisher_thread_prefix_changed,
    nat64_event_nat64_br_prefix_publisher_infra_prefix_changed,
    nat64_event_nat64_br_prefix_publisher_shutdown,
} nat64_br_prefix_publisher_event_type_t;

typedef struct nat64_br_prefix_publisher_event {
    char *NONNULL name;
    nat64_prefix_t *NULLABLE prefix;
    nat64_br_prefix_publisher_event_type_t event_type;
} nat64_br_prefix_publisher_event_t;

typedef enum {
    nat64_br_prefix_publisher_state_invalid = 0,
    nat64_br_prefix_publisher_state_init,
    nat64_br_prefix_publisher_state_start_timer,
    nat64_br_prefix_publisher_state_wait_for_anything,
    nat64_br_prefix_publisher_state_publish,
    nat64_br_prefix_publisher_state_publishing,
} nat64_br_prefix_publisher_state_type_t;

typedef struct {
    uint32_t ref_count;
    nat64_br_prefix_publisher_state_type_t state;
    char *NONNULL state_name;
    nat64_prefix_t *NULLABLE br_prefix;
    wakeup_t *NULLABLE timer;
    bool wait_finished;
    nat64_t *NONNULL nat64;
} nat64_br_prefix_publisher_t;

struct nat64 {
    uint32_t ref_count;
    route_state_t *NONNULL route_state;
    nat64_prefix_t *NULLABLE update_queue;

    // State machines
    nat64_ipv4_default_route_monitor_t *NULLABLE ipv4_monitor;
    nat64_infra_prefix_monitor_t *NULLABLE infra_monitor;
    nat64_thread_prefix_monitor_t *NULLABLE thread_monitor;
    nat64_infra_prefix_publisher_t *NULLABLE nat64_infra_prefix_publisher;
    nat64_br_prefix_publisher_t *NULLABLE nat64_br_prefix_publisher;
};


nat64_t *NULLABLE nat64_create(route_state_t *NONNULL route_state);
nat64_prefix_t *NULLABLE nat64_prefix_create(struct in6_addr *NONNULL address, int prefix_length,
                                             nat64_preference pref, int rloc);
void nat64_add_prefix(route_state_t *NONNULL route_state, const uint8_t *NONNULL const data,
                      offmesh_route_preference_t route_pref);
void nat64_remove_prefix(route_state_t *NONNULL route_state, const uint8_t *NONNULL const data);
void nat64_offmesh_route_list_callback(route_state_t *NONNULL route_state, cti_route_vec_t *NONNULL routes,
                                       cti_status_t status);
void nat64_init(route_state_t *NONNULL route_state);
void nat64_default_route_update(nat64_t *NONNULL nat64, bool has_ipv4_connectivity);
void nat64_omr_route_update(nat64_t *NONNULL nat64, bool has_routable_omr_prefix);
void nat64_stop(route_state_t *NONNULL route_state);
void nat64_start(route_state_t *NONNULL route_state);
void nat64_thread_shutdown(route_state_t *NONNULL route_state);
#endif /* NAT64_H */

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
