/* srp-mdns-proxy.h
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
 * This file contains structure definitions used by the SRP Advertising Proxy.
 */

#ifndef __SRP_MDNS_PROXY_H__
#define __SRP_MDNS_PROXY_H__ 1

#include <stddef.h> // For ptrdiff_t
#include "ioloop-common.h" // for service_connection_t

typedef struct adv_instance adv_instance_t;
typedef struct adv_record_registration adv_record_t;
typedef struct adv_host adv_host_t;
typedef struct adv_update adv_update_t;
typedef struct client_update client_update_t;
typedef struct adv_instance_vec adv_instance_vec_t;
typedef struct adv_record_vec adv_record_vec_t;
typedef struct srpl_connection srpl_connection_t;
typedef struct srp_server_state srp_server_t;
typedef struct route_state route_state_t;
typedef struct srp_wanted_state srp_wanted_state_t; // private
typedef struct srp_xpc_client srp_xpc_client_t;     // private
typedef struct srpl_domain srpl_domain_t;           // private
typedef struct service_tracker service_tracker_t;
typedef struct thread_tracker thread_tracker_t;
typedef struct node_type_tracker node_type_tracker_t;
typedef struct service_publisher service_publisher_t;
typedef struct dns_host_description dns_host_description_t;
typedef struct service_instance service_instance_t;
typedef struct service service_t;
typedef struct delete delete_t;
typedef struct _cti_connection_t *cti_connection_t;
typedef struct dnssd_proxy_advertisements dnssd_proxy_advertisements_t;
#if SRP_FEATURE_DISCOVERY_PROXY_SERVER
typedef struct dnssd_dp_proxy_advertisements dnssd_dp_proxy_advertisements_t;
#endif
typedef struct dnssd_client dnssd_client_t;
typedef struct probe_state probe_state_t;
typedef struct srpl_instance srpl_instance_t;
typedef struct wanted_service wanted_service_t;

#ifdef SRP_TEST_SERVER
typedef struct dns_service_event dns_service_event_t;
typedef struct test_state test_state_t;
typedef struct srp_server_state srp_server_t;
typedef struct srpl_connection srpl_connection_t;
#endif

#define TSR_TIMESTAMP_STRING_LEN 28

enum {
    PRIORITY_DEFAULT = 0,
#if !defined(__OPEN__SOURCE__) && !defined(POSIX_BUILD)
    PRIORITY_HOMEPOD_ODEON = 1,
    PRIORITY_HOMEPOD_NO_ODEON = 25,
    PRIORITY_APPLETV_WIFI = 50,
    PRIORITY_APPLETV_ETHERNET = 75,
#endif
};

// Server internal state
struct srp_server_state {
#if SRP_TEST_SERVER
    srp_server_t *NULLABLE next;
#endif
    char *NULLABLE name;
    adv_host_t *NULLABLE hosts;
    dnssd_txn_t *NULLABLE shared_registration_txn;
    srpl_domain_t *NULLABLE srpl_domains;
    srpl_instance_t *NULLABLE unmatched_instances;
#ifdef SRP_TEST_SERVER
    dns_service_event_t *NULLABLE dns_service_events;
    test_state_t *NULLABLE test_state;
    comm_t *NULLABLE srpl_listener;
    srpl_connection_t *NULLABLE connections; // list of connections that other srp servers create to connect to us
    int server_id;
#endif
#if STUB_ROUTER
    route_state_t *NULLABLE route_state;
#endif
#if THREAD_DEVICE
    service_tracker_t *NULLABLE service_tracker;
    service_publisher_t *NULLABLE service_publisher;
    thread_tracker_t *NULLABLE thread_tracker;
    node_type_tracker_t *NULLABLE node_type_tracker;
    char *NULLABLE wed_ext_address; // For Wake-on End Device, the extended MAC address, if we are in p2p mode
    struct in6_addr wed_ml_eid;     // For Wake-on End Device, the ML-EID, if we are in p2p mode
#endif
    dnssd_proxy_advertisements_t *NULLABLE dnssd_proxy_advertisements;
#if SRP_FEATURE_DISCOVERY_PROXY_SERVER
    dnssd_dp_proxy_advertisements_t *NULLABLE dnssd_dp_proxy_advertisements;
#endif
    dnssd_client_t *NULLABLE dnssd_client;
    io_t *NULLABLE adv_ctl_listener;
    wakeup_t *NULLABLE srpl_browse_wakeup;
    wakeup_t *NULLABLE object_allocation_stats_dump_wakeup;
    struct in6_addr ula_prefix;
    uint64_t xpanid;
    int advertise_interface;

