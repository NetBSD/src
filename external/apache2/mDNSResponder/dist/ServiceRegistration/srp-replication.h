/* srp-replication.h
 *
 * Copyright (c) 2020-2023 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains structure definitions and external definitions for the SRP Replication code.
 */

#ifndef __SRP_REPLICATION_H__
#define __SRP_REPLICATION_H__

// States: each state has a function, which bears the name of the state. The function takes an
//         srpl_connection_t and an srpl_event_t. The function is called once on entering the
//         state, once on leaving the state, and once whenever an event arrives while in the state.
//
//         Whenever this function is called, it returns a next state. If the next state is
//         "invalid," that means that there is no state change.
//
// Events: Events can be triggered by connection activities, e.g. connect, disconnect, add_address, etc.
//         They can also be triggered by the arrival of messages on connections.
//         They can also be triggered by happenings on the srp server.
//         Events are never sent by state actions.
//         Each event has a single function which sends the event. That function may in some cases force
//         a state change (e.g., a disconnect event). This is the exception, not the rule. Events are
//         otherwise handled by the state action function for the current state.
//
// In some cases, an event that's expected to be delivered asynchronously arrives synchronously because
// no asynchronous action was required. In this case, the action that can trigger this synchronous event
// has to also handle the event. Since the event is normally expected to be delivered asynchronously,
// the right solution in this case is to queue the event for later delivery.
//
// An example of this pattern is in srpl_advertise_finished_event_defer. Because events are normally automatic
// variables, any event that needs to be deferred has to be allocated and its contents (if any) copied. The
// srpl_connection_t is stashed on the event; srpl_deferred_event_deliver is then called asynchronously to deliver the
// event, release the reference to the srpl_connection_t, and free the event data structure.

enum srpl_state {
    srpl_state_invalid = 0,  // Only as a return value, means do not move to a new state

    // Connection-related states
    srpl_state_disconnected,
    srpl_state_next_address_get,
    srpl_state_connect,
    srpl_state_idle,
    srpl_state_reconnect_wait,
    srpl_state_retry_delay_send,
    srpl_state_disconnect,
    srpl_state_disconnect_wait,
    srpl_state_connecting,

    // Session establishment
    srpl_state_session_send,
    srpl_state_session_response_wait,
    srpl_state_session_evaluate,
    srpl_state_sync_wait,

    // Requesting and getting remote candidate list
    srpl_state_send_candidates_send,
    srpl_state_send_candidates_wait,

    // Waiting for candidate to arrive
    srpl_state_candidate_check,

    // Waiting for a host to arrive after requesting it
    srpl_state_candidate_host_wait,
    srpl_state_candidate_host_prepare,
    srpl_state_candidate_host_contention_wait,
    srpl_state_candidate_host_re_evaluate,
    srpl_state_candidate_host_apply,
    srpl_state_candidate_host_apply_wait,

    // Getting request for candidate list and sending them.
    srpl_state_send_candidates_received,
    srpl_state_send_candidates_remaining_check,
    srpl_state_next_candidate_send,
    srpl_state_next_candidate_send_wait,
    srpl_state_candidate_host_send,
    srpl_state_candidate_host_response_wait,

    // When we're done sending candidates
    srpl_state_send_candidates_response_send,

    // Ready states
    srpl_state_ready,
    srpl_state_srp_client_update_send,
    srpl_state_srp_client_ack_evaluate,
    srpl_state_stashed_host_check,
    srpl_state_stashed_host_apply,
    srpl_state_stashed_host_finished,

    // States for connections received by this server
    srpl_state_session_message_wait,
    srpl_state_session_response_send,
    srpl_state_send_candidates_message_wait,

#ifdef SRP_TEST_SERVER
    // States for testing
    srpl_state_test_event_intercept,
#endif
};

enum srpl_event_type {
    srpl_event_invalid = 0,
    srpl_event_address_add,
    srpl_event_address_remove,
    srpl_event_server_disconnect,
    srpl_event_reconnect_timer_expiry,
    srpl_event_disconnected,
    srpl_event_connected,
    srpl_event_session_response_received,
    srpl_event_send_candidates_response_received,
    srpl_event_candidate_received,
    srpl_event_host_message_received,
    srpl_event_srp_client_update_finished,
    srpl_event_advertise_finished,
    srpl_event_candidate_response_received,
    srpl_event_host_response_received,
    srpl_event_session_message_received,
    srpl_event_send_candidates_message_received,
    srpl_event_do_sync,
};
enum srpl_candidate_disposition { srpl_candidate_yes, srpl_candidate_no, srpl_candidate_conflict };

