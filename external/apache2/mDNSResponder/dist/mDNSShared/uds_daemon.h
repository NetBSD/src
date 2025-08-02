/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2024 Apple Inc. All rights reserved.
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
 */

#ifndef UDS_DAEMON_H
#define UDS_DAEMON_H

#include "mDNSEmbeddedAPI.h"
#include "dnssd_ipc.h"
#include "ClientRequests.h"
#include "general.h"
#if MDNSRESPONDER_SUPPORTS(APPLE, AUDIT_TOKEN)
#include <mdns/audit_token.h>
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, TRUST_ENFORCEMENT)
#include "mdns_trust.h"
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, SIGNED_RESULTS)
#include "signed_result.h"
#endif

/* Client request: */

// ***************************************************************************
// MARK: - Types and Data Structures

MDNS_CLOSED_ENUM(transfer_state, mDNSu8,
    t_uninitialized,
    t_morecoming,
    t_complete,
    t_error,
    t_terminated
);

typedef struct request_state request_state;

typedef void (*req_termination_fn)(request_state *request);

typedef struct registered_record_entry
{
#if MDNSRESPONDER_SUPPORTS(APPLE, POWERLOG_MDNS_REQUESTS)
    uint64_t powerlog_start_time;
#endif
    struct registered_record_entry *next;
    mDNSu32 key;
    client_context_t regrec_client_context;
    request_state *request;
    mDNSBool external_advertise;
    mDNSInterfaceID origInterfaceID;
    AuthRecord *rr;             // Pointer to variable-sized AuthRecord (Why a pointer? Why not just embed it here?)
} registered_record_entry;

// A single registered service: ServiceRecordSet + bookkeeping
// Note that we duplicate some fields from parent service_info object
// to facilitate cleanup, when instances and parent may be deallocated at different times.
typedef struct service_instance
{
    struct service_instance *next;
    request_state *request;
    AuthRecord *subtypes;
    mDNSBool renameonmemfree;       // Set on config change when we deregister original name
    mDNSBool clientnotified;        // Has client been notified of successful registration yet?
    mDNSBool default_local;         // is this the "local." from an empty-string registration?
    mDNSBool external_advertise;    // is this is being advertised externally?
    domainname domain;
    ServiceRecordSet srs;           // note -- variable-sized object -- must be last field in struct
} service_instance;

// for multi-domain default browsing
typedef struct browser_t
{
    struct browser_t *next;
    domainname domain;
    DNSQuestion q;
} browser_t;

#ifdef _WIN32
# ifdef __MINGW32__
typedef int pid_t;
typedef int socklen_t;
# else
typedef unsigned int pid_t;
typedef int socklen_t;
# endif // __MINGW32__
#endif //_WIN32

#if (!defined(MAXCOMLEN))
#define MAXCOMLEN 16
#endif

typedef struct
{
    DNSServiceFlags flags;
    DNSQuestion q_all;
    DNSQuestion q_default;
    DNSQuestion q_autoall;
} request_enumeration;
mdns_compile_time_max_size_check(request_enumeration, 2144);

typedef struct
{
    mDNSInterfaceID InterfaceID;
    mDNSu16 txtlen;
    void *txtdata;
    mDNSIPPort port;
    domainlabel name;
    char type_as_string[MAX_ESCAPED_DOMAIN_NAME];
    domainname type;
    mDNSBool default_domain;
    domainname host;
    mDNSBool autoname;              // Set if this name is tied to the Computer Name
    mDNSBool autorename;            // Set if this client wants us to automatically rename on conflict
    mDNSBool allowremotequery;      // Respond to unicast queries from outside the local link?
    mDNSu32 num_subtypes;
    service_instance *instances;
} request_servicereg;
mdns_compile_time_max_size_check(request_servicereg, 1632);

