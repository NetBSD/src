/* srp-dns-proxy.c
 *
 * Copyright (c) 2018-2021 Apple Inc. All rights reserved.
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
 * This is a DNSSD Service Registration Protocol gateway.   The purpose of this is to make it possible
 * for SRP clients to update DNS servers that don't support SRP.
 *
 * The way it works is that this gateway listens on port ANY:53 and forwards either to another port on
 * the same host (not recommended) or to any port (usually 53) on a different host.   Requests are accepted
 * over both TCP and UDP in principle, but UDP requests should be from constrained nodes, and rely on
 * network topology for authentication.
 *
 * Note that this is not a full DNS proxy, so you can't just put it in front of a DNS server.
 */

// Get DNS server IP address
// Get list of permitted source subnets for TCP updates
// Get list of permitted source subnet/interface tuples for UDP updates
// Set up UDP listener
// Set up TCP listener (no TCP Fast Open)
// Event loop
// Transaction processing:
//   1. If UDP, validate that it's from a subnet that is valid for the interface on which it was received.
//   2. If TCP, validate that it's from a permitted subnet
//   3. Check that the message is a valid SRP update according to the rules
//   4. Check the signature
//   5. Do a DNS Update with prerequisites to prevent overwriting a host record with the same owner name but
//      a different key.
//   6. Send back the response

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
#include <dns_sd.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "config-parse.h"
#include "srp-proxy.h"

static addr_t dns_server;
static dns_name_t *service_update_zone; // The zone to update when we receive an update for default.service.arpa.
static hmac_key_t *key;

static int
usage(const char *progname)
{
    ERROR("usage: %s -s <addr> <port> -k <key-file> -t <subnet> ... -u <ifname> <subnet> ...", progname);
    ERROR("  -s can only appear once.");
    ERROR("  -k can appear once.");
    ERROR("  -t can only appear once, and is followed by one or more subnets.");
    ERROR("  -u can appear more than once, is followed by one interface name, and");
    ERROR("     one or more subnets.");
    ERROR("  <addr> is an IPv4 address or IPv6 address.");
    ERROR("  <port> is a UDP port number.");
    ERROR("  <key-file> is a file containing an HMAC-SHA256 key for authenticating updates to the auth server.");
    ERROR("  <subnet> is an IP address followed by a slash followed by the prefix width.");
    ERROR("  <ifname> is the printable name of the interface.");
    ERROR("ex: srp-gw -s 2001:DB8::1 53 -k srp.key -t 2001:DB8:1300::/48 -u en0 2001:DB8:1300:1100::/56");
    return 1;
}

// Free the data structures into which the SRP update was parsed.   The pointers to the various DNS objects that these
// structures point to are owned by the parsed DNS message, and so these do not need to be freed here.
void
update_free_parts(service_instance_t *service_instances, service_instance_t *added_instances,
                  service_t *services, dns_host_description_t *host_description)
{
    service_instance_t *sip;
    service_t *sp;

    for (sip = service_instances; sip; ) {
        service_instance_t *next = sip->next;
        free(sip);
        sip = next;
    }
    for (sip = added_instances; sip; ) {
        service_instance_t *next = sip->next;
        free(sip);
        sip = next;
    }
    for (sp = services; sp; ) {
        service_t *next = sp->next;
        free(sp);
        sp = next;
    }
    if (host_description != NULL) {
        free(host_description);
    }
}

// Free all the stuff that we accumulated while processing the SRP update.
void
update_free(update_t *update)
{
    // Free all of the structures we collated RRs into:
    update_free_parts(update->instances, update->added_instances, update->services, update->host);
    // We don't need to free the zone name: it's either borrowed from the message,
    // or it's service_update_zone, which is static.
    message_free(update->message);
    dns_message_free(update->parsed_message);
    free(update);
}


#define name_to_wire(towire, name) name_to_wire_(towire, name, __LINE__)
void
name_to_wire_(dns_towire_state_t *towire, dns_name_t *name, int line)
{
    // Does compression...
    dns_concatenate_name_to_wire_(towire, name, NULL, NULL, line);
}

void
rdata_to_wire(dns_towire_state_t *towire, dns_rr_t *rr)
{
    dns_rdlength_begin(towire);

    // These are the only types we expect to see.  If something else were passed, it would be written as rdlen=0.
    switch(rr->type) {
    case dns_rrtype_ptr:
        name_to_wire(towire, rr->data.ptr.name);
        break;

    case dns_rrtype_srv:
        dns_u16_to_wire(towire, rr->data.srv.priority);
        dns_u16_to_wire(towire, rr->data.srv.weight);
        dns_u16_to_wire(towire, rr->data.srv.port);
        name_to_wire(towire, rr->data.srv.name);
        break;

    case dns_rrtype_txt:
        dns_rdata_raw_data_to_wire(towire, rr->data.txt.data, rr->data.txt.len);
        break;

    case dns_rrtype_key:
        dns_u16_to_wire(towire, rr->data.key.flags);
        dns_u8_to_wire(towire, rr->data.key.protocol);
        dns_u8_to_wire(towire, rr->data.key.algorithm);
        dns_rdata_raw_data_to_wire(towire, rr->data.key.key, rr->data.key.len);
        break;

    case dns_rrtype_a:
        dns_rdata_raw_data_to_wire(towire, &rr->data.a, sizeof rr->data.a);
        break;

    case dns_rrtype_aaaa:
        dns_rdata_raw_data_to_wire(towire, &rr->data.aaaa, sizeof rr->data.aaaa);
        break;
    }

    dns_rdlength_end(towire);
}