typedef struct srpl_instance_service srpl_instance_service_t;
typedef struct srpl_connection srpl_connection_t;
typedef struct srpl_instance srpl_instance_t;
typedef struct srpl_domain srpl_domain_t;
typedef struct address_query address_query_t;
typedef struct unclaimed_connection unclaimed_connection_t;
typedef enum srpl_state srpl_state_t;
typedef enum srpl_event_type srpl_event_type_t;
typedef struct srpl_event srpl_event_t;
typedef struct srpl_candidate srpl_candidate_t;
typedef enum srpl_candidate_disposition srpl_candidate_disposition_t;
typedef struct srpl_srp_client_queue_entry srpl_srp_client_queue_entry_t;
typedef struct srpl_srp_client_update_result srpl_srp_client_update_result_t;
typedef struct srpl_host_update srpl_host_update_t;
typedef struct srpl_advertise_finished_result srpl_advertise_finished_result_t;
typedef struct srpl_session srpl_session_t;
#ifdef SRP_TEST_SERVER
typedef struct test_packet_state test_packet_state_t;
#endif

typedef void (*address_change_callback_t)(void *NULLABLE context, addr_t *NULLABLE address, bool added, bool more, int err);
typedef void (*address_query_cancel_callback_t)(void *NULLABLE context);
typedef enum {
    address_query_next_address_gotten, // success
    address_query_next_address_empty,  // no addresses at all
    address_query_cycle_complete       // all addresses have been tried
} address_query_result_t;

#define ADDRESS_QUERY_MAX_ADDRESSES 20
struct address_query {
    int ref_count;
    dnssd_txn_t *NULLABLE aaaa_query, *NULLABLE a_query;
    addr_t addresses[ADDRESS_QUERY_MAX_ADDRESSES]; // If there are more than this many viable addresses, too bad?
    uint32_t address_interface[ADDRESS_QUERY_MAX_ADDRESSES];
    int num_addresses, cur_address;
    address_change_callback_t NULLABLE change_callback;
    address_query_cancel_callback_t NULLABLE cancel_callback;
    void *NULLABLE context;
    char *NONNULL hostname;
};

struct srpl_candidate {
    dns_label_t *NULLABLE name;
    uint32_t key_id;                 // key id from adv_host_t
    uint32_t update_offset;          // Offset in seconds before the time candidate message was sent that update was received.
    time_t update_time;              // the time of registration received from remote
    time_t local_time;               // our time of registration when we fetched the host
    message_t *NULLABLE message;     // The SRP message.
    adv_host_t *NULLABLE host;       // the host, when it's been fetched
};

struct srpl_advertise_finished_result {
    char *NULLABLE hostname;
    srp_server_t *NULLABLE server_state;
    int rcode;
};

// 1: local > remote
// 0: local == remote
// -1: local < remote
// -2: undefined result.
enum {
    EQUAL = 0,
    LOCAL_LARGER = 1,
    LOCAL_SMALLER = -1,
    UNDEFINED = -2,
};

typedef enum {
    srpl_event_content_type_none = 0,
    srpl_event_content_type_address,
    srpl_event_content_type_session,
    srpl_event_content_type_candidate,
    srpl_event_content_type_rcode,
    srpl_event_content_type_candidate_disposition,
    srpl_event_content_type_host_update,
    srpl_event_content_type_client_result,
    srpl_event_content_type_advertise_finished_result,
} srpl_event_content_type_t;

typedef srpl_state_t (*srpl_action_t)(srpl_connection_t *NONNULL connection, srpl_event_t *NULLABLE event);

struct srpl_srp_client_update_result {
    adv_host_t *NONNULL host;
    int rcode;
};