typedef struct
{
    DNSQuestion qtxt;
    DNSQuestion qsrv;

    domainname *srv_target_name;    // Dynamically allocated SRV rdata(target name)
    mDNSu8 *txt_rdata;              // Dynamically allocated TXT rdata.
    mDNSIPPort srv_port;            // The port number specified in the SRV rdata.
    mDNSu16 txt_rdlength;           // The length of the TXT record rdata.

    mDNSs32 ReportTime;
    mDNSBool external_advertise;
    mDNSBool srv_negative;          // Whether we have received a negative SRV record. If true, srv_target_name is
                                    // always NULL and srv_port's value has no meaning. When srv_target_name is
                                    // non-NULL, srv_negative is always false;
    mDNSBool txt_negative;          // Whether we have received a negative TXT record. If true, txt_rdata is always NULL
                                    // and txt_rdlength is 0. When txt_rdata is non-NULL, txt_negative is always false.
} request_resolve;
mdns_compile_time_max_size_check(request_resolve, 1456);

typedef struct
{
    mDNSInterfaceID interface_id;
    mDNSBool default_domain;
    mDNSBool ForceMCast;
    domainname regtype;
    browser_t *browsers;
} request_browse;
mdns_compile_time_max_size_check(request_browse, 280);

typedef struct
{
    mDNSIPPort ReqExt; // External port we originally requested, for logging purposes
    NATTraversalInfo NATinfo;
} request_port_mapping;
mdns_compile_time_max_size_check(request_port_mapping, 208);

#if MDNSRESPONDER_SUPPORTS(APPLE, PADDING_CHECKS)
// The member variables of struct request_state are in descending order of alignment requirement to eliminate
// padding between member variables. That is, member variables with an 8-byte alignment requirement come first, followed
// by member variables with a 4-byte alignment requirement, and so forth.
MDNS_CLANG_TREAT_WARNING_AS_ERROR_BEGIN(-Wpadded)
#endif
struct request_state
{
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_dns_service_id_t custom_service_id;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, POWERLOG_MDNS_REQUESTS)
    uint64_t powerlog_start_time;
#endif
    request_state *next;            // For a shared connection, the next element in the list of subordinate
                                    // requests on that connection. Otherwise null.
    request_state *primary;         // For a subordinate request, the request that represents the shared
                                    // connection to which this request is subordinate (must have been created
                                    // by DNSServiceCreateConnection().
#if MDNSRESPONDER_SUPPORTS(APPLE, AUDIT_TOKEN)
    mdns_audit_token_t peer_token;  // The immediate client's audit token.
#endif
    void * platform_data;
#if MDNSRESPONDER_SUPPORTS(APPLE, TRUST_ENFORCEMENT)
    CFMutableArrayRef trusts;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, SIGNED_RESULTS)
    mdns_signed_result_t signed_obj;
#endif
    size_t data_bytes;              // bytes of message data already read [1]
    uint8_t       *msgbuf;          // pointer to data storage to pass to free() [1]
    const uint8_t *msgptr;          // pointer to data to be read from (may be modified) [1]
    const uint8_t *msgend;          // pointer to byte after last byte of message [1]
    struct reply_state *replies;    // corresponding (active) reply list
    req_termination_fn terminate;
    request_enumeration *enumeration;
    request_servicereg *servicereg;
    request_resolve *resolve;
    QueryRecordClientRequest *queryrecord;
    request_browse *browse;
    request_port_mapping *pm;
    GetAddrInfoClientRequest *addrinfo;
    registered_record_entry *reg_recs;  // list of registrations for a connection-oriented request
    dnssd_sock_t sd;
    pid_t process_id;               // The effective client's PID value.
    dnssd_sock_t errsd;
    mDNSu32 uid;
    mDNSu32 request_id;
    mDNSs32 request_start_time_secs; // The time when the request is started in continuous time seconds.
    mDNSs32 last_full_log_time_secs; // The time when we print full log information in continuous time seconds.
    mDNSu32 hdr_bytes;              // bytes of header already read [1]
    ipc_msg_hdr hdr;                // [1]
    mDNSs32 time_blocked;           // record time of a blocked client
    DNSServiceFlags flags;
    mDNSu32 interfaceIndex;
    char pid_name[MAXCOMLEN];       // The effective client's process name.
    mDNSu8 uuid[UUID_SIZE];
    mDNSBool validUUID;
#if MDNSRESPONDER_SUPPORTS(APPLE, TRACKER_DEBUGGING)
    mDNSBool addTrackerInfo;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, SIGNED_RESULTS)
    mDNSBool sign_result;