// We only list the types we are using--there are other types that we don't support.
typedef enum prereq_type prereq_type_t;
enum prereq_type {
    update_rrset_equals,     // RFC 2136 section 2.4.2: RRset Exists (Value Dependent)
    update_name_not_in_use,  // RFC 2136 section 2.4.5: Name Is Not In Use
};

void
add_prerequisite(dns_wire_t *msg, dns_towire_state_t *towire, prereq_type_t ptype, dns_name_t *name, dns_rr_t *rr)
{
    char namebuf[DNS_MAX_NAME_SIZE + 1];
    if (ntohs(msg->nscount) != 0 || ntohs(msg->arcount) != 0) {
        ERROR("%s: adding prerequisite after updates", dns_name_print(name, namebuf, sizeof namebuf));
        towire->truncated = true;
    }
    name_to_wire(towire, name);
    switch(ptype) {
    case update_rrset_equals:
        dns_u16_to_wire(towire, rr->type);
        dns_u16_to_wire(towire, rr->qclass);
        dns_ttl_to_wire(towire, 0);
        rdata_to_wire(towire, rr);
        break;
    case update_name_not_in_use:
        dns_u16_to_wire(towire, dns_rrtype_any);   // TYPE
        dns_u16_to_wire(towire, dns_qclass_none);  // CLASS
        dns_ttl_to_wire(towire, 0);                // TTL
        dns_u16_to_wire(towire, 0);                // RDLEN
        break;
    }
    msg->ancount = htons(ntohs(msg->ancount) + 1);
}

// We actually only support one type of delete, so it's a bit silly to specify it, but in principle we might
// want more later.
typedef enum delete_type delete_type_t;
enum delete_type {
    delete_name, // RFC 2136 section 2.5.3: Delete all RRsets from a name
};

void
add_delete(dns_wire_t *msg, dns_towire_state_t *towire, delete_type_t dtype, dns_name_t *name)
{
    name_to_wire(towire, name);
    switch(dtype) {
    case delete_name:
        dns_u16_to_wire(towire, dns_rrtype_any);   // TYPE
        dns_u16_to_wire(towire, dns_qclass_any);   // CLASS
        dns_ttl_to_wire(towire, 0);                // TTL
        dns_u16_to_wire(towire, 0);                // RDLEN
        break;
    }
    msg->nscount = htons(ntohs(msg->nscount) + 1);
}

// Copy the RR we received in the SRP update out in wire format.

void
add_rr(dns_wire_t *msg, dns_towire_state_t *towire, dns_name_t *name, dns_rr_t *rr)
{
    if (rr != NULL) {
        name_to_wire(towire, name);
        dns_u16_to_wire(towire, rr->type);   // TYPE
        dns_u16_to_wire(towire, rr->qclass); // CLASS
        dns_ttl_to_wire(towire, rr->ttl);    // TTL
        rdata_to_wire(towire, rr);           // RDLEN
        msg->nscount = htons(ntohs(msg->nscount) + 1);
    }
}

// Construct an update of the specified type, assuming that the record being updated
// either exists or does not exist, depending on the value of exists.   Actual records
// to be update are taken from the update_t.
//
// Analysis:
//
// The goal of the update is to either bring the zone to the state described in the SRP update, or
// determine that the state described in the SRP update conflicts with what is already present in
// the zone.
//
// Possible scenarios:
// 1. Update and Zone are the same (A and AAAA records may differ):
//    Prerequisites:
//    a. for each instance: KEY RR exists on instance name and is the same
//    b. for host: KEY RR exists on host name and is the same
//    Update:
//    a. for each instance: delete all records on instance name, add KEY RR, add SRV RR, add TXT RR
//    b. for host: delete host instance, add A, AAAA and KEY RRs
//    c. for each service: add PTR record pointing on service name to service instance name
//
// We should try 1 first, because it should be the steady state case; that is, it should be what happens
// most of the time.
// If 1 fails, then we could have some service instances present and others not.   There is no way to
// know without trying.   We can at this point either try to add each service instance in a separate update,
// or assume that none are present and add them all at once, and then if this fails add them individually.
// I think that it makes sense to try them all first, because that should be the second most common case:
//
// 2. Nothing in update is present in zone:
//    Prerequisites:
//    a. For each instance: instance name is not in use
//    b. Host name is not in use
//    Update:
//    a. for each instance: add KEY RR, add SRV RR, add TXT RR on instance name
//    b. for host: add A, AAAA and KEY RRs on host name
//    c. for each service: add PTR record pointing on service name to service instance name
//
// If either (1) or (2) works, we're done.   If both fail, then we need to do the service instance updates
// and host update one by one.   This is a bit nasty because we actually have to try twice: once assuming
// the RR exists, and once assuming it doesn't.   If any of the instance updates fail, or the host update
// fails, we delete all the ones that succeeded.
//
// In the cases other than (1) and (2), we can add all the service PTRs in the host update, because they're
// only added if the host update succeeds; if it fails, we have to go back and remove all the service
// instances.
//
// One open question for the SRP document: we probably want to signal whether the conflict is with the
// hostname or one of the service instance names.   We can do this with an EDNS(0) option.
//
// The flow will be:
// - Try to update assuming everything is there already (case 1)
// - Try to update assuming nothing is there already (case 2)
// - For each service instance:
//   - Try to update assuming it's not there; if this succeeds, add this instance to the list of
//     instances that have been added. If not:
//     - Try to update assuming it is there
//     - If this fails, go to fail
// - Try to update the host (and also services) assuming the host is not there.   If this fails:
//   - Try to update the host (and also services) assuming the host is there.  If this succeeds:
//     - return success
// fail:
// - For each service instance in the list of instances that have been added:
//   - delete all records on the instance name.
//
// One thing that isn't accounted for here: it's possible that a previous update added some but not all
// instances in the current update.  Subsequently, some other device may have claimed an instance that is
// present but in conflict in the current update.   In this case, all of the instances prior to that one
// in the update will actually have been updated by this update, but then the update as a whole will fail.
// I think this is unlikely to be an actual problem, and there's no way to address it without a _lot_ of
// complexity.

