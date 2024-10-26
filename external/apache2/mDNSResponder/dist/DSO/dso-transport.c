/* dso-transport.c
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
 */

//*************************************************************************************************************
// Headers

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <netdb.h>              // For gethostbyname().
#include <sys/socket.h>         // For AF_INET, AF_INET6, etc.
#include <net/if.h>             // For IF_NAMESIZE.
#include <netinet/in.h>         // For INADDR_NONE.
#include <arpa/inet.h>          // For inet_addr().
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "dns_sd.h"
#include "DNSCommon.h"
#include "mDNSEmbeddedAPI.h"
#include "dso.h"
#include "dso-transport.h"
#include "DebugServices.h"      // For check_compile_time_code().
#include "mDNSDebug.h"
#include "misc_utilities.h"     // For mDNSAddr_from_sockaddr().

#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
#include "tls-keychain.h"       // For evaluate_tls_cert().
#endif

#ifdef DSO_USES_NETWORK_FRAMEWORK
// Network Framework only works on MacOS X at the moment, and we need the locking primitives for
// MacOSX.
#include "mDNSMacOSX.h"
#endif

#include "mDNSFeatures.h"
#include "mdns_strict.h"

extern mDNS mDNSStorage;

static dso_connect_state_t *dso_connect_states; // DSO connect states that exist.
static dso_transport_t *dso_transport_states; // DSO transport states that exist.
#ifdef DSO_USES_NETWORK_FRAMEWORK
static dispatch_queue_t dso_dispatch_queue;
#else
static void dso_read_callback(TCPSocket *sock, void *context, mDNSBool connection_established,
                       mStatus err);
#endif

static void dso_connect_state_process_address_port_change(const mDNSAddr *addr_changed, mDNSIPPort port,
                                                          bool add, dso_connect_state_t *const cs);
static void dso_connect_internal(dso_connect_state_t *cs, mDNSBool reconnecting);

void
dso_transport_init(void)
{
#ifdef DSO_USES_NETWORK_FRAMEWORK
    // It's conceivable that we might want a separate queue, but we don't know yet, so for
    // now we just use the main dispatch queue, which should be on the main dispatch thread,
    // which is _NOT_ the kevent thread.   So whenever we are doing anything on the dispatch
    // queue (any completion functions for NW framework) we need to acquire the lock before
    // we even look at any variables that could be changed by the other thread.
    dso_dispatch_queue = dispatch_get_main_queue();
#endif
}

#ifdef DSO_USES_NETWORK_FRAMEWORK
static dso_connect_state_t *
dso_connect_state_find(uint32_t serial)
{
    dso_connect_state_t *csp;
    for (csp = dso_connect_states; csp; csp = csp->next) {
        if (csp->serial ==  serial) {
            return csp;
        }
    }
    return NULL;
}
#endif

static void
dso_transport_finalize(dso_transport_t *transport, const char *whence)
{
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSO%u->DSOT%u] dso_transport_t finalizing for " PUB_S " - "
              "transport: %p.", transport->dso != NULL ? transport->dso->serial : DSO_STATE_INVALID_SERIAL,
              transport->serial, whence, transport);

    dso_transport_t **tp = &dso_transport_states;
    for (; *tp != NULL && *tp != transport; tp = &((*tp)->next))
        ;
    if (*tp != NULL) {
        *tp = (*tp)->next;
    } else {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, "dso_transport_t is not in the dso_transport_states list -"
            " transport: %p.", transport);
    }

    if (transport->connection != NULL) {
#ifdef DSO_USES_NETWORK_FRAMEWORK
        MDNS_DISPOSE_NW(transport->connection);
#else
        mDNSPlatformTCPCloseConnection(transport->connection);
        transport->connection = NULL;
#endif
    } else {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "Finalizing a dso_transport_t with no corresponding underlying connection - transport: %p.", transport);
    }

    mdns_free(transport);
}

// dso_connect_state_t objects that have been canceled but aren't yet freed.
static dso_connect_state_t *dso_connect_state_needing_clean_up = NULL;

// We do all of the finalization for the dso state object and any objects it depends on here in the
// dso_idle function because it avoids the possibility that some code on the way out to the event loop
// _after_ the DSO connection has been dropped might still write to the DSO structure or one of the
// dependent structures and corrupt the heap, or indeed in the unlikely event that this memory was
// freed and then reallocated before the exit to the event loop, there could be a bad pointer
// dereference.
//
// If there is a finalize function, that function MUST either free its own state that references the
// DSO state, or else must NULL out the pointer to the DSO state.
int32_t dso_transport_idle(void *context, int32_t now_in, int32_t next_timer_event)
{
    dso_connect_state_t *cs, *cnext;
    mDNS *m = context;
    mDNSs32 now = now_in;
    mDNSs32 next_event = next_timer_event;

    // Clean any remaining dso_connect_state_t objects that have been canceled.
    for (cs = dso_connect_state_needing_clean_up; cs != NULL; cs = cnext) {
        cnext = cs->next;
        if (cs->lookup != NULL) {
            DNSServiceRef ref = cs->lookup;
            cs->lookup = NULL;
            // dso_transport_idle runs under KQueueLoop() which holds a mDNS_Lock already, so directly call
            // DNSServiceRefDeallocate() will grab the lock again. Given that:
            // 1. dso_transport_idle runs under KQueueLoop() that does not traverse any existing mDNSCore structure.
            // 2. The work we do here is cleaning up, not starting a new request.
            // It is "relatively" safe and reasonable to temporarily unlock the mDNSCore lock here.
            mDNS_DropLockBeforeCallback();
            MDNS_DISPOSE_DNS_SERVICE_REF(ref);
            mDNS_ReclaimLockAfterCallback();
        }

        // Remove any remaining unused addresses.
        for (dso_transport_address_t **addrs = &cs->addrs; *addrs != NULL; ) {
            dso_transport_address_t *addr = *addrs;
            *addrs = addr->next;
            mdns_free(addr);
        }

        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] dso_connect_state_t finalizing - "
            "dso_connect: %p, hostname: " PRI_S ", dso_connect->context: %p.", cs->serial, cs, cs->hostname,
            cs->context);
        // If this connect state object is released before we get canceled event for the underlying nw_connection_t,
        // we need to release the reference it holds. The last reference of this nw_connection_t will be released when
        // canceled event is delivered.
    #if defined(DSO_USES_NETWORK_FRAMEWORK)
        MDNS_DISPOSE_NW(cs->connection);
    #endif
        if (cs->dso_connect_context_callback != NULL) {
            cs->dso_connect_context_callback(dso_connect_life_cycle_free, cs->context, cs);
        }
        mDNSPlatformMemFree(cs);
    }
    dso_connect_state_needing_clean_up = NULL;

    // Notice if a DSO connection state is active but hasn't seen activity in a while.
    for (cs = dso_connect_states; cs != NULL; cs = cnext) {
        cnext = cs->next;
        if (!cs->connecting && cs->last_event != 0) {
            mDNSs32 expiry = cs->last_event + 90 * mDNSPlatformOneSecond;
            if (now - expiry > 0) {
                cs->last_event = 0;
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] dso_transport_idle: expiry has happened "
                          ": %p, hostname: " PRI_S ", dso_connect->context: %p; now %d expiry %d last_event %d",
                          cs->serial, cs, cs->hostname, cs->context, now, expiry, cs->last_event);
                cs->callback(cs->context, NULL, cs->dso, kDSOEventType_ConnectFailed);
                // Should not touch the current dso_connect_state_t after we deliver kDSOEventType_ConnectFailed event,
                // because it is possible that the current dso_connect_state_t has been canceled in the callback.
                // Any status update for the canceled dso_connect_state_t will not work as expected.
                continue;
            } else {
                if (next_timer_event - expiry > 0) {
                    next_timer_event = expiry;
                }
            }
        } else if (!cs->connecting && cs->reconnect_time != 0) {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] reconnect time %d "
                      "hostname: " PRI_S ", dso_connect->context: %p.",
                      cs->serial, now - cs->reconnect_time, cs->hostname, cs->context);
            if (now - cs->reconnect_time > 0) {
                cs->reconnect_time = 0; // Don't try to immediately reconnect if it fails.
                // If cs->dso->transport is non-null, we're already connected.
                if (cs->dso && cs->dso->transport == NULL) {
                    cs->callback(cs->context, NULL, cs->dso, kDSOEventType_ShouldReconnect);
                }
                // Should not touch the current dso_connect_state_t after we deliver kDSOEventType_ShouldReconnect event,
                // because it is possible that the current dso_connect_state_t has been canceled in the callback.
                // Any status update for the canceled dso_connect_state_t will not work as expected.
                continue;
            }
        }
        if (cs->reconnect_time != 0 && next_event - cs->reconnect_time > 0) {
            next_event = cs->reconnect_time;
        }
    }

    return next_event;
}