    uint32_t max_lease_time;
    uint32_t min_lease_time; // thirty seconds
    uint32_t key_max_lease_time;
    uint32_t key_min_lease_time; // thirty seconds
    int full_dump_count;

    uint32_t priority;
    uint16_t rloc16;

    bool have_rloc16;
    bool have_mesh_local_address;
    bool srp_replication_enabled;
    bool break_srpl_time;
#if STUB_ROUTER
    bool stub_router_enabled;
#endif
    bool srp_unicast_service_blocked;
    bool srp_anycast_service_blocked;
};

struct adv_instance {
    int ref_count;
    dnssd_txn_t *NULLABLE txn;           // The dnssd_txn_t that was created from the shared connection.
    intptr_t shared_txn;                 // The shared txn on which the txn for this instance's registration was created
    adv_host_t *NULLABLE host;           // Host to which this service instance belongs
    adv_update_t *NULLABLE update;       // Ongoing update that currently owns this instance, if any.
    wakeup_t *NULLABLE retry_wakeup;     // In case we get a spurious name conflict.
    char *NONNULL instance_name;         // Single label instance name (future: service instance FQDN)
    char *NONNULL service_type;          // Two label service type (e.g., _ipps._tcp)
    int port;                            // Port on which service can be found.
    uint8_t *NULLABLE txt_data;          // Contents of txt record
    uint16_t txt_length;                 // length of txt record contents
    message_t *NULLABLE message;         // Message that produces the current value of this instance
    ptrdiff_t recent_message;            // Most recent message (never dereference--this is for comparison only).
    int64_t lease_expiry;                // Time when lease expires, relative to ioloop_timenow().
    unsigned wakeup_interval;            // Exponential backoff interval
    bool removed;                        // True if this instance is being kept around for replication.
    bool update_pending;                 // True if we got a conflict while updating and are waiting to try again
    bool anycast;                        // True if service registration is through anycast service.
    bool skip_update;                    // True if we shouldn't actually register this instance
};

// A record registration
struct adv_record_registration {
    int ref_count;
    DNSRecordRef NULLABLE rref;            // The RecordRef we get back from DNSServiceRegisterRecord().
    adv_host_t *NULLABLE host;             // The host object to which this record refers.
    intptr_t shared_txn;                   // The shared transaction on which this record was registered.
    adv_update_t *NULLABLE update;         // The ongoing update, if any
    uint8_t *NULLABLE rdata;
    uint16_t rrtype;                       // For hosts, always A or AAAA, for instances always TXT, PTR or SRV.
    uint16_t rdlen;                        // Length of the RR
    bool update_pending;                   // True if we are updating this record and haven't gotten a response yet.
};

struct adv_host {
    int ref_count;
    srp_server_t *NULLABLE server_state;   // Server state to which this host belongs.
    wakeup_t *NONNULL re_register_wakeup;  // Wakeup for retry when we run into a name conflict
    wakeup_t *NONNULL retry_wakeup;        // Wakeup for retry when we run into a temporary failure
    wakeup_t *NONNULL lease_wakeup;        // Wakeup at least expiry time
    adv_host_t *NULLABLE next;             // Hosts are maintained in a linked list.
    adv_update_t *NULLABLE update;         // Update to this host, if one is being done
    char *NONNULL name;                    // Name of host (without domain)
    char *NULLABLE registered_name;        // The name that is registered, which may be different due to mDNS conflict
    message_t *NULLABLE message;           // Most recent successful SRP update for this host
    srpl_connection_t *NULLABLE srpl_connection; // SRP replication server that is currently updating this host.
    int name_serial;                       // The number we are using to disambiguate the name.
    adv_record_vec_t *NULLABLE addresses;  // One or more address records
    adv_record_t *NULLABLE key_record;     // Key record, if we registered that.
    adv_instance_vec_t *NULLABLE instances; // Zero or more service instances.

