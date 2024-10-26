/* srp-api.h
 *
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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
 * Structure definitions for the Service Registration Protocol gateway.
 */

#include "srp.h"
#if defined(THREAD_DEVKIT_ADK) || defined(LINUX)
#include "../mDNSShared/dns_sd.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*srp_hostname_conflict_callback_t)(const char *NONNULL hostname);
typedef void (*srp_wakeup_callback_t)(void *NONNULL state);
typedef void (*srp_datagram_callback_t)(void *NONNULL state, void *NONNULL message, size_t message_length);
typedef struct client_state client_state_t;
typedef struct dns_wire dns_wire_t;

// The below functions provide a way for the host to inform the SRP service of the state of the network.

// For testing
client_state_t *NULLABLE srp_client_get_current(void);
void srp_client_set_current(client_state_t *NONNULL new_client);
dns_wire_t *NULLABLE srp_client_generate_update(client_state_t *NONNULL client,
                                                uint32_t update_lease_time, uint32_t update_key_lease_time,
                                                size_t *NONNULL p_length, dns_wire_t *NULLABLE in_wire,
                                                uint32_t serial, bool removing);
int srp_host_key_reset_for_client(client_state_t *NONNULL client);

// Call this before calling anything else.   Context will be passed back whenever the srp code
// calls any of the host functions.
int srp_host_init(void *NULLABLE host_context);

// Call this to reset the host key (e.g. on factory reset)
int srp_host_key_reset(void);

// This function can be called by accessories that have different requirements for lease intervals.
// Normally new_lease_time would be 3600 (1 hour) and new_key_lease_type would be 604800 (7 days).
int srp_set_lease_times(uint32_t new_lease_time, uint32_t new_key_lease_time);

// Called when a new address is configured that should be advertised.  This can be called during a refresh,
// in which case it doesn't mark the network state as changed if the address was already present.
int srp_add_interface_address(uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen);

// Called whenever the SRP server address changes or the SRP server becomes newly reachable.  This can be
// called during a refresh, in which case it doesn't mark the network state as changed if the address was
// already present.
int srp_add_server_address(const uint8_t *NONNULL port, uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen);

// Called when the node knows its hostname (usually once).   The callback is called if we try to do an SRP
// update and find out that the hostname is in use; in this case, the callback is expected to generate a new
// hostname and re-register it.   It is permitted to call srp_set_hostname() from the callback.
// If the hostname is changed by the callback, then it is used immediately on return from the callback;
// if the hostname is changed in any other situation, nothing is done with the new name until
// srp_network_state_stable() is called.
int srp_set_hostname(const char *NONNULL hostname, srp_hostname_conflict_callback_t NULLABLE callback);

// Called when a network state change is complete (that is, all new addresses have been saved and
// any update to the SRP server address has been provided).   This is only needed when not using the
// refresh mechanism.
int srp_network_state_stable(bool *NULLABLE did_something);

// Delete a previously-configured SRP server address.  This should not be done during a refresh.
int srp_delete_interface_address(uint16_t rrtype, const uint8_t *NONNULL rdata, uint16_t rdlen);

// Delete a previously-configured SRP server address.  This should not be done during a refresh.
int srp_delete_server_address(uint16_t rrtype, const uint8_t *NONNULL port, const uint8_t *NONNULL rdata,
                              uint16_t rdlen);

// Call this to start an address refresh.   This makes sense to do in cases where the caller
// is not tracking changes, but rather is just doing a full refresh whenever the network state
// is seen to have changed.   When the refresh is done, if any addresses were added or removed,
// network_state_changed will be true, and so a call to dnssd_network_state_change_finished()
// will trigger an update; if nothing changed, no update will be sent.
int srp_start_address_refresh(void);

// Call this when the address refresh is done.   This invokes srp_network_state_stable().
int srp_finish_address_refresh(bool *NULLABLE did_something);

// Call this to deregister everything that's currently registered.  A return value other than kDNSServiceErr_NoError
// means that there's nothing to deregister.
int srp_deregister(void *NULLABLE os_context);

// Call this to deregister a specific service instance, identified by the DNSServiceRef.  A return value
// other than kDNSServiceErr_NoError means that the specified service instance wasn't found.
int srp_deregister_instance(DNSServiceRef NULLABLE sdRef);