void dso_reconnect(dso_connect_state_t *cs, dso_state_t *dso)
{
    cs->dso = dso;
    dso_connect_internal(cs, mDNStrue);
}

// Call to schedule a reconnect at a later time.
void dso_schedule_reconnect(mDNS *m, dso_connect_state_t *cs, mDNSs32 when)
{
    cs->reconnect_time = when * mDNSPlatformOneSecond + m->timenow;
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] scheduling reconnect in %d (%d %d) seconds, "
              "hostname: " PRI_S ", dso_connect->context: %p.", cs->serial, when,
              m->timenow, cs->reconnect_time, cs->hostname, cs->context);
}

// If a DSO was created by an incoming connection, the creator of the listener can use this function
// to supply context and a callback for future events.
void dso_set_callback(dso_state_t *dso, void *context, dso_event_callback_t cb)
{
    dso->cb = cb;
    dso->context = context;
}

// This is called before writing a DSO message to the output buffer.  length is the length of the message.
// Returns true if we have successfully selected for write (which means that we're under TCP_NOTSENT_LOWAT).
// Otherwise returns false.   It is valid to write even if it returns false, but there is a risk that
// the write will return EWOULDBLOCK, at which point we'd have to blow away the connection.   It is also
// valid to give up at this point and not write a message; as long as dso_write_finish isn't called, a later
// call to dso_write_start will overwrite the length that was stored by the previous invocation.
//
// The circumstance in which this would occur is that we have filled the kernel's TCP output buffer for this
// connection all the way up to TCP_NOTSENT_LOWAT, and then we get a query from the Discovery Proxy to which we
// need to respond.  Because TCP_NOTSENT_LOWAT is fairly low, there should be a lot of room in the TCP output
// buffer for small responses; it would need to be the case that we are getting requests from the proxy at a
// high rate for us to fill the output buffer to the point where a write of a 12-byte response returns
// EWOULDBLOCK; in that case, things are so dysfunctional that killing the connection isn't any worse than
// allowing it to continue.

// An additional note about the motivation for this code: the idea originally was that we'd do scatter/gather
// I/O here: this lets us write everything out in a single sendmsg() call.   This isn't used with the mDNSPlatformTCP
// code because it doesn't support scatter/gather.   Network Framework does, however, and in principle we could
// write to the descriptor directly if that were really needed.

bool dso_write_start(dso_transport_t *transport, size_t in_length)
{
    // The transport doesn't support messages outside of this range.
    if (in_length < 12 || in_length > 65535) {
        return false;
    }

    const uint16_t length = (uint16_t)in_length;

#ifdef DSO_USES_NETWORK_FRAMEWORK
    uint8_t lenbuf[2];

    if (transport->to_write != NULL) {
        nw_release(transport->to_write);
        transport->to_write = NULL;
    }
    lenbuf[0] = length >> 8;
    lenbuf[1] = length & 255;
    transport->to_write = dispatch_data_create(lenbuf, 2, dso_dispatch_queue,
                                               DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    if (transport->to_write == NULL) {
        transport->write_failed = true;
        return false;
    }
    transport->bytes_to_write = length + 2;

    // We don't have access to TCP_NOTSENT_LOWAT, so for now we track how many bytes we've written
    // versus how many bytes that we've written have completed, and if that creeps above MAX_UNSENT_BYTES,
    // we return false here to indicate that there is congestion.
    if (transport->unsent_bytes > MAX_UNSENT_BYTES) {
        return false;
    } else {
        return true;
    }
#else
    transport->lenbuf[0] = length >> 8;
    transport->lenbuf[1] = length & 255;

    transport->to_write[0] = transport->lenbuf;
    transport->write_lengths[0] = 2;
    transport->num_to_write = 1;

    return mDNSPlatformTCPWritable(transport->connection);
#endif // DSO_USES_NETWORK_FRAMEWORK
}

// Called to finish a write (dso_write_start .. dso_write .. [ dso_write ... ] dso_write_finish).  The
// write must completely finish--if we get a partial write, this means that the connection is stalled, and
// so we cancel it.  Since this can call dso_state_cancel, the caller must not reference the DSO state object
// after this call if the return value is false.
bool dso_write_finish(dso_transport_t *transport)
{
#ifdef DSO_USES_NETWORK_FRAMEWORK
    const uint32_t serial = transport->dso->serial;
    const size_t bytes_to_write = transport->bytes_to_write;
    transport->bytes_to_write = 0;
    if (transport->write_failed) {
        dso_state_cancel(transport->dso);
        return false;
    }
    transport->unsent_bytes += bytes_to_write;
    nw_connection_send(transport->connection, transport->to_write, NW_CONNECTION_DEFAULT_STREAM_CONTEXT, true,
        ^(nw_error_t  _Nullable error)
    {
        KQueueLock();
        dso_state_t *const dso = dso_find_by_serial(serial);
        if (error != NULL) {
            const nw_error_t tmp = error;
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSO%u] dso_write_finish: write failed - "
                "error: " PUB_S ".", serial, strerror(nw_error_get_error_code(tmp)));
            if (dso != NULL) {
                dso_state_cancel(dso);
            }
        } else {
            if (dso != NULL && dso->transport != NULL) {
                dso->transport->unsent_bytes -= bytes_to_write;
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSO%u] dso_write_finish: completed - "
                    "bytes written: %zu, bytes outstanding: %zu.", serial, bytes_to_write,
                    dso->transport->unsent_bytes);
            } else {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_WARNING,
                    "[DSO%u] dso_write_finish: completed but the corresponding dso_state_t has been canceled - "
                    "bytes written: %zu.", serial, bytes_to_write);
            }
        }
        KQueueUnlock("dso_write_finish completion routine");
    });
    nw_release(transport->to_write);
    transport->to_write = NULL;
    return true;
#else
    ssize_t result, total = 0;
    int i;

   if (transport->num_to_write > MAX_WRITE_HUNKS) {
        LogMsg("dso_write_finish: fatal internal programming error: called %d times (more than limit of %d)",
               transport->num_to_write, MAX_WRITE_HUNKS);
        dso_state_cancel(transport->dso);
        return false;
    }

    // This is our ersatz scatter/gather I/O.
    for (i = 0; i < transport->num_to_write; i++) {
        result = mDNSPlatformWriteTCP(transport->connection, (const char *)transport->to_write[i], transport->write_lengths[i]);
        if (result != transport->write_lengths[i]) {
            if (result < 0) {
                LogMsg("dso_write_finish: fatal: mDNSPlatformWrite on %s returned %d", transport->dso->remote_name, errno);
            } else {
                LogMsg("dso_write_finish: fatal: mDNSPlatformWrite: short write on %s: %ld < %ld",
                       transport->dso->remote_name, (long)result, (long)total);
            }
            dso_state_cancel(transport->dso);
            return false;
        }
    }
#endif
    return true;
}

