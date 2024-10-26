/* dnssd-proxy.c
 *
 * Copyright (c) 2018-2024 Apple Inc. All rights reserved.
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
 * This is a Discovery Proxy module for the SRP gateway.
 *
 * The motivation here is that it makes sense to co-locate the SRP relay and the Discovery Proxy because
 * these functions are likely to co-exist on the same node, listening on the same port.  For homenet-style
 * name resolution, we need a DNS proxy that implements DNSSD Discovery Proxy for local queries, but
 * forwards other queries to an ISP resolver.  The SRP gateway is already expecting to do this.
 * This module implements the functions required to allow the SRP gateway to also do Discovery Relay.
 *
 * The Discovery Proxy relies on Apple's DNS-SD library and the mDNSResponder DNSSD server, which is included
 * in Apple's open source mDNSResponder package, available here:
 *
 *            https://opensource.apple.com/tarballs/mDNSResponder/
 */

#ifndef __APPLE_USE_RFC_3542
    #define __APPLE_USE_RFC_3542
#endif // #ifndef __APPLE_USE_RFC_3542

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>
#include <notify.h>
#ifdef IOLOOP_MACOS
#include <AssertMacros.h>
#include <SystemConfiguration/SystemConfiguration.h>
#endif // #ifdef IOLOOP_MACOS

#include "dns_sd.h"
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "dso-utils.h"
#include "dso.h"
#include "srp-tls.h"
#include "config-parse.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "srp-tls.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "cti-services.h"
#include "route.h"
#include "srp-replication.h"
#if THREAD_DEVICE
#  include "state-machine.h"
#  include "service-publisher.h"
#endif
#if SRP_FEATURE_NAT64
#include "dns_sd_private.h"
#include "nat64-macos.h"
#endif
#include "advertising_proxy_services.h"
#include "srp-dnssd.h"

#define RESPONSE_WINDOW_MSECS 800
#define RESPONSE_WINDOW_USECS (RESPONSE_WINDOW_MSECS * 1000) // 800ms in microseconds.

extern srp_server_t *srp_servers;
#define SERIAL(x) ((x) == NULL ? 0 : (x)->serial)

// When do we build dnssd-proxy?
// 1. When we are integrating dnssd-proxy into srp-mdns-proxy: SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY == 1
// 2. When we are building standalone dnssd-proxy: !defined(BUILD_SRP_MDNS_PROXY) || (BUILD_SRP_MDNS_PROXY == 0)
// When do we not build dnssd-proxy?
// 3. When we are building srp-mdns-proxy without dnssd-proxy: other than the two cases above.
#if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY) || (!defined(BUILD_SRP_MDNS_PROXY) || (BUILD_SRP_MDNS_PROXY == 0))

// Enumerate the list of interfaces, map them to interface indexes, give each one a name
// Have a tree of subdomains for matching

// Structures

typedef struct interface_addr interface_addr_t;
struct interface_addr {
    interface_addr_t *next;
    addr_t addr, mask;
};

typedef struct dp_interface dp_interface_t;
struct dp_interface {
    int ifindex;                            // The interface index (for use with sendmsg() and recvmsg().
    bool no_push;                           // If true, don't set up DNS Push for this domain
    char *NONNULL name;                     // The name of the interface
    interface_addr_t *NULLABLE addresses;   // Addresses on this interface.
};

typedef struct hardwired hardwired_t;
struct hardwired {
    hardwired_t *NULLABLE next;
    uint16_t type;
    char *NONNULL name;
    char *NONNULL fullname;
    uint8_t *NULLABLE rdata;
    uint16_t rdlen;
};

typedef struct question question_t;
typedef struct answer answer_t;
typedef struct served_domain served_domain_t;
struct served_domain {
    served_domain_t *NULLABLE next;             // Active configurations, used for identifying a domain that matches
    char *NONNULL domain;                       // The domain name of the interface, represented as a text string.
    char *NONNULL domain_ld;                    // The same name, with a leading dot (if_domain_lp == if_domain + 1)
    dns_name_t *NONNULL domain_name;            // The domain name, parsed into labels.
    hardwired_t *NULLABLE hardwired_responses;  // Hardwired responses for this interface.
    dp_interface_t *NULLABLE interface;         // Interface to which this domain applies (may be NULL).
    question_t *NULLABLE questions;             // Questions that have been asked in the served domain.
};

// There are two ways that a dnssd_query_t can be created. One is that a DNS datagram comes in that's a DNS
// query.  In this case, we create the query, ask the question, generate a single DNS response, send it, and
// the dnssd query is finished. We could optimize retransmissions, but currently do not. UDP queries can
// happen either over a TCP connection or a UDP connection--the behavior is the same in either case.  The
// other way is that it can be a DNS Push subscribe. A DNS Push subscribe query is finished either when the
// connection dies, or when we get a corresponding unsubscribe.

// For DNS Push queries, there is an "activity" object which tracks a particular subscription. Each activity
// can hold a reference to the comm_t. A disconnect from the comm_t should cancel all activities. Otherwise,
// the lifecycle of activities should not affect the comm_t.

// For DNS queries, a disconnect should cancel every query associated with the connection. This is complicated
// by the fact that we are not tracking outstanding queries--the query survives because the dnssd_txn_t object
// holds a reference to it; when the txn_t goes away, the connection will have no remaining references.

// In order to make this work, dnssd_query_t objects are tracked per connection. Dnssd_query_t objects do not
// hold a reference to their comm_t, but rather to their tracker.

// When a connection drops, the tracker object gets a disconnect callback, which triggers it to cancel out any
// remaining DNS transactions, and to cancel the associated DSO object. Cancelling the DSO object cancels all
// of the activities on that object; each activity is a dnssd_query_t object. Consequently, the tracker does
// not directly track DSO activities.

// In order to follow RFC 7766, the tracker keeps an idle timer going. If no DNS messages have been received
// on a connection for that amount of time, the tracker closes the connection; when the disconnect event arrives,
// the tracker is collected. If there is a DSO object on the tracker, the tracker is not responsible for tracking
// idle state.

typedef enum {
    dp_tracker_session_none,
    dp_tracker_session_push,
    dp_tracker_session_srpl
} dp_tracker_session_type_t;

int cur_tracker_serial;
typedef struct dnssd_query dnssd_query_t;
typedef struct dp_tracker {
    int ref_count;
    int serial;
    comm_t *connection;
    dnssd_query_t *dns_queries;
    dso_state_t *dso;
    wakeup_t *idle_timeout;
    dp_tracker_session_type_t session_type;
} dp_tracker_t;

struct answer {
    answer_t *next;                 // List of answers to a question.
    char *fullname;                 // Name returned in callback.
    uint8_t *rdata;                 // Rdata returned in callback (wire format).
    uint32_t interface_index;       // Interface index returned in callback.
    uint32_t ttl;                   // Time-to-live returned in callback (probably not useful for LLQ).
    uint16_t rrtype;                // Resource record type returned in callback.
    uint16_t rrclass;               // Resource record class returned in callback.
    uint16_t rdlen;                 // Length of resource record data returned in callback.
};

static int cur_question_serial;
struct question {
    question_t *next;               // List of questions that are being asked.
    served_domain_t *served_domain;
    dnssd_query_t *queries;         // dnssd queries that are asking this question.
    dnssd_txn_t *txn;               // Subordinate DNSServiceRef for this question
    char *name;                     // The name we are looking up.
    answer_t *answers;              // Answers this question has received.
    int64_t start_time;             // When this question was started.
    int serviceFlags;               // Service flags to use with this question.
    int ref_count;                  // Reference count.
    int serial;
    uint32_t interface_index;       // Which interface the query should use.
    uint16_t type;                  // The type.
    uint16_t qclass;                // The class.
    bool no_data;                   // True if "no such record" is received or all data gets removed
};

static int cur_query_serial;
struct dnssd_query {
    int ref_count;
    int serial;
    dp_tracker_t *tracker;          // Tracks the connection that delivered this query.
    dnssd_query_t *next;            // For DNS queries, tracks other queries on the same connection, if any.
    wakeup_t *wakeup;
    dns_name_pointer_t enclosing_domain_pointer;

    message_t *message;
    dso_state_t *dso;               // If this is a DNS Push query, the DSO state associated with it.
    dso_activity_t *activity;
    int num_questions;              // In case of a multi-question query, how many questions were asked
    bool is_edns0;
    dns_towire_state_t towire;
    uint8_t *p_dso_length;          // Where to store the DSO length just before we write out a push notification.
    dns_wire_t *response;
    dns_message_t *response_msg;    // In case we need to decompose the message to construct a multi-answer message.
    size_t data_size;               // Size of the data payload of the response.
    dnssd_query_t *question_next;   // Linked list of queries on the question this query is subscribed to.
    question_t *question;           // Question asked by this query pointing to a cache entry.
    bool satisfied;                 // If true, this query has gotten an answer. Only relevant for straight DNS.
    bool canceled;                  // If true, dnssd_query_cancel has been called on this query, so don't cancel it again.
};

// Structure that is used to setup the mDNS discovery for dnssd-proxy.
struct dnssd_proxy_advertisements {
    wakeup_t *wakeup_timer;         // Used to setup a timer to advertise records repeatedly until it succeeds.
    dnssd_txn_t *txn;               // Contains event loop.
    DNSServiceRef service_ref;      // Shared DNSServiceRef for all registering operation.
    DNSRecordRef ns_record_ref;     // Used to update the advertised NS record.
    DNSRecordRef ptr_record_ref;    // Used to update the advertised PTR record.
    char *domain_to_advertise;      // The domain to be advertised in NS and PTR records.
    srp_server_t *server_state;
    SCDynamicStoreContext sc_context;
};

#if SRP_FEATURE_DISCOVERY_PROXY_SERVER
// Structure that is used to advertise the push discovery service in .local domain.
struct dnssd_dp_proxy_advertisements {
    wakeup_t *wakeup_timer;     // Used to setup a timer to advertise push service repeatedly until it succeeds.
    dnssd_txn_t *txn;           // The event loop.
    DNSServiceRef service_ref;  // DNSServiceRef to register the service.
    srp_server_t *server_state;
};
#endif

// Configuration file settings

uint16_t dnssd_proxy_udp_port;
uint16_t dnssd_proxy_tcp_port;
uint16_t dnssd_proxy_tls_port;
const char *my_name = "discoveryproxy.home.arpa.";
char *listen_addrs[MAX_ADDRS];
int num_listen_addrs;
char *publish_addrs[MAX_ADDRS];
int num_publish_addrs;
char *tls_cacert_filename;
char *tls_cert_filename = "/etc/dnssd-proxy/server.crt";
char *tls_key_filename = "/etc/dnssd-proxy/server.key";

comm_t *dnssd_proxy_listeners[4 + MAX_ADDRS];
int dnssd_proxy_num_listeners;
question_t *questions_without_domain; // Questions that aren't in a served domain
served_domain_t *served_domains;
int num_push_sessions; // Number of connections from DNS Push clients
int dp_num_outstanding_queries;
int num_push_sessions_dropped_for_load;
int num_queries_dropped_for_load;

#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
dnssd_txn_t *shared_discovery_txn;
#endif // SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
wakeup_t *discovery_restart_wakeup;

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static char uuid_name[DNS_MAX_NAME_SIZE + 1];
static char my_name_buf[DNS_MAX_NAME_SIZE + 1];
static CFStringRef sc_dynamic_store_key_host_name;
static char local_host_name[DNS_MAX_NAME_SIZE + 1];
static char local_host_name_dot_local[DNS_MAX_NAME_SIZE + 1];
#endif // #if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)

#if THREAD_BORDER_ROUTER && SRP_FEATURE_SRP_COMBINED_DNSSD_PROXY
extern char *thread_interface_name;
#endif // THREAD_BORDER_ROUTER && SRP_FEATURE_COMBINED_DNSSD_PROXY

// Globals

const char push_subscription_activity_type[] = "push subscription";

static const char local_suffix[] = ".local.";

bool tls_fail = false; // Command line argument, for testing.

// Macros

#define THREAD_DOMAIN "thread.home.arpa."
// "openthread." will change in the future once we have a way to get thread network ID
#define THREAD_DOMAIN_WITH_ID "openthread." THREAD_DOMAIN
#define DEFAULT_SERVICE_ARPA_DOMAIN "default.service.arpa."
#define HOME_NET_DOMAIN "home.arpa."
#define DOT_HOME_NET_DOMAIN ".home.arpa."
#define DOT_LOCAL_DOMAIN "local."
#define LOCAL "local."
#define DOT_LOCAL ".local."
#define IPV4_REVERSE_LOOKUP_DOMAIN "in-addr.arpa."
#define IPV6_REVERSE_LOOKUP_DOMAIN "ip6.arpa."
#define SRV_TYPE_FOR_AUTOMATIC_BROWSING_DOMAIN "lb._dns-sd._udp"
#define TOWIRE_CHECK(note, towire, func) { func; if ((towire)->error != 0 && failnote == NULL) failnote = (note); }
#define BUSY_RETRY_DELAY_MS (5 * 60 * 1000) // Five minutes.
#define MAX_DSO_CONNECTIONS 15 // Should be enough for a typical home network, assuming more hosts -> more BRs

// RFC8766 says for us to clamp the TTL on proxied mDNS records to 10s. In practice this appears to be much
// too short, because if the DNSSD server (e.g. mDNSResponder) is doing an LLQ, this results in a refresh
// interval of <10s, which is kind of painful.
#define RFC8766_TTL_CLAMP 300

#define VALIDATE_TRACKER_CONNECTION_NON_NULL()                                                            \
    do {                                                                                                  \
        if (query->tracker == NULL) {                                                                     \
            ERROR("[Q%d] query->tracker NULL for query!", SERIAL(query));                                 \
            return;                                                                                       \
        }                                                                                                 \
        if (query->tracker->connection == NULL) {                                                         \
            ERROR("[Q%d][TRK%d] query->tracker->connection NULL", SERIAL(query), SERIAL(query->tracker)); \
            return;                                                                                       \
        }                                                                                                 \
    } while (false)

#ifdef SRP_TEST_SERVER
extern void (*srp_test_dso_message_finished)(void *context, message_t *message, dso_state_t *dso);
extern ready_callback_t srp_test_dnssd_tls_listener_ready;
extern void *srp_test_tls_listener_context;
#endif

// Forward references

static void dns_push_start(dnssd_query_t *query);

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static void served_domain_free(served_domain_t *const served_domain);
#endif

static served_domain_t *NULLABLE
new_served_domain(dp_interface_t *const NULLABLE interface, const char * NONNULL domain);

#if STUB_ROUTER
static served_domain_t *NULLABLE
find_served_domain(const char *const NONNULL domain);

static bool
string_ends_with(const char *const NONNULL str, const char *const NONNULL suffix);
#endif // STUB_ROUTER

static void dp_query_towire_reset(dnssd_query_t *query);

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static served_domain_t *NONNULL
add_new_served_domain_with_interface(const char *const NONNULL name,
    const addr_t *const NULLABLE address, const addr_t *const NULLABLE mask);;

#if STUB_ROUTER
static bool
dnssd_hardwired_add_or_remove_address_in_domain(const char *const NONNULL name,
    const char *const NONNULL domain_to_change, const addr_t *const NONNULL address, const bool add);
#endif // STUB_ROUTER

static bool
dnssd_hardwired_setup_dns_push_for_domain(served_domain_t *const NONNULL served_domain);
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

#if STUB_ROUTER
static bool
start_timer_to_advertise(dnssd_proxy_advertisements_t *NONNULL context,
    const char *const NULLABLE domain_to_advertise, const uint32_t interval);
#endif

static bool
interface_process_addr_change(dp_interface_t *const NONNULL interface, const addr_t *const NONNULL address,
                              const addr_t *const NONNULL mask, const enum interface_address_change event_type);

static void dns_question_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t UNUSED interfaceIndex,
                                  DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype,
                                  uint16_t rrclass, uint16_t rdlen, const void *rdata, uint32_t ttl, void *context);
static void dns_push_callback(void *context, void *event_context, dso_state_t *dso, dso_event_type_t eventType);
#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
static void dp_setup_shared_discovery_txn(void);
#endif
static void dp_query_reply_from_cache(question_t *question, dnssd_query_t *query, bool remove);
static void dp_handle_server_disconnect(void *UNUSED context, int status);
static void dp_question_answers_free(question_t *question);
static void question_finalize(question_t *question);

// For debugging
static wakeup_t *connection_dropper;
extern dso_state_t *dso_connections;
static void dp_drop_connections(void *UNUSED context);
#ifdef SRP_TEST_SERVER
served_domain_t *NULLABLE last_freed_domain;
#endif

static void
dp_question_context_release(void *context)
{
    question_t *question = context;
    RELEASE_HERE(question, question);
}

static DNSServiceErrorType
dp_start_question(question_t *question, bool dns64)
{
    DNSServiceErrorType err;
    DNSServiceRef sdref;
    size_t len;
    char name[DNS_MAX_NAME_SIZE + 1];
    char *np;

    // If a query has a served domain, query->question->name is the subdomain of the served domain that is
    // being queried; otherwise query->question->name is the whole name.
    if (question->served_domain != NULL) {
        len = strlen(question->name);
        if (question->served_domain->interface != NULL) {
            if (len + sizeof local_suffix > sizeof name) {
                ERROR("[QU%d] question name %s is too long for .local.", SERIAL(question), name);
                return kDNSServiceErr_BadParam;
            }
            memcpy(name, question->name, len);
            memcpy(&name[len], local_suffix, sizeof local_suffix);
        } else {
            size_t dlen = strlen(question->served_domain->domain_ld) + 1;
            if (len + dlen > sizeof name) {
                ERROR("[QU%d] question name %s is too long for %s.", SERIAL(question), name, question->served_domain->domain);
                return kDNSServiceErr_BadParam;
            }
            memcpy(name, question->name, len);
            memcpy(&name[len], question->served_domain->domain_ld, dlen);
        }
        np = name;
    } else {
        np = question->name;
    }

    int shared_connection_flag = 0;
#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
    dp_setup_shared_discovery_txn();
    if (shared_discovery_txn != NULL) {
        sdref = shared_discovery_txn->sdref;
        shared_connection_flag = kDNSServiceFlagsShareConnection;
    }
#endif // SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS

    uint32_t question_interface = question->interface_index;
#if SRP_FEATURE_LOCAL_DISCOVERY
    if (question_interface == kDNSServiceInterfaceIndexInfra) {
        int ret = -1;
#if STUB_ROUTER
        ret = route_get_current_infra_interface_index();
#else
        static unsigned en0_ifindex = 0;
        if (en0_ifindex == 0) {
            en0_ifindex = if_nametoindex("en0");
            if (en0_ifindex == 0) {
                ERROR("getting en0 ifindex failed!");
            }
        }
        if (en0_ifindex != 0) {
            ret = en0_ifindex;
        }
#endif // STUB_ROUTER
        // If we don't have an infrastructure interface, refuse the query.
        if (ret < 0) {
            return kDNSServiceErr_Refused;
        }
        question_interface = ret;
    }
#endif // SRP_FEATURE_LOCAL_DISCOVERY

#if SRP_FEATURE_NAT64
    const DNSServiceAttribute *attr = NULL;
    if (dns64 && (question->type == dns_rrtype_aaaa) && (question->qclass == dns_qclass_in)) {
        attr = &kDNSServiceAttributeAAAAFallback;
    }
    err = dns_service_query_record_wa(srp_servers, &sdref, question->serviceFlags | shared_connection_flag,
                                      question_interface, np, question->type,
                                      question->qclass, attr, dns_question_callback, question);
#else
    (void)dns64;
    err = dns_service_query_record(srp_servers, &sdref, question->serviceFlags | shared_connection_flag, question_interface, np,
                                   question->type, question->qclass, dns_question_callback, question);
#endif
    if (err != kDNSServiceErr_NoError) {
        ERROR("[QU%d] DNSServiceQueryRecord failed for '%s': %d", SERIAL(question), np, err);
    } else {
        INFO("[QU%d] txn %p new sdref %p", SERIAL(question), question->txn, sdref);
#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTION
        question->txn = ioloop_dnssd_txn_add_subordinate(sdref, dp_question_context_release, NULL);
#else
        question->txn = dns_service_ioloop_txn_add(srp_servers, sdref, question, dp_question_context_release, dp_handle_server_disconnect);
#endif // SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
        RETAIN_HERE(question, question); // For the callback
#if SRP_FEATURE_NAT64
        INFO("[QU%d] DNSServiceQueryRecordWithAttribute started for '" PRI_S_SRP "': %d", SERIAL(question), np, err);
#else
        INFO("[QU%d] DNSServiceQueryRecord started for '" PRI_S_SRP "': %d", SERIAL(question), np, err);
#endif // SRP_FEATURE_NAT64
    }
    return err;
}

static bool
dp_iterate_questions_on_list(question_t *list, bool (*callback)(question_t *question, void *context), void *context)
{
    for (question_t *question = list; question; question = question->next) {
        if (callback(question, context)) {
            return true;
        }
    }
    return false;
}

static bool
dp_iterate_questions(bool (*callback)(question_t *question, void *context), void *context)
{
    if (dp_iterate_questions_on_list(questions_without_domain, callback, context)) {
        return true;
    }

    for (served_domain_t *domain = served_domains; domain != NULL; domain = domain->next) {
        if (dp_iterate_questions_on_list(domain->questions, callback, context)) {
            return true;
        }
    }
    return false;
}

static bool
dp_restart_question(question_t *question, void *context)
{
    bool dns64 = *((bool *)context);
    if (question->txn == NULL) {
        dp_start_question(question, dns64);
    }
    return false;
}

static void
dp_restart_all_questions(void *UNUSED context)
{
    bool dns64 = false;
#if SRP_FEATURE_NAT64
    if (srp_servers->srp_nat64_enabled) {
        dns64 = nat64_is_active();
    }
#endif
    dp_iterate_questions(dp_restart_question, &dns64);
}

static bool
dp_void_question(question_t *question, void *UNUSED context)
{
    if (question->txn != NULL) {
        INFO("[QU%d] question->txn = %p", SERIAL(question), question->txn);
#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
        question->txn->sdref = NULL;
#else
        ioloop_dnssd_txn_cancel(question->txn);
#endif
        ioloop_dnssd_txn_release(question->txn);
        question->txn = NULL;
    }
    if (question->answers != NULL) {
        dnssd_query_t *next, *query = question->queries;
        while(query != NULL) {
            next = query->question_next;
            if (query->dso != NULL) {
                dp_query_reply_from_cache(question, query, true);
            }
            query = next;
        }
        dp_question_answers_free(question);
    }

    return false;
}

// NULLs out all outstanding questions (after an mDNSResponder crash). These pointers are rendered invalid when the
// parent transaction is deallocated, so this should not result in any leaks.
static void
dp_void_all_questions(void)
{
    dp_iterate_questions(dp_void_question, NULL);
}

static void
dp_handle_server_disconnect(void *UNUSED context, int status)
{
    INFO("status %d", status);
    dp_void_all_questions();
    if (discovery_restart_wakeup == NULL) {
        discovery_restart_wakeup = ioloop_wakeup_create();
    }
    if (discovery_restart_wakeup != NULL) {
        // Try to reconnect to mDNSResponder after a second.
        ioloop_add_wake_event(discovery_restart_wakeup, NULL, dp_restart_all_questions, NULL, 1000);
    }
}

#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
static void
dp_setup_shared_discovery_txn(void)
{
    if (shared_discovery_txn == NULL) {
        DNSServiceRef sdref;
        int err = DNSServiceCreateConnection(&sdref);
        if (err != kDNSServiceErr_NoError) {
            return false;
        }
        shared_discovery_txn = ioloop_dnssd_txn_add(sdref, NULL, NULL, dp_handle_server_disconnect);
        if (shared_discovery_txn == NULL) {
            ERROR("unable to create shared connection for registration.");
            DNSServiceRefDeallocate(sdref);
            return false;
        }
        INFO("shared_discovery_txn = %p  sdref = %p", shared_discovery_txn, sdref);
    }
}
#endif // SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS

void
dp_start_dropping(void)
{
    if (connection_dropper == NULL) {
        connection_dropper = ioloop_wakeup_create();
        if (connection_dropper == NULL) {
            ERROR("can't create connection dropper.");
            return;
        }
    }
    ioloop_add_wake_event(connection_dropper, NULL, dp_drop_connections, NULL, 90 * 1000);
}

static void
dp_drop_connections(void *UNUSED context)
{
    for (dso_state_t *dso = dso_connections; dso != NULL; dso = dso->next) {
        if (dso->cb == dns_push_callback) {
            dp_tracker_t *tracker = dso->context;
            INFO("dropping connection for " PRI_S_SRP ".", dso->remote_name);
            if (tracker->connection != NULL) {
                ioloop_comm_cancel(tracker->connection);
            }
        }
    }
    dp_start_dropping();
}

static void
dp_tracker_finalize(dp_tracker_t *tracker)
{
    // At this point tracker should have nothing attached to it that we need to get rid of, except maybe the
    // wakeup timer.
    if (tracker->idle_timeout != NULL) {
        ioloop_wakeup_release(tracker->idle_timeout);
    }
    // The only case where tracker->connection should still exist at this point is when the connection turned
    // out to be an srp replication connection.
    if (tracker->connection) {
        ioloop_comm_release(tracker->connection);
    }
    free(tracker);
}

static void
dp_answer_free(answer_t *answer)
{
    if (answer != NULL) {
        free(answer->fullname);
        free(answer);
    }
}

static void
dp_question_answers_free(question_t *question)
{
    // De-allocate answers
    answer_t *answer = question->answers;
    answer_t *next;
    while (answer != NULL) {
        next = answer->next;
        dp_answer_free(answer);
        answer = next;
    }
    question->answers = NULL;
}

// The finalize function will deallocate answers associated with the question,
// remove question from the question list and deallocate the question.
static void
question_finalize(question_t *question)
{
    INFO("[QU%d] type %d class %d " PRI_S_SRP, SERIAL(question), question->type, question->qclass, question->name);
    dp_question_answers_free(question);
    free(question->name);
    free(question);
}

static void
dp_question_cancel(question_t *question)
{
    if (question->txn != NULL) {
        INFO("[QU%d] question->txn = %p sdref=%p", SERIAL(question), question->txn, question->txn->sdref);
        ioloop_dnssd_txn_cancel(question->txn);
        ioloop_dnssd_txn_release(question->txn);
        question->txn = NULL;
    }

    // Remove the question from its list.
    question_t **questions, *q_cur;
    if (question->served_domain != NULL) {
        questions = &question->served_domain->questions;
    } else {
        questions = &questions_without_domain;
    }
    while (*questions != NULL ) {
        q_cur = *questions;
        if (q_cur == question) {
            *questions = q_cur->next;
            break;
        } else {
            questions = &q_cur->next;
        }
    }
    // If this was the last question, see if the served domain is still on the served domain list; if not,
    // this is the last reference, so free it.
    if (question->served_domain != NULL && question->served_domain->questions == NULL) {
        served_domain_t *served_domain;
        for (served_domain = served_domains; served_domain; served_domain = served_domain->next) {
            if (served_domain == question->served_domain) {
                break;
            }
        }
        if (served_domain == NULL) {
            served_domain_free(question->served_domain);
        }
    }
    RELEASE_HERE(question, question); // Release from the list.
}

// Called when the last reference on the query has been released.
static void
dnssd_query_finalize(void *context)
{
    dnssd_query_t *query = context;
    if (query->tracker != NULL) {
        RELEASE_HERE(query->tracker, dp_tracker);
        query->tracker = NULL;
    }
    if (query->message != NULL) {
        ioloop_message_release(query->message);
        query->message = NULL;
    }
    if (query->wakeup != NULL) {
        ioloop_wakeup_release(query->wakeup);
        query->wakeup = NULL;
    }
    if (query->response != NULL) {
        free(query->response);
        query->response = NULL;
    }
    if (query->response_msg != NULL) {
        dns_message_free(query->response_msg);
        query->response_msg = NULL;
    }
    if (query->question != NULL) {
        RELEASE_HERE(query->question, question);
        query->question = NULL;
    }
    free(query);
    dp_num_outstanding_queries--;
}

// Remove any finished queries from the question cache query list.
static void
dp_question_cache_remove_queries(question_t *question)
{
    // Convenience
    if (question == NULL) {
        return;
    }

    dnssd_query_t **pptr = &(question->queries);
    RETAIN_HERE(question, question);
    if (question->queries != NULL) {
        while (*pptr != NULL) {
            dnssd_query_t *cquery = *pptr;
            if (cquery->satisfied) {
                *pptr = cquery->question_next;
                RELEASE_HERE(cquery, dnssd_query);
            } else {
                pptr = &cquery->question_next;
            }
        }
        if (question->queries == NULL) {
            dp_question_cancel(question);
        }
    }
    RELEASE_HERE(question, question);
}

static void
dp_tracker_context_release(void *context)
{
    dp_tracker_t *tracker = context;
    RELEASE_HERE(tracker, dp_tracker);
}

static void
dp_tracker_went_away(dp_tracker_t *tracker)
{
    // Reduce the number of outstanding connections (should never go below zero).
    if (tracker->session_type == dp_tracker_session_push) {
        if (--num_push_sessions < 0) {
            FAULT("DNS Push connection count went negative");
            num_push_sessions = 0;
        } else {
            INFO("[TRK%d][DSO%d][C%d] dso connection count dropped: %d", SERIAL(tracker), SERIAL(tracker->dso), SERIAL(tracker->connection), num_push_sessions);
        }
    }
}