#endif
    transfer_state ts;              // [1]
    mDNSBool no_reply;              // don't send asynchronous replies to client
    mDNSu8 unresponsiveness_reports;
#if MDNSRESPONDER_SUPPORTS(APPLE, PADDING_CHECKS)
    MDNS_STRUCT_PAD_64_32(2, 6);
#endif
};
#if MDNSRESPONDER_SUPPORTS(APPLE, PADDING_CHECKS)
MDNS_CLANG_TREAT_WARNING_AS_ERROR_END()
MDNS_GENERAL_STRUCT_PAD_CHECK(struct request_state);
#endif
mdns_compile_time_max_size_check(struct request_state, 288);

// Notes:
// 1. On a shared connection these fields in the primary structure, including hdr, are re-used
//    for each new request. This is because, until we've read the ipc_msg_hdr to find out what the
//    operation is, we don't know if we're going to need to allocate a new request_state or not.

// struct physically sits between ipc message header and call-specific fields in the message buffer
typedef struct
{
    DNSServiceFlags flags;          // Note: This field is in NETWORK byte order
    mDNSu32 ifi;                    // Note: This field is in NETWORK byte order
    DNSServiceErrorType error;      // Note: This field is in NETWORK byte order
} reply_hdr;

typedef struct reply_state
{
    struct reply_state *next;       // If there are multiple unsent replies
    mDNSu32 totallen;
    mDNSu32 nwritten;
    ipc_msg_hdr mhdr[1];
    reply_hdr rhdr[1];
} reply_state;

/* Client interface: */

#define SRS_PORT(S) mDNSVal16((S)->RR_SRV.resrec.rdata->u.srv.port)

#define LogTimerToFD(FILE_DESCRIPTOR, MSG, T) LogToFD((FILE_DESCRIPTOR), MSG " %08X %11d  %08X %11d", (T), (T), (T)-now, (T)-now)

extern int udsserver_init(dnssd_sock_t skts[], size_t count);
extern mDNSs32 udsserver_idle(mDNSs32 nextevent);
extern void udsserver_info_dump_to_fd(int fd);
extern void udsserver_handle_configchange(mDNS *const m);
extern int udsserver_exit(void);    // should be called prior to app exit
extern void LogMcastStateInfo(mDNSBool mflag, mDNSBool start, mDNSBool mstatelog);
#define LogMcastQ       (mDNS_McastLoggingEnabled == 0) ? ((void)0) : LogMcastQuestion
#define LogMcastS       (mDNS_McastLoggingEnabled == 0) ? ((void)0) : LogMcastService
#define LogMcast        (mDNS_McastLoggingEnabled == 0) ? ((void)0) : LogMsg
#define LogMcastNoIdent (mDNS_McastLoggingEnabled == 0) ? ((void)0) : LogMsgNoIdent

/* Routines that uds_daemon expects to link against: */

typedef void (*udsEventCallback)(int fd, void *context);
extern mStatus udsSupportAddFDToEventLoop(dnssd_sock_t fd, udsEventCallback callback, void *context, void **platform_data);
extern ssize_t udsSupportReadFD(dnssd_sock_t fd, char* buf, mDNSu32 len, int flags, void *platform_data);
extern mStatus udsSupportRemoveFDFromEventLoop(dnssd_sock_t fd, void *platform_data); // Note: This also CLOSES the file descriptor as well

extern void RecordUpdatedNiceLabel(mDNSs32 delay);

// Globals and functions defined in uds_daemon.c and also shared with the old "daemon.c" on OS X

extern mDNS mDNSStorage;
extern DNameListElem *AutoRegistrationDomains;
extern DNameListElem *AutoBrowseDomains;

extern int CountExistingRegistrations(domainname *srv, mDNSIPPort port);
extern void FreeExtraRR(mDNS *const m, AuthRecord *const rr, mStatus result);
extern int CountPeerRegistrations(ServiceRecordSet *const srs);

extern const char mDNSResponderVersionString_SCCS[];
#define mDNSResponderVersionString (mDNSResponderVersionString_SCCS+5)

#if defined(DEBUG) && DEBUG
extern void SetDebugBoundPath(void);
extern int IsDebugSocketInUse(void);
#endif

#endif /* UDS_DAEMON_H */