// This function may only be called after a previous call to dso_write_start; it records the length of and
// pointer to the write buffer.  These buffers must remain valid until dso_write_finish() is called.  The
// caller is responsible for managing the memory they contain.  The expected control flow for writing is:
// dso_write_start(); dso_write(); dso_write(); dso_write(); dso_write_finished(); There should be one or
// more calls to dso_write; these will ideally be translated into a single scatter/gather sendmsg call (or
// equivalent) to the kernel.
void dso_write(dso_transport_t *transport, const uint8_t *buf, size_t length)
{
    if (length == 0) {
        return;
    }

#ifdef DSO_USES_NETWORK_FRAMEWORK
    if (transport->write_failed) {
        return;
    }
    dispatch_data_t dpd = dispatch_data_create(buf, length, dso_dispatch_queue,
                                               DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    if (dpd == NULL) {
        transport->write_failed = true;
        return;
    }
    if (transport->to_write != NULL) {
        dispatch_data_t dpc = dispatch_data_create_concat(transport->to_write, dpd);
        MDNS_DISPOSE_DISPATCH(dpd);
        MDNS_DISPOSE_DISPATCH(transport->to_write);
        if (dpc == NULL) {
            transport->to_write = NULL;
            transport->write_failed = true;
            return;
        }
        transport->to_write = dpc;
    }
#else
    // We'll report this in dso_write_finish();
    if (transport->num_to_write >= MAX_WRITE_HUNKS) {
        transport->num_to_write++;
        return;
    }

    transport->to_write[transport->num_to_write] = buf;
    transport->write_lengths[transport->num_to_write] = length;
    transport->num_to_write++;
#endif
}

// Write a DSO message
int dso_message_write(dso_state_t *dso, dso_message_t *msg, bool disregard_low_water)
{
    dso_transport_t *transport = dso->transport;
    if (transport == NULL || transport->dso == NULL) {
        return mStatus_BadStateErr;
    }
    if (transport->connection != NULL) {
        if (dso_write_start(transport, dso_message_length(msg)) || disregard_low_water) {
            dso_write(transport, msg->buf, msg->no_copy_bytes_offset);
            dso_write(transport, msg->no_copy_bytes, msg->no_copy_bytes_len);
            dso_write(transport, &msg->buf[msg->no_copy_bytes_offset], msg->cur - msg->no_copy_bytes_offset);
            return dso_write_finish(transport);
        }
    }
    return mStatus_NoMemoryErr;
}

// Replies to some message we were sent with a response code and no data.
// This is a convenience function for replies that do not require that a new
// packet be constructed.   It takes advantage of the fact that the message
// to which this is a reply is still in the input buffer, and modifies that
// message in place to turn it into a response.

bool dso_send_simple_response(dso_state_t *dso, int rcode, const DNSMessageHeader *header, const char *pres)
{
    dso_transport_t *transport = dso->transport;
    (void)pres; // might want this later.
    DNSMessageHeader response = *header;

    // The OPCODE is a 4-bit value in DNS message
    if (rcode < 0 || rcode > 15) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, "[DSO%u]: rcode[%d] is out of range", dso->serial, rcode);
        return false;
    }
    // Just return the message, with no questions, answers, etc.
    response.flags.b[1] = (response.flags.b[1] & ~kDNSFlag1_RC_Mask) | (uint8_t)rcode;
    response.flags.b[0] |= kDNSFlag0_QR_Response;
    response.numQuestions = 0;
    response.numAnswers = 0;
    response.numAuthorities = 0;
    response.numAdditionals = 0;

    // Buffered write back to discovery proxy
    (void)dso_write_start(transport, 12);
    dso_write(transport, (uint8_t *)&response, 12);
    if (!dso_write_finish(transport)) {
        return false;
    }
    return true;
}

// DSO Message we received has a primary TLV that's not implemented.
// XXX is this what we're supposed to do here? check draft.
bool dso_send_not_implemented(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_DSOTypeNI, header, "DSOTYPENI");
}

// Non-DSO message we received is refused.
bool dso_send_refused(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_Refused, header, "REFUSED");
}

bool dso_send_formerr(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_FormErr, header, "FORMERR");
}

bool dso_send_servfail(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_ServFail, header, "SERVFAIL");
}

bool dso_send_name_error(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_NXDomain, header, "NXDOMAIN");
}

bool dso_send_no_error(dso_state_t *dso, const DNSMessageHeader *header)
{
    return dso_send_simple_response(dso, kDNSFlag1_RC_NoErr, header, "NOERROR");
}

#ifdef DSO_USES_NETWORK_FRAMEWORK
static void dso_read_message(dso_transport_t *transport, uint32_t length);

static void dso_read_message_length(dso_transport_t *transport)
{
    if (transport == NULL) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
            "dso_read_message_length: dso_transport_t is NULL while reading message");
        return;
    }

    if (transport->dso == NULL) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
            "dso_read_message_length: transport->dso is NULL while reading message");
        return;
    }

    const uint32_t serial = transport->dso->serial;
    if (transport->connection == NULL) {
        LogMsg("dso_read_message_length called with null connection.");
        return;
    }
    nw_connection_receive(transport->connection, 2, 2,
                          ^(dispatch_data_t content, nw_content_context_t __unused context,
                            bool __unused is_complete, nw_error_t error) {
                              dso_state_t *dso;
                              // Don't touch anything or look at anything until we have the lock.
                              KQueueLock();
                              dso = dso_find_by_serial(serial);
                              if (error != NULL) {
                                  LogMsg("dso_read_message_length: read failed: %s",
                                         strerror(nw_error_get_error_code(error)));
                              fail:
                                  if (dso != NULL) {
                                      mDNS_Lock(&mDNSStorage);
                                      dso_state_cancel(dso);
                                      mDNS_Unlock(&mDNSStorage);
                                  }
                              } else if (content == NULL) {
                                  dso_disconnect_context_t disconnect_context;
                                  if (dso != NULL && transport->dso == dso && transport->dso->cb != NULL) {
                                      LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                                                "dso_read_message_length: remote end closed connection: "
                                                "passing disconnect event to callback.");
                                      memset(&disconnect_context, 0, sizeof disconnect_context);
                                      disconnect_context.reconnect_delay = 1; // reconnect in 1s
                                      mDNS_Lock(&mDNSStorage);
                                      dso->transport = NULL;
                                      nw_connection_cancel(transport->connection);
                                      transport->dso->cb(transport->dso->context, &disconnect_context, transport->dso,
                                                         kDSOEventType_Disconnected);
                                      mDNS_Unlock(&mDNSStorage);
                                  } else if (dso != NULL) {
                                      LogMsg("dso_read_message_length: remote end closed connection: "
                                             "no callback to notify.");
                                      mDNS_Lock(&mDNSStorage);
                                      dso_state_cancel(dso);
                                      mDNS_Unlock(&mDNSStorage);
                                  }
                              } else if (dso != NULL) {
                                  uint32_t length;
                                  size_t length_length;
                                  const uint8_t *lenbuf;
                                  dispatch_data_t map = dispatch_data_create_map(content, (const void **)&lenbuf,
                                                                                 &length_length);
                                  if (map == NULL) {
                                      LogMsg("dso_read_message_length: map create failed");
                                      goto fail;
                                  } else if (length_length != 2) {
                                      LogMsg("dso_read_message_length: invalid length = %d", length_length);
                                      MDNS_DISPOSE_DISPATCH(map);
                                      goto fail;
                                  }
                                  length = ((unsigned)(lenbuf[0]) << 8) | ((unsigned)lenbuf[1]);
                                  MDNS_DISPOSE_DISPATCH(map);
                                  dso_read_message(transport, length);
                              } else {
                                  LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_WARNING,
                                      "[DSO%u] dso_read_message_length: the corresponding dso_state_t has been canceled.",
                                      serial);
                              }
                              KQueueUnlock("dso_read_message_length completion routine");
                          });
}