struct srpl_host_update {
    message_t *NULLABLE *NULLABLE messages;
    intptr_t orig_buffer;
    uint64_t server_stable_id;
    dns_name_t *NULLABLE hostname;
    uint32_t update_offset;
    int num_messages, max_messages, messages_processed;
    int rcode;
    unsigned num_bytes;
};

struct srpl_session {
    uint64_t partner_id;
    dns_name_t *NULLABLE domain_name;
    uint16_t remote_version;
    bool new_partner;
};

struct srpl_event {
    char *NONNULL name;
    srpl_event_content_type_t content_type;
    union {
        addr_t address;
        srpl_session_t session;
        srpl_srp_client_update_result_t client_result;
        srpl_candidate_t *NULLABLE candidate;
        int rcode;
        srpl_candidate_disposition_t disposition;
        srpl_host_update_t host_update;
        srpl_advertise_finished_result_t advertise_finished;
    } content;
    message_t *NULLABLE message;
    srpl_event_type_t event_type;
    srpl_connection_t *NULLABLE srpl_connection; // if the event's been deferred, otherwise ALWAYS NULL.
};

struct srpl_srp_client_queue_entry {
    srpl_srp_client_queue_entry_t *NULLABLE next;
    adv_host_t *NONNULL host;
    bool sent;
};

struct srpl_connection {
    int ref_count;
    uint64_t remote_partner_id;
    char *NONNULL name;
    char *NONNULL state_name;
    comm_t *NULLABLE connection;
    const char *NULLABLE connection_null_reason; // for debugging, records why we NULLed connection.
    struct timeval connection_null_time; // When connection was set to NULL
    addr_t connected_address;
    srpl_candidate_t *NULLABLE candidate;
    dso_state_t *NULLABLE dso;
    srpl_instance_t *NULLABLE instance;
    wakeup_t *NULLABLE reconnect_wakeup;
    wakeup_t *NULLABLE state_timeout; // how long the srpl connecton could stay in a state before we assume it's gone.
    message_t *NULLABLE message;
    adv_host_t *NULLABLE *NULLABLE candidates;
    srpl_host_update_t stashed_host;
    srpl_srp_client_queue_entry_t *NULLABLE client_update_queue;
    wakeup_t *NULLABLE keepalive_send_wakeup;
    wakeup_t *NULLABLE keepalive_receive_wakeup;
#ifdef SRP_TEST_SERVER
    void (*NULLABLE advertise_finished_callback)(test_state_t *NONNULL state);
    void (*NULLABLE srpl_advertise_finished_callback)(srpl_connection_t *NONNULL connection);
    test_state_t *NULLABLE test_state;
    srpl_connection_t *NULLABLE next;
    srp_server_t *NONNULL server;
#endif
    time_t last_message_sent;
    time_t last_message_received;
    time_t state_start_time;
    int num_candidates;
    int current_candidate;
    int retry_delay; // How long to send when we send a retry_delay message
    int keepalive_interval;
    srpl_state_t state, next_state;
    uint32_t variation_mask; // Protocol variations to support pre-standard TLV formats
    bool is_server;
    bool new_partner;
    bool database_synchronized;
    bool candidates_not_generated; // If this is true, we haven't generated a candidates list yet.
};

struct srpl_instance_service {
    int ref_count;
    srpl_instance_t *NULLABLE instance;
    dnssd_txn_t *NULLABLE txt_txn;
    dnssd_txn_t *NULLABLE srv_txn;
    srpl_instance_service_t *NULLABLE next;
    srpl_domain_t *NONNULL domain;
    wakeup_t *NULLABLE resolve_wakeup;
    wakeup_t *NULLABLE discontinue_timeout;
    uint8_t *NULLABLE txt_rdata;
    uint8_t *NULLABLE srv_rdata;
    uint8_t *NULLABLE ptr_rdata;
    uint8_t *NULLABLE addr_rdata;
    char *NULLABLE host_name;
    char *NULLABLE full_service_name;
    address_query_t *NULLABLE address_query;
    int num_copies;     // Tracks adds and deletes from the DNSServiceBrowse for this instance.
    uint32_t ifindex;
    uint16_t outgoing_port;
    uint16_t txt_length;
    uint16_t srv_length;
    uint16_t ptr_length;
    bool have_srv_record, have_txt_record;
    // True if we've already started a resolve for this instance, to prevent starting a second resolve if the instance
    // is seen on more than one interface.
    bool resolve_started;
    bool discontinuing; // True if we are in the process of discontinuing this instance.
    bool got_new_info; // True if we have received new information since the last time we did a reconfirm.
};