bool
construct_update(update_t *update)
{
    dns_towire_state_t towire;
    dns_wire_t *msg = update->update; // Solely to reduce the amount of typing.
    service_instance_t *instance;
    service_t *service;
    host_addr_t *host_addr;

    // Set up the message constructor
    memset(&towire, 0, sizeof towire);
    towire.p = &msg->data[0];  // We start storing RR data here.
    towire.lim = &msg->data[0] + update->update_max; // This is the limit to how much we can store.
    towire.message = msg;

    // Initialize the update message...
    memset(msg, 0, DNS_HEADER_SIZE);
    dns_qr_set(msg, dns_qr_query);
    dns_opcode_set(msg, dns_opcode_update);
    msg->id = srp_random16();

    // An update always has one question, which is the zone name.
    msg->qdcount = htons(1);
    name_to_wire(&towire, update->zone_name);
    dns_u16_to_wire(&towire, dns_rrtype_soa);
    dns_u16_to_wire(&towire, dns_qclass_in);

    switch(update->state) {
    case connect_to_server:
        ERROR("Update construction requested when still connecting.");
        update->update_length = 0;
        return false;

        // Do a DNS Update for a service instance
    case refresh_existing:
        // Add a "KEY exists and is <x> and a PTR exists and is <x> prerequisite for each instance being updated.
        for (instance = update->instances; instance; instance = instance->next) {
            add_prerequisite(msg, &towire, update_rrset_equals, instance->name, update->host->key);
        }
        add_prerequisite(msg, &towire, update_rrset_equals, update->host->name, update->host->key);
        // Now add a delete for each service instance
        for (instance = update->instances; instance; instance = instance->next) {
            add_delete(msg, &towire, delete_name, instance->name);
        }
        add_delete(msg, &towire, delete_name, update->host->name);

    add_instances:
        // Now add the update for each instance.
        for (instance = update->instances; instance; instance = instance->next) {
            add_rr(msg, &towire, instance->name, update->host->key);
            add_rr(msg, &towire, instance->name, instance->srv);
            add_rr(msg, &towire, instance->name, instance->txt);
        }
        // Add the update for each service
        for (service = update->services; service; service = service->next) {
            add_rr(msg, &towire, service->rr->name, service->rr);
        }
        // Add the host records...
        add_rr(msg, &towire, update->host->name, update->host->key);
        for (host_addr = update->host->addrs; host_addr; host_addr = host_addr->next) {
            add_rr(msg, &towire, update->host->name, &host_addr->rr);
        }
        break;

    case create_nonexistent:
        // Add a "name not in use" prerequisite for each instance being updated.
        for (instance = update->instances; instance; instance = instance->next) {
            add_prerequisite(msg, &towire, update_name_not_in_use, instance->name, (dns_rr_t *)NULL);
        }
        add_prerequisite(msg, &towire, update_name_not_in_use, update->host->name, (dns_rr_t *)NULL);
        goto add_instances;

    case create_nonexistent_instance:
        // The only prerequisite is that this specific service instance doesn't exist.
        add_prerequisite(msg, &towire, update_name_not_in_use, update->instance->name, (dns_rr_t *)NULL);
        goto add_instance;

    case refresh_existing_instance:
        // If instance already exists, prerequisite is that it has the same key, and we also have to
        // delete all RRs on the name before adding our RRs, in case they have changed.
        add_prerequisite(msg, &towire, update_rrset_equals, update->instance->name, update->host->key);
        add_delete(msg, &towire, delete_name, update->instance->name);
    add_instance:
        add_rr(msg, &towire, update->instance->name, update->host->key);
        add_rr(msg, &towire, update->instance->name, update->instance->srv);
        add_rr(msg, &towire, update->instance->name, update->instance->txt);
        break;

    case create_nonexistent_host:
        add_prerequisite(msg, &towire, update_name_not_in_use, update->host->name, (dns_rr_t *)NULL);
        goto add_host;

    case refresh_existing_host:
        add_prerequisite(msg, &towire, update_rrset_equals, update->host->name, update->host->key);
        add_delete(msg, &towire, delete_name, update->host->name);
        // Add the service PTRs here--these don't need to be in a separate update, because if we get here
        // the only thing that can make adding them not okay is if adding the host fails.
        // Add the update for each service
        for (service = update->services; service; service = service->next) {
            add_rr(msg, &towire, service->rr->name, service->rr);
        }
    add_host:
        // Add the host records...
        add_rr(msg, &towire, update->host->name, update->host->key);
        for (host_addr = update->host->addrs; host_addr; host_addr = host_addr->next) {
            add_rr(msg, &towire, update->host->name, &host_addr->rr);
        }
        break;

    case delete_failed_instance:
        // Delete all the instances we successfull added before discovering a problem.
        // It is possible in principle that these could have been overwritten by some other
        // process and we could be deleting the wrong stuff, but in practice this should
        // never happen if these are legitimately managed by SRP.   Once a name has been
        // claimed by SRP, it should continue to be managed by SRP until its lease expires
        // and SRP deletes it, at which point it is of course fair game.
        for (instance = update->instances; instance; instance = instance->next) {
            add_delete(msg, &towire, delete_name, instance->name);
        }
        break;
    }
    if (towire.error != 0) {
        ERROR("construct_update: error %s while generating update at line %d", strerror(towire.error), towire.line);
        return false;
    }
    update->update_length = towire.p - (uint8_t *)msg;
    return true;
}