void dso_read_message(dso_transport_t *transport, uint32_t length)
{
    if (transport == NULL) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
            "dso_read_message: dso_transport_t is NULL while reading message");
        return;
    }
    if (transport->dso == NULL) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
            "dso_read_message: transport->dso is NULL while reading message");
        return;
    }

    const uint32_t serial = transport->dso->serial;
    if (transport->connection == NULL) {
        LogMsg("dso_read_message called with null connection.");
        return;
    }
    nw_connection_receive(transport->connection, length, length,
                          ^(dispatch_data_t content, nw_content_context_t __unused context,
                            bool __unused is_complete, nw_error_t error) {
                              dso_state_t *dso;
                              // Don't touch anything or look at anything until we have the lock.
                              KQueueLock();
                              dso = dso_find_by_serial(serial);
                              if (error != NULL) {
                                  LogMsg("dso_read_message: read failed: %s", strerror(nw_error_get_error_code(error)));
                              fail:
                                  if (dso != NULL) {
                                      mDNS_Lock(&mDNSStorage);
                                      dso_state_cancel(dso);
                                      mDNS_Unlock(&mDNSStorage);
                                  }
                              } else if (content == NULL) {
                                  dso_disconnect_context_t disconnect_context;
                                  if (dso != NULL && transport->dso == dso && transport->dso->cb != NULL) {
                                      LogMsg("dso_read_message: remote end closed connection: "
                                             "passing disconnect event to callback.");
                                      memset(&disconnect_context, 0, sizeof disconnect_context);
                                      disconnect_context.reconnect_delay = 1; // reconnect in 1s
                                      mDNS_Lock(&mDNSStorage);
                                      dso->transport = NULL;
                                      nw_connection_cancel(transport->connection);
                                      transport->dso->cb(transport->dso->context, &disconnect_context, transport->dso,
                                                         kDSOEventType_Disconnected);
                                      mDNS_Unlock(&mDNSStorage);
                                  } else if (dso != NULL) {
                                      LogMsg("dso_read_message: remote end closed connection: "
                                             "no callback to notify.");
                                      mDNS_Lock(&mDNSStorage);
                                      dso_state_cancel(dso);
                                      mDNS_Unlock(&mDNSStorage);
                                  }
                              } else if (dso != NULL) {
                                  dso_message_payload_t message;
                                  dispatch_data_t map = dispatch_data_create_map(content,
                                                                                 (const void **)&message.message, &message.length);
                                  if (map == NULL) {
                                      LogMsg("dso_read_message_length: map create failed");
                                      goto fail;
                                  } else if (message.length != length) {
                                      LogMsg("dso_read_message_length: only %d of %d bytes read", message.length, length);
                                      MDNS_DISPOSE_DISPATCH(map);
                                      goto fail;
                                  }
                                  // Process the message.
                                  mDNS_Lock(&mDNSStorage);
                                  dns_message_received(dso, message.message, message.length, &message);
                                  mDNS_Unlock(&mDNSStorage);

                                  // Release the map object now that we no longer need its buffers.
                                  MDNS_DISPOSE_DISPATCH(map);

                                  // Now read the next message length.
                                  dso_read_message_length(transport);
                              } else {
                                  LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_WARNING,
                                      "[DSO%u] dso_read_message: the corresponding dso_state_t has been canceled.",
                                      serial);
                              }
                              KQueueUnlock("dso_read_message completion routine");
                          });
}
#else
// Called whenever there's data available on a DSO connection
void dso_read_callback(TCPSocket *sock, void *context, mDNSBool connection_established, int err)
{
    dso_transport_t *transport = context;
    dso_state_t *dso;
    mDNSBool closed = mDNSfalse;

    mDNS_Lock(&mDNSStorage);
    dso = transport->dso;

    // This shouldn't ever happen.
    if (err) {
        LogMsg("dso_read_callback: error %d", err);
        dso_state_cancel(dso);
        goto out;
    }

    // Connection is already established by the time we set this up.
    if (connection_established) {
        goto out;
    }

    // This will be true either if we have never read a message or
    // if the last thing we did was to finish reading a message and
    // process it.
    if (transport->message_length == 0) {
        transport->need_length = true;
        transport->inbufp = transport->inbuf;
        transport->bytes_needed = 2;
    }

    // Read up to bytes_needed bytes.
    ssize_t count = mDNSPlatformReadTCP(sock, transport->inbufp, transport->bytes_needed, &closed);
    // LogMsg("read(%d, %p:%p, %d) -> %d", fd, dso->inbuf, dso->inbufp, dso->bytes_needed, count);
    if (count < 0) {
        LogMsg("dso_read_callback: read from %s returned %d", dso->remote_name, errno);
        dso_state_cancel(dso);
        goto out;
    }

    // If we get selected for read and there's nothing to read, the remote end has closed the
    // connection.
    if (closed) {
        LogMsg("dso_read_callback: remote %s closed", dso->remote_name);
        dso_state_cancel(dso);
        goto out;
    }

    transport->inbufp += count;
    transport->bytes_needed -= count;

    // If we read all the bytes we wanted, do what's next.
    if (transport->bytes_needed == 0) {
        // We just finished reading the complete length of a DNS-over-TCP message.
        if (transport->need_length) {
            // Get the number of bytes in this DNS message
            size_t bytes_needed = (((size_t)transport->inbuf[0]) << 8) | transport->inbuf[1];

            // Under no circumstances can length be zero.
            if (bytes_needed == 0) {
                LogMsg("dso_read_callback: %s sent zero-length message.", dso->remote_name);
                dso_state_cancel(dso);
                goto out;
            }

            // The input buffer size is AbsoluteMaxDNSMessageData, which is around 9000 bytes on
            // big platforms and around 1500 bytes on smaller ones.   If the remote end has sent
            // something larger than that, it's an error from which we can't recover.
            if (bytes_needed > transport->inbuf_size - 2) {
                LogMsg("dso_read_callback: fatal: Proxy at %s sent a too-long (%zd bytes) message",
                       dso->remote_name, bytes_needed);
                dso_state_cancel(dso);
                goto out;
            }

            transport->message_length = bytes_needed;
            transport->bytes_needed = bytes_needed;
            transport->inbufp = transport->inbuf + 2;
            transport->need_length = false;

        // We just finished reading a complete DNS-over-TCP message.
        } else {
            dso_message_payload_t message = { &transport->inbuf[2], transport->message_length };
            dns_message_received(dso, message.message, message.length, &message);
            transport->message_length = 0;
        }
    }
out:
    mDNS_Unlock(&mDNSStorage);
}
#endif // DSO_USES_NETWORK_FRAMEWORK

#ifdef DSO_USES_NETWORK_FRAMEWORK
static dso_transport_t *dso_transport_create(nw_connection_t connection, bool is_server, void *context,
    const dso_life_cycle_context_callback_t context_callback, int max_outstanding_queries, size_t outbuf_size_in,
    const char *remote_name, dso_event_callback_t cb, dso_state_t *dso)
{
    dso_transport_t *transport;
    uint8_t *transp;
    const size_t outbuf_size = outbuf_size_in + 256; // Space for additional TLVs

    // We allocate everything in a single hunk so that we can free it together as well.
    transp = mallocL("dso_transport_create", (sizeof *transport) + outbuf_size);
    if (transp == NULL) {
        transport = NULL;
        goto out;
    }
    // Don't clear the buffers.
    mDNSPlatformMemZero(transp, sizeof (*transport));

    transport = (dso_transport_t *)transp;
    transp += sizeof *transport;

    transport->outbuf = transp;
    transport->outbuf_size = outbuf_size;

    if (dso == NULL) {
        transport->dso = dso_state_create(is_server, max_outstanding_queries, remote_name, cb, context, context_callback,
                                          transport);
        if (transport->dso == NULL) {
            mDNSPlatformMemFree(transport);
            transport = NULL;
            goto out;
        }
    } else {
        transport->dso = dso;
    }
    transport->connection = connection;
    nw_retain(transport->connection);

    // Used to uniquely mark dso_transport_t objects, incremented once for each dso_transport_t created.
    // DSO_TRANSPORT_INVALID_SERIAL(0) is used to identify the invalid dso_transport_t.
    static uint32_t dso_transport_serial = DSO_TRANSPORT_INVALID_SERIAL + 1;
    transport->serial = dso_transport_serial++;

    transport->dso->transport = transport;
    transport->dso->transport_finalize = dso_transport_finalize;
    transport->next = dso_transport_states;
    dso_transport_states = transport;

    // Start looking for messages...
    dso_read_message_length(transport);
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSO%u->DSOT%u] New dso_transport_t created - "
        "transport: %p, remote_name: " PRI_S ".", transport->dso->serial, transport->serial, transport, remote_name);
out:
    return transport;
}
#else
// Create a dso_transport_t structure
static dso_transport_t *dso_transport_create(TCPSocket *sock, bool is_server, void *context,
    const dso_life_cycle_context_callback_t context_callback, int max_outstanding_queries, size_t inbuf_size_in,
    size_t outbuf_size_in, const char *remote_name, dso_event_callback_t cb, dso_state_t *dso)
{
    dso_transport_t *transport;
    size_t outbuf_size;
    size_t inbuf_size;
    uint8_t *transp;
    int status;

    // There's no point in a DSO that doesn't have a callback.
    if (!cb) {
        return NULL;
    }

    outbuf_size = outbuf_size_in + 256; // Space for additional TLVs
    inbuf_size = inbuf_size_in + 2;   // Space for length

    // We allocate everything in a single hunk so that we can free it together as well.
    transp = mallocL("dso_transport_create", (sizeof *transport) + inbuf_size + outbuf_size);
    if (transp == NULL) {
        transport = NULL;
        goto out;
    }
    // Don't clear the buffers.
    mDNSPlatformMemZero(transp, sizeof (*transport));

    transport = (dso_transport_t *)transp;
    transp += sizeof *transport;

    transport->inbuf = transp;
    transport->inbuf_size = inbuf_size;
    transp += inbuf_size;

    transport->outbuf = transp;
    transport->outbuf_size = outbuf_size;

    if (dso == NULL) {
        transport->dso = dso_state_create(is_server, max_outstanding_queries, remote_name, cb, context, context_callback,
                                          transport);
        if (transport->dso == NULL) {
            mDNSPlatformMemFree(transport);
            transport = NULL;
            goto out;
        }
    } else {
        transport->dso = dso;
    }
    transport->connection = sock;

    // Used to uniquely mark dso_transport_t objects, incremented once for each dso_transport_t created.
    // DSO_TRANSPORT_INVALID_SERIAL(0) is used to identify the invalid dso_transport_t.
    static uint32_t dso_transport_serial = DSO_TRANSPORT_INVALID_SERIAL + 1;
    transport->serial = dso_transport_serial++;

    status = mDNSPlatformTCPSocketSetCallback(sock, dso_read_callback, transport);
    if (status != mStatus_NoError) {
        LogMsg("dso_state_create: unable to set callback: %d", status);
        dso_state_cancel(transport->dso);
        goto out;
    }

    transport->dso->transport = transport;
    transport->dso->transport_finalize = dso_transport_finalize;
    transport->next = dso_transport_states;
    dso_transport_states = transport;
out:
    return transport;
}
#endif // DSO_USES_NETWORK_FRAMEWORK