static void
dp_tracker_idle(void *context)
{
    dp_tracker_t *tracker = context;
    // Shouldn't be NULL.
    if (tracker->connection != NULL) {
        comm_t *connection = tracker->connection;
        INFO("[TRK%d][DSO%d][C%d] tracker for connection " PRI_S_SRP " has gone idle.",
             SERIAL(tracker), SERIAL(tracker->dso), SERIAL(connection), connection->name);

        // If the connection is already disconnected, it's already released its reference to the tracker. If not,
        // the release below will release tracker as a side effect. So in case tracker survives, clear the
        // connection pointer.
        tracker->connection = NULL;
        // The POSIX ioloop just hands us the "listener", which we do not want to cancel.
        if (!connection->is_listener) {
            ioloop_comm_cancel(connection);
        }
        ioloop_comm_release(connection);
    }
}

static void
dp_tracker_idle_after(dp_tracker_t *tracker, int seconds, dnssd_query_t *query)
{
    if (tracker->connection != NULL && !tracker->connection->is_listener &&
        tracker->dso == NULL && (query == NULL ||
                                 (tracker->dns_queries == NULL ||
                                  (tracker->dns_queries == query && query->next == NULL))))
    {
        if (tracker->idle_timeout == NULL) {
            tracker->idle_timeout = ioloop_wakeup_create();
        }
        if (tracker->idle_timeout == NULL) {
            ERROR("[TRK%d] no memory for idle timeout", SERIAL(tracker));
        } else {
            ioloop_add_wake_event(tracker->idle_timeout, tracker, dp_tracker_idle, dp_tracker_context_release, seconds * MSEC_PER_SEC);
            RETAIN_HERE(tracker, dp_tracker);
        }
    }
}

static bool
dp_same_message(message_t *a, message_t *b)
{
    // Code commented out below catches retransmissions, but right now this won't work and we'll leak queries,
    // so saving it for rdar://111808637 (dnssd-proxy is way too complicated)
    if (a == b /*  || (a != NULL && b != NULL && a->wire.id == b->wire.id) */ ) {
        return true;
    }
    return false;
}

// Called at any time (prior to release!) to cancel a query.
static void
dnssd_query_cancel(dnssd_query_t *query)
{
    INFO("[Q%d][QU%d] " PRI_S_SRP PUB_S_SRP PUB_S_SRP, SERIAL(query), SERIAL(query->question),
         query->question == NULL ? "<null>" : query->question->name,
         query->question == NULL ? "" : ((query->question->served_domain
                                          ? (query->question->served_domain->interface
                                             ? DOT_LOCAL
                                             : query->question->served_domain->domain_ld)
                                          : "")),
         query->canceled ? " canceled" : "");
    // Avoid double-cancellation
    if (query->canceled) {
        return;
    }
    // Retain the query for the duration of dnssd_query_cancel so that it doesn't get finalized while we are working on it.
    RETAIN_HERE(query, dnssd_query);
    query->canceled = true; // prevent double-cancellation
    if (query->tracker != NULL) {
        dp_tracker_t *tracker = query->tracker;

        // Retain the tracker so it doesn't get released while we are working on it.
        RETAIN_HERE(tracker, dp_tracker);

        if (query->dso == NULL) {
            bool unsatisfied = false;
            for (dnssd_query_t *list_query = tracker->dns_queries; list_query != NULL; list_query = list_query->next) {
                if (dp_same_message(query->message, list_query->message)) {
                    if (!query->satisfied) {
                        unsatisfied = true;
                    }
                }
            }

            if (!unsatisfied) {
                // Scan the list freeing all queries relating to the message attached to the query that's been canceled.
                // A UDP message will never have any other queries, but TCP connections can have multiple messages.
                for (dnssd_query_t **qp = &tracker->dns_queries; *qp != NULL; ) {
                    dnssd_query_t *list_query = *qp;

                    // Release the current query either if it's the query that's being canceled, or this is a UDP message.
                    if (dp_same_message(query->message, list_query->message)) {
                        *qp = list_query->next;

                        // This might release the query, but we know that the tracker holds a reference to it, so
                        // we don't need another reference to it.
                        if (list_query->wakeup != NULL) {
                            ioloop_wakeup_release(list_query->wakeup);
                            list_query->wakeup = NULL;
                        }

                        // Release this query's reference to the tracker
                        RELEASE_HERE(tracker, dp_tracker);
                        list_query->tracker = NULL;

                        // The tracker was holding a reference to the query.
                        RELEASE_HERE(list_query, dnssd_query);
                    } else {
                        qp = &list_query->next;
                    }
                }
            }
        } else {
            // For DNS Push queries, drop the activity, which will release the query.
            if (query->activity != NULL && query->dso != NULL) {
                dso_activity_t *activity = query->activity;
                dso_state_t *dso = query->dso;
                dso_drop_activity(dso, activity);
                query->activity = NULL;
            }
            // Now release the reference the query had on the tracker.
            query->tracker = NULL;
            RELEASE_HERE(tracker, dp_tracker);
        }

        // For TCP connections, wait for it to become idle before closing.
        if (tracker->connection != NULL && tracker->dns_queries == NULL) {
            if (tracker->connection->tcp_stream) {
                dp_tracker_idle_after(tracker, 15, query);
            } else {
#if UDP_LISTENER_USES_CONNECTION_GROUPS
                ioloop_comm_cancel(tracker->connection);
#else
                ioloop_comm_release(tracker->connection);
                tracker->connection = NULL;
#endif
            }
        }

        // Release the reference we retained on entry.
        RELEASE_HERE(tracker, dp_tracker);
    } else {
        if (query->wakeup != NULL) {
            ioloop_wakeup_release(query->wakeup);
            query->wakeup = NULL;
        }
    }
    query->satisfied = true;
    RELEASE_HERE(query, dnssd_query);
}

static void
dp_query_track(dp_tracker_t *tracker, dnssd_query_t *query)
{
    dnssd_query_t **qp = &tracker->dns_queries;

    while (*qp != NULL) {
        if (*qp == query) {
            ERROR("[Q%d][TRK%d] query is already being tracked.", SERIAL(query), SERIAL(query->tracker));
            return;
        }
        qp = &(*qp)->next;
    }
    *qp = query;
    RETAIN_HERE(query, dnssd_query);
}

static void
dp_tracker_disconnected(comm_t *UNUSED connection, void *context, int UNUSED error)
{
    dp_tracker_t *tracker = context;
    dnssd_query_t *dns_queries = tracker->dns_queries, **qp, *query;
    comm_t *tracker_connection = tracker->connection;
    tracker->connection = NULL;
    tracker->dns_queries = NULL;

    INFO("[TRK%d][DSO%d][C%d] queries %p", SERIAL(tracker), SERIAL(tracker->dso), SERIAL(tracker_connection), dns_queries);

    // If there is a DSO state outstanding on the tracker, cancel any activities connected to it.
    if (tracker->dso != NULL) {
        dso_activity_t *activity = tracker->dso->activities;
        while (activity != NULL) {
            dso_drop_activity(tracker->dso, tracker->dso->activities);
            // Failsafe in case dso_drop_activity for some reason doesn't drop the activity.
            if (tracker->dso->activities == activity) {
                break;
            }
            activity = tracker->dso->activities;
        }
        dso_state_cancel(tracker->dso);
        dp_tracker_went_away(tracker);
        tracker->session_type = dp_tracker_session_none;
        tracker->dso = NULL;
    }

    // We probably still have the connection object at this point, so we should release it, which could
    // in turn finalize the tracker.
    if (tracker_connection != NULL) {
        ioloop_comm_release(tracker_connection);
        tracker->connection = NULL;
    }

    // If dns_queries is non-null, tracker still exists, but it might go away when we cancel the last
    // query.
    qp = &dns_queries;
    while (*qp != NULL) {
        query = *qp;
        *qp = query->next;
        RELEASE_HERE(query, dnssd_query);
    }
}

static void
dns_push_cancel(dso_activity_t *activity)
{
    dnssd_query_t *query = (dnssd_query_t *)activity->context;
    INFO("[Q%d][QU%d] " PUB_S_SRP, SERIAL(query), SERIAL(query->question), activity->name);
    // We can either get here because the dso object is being finalized, or because the activity is being dropped.
    // In the former case, we need to cancel the query. In the latter case, we've been called as a result of
    // dnssd_query_cancel calling dso_drop_activity. dnssd_query_cancel sets query->activity to NULL before dropping
    // it, so we mustn't call back in to dnssd_query_cancel.
    if (query->activity != NULL) {
        query->activity = NULL;
        query->satisfied = true;
        dp_question_cache_remove_queries(query->question);
        dnssd_query_cancel(query);
    }
    // The activity held a reference to the query.
    RELEASE_HERE(query, dnssd_query);
}

static void
dp_tracker_not_idle(dp_tracker_t *tracker)
{
    if (tracker->idle_timeout) {
        ioloop_cancel_wake_event(tracker->idle_timeout);
    }
}

static served_domain_t *
dp_served(dns_name_t *name, char *buf, size_t bufsize)
{
    served_domain_t *sdt;
    dns_label_t *lim;

    for (sdt = served_domains; sdt; sdt = sdt->next) {
        if ((lim = dns_name_subdomain_of(name, sdt->domain_name))) {
            dns_name_print_to_limit(name, lim, buf, bufsize);
            return sdt;
        }
    }
    return NULL;
}

static bool
is_in_local_domain(const dns_name_t *const NONNULL name)
{
    const dns_label_t *prev_root_label;
    const dns_label_t *root_label;

    for (prev_root_label = NULL, root_label = name;
         root_label->next != NULL;
         prev_root_label = root_label, root_label = root_label->next)
        ;

    if (prev_root_label == NULL) {
        return false;
    }

#define LOCAL_DOMAIN_LABEL "local"
    if (prev_root_label->len != strlen(LOCAL_DOMAIN_LABEL)) {
        return false;
    }

    if (!dns_labels_equal(prev_root_label->data, LOCAL_DOMAIN_LABEL, strlen(LOCAL_DOMAIN_LABEL))) {
        return false;
    }
#undef LOCAL_DOMAIN_LABEL

    return true;
}

// Utility function to find "local" on the end of a string of labels.
static bool
truncate_local(dns_name_t *name)
{
    dns_label_t *lp, *prev, *prevprev;

    prevprev = prev = NULL;
    // Find the root label.
    for (lp = name; lp && lp->len; lp = lp->next) {
        prevprev = prev;
        prev = lp;
    }
    if (lp && prev && prevprev) {
        if (prev->len == 5 && dns_labels_equal(prev->data, "local", 5)) {
            dns_name_free(prev);
            prevprev->next = NULL;
            return true;
        }
    }
    return false;
}

static bool
dp_query_add_data_to_response(dnssd_query_t *query, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                              uint16_t rdlen, const void *rdata, int32_t ttl, const bool hardwired_response,
                              bool dont_elide, uint16_t *counter)
{
    bool record_added;
    dns_towire_state_t *towire = &query->towire;
    const char *failnote = NULL;
    const uint8_t *rd = rdata;
    char pbuf[DNS_MAX_NAME_SIZE + 1];
    char rbuf[DNS_MAX_NAME_SIZE + 1];
    uint8_t *revert = query->towire.p; // Remember where we were in case there's no room.
    question_t *question = query->question;

    // Only do the translation if:
    // 1. We serve the domain.
    // 2. The response we will add does not come from our hardwired response set.
    const bool translate = (question->served_domain != NULL) && (!hardwired_response);

    if (rdlen == 0) {
        INFO("[Q%d][QU%d] eliding zero-length response for " PRI_S_SRP " " PUB_S_SRP " %d",
             SERIAL(query), SERIAL(question), fullname, dns_rrtype_to_string(rrtype), rrclass);
        record_added = false;
        goto exit;
    }
    // Don't send A records for 127.* nor AAAA records for ::1
    if (dont_elide) {
    } else if (rrtype == dns_rrtype_a && rdlen == 4) {
        // Should use IN_LINKLOCAL and IN_LOOPBACK macros here, but for some reason they are not present on
        // OpenWRT.
        if (rd[0] == 127) {
            IPv4_ADDR_GEN_SRP(rd, rd_buf);
            INFO("[Q%d][QU%d] eliding localhost response for " PRI_S_SRP ": " PRI_IPv4_ADDR_SRP,
                 SERIAL(query), SERIAL(question), fullname, IPv4_ADDR_PARAM_SRP(rd, rd_buf));
            record_added = false;
            goto exit;
        }
        if (rd[0] == 169 && rd[1] == 254) {
            IPv4_ADDR_GEN_SRP(rd, rd_buf);
            INFO("[Q%d][QU%d] eliding link-local response for " PRI_S_SRP ": " PRI_IPv4_ADDR_SRP,
                 SERIAL(query), SERIAL(question), fullname, IPv4_ADDR_PARAM_SRP(rd, rd_buf));
            record_added = false;
            goto exit;
        }
    } else if (rrtype == dns_rrtype_aaaa && rdlen == 16) {
        struct in6_addr addr = *(struct in6_addr *)rdata;
        if (IN6_IS_ADDR_LOOPBACK(&addr)) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(rdata, rdata_buf);
            INFO("[Q%d][QU%d] eliding localhost response for " PRI_S_SRP ": " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SERIAL(query), SERIAL(question), fullname, SEGMENTED_IPv6_ADDR_PARAM_SRP(rdata, rdata_buf));
            record_added = false;
            goto exit;
        }
        if (IN6_IS_ADDR_LINKLOCAL(&addr)) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(rdata, rdata_buf);
            INFO("[Q%d][QU%d] eliding link-local response for " PRI_S_SRP ": " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 SERIAL(query), SERIAL(question), fullname, SEGMENTED_IPv6_ADDR_PARAM_SRP(rdata, rdata_buf));
            record_added = false;
            goto exit;
        }
    }
    INFO("survived for rrtype " PUB_S_SRP " rdlen %d", dns_rrtype_to_string(rrtype), rdlen);

    if (query->dso != NULL) {
        dns_push_start(query);
    }
    // Rewrite the domain if it's .local.
    if (question->served_domain != NULL) {
        TOWIRE_CHECK("concatenate_name_to_wire", towire,
                     dns_concatenate_name_to_wire(towire, NULL, question->name, question->served_domain->domain));
        INFO("[Q%d][QU%d] " PUB_S_SRP " answer:  type " PUB_S_SRP " class %02d " PRI_S_SRP "." PRI_S_SRP, SERIAL(query), SERIAL(question),
             query->dso != NULL ? "PUSH" : "DNS ", dns_rrtype_to_string(rrtype), rrclass, question->name, question->served_domain->domain);
    } else {
        TOWIRE_CHECK("compress_name_to_wire", towire, dns_concatenate_name_to_wire(towire, NULL, NULL, question->name));
        INFO("[Q%d][QU%d] " PUB_S_SRP " answer:  type " PUB_S_SRP " class %02d " PRI_S_SRP " (p)",
             SERIAL(query), SERIAL(question), query->dso != NULL ? "push" : " dns", dns_rrtype_to_string(rrtype), rrclass, question->name);
    }
    TOWIRE_CHECK("rrtype", towire, dns_u16_to_wire(towire, rrtype));
    TOWIRE_CHECK("rrclass", towire, dns_u16_to_wire(towire, rrclass));
    TOWIRE_CHECK("ttl", towire, dns_ttl_to_wire(towire, ttl));

    // If necessary, correct domain names inside of rrdata.
    dns_rr_t answer;
    dns_name_t *name;
    unsigned offp = 0;

    answer.type = rrtype;
    answer.qclass = rrclass;
    if (dns_rdata_parse_data(&answer, rdata, &offp, rdlen, rdlen, 0)) {
        switch(rrtype) {
            case dns_rrtype_cname:
            case dns_rrtype_ptr:
            case dns_rrtype_ns:
            case dns_rrtype_md:
            case dns_rrtype_mf:
            case dns_rrtype_mb:
            case dns_rrtype_mg:
            case dns_rrtype_mr:
            case dns_rrtype_nsap_ptr:
            case dns_rrtype_dname:
                name = answer.data.ptr.name;
                TOWIRE_CHECK("rdlength begin", towire, dns_rdlength_begin(towire));
                break;
            case dns_rrtype_srv:
                name = answer.data.srv.name;
                TOWIRE_CHECK("rdlength begin", towire, dns_rdlength_begin(towire));
                TOWIRE_CHECK("answer.data.srv.priority", towire, dns_u16_to_wire(towire, answer.data.srv.priority));
                TOWIRE_CHECK("answer.data.srv.weight", towire, dns_u16_to_wire(towire, answer.data.srv.weight));
                TOWIRE_CHECK("answer.data.srv.port", towire, dns_u16_to_wire(towire, answer.data.srv.port));
                break;
            default:
                INFO("[Q%d][QU%d] record type " PUB_S_SRP " not translated", SERIAL(query), SERIAL(question), dns_rrtype_to_string(rrtype));
                dns_rrdata_free(&answer);
                goto raw;
        }

        dns_name_print(name, rbuf, sizeof rbuf);

        if (translate && is_in_local_domain(name)) {
            // If the response requires the translation from <served domain> to ".local." and the response ends in
            // ".local.", truncate it.
            truncate_local(name);
            dns_name_print(name, pbuf, sizeof pbuf);
            TOWIRE_CHECK("concatenate_name_to_wire 2", towire,
                         dns_concatenate_name_to_wire(towire, name, NULL, question->served_domain->domain));
            INFO("[Q%d][QU%d] translating " PRI_S_SRP " to " PRI_S_SRP " . " PRI_S_SRP,
                 SERIAL(query), SERIAL(question), rbuf, pbuf, question->served_domain->domain);
        } else {
            TOWIRE_CHECK("concatenate_name_to_wire 2", towire,
                         dns_concatenate_name_to_wire(towire, name, NULL, NULL));
            INFO("[Q%d][QU%d] compressing " PRI_S_SRP, SERIAL(query), SERIAL(question), rbuf);
        }

        dns_name_free(name);
        dns_rdlength_end(towire);
    } else {
        ERROR("[Q%d][QU%d] rdata from mDNSResponder didn't parse!!", SERIAL(query), SERIAL(question));
    raw:
        TOWIRE_CHECK("rdlen", towire, dns_u16_to_wire(towire, rdlen));
        TOWIRE_CHECK("rdata", towire, dns_rdata_raw_data_to_wire(towire, rdata, rdlen));
    }

    if (towire->truncated || failnote) {
        ERROR("[Q%d][QU%d] RR ADD FAIL: " PUB_S_SRP, SERIAL(query), SERIAL(question), failnote);
        query->towire.p = revert;
        record_added = false;
        goto exit;
    }

    record_added = true;
    if (counter != NULL && query->dso == NULL) {
        *counter = htons(ntohs(*counter) + 1);
    }
exit:
    return record_added;
}

static void
dnssd_hardwired_add(served_domain_t *sdt,
                    const char *name, const char *domain, size_t rdlen, const uint8_t *rdata, uint16_t type)
{
    hardwired_t *hp, **hrp;
    size_t namelen = strlen(name);
    size_t domainlen = strlen(domain);
    size_t total = sizeof *hp;
    uint8_t *trailer;
    total += rdlen; // Space for RDATA
    total += namelen; // Space for name
    total += 1; // NUL
    total += namelen;// space for FQDN
    total += domainlen;
    total += 1; // NUL

    hp = calloc(1, total + 4);
    if (hp == NULL) {
        ERROR("no memory for %s %s", name, domain);
        return;
    }
    trailer = ((uint8_t *)hp) + total;
    memcpy(trailer, "abcd", 4);
    hp->rdata = (uint8_t *)(hp + 1);
    hp->rdlen = rdlen;
    memcpy(hp->rdata, rdata, rdlen);
    hp->name = (char *)hp->rdata + rdlen;
    memcpy(hp->name, name, namelen);
    hp->name[namelen] = '\0';
    hp->fullname = hp->name + namelen + 1;
    if (namelen != 0) {
        snprintf(hp->fullname, namelen + domainlen + 1, "%s%s", name, domain);
    } else {
        memcpy(hp->fullname, domain, domainlen);
        hp->fullname[domainlen] = '\0';
    }
    if (hp->fullname + strlen(hp->fullname) + 1 != (char *)hp + total) {
        ERROR("%p != %p", hp->fullname + strlen(hp->fullname) + 1, ((char *)hp) + total);
        return;
    }
    if (memcmp(trailer, "abcd", 4)) {
        ERROR("ran off the end.");
        return;
    }
    hp->type = type;
    hp->next = NULL;

    // Store this new hardwired_t at the end of the list unless a hardwired_t with the same name
    // is already on the list.   If it is, splice it in.
    for (hrp = &sdt->hardwired_responses; *hrp != NULL; hrp = &(*hrp)->next) {
        hardwired_t *old = *hrp;
        if (old->type != hp->type) {
            continue;
        }
        if (strcasecmp(old->fullname, hp->fullname) != 0) {
            continue;
        }
        // The same name and type
        bool superseded;
        switch (type) {
            case dns_rrtype_a:
            case dns_rrtype_aaaa:
            case dns_rrtype_ns:
            case dns_rrtype_ptr:
                superseded = false;
                break;
            default:
                // dns_rrtype_soa
                // dns_rrtype_srv
                superseded = true;
                break;
        }

        if (superseded) {
            INFO("superseding " PRI_S_SRP " name " PRI_S_SRP " type %d rdlen %d", old->fullname,
                 old->name, old->type, old->rdlen);
            hp->next = old->next;
            free(old);
        } else {
            INFO("inserting before " PRI_S_SRP " name " PRI_S_SRP " type %d rdlen %d", old->fullname,
                 old->name, old->type, old->rdlen);
            hp->next = old;
        }
        break;
    }
    *hrp = hp;

    INFO("fullname " PRI_S_SRP " name " PRI_S_SRP " type %d rdlen %d",
         hp->fullname, hp->name, hp->type, hp->rdlen);
}

#if STUB_ROUTER
static bool
dnssd_hardwired_remove_record(served_domain_t *const NONNULL sdt, const char *const NONNULL name, const char *const NONNULL domain, size_t rdlen,
    const void *const NULLABLE rdata, uint16_t type)
{
    bool removed;
    hardwired_t *prev = NULL;
    hardwired_t *current;
    char full_name[DNS_MAX_NAME_SIZE + 1];

    int bytes_written = snprintf(full_name, sizeof(full_name), "%s%s", name, domain);
    require_action_quiet(bytes_written > 0 && (size_t)bytes_written < sizeof(full_name), exit, removed = false;
        ERROR("snprintf truncates the string - name length: %zu, domain length: %zu, buffer length: %zu",
            strlen(name), strlen(domain), sizeof(full_name))
    );

    for (current = sdt->hardwired_responses; current != NULL; prev = current, current = current->next) {
        if (current->type != type) {
            continue;
        }
        if (rdata != NULL && current->rdlen != rdlen) {
            continue;
        }
        if (strcasecmp(current->fullname, full_name) != 0) {
            continue;
        }
        if (rdata != NULL && memcmp(current->rdata, rdata, rdlen) != 0) {
            continue;
        }
        // record found
        break;
    }
    require_action_quiet(current != NULL, exit, removed = false;
        ERROR("no matching hardwired_t found - record name: " PUB_S_SRP ", record type: %d", full_name, type));

    if (prev != NULL) {
        prev->next = current->next;
    } else {
        sdt->hardwired_responses = current->next;
    }
    free(current);

    removed = true;
exit:
    return removed;
}

static bool
dnssd_hardwired_add_or_remove_addr_record(served_domain_t *const NONNULL sdt, const addr_t *const NONNULL addr,
    const char *const NONNULL name, bool add)
{
    dns_wire_t wire;
    dns_towire_state_t towire;
    bool succeeded;

    memset(&towire, 0, sizeof towire);
    towire.message = &wire;
    towire.p = wire.data;
    towire.lim = towire.p + sizeof wire.data;

    const void *rdata_ptr;
    size_t addr_len;
    uint16_t addr_type;
    if (addr->sa.sa_family == AF_INET) {
        rdata_ptr = &addr->sin.sin_addr;
        addr_len = sizeof(addr->sin.sin_addr);
        addr_type = dns_rrtype_a;
    } else { // addr.sa.sa_family == AF_INET6
        rdata_ptr = &addr->sin6.sin6_addr;
        addr_len = sizeof(addr->sin6.sin6_addr);
        addr_type = dns_rrtype_aaaa;
    }
    dns_rdata_raw_data_to_wire(&towire, rdata_ptr, addr_len);

    if (add) {
        dnssd_hardwired_add(sdt, name, name[0] == '\0' ? sdt->domain : sdt->domain_ld, towire.p - wire.data, wire.data,
            addr_type);
        succeeded = true;
    } else {
        succeeded = dnssd_hardwired_remove_record(sdt, name,  name[0] == '\0' ? sdt->domain : sdt->domain_ld,
            towire.p - wire.data, wire.data, addr_type);
    }

    return succeeded;
}

static bool
dnssd_hardwired_add_or_remove_address_in_domain(const char *const NONNULL name,
    const char *const NONNULL domain_to_change, const addr_t *const NONNULL address, const bool add)
{
    bool succeeded;

    served_domain_t *served_domain = find_served_domain(domain_to_change);
    require_action_quiet(served_domain != NULL, exit, succeeded = false;
        ERROR("could not find served domain with the specified domain name - domain name: " PRI_S_SRP, domain_to_change)
    );

    succeeded = dnssd_hardwired_add_or_remove_addr_record(served_domain, address, name, add);
    require_action_quiet(succeeded, exit, succeeded = false;
        ERROR("failed to " PUB_S_SRP " address record - domain name: " PRI_S_SRP,
            domain_to_change, add ? "add" : "remove")
    );

exit:
    return succeeded;
}

static bool
dnssd_hardwired_generate_ptr_name(const addr_t *const NONNULL addr, const addr_t *const NONNULL mask,
                                  char *name_buf, size_t buf_size)
{
    char *name_ptr = name_buf;
    const char *const name_limit = name_ptr + buf_size;
    int bytes_written;
    bool succeeded;

#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    bytes_written = snprintf(name_ptr, name_limit - name_ptr, SRV_TYPE_FOR_AUTOMATIC_BROWSING_DOMAIN);
    require_action_quiet(bytes_written > 0 && bytes_written < name_limit - name_ptr, exit, succeeded = false;
        ERROR("snprintf truncates the string - bytes_written: %d, limit: %zd", bytes_written, name_limit - name_ptr));
    name_ptr += bytes_written;


    if (addr->sa.sa_family == AF_INET) {
        const uint32_t subnet = (ntohl(addr->sin.sin_addr.s_addr) & ntohl(mask->sin.sin_addr.s_addr));
        bytes_written = snprintf(name_ptr, name_limit - name_ptr, ".%u.%u.%u.%u",
            subnet & 0xFFU, (subnet >> 8) & 0xFFU, (subnet >> 16) & 0xFFU, (subnet >> 24) & 0xFFU);
        require_action(bytes_written > 0 && bytes_written < name_limit - name_ptr, exit, succeeded = false);
        // Remember to increase the name_ptr by bytes_written bytes if name_ptr is used later.

    } else if (addr->sa.sa_family == AF_INET6) {
        const uint8_t *const addr_bytes = addr->sin6.sin6_addr.s6_addr;
        const uint8_t *const mask_bytes = mask->sin6.sin6_addr.s6_addr;
        for (int i = 15; i >= 0; i--) {
            for (int shift = 0; shift < 8; shift += 4) {
                bytes_written = snprintf(name_ptr, name_limit - name_ptr, ".%x",
                    (addr_bytes[i] >> shift) & (mask_bytes[i] >> shift) & 15);
                require_action_quiet(bytes_written > 0 && bytes_written < name_limit - name_ptr, exit, succeeded = false;
                    ERROR("snprintf truncates the string - bytes_written: %d, limit: %zd",
                        bytes_written, name_limit - name_ptr)
                );
                name_ptr += bytes_written;
            }
        }

    } else {
        FAULT("skipping address type other than IPv4/IPv6 - type: %u", addr->sa.sa_family);
        succeeded = false;
        goto exit;
    }

    succeeded = true;
exit:
    return succeeded;
}

static bool
dnssd_hardwired_add_or_remove_ptr_record(served_domain_t *const NONNULL sdt, const addr_t *const NONNULL addr,
    const addr_t *const NONNULL mask, bool add)
{
    char name[DNS_MAX_NAME_SIZE + 1];
    dns_wire_t wire;
    dns_towire_state_t towire;
    bool succeeded;

#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    succeeded = dnssd_hardwired_generate_ptr_name(addr, mask, name, sizeof(name));
    if (!succeeded) {
        INFO("address is not eligible to construct PTR record");
        goto exit;
    }

    for (served_domain_t *if_domain = served_domains; if_domain != NULL; if_domain = if_domain->next) {
        if (if_domain->interface == NULL || if_domain->interface->ifindex == 0) {
            continue;
        }
        RESET;
        INFO(PUB_S_SRP " PTR from " PRI_S_SRP " to " PRI_S_SRP, add ? "Adding" : "Removing", name, if_domain->domain);
        dns_full_name_to_wire(NULL, &towire, if_domain->domain);

        if (add) {
            dnssd_hardwired_add(sdt, name, sdt->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_ptr);
            succeeded = true;
        } else {
            succeeded = dnssd_hardwired_remove_record(sdt, name, sdt->domain_ld, towire.p - wire.data, wire.data,
                dns_rrtype_ptr);
        }
    }

exit:
    return succeeded;
}