void
update_finished(update_t *update, int rcode)
{
    comm_t *comm = update->client;
    struct iovec iov;
    dns_wire_t response;
    INFO("Update Finished, rcode = " PUB_S_SRP, dns_rcode_name(rcode));

    memset(&response, 0, DNS_HEADER_SIZE);
    response.id = update->message->wire.id;
    response.bitfield = update->message->wire.bitfield;
    dns_rcode_set(&response, rcode);
    dns_qr_set(&response, dns_qr_response);

    iov.iov_base = &response;
    iov.iov_len = DNS_HEADER_SIZE;

    comm->send_response(comm, update->message, &iov, 1);

    // If success, construct a response
    // If fail, send a quick status code
    // Signal host name conflict and instance name conflict using different rcodes (?)
    // Okay, so if there's a host name/instance name conflict, and the host name has the right key, then
    // the instance name is actually bogus and should be overwritten.
    // If the host has the wrong key, and the instance is present, then the instance is also bogus.
    // So in each of these cases, perhaps we should just gc the instance.
    // This would mean that there is nothing to signal: either the instance is a mismatch, and we
    // overwrite it and return success, or the host is a mismatch and we gc the instance and return failure.
    ioloop_close(&update->server->io);
    update_free(update);
}

void
update_send(update_t *update)
{
    struct iovec iov[4];
    dns_towire_state_t towire;
    dns_wire_t *msg = update->update;
    struct timeval tv;
    uint8_t *p_mac;
#ifdef DEBUG_DECODE_UPDATE
    dns_message_t *decoded;
#endif

    // Set up the message constructor
    memset(&towire, 0, sizeof towire);
    towire.p = (uint8_t *)msg + update->update_length;  // We start storing RR data here.
    towire.lim = &msg->data[0] + update->update_max;    // This is the limit to how much we can store.
    towire.message = msg;
    towire.p_rdlength = NULL;
    towire.p_opt = NULL;

    // If we have a key, sign the message with the key using TSIG HMAC-SHA256.
    if (key != NULL) {
        // Maintain an IOV with the bits of the message that we need to sign.
        iov[0].iov_base = msg;

        name_to_wire(&towire, key->name);
        iov[0].iov_len = towire.p - (uint8_t *)iov[0].iov_base;
        dns_u16_to_wire(&towire, dns_rrtype_tsig);            // RRTYPE
        iov[1].iov_base = towire.p;
        dns_u16_to_wire(&towire, dns_qclass_any);             // CLASS
        dns_ttl_to_wire(&towire, 0);                          // TTL
        iov[1].iov_len = towire.p - (uint8_t *)iov[1].iov_base;
        // The message digest skips the RDLEN field.
        dns_rdlength_begin(&towire);                          // RDLEN
        iov[2].iov_base = towire.p;
        dns_full_name_to_wire(NULL, &towire, "hmac-sha256."); // Algorithm Name
        gettimeofday(&tv, NULL);
        dns_u48_to_wire(&towire, tv.tv_sec);                  // Time since epoch
        dns_u16_to_wire(&towire, 300);                        // Fudge interval
                                                              // (clocks can be skewed by up to 5 minutes)
        // Message digest doesn't cover MAC size or MAC fields, for obvious reasons, nor original message ID.
        iov[2].iov_len = towire.p - (uint8_t *)iov[2].iov_base;
        dns_u16_to_wire(&towire, SRP_SHA256_DIGEST_SIZE);       // MAC Size
        p_mac = towire.p;                                     // MAC
        if (!towire.error) {
            if (towire.p + SRP_SHA256_DIGEST_SIZE >= towire.lim) {
                towire.error = ENOBUFS;
                towire.truncated = true;
                towire.line = __LINE__;
            } else {
                towire.p += SRP_SHA256_DIGEST_SIZE;
            }
        }
        // We have to copy the message ID into the tsig signature; this is because in some cases, although not this one,
        // the message ID will be overwritten.   So the copy of the ID is what's validated, but it's copied into the
        // header for validation, so we don't include it when generating the hash.
        dns_rdata_raw_data_to_wire(&towire, &msg->id, sizeof msg->id);
        iov[3].iov_base = towire.p;
        dns_u16_to_wire(&towire, 0);                     // TSIG Error (always 0 on send).
        dns_u16_to_wire(&towire, 0);                     // Other Len (MBZ?)
        iov[3].iov_len = towire.p - (uint8_t *)iov[3].iov_base;
        dns_rdlength_end(&towire);

        // Okay, we have stored the TSIG signature, now compute the message digest.
        srp_hmac_iov(key, p_mac, SRP_SHA256_DIGEST_SIZE, &iov[0], 4);
        msg->arcount = htons(ntohs(msg->arcount) + 1);
        update->update_length = towire.p - (const uint8_t *)msg;
    }

    if (towire.error != 0) {
        ERROR("update_send: error \"%s\" while generating update at line %d",
              strerror(towire.error), towire.line);
        update_finished(update, dns_rcode_servfail);
        return;
    }

#ifdef DEBUG_DECODE_UPDATE
    if (!dns_wire_parse(&decoded, msg, update->update_length, false)) {
        ERROR("Constructed message does not successfully parse.");
        update_finished(update, dns_rcode_servfail);
        return;
    }
#endif

    // Transmit the update
    iov[0].iov_base = update->update;
    iov[0].iov_len = update->update_length;
    update->server->send_response(update->server, update->message, iov, 1);
}

