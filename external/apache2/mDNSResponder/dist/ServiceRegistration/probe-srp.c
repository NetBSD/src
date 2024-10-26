/* probe-srp.c
 *
 * Copyright (c) 2023 Apple Inc. All rights reserved.
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

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"

#include "srp-mdns-proxy.h"
#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"
#include "probe-srp.h"

struct probe_state {
    int ref_count;
    comm_t *connection;
    wakeup_t *wakeup;
    thread_service_t *service;
    void *context;
    void (*callback)(thread_service_t *service, void *context, bool succeeded);
    void (*context_release)(void *context);
    route_state_t *route_state;
    dns_wire_t question;
    int num_retransmissions, retransmission_delay;
    uint16_t question_length;
};

static void
probe_state_finalize(probe_state_t *probe_state)
{
    if (probe_state->wakeup != NULL) {
        ioloop_wakeup_release(probe_state->wakeup);
    }
    if (probe_state->service != NULL) {
        thread_service_release(probe_state->service);
        probe_state->service = NULL;
    }
    if (probe_state->connection != NULL) {
        ioloop_comm_release(probe_state->connection);
        probe_state->connection = NULL;
    }
    if (probe_state->context_release) {
        probe_state->context_release(probe_state->context);
    }
    free(probe_state);
}

void
probe_srp_service_probe_cancel(thread_service_t *service)
{
    probe_state_t *probe_state = service->probe_state;
    probe_state->service = NULL;
    service->probe_state = NULL;

    if (probe_state->context_release != NULL) {
        probe_state->context_release(probe_state->context);
    }
    probe_state->context = NULL;

    thread_service_release(service); // The probe state's reference to the service
    if (probe_state->wakeup != NULL) {
        ioloop_cancel_wake_event(probe_state->wakeup);
    }

    if (probe_state->connection != NULL) {
        ioloop_comm_cancel(probe_state->connection); // Cancel the connection (should result in the state being released)
        ioloop_comm_release(probe_state->connection);
        probe_state->connection = NULL;
    }
    RELEASE_HERE(probe_state, probe_state); // The thread_service_t's reference to the probe state
}

static void
probe_srp_done(void *context, bool succeeded)
{
    probe_state_t *probe_state = context;
    struct in6_addr *address;
    int port;
    thread_service_t *service = probe_state->service;
    // Note: we should still have references both to probe_state and to service here because they each held references
    // to each other.
    probe_state->service = NULL;
    service->probe_state = NULL;
    if (service->service_type == anycast_service) {
        address = &service->u.anycast.address;
        port = 53;
    } else {
        address = &service->u.unicast.address;
        // If the anycast service is present, we can use port 53, which we need to prefer because pre-2024 Apple BRs
        // will not answer DNS queries on the SRP service port.
        if (service->u.unicast.anycast_also_present) {
            port = 53;
        } else {
            port = (service->u.unicast.port[0] << 8) | service->u.unicast.port[1];
        }
    }
    SEGMENTED_IPv6_ADDR_GEN_SRP(address->s6_addr, addr_buf);
    if (!succeeded) {
        INFO("service " PRI_SEGMENTED_IPv6_ADDR_SRP " not responding on port %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(address->s6_addr, addr_buf), port);
        service->checking = false;
        service->ignore = true;  // Don't consider this service when deciding what to advertise
        service->responding = false;
    } else {
        INFO("service " PRI_SEGMENTED_IPv6_ADDR_SRP " responded on port %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(address->s6_addr, addr_buf), port);
        service->checking = false;
        service->ignore = false;
        service->checked = true;
    }
    service->responding = true;

    if (probe_state->callback != NULL) {
        probe_state->callback(probe_state->service, probe_state->context, succeeded);
        probe_state->callback = NULL;
    }
    if (probe_state->context_release != NULL) {
        probe_state->context_release(probe_state->context);
    }
    probe_state->context = NULL;

    thread_service_release(service); // The probe state's reference to the service
    if (probe_state->wakeup != NULL) {
        ioloop_cancel_wake_event(probe_state->wakeup);
    }
    RELEASE_HERE(probe_state, probe_state); // The thread_service_t's reference to the probe state
}

static void
probe_srp_datagram(comm_t *connection, message_t *message, void *context)
{
#ifdef PROBE_SRP_TCP
    (void)message;
    // We should never get a datagram
    ERROR("got a datagram on %p", context);
#else
    int rcode = dns_rcode_get(&message->wire);
    probe_state_t *probe_state = context;
    if (connection->connection != NULL) {
        INFO("datagram from " PRI_S_SRP " on port %d xid %x (question xid %x) rcode %d", connection->name,
             ntohs(probe_state->connection->address.sin6.sin6_port), message->wire.id, probe_state->question.id, rcode);
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(&probe_state->connection->address.sin6.sin6_addr, addr_buf);
        INFO("datagram from " PRI_SEGMENTED_IPv6_ADDR_SRP " on port %d xid %x (question xid %x) rcode %d",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(&probe_state->connection->address.sin6.sin6_addr, addr_buf),
             ntohs(probe_state->connection->address.sin6.sin6_port), message->wire.id, probe_state->question.id, rcode);
    }
    if (message->wire.id != probe_state->question.id) {
        return; // not a response to the question we asked
    }
    dns_message_t *dns_message = NULL;
    if (!dns_wire_parse(&dns_message, &message->wire, message->length, false)) {
        // Not a valid response, who knows what happened?
        return;
    }
    dns_message_free(dns_message);
    // If we get a servfail, treat it like a dropped packet, since that might just mean that the remote end is
    // temporarily busy.
    if (rcode == dns_rcode_servfail) {
        return;
    }
    probe_srp_done(context, rcode == dns_rcode_noerror);
    ioloop_comm_cancel(probe_state->connection); // Cancel the connection (should result in the state being released)
    if (probe_state->wakeup != NULL) {
        ioloop_cancel_wake_event(probe_state->wakeup);
    }
#endif
}

static void
probe_srp_probe_state_context_release(void *context)
{
    probe_state_t *probe_state = context;
    RELEASE_HERE(probe_state, probe_state);
}

static void
probe_srp_disconnected(comm_t *UNUSED connection, void *context, int UNUSED error)
{
    probe_state_t *probe_state = context;

    // We can get here either because the connection object got canceled or because we made a TCP connection that
    // failed to connect. If we haven't signaled "done" yet, probe_state->service will be non-NULL.
    if (probe_state->service != NULL) {
        probe_srp_done(context, false);
    }
    // Once we've gotten the connection disconnect event, we should not get any more callbacks from the connection
    // object.
    if (probe_state->connection != NULL) {
        ioloop_comm_release(probe_state->connection);
        probe_state->connection = NULL;
    }
}

#ifndef PROBE_SRP_TCP
static void probe_srp_schedule_retransmission(probe_state_t *probe_state);

static void
probe_srp_retransmit(void *context)
{
    probe_state_t *probe_state = context;

    // Only retransmit three times.
    probe_state->num_retransmissions++;
    INFO("num_retransmissions = %d, time = %lg", probe_state->num_retransmissions, probe_state->retransmission_delay / 1000.0);
    if (probe_state->num_retransmissions > 3) {
        probe_srp_done(context, false); // fail
        ioloop_comm_cancel(probe_state->connection);
   } else {
        // Schedule a retransmission with exponential backoff
        probe_srp_schedule_retransmission(probe_state);

        // Send the question
        struct iovec iov;
        iov.iov_len = probe_state->question_length;
        iov.iov_base = &probe_state->question;
        ioloop_send_message(probe_state->connection, NULL, &iov, 1);
    }
}

static void
probe_srp_context_release(void *context)
{
    probe_state_t *probe_state = context;
    RELEASE_HERE(probe_state, probe_state);
}

static void
probe_srp_schedule_retransmission(probe_state_t *probe_state)
{
    if (probe_state->wakeup == NULL) {
        probe_state->wakeup = ioloop_wakeup_create();
        if (probe_state->wakeup == NULL) {
            ERROR("can't allocate probe state wakeup");
            probe_srp_done(probe_state, false);
            ioloop_comm_cancel(probe_state->connection);
            return;
        }
    }
    int next_time = probe_state->retransmission_delay + srp_random16() % probe_state->retransmission_delay;
    probe_state->retransmission_delay *= 2;
    ioloop_add_wake_event(probe_state->wakeup, probe_state, probe_srp_retransmit, probe_srp_context_release, next_time);
    RETAIN_HERE(probe_state, probe_state); // The wakeup holds a reference to probe_state.
}
#endif // PROBE_SRP_TCP

static void
probe_srp_connected(comm_t *connection, void *context)
{
#ifdef PROBE_SRP_TCP
    probe_srp_done(context, true);
#else
    (void)connection;
    probe_state_t *probe_state = context;
    // Initialize a DNS message to send to the target
    memset(&probe_state->question, 0, DNS_HEADER_SIZE);
    dns_towire_state_t towire;
    memset(&towire, 0, sizeof(towire));
    towire.p = &probe_state->question.data[0];
    towire.lim = &probe_state->question.data[0] + DNS_DATA_SIZE;
    towire.message = &probe_state->question;

    // Set up the header.
    probe_state->question.id = srp_random16();
    probe_state->question.bitfield = 0;
    dns_qr_set(&probe_state->question, dns_qr_query);
    dns_opcode_set(&probe_state->question, dns_opcode_query);
    probe_state->question.qdcount = htons(1); // Just ask one question

    // Query SOA for default.service.arpa--if this fails, we can't use this server.
    dns_full_name_to_wire(NULL, &towire, "default.service.arpa");
    dns_u16_to_wire(&towire, dns_rrtype_soa);
    dns_u16_to_wire(&towire, dns_qclass_in);
    probe_state->question_length = (uint16_t)(towire.p - (uint8_t *)&probe_state->question);

    // We're not in a hurry; the goal is to probe.
    probe_state->retransmission_delay = 1000; // milliseconds
    probe_state->num_retransmissions = 0;

    // Schedule the first send
    probe_srp_schedule_retransmission(probe_state);
#endif // PROBE_SRP_TCP
}

static probe_state_t *
probe_srp_create(addr_t *address, thread_service_t *service, void *context,
                   void (*callback)(thread_service_t *service, void *context, bool succeeded),
                   void (*context_release)(void *context))
{
    probe_state_t *ret = NULL, *probe_state = calloc(1, sizeof(*probe_state));
    if (probe_state == NULL) {
        INFO("failed to create probe state");
        goto out;
    }
    RETAIN_HERE(probe_state, probe_state); // Retain for the caller
    // tls   stream stable  opportunistic
    probe_state->connection = ioloop_connection_create(address, false, false, false, false,
                                                       probe_srp_datagram, probe_srp_connected, probe_srp_disconnected,
                                                       probe_srp_probe_state_context_release, probe_state);
    if (probe_state->connection == NULL) {
        INFO("failed to create connection");
        goto out;
    }
    RETAIN_HERE(probe_state, probe_state); // for connection
    SEGMENTED_IPv6_ADDR_GEN_SRP(&address->sin6.sin6_addr, addr_buf);
    INFO("probing service " PRI_SEGMENTED_IPv6_ADDR_SRP " on port %d",
         SEGMENTED_IPv6_ADDR_PARAM_SRP(&address->sin6.sin6_addr, addr_buf), ntohs(address->sin6.sin6_port));
    probe_state->context = context;
    probe_state->callback = callback;
    service->last_probe_time = srp_time();

    service->probe_state = probe_state;
    RETAIN_HERE(service->probe_state, probe_state);

    probe_state->service = service;
    thread_service_retain(probe_state->service);

    // connection holds the only reference to probe_state.
    ret = probe_state;
    probe_state = NULL;
out:
    if (probe_state != NULL) {
        RELEASE_HERE(probe_state, probe_state);
    }
    if (ret == NULL && callback != NULL) {
        dispatch_async(dispatch_get_main_queue(), ^{
            callback(service, context, true); // We claim success here because this should never fail; if it does, it's our problem.
            if (context_release != NULL) {
                context_release(context);
            }
        });
    }
    return ret;
}

// If we've been asked to probe a service, go through the list.
static probe_state_t *
probe_srp_anycast_service(thread_service_t *service, void *context,
                          void (*callback)(thread_service_t *service, void *context, bool succeeded),
                          void (*context_release)(void *context))
{
    addr_t address;
    memset(&address, 0, sizeof(address));
    memcpy(&address.sin6.sin6_addr, &service->u.anycast.address, sizeof(service->u.anycast.address));
    address.sin6.sin6_port = htons(53);
    address.sa.sa_family = AF_INET6;
    return probe_srp_create(&address, service, context, callback, context_release);
}

static probe_state_t *
probe_srp_unicast_service(thread_service_t *service, void *context,
                          void (*callback)(thread_service_t *service, void *context, bool succeeded),
                          void (*context_release)(void *context)){
    if (service->checking || service->user) {
        return NULL;
    }
    addr_t address;
    memset(&address, 0, sizeof(address));
    address.sa.sa_family = AF_INET6;
    memcpy(&address.sin6.sin6_addr, &service->u.unicast.address, sizeof(address.sin6.sin6_addr));
    memcpy(&address.sin6.sin6_port, service->u.unicast.port, sizeof(address.sin6.sin6_port));
    return probe_srp_create(&address, service, context, callback, context_release);
}

void
probe_srp_service(thread_service_t *service, void *context,
                  void (*callback)(thread_service_t *service, void *context, bool succeeded),
                  void (*context_release)(void *context))
{
    probe_state_t *probe_state;
    if (service->service_type == unicast_service) {
        probe_state = probe_srp_unicast_service(service, context, callback, context_release);
    } else if (service->service_type == anycast_service){
        probe_state = probe_srp_anycast_service(service, context, callback, context_release);
    } else {
        FAULT("bogus service type in probe_srp_service: %d", service->service_type);
        if (callback != NULL) {
            dispatch_async(dispatch_get_main_queue(), ^{
                    callback(service, context, false); // False because this isn't a valid service
                    if (context_release) {
                        context_release(context);
                    }
                });
        }
        return;
    }

    // probe_srp_create returns this retained, but we don't store the pointer.
    RELEASE_HERE(probe_state, probe_state);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