static bool
dnssd_hardwired_add_or_remove_ptr_in_domain(const char *const NONNULL domain_to_change,
    const addr_t *const NONNULL address, const addr_t *const NONNULL mask, const bool add)
{
    bool succeeded;

    served_domain_t *served_domain = find_served_domain(domain_to_change);
    require_action_quiet(served_domain != NULL, exit, succeeded = false;
        ERROR("could not find served domain with the specified domain name - domain name: " PRI_S_SRP, domain_to_change)
    );

    succeeded = dnssd_hardwired_add_or_remove_ptr_record(served_domain, address, mask, add);
    require_action_quiet(succeeded, exit, succeeded = false;
        ERROR("failed to " PUB_S_SRP " address record - domain name: " PRI_S_SRP,
            add ? "adding" : "removing", domain_to_change)
    );

exit:
    return succeeded;
}

static bool
is_valid_address_to_publish(const addr_t *const NONNULL address)
{
    bool is_valid = true;

    if (address->sa.sa_family == AF_INET) {
        const struct in_addr *const ipv4_address = &(address->sin.sin_addr);
        const bool is_linklocal = is_in_addr_link_local(ipv4_address);
        const bool is_loopback = is_in_addr_loopback(ipv4_address);

        if (is_linklocal || is_loopback) {
            IPv4_ADDR_GEN_SRP(&ipv4_address, ipv4_address_buf);
            INFO("ignoring the address for interface - address: " PRI_IPv4_ADDR_SRP ", address type: " PUB_S_SRP ".",
                IPv4_ADDR_PARAM_SRP(&ipv4_address, ipv4_address_buf), is_linklocal ? "link local" : "loopback");
            is_valid = false;
        }

    } else if (address->sa.sa_family == AF_INET6) {
        const struct in6_addr *const ipv6_address = &(address->sin6.sin6_addr);
        const bool is_linklocal = IN6_IS_ADDR_LINKLOCAL(ipv6_address);
        const bool is_loopback = IN6_IS_ADDR_LOOPBACK(ipv6_address);

        if (is_linklocal || is_loopback) {
            IPv6_ADDR_GEN_SRP(ipv6_address->s6_addr, ipv6_address_buf);
            INFO("ignoring the address for interface - address: " PRI_IPv6_ADDR_SRP ", address type: " PUB_S_SRP ".",
                IPv6_ADDR_PARAM_SRP(ipv6_address->s6_addr, ipv6_address_buf), is_linklocal ? "link local" : "loopback");
            is_valid = false;
        }

    } else {
        // It is possible that MAC address is added for the interface, so ignore it.
        INFO("Non IPv4/IPv6 address added for the interface - sa_family: %u", address->sa.sa_family);
        is_valid = false;
    }

    return is_valid;
}

static bool
dnssd_hardwired_process_addr_change(const addr_t *const NONNULL addr, const addr_t *const NONNULL mask, const bool add)
{
    bool succeeded;

    if (!is_valid_address_to_publish(addr)) {
        succeeded = true;
        goto exit;
    }

    // Update the <local host name>.home.arpa. address mapping.
    succeeded = dnssd_hardwired_add_or_remove_address_in_domain("", my_name, addr, add);
    if (!succeeded) {
        ERROR("failed to update address record for domain - domain: " PRI_S_SRP, my_name);
        goto exit;
    }

    // Update the <local host name>.<Thread ID>.thread.home.arpa. address mapping.
    succeeded = dnssd_hardwired_add_or_remove_address_in_domain(local_host_name, THREAD_DOMAIN_WITH_ID, addr, add);
    if (!succeeded) {
        ERROR("failed to update address record for domain - domain: " PUB_S_SRP, THREAD_DOMAIN_WITH_ID);
        goto exit;
    }

    // Update the default.service.arpa. address mapping.
    succeeded = dnssd_hardwired_add_or_remove_address_in_domain(local_host_name, DEFAULT_SERVICE_ARPA_DOMAIN, addr, add);
    if (!succeeded) {
        ERROR("failed to update address record for domain - domain: " PUB_S_SRP, DEFAULT_SERVICE_ARPA_DOMAIN);
        goto exit;
    }

#if SRP_FEATURE_LOCAL_DISCOVERY
    // Update the default.service.arpa. address mapping.
    succeeded = dnssd_hardwired_add_or_remove_address_in_domain(local_host_name, DOT_LOCAL_DOMAIN, addr, add);
    if (!succeeded) {
        ERROR("failed to update address record for domain - domain: " PUB_S_SRP, LOCAL);
        goto exit;
    }
#endif

    // Setup the "_lb.dns-sd"
    // Update the "reverse mapping from address to browsing domain" for each eligible served domain under IPv6 or IPv4
    // reverse lookup domain.
    if (addr->sa.sa_family == AF_INET6) {
        succeeded = dnssd_hardwired_add_or_remove_ptr_in_domain(IPV6_REVERSE_LOOKUP_DOMAIN, addr, mask, add);
    } else if (addr->sa.sa_family == AF_INET) {
        succeeded = dnssd_hardwired_add_or_remove_ptr_in_domain(IPV4_REVERSE_LOOKUP_DOMAIN, addr, mask, add);
    } else {
        char buf[INET6_ADDRSTRLEN];
        IOLOOP_NTOP(addr, buf);
        INFO("Skipping non IPv6/IPv4 address - addr:" PRI_S_SRP, buf);
        succeeded = true;
    }

exit:
    return succeeded;
}

static void
dnssd_hardwired_lbdomains_setup(void)
{
    served_domain_t *ipv6, *ipv4;
#if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
    // When dnssd-proxy is combined with srp-mdns-proxy, IPv4 and IPv6 reverse look up domain is set from the begining.
    ipv4 = find_served_domain(IPV4_REVERSE_LOOKUP_DOMAIN);
    ipv6 = find_served_domain(IPV6_REVERSE_LOOKUP_DOMAIN);
#else // #if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
    ipv4 = new_served_domain(NULL, IPV4_REVERSE_LOOKUP_DOMAIN);
    ipv6 = new_served_domain(NULL, IPV6_REVERSE_LOOKUP_DOMAIN);
#endif // #if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
    require_action_quiet(ipv4 != NULL && ipv6 != NULL, exit, ERROR("cannot find/create new served domain"));

    for (served_domain_t *addr_domain = served_domains; addr_domain; addr_domain = addr_domain->next) {
        dp_interface_t *interface = addr_domain->interface;
        interface_addr_t *ifaddr;
        if (interface == NULL) {
            INFO("Domain " PRI_S_SRP " has no interface", addr_domain->domain);
            continue;
        }
        INFO("Interface " PUB_S_SRP, interface->name);
        // Add lb domain support for link domain
        for (ifaddr = interface->addresses; ifaddr != NULL; ifaddr = ifaddr->next) {
            // Do not publish link-local or loopback address
            if (!is_valid_address_to_publish(&ifaddr->addr)) {
                continue;
            }

            if (ifaddr->addr.sa.sa_family == AF_INET) {
                dnssd_hardwired_add_or_remove_ptr_record(ipv4, &ifaddr->addr, &ifaddr->mask, true);
            } else if (ifaddr->addr.sa.sa_family == AF_INET6) {
                dnssd_hardwired_add_or_remove_ptr_record(ipv6, &ifaddr->addr, &ifaddr->mask, true);
            } else {
                char buf[INET6_ADDRSTRLEN];
                IOLOOP_NTOP(&ifaddr->addr, buf);
                INFO("Skipping " PRI_S_SRP, buf);
            }
        }
    }
exit:
    return;
}
#endif

static void
dnssd_hardwired_setup(void)
{
    dns_wire_t wire;
    dns_towire_state_t towire;
    served_domain_t *sdt;
#if STUB_ROUTER
    dns_name_t *my_name_parsed = my_name == NULL ? NULL : dns_pres_name_parse(my_name);
#endif

#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    // For each interface, set up the hardwired names.
    for (sdt = served_domains; sdt; sdt = sdt->next) {
        if (sdt->interface == NULL) {
            continue;
        }

        // SRV
        // _dns-llq._udp
        // _dns-llq-tls._tcp
        // _dns-update._udp
        // _dns-update-tls._udp
        // We deny the presence of support for LLQ, because we only support DNS Push
        RESET;
        dnssd_hardwired_add(sdt, "_dns-llq._udp", sdt->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);
        dnssd_hardwired_add(sdt, "_dns-llq-tls._tcp", sdt->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);

        // We deny the presence of support for DNS Update, because a Discovery Proxy zone is stateless.
        dnssd_hardwired_add(sdt, "_dns-update._udp", sdt->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);
        dnssd_hardwired_add(sdt, "_dns-update-tls._tcp", sdt->domain_ld, towire.p - wire.data, wire.data,
                            dns_rrtype_srv);

        // Until we set up the DNS Push listener, we deny its existence.   If TLS is ready to go, this will be
        // overwritten immediately; otherwise it will be overwritten when the TLS key has been generated and signed.
        dnssd_hardwired_add(sdt, "_dns-push-tls._tcp", sdt->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);

#if STUB_ROUTER
        char namebuf[DNS_MAX_NAME_SIZE + 1];
        const char *local_name;
        addr_t addr;

        // If my_name wasn't set, or if my_name is in this interface's domain, we need to answer
        // for it when queried.
        if (my_name == NULL || my_name_parsed != NULL) {
            const char *local_domain = NULL;
            if (my_name == NULL) {
                local_name = "ns";
                local_domain = sdt->domain_ld;
            } else {
                dns_name_t *lim;
                local_name = NULL;

                // See if my_name is a subdomain of this interface's domain
                if ((lim = dns_name_subdomain_of(my_name_parsed, sdt->domain_name)) != NULL) {
                    dns_name_print_to_limit(my_name_parsed, lim, namebuf, sizeof namebuf);
                    local_name = namebuf;
                    dns_name_free(my_name_parsed);
                    my_name_parsed = NULL;
                    if (local_name[0] == '\0') {
                        local_domain = sdt->domain;
                    } else {
                        local_domain = sdt->domain_ld;
                    }
                }
            }
            if (local_name != NULL) {
                for (int i = 0; i < num_publish_addrs; i++) {
                    RESET;
                    memset(&addr, 0, sizeof addr);
                    getipaddr(&addr, publish_addrs[i]);
                    if (addr.sa.sa_family == AF_INET) {
                        // A
                        // ns
                        dns_rdata_raw_data_to_wire(&towire, &addr.sin.sin_addr, sizeof addr.sin.sin_addr);
                        dnssd_hardwired_add(sdt, local_name, local_domain, towire.p - wire.data, wire.data,
                                            dns_rrtype_a);
                    } else {
                        // AAAA
                        RESET;
                        dns_rdata_raw_data_to_wire(&towire, &addr.sin6.sin6_addr, sizeof addr.sin6.sin6_addr);
                        dnssd_hardwired_add(sdt, local_name, local_domain, towire.p - wire.data, wire.data,
                                            dns_rrtype_aaaa);
                    }
                }
            }
        }
#endif // STUB_ROUTER

        // NS
        RESET;
#if STUB_ROUTER
        if (string_ends_with(sdt->domain, THREAD_DOMAIN)) {
            // For served domain in the THREAD_DOMAIN, set the NS record to the local host name:
            // For example, openthread.thread.home.arpa. NS Office.local.
            // XXX is this right?
            require_quiet(local_host_name_dot_local[0] != 0, exit);
            dns_full_name_to_wire(NULL, &towire, local_host_name_dot_local);
        } else
#endif
        if (uuid_name[0] != 0) {
            dns_name_to_wire(NULL, &towire, uuid_name);
            dns_full_name_to_wire(NULL, &towire, sdt->domain);
        } else {
            dns_name_to_wire(NULL, &towire, "ns");
            dns_full_name_to_wire(NULL, &towire, sdt->domain);
        }
        dnssd_hardwired_add(sdt, "", sdt->domain, towire.p - wire.data, wire.data, dns_rrtype_ns);

        // SOA (piggybacking on what we already did for NS, which starts the same.
        dns_name_to_wire(NULL, &towire, "postmaster");
        dns_full_name_to_wire(NULL, &towire, sdt->domain);
        dns_u32_to_wire(&towire, 0);     // serial
        dns_ttl_to_wire(&towire, 7200);  // refresh
        dns_ttl_to_wire(&towire, 3600);  // retry
        dns_ttl_to_wire(&towire, 86400); // expire
        dns_ttl_to_wire(&towire, 120);    // minimum
        dnssd_hardwired_add(sdt, "", sdt->domain, towire.p - wire.data, wire.data, dns_rrtype_soa);
    }

    // Setup hardwired response A/AAAA record for <local host name>.home.arpa.
#if SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY
#if STUB_ROUTER
    // When dnssd-proxy is combined with srp-mdns-proxy, we get the address from the interface address list not from the
    // config file, so we search through the served domains for all available address.
    if (my_name_parsed != NULL) {
        dns_name_free(my_name_parsed);
        my_name_parsed = NULL;
    }

    require_action_quiet(my_name != NULL, exit, ERROR("Failed to get my_name and unable to set hardwired response"));
    served_domain_t *const my_name_served_domain = find_served_domain(my_name);
    require_action_quiet(my_name_served_domain != NULL, exit,
        ERROR("Failed to find my_name domain - my_name: " PRI_S_SRP, my_name));

    served_domain_t *const thread_served_domain = find_served_domain(THREAD_DOMAIN_WITH_ID);
    require_action(thread_served_domain != NULL, exit,
        ERROR("Failed to find thread domain - domain: " PUB_S_SRP, THREAD_DOMAIN_WITH_ID));

    served_domain_t *const default_service_arpa_domain = find_served_domain(DEFAULT_SERVICE_ARPA_DOMAIN);
    require_action(default_service_arpa_domain != NULL, exit,
        ERROR("Failed to find thread domain - domain: " PUB_S_SRP, DEFAULT_SERVICE_ARPA_DOMAIN));

#if SRP_FEATURE_LOCAL_DISCOVERY
    served_domain_t *const dot_local_domain = find_served_domain(DOT_LOCAL_DOMAIN);
    require_action(dot_local_domain != NULL, exit,
        ERROR("Failed to find thread domain - domain: " PUB_S_SRP, DOT_LOCAL_DOMAIN));
#endif
    for (const served_domain_t *domain = served_domains; domain != NULL; domain = domain->next) {
        if (domain->interface == NULL) {
            continue;
        }
        for (const interface_addr_t *if_addrs = domain->interface->addresses; if_addrs != NULL;
             if_addrs = if_addrs->next) {
            const addr_t *const if_addr = &if_addrs->addr;
            // Only publish routable IP address.
            if (!is_valid_address_to_publish(if_addr)) {
                continue;
            }

            RESET;
            uint16_t rr_type;
            if (if_addr->sa.sa_family == AF_INET) {
                dns_rdata_raw_data_to_wire(&towire, &if_addr->sin.sin_addr, sizeof(if_addr->sin.sin_addr));
                rr_type = dns_rrtype_a;
            } else if (if_addr->sa.sa_family == AF_INET6) {
                dns_rdata_raw_data_to_wire(&towire, &if_addr->sin6.sin6_addr, sizeof(if_addr->sin6.sin6_addr));
                rr_type = dns_rrtype_aaaa;
            } else {
                ERROR("Non IPv4/IPv6 address in interface addresses - sa_family: %u", if_addr->sa.sa_family);
                continue;
            }

            // <local host name>.home.arpa. A/AAAA <IP address>
            dnssd_hardwired_add(my_name_served_domain, "", my_name_served_domain->domain, towire.p - wire.data,
                wire.data, rr_type);

            // <local host name>.openthread.thread.home.arpa. A/AAAA <IP address>
            dnssd_hardwired_add(thread_served_domain, local_host_name, thread_served_domain->domain_ld,
                towire.p - wire.data, wire.data, rr_type);

            // <local host name>.default.service.arpa. A/AAAA <IP address>
            dnssd_hardwired_add(default_service_arpa_domain, local_host_name, default_service_arpa_domain->domain_ld,
                towire.p - wire.data, wire.data, rr_type);

#if SRP_FEATURE_LOCAL_DISCOVERY
            // <local host name>.local. A/AAAA <IP address>
            dnssd_hardwired_add(dot_local_domain, local_host_name, dot_local_domain->domain_ld,
                towire.p - wire.data, wire.data, rr_type);
#endif
        }
    }
#endif
#else // SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY
    if (my_name_parsed != NULL) {
        dns_name_free(my_name_parsed);
        my_name_parsed = NULL;

        sdt = new_served_domain(NULL, my_name);
        if (sdt == NULL) {
            ERROR("Unable to allocate domain for %s", my_name);
        } else {
            for (int i = 0; i < num_publish_addrs; i++) {
                // AAAA
                // A
                RESET;
                memset(&addr, 0, sizeof addr);
                getipaddr(&addr, publish_addrs[i]);
                if (addr.sa.sa_family == AF_INET) {
                    dns_rdata_raw_data_to_wire(&towire, &addr.sin.sin_addr, sizeof addr.sin.sin_addr);
                    dnssd_hardwired_add(sdt, "", sdt->domain, towire.p - wire.data, wire.data, dns_rrtype_a);
                } else {
                    dns_rdata_raw_data_to_wire(&towire, &addr.sin6.sin6_addr, sizeof addr.sin6.sin6_addr);
                    dnssd_hardwired_add(sdt, "", sdt->domain, towire.p - wire.data, wire.data, dns_rrtype_aaaa);
                }
            }
        }
    }
#endif // SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY

#if STUB_ROUTER
    // Setup _lb._udp.<reversed IP address> PTR record for the domain we are advertising, for example:
    // _lb._udp.0.0.168.192.in-addr.arpa. PTR my-discovery-proxy-en0.home.arpa.
    dnssd_hardwired_lbdomains_setup();

exit:
#endif
    return;
}

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static void
dnssd_hardwired_clear(void)
{
    INFO("Clearing all hardwired response");
    for (served_domain_t *domain = served_domains; domain != NULL; domain = domain->next) {
        hardwired_t *hardwired_responses = domain->hardwired_responses;
        if (hardwired_responses == NULL) {
            continue;
        }

        domain->hardwired_responses = NULL;
        hardwired_t *next_response;
        for (hardwired_t *response = hardwired_responses; response != NULL; response = next_response) {
            next_response = response->next;
            free(response);
        }
    }
}

static void
dnssd_hardwired_push_setup(void)
{
    // For each interface, set up the hardwired names.
    for (served_domain_t *sdt = served_domains; sdt; sdt = sdt->next) {
        if (sdt->interface == NULL) {
            continue;
        }

        if (!sdt->interface->no_push) {
            // SRV
            // _dns-push-tls._tcp
            // _dns-query-tls._udp
            dnssd_hardwired_setup_dns_push_for_domain(sdt);
        }
    }
}

static void
dnssd_hardwired_deny_service_existence_for_served_domain(served_domain_t *const NONNULL served_domain)
{
    dns_wire_t wire;
    dns_towire_state_t towire;

#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    RESET;
    // We deny the presence of support for LLQ, because we only support DNS Push.
    dnssd_hardwired_add(served_domain, "_dns-llq._udp", served_domain->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);
    dnssd_hardwired_add(served_domain, "_dns-llq-tls._tcp", served_domain->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);

    // We deny the presence of support for DNS Update, because a Discovery Proxy zone is stateless.
    dnssd_hardwired_add(served_domain, "_dns-update._udp", served_domain->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);
    dnssd_hardwired_add(served_domain, "_dns-update-tls._tcp", served_domain->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);

    // We deny the presence of support for UDP with TLS, because we have not implemented DTLS (datagram TLS).
    dnssd_hardwired_add(served_domain, "_dns-query-tls._udp", served_domain->domain_ld, towire.p - wire.data, wire.data, dns_rrtype_srv);

    // We deny the presence of "lb._dns-sd._udp" for the served domain, to avoid the response like:
    // lb._dns-sd._udp.openthread.thread.home.arpa. PTR openthread.thread.home.arpa.
    dnssd_hardwired_add(served_domain, SRV_TYPE_FOR_AUTOMATIC_BROWSING_DOMAIN, served_domain->domain_ld,
        towire.p - wire.data, wire.data, dns_rrtype_ptr);
    dnssd_hardwired_add(served_domain, "b._dns-sd._udp", served_domain->domain_ld,
        towire.p - wire.data, wire.data, dns_rrtype_ptr);
    dnssd_hardwired_add(served_domain, "db._dns-sd._udp", served_domain->domain_ld,
        towire.p - wire.data, wire.data, dns_rrtype_ptr);
}

static bool
dnssd_hardwired_setup_for_served_domain(served_domain_t *const NONNULL served_domain)
{
    bool succeeded = false;
    dns_wire_t wire;
    dns_towire_state_t towire;

#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    require_action_quiet(served_domain->interface != NULL, exit, succeeded = false;
        ERROR("only domain with usable interface can setup hardwired response - domain name: " PRI_S_SRP,
            served_domain->domain)
    );

    // deny the existence of the following services:
    // _dns-llq._udp
    // _dns-llq-tls._tcp
    // _dns-update._udp
    dnssd_hardwired_deny_service_existence_for_served_domain(served_domain);

    // Setup NS record for this served domain.
    RESET;
#if STUB_ROUTER
    if (string_ends_with(served_domain->domain, THREAD_DOMAIN)) {
        // If the response requires the translation from <served domain> to ".local." and the response ends in
        // ".local.", truncate it.
        require_action_quiet(local_host_name_dot_local[0] != 0, exit, succeeded = false);
        dns_full_name_to_wire(NULL, &towire, local_host_name_dot_local);
    } else
#endif
    if (uuid_name[0] != 0) {
        dns_name_to_wire(NULL, &towire, uuid_name);
        dns_full_name_to_wire(NULL, &towire, served_domain->domain);
    } else {
        dns_name_to_wire(NULL, &towire, "ns");
        dns_full_name_to_wire(NULL, &towire, served_domain->domain);
    }
    dnssd_hardwired_add(served_domain, "", served_domain->domain, towire.p - wire.data, wire.data, dns_rrtype_ns);

    // Setup SOA record for this served domain. (piggybacking on what we already did for NS, which starts the same.)
    dns_name_to_wire(NULL, &towire, "postmaster");
    dns_full_name_to_wire(NULL, &towire, served_domain->domain);
    dns_u32_to_wire(&towire, 0);     // serial
    dns_ttl_to_wire(&towire, 7200);  // refresh
    dns_ttl_to_wire(&towire, 3600);  // retry
    dns_ttl_to_wire(&towire, 86400); // expire
    dns_ttl_to_wire(&towire, 120);    // minimum
    dnssd_hardwired_add(served_domain, "", served_domain->domain, towire.p - wire.data, wire.data, dns_rrtype_soa);

    // Setup DNS push
    if (served_domain->interface == NULL || !served_domain->interface->no_push) {
        succeeded = dnssd_hardwired_setup_dns_push_for_domain(served_domain);
        if (!succeeded) {
            ERROR("failed to setup DNS push service for hardwired response - domain: " PRI_S_SRP,
                  served_domain->domain);
            goto exit;
        }
    }
    succeeded = true;

exit:
    return succeeded;
}

static bool
dnssd_hardwired_setup_dns_push_for_domain(served_domain_t *const NONNULL served_domain)
{
    bool succeeded;

    require_action_quiet(served_domain->interface != NULL && !served_domain->interface->no_push, exit, succeeded = false;
        ERROR("the associated interface does not enable DNS push - domain: " PRI_S_SRP, served_domain->domain));

    require_action_quiet(my_name != NULL, exit, succeeded = false; ERROR("my_name is not set"));

    dns_wire_t wire;
    dns_towire_state_t towire;
#define RESET \
    memset(&towire, 0, sizeof towire); \
    towire.message = &wire; \
    towire.p = wire.data; \
    towire.lim = towire.p + sizeof wire.data

    RESET;
    dns_u16_to_wire(&towire, 0); // priority
    dns_u16_to_wire(&towire, 0); // weight
    dns_u16_to_wire(&towire, 853); // port

#if STUB_ROUTER
    if (string_ends_with(served_domain->domain, THREAD_DOMAIN)) {
        // If the served domain is subdomain of "thread.home.arpa.", use name <local host name>.local for the DNS push
        // service. Currently we only support DNS push in "thread.home.arpa." domain in local subnet, so DNS push
        // service for "thread.home.arpa." will be registered with a name in ".local.".
        require_action_quiet(local_host_name_dot_local[0] != 0, exit, succeeded = false);
        dns_full_name_to_wire(NULL, &towire, local_host_name_dot_local);
    } else
#endif
    if (uuid_name[0] != 0) {
        // Use <local host name>.<domain>
        dns_name_to_wire(NULL, &towire, uuid_name);
        dns_full_name_to_wire(NULL, &towire, served_domain->domain);
    } else {
        // Use name ns.<served domain>.
        dns_name_to_wire(NULL, &towire, "ns");
        dns_full_name_to_wire(NULL, &towire, served_domain->domain);
    }

    dnssd_hardwired_add(served_domain, "_dns-push-tls._tcp", served_domain->domain_ld, towire.p - wire.data, wire.data,
        dns_rrtype_srv);

    succeeded = true;
exit:
    return succeeded;
}
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

static bool
embiggen(dnssd_query_t *query)
{
    dns_wire_t *nr = malloc(query->data_size + DNS_DATA_SIZE + DNS_HEADER_SIZE); // increments wire size by DNS_DATA_SIZE
    if (nr == NULL) {
        return false;
    }
    memcpy(nr, query->response, query->data_size + DNS_HEADER_SIZE);
    query->data_size += DNS_DATA_SIZE;
    size_t len = query->towire.p - &query->response->data[0];
    query->towire.p = &nr->data[0] + len;
    query->towire.lim = &nr->data[0] + query->data_size;
    query->towire.p_rdlength = NULL;
    query->towire.p_opt = NULL;
    query->towire.message = nr;
    free(query->response);
    query->response = nr;
    return true;
}