void
update_connect_callback(comm_t *comm)
{
    update_t *update = comm->context;

    // Once we're connected, construct the first update.
    INFO("Connected to " PUB_S_SRP ".", comm->name);
    // STATE CHANGE: connect_to_server -> refresh_existing
    update->state = refresh_existing;
    if (!construct_update(update)) {
        update_finished(update, dns_rcode_servfail);
        return;
    }
    update_send(update);
}

const char *NONNULL
update_state_name(update_state_t state)
{
    switch(state) {
    case connect_to_server:
        return "connect_to_server";
    case create_nonexistent:
        return "create_nonexistent";
    case refresh_existing:
        return "refresh_existing";
    case create_nonexistent_instance:
        return "create_nonexistent_instance";
    case refresh_existing_instance:
        return "refresh_existing_instance";
    case create_nonexistent_host:
        return "create_nonexistent_host";
    case refresh_existing_host:
        return "refresh_existing_host";
    case delete_failed_instance:
        return "delete_failed_instance";
    }
    return "unknown state";
}

void
update_finalize(io_t *context)
{
}

void
update_disconnect_callback(comm_t *comm, int error)
{
    update_t *update = comm->context;

    if (update->state == connect_to_server) {
        INFO(PUB_S_SRP " disconnected: " PUB_S_SRP, comm->name, strerror(error));
        update_finished(update, dns_rcode_servfail);
    } else {
        // This could be bad if any updates succeeded.
        ERROR("%s disconnected during update in state %s: %s",
              comm->name, update_state_name(update->state), strerror(error));
        update_finished(update, dns_rcode_servfail);
    }
}