struct srpl_instance {
    int ref_count;
    srpl_instance_t *NULLABLE next;
    srpl_domain_t *NONNULL domain;
    srpl_connection_t *NULLABLE connection;
    wakeup_t *NULLABLE reconnect_timeout;
    char *NULLABLE instance_name;
    srpl_instance_service_t *NONNULL services;
    uint64_t partner_id;
    uint64_t dataset_id;
    uint32_t priority;
    bool have_partner_id;
    bool have_dataset_id;
    bool have_priority;
    bool sync_to_join;  // True if sync with the remote partner is required to join the replication
    bool sync_fail;     // True if sync with the remote partner is declared fail
    bool discovered_in_window; // True if the instance is discovered in partner discovery window
    bool is_me;
    bool discontinuing; // True if we are in the process of discontinuing this instance.
    bool unmatched; // True if this is an incoming connection that hasn't been associated with a real instance.
    bool matched_unidentified; // True if an address from address callback matches an unidentified connection
    bool added_address; // True if address callback adds an address to the instance
    bool version_mismatch; // True if the version mismatches
};

typedef enum {
    SRPL_OPSTATE_STARTUP = 0,
    SRPL_OPSTATE_ROUTINE = 1
} srpl_opstate_t;

struct srpl_domain {
    uint64_t partner_id; // SRP replication partner ID
    uint64_t dataset_id;
    bool have_dataset_id;
    bool dataset_id_committed;
    bool partner_discovery_pending;
    int ref_count;
    srpl_opstate_t srpl_opstate;
    srpl_domain_t *NULLABLE next;
    char *NONNULL name;
    srpl_instance_t *NULLABLE instances;
    srpl_instance_service_t *NULLABLE unresolved_services;
    dnssd_txn_t *NULLABLE query;
    srp_server_t *NULLABLE server_state;
    dnssd_txn_t *NULLABLE srpl_advertise_txn;
    wakeup_t *NULLABLE srpl_register_wakeup;
    wakeup_t *NULLABLE partner_discovery_timeout;
};

#define SRP_THREAD_DOMAIN "thread.home.arpa."

#define DSO_TLV_HEADER_SIZE 4 // opcode (u16) + length (u16)
#define DSO_MESSAGE_MIN_LENGTH DNS_HEADER_SIZE + DSO_TLV_HEADER_SIZE + 1


#define SRPL_RETRY_DELAY_LENGTH        DSO_MESSAGE_MIN_LENGTH + sizeof(uint32_t)
#define SRPL_SESSION_MESSAGE_LENGTH    (DSO_MESSAGE_MIN_LENGTH +                  \
                                        sizeof(uint64_t) +                        \
                                        DNS_MAX_NAME_SIZE + DSO_TLV_HEADER_SIZE + \
                                        DSO_TLV_HEADER_SIZE + sizeof(uint16_t)  + \
                                        DSO_TLV_HEADER_SIZE + sizeof(uint16_t))
#define SRPL_SEND_CANDIDATES_LENGTH    DSO_MESSAGE_MIN_LENGTH
#define SRPL_CANDIDATE_MESSAGE_LENGTH  (DSO_MESSAGE_MIN_LENGTH + \
                                        DNS_MAX_NAME_SIZE + DSO_TLV_HEADER_SIZE + \
                                        sizeof(uint32_t) + DSO_TLV_HEADER_SIZE + \
                                        sizeof(uint32_t) + DSO_TLV_HEADER_SIZE)
#define SRPL_KEEPALIVE_MESSAGE_LENGTH  (DSO_MESSAGE_MIN_LENGTH + \
                                        DNS_MAX_NAME_SIZE + DSO_TLV_HEADER_SIZE + \
                                        sizeof(uint32_t) + DSO_TLV_HEADER_SIZE + \
                                        sizeof(uint32_t) + DSO_TLV_HEADER_SIZE)