static void
dp_move_rrs(dns_rr_t *first_section, unsigned *p_first_count, dns_rr_t *source_section, unsigned source_count, unsigned count, bool rdata_present)
{
    unsigned first_count = *p_first_count;

    // Copy the rrs into the combined section.
    for (unsigned i = 0; i < source_count; i++) {
        // Skip this RR if there's already another one just like it in the section (most likely to happen
        // with authority records.
        bool duplicate = false;
        for (unsigned j = 0; j < first_count; j++) {
            if (dns_rrs_equal(&first_section[j], &source_section[i], rdata_present)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        // Only if there is space...
        if (first_count < count) {
            first_section[first_count] = source_section[i];
            first_count++;
            source_section[i].type = dns_invalid_rr;
        } else {
            ERROR("first_count %d unexpectedly equal to count %d with i = %d", first_count, count, i);
        }
    }
    *p_first_count = first_count;
}

static dnssd_query_t *
dp_dns_queries_finished(dnssd_query_t *answered_query)
{
    dns_message_t *first_message = NULL;
    unsigned qdcount = 0, ancount = 0, nscount = 0, arcount = 0;
    unsigned first_qdcount, first_ancount, first_nscount, first_arcount;
    dnssd_query_t *first_query = NULL;
    dp_tracker_t *tracker = answered_query->tracker;

    // response_query will be set to NULL if we don't want anything sent yet, to query if query is the
    // only query or if there's an error in this function, or to the query that holds the aggregate response
    // otherwise.
    dnssd_query_t *response_query = answered_query;
    const char *name = answered_query->question != NULL ? answered_query->question->name : "(null)";

    require_action_quiet(tracker != NULL, exit,
                         dns_rcode_set(answered_query->response, dns_rcode_servfail);
                         ERROR("NULL tracker on " PRI_S_SRP, name));
    require_action_quiet(tracker->dns_queries != NULL, exit,
                         dns_rcode_set(answered_query->response, dns_rcode_servfail);
                         ERROR("NULL tracker->dns_queries on " PRI_S_SRP, name));

    // The usual case, there's only one question in the DNS message, so we can just
    // return the answer now.
    if (answered_query->num_questions == 1) {
        goto exit;
    }

    // Otherwise, we have more than one query, so see if any remain unsatisfied.
    int satisfied = 0;
    for (dnssd_query_t *match = tracker->dns_queries; match != NULL; match = match->next) {
        // It's possible we could creep in here without actually generating one of the responses,
        // in which case we should definitely fail at this point.
        require_action_quiet(match->response != NULL,
                             exit,
                             ERROR("null response on match query");
                             dns_rcode_set(response_query->response, dns_rcode_servfail));
        if (match->message == answered_query->message) {
            if (!match->satisfied) {
                response_query = NULL; // More answers coming.
                goto exit;
            }
            satisfied++;
        }
    }
    if (satisfied != answered_query->num_questions) {
        response_query = NULL;
        goto exit;
    }
    // All queries have been satisfied.

    // Parse all of the messages (this is gross--later on we should just never convert to wire format until
    // we get here.
    for (dnssd_query_t *source = tracker->dns_queries; source != NULL; source = source->next) {
        if (source->message == answered_query->message) {
            // This should never fail, but...
            require_action_quiet(dns_wire_parse(&source->response_msg, source->response,
                                                (unsigned)(source->towire.p - source->response->data) + DNS_HEADER_SIZE, false),
                                 exit,
                                 dns_rcode_set(response_query->response, dns_rcode_servfail));

            if (first_query == NULL) {
                first_query = source;
                first_message = first_query->response_msg;
                first_qdcount = first_message->qdcount;
                first_ancount = first_message->ancount;
                first_nscount = first_message->nscount;
                first_arcount = first_message->arcount;
            }
            qdcount += source->response_msg->qdcount;
            ancount += source->response_msg->ancount;
            nscount += source->response_msg->nscount;
            arcount += source->response_msg->arcount;
        }
    }

    // Copy records from the response.
    for (int i = 0; i < 4; i++) {
        dns_rr_t *section, **first_section = NULL, **source_section = NULL;
        unsigned section_count = 0, source_count = 0, *first_count = NULL;

        // Start with the second message, since the first is already populated.
        for (dnssd_query_t *source = tracker->dns_queries;
             source != NULL && answered_query->message == source->message ; source = source->next)
        {
#define SECTION_CASE(index, counter_name, section_name)                   \
            case index:                                                   \
                first_section = &first_message->section_name;             \
                source_section = &source->response_msg->section_name;     \
                section_count = counter_name;                             \
                source_count = source->response_msg->counter_name;        \
                first_count = &first_message->counter_name;               \
                break

            switch (i) {
                    SECTION_CASE(0, qdcount, questions);
                    SECTION_CASE(1, ancount, answers);
                    SECTION_CASE(2, nscount, authority);
                    SECTION_CASE(3, arcount, additional);
            }

            // If this is the first matching query, expand the current section to be able to fit all of the data we're
            // copying in, and then copy the data from the first section.
            if (first_section == source_section) {
                section = calloc(section_count, sizeof(*section));
                require_action_quiet(section != NULL, exit,
                                     dns_rcode_set(answered_query->response, dns_rcode_servfail);
                                     ERROR("Unable to allocate memory for query response section on " PRI_S_SRP, name));
                memcpy(section, *first_section, source_count * sizeof(*section));
                memset(*first_section, 0, source_count * sizeof(*section)); // NULL out any pointers
                free(*first_section);
                *first_section = section;
            } else {
                dp_move_rrs(*first_section, first_count, *source_section, source_count, section_count, i != 0);
            }
        }
    }

    // Use the response in the first query to turn the answer to wire format.
redo_message:
    dp_query_towire_reset(first_query);
    dns_message_rrs_to_wire(&first_query->towire, first_query->response_msg);
    if (first_query->towire.truncated) {
        if (first_query->tracker->connection->tcp_stream) {
            if (embiggen(first_query)) {
                first_query->towire.error = false;
                first_query->towire.truncated = false;
                goto redo_message;
            }
        }
    }
    first_query->response->qdcount = htons(first_message->qdcount);
    first_query->response->ancount = htons(first_message->ancount);
    first_query->response->nscount = htons(first_message->nscount);
    first_query->response->arcount = htons(first_message->arcount);
    response_query = first_query;

exit:
    return response_query;
}

static void
dp_query_send_dns_response(dnssd_query_t *query, const char *context_description)
{
    struct iovec iov;
    dns_towire_state_t *towire = &query->towire;
    const char *failnote = NULL;
    uint8_t *revert = towire->p;
    uint16_t tc = towire->truncated ? dns_flags_tc : 0;
    uint16_t bitfield = ntohs(query->response->bitfield);
    uint16_t mask = 0;
    int rcode = dns_rcode_get(query->response);
    question_t *question = query->question;

    // Mark this query as complete.
    query->satisfied = true;

    VALIDATE_TRACKER_CONNECTION_NON_NULL();

    // Send an SOA record if it's a .local query.
    if (question->served_domain != NULL && question->served_domain->interface != NULL &&
        !towire->truncated && (question->type != dns_rrtype_soa || question->name[0] != '\0'))
    {
    redo:
        // DNSSD Hybrid, Section 6.1.
        TOWIRE_CHECK("&query->enclosing_domain_pointer 1", towire,
                     dns_pointer_to_wire(NULL, towire, &query->enclosing_domain_pointer));
        TOWIRE_CHECK("dns_rrtype_soa", towire,
                     dns_u16_to_wire(towire, dns_rrtype_soa));
        TOWIRE_CHECK("dns_qclass_in", towire,
                     dns_u16_to_wire(towire, dns_qclass_in));
        TOWIRE_CHECK("ttl", towire, dns_ttl_to_wire(towire, 3600));
        TOWIRE_CHECK("rdlength_begin ", towire, dns_rdlength_begin(towire));
        if (0) {
#if STUB_ROUTER
        } else if (srp_servers->stub_router_enabled && my_name != NULL) {
            TOWIRE_CHECK(my_name, towire, dns_full_name_to_wire(NULL, towire, my_name));
#endif
        } else if (uuid_name[0] != 0) {
            TOWIRE_CHECK("uuid_name", towire, dns_name_to_wire(NULL, towire, uuid_name));
            TOWIRE_CHECK("&query->enclosing_domain_pointer 2", towire,
                         dns_pointer_to_wire(NULL, towire, &query->enclosing_domain_pointer));
        } else {
            TOWIRE_CHECK("\"ns\"", towire, dns_name_to_wire(NULL, towire, "ns"));
            TOWIRE_CHECK("&query->enclosing_domain_pointer 2", towire,
                         dns_pointer_to_wire(NULL, towire, &query->enclosing_domain_pointer));
        }
        TOWIRE_CHECK("\"postmaster\"", towire,
                     dns_name_to_wire(NULL, towire, "postmaster"));
        TOWIRE_CHECK("&query->enclosing_domain_pointer 3", towire,
                     dns_pointer_to_wire(NULL, towire, &query->enclosing_domain_pointer));
        TOWIRE_CHECK("serial", towire,dns_u32_to_wire(towire, 0));     // serial
        TOWIRE_CHECK("refresh", towire, dns_ttl_to_wire(towire, 7200));  // refresh
        TOWIRE_CHECK("retry", towire, dns_ttl_to_wire(towire, 3600));  // retry
        TOWIRE_CHECK("expire", towire, dns_ttl_to_wire(towire, 86400)); // expire
        TOWIRE_CHECK("minimum", towire, dns_ttl_to_wire(towire, 120));    // minimum
        dns_rdlength_end(towire);
        if (towire->truncated) {
            query->towire.p = revert;
            if (query->tracker->connection->tcp_stream) {
                if (embiggen(query)) {
                    query->towire.error = 0;
                    towire->truncated = false;
                    goto redo;
                }
            } else {
                tc = dns_flags_tc;
            }
        } else {
            query->response->nscount = htons(1);
        }

        // Response is authoritative and not recursive.
    authoritative:
        mask = ~dns_flags_ra;
        bitfield = bitfield | dns_flags_aa | tc;
        bitfield = bitfield & mask;
    } else {
        // Response is recursive and not authoritative.
        if (question->type != dns_rrtype_soa) {
            mask = ~dns_flags_aa;
            bitfield = bitfield | dns_flags_ra | tc;
            bitfield = bitfield & mask;
        } else {
            goto authoritative;
        }
    }

    INFO("[Q%d][QU%d][QID %x] ->p %p ->lim %p len %zd rcode %d " PUB_S_SRP, SERIAL(query), SERIAL(question),
         ntohs(query->message->wire.id), query->towire.p, &query->towire.message->data[0],
         query->towire.p - &query->towire.message->data[0], dns_rcode_get(query->response), context_description);

    // In the case that we get an error looking something up, we return that error immediately on the query that failed,
    // rather than trying to assemble a complete answer. In returning the error, we cancel any outstanding queries.
    dnssd_query_t *send_query;

    if (!towire->error && rcode == dns_rcode_noerror) {
        // It's possible that we got a query with qdcount > 1. In this case, we are going to marshal all of the
        // answers from the responses we've constructed into a new response and send it after all of the queries
        // have responses.  So at this point, if we don't have all the responses yet, there's no point in adding
        // the edns0 option.  If we do, dp_dns_update_queries_finished will marshal all the answers into one
        // message and after that we can add the edns0 option. If there's only one query, this is a no-op.
        send_query = dp_dns_queries_finished(query);
        if (send_query == NULL) {
#ifdef DNSSD_PROXY_DUMP_TRACKER_QUERIES
            if (query->tracker == NULL) {
                ERROR("[Q%d] query->tracker is NULL", SERIAL(query));
            } else {
                char logbuf[200];
                char *lbp = logbuf;
                char *lbend = logbuf + sizeof(logbuf);
                char *lbrestart;
                bool print_last = true;
                if (query->tracker->connection != NULL && query->tracker->connection->tcp_stream) {
                    int len = snprintf(logbuf, sizeof(logbuf), "[Q%d] TCP %d: ", SERIAL(query), query->num_questions);
                    lbrestart = logbuf + len;
                } else {
                    int len = snprintf(logbuf, sizeof(logbuf), "[Q%d] UDP %p %d: ", SERIAL(query), query->num_questions);
                    lbrestart = logbuf + len;
                }
                lbp = logbuf + strlen(logbuf);
                for (dnssd_query_t *list_query = query->tracker->dns_queries;
                     list_query != NULL; list_query = list_query->next)
                {
                    if (list_query->message != query->message) {
                        continue;
                    }
                    int len = snprintf(lbp, lbend - lbp, "%p%s ", list_query, list_query->satisfied ? "+" : "=");
                    if (lbp + len < lbend) {
                        lbp += len;
                        print_last = true;
                    } else {
                        *lbp = 0;
                        INFO("[Q%d] " PUB_S_SRP, SERIAL(query), logbuf);
                        lbp = lbrestart;
                        *lbp = 0;
                        print_last = false;
                    }
                }
                if (print_last) {
                    INFO("[Q%d] " PUB_S_SRP, SERIAL(query), logbuf);
                }
            }
#endif // DNSSD_PROXY_DUMP_TRACKER_QUERIES
            return;
        }
        if (dns_rcode_get(send_query->response) != dns_rcode_noerror) {
            rcode = dns_rcode_get(send_query->response);
        }

        towire = &send_query->towire;
        revert = towire->p;
    } else {
        send_query = query;
    }

    // Not authentic, checking not disabled.
    mask = ~(dns_flags_rd | dns_flags_ad | dns_flags_cd);
    bitfield = bitfield & mask;
    send_query->response->bitfield = htons(bitfield);

    // This is a response
    dns_qr_set(send_query->response, dns_qr_response);

    // If we got a failure from dp_dns_queries_finished(), skip adding the opt RR and checking for a towire error.
    if (rcode == dns_rcode_noerror) {
    // Send an OPT RR if we got one
    // XXX reserve space so we can always send an OPT RR?
        if (send_query->is_edns0) {
        redo_edns0:
            TOWIRE_CHECK("Root label", towire, dns_u8_to_wire(towire, 0));     // Root label
            TOWIRE_CHECK("dns_rrtype_opt", towire, dns_u16_to_wire(towire, dns_rrtype_opt));
            TOWIRE_CHECK("UDP Payload size", towire, dns_u16_to_wire(towire, 4096)); // UDP Payload size
            TOWIRE_CHECK("extended-rcode", towire, dns_u8_to_wire(towire, 0));     // extended-rcode
            TOWIRE_CHECK("EDNS version 0", towire, dns_u8_to_wire(towire, 0));     // EDNS version 0
            TOWIRE_CHECK("No extended flags", towire, dns_u16_to_wire(towire, 0));    // No extended flags
            TOWIRE_CHECK("No payload", towire, dns_u16_to_wire(towire, 0));    // No payload
            if (towire->truncated) {
                send_query->towire.p = revert;
                if (send_query->tracker->connection->tcp_stream) {
                    if (embiggen(send_query)) {
                        send_query->towire.error = false;
                        send_query->towire.truncated = false;
                        goto redo_edns0;
                    }
                }
            } else {
#if SRP_FEATURE_NAT64
                send_query->response->arcount = htons(ntohs(send_query->response->arcount) + 1);
#else
                send_query->response->arcount = htons(1);
#endif
            }
        }

        if (towire->error) {
            ERROR("[Q%d][QU%d][QID%x] failed on %s", SERIAL(query), SERIAL(question), ntohs(send_query->message->wire.id), failnote);
            if (tc == dns_flags_tc) {
                dns_rcode_set(send_query->response, dns_rcode_noerror);
            } else {
                dns_rcode_set(send_query->response, dns_rcode_servfail);
            }
        } else {
            // No error.
            dns_rcode_set(send_query->response, dns_rcode_noerror);
        }
    } else {
        dns_rcode_set(send_query->response, rcode);
    }

    iov.iov_len = (send_query->towire.p - (uint8_t *)send_query->response);
    iov.iov_base = send_query->response;
    INFO("[Q%d][QU%d] (len %zd)", SERIAL(query), SERIAL(question), iov.iov_len);

    ioloop_send_message(send_query->tracker->connection, send_query->message, &iov, 1);

    // Cancel the send_query.
    dnssd_query_cancel(send_query);
}

static void
dp_query_towire_reset(dnssd_query_t *query)
{
    query->towire.p = &query->response->data[0];  // We start storing RR data here.
    query->towire.lim = &query->response->data[0] + query->data_size; // This is the limit to how much we can store.
    query->towire.message = query->response;
    query->towire.p_rdlength = NULL;
    query->towire.p_opt = NULL;
    query->p_dso_length = NULL;
}

static void
dns_push_start(dnssd_query_t *query)
{
    const char *failnote = NULL;

    // If we don't have a dso header yet, start one.
    if (query->p_dso_length == NULL) {
        memset(query->response, 0, (sizeof *query->response) - DNS_DATA_SIZE);
        dns_opcode_set(query->response, dns_opcode_dso);
        // This is a unidirectional DSO message, which is marked as a query
        dns_qr_set(query->response, dns_qr_query);
        // No error cuz not a response.
        dns_rcode_set(query->response, dns_rcode_noerror);

        TOWIRE_CHECK("kDSOType_DNSPushUpdate", &query->towire,
                     dns_u16_to_wire(&query->towire, kDSOType_DNSPushUpdate));
        if (query->towire.p + 2 > query->towire.lim) {
            ERROR("[Q%d] No room for dso length in DNS Push notification message.", SERIAL(query));
            dp_query_towire_reset(query);
            return;
        }
        query->p_dso_length = query->towire.p;
        query->towire.p += 2;
    }
    if (failnote != NULL) {
        ERROR("[Q%d] couldn't start update: %s", SERIAL(query), failnote);
    }
}

static void
dp_push_response(dnssd_query_t *query, dns_rr_t *original_question)
{
    struct iovec iov;
    question_t *question = query->question;
    char nbuf[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    char *name = "<null question name>";
    if (question != NULL) {
        name = question->name;
    } else if (original_question != NULL) {
        dns_name_print(original_question->name, nbuf, sizeof(nbuf));
        name = nbuf;
    }

    VALIDATE_TRACKER_CONNECTION_NON_NULL();

    if (query->p_dso_length != NULL) {
        int16_t dso_length = query->towire.p - query->p_dso_length - 2;
        iov.iov_len = (query->towire.p - (uint8_t *)query->response);
        iov.iov_base = query->response;
        INFO("[Q%d][QU%d] " PRI_S_SRP " (len %zd)", SERIAL(query), SERIAL(question), name, iov.iov_len);

        query->towire.p = query->p_dso_length;
        dns_u16_to_wire(&query->towire, dso_length);
        ioloop_send_message(query->tracker->connection, query->message, &iov, 1);
        dp_query_towire_reset(query);
    }
}

static bool
dnssd_hardwired_response(dnssd_query_t *query, DNSServiceQueryRecordReply UNUSED callback)
{
    hardwired_t *hp;
    question_t *question = query->question;
    const char *response_type = NULL;
    uint8_t v4mapped[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

    // If the question is for our uuid name in a domain we're authoritative for, respond with the IP address that
    // the question was received on.
    if (!strcasecmp(question->name, uuid_name) && (question->type == dns_rrtype_a || question->type == dns_rrtype_aaaa)) {
        addr_t *local = NULL;
        if (query->message != NULL) {
            local = &query->message->local;
        } else {
            local = &query->dso->transport->local;
        }

        // If it's an IPv4 address we can respond with an A record.
        if (question->type == dns_rrtype_a && local->sa.sa_family == AF_INET) {
            dp_query_add_data_to_response(query, question->name, question->type, dns_qclass_in, 4,
                                          &local->sin.sin_addr, 300, true, true, &query->response->ancount);
            response_type = "local host IPv4 address";
        }
        // If it's an IPv4-mapped IPv6 address, we can respond with an A record
        else if (local->sa.sa_family == AF_INET6 && question->type == dns_rrtype_a &&
                 !memcmp(&local->sin6.sin6_addr, v4mapped, sizeof(v4mapped)))
        {
            dp_query_add_data_to_response(query, question->name, question->type, dns_qclass_in, 4,
                                          ((uint8_t *)&local->sin6.sin6_addr) + 12, 3600, true, true,
                                          &query->response->ancount);
            response_type = "local host v4-mapped address";
        }
        // If it's an IPv6 address and NOT a v4-mapped address, we can respond with an AAAA record.
        else if (local->sa.sa_family == AF_INET6 && question->type == dns_rrtype_aaaa &&
                 memcmp(&local->sin6.sin6_addr, v4mapped, sizeof(v4mapped)))
        {
            struct in6_addr response = local->sin6.sin6_addr;
            bool address_is_usable = false;
            // If it's not a synthesized anycast or rloc address, we can just use it.
            if (!is_thread_mesh_synthetic_address(&response)) {
                address_is_usable = true;
                response_type = "local host IPv6 address";
            } else {
                // Otherwise, we need to find the mesh-local address and respond with that.
                srp_server_t *server_state = srp_servers;
#if SRP_TEST_SERVER
                for (; !address_is_usable && server_state != NULL; server_state = server_state->next)
#endif
                {
                    if (0) {
#if STUB_ROUTER
                    } else if (server_state->stub_router_enabled) {
                        route_state_t *route_state = server_state->route_state;
                        if (route_state->thread_interface_name != NULL) {
                            for (interface_address_state_t *address = route_state->interface_addresses;
                                 !address_is_usable && address != NULL; address = address->next)
                            {
                                // Wrong interface or wrong type of address
                                if (strcmp(address->name, route_state->thread_interface_name) ||
                                    address->addr.sa.sa_family != AF_INET6)
                                {
                                    continue;
                                }
                                if (!is_thread_mesh_synthetic_or_link_local(&address->addr.sin6.sin6_addr))
                                {
                                    memcpy(&response, &address->addr.sin6.sin6_addr, sizeof(response));
                                    response_type = "thread interface address";
                                    address_is_usable = true;
                                }
                            }
                            if (address_is_usable == false) {
                                response_type = "no usable address on thread interface";
                            }
                        } else {
                            address_is_usable = false;
                            response_type = "thread interface name unknown";
                        }
#endif
                    } else {
                        // For thread device, the only thing that can work is the ML-EID.
                        if (service_publisher_get_ml_eid(server_state->service_publisher, &response)) {
                            address_is_usable = true;
                            response_type = "thread device ML-EID";
                        } else {
                            response_type = "thread ML-EID not known";
                        }
                    }
                }
            }
            if (address_is_usable) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(&response, response_buf);
                INFO(PUB_S_SRP " IN AAAA " PRI_SEGMENTED_IPv6_ADDR_SRP " " PUB_S_SRP, question->name,
                     SEGMENTED_IPv6_ADDR_PARAM_SRP(&response, response_buf), response_type);
                dp_query_add_data_to_response(query, question->name, question->type, dns_qclass_in, 16,
                                              &response, 300, true, true, &query->response->ancount);
            }
        }
    } else {
        for (hp = query->question->served_domain->hardwired_responses; hp; hp = hp->next) {
            if ((query->question->type == hp->type || query->question->type == dns_rrtype_any) &&
                query->question->qclass == dns_qclass_in && !strcasecmp(hp->name, query->question->name))
            {
                if (query->dso != NULL) {
                    // Since hardwired response is set by the dnssd-proxy itself, do not do ".local" translation.
                    dp_query_add_data_to_response(query, hp->fullname, hp->type, dns_qclass_in, hp->rdlen, hp->rdata,
                                                  3600, true, false, NULL);
                } else {
                    // Store the response
                    if (!query->towire.truncated) {
                        // Since hardwired response is set by the dnssd-proxy itself, do not do ".local" translation.
                        dp_query_add_data_to_response(query, hp->fullname, hp->type, dns_qclass_in,
                                                      hp->rdlen, hp->rdata, 3600, true, false,
                                                      &query->response->ancount);
                    }
                }
                response_type = "hardwired";
            }
        }
    }
    if (response_type != NULL) {
        return true;
    }
    return false;
}

#if SRP_FEATURE_NAT64
static void
dp_query_append_nat64_prefix_records(dnssd_query_t *query)
{
    // 192.0.0.170 and 192.0.0.171 are reserved IPv4 addresses for ipv4only.arpa.
    // See <https://tools.ietf.org/html/rfc7050#section-8.2>.
    const uint8_t ipv4_addrs[2][4] = {
        {192, 0, 0, 170},
        {192, 0, 0, 171}
    };
    uint8_t rdata[16] = {0};

    VALIDATE_TRACKER_CONNECTION_NON_NULL();

    const struct in6_addr *prefix = nat64_get_ipv6_prefix();
    memcpy(rdata, prefix->s6_addr, sizeof(rdata));
    for (size_t i = 0; i < countof(ipv4_addrs);) {
        memcpy(&rdata[12], ipv4_addrs[i], 4);
        uint8_t *revert = query->towire.p;
        dp_query_add_data_to_response(query, "ipv4only.arpa.", dns_rrtype_aaaa, query->question->qclass,
                                      (uint16_t)sizeof(rdata), rdata, RFC8766_TTL_CLAMP, true, false,
                                      &query->response->arcount);
        if (query->towire.truncated) {
            query->towire.p = revert;
            if (query->tracker->connection->tcp_stream) {
                if (embiggen(query)) {
                    query->towire.truncated = false;
                    query->towire.error = false;
                    continue;
                } else {
                    dns_rcode_set(query->response, dns_rcode_servfail);
                }
            }
            return;
        }
        i++;
    }
}
#endif // SRP_FEATURE_NAT64

static void
dns_query_answer_process(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                         const char *fullname, uint16_t rrtype, uint16_t rrclass,
                         uint16_t rdlen, const void *rdata, uint32_t ttl, dnssd_query_t *query, bool send)
{
    question_t *question = query->question;

    INFO("[Q%d][QU%d] " PRI_S_SRP PUB_S_SRP PUB_S_SRP " %d %x %d %p", SERIAL(query), SERIAL(question),
         fullname, (flags & kDNSServiceFlagsMoreComing) ? " m " : " ", dns_rrtype_to_string(rrtype), rrclass, rdlen,
         errorCode, query);

    VALIDATE_TRACKER_CONNECTION_NON_NULL();

    if (errorCode == kDNSServiceErr_NoError) {
#if SRP_FEATURE_NAT64
        const bool aaaa_query_got_a_record = (question->type == dns_rrtype_aaaa) && (rrtype == dns_rrtype_a);
        if (srp_servers->srp_nat64_enabled && (ntohs(query->response->arcount) != 0) && !aaaa_query_got_a_record) {
            return;
        }
#endif
    re_add:
        if (send) {
            uint16_t *counter = &query->response->ancount;
#if SRP_FEATURE_NAT64
            if (srp_servers->srp_nat64_enabled && aaaa_query_got_a_record) {
                counter = &query->response->arcount;
            }
#endif
            uint8_t *revert = query->towire.p;
            dp_query_add_data_to_response(query, fullname, rrtype, rrclass, rdlen, rdata,
                                          ttl > RFC8766_TTL_CLAMP ? RFC8766_TTL_CLAMP : ttl, false, false, counter);
            if (query->towire.truncated) {
                query->towire.p = revert;
                if (query->tracker->connection->tcp_stream) {
                    if (embiggen(query)) {
                        query->towire.truncated = false;
                        query->towire.error = false;
                        goto re_add;
                    } else {
                        dns_rcode_set(query->response, dns_rcode_servfail);
                        dp_query_send_dns_response(query, "failed embiggen");
                        return;
                    }
                }
            }
        }
        // If there isn't more coming, send the response now
        if (!(flags & kDNSServiceFlagsMoreComing) || query->towire.truncated) {
            // When we get a CNAME response, we may not get the record it points to with the MoreComing
            // flag set, so don't respond yet.
            if (question->type != dns_rrtype_cname && rrtype == dns_rrtype_cname) {
                INFO("[Q%d][QU%d] not responding yet because CNAME.", SERIAL(query), SERIAL(question));
            } else {
#if SRP_FEATURE_NAT64
                if (srp_servers->srp_nat64_enabled && (ntohs(query->response->arcount) != 0)) {
                    dp_query_append_nat64_prefix_records(query);
                }
#endif
                dp_query_send_dns_response(query, "normal success");
            }
        }
    } else if (errorCode == kDNSServiceErr_NoSuchRecord) {
        // If we get "no such record," we can't really do much except return the answer.
        dp_query_send_dns_response(query, "no such record");
    } else {
        dns_rcode_set(query->response, dns_rcode_servfail);
        dp_query_send_dns_response(query, "unhandled error");
    }
}

// answer_match is to decide if an answer matches the one requested to be removed.
// Based on rfc8765
// Remove all RRsets from a name in all classes:
//      TTL = 0xFFFFFFFE, RDLEN = 0, CLASS = 255 (ANY).
// Remove all RRsets from a name in given class:
//      TTL = 0xFFFFFFFE, RDLEN = 0, CLASS gives class, TYPE = 255 (ANY).
// Remove specified RRset from a name in given class:
//      TTL = 0xFFFFFFFE, RDLEN = 0,
//      CLASS and TYPE specify the RRset being removed.
// Remove an individual RR from a name:
//      TTL = 0xFFFFFFFF,
//      CLASS, TYPE, RDLEN, and RDATA specify the RR being removed.
static bool
answer_match(const answer_t *answer, uint32_t rdlen, const char *fullname, uint16_t rrtype, uint16_t rrclass, const void *rdata)
{
    return (((rrclass == dns_qclass_any) || (rrclass == answer->rrclass)) &&
            ((rrtype == dns_rrtype_any) || (rrtype == answer->rrtype)) &&
            ((rdlen == 0) || ((rdlen == answer->rdlen) && (memcmp(answer->rdata, rdata, rdlen) == 0))) &&
            (!strcmp(answer->fullname, fullname))
            );
}

static void
dns_push_query_answer_process(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                              const char *fullname, uint16_t rrtype, uint16_t rrclass,
                              uint16_t rdlen, const void *rdata, uint32_t ttl, dnssd_query_t *query, bool send);

// This is the callback for both dns query and dns push query results.
static void
dns_question_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                      DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                      uint16_t rdlen, const void *rdata, uint32_t ttl, void *context)
{
    question_t *question = context;
    dnssd_query_t *query, *next;
    bool send = true;

    // For dns push query,  insert or remove answer from the question cache depending on the flags
    // For dns query (dso==NULL), add answer when receiving callback to the question
    if (errorCode == kDNSServiceErr_NoError) {
        if (flags & kDNSServiceFlagsAdd) {
            // Eliminate duplicates (appears on more than one interface)
            for (answer_t *answer = question->answers; answer != NULL; answer = answer->next) {
                if (answer_match(answer, rdlen, fullname, rrtype, rrclass, rdata)) {
                    INFO("[QU%d] duplicate answer in cache - name: " PRI_S_SRP ", rrtype: " PUB_S_SRP
                         ", rrclass: " PUB_S_SRP ", rdlen: %u." PUB_S_SRP, SERIAL(question),
                         fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen,
                         (flags & kDNSServiceFlagsMoreComing) ? " more coming" : " done");
                    send = false;
                    break;
                }
            }
            if (send) {
                // Add the extra space rdlen stores rdata at the end
                answer_t *answer = calloc(1, sizeof(*answer) + rdlen);
                if (answer == NULL) {
                    ERROR("[QU%d] unable to allocate memory for answer - name: " PRI_S_SRP ", rrtype: " PUB_S_SRP
                          ", rrclass: " PUB_S_SRP ", rdlen: %u.", SERIAL(question),
                          fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen);
                    return;
                }
                answer->fullname = strdup(fullname);
                if (answer->fullname == NULL) {
                    ERROR("[QU%d] strdup failed to copy the answer name: " PRI_S_SRP, SERIAL(question), fullname);
                    free(answer);
                    return;
                }
                answer->interface_index = interfaceIndex;
                answer->ttl = ttl;
                answer->rrtype = rrtype;
                answer->rrclass = rrclass;
                answer->rdlen = rdlen;
                answer->rdata = (uint8_t *)(answer + 1);
                memcpy(answer->rdata, rdata, rdlen);
                answer->next = NULL;
                // Insert answer at the tail
                answer_t **tail = &(question->answers);
                while (*tail != NULL) {
                    tail = &((*tail)->next);
                }
                *tail = answer;
                // Received data; reset no_data flag.
                question->no_data = false;
                INFO("[QU%d] add answer to cache - name: " PRI_S_SRP ", rrtype: " PUB_S_SRP
                     ", rrclass: " PUB_S_SRP ", rdlen: %u." PUB_S_SRP, SERIAL(question),
                     fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen,
                 (flags & kDNSServiceFlagsMoreComing) ? " more coming" : " done");
            }
        } else {
            // Remove
            answer_t **answer = &(question->answers);
            answer_t *cur = NULL;
            bool matched = false;
            while (*answer != NULL) {
                cur = *answer;
                if (answer_match(cur, rdlen, fullname, rrtype, rrclass, rdata)) {
                    INFO("[QU%d] remove answer from cache - "
                         "name: " PRI_S_SRP ", rrtype: " PUB_S_SRP ", rrclass: " PUB_S_SRP ", rdlen: %u." PUB_S_SRP,
                         SERIAL(question), fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen,
                         (flags & kDNSServiceFlagsMoreComing) ? " more coming" : " done");
                    *answer = cur->next;
                    dp_answer_free(cur);
                    matched = true;
                    // If individual RR to be removed, get out of the loop once the RR has been removed
                    if (rdlen != 0) {
                        break;
                    }
                } else {
                    answer = &cur->next;
                }
            }
            if (!matched) {
                INFO("[Q%d] remove not found in cache - name: " PRI_S_SRP
                     ", rrtype: " PUB_S_SRP ", rrclass: " PUB_S_SRP ", rdlen: %u." PUB_S_SRP, SERIAL(question),
                     fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen,
                     (flags & kDNSServiceFlagsMoreComing) ? " more coming" : " done");
            }
            if (*answer == NULL) {
                // All the answers get removed; set no_data flag.
                question->no_data = true;
            }
        }
    } else if (errorCode == kDNSServiceErr_NoSuchRecord) {
        INFO("[QU%d] no data - name: " PRI_S_SRP ", rrtype: " PUB_S_SRP
             ", rrclass: " PUB_S_SRP ", rdlen: %u." PUB_S_SRP, SERIAL(question), fullname, dns_rrtype_to_string(rrtype),
             dns_qclass_to_string(rrclass), rdlen, (flags & kDNSServiceFlagsMoreComing) ? " more coming" : " done");
        question->no_data = true;
    } else if (errorCode == kDNSServiceErr_ServiceNotRunning || errorCode == kDNSServiceErr_DefunctConnection) {
#if SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTIONS
        if (shared_discovery_txn != NULL) {
            ioloop_dnssd_txn_cancel(shared_discovery_txn);
            ioloop_dnssd_txn_release(shared_discovery_txn);
            shared_discovery_txn = NULL;
            dp_handle_server_disconnect(NULL, errorCode);
        }
#else
        ioloop_dnssd_txn_cancel(question->txn);
        ioloop_dnssd_txn_release(question->txn);
        question->txn = NULL;
        dp_handle_server_disconnect(NULL, errorCode);
#endif
        return; // This doesn't count as a result.
    }
    query = question->queries;
    while(query != NULL) {
        next = query->question_next;
        if (query->dso != NULL) {
            dns_push_query_answer_process(flags, errorCode, fullname, rrtype, rrclass,
                                          rdlen, rdata, ttl, query, send);
        } else {
            dns_query_answer_process(flags, errorCode, fullname, rrtype, rrclass,
                                     rdlen, rdata, ttl, query, send);
        }
        query = next;
    }
    dp_question_cache_remove_queries(question);
}

static void
dp_query_wakeup(void *context)
{
    dnssd_query_t *query = context;
    char name[DNS_MAX_NAME_SIZE + 1];
    size_t namelen = strlen(query->question->name);
    question_t *question = query->question;

    if (question->answers != NULL) {
        FAULT("[Q%d][QU%d] answers present, but dp_query_wakeup reached for name " PRI_S_SRP,
              SERIAL(query), SERIAL(question), question->name);
    } else {
        question->no_data = true;
    }

    // Should never happen.
    if (namelen + (question->served_domain
                   ? (question->served_domain->interface != NULL
                      ? sizeof local_suffix
                      // XXX why are we checking this but not copying in the served domain name below?
                      : strlen(question->served_domain->domain_ld) + 1)
                   : 0) > sizeof name) {
        ERROR("[Q%d][QU%d] no space to construct name.", SERIAL(query), SERIAL(question));
        dnssd_query_cancel(query);
        return;
    }

    memcpy(name, question->name, namelen + 1);
    if (question->served_domain != NULL) {
        memcpy(name + namelen, local_suffix, sizeof(local_suffix));
    }
    RETAIN_HERE(query, dnssd_query);
    dp_query_send_dns_response(query, "query wakeup");
    dp_question_cache_remove_queries(question);
    RELEASE_HERE(query, dnssd_query);
}

// Search asked question in the cache; if not existing, create one.
static question_t *
dp_query_question_cache_copy(dns_rr_t *search_term, bool *new)
{
    char name[DNS_MAX_NAME_SIZE + 1];
    served_domain_t *sdt = dp_served(search_term->name, name, sizeof(name));

    if (sdt == NULL) {
        dns_name_print(search_term->name, name, sizeof name);
    }
    question_t **questions, *ret = NULL;
    question_t *new_question = NULL;
    // if the query is in served domain, lookup in served_domain->questions
    // otherwise lookup in the out-of-domain question cache
    if (sdt != NULL) {
        questions = &sdt->questions;
    } else {
        questions = &questions_without_domain;
    }
    *new = false;
    while (*questions != NULL) {
        question_t *question = *questions;
        if (search_term->type == question->type &&
            search_term->qclass == question->qclass &&
            !strcmp(name, question->name))
        {
            ret = question;
            break;
        }
        questions = &question->next;
    }

    // If no cache entry was found, create one
    if (*questions == NULL) {
        new_question = calloc(1, sizeof(*new_question));
        require_action_quiet(new_question != NULL, exit,
                             ERROR("Unable to allocate memory for question entry on " PRI_S_SRP, name));
        new_question->name = strdup(name);
        require_action_quiet(new_question->name != NULL, exit,
                             ERROR("unable to allocate memory for question name on " PRI_S_SRP, name));
        new_question->type = search_term->type;
        new_question->qclass = search_term->qclass;
        new_question->start_time = srp_utime();
        new_question->answers = NULL;
        new_question->served_domain = sdt;
        new_question->queries = NULL;
        new_question->no_data = false;
        new_question->serial = ++cur_question_serial;

        if (sdt != NULL && sdt->interface != NULL) {
            new_question->interface_index = sdt->interface->ifindex;
            new_question->serviceFlags = kDNSServiceFlagsForceMulticast;
        } else {
            new_question->interface_index = kDNSServiceInterfaceIndexAny;
            new_question->serviceFlags = kDNSServiceFlagsReturnIntermediates;
        }

        // Link the new_question to the question list.
        new_question->next = *questions;
        *questions = new_question;
        RETAIN_HERE(*questions, question); // retain

        // Successfully created a new question, which will be the returned question.
        ret = new_question;
        new_question = NULL;
        *new = true;
    }
exit:
    if (new_question != NULL) {
        RELEASE_HERE(new_question, question);
    }
    if (ret != NULL) {
        RETAIN_HERE(ret, question);
    }
    return ret;
}

// Look for answers in the cache for the current query. Remove flag is used when mDNSResponder connection is broken, to signal to
// DNS Push clients only that the records previously sent should be discarded.
static void
dp_query_reply_from_cache(question_t *question, dnssd_query_t *query, bool remove)
{
    // For dns query, if no_data is flagged or it's been six seconds since the question
    // was started and there is still no answer yet, we should also respond immediately.
    // [DNS Discovery Proxy RFC, RFC 8766, Section 5.6]
    // Note that six seconds as stated in RFC8766 is probably too long, currently we're using 800ms.
    if (query->dso == NULL &&
        (question->no_data == true ||
         (question->answers == NULL &&
          srp_utime() - question->start_time > RESPONSE_WINDOW_USECS)))
    {
        INFO("[Q%d][QU%d] no data for question - type %d class %d " PRI_S_SRP,
             SERIAL(query), SERIAL(question), question->type, question->qclass, question->name);
        dns_query_answer_process(0, kDNSServiceErr_NoSuchRecord, question->name,
                                 question->type, question->qclass, 0,
                                 NULL, 0, query, true);
        dp_question_cache_remove_queries(question);
        return;
    }
    // answers are available for the question being asked
    if (question->answers != NULL) {
        INFO("[Q%d][QU%d] reply from cache for question - type %d class %d " PRI_S_SRP,
             SERIAL(query), SERIAL(question), question->type, question->qclass, question->name);
        DNSServiceFlags flags;
        answer_t *answer = question->answers;
        while (answer != NULL) {
            if (remove) {
                flags = 0;
            } else {
                flags = kDNSServiceFlagsAdd;
            }

            answer_t *next = answer->next;
            if (next != NULL) {
                flags |= kDNSServiceFlagsMoreComing;
            }
            if (query->dso == NULL) {
                if (!remove) {
                    dns_query_answer_process(flags, kDNSServiceErr_NoError, answer->fullname,
                                             answer->rrtype, answer->rrclass, answer->rdlen,
                                             answer->rdata, answer->ttl, query, true);
                }
            } else {
                dns_push_query_answer_process(flags, kDNSServiceErr_NoError, answer->fullname,
                                              answer->rrtype, answer->rrclass, answer->rdlen,
                                              answer->rdata, answer->ttl, query, true);
            }
            answer = next;
        }
        dp_question_cache_remove_queries(question);
    }
}

static void
dp_query_context_release(void *context)
{
    dnssd_query_t *query = context;
    RELEASE_HERE(query, dnssd_query);
}

static bool
dp_query_start(dnssd_query_t *query, int *rcode, bool *hardwired, bool dns64)
{
    bool local = false;
    question_t *question = query->question;

    if (question->served_domain != NULL) {
        if (dnssd_hardwired_response(query, dns_question_callback)) {
            *rcode = dns_rcode_noerror; // indicate that we already sent the response
            *hardwired = true;
            INFO("[Q%d] hardwired response", SERIAL(query));
            return true;
        }
        local = true;
    }

    // If we get an SOA query for record that's under a zone cut we're authoritative for, which
    // is the case of query->served_domain->interface != NULL, then answer with a negative response that includes
    // our authority records, rather than waiting for the query to time out.
    if (question->served_domain != NULL && question->served_domain->interface != NULL && !question->name[0]) {
        // If this isn't a DNS Push query, we can signal that there is no data. Otherwise we just never send an answer since
        // we will never have one.
        if (query->dso == NULL) {
            *hardwired = true;
        }
        return true;
    }

    // Check if DNSServiceQueryRecord call needs to be made
    if (question->txn == NULL) {
        int ret = dp_start_question(question, dns64);
        if (ret == kDNSServiceErr_Refused) {
            *rcode = dns_rcode_servfail;
            INFO("question was refused");
        } else if (ret != kDNSServiceErr_NoError) {
            *rcode = dns_rcode_servfail;
            INFO("[Q%d] couldn't start question", SERIAL(query));
            return false;
        }
    } else {
        if (question->answers != NULL || question->no_data) {
            INFO("[Q%d] answering immediately from cache", SERIAL(query));
            *rcode = dns_rcode_noerror;
            return true;
        }
    }

    // If this isn't a DNS Push subscription, we need to respond quickly with as much data as we have.  It
    // turns out that dig gives us a second, but also that responses seem to come back in on the order of a
    // millisecond, so we'll wait 100ms.
    if (query->dso == NULL && local) {
        // [DNS Discovery Proxy RFC, RFC 8766, Section 5.6, Answer Aggregation]

        // RFC8766 asks us to wait six seconds, but this is probably too long. Most likely we will have all
        // our answers much sooner than that, and waiting this long means that we have to keep state for
        // this long; when there are a lot of queries coming in, that can amount to too much state, causing
        // us to drop requests we could easily have answered.
        if (query->wakeup == NULL) {
            query->wakeup = ioloop_wakeup_create();
            if (query->wakeup == NULL) {
                *rcode = dns_rcode_servfail;
                return false;
            }
        }
        ioloop_add_wake_event(query->wakeup, query, dp_query_wakeup, dp_query_context_release, RESPONSE_WINDOW_MSECS /* ms */);
        RETAIN_HERE(query, dnssd_query);
    }

    INFO("[Q%d] waiting for wakeup or response", SERIAL(query));
    return true;
}

static dnssd_query_t *
dp_query_create(dp_tracker_t *tracker, dns_rr_t *question, message_t *message, dso_state_t *dso, int *rcode)
{
    char name[DNS_MAX_NAME_SIZE + 1];
    served_domain_t *sdt = dp_served(question->name, name, sizeof name);
    int xid = message == NULL ? 0 : ntohs(message->wire.id);

    dnssd_query_t *query = calloc(1,sizeof *query);
    require_action_quiet(query != NULL, exit, *rcode = dns_rcode_servfail;
                         ERROR("Unable to allocate memory for query on " PRI_S_SRP, name));
    RETAIN_HERE(query, dnssd_query); // for the caller
    query->serial = ++cur_query_serial;

    // If it's a query for a name served by the local discovery proxy, do an mDNS lookup.
    if (sdt != NULL) {
        INFO("[Q%d][QID%x] msg %p " PUB_S_SRP " question: type %d class %d " PRI_S_SRP "." PRI_S_SRP " -> "
             PRI_S_SRP DOT_LOCAL, SERIAL(query), xid, message, dso != NULL ? "push" : " dns",
             question->type, question->qclass, name, sdt->domain, name);
    } else {
        dns_name_print(question->name, name, sizeof name);
        INFO("[Q%d][QID%x] msg %p " PUB_S_SRP " question: type %d class %d " PRI_S_SRP, SERIAL(query), xid,
             message, dso != NULL ? "push" : " dns", question->type, question->qclass, name);
    }

    query->response = malloc(sizeof *query->response);
    require_action_quiet(query->response != NULL, exit, *rcode = dns_rcode_servfail;
                         ERROR("[Q%d] Unable to allocate memory for query response on " PRI_S_SRP,
                               SERIAL(query), name));

    query->data_size = DNS_DATA_SIZE;

    // Zero out the DNS header, but not the data.
    memset(query->response, 0, DNS_HEADER_SIZE);

    // Name now contains the name we want mDNSResponder to look up.

    // The only thing holding a reference to query is its tracker.
    query->tracker = tracker;
    RETAIN_HERE(query->tracker, dp_tracker);

    // Remember whether this is a long-lived query.
    query->dso = dso;

    // Retain the question, as we will need it to send a response.
    if (message != NULL) {
        query->message = message;
        ioloop_message_retain(query->message);
    }

    // Start writing the response
    dp_query_towire_reset(query);

    bool new_entry;
    query->question = dp_query_question_cache_copy(question, &new_entry);
    require_action_quiet(query->question != NULL, exit, *rcode = dns_rcode_servfail);

    // add the query to the list of queries that are asking this question.
    dnssd_query_t **qr = &(query->question->queries);
    while (*qr != NULL) {
        qr = &(*qr)->question_next;
    }
    *qr = query;
    // Question query list holds a reference to the query.
    RETAIN_HERE(*qr, dnssd_query);
    INFO("[Q%d][QID%x] msg %p " PUB_S_SRP " cache entry for question: type %d class %d " PRI_S_SRP, SERIAL(query),
         xid, query->message, new_entry ? "new" : " existing", question->type, question->qclass, name);
    *rcode = dns_rcode_noerror;
    dp_num_outstanding_queries++;

exit:
    if (*rcode != dns_rcode_noerror && query != NULL) {
        RELEASE_HERE(query, dnssd_query);
        query = NULL;
    }
    return query;
}


static void
dns_push_query_answer_process(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                              const char *fullname, uint16_t rrtype, uint16_t rrclass,
                              uint16_t rdlen, const void *rdata, uint32_t ttl, dnssd_query_t *query, bool send)
{
    uint8_t *revert = query->towire.p;

    VALIDATE_TRACKER_CONNECTION_NON_NULL();

    // From DNSSD-Hybrid, for mDNS queries:
    // If we have cached answers, respond immediately, because we probably have all the answers.
    // If we don't have cached answers, respond as soon as we get an answer (presumably more-coming will be false).

    // The spec says to not query if we have cached answers.   We trust the DNSServiceQueryRecord call to handle this.

    // If we switch to using a single connection to mDNSResponder, we could have !more-coming trigger a flush of
    // all outstanding queries that aren't waiting on a time trigger.   This is because more-coming isn't
    // query-specific

    INFO("[Q%d] PUSH " PRI_S_SRP " %d %d %x %d %p", SERIAL(query), fullname, rrtype, rrclass, rdlen, errorCode, query);

    // query_state_waiting means that we're answering a regular DNS question
    if (errorCode == kDNSServiceErr_NoError) {
        if (send) {
            const void *rdata_to_send;
            uint32_t ttl_to_send;
            // If kDNSServiceFlagsAdd is set, it's an add, otherwise a delete.
        re_add:
            if (flags & kDNSServiceFlagsAdd) {
                rdata_to_send = rdata;
                ttl_to_send = ttl;
                INFO("[Q%d] DNS Push adding record - "
                     "name: " PRI_S_SRP ", rrtype: " PUB_S_SRP ", rrclass: " PUB_S_SRP ", rdlen: %u, ttl: %u.",
                     SERIAL(query), fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen, ttl_to_send);
            } else {
                // See <https://tools.ietf.org/html/rfc8765#section-6.3.1>.
#define TTL_TO_REMOVE_INDIVIDUAL_RECORDS    0xFFFFFFFF
#define TTL_TO_REMOVE_MULTIPLE_RECORDS      0xFFFFFFFE
                if (rdlen == 0) {
                    // Remove specified RRset from a name in given class:
                    // TTL = 0xFFFFFFFE, RDLEN = 0,
                    // CLASS and TYPE specify the RRset being removed.
                    rdata_to_send = NULL;
                    ttl_to_send = TTL_TO_REMOVE_MULTIPLE_RECORDS;
                } else {
                    // Remove an individual RR from a name:
                    // TTL = 0xFFFFFFFF,
                    // CLASS, TYPE, RDLEN, and RDATA specify the RR being removed.
                    rdata_to_send = rdata;
                    ttl_to_send = TTL_TO_REMOVE_INDIVIDUAL_RECORDS;
                }
                INFO("[Q%d] DNS Push removing record - "
                     "name: " PRI_S_SRP ", rrtype: " PUB_S_SRP ", rrclass: " PUB_S_SRP ", rdlen: %u, ttl: 0x%X.",
                     SERIAL(query), fullname, dns_rrtype_to_string(rrtype), dns_qclass_to_string(rrclass), rdlen, ttl_to_send);
            }

            // Do the update.
            dp_query_add_data_to_response(query, fullname, rrtype, rrclass, rdlen, rdata_to_send,
                                          ttl_to_send, false, false, NULL);

            if (query->towire.truncated) {
                query->towire.truncated = false;
                query->towire.p = revert;
                query->towire.error = 0;
                dp_push_response(query, NULL);
                dns_push_start(query);
                goto re_add;
            }
        }
        // If there isn't more coming, send a DNS Push notification now.
        // XXX If enough comes to fill the response, send the message.
        if (!(flags & kDNSServiceFlagsMoreComing)) {
            dp_push_response(query, NULL);
        }
    } else if (errorCode != kDNSServiceErr_NoSuchRecord) { // Do nothing if kDNSServiceErr_NoSuchRecord is received.
        ERROR("[Q%d] unexpected error code %d", SERIAL(query), errorCode);
        dnssd_query_cancel(query);
    }
}

static void
dns_push_subscribe(dp_tracker_t *tracker, const dns_wire_t *header, dso_state_t *dso, dns_rr_t *question,
                   const char *activity_name, const char * UNUSED opcode_name)
{
    int rcode;
    dnssd_query_t *query = dp_query_create(tracker, question, NULL, dso, &rcode);

    if (!query) {
        dso_simple_response(tracker->connection, NULL, header, rcode);
        return;
    }

    dso_activity_t *activity = dso_add_activity(dso, activity_name, push_subscription_activity_type, query,
                                                dns_push_cancel);
    RETAIN_HERE(query, dnssd_query); // The activity holds a reference to the query.
    query->activity = activity;
    bool dns64 = false;
#if SRP_FEATURE_NAT64
    if (srp_servers->srp_nat64_enabled) {
        dns64 = nat64_is_active();
    }
#endif
    bool hardwired = false;
    if (!dp_query_start(query, &rcode, &hardwired, dns64)) {
        dso_simple_response(tracker->connection, NULL, header, rcode);
        dp_question_cache_remove_queries(query->question);
        dnssd_query_cancel(query);
    } else {
        char nbuf[DNS_MAX_NAME_SIZE + 1];
        dns_name_print(question->name, nbuf, sizeof(nbuf));
        // The push subscribe can be considered a success at this point.
        dso_simple_response(tracker->connection, NULL, header, dns_rcode_noerror);
        if (hardwired) {
            INFO("[DSO%d][Q%d] hardwired response for " PRI_S_SRP " %d %d",
                 SERIAL(dso), SERIAL(query), nbuf, question->type, question->qclass);
            dp_push_response(query, question);
        } else if (query->question != NULL) {
            INFO("[DSO%d][Q%d][QU%d] replying from cache for " PRI_S_SRP " %d %d",
                 SERIAL(dso), SERIAL(query), SERIAL(query->question), nbuf, question->type, question->qclass);
            dp_query_reply_from_cache(query->question, query, false);
        }
    }

    // dp_query_create() returned the query retained; when we added the query to the activity, we retained it again;
    // if something went wrong, the second retain was released, but whether or not something went wrong, we can now
    // safely release the initial retain.
    RELEASE_HERE(query, dnssd_query);
}

static void
dns_push_reconfirm(comm_t *comm, const dns_wire_t *header, dso_state_t *dso)
{
    dns_rr_t question;
    char name[DNS_MAX_NAME_SIZE + 1];
    uint16_t rdlen;
    memset(&question, 0, sizeof(question));

    // The TLV offset should always be pointing into the message.
    unsigned offp = (unsigned)(dso->primary.payload - &header->data[0]);
    unsigned len = offp + dso->primary.length;
    dp_tracker_t *tracker = comm->context;

    // Parse the name, rrtype and class.   We say there's no rdata even though there is
    // because there's no ttl and also we want the raw rdata, not parsed rdata.
    if (!dns_rr_parse(&question, header->data, len, &offp, false, false) ||
        !dns_u16_parse(header->data, len, &offp, &rdlen))
    {
        dso_simple_response(comm, NULL, header, dns_rcode_formerr);
        ERROR("[DSO%d][C%d][TRK%d] RR parse from %s failed",
              SERIAL(dso), SERIAL(comm), SERIAL(tracker), dso->remote_name);
        goto out;
    }
    if (rdlen + offp != len) {
        dso_simple_response(comm, NULL, header, dns_rcode_formerr);
        ERROR("[DSO%d][C%d][TRK%d] RRdata parse from %s failed: length mismatch (%d != %d)",
              SERIAL(dso), SERIAL(comm), SERIAL(tracker), dso->remote_name, rdlen + offp, len);
        goto out;
    }

    if ((dp_served(question.name, name, sizeof name))) {
        size_t name_len = strlen(name);
        if (name_len + sizeof local_suffix > sizeof name) {
            dso_simple_response(comm, NULL, header, dns_rcode_formerr);
            ERROR("[DSO%d][C%d][TRK%d] name is too long for .local suffix: %s", SERIAL(dso), SERIAL(comm), SERIAL(tracker), name);
            goto out;
        }
        memcpy(&name[name_len], local_suffix, sizeof local_suffix);
    } else {
        dns_name_print(question.name, &name[8], sizeof name - 8);
    }
    // transmogrify name.
    DNSServiceReconfirmRecord(0, kDNSServiceInterfaceIndexAny, name,
                              question.type, question.qclass, rdlen, &header->data[offp]);
    dso_simple_response(comm, NULL, header, dns_rcode_noerror);
out:
    dns_rrdata_free(&question);
    dns_name_free(question.name);
}

static void
dns_push_unsubscribe(dso_activity_t *activity)
{
    dnssd_query_t *query = activity->context;
    dnssd_query_cancel(query);
    // No response, unsubscribe is unidirectional.
}

static void
dns_push_subscription_change(const char *opcode_name, dp_tracker_t *tracker, const dns_wire_t *header, dso_state_t *dso)
{
    // type-in-hex/class-in-hex/name-to-subscribe
    char activity_name[5];
    dso_activity_t *activity;

    // The TLV offset should always be pointing into the message.
    unsigned offp = (unsigned)(dso->primary.payload - &header->data[0]);
    unsigned len = offp + dso->primary.length;
    // Get the question
    dns_rr_t question;
    uint16_t subscribe_xid = ntohs(header->id);
    char nbuf[DNS_MAX_NAME_SIZE + 1];

    memset(&question, 0, sizeof(question));
    if (dso->primary.opcode == kDSOType_DNSPushSubscribe) {
        if (!dns_rr_parse(&question, header->data, offp + dso->primary.length, &offp, false, false)) {
            dso_simple_response(tracker->connection, NULL, header, dns_rcode_formerr);
            ERROR("[DSO%d][TRK%d] RR parse for %s from %s failed", SERIAL(dso), SERIAL(tracker), dso->remote_name, opcode_name);
            goto out;
        }
        dns_name_print(question.name, nbuf, sizeof(nbuf));
    } else {
        // Unsubscribes are unidirectional, so no response can be sent
        if (!dns_u16_parse(header->data, offp + dso->primary.length, &offp, &subscribe_xid)) {
            ERROR("unable to get subscribe xid from primary");
            goto out;
        }
        const char none[] = "none";
        memcpy(nbuf, none, sizeof(none));
    }
    if (offp != len) {
        if (dso->primary.opcode == kDSOType_DNSPushSubscribe) {
            dso_simple_response(tracker->connection, NULL, header, dns_rcode_formerr);
        }
        ERROR("DNS push " PUB_S_SRP " parse from %s failed: length mismatch (%d != %d)",
              dso->primary.opcode == kDSOType_DNSPushSubscribe ? "subscribe" : "unsubscribe",
              dso->remote_name, offp, len);
        goto out;
    }

    // Concoct an activity name. The subscribe transaction ID is required to be unique and is used by the
    // protocol to identify the subscription, so we can just use that.
    snprintf(activity_name, sizeof(activity_name), "%04x", subscribe_xid);

    activity = dso_find_activity(dso, activity_name, push_subscription_activity_type, NULL);
    if (activity == NULL) {
        // Unsubscribe with no activity means no work to do; just return noerror.
        if (dso->primary.opcode != kDSOType_DNSPushSubscribe) {
            ERROR("[DSO%d][TRK%d] " PUB_S_SRP " for " PRI_S_SRP " (" PUB_S_SRP ") when no subscription exists.",
                  SERIAL(dso), SERIAL(tracker), opcode_name, nbuf, activity_name);
        } else {
            INFO("[DSO%d][TRK%d] " PUB_S_SRP " for " PRI_S_SRP " (" PUB_S_SRP ") type %d.",
                 SERIAL(dso), SERIAL(tracker), opcode_name, nbuf, activity_name, question.type);
            // In this case we have a push subscribe for which no subscription exists, which means we can do it.
            dns_push_subscribe(tracker, header, dso, &question, activity_name, opcode_name);
        }
    } else {
        // We should never get two subscribes with the same transaction id.
        if (dso->primary.opcode == kDSOType_DNSPushSubscribe) {
            ERROR("[DSO%d][TRK%d] " PUB_S_SRP " for " PRI_S_SRP " (" PUB_S_SRP ") "
                  "xid %d when subscription already exists.", SERIAL(dso), SERIAL(tracker),
                  opcode_name, nbuf, activity_name, subscribe_xid);
            dso_simple_response(tracker->connection, NULL, header, dns_rcode_refused);
        }
        // Otherwise cancel the subscription.
        else {
            dnssd_query_t *query = activity->context;
            char *question_name = query->question != NULL ? query->question->name : nbuf;
            INFO("[DSO%d][TRK%d] " PUB_S_SRP " for " PRI_S_SRP " (" PUB_S_SRP ") type %d.",
                 SERIAL(dso), SERIAL(tracker), opcode_name, question_name, activity_name, question.type);
            dns_push_unsubscribe(activity);
        }
    }
out:
    dns_rrdata_free(&question);
    dns_name_free(question.name);
}

static bool
dso_limit(dp_tracker_t *tracker, message_t *message, dp_tracker_session_type_t session_type)
{
    if (num_push_sessions == MAX_DSO_CONNECTIONS) {
        // We are too busy. Return a retry-delay response.
        INFO("[TRK%d] no more DNS Push connections allowed--sending retry-delay: %d", SERIAL(tracker), num_push_sessions);
        dso_retry_delay_response(tracker->connection, message, &message->wire, dns_rcode_servfail, BUSY_RETRY_DELAY_MS);

        num_push_sessions_dropped_for_load++;
        // Cancel the connection after five seconds
        dp_tracker_idle_after(tracker, 5, NULL);
        return true;
    }

    // Count this as a DSO connection.
    (num_push_sessions)++;
    INFO("[TRK%d] new DNS Push connection, count is now %d", SERIAL(tracker), num_push_sessions);

    tracker->session_type = session_type;
    return false;
}

static void dso_message(dp_tracker_t *tracker, message_t *message, dso_state_t *dso)
{
    // For the first DSO message we get on a connection, see if we already have too many connections of
    // the same type. We track SRP replication and DNS Push separately, because we don't want a surfeit of
    // DNS Push messages to prevent replication from working. A surfeit of SRP Replication connections is
    // less likely, and less problematic.
    if (tracker->session_type == dp_tracker_session_none) {
        if (dso->primary.opcode != kDSOType_SRPLSession) {
            if (dso_limit(tracker, message, dp_tracker_session_push)) {
                return;
            }
        }
    }

    switch(dso->primary.opcode) {
    case kDSOType_DNSPushSubscribe:
        dns_push_subscription_change("DNS Push Subscribe", tracker, &message->wire, dso);
        break;
    case kDSOType_DNSPushUnsubscribe:
        dns_push_subscription_change("DNS Push Unsubscribe", tracker, &message->wire, dso);
        break;

    case kDSOType_DNSPushReconfirm:
        dns_push_reconfirm(tracker->connection, &message->wire, dso);
        break;

    case kDSOType_DNSPushUpdate:
        INFO("[DSO%d][TRK%d] bogus push update message %d", SERIAL(dso), SERIAL(tracker), dso->primary.opcode);
        dso_state_cancel(dso);
        break;

#if SRP_FEATURE_REPLICATION
    case kDSOType_SRPLSession:
        if (dso->activities != NULL) {
            dso_state_cancel(dso);
            ERROR("[DSO%d][TRK%d][C%d] " PRI_S_SRP ": SRP Replication session start received on a connection that is already doing DNS Push.",
                  SERIAL(dso), SERIAL(tracker), SERIAL(tracker->connection), tracker->connection->name);
            return;
        }
#ifdef SRP_TEST_SERVER
        srpl_dso_server_message(tracker->connection, message, dso, (srp_server_t*)tracker->connection->srp_server);
#else
        srpl_dso_server_message(tracker->connection, message, dso, srp_servers);
#endif
        break;
#endif

    default:
        INFO("[DSO%d][TRK%d] unexpected primary TLV %d", SERIAL(dso), SERIAL(tracker), dso->primary.opcode);
        dso_simple_response(tracker->connection, NULL, &message->wire, dns_rcode_dsotypeni);
        break;
    }
    // XXX free the message if we didn't consume it.
#ifdef SRP_TEST_SERVER
    if (srp_test_dso_message_finished != NULL) {
        srp_test_dso_message_finished(srp_test_tls_listener_context, message, dso);
    }
#endif
}

static void
dp_keepalive_response_send(dso_keepalive_context_t *keepalive_event, dso_state_t *dso)
{
    uint8_t dsobuf[SRPL_KEEPALIVE_MESSAGE_LENGTH];
    dns_towire_state_t towire;
    struct iovec iov;
    uint16_t *p_dso_length;
    dso_message_t state;

    if (dso->transport == NULL) {
        ERROR("dso state " PRI_S_SRP " has no transport", dso->remote_name);
        return;
    }

    memset(&towire, 0, sizeof(towire));
    towire.p = &dsobuf[DNS_HEADER_SIZE];
    towire.lim = towire.p + (sizeof(dsobuf) - DNS_HEADER_SIZE);
    towire.message = (dns_wire_t *)dsobuf;
    towire.p_rdlength = NULL;
    towire.p_opt = NULL;
    p_dso_length = NULL;

    dso_make_message(&state, dsobuf, sizeof(dsobuf), dso, false /* unidirectional */, true /* response */,
                     keepalive_event->xid, dns_rcode_noerror, dso->transport);
    dns_u16_to_wire(&towire, kDSOType_Keepalive);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, keepalive_event->inactivity_timeout); // Idle timeout (we are never idle)
    dns_u32_to_wire(&towire, keepalive_event->keepalive_interval); // Keepalive timeout
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(dso->transport, NULL, &iov, 1)) {
        INFO("send failed");
        return;
    }

    INFO("sent %zd byte response Keepalive, xid %02x%02x (was %04x), to " PRI_S_SRP,
         iov.iov_len, dsobuf[0], dsobuf[1], keepalive_event->xid, dso->transport->name);
}