void
update_reply_callback(comm_t *comm)
{
    update_t *update = comm->context;
    dns_wire_t *wire = &comm->message->wire;
    char namebuf[DNS_MAX_NAME_SIZE + 1], namebuf1[DNS_MAX_NAME_SIZE + 1];
    service_instance_t **pinstance;
    update_state_t initial_state;
    service_instance_t *initial_instance;

    initial_instance = update->instance;
    initial_state = update->state;

    INFO("Message from " PUB_S_SRP " in state " PUB_S_SRP ", rcode = " PUB_S_SRP ".", comm->name,
        update_state_name(update->state), dns_rcode_name(dns_rcode_get(wire)));

    // Sanity check the response
    if (dns_qr_get(wire) == dns_qr_query) {
        ERROR("Received a query from the authoritative server!");
        update_finished(update, dns_rcode_servfail);
        return;
    }
    if (dns_opcode_get(wire) != dns_opcode_update) {
        ERROR("Received a response with opcode %d from the authoritative server!",
              dns_opcode_get(wire));
        update_finished(update, dns_rcode_servfail);
        return;
    }
    if (update->update == NULL) {
        ERROR("Received a response from auth server when no update has been sent yet.");
        update_finished(update, dns_rcode_servfail);
    }
    // This isn't an error in the protocol, because we might be pipelining.   But we _aren't_ pipelining,
    // so there is only one message in flight.   So the message IDs should match.
    if (update->update->id != wire->id) {
        ERROR("Response doesn't have the expected id: %x != %x.", wire->id, update->update->id);
        update_finished(update, dns_rcode_servfail);
    }

    // Handle the case where the update succeeded.
    switch(dns_rcode_get(wire)) {
    case dns_rcode_noerror:
        switch(update->state) {
        case connect_to_server:  // Can't get a response when connecting.
        invalid:
            ERROR("Invalid rcode \"%s\" for state %s",
                  dns_rcode_name(dns_rcode_get(wire)), update_state_name(update->state));
            update_finished(update, dns_rcode_servfail);
            return;

        case create_nonexistent:
            DM_NAME_GEN_SRP(update->host->name, freshly_added_name_buf);
            INFO("SRP Update for host " PRI_DM_NAME_SRP " was freshly added.",
                     DM_NAME_PARAM_SRP(update->host->name, freshly_added_name_buf));
            update_finished(update, dns_rcode_noerror);
            return;

        case refresh_existing:
            DM_NAME_GEN_SRP(update->host->name, refreshed_name_buf);
            INFO("SRP Update for host " PRI_DM_NAME_SRP " was refreshed.",
                 DM_NAME_PARAM_SRP(update->host->name, refreshed_name_buf));
            update_finished(update, dns_rcode_noerror);
            return;

        case create_nonexistent_instance:
            DM_NAME_GEN_SRP(update->instance->name, create_instance_buf);
            INFO("Instance create for " PRI_DM_NAME_SRP " succeeded",
                 DM_NAME_PARAM_SRP(update->instance->name, create_instance_buf));
            // If we created a new instance, we need to remember it in case we have to undo it.
            // To do that, we have to take it off the list.
            for (pinstance = &update->instances; *pinstance != NULL; pinstance = &((*pinstance)->next)) {
                if (*pinstance == update->instance) {
                    break;
                }
            }
            *pinstance = update->instance->next;
            // If there are no more instances to update, then do the host add.
            if (*pinstance == NULL) {
                // STATE CHANGE: create_nonexistent_instance -> create_nonexistent_host
                update->state = create_nonexistent_host;
            } else {
                // Not done yet, do the next one.
                update->instance = *pinstance;
            }
            break;

        case refresh_existing_instance:
            DM_NAME_GEN_SRP(update->instance->name, refreshed_instance_buf);
            INFO("Instance refresh for " PRI_S_SRP " succeeded",
                 DM_NAME_PARAM_SRP(update->instance->name, refreshed_instance_buf));

            // Move on to the next instance to update.
            update->instance = update->instance->next;
            // If there are no more instances to update, then do the host add.
            if (update->instance == NULL) {
                // STATE CHANGE: refresh_existing_instance -> create_nonexistent_host
                update->state = create_nonexistent_host;
            } else {
                // Not done yet, do the next one.
                // STATE CHANGE: refresh_existing_instance -> create_nonexistent_instance
                update->state = create_nonexistent_instance;
            }
            break;

        case create_nonexistent_host:
            DM_NAME_GEN_SRP(update->instance->name, new_host_buf);
            INFO("SRP Update for new host " PRI_S_SRP " was successful.",
                 DM_NAME_PARAM_SRP(update->instance->name, new_host_buf));
            update_finished(update, dns_rcode_noerror);
            return;

        case refresh_existing_host:
            DM_NAME_GEN_SRP(update->instance->name, existing_host_buf);
            INFO("SRP Update for existing host " PRI_S_SRP " was successful.",
                 DM_NAME_PARAM_SRP(update->instance->name, existing_host_buf));
            update_finished(update, dns_rcode_noerror);
            return;

        case delete_failed_instance:
            DM_NAME_GEN_SRP(update->host->name, failed_instance_buf);
            INFO("Instance deletes for host %s succeeded",
                 DM_NAME_PARAM_SRP(update->host->name, failed_instance_buf));
            update_finished(update, update->fail_rcode);
            return;
        }
        break;

        // We will get NXRRSET if we were adding an existing host with the prerequisite that a KEY
        // RR exist on the name with the specified value.  Some other KEY RR may exist, or there may
        // be no such RRSET; we can't tell from this response.
    case dns_rcode_nxrrset:
        switch(update->state) {
        case connect_to_server:           // Can't get a response while connecting.
        case create_nonexistent:          // Can't get nxdomain when creating.
        case create_nonexistent_instance: // same
        case create_nonexistent_host:     // same
        case delete_failed_instance:      // There are no prerequisites for deleting failed instances, so
                                          // in principle this should never fail.
            goto invalid;

        case refresh_existing:
            // If we get an NXDOMAIN when doing a refresh, it means either that there is a conflict,
            // or that one of the instances we are refreshing doesn't exist.   So now do the instances
            // one at a time.

            // STATE CHANGE: refresh_existing -> create_nonexistent
            update->state = create_nonexistent;
            update->instance = update->instances;
            break;

        case refresh_existing_instance:
            // In this case, we tried to update an existing instance and found that the prerequisite
            // didn't match.   This means either that there is a conflict, or else that the instance
            // expired and was deleted between the time that we attempted to create it and the time
            // we attempted to update it.  We could account for this with an create_nonexistent_instance_again
            // state, but currently do not.

            // If we have added some instances, we need to delete them before we send the fail response.
            if (update->added_instances != NULL) {
                // STATE CHANGE: refresh_existing_instance -> delete_failed_instance
                update->state = delete_failed_instance;
            delete_added_instances:
                update->instance = update->added_instances;
                update->fail_rcode = dns_rcode_get(wire);
                break;
            } else {
                update_finished(update, dns_rcode_get(wire));
                return;
            }

        case refresh_existing_host:
            // In this case, there is a conflicting host entry.  This means that all the service
            // instances that exist and are owned by the key we are using are bogus, whether we
            // created them or they were already there.  However, it is not our mission to remove
            // pre-existing messes here, so we'll just delete the ones we added.
            if (update->added_instances != NULL) {
                // STATE CHANGE: refresh_existing_host -> delete_failed_instance
                update->state = delete_failed_instance;
                goto delete_added_instances;
            }
            update_finished(update, dns_rcode_get(wire));
            return;
        }
        break;
        // We get YXDOMAIN if we specify a prerequisite that the name not exist, but it does exist.
    case dns_rcode_yxdomain:
       switch(update->state) {
        case connect_to_server:         // We can't get a response while connecting.
        case refresh_existing:          // If we are refreshing, our prerequisites are all looking for
        case refresh_existing_instance: // a specific RR with a specific value, so we can never get
        case refresh_existing_host:     // YXDOMAIN.
        case delete_failed_instance:    // And if we are deleting failed instances, we should never get an error.
            goto invalid;

        case create_nonexistent:
            // If we get an NXDOMAIN when doing a refresh, it means either that there is a conflict,
            // or that one of the instances we are refreshing doesn't exist.   So now do the instances
            // one at a time.

            // STATE CHANGE: create_nonexistent -> create_nonexistent_instance
            update->state = create_nonexistent_instance;
            update->instance = update->instances;
            break;

        case create_nonexistent_instance:
            // STATE CHANGE: create_nonexistent_instance -> refresh_existing_instance
            update->state = refresh_existing_instance;
            break;

        case create_nonexistent_host:
            // STATE CHANGE: create_nonexistent_host -> refresh_existing_host
            update->state = refresh_existing_host;
            break;
       }
       break;

    case dns_rcode_notauth:
        ERROR("DNS Authoritative server does not think we are authorized to update it, please fix.");
        update_finished(update, dns_rcode_servfail);
        return;

        // We may want to return different error codes or do more informative logging for some of these:
    case dns_rcode_formerr:
    case dns_rcode_servfail:
    case dns_rcode_notimp:
    case dns_rcode_refused:
    case dns_rcode_yxrrset:
    case dns_rcode_notzone:
    case dns_rcode_dsotypeni:
    default:
        goto invalid;
    }

    if (update->state != initial_state) {
        INFO("Update state changed from " PUB_S_SRP " to " PUB_S_SRP, update_state_name(initial_state),
             update_state_name(update->state));
    }
    if (update->instance != initial_instance) {
        DM_NAME_GEN_SRP(initial_instance->name, initial_name_buf);
        DM_NAME_GEN_SRP(update->instance->name, updated_name_buf);
        INFO("Update instance changed from " PRI_DM_NAME_SRP " to " PRI_DM_NAME_SRP,
             DM_NAME_PARAM_SRP(initial_instance->name, initial_name_buf),
             DM_NAME_PARAM_SRP(update->instance->name, updated_name_buf));
    }
    if (construct_update(update)) {
        update_send(update);
    } else {
        ERROR("Failed to construct update");
        update_finished(update, dns_rcode_servfail);
    }
     return;
}