#define SRPL_CANDIDATE_RESPONSE_LENGTH DSO_MESSAGE_MIN_LENGTH + DSO_TLV_HEADER_SIZE
#define SRPL_HOST_MESSAGE_LENGTH  (DSO_MESSAGE_MIN_LENGTH + \
                                   DNS_MAX_NAME_SIZE + DSO_TLV_HEADER_SIZE + \
                                   sizeof(uint32_t) + DSO_TLV_HEADER_SIZE + \
                                   sizeof(uint32_t) + DSO_TLV_HEADER_SIZE)
#define SRPL_HOST_RESPONSE_LENGTH      DSO_MESSAGE_MIN_LENGTH

#define SRPL_UPDATE_JITTER_WINDOW 10

#define MIN_PARTNER_DISCOVERY_INTERVAL 4000  // minimum partner discovery time interval in milliseconds
#define MAX_PARTNER_DISCOVERY_INTERVAL 7500  // maximum partner discovery time interval in milliseconds
#define PARTNER_DISCOVERY_INTERVAL_RANGE (MAX_PARTNER_DISCOVERY_INTERVAL - \
                                          MIN_PARTNER_DISCOVERY_INTERVAL + 1)
#define DEFAULT_KEEPALIVE_WAKEUP_EXPIRY (5 * 60 * 1000) // five minutes

#define PARTNER_ID_BITS 64
#define LOWER56_BIT_MASK 0xFFFFFFFFFFFFFFULL

// SRP Replication protocol versioning
// Protocol version number 1: outdated and no longer being used. This version was supposed to
//                            support multi host messages but did not really work. After making
//                            it work, we increment the version number to 3.
// Protocol version number 2: to support anycast service
// Protocol version number 3: to support multi host messages
#define SRPL_VERSION_ANYCAST                    2
#define SRPL_VERSION_MULTI_HOST_MESSAGE         3
#define SRPL_VERSION_EDNS0_TSR                  4
#define SRPL_CURRENT_VERSION                    SRPL_VERSION_EDNS0_TSR

// Variation bits.
#define SRPL_VARIATION_MULTI_HOST_MESSAGE   1
#define SRPL_SUPPORTS(srpl_connection, variation) \
    (((srpl_connection)->variation_mask & (variation)) != 0)

// Exported functions...
srpl_connection_t *NULLABLE srpl_connection_create(srpl_instance_t *NONNULL instance, bool outgoing);
void srpl_connection_next_state(srpl_connection_t *NONNULL srpl_connection, srpl_state_t state);
void srpl_startup(srp_server_t *NONNULL srp_server);
void srpl_shutdown(srp_server_t *NONNULL server_state);
void srpl_disable(srp_server_t *NONNULL srp_server);
void srpl_drop_srpl_connection(srp_server_t *NONNULL srp_server);
void srpl_undrop_srpl_connection(srp_server_t *NONNULL srp_server);
void srpl_drop_srpl_advertisement(srp_server_t *NONNULL srp_server);
void srpl_undrop_srpl_advertisement(srp_server_t *NONNULL srp_server);
void srpl_dso_server_message(comm_t *NONNULL connection, message_t *NULLABLE message, dso_state_t *NONNULL dso,
                             srp_server_t *NONNULL server_state);
void srpl_advertise_finished_event_send(char *NONNULL host, int rcode, srp_server_t *NONNULL server_state);
void srpl_srp_client_update_finished_event_send(adv_host_t *NONNULL host, int rcode);
#define srpl_connection_release(connection) srpl_connection_release_(connection, __FILE__, __LINE__)
void srpl_connection_release_(srpl_connection_t *NONNULL srpl_connection, const char *NONNULL file, int line);
#define srpl_connection_retain(connection) srpl_connection_retain_(connection, __FILE__, __LINE__)
void srpl_connection_retain_(srpl_connection_t *NONNULL srpl_connection, const char *NONNULL file, int line);
srpl_domain_t *NULLABLE srpl_domain_create_or_copy(srp_server_t *NONNULL server_state, const char *NONNULL domain_name);
void srpl_dump_connection_states(srp_server_t *NONNULL server_state);
void srpl_change_server_priority(srp_server_t *NONNULL server_state, uint32_t new);
#endif // __SRP_REPLICATION_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