// This should all be replaced with Network Framework connection setup.
dso_connect_state_t *dso_connect_state_create(
    const char *hostname, mDNSAddr *addr, mDNSIPPort port,
    int max_outstanding_queries, size_t inbuf_size, size_t outbuf_size,
    dso_event_callback_t callback,
    dso_state_t *dso, void *context,
    const dso_life_cycle_context_callback_t dso_context_callback,
    const dso_connect_life_cycle_context_callback_t dso_connect_context_callback,
    const char *detail)
{
    size_t detlen = strlen(detail) + 1;
    size_t hostlen = hostname == NULL ? 0 : strlen(hostname) + 1;
    size_t len;
    dso_connect_state_t *cs = NULL;
    dso_connect_state_t *cs_to_return = NULL;
    char *csp;
    char nbuf[INET6_ADDRSTRLEN + 1];
    dso_connect_state_t **states;

    // Enforce Some Minimums (Xxx these are a bit arbitrary, maybe not worth doing?)
    if (inbuf_size < MaximumRDSize || outbuf_size < 128 || max_outstanding_queries < 1) {
        goto exit;
    }

    // If we didn't get a hostname, make a presentation form of the IP address to use instead.
    if (!hostlen) {
        if (addr != NULL) {
            if (addr->type == mDNSAddrType_IPv4) {
                hostname = inet_ntop(AF_INET, &addr->ip.v4, nbuf, sizeof nbuf);
            } else {
                hostname = inet_ntop(AF_INET6, &addr->ip.v6, nbuf, sizeof nbuf);
            }
            if (hostname != NULL) {
                hostlen = strlen(nbuf);
            }
        }
    }
    // If we don't have a printable name, we won't proceed, because this means we don't know
    // what to connect to.
    if (!hostlen) {
        goto exit;
    }

    len = (sizeof *cs) + detlen + hostlen;
    csp = mdns_malloc(len);
    if (!csp) {
        goto exit;
    }
    cs = (dso_connect_state_t *)csp;
    memset(cs, 0, sizeof *cs);
    csp += sizeof *cs;

    cs->detail = csp;
    memcpy(cs->detail, detail, detlen);
    csp += detlen;
    cs->hostname = csp;
    memcpy(cs->hostname, hostname, hostlen);

    // Used to uniquely mark dso_connect_state_t objects, incremented once for each dso_connect_state_t created.
    // DSO_CONNECT_STATE_INVALID_SERIAL(0) is used to identify the invalid dso_connect_state_t.
    static uint32_t dso_connect_state_serial = DSO_CONNECT_STATE_INVALID_SERIAL + 1;
    cs->serial = dso_connect_state_serial++;

    cs->config_port = port;
    cs->max_outstanding_queries = max_outstanding_queries;
    cs->outbuf_size = outbuf_size;
    if (context) {
        cs->context = context;
    } // else cs->context = NULL because of memset call above.
    if (dso_context_callback != NULL) {
        cs->dso_context_callback = dso_context_callback;
    }
    if (dso_connect_context_callback != NULL) {
        cs->dso_connect_context_callback = dso_connect_context_callback;
        dso_connect_context_callback(dso_connect_life_cycle_create, context, cs);
    }
    cs->callback = callback;
    cs->connect_port.NotAnInteger = 0;
    cs->dso = dso;
#ifndef DSO_USES_NETWORK_FRAMEWORK
    cs->inbuf_size = inbuf_size;
#endif

    if (addr != NULL) {
        dso_connect_state_process_address_port_change(addr, port, mDNStrue, cs);
        if (cs->addrs == NULL) {
            goto exit;
        }
    }

    // cs->canceled must be set to false here, because we use it to determine if the current dso_connect_state_t is
    // still valid. We do not need to set it explicitly because the memset(cs, 0, sizeof *cs); above will initialize it
    // to 0(false).
    // cs->canceled = mDNSfalse;

    for (states = &dso_connect_states; *states != NULL; states = &(*states)->next)
        ;
    *states = cs;

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSO%u->DSOC%u] New dso_connect_state_t created - "
        "dso_connect: %p, hostname: " PUB_S ", context: %p.", dso->serial, cs->serial, cs, hostname, context);

    cs_to_return = cs;
    cs = NULL;

exit:
    if (cs != NULL) {
        mdns_free(cs->addrs);
    }
    mdns_free(cs);
    return cs_to_return;
}

#ifdef DSO_USES_NETWORK_FRAMEWORK
void dso_connect_state_use_tls(dso_connect_state_t *cs)
{
    cs->tls_enabled = true;
}
#endif

void
dso_connect_state_cancel(dso_connect_state_t *const cs)
{
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] Canceling dso_connect_state_t.", cs->serial);

    // Remove the dso_connect_state_t from the main dso_connect_states list.
    dso_connect_state_t **cs_pp;
    for (cs_pp = &dso_connect_states; *cs_pp != NULL && *cs_pp != cs; cs_pp = &(*cs_pp)->next)
        ;
    if (*cs_pp != NULL) {
        *cs_pp = cs->next;
    } else {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
            "[DSOC%u] Canceling a dso_connect_state_t that is not in the dso_connect_states list - host name: " PRI_S
            ", detail: " PUB_S ".", cs->serial, cs->hostname, cs->detail);
    }

#ifdef DSO_USES_NETWORK_FRAMEWORK
    // Cancel the underlying nw_connection_t.
    if (cs->connection != NULL) {
        nw_connection_cancel(cs->connection);
    }
    if (cs->transport != NULL && cs->transport->connection != NULL) {
        nw_connection_cancel(cs->transport->connection);
    }
#endif

    // We cannot call `DNSServiceRefDeallocate(cs->lookup);` directly to cancel the address lookup, because we are
    // holding the mDNS_Lock when calling the function. And it is also possible that we are traversing some mDNSCore
    // data structures while calling it, so use mDNS_DropLockBeforeCallback is not correct either.

    if (cs->dso_connect_context_callback != NULL) {
        cs->dso_connect_context_callback(dso_connect_life_cycle_cancel, cs->context, cs);
    }

    // Invalidate this dso_connect_state_t object, so that when we get a callback for dso_inaddr_callback(), we can skip
    // the callback for the canceled dso_connect_state_t object.
    cs->canceled = mDNStrue;

    // We leave cs and cs->lookup to be freed by dso_transport_idle.
    cs->next = dso_connect_state_needing_clean_up;
    dso_connect_state_needing_clean_up = cs;
}

#ifdef DSO_USES_NETWORK_FRAMEWORK
static void
dso_connection_succeeded(dso_connect_state_t *cs)
{
    // We got a connection.
    dso_transport_t *transport =
        dso_transport_create(cs->connection, false, cs->context, cs->dso_context_callback,
            cs->max_outstanding_queries, cs->outbuf_size, cs->hostname, cs->callback, cs->dso);
    if (transport == NULL) {
        // If dso_transport_create fails, there's no point in continuing to try to connect to new
        // addresses
        LogMsg("dso_connection_succeeded: dso_state_create failed");
        // XXX we didn't retain the connection, so we're done when it goes out of scope, right?
    } else {
        // Call the "we're connected" callback, which will start things up.
        transport->dso->cb(cs->context, NULL, transport->dso, kDSOEventType_Connected);
    }

    cs->last_event = 0;
    // Remember the transport on the connect state so that we can cancel it when needed.
    cs->transport = transport;
    return;
}

