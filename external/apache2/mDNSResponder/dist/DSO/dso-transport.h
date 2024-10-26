/* dso-transport.h
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
 */

#ifndef __DSO_TRANSPORT_H
#define __DSO_TRANSPORT_H

#include "mdns_addr_tailq.h"   // For mdns_addr_tailq_t.

#ifdef DSO_USES_NETWORK_FRAMEWORK
#include <Network/Network.h>
#endif

// Maximum number of IP addresses that we'll deal with as a result of looking up a name
// to which to connect.
#define MAX_DSO_CONNECT_ADDRS 16

// Threshold above which we indicate that a DSO connection isn't writable.   This is advisory,
// but e.g. for a Discovery Relay, if the remote proxy isn't consuming what we are sending, we
// should start dropping packets on the floor rather than just queueing more and more packets.
// 60k may actually be too much.   This is used when we're using NW Framework, because it doesn't
// allow us to use TCP_NOTSENT_LOWAT directly.
#define MAX_UNSENT_BYTES 60000

// Use 0 to represent an invalid ID for the object dso_connect_state_t and dso_transport_t.
#define DSO_CONNECT_STATE_INVALID_SERIAL 0
#define DSO_TRANSPORT_INVALID_SERIAL 0

struct dso_transport {
    dso_state_t *dso;              // DSO state for which this is the transport
    struct dso_transport *next;    // Transport is on list of transports.
    void *event_context;           // I/O event context
    mDNSAddr remote_addr;          // The IP address to which we have connected
    int remote_port;               // The port to which we have connected

    uint32_t serial;               // Serial number for locating possibly freed dso_transport_t structs.
#ifdef DSO_USES_NETWORK_FRAMEWORK
    nw_connection_t connection;
    dispatch_data_t to_write;
    size_t bytes_to_write;
    size_t unsent_bytes;
    bool write_failed;             // This is set if any of the parts of the dso_write process fail
#else
    TCPSocket *connection;         // Socket connected to Discovery Proxy
    size_t bytes_needed;
    size_t message_length;         // Length of message we are currently accumulating, if known
    uint8_t *inbuf;                // Buffer for incoming messages.
    size_t inbuf_size;
    uint8_t *inbufp;               // Current read pointer (may not be in inbuf)
    bool need_length;              // True if we need a 2-byte length

    uint8_t lenbuf[2];             // Buffer for storing the length in a DNS TCP message

#define MAX_WRITE_HUNKS 4          // When writing a DSO message, we need this many separate hunks.
    const uint8_t *to_write[MAX_WRITE_HUNKS];
    ssize_t write_lengths[MAX_WRITE_HUNKS];
    int num_to_write;
#endif // DSO_USES_NETWORK_FRAMEWORK

    uint8_t *outbuf;               // Output buffer for building and sending DSO messages
    size_t outbuf_size;
};

typedef struct dso_lookup dso_lookup_t;
struct dso_lookup {
    dso_lookup_t *next;
    DNSServiceRef sdref;
};

typedef struct dso_connect_state dso_connect_state_t;

typedef enum {
    // When the object is created and holds a reference to the context, the callback(see below) is called with
    // dso_connect_life_cycle_create.
    dso_connect_life_cycle_create,
    // When the object is canceled, the callback(see below) is called with dso_connect_life_cycle_cancel to provide a
    // chance for the context to do the corresponding cleaning work(cancel or release/free).
    dso_connect_life_cycle_cancel,
    // When the object is freed, the callback(see below) is called with dso_connect_life_cycle_free to provide a chance
    // for the context to clean anything remains allocated.
    dso_connect_life_cycle_free
} dso_connect_life_cycle_t;

typedef bool (*dso_connect_life_cycle_context_callback_t)(const dso_connect_life_cycle_t life_cycle,
     void *const context, dso_connect_state_t *const dso_connect);

typedef struct dso_transport_address dso_transport_address_t;
struct dso_transport_address {
    dso_transport_address_t *next;
    mDNSAddr address;
    mDNSIPPort port;
};

struct dso_connect_state {
    dso_connect_state_t *next;
    dso_event_callback_t callback;
    dso_state_t *dso;
    char *detail;
    void *context;

    // The callback gets called when dso_state_t is created, canceled or finalized to do some status maintaining
    // operation for the context. This is passed into dso_state_create(), when dso_connect_state_t uses the context to
    // create a new dso_state_t in dso_connection_succeeded().
    dso_life_cycle_context_callback_t dso_context_callback;
    // The callback gets called when dso_connect_t is created, canceled or finalized to do some status maintaining
    // operation for the context.
    dso_connect_life_cycle_context_callback_t dso_connect_context_callback;
    TCPListener *listener;

    uint32_t serial; // Serial number that identifies a specific dso_connect_state_t.
    char *hostname;

    // A list of addresses that we've discovered, and the next address to try.
    dso_transport_address_t *addrs, *next_addr;
    DNSServiceRef lookup;
    mDNSBool canceled; // Indicates if the dso_connect_state_t has been canceled and should be ignored for processing.

    mDNSBool connecting;

#if MDNSRESPONDER_SUPPORTS(APPLE, TERMINUS_ASSISTED_UNICAST_DISCOVERY)
    // A boolean value to indicate whether the dso session should use the preconfigured TLS server certificates to
    // perform the trust evaluation.
    mDNSBool trusts_alternative_server_certificates;
#endif

    mDNSIPPort config_port, connect_port;
    dso_transport_t *transport;
#ifdef DSO_USES_NETWORK_FRAMEWORK
    nw_connection_t connection;
    bool tls_enabled;
#else
    size_t inbuf_size;
#endif
    size_t outbuf_size;
    int max_outstanding_queries;
    mDNSs32 last_event;
    mDNSs32 reconnect_time;
    mDNSu32 if_idx; // The index of the interface where the DSO session is established through.
};

typedef struct {
	TCPListener *listener;
    dso_event_callback_t callback;
	void *context;
} dso_listen_context_t;

typedef struct {
    const uint8_t *message;
    size_t length;
} dso_message_payload_t;

void dso_transport_init(void);
mStatus dso_set_connection(dso_state_t *dso, TCPSocket *socket);
void dso_schedule_reconnect(mDNS *m, dso_connect_state_t *cs, mDNSs32 when);
void dso_set_callback(dso_state_t *dso, void *context, dso_event_callback_t cb);
mStatus dso_message_write(dso_state_t *dso, dso_message_t *msg, bool disregard_low_water);
dso_connect_state_t *dso_connect_state_create(
    const char *hostname, mDNSAddr *addr, mDNSIPPort port,
    int max_outstanding_queries, size_t inbuf_size, size_t outbuf_size,
    dso_event_callback_t callback,
    dso_state_t *dso, void *context,
    const dso_life_cycle_context_callback_t dso_context_callback,
    const dso_connect_life_cycle_context_callback_t dso_connect_context_callback,
    const char *detail);
#ifdef DSO_USES_NETWORK_FRAMEWORK
void dso_connect_state_use_tls(dso_connect_state_t *cs);
#endif
void dso_connect_state_cancel(dso_connect_state_t *const cs);
bool dso_connect(dso_connect_state_t *connect_state);
void dso_reconnect(dso_connect_state_t *cs, dso_state_t *dso);
mStatus dso_listen(dso_connect_state_t *listen_context);
bool dso_write_start(dso_transport_t *transport, size_t length);
bool dso_write_finish(dso_transport_t *transport);
void dso_write(dso_transport_t *transport, const uint8_t *buf, size_t length);
#endif // __DSO_TRANSPORT_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