static void
dns_push_callback(void *context, void *event_context, dso_state_t *dso, dso_event_type_t eventType)
{
    dso_keepalive_context_t *keepalive_context;
    message_t *message;
    switch(eventType)
    {
    case kDSOEventType_DNSMessage:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("[DSO%d] DNS Message (opcode=%d) received from " PRI_S_SRP, SERIAL(dso), dns_opcode_get(&message->wire),
             dso->remote_name);
        break;
    case kDSOEventType_DNSResponse:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("[DSO%d] DNS Response (opcode=%d) received from " PRI_S_SRP, SERIAL(dso), dns_opcode_get(&message->wire),
             dso->remote_name);
        break;
    case kDSOEventType_DSOMessage:
        INFO("[DSO%d] DSO Message (Primary TLV=%d) received from " PRI_S_SRP,
             SERIAL(dso), dso->primary.opcode, dso->remote_name);
        message = event_context;
        dso_message((dp_tracker_t *)context, message, dso);
        break;
    case kDSOEventType_DSOResponse:
        INFO("[DSO%d] DSO Response (Primary TLV=%d) received from " PRI_S_SRP,
             SERIAL(dso), dso->primary.opcode, dso->remote_name);
        break;

    case kDSOEventType_Finalize:
        INFO("[DSO%d] Finalize", SERIAL(dso));
        break;

    case kDSOEventType_Connected:
        INFO("[DSO%d] Connected to " PRI_S_SRP, SERIAL(dso), dso->remote_name);
        break;

    case kDSOEventType_ConnectFailed:
        INFO("[DSO%d] Connection to " PRI_S_SRP " failed", SERIAL(dso), dso->remote_name);
        break;

    case kDSOEventType_Disconnected:
        INFO("[DSO%d] Connection to " PRI_S_SRP " disconnected", SERIAL(dso), dso->remote_name);
        break;
    case kDSOEventType_ShouldReconnect:
        INFO("[DSO%d] Connection to " PRI_S_SRP " should reconnect (not for a server)", SERIAL(dso), dso->remote_name);
        break;
    case kDSOEventType_Inactive:
        INFO("[DSO%d] Inactivity timer went off, closing connection.", SERIAL(dso));
        break;
    case kDSOEventType_Keepalive:
        INFO("[DSO%d] should send a keepalive now.", SERIAL(dso));
        break;
    case kDSOEventType_KeepaliveRcvd:
        keepalive_context = event_context;
        keepalive_context->send_response = false;
        INFO("[DSO%d] " PRI_S_SRP ": keepalive received, xid %04x.", SERIAL(dso), dso->transport->name, keepalive_context->xid);

        // If we are the server, we have to send a response to the keepalive.
        if (dso->is_server) {
            dp_keepalive_response_send(keepalive_context, dso);
        }
        break;
    case kDSOEventType_RetryDelay:
        INFO("[DSO%d] keepalive received.", SERIAL(dso));
        break;
    }
}