static bool tls_cert_verify(const sec_protocol_metadata_t metadata, const sec_trust_t trust_ref,
    const bool trusts_alternative_trusted_server_certificates, const uint32_t cs_serial)
{
    bool valid;

    // When iCloud keychain is enabled, it is possible that the TLS certificate that DNS push server
    // uses has been synced to the client device, so we do the evaluation here.
    const tls_keychain_context_t context = {metadata, trust_ref, trusts_alternative_trusted_server_certificates};
    valid = tls_cert_evaluate(&context);
    if (valid) {
        // If the evaluation succeeds, it means that the DNS push server that mDNSResponder is
        // talking to, is registered under the same iCloud account. Therefore, it is trustworthy.
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                  "[DSOC%u] TLS certificate evaluation SUCCEEDS, the DNS push server is trustworthy.",
                  cs_serial);
    } else {
        // If the evaluation fails, it means that, the DNS push server that mDNSResponder is
        // talking to, is not registered under the same iCloud account or the user does not enable iCloud Keychain.
        // Case 1: The DNS push server is not owned by the user making the request. For example,
        // a user goes to other's home and trying to discover the services there.
        // Case 2: The DNS push server is owned by the client, but does not enable iCloud Keychain.
        // Case 3: The DNS push server is a malicious server.
        // Case 4: The user does not enable iCloud Keychain, thus the TLS certificate on the client may be out of date,
        // or not available due to no certificate syncing.
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                  "[DSOC%u] TLS certificate evaluation FAILS, the DNS push server is not trustworthy.",
                  cs_serial);
    }

    return valid;
}

static void dso_connect_internal(dso_connect_state_t *cs, mDNSBool reconnecting)
{
    uint32_t serial = cs->serial;
    nw_endpoint_t endpoint = NULL;
    nw_parameters_t parameters = NULL;
    nw_connection_t connection = NULL;

    cs->last_event = mDNSStorage.timenow;

    // If we do not have more address to connect.
    if (cs->next_addr == NULL) {
        if (reconnecting) {
            if (cs->addrs != NULL) {
                dso_disconnect_context_t context;
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "[DSOC%u] At end of address list, delaying retry - "
                          "server name: " PRI_S ".", cs->serial, cs->hostname);
                cs->last_event = 0;
                context.reconnect_delay = 60; // Wait one minute before attempting to reconnect.
                cs->callback(cs->context, &context, cs->dso, kDSOEventType_Disconnected);
                cs->next_addr = cs->addrs;
            } else {
                // Otherwise, we will get more callbacks when outstanding queries either fail or succeed.
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                          "[DSOC%u] Waiting for newly resolved address to connect - server name: " PRI_S ".",
                          cs->serial, cs->hostname);
            }
        } else {
            // The expectation is that if we are connecting to a DSO server, we really should succeed. If we
            // don't succeed in connecting to any of the advertised servers, it's a good assumption that it's
            // not working, so we should give up, rather than continuing forever to try.
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "[DSOC%u] No more addresses to try - "
                      "server name: " PRI_S ".", cs->serial, cs->hostname);
            cs->last_event = 0;
            cs->callback(cs->context, NULL, cs->dso, kDSOEventType_ConnectFailed);
        }
        goto exit;
    }

    // Always use the first address in the list to set up the connection.
    const dso_transport_address_t *dest_addr = cs->next_addr;

    const mDNSAddr addr = dest_addr->address;
    const mDNSIPPort port = dest_addr->port;

    char addrbuf[INET6_ADDRSTRLEN + 1];
    char portbuf[6];
    get_address_string_from_mDNSAddr(&addr, addrbuf);
    snprintf(portbuf, sizeof(portbuf), "%u", mDNSVal16(port));

    endpoint = nw_endpoint_create_host(addrbuf, portbuf);
    if (endpoint == NULL) {
        goto exit;
    }

    nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;
    if (cs->tls_enabled) {
        const uint32_t cs_serial = cs->serial;
        bool trusts_alternative_server_certificates = false;
    #if MDNSRESPONDER_SUPPORTS(APPLE, TERMINUS_ASSISTED_UNICAST_DISCOVERY)
        trusts_alternative_server_certificates = cs->trusts_alternative_server_certificates;
    #endif
        configure_tls = ^(nw_protocol_options_t tls_options) {
            sec_protocol_options_t sec_options = nw_tls_copy_sec_protocol_options(tls_options);
            sec_protocol_options_set_verify_block(sec_options,
                ^(sec_protocol_metadata_t metadata, sec_trust_t trust_ref, sec_protocol_verify_complete_t complete)
                {
                    const bool valid = tls_cert_verify(metadata, trust_ref, trusts_alternative_server_certificates,
                        cs_serial);
                    complete(valid);
                },
                dso_dispatch_queue);
            sec_release(sec_options);
        };
    }
    parameters = nw_parameters_create_secure_tcp(configure_tls, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    if (parameters == NULL) {
        goto exit;
    }

    // connection now holds a reference to the nw_connection.
    // It holds the reference during the entire life time of the nw_connection_t, until it is canceled.
    connection = nw_connection_create(endpoint, parameters);
    if (connection == NULL) {
        goto exit;
    }

    const uint64_t nw_connection_id = nw_connection_get_id(connection);

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u->C%" PRIu64 "] Connecting to the server - "
        "server: " PRI_IP_ADDR ":%u.", cs->serial, nw_connection_id, &addr, mDNSVal16(port));

    nw_connection_set_queue(connection, dso_dispatch_queue);
    nw_connection_set_state_changed_handler(
        connection, ^(nw_connection_state_t state, nw_error_t error) {
            dso_connect_state_t *ncs;
            KQueueLock();
            ncs = dso_connect_state_find(serial); // Might have been canceled.
            if (ncs == NULL) {
                // If we cannot find dso_connect_state_t in the system's list, it means that we have already canceled it
                // in dso_connect_state_cancel() including the corresponding nw_connection_t. Therefore, there is no
                // need to cancel the nw_connection_t again.
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u->C%" PRIu64
                          "] No connect state found - nw_connection_state_t: " PUB_S ".",
                          serial, nw_connection_id, nw_connection_state_to_string(state));
            } else {
                if (state == nw_connection_state_waiting) {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                              "[DSOC%u->C%" PRIu64 "] Connection to server: " PRI_IP_ADDR ":%u waiting.",
                              serial, nw_connection_id, &addr, mDNSVal16(port));

                    // XXX the right way to do this is to just let NW Framework wait until we get a connection,
                    // but there are a bunch of problems with that right now.   First, will we get "waiting" on
                    // every connection we try?   We aren't relying on NW Framework for DNS lookups, so we are
                    // connecting to an IP address, not a host, which means in principle that a later IP address
                    // might be reachable.   So we have to stop trying on this one to try that one.   Oops.
                    // Once we get NW Framework to use internal calls to resolve names, we can fix this.
                    // Second, maybe we want to switch to polling if this happens.   Probably not, but we need
                    // to think this through.   So right now we're just using the semantics of regular sockets,
                    // which we /have/ thought through.   So in the future we should do this think-through and
                    // try to use NW Framework as it's intended to work rather than as if it were just sockets.
                    nw_connection_cancel(connection);
                } else if (state == nw_connection_state_failed) {
                    // We tried to connect, but didn't succeed.
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                              "[DSOC%u->C%" PRIu64 "] Connection to server: " PRI_IP_ADDR ":%u failed, error:"
                              PUB_S ", detail: " PUB_S ".", serial, nw_connection_id,
                              &addr, mDNSVal16(port), strerror(nw_error_get_error_code(error)), ncs->detail);
                    nw_connection_cancel(connection);
                } else if (state == nw_connection_state_ready) {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                        "[DSOC%u->C%" PRIu64 "] Connection to server: " PRI_IP_ADDR ":%u ready.",
                        serial, nw_connection_id, &addr, mDNSVal16(port));

                    // Get the interface index from the path.
                    nw_path_t curr_path = nw_connection_copy_current_path(connection);
                    if (curr_path) {
                        ncs->if_idx = nw_path_get_interface_index(curr_path);
                    }
                    MDNS_DISPOSE_NW(curr_path);

                    ncs->connecting = mDNSfalse;
                    mDNS_Lock(&mDNSStorage);
                    dso_connection_succeeded(ncs);
                    mDNS_Unlock(&mDNSStorage);
                } else if (state == nw_connection_state_cancelled) {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                        "[DSOC%u->C%" PRIu64 "] Connection to server: " PRI_IP_ADDR ":%u canceled.",
                        serial, nw_connection_id, &addr, mDNSVal16(port));
                    if (ncs->transport) {
                        MDNS_DISPOSE_NW(ncs->transport->connection);
                        // If there is a dso state on the connect state and it is referencing this transport,
                        // remove the reference.
                        if (ncs->dso != NULL && ncs->dso->transport == ncs->transport) {
                            ncs->dso->transport = NULL;
                        }
                        // If the dso_state_t is still referencing this transport, remove the reference.
                        if (ncs->transport->dso != NULL && ncs->transport->dso->transport == ncs->transport) {
                            ncs->transport->dso->transport = NULL;
                        }
                        dso_transport_finalize(ncs->transport, "dso_connect_internal");
                        ncs->transport = NULL;
                    }
                    MDNS_DISPOSE_NW(ncs->connection);
                    if (ncs->connecting) {
                        ncs->connecting = mDNSfalse;
                        // If we get here and cs exists, we are still trying to connect.   So do the next step.
                        mDNS_Lock(&mDNSStorage);
                        dso_connect_internal(ncs, reconnecting);
                        mDNS_Unlock(&mDNSStorage);
                    }
                }

                // Except for the state nw_connection_state_ready, all the other states mean that the nw_connection is
                // not ready for use. Therefore, it is no longer correct to say that we have an established session.
                // In which case, set has_session to false.
                if (state != nw_connection_state_ready) {
                    if (ncs->dso != NULL) {
                        ncs->dso->has_session = false;
                    }
                }
            }

            // Release the nw_connection_t reference held by `connection`, the nw_release here always releases the last
            // reference we have for the nw_connection_t.
            if ((state == nw_connection_state_cancelled) && connection) {
                nw_release(connection);
            }

            KQueueUnlock("dso_connect_internal state change handler");
        });
    nw_connection_start(connection);
    cs->connecting = mDNStrue;

    // Connect state now also holds a reference to the nw_connection.
    cs->connection = connection;
    nw_retain(cs->connection);

    // We finished setting up the connection with the first address in the list, so remove it from the list.
    cs->next_addr = dest_addr->next;
    dest_addr = NULL;