    dns_rr_t key;                          // The key data represented as an RR; key->name is NULL.
    uint32_t key_id;                       // A possibly-unique id that is computed across the key for brevity in
                                           // debugging
    unsigned wakeup_interval;              // Exponential backoff interval for re-registration
    int retry_interval;                    // Interval to wait before attempting to re-register after the daemon has
                                           // died.
    time_t update_time;                    // Time when the update completed.
    time_t remove_received_time;           // Time when the remove is received
    uint64_t update_server_id;             // Server ID from server that sent the update
    uint64_t server_stable_id;             // Stable ID of server that got update from client (we're using server ULA).
    uint16_t key_rdlen;                    // Length of key
    uint8_t *NULLABLE key_rdata;           // Raw KEY rdata, suitable for DNSServiceRegisterRecord().
    uint32_t lease_interval;               // Interval for address lease
    uint32_t key_lease;                    // Interval for key lease
    int64_t lease_expiry;                  // Time when lease expires, relative to ioloop_timenow().
    bool removed;                          // True if this host has been removed (and is being kept for replication)

    // True if we have a pending late conflict resolution. If we get a conflict after the update for the
    // host registration has expired, and there happens to be another update in progress, then we want
    // to defer the host registration.
    bool update_pending;
    bool re_register_pending;

};

struct adv_update {
    int ref_count;

    adv_host_t *NULLABLE host;              // Host being updated

    // Connection state, if applicable, of the client request that produced this update.
    client_update_t *NULLABLE client;
    int num_outstanding_updates;   // Total count updates that have been issued but not yet confirmed.

    // Addresses to apply to the host.  At present only one address is ever advertised, but we remember all the
    // addresses that were registered.
    adv_record_vec_t *NULLABLE remove_addresses;
    adv_record_vec_t *NULLABLE add_addresses;

    // If non-null, this update is changing the advertised address of the host to the referenced address.
    adv_record_t *NULLABLE key;

    // The set of instances from the update that already exist but have changed.
    // This array mirrors the array of instances configured on the host; entries to be updated
    // are non-NULL, entries that don't need updated are NULL.
    adv_instance_vec_t *NONNULL update_instances;

    // The set of instances that exist and need to be removed.
    adv_instance_vec_t *NONNULL remove_instances;

    // The set of instances that exist and were renewed
    adv_instance_vec_t *NONNULL renew_instances;

    // The set of instances that need to be added.
    adv_instance_vec_t *NONNULL add_instances;

    // Outstanding instance updates
    int num_instances_started;
    int num_instances_completed;

    // Outstanding record updates
    int num_records_started;
    int num_records_completed;

    // Lease intervals for host entry and key entry.
    uint32_t host_lease, key_lease;

    // If nonzero, this is an explicit expiry time for the lease, because the update is restoring
    // a host after a server restart, or else renaming a host after a late name conflict. In this
    // case, we do not want to extend the lease--just get the host registration right.
    int64_t lease_expiry;

    // The time when we started doing the update. If we get a retransmission, we can compare the current
    // to this time to see if we ought to try again.
    time_t start_time;

    // True if we are registering the key to hold the hostname.
    bool registering_key;
};

struct adv_instance_vec {
    int ref_count;
    int num;
    adv_instance_t * NULLABLE *NONNULL vec;
};

struct adv_record_vec {
    int ref_count;
    int num;
    adv_record_t * NULLABLE *NONNULL vec;
};

struct client_update {
    client_update_t *NULLABLE next;
    comm_t *NULLABLE connection;                 // Connection on which in-process update was received.
    srpl_connection_t *NULLABLE srpl_connection; // SRP replication connection on which update was received.
    dns_message_t *NULLABLE parsed_message;      // Message that triggered the update.
    message_t *NULLABLE message;                 // Message that triggered the update.

