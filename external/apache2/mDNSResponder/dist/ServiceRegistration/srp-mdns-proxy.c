/* srp-mdns-proxy.c
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
 * This file contains the SRP Advertising Proxy, which is an SRP Server
 * that offers registered addresses using mDNS.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
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
#include <ctype.h>
#include <mdns/pf.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "route.h"
#include "adv-ctl-server.h"
#include "srp-replication.h"
#include "ioloop-common.h"
#include "thread-device.h"
#include "nat64-macos.h"
#include "srp-dnssd.h"
#include "ifpermit.h"
#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "omr-publisher.h"
#include "service-publisher.h"


#if SRP_FEATURE_NAT64
#include "nat64.h"
#endif


#ifdef SRP_TEST_SERVER
#include "srp-test-runner.h"
#endif

#define ADDRESS_RECORD_TTL  4500
#define OTHER_RECORD_TTL    4500

#define _DNSSD_API_AVAILABLE_FALL_2024 (1)

static const char local_suffix_ld[] = ".local";
static const char *local_suffix = &local_suffix_ld[1];

os_log_t global_os_log;
void *dns_service_op_not_to_be_freed;
srp_server_t *srp_servers;
const uint8_t thread_anycast_preamble[7] = { 0, 0, 0, 0xff, 0xfe, 0, 0xfc };
const uint8_t thread_rloc_preamble[6] = { 0, 0, 0, 0xff, 0xfe, 0 };

extern int num_push_sessions;
extern int dp_num_outstanding_queries;
extern int num_push_sessions_dropped_for_load;
extern int num_queries_dropped_for_load;

//======================================================================================================================
// MARK: - Forward references

static bool register_host_record(adv_host_t *host, adv_record_t *record, const bool skipping);
static void register_host_record_completion(DNSServiceRef sdref, DNSRecordRef rref,
                                            DNSServiceFlags flags, DNSServiceErrorType error_code, void *context);
static bool register_instance(adv_instance_t *instance);
static void register_instance_completion(DNSServiceRef sdref, DNSServiceFlags flags, DNSServiceErrorType error_code,
                                         const char *name, const char *regtype, const char *domain, void *context);
static void update_from_host(adv_host_t *host);
static void start_host_update(adv_host_t *host);
static void prepare_update(adv_host_t *host, client_update_t *client_update);
static void delete_host(void *context);
static void lease_callback(void *context);
static void adv_host_finalize(adv_host_t *host);
static void adv_record_finalize(adv_record_t *record);
static void adv_update_finalize(adv_update_t *update);

//======================================================================================================================
// MARK: - Functions

void
srp_mdns_shared_record_remove(srp_server_t *server_state, adv_record_t *record)
{
    RETAIN_HERE(record, adv_record);
    if (record->rref != NULL) {
        if (record->shared_txn != 0 && record->shared_txn == (intptr_t)server_state->shared_registration_txn) {
            INFO("removing rref %p", record->rref);
            int err = dns_service_remove_record(server_state,
                                                server_state->shared_registration_txn->sdref, record->rref, 0);
            // We can't release the record here if we got an error removing it, because if we get an error removing it,
            // it doesn't get removed from the list. This should never happen, but if it does, the record will leak.
            if (err == kDNSServiceErr_NoError) {
                RELEASE_HERE(record, adv_record); // Release the DNSService callback's reference
            } else {
                // At this point we should never see an error calling DNSServiceRemoveRecord, so if we do, call
                // attention to it.
                if (!record->update_pending) {
                    FAULT("DNSServiceRemoveRecord(%p, %p, %p, 0) returned %d",
                          server_state->shared_registration_txn->sdref, record, record->rref, err);
                }
            }
        } else {
            INFO("didn't remove stale rref %p because %lx != %p",
                 record->rref, record->shared_txn, server_state->shared_registration_txn);
        }
        record->rref = NULL;
    }
    record->shared_txn = 0;
    RELEASE_HERE(record, adv_record);
}


static void
adv_record_finalize(adv_record_t *record)
{
    // We should not be able to get to the finalize function without having removed the rref, because the DNSService
    // callback always holds a reference to the record.
    if (record->update != NULL) {
        RELEASE_HERE(record->update, adv_update);
    }
    if (record->host != NULL) {
        RELEASE_HERE(record->host, adv_host);
    }

    free(record->rdata);
    free(record);
}

static void
adv_instance_finalize(adv_instance_t *instance)
{
    if (instance->txn != NULL) {
        ioloop_dnssd_txn_cancel_srp(instance->host->server_state, instance->txn);
        ioloop_dnssd_txn_release(instance->txn);
    }
    if (instance->txt_data != NULL) {
        free(instance->txt_data);
    }
    if (instance->instance_name != NULL) {
        free(instance->instance_name);
    }
    if (instance->service_type != NULL) {
        free(instance->service_type);
    }
    if (instance->host != NULL) {
        RELEASE_HERE(instance->host, adv_host);
        instance->host = NULL;
    }
    if (instance->message != NULL) {
        ioloop_message_release(instance->message);
        instance->message = NULL;
    }
    if (instance->update != NULL) {
        RELEASE_HERE(instance->update, adv_update);
        instance->update = NULL;
    }
    if (instance->retry_wakeup != NULL) {
        ioloop_wakeup_release(instance->retry_wakeup);
        instance->retry_wakeup = NULL;
    }
    free(instance);
}

void
adv_instance_context_release(void *NONNULL context)
{
    adv_instance_t *instance = context;
    RELEASE_HERE(instance, adv_instance);
}

void
adv_instance_retain_(adv_instance_t *instance, const char *file, int line)
{
    RETAIN(instance, adv_instance);
}

void
adv_instance_release_(adv_instance_t *instance, const char *file, int line)
{
    RELEASE(instance, adv_instance);
}

void
adv_record_retain_(adv_record_t *record, const char *file, int line)
{
    RETAIN(record, adv_record);
}

void
adv_record_release_(adv_record_t *record, const char *file, int line)
{
    RELEASE(record, adv_record);
}

#define DECLARE_VEC_CREATE(type)                        \
static type ## _vec_t *                                 \
type ## _vec_create(int size)                           \
{                                                       \
    type ## _vec_t *vec;                                \
                                                        \
    vec = calloc(1, sizeof(*vec));                      \
    if (vec != NULL) {                                  \
        if (size == 0) {                                \
            size = 1;                                   \
        }                                               \
        vec->vec = calloc(size, sizeof (*(vec->vec)));  \
        if (vec->vec == NULL) {                         \
            free(vec);                                  \
            vec = NULL;                                 \
        } else {                                        \
            RETAIN_HERE(vec, type##_vec);               \
        }                                               \
    }                                                   \
    return vec;                                         \
}

#define DECLARE_VEC_COPY(type)                          \
static type ## _vec_t *                                 \
type ## _vec_copy(type ## _vec_t *vec)                  \
{                                                       \
    type ## _vec_t *new_vec;                            \
    int i;                                              \
                                                        \
    new_vec = type ## _vec_create(vec->num);            \
    if (new_vec != NULL) {                              \
        for (i = 0; i < vec->num; i++) {                \
            if (vec->vec[i] != NULL) {                  \
                new_vec->vec[i] = vec->vec[i];          \
                RETAIN_HERE(new_vec->vec[i], type);     \
            }                                           \
        }                                               \
        new_vec->num = vec->num;                        \
    }                                                   \
    return new_vec;                                     \
}

#define DECLARE_VEC_FINALIZE(type)                              \
static void                                                     \
type ## _vec_finalize(type ## _vec_t *vec)                      \
{                                                               \
    int i;                                                      \
                                                                \
    for (i = 0; i < vec->num; i++) {                            \
        if (vec->vec[i] != NULL) {                              \
            RELEASE_HERE(vec->vec[i], type);                    \
            vec->vec[i] = NULL;                                 \
        }                                                       \
    }                                                           \
    free(vec->vec);                                             \
    free(vec);                                                  \
}

DECLARE_VEC_CREATE(adv_instance);
DECLARE_VEC_COPY(adv_instance);
DECLARE_VEC_FINALIZE(adv_instance);

DECLARE_VEC_CREATE(adv_record);
DECLARE_VEC_COPY(adv_record);
DECLARE_VEC_FINALIZE(adv_record);

static void
srp_dump_server_stats(srp_server_t *server_state, bool full, bool periodic)
{
    // For testing, emit a count of how many hosts, services and address records there are
    int host_count = 0;
    int a_record_count = 0;
    int aaaa_record_count = 0;
    int instance_count = 0;
    int matter_host_count = 0;
    int hap_host_count = 0;
    int64_t now = ioloop_timenow();
    static int last_num_push_sessions;
    static int last_dp_num_outstanding_queries;
    static int last_num_push_sessions_dropped_for_load;
    static int last_num_queries_dropped_for_load;

    for (adv_host_t *hp = server_state->hosts; hp != NULL; hp = hp->next) {
        if (hp->removed) {
            continue;
        }
        host_count++;
        int expiry;
        if (hp->lease_expiry < now) {
            expiry = -1;
        } else {
            expiry = (int)((hp->lease_expiry - now) / 1000); // This should never be >MAXINT
        }
        if (full) {
            INFO("host " PRI_S_SRP " key_id %xu stable %" PRIx64 " lease %d key_lease %d expiry %d" PUB_S_SRP PUB_S_SRP,
                 hp->name, hp->key_id, hp->server_stable_id, hp->lease_interval, hp->key_lease, expiry,
                 hp->removed ? " removed" : "", hp->update_pending ? " update-pending" : "");
        }
        if (hp->addresses != NULL) {
            for (int i = 0; i < hp->addresses->num; i++) {
                if (hp->addresses->vec[i] != NULL) {
                    adv_record_t *record = hp->addresses->vec[i];
                    if (record->rrtype == dns_rrtype_a) {
                        if (full) {
                            IPv4_ADDR_GEN_SRP(record->rdata, addr_buf);
                            INFO("  IN    A " PRI_IPv4_ADDR_SRP PRI_S_SRP, IPv4_ADDR_PARAM_SRP(record->rdata, addr_buf),
                                 record->shared_txn == (intptr_t)server_state->shared_registration_txn ? " live" : "");
                        }
                        a_record_count++;
                    } else if (record->rrtype == dns_rrtype_aaaa) {
                        if (full) {
                            IPv6_ADDR_GEN_SRP((const uint8_t *)record->rdata, addr_buf);
                            INFO("  IN AAAA " PRI_IPv6_ADDR_SRP PRI_S_SRP, IPv6_ADDR_PARAM_SRP(record->rdata, addr_buf),
                                 record->shared_txn == (intptr_t)server_state->shared_registration_txn ? " live" : "");
                        }
                        aaaa_record_count++;
                    }
                }
            }
        }
        bool matter_instance_present = false, hap_instance_present = false;
        if (hp->instances != NULL) {
            for (int i = 0; i < hp->instances->num; i++) {
                adv_instance_t *instance = hp->instances->vec[i];
                if (instance != NULL) {
                    if (full) {
                        char txt_buf[DNS_DATA_SIZE];
                        if (instance->txt_data != NULL) {
                            dns_txt_data_print(txt_buf, DNS_DATA_SIZE, instance->txt_length, instance->txt_data);
                        } else {
                            txt_buf[0] = 0;
                        }
                        const char *status = "removed";
                        if (!instance->removed) {
                            if (instance->txn == NULL) {
                                status = "unregistered";
                            } else if (instance->shared_txn != (intptr_t)server_state->shared_registration_txn) {
                                status = "stale";
                            } else {
                                status = "live";
                            }
                        }
                        INFO("  " PUB_S_SRP " instance " PRI_S_SRP " " PRI_S_SRP " %d (" PRI_S_SRP ")",
                             status, instance->instance_name, instance->service_type, instance->port, txt_buf);
                    }
                    if (!instance->removed) {
                        instance_count++;
                        if (instance->service_type != NULL) {
                            const char matter_prefix[] = "_matter";
                            const char hap_prefix[] = "_hap._udp";
                            if (!strncmp(instance->service_type, matter_prefix, sizeof(matter_prefix) - 1)) {
                                matter_instance_present = true;
                            } else if (!strncmp(instance->service_type, hap_prefix, sizeof(hap_prefix) - 1)) {
                                hap_instance_present = true;
                            }
                        }
                    }
                }
            }
        }
        if (matter_instance_present) {
            matter_host_count++;
        } else if (hap_instance_present) { // If both, only count matter.
            hap_host_count++;
        }
    }
    INFO(PUB_S_SRP "%d hosts (%d matter, %d hap), %d instances, %d a records, %d aaaa records at %.6lf",
         periodic ? "" : "after update, ", host_count, matter_host_count, hap_host_count, instance_count, a_record_count,
         aaaa_record_count, srp_fractional_time());

#if STUB_ROUTER
    route_state_t *route_state = server_state->route_state;
    if (route_state) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&route_state->srp_listener_ip_address, addr_buf);
        // do we have an SRP listener?
        if (route_state->srp_listener != NULL) {
            INFO("have SRP listener on " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf),
                 route_state->srp_service_listen_port);
        } else {
            INFO("no SRP listener");
        }

        // are we publishing anycast services?
        INFO(PUB_S_SRP "advertising anycast service", route_state->advertising_srp_anycast_service ? "" : "not ");

        // are we publishing unicast service?
        if (route_state->advertising_srp_unicast_service) {
            INFO("advertising unicast service on " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&route_state->srp_listener_ip_address, addr_buf),
                 route_state->srp_service_listen_port);
        } else {
            INFO("not advertising unicast service");
        }

        // what SRP replication peers do we see? and how many are we actively connected to?
        srpl_dump_connection_states(server_state);

        // are we publishing OMR prefix?
        if (route_state->omr_publisher != NULL &&
            omr_publisher_publishing_prefix(route_state->omr_publisher))
        {
            omr_prefix_t *prefix = omr_publisher_published_prefix_get(route_state->omr_publisher);
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
            INFO("publishing " PUB_S_SRP " OMR prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d",
                 omr_publisher_publishing_dhcp(route_state->omr_publisher) ? "dhcp" : "ula",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf),
                 prefix->prefix_length);
        } else {
            INFO("not publishing OMR prefix");
        }

        // what prefixes do we see on Thread?
        if (route_state->omr_watcher != NULL) {
            omr_prefix_t *thread_prefixes = omr_watcher_prefixes_get(route_state->omr_watcher);
            for (struct omr_prefix *prefix = thread_prefixes; prefix != NULL; prefix = prefix->next) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(prefix->prefix.s6_addr, prefix_buf);
                INFO("OMR prefix " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d seen on thread" PUB_S_SRP PUB_S_SRP
                     PUB_S_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix->prefix.s6_addr, prefix_buf),
                     prefix->prefix_length, prefix->user ? " (user)" : "", prefix->ncp ? " (ncp)": "",
                     prefix->stable ? " (stable)" : "");
            }
        }

        // are we publishing infrastructure prefix?
        interface_t *interface;
        bool is_advertising = false;
        for (interface = route_state->interfaces; interface; interface = interface->next) {
            if (interface->our_prefix_advertised) {
               SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, ipv6_prefix_buf);
               INFO("advertising infrastructure prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " on " PUB_S_SRP,
                    SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, ipv6_prefix_buf),
                    interface->name);
               is_advertising = true;
            }
        }
        if (!is_advertising) {
            INFO("not advertising infrastructure prefix");
        }
    }
#endif // STUB_ROUTER

    // how many DNS push queries added since last state dump?
    // how many DNS queries seen since last state dump?
    // how many DNS queries dropped for load?
    // how many DNS Push connections dropped for load?
    INFO("%d push sessions and %d queries added, %d push sessions and %d queries dropped for load",
         num_push_sessions - last_num_push_sessions,
         dp_num_outstanding_queries - last_dp_num_outstanding_queries,
         num_push_sessions_dropped_for_load - last_num_push_sessions_dropped_for_load,
         num_queries_dropped_for_load - last_num_queries_dropped_for_load);
    last_num_push_sessions = num_push_sessions;
    last_dp_num_outstanding_queries = dp_num_outstanding_queries;
    last_num_push_sessions_dropped_for_load = num_push_sessions_dropped_for_load;
    last_num_queries_dropped_for_load = num_queries_dropped_for_load;
}

// We call advertise_finished when a client request has finished, successfully or otherwise.
#if SRP_FEATURE_REPLICATION
static bool
srp_replication_advertise_finished(adv_host_t *host, char *hostname, srp_server_t *server_state,
                                   srpl_connection_t *srpl_connection, comm_t *connection, int rcode, bool last)
{
	if (server_state->srp_replication_enabled) {
        INFO("hostname = " PRI_S_SRP "  host = %p  server_state = %p  srpl_connection = %p  connection = %p  rcode = "
             PUB_S_SRP, hostname, host, server_state, srpl_connection, connection, dns_rcode_name(rcode));
        if (connection == NULL) {
            // connection is the SRP client connection on which an update arrived. If it's null,
            // this is an SRP replication update, not an actual client we're communicating with.
            INFO("replication advertise finished: host " PRI_S_SRP ": rcode = " PUB_S_SRP,
                 hostname, dns_rcode_name(rcode));
            if (srpl_connection != NULL) {
                if (last) {
                    srpl_advertise_finished_event_send(hostname, rcode, server_state);
#ifdef SRP_TEST_SERVER
                    if (srpl_connection->srpl_advertise_finished_callback != NULL) {
                        srpl_connection->srpl_advertise_finished_callback(srpl_connection);
                    }
#endif
                }

                if (host != NULL && host->srpl_connection != NULL) {
                    if (rcode == dns_rcode_noerror) {
                        host->update_server_id = host->srpl_connection->remote_partner_id;
                        host->server_stable_id = host->srpl_connection->stashed_host.server_stable_id;
                        INFO("replicated host " PRI_S_SRP " server stable ID %" PRIx64, hostname, host->server_stable_id);
                    }

                    // This is the safest place to clear this pointer--we do not want the srpl_connection pointer to not
                    // get reset because of some weird sequence of events, leaving this host unable to be further updated
                    // or worse.
                    srpl_connection_release(host->srpl_connection);
                    host->srpl_connection = NULL;
                } else {
                    if (host != NULL) {
                        INFO("disconnected host " PRI_S_SRP " server stable ID %" PRIx64, hostname, host->server_stable_id);
                    }
                }
            } else {
                if (host != NULL) {
                    INFO("context-free host " PRI_S_SRP " server stable ID %" PRIx64, hostname, host->server_stable_id);
                }
            }
            return true;
        }

        if (host != NULL) {
            if (rcode == dns_rcode_noerror) {
                memcpy(&host->server_stable_id, &host->server_state->ula_prefix, sizeof(host->server_stable_id));
            }
            INFO("local host " PRI_S_SRP " server stable ID %" PRIx64, hostname, host->server_stable_id);
            srpl_srp_client_update_finished_event_send(host, rcode);
            host->update_server_id = 0;
        }
    } else
    {
        if (host != NULL && host->server_state != NULL) {
            memcpy(&host->server_stable_id, &host->server_state->ula_prefix, sizeof(host->server_stable_id));
            host->update_server_id = 0;
        }
    }
    return false;
}
#endif // SRP_FEATURE_REPLICATION

static void
srp_ml_eid_mapping_callback(void *context, cti_status_t status)
{
    adv_record_t *arec = context;
    adv_host_t *host = arec->host;
    SEGMENTED_IPv6_ADDR_GEN_SRP(arec->rdata, omr_buf);
    if (status == kCTIStatus_NoError) {
        if (host == NULL) {
            INFO("mapping for address " PRI_SEGMENTED_IPv6_ADDR_SRP " was orphaned.",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(arec->rdata, omr_buf));
        } else {
            INFO("mapping for address " PRI_SEGMENTED_IPv6_ADDR_SRP " to host " PRI_S_SRP " succeeded",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(arec->rdata, omr_buf), host->name);
        }
    } else {
        if (host == NULL) {
            INFO("orphaned mapping for address " PRI_SEGMENTED_IPv6_ADDR_SRP " failed: %d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(arec->rdata, omr_buf), status);
        } else {
            INFO("mapping for address " PRI_SEGMENTED_IPv6_ADDR_SRP " to host " PRI_S_SRP " failed: %d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(arec->rdata, omr_buf), host->name, status);
        }
    }
    RELEASE_HERE(arec, adv_record);
}

// We call advertise_finished when a client request has finished, successfully or otherwise.
static void
advertise_finished(adv_host_t *host, char *hostname, srp_server_t *server_state, srpl_connection_t *srpl_connection,
                   comm_t *connection, message_t *message, int rcode, client_update_t *client, bool send_response,
                   bool last)
{
    struct iovec iov;
    dns_wire_t response;

#if SRP_FEATURE_REPLICATION
    if (srp_replication_advertise_finished(host, hostname, server_state, srpl_connection, connection, rcode, last)) {
        return;
    }
#else
    (void)host;
    (void)server_state;
    (void)srpl_connection;
    (void)last;
#endif // SRP_FEATURE_REPLICATION
    INFO("host " PRI_S_SRP ": rcode = " PUB_S_SRP ", lease = %d, key_lease = %d  connection = %p", hostname, dns_rcode_name(rcode),
         client ? client->host_lease : 0, client ? client->key_lease : 0, connection);

    // This can happen if we turn off replication in the middle of an update of a replicated host.
    if (connection == NULL) {
        return;
    }
    if (!send_response) {
        INFO("not sending response.");
        return;
    }

    memset(&response, 0, DNS_HEADER_SIZE);
    response.id = message->wire.id;
    response.bitfield = message->wire.bitfield;
    dns_rcode_set(&response, rcode);
    dns_qr_set(&response, dns_qr_response);

    iov.iov_base = &response;
    // If this was a successful update, send back the lease time, which will either
    // be what the client asked for, or a shorter lease, depending on what limit has
    // been set.
    if (client != NULL) {
        dns_towire_state_t towire;
        memset(&towire, 0, sizeof towire);
        towire.p = &response.data[0];               // We start storing RR data here.
        towire.lim = &response.data[DNS_DATA_SIZE]; // This is the limit to how much we can store.
        towire.message = &response;
        response.qdcount = 0;
        response.ancount = 0;
        response.nscount = 0;
        response.arcount = htons(1);
        dns_edns0_header_to_wire(&towire, DNS_MAX_UDP_PAYLOAD, 0, 0, 1);
        dns_rdlength_begin(&towire);
        dns_u16_to_wire(&towire, dns_opt_update_lease);  // OPTION-CODE
        dns_edns0_option_begin(&towire);                 // OPTION-LENGTH
        dns_u32_to_wire(&towire, client->host_lease);    // LEASE (e.g. 1 hour)
        dns_u32_to_wire(&towire, client->key_lease);     // KEY-LEASE (7 days)
        dns_edns0_option_end(&towire);                   // Now we know OPTION-LENGTH
        dns_rdlength_end(&towire);
        // It should not be possible for this to happen; if it does, the client
        // might not renew its lease in a timely manner.
        if (towire.error) {
            ERROR("unexpectedly failed to send EDNS0 lease option.");
            iov.iov_len = DNS_HEADER_SIZE;
        } else {
            iov.iov_len = towire.p - (uint8_t *)&response;
        }
    } else {
        iov.iov_len = DNS_HEADER_SIZE;
    }
    ioloop_send_message(connection, message, &iov, 1);
}

static void
retry_callback(void *context)
{
    adv_host_t *host = (adv_host_t *)context;
    if (host->update == NULL) {
        update_from_host(host);
    } else {
        start_host_update(host);
    }
}

static void
srp_adv_host_context_release(void *context)
{
    adv_host_t *host = context;
    RELEASE_HERE(host, adv_host);
}

static void
wait_retry(adv_host_t *host)
{
    int64_t now = ioloop_timenow();
#define MIN_HOST_RETRY_INTERVAL 15
#define MAX_HOST_RETRY_INTERVAL 120
    // If we've been retrying long enough for the lease to expire, give up.
    if (!host->lease_expiry || host->lease_expiry < now) {
        INFO("host lease has expired, not retrying: lease_expiry = %" PRId64
             " now = %" PRId64 " difference = %" PRId64, host->lease_expiry, now, host->lease_expiry - now);
        delete_host(host);
        return;
    }
    if (host->retry_interval == 0) {
        host->retry_interval = MIN_HOST_RETRY_INTERVAL;
    } else if (host->retry_interval < MAX_HOST_RETRY_INTERVAL) {
        host->retry_interval *= 2;
    }
    INFO("waiting %d seconds...", host->retry_interval);
    ioloop_add_wake_event(host->retry_wakeup, host, retry_callback, srp_adv_host_context_release, host->retry_interval * 1000);
    RETAIN_HERE(host, adv_host);
}

static void
shared_registration_fail(void *context, int UNUSED status)
{
    srp_server_t *server_state = context;
    dnssd_txn_t *txn = server_state->shared_registration_txn;
    DNSServiceRef sdref = txn == NULL ? NULL : txn->sdref;
    INFO("shared registration failed: txn %p sdref %p", server_state->shared_registration_txn, sdref);
    if (txn != NULL) {
        ioloop_dnssd_txn_cancel(txn);
        ioloop_dnssd_txn_release(txn);
        server_state->shared_registration_txn = NULL;
    }
}

bool
srp_mdns_shared_registration_txn_setup(srp_server_t *server_state)
{
    if (server_state->shared_registration_txn == NULL) {
        DNSServiceRef sdref;
        int err = DNSServiceCreateConnection(&sdref);
        if (err != kDNSServiceErr_NoError) {
            return false;
        }
        server_state->shared_registration_txn = ioloop_dnssd_txn_add(sdref, server_state, NULL, shared_registration_fail);
        if (server_state->shared_registration_txn == NULL) {
            ERROR("unable to create shared connection for registration.");
            dns_service_op_not_to_be_freed = NULL;
            DNSServiceRefDeallocate(sdref);
            return false;
        }
        dns_service_op_not_to_be_freed = server_state->shared_registration_txn->sdref;
        INFO("server_state->shared_registration_txn = %p  sdref = %p", server_state->shared_registration_txn, sdref);
    }
    return true;
}

static void
record_txn_forget(adv_record_t *record, intptr_t affected_service_pointer,
                  const char *parent_type, const void *parent_pointer, const char *hostname)
{
    if (record == NULL) {
        return;
    }
    if (record->rref != NULL && record->shared_txn == affected_service_pointer) {
        INFO("forgetting rref %p on " PUB_S_SRP " %p " PRI_S_SRP, record->rref, parent_type, parent_pointer, hostname);
        record->rref = NULL;
    }
}

static void
record_vec_txns_forget(adv_record_vec_t *records, intptr_t affected_service_pointer,
                       const char *parent_type, const void *parent_pointer, const char *hostname)
{
    if (records == NULL) {
        return;
    }
    for (int i = 0; i < records->num; i++) {
        record_txn_forget(records->vec[i], affected_service_pointer, parent_type, parent_pointer, hostname);
    }
}

static void
instance_vec_txns_forget(adv_instance_vec_t *instances, intptr_t affected_service_pointer,
                         const char *parent_type, const void *parent_pointer, const char *hostname)
{
    if (instances == NULL) {
        return;
    }
    for (int i = 0; i < instances->num; i++) {
        adv_instance_t *instance = instances->vec[i];
        if (instance != NULL && instance->txn != NULL && instance->txn->sdref != NULL &&
            instance->shared_txn == affected_service_pointer)
        {
            INFO("forgetting sdref %p on " PUB_S_SRP " %p " PRI_S_SRP " instance " PRI_S_SRP " . " PRI_S_SRP,
                 instance->txn->sdref,
                 parent_type, parent_pointer, hostname, instance->instance_name, instance->service_type);
            instance->txn->sdref = NULL;
        }
    }
}

static void
host_txns_forget(adv_host_t *host, intptr_t affected_service_pointer)
{
    // We call this when the shared transaction fails for some reason. That failure invalidates all the subsidiary
    // RecordRefs and ServiceRefs hanging off of the shared transaction; to avoid holding on to invalid pointers,
    // we traverse the registration database and NULL out all the rrefs and sdrefs that relate to the subsidiary
    // service pointer.
    record_vec_txns_forget(host->addresses, affected_service_pointer, "host", host, host->name);
    instance_vec_txns_forget(host->instances, affected_service_pointer, "host", host, host->name);
    record_txn_forget(host->key_record, affected_service_pointer, "host key", host, host->name);
    if (host->update != NULL) {
        record_vec_txns_forget(host->update->remove_addresses, affected_service_pointer,
                               "host update remove addresses", host->update, host->name);
        record_vec_txns_forget(host->update->add_addresses, affected_service_pointer,
                               "host update add addresses", host->update, host->name);
        record_txn_forget(host->update->key, affected_service_pointer, "host update key", host->update, host->name);
        instance_vec_txns_forget(host->update->update_instances, affected_service_pointer,
                                 "host update update instances", host->update, host->name);
        instance_vec_txns_forget(host->update->remove_instances, affected_service_pointer,
                                 "host update remove instances", host->update, host->name);
        instance_vec_txns_forget(host->update->renew_instances, affected_service_pointer,
                                 "host update renew instances", host->update, host->name);
        instance_vec_txns_forget(host->update->add_instances, affected_service_pointer,
                                 "host update add instances", host->update, host->name);
    }
}

static void
service_disconnected(srp_server_t *server_state, intptr_t service_pointer)
{
    if (service_pointer == (intptr_t)server_state->shared_registration_txn &&
        server_state->shared_registration_txn != NULL)
    {
        INFO("server_state->shared_registration_txn = %p  sdref = %p",
             server_state->shared_registration_txn, server_state->shared_registration_txn->sdref);
        // For every host that's active right now that has transactions on this shared transaction, forget all those
        // transactions. The txn_cancel following this will free all of the memory in the client stub.
        for (adv_host_t *host = server_state->hosts; host != NULL; host = host->next) {
            host_txns_forget(host, service_pointer);
        }
        dns_service_op_not_to_be_freed = NULL;
        ioloop_dnssd_txn_cancel(server_state->shared_registration_txn);
        ioloop_dnssd_txn_release(server_state->shared_registration_txn);
        server_state->shared_registration_txn = NULL;
    }
}

static void
adv_record_vec_remove_update(adv_record_vec_t *vec, adv_update_t *update)
{
    for (int i = 0; i < vec->num; i++) {
        if (vec->vec[i] != NULL && vec->vec[i]->update != NULL && vec->vec[i]->update == update) {
            RELEASE_HERE(vec->vec[i]->update, adv_update);
            vec->vec[i]->update = NULL;
        }
    }
}

static void
adv_instance_vec_remove_update(adv_instance_vec_t *vec, adv_update_t *update)
{
    for (int i = 0; i < vec->num; i++) {
        if (vec->vec[i] != NULL && vec->vec[i]->update != NULL && vec->vec[i]->update == update) {
            RELEASE_HERE(vec->vec[i]->update, adv_update);
            vec->vec[i]->update = NULL;
        }
    }
}

static void
adv_instances_cancel(adv_instance_vec_t *instances)
{
    for (int i = 0; i < instances->num; i++) {
        adv_instance_t *instance = instances->vec[i];
        if (instance != NULL) {
            if (instance->txn != NULL) {
                ioloop_dnssd_txn_cancel_srp(instance->host->server_state, instance->txn);
                ioloop_dnssd_txn_release(instance->txn);
                instance->txn = NULL;
            }
            if (instance->retry_wakeup != NULL) {
                ioloop_cancel_wake_event(instance->retry_wakeup);
                ioloop_wakeup_release(instance->retry_wakeup);
                instance->retry_wakeup = NULL;
            }
        }
    }
}

static void
adv_update_free_instance_vectors(adv_update_t *NONNULL update)
{
    if (update->update_instances != NULL) {
        adv_instance_vec_remove_update(update->update_instances, update);
        adv_instances_cancel(update->update_instances);
        RELEASE_HERE(update->update_instances, adv_instance_vec);
        update->update_instances = NULL;
    }
    if (update->remove_instances != NULL) {
        adv_instance_vec_remove_update(update->remove_instances, update);
        RELEASE_HERE(update->remove_instances, adv_instance_vec);
        update->remove_instances = NULL;
    }
    if (update->renew_instances != NULL) {
        adv_instance_vec_remove_update(update->renew_instances, update);
        RELEASE_HERE(update->renew_instances, adv_instance_vec);
        update->renew_instances = NULL;
    }
    if (update->add_instances != NULL) {
        adv_instance_vec_remove_update(update->add_instances, update);
        adv_instances_cancel(update->add_instances);
        RELEASE_HERE(update->add_instances, adv_instance_vec);
        update->add_instances = NULL;
    }
}

static void
adv_update_finalize(adv_update_t *NONNULL update)
{
    if (update->host != NULL) {
        RELEASE_HERE(update->host, adv_host);
    }

    if (update->client != NULL) {
        srp_parse_client_updates_free(update->client);
        update->client = NULL;
    }

    if (update->remove_addresses != NULL) {
        adv_record_vec_remove_update(update->remove_addresses, update);
        RELEASE_HERE(update->remove_addresses, adv_record_vec);
    }

    if (update->add_addresses != NULL) {
        adv_record_vec_remove_update(update->add_addresses, update);
        RELEASE_HERE(update->add_addresses, adv_record_vec);
    }

    adv_update_free_instance_vectors(update);
    if (update->key != NULL) {
        RELEASE_HERE(update->key, adv_record);
    }
    free(update);
}

static void
adv_update_cancel(adv_update_t *NONNULL update)
{
    adv_host_t *host = update->host;
    bool faulted = false;

    RETAIN_HERE(update, adv_update); // ensure that update remains valid for the duration of this function call.

    if (host != NULL) {
        RETAIN_HERE(host, adv_host); // in case the update is holding the last reference to the host
        RELEASE_HERE(update->host, adv_host);
        update->host = NULL;

        INFO("cancelling update %p for host " PRI_S_SRP, update, host->registered_name);

        if (host->update == update) {
            RELEASE_HERE(host->update, adv_update);
            host->update = NULL;
        }

        // In case we needed to re-register some of the host's addresses, remove the update pointer from them.
        if (host->addresses != NULL) {
            for (int i = 0; i < host->addresses->num; i++) {
                adv_record_t *record = host->addresses->vec[i];
                if (record->update == update) {
                    RELEASE_HERE(host->addresses->vec[i]->update, adv_update);
                    record->update = NULL;
                }
            }
        }
    } else {
        INFO("canceling update with no host.");
    }

    adv_update_free_instance_vectors(update);

    if (update->add_addresses != NULL) {
        // Any record that we attempted to add as part of this update should be removed because the update failed.
        for (int i = 0; i < update->add_addresses->num; i++) {
            adv_record_t *record = update->add_addresses->vec[i];
            if (record != NULL) {
                if (host == NULL) {
                    if (!faulted) {
                        FAULT("unable to clean up host address registration because host object is gone from update.");
                        faulted = true;
                    }
                } else {
                    if (record->rref != NULL) {
                        srp_mdns_shared_record_remove(host->server_state, record);
                    }
                }
            }
        }
        adv_record_vec_remove_update(update->add_addresses, update);
        RELEASE_HERE(update->add_addresses, adv_record_vec);
        update->add_addresses = NULL;
    }

    if (update->remove_addresses != NULL) {
        adv_record_vec_remove_update(update->remove_addresses, update);
        RELEASE_HERE(update->remove_addresses, adv_record_vec);
        update->remove_addresses = NULL;
    }

    if (update->key != NULL) {
        if (update->key->update != NULL) {
            RELEASE_HERE(update->key->update, adv_update);
            update->key->update = NULL;
        }
        // Any record that we attempted to add as part of this update should be removed because the update failed.
        if (update->key->rref != NULL) {
            if (host == NULL) {
                if (!faulted) {
                    FAULT("unable to clean up host key registration because host object is gone from update.");
                    faulted = true;
                }
            } else {
                srp_mdns_shared_record_remove(host->server_state, update->key);
            }
        }
        RELEASE_HERE(update->key, adv_record);
        update->key = NULL;
    }
    if (host != NULL) {
        RELEASE_HERE(host, adv_host);
    }
    RELEASE_HERE(update, adv_update);
}

static void
update_failed(adv_update_t *update, int rcode, bool expire, bool send_response)
{
    // Retain the update for the life of this function call, since we may well release the last other reference to it.
    RETAIN_HERE(update, adv_update);

    // If we still have a client waiting for the result of this update, tell it we failed.
    // Updates that have never worked are abandoned when the client is notified.
    if (update->client != NULL) {
        adv_host_t *host = update->host;
        client_update_t *client = update->client;
        adv_update_cancel(update);
        advertise_finished(host, host->name, host->server_state, host->srpl_connection,
                           client->connection, client->message, rcode, NULL, send_response, true);
        srp_parse_client_updates_free(client);
        update->client = NULL;
        // If we don't have a lease yet, or the old lease has expired, remove the host.
        // However, if the expire flag is false, it's because we're already finalizing the
        // host, so doing an expiry here would double free the host. In this case, we leave
        // it to the caller to do the expiry (really, to finalize the host).
        if (expire && (host->lease_expiry == 0 || host->lease_expiry <= ioloop_timenow())) {
            delete_host(host);
        }
        RELEASE_HERE(update, adv_update);
        return;
    }
    adv_update_cancel(update);
    RELEASE_HERE(update, adv_update);
}

static void
host_addr_free(adv_host_t *host)
{
    int i;

    // We can't actually deallocate the address vector until the host object is collected, so deallocate the address
    // records.
    if (host->addresses != NULL) {
        for (i = 0; i < host->addresses->num; i++) {
            if (host->addresses->vec[i] != NULL) {
                INFO("Removing AAAA record for " PRI_S_SRP, host->registered_name);
                srp_mdns_shared_record_remove(host->server_state, host->addresses->vec[i]);
                RELEASE_HERE(host->addresses->vec[i], adv_record);
                host->addresses->vec[i] = NULL;
            }
        }
        host->addresses->num = 0;
    }
}

// Free just those parts that are no longer needed when the host is no longer valid.
static void
host_invalidate(adv_host_t *host)
{
    // Get rid of the retry wake event.
    if (host->retry_wakeup != NULL) {
        ioloop_cancel_wake_event(host->retry_wakeup);
    }
    if (host->re_register_wakeup != NULL) {
        ioloop_cancel_wake_event(host->re_register_wakeup);
    }

    // Remove the address records.
    host_addr_free(host);

    // Remove the services.
    if (host->instances != NULL) {
        adv_instances_cancel(host->instances);
        RELEASE_HERE(host->instances, adv_instance_vec);
        host->instances = NULL;
    }

    if (host->update != NULL) {
        RELEASE_HERE(host->update, adv_update);
        host->update = NULL;
    }
    if (host->key_record != NULL) {
        srp_mdns_shared_record_remove(host->server_state, host->key_record);
        RELEASE_HERE(host->key_record, adv_record);
        host->key_record = NULL;
    }
    host->update = NULL;
    host->removed = true;
}

// Free everything associated with the host, including the host object.
static void
adv_host_finalize(adv_host_t *host)
{
    // Just in case this hasn't happened yet, free the non-identifying host data and cancel any outstanding
    // transactions.
    host_invalidate(host);

    if (host->addresses != NULL) {
        RELEASE_HERE(host->addresses, adv_record_vec);
        host->addresses = NULL;
    }

    if (host->key_rdata != NULL) {
        free(host->key_rdata);
        host->key_rdata = NULL;
    }
    if (host->key_record != NULL) {
        RELEASE_HERE(host->key_record, adv_record);
        host->key_record = NULL;
    }

    if (host->message != NULL) {
        ioloop_message_release(host->message);
        host->message = NULL;
    }

    // We definitely don't want a lease callback at this point.
    if (host->lease_wakeup != NULL) {
        ioloop_cancel_wake_event(host->lease_wakeup);
        ioloop_wakeup_release(host->lease_wakeup);
        host->lease_wakeup = NULL; // this will make us crash if we use it after free
    }
    // Get rid of the retry wake event.
    if (host->retry_wakeup != NULL) {
        ioloop_cancel_wake_event(host->retry_wakeup);
        ioloop_wakeup_release(host->retry_wakeup);
        host->retry_wakeup = NULL;
    }

    if (host->re_register_wakeup != NULL) {
        ioloop_cancel_wake_event(host->re_register_wakeup);
        ioloop_wakeup_release(host->re_register_wakeup);
        host->re_register_wakeup = NULL;
    }
    INFO("removed " PRI_S_SRP ", key_id %x", host->name ? host->name : "<null>", host->key_id);

    // In the default case, host->name and host->registered_name point to the same memory: we don't want a double free.
    if (host->registered_name == host->name) {
        host->registered_name = NULL;
    }
    if (host->name != NULL) {
        free(host->name);
    }
    if (host->registered_name != NULL) {
        free(host->registered_name);
    }
    free(host);
}

void
srp_adv_host_release_(adv_host_t *host, const char *file, int line)
{
    RELEASE(host, adv_host);
}

void
srp_adv_host_retain_(adv_host_t *host, const char *file, int line)
{
    RETAIN(host, adv_host);
}

bool
srp_adv_host_valid(adv_host_t *host)
{
    // If the host has been removed, it's not valid.
    if (host->removed) {
        return false;
    }
    // If there is no key data, the host is invalid.
    if (host->key_rdata == NULL) {
        return false;
    }
    return true;
}

int
srp_current_valid_host_count(srp_server_t *server_state)
{
    adv_host_t *host;
    int count = 0;
    for (host = server_state->hosts; host; host = host->next) {
        if (srp_adv_host_valid(host)) {
            count++;
        }
    }
    return count;
}

int
srp_hosts_to_array(srp_server_t *server_state, adv_host_t **host_array, int max)
{
    int count = 0;
    for (adv_host_t *host = server_state->hosts; count < max && host != NULL; host = host->next) {
        if (srp_adv_host_valid(host)) {
            host_array[count] = host;
            RETAIN_HERE(host_array[count], adv_host);
            count++;
        }
    }
    return count;
}

adv_host_t *
srp_adv_host_copy_(srp_server_t *server_state, dns_name_t *name, const char *file, int line)
{
    for (adv_host_t *host = server_state->hosts; host; host = host->next) {
        if (srp_adv_host_valid(host) && dns_names_equal_text(name, host->name)) {
            RETAIN(host, adv_host);
            return host;
        }
    }
    return NULL;
}

static void
host_remove(adv_host_t *host)
{
    // This host is no longer valid. Get rid of the associated transactions and other stuff that's not required to
    // identify it, and then release the host list reference to it.
    host_invalidate(host);
    // Note that while adv_host_finalize calls host_invalidate, adv_host_finalize won't necessarily be called here because there
    // may be outstanding references on the host. It's okay to call host_invalidate twice--the second time it should be
    // a no-op.
    RELEASE_HERE(host, adv_host);
}

static adv_host_t **
host_ready(adv_host_t *host)
{
    adv_host_t **p_hosts;

    // Find the host on the list of hosts.
    for (p_hosts = &host->server_state->hosts; *p_hosts != NULL; p_hosts = &(*p_hosts)->next) {
        if (*p_hosts == host) {
            break;
        }
    }
    if (*p_hosts == NULL) {
        ERROR("called with nonexistent host.");
        return NULL;
    }

    // It's possible that we got an update to this host, but haven't processed it yet.  In this
    // case, we don't want to get rid of the host, but we do want to get rid of it later if the
    // update fails.  So postpone the removal for a bit.
    if (host->update != NULL) {
        INFO("reached with pending updates on host " PRI_S_SRP ".", host->registered_name);
        ioloop_add_wake_event(host->lease_wakeup, host, lease_callback, srp_adv_host_context_release, 10 * 1000);
        RETAIN_HERE(host, adv_host);
        host->lease_expiry = ioloop_timenow() + 10 * 1000; // ten seconds
        return NULL;
    }

    return p_hosts;
}

static void
lease_callback(void *context)
{
    int64_t now = ioloop_timenow();
    adv_host_t **p_hosts, *host = context;
    int i, num_instances = 0;

    p_hosts = host_ready(host);
    if (p_hosts == NULL) {
        INFO("host expired");
        return;
    }

    INFO("host " PRI_S_SRP, host->name);

    // If the host entry lease has expired, any instance leases have also.
    if (host->lease_expiry < now) {
        delete_host(host);
        return;
    }

    INFO("host " PRI_S_SRP " is still alive", host->name);

    if (host->instances == NULL) {
        INFO("no instances");
        return;
    }

    // Find instances that have expired and release them.
    for (i = 0; i < host->instances->num; i++) {
        adv_instance_t *instance = host->instances->vec[i];
        if (instance == NULL) {
            continue;
        }
        if (instance->lease_expiry < now) {
            INFO("host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP " has expired",
                 host->name, instance->instance_name, instance->service_type);
            // We have to release the transaction so that we can release the reference the transaction has to the instance.
            if (instance->txn != NULL) {
                dnssd_txn_t *txn = instance->txn;
                instance->txn = NULL;
                ioloop_dnssd_txn_release(txn);
            }
            host->instances->vec[i] = NULL;
            RELEASE_HERE(instance, adv_instance);
            continue;
        } else {
            INFO("host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP " has not expired",
                 host->name, instance->instance_name, instance->service_type);
        }
        num_instances++;
    }

    int64_t next_lease_expiry = host->lease_expiry;

    // Get rid of holes in the host instance vector and compute the next lease callback time
    int j = 0;

    for (i = 0; i < host->instances->num; i++) {
        if (host->instances->vec[i] != NULL) {
            adv_instance_t *instance = host->instances->vec[i];
            host->instances->vec[j++] = instance;
            if (next_lease_expiry > instance->lease_expiry) {
                next_lease_expiry = instance->lease_expiry;
            }
        }
    }
    INFO("host " PRI_S_SRP " lost %d instances", host->name, host->instances->num - j);
    host->instances->num = j;

    // Now set a timer for the next lease expiry event
    uint64_t when = next_lease_expiry - now;
    if (when > INT32_MAX) {
        when = INT32_MAX;
    }

    ioloop_add_wake_event(host->lease_wakeup, host, lease_callback, srp_adv_host_context_release, (uint32_t)when);
    RETAIN_HERE(host, adv_host);
}

// Called when we definitely want to make all the advertisements associated with a host go away.
static void
delete_host(void *context)
{
    adv_host_t **p_hosts, *host = context;


    p_hosts = host_ready(host);
    if (p_hosts == NULL) {
        return;
    }

    INFO("deleting host " PRI_S_SRP, host->name);

    // De-link the host.
    *p_hosts = host->next;

    // Get rid of any transactions attached to the host, any timer events, and any other associated data.
    host_remove(host);
}

// We remember the message that produced this instance so that if we get an update that doesn't update everything,
// we know which instances /were/ updated by this particular message. instance->recent_message is a copy of the pointer
// to the message that most recently updated this instance. When we set instance->recent_message, we don't yet know
// if the update is going to succeed; if it fails, we can't have changed update->message. If it succeeds, then when we
// get down to srp_mdns_update_finished, we can compare the message that did the update to instance->recent_message; if they
// are the same, then we set the message on the instance.
// Note that we only set instance->recent_message during register_instance_completion, so there's no timing race that
// could happen as a result of receiving a second update to the same instance before the first has been processed.
static void
set_instance_message(adv_instance_t *instance, message_t *message)
{
    if (message != NULL && (ptrdiff_t)message == instance->recent_message) {
        if (instance->message != NULL) {
            ioloop_message_release(instance->message);
        }
        instance->message = message;
        ioloop_message_retain(instance->message);
        instance->recent_message = 0;
    }
}

void
srp_mdns_update_finished(adv_update_t *update)
{
    adv_host_t *host = update->host;
    client_update_t *client = update->client;
    int num_addresses = 0;
    adv_record_vec_t *addresses = NULL;
    int num_instances = 0;
    adv_instance_vec_t *instances = NULL;
    int i, j;
    int num_host_addresses = 0;
    int num_add_addresses = 0;
    int num_host_instances = 0;
    int num_add_instances = 0;
    message_t *message = NULL;
    client_update_t *remaining_updates = NULL;
    srp_server_t *server_state = host->server_state;

    // Get the message that produced the update, if any
    if (client != NULL) {
        message = client->message;
    }

    // Once an update has finished, we need to apply all of the proposed changes to the host object.
    if (host->addresses != NULL) {
        for (i = 0; i < host->addresses->num; i++) {
            if (host->addresses->vec[i] != NULL &&
                (update->remove_addresses == NULL || update->remove_addresses->vec[i] == NULL))
            {
                num_host_addresses++;
            }
        }
    }

    if (update->add_addresses != NULL) {
        for (i = 0; i < update->add_addresses->num; i++) {
            if (update->add_addresses->vec[i] != NULL) {
                num_add_addresses++;
            }
        }
    }

    num_addresses = num_host_addresses + num_add_addresses;
    if (num_addresses > 0) {
        addresses = adv_record_vec_create(num_addresses);
        if (addresses == NULL) {
            update_failed(update, dns_rcode_servfail, true, true);
            return;
        }

        j = 0;

        if (host->addresses != NULL) {
            for (i = 0; i < host->addresses->num; i++) {
                adv_record_t *rec = host->addresses->vec[i];
                if (rec != NULL && (update->remove_addresses == NULL || update->remove_addresses->vec[i] == NULL))
                {
#ifdef DEBUG_VERBOSE
                    uint8_t *rdp = rec->rdata;
                    if (rec->rrtype == dns_rrtype_aaaa) {
                        SEGMENTED_IPv6_ADDR_GEN_SRP(rdp, rdp_buf);
                        INFO("retaining " PRI_SEGMENTED_IPv6_ADDR_SRP "on host " PRI_S_SRP,
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(rdp, rdp_buf), host->registered_name);
                    } else {
                        IPv4_ADDR_GEN_SRP(rdp, rdp_buf);
                        INFO("retaining " PRI_IPv4_ADDR_SRP "on host " PRI_S_SRP,
                             IPv4_ADDR_PARAM_SRP(rdp, rdp_buf), host->registered_name);
                    }
#endif
                    addresses->vec[j] = rec;
                    RETAIN_HERE(addresses->vec[j], adv_record);
                    j++;
                }
            }
        }
        if (update->add_addresses != NULL) {
            for (i = 0; i < update->add_addresses->num; i++) {
                adv_record_t *rec = update->add_addresses->vec[i];
                if (rec != NULL) {
#ifdef DEBUG_VERBOSE
                    uint8_t *rdp = rec->rdata;
                    if (rec->rrtype == dns_rrtype_aaaa) {
                        SEGMENTED_IPv6_ADDR_GEN_SRP(rdp, rdp_buf);
                        INFO("adding " PRI_SEGMENTED_IPv6_ADDR_SRP "to host " PRI_S_SRP,
                             SEGMENTED_IPv6_ADDR_PARAM_SRP(rdp, rdp_buf), host->registered_name);
                    } else {
                        IPv4_ADDR_GEN_SRP(rdp, rdp_buf);
                        INFO("adding " PRI_IPv4_ADDR_SRP "to host " PRI_S_SRP,
                             IPv4_ADDR_PARAM_SRP(rdp, rdp_buf), host->registered_name);
                    }
#endif
                    addresses->vec[j] = rec;
                    RETAIN_HERE(addresses->vec[j], adv_record);
                    j++;
                    if (rec->update != NULL) {
                        RELEASE_HERE(update->add_addresses->vec[i]->update, adv_update);
                        update->add_addresses->vec[i]->update = NULL;
                    }
                    RELEASE_HERE(update->add_addresses->vec[i], adv_record);
                    update->add_addresses->vec[i] = NULL;
                }
            }
        }
        addresses->num = j;
    }

    // Do the same for instances.
    if (host->instances != NULL) {
        for (i = 0; i < host->instances->num; i++) {
            // We're counting the number of non-NULL instances in the host instance vector, which is probably always
            // going to be the same as host->instances->num, but we are not relying on this.
            if (host->instances->vec[i] != NULL) {
                num_host_instances++;
            }
        }
    }

    if (update->add_instances != NULL) {
        for (i = 0; i < update->add_instances->num; i++) {
            if (update->add_instances->vec[i] != NULL) {
                num_add_instances++;
            }
        }
    }

    num_instances = num_add_instances + num_host_instances;
    instances = adv_instance_vec_create(num_instances);
    if (instances == NULL) {
        if (addresses != NULL) {
            RELEASE_HERE(addresses, adv_record_vec);
            addresses = NULL;
        }
        update_failed(update, dns_rcode_servfail, true, true);
        return;
    }

    j = 0;
    if (host->instances != NULL) {
        for (i = 0; i < host->instances->num; i++) {
            if (j == num_instances) {
                FAULT("j (%d) == num_instances (%d)", j, num_instances);
                break;
            }
            if (update->update_instances != NULL && update->update_instances->vec[i] != NULL) {
                adv_instance_t *instance = update->update_instances->vec[i];
                if (update->remove_instances != NULL && update->remove_instances->vec[i] != NULL) {
                    adv_instance_t *removed_instance = update->remove_instances->vec[i];
                    INFO("removed instance " PRI_S_SRP " " PRI_S_SRP " %d",
                         removed_instance->instance_name, removed_instance->service_type, removed_instance->port);
                    INFO("added instance " PRI_S_SRP " " PRI_S_SRP " %d",
                         instance->instance_name, instance->service_type, instance->port);
                } else {
                    INFO("updated instance " PRI_S_SRP " " PRI_S_SRP " %d",
                         instance->instance_name, instance->service_type, instance->port);
                }
                instances->vec[j] = instance;
                RETAIN_HERE(instances->vec[j], adv_instance);
                j++;
                RELEASE_HERE(update->update_instances->vec[i], adv_instance);
                update->update_instances->vec[i] = NULL;
                if (instance->update != NULL) {
                    RELEASE_HERE(instance->update, adv_update);
                    instance->update = NULL;
                }
                set_instance_message(instance, message);
            } else {
                if (update->remove_instances != NULL && update->remove_instances->vec[i] != NULL) {
                    adv_instance_t *instance = update->remove_instances->vec[i];
                    INFO("removed instance " PRI_S_SRP " " PRI_S_SRP " %d",
                         instance->instance_name, instance->service_type, instance->port);
                    instances->vec[j] = instance;
                    RETAIN_HERE(instances->vec[j], adv_instance);
                    j++;
                    instance->removed = true;
                    if (message != NULL) {
                        if (instance->message != NULL) {
                            ioloop_message_release(instance->message);
                        }
                        instance->message = message;
                        ioloop_message_retain(instance->message);
                    }
                    if (instance->txn == NULL) {
                        ERROR("instance " PRI_S_SRP "." PRI_S_SRP " for host " PRI_S_SRP " has no connection.",
                              instance->instance_name, instance->service_type, host->name);
                    } else {
                        ioloop_dnssd_txn_cancel_srp(host->server_state, instance->txn);
                        ioloop_dnssd_txn_release(instance->txn);
                        instance->txn = NULL;
                    }
                } else {
                    if (host->instances->vec[i] != NULL) {
                        adv_instance_t *instance = host->instances->vec[i];
                        INFO("kept instance " PRI_S_SRP " " PRI_S_SRP " %d, instance->message = %p",
                             instance->instance_name, instance->service_type, instance->port, instance->message);
                        instances->vec[j] = instance;
                        RETAIN_HERE(instances->vec[j], adv_instance);
                        j++;
                        set_instance_message(instance, message);
                    }
                }
            }
        }
    }

    // Set the message on all of the instances that were renewed to the current message.
    if (update->renew_instances != NULL) {
        for (i = 0; i < update->renew_instances->num; i++) {
            adv_instance_t *instance = update->renew_instances->vec[i];
            if (instance != NULL) {
                if (message != NULL) { // Should never not be NULL for a renew, of course.
                    if (instance->message != NULL) {
                        ioloop_message_release(instance->message);
                    }
                    instance->message = message;
                    ioloop_message_retain(instance->message);
                }
                instance->recent_message = 0;
                INFO("renewed instance " PRI_S_SRP " " PRI_S_SRP " %d",
                     instance->instance_name, instance->service_type, instance->port);
            }
        }
    }

    if (update->add_instances != NULL) {
        for (i = 0; i < update->add_instances->num; i++) {
            adv_instance_t *instance = update->add_instances->vec[i];
            if (instance != NULL) {
                INFO("added instance " PRI_S_SRP " " PRI_S_SRP " %d",
                      instance->instance_name, instance->service_type, instance->port);
                instances->vec[j] = instance;
                RETAIN_HERE(instances->vec[j], adv_instance);
                j++;
                RELEASE_HERE(update->add_instances->vec[i], adv_instance);
                update->add_instances->vec[i] = NULL;
                if (instance->update != NULL) {
                    RELEASE_HERE(instance->update, adv_update);
                    instance->update = NULL;
                }
                set_instance_message(instance, message);
            }
        }
    }
    instances->num = j;
    // Clear "skip update" flag on instances.
    for (i = 0; i < instances->num; i++) {
        if (instances->vec[i] != NULL) {
            instances->vec[i]->skip_update = false;
        }
    }

    // At this point we can safely modify the host object because we aren't doing any more
    // allocations.
    if (host->addresses != NULL) {
        RELEASE_HERE(host->addresses, adv_record_vec);
    }
    host->addresses = addresses; // Either NULL or else returned retained by adv_record_vec_create().

    if (host->instances != NULL) {
        for (i = 0; i < host->instances->num; i++) {
            adv_instance_t *instance = host->instances->vec[i];
            if (instance != NULL) {
                INFO("old host instance %d " PRI_S_SRP "." PRI_S_SRP " for host " PRI_S_SRP " has ref_count %d",
                     i, instance->instance_name, instance->service_type, host->name, instance->ref_count);
            } else {
                INFO("old host instance %d is NULL", i);
            }
        }
        RELEASE_HERE(host->instances, adv_instance_vec);
    }
    host->instances = instances;

    if (host->key_record != NULL && update->key != NULL && host->key_record != update->key) {
        srp_mdns_shared_record_remove(host->server_state, host->key_record);
        RELEASE_HERE(host->key_record, adv_record);
        host->key_record = NULL;
    }
    if (host->key_record == NULL && update->key != NULL) {
        host->key_record = update->key;
        RETAIN_HERE(host->key_record, adv_record);
        if (update->key->update != NULL) {
            RELEASE_HERE(update->key->update, adv_update);
            update->key->update = NULL;
        }
    }

    // Remove any instances that are to be removed
    if (update->remove_addresses != NULL) {
        for (i = 0; i < update->remove_addresses->num; i++) {
            adv_record_t *record = update->remove_addresses->vec[i];
            if (record != NULL) {
                srp_mdns_shared_record_remove(host->server_state, record);
            }
        }
    }

    time_t lease_offset = 0;

    if (client) {
        if (host->message != NULL) {
            ioloop_message_release(host->message);
        }
        host->message = client->message;
        ioloop_message_retain(host->message);
        advertise_finished(host, host->name, host->server_state, host->srpl_connection,
                           client->connection, client->message, dns_rcode_noerror, client, true,
                           client->next == NULL);
        remaining_updates = client->next;
        client->next = NULL;
        srp_parse_client_updates_free(client);
        update->client = NULL;
        if (host->message->received_time != 0) {
            host->update_time = host->message->received_time;
            lease_offset = srp_time() - host->update_time;
            INFO("setting host update time based on message received time: %ld, lease offset = %ld",
                 host->update_time, lease_offset);
        } else {
            INFO("setting host update time based on current time: %ld", host->message->received_time);
            host->update_time = srp_time();
        }
    } else {
        INFO("lease offset = %ld", lease_offset);
        lease_offset = srp_time() - host->update_time;
    }
    RETAIN_HERE(update, adv_update); // We need to hold a reference to the update since this might be the last.

    // The update should still be on the host.
    if (host->update == NULL) {
        ERROR("p_update is null.");
    } else {
        RELEASE_HERE(host->update, adv_update);
        host->update = NULL;
    }

    // Reset the retry interval, since we succeeded in updating.
    host->retry_interval = 0;

    // Set the lease time based on this update. Even if we scheduled an update for the next time we
    // enter the dispatch loop, we still want to schedule a lease expiry here, because it's possible
    // that in the process of returning to the dispatch loop, the scheduled update will be removed.
    if (0) {
#if STUB_ROUTER
    } else if (server_state->stub_router_enabled) {
        host->lease_interval = update->host_lease;
        host->key_lease = update->key_lease;
#endif
    } else {
        // For the Thread in Mobile use case, use the duration of the key lease to determine when to expire host
        // entries, rather than expiring them when the host lease expires. This is technically out of spec, but
        // accomplishes part of the stated goal of keeping usable cached data around for use immediately after
        // connecting to a Thread mesh.
        host->lease_interval = update->key_lease;
        host->key_lease = update->key_lease;
    }

    // It would probably be harmless to set this for replications, since the value currently wouldn't change,
    // but to avoid future issues we only set this if it's a direct SRP update and not a replicated update.
    // We know it's a direct SRP update because host->message->lease is zero. It would not be zero if we
    // had received this as an SRP update, but is always zero when received directly via UDP.
    INFO("host->message->lease = %d, host->lease_interval = %d, host->key_lease = %d",
         host->message->lease, host->lease_interval, host->key_lease);
    if (host->message->lease == 0) {
        host->message->lease = host->lease_interval;
        host->message->key_lease = host->key_lease;
    }

    // We want the lease expiry event to fire the next time the lease on any instance expires, or
    // at the time the lease for the current update would expire, whichever is sooner.
    int64_t next_lease_expiry = INT64_MAX;
    int64_t now = ioloop_timenow();

#define LEASE_EXPIRY_DEBUGGING 1
    // update->lease_expiry is nonzero if we are re-doing a previous registration.
    if (update->lease_expiry != 0) {
        if (update->lease_expiry < now) {
#ifdef LEASE_EXPIRY_DEBUGGING
            ERROR("lease expiry for host " PRI_S_SRP " happened %" PRIu64 " milliseconds ago.",
                  host->registered_name, now - update->lease_expiry);
#endif
            // Expire the lease when next we hit the run loop
            next_lease_expiry = now;
        } else {
#ifdef LEASE_EXPIRY_DEBUGGING
            INFO("lease_expiry (1) for host " PRI_S_SRP " set to %" PRId64, host->name,
                 (int64_t)(update->lease_expiry - now));
#endif
            next_lease_expiry = update->lease_expiry;
        }
        host->lease_expiry = update->lease_expiry;
    }
    // This is the more usual case.
    else {
#ifdef LEASE_EXPIRY_DEBUGGING
        INFO("lease_expiry (2) for host " PRI_S_SRP " set to %ld", host->name, (host->lease_interval - lease_offset) * 1000);
#endif
        next_lease_expiry = now + (host->lease_interval - lease_offset) * 1000;
        if (next_lease_expiry < now) {
            next_lease_expiry = now;
        }
        host->lease_expiry = next_lease_expiry;
    }

    // We're doing two things here: setting the lease expiry on instances that were touched by the current
    // update, and also finding the soonest update.
    for (i = 0; i < host->instances->num; i++) {
        adv_instance_t *instance = host->instances->vec[i];

        if (instance != NULL) {
            // This instance was updated by the current update, so set its lease time to
            // next_lease_expiry.
            if (instance->message == message) {
                if (instance->removed) {
#ifdef LEASE_EXPIRY_DEBUGGING
                    INFO("lease_expiry (7) for host " PRI_S_SRP " removed instance " PRI_S_SRP "." PRI_S_SRP
                         " left at %" PRId64, host->name, instance->instance_name, instance->service_type,
                         (int64_t)(instance->lease_expiry - now));
#endif
                } else {
#ifdef LEASE_EXPIRY_DEBUGGING
                    INFO("lease_expiry (4) for host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP " set to %" PRId64,
                         host->name, instance->instance_name, instance->service_type,
                         (int64_t)(host->lease_expiry - now));
#endif
                    instance->lease_expiry = host->lease_expiry;
                }
            }
            // Instance was not updated by this update, so see if it expires sooner than this update
            // (which is likely).
            else if (instance->lease_expiry > now && instance->lease_expiry < next_lease_expiry) {
#ifdef LEASE_EXPIRY_DEBUGGING
                INFO("lease_expiry (3) for host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP " set to %" PRId64,
                     host->name, instance->instance_name, instance->service_type,
                     (int64_t)(instance->lease_expiry - now));
#endif
                next_lease_expiry = instance->lease_expiry;
            } else {
                if (instance->lease_expiry <= now) {
#ifdef LEASE_EXPIRY_DEBUGGING
                    INFO("lease_expiry (5) for host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP
                         " in the past at %" PRId64,
                         host->name, instance->instance_name, instance->service_type,
                         (int64_t)(now - instance->lease_expiry));
#endif
                    next_lease_expiry = now;
#ifdef LEASE_EXPIRY_DEBUGGING
                } else {
                    INFO("lease_expiry (6) for host " PRI_S_SRP " instance " PRI_S_SRP "." PRI_S_SRP
                         " is later than next_lease_expiry by %" PRId64, host->name, instance->instance_name,
                         instance->service_type, (int64_t)(next_lease_expiry - instance->lease_expiry));

#endif
                }
            }
        }
    }

    // Now set a timer for the next lease expiry.
    uint64_t when = next_lease_expiry - now;
    if (when > INT32_MAX) {
        when = INT32_MAX;
    }

    if (next_lease_expiry == now) {
        INFO("scheduling immediate call to lease_callback in the run loop for " PRI_S_SRP, host->name);
        ioloop_run_async(lease_callback, host);
    } else {
        INFO("scheduling wakeup to lease_callback in %" PRIu64 " for host " PRI_S_SRP,
             when / 1000, host->name);
        ioloop_add_wake_event(host->lease_wakeup, host, lease_callback, srp_adv_host_context_release, (uint32_t)when);
        RETAIN_HERE(host, adv_host);
    }

    // Instance vectors can hold circular references to the update object, which won't get freed until we call
    // adv_update_finalize, which we will never do because of the circular reference. So break any remaining
    // circular references before releasing the update.
    adv_update_free_instance_vectors(update);

    // This is letting go of the reference we retained earlier in this function, not some outstanding reference retained elsewhere.
    RELEASE_HERE(update, adv_update);

    // If we were processing an SRP update, we may have additional updates to do. Start the next one now if so.
    if (remaining_updates != NULL) {
        srp_update_start(remaining_updates);
    } else {
        srp_dump_server_stats(server_state, false, false);
    }
}

#ifdef USE_DNSSERVICE_QUEUING
static void
process_dnsservice_error(adv_update_t *update, int err)
{
    if (err != kDNSServiceErr_NoError) {
        update_failed(update, dns_rcode_servfail, true, true);
        if (err == kDNSServiceErr_ServiceNotRunning || err == kDNSServiceErr_DefunctConnection || err == 1) {
            if (err == 1) {
                FAULT("bogus error code 1");
            }
            if (update->host != NULL) {
                if (update->host->server_state != NULL) {
                    service_disconnected(update->host->server_state,
                                         (intptr_t)update->host->server_state->shared_registration_txn);
                }
                wait_retry(update->host);
            }
        }
    }
}
#endif // USE_DNSSERVICE_QUEUING

#define GENERATE_WAKEUP(ptr)                  \
    if ((*ptr) == NULL) {                     \
        (*ptr) = ioloop_wakeup_create();      \
    }                                         \
    if ((*ptr) == NULL) {                     \
        ERROR("unable to make wakeup " #ptr); \
    } else

static void
srp_instance_retry_callback(void *context)
{
    adv_instance_t *instance = context;
    adv_host_t *host = instance->host;
    if (host == NULL || host->removed) {
        INFO("no longer updating instance %p because host is no longer valid.", instance);
        return;
    }
    INFO("re-registering updating instance %p.", instance);
    register_instance(instance);
}

static void
srp_schedule_instance_retry(adv_instance_t *instance)
{
    GENERATE_WAKEUP(&instance->retry_wakeup) {
        if (instance->wakeup_interval == 0) {
            instance->wakeup_interval = 5 * 1000;
        } else {
            instance->wakeup_interval *= 2;
        }
        unsigned interval = instance->wakeup_interval * 1.5 - (srp_random32() % instance->wakeup_interval);
        RETAIN_HERE(instance, adv_instance);
        ioloop_add_wake_event(instance->retry_wakeup, instance, srp_instance_retry_callback, adv_instance_context_release, interval);
        INFO("will attempt to reregister instance %p in %.3lf seconds", instance, ((double)interval) / 1000.0);
    }
}

static void
srp_host_record_retry_callback(void *context)
{
    adv_host_t *host = context;
    if (host != NULL) {
        host->re_register_pending = false;
    }
    if (host == NULL || host->removed) {
        INFO("no longer updating host %p because host is no longer valid.", host);
        return;
    }

    if (host->addresses != NULL) {
        for (int i = 0; i < host->addresses->num; i++) {
            adv_record_t *record = host->addresses->vec[i];
            if (record != NULL) {
                INFO("re-registering host record %p.", record);
                register_host_record(host, record, false);
            }
        }
    }
    if (host->key_record != NULL) {
        INFO("re-registering host record %p.", host->key_record);
        register_host_record(host, host->key_record, false);
    }
}

static void
srp_schedule_host_record_retry(adv_record_t *record)
{
    // If the host isn't valid or we're already re-registering, don't schedule a retry.
    if (record->host == NULL || record->host->removed) {
        INFO("will not attempt to reregister record %p", record);
        return;
    }
    if (record->host->re_register_pending) {
        INFO("already scheduled attempt to reregister record %p", record);
        return;
    }

    adv_host_t *host = record->host;
    GENERATE_WAKEUP(&host->re_register_wakeup) {
        if (host->wakeup_interval == 0) {
            host->wakeup_interval = 5 * 1000;
        } else {
            host->wakeup_interval *= 2;
        }
        unsigned interval = host->wakeup_interval * 1.5 - (srp_random32() % host->wakeup_interval);
        RETAIN_HERE(host, adv_host);
        ioloop_add_wake_event(host->re_register_wakeup, host, srp_host_record_retry_callback, srp_adv_host_context_release, interval);
        INFO("will attempt to reregister record %p in %.3lf seconds", record, ((double)interval) / 1000.0);
    }
}


// When the host registration has completed, we get this callback.   Completion either means that we succeeded in
// registering the record, or that something went wrong and the registration has failed.
static void
register_instance_completion(DNSServiceRef sdref, DNSServiceFlags flags, DNSServiceErrorType error_code,
                             const char *name, const char *regtype, const char *domain, void *context)
{
    (void)flags;
    (void)sdref;
    adv_instance_t *instance = context;
    adv_update_t *update = instance->update;
    adv_host_t *host = instance->host;

    // Retain the instance for the life of this function, just in case we release stuff that is holding the last reference to it.
    RETAIN_HERE(instance, adv_instance);

    // It's possible that we could restart a host update due to an error while a callback is still pending on a stale
    // update.  In this case, we just cancel all of the work that's been done on the stale update (it's probably already
    // moot anyway.
    if (update != NULL && host->update != update) {
        INFO("registration for service " PRI_S_SRP "." PRI_S_SRP " completed with invalid state.", name, regtype);
        RELEASE_HERE(instance->update, adv_update);
        instance->update = NULL;
        RELEASE_HERE(instance, adv_instance);
        return;
    }

    // We will generally get a callback on success or failure of the initial registration; this is what causes
    // the update to complete or fail. We may get subsequent callbacks because of name conflicts. So the first
    // time we get a callback, instance->update will always be valid; thereafter, it will not, so null it out.
    if (update != NULL) {
        RETAIN_HERE(update, adv_update); // We need to hold onto this until we are done with the update.
        RELEASE_HERE(instance->update, adv_update);
        instance->update = NULL;
    }

    if (error_code == kDNSServiceErr_NoError || error_code == kDNSServiceErr_NameConflict) {
        INFO("registration for service " PRI_S_SRP "." PRI_S_SRP "." PRI_S_SRP " -> "
             PRI_S_SRP " has completed" PUB_S_SRP ".", instance->instance_name, instance->service_type, domain,
             host->registered_name, error_code == kDNSServiceErr_NoError ? ":" : " with a conflict");
        INFO("registration is under " PRI_S_SRP "." PRI_S_SRP PRI_S_SRP, name, regtype,
             domain);

        if (error_code != kDNSServiceErr_NoError) {
            if (instance->txn == NULL) {
                FAULT("instance->txn is NULL for instance %p!", instance);
            } else {
                ioloop_dnssd_txn_cancel_srp(host->server_state, instance->txn);
                ioloop_dnssd_txn_release(instance->txn);
                instance->txn = NULL;
            }
            srp_schedule_instance_retry(instance);
        }

        // In principle update->instance should always be non-NULL here because a no-error response should
        // only happen once or not at all. But just to be safe...
        if (update != NULL) {
            if (instance->update_pending) {
                if (update->client != NULL) {
                    instance->recent_message = (ptrdiff_t)update->client->message; // for comparison later in srp_mdns_update_finished
                }
                update->num_instances_completed++;
                if (update->num_records_completed == update->num_records_started &&
                    update->num_instances_completed == update->num_instances_started)
                {
                    srp_mdns_update_finished(update);
                }
                RELEASE_HERE(update, adv_update);
                instance->update_pending = false;
                update = NULL;
            }
        } else {
            INFO("re-update succeeded for instance " PRI_S_SRP " (" PRI_S_SRP
                  " " PRI_S_SRP " " PRI_S_SRP ")", instance->instance_name, name, regtype, domain);
        }
    } else {
        INFO("registration for service " PRI_S_SRP "." PRI_S_SRP "." PRI_S_SRP " -> "
             PRI_S_SRP " failed with code %d", instance->instance_name, instance->service_type, domain,
             host->registered_name, error_code);

        // If the reason this failed is that we couldn't talk to mDNSResponder, or mDNSResponder disconnected, then we want to retry
        // later on in the hopes that mDNSResponder will come back.
        if (error_code == kDNSServiceErr_ServiceNotRunning || error_code == kDNSServiceErr_DefunctConnection) {
            service_disconnected(host->server_state, instance->shared_txn);
            instance->shared_txn = 0;
            wait_retry(host);
        } else {
            if (update != NULL) {
                update_failed(update, (error_code == kDNSServiceErr_NameConflict
                                       ? dns_rcode_yxdomain
                                       : dns_rcode_servfail), true, true);
                if (instance->update != NULL) {
                    RELEASE_HERE(instance->update, adv_update);
                    instance->update = NULL;
                }
                RELEASE_HERE(update, adv_update);
            } else {
            }
        }

        // The transaction still holds a reference to the instance. instance->txn should never be NULL here. When we cancel
        // the transaction, the reference the transaction held on the instance will be released.
        if (instance->txn == NULL) {
            FAULT("instance->txn is NULL for instance %p!", instance);
        } else {
            ioloop_dnssd_txn_cancel_srp(host->server_state, instance->txn);
            ioloop_dnssd_txn_release(instance->txn);
            instance->txn = NULL;
        }
    }
    RELEASE_HERE(instance, adv_instance);
}

static bool
extract_instance_name(char *instance_name, size_t instance_name_max,
                      char *service_type, size_t service_type_max, service_instance_t *instance)
{
    dns_name_t *end_of_service_type = instance->service->rr->name->next;
    size_t service_index;
    service_t *service, *base_type;
    if (end_of_service_type != NULL) {
        if (end_of_service_type->next != NULL) {
            end_of_service_type = end_of_service_type->next;
        }
    }
    dns_name_print_to_limit(instance->service->rr->name, end_of_service_type, service_type, service_type_max);

    // It's possible that the registration might include subtypes. If so, we need to convert them to the
    // format that DNSServiceRegister expects: service_type,subtype,subtype...
    service_index = strlen(service_type);
    base_type = instance->service->base_type;
    for (service = instance->service->next; service != NULL && service->base_type == base_type; service = service->next)
    {
        if (service_index + service->rr->name->len + 2 > service_type_max) {
            ERROR("service name: " PRI_S_SRP " is too long for additional subtype " PRI_S_SRP,
                  service_type, service->rr->name->data);
            return false;
        }
        service_type[service_index++] = ',';
        memcpy(&service_type[service_index], service->rr->name->data, service->rr->name->len + 1);
        service_index += service->rr->name->len;
    }

    // Make a presentation-format version of the service instance name.
    dns_name_print_to_limit(instance->name, instance->name != NULL ? instance->name->next : NULL,
                            instance_name, instance_name_max);
    return true;
}

void
srp_format_time_offset(char *buf, size_t buf_len, time_t offset)
{
    struct tm tm_now;
    time_t when = time(NULL) - offset;
    localtime_r(&when, &tm_now);
    strftime(buf, buf_len, "%F %T", &tm_now);
}

DNSServiceAttributeRef
srp_message_tsr_attribute_generate(message_t *message, uint32_t key_id, char *time_buf, size_t time_buf_size)
{
    DNSServiceAttributeRef attribute = DNSServiceAttributeCreate();
    if (attribute == NULL) {
        ERROR("Failed to create new DNSServiceAttributeRef");
    } else {
        uint32_t offset = 0;

        if (message != NULL && message->received_time != 0) {
            offset = (uint32_t)(srp_time() - message->received_time);
            srp_format_time_offset(time_buf, time_buf_size, offset);
        } else {
            static char msg[] = "now";
            if (time_buf_size < sizeof(msg)) {
                FAULT("bogus time buf size %zd", time_buf_size);
                time_buf[0] = 0;
            } else {
                memcpy(time_buf, msg, sizeof(msg));
            }
        }
        if (_DNSSD_API_AVAILABLE_FALL_2024) {
            DNSServiceAttributeSetHostKeyHash(attribute, key_id);
        }
        DNSServiceAttributeSetTimestamp(attribute, offset);
    }
    return attribute;
}

DNSServiceAttributeRef
srp_adv_instance_tsr_attribute_generate(adv_instance_t *instance, char *time_buf, size_t time_buf_size)
{
    message_t *message = NULL;
    if (instance->update != NULL && instance->update->client != NULL && instance->update->client->message != NULL) {
        message = instance->update->client->message;
    } else if (instance->update == NULL && instance->message != NULL) {
        message = instance->message;
    }
    return srp_message_tsr_attribute_generate(message, instance->host->key_id, time_buf, time_buf_size);
}

static bool
register_instance(adv_instance_t *instance)
{
    int err = kDNSServiceErr_Unknown;
    bool exit_status = false;
    srp_server_t *server_state = instance->host->server_state;

    // If we don't yet have a shared connection, create one.
    if (!srp_mdns_shared_registration_txn_setup(server_state)) {
        goto exit;
    }
    DNSServiceRef service_ref = server_state->shared_registration_txn->sdref;

    INFO(PUB_S_SRP "DNSServiceRegister(%p, " PRI_S_SRP ", " PRI_S_SRP ", " PRI_S_SRP ", %d, %p)",
         instance->skip_update ? "skipping " : "", service_ref, instance->instance_name, instance->service_type,
         instance->host->registered_name, instance->port, instance);

    if (instance->skip_update) {
        if (instance->update->client != NULL) {
            instance->recent_message = (ptrdiff_t)instance->update->client->message; // for comparison later in srp_mdns_update_finished
        }
        exit_status = true;
        goto exit;
    }

    char time_buf[TSR_TIMESTAMP_STRING_LEN];
    DNSServiceAttributeRef tsr_attribute =
        srp_adv_instance_tsr_attribute_generate(instance, time_buf, sizeof(time_buf));
    if (tsr_attribute == NULL) {
        err = kDNSServiceErr_NoMemory;
    } else {
        uint32_t flags = kDNSServiceFlagsShareConnection | kDNSServiceFlagsNoAutoRename;
        if (_DNSSD_API_AVAILABLE_FALL_2024) {
            flags |= kDNSServiceFlagsKnownUnique;
        }
        err = dns_service_register_wa(server_state, &service_ref, flags,
                                      server_state->advertise_interface,
                                      instance->instance_name, instance->service_type, local_suffix,
                                      instance->host->registered_name, htons(instance->port), instance->txt_length,
                                      instance->txt_data, tsr_attribute, register_instance_completion, instance);
        DNSServiceAttributeDeallocate(tsr_attribute);
        if (err == kDNSServiceErr_NoError) {
            INFO("DNSServiceRegister, TSR for instance " PRI_S_SRP " host " PRI_S_SRP " set to " PUB_S_SRP
                 "(instance %p sdref %p)", instance->instance_name,
                 instance->host->name == NULL ? "<null>" : instance->host->name, time_buf, instance, service_ref);
        }
    }

    // This would happen if we pass NULL for regtype, which we don't, or if we run out of memory, or if
    // the server isn't running; in the second two cases, we can always try again later.
    if (err != kDNSServiceErr_NoError) {
        if (err == kDNSServiceErr_ServiceNotRunning || err == kDNSServiceErr_DefunctConnection ||
            err == kDNSServiceErr_BadParam || err == kDNSServiceErr_BadReference || err == 1)
        {
            if (err == 1) {
                FAULT("bogus error code 1");
            }
            INFO("DNSServiceRegister failed: " PUB_S_SRP " (instance %p)",
                 err == kDNSServiceErr_ServiceNotRunning ? "not running" : "defunct", instance);
            service_disconnected(server_state, (intptr_t)server_state->shared_registration_txn);
        } else {
            INFO("DNSServiceRegister failed: %d (instance %p)", err, instance);
        }
        goto exit;
    }
    if (instance->update != NULL) {
        instance->update->num_instances_started++;
        instance->update_pending = true;
    }
    // After DNSServiceRegister succeeds, it creates a copy of DNSServiceRef that indirectly uses the shared connection,
    // so we update it here.
    instance->txn = ioloop_dnssd_txn_add_subordinate(service_ref, instance, adv_instance_context_release, NULL);
    if (instance->txn == NULL) {
        ERROR("no memory for instance transaction.");
        goto exit;
    }
    instance->shared_txn = (intptr_t)server_state->shared_registration_txn;
    RETAIN_HERE(instance, adv_instance); // for the callback
    exit_status = true;

exit:
    return exit_status;
}

// When we get a late name conflict on the hostname, we need to update the host registration and all of the
// service registrations. To do this, we construct an update and then apply it. If there is already an update
// in progress, we put this update at the end of the list.
static void
update_from_host(adv_host_t *host)
{
    adv_update_t *update = NULL;
    int i;

    if (host->update != NULL) {
        ERROR("already have an update.");
    }

    // Allocate the update structure.
    update = calloc(1, sizeof *update);
    if (update == NULL) {
        ERROR("no memory for update.");
        goto fail;
    }
    RETAIN_HERE(update, adv_update);

    if (host->addresses != NULL) {
        update->add_addresses = adv_record_vec_copy(host->addresses);
        if (update->add_addresses == NULL) {
            ERROR("no memory for addresses");
            goto fail;
        }
        for (i = 0; i < update->add_addresses->num; i++) {
            if (update->add_addresses->vec[i] != NULL) {
                update->add_addresses->vec[i]->update = update;
                RETAIN_HERE(update, adv_update);
            }
        }
    }

    // We can never update more instances than currently exist for this host.
    if (host->instances != NULL) {
        update->update_instances = adv_instance_vec_copy(host->instances);
        if (update->update_instances == NULL) {
            ERROR("no memory for update_instances");
            goto fail;
        }
        for (i = 0; i < update->update_instances->num; i++) {
            if (update->update_instances->vec[i] != NULL) {
                update->update_instances->vec[i]->update = update;
                RETAIN_HERE(update, adv_update);
            }
        }

        // We aren't actually adding or deleting any instances, but...
        update->remove_instances = adv_instance_vec_create(host->instances->num);
        if (update->remove_instances == NULL) {
            ERROR("no memory for remove_instances");
            goto fail;
        }
        update->remove_instances->num = host->instances->num;

        update->add_instances = adv_instance_vec_create(host->instances->num);
        if (update->add_instances == NULL) {
            ERROR("no memory for add_instances");
            goto fail;
        }
        update->add_instances->num = host->instances->num;
    }


    // At this point we have figured out all the work we need to do, so hang it off an update structure.
    update->host = host;
    RETAIN_HERE(update->host, adv_host);
    update->host_lease = host->lease_interval;
    update->key_lease = host->key_lease;
    update->lease_expiry = host->lease_expiry;

    // Stash the update on the host.
    host->update = update;
    RETAIN_HERE(host->update, adv_update);  // host gets a reference
    RELEASE_HERE(update, adv_update);       // we're done with our reference.
    start_host_update(host);
    return;

fail:
    if (update != NULL) {
        adv_update_cancel(update);
        RELEASE_HERE(update, adv_update);
    }
    wait_retry(host);
    return;
}

// When the host registration has completed, we get this callback.   Completion either means that we succeeded in
// registering the record, or that something went wrong and the registration has failed.
static void
register_host_record_completion(DNSServiceRef sdref, DNSRecordRef rref,
                                DNSServiceFlags flags, DNSServiceErrorType error_code, void *context)
{
    adv_record_t *record = context;
    adv_host_t *host = NULL;
    adv_update_t *update = NULL;
    (void)sdref;
    (void)rref;
    (void)error_code;
    (void)flags;

    // This can happen if for some reason DNSServiceRemoveRecord returns something other than success. In this case, all
    // the cleanup that can be done has already been done, and all we can do is ignore the problem.
    if (record->rref == NULL) {
        ERROR("null rref");
        return;
    }
    // For analyzer, can't actually happen.
    if (record == NULL) {
        ERROR("null record");
        return;
    }
    host = record->host;
    if (host == NULL) {
        ERROR("no host");
        return;
    }

    // Make sure record remains valid for the duration of this call.
    RETAIN_HERE(record, adv_record);

    // It's possible that we could restart a host update due to an error while a callback is still pending on a stale
    // update.  In this case, we just cancel all of the work that's been done on the stale update (it's probably already
    // moot anyway.
    if (record->update != NULL && host->update != record->update) {
        INFO("registration for host record completed with invalid state.");
        adv_update_cancel(record->update);
        RELEASE_HERE(record->update, adv_update);
        record->update = NULL;
        srp_mdns_shared_record_remove(host->server_state, record); // This will prevent further callbacks and release the reference held by the transaction.
        RELEASE_HERE(record, adv_record); // The callback has a reference to the record.
        RELEASE_HERE(record, adv_record); // Release the reference to the record that we retained at the beginning
        return;

    }
    update = record->update;
    if (update != NULL) {
        RETAIN_HERE(update, adv_update);
    }

    if (error_code == kDNSServiceErr_NoError || error_code == kDNSServiceErr_NameConflict) {
        // If the update is pending, it means that we just finished registering this record for the first time,
        // so we can count it as complete and check to see if there is any work left to do; if not, we call
        // srp_mdns_update_finished to apply the update to the host object.
        const char *note = " has completed.";
        if (record->update_pending) {
            record->update_pending = false;
            if (update != NULL) {
                update->num_records_completed++;
                if (update->num_records_completed == update->num_records_started &&
                    update->num_instances_completed == update->num_instances_started)
                {
                    srp_mdns_update_finished(update);
                }
            }
        } else {
            note = " got spurious success callback after completion.";
        }

        if (error_code != kDNSServiceErr_NoError) {
            // Shared record is no longer good.
            srp_mdns_shared_record_remove(host->server_state, record);
            note = " completed with conflict.";
            srp_schedule_host_record_retry(record);
        }

        if (record->rrtype == dns_rrtype_a) {
            IPv4_ADDR_GEN_SRP(record->rdata, addr_buf);
            INFO("registration for host " PRI_S_SRP " address " PRI_IPv4_ADDR_SRP PUB_S_SRP,
                 host->registered_name, IPv4_ADDR_PARAM_SRP(record->rdata, addr_buf), note);
        } else if (record->rrtype == dns_rrtype_aaaa) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(record->rdata, addr_buf);
            INFO("registration for host " PRI_S_SRP " address " PRI_SEGMENTED_IPv6_ADDR_SRP PUB_S_SRP,
                 host->registered_name, SEGMENTED_IPv6_ADDR_PARAM_SRP(record->rdata, addr_buf), note);
        } else if (record->rrtype == dns_rrtype_key) {
            INFO("registration for host " PRI_S_SRP " key" PUB_S_SRP, host->registered_name, note);
        } else {
            INFO("registration for host " PRI_S_SRP " unknown record type %d " PUB_S_SRP, host->registered_name, record->rrtype, note);
        }
    } else {
        if (record->rrtype == dns_rrtype_a) {
            IPv4_ADDR_GEN_SRP(record->rdata, addr_buf);
            INFO("registration for host " PRI_S_SRP " address " PRI_IPv4_ADDR_SRP " failed, error code = %d.",
             host->registered_name, IPv4_ADDR_PARAM_SRP(record->rdata, addr_buf), error_code);
        } else if (record->rrtype == dns_rrtype_aaaa) {
            SEGMENTED_IPv6_ADDR_GEN_SRP(record->rdata, addr_buf);
            INFO("registration for host " PRI_S_SRP " address " PRI_SEGMENTED_IPv6_ADDR_SRP " failed, error code = %d.",
                 host->registered_name, SEGMENTED_IPv6_ADDR_PARAM_SRP(record->rdata, addr_buf), error_code);
        } else if (record->rrtype == dns_rrtype_key) {
            INFO("registration for host " PRI_S_SRP " key failed, error code = %d.", host->registered_name, error_code);
        } else {
            INFO("registration for host " PRI_S_SRP " unknown record type %d failed, error code = %d.",
                 host->registered_name, record->rrtype, error_code);
        }

        // If the reason this failed is that we couldn't talk to mDNSResponder, or mDNSResponder disconnected, then we want to retry
        // later on in the hopes that mDNSResponder will come back.
        if (error_code == kDNSServiceErr_ServiceNotRunning || error_code == kDNSServiceErr_DefunctConnection) {
            service_disconnected(host->server_state, record->shared_txn);
            if (update != NULL) {
                wait_retry(host);
            }
        } else {
            // The other error we could get is a name conflict. This means that some other advertising proxy or host on
            // the network is advertising the hostname we chose, and either got there first with no TSR record, or got
            // its copy of the host information later than ours. So if we get a name conflict, it's up to the client or
            // the replication peer to make the next move.

            if (update != NULL) {
                update_failed(update, (error_code == kDNSServiceErr_NameConflict
                                       ? dns_rcode_yxdomain
                                       : dns_rcode_servfail), true, true);
            } else {
            }
        }
        // Regardless of what else happens, this transaction is dead, so get rid of our references to it.
        srp_mdns_shared_record_remove(host->server_state, record);
    }
    if (update != NULL) {
        RELEASE_HERE(update, adv_update);
    }
    RELEASE_HERE(record, adv_record); // Release the reference to the record that we retained at the beginning
}

static adv_instance_t *
adv_instance_create(service_instance_t *raw, adv_host_t *host, adv_update_t *update)
{
    char service_type[DNS_MAX_LABEL_SIZE_ESCAPED * 2 + 2]; // sizeof '.' + sizeof '\0'.
    char instance_name[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    uint8_t *txt_data;

    // Allocate the raw registration
    adv_instance_t *instance = calloc(1, sizeof *instance);
    if (instance == NULL) {
        ERROR("adv_instance:create: unable to allocate raw registration struct.");
        return NULL;
    }
    RETAIN_HERE(instance, adv_instance);
    instance->host = host;
    RETAIN_HERE(instance->host, adv_host);
    instance->update = update;
    RETAIN_HERE(instance->update, adv_update);

    // SRV records have priority, weight and port, but DNSServiceRegister only uses port.
    instance->port = (raw->srv == NULL) ? 0 : raw->srv->data.srv.port;

    // Make a presentation-format version of the service name.
    if (!extract_instance_name(instance_name, sizeof instance_name, service_type, sizeof service_type, raw)) {
        RELEASE_HERE(instance, adv_instance);
        return NULL;
    }

    instance->instance_name = strdup(instance_name);
    if (instance->instance_name == NULL) {
        ERROR("adv_instance:create: unable to allocate instance name.");
        RELEASE_HERE(instance, adv_instance);
        return NULL;
    }
    instance->service_type = strdup(service_type);
    if (instance->service_type == NULL) {
        ERROR("adv_instance:create: unable to allocate instance type.");
        RELEASE_HERE(instance, adv_instance);
        return NULL;
    }

    // Allocate the text record buffer
    if (raw->txt != NULL) {
        txt_data = malloc(raw->txt->data.txt.len);
        if (txt_data == NULL) {
            RELEASE_HERE(instance, adv_instance);
            ERROR("adv_instance:create: unable to allocate txt_data buffer");
            return NULL;
        }
        // Format the txt buffer as required by DNSServiceRegister().
        memcpy(txt_data, raw->txt->data.txt.data, raw->txt->data.txt.len);
        instance->txt_data = txt_data;
        instance->txt_length = raw->txt->data.txt.len;
    } else {
        instance->txt_data = NULL;
        instance->txt_length = 0;
    }

    // If the service_instance_t is marked to skip updating, mark the adv_instance_t as well.
    instance->skip_update = raw->skip_update;

    return instance;
}

#define adv_record_create(rrtype, rdlen, rdata, host) \
    adv_record_create_(rrtype, rdlen, rdata, host, __FILE__, __LINE__)
static adv_record_t *
adv_record_create_(uint16_t rrtype, uint16_t rdlen, uint8_t *rdata, adv_host_t *host, const char *file, int line)
{

    adv_record_t *new_record = calloc(1, sizeof(*new_record) + rdlen - 1);
    if (new_record == NULL) {
        ERROR("no memory for new_record");
        return NULL;
    }
    new_record->rdata = malloc(rdlen);
    if (new_record->rdata == NULL) {
        ERROR("no memory for new_record->rdata");
        free(new_record);
        return NULL;
    }
    new_record->host = host;
    RETAIN(host, adv_host);
    new_record->rrtype = rrtype;
    new_record->rdlen = rdlen;
    memcpy(new_record->rdata, rdata, rdlen);
    RETAIN(new_record, adv_record);
    return new_record;
}

// Given a pair of service types which may or may not have subtypes, e.g. _foo._tcp, which doesn't have subtypes, or
// _foo.tcp,bar, which does, return true if type1 matches the type2 for the base type, ignoring subtypes.
static bool
service_types_equal(const char *type1, const char *type2)
{
    size_t len1;
    char *comma1 = strchr(type1, ',');
    if (comma1 == NULL) {
        len1 = strlen(type1);
    } else {
        len1 = comma1 - type1;
    }
    char *comma2 = strchr(type2, ',');
    size_t len2;
    if (comma2 != NULL) {
        len2 = comma2 - type2;
    } else {
        len2 = strlen(type2);
    }
    if (len1 != len2) {
        return false;
    }
    if (memcmp(type2, type1, len1)) {
        return false;
    }
    return true;
}

DNSServiceAttributeRef
srp_adv_host_tsr_attribute_generate(adv_host_t *host, char *time_buf, size_t time_buf_size)
{
    message_t *message = NULL;
    if (host->update != NULL && host->update->client != NULL && host->update->client->message != NULL) {
        message = host->update->client->message;
    } else if (host->update == NULL && host->message != NULL) {
        message = host->message;
    }
    return srp_message_tsr_attribute_generate(message, host->key_id, time_buf, time_buf_size);
}

static bool
register_host_record(adv_host_t *host, adv_record_t *record, bool skipping)
{
    int err;

    // If this record is already registered, get rid of the old transaction.
    if (record->rref != NULL && !skipping) {
        srp_mdns_shared_record_remove(host->server_state, record);
    }

    // If we don't yet have a shared connection, create one.
    if (!srp_mdns_shared_registration_txn_setup(host->server_state)) {
        return false;
    }

    const DNSServiceRef service_ref = host->server_state->shared_registration_txn->sdref;

    if (record->rrtype == dns_rrtype_a) {
        IPv4_ADDR_GEN_SRP(record->rdata, rdata_buf);
        INFO(PUB_S_SRP "DNSServiceRegisterRecord(%p %p %d %d %s %d %d %d " PRI_IPv4_ADDR_SRP " %d %p %p)",
             skipping ? "skipping " : "", service_ref, &record->rref, kDNSServiceFlagsShared,
             host->server_state->advertise_interface, host->registered_name, record->rrtype, dns_qclass_in,
             record->rdlen, IPv4_ADDR_PARAM_SRP(record->rdata, rdata_buf),
             ADDRESS_RECORD_TTL, register_host_record_completion, record);
    } else if (record->rrtype == dns_rrtype_aaaa) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(record->rdata, rdata_buf);
        INFO(PUB_S_SRP "DNSServiceRegisterRecord(%p %p %d %d %s %d %d %d " PRI_SEGMENTED_IPv6_ADDR_SRP " %d %p %p)",
             skipping ? "skipping " : "", service_ref, &record->rref, kDNSServiceFlagsShared,
             host->server_state->advertise_interface, host->registered_name, record->rrtype, dns_qclass_in,
             record->rdlen, SEGMENTED_IPv6_ADDR_PARAM_SRP(record->rdata, rdata_buf),
             ADDRESS_RECORD_TTL, register_host_record_completion, record);
    } else {
        INFO(PUB_S_SRP "DNSServiceRegisterRecord(%p %p %d %d %s %d %d %d %p %d %p %p)",
             skipping ? "skipping " : "", service_ref, &record->rref,
             kDNSServiceFlagsShared,
             host->server_state->advertise_interface, host->registered_name,
             record->rrtype, dns_qclass_in, record->rdlen, record->rdata, ADDRESS_RECORD_TTL,
             register_host_record_completion, record);
    }
    // If we're skipping, we don't actually have to do any work.
    if (skipping) {
        return true;
    }

    char time_buf[TSR_TIMESTAMP_STRING_LEN];
    DNSServiceAttributeRef tsr_attribute =
        srp_adv_host_tsr_attribute_generate(host, time_buf, sizeof(time_buf));
    if (tsr_attribute == NULL) {
        ERROR("Failed to create new DNSServiceAttributeRef");
        return false;
    } else {
        err = dns_service_register_record_wa(host->server_state, service_ref, &record->rref,
                                             kDNSServiceFlagsKnownUnique,
                                             host->server_state->advertise_interface, host->registered_name,
                                             record->rrtype, dns_qclass_in, record->rdlen, record->rdata,
                                             ADDRESS_RECORD_TTL, tsr_attribute, register_host_record_completion,
                                             record);
        DNSServiceAttributeDeallocate(tsr_attribute);
        if (err == kDNSServiceErr_NoError) {
            INFO("DNSServiceRegisterRecord for " PRI_S_SRP ", TSR set to " PUB_S_SRP " (record %p rref %p)",
                 host->name, time_buf, record, record->rref);
        }
    }

    if (err != kDNSServiceErr_NoError) {
        if (err == kDNSServiceErr_ServiceNotRunning || err == kDNSServiceErr_DefunctConnection ||
            err == kDNSServiceErr_BadParam || err == kDNSServiceErr_BadReference || err == 1)
        {
            if (err == 1) { // This is for an old bug that probably doesn't happen anymore.
                FAULT("bogus error code 1");
            }
            INFO("DNSServiceRegisterRecord failed on host " PUB_S_SRP ": " PUB_S_SRP " (record %p)", host->name,
                 err == kDNSServiceErr_ServiceNotRunning ? "not running" : "defunct", record);
            service_disconnected(host->server_state, (intptr_t)host->server_state->shared_registration_txn);
        } else {
            INFO("DNSServiceRegisterRecord failed: %d (record %p)", err, record);
        }
        return false;
    }
    record->shared_txn = (intptr_t)host->server_state->shared_registration_txn;
    RETAIN_HERE(record, adv_record); // for the callback
    if (host->update != NULL) {
        record->update_pending = true;
    }
    return true;
}

static bool
update_instance_tsr(adv_instance_t *instance, adv_instance_t *new_instance)
{
    int err = kDNSServiceErr_NoError;
    bool success = false;

    if (instance->txn == NULL) {
        ERROR("txn is NULL updating instance TSR.");
        goto out;
    }
    if (instance->txn->sdref == NULL) {
        ERROR("sdref is NULL when updating instance TSR.");
        goto out;
    }
    // Currently if we want to update the rdata, we need to do that separately from the TSR.
    if (new_instance != NULL) {
        if (instance->skip_update) {
            err = kDNSServiceErr_NoError;
        } else {
            err = dns_service_update_record(instance->host->server_state, instance->txn->sdref,
                                            NULL, 0, new_instance->txt_length, new_instance->txt_data, 0);
        }
        if (err != kDNSServiceErr_NoError) {
            INFO("DNSServiceUpdateRecord for instance " PRI_S_SRP " TXT record failed: %d (instance %p)",
                 instance->instance_name, err, instance);
            goto out;
        } else {
            INFO("updated TXT record for " PRI_S_SRP " . " PRI_S_SRP " (instance %p sdref %p).",
                 instance->instance_name, instance->service_type, instance, instance->txn->sdref);
            success = true;
        }
    }

    DNSServiceAttributeRef attr;

    if (instance->skip_update) {
        INFO("skipping DNSServiceUpdateRecord for instance " PRI_S_SRP " TSR (instance %p)",
             instance->instance_name, instance);
    } else {
        success = false;
        attr = DNSServiceAttributeCreate();
        if (attr == NULL) {
            ERROR("failed to create new DNSServiceAttributeRef");
        } else {
            uint32_t offset = 0;
            char time_buf[TSR_TIMESTAMP_STRING_LEN];
            if (instance->update != NULL && instance->update->client != NULL && instance->update->client->message != NULL &&
                instance->update->client->message->received_time != 0)
            {
                offset = (uint32_t)(srp_time() - instance->update->client->message->received_time);
                srp_format_time_offset(time_buf, sizeof(time_buf), offset);
            } else {
                static char msg[] = "now";
                memcpy(time_buf, msg, sizeof(msg));
            }
            if (_DNSSD_API_AVAILABLE_FALL_2024) {
                DNSServiceAttributeSetHostKeyHash(attr, instance->host->key_id);
            }
            DNSServiceAttributeSetTimestamp(attr, offset);
            err = dns_service_update_record_wa(instance->host->server_state,
                                               instance->txn->sdref, NULL, 0, 0, NULL, 0, attr);
            DNSServiceAttributeDeallocate(attr);
            if (err == kDNSServiceErr_NoError) {
                INFO("DNSServiceUpdateRecord for " PRI_S_SRP ", TSR set to " PUB_S_SRP " (instance %p sdref %p)",
                     instance->host == NULL ? "<null>" : instance->host->name, time_buf, instance, instance->txn->sdref);
                success = true;
            } else {
                INFO("DNSServiceUpdateRecord for instance " PRI_S_SRP ", TSR failed: %d (instance %p sdref %p)",
                     instance->instance_name, err, instance, instance->txn->sdref);
            }
        }
    }

out:
    if (success == false) {
        if (instance->txn != NULL) {
            // We should never get a bad reference error.
            if (err == kDNSServiceErr_BadReference || err == kDNSServiceErr_BadParam) {
                FAULT("we got a bad reference error: why?");
            }
            // For all errors, we should cancel and release the transaction.
            ioloop_dnssd_txn_cancel_srp(instance->host->server_state, instance->txn);
            ioloop_dnssd_txn_release(instance->txn);
            instance->txn = NULL;
        }
    } else if (new_instance != NULL) {
        // If we have new_instance, the caller is going to get rid of it, so we need to
        // steal the (possibly changed) data from it and put it on instance.
        free(instance->txt_data);
        instance->txt_data = new_instance->txt_data;
        instance->txt_length = new_instance->txt_length;
        new_instance->txt_data = NULL;
        new_instance->txt_length = 0;
    }
    return success;
}

static void
update_host_tsr(adv_record_t *record, adv_update_t *update)
{
    DNSServiceAttributeRef attr;
    int err;
    dnssd_txn_t *shared_txn;

    if (record->host == NULL || record->rref == NULL) {
        ERROR("record->host[%p], record->rref[%p] when we update host TSR.", record->host, record->rref);
        return;
    }

    shared_txn = record->host->server_state->shared_registration_txn;
    if (shared_txn == NULL) {
        ERROR("shared_txn is NULL when we update host TSR.");
        return;
    }
    if (shared_txn->sdref == NULL) {
        ERROR("shared_txn->sdref is NULL when we update host TSR.");
        return;
    }

    attr = DNSServiceAttributeCreate();
    if (attr == NULL) {
        ERROR("failed to create new DNSServiceAttributeRef");
    } else {
        uint32_t offset = 0;
        char time_buf[TSR_TIMESTAMP_STRING_LEN];
        if (update->client != NULL && update->client->message != NULL && update->client->message->received_time != 0) {
            offset = (uint32_t)(srp_time() - update->client->message->received_time);
            srp_format_time_offset(time_buf, sizeof(time_buf), offset);
        } else {
            static char msg[] = "now";
            memcpy(time_buf, msg, sizeof(msg));
        }
        if (_DNSSD_API_AVAILABLE_FALL_2024) {
            DNSServiceAttributeSetHostKeyHash(attr, record->host->key_id);
        }
        DNSServiceAttributeSetTimestamp(attr, offset);
        err = dns_service_update_record_wa(record->host->server_state,
                                           shared_txn->sdref, record->rref, 0, 0, NULL, 0, attr);
        DNSServiceAttributeDeallocate(attr);
        if (err == kDNSServiceErr_NoError) {
            INFO("DNSServiceUpdateRecord TSR for " PRI_S_SRP " set to " PUB_S_SRP " (record %p rref %p)",
                 record->host == NULL ? "<null>" : record->host->name, time_buf, record, record->rref);
        } else {
            INFO("DNSServiceUpdateRecordWithAttribute for host tsr failed: %d (record %p rref %p)",
                 err, record, record->rref);
        }
    }
}

// When we need to register a host with mDNSResponder, start_host_update is called.   This can be either because
// we just got a new registration for a host, or if the daemon dies and we need to re-do the host registration.
// This just registers the host; if that succeeds, then we register the service instances.
static void
start_host_update(adv_host_t *host)
{
    adv_update_t *update = host->update;
#ifdef USE_DNSSERVICE_QUEUING
    int err;
#endif
    int i;

    // No work to do?
    if (update == NULL) {
        ERROR("start_host_update: no work to do for host " PRI_S_SRP, host->registered_name);
        return;
    }

    bool skip_host_updates = (update->client != NULL && update->client->skip_host_updates);


    update->num_records_started = 0;

    // Add all of the addresses that have been registered.
    if (update->add_addresses != NULL) {
        for (i = 0; i < update->add_addresses->num; i++) {
            if (update->add_addresses->vec[i] != NULL) {
                if (!register_host_record(host, update->add_addresses->vec[i], skip_host_updates)) {
                    update_failed(update, dns_rcode_servfail, true, true);
                    return;
                } else if (!skip_host_updates) {
                    update->num_records_started++;
                }
            }
        }
    }

    // It's possible that some existing addresses are no longer registered because of a service disconnect. Check all the
    // existing addresses for this situation: if an existing address has no rref, and does not appear in update->remove_addrs,
    // then re-register it.
    if (host->addresses != NULL) {
        for (i = 0; i < host->addresses->num; i++) {
            adv_record_t *record = host->addresses->vec[i];
            adv_record_t *remove_address = NULL;
            if (update->remove_addresses != NULL) {
                remove_address = update->remove_addresses->vec[i];
            }
            if (remove_address == NULL && record != NULL && record->rref == NULL) {
                host->addresses->vec[i]->update = update;
                RETAIN_HERE(host->addresses->vec[i]->update, adv_update);
                if (!register_host_record(host, record, skip_host_updates)) {
                    update_failed(update, dns_rcode_servfail, true, true);
                    return;
                } else if (!skip_host_updates) {
                    update->num_records_started++;
                }
            }
        }
    }

    if (update->key != NULL) {
        if (!register_host_record(host, update->key, skip_host_updates)) {
            update_failed(update, dns_rcode_servfail, true, true);
            return;
        } else if (!skip_host_updates) {
            update->num_records_started++;
        }
    }

    // If the shared transaction has changed since the key record was added, add it again.
    if (update->key == NULL && host->key_record != NULL &&
        (host->key_record->shared_txn != (intptr_t)host->server_state->shared_registration_txn ||
         host->key_record->rref == NULL))
    {
        update->key = host->key_record;
        RETAIN_HERE(update->key, adv_record);
        RELEASE_HERE(host->key_record, adv_record);
        host->key_record = NULL;
        update->key->rref = NULL;
        update->key->update = update;
        RETAIN_HERE(update, adv_update);
        if (!register_host_record(host, update->key, skip_host_updates)) {
            update_failed(update, dns_rcode_servfail, true, true);
            return;
        } else if (!skip_host_updates) {
            update->num_records_started++;
        }
    }

    if (update->num_records_started == 0) {
        adv_record_t *record = update->key != NULL ? update->key : (host->key_record != NULL ? host->key_record : NULL);
        if (record == NULL) {
        } else {
            if (record->rref == NULL) {
                if (!register_host_record(host, record, skip_host_updates)) {
                    update_failed(update, dns_rcode_servfail, true, true);
                    return;
                } else if (!skip_host_updates) {
                    update->num_records_started++;
                }
            } else if (!skip_host_updates) {
                update_host_tsr(record, update);
            }
        }
    }

    if (host->instances != NULL) {
        // For each service instance that's being added, register it.
        if (update->add_instances != NULL) {
            for (i = 0; i < update->add_instances->num; i++) {
                if (update->add_instances->vec[i] != NULL) {
                    if (!register_instance(update->add_instances->vec[i])) {
                        update_failed(update, dns_rcode_servfail, true, true);
                        return;
                    }
                }
            }
        }

        // For each service instance that's being renewed, update its TSR if the original registration still exist,
        // Otherwise re-register the instance.
        if (update->renew_instances != NULL) {
            for (i = 0; i < update->renew_instances->num; i++) {
                if (update->renew_instances->vec[i] != NULL) {
                    adv_instance_t *instance = update->renew_instances->vec[i];
                    bool must_update = true;
                    bool renew_failed = instance->txn != NULL;
                    if (instance->txn != NULL) {
                        bool must_remove = false;
                        // Make sure the instance is still registered and is registered on the current shared connection.
                        if (instance->txn->sdref != NULL) {
                            if (((intptr_t)host->server_state->shared_registration_txn == instance->shared_txn)) {
                                if (update_instance_tsr(instance, NULL)) {
                                    must_remove = false;
                                    must_update = false;
                                    instance->recent_message = (ptrdiff_t)update->client->message;
                                } else {
                                    INFO("instance " PRI_S_SRP " (%p) tsr update failed, re-registering",
                                         instance->instance_name, instance);
                                    must_remove = true;
                                }
                            } else {
                                // If the shared transaction has changed, then the registration no longer exists, and
                                // the sdref is no longer valid.
                                INFO("instance " PRI_S_SRP " (%p) shared connection (%" PRIxPTR ") is stale, re-registering",
                                     instance->instance_name, instance, instance->shared_txn);
                                instance->txn->sdref = NULL;
                                must_remove = true;
                                must_update = true;
                                renew_failed = false;
                            }
                        }
                        if (must_remove) {
                            // If not, dispose of the transaction and re-register.
                            if (instance->txn != NULL) {
                                ioloop_dnssd_txn_cancel_srp(host->server_state, instance->txn);
                                ioloop_dnssd_txn_release(instance->txn);
                                instance->txn = NULL;
                            }
                        }
                    }
                    if (must_update) {
                        if (renew_failed) {
                            INFO(PRI_S_SRP " (%p): failed to update TSR, re-registering", instance->instance_name, instance);
                        }
                        if (!register_instance(update->renew_instances->vec[i])) {
                            update_failed(update, dns_rcode_servfail, true, true);
                            return;
                        }
                    }
                }
            }
        }

        // Sanity check that the instance vector sizes match between host and update.
        if (update->update_instances != NULL && update->update_instances->num != host->instances->num) {
            FAULT("update instance count %d differs from host instance count %d",
                  update->update_instances->num, host->instances->num);
            update_failed(update, dns_rcode_servfail, true, true);
            return;
        }
        if (update->remove_instances != NULL && update->remove_instances->num != host->instances->num) {
            FAULT("delete instance count %d differs from host instance count %d",
                  update->remove_instances->num, host->instances->num);
            update_failed(update, dns_rcode_servfail, true, true);
            return;
        }
        for (i = 0; i < host->instances->num; i++) {
            adv_instance_t *update_instance = update->update_instances->vec[i];
            if (update_instance != NULL && !update_instance->removed) {
                adv_instance_t *host_instance = host->instances->vec[i];
                bool must_register = true;
                // Check to see if just the TXT record changes; in this case use DNSServiceUpdateRecord rather than re-registering
                // the instance. If we can't update, we have to remove and then add. We could do this as a pair of atomic transactions
                // if we used DNSServiceRegisterRecord rather than DNSServiceRegister, but currently we don't do that.
                // Of course if the previous registration is no longer valid, re-register.
                if (host_instance->txn != NULL && host_instance->txn->sdref != NULL && host->server_state != NULL &&
                    ((intptr_t)host->server_state->shared_registration_txn == host_instance->shared_txn))
                {
                    if (update_instance->port == host_instance->port &&
                        update_instance->txt_length != 0 &&
                        memcmp(update_instance->txt_data, host_instance->txt_data, update_instance->txt_length))
                    {
                        // If we are able to update the TXT record using DNSServiceUpdateRecord, we don't actually need
                        // this update instance.
                        if (update_instance_tsr(host_instance, update_instance)) {
                            host_instance->recent_message = (ptrdiff_t)update->client->message;
                            RELEASE_HERE(update->update_instances->vec[i], adv_instance);
                            update_instance = NULL;
                            update->update_instances->vec[i] = NULL;
                            must_register = false;
                        }
                    }
                }
                if (must_register) {
                    if (host_instance->txn != NULL) {
                        ioloop_dnssd_txn_cancel_srp(host->server_state, host->instances->vec[i]->txn);
                        ioloop_dnssd_txn_release(host->instances->vec[i]->txn);
                        host->instances->vec[i]->txn = NULL;
                    }

                    if (!register_instance(update->update_instances->vec[i])) {
                        INFO("register instance failed.");
                        update_failed(update, dns_rcode_servfail, true, true);
                        return;
                    }
                }
            }
        }
    }

    if (update->num_instances_started == 0 && update->num_records_started == 0) {
        INFO("no service or record updates, so we're finished.");
        srp_mdns_update_finished(update);
        return;
    }

}

// When a host has no update in progress, and there is a client update ready to process, we need to analyze
// the client update to see what work needs to be done.  This work is constructed as an translation from the
// raw update sent by the client (host->clients) into a prepared update that can be used directly to
// register the information with mDNSResponder.
//
// Normally a host will only have one prepared update in progress; however, if we lose our connection to
// mDNSResponder, then we need to re-create the host advertisement.  If there was an update in progress when
// this happened, we then need to reapply that as well.  In this case an update is constructed from the host, to
// get the host into the intended state, and the in-progress update is pushed below that; when the host has
// been re-created on the daemon, the pending update is popped back off the stack and restarted.
static void
prepare_update(adv_host_t *host, client_update_t *client_update)
{
    host_addr_t *addr;
    int i, j;
    service_instance_t *instance;
    adv_record_vec_t *remove_addrs = NULL;
    int num_remove_addrs = 0;
    adv_record_vec_t *add_addrs = NULL;
    int num_add_addrs = 0;
    int num_update_instances = 0;
    int num_add_instances = 0;
    int num_remove_instances = 0;
    int num_renew_instances = 0;
    adv_instance_vec_t *update_instances = NULL, *add_instances = NULL;
    adv_instance_vec_t *remove_instances = NULL, *renew_instances = NULL;
    adv_update_t *update = NULL;

    // Work to do:
    // - Figure out what address records to add and what address records to delete.
    // - Because we can only have one address record at a time currently, figure out which address record we want
    // - If we already have an address record published, and it's the same, do nothing
    //   - else if we already have an address record published, and it's changed to a different address, do an update
    //   - else if we have a new address record, publish it
    //   - else publish the key to hold the name
    // - Go through the set of service instances, identifying deletes, changes and adds
    //   - We don't currently allow deletes, but what that would look like would be an instance with no SRV or TXT
    //     record.
    //   - What about a delete that keeps the name but un-advertises the service?   How would we indicate that?
    //     Maybe if there's no service PTR for the service?
    //   - Changes means that the contents of the text record changed, or the contents of the SRV record
    //     changed (but not the hostname) or both.
    //   - New means that we don't have a service with that service instance name on the host (and we previously
    //     eliminated the possibility that it exists on some other host).

    // Allocate the update structure.
    update = calloc(1, sizeof *update);
    if (update == NULL) {
        ERROR("no memory for update.");
        goto fail;
    }
    RETAIN_HERE(update, adv_update); // For the lifetime of this function

    if (host->re_register_wakeup != NULL) {
        ioloop_cancel_wake_event(host->re_register_wakeup);
    }
    host->re_register_pending = false;
    update->start_time = srp_time();

    // The maximum number of addresses we could be deleting is all the ones the host currently has.
    if (host->addresses == NULL || host->addresses->num == 0) {
        num_remove_addrs = 0;
        remove_addrs = NULL;
    } else {
        num_remove_addrs = host->addresses->num;
        if (num_remove_addrs != 0) {
            remove_addrs = adv_record_vec_create(num_remove_addrs);
            // If we can't allocate space, just wait a bit.
            if (remove_addrs == NULL) {
                ERROR("no memory for remove_addrs");
                goto fail;
            }
            remove_addrs->num = num_remove_addrs;
        }
    }

    num_add_addrs = 0;
    for (addr = client_update->host->addrs; addr != NULL; addr = addr->next) {
        num_add_addrs++;
    }
    add_addrs = adv_record_vec_create(num_add_addrs);
    if (add_addrs == NULL) {
        ERROR("no memory for add_addrs");
        goto fail;
    }

    // Copy all of the addresses in the update into add_addresses
    num_add_addrs = 0;
    for (addr = client_update->host->addrs; addr; addr = addr->next) {
        bool add = true;
        for (i = 0; i < num_add_addrs; i++) {
            // If the client sends duplicate addresses, only add one of them.
            if (add_addrs->vec[i] != NULL &&
                add_addrs->vec[i]->rrtype == addr->rr.type &&
                add_addrs->vec[i]->rdlen == (addr->rr.type == dns_rrtype_a ? 4 : 16) &&
                !memcmp(add_addrs->vec[i]->rdata, (uint8_t *)&addr->rr.data, add_addrs->vec[i]->rdlen))
            {
                add = false;
            }
        }
        if (add) {
            adv_record_t *prepared_address = adv_record_create(addr->rr.type, addr->rr.type == dns_rrtype_a ? 4 : 16,
                                                               (uint8_t *)&addr->rr.data, host);
            if (prepared_address == NULL) {
                ERROR("No memory for prepared address");
                goto fail;
            }
            add_addrs->vec[num_add_addrs++] = prepared_address;
        }
    }
    add_addrs->num = num_add_addrs;
    for (i = 0; i < add_addrs->num; i++) {
        if (add_addrs->vec[i] != NULL) {
            add_addrs->vec[i]->update = update;
            RETAIN_HERE(add_addrs->vec[i]->update, adv_update);
        }
    }

#ifdef DEBUG_HOST_RECORDS_VERBOSE
    for (i = 0; i < 2; i++) {
        for (j = 0; j < (i ? num_add_addrs : num_remove_addrs); j++) {
            adv_record_t *address = i ? add_addrs->vec[j] : (host->addresses != NULL ? host->addresses->vec[j] : NULL);
            if (address == NULL) {
                INFO(PUB_S_SRP " before: %d NULL", i ? "add" : "rmv", j);
            } else {
                char foobuf[385], *foop = foobuf;
                for (unsigned long k = 0; k < address->rdlen && k * 3 + 1 < sizeof(foobuf); k++) {
                    snprintf(foop, 4, k ? ":%02x" : "%02x", address->rdata[k]);
                    foop += k ? 3 : 2;
                }
                *foop = 0;
                INFO(PUB_S_SRP " before: %d rrtype %d rdlen %d rdata " PRI_S_SRP, i ? "add" : "rmv", j,
                     address->rrtype, address->rdlen, foobuf);
            }
        }
    }
#endif // DEBUG_HOST_RECORDS_VERBOSE

    // For every host address, see if it's in add_addresses.   If it's not, it needs to be removed.
    // If it is, it doesn't need to be added.
    if (num_remove_addrs != 0) {
        for (i = 0; i < num_remove_addrs; i++) {
            if (host->addresses != NULL && host->addresses->vec[i] != NULL) {
                remove_addrs->vec[i] = host->addresses->vec[i];
                RETAIN_HERE(remove_addrs->vec[i], adv_record);
            }
            for (j = 0; j < num_add_addrs; j++) {
                // If the address is present in both places, and has a valid registration, remove it from the list of
                // addresses to add, and also remove it from the list of addresses to remove.  When we're done, all that
                // will be remaining in the list to remove will be addresses that weren't present in the add list.
                if (remove_addrs->vec[i] != NULL && add_addrs->vec[j] != NULL &&
                    remove_addrs->vec[i]->rref != NULL && host->server_state != NULL &&
                    (intptr_t)host->server_state->shared_registration_txn == remove_addrs->vec[i]->shared_txn &&
                    add_addrs->vec[j]->rrtype == remove_addrs->vec[i]->rrtype &&
                    add_addrs->vec[j]->rdlen == remove_addrs->vec[i]->rdlen &&
                    !memcmp(add_addrs->vec[j]->rdata, remove_addrs->vec[i]->rdata, remove_addrs->vec[i]->rdlen))
                {
                    RELEASE_HERE(remove_addrs->vec[i], adv_record);
                    remove_addrs->vec[i] = NULL;
                    RELEASE_HERE(add_addrs->vec[j], adv_record);
                    add_addrs->vec[j] = NULL;
                }
            }
        }
        remove_addrs->num = num_remove_addrs;
    }

#ifdef DEBUG_HOST_RECORDS_VERBOSE
    for (i = 0; i < 2; i++) {
        for (j = 0; j < (i ? num_add_addrs : num_remove_addrs); j++) {
            adv_record_t *address = i ? add_addrs->vec[j] : (remove_addrs != NULL ? remove_addrs->vec[j] : NULL);
            if (address == NULL) {
                INFO(PUB_S_SRP "  after: %d NULL", i ? "add" : "rmv", j);
            } else {
                char foobuf[385], *foop = foobuf;
                for (unsigned long k = 0; k < address->rdlen && k * 3 + 1 < sizeof(foobuf); k++) {
                    snprintf(foop, 4, k ? ":%02x" : "%02x", address->rdata[k]);
                    foop += k ? 3 : 2;
                }
                *foop = 0;
                INFO(PUB_S_SRP "  after: %d rrtype %d rdlen %d rdata " PRI_S_SRP, i ? "add" : "rmv", j,
                     address->rrtype, address->rdlen, foobuf);
            }
        }
    }
#endif // DEBUG_HOST_RECORDS_VERBOSE

    // Make a key record
    if (host->key_record == NULL) {
        update->key = adv_record_create(dns_rrtype_key, host->key_rdlen, host->key_rdata, host);
        if (update->key == NULL) {
            ERROR("no memory for key record");
            goto fail;
        }
        update->key->update = update;
        RETAIN_HERE(update->key->update, adv_update);
    }

    // We can never update more instances than currently exist for this host.
    num_update_instances = host->instances->num;
    num_remove_instances = host->instances->num;
    num_renew_instances = host->instances->num;

    update_instances = adv_instance_vec_create(num_update_instances);
    if (update_instances == NULL) {
        ERROR("no memory for update_instances");
        goto fail;
    }
    update_instances->num = num_update_instances;

    remove_instances = adv_instance_vec_create(num_remove_instances);
    if (remove_instances == NULL) {
        ERROR("no memory for remove_instances");
        goto fail;
    }
    remove_instances->num = num_remove_instances;

    renew_instances = adv_instance_vec_create(num_renew_instances);
    if (renew_instances == NULL) {
        ERROR("no memory for renew_instances");
        goto fail;
    }
    renew_instances->num = num_renew_instances;

    // Handle removes. Service instance removes have to remove the whole service instance, not some subset.
    for (delete_t *dp = client_update->removes; dp; dp = dp->next) {
        // Removes can be for services or service instances. Because we're acting as an
        // Advertising Proxy and not a regular name server, we don't track service instances,
        // and so we don't need to match them here. This if statement checks to see if the
        // name could possibly be a service instance name followed by a service type. We
        // can then extract the putative service instance name and service type and compare;
        // if they match, they are in fact those things, and if they don't, we don't care.
        if (dp->name != NULL && dp->name->next != NULL && dp->name->next->next != NULL) {
            char instance_name[DNS_MAX_LABEL_SIZE_ESCAPED + 1];
            char service_type[DNS_MAX_LABEL_SIZE_ESCAPED + 2];

            dns_name_print_to_limit(dp->name, dp->name->next, instance_name, sizeof(instance_name));
            dns_name_print_to_limit(dp->name->next, dp->name->next->next->next, service_type, sizeof(service_type));

            for (i = 0; i < host->instances->num; i++) {
                adv_instance_t *remove_instance = host->instances->vec[i];
                if (remove_instance != NULL) {
                    if (!strcmp(instance_name, remove_instance->instance_name) &&
                        service_types_equal(service_type, remove_instance->service_type))
                    {
                        remove_instances->vec[i] = remove_instance;
                        RETAIN_HERE(remove_instances->vec[i], adv_instance);
                        break;
                    }
                }
            }
        }
    }

    // The number of instances to add can be as many as there are instances in the update.
    num_add_instances = 0;
    for (instance = client_update->instances; instance; instance = instance->next) {
        num_add_instances++;
    }
    add_instances = adv_instance_vec_create(num_add_instances);
    if (add_instances == NULL) {
        ERROR("prepare_update: no memory for add_instances");
        goto fail;
    }

    // Convert all of the instances in the client update to adv_instance_t structures for easy comparison.
    // Any that are unchanged will have to be freed--oh well.
    i = 0;
    for (instance = client_update->instances; instance != NULL; instance = instance->next) {
        adv_instance_t *prepared_instance = adv_instance_create(instance, host, update);
        if (prepared_instance == NULL) {
            // prepare_instance logs.
            goto fail;
        }
        if (i >= num_add_instances) {
            FAULT("while preparing client update instances, i >= num_add_instances");
            RELEASE_HERE(prepared_instance, adv_instance);
            prepared_instance = NULL;
            goto fail;
        }

        prepared_instance->anycast = false;
        if (client_update != NULL && client_update->connection != NULL) {
            const struct sockaddr *server_addr = connection_get_local_address(client_update->message);
            if (server_addr && server_addr->sa_family == AF_INET6) {
                const struct in6_addr *const ipv6_address = &(((const struct sockaddr_in6 *)server_addr)->sin6_addr);
                uint16_t server_port = ntohs(((const struct sockaddr_in6 *)server_addr)->sin6_port);
                SEGMENTED_IPv6_ADDR_GEN_SRP(ipv6_address, addr_buf);
                INFO("server address " PRI_SEGMENTED_IPv6_ADDR_SRP "; server port %d",
                      SEGMENTED_IPv6_ADDR_PARAM_SRP(ipv6_address, addr_buf), server_port);
                if (is_thread_mesh_anycast_address(ipv6_address) && server_port == 53) {
                    prepared_instance->anycast = true;
                }
            }
        }
        add_instances->vec[i++] = prepared_instance;
    }
    add_instances->num = i;

    // The instances in the update are now in add_instances.  If they are updates, move them to update_instances.  If
    // they are unchanged, free them and null them out, and remember the current instance in renew_instances.  If they
    // are adds, leave them.
    for (i = 0; i < num_add_instances; i++) {
        adv_instance_t *add_instance = add_instances->vec[i];

        if (add_instance != NULL) {
            for (j = 0; j < host->instances->num; j++) {
                adv_instance_t *host_instance = host->instances->vec[j];

                // See if the instance names match.
                if (host_instance != NULL &&
                    !strcmp(add_instance->instance_name, host_instance->instance_name) &&
                    service_types_equal(add_instance->service_type, host_instance->service_type))
                {
                    // If the rdata is the same, and the service type is the same (including subtypes), and it's not
                    // deleted, it's not an add or an update.
                    if (!host_instance->removed && add_instance->txt_length == host_instance->txt_length &&
                        add_instance->port == host_instance->port &&
                        !strcmp(add_instance->service_type, host_instance->service_type) &&
                        (add_instance->txt_length == 0 ||
                         !memcmp(add_instance->txt_data, host_instance->txt_data, add_instance->txt_length)))
                    {
                        RELEASE_HERE(add_instances->vec[i], adv_instance);
                        add_instances->vec[i] = NULL;
                        renew_instances->vec[j] = host_instance;
                        RETAIN_HERE(host_instance, adv_instance);
                        renew_instances->vec[j]->update = update;
                        RETAIN_HERE(renew_instances->vec[j]->update, adv_update);
                        INFO(PRI_S_SRP "." PRI_S_SRP " renewed for host " PRI_S_SRP,
                             host_instance->instance_name, host_instance->service_type, host->name);
                    } else {
                        update_instances->vec[j] = add_instance;
                        RETAIN_HERE(update_instances->vec[j], adv_instance);
                        RELEASE_HERE(add_instances->vec[i], adv_instance);
                        add_instances->vec[i] = NULL;
                    }
                    break;
                }
            }
        }
    }

    // At this point we have figured out all the work we need to do, so hang it off an update structure.
    update->host = host;
    RETAIN_HERE(update->host, adv_host);
    update->client = client_update;
    update->remove_addresses = remove_addrs;
    update->add_addresses = add_addrs;
    update->remove_instances = remove_instances;
    update->add_instances = add_instances;
    update->update_instances = update_instances;
    update->renew_instances = renew_instances;
    update->host_lease = client_update->host_lease;
    update->key_lease = client_update->key_lease;

    // Register any added addresses with threadradiod before we actually advertise them, to avoid a spurious
    // address query.

    host->update = update;
    RETAIN_HERE(host->update, adv_update);
    RELEASE_HERE(update, adv_update);
    update = NULL;


    start_host_update(host);
    return;

fail:
    if (client_update != NULL) {
        srp_parse_client_updates_free(client_update);
        client_update = NULL;
    }
    if (remove_addrs != NULL) {
        // Addresses in remove_addrs are owned by the host and don't need to be freed.
        RELEASE_HERE(remove_addrs, adv_record_vec);
        remove_addrs = NULL;
    }
    if (add_addrs != NULL) {
        RELEASE_HERE(add_addrs, adv_record_vec);
        add_addrs = NULL;
    }
    if (add_instances != NULL) {
        RELEASE_HERE(add_instances, adv_instance_vec);
        add_instances = NULL;
    }
    if (remove_instances != NULL) {
        RELEASE_HERE(remove_instances, adv_instance_vec);
        remove_instances = NULL;
    }
    if (update_instances != NULL) {
        RELEASE_HERE(update_instances, adv_instance_vec);
        update_instances = NULL;
    }
    if (update) {
        RELEASE_HERE(update, adv_update);
    }
}

typedef enum { missed, match, conflict } instance_outcome_t;
static instance_outcome_t
compare_instance(adv_instance_t *instance,
                 dns_host_description_t *new_host, adv_host_t *host,
                 char *instance_name, char *service_type)
{
    if (instance == NULL) {
        return missed;
    }
    if (host->removed) {
        return missed;
    }
    if (!strcmp(instance_name, instance->instance_name) && service_types_equal(service_type, instance->service_type)) {
        if (!dns_names_equal_text(new_host->name, host->name)) {
            return conflict;
        }
        return match;
    }
    return missed;
}

bool
srp_update_start(client_update_t *client_update)
{
    dns_host_description_t *new_host = client_update->host;
    char new_host_name[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    srp_server_t *server_state = client_update->server_state;
    uint32_t key_id = 0;
    dns_name_print(new_host->name, new_host_name, sizeof new_host_name);
    adv_host_t *host = NULL;
    srpl_connection_t *srpl_connection = client_update->srpl_connection;
    message_t *raw_message = client_update->message;
    comm_t *connection = client_update->connection;


    // Compute a checksum on the key, ignoring up to three bytes at the end.
    for (client_update_t *update = client_update; update != NULL; update = update->next) {
        dns_host_description_t *update_host = update->host;

        uint32_t update_key_id = 0;
        for (unsigned i = 0; i < update_host->key->data.key.len; i += 4) {
            update_key_id += ((update_host->key->data.key.key[i] << 24) | (update_host->key->data.key.key[i + 1] << 16) |
                              (update_host->key->data.key.key[i + 2] << 8) | (update_host->key->data.key.key[i + 3]));
        }
        if (update == client_update) {
            key_id = update_key_id;
        } else if (key_id != update_key_id) {
            ERROR("update contains multiple key ids %x and %x", key_id, update_key_id);
            advertise_finished(NULL, new_host_name, server_state,
                               srpl_connection, NULL, raw_message, dns_rcode_refused, NULL, false, true);
            goto cleanup;
        }
    }

    char seenbuf[200];
    char *already_seen = seenbuf;
    const char *plural = "";

    // For replicated updates, check the transaction IDs to make sure we aren't applying an update we've already gotten.
    if (srpl_connection != NULL) {
        for (host = server_state->hosts; host != NULL; host = host->next) {
            if (host->key_id == key_id && !strcmp(host->name, new_host_name)) {
                break;
            }
        }

        if (host != NULL) {
            bool replay = true;
            while (client_update != NULL && replay) {
                replay = false;
                if (host->message != NULL && host->message->wire.id == client_update->message->wire.id) {
                    replay = true;
                } else if (host->instances != NULL) {
                    for (int i = 0; i < host->instances->num; i++) {
                        adv_instance_t *instance = host->instances->vec[i];
                        if (instance != NULL) {
                            if (instance->message != NULL &&
                                instance->message->wire.id == client_update->message->wire.id)
                            {
                                replay = true;
                                break;
                            }
                        }
                    }
                }
                if (replay) {
                    client_update_t *skip_update = client_update;
                    client_update = client_update->next;
                    if (already_seen != seenbuf) {
                        plural = "s";
                    }
                    if (already_seen + 6 < &seenbuf[sizeof(seenbuf)]) {
                        snprintf(already_seen, 6, " %04x", skip_update->message->wire.id);
                        already_seen += 5;
                    }
                    skip_update->next = NULL;
                    srp_parse_client_updates_free(skip_update);
                    if (client_update != NULL) {
                        new_host = client_update->host;
                    } else {
                        new_host = NULL;
                    }
                }
            }
        }
    }
    if (already_seen != seenbuf) {
        INFO("host update for " PRI_S_SRP ", key id %" PRIx32 " " PUB_S_SRP " (skipped xid" PUB_S_SRP PUB_S_SRP ")",
             new_host_name, key_id, srpl_connection == NULL ? "" : srpl_connection->name, plural, seenbuf);
    } else {
        INFO("host update for " PRI_S_SRP ", key id %" PRIx32 " " PUB_S_SRP,
             new_host_name, key_id, srpl_connection == NULL ? "" : srpl_connection->name);
    }
    if (client_update == NULL) {
        advertise_finished(host, new_host_name, server_state, srpl_connection,
                           NULL, raw_message, dns_rcode_noerror, NULL, false, true);
        return true; // It's safe to just return here because we've freed all the client updates.
    }

    service_instance_t *instances = client_update->instances;
    delete_t *removes = client_update->removes;
    adv_host_t **p_hosts = NULL;
    char pres_name[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    service_instance_t *new_instance;
    instance_outcome_t outcome = missed;
    char instance_name[DNS_MAX_LABEL_SIZE_ESCAPED + 1];
    char service_type[DNS_MAX_LABEL_SIZE_ESCAPED * 2 + 2];
    host_addr_t *addr;
    const bool remove = client_update->host_lease == 0;
    const char *updatestr = client_update->host_lease == 0 ? "remove" : "update";
    delete_t *dp;


    for (addr = new_host->addrs; addr != NULL; addr = addr->next) {
        if (addr->rr.type == dns_rrtype_a) {
            IPv4_ADDR_GEN_SRP(&addr->rr.data.a.s_addr, addr_buf);
            INFO("host " PUB_S_SRP " for " PRI_S_SRP ", address " PRI_IPv4_ADDR_SRP " " PUB_S_SRP, updatestr,
                 new_host_name, IPv4_ADDR_PARAM_SRP(&addr->rr.data.a.s_addr, addr_buf),
                 srpl_connection == NULL ? "" : srpl_connection->name);
        } else {
            SEGMENTED_IPv6_ADDR_GEN_SRP(addr->rr.data.aaaa.s6_addr, addr_buf);
            INFO("host " PUB_S_SRP " for " PRI_S_SRP ", address " PRI_SEGMENTED_IPv6_ADDR_SRP " " PUB_S_SRP,
                 updatestr, new_host_name, SEGMENTED_IPv6_ADDR_PARAM_SRP(addr->rr.data.aaaa.s6_addr, addr_buf),
                 srpl_connection == NULL ? "" : srpl_connection->name);
        }
    }
    for (new_instance = instances; new_instance != NULL; new_instance = new_instance->next) {
        extract_instance_name(instance_name, sizeof instance_name, service_type, sizeof service_type, new_instance);
        INFO("host " PUB_S_SRP " for " PRI_S_SRP ", instance name " PRI_S_SRP ", type " PRI_S_SRP
             ", port %d " PUB_S_SRP, updatestr, new_host_name, instance_name, service_type,
             new_instance->srv != NULL ? new_instance->srv->data.srv.port : -1,
             srpl_connection == NULL ? "" : srpl_connection->name);
        if (new_instance->txt != NULL) {
            char txt_buf[DNS_DATA_SIZE];
            dns_txt_data_print(txt_buf, DNS_DATA_SIZE, new_instance->txt->data.txt.len, new_instance->txt->data.txt.data);
            INFO("text data for instance " PRI_S_SRP ": " PRI_S_SRP, instance_name, txt_buf);
        }
    }

    // Look for matching service instance names.   A service instance name that matches, but has a different
    // hostname, means that there is a conflict.   We have to look through all the entries; the presence of
    // a matching hostname doesn't mean we are done UNLESS there's a matching service instance name pointing
    // to that hostname.
    for (host = server_state->hosts; host; host = host->next) {
        // If a host has been removed, it won't have any instances to compare against. Later on, if we find that
        // there is no matching host for this update, we look through the host list again and remove the
        // "removed" host if it has the same name, so we don't need to do anything further here.
        if (host->removed) {
            continue;
        }
        // We need to look for matches both in the registered instances for this registration, and also in
        // the list of new instances, in case we get a duplicate update while a previous update is in progress.
        for (new_instance = instances; new_instance; new_instance = new_instance->next) {
            extract_instance_name(instance_name, sizeof instance_name, service_type, sizeof service_type, new_instance);

            // First check for a match or conflict in the host itself.
            for (int i = 0; i < host->instances->num; i++) {
                outcome = compare_instance(host->instances->vec[i], new_host, host,
                                           instance_name, service_type);
                if (outcome != missed) {
                    goto found_something;
                }
            }

            // Then look for the same thing in any subsequent updates that have been baked.
            if (host->update != NULL) {
                if (host->update->add_instances != NULL) {
                    for (int i = 0; i < host->update->add_instances->num; i++) {
                        outcome = compare_instance(host->update->add_instances->vec[i], new_host, host,
                                                   instance_name, service_type);
                        if (outcome != missed) {
                            goto found_something;
                        }
                    }
                }
            }
        }
    }
found_something:
    if (outcome == conflict) {
        ERROR("service instance name " PRI_S_SRP "/" PRI_S_SRP " already pointing to host "
              PRI_S_SRP ", not host " PRI_S_SRP, instance_name, service_type, host->name, new_host_name);
        advertise_finished(NULL, host->name,
                           server_state, srpl_connection, connection, raw_message, dns_rcode_yxdomain, NULL, true, true);
        goto cleanup;
    }

    // We may have received removes for individual records. In this case, we need to make sure they only remove
    // records that have been added to the host that matches.
    for (adv_host_t *rhp = server_state->hosts; rhp != NULL; rhp = rhp->next) {
        if (rhp->removed) {
            continue;
        }

        // Look for removes that conflict
        for (dp = removes; dp != NULL; dp = dp->next) {
            // We only need to do this for service instance names. We don't really know what is and isn't a
            // service instance name, but if it /could/ be a service instance name, we compare; if it matches,
            // it is a service instance name, and if not, no problem.
            if (dp->name != NULL && dp->name->next != NULL && dp->name->next->next != NULL) {
                dns_name_print_to_limit(dp->name, dp->name->next, instance_name, sizeof(instance_name));
                dns_name_print_to_limit(dp->name->next, dp->name->next->next->next, service_type, sizeof(service_type));

                // See if the delete deletes an instance on the host
                for (int i = 0; i < rhp->instances->num; i++) {
                    adv_instance_t *instance = rhp->instances->vec[i];
                    if (instance != NULL) {
                        if (!strcmp(instance_name, instance->instance_name) &&
                            service_types_equal(service_type, instance->service_type))
                        {
                            if (!strcmp(new_host_name, rhp->name)) {
                                ERROR("remove for " PRI_S_SRP "." PRI_S_SRP " matches instance on host " PRI_S_SRP,
                                      instance_name, service_type, rhp->name);
                                dp->consumed = true;
                            } else {
                                ERROR("remove for " PRI_S_SRP "." PRI_S_SRP " conflicts with instance on host " PRI_S_SRP,
                                      instance_name, service_type, rhp->name);
                                advertise_finished(NULL, rhp->name, server_state, srpl_connection,
                                                   connection, raw_message, dns_rcode_formerr, NULL, true, true);
                                goto cleanup;
                            }
                        }
                    }
                }

                // See if the remove removes an instance on an update on the host
                if (rhp->update) {
                    if (rhp->update->add_instances != NULL) {
                        for (int i = 0; i < rhp->update->add_instances->num; i++) {
                            adv_instance_t *instance = rhp->update->add_instances->vec[i];
                            if (instance != NULL) {
                                if (!strcmp(instance_name, instance->instance_name) &&
                                    service_types_equal(service_type, instance->service_type))
                                {
                                    if (!strcmp(new_host_name, rhp->name)) {
                                        dp->consumed = true;
                                    } else {
                                        ERROR("remove for " PRI_S_SRP " conflicts with instance on update to host " PRI_S_SRP,
                                              instance->instance_name, rhp->name);
                                        advertise_finished(NULL, rhp->name, server_state, srpl_connection,
                                                           connection, raw_message, dns_rcode_formerr, NULL, true, true);
                                        goto cleanup;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Log any unmatched deletes, but we don't consider these to be errors.
    for (dp = removes; dp != NULL; dp = dp->next) {
        if (!dp->consumed) {
            DNS_NAME_GEN_SRP(dp->name, name_buf);
            INFO("remove for " PRI_DNS_NAME_SRP " doesn't match any instance on any host.",
                 DNS_NAME_PARAM_SRP(dp->name, name_buf));
        }
    }

    // If we fall off the end looking for a matching service instance, there isn't a matching
    // service instance, but there may be a matching host, so look for that.
    if (outcome == missed) {
        // Search for the new hostname in the list of hosts, which is sorted.
        for (p_hosts = &server_state->hosts; *p_hosts; p_hosts = &host->next) {
            host = *p_hosts;
            int comparison = strcasecmp(new_host_name, host->name);
            if (comparison == 0) {
                // If we get an update for a host that was removed, and it's not also a remove,
                // remove the host entry that's marking the remove. If this is a remove, just flag
                // it as a miss.
                if (host->removed) {
                    outcome = missed;
                    if (remove) {
                        break;
                    }
                    // if remove is more recent than this message (for example, we firt receive remove
                    // from the actual client and then receive a stale update message from a replication
                    // peer), we don't apply this message and end processing here.
                    if (host->remove_received_time > client_update->message->received_time) {
                        INFO("update for host " PRI_S_SRP " which has been deleted.", host->name);
                        advertise_finished(NULL, host->name, server_state, srpl_connection,
                                           connection, raw_message, dns_rcode_servfail, NULL, true, true);
                        goto cleanup;
                    }
                    *p_hosts = host->next;
                    host_invalidate(host);
                    RELEASE_HERE(host, adv_host);
                    host = NULL;
                    break;
                }
                if (key_id == host->key_id && dns_keys_rdata_equal(new_host->key, &host->key)) {
                    outcome = match;
                    break;
                }
                ERROR("update for host " PRI_S_SRP " has key id %" PRIx32
                      " which doesn't match host key id %" PRIx32 ".",
                      host->name, key_id, host->key_id);
                advertise_finished(NULL, host->name, server_state, srpl_connection,
                                   connection, raw_message, dns_rcode_yxdomain, NULL, true, true);
                goto cleanup;
            } else if (comparison < 0) {
                break;
            }
        }
    } else {
        if (key_id != host->key_id || !dns_keys_rdata_equal(new_host->key, &host->key)) {
            ERROR("new host with name " PRI_S_SRP " and key id %" PRIx32
                  " conflicts with existing host " PRI_S_SRP " with key id %" PRIx32,
                  new_host_name, key_id, host->name, host->key_id);
            advertise_finished(NULL, host->name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_yxdomain, NULL, true, true);
            goto cleanup;
        }
    }

    // If we didn't find a matching host, we can make a new one.   When we create it, it just has
    // a name and no records.  The update that we then construct will have the missing records.
    // We don't want to do this for a remove, obviously.
    if (outcome == missed) {
        if (remove) {
            ERROR("Remove for host " PRI_S_SRP " which doesn't exist.", new_host_name);
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_noerror, NULL, true, true);
            goto cleanup;
        }
        host = calloc(1, sizeof *host);
        if (host == NULL) {
            ERROR("no memory for host data structure.");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            goto cleanup;
        }
        RETAIN_HERE(host, adv_host);
        host->server_state = server_state;
        host->instances = adv_instance_vec_create(0);
        if (host->instances == NULL) {
            ERROR("no memory for host instance vector.");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            RELEASE_HERE(host, adv_host);
            host = NULL;
            goto cleanup;
        }
        host->addresses = adv_record_vec_create(0);
        if (host->addresses == NULL) {
            ERROR("no memory for host address vector.");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            RELEASE_HERE(host, adv_host);
            host = NULL;
            goto cleanup;
        }

        host->retry_wakeup = ioloop_wakeup_create();
        if (host->retry_wakeup != NULL) {
            host->lease_wakeup = ioloop_wakeup_create();
        }
        if (host->lease_wakeup == NULL) {
            ERROR("no memory for wake event on host");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            RELEASE_HERE(host, adv_host);
            host = NULL;
            goto cleanup;
        }
        dns_name_print(new_host->name, pres_name, sizeof pres_name);
        host->name = strdup(pres_name);
        if (host->name == NULL) {
            RELEASE_HERE(host, adv_host);
            host = NULL;
            ERROR("no memory for hostname.");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            goto cleanup;
        }
        host->key = *new_host->key;
#ifndef __clang_analyzer__
        // Normally this would be invalid, but we never use the name of the key record.
        host->key.name = NULL;
#endif
        host->key_rdlen = new_host->key->data.key.len + 4;
        host->key_rdata = malloc(host->key_rdlen);
        if (host->key_rdata == NULL) {
            RELEASE_HERE(host, adv_host);
            host = NULL;
            ERROR("no memory for host key.");
            advertise_finished(NULL, new_host_name, server_state, srpl_connection,
                               connection, raw_message, dns_rcode_servfail, NULL, true, true);
            goto cleanup;
        }
        memcpy(host->key_rdata, &new_host->key->data.key.flags, 2);
        host->key_rdata[2] = new_host->key->data.key.protocol;
        host->key_rdata[3] = new_host->key->data.key.algorithm;
        memcpy(&host->key_rdata[4], new_host->key->data.key.key, new_host->key->data.key.len);
        host->key.data.key.key = &host->key_rdata[4];
        host->key_id = key_id;

        // Insert this in the list where it would have sorted.  The if test is because the optimizer doesn't notice that
        // p_hosts can never be null here--it will always be pointing to the end of the list of hosts if we get here.
        if (p_hosts != NULL) {
            host->next = *p_hosts;
            *p_hosts = host;
        }
        p_hosts = NULL;
    }

    // If we are already updating this host, either this is a retransmission, or it's a new transaction. In the case
    // of a retransmission, we need to keep doing the work we've been asked to do, and hopefully we'll reply before the
    // client gives up. In the case of a new request, we aren't ready for it yet; the client really shouldn't have sent
    // it so quickly, but if it's behaving correctly, we should be done with the current update before it retransmits,
    // so we can safely ignore it. If we're getting a replication update, it can't be newer than the current update.
    // So we can ignore it--we'll send a replication update when we're done processing the client update.
    if (host->update != NULL) {
#ifdef SRP_DETECT_STALLS
        time_t now = srp_time();
        // It's possible that we could get an update that stalls due to a problem communicating with mDNSResponder
        // and that a timing race prevents this from being detected correctly. In this case, cancel the update and
        // let the retry go through. We don't want to do this unless there's a clear stall, so we're allowing ten
        // seconds.
        if (now - host->update->start_time > 10) {
            INFO("update has stalled, failing it silently.");
            update_failed(host->update, dns_rcode_servfail, false, false);
            service_disconnected(server_state, (intptr_t)server_state->shared_registration_txn);
        } else {
#endif // SRP_DETECT_STALLS
            INFO("dropping retransmission of in-progress update for host " PRI_S_SRP, host->name);
#if SRP_FEATURE_REPLICATION
            srp_replication_advertise_finished(host, host->name, server_state, srpl_connection,
                                               connection, dns_rcode_servfail, true);
#endif
        cleanup:
            srp_parse_client_updates_free(client_update);
            return false;
#ifdef SRP_DETECT_STALLS
        }
#endif
    }

    // If this is a remove, remove the host registrations and mark the host removed. We keep it around until the
    // lease expires to prevent replication accidentally re-adding a removed host as a result of a bad timing
    // coincidence.
    if (remove) {
        host_invalidate(host);
        // We need to propagate the remove message.
        if (host->message != NULL) {
            ioloop_message_release(host->message);
        }
        host->message = raw_message;
        // remember the time when the message that removes the host was received
        host->remove_received_time = host->message->received_time;
        ioloop_message_retain(host->message);
        advertise_finished(host, new_host_name, server_state, srpl_connection,
                           connection, raw_message, dns_rcode_noerror, NULL, true, true);
        goto cleanup;
    }

    // At this point we have an update and a host to which to apply it.  We may already be doing an earlier
    // update, or not.  Create a client update structure to hold the communication, so that when we are done,
    // we can respond.
    if (outcome == missed) {
        INFO("New host " PRI_S_SRP ", key id %" PRIx32 , host->name, host->key_id);
    } else {
        if (host->registered_name != host->name) {
            INFO("Renewing host " PRI_S_SRP ", alias " PRI_S_SRP ", key id %" PRIx32,
                 host->name, host->registered_name, host->key_id);
        } else {
            INFO("Renewing host " PRI_S_SRP ", key id %" PRIx32, host->name, host->key_id);
        }
    }

    if (host->registered_name == NULL) {
        host->registered_name = host->name;
    }

    // We have to take the lease from the SRP update--the original registrar negotiated it, and if it's out
    // of our range, that's too bad (ish).
    if (raw_message->lease != 0) {
        INFO("basing lease time on message: raw_message->lease = %d, raw_message->key_lease = %d",
             raw_message->lease, raw_message->key_lease);
        client_update->host_lease = raw_message->lease;
        client_update->key_lease = raw_message->key_lease;
    } else {
        if (client_update->host_lease < server_state->max_lease_time) {
            if (client_update->host_lease < server_state->min_lease_time) {
                INFO("basing lease time on server_state->min_lease_time: %d", server_state->min_lease_time);
                client_update->host_lease = server_state->min_lease_time;
            } else {
                INFO("basing lease time on client_update->host_lease: %d", client_update->host_lease);
                // client_update->host_lease = client_update->host_lease;
            }
        } else {
            client_update->host_lease = server_state->max_lease_time;
                INFO("basing lease time on server_state->max_lease_time: %d", server_state->max_lease_time);
        }
        if (client_update->key_lease < server_state->key_max_lease_time) {
            if (client_update->key_lease < server_state->key_min_lease_time) {
                client_update->key_lease = server_state->key_min_lease_time;
            } else {
                // client_update->key_lease = client_update->key_lease;
            }
        } else {
            client_update->key_lease = server_state->key_max_lease_time;
        }
    }

#if SRP_FEATURE_REPLICATION
    if (srpl_connection != NULL) {
        host->srpl_connection = srpl_connection;
        srpl_connection_retain(host->srpl_connection);
    }
#endif // SRP_FEATURE_REPLICATION

    // Apply the update.
    prepare_update(host, client_update);
    return true;
}

void
srp_mdns_flush(srp_server_t *server_state)
{
    adv_host_t *host, *host_next;

    INFO("flushing all host entries.");
    for (host = server_state->hosts; host; host = host_next) {
        INFO("Flushing services and host entry for " PRI_S_SRP " (" PRI_S_SRP ")",
             host->name, host->registered_name);
        // Get rid of the updates before calling delete_host, which will fail if update is not NULL.
        if (host->update != NULL) {
            update_failed(host->update, dns_rcode_refused, false, true);
        }
        host_next = host->next;
        host_remove(host);
    }
    server_state->hosts = NULL;
}

static void
usage(void)
{
    ERROR("srp-mdns-proxy [--max-lease-time <seconds>] [--min-lease-time <seconds>] [--log-stderr]");
    ERROR("               [--enable-replication | --disable-replication]");
#if SRP_FEATURE_NAT64
    ERROR("               [--enable-nat64 | --disable-nat64]");
#endif
    exit(1);
}

srp_server_t *
server_state_create(const char *name, int max_lease_time, int min_lease_time,
                    int key_max_lease_time, int key_min_lease_time)
{
    srp_server_t *server_state = calloc(1, sizeof(*server_state));
    if (server_state == NULL || (server_state->name = strdup(name)) == NULL) {
        ERROR("no memory for server state");
        free(server_state);
        return NULL;
    }
    server_state->max_lease_time = max_lease_time;
    server_state->min_lease_time = min_lease_time;
    server_state->key_max_lease_time = key_max_lease_time;
    server_state->key_min_lease_time = key_min_lease_time;
    server_state->priority = PRIORITY_DEFAULT;
#if TARGET_OS_TV
#endif
    INFO("priority set to %d", server_state->priority);
    return server_state;
}

static void
object_allocation_stats_dump_callback(void *context)
{
    srp_server_t *server_state = context;

    ioloop_dump_object_allocation_stats();

    if (server_state->full_dump_count == 0) {
        srp_dump_server_stats(server_state, true, true);
        server_state->full_dump_count = 12;
    } else {
        srp_dump_server_stats(server_state, false, true);
    }
    --server_state->full_dump_count;

    // Do the next object memory allocation statistics dump in five minutes
    ioloop_add_wake_event(server_state->object_allocation_stats_dump_wakeup, server_state,
                          object_allocation_stats_dump_callback, NULL, 5 * 60 * 1000);
}

int
main(int argc, char **argv)
{
    int i;
    char *end;
    int log_stderr = false;
#ifdef SRP_TEST_SERVER
    char *test_to_run = NULL;
    bool normal_srp_startup = false;
#else
    bool normal_srp_startup = true;
#endif
#if STUB_ROUTER
    bool stub_router_enabled = false;
#endif
    bool thread_device_enabled = false;

    srp_servers = server_state_create("srp-mdns-proxy",
                                      3600 * 27,     // max lease time one day plus 20%
                                      30,            // min lease time 30 seconds
                                      3600 * 24 * 7, // max key lease 7 days
                                      30);           // min key lease time 30s
    if (srp_servers == NULL) {
        return 1;
    }

    if (normal_srp_startup) {
        srp_servers->srp_replication_enabled = true;
#  if SRP_FEATURE_NAT64
        srp_servers->srp_nat64_enabled = true;
#  endif
    }


    // Set the advertise interface
    if (0) {
#if STUB_ROUTER
    } else if (stub_router_enabled) {
        srp_servers->advertise_interface = kDNSServiceInterfaceIndexAny;
#endif
    } else {
        srp_servers->advertise_interface = if_nametoindex("lo0");
    }
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--max-lease-time")) {
            if (i + 1 == argc) {
                usage();
            }
            srp_servers->max_lease_time = (uint32_t)strtoul(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || end[0] != 0) {
                usage();
            }
            i++;
        } else if (!strcmp(argv[i], "--min-lease-time")) {
            if (i + 1 == argc) {
                usage();
            }
            srp_servers->min_lease_time = (uint32_t)strtoul(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || end[0] != 0) {
                usage();
            }
            i++;
        } else if (!strcmp(argv[i], "--log-stderr")) {
            log_stderr = true;
#ifdef LOG_FPRINTF_STDERR
        } else if (!strcmp(argv[i], "--log-relative-timestamp")) {
            srp_log_timestamp_relative = true;
#endif
        } else if (!strcmp(argv[i], "--enable-replication")) {
            srp_servers->srp_replication_enabled = true;
        } else if (!strcmp(argv[i], "--disable-replication")) {
            srp_servers->srp_replication_enabled = false;
        } else if (!strcmp(argv[i], "--fake-xpanid")) {
            if (i + 1 == argc) {
                usage();
            }
            srp_servers->xpanid = strtoul(argv[i + 1], &end, 16);
            if (end == argv[i + 1] || end[0] != 0) {
                usage();
            }
#ifdef SRP_TEST_SERVER
        } else if (!strcmp(argv[i], "--test")) {
            if (i + 1 == argc) {
                usage();
            }
            test_to_run = argv[i + 1];
            i++;
#endif
#if SRP_FEATURE_NAT64
        } else if (!strcmp(argv[i], "--enable-nat64")) {
            srp_servers->srp_nat64_enabled = true;
        } else if (!strcmp(argv[i], "--disable-nat64")) {
            srp_servers->srp_nat64_enabled = false;
#endif
        } else {
            usage();
        }
    }

    // Setup log category for srp-mdns-prox and dnssd-proxy.
    OPENLOG("srp-mdns-proxy", log_stderr);

#ifdef SRP_TEST_SERVER
    INFO("srp-test-server starting, compiled on " PUB_S_SRP ", " PUB_S_SRP, __DATE__, __TIME__);
#else
    INFO("--------------------------------"
         "srp-mdns-proxy starting, compiled on " PUB_S_SRP ", " PUB_S_SRP
         "--------------------------------", __DATE__, __TIME__);
#endif

    if (!ioloop_init()) {
        return 1;
    }

    if (normal_srp_startup) {
#if THREAD_DEVICE
        if (0) {
#if STUB_ROUTER
        } else if (stub_router_enabled) {
            srp_servers->route_state = route_state_create(srp_servers, "srp-mdns-proxy");
            if (srp_servers->route_state == NULL) {
                return 1;
            }
#endif // STUB_ROUTER
        }

        if (!srp_mdns_shared_registration_txn_setup(srp_servers)) {
            return 1;
        }
        dns_service_op_not_to_be_freed = srp_servers->shared_registration_txn->sdref;
#endif // THREAD_DEVICE

#if STUB_ROUTER
        if (stub_router_enabled) {
            // Set up the ULA early just in case we get an early registration, nat64 will use the ula
            route_ula_setup(srp_servers->route_state);
        }
#endif


#if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)
        if (!init_dnssd_proxy(srp_servers)) {
            ERROR("failed to setup dnssd-proxy");
        }
#endif // #if (SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY)

#if STUB_ROUTER
        if (stub_router_enabled) {
            if (!start_icmp_listener()) {
                ERROR("failed to start icmp listener");
            }
        }
#endif


        infrastructure_network_startup(srp_servers->route_state);

        if (adv_ctl_init(srp_servers) != kDNSServiceErr_NoError) {
            ERROR("Can't start advertising proxy control server.");
        }

        // We require one open file per service and one per instance.
        struct rlimit limits;
        if (getrlimit(RLIMIT_NOFILE, &limits) < 0) {
            ERROR("getrlimit failed: " PUB_S_SRP, strerror(errno));
        }

        if (limits.rlim_cur < 1024) {
            if (limits.rlim_max < 1024) {
                INFO("file descriptor hard limit is %llu", (unsigned long long)limits.rlim_max);
                if (limits.rlim_cur != limits.rlim_max) {
                    limits.rlim_cur = limits.rlim_max;
                }
            } else {
                limits.rlim_cur = 1024;
            }
            if (setrlimit(RLIMIT_NOFILE, &limits) < 0) {
                ERROR("setrlimit failed: " PUB_S_SRP, strerror(errno));
            }
        }

        srp_proxy_init("local");
#ifdef SRP_TEST_SERVER
    } else  {
        ioloop_run_async(srp_test_server_run_test, test_to_run);
#endif
    }

    srp_servers->object_allocation_stats_dump_wakeup = ioloop_wakeup_create();
    if (srp_servers->object_allocation_stats_dump_wakeup == NULL) {
        INFO("no memory for srp_servers->object_allocation_stats_dump_wakeup");
    } else {
        // Do an object memory allocation statistics dump every five minutes, and a full database dump every half hour
        // starting after the first five minutes
        srp_servers->full_dump_count = 1;
        object_allocation_stats_dump_callback(srp_servers);
    }

    ioloop();
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
