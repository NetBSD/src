/* route.h
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
 * Definitions for route.c
 */

#ifndef __SERVICE_REGISTRATION_ROUTE_H
#define __SERVICE_REGISTRATION_ROUTE_H
#if defined(USE_IPCONFIGURATION_SERVICE)
#include <SystemConfiguration/SystemConfiguration.h>
#include "IPConfigurationService.h"
#endif

typedef struct icmp_listener icmp_listener_t;
typedef struct route_state route_state_t;
typedef struct srp_server_state srp_server_t;
typedef struct nat64 nat64_t;
typedef struct omr_watcher omr_watcher_t;
typedef struct omr_watcher_callback omr_watcher_callback_t;
typedef struct omr_publisher omr_publisher_t;
typedef struct route_tracker route_tracker_t;

// RFC 4861 specifies a minimum of 4 seconds between RAs. We add a bit of fuzz.
#define MIN_DELAY_BETWEEN_RAS 4000
#define RA_FUZZ_TIME          1000

// RFC 4861 specifies a maximum of three transmissions when sending an RA.
#define MAX_RA_RETRANSMISSION 3

// There's no limit for unicast neighbor solicits, but we limit it to three.
#define MAX_NS_RETRANSMISSIONS 3

// 60 seconds between router probes, three retries, four seconds per retry. We should have an answer from the router at
// most 76 seconds after the previous answer, assuming that it takes four seconds for the response to arrive, which is
// of course ridiculously long.
#define MAX_ROUTER_RECEIVED_TIME_GAP_BEFORE_UNREACHABLE 76 * MSEC_PER_SEC

// The end of the valid lifetime of the prefix is the time we received it plus the valid lifetime that was expressed in
// the PIO option of the RA that advertised the prefix. If we have a prefix that is within ten minutes of expiring, we
// consider it stale and start advertising a prefix. This should never happen in a network where a router is advertising
// a prefix--if it does, either we're having trouble receiving multicast RAs (meaning that we don't get every beacon) or
// the router has gone away.
#define MAX_ROUTER_RECEIVED_TIME_GAP_BEFORE_STALE      600 * MSEC_PER_SEC

// The Thread BR prefix needs to stick around long enough that it's not likely to accidentally disappear because of
// dropped multicasts, but short enough that it goes away quickly when a router that's advertising IPv6 connectivity
// comes online.
#define BR_PREFIX_LIFETIME 30 * 60

#define BR_PREFIX_SLASH_64_BYTES 8


#ifndef RTR_SOLICITATION_INTERVAL
#define RTR_SOLICITATION_INTERVAL       4       /* 4sec */
#endif

#ifndef ND6_INFINITE_LIFETIME
#define ND6_INFINITE_LIFETIME           0xffffffff
#endif

#define MAX_ANYCAST_NUM 5

typedef struct interface interface_t;
typedef struct icmp_message icmp_message_t;
struct interface {
    int ref_count;

    interface_t *NULLABLE next;
    char *NONNULL name;

    // Wakeup event for next beacon.
    wakeup_t *NULLABLE beacon_wakeup;

    // Wakeup event called after we're done sending solicits.  At this point we delete all routes more than 10 minutes
    // old; if none are left, then we assume there's no IPv6 service on the interface.
    wakeup_t *NULLABLE post_solicit_wakeup;

    // Wakeup event to trigger the next router solicit or neighbor solicit to be sent.
    wakeup_t *NULLABLE router_solicit_wakeup;

    // Wakeup event to trigger the next router solicit to be sent.
    wakeup_t *NULLABLE neighbor_solicit_wakeup;

    // Wakeup event to deconfigure the on-link prefix after it is no longer valid.
    wakeup_t *NULLABLE deconfigure_wakeup;

#if SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY
    // Wakeup event to detect that vicarious router discovery is complete
    wakeup_t *NULLABLE vicarious_discovery_complete;
#endif // SRP_FEATURE_VICARIOUS_ROUTER_DISCOVERY

    // Wakeup event to periodically notice whether routers we have heard previously on this interface have gone stale.
    wakeup_t *NULLABLE stale_evaluation_wakeup;

    // Wakeup event to periodically probe routers for reachability
    wakeup_t *NULLABLE router_probe_wakeup;

    // The route state object to which this interface belongs.
    route_state_t *NULLABLE route_state;

    // List of Router Advertisement messages from different routers.
    icmp_message_t *NULLABLE routers;

    // List of Router Solicit messages from different hosts for which we are still transmitting unicast
    // RAs (we sent three unicast RAs per solicit to ensure delivery).
    icmp_message_t *NULLABLE solicits;

