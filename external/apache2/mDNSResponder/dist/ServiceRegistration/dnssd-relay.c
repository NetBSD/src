/* dnssd-relay.c
 *
 * Copyright (c) 2019-2021 Apple Inc. All rights reserved.
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

#define __APPLE_USE_RFC_3542

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

#include "dns_sd.h"
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#define DNSMessageHeader dns_wire_t
#include "dso.h"
#include "ioloop.h"
#include "srp-tls.h"
#include "config-parse.h"

// Enumerate the list of interfaces, map them to interface indexes, give each one a name
// Have a tree of subdomains for matching

// Configuration file settings
uint16_t udp_port;
uint16_t tcp_port;
uint16_t tls_port;
#define MAX_ADDRS 10
char *listen_addrs[MAX_ADDRS];
int num_listen_addrs = 0;
char *publish_addrs[MAX_ADDRS];
int num_publish_addrs = 0;
char *tls_cacert_filename = NULL;
char *tls_cert_filename = "/etc/dnssd-relay/server.crt";
char *tls_key_filename = "/etc/dnssd-relay/server.key";

// Code

int64_t dso_transport_idle(void *context, int64_t now, int64_t next_event)
{
    return next_event;
}

void
dp_simple_response(comm_t *comm, int rcode)
{
    if (comm->send_response) {
        struct iovec iov;
        dns_wire_t response;
        memset(&response, 0, DNS_HEADER_SIZE);

        // We take the ID and the opcode from the incoming message, because if the
        // header has been mangled, we (a) wouldn't have gotten here and (b) don't
        // have any better choice anyway.
        response.id = comm->message->wire.id;
        dns_qr_set(&response, dns_qr_response);
        dns_opcode_set(&response, dns_opcode_get(&comm->message->wire));
        dns_rcode_set(&response, rcode);
        iov.iov_base = &response;
        iov.iov_len = DNS_HEADER_SIZE; // No RRs
        comm->send_response(comm, comm->message, &iov, 1);
    }
}

bool
dso_send_formerr(dso_state_t *dso, const dns_wire_t *header)
{
    comm_t *transport = dso->transport;
    (void)header;
    dp_simple_response(transport, dns_rcode_formerr);
    return true;
}

static void dso_message(comm_t *comm, const dns_wire_t *header, dso_state_t *dso)
{
    switch(dso->primary.opcode) {
    case kDSOType_DNSPushSubscribe:
        dns_push_subscription_change("DNS Push Subscribe", comm, header, dso);
        break;
    case kDSOType_DNSPushUnsubscribe:
        dns_push_subscription_change("DNS Push Unsubscribe", comm, header, dso);
        break;

    case kDSOType_DNSPushReconfirm:
        dns_push_reconfirm(comm, header, dso);
        break;

    case kDSOType_DNSPushUpdate:
        INFO("bogus push update message %d", dso->primary.opcode);
        dso_drop(dso);
        break;

    default:
        INFO("unexpected primary TLV %d", dso->primary.opcode);
        dp_simple_response(comm, dns_rcode_dsotypeni);
        break;
    }
    // XXX free the message if we didn't consume it.
}

static void dns_push_callback(void *context, const void *event_context,
                              dso_state_t *dso, dso_event_type_t eventType)
{
    const dns_wire_t *message;
    switch(eventType)
    {
    case kDSOEventType_DNSMessage:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("DNS Message (opcode=%d) received from " PRI_S_SRP, dns_opcode_get(message),
             dso->remote_name);
        break;
    case kDSOEventType_DNSResponse:
        // We shouldn't get here because we already handled any DNS messages
        message = event_context;
        INFO("DNS Response (opcode=%d) received from " PRI_S_SRP, dns_opcode_get(message),
             dso->remote_name);
        break;
    case kDSOEventType_DSOMessage:
        INFO("DSO Message (Primary TLV=%d) received from " PRI_S_SRP,
             dso->primary.opcode, dso->remote_name);
        message = event_context;
        dso_message((comm_t *)context, message, dso);
        break;
    case kDSOEventType_DSOResponse:
        INFO("DSO Response (Primary TLV=%d) received from " PRI_S_SRP,
             dso->primary.opcode, dso->remote_name);
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
        INFO("Inactivity timer went off, closing connection.");
        // XXX
        break;
    case kDSOEventType_Keepalive:
        INFO("should send a keepalive now.");
        break;
    case kDSOEventType_KeepaliveRcvd:
        INFO("keepalive received.");
        break;
    case kDSOEventType_RetryDelay:
        INFO("keepalive received.");
        break;
    }
}

void
dp_dns_query(comm_t *comm, dns_rr_t *question)
{
    int rcode;
    dnssd_query_t *query = dp_query_generate(comm, question, false, &rcode);
    const char *failnote = NULL;
    if (!query) {
        dp_simple_response(comm, rcode);
        return;
    }

    // For regular DNS queries, copy the ID, etc.
    query->response->id = comm->message->wire.id;
    query->response->bitfield = comm->message->wire.bitfield;
    dns_rcode_set(query->response, dns_rcode_noerror);

    // For DNS queries, we need to return the question.
    query->response->qdcount = htons(1);
    if (query->iface != NULL) {
        TOWIRE_CHECK("name", &query->towire, dns_name_to_wire(NULL, &query->towire, query->name));
        TOWIRE_CHECK("enclosing_domain", &query->towire,
                     dns_full_name_to_wire(&query->enclosing_domain_pointer,
                                           &query->towire, query->iface->domain));
    } else {
        TOWIRE_CHECK("full name", &query->towire, dns_full_name_to_wire(NULL, &query->towire, query->name));
    }
    TOWIRE_CHECK("TYPE", &query->towire, dns_u16_to_wire(&query->towire, question->type));    // TYPE
    TOWIRE_CHECK("CLASS", &query->towire, dns_u16_to_wire(&query->towire, question->qclass));  // CLASS
    if (failnote != NULL) {
        ERROR("dp_dns_query: failure encoding question: " PUB_S_SRP, failnote);
        goto fail;
    }

    // We should check for OPT RR, but for now assume it's there.
    query->is_edns0 = true;

    if (!dp_query_start(comm, query, &rcode, dns_query_callback)) {
    fail:
        dp_simple_response(comm, rcode);
        free(query->name);
        free(query);
        return;
    }

    // XXX make sure that finalize frees this.
    if (comm->message) {
        query->question = comm->message;
        comm->message = NULL;
    }
}

void dso_transport_finalize(comm_t *comm)
{
    dso_state_t *dso = comm->dso;
    INFO(PRI_S_SRP, dso->remote_name);
    if (comm) {
        ioloop_close(&comm->io);
    }
    free(dso);
    comm->dso = NULL;
}

void dns_evaluate(comm_t *comm)
{
    dns_rr_t question;
    unsigned offset = 0;

    // Drop incoming responses--we're a server, so we only accept queries.
    if (dns_qr_get(&comm->message->wire) == dns_qr_response) {
        return;
    }

    // If this is a DSO message, see if we have a session yet.
    switch(dns_opcode_get(&comm->message->wire)) {
    case dns_opcode_dso:
        if (!comm->tcp_stream) {
            ERROR("DSO message received on non-tcp socket " PRI_S_SRP, comm->name);
            dp_simple_response(comm, dns_rcode_notimp);
            return;
        }

        if (!comm->dso) {
            comm->dso = dso_create(true, 0, comm->name, dns_push_callback, comm, comm);
            if (!comm->dso) {
                ERROR("Unable to create a dso context for " PRI_S_SRP, comm->name);
                dp_simple_response(comm, dns_rcode_servfail);
                ioloop_close(&comm->io);
                return;
            }
            comm->dso->transport_finalize = dso_transport_finalize;
        }
        dso_message_received(comm->dso, (uint8_t *)&comm->message->wire, comm->message->length);
        break;

    case dns_opcode_query:
        // In theory this is permitted but it can't really be implemented because there's no way
        // to say "here's the answer for this, and here's why that failed.
        if (ntohs(comm->message->wire.qdcount) != 1) {
            dp_simple_response(comm, dns_rcode_formerr);
            return;
        }
        if (!dns_rr_parse(&question, comm->message->wire.data, comm->message->length, &offset, false, false)) {
            dp_simple_response(comm, dns_rcode_formerr);
            return;
        }
        dp_dns_query(comm, &question);
        dns_rrdata_free(&question);
        break;

        // No support for other opcodes yet.
    default:
        dp_simple_response(comm, dns_rcode_notimp);
        break;
    }
}

void dns_input(comm_t *comm)
{
    dns_evaluate(comm);
    if (comm->message != NULL) {
        message_free(comm->message);
        comm->message = NULL;
    }
}

static int
usage(const char *progname)
{
    ERROR("usage: " PUB_S_SRP, progname);
    ERROR("ex: dnssd-proxy");
    return 1;
}

// Called whenever we get a connection.
void
connected(comm_t *comm)
{
    INFO("connection from " PRI_S_SRP, comm->name);
    return;
}

static bool config_string_handler(char **ret, const char *filename, const char *string, int lineno, bool tdot,
                                  bool ldot)
{
    char *s;
    int add_trailing_dot = 0;
    int add_leading_dot = ldot ? 1 : 0;
    int len = strlen(string);

    // Space for NUL and leading dot.
    if (tdot && len > 0 && string[len - 1] != '.') {
        add_trailing_dot = 1;
    }
    s = malloc(strlen(string) + add_leading_dot + add_trailing_dot + 1);
    if (s == NULL) {
        ERROR("Unable to allocate domain name " PRI_S_SRP, string);
        return false;
    }
    *ret = s;
    if (ldot) {
        *s++ = '.';
    }
    strcpy(s, string);
    if (add_trailing_dot) {
        s[len] = '.';
        s[len + 1] = 0;
    }
    return true;
}

// Config file parsing...
static bool interface_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    interface_t *interface = calloc(1, sizeof *interface);
    if (interface == NULL) {
        ERROR("Unable to allocate interface " PUB_S_SRP, hunks[1]);
        return false;
    }

    interface->name = strdup(hunks[1]);
    if (interface->name == NULL) {
        ERROR("Unable to allocate interface name " PUB_S_SRP, hunks[1]);
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

static bool port_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    char *ep = NULL;
    long port = strtol(hunks[1], &ep, 10);
    if (port < 0 || port > 65535 || *ep != 0) {
        ERROR("Invalid port number: " PUB_S_SRP, hunks[1]);
        return false;
    }
    if (!strcmp(hunks[0], "udp-port")) {
        udp_port = port;
    } else if (!strcmp(hunks[0], "tcp-port")) {
        tcp_port = port;
    } else if (!strcmp(hunks[0], "tls-port")) {
        tls_port = port;
    }
    return true;
}

static bool listen_addr_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    if (num_listen_addrs == MAX_ADDRS) {
        ERROR("Only %d IPv4 listen addresses can be configured.", MAX_ADDRS);
        return false;
    }
    return config_string_handler(&listen_addrs[num_listen_addrs++], filename, hunks[1], lineno, false, false);
}

static bool tls_key_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    return config_string_handler(&tls_key_filename, filename, hunks[1], lineno, false, false);
}

static bool tls_cert_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    return config_string_handler(&tls_cert_filename, filename, hunks[1], lineno, false, false);
}

static bool tls_cacert_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    return config_string_handler(&tls_cacert_filename, filename, hunks[1], lineno, false, false);
}

config_file_verb_t dp_verbs[] = {
    { "interface",    3, 3, interface_handler },    // interface <name> <domain>
    { "nopush",       3, 3, interface_handler },    // nopush <name> <domain>
    { "udp-port",     2, 2, port_handler },         // udp-port <number>
    { "tcp-port",     2, 2, port_handler },         // tcp-port <number>
    { "tls-port",     2, 2, port_handler },         // tls-port <number>
    { "tls-key",      2, 2, tls_key_handler },      // tls-key <filename>
    { "tls-cert",     2, 2, tls_cert_handler },     // tls-cert <filename>
    { "tls-cacert",   2, 2, tls_cacert_handler },   // tls-cacert <filename>
    { "listen-addr",  2, 2, listen_addr_handler },  // listen-addr <IP address>
};
#define NUMCFVERBS ((sizeof dp_verbs) / sizeof (config_file_verb_t))

int
main(int argc, char **argv)
{
    int i;
    comm_t *listener[4 + MAX_ADDRS];
    int num_listeners = 0;

    udp_port = tcp_port = 53;
    tls_port = 853;

    // Parse command line arguments
    for (i = 1; i < argc; i++) {
	  return usage(argv[0]);
    }

    // Read the config file
    if (!config_parse(NULL, "/etc/dnssd-relay.cf", dp_verbs, NUMCFVERBS)) {
        return 1;
    }

    map_interfaces();

    if (!srp_tls_init()) {
        return 1;
    }

    if (!ioloop_init()) {
        return 1;
    }

    for (i = 0; i < num_listen_addrs; i++) {
        listener[num_listeners] = setup_listener_socket(AF_UNSPEC, IPPROTO_TCP, true,
                                                        tls_port, listen_addrs[i], "DNS TLS Listener", dns_input,
                                                        connected, 0);
        if (listener[num_listeners] == NULL) {
            ERROR("TLS4 listener: fail.");
            return 1;
        }
        num_listeners++;
	}

    // If we haven't been given any addresses to listen on, listen on an IPv4 address and an IPv6 address.
    if (num_listen_addrs == 0) {
        listener[num_listeners] = setup_listener_socket(AF_INET, IPPROTO_TCP, true, tls_port, NULL,
                                                        "IPv4 DNS TLS Listener", dns_input, 0, 0);
        if (listener[num_listeners] == NULL) {
            ERROR("UDP4 listener: fail.");
            return 1;
        }
        num_listeners++;

        listener[num_listeners] = setup_listener_socket(AF_INET6, IPPROTO_TCP, true, tls_port, NULL,
                                                        "IPv6 DNS TLS Listener", dns_input, 0, 0);
        if (listener[num_listeners] == NULL) {
            ERROR("UDP6 listener: fail.");
            return 1;
        }
        num_listeners++;
    }

    for (i = 0; i < num_listeners; i++) {
        INFO("Started " PRI_S_SRP, listener[i]->name);
    }

    do {
        int something = 0;
        something = ioloop_events(0);
        INFO("dispatched %d events.", something);
    } while (1);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
