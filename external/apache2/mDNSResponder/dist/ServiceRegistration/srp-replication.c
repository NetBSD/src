/* srp-replication.c
 *
 * Copyright (c) 2020-2023 Apple Inc. All rights reserved.
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
 * This file contains an implementation of SRP Replication, which allows two or more
 * SRP servers to cooperatively maintain an SRP registration dataset.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
#include <math.h>
#include <CoreUtils/CoreUtils.h>

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
#define DNSMessageHeader dns_wire_t
#include "dso.h"
#include "dso-utils.h"

#if SRP_FEATURE_REPLICATION
#include "srp-replication.h"
#ifdef SRP_TEST_SERVER
#include "test-packet.h"
#include "test-srpl.h"
#endif


#define SRPL_CONNECTION_IS_CONNECTED(connection) ((connection)->state > srpl_state_connecting)

#define srpl_event_content_type_set(event, content_type) \
    srpl_event_content_type_set_(event, content_type, __FILE__, __LINE__)
static bool srpl_event_content_type_set_(srpl_event_t *event,
                                         srpl_event_content_type_t content_type, const char *file, int line);
static srpl_state_t srpl_connection_drop_state_delay(srpl_instance_t *instance,
                                                     srpl_connection_t *srpl_connection, int delay);
static srpl_state_t srpl_connection_drop_state(srpl_instance_t *instance, srpl_connection_t *srpl_connection);
static void srpl_disconnect(srpl_connection_t *srpl_connection);
static void srpl_connection_discontinue(srpl_connection_t *srpl_connection);
static void srpl_event_initialize(srpl_event_t *event, srpl_event_type_t event_type);
static void srpl_event_deliver(srpl_connection_t *srpl_connection, srpl_event_t *event);
static void srpl_domain_advertise(srpl_domain_t *domain);
static void srpl_connection_finalize(srpl_connection_t *srpl_connection);
static void srpl_instance_address_query_reset(srpl_instance_t *instance);
static void srpl_instance_reconnect(srpl_instance_t *instance);
static void srpl_instance_reconnect_callback(void *context);
static bool srpl_domain_browse_start(srpl_domain_t *domain);
static const char *srpl_state_name(srpl_state_t state);
static bool srpl_can_transition_to_routine_state(srpl_domain_t *domain);
static void srpl_transition_to_routine_state(srpl_domain_t *domain);
static void srpl_message_sent(srpl_connection_t *srpl_connection);
static srpl_state_t srpl_connection_schedule_reconnect_event(srpl_connection_t *srpl_connection, uint32_t when);
static void srpl_partner_discovery_timeout(void *context);
static void srpl_instance_services_discontinue(srpl_instance_t *instance);
static void srpl_instance_service_discontinue_timeout(void *context);
static void srpl_maybe_sync_or_transition(srpl_domain_t *domain);
static int srpl_dataset_id_compare(uint64_t id1, uint64_t id2);
static void srpl_state_transition_by_dataset_id(srpl_domain_t *domain, srpl_instance_t *instance);

#define EQUI_DISTANCE64 (int64_t)0x8000000000000000
#define MAX_ADDITIONAL_HOST_MESSAGES 32
#define SRPL_STATE_TIMEOUT (30 * 1000) // state timeout in milliseconds
#define SECONDS_IN_MINUTE  60
#define SECONDS_IN_HOUR    (60 * 60)
#define SECONDS_IN_DAY     (24 * SECONDS_IN_HOUR)

#ifdef DEBUG
#define STATE_DEBUGGING_ABORT() abort();
#else
#define STATE_DEBUGGING_ABORT()
#endif

// Send reconfirm records for all queries relating to this connection.
static void
srpl_reconfirm(srpl_connection_t *connection)
{
    // If there's no instance, that's why we got here, so no need for reconfirms.
    if (connection->instance == NULL) {
        INFO("no instance");
        return;
    }
    srpl_instance_t *instance = connection->instance;

    for (srpl_instance_service_t *service = instance->services; service != NULL; service = service->next) {
        if (!service->got_new_info) {
            INFO("we haven't had any new information since the last time we did a reconfirm, so no point doing it again.");
            continue;
        }
        service->got_new_info = false;

        if (service->full_service_name != NULL && service->ptr_rdata != NULL) {
            // The service name is the service instance name minus the first label.
            char *service_type = strchr(service->full_service_name, '.');
            if (service_type != NULL) {
                service_type++; // Skip the '.'
                // Send a reconfirm for the PTR record
                DNSServiceReconfirmRecord(0, service->ifindex, service_type, dns_rrtype_ptr, dns_qclass_in,
                                          service->ptr_length, service->ptr_rdata);
            }
            if (service->srv_rdata != NULL) {
                DNSServiceReconfirmRecord(0, service->ifindex, service->full_service_name, dns_rrtype_srv, dns_qclass_in,
                                          service->srv_length, service->srv_rdata);
            }
            if (service->txt_rdata != NULL) {
                DNSServiceReconfirmRecord(0, service->ifindex, service->full_service_name, dns_rrtype_txt, dns_qclass_in,
                                          service->txt_length, service->txt_rdata);
            }
            if (service->address_query != NULL) {
                address_query_t *query = service->address_query;
                for (int i = 0; i > query->num_addresses; i++) {
                    if (query->addresses[i].sa.sa_family == AF_INET) {
                        DNSServiceReconfirmRecord(0, query->address_interface[i], query->hostname, dns_rrtype_a,
                                                  dns_qclass_in, 4, &query->addresses[i].sin.sin_addr);
                    } else if (query->addresses[i].sa.sa_family == AF_INET6) {
                        DNSServiceReconfirmRecord(0, query->address_interface[i], query->hostname, dns_rrtype_aaaa,
                                                  dns_qclass_in, 16, &query->addresses[i].sin6.sin6_addr);
                    }
                }
            }
        }
    }
}

//
// 1. Enumerate all SRP servers that are participating in synchronization on infrastructure: This is done by looking up
//    NS records for <thread-network-name>.thread.home.arpa with the ForceMulticast flag set so that we use mDNS to
//    discover them.

// 2. For each identified server that is not this server, look up A and AAAA records for the server's hostname.

// 3. Maintain a state object with the list of IP addresses and an index to the current server being tried.

// 4. Try to connect to the first address on the list -> connection management state machine:

//   * When we have established an outgoing connection to a server, generate a random 64-bit unsigned number and send a
//     SRPLSession DSO message using that number as the server ID.

//   * When we receive an SRPLSession DSO message, see if we have an outgoing connection from the same server
//     for which we've sent a server ID. If so, and if the server id we received is less than the one we sent,
//     terminate the outgoing connection. If the server ids are equal, generate a new server id for the outgoing
//     connection and send another session establishment message.
//
//   * When we receive an acknowledgement to our SRPLSession DSO message, see if we have an incoming
//     connection from the same server from which we've received a server id. If the server id we received is less
//     than the one we sent, terminate the outgoing connection. If the server ids are equal, generate a new random
//     number for the outgoing connection and send another session establishment message.
//
//   * When a connection from a server is terminated, see if we have an established outgoing connection with that
//     server.  If not, attempt to connect to the next address we have for that server.
//
//   * When our connection to a server is terminated or fails, and there is no established incoming connection from that
//     server, attempt to connect to the next address we have for that server.
//
//   * When the NS record for a server goes away, drop any outgoing connection to that server and discontinue trying to
//     connect to it.
//
// 5. When we have established a session, meaning that we either got an acknowledgment to a SRPLSession DSO
//    message that we sent and _didn't_ drop the connection, or we got an SRPLSession DSO message on an
//    incoming connection with a lower server id than the outgoing connection, we begin the synchronization process.
//
//    * Immediately following session establishment, we generate a list of candidate hosts to send to the other server
//      from our internal list of SRP hosts (clients). Every non-expired host entry goes into the candidate list.
//
//    * Then, if we are the originator, we sent an SRPLSendCandidates message.
//
//    * If we are the recipient, we wait for an SRPLSendCandidates message.
//
//    * When we receive an SRPLSendCandidates message, we iterate across the candidates list, for each
//      candidate sending an SRPLCandidate message containing the host key, current time, and last message
//      received times, in seconds since the epoch. When we come to the end of the candidates list, we send an
//      acknowledgement to the SRPLSendCandidates message and discard the candidates list.
//
//    * When we receive an SRPLCandidate message, we look in our own candidate list, if there is one, to see
//      if the host key is present in the candidates list. If it is not present, or if it is present and the received
//      time from the SRPLCandidate message is later than the time we have recorded in our own candidate
//      list, we send an SRPLCandidateRequest message with the host key from the SRPLCandidate
//      message.
//
//    * When we receive an SRPLCandidateRequest message, we send an SRPLHost message which
//      encapsulates the SRP update for the host and includes the timestamp when we received the SRP update from that
//      host, which may have changed since we sent the SRPLCandidate message.
//
//    * When we receive an SRPLHost message, we look in our list of hosts (SRP clients) for a matching
//      host. If no such host exists, or if it exists and the timestamp is less than the timestamp in the
//      SRPLHost message, we process the SRP update from the SRPLHost message and mark the host as
//      "not received locally."  In other words, this message was not received directly from an SRP client, but rather
//      indirectly through our SRP replication partner. Note that this message cannot be assumed to be syntactically
//      correct and must be treated like any other data received from the network. If we are sent an invalid message,
//      this is an indication that our partner is broken in some way, since it should have validated the message before
//      accepting it.
//
//    * Whenever the SRP engine applies an SRP update from a host, it also delivers that update to each replication
//      server state engine.
//
//      * That replication server state engine first checks to see if it is connected to its partner; if not, no action
//        is taken.
//
//      * It then checks to see if there is a candidate list.
//        * If so, it checks to see if the host implicated in the update is already on the candidate list.
//          * If so, it updates that candidate's update time.
//          * If not, it adds the host to the end of the candidate list.
//        * If not, it sends an SRPLCandidate message to the other replication server, with the host
//          key and new timestamp.
//
// 6. When there is more than one SRP server participating in replication, only one server should advertise using
//    mDNS. All other servers should only advertise using DNS and DNS Push (SRP scalability feature). The SRP server
//    with the lowest numbered server ID is the one that acts as an advertising proxy for SRP. In practice this means
//    that if we have the lowest server ID of all the SRP servers we are connected to, we advertise mDNS. If two servers
//    on the same link can't connect to each other, they probably can't see each others' multicasts, so this is the
//    right outcome.

static bool
ip_addresses_equal(const addr_t *a, const addr_t *b)
{
    return (a->sa.sa_family == b->sa.sa_family &&
            ((a->sa.sa_family == AF_INET && !memcmp(&a->sin.sin_addr, &b->sin.sin_addr, 4)) ||
             (a->sa.sa_family == AF_INET6 && !memcmp(&a->sin6.sin6_addr, &b->sin6.sin6_addr, 16))));
}

#define ADDR_NAME_LOGGER(log_type, address, preamble, conjunction, number, fullname, interfaceIndex)            \
    if ((address)->sa.sa_family == AF_INET6) {                                                                  \
        SEGMENTED_IPv6_ADDR_GEN_SRP(&(address)->sin6.sin6_addr, rdata_buf);                                     \
        log_type(PUB_S_SRP PRI_SEGMENTED_IPv6_ADDR_SRP PUB_S_SRP PRI_S_SRP PUB_S_SRP "%d", preamble,            \
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&(address)->sin6.sin6_addr, rdata_buf),                          \
                 conjunction, fullname, number, interfaceIndex);                                                \
    } else {                                                                                                    \
        IPv4_ADDR_GEN_SRP(&(address)->sin.sin_addr, rdata_buf);                                                 \
        log_type(PUB_S_SRP PRI_IPv4_ADDR_SRP PUB_S_SRP PRI_S_SRP PUB_S_SRP "%d", preamble,                      \
                 IPv4_ADDR_PARAM_SRP(&(address)->sin.sin_addr, rdata_buf),                                      \
                 conjunction, fullname, number, interfaceIndex);                                                \
    }

static void
address_query_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                       DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                       uint16_t rdlen, const void *rdata, uint32_t UNUSED ttl, void *context)
{
    address_query_t *address = context;
    addr_t addr;
    int i, j;

    if (errorCode != kDNSServiceErr_NoError) {
        ERROR("address resolution for " PRI_S_SRP " failed with %d", fullname, errorCode);
        address->change_callback(address->context, NULL, false, flags & kDNSServiceFlagsMoreComing, errorCode);
        return;
    }
    if (rrclass != dns_qclass_in || ((rrtype != dns_rrtype_a || rdlen != 4) &&
                                     (rrtype != dns_rrtype_aaaa || rdlen != 16))) {
        ERROR("Invalid response record type (%d) or class (%d) provided for " PRI_S_SRP, rrtype, rrclass, fullname);
        return;
    }

    memset(&addr, 0, sizeof(addr));
    if (rrtype == dns_rrtype_a) {
#ifndef NOT_HAVE_SA_LEN
        addr.sa.sa_len = sizeof(struct sockaddr_in);
#endif
        addr.sa.sa_family = AF_INET;
        memcpy(&addr.sin.sin_addr, rdata, rdlen);
        if (IN_LINKLOCAL(addr.sin.sin_addr.s_addr)) {
            ADDR_NAME_LOGGER(INFO, &addr, "Skipping link-local address ", " received for instance ", " index ",
                             fullname, interfaceIndex);
            return;
        }
    } else {
#ifndef NOT_HAVE_SA_LEN
        addr.sa.sa_len = sizeof(struct sockaddr_in6);
#endif
        addr.sa.sa_family = AF_INET6;
        memcpy(&addr.sin6.sin6_addr, rdata, rdlen);
        if (IN6_IS_ADDR_LINKLOCAL(&addr.sin6.sin6_addr)) {
            ADDR_NAME_LOGGER(INFO, &addr, "Skipping link-local address ", " received for instance ", " index ",
                             fullname, interfaceIndex);
            return;
        }
    }

    for (i = 0, j = 0; i < address->num_addresses; i++) {
        // Already in the list?
        if (address->address_interface[i] == interfaceIndex && !memcmp(&address->addresses[i], &addr, sizeof(addr))) {
            if (flags & kDNSServiceFlagsAdd) {
                ADDR_NAME_LOGGER(INFO, &addr, "Duplicate address ", " received for instance ", " index ",
                                 fullname, interfaceIndex);
                return;
            } else {
                ADDR_NAME_LOGGER(INFO, &addr, "Removing address ", " from instance ", " index ",
                                            fullname, interfaceIndex);

                // If we're removing an address, we keep going through the array copying down.
                if (address->cur_address >= i) {
                    address->cur_address--;
                }
            }
        } else {
            // Copy down.
            if (i != j) {
                address->addresses[j] = address->addresses[i];
                address->address_interface[j] = address->address_interface[i];
            }
            j++;
        }
    }
    if (flags & kDNSServiceFlagsAdd) {
        if (i == ADDRESS_QUERY_MAX_ADDRESSES) {
            ADDR_NAME_LOGGER(ERROR, &addr, "No room for address ", " received for ", " index ",
                                         fullname, interfaceIndex);
            return;
        }

        ADDR_NAME_LOGGER(INFO, &addr, "Adding address ", " to ", " index ", fullname, interfaceIndex);

        address->addresses[i] = addr;
        address->address_interface[i] = interfaceIndex;
        address->num_addresses++;
        address->change_callback(address->context, &address->addresses[i], true, flags & kDNSServiceFlagsMoreComing, kDNSServiceErr_NoError);
    } else {
        if (i == j) {
            ADDR_NAME_LOGGER(ERROR, &addr, "Remove for unknown address ", " received for ", " index ",
                               fullname, interfaceIndex);
            return;
        } else {
            address->num_addresses--;
            address->change_callback(address->context, &addr, false, flags & kDNSServiceFlagsMoreComing, kDNSServiceErr_NoError);
        }
    }
}

static void
address_query_finalize(void *context)
{
    address_query_t *address = context;
    free(address->hostname);
    free(address);
}

static void
address_query_cancel(address_query_t *address)
{
    if (address->a_query != NULL) {
        ioloop_dnssd_txn_cancel(address->a_query);
        ioloop_dnssd_txn_release(address->a_query);
        address->a_query = NULL;
    }
    if (address->aaaa_query != NULL) {
        ioloop_dnssd_txn_cancel(address->aaaa_query);
        ioloop_dnssd_txn_release(address->aaaa_query);
        address->aaaa_query = NULL;
    }

    // Have whatever holds a reference to the address query let go of it.
    if (address->cancel_callback != NULL && address->context != NULL) {
        address->cancel_callback(address->context);
        address->context = NULL;
        address->cancel_callback = NULL;
    }
}

static void
address_query_txn_fail(void *context, int err)
{
    address_query_t *address = context;
    ERROR("address query " PRI_S_SRP " i/o failure: %d", address->hostname, err);
    address_query_cancel(address);
}

static void
address_query_context_release(void *context)
{
    address_query_t *address = context;
    RELEASE_HERE(address, address_query);
}

static address_query_t *
address_query_create(const char *hostname, void *context, address_change_callback_t change_callback,
                     address_query_cancel_callback_t cancel_callback)
{
    address_query_t *address = calloc(1, sizeof(*address));
    DNSServiceRef sdref;
    dnssd_txn_t **txn;

    require_action_quiet(address != NULL, exit_no_free, ERROR("No memory for address query."));
    RETAIN_HERE(address, address_query); // We return a retained object, or free it.
    address->hostname = strdup(hostname);
    require_action_quiet(address->hostname != NULL, exit, ERROR("No memory for address query hostname."));

    for (int i = 0; i < 2; i++) {
        int ret = DNSServiceQueryRecord(&sdref, kDNSServiceFlagsForceMulticast | kDNSServiceFlagsLongLivedQuery,
                                        kDNSServiceInterfaceIndexAny, hostname, (i
                                                                                 ? kDNSServiceType_A
                                                                                 : kDNSServiceType_AAAA),
                                        kDNSServiceClass_IN, address_query_callback, address);
        require_action_quiet(ret == kDNSServiceErr_NoError, exit,
                             ERROR("Unable to resolve instance hostname " PRI_S_SRP " addresses: %d",
                                   hostname, ret));

        txn = i ? &address->a_query : &address->aaaa_query;
        *txn = ioloop_dnssd_txn_add(sdref, address, address_query_context_release, address_query_txn_fail);
        require_action_quiet(*txn != NULL, exit,
                             ERROR("Unable to set up ioloop transaction for " PRI_S_SRP " query on " THREAD_BROWSING_DOMAIN,
                                   hostname);
                             DNSServiceRefDeallocate(sdref));
        RETAIN_HERE(address, address_query); // For the QueryRecord context
    }
    address->change_callback = change_callback;
    address->cancel_callback = cancel_callback;
    address->context = context;
    address->cur_address = -1;
    return address;

exit:
    if (address->a_query != NULL) {
        ioloop_dnssd_txn_cancel(address->a_query);
        ioloop_dnssd_txn_release(address->a_query);
        address->a_query = NULL;
    }
    if (address->aaaa_query != NULL) { // Un-possible right now, but better safe than sorry in case of future change
        ioloop_dnssd_txn_cancel(address->aaaa_query);
        ioloop_dnssd_txn_release(address->aaaa_query);
        address->aaaa_query = NULL;
    }
    RELEASE_HERE(address, address_query);
    address = NULL;

exit_no_free:
    return address;
}

static void
srpl_domain_finalize(srpl_domain_t *domain)
{
    srpl_instance_t *instance, *next;
    srpl_instance_service_t *service, *next_service;

    free(domain->name);
    if (domain->query != NULL) {
        ioloop_dnssd_txn_cancel(domain->query);
        ioloop_dnssd_txn_release(domain->query);
    }

    for (instance = domain->instances; instance != NULL; instance = next) {
        next = instance->next;
        srpl_instance_services_discontinue(instance);
    }
    for (service = domain->unresolved_services; service != NULL; service = next_service) {
        next_service = service->next;
        srpl_instance_service_discontinue_timeout(service);
    }
    if (domain->query != NULL) {
        ioloop_dnssd_txn_cancel(domain->query);
        ioloop_dnssd_txn_release(domain->query);
        domain->query = NULL;
    }
    if (domain->srpl_advertise_txn != NULL) {
        ioloop_dnssd_txn_cancel(domain->srpl_advertise_txn);
        ioloop_dnssd_txn_release(domain->srpl_advertise_txn);
        domain->srpl_advertise_txn = NULL;
    }
    if (domain->srpl_register_wakeup != NULL) {
        ioloop_cancel_wake_event(domain->srpl_register_wakeup);
        ioloop_wakeup_release(domain->srpl_register_wakeup);
        domain->srpl_register_wakeup = NULL;
    }
    if (domain->partner_discovery_timeout != NULL) {
        ioloop_cancel_wake_event(domain->partner_discovery_timeout);
        ioloop_wakeup_release(domain->partner_discovery_timeout);
        domain->partner_discovery_timeout = NULL;
    }

    free(domain);
}

static void srpl_instance_finalize(srpl_instance_t *instance);

static void
srpl_instance_service_finalize(srpl_instance_service_t *service)
{
    if (service->domain != NULL) {
        RELEASE_HERE(service->domain, srpl_domain);
    }
    if (service->txt_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->txt_txn);
        ioloop_dnssd_txn_release(service->txt_txn);
        service->txt_txn = NULL;
    }
    if (service->srv_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->srv_txn);
        ioloop_dnssd_txn_release(service->srv_txn);
        service->srv_txn = NULL;
    }
    free(service->full_service_name);
    free(service->host_name);
    free(service->txt_rdata);
    free(service->srv_rdata);
    free(service->ptr_rdata);
    if (service->address_query != NULL) {
        address_query_cancel(service->address_query);
        RELEASE_HERE(service->address_query, address_query);
        service->address_query = NULL;
    }
    if (service->discontinue_timeout != NULL) {
        ioloop_cancel_wake_event(service->discontinue_timeout);
        ioloop_wakeup_release(service->discontinue_timeout);
        service->discontinue_timeout = NULL;
    }
    if (service->resolve_wakeup != NULL) {
        ioloop_cancel_wake_event(service->resolve_wakeup);
        ioloop_wakeup_release(service->resolve_wakeup);
        service->resolve_wakeup = NULL;
    }
    if (service->instance != NULL) {
        RELEASE_HERE(service->instance, srpl_instance);
        service->instance = NULL;
    }
    free(service);
}

static void
srpl_instance_finalize(srpl_instance_t *instance)
{
    if (instance->domain != NULL) {
        RELEASE_HERE(instance->domain, srpl_domain);
        instance->domain = NULL;
    }
    free(instance->instance_name);
    if (instance->connection != NULL) {
        srpl_connection_discontinue(instance->connection);
        RELEASE_HERE(instance->connection, srpl_connection);
        instance->connection = NULL;
    }
    if (instance->reconnect_timeout != NULL) {
        ioloop_cancel_wake_event(instance->reconnect_timeout);
        ioloop_wakeup_release(instance->reconnect_timeout);
        instance->reconnect_timeout = NULL;
    }

    srpl_instance_service_t *service = instance->services, *next;
    while (service != NULL) {
        next = service->next;
        RELEASE_HERE(service, srpl_instance_service);
        service = next;
    }
    instance->services = NULL;
    free(instance);
}

#define srpl_connection_message_set(srpl_connection, message) \
    srpl_connection_message_set_(srpl_connection, message, __FILE__, __LINE__)
static void
srpl_connection_message_set_(srpl_connection_t *srpl_connection, message_t *message, const char *file, int line)
{
    if (srpl_connection->message != NULL) {
        ioloop_message_release_(srpl_connection->message, file, line);
        srpl_connection->message = NULL;
    }
    if (message != NULL) {
        srpl_connection->message = message;
        ioloop_message_retain_(srpl_connection->message, file, line);
    }
}

static message_t *
srpl_connection_message_get(srpl_connection_t *srpl_connection)
{
    return srpl_connection->message;
}

#define srpl_candidate_free(candidate) srpl_candidate_free_(candidate, __FILE__, __LINE__)
static void
srpl_candidate_free_(srpl_candidate_t *candidate, const char *file, int line)
{
    if (candidate != NULL) {
        if (candidate->name != NULL) {
            dns_name_free(candidate->name);
            candidate->name = NULL;
        }
        if (candidate->message != NULL) {
            ioloop_message_release_(candidate->message, file, line);
            candidate->message = NULL;
        }
        if (candidate->host != NULL) {
            srp_adv_host_release_(candidate->host, file, line);
            candidate->host = NULL;
        }
        free(candidate);
    }
}

static void
srpl_connection_candidates_free(srpl_connection_t *srpl_connection)
{
    if (srpl_connection->candidates == NULL) {
        goto out;
    }
    for (int i = 0; i < srpl_connection->num_candidates; i++) {
        if (srpl_connection->candidates[i] != NULL) {
            srp_adv_host_release(srpl_connection->candidates[i]);
        }
    }
    free(srpl_connection->candidates);
    srpl_connection->candidates = NULL;
out:
    srpl_connection->num_candidates = srpl_connection->current_candidate = 0;
    return;
}

static void
srpl_srp_client_update_queue_free(srpl_connection_t *srpl_connection)
{
    srpl_srp_client_queue_entry_t **cp = &srpl_connection->client_update_queue;
    while (*cp) {
        srpl_srp_client_queue_entry_t *entry = *cp;
        srp_adv_host_release(entry->host);
        *cp = entry->next;
        free(entry);
    }
}

static void
srpl_connection_candidate_set(srpl_connection_t *srpl_connection, srpl_candidate_t *candidate)
{
    if (srpl_connection->candidate != NULL) {
        srpl_candidate_free(srpl_connection->candidate);
    }
    srpl_connection->candidate = candidate;
}

static void
srpl_host_update_parts_free(srpl_host_update_t *update)
{
    if (update->messages != NULL) {
        for (int i = 0; i < update->num_messages; i++) {
            ioloop_message_release(update->messages[i]);
        }
        free(update->messages);
        update->messages = NULL;
        update->num_messages = update->max_messages = update->messages_processed = 0;
    }
    if (update->hostname != NULL) {
        dns_name_free(update->hostname);
        update->hostname = NULL;
    }
}

// Free up any temporarily retained or allocated objects on the connection (i.e., not the name).
static void
srpl_connection_reset(srpl_connection_t *srpl_connection)
{
    srpl_connection->candidates_not_generated = true;
    srpl_connection->database_synchronized = false;
    srpl_host_update_parts_free(&srpl_connection->stashed_host);
    srpl_connection_message_set(srpl_connection, NULL);
    if (srpl_connection->candidate != NULL) {
        srpl_candidate_free(srpl_connection->candidate);
        srpl_connection->candidate = NULL;
    }

    // Cancel keepalive timers
    if (srpl_connection->keepalive_send_wakeup) {
        ioloop_cancel_wake_event(srpl_connection->keepalive_send_wakeup);
    }
    if (srpl_connection->keepalive_receive_wakeup) {
        ioloop_cancel_wake_event(srpl_connection->keepalive_receive_wakeup);
    }

    srpl_connection_candidates_free(srpl_connection);
    srpl_srp_client_update_queue_free(srpl_connection);
}

static void
srpl_connection_finalize(srpl_connection_t *srpl_connection)
{
    if (srpl_connection->instance) {
        RELEASE_HERE(srpl_connection->instance, srpl_instance);
        srpl_connection->instance = NULL;
    }
    if (srpl_connection->connection != NULL) {
        ioloop_comm_release(srpl_connection->connection);
        srpl_connection->connection = NULL;
        gettimeofday(&srpl_connection->connection_null_time, NULL);
        srpl_connection->connection_null_reason = "finalize"; // obvsly should never see this!
    }
    if (srpl_connection->reconnect_wakeup != NULL) {
        ioloop_cancel_wake_event(srpl_connection->reconnect_wakeup);
        ioloop_wakeup_release(srpl_connection->reconnect_wakeup);
        srpl_connection->reconnect_wakeup = NULL;
    }
    if (srpl_connection->state_timeout != NULL) {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        ioloop_wakeup_release(srpl_connection->state_timeout);
        srpl_connection->state_timeout = NULL;
    }
    if (srpl_connection->keepalive_send_wakeup != NULL) {
        ioloop_cancel_wake_event(srpl_connection->keepalive_send_wakeup);
        ioloop_wakeup_release(srpl_connection->keepalive_send_wakeup);
        srpl_connection->keepalive_send_wakeup = NULL;
    }
    if (srpl_connection->keepalive_receive_wakeup != NULL) {
        ioloop_cancel_wake_event(srpl_connection->keepalive_receive_wakeup);
        ioloop_wakeup_release(srpl_connection->keepalive_receive_wakeup);
        srpl_connection->keepalive_receive_wakeup = NULL;
    }
    srpl_host_update_parts_free(&srpl_connection->stashed_host);
    free(srpl_connection->name);
    srpl_connection_reset(srpl_connection);
    free(srpl_connection);
}

void
srpl_connection_release_(srpl_connection_t *srpl_connection, const char *file, int line)
{
    RELEASE(srpl_connection, srpl_connection);
}

void
srpl_connection_retain_(srpl_connection_t *srpl_connection, const char *file, int line)
{
    RETAIN(srpl_connection, srpl_connection);
}

srpl_connection_t *
srpl_connection_create(srpl_instance_t *instance, bool outgoing)
{
    srpl_connection_t *srpl_connection = calloc(1, sizeof (*srpl_connection)), *ret = NULL;
    if (srpl_connection == NULL) {
        goto out;
    }
    RETAIN_HERE(srpl_connection, srpl_connection);
#define POINTER_TO_HEX_MAX_STRLEN 19 // 0x<...>
    size_t srpl_connection_name_length = strlen(instance->instance_name) + 2 + POINTER_TO_HEX_MAX_STRLEN + 3;
    srpl_connection->name = malloc(srpl_connection_name_length);
    if (srpl_connection->name == NULL) {
        goto out;
    }
    srpl_connection->keepalive_send_wakeup = ioloop_wakeup_create();
    if (srpl_connection->keepalive_send_wakeup == NULL) {
        goto out;
    }
    srpl_connection->keepalive_receive_wakeup = ioloop_wakeup_create();
    if (srpl_connection->keepalive_receive_wakeup == NULL) {
        goto out;
    }
    srpl_connection->keepalive_interval = DEFAULT_KEEPALIVE_WAKEUP_EXPIRY / 2;
    snprintf(srpl_connection->name, srpl_connection_name_length, "%s%s (%p)", outgoing ? ">" : "<", instance->instance_name, srpl_connection);
    srpl_connection->is_server = !outgoing;
    srpl_connection->instance = instance;
    RETAIN_HERE(instance, srpl_instance);
#ifdef SRP_TEST_SERVER
    srpl_connection->state = srpl_state_test_event_intercept;
#endif
    ret = srpl_connection;
    srpl_connection = NULL;
out:
    if (srpl_connection != NULL) {
        RELEASE_HERE(srpl_connection, srpl_connection);
    }
    return ret;
}

static void
srpl_connection_context_release(void *context)
{
    srpl_connection_t *srpl_connection = context;

    RELEASE_HERE(srpl_connection, srpl_connection);
}

static void
srpl_instance_service_context_release(void *context)
{
    srpl_instance_service_t *service = context;

    RELEASE_HERE(service, srpl_instance_service);
}

static void
srpl_instance_context_release(void *context)
{
    srpl_instance_t *instance = context;

    RELEASE_HERE(instance, srpl_instance);
}

static void
srpl_instance_discontinue_timeout(void *context)
{
    srpl_instance_t **sp = NULL, *instance = context;
    srpl_domain_t *domain = instance->domain;

    INFO("discontinuing instance " PRI_S_SRP " with partner id %" PRIx64, instance->instance_name, instance->partner_id);
    for (sp = &domain->instances; *sp; sp = &(*sp)->next) {
        if (*sp == instance) {
            *sp = instance->next;
            break;
        }
    }

    srpl_connection_t *srpl_connection = instance->connection;
    if (srpl_connection != NULL) {
        RELEASE_HERE(srpl_connection->instance, srpl_instance);
        srpl_connection->instance = NULL;
        srpl_connection_discontinue(srpl_connection);
        // The instance no longer has a reference to the srpl_connection object.
        RELEASE_HERE(srpl_connection, srpl_connection);
        instance->connection = NULL;
    }
    RELEASE_HERE(instance, srpl_instance);

    // Check to see if we are eligible to move into the routine state if we haven't done so.
    // If the partner we failed to sync with goes away, we could enter the routine state if
    // we have succcessfully sync-ed with all other partners discovered in startup.
    if (domain->srpl_opstate != SRPL_OPSTATE_ROUTINE) {
        srpl_maybe_sync_or_transition(domain);
    }
}

static void
srpl_instance_service_discontinue_timeout(void *context)
{
    srpl_instance_service_t **hp = NULL, *service = context;
    srpl_domain_t *domain = service->domain;
    srpl_instance_t *instance = service->instance;

    // Retain for duration of function, since otherwise we might finalize it below.

    // This retain shouldn't be necessary if we are actually being called by the timeout, because the timeout holds a
    // reference to service that can't be released below. However, this function can be called directly, outside of a
    // timeout, and in that case we do need to retain service for the function.

    RETAIN_HERE(service, srpl_instance_service);

    // Remove the service from either the unresolved_services list or resolved instance list
    if (instance == NULL) {
        hp = &domain->unresolved_services;
    } else {
        RETAIN_HERE(instance, srpl_instance); // Retain instance for life of function in case we decrement its refcnt below.
        hp = &instance->services;
    }
    for (; *hp; hp = &(*hp)->next) {
        if (*hp == service) {
            *hp = service->next;
            RELEASE_HERE(service, srpl_instance_service); // Release service list's reference to instance_service.
            break;
        }
    }

    if (service->discontinue_timeout != NULL) {
        ioloop_cancel_wake_event(service->discontinue_timeout);
        ioloop_wakeup_release(service->discontinue_timeout);
        service->discontinue_timeout = NULL;
    }
    if (service->resolve_wakeup != NULL) {
        ioloop_cancel_wake_event(service->resolve_wakeup);
        ioloop_wakeup_release(service->resolve_wakeup);
        service->resolve_wakeup = NULL;
    }
    if (service->address_query != NULL) {
        address_query_cancel(service->address_query);
        RELEASE_HERE(service->address_query, address_query);
        service->address_query = NULL;
    }
    if (service->txt_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->txt_txn);
        ioloop_dnssd_txn_release(service->txt_txn);
        service->txt_txn = NULL;
        service->resolve_started = false;
    }
    if (service->srv_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->srv_txn);
        ioloop_dnssd_txn_release(service->srv_txn);
        service->srv_txn = NULL;
        service->resolve_started = false;
    }
    if (service->instance != NULL) {
        RELEASE_HERE(service->instance, srpl_instance);
        service->instance = NULL;
    }
    if (instance != NULL) {
        if (instance->services == NULL) {
            srpl_instance_discontinue_timeout(instance);
        }
        RELEASE_HERE(instance, srpl_instance); // Release this function's reference to instance
    }
    RELEASE_HERE(service, srpl_instance_service); // Release this functions reference to instance_service.
}

static void
srpl_instance_services_discontinue(srpl_instance_t *instance)
{
    srpl_instance_service_t *service;
    for (service = instance->services; service != NULL; ) {
        // The service is retained on the list, but...
        srpl_instance_service_t *next = service->next;
        // This is going to release it...
        srpl_instance_service_discontinue_timeout(service);
        // So next is still valid here, but service isn't.
        service = next;
    }
}

static void
srpl_instance_service_discontinue(srpl_instance_service_t *service)
{
    // Already discontinuing.
    if (service->discontinuing) {
        INFO("Replication service " PRI_S_SRP " went away, already discontinuing", service->full_service_name);
        return;
    }
    if (service->num_copies > 0) {
        INFO("Replication service " PRI_S_SRP " went away, %d still left", service->host_name, service->num_copies);
        return;
    }
    INFO("Replication service " PRI_S_SRP " went away, none left, discontinuing", service->full_service_name);
    service->discontinuing = true;

    // DNSServiceResolve doesn't give us the kDNSServiceFlagAdd flag--apparently it's assumed that we know the
    // service was removed because we get a remove on the browse. So we need to restart the resolve if the
    // instance comes back, rather than continuing to use the old resolve transaction.
    if (service->txt_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->txt_txn);
        ioloop_dnssd_txn_release(service->txt_txn);
        service->txt_txn = NULL;
    }
    if (service->srv_txn != NULL) {
        ioloop_dnssd_txn_cancel(service->srv_txn);
        ioloop_dnssd_txn_release(service->srv_txn);
        service->srv_txn = NULL;
    }
    service->resolve_started = false;

    // if all the services are discontinuing, we mark the instance to be discontinuing as well.
    // discontinuing instance will be exluded when we check if the server has sync-ed on all the
    // instances in order to move to the routine state and when we pick the winning dataset id
    // from the discovered instances (i.e., discontinuing instance no longer qualifies for
    // dataset_id election).
    srpl_instance_t *instance = service->instance;
    if (instance != NULL) {
        srpl_instance_service_t *sp;
        for(sp = instance->services; sp != NULL; sp = sp->next) {
            if (!sp->discontinuing) {
                break;
            }
        }
        if (sp == NULL) {
            instance->discontinuing = true;
        }
    }
    // It's not uncommon for a name to drop and then come back immediately. Wait 30s before
    // discontinuing the instance host.
    if (service->discontinue_timeout == NULL) {
        service->discontinue_timeout = ioloop_wakeup_create();
        // Oh well.
        if (service->discontinue_timeout == NULL) {
            srpl_instance_service_discontinue_timeout(service);
            return;
        }
    }

    RETAIN_HERE(service, srpl_instance_service);
    ioloop_add_wake_event(service->discontinue_timeout, service, srpl_instance_service_discontinue_timeout,
                          srpl_instance_service_context_release, 30 * 1000);
}


static void
srpl_instance_discontinue(srpl_instance_t *instance)
{
    srpl_instance_service_t *service, *next;
    instance->discontinuing = true;
    for (service = instance->services; service != NULL; service = next) {
        next = service->next;
        service->num_copies = 0;
        srpl_instance_service_discontinue(service);
    }
}

void
srpl_shutdown(srp_server_t *server_state)
{
    srpl_instance_t *instance, *next;
    srpl_instance_service_t *service, *next_service;

    if (server_state->current_thread_domain_name == NULL) {
        INFO("no current domain");
        return;
    }
    for (srpl_domain_t **dp = &server_state->srpl_domains; *dp != NULL; ) {
        srpl_domain_t *domain = *dp;
        if (!strcmp(domain->name, server_state->current_thread_domain_name)) {
            for (instance = domain->instances; instance != NULL; instance = next) {
                next = instance->next;
                srpl_instance_services_discontinue(instance);
            }
            for (service = domain->unresolved_services; service != NULL; service = next_service) {
                next_service = service->next;
                srpl_instance_service_discontinue_timeout(service);
            }
            if (domain->query != NULL) {
                ioloop_dnssd_txn_cancel(domain->query);
                ioloop_dnssd_txn_release(domain->query);
                domain->query = NULL;
            }
            if (domain->srpl_advertise_txn != NULL) {
                ioloop_dnssd_txn_cancel(domain->srpl_advertise_txn);
                ioloop_dnssd_txn_release(domain->srpl_advertise_txn);
                domain->srpl_advertise_txn = NULL;
            }
            if (domain->partner_discovery_timeout != NULL) {
                ioloop_cancel_wake_event(domain->partner_discovery_timeout);
                ioloop_wakeup_release(domain->partner_discovery_timeout);
                domain->partner_discovery_timeout = NULL;
            }
            *dp = domain->next;
            RELEASE_HERE(domain, srpl_domain);
            free(server_state->current_thread_domain_name);
            server_state->current_thread_domain_name = NULL;
        } else {
            dp = &(*dp)->next;
        }
    }
}

void
srpl_disable(srp_server_t *server_state)
{
    srpl_shutdown(server_state);
    server_state->srp_replication_enabled = false;
}

void
srpl_drop_srpl_connection(srp_server_t *NONNULL server_state)
{
    for (srpl_domain_t *domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            if (instance->connection != NULL && instance->connection->state > srpl_state_disconnect_wait) {
                srpl_connection_discontinue(instance->connection);
            }
        }
    }
}

void
srpl_undrop_srpl_connection(srp_server_t *NONNULL server_state)
{
    for (srpl_domain_t *domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            srpl_instance_reconnect(instance);
        }
    }
}

// Stop service advertisement in the given domain.
static void
srpl_stop_domain_advertisement(srpl_domain_t *NONNULL domain)
{
    INFO("dropping advertisement for domain " PUB_S_SRP, domain->name);
    if (domain->srpl_advertise_txn != NULL) {
        ioloop_dnssd_txn_cancel(domain->srpl_advertise_txn);
        ioloop_dnssd_txn_release(domain->srpl_advertise_txn);
        domain->srpl_advertise_txn = NULL;
    }
}

// Stop service advertisement in all the domains
void
srpl_drop_srpl_advertisement(srp_server_t *NONNULL server_state)
{
    srpl_domain_t *domain;
    for (domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        srpl_stop_domain_advertisement(domain);
    }
}

void
srpl_undrop_srpl_advertisement(srp_server_t *NONNULL server_state)
{
    srpl_domain_t *domain;
    for (domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        srpl_domain_advertise(domain);
    }
}


// Copy from into to, and then NULL out the host pointer in from, which is not refcounted, so that we don't get a
// double free later. Add a reference to the message, since it is refcounted.
static void
srpl_host_update_steal_parts(srpl_host_update_t *to, srpl_host_update_t *from)
{
    srpl_host_update_parts_free(to);
    *to = *from;
    from->hostname = NULL;
    from->messages = NULL;
    from->num_messages = from->max_messages = from->messages_processed = 0;
}

static bool
srpl_event_content_type_set_(srpl_event_t *event, srpl_event_content_type_t content_type, const char *file, int line)
{
    switch(event->content_type) {
    case srpl_event_content_type_none:
    case srpl_event_content_type_address:
    case srpl_event_content_type_session:
    case srpl_event_content_type_candidate_disposition:
    case srpl_event_content_type_rcode:
    case srpl_event_content_type_client_result: // pointers owned by caller
    case srpl_event_content_type_advertise_finished_result:
        break;

    case srpl_event_content_type_candidate:
        if (event->content.candidate != NULL) {
            srpl_candidate_free_(event->content.candidate, file, line);
            event->content.candidate = NULL;
        }
        break;
    case srpl_event_content_type_host_update:
        srpl_host_update_parts_free(&event->content.host_update);
        break;
    }
    memset(&event->content, 0, sizeof(event->content));
    if (content_type == srpl_event_content_type_candidate) {
        event->content.candidate = calloc(1, sizeof(srpl_candidate_t));
        if (event->content.candidate == NULL) {
            return false;
        }
    }
    event->content_type = content_type;
    return true;
}

static void
srpl_disconnected_callback(comm_t *comm, void *context, int UNUSED error)
{
    srpl_connection_t *srpl_connection = context;
    srpl_domain_t *domain;

    // No matter what state we are in, if we are disconnected, we can't continue with the existing connection.
    // Either we need to make a new connection, or go idle.

    srpl_instance_t *instance = srpl_connection->instance;

    // The connection would still be holding a reference; hold a reference to the connection to avoid it being released
    // prematurely.
    RETAIN_HERE(srpl_connection, srpl_connection);

    // Get rid of the comm_t connection object if it's still around
    if (srpl_connection->connection != NULL && srpl_connection->connection == comm) {
        comm_t *connection = srpl_connection->connection;
        srpl_connection->connection = NULL;
        gettimeofday(&srpl_connection->connection_null_time, NULL);
        srpl_connection->connection_null_reason = "disconnected_callback";
        ioloop_comm_release(connection);

        if (srpl_connection->dso != NULL) {
            dso_state_cancel(srpl_connection->dso);
            srpl_connection->dso = NULL;
        }
    }

    // If there's no instance, this connection just needs to go away (and presumably has).
    if (instance == NULL) {
        INFO("the instance is NULL.");
        goto out;
    }

    // Because instance is still holding a reference to srpl_connection, it's safe to keep using srpl_connection.

    // Clear old data from connection.
    srpl_connection_reset(srpl_connection);

    domain = instance->domain;
    if (domain == NULL) {
        // If domain is NULL, instance has been discontinued.
        INFO(PRI_S_SRP "instance was discontinued, not reconnecting.", instance->instance_name);
    } else {
        // If we are in the startup state, we should reinitiate the connection to the peer.
        // Otherwise, we should reconnect only if our partner id is greater than the peer's.
        // If there's no partner id on the instance, the instance should be a temporary one
        // and that means we haven't discovered the peer yet, so we can just drop the connection
        // and wait to discover it or for it to reconnect.
        if (domain->srpl_opstate == SRPL_OPSTATE_STARTUP ||
            (instance->have_partner_id && domain->server_state != NULL &&
            domain->partner_id > instance->partner_id))
        {
            // cancel reconnect_timeout if there's one scheduled.
            if (instance->reconnect_timeout != NULL) {
                ioloop_cancel_wake_event(instance->reconnect_timeout);
            }
            INFO(PRI_S_SRP ": disconnect received, reconnecting.", srpl_connection->name);
            srpl_connection_next_state(srpl_connection, srpl_state_next_address_get);
            goto out;
        }
    }

    // If the connection is in the disconnect_wait state, deliver an event.
    if (srpl_connection->state == srpl_state_disconnect_wait) {
        srpl_event_t event;
        srpl_event_initialize(&event, srpl_event_disconnected);
        srpl_event_deliver(srpl_connection, &event);
        goto out;
    }

    // If it's not our job to reconnect, we no longer need this connection. Release the reference
    // held by the instance (which'd cause the connection to be finalized).
    srpl_connection_next_state(srpl_connection, srpl_state_idle);
    if (instance->connection == srpl_connection) {
        RELEASE_HERE(srpl_connection, srpl_connection);
        instance->connection = NULL;
    }

out:
    RELEASE_HERE(srpl_connection, srpl_connection);
}

static bool
srpl_dso_message_setup(dso_state_t *dso, dso_message_t *state, dns_towire_state_t *towire, uint8_t *buffer,
                       size_t buffer_size, message_t *message, bool unidirectional, bool response, int rcode,
                       uint16_t xid, srpl_connection_t *srpl_connection)
{
    uint16_t send_xid = 0;

    if (srpl_connection->connection == NULL) {
        struct tm tm;
        localtime_r(&srpl_connection->connection_null_time.tv_sec, &tm);
        char tmoff = tm.tm_gmtoff > 0 ? '+' : '-';
        long tzoff = tm.tm_gmtoff > 0 ? tm.tm_gmtoff : -tm.tm_gmtoff;
        FAULT("sending a message on a nonexistent connection: " PUB_S_SRP " (%04d-%02d-%02d %02d:%02d:%02d.%06d%c%02ld%02ld)!",
              srpl_connection->connection_null_reason == NULL
              ? "no connection ever set" : srpl_connection->connection_null_reason,
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
              srpl_connection->connection_null_time.tv_usec, tmoff, tzoff / 3600, (tzoff / 60) % 60);
        return false;
    }

    if (buffer_size < DNS_HEADER_SIZE) {
        ERROR("internal: invalid buffer size %zd", buffer_size);
        return false;
    }

    if (response) {
        if (message != NULL) {
            send_xid = message->wire.id;
        } else {
            send_xid = xid;
        }
    }
    dso_make_message(state, buffer, buffer_size, dso, unidirectional, response,
                     send_xid, rcode, srpl_connection);
    memset(towire, 0, sizeof(*towire));
    towire->p = &buffer[DNS_HEADER_SIZE];
    towire->lim = towire->p + (buffer_size - DNS_HEADER_SIZE);
    towire->message = (dns_wire_t *)buffer;
    return true;
}

static srpl_domain_t *
srpl_connection_domain(srpl_connection_t *srpl_connection)
{
    if (srpl_connection->instance == NULL) {
        INFO("connection " PRI_S_SRP " (%p) has no instance.", srpl_connection->name, srpl_connection);
        return NULL;
    }
    return srpl_connection->instance->domain;
}

static bool
srpl_keepalive_send(srpl_connection_t *srpl_connection, bool response, uint16_t xid)
{
    uint8_t dsobuf[SRPL_KEEPALIVE_MESSAGE_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                NULL, srpl_connection->dso->is_server, response,
                                dns_rcode_noerror, xid, srpl_connection)) {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_Keepalive);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, DEFAULT_KEEPALIVE_WAKEUP_EXPIRY / 2); // Idle timeout (we are never idle)
    dns_u32_to_wire(&towire, DEFAULT_KEEPALIVE_WAKEUP_EXPIRY / 2); // Keepalive timeout
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO("sent %zd byte " PUB_S_SRP " Keepalive, xid %02x%02x (was %04x), to " PRI_S_SRP, iov.iov_len,
         (response ? "response" : (srpl_connection->is_server
                                   ? "unidirectional"
                                   : "query")), dsobuf[0], dsobuf[1], xid, srpl_connection->name);
    srpl_message_sent(srpl_connection);
    return true;
}

// If we ever get a wakeup, it means that a wakeup send interval has passed since the last time we sent any message on
// this connection, so we should send a keepalive message.
static void
srpl_connection_keepalive_send_wakeup(void *context)
{
    srpl_connection_t *srpl_connection = context;

    // In case we lost our connection but still have keepalive timers going, now's a good time to
    // cancel them.
    if (srpl_connection->connection == NULL) {
        // Cancel keepalive timers
        if (srpl_connection->keepalive_send_wakeup) {
            ioloop_cancel_wake_event(srpl_connection->keepalive_send_wakeup);
        }
        if (srpl_connection->keepalive_receive_wakeup) {
            ioloop_cancel_wake_event(srpl_connection->keepalive_receive_wakeup);
        }
        return;
    }
    srpl_keepalive_send(srpl_connection, false, 0);
    srpl_message_sent(srpl_connection);
}

static void
srpl_message_sent(srpl_connection_t *srpl_connection)
{
    if (!srpl_connection->is_server) {
        srpl_connection->last_message_sent = srp_time();
        ioloop_add_wake_event(srpl_connection->keepalive_send_wakeup,
                              srpl_connection, srpl_connection_keepalive_send_wakeup, srpl_connection_context_release,
                              srpl_connection->keepalive_interval);
        RETAIN_HERE(srpl_connection, srpl_connection); // for the callback
    }
}

static bool
srpl_session_message_send(srpl_connection_t *srpl_connection, bool response)
{
    uint8_t dsobuf[SRPL_SESSION_MESSAGE_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
    if (domain == NULL) {
        return false;
    }
#ifdef TEST_DSO_MESSAGE_SETUP_CONNECTION_NULL_FAULT
    comm_t *connection = srpl_connection->connection;
    const char *old_null_reason = srpl_connection->connection_null_reason;
    struct timeval old_null_time = srpl_connection->connection_null_time;
    srpl_connection->connection = NULL;
    srpl_connection->connection_null_reason = "unit test";
    gettimeofday(&srpl_connection->connection_null_time, NULL);
    bool ret = srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                      srpl_connection_message_get(srpl_connection), false, response, 0, 0, srpl_connection);
    if (ret != false) {
        FAULT("failed to detect null connection!");
    }
    srpl_connection->connection = connection;
    srpl_connection->connection_null_reason = old_null_reason;
    srpl_connection->connection_null_time = old_null_time;
#endif

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                srpl_connection_message_get(srpl_connection), false, response, 0, 0, srpl_connection)) {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLSession);
    dns_rdlength_begin(&towire);
    dns_u64_to_wire(&towire, domain->partner_id);
    dns_rdlength_end(&towire);

    // version TLV
    dns_u16_to_wire(&towire, kDSOType_SRPLVersion);
    dns_rdlength_begin(&towire);
    dns_u16_to_wire(&towire, SRPL_CURRENT_VERSION);
    dns_rdlength_end(&towire);

    // domain name tlv
    dns_u16_to_wire(&towire, kDSOType_SRPLDomainName);
    dns_rdlength_begin(&towire);
    INFO("include domain " PRI_S_SRP, domain->name);
    dns_full_name_to_wire(NULL, &towire, domain->name);
    dns_rdlength_end(&towire);
    if (domain->srpl_opstate == SRPL_OPSTATE_STARTUP) {
        // new partner TLV
        dns_u16_to_wire(&towire, kDSOType_SRPLNewPartner);
        dns_rdlength_begin(&towire);
        dns_rdlength_end(&towire);
    }

    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent SRPLSession " PUB_S_SRP ", id %" PRIx64, srpl_connection->name,
         response ? "response" : "message", domain->partner_id);
    srpl_message_sent(srpl_connection);
    return true;
}

static bool
srpl_send_candidates_message_send(srpl_connection_t *srpl_connection, bool response)
{
    uint8_t dsobuf[SRPL_SEND_CANDIDATES_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                srpl_connection_message_get(srpl_connection), false, response, 0, 0, srpl_connection)) {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLSendCandidates);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent SRPLSendCandidates " PUB_S_SRP, srpl_connection->name, response ? "response" : "query");
    srpl_message_sent(srpl_connection);
    return true;
}

static bool
srpl_candidate_message_send(srpl_connection_t *srpl_connection, adv_host_t *host)
{
    uint8_t dsobuf[SRPL_CANDIDATE_MESSAGE_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;
    time_t update_time = host->update_time;

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire,
                                dsobuf, sizeof(dsobuf), NULL, false, false, 0, 0, srpl_connection)) {
        return false;
    }

    // For testing, make the update time really wrong so that signature validation fails. This will not actually
    // cause a failure unless the SRP requestor sends a time range, so really only useful for testing with the
    // mDNSResponder srp-client, not with e.g. a Thread client.
    if (host->server_state != NULL && host->server_state->break_srpl_time) {
        INFO("breaking time: %lu -> %lu", (unsigned long)update_time, (unsigned long)(update_time - 1800));
        update_time -= 1800;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLCandidate);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    dns_u16_to_wire(&towire, kDSOType_SRPLHostname);
    dns_rdlength_begin(&towire);
    dns_full_name_to_wire(NULL, &towire, host->name);
    dns_rdlength_end(&towire);
    dns_u16_to_wire(&towire, kDSOType_SRPLTimeOffset);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, (uint32_t)(srp_time() - update_time));
    dns_rdlength_end(&towire);
    dns_u16_to_wire(&towire, kDSOType_SRPLKeyID);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, host->key_id);
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent SRPLCandidate message on connection.", srpl_connection->name);
    srpl_message_sent(srpl_connection);
    return true;
}

static bool
srpl_candidate_response_send(srpl_connection_t *srpl_connection, dso_message_types_t response_type)
{
    uint8_t dsobuf[SRPL_CANDIDATE_RESPONSE_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                srpl_connection_message_get(srpl_connection), false, true, 0, 0, srpl_connection)) {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLCandidate);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    dns_u16_to_wire(&towire, response_type);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent SRPLCandidate response on connection.", srpl_connection->name);
    srpl_message_sent(srpl_connection);
    return true;
}

// Qsort comparison function for message receipt times.
static int
srpl_message_compare(const void *v1, const void *v2)
{
    const message_t *m1 = *(message_t**)v1;
    const message_t *m2 = *(message_t**)v2;
    if (m1->received_time - m2->received_time < 0) {
        return -1;
    } else if(m1->received_time - m2->received_time > 0) {
        return 1;
    } else {
        return 0;
    }
}

static bool
srpl_host_message_send(srpl_connection_t *srpl_connection, adv_host_t *host)
{
    uint8_t *dsobuf = NULL;
    size_t dsobuf_length = SRPL_HOST_MESSAGE_LENGTH;
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec *iov = NULL;
    int num_messages; // Number of SRP updates we need to send
    int iovec_count = 1, iov_cur = 0;
    message_t **messages = NULL;
    bool rv = false;

    if (host->message == NULL) {
        FAULT("no host message to send for " PRI_S_SRP " on " PRI_S_SRP ".", host->name, srpl_connection->name);
        goto out;
    }
    iovec_count++;
    num_messages = 1;

    time_t srpl_now = srp_time();

    if (SRPL_SUPPORTS(srpl_connection, SRPL_VARIATION_MULTI_HOST_MESSAGE)) {
        int num_instances = host->instances == NULL ? 0 : host->instances->num;
        for (int i = 0; i < num_instances; i++) {
            adv_instance_t *instance = host->instances->vec[i];
            if (instance != NULL) {
                if (instance->message != NULL && instance->message != host->message && instance->message != NULL) {
                    num_messages++;
                }
            }
        }
        messages = calloc(num_messages, sizeof (*messages));
        if (messages == NULL) {
            INFO("no memory for message vector");
            goto out;
        }
        messages[0] = host->message;
        num_messages = 1;
        for (int i = 0; i < num_instances; i++) {
            adv_instance_t *instance = host->instances->vec[i];
            if (instance != NULL) {
                if (instance->message != NULL && instance->message != host->message && instance->message != NULL) {
                    messages[num_messages] = instance->message;
                    num_messages++;
                }
            }
        }
        qsort(messages, num_messages, sizeof(*messages), srpl_message_compare);
        // eliminate the duplicate messages in the sorted array.
        int nondup_pos = 0;
        INFO("messages[0] = %p, received_time = %ld", messages[0], srpl_now - messages[0]->received_time);
        for (int i = 1; i < num_messages; i++) {
            if (messages[i] != messages[nondup_pos]) {
                nondup_pos++;
                messages[nondup_pos] = messages[i];
                INFO("messages[%d] = messages[%d] (%p), received_time = %ld", nondup_pos, i, messages[i],
                     srpl_now - messages[nondup_pos]->received_time);
            }
        }
        num_messages = nondup_pos + 1;
        // update iovec_count and dsobuf_length based on the number of messages.
        // nondup_pos now is the position of the last unique message and it also
        // is the number of extra host messages we have got in this SRPLHost message.
        iovec_count += 2 * nondup_pos;
        // Account for additional HostMessage TLV.
        dsobuf_length += (DSO_TLV_HEADER_SIZE + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)) * nondup_pos;
    }
    iov = calloc(iovec_count, sizeof(*iov));
    if (iov == NULL) {
        ERROR("no memory for iovec.");
        goto out;
    }
    dsobuf = malloc(dsobuf_length);
    if (dsobuf == NULL) {
        ERROR("no memory for dso buffer");
        goto out;
    }

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire,
                                dsobuf, dsobuf_length, NULL, false, false, 0, 0, srpl_connection)) {
        goto out;
    }

    // For testing, make the update time really wrong so that signature validation fails. This will not actually
    // cause a failure unless the SRP requestor sends a time range, so really only useful for testing with the
    // mDNSResponder srp-client, not with e.g. a Thread client.
    if (host->server_state != NULL && host->server_state->break_srpl_time) {
        INFO("breaking time: %lu -> %lu", (unsigned long)srpl_now, (unsigned long)(srpl_now + 1800));
        srpl_now += 1800;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLHost);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    dns_u16_to_wire(&towire, kDSOType_SRPLHostname);
    dns_rdlength_begin(&towire);
    dns_full_name_to_wire(NULL, &towire, host->name);
    dns_rdlength_end(&towire);
    // v0 of the protocol only includes one host message option, and timeoffset is sent
    // as its own secondary TLV.
    if (!SRPL_SUPPORTS(srpl_connection, SRPL_VARIATION_MULTI_HOST_MESSAGE)) {
        dns_u16_to_wire(&towire, kDSOType_SRPLTimeOffset);
        dns_rdlength_begin(&towire);
        dns_u32_to_wire(&towire, (uint32_t)(srpl_now - host->message->received_time));
        dns_rdlength_end(&towire);
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLServerStableID);
    dns_rdlength_begin(&towire);
    dns_u64_to_wire(&towire, host->server_stable_id);
    dns_rdlength_end(&towire);
    if (!SRPL_SUPPORTS(srpl_connection, SRPL_VARIATION_MULTI_HOST_MESSAGE)) {
        dns_u16_to_wire(&towire, kDSOType_SRPLHostMessage);
        dns_u16_to_wire(&towire, host->message->length);
        iov[iov_cur].iov_len = towire.p - dsobuf;
        iov[iov_cur].iov_base = dsobuf;
        iov_cur++;
        iov[iov_cur].iov_len = host->message->length;
        iov[iov_cur].iov_base = &host->message->wire;
        iov_cur++;
    } else {
        uint8_t *start = dsobuf;
        for (int i = 0; i < num_messages; i++) {
            dns_u16_to_wire(&towire, kDSOType_SRPLHostMessage);
            dns_u16_to_wire(&towire, 12 + messages[i]->length);
            dns_u32_to_wire(&towire, messages[i]->lease);
            dns_u32_to_wire(&towire, messages[i]->key_lease);
            dns_u32_to_wire(&towire, (uint32_t)(srpl_now - messages[i]->received_time));
            iov[iov_cur].iov_len = towire.p - start;
            iov[iov_cur].iov_base = start;
            iov_cur++;
            iov[iov_cur].iov_len = messages[i]->length;
            iov[iov_cur].iov_base = &messages[i]->wire;
            iov_cur++;
            start = towire.p;
        }
    }

    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        goto out;
    }

    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), iov, iov_cur)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent SRPLHost message %02x%02x " PRI_S_SRP " stable ID %" PRIx64 ", Host Message Count %d",
         srpl_connection->name, message.buf[0], message.buf[1], host->name, host->server_stable_id, num_messages);
    rv = true;
    srpl_message_sent(srpl_connection);
    out:
    if (messages != NULL) {
        free(messages);
    }
    if (iov != NULL) {
        free(iov);
    }
    if (dsobuf != NULL) {
        free(dsobuf);
    }
    return rv;
}


static bool
srpl_host_response_send(srpl_connection_t *srpl_connection, int rcode)
{
    uint8_t dsobuf[SRPL_HOST_RESPONSE_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                srpl_connection_message_get(srpl_connection), false, true, rcode, 0, srpl_connection)) {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_SRPLHost);
    dns_rdlength_begin(&towire);
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }
    INFO(PRI_S_SRP " sent SRPLHost response %02x%02x rcode %d on connection.",
         srpl_connection->name, message.buf[0], message.buf[1], rcode);
    srpl_message_sent(srpl_connection);
    return true;
}

static bool
srpl_retry_delay_send(srpl_connection_t *srpl_connection, uint32_t delay)
{
    uint8_t dsobuf[SRPL_RETRY_DELAY_LENGTH];
    dns_towire_state_t towire;
    dso_message_t message;
    struct iovec iov;
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
    if (domain == NULL) {
        ERROR("domain is NULL.");
        return false;
    }

    // If this isn't a server, there's no benefit to sending retry delay.
    if (!srpl_connection->is_server) {
        return true;
    }

    if (!srpl_dso_message_setup(srpl_connection->dso, &message, &towire, dsobuf, sizeof(dsobuf),
                                srpl_connection_message_get(srpl_connection), false, true, dns_rcode_noerror, 0,
                                srpl_connection))
    {
        return false;
    }
    dns_u16_to_wire(&towire, kDSOType_RetryDelay);
    dns_rdlength_begin(&towire);
    dns_u32_to_wire(&towire, delay); // One hour.
    dns_rdlength_end(&towire);
    if (towire.error) {
        ERROR("ran out of message space at " PUB_S_SRP ", :%d", __FILE__, towire.line);
        return false;
    }
    memset(&iov, 0, sizeof(iov));
    iov.iov_len = towire.p - dsobuf;
    iov.iov_base = dsobuf;
    if (!ioloop_send_message(srpl_connection->connection, srpl_connection_message_get(srpl_connection), &iov, 1)) {
        INFO("send failed");
        srpl_disconnect(srpl_connection);
        return false;
    }

    INFO(PRI_S_SRP " sent Retry Delay, id %" PRIx64, srpl_connection->name, domain->partner_id);
    srpl_message_sent(srpl_connection);
    return true;
}
static bool
srpl_find_dso_additionals(srpl_connection_t *srpl_connection, dso_state_t *dso, const dso_message_types_t *additionals,
                          bool *required, bool *multiple, const char **names, int *indices, int num,
                          int min_additls, int max_additls, const char *message_name, void *context,
                          bool (*iterator)(int index, const uint8_t *buf, unsigned *offp, uint16_t len, void *context))
{
    int ret = true;
    int count = 0;

    for (int j = 0; j < num; j++) {
        indices[j] = -1;
    }
    for (unsigned i = 0; i < dso->num_additls; i++) {
        bool found = false;
        for (int j = 0; j < num; j++) {
            if (dso->additl[i].opcode == additionals[j]) {
                if (indices[j] != -1 && (multiple == NULL || multiple[j] == false)) {
                    ERROR(PRI_S_SRP ": duplicate " PUB_S_SRP " for " PUB_S_SRP ".",
                          srpl_connection->name, names[j], message_name);
                    ret = false;
                    continue;
                }
                indices[j] = i;
                unsigned offp = 0;
                if (!iterator(j, dso->additl[i].payload, &offp, dso->additl[i].length, context) ||
                    offp != dso->additl[i].length)
                {
                    ERROR(PRI_S_SRP ": invalid " PUB_S_SRP " for " PUB_S_SRP ".",
                          srpl_connection->name, names[j], message_name);
                    found = true; // So we don't complain later.
                    count++;
                    ret = false;
                } else {
                    found = true;
                    count++;
                }
            }
        }
        if (!found) {
            ERROR(PRI_S_SRP ": unexpected opcode %x for " PUB_S_SRP ".",
                  srpl_connection->name, dso->additl[i].opcode, message_name);
        }
    }
    for (int j = 0; j < num; j++) {
        if (required[j] && indices[j] == -1) {
            ERROR(PRI_S_SRP ": missing " PUB_S_SRP " for " PUB_S_SRP ".",
                  srpl_connection->name, names[j], message_name);
            ret = false;
        }
    }
    if (count < min_additls) {
        ERROR(PRI_S_SRP ": not enough additional TLVs (%d < %d) for " PUB_S_SRP ".",
              srpl_connection->name, count, min_additls, message_name);
        ret = false;
    } else if (count > max_additls) {
        ERROR(PRI_S_SRP ": too many additional TLVs (%d > %d) for " PUB_S_SRP ".",
              srpl_connection->name, count, max_additls, message_name);
        ret = false;
    }
    return ret;
}

static void
srpl_connection_discontinue(srpl_connection_t *srpl_connection)
{
    srpl_connection->candidates_not_generated = true;
    // Cancel any outstanding reconnect wakeup event, so that we don't accidentally restart the connection we decided to
    // discontinue.
    if (srpl_connection->reconnect_wakeup != NULL) {
        ioloop_cancel_wake_event(srpl_connection->reconnect_wakeup);
        // We have to get rid of the wakeup here because it's holding a reference to the connection, which we may want to
        // have go away.
        ioloop_wakeup_release(srpl_connection->reconnect_wakeup);
        srpl_connection->reconnect_wakeup = NULL;
    }
    // Cancel any outstanding state timeout event.
    if (srpl_connection->state_timeout != NULL) {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        ioloop_wakeup_release(srpl_connection->state_timeout);
        srpl_connection->state_timeout = NULL;
    }
    srpl_connection_reset(srpl_connection);
    srpl_connection_next_state(srpl_connection, srpl_state_disconnect);
}

static bool
srpl_session_message_parse_in(int index, const uint8_t *buffer, unsigned *offp, uint16_t length, void *context)
{
    srpl_session_t *session = context;

    switch(index) {
    case 0:
        session->new_partner = true;
        return true;
    case 1:
        return dns_name_parse(&session->domain_name, buffer, length, offp, length);
    case 2:
        return dns_u16_parse(buffer, length, offp, &session->remote_version);
    }
    return false;
}

static bool
srpl_session_message_parse(srpl_connection_t *srpl_connection,
                           srpl_event_t *event, dso_state_t *dso, const char *message_name)
{
    const char *names[3] = { "New Partner", "Domain Name", "Protocol Version" };
    dso_message_types_t additionals[3] = { kDSOType_SRPLNewPartner, kDSOType_SRPLDomainName, kDSOType_SRPLVersion };
    bool required[3] = { false, false, false };
    bool multiple[3] = { false, false, false };
    int indices[3];

    if (dso->primary.length != 8) {
        ERROR(PRI_S_SRP ": invalid DSO Primary length %d for " PUB_S_SRP ".",
              srpl_connection->name, dso->primary.length, message_name);
        return false;
    }

    unsigned offp = 0;
    srpl_event_content_type_set(event, srpl_event_content_type_session);
    if (!dns_u64_parse(dso->primary.payload, 8, &offp, &event->content.session.partner_id)) {
        // This should be un-possible.
        ERROR(PRI_S_SRP ": invalid DSO Primary server id in " PRI_S_SRP ".",
              srpl_connection->name, message_name);
        return false;
    }

    event->content.session.new_partner = false;
    if (!srpl_find_dso_additionals(srpl_connection, dso, additionals, required, multiple, names, indices, 3, 0, 3,
                                   "SRPLSession message", &(event->content.session), srpl_session_message_parse_in)) {
        return false;
    }

    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
    if (domain == NULL) {
        ERROR("connection has no domain.");
        return false;
    }
    // If this is an unidentified connection that is associated with a temporary instance and
    // a temporary domain, we need to retrieve the domain name from the session message and
    // find the real domain for this connection.
    // A connection can not be identified due to either
    // the sending partner is still in the startup state and has not advertised yet; or
    // the sending partner is in the routine state and has advertised the domain, but
    // the receiving partner has not discovered it yet.
    DNS_NAME_GEN_SRP(event->content.session.domain_name, dname_buf);
    if (domain->name == NULL) {
        srp_server_t *server_state = domain->server_state;
        if (server_state == NULL) {
            ERROR("server state is NULL.");
            return false;
        }
        if (event->content.session.domain_name == NULL) {
            ERROR(PUB_S_SRP " does not include domain name", message_name);
            return false;
        }
        srpl_domain_t **dp, *match_domain = NULL;
        // Find the domain.
        for (dp = &server_state->srpl_domains; *dp; dp = &(*dp)->next) {
            match_domain = *dp;
            if (!strcasecmp(match_domain->name, dname_buf)) {
                break;
            }
        }
        if (match_domain == NULL) {
            ERROR("domain name in " PUB_S_SRP " does not match any domain", message_name);
            return false;
        }
        RELEASE_HERE(srpl_connection->instance->domain, srpl_domain);
        srpl_connection->instance->domain = match_domain;
        RETAIN_HERE(match_domain, srpl_domain);
    }


    if (event->content.session.remote_version >= SRPL_VERSION_MULTI_HOST_MESSAGE) {
        srpl_connection->variation_mask |= SRPL_VARIATION_MULTI_HOST_MESSAGE;
    }

    INFO(PRI_S_SRP " received " PUB_S_SRP ", id %" PRIx64 ", startup " PUB_S_SRP
         ", domain " PRI_S_SRP ", version %d", srpl_connection->name, message_name,
         event->content.session.partner_id, event->content.session.new_partner? "yes" : "no",
         dname_buf, event->content.session.remote_version);
    return true;
}

static void
srpl_session_message(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso)
{
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_session_message_received);

    srpl_connection_message_set(srpl_connection, message);
    if (!srpl_session_message_parse(srpl_connection, &event, dso, "SRPLSession message")) {
        dns_name_free(event.content.session.domain_name);
        srpl_disconnect(srpl_connection);
        return;
    }
    srpl_event_deliver(srpl_connection, &event);
    dns_name_free(event.content.session.domain_name);
}

static void
srpl_session_response(srpl_connection_t *srpl_connection, dso_state_t *dso)
{
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_session_response_received);
    if (!srpl_session_message_parse(srpl_connection, &event, dso, "SRPLSession response")) {
        srpl_disconnect(srpl_connection);
        return;
    }

    srpl_event_deliver(srpl_connection, &event);
    dns_name_free(event.content.session.domain_name);
}

static bool
srpl_send_candidates_message_parse(srpl_connection_t *srpl_connection, dso_state_t *dso, const char *message_name)
{
    if (dso->primary.length != 0) {
        ERROR(PRI_S_SRP ": invalid DSO Primary length %d for " PUB_S_SRP ".",
              srpl_connection->name, dso->primary.length, message_name);
        srpl_disconnect(srpl_connection);
        return false;
    }
    return true;
}

static void
srpl_send_candidates_message(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso)
{
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_send_candidates_message_received);

    srpl_connection_message_set(srpl_connection, message);
    if (srpl_send_candidates_message_parse(srpl_connection, dso, "SRPLSendCandidates message")) {
        INFO(PRI_S_SRP " received SRPLSendCandidates query", srpl_connection->name);

        srpl_event_deliver(srpl_connection, &event);
        return;
    }
    srpl_disconnect(srpl_connection);
}

static void
srpl_send_candidates_response(srpl_connection_t *srpl_connection, dso_state_t *dso)
{
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_send_candidates_response_received);

    if (srpl_send_candidates_message_parse(srpl_connection, dso, "SRPLSendCandidates message")) {
        INFO(PRI_S_SRP " received SRPLSendCandidates response", srpl_connection->name);
        srpl_event_deliver(srpl_connection, &event);
        return;
    }
}

static bool
srpl_candidate_message_parse_in(int index, const uint8_t *buffer, unsigned *offp, uint16_t length, void *context)
{
    srpl_candidate_t *candidate = context;

    switch(index) {
    case 0:
        return dns_name_parse(&candidate->name, buffer, length, offp, length);
    case 1:
        return dns_u32_parse(buffer, length, offp, &candidate->update_offset);
    case 2:
        return dns_u32_parse(buffer, length, offp, &candidate->key_id);
    }
    return false;
}

static void
srpl_candidate_message(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso)
{
    const char *names[3] = { "Candidate Name", "Time Offset", "Key ID" };
    dso_message_types_t additionals[3] = { kDSOType_SRPLHostname, kDSOType_SRPLTimeOffset, kDSOType_SRPLKeyID };
    bool required[3] = { true, true, true };
    int indices[3];

    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_candidate_received);
    srpl_connection_message_set(srpl_connection, message);
    if (!srpl_event_content_type_set(&event, srpl_event_content_type_candidate) ||
        !srpl_find_dso_additionals(srpl_connection, dso, additionals,
                                   required, NULL, names, indices, 3, 3, 3, "SRPLCandidate message",
                                   event.content.candidate, srpl_candidate_message_parse_in)) {
        goto fail;
    }

    event.content.candidate->update_time = srp_time() - event.content.candidate->update_offset;
    srpl_event_deliver(srpl_connection, &event);
    srpl_event_content_type_set(&event, srpl_event_content_type_none);
    return;

fail:
    srpl_disconnect(srpl_connection);
}

static bool
srpl_candidate_response_parse_in(int index,
                                 const uint8_t *UNUSED buffer, unsigned *offp, uint16_t length, void *context)
{
    srpl_candidate_disposition_t *candidate_disposition = context;

    if (length != 0) {
        return false;
    }

    switch(index) {
    case 0:
        *candidate_disposition = srpl_candidate_yes;
        break;
    case 1:
        *candidate_disposition = srpl_candidate_no;
        break;
    case 2:
        *candidate_disposition = srpl_candidate_conflict;
        break;
    }
    *offp = 0;
    return true;
}

static void
srpl_candidate_response(srpl_connection_t *srpl_connection, dso_state_t *dso)
{
    const char *names[3] = { "Candidate Yes", "Candidate No", "Conflict" };
    dso_message_types_t additionals[3] = { kDSOType_SRPLCandidateYes, kDSOType_SRPLCandidateNo, kDSOType_SRPLConflict };
    bool required[3] = { false, false, false };
    int indices[3];
    srpl_event_t event;

    srpl_event_initialize(&event, srpl_event_candidate_response_received);
    srpl_event_content_type_set(&event, srpl_event_content_type_candidate_disposition);
    if (!srpl_find_dso_additionals(srpl_connection, dso, additionals,
                                   required, NULL, names, indices, 3, 1, 1, "SRPLCandidate reply",
                                   &event.content.disposition, srpl_candidate_response_parse_in)) {
        goto fail;
    }
    srpl_event_deliver(srpl_connection, &event);
    return;

fail:
    srpl_disconnect(srpl_connection);
}

static bool
srpl_host_message_parse_in(int index, const uint8_t *buffer, unsigned *offp, uint16_t length, void *context)
{
    srpl_host_update_t *update = context;

    switch(index) {
    case 0: // Host Name
        if (update->hostname == NULL) {
            unsigned offp_orig = *offp;
            bool ret = dns_name_parse(&update->hostname, buffer, length, offp, length);
            update->num_bytes = *offp - offp_orig;
            update->orig_buffer = (intptr_t)buffer;
            return ret;
        } else {
            if ((intptr_t)buffer == update->orig_buffer) {
                (*offp) += update->num_bytes;
            }
            return true;
        }
    case 1: // Host Message
        if (update->messages != NULL) {
            const uint8_t *message_buffer;
            size_t message_length;
            if (update->rcode) {
                message_buffer = buffer + 12; // lease, key-lease, time offset
                message_length = length - 12;
            } else {
                message_buffer = buffer;
                message_length = length;
            }
            message_t *message = ioloop_message_create(message_length);
            if (message == NULL) {
                return false;
            }
            if (update->rcode) {
                uint32_t time_offset = 0;
                if (!(dns_u32_parse(buffer, length, offp, &message->lease) &&
                      dns_u32_parse(buffer, length, offp, &message->key_lease) &&
                      dns_u32_parse(buffer, length, offp, &time_offset)))
                {
                    INFO("failed to parse lease, key_lease or time_offset");
                    return false;
                }
                message->received_time = srp_time() - time_offset;
            }
            memcpy(&message->wire, message_buffer, message_length);

            // We are parsing across the same message, so we can't exceed max_messages here.
            update->messages[update->num_messages++] = message;
        } else {
            update->max_messages++;
        }
        *offp = length;
        return true;
    case 2: // Server Stable ID
        return dns_u64_parse(buffer, length, offp, &update->server_stable_id);
    case 3: // Time Offset
        return dns_u32_parse(buffer, length, offp, &update->update_offset);
    }
    return false;
}

static void
srpl_host_message(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso)
{
    srpl_event_t event;
    memset(&event, 0, sizeof(event));
    srpl_connection_message_set(srpl_connection, message);
    if (dso->primary.length != 0) {
        ERROR(PRI_S_SRP ": invalid DSO Primary length %d for SRPLHost message.",
              srpl_connection->name, dso->primary.length);
        goto fail;
    } else {
        const char *names[4] = { "Host Name", "Host Message", "Server Stable ID", "Time Offset" };
        dso_message_types_t additionals[4] = { kDSOType_SRPLHostname, kDSOType_SRPLHostMessage,
            kDSOType_SRPLServerStableID, kDSOType_SRPLTimeOffset };
        bool required[4] = { true, true, false, true };
        bool multiple[4] = { false, true, false, false };
        int indices[4];
        int num_additls = 4;

        // Parse host message
        srpl_event_initialize(&event, srpl_event_host_message_received);
        srpl_event_content_type_set(&event, srpl_event_content_type_host_update);
        if (SRPL_SUPPORTS(srpl_connection, SRPL_VARIATION_MULTI_HOST_MESSAGE)) {
            num_additls--;
            event.content.host_update.rcode = 1; // temporarily use rcode for flag
        }

        if (!srpl_find_dso_additionals(srpl_connection, dso, additionals, required, multiple, names, indices,
                                       num_additls, num_additls - 1, num_additls + MAX_ADDITIONAL_HOST_MESSAGES,
                                       "SRPLHost message", &event.content.host_update, srpl_host_message_parse_in)) {
            goto fail;
        }
        // update->max_messages can't be zero here, or we would have gotten a false return from
        // srpl_find_dso_additionals and not gotten here.
        event.content.host_update.messages = calloc(event.content.host_update.max_messages,
                                                    sizeof (*event.content.host_update.messages));
        if (event.content.host_update.messages == NULL) {
            goto fail;
        }
        // Now that we know how many messages, we can copy them out.
        if (!srpl_find_dso_additionals(srpl_connection, dso, additionals, required, multiple, names, indices,
                                       num_additls, num_additls - 1, num_additls + MAX_ADDITIONAL_HOST_MESSAGES,
                                       "SRPLHost message", &event.content.host_update, srpl_host_message_parse_in)) {
            goto fail;
        }
        DNS_NAME_GEN_SRP(event.content.host_update.hostname, hostname_buf);
        if (!SRPL_SUPPORTS(srpl_connection, SRPL_VARIATION_MULTI_HOST_MESSAGE)) {
            time_t update_time = srp_time() - event.content.host_update.update_offset;
            event.content.host_update.messages[0]->received_time = update_time;
            INFO(PRI_S_SRP " received SRPLHost message %x for " PRI_DNS_NAME_SRP " server stable ID %" PRIx64
                 " update offset = %d", srpl_connection->name, ntohs(message->wire.id),
                 DNS_NAME_PARAM_SRP(event.content.host_update.hostname, hostname_buf),
                 event.content.host_update.server_stable_id, event.content.host_update.update_offset);
        } else {
            // Make sure times are sequential.
            time_t last_received_time = event.content.host_update.messages[0]->received_time;
            INFO(PRI_S_SRP " received SRPLHost message %x for " PRI_DNS_NAME_SRP " server stable ID %" PRIx64
                 " message 0 received time = %ld", srpl_connection->name, ntohs(message->wire.id),
                 DNS_NAME_PARAM_SRP(event.content.host_update.hostname, hostname_buf),
                 event.content.host_update.server_stable_id, srp_time() - last_received_time);
            for (int i = 1; i < event.content.host_update.num_messages; i++) {
                time_t cur_received_time = event.content.host_update.messages[i]->received_time;
                if (cur_received_time - last_received_time <= 0) {
                    INFO(PRI_S_SRP
                         " received invalid SRPLHost message %x with message %d time %lld <= message %d time %lld",
                         srpl_connection->name, ntohs(event.content.host_update.messages[i]->wire.id),
                         i, (long long)cur_received_time, i - 1, (long long)last_received_time);
                    goto fail_no_message;
                }
                INFO("message %d received time = %ld", i, cur_received_time);
                last_received_time = cur_received_time;
            }
        }
        event.content.host_update.rcode = 0;
        srpl_event_deliver(srpl_connection, &event);
        srpl_event_content_type_set(&event, srpl_event_content_type_none);
    }
    return;
fail:
    INFO(PRI_S_SRP " received invalid SRPLHost message %x", srpl_connection->name, ntohs(message->wire.id));
fail_no_message:
    if (event.content_type == srpl_event_content_type_host_update) {
        srpl_event_content_type_set(&event, srpl_event_content_type_none);
    }
    srpl_disconnect(srpl_connection);
    return;
}

static void
srpl_host_response(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso)
{
    if (dso->primary.length != 0) {
        ERROR(PRI_S_SRP ": invalid DSO Primary length %d for SRPLHost response.",
              srpl_connection->name, dso->primary.length);
        srpl_disconnect(srpl_connection);
        return;
    } else {
        srpl_event_t event;
        INFO(PRI_S_SRP " received SRPLHost response %x", srpl_connection->name, ntohs(message->wire.id));
        srpl_event_initialize(&event, srpl_event_host_response_received);
        srpl_event_content_type_set(&event, srpl_event_content_type_rcode);
        event.content.rcode = dns_rcode_get(&message->wire);
        srpl_event_deliver(srpl_connection, &event);
        srpl_event_content_type_set(&event, srpl_event_content_type_none);
        return;
    }
}

static void
srpl_keepalive_receive(srpl_connection_t *srpl_connection, int keepalive_interval, uint16_t xid)
{
    if (srpl_connection->is_server) {
        int num_standby = 0;
        srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
        if (domain != NULL && domain->server_state != NULL) {
            for (srpl_instance_t *unmatched = domain->server_state->unmatched_instances; unmatched != NULL; unmatched = unmatched->next) {
                srpl_connection_t *unidentified = unmatched->connection;
                if (unidentified != NULL && unidentified->state > srpl_state_session_evaluate) {
                    num_standby++;
                }
            }
        }
        int new_interval = num_standby * DEFAULT_KEEPALIVE_WAKEUP_EXPIRY / 2;
        if (new_interval > 0 && srpl_connection->keepalive_interval != new_interval) {
            srpl_connection->keepalive_interval = new_interval;
            INFO("suggest keepalive %d for connection " PRI_S_SRP, new_interval, srpl_connection->name);
        }
        srpl_keepalive_send(srpl_connection, true, xid);
    } else {
        if (srpl_connection->state <= srpl_state_sync_wait) {
            INFO("keepalive for connection " PRI_S_SRP " - old %d, new %d.", srpl_connection->name,
                 srpl_connection->keepalive_interval, keepalive_interval);
            srpl_connection->keepalive_interval = keepalive_interval;
        }
    }
}

static void
srpl_dso_retry_delay(srpl_connection_t *srpl_connection, int reconnect_delay)
{
    if (srpl_connection->instance == NULL) {
        // If there's no instance, we're already disconnecting.
        INFO(PRI_S_SRP ": no instance", srpl_connection->name);
        return;
    }
    srpl_instance_t *instance = srpl_connection->instance;
    RETAIN_HERE(srpl_connection, srpl_connection); // In case there's only one reference left.
    if (instance->unmatched) {
        INFO(PRI_S_SRP ": not sending retry delay for %d seconds because unidentified", srpl_connection->name, reconnect_delay);
        if (instance->connection == srpl_connection) {
            RELEASE_HERE(instance->connection, srpl_connection);
            instance->connection = NULL;
        }
    } else {
        INFO(PRI_S_SRP ": sending retry delay for %d seconds", srpl_connection->name, reconnect_delay);

        // Set things up to reconnect later.
        srpl_connection_drop_state_delay(instance, srpl_connection, reconnect_delay);
    }

    // Drop the connection
    srpl_connection_discontinue(srpl_connection);
    RELEASE_HERE(srpl_connection, srpl_connection); // For the function.
}

static void
srpl_dso_message(srpl_connection_t *srpl_connection, message_t *message, dso_state_t *dso, bool response)
{
    const char *name = "<null>";
    if (srpl_connection != NULL) {
        if (srpl_connection->instance != NULL) {
            name = srpl_connection->instance->instance_name;
        } else {
            name = srpl_connection->name;
        }
    }

    switch(dso->primary.opcode) {
    case kDSOType_SRPLSession:
        if (response) {
            srpl_session_response(srpl_connection, dso);
        } else {
            srpl_session_message(srpl_connection, message, dso);
        }
        break;

    case kDSOType_SRPLSendCandidates:
        if (response) {
            srpl_send_candidates_response(srpl_connection, dso);
        } else {
            srpl_send_candidates_message(srpl_connection, message, dso);
        }
        break;

    case kDSOType_SRPLCandidate:
        if (response) {
            srpl_candidate_response(srpl_connection, dso);
        } else {
            srpl_candidate_message(srpl_connection, message, dso);
        }
        break;

    case kDSOType_SRPLHost:
        if (response) {
            srpl_host_response(srpl_connection, message, dso);
        } else {
            srpl_host_message(srpl_connection, message, dso);
        }
        break;

    case kDSOType_Keepalive:
        if (response) {
            INFO(PRI_S_SRP ": keepalive response, xid %04x", name, message->wire.id);
        } else if (message->wire.id) {
            INFO(PRI_S_SRP ": keepalive query, xid %04x", name, message->wire.id);
        } else {
            INFO(PRI_S_SRP ": keepalive unidirectional, xid %04x (should be zero)", name, message->wire.id);
        }
        break;

    default:
        INFO(PRI_S_SRP ": unexpected primary TLV %d", name, dso->primary.opcode);
        dso_simple_response(srpl_connection->connection, NULL, &message->wire, dns_rcode_dsotypeni);
        break;
    }

}

// We should never get here. If we do, it means that we haven't gotten a keepalive in the required interval.
static void
srpl_keepalive_receive_wakeup(void *context)
{
    srpl_connection_t *srpl_connection = context;

    INFO(PUB_S_SRP ": nothing heard from partner across keepalive interval--disconnecting", srpl_connection->name);
    srpl_connection_discontinue(srpl_connection); // Drop the connection, don't send a retry_delay.
}

static void
srpl_instance_dso_event_callback(void *context, void *event_context, dso_state_t *dso, dso_event_type_t eventType)
{
    message_t *message;
    dso_query_receive_context_t *response_context;
    dso_disconnect_context_t *disconnect_context;
    dso_keepalive_context_t *keepalive_context;
    srpl_connection_t *srpl_connection = context;
    const char *name = "<null>";
    if (srpl_connection != NULL) {
        if (srpl_connection->instance != NULL) {
            name = srpl_connection->instance->instance_name;
        } else {
            name = srpl_connection->name;
        }
    }

    switch(eventType)
    {
    case kDSOEventType_DNSMessage:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO(PRI_S_SRP ": DNS Message (opcode=%d) received from " PRI_S_SRP,
             name, dns_opcode_get(&message->wire), dso->remote_name);
        break;
    case kDSOEventType_DNSResponse:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO(PRI_S_SRP ": DNS Response (opcode=%d) received from " PRI_S_SRP,
             name, dns_opcode_get(&message->wire), dso->remote_name);
        break;
    case kDSOEventType_DSOMessage:
        INFO(PRI_S_SRP ": DSO Message (Primary TLV=%d) received from " PRI_S_SRP,
             name, dso->primary.opcode, dso->remote_name);
        srpl_connection->last_message_received = srp_time();
        ioloop_add_wake_event(srpl_connection->keepalive_receive_wakeup,
                              srpl_connection, srpl_keepalive_receive_wakeup, srpl_connection_context_release,
                              srpl_connection->keepalive_interval * 2);
        RETAIN_HERE(srpl_connection, srpl_connection); // for the callback
        message = event_context;
        srpl_dso_message(srpl_connection, message, dso, false);
        break;
    case kDSOEventType_DSOResponse:
        INFO(PRI_S_SRP ": DSO Response (Primary TLV=%d) received from " PRI_S_SRP,
             name, dso->primary.opcode, dso->remote_name);
        srpl_connection->last_message_received = srp_time();
        ioloop_add_wake_event(srpl_connection->keepalive_receive_wakeup,
                              srpl_connection, srpl_keepalive_receive_wakeup, srpl_connection_context_release,
                              srpl_connection->keepalive_interval * 2);
        RETAIN_HERE(srpl_connection, srpl_connection); // for the callback
        response_context = event_context;
        message = response_context->message_context;
        srpl_dso_message(srpl_connection, message, dso, true);
        break;

    case kDSOEventType_Finalize:
        INFO("Finalize");
        break;

    case kDSOEventType_Connected:
        INFO("Connected to " PRI_S_SRP, dso->remote_name);
        break;

    case kDSOEventType_ConnectFailed:
        INFO("Connection to " PRI_S_SRP " failed", dso->remote_name);
        break;

    case kDSOEventType_Disconnected:
        INFO("Connection to " PRI_S_SRP " disconnected", dso->remote_name);
        break;
    case kDSOEventType_ShouldReconnect:
        INFO("Connection to " PRI_S_SRP " should reconnect (not for a server)", dso->remote_name);
        break;
    case kDSOEventType_Inactive:
        INFO(PRI_S_SRP "Inactivity timer went off, closing connection.", name);
        break;
    case kDSOEventType_Keepalive:
        INFO("should send a keepalive now.");
        break;
    case kDSOEventType_KeepaliveRcvd:
        keepalive_context = event_context;
        keepalive_context->send_response = false;
        INFO(PRI_S_SRP ": keepalive received, xid %04x.", name, keepalive_context->xid);
        srpl_keepalive_receive(srpl_connection, keepalive_context->keepalive_interval, keepalive_context->xid);
        srpl_connection->last_message_received = srp_time();
        ioloop_add_wake_event(srpl_connection->keepalive_receive_wakeup,
                              srpl_connection, srpl_keepalive_receive_wakeup, srpl_connection_context_release,
                              srpl_connection->keepalive_interval * 2);
        RETAIN_HERE(srpl_connection, srpl_connection); // for the callback
        break;
    case kDSOEventType_RetryDelay:
        disconnect_context = event_context;
        INFO(PRI_S_SRP ": retry delay received, %d seconds", name, disconnect_context->reconnect_delay);
        srpl_dso_retry_delay(srpl_connection, disconnect_context->reconnect_delay);
        break;
    }
}

static void
srpl_datagram_callback(comm_t *comm, message_t *message, void *context)
{
    srpl_connection_t *srpl_connection = context;
    srpl_instance_t *instance = srpl_connection->instance;
    if (instance == NULL) {
        INFO("datagram on connection " PRI_S_SRP " with no instance.", comm->name);
        return;
    }

    // If this is a DSO message, see if we have a session yet.
    switch(dns_opcode_get(&message->wire)) {
    case dns_opcode_dso:
        if (srpl_connection->dso == NULL) {
            INFO("dso message received with no DSO object on instance " PRI_S_SRP, instance->instance_name);
            srpl_disconnect(srpl_connection);
            return;
        }
        dso_message_received(srpl_connection->dso, (uint8_t *)&message->wire, message->length, message);
        return;
        break;
    }
    INFO("datagram on connection " PRI_S_SRP " not handled, type = %d.",
         comm->name, dns_opcode_get(&message->wire));
}

static void
srpl_connection_dso_cleanup(void *UNUSED context)
{
    dso_cleanup(false);
}

// Call this to break the current srpl_connection without sending the state machine into idle.
static void
srpl_trigger_disconnect(srpl_connection_t *srpl_connection)
{
    // Trigger a disconnect
    if (srpl_connection->dso != NULL) {
        dso_state_cancel(srpl_connection->dso);
        srpl_connection->dso = NULL;
    } else {
        ioloop_comm_cancel(srpl_connection->connection);
        ioloop_comm_release(srpl_connection->connection);
        gettimeofday(&srpl_connection->connection_null_time, NULL);
        srpl_connection->connection_null_reason = "trigger_disconnect";
        srpl_connection->connection = NULL;
    }
}

static bool
srpl_connection_dso_life_cycle_callback(dso_life_cycle_t cycle, void *const context, dso_state_t *const dso)
{
    if (cycle == dso_life_cycle_cancel) {
        srpl_connection_t *srpl_connection = context;
        INFO(PRI_S_SRP ": %p %p", srpl_connection->name, srpl_connection, dso);
        if (srpl_connection->connection != NULL) {
            ioloop_comm_cancel(srpl_connection->connection);
            srpl_connection->connection->dso = NULL;
            ioloop_comm_release(srpl_connection->connection);
            gettimeofday(&srpl_connection->connection_null_time, NULL);
            srpl_connection->connection_null_reason = "dso_life_cycle_callback";
            srpl_connection->connection = NULL;
        }
        srpl_connection_reset(srpl_connection);
        srpl_connection->dso = NULL;
        RELEASE_HERE(srpl_connection, srpl_connection);
        ioloop_run_async(srpl_connection_dso_cleanup, NULL);
        return true;
    }
    return false;
}

static void
srpl_associate_incoming_with_instance(comm_t *connection, message_t *message,
                                      dso_state_t *dso, srpl_instance_t *instance)
{
    srpl_connection_t *old_connection = NULL;

    srpl_connection_t *srpl_connection = srpl_connection_create(instance, false);
    if (srpl_connection == NULL) {
        ioloop_comm_cancel(connection);
        return;
    }


    srpl_connection->connection = connection;
    ioloop_comm_retain(srpl_connection->connection);

    srpl_connection->dso = dso;
    srpl_connection->instance = instance;
    srpl_connection->connected_address = connection->address;
    srpl_connection->state = srpl_state_session_message_wait;

    dso_set_event_context(dso, srpl_connection);
    RETAIN_HERE(srpl_connection, srpl_connection); // dso holds reference.
    dso_set_event_callback(dso, srpl_instance_dso_event_callback);
    dso_set_life_cycle_callback(dso, srpl_connection_dso_life_cycle_callback);

    connection->datagram_callback = srpl_datagram_callback;
    connection->disconnected = srpl_disconnected_callback;
    ioloop_comm_context_set(connection, srpl_connection, srpl_connection_context_release);
    RETAIN_HERE(srpl_connection, srpl_connection); // the connection has a reference.

    srpl_connection_next_state(srpl_connection, srpl_state_session_message_wait);
    srpl_instance_dso_event_callback(srpl_connection, message, dso, kDSOEventType_DSOMessage);

    // We drop it after we set it up because that lets us send a retry_delay to the peer.
    if (instance->domain == NULL || instance->domain->srpl_opstate != SRPL_OPSTATE_ROUTINE) {
        INFO(PRI_S_SRP ": dropping peer reconnect because we aren't in routine state.", instance->instance_name);
        RELEASE_HERE(instance, srpl_instance);
        srpl_connection->instance = NULL;
        srpl_connection_discontinue(srpl_connection);
        RELEASE_HERE(srpl_connection, srpl_connection);
        return;
    }

    // If we already have a connection with the remote partner, we replace it with the new connection.
    if (instance->connection != NULL) {
        INFO(PRI_S_SRP ": we already have a connection (%p).", instance->instance_name, old_connection);
        old_connection = instance->connection;
        RELEASE_HERE(old_connection->instance, srpl_instance);
        old_connection->instance = NULL;
        srpl_connection_discontinue(old_connection);
        RELEASE_HERE(old_connection, srpl_connection);
    }
    instance->connection = srpl_connection; // Retained via create/copy rule.
}

static void
srpl_add_unidentified_server(comm_t *connection, message_t *message, dso_state_t *dso, srp_server_t *server_state)
{
    srpl_instance_t **inp, *unmatched_instance = NULL;

    const char *instance_name;
    char nbuf[kDNSServiceMaxDomainName];
    // Take ip address as the instance name
    if (connection->address.sa.sa_family == AF_INET6) {
        instance_name = inet_ntop(AF_INET6, &connection->address.sin6.sin6_addr, nbuf, sizeof nbuf);
    } else {
        instance_name = inet_ntop(AF_INET, &connection->address.sin.sin_addr, nbuf, sizeof nbuf);
    }

    // Check if an unmatched instance has been created for the same address
    for (inp = &server_state->unmatched_instances; *inp != NULL; inp = &(*inp)->next) {
        if (!strcmp((*inp)->instance_name, instance_name)) {
            INFO("we already have an unmatched instance " PRI_S_SRP, instance_name);
            unmatched_instance = *inp;
            break;
        }
    }
    if (unmatched_instance == NULL) {
        INFO("create a temporary instance " PRI_S_SRP, instance_name);
        unmatched_instance = calloc(1, sizeof(*unmatched_instance));
        if (unmatched_instance == NULL) {
            ERROR("no memory for unmatched instance");
            return;
        }
        RETAIN_HERE(unmatched_instance, srpl_instance); // The unmatched instance list will hold this reference.
        // Create a dummy domain because domain can not be decided at this point
        srpl_domain_t *domain = calloc(1, sizeof(*domain));
        if (domain == NULL) {
            ERROR("no memory for domain structure");
            RELEASE_HERE(unmatched_instance, srpl_instance);
            return;
        }
        RETAIN_HERE(domain, srpl_domain);
        unmatched_instance->domain = domain;
        domain->server_state = server_state;
        unmatched_instance->unmatched = true;

        unmatched_instance->instance_name = strdup(instance_name);
        if (unmatched_instance->instance_name == NULL) {
            ERROR("no memory for unmatched instance" PRI_S_SRP, instance_name);
            RELEASE_HERE(unmatched_instance, srpl_instance);
            return;
        }
        // Find the end of the list and append the newly created instance.
        for (inp = &server_state->unmatched_instances; *inp != NULL; inp = &(*inp)->next) {
        }
        *inp = unmatched_instance;
    }
    srpl_associate_incoming_with_instance(connection, message, dso, unmatched_instance);
}

static void
srpl_match_unidentified_with_instance(srpl_connection_t *connection,
                                      srpl_instance_t *instance)
{
    srpl_instance_t *cur = connection->instance;

    // Take the connection from its instance.
    RETAIN_HERE(connection, srpl_connection); // for the function
    RELEASE_HERE(cur->connection, srpl_connection);
    cur->connection = NULL;
    RELEASE_HERE(connection->instance, srpl_instance); // Get rid of the instance's reference to the connection
    connection->instance = NULL;

    // Remove the currently associated instance from the unmatched_instances
    srpl_instance_t **sp = NULL;

    INFO("matched temporary instance " PRI_S_SRP " to instance " PRI_S_SRP " with partner_id %" PRIx64,
         cur->instance_name, instance->instance_name, instance->partner_id);
    instance->version_mismatch = cur->version_mismatch;
#define POINTER_TO_HEX_MAX_STRLEN 19 // 0x<...>
    size_t srpl_connection_name_length = strlen(instance->instance_name) + 2 + POINTER_TO_HEX_MAX_STRLEN + 3;
    char *new_name = malloc(srpl_connection_name_length);
    if (new_name != NULL) {
        free(connection->name);
        connection->name = new_name;
        // unidentified connection is by definition an incoming connection.
        snprintf(new_name, srpl_connection_name_length, "<%s (%p)", instance->instance_name, connection);
    }
    srp_server_t *server_state = cur->domain->server_state;
    for (sp = &server_state->unmatched_instances; *sp; sp = &(*sp)->next) {
        if (*sp == cur) {
            *sp = cur->next;
            break;
        }
    }

    // Release the unmatched instance list's reference to the unmatched instance. We already released the srpl_connection's
    // reference, so this should dispose of the unmatched instance.
    RELEASE_HERE(cur, srpl_instance);

    if (instance->domain == NULL || instance->domain->srpl_opstate != SRPL_OPSTATE_ROUTINE) {
        INFO(PRI_S_SRP "dropping peer reconnect because we aren't in routine state.", instance->domain->name);
        srpl_disconnect(connection);
        goto out;
    }

    if (connection->state == srpl_state_idle ||
        connection->state == srpl_state_disconnect ||
        connection->state == srpl_state_disconnect_wait)
    {
        INFO("connection " PRI_S_SRP " is in " PUB_S_SRP, connection->name, srpl_state_name(connection->state));
        goto out;
    }

    if (connection->database_synchronized) {
        srpl_state_transition_by_dataset_id(instance->domain, instance);
    }

    // Release any older connection we might have.
    if (instance->connection) {
        srpl_connection_t *old_connection = instance->connection;
        if (old_connection->instance != NULL) {
            RELEASE_HERE(old_connection->instance, srpl_instance);
            old_connection->instance = NULL;
        }
        srpl_disconnect(instance->connection);
        RELEASE_HERE(instance->connection, srpl_connection); // Instance still holds reference.
        instance->connection = NULL;
        INFO("release connection on instance " PRI_S_SRP " with partner_id %" PRIx64, instance->instance_name, instance->partner_id);
    }
    instance->connection = connection;
    RETAIN_HERE(instance->connection, srpl_connection);
    connection->instance = instance;
    RETAIN_HERE(connection->instance, srpl_instance); // Retain on the connection
out:
    RELEASE_HERE(connection, srpl_connection); // done with using it for this function.
}

void
srpl_dso_server_message(comm_t *connection, message_t *message, dso_state_t *dso, srp_server_t *server_state)
{
    srpl_domain_t *domain;
    srpl_instance_t *instance;
    srpl_instance_service_t *service;
    address_query_t *address;
    int i;

    // Figure out from which instance this connection originated
    for (domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        for (instance = domain->instances; instance != NULL; instance = instance->next) {
            for (service = instance->services; service != NULL; service = service->next) {
                address = service->address_query;
                if (address == NULL) {
                    continue;
                }
                for (i = 0; i < address->num_addresses; i++) {
                    if (ip_addresses_equal(&connection->address, &address->addresses[i])) {
                        INFO("SRP Replication connection received from " PRI_S_SRP " on " PRI_S_SRP,
                             address->hostname, connection->name);
                        srpl_associate_incoming_with_instance(connection, message, dso, instance);
                        return;
                    }
                }
            }
        }
    }

    INFO("incoming SRP Replication server connection from unrecognized server " PRI_S_SRP, connection->name);
    srpl_add_unidentified_server(connection, message, dso, server_state);
}

static void
srpl_connected(comm_t *connection, void *context)
{
    srpl_connection_t *srpl_connection = context;

    INFO(PRI_S_SRP " connected", connection->name);
    connection->dso = dso_state_create(false, 2, connection->name, srpl_instance_dso_event_callback,
                                       srpl_connection, srpl_connection_dso_life_cycle_callback, connection);
    if (connection->dso == NULL) {
        ERROR(PRI_S_SRP " can't create dso state object.", srpl_connection->name);
        srpl_disconnect(srpl_connection);
        return;
    }
    RETAIN_HERE(srpl_connection, srpl_connection); // dso holds reference to connection
    srpl_connection->dso = connection->dso;

    // Generate an event indicating that we've been connected
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_connected);
    srpl_event_deliver(srpl_connection, &event);
}

static bool
srpl_connection_connect(srpl_connection_t *srpl_connection)
{
    if (srpl_connection->instance == NULL) {
        ERROR(PRI_S_SRP ": no instance to connect to", srpl_connection->name);
        return false;
    }
    srpl_connection->connection = ioloop_connection_create(&srpl_connection->connected_address,
                                                           // tls, stream, stable, opportunistic
                                                             true,   true,   true, true,
                                                           srpl_datagram_callback, srpl_connected,
                                                           srpl_disconnected_callback, srpl_connection_context_release,
                                                           srpl_connection);
    if (srpl_connection->connection == NULL) {
        ADDR_NAME_LOGGER(ERROR, &srpl_connection->connected_address, "can't create connection to address ",
                         " for srpl connection ", " port ", srpl_connection->name,
                         srpl_connection->connected_address.sa.sa_family == AF_INET ?
                         srpl_connection->connected_address.sin.sin_port: srpl_connection->connected_address.sin6.sin6_port);
        return false;
    }
    srpl_connection->is_server = false;
    ADDR_NAME_LOGGER(INFO, &srpl_connection->connected_address, "connecting to address ", " for instance ", " port ",
                     srpl_connection->name, srpl_connection->connected_address.sa.sa_family == AF_INET ?
                     srpl_connection->connected_address.sin.sin_port: srpl_connection->connected_address.sin6.sin6_port);
    RETAIN_HERE(srpl_connection, srpl_connection); // For the connection's reference
    return true;
}

static void
srpl_instance_is_me(srpl_instance_t *instance, srpl_instance_service_t *service, const char *ifname, const addr_t *address, bool pid_match)
{
    instance->is_me = true;
    if (ifname != NULL) {
        INFO(PUB_S_SRP "/" PUB_S_SRP ": name server for service " PRI_S_SRP " is me.", service->host_name, ifname, service->full_service_name);
    } else if (address != NULL) {
        ADDR_NAME_LOGGER(INFO, address, "", " service ", " is me. ", service->host_name, 0);
    } else if (pid_match) {
        INFO(PUB_S_SRP ": partner id %" PRIx64 "match.", instance->instance_name, instance->partner_id);
    } else {
        ERROR("null ifname and address, partner id doesn't match!");
        return;
    }

    // When we create the instance, we start an outgoing connection; when we discover that this is a connection
    // to me, we can discontinue that outgoing connection.
    if (instance->connection && !instance->connection->is_server) {
        srpl_connection_discontinue(instance->connection);
    }
}

static bool
srpl_my_address_check(srp_server_t *server_state, const addr_t *address)
{
    static interface_address_state_t *ifaddrs = NULL;
    interface_address_state_t *ifa;
    static time_t last_fetch = 0;
    // Update the interface address list every sixty seconds, but only if we're asked to check an address.
    const time_t now = srp_time();
    if (last_fetch == 0 || now - last_fetch > 60) {
        last_fetch = now;
        ioloop_map_interface_addresses_here(server_state, &ifaddrs, NULL, NULL, NULL);
    }
    // See if there's a match.
    for (ifa = ifaddrs; ifa; ifa = ifa->next) {
        if (ip_addresses_equal(address, &ifa->addr)) {
            return true;
        }
    }
    return false;
}

static void
srpl_instance_address_callback(void *context, addr_t *address, bool added, bool more, int err)
{
    srpl_instance_service_t *service = context;
    srpl_instance_t *instance = service->instance;
    if (err != kDNSServiceErr_NoError) {
        ERROR("service instance address resolution for " PRI_S_SRP " failed with %d", service->host_name, err);
        if (service->address_query) {
            address_query_cancel(service->address_query);
            RELEASE_HERE(service->address_query, address_query);
            service->address_query = NULL;
        }
        return;
    }

    if (added) {
        bool matched_unidentified = false;
        srp_server_t *server_state = service->domain->server_state;
        srpl_instance_t **up = &server_state->unmatched_instances;
        while (*up != NULL) {
            srpl_instance_t *unmatched_instance = *up;
            srpl_connection_t *unidentified = unmatched_instance->connection;
            if (unidentified == NULL) {
                up = &(*up)->next;
                continue;
            }
            if (unidentified->dso == NULL) {
                FAULT("unidentified instance " PRI_S_SRP " (%p) has outgoing connection (%p)!",
                      unmatched_instance->instance_name, unmatched_instance, unidentified);
                srpl_connection_discontinue(unidentified);
                RELEASE_HERE(unmatched_instance->connection, srpl_connection);
                unmatched_instance->connection = NULL;
                up = &(*up)->next;
                continue;
            }
            if (ip_addresses_equal(address, &unidentified->connected_address)) {
                INFO("Unidentified connection " PRI_S_SRP " matches new address for instance " PRI_S_SRP,
                     unidentified->dso->remote_name, instance->instance_name);
                srpl_match_unidentified_with_instance(unidentified, instance);
                matched_unidentified = true;
                break;
            } else {
                if (unidentified->connected_address.sa.sa_family == AF_INET6) {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(&unidentified->connected_address.sin6.sin6_addr, rdata_buf);
                    INFO("Unidentified connection address is: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(&unidentified->connected_address.sin6.sin6_addr, rdata_buf));
                } else {
                    IPv4_ADDR_GEN_SRP(&unidentified->connected_address.sin.sin_addr, rdata_buf);
                    INFO("Unidentified connection address is: " PRI_IPv4_ADDR_SRP,
                         IPv4_ADDR_PARAM_SRP(&unidentified->connected_address.sin.sin_addr, rdata_buf));
                }
                if (address->sa.sa_family == AF_INET6) {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(&address->sin6.sin6_addr, rdata_buf);
                    INFO("New address is: " PRI_SEGMENTED_IPv6_ADDR_SRP,
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(&address->sin6.sin6_addr, rdata_buf));
                } else {
                    IPv4_ADDR_GEN_SRP(&address->sin.sin_addr, rdata_buf);
                    INFO("New address is: " PRI_IPv4_ADDR_SRP,
                         IPv4_ADDR_PARAM_SRP(&address->sin.sin_addr, rdata_buf));
                }
                INFO("Unidentified connection addr %p does not match new address for instance addr %p",
                     unidentified, instance);
                INFO("Unidentified connection " PRI_S_SRP " does not match new address for instance " PRI_S_SRP,
                     unidentified->dso->remote_name, instance->instance_name);
                up = &(*up)->next;
            }
        }

        if (srpl_my_address_check(server_state, address)) {
            srpl_instance_is_me(instance, service, NULL, address, false);
        } else {
            instance->added_address = true;
            if (matched_unidentified) {
                instance->matched_unidentified = true;
            }
        }
    } else {
        srpl_event_t event;
        srpl_event_initialize(&event, srpl_event_address_remove);

        // Generate an event indicating that an address has been removed.
        if (!instance->is_me) {
            if (instance->connection != NULL) {
                srpl_event_deliver(instance->connection, &event);
            }
        }
    }

    INFO(PRI_S_SRP " on instance " PRI_S_SRP ", matched_unidentified " PRI_S_SRP ", added_address " PRI_S_SRP,
         more ? "more coming" : "done", instance->instance_name, instance->matched_unidentified ? "yes" : "no",
         instance->added_address ? "yes" : "no");
    if (!more) {
        if (instance->added_address && !instance->matched_unidentified) {
            // Generate an event indicating that we have a new address.
            if (instance->connection != NULL && !instance->connection->is_server) {
                srpl_event_t event;
                srpl_event_initialize(&event, srpl_event_address_add);
                srpl_event_deliver(instance->connection, &event);
            }
        }
        // reset added_address and matched_unidentified
        instance->added_address = false;
        instance->matched_unidentified = false;
    }
}

static void
srpl_abandon_nonpreferred_dataset(srpl_domain_t *NONNULL domain)
{
    srpl_instance_t *instance, *next;
    for (instance = domain->instances; instance != NULL; instance = next) {
        next = instance->next;
        if (instance->have_dataset_id) {
            if (srpl_dataset_id_compare(domain->dataset_id, instance->dataset_id) > 0) {
                instance->sync_to_join = false;
                if (instance->connection != NULL) {
                    INFO("abandon dataset with instance " PRI_S_SRP " of partner id %" PRIx64,
                         instance->instance_name, instance->partner_id);
                    srpl_connection_reset(instance->connection);
                    srpl_connection_next_state(instance->connection, srpl_state_disconnected);
                }
            }
        }
    }
}

static void srpl_transition_to_startup_state(srpl_domain_t *domain);

static int
srpl_dataset_id_compare(uint64_t id1, uint64_t id2)
{
    int64_t distance = id1 - id2;
    if (distance == 0) {
        return 0;
    } else if (distance > 0) {
        return 1;
    } else if (distance == EQUI_DISTANCE64 && (int64_t)id1 > (int64_t)id2) {
        // the number 2^(N−1) (where N is 64) is equidistant in both directions in sequence number terms.
        // they are both considered to be "less than" each other. This is true for any serial number with
        // distance of 0x8000000000000000 between them. To break the tie, higher signed number wins.
        return 1;
    } else {
        return -1;
    }
}

static bool
srpl_instances_max_dataset_id(srpl_domain_t *domain, uint64_t *dataset_id)
{
    srpl_instance_t *instance = NULL;
    bool have_max = false;
    uint64_t max = 0;

    for (instance = domain->instances; instance != NULL; instance = instance->next) {
        if (instance->sync_fail || instance->discontinuing || instance->version_mismatch) {
            continue;
        }
        if (!have_max) {
            max = instance->dataset_id;
            have_max = true;
        } else {
            if (srpl_dataset_id_compare(instance->dataset_id, max) > 0) {
                max = instance->dataset_id;
            }
        }
    }
    if (!have_max) {
        INFO("no available dataset id.");
    } else {
        *dataset_id = max;
    }
    return have_max;
}

static void
srpl_instance_add(const char *hostname, const char *service_name,
                  const char *ifname, srpl_domain_t *domain, srpl_instance_service_t *service,
                  bool have_partner_id, uint64_t advertised_partner_id,
                  bool have_dataset_id, uint64_t advertised_dataset_id,
                  bool have_priority, uint32_t advertised_priority)
{
    srpl_instance_t **sp, *instance;
    srpl_instance_service_t **hp;

    // Find the service on the instance list for this domain.
    for (instance = domain->instances; instance != NULL; instance = instance->next) {
        for (hp = &instance->services; *hp != NULL; hp = &(*hp)->next) {
            if (service == *hp) {
                INFO("service " PRI_S_SRP " is found with instance " PRI_S_SRP, service_name, instance->instance_name);
                break;
            }
        }
        if (*hp != NULL) {
            break;
        }
    }

    if (instance == NULL) {
        INFO("service " PRI_S_SRP " for " PRI_S_SRP "/" PUB_S_SRP " " PUB_S_SRP
             "id %" PRIx64 " " PUB_S_SRP "did %" PRIx64 " not on list",
             service_name != NULL ? service_name : "<NULL>", hostname, ifname,
             have_partner_id ? "" : "!", advertised_partner_id,
             have_dataset_id ? "" : "!", advertised_dataset_id);
        // Look for the instance with the same partner id
        for (sp = &domain->instances; *sp != NULL; sp = &(*sp)->next) {
            instance = *sp;
            if (instance->have_partner_id && instance->partner_id == advertised_partner_id) {
                INFO("instance " PRI_S_SRP " has matched partner_id %" PRIx64, instance->instance_name, instance->partner_id);
                break;
            }
        }

        if (*sp == NULL) {
            // We don't have the instance to the remote partner yet, create one
            instance = calloc(1, sizeof(*instance));
            if (instance == NULL) {
                ERROR("no memory to create instance for service " PRI_S_SRP, service_name);
                RELEASE_HERE(service, srpl_instance_service);
                return;
            }
            // Retain for the instance list on the domain
            RETAIN_HERE(instance, srpl_instance);
            instance->domain = domain;
            RETAIN_HERE(instance->domain, srpl_domain);
            instance->services = service;
            *sp = instance;
            INFO("create a new instance for service " PRI_S_SRP, service_name);
        } else {
            for (hp = &instance->services; *hp != NULL; hp = &(*hp)->next)
                ;
            *hp = service;
            INFO("instance " PRI_S_SRP " exists; just link the service " PRI_S_SRP, instance->instance_name, service_name);
        }
        // Retain service for the instance service list
        RETAIN_HERE(service, srpl_instance_service);
        // Retain instance for service
        RETAIN_HERE(instance, srpl_instance);
        service->instance = instance;
    }
    // take the host name of the remote partner as the instance name
    char *pch = strchr(service_name, '.');
    if (instance->instance_name == NULL || strncmp(instance->instance_name, service_name, pch - service_name)) {
        char partner_name[kDNSServiceMaxDomainName];
        memcpy(partner_name, service_name, pch - service_name);
        partner_name[pch - service_name] = '\0';
        char *new_partner_name = strdup(partner_name);
        if (new_partner_name == NULL) {
            ERROR("no memory for instance name.");
            return;
        } else {
            INFO("instance name changed from " PRI_S_SRP " to " PRI_S_SRP, instance->instance_name ?
                 instance->instance_name : "NULL", new_partner_name);
            free(instance->instance_name);
            instance->instance_name = new_partner_name;
        }
    }
    bool some_id_updated = false;
    if (have_dataset_id && (!instance->have_dataset_id || instance->dataset_id != advertised_dataset_id)) {
        some_id_updated = true;
        instance->have_dataset_id = true;
        instance->dataset_id = advertised_dataset_id;
        INFO("update instance " PRI_S_SRP " dataset_id %" PRIx64 " from service " PRI_S_SRP,
             instance->instance_name, instance->dataset_id, service_name);
    }

    // If this add changed the partner ID, we may want to re-attempt a connect.
    if (have_partner_id && (!instance->have_partner_id || instance->partner_id != advertised_partner_id)) {
        some_id_updated = true;
        instance->have_partner_id = true;
        instance->partner_id = advertised_partner_id;
        INFO("instance " PRI_S_SRP " update partner_id to %" PRIx64, instance->instance_name, advertised_partner_id);
    }

    // If this add changed the priority, we may want to re-attempt a connect.
    if (have_priority && (!instance->have_priority || instance->priority != advertised_priority)) {
        some_id_updated = true;
        instance->have_priority = true;
        instance->priority = advertised_priority;
        INFO("instance " PRI_S_SRP " update priority to %" PRIx32, instance->instance_name, advertised_priority);
    }
    // To join the replication, sync with remote partners that are discovered during the
    // discovery window.
    if (domain->partner_discovery_pending) {
         instance->discovered_in_window = true;
     }

    // If the hostname changed, we need to restart the address query.
    if (service->host_name == NULL || strcmp(service->host_name, hostname)) {
        if (service->address_query != NULL) {
            address_query_cancel(service->address_query);
            RELEASE_HERE(service->address_query, address_query);
            service->address_query = NULL;
        }

        if (service->host_name != NULL) {
            INFO("name server name change from " PRI_S_SRP " to " PRI_S_SRP " for " PRI_S_SRP "/" PUB_S_SRP " in domain " PRI_S_SRP,
                 service->host_name, hostname, service_name == NULL ? "<NULL>" : service_name, ifname, domain->name);
        } else {
            INFO("new name server " PRI_S_SRP " for " PRI_S_SRP "/" PUB_S_SRP " in domain " PRI_S_SRP,
                 hostname, service_name == NULL ? "<NULL>" : service_name, ifname, domain->name);
        }

        char *new_name = strdup(hostname);
        if (new_name == NULL) {
            // This should never happen, and if it does there's actually no clean way to recover from it.  This approach
            // will result in no crash, and since we don't start an address query in this case, we will just wind up in
            // a quiescent state for this replication peer until something changes.
            ERROR("no memory for service name.");
            return;
        } else {
            free(service->host_name);
            service->host_name = new_name;
        }
        // The instance may be connected. It's possible its IP address hasn't changed. If it has changed, we should
        // get a disconnect due to a connection timeout or (if something else got the same address, a reset) if for
        // no other reason, and then we'll try to reconnect, so this should be harmless.
    }

    // The address query can be NULL either because we only just created the instance, or because the instance name changed (e.g.
    // as the result of a hostname conflict).
    if (service->address_query == NULL) {
        service->address_query = address_query_create(service->host_name, service,
                                                      srpl_instance_address_callback,
                                                      srpl_instance_service_context_release);
        if (service->address_query == NULL) {
            INFO("unable to create address query");
        } else {
            RETAIN_HERE(service, srpl_instance_service); // retain for the address query.
        }
    }

    if (instance->have_partner_id &&
        domain->partner_id  == instance->partner_id)
    {
        srpl_instance_is_me(instance, service, NULL, NULL, true);
    }

    // If there's no existing connection, the partner initiates an outgoing connection if
    // it is in the startup state or its partner id is greater than the remote partner id.
    if (!instance->is_me && ((instance->connection == NULL || some_id_updated) &&
        (domain->srpl_opstate == SRPL_OPSTATE_STARTUP ||
         domain->partner_id > advertised_partner_id)))
    {
        char msg_buf[256];
        if (domain->srpl_opstate == SRPL_OPSTATE_STARTUP) {
            snprintf(msg_buf, sizeof(msg_buf), "I am in startup state");
        } else {
            snprintf(msg_buf, sizeof(msg_buf), "local partner_id %" PRIx64
                     " greater than remote partner_id %" PRIx64, domain->partner_id,
                     advertised_partner_id);
        }
        INFO("making outgoing connection on instance " PRI_S_SRP " (partner_id: %" PRIx64 ") since " PUB_S_SRP,
             instance->instance_name, instance->partner_id, msg_buf);
        if (instance->connection != NULL && instance->connection->connection != NULL) {
            srpl_trigger_disconnect(instance->connection);
            srpl_connection_next_state(instance->connection, srpl_state_idle);
        } else {
            srpl_instance_reconnect(instance);
        }
    } else {
        INFO(PRI_S_SRP ": not making outgoing connection: " PUB_S_SRP "is_me, connection = %p, " PRI_S_SRP
             "some_id_updated, local partner_id %" PRIx64 ", remote partner_id %" PRIx64,
             instance->instance_name, instance->is_me ? "" : "!", instance->connection, some_id_updated ? "" : "!",
             domain->partner_id, advertised_partner_id);
        // it's not our job to connect, but since there's some id change, we'd disconnect
        // the current connection and trigger the peer to reconnect.
        if (some_id_updated && instance->connection != NULL &&
            instance->connection->connection != NULL)
        {
            INFO("some id updated, disconnect the current connection.");
            srpl_trigger_disconnect(instance->connection);
            srpl_connection_next_state(instance->connection, srpl_state_idle);
        }
    }
}

static void
srpl_resolve_callback(srpl_instance_service_t *service)
{
    char ifname[IFNAMSIZ];
    srpl_domain_t *domain = service->domain;
    const char *domain_name;
    uint8_t domain_len;
    const char *partner_id_string;
    const char *dataset_id_string;
    const char *xpanid_string;
    const char *priority_string;
    uint8_t partner_id_string_len;
    uint8_t dataset_id_string_len;
    uint8_t xpanid_string_len;
    uint8_t priority_string_len;
    char partner_id_buf[INT64_HEX_STRING_MAX];
    char dataset_id_buf[INT64_HEX_STRING_MAX];
    char xpanid_buf[INT64_HEX_STRING_MAX];
    char priority_buf[INT64_HEX_STRING_MAX];
    uint64_t advertised_partner_id = 0;
    bool have_partner_id = false;
    uint64_t advertised_dataset_id = 0;
    bool have_dataset_id = false;
    bool have_priority = false;
    uint16_t advertised_priority = 0;
    srpl_instance_service_t **sp;
    srp_server_t *server_state = domain->server_state;

    // These are just used to do the "satisfied" check--we can tell that we have these records from the rdata pointers.
    service->have_txt_record = service->have_srv_record = false;

    // In case we later determine that the data we got is stale, this flag indicates that it's okay to try to
    // reconfirm it.
    service->got_new_info = true;

    if (service->txt_rdata == NULL) {
        INFO(PRI_S_SRP ": service update with no TXT record--skipping", service->full_service_name);
        return;
    }
    if (service->srv_rdata == NULL) {
        INFO(PRI_S_SRP ": service update with no SRV record--skipping", service->full_service_name);
        return;
    }

    domain_name = TXTRecordGetValuePtr(service->txt_length, service->txt_rdata, "dn", &domain_len);
    if (domain_name == NULL) {
        INFO("resolve for " PRI_S_SRP " succeeded, but there is no domain name.", service->full_service_name);
        return;
    }

    if (domain_len != strlen(domain->name) || memcmp(domain_name, domain->name, domain_len)) {
        const char *domain_print;
        char *domain_terminated = malloc(domain_len + 1);
        if (domain_terminated == NULL) {
            domain_print = "<no memory for domain name>";
        } else {
            memcpy(domain_terminated, domain_name, domain_len);
            domain_terminated[domain_len] = 0;
            domain_print = domain_terminated;
        }
        INFO("domain (" PRI_S_SRP ") for " PRI_S_SRP " doesn't match expected domain " PRI_S_SRP,
             domain_print, service->full_service_name, domain->name);
        free(domain_terminated);
        return;
    }

    if (server_state->current_thread_domain_name == NULL) {
        FAULT("current_thread_domain_name is NULL.");
        return;
    }

    if (strcmp(domain->name, server_state->current_thread_domain_name)) {
        INFO("discovered srpl instance is not for current thread domain, so not setting up replication.");
        return;
    }

    INFO("server " PRI_S_SRP " for " PRI_S_SRP, service->full_service_name, domain->name);

    // Make sure it's for our mesh.
    snprintf(xpanid_buf, sizeof(xpanid_buf), "%" PRIx64, server_state->xpanid);
    xpanid_string = TXTRecordGetValuePtr(service->txt_length, service->txt_rdata, "xpanid", &xpanid_string_len);
    if (xpanid_string == NULL ||
        (xpanid_string_len != strlen(xpanid_buf) || memcmp(xpanid_buf, xpanid_string, xpanid_string_len)))
    {
        char other_xpanid_buf[INT64_HEX_STRING_MAX];
        if (xpanid_string_len >= sizeof(other_xpanid_buf)) {
            xpanid_string_len = sizeof(other_xpanid_buf) - 1;
        }
        if (xpanid_string == NULL) {
            const char none[] = "(none)";
            memcpy(other_xpanid_buf, none, sizeof(none));
        } else {
            memcpy(other_xpanid_buf, xpanid_string, xpanid_string_len);
            other_xpanid_buf[xpanid_string_len] = 0;
        }
        INFO("discovered srpl instance is not for xpanid " PRI_S_SRP ", not " PRI_S_SRP
             " so not setting up replication.", xpanid_buf, other_xpanid_buf);
        return;
    }

    partner_id_string = TXTRecordGetValuePtr(service->txt_length, service->txt_rdata, "pid", &partner_id_string_len);
    if (partner_id_string != NULL && partner_id_string_len < INT64_HEX_STRING_MAX) {
        char *endptr, *nulptr;
        unsigned long long num;
        memcpy(partner_id_buf, partner_id_string, partner_id_string_len);
        nulptr = &partner_id_buf[partner_id_string_len];
        *nulptr = '\0';
        num = strtoull(partner_id_buf, &endptr, 16);
        // On current architectures, unsigned long long and uint64_t are the same size, but we should have a check here
        // just in case, because the standard doesn't guarantee that this will be true.
        // If endptr == nulptr, that means we converted the entire buffer and didn't run into a NUL in the middle of it
        // somewhere.
        if (num < UINT64_MAX && endptr == nulptr) {
            advertised_partner_id = num;
            have_partner_id = true;
        }
    }

    dataset_id_string = TXTRecordGetValuePtr(service->txt_length, service->txt_rdata, "did", &dataset_id_string_len);
    if (dataset_id_string != NULL && dataset_id_string_len < INT64_HEX_STRING_MAX) {
        char *endptr, *nulptr;
        unsigned long long num;
        memcpy(dataset_id_buf, dataset_id_string, dataset_id_string_len);
        nulptr = &dataset_id_buf[dataset_id_string_len];
        *nulptr = '\0';
        num = strtoull(dataset_id_buf, &endptr, 16);
        if (num < UINT64_MAX && endptr == nulptr) {
            advertised_dataset_id = num;
            have_dataset_id = true;
        }
    }

    priority_string = TXTRecordGetValuePtr(service->txt_length, service->txt_rdata, "priority", &priority_string_len);
    if (priority_string != NULL && priority_string_len < INT64_HEX_STRING_MAX) {
        char *endptr, *nulptr;
        unsigned long long num;
        memcpy(priority_buf, priority_string, priority_string_len);
        nulptr = &priority_buf[priority_string_len];
        *nulptr = '\0';
        num = strtoull(priority_buf, &endptr, 16);
        if (num < UINT64_MAX && endptr == nulptr) {
            advertised_priority = num;
            have_priority = true;
        }
    }

    dns_rr_t srv_record;
    unsigned offset = 0;
    memset(&srv_record, 0, sizeof(srv_record));
    srv_record.type = dns_rrtype_srv;
    if (!dns_rdata_parse_data(&srv_record, service->srv_rdata, &offset, offset + service->srv_length, service->srv_length, 0)) {
        ERROR(PRI_S_SRP ": unable to parse srv record", service->full_service_name);
        return;
    }

    service->outgoing_port = srv_record.data.srv.port;

    if (if_indextoname(service->ifindex, ifname) == NULL) {
        snprintf(ifname, sizeof(ifname), "%d", service->ifindex);
    }

    char namebuf[DNS_MAX_NAME_SIZE_ESCAPED + 1];
    dns_name_print(srv_record.data.srv.name, namebuf, sizeof(namebuf));
    dns_name_free(srv_record.data.srv.name);

    srpl_instance_add(namebuf, service->full_service_name, ifname, service->domain, service, have_partner_id,
                      advertised_partner_id, have_dataset_id, advertised_dataset_id, have_priority,
                      advertised_priority);

    // After the service is associated with a resolved instance, we should take it off the unresolved
    // list if the service is still on it. If the service fails to assocaited with an instance because
    // for example, the resolve shows a service that does not include required data, we should still
    // keep the service on the unresolved list. Later on when we get an expected resolve, the service
    // can be moved to the associated list. This guarantees that a service at a time has to be on either
    // unresolved or associated list.
    for (sp = &domain->unresolved_services; *sp; sp = &(*sp)->next) {
        if (*sp == service) {
            *sp = service->next;
            service->next = NULL;
            RELEASE_HERE(service, srpl_instance_service);
            break;
        }
    }
}

static void
srpl_instance_service_newdata_timeout(void *context)
{
    srpl_instance_service_t *service = context;
    srpl_resolve_callback(service);
}

static void
srpl_instance_service_satisfied_check(srpl_instance_service_t *service)
{
    if (service->have_srv_record && service->have_txt_record) {
        if (service->resolve_wakeup != NULL) {
            ioloop_cancel_wake_event(service->resolve_wakeup);
        }
        srpl_resolve_callback(service);
        return;
    }
    INFO(PRI_S_SRP ": not satisfied, waiting.", service->full_service_name);
    if (service->resolve_wakeup == NULL) {
        service->resolve_wakeup = ioloop_wakeup_create();
        if (service->resolve_wakeup == NULL) {
            ERROR(PRI_S_SRP ": unable to allocate resolve wakeup", service->full_service_name);
            return;
        }
    }
    ioloop_add_wake_event(service->resolve_wakeup, service, srpl_instance_service_newdata_timeout,
                          srpl_instance_service_context_release, 1000); // max one second
    RETAIN_HERE(service, srpl_instance_service); // for the wakeup
}

static void
srpl_service_txt_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags UNUSED flags, uint32_t UNUSED interfaceIndex,
                          DNSServiceErrorType errorCode, const char *fullname, uint16_t UNUSED rrtype, uint16_t UNUSED rrclass,
                          uint16_t rdlen, const void *rdata, uint32_t UNUSED ttl, void *context)
{
    srpl_instance_service_t *service = context;
    if (errorCode != kDNSServiceErr_NoError) {
        ERROR("txt resolve for " PRI_S_SRP " failed with %d", fullname, errorCode);
        if (service->txt_txn != NULL) {
            ioloop_dnssd_txn_cancel(service->txt_txn);
            ioloop_dnssd_txn_release(service->txt_txn);
            service->txt_txn = NULL;
        }
        return;
    }

    free(service->txt_rdata);
    if (!(flags & kDNSServiceFlagsAdd)) {
        INFO("TXT record for " PRI_S_SRP " went away", service->full_service_name);
        service->txt_rdata = NULL;
        service->txt_length = 0;
        service->have_txt_record = false;
        return;
    }
    service->txt_rdata = malloc(rdlen);
    if (service->txt_rdata == NULL) {
        ERROR("unable to save txt rdata for " PRI_S_SRP, service->full_service_name);
        return;
    }
    memcpy(service->txt_rdata, rdata, rdlen);
    service->txt_length = rdlen;
    service->have_txt_record = true;
    srpl_instance_service_satisfied_check(service);
}

static void
srpl_service_srv_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t UNUSED interfaceIndex,
                          DNSServiceErrorType errorCode, const char *UNUSED fullname, uint16_t UNUSED rrtype, uint16_t UNUSED rrclass,
                          uint16_t rdlen, const void *rdata, uint32_t UNUSED ttl, void *context)
{
    srpl_instance_service_t *service = context;
    if (errorCode != kDNSServiceErr_NoError) {
        ERROR("srv resolve for " PRI_S_SRP " failed with %d", fullname, errorCode);
        if (service->srv_txn != NULL) {
            ioloop_dnssd_txn_cancel(service->srv_txn);
            ioloop_dnssd_txn_release(service->srv_txn);
            service->srv_txn = NULL;
        }
        return;
    }

    free(service->srv_rdata);
    if (!(flags & kDNSServiceFlagsAdd)) {
        INFO("SRV record for " PRI_S_SRP " went away", service->full_service_name);
        service->srv_rdata = NULL;
        service->srv_length = 0;
        service->have_srv_record = false;
        return;
    }
    service->srv_rdata = malloc(rdlen);
    if (service->srv_rdata == NULL) {
        ERROR("unable to save srv rdata for " PRI_S_SRP, service->full_service_name);
        return;
    }
    memcpy(service->srv_rdata, rdata, rdlen);
    service->srv_length = rdlen;
    service->have_srv_record = true;
    srpl_instance_service_satisfied_check(service);
}

static void
srpl_browse_restart(void *context)
{
    srpl_domain_t *domain = context;
    ERROR("restarting browse on domain " PRI_S_SRP, domain->name);
    srpl_domain_browse_start(domain);
}

static bool
srpl_service_instance_query_start(srpl_instance_service_t *service, dnssd_txn_t **txn, const char *rrtype_name,
                                  uint16_t rrtype, uint16_t qclass, DNSServiceQueryRecordReply callback)
{
    DNSServiceRef sdref;

    int err = DNSServiceQueryRecord(&sdref, kDNSServiceFlagsLongLivedQuery, kDNSServiceInterfaceIndexAny,
                                    service->full_service_name,  rrtype, qclass, callback, service);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to resolve " PUB_S_SRP " record for " PRI_S_SRP ": code %d",
              rrtype_name, service->full_service_name, err);
        return false;
    }
    *txn = ioloop_dnssd_txn_add(sdref, service, srpl_instance_service_context_release, NULL);
    if (*txn == NULL) {
        ERROR("unable to allocate dnssd_txn_t for " PUB_S_SRP " record for " PRI_S_SRP,
              rrtype_name, service->full_service_name);
        DNSServiceRefDeallocate(sdref);
        return false;
    }
    // Retain for the dnssd_txn.
    RETAIN_HERE(service, srpl_instance_service);
    return true;
}

static void
srpl_domain_context_release(void *context)
{
    srpl_domain_t *domain = context;
    RELEASE_HERE(domain, srpl_domain);
}

static void
srpl_browse_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                     DNSServiceErrorType errorCode, const char *serviceName, const char *regtype,
                     const char *replyDomain, void *context)
{
    srpl_domain_t *domain = context;
    if (errorCode != kDNSServiceErr_NoError) {
        ERROR("browse on domain " PRI_S_SRP " failed with %d", domain->name, errorCode);
        if (domain->query != NULL) {
            ioloop_dnssd_txn_cancel(domain->query);
            ioloop_dnssd_txn_release(domain->query);
            domain->query = NULL;
        }

        // Get rid of all instances on the domain, because we aren't going to get remove events for them.
        // If we start a new browse and get add events while the connections are still up, this will
        // have no effect.
        srpl_instance_t *next_instance;
        for (srpl_instance_t *instance = domain->instances; instance; instance = next_instance) {
            INFO("_srpl-tls._tcp instance " PRI_S_SRP " went away.", instance->instance_name);
            next_instance = instance->next;
            srpl_instance_discontinue(instance);
        }

        srpl_instance_service_t *service, *next;
        for (service = domain->unresolved_services; service != NULL; service = next) {
            INFO("discontinue unresolved service " PRI_S_SRP, service->full_service_name);
            next = service->next;
            service->num_copies = 0;
            srpl_instance_service_discontinue(service);
        }

        if (domain->server_state->srpl_browse_wakeup == NULL) {
            domain->server_state->srpl_browse_wakeup = ioloop_wakeup_create();
        }
        if (domain->server_state->srpl_browse_wakeup != NULL) {
            ioloop_add_wake_event(domain->server_state->srpl_browse_wakeup,
                                  domain, srpl_browse_restart, srpl_domain_context_release, 1000);
            RETAIN_HERE(domain, srpl_domain);
        }
        return;
    }

    char full_service_name[kDNSServiceMaxDomainName];
    DNSServiceConstructFullName(full_service_name, serviceName, regtype, replyDomain);

    srpl_instance_t *instance;
    srpl_instance_service_t *service;
    // See if we already have a service record going; First search in unresolved_services list which
    // contains the services that haven't been resolved yet.
    for (service = domain->unresolved_services; service; service = service->next) {
        if (!strcmp(service->full_service_name, full_service_name)) {
            break;
        }
    }
    // If the service is not found in unresolved_services list, search in instance list which contains services
    // that have been resolved.
    if (service == NULL) {
        for (instance = domain->instances; instance; instance = instance->next) {
            for (service = instance->services; service; service = service->next) {
                if (!strcmp(service->full_service_name, full_service_name)) {
                   break;
                }
            }
            if (service != NULL) {
                break;
            }
        }
    }
    if (flags & kDNSServiceFlagsAdd) {
        if (service != NULL) {
            // it's possible that a service goes away and starts discontinuing, and before the timeout,
            // the service comes back again. In this case, since the service is still on the list, it
            // appears as a duplicate add. But we should cancel the discontinue timer.
            if (service->discontinue_timeout != NULL) {
                if (service->discontinuing) {
                    INFO("discontinue on service " PRI_S_SRP " canceled.", service->full_service_name);
                    ioloop_cancel_wake_event(service->discontinue_timeout);
                    service->discontinuing = false;
                    if (service->instance != NULL) {
                        service->instance->discontinuing = false;
                    }
                }
            }

            if (service->resolve_started) {
                INFO(PRI_S_SRP ": resolve_started true, incrementing num_copies to %d",
                     full_service_name, service->num_copies + 1);
                service->num_copies++;
                INFO("duplicate add for service " PRI_S_SRP, full_service_name);
                return;
             }
             // In this case the service went away and came back, so service->resolve_started is false, but the
             // instance still exists.
             INFO(PRI_S_SRP ": resolve_started false, incrementing num_copies to %d",
                  full_service_name, service->num_copies + 1);
             service->num_copies++;
             INFO("service " PRI_S_SRP " went away but came back.", full_service_name);
        } else {
            service = calloc(1, sizeof(*service));
            if (service == NULL) {
                ERROR("no memory for service " PRI_S_SRP, full_service_name);
                return;
            }
            // Retain for unresolved_services list
            RETAIN_HERE(service, srpl_instance_service);
            service->domain = domain;
            RETAIN_HERE(service->domain, srpl_domain);

            service->full_service_name = strdup(full_service_name);
            if (service->full_service_name == NULL) {
                ERROR("no memory for service name " PRI_S_SRP, full_service_name);
                RELEASE_HERE(service, srpl_instance_service);
                return;
            }
            INFO(PRI_S_SRP ": new service, setting num_copies to 1", full_service_name);
            service->num_copies = 1;
            service->ifindex = interfaceIndex;
            // add to the unresolved service list
            srpl_instance_service_t **sp;
            for (sp = &domain->unresolved_services; *sp != NULL; sp = &(*sp)->next) {;}
            *sp = service;

            dns_towire_state_t towire;
            uint8_t name_buffer[kDNSServiceMaxDomainName];
            memset(&towire, 0, sizeof(towire));
            towire.p = name_buffer;
            towire.lim = towire.p + kDNSServiceMaxDomainName;
            dns_full_name_to_wire(NULL, &towire, full_service_name);

            free(service->ptr_rdata);
            service->ptr_length = towire.p - name_buffer;
            service->ptr_rdata = malloc(service->ptr_length);
            if (service->ptr_rdata == NULL) {
                ERROR("unable to save PTR rdata for " PRI_S_SRP, full_service_name);
                return;
            }
            memcpy(service->ptr_rdata, name_buffer, service->ptr_length);
        }

        if (!srpl_service_instance_query_start(service, &service->txt_txn, "TXT", dns_rrtype_txt, dns_qclass_in,
                                               srpl_service_txt_callback) ||
            !srpl_service_instance_query_start(service, &service->srv_txn, "SRV", dns_rrtype_srv, dns_qclass_in,
                                               srpl_service_srv_callback))
        {
            return;
        }
        INFO("resolving " PRI_S_SRP, full_service_name);
        service->resolve_started = true;
    } else {
        if (service != NULL) {
            INFO(PRI_S_SRP ": decrementing num_copies to %d", full_service_name, service->num_copies - 1);
            service->num_copies--;
            if (service->num_copies < 0) {
                FAULT("num_copies went negative");
                service->num_copies = 0;
            }
            if (service->num_copies == 0) {
                INFO("discontinuing service " PRI_S_SRP, full_service_name);
                srpl_instance_service_discontinue(service);
                return;
            }
        }
    }
}

static void
srpl_dnssd_txn_fail(void *context, int err)
{
    srpl_domain_t *domain = context;
    ERROR("service browse " PRI_S_SRP " i/o failure: %d", domain->name, err);
}

static bool
srpl_domain_browse_start(srpl_domain_t *domain)
{
    int ret;
    DNSServiceRef sdref;

    INFO("starting browse on _srpl-tls._tcp");
    // Look for an NS record for the specified domain using mDNS, not DNS.
    ret = DNSServiceBrowse(&sdref, kDNSServiceFlagsLongLivedQuery,
                           kDNSServiceInterfaceIndexAny, "_srpl-tls._tcp", NULL, srpl_browse_callback, domain);
    if (ret != kDNSServiceErr_NoError) {
        ERROR("Unable to query for NS records for " PRI_S_SRP, domain->name);
        return false;
    }
    domain->query = ioloop_dnssd_txn_add(sdref, domain, srpl_domain_context_release, srpl_dnssd_txn_fail);
    if (domain->query == NULL) {
        ERROR("Unable to set up ioloop transaction for NS query on " PRI_S_SRP, domain->name);
        DNSServiceRefDeallocate(sdref);
        return false;
    }
    RETAIN_HERE(domain, srpl_domain); // For the browse
    return true;
}

srpl_domain_t *
srpl_domain_create_or_copy(srp_server_t *server_state, const char *domain_name)
{
    srpl_domain_t **dp, *domain;

    // Find the domain, if it's already there.
    for (dp = &server_state->srpl_domains; *dp; dp = &(*dp)->next) {
        domain = *dp;
        if (!strcasecmp(domain->name, domain_name)) {
            break;
        }
    }

    // If not there, make it.
    if (*dp == NULL) {
        domain = calloc(1, sizeof(*domain));
        if (domain == NULL || (domain->name = strdup(domain_name)) == NULL) {
            ERROR("Unable to allocate replication structure for domain " PRI_S_SRP, domain_name);
            free(domain);
            return NULL;
        }
        *dp = domain;
        // Hold a reference for the domain list
        RETAIN_HERE(domain, srpl_domain);
        INFO("New service replication browsing domain: " PRI_S_SRP, domain->name);

        domain->srpl_opstate = SRPL_OPSTATE_STARTUP;
        domain->server_state = server_state;
        domain->partner_id = srp_random64();
        INFO("generate partner id %" PRIx64 " for domain " PRI_S_SRP, domain->partner_id, domain->name);
    } else {
        ERROR("Unexpected duplicate replication domain: " PRI_S_SRP, domain_name);
        return NULL;
    }
    return domain;
}

static void
srpl_domain_add(srp_server_t *server_state, const char *domain_name)
{
    srpl_domain_t *domain = srpl_domain_create_or_copy(server_state, domain_name);
    if (domain == NULL) {
        return;
    }

    domain->partner_discovery_timeout = ioloop_wakeup_create();
    if (domain->partner_discovery_timeout) {
        ioloop_add_wake_event(domain->partner_discovery_timeout, domain,
                              srpl_partner_discovery_timeout, srpl_domain_context_release,
                              MIN_PARTNER_DISCOVERY_INTERVAL + srp_random16() % PARTNER_DISCOVERY_INTERVAL_RANGE);
        RETAIN_HERE(domain, srpl_domain);
    } else {
        ERROR("unable to add wakeup event for partner discovery for domain " PRI_S_SRP, domain->name);
        return;
    }
    domain->partner_discovery_pending = true;

    // Start a browse on the domain.
    if (!srpl_domain_browse_start(domain)) {
        return;
    }
}

static void
srpl_domain_rename(const char *current_name, const char *new_name)
{
    ERROR("replication domain " PRI_S_SRP " renamed to " PRI_S_SRP ", not currently handled.", current_name, new_name);
}

// Note that when this is implemented, it has the potential to return new thread domain names more than once, so
// in principle we need to change the name of the domain we are advertising.
static cti_status_t
cti_get_thread_network_name(void *context, cti_string_property_reply_t NONNULL callback,
                            run_context_t NULLABLE UNUSED client_queue)
{
    callback(context, "openthread", kCTIStatus_NoError);
    return kCTIStatus_NoError;
}

//
// Event apply functions, print functions, and state actions, generally in order
//

static bool
event_is_message(srpl_event_t *event)
{
    switch(event->event_type) {
    case srpl_event_invalid:
    case srpl_event_address_add:
    case srpl_event_address_remove:
    case srpl_event_server_disconnect:
    case srpl_event_reconnect_timer_expiry:
    case srpl_event_disconnected:
    case srpl_event_connected:
    case srpl_event_advertise_finished:
    case srpl_event_srp_client_update_finished:
    case srpl_event_do_sync:
        return false;

    case srpl_event_session_response_received:
    case srpl_event_send_candidates_response_received:
    case srpl_event_candidate_received:
    case srpl_event_host_message_received:
    case srpl_event_candidate_response_received:
    case srpl_event_host_response_received:
    case srpl_event_session_message_received:
    case srpl_event_send_candidates_message_received:
        return true;
    }
    return false;
}

// States that require an instance (most states). We also validate the chain up to the server state, because
// it's possible for that to go away and yet still for one last event to arrive, at least in principle.
#define REQUIRE_SRPL_INSTANCE(srpl_connection)                                                              \
    do {                                                                                                    \
        if ((srpl_connection)->instance == NULL || (srpl_connection)->instance->domain == NULL ||           \
            (srpl_connection)->instance->domain->server_state == NULL) {                                    \
            ERROR(PRI_S_SRP ": no instance in state " PUB_S_SRP, srpl_connection->name,                     \
                  srpl_connection->state_name);                                                             \
            return srpl_state_invalid;                                                                      \
        }                                                                                                   \
    } while(false)

// For states that never receive events.
#define REQUIRE_SRPL_EVENT_NULL(srpl_connection, event)                                      \
    do {                                                                                     \
        if ((event) != NULL) {                                                               \
            ERROR(PRI_S_SRP ": received unexpected " PUB_S_SRP " event in state " PUB_S_SRP, \
                  srpl_connection->name, event->name, srpl_connection->state_name);          \
            return srpl_state_invalid;                                                       \
        }                                                                                    \
    } while (false)

// Announce that we have entered a state that takes no events
#define STATE_ANNOUNCE_NO_EVENTS(srpl_connection)                                                          \
    do {                                                                                                   \
        INFO(PRI_S_SRP ": entering state " PUB_S_SRP, srpl_connection->name, srpl_connection->state_name); \
    } while (false)

// Announce that we have entered a state that takes no events
#define STATE_ANNOUNCE_NO_EVENTS_NAME(connection, fqdn)                                                    \
    do {                                                                                                   \
        char hostname[kDNSServiceMaxDomainName];                                                           \
        dns_name_print(fqdn, hostname, sizeof(hostname));                                                  \
        INFO(PRI_S_SRP ": entering state " PUB_S_SRP " with host " PRI_S_SRP,                              \
            connection->name, connection->state_name, hostname);                                           \
    } while (false)

// Announce that we have entered a state that takes no events
#define STATE_ANNOUNCE(srpl_connection, event)                                                       \
    do {                                                                                             \
        if (event != NULL)  {                                                                        \
            INFO(PRI_S_SRP ": event " PUB_S_SRP " received in state " PUB_S_SRP,                     \
                 srpl_connection->name, event->name, srpl_connection->state_name);                   \
        } else {                                                                                     \
            INFO(PRI_S_SRP ": entering state " PUB_S_SRP,                                            \
            srpl_connection->name, srpl_connection->state_name);                                     \
        }                                                                                            \
    } while (false)

#define UNEXPECTED_EVENT_MAIN(srpl_connection, event, bad)                             \
    do {                                                                               \
        if (event_is_message(event)) {                                                 \
            INFO(PRI_S_SRP ": invalid event " PUB_S_SRP " in state " PUB_S_SRP,        \
                 (srpl_connection)->name, (event)->name, srpl_connection->state_name); \
            return bad;                                                                \
        }                                                                              \
        INFO(PRI_S_SRP ": unexpected event " PUB_S_SRP " in state " PUB_S_SRP,         \
            (srpl_connection)->name, (event)->name,                                    \
             srpl_connection->state_name);                                             \
        return srpl_state_invalid;                                                     \
    } while (false)

// UNEXPECTED_EVENT flags the response as bad on a protocol level, triggering a retry delay
// UNEXPECTED_EVENT_NO_ERROR doesn't.
#define UNEXPECTED_EVENT(srpl_connection, event) UNEXPECTED_EVENT_MAIN(srpl_connection, event, srpl_state_invalid)
#define UNEXPECTED_EVENT_NO_ERROR(srpl_connection, event) \
    UNEXPECTED_EVENT_MAIN(srpl_connection, event, srpl_connection_drop_state(srpl_connection->instance, srpl_connection))

static void
srpl_instance_reconnect(srpl_instance_t *instance)
{
    srpl_event_t event;

    // If we have a new connection, no need to reconnect.
    if (instance->connection != NULL && instance->connection->is_server &&
        SRPL_CONNECTION_IS_CONNECTED(instance->connection))
    {
        INFO(PRI_S_SRP ": we have a valid connection.", instance->instance_name);
        return;
    }
    // We shouldn't have an outgoing connection.
    if (instance->connection != NULL && !instance->connection->is_server &&
        SRPL_CONNECTION_IS_CONNECTED(instance->connection))
    {
        FAULT(PRI_S_SRP ": got to srpl_instance_reconnect with a connected (" PUB_S_SRP ") outgoing connection.",
              instance->instance_name, srpl_state_name(instance->connection->state));
        return;
    }

    // Start from the beginning of the address list.
    srpl_instance_address_query_reset(instance);

    // If we don't have an srpl_connection at this point, make one.
    if (instance->connection == NULL) {
        INFO(PRI_S_SRP ": instance has no connection.", instance->instance_name);
        instance->connection = srpl_connection_create(instance, true);
        if (instance->connection == NULL) {
            ERROR(PRI_S_SRP ": unable to create srpl_connection", instance->instance_name);
            return;
        }
        srpl_connection_next_state(instance->connection, srpl_state_idle);
    }

    // Trigger a reconnect if appropriate
    if (!instance->is_me && instance->domain != NULL &&
        (instance->domain->srpl_opstate == SRPL_OPSTATE_STARTUP || instance->domain->partner_id > instance->partner_id))
    {
        // We might be in some disconnected state other than idle, so first move to idle if that's the case.
        if (instance->connection->state != srpl_state_idle) {
            srpl_connection_next_state(instance->connection, srpl_state_idle);
        }
        srpl_event_initialize(&event, srpl_event_reconnect_timer_expiry);
        srpl_event_deliver(instance->connection, &event);
    } else {
        ERROR(PRI_S_SRP ": reconnect requested but not appropriate: is_me = " PUB_S_SRP
              ", domain = %p, opstate = %d, ddid %" PRIx64 ", idid %" PRIx64,
              instance->instance_name, instance->is_me ? "true" : "false", instance->domain,
              instance->domain == NULL ? -1 : instance->domain->srpl_opstate,
              instance->domain == NULL ? 0 : instance->domain->partner_id, instance->partner_id);
    }
}

static void
srpl_instance_reconnect_callback(void *context)
{
    srpl_instance_reconnect(context);
}

static srpl_state_t
srpl_connection_drop_state_delay(srpl_instance_t *instance, srpl_connection_t *srpl_connection, int delay)
{
    // Schedule a reconnect.
    if (instance->reconnect_timeout == NULL) {
        instance->reconnect_timeout = ioloop_wakeup_create();
    }
    if (instance->reconnect_timeout == NULL) {
        FAULT(PRI_S_SRP "disconnecting, but can't reconnect!", srpl_connection->name);
    } else {
        RETAIN_HERE(instance, srpl_instance); // for the timeout
        ioloop_add_wake_event(instance->reconnect_timeout, instance,
                              srpl_instance_reconnect_callback, srpl_instance_context_release, delay * MSEC_PER_SEC);
    }

    if (srpl_connection == instance->connection && srpl_connection->is_server) {
        srpl_connection->retry_delay = delay;
        return srpl_state_retry_delay_send;
    } else {
        return srpl_state_disconnect;
    }
}

static srpl_state_t
srpl_connection_drop_state(srpl_instance_t *instance, srpl_connection_t *srpl_connection)
{
    if (instance == NULL) {
        return srpl_state_disconnect;
    } else if (instance->unmatched) {
        if (instance->connection == srpl_connection) {
            RELEASE_HERE(instance->connection, srpl_connection);
            instance->connection = NULL;
        }
        return srpl_state_disconnect;
    } else {
        return srpl_connection_drop_state_delay(instance, srpl_connection, 300);
    }
}

// Call when there's a protocol error, so that we don't start reconnecting over and over.
static void
srpl_disconnect(srpl_connection_t *srpl_connection)
{
    const int delay = 300; // five minutes
    srpl_instance_t *instance = srpl_connection->instance;
    if (instance != NULL && srpl_connection->connection != NULL) {
        srpl_state_t state = srpl_connection_drop_state_delay(instance, srpl_connection, delay);
        if (state == srpl_state_retry_delay_send) {
            srpl_retry_delay_send(srpl_connection, delay);
        }
    }
    srpl_connection_discontinue(srpl_connection);
}

static void
srpl_connection_state_timeout(void *context)
{
    srpl_connection_t *srpl_connection = context;
    srpl_instance_t *instance = srpl_connection->instance;
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    // Connection might have been discontinued before we came back.
    if (instance == NULL) {
        return;
    }

    // If the srpl connection has been in the current state for timeout and not received any
    // event to get out of the current state, we assume the peer is gone or unavailable and
    // can exclude this instance when we make a decision to enter the routine state.
    instance->sync_fail = true;
    INFO("connection for instance " PRI_S_SRP " timed out in state " PUB_S_SRP,
         instance->instance_name, srpl_connection->state_name);
    if (instance != NULL && instance->sync_to_join &&
        domain != NULL && domain->srpl_opstate != SRPL_OPSTATE_ROUTINE)
    {
        instance->sync_to_join = false;
        srpl_maybe_sync_or_transition(domain);
    }
}

static void
srpl_connection_schedule_state_timeout(srpl_connection_t *srpl_connection, uint32_t when)
{
    // Create a state timer on the srpl_connection_t
    if (srpl_connection->state_timeout == NULL) {
        srpl_connection->state_timeout = ioloop_wakeup_create();
        if (srpl_connection->state_timeout == NULL) {
            ERROR("no memory for state_timeout for service instance " PRI_S_SRP, srpl_connection->name);
            return;
        }
    } else {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
    }
    ioloop_add_wake_event(srpl_connection->state_timeout, srpl_connection, srpl_connection_state_timeout,
                          srpl_connection_context_release, when);
    RETAIN_HERE(srpl_connection, srpl_connection); // the timer has a reference.
    return;
}

// We arrive at the disconnected state when there is no connection to make, or no need to make a connection.
// This state takes no action, but waits for events. If we get an add event and we don't have a viable incoming
// connection, we go to the next_address_get event.
static srpl_state_t
srpl_disconnected_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        srpl_connection_schedule_state_timeout(srpl_connection, SRPL_STATE_TIMEOUT);
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_address_add) {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        return srpl_state_next_address_get;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

static void
srpl_instance_address_query_reset(srpl_instance_t *instance)
{
    for (srpl_instance_service_t *service = instance->services; service != NULL; service = service->next) {
        address_query_t *address_query = service->address_query;
        if (address_query != NULL && address_query->num_addresses > 0) {
            address_query->cur_address = -1;
        }
    }
}

// This state takes the action of looking for an address to try. This can have three outcomes:
//
// * No addresses available: go to the disconnected state
// * End of address list: go to the reconnect_wait state
// * Address found: got to the connect state

static srpl_state_t
srpl_next_address_get_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    address_query_t *address_query = NULL;
    srpl_instance_t *instance;
    srpl_instance_service_t *service;
    bool no_address = true;
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);

    instance = srpl_connection->instance;
    // Get the next address
    // Return an event, one of "next address", "end of address list" or "no addresses"
    for (service = instance->services; service != NULL; service = service->next) {
        address_query = service->address_query;
        if (address_query == NULL || address_query->num_addresses == 0) {
            continue;
        } else {
            no_address = false;
            if (address_query->cur_address == address_query->num_addresses ||
                ++address_query->cur_address == address_query->num_addresses)
            {
                continue;
            } else {
                memcpy(&srpl_connection->connected_address,
                       &address_query->addresses[address_query->cur_address], sizeof(addr_t));
                if (srpl_connection->connected_address.sa.sa_family == AF_INET) {
                    srpl_connection->connected_address.sin.sin_port = htons(service->outgoing_port);
                } else {
                    srpl_connection->connected_address.sin6.sin6_port = htons(service->outgoing_port);
                }
                return srpl_state_connect;
            }
        }
     }

     if (no_address) {
         return srpl_state_disconnected;
     } else {
         srpl_instance_address_query_reset(instance);
         return srpl_state_reconnect_wait;
     }
}

// This state takes the action of connecting to the connection's current address, which is expected to have
// been set. This can have two outcomes:
//
// * The connect attempt fails immediately: go to the next_address_get state
// * The connection attempt is in progress: go to the connecting state
static srpl_state_t
srpl_connect_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);

    // Connect to the address from the event.
    if (!srpl_connection_connect(srpl_connection)) {
        return srpl_state_next_address_get;
    } else {
        return srpl_state_connecting;
    }
}

// We reach this state when we are disconnected and don't need to reconnect because we have an active server
// connection. If we get a server disconnect here, then we go to the next_address_get state.
static srpl_state_t
srpl_idle_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);
    if (event == NULL) {
        srpl_connection_schedule_state_timeout(srpl_connection, SRPL_STATE_TIMEOUT);
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_server_disconnect ||
               event->event_type == srpl_event_reconnect_timer_expiry)
    {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        INFO(PRI_S_SRP ": event " PUB_S_SRP " received in state " PUB_S_SRP,
             srpl_connection->name, event->name, srpl_connection->state_name);
        return srpl_state_next_address_get;
    } else {
        // We don't log unhandled events in the idle state because it creates a lot of noise.
        return srpl_state_invalid;
    }
}

static void
srpl_maybe_propose_new_dataset_id(srpl_domain_t *domain)
{
    if (domain->have_dataset_id) {
        for(srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next)
        {
            // as long as there's one instance of the proposed dataset id that has not failed
            // to sync, we are going to wait
            if (!instance->sync_fail &&
                instance->dataset_id == domain->dataset_id)
            {
                INFO("instance " PRI_S_SRP " has matched dataset_id %" PRIx64,
                     instance->instance_name, instance->dataset_id);
                return;
            }
        }
    }
    domain->have_dataset_id = srpl_instances_max_dataset_id(domain, &domain->dataset_id);
    INFO(PRI_S_SRP "propose a new dataset id %" PRIx64, domain->have_dataset_id? "": "fail to ", domain->dataset_id);
}

// We've received a timeout event on the reconnect timer. Generate a reconnect_timeout event and send it to the
// connection.
static void
srpl_connection_reconnect_timeout(void *context)
{
    srpl_connection_t *srpl_connection = context;
    srpl_instance_t *instance = srpl_connection->instance;
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    INFO("reconnect timeout on " PRI_S_SRP, srpl_connection->name);
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_reconnect_timer_expiry);
    srpl_event_deliver(srpl_connection, &event);
    // If we have tried to connect to all the addresses but failed, we assume the peer is
    // gone. We no longer need to synchronize with this peer and if this was an obstacle
    // to enter the routine state, we should recheck again.
    instance->sync_fail = true;
    INFO("fail to sync with instance " PRI_S_SRP, instance->instance_name);
    if (instance != NULL && instance->sync_to_join &&
        domain != NULL && domain->srpl_opstate != SRPL_OPSTATE_ROUTINE)
    {
        instance->sync_to_join = false;
        srpl_maybe_sync_or_transition(domain);
    }
}

static srpl_state_t
srpl_connection_schedule_reconnect_event(srpl_connection_t *srpl_connection, uint32_t when)
{
    // Create a reconnect timer on the srpl_connection_t
    if (srpl_connection->reconnect_wakeup == NULL) {
        srpl_connection->reconnect_wakeup = ioloop_wakeup_create();
        if (srpl_connection->reconnect_wakeup == NULL) {
            ERROR("no memory for reconnect_wakeup for service instance " PRI_S_SRP, srpl_connection->name);
            return srpl_state_invalid;
        }
    } else {
        ioloop_cancel_wake_event(srpl_connection->reconnect_wakeup);
    }
    ioloop_add_wake_event(srpl_connection->reconnect_wakeup, srpl_connection, srpl_connection_reconnect_timeout,
                          srpl_connection_context_release, when);
    RETAIN_HERE(srpl_connection, srpl_connection); // the timer has a reference.
    return srpl_state_invalid;
}

// We reach the set_reconnect_timer state when we have tried to connect to all the known addresses.  Once we have set a
// timer, we wait for events. If we get a reconnect_timeout event, we go to the next_address_get state. If we get an
// add_adress event, we cancel the retransmit timer and go to the next_address_get state.
static srpl_state_t
srpl_reconnect_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    if (event == NULL) {
        return srpl_connection_schedule_reconnect_event(srpl_connection, 60 * 1000);
    }
    if (event->event_type == srpl_event_reconnect_timer_expiry) {
        // if it's not our job to reconnect, we move into idle.
        if (domain->srpl_opstate != SRPL_OPSTATE_STARTUP &&
            domain->partner_id < srpl_connection->instance->partner_id)
        {
            return srpl_state_idle;
        }
        return srpl_state_next_address_get;
    } else if (event->event_type == srpl_event_address_add) {
        ioloop_cancel_wake_event(srpl_connection->reconnect_wakeup);
        // if it's not our job to reconnect, we move into idle.
        if (domain->srpl_opstate != SRPL_OPSTATE_STARTUP &&
            domain->partner_id < srpl_connection->instance->partner_id)
        {
            return srpl_state_idle;
        }
        return srpl_state_next_address_get;
    }
    UNEXPECTED_EVENT(srpl_connection, event);
}

// We get to this state when the remote end has sent something bogus; in this case we send a retry_delay message to
// tell the client not to reconnect for a while.
static srpl_state_t
srpl_retry_delay_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    srpl_retry_delay_send(srpl_connection, srpl_connection->retry_delay);
    return srpl_state_disconnect;
}

// We go to the disconnect state when the connection needs to be dropped either because we lost the session ID
// coin toss or something's gone wrong. In either case, we do not attempt to reconnect--we either go to the idle state
// or the disconnect_wait state, depending on whether or not the connection has already been closed.
static srpl_state_t
srpl_disconnect_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // Any ongoing state needs to be discarded.
    srpl_connection_reset(srpl_connection);

    // Disconnect the srpl_connection_t
    if (srpl_connection->connection == NULL) {
        return srpl_state_idle;
    }
    srpl_trigger_disconnect(srpl_connection);
    return srpl_state_disconnect_wait;
}

// We enter disconnect_wait when we are waiting for a disconnect event after cancelling a connection.
// There is no action for this event. The only event we are interested in is the disconnect event.
static srpl_state_t
srpl_disconnect_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);
    if (event == NULL) {
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_disconnected) {
        return srpl_state_idle;
    } else {
        UNEXPECTED_EVENT_NO_ERROR(srpl_connection, event);
    }
    return srpl_state_invalid;
}

// We enter the connecting state when we've attempted a connection to some address.
// This state has no action. If a connected event is received, we move to the connected state.
// If a disconnected event is received, we move to the next_address_get state.
static srpl_state_t
srpl_connecting_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);
    if (event == NULL) {
        srpl_connection_schedule_state_timeout(srpl_connection, SRPL_STATE_TIMEOUT);
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_disconnected) {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        // We tried to connect and the connection failed. This may mean that the information we see in the _srpl-tls.tcp
        // advertisement is wrong, or that the address records are wrong. Reconfirm the records.
        srpl_reconfirm(srpl_connection);
        return srpl_state_next_address_get;
    } else if (event->event_type == srpl_event_connected) {
        ioloop_cancel_wake_event(srpl_connection->state_timeout);
        return srpl_state_session_send;
    } else {
        UNEXPECTED_EVENT_NO_ERROR(srpl_connection, event);
    }
    return srpl_state_invalid;
}

static void
srpl_sync_wait_check(void *context)
{
    srpl_connection_t *srpl_connection = context;

    if (srpl_connection->instance == NULL) {
        FAULT("srpl_connection->instance shouldn't ever be NULL here, but it is.");
        return;
    }
    if (srpl_connection->instance->domain == NULL) {
        FAULT("srpl_connection->instance->domain shouldn't ever be NULL here, but it is.");
        return;
    }
    if (!srpl_connection->instance->domain->partner_discovery_pending) {
        srpl_maybe_sync_or_transition(srpl_connection->instance->domain);
    }
}

static srpl_state_t
srpl_sync_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);
    if (event == NULL) {
        // This will trigger a do_sync event if we should synchronize at this point.
        ioloop_run_async(srpl_sync_wait_check, srpl_connection);
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_do_sync) {
        // When starting to sync, we reset the keepalive_interval so that we can detect
        // the problem sooner during synchronization.
        srpl_connection->keepalive_interval = DEFAULT_KEEPALIVE_WAKEUP_EXPIRY / 2;
        return srpl_state_send_candidates_send;
    } else {
        UNEXPECTED_EVENT_NO_ERROR(srpl_connection, event);
    }
    return srpl_state_invalid;
}

// This state sends a SRPL session message and then goes to session_response_wait, unless the send failed, in which
// case it goes to the disconnect state.
static srpl_state_t
srpl_session_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // Send a session message
    // Now we say hello.
    if (!srpl_session_message_send(srpl_connection, false)) {
        return srpl_state_disconnect;
    }
    return srpl_state_session_response_wait;
}

// This state waits for a session response with the remote partner ID and whether
// the remote partner is in startup state.
// When the response arrives, it goes to the send_candidates_send state.
static srpl_state_t
srpl_session_response_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);
    if (event == NULL) {
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_session_response_received) {
        // we are connecting and have sent the peer a session message. if the received session
        // response shows that the peer has a lower version number, we don't have to sync with
        // this peer to join the replication. we also note version_mismatch on the corresponding
        // instance and move to the idle state. when we move to the routine state later, we'll
        // pick a dataset id so that the sequence number of our anycast service would win over
        // the low-version peers.
        if (event->content.session.remote_version < SRPL_CURRENT_VERSION) { // version mismatch
            INFO("instance " PRI_S_SRP " has a lower version number", srpl_connection->instance->instance_name);
            srpl_connection->instance->version_mismatch = true;
            srpl_connection->instance->sync_to_join = false;
            return srpl_state_idle;
        }

        srpl_connection->remote_partner_id = event->content.session.partner_id;
        srpl_connection->new_partner = event->content.session.new_partner;
        srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
        // if we are already in the routine state, we can directly move forward
        // with sync; otherwise we put the srpl connection in the sync_wait state
        // where we check the number of active srp servers to decide if we should
        // continue sync at this point.
        if (domain->srpl_opstate == SRPL_OPSTATE_ROUTINE) {
            return srpl_state_send_candidates_send;
        }
        return srpl_state_sync_wait;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
    return srpl_state_invalid;
}

// When evaluating the incoming session, we've decided to continue (called by srpl_session_evaluate_action).
static srpl_state_t
srpl_evaluate_incoming_continue(srpl_connection_t *srpl_connection)
{
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    if (srpl_connection->new_partner) {
        INFO(PRI_S_SRP " connecting partner is in startup state", srpl_connection->name);
    } else {
        INFO(PRI_S_SRP ": my partner id %" PRIx64 " < connecting partner id %" PRIx64,
             srpl_connection->name, domain->partner_id, srpl_connection->remote_partner_id);
    }
    if (srpl_connection->is_server) {
        return srpl_state_session_response_send;
    } else {
        return srpl_state_send_candidates_send;
    }
}

// When evaluating the incoming ID, we've decided to disconnect (called by srpl_session_evaluate_action).
static srpl_state_t
srpl_evaluate_incoming_disconnect(srpl_connection_t *srpl_connection, bool bad)
{
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    if (domain->srpl_opstate != SRPL_OPSTATE_ROUTINE) {
        INFO(PRI_S_SRP ": not in the routine state yet in domain " PRI_S_SRP,
             srpl_connection->name, domain->name);
    } else {
        INFO(PRI_S_SRP ": my partner id %" PRIx64 " > connectiong partner id %" PRIx64,
             srpl_connection->name, domain->partner_id, srpl_connection->remote_partner_id);
    }
    if (srpl_connection->instance->is_me) {
        return srpl_evaluate_incoming_continue(srpl_connection);
    } else {
        if (bad) {
            // bad is set if the server send back the same ID we sent, which means it's misbehaving.
            return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
        }
        return srpl_state_disconnect;
    }
}

// This state's action is to evaluate if the partner should accept the connection.
// The receiving partner accepts the connection if the connecting partner is in the
// "startup" state (flaged as new partner), or the receiving partner's ID is smaller
// than the connecting partner's ID. Otherwise, the receiving partner disconnects.
static srpl_state_t
srpl_session_evaluate_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);

    // The receiving partner must be in routine state to accept the connection.
    // Recceiving connection in startup state should not happen, but add a guard
    // here anyway to protect against such situation.
    if (domain->srpl_opstate == SRPL_OPSTATE_ROUTINE && (srpl_connection->new_partner ||
        domain->partner_id < srpl_connection->remote_partner_id))
    {
        return srpl_evaluate_incoming_continue(srpl_connection);
    } else {
        return srpl_evaluate_incoming_disconnect(srpl_connection, false);
    }
}

// This state's action is to send the "send candidates" message, and then go to the send_candidates_wait state.
static srpl_state_t
srpl_send_candidates_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // Send "send candidates" message
    // Return no event
    srpl_send_candidates_message_send(srpl_connection, false);
    return srpl_state_send_candidates_wait;
}

static bool
srpl_can_transition_to_routine_state(srpl_domain_t *domain)
{
    if (domain == NULL) {
        INFO("returning false because there's no domain");
        return false;
    }

    // We only transition to routine state after discovery is completed, as
    // we need to sync with all the partners discovered during the discovery
    // window to join the replication
    if (domain->partner_discovery_pending) {
        INFO("returning false because partner discovery is still pending");
        return false;
    }

    if (domain->srpl_opstate == SRPL_OPSTATE_ROUTINE) {
        INFO("returning false because we are already in routine state");
        return false;
    }

    if (domain->have_dataset_id) {
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            // We skip checking the instance if the instance
            // 1. is to myself (this could happen if I restarts and receives a stale advertisement from myself).
            // 2. is not discovered during the discovery_timeout, or
            // 3. the dataset id is not what we are looking for, or
            // 4. is discontinuing, or
            // 5. has lower version
            if (instance->is_me || !instance->sync_to_join ||
                instance->dataset_id != domain->dataset_id ||
                instance->discontinuing || instance->version_mismatch)
            {
                INFO("instance " PUB_S_SRP ": is_me (" PUB_S_SRP ") or sync_to_join (" PUB_S_SRP
                     ") or discontinuing (" PUB_S_SRP ") or version_mismatch (" PUB_S_SRP ")",
                     instance->instance_name, instance->is_me ? "true" : "false", instance->sync_to_join ? "true" : "false",
                     instance->discontinuing ? "true" : "false", instance->version_mismatch ? "true" : "false");
                continue;
            }
            // Instance is a valid partner that we should sync with to possibly move to the routine state
            if (instance->connection == NULL ||
                !instance->connection->database_synchronized)
            {
                INFO("synchronization on " PRI_S_SRP " with partner_id %" PRIx64 " is not ready (%p " PUB_S_SRP ").",
                     instance->instance_name, instance->partner_id, instance->connection,
                     (instance->connection == NULL ? "null" :
                     instance->connection->database_synchronized ? "true" : "false"));
                return false;
            }
        }
    }
    INFO("ready");
    return true;
}

static void
srpl_store_dataset_id(srpl_domain_t *domain)
{
    uint64_t dataset_id = domain->dataset_id;
    uint8_t msb;
    OSStatus err;

    // read out the stored msb of the dataset id. increment the msb and generate the dataset id.
    const CFStringRef app_id = CFSTR("com.apple.srp-mdns-proxy.preferences");
    const CFStringRef key = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("dataset-id-msb-%s"), domain->name);

    if (key) {
        msb = (dataset_id & 0xFF00000000000000) >> 56;
        err = CFPrefs_SetInt64(app_id, key, msb);

        if (err) {
            ERROR("Unable to store the msb of the dataset id in preferences.");
        }
        INFO("store msb %d.", msb);
        CFRelease(key);
    } else {
        ERROR("unable to create key for domain " PRI_S_SRP, domain->name);
    }
}


// SRPL partners MUST persist the highest (most significant byte or MSB) of the dataset ID.
// When generating a new dataset ID, the partner MUST increment the MSB of last used dataset
// ID to use as MSB of new dataset ID and populate the lower 56 bits randomly. If there is no
// previously saved ID, then the partner randomly generates the entire 64-bit ID.
static uint64_t
srpl_generate_store_dataset_id(srpl_domain_t *domain)
{
    uint64_t dataset_id;
    uint8_t msb;
    OSStatus err;

    // read out the stored msb of the dataset id. increment the msb and generate the dataset id.
    const CFStringRef app_id = CFSTR("com.apple.srp-mdns-proxy.preferences");
    const CFStringRef key = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("dataset-id-msb-%s"), domain->name);

    if (key) {
        msb = (uint8_t)CFPrefs_GetInt64(app_id, key, &err);
        if (err) {
            INFO("fail to fetch msb, generate random dataset id.");
            dataset_id = srp_random64();
        } else {
            dataset_id = (((uint64_t)msb+1) << 56) | (srp_random64() & LOWER56_BIT_MASK);
        }
        // store the most significant byte (msb) of the generated dataset id
        msb = (dataset_id & 0xFF00000000000000) >> 56;
        err = CFPrefs_SetInt64(app_id, key, msb);

        if (err) {
            ERROR("Unable to store the msb of the dataset id in preferences.");
        }
        CFRelease(key);
    } else {
        ERROR("unable to create key for domain " PRI_S_SRP, domain->name);
        dataset_id = srp_random64();
    }

    return dataset_id;
}

static void
srpl_transition_to_routine_state(srpl_domain_t *domain)
{
    domain->srpl_opstate = SRPL_OPSTATE_ROUTINE;
    INFO("transitions to routine state in domain " PRI_S_SRP, domain->name);
    // If the partner does not discover any other partners advertising
    // the same domain in the "startup" state, it generates a new dataset
    // ID when entering the "routine operation" state.
    // When generating a new dataset ID, the partner MUST increment the MSB of
    // last used dataset ID to use as MSB of new dataset ID and populate the
    // lower 56 bits randomly using a random number generator. If there is no
    // previously saved ID, then the partner randomly generates the entire 64-bit ID.
    if (!domain->have_dataset_id) {
        domain->dataset_id = srpl_generate_store_dataset_id(domain);
        domain->have_dataset_id = true;
        domain->dataset_id_committed = true;
        INFO("generate new dataset id %" PRIx64 " for domain " PRI_S_SRP,
             domain->dataset_id, domain->name);
    } else {
        // if there are instances that have lower version number but the MSB of their dataset_id
        // is larger than ours, we'll increment the MSB of the instance dataset id and use it
        // as our dataset id. this is to guarantee that the current version has a higher sequence
        // number so that its service will be preferred.
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            if (instance->version_mismatch &&
                srpl_dataset_id_compare(instance->dataset_id & 0xFF00000000000000,
                                        domain->dataset_id & 0xFF00000000000000) >= 0)
            {
                INFO("increment the MSB of the dataset_id %" PRIx64 " of instance " PRI_S_SRP,
                     instance->dataset_id, instance->instance_name);
                domain->dataset_id = instance->dataset_id + 0x0100000000000000;
            }
        }
    }
#if STUB_ROUTER
    srp_server_t *server_state = domain->server_state;
    // Advertise the SRPL service in the "routine" state.
    srpl_domain_advertise(domain);
    if (!strcmp(domain->name, server_state->current_thread_domain_name)) {
        route_state_t *route_state = server_state->route_state;
        if (route_state != NULL) {
            route_state->thread_sequence_number = (domain->dataset_id & 0xFF00000000000000) >> 56;
            INFO("thread sequence number 0x%02x", route_state->thread_sequence_number);
            route_state->partition_can_advertise_anycast_service = true;
            partition_maybe_advertise_anycast_service(route_state);
        }
    }
#endif
}

static bool
srpl_keep_current_dataset_id(srpl_domain_t *domain, srpl_instance_t *instance)
{
    bool keep = true;
    bool stored = domain->dataset_id_committed;
    if (instance->have_dataset_id) {
        uint64_t new = instance->dataset_id;
        int compare = srpl_dataset_id_compare(new, domain->dataset_id);
        if (compare == 0) {
            domain->dataset_id_committed = true;
            keep = true;
            INFO("keep and commit dataset_id %" PRIx64, domain->dataset_id);
        } else if (compare > 0) {
            INFO("abandon dataset_id %" PRIx64 " and commit preferred %" PRIx64, domain->dataset_id, new);
            domain->dataset_id = new;
            domain->dataset_id_committed = true;
            keep = false;
        } else {
            INFO("non-preferred dataset id %" PRIx64, instance->dataset_id);
        }
    }
    // we store the msb of dataset id if we haven't done so for current
    // committed dataset id or the committed dataset id has changed.
    if ((!stored || !keep) && domain->dataset_id_committed) {
        srpl_store_dataset_id(domain);
    }
    return keep;
}

static void
srpl_state_transition_by_dataset_id(srpl_domain_t *domain, srpl_instance_t *instance)
{
    if (!srpl_keep_current_dataset_id(domain, instance)) {
        // DNS-SD SRP Replication Spec: if at any time (regardless of "startup" or "routine
        // operation" state) an SRPL partner discovers that it is synchronizing with a
        // non-preferred dataset ID, it MUST abandon that dataset, re-enter the "startup"
        // state, and attempt to synchronize with the (newly discovered) preferred dataset id.
        INFO("more preferred dataset id %" PRIx64 ", reenter startup state", domain->dataset_id);
        srpl_abandon_nonpreferred_dataset(domain);
        srpl_transition_to_startup_state(domain);
    } else {
        srpl_maybe_sync_or_transition(domain);
    }
}

// Used by srpl_send_candidates_wait_action and srpl_host_wait_action
static srpl_state_t
srpl_send_candidates_wait_event_process(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
    if (event->event_type == srpl_event_send_candidates_response_received) {
        if (srpl_connection->is_server) {
            srpl_connection->database_synchronized = true;
            srpl_instance_t *instance = srpl_connection->instance;
            instance->sync_fail = false;
            srpl_state_transition_by_dataset_id(domain, instance);
            return srpl_state_ready;
        } else {
            return srpl_state_send_candidates_message_wait;
        }
    } else if (event->event_type == srpl_event_candidate_received) {
        srpl_connection_candidate_set(srpl_connection, event->content.candidate);
        event->content.candidate = NULL; // steal!
        return srpl_state_candidate_check;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// We reach this state after having sent a "send candidates" message, so we can in principle get either a
// "candidate" message or a "send candidates" response here, leading either to send_candidates check or one
// of two states depending on whether this connection is an incoming or outgoing connection. Outgoing
// connections send the "send candidates" message first, so when they get a "send candidates" reply, they
// need to wait for a "send candidates" message from the remote. Incoming connections send the "send candidates"
// message last, so when they get the "send candidates" reply, the database sync is done and it's time to
// just deal with ongoing updates. In this case we go to the check_for_srp_client_updates state, which
// looks to see if any updates came in from SRP clients while we were syncing the databases.
static srpl_state_t
srpl_send_candidates_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    }
    return srpl_send_candidates_wait_event_process(srpl_connection, event);
}

static srpl_candidate_disposition_t
srpl_candidate_host_check(srpl_connection_t *srpl_connection, adv_host_t *host)
{
    // Evaluate candidate
    // Return "host candidate wanted" or "host candidate not wanted" event
    if (host == NULL) {
        INFO("host is NULL, answer is yes.");
        return srpl_candidate_yes;
    } else {
        if (host->removed) {
            INFO("host is removed, answer is yes.");
            return srpl_candidate_yes;
        } else if (host->key_id != srpl_connection->candidate->key_id) {
            INFO("host key conflict (%x vs %x), answer is conflict.", host->key_id, srpl_connection->candidate->key_id);
            return srpl_candidate_conflict;
        } else {
            // We allow for a bit of jitter. Bear in mind that candidates only happen on startup, so
            // even if a previous run of the SRP server on this device was responsible for registering
            // the candidate, we don't have it, so we still need it.
            if (host->update_time - srpl_connection->candidate->update_time > SRPL_UPDATE_JITTER_WINDOW) {
                INFO("host update time %" PRId64 " candidate update time %" PRId64 ", answer is no.",
                     (int64_t)host->update_time, (int64_t)srpl_connection->candidate->update_time);
                return srpl_candidate_no;
            } else {
                INFO("host update time %" PRId64 " candidate update time %" PRId64 ", answer is yes.",
                     (int64_t)host->update_time, (int64_t)srpl_connection->candidate->update_time);
                return srpl_candidate_yes;
            }
        }
    }
}

// We enter this state after we've received a "candidate" message, and check to see if we want the host the candidate
// represents. We then send an appropriate response.
static srpl_state_t
srpl_candidate_check_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS_NAME(srpl_connection, srpl_connection->candidate->name);

    adv_host_t *host = srp_adv_host_copy(srpl_connection->instance->domain->server_state,
                                         srpl_connection->candidate->name);
    srpl_candidate_disposition_t disposition = srpl_candidate_host_check(srpl_connection, host);
    if (host != NULL) {
        srp_adv_host_release(host);
    }
    switch(disposition) {
    case srpl_candidate_yes:
        srpl_candidate_response_send(srpl_connection, kDSOType_SRPLCandidateYes);
        return srpl_state_candidate_host_wait;
    case srpl_candidate_no:
        srpl_candidate_response_send(srpl_connection, kDSOType_SRPLCandidateNo);
        return srpl_state_send_candidates_wait;
    case srpl_candidate_conflict:
        srpl_candidate_response_send(srpl_connection, kDSOType_SRPLConflict);
        return srpl_state_send_candidates_wait;
    }
    return srpl_state_invalid;
}

// In candidate_host_send_wait, we take no action and wait for events. We're hoping for a "host" message, leading to
// candidate_host_prepare. We could also receive a "candidate" message, leading to candidate_received, or a "send
// candidates" reply, leading to candidate_reply_received.

static srpl_state_t
srpl_candidate_host_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_host_message_received) {
        // Copy the update information, retain what's refcounted, and free what's not on the event.
        srpl_host_update_steal_parts(&srpl_connection->stashed_host, &event->content.host_update);
        return srpl_state_candidate_host_prepare;
    } else {
        return srpl_send_candidates_wait_event_process(srpl_connection, event);
    }
}

// Here we want to see if we can do an immediate update; if so, we go to candidate_host_re_evaluate; otherwise
// we go to candidate_host_contention_wait
static srpl_state_t
srpl_candidate_host_prepare_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS_NAME(srpl_connection, srpl_connection->candidate->name);

    // Apply the host from the event to the current host list
    // Return no event
    adv_host_t *host = srp_adv_host_copy(srpl_connection->instance->domain->server_state,
                                         srpl_connection->candidate->name);
    if (host == NULL) {
        // If we don't have this host, we can apply the update immediately.
        return srpl_state_candidate_host_apply;
    }
    if (host->srpl_connection != NULL || host->update != NULL) {
        // We are processing an update from a different srpl server or a client.
        INFO(PRI_S_SRP ": host->srpl_connection = %p  host->update=%p--going into contention",
             srpl_connection->name, host->srpl_connection, host->update);
        srp_adv_host_release(host);
        return srpl_state_candidate_host_contention_wait;
    } else {
        srpl_connection->candidate->host = host;
        return srpl_state_candidate_host_re_evaluate;
    }
}

static adv_host_t *
srpl_client_update_matches(dns_name_t *hostname, srpl_event_t *event)
{
    adv_host_t *host = event->content.client_result.host;
    if (event->content.client_result.rcode == dns_rcode_noerror && dns_names_equal_text(hostname, host->name)) {
        INFO("returning host " PRI_S_SRP, host->name);
        return host;
    }
    char name[kDNSServiceMaxDomainName];
    dns_name_print(hostname, name, sizeof(name));
    INFO("returning NULL: rcode = " PUB_S_SRP "  hostname = " PRI_S_SRP "  host->name = " PRI_S_SRP,
         dns_rcode_name(event->content.client_result.rcode), name, host->name);
    return NULL;
}

// and wait for a srp_client_update_finished event for the host, which
// will trigger us to move to candidate_host_re_evaluate.
static srpl_state_t
srpl_candidate_host_contention_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_srp_client_update_finished) {
        adv_host_t *host = srpl_client_update_matches(srpl_connection->candidate->name, event);
        if (host != NULL) {
            srpl_connection->candidate->host = host;
            srp_adv_host_retain(srpl_connection->candidate->host);
            return srpl_state_candidate_host_re_evaluate;
        }
        return srpl_state_invalid; // Keep waiting
    } else if (event->event_type == srpl_event_advertise_finished) {
        // See if this is an event on the host we were waiting for.
        if (event->content.advertise_finished.hostname != NULL &&
            dns_names_equal_text(srpl_connection->candidate->name, event->content.advertise_finished.hostname))
        {
            return srpl_state_candidate_host_re_evaluate;
        }
        return srpl_state_invalid;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// At this point we've either waited for the host to no longer be in contention, or else it wasn't in contention.
// There was a time gap between when we sent the candidate response and when the host message arrived, so an update
// may have arrived locally for that SRP client. We therefore re-evaluate at this point.
static srpl_state_t
srpl_candidate_host_re_evaluate_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS_NAME(srpl_connection, srpl_connection->candidate->name);

    adv_host_t *host = srpl_connection->candidate->host;
    // The host we retained may have become invalid; if so, discard it
    if (host != NULL && !srp_adv_host_valid(host)) {
        srp_adv_host_release(srpl_connection->candidate->host);
        srpl_connection->candidate->host = host = NULL;
    }
    // If it was invalidated, or if we got here directly, look up the host by name
    if (host == NULL) {
        host = srp_adv_host_copy(srpl_connection->instance->domain->server_state,
                                 srpl_connection->candidate->name);
        srpl_connection->candidate->host = host;
    }
    // It's possible that the host is gone; in this case we definitely want the update.
    if (host == NULL) {
        return srpl_state_candidate_host_apply;
    }

    // At this point we know that the host we were looking for is valid. Now check to see if we still want to apply it.
    srpl_state_t ret = srpl_state_invalid;
    srpl_candidate_disposition_t disposition = srpl_candidate_host_check(srpl_connection, host);
    switch(disposition) {
    case srpl_candidate_yes:
        ret = srpl_state_candidate_host_apply;
        break;
    case srpl_candidate_no:
        // This happens if we got a candidate and wanted it, but then got an SRP update on that candidate while waiting
        // for events. In this case, there's no real problem, and the successful update should trigger an update to be
        // sent to the remote.
        srpl_host_response_send(srpl_connection, dns_rcode_noerror);
        INFO("candidate_no: freeing parts");
        srpl_host_update_parts_free(&srpl_connection->stashed_host);
        ret = srpl_state_send_candidates_wait;
        break;
    case srpl_candidate_conflict:
        srpl_host_response_send(srpl_connection, dns_rcode_yxdomain);
        INFO("candidate_conflict: freeing parts");
        srpl_host_update_parts_free(&srpl_connection->stashed_host);
        ret = srpl_state_send_candidates_wait;
        break;
    }
    return ret;
}

static bool
srpl_connection_host_apply(srpl_connection_t *srpl_connection)
{
    DNS_NAME_GEN_SRP(srpl_connection->stashed_host.hostname, name_buf);
    INFO("applying update from " PRI_S_SRP " for host " PRI_DNS_NAME_SRP ", %d messages",
         srpl_connection->name, DNS_NAME_PARAM_SRP(srpl_connection->stashed_host.hostname, name_buf),
         srpl_connection->stashed_host.num_messages);
    if (!srp_parse_host_messages_evaluate(srpl_connection->instance->domain->server_state, srpl_connection,
                                          srpl_connection->stashed_host.messages,
                                          srpl_connection->stashed_host.num_messages))
    {
        srpl_host_response_send(srpl_connection, dns_rcode_formerr);
        return false;
    }
    return true;
}

// At this point we know there is no contention on the host, and we want to update it, so start the update by passing the
// host message to dns_evaluate.
static srpl_state_t
srpl_candidate_host_apply_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    // Apply the host from the event to the current host list
    // Return no event
    // Note that we set host->srpl_connection _after_ we call dns_evaluate. This ensures that any "advertise_finished"
    // calls that are done during the call to dns_evaluate do not deliver an event here.
    if (event == NULL) {
        if (!srpl_connection_host_apply(srpl_connection)) {
            return srpl_state_send_candidates_wait;
        }
        return srpl_state_candidate_host_apply_wait;
    } else if (event->event_type == srpl_event_advertise_finished) {
        // This shouldn't be possible anymore, but I'm putting a FAULT in here in case I'm mistaken.
        FAULT(PRI_S_SRP ": advertise_finished event!", srpl_connection->name);
        return srpl_state_invalid;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// Called by the SRP server when an advertise has finished for an update recevied on a connection.
static void
srpl_deferred_advertise_finished_event_deliver(void *context)
{
    srpl_event_t *event = context;
    srp_server_t *server_state = event->content.advertise_finished.server_state;
    for (srpl_domain_t *domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            if (instance->connection != NULL) {
                srpl_event_deliver(instance->connection, event);
            }
        }
    }
    for (srpl_instance_t *instance = server_state->unmatched_instances; instance != NULL; instance = instance->next) {
        if (instance->connection != NULL) {
            srpl_event_deliver(instance->connection, event);
        }
    }

    free(event->content.advertise_finished.hostname);
    free(event);
}

// Send an advertise_finished event for the specified hostname to all connections. Because this is called from
// advertise_finished, we do not want any state machine to advance immediately, so we defer delivery of this
// event until the next time we return to the main event loop.
void
srpl_advertise_finished_event_send(char *hostname, int rcode, srp_server_t *server_state)
{
    srpl_event_t *event = calloc(1, sizeof(*event));
    if (event == NULL) {
        ERROR("No memory to defer advertise_finished event for " PUB_S_SRP, hostname);
        return;
    }

    srpl_event_initialize(event, srpl_event_advertise_finished);
    event->content.advertise_finished.rcode = rcode;
    event->content.advertise_finished.hostname = strdup(hostname);
    event->content.advertise_finished.server_state = server_state;
    if (event->content.advertise_finished.hostname == NULL) {
        INFO(PRI_S_SRP ": no memory for hostname", hostname);
        free(event);
        return;
    }
    ioloop_run_async(srpl_deferred_advertise_finished_event_deliver, event);
}


// We enter this state to wait for the application of a host update to complete.
// We exit the state for the send_candidates_wait state when we receive an advertise_finished event.
// Additionally when we receive an advertise_finished event we send a "host" response with the rcode
// returned in the advertise_finished event.
static srpl_state_t
srpl_candidate_host_apply_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_advertise_finished) {
        srpl_host_response_send(srpl_connection, event->content.advertise_finished.rcode);
        INFO("freeing parts");
        srpl_host_update_parts_free(&srpl_connection->stashed_host);
        return srpl_state_send_candidates_wait;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// This marks the end of states that occur as a result of sending a "send candidates" message.
// This marks the beginning of states that occur as a result of receiving a send_candidates message.

// We have received a "send candidates" message; the action is to create a candidates list.
static srpl_state_t
srpl_send_candidates_received_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    int num_candidates;
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // Make sure we don't have a candidate list.
    if (srpl_connection->candidates != NULL) {
        srpl_connection_candidates_free(srpl_connection);
        srpl_connection->candidates = NULL;
        // Just in case we exit due to a failure...
        srpl_connection->num_candidates = 0;
        srpl_connection->current_candidate = -1;
    }
    // Generate a list of candidates from the current host list.
    // Return no event
    srp_server_t *server_state = srpl_connection->instance->domain->server_state;
    num_candidates = srp_current_valid_host_count(server_state);
    if (num_candidates > 0) {
        adv_host_t **candidates = calloc(num_candidates, sizeof(*candidates));
        int copied_candidates;
        if (candidates == NULL) {
            ERROR("unable to allocate candidates list.");
            return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
        }
        copied_candidates = srp_hosts_to_array(server_state, candidates, num_candidates);
        if (copied_candidates > num_candidates) {
            FAULT("copied_candidates %d > num_candidates %d",
                  copied_candidates, num_candidates);
            return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
        }
        if (num_candidates != copied_candidates) {
            INFO("srp_hosts_to_array returned the wrong number of hosts: copied_candidates %d > num_candidates %d",
                 copied_candidates, num_candidates);
            num_candidates = copied_candidates;
        }
        srpl_connection->candidates = candidates;
    }
    srpl_connection->candidates_not_generated = false;
    srpl_connection->num_candidates = num_candidates;
    srpl_connection->current_candidate = -1;
    return srpl_state_send_candidates_remaining_check;
}

// See if there are candidates remaining; if not, send "send candidates" response.
static srpl_state_t
srpl_candidates_remaining_check_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // Get the next candidate out of the candidate list
    // Return "no candidates left" or "next candidate"
    if (srpl_connection->current_candidate + 1 < srpl_connection->num_candidates) {
        srpl_connection->current_candidate++;
        return srpl_state_next_candidate_send;
    } else {
        return srpl_state_send_candidates_response_send;
    }
}

// Send the next candidate.
static srpl_state_t
srpl_next_candidate_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    srpl_candidate_message_send(srpl_connection, srpl_connection->candidates[srpl_connection->current_candidate]);
    return srpl_state_next_candidate_send_wait;
}

// Wait for a "candidate" response.
static srpl_state_t
srpl_next_candidate_send_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_candidate_response_received) {
        switch (event->content.disposition) {
        case srpl_candidate_yes:
            return srpl_state_candidate_host_send;
        case srpl_candidate_no:
        case srpl_candidate_conflict:
            return srpl_state_send_candidates_remaining_check;
        }
        return srpl_state_invalid;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// Send the host for the candidate.
static srpl_state_t
srpl_candidate_host_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    // It's possible that the host that we put on the candidates list has become invalid. If so, just go back and send
    // the next candidate (or finish).
    adv_host_t *host = srpl_connection->candidates[srpl_connection->current_candidate];
    if (!srp_adv_host_valid(host) || host->message == NULL) {
        return srpl_state_send_candidates_remaining_check;
    }
    if (!srpl_host_message_send(srpl_connection, host)) {
        srpl_disconnect(srpl_connection);
        return srpl_state_invalid;
    }
    return srpl_state_candidate_host_response_wait;
}

// Wait for a "host" response.
static srpl_state_t
srpl_candidate_host_response_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_host_response_received) {
        // The only failure case we care about is a conflict, and we don't have a way to handle that, so just
        // continue without checking the status.
        return srpl_state_send_candidates_remaining_check;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// At this point we're done sending candidates, so we send a "send candidates" response.
static srpl_state_t
srpl_send_candidates_response_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    srpl_send_candidates_message_send(srpl_connection, true);
    // When the server has sent its candidate response, it's immediately ready to send a "send candidate" message
    // When the client has sent its candidate response, the database synchronization is done on the client.
    if (srpl_connection->is_server) {
        return srpl_state_send_candidates_send;
    } else {
        srpl_domain_t *domain = srpl_connection_domain(srpl_connection);
        srpl_connection->database_synchronized = true;
        srpl_instance_t *instance = srpl_connection->instance;
        instance->sync_fail = false;
        srpl_state_transition_by_dataset_id(domain, instance);
        return srpl_state_ready;
    }
}

// The ready state is where we land when there's no remaining work to do. We wait for events, and when we get one,
// we handle it, ultimately returning to this state.
static srpl_state_t
srpl_ready_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        // Whenever we newly land in this state, see if there is an unsent client update at the head of the
        // queue, and if so, send it.
        if (srpl_connection->client_update_queue != NULL && !srpl_connection->client_update_queue->sent) {
            adv_host_t *host = srpl_connection->client_update_queue->host;
            if (host == NULL || host->name == NULL) {
                INFO(PRI_S_SRP ": we have an update to send for bogus host %p.", srpl_connection->name, host);
            } else {
                INFO(PRI_S_SRP ": we have an update to send for host " PRI_S_SRP, srpl_connection->name,
                     srpl_connection->client_update_queue->host->name);
            }
            return srpl_state_srp_client_update_send;
        } else {
            if (srpl_connection->client_update_queue != NULL) {
                adv_host_t *host = srpl_connection->client_update_queue->host;
                if (host == NULL || host->name == NULL) {
                    INFO(PRI_S_SRP ": there is anupdate that's marked sent for bogus host %p.",
                         srpl_connection->name, host);
                } else {
                    INFO(PRI_S_SRP ": there is an update on the queue that's marked sent for host " PRI_S_SRP,
                         srpl_connection->name, host->name);
                }
            } else {
                INFO(PRI_S_SRP ": the client update queue is empty.", srpl_connection->name);
            }
        }
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_host_message_received) {
        if (srpl_connection->stashed_host.messages != NULL) {
            FAULT(PRI_S_SRP ": stashed host present but host message received", srpl_connection->name);
            return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
        }
        // Copy the update information, retain what's refcounted, and NULL out what's not, on the event.
        srpl_host_update_steal_parts(&srpl_connection->stashed_host, &event->content.host_update);
        return srpl_state_stashed_host_check;
    } else if (event->event_type == srpl_event_host_response_received) {
        return srpl_state_srp_client_ack_evaluate;
    } else if (event->event_type == srpl_event_advertise_finished) {
        if (srpl_connection->stashed_host.hostname != NULL &&
            event->content.advertise_finished.hostname != NULL &&
            dns_names_equal_text(srpl_connection->stashed_host.hostname, event->content.advertise_finished.hostname))
        {
            srpl_connection->stashed_host.rcode = event->content.advertise_finished.rcode;
            return srpl_state_stashed_host_finished;
        }
        return srpl_state_invalid;
    } else if (event->event_type == srpl_event_srp_client_update_finished) {
        // When we receive a client update in ready state, we just need to re-run the state's action.
        return srpl_state_ready;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// We get here when there is at least one client update queued up to send
static srpl_state_t
srpl_srp_client_update_send_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    srpl_srp_client_queue_entry_t *update = srpl_connection->client_update_queue;
    if (update != NULL) {
        // If the host has a message, send it. Note that this host may well be removed, but if it had been removed
        // through a lease expiry we wouldn't have got here, because the host object would have been removed from
        // the list. So if it has a message attached to it, that means that either it's been removed explicitly by
        // the client, which we need to propagate, or else it is still valid, and so we need to propagate the most
        // recent update we got.
        if (update->host->message != NULL) {
            srpl_host_message_send(srpl_connection, update->host);
            update->sent = true;
        } else {
            ERROR(PRI_S_SRP ": no host message to send for host " PRI_S_SRP ".",
                  srpl_connection->name, update->host->name);

            // We're not going to send this update, so take it off the queue.
            srpl_connection->client_update_queue = update->next;
            srp_adv_host_release(update->host);
            free(update);
        }
    }
    return srpl_state_ready;
}

// We go here when we get a "host" response; all we do is remove the host from the top of the queue.
static srpl_state_t
srpl_srp_client_ack_evaluate_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    if (srpl_connection->client_update_queue == NULL) {
        FAULT(PRI_S_SRP ": update queue empty in ready, but host_response_received event received.",
              srpl_connection->name);
        return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
    }
    if (!srpl_connection->client_update_queue->sent) {
        FAULT(PRI_S_SRP ": top of update queue not sent, but host_response_received event received.",
              srpl_connection->name);
        return srpl_connection_drop_state(srpl_connection->instance, srpl_connection);
    }
    srpl_srp_client_queue_entry_t *finished_update = srpl_connection->client_update_queue;
    srpl_connection->client_update_queue = finished_update->next;
    if (finished_update->host != NULL) {
        srp_adv_host_release(finished_update->host);
    }
    free(finished_update);
    return srpl_state_ready;
}

// We go here when we get a "host" message
static srpl_state_t
srpl_stashed_host_check_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS_NAME(srpl_connection, srpl_connection->stashed_host.hostname);

    adv_host_t *host = srp_adv_host_copy(srpl_connection->instance->domain->server_state,
                                         srpl_connection->stashed_host.hostname);
    // No contention...
    if (host == NULL) {
        INFO("applying host because it doesn't exist locally.");
        return srpl_state_stashed_host_apply;
    } else if (host->update == NULL && host->srpl_connection == NULL) {
        INFO("applying host because there's no contention.");
        srp_adv_host_release(host);
        return srpl_state_stashed_host_apply;
    } else {
        INFO("not applying host because there is contention. host->update %p   host->srpl_connection: %p",
             host->update, host->srpl_connection);
    }
    srp_adv_host_release(host);
    return srpl_state_ready; // Wait for something to happen
}

// We go here when we have a stashed host to apply.
static srpl_state_t
srpl_stashed_host_apply_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        if (!srpl_connection_host_apply(srpl_connection)) {
            srpl_connection->stashed_host.rcode = dns_rcode_servfail;
            return srpl_state_stashed_host_finished;
        }
        return srpl_state_ready; // Wait for something to happen
    } else if (event->event_type == srpl_event_advertise_finished) {
        // This shouldn't be possible anymore, but I'm putting a FAULT in here in case I'm mistaken.
        FAULT(PRI_S_SRP ": advertise_finished event!", srpl_connection->name);
        return srpl_state_invalid;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// We go here when a host update advertise finishes.
static srpl_state_t
srpl_stashed_host_finished_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    if (srpl_connection->stashed_host.hostname == NULL) {
        FAULT(PRI_S_SRP ": stashed host not present, but advertise_finished event received.", srpl_connection->name);
        return srpl_state_ready;
    }
    if (srpl_connection->stashed_host.messages == NULL) {
        FAULT(PRI_S_SRP ": stashed host present, no messages.", srpl_connection->name);
        return srpl_state_ready;
    }
    srpl_host_response_send(srpl_connection, srpl_connection->stashed_host.rcode);
    INFO("freeing parts");
    srpl_host_update_parts_free(&srpl_connection->stashed_host);
    return srpl_state_ready;
}

// We land here immediately after a server connection is received.
static srpl_state_t
srpl_session_message_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_session_message_received) {
        // we check the remote_version. if it's smaller than our current version, we note
        // version_mismatch on the instance and move to the idle state. Since no response
        // is sent out, the remote peer will stay in the session_response_wait.
        if (event->content.session.remote_version < SRPL_CURRENT_VERSION) { // version mismatch
            INFO("instance " PRI_S_SRP " has a lower version number", srpl_connection->instance->instance_name);
            srpl_connection->instance->version_mismatch = true;
            return srpl_state_idle;
        }
        srpl_connection->remote_partner_id = event->content.session.partner_id;
        srpl_connection->new_partner = event->content.session.new_partner;
        return srpl_state_session_evaluate;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

// Send a session response
static srpl_state_t
srpl_session_response_send(srpl_connection_t *UNUSED srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_EVENT_NULL(srpl_connection, event);
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE_NO_EVENTS(srpl_connection);

    if (!srpl_session_message_send(srpl_connection, true)) {
        return srpl_state_disconnect;
    }
    return srpl_state_send_candidates_message_wait;
}

// We land here immediately after a server connection is received.
static srpl_state_t
srpl_send_candidates_message_wait_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    REQUIRE_SRPL_INSTANCE(srpl_connection);
    STATE_ANNOUNCE(srpl_connection, event);

    if (event == NULL) {
        return srpl_state_invalid; // Wait for events.
    } else if (event->event_type == srpl_event_send_candidates_message_received) {
        return srpl_state_send_candidates_received;
    } else {
        UNEXPECTED_EVENT(srpl_connection, event);
    }
}

#ifdef SRP_TEST_SERVER
// When testing, we may want an srpl_connection_t that just calls back to the test system when an event
// is delivered and otherwise does nothing.
static srpl_state_t
srpl_test_event_intercept_action(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    STATE_ANNOUNCE(srpl_connection, event);

    return test_packet_srpl_intercept(srpl_connection, event);
}
#endif

// Check to see if host is on the list of remaining candidates to send. If so, no need to do anything--it'll go out soon.
static bool
srpl_reschedule_candidate(srpl_connection_t *srpl_connection, adv_host_t *host)
{
    // We don't need to queue new updates if we haven't yet generated a candidates list.
    if (srpl_connection->candidates_not_generated) {
        INFO("returning true because we haven't generated candidates.");
        return true;
    }
    if (srpl_connection->candidates == NULL) {
        INFO("returning false because we have no candidates.");
        return false;
    }
    for (int i = srpl_connection->current_candidate + 1; i < srpl_connection->num_candidates; i++) {
        if (srpl_connection->candidates[i] == host) {
            INFO("returning true because the host is on the candidate list.");
            return true;
        }
    }
    INFO("returning false because the host is not on the candidate list.");
    return false;
}

static void
srpl_queue_srp_client_update(srpl_connection_t *srpl_connection, adv_host_t *host)
{
    srpl_srp_client_queue_entry_t *new_entry, **qp;
    // Find the end of the queue
    for (qp = &srpl_connection->client_update_queue; *qp; qp = &(*qp)->next) {
        srpl_srp_client_queue_entry_t *entry = *qp;
        // No need to re-queue if we're already on the queue
        if (!entry->sent && entry->host == host) {
            INFO("host " PRI_S_SRP " is already on the update queue for connection " PRI_S_SRP,
                 host->name, srpl_connection->name);
            return;
        }
    }
    new_entry = calloc(1, sizeof(*new_entry));
    if (new_entry == NULL) {
        ERROR(PRI_S_SRP ": no memory to queue SRP client update.", srpl_connection->name);
        return;
    }
    INFO("adding host " PRI_S_SRP " to the update queue for connection " PRI_S_SRP, host->name, srpl_connection->name);
    new_entry->host = host;
    srp_adv_host_retain(new_entry->host);
    *qp = new_entry;
}

// Client update events are interesting in two cases. First, we might have received a host update for a
// host that was in contention when the update was received; in this case, we want to now apply the update,
// assuming that the contention is no longer present (it's possible that there are multiple sources of
// contention).
//
// The second case is where a client update succeeded; in this case we want to send that update to all of
// the remotes.
//
// We do not receive this event when an update that was triggered by an SRP Replication update; in that
// case we get an "apply finished" event instead of a "client update finished" event.
static void
srpl_srp_client_update_send_event_to_connection(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    if (event->content.client_result.rcode == dns_rcode_noerror) {
        adv_host_t *host = event->content.client_result.host;
        if (!srpl_reschedule_candidate(srpl_connection, host)) {
            srpl_queue_srp_client_update(srpl_connection, host);
        }
    }
    srpl_event_deliver(srpl_connection, event);
}

static void
srpl_deferred_srp_client_update_finished_event_deliver(void *context)
{
    srpl_event_t *event = context;
    srp_server_t *server_state = event->content.client_result.host->server_state;
    if (server_state == NULL) {
        FAULT("server state is NULL."); // this can't currently happen, because we just finished updating the host.
        goto out;
    }
    for (srpl_domain_t *domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            if (instance->connection != NULL) {
                srpl_srp_client_update_send_event_to_connection(instance->connection, event);
            }
        }
    }
    for (srpl_instance_t *instance = server_state->unmatched_instances; instance != NULL; instance = instance->next) {
        if (instance->connection != NULL) {
            srpl_srp_client_update_send_event_to_connection(instance->connection, event);
        }
    }
out:
    srp_adv_host_release(event->content.client_result.host);
    free(event);
}

// When an SRP client update finished, we need to deliver an event to all connections indicating that this has
// occurred. This event must be delivered from the main run loop, to avoid starting an update before advertise_finish
// has completed its work.
void
srpl_srp_client_update_finished_event_send(adv_host_t *host, int rcode)
{
    srpl_event_t *event;
    event = malloc(sizeof(*event));
    if (event == NULL) {
        FAULT(PRI_S_SRP ": unable to allocate memory to defer event", host->name);
        return;
    }
    srpl_event_initialize(event, srpl_event_srp_client_update_finished);
    srpl_event_content_type_set(event, srpl_event_content_type_client_result);
    event->content.client_result.host = host;
    srp_adv_host_retain(event->content.client_result.host);
    event->content.client_result.rcode = rcode;
    ioloop_run_async(srpl_deferred_srp_client_update_finished_event_deliver, event);
}

typedef struct {
    srpl_state_t state;
    char *name;
    srpl_action_t action;
} srpl_connection_state_t;

#define STATE_NAME_DECL(name) srpl_state_##name, #name
static srpl_connection_state_t srpl_connection_states[] = {
    { STATE_NAME_DECL(invalid),                              NULL },
    { STATE_NAME_DECL(disconnected),                         srpl_disconnected_action },
    { STATE_NAME_DECL(next_address_get),                     srpl_next_address_get_action },
    { STATE_NAME_DECL(connect),                              srpl_connect_action },
    { STATE_NAME_DECL(idle),                                 srpl_idle_action },
    { STATE_NAME_DECL(reconnect_wait),                       srpl_reconnect_wait_action },
    { STATE_NAME_DECL(retry_delay_send),                     srpl_retry_delay_send_action },
    { STATE_NAME_DECL(disconnect),                           srpl_disconnect_action },
    { STATE_NAME_DECL(disconnect_wait),                      srpl_disconnect_wait_action },
// If a disconnected state is added here, please fix SRPL_CONNECTION_IS_CONNECTED above.
// connecting is counted as a connected state because we have a connection, even if it is not
// actually connected, and we'll get a disconnect if it fails, so we aren't stuck.
    { STATE_NAME_DECL(connecting),                           srpl_connecting_action },
    { STATE_NAME_DECL(session_send),                         srpl_session_send_action },
    { STATE_NAME_DECL(session_response_wait),                srpl_session_response_wait_action },
    { STATE_NAME_DECL(session_evaluate),                     srpl_session_evaluate_action },
    { STATE_NAME_DECL(sync_wait),                            srpl_sync_wait_action },
    // Here we are the endpoint that has send the "send candidates message" and we are cycling through the candidates
    // we receive until we get a "send candidates" reply.

    { STATE_NAME_DECL(send_candidates_send),                 srpl_send_candidates_send_action },
    { STATE_NAME_DECL(send_candidates_wait),                 srpl_send_candidates_wait_action },

    // Got a "candidate" message, need to check it and send the right reply.
    { STATE_NAME_DECL(candidate_check),                      srpl_candidate_check_action },

    // At this point we've send a candidate reply, so we're waiting for a host message. It's possible that the host
    // went away in the interim, in which case we will get a "candidate" message or a "send candidate" reply.

    { STATE_NAME_DECL(candidate_host_wait),                  srpl_candidate_host_wait_action },
    { STATE_NAME_DECL(candidate_host_prepare),               srpl_candidate_host_prepare_action },
    { STATE_NAME_DECL(candidate_host_contention_wait),       srpl_candidate_host_contention_wait_action },
    { STATE_NAME_DECL(candidate_host_re_evaluate),           srpl_candidate_host_re_evaluate_action },

    // Here we've gotten the host message (the SRP message), and need to apply it and send a response
    { STATE_NAME_DECL(candidate_host_apply),                 srpl_candidate_host_apply_action },
    { STATE_NAME_DECL(candidate_host_apply_wait),            srpl_candidate_host_apply_wait_action },

    // We've received a "send candidates" message. Make a list of candidates to send, and then start sending them.
    { STATE_NAME_DECL(send_candidates_received),             srpl_send_candidates_received_action },
    // See if there are any candidates left to send; if not, go to send_candidates_response_send
    { STATE_NAME_DECL(send_candidates_remaining_check),      srpl_candidates_remaining_check_action },
    // Send a "candidate" message for the next candidate
    { STATE_NAME_DECL(next_candidate_send),                  srpl_next_candidate_send_action },
    // Wait for a response to the "candidate" message
    { STATE_NAME_DECL(next_candidate_send_wait),             srpl_next_candidate_send_wait_action },
    // The candidate requested, so send its host info
    { STATE_NAME_DECL(candidate_host_send),                  srpl_candidate_host_send_action },
    // We're waiting for the remote to acknowledge the host update
    { STATE_NAME_DECL(candidate_host_response_wait),         srpl_candidate_host_response_wait_action },

    // When we've run out of candidates to send, we send the candidates response.
    { STATE_NAME_DECL(send_candidates_response_send),        srpl_send_candidates_response_send_action },

    // This is the quiescent state for servers and clients after session establishment database sync.
    // Waiting for updates received locally, or updates sent by remote
    { STATE_NAME_DECL(ready),                                srpl_ready_action },
    // An update was received locally
    { STATE_NAME_DECL(srp_client_update_send),               srpl_srp_client_update_send_action },
    // We've gotten an ack
    { STATE_NAME_DECL(srp_client_ack_evaluate),              srpl_srp_client_ack_evaluate_action },
    // See if we have an update from the remote that we stashed because it arrived while we were sending one
    { STATE_NAME_DECL(stashed_host_check),                   srpl_stashed_host_check_action },
    // Apply a stashed update (which may have been stashed in the ready state or the client_update_ack_wait state
    { STATE_NAME_DECL(stashed_host_apply),                   srpl_stashed_host_apply_action },
    // A stashed update finished; check the results
    { STATE_NAME_DECL(stashed_host_finished),                srpl_stashed_host_finished_action },

    // Initial startup state for server
    { STATE_NAME_DECL(session_message_wait),                 srpl_session_message_wait_action },
    // Send a response once we've figured out that we're going to continue
    { STATE_NAME_DECL(session_response_send),                srpl_session_response_send },
    // Wait for a "send candidates" message.
    { STATE_NAME_DECL(send_candidates_message_wait),         srpl_send_candidates_message_wait_action },

#ifdef SRP_TEST_SERVER
    { STATE_NAME_DECL(test_event_intercept),                 srpl_test_event_intercept_action },
#endif
};
#define SRPL_NUM_CONNECTION_STATES (sizeof(srpl_connection_states) / sizeof(srpl_connection_state_t))

static srpl_connection_state_t *
srpl_state_get(srpl_state_t state)
{
    static bool once = false;
    if (!once) {
        for (unsigned i = 0; i < SRPL_NUM_CONNECTION_STATES; i++) {
            if (srpl_connection_states[i].state != (srpl_state_t)i) {
                ERROR("srpl connection state %d doesn't match " PUB_S_SRP, i, srpl_connection_states[i].name);
                STATE_DEBUGGING_ABORT();
                return NULL;
            }
        }
        once = true;
    }
    if (state < 0 || state >= SRPL_NUM_CONNECTION_STATES) {
        STATE_DEBUGGING_ABORT();
        return NULL;
    }
    return &srpl_connection_states[state];
}

void
srpl_connection_next_state(srpl_connection_t *srpl_connection, srpl_state_t state)
{
    srpl_state_t next_state = state;

    do {
        srpl_connection_state_t *new_state = srpl_state_get(next_state);

        if (new_state == NULL) {
            ERROR(PRI_S_SRP " next state is invalid: %d", srpl_connection->name, next_state);
            STATE_DEBUGGING_ABORT();
            return;
        }
        srpl_connection->state = next_state;
        srpl_connection->state_name = new_state->name;
        srpl_connection->state_start_time = srp_time();
        srpl_action_t action = new_state->action;
        if (action != NULL) {
            next_state = action(srpl_connection, NULL);
        }
    } while (next_state != srpl_state_invalid);
}

//
// Event functions
//

typedef struct {
    srpl_event_type_t event_type;
    char *name;
} srpl_event_configuration_t;

#define EVENT_NAME_DECL(name) { srpl_event_##name, #name }

srpl_event_configuration_t srpl_event_configurations[] = {
    EVENT_NAME_DECL(invalid),
    EVENT_NAME_DECL(address_add),
    EVENT_NAME_DECL(address_remove),
    EVENT_NAME_DECL(server_disconnect),
    EVENT_NAME_DECL(reconnect_timer_expiry),
    EVENT_NAME_DECL(disconnected),
    EVENT_NAME_DECL(connected),
    EVENT_NAME_DECL(session_response_received),
    EVENT_NAME_DECL(send_candidates_response_received),
    EVENT_NAME_DECL(candidate_received),
    EVENT_NAME_DECL(host_message_received),
    EVENT_NAME_DECL(srp_client_update_finished),
    EVENT_NAME_DECL(advertise_finished),
    EVENT_NAME_DECL(candidate_response_received),
    EVENT_NAME_DECL(host_response_received),
    EVENT_NAME_DECL(session_message_received),
    EVENT_NAME_DECL(send_candidates_message_received),
    EVENT_NAME_DECL(do_sync),
};
#define SRPL_NUM_EVENT_TYPES (sizeof(srpl_event_configurations) / sizeof(srpl_event_configuration_t))

static srpl_event_configuration_t *
srpl_event_configuration_get(srpl_event_type_t event)
{
    static bool once = false;
    if (!once) {
        for (unsigned i = 0; i < SRPL_NUM_EVENT_TYPES; i++) {
            if (srpl_event_configurations[i].event_type != (srpl_event_type_t)i) {
                ERROR("srpl connection event %d doesn't match " PUB_S_SRP, i, srpl_event_configurations[i].name);
                STATE_DEBUGGING_ABORT();
                return NULL;
            }
        }
        once = true;
    }
    if (event < 0 || event >= SRPL_NUM_EVENT_TYPES) {
        STATE_DEBUGGING_ABORT();
        return NULL;
    }
    return &srpl_event_configurations[event];
}

static const char *
srpl_state_name(srpl_state_t state)
{
    for (unsigned i = 0; i < SRPL_NUM_CONNECTION_STATES; i++) {
        if (srpl_connection_states[i].state == state) {
            return srpl_connection_states[i].name;
        }
    }
    return "unknown state";
}

void
srpl_dump_connection_states(srp_server_t *server_state)
{
    srpl_domain_t *domain;
    srpl_instance_t *instance;
    srpl_connection_t *connection;
    uint32_t days, hours, minutes, seconds;

    for (domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
        INFO("srpl connections in domain " PRI_S_SRP ":", domain->name);
        for (instance = domain->instances; instance != NULL; instance = instance->next) {
            connection = instance->connection;
            if (connection != NULL) {
                seconds = (uint32_t)(srp_time() - connection->state_start_time);
                days = seconds / SECONDS_IN_DAY;
                seconds -= days * SECONDS_IN_DAY;
                hours = seconds / SECONDS_IN_HOUR;
                seconds -= hours * SECONDS_IN_HOUR;
                minutes = seconds / SECONDS_IN_MINUTE;
                seconds -= minutes * SECONDS_IN_MINUTE;
                INFO(PUB_S_SRP "connected to srpl instance " PRI_S_SRP " - connection " PRI_S_SRP
                     ", in state " PUB_S_SRP " for %" PRIu32 " days %" PRIu32 " hours %" PRIu32 " minutes %" PRIu32 " seconds",
                     connection->state > srpl_state_connecting ? "" : "not yet ",
                     instance->instance_name, connection->name, srpl_state_name(connection->state),
                     days, hours, minutes, seconds);
            } else {
                INFO("no connection to srpl instance " PRI_S_SRP PUB_S_SRP, instance->instance_name,
                     instance->is_me ? ", is_me" : "");
            }
        }
    }

    for (instance = server_state->unmatched_instances; instance != NULL; instance = instance->next) {
        connection = instance->connection;
        if (connection != NULL) {
            seconds = (uint32_t)(srp_time() - connection->state_start_time);
            days = seconds / SECONDS_IN_DAY;
            seconds -= days * SECONDS_IN_DAY;
            hours = seconds / SECONDS_IN_HOUR;
            seconds -= hours * SECONDS_IN_HOUR;
            minutes = seconds / SECONDS_IN_MINUTE;
            seconds -= minutes * SECONDS_IN_MINUTE;
            INFO(PUB_S_SRP "connected to unidentified instance " PRI_S_SRP " - connection " PRI_S_SRP
                 ", in state " PUB_S_SRP " for %" PRIu32 " days %" PRIu32 " hours %" PRIu32 " minutes %" PRIu32 " seconds",
                 connection->state > srpl_state_connecting ? "" : "not yet ",
                 instance->instance_name, connection->name, srpl_state_name(connection->state),
                 days, hours, minutes, seconds);
        }
    }
}

void
srpl_change_server_priority(srp_server_t *server_state, uint32_t new)
{
    if (server_state != NULL && new != server_state->priority) {
        server_state->priority = new;
        for (srpl_domain_t *domain = server_state->srpl_domains; domain != NULL; domain = domain->next) {
            // priority changed. re-advertise.
            srpl_domain_advertise(domain);
        }
    }
}

static void
srpl_event_initialize(srpl_event_t *event, srpl_event_type_t event_type)
{
    memset(event, 0, sizeof(*event));
    srpl_event_configuration_t *event_config = srpl_event_configuration_get(event_type);
    if (event_config == NULL) {
        ERROR("invalid event type %d", event_type);
        STATE_DEBUGGING_ABORT();
        return;
    }
    event->event_type = event_type;
    event->name = event_config->name;
}

static void
srpl_event_deliver(srpl_connection_t *srpl_connection, srpl_event_t *event)
{
    srpl_connection_state_t *state = srpl_state_get(srpl_connection->state);
    if (state == NULL) {
        ERROR(PRI_S_SRP ": event " PUB_S_SRP " received in invalid state %d",
              srpl_connection->name, event->name, srpl_connection->state);
        STATE_DEBUGGING_ABORT();
        return;
    }
    if (state->action == NULL) {
        FAULT(PRI_S_SRP": event " PUB_S_SRP " received in state " PUB_S_SRP " with NULL action",
              srpl_connection->name, event->name, state->name);
        return;
    }
    srpl_state_t next_state = state->action(srpl_connection, event);
    if (next_state != srpl_state_invalid) {
        srpl_connection_next_state(srpl_connection, next_state);
    }
}

static void
srpl_re_register(void *context)
{
    INFO("re-registering SRPL service");
    srpl_domain_advertise(context);
}

static void
srpl_register_completion(DNSServiceRef UNUSED sdref, DNSServiceFlags UNUSED flags, DNSServiceErrorType error_code,
                         const char *name, const char *regtype, const char *domain, void *context)
{
    srpl_domain_t *srpl_domain = context;

    if (error_code != kDNSServiceErr_NoError) {
        ERROR("unable to advertise _srpl-tls._tcp service: %d", error_code);
        if (srpl_domain->srpl_register_wakeup == NULL) {
            srpl_domain->srpl_register_wakeup = ioloop_wakeup_create();
        }
        if (srpl_domain->srpl_register_wakeup != NULL) {
            // Try registering again in one second.
            ioloop_add_wake_event(srpl_domain->srpl_register_wakeup, srpl_domain, srpl_re_register, srpl_domain_context_release, 1000);
            RETAIN_HERE(srpl_domain, srpl_domain);
        }
        return;
    }
    INFO("registered SRP Replication instance name " PRI_S_SRP "." PUB_S_SRP "." PRI_S_SRP, name, regtype, domain);
}

static void
srpl_domain_advertise(srpl_domain_t *domain)
{
    DNSServiceRef sdref = NULL;
    TXTRecordRef txt_record;
    char partner_id_buf[INT64_HEX_STRING_MAX];
    char dataset_id_buf[INT64_HEX_STRING_MAX];
    char xpanid_buf[INT64_HEX_STRING_MAX];
    char priority_buf[INT64_HEX_STRING_MAX];
    srp_server_t *server_state = domain->server_state;

    if (domain->srpl_opstate != SRPL_OPSTATE_ROUTINE) {
        INFO(PUB_S_SRP ": not in routine state", domain->name);
        return;
    }

    TXTRecordCreate(&txt_record, 0, NULL);

    int err = TXTRecordSetValue(&txt_record, "dn", strlen(server_state->current_thread_domain_name), domain->name);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to set domain in TXT record for _srpl-tls._tcp to " PRI_S_SRP, domain->name);
        goto exit;
    }

    snprintf(partner_id_buf, sizeof(partner_id_buf), "%" PRIx64, domain->partner_id);
    err = TXTRecordSetValue(&txt_record, "pid", strlen(partner_id_buf), partner_id_buf);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to set partner-id in TXT record for _srpl-tls._tcp to " PUB_S_SRP, partner_id_buf);
        goto exit;
    }

    snprintf(dataset_id_buf, sizeof(dataset_id_buf), "%" PRIx64, domain->dataset_id);
    err = TXTRecordSetValue(&txt_record, "did", strlen(dataset_id_buf), dataset_id_buf);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to set dataset-id in TXT record for _srpl-tls._tcp to " PUB_S_SRP, dataset_id_buf);
        goto exit;
    }

    snprintf(xpanid_buf, sizeof(xpanid_buf), "%" PRIx64, domain->server_state->xpanid);
    err = TXTRecordSetValue(&txt_record, "xpanid", strlen(xpanid_buf), xpanid_buf);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to set xpanid in TXT record for _srpl-tls._tcp to " PUB_S_SRP, dataset_id_buf);
        goto exit;
    }

    snprintf(priority_buf, sizeof(priority_buf), "%" PRIx32, domain->server_state->priority);
    err = TXTRecordSetValue(&txt_record, "priority", strlen(priority_buf), priority_buf);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to set priority in TXT record for _srpl-tls._tcp to " PUB_S_SRP, priority_buf);
        goto exit;
    }

    // If there is already a registration, get rid of it
    if (domain->srpl_advertise_txn != NULL) {
        ioloop_dnssd_txn_cancel(domain->srpl_advertise_txn);
        ioloop_dnssd_txn_release(domain->srpl_advertise_txn);
        domain->srpl_advertise_txn = NULL;
    }

    err = DNSServiceRegister(&sdref, kDNSServiceFlagsUnique,
                             kDNSServiceInterfaceIndexAny, NULL, "_srpl-tls._tcp", NULL,
                             NULL, htons(853), TXTRecordGetLength(&txt_record), TXTRecordGetBytesPtr(&txt_record),
                             srpl_register_completion, domain);
    if (err != kDNSServiceErr_NoError) {
        ERROR("unable to advertise _srpl-tls._tcp service");
        goto exit;
    }
    domain->srpl_advertise_txn = ioloop_dnssd_txn_add(sdref, NULL, NULL, NULL);
    if (domain->srpl_advertise_txn == NULL) {
        ERROR("unable to set up a dnssd_txn_t for _srpl-tls._tcp advertisement.");
        goto exit;
    }
    sdref = NULL; // srpl_advertise_txn holds the reference.
    INFO(PUB_S_SRP ": successfully advertised", domain->name);

exit:
    if (sdref != NULL) {
        DNSServiceRefDeallocate(sdref);
    }
    TXTRecordDeallocate(&txt_record);
    return;
}

static void
srpl_thread_network_name_callback(void *NULLABLE context, const char *NULLABLE thread_network_name, cti_status_t status)
{
    size_t thread_domain_size;
    char domain_buf[kDNSServiceMaxDomainName];
    char *new_thread_domain_name;
    srp_server_t *server_state = context;

    if (thread_network_name == NULL || status != kCTIStatus_NoError) {
        ERROR("unable to get thread network name.");
        return;
    }
    thread_domain_size = snprintf(domain_buf, sizeof(domain_buf),
                                  "%s.%s", thread_network_name, SRP_THREAD_DOMAIN);
    if (thread_domain_size < 0 || thread_domain_size >= sizeof (domain_buf) ||
        (new_thread_domain_name = strdup(domain_buf)) == NULL)
    {
        ERROR("no memory for new thread network name: " PRI_S_SRP, thread_network_name);
        return;
    }

    if (server_state->current_thread_domain_name != NULL) {
        srpl_domain_rename(server_state->current_thread_domain_name, new_thread_domain_name);
    }
    srpl_domain_add(server_state, new_thread_domain_name);
    free(server_state->current_thread_domain_name);
    server_state->current_thread_domain_name = new_thread_domain_name;
}

// If the partner does not discover any other SRPL partners to synchronize with,
// or it has synchronized with all the partners discovered so far, it transitions
// out of the "startup" state to the "routine operation" state.
static void
srpl_partner_discovery_timeout(void *context)
{
    srpl_domain_t *domain = context;

    INFO("partner discovery timeout.");

    domain->partner_discovery_pending = false;
    if (domain->partner_discovery_timeout != NULL) {
        ioloop_cancel_wake_event(domain->partner_discovery_timeout);
        ioloop_wakeup_release(domain->partner_discovery_timeout);
        domain->partner_discovery_timeout = NULL;
    }
    srpl_maybe_sync_or_transition(domain);
}

// count how many partners are advertising the proposed dataset id
static int
srpl_active_winning_partners(srpl_domain_t *domain, int *rank)
{
    int num_winners = 0;
    int my_rank = 0;
    if (domain->have_dataset_id) {
        uint64_t proposed_dataset_id = domain->dataset_id;
        uint32_t my_priority = domain->server_state->priority;
        // winning partners are those with higher priority, or same priority but
        // lower partner id, and advertising the proposed dataset id.
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            if (instance->connection != NULL &&
                instance->connection->state > srpl_state_session_evaluate &&
                instance->dataset_id == proposed_dataset_id &&
                instance->priority >= my_priority)
            {
                num_winners++;
                if (instance->priority > my_priority || (instance->priority == my_priority &&
                    instance->partner_id < domain->partner_id))
                {
                    my_rank++;
                }
            }
        }
    }
    *rank = my_rank;
    return num_winners;
}

static void
srpl_sync_with_instance(srpl_instance_t *instance)
{
    INFO("sync with " PRI_S_SRP " with dataset_id %" PRIx64, instance->instance_name, instance->dataset_id);
    if (instance->discovered_in_window) {
        instance->sync_to_join = true;
    }
    instance->sync_fail = false;
    srpl_event_t event;
    srpl_event_initialize(&event, srpl_event_do_sync);
    srpl_event_deliver(instance->connection, &event);
}

// We check how many active srp servers we discovered. If less than 5, we first
// check if we are ready to enter the routine state. If there are still srp servers
// that we haven't started wo sync with, we do so.
static void
srpl_maybe_sync_or_transition(srpl_domain_t *domain)
{
    int num_winners = 0;
    int rank = 0;
    // if we haven't committed a dataset id, here we check if we
    // should propose a new one. we propose a new dataset id if
    // sync with the instances of the proposed dataset id all fail, or
    // there's no partner advertising the proposed dataset id.
    if (!domain->dataset_id_committed) {
        srpl_maybe_propose_new_dataset_id(domain);
    }
    num_winners = srpl_active_winning_partners(domain, &rank);

    if (num_winners < MAX_ANYCAST_NUM) {
        INFO("%d other srp servers are advertising.", num_winners);
        for (srpl_instance_t *instance = domain->instances; instance != NULL; instance = instance->next) {
            // we sync with the instances with the same dataset id
            if (instance->connection != NULL &&
                instance->connection->state == srpl_state_sync_wait &&
                srpl_dataset_id_compare(instance->dataset_id, domain->dataset_id) >= 0)
            {
                srpl_sync_with_instance(instance);
            }
        }
        if (domain->srpl_opstate != SRPL_OPSTATE_ROUTINE &&
            srpl_can_transition_to_routine_state(domain))
        {
            srpl_transition_to_routine_state(domain);
        }
    } else {
        // MAX_ANYCAST_NUM or more better qualified servers are advertising. if
        // we are in the routine state and advertising, we should withdraw and
        // reenter the startup state.
        INFO("%d other srp servers are advertising, rank = %d.", num_winners, rank);
        if (domain->srpl_opstate == SRPL_OPSTATE_ROUTINE &&
            rank >= MAX_ANYCAST_NUM)
        {
            INFO("transition to startup state.");
            srpl_transition_to_startup_state(domain);
        }
    }
}

static void
srpl_transition_to_startup_state(srpl_domain_t *domain)
{
    srp_server_t *server_state = domain->server_state;

    // stop advertising the domain.
    srpl_stop_domain_advertisement(domain);
    // move to "startup" state.
    domain->srpl_opstate = SRPL_OPSTATE_STARTUP;

    if (server_state == NULL) {
        ERROR("server state is NULL.");
        return;
    }

#if STUB_ROUTER
    route_state_t *route_state = NULL;
    route_state = server_state->route_state;
    if (route_state != NULL && !strcmp(domain->name, server_state->current_thread_domain_name)) {
        route_state->partition_can_advertise_anycast_service = false;
        partition_stop_advertising_anycast_service(route_state, route_state->thread_sequence_number);
    }
#endif
    if (domain->partner_discovery_timeout == NULL) {
        domain->partner_discovery_timeout = ioloop_wakeup_create();
    }
    if (domain->partner_discovery_timeout) {
        ioloop_add_wake_event(domain->partner_discovery_timeout, domain,
                              srpl_partner_discovery_timeout, srpl_domain_context_release,
                              MIN_PARTNER_DISCOVERY_INTERVAL +
                              srp_random16() % PARTNER_DISCOVERY_INTERVAL_RANGE);
        RETAIN_HERE(domain, srpl_domain);
    } else {
        ERROR("unable to add wakeup event for partner discovery.");
        return;
    }
    domain->partner_discovery_pending = true;
}

void
srpl_startup(srp_server_t *server_state)
{
    cti_get_thread_network_name(server_state, srpl_thread_network_name_callback, NULL);
}
#endif // SRP_FEATURE_REPLICATION

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