static bool
dp_dns_query(dp_tracker_t *tracker, message_t *message, dns_rr_t *question, int num_questions)
{
    int rcode;

    // Limit outstanding queries if we don't have shared connection support
    if (dp_num_outstanding_queries >= 256) {
        num_queries_dropped_for_load++;
        dso_simple_response(tracker->connection, message, &message->wire, dns_rcode_servfail);
        ERROR("[TRK%d][QID %x] dropping query because there are too many", SERIAL(tracker), ntohs(message->wire.id));
        return false;
    }


    dnssd_query_t *query = dp_query_create(tracker, question, message, NULL, &rcode);
    const char *failnote = NULL;
    if (!query) {
        ERROR("[TRK%d][QID %x] query create failed", SERIAL(tracker), ntohs(message->wire.id));
        dso_simple_response(tracker->connection, message, &message->wire, rcode);
        return false;
    }
    query->num_questions = num_questions;

    dns_rcode_set(query->response, dns_rcode_noerror);

    // For DNS queries, we need to return the question.
    query->response->qdcount = htons(1);
    if (query->question->served_domain != NULL) {
        TOWIRE_CHECK("name", &query->towire, dns_name_to_wire(NULL, &query->towire, query->question->name));
        TOWIRE_CHECK("enclosing_domain", &query->towire,
                     dns_full_name_to_wire(&query->enclosing_domain_pointer,
                                           &query->towire, query->question->served_domain->domain));
    } else {
        TOWIRE_CHECK("full name", &query->towire, dns_full_name_to_wire(NULL, &query->towire, query->question->name));
    }
    TOWIRE_CHECK("TYPE", &query->towire, dns_u16_to_wire(&query->towire, question->type));    // TYPE
    TOWIRE_CHECK("CLASS", &query->towire, dns_u16_to_wire(&query->towire, question->qclass));  // CLASS
    if (failnote != NULL) {
        ERROR("[TRK%d][QID %x] failure encoding question: " PUB_S_SRP, SERIAL(tracker), ntohs(message->wire.id), failnote);
        goto fail;
    }

    // Set message ID.
    query->towire.message->id = message->wire.id;

    // We should check for OPT RR, but for now assume it's there.
    query->is_edns0 = true;

    bool dns64 = false;
#if SRP_FEATURE_NAT64
    if (srp_servers->srp_nat64_enabled) {
        dns64 = nat64_is_active();
    }
#endif
    dp_query_track(tracker, query);
    bool hardwired = false;
    if (dp_query_start(query, &rcode, &hardwired, dns64)) {
        // If query->question isn't NULL, we need to reply from cache
        if (hardwired) {
            INFO("[Q%d][TRK%d] hardwired reply", SERIAL(query), SERIAL(tracker));
            dp_query_send_dns_response(query, "hardwired");
            dp_question_cache_remove_queries(query->question);
            RELEASE_HERE(query->question, question);
            query->question = NULL;
        } else if (query->question != NULL) {
            INFO("[Q%d][TRK%d] replying from cache", SERIAL(query), SERIAL(tracker));
            dp_query_reply_from_cache(query->question, query, false);
            dp_question_cache_remove_queries(query->question);
        } else {
            INFO("[Q%d][TRK%d] not replying from cache", SERIAL(query), SERIAL(tracker));
        }
    } else {
        INFO("[Q%d][TRK%d][QID %x] query start failed", SERIAL(query), SERIAL(tracker), ntohs(message->wire.id));
    fail:
        dso_simple_response(tracker->connection, message, &message->wire, rcode);
        query->satisfied = true;
        dp_question_cache_remove_queries(query->question);
        dnssd_query_cancel(query);
        RELEASE_HERE(query, dnssd_query);
        return false;
    }
    // Query is returned retained, and dp_query_track retains it, so we always need to release the reference here.
    RELEASE_HERE(query, dnssd_query);
    return true;
}

static void
dp_tracker_dso_cleanup(void *UNUSED context)
{
    dso_cleanup(false);
}

static bool
dp_tracker_dso_state_change(const dso_life_cycle_t cycle, void *const context, dso_state_t *const dso)
{
    if (cycle == dso_life_cycle_cancel) {
        dp_tracker_t *tracker = context;
        if (tracker->dso != NULL) {
            tracker->dso = NULL;
            if (tracker->connection != NULL) {
                tracker->connection->dso = NULL;
                ioloop_comm_cancel(tracker->connection);
            }
            for (dnssd_query_t *query = tracker->dns_queries; query != NULL; query = query->next) {
                if (query->dso == dso) {
                    query->dso = NULL;
                }
                if (query->activity != NULL) {
                    query->activity = NULL;
                    // Release the activity's reference to the query.
                    RELEASE_HERE(query, dnssd_query);
                }
            }
        }
        ioloop_run_async(dp_tracker_dso_cleanup, NULL);
        return true;
    }
    return false;
}

static void
dnssd_proxy_dns_evaluate(comm_t *comm, message_t *message, dp_tracker_t *tracker)
{
    dns_rr_t question;
    unsigned offset = 0;

    if (tracker == NULL) {
        tracker = calloc(1, sizeof(*tracker));
        if (tracker == NULL) {
            ERROR("[C%d] " PRI_S_SRP ": no memory for a connection tracker object!", SERIAL(comm), comm->name);
            goto fail;
        }
        tracker->connection = comm;
        tracker->serial = ++cur_tracker_serial;
        ioloop_comm_retain(tracker->connection);
        if (comm->tcp_stream) {
            ioloop_comm_context_set(comm, tracker, dp_tracker_context_release);
            RETAIN_HERE(tracker, dp_tracker); // connection has a reference.
        }
        if (!comm->is_listener) {
            ioloop_comm_disconnect_callback_set(comm, dp_tracker_disconnected);
        }
    }
    RETAIN_HERE(tracker, dp_tracker); // For the function.

    // Drop incoming responses--we're a server, so we only accept queries.
    if (dns_qr_get(&message->wire) == dns_qr_response) {
        INFO("[TRK%d][C%d] " PRI_S_SRP ": dropping unexpected response", SERIAL(tracker), SERIAL(comm), comm->name);
        goto fail;
    }

    // If this is a DSO message, see if we have a session yet.
    switch(dns_opcode_get(&message->wire)) {
    case dns_opcode_dso:
        if (!comm->tcp_stream) {
            ERROR("[TRK%d][C%d] " PRI_S_SRP ": DSO message received on non-tcp socket.", SERIAL(tracker), SERIAL(comm), comm->name);
            dso_simple_response(comm, message, &message->wire, dns_rcode_notimp);
            goto fail;
        }

        if (!tracker->dso) {
            tracker->dso = dso_state_create(true, 2, comm->name, dns_push_callback, tracker,
                                            dp_tracker_dso_state_change, comm);
            if (!tracker->dso) {
                ERROR("[TRK%d][C%d] " PRI_S_SRP ": Unable to create a dso context.", SERIAL(tracker), SERIAL(comm), comm->name);
                dso_simple_response(comm, message, &message->wire, dns_rcode_servfail);
                goto fail;
            }
            comm->dso = tracker->dso;
        }
        dp_tracker_not_idle(tracker);
        dso_message_received(comm->dso, (uint8_t *)&message->wire, message->length, message);
        break;

    case dns_opcode_query: {
        int num_questions = ntohs(message->wire.qdcount);

        // Some Matter accessories will send queries with more than one question, and if we don't answer these
        // queries, automations fail. So even though this is a bit weird, we need to answer the queries.
        for (int i = 0; i < num_questions; i++) {
            memset(&question, 0, sizeof(question));
            if (!dns_rr_parse(&question, message->wire.data, message->length - DNS_HEADER_SIZE, &offset, false, false)) {
                ERROR("[TRK%d][C%d] " PRI_S_SRP ": rr parse failed.", SERIAL(tracker), SERIAL(comm), comm->name);
                dso_simple_response(comm, message, &message->wire, dns_rcode_formerr);
                goto fail;
            }
            bool success = dp_dns_query(tracker, message, &question, num_questions);
            dns_rrdata_free(&question);
            dns_name_free(question.name);
            if (!success) {
                dnssd_query_t *next = NULL, *match = NULL;
                for (dnssd_query_t *query = tracker->dns_queries; query != NULL; query = next) {
                    next = query->next;
                    if (dp_same_message(query->message, message)) {
                        query->satisfied = true;
                        dp_question_cache_remove_queries(query->question);
                        if (match == NULL) {
                            match = query;
                            RETAIN_HERE(match, dnssd_query);
                        }
                    }
                }
                if (match != NULL) {
                    dnssd_query_cancel(match);
                    RELEASE_HERE(match, dnssd_query);
                }
                goto out;
            }
        }
        dp_tracker_not_idle(tracker);
        break;
    }
        // No support for other opcodes yet.
    default:
        dso_simple_response(comm, message, &message->wire, dns_rcode_notimp);
        break;
    }
    goto out;
fail:
    // For connected connections, if we exit unexpectedly, we need to cancel the connection.
    if (comm->tcp_stream) {
        ioloop_comm_cancel(tracker->connection);
    }
out:
    if (tracker != NULL) {
        RELEASE_HERE(tracker, dp_tracker); // For the function.
    }
}

void
dns_proxy_input_for_server(comm_t *comm, srp_server_t *server_state, message_t *message, void *context)
{
    char buf[INET6_ADDRSTRLEN];
    const char *remote_name = buf;
    if (comm->tcp_stream) {
        remote_name = comm->name;
    } else {
        IOLOOP_NTOP(&message->src, buf);
    }
    dp_tracker_t *tracker = comm->context;
    INFO("[C%d][TRK%d][QID %x] Received a new DNS message - src: " PRI_S_SRP ", message length: %u bytes.",
         SERIAL(comm), SERIAL(tracker), ntohs(message->wire.id), remote_name, message->length);


    dnssd_proxy_dns_evaluate(comm, message, context);
}

static void
dns_proxy_input(comm_t *comm, message_t *message, void *context)
{
    dns_proxy_input_for_server(comm, srp_servers, message, context);
}

// usage is only called when we are building standalone dnssd-proxy, not the combined one.
#if (!SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
static int
usage(const char *progname)
{
    ERROR("usage: %s", progname);
    ERROR("ex: dnssd-proxy");
    return 1;
}
#endif // #if (!SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)

// Called whenever we get a connection.
static void UNUSED
connected(comm_t *comm)
{
    INFO("[C%d] connection from " PRI_S_SRP, SERIAL(comm), comm->name);
    return;
}

static served_domain_t *NULLABLE
new_served_domain(dp_interface_t *const NULLABLE interface, const char *const NONNULL domain)
{
    served_domain_t *sdt = calloc(1, sizeof *sdt);
    if (sdt == NULL) {
        ERROR("Unable to allocate served domain %s", domain);
        return NULL;
    }
    size_t domain_len = strlen(domain);
    sdt->domain_ld = malloc(domain_len + 2);
    if (sdt->domain_ld == NULL) {
        ERROR("Unable to allocate served domain name %s", domain);
        free(sdt);
        return NULL;
    }
    sdt->domain_ld[0] = '.';
    sdt->domain = sdt->domain_ld + 1;
    memcpy(sdt->domain, domain, domain_len + 1);
    sdt->domain_name = dns_pres_name_parse(sdt->domain);
    sdt->interface = interface;
    if (sdt->domain_name == NULL) {
        if (interface != NULL) {
            ERROR("invalid domain name for interface %s: %s", interface->name, sdt->domain);
        } else {
            ERROR("invalid domain name: %s", sdt->domain);
        }
        free(sdt);
        return NULL;
    }
    sdt->next = served_domains;
    served_domains = sdt;

    INFO("new served domain created - domain name: " PRI_S_SRP, sdt->domain);
    return sdt;
}

#if STUB_ROUTER
static served_domain_t *NULLABLE
find_served_domain(const char *const NONNULL domain)
{
    served_domain_t *current;
    for (current = served_domains; current != NULL; current = current->next) {
        if (strcasecmp(domain, current->domain) == 0) {
            break;
        }
    }

    return current;
}
#endif

// served domain can only go away when combined with srp-mdns-proxy and interface going up and down.
#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static void
served_domain_free(served_domain_t *const served_domain)
{
    INFO("served domain removed - domain name: " PRI_S_SRP, served_domain->domain);

   // free struct interface *NULLABLE interface
    if (served_domain->interface != NULL) {
        interface_addr_t *current = served_domain->interface->addresses;
        interface_addr_t *next;
        for (;current != NULL; current = next) {
            next = current->next;
            free(current);
        }
        if (served_domain->interface->name != NULL) {
            free(served_domain->interface->name);
        }
        free(served_domain->interface);
    }

    // free hardwired_t *NULLABLE hardwired_responses
    if (served_domain->hardwired_responses != NULL) {
        hardwired_t *current = served_domain->hardwired_responses;
        hardwired_t *next;
        for (; current != NULL; current = next) {
            next = current->next;
            free(current);
        }
    }

    // free dns_name_t *NONNULL domain_name;
    if (served_domain->domain_name != NULL) {
        dns_name_free(served_domain->domain_name);
    }

    // free char *NONNULL domain_ld;
    free(served_domain->domain_ld);

#ifdef SRP_TEST_SERVER
    last_freed_domain = served_domain;
#endif

    // free served_domain_t *
    free(served_domain);
}

static void
delete_served_domain(served_domain_t *const served_domain)
{
    if (served_domain->questions == NULL) {
        served_domain_free(served_domain);
    }
}

#if STUB_ROUTER
served_domain_t *
delete_served_domain_by_interface_name(const char *const NONNULL interface_name)
{
    served_domain_t *current;
    served_domain_t *prev = NULL;
    for(current= served_domains; current != NULL; prev = current, current = current->next) {
        if (current->interface == NULL) {
            continue;
        }
        if (strcmp(interface_name, current->interface->name) != 0) {
            continue;
        }

        INFO("served domain deleted with interface - "
            "domain: " PRI_S_SRP ", interface name: " PUB_S_SRP, current->domain, interface_name);

        // Since we are removing the entire served domain and the interface, the addresses that are associated with
        // this interface will also be removed. Therefore, any hardwired response that contains these addresses should
        // also be removed.
        for (interface_addr_t *address = current->interface->addresses; address != NULL; address = address->next) {
            dnssd_hardwired_process_addr_change(&address->addr, &address->mask, false);
        }

        if (prev == NULL) {
            served_domains = current->next;
        } else {
            prev->next = current->next;
        }

        delete_served_domain(current);
        break;
    }

    return current;
}
#endif // STUB_ROUTER
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

// Dynamic interface detection...
// This is called whenever a new interface address is encountered.

void
dnssd_proxy_ifaddr_callback(srp_server_t *UNUSED server_state, void *UNUSED context, const char *name,
                            const addr_t *address, const addr_t *mask, uint32_t UNUSED flags,
                            enum interface_address_change event_type)
{
#if SRP_FEATURE_DYNAMIC_CONFIGURATION
    bool is_new_interface = true;
#endif
    bool succeeded;
    const char *const action = (event_type == interface_address_added ? "Adding" : "Removing");

    if (event_type == interface_address_unchanged) {
        goto exit;
    }

    int interface_index = if_nametoindex(name);
    if (address->sa.sa_family == AF_INET) {
        IPv4_ADDR_GEN_SRP((const uint8_t *)&address->sin.sin_addr, addr_buf);
        IPv4_ADDR_GEN_SRP((const uint8_t *)&mask->sin.sin_addr, mask_buf);
        INFO("Interface " PUB_S_SRP " address " PRI_IPv4_ADDR_SRP " mask " PRI_IPv4_ADDR_SRP " index %d " PUB_S_SRP,
             name, IPv4_ADDR_PARAM_SRP((const uint8_t *)&address->sin.sin_addr, addr_buf),
             IPv4_ADDR_PARAM_SRP((const uint8_t *)&mask->sin.sin_addr, mask_buf), interface_index,
             event_type == interface_address_added ? "added" : "removed");
    } else if (address->sa.sa_family == AF_INET6) {
        IPv6_ADDR_GEN_SRP((const uint8_t *)&address->sin6.sin6_addr, addr_buf);
        IPv6_ADDR_GEN_SRP((const uint8_t *)&mask->sin6.sin6_addr, mask_buf);
        INFO("Interface " PUB_S_SRP " address " PRI_IPv6_ADDR_SRP " mask " PRI_IPv6_ADDR_SRP " index %d " PUB_S_SRP,
             name, IPv6_ADDR_PARAM_SRP((const uint8_t *)&address->sin6.sin6_addr, addr_buf),
             IPv6_ADDR_PARAM_SRP((const uint8_t *)&mask->sin6.sin6_addr, mask_buf), interface_index,
             event_type == interface_address_added ? "added" : "removed");
    } else {
        INFO("Interface " PUB_S_SRP " address type %d index %d " PUB_S_SRP, name, address->sa.sa_family, interface_index,
             event_type == interface_address_added ? "added" : "removed");
        INFO("ignoring non IP address");
        goto exit;
    }

#if THREAD_BORDER_ROUTER && SRP_FEATURE_COMBINED_DNSSD_PROXY
    // Ignore Thread interface
    bool is_valid_address = thread_interface_name == NULL || strcmp(thread_interface_name, name) != 0;
    if (!is_valid_address) {
        INFO("skipping thread interface address");
        goto exit;
    }
#endif

    // Add/remove the address from the corresponding served domain.
    served_domain_t **sp = &served_domains;
    while (*sp != NULL) {
        served_domain_t *current = *sp;
        // Only change the served domain that owns the current interface and address.
        if (current->interface == NULL || current->interface->ifindex == 0 ||
            strcmp(current->interface->name, name) != 0) {
            goto again;
        }

        INFO(PUB_S_SRP " address from the served domain - domain: " PRI_S_SRP, action, current->domain);
        succeeded = interface_process_addr_change(current->interface, address, mask, event_type);
        require_action_quiet(succeeded, exit, ERROR("failed to " PUB_S_SRP " new interface address", action));

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
        is_new_interface = false;

        // if interface loses all usable IP addresses, the interface has gone, remove this interface and the
        // corresponding served domain.
        if (event_type == interface_address_deleted) {
            if (current->interface->addresses == NULL) {
                INFO("Removing served domain with 0 address - domain: " PRI_S_SRP ", interface name: " PUB_S_SRP,
                    current->domain, current->interface->name);
                *sp = current->next;
                delete_served_domain(current);
                continue;
            }
        }
#else // SRP_FEATURE_DYNAMIC_CONFIGURATION
        if (current->interface->addresses == NULL) {
            current->interface->ifindex = 0;
        }
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION
    again:
        sp = &(*sp)->next;
    }

    // We will only create new served domain from dnssd_proxy_ifaddr_callback if the callback gets called from
    // srp-mdns-proxy.
#if SRP_FEATURE_DYNAMIC_CONFIGURATION
    if (event_type == interface_address_added && is_new_interface) {
        served_domain_t *const new_served_domain = add_new_served_domain_with_interface(name, address, mask);
        require_action_quiet(new_served_domain != NULL, exit,
            ERROR("failed to add new served domain ""- interface name: " PUB_S_SRP, name));

        bool hardwired_set = dnssd_hardwired_setup_for_served_domain(new_served_domain);
        if (!hardwired_set) {
            ERROR("failed to setup hardwired response for newly created served domain - domain: " PRI_S_SRP, name);
            delete_served_domain(new_served_domain);
        }
        INFO("New served domain created and hardwired response created - domain: " PRI_S_SRP,
            new_served_domain->domain);
    }
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

#if STUB_ROUTER
    // Added or removed address will possibly need hardwired response to be updated.
    dnssd_hardwired_process_addr_change(address, mask, event_type == interface_address_added);
#endif

exit:
    return;
}

#if !SRP_FEATURE_DYNAMIC_CONFIGURATION
// Config file parsing...
static bool
interface_handler(void * UNUSED context, const char * UNUSED filename, char **hunks, int UNUSED num_hunks,
                  int UNUSED lineno)
{
    dp_interface_t *interface = calloc(1, sizeof *interface);
    if (interface == NULL) {
        ERROR("Unable to allocate interface %s", hunks[1]);
        return false;
    }

    interface->name = strdup(hunks[1]);
    if (interface->name == NULL) {
        ERROR("Unable to allocate interface name %s", hunks[1]);
        free(interface);
        return false;
    }

    if (!strcmp(hunks[0], "nopush")) {
        interface->no_push = true;
    }

    if (new_served_domain(interface, hunks[2]) == NULL) {
        free(interface->name);
        free(interface);
        return false;
    }
    return true;
}

static bool
port_handler(void * UNUSED context, const char * UNUSED filename, char **hunks, int UNUSED num_hunks, int UNUSED lineno)
{
    char *ep = NULL;
    long port = strtol(hunks[1], &ep, 10);
    if (port < 0 || port > 65535 || *ep != 0) {
        ERROR("Invalid port number: %s", hunks[1]);
        return false;
    }
    if (!strcmp(hunks[0], "udp-port")) {
        dnssd_proxy_udp_port = port;
    } else if (!strcmp(hunks[0], "tcp-port")) {
        dnssd_proxy_tcp_port = port;
    } else if (!strcmp(hunks[0], "tls-port")) {
        dnssd_proxy_tls_port = port;
    }
    return true;
}

static bool
config_string_handler(char **ret, const char * UNUSED filename, const char *string, int UNUSED lineno, bool tdot,
                                  bool ldot)
{
    char *s;
    int add_trailing_dot = 0;
    int add_leading_dot = ldot ? 1 : 0;
    size_t len = strlen(string);

    // Space for NUL and leading dot.
    if (tdot && len > 0 && string[len - 1] != '.') {
        add_trailing_dot = 1;
    }
    s = malloc(strlen(string) + add_leading_dot + add_trailing_dot + 1);
    if (s == NULL) {
        ERROR("Unable to allocate domain name %s", string);
        return false;
    }
    *ret = s;
    if (ldot) {
        *s++ = '.';
    }
    memcpy(s, string, len + add_leading_dot + add_trailing_dot + 1);
    if (add_trailing_dot) {
        s[len] = '.';
        s[len + 1] = 0;
    }
    return true;
}

static bool
my_name_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    static char *new_name = NULL;
    if (new_name != NULL) {
        free(new_name);
        my_name = NULL;
        new_name = NULL;
    }
    if (!config_string_handler(&new_name, filename, hunks[1], lineno, false, false)) {
        return false;
    }
    my_name = new_name;
    size_t len = strlen(my_name);
    size_t bigger = sizeof(DOT_HOME_NET_DOMAIN) > sizeof(DOT_LOCAL) ? sizeof(DOT_HOME_NET_DOMAIN) : sizeof(DOT_LOCAL);
    if (len >= sizeof(local_host_name) - bigger) {
        ERROR("truncating local hostname %s", my_name);
        return false;
    }

    // Set up existing local host name with .local. suffix
    snprintf(local_host_name_dot_local, sizeof(local_host_name_dot_local), "%s" DOT_LOCAL, my_name);

    // Set up existing local host name with .home.net. suffix
    snprintf(local_host_name, sizeof(local_host_name), "%s" DOT_HOME_NET_DOMAIN, my_name);
    return true;
}

