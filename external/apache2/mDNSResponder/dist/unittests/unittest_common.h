/*
 * Copyright (c) 2016-2021 Apple Inc. All rights reserved.
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

#ifndef UNITTEST_COMMON_H
#define UNITTEST_COMMON_H

#include "dns_sd.h"
#include "uds_daemon.h"
#include "uDNS.h"
#include "dnssd_ipc.h"
#include <netdb.h>                  // for getaddrinfo
#include <net/if.h>
#include <pthread.h>
#include <CoreFoundation/CoreFoundation.h>
#include "mDNSFeatures.h"

// Primary interface info that is used when simulating the receive of the response packet
extern mDNSInterfaceID primary_interfaceID;
extern mDNSAddr primary_v4;
extern mDNSAddr primary_v6;
extern mDNSAddr primary_router;

// Arbitrary values to simulate a client_request request
#define client_req_sd				12
#define client_req_uid				502
#define client_req_hdr_bytes		28
#define client_req_hdr_version		1
#define client_resp_src_port		13568
#define client_resp_dst_port		49339
#define uDNS_TargetQID				16745
#define client_req_process_id		15418
static char client_req_pid_name[MAXCOMLEN] = "mDNSUnitTest";

//Arbitrary values to simulate a DNS server
#define dns_server_timeout			30
#define dns_server_resGroupID		12
static const mDNSv4Addr dns_server_ipv4 = {{ 192, 168, 1, 20 }};

extern mStatus  init_mdns_environment(mDNSBool enableLogging);
extern mStatus  init_mdns_storage(void);
extern size_t   get_reply_len(char* name, uint16_t rdlen);
extern mStatus  start_client_request(request_state* req, const uint8_t *msgbuf, uint32_t msgsz, uint32_t op, UDPSocket* socket);
extern void     receive_response(const request_state* req, DNSMessage *msg, size_t msgSize);
extern void     get_ip(const char *const name, struct sockaddr_storage *result);
extern void     free_req(request_state* req);

extern mStatus  mDNS_InitStorage_ut(mDNS *const m, mDNS_PlatformSupport *const p,
                                   CacheEntity *rrcachestorage, mDNSu32 rrcachesize,
                                   mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context);

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

#define MAX_CNAME_TRAVERSAL 11
void
ParseCNameChainFromMessage_ut(
    const DNSMessage *      response,
    const mDNSu8 *          limit,
    const mDNSInterfaceID   InterfaceID,
    const domainname *      qname,
    domainname              cnameChain[static MAX_CNAME_TRAVERSAL],
    mDNSu32  *              outChainLen);

#define MAX_NUM_NSEC_NSEC3_TO_PROCESS   10
#define MAX_NUM_RRSIG_TO_PROCESS        30
void
ParseDenialOfExistenceObjsFromMessage_ut(
    const DNSMessage *      response,
    const mDNSu8 *          limit,
    const mDNSInterfaceID   InterfaceID,
    dnssec_obj_rr_soa_t *   outObjSOA,
    dnssec_obj_rr_rrsig_t   objSOARRSIG[static MAX_NUM_RRSIG_TO_PROCESS],
    mDNSu8 *                outSOARRSIGCount,
    dnssec_obj_rr_nsec_t    outObjNSECs[static MAX_NUM_NSEC_NSEC3_TO_PROCESS],
    mDNSu8 *                outNSECCount,
    dnssec_obj_rr_nsec3_t   outObjNSEC3s[static MAX_NUM_NSEC_NSEC3_TO_PROCESS],
    mDNSu8 *                outNSEC3Count,
    dnssec_obj_rr_rrsig_t   outObjRRSIGs[static MAX_NUM_RRSIG_TO_PROCESS],
    mDNSu8 *                outRRSIGCount);

mDNSu16
GetRRTypeFromMessage(const DNSMessage *response, const mDNSu8 *limit, mDNSInterfaceID InterfaceID);

#endif

extern void     init_logging_ut(void);
extern void     SetInterfaces_ut(mDNSInterfaceID* primary_interfaceID, mDNSAddr *primary_v4,
                                 mDNSAddr* primary_v6, mDNSAddr* primary_router);
extern mStatus  handle_client_request_ut(void *req);
extern void     LogCacheRecords_ut(mDNSs32 now, mDNSu32* retCacheUsed, mDNSu32* retCacheActive);
extern int      LogEtcHosts_ut(mDNS *const m);
extern mDNSBool mDNSMacOSXCreateEtcHostsEntry_ut(const domainname *domain, const struct sockaddr *sa,
                                                 const domainname *cname, char *ifname, AuthHash *auth);
extern void     UpdateEtcHosts_ut(void *context);
extern mStatus  AddDNSServer_ut(void);
extern mStatus  AddDNSServerScoped_ut(mDNSInterfaceID interfaceID, ScopeType scoped);
extern mStatus  force_uDNS_SetupDNSConfig_ut(mDNS *const m);
extern mStatus  verify_cache_addr_order_for_domain_ut(mDNS *const m, mDNSu8* octet, mDNSu32 count, const domainname *const name);

// HelperFunctionTest
extern void mDNSDomainLabelFromCFString_ut(CFStringRef cfs, domainlabel *const namelabel);
mDNSexport mDNSu32 IndexForInterfaceByName_ut(const char *ifname);

#endif /* UNITTEST_COMMON_H */
