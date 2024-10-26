/* service-publisher.c
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
 * This file contains code to queue and send updates for Thread services.
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
#include "ifpermit.h"
#include "srp-dnssd.h"

#define STATE_MACHINE_IMPLEMENTATION 1
typedef enum {
    service_publisher_state_invalid,
    service_publisher_state_startup,
    service_publisher_state_waiting_to_publish,
    service_publisher_state_not_publishing,
    service_publisher_state_start_listeners,
    service_publisher_state_publishing,
} state_machine_state_t;
#define state_machine_state_invalid service_publisher_state_invalid

#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"

#include "service-publisher.h"
#include "thread-tracker.h"
#include "node-type-tracker.h"

// This is pretty short because our normal use case is connecting to a single device, so we don't actually anticipate
// normally seeing a service. But if we do, we don't suffer power events, so we don't need to desynchronize from other
// devices booting at the same time. So we just want to wait long enough that if there's already a service published,
// we see it and don't publish.
#define SERVICE_PUBLISHER_START_WAIT 750

// When a competing service is lost, we want to wait a bit longer.
#define SERVICE_PUBLISHER_LOST_WAIT 5000

// When the listener restarts, we don't actually want to wait.
#define SERVICE_PUBLISHER_LISTENER_RESTART_WAIT 1

#define ADDRESS_RECORD_TTL  4500
#define OTHER_RECORD_TTL    4500

struct service_publisher {
    int ref_count;
    state_machine_header_t state_header;
    char *id;
    char *thread_interface_name;
    srp_server_t *server_state;
    wakeup_t *NULLABLE wakeup_timer;
    wakeup_t *NULLABLE sed_timeout;
    comm_t *srp_listener;
    void (*reconnect_callback)(void *context);
    thread_service_t *published_unicast_service;
    thread_service_t *published_anycast_service;
    thread_service_t *publication_queue;
    cti_connection_t active_data_set_connection;
    cti_connection_t wed_tracker_connection;
    cti_connection_t neighbor_tracker_connection;
    struct in6_addr thread_mesh_local_address;
    struct in6_addr wed_ml_eid;
    struct in6_addr neighbor_ml_eid;
    char *NULLABLE wed_ext_address_string;
    char *NULLABLE wed_ml_eid_string;
    char *NULLABLE neighbor_ml_eid_string;
    int startup_delay_range;
    int retry_interval; // Retry interval in seconds for re-publishing service(s)
    uint16_t srp_listener_port;
    bool have_ml_eid;
    bool first_time;
    bool force_publication;
    bool canceled;
    bool have_srp_listener;
    bool seen_service_list;
    bool stopped;
    bool have_thread_interface_name;
    bool have_unicast_in_net_data, have_anycast_in_net_data;
    bool cached_services_published, started_stale_service_timeout;
};

static uint64_t service_publisher_serial_number;

static void service_publisher_queue_run(service_publisher_t *publisher);

void
service_publisher_unadvertise_all(service_publisher_t *publisher)
{
    srp_server_t *server_state = publisher->server_state;

    publisher->cached_services_published = false;
    for (adv_host_t *host = server_state->hosts; host; host = host->next) {
        // If we have an outstanding update, finish it.
        if (host->update != NULL) {
            srp_mdns_update_finished(host->update);
        }
        if (host->addresses != NULL) {
            for (int i = 0; i < host->addresses->num; i++) {
                adv_record_t *record = host->addresses->vec[i];
                if (record != NULL) {
                    if (record->rrtype == dns_rrtype_aaaa && record->rdlen == 16) {
                        SEGMENTED_IPv6_ADDR_GEN_SRP(record->rdata, rdata_buf);
                        INFO("unadvertising " PRI_S_SRP " IN AAAA " PRI_SEGMENTED_IPv6_ADDR_SRP " rec %p rref %p",
                             host->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(record->rdata, rdata_buf),
                            record, record->rref);
                    }
                    srp_mdns_shared_record_remove(server_state, record);
                    // DNSServiceRemoveRecord should clear the cache, but it doesn't.
                    DNSServiceReconfirmRecord(0, server_state->advertise_interface, host->name, record->rrtype,
                                              dns_qclass_in, record->rdlen, record->rdata);
                }
            }
        }
        if (host->key_record != NULL) {
            srp_mdns_shared_record_remove(server_state, host->key_record);
        }
        if (host->instances != NULL) {
            for (int i = 0; i < host->instances->num; i++) {
                adv_instance_t *instance = host->instances->vec[i];
                if (instance != NULL && instance->txn != NULL) {
                    INFO("unadvertising " PRI_S_SRP "." PRI_S_SRP " instance %p sdref %p",
                         instance->instance_name, instance->service_type, instance, instance->txn->sdref);
                    ioloop_dnssd_txn_cancel(instance->txn);
                    ioloop_dnssd_txn_release(instance->txn);
                    instance->txn = NULL;
                }
            }
        }
    }
}

static void
service_publisher_instance_callback(DNSServiceRef UNUSED sdref, DNSServiceFlags UNUSED flags, DNSServiceErrorType error_code,
                                    const char *name, const char *regtype, const char *domain, void *context)
{
    adv_instance_t *instance = context;
    if (error_code != kDNSServiceErr_NoError) {
        INFO("DNSServiceRegister failed: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP ": %d (instance %p sdref %p)",
             name, regtype, domain, error_code, instance, instance->txn == NULL ? 0 : instance->txn->sdref);
        ioloop_dnssd_txn_cancel(instance->txn);
        ioloop_dnssd_txn_release(instance->txn);
        instance->txn = NULL;
    } else {
        INFO("DNSServiceRegister succeeded: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP " (instance %p sdref %p)",
             instance->instance_name, instance->service_type, instance->host->registered_name,
             instance, instance->txn == NULL ? 0 : instance->txn->sdref);
    }
}

static void
service_publisher_re_advertise_instance(srp_server_t *server_state, adv_host_t *host, adv_instance_t *instance)
{
    DNSServiceRef service_ref = server_state->shared_registration_txn->sdref;

    // Make sure we don't double-register.
    if (instance->txn != NULL) {
        if (instance->txn->sdref != NULL) {
            if (instance->shared_txn == (intptr_t)server_state->shared_registration_txn) {
                INFO("instance is already registered: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP
                     " (instance %p sdref %p)",
                     instance->instance_name, instance->service_type, host->registered_name,
                     instance, instance->txn->sdref);
                return;
            }
            INFO("instance registration is stale: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP
                 " (instance %p sdref %p)",
                 instance->instance_name, instance->service_type, host->registered_name,
                 instance, instance->txn->sdref);
            instance->txn->sdref = NULL;
        }
        ioloop_dnssd_txn_release(instance->txn);
        instance->txn = NULL;
    }

    // Get the TSR attribute for the host object.
    char time_buf[TSR_TIMESTAMP_STRING_LEN];
    DNSServiceAttributeRef tsr_attribute = srp_message_tsr_attribute_generate(NULL, host->key_id,
                                                                              time_buf, sizeof(time_buf));

    int err = dns_service_register_wa(server_state, &service_ref,
                                      (kDNSServiceFlagsShareConnection | kDNSServiceFlagsNoAutoRename |
                                       kDNSServiceFlagsKnownUnique), server_state->advertise_interface,
                                      instance->instance_name, instance->service_type, NULL,
                                      host->registered_name, htons(instance->port), instance->txt_length,
                                      instance->txt_data, tsr_attribute, service_publisher_instance_callback, instance);

    // This would happen if we pass NULL for regtype, which we don't, or if we run out of memory, or if
    // the server isn't running; in the second two cases, we can always try again later.
    if (err != kDNSServiceErr_NoError) {
        INFO("DNSServiceRegister failed: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP ": %d (instance %p)",
             instance->instance_name, instance->service_type, host->registered_name, err, instance);
    } else {
        INFO("DNSServiceRegister succeeded: " PUB_S_SRP "." PUB_S_SRP " host " PRI_S_SRP " at " PUB_S_SRP
             " (instance %p sdref %p)",
             instance->instance_name, instance->service_type, host->registered_name, time_buf,
             instance, service_ref);
        instance->txn = ioloop_dnssd_txn_add_subordinate(service_ref, instance, adv_instance_context_release, NULL);
        if (instance->txn == NULL) {
            ERROR("no memory for instance transaction.");
            DNSServiceRefDeallocate(service_ref);
            return;
        }
        instance->shared_txn = (intptr_t)server_state->shared_registration_txn;
        adv_instance_retain(instance); // for the callback
    }
}

static void
service_publisher_record_callback(DNSServiceRef UNUSED sdref, DNSRecordRef rref,
                                  DNSServiceFlags UNUSED flags, DNSServiceErrorType error_code, void *context)
{
    adv_record_t *record = context;
    const char *host_name = record->host != NULL ? record->host->name : "<null>";
    if (error_code != kDNSServiceErr_NoError) {
        ERROR("re-registration for " PRI_S_SRP " (record %p rref %p) failed with code %d",
              host_name, record, rref, error_code);
        record->rref = NULL;
        record->shared_txn = 0;
        adv_record_release(record); // no more callbacks.
    } else {
        INFO("re-registration for " PRI_S_SRP " (record %p rref %p) succeeded.", host_name, record, rref);
        // could get more callbacks.
    }
}

static void
service_publisher_re_advertise_record(srp_server_t *server_state, adv_host_t *host, adv_record_t *record)
{
    const DNSServiceRef service_ref = host->server_state->shared_registration_txn->sdref;

    // Make sure we don't double register.
    if (record->rref != NULL) {
        if (record->shared_txn == (intptr_t)server_state->shared_registration_txn) {
            INFO("host is already registered: " PUB_S_SRP " (record %p rref %p)",
                 host->registered_name, record, record->rref);
            return;
        }
        INFO("host registration is stale: " PUB_S_SRP " (record %p rref %p)",
             host->registered_name, record, record->rref);
        srp_mdns_shared_record_remove(host->server_state, record);
    }

    // Get the TSR attribute for the host object.
    char time_buf[TSR_TIMESTAMP_STRING_LEN];
    DNSServiceAttributeRef tsr_attribute = srp_message_tsr_attribute_generate(NULL, host->key_id,
                                                                              time_buf, sizeof(time_buf));

    int err = dns_service_register_record_wa(server_state, service_ref, &record->rref, kDNSServiceFlagsKnownUnique,
                                             server_state->advertise_interface, host->registered_name,
                                             record->rrtype, dns_qclass_in, record->rdlen, record->rdata, ADDRESS_RECORD_TTL,
                                             tsr_attribute, service_publisher_record_callback, record);
    if (err != kDNSServiceErr_NoError) {
        INFO("DNSServiceRegisterRecord failed on host " PUB_S_SRP ": %d (record %p)", host->name, err, record);
    } else {
        INFO("DNSServiceRegisterRecord succeeded on host " PUB_S_SRP " at " PUB_S_SRP " (record %p rref %p)",
             host->name, time_buf, record, record->rref);
        record->shared_txn = (intptr_t)host->server_state->shared_registration_txn;
        adv_record_retain(record); // for the callback
    }
}

// Re-advertise records that match the address of the WED device we're bonded to if we're bonded to a WED device, or
// that match the mesh-local prefix of our mesh-local address.  This is a best effort--if it fails, we log it, but it
// just means that our cached info isn't discoverable and we have to wait for a new registration, which should come soon
// or else the cached data wasn't valid.
void
service_publisher_re_advertise_matching(service_publisher_t *publisher)
{
    if (publisher->state_header.state == service_publisher_state_invalid) {
        INFO("publisher is in an invalid state, so we shouldn't re-advertise anything.");
        return;
    }

    srp_server_t *server_state = publisher->server_state;

    // If we don't yet have a shared connection, create one.
    if (!srp_mdns_shared_registration_txn_setup(server_state)) {
        return;
    }

    for (adv_host_t *host = server_state->hosts; host; host = host->next) {
        bool matched = false;

        if (host->addresses != NULL) {
            for (int i = 0; i < host->addresses->num; i++) {
                adv_record_t *record = host->addresses->vec[i];
                // A match means that if we are in WED p2p mode, the ML-EID of the WED matches the record. If not, then
                // it means that the record is on the mesh-local prefix. In both cases, the record has to be a valid
                // AAAA record, of course.
                if (record != NULL && record->rrtype == dns_rrtype_aaaa && record->rdlen == 16 &&
                    (publisher->wed_ml_eid_string != NULL
                     ? !in6addr_compare(&publisher->wed_ml_eid, (struct in6_addr *)record->rdata)
                     : !in6prefix_compare(&publisher->thread_mesh_local_address, (struct in6_addr *)record->rdata, 8)))
                {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(record->rdata, rdata_buf);
                    INFO("re-advertising " PRI_S_SRP " IN AAAA " PRI_SEGMENTED_IPv6_ADDR_SRP,
                         host->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(record->rdata, rdata_buf));
                    service_publisher_re_advertise_record(server_state, host, record);
                    matched = true;
                }
            }
        }
        if (matched) {
            if (host->key_record != NULL) {
                service_publisher_re_advertise_record(server_state, host, host->key_record);
            }
            if (host->instances != NULL) {
                for (int i = 0; i < host->instances->num; i++) {
                    adv_instance_t *instance = host->instances->vec[i];
                    if (instance != NULL && instance->txn == NULL) {
                        service_publisher_re_advertise_instance(server_state, host, instance);
                    }
                }
            }
        }
    }

    publisher->cached_services_published = true;
}

bool
service_publisher_is_address_mesh_local(service_publisher_t *publisher, addr_t *address)
{
    if (address->sa.sa_family == AF_INET) {
        IPv4_ADDR_GEN_SRP(&address->sin.sin_addr, addr_buf);
        if (!IN_LOOPBACK(address->sin.sin_addr.s_addr)) {
            INFO(PRI_IPv4_ADDR_SRP "is not mesh-local", IPv4_ADDR_PARAM_SRP(&address->sin.sin_addr, addr_buf));
            return false;
        }
        INFO(PRI_IPv4_ADDR_SRP "is the IPv4 loopback address", IPv4_ADDR_PARAM_SRP(&address->sin.sin_addr, addr_buf));
        return true;
    }
    if (address->sa.sa_family != AF_INET6) {
        INFO("address family %d can't be mesh-local", address->sa.sa_family);
        return false;
    }

    uint8_t *addr_ptr = (uint8_t *)&address->sin6.sin6_addr;
    SEGMENTED_IPv6_ADDR_GEN_SRP(addr_ptr, addr_buf);
    if (IN6_IS_ADDR_LOOPBACK(&address->sin6.sin6_addr)) {
        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP " is the IPv6 loopback address.",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr_ptr, addr_buf));
        return true;
    }
    static const uint8_t ipv4mapped_loopback[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1 };
    if (!memcmp(&address->sin6.sin6_addr, ipv4mapped_loopback, sizeof(ipv4mapped_loopback))) {
        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP " is the IPv4-mapped loopback address.",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr_ptr, addr_buf));
        return true;
    }

    if (!publisher->have_ml_eid) {
        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP "is not mesh-local",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr_ptr, addr_buf));
        return false;
    }

    SEGMENTED_IPv6_ADDR_GEN_SRP(&publisher->thread_mesh_local_address, mle_buf);
    if (in6prefix_compare(&address->sin6.sin6_addr, &publisher->thread_mesh_local_address, 8)) {
        INFO(PRI_SEGMENTED_IPv6_ADDR_SRP
             " is not on the same prefix as mesh-local address " PRI_SEGMENTED_IPv6_ADDR_SRP ".",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(addr_ptr, addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&publisher->thread_mesh_local_address, mle_buf));
        return false;
    }
    INFO(PRI_SEGMENTED_IPv6_ADDR_SRP "is on the same prefix as mesh-local address " PRI_SEGMENTED_IPv6_ADDR_SRP ".",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(addr_ptr, addr_buf),
         SEGMENTED_IPv6_ADDR_PARAM_SRP(&publisher->thread_mesh_local_address, mle_buf));
    return true;
}

static void
service_publisher_finalize(service_publisher_t *publisher)
{
    thread_service_release(publisher->published_unicast_service);
    thread_service_release(publisher->published_anycast_service);
    thread_service_list_release(&publisher->publication_queue);

    free(publisher->state_header.name);
    free(publisher->wed_ext_address_string);
    free(publisher->wed_ml_eid_string);
    free(publisher->neighbor_ml_eid_string);
    free(publisher->id);
    ioloop_wakeup_release(publisher->wakeup_timer);
    free(publisher);
}

RELEASE_RETAIN_FUNCS(service_publisher);

static void
service_publisher_context_release(void *context)
{
    service_publisher_t *publisher = context;
    RELEASE_HERE(publisher, service_publisher);
}

static void
service_publisher_update_callback(void *context, cti_status_t status)
{
    service_publisher_t *publisher = context;
    thread_service_t *service = publisher->publication_queue;

    if (publisher->canceled) {
        RELEASE_HERE(publisher, service_publisher);
        return;
    }

    if (service == NULL) {
        ERROR("no pending service update");
        return;
    }
    char status_buf[256];
    snprintf(status_buf, sizeof(status_buf), "is in state %s, status = %d",
             thread_service_publication_state_name_get(service->publication_state), status);
    thread_service_note(publisher->id, service, status_buf);
    if (status == kCTIStatus_NoError) {
        if (service->publication_state == add_pending) {
            service->publication_state = add_complete;
        } else if (service->publication_state == delete_pending) {
            service->publication_state = delete_complete;
        }
    } else {
        if (service->publication_state == add_pending) {
            service->publication_state = add_failed;
        } else if (service->publication_state == delete_pending) {
            service->publication_state = delete_failed;
        }
    }
    publisher->publication_queue = service->next;
    thread_service_release(service);
    if (!publisher->canceled) {
        service_publisher_queue_run(publisher);
    }
    RELEASE_HERE(publisher, service_publisher);
}

static cti_status_t
service_publisher_service_update(service_publisher_t *publisher, thread_service_t *service, bool add)
{
    uint8_t service_data[20];
    uint8_t server_data[20];
    size_t service_data_length, server_data_length;
    switch(service->service_type) {
    default:
        return kCTIStatus_Invalid;
    case unicast_service:
        service_data[0] = THREAD_SRP_SERVER_OPTION;
        service_data_length = 1;
        memcpy(server_data, &service->u.unicast.address, 16);
        memcpy(&server_data[16], service->u.unicast.port, 2);
        server_data_length = 18;
        break;
    case anycast_service:
        service_data[0] = THREAD_SRP_SERVER_ANYCAST_OPTION;
        service_data[1] = service->u.anycast.sequence_number;
        service_data_length = 2;
        server_data_length = 0;
        break;
    case pref_id:
        return kCTIStatus_BadParam; // Not supported anymore.
    }

    if (add) {
        return cti_add_service(publisher->server_state, publisher, service_publisher_update_callback, NULL,
                               THREAD_ENTERPRISE_NUMBER, service_data, service_data_length, server_data, server_data_length);
    } else {
        return cti_remove_service(publisher->server_state, publisher, service_publisher_update_callback, NULL,
                                  THREAD_ENTERPRISE_NUMBER, service_data, service_data_length);
    }
}

static void
service_publisher_queue_run(service_publisher_t *publisher)
{
    thread_service_t *service = publisher->publication_queue;
    if (service == NULL) {
        INFO("the queue is empty.");
        return;
    }
    if (service->publication_state == delete_pending || service->publication_state == add_pending) {
        INFO("there is a pending update at the head of the queue.");
        return;
    }
    if (service->publication_state == want_delete) {
        cti_status_t status = service_publisher_service_update(publisher, service, false);
        if (status != kCTIStatus_NoError) {
            ERROR("cti_remove_service failed: %d", status);
            // For removes, we'll leave it on the queue
        } else {
            service->publication_state = delete_pending;
            RETAIN_HERE(publisher, service_publisher); // for the callback
        }
    } else if (service->publication_state == want_add) {
        cti_status_t status = service_publisher_service_update(publisher, service, true);
        if (status != kCTIStatus_NoError) {
            ERROR("cti_add_service failed: %d", status);
            publisher->publication_queue = service->next;
            thread_service_release(service);
        } else {
            service->publication_state = add_pending;
            RETAIN_HERE(publisher, service_publisher); // for the callback
        }
    } else {
        char status_buf[256];
        snprintf(status_buf, sizeof(status_buf), "is in unexpected state %s on the publication queue",
                 thread_service_publication_state_name_get(service->publication_state));
        thread_service_note(publisher->id, service, status_buf);
        publisher->publication_queue = service->next;
        thread_service_release(service);
    }
}

static void
service_publisher_queue_update(service_publisher_t *publisher, thread_service_t *service,
                               thread_service_publication_state_t initial_state)
{
    thread_service_t **ppref;

    thread_service_t **p_published;
    if (service->service_type == unicast_service) {
        p_published = &publisher->published_unicast_service;
    } else if (service->service_type == unicast_service) {
        p_published = &publisher->published_unicast_service;
    } else {
        ERROR("unsupported service type %d", service->service_type);
        return;
    }
    if (thread_service_equal(*p_published, service)) {
        FAULT("published service still present: %p", *p_published);
    }
    service->publication_state = initial_state;
    if (initial_state == want_add) {
        *p_published = service;
        thread_service_retain(*p_published);
    }
    // Find the end of the queue
    for (ppref = &publisher->publication_queue; *ppref != NULL; ppref = &(*ppref)->next)
        ;
    *ppref = service;
    // Retain the service on the queue.
    thread_service_retain(*ppref);
    service_publisher_queue_run(publisher);
}

static thread_service_t *
service_publisher_create_service_for_queue(thread_service_t *service)
{
    switch(service->service_type) {
    case unicast_service:
        return thread_service_unicast_create(service->rloc16, service->u.unicast.address.s6_addr,
                                             service->u.unicast.port, service->service_id);
    case anycast_service:
        return thread_service_anycast_create(service->rloc16, service->u.anycast.sequence_number, service->service_id);
    case pref_id:
        return thread_service_pref_id_create(service->rloc16, service->u.pref_id.partition_id,
                                             service->u.pref_id.prefix, service->service_id);
    default:
        return NULL;
    }
}

static void UNUSED
service_publisher_service_publish(service_publisher_t *publisher, thread_service_t *service)
{
    service_publisher_queue_update(publisher, service, want_add);
}

static void UNUSED
service_publisher_service_unpublish(service_publisher_t *publisher, thread_service_type_t service_type, bool enqueue)
{
    thread_service_t *service;

    if (service_type == unicast_service) {
        service = publisher->published_unicast_service;
        publisher->published_unicast_service = NULL;
    } else if (service_type == anycast_service) {
        service = publisher->published_anycast_service;
        publisher->published_anycast_service = NULL;
    } else {
        ERROR("unsupported service type %d", service_type);
        return;
    }
    if (service == NULL) {
        ERROR("request to unpublished service that's not present");
        return;
    }

    if (enqueue) {
        thread_service_t *to_delete = service_publisher_create_service_for_queue(service);
        service_publisher_queue_update(publisher, to_delete, want_delete);
        thread_service_release(to_delete); // service_publisher_queue_update explicitly retains the references it makes.
        thread_service_release(service); // No longer published.
    }
}

static void
service_publisher_unpublish_stale_service(service_publisher_t *publisher, thread_service_t *service)
{
    // If there's a stale service, don't try to publish the real service until we see the stale service go away.
    INFO("setting seen_service_list to false");
    publisher->seen_service_list = false;

    thread_service_t *to_delete = service_publisher_create_service_for_queue(service);

    if (to_delete == NULL) {
        thread_service_note(publisher->id, service, "no memory for service to delete");
    } else {
        service_publisher_queue_update(publisher, to_delete, want_delete);
        thread_service_release(to_delete); // service_publisher_queue_update explicitly retains all the references it makes
        service->ignore = true;
    }
}

static void
service_publisher_wait_expired(void *context)
{
    service_publisher_t *publisher = context;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_timeout, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
service_publisher_start_wait(service_publisher_t *publisher, int32_t milliseconds)
{
    ioloop_add_wake_event(publisher->wakeup_timer, publisher, service_publisher_wait_expired,
                          service_publisher_context_release, milliseconds);
    RETAIN_HERE(publisher, service_publisher); // For wakeup
}

static bool
service_publisher_have_competing_unicast_service(service_publisher_t *publisher, bool want_stale_service_timeout)
{
    thread_service_t *NULLABLE published_service = publisher->published_unicast_service;
    bool competing_service_present = false;

    for (thread_service_t *service = service_tracker_services_get(publisher->server_state->service_tracker);
         service != NULL; service = service->next)
    {
        if (service->ignore) {
            continue;
        }
        if (service->service_type == unicast_service) {
            if (published_service == NULL) {
                if (service->rloc16 == publisher->server_state->rloc16 ||
                    (publisher->have_ml_eid &&
                     !in6addr_compare(&service->u.unicast.address, &publisher->thread_mesh_local_address)))
                {
                    thread_service_note(publisher->id, service,
                                        "is on our ml-eid or rloc16 but we aren't publishing it, so it's stale.");
                    service_publisher_unpublish_stale_service(publisher, service);

                    if (want_stale_service_timeout &&
                        !publisher->cached_services_published && !publisher->started_stale_service_timeout)
                    {
                        INFO("starting wakeup timer to publish cached services after stale service timeout.");
                        publisher->started_stale_service_timeout = true;
                        service_publisher_start_wait(publisher, 2000);
                    }
                    continue;
                }
                thread_service_note(publisher->id, service, "is not ours and we aren't publishing.");
                competing_service_present = true;
            // First check to see if the other service is on the mesh-local prefix. If it's not, it wins.
            } else if (in6prefix_compare(&service->u.unicast.address, &published_service->u.unicast.address, 8)) {
                competing_service_present = true;
                thread_service_note(publisher->id, service, "is a competing service.");
            } else {
                int cmp = in6addr_compare(&service->u.unicast.address, &published_service->u.unicast.address);

                // If equal, compare ports...
                if (!cmp) {
                    cmp = memcmp(service->u.unicast.port, published_service->u.unicast.port,
                                 sizeof(service->u.unicast.port));
                    // If the port doesn't match our published service, it's a weird stale service (this probably can't
                    // happen);
                    if (cmp) {
                        thread_service_note(publisher->id, service,
                                            "is on our ml-eid but is not the one we are publishing, so it's stale.");
                        service_publisher_unpublish_stale_service(publisher, service);
                        continue;
                    } else {
                        thread_service_note(publisher->id, service, "is the one we are publishing.");
                    }
                } else {
                    // This is a stale service published on our RLOC16 with a different ML-EID.
                    if (service->rloc16 == publisher->server_state->rloc16) {
                        thread_service_note(publisher->id, service,
                                            "is a stale service published on our rloc16 with a different ml-eid.");
                        service_publisher_unpublish_stale_service(publisher, service);
                        continue;
                    } else if (cmp < 0) {
                        competing_service_present = true;
                        thread_service_note(publisher->id, service, "is not ours and wins against ours.");
                    } else {
                        thread_service_note(publisher->id, service, "is not ours and loses against ours.");
                    }
                }
            }
        }
    }
    return competing_service_present;
}

static bool
service_publisher_have_anycast_service(service_publisher_t *publisher)
{
    bool anycast_service_present = false;

    for (thread_service_t *service = service_tracker_services_get(publisher->server_state->service_tracker);
         service != NULL; service = service->next)
    {
        if (service->ignore) {
            continue;
        }
        if (service->service_type == anycast_service) {
            thread_service_note(publisher->id, service, "is present and supersedes our unicast service");
            anycast_service_present = true;
        }
    }
    return anycast_service_present;
}

void
service_publisher_wanted_service_added(service_publisher_t *publisher)
{
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_srp_needed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
service_publisher_sed_timeout_expired(void *context)
{
    service_publisher_t *publisher = context;

    if (publisher->sed_timeout != NULL) {
        ioloop_wakeup_release(publisher->sed_timeout);
        publisher->sed_timeout = NULL;
    }
    if (publisher->neighbor_ml_eid_string == NULL) {
        publisher->neighbor_ml_eid_string = strdup("none");
        memset(&publisher->neighbor_ml_eid, 0, sizeof(publisher->neighbor_ml_eid));
    }
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_neighbor_ml_eid_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
    } else {
        state_machine_event_deliver(&publisher->state_header, event);
        RELEASE_HERE(event, state_machine_event);
    }
}

static void
service_publisher_service_tracker_callback(void *context)
{
    service_publisher_t *publisher = context;

    // If we get a service list update when we're associated, set the seen_service-list flag to true. This tells us we can proceed
    // with publishing a service if we just associated to the thread network.
    if (thread_tracker_associated_get(publisher->server_state->thread_tracker, false)) {
        INFO("setting seen_service_list to true");
        publisher->seen_service_list = true;
    }

    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_service_list_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
service_publisher_thread_tracker_callback(void *context)
{
    service_publisher_t *publisher = context;

    // This is a temporary hack because right now we won't get a prefix list event on associate, and we need
    // the (presumably empty) prefix list event to trigger an immediate service advertisement.
    // IMPORTANT: when this is fixed, make sure to start the service_tracker in thread-device.c again.
    // To work around this issue, we stop service list events in the service tracker when we dissociate, and
    // restart events when we re-associate.
    // Hack is tracked in rdar://109266343 (Optimize SRP registration time)
    if (!thread_tracker_associated_get(publisher->server_state->thread_tracker, false)) {
        service_tracker_stop(publisher->server_state->service_tracker);
    } else {
        service_tracker_start(publisher->server_state->service_tracker);
    }

    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_thread_network_state_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

static void
service_publisher_node_type_tracker_callback(void *context)
{
    service_publisher_t *publisher = context;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_thread_node_type_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

// Service publisher states
//
// <startup>
//      on entry: start timer for a random amount of time
//      timer event: -> <waiting to publish>
//      other events: ignore
// <waiting to publish>
//      on entry: check for service present
//                yes: -> <not publishing>
//                check for ML-EID present
//                yes: -> <start listeners>
//      ML-EID shows up: -> <start listeners>
//      service shows up: -> <not publishing>
// <not publishing>
//      on entry: do nothing
//      last service advertisement goes away: -> <waiting to publish>
// <start listeners>
//      on entry: start listeners
//      listener ready: -> <publishing>
// <publishing>
//      on entry: publish the service
//                start wait timer
//      on timery expiry: publish the service again
//      our service shows up: stop the timer
//      winning service shows up: stop advertising
//                                -> <not publishing>

static state_machine_state_t service_publisher_action_startup(state_machine_header_t *state_header,
                                                              state_machine_event_t *event);
static state_machine_state_t service_publisher_action_waiting_to_publish(state_machine_header_t *state_header,
                                                                         state_machine_event_t *event);
static state_machine_state_t service_publisher_action_not_publishing(state_machine_header_t *state_header,
                                                                 state_machine_event_t *event);
static state_machine_state_t service_publisher_action_start_listeners(state_machine_header_t *state_header,
                                                                      state_machine_event_t *event);
static state_machine_state_t service_publisher_action_publishing(state_machine_header_t *state_header,
                                                                 state_machine_event_t *event);

#define SERVICE_PUB_NAME_DECL(name) service_publisher_state_##name, #name
static state_machine_decl_t service_publisher_states[] = {
    { SERVICE_PUB_NAME_DECL(invalid),                            NULL },
    { SERVICE_PUB_NAME_DECL(startup),                            service_publisher_action_startup },
    { SERVICE_PUB_NAME_DECL(waiting_to_publish),                 service_publisher_action_waiting_to_publish },
    { SERVICE_PUB_NAME_DECL(not_publishing),                     service_publisher_action_not_publishing },
    { SERVICE_PUB_NAME_DECL(start_listeners),                    service_publisher_action_start_listeners },
    { SERVICE_PUB_NAME_DECL(publishing),                         service_publisher_action_publishing },
};
#define SERVICE_PUBLISHER_NUM_STATES ((sizeof(service_publisher_states)) / (sizeof(state_machine_decl_t)))

#define STATE_MACHINE_HEADER_TO_PUBLISHER(state_header)                                                                \
    if (state_header->state_machine_type != state_machine_type_service_publisher) {                                    \
        ERROR("state header type isn't service_publisher: %d", state_header->state_machine_type);                      \
        return service_publisher_state_invalid;                                                                        \
    }                                                                                                                  \
    service_publisher_t *publisher = state_header->state_object

// In the startup state, we wait for a timeout to expire before doing anything. This gives other devices on the Thread mesh
// an opportunity to publish as well.
static state_machine_state_t
service_publisher_action_startup(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        // We only need a random startup delay for stub routers, which are generally powered devices that can synchronize
        // on restart after a power failure.
#if STUB_ROUTER
        if (publisher->server_state->stub_router_enabled) {
            service_publisher_start_wait(publisher, publisher->startup_delay_range + srp_random16() % publisher->startup_delay_range);
            RETAIN_HERE(publisher, service_publisher); // For wakeup
            return service_publisher_state_invalid;
        }
#endif
        return service_publisher_state_waiting_to_publish;
    }

    // The only way out of the startup state is for the timer to expire--we don't care about prefixes showing up or
    // going away.
    if (event->type == state_machine_event_type_timeout) {
        INFO("startup timeout");
        return service_publisher_state_waiting_to_publish;
    }

    BR_UNEXPECTED_EVENT(publisher, event);
}

static bool
service_publisher_can_publish(service_publisher_t *publisher)
{
    bool no_anycast_service = true;
    bool no_competing_service = true;
    bool associated = true;
    bool router = false;
    bool have_ml_eid = true;
    bool have_thread_interface_name = true;
    bool can_publish = true;
    bool sleepy_router = false;
    bool sleepy_end_device = false;
    bool have_node_type = false;
    bool have_wed_ml_eid = true;
    bool have_neighbor_ml_eid = true;

    // Check the conditions that prevent publication.
    if (service_publisher_have_competing_unicast_service(publisher, false)) {
        no_competing_service = false;
        can_publish = false;
    }
    if (service_publisher_have_anycast_service(publisher)) {
        no_anycast_service = false;
        can_publish = false;
    }
    srp_server_t *server_state = publisher->server_state;
    if (!thread_tracker_associated_get(server_state->thread_tracker, false)) {
        associated = false;
        can_publish = false;
        INFO("setting seen_service_list to false");
        publisher->seen_service_list = false;
    }

    thread_node_type_t node_type = node_type_tracker_thread_node_type_get(server_state->node_type_tracker, false);
    switch(node_type) {
    case node_type_router:
    case node_type_leader:
        router = true;
        have_node_type = true;
        break;
    case node_type_sleepy_router:
        sleepy_router = true;
        have_node_type = true;
        break;
    case node_type_unknown:
        have_node_type = false;
        can_publish = false;
        break;
    case node_type_sleepy_end_device:
    case node_type_synchronized_sleepy_end_device:
        sleepy_end_device = true;
        have_node_type = true;
        break;
    default:
        have_node_type = true;
        break;
    }

    if (!publisher->have_ml_eid) {
        have_ml_eid = false;
        can_publish = false;
    }
    if (!publisher->have_thread_interface_name) {
        have_thread_interface_name = false;
        can_publish = false;
    }
    if (!publisher->seen_service_list) {
        can_publish = false;
    }
    if (publisher->stopped) {
        can_publish = false;
    }
    if (publisher->wed_ml_eid_string == NULL) {
        have_wed_ml_eid = false;
        if (sleepy_router) {
            can_publish = false;
        }
    }
    if (publisher->neighbor_ml_eid_string == NULL) {
        have_neighbor_ml_eid = false;
        if (sleepy_end_device) {
            if (publisher->sed_timeout == NULL) {
                publisher->sed_timeout = ioloop_wakeup_create();
                if (publisher->sed_timeout != NULL) {
                    ioloop_add_wake_event(publisher->sed_timeout, publisher, service_publisher_sed_timeout_expired,
                                          service_publisher_context_release, 500);
                    RETAIN_HERE(publisher, service_publisher);
                }
            }
            can_publish = false;
        }
    }

    INFO(PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP PUB_S_SRP,
         can_publish ?                         "can publish" :  "can't publish",
         publisher->seen_service_list ?                   "" : " have not seen service list",
         no_competing_service ?                           "" : " competing service present",
         no_anycast_service ?                             "" : " anycast service present",
         associated ?                                     "" : " not associated ",
         router ?                                         "" : " not a router ",
         sleepy_router ?                                  "" : " not a sleepy router",
         sleepy_end_device ?                              "" : " not a sleepy end device",
         have_node_type ?                                 "" : " don't have node type",
         have_ml_eid ?                                    "" : " no ml-eid ",
         have_wed_ml_eid ?                                "" : " no wed ml-eid ",
         have_neighbor_ml_eid ?                           "" : " no neighbor ml-eid ",
         have_thread_interface_name ?                     "" : " no thread interface name ",
         publisher->stopped ?                     " stopped" : "");
    return can_publish;
}

// This function tells the caller whether the service publisher could publish a service. This will be the case either
// because it actually is publishing a service, or because it's in the process of finding out if it can publish a
// service, or preparing to publish a service. In practice, this means that the only state for which the answer is false
// at present is the "not_publishing" state.
bool
service_publisher_could_publish(service_publisher_t *publisher)
{
    if (publisher == NULL) {
        return false;
    }
    if (publisher->state_header.state == service_publisher_state_startup ||
        publisher->state_header.state == service_publisher_state_waiting_to_publish ||
        publisher->state_header.state == service_publisher_state_start_listeners ||
        publisher->state_header.state == service_publisher_state_publishing)
    {
        return true;
    }
    return false;
}

void
service_publisher_stop_publishing(service_publisher_t *publisher)
{
    publisher->stopped = true;
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_stop, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
}

// We go to this state whenever we think we might need to publish, but are not yet publishing. So this state
// acts as a gatekeeper: if there is already a service published, we go straight to not_publishing. If we don't
// yet have an ML-EID, we have to wait until we get one to publish. If, while we are waiting for the ML-EID,
// we see a service show up, we go to not_publishing. Otherwise, when the ML-EID comes, we go to the listener_start
// page.
static state_machine_state_t
service_publisher_action_waiting_to_publish(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    // We do the same thing here whether we've gotten an event or just on entry, so no need to check.
    if (service_publisher_can_publish(publisher)) {
        ioloop_cancel_wake_event(publisher->wakeup_timer);
        publisher->started_stale_service_timeout = false;
        return service_publisher_state_start_listeners;
    }
    if (service_publisher_have_competing_unicast_service(publisher, true)) {
        ioloop_cancel_wake_event(publisher->wakeup_timer);
        publisher->started_stale_service_timeout = false;
        return service_publisher_state_not_publishing;
    }
    // If we saw a stale service, we'll get a timeout event here.
    if (event != NULL && event->type == state_machine_event_type_timeout && !publisher->cached_services_published) {
        service_publisher_re_advertise_matching(publisher);
    }
    return service_publisher_state_invalid;
}

// We get into this state when there is a competing service that wins the election against our service.
// We only leave the state when there is no longer a competing prefix.
static state_machine_state_t
service_publisher_action_not_publishing(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        if (publisher->published_unicast_service != NULL) {
            service_publisher_service_unpublish(publisher, unicast_service, true);
        }
        return service_publisher_state_invalid;
    }

    // We do the same thing here for any event we get, because we're just waiting for conditions to be right.
    if (service_publisher_can_publish(publisher)) {
        publisher->startup_delay_range = SERVICE_PUBLISHER_LOST_WAIT;
        return service_publisher_state_startup;
    }
    return service_publisher_state_invalid;
}

static void
service_publisher_listener_cancel_callback(comm_t *UNUSED listener, void *context)
{
    srp_server_t *server_state = context;
    service_publisher_t *publisher = server_state->service_publisher;
    if (publisher != NULL) {
        state_machine_event_t *event = state_machine_event_create(state_machine_event_type_listener_canceled, NULL);
        if (event == NULL) {
            ERROR("unable to allocate event to deliver");
            return;
        }
        state_machine_event_deliver(&publisher->state_header, event);
        RELEASE_HERE(event, state_machine_event);
    }
}

static void
service_publisher_listener_cancel(service_publisher_t *publisher)
{
    if (publisher->srp_listener != NULL) {
        ioloop_listener_cancel(publisher->srp_listener);
        ioloop_comm_release(publisher->srp_listener);
        publisher->srp_listener = NULL;
    }
    publisher->have_srp_listener = false;
    service_publisher_unadvertise_all(publisher);
}

static void
service_publisher_listener_ready(void *context, uint16_t port)
{
    srp_server_t *server_state = context;
    service_publisher_t *publisher = server_state->service_publisher;
    if (publisher != NULL) {
        state_machine_event_t *event = state_machine_event_create(state_machine_event_type_listener_ready, NULL);
        if (event == NULL) {
            ERROR("unable to allocate event to deliver");
            return;
        }
        publisher->have_srp_listener = true;
        publisher->srp_listener_port = port;
        state_machine_event_deliver(&publisher->state_header, event);
        RELEASE_HERE(event, state_machine_event);
    }
}

static void
service_publisher_listener_start(service_publisher_t *publisher)
{
    if (publisher->srp_listener) {
        FAULT("listener still present");
        service_publisher_listener_cancel(publisher);
    }
    publisher->srp_listener = srp_proxy_listen(NULL, 0, publisher->thread_interface_name, service_publisher_listener_ready,
                                               service_publisher_listener_cancel_callback, NULL,
                                               NULL, publisher->server_state);
    if (publisher->srp_listener == NULL) {
        ERROR("failed to setup SRP listener");
    }
    service_publisher_re_advertise_matching(publisher);
}

// We go to this state when we have decided to publish, but perhaps do not currently have an SRP listener
// running.
static state_machine_state_t
service_publisher_action_start_listeners(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL) {
        if (publisher->have_srp_listener) {
            if (publisher->srp_listener != NULL) {
                return service_publisher_state_publishing;
            }
            FAULT("have_srp_listener is true but there's no listener!");
            publisher->have_srp_listener = false;
        }
        service_publisher_listener_start(publisher);
        return service_publisher_state_invalid;
    }

    // If we get a competing service while we're waiting for the listener to start, cancel the listener.
    // We do the same thing here for any event we get, because we're just waiting for conditions to be right.
    if (!service_publisher_can_publish(publisher)) {
        service_publisher_listener_cancel(publisher);
        return service_publisher_state_not_publishing;
    }

    // If the listener is ready, we can publish.
    if (event->type == state_machine_event_type_listener_ready) {
        return service_publisher_state_publishing;
    }

    return service_publisher_state_invalid;
}

static state_machine_state_t
service_publisher_published_services_seen(service_publisher_t *publisher)
{
    return ((publisher->published_unicast_service == NULL || publisher->have_unicast_in_net_data) &&
            (publisher->published_anycast_service == NULL || publisher->have_anycast_in_net_data));
}

static bool
service_publisher_wanted_service_missing(service_publisher_t *publisher)
{
    srp_server_t *server_state = publisher->server_state;

        // If we get here, the named service instance is not represented in the current set of services
        // about which we have information.
#if STUB_ROUTER
    if (server_state->stub_router_enabled) {
        return true; // always publish when stub router
    }
#endif
    if (server_state->srp_service_needed) {
        INFO("srp_service_needed == true -> true");
        return true; // srp service unconditionally requested
    }

    for (wanted_service_t *service = server_state->wanted_services; service != NULL; service = service->next) {
        for (adv_host_t *host = server_state->hosts; host != NULL; host = host->next) {
            if (host->instances != NULL) {
                for (int i = 0; i < host->instances->num; i++) {
                    adv_instance_t *instance = host->instances->vec[i];
                    if (instance != NULL) {
                        if (!strcasecmp(instance->instance_name, service->name) &&
                            !adv_ctl_service_types_compare(service->service_type, instance->service_type))
                        {
                            if (host->addresses != NULL) {
                                for (int j = 0; j < host->addresses->num; j++) {
                                    adv_record_t *address = host->addresses->vec[j];
                                    if (address != NULL) {
                                        if (address->rdlen == 16 &&
                                            !in6prefix_compare((struct in6_addr *)address->rdata,
                                                               &publisher->thread_mesh_local_address, 8))
                                        {
                                            goto instance_found;
                                        }
                                    }
                                }
                                INFO("srp service " PRI_S_SRP "." PRI_S_SRP " present as " PRI_S_SRP
                                     " but has no address on local mesh -> true",
                                     service->name, service->service_type, instance->instance_name);
                                return true; // There is no valid address for this instance in the cache.
                            }
                            INFO("srp service " PRI_S_SRP "." PRI_S_SRP " present but no addresses -> true",
                                 service->name, service->service_type);
                            return true; // We didn't find the named instance in the database
                        }
                    }
                }
            }
        }
        INFO("service " PRI_S_SRP "." PRI_S_SRP " host not present -> true", service->name, service->service_type);
        return true;
    instance_found:
        INFO("service " PRI_S_SRP "." PRI_S_SRP " is present", service->name, service->service_type);
    }
    INFO("all needed services present -> false");
    return false; // There weren't any named instances for which we don't have a usable registration.
}

// We enter this state when we have an SRP listener and no competing unicast services. On entry, we publish our unicast service.
// If a competing service shows up that wins, we stop publishing and cancel the listener. Otherwise we remain in this state.
static state_machine_state_t
service_publisher_action_publishing(state_machine_header_t *state_header, state_machine_event_t *event)
{
    STATE_MACHINE_HEADER_TO_PUBLISHER(state_header);
    BR_STATE_ANNOUNCE(publisher, event);

    if (event == NULL || event->type == state_machine_event_type_timeout ||
        event->type == state_machine_event_type_srp_needed)
    {
        if (publisher->published_unicast_service != NULL) {
            // We shouldn't see a published service on state entry.
            if (event == NULL) {
                ERROR("unicast service still published!");
            }
            // Only actually enqueue a delete if we aren't retrying.
            service_publisher_service_unpublish(publisher, unicast_service, event == NULL);
        }

        // On non-BR devices, don't actually publish the service until we get a signal that it's needed.
        if (!publisher->server_state->srp_on_demand || service_publisher_wanted_service_missing(publisher)) {
            uint8_t port[] = { publisher->srp_listener_port >> 8, publisher->srp_listener_port & 255 };
            thread_service_t *service = thread_service_unicast_create(publisher->server_state->rloc16,
                                                                      (uint8_t *)&publisher->thread_mesh_local_address,
                                                                      port, 0);
            service_publisher_service_publish(publisher, service);
            thread_service_release(service); // service_publisher_publish retains the references it keeps.

            // Set up a retransmit timer in case the service publication fails.
            if (event == NULL) {
                publisher->retry_interval = 5; // First retry after five seconds
            } else {
                // Maybe the service tracker is wedged, so restart it
                service_tracker_start(publisher->server_state->service_tracker);

                // Exponential backoff.
                if (publisher->retry_interval < 3600) {
                    publisher->retry_interval *= 2;
                }
            }
            service_publisher_start_wait(publisher, (publisher->retry_interval * MSEC_PER_SEC +
                                                     srp_random32() % (publisher->retry_interval * MSEC_PER_SEC) / 2));
        }

        return service_publisher_state_invalid;
    }

    // If the listener got canceled for some reason, restart it.
    if (event->type == state_machine_event_type_listener_canceled) {
        service_publisher_service_unpublish(publisher, unicast_service, true);
        publisher->startup_delay_range = SERVICE_PUBLISHER_LISTENER_RESTART_WAIT;
        return service_publisher_state_startup;
    }

    // Any other event triggers a re-evaluation.
    if (event->type == state_machine_event_type_ml_eid_changed) {
        service_publisher_listener_cancel(publisher);
        service_publisher_service_unpublish(publisher, unicast_service, true);
        return service_publisher_state_startup;
    }

    if (!service_publisher_can_publish(publisher)) {
        service_publisher_listener_cancel(publisher);
        service_publisher_service_unpublish(publisher, unicast_service, true);
        return service_publisher_state_not_publishing;
    }

    // If we haven't yet seen all the services we are publishing in the network data, check to see if it showed up.
    if (event->type == state_machine_event_type_service_list_changed &&
        !service_publisher_published_services_seen(publisher))
    {
        publisher->have_unicast_in_net_data = false;
        publisher->have_anycast_in_net_data = false;
        for (thread_service_t *service = service_tracker_services_get(publisher->server_state->service_tracker);
             service != NULL; service = service->next)
        {
            if (service->service_type == unicast_service && service->ncp &&
                publisher->published_unicast_service != NULL &&
                !in6addr_compare(&service->u.unicast.address, &publisher->published_unicast_service->u.unicast.address) &&
                !memcmp(service->u.unicast.port, publisher->published_unicast_service->u.unicast.port, 2))
            {
                publisher->have_unicast_in_net_data = true;
            }
            else if (service->service_type == anycast_service && service->ncp &&
                       publisher->server_state->have_rloc16 && service->rloc16 == publisher->server_state->rloc16 &&
                       publisher->published_anycast_service != NULL &&
                       service->u.anycast.sequence_number == publisher->published_anycast_service->u.anycast.sequence_number)
            {
                publisher->have_anycast_in_net_data = true;
            }
        }

        // If all of the services we are publishing are showing up in NCP, we can cancel the timer and go to the publishing state.
        if (service_publisher_published_services_seen(publisher)) {
            ioloop_cancel_wake_event(publisher->wakeup_timer);
            return service_publisher_state_invalid;
        }
    }
    return service_publisher_state_invalid;
}

void
service_publisher_cancel(service_publisher_t *publisher)
{
    ioloop_cancel_wake_event(publisher->wakeup_timer);
    service_publisher_listener_cancel(publisher);
    service_tracker_callback_cancel(publisher->server_state->service_tracker, publisher);
    thread_tracker_callback_cancel(publisher->server_state->thread_tracker, publisher);
    node_type_tracker_callback_cancel(publisher->server_state->node_type_tracker, publisher);
    if (publisher->active_data_set_connection != NULL) {
        cti_events_discontinue(publisher->active_data_set_connection);
        publisher->active_data_set_connection = NULL;
        RELEASE_HERE(publisher, service_publisher);
    }
    if (publisher->wed_tracker_connection != NULL) {
        cti_events_discontinue(publisher->wed_tracker_connection);
        publisher->wed_tracker_connection = NULL;
        RELEASE_HERE(publisher, service_publisher);
    }
    if (publisher->neighbor_tracker_connection != NULL) {
        cti_events_discontinue(publisher->neighbor_tracker_connection);
        publisher->neighbor_tracker_connection = NULL;
        RELEASE_HERE(publisher, service_publisher);
    }
    state_machine_cancel(&publisher->state_header);
}

service_publisher_t *
service_publisher_create(srp_server_t *server_state)
{
    service_publisher_t *ret = NULL, *publisher = calloc(1, sizeof(*publisher));
    if (publisher == NULL) {
        return publisher;
    }
    RETAIN_HERE(publisher, service_publisher);
    publisher->wakeup_timer = ioloop_wakeup_create();
    if (publisher->wakeup_timer == NULL) {
        ERROR("wakeup timer alloc failed");
        goto out;
    }

    char server_id_buf[100];
    snprintf(server_id_buf, sizeof(server_id_buf), "[SP%lld]", ++service_publisher_serial_number);
    publisher->id = strdup(server_id_buf);
    if (publisher->id == NULL) {
        ERROR("no memory for server ID");
        goto out;
    }

    if (!state_machine_header_setup(&publisher->state_header,
                                    publisher, publisher->id,
                                    state_machine_type_service_publisher,
                                    service_publisher_states,
                                    SERVICE_PUBLISHER_NUM_STATES)) {
        ERROR("header setup failed");
        goto out;
    }

    publisher->server_state = server_state;
    if (!service_tracker_callback_add(server_state->service_tracker, service_publisher_service_tracker_callback,
                                      service_publisher_context_release, publisher))
    {
        goto out;
    }
    RETAIN_HERE(publisher, service_publisher); // for service tracker

    if (!thread_tracker_callback_add(server_state->thread_tracker, service_publisher_thread_tracker_callback,
                                     service_publisher_context_release, publisher))
    {
        goto out;
    }
    RETAIN_HERE(publisher, service_publisher); // for thread network state tracker

    if (!node_type_tracker_callback_add(server_state->node_type_tracker, service_publisher_node_type_tracker_callback,
                                        service_publisher_context_release, publisher))
    {
        goto out;
    }
    RETAIN_HERE(publisher, service_publisher); // for thread network state tracker

    // Set the first_time flag so that we'll know to remove any locally-published on-mesh prefixes.
    publisher->first_time = true;
    publisher->startup_delay_range = SERVICE_PUBLISHER_START_WAIT;
    ret = publisher;
    publisher = NULL;
out:
    if (publisher != NULL) {
        RELEASE_HERE(publisher, service_publisher);
    }
    return ret;
}

static void
service_publisher_get_mesh_local_address_callback(void *context, const char *address_string, cti_status_t status)
{
    service_publisher_t *publisher = context;

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        if (publisher->reconnect_callback != NULL) {
            publisher->reconnect_callback(publisher->server_state);
        }
        goto fail;
    }

    INFO(PUB_S_SRP " %d", address_string != NULL ? address_string : "<null>", status);
    if (status != kCTIStatus_NoError || address_string == NULL) {
        goto fail;
    }

    struct in6_addr new_mesh_local_address;
    if (!inet_pton(AF_INET6, address_string, &new_mesh_local_address)) {
        ERROR("address syntax incorrect: " PRI_S_SRP, address_string);
        goto fail;
    }

    if (publisher->have_ml_eid && !in6addr_compare(&new_mesh_local_address, &publisher->thread_mesh_local_address)) {
        INFO("address didn't change");
        return;
    }
    publisher->thread_mesh_local_address = new_mesh_local_address;
    publisher->have_ml_eid = true;

    for (thread_service_t *service = service_tracker_services_get(publisher->server_state->service_tracker);
         service != NULL; service = service->next)
    {
        if (service->ignore) {
            continue;
        }
        if (service->service_type == unicast_service) {
            if (publisher->published_unicast_service == NULL) {
                if (service->rloc16 == publisher->server_state->rloc16 ||
                    (publisher->have_ml_eid &&
                     !in6addr_compare(&service->u.unicast.address, &publisher->thread_mesh_local_address)))
                {
                    thread_service_note(publisher->id, service,
                                        "is on our ml-eid or rloc16 but we aren't publishing it, so it's stale.");
                    service_publisher_unpublish_stale_service(publisher, service);
                    continue;
                }
            }
        }
    }

    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_ml_eid_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        return;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
    RELEASE_HERE(publisher, service_publisher); // callback held a reference.
    return;
fail:
    RELEASE_HERE(publisher, service_publisher); // callback held a reference.
    publisher->have_ml_eid = false;
    return;
}

static void
service_publisher_active_data_set_changed_callback(void *context, cti_status_t status)
{
    service_publisher_t *publisher = context;

    if (status != kCTIStatus_NoError) {
        ERROR("error %d", status);
        RELEASE_HERE(publisher, service_publisher); // no more callbacks
        cti_events_discontinue(publisher->active_data_set_connection);
        publisher->active_data_set_connection = NULL;
        return;
    }

    status = cti_get_mesh_local_address(publisher->server_state, publisher,
                                        service_publisher_get_mesh_local_address_callback, NULL);
    if (status != kCTIStatus_NoError) {
        ERROR("cti_get_mesh_local_address failed with status %d", status);
    } else {
        RETAIN_HERE(publisher, service_publisher); // for mesh-local callback
    }
}

static void
service_publisher_tunnel_name_callback(void *context, const char *name, cti_status_t status)
{
    service_publisher_t *publisher = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        goto out;
    }

    if (status != kCTIStatus_NoError) {
        INFO(PUB_S_SRP " %d", name != NULL ? name : "<null>", status);
        goto out;
    }
    publisher->have_thread_interface_name = true;

    // Get rid of the old interface name if it's changed.
    bool changed = true;
    if (publisher->thread_interface_name != NULL) {
        if (!strcmp(name, publisher->thread_interface_name)) {
            changed = false;
        } else {
            free(publisher->thread_interface_name);
            publisher->thread_interface_name = NULL;
        }
    }

    // Store the new interface name if it's changed.
    if (changed) {
        publisher->thread_interface_name = strdup(name);

        INFO("thread interface at " PUB_S_SRP, name);
    }

    if (publisher->thread_interface_name == NULL) {
        ERROR("No memory to save thread interface name " PUB_S_SRP, name);
    }

    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_thread_interface_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        goto out;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
out:
    RELEASE_HERE(publisher, service_publisher); // callback held a reference.
}

static void
service_publisher_wed_callback(void *context, const char *ext_address, const char *ml_eid, bool added, int status)
{
    service_publisher_t *publisher = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        goto out;
    }

    const char *none = "<none>";
    const char *ea = none;
    if (ext_address != NULL) {
        ea = ext_address;
    }
    const char *mle = none;
    if (ml_eid != NULL) {
        int ret = inet_pton(AF_INET6, ml_eid, &publisher->wed_ml_eid);
        if (ret) {
            mle = ml_eid;
        }
    }

    INFO("ext_address: " PRI_S_SRP "  ml_eid: " PRI_S_SRP PUB_S_SRP " %d", ea, mle, added ? " added" : " removed", status);
    if (status != kCTIStatus_NoError) {
        goto out;
    }

    if (publisher->wed_ext_address_string != NULL) {
        free(publisher->wed_ext_address_string);
        publisher->wed_ext_address_string = NULL;
    }

    if (publisher->wed_ml_eid_string != NULL) {
        free(publisher->wed_ml_eid_string);
        publisher->wed_ml_eid_string = NULL;
    }

    // API guarantees addresses are non-NULL if added is true.
    if (added) {
        if (ea != none) {
            publisher->wed_ext_address_string = strdup(ea);
            if (publisher->wed_ext_address_string == NULL) {
                ERROR("no memory for wed_ext_address string!");
            }
        }
        if (mle != none) {
            publisher->wed_ml_eid_string = strdup(mle);
            if (publisher->wed_ml_eid_string == NULL) {
                ERROR("no memory for wed_ml_eid string!");
                memset(&publisher->wed_ml_eid, 0, sizeof(publisher->wed_ml_eid));
            }
        } else {
            memset(&publisher->wed_ml_eid, 0, sizeof(publisher->wed_ml_eid));
        }
    }
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_wed_ml_eid_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        goto out;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);
out:
    ;
}

static void
service_publisher_neighbor_callback(void *context, const char *ml_eid, cti_status_t status)
{
    service_publisher_t *publisher = context;
    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("disconnected");
        goto out;
    }

    const char *none = "<none>";
    const char *mle = none;
    if (ml_eid != NULL) {
        if (!strcmp(ml_eid, "none")) {
            mle = ml_eid;
            memset(&publisher->neighbor_ml_eid, 0, sizeof(publisher->neighbor_ml_eid));
        } else {
            int ret = inet_pton(AF_INET6, ml_eid, &publisher->neighbor_ml_eid);
            if (ret) {
                mle = ml_eid;
            }
        }
    }

    INFO("ml_eid: " PRI_S_SRP ", status %d", mle, status);
    if (status != kCTIStatus_NoError) {
        goto out;
    }


    if (publisher->neighbor_ml_eid_string != NULL) {
        free(publisher->neighbor_ml_eid_string);
        publisher->neighbor_ml_eid_string = NULL;
    }

    if (mle != none) {
        publisher->neighbor_ml_eid_string = strdup(mle);
        if (publisher->neighbor_ml_eid_string == NULL) {
            ERROR("no memory for neighbor_ml_eid string!");
            memset(&publisher->neighbor_ml_eid, 0, sizeof(publisher->neighbor_ml_eid));
        }
    } else {
        memset(&publisher->neighbor_ml_eid, 0, sizeof(publisher->neighbor_ml_eid));
    }
    state_machine_event_t *event = state_machine_event_create(state_machine_event_type_neighbor_ml_eid_changed, NULL);
    if (event == NULL) {
        ERROR("unable to allocate event to deliver");
        goto out;
    }
    state_machine_event_deliver(&publisher->state_header, event);
    RELEASE_HERE(event, state_machine_event);

    if (publisher->sed_timeout != NULL) {
        ioloop_cancel_wake_event(publisher->sed_timeout);
        ioloop_wakeup_release(publisher->sed_timeout);
        publisher->sed_timeout = NULL;
    }
out:
    ;
}

void
service_publisher_start(service_publisher_t *publisher)
{
    cti_status_t status = cti_track_active_data_set(publisher->server_state, &publisher->active_data_set_connection,
                                                    publisher, service_publisher_active_data_set_changed_callback,
                                                    NULL);
    if (status != kCTIStatus_NoError) {
        ERROR("unable to start tracking active dataset: %d", status);
    } else {
        RETAIN_HERE(publisher, service_publisher); // for active dataset callback
    }

    status = cti_get_tunnel_name(publisher->server_state, publisher, service_publisher_tunnel_name_callback, NULL);
    if (status != kCTIStatus_NoError) {
        ERROR("unable to get tunnel name: %d", status);
    } else {
        RETAIN_HERE(publisher, service_publisher); // for tunnel name callback
    }


    status = cti_track_wed_status(publisher->server_state, &publisher->wed_tracker_connection,
                                  publisher, service_publisher_wed_callback, NULL);
    if (status != kCTIStatus_NoError) {
        FAULT("can't track WED status: %d", status);
    } else {
        RETAIN_HERE(publisher, service_publisher);
    }

    status = cti_track_neighbor_ml_eid(publisher->server_state, &publisher->neighbor_tracker_connection,
                                       publisher, service_publisher_neighbor_callback, NULL);
    if (status != kCTIStatus_NoError) {
        FAULT("can't track WED status: %d", status);
    } else {
        RETAIN_HERE(publisher, service_publisher);
    }

    service_publisher_active_data_set_changed_callback(publisher, kCTIStatus_NoError); // Get the initial state.
    state_machine_next_state(&publisher->state_header, service_publisher_state_startup);
}

bool
service_publisher_get_ml_eid(service_publisher_t *publisher, struct in6_addr *ml_eid)
{
    if (publisher != NULL && publisher->have_ml_eid) {
        in6addr_copy(ml_eid, &publisher->thread_mesh_local_address);
        return true;
    }
    return false;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