    dns_host_description_t *NULLABLE host;       // Host data parsed from message
    service_instance_t *NULLABLE instances;      // Service instances parsed from message
    service_t *NULLABLE services;                // Services parsed from message
    delete_t *NULLABLE removes;                  // Removes parsed from message
    dns_name_t *NULLABLE update_zone;            // Zone being updated
    srp_server_t *NULLABLE server_state;         // SRP server state associated with this update, for testing
    uint32_t host_lease, key_lease;              // Lease intervals for host entry and key entry.
    int index;                                   // Message number for multi-message SRP updates
    uint8_t rcode;
    bool skip;                                   // If true, this update is completely overshadowed by later updates and we
                                                 // should skip it.
    bool drop;                                   // If true, the signature on this message didn't validate and we mustn't
                                                 // send a response
    bool skip_host_updates;                      // If true, don't actually register any host records.
};

struct wanted_service {
    wanted_service_t *NULLABLE next;
    char *NULLABLE name;
    char *NULLABLE service_type;
};

// Exported functions.
bool srp_mdns_shared_registration_txn_setup(srp_server_t *NONNULL server_state);
void srp_mdns_shared_record_remove(srp_server_t *NONNULL server_state, adv_record_t *NONNULL record);
void srp_mdns_update_finished(adv_update_t *NONNULL update);
#define adv_instance_retain(instance) adv_instance_retain_(instance, __FILE__, __LINE__)
void adv_instance_retain_(adv_instance_t *NONNULL instance, const char *NONNULL file, int line);
#define adv_instance_release(instance) adv_instance_release_(instance, __FILE__, __LINE__)
void adv_instance_release_(adv_instance_t *NONNULL instance, const char *NONNULL file, int line);
#define adv_record_retain(record) adv_record_retain_(record, __FILE__, __LINE__)
void adv_record_retain_(adv_record_t *NONNULL record, const char *NONNULL file, int line);
#define adv_record_release(record) adv_record_release_(record, __FILE__, __LINE__)
void adv_record_release_(adv_record_t *NONNULL record, const char *NONNULL file, int line);
void adv_instance_context_release(void *NONNULL context);

#define srp_adv_host_release(host) srp_adv_host_release_(host, __FILE__, __LINE__)
void srp_adv_host_release_(adv_host_t *NONNULL host, const char *NONNULL file, int line);
#define srp_adv_host_retain(host) srp_adv_host_retain_(host, __FILE__, __LINE__)
void srp_adv_host_retain_(adv_host_t *NONNULL host, const char *NONNULL file, int line);
#define srp_adv_host_copy(server_state, name) srp_adv_host_copy_(server_state, name, __FILE__, __LINE__)
adv_host_t *NULLABLE srp_adv_host_copy_(srp_server_t *NONNULL server_state, dns_name_t *NONNULL name,
                                        const char *NONNULL file, int line);
int srp_current_valid_host_count(srp_server_t *NONNULL server_state);
int srp_hosts_to_array(srp_server_t *NONNULL server_state, adv_host_t *NONNULL *NULLABLE host_array, int max_hosts);
bool srp_adv_host_valid(adv_host_t *NONNULL host);
srp_server_t *NULLABLE server_state_create(const char *NONNULL name, int max_lease_time,
                                           int min_lease_time, int key_max_lease_time, int key_min_lease_time);
DNSServiceAttributeRef NULLABLE srp_adv_instance_tsr_attribute_generate(adv_instance_t *NONNULL instance,
                                                                        char *NONNULL time_buf, size_t time_buf_size);
DNSServiceAttributeRef NULLABLE srp_adv_host_tsr_attribute_generate(adv_host_t *NONNULL host,
                                                                    char *NONNULL time_buf, size_t time_buf_size);
DNSServiceAttributeRef NULLABLE srp_message_tsr_attribute_generate(message_t *NULLABLE message, uint32_t key_id,
                                                                   char *NONNULL time_buf, size_t time_buf_size);
#endif // __SRP_MDNS_PROXY_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