bool
srp_update_start(comm_t *connection, dns_message_t *parsed_message, dns_host_description_t *host,
                 service_instance_t *instance, service_t *service, dns_name_t *update_zone,
                 uint32_t lease_time, uint32_t key_lease_time)
{
    update_t *update;

    // Allocate the data structure
    update = calloc(1, sizeof *update);
    if (update == NULL) {
        ERROR("start_dns_update: unable to allocate update structure!");
        return false;
    }
    // Allocate the buffer in which updates will be constructed.
    update->update = calloc(1, DNS_MAX_UDP_PAYLOAD);
    if (update->update == NULL) {
        ERROR("start_dns_update: unable to allocate update message buffer.");
        return false;
    }
    update->update_max = DNS_DATA_SIZE;

    // Retain the stuff we're supposed to send.
    update->host = host;
    update->instances = instance;
    update->services = service;
    update->parsed_message = parsed_message;
    update->message = connection->message;
    update->state = connect_to_server;
    update->zone_name = update_zone;
    update->client = connection;

    // Start the connection to the server
    update->server = ioloop_connect(&dns_server, false, true, update_reply_callback,
                                    update_connect_callback, update_disconnect_callback, update_finalize, update);
    if (update->server == NULL) {
        free(update);
        return false;
    }
    INFO("Connecting to auth server.");
    return true;
}

static bool
key_handler(void *context, const char *filename, char **hunks, int num_hunks, int lineno)
{
    hmac_key_t *key = context;
    long val;
    char *endptr;
    size_t len;
    uint8_t keybuf[SRP_SHA256_DIGEST_SIZE];
    int error;

    // Validate the constant-size stuff first.
    if (strcasecmp(hunks[1], "in")) {
        ERROR("Expecting tsig key class IN, got %s.", hunks[1]);
        return false;
    }

    if (strcasecmp(hunks[2], "key")) {
        ERROR("expecting tsig key type KEY, got %s", hunks[2]);
        return false;
    }

    // There's not much meaning to be extracted from the flags.
    val = strtol(hunks[3], &endptr, 10);
    if (*endptr != 0 || endptr == hunks[3]) {
        ERROR("Invalid key flags: %s", hunks[3]);
        return false;
    }

    // The protocol number as produced by BIND will always be 3, meaning DNSSEC, but of
    // course we aren't using this key for DNSSEC, so it's not clear that we should take
    // this seriously; hence we just check to see that it's a number.
    val = strtol(hunks[4], &endptr, 10);
    if (*endptr != 0 || endptr == hunks[4]) {
        ERROR("Invalid protocol number: %s", hunks[4]);
        return false;
    }

    // The key algorithm should be HMAC-SHA253.  BIND uses 163, but this is not registered
    // with IANA.   So again, we don't actually require this, but we do validate it so that
    // if someone generated the wrong key type, they'll get a message.
    val = strtol(hunks[5], &endptr, 10);
    if (*endptr != 0 || endptr == hunks[5]) {
        ERROR("Invalid protocol number: %s", hunks[5]);
        return false;
    }
    if (val != 163) {
        INFO("Warning: Protocol number for HMAC-SHA256 TSIG KEY is not 163, but %ld", val);
    }

    key->name = dns_pres_name_parse(hunks[0]);
    if (key->name == NULL) {
        ERROR("Invalid key name: %s", hunks[0]);
        return false;
    }

    error = srp_base64_parse(hunks[6], &len, keybuf, sizeof keybuf);
    if (error != 0) {
        ERROR("Invalid HMAC-SHA256 key: %s", strerror(errno));
        goto fail;
    }

    // The key should be 32 bytes (256 bits).
    if (len == 0) {
        ERROR("Invalid (null) secret for key %s", hunks[0]);
        goto fail;
    }
    key->secret = malloc(len);
    if (key->secret == NULL) {
        ERROR("Unable to allocate space for secret for key %s", hunks[0]);
    fail:
        dns_name_free(key->name);
        key->name = NULL;
        return false;
    }
    memcpy(key->secret, keybuf, len);
    key->length = len;
    key->algorithm = SRP_HMAC_TYPE_SHA256;
    return true;
}