    int prefix_number;

#if defined(USE_IPCONFIGURATION_SERVICE)
    // The service used to configure this interface with an address in the on-link prefix
    IPConfigurationServiceRef NULLABLE ip_configuration_service;

    // SCDynamicStoreRef
    SCDynamicStoreRef NULLABLE ip_configuration_store;
#else
    subproc_t *NULLABLE link_route_adder_process;
#endif

    struct in6_addr link_local;  // Link-local address
    struct in6_addr ipv6_prefix; // This is the prefix we advertise, if advertise_ipv6_prefix is true.

    // Absolute time of last beacon, and of next beacon.
    uint64_t last_beacon, next_beacon;

    // Absolute deadline for deprecating the on-link prefix we've been announcing
    uint64_t deprecate_deadline;

    // Last time we did a router probe
    uint64_t last_router_probe;

    // Preferred lifetime for the on-link prefix
    uint32_t preferred_lifetime;

    // Valid lifetime for the on-link prefix
    uint32_t valid_lifetime;

    // When the interface becomes active, we send up to three solicits.
    // Later on, we send three neighbor solicit probes every sixty seconds to verify router reachability
    int num_solicits_sent;

    // The interface index according to the operating systme.
    int index;

    // Number of IPv4 addresses configured on link.
    int num_ipv4_addresses;

    // Number of IPv4 addresses configured on link.
    int num_ipv6_addresses, old_num_ipv6_addresses;

    // Number of beacons sent. After the first three, the inter-beacon interval goes up.
    int num_beacons_sent;

    // The interface link layer address, if known.
    uint8_t link_layer[6];

    // True if the interface is not usable.
    bool inactive, previously_inactive;

    // True if this interface can never be used for routing to the thread network (e.g., loopback, tunnels, etc.)
    bool ineligible, previously_ineligible;

    // True if we've determined that it's the thread interface.
    bool is_thread;


    // True if we have (or intended to) advertised our own prefix on this link. It should be true until the prefix
    // we advertised should have expired on all hosts that might have received it. This will be set before we actually
    // advertise a prefix, so before the first time we advertise a prefix it may be set even though the prefix can't
    // appear in any host's routing table yet.
    bool our_prefix_advertised;

    // True if we should suppress on-link prefix. This would be the case when deprecating if we aren't sending
    // periodic updates of the deprecated prefix.
    bool suppress_ipv6_prefix;

    // True if we've gotten a link-layer address.
    bool have_link_layer_address;

    // True if the on-link prefix is configured on the interface.
    bool on_link_prefix_configured;

    // True if we've sent our first beacon since the interface came up.
    bool sent_first_beacon;

    // Indicates whether or not router discovery was ever started for this interface.
    bool router_discovery_started;

    // Indicates whether or not router discovery has completed for this interface.
    bool router_discovery_complete;

    // Indicates whether we're currently doing router discovery, so that we don't
    // restart it when we're already doing it.
    bool router_discovery_in_progress;

    // Indicates that we've received a router discovery message from some other host,
    // and are waiting 20 seconds to snoop for replies to that RD message that are
    // multicast.   If we hear no replies during that time, we trigger router discovery.
    bool vicarious_router_discovery_in_progress;

    // True if we are probing usable routers with neighbor solicits to see if they are still alive.
    bool probing;

    // Indicates that we have received an interface removal event, it is useful when srp-mdns-proxy is changed to a new
    // network where the network signature are the same and they both have no IPv6 service (so no IPv6 prefix will be
    // removed), in such case there will be no change from srp-mdns-proxy's point of view. However, configd may still
    // flush the IPv6 routing since changing network would cause interface up/down. When the flushing happens,
    // srp-mdns-proxy should be able to reconfigure the IPv6 routing by reconfiguring IPv6 prefix. By setting
    // need_reconfigure_prefix only when interface address removal happens and check it during the routing evaluation
    // srp-mdns-proxy can reconfigure it after the routing evaluation finishes, like router discovery.
    bool need_reconfigure_prefix;

    // This variable is used to notice when the path evaluator doesn't return an interface on the interface list.
    // In this situation, the interface is inactive and if we are using it we should stop.
    bool listed;
};

typedef enum icmp_option_type {
    icmp_option_source_link_layer_address =  1,
    icmp_option_target_link_layer_address =  2,
    icmp_option_prefix_information        =  3,
    icmp_option_redirected_header         =  4,
    icmp_option_mtu                       =  5,
    icmp_option_route_information         = 24,
    icmp_option_ra_flags_extension        = 26,
} icmp_option_type_t;

