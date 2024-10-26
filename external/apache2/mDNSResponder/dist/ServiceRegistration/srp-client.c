/* srp-client.c
 *
 * Copyright (c) 2018-2023 Apple Inc. All rights reserved.
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
 * SRP Client
 *
 * DNSServiceRegister API for SRP.   See dns_sd.h for details on the API.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include "srp.h"
#ifdef SRP_TEST_SERVER
#undef DNSServiceRegister
#define DNSServiceRegister      srp_client_register
#undef DNSServiceUpdateRecord
#define DNSServiceUpdateRecord  srp_client_update_record
#undef DNSServiceRefDeallocate
#define DNSServiceRefDeallocate srp_client_ref_deallocate
#endif
#ifdef THREAD_DEVKIT_ADK
#include "../mDNSShared/dns_sd.h"
#else
#include <dns_sd.h>
#include <arpa/inet.h>
#endif
#include "srp-api.h"
#include "dns-msg.h"
#include "srp-crypto.h"

// By default, never wait longer than an hour to do another registration attempt.
#define DEFAULT_MAX_ATTEMPT_INTERVAL       1000 * 60 * 60

// Default retry interval is 15 seconds--three attempts. This is how long we will remain in the process of retrying
// an update on a particular server before we give up on that server.

#define DEFAULT_MAX_RETRY_INTERVAL         1000 * 15

// When we start talking to a particular server, we allow 2 seconds before the first retransmission
#define INITIAL_NEXT_RETRANSMISSION_TIME   2000

// When we fail to get through to any server, we will initially re-attempt contacting that server after
// this amount of time
#define INITIAL_NEXT_ATTEMPT_TIME          1000 * 2 * 60

typedef struct client_state client_state_t;

typedef struct service_addr service_addr_t;
struct service_addr {
    service_addr_t *NULLABLE next;
    dns_rr_t rr;
    uint8_t port[2];
};

typedef struct _DNSServiceRef_t reg_state_t;
typedef struct update_context {
    void *udp_context;
    void *message;
    client_state_t *NONNULL client;
    service_addr_t *server;
    size_t message_length;
    uint32_t next_retransmission_time;
    uint32_t next_attempt_time;
    uint32_t lease_time;
    uint32_t key_lease_time;
    uint32_t serial;
    uint32_t interface_serial;
    bool notified;  // Callers have been notified.
    bool connected; // UDP context is connected.
    bool removing;  // We are removing the current registration(s)
} update_context_t;

struct _DNSServiceRef_t {
    reg_state_t *NULLABLE next;
    uint32_t serial;
    DNSServiceFlags flags;
    uint32_t interfaceIndex;
    char *NULLABLE name;
    char *NULLABLE regtype;
    char *NULLABLE domain;
    char *NULLABLE host;
    int port;
    uint16_t txtLen;
    void *NULLABLE txtRecord;
    DNSServiceRegisterReply callback;
    bool succeeded;
    bool called_back;
    bool removing;
    bool skip;
    void *NULLABLE context;
};

struct client_state {
    client_state_t *next;
    reg_state_t *registrations;
    char *hostname;
    int hostname_rename_number; // If we've had a naming conflict, this will be nonzero.
    srp_hostname_conflict_callback_t hostname_conflict_callback;
    srp_key_t *key;
    void *os_context;
    uint32_t lease_time;
    uint32_t key_lease_time;
    uint32_t srp_max_attempt_interval;
    uint32_t registration_serial;
    uint32_t srp_max_retry_interval;
    service_addr_t stable_server;
    bool srp_server_synced;

    // Currently we only ever have one update in flight.  If we decide we need to send another,
    // we need to cancel the one we're currently doing.
    update_context_t *active_update;
};

// Implementation of SRP network entry points, which can be called by the network implementation on the
// hosting platform.

static bool network_state_changed = false;
static bool doing_refresh = false;
static service_addr_t *interfaces;
static service_addr_t *servers;
static service_addr_t *interface_refresh_state;
static service_addr_t *server_refresh_state;
static uint8_t no_port[2];
static uint32_t interface_serial;

client_state_t *clients;
client_state_t *current_client;
bool zero_addresses = false; // for testing, used by srp-ioloop.c.

// Forward references
static int do_srp_update(client_state_t *client, bool definite, bool *did_something);
static void udp_response(void *v_update_context, void *v_message, size_t message_length);

static bool srp_is_network_active(void);

#define VALIDATE_IP_ADDR                                         \
    if ((rrtype != dns_rrtype_a && rrtype != dns_rrtype_aaaa) || \
        (rrtype == dns_rrtype_a && rdlen != 4) ||                \
        (rrtype == dns_rrtype_aaaa && rdlen != 16)) {            \
        return kDNSServiceErr_Invalid;                           \
    }


client_state_t *
srp_client_get_current(void)
{
    return current_client;
}

void
srp_client_set_current(client_state_t *new_client)
{
    current_client = new_client;
}

// Call this before calling anything else.   Context will be passed back whenever the srp code
// calls any of the host functions.
int
srp_host_init(void *context)
{
    client_state_t *new_client = calloc(1, sizeof(*new_client));
    if (new_client == NULL) {
        return kDNSServiceErr_NoMemory;
    }
    new_client->os_context = context;
    new_client->lease_time = 3600;       // 1 hour for registration leases
    new_client->key_lease_time = 604800; // 7 days for key leases
    new_client->registration_serial = 1;
    new_client->srp_max_attempt_interval = DEFAULT_MAX_ATTEMPT_INTERVAL;
    new_client->srp_max_retry_interval = DEFAULT_MAX_RETRY_INTERVAL;

    current_client = new_client;
    new_client->next = clients;
    clients = current_client;
    return kDNSServiceErr_NoError;
}

int
srp_host_key_reset_for_client(client_state_t *client)
{
    if (client->key != NULL) {
        srp_keypair_free(client->key);
        client->key = NULL;
    }
    return srp_reset_key("com.apple.srp-client.host-key", client->os_context);
}

int
srp_host_key_reset(void)
{
    return srp_host_key_reset_for_client(current_client);
}

int
srp_set_lease_times(uint32_t new_lease_time, uint32_t new_key_lease_time)
{
    current_client->lease_time = new_lease_time;
    current_client->key_lease_time = new_key_lease_time;
    return kDNSServiceErr_NoError;
}

static void
sync_from_stable_storage(update_context_t *update)
{
    service_addr_t *server;
    client_state_t *client = update->client;
    if (!client->srp_server_synced) {
        client->srp_server_synced =
            srp_get_last_server(&client->stable_server.rr.type, (uint8_t *)&client->stable_server.rr.data,
                                sizeof(client->stable_server.rr.data), &client->stable_server.port[0],
                                client->os_context);
        // Nothing read.
        if (!client->srp_server_synced) {
            return;
        }
    } else {
        if (update->server != NULL) {
            return;
        }
    }

    // See if one of the advertised servers is the one we last updated.
    for (server = servers; server; server = server->next) {
        if (server->rr.type == client->stable_server.rr.type &&
            !memcmp(&server->port, &client->stable_server.port, 2) &&
            ((server->rr.type == dns_rrtype_a && !memcmp(&server->rr.data, &client->stable_server.rr.data, 4)) ||
             (server->rr.type == dns_rrtype_aaaa && !memcmp(&server->rr.data, &client->stable_server.rr.data, 16))))
        {
            update->server = server;
            return;
        }
    }
}

static void
sync_to_stable_storage(update_context_t *update)
{
    client_state_t *client = update->client;
    if (!client->srp_server_synced) {
        client->srp_server_synced =
            srp_save_last_server(client->stable_server.rr.type, (uint8_t *)&client->stable_server.rr.data,
                                 client->stable_server.rr.type == dns_rrtype_a ? 4 : 16,
                                 client->stable_server.port, client->os_context);
    }
}

// Find an address on a list of addresses.
static service_addr_t **
find_address(service_addr_t **addrs, const uint8_t *port, uint16_t rrtype, const uint8_t *rdata, uint16_t rdlen)
{
    service_addr_t *addr, **p_addr = addrs;

    while (*p_addr != NULL) {
        addr = *p_addr;
        if (addr->rr.type == rrtype && !memcmp(&addr->rr.data, rdata, rdlen) && !memcmp(addr->port, port, 2)) {
            break;
        }
        p_addr = &addr->next;
    }
    return p_addr;
}

// Worker function to add an address and notice whether the network state has changed (so as to trigger a
// refresh).
static int
add_address(service_addr_t **list, service_addr_t **refresh,
            const uint8_t *port, uint16_t rrtype, const uint8_t *rdata, uint16_t rdlen, bool interface_serial_update)
{
    service_addr_t *addr, **p_addr, **p_refresh;

    VALIDATE_IP_ADDR;

    // See if the address is on the refresh list.
    p_refresh = find_address(refresh, port, rrtype, rdata, rdlen);

    // See also if it's on the address list (shouldn't be on both).  This also finds the end of the list.
    p_addr = find_address(list, port, rrtype, rdata, rdlen);
    if (*p_addr != NULL) {
        return kDNSServiceErr_NoError;
    }

    if (*p_refresh != NULL) {
        addr = *p_refresh;

        // This shouldn't happen, but if it does, free the old address.
        if (*p_addr != NULL) {
            ERROR("duplicate address during refresh!");
            free(addr);
            return kDNSServiceErr_NoError;
        }

        *p_refresh = addr->next;
        addr->next = NULL;
        *p_addr = addr;

        // In this case, the network state has not changed.
        return kDNSServiceErr_NoError;
    }

    addr = calloc(1, sizeof *addr);
    if (addr == NULL) {
        return kDNSServiceErr_NoMemory;
    }
    addr->rr.type = rrtype;
    addr->rr.qclass = dns_qclass_in;
    memcpy(&addr->rr.data, rdata, rdlen);
    memcpy(&addr->port, port, 2);
    *p_addr = addr;
    network_state_changed = true;
    if (interface_serial_update) {
        interface_serial++;
    }

    // Print IPv6 address directly here because the code has to be portable for ADK, and OpenThread environment
    // has no support for INET6_ADDRSTRLEN.
    INFO("added " PUB_S_SRP
         " address: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x port %u (%04x)",
         *list == servers ? "server" : "interface",
         rdata[0], rdata[1], rdata[2], rdata[3], rdata[4], rdata[5], rdata[6], rdata[7],
         rdata[8], rdata[9], rdata[10], rdata[11], rdata[12], rdata[13], rdata[14], rdata[15],
         port != NULL ? (port[0] << 8 | port[1]) : 0, port != NULL ? (port[0] << 8 | port[1]) : 0);

    return kDNSServiceErr_NoError;
}

// Called when a new address is configured that should be advertised.  This can be called during a refresh,
// in which case it doesn't mark the network state as changed if the address was already present.
int
srp_add_interface_address(uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen)
{
    return add_address(&interfaces, &interface_refresh_state, no_port, rrtype, rdata, rdlen, true);
}

// Called whenever the SRP server address changes or the SRP server becomes newly reachable.  This can be
// called during a refresh, in which case it doesn't mark the network state as changed if the address was
// already present.
int
srp_add_server_address(const uint8_t *port, uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen)
{
    VALIDATE_IP_ADDR;

    return add_address(&servers, &server_refresh_state, port, rrtype, rdata, rdlen, false);
}

// Called when the node knows its hostname (usually once).   The callback is called if we try to do an SRP
// update and find out that the hostname is in use; in this case, the callback is expected to generate a new
// hostname and re-register it.   It is permitted to call srp_set_hostname() from the callback.
// If the hostname is changed by the callback, then it is used immediately on return from the callback;
// if the hostname is changed in any other situation, nothing is done with the new name until
// srp_network_state_stable() is called.
int
srp_set_hostname(const char *NONNULL name, srp_hostname_conflict_callback_t callback)
{
    if (current_client->hostname != NULL) {
        free(current_client->hostname);
    }
    current_client->hostname = strdup(name);
    if (current_client->hostname == NULL) {
        return kDNSServiceErr_NoMemory;
    }
    current_client->hostname_conflict_callback = callback;
    network_state_changed = true;
    return kDNSServiceErr_NoError;
}

// Called when a network state change is complete (that is, all new addresses have been saved and
// any update to the SRP server address has been provided).   This is only needed when not using the
// refresh mechanism.
static bool
srp_is_network_active(void)
{
    INFO("nsc = %d servers = %p interfaces = %p, hostname = " PRI_S_SRP,
          network_state_changed, servers, interfaces,
         current_client->hostname ? current_client->hostname : "<not set>");
    return servers != NULL && (interfaces != NULL || zero_addresses) && current_client->hostname != NULL;
}

int
srp_network_state_stable(bool *did_something)
{
    client_state_t *client;
    int status = kDNSServiceErr_NoError;
    if (network_state_changed && srp_is_network_active()) {
        network_state_changed = false;
        for (client = clients; client; client = client->next) {
            int ret = do_srp_update(client, false, did_something);
            // In the normal case, there will only be one client, and therefore one return status.  For testing,
            // we allow more than one client; if we get an error here, we return it, but we still launch all the
            // updates.
            if (ret != kDNSServiceErr_NoError && status == kDNSServiceErr_NoError) {
                status = ret;
            }
        }
    }
    return kDNSServiceErr_NoError;
}

// Worker function to delete a server or interface address that was previously configured.
static int
delete_address(service_addr_t **list, const uint8_t *port, uint16_t rrtype, const uint8_t *NONNULL rdata,
               uint16_t rdlen, bool interface_serial_update)
{
    service_addr_t *addr, **p_addr;

    // Delete API and refresh API are incompatible.
    if (doing_refresh) {
        return kDNSServiceErr_BadState;
    }
    VALIDATE_IP_ADDR;

    // See if we know this address.
    p_addr = find_address(list, port, rrtype, rdata, rdlen);
    if (*p_addr != NULL) {
        addr = *p_addr;
        *p_addr = addr->next;
        free(addr);
        network_state_changed = true;
        if (interface_serial_update) {
            interface_serial++;
        }
        return kDNSServiceErr_NoError;
    }
    return kDNSServiceErr_NoSuchRecord;
}

// Delete a previously-configured SRP server address.  This should not be done during a refresh.
int
srp_delete_interface_address(uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen)
{
    return delete_address(&interfaces, no_port, rrtype, rdata, rdlen, true);
}

// Delete a previously-configured SRP server address.  This should not be done during a refresh.
int
srp_delete_server_address(uint16_t rrtype, const uint8_t *port, const uint8_t *NONNULL rdata, uint16_t rdlen)
{
    return delete_address(&servers, port, rrtype, rdata, rdlen, false);
}

// Call this to start an address refresh.   This makes sense to do in cases where the caller
// is not tracking changes, but rather is just doing a full refresh whenever the network state
// is seen to have changed.   When the refresh is done, if any addresses were added or removed,
// network_state_changed will be true, and so a call to dnssd_network_state_change_finished()
// will trigger an update; if nothing changed, no update will be sent.
int
srp_start_address_refresh(void)
{
    if (doing_refresh) {
        return kDNSServiceErr_BadState;
    }
    doing_refresh = true;
    interface_refresh_state = interfaces;
    server_refresh_state = servers;
    interfaces = NULL;
    servers = NULL;
    network_state_changed = false;
    return kDNSServiceErr_NoError;
}

// Call this when the address refresh is done.   This invokes srp_network_state_stable().
int
srp_finish_address_refresh(bool *did_something)
{
    service_addr_t *addr, *next;
    int i;
    if (!doing_refresh) {
        return kDNSServiceErr_BadState;
    }
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            next = server_refresh_state;
            server_refresh_state = NULL;
        } else {
            if (interface_refresh_state != NULL) {
                interface_serial++;
            }
            next = interface_refresh_state;
            interface_refresh_state = NULL;
        }
        if (next != NULL) {
            network_state_changed = true;
        }
        while (next) {
            uint8_t *rdata = (uint8_t *)&next->rr.data;
            // Print IPv6 address directly here because the code has to be portable for ADK, and OpenThread environment
            // has no support for INET6_ADDRSTRLEN.
            INFO("deleted " PUB_S_SRP
                 " address: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x port %u (%x)",
                 i ? "interface" : "server",
                 rdata[0], rdata[1], rdata[2], rdata[3], rdata[4], rdata[5], rdata[6], rdata[7],
                 rdata[8], rdata[9], rdata[10], rdata[11], rdata[12], rdata[13], rdata[14], rdata[15],
                 (next->port[0] << 8) | next->port[1], (next->port[0] << 8) | next->port[1]);
            addr = next;
            next = addr->next;
            free(addr);
        }
    }
    doing_refresh = false;
    return srp_network_state_stable(did_something);
}

// Implementation of the API that the application will call to update the TXT record after having registered
// a service previously with a different TXT record.   In principle this can also update a record added with
// DNSServiceAddRecord or DNSServiceRegisterRecord, but we don't support those APIs at present.

DNSServiceErrorType
DNSServiceUpdateRecord(DNSServiceRef sdRef, DNSRecordRef RecordRef, DNSServiceFlags flags,
                       uint16_t rdlen, const void *rdata, uint32_t ttl)
{
    reg_state_t *registration;
    void *txtRecord = NULL;

    (void)RecordRef;
    (void)flags;
    (void)ttl;

    if (sdRef == NULL || RecordRef != NULL || rdata == NULL) {
        return kDNSServiceErr_Invalid;
    }

    // Add it to the list (so it will appear valid to DNSServiceRefDeallocate()).
    for (registration = current_client->registrations; registration != NULL; registration = registration->next) {
        if (registration == sdRef) {
            break;
        }
    }
    if (registration == NULL) {
        return kDNSServiceErr_BadReference;
    }

    if (rdlen != 0) {
        txtRecord = malloc(rdlen);
        if (txtRecord == NULL) {
            return kDNSServiceErr_NoMemory;
        }
        memcpy(txtRecord, rdata, rdlen);
    } else {
        registration->txtRecord = NULL;
    }

    if (registration->txtRecord != NULL) {
        free(registration->txtRecord);
    }

    registration->txtRecord = txtRecord;
    registration->txtLen = rdlen;
    network_state_changed = true;
    interface_serial++;
    return kDNSServiceErr_NoError;
}

// Implementation of the API that applications will call to register services.   This is independent of the
// hosting platform API.
DNSServiceErrorType
DNSServiceRegister(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                   const char *NULLABLE name, const char *NULLABLE regtype, const char *NULLABLE domain,
                   const char *NULLABLE host, uint16_t port,
                   uint16_t txtLen, const void *txtRecord,
                   DNSServiceRegisterReply callBack, void *context)
{
    reg_state_t **rp, *reg = calloc(1, sizeof *reg);
    if (reg == NULL) {
        return kDNSServiceErr_NoMemory;
    }

    // Add it to the list (so it will appear valid to DNSServiceRefDeallocate()).
    rp = &current_client->registrations;
    while (*rp) {
        rp = &((*rp)->next);
    }
    *rp = reg;

    // If we don't already have a hostname, use the one from the registration.
    if (current_client->hostname == NULL) {
        srp_set_hostname(host, NULL);
    }

    reg->serial = current_client->registration_serial++;
    reg->flags = flags;
    reg->interfaceIndex = interfaceIndex;
    reg->called_back = true;
#define stashName(thing)                        \
    if (thing != NULL) {                        \
        reg->thing = strdup(thing);             \
        if (reg->thing == NULL) {               \
            DNSServiceRefDeallocate(reg);       \
            return kDNSServiceErr_NoMemory;     \
        }                                       \
    } else {                                    \
        reg->thing = NULL;                      \
    }
    stashName(name);
    stashName(regtype);
    stashName(domain);
    stashName(host);
    reg->port = port;
    reg->txtLen = txtLen;
    if (txtLen != 0) {
        reg->txtRecord = malloc(txtLen);
        if (reg->txtRecord == NULL) {
            DNSServiceRefDeallocate(reg);
            return kDNSServiceErr_NoMemory;
        }
        memcpy(reg->txtRecord, txtRecord, txtLen);
    } else {
        reg->txtRecord = NULL;
    }
    reg->callback = callBack;
    reg->context = context;
    *sdRef = reg;
    network_state_changed = true;
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType
srp_update_service_type(DNSServiceRef NONNULL reg, const char *NONNULL regtype, DNSServiceRegisterReply callback, void *context)
{
    if (reg != NULL) {
        client_state_t *client = NULL;

        for (client_state_t *cp = clients; client == NULL && cp != NULL; cp = cp->next) {
            // Remove it from the list.
            for (reg_state_t *rp = cp->registrations; client == NULL && rp != NULL; rp = rp->next) {
                if (rp == reg) {
                    client = cp;
                }
            }
        }
        if (client == NULL) {
            return kDNSServiceErr_Unknown;
        }

        stashName(regtype);
        reg->serial = client->registration_serial++;
        reg->callback = callback;
        reg->context = context;
        network_state_changed = true;
    } else {
        return kDNSServiceErr_Invalid;
    }
    return kDNSServiceErr_NoError;
}

void
DNSServiceRefDeallocate(DNSServiceRef sdRef)
{
    reg_state_t **rp, *reg = NULL;
    bool found = false;
    client_state_t *client;

    if (sdRef == NULL) {
        return;
    }

    for (client = clients; client; client = client->next) {
        // Remove it from the list.
        rp = &client->registrations;
        reg = *rp;
        while (*rp) {
            if (reg == sdRef) {
                *rp = reg->next;
                found = true;
                break;
            }
            rp = &((*rp)->next);
            reg = *rp;
        }

        if (found) {
            break;
        }
    }

    // This avoids a bogus free.
    if (!found || reg == NULL) {
        return;
    }
    if (reg->name != NULL) {
        free(reg->name);
    }
    if (reg->regtype != NULL) {
        free(reg->regtype);
    }
    if (reg->domain != NULL) {
        free(reg->domain);
    }
    if (reg->host != NULL) {
        free(reg->host);
    }
    if (reg->txtRecord != NULL) {
        free(reg->txtRecord);
    }
    free(reg);
}

static void
update_finalize(update_context_t *update)
{
    client_state_t *client = update->client;
    INFO("%p %p %p", update, update->udp_context, update->message);
    if (update->udp_context != NULL) {
        srp_deactivate_udp_context(client->os_context, update->udp_context);
    }
    if (update->message != NULL) {
        free(update->message);
    }
    free(update);
}

static void
do_callbacks(client_state_t *client, reg_state_t *registration, uint32_t serial, int err, bool succeeded)
{
    reg_state_t *rp;
    bool work;

    // The callback can modify the list, so we use a marker to remember where we are in the list rather
    // than remembering a pointer which could be invalidated.  If a callback adds a registration, that
    // registration doesn't get called because called_back is set to true when a registration is added.
    for (rp = client->registrations; rp; rp = rp->next) {
        if (rp->serial <= serial) {
            rp->called_back = false;
        }
    }
    do {
        work = false;
        for (rp = client->registrations; rp; rp = rp->next) {
            if (registration != NULL && registration != rp) {
                continue;
            }
            if (rp->serial > serial || rp->callback == NULL || rp->called_back) {
                continue;
            }
            work = true;
            rp->called_back = true;
            if (succeeded) {
                rp->succeeded = true;
            }
            if (rp->callback != NULL) {
                rp->callback(rp, kDNSServiceFlagsAdd, err, rp->name, rp->regtype, rp->domain, rp->context);
                break;
            }
        }
    } while (work);
}

static void
udp_retransmit(void *v_update_context)
{
    update_context_t *context = v_update_context;
    client_state_t *client;
    service_addr_t *next_server = NULL;
    int err;

    client = context->client;

    if (!srp_is_network_active()) {
        INFO("network is down, discontinuing renewals.");
        if (client->active_update != NULL) {
            update_finalize(client->active_update);
            client->active_update = NULL;
        }
        return;
    }
    // It shouldn't be possible for this to happen.
    if (client->active_update == NULL) {
        INFO("no active update for " PRI_S_SRP " (%p).",
             client->hostname ? client->hostname : "<null>", client);
        return;
    }
    INFO("next_attempt %" PRIu32 " next_retransmission %" PRIu32 " for " PRI_S_SRP " (%p)",
         context->next_attempt_time, context->next_retransmission_time,
         client->hostname ? client->hostname : "<null>", client);

    // If the interface serial number has changed, we need to generate a new update message.
    if (client->active_update->interface_serial != interface_serial) {
        client->active_update->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
        client->active_update->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;
        free(context->message);
        context->message = NULL;
    }

    // If next retransmission time is zero, this means that we gave up our last attempt to register, and have
    // now waited long enough to try again.  We will then use an exponential backoff for 90 seconds before giving
    // up again; if we give up again, we will wait longer to retry, up to an hour.
    else if (context->next_retransmission_time == 0) {
        // If there are no servers, we don't need to schedule a re-attempt: when a server is seen, we will do
        // an update immediately.
        if (servers == NULL) {
            return;
        }
        next_server = servers;

        // If this attempt fails, don't try again for a while longer, but limit the retry interval to an hour.
        context->next_attempt_time *= 2;
        if (context->next_attempt_time > client->srp_max_attempt_interval) {
            context->next_attempt_time = client->srp_max_attempt_interval;
        }
        context->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
    }
    // If this would be our fourth retry on a particular server, try the next server.
    else if (context->next_retransmission_time > client->srp_max_retry_interval) {
        // If we are removing, there is no point in trying the next server--just give up and report a timeout.
        if (context->removing) {
            do_callbacks(client, NULL, context->serial, kDNSServiceErr_Timeout, false);
            // Once the goodbye retransmission has timed out, we're done.
            return;
        }
        for (next_server = servers; next_server; next_server = next_server->next) {
            if (next_server == context->server) {
                // We're going to use the next server after the one we just tried.  If we run out of servers,
                // we'll give up for a while.
                next_server = next_server->next;
                break;
            }
        }

        // If we run off the end of the list, give up for a bit.
        if (next_server == NULL) {
            context->next_retransmission_time = 0;
        } else {
            context->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
        }
    }
    // Otherwise, we are still trying to win with a particular server, so back off exponentially.
    else {
        context->next_retransmission_time *= 2;
    }

    // If we are giving up on the current server, get rid of any udp state.
    if (context->next_retransmission_time == 0 || next_server != NULL) {
        if (next_server != NULL) {
            context->server = next_server;
        }
        srp_disconnect_udp(context->udp_context);
        context->connected = false;
        if (context->message != NULL) {
            free(context->message);
        }
        context->message = NULL;
        context->message_length = 0;
        context->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
        context->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;
    }

    // If we are not giving up, send the next packet.
    if (context->server != NULL && context->next_retransmission_time != 0) {
        if (!context->connected) {
            // Create a UDP context for this transaction.
            err = srp_connect_udp(context->udp_context, context->server->port, context->server->rr.type,
                                  (uint8_t *)&context->server->rr.data,
                                  context->server->rr.type == dns_rrtype_a ? 4 : 16);
            // In principle if it fails here, it might succeed later, so we just don't send a packet and let
            // the timeout take care of it.
            if (err != kDNSServiceErr_NoError) {
                ERROR("udp_retransmit: error %d connecting udp context.", err);
            } else {
                if (context->server->rr.type == dns_rrtype_a) {
#ifdef THREAD_DEVKIT_ADK
                    INFO("updating server at address %d.%d.%d.%d", (context->server->rr.data.a.s_addr >> 24) & 255,
                         (context->server->rr.data.a.s_addr >> 16) & 255,
                         (context->server->rr.data.a.s_addr >> 8) & 255, (context->server->rr.data.a.s_addr) & 255);
#else
                    IPv4_ADDR_GEN_SRP(&context->server->rr.data.a, addr_buf);
                    INFO("updating server at address " PRI_IPv4_ADDR_SRP,
                         IPv4_ADDR_PARAM_SRP(&context->server->rr.data.a, addr_buf));
#endif
                } else if (context->server->rr.type == dns_rrtype_aaaa) {
#ifdef THREAD_DEVKIT_ADK
                    INFO("updating server at address "
                         "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                         context->server->rr.data.aaaa.s6_addr[0], context->server->rr.data.aaaa.s6_addr[1],
                         context->server->rr.data.aaaa.s6_addr[2], context->server->rr.data.aaaa.s6_addr[3],
                         context->server->rr.data.aaaa.s6_addr[4], context->server->rr.data.aaaa.s6_addr[5],
                         context->server->rr.data.aaaa.s6_addr[6], context->server->rr.data.aaaa.s6_addr[7],
                         context->server->rr.data.aaaa.s6_addr[8], context->server->rr.data.aaaa.s6_addr[9],
                         context->server->rr.data.aaaa.s6_addr[10], context->server->rr.data.aaaa.s6_addr[11],
                         context->server->rr.data.aaaa.s6_addr[12], context->server->rr.data.aaaa.s6_addr[13],
                         context->server->rr.data.aaaa.s6_addr[14], context->server->rr.data.aaaa.s6_addr[15]);
#else
                    SEGMENTED_IPv6_ADDR_GEN_SRP(&context->server->rr.data.aaaa, addr_buf);
                    INFO("updating server at address " PRI_SEGMENTED_IPv6_ADDR_SRP,
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(&context->server->rr.data.aaaa, addr_buf));
#endif
                }
                context->connected = true;
            }
        }

        if (context->message == NULL) {
            context->message = srp_client_generate_update(client, client->lease_time, client->key_lease_time,
                                                          &context->message_length, NULL, context->serial,
                                                          context->removing);
            if (context->message == NULL) {
                ERROR("No memory for message.");
                return;
            }
        }

        if (context->connected) {
            // Send the datagram to the server
            err = srp_send_datagram(client->os_context, context->udp_context, context->message, context->message_length);
            if (err != kDNSServiceErr_NoError) {
                ERROR("udp_retransmit: error %d sending a datagram.", err);
            }
        }
    }

    // If we've given up for now, either schedule a next attempt or notify the caller; otherwise, schedule the next retransmission.
    if (context->next_retransmission_time == 0) {
        bool timeout_requested = false;
        reg_state_t *registration;
        for (registration = client->registrations; registration; registration = registration->next) {
            if (registration->callback != NULL && registration->serial <= context->serial &&
                (registration->flags & kDNSServiceFlagsTimeout))
            {
                timeout_requested = true;
            }
        }
        // If any of the callers requested a timeout, we treat it as if they all did, and call all the callbacks with the "timed out"
        // error.
        if (timeout_requested) {
            do_callbacks(client, NULL, context->serial, kDNSServiceErr_Timeout, false);
            err = kDNSServiceErr_NoError;
        } else {
            err = srp_set_wakeup(client->os_context, context->udp_context, context->next_attempt_time, udp_retransmit);
        }
    } else {
        err = srp_set_wakeup(client->os_context, context->udp_context,
                             context->next_retransmission_time - 512 + srp_random16() % 1024, udp_retransmit);
    }
    if (err != kDNSServiceErr_NoError) {
        INFO("error %d setting wakeup", err);
        // what to do?
    }
}

static void
renew_callback(void *v_update_context)
{
    update_context_t *context = v_update_context;
    client_state_t *client = context->client;
    INFO("renew callback");
    do_srp_update(client, true, NULL);
}

// This function will, if hostname_rename_number is nonzero, create a hostname using the chosen hostname plus
// space plus the number as ascii text.   The caller is responsible for freeing the return value if it's not NULL.
static char *
conflict_print(client_state_t *client, dns_towire_state_t *towire, char **return_hostname, char *chosen_hostname)
{
    char *conflict_hostname;
    size_t hostname_len;

    if (client->hostname_rename_number == 0) {
        *return_hostname = chosen_hostname;
        return NULL;
    }

    hostname_len = strlen(chosen_hostname);
    // 7 is max length of decimal short (5) plus space plus NUL
    if (hostname_len + 7 > DNS_MAX_LABEL_SIZE) {
        hostname_len = DNS_MAX_LABEL_SIZE - 7;
    }
    conflict_hostname = malloc(hostname_len + 7);
    if (conflict_hostname == NULL) {
        if (towire != NULL) {
            towire->line = __LINE__;
            towire->outer_line = -1;
            towire->error = true;
        }
        *return_hostname = chosen_hostname;
        return NULL;
    }

    memcpy(conflict_hostname, chosen_hostname, hostname_len);
    snprintf(conflict_hostname + hostname_len, 7, " %d", client->hostname_rename_number);
    *return_hostname = conflict_hostname;
    return conflict_hostname;
}

static void
udp_response(void *v_update_context, void *v_message, size_t message_length)
{
    update_context_t *context = v_update_context;
    client_state_t *client = context->client;
    dns_wire_t *message = v_message;
    int err;
    int rcode = dns_rcode_get(message);
    (void)message_length;
    uint32_t new_lease_time = 0;
    bool lease_time_sent = false;
    reg_state_t *registration;
    const uint8_t *p = message->data;
    const uint8_t *end = (const uint8_t *)v_message + message_length;
    bool resolve_name_conflict = false;
    char *conflict_hostname = NULL, *chosen_hostname;

    INFO("Got a response for %p, rcode = %d", client, dns_rcode_get(message));

    // Cancel the existing retransmit wakeup, since we definitely don't want to retransmit to the current
    // server.
    err = srp_cancel_wakeup(client->os_context, context->udp_context);
    if (err != kDNSServiceErr_NoError) {
        INFO("%d", err);
    }

    // We want a different UDP source port for each transaction, so cancel the current UDP state.
    srp_disconnect_udp(context->udp_context);
    if (context->message != NULL) {
        free(context->message);
    }
    context->message = NULL;
    context->message_length = 0;
    context->connected = false;
    context->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
    context->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;

    // When we are doing a remove, we don't actually care what the result is--if we get back an answer, we call
    // the callback.
    if (context->removing) {
        do_callbacks(client, NULL, context->serial, kDNSServiceErr_NoSuchRecord, false);
        goto out;
    }

    // Deal with the response.
    switch (rcode) {
    case dns_rcode_noerror:
        // Remember the server we connected with.  active_update and active_update->server should always be
        // non-NULL here.
        if (client->active_update != NULL) {
            if (client->active_update->server != NULL) {
                // If the new server is not the one that's mentioned in stable_server, then update the one
                // in stable_server.
                if (client->active_update->server->rr.type != client->stable_server.rr.type ||
                    (client->stable_server.rr.type == dns_rrtype_a
                     ? memcmp(&client->stable_server.rr.data, &client->active_update->server->rr.data, 4)
                     : (client->stable_server.rr.type == dns_rrtype_aaaa
                        ? memcmp(&client->stable_server.rr.data, &client->active_update->server->rr.data, 16)
                        : true)) ||
                    memcmp(client->stable_server.port, client->active_update->server->port, 2))
                {
                    memcpy(&client->stable_server, client->active_update->server, sizeof(client->stable_server));
                    client->srp_server_synced = false;
                }
                sync_to_stable_storage(client->active_update);
            }
            client->active_update->interface_serial = interface_serial;
        }

        for (reg_state_t *rp = client->registrations; rp != NULL; rp = rp->next) {
            if (rp->removing) {
                INFO("removal for " PRI_S_SRP "." PRI_S_SRP " completed.", rp->name, rp->regtype);
                do_callbacks(client, rp, context->serial, kDNSServiceErr_NoSuchRecord, false);
                rp->skip = true; // The caller should do DNSServiceRefDeallocate, but if they don't, we don't want
                                 // to either continually send removes, nor to send the update again.
            }
        }

        // Get the renewal time
        // At present, there's no code to actually parse a real DNS packet in the client, so
        // we rely on the server returning just an EDNS0 option; if this assumption fails, we
        // are out of luck.
        if (message->qdcount == 0 && message->ancount == 0 && message->nscount == 0 && ntohs(message->arcount) == 1 &&
            // We expect the edns0 option to be:
            // root label - 1 byte
            // type = 2 bytes
            // class = 2 bytes
            // ttl = 4 bytes
            // rdlength = 2 bytes
            // total of 11 bytes
            // data
            end - p > 11 && // Enough room for an EDNS0 option
            *p == 0 &&       // root label
            p[1] == (dns_rrtype_opt >> 8) && p[2] == (dns_rrtype_opt & 255)) // opt rrtype
        {
            // skip class and ttl, we don't care
            const uint8_t *opt_start = &p[11]; // Start of opt data
            uint16_t opt_len = (((uint16_t)p[9]) << 8) + p[10]; // length of opt data
            const uint8_t *opt_cur = opt_start;
            uint16_t opt_remaining = opt_len;
            // Scan for options until there's no room.
            while (opt_cur + 4 <= end) {
                int option_code = (((uint16_t)opt_cur[0]) << 8) + opt_cur[1];
                int option_len =  (((uint16_t)opt_cur[2]) << 8) + opt_cur[3];
                const uint8_t *option_data = opt_cur + 4;
                if (option_len + option_data <= end) {
                    if (option_code == dns_opt_update_lease) {
                        if (option_len == 8) {
                            new_lease_time = (((uint32_t)option_data[0] << 24) | ((uint32_t)option_data[1] << 16) |
                                              ((uint32_t)option_data[2] << 8) | ((uint32_t)option_data[3]));
                            INFO("Lease time set to %" PRIu32, new_lease_time);
                            lease_time_sent = true;
                        }
                    }
                }
                opt_cur = option_data + option_len;
                opt_remaining = opt_remaining - (option_len + 4);
            }
        }

        if (!lease_time_sent) {
            new_lease_time = context->lease_time;
            INFO("Lease time defaults to %" PRIu32, new_lease_time);
            DEBUG("len %zd qd %d an %d ns %d ar %d data %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                  " %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                  end - p, ntohs(message->qdcount), ntohs(message->ancount),
                  ntohs(message->nscount), ntohs(message->arcount),
                  p[0], p[1], p[2], p[3], p[3], p[5], p[6], p[7], p[8], p[9], p[10], p[11],
                  p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], p[20], p[21], p[22]);
        }

        // Set up to renew.  Time is in milliseconds, and we want to renew at 80% of the lease time.
        srp_set_wakeup(client->os_context, context->udp_context, (new_lease_time * 1000) * 8 / 10, renew_callback);

        do_callbacks(client, NULL, context->serial, kDNSServiceErr_NoError, true);
        break;
    case dns_rcode_yxdomain:
        // Get the actual hostname that we sent.
        if (client->hostname_conflict_callback != NULL && client->hostname != NULL) {
            conflict_hostname = conflict_print(client, NULL, &chosen_hostname, client->hostname);
            client->hostname_conflict_callback(chosen_hostname);
            if (conflict_hostname != NULL) {
                free(conflict_hostname);
            }
        }

        bool resolve_with_callback = false;
        for (registration = client->registrations; registration; registration = registration->next) {
            if (registration->callback != NULL && registration->serial <= context->serial &&
                (registration->flags & kDNSServiceFlagsNoAutoRename))
            {
                resolve_with_callback = true;
            }
        }
        if (resolve_with_callback) {
            do_callbacks(client, NULL, context->serial, kDNSServiceErr_NameConflict, false);
            resolve_name_conflict = false;
        } else {
            resolve_name_conflict = true;
        }

        if (resolve_name_conflict) {
            // If we get a name conflict, try using a low number to rename, but only twice; it's time consuming to do
            // this, so if we get two conflicts, we switch to using a random number.
            if (client->hostname_rename_number < 2) {
                client->hostname_rename_number++;
            } else {
                client->hostname_rename_number = srp_random16();
            }
            // When we get a name conflict response, we need to re-do the update immediately
            // (with a 0-500ms delay of course).
            do_srp_update(client, true, NULL);
            return;
        }
        break;

    default:
        // If we get here, it means that the server failed to process the transmission, and there is no
        // action we can take to change the situation other than trying another server. We set the
        // retransmission time for the current server long enough to force a switch to the next server,
        // if any.
        context->next_retransmission_time = client->srp_max_retry_interval + 1;
        err = srp_set_wakeup(client->os_context, context->udp_context,
                             context->next_retransmission_time - 512 + srp_random16() % 1024, udp_retransmit);
        return;
    }
out:
    if (client->active_update != NULL) {
        client->active_update->serial = client->registration_serial;
    }
}

// Generate a new SRP update message
dns_wire_t *
srp_client_generate_update(client_state_t *client, uint32_t update_lease_time, uint32_t update_key_lease_time,
                           size_t *NONNULL p_length, dns_wire_t *in_wire, uint32_t serial, bool removing)
{
    dns_wire_t *message;
    const char *zone_name = "default.service.arpa";
    const char *service_type = "_ipps._tcp";
    const char *txt_record = "0";
    uint16_t key_tag;
    dns_towire_state_t towire;
    dns_name_pointer_t p_host_name;
    dns_name_pointer_t p_zone_name;
    dns_name_pointer_t p_service_name;
    dns_name_pointer_t p_service_instance_name;
    int line;
    service_addr_t *addr;
    reg_state_t *reg;
    char *conflict_hostname = NULL, *chosen_hostname;

#define INCREMENT(x) (x) = htons(ntohs(x) + 1)
    memset(&towire, 0, sizeof towire);

    // Get the key if we don't already have it.
    if (client->key == NULL) {
        client->key = srp_get_key("com.apple.srp-client.host-key", client->os_context);
        if (client->key == NULL) {
            INFO("No key gotten.");
            return NULL;
        }
    }

#define CH if (towire.error) { line = __LINE__; goto fail; }

    if (client->hostname == NULL) {
        ERROR("called with NULL hostname.");
        return NULL;
    }

    // If we were given a message buffer, use it, otherwise allocate one.
    if (in_wire != NULL) {
        message = in_wire;
        towire.p = &message->data[0];
        towire.lim = towire.p + *p_length;
        towire.message = in_wire;
    } else {
        // Allocate a message buffer.
        message = calloc(1, sizeof *message);
        if (message == NULL) {
            return NULL;
        }
        towire.p = &message->data[0];               // We start storing RR data here.
        towire.lim = &message->data[DNS_DATA_SIZE]; // This is the limit to how much we can store.
        towire.message = message;
    }

    // Generate a random UUID.
    message->id = srp_random16();
    message->bitfield = 0;
    dns_qr_set(message, dns_qr_query);
    dns_opcode_set(message, dns_opcode_update);

    message->qdcount = 0;
    // Copy in Zone name (and save pointer)
    // ZTYPE = SOA
    // ZCLASS = IN
    dns_full_name_to_wire(&p_zone_name, &towire, zone_name); CH;
    dns_u16_to_wire(&towire, dns_rrtype_soa); CH;
    dns_u16_to_wire(&towire, dns_qclass_in); CH;
    INCREMENT(message->qdcount);

    message->ancount = 0;
    // PRCOUNT = 0

    message->nscount = 0;
    // UPCOUNT = ...

    // Host Description:
    //  * Delete all RRsets from <hostname>; remember the pointer to hostname
    //      NAME = hostname label followed by pointer to SOA name.
    //      TYPE = ANY
    //      CLASS = ANY
    //      TTL = 0
    //      RDLENGTH = 0

    conflict_hostname = conflict_print(client, &towire, &chosen_hostname, client->hostname); CH;
    dns_name_to_wire(&p_host_name, &towire, chosen_hostname); CH;
    dns_pointer_to_wire(&p_host_name, &towire, &p_zone_name); CH;
    dns_u16_to_wire(&towire, dns_rrtype_any); CH;
    dns_u16_to_wire(&towire, dns_qclass_any); CH;
    dns_ttl_to_wire(&towire, 0); CH;
    dns_u16_to_wire(&towire, 0); CH;
    INCREMENT(message->nscount);

    //  * Add addresses: A and/or AAAA RRsets, each of which contains one
    //    or more A or AAAA RRs.
    //      NAME = pointer to hostname from Delete (above)
    //      TYPE = A or AAAA
    //      CLASS = IN
    //      TTL = 3600 ?
    //      RDLENGTH = number of RRs * RR length (4 or 16)
    //      RDATA = <the data>
    if (!removing) {
        for (addr = interfaces; addr; addr = addr->next) {
            dns_pointer_to_wire(NULL, &towire, &p_host_name); CH;
            dns_u16_to_wire(&towire, addr->rr.type); CH;
            dns_u16_to_wire(&towire, dns_qclass_in); CH;
            dns_ttl_to_wire(&towire, 3600); CH;
            dns_rdlength_begin(&towire); CH;
            dns_rdata_raw_data_to_wire(&towire, &addr->rr.data,
                                       addr->rr.type == dns_rrtype_a ? 4 : 16); CH;
            dns_rdlength_end(&towire); CH;
            INCREMENT(message->nscount);
        }
   }

    //  * Exactly one KEY RR:
    //      NAME = pointer to hostname from Delete (above)
    //      TYPE = KEY
    //      CLASS = IN
    //      TTL = 3600
    //      RDLENGTH = length of key + 4 (32 bits)
    //      RDATA = <flags(16) = 0000 0010 0000 0001, protocol(8) = 3, algorithm(8) = 8?, public key(variable)>
    dns_pointer_to_wire(NULL, &towire, &p_host_name); CH;
    dns_u16_to_wire(&towire, dns_rrtype_key); CH;
    dns_u16_to_wire(&towire, dns_qclass_in); CH;
    dns_ttl_to_wire(&towire, 3600); CH;
    dns_rdlength_begin(&towire); CH;
    key_tag = dns_rdata_key_to_wire(&towire, 0, 2, 1, client->key); CH;
    dns_rdlength_end(&towire); CH;
    INCREMENT(message->nscount);

    // If we are removing the host, we don't need to send instances.
    if (!removing) {

        // Emit any registrations.
        for (reg = client->registrations; reg; reg = reg->next) {
            // Only remove the registrations that are actually registered. Normally this will be all of them, but it's
            // possible for a registration to be added but not to have been updated yet, and then for us to get a remove
            // call, in which case we don't need to remove it.
            if (((removing || reg->removing) && reg->serial > serial) || reg->skip) {
                continue;
            }

            // Service:
            //   * Update PTR RR
            //     NAME = service name (_a._b.service.arpa)
            //     TYPE = PTR
            //     CLASS = IN
            //     TTL = 3600
            //     RDLENGTH = 2
            //     RDATA = service instance name

            // Service registrations can have subtypes, in which case we need to send multiple PTR records, one for
            // the main type and one for each subtype. Subtypes are represented in the regtype by following the
            // primary service type with subtypes, separated by commas. So we have to parse through that to get
            // the actual domain names to register.
            const char *commap = reg->regtype == NULL ? service_type : reg->regtype;
            dns_name_pointer_t p_sub_service_name;
            bool primary = true;
            do {
                char regtype[DNS_MAX_LABEL_SIZE_ESCAPED + 6]; // plus NUL, ._sub
                int i;
                // Copy the next service type into regtype, ending when we hit the end of reg->regtype
                // or when we hit a comma.
                for (i = 0; *commap != '\0' && *commap != ',' && i < DNS_MAX_LABEL_SIZE_ESCAPED; i++) {
                    regtype[i] = *commap;
                    commap++;
                }

                // If we hit a comma, skip over the comma for the beginning of the next subtype.
                if (*commap == ',') {
                    commap++;
                }

                // If we aren't at a NULL or a comma, it means that the label was too long, so the output
                // is invalid.
                else if (*commap != '\0') {
                    towire.error = ENOBUFS; CH;
                }

                // First time through, it's the base type, so emit the service name and a pointer to the
                // zone name. Other times through, it's a subtype, so the pointer is now to the base type,
                // and since the API makes ._sub implicit, we have to add that.
                if (primary) {
                    regtype[i] = 0;
                    dns_name_to_wire(&p_service_name, &towire, regtype); CH;
                    dns_pointer_to_wire(&p_service_name, &towire, &p_zone_name); CH;
                } else {
                    // Copy in the string and the NUL. We know there's space (see above).
                    memcpy(&regtype[i], "._sub", 6);
                    dns_name_to_wire(&p_sub_service_name, &towire, regtype); CH;
                    dns_pointer_to_wire(&p_sub_service_name, &towire, &p_service_name); CH;
                }
                dns_u16_to_wire(&towire, dns_rrtype_ptr); CH;
                if (reg->removing) {
                    dns_u16_to_wire(&towire, dns_qclass_none); CH;
                    dns_ttl_to_wire(&towire, 0); CH;
                } else {
                    dns_u16_to_wire(&towire, dns_qclass_in); CH;
                    dns_ttl_to_wire(&towire, 3600); CH;
                }
                dns_rdlength_begin(&towire); CH;
                if (reg->name != NULL) {
                    char *service_instance_name, *to_free = conflict_print(client, &towire, &service_instance_name, reg->name);
                    dns_name_to_wire(&p_service_instance_name, &towire, service_instance_name); CH;
                    if (to_free != NULL) {
                        free(to_free);
                    }
                } else {
                    dns_name_to_wire(&p_service_instance_name, &towire, chosen_hostname); CH;
                }
                dns_pointer_to_wire(&p_service_instance_name, &towire, &p_service_name); CH;
                dns_rdlength_end(&towire); CH;
                INCREMENT(message->nscount);
                primary = false;
                // We don't need to remove subtypes: removing the instance removes all its subtypes.
                if (reg->removing) {
                    break;
                }
            } while (*commap != '\0');

            // Service Instance:
            //   * Delete all RRsets from service instance name
            //      NAME = service instance name (save pointer to service name, which is the second label)
            //      TYPE = ANY
            //      CLASS = ANY
            //      TTL = 0
            //      RDLENGTH = 0
            dns_pointer_to_wire(NULL, &towire, &p_service_instance_name); CH;
            dns_u16_to_wire(&towire, dns_rrtype_any); CH;
            dns_u16_to_wire(&towire, dns_qclass_any); CH;
            dns_ttl_to_wire(&towire, 0); CH;
            dns_u16_to_wire(&towire, 0); CH;
            INCREMENT(message->nscount);

            if (!reg->removing) {
                //   * Add one SRV RRset pointing to Host Description
                //      NAME = pointer to service instance name from above
                //      TYPE = SRV
                //      CLASS = IN
                //      TTL = 3600
                //      RDLENGTH = 8
                //      RDATA = <priority(16) = 0, weight(16) = 0, port(16) = service port, target = pointer to hostname>
                dns_pointer_to_wire(NULL, &towire, &p_service_instance_name); CH;
                dns_u16_to_wire(&towire, dns_rrtype_srv); CH;
                dns_u16_to_wire(&towire, dns_qclass_in); CH;
                dns_ttl_to_wire(&towire, 3600); CH;
                dns_rdlength_begin(&towire); CH;
                dns_u16_to_wire(&towire, 0); CH; // priority
                dns_u16_to_wire(&towire, 0); CH; // weight
                dns_u16_to_wire(&towire, reg->port); CH; // port
                dns_pointer_to_wire(NULL, &towire, &p_host_name); CH;
                dns_rdlength_end(&towire); CH;
                INCREMENT(message->nscount);

                //   * Add one or more TXT records
                //      NAME = pointer to service instance name from above
                //      TYPE = TXT
                //      CLASS = IN
                //      TTL = 3600
                //      RDLENGTH = <length of text>
                //      RDATA = <text>
                dns_pointer_to_wire(NULL, &towire, &p_service_instance_name); CH;
                dns_u16_to_wire(&towire, dns_rrtype_txt); CH;
                dns_u16_to_wire(&towire, dns_qclass_in); CH;
                dns_ttl_to_wire(&towire, 3600); CH;
                dns_rdlength_begin(&towire); CH;
                if (reg->txtRecord != NULL) {
                    dns_rdata_raw_data_to_wire(&towire, reg->txtRecord, reg->txtLen);
                } else {
                    dns_rdata_txt_to_wire(&towire, txt_record); CH;
                }
                dns_rdlength_end(&towire); CH;
                INCREMENT(message->nscount);
            }
        }
    }

    // What about services with more than one name?   Are these multiple service descriptions?

    // ARCOUNT = 2
    //   EDNS(0) options
    //     ...
    //   SIG(0)

    message->arcount = 0;
    dns_edns0_header_to_wire(&towire, DNS_MAX_UDP_PAYLOAD, 0, 0, 1); CH; // XRCODE = 0; VERSION = 0; DO=1
    dns_rdlength_begin(&towire); CH;
    dns_u16_to_wire(&towire, dns_opt_update_lease); CH;  // OPTION-CODE
    dns_edns0_option_begin(&towire); CH;                 // OPTION-LENGTH
    if (removing) {
        // If we are removing the record, lease time should be zero. Key_lease_time can be nonzero, but we
        // aren't currently offering a way to do that in the server. Nevertheless, we send a key lease time.
        dns_u32_to_wire(&towire, 0); CH;
        dns_u32_to_wire(&towire, update_key_lease_time); CH;
    } else {
        dns_u32_to_wire(&towire, update_lease_time); CH;     // LEASE (e.g. 1 hour)
        dns_u32_to_wire(&towire, update_key_lease_time); CH; // KEY-LEASE (7 days)
    }
    dns_edns0_option_end(&towire); CH;                   // Now we know OPTION-LENGTH
    dns_rdlength_end(&towire); CH;
    INCREMENT(message->arcount);

    // The signature must be computed before counting the signature RR in the header counts.
    dns_sig0_signature_to_wire(&towire,
                               client->key, key_tag, &p_host_name, chosen_hostname, zone_name, srp_timenow()); CH;
    INCREMENT(message->arcount);
    *p_length = towire.p - (uint8_t *)message;

    if (conflict_hostname != NULL) {
        free(conflict_hostname);
    }
    return message;

fail:
    if (conflict_hostname != NULL) {
        free(conflict_hostname);
    }

    if (towire.error) {
        ERROR("Ran out of message space at srp-client.c:%d (%d, %d)",
              line, towire.line, towire.outer_line);
    }
    if (client->active_update != NULL) {
        update_finalize(client->active_update);
        client->active_update = NULL;
    }
    if (in_wire == NULL && message != NULL) {
        free(message);
    }
    return NULL;
}

// Send SRP updates for host records that have changed.
static int
do_srp_update(client_state_t *client, bool definite, bool *did_something)
{
    int err;
    service_addr_t *server;

    // Cancel any ongoing active update.
    if (!definite && client->active_update != NULL && client->registration_serial == client->active_update->serial) {
        bool server_changed = true;
        for (server = servers; server != NULL; server = server->next) {
            if (server == client->active_update->server) {
                server_changed = false;
            }
        }
        if (client->active_update->interface_serial == interface_serial && !server_changed) {
            INFO("addresses to register are the same; server is the same.");
            return kDNSServiceErr_NoError;
        }
    }

    // At this point we're definitely doing something.
    if (did_something) {
        *did_something = true;
    }

    // Get rid of the previous update, if any.
    if (client->active_update != NULL) {
        update_finalize(client->active_update);
        client->active_update = NULL;
    }

    // Make an update context.
    update_context_t *active_update = calloc(1, sizeof(*active_update));
    if (active_update == NULL) {
        err = kDNSServiceErr_NoMemory;
    } else {
        // If possible, use the server we used last time.
        active_update->client = client;
        sync_from_stable_storage(active_update);
        if (active_update->server == NULL) {
            active_update->server = servers;
        }
        active_update->serial = client->registration_serial;
        active_update->message = NULL;
        active_update->message_length = 0;
        active_update->lease_time = client->lease_time;
        active_update->key_lease_time = client->key_lease_time;
        err = srp_make_udp_context(client->os_context, &active_update->udp_context, udp_response, active_update);

        if (err == kDNSServiceErr_NoError) {
            // XXX use some random jitter on these times.
            active_update->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
            active_update->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;
            err = srp_set_wakeup(client->os_context, active_update->udp_context, srp_random16() % 1023, udp_retransmit);
        }
    }
    client->active_update = active_update;
    return err;
}

// Deregister all existing registrations.
int
srp_deregister(void *os_context)
{
    reg_state_t *rp;
    bool something_to_deregister = false;
    client_state_t *client;

    for (client = clients; client; client = client->next) {
        if (client->os_context == os_context) {
            break;
        }
    }
    if (client == NULL) {
        return kDNSServiceErr_Invalid;
    }

    if (client->active_update == NULL) {
        INFO("no active update.");
        return kDNSServiceErr_NoSuchRecord;
    }

    // See if there are any registrations that have succeeded.
    for (rp = client->registrations; rp; rp = rp->next) {
        if (rp->serial <= client->active_update->serial && rp->succeeded) {
            something_to_deregister = true;
        }
    }

    // If so, start a deregistration update; otherwise return NoSuchRecord.
    if (something_to_deregister) {
        if (client->active_update->message) {
            free(client->active_update->message);
            client->active_update->message = NULL;
        }
        client->active_update->removing = true;
        client->active_update->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
        client->active_update->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;
        udp_retransmit(client->active_update);
        return kDNSServiceErr_NoError;
    } else {
        return kDNSServiceErr_NoSuchRecord;
    }
}

// Deregister a specific registration
int
srp_deregister_instance(DNSServiceRef sdRef)
{
    client_state_t *client;
    reg_state_t *rp;

    // We only expect to find one match.
    for (client = clients; client; client = client->next) {
        for (rp = client->registrations; rp; rp = rp->next) {
            if (rp == sdRef) {
                goto found;
            }
        }
    }
    return kDNSServiceErr_NoSuchRecord;
found:
    rp->removing = true;
    if (client->active_update != NULL) {
        if (client->active_update->message) {
            free(client->active_update->message);
            client->active_update->message = NULL;
        }
        client->active_update->next_retransmission_time = INITIAL_NEXT_RETRANSMISSION_TIME;
        client->active_update->next_attempt_time = INITIAL_NEXT_ATTEMPT_TIME;
        udp_retransmit(client->active_update);
    }
    return kDNSServiceErr_NoError;
}

#ifdef THREAD_DEVKIT_ADK
uint32_t
srp_timenow(void)
{
    return 0;
}
#endif // THREAD_DEVKIT_ADK

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