static bool
listen_addr_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    if (num_listen_addrs == MAX_ADDRS) {
        ERROR("Only %d IPv4 listen addresses can be configured.", MAX_ADDRS);
        return false;
    }
    return config_string_handler(&listen_addrs[num_listen_addrs++], filename, hunks[1], lineno, false, false);
}

static bool
publish_addr_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    if (num_publish_addrs == MAX_ADDRS) {
        ERROR("Only %d addresses can be published.", MAX_ADDRS);
        return false;
    }
    return config_string_handler(&publish_addrs[num_publish_addrs++], filename, hunks[1], lineno, false, false);
}

static bool
tls_key_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    return config_string_handler(&tls_key_filename, filename, hunks[1], lineno, false, false);
}

static bool
tls_cert_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    return config_string_handler(&tls_cert_filename, filename, hunks[1], lineno, false, false);
}

static bool
tls_cacert_handler(void * UNUSED context, const char *filename, char **hunks, int UNUSED num_hunks, int lineno)
{
    return config_string_handler(&tls_cacert_filename, filename, hunks[1], lineno, false, false);
}

config_file_verb_t dp_verbs[] = {
    { "interface",    3, 3, interface_handler },    // interface <name> <domain>
    { "nopush",       3, 3, interface_handler },    // nopush <name> <domain>
    { "udp-port",     2, 2, port_handler },         // udp-port <number>
    { "tcp-port",     2, 2, port_handler },         // tcp-port <number>
    { "tls-port",     2, 2, port_handler },         // tls-port <number>
    { "my-name",      2, 2, my_name_handler },      // my-name <domain name>
    { "tls-key",      2, 2, tls_key_handler },      // tls-key <filename>
    { "tls-cert",     2, 2, tls_cert_handler },     // tls-cert <filename>
    { "tls-cacert",   2, 2, tls_cacert_handler },   // tls-cacert <filename>
    { "listen-addr",  2, 2, listen_addr_handler },  // listen-addr <IP address>
    { "publish-addr", 2, 2, publish_addr_handler }  // publish-addr <IP address>
};
#define NUMCFVERBS ((sizeof dp_verbs) / sizeof (config_file_verb_t))
#endif // !SRP_FEATURE_DYNAMIC_CONFIGURATION

static wakeup_t *tls_listener_wakeup;
static int tls_listener_index;
static void dnssd_tls_listener_restart(comm_t *NONNULL listener, void *NULLABLE context);

static void dnssd_tls_key_change_notification_send(void)
{
    static int dnssd_tls_change_notification_token = NOTIFY_TOKEN_INVALID;

    if (dnssd_tls_change_notification_token == NOTIFY_TOKEN_INVALID) {
        uint32_t notifyStatus = notify_register_check(kDNSSDAdvertisingProxyTLSKeyUpdateNotification,
                                                      &dnssd_tls_change_notification_token);
        if (notifyStatus != NOTIFY_STATUS_OK) {
            dnssd_tls_change_notification_token = NOTIFY_TOKEN_INVALID;
            ERROR("notify_register_check(%s) failed with %u", kDNSSDAdvertisingProxyTLSKeyUpdateNotification, notifyStatus);
            return;
        }
    }

    if (dnssd_tls_change_notification_token != NOTIFY_TOKEN_INVALID) {
        uint32_t notifyStatus = notify_post(kDNSSDAdvertisingProxyTLSKeyUpdateNotification);
        if (notifyStatus != NOTIFY_STATUS_OK) {
            ERROR("notify_post(%s) %u", kDNSSDAdvertisingProxyTLSKeyUpdateNotification, notifyStatus);
            notify_cancel(dnssd_tls_change_notification_token);
            dnssd_tls_change_notification_token = NOTIFY_TOKEN_INVALID;
        }
    }
}

static void
dnssd_tls_listener_ready(void *UNUSED context, uint16_t port)
{
#ifdef SRP_TEST_SERVER
    if (srp_test_dnssd_tls_listener_ready != NULL) {
        srp_test_dnssd_tls_listener_ready(srp_test_tls_listener_context, port);
    }
#else
    (void)context;
    (void)port;
#endif
}

static void dnssd_tls_listener_listen(void *context, bool init_daemon)
{
    addr_t addr;
    INFO("starting DoT listener");
    memset(&addr, 0, sizeof(addr));
    addr.sa.sa_family = AF_UNSPEC;
#ifndef NOT_HAVE_SA_LEN
    addr.sa.sa_len = sizeof(addr.sin6);
#endif
    addr.sin6.sin6_port = htons(dnssd_proxy_tls_port);
#ifndef EXCLUDE_TLS
    dnssd_proxy_listeners[tls_listener_index] =
            ioloop_listener_create(true, true, init_daemon, NULL, 0, &addr, NULL, "DNS over TLS",
                                   dns_proxy_input, NULL, dnssd_tls_listener_restart, dnssd_tls_listener_ready,
                                   NULL, srp_tls_configure, 0, context);
#else
    dnssd_proxy_listeners[tls_listener_index] =
        ioloop_listener_create(true, true, init_daemon, NULL, 0, &addr, NULL, "DNS over TLS",
                               dns_proxy_input, NULL, dnssd_tls_listener_restart, dnssd_tls_listener_ready,
                               NULL, NULL, 0, context);
#endif
    if (dnssd_proxy_listeners[tls_listener_index] == NULL) {
        ERROR("DNS Push listener: fail.");
        goto exit;
    }

    // Notify about intial key update
    dnssd_tls_key_change_notification_send();

    // Schedule a wake up timer to rotate the expired TLS certificate.
    schedule_tls_certificate_rotation(&tls_listener_wakeup, dnssd_proxy_listeners[tls_listener_index]);
exit:
    return;
}

static void dnssd_tls_listener_relisten(void *context)
{
    dnssd_tls_listener_listen(context, false);
}

static void
dnssd_tls_listener_restart(comm_t *UNUSED in_listener, void *context)
{
    const bool doing_rotation = dnssd_proxy_listeners[tls_listener_index]->tls_rotation_ready;
    ioloop_listener_release(dnssd_proxy_listeners[tls_listener_index]);
    dnssd_proxy_listeners[tls_listener_index] = NULL;

    if (doing_rotation) {
        const bool succeeded = srp_tls_init();
        if (!succeeded) {
            FAULT("srp_tls_init failed.");
            return;
        }

        // Send TLS key update notification
        dnssd_tls_key_change_notification_send();
        dnssd_tls_listener_listen(NULL, false);
    } else {
        INFO("Creation of TLS listener failed; reattempting in 10s.");

        if (tls_listener_wakeup == NULL) {
            tls_listener_wakeup = ioloop_wakeup_create();
            if (tls_listener_wakeup == NULL) {
                ERROR("Unable to allocate wakeup in order to re-attempt TLS listener creation.");
                return;
            }
        }
        ioloop_add_wake_event(tls_listener_wakeup, context, dnssd_tls_listener_relisten, NULL, 10 * MSEC_PER_SEC);
    }
}

static void
dnssd_push_setup(void)
{
    tls_listener_index = dnssd_proxy_num_listeners++;
    dnssd_tls_listener_listen(NULL, true);

    // Only set hardwired response when dynamic configuration is enabled.  Dynamic configuration
    // sets up hardwired response when new address of the interface is added.
#if SRP_FEATURE_DYNAMIC_CONFIGURATION // not set hardwired response for now
    dnssd_hardwired_push_setup();
#endif // !SRP_FEATURE_DYNAMIC_CONFIGURATION
}

#if (!SRP_FEATURE_CAN_GENERATE_TLS_CERT)

// Start a key generation or cert signing program.   Arguments are key=value pairs.
// Arguments that can be constant should be <"key=value", NULL>.   Arguments that
// have a variable component should be <"key", value">.  References to arguments
// will be held, except that if the rhs of the pair is variable, memory is allocated
// to store the key=value pair, so the neither the key nor the value is retained.
// The callback is called when the program exits.

static void
keyprogram_start(const char *program, subproc_callback_t callback, ...)
{
#define MAX_SUBPROC_VARS 3
    size_t lens[MAX_SUBPROC_VARS];
    char *vars[MAX_SUBPROC_VARS];
    int num_vars = 0;
    char *argv[MAX_SUBPROC_ARGS + 1];
    int argc = 0;
    va_list vl;
    int i;
    subproc_t *subproc = NULL;

    va_start(vl, callback);
    while (true) {
        char *vname, *value;
        char *arg;

        vname = va_arg(vl, char *);
        if (vname == NULL) {
            break;
        }
        value = va_arg(vl, char *);

        if (argc >= MAX_SUBPROC_ARGS) {
            ERROR("keyprogram_start: too many arguments.");
        }

        if (value == NULL) {
            arg = vname;
        } else {
            if (num_vars >= MAX_SUBPROC_VARS) {
                ERROR("Too many variable args: %s %s", vname, value);
                goto out;
            }
            lens[num_vars] = strlen(vname) + strlen(value) + 2;
            vars[num_vars] = malloc(lens[num_vars]);
            if (vars[num_vars] == NULL) {
                ERROR("No memory for variable key=value %s %s", vname, value);
                goto out;
            }
            snprintf(vars[num_vars], lens[num_vars], "%s=%s", vname, value);
            arg = vars[num_vars];
            num_vars++;
        }
        argv[argc++] = arg;
    }
    argv[argc] = NULL;
    subproc = ioloop_subproc(program, argv, argc, callback, NULL, NULL);
    if (subproc != NULL) {
        ioloop_subproc_run_sync(subproc);
        ioloop_subproc_release(subproc);
    }
out:
    for (i = 0; i < num_vars; i++) {
        free(vars[i]);
    }
}

static bool
finished_okay(const char *context, int status, const char *error)
{
    // If we get an error, something failed before the program had been successfully started.
    if (error != NULL) {
        ERROR("%s failed on startup: %s", context, error);
    }

    // The key file generation process completed
    else if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            ERROR("%s program exited with status %d", context, status);
            // And that means we don't have DNS Push--sorry!
        } else {
            return true;
        }
    } else if (WIFSIGNALED(status)) {
        ERROR("%s program exited on signal %d", context, WTERMSIG(status));
        // And that means we don't have DNS Push--sorry!
    } else if (WIFSTOPPED(status)) {
        ERROR("%s program stopped on signal %d", context, WSTOPSIG(status));
        // And that means we don't have DNS Push--sorry!
    } else {
        ERROR("%s program exit status unknown: %d", context, status);
        // And that means we don't have DNS Push--sorry!
    }
    return false;
}

// Called after the cert has been generated.
static void
certfile_finished_callback(void *NULLABLE context, int status, const char *error)
{
    (void)context;

    // If we were able to generate a cert, we can start DNS Push service and start advertising it.
    if (finished_okay("Certificate signing", status, error)) {
        int i = dnssd_proxy_num_listeners;

        dnssd_push_setup();

        for (; i < dnssd_proxy_num_listeners; i++) {
            INFO("Started " PUB_S_SRP, dnssd_proxy_listeners[i]->name);
        }
    }
}

// Called after the key has been generated.
static void
keyfile_finished_callback(void *context, int status, const char *error)
{
    (void)context;
    if (finished_okay("Keyfile generation", status, error)) {
        INFO("Keyfile generation completed.");

    // XXX dates need to not be constant!!!
    keyprogram_start(CERTWRITE_PROGRAM, certfile_finished_callback,
                     "selfsign=1", NULL, "issuer_key", tls_key_filename, "issuer_name=CN", my_name,
                     "not_before=20210825000000", NULL, "not_after=20230824235959", NULL, "is_ca=1", NULL,
                     "max_pathlen=0", NULL, "output_file", tls_cert_filename, NULL);
    }

}
#endif // #if (SRP_FEATURE_CAN_GENERATE_TLS_CERT)

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
static served_domain_t *NONNULL
add_new_served_domain_with_interface(const char *const NONNULL name,
                                     const addr_t *const NULLABLE address, const addr_t *const NULLABLE mask)
{
    dp_interface_t *new_interface = NULL;
    served_domain_t *served_domain = NULL;
    bool local_only_interface = !strcmp(LOCAL_ONLY_PSEUDO_INTERFACE, name);
    bool locally_served_interface = !local_only_interface && !strcmp(ALL_LOCALS_PSEUDO_INTERFACE, name);
#if SRP_FEATURE_LOCAL_DISCOVERY
    bool infrastructure_interface = !locally_served_interface && !strcmp(INFRASTRUCTURE_PSEUDO_INTERFACE, name);
#endif
    bool succeeded;

    new_interface = calloc(1, sizeof(*new_interface));
    require_action_quiet(new_interface != NULL, exit, succeeded = false;
        ERROR("calloc failed - name: " PRI_S_SRP ", allocate size: %lu", name, sizeof(*new_interface)));

    new_interface->name = strdup(name);
    require_action_quiet(new_interface->name != NULL, exit, succeeded = false;
        ERROR("strdup failed to copy interface name - interface name: " PRI_S_SRP, name));

    if (local_only_interface) {
        new_interface->ifindex = kDNSServiceInterfaceIndexLocalOnly;
    } else if (locally_served_interface) {
        new_interface->ifindex = kDNSServiceInterfaceIndexAny;
#if SRP_FEATURE_LOCAL_DISCOVERY
    } else if (infrastructure_interface) {
        new_interface->ifindex = kDNSServiceInterfaceIndexInfra;
#endif
    } else {
        new_interface->ifindex = if_nametoindex(name);
    }

    // Enable DNS push by default.
    new_interface->no_push = false;

    if (address != NULL) {
        require_action_quiet(mask != NULL, exit, succeeded = false);

        new_interface->addresses = calloc(1, sizeof(*new_interface->addresses));
        require_action_quiet(new_interface->addresses != NULL, exit, succeeded = false;
            ERROR("calloc failed - allocated size: %lu", sizeof(*new_interface->addresses)));
        new_interface->addresses->addr = *address;
        new_interface->addresses->mask = *mask;
    }

    char *per_interface_served_domain;
#if STUB_ROUTER
    char served_domain_buffer[DNS_MAX_NAME_SIZE];
#endif
    if (local_only_interface) {
        // All queries sent to <Thread ID>.thread.home.arpa. will only be proxied to local only interface.
        per_interface_served_domain = THREAD_DOMAIN_WITH_ID;
    } else if (locally_served_interface) {
        per_interface_served_domain = DEFAULT_SERVICE_ARPA_DOMAIN;
#if SRP_FEATURE_LOCAL_DISCOVERY
    } else if (infrastructure_interface) {
        per_interface_served_domain = DOT_LOCAL_DOMAIN;
#endif
    } else {
#if STUB_ROUTER
        int bytes_written = snprintf(served_domain_buffer, sizeof(served_domain_buffer),
            "%s-%s." HOME_NET_DOMAIN, local_host_name, name);
        require_action_quiet(bytes_written > 0 && (size_t)bytes_written < sizeof(served_domain_buffer), exit,
            succeeded = false;
            ERROR("snprintf failed - local host name: " PRI_S_SRP ", interface name: " PUB_S_SRP
                ", name buffer size: %lu", my_name, name, sizeof(served_domain_buffer))
        );
        per_interface_served_domain = served_domain_buffer;
#else
        ERROR("unexpected served domain " PRI_S_SRP, name);
        succeeded = false;
        goto exit;
#endif
    }

    served_domain = new_served_domain(new_interface, per_interface_served_domain);
    require_action_quiet(served_domain != NULL, exit, succeeded = false;
        ERROR("new_served_domain failed - interface name: " PUB_S_SRP ", served domain: " PRI_S_SRP,
            name, per_interface_served_domain)
    );

    succeeded = true;
    INFO("new served domain added with interface - served domain: " PUB_S_SRP ", interface name: " PUB_S_SRP,
        per_interface_served_domain, name);
exit:
    if (!succeeded) {
        if (new_interface != NULL) {
            if (new_interface->addresses != NULL) {
                verify_action(new_interface->addresses->next == NULL,
                    ERROR("multiple addresses added for this new interface"));
                free(new_interface->addresses);
            }
            if (new_interface->name != NULL) {
                free(new_interface->name);
            }
            free(new_interface);
        }
    }

    return served_domain;
}
#endif // !SRP_FEATURE_DYNAMIC_CONFIGURATION

static bool
interface_addr_t_equal(const interface_addr_t *const NONNULL a, const interface_addr_t *const NONNULL b)
{
    bool equal;
    const addr_t *const a_addr = &a->addr;
    const addr_t *const a_mask = &a->mask;
    const addr_t *const b_addr = &b->addr;
    const addr_t *const b_mask = &b->mask;

    if (a_addr->sa.sa_family != b_addr->sa.sa_family) {
        equal = false;
        goto exit;
    }

    require_action_quiet(a_addr->sa.sa_family == a_mask->sa.sa_family, exit, equal = false;
        ERROR("A address and mask are not in the same sa_family - address family: %d, mask family: %d",
            a_addr->sa.sa_family, a_mask->sa.sa_family)
    );

    require_action_quiet(b_addr->sa.sa_family == b_mask->sa.sa_family, exit, equal = false;
        ERROR("B address and mask are no in the same sa_family - address family: %d, mask family: %d",
            b_addr->sa.sa_family, b_mask->sa.sa_family)
    );

    if (a_addr->sa.sa_family == AF_INET) {
        if (a_addr->sin.sin_addr.s_addr != b_addr->sin.sin_addr.s_addr) {
            equal = false;
            goto exit;
        }

        if (a_mask->sin.sin_addr.s_addr != b_mask->sin.sin_addr.s_addr) {
            equal = false;
            goto exit;
        }
    } else { // a_addr->sa.sa_family == AF_INET6
        if (memcmp(&a_addr->sin6.sin6_addr, &b_addr->sin6.sin6_addr, sizeof(a_addr->sin6.sin6_addr)) != 0) {
            equal = false;
            goto exit;
        }

        if (memcmp(&a_mask->sin6.sin6_addr, &b_mask->sin6.sin6_addr, sizeof(a_mask->sin6.sin6_addr)) != 0) {
            equal = false;
            goto exit;
        }
    }

    equal = true;
exit:
    return equal;
}

static bool
interface_add_new_address(dp_interface_t *const NONNULL interface, const addr_t *const NONNULL address,
                          const addr_t *const NONNULL mask)
{
    bool succeeded;

    interface_addr_t *new_if_addr = calloc(1, sizeof(*new_if_addr));
    require_action_quiet(new_if_addr != NULL, exit, succeeded = false;
        ERROR("calloc failed - allocated size: %zu", sizeof(*new_if_addr)));
    new_if_addr->addr = *address;
    new_if_addr->mask = *mask;
    new_if_addr->next = NULL;

    interface_addr_t **ap;

    for (ap = &interface->addresses; *ap != NULL; ap = &(*ap)->next)
        ;

    *ap = new_if_addr;
    succeeded = true;;
exit:
    return succeeded;
}

static bool
interface_remove_old_address(dp_interface_t *const NONNULL interface, const addr_t *const NONNULL address,
                             const addr_t *const NONNULL mask)
{
    bool succeeded;
    interface_addr_t addr_to_remove = {NULL, *address, *mask};
    interface_addr_t **ap;
    interface_addr_t *current;

    for (ap = &interface->addresses; *ap != NULL; ap = &(*ap)->next) {
        if (interface_addr_t_equal(*ap, &addr_to_remove)) {
            break;
        }
    }
    if (*ap == NULL) {
        INFO("address not found in the interface address list - interface name: " PUB_S_SRP, interface->name);
        succeeded = false;
        goto exit;
    }
    current = *ap;
    *ap = current->next;
    free(current);

    succeeded = true;
exit:
    return succeeded;
}

static bool
interface_process_addr_change(dp_interface_t *const NONNULL interface, const addr_t *const NONNULL address,
                              const addr_t *const NONNULL mask, const enum interface_address_change event_type)
{
    bool succeeded;

    require_action_quiet(event_type != interface_address_unchanged, exit, succeeded = false;
        INFO("no address change event happens"));

    if (event_type == interface_address_added) {
        succeeded = interface_add_new_address(interface, address, mask);
    } else { // event_type == interface_address_removed
        succeeded = interface_remove_old_address(interface, address, mask);
    }

    INFO("address added/removed successfully - event: " PUB_S_SRP,
        event_type == interface_address_added ? "added" : "removed");

exit:
    return succeeded;
}

static void
towire_init(dns_wire_t * const NONNULL wire_ptr, dns_towire_state_t * const NONNULL towire_ptr)
{
    memset(wire_ptr, 0, sizeof(*wire_ptr));
    memset(towire_ptr, 0, sizeof(*towire_ptr));
    towire_ptr->message = wire_ptr;
    towire_ptr->lim = &wire_ptr->data[DNS_DATA_SIZE];
    towire_ptr->p = wire_ptr->data;
}

#if STUB_ROUTER
static bool
string_ends_with(const char *const NONNULL str, const char *const NONNULL suffix)
{
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    bool ret;

    if (str_len < suffix_len) {
        ret = false;
        goto exit;
    }

    if (strcmp(str + (str_len-suffix_len), suffix) != 0) {
        ret = false;
        goto exit;
    }

    ret = true;
exit:
    return ret;
}
#endif

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
#if STUB_ROUTER
static bool
served_domain_change_domain_name(void)
{
    bool succeeded = true;

    served_domain_t *next;
    for (served_domain_t *current = served_domains; current != NULL; current = next) {
        next = current->next;
        if (!string_ends_with(current->domain, HOME_NET_DOMAIN)) {
            continue;
        }
        // Skip local only interface because only served domain <Thread ID>.thread.home.arpa. does not contain domain
        // string.
        if (current->interface != NULL && strcmp(LOCAL_ONLY_PSEUDO_INTERFACE, current->interface->name) == 0) {
            continue;
        }

        // Constructs new served domain name.
        char *new_served_domain_name;
        char new_served_domain_buff[DNS_MAX_NAME_SIZE];

        if (0) {
        } else if (current->interface != NULL) { // <local host name>-<interface name>.home.arpa.
            const dp_interface_t *const interface = current->interface;
            int bytes_written = snprintf(new_served_domain_buff, sizeof(new_served_domain_buff),
                "%s-%s." HOME_NET_DOMAIN, local_host_name, interface->name);
            require_action_quiet(bytes_written > 0 && (size_t)bytes_written < sizeof(new_served_domain_buff), exit,
                succeeded = false; ERROR("snprintf failed"));
            new_served_domain_name = new_served_domain_buff;
        } else { // <local host name>.home.arpa.
            int bytes_written = snprintf(new_served_domain_buff, sizeof(new_served_domain_buff),
                "%s." HOME_NET_DOMAIN, local_host_name);
            require_action_quiet(bytes_written > 0 && (size_t)bytes_written < sizeof(new_served_domain_buff), exit,
                succeeded = false; ERROR("snprintf failed"));
            new_served_domain_name = new_served_domain_buff;
        }

        INFO("Updating served domain from " PRI_S_SRP " to " PRI_S_SRP, current->domain, new_served_domain_name);

        // Free the old served domain name.
        free(current->domain_ld);
        dns_name_free(current->domain_name);

        // Set the new served domain name.
        size_t domain_len = strlen(new_served_domain_name);
        current->domain_ld = malloc(domain_len + 2);
        require_action_quiet(current->domain_ld != NULL, for_loop_exit, succeeded = false;
            ERROR("malloc failed - allocated length: %zu", domain_len + 2));
        current->domain_ld[0] = '.';
        current->domain = current->domain_ld + 1;
        memcpy(current->domain, new_served_domain_name, domain_len);
        current->domain[domain_len] = '\0';

        current->domain_name = dns_pres_name_parse(current->domain);
        require_action_quiet(current->domain_name != NULL, for_loop_exit, succeeded = false;
            ERROR("failed to create parsed DNS name - domain name to be parsed: " PRI_S_SRP, current->domain)
        );

    for_loop_exit:
        if (!succeeded) {
            delete_served_domain(current);
        }
    }

exit:
    return succeeded;
}
#endif // STUB_ROUTER

static bool
served_domain_process_name_change(void)
{
    bool succeeded;

    // Deletes all hardwired response set in the served domain.
    dnssd_hardwired_clear();

#if STUB_ROUTER
    // Since local host name changes, we need to reflect the change in the served domain name.
    succeeded = served_domain_change_domain_name();
    require_action_quiet(succeeded, exit, ERROR("served_domain_change_domain_name failed"));
#endif

    // Re-set the hardwired response
    dnssd_hardwired_setup();

    // Re-set the DNS push hardwired response
    dnssd_hardwired_push_setup();

    succeeded = true;
#if STUB_ROUTER
exit:
#endif
    return succeeded;
}

static bool
initialize_uuid_name(srp_server_t *UNUSED server_state)
{
    char *s;
    uint64_t uuid = srp_random64();
    static const char letters[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static int letlen = sizeof(letters) - 1;
    s = uuid_name;
    *s++ = 'u'; // So that it always starts with a letter.
    while (s < uuid_name + sizeof(uuid_name) - 1 && uuid != 0) {
        *s++ = letters[uuid % letlen];
        uuid /= letlen;
    }
    *s++ = 0;
    return true;
}

static bool
update_my_name(CFStringRef local_host_name_cfstr)
{
    bool succeeded;
    size_t name_length;

    if (local_host_name_cfstr == NULL) {
        // If we are a thread device and not a stub router, make up a hostname for the remote server in case we need it.
        char localhost[] = "localhost.";
        name_length = sizeof(localhost);
        memcpy(local_host_name, localhost, name_length);
        memcpy(local_host_name_dot_local, localhost, name_length);
        memcpy(my_name_buf, localhost, name_length);
    } else {
        // local host name to c string.
        succeeded = CFStringGetCString(local_host_name_cfstr, local_host_name, sizeof(local_host_name),
                                       kCFStringEncodingUTF8);
        require_action_quiet(succeeded, exit, succeeded = false;
                             ERROR("CFStringGetCString failed - local host name: " PRI_S_SRP,
                                   CFStringGetCStringPtr(local_host_name_cfstr, kCFStringEncodingUTF8))
            );
        name_length = strlen(local_host_name);

        // Validate the local host name.
        for (size_t i = 0; i < name_length; i++) {
            char ch = local_host_name[i];
            bool is_valid_char = isalnum(ch) || (ch == '-');
            require_action_quiet(is_valid_char, exit, succeeded = false;
                                 ERROR("invalid DNS name - name: " PUB_S_SRP, local_host_name));
        }

        require_action_quiet(name_length + sizeof(DOT_HOME_NET_DOMAIN) <= sizeof(my_name_buf),
                             exit,
                             succeeded = false;
                             ERROR("generated name too long: " PUB_S_SRP DOT_HOME_NET_DOMAIN, local_host_name));

        // Update existing local host name in my_name.
        memcpy(my_name_buf, local_host_name, name_length);
        memcpy(my_name_buf + name_length, DOT_HOME_NET_DOMAIN, sizeof(DOT_HOME_NET_DOMAIN));

        // Update existing local host name with .local suffix.
        int bytes_written = snprintf(local_host_name_dot_local, sizeof(local_host_name_dot_local), "%s" DOT_LOCAL, local_host_name);
        if (bytes_written < 0 || (size_t) bytes_written > sizeof(local_host_name_dot_local)) {
           ERROR("snprintf failed - name length: %lu, max: %lu", strlen(local_host_name) + sizeof(DOT_LOCAL),
                 sizeof(local_host_name_dot_local));
           succeeded = false;
           goto exit;
        }
    }
    my_name = my_name_buf;

    succeeded = true;
    INFO(PUB_S_SRP " my_name: " PRI_S_SRP ", local host name: " PRI_S_SRP, my_name == NULL ? "initialized" : "updated",
         my_name, local_host_name_dot_local);

exit:
    return succeeded;
}

// Gets called when name change event happens
static void
monitor_name_changes_callback(SCDynamicStoreRef store, CFArrayRef changed_keys, void *context)
{
    bool succeeded;
    CFStringRef local_host_name_cfstring = NULL;
    dnssd_proxy_advertisements_t *advertisements = context;

    // Check if name changes.
    CFRange range = {0, CFArrayGetCount(changed_keys)};
    const bool host_name_changed = CFArrayContainsValue(changed_keys, range, sc_dynamic_store_key_host_name);
    if (!host_name_changed) {
        goto exit;
    }

    // Get the new local host name.
    local_host_name_cfstring = SCDynamicStoreCopyLocalHostName(store);
    require_action_quiet(local_host_name_cfstring != NULL, exit, ERROR("failed to get updated local host name"));

    // Update the old my_name
    succeeded = update_my_name(local_host_name_cfstring);
    require_action_quiet(succeeded, exit, ERROR("failed to update my name"));

    // With the new local host name, update the served domains and hardwired response.
    succeeded = served_domain_process_name_change();
    require_action_quiet(succeeded, exit, ERROR("failed to process name change for served domains"));

    if (advertisements->txn != NULL) {
        dns_wire_t wire;
        dns_towire_state_t towire;
        towire_init(&wire, &towire);
        dns_full_name_to_wire(NULL, &towire, local_host_name_dot_local);

        DNSServiceErrorType err = DNSServiceUpdateRecord(advertisements->service_ref, advertisements->ns_record_ref, 0,
            towire.p - wire.data, wire.data, 0);
        if (err != kDNSServiceErr_NoError) {
            ERROR("DNSServiceUpdateRecord failed to update NS record to new name - name: " PRI_S_SRP,
                local_host_name_dot_local);
        }

        INFO("Updating record - new NS record rdata: " PRI_S_SRP, local_host_name_dot_local);
    }

exit:
    if (local_host_name_cfstring != NULL) {
        CFRelease(local_host_name_cfstring);
    }
    return;
}

static bool
monitor_name_changes(dnssd_proxy_advertisements_t *advertisements)
{
    bool succeeded;
    SCDynamicStoreRef store = NULL;
    const void *monitored_keys[1];
    CFArrayRef monitored_keys_array = NULL;

    // Set the callback function for name change event.
    store = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("dnssd-proxy:watch for name change events"),
                                 monitor_name_changes_callback, &advertisements->sc_context);
    require_action_quiet(store != NULL, exit, succeeded = false; ERROR("failed to create SCDynamicStoreRef"));

    // Set the key to be monitored, which is host name
    sc_dynamic_store_key_host_name = SCDynamicStoreKeyCreateHostNames(kCFAllocatorDefault);
    require_action_quiet(sc_dynamic_store_key_host_name != NULL, exit, succeeded = false;
        ERROR("failed to create SCDynamicStoreKey for host name"));

    monitored_keys[0] = sc_dynamic_store_key_host_name;
    monitored_keys_array = CFArrayCreate(kCFAllocatorDefault, monitored_keys, countof(monitored_keys),
        &kCFTypeArrayCallBacks);
    require_action_quiet(monitored_keys_array != NULL, exit, succeeded = false;
        ERROR("failed to create CFArrayRef for monitored keys"));

    succeeded = SCDynamicStoreSetNotificationKeys(store, monitored_keys_array, NULL);
    require_action_quiet(succeeded, exit, ERROR("SCDynamicStoreSetNotificationKeys failed"));

    succeeded = SCDynamicStoreSetDispatchQueue(store, dispatch_get_main_queue());
    require_action_quiet(succeeded, exit, ERROR("SCDynamicStoreSetDispatchQueue failed"));

    succeeded = true;
    INFO("Start to monitor local host name changes");
exit:
    if (!succeeded) {
        if (store != NULL) {
            CFRelease(store);
        }
    }
    if (monitored_keys_array != NULL) {
        CFRelease(monitored_keys_array);
    }
    return succeeded;
}