typedef enum icmp_type {
    icmp_type_echo_request           = 128,
    icmp_type_echo_reply             = 129,
    icmp_type_router_solicitation    = 133,
    icmp_type_router_advertisement   = 134,
    icmp_type_neighbor_solicitation  = 135,
    icmp_type_neighbor_advertisement = 136,
    icmp_type_redirect               = 137,
} icmp_type_t;

#ifndef ND_OPT_RA_FLAGS_EXTENSION
#define ND_OPT_RA_FLAGS_EXTENSION icmp_option_ra_flags_extension
#endif
#define RA_FLAGS1_STUB_ROUTER 0x80

typedef struct link_layer_address {
    uint16_t length;
    uint8_t address[32];
} link_layer_address_t;

typedef uint8_t ra_flags_extension_t[6];

typedef struct prefix_information {
    struct in6_addr prefix;
    uint8_t length;
    uint8_t flags;
    uint32_t valid_lifetime;
    uint32_t preferred_lifetime;
    bool found; // For comparing RAs
} prefix_information_t;

typedef struct route_information {
    struct in6_addr prefix;
    uint8_t length;
    uint8_t flags;
    uint32_t route_lifetime;
} route_information_t;

typedef struct icmp_option {
    icmp_option_type_t type;
    union {
        link_layer_address_t link_layer_address;
        prefix_information_t prefix_information;
        route_information_t route_information;
        ra_flags_extension_t ra_flags_extension;
    } option;
} icmp_option_t;

struct icmp_message {
    icmp_message_t *NULLABLE next;
    interface_t *NULLABLE interface;
    icmp_option_t *NULLABLE options;
    wakeup_t *NULLABLE wakeup;
    route_state_t *NULLABLE route_state;

    bool usable;                         // True if this router was usable at the last policy evaluation
    bool reachable;                      // True if this router was reachable when last probed
    bool reached;                        // Set to true when we get a neighbor advertise from the router
    bool new_router;                     // If this router information is a newly received one.
    bool received_time_already_adjusted; // if the received time of the message is already adjusted by vicarious mode
    bool stub_router;                    // True if this RA came from a stub router.
    int retransmissions_received;        // # times we've received a solicit from this host during retransmit window
    int messages_sent;                   // # of unicast RAs we've sent in response to a solicit, or # of unicast NSs
                                         // we've sent to confirm router aliveness

    struct in6_addr source;
    struct in6_addr destination;

    uint64_t received_time;
    uint64_t latest_na;                 // Most recent time at which we successfully got a neighbor advertise

    uint32_t reachable_time;
    uint32_t retransmission_timer;
    uint8_t cur_hop_limit;          // Current hop limit for Router Advertisement messages.
    uint8_t flags;
    uint8_t type;
    uint8_t code;
    uint16_t checksum;              // We hope the kernel figures this out for us.
    uint16_t router_lifetime;

    int num_options;
    int hop_limit;                  // Hop limit provided by the kernel, must be 255.
};

struct route_state {
    route_state_t *NULLABLE next;
    const char *NULLABLE name;
    srp_server_t *NULLABLE srp_server;
    interface_address_state_t *NULLABLE interface_addresses;
    omr_watcher_t *NULLABLE omr_watcher;
    omr_publisher_t *NULLABLE omr_publisher;
    route_tracker_t *NULLABLE route_tracker;
    omr_watcher_callback_t *NULLABLE omr_watcher_callback;

    // If true, a prefix with L=1, A=0 in an RA with M=1 is treated as usable. The reason it's not treated as
    // usable by default is that this will break Thread for Android phones on networks where IPv6 is present
    // but only DHCPv6 is supported.
    bool config_enable_dhcpv6_prefixes;