exit:
    MDNS_DISPOSE_NW(endpoint);
    MDNS_DISPOSE_NW(parameters);
}

#else
static void dso_connect_callback(TCPSocket *sock, void *context, mDNSBool connected, int err)
{
    dso_connect_state_t *cs = context;
    char *detail;
    int status;
    dso_transport_t *transport;
    mDNS *m = &mDNSStorage;

    (void)connected;
    mDNS_Lock(m);
    detail = cs->detail;

    // If we had a socket open but the connect failed, close it and try the next address, if we have
    // a next address.
    if (sock != NULL) {
        cs->last_event = m->timenow;

        cs->connecting = mDNSfalse;
        if (err != mStatus_NoError) {
            mDNSPlatformTCPCloseConnection(sock);
            LogMsg("dso_connect_callback: connect %p failed (%d)", cs, err);
        } else {
        success:
            // We got a connection.
            transport = dso_transport_create(sock, false, cs->context, cs->dso_context_callback,
                cs->max_outstanding_queries, cs->inbuf_size, cs->outbuf_size, cs->hostname, cs->callback, cs->dso);
            if (transport == NULL) {
                // If dso_state_create fails, there's no point in continuing to try to connect to new
                // addresses
            fail:
                LogMsg("dso_connect_callback: dso_state_create failed");
                mDNSPlatformTCPCloseConnection(sock);
            } else {
                // Call the "we're connected" callback, which will start things up.
                transport->dso->cb(cs->context, NULL, transport->dso, kDSOEventType_Connected);
            }

            cs->last_event = 0;

            // When the connection has succeeded, stop asking questions.
            if (cs->lookup != NULL) {
                DNSServiceRef ref = cs->lookup;
                cs->lookup = NULL;
                mDNS_DropLockBeforeCallback();
                DNSServiceRefDeallocate(ref);
                mDNS_ReclaimLockAfterCallback();
            }
            mDNS_Unlock(m);
            return;
        }
    }

    // If there are no addresses to connect to, and there are no queries running, then we can give
    // up.  Otherwise, we wait for one of the queries to deliver an answer.
    if (cs->next_addr == NULL) {
        // We may get more callbacks when outstanding queries either fail or succeed, at which point we can try to
        // connect to those addresses, or give up.
        mDNS_Unlock(m);
        return;
    }

    const mDNSAddr addr = cs->next_addr->address;
    const mDNSIPPort port = cs->next_addr->port;

    sock = mDNSPlatformTCPSocket(kTCPSocketFlags_Zero, addr.type, NULL, NULL, mDNSfalse);
    if (sock == NULL) {
        LogMsg("drConnectCallback: couldn't get a socket for %s: %s%s",
               cs->hostname, strerror(errno), detail);
        goto fail;
    }

    LogMsg("dso_connect_callback: Attempting to connect to %#a%%%d", &addr, mDNSVal16(port));

    status = mDNSPlatformTCPConnect(sock, &addr, port, NULL, dso_connect_callback, cs);
    // We finished setting up the connection with the first address in the list, so remove it from the list.
    cs->next_addr = cs->next_addr->next;

    if (status == mStatus_NoError || status == mStatus_ConnEstablished) {
        // This can't happen in practice on MacOS; we don't know about all other operating systems,
        // so we handle it just in case.
        LogMsg("dso_connect_callback: synchronous connect to %s", cs->hostname);
        goto success;
    } else if (status == mStatus_ConnPending) {
        LogMsg("dso_connect_callback: asynchronous connect to %s", cs->hostname);
        cs->connecting = mDNStrue;
        // We should get called back when the connection succeeds or fails.
        mDNS_Unlock(m);
        return;
    }
    LogMsg("dso_connect_callback: failed to connect to %s on %#a%d: %s%s",
           cs->hostname, &addr, mDNSVal16(port), strerror(errno), detail);
    mDNS_Unlock(m);
}

static void dso_connect_internal(dso_connect_state_t *cs, mDNSBool reconnecting)
{
    (void)reconnecting;
    dso_connect_callback(NULL, cs, false, mStatus_NoError);
}
#endif // DSO_USES_NETWORK_FRAMEWORK

static void dso_connect_state_process_address_port_change(const mDNSAddr *addr_changed, mDNSIPPort port,
                                                          bool add, dso_connect_state_t *const cs)
{
    bool succeeded;
    dso_transport_address_t **addrs = &cs->addrs;

    if (add) {
        // Always add the new address to the tail, so the order of using the address to connect is the same as the order
        // of the address being added.
        while (*addrs != NULL) {
            addrs = &(*addrs)->next;
        }
        dso_transport_address_t *new_addr = mdns_calloc(1, sizeof(*new_addr));
        if (new_addr == NULL) {
            succeeded = false;
            goto exit;
        }
        memcpy(&new_addr->address, addr_changed, sizeof (*addr_changed));
        new_addr->port = port;
        *addrs = new_addr;
        if (cs->next_addr == NULL) {
            cs->next_addr = new_addr;
        }
    } else {
        // Remove address that has been previously added, so that mDNSResponder will not even try the removed address in
        // the future when reconnecting.
        bool removed = mDNSfalse;
        while (*addrs != NULL) {
            dso_transport_address_t *addr = *addrs;
            if (((addr->address.type == mDNSAddrType_IPv4 && !memcmp(&addr->address.ip.v4, &addr_changed->ip.v4,
                                                                     sizeof(addr->address.ip.v4))) ||
                 (addr->address.type == mDNSAddrType_IPv6 && !memcmp(&addr->address.ip.v6, &addr_changed->ip.v6,
                                                                     sizeof(addr->address.ip.v4)))) &&
                addr->port.NotAnInteger == port.NotAnInteger)
            {
                if (cs->next_addr == addr) {
                    cs->next_addr = addr->next;
                }
                *addrs = addr->next;
                mdns_free(addr);
                removed = mDNStrue;
                break;
            }
            addrs = &addr->next;
        }

        if (!removed) {
            // If the address being removed is not in the list, it indicates the following two scenarios:
            // 1. The address has been traversed when dso_connect_state_t tries to connect to the server address.
            // 2. The address is the server address that dso_transport_t currently connects to, for efficiency, it is
            // not terminated immediately. If the address is removed because of the network changes, dso_transport_t
            // will notice that and terminate the connection by itself.
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[DSOC%u] The address being removed has been tried for the connection or is being used right now - "
                "address: " PRI_IP_ADDR ":%u.", cs->serial, addr_changed, mDNSVal16(cs->config_port));
        }
    }

    if (!cs->connecting && !cs->transport) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] Starting a new connection.", cs->serial);
        dso_connect_internal(cs, mDNSfalse);
    } else {
        if (cs->connecting) {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                      "[DSOC%u] Connection in progress, deferring new connection until it fails.", cs->serial);
        } else {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                      "[DSOC%u] Already connected, retained new address for later need.", cs->serial);
        }
    }

    succeeded = true;