// Call this to update the service type on an existing registration. This only makes sense for a subtype: if
// this changes the base type, it will look like a new service instance to the SRP server.
DNSServiceErrorType srp_update_service_type(DNSServiceRef NONNULL sdRef, const char *NONNULL regtype, DNSServiceRegisterReply NULLABLE callback, void *NULLABLE context);

// The below functions must be provided by the host.

// This function fetches a key with the specified name for use in signing SRP updates.
// At present, only ECDSA is supported.   If a key with the specified name doesn't exist,
// the host is expected to generate and store it.
srp_key_t *NULLABLE srp_get_key(const char *NONNULL key_name, void *NULLABLE host_context);

// This function clears the key with the specified name.
int srp_reset_key(const char *NONNULL key_name, void *NULLABLE host_context);

// This function fetches the IP address type (rrtype), address (rrdata x rdlength) and port (port[0],
// port[1]) of the most recent server with which the SRP client has successfully registered from stable
// storage.  If the fetch is successful and there was a server recorded in stable storage, it returns true;
// otherwise it returns false. A false status can mean that there's no way to fetch this information, that
// no registration has happened in the past, or that there was some other error accessing stable storage.
bool srp_get_last_server(uint16_t *NONNULL rrtype, uint8_t *NONNULL rrdata, uint16_t rdlength,
                         uint8_t *NONNULL port, void *NULLABLE host_context);

// This function stores the IP address type (rrtype), address (rrdata x rdlength) and port (port[0],
// port[1]) of the most recent server with which the SRP client has successfully registered to stable
// storage.  If the store is successful, it returns true; otherwise it returns false. A false status can
// mean that there's no way to store this information, or that there was an error writing this information
// to stable storage.
bool srp_save_last_server(uint16_t rrtype, uint8_t *NONNULL rrdata, uint16_t rdlength,
                          uint8_t *NONNULL port, void *NULLABLE host_context);

// This is called to create a context for sending and receiving UDP messages to and from a specified
// remote host address and port.  The context passed is to be used whenever the srp host implementation
// does a callback, e.g. when a datagram arrives or when a wakeup occurs (see srp_set_wakeup()).
// The context is not actually connected to a specific address and port until srp_connect_udp() is
// invoked on it.
int srp_make_udp_context(void *NULLABLE host_context, void *NULLABLE *NONNULL p_context,
                         srp_datagram_callback_t NONNULL callback, void *NONNULL context);

// Connect a udp context to a particular destination.  The context has to have already been created by
// srp_make_udp_context().  When packets are received, they will be passed to the callback set
// in srp_make_udp_context().   This must not be called on a context that is already bound to
// some other destination--call srp_disconnect_udp() first if reusing.
int
srp_connect_udp(void *NONNULL context, const uint8_t *NONNULL port, uint16_t address_type,
                const uint8_t *NONNULL address, uint16_t addrlen);

// Disconnect a udp context.  This is used to dissociate from the udp context state that was created
// by a previous call to srp_connect_udp
int
srp_disconnect_udp(void *NONNULL context);

// This gets rid of the UDP context, frees any associated memory, cancels any outstanding wakeups.
// The freeing may occur later than the deactivating, depending on how the underlying event loop
// works.
int srp_deactivate_udp_context(void *NONNULL host_context, void *NONNULL context);

// This is called to send a datagram to a UDP connection.   The UDP connection is identified by the
// anonymous pointer that was returned by srp_make_udp_context().
int srp_send_datagram(void *NULLABLE host_context,
                      void *NONNULL context, void *NONNULL message, size_t message_length);

// This is called with the context returned by srp_make_udp_context.  The caller is expected to schedule
// a wakeup event <milliseconds> in the future, when when that event occurs, it's expected to call the
// callback with the context that was passed to srp_make_udp_context.
int srp_set_wakeup(void *NULLABLE host_context,
                   void *NONNULL context, int milliseconds, srp_wakeup_callback_t NONNULL callback);

// This is called to cancel a wakeup, and should not fail even if there is no wakeup pending.
int srp_cancel_wakeup(void *NULLABLE host_context, void *NONNULL context);

// Returns the current wall clock time in seconds since 1970
uint32_t srp_timenow(void);

#ifdef __cplusplus
} // extern "C"
#endif

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