static bool
initialize_my_name_and_monitoring(srp_server_t *server_state)
{
    bool succeeded;
    CFStringRef local_host_name_cfstring = NULL;

    // Set notification from configd.
    succeeded = monitor_name_changes(server_state->dnssd_proxy_advertisements);
    require_action_quiet(succeeded, exit, ERROR("failed to monitor name changes"));

    // Get the initial local host name
    local_host_name_cfstring = SCDynamicStoreCopyLocalHostName(NULL);
    require_action_quiet(local_host_name != NULL, exit, succeeded = false; ERROR("failed to get local host name"));

    succeeded = update_my_name(local_host_name_cfstring);
    require_action_quiet(succeeded, exit, ERROR("failed to update myname"));

exit:
    if (local_host_name_cfstring != NULL) {
        CFRelease(local_host_name_cfstring);
    }
    return succeeded;
}

#ifndef SRP_TEST_SERVER
static bool
configure_dnssd_proxy(void)
{
    dnssd_proxy_udp_port= 53;
    dnssd_proxy_tcp_port = 53;
    dnssd_proxy_tls_port = 853;
    return true;
}
#endif // SRP_TEST_SERVER
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

static bool
start_dnssd_proxy_listener(void)
{
    bool succeeded;

#if STUB_ROUTER
#ifndef NOT_HAVE_SA_LEN
#  define SA_LEN_INIT addr.sa.sa_len = sizeof(addr.sin6)
#else
#  define SA_LEN_INIT
#endif // NOT_HAVE_SA_LEN
#define INIT_ADDR_T(PORT)                       \
        do {                                    \
            memset(&addr, 0, sizeof(addr));     \
            addr.sa.sa_family = AF_UNSPEC;      \
            addr.sin6.sin6_port = htons(PORT);  \
            SA_LEN_INIT;                        \
        } while (false)

    addr_t addr;

    INIT_ADDR_T(dnssd_proxy_udp_port);
    dnssd_proxy_listeners[dnssd_proxy_num_listeners] =
        ioloop_listener_create(false, false, true, NULL, 0, &addr, NULL, "DNS over UDP", dns_proxy_input,
                               NULL, NULL, NULL, NULL, NULL, 0, NULL);
    require_action_quiet(dnssd_proxy_listeners[dnssd_proxy_num_listeners] != NULL, exit, succeeded = false;
        ERROR("failed to start UDP listener - listener index: %d", dnssd_proxy_num_listeners));
    dnssd_proxy_num_listeners++;

    INIT_ADDR_T(dnssd_proxy_tcp_port);
    dnssd_proxy_listeners[dnssd_proxy_num_listeners] =
        ioloop_listener_create(true, false, true, NULL, 0, &addr, NULL, "DNS over TCP", dns_proxy_input,
                               NULL, NULL, NULL, NULL, NULL, 0, NULL);
    require_action_quiet(dnssd_proxy_listeners[dnssd_proxy_num_listeners] != NULL, exit, succeeded = false;
        ERROR("failed to start TCP listener - listener index: %d", dnssd_proxy_num_listeners));
    dnssd_proxy_num_listeners++;
#endif // STUB_ROUTER

    dnssd_push_setup();

    for (int i = 0; i < dnssd_proxy_num_listeners; i++) {
        INFO("listener started - name: " PUB_S_SRP, dnssd_proxy_listeners[i]->name);
    }

    succeeded = true;
    goto exit;

exit:
    return succeeded;
}

#define ADVERTISEMENT_RETRY_TIMER 10 * MSEC_PER_SEC

#if STUB_ROUTER
static void
advertisements_finalize(void *context)
{
    dnssd_proxy_advertisements_t *advertisements_context = context;
    advertisements_context->txn = NULL;
}

static void
advertisements_failed(void *UNUSED context, int status)
{
    ERROR("%d", status);
}

static void
advertisements_callback(DNSServiceRef sd_ref, DNSRecordRef record_ref, DNSServiceFlags UNUSED flags,
                        DNSServiceErrorType error, void *context)
{
    dnssd_proxy_advertisements_t *advertisements_context = context;

    if (error == kDNSServiceErr_NoError) {
        const char * const description = record_ref == advertisements_context->ns_record_ref ? "NS" : "PTR";
        INFO("record registered successfully - registered: " PUB_S_SRP, description);
    } else if (error == kDNSServiceErr_ServiceNotRunning) {
        // The record is not being advertised because mDNSResponder stopped running for some reason (like crashes),
        // in which case, we will stop the previous DNSService operation and start a new one 10s later.

        // Release the previous DNSServiceRef.
        if (advertisements_context->service_ref != sd_ref) {
            ERROR("Invalid DNSServiceRef - context->service_ref: %p, sd_ref: %p", advertisements_context->service_ref,
                sd_ref);
        }
        if (advertisements_context->txn != NULL) {
            ioloop_dnssd_txn_cancel(advertisements_context->txn);
            ioloop_dnssd_txn_release(advertisements_context->txn);
            advertisements_context->txn = NULL;
        }
        advertisements_context->service_ref = NULL;

        // Restart the advertisement.
        bool succeeded = start_timer_to_advertise(advertisements_context, NULL, ADVERTISEMENT_RETRY_TIMER);
        if (!succeeded) {
            ERROR("start_timer_to_advertise failed");
        } else {
            INFO("mDNSResponder stopped running, preparing to re-advertise the PTR and NS records");
        }
    } else {
        ERROR("record not registered - error: %d", error);
    }
}

static void
advertise_dnssd_proxy_callback(void *NONNULL context)
{
    DNSServiceErrorType err;
    bool succeeded;
    bool dns_service_initialized = false;
    dns_wire_t wire;
    dns_towire_state_t towire;
    dnssd_proxy_advertisements_t *advertisement_context = context;
    srp_server_t *server_state = advertisement_context->server_state;
    const char *const domain_to_advertise = advertisement_context->domain_to_advertise;

    INFO("Start advertising lb._dns-sd._udp.local. PTR and openthread.thread.home.arpa.local NS records");

    // Create DNSServiceRef
    err = DNSServiceCreateConnection(&advertisement_context->service_ref);
    if (err != kDNSServiceErr_NoError) {
        ERROR("DNSServiceCreateConnection failed");
        succeeded = false;
        goto exit;
    }
    dns_service_initialized = true;

    // Setup lb._dns-sd._udp.local. PTR openthread.thread.home.arpa.
    towire_init(&wire, &towire);
    dns_full_name_to_wire(NULL, &towire, domain_to_advertise);

    err = DNSServiceRegisterRecord(advertisement_context->service_ref, &advertisement_context->ptr_record_ref,
                                   kDNSServiceFlagsShared, server_state->advertise_interface, AUTOMATIC_BROWSING_DOMAIN,
                                   kDNSServiceType_PTR, kDNSServiceClass_IN, towire.p - wire.data, wire.data, 0,
                                   advertisements_callback, advertisement_context);
    if (err != kDNSServiceErr_NoError) {
        ERROR("DNSServiceRegisterRecord failed - record: " PUB_S_SRP " PTR " PRI_S_SRP, AUTOMATIC_BROWSING_DOMAIN,
              domain_to_advertise);
        succeeded = false;
        goto exit;
    }

    // Setup openthread.thread.home.arpa. NS <local host name>.local.
    towire_init(&wire, &towire);
    dns_full_name_to_wire(NULL, &towire, local_host_name_dot_local);

    err = DNSServiceRegisterRecord(advertisement_context->service_ref, &advertisement_context->ns_record_ref,
                                   kDNSServiceFlagsShared | kDNSServiceFlagsForceMulticast,
                                   server_state->advertise_interface, domain_to_advertise, kDNSServiceType_NS,
                                   kDNSServiceClass_IN, towire.p - wire.data, wire.data, 0,
                                   advertisements_callback, advertisement_context);
    if (err != kDNSServiceErr_NoError) {
        ERROR("DNSServiceRegisterRecord failed - record: " PUB_S_SRP " NS " PRI_S_SRP, domain_to_advertise,
            local_host_name_dot_local);
        succeeded = false;
        goto exit;
    }

    // Start the running loop
    advertisement_context->txn = ioloop_dnssd_txn_add(advertisement_context->service_ref, advertisement_context,
                                                      advertisements_finalize, advertisements_failed);
    if (advertisement_context->txn == NULL) {
        ERROR("ioloop_dnssd_txn_add failed");
        succeeded = false;
        goto exit;
    }

    INFO("Advertising records - " PUB_S_SRP " PTR " PRI_S_SRP ", " PRI_S_SRP " NS " PRI_S_SRP,
         AUTOMATIC_BROWSING_DOMAIN, domain_to_advertise, domain_to_advertise, local_host_name_dot_local);
    succeeded = true;
exit:
    if (!succeeded) {
        if (dns_service_initialized) {
            DNSServiceRefDeallocate(advertisement_context->service_ref);
            advertisement_context->service_ref = NULL;
        }
        if (err == kDNSServiceErr_ServiceNotRunning) {
            ERROR("mDNSResponder is not running yet when trying to advertise PTR and NS records, try again 10s later");
            // advertise_dnssd_proxy_callback will be called again 10s later, since we did not cancel the timer.
        } else {
            // Other kDNSServiceErr, should be impossible. If it happens, give up advertising the records.
            ioloop_cancel_wake_event(advertisement_context->wakeup_timer);
        }
    } else {
        // Since we registered successfully, there is no need to trigger another timer to set the records.
        // Stop the timer.
        ioloop_cancel_wake_event(advertisement_context->wakeup_timer);
    }
}

static bool
start_timer_to_advertise(dnssd_proxy_advertisements_t *NONNULL context,
    const char *const NULLABLE domain_to_advertise, const uint32_t interval)
{
    bool succeeded;

    // Only create timer once.
    if (context->wakeup_timer == NULL) {
        context->wakeup_timer = ioloop_wakeup_create();
        if (context->wakeup_timer == NULL) {
            succeeded = false;
            goto exit;
        }
    }

    // Only copy advertised domain once.
    if (context->domain_to_advertise == NULL) {
        if (domain_to_advertise == NULL) {
            succeeded = false;
            goto exit;
        }

        context->domain_to_advertise = strdup(domain_to_advertise);
        if (context->domain_to_advertise == NULL) {
            succeeded = false;
            goto exit;
        }
    }

    // Start the timer, finalize callback is not necessary here because the context should always be available.
    succeeded = ioloop_add_wake_event(context->wakeup_timer, context, advertise_dnssd_proxy_callback, NULL, interval);
    if (!succeeded) {
        goto exit;
    }

    succeeded = true;
exit:
    if (!succeeded) {
        if (context->domain_to_advertise != NULL) {
            free(context->domain_to_advertise);
            context->domain_to_advertise = NULL;
        }
        if (context->wakeup_timer != NULL) {
            ioloop_wakeup_release(context->wakeup_timer);
            context->wakeup_timer = NULL;
        }
    }
    return succeeded;
}

#if SRP_FEATURE_DISCOVERY_PROXY_SERVER

static bool
start_timer_to_advertise_dnssd_dp_proxy(dnssd_dp_proxy_advertisements_t *context, uint32_t interval);

static void
dp_advertisements_finalize(void *const context)
{
    dnssd_dp_proxy_advertisements_t *advertisements_context = context;
    advertisements_context->txn = NULL;
}

static void
dp_advertisements_failed(void *const UNUSED context, const int status)
{
    ERROR("push service advertisement failed  -- error: %d", status);
}

static void
dp_advertisements_callback(const DNSServiceRef UNUSED sd_ref, const DNSServiceFlags UNUSED flags,
                           const DNSServiceErrorType error, const char *const name, const char *const reg_type,
                           const char *const domain, void *const context)
{
    dnssd_dp_proxy_advertisements_t *const ads_ctx = context;
    if (error == kDNSServiceErr_NoError) {
        INFO("Push service registered successfully -- %s.%s%s", name, reg_type, domain);
    } else if (error == kDNSServiceErr_ServiceNotRunning) {
        if (ads_ctx->txn != NULL) {
            ioloop_dnssd_txn_cancel(ads_ctx->txn);
            ioloop_dnssd_txn_forget(&ads_ctx->txn);
        }
        ads_ctx->service_ref = NULL;

        const bool succeeded = start_timer_to_advertise_dnssd_dp_proxy(ads_ctx, ADVERTISEMENT_RETRY_TIMER);
        if (!succeeded) {
            ERROR("start_timer_to_advertise_dnssd_dp_proxy failed");
        } else {
            INFO("mDNSResponder stopped running, preparing to re-advertise DNS push service");
        }
    } else {
        ERROR("Push service not registered -- error: %d", error);
    }
}

static void
advertise_dnssd_dp_proxy_callback(void *const context)
{
    DNSServiceRef service_ref = NULL;
    dnssd_txn_t *txn = NULL;
    dnssd_dp_proxy_advertisements_t *const advertisement_context = context;
    srp_server_t *const server_state = advertisement_context->server_state;
    int bytes_written = 0;

    // Construct ,_local,_openthread#thread#home#arpa
    const char *domains_support_push[] = {
        DOT_LOCAL_DOMAIN,
        THREAD_BROWSING_DOMAIN,
    };
    char subtype_domains[256];
    size_t current_len = 0;
    for (size_t i = 0; i < countof(domains_support_push); i++) {
        const char *const domain_supports_push = domains_support_push[i];
        const size_t len = strlen(domain_supports_push);
        char subtype_domain[128];
        // Convert domain name like `_openthread.thread.home.arpa` to `_openthread#thread#home#arpa`.
        require_quiet(len < sizeof(subtype_domain), exit);
        for (size_t j = 0; j < len + 1; j++) {
            if (domain_supports_push[j] == '.') {
                subtype_domain[j] = '#';
            } else {
                subtype_domain[j] = domain_supports_push[j];
            }
        }
        // Remove the trailing '#'.
        if (subtype_domain[len - 1] == '#') {
            subtype_domain[len - 1] = '\0';
        }

        bytes_written = snprintf(subtype_domains + current_len, sizeof(subtype_domains) - current_len, ",_%s",
                                 subtype_domain);
        require_quiet((bytes_written > 0) && ((size_t)bytes_written < (sizeof(subtype_domains) - current_len)), exit);
        current_len += bytes_written;
    }

    // Construct _dnssd-dp._tcp,_local,_openthread#thread#home#arpa with subtype.
    char reg_type[128];
    bytes_written = snprintf(reg_type, sizeof(reg_type), "_dnssd-dp._tcp%s", subtype_domains);
    require_quiet((bytes_written > 0) && ((size_t)bytes_written < sizeof(reg_type)), exit);

    // Construct service instance name like: p128-undulw2d1vktd1
    const uint8_t priority = 128;
    const char * const random_identifier = uuid_name;
    char name[128];
    bytes_written = snprintf(name, sizeof(name), "p%03d-%s", priority, random_identifier);
    require_quiet((bytes_written > 0) && ((size_t)bytes_written < sizeof(name)), exit);

    const uint16_t dp_port = dnssd_proxy_tls_port;

    // The registered PTRs will be like:
    // _openthread#thread#home#arpa._sub._dnssd-dp._tcp.local   PTR p128-undulw2d1vktd1._dnssd-dp._tcp.local
    // _local._sub._dnssd-dp._tcp.local                         PTR p128-undulw2d1vktd1._dnssd-dp._tcp.local
    const DNSServiceErrorType dnsssd_err = DNSServiceRegister(&service_ref, 0, server_state->advertise_interface, name,
        reg_type, DOT_LOCAL_DOMAIN, NULL, htons(dp_port), 0, NULL, dp_advertisements_callback, context);
    require_action_quiet(dnsssd_err == kDNSServiceErr_NoError, exit,
        ERROR("DNSServiceRegisterRecord failed -- error: %d", dnsssd_err));

    txn = ioloop_dnssd_txn_add(service_ref, advertisement_context, dp_advertisements_finalize,
                               dp_advertisements_failed);
    require_quiet(txn != NULL, exit);

    advertisement_context->service_ref = service_ref;
    service_ref = NULL;
    advertisement_context->txn = txn;
    txn = NULL;
    INFO("Advertising push discovery service -- reg_type: %s.%s, service instance name: %s._dnssd-dp._tcp.%s",
         reg_type, DOT_LOCAL_DOMAIN, name, DOT_LOCAL_DOMAIN);

exit:
    DNSServiceRefSourceForget(&service_ref);
    if (txn != NULL) {
        ioloop_dnssd_txn_cancel(txn);
        ioloop_dnssd_txn_forget(&txn);
    }
}

static bool
start_timer_to_advertise_dnssd_dp_proxy(dnssd_dp_proxy_advertisements_t *const context, const uint32_t interval)
{
    bool succeeded;
    wakeup_t *wakeup_timer = context->wakeup_timer;

    if (wakeup_timer == NULL) {
        wakeup_timer = ioloop_wakeup_create();
    } else {
        ioloop_wakeup_retain(wakeup_timer);
    }
    require_action_quiet(wakeup_timer != NULL, exit, succeeded = false);

    succeeded = ioloop_add_wake_event(wakeup_timer, context, advertise_dnssd_dp_proxy_callback, NULL, interval);
    require_quiet(succeeded, exit);

    if (context->wakeup_timer == NULL) {
        context->wakeup_timer = wakeup_timer;
        wakeup_timer = NULL;
    }

exit:
    ioloop_wakeup_forget(&wakeup_timer);
    return succeeded;
}

static bool
advertise_dnssd_dp_proxy(srp_server_t *const server_state)
{
    return start_timer_to_advertise_dnssd_dp_proxy(server_state->dnssd_dp_proxy_advertisements,
        ADVERTISEMENT_RETRY_TIMER);
}

static bool
is_eligible_to_provide_push_discovery_service(void)
{
    return true;
}

#endif // SRP_FEATURE_DISCOVERY_PROXY_SERVER

#endif // STUB_ROUTER

#if SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY
#  if SRP_FEATURE_DYNAMIC_CONFIGURATION
static bool
served_domain_init(srp_server_t *server_state)
{
    bool succeeded;
    served_domain_t *my_name_served_domain = NULL;
    served_domain_t *ipv6 = NULL;
    served_domain_t *ipv4 = NULL;
    served_domain_t *thread_served_domain = NULL;
    served_domain_t *default_service_arpa_domain = NULL;
#if SRP_FEATURE_LOCAL_DISCOVERY
    served_domain_t *dot_local_domain = NULL;
#endif

    // <local host name>.home.arpa.
    my_name_served_domain = new_served_domain(NULL, my_name);
    require_action_quiet(my_name_served_domain != NULL, exit, succeeded = false;
        ERROR("failed to create new served domain - domain name: " PUB_S_SRP, my_name));

#if STUB_ROUTER
    if (server_state->stub_router_enabled) {
        // ip6.arpa.
        // in-addr.arpa.
        ipv6 = new_served_domain(NULL, IPV6_REVERSE_LOOKUP_DOMAIN);
        ipv4 = new_served_domain(NULL, IPV4_REVERSE_LOOKUP_DOMAIN);
        require_action_quiet(ipv6 != NULL && ipv4 != NULL, exit, succeeded = false;
                             ERROR("failed to create new served domain for reverse look up -  domain name: " PUB_S_SRP ", " PUB_S_SRP,
                                   IPV6_REVERSE_LOOKUP_DOMAIN, IPV4_REVERSE_LOOKUP_DOMAIN)
            );
    }
#else
    (void)server_state;
#endif

    // THREAD_BROWSING_DOMAIN
    // It will be served by kDNSServiceInterfaceIndexLocalOnly, which is a pseudo interface.
    thread_served_domain = add_new_served_domain_with_interface(LOCAL_ONLY_PSEUDO_INTERFACE, NULL, NULL);
    require_action_quiet(thread_served_domain != NULL, exit, succeeded = false);
    bool hardwired_set = dnssd_hardwired_setup_for_served_domain(thread_served_domain);
    require_action_quiet(hardwired_set, exit, succeeded = false);

    // default.service.arpa
    // For Thread 1.3.0, default.service.arpa has to return all locally-discoverable services
    default_service_arpa_domain = add_new_served_domain_with_interface(ALL_LOCALS_PSEUDO_INTERFACE, NULL, NULL);
    require_action_quiet(default_service_arpa_domain != NULL, exit, succeeded = false);
    hardwired_set = dnssd_hardwired_setup_for_served_domain(default_service_arpa_domain);
    require_action_quiet(hardwired_set, exit, succeeded = false);

#if SRP_FEATURE_LOCAL_DISCOVERY
    // local
    // discovery proxy for the infrastructure interface
    dot_local_domain = add_new_served_domain_with_interface(INFRASTRUCTURE_PSEUDO_INTERFACE, NULL, NULL);
    require_action_quiet(dot_local_domain != NULL, exit, succeeded = false);
    hardwired_set = dnssd_hardwired_setup_for_served_domain(dot_local_domain);
    require_action_quiet(hardwired_set, exit, succeeded = false);
#endif

    succeeded = true;
exit:
    if (!succeeded) {
        if (thread_served_domain != NULL) {
            delete_served_domain(thread_served_domain);
        }
        if (default_service_arpa_domain != NULL) {
            delete_served_domain(default_service_arpa_domain);
        }
#if SRP_FEATURE_LOCAL_DISCOVERY
        if (dot_local_domain != NULL) {
            delete_served_domain(dot_local_domain);
        }
#endif
        if (ipv4 != NULL) {
            delete_served_domain(ipv4);
        }
        if (ipv6 != NULL) {
            delete_served_domain(ipv6);
        }
        if (my_name_served_domain != NULL) {
            delete_served_domain(my_name_served_domain);
        }
    }
    return succeeded;
}
#  endif // SRP_FEATURE_DYNAMIC_CONFIGURATION
#endif // SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY

bool
init_dnssd_proxy(srp_server_t *server_state)
{
    bool succeeded;
    dnssd_proxy_advertisements_t *advertisements = server_state->dnssd_proxy_advertisements;
    if (advertisements == NULL) {
        advertisements = calloc(1, sizeof(*advertisements));
        require_action_quiet(advertisements != NULL, exit,
                             succeeded = false;
                             ERROR("no memory for advertisements"));
        server_state->dnssd_proxy_advertisements = advertisements;
        advertisements->server_state = server_state;
        advertisements->sc_context.info = advertisements;
    }

#if STUB_ROUTER
#if SRP_FEATURE_DISCOVERY_PROXY_SERVER
    if (is_eligible_to_provide_push_discovery_service()) {
        dnssd_dp_proxy_advertisements_t *dp_ads = server_state->dnssd_dp_proxy_advertisements;
        if (dp_ads == NULL) {
            dp_ads = calloc(1, sizeof(*dp_ads));
            require_action_quiet(dp_ads != NULL, exit,
                                 succeeded = false; ERROR("no memory for push discovery service advertisements"));
            server_state->dnssd_dp_proxy_advertisements = dp_ads;
            dp_ads->server_state = server_state;
        }
    }
#endif // SRP_FEATURE_DISCOVERY_PROXY_SERVER
#endif // STUB_ROUTER

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
    succeeded = configure_dnssd_proxy();
    require_action_quiet(succeeded, exit, ERROR("configure_dnssd_proxy failed"));


    succeeded = initialize_my_name_and_monitoring(server_state);

    require_action_quiet(succeeded, exit, ERROR("initialize_my_name_and_monitoring failed"));
    succeeded = initialize_uuid_name(server_state);
    require_action_quiet(succeeded, exit, ERROR("initialize_uuid_name failed"));
#if STUB_ROUTER
    if (!server_state->stub_router_enabled) {
        served_domain_process_name_change();
    }
#endif

#else // SRP_FEATURE_DYNAMIC_CONFIGURATION
    // Read the config file
    succeeded = config_parse(NULL, "/etc/dnssd-proxy.cf", dp_verbs, NUMCFVERBS);
    require_action_quiet(succeeded,
                         exit,);

    // Insist that we have at least one address we're listening on.
    succeeded = !(num_listen_addrs == 0 && num_publish_addrs == 0);
    require_action_quiet(succeeded,
                         exit,
                         ERROR("Please configure at least one my-ipv4-addr and/or one my-ipv6-addr."));

    ioloop_map_interface_addresses(server_state, NULL, &served_domains, dnssd_proxy_ifaddr_callback);

    // Set up hardwired answers
    dnssd_hardwired_setup();
#endif // SRP_FEATURE_DYNAMIC_CONFIGURATION

    succeeded = srp_tls_init();
    require_action_quiet(succeeded, exit, ERROR("srp_tls_init failed."));

#if !SRP_FEATURE_CAN_GENERATE_TLS_CERT
    // The tls_fail flag allows us to run the proxy in such a way that TLS connections will fail.
    // This is never what you want in production, but is useful for testing.
    if (!tls_fail) {
        if (access(tls_key_filename, R_OK) < 0) {
            keyprogram_start(GENKEY_PROGRAM, keyfile_finished_callback,
                             "type=rsa", NULL, "rsa_keysize=4096", NULL, "filename", tls_key_filename, NULL);
        } else if (access(tls_cert_filename, R_OK) < 0) {
            keyfile_finished_callback(NULL, 0, NULL);
        }
        require_action_quiet(access(tls_key_filename, R_OK) >= 0, exit, ERROR("failed to create tls listener key."));
        require_action_quiet(access(tls_cert_filename, R_OK) >= 0, exit, ERROR("failed to create tls listener cert."));

        require_action_quiet(srp_tls_server_init(NULL, tls_cert_filename, tls_key_filename),
                             exit, ERROR("srp_tls_server_init failed."));
        require_action_quiet(srp_tls_client_init(), exit, ERROR("srp_tls_client_init failed."));
    }
#endif

    succeeded = start_dnssd_proxy_listener();
    require_action_quiet(succeeded, exit, ERROR("start_dnssd_proxy_listener failed"));

#if STUB_ROUTER
    if (server_state->stub_router_enabled) {
    #if SRP_FEATURE_DISCOVERY_PROXY_SERVER
        if (server_state->dnssd_dp_proxy_advertisements != NULL) {
            succeeded = advertise_dnssd_dp_proxy(server_state);
            require_action_quiet(succeeded, exit, ERROR("advertise_dnssd_dp_proxy failed"));
        }
    #endif
    }
#endif

#if SRP_FEATURE_DYNAMIC_CONFIGURATION
    succeeded = served_domain_init(server_state);
#endif

exit:
    return succeeded;
}

#if !SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY
int
main(int argc, char **argv)
{
    int i;
    bool log_stderr = false;

    dnssd_proxy_udp_port = dnssd_proxy_tcp_port = 53;
    dnssd_proxy_tls_port = 853;

    // Parse command line arguments
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--tls-fail")) {
            tls_fail = true;
        } else if (!strcmp(argv[i], "--log-stderr")) {
            log_stderr = true;
        } else {
            return usage(argv[0]);
        }
    }

    OPENLOG("dnssd-proxy", log_stderr);

    if (!ioloop_init()) {
        return 1;
    }

    init_dnssd_proxy();

    ioloop();
}
#endif // #if !SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY

#endif // (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY) || (!defined(BUILD_SRP_MDNS_PROXY) || (BUILD_SRP_MDNS_PROXY == 0))

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