exit:
    if (!succeeded) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
                  "[DSOC%u] Failed to process address changes for dso_connect_state_t.", cs->serial);
    }
}

static void dso_connect_state_process_address_change(const mDNSAddr *addr_changed, const bool add,
                                                     dso_connect_state_t *const cs)
{
    dso_connect_state_process_address_port_change(addr_changed, cs->config_port, add, cs);
}

static void dso_inaddr_callback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                DNSServiceErrorType errorCode, const char *fullname, const struct sockaddr *sa,
                                uint32_t ttl, void *context)
{
    (void)sdRef;
    dso_connect_state_t *cs = context;
    mDNS *const m = &mDNSStorage;

    // Do not proceed if we find that the dso_connect_state_t has been canceled previously.
    if (cs->canceled) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[DSOC%u] The current dso_connect_state_t has been canceled - hostname: " PRI_S ".", cs->serial,
            cs->hostname);
        goto exit;
    }

    cs->last_event = mDNSStorage.timenow;

    // It is possible that the network does not support IPv4 or IPv6, in which case we will get the
    // kDNSServiceErr_NoSuchRecord error for the corresponding unsupported address type. This is a valid case.
    if (errorCode == kDNSServiceErr_NoSuchRecord) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] No usable IP address for the DNS push server - "
            "host name: " PRI_S ", address type: " PUB_S ".", cs->serial, fullname,
            sa->sa_family == AF_INET ? "A" : "AAAA");
        goto exit;
    }

    // All the other error cases other than kDNSServiceErr_NoSuchRecord and kDNSServiceErr_NoError are invalid. They
    // should be reported as FAULT.
    if (errorCode != kDNSServiceErr_NoError) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, "[DSOC%u] Unexpected dso_inaddr_callback call - "
            "flags: %x, error: %d.", cs->serial, flags, errorCode);
        goto exit;
    }

    const mDNSAddr addr_changed = mDNSAddr_from_sockaddr(sa);
    const mDNSBool addr_add = (flags & kDNSServiceFlagsAdd) != 0;

    // mDNSPlatformInterfaceIDfromInterfaceIndex() should be called without holding mDNS lock, because the function itself
    // may need to grab mDNS lock.
    const mDNSInterfaceID if_id = mDNSPlatformInterfaceIDfromInterfaceIndex(m, interfaceIndex);
    mDNS_Lock(m);
    const char *const if_name = InterfaceNameForID(m, if_id);
    const mDNSBool on_the_local_subnet = mDNS_AddressIsLocalSubnet(m, if_id, &addr_changed);
    mDNS_Unlock(m);

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] dso_inaddr_callback - address " PUB_ADD_RMV
        ", resolved name: " PRI_S ", flags: %x, interface name: " PUB_S "(%u), erorr: %d, full name: " PRI_S
        ", addr: " PRI_IP_ADDR ":%u, ttl: %u, on the local subnet: " PUB_BOOL ".", cs->serial, ADD_RMV_PARAM(addr_add),
        fullname, flags, if_name, interfaceIndex, errorCode, fullname, &addr_changed, mDNSVal16(cs->config_port), ttl,
        BOOL_PARAM(on_the_local_subnet));

    dso_connect_state_process_address_change(&addr_changed, addr_add, cs);

exit:
    return;
}

bool dso_connect(dso_connect_state_t *cs)
{
    struct in_addr in;
    struct in6_addr in6;

    if (cs->next_addr != NULL) {
        // If the connection state was created with an address, use that rather than hostname,
        dso_connect_internal(cs, mDNSfalse);

    } else if (inet_pton(AF_INET, cs->hostname, &in)) {
        // else allow an IPv4 address literal string,
        const mDNSAddr v4 = mDNSAddr_from_in_addr(&in);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[DSOC%u] Add and connecting to an IPv4 address literal string directly - address: " PRI_IP_ADDR ":%u.",
            cs->serial, &v4, mDNSVal16(cs->config_port));

        dso_connect_state_process_address_change(&v4, true, cs);

    } else if (inet_pton(AF_INET6, cs->hostname, &in6)) {
        // or an IPv6 address literal string,
        const mDNSAddr v6 = mDNSAddr_from_in6_addr(&in6);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[DSOC%u] Add and connecting to an IPv6 address literal string directly - address: " PRI_IP_ADDR ":%u.",
            cs->serial, &v6, mDNSVal16(cs->config_port));

        dso_connect_state_process_address_change(&v6, true, cs);

    } else {
        // else look it up.
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[DSOC%u] Resolving server name to start a new connection - "
            "server: " PRI_S ".", cs->serial, cs->hostname);
        mDNS *m = &mDNSStorage;
        mDNS_DropLockBeforeCallback();
        const DNSServiceErrorType err = DNSServiceGetAddrInfo(&cs->lookup, kDNSServiceFlagsReturnIntermediates,
            kDNSServiceInterfaceIndexAny, 0, cs->hostname, dso_inaddr_callback, cs);

        mDNS_ReclaimLockAfterCallback();
        if (err != mStatus_NoError) {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
                "[DSOC%u] Name resolving failed for the server to connect - server: " PRI_S ", error: %d.",
                cs->serial, cs->hostname, err);
            return false;
        }
    }
    return true;
}

#ifdef DSO_USES_NETWORK_FRAMEWORK
// We don't need this for DNS Push, so it is being left as future work.
int dso_listen(dso_connect_state_t * __unused listen_context)
{
    return mStatus_UnsupportedErr;
}

#else

// Called whenever we get a connection on the DNS TCP socket
static void dso_listen_callback(TCPSocket *sock, mDNSAddr *addr, mDNSIPPort *port,
                                const char *remote_name, void *context)
{
    dso_connect_state_t *lc = context;
    dso_transport_t *transport;

    mDNS_Lock(&mDNSStorage);
    transport = dso_transport_create(sock, mDNStrue, lc->context, lc->dso_context_callback, lc->max_outstanding_queries,
                                     lc->inbuf_size, lc->outbuf_size, remote_name, lc->callback, NULL);
    if (transport == NULL) {
        mDNSPlatformTCPCloseConnection(sock);
        LogMsg("No memory for new DSO connection from %s", remote_name);
        goto out;
    }

    transport->remote_addr = *addr;
    transport->remote_port = ntohs(port->NotAnInteger);
    if (transport->dso->cb) {
        transport->dso->cb(lc->context, 0, transport->dso, kDSOEventType_Connected);
    }
    LogMsg("DSO connection from %s", remote_name);
out:
    mDNS_Unlock(&mDNSStorage);
}

// Listen for connections; each time we get a connection, make a new dso_state_t object with the specified
// parameters and call the callback.   Port can be zero to leave it unspecified.

int dso_listen(dso_connect_state_t *listen_context)
{
    mDNSIPPort port;
    mDNSBool reuseAddr = mDNSfalse;

    if (listen_context->config_port.NotAnInteger) {
        port = listen_context->config_port;
        reuseAddr = mDNStrue;
    }
    listen_context->listener = mDNSPlatformTCPListen(mDNSAddrType_None, &port, NULL, kTCPSocketFlags_Zero,
                                                     reuseAddr, 5, dso_listen_callback, listen_context);
    if (!listen_context->listener) {
        return mStatus_UnknownErr;
    }
    listen_context->connect_port = port;

    LogMsg("DSOListen: Listening on <any>%%%d", mDNSVal16(port));
    return mStatus_NoError;
}
#endif // DSO_USES_NETWORK_FRAMEWORK

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