config_file_verb_t key_verbs[] = {
    { NULL, 7, 7, key_handler }
};
#define NUMKEYVERBS ((sizeof key_verbs) / sizeof (config_file_verb_t))

hmac_key_t *
parse_hmac_key_file(const char *filename)
{
    hmac_key_t *key = calloc(1, sizeof *key);
    if (key == NULL) {
        ERROR("No memory for tsig key structure.");
        return NULL;
    }
    if (!config_parse(key, filename, key_verbs, NUMKEYVERBS)) {
        ERROR("Failed to parse key file.");
        free(key);
        return NULL;
    }
    return key;
}

int
main(int argc, char **argv)
{
    int i;
    subnet_t *tcp_validators = NULL;
    udp_validator_t *udp_validators = NULL;
    udp_validator_t *NULLABLE *NONNULL up = &udp_validators;
    subnet_t *NULLABLE *NONNULL nt = &tcp_validators;
    subnet_t *NULLABLE *NONNULL sp;
    addr_t pref;
    uint16_t port;
    socklen_t len, prefalen;
    char *s, *p;
    int width;
    bool got_server = false;

    // Read the configuration from the command line.
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            if (got_server) {
                ERROR("only one authoritative server can be specified.");
                return usage(argv[0]);
            }
            if (++i == argc) {
                ERROR("-s is missing dns server IP address.");
                return usage(argv[0]);
            }
            len = getipaddr(&dns_server, argv[i]);
            if (!len) {
                ERROR("Invalid IP address: %s.", argv[i]);
                return usage(argv[0]);
            }
            if (++i == argc) {
                ERROR("-s is missing dns server port.");
                return usage(argv[0]);
            }
            port = strtol(argv[i], &s, 10);
            if (s == argv[i] || s[0] != '\0') {
                ERROR("Invalid port number: %s", argv[i]);
                return usage(argv[0]);
            }
            if (dns_server.sa.sa_family == AF_INET) {
                dns_server.sin.sin_port = htons(port);
            } else {
                dns_server.sin6.sin6_port = htons(port);
            }
            got_server = true;
        } else if (!strcmp(argv[i], "-k")) {
            if (++i == argc) {
                ERROR("-k is missing key file name.");
                return usage(argv[0]);
            }
            key = parse_hmac_key_file(argv[i]);
            // Someething should already have printed the error message.
            if (key == NULL) {
                return 1;
            }
        } else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "-u")) {
            if (!strcmp(argv[i], "-u")) {
                if (++i == argc) {
                    ERROR("-u is missing interface name.");
                    return usage(argv[0]);
                }
                *up = calloc(1, sizeof **up);
                if (*up == NULL) {
                    ERROR("udp_validators: out of memory.");
                    return usage(argv[0]);
                }
                (*up)->ifname = strdup(argv[i]);
                if ((*up)->ifname == NULL) {
                    ERROR("udp validators: ifname: out of memory.");
                    return usage(argv[0]);
                }
                sp = &((*up)->subnets);
            } else {
                sp = nt;
            }

            if (++i == argc) {
                ERROR("%s requires at least one prefix.", argv[i - 1]);
                return usage(argv[0]);
            }
            s = strchr(argv[i], '/');
            if (s == NULL) {
                ERROR("%s is not a prefix.", argv[i]);
                return usage(argv[0]);
            }
            *s = 0;
            ++s;
            prefalen = getipaddr(&pref, argv[i]);
            if (!prefalen) {
                ERROR("%s is not a valid prefix address.", argv[i]);
                return usage(argv[0]);
            }
            width = strtol(s, &p, 10);
            if (s == p || p[0] != '\0') {
                ERROR("%s (prefix width) is not a number.", p);
                return usage(argv[0]);
            }
            if (width < 0 ||
                (pref.sa.sa_family == AF_INET && width > 32) ||
                (pref.sa.sa_family == AF_INET6 && width > 64)) {
                ERROR("%s is not a valid prefix length for %s", p,
                        pref.sa.sa_family == AF_INET ? "IPv4" : "IPv6");
                return usage(argv[0]);
            }

            *nt = calloc(1, sizeof **nt);
            if (!*nt) {
                ERROR("tcp_validators: out of memory.");
                return 1;
            }

            (*nt)->preflen = width;
            (*nt)->family = pref.sa.sa_family;
            if (pref.sa.sa_family == AF_INET) {
                memcpy((*nt)->bytes, &pref.sin.sin_addr, 4);
            } else {
                memcpy((*nt)->bytes, &pref.sin6.sin6_addr, 8);
            }

            // *up will be non-null for -u and null for -t.
            if (*up) {
                up = &((*up)->next);
            } else {
                nt = sp;
            }
        }
    }
    if (!got_server) {
        ERROR("No authoritative DNS server specified to take updates!");
        return 1;
    }

    if (!ioloop_init()) {
        return 1;
    }

    if (!srp_proxy_listen("home.arpa")) {
        return 1;
    }

    // For now, hardcoded, should be configurable
    service_update_zone = dns_pres_name_parse("home.arpa");

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