    interface_t *NULLABLE interfaces;
    bool have_thread_prefix;
    struct in6_addr my_thread_ula_prefix;
    bool have_mesh_local_prefix;
    bool have_mesh_local_address;
    bool advertising_srp_anycast_service;
    bool advertising_srp_unicast_service;
    bool have_proposed_srp_listener_address;
    bool seen_listener_address;
    struct in6_addr thread_mesh_local_prefix;
    struct in6_addr thread_mesh_local_address;
    struct in6_addr proposed_srp_listener_address;
    struct in6_addr srp_listener_ip_address;
    uint16_t srp_service_listen_port;
    comm_t *NULLABLE srp_listener;
    struct in6_addr xpanid_prefix;
    bool have_xpanid_prefix;
    int num_thread_interfaces; // Should be zero or one.
    int ula_serial;
    int num_thread_prefixes;
    int times_advertised_unicast, times_advertised_anycast;
    int times_unadvertised_unicast, times_unadvertised_anycast;
    subproc_t *NULLABLE thread_interface_enumerator_process;
    subproc_t *NULLABLE thread_prefix_adder_process;
    subproc_t *NULLABLE thread_rti_setter_process;
    subproc_t *NULLABLE thread_forwarding_setter_process;
    subproc_t *NULLABLE tcpdump_logger_process;
    char *NULLABLE thread_interface_name;
    char *NULLABLE home_interface_name;
    bool have_non_thread_interface;
    bool seen_legacy_service;
#if SRP_FEATURE_NAT64
    nat64_t *NULLABLE nat64;
#endif
    bool have_rloc16;
    uint8_t thread_sequence_number;

#ifndef RA_TESTER
    wakeup_t *NULLABLE thread_network_shutdown_wakeup;
    cti_network_state_t current_thread_state;
    cti_connection_t NULLABLE thread_role_context;
    cti_connection_t NULLABLE thread_state_context;
    cti_connection_t NULLABLE thread_xpanid_context;
    cti_connection_t NULLABLE thread_route_context;
    cti_connection_t NULLABLE thread_rloc16_context;
    cti_connection_t NULLABLE thread_ml_prefix_connection;
    bool thread_network_running;
    bool thread_network_shutting_down;
#endif

#if !defined(RA_TESTER)
    wakeup_t *NULLABLE wpan_reconnect_wakeup;
#endif // !defined(RA_TESTER)
#if !defined(RA_TESTER)
    uint64_t partition_last_prefix_set_change;
    uint64_t partition_last_pref_id_set_change;
    uint64_t partition_last_role_change;
    uint64_t partition_last_state_change;
    uint64_t partition_settle_start;
    uint64_t partition_service_last_add_time;
    bool partition_have_prefix_list;
    bool partition_have_pref_id_list;
    bool partition_tunnel_name_is_known;
    bool partition_can_advertise_service;
    bool partition_can_advertise_anycast_service;
    bool partition_can_provide_routing;
    bool partition_has_xpanid;
    bool partition_may_offer_service;
    bool partition_settle_satisfied;
    wakeup_t *NULLABLE partition_settle_wakeup;
    wakeup_t *NULLABLE partition_post_partition_wakeup;
    wakeup_t *NULLABLE partition_pref_id_wait_wakeup;
    wakeup_t *NULLABLE partition_service_add_pending_wakeup;
    wakeup_t *NULLABLE partition_anycast_service_add_pending_wakeup;
    wakeup_t *NULLABLE service_set_changed_wakeup;
#endif // RA_TESTER
};

extern route_state_t *NONNULL route_states; // same

route_state_t *NULLABLE route_state_create(srp_server_t *NONNULL server_state, const char *NONNULL name);
void route_ula_setup(route_state_t *NULLABLE route_state);
void route_ula_generate(route_state_t *NULLABLE route_state);
bool start_route_listener(route_state_t *NULLABLE route_state);
bool start_icmp_listener(void);
void icmp_leave_join(int sock, int ifindex, bool join);
void infrastructure_network_startup(route_state_t *NULLABLE route_state);
void infrastructure_network_shutdown(route_state_t *NULLABLE route_state);
void partition_maybe_advertise_anycast_service(route_state_t *NULLABLE route_state);
void partition_stop_advertising_anycast_service(route_state_t *NULLABLE route_state, uint8_t sequence_number);
void partition_stop_advertising_pref_id(route_state_t *NULLABLE route_state);
void partition_start_srp_listener(route_state_t *NULLABLE route_state);
void partition_discontinue_srp_service(route_state_t *NULLABLE route_state);
void partition_discontinue_all_srp_service(route_state_t *NULLABLE route_state);
void partition_block_anycast_service(route_state_t *NULLABLE route_state, bool block);
void adv_ctl_add_prefix(route_state_t *NONNULL route_state, const uint8_t *NONNULL data);
void adv_ctl_remove_prefix(route_state_t *NONNULL route_state, const uint8_t *NONNULL data);
#define interface_retain(interface) interface_retain_(interface, __FILE__, __LINE__)
void interface_retain_(interface_t *NONNULL interface, const char *NONNULL file, int line);
#define interface_release(interface) interface_release_(interface, __FILE__, __LINE__)
void interface_release_(interface_t *NONNULL interface, const char *NONNULL file, int line);
void route_refresh_interface_list(route_state_t *NONNULL route_state);

void router_solicit(icmp_message_t *NONNULL message);
void router_advertisement(icmp_message_t *NONNULL message);
void neighbor_advertisement(icmp_message_t *NONNULL message);

int route_get_current_infra_interface_index(void);
#endif // __SERVICE_REGISTRATION_ROUTE_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
