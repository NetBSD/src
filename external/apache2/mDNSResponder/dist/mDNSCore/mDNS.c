/* -*- Mode: C; tab-width: 4; c-file-style: "bsd"; c-basic-offset: 4; fill-column: 108; indent-tabs-mode: nil; -*-
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
 *
 * This code is completely 100% portable C. It does not depend on any external header files
 * from outside the mDNS project -- all the types it expects to find are defined right here.
 *
 * The previous point is very important: This file does not depend on any external
 * header files. It should compile on *any* platform that has a C compiler, without
 * making *any* assumptions about availability of so-called "standard" C functions,
 * routines, or types (which may or may not be present on any given platform).
 */

#include "DNSCommon.h"                  // Defines general DNS utility routines
#include "uDNS.h"                       // Defines entry points into unicast-specific routines
#include <sys/time.h>                   // For gettimeofday().

#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
#include "D2D.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, SYMPTOMS)
#include "SymptomReporter.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, TRACKER_STATE)
#include "resolved_cache.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, ANALYTICS)
#include "dnssd_analytics.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
#include "dns_push_discovery.h"
#include "dns_push_mdns_core.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
#include "QuerierSupport.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
#include "misc_utilities.h"
#include "unicast_assist_cache.h"
#endif

// Disable certain benign warnings with Microsoft compilers
#if (defined(_MSC_VER))
// Disable "conditional expression is constant" warning for debug macros.
// Otherwise, this generates warnings for the perfectly natural construct "while(1)"
// If someone knows a variant way of writing "while(1)" that doesn't generate warning messages, please let us know
    #pragma warning(disable:4127)

// Disable "assignment within conditional expression".
// Other compilers understand the convention that if you place the assignment expression within an extra pair
// of parentheses, this signals to the compiler that you really intended an assignment and no warning is necessary.
// The Microsoft compiler doesn't understand this convention, so in the absense of any other way to signal
// to the compiler that the assignment is intentional, we have to just turn this warning off completely.
    #pragma warning(disable:4706)
#endif

#include "bsd_queue.h"
#include "CommonServices.h"
#include "dns_sd.h" // for kDNSServiceFlags* definitions
#include "dns_sd_internal.h"


#ifndef MDNS_LOG_ANSWER_SUPPRESSION_TIMES
#define MDNS_LOG_ANSWER_SUPPRESSION_TIMES 0
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, WEB_CONTENT_FILTER)
#include <WebFilterDNS/WebFilterDNS.h>

WCFConnection *WCFConnectionNew(void) __attribute__((weak_import));
void WCFConnectionDealloc(WCFConnection* c) __attribute__((weak_import));
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
#include <CoreAnalytics/CoreAnalytics.h>
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
#include "dnssd_analytics.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
#include "DNS64.h"
#endif


#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
#include "discover_resolver.h" // For resolver_discovery_process().
#include "tls-keychain.h"
#endif

#include "mdns_strict.h"

// Forward declarations
extern mDNS mDNSStorage; // NOLINT(misc-uninitialized-record-variable): Initialized by mDNS_InitStorage().
mDNSlocal void BeginSleepProcessing(mDNS *const m);
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal void RetrySPSRegistrations(mDNS *const m);
#endif
mDNSlocal void SendWakeup(mDNS *const m, mDNSInterfaceID InterfaceID, mDNSEthAddr *EthAddr, mDNSOpaque48 *password, mDNSBool unicastOnly);
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSlocal mDNSBool LocalRecordRmvEventsForQuestion(mDNS *const m, DNSQuestion *q);
#endif
mDNSlocal void mDNS_PurgeBeforeResolve(mDNS *const m, DNSQuestion *q);
mDNSlocal void mDNS_SendKeepalives(mDNS *const m);
mDNSlocal void mDNS_ExtractKeepaliveInfo(AuthRecord *ar, mDNSu32 *timeout, mDNSAddr *laddr, mDNSAddr *raddr, mDNSEthAddr *eth,
                                         mDNSu32 *seq, mDNSu32 *ack, mDNSIPPort *lport, mDNSIPPort *rport, mDNSu16 *win);

typedef mDNSu32 DeadvertiseFlags;
#define kDeadvertiseFlag_NormalHostname (1U << 0)
#define kDeadvertiseFlag_RandHostname   (1U << 1)
#define kDeadvertiseFlag_All            (kDeadvertiseFlag_NormalHostname | kDeadvertiseFlag_RandHostname)

mDNSlocal void DeadvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set, DeadvertiseFlags flags);
mDNSlocal void AdvertiseInterfaceIfNeeded(mDNS *const m, NetworkInterfaceInfo *set);
mDNSlocal mDNSu8 *GetValueForMACAddr(mDNSu8 *ptr, mDNSu8 *limit, mDNSEthAddr *eth);

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
typedef mdns_dns_service_t DNSServRef;
#else
typedef DNSServer *DNSServRef;
#endif

mDNSlocal void MakeNegativeCacheRecord(mDNS *m, CacheRecord *cr, const domainname *name, mDNSu32 namehash, mDNSu16 rrtype,
    mDNSu16 rrclass, mDNSu32 ttl, mDNSInterfaceID InterfaceID, DNSServRef dnsserv, mDNSOpaque16 responseFlags);

mDNSlocal mStatus mDNS_SetUpDomainEnumeration(mDNS *m, DomainEnumerationOp *op, mDNS_DomainType type);

// ***************************************************************************
// MARK: - Program Constants

// To Turn OFF mDNS_Tracer set MDNS_TRACER to 0 or undef it
#define MDNS_TRACER 1

// Any records bigger than this are considered 'large' records
#define SmallRecordLimit 1024

#define kMaxUpdateCredits 10
#define kUpdateCreditRefreshInterval (mDNSPlatformOneSecond * 6)

// define special NR_AnswerTo values
#define NR_AnswerMulticast  (mDNSu8*)~0
#define NR_AnswerUnicast    (mDNSu8*)~1

// Question default timeout values
#define DEFAULT_MCAST_TIMEOUT       5
#define DEFAULT_LO_OR_P2P_TIMEOUT   5

// The code (see SendQueries() and BuildQuestion()) needs to have the
// RequestUnicast value set to a value one greater than the number of times you want the query
// sent with the "request unicast response" (QU) bit set.
#define SET_QU_IN_FIRST_QUERY   2
#define kDefaultRequestUnicastCount SET_QU_IN_FIRST_QUERY

// The time needed to offload records to a sleep proxy after powerd sends the kIOMessageSystemWillSleep notification
#define DARK_WAKE_DELAY_SLEEP  5
#define kDarkWakeDelaySleep    (mDNSPlatformOneSecond * DARK_WAKE_DELAY_SLEEP)

struct TSRDataRec {
    SLIST_ENTRY(TSRDataRec) entries;
    TSROptData              tsr;
    domainname              name;
};
typedef SLIST_HEAD(, TSRDataRec) TSRDataRecHead;
mDNSlocal struct TSRDataRec *TSRDataRecCreate(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end,
    const rdataOPT * const opt)
{
    domainname name;
    const mDNSu8 *next_ptr;
    struct TSRDataRec *rec = mDNSNULL;

    next_ptr = getDomainName(msg, ptr, end, &name);
    mdns_require_action_quiet(next_ptr, exit,
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
            "TSRDataRecCreate: Bad RR domain name for TSR - tsrTime %d tsrHost %x recIndex %u",
            opt->u.tsr.timeStamp, opt->u.tsr.hostkeyHash, opt->u.tsr.recIndex));

    rec = (struct TSRDataRec *)mDNSPlatformMemAllocateClear(sizeof(*rec));
    mdns_require_quiet(rec, exit);

    AssignDomainName(&rec->name, &name);
    mDNSPlatformMemCopy(&rec->tsr, &opt->u.tsr, sizeof(TSROptData));
exit:
    return rec;
}
mDNSlocal void TSRDataRecHeadFreeList(TSRDataRecHead *tsrHead)
{
    mdns_require_quiet(tsrHead, exit);
    while (!SLIST_EMPTY(tsrHead)) {
        struct TSRDataRec *next = SLIST_FIRST(tsrHead);
        SLIST_REMOVE_HEAD(tsrHead, entries);
        mDNSPlatformMemFree(next);
    }
exit:
    return;
}

struct TSRDataPtrRec {
    SLIST_ENTRY(TSRDataPtrRec)  entries;
    const TSROptData            *tsr;
    const domainname            *name;
};
typedef SLIST_HEAD(, TSRDataPtrRec) TSRDataPtrRecHead;
mDNSlocal mDNSBool TSRDataRecPtrHeadAddTSROpt(TSRDataPtrRecHead * const tsrHead, TSROptData * const tsrOpt,
    const domainname * const name, mDNSu16 index)
{
    struct TSRDataPtrRec *rec = (struct TSRDataPtrRec *)mDNSPlatformMemAllocateClear(sizeof(*rec));
    mdns_require_quiet(rec, exit);

    tsrOpt->recIndex = index;
    rec->tsr = tsrOpt;
    rec->name = name;
    SLIST_INSERT_HEAD(tsrHead, rec, entries);
    return mDNStrue;

exit:
    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
        "TSRDataRecPtrHeadAddRec: Alloc TSRDataPtrRec failed " PRI_DM_NAME " index %u", DM_NAME_PARAM(name), index);
    return mDNSfalse;
}
mDNSlocal void TSRDataRecPtrHeadRemoveAndFreeFirst(TSRDataPtrRecHead *tsrHead)
{
    mdns_require_quiet(tsrHead, exit);

    struct TSRDataPtrRec *first = SLIST_FIRST(tsrHead);
    SLIST_REMOVE_HEAD(tsrHead, entries);
    mdns_require_quiet(first, exit);

    mDNSPlatformMemFree(first);
exit:
    return;
}
mDNSlocal void TSRDataRecPtrHeadFreeList(TSRDataPtrRecHead *tsrHead)
{
    mdns_require_quiet(tsrHead, exit);
    while (!SLIST_EMPTY(tsrHead)) {
        TSRDataRecPtrHeadRemoveAndFreeFirst(tsrHead);
    }
exit:
    return;
}

// RFC 6762 defines Passive Observation Of Failures (POOF)
//
//    A host observes the multicast queries issued by the other hosts on
//    the network.  One of the major benefits of also sending responses
//    using multicast is that it allows all hosts to see the responses
//    (or lack thereof) to those queries.
//
//    If a host sees queries, for which a record in its cache would be
//    expected to be given as an answer in a multicast response, but no
//    such answer is seen, then the host may take this as an indication
//    that the record may no longer be valid.
//
//    After seeing two or more of these queries, and seeing no multicast
//    response containing the expected answer within ten seconds, then even
//    though its TTL may indicate that it is not yet due to expire, that
//    record SHOULD be flushed from the cache.
//
// <https://tools.ietf.org/html/rfc6762#section-10.5>

#define POOF_ENABLED 1

mDNSexport const char *const mDNS_DomainTypeNames[] =
{
    "b._dns-sd._udp.",      // Browse
    "db._dns-sd._udp.",     // Default Browse
    "lb._dns-sd._udp.",     // Automatic Browse
    "r._dns-sd._udp.",      // Registration
    "dr._dns-sd._udp."      // Default Registration
};

#ifdef UNICAST_DISABLED
#define uDNS_IsActiveQuery(q, u) mDNSfalse
#endif

// ***************************************************************************
// MARK: - General Utility Functions

#if MDNS_MALLOC_DEBUGGING
// When doing memory allocation debugging, this function traverses all lists in the mDNS query
// structures and caches and checks each entry in the list to make sure it's still good.
mDNSlocal void mDNS_ValidateLists(void *context)
{
    mDNS *m = context;
#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    mDNSu32 NumAllInterfaceRecords   = 0;
    mDNSu32 NumAllInterfaceQuestions = 0;
#endif

    // Check core mDNS lists
    AuthRecord                  *rr;
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        if (rr->next == (AuthRecord *)~0 || rr->resrec.RecordType == 0 || rr->resrec.RecordType == 0xFF)
            LogMemCorruption("ResourceRecords list: %p is garbage (%X)", rr, rr->resrec.RecordType);
        if (rr->resrec.name != &rr->namestorage)
            LogMemCorruption("ResourceRecords list: %p name %p does not point to namestorage %p %##s",
                             rr, rr->resrec.name->c, rr->namestorage.c, rr->namestorage.c);
#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
        if (!AuthRecord_uDNS(rr) && !RRLocalOnly(rr)) NumAllInterfaceRecords++;
#endif
    }

    for (rr = m->DuplicateRecords; rr; rr=rr->next)
    {
        if (rr->next == (AuthRecord *)~0 || rr->resrec.RecordType == 0 || rr->resrec.RecordType == 0xFF)
            LogMemCorruption("DuplicateRecords list: %p is garbage (%X)", rr, rr->resrec.RecordType);
#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
        if (!AuthRecord_uDNS(rr) && !RRLocalOnly(rr)) NumAllInterfaceRecords++;
#endif
    }

    rr = m->NewLocalRecords;
    if (rr)
        if (rr->next == (AuthRecord *)~0 || rr->resrec.RecordType == 0 || rr->resrec.RecordType == 0xFF)
            LogMemCorruption("NewLocalRecords: %p is garbage (%X)", rr, rr->resrec.RecordType);

    rr = m->CurrentRecord;
    if (rr)
        if (rr->next == (AuthRecord *)~0 || rr->resrec.RecordType == 0 || rr->resrec.RecordType == 0xFF)
            LogMemCorruption("CurrentRecord: %p is garbage (%X)", rr, rr->resrec.RecordType);

    DNSQuestion                 *q;
    for (q = m->Questions; q; q=q->next)
    {
        if (q->next == (DNSQuestion*)~0 || q->ThisQInterval == (mDNSs32) ~0)
            LogMemCorruption("Questions list: %p is garbage (%lX %p)", q, q->ThisQInterval, q->next);
        if (q->DuplicateOf && q->LocalSocket)
            LogMemCorruption("Questions list: Duplicate Question %p should not have LocalSocket set %##s (%s)", q, q->qname.c, DNSTypeName(q->qtype));
#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
        if (!LocalOnlyOrP2PInterface(q->InterfaceID) && mDNSOpaque16IsZero(q->TargetQID))
            NumAllInterfaceQuestions++;
#endif
    }

    CacheGroup                  *cg;
    CacheRecord                 *cr;
    mDNSu32 slot;
    FORALL_CACHERECORDS(slot, cg, cr)
    {
        if (cr->resrec.RecordType == 0 || cr->resrec.RecordType == 0xFF)
            LogMemCorruption("Cache slot %lu: %p is garbage (%X)", slot, cr, cr->resrec.RecordType);
        if (cr->CRActiveQuestion)
        {
            for (q = m->Questions; q; q=q->next) if (q == cr->CRActiveQuestion) break;
            if (!q) LogMemCorruption("Cache slot %lu: CRActiveQuestion %p not in m->Questions list %s", slot, cr->CRActiveQuestion, CRDisplayString(m, cr));
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    if (m->NumAllInterfaceRecords != NumAllInterfaceRecords)
    	LogMemCorruption("NumAllInterfaceRecords is %d should be %d", m->NumAllInterfaceRecords, NumAllInterfaceRecords);

    if (m->NumAllInterfaceQuestions != NumAllInterfaceQuestions)
    	LogMemCorruption("NumAllInterfaceQuestions is %d should be %d", m->NumAllInterfaceQuestions, NumAllInterfaceQuestions);
#endif // MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
}
#endif // MDNS_MALLOC_DEBUGGING

// Returns true if this is a  unique, authoritative LocalOnly record that answers questions of type
// A, AAAA , CNAME, or PTR.  The caller should answer the question with this record and not send out
// the question on the wire if LocalOnlyRecordAnswersQuestion() also returns true.
// Main use is to handle /etc/hosts records and the LocalOnly PTR records created for localhost.
#define UniqueLocalOnlyRecord(rr) ((rr)->ARType == AuthRecordLocalOnly && \
                                        (rr)->resrec.RecordType & kDNSRecordTypeUniqueMask && \
                                        ((rr)->resrec.rrtype == kDNSType_A || (rr)->resrec.rrtype == kDNSType_AAAA || \
                                         (rr)->resrec.rrtype == kDNSType_CNAME || \
                                         (rr)->resrec.rrtype == kDNSType_PTR))

mDNSlocal void SetNextQueryStopTime(mDNS *const m, const DNSQuestion *const q)
{
    mDNS_CheckLock(m);

    if (m->NextScheduledStopTime - q->StopTime > 0)
        m->NextScheduledStopTime = q->StopTime;
}

mDNSexport void SetNextQueryTime(mDNS *const m, const DNSQuestion *const q)
{
    mDNS_CheckLock(m);

    if (ActiveQuestion(q))
    {
        // Depending on whether this is a multicast or unicast question we want to set either:
        // m->NextScheduledQuery = NextQSendTime(q) or
        // m->NextuDNSEvent      = NextQSendTime(q)
        mDNSs32 *const timer = mDNSOpaque16IsZero(q->TargetQID) ? &m->NextScheduledQuery : &m->NextuDNSEvent;
        if (*timer - NextQSendTime(q) > 0)
            *timer = NextQSendTime(q);
    }
}

mDNSlocal void ReleaseAuthEntity(AuthHash *r, AuthEntity *e)
{
#if MDNS_MALLOC_DEBUGGING >= 1
    unsigned int i;
    for (i=0; i<sizeof(*e); i++) ((char*)e)[i] = 0xFF;
#endif
    e->next = r->rrauth_free;
    r->rrauth_free = e;
    r->rrauth_totalused--;
}

mDNSlocal void ReleaseAuthGroup(AuthHash *r, AuthGroup **cp)
{
    AuthEntity *e = (AuthEntity *)(*cp);
    LogMsg("ReleaseAuthGroup:  Releasing AuthGroup %##s", (*cp)->name->c);
    if ((*cp)->rrauth_tail != &(*cp)->members)
        LogMsg("ERROR: (*cp)->members == mDNSNULL but (*cp)->rrauth_tail != &(*cp)->members)");
    if ((*cp)->name != (domainname*)((*cp)->namestorage)) mDNSPlatformMemFree((*cp)->name);
    (*cp)->name = mDNSNULL;
    *cp = (*cp)->next;          // Cut record from list
    ReleaseAuthEntity(r, e);
}

mDNSlocal AuthEntity *GetAuthEntity(AuthHash *r, const AuthGroup *const PreserveAG)
{
    AuthEntity *e = mDNSNULL;

    if (r->rrauth_lock) { LogMsg("GetFreeCacheRR ERROR! Cache already locked!"); return(mDNSNULL); }
    r->rrauth_lock = 1;

    if (!r->rrauth_free)
    {
        // We allocate just one AuthEntity at a time because we need to be able
        // free them all individually which normally happens when we parse /etc/hosts into
        // AuthHash where we add the "new" entries and discard (free) the already added
        // entries. If we allocate as chunks, we can't free them individually.
        AuthEntity *storage = (AuthEntity *) mDNSPlatformMemAllocateClear(sizeof(*storage));
        storage->next = mDNSNULL;
        r->rrauth_free = storage;
    }

    // If we still have no free records, recycle all the records we can.
    // Enumerating the entire auth is moderately expensive, so when we do it, we reclaim all the records we can in one pass.
    if (!r->rrauth_free)
    {
        mDNSu32 oldtotalused = r->rrauth_totalused;
        mDNSu32 slot;
        for (slot = 0; slot < AUTH_HASH_SLOTS; slot++)
        {
            AuthGroup **cp = &r->rrauth_hash[slot];
            while (*cp)
            {
                if ((*cp)->members || (*cp)==PreserveAG) cp=&(*cp)->next;
                else ReleaseAuthGroup(r, cp);
            }
        }
        LogInfo("GetAuthEntity: Recycled %d records to reduce auth cache from %d to %d",
                oldtotalused - r->rrauth_totalused, oldtotalused, r->rrauth_totalused);
    }

    if (r->rrauth_free) // If there are records in the free list, take one
    {
        e = r->rrauth_free;
        r->rrauth_free = e->next;
        if (++r->rrauth_totalused >= r->rrauth_report)
        {
            LogInfo("RR Auth now using %ld objects", r->rrauth_totalused);
            if      (r->rrauth_report <  100) r->rrauth_report += 10;
            else if (r->rrauth_report < 1000) r->rrauth_report += 100;
            else r->rrauth_report += 1000;
        }
        mDNSPlatformMemZero(e, sizeof(*e));
    }

    r->rrauth_lock = 0;

    return(e);
}

mDNSexport AuthGroup *AuthGroupForName(AuthHash *r, const mDNSu32 namehash, const domainname *const name)
{
    AuthGroup *ag;
    const mDNSu32 slot = namehash % AUTH_HASH_SLOTS;

    for (ag = r->rrauth_hash[slot]; ag; ag=ag->next)
        if (ag->namehash == namehash && SameDomainName(ag->name, name))
            break;
    return(ag);
}

mDNSexport AuthGroup *AuthGroupForRecord(AuthHash *r, const ResourceRecord *const rr)
{
    return(AuthGroupForName(r, rr->namehash, rr->name));
}

mDNSlocal AuthGroup *GetAuthGroup(AuthHash *r, const ResourceRecord *const rr)
{
    mDNSu16 namelen = DomainNameLength(rr->name);
    AuthGroup *ag = (AuthGroup*)GetAuthEntity(r, mDNSNULL);
    const mDNSu32 slot = rr->namehash % AUTH_HASH_SLOTS;
    if (!ag)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "GetAuthGroup: Failed to allocate memory for " PRI_DM_NAME, DM_NAME_PARAM(rr->name));
        return(mDNSNULL);
    }
    ag->next         = r->rrauth_hash[slot];
    ag->namehash     = rr->namehash;
    ag->members      = mDNSNULL;
    ag->rrauth_tail  = &ag->members;
    ag->NewLocalOnlyRecords = mDNSNULL;
    if (namelen > sizeof(ag->namestorage))
        ag->name = (domainname *) mDNSPlatformMemAllocate(namelen);
    else
        ag->name = (domainname*)ag->namestorage;
    if (!ag->name)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "GetAuthGroup: Failed to allocate name storage for " PRI_DM_NAME, DM_NAME_PARAM(rr->name));
        ReleaseAuthEntity(r, (AuthEntity*)ag);
        return(mDNSNULL);
    }
    AssignDomainName(ag->name, rr->name);

    if (AuthGroupForRecord(r, rr))
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "GetAuthGroup: Already have AuthGroup for " PRI_DM_NAME, DM_NAME_PARAM(rr->name));
    }
    r->rrauth_hash[slot] = ag;
    if (AuthGroupForRecord(r, rr) != ag)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "GetAuthGroup: Not finding AuthGroup for " PRI_DM_NAME, DM_NAME_PARAM(rr->name));
    }

    return(ag);
}

// Returns the AuthGroup in which the AuthRecord was inserted
mDNSexport AuthGroup *InsertAuthRecord(mDNS *const m, AuthHash *r, AuthRecord *rr)
{
    AuthGroup *ag;

    (void)m;
    ag = AuthGroupForRecord(r, &rr->resrec);
    if (!ag) ag = GetAuthGroup(r, &rr->resrec);   // If we don't have a AuthGroup for this name, make one now
    if (ag)
    {
        *(ag->rrauth_tail) = rr;                // Append this record to tail of cache slot list
        ag->rrauth_tail = &(rr->next);          // Advance tail pointer
    }
    return ag;
}

mDNSexport AuthGroup *RemoveAuthRecord(mDNS *const m, AuthHash *r, AuthRecord *rr)
{
    AuthGroup *a;
    AuthRecord **rp;

    a = AuthGroupForRecord(r, &rr->resrec);
    if (!a)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "RemoveAuthRecord: ERROR!! AuthGroup not found for " PRI_S, ARDisplayString(m, rr));
        return mDNSNULL;
    }
    rp = &a->members;
    while (*rp)
    {
        if (*rp != rr)
            rp=&(*rp)->next;
        else
        {
            // We don't break here, so that we can set the tail below without tracking "prev" pointers

            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "RemoveAuthRecord: removing auth record " PRI_S " from table", ARDisplayString(m, rr));
            *rp = (*rp)->next;          // Cut record from list
        }
    }
    // TBD: If there are no more members, release authgroup ?
    a->rrauth_tail = rp;
    return a;
}

mDNSexport CacheGroup *CacheGroupForName(const mDNS *const m, const mDNSu32 namehash, const domainname *const name)
{
    CacheGroup *cg;
    mDNSu32    slot = HashSlotFromNameHash(namehash);
    for (cg = m->rrcache_hash[slot]; cg; cg=cg->next)
        if (cg->namehash == namehash && SameDomainName(cg->name, name))
            break;
    return(cg);
}

mDNSlocal CacheGroup *CacheGroupForRecord(const mDNS *const m, const ResourceRecord *const rr)
{
    return(CacheGroupForName(m, rr->namehash, rr->name));
}

mDNSexport mDNSBool mDNS_AddressIsLocalSubnet(mDNS *const m, const mDNSInterfaceID InterfaceID, const mDNSAddr *addr)
{
    NetworkInterfaceInfo *intf;

    if (addr->type == mDNSAddrType_IPv4)
    {
        // Normally we resist touching the NotAnInteger fields, but here we're doing tricky bitwise masking so we make an exception
        for (intf = m->HostInterfaces; intf; intf = intf->next)
            if (intf->ip.type == addr->type && intf->InterfaceID == InterfaceID && intf->McastTxRx)
                if (((intf->ip.ip.v4.NotAnInteger ^ addr->ip.v4.NotAnInteger) & intf->mask.ip.v4.NotAnInteger) == 0)
                    return(mDNStrue);
    }

    if (addr->type == mDNSAddrType_IPv6)
    {
        for (intf = m->HostInterfaces; intf; intf = intf->next)
            if (intf->ip.type == addr->type && intf->InterfaceID == InterfaceID && intf->McastTxRx)
                if ((((intf->ip.ip.v6.l[0] ^ addr->ip.v6.l[0]) & intf->mask.ip.v6.l[0]) == 0) &&
                    (((intf->ip.ip.v6.l[1] ^ addr->ip.v6.l[1]) & intf->mask.ip.v6.l[1]) == 0) &&
                    (((intf->ip.ip.v6.l[2] ^ addr->ip.v6.l[2]) & intf->mask.ip.v6.l[2]) == 0) &&
                    (((intf->ip.ip.v6.l[3] ^ addr->ip.v6.l[3]) & intf->mask.ip.v6.l[3]) == 0))
                        return(mDNStrue);
    }

    return(mDNSfalse);
}

mDNSlocal mDNSBool FollowCNAME(const DNSQuestion *const q, const ResourceRecord *const rr, const QC_result qcResult)
{
    // Do not follow CNAME if it is a remove event.
    if (qcResult == QC_rmv)
    {
        return mDNSfalse;
    }

    // Do not follow CNAME if the question asks for CNAME record.
    if (q->qtype == kDNSType_CNAME)
    {
        return mDNSfalse;
    }

    // Do not follow CNAME if the record is not a CNAME record.
    if (rr->rrtype != kDNSType_CNAME)
    {
        return mDNSfalse;
    }

    // Do not follow CNAME if the record is a negative record, which means CNAME does not exist.
    if (rr->RecordType == kDNSRecordTypePacketNegative)
    {
        return mDNSfalse;
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // Only follow the CNAME if:
    // 1. The question is not DNSSEC question, or
    // 2. The question is DNSSEC question and the CNAME being followed has been validated by DNSSEC.
    // So that we can ensure only the secure CNAME can be followed.
    if (!resource_record_as_cname_should_be_followed(rr, q))
    {
        return mDNSfalse;
    }
#endif

    return mDNStrue;
}

// Caller should hold the lock
mDNSlocal void GenerateNegativeResponseEx(mDNS *const m, const mDNSInterfaceID InterfaceID, const QC_result qc,
    const mDNSOpaque16 responseFlags)
{
    DNSQuestion *q;
    if (!m->CurrentQuestion) { LogMsg("GenerateNegativeResponse: ERROR!! CurrentQuestion not set"); return; }
    q = m->CurrentQuestion;
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
        "[Q%d] GenerateNegativeResponse: Generating negative response for question", mDNSVal16(q->TargetQID));

    MakeNegativeCacheRecordForQuestion(m, &m->rec.r, q, 60, InterfaceID, responseFlags);

    // We need to force the response through in the following cases
    //
    //  a) SuppressUnusable questions that are suppressed
    //  b) Append search domains and retry the question
    //
    // The question may not have set Intermediates in which case we don't deliver negative responses. So, to force
    // through we use "QC_forceresponse".
    AnswerCurrentQuestionWithResourceRecord(m, &m->rec.r, qc);
    if (m->CurrentQuestion == q) { q->ThisQInterval = 0; }              // Deactivate this question
    // Don't touch the question after this
    mDNSCoreResetRecord(m);
}
#define GenerateNegativeResponse(M, INTERFACE_ID, QC) GenerateNegativeResponseEx(M, INTERFACE_ID, QC, zeroID)

mDNSexport void AnswerQuestionByFollowingCNAME(mDNS *const m, DNSQuestion *q, ResourceRecord *rr)
{
    const domainname *const cname = &rr->rdata->u.name;
    const mDNSBool selfref = SameDomainName(&q->qname, &rr->rdata->u.name);
    if (q->CNAMEReferrals >= 10 || selfref)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[Q%u] AnswerQuestionByFollowingCNAME: Not following CNAME referral -- "
            "CNAME: " PRI_DM_NAME ", referral count: %u, self referential: " PUB_BOOL,
            mDNSVal16(q->TargetQID), DM_NAME_PARAM_NONNULL(cname), q->CNAMEReferrals, BOOL_PARAM(selfref));
    }
    else
    {
        UDPSocket *sock = q->LocalSocket;
        mDNSOpaque16 id = q->TargetQID;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
        DNSMetrics metrics;
#endif

        q->LocalSocket = mDNSNULL;

        // The SameDomainName check above is to ignore bogus CNAME records that point right back at
        // themselves. Without that check we can get into a case where we have two duplicate questions,
        // A and B, and when we stop question A, UpdateQuestionDuplicates copies the value of CNAMEReferrals
        // from A to B, and then A is re-appended to the end of the list as a duplicate of B (because
        // the target name is still the same), and then when we stop question B, UpdateQuestionDuplicates
        // copies the B's value of CNAMEReferrals back to A, and we end up not incrementing CNAMEReferrals
        // for either of them. This is not a problem for CNAME loops of two or more records because in
        // those cases the newly re-appended question A has a different target name and therefore cannot be
        // a duplicate of any other question ('B') which was itself a duplicate of the previous question A.

        // Right now we just stop and re-use the existing query. If we really wanted to be 100% perfect,
        // and track CNAMEs coming and going, we should really create a subordinate query here,
        // which we would subsequently cancel and retract if the CNAME referral record were removed.
        // In reality this is such a corner case we'll ignore it until someone actually needs it.

        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[Q%u] AnswerQuestionByFollowingCNAME: following CNAME referral -- "
            "CNAME: " PRI_DM_NAME ", referral count: %u",
            mDNSVal16(q->TargetQID), DM_NAME_PARAM_NONNULL(cname), q->CNAMEReferrals);

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
        metrics = q->metrics;
        // The metrics will be transplanted to the restarted question, so zero out the old copy instead of using
        // DNSMetricsClear(), which will free any pointers to allocated memory.
        mDNSPlatformMemZero(&q->metrics, sizeof(q->metrics));
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        mdns_client_t client = mDNSNULL;
        const mDNSBool handleRestart = !mDNSOpaque16IsZero(q->TargetQID);
        if (handleRestart)
        {
            client = Querier_HandlePreCNAMERestart(q);
        }
#endif
        mDNS_StopQuery_internal(m, q);                              // Stop old query
        AssignDomainName(&q->qname, &rr->rdata->u.name);            // Update qname
        q->qnamehash = DomainNameHashValue(&q->qname);              // and namehash
        // If a unicast query results in a CNAME that points to a .local, we need to re-try
        // this as unicast. Setting the mDNSInterface_Unicast tells mDNS_StartQuery_internal
        // to try this as unicast query even though it is a .local name
        if (!mDNSOpaque16IsZero(q->TargetQID) && IsLocalDomain(&q->qname))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[Q%d] AnswerQuestionByFollowingCNAME: Resolving a .local CNAME -- CNAME: " PRI_DM_NAME,
                mDNSVal16(q->TargetQID), DM_NAME_PARAM_NONNULL(cname));
            q->IsUnicastDotLocal = mDNStrue;
        }
        q->CNAMEReferrals += 1;                                     // Increment value before calling mDNS_StartQuery_internal
        const mDNSu8 c = q->CNAMEReferrals;                         // Stash a copy of the new q->CNAMEReferrals value
        mDNS_StartQuery_internal(m, q);                             // start new query
        // Record how many times we've done this. We need to do this *after* mDNS_StartQuery_internal,
        // because mDNS_StartQuery_internal re-initializes CNAMEReferrals to zero
        q->CNAMEReferrals = c;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
        q->metrics = metrics;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        if (handleRestart)
        {
            Querier_HandlePostCNAMERestart(q, client);
        }
        mdns_forget(&client);
#endif
        if (sock)
        {
            // If our new query is a duplicate, then it can't have a socket of its own, so we have to close the one we saved.
            if (q->DuplicateOf) mDNSPlatformUDPClose(sock);
            else
            {
                // Transplant the old socket into the new question, and copy the query ID across too.
                // No need to close the old q->LocalSocket value because it won't have been created yet (they're made lazily on-demand).
                q->LocalSocket = sock;
                q->TargetQID = id;
            }
        }
    }
}

#ifdef USE_LIBIDN

#include <unicode/uidna.h>

#define DEBUG_PUNYCODE 0

mDNSlocal mDNSu8 *PunycodeConvert(const mDNSu8 *const src, mDNSu8 *const dst, const mDNSu8 *const end)
{
    UErrorCode errorCode = U_ZERO_ERROR;
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;
    UIDNA *uts46 = uidna_openUTS46(UIDNA_USE_STD3_RULES|UIDNA_NONTRANSITIONAL_TO_UNICODE, &errorCode);
    int32_t len = uidna_nameToASCII_UTF8(uts46, (const char *)src+1, src[0], (char *)dst+1, (int32_t)(end-(dst+1)), &info, &errorCode);
    uidna_close(uts46);
    #if DEBUG_PUNYCODE
    if (errorCode) LogMsg("uidna_nameToASCII_UTF8(%##s) failed errorCode %d", src, errorCode);
    if (info.errors) LogMsg("uidna_nameToASCII_UTF8(%##s) failed info.errors 0x%08X", src, info.errors);
    if (len > MAX_DOMAIN_LABEL) LogMsg("uidna_nameToASCII_UTF8(%##s) result too long %d", src, len);
    #endif
    if (errorCode || info.errors || len > MAX_DOMAIN_LABEL) return mDNSNULL;
    *dst = (mDNSu8)len;
    return(dst + 1 + len);
}

mDNSlocal mDNSBool IsHighASCIILabel(const mDNSu8 *d)
{
    int i;
    for (i=1; i<=d[0]; i++) if (d[i] & 0x80) return mDNStrue;
    return mDNSfalse;
}

mDNSlocal const mDNSu8 *FindLastHighASCIILabel(const domainname *const d)
{
    const mDNSu8 *ptr = d->c;
    const mDNSu8 *ans = mDNSNULL;
    while (ptr[0])
    {
        const mDNSu8 *const next = ptr + 1 + ptr[0];
        if (ptr[0] > MAX_DOMAIN_LABEL || next >= d->c + MAX_DOMAIN_NAME) return mDNSNULL;
        if (IsHighASCIILabel(ptr)) ans = ptr;
        ptr = next;
    }
    return ans;
}

mDNSlocal mDNSBool PerformNextPunycodeConversion(const DNSQuestion *const q, domainname *const newname)
{
    const mDNSu8 *h = FindLastHighASCIILabel(&q->qname);
    #if DEBUG_PUNYCODE
    LogMsg("PerformNextPunycodeConversion: %##s (%s) Last High-ASCII Label %##s", q->qname.c, DNSTypeName(q->qtype), h);
    #endif
    if (!h) return mDNSfalse;  // There are no high-ascii labels to convert

    mDNSu8 *const dst = PunycodeConvert(h, newname->c + (h - q->qname.c), newname->c + MAX_DOMAIN_NAME);
    if (!dst)
        return mDNSfalse;  // The label was not convertible to Punycode
    else
    {
        // If Punycode conversion of final eligible label was successful, copy the rest of the domainname
        const mDNSu8 *const src = h + 1 + h[0];
        const mDNSu16 remainder  = DomainNameLength((const domainname*)src);
        if (dst + remainder > newname->c + MAX_DOMAIN_NAME) return mDNSfalse;  // Name too long -- cannot be converted to Punycode

        mDNSPlatformMemCopy(newname->c, q->qname.c, (mDNSu32)(h - q->qname.c));  // Fill in the leading part
        mDNSPlatformMemCopy(dst, src, remainder);                     // Fill in the trailing part
        #if DEBUG_PUNYCODE
        LogMsg("PerformNextPunycodeConversion: %##s converted to %##s", q->qname.c, newname->c);
        #endif
        return mDNStrue;
    }
}

#endif // USE_LIBIDN

// For a single given DNSQuestion pointed to by CurrentQuestion, deliver an add/remove result for the single given AuthRecord
// Note: All the callers should use the m->CurrentQuestion to see if the question is still valid or not
mDNSlocal void AnswerLocalQuestionWithLocalAuthRecord(mDNS *const m, AuthRecord *rr, QC_result AddRecord)
{
    DNSQuestion *q = m->CurrentQuestion;
    mDNSBool followcname;

    if (!q)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "AnswerLocalQuestionWithLocalAuthRecord: ERROR!! CurrentQuestion NULL while answering with " PUB_S,
            ARDisplayString(m, rr));
        return;
    }

    followcname = FollowCNAME(q, &rr->resrec, AddRecord);

    // We should not be delivering results for record types Unregistered, Deregistering, and (unverified) Unique
    if (!(rr->resrec.RecordType & kDNSRecordTypeActiveMask))
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "AnswerLocalQuestionWithLocalAuthRecord: *NOT* delivering " PUB_S " event for local record type %X "
            PRI_S, AddRecord ? "Add" : "Rmv", rr->resrec.RecordType, ARDisplayString(m, rr));
        return;
    }

    // Indicate that we've given at least one positive answer for this record, so we should be prepared to send a goodbye for it
    if (AddRecord) rr->AnsweredLocalQ = mDNStrue;
    mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
    if (q->QuestionCallback)
    {
        q->CurrentAnswers += AddRecord ? 1 : -1;
        if (UniqueLocalOnlyRecord(rr))
        {
            if (!followcname || q->ReturnIntermed)
            {
                // Don't send this packet on the wire as we answered from /etc/hosts
                q->ThisQInterval = 0;
                q->LOAddressAnswers += AddRecord ? 1 : -1;
                q->QuestionCallback(m, q, &rr->resrec, AddRecord);
            }
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
            // The callback above could have caused the question to stop. Detect that
            // using m->CurrentQuestion
            if (followcname && m->CurrentQuestion == q)
                AnswerQuestionByFollowingCNAME(m, q, &rr->resrec);
            return;
        }
        else
        {
            q->QuestionCallback(m, q, &rr->resrec, AddRecord);
        }
    }
    mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
}

mDNSlocal void AnswerInterfaceAnyQuestionsWithLocalAuthRecord(mDNS *const m, AuthRecord *ar, QC_result AddRecord)
{
    if (m->CurrentQuestion)
        LogMsg("AnswerInterfaceAnyQuestionsWithLocalAuthRecord: ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;
    while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
    {
        mDNSBool answered;
        DNSQuestion *q = m->CurrentQuestion;
        if (RRAny(ar))
            answered = AuthRecordAnswersQuestion(ar, q);
        else
            answered = LocalOnlyRecordAnswersQuestion(ar, q);
        if (answered)
            AnswerLocalQuestionWithLocalAuthRecord(m, ar, AddRecord);       // MUST NOT dereference q again
        if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
            m->CurrentQuestion = q->next;
    }
    m->CurrentQuestion = mDNSNULL;
}

// When a new local AuthRecord is created or deleted, AnswerAllLocalQuestionsWithLocalAuthRecord()
// delivers the appropriate add/remove events to listening questions:
// 1. It runs though all our LocalOnlyQuestions delivering answers as appropriate,
//    stopping if it reaches a NewLocalOnlyQuestion -- brand-new questions are handled by AnswerNewLocalOnlyQuestion().
// 2. If the AuthRecord is marked mDNSInterface_LocalOnly or mDNSInterface_P2P, then it also runs though
//    our main question list, delivering answers to mDNSInterface_Any questions as appropriate,
//    stopping if it reaches a NewQuestion -- brand-new questions are handled by AnswerNewQuestion().
//
// AnswerAllLocalQuestionsWithLocalAuthRecord is used by the m->NewLocalRecords loop in mDNS_Execute(),
// and by mDNS_Deregister_internal()

mDNSlocal void AnswerAllLocalQuestionsWithLocalAuthRecord(mDNS *const m, AuthRecord *ar, QC_result AddRecord)
{
    if (m->CurrentQuestion)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "AnswerAllLocalQuestionsWithLocalAuthRecord ERROR m->CurrentQuestion already set: " PRI_DM_NAME
            " (" PUB_S ")", DM_NAME_PARAM(&m->CurrentQuestion->qname), DNSTypeName(m->CurrentQuestion->qtype));
    }

    m->CurrentQuestion = m->LocalOnlyQuestions;
    while (m->CurrentQuestion && m->CurrentQuestion != m->NewLocalOnlyQuestions)
    {
        mDNSBool answered;
        DNSQuestion *q = m->CurrentQuestion;
        // We are called with both LocalOnly/P2P record or a regular AuthRecord
        if (RRAny(ar))
            answered = AuthRecordAnswersQuestion(ar, q);
        else
            answered = LocalOnlyRecordAnswersQuestion(ar, q);
        if (answered)
            AnswerLocalQuestionWithLocalAuthRecord(m, ar, AddRecord);           // MUST NOT dereference q again
        if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
            m->CurrentQuestion = q->next;
    }

    m->CurrentQuestion = mDNSNULL;

    // If this AuthRecord is marked LocalOnly or P2P, then we want to deliver it to all local 'mDNSInterface_Any' questions
    if (ar->ARType == AuthRecordLocalOnly || ar->ARType == AuthRecordP2P)
        AnswerInterfaceAnyQuestionsWithLocalAuthRecord(m, ar, AddRecord);

}

// ***************************************************************************
// MARK: - Resource Record Utility Functions

#define RRTypeIsAddressType(T) ((T) == kDNSType_A || (T) == kDNSType_AAAA)

mDNSlocal mDNSBool ResourceRecordIsValidAnswer(const AuthRecord *const rr)
{
    if ((rr->resrec.RecordType & kDNSRecordTypeActiveMask) &&
        ((rr->Additional1 == mDNSNULL) || (rr->Additional1->resrec.RecordType & kDNSRecordTypeActiveMask)) &&
        ((rr->Additional2 == mDNSNULL) || (rr->Additional2->resrec.RecordType & kDNSRecordTypeActiveMask)) &&
        ((rr->DependentOn == mDNSNULL) || (rr->DependentOn->resrec.RecordType & kDNSRecordTypeActiveMask)))
    {
        return mDNStrue;
    }
    else
    {
        return mDNSfalse;
    }
}

mDNSlocal mDNSBool IsInterfaceValidForAuthRecord(const AuthRecord *const rr, const mDNSInterfaceID InterfaceID)
{
    if (rr->resrec.InterfaceID == mDNSInterface_Any)
    {
        return mDNSPlatformValidRecordForInterface(rr, InterfaceID);
    }
    else
    {
        return ((rr->resrec.InterfaceID == InterfaceID) ? mDNStrue : mDNSfalse);
    }
}

mDNSlocal mDNSBool ResourceRecordIsValidInterfaceAnswer(const AuthRecord *const rr, const mDNSInterfaceID interfaceID)
{
    return ((IsInterfaceValidForAuthRecord(rr, interfaceID) && ResourceRecordIsValidAnswer(rr)) ? mDNStrue : mDNSfalse);
}

#define DefaultProbeCountForTypeUnique ((mDNSu8)3)
#define DefaultProbeCountForRecordType(X)      ((X) == kDNSRecordTypeUnique ? DefaultProbeCountForTypeUnique : (mDNSu8)0)

// Parameters for handling probing conflicts
#define kMaxAllowedMCastProbingConflicts 1                     // Maximum number of conflicts to allow from mcast messages.
#define kProbingConflictPauseDuration    mDNSPlatformOneSecond // Duration of probing pause after an allowed mcast conflict.

// See RFC 6762: "8.3 Announcing"
// "The Multicast DNS responder MUST send at least two unsolicited responses, one second apart."
// Send 4, which is really 8 since we send on both IPv4 and IPv6.
#define InitialAnnounceCount ((mDNSu8)4)

// For goodbye packets we set the count to 3, and for wakeups we set it to 18
// (which will be up to 15 wakeup attempts over the course of 30 seconds,
// and then if the machine fails to wake, 3 goodbye packets).
#define GoodbyeCount ((mDNSu8)3)
#define WakeupCount ((mDNSu8)18)
#define MAX_PROBE_RESTARTS ((mDNSu8)20)
#define MAX_GHOST_TIME ((mDNSs32)((60*60*24*7)*mDNSPlatformOneSecond))  //  One week

// Number of wakeups we send if WakeOnResolve is set in the question
#define InitialWakeOnResolveCount ((mDNSu8)3)

// Note that the announce intervals use exponential backoff, doubling each time. The probe intervals do not.
// This means that because the announce interval is doubled after sending the first packet, the first
// observed on-the-wire inter-packet interval between announcements is actually one second.
// The half-second value here may be thought of as a conceptual (non-existent) half-second delay *before* the first packet is sent.
#define DefaultProbeIntervalForTypeUnique (mDNSPlatformOneSecond/4)
#define DefaultAnnounceIntervalForTypeShared (mDNSPlatformOneSecond/2)
#define DefaultAnnounceIntervalForTypeUnique (mDNSPlatformOneSecond/2)

#define DefaultAPIntervalForRecordType(X)  ((X) &kDNSRecordTypeActiveSharedMask ? DefaultAnnounceIntervalForTypeShared : \
                                            (X) &kDNSRecordTypeUnique           ? DefaultProbeIntervalForTypeUnique    : \
                                            (X) &kDNSRecordTypeActiveUniqueMask ? DefaultAnnounceIntervalForTypeUnique : 0)

#define TimeToAnnounceThisRecord(RR,time) ((RR)->AnnounceCount && (time) - ((RR)->LastAPTime + (RR)->ThisAPInterval) >= 0)
mDNSexport mDNSs32 RRExpireTime(const CacheRecord *const cr)
{
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) || MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
    return cr->DNSPushSubscribed ? (mDNSStorage.timenow + FutureTime) : (cr->TimeRcvd + TicksTTL(cr));
#else
    return cr->TimeRcvd + TicksTTL(cr);
#endif
}

// ResourceRecordNameClassInterfaceMatch returns true if two resource records have the same InterfaceID, class and name.
mDNSlocal mDNSBool ResourceRecordNameClassInterfaceMatch(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
    if (!r1 || !r2)
    {
        return mDNSfalse;
    }
    if (r1->InterfaceID && r2->InterfaceID && r1->InterfaceID != r2->InterfaceID)
    {
        return mDNSfalse;
    }
    return (mDNSBool)(r1->rrclass  == r2->rrclass && r1->namehash == r2->namehash && SameDomainName(r1->name, r2->name));
}

// SameResourceRecordSignature returns true if two resources records have the same name, type, and class, and may be sent
// (or were received) on the same interface (i.e. if *both* records specify an interface, then it has to match).
// TTL and rdata may differ.
// This is used for cache flush management:
// When sending a unique record, all other records matching "SameResourceRecordSignature" must also be sent
// When receiving a unique record, all old cache records matching "SameResourceRecordSignature" are flushed

// SameResourceRecordNameClassInterface is functionally the same as SameResourceRecordSignature, except rrtype does not have to match

#define SameResourceRecordSignature(A,B) (A)->resrec.rrtype == (B)->resrec.rrtype && SameResourceRecordNameClassInterface((A),(B))

mDNSBool SameResourceRecordNameClassInterface(const AuthRecord *const r1, const AuthRecord *const r2)
{
    if (!r1) { LogMsg("SameResourceRecordSignature ERROR: r1 is NULL"); return(mDNSfalse); }
    if (!r2) { LogMsg("SameResourceRecordSignature ERROR: r2 is NULL"); return(mDNSfalse); }
    return ResourceRecordNameClassInterfaceMatch(&r1->resrec, &r2->resrec);
}

// PacketRRMatchesSignature behaves as SameResourceRecordSignature, except that types may differ if our
// authoratative record is unique (as opposed to shared). For unique records, we are supposed to have
// complete ownership of *all* types for this name, so *any* record type with the same name is a conflict.
// In addition, when probing we send our questions with the wildcard type kDNSQType_ANY,
// so a response of any type should match, even if it is not actually the type the client plans to use.

// For now, to make it easier to avoid false conflicts, we treat SPS Proxy records like shared records,
// and require the rrtypes to match for the rdata to be considered potentially conflicting
mDNSlocal mDNSBool PacketRRMatchesSignature(const CacheRecord *const pktrr, const AuthRecord *const authrr)
{
    if (!pktrr)  { LogMsg("PacketRRMatchesSignature ERROR: pktrr is NULL"); return(mDNSfalse); }
    if (!authrr) { LogMsg("PacketRRMatchesSignature ERROR: authrr is NULL"); return(mDNSfalse); }
    if (pktrr->resrec.InterfaceID &&
        authrr->resrec.InterfaceID &&
        pktrr->resrec.InterfaceID != authrr->resrec.InterfaceID) return(mDNSfalse);
    if (!(authrr->resrec.RecordType & kDNSRecordTypeUniqueMask) || authrr->WakeUp.HMAC.l[0])
        if (pktrr->resrec.rrtype != authrr->resrec.rrtype) return(mDNSfalse);
    if ((authrr->resrec.InterfaceID == mDNSInterface_Any) &&
        !mDNSPlatformValidRecordForInterface(authrr, pktrr->resrec.InterfaceID)) return(mDNSfalse);
    return (mDNSBool)(
               pktrr->resrec.rrclass == authrr->resrec.rrclass &&
               pktrr->resrec.namehash == authrr->resrec.namehash &&
               SameDomainName(pktrr->resrec.name, authrr->resrec.name));
}

// CacheRecord *ka is the CacheRecord from the known answer list in the query.
// This is the information that the requester believes to be correct.
// AuthRecord *rr is the answer we are proposing to give, if not suppressed.
// This is the information that we believe to be correct.
// We've already determined that we plan to give this answer on this interface
// (either the record is non-specific, or it is specific to this interface)
// so now we just need to check the name, type, class, rdata and TTL.
mDNSlocal mDNSBool ShouldSuppressKnownAnswer(const CacheRecord *const ka, const AuthRecord *const rr)
{
    // If RR signature is different, or data is different, then don't suppress our answer
    if (!IdenticalResourceRecord(&ka->resrec, &rr->resrec)) return(mDNSfalse);

    // If the requester's indicated TTL is less than half the real TTL,
    // we need to give our answer before the requester's copy expires.
    // If the requester's indicated TTL is at least half the real TTL,
    // then we can suppress our answer this time.
    // If the requester's indicated TTL is greater than the TTL we believe,
    // then that's okay, and we don't need to do anything about it.
    // (If two responders on the network are offering the same information,
    // that's okay, and if they are offering the information with different TTLs,
    // the one offering the lower TTL should defer to the one offering the higher TTL.)
    return (mDNSBool)(ka->resrec.rroriginalttl >= rr->resrec.rroriginalttl / 2);
}

mDNSlocal void SetNextAnnounceProbeTime(mDNS *const m, const AuthRecord *const rr)
{
    if (rr->resrec.RecordType == kDNSRecordTypeUnique)
    {
        if ((rr->LastAPTime + rr->ThisAPInterval) - m->timenow > mDNSPlatformOneSecond * 10)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SetNextAnnounceProbeTime: ProbeCount %d Next in %d " PRI_S,
                rr->ProbeCount, (rr->LastAPTime + rr->ThisAPInterval) - m->timenow, ARDisplayString(m, rr));
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SetNextAnnounceProbeTime: m->SuppressProbes %d m->timenow %d diff %d",
                m->SuppressProbes, m->timenow, m->SuppressProbes - m->timenow);
        }
        if (m->NextScheduledProbe - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
            m->NextScheduledProbe = (rr->LastAPTime + rr->ThisAPInterval);
        // Some defensive code:
        // If (rr->LastAPTime + rr->ThisAPInterval) happens to be far in the past, we don't want to allow
        // NextScheduledProbe to be set excessively in the past, because that can cause bad things to happen.
        // See: <rdar://problem/7795434> mDNS: Sometimes advertising stops working and record interval is set to zero
        if (m->NextScheduledProbe - m->timenow < 0)
            m->NextScheduledProbe = m->timenow;
    }
    else if (rr->AnnounceCount && (ResourceRecordIsValidAnswer(rr) || rr->resrec.RecordType == kDNSRecordTypeDeregistering))
    {
        if (m->NextScheduledResponse - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
            m->NextScheduledResponse = (rr->LastAPTime + rr->ThisAPInterval);
    }
}

mDNSlocal void InitializeLastAPTime(mDNS *const m, AuthRecord *const rr)
{
    // For reverse-mapping Sleep Proxy PTR records, probe interval is one second
    rr->ThisAPInterval = rr->AddressProxy.type ? mDNSPlatformOneSecond : DefaultAPIntervalForRecordType(rr->resrec.RecordType);

    // * If this is a record type that's going to probe, then we use the m->SuppressProbes time.
    // * Otherwise, if it's not going to probe, but m->SuppressProbes is set because we have other
    //   records that are going to probe, then we delay its first announcement so that it will
    //   go out synchronized with the first announcement for the other records that *are* probing.
    //   This is a minor performance tweak that helps keep groups of related records synchronized together.
    //   The addition of "interval / 2" is to make sure that, in the event that any of the probes are
    //   delayed by a few milliseconds, this announcement does not inadvertently go out *before* the probing is complete.
    //   When the probing is complete and those records begin to announce, these records will also be picked up and accelerated,
    //   because they will meet the criterion of being at least half-way to their scheduled announcement time.
    // * If it's not going to probe and m->SuppressProbes is not already set then we should announce immediately.

    if (rr->ProbeCount)
    {
        rr->ProbingConflictCount = 0;
        // If we have no probe suppression time set, or it is in the past, set it now
        if (m->SuppressProbes == 0 || m->SuppressProbes - m->timenow < 0)
        {
            // To allow us to aggregate probes when a group of services are registered together,
            // the first probe is delayed by a random delay in the range 1/8 to 1/4 second.
            // This means the common-case behaviour is:
            // randomized wait; probe
            // 1/4 second wait; probe
            // 1/4 second wait; probe
            // 1/4 second wait; announce (i.e. service is normally announced 7/8 to 1 second after being registered)
            m->SuppressProbes = NonZeroTime(m->timenow + DefaultProbeIntervalForTypeUnique/2 + mDNSRandom(DefaultProbeIntervalForTypeUnique/2));

            // If we already have a *probe* scheduled to go out sooner, then use that time to get better aggregation
            if (m->SuppressProbes - m->NextScheduledProbe >= 0)
                m->SuppressProbes = NonZeroTime(m->NextScheduledProbe);
            if (m->SuppressProbes - m->timenow < 0)     // Make sure we don't set m->SuppressProbes excessively in the past
                m->SuppressProbes = m->timenow;

            // If we already have a *query* scheduled to go out sooner, then use that time to get better aggregation
            if (m->SuppressProbes - m->NextScheduledQuery >= 0)
                m->SuppressProbes = NonZeroTime(m->NextScheduledQuery);
            if (m->SuppressProbes - m->timenow < 0)     // Make sure we don't set m->SuppressProbes excessively in the past
                m->SuppressProbes = m->timenow;

            // except... don't expect to be able to send before the m->SuppressQueries timer fires
            if (m->SuppressQueries && ((m->SuppressProbes - m->SuppressQueries) < 0))
            {
                m->SuppressProbes = NonZeroTime(m->SuppressQueries);
            }
            if (m->SuppressProbes - m->timenow > mDNSPlatformOneSecond * 8)
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                    "InitializeLastAPTime ERROR m->SuppressProbes %d m->NextScheduledProbe %d "
                    "m->NextScheduledQuery %d m->SuppressQueries %d %d",
                    m->SuppressProbes - m->timenow, m->NextScheduledProbe - m->timenow,
                    m->NextScheduledQuery - m->timenow, m->SuppressQueries, m->SuppressQueries - m->timenow);
                m->SuppressProbes = NonZeroTime(m->timenow + DefaultProbeIntervalForTypeUnique/2 + mDNSRandom(DefaultProbeIntervalForTypeUnique/2));
            }
        }
        rr->LastAPTime = m->SuppressProbes - rr->ThisAPInterval;
    }
    // Skip kDNSRecordTypeKnownUnique and kDNSRecordTypeShared records here and set their LastAPTime in the "else" block below so
    // that they get announced immediately, otherwise, their announcement would be delayed until the based on the SuppressProbes value.
    else if ((rr->resrec.RecordType != kDNSRecordTypeKnownUnique) && (rr->resrec.RecordType != kDNSRecordTypeShared) && m->SuppressProbes && (m->SuppressProbes - m->timenow >= 0))
        rr->LastAPTime = m->SuppressProbes - rr->ThisAPInterval + DefaultProbeIntervalForTypeUnique * DefaultProbeCountForTypeUnique + rr->ThisAPInterval / 2;
    else
        rr->LastAPTime = m->timenow - rr->ThisAPInterval;

    // For reverse-mapping Sleep Proxy PTR records we don't want to start probing instantly -- we
    // wait one second to give the client a chance to go to sleep, and then start our ARP/NDP probing.
    // After three probes one second apart with no answer, we conclude the client is now sleeping
    // and we can begin broadcasting our announcements to take over ownership of that IP address.
    // If we don't wait for the client to go to sleep, then when the client sees our ARP Announcements there's a risk
    // (depending on the OS and networking stack it's using) that it might interpret it as a conflict and change its IP address.
    if (rr->AddressProxy.type)
        rr->LastAPTime = m->timenow;

    // Set LastMCTime to now, to inhibit multicast responses
    // (no need to send additional multicast responses when we're announcing anyway)
    rr->LastMCTime      = m->timenow;
    rr->LastMCInterface = mDNSInterfaceMark;

    SetNextAnnounceProbeTime(m, rr);
}

mDNSlocal const domainname *SetUnicastTargetToHostName(mDNS *const m, AuthRecord *rr)
{
    const domainname *target;
    if (rr->AutoTarget)
    {
        rr->AutoTarget = Target_AutoHostAndNATMAP;
    }

    target = GetServiceTarget(m, rr);
    if (!target || target->c[0] == 0)
    {
        // defer registration until we've got a target
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SetUnicastTargetToHostName No target for " PRI_S, ARDisplayString(m, rr));
        rr->state = regState_NoTarget;
        return mDNSNULL;
    }
    else
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SetUnicastTargetToHostName target " PRI_DM_NAME " for resource record " PUB_S,
            DM_NAME_PARAM(target), ARDisplayString(m,rr));
        return target;
    }
}

#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
mDNSlocal mDNSBool AuthRecordIncludesOrIsAWDL(const AuthRecord *const ar)
{
    return ((AuthRecordIncludesAWDL(ar) || mDNSPlatformInterfaceIsAWDL(ar->resrec.InterfaceID)) ? mDNStrue : mDNSfalse);
}
#endif

// Right now this only applies to mDNS (.local) services where the target host is always m->MulticastHostname
// Eventually we should unify this with GetServiceTarget() in uDNS.c
mDNSlocal void SetTargetToHostName(mDNS *const m, AuthRecord *const rr)
{
    domainname *const target = GetRRDomainNameTarget(&rr->resrec);
    const domainname *newname;

    if (0) {}
    else if (rr->resrec.InterfaceID == mDNSInterface_LocalOnly)
    {
        newname = (const domainname *)"\x9" "localhost";
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    else if (AuthRecordIncludesOrIsAWDL(rr))
    {
        newname = &m->RandomizedHostname;
    }
#endif
    else
    {
        newname = &m->MulticastHostname;
    }
    if (!target)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SetTargetToHostName: Don't know how to set the target of rrtype " PUB_S,
            DNSTypeName(rr->resrec.rrtype));
    }

    if (!(rr->ForceMCast || rr->ARType == AuthRecordLocalOnly || rr->ARType == AuthRecordP2P || IsLocalDomain(&rr->namestorage)))
    {
        const domainname *const n = SetUnicastTargetToHostName(m, rr);
        if (n) newname = n;
        else { if (target) target->c[0] = 0; SetNewRData(&rr->resrec, mDNSNULL, 0); return; }
    }

    if (target && SameDomainName(target, newname))
        debugf("SetTargetToHostName: Target of %##s is already %##s", rr->resrec.name->c, target->c);

    if (target && !SameDomainName(target, newname))
    {
        AssignDomainName(target, newname);
        SetNewRData(&rr->resrec, mDNSNULL, 0);      // Update rdlength, rdestimate, rdatahash

        // If we're in the middle of probing this record, we need to start again,
        // because changing its rdata may change the outcome of the tie-breaker.
        // (If the record type is kDNSRecordTypeUnique (unconfirmed unique) then DefaultProbeCountForRecordType is non-zero.)
        rr->ProbeCount     = DefaultProbeCountForRecordType(rr->resrec.RecordType);

        // If we've announced this record, we really should send a goodbye packet for the old rdata before
        // changing to the new rdata. However, in practice, we only do SetTargetToHostName for unique records,
        // so when we announce them we'll set the kDNSClass_UniqueRRSet and clear any stale data that way.
        if (rr->RequireGoodbye && rr->resrec.RecordType == kDNSRecordTypeShared)
            debugf("Have announced shared record %##s (%s) at least once: should have sent a goodbye packet before updating",
                   rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));

        rr->AnnounceCount  = InitialAnnounceCount;
        rr->RequireGoodbye = mDNSfalse;
        rr->ProbeRestartCount = 0;
        InitializeLastAPTime(m, rr);
    }
}

mDNSlocal void AcknowledgeRecord(mDNS *const m, AuthRecord *const rr)
{
    if (rr->RecordCallback)
    {
        // CAUTION: MUST NOT do anything more with rr after calling rr->Callback(), because the client's callback function
        // is allowed to do anything, including starting/stopping queries, registering/deregistering records, etc.
        rr->Acknowledged = mDNStrue;
        mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
        rr->RecordCallback(m, rr, mStatus_NoError);
        mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
    }
}

mDNSexport void ActivateUnicastRegistration(mDNS *const m, AuthRecord *const rr)
{
    // Make sure that we don't activate the SRV record and associated service records, if it is in
    // NoTarget state. First time when a service is being instantiated, SRV record may be in NoTarget state.
    // We should not activate any of the other reords (PTR, TXT) that are part of the service. When
    // the target becomes available, the records will be reregistered.
    if (rr->resrec.rrtype != kDNSType_SRV)
    {
        AuthRecord *srvRR = mDNSNULL;
        if (rr->resrec.rrtype == kDNSType_PTR)
            srvRR = rr->Additional1;
        else if (rr->resrec.rrtype == kDNSType_TXT)
            srvRR = rr->DependentOn;
        if (srvRR)
        {
            if (srvRR->resrec.rrtype != kDNSType_SRV)
            {
                LogMsg("ActivateUnicastRegistration: ERROR!! Resource record %s wrong, expecting SRV type", ARDisplayString(m, srvRR));
            }
            else
            {
                LogInfo("ActivateUnicastRegistration: Found Service Record %s in state %d for %##s (%s)",
                        ARDisplayString(m, srvRR), srvRR->state, rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));
                rr->state = srvRR->state;
            }
        }
    }

    if (rr->state == regState_NoTarget)
    {
        LogInfo("ActivateUnicastRegistration record %s in regState_NoTarget, not activating", ARDisplayString(m, rr));
        return;
    }
    // When we wake up from sleep, we call ActivateUnicastRegistration. It is possible that just before we went to sleep,
    // the service/record was being deregistered. In that case, we should not try to register again. For the cases where
    // the records are deregistered due to e.g., no target for the SRV record, we would have returned from above if it
    // was already in NoTarget state. If it was in the process of deregistration but did not complete fully before we went
    // to sleep, then it is okay to start in Pending state as we will go back to NoTarget state if we don't have a target.
    if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
    {
        LogInfo("ActivateUnicastRegistration: Resource record %s, current state %d, moving to DeregPending", ARDisplayString(m, rr), rr->state);
        rr->state = regState_DeregPending;
    }
    else
    {
        LogInfo("ActivateUnicastRegistration: Resource record %s, current state %d, moving to Pending", ARDisplayString(m, rr), rr->state);
        rr->state = regState_Pending;
    }
    rr->ProbingConflictCount = 0;
    rr->LastConflictPktNum   = 0;
    rr->ProbeRestartCount    = 0;
    rr->ProbeCount           = 0;
    rr->AnnounceCount        = 0;
    rr->ThisAPInterval       = INIT_RECORD_REG_INTERVAL;
    rr->LastAPTime           = m->timenow - rr->ThisAPInterval;
    rr->expire               = 0; // Forget about all the leases, start fresh
    rr->uselease             = mDNStrue;
    rr->updateid             = zeroID;
    rr->SRVChanged           = mDNSfalse;
    rr->updateError          = mStatus_NoError;
    // RestartRecordGetZoneData calls this function whenever a new interface gets registered with core.
    // The records might already be registered with the server and hence could have NAT state.
    if (rr->NATinfo.clientContext)
    {
        mDNS_StopNATOperation_internal(m, &rr->NATinfo);
        rr->NATinfo.clientContext = mDNSNULL;
    }
    if (rr->nta) { CancelGetZoneData(m, rr->nta); rr->nta = mDNSNULL; }
    if (rr->tcp) { DisposeTCPConn(rr->tcp);       rr->tcp = mDNSNULL; }
    if (m->NextuDNSEvent - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
        m->NextuDNSEvent = (rr->LastAPTime + rr->ThisAPInterval);
}

// Two records qualify to be local duplicates if:
// (a) the RecordTypes are the same, or
// (b) one is Unique and the other Verified
// (c) either is in the process of deregistering
#define RecordLDT(A,B) ((A)->resrec.RecordType == (B)->resrec.RecordType || \
                        ((A)->resrec.RecordType | (B)->resrec.RecordType) == (kDNSRecordTypeUnique | kDNSRecordTypeVerified) || \
                        ((A)->resrec.RecordType == kDNSRecordTypeDeregistering || (B)->resrec.RecordType == kDNSRecordTypeDeregistering))

#define RecordIsLocalDuplicate(A,B) \
    ((A)->resrec.InterfaceID == (B)->resrec.InterfaceID && RecordLDT((A),(B)) && IdenticalResourceRecord(& (A)->resrec, & (B)->resrec))

mDNSlocal AuthRecord *CheckAuthIdenticalRecord(AuthHash *r, AuthRecord *rr)
{
    const AuthGroup *a;
    AuthRecord *rp;

    a = AuthGroupForRecord(r, &rr->resrec);
    if (!a) return mDNSNULL;
    rp = a->members;
    while (rp)
    {
        if (!RecordIsLocalDuplicate(rp, rr))
            rp = rp->next;
        else
        {
            if (rp->resrec.RecordType == kDNSRecordTypeDeregistering)
            {
                rp->AnnounceCount = 0;
                rp = rp->next;
            }
            else return rp;
        }
    }
    return (mDNSNULL);
}

mDNSlocal mDNSBool CheckAuthRecordConflict(AuthHash *r, AuthRecord *rr)
{
    const AuthGroup *a;
    const AuthRecord *rp;

    a = AuthGroupForRecord(r, &rr->resrec);
    if (!a) return mDNSfalse;
    rp = a->members;
    while (rp)
    {
        const uintptr_t s1 = rr->RRSet ? rr->RRSet : (uintptr_t)rr;
        const uintptr_t s2 = rp->RRSet ? rp->RRSet : (uintptr_t)rp;
        if (s1 != s2 && SameResourceRecordSignature(rp, rr) && !IdenticalSameNameRecord(&rp->resrec, &rr->resrec))
            return mDNStrue;
        else
            rp = rp->next;
    }
    return (mDNSfalse);
}

// checks to see if "rr" is already present
mDNSlocal AuthRecord *CheckAuthSameRecord(AuthHash *r, AuthRecord *rr)
{
    const AuthGroup *a;
    AuthRecord *rp;

    a = AuthGroupForRecord(r, &rr->resrec);
    if (!a) return mDNSNULL;
    rp = a->members;
    while (rp)
    {
        if (rp != rr)
            rp = rp->next;
        else
        {
            return rp;
        }
    }
    return (mDNSNULL);
}

mDNSlocal void DecrementAutoTargetServices(mDNS *const m, AuthRecord *const rr)
{
    if (RRLocalOnly(rr))
    {
        // A sanity check, this should be prevented in calling code.
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "DecrementAutoTargetServices: called for RRLocalOnly() record: " PUB_S, ARDisplayString(m, rr));
        return;
    }

    if (!AuthRecord_uDNS(rr) && (rr->resrec.rrtype == kDNSType_SRV) && (rr->AutoTarget == Target_AutoHost))
    {
        NetworkInterfaceInfo *intf;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
        DeadvertiseFlags flags     = 0; // DeadvertiseFlags for non-AWDL interfaces.
        DeadvertiseFlags flagsAWDL = 0; // DeadvertiseFlags for AWDL interfaces.
        if (AuthRecordIncludesOrIsAWDL(rr))
        {
            if (AuthRecordIncludesAWDL(rr))
            {
                m->AutoTargetAWDLIncludedCount--;
                LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "DecrementAutoTargetServices: AutoTargetAWDLIncludedCount %u Record " PRI_S,
                    m->AutoTargetAWDLIncludedCount, ARDisplayString(m, rr));
                if (m->AutoTargetAWDLIncludedCount == 0)
                {
                    flags |= kDeadvertiseFlag_RandHostname;
                    if (m->AutoTargetAWDLOnlyCount == 0) flagsAWDL |= kDeadvertiseFlag_RandHostname;
                }
            }
            else
            {
                m->AutoTargetAWDLOnlyCount--;
                LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "DecrementAutoTargetServices: AutoTargetAWDLOnlyCount %u Record " PRI_S,
                    m->AutoTargetAWDLOnlyCount, ARDisplayString(m, rr));
                if ((m->AutoTargetAWDLIncludedCount == 0) && (m->AutoTargetAWDLOnlyCount == 0))
                {
                    flagsAWDL |= kDeadvertiseFlag_RandHostname;
                }
            }
            if (flags || flagsAWDL)
            {
                for (intf = m->HostInterfaces; intf; intf = intf->next)
                {
                    if (!intf->Advertise) continue;
                    if (mDNSPlatformInterfaceIsAWDL(intf->InterfaceID))
                    {
                        if (flagsAWDL) DeadvertiseInterface(m, intf, flagsAWDL);
                    }
                    else
                    {
                        if (flags) DeadvertiseInterface(m, intf, flags);
                    }
                }
            }
            if ((m->AutoTargetAWDLIncludedCount == 0) && (m->AutoTargetAWDLOnlyCount == 0))
            {
                GetRandomUUIDLocalHostname(&m->RandomizedHostname);
            }
        }
        else
#endif
        {
            m->AutoTargetServices--;
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "DecrementAutoTargetServices: AutoTargetServices %u Record " PRI_S,
                m->AutoTargetServices, ARDisplayString(m, rr));
            if (m->AutoTargetServices == 0)
            {
                for (intf = m->HostInterfaces; intf; intf = intf->next)
                {
                    if (intf->Advertise) DeadvertiseInterface(m, intf, kDeadvertiseFlag_NormalHostname);
                }
            }
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    if (!AuthRecord_uDNS(rr))
    {
        if (m->NumAllInterfaceRecords + m->NumAllInterfaceQuestions == 1)
            m->NextBonjourDisableTime = NonZeroTime(m->timenow + (BONJOUR_DISABLE_DELAY * mDNSPlatformOneSecond));
        m->NumAllInterfaceRecords--;
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "DecrementAutoTargetServices: NumAllInterfaceRecords %u NumAllInterfaceQuestions %u " PRI_S,
            m->NumAllInterfaceRecords, m->NumAllInterfaceQuestions, ARDisplayString(m, rr));
    }
#endif
}

mDNSlocal void AdvertiseNecessaryInterfaceRecords(mDNS *const m)
{
    NetworkInterfaceInfo *intf;
    for (intf = m->HostInterfaces; intf; intf = intf->next)
    {
        if (intf->Advertise) AdvertiseInterfaceIfNeeded(m, intf);
    }
}

mDNSlocal void IncrementAutoTargetServices(mDNS *const m, AuthRecord *const rr)
{
    mDNSBool enablingBonjour = mDNSfalse;

    if (RRLocalOnly(rr))
    {
        // A sanity check, this should be prevented in calling code.
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "IncrementAutoTargetServices: called for RRLocalOnly() record: " PRI_S, ARDisplayString(m, rr));
        return;
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    if (!AuthRecord_uDNS(rr))
    {
        m->NumAllInterfaceRecords++;
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "IncrementAutoTargetServices: NumAllInterfaceRecords %u NumAllInterfaceQuestions %u " PRI_S,
            m->NumAllInterfaceRecords, m->NumAllInterfaceQuestions, ARDisplayString(m, rr));
        if (m->NumAllInterfaceRecords + m->NumAllInterfaceQuestions == 1)
        {
            m->NextBonjourDisableTime = 0;
            if (m->BonjourEnabled == 0)
            {
                // Enable Bonjour immediately by scheduling network changed processing where
                // we will join the multicast group on each active interface.
                m->BonjourEnabled = 1;
                enablingBonjour = mDNStrue;
                m->NetworkChanged = m->timenow;
            }
        }
    }
#endif

    if (!AuthRecord_uDNS(rr) && (rr->resrec.rrtype == kDNSType_SRV) && (rr->AutoTarget == Target_AutoHost))
    {
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
        if (AuthRecordIncludesAWDL(rr))
        {
            m->AutoTargetAWDLIncludedCount++;
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "IncrementAutoTargetServices: AutoTargetAWDLIncludedCount %u Record " PRI_S,
                m->AutoTargetAWDLIncludedCount, ARDisplayString(m, rr));
        }
        else if (mDNSPlatformInterfaceIsAWDL(rr->resrec.InterfaceID))
        {
            m->AutoTargetAWDLOnlyCount++;
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "IncrementAutoTargetServices: AutoTargetAWDLOnlyCount %u Record " PRI_S,
                m->AutoTargetAWDLOnlyCount, ARDisplayString(m, rr));
        }
        else
#endif
        {
            m->AutoTargetServices++;
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG, "IncrementAutoTargetServices: AutoTargetServices %u Record " PRI_S,
                m->AutoTargetServices, ARDisplayString(m, rr));
        }
        // If this is the first advertised service and we did not just enable Bonjour above, then
        // advertise all the interface records.  If we did enable Bonjour above, the interface records will
        // be advertised during the network changed processing scheduled above, so no need
        // to do it here.
        if (!enablingBonjour) AdvertiseNecessaryInterfaceRecords(m);
    }
}

mDNSlocal void getKeepaliveRaddr(mDNS *const m, AuthRecord *rr, mDNSAddr *raddr)
{
    mDNSAddr     laddr = zeroAddr;
    mDNSEthAddr  eth = zeroEthAddr;
    mDNSIPPort   lport = zeroIPPort;
    mDNSIPPort   rport = zeroIPPort;
    mDNSu32      timeout = 0;
    mDNSu32      seq = 0;
    mDNSu32      ack = 0;
    mDNSu16      win = 0;

    if (mDNS_KeepaliveRecord(&rr->resrec))
    {
        mDNS_ExtractKeepaliveInfo(rr, &timeout, &laddr, raddr, &eth, &seq, &ack, &lport, &rport, &win);
        if (!timeout || mDNSAddressIsZero(&laddr) || mDNSAddressIsZero(raddr) || mDNSIPPortIsZero(lport) || mDNSIPPortIsZero(rport))
        {
            LogMsg("getKeepaliveRaddr: not a valid record %s for keepalive %#a:%d %#a:%d", ARDisplayString(m, rr), &laddr, lport.NotAnInteger, raddr, rport.NotAnInteger);
            return;
        }
    }
}

// Exported so uDNS.c can call this
mDNSexport mStatus mDNS_Register_internal(mDNS *const m, AuthRecord *const rr)
{
    domainname *target = GetRRDomainNameTarget(&rr->resrec);
    AuthRecord *r;
    AuthRecord **p = &m->ResourceRecords;
    AuthRecord **d = &m->DuplicateRecords;

    if ((mDNSs32)rr->resrec.rroriginalttl <= 0)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: TTL %X should be 1 - 0x7FFFFFFF " PRI_S, rr->resrec.rroriginalttl,
            ARDisplayString(m, rr));
        return(mStatus_BadParamErr);
    }

    if (!rr->resrec.RecordType)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: RecordType must be non-zero " PRI_S, ARDisplayString(m, rr));
        return(mStatus_BadParamErr); }

    if (m->ShutdownTime)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: Shutting down, can't register " PRI_S, ARDisplayString(m, rr));
        return(mStatus_ServiceNotRunning);
    }

    if (m->DivertMulticastAdvertisements && !AuthRecord_uDNS(rr))
    {
        mDNSInterfaceID previousID = rr->resrec.InterfaceID;
        if (rr->resrec.InterfaceID == mDNSInterface_Any || rr->resrec.InterfaceID == mDNSInterface_P2P)
        {
            rr->resrec.InterfaceID = mDNSInterface_LocalOnly;
            rr->ARType = AuthRecordLocalOnly;
        }
        if (rr->resrec.InterfaceID != mDNSInterface_LocalOnly)
        {
            NetworkInterfaceInfo *intf = FirstInterfaceForID(m, rr->resrec.InterfaceID);
            if (intf && !intf->Advertise) { rr->resrec.InterfaceID = mDNSInterface_LocalOnly; rr->ARType = AuthRecordLocalOnly; }
        }
        if (rr->resrec.InterfaceID != previousID)
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: Diverting record to local-only " PRI_S, ARDisplayString(m, rr));
        }
    }

    if (RRLocalOnly(rr))
    {
        if (CheckAuthSameRecord(&m->rrauth, rr))
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: ERROR!! Tried to register LocalOnly AuthRecord %p "
                PRI_DM_NAME " (" PUB_S ") that's already in the list",
                rr, DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype));
            return(mStatus_AlreadyRegistered);
        }
    }
    else
    {
        while (*p && *p != rr) p=&(*p)->next;
        if (*p)
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: ERROR!! Tried to register AuthRecord %p "
                PRI_DM_NAME " (" PUB_S ") that's already in the list",
                rr, DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype));
            return(mStatus_AlreadyRegistered);
        }
    }

    while (*d && *d != rr) d=&(*d)->next;
    if (*d)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: ERROR!! Tried to register AuthRecord %p "
            PRI_DM_NAME " (" PUB_S ") that's already in the Duplicate list",
            rr, DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype));
        return(mStatus_AlreadyRegistered);
    }

    if (rr->DependentOn)
    {
        if (rr->resrec.RecordType == kDNSRecordTypeUnique)
            rr->resrec.RecordType =  kDNSRecordTypeVerified;
        else if (rr->resrec.RecordType != kDNSRecordTypeKnownUnique)
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: ERROR! " PRI_DM_NAME " (" PUB_S
                "): rr->DependentOn && RecordType != kDNSRecordTypeUnique or kDNSRecordTypeKnownUnique",
                DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype));
            return(mStatus_Invalid);
        }
        if (!(rr->DependentOn->resrec.RecordType & (kDNSRecordTypeUnique | kDNSRecordTypeVerified | kDNSRecordTypeKnownUnique)))
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: ERROR! " PRI_DM_NAME " (" PUB_S
                "): rr->DependentOn->RecordType bad type %X",
                DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype), rr->DependentOn->resrec.RecordType);
            return(mStatus_Invalid);
        }
    }

    rr->next = mDNSNULL;

    // Field Group 1: The actual information pertaining to this resource record
    // Set up by client prior to call

    // Field Group 2: Persistent metadata for Authoritative Records
//  rr->Additional1       = set to mDNSNULL  in mDNS_SetupResourceRecord; may be overridden by client
//  rr->Additional2       = set to mDNSNULL  in mDNS_SetupResourceRecord; may be overridden by client
//  rr->DependentOn       = set to mDNSNULL  in mDNS_SetupResourceRecord; may be overridden by client
//  rr->RRSet             = set to mDNSNULL  in mDNS_SetupResourceRecord; may be overridden by client
//  rr->Callback          = already set      in mDNS_SetupResourceRecord
//  rr->Context           = already set      in mDNS_SetupResourceRecord
//  rr->RecordType        = already set      in mDNS_SetupResourceRecord
//  rr->HostTarget        = set to mDNSfalse in mDNS_SetupResourceRecord; may be overridden by client
//  rr->AllowRemoteQuery  = set to mDNSfalse in mDNS_SetupResourceRecord; may be overridden by client
    // Make sure target is not uninitialized data, or we may crash writing debugging log messages
    if (rr->AutoTarget && target) target->c[0] = 0;

    // Field Group 3: Transient state for Authoritative Records
    rr->Acknowledged      = mDNSfalse;
    rr->ProbeCount        = DefaultProbeCountForRecordType(rr->resrec.RecordType);
    rr->ProbeRestartCount = 0;
    rr->AnnounceCount     = InitialAnnounceCount;
    rr->RequireGoodbye    = mDNSfalse;
    rr->AnsweredLocalQ    = mDNSfalse;
    rr->IncludeInProbe    = mDNSfalse;
    rr->ImmedUnicast      = mDNSfalse;
    rr->SendNSECNow       = mDNSNULL;
    rr->ImmedAnswer       = mDNSNULL;
    rr->ImmedAdditional   = mDNSNULL;
    rr->SendRNow          = mDNSNULL;
    rr->v4Requester       = zerov4Addr;
    rr->v6Requester       = zerov6Addr;
    rr->NextResponse      = mDNSNULL;
    rr->NR_AnswerTo       = mDNSNULL;
    rr->NR_AdditionalTo   = mDNSNULL;
    if (!rr->AutoTarget) InitializeLastAPTime(m, rr);
//  rr->LastAPTime        = Set for us in InitializeLastAPTime()
//  rr->LastMCTime        = Set for us in InitializeLastAPTime()
//  rr->LastMCInterface   = Set for us in InitializeLastAPTime()
    rr->NewRData          = mDNSNULL;
    rr->newrdlength       = 0;
    rr->UpdateCallback    = mDNSNULL;
    rr->UpdateCredits     = kMaxUpdateCredits;
    rr->NextUpdateCredit  = 0;
    rr->UpdateBlocked     = 0;

    // For records we're holding as proxy (except reverse-mapping PTR records) two announcements is sufficient
    if (rr->WakeUp.HMAC.l[0] && !rr->AddressProxy.type) rr->AnnounceCount = 2;

    // Field Group 4: Transient uDNS state for Authoritative Records
    rr->state             = regState_Zero;
    rr->uselease          = 0;
    rr->expire            = 0;
    rr->Private           = 0;
    rr->updateid          = zeroID;
    rr->updateIntID       = zeroOpaque64;
    rr->zone              = rr->resrec.name;
    rr->nta               = mDNSNULL;
    rr->tcp               = mDNSNULL;
    rr->OrigRData         = 0;
    rr->OrigRDLen         = 0;
    rr->InFlightRData     = 0;
    rr->InFlightRDLen     = 0;
    rr->QueuedRData       = 0;
    rr->QueuedRDLen       = 0;
    //mDNSPlatformMemZero(&rr->NATinfo, sizeof(rr->NATinfo));
    // We should be recording the actual internal port for this service record here. Once we initiate our NAT mapping
    // request we'll subsequently overwrite srv.port with the allocated external NAT port -- potentially multiple
    // times with different values if the external NAT port changes during the lifetime of the service registration.
    //if (rr->resrec.rrtype == kDNSType_SRV) rr->NATinfo.IntPort = rr->resrec.rdata->u.srv.port;

//  rr->resrec.interface         = already set in mDNS_SetupResourceRecord
//  rr->resrec.name->c           = MUST be set by client
//  rr->resrec.rrtype            = already set in mDNS_SetupResourceRecord
//  rr->resrec.rrclass           = already set in mDNS_SetupResourceRecord
//  rr->resrec.rroriginalttl     = already set in mDNS_SetupResourceRecord
//  rr->resrec.rdata             = MUST be set by client, unless record type is CNAME or PTR and rr->HostTarget is set

    // BIND named (name daemon) doesn't allow TXT records with zero-length rdata. This is strictly speaking correct,
    // since RFC 1035 specifies a TXT record as "One or more <character-string>s", not "Zero or more <character-string>s".
    // Since some legacy apps try to create zero-length TXT records, we'll silently correct it here.
    if (rr->resrec.rrtype == kDNSType_TXT && rr->resrec.rdlength == 0) { rr->resrec.rdlength = 1; rr->resrec.rdata->u.txt.c[0] = 0; }
    if (rr->AutoTarget)
    {
        SetTargetToHostName(m, rr); // Also sets rdlength and rdestimate for us, and calls InitializeLastAPTime();
#ifndef UNICAST_DISABLED
        // If we have no target record yet, SetTargetToHostName will set rr->state == regState_NoTarget
        // In this case we leave the record half-formed in the list, and later we'll remove it from the list and re-add it properly.
        if (rr->state == regState_NoTarget)
        {
            // Initialize the target so that we don't crash while logging etc.
            domainname *tar = GetRRDomainNameTarget(&rr->resrec);
            if (tar) tar->c[0] = 0;
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: record " PUB_S " in NoTarget state", ARDisplayString(m, rr));
        }
#endif
    }
    else
    {
        rr->resrec.rdlength   = GetRDLength(&rr->resrec, mDNSfalse);
        rr->resrec.rdestimate = GetRDLength(&rr->resrec, mDNStrue);
    }

    if (!ValidateDomainName(rr->resrec.name))
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "Attempt to register record with invalid name: " PRI_S, ARDisplayString(m, rr));
        return(mStatus_Invalid);
    }

    // Don't do this until *after* we've set rr->resrec.rdlength
    if (!ValidateRData(rr->resrec.rrtype, rr->resrec.rdlength, rr->resrec.rdata))
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "Attempt to register record with invalid rdata: " PRI_S, ARDisplayString(m, rr));
        return(mStatus_Invalid);
    }

    rr->resrec.namehash   = DomainNameHashValue(rr->resrec.name);
    rr->resrec.rdatahash  = target ? DomainNameHashValue(target) : RDataHashValue(&rr->resrec);

    if (RRLocalOnly(rr))
    {
        // If this is supposed to be unique, make sure we don't have any name conflicts.
        // If we found a conflict, we may still want to insert the record in the list but mark it appropriately
        // (kDNSRecordTypeDeregistering) so that we deliver RMV events to the application. But this causes more
        // complications and not clear whether there are any benefits. See rdar:9304275 for details.
        // Hence, just bail out.
        // This comment is doesn’t make any sense. -- SC
        if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
        {
            if (CheckAuthRecordConflict(&m->rrauth, rr))
            {
                LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Register_internal: Name conflict " PRI_S " (%p), InterfaceID %p",
                    ARDisplayString(m, rr), rr, rr->resrec.InterfaceID);
                return mStatus_NameConflict;
            }
        }
    }

    // For uDNS records, we don't support duplicate checks at this time.
#ifndef UNICAST_DISABLED
    if (AuthRecord_uDNS(rr))
    {
        if (!m->NewLocalRecords) m->NewLocalRecords = rr;
        // When we called SetTargetToHostName, it may have caused mDNS_Register_internal to be re-entered, appending new
        // records to the list, so we now need to update p to advance to the new end to the list before appending our new record.
        while (*p) p=&(*p)->next;
        *p = rr;
        if (rr->resrec.RecordType == kDNSRecordTypeUnique) rr->resrec.RecordType = kDNSRecordTypeVerified;
        rr->ProbeCount    = 0;
        rr->ProbeRestartCount = 0;
        rr->AnnounceCount = 0;
        if (rr->state != regState_NoTarget) ActivateUnicastRegistration(m, rr);
        return(mStatus_NoError);            // <--- Note: For unicast records, code currently bails out at this point
    }
#endif

    // Now that we've finished building our new record, make sure it's not identical to one we already have
    if (RRLocalOnly(rr))
    {
        rr->ProbeCount    = 0;
        rr->ProbeRestartCount = 0;
        rr->AnnounceCount = 0;
        r = CheckAuthIdenticalRecord(&m->rrauth, rr);
    }
    else
    {
        for (r = m->ResourceRecords; r; r=r->next)
            if (RecordIsLocalDuplicate(r, rr))
            {
                if (r->resrec.RecordType == kDNSRecordTypeDeregistering) r->AnnounceCount = 0;
                else break;
            }
    }

    const domainname *const rrName = rr->resrec.name;
    const mDNSu32 nameHash = mDNS_DomainNameFNV1aHash(rrName);
    if (r)
    {
        MDNS_CORE_LOG_RDATA(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, &((rr)->resrec),
            "mDNS_Register_internal: adding to duplicate list -- name: " PRI_DM_NAME " (%x), ",
            DM_NAME_PARAM(rrName), nameHash);
        *d = rr;
        // If the previous copy of this record is already verified unique,
        // then indicate that we should move this record promptly to kDNSRecordTypeUnique state.
        // Setting ProbeCount to zero will cause SendQueries() to advance this record to
        // kDNSRecordTypeVerified state and call the client callback at the next appropriate time.
        if (rr->resrec.RecordType == kDNSRecordTypeUnique && r->resrec.RecordType == kDNSRecordTypeVerified)
            rr->ProbeCount = 0;
    }
    else
    {
        MDNS_CORE_LOG_RDATA(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, &((rr)->resrec),
            "mDNS_Register_internal: adding to active record list -- name: " PRI_DM_NAME " (%x), ",
            DM_NAME_PARAM(rrName), nameHash);
        if (RRLocalOnly(rr))
        {
            AuthGroup *ag;
            ag = InsertAuthRecord(m, &m->rrauth, rr);
            if (ag && !ag->NewLocalOnlyRecords)
            {
                m->NewLocalOnlyRecords = mDNStrue;
                ag->NewLocalOnlyRecords = rr;
            }
            // No probing for LocalOnly records; acknowledge them right away
            if (rr->resrec.RecordType == kDNSRecordTypeUnique) rr->resrec.RecordType = kDNSRecordTypeVerified;
            AcknowledgeRecord(m, rr);
            return(mStatus_NoError);
        }
        else
        {
            if (!m->NewLocalRecords) m->NewLocalRecords = rr;
            *p = rr;
        }
    }

    if (!AuthRecord_uDNS(rr))   // This check is superfluous, given that for unicast records we (currently) bail out above
    {
        // We have inserted the record in the list. See if we have to advertise the A/AAAA, HINFO, PTR records.
        IncrementAutoTargetServices(m, rr);

        // For records that are not going to probe, acknowledge them right away
        if (rr->resrec.RecordType != kDNSRecordTypeUnique && rr->resrec.RecordType != kDNSRecordTypeDeregistering)
            AcknowledgeRecord(m, rr);

        // Adding a record may affect whether or not we should sleep
        mDNS_UpdateAllowSleep(m);
    }

    // If this is a non-sleep proxy keepalive record, fetch the MAC address of the remote host.
    // This is used by the in-NIC proxy to send the keepalive packets.
    if (!rr->WakeUp.HMAC.l[0] && mDNS_KeepaliveRecord(&rr->resrec))
    {
        mDNSAddr raddr;
        // Set the record type to known unique to prevent probing keep alive records.
        // Also make sure we do not announce the keepalive records.
       rr->resrec.RecordType = kDNSRecordTypeKnownUnique;
       rr->AnnounceCount     = 0;
       getKeepaliveRaddr(m, rr, &raddr);
       // This is an asynchronous call. Once the remote MAC address is available, helper will schedule an
       // asynchronous task to update the resource record
       mDNSPlatformGetRemoteMacAddr(&raddr);
    }

    rr->TimeRegistered = m->timenow;

    return(mStatus_NoError);
}

mDNSlocal void RecordProbeFailure(mDNS *const m, const AuthRecord *const rr)
{
    m->ProbeFailTime = m->timenow;
    m->NumFailedProbes++;
    // If we've had fifteen or more probe failures, rate-limit to one every five seconds.
    // If a bunch of hosts have all been configured with the same name, then they'll all
    // conflict and run through the same series of names: name-2, name-3, name-4, etc.,
    // up to name-10. After that they'll start adding random increments in the range 1-100,
    // so they're more likely to branch out in the available namespace and settle on a set of
    // unique names quickly. If after five more tries the host is still conflicting, then we
    // may have a serious problem, so we start rate-limiting so we don't melt down the network.
    if (m->NumFailedProbes >= 15)
    {
        m->SuppressProbes = NonZeroTime(m->timenow + mDNSPlatformOneSecond * 5);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "Excessive name conflicts (%u) for " PRI_DM_NAME " (" PUB_S "); rate limiting in effect",
            m->NumFailedProbes, DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype));
    }
}

mDNSlocal void CompleteRDataUpdate(mDNS *const m, AuthRecord *const rr)
{
    RData *OldRData = rr->resrec.rdata;
    mDNSu16 OldRDLen = rr->resrec.rdlength;
    SetNewRData(&rr->resrec, rr->NewRData, rr->newrdlength);    // Update our rdata
    rr->NewRData = mDNSNULL;                                    // Clear the NewRData pointer ...
    if (rr->UpdateCallback)
        rr->UpdateCallback(m, rr, OldRData, OldRDLen);          // ... and let the client know
}

mDNSexport mDNSBool getValidContinousTSRTime(mDNSs32 *timestampContinuous, mDNSu32 tsrTimestamp)
{
    if (tsrTimestamp <= MaxTimeSinceReceived)
    {
        *timestampContinuous = mDNSPlatformContinuousTimeSeconds() - (mDNSs32)tsrTimestamp;
        return mDNStrue;
    }
    return mDNSfalse;
}

mDNSlocal AuthRecord *mDNSGetTSRForAuthRecordNamed(mDNS *const m, const domainname * const name, const mDNSu32 namehash)
{
    AuthRecord *ar = mDNSNULL;
    for (ar = m->ResourceRecords; ar; ar = ar->next)
    {
        if (ar->resrec.rrtype == kDNSType_OPT &&
            ar->resrec.namehash == namehash &&
            SameDomainName(ar->resrec.name, name))
        {
            const rdataOPT *const opt = (const rdataOPT *)&ar->resrec.rdata->u.data[0];
            if (opt->opt != kDNSOpt_TSR)
            {
                ar = mDNSNULL;
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
                    "mDNSGetTSRForAuthRecordNamed: Found OPT that is not kDNSOpt_TSR (%d)", opt->opt);
            }
            break;
        }
    }
    return ar;
}

mDNSexport AuthRecord *mDNSGetTSRForAuthRecord(mDNS *const m, const AuthRecord *const rr)
{
    return mDNSGetTSRForAuthRecordNamed(m, rr->resrec.name, rr->resrec.namehash);
}

mDNSexport CacheRecord *mDNSGetTSRForCacheGroup(const CacheGroup *const cg)
{
    CacheRecord *rr;
    for (rr = cg ? cg->members : mDNSNULL; rr; rr=rr->next)
    {
        if (rr->resrec.rrtype == kDNSType_OPT)
        {
            const rdataOPT *const opt = (const rdataOPT *)&rr->resrec.rdata->u.data[0];
            if (opt->opt == kDNSOpt_TSR)
            {
                return rr;
            }
            else
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
                    "mDNSGetTSRForCacheGroup: Found OPT that is not kDNSOpt_TSR (%d)", opt->opt);
                break;
            }
        }
    }
    return mDNSNULL;
}

mDNSlocal AuthRecord *FindOrphanedTSR(mDNS *const m, const domainname *const name, const mDNSu32 namehash)
{
    AuthRecord *tsr = mDNSGetTSRForAuthRecordNamed(m, name, namehash);

    for (const AuthRecord *ar = m->ResourceRecords; ar && tsr ; ar = ar->next)
    {
        if (ar->resrec.rrtype != kDNSType_OPT && 
            ar->resrec.namehash == namehash &&
            SameDomainName(ar->resrec.name, name))
        {
            // There is at least one non-TSR record that has the same name.
            // So this TSR is not an orphan.
            tsr = mDNSNULL;
        }
    }

    return tsr;
}

mDNSlocal void SetupTSROpt(const TSROptData *tsrData, rdataOPT *const tsrOPT)
{
    tsrOPT->u.tsr.hostkeyHash   = tsrData->hostkeyHash;
    tsrOPT->u.tsr.recIndex      = tsrData->recIndex;
    tsrOPT->u.tsr.timeStamp     = tsrData->timeStamp;

    tsrOPT->opt              = kDNSOpt_TSR;
    tsrOPT->optlen           = DNSOpt_TSRData_Space - 4;
}

mDNSlocal mDNSu8 *AddTSRROptsToMessage(const TSRDataPtrRecHead * const tsrHead, const DNSMessage *const msg,
    mDNSu8 * const rdlengthptr, mDNSu8 *ptr, const mDNSu8 *end)
{
    RData rdatastorage = {sizeof(RDataBody), 0, {0}};
    ResourceRecord next_opt;
    mDNSu8 *startptr = ptr;
    mDNSu16 actualLength = (mDNSu16)(rdlengthptr[0] << 8) + (mDNSu16)rdlengthptr[1];
    next_opt.rrtype     = kDNSType_OPT;
    next_opt.rdata      = &rdatastorage;
    next_opt.rdlength   = sizeof(rdataOPT);
    struct TSRDataPtrRec *next;
    SLIST_FOREACH(next, tsrHead, entries)
    {
        SetupTSROpt(next->tsr, &next_opt.rdata->u.opt[0]);
        ptr = putRData(msg, ptr, end, &next_opt);
        if (!ptr)
        {
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                "AddTSRRDataToMessage: TSR can't be written -- name " PRI_DM_NAME " hashkey %x",
                DM_NAME_PARAM(next->name), next->tsr->hostkeyHash);
            break;
        }
    }
    if (ptr && startptr != ptr)
    {
        actualLength += ptr - startptr;
        rdlengthptr[0] = (mDNSu8)(actualLength  >> 8);
        rdlengthptr[1] = (mDNSu8)(actualLength  &  0xFF);
    }
    return ptr;
}


mDNSlocal const TSROptData *TSRForNameFromDataRec(TSRDataRecHead *const tsrHead, const domainname *const name)
{
    struct TSRDataRec *nextTSR;
    SLIST_FOREACH(nextTSR, tsrHead, entries)
    {
        if (SameDomainName(&nextTSR->name, name))
        {
            return &nextTSR->tsr;
        }
    }
    return mDNSNULL;
}

mDNSlocal const TSROptData *TSRPtrForNameFromDataPtrRec(TSRDataPtrRecHead *const tsrHead, const domainname *const name)
{
    struct TSRDataPtrRec *nextTSR;
    SLIST_FOREACH(nextTSR, tsrHead, entries)
    {
        if (SameDomainName(nextTSR->name, name))
        {
            return nextTSR->tsr;
        }
    }
    return mDNSNULL;
}

mDNSlocal TSROptData *TSROptGetIfNew(mDNS *const m, const AuthRecord *const rr, TSRDataPtrRecHead * const tsrHead)
{
    AuthRecord *tsrOptRecord = mDNSGetTSRForAuthRecord(m, rr);
    if (tsrOptRecord    &&
        !TSRPtrForNameFromDataPtrRec(tsrHead, rr->resrec.name))
    {
        return &tsrOptRecord->resrec.rdata->u.opt[0].u.tsr;
    }
    return mDNSNULL;
}

// Note: mDNS_Deregister_internal can call a user callback, which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
// Exported so uDNS.c can call this
mDNSexport mStatus mDNS_Deregister_internal(mDNS *const m, AuthRecord *const rr, mDNS_Dereg_type drt)
{
    AuthRecord *r2;
    mDNSu8 RecordType = rr->resrec.RecordType;
    AuthRecord **p = &m->ResourceRecords;   // Find this record in our list of active records
    mDNSBool dupList = mDNSfalse;

    const CacheGroup *cg = mDNSNULL;
    const mDNSu32 nameHashToMatchTSR = rr->resrec.namehash;
    domainname nameToMatchTSR;
    AssignDomainName(&nameToMatchTSR, rr->resrec.name);
    const mDNSBool isTSR = (rr->resrec.rrtype == kDNSType_OPT);

    if (RRLocalOnly(rr))
    {
        AuthGroup *a;
        AuthRecord **rp;

        a = AuthGroupForRecord(&m->rrauth, &rr->resrec);
        if (!a) return mDNSfalse;
        rp = &a->members;
        while (*rp && *rp != rr) rp=&(*rp)->next;
        p = rp;
    }
    else
    {
        while (*p && *p != rr) p=&(*p)->next;
    }

    if (*p)
    {
        // We found our record on the main list. See if there are any duplicates that need special handling.
        if (drt == mDNS_Dereg_conflict)     // If this was a conflict, see that all duplicates get the same treatment
        {
            // Scan for duplicates of rr, and mark them for deregistration at the end of this routine, after we've finished
            // deregistering rr. We need to do this scan *before* we give the client the chance to free and reuse the rr memory.
            for (r2 = m->DuplicateRecords; r2; r2=r2->next) if (RecordIsLocalDuplicate(r2, rr)) r2->ProbeCount = 0xFF;
        }
        else if (drt == mDNS_Dereg_stale)     // If this was stale data, see that all duplicates on all interfaces get the same treatment
        {
            mDNSInterfaceID storeID = rr->resrec.InterfaceID;
            rr->resrec.InterfaceID = kDNSServiceInterfaceIndexAny;
            // Scan for duplicates of rr, and mark them for deregistration at the end of this routine, after we've finished
            // deregistering rr. We need to do this scan *before* we give the client the chance to free and reuse the rr memory.
            for (r2 = m->DuplicateRecords; r2; r2=r2->next) if (RecordIsLocalDuplicate(r2, rr)) r2->ProbeCount = 0xFF;
            rr->resrec.InterfaceID = storeID;
        }
        else
        {
            // Before we delete the record (and potentially send a goodbye packet)
            // first see if we have a record on the duplicate list ready to take over from it.
            AuthRecord **d = &m->DuplicateRecords;
            while (*d && !RecordIsLocalDuplicate(*d, rr)) d=&(*d)->next;
            if (*d)
            {
                AuthRecord *dup = *d;
                debugf("mDNS_Register_internal: Duplicate record %p taking over from %p %##s (%s)",
                       dup, rr, rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));
                *d        = dup->next;      // Cut replacement record from DuplicateRecords list
                if (RRLocalOnly(rr))
                {
                    dup->next = mDNSNULL;
                    if (!InsertAuthRecord(m, &m->rrauth, dup))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Deregister_internal: ERROR!! cannot insert " PRI_S, ARDisplayString(m, dup));
                    }
                }
                else
                {
                    dup->next = rr->next;       // And then...
                    rr->next  = dup;            // ... splice it in right after the record we're about to delete
                }
                dup->resrec.RecordType        = rr->resrec.RecordType;
                dup->ProbeCount      = rr->ProbeCount;
                dup->ProbeRestartCount = rr->ProbeRestartCount;
                dup->AnnounceCount   = rr->AnnounceCount;
                dup->RequireGoodbye  = rr->RequireGoodbye;
                dup->AnsweredLocalQ  = rr->AnsweredLocalQ;
                dup->ImmedAnswer     = rr->ImmedAnswer;
                dup->ImmedUnicast    = rr->ImmedUnicast;
                dup->ImmedAdditional = rr->ImmedAdditional;
                dup->v4Requester     = rr->v4Requester;
                dup->v6Requester     = rr->v6Requester;
                dup->ThisAPInterval  = rr->ThisAPInterval;
                dup->LastAPTime      = rr->LastAPTime;
                dup->LastMCTime      = rr->LastMCTime;
                dup->LastMCInterface = rr->LastMCInterface;
                dup->Private         = rr->Private;
                dup->state           = rr->state;
                rr->RequireGoodbye = mDNSfalse;
                rr->AnsweredLocalQ = mDNSfalse;
            }
        }
    }
    else
    {
        // We didn't find our record on the main list; try the DuplicateRecords list instead.
        p = &m->DuplicateRecords;
        while (*p && *p != rr) p=&(*p)->next;
        // If we found our record on the duplicate list, then make sure we don't send a goodbye for it
        if (*p)
        {
            // Duplicate records are not used for sending wakeups or goodbyes. Hence, deregister them
            // immediately. When there is a conflict, we deregister all the conflicting duplicate records
            // also that have been marked above in this function. In that case, we come here and if we don't
            // deregister (unilink from the DuplicateRecords list), we will be recursing infinitely. Hence,
            // clear the HMAC which will cause it to deregister. See <rdar://problem/10380988> for
            // details.
            rr->WakeUp.HMAC    = zeroEthAddr;
            rr->RequireGoodbye = mDNSfalse;
            rr->resrec.RecordType = kDNSRecordTypeDeregistering;
            dupList = mDNStrue;
        }
        if (*p) debugf("mDNS_Deregister_internal: Deleting DuplicateRecord %p %##s (%s)",
                       rr, rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));
    }

    if (!*p)
    {
        // No need to log an error message if we already know this is a potentially repeated deregistration
        if (drt != mDNS_Dereg_repeat)
        {
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Deregister_internal: Record %p not found in list " PRI_S, rr, ARDisplayString(m,rr));
        }
        return(mStatus_BadReferenceErr);
    }

    // If this is a shared record and we've announced it at least once,
    // we need to retract that announcement before we delete the record

    // If this is a record (including mDNSInterface_LocalOnly records) for which we've given local-only answers then
    // it's tempting to just do "AnswerAllLocalQuestionsWithLocalAuthRecord(m, rr, QC_rmv)" here, but that would not not be safe.
    // The AnswerAllLocalQuestionsWithLocalAuthRecord routine walks the question list invoking client callbacks, using the "m->CurrentQuestion"
    // mechanism to cope with the client callback modifying the question list while that's happening.
    // However, mDNS_Deregister could have been called from a client callback (e.g. from the domain enumeration callback FoundDomain)
    // which means that the "m->CurrentQuestion" mechanism is already in use to protect that list, so we can't use it twice.
    // More generally, if we invoke callbacks from within a client callback, then those callbacks could deregister other
    // records, thereby invoking yet more callbacks, without limit.
    // The solution is to defer delivering the "Remove" events until mDNS_Execute time, just like we do for sending
    // actual goodbye packets.

#ifndef UNICAST_DISABLED
    if (AuthRecord_uDNS(rr))
    {
        if (rr->RequireGoodbye)
        {
            if (rr->tcp) { DisposeTCPConn(rr->tcp); rr->tcp = mDNSNULL; }
            rr->resrec.RecordType    = kDNSRecordTypeDeregistering;
            m->LocalRemoveEvents     = mDNStrue;
            uDNS_DeregisterRecord(m, rr);
            // At this point unconditionally we bail out
            // Either uDNS_DeregisterRecord will have completed synchronously, and called CompleteDeregistration,
            // which calls us back here with RequireGoodbye set to false, or it will have initiated the deregistration
            // process and will complete asynchronously. Either way we don't need to do anything more here.
            return(mStatus_NoError);
        }
        // Sometimes the records don't complete proper deregistration i.e., don't wait for a response
        // from the server. In that case, if the records have been part of a group update, clear the
        // state here.
        rr->updateid = zeroID;

        // We defer cleaning up NAT state only after sending goodbyes. This is important because
        // RecordRegistrationGotZoneData guards against creating NAT state if clientContext is non-NULL.
        // This happens today when we turn on/off interface where we get multiple network transitions
        // and RestartRecordGetZoneData triggers re-registration of the resource records even though
        // they may be in Registered state which causes NAT information to be setup multiple times. Defering
        // the cleanup here keeps clientContext non-NULL and hence prevents that. Note that cleaning up
        // NAT state here takes care of the case where we did not send goodbyes at all.
        if (rr->NATinfo.clientContext)
        {
            mDNS_StopNATOperation_internal(m, &rr->NATinfo);
            rr->NATinfo.clientContext = mDNSNULL;
        }
        if (rr->nta) { CancelGetZoneData(m, rr->nta); rr->nta = mDNSNULL; }
        if (rr->tcp) { DisposeTCPConn(rr->tcp);       rr->tcp = mDNSNULL; }
    }
#endif // UNICAST_DISABLED

    if      (RecordType == kDNSRecordTypeUnregistered)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Deregister_internal: " PRI_S " already marked kDNSRecordTypeUnregistered",
            ARDisplayString(m, rr));
    }
    else if (RecordType == kDNSRecordTypeDeregistering)
    {
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Deregister_internal: " PRI_S " already marked kDNSRecordTypeDeregistering",
            ARDisplayString(m, rr));
        return(mStatus_BadReferenceErr);
    }

    if (rr->WakeUp.HMAC.l[0] ||
        (((RecordType == kDNSRecordTypeShared) || (rr->ARType == AuthRecordLocalOnly)) &&
        (rr->RequireGoodbye || rr->AnsweredLocalQ)))
    {
        verbosedebugf("mDNS_Deregister_internal: Starting deregistration for %s", ARDisplayString(m, rr));
        rr->resrec.RecordType    = kDNSRecordTypeDeregistering;
        rr->resrec.rroriginalttl = 0;
        rr->AnnounceCount        = rr->WakeUp.HMAC.l[0] ? WakeupCount : (drt == mDNS_Dereg_rapid) ? 1 : GoodbyeCount;
        rr->ThisAPInterval       = mDNSPlatformOneSecond * 2;
        rr->LastAPTime           = m->timenow - rr->ThisAPInterval;
        m->LocalRemoveEvents     = mDNStrue;
        if (m->NextScheduledResponse - (m->timenow + mDNSPlatformOneSecond/10) >= 0)
            m->NextScheduledResponse = (m->timenow + mDNSPlatformOneSecond/10);
    }
    else
    {
        // If the AuthRecord isn't a duplicate, isn't LocalOnly, and is unique, then flush each cached record that was
        // received via an interface that applies to the AuthRecord and whose name, type, class, and record data matches
        // that of the AuthRecord.
        //
        // Some clients are counting on fully deregistered records to no longer be present in the record cache (see
        // rdar://121145674).
        //
        // Notes:
        // 1. LocalOnly AuthRecords don't cause any mDNS traffic, so their records never populate the record cache.
        // 2. Shared AuthRecords, unlike unique AuthRecords, send out goodbye packets when deregistered, which causes
        //    their cached copies to be purged from record caches including this mDNSResponder's record cache.
        if (!dupList && !RRLocalOnly(rr) && (rr->resrec.RecordType & kDNSRecordTypeUniqueMask))
        {
            cg = CacheGroupForRecord(m, &rr->resrec);
            for (CacheRecord *cr = cg ? cg->members : mDNSNULL; cr; cr = cr->next)
            {
                const mDNSInterfaceID interface = cr->resrec.InterfaceID;
                if (IsInterfaceValidForAuthRecord(rr, interface) && IdenticalSameNameRecord(&cr->resrec, &rr->resrec))
                {
                    LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEBUG,
                        "mDNS_Deregister_internal: Purging cached record that matches deregistered AuthRecord -- "
                        "interface: " PUB_S "/%u, record: " PRI_S,
                        InterfaceNameForIDOrEmptyString(interface), IIDPrintable(interface), CRDisplayString(m, cr));
                    mDNS_PurgeCacheResourceRecord(m, cr);
                }
            }
        }
        if (!dupList && RRLocalOnly(rr))
        {
            AuthGroup *ag = RemoveAuthRecord(m, &m->rrauth, rr);
            if (ag->NewLocalOnlyRecords == rr) ag->NewLocalOnlyRecords = rr->next;
        }
        else
        {
            *p = rr->next;                  // Cut this record from the list
            if (m->NewLocalRecords == rr) m->NewLocalRecords = rr->next;
            DecrementAutoTargetServices(m, rr);
        }
        // If someone is about to look at this, bump the pointer forward
        if (m->CurrentRecord   == rr) m->CurrentRecord   = rr->next;
        rr->next = mDNSNULL;

        verbosedebugf("mDNS_Deregister_internal: Deleting record for %s", ARDisplayString(m, rr));
        rr->resrec.RecordType = kDNSRecordTypeUnregistered;

        if ((drt == mDNS_Dereg_conflict || drt == mDNS_Dereg_stale || drt == mDNS_Dereg_repeat) && RecordType == kDNSRecordTypeShared)
            debugf("mDNS_Deregister_internal: Cannot have a conflict on a shared record! %##s (%s)",
                   rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));

        // If we have an update queued up which never executed, give the client a chance to free that memory
        if (rr->NewRData) CompleteRDataUpdate(m, rr);   // Update our rdata, clear the NewRData pointer, and return memory to the client


        // CAUTION: MUST NOT do anything more with rr after calling rr->Callback(), because the client's callback function
        // is allowed to do anything, including starting/stopping queries, registering/deregistering records, etc.
        // In this case the likely client action to the mStatus_MemFree message is to free the memory,
        // so any attempt to touch rr after this is likely to lead to a crash.
        if (drt != mDNS_Dereg_conflict && drt != mDNS_Dereg_stale)
        {
            mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
            LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "mDNS_Deregister_internal: callback with mStatus_MemFree for " PRI_S, ARDisplayString(m, rr));
            if (rr->RecordCallback)
                rr->RecordCallback(m, rr, mStatus_MemFree);         // MUST NOT touch rr after this
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
        }
        else
        {
            const mStatus status_result = (drt == mDNS_Dereg_conflict) ? mStatus_NameConflict : mStatus_StaleData;
            RecordProbeFailure(m, rr);
            mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
            if (rr->RecordCallback)
                rr->RecordCallback(m, rr, status_result);    // MUST NOT touch rr after this
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
            // Now that we've finished deregistering rr, check our DuplicateRecords list for any that we marked previously.
            // Note that with all the client callbacks going on, by the time we get here all the
            // records we marked may have been explicitly deregistered by the client anyway.
            r2 = m->DuplicateRecords;
            while (r2)
            {
                if (r2->ProbeCount != 0xFF)
                {
                    r2 = r2->next;
                }
                else
                {
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
                    // See if this record was also registered with any D2D plugins.
                    D2D_stop_advertising_record(r2);
#endif
                    mDNS_Deregister_internal(m, r2, status_result);
                    // As this is a duplicate record, it will be unlinked from the list
                    // immediately
                    r2 = m->DuplicateRecords;
                }
            }
        }
    }
    mDNS_UpdateAllowSleep(m);

    // Find the corresponding TSR after we have finished the clean process.
    AuthRecord *const tsr = isTSR ? NULL :
        FindOrphanedTSR(m, &nameToMatchTSR, nameHashToMatchTSR);

    // When the last record sharing the same with the TSR record was deregistered, we should deregister the TSR record.
    if (tsr)
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, 
            "Deregistering orphaned TSR - " PRI_S, ARDisplayString(m, tsr));
        mDNS_Deregister_internal(m, tsr, mDNS_Dereg_repeat);

        if (cg)
        {   // Also remove associated TSR cache record
            CacheRecord *cacheTSR = mDNSGetTSRForCacheGroup(cg);
            if (cacheTSR)
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG,
                    "Purging cached TSR record that matches orphaned TSR -- " PRI_S, CRDisplayString(m, cacheTSR));
                mDNS_PurgeCacheResourceRecord(m, cacheTSR);
            }
        }
    }

    return(mStatus_NoError);
}

// ***************************************************************************
// MARK: - Packet Sending Functions

mDNSlocal void AddRecordToResponseList(AuthRecord ***nrpp, AuthRecord *rr, AuthRecord *add)
{
    // Add the record if it hasn't already been added.
    if (rr->NextResponse == mDNSNULL && *nrpp != &rr->NextResponse)
    {
        **nrpp = rr;
        // NR_AdditionalTo must point to a record with NR_AnswerTo set (and not NR_AdditionalTo)
        // If 'add' does not meet this requirement, then follow its NR_AdditionalTo pointer to a record that does
        // The referenced record will definitely be acceptable (by recursive application of this rule)
        if (add && add->NR_AdditionalTo) add = add->NR_AdditionalTo;
        rr->NR_AdditionalTo = add;
        *nrpp = &rr->NextResponse;
        debugf("AddRecordToResponseList: %##s (%s) already in list", rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));
    }
}

mDNSlocal void AddRRSetAdditionalsToResponseList(mDNS *const m, AuthRecord ***nrpp, AuthRecord *rr, AuthRecord *additional, const mDNSInterfaceID InterfaceID)
{
    AuthRecord *rr2;
    if (additional->resrec.RecordType & kDNSRecordTypeUniqueMask)
    {
        for (rr2 = m->ResourceRecords; rr2; rr2 = rr2->next)
        {
            if ((rr2->resrec.namehash == additional->resrec.namehash) &&
                (rr2->resrec.rrtype   == additional->resrec.rrtype) &&
                (rr2 != additional) &&
                (rr2->resrec.RecordType & kDNSRecordTypeUniqueMask) &&
                (rr2->resrec.rrclass  == additional->resrec.rrclass) &&
                ResourceRecordIsValidInterfaceAnswer(rr2, InterfaceID) &&
                SameDomainName(rr2->resrec.name, additional->resrec.name))
            {
                AddRecordToResponseList(nrpp, rr2, rr);
            }
        }
    }
}

mDNSlocal void AddAdditionalsToResponseList(mDNS *const m, AuthRecord *ResponseRecords, AuthRecord ***nrpp, const mDNSInterfaceID InterfaceID)
{
    AuthRecord  *rr, *rr2;
    for (rr=ResponseRecords; rr; rr=rr->NextResponse)           // For each record we plan to put
    {
        // (Note: This is an "if", not a "while". If we add a record, we'll find it again
        // later in the "for" loop, and we will follow further "additional" links then.)
        if (rr->Additional1 && ResourceRecordIsValidInterfaceAnswer(rr->Additional1, InterfaceID))
        {
            AddRecordToResponseList(nrpp, rr->Additional1, rr);
            AddRRSetAdditionalsToResponseList(m, nrpp, rr, rr->Additional1, InterfaceID);
        }

        if (rr->Additional2 && ResourceRecordIsValidInterfaceAnswer(rr->Additional2, InterfaceID))
        {
            AddRecordToResponseList(nrpp, rr->Additional2, rr);
            AddRRSetAdditionalsToResponseList(m, nrpp, rr, rr->Additional2, InterfaceID);
        }

        // For SRV records, automatically add the Address record(s) for the target host
        if (rr->resrec.rrtype == kDNSType_SRV)
        {
            for (rr2=m->ResourceRecords; rr2; rr2=rr2->next)                    // Scan list of resource records
                if (RRTypeIsAddressType(rr2->resrec.rrtype) &&                  // For all address records (A/AAAA) ...
                    ResourceRecordIsValidInterfaceAnswer(rr2, InterfaceID) &&   // ... which are valid for answer ...
                    rr->resrec.rdatahash == rr2->resrec.namehash &&         // ... whose name is the name of the SRV target
                    SameDomainName(&rr->resrec.rdata->u.srv.target, rr2->resrec.name))
                    AddRecordToResponseList(nrpp, rr2, rr);
        }
        else if (RRTypeIsAddressType(rr->resrec.rrtype))    // For A or AAAA, put counterpart as additional
        {
            for (rr2=m->ResourceRecords; rr2; rr2=rr2->next)                    // Scan list of resource records
                if (RRTypeIsAddressType(rr2->resrec.rrtype) &&                  // For all address records (A/AAAA) ...
                    ResourceRecordIsValidInterfaceAnswer(rr2, InterfaceID) &&   // ... which are valid for answer ...
                    rr->resrec.namehash == rr2->resrec.namehash &&              // ... and have the same name
                    SameDomainName(rr->resrec.name, rr2->resrec.name))
                    AddRecordToResponseList(nrpp, rr2, rr);
        }
        else if (rr->resrec.rrtype == kDNSType_PTR)         // For service PTR, see if we want to add DeviceInfo record
        {
            if (ResourceRecordIsValidInterfaceAnswer(&m->DeviceInfo, InterfaceID) &&
                SameDomainLabel(rr->resrec.rdata->u.name.c, m->DeviceInfo.resrec.name->c))
                AddRecordToResponseList(nrpp, &m->DeviceInfo, rr);
        }
    }
}

mDNSlocal void SendDelayedUnicastResponse(mDNS *const m, const mDNSAddr *const dest, const mDNSInterfaceID InterfaceID)
{
    AuthRecord *rr;
    AuthRecord  *ResponseRecords = mDNSNULL;
    AuthRecord **nrp             = &ResponseRecords;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, InterfaceID);

    // Make a list of all our records that need to be unicast to this destination
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        // If we find we can no longer unicast this answer, clear ImmedUnicast
        if (rr->ImmedAnswer == mDNSInterfaceMark               ||
            mDNSSameIPv4Address(rr->v4Requester, onesIPv4Addr) ||
            mDNSSameIPv6Address(rr->v6Requester, onesIPv6Addr)  )
            rr->ImmedUnicast = mDNSfalse;

        if (rr->ImmedUnicast && rr->ImmedAnswer == InterfaceID)
        {
            if ((dest->type == mDNSAddrType_IPv4 && mDNSSameIPv4Address(rr->v4Requester, dest->ip.v4)) ||
                (dest->type == mDNSAddrType_IPv6 && mDNSSameIPv6Address(rr->v6Requester, dest->ip.v6)))
            {
                rr->ImmedAnswer  = mDNSNULL;                // Clear the state fields
                rr->ImmedUnicast = mDNSfalse;
                rr->v4Requester  = zerov4Addr;
                rr->v6Requester  = zerov6Addr;

                // Only sent records registered for P2P over P2P interfaces
                if (intf && !mDNSPlatformValidRecordForInterface(rr, intf->InterfaceID))
                {
                    continue;
                }

                if (rr->NextResponse == mDNSNULL && nrp != &rr->NextResponse)   // rr->NR_AnswerTo
                {
                    rr->NR_AnswerTo = NR_AnswerMulticast;
                    *nrp = rr;
                    nrp = &rr->NextResponse;
                }
            }
        }
    }

    AddAdditionalsToResponseList(m, ResponseRecords, &nrp, InterfaceID);

    while (ResponseRecords)
    {
        TSRDataPtrRecHead tsrOpts = SLIST_HEAD_INITIALIZER(tsrOpts);
        TSROptData *newTSROpt;
        mDNSu16 tsrOptsCount = 0;
        mDNSu8 *responseptr = m->omsg.data;
        mDNSu8 *newptr;
        InitializeDNSMessage(&m->omsg.h, zeroID, ResponseFlags);

        // Put answers in the packet
        while (ResponseRecords && ResponseRecords->NR_AnswerTo)
        {
            rr = ResponseRecords;
            if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
                rr->resrec.rrclass |= kDNSClass_UniqueRRSet;        // Temporarily set the cache flush bit so PutResourceRecord will set it

            if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;
            newptr = PutResourceRecordTSR(&m->omsg, responseptr, &m->omsg.h.numAnswers, &rr->resrec);
            if (newTSROpt)
            {
                if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers - 1);
                else        tsrOptsCount--;
            }
            rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;           // Make sure to clear cache flush bit back to normal state
            if (!newptr && m->omsg.h.numAnswers)
            {
                break; // If packet full, send it now
            }
            if (newptr) responseptr = newptr;
            ResponseRecords = rr->NextResponse;
            rr->NextResponse    = mDNSNULL;
            rr->NR_AnswerTo     = mDNSNULL;
            rr->NR_AdditionalTo = mDNSNULL;
            rr->RequireGoodbye  = mDNStrue;
        }

        // Add additionals, if there's space
        while (ResponseRecords && !ResponseRecords->NR_AnswerTo)
        {
            rr = ResponseRecords;
            if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
                rr->resrec.rrclass |= kDNSClass_UniqueRRSet;        // Temporarily set the cache flush bit so PutResourceRecord will set it
            if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;
            newptr = PutResourceRecordTSR(&m->omsg, responseptr, &m->omsg.h.numAdditionals, &rr->resrec);
            if (newTSROpt)
            {
                if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers + m->omsg.h.numAdditionals - 1);
                else        tsrOptsCount--;
            }
            rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;           // Make sure to clear cache flush bit back to normal state
            if (newptr) responseptr = newptr;
            if (newptr && m->omsg.h.numAnswers) rr->RequireGoodbye = mDNStrue;
            else if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask) rr->ImmedAnswer = mDNSInterfaceMark;
            ResponseRecords = rr->NextResponse;
            rr->NextResponse    = mDNSNULL;
            rr->NR_AnswerTo     = mDNSNULL;
            rr->NR_AdditionalTo = mDNSNULL;
        }

        if (m->omsg.h.numAnswers)
        {
            if (!SLIST_EMPTY(&tsrOpts))
            {
                mDNSu8 *saveptr;
                AuthRecord opt;
                mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
                opt.resrec.rrclass    = NormalMaxDNSMessageData;
                opt.resrec.rdlength   = 0;
                opt.resrec.rdestimate = 0;
                if (!SLIST_EMPTY(&tsrOpts))
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupTSROpt(SLIST_FIRST(&tsrOpts)->tsr, &opt.resrec.rdata->u.opt[0]);
                    TSRDataRecPtrHeadRemoveAndFreeFirst(&tsrOpts);
                }
                // Put record after first TSR
                saveptr = responseptr;
                newptr = PutResourceRecord(&m->omsg, responseptr, &m->omsg.h.numAdditionals, &opt.resrec);
                if (newptr && !SLIST_EMPTY(&tsrOpts))
                {
                    mDNSu8 *rdlengthptr = saveptr + 2 + 2 + 4 + 1; // rrtype, rrclass, ttl, 0-length name
                    newptr = AddTSRROptsToMessage(&tsrOpts, &m->omsg, rdlengthptr, newptr,
                        m->omsg.data + AllowedRRSpace(&m->omsg));
                }
                if (newptr)
                {
                    responseptr = newptr;
                }
                else
                {
                    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "SendDelayedUnicastResponse: How did we fail to have space for OPT record (%d/%d/%d/%d) %s",
                        m->omsg.h.numQuestions, m->omsg.h.numAnswers, m->omsg.h.numAuthorities, m->omsg.h.numAdditionals, ARDisplayString(m, &opt));
                }
            }
            mDNSSendDNSMessage(m, &m->omsg, responseptr, InterfaceID, mDNSNULL, mDNSNULL, dest, MulticastDNSPort, mDNSNULL, mDNSfalse);
        }
        TSRDataRecPtrHeadFreeList(&tsrOpts);
    }
}

// CompleteDeregistration guarantees that on exit the record will have been cut from the m->ResourceRecords list
// and the client's mStatus_MemFree callback will have been invoked
mDNSexport void CompleteDeregistration(mDNS *const m, AuthRecord *rr)
{
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "CompleteDeregistration: called for Resource record " PRI_S, ARDisplayString(m, rr));
    // Clearing rr->RequireGoodbye signals mDNS_Deregister_internal() that
    // it should go ahead and immediately dispose of this registration
    rr->resrec.RecordType = kDNSRecordTypeShared;
    rr->RequireGoodbye    = mDNSfalse;
    rr->WakeUp.HMAC       = zeroEthAddr;
    if (rr->AnsweredLocalQ) { AnswerAllLocalQuestionsWithLocalAuthRecord(m, rr, QC_rmv); rr->AnsweredLocalQ = mDNSfalse; }
    mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);     // Don't touch rr after this
}

// DiscardDeregistrations is used on shutdown and sleep to discard (forcibly and immediately)
// any deregistering records that remain in the m->ResourceRecords list.
// DiscardDeregistrations calls mDNS_Deregister_internal which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void DiscardDeregistrations(mDNS *const m)
{
    if (m->CurrentRecord)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "DiscardDeregistrations ERROR m->CurrentRecord already set " PRI_S,
            ARDisplayString(m, m->CurrentRecord));
    }
    m->CurrentRecord = m->ResourceRecords;

    while (m->CurrentRecord)
    {
        AuthRecord *rr = m->CurrentRecord;
        if (!AuthRecord_uDNS(rr) && rr->resrec.RecordType == kDNSRecordTypeDeregistering)
            CompleteDeregistration(m, rr);      // Don't touch rr after this
        else
            m->CurrentRecord = rr->next;
    }
}

mDNSlocal mStatus GetLabelDecimalValue(const mDNSu8 *const src, mDNSu8 *dst)
{
    int i, val = 0;
    if (src[0] < 1 || src[0] > 3) return(mStatus_Invalid);
    for (i=1; i<=src[0]; i++)
    {
        if (src[i] < '0' || src[i] > '9') return(mStatus_Invalid);
        val = val * 10 + src[i] - '0';
    }
    if (val > 255) return(mStatus_Invalid);
    *dst = (mDNSu8)val;
    return(mStatus_NoError);
}

mDNSlocal mStatus GetIPv4FromName(mDNSAddr *const a, const domainname *const name)
{
    int skip = CountLabels(name) - 6;
    if (skip < 0) { LogMsg("GetIPFromName: Need six labels in IPv4 reverse mapping name %##s", name); return mStatus_Invalid; }
    if (GetLabelDecimalValue(SkipLeadingLabels(name, skip+3)->c, &a->ip.v4.b[0]) ||
        GetLabelDecimalValue(SkipLeadingLabels(name, skip+2)->c, &a->ip.v4.b[1]) ||
        GetLabelDecimalValue(SkipLeadingLabels(name, skip+1)->c, &a->ip.v4.b[2]) ||
        GetLabelDecimalValue(SkipLeadingLabels(name, skip+0)->c, &a->ip.v4.b[3])) return mStatus_Invalid;
    a->type = mDNSAddrType_IPv4;
    return(mStatus_NoError);
}

#define HexVal(X) ( ((X) >= '0' && (X) <= '9') ? ((X) - '0'     ) :   \
                    ((X) >= 'A' && (X) <= 'F') ? ((X) - 'A' + 10) :   \
                    ((X) >= 'a' && (X) <= 'f') ? ((X) - 'a' + 10) : -1)

mDNSlocal mStatus GetIPv6FromName(mDNSAddr *const a, const domainname *const name)
{
    int i, h, l;
    const domainname *n;

    int skip = CountLabels(name) - 34;
    if (skip < 0) { LogMsg("GetIPFromName: Need 34 labels in IPv6 reverse mapping name %##s", name); return mStatus_Invalid; }

    n = SkipLeadingLabels(name, skip);
    for (i=0; i<16; i++)
    {
        if (n->c[0] != 1) return mStatus_Invalid;
        l = HexVal(n->c[1]);
        n = (const domainname *)(n->c + 2);

        if (n->c[0] != 1) return mStatus_Invalid;
        h = HexVal(n->c[1]);
        n = (const domainname *)(n->c + 2);

        if (l<0 || h<0) return mStatus_Invalid;
        a->ip.v6.b[15-i] = (mDNSu8)((h << 4) | l);
    }

    a->type = mDNSAddrType_IPv6;
    return(mStatus_NoError);
}

mDNSlocal mDNSs32 ReverseMapDomainType(const domainname *const name)
{
    int skip = CountLabels(name) - 2;
    if (skip >= 0)
    {
        const domainname *suffix = SkipLeadingLabels(name, skip);
        if (SameDomainName(suffix, (const domainname*)"\x7" "in-addr" "\x4" "arpa")) return mDNSAddrType_IPv4;
        if (SameDomainName(suffix, (const domainname*)"\x3" "ip6"     "\x4" "arpa")) return mDNSAddrType_IPv6;
    }
    return(mDNSAddrType_None);
}

mDNSlocal void SendARP(mDNS *const m, const mDNSu8 op, const AuthRecord *const rr,
                       const mDNSv4Addr *const spa, const mDNSEthAddr *const tha, const mDNSv4Addr *const tpa, const mDNSEthAddr *const dst)
{
    int i;
    mDNSu8 *ptr = m->omsg.data;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, rr->resrec.InterfaceID);
    if (!intf) { LogMsg("SendARP: No interface with InterfaceID %p found %s", rr->resrec.InterfaceID, ARDisplayString(m,rr)); return; }

    // 0x00 Destination address
    for (i=0; i<6; i++) *ptr++ = dst->b[i];

    // 0x06 Source address (Note: Since we don't currently set the BIOCSHDRCMPLT option, BPF will fill in the real interface address for us)
    for (i=0; i<6; i++) *ptr++ = intf->MAC.b[0];

    // 0x0C ARP Ethertype (0x0806)
    *ptr++ = 0x08; *ptr++ = 0x06;

    // 0x0E ARP header
    *ptr++ = 0x00; *ptr++ = 0x01;   // Hardware address space; Ethernet = 1
    *ptr++ = 0x08; *ptr++ = 0x00;   // Protocol address space; IP = 0x0800
    *ptr++ = 6;                     // Hardware address length
    *ptr++ = 4;                     // Protocol address length
    *ptr++ = 0x00; *ptr++ = op;     // opcode; Request = 1, Response = 2

    // 0x16 Sender hardware address (our MAC address)
    for (i=0; i<6; i++) *ptr++ = intf->MAC.b[i];

    // 0x1C Sender protocol address
    for (i=0; i<4; i++) *ptr++ = spa->b[i];

    // 0x20 Target hardware address
    for (i=0; i<6; i++) *ptr++ = tha->b[i];

    // 0x26 Target protocol address
    for (i=0; i<4; i++) *ptr++ = tpa->b[i];

    // 0x2A Total ARP Packet length 42 bytes
    mDNSPlatformSendRawPacket(m->omsg.data, ptr, rr->resrec.InterfaceID);
}

mDNSlocal mDNSu16 CheckSum(const void *const data, mDNSs32 length, mDNSu32 sum)
{
    const mDNSu16 *ptr = data;
    while (length > 0) { length -= 2; sum += *ptr++; }
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);
    return (mDNSu16)(sum != 0xFFFF ? sum : 0);
}

mDNSlocal mDNSu16 IPv6CheckSum(const mDNSv6Addr *const src, const mDNSv6Addr *const dst, const mDNSu8 protocol, const void *const data, const mDNSu32 length)
{
    IPv6PseudoHeader ph;
    ph.src = *src;
    ph.dst = *dst;
    ph.len.b[0] = (mDNSu8)(length >> 24);
    ph.len.b[1] = (mDNSu8)(length >> 16);
    ph.len.b[2] = (mDNSu8)(length >> 8);
    ph.len.b[3] = (mDNSu8) length;
    ph.pro.b[0] = 0;
    ph.pro.b[1] = 0;
    ph.pro.b[2] = 0;
    ph.pro.b[3] = protocol;
    return CheckSum(&ph, sizeof(ph), CheckSum(data, length, 0));
}

mDNSlocal void SendNDP(mDNS *const m, const mDNSu8 op, const mDNSu8 flags, const AuthRecord *const rr,
                       const mDNSv6Addr *const spa, const mDNSEthAddr *const tha, const mDNSv6Addr *const tpa, const mDNSEthAddr *const dst)
{
    int i;
    mDNSOpaque16 checksum;
    mDNSu8 *ptr = m->omsg.data;
    // Some recipient hosts seem to ignore Neighbor Solicitations if the IPv6-layer destination address is not the
    // appropriate IPv6 solicited node multicast address, so we use that IPv6-layer destination address, even though
    // at the Ethernet-layer we unicast the packet to the intended target, to avoid wasting network bandwidth.
    const mDNSv6Addr mc = { { 0xFF,0x02,0x00,0x00, 0,0,0,0, 0,0,0,1, 0xFF,tpa->b[0xD],tpa->b[0xE],tpa->b[0xF] } };
    const mDNSv6Addr *const v6dst = (op == NDP_Sol) ? &mc : tpa;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, rr->resrec.InterfaceID);
    if (!intf) { LogMsg("SendNDP: No interface with InterfaceID %p found %s", rr->resrec.InterfaceID, ARDisplayString(m,rr)); return; }

    // 0x00 Destination address
    for (i=0; i<6; i++) *ptr++ = dst->b[i];
    // Right now we only send Neighbor Solicitations to verify whether the host we're proxying for has gone to sleep yet.
    // Since we know who we're looking for, we send it via Ethernet-layer unicast, rather than bothering every host on the
    // link with a pointless link-layer multicast.
    // Should we want to send traditional Neighbor Solicitations in the future, where we really don't know in advance what
    // Ethernet-layer address we're looking for, we'll need to send to the appropriate Ethernet-layer multicast address:
    // *ptr++ = 0x33;
    // *ptr++ = 0x33;
    // *ptr++ = 0xFF;
    // *ptr++ = tpa->b[0xD];
    // *ptr++ = tpa->b[0xE];
    // *ptr++ = tpa->b[0xF];

    // 0x06 Source address (Note: Since we don't currently set the BIOCSHDRCMPLT option, BPF will fill in the real interface address for us)
    for (i=0; i<6; i++) *ptr++ = (tha ? *tha : intf->MAC).b[i];

    // 0x0C IPv6 Ethertype (0x86DD)
    *ptr++ = 0x86; *ptr++ = 0xDD;

    // 0x0E IPv6 header
    *ptr++ = 0x60; *ptr++ = 0x00; *ptr++ = 0x00; *ptr++ = 0x00;     // Version, Traffic Class, Flow Label
    *ptr++ = 0x00; *ptr++ = 0x20;                                   // Length
    *ptr++ = 0x3A;                                                  // Protocol == ICMPv6
    *ptr++ = 0xFF;                                                  // Hop Limit

    // 0x16 Sender IPv6 address
    for (i=0; i<16; i++) *ptr++ = spa->b[i];

    // 0x26 Destination IPv6 address
    for (i=0; i<16; i++) *ptr++ = v6dst->b[i];

    // 0x36 NDP header
    *ptr++ = op;                    // 0x87 == Neighbor Solicitation, 0x88 == Neighbor Advertisement
    *ptr++ = 0x00;                  // Code
    *ptr++ = 0x00; *ptr++ = 0x00;   // Checksum placeholder (0x38, 0x39)
    *ptr++ = flags;
    *ptr++ = 0x00; *ptr++ = 0x00; *ptr++ = 0x00;

    if (op == NDP_Sol)  // Neighbor Solicitation. The NDP "target" is the address we seek.
    {
        // 0x3E NDP target.
        for (i=0; i<16; i++) *ptr++ = tpa->b[i];
        // 0x4E Source Link-layer Address
        // <http://www.ietf.org/rfc/rfc2461.txt>
        // MUST NOT be included when the source IP address is the unspecified address.
        // Otherwise, on link layers that have addresses this option MUST be included
        // in multicast solicitations and SHOULD be included in unicast solicitations.
        if (!mDNSIPv6AddressIsZero(*spa))
        {
            *ptr++ = NDP_SrcLL; // Option Type 1 == Source Link-layer Address
            *ptr++ = 0x01;      // Option length 1 (in units of 8 octets)
            for (i=0; i<6; i++) *ptr++ = (tha ? *tha : intf->MAC).b[i];
        }
    }
    else            // Neighbor Advertisement. The NDP "target" is the address we're giving information about.
    {
        // 0x3E NDP target.
        for (i=0; i<16; i++) *ptr++ = spa->b[i];
        // 0x4E Target Link-layer Address
        *ptr++ = NDP_TgtLL; // Option Type 2 == Target Link-layer Address
        *ptr++ = 0x01;      // Option length 1 (in units of 8 octets)
        for (i=0; i<6; i++) *ptr++ = (tha ? *tha : intf->MAC).b[i];
    }

    // 0x4E or 0x56 Total NDP Packet length 78 or 86 bytes
    m->omsg.data[0x13] = (mDNSu8)(ptr - &m->omsg.data[0x36]);     // Compute actual length
    checksum.NotAnInteger = ~IPv6CheckSum(spa, v6dst, 0x3A, &m->omsg.data[0x36], m->omsg.data[0x13]);
    m->omsg.data[0x38] = checksum.b[0];
    m->omsg.data[0x39] = checksum.b[1];

    mDNSPlatformSendRawPacket(m->omsg.data, ptr, rr->resrec.InterfaceID);
}

mDNSlocal void SetupTracerOpt(const mDNS *const m, rdataOPT *const Trace)
{
    mDNSu32 DNS_VERS = _DNS_SD_H;
    Trace->u.tracer.platf    = m->mDNS_plat;
    Trace->u.tracer.mDNSv    = DNS_VERS;

    Trace->opt              = kDNSOpt_Trace;
    Trace->optlen           = DNSOpt_TraceData_Space - 4;
}

mDNSlocal void SetupOwnerOpt(const mDNS *const m, const NetworkInterfaceInfo *const intf, rdataOPT *const owner)
{
    owner->u.owner.vers     = 0;
    owner->u.owner.seq      = m->SleepSeqNum;
    owner->u.owner.HMAC     = m->PrimaryMAC;
    owner->u.owner.IMAC     = intf->MAC;
    owner->u.owner.password = zeroEthAddr;

    // Don't try to compute the optlen until *after* we've set up the data fields
    // Right now the DNSOpt_Owner_Space macro does not depend on the owner->u.owner being set up correctly, but in the future it might
    owner->opt              = kDNSOpt_Owner;
    owner->optlen           = DNSOpt_Owner_Space(&m->PrimaryMAC, &intf->MAC) - 4;
}

mDNSlocal void GrantUpdateCredit(AuthRecord *rr)
{
    if (++rr->UpdateCredits >= kMaxUpdateCredits) rr->NextUpdateCredit = 0;
    else rr->NextUpdateCredit = NonZeroTime(rr->NextUpdateCredit + kUpdateCreditRefreshInterval);
}

mDNSlocal mDNSBool ShouldSendGoodbyesBeforeSleep(mDNS *const m, const NetworkInterfaceInfo *intf, AuthRecord *rr)
{
    // If there are no sleep proxies, we set the state to SleepState_Sleeping explicitly
    // and hence there is no need to check for Transfering state. But if we have sleep
    // proxies and partially sending goodbyes for some records, we will be in Transfering
    // state and hence need to make sure that we send goodbyes in that case too. Checking whether
    // we are not awake handles both cases.
    if ((rr->AuthFlags & AuthFlagsWakeOnly) && (m->SleepState != SleepState_Awake))
    {
        debugf("ShouldSendGoodbyesBeforeSleep: marking for goodbye", ARDisplayString(m, rr));
        return mDNStrue;
    }

    if (m->SleepState != SleepState_Sleeping)
        return mDNSfalse;

    // If we are going to sleep and in SleepState_Sleeping, SendGoodbyes on the interface tell you
    // whether you can send goodbyes or not.
    if (!intf->SendGoodbyes)
    {
        debugf("ShouldSendGoodbyesBeforeSleep: not sending goodbye %s, int %p", ARDisplayString(m, rr), intf->InterfaceID);
        return mDNSfalse;
    }
    else
    {
        debugf("ShouldSendGoodbyesBeforeSleep: sending goodbye %s, int %p", ARDisplayString(m, rr), intf->InterfaceID);
        return mDNStrue;
    }
}

mDNSlocal mDNSu32 DetermineOwnerRecordSpace(const NetworkInterfaceInfo *const intf)
{
    mDNSu32 space = 0;
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    mDNS *const m = &mDNSStorage;
    if (m->AnnounceOwner && intf->MAC.l[0])
    {
        space = DNSOpt_Header_Space + DNSOpt_Owner_Space(&m->PrimaryMAC, &intf->MAC);
    }
#else
    (void)intf;
#endif
    return space;
}

// Note about acceleration of announcements to facilitate automatic coalescing of
// multiple independent threads of announcements into a single synchronized thread:
// The announcements in the packet may be at different stages of maturity;
// One-second interval, two-second interval, four-second interval, and so on.
// After we've put in all the announcements that are due, we then consider
// whether there are other nearly-due announcements that are worth accelerating.
// To be eligible for acceleration, a record MUST NOT be older (further along
// its timeline) than the most mature record we've already put in the packet.
// In other words, younger records can have their timelines accelerated to catch up
// with their elder bretheren; this narrows the age gap and helps them eventually get in sync.
// Older records cannot have their timelines accelerated; this would just widen
// the gap between them and their younger bretheren and get them even more out of sync.

// Note: SendResponses calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void SendResponses(mDNS *const m)
{
    int pktcount = 0;
    AuthRecord *rr, *r2;
    mDNSs32 maxExistingAnnounceInterval = 0;
    const NetworkInterfaceInfo *intf = GetFirstActiveInterface(m->HostInterfaces);

    m->NextScheduledResponse = m->timenow + FutureTime;

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    if (m->SleepState == SleepState_Transferring) RetrySPSRegistrations(m);
#endif

    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (rr->ImmedUnicast)
        {
            mDNSAddr v4 = { mDNSAddrType_IPv4, {{{0}}} };
            mDNSAddr v6 = { mDNSAddrType_IPv6, {{{0}}} };
            v4.ip.v4 = rr->v4Requester;
            v6.ip.v6 = rr->v6Requester;
            if (!mDNSIPv4AddressIsZero(rr->v4Requester)) SendDelayedUnicastResponse(m, &v4, rr->ImmedAnswer);
            if (!mDNSIPv6AddressIsZero(rr->v6Requester)) SendDelayedUnicastResponse(m, &v6, rr->ImmedAnswer);
            if (rr->ImmedUnicast)
            {
                LogMsg("SendResponses: ERROR: rr->ImmedUnicast still set: %s", ARDisplayString(m, rr));
                rr->ImmedUnicast = mDNSfalse;
            }
        }

    // ***
    // *** 1. Setup: Set the SendRNow and ImmedAnswer fields to indicate which interface(s) the records need to be sent on
    // ***

    // Run through our list of records, and decide which ones we're going to announce on all interfaces
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        while (rr->NextUpdateCredit && m->timenow - rr->NextUpdateCredit >= 0) GrantUpdateCredit(rr);
        if (TimeToAnnounceThisRecord(rr, m->timenow))
        {
            if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
            {
                if (!rr->WakeUp.HMAC.l[0])
                {
                    if (rr->AnnounceCount) rr->ImmedAnswer = mDNSInterfaceMark;     // Send goodbye packet on all interfaces
                }
                else
                {
                    mDNSBool unicastOnly;
                    LogSPS("SendResponses: Sending wakeup %2d for %.6a %s", rr->AnnounceCount-3, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                    unicastOnly = ((rr->AnnounceCount == WakeupCount) || (rr->AnnounceCount == WakeupCount - 1)) ? mDNStrue : mDNSfalse;
                    SendWakeup(m, rr->resrec.InterfaceID, &rr->WakeUp.IMAC, &rr->WakeUp.password, unicastOnly);
                    for (r2 = rr; r2; r2=r2->next)
                        if ((r2->resrec.RecordType == kDNSRecordTypeDeregistering) && r2->AnnounceCount && (r2->resrec.InterfaceID == rr->resrec.InterfaceID) &&
                            mDNSSameEthAddress(&r2->WakeUp.IMAC, &rr->WakeUp.IMAC) && !mDNSSameEthAddress(&zeroEthAddr, &r2->WakeUp.HMAC))
                        {
                            // For now we only want to send a single Unsolicited Neighbor Advertisement restoring the address to the original
                            // owner, because these packets can cause some IPv6 stacks to falsely conclude that there's an address conflict.
                            if (r2->AddressProxy.type == mDNSAddrType_IPv6 && r2->AnnounceCount == WakeupCount)
                            {
                                LogSPS("NDP Announcement %2d Releasing traffic for H-MAC %.6a I-MAC %.6a %s",
                                       r2->AnnounceCount-3, &r2->WakeUp.HMAC, &r2->WakeUp.IMAC, ARDisplayString(m,r2));
                                SendNDP(m, NDP_Adv, NDP_Override, r2, &r2->AddressProxy.ip.v6, &r2->WakeUp.IMAC, &AllHosts_v6, &AllHosts_v6_Eth);
                            }
                            r2->LastAPTime = m->timenow;
                            // After 15 wakeups without success (maybe host has left the network) send three goodbyes instead
                            if (--r2->AnnounceCount <= GoodbyeCount) r2->WakeUp.HMAC = zeroEthAddr;
                        }
                }
            }
            else if (ResourceRecordIsValidAnswer(rr))
            {
                if (rr->AddressProxy.type)
                {
                    if (!mDNSSameEthAddress(&zeroEthAddr, &rr->WakeUp.HMAC))
                    {
                        rr->AnnounceCount--;
                        rr->ThisAPInterval *= 2;
                        rr->LastAPTime = m->timenow;
                        if (rr->AddressProxy.type == mDNSAddrType_IPv4)
                        {
                            LogSPS("ARP Announcement %2d Capturing traffic for H-MAC %.6a I-MAC %.6a %s",
                                    rr->AnnounceCount, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m,rr));
                            SendARP(m, 1, rr, &rr->AddressProxy.ip.v4, &zeroEthAddr, &rr->AddressProxy.ip.v4, &onesEthAddr);
                        }
                        else if (rr->AddressProxy.type == mDNSAddrType_IPv6)
                        {
                            LogSPS("NDP Announcement %2d Capturing traffic for H-MAC %.6a I-MAC %.6a %s",
                                    rr->AnnounceCount, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m,rr));
                            SendNDP(m, NDP_Adv, NDP_Override, rr, &rr->AddressProxy.ip.v6, mDNSNULL, &AllHosts_v6, &AllHosts_v6_Eth);
                        }
                    }
                }
                else
                {
                    rr->ImmedAnswer = mDNSInterfaceMark;        // Send on all interfaces
                    if (maxExistingAnnounceInterval < rr->ThisAPInterval)
                        maxExistingAnnounceInterval = rr->ThisAPInterval;
                    if (rr->UpdateBlocked) rr->UpdateBlocked = 0;
                }
            }
        }
    }

    // Any interface-specific records we're going to send are marked as being sent on all appropriate interfaces (which is just one)
    // Eligible records that are more than half-way to their announcement time are accelerated
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if ((rr->resrec.InterfaceID && rr->ImmedAnswer) ||
            (rr->ThisAPInterval <= maxExistingAnnounceInterval &&
             TimeToAnnounceThisRecord(rr, m->timenow + rr->ThisAPInterval/2) &&
             !rr->AddressProxy.type &&                  // Don't include ARP Annoucements when considering which records to accelerate
             ResourceRecordIsValidAnswer(rr)))
            rr->ImmedAnswer = mDNSInterfaceMark;        // Send on all interfaces

    // When sending SRV records (particularly when announcing a new service) automatically add related Address record(s) as additionals
    // Note: Currently all address records are interface-specific, so it's safe to set ImmedAdditional to their InterfaceID,
    // which will be non-null. If by some chance there is an address record that's not interface-specific (should never happen)
    // then all that means is that it won't get sent -- which would not be the end of the world.
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        if (rr->ImmedAnswer && rr->resrec.rrtype == kDNSType_SRV)
            for (r2=m->ResourceRecords; r2; r2=r2->next)                // Scan list of resource records
                if (RRTypeIsAddressType(r2->resrec.rrtype) &&           // For all address records (A/AAAA) ...
                    ResourceRecordIsValidAnswer(r2) &&                  // ... which are valid for answer ...
                    rr->LastMCTime - r2->LastMCTime >= 0 &&             // ... which we have not sent recently ...
                    rr->resrec.rdatahash == r2->resrec.namehash &&      // ... whose name is the name of the SRV target
                    SameDomainName(&rr->resrec.rdata->u.srv.target, r2->resrec.name) &&
                    (rr->ImmedAnswer == mDNSInterfaceMark || rr->ImmedAnswer == r2->resrec.InterfaceID))
                    r2->ImmedAdditional = r2->resrec.InterfaceID;       // ... then mark this address record for sending too
        // We also make sure we send the DeviceInfo TXT record too, if necessary
        // We check for RecordType == kDNSRecordTypeShared because we don't want to tag the
        // DeviceInfo TXT record onto a goodbye packet (RecordType == kDNSRecordTypeDeregistering).
        if (rr->ImmedAnswer && rr->resrec.RecordType == kDNSRecordTypeShared && rr->resrec.rrtype == kDNSType_PTR)
            if (ResourceRecordIsValidAnswer(&m->DeviceInfo) && SameDomainLabel(rr->resrec.rdata->u.name.c, m->DeviceInfo.resrec.name->c))
            {
                if (!m->DeviceInfo.ImmedAnswer) m->DeviceInfo.ImmedAnswer = rr->ImmedAnswer;
                else m->DeviceInfo.ImmedAnswer = mDNSInterfaceMark;
            }
    }

    // If there's a record which is supposed to be unique that we're going to send, then make sure that we give
    // the whole RRSet as an atomic unit. That means that if we have any other records with the same name/type/class
    // then we need to mark them for sending too. Otherwise, if we set the kDNSClass_UniqueRRSet bit on a
    // record, then other RRSet members that have not been sent recently will get flushed out of client caches.
    // -- If a record is marked to be sent on a certain interface, make sure the whole set is marked to be sent on that interface
    // -- If any record is marked to be sent on all interfaces, make sure the whole set is marked to be sent on all interfaces
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
        {
            if (rr->ImmedAnswer)            // If we're sending this as answer, see that its whole RRSet is similarly marked
            {
                for (r2 = m->ResourceRecords; r2; r2=r2->next)
                {
                    if ((r2->resrec.RecordType & kDNSRecordTypeUniqueMask) && ResourceRecordIsValidAnswer(r2) &&
                        (r2->ImmedAnswer != mDNSInterfaceMark) && (r2->ImmedAnswer != rr->ImmedAnswer) &&
                        SameResourceRecordSignature(r2, rr) &&
                        ((rr->ImmedAnswer == mDNSInterfaceMark) || IsInterfaceValidForAuthRecord(r2, rr->ImmedAnswer)))
                    {
                        r2->ImmedAnswer = !r2->ImmedAnswer ? rr->ImmedAnswer : mDNSInterfaceMark;
                    }
                }
            }
            else if (rr->ImmedAdditional)   // If we're sending this as additional, see that its whole RRSet is similarly marked
            {
                for (r2 = m->ResourceRecords; r2; r2=r2->next)
                {
                    if ((r2->resrec.RecordType & kDNSRecordTypeUniqueMask) && ResourceRecordIsValidAnswer(r2) &&
                        (r2->ImmedAdditional != rr->ImmedAdditional) &&
                        SameResourceRecordSignature(r2, rr) &&
                        IsInterfaceValidForAuthRecord(r2, rr->ImmedAdditional))
                    {
                        r2->ImmedAdditional = rr->ImmedAdditional;
                    }
                }
            }
        }

    // Now set SendRNow state appropriately
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        if (rr->ImmedAnswer == mDNSInterfaceMark)       // Sending this record on all appropriate interfaces
        {
            rr->SendRNow = !intf ? mDNSNULL : (rr->resrec.InterfaceID) ? rr->resrec.InterfaceID : intf->InterfaceID;
            rr->ImmedAdditional = mDNSNULL;             // No need to send as additional if sending as answer
            rr->LastMCTime      = m->timenow;
            rr->LastMCInterface = rr->ImmedAnswer;
            rr->ProbeRestartCount = 0;                  // Reset the probe restart count
            // If we're announcing this record, and it's at least half-way to its ordained time, then consider this announcement done
            if (TimeToAnnounceThisRecord(rr, m->timenow + rr->ThisAPInterval/2))
            {
                rr->AnnounceCount--;
                if (rr->resrec.RecordType != kDNSRecordTypeDeregistering)
                    rr->ThisAPInterval *= 2;
                rr->LastAPTime = m->timenow;
                debugf("Announcing %##s (%s) %d", rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype), rr->AnnounceCount);
            }
        }
        else if (rr->ImmedAnswer)                       // Else, just respond to a single query on single interface:
        {
            rr->SendRNow        = rr->ImmedAnswer;      // Just respond on that interface
            rr->ImmedAdditional = mDNSNULL;             // No need to send as additional too
            rr->LastMCTime      = m->timenow;
            rr->LastMCInterface = rr->ImmedAnswer;
        }
        SetNextAnnounceProbeTime(m, rr);
        //if (rr->SendRNow) LogMsg("%-15.4a %s", &rr->v4Requester, ARDisplayString(m, rr));
    }

    // ***
    // *** 2. Loop through interface list, sending records as appropriate
    // ***

    while (intf)
    {
        const mDNSu32 OwnerRecordSpace = DetermineOwnerRecordSpace(intf);
        int TraceRecordSpace = (mDNS_McastTracingEnabled && MDNS_TRACER) ? DNSOpt_Header_Space + DNSOpt_TraceData_Space : 0;
        TSRDataPtrRecHead tsrOpts = SLIST_HEAD_INITIALIZER(tsrOpts);
        TSROptData *newTSROpt;
        mDNSu16 tsrOptsCount = 0;
        int numDereg    = 0;
        int numAnnounce = 0;
        int numAnswer   = 0;
        mDNSu8 *responseptr = m->omsg.data;
        mDNSu8 *newptr;
        InitializeDNSMessage(&m->omsg.h, zeroID, ResponseFlags);

        // First Pass. Look for:
        // 1. Deregistering records that need to send their goodbye packet
        // 2. Updated records that need to retract their old data
        // 3. Answers and announcements we need to send
        for (rr = m->ResourceRecords; rr; rr=rr->next)
        {

            // Skip this interface if the record InterfaceID is *Any and the record is not
            // appropriate for the interface type.
            if ((rr->SendRNow == intf->InterfaceID) &&
                ((rr->resrec.InterfaceID == mDNSInterface_Any) && !mDNSPlatformValidRecordForInterface(rr, intf->InterfaceID)))
            {
                rr->SendRNow = GetNextActiveInterfaceID(intf);
            }
            else if (rr->SendRNow == intf->InterfaceID)
            {
                RData  *OldRData    = rr->resrec.rdata;
                mDNSu16 oldrdlength = rr->resrec.rdlength;
                mDNSu8 active = (mDNSu8)
                                (rr->resrec.RecordType != kDNSRecordTypeDeregistering && !ShouldSendGoodbyesBeforeSleep(m, intf, rr));
                newptr = mDNSNULL;
                if (rr->NewRData && active)
                {
                    // See if we should send a courtesy "goodbye" for the old data before we replace it.
                    if (ResourceRecordIsValidAnswer(rr) && rr->resrec.RecordType == kDNSRecordTypeShared && rr->RequireGoodbye)
                    {
                        if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;
                        newptr = PutRR_OS_TTL(responseptr, &m->omsg.h.numAnswers, &rr->resrec, 0);
                        if (newTSROpt)
                        {
                            if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers - 1);
                            else        tsrOptsCount--;
                        }
                        if (newptr) { responseptr = newptr; numDereg++; rr->RequireGoodbye = mDNSfalse; }
                        else continue; // If this packet is already too full to hold the goodbye for this record, skip it for now and we'll retry later
                    }
                    SetNewRData(&rr->resrec, rr->NewRData, rr->newrdlength);
                }

                if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
                    rr->resrec.rrclass |= kDNSClass_UniqueRRSet;        // Temporarily set the cache flush bit so PutResourceRecord will set it
                if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;
                newptr = PutRR_OS_TTL(responseptr, &m->omsg.h.numAnswers, &rr->resrec, active ? rr->resrec.rroriginalttl : 0);
                if (newTSROpt)
                {
                    if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers - 1);
                    else        tsrOptsCount--;
                }
                rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;           // Make sure to clear cache flush bit back to normal state
                if (newptr)
                {
                    responseptr = newptr;
                    rr->RequireGoodbye = active;
                    if (rr->resrec.RecordType == kDNSRecordTypeDeregistering) numDereg++;
                    else if (rr->LastAPTime == m->timenow) numAnnounce++;else numAnswer++;
                }

                if (rr->NewRData && active)
                    SetNewRData(&rr->resrec, OldRData, oldrdlength);

                // The first time through (pktcount==0), if this record is verified unique
                // (i.e. typically A, AAAA, SRV, TXT and reverse-mapping PTR), set the flag to add an NSEC too.
                if (!pktcount && active && (rr->resrec.RecordType & kDNSRecordTypeActiveUniqueMask) && !rr->SendNSECNow)
                    rr->SendNSECNow = mDNSInterfaceMark;

                if (newptr)     // If succeeded in sending, advance to next interface
                {
                    // If sending on all interfaces, go to next interface; else we're finished now
                    if (rr->ImmedAnswer == mDNSInterfaceMark && rr->resrec.InterfaceID == mDNSInterface_Any)
                        rr->SendRNow = GetNextActiveInterfaceID(intf);
                    else
                        rr->SendRNow = mDNSNULL;
                }
            }
        }

        // Second Pass. Add additional records, if there's space.
        newptr = responseptr;
        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->ImmedAdditional == intf->InterfaceID)
                if (ResourceRecordIsValidAnswer(rr))
                {
                    // If we have at least one answer already in the packet, then plan to add additionals too
                    mDNSBool SendAdditional = (m->omsg.h.numAnswers > 0);

                    // If we're not planning to send any additionals, but this record is a unique one, then
                    // make sure we haven't already sent any other members of its RRSet -- if we have, then they
                    // will have had the cache flush bit set, so now we need to finish the job and send the rest.
                    if (!SendAdditional && (rr->resrec.RecordType & kDNSRecordTypeUniqueMask))
                    {
                        const AuthRecord *a;
                        for (a = m->ResourceRecords; a; a=a->next)
                            if (a->LastMCTime      == m->timenow &&
                                a->LastMCInterface == intf->InterfaceID &&
                                SameResourceRecordSignature(a, rr)) { SendAdditional = mDNStrue; break; }
                    }
                    if (!SendAdditional)                    // If we don't want to send this after all,
                        rr->ImmedAdditional = mDNSNULL;     // then cancel its ImmedAdditional field
                    else if (newptr)                        // Else, try to add it if we can
                    {
                        // The first time through (pktcount==0), if this record is verified unique
                        // (i.e. typically A, AAAA, SRV, TXT and reverse-mapping PTR), set the flag to add an NSEC too.
                        if (!pktcount && (rr->resrec.RecordType & kDNSRecordTypeActiveUniqueMask) && !rr->SendNSECNow)
                            rr->SendNSECNow = mDNSInterfaceMark;

                        if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
                            rr->resrec.rrclass |= kDNSClass_UniqueRRSet;    // Temporarily set the cache flush bit so PutResourceRecord will set it
                        if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;;
                        newptr = PutRR_OS(newptr, &m->omsg.h.numAdditionals, &rr->resrec);
                        if (newTSROpt)
                        {
                            if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers + m->omsg.h.numAdditionals - 1);
                            else        tsrOptsCount--;
                        }
                        rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;       // Make sure to clear cache flush bit back to normal state
                        if (newptr)
                        {
                            responseptr = newptr;
                            rr->ImmedAdditional = mDNSNULL;
                            rr->RequireGoodbye = mDNStrue;
                            // If we successfully put this additional record in the packet, we record LastMCTime & LastMCInterface.
                            // This matters particularly in the case where we have more than one IPv6 (or IPv4) address, because otherwise,
                            // when we see our own multicast with the cache flush bit set, if we haven't set LastMCTime, then we'll get
                            // all concerned and re-announce our record again to make sure it doesn't get flushed from peer caches.
                            rr->LastMCTime      = m->timenow;
                            rr->LastMCInterface = intf->InterfaceID;
                        }
                    }
                }

        // Third Pass. Add NSEC records, if there's space.
        // When we're generating an NSEC record in response to a specify query for that type
        // (recognized by rr->SendNSECNow == intf->InterfaceID) we should really put the NSEC in the Answer Section,
        // not Additional Section, but for now it's easier to handle both cases in this Additional Section loop here.
        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->SendNSECNow == mDNSInterfaceMark || rr->SendNSECNow == intf->InterfaceID)
            {
                AuthRecord nsec;
                int len;
                mDNS_SetupResourceRecord(&nsec, mDNSNULL, mDNSInterface_Any, kDNSType_NSEC, rr->resrec.rroriginalttl, kDNSRecordTypeUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
                nsec.resrec.rrclass |= kDNSClass_UniqueRRSet;
                AssignDomainName(&nsec.namestorage, rr->resrec.name);
                len = DomainNameLength(rr->resrec.name);
                // We have a nxt name followed by window number, window length and a window bitmap
                nsec.resrec.rdlength = (mDNSu16)(len + 2 + NSEC_MCAST_WINDOW_SIZE);
                if (nsec.resrec.rdlength <= StandardAuthRDSize)
                {
                    mDNSu8 *ptr = nsec.rdatastorage.u.data;
                    mDNSPlatformMemZero(ptr, nsec.resrec.rdlength);
                    AssignDomainName(&nsec.rdatastorage.u.name, rr->resrec.name);
                    ptr += len;
                    *ptr++ = 0; // window number
                    *ptr++ = NSEC_MCAST_WINDOW_SIZE; // window length
                    for (r2 = m->ResourceRecords; r2; r2=r2->next)
                        if (ResourceRecordIsValidAnswer(r2) && SameResourceRecordNameClassInterface(r2, rr))
                        {
                            if (r2->resrec.rrtype >= kDNSQType_ANY) { LogMsg("SendResponses: Can't create NSEC for record %s", ARDisplayString(m, r2)); break; }
                            else ptr[r2->resrec.rrtype >> 3] |= 128 >> (r2->resrec.rrtype & 7);
                        }
                    newptr = responseptr;
                    if (!r2)    // If we successfully built our NSEC record, add it to the packet now
                    {
                        if ((newTSROpt = TSROptGetIfNew(m, rr, &tsrOpts)) != mDNSNULL) tsrOptsCount++;
                        newptr = PutRR_OS(responseptr, &m->omsg.h.numAdditionals, &nsec.resrec);
                        if (newTSROpt)
                        {
                            if (newptr) TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, rr->resrec.name, m->omsg.h.numAnswers + m->omsg.h.numAdditionals - 1);
                            else        tsrOptsCount--;
                        }
                        if (newptr) responseptr = newptr;
                    }
                }
                else LogMsg("SendResponses: not enough space (%d)  in authrecord for nsec", nsec.resrec.rdlength);

                // If we successfully put the NSEC record, clear the SendNSECNow flag
                // If we consider this NSEC optional, then we unconditionally clear the SendNSECNow flag, even if we fail to put this additional record
                if (newptr || rr->SendNSECNow == mDNSInterfaceMark)
                {
                    rr->SendNSECNow = mDNSNULL;
                    // Run through remainder of list clearing SendNSECNow flag for all other records which would generate the same NSEC
                    for (r2 = rr->next; r2; r2=r2->next)
                        if (SameResourceRecordNameClassInterface(r2, rr))
                            if (r2->SendNSECNow == mDNSInterfaceMark || r2->SendNSECNow == intf->InterfaceID)
                                r2->SendNSECNow = mDNSNULL;
                }
            }

        if (m->omsg.h.numAnswers || m->omsg.h.numAdditionals)
        {
            // If we have data to send, add OWNER/TRACER/OWNER+TRACER option if necessary, then send packet
            if (OwnerRecordSpace || TraceRecordSpace || !SLIST_EMPTY(&tsrOpts))
            {
                mDNSu8 *saveptr;
                AuthRecord opt;
                mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
                opt.resrec.rrclass    = NormalMaxDNSMessageData;
                opt.resrec.rdlength   = 0;
                opt.resrec.rdestimate = 0;
                mDNSu16 optCount      = 0;
                if (OwnerRecordSpace)
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupOwnerOpt(m, intf, &opt.resrec.rdata->u.opt[optCount++]);
                }
                if (TraceRecordSpace)
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupTracerOpt(m, &opt.resrec.rdata->u.opt[optCount++]);
                }
                if (!SLIST_EMPTY(&tsrOpts))
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupTSROpt(SLIST_FIRST(&tsrOpts)->tsr, &opt.resrec.rdata->u.opt[optCount++]);
                    TSRDataRecPtrHeadRemoveAndFreeFirst(&tsrOpts);
                }
                // Put record after first TSR
                saveptr = responseptr;
                newptr = PutResourceRecord(&m->omsg, responseptr, &m->omsg.h.numAdditionals, &opt.resrec);
                if (newptr && !SLIST_EMPTY(&tsrOpts))
                {
                    mDNSu8 *rdlengthptr = saveptr + 2 + 2 + 4 + 1; // rrtype, rrclass, ttl, 0-length name
                    newptr = AddTSRROptsToMessage(&tsrOpts, &m->omsg, rdlengthptr, newptr,
                        m->omsg.data + AbsoluteMaxDNSMessageData);
                }
                if (newptr)
                {
                    responseptr = newptr;
                }
                else if (m->omsg.h.numAnswers + m->omsg.h.numAuthorities + m->omsg.h.numAdditionals == 1)
                {
                    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "SendResponses: No space in packet for %s %s TSR(%d) OPT record (%d/%d/%d/%d) %s",
                        OwnerRecordSpace ? "OWNER" : "", TraceRecordSpace ? "TRACER" : "", tsrOptsCount,
                        m->omsg.h.numQuestions, m->omsg.h.numAnswers, m->omsg.h.numAuthorities, m->omsg.h.numAdditionals, ARDisplayString(m, &opt));
                }
                else
                {
                    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "SendResponses: How did we fail to have space for %s %s TSR(%d) OPT record (%d/%d/%d/%d) %s",
                        OwnerRecordSpace ? "OWNER" : "", TraceRecordSpace ? "TRACER" : "", tsrOptsCount,
                        m->omsg.h.numQuestions, m->omsg.h.numAnswers, m->omsg.h.numAuthorities, m->omsg.h.numAdditionals, ARDisplayString(m, &opt));
                }
            }

            debugf("SendResponses: Sending %d Deregistration%s, %d Announcement%s, %d Answer%s, %d Additional%s on %p",
                   numDereg,                 numDereg                 == 1 ? "" : "s",
                   numAnnounce,              numAnnounce              == 1 ? "" : "s",
                   numAnswer,                numAnswer                == 1 ? "" : "s",
                   m->omsg.h.numAdditionals, m->omsg.h.numAdditionals == 1 ? "" : "s", intf->InterfaceID);

            if (intf->IPv4Available) mDNSSendDNSMessage(m, &m->omsg, responseptr, intf->InterfaceID, mDNSNULL, mDNSNULL, &AllDNSLinkGroup_v4, MulticastDNSPort, mDNSNULL, mDNSfalse);
            if (intf->IPv6Available) mDNSSendDNSMessage(m, &m->omsg, responseptr, intf->InterfaceID, mDNSNULL, mDNSNULL, &AllDNSLinkGroup_v6, MulticastDNSPort, mDNSNULL, mDNSfalse);
            // If shutting down, don't suppress responses so that goodbyes for auth records get sent without delay.
            if (!m->SuppressResponses && !m->ShutdownTime)
            {
                m->SuppressResponses = NonZeroTime(m->timenow + ((mDNSPlatformOneSecond + 9) / 10));
            }
            if (++pktcount >= 1000) { LogMsg("SendResponses exceeded loop limit %d: giving up", pktcount); break; }
            // There might be more things to send on this interface, so go around one more time and try again.
        }
        else    // Nothing more to send on this interface; go to next
        {
            const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
            intf = next;
            pktcount = 0;       // When we move to a new interface, reset packet count back to zero -- NSEC generation logic uses it
        }
        TSRDataRecPtrHeadFreeList(&tsrOpts);
    }

    // ***
    // *** 3. Cleanup: Now that everything is sent, call client callback functions, and reset state variables
    // ***

    if (m->CurrentRecord)
        LogMsg("SendResponses ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
    m->CurrentRecord = m->ResourceRecords;
    while (m->CurrentRecord)
    {
        rr = m->CurrentRecord;
        m->CurrentRecord = rr->next;

        if (rr->SendRNow)
        {
            if (rr->ARType != AuthRecordLocalOnly && rr->ARType != AuthRecordP2P)
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_INFO, "SendResponses: No active interface "
                    "%d to send: %d %02X " PRI_S, IIDPrintable(rr->SendRNow), IIDPrintable(rr->resrec.InterfaceID),
                    rr->resrec.RecordType, ARDisplayString(m, rr));
            }

            rr->SendRNow = mDNSNULL;
        }

        if (rr->ImmedAnswer || rr->resrec.RecordType == kDNSRecordTypeDeregistering)
        {
            if (rr->NewRData) CompleteRDataUpdate(m, rr);   // Update our rdata, clear the NewRData pointer, and return memory to the client

            if (rr->resrec.RecordType == kDNSRecordTypeDeregistering && rr->AnnounceCount == 0)
            {
                // For Unicast, when we get the response from the server, we will call CompleteDeregistration
                if (!AuthRecord_uDNS(rr)) CompleteDeregistration(m, rr);        // Don't touch rr after this
            }
            else
            {
                rr->ImmedAnswer  = mDNSNULL;
                rr->ImmedUnicast = mDNSfalse;
                rr->v4Requester  = zerov4Addr;
                rr->v6Requester  = zerov6Addr;
            }
        }
    }
    verbosedebugf("SendResponses: Next in %ld ticks", m->NextScheduledResponse - m->timenow);
}

// Calling CheckCacheExpiration() is an expensive operation because it has to look at the entire cache,
// so we want to be lazy about how frequently we do it.
// 1. If a cache record is currently referenced by *no* active questions,
//    then we don't mind expiring it up to a minute late (who will know?)
// 2. Else, if a cache record is due for some of its final expiration queries,
//    we'll allow them to be late by up to 2% of the TTL
// 3. Else, if a cache record has completed all its final expiration queries without success,
//    and is expiring, and had an original TTL more than ten seconds, we'll allow it to be one second late
// 4. Else, it is expiring and had an original TTL of ten seconds or less (includes explicit goodbye packets),
//    so allow at most 1/10 second lateness
// 5. For records with rroriginalttl set to zero, that means we really want to delete them immediately
//    (we have a new record with DelayDelivery set, waiting for the old record to go away before we can notify clients).
#define CacheCheckGracePeriod(CR) (                                                   \
        ((CR)->CRActiveQuestion == mDNSNULL            ) ? (60 * mDNSPlatformOneSecond) : \
        ((CR)->UnansweredQueries < MaxUnansweredQueries) ? (TicksTTL(CR)/50)            : \
        ((CR)->resrec.rroriginalttl > 10               ) ? (mDNSPlatformOneSecond)      : \
        ((CR)->resrec.rroriginalttl > 0                ) ? (mDNSPlatformOneSecond/10)   : 0)

#define NextCacheCheckEvent(CR) ((CR)->NextRequiredQuery + CacheCheckGracePeriod(CR))

mDNSexport void ScheduleNextCacheCheckTime(mDNS *const m, const mDNSu32 slot, const mDNSs32 event)
{
    if (m->rrcache_nextcheck[slot] - event > 0)
        m->rrcache_nextcheck[slot] = event;
    if (m->NextCacheCheck          - event > 0)
        m->NextCacheCheck          = event;
}

// Note: MUST call SetNextCacheCheckTimeForRecord any time we change:
// rr->TimeRcvd
// rr->resrec.rroriginalttl
// rr->UnansweredQueries
// rr->CRActiveQuestion
mDNSexport void SetNextCacheCheckTimeForRecord(mDNS *const m, CacheRecord *const rr)
{
    rr->NextRequiredQuery = RRExpireTime(rr);

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // Do not schedule the refresher query for DNSSEC-validated record, because its TTL is controlled by multiple
    // records that might be already in the progress of refreshing.
    // Set UnansweredQueries to MaxUnansweredQueries to avoid the expensive and unnecessary queries.
    if (resource_record_is_dnssec_validated(&rr->resrec) && rr->UnansweredQueries != MaxUnansweredQueries)
    {
        rr->UnansweredQueries = MaxUnansweredQueries;
    }
#endif

    // If we have an active question, then see if we want to schedule a refresher query for this record.
    // Usually we expect to do four queries, at 80-82%, 85-87%, 90-92% and then 95-97% of the TTL.
    // Unicast Assist Notes:
    //      A Unicast Assist query will be expected at 75-77%, then the rest will proceed via multicast as needed.
    //      The unicast assist query will not increment UnansweredQueries.
    if (rr->CRActiveQuestion && rr->UnansweredQueries < MaxUnansweredQueries)
    {
        mDNSu8 maxUnansweredQueryFactor = MaxUnansweredQueries;
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
        if (rr->UnansweredQueries == 0      &&
            !rr->unicastAssistSent          &&
            mDNSOpaque16IsZero(rr->CRActiveQuestion->TargetQID))
        {
            maxUnansweredQueryFactor++;
        }
#endif
        rr->NextRequiredQuery -= TicksTTL(rr)/20 * (maxUnansweredQueryFactor - rr->UnansweredQueries);
        rr->NextRequiredQuery += mDNSRandom((mDNSu32)TicksTTL(rr)/50);
        verbosedebugf("SetNextCacheCheckTimeForRecord: NextRequiredQuery in %ld sec CacheCheckGracePeriod %d ticks for %s",
                      (rr->NextRequiredQuery - m->timenow) / mDNSPlatformOneSecond, CacheCheckGracePeriod(rr), CRDisplayString(m,rr));
    }
    ScheduleNextCacheCheckTime(m, HashSlotFromNameHash(rr->resrec.namehash), NextCacheCheckEvent(rr));
}

#define kMinimumReconfirmTime                     ((mDNSu32)mDNSPlatformOneSecond *  5)
#define kDefaultReconfirmTimeForWake              ((mDNSu32)mDNSPlatformOneSecond *  5)
#define kDefaultReconfirmTimeForNoAnswer          ((mDNSu32)mDNSPlatformOneSecond *  5)

// Delay before restarting questions on a flapping interface.
#define kDefaultQueryDelayTimeForFlappingInterface ((mDNSu32)mDNSPlatformOneSecond *  3)
// After kDefaultQueryDelayTimeForFlappingInterface seconds, allow enough time for up to three queries (0, 1, and 4 seconds)
// plus three seconds for "response delay" before removing the reconfirmed records from the cache.
#define kDefaultReconfirmTimeForFlappingInterface (kDefaultQueryDelayTimeForFlappingInterface + ((mDNSu32)mDNSPlatformOneSecond *  7))

mDNSexport mStatus mDNS_Reconfirm_internal(mDNS *const m, CacheRecord *const rr, mDNSu32 interval)
{
    if (interval < kMinimumReconfirmTime)
        interval = kMinimumReconfirmTime;
    if (interval > 0x10000000)  // Make sure interval doesn't overflow when we multiply by four below
        interval = 0x10000000;

    // If the expected expiration time for this record is more than interval+33%, then accelerate its expiration
    if (RRExpireTime(rr) - m->timenow > (mDNSs32)((interval * 4) / 3))
    {
        // Add a 33% random amount to the interval, to avoid synchronization between multiple hosts
        // For all the reconfirmations in a given batch, we want to use the same random value
        // so that the reconfirmation questions can be grouped into a single query packet
        if (!m->RandomReconfirmDelay) m->RandomReconfirmDelay = 1 + mDNSRandom(FutureTime);
        interval += m->RandomReconfirmDelay % ((interval/3) + 1);
        rr->TimeRcvd          = m->timenow - (mDNSs32)interval * 3;
        rr->resrec.rroriginalttl     = (interval * 4 + mDNSPlatformOneSecond - 1) / mDNSPlatformOneSecond;
        SetNextCacheCheckTimeForRecord(m, rr);
    }
    debugf("mDNS_Reconfirm_internal:%6ld ticks to go for %s %p",
           RRExpireTime(rr) - m->timenow, CRDisplayString(m, rr), rr->CRActiveQuestion);
    return(mStatus_NoError);
}

// BuildQuestion puts a question into a DNS Query packet and if successful, updates the value of queryptr.
// It also appends to the list of known answer records that need to be included,
// and updates the forcast for the size of the known answer section.
mDNSlocal mDNSBool BuildQuestion(mDNS *const m, const NetworkInterfaceInfo *intf, DNSMessage *query, mDNSu8 **queryptr,
                                 DNSQuestion *q, CacheRecord ***kalistptrptr, mDNSu32 *answerforecast)
{
    mDNSBool ucast = (q->LargeAnswers || q->RequestUnicast) && m->CanReceiveUnicastOn5353 && intf->SupportsUnicastMDNSResponse;
    mDNSu16 ucbit = (mDNSu16)(ucast ? kDNSQClass_UnicastResponse : 0);
    const mDNSu8 *const limit = query->data + NormalMaxDNSMessageData;
    mDNSu8 *newptr = putQuestion(query, *queryptr, limit - *answerforecast, &q->qname, q->qtype, (mDNSu16)(q->qclass | ucbit));
    if (!newptr)
    {
        debugf("BuildQuestion: No more space in this packet for question %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
        return(mDNSfalse);
    }
    else
    {
        mDNSu32 forecast = *answerforecast;
        const CacheGroup *const cg = CacheGroupForName(m, q->qnamehash, &q->qname);
        CacheRecord *cr;
        CacheRecord **ka = *kalistptrptr;   // Make a working copy of the pointer we're going to update

        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)             // If we have a resource record in our cache,
            if (cr->resrec.InterfaceID == q->SendQNow &&                    // received on this interface
                !(cr->resrec.RecordType & kDNSRecordTypeUniqueMask) &&      // which is a shared (i.e. not unique) record type
                cr->NextInKAList == mDNSNULL && ka != &cr->NextInKAList &&  // which is not already in the known answer list
                cr->resrec.rdlength <= SmallRecordLimit &&                  // which is small enough to sensibly fit in the packet
                SameNameCacheRecordAnswersQuestion(cr, q) &&                // which answers our question
                cr->TimeRcvd + TicksTTL(cr)/2 - m->timenow >                // and its half-way-to-expiry time is at least 1 second away
                mDNSPlatformOneSecond)                                      // (also ensures we never include goodbye records with TTL=1)
            {
                // We don't want to include unique records in the Known Answer section. The Known Answer section
                // is intended to suppress floods of shared-record replies from many other devices on the network.
                // That concept really does not apply to unique records, and indeed if we do send a query for
                // which we have a unique record already in our cache, then including that unique record as a
                // Known Answer, so as to suppress the only answer we were expecting to get, makes little sense.

                *ka = cr;   // Link this record into our known answer chain
                ka = &cr->NextInKAList;
                // We forecast: compressed name (2) type (2) class (2) TTL (4) rdlength (2) rdata (n)
                forecast += 12 + cr->resrec.rdestimate;
                // If we're trying to put more than one question in this packet, and it doesn't fit
                // then undo that last question and try again next time
                if (query->h.numQuestions > 1 && newptr + forecast >= limit)
                {
                    query->h.numQuestions--;
                    debugf("BuildQuestion: Retracting question %##s (%s) new forecast total %d, total questions %d",
                           q->qname.c, DNSTypeName(q->qtype), newptr + forecast - query->data, query->h.numQuestions);
                    ka = *kalistptrptr;     // Go back to where we started and retract these answer records
                    while (*ka) { CacheRecord *c = *ka; *ka = mDNSNULL; ka = &c->NextInKAList; }
                    return(mDNSfalse);      // Return false, so we'll try again in the next packet
                }
            }

        // Success! Update our state pointers, increment UnansweredQueries as appropriate, and return
        *queryptr        = newptr;              // Update the packet pointer
        *answerforecast  = forecast;            // Update the forecast
        *kalistptrptr    = ka;                  // Update the known answer list pointer
        if (ucast) q->ExpectUnicastResp = NonZeroTime(m->timenow);

        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)             // For every resource record in our cache,
            if (cr->resrec.InterfaceID == q->SendQNow &&                    // received on this interface
                cr->NextInKAList == mDNSNULL && ka != &cr->NextInKAList &&  // which is not in the known answer list
                SameNameCacheRecordAnswersQuestion(cr, q))                  // which answers our question
            {
                cr->UnansweredQueries++;                                    // indicate that we're expecting a response
                cr->LastUnansweredTime = m->timenow;
                SetNextCacheCheckTimeForRecord(m, cr);
            }

        return(mDNStrue);
    }
}

// When we have a query looking for a specified name, but there appear to be no answers with
// that name, ReconfirmAntecedents() is called with depth=0 to start the reconfirmation process
// for any records in our cache that reference the given name (e.g. PTR and SRV records).
// For any such cache record we find, we also recursively call ReconfirmAntecedents() for *its* name.
// We increment depth each time we recurse, to guard against possible infinite loops, with a limit of 5.
// A typical reconfirmation scenario might go like this:
// Depth 0: Name "myhost.local" has no address records
// Depth 1: SRV "My Service._example._tcp.local." refers to "myhost.local"; may be stale
// Depth 2: PTR "_example._tcp.local." refers to "My Service"; may be stale
// Depth 3: PTR "_services._dns-sd._udp.local." refers to "_example._tcp.local."; may be stale
// Currently depths 4 and 5 are not expected to occur; if we did get to depth 5 we'd reconfim any records we
// found referring to the given name, but not recursively descend any further reconfirm *their* antecedents.
mDNSlocal void ReconfirmAntecedents(mDNS *const m, const domainname *const name, const mDNSu32 namehash, const mDNSInterfaceID InterfaceID, const int depth)
{
    mDNSu32 slot;
    const CacheGroup *cg;
    CacheRecord *cr;
    debugf("ReconfirmAntecedents (depth=%d) for %##s", depth, name->c);
    if (!InterfaceID) return; // mDNS records have a non-zero InterfaceID. If InterfaceID is 0, then there's nothing to do.
    FORALL_CACHERECORDS(slot, cg, cr)
    {
        const domainname *crtarget;
    #if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) || MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
        if (cr->DNSPushSubscribed)                 continue; // Skip records that are subscribed with a push server. [1]
    #endif
        if (cr->resrec.InterfaceID != InterfaceID) continue; // Skip non-mDNS records and mDNS records from other interfaces.
        if (cr->resrec.rdatahash != namehash)      continue; // Skip records whose rdata hash doesn't match the name hash.
        // Notes:
        // 1. If the records are resolved through DNS push subscription, only the push server can ask us to add or
        //    remove the record, so we do not need to reconfirm it here.
        crtarget = GetRRDomainNameTarget(&cr->resrec);
        if (crtarget && SameDomainName(crtarget, name))
        {
            const mDNSu32 nameHash = mDNS_NonCryptoHash(mDNSNonCryptoHash_FNV1a, cr->resrec.name->c,
                DomainNameLength(cr->resrec.name));
            const mDNSu32 targetNameHash = mDNS_NonCryptoHash(mDNSNonCryptoHash_FNV1a, crtarget->c,
                DomainNameLength(crtarget));
            // In the case of a PTR record, name_hash is the name of the service, target_name_hash is the hash
            // of the SRV record name, so target_name_hash is also useful information.
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "ReconfirmAntecedents: Reconfirming "
                "(depth=%d, InterfaceID=%p, name_hash=%x, target_name_hash=%x) " PRI_S, depth, InterfaceID, nameHash,
                targetNameHash, CRDisplayString(m, cr));
            mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
            if (depth < 5)
                ReconfirmAntecedents(m, cr->resrec.name, cr->resrec.namehash, InterfaceID, depth+1);
        }
    }
}

// If we get no answer for a AAAA query, then before doing an automatic implicit ReconfirmAntecedents
// we check if we have an address record for the same name. If we do have an IPv4 address for a given
// name but not an IPv6 address, that's okay (it just means the device doesn't do IPv6) so the failure
// to get a AAAA response is not grounds to doubt the PTR/SRV chain that lead us to that name.
mDNSlocal const CacheRecord *CacheHasAddressTypeForName(mDNS *const m, const domainname *const name, const mDNSu32 namehash)
{
    CacheGroup *const cg = CacheGroupForName(m, namehash, name);
    const CacheRecord *cr = cg ? cg->members : mDNSNULL;
    while (cr && !RRTypeIsAddressType(cr->resrec.rrtype)) cr=cr->next;
    return(cr);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
mDNSlocal mDNSBool CacheGroupHasAddressOnInterface(const CacheGroup *const cg, mDNSu16 rrtype, const mDNSAddr *const addr, const mDNSInterfaceID interfaceID)
{
    mDNS *const m = &mDNSStorage;
    mDNSBool result = mDNSfalse;
    const CacheRecord *cr;
    for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
    {
        if (cr->resrec.rrtype == rrtype                                     &&
            cr->resrec.InterfaceID == interfaceID                           &&
            RRExpireTime(cr) - m->timenow > UNICAST_ASSIST_MIN_REFRESH_TIME &&
            mDNSSameAddress(&cr->sourceAddress, addr))
        {
            result = mDNStrue;
            break;
        }
    }
    return(result);
}
#endif

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal const CacheRecord *FindSPSInCache1(mDNS *const m, const DNSQuestion *const q, const CacheRecord *const c0, const CacheRecord *const c1)
{
#ifndef SPC_DISABLED
    CacheGroup *const cg = CacheGroupForName(m, q->qnamehash, &q->qname);
    const CacheRecord *cr, *bestcr = mDNSNULL;
    mDNSu32 bestmetric = 1000000;
    for (cr = cg ? cg->members : mDNSNULL; cr; cr = cr->next)
        if (cr->resrec.rrtype == kDNSType_PTR && cr->resrec.rdlength >= 6)                      // If record is PTR type, with long enough name,
            if (cr != c0 && cr != c1)                                                           // that's not one we've seen before,
                if (SameNameCacheRecordAnswersQuestion(cr, q))                                  // and answers our browse query,
                    if (!IdenticalSameNameRecord(&cr->resrec, &m->SPSRecords.RR_PTR.resrec))    // and is not our own advertised service...
                    {
                        mDNSu32 metric = SPSMetric(cr->resrec.rdata->u.name.c);
                        if (bestmetric > metric) { bestmetric = metric; bestcr = cr; }
                    }
    return(bestcr);
#else // SPC_DISABLED
    (void) m;
    (void) q;
    (void) c0;
    (void) c1;
    (void) c1;
    return mDNSNULL;
#endif // SPC_DISABLED
}

mDNSlocal void CheckAndSwapSPS(const CacheRecord **sps1, const CacheRecord **sps2)
{
    const CacheRecord *swap_sps;
    mDNSu32 metric1, metric2;

    if (!(*sps1) || !(*sps2)) return;
    metric1 = SPSMetric((*sps1)->resrec.rdata->u.name.c);
    metric2 = SPSMetric((*sps2)->resrec.rdata->u.name.c);
    if (!SPSFeatures((*sps1)->resrec.rdata->u.name.c) && SPSFeatures((*sps2)->resrec.rdata->u.name.c) && (metric2 >= metric1))
    {
        swap_sps = *sps1;
        *sps1    = *sps2;
        *sps2    = swap_sps;
    }
}

mDNSlocal void ReorderSPSByFeature(const CacheRecord *sps[3])
{
    CheckAndSwapSPS(&sps[0], &sps[1]);
    CheckAndSwapSPS(&sps[0], &sps[2]);
    CheckAndSwapSPS(&sps[1], &sps[2]);
}


// Finds the three best Sleep Proxies we currently have in our cache
mDNSexport void FindSPSInCache(mDNS *const m, const DNSQuestion *const q, const CacheRecord *sps[3])
{
    sps[0] =                      FindSPSInCache1(m, q, mDNSNULL, mDNSNULL);
    sps[1] = !sps[0] ? mDNSNULL : FindSPSInCache1(m, q, sps[0],   mDNSNULL);
    sps[2] = !sps[1] ? mDNSNULL : FindSPSInCache1(m, q, sps[0],   sps[1]);

    // SPS is already sorted by metric. We want to move the entries to the beginning of the array
    // only if they have equally good metric and support features.
    ReorderSPSByFeature(sps);
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)

// Only DupSuppressInfos newer than the specified 'time' are allowed to remain active
mDNSlocal void ExpireDupSuppressInfo(DupSuppressState *const state, const mDNSs32 time)
{
    mdns_require_return(state);

    for (mDNSu32 i = 0; i < mdns_countof(state->slots); i++)
    {
        DupSuppressInfo *const slot = &state->slots[i];
        if ((slot->Time - time) < 0)
        {
            slot->InterfaceID = mDNSNULL;
        }
    }
}

mDNSlocal void ExpireDupSuppressInfoOnInterface(DupSuppressState *const state, const mDNSs32 time,
    const mDNSInterfaceID InterfaceID)
{
    mdns_require_return(state);

    for (mDNSu32 i = 0; i < mdns_countof(state->slots); i++)
    {
        DupSuppressInfo *const slot = &state->slots[i];
        if ((slot->InterfaceID == InterfaceID) && ((slot->Time - time) < 0))
        {
            slot->InterfaceID = mDNSNULL;
        }
    }
}

mDNSlocal mDNSBool SuppressOnThisInterface(const DupSuppressState *const state, const NetworkInterfaceInfo * const intf)
{
    mdns_require_quiet(state, exit);

    mDNSBool v4 = !intf->IPv4Available; // If interface doesn't support IPv4, we don't need to find an IPv4 duplicate.
    mDNSBool v6 = !intf->IPv6Available; // If interface doesn't support IPv6, we don't need to find an IPv6 duplicate.
    for (mDNSu32 i = 0; i < mdns_countof(state->slots); i++)
    {
        const DupSuppressInfo *const slot = &state->slots[i];
        if (slot->InterfaceID == intf->InterfaceID)
        {
            if (slot->Type == mDNSAddrType_IPv4)
            {
                v4 = mDNStrue;
            }
            else if (slot->Type == mDNSAddrType_IPv6)
            {
                v6 = mDNStrue;
            }
            if (v4 && v6)
            {
                return(mDNStrue);
            }
        }
    }

exit:
    return(mDNSfalse);
}

mDNSlocal void RecordDupSuppressInfo(DNSQuestion *const q, const mDNSs32 time, const mDNSInterfaceID InterfaceID,
    const mDNSs32 type)
{
    DupSuppressInfo *slot = mDNSNULL;
    if (!q->DupSuppress)
    {
        q->DupSuppress = (DupSuppressState *)mDNSPlatformMemAllocateClear(sizeof(*q->DupSuppress));
        mdns_require_quiet(q->DupSuppress, exit);
    }
    else
    {
        // See if we have this one in our list somewhere already
        DupSuppressState *const state = q->DupSuppress;
        for (mDNSu32 i = 0; i < mdns_countof(state->slots); i++)
        {
            DupSuppressInfo *const candidate = &state->slots[i];
            if ((candidate->InterfaceID == InterfaceID) && (candidate->Type == type))
            {
                slot = candidate;
                break;
            }
        }
    }

    // If not, find a slot we can re-use
    if (!slot)
    {
        DupSuppressState *const state = q->DupSuppress;
        slot = &state->slots[0];
        for (mDNSu32 i = 0; i < mdns_countof(state->slots); i++)
        {
            DupSuppressInfo *const candidate = &state->slots[i];
            const mDNSBool unused = !candidate->InterfaceID;
            if (unused || ((candidate->Time - slot->Time) < 0))
            {
                slot = candidate;
                if (unused)
                {
                    break;
                }
            }
        }
    }

    // Record the info about this query we saw
    slot->Time        = time;
    slot->InterfaceID = InterfaceID;
    slot->Type        = type;

exit:
    return;
}

mDNSlocal void mDNSSendWakeOnResolve(mDNS *const m, DNSQuestion *q)
{
    int len, i, cnt;
    mDNSInterfaceID InterfaceID = q->InterfaceID;
    domainname *d = &q->qname;

    // We can't send magic packets without knowing which interface to send it on.
    if (InterfaceID == mDNSInterface_Any || LocalOnlyOrP2PInterface(InterfaceID))
    {
        LogMsg("mDNSSendWakeOnResolve: ERROR!! Invalid InterfaceID %p for question %##s", InterfaceID, q->qname.c);
        return;
    }

    // Split MAC@IPAddress and pass them separately
    len = d->c[0];
    cnt = 0;
    for (i = 1; i < len; i++)
    {
        if (d->c[i] == '@')
        {
            char EthAddr[18];   // ethernet adddress : 12 bytes + 5 ":" + 1 NULL byte
            char IPAddr[47];    // Max IP address len: 46 bytes (IPv6) + 1 NULL byte
            if (cnt != 5)
            {
                LogMsg("mDNSSendWakeOnResolve: ERROR!! Malformed Ethernet address %##s, cnt %d", q->qname.c, cnt);
                return;
            }
            if ((i - 1) > (int) (sizeof(EthAddr) - 1))
            {
                LogMsg("mDNSSendWakeOnResolve: ERROR!! Malformed Ethernet address %##s, length %d", q->qname.c, i - 1);
                return;
            }
            if ((len - i) > (int)(sizeof(IPAddr) - 1))
            {
                LogMsg("mDNSSendWakeOnResolve: ERROR!! Malformed IP address %##s, length %d", q->qname.c, len - i);
                return;
            }
            mDNSPlatformMemCopy(EthAddr, &d->c[1], i - 1);
            EthAddr[i - 1] = 0;
            mDNSPlatformMemCopy(IPAddr, &d->c[i + 1], len - i);
            IPAddr[len - i] = 0;
            m->mDNSStats.WakeOnResolves++;
            mDNSPlatformSendWakeupPacket(InterfaceID, EthAddr, IPAddr, InitialWakeOnResolveCount - q->WakeOnResolveCount);
            return;
        }
        else if (d->c[i] == ':')
            cnt++;
    }
    LogMsg("mDNSSendWakeOnResolve: ERROR!! Malformed WakeOnResolve name %##s", q->qname.c);
}


mDNSlocal mDNSBool AccelerateThisQuery(mDNS *const m, DNSQuestion *q)
{
    // If more than 90% of the way to the query time, we should unconditionally accelerate it
    if (TimeToSendThisQuestion(q, m->timenow + q->ThisQInterval/10))
        return(mDNStrue);

    // If half-way to next scheduled query time, only accelerate if it will add less than 512 bytes to the packet
    if (TimeToSendThisQuestion(q, m->timenow + q->ThisQInterval/2))
    {
        // We forecast: qname (n) type (2) class (2)
        mDNSu32 forecast = (mDNSu32)DomainNameLength(&q->qname) + 4;
        const CacheGroup *const cg = CacheGroupForName(m, q->qnamehash, &q->qname);
        const CacheRecord *cr;
        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)              // If we have a resource record in our cache,
            if (cr->resrec.rdlength <= SmallRecordLimit &&                   // which is small enough to sensibly fit in the packet
                SameNameCacheRecordAnswersQuestion(cr, q) &&                 // which answers our question
                cr->TimeRcvd + TicksTTL(cr)/2 - m->timenow >= 0 &&           // and it is less than half-way to expiry
                cr->NextRequiredQuery - (m->timenow + q->ThisQInterval) > 0) // and we'll ask at least once again before NextRequiredQuery
            {
                // We forecast: compressed name (2) type (2) class (2) TTL (4) rdlength (2) rdata (n)
                forecast += 12 + cr->resrec.rdestimate;
                if (forecast >= 512) return(mDNSfalse); // If this would add 512 bytes or more to the packet, don't accelerate
            }
        return(mDNStrue);
    }

    return(mDNSfalse);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
mDNSlocal mDNSBool QuestionSendsMDNSQueriesViaUnicast(const DNSQuestion *const q)
{
    return (mDNSOpaque16IsZero(q->TargetQID) && mDNSAddressIsValidNonZero(&q->UnicastMDNSResolver));
}
#endif

// Return true if we should add record rr in the probe packet's authoritative section when probing for ar
// Otherwise return false.
mDNSlocal mDNSBool AddRecordInProbe(mDNS *const m, const AuthRecord *const ar, const AuthRecord *const rr, 
    const mDNSInterfaceID InterfaceID)
{
    // Already marked
    if (rr->IncludeInProbe)
    {
        return mDNSfalse;
    }
    mDNSBool hasTSR = (mDNSGetTSRForAuthRecord(m, ar) != mDNSNULL);

    // Only include TXT record in probe query's authority section if TSR record exist for the name
    if (rr->DependentOn && !hasTSR)
    {
        return mDNSfalse;
    }

    if (!IsInterfaceValidForAuthRecord(rr, InterfaceID))
    {
        return mDNSfalse;
    }

    // If a probe question is being sent for an AuthRecord and the AuthRecord is associated with a TSR record, then
    // all of the AuthRecords with the same name from the same client connection need to be present in the
    // authority section. So if one of them happens to have already gotten past the probing stage, it still needs
    // to be included. Currently, individually-registered AuthRecords from the same client connection will have the
    // same non-zero RRSet value.
    mDNSBool skipProbingStageCheck = mDNSfalse;
    if (hasTSR)
    {
        const uintptr_t s1 = ar->RRSet ? ar->RRSet : (uintptr_t)ar;
        const uintptr_t s2 = rr->RRSet ? rr->RRSet : (uintptr_t)rr;
        if (s1 == s2)
        {
            skipProbingStageCheck = mDNStrue;
        }
    }
    if (!skipProbingStageCheck)
    {
        // rr is not in probing stage and not dependent on other records
        // This is to exclude record that's already verified, but include TXT and TSR record if it's service registration
        // Refer to rdar://109086182 and rdar://109635078
        if (rr->resrec.RecordType != kDNSRecordTypeUnique && !rr->DependentOn)
        {
            return mDNSfalse;
        }
    }
    // Has the same name, class, interface with ar
    if (SameResourceRecordNameClassInterface(ar, rr))
    {
        return mDNStrue;
    }
    return mDNSfalse;
}

// How Standard Queries are generated:
// 1. The Question Section contains the question
// 2. The Additional Section contains answers we already know, to suppress duplicate responses

// How Probe Queries are generated:
// 1. The Question Section contains queries for the name we intend to use, with QType=ANY because
// if some other host is already using *any* records with this name, we want to know about it.
// 2. The Authority Section contains the proposed values we intend to use for one or more
// of our records with that name (analogous to the Update section of DNS Update packets)
// because if some other host is probing at the same time, we each want to know what the other is
// planning, in order to apply the tie-breaking rule to see who gets to use the name and who doesn't.

mDNSlocal void SendQueries(mDNS *const m)
{
    mDNSu32 slot;
    CacheGroup *cg;
    CacheRecord *cr;
    AuthRecord *ar;
    int pktcount = 0;
    DNSQuestion *q;
    // For explanation of maxExistingQuestionInterval logic, see comments for maxExistingAnnounceInterval
    mDNSs32 maxExistingQuestionInterval = 0;
    const NetworkInterfaceInfo *intf = GetFirstActiveInterface(m->HostInterfaces);
    CacheRecord *KnownAnswerList = mDNSNULL;

    // 1. If time for a query, work out what we need to do

    // We're expecting to send a query anyway, so see if any expiring cache records are close enough
    // to their NextRequiredQuery to be worth batching them together with this one
    FORALL_CACHERECORDS(slot, cg, cr)
    {
        if (cr->CRActiveQuestion && cr->UnansweredQueries < MaxUnansweredQueries)
        {
            if (m->timenow + TicksTTL(cr)/50 - cr->NextRequiredQuery >= 0)
            {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
                debugf("Sending %d%% cache expiration query for %s", (!cr->unicastAssistSent ? 75 : 80) + 5 * cr->UnansweredQueries, CRDisplayString(m, cr));
#else
                debugf("Sending %d%% cache expiration query for %s", 80 + 5 * cr->UnansweredQueries, CRDisplayString(m, cr));
#endif
                q = cr->CRActiveQuestion;
                ExpireDupSuppressInfoOnInterface(q->DupSuppress, m->timenow - TicksTTL(cr)/20, cr->resrec.InterfaceID);
                // For uDNS queries (TargetQID non-zero) we adjust LastQTime,
                // and bump UnansweredQueries so that we don't spin trying to send the same cache expiration query repeatedly
                if (!mDNSOpaque16IsZero(q->TargetQID))
                {
                    q->LastQTime = m->timenow - q->ThisQInterval;
                    cr->UnansweredQueries++;
                    m->mDNSStats.CacheRefreshQueries++;
                }
                else if (q->SendQNow == mDNSNULL)
                {
                    q->SendQNow = cr->resrec.InterfaceID;
                }
                else if (q->SendQNow != cr->resrec.InterfaceID)
                {
                    q->SendQNow = mDNSInterfaceMark;
                }

                // Indicate that this question was marked for sending
                // to update an existing cached answer record.
                // The browse throttling logic below uses this to determine
                // if the query should be sent.
                if (mDNSOpaque16IsZero(q->TargetQID))
                {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
                    if (!cr->unicastAssistSent                          &&
                        mDNSAddressIsValidNonZero(&cr->sourceAddress)   &&
                        !mDNSAddrIsDNSMulticast(&cr->sourceAddress))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_INFO,
                            "[Q%u] Sending unicast assist query (expiring) - " PRI_IP_ADDR " " PRI_DM_NAME " %s qhash %x" ,
                            mDNSVal16(q->TargetQID), &cr->sourceAddress, DM_NAME_PARAM(&q->qname),
                            DNSTypeName(q->qtype), q->qnamehash);

                        InitializeDNSMessage(&m->omsg.h, q->TargetQID, QueryFlags);
                        const mDNSu8 *const limit = m->omsg.data + sizeof(m->omsg.data);
                        const mDNSu16 qclass = q->qclass | kDNSQClass_UnicastResponse;
                        mDNSu8 *const end = putQuestion(&m->omsg, m->omsg.data, limit, &q->qname, q->qtype, qclass);
                        mDNSSendDNSMessage(m, &m->omsg, end, cr->resrec.InterfaceID, mDNSNULL, mDNSNULL, &cr->sourceAddress,
                            MulticastDNSPort, mDNSNULL, q->UseBackgroundTraffic);
                        q->LastQTime                = m->timenow;
                        q->LastQTxTime              = m->timenow;
                        q->RecentAnswerPkts         = 0;
                        q->ExpectUnicastResp        = NonZeroTime(m->timenow);
                        q->SendQNow                 = mDNSNULL;

                        cr->unicastAssistSent       = mDNStrue;
                        cr->LastUnansweredTime      = m->timenow;
                    }
                    else
#endif
                    {
                        q->CachedAnswerNeedsUpdate = mDNStrue;
                    }
                }
            }
        }
    }

    // Scan our list of questions to see which:
    //     *WideArea*  queries need to be sent
    //     *unicast*   queries need to be sent
    //     *multicast* queries we're definitely going to send
    if (m->CurrentQuestion)
        LogMsg("SendQueries ERROR m->CurrentQuestion already set: %##s (%s)", m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;
    while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
    {
        q = m->CurrentQuestion;
        if (mDNSOpaque16IsZero(q->TargetQID))
        {
            if (TimeToSendThisQuestion(q, m->timenow))
            {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
                if (QuestionSendsMDNSQueriesViaUnicast(q))
                {
                    InitializeDNSMessage(&m->omsg.h, q->TargetQID, QueryFlags);
                    const mDNSu8 *const limit = m->omsg.data + sizeof(m->omsg.data);
                    const mDNSu16 qclass = q->qclass | kDNSQClass_UnicastResponse;
                    mDNSu8 *const end = putQuestion(&m->omsg, m->omsg.data, limit, &q->qname, q->qtype, qclass);
                    mDNSSendDNSMessage(m, &m->omsg, end, q->InterfaceID, mDNSNULL, mDNSNULL, &q->UnicastMDNSResolver,
                        MulticastDNSPort, mDNSNULL, q->UseBackgroundTraffic);
                    q->ThisQInterval    *= QuestionIntervalStep;
                    if (q->ThisQInterval > MaxQuestionInterval)
                    {
                        q->ThisQInterval = MaxQuestionInterval;
                    }
                    q->LastQTime         = m->timenow;
                    q->LastQTxTime       = m->timenow;
                    q->RecentAnswerPkts  = 0;
                    q->SendQNow          = mDNSNULL;
                    q->ExpectUnicastResp = NonZeroTime(m->timenow);
                }
                else
#endif
                {
                    mDNSBool delayQuestion = mDNSfalse;
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
                    if (!q->initialAssistPerformed)
                    {
                        __block CacheGroup *const current_cg = CacheGroupForName(m, q->qnamehash, &q->qname);
                        __block bool assistSentForUnique = false;
                        q->initialAssistPerformed = mDNStrue;
                        unicast_assist_addr_enumerate(q->qnamehash, q->InterfaceID,
                            ^bool(const mDNSAddr * const addr, mDNSInterfaceID ifid, bool unique)
                        {
                            bool result = false;
                            if (!CacheGroupHasAddressOnInterface(current_cg, q->qtype, addr, ifid))
                            {
                                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_INFO,
                                    "[Q%u] Sending unicast assist query - " PRI_IP_ADDR " %d " PRI_DM_NAME " "
                                    PUB_DNS_TYPE " qhash %x", mDNSVal16(q->TargetQID), addr,
                                    IIDPrintable(ifid), DM_NAME_PARAM(&q->qname), DNS_TYPE_PARAM(q->qtype),
                                    q->qnamehash);

                                InitializeDNSMessage(&m->omsg.h, q->TargetQID, QueryFlags);
                                const mDNSu8 *const limit = m->omsg.data + sizeof(m->omsg.data);
                                const mDNSu16 qclass = q->qclass | kDNSQClass_UnicastResponse;
                                mDNSu8 *const end = putQuestion(&m->omsg, m->omsg.data, limit, &q->qname, q->qtype, qclass);
                                mDNSSendDNSMessage(m, &m->omsg, end, ifid, mDNSNULL, mDNSNULL, addr,
                                    MulticastDNSPort, mDNSNULL, q->UseBackgroundTraffic);
                                q->LastQTime         = m->timenow;
                                q->LastQTxTime       = m->timenow;
                                q->RecentAnswerPkts  = 0;
                                q->SendQNow          = mDNSNULL;
                                q->ExpectUnicastResp = NonZeroTime(m->timenow);
                                assistSentForUnique = unique;
                                result = true;
                            }
                            else
                            {
                                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_INFO, "SKIPPED unicast assist query - "
                                    PRI_IP_ADDR " %d " PRI_DM_NAME " " PUB_S " qhash %x",
                                    addr, IIDPrintable(ifid), DM_NAME_PARAM(&q->qname),
                                    DNSTypeName(q->qtype), q->qnamehash);
                            }
                            return result;
                        });
                        if (assistSentForUnique)
                        {
                            delayQuestion = mDNStrue;
                        }
                    }
#endif
                    if (!delayQuestion)
                    {
                        //LogInfo("Time to send %##s (%s) %d", q->qname.c, DNSTypeName(q->qtype), m->timenow - NextQSendTime(q));
                        q->SendQNow = mDNSInterfaceMark;        // Mark this question for sending on all interfaces
                        if (maxExistingQuestionInterval < q->ThisQInterval)
                            maxExistingQuestionInterval = q->ThisQInterval;
                    }
                }
            }
        }
        // If m->CurrentQuestion wasn't modified out from under us, advance it now
        // We can't do this at the start of the loop because uDNS_CheckCurrentQuestion() depends on having
        // m->CurrentQuestion point to the right question
        if (q == m->CurrentQuestion) m->CurrentQuestion = m->CurrentQuestion->next;
    }
    while (m->CurrentQuestion)
    {
        LogInfo("SendQueries question loop 1: Skipping NewQuestion %##s (%s)", m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
        m->CurrentQuestion = m->CurrentQuestion->next;
    }
    m->CurrentQuestion = mDNSNULL;

    // Scan our list of questions
    // (a) to see if there are any more that are worth accelerating, and
    // (b) to update the state variables for *all* the questions we're going to send
    // Note: Don't set NextScheduledQuery until here, because uDNS_CheckCurrentQuestion in the loop above can add new questions to the list,
    // which causes NextScheduledQuery to get (incorrectly) set to m->timenow. Setting it here is the right place, because the very
    // next thing we do is scan the list and call SetNextQueryTime() for every question we find, so we know we end up with the right value.
    m->NextScheduledQuery = m->timenow + FutureTime;
    for (q = m->Questions; q && q != m->NewQuestions; q=q->next)
    {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
        const mDNSBool qIsNormalMDNS = mDNSOpaque16IsZero(q->TargetQID) && !QuestionSendsMDNSQueriesViaUnicast(q);
#else
        const mDNSBool qIsNormalMDNS = mDNSOpaque16IsZero(q->TargetQID);
#endif
        if (qIsNormalMDNS
            && (q->SendQNow || (ActiveQuestion(q) && q->ThisQInterval <= maxExistingQuestionInterval && AccelerateThisQuery(m,q)
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
            && q->LastQTime != m->timenow   // Was not just sent above
#endif
            )))
        {
            // If at least halfway to next query time, advance to next interval
            // If less than halfway to next query time, then
            // treat this as logically a repeat of the last transmission, without advancing the interval
            if (m->timenow - (q->LastQTime + (q->ThisQInterval/2)) >= 0)
            {
                // If we have reached the answer threshold for this question,
                // don't send it again until MaxQuestionInterval unless:
                //  one of its cached answers needs to be refreshed,
                //  or it's the initial query for a kDNSServiceFlagsThresholdFinder mode browse.
                if (q->BrowseThreshold
                    && (q->CurrentAnswers >= q->BrowseThreshold)
                    && (q->CachedAnswerNeedsUpdate == mDNSfalse)
                    && !((q->flags & kDNSServiceFlagsThresholdFinder) && (q->ThisQInterval == InitialQuestionInterval)))
                {
                    q->SendQNow = mDNSNULL;
                    q->ThisQInterval = MaxQuestionInterval;
                    q->LastQTime = m->timenow;
                    q->RequestUnicast = 0;
                    LogInfo("SendQueries: (%s) %##s reached threshold of %d answers",
                         DNSTypeName(q->qtype), q->qname.c, q->BrowseThreshold);
                }
                else
                {
                    // Mark this question for sending on all interfaces
                    q->SendQNow = mDNSInterfaceMark;
                    q->ThisQInterval *= QuestionIntervalStep;
                }

                debugf("SendQueries: %##s (%s) next interval %d seconds RequestUnicast = %d",
                       q->qname.c, DNSTypeName(q->qtype), q->ThisQInterval / InitialQuestionInterval, q->RequestUnicast);

                if (q->ThisQInterval > MaxQuestionInterval)
                {
                    q->ThisQInterval = MaxQuestionInterval;
                }
                else if (mDNSOpaque16IsZero(q->TargetQID) && q->InterfaceID &&
                         q->CurrentAnswers == 0 && q->ThisQInterval == InitialQuestionInterval * QuestionIntervalStep3 && !q->RequestUnicast &&
                         !(RRTypeIsAddressType(q->qtype) && CacheHasAddressTypeForName(m, &q->qname, q->qnamehash)))
                {
                    // Generally don't need to log this.
                    // It's not especially noteworthy if a query finds no results -- this usually happens for domain
                    // enumeration queries in the LL subdomain (e.g. "db._dns-sd._udp.0.0.254.169.in-addr.arpa")
                    // and when there simply happen to be no instances of the service the client is looking
                    // for (e.g. iTunes is set to look for RAOP devices, and the current network has none).
                    debugf("SendQueries: Zero current answers for %##s (%s); will reconfirm antecedents",
                           q->qname.c, DNSTypeName(q->qtype));
                    // Sending third query, and no answers yet; time to begin doubting the source
                    ReconfirmAntecedents(m, &q->qname, q->qnamehash, q->InterfaceID, 0);
                }
            }

            // Mark for sending. (If no active interfaces, then don't even try.)
            q->SendOnAll = (q->SendQNow == mDNSInterfaceMark);
            if (q->SendOnAll)
            {
                q->SendQNow  = !intf ? mDNSNULL : (q->InterfaceID) ? q->InterfaceID : intf->InterfaceID;
                q->LastQTime = m->timenow;
            }

            // If we recorded a duplicate suppression for this question less than half an interval ago,
            // then we consider it recent enough that we don't need to do an identical query ourselves.
            ExpireDupSuppressInfo(q->DupSuppress, m->timenow - q->ThisQInterval/2);

            q->LastQTxTime      = m->timenow;
            q->RecentAnswerPkts = 0;
            if (q->RequestUnicast) q->RequestUnicast--;
        }
        // For all questions (not just the ones we're sending) check what the next scheduled event will be
        // We don't need to consider NewQuestions here because for those we'll set m->NextScheduledQuery in AnswerNewQuestion
        SetNextQueryTime(m,q);
    }

    // 2. Scan our authoritative RR list to see what probes we might need to send

    m->NextScheduledProbe = m->timenow + FutureTime;

    if (m->CurrentRecord)
        LogMsg("SendQueries ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
    m->CurrentRecord = m->ResourceRecords;
    while (m->CurrentRecord)
    {
        ar = m->CurrentRecord;
        m->CurrentRecord = ar->next;
        if (!AuthRecord_uDNS(ar) && ar->resrec.RecordType == kDNSRecordTypeUnique && ar->resrec.rrtype != kDNSType_OPT)  // For all records that are still probing...
        {
            // 1. If it's not reached its probe time, just make sure we update m->NextScheduledProbe correctly
            if (m->timenow - (ar->LastAPTime + ar->ThisAPInterval) < 0)
            {
                SetNextAnnounceProbeTime(m, ar);
            }
            // 2. else, if it has reached its probe time, mark it for sending and then update m->NextScheduledProbe correctly
            else if (ar->ProbeCount)
            {
                if (ar->AddressProxy.type == mDNSAddrType_IPv4)
                {
                    // There's a problem here. If a host is waking up, and we probe to see if it responds, then
                    // it will see those ARP probes as signalling intent to use the address, so it picks a different one.
                    // A more benign way to find out if a host is responding to ARPs might be send a standard ARP *request*
                    // (using our sender IP address) instead of an ARP *probe* (using all-zero sender IP address).
                    // A similar concern may apply to the NDP Probe too. -- SC
                    LogSPS("SendQueries ARP Probe %d %s %s", ar->ProbeCount, InterfaceNameForID(m, ar->resrec.InterfaceID), ARDisplayString(m,ar));
                    SendARP(m, 1, ar, &zerov4Addr, &zeroEthAddr, &ar->AddressProxy.ip.v4, &ar->WakeUp.IMAC);
                }
                else if (ar->AddressProxy.type == mDNSAddrType_IPv6)
                {
                    LogSPS("SendQueries NDP Probe %d %s %s", ar->ProbeCount, InterfaceNameForID(m, ar->resrec.InterfaceID), ARDisplayString(m,ar));
                    // IPv6 source = zero
                    // No target hardware address
                    // IPv6 target address is address we're probing
                    // Ethernet destination address is Ethernet interface address of the Sleep Proxy client we're probing
                    SendNDP(m, NDP_Sol, 0, ar, &zerov6Addr, mDNSNULL, &ar->AddressProxy.ip.v6, &ar->WakeUp.IMAC);
                }
                // Mark for sending. (If no active interfaces, then don't even try.)
                ar->SendRNow   = (!intf || ar->WakeUp.HMAC.l[0]) ? mDNSNULL : ar->resrec.InterfaceID ? ar->resrec.InterfaceID : intf->InterfaceID;
                ar->LastAPTime = m->timenow;
                // When we have a late conflict that resets a record to probing state we use a special marker value greater
                // than DefaultProbeCountForTypeUnique. Here we detect that state and reset ar->ProbeCount back to the right value.
                if (ar->ProbeCount > DefaultProbeCountForTypeUnique)
                    ar->ProbeCount = DefaultProbeCountForTypeUnique;
                ar->ProbeCount--;
                SetNextAnnounceProbeTime(m, ar);
                if (ar->ProbeCount == 0)
                {
                    // If this is the last probe for this record, then see if we have any matching records
                    // on our duplicate list which should similarly have their ProbeCount cleared to zero...
                    AuthRecord *r2;
                    for (r2 = m->DuplicateRecords; r2; r2=r2->next)
                        if (r2->resrec.RecordType == kDNSRecordTypeUnique && RecordIsLocalDuplicate(r2, ar))
                            r2->ProbeCount = 0;
                    // ... then acknowledge this record to the client.
                    // We do this optimistically, just as we're about to send the third probe.
                    // This helps clients that both advertise and browse, and want to filter themselves
                    // from the browse results list, because it helps ensure that the registration
                    // confirmation will be delivered 1/4 second *before* the browse "add" event.
                    // A potential downside is that we could deliver a registration confirmation and then find out
                    // moments later that there's a name conflict, but applications have to be prepared to handle
                    // late conflicts anyway (e.g. on connection of network cable, etc.), so this is nothing new.
                    if (!ar->Acknowledged) AcknowledgeRecord(m, ar);
                }
            }
            // else, if it has now finished probing, move it to state Verified,
            // and update m->NextScheduledResponse so it will be announced
            else
            {
                if (!ar->Acknowledged) AcknowledgeRecord(m, ar);    // Defensive, just in case it got missed somehow
                ar->resrec.RecordType     = kDNSRecordTypeVerified;
                ar->ThisAPInterval = DefaultAnnounceIntervalForTypeUnique;
                ar->LastAPTime     = m->timenow - DefaultAnnounceIntervalForTypeUnique;
                SetNextAnnounceProbeTime(m, ar);
            }
        }
    }
    m->CurrentRecord = m->DuplicateRecords;
    while (m->CurrentRecord)
    {
        ar = m->CurrentRecord;
        m->CurrentRecord = ar->next;
        if (ar->resrec.RecordType == kDNSRecordTypeUnique && ar->ProbeCount == 0 && !ar->Acknowledged)
            AcknowledgeRecord(m, ar);
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
    mDNSBool queryHasDPCBrowse = mDNSfalse;
#endif
    // 3. Now we know which queries and probes we're sending,
    // go through our interface list sending the appropriate queries on each interface
    while (intf)
    {
        const mDNSu32 OwnerRecordSpace = DetermineOwnerRecordSpace(intf);
        int TraceRecordSpace = (mDNS_McastTracingEnabled && MDNS_TRACER) ? DNSOpt_Header_Space + DNSOpt_TraceData_Space : 0;
        mDNSu8 *queryptr = m->omsg.data;
        mDNSBool useBackgroundTrafficClass = mDNSfalse;    // set if we should use background traffic class
        TSRDataPtrRecHead tsrOpts = SLIST_HEAD_INITIALIZER(tsrOpts);
        TSROptData *newTSROpt;
        mDNSu16 tsrOptsCount = 0;
        mDNSu32 tsrHeaderSpace = (OwnerRecordSpace || TraceRecordSpace) ? 0 : DNSOpt_Header_Space;

        InitializeDNSMessage(&m->omsg.h, zeroID, QueryFlags);
        if (KnownAnswerList) verbosedebugf("SendQueries:   KnownAnswerList set... Will continue from previous packet");
        if (!KnownAnswerList)
        {
            // Start a new known-answer list
            CacheRecord **kalistptr = &KnownAnswerList;
            mDNSu32 answerforecast = OwnerRecordSpace + TraceRecordSpace;  // Start by assuming we'll need at least enough space to put the Owner+Tracer Option
            mDNSu32 numQuestionSkipped = 0;

            // Put query questions in this packet
            for (q = m->Questions; q && q != m->NewQuestions; q=q->next)
            {
                if (mDNSOpaque16IsZero(q->TargetQID) && (q->SendQNow == intf->InterfaceID))
                {
                    mDNSBool Suppress = mDNSfalse;
                    debugf("SendQueries: %s question for %##s (%s) at %d forecast total %d",
                           SuppressOnThisInterface(q->DupSuppress, intf) ? "Suppressing" : "Putting    ",
                           q->qname.c, DNSTypeName(q->qtype), queryptr - m->omsg.data, queryptr + answerforecast - m->omsg.data);

                    mDNSBool updateInterface = mDNSfalse;
                    // If interface is P2P type, verify that query should be sent over it.
                    if (!mDNSPlatformValidQuestionForInterface(q, intf))
                    {
                        updateInterface = mDNStrue;
                    }
                #if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
                    else if (DPCSuppressMDNSQuery(q, intf->InterfaceID))
                    {
                        updateInterface = mDNStrue;
                    }
                #endif
                    // If we're suppressing this question, or we successfully put it, update its SendQNow state
                    else if ((Suppress = SuppressOnThisInterface(q->DupSuppress, intf)) ||
                        BuildQuestion(m, intf, &m->omsg, &queryptr, q, &kalistptr, &answerforecast))
                    {
                        if (Suppress)
                        {
                            m->mDNSStats.DupQuerySuppressions++;
                        }
                    #if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
                        else if (DNSQuestionCollectsMDNSMetric(q))
                        {
                            // 1. If this is our first time to send query, or
                            // 2. If we have received some response for the query we sent out before
                            // The delay timer ticks starting from 0. (which means we are calculating a new delay for a
                            // new query sent out.)
                            // The reason why we do this:
                            // If we have not received any response since our first query has been sent out, then
                            // reissuing a query should not make the delay we are counting shorter. The more query
                            // retransmission we have, the greater the response delay should be. Therefore, if we have
                            // not received any response to the current query, do not update the first query time even
                            // if we have sent multiple queries.
                            // However, if the query has been answered by at least one answer (from wire, not from
                            // cache), then reissuing a new query should reset the first query time so that the delay
                            // is not exaggerated.
                            if ((q->metrics.firstQueryTime == 0) || (q->metrics.answered))
                            {
                                q->metrics.firstQueryTime = NonZeroTime(m->timenow);
                                q->metrics.answered = mDNSfalse;
                            }
                            q->metrics.querySendCount++;
                        }
                    #endif
                        updateInterface = mDNStrue;
                        if (q->WakeOnResolveCount)
                        {
                            mDNSSendWakeOnResolve(m, q);
                            q->WakeOnResolveCount--;
                        }

                        // use background traffic class if any included question requires it
                        if (q->UseBackgroundTraffic)
                        {
                            useBackgroundTrafficClass = mDNStrue;
                        }
                    }
                    if (updateInterface)
                    {
                        q->SendQNow = (q->InterfaceID || !q->SendOnAll) ? mDNSNULL : GetNextActiveInterfaceID(intf);
                    }
                }
            }
        #if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
            // If the message being constructed for the current interface contains at least one non-probe question,
            // try to opportunistically include the Discovery Proxy browse's question as well.
            if (DPCFeatureEnabled() && !queryHasDPCBrowse && (m->omsg.h.numQuestions > 0))
            {
                const mDNSu8 *questionPtr = m->omsg.data;
                const mDNSu8 *const end = queryptr;
                for (mDNSu32 i = 0; i < m->omsg.h.numQuestions; i++)
                {
                    DNSQuestion question;
                    questionPtr = getQuestion(&m->omsg, questionPtr, end, mDNSInterface_Any, &question);
                    if (!questionPtr)
                    {
                        break;
                    }
                    question.qclass &= ~kDNSQClass_UnicastResponse;
                    if ((question.qtype == DPCBrowse.qtype) && (question.qclass == DPCBrowse.qclass) &&
                        (question.qnamehash == DPCBrowse.qnamehash) && SameDomainName(&question.qname, &DPCBrowse.qname))
                    {
                        queryHasDPCBrowse = mDNStrue;
                        break;
                    }
                }
                if (!queryHasDPCBrowse)
                {
                    DPCBrowse.SendQNow = intf->InterfaceID;
                    DPCBrowse.RequestUnicast = kDefaultRequestUnicastCount;
                    BuildQuestion(m, intf, &m->omsg, &queryptr, &DPCBrowse, &kalistptr, &answerforecast);
                    DPCBrowse.SendQNow = mDNSNULL;
                    queryHasDPCBrowse = mDNStrue;
                }
            }
        #endif
            // Put probe questions in this packet
            for (ar = m->ResourceRecords; ar; ar=ar->next)
            {   // Skip if already marked for probing, or interface does not match, or TSR record
                if (ar->IncludeInProbe || (ar->SendRNow != intf->InterfaceID) || 
                    (ar->resrec.rrtype == kDNSType_OPT))
                    continue;

                // If interface is a P2P variant, verify that the probe should be sent over it.
                if (!mDNSPlatformValidRecordForInterface(ar, intf->InterfaceID))
                {
                    ar->SendRNow = (ar->resrec.InterfaceID) ? mDNSNULL : GetNextActiveInterfaceID(intf);
                    ar->IncludeInProbe = mDNSfalse;
                }
                else
                {
                    mDNSBool ucast = (ar->ProbeCount >= DefaultProbeCountForTypeUnique-1) && m->CanReceiveUnicastOn5353 && intf->SupportsUnicastMDNSResponse;
                    mDNSu16 ucbit = (mDNSu16)(ucast ? kDNSQClass_UnicastResponse : 0);
                    const mDNSu8 *const limit = m->omsg.data + (m->omsg.h.numQuestions ? NormalMaxDNSMessageData : AbsoluteMaxDNSMessageData);
                    mDNSu32 forecast = answerforecast;
                    mDNSBool putProbeQuestion = mDNStrue;
                    mDNSu16 qclass = ar->resrec.rrclass | ucbit;

                    {// Determine if this probe question is already in packet's dns message
                        const mDNSu8 *questionptr = m->omsg.data;
                        DNSQuestion question;
                        mDNSu16 n;
                        for (n = 0; n < m->omsg.h.numQuestions && questionptr; n++)
                        {
                            questionptr = getQuestion(&m->omsg, questionptr, limit, mDNSInterface_Any, &question);
                            if (questionptr && (question.qtype == kDNSQType_ANY) && (question.qclass == qclass) &&
                                (question.qnamehash == ar->resrec.namehash) && SameDomainName(&question.qname, ar->resrec.name))
                            {
                                putProbeQuestion = mDNSfalse;  // set to false if already in message
                                break;
                            }
                        }
                    }

                    if (TSROptGetIfNew(m, ar, &tsrOpts) != mDNSNULL)
                    {
                        forecast += (DNSOpt_TSRData_Space + ((tsrOptsCount == 0) ? tsrHeaderSpace : 0));
                        tsrOptsCount++;
                    }

                    for (const AuthRecord *tmp = m->ResourceRecords; tmp; tmp = tmp->next)
                    {
                        if (AddRecordInProbe(m, ar, tmp, intf->InterfaceID))
                        {
                            // compressed name (2) type (2) class (2) TTL (4) rdlength (2) estimated rdata length
                            forecast = forecast + 12 + tmp->resrec.rdestimate;
                        }
                    }

                    if (putProbeQuestion)
                    {
                        mDNSu8 *newptr = putQuestion(&m->omsg, queryptr, limit - forecast, ar->resrec.name, kDNSQType_ANY, qclass);
                        if (!newptr) {
                            // There is not enough space for the probe question and its corresponding records
                            numQuestionSkipped++;
                            continue;
                        }
                        else
                        {
                            queryptr = newptr;
                            verbosedebugf("SendQueries:   Put Question %##s (%s) probecount %d InterfaceID= %d %d %d",
                                          ar->resrec.name->c, DNSTypeName(ar->resrec.rrtype), ar->ProbeCount, ar->resrec.InterfaceID, ar->resrec.rdestimate, answerforecast);
                        }
                    }
                    else
                    {
                        if (queryptr + forecast >= limit)
                        {
                            // There is not enough space
                            continue;
                        }
                    }

                    answerforecast = forecast;
                    for (AuthRecord *tmp = m->ResourceRecords; tmp; tmp = tmp->next)
                    {
                        if (AddRecordInProbe(m, ar, tmp, intf->InterfaceID))
                        {
                            tmp->SendRNow = (ar->resrec.InterfaceID) ? mDNSNULL : GetNextActiveInterfaceID(intf);
                            tmp->IncludeInProbe = mDNStrue;
                        }
                    }
                }
            }
            if (numQuestionSkipped > 0)
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG, "SendQueries: %u questions will be sent in a later request on InterfaceID= %p",
                          numQuestionSkipped, intf->InterfaceID);
            }
        }

        // Put our known answer list (either new one from this question or questions, or remainder of old one from last time)
        while (KnownAnswerList)
        {
            CacheRecord *ka = KnownAnswerList;
            mDNSu32 SecsSinceRcvd = ((mDNSu32)(m->timenow - ka->TimeRcvd)) / mDNSPlatformOneSecond;
            mDNSu8 *newptr = PutResourceRecordTTLWithLimit(&m->omsg, queryptr, &m->omsg.h.numAnswers, &ka->resrec, ka->resrec.rroriginalttl - SecsSinceRcvd,
                                                           m->omsg.data + NormalMaxDNSMessageData - RR_OPT_SPACE);
            if (newptr)
            {
                verbosedebugf("SendQueries:   Put %##s (%s) at %d - %d",
                              ka->resrec.name->c, DNSTypeName(ka->resrec.rrtype), queryptr - m->omsg.data, newptr - m->omsg.data);
                queryptr = newptr;
                KnownAnswerList = ka->NextInKAList;
                ka->NextInKAList = mDNSNULL;
            }
            else
            {
                // If we ran out of space and we have more than one question in the packet, that's an error --
                // we shouldn't have put more than one question if there was a risk of us running out of space.
                if (m->omsg.h.numQuestions > 1)
                    LogMsg("SendQueries:   Put %d answers; No more space for known answers", m->omsg.h.numAnswers);
                m->omsg.h.flags.b[0] |= kDNSFlag0_TC;
                break;
            }
        }

        tsrOptsCount = 0;
        for (ar = m->ResourceRecords; ar; ar=ar->next)
        {
            if (ar->IncludeInProbe)
            {
                mDNSu8 *newptr = PutResourceRecordTTLWithLimit(&m->omsg, queryptr, &m->omsg.h.numAuthorities, &ar->resrec, ar->resrec.rroriginalttl,
                                                               (m->omsg.h.numQuestions > 1 ? m->omsg.data + NormalMaxDNSMessageData : m->omsg.data + AbsoluteMaxDNSMessageData));
                ar->IncludeInProbe = mDNSfalse;
                if (newptr) queryptr = newptr;
                else LogMsg("SendQueries:   How did we fail to have space for the Update record %s", ARDisplayString(m,ar));

                if ((newTSROpt = TSROptGetIfNew(m, ar, &tsrOpts)) != mDNSNULL)
                {
                    tsrOptsCount++;
                    TSRDataRecPtrHeadAddTSROpt(&tsrOpts, newTSROpt, ar->resrec.name, m->omsg.h.numAnswers + m->omsg.h.numAdditionals + m->omsg.h.numAuthorities - 1);
                }
            }
        }

        if (queryptr > m->omsg.data)
        {
            // If we have data to send, add OWNER/TRACER/OWNER+TRACER option if necessary, then send packet
            if (OwnerRecordSpace || TraceRecordSpace || !SLIST_EMPTY(&tsrOpts))
            {
                mDNSu8 *saveptr;
                AuthRecord opt;
                mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
                opt.resrec.rrclass    = NormalMaxDNSMessageData;
                opt.resrec.rdlength   = 0;
                opt.resrec.rdestimate = 0;
                mDNSu16 optCount      = 0;
                if (OwnerRecordSpace)
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupOwnerOpt(m, intf, &opt.resrec.rdata->u.opt[optCount++]);
                }
                if (TraceRecordSpace)
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupTracerOpt(m, &opt.resrec.rdata->u.opt[optCount++]);
                }
                if (!SLIST_EMPTY(&tsrOpts))
                {
                    opt.resrec.rdlength   += sizeof(rdataOPT);
                    opt.resrec.rdestimate += sizeof(rdataOPT);
                    SetupTSROpt(SLIST_FIRST(&tsrOpts)->tsr, &opt.resrec.rdata->u.opt[optCount++]);
                    TSRDataRecPtrHeadRemoveAndFreeFirst(&tsrOpts);
                }
                // Put record after first TSR
                saveptr = queryptr;
                queryptr = PutResourceRecordTTLWithLimit(&m->omsg, queryptr, &m->omsg.h.numAdditionals, &opt.resrec,
                    opt.resrec.rroriginalttl, m->omsg.data + AbsoluteMaxDNSMessageData);
                if (queryptr && !SLIST_EMPTY(&tsrOpts))
                {
                    mDNSu8 *rdlengthptr = saveptr + 2 + 2 + 4 + 1; // rrtype, rrclass, ttl, 0-length name
                    queryptr = AddTSRROptsToMessage(&tsrOpts, &m->omsg, rdlengthptr, queryptr,
                        m->omsg.data + AbsoluteMaxDNSMessageData);
                }
                if (!queryptr)
                {
                    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "SendQueries: How did we fail to have space for %s %s TSR(%d) OPT record (%d/%d/%d/%d) %s",
                        OwnerRecordSpace ? "OWNER" : "", TraceRecordSpace ? "TRACER" : "", tsrOptsCount,
                        m->omsg.h.numQuestions, m->omsg.h.numAnswers, m->omsg.h.numAuthorities, m->omsg.h.numAdditionals, ARDisplayString(m, &opt));
                }
                if (queryptr > m->omsg.data + NormalMaxDNSMessageData)
                {
                    if (m->omsg.h.numQuestions != 1 || m->omsg.h.numAnswers != 0 || m->omsg.h.numAuthorities != 1 || m->omsg.h.numAdditionals != 1)
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "SendQueries: Why did we generate oversized packet with %s %s TSR(%d) OPT record %p %p %p (%d/%d/%d/%d) %s",
                            OwnerRecordSpace ? "OWNER" : "", TraceRecordSpace ? "TRACER" : "", tsrOptsCount,
                            m->omsg.data, m->omsg.data + NormalMaxDNSMessageData, queryptr, m->omsg.h.numQuestions,
                            m->omsg.h.numAnswers, m->omsg.h.numAuthorities, m->omsg.h.numAdditionals, ARDisplayString(m, &opt));
                }
            }

            if ((m->omsg.h.flags.b[0] & kDNSFlag0_TC) && m->omsg.h.numQuestions > 1)
                LogMsg("SendQueries: Should not have more than one question (%d) in a truncated packet", m->omsg.h.numQuestions);
            debugf("SendQueries:   Sending %d Question%s %d Answer%s %d Update%s on %d (%s)",
                   m->omsg.h.numQuestions,   m->omsg.h.numQuestions   == 1 ? "" : "s",
                   m->omsg.h.numAnswers,     m->omsg.h.numAnswers     == 1 ? "" : "s",
                   m->omsg.h.numAdditionals, m->omsg.h.numAdditionals == 1 ? "" : "s",
                   m->omsg.h.numAuthorities, m->omsg.h.numAuthorities == 1 ? "" : "s", IIDPrintable(intf->InterfaceID), intf->ifname);
            if (intf->IPv4Available) mDNSSendDNSMessage(m, &m->omsg, queryptr, intf->InterfaceID, mDNSNULL, mDNSNULL, &AllDNSLinkGroup_v4, MulticastDNSPort, mDNSNULL, useBackgroundTrafficClass);
            if (intf->IPv6Available) mDNSSendDNSMessage(m, &m->omsg, queryptr, intf->InterfaceID, mDNSNULL, mDNSNULL, &AllDNSLinkGroup_v6, MulticastDNSPort, mDNSNULL, useBackgroundTrafficClass);
            if (!m->SuppressQueries) m->SuppressQueries = NonZeroTime(m->timenow + ((mDNSPlatformOneSecond + 9) / 10));
            if (++pktcount >= 1000)
            { LogMsg("SendQueries exceeded loop limit %d: giving up", pktcount); break; }
            // There might be more records left in the known answer list, or more questions to send
            // on this interface, so go around one more time and try again.
        }
        else    // Nothing more to send on this interface; go to next
        {
            const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
            intf = next;
        #if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
            queryHasDPCBrowse = mDNSfalse;
        #endif
        }
        TSRDataRecPtrHeadFreeList(&tsrOpts);
    }

    // 4. Final housekeeping

    // 4a. Debugging check: Make sure we announced all our records
    for (ar = m->ResourceRecords; ar; ar=ar->next)
        if (ar->SendRNow)
        {
            if (ar->ARType != AuthRecordLocalOnly && ar->ARType != AuthRecordP2P)
                LogInfo("SendQueries: No active interface %d to send probe: %d %s",
                        IIDPrintable(ar->SendRNow), IIDPrintable(ar->resrec.InterfaceID), ARDisplayString(m, ar));
            ar->SendRNow = mDNSNULL;
        }

    // 4b. When we have lingering cache records that we're keeping around for a few seconds in the hope
    // that their interface which went away might come back again, the logic will want to send queries
    // for those records, but we can't because their interface isn't here any more, so to keep the
    // state machine ticking over we just pretend we did so.
    // If the interface does not come back in time, the cache record will expire naturally
    FORALL_CACHERECORDS(slot, cg, cr)
    {
        if (cr->CRActiveQuestion && cr->UnansweredQueries < MaxUnansweredQueries)
        {
            if (m->timenow + TicksTTL(cr)/50 - cr->NextRequiredQuery >= 0)
            {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
                if (cr->LastUnansweredTime != m->timenow) // It was not just sent
#endif
                {
                    cr->UnansweredQueries++;
                }
                cr->CRActiveQuestion->SendQNow = mDNSNULL;
                SetNextCacheCheckTimeForRecord(m, cr);
            }
        }
    }

    // 4c. Debugging check: Make sure we sent all our planned questions
    // Do this AFTER the lingering cache records check above, because that will prevent spurious warnings for questions
    // we legitimately couldn't send because the interface is no longer available
    for (q = m->Questions; q; q=q->next)
    {
        if (q->SendQNow)
        {
            DNSQuestion *x;
            for (x = m->NewQuestions; x; x=x->next) if (x == q) break;  // Check if this question is a NewQuestion
            // There will not be an active interface for questions applied to mDNSInterface_BLE
            // so don't log the warning in that case.
            if (q->InterfaceID != mDNSInterface_BLE)
                LogInfo("SendQueries: No active interface %d to send %s question: %d %##s (%s)",
                        IIDPrintable(q->SendQNow), x ? "new" : "old", IIDPrintable(q->InterfaceID), q->qname.c, DNSTypeName(q->qtype));
            q->SendQNow = mDNSNULL;
        }
        q->CachedAnswerNeedsUpdate = mDNSfalse;
    }
}

mDNSlocal void SendWakeup(mDNS *const m, mDNSInterfaceID InterfaceID, mDNSEthAddr *EthAddr, mDNSOpaque48 *password, mDNSBool unicastOnly)
{
    int i, j;

    mDNSu8 *ptr = m->omsg.data;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, InterfaceID);
    if (!intf)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "SendARP: No interface with InterfaceID %p found", InterfaceID);
        return;
    }

    // 0x00 Destination address
    for (i=0; i<6; i++) *ptr++ = EthAddr->b[i];

    // 0x06 Source address (Note: Since we don't currently set the BIOCSHDRCMPLT option, BPF will fill in the real interface address for us)
    for (i=0; i<6; i++) *ptr++ = intf->MAC.b[0];

    // 0x0C Ethertype (0x0842)
    *ptr++ = 0x08;
    *ptr++ = 0x42;

    // 0x0E Wakeup sync sequence
    for (i=0; i<6; i++) *ptr++ = 0xFF;

    // 0x14 Wakeup data
    for (j=0; j<16; j++) for (i=0; i<6; i++) *ptr++ = EthAddr->b[i];

    // 0x74 Password
    for (i=0; i<6; i++) *ptr++ = password->b[i];

    mDNSPlatformSendRawPacket(m->omsg.data, ptr, InterfaceID);

    if (!unicastOnly)
    {
        // For Ethernet switches that don't flood-foward packets with unknown unicast destination MAC addresses,
        // broadcast is the only reliable way to get a wakeup packet to the intended target machine.
        // For 802.11 WPA networks, where a sleeping target machine may have missed a broadcast/multicast
        // key rotation, unicast is the only way to get a wakeup packet to the intended target machine.
        // So, we send one of each, unicast first, then broadcast second.
        for (i=0; i<6; i++) m->omsg.data[i] = 0xFF;
        mDNSPlatformSendRawPacket(m->omsg.data, ptr, InterfaceID);
    }
}

// ***************************************************************************
// MARK: - RR List Management & Task Management

// Whenever a question is answered, reset its state so that we don't query
// the network repeatedly. This happens first time when we answer the question and
// and later when we refresh the cache.
mDNSlocal void ResetQuestionState(mDNS *const m, DNSQuestion *q)
{
    q->LastQTime          = m->timenow;
    q->LastQTxTime        = m->timenow;
    q->RecentAnswerPkts   = 0;
    q->ThisQInterval      = MaxQuestionInterval;
    q->RequestUnicast     = 0;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    q->NeedUpdatedQuerier = mDNSfalse;
#else
    // Reset unansweredQueries so that we don't penalize this server later when we
    // start sending queries when the cache expires.
    q->unansweredQueries  = 0;
#endif
    debugf("ResetQuestionState: Set MaxQuestionInterval for %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
}

mDNSlocal void AdjustUnansweredQueries(mDNS *const m, CacheRecord *const rr)
{
    const mDNSs32 expireTime = RRExpireTime(rr);
    const mDNSu32 interval = TicksTTL(rr) / 20; // Calculate 5% of the cache record's TTL.
    mDNSu32 rem;

    // If the record is expired or UnansweredQueries is already at the max, then return early.
    if (((m->timenow - expireTime) >= 0) || (rr->UnansweredQueries >= MaxUnansweredQueries)) return;

    if (interval == 0)
    {
        LogInfo("AdjustUnansweredQueries: WARNING: unusually small TTL (%d ticks) for %s", TicksTTL(rr), CRDisplayString(m, rr));
        return;
    }

    // Calculate the number of whole 5% TTL intervals between now and expiration time.
    rem = ((mDNSu32)(expireTime - m->timenow)) / interval;

    // Calculate the expected number of remaining refresher queries.
    // Refresher queries are sent at the start of the last MaxUnansweredQueries intervals.
    if (rem > MaxUnansweredQueries) rem = MaxUnansweredQueries;

    // If the current number of remaining refresher queries is greater than expected, then at least one refresher query time
    // was missed. This can happen if the cache record didn't have an active question during any of the times at which
    // refresher queries would have been sent if the cache record did have an active question. The cache record's
    // UnansweredQueries count needs to be adjusted to avoid a burst of refresher queries being sent in an attempt to make up
    // for lost time. UnansweredQueries is set to the number of queries that would have been sent had the cache record had an
    // active question from the 80% point of its lifetime up to now, with one exception: if the number of expected remaining
    // refresher queries is zero (because timenow is beyond the 95% point), then UnansweredQueries is set to
    // MaxUnansweredQueries - 1 so that at least one refresher query is sent before the cache record expires.
	// Note: The cast is safe because rem is never greater than MaxUnansweredQueries; the comparison has to be signed.
    if ((MaxUnansweredQueries - rr->UnansweredQueries) > (mDNSs32)rem)
    {
        if (rem == 0) rem++;
        rr->UnansweredQueries = (mDNSu8)(MaxUnansweredQueries - rem);
    }
}

// Note: AnswerCurrentQuestionWithResourceRecord can call a user callback, which may change the record list and/or question list.
// Any code walking either list must use the m->CurrentQuestion (and possibly m->CurrentRecord) mechanism to protect against this.
// In fact, to enforce this, the routine will *only* answer the question currently pointed to by m->CurrentQuestion,
// which will be auto-advanced (possibly to NULL) if the client callback cancels the question.
mDNSexport void AnswerCurrentQuestionWithResourceRecord(mDNS *const m, CacheRecord *const rr, const QC_result AddRecord)
{
    DNSQuestion *const q = m->CurrentQuestion;
    const mDNSBool followcname = FollowCNAME(q, &rr->resrec, AddRecord);

    verbosedebugf("AnswerCurrentQuestionWithResourceRecord:%4lu %s (%s) TTL %d %s",
                  q->CurrentAnswers, AddRecord ? "Add" : "Rmv", MortalityDisplayString(rr->resrec.mortality),
                  rr->resrec.rroriginalttl, CRDisplayString(m, rr));

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
    // When a cache record is used to answer a question, check if we need to enable sensitive logging for it.
    if (DNSQuestionNeedsSensitiveLogging(q)) // If the question enables sensitive logging.
    {
        // default means that we have not remember any private log level, by default, this record will be printed in
        // the state dump output.
        if (rr->PrivacyLevel == mDNSCRLogPrivacyLevel_Default)
        {
            // Since this cache record is used to answer a question that enables sensitive logging, it will be
            // marked as private, and will be redacted in the state dump.
            rr->PrivacyLevel = mDNSCRLogPrivacyLevel_Private;
            // If it is associated with an SOA record, the SOA should also be marked as private because it
            // has the same record name.
            if (rr->soa)
            {
                rr->soa->PrivacyLevel = mDNSCRLogPrivacyLevel_Private;
            }
        }
    }
    else // q->logPrivacyLevel == dnssd_log_privacy_level_default // If the question does not enable sensitive logging.
    {
        // If this cache record is used to answer a question that does not enable sensitive logging, it will be printed
        // in the state dump.
        rr->PrivacyLevel = mDNSCRLogPrivacyLevel_Public;
        if (rr->soa)
        {
            rr->soa->PrivacyLevel = mDNSCRLogPrivacyLevel_Public;
        }
    }
#endif

    // Normally we don't send out the unicast query if we have answered using our local only auth records e.g., /etc/hosts.
    // But if the query for "A" record has a local answer but query for "AAAA" record has no local answer, we might
    // send the AAAA query out which will come back with CNAME and will also answer the "A" query. To prevent that,
    // we check to see if that query already has a unique local answer.
    if (q->LOAddressAnswers)
    {
        LogInfo("AnswerCurrentQuestionWithResourceRecord: Question %p %##s (%s) not answering with record %s due to "
                "LOAddressAnswers %d", q, q->qname.c, DNSTypeName(q->qtype), ARDisplayString(m, rr),
                q->LOAddressAnswers);
        return;
    }

    if (q->Suppressed && (AddRecord != QC_suppressed) && !(q->ForceCNAMEFollows && followcname))
    {
        // If the query is suppressed, then we don't want to answer from the cache. But if this query is
        // supposed to time out, we still want to callback the clients. We do this only for TimeoutQuestions
        // that are timing out, which we know are answered with negative cache record when timing out.
        if (!q->TimeoutQuestion || rr->resrec.RecordType != kDNSRecordTypePacketNegative || (m->timenow - q->StopTime < 0))
            return;
    }

    //  Set the record to immortal if appropriate
    if (!mDNSOpaque16IsZero(q->TargetQID) && (AddRecord == QC_add) && (rr->resrec.mortality == Mortality_Mortal))
    {
        switch (q->ExpRecordPolicy)
        {
            case mDNSExpiredRecordPolicy_DoNotUse:
            MDNS_COVERED_SWITCH_DEFAULT:
                break;

            case mDNSExpiredRecordPolicy_UseCached:
            case mDNSExpiredRecordPolicy_Immortalize:
            {
                mDNSBool eligible = (rr->resrec.RecordType != kDNSRecordTypePacketNegative);
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                // For now, do not use optimistic DNS for DNSSEC response and DNSSEC question.
                eligible = eligible && !dns_question_is_dnssec_requestor(q) && !resource_record_is_dnssec_aware(&rr->resrec);
            #endif
                if (eligible)
                {
                    rr->resrec.mortality = Mortality_Immortal;
                }
                break;
            }
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
    if ((AddRecord == QC_add) && Question_uDNS(q) && !followcname && !q->metrics.answered)
    {
        mDNSBool skipUpdate = mDNSfalse;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        mdns_resolver_type_t resolver_type = (!q->dnsservice) ? mdns_resolver_type_null : mdns_dns_service_get_resolver_type(q->dnsservice);
        if (resolver_type == mdns_resolver_type_null)
        {
            skipUpdate = mDNStrue;
        }
#endif
        if (!skipUpdate)
        {
            uint32_t         responseLatencyMs, querySendCount;
            bool            isForCellular;
            dns_transport_t transport;

            querySendCount = q->metrics.querySendCount;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            const mdns_querier_t querier = mdns_querier_downcast(q->client);
            if (querier)
            {
                querySendCount += mdns_querier_get_send_count(querier);
            }
            isForCellular = mdns_dns_service_interface_is_cellular(q->dnsservice);
            transport = dnssd_analytics_dns_transport_for_resolver_type(resolver_type);
#else
            isForCellular = (q->qDNSServer && q->qDNSServer->isCell);
            transport = dns_transport_Do53;
#endif
            if (querySendCount > 0 && q->metrics.firstQueryTime != 0)
            {
                responseLatencyMs = ((m->timenow - q->metrics.firstQueryTime) * 1000) / mDNSPlatformOneSecond;
                dnssd_analytics_update_dns_query_info(isForCellular, transport, q->qtype, querySendCount,
                    responseLatencyMs, rr->resrec.RecordType != kDNSRecordTypePacketNegative);
            }
        }
        q->metrics.answered = mDNStrue;
    }
#endif
    // Note: Use caution here. In the case of records with rr->DelayDelivery set, AnswerCurrentQuestionWithResourceRecord(... mDNStrue)
    // may be called twice, once when the record is received, and again when it's time to notify local clients.
    // If any counters or similar are added here, care must be taken to ensure that they are not double-incremented by this.

    if (AddRecord == QC_add && !q->DuplicateOf && rr->CRActiveQuestion != q && rr->resrec.mortality != Mortality_Ghost)
    {
        debugf("AnswerCurrentQuestionWithResourceRecord: Updating CRActiveQuestion from %p to %p for cache record %s, CurrentAnswer %d",
               rr->CRActiveQuestion, q, CRDisplayString(m,rr), q->CurrentAnswers);
        if (!rr->CRActiveQuestion)
        {
            m->rrcache_active++;            // If not previously active, increment rrcache_active count
            AdjustUnansweredQueries(m, rr); // Adjust UnansweredQueries in case the record missed out on refresher queries
        }
        rr->CRActiveQuestion = q;           // We know q is non-null
        SetNextCacheCheckTimeForRecord(m, rr);
    }

    // If this is:
    // (a) a no-cache add, where we've already done at least one 'QM' query, or
    // (b) a normal add, where we have at least one unique-type answer,
    // then there's no need to keep polling the network.
    // (If we have an answer in the cache, then we'll automatically ask again in time to stop it expiring.)
    // We do this for mDNS questions and uDNS one-shot questions, but not for
    // uDNS LongLived questions, because that would mess up our LLQ lease renewal timing.
    if ((AddRecord == QC_addnocache && !q->RequestUnicast) ||
        (AddRecord == QC_add && (q->ExpectUnique || (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask))))
        if (ActiveQuestion(q) && (mDNSOpaque16IsZero(q->TargetQID) || !q->LongLived))
        {
            ResetQuestionState(m, q);
        }

    if (rr->DelayDelivery) return;      // We'll come back later when CacheRecordDeferredAdd() calls us

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
    // If DNS64StateMachine() returns true, then the question was restarted as a different question, so return.
    if (!mDNSOpaque16IsZero(q->TargetQID) && DNS64StateMachine(m, q, &rr->resrec, AddRecord)) return;
#endif

#ifdef USE_LIBIDN
    if (rr->resrec.RecordType == kDNSRecordTypePacketNegative)  // If negative answer, check if we need to try Punycode conversion
    {
        domainname newname;
        if (PerformNextPunycodeConversion(q, &newname))         // Itertative Punycode conversion succeeded, so reissue question with new name
        {
            UDPSocket *const sock = q->LocalSocket;             // Save old socket and transaction ID
            const mDNSOpaque16 id = q->TargetQID;
            q->LocalSocket = mDNSNULL;
            mDNS_StopQuery_internal(m, q);                      // Stop old query
            AssignDomainName(&q->qname, &newname);              // Update qname
            q->qnamehash = DomainNameHashValue(&q->qname);      // and namehash
            mDNS_StartQuery_internal(m, q);                     // Start new query

            if (sock)                                           // Transplant saved socket, if appropriate
            {
                if (q->DuplicateOf) mDNSPlatformUDPClose(sock);
                else { q->LocalSocket = sock; q->TargetQID = id; }
            }
            return;                                             // All done for now; wait until we get the next answer
        }
    }
#endif // USE_LIBIDN

    // Only deliver negative answers if client has explicitly requested them except when we are forcing a negative response
    // for the purpose of retrying search domains/timeout OR the question is suppressed
    const mDNSBool answersQuestionNegativelyDirectly = (rr->resrec.RecordType == kDNSRecordTypePacketNegative);
    const mDNSBool answersMDNSQuestionNegativelyIndirectly = (q->qtype != kDNSType_NSEC && RRAssertsNonexistence(&rr->resrec, q->qtype));
    if (answersQuestionNegativelyDirectly || answersMDNSQuestionNegativelyIndirectly)
    {
        switch (AddRecord)
        {
            case QC_rmv:
                // Do not deliver remove event for a negative record, because when it is deleted there must be a
                // positive record that answers the question coming. The add event of the positive record will
                // implicitly indicate the remove of the previously added negative record.
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                // However, if the question is the primary DNSSEC requestor that will validate the records with DNSSEC,
                // always deliver the negative record because it contains the denial of existence records that could
                // have changed.
                if (dns_question_is_primary_dnssec_requestor(q))
                {
                    // answersMDNSQuestionNegativelyIndirectly should be false here.
                    break;
                }
            #endif
                return;
            case QC_add:
            case QC_addnocache:
                if (!q->ReturnIntermed)
                {
                    // If the question does not want the intermediate result (for example, NXDomain or NoSuchRecord)
                    // rather than the requested record, do not deliver the negative record.
                    return;
                }
                break;
            case QC_forceresponse:
            case QC_suppressed:
                // A negative response is forced to be delivered to the callback;
                // Fall through.
            MDNS_COVERED_SWITCH_DEFAULT:
                break;
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
    if (AddRecord && DNSQuestionCollectsMDNSMetric(q))
    {
        // AddRecord proves that we get a new response.
        if (!q->metrics.answered)
        {
            q->metrics.answered = mDNStrue;
        }

        const mDNSBool cache_hit = !q->InitialCacheMiss;
        // Iterate through all interfaces to save the response delay and cache hit data to the specific interface.
        for (const NetworkInterfaceInfo *interface = GetFirstActiveInterface(m->HostInterfaces); interface;
             interface = GetFirstActiveInterface(interface->next))
        {
            if (interface->delayHistogram && (interface->InterfaceID == rr->resrec.InterfaceID))
            {
                if (cache_hit)
                {
                    mdns_multicast_delay_histogram_collect_cache_hit(interface->delayHistogram);
                }
                else
                {
                    const mDNSs32 diffInTicks = (m->timenow - q->metrics.firstQueryTime);
                    if ((q->metrics.firstQueryTime != 0) && (diffInTicks > 0))
                    {
                        const mDNSu32 delayInInMillisecond = getMillisecondsFromTicks(diffInTicks);

                        mdns_multicast_delay_histogram_collect_delay(interface->delayHistogram, delayInInMillisecond);
                    }
                }
                break;
            }
        }
    }
#endif

    if (q->QuestionCallback)
    {
        // For CNAME results to non-CNAME questions, only inform the client if they want all intermediates or if
        // the expired record policy is mDNSExpiredRecordPolicy_UseCached.
        if (!followcname || q->ReturnIntermed || (q->ExpRecordPolicy == mDNSExpiredRecordPolicy_UseCached))
        {
            mDNS_DropLockBeforeCallback();      // Allow client (and us) to legally make mDNS API calls
            if (q->qtype != kDNSType_NSEC && RRAssertsNonexistence(&rr->resrec, q->qtype))
            {
                if (mDNSOpaque16IsZero(q->TargetQID))
                {
                    CacheRecord neg;
                #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                    mDNSPlatformMemZero(&neg, sizeof(neg));
                #endif
                    MakeNegativeCacheRecordForQuestion(m, &neg, q, 1, rr->resrec.InterfaceID, zeroID);
                    q->QuestionCallback(m, q, &neg.resrec, AddRecord);
                }
            }
            else
            {
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
                if (DNS64ShouldAnswerQuestion(q, &rr->resrec))
                {
                    DNS64AnswerCurrentQuestion(m, &rr->resrec, AddRecord);
                }
                else
            #endif
                {
                    q->QuestionCallback(m, q, &rr->resrec, AddRecord);
                }
            }
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
        }
    }
    // Note: Proceed with caution after this point because client callback function
    // invoked above is allowed to do anything, such as starting/stopping queries
    // (including this one itself, or the next or previous query in the linked list),
    // registering/deregistering records, starting/stopping NAT traversals, etc.

    if (m->CurrentQuestion == q)
    {
        // If we get a CNAME back while we are validating the response (i.e., CNAME for DS, DNSKEY, RRSIG),
        // don't follow them. If it is a ValidationRequired question, wait for the CNAME to be validated
        // first before following it
        if (followcname)  AnswerQuestionByFollowingCNAME(m, q, &rr->resrec);
    }
}

mDNSlocal void CacheRecordDeferredAdd(mDNS *const m, CacheRecord *cr)
{
    cr->DelayDelivery = 0;
    if (m->CurrentQuestion)
        LogMsg("CacheRecordDeferredAdd ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;
    while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
    {
        DNSQuestion *q = m->CurrentQuestion;
        if (CacheRecordAnswersQuestion(cr, q))
            AnswerCurrentQuestionWithResourceRecord(m, cr, QC_add);
        if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
            m->CurrentQuestion = q->next;
    }
    m->CurrentQuestion = mDNSNULL;
}

// Special values to pass to CheckForSoonToExpireRecordsEx() when the caller doesn't care about matching a cache
// record's type or class.
#define kCheckForSoonToExpireAnyType  -1
#define kCheckForSoonToExpireAnyClass -1

mDNSlocal mDNSs32 CheckForSoonToExpireRecordsEx(mDNS *const m, const domainname *const name, const mDNSu32 namehash,
    const int rrtype, const int rrclass)
{
    const mDNSs32 threshold = m->timenow + mDNSPlatformOneSecond;  // See if there are any records expiring within one second
    const mDNSs32 start     = m->timenow - 0x10000000;
    mDNSs32 delay = start;
    const CacheGroup *const cg = CacheGroupForName(m, namehash, name);
    for (const CacheRecord *cr = cg ? cg->members : mDNSNULL; cr; cr = cr->next)
    {
        const ResourceRecord *const rr = &cr->resrec;
        const mDNSBool typeMatch = (rrtype < 0) || (rrtype == kDNSQType_ANY) || (rr->rrtype == rrtype);
        const mDNSBool classMatch = (rrclass < 0) || (rr->rrclass == rrclass);
        if (typeMatch && classMatch)
        {
            // If we have a record that is about to expire within a second, then delay until after it's been deleted.
            const mDNSs32 expireTime = RRExpireTime(cr);
            if (((threshold - expireTime) >= 0) && ((expireTime - delay) > 0))
            {
                delay = expireTime;
            }
        }
    }
    if (delay - start > 0)
        return(NonZeroTime(delay));
    else
        return(0);
}

mDNSlocal mDNSs32 CheckForSoonToExpireRecords(mDNS *const m, const domainname *const name, const mDNSu32 namehash)
{
    return CheckForSoonToExpireRecordsEx(m, name, namehash, kCheckForSoonToExpireAnyType, kCheckForSoonToExpireAnyClass);
}

// CacheRecordAdd is only called from CreateNewCacheEntry, *never* directly as a result of a client API call.
// If new questions are created as a result of invoking client callbacks, they will be added to
// the end of the question list, and m->NewQuestions will be set to indicate the first new question.
// rr is a new CacheRecord just received into our cache
// (kDNSRecordTypePacketAns/PacketAnsUnique/PacketAdd/PacketAddUnique).
// Note: CacheRecordAdd calls AnswerCurrentQuestionWithResourceRecord which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void CacheRecordAdd(mDNS *const m, CacheRecord *cr)
{
    DNSQuestion *q;

    // We stop when we get to NewQuestions -- if we increment their CurrentAnswers/LargeAnswers/UniqueAnswers
    // counters here we'll end up double-incrementing them when we do it again in AnswerNewQuestion().
    for (q = m->Questions; q && q != m->NewQuestions; q=q->next)
    {
        if (CacheRecordAnswersQuestion(cr, q))
        {
            // If this question is one that's actively sending queries, and it's received ten answers within one
            // second of sending the last query packet, then that indicates some radical network topology change,
            // so reset its exponential backoff back to the start. We must be at least at the eight-second interval
            // to do this. If we're at the four-second interval, or less, there's not much benefit accelerating
            // because we will anyway send another query within a few seconds. The first reset query is sent out
            // randomized over the next four seconds to reduce possible synchronization between machines.
            if (q->LastAnswerPktNum != m->PktNum)
            {
                q->LastAnswerPktNum = m->PktNum;
                if (mDNSOpaque16IsZero(q->TargetQID) && ActiveQuestion(q) && ++q->RecentAnswerPkts >= 10 &&
                    q->ThisQInterval > InitialQuestionInterval * QuestionIntervalStep3 && m->timenow - q->LastQTxTime < mDNSPlatformOneSecond)
                {
                    LogMsg("CacheRecordAdd: %##s (%s) got immediate answer burst (%d); restarting exponential backoff sequence (%d)",
                           q->qname.c, DNSTypeName(q->qtype), q->RecentAnswerPkts, q->ThisQInterval);
                    q->LastQTime      = m->timenow - InitialQuestionInterval + (mDNSs32)mDNSRandom((mDNSu32)mDNSPlatformOneSecond*4);
                    q->ThisQInterval  = InitialQuestionInterval;
                    SetNextQueryTime(m,q);
                }
            }
            verbosedebugf("CacheRecordAdd %p %##s (%s) %lu %#a:%d question %p", cr, cr->resrec.name->c,
                          DNSTypeName(cr->resrec.rrtype), cr->resrec.rroriginalttl, cr->resrec.rDNSServer ?
                          &cr->resrec.rDNSServer->addr : mDNSNULL, mDNSVal16(cr->resrec.rDNSServer ?
                                                                             cr->resrec.rDNSServer->port : zeroIPPort), q);
            q->CurrentAnswers++;

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            q->unansweredQueries = 0;
#endif
            if (cr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers++;
            if (cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers++;
            if (q->CurrentAnswers > 4000)
            {
                static int msgcount = 0;
                if (msgcount++ < 10)
                    LogMsg("CacheRecordAdd: %##s (%s) has %d answers; shedding records to resist DOS attack",
                           q->qname.c, DNSTypeName(q->qtype), q->CurrentAnswers);
                cr->resrec.rroriginalttl = 0;
                cr->UnansweredQueries = MaxUnansweredQueries;
            }
        }
    }

    if (!cr->DelayDelivery)
    {
        if (m->CurrentQuestion)
            LogMsg("CacheRecordAdd ERROR m->CurrentQuestion already set: %##s (%s)", m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
        m->CurrentQuestion = m->Questions;
        while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
        {
            q = m->CurrentQuestion;
            if (CacheRecordAnswersQuestion(cr, q))
                AnswerCurrentQuestionWithResourceRecord(m, cr, QC_add);
            if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
                m->CurrentQuestion = q->next;
        }
        m->CurrentQuestion = mDNSNULL;
    }

    SetNextCacheCheckTimeForRecord(m, cr);
}

// NoCacheAnswer is only called from mDNSCoreReceiveResponse, *never* directly as a result of a client API call.
// If new questions are created as a result of invoking client callbacks, they will be added to
// the end of the question list, and m->NewQuestions will be set to indicate the first new question.
// rr is a new CacheRecord just received from the wire (kDNSRecordTypePacketAns/AnsUnique/Add/AddUnique)
// but we don't have any place to cache it. We'll deliver question 'add' events now, but we won't have any
// way to deliver 'remove' events in future, nor will we be able to include this in known-answer lists,
// so we immediately bump ThisQInterval up to MaxQuestionInterval to avoid pounding the network.
// Note: NoCacheAnswer calls AnswerCurrentQuestionWithResourceRecord which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void NoCacheAnswer(mDNS *const m, CacheRecord *cr)
{
    LogMsg("No cache space: Delivering non-cached result for %##s", m->rec.r.resrec.name->c);
    if (m->CurrentQuestion)
        LogMsg("NoCacheAnswer ERROR m->CurrentQuestion already set: %##s (%s)", m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;
    // We do this for *all* questions, not stopping when we get to m->NewQuestions,
    // since we're not caching the record and we'll get no opportunity to do this later
    while (m->CurrentQuestion)
    {
        DNSQuestion *q = m->CurrentQuestion;
        if (CacheRecordAnswersQuestion(cr, q))
            AnswerCurrentQuestionWithResourceRecord(m, cr, QC_addnocache);  // QC_addnocache means "don't expect remove events for this"
        if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
            m->CurrentQuestion = q->next;
    }
    m->CurrentQuestion = mDNSNULL;
}

// CacheRecordRmv is only called from CheckCacheExpiration, which is called from mDNS_Execute.
// Note that CacheRecordRmv is *only* called for records that are referenced by at least one active question.
// If new questions are created as a result of invoking client callbacks, they will be added to
// the end of the question list, and m->NewQuestions will be set to indicate the first new question.
// cr is an existing cache CacheRecord that just expired and is being deleted
// (kDNSRecordTypePacketAns/PacketAnsUnique/PacketAdd/PacketAddUnique).
// Note: CacheRecordRmv calls AnswerCurrentQuestionWithResourceRecord which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void CacheRecordRmv(mDNS *const m, CacheRecord *cr)
{
    if (m->CurrentQuestion)
        LogMsg("CacheRecordRmv ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;

    // We stop when we get to NewQuestions -- for new questions their CurrentAnswers/LargeAnswers/UniqueAnswers counters
    // will all still be zero because we haven't yet gone through the cache counting how many answers we have for them.
    while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
    {
        DNSQuestion *q = m->CurrentQuestion;
        // When a question enters suppressed state, we generate RMV events and generate a negative
        // response. A cache may be present that answers this question e.g., cache entry generated
        // before the question became suppressed. We need to skip the suppressed questions here as
        // the RMV event has already been generated.
        if (!q->Suppressed && CacheRecordAnswersQuestion(cr, q))
        {
            verbosedebugf("CacheRecordRmv %p %s", cr, CRDisplayString(m, cr));
            q->FlappingInterface1 = mDNSNULL;
            q->FlappingInterface2 = mDNSNULL;

            if (q->CurrentAnswers == 0)
            {
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                LogMsg("CacheRecordRmv ERROR!!: How can CurrentAnswers already be zero for %p %##s (%s) DNSServer %#a:%d",
                       q, q->qname.c, DNSTypeName(q->qtype), q->qDNSServer ? &q->qDNSServer->addr : mDNSNULL,
                       mDNSVal16(q->qDNSServer ? q->qDNSServer->port : zeroIPPort));
#endif
            }
            else
            {
                q->CurrentAnswers--;
                if (cr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers--;
                if (cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers--;
            }

            // If we have dropped below the answer threshold for this mDNS question,
            // restart the queries at InitialQuestionInterval.
            if (mDNSOpaque16IsZero(q->TargetQID) && (q->BrowseThreshold > 0) && (q->CurrentAnswers < q->BrowseThreshold))
            {
                q->ThisQInterval = InitialQuestionInterval;
                q->LastQTime     = m->timenow - q->ThisQInterval;
                SetNextQueryTime(m,q);
                LogInfo("CacheRecordRmv: (%s) %##s dropped below threshold of %d answers",
                    DNSTypeName(q->qtype), q->qname.c, q->BrowseThreshold);
            }

            // Never generate "remove" events for negative results of the DNSSEC-unaware record.
            mDNSBool generateRemoveEvents = (cr->resrec.rdata->MaxRDLength > 0);
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            // However, generate "remove" events for negative results of the record that is used for DNSSEC validation:
            if (!generateRemoveEvents)
            {
                // 1. The to-be-validated negative record can contain denial of existence records that are being
                // tracked by the DNSSEC callback.
                if (resource_record_is_to_be_dnssec_validated(&cr->resrec))
                {
                    generateRemoveEvents = mDNStrue;
                }
                // 2. The DNSSEC secure negative record is currently being tracked by the DNSSEC callback.
                else if (resource_record_is_dnssec_validated(&cr->resrec))
                {
                    generateRemoveEvents = (resource_record_get_validation_result(&cr->resrec) == dnssec_secure);
                }

                if (generateRemoveEvents)
                {
                    LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_INFO, "[Q%u] Delivering RMV event for the negative record - "
                        "rr type: " PUB_DNS_TYPE ", validated: " PUB_BOOL, mDNSVal16(q->TargetQID),
                        DNS_TYPE_PARAM(cr->resrec.rrtype), BOOL_PARAM(resource_record_is_dnssec_validated(&cr->resrec)));
                }
            }
        #endif

            if (generateRemoveEvents)
            {
                if ((q->CurrentAnswers == 0) && mDNSOpaque16IsZero(q->TargetQID))
                {
                    LogInfo("CacheRecordRmv: Last answer for %##s (%s) expired from cache; will reconfirm antecedents",
                            q->qname.c, DNSTypeName(q->qtype));
                    ReconfirmAntecedents(m, &q->qname, q->qnamehash, cr->resrec.InterfaceID, 0);
                }
                AnswerCurrentQuestionWithResourceRecord(m, cr, QC_rmv);
            }
        }
        if (m->CurrentQuestion == q)    // If m->CurrentQuestion was not auto-advanced, do it ourselves now
            m->CurrentQuestion = q->next;
    }
    m->CurrentQuestion = mDNSNULL;
}

mDNSlocal void ReleaseCacheEntity(mDNS *const m, CacheEntity *e)
{
#if MDNS_MALLOC_DEBUGGING >= 1
    unsigned int i;
    for (i=0; i<sizeof(*e); i++) ((char*)e)[i] = 0xFF;
#endif
    e->next = m->rrcache_free;
    m->rrcache_free = e;
    m->rrcache_totalused--;
}

mDNSlocal void ReleaseCacheGroup(mDNS *const m, CacheGroup **cp)
{
    CacheEntity *e = (CacheEntity *)(*cp);
    //LogMsg("ReleaseCacheGroup:  Releasing CacheGroup for %p, %##s", (*cp)->name->c, (*cp)->name->c);
    if ((*cp)->rrcache_tail != &(*cp)->members)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, "ERROR: (*cp)->members == mDNSNULL but (*cp)->rrcache_tail != &(*cp)->members)");
    }
    //if ((*cp)->name != (domainname*)((*cp)->namestorage))
    //  LogMsg("ReleaseCacheGroup: %##s, %p %p", (*cp)->name->c, (*cp)->name, (domainname*)((*cp)->namestorage));
    if ((*cp)->name != (domainname*)((*cp)->namestorage)) mDNSPlatformMemFree((*cp)->name);
    (*cp)->name = mDNSNULL;
    *cp = (*cp)->next;          // Cut record from list
    ReleaseCacheEntity(m, e);
}

mDNSlocal void ReleaseAdditionalCacheRecords(mDNS *const m, CacheRecord **rp)
{
    while (*rp)
    {
        CacheRecord *rr = *rp;
        *rp = (*rp)->next;          // Cut record from list
        if (rr->resrec.rdata && rr->resrec.rdata != (RData*)&rr->smallrdatastorage)
        {
            mDNSPlatformMemFree(rr->resrec.rdata);
            rr->resrec.rdata = mDNSNULL;
        }
        // NSEC or SOA records that are not added to the CacheGroup do not share the name
        // of the CacheGroup.
        if (rr->resrec.name)
        {
            debugf("ReleaseAdditionalCacheRecords: freeing cached record %##s (%s)", rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype));
            mDNSPlatformMemFree((void *)rr->resrec.name);
            rr->resrec.name = mDNSNULL;
        }
        // Don't count the NSEC3 records used by anonymous browse/reg
        if (!rr->resrec.InterfaceID)
        {
            m->rrcache_totalused_unicast -= rr->resrec.rdlength;
        }
        ReleaseCacheEntity(m, (CacheEntity *)rr);
    }
}

mDNSexport void ReleaseCacheRecord(mDNS *const m, CacheRecord *r)
{
    CacheGroup *cg;

    //LogMsg("ReleaseCacheRecord: Releasing %s", CRDisplayString(m, r));
    if (r->resrec.rdata && r->resrec.rdata != (RData*)&r->smallrdatastorage) mDNSPlatformMemFree(r->resrec.rdata);
    r->resrec.rdata = mDNSNULL;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_forget(&r->resrec.metadata);
#endif

    cg = CacheGroupForRecord(m, &r->resrec);

    if (!cg)
    {
        // It is okay to have this printed for NSEC/NSEC3s
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "ReleaseCacheRecord: ERROR!! cg NULL for " PRI_DM_NAME " (" PUB_S ")", DM_NAME_PARAM(r->resrec.name),
            DNSTypeName(r->resrec.rrtype));
    }
    // When NSEC records are not added to the cache, it is usually cached at the "nsec" list
    // of the CacheRecord. But sometimes they may be freed without adding to the "nsec" list
    // (which is handled below) and in that case it should be freed here.
    if (r->resrec.name && cg && r->resrec.name != cg->name)
    {
        debugf("ReleaseCacheRecord: freeing %##s (%s)", r->resrec.name->c, DNSTypeName(r->resrec.rrtype));
        mDNSPlatformMemFree((void *)r->resrec.name);
    }
    r->resrec.name = mDNSNULL;

    if (!r->resrec.InterfaceID)
    {
        m->rrcache_totalused_unicast -= r->resrec.rdlength;
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    MDNS_DISPOSE_DNSSEC_OBJ(r->resrec.dnssec);
#endif

    ReleaseAdditionalCacheRecords(m, &r->soa);

    ReleaseCacheEntity(m, (CacheEntity *)r);
}

// Note: We want to be careful that we deliver all the CacheRecordRmv calls before delivering
// CacheRecordDeferredAdd calls. The in-order nature of the cache lists ensures that all
// callbacks for old records are delivered before callbacks for newer records.
mDNSlocal void CheckCacheExpiration(mDNS *const m, const mDNSu32 slot, CacheGroup *const cg)
{
    CacheRecord **rp = &cg->members;

    if (m->lock_rrcache) { LogMsg("CheckCacheExpiration ERROR! Cache already locked!"); return; }
    m->lock_rrcache = 1;

    while (*rp)
    {
        CacheRecord *const rr = *rp;
        mDNSBool recordReleased = mDNSfalse;
        mDNSs32 event = RRExpireTime(rr);
        if (m->timenow - event >= 0)    // If expired, delete it
        {
            if (rr->CRActiveQuestion)   // If this record has one or more active questions, tell them it's going away
            {
                DNSQuestion *q = rr->CRActiveQuestion;
                verbosedebugf("CheckCacheExpiration: Removing%7d %7d %p %s",
                              m->timenow - rr->TimeRcvd, rr->resrec.rroriginalttl, rr->CRActiveQuestion, CRDisplayString(m, rr));
                // When a cache record is about to expire, we expect to do four queries at 80-82%, 85-87%, 90-92% and
                // then 95-97% of the TTL. If the DNS server does not respond, then we will remove the cache entry
                // before we pick a new DNS server. As the question interval is set to MaxQuestionInterval, we may
                // not send out a query anytime soon. Hence, we need to reset the question interval. If this is
                // a normal deferred ADD case, then AnswerCurrentQuestionWithResourceRecord will reset it to
                // MaxQuestionInterval. If we have inactive questions referring to negative cache entries,
                // don't ressurect them as they will deliver duplicate "No such Record" ADD events
                if (((mDNSOpaque16IsZero(q->TargetQID) && (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask)) ||
                     (!mDNSOpaque16IsZero(q->TargetQID) && !q->LongLived)) && ActiveQuestion(q))
                {
                    q->ThisQInterval = InitialQuestionInterval;
                    q->LastQTime     = m->timenow - q->ThisQInterval;
                    SetNextQueryTime(m, q);
                }
                CacheRecordRmv(m, rr);
                m->rrcache_active--;
            }

            event += MAX_GHOST_TIME;                                                    // Adjust so we can check for a ghost expiration
            if (rr->resrec.mortality == Mortality_Mortal ||                             // Normal expired mortal record that needs released
                rr->resrec.rroriginalttl == 0            ||                             // Non-mortal record that is set to be purged
                (rr->resrec.mortality == Mortality_Ghost && m->timenow - event >= 0))   // A ghost record that expired more than MAX_GHOST_TIME ago
            {   //  Release as normal
                *rp = rr->next;                                     // Cut it from the list before ReleaseCacheRecord
                verbosedebugf("CheckCacheExpiration: Deleting (%s)%7d %7d %p %s",
                              MortalityDisplayString(rr->resrec.mortality),
                              m->timenow - rr->TimeRcvd, rr->resrec.rroriginalttl, rr->CRActiveQuestion, CRDisplayString(m, rr));
                ReleaseCacheRecord(m, rr);
                recordReleased = mDNStrue;
            }
            else                                                    // An immortal record needs to become a ghost when it expires
            {   // Don't release this entry
                if (rr->resrec.mortality == Mortality_Immortal)
                {
                    rr->resrec.mortality = Mortality_Ghost;         // Expired immortal records become ghosts
                    verbosedebugf("CheckCacheExpiration: NOT Deleting (%s)%7d %7d %p %s",
                                  MortalityDisplayString(rr->resrec.mortality),
                                  m->timenow - rr->TimeRcvd, rr->resrec.rroriginalttl, rr->CRActiveQuestion, CRDisplayString(m, rr));
                    if (rr->DelayDelivery)
                    {
                        rr->DelayDelivery = 0;
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "CheckCacheExpiration: Resetting DelayDelivery for new ghost");
                    }
                }
            }
        }
        else                                                        // else, not expired; see if we need to query
        {
            // If waiting to delay delivery, do nothing until then
            if (rr->DelayDelivery && rr->DelayDelivery - m->timenow > 0)
                event = rr->DelayDelivery;
            else
            {
                if (rr->DelayDelivery) CacheRecordDeferredAdd(m, rr);
                if (rr->CRActiveQuestion && rr->UnansweredQueries < MaxUnansweredQueries)
                {
                    if (m->timenow - rr->NextRequiredQuery < 0)     // If not yet time for next query
                        event = NextCacheCheckEvent(rr);            // then just record when we want the next query
                    else                                            // else trigger our question to go out now
                    {
                        // Set NextScheduledQuery to timenow so that SendQueries() will run.
                        // SendQueries() will see that we have records close to expiration, and send FEQs for them.
                        m->NextScheduledQuery = m->timenow;
                        // After sending the query we'll increment UnansweredQueries and call SetNextCacheCheckTimeForRecord(),
                        // which will correctly update m->NextCacheCheck for us.
                        event = m->timenow + FutureTime;
                    }
                }
            }
        }

        if (!recordReleased)  //  Schedule if we did not release the record
        {
            verbosedebugf("CheckCacheExpiration:%6d %5d %s",
                          (event - m->timenow) / mDNSPlatformOneSecond, CacheCheckGracePeriod(rr), CRDisplayString(m, rr));
            if (m->rrcache_nextcheck[slot] - event > 0)
                m->rrcache_nextcheck[slot] = event;
            rp = &rr->next;
        }
    }
    if (cg->rrcache_tail != rp) verbosedebugf("CheckCacheExpiration: Updating CacheGroup tail from %p to %p", cg->rrcache_tail, rp);
    cg->rrcache_tail = rp;
    m->lock_rrcache = 0;
}

// "LORecord" includes both LocalOnly and P2P record. This function assumes m->CurrentQuestion is pointing to "q".
//
// If "CheckOnly" is set to "true", the question won't be answered but just check to see if there is an answer and
// returns true if there is an answer.
//
// If "CheckOnly" is set to "false", the question will be answered if there is a LocalOnly/P2P record and
// returns true to indicate the same.
mDNSlocal mDNSBool AnswerQuestionWithLORecord(mDNS *const m, DNSQuestion *q, mDNSBool checkOnly)
{
    AuthRecord *lr;
    AuthGroup *ag;

    if (m->CurrentRecord)
        LogMsg("AnswerQuestionWithLORecord ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));

    ag = AuthGroupForName(&m->rrauth, q->qnamehash, &q->qname);
    if (ag)
    {
        m->CurrentRecord = ag->members;
        while (m->CurrentRecord && m->CurrentRecord != ag->NewLocalOnlyRecords)
        {
            AuthRecord *rr = m->CurrentRecord;
            m->CurrentRecord = rr->next;
            //
            // If the question is mDNSInterface_LocalOnly, all records local to the machine should be used
            // to answer the query. This is handled in AnswerNewLocalOnlyQuestion.
            //
            // We handle mDNSInterface_Any and scoped questions here. See LocalOnlyRecordAnswersQuestion for more
            // details on how we handle this case. For P2P we just handle "Interface_Any" questions. For LocalOnly
            // we handle both mDNSInterface_Any and scoped questions.

            if (rr->ARType == AuthRecordLocalOnly || (rr->ARType == AuthRecordP2P && (q->InterfaceID == mDNSInterface_Any || q->InterfaceID == mDNSInterface_BLE)))
                if (LocalOnlyRecordAnswersQuestion(rr, q))
                {
                    if (checkOnly)
                    {
                        LogInfo("AnswerQuestionWithLORecord: question %##s (%s) answered by %s", q->qname.c, DNSTypeName(q->qtype),
                            ARDisplayString(m, rr));
                        m->CurrentRecord = mDNSNULL;
                        return mDNStrue;
                    }
                    AnswerLocalQuestionWithLocalAuthRecord(m, rr, QC_add);
                    if (m->CurrentQuestion != q)
                        break;     // If callback deleted q, then we're finished here
                }
        }
    }
    m->CurrentRecord = mDNSNULL;

    if (m->CurrentQuestion != q)
    {
        LogInfo("AnswerQuestionWithLORecord: Question deleted while while answering LocalOnly record answers");
        return mDNStrue;
    }

    if (q->LOAddressAnswers)
    {
        LogInfo("AnswerQuestionWithLORecord: Question %p %##s (%s) answered using local auth records LOAddressAnswers %d",
                q, q->qname.c, DNSTypeName(q->qtype), q->LOAddressAnswers);
        return mDNStrue;
    }

    // Before we go check the cache and ship this query on the wire, we have to be sure that there are
    // no local records that could possibly answer this question. As we did not check the NewLocalRecords, we
    // need to just peek at them to see whether it will answer this question. If it would answer, pretend
    // that we answered. AnswerAllLocalQuestionsWithLocalAuthRecord will answer shortly. This happens normally
    // when we add new /etc/hosts entries and restart the question. It is a new question and also a new record.
    if (ag)
    {
        lr = ag->NewLocalOnlyRecords;
        while (lr)
        {
            if (UniqueLocalOnlyRecord(lr) && LocalOnlyRecordAnswersQuestion(lr, q))
            {
                LogInfo("AnswerQuestionWithLORecord: Question %p %##s (%s) will be answered using new local auth records "
                        " LOAddressAnswers %d", q, q->qname.c, DNSTypeName(q->qtype), q->LOAddressAnswers);
                return mDNStrue;
            }
            lr = lr->next;
        }
    }
    return mDNSfalse;
}

// Today, we suppress questions (not send them on the wire) for several reasons e.g.,
// AAAA query is suppressed because no IPv6 capability or PID is not allowed to make
// DNS requests.
mDNSlocal void AnswerSuppressedQuestion(mDNS *const m, DNSQuestion *q)
{
    // If the client did not set the kDNSServiceFlagsReturnIntermediates flag, then don't generate a negative response,
    // just deactivate the DNSQuestion.
    if (q->ReturnIntermed)
    {
        GenerateNegativeResponse(m, mDNSInterface_Any, QC_suppressed);
    }
    else
    {
        q->ThisQInterval = 0;
    }
}

mDNSlocal void AnswerNewQuestion(mDNS *const m)
{
    mDNSBool ShouldQueryImmediately = mDNStrue;
    DNSQuestion *const q = m->NewQuestions;     // Grab the question we're going to answer
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
    if (!mDNSOpaque16IsZero(q->TargetQID)) DNS64HandleNewQuestion(m, q);
#endif
    CacheGroup *const cg = CacheGroupForName(m, q->qnamehash, &q->qname);

    verbosedebugf("AnswerNewQuestion: Answering %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

    if (cg) CheckCacheExpiration(m, HashSlotFromNameHash(q->qnamehash), cg);
    if (m->NewQuestions != q) { LogInfo("AnswerNewQuestion: Question deleted while doing CheckCacheExpiration"); goto exit; }
    m->NewQuestions = q->next;
    // Advance NewQuestions to the next *after* calling CheckCacheExpiration, because if we advance it first
    // then CheckCacheExpiration may give this question add/remove callbacks, and it's not yet ready for that.
    //
    // Also, CheckCacheExpiration() calls CacheRecordDeferredAdd() and CacheRecordRmv(), which invoke
    // client callbacks, which may delete their own or any other question. Our mechanism for detecting
    // whether our current m->NewQuestions question got deleted by one of these callbacks is to store the
    // value of m->NewQuestions in 'q' before calling CheckCacheExpiration(), and then verify afterwards
    // that they're still the same. If m->NewQuestions has changed (because mDNS_StopQuery_internal
    // advanced it), that means the question was deleted, so we no longer need to worry about answering
    // it (and indeed 'q' is now a dangling pointer, so dereferencing it at all would be bad, and the
    // values we computed for slot and cg are now stale and relate to a question that no longer exists).
    //
    // We can't use the usual m->CurrentQuestion mechanism for this because  CacheRecordDeferredAdd() and
    // CacheRecordRmv() both use that themselves when walking the list of (non-new) questions generating callbacks.
    // Fortunately mDNS_StopQuery_internal auto-advances both m->CurrentQuestion *AND* m->NewQuestions when
    // deleting a question, so luckily we have an easy alternative way of detecting if our question got deleted.

    if (m->lock_rrcache) LogMsg("AnswerNewQuestion ERROR! Cache already locked!");
    // This should be safe, because calling the client's question callback may cause the
    // question list to be modified, but should not ever cause the rrcache list to be modified.
    // If the client's question callback deletes the question, then m->CurrentQuestion will
    // be advanced, and we'll exit out of the loop
    m->lock_rrcache = 1;
    if (m->CurrentQuestion) {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->Q%d] AnswerNewQuestion ERROR m->CurrentQuestion already set: " PRI_DM_NAME " (" PUB_S ")",
            m->CurrentQuestion->request_id, mDNSVal16(m->CurrentQuestion->TargetQID),
            DM_NAME_PARAM(&m->CurrentQuestion->qname), DNSTypeName(m->CurrentQuestion->qtype));
    }

    m->CurrentQuestion = q;     // Indicate which question we're answering, so we'll know if it gets deleted
    if (m->CurrentQuestion != q)
    {
        LogInfo("AnswerNewQuestion: Question deleted while generating NoAnswer_Fail response");
        goto exit;
    }

    // See if we want to tell it about LocalOnly/P2P records. If we answered them using LocalOnly
    // or P2P record, then we are done.
    if (AnswerQuestionWithLORecord(m, q, mDNSfalse))
        goto exit;

    // Use expired records for non-mDNS DNSQuestions with the mDNSExpiredRecordPolicy_UseCached policy.
    const mDNSExpiredRecordPolicy policy = q->ExpRecordPolicy;
    const mDNSBool useExpiredRecords = !mDNSOpaque16IsZero(q->TargetQID) && (policy == mDNSExpiredRecordPolicy_UseCached);
    if (!q->Suppressed || q->ForceCNAMEFollows)
    {
        CacheRecord *cr;
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        mDNSu32 answersToValidateCount = 0;         // Count the number of "to be validated" answers for a primary DNSSEC requestor.
        mDNSBool wildcardCNameAnswer = mDNSfalse;   // Set to true if the returned "to be validated" answers contains a wildcard matched CNAME.
        mDNSBool negativeProvesWildcard = mDNSfalse;// Set to true if a negative record that proves wildcard answer exists has been returned.
        mDNSu32 positiveRRSetSize = 0;              // The positive RRSet size (excluding the denial of existence record).
        #endif
        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
        {
            if (SameNameCacheRecordAnswersQuestion(cr, q))
            {
                // SecsSinceRcvd is whole number of elapsed seconds, rounded down
                mDNSu32 SecsSinceRcvd = ((mDNSu32)(m->timenow - cr->TimeRcvd)) / mDNSPlatformOneSecond;
                mDNSBool IsExpired = (cr->resrec.rroriginalttl <= SecsSinceRcvd);
                if (IsExpired)
                {
                    if (!useExpiredRecords || (cr->resrec.RecordType == kDNSRecordTypePacketNegative))
                    {
                        continue;
                    }
                }
                // If this record set is unexpired and marked unique, then that means we can reasonably assume we have
                // the whole set -- we don't need to rush out on the network and query immediately to see if there are
                // more answers out there.
                if ((cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) || (q->ExpectUnique))
                {
                    if (!IsExpired)
                    {
                        ShouldQueryImmediately = mDNSfalse;
                    }
                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    // However, for DNSSEC we need to take extra steps to check if a query is necessary.
                    if (dns_question_is_primary_dnssec_requestor(q) && resource_record_is_to_be_dnssec_validated(&cr->resrec))
                    {
                        answersToValidateCount++;
                        if (resource_record_is_positive(&cr->resrec))
                        {
                            if (resource_record_as_rrsig_covers_rr_type(&cr->resrec, kDNSType_CNAME) &&
                                resource_record_as_rrsig_covers_wildcard_rr(&cr->resrec))
                            {
                                // If the RRSIG covers a CNAME, and this RRSIG covers a wildcard resource record,
                                // then the record (the CNAME) is a wildcard match.
                                wildcardCNameAnswer = mDNStrue;
                            }
                            if (positiveRRSetSize == 0)
                            {
                                // Initialized once, to get the positive RRSet size.
                                positiveRRSetSize = (mDNSu32)dnssec_obj_resource_record_member_get_rrset_size(cr->resrec.dnssec);
                            }
                        }
                        else // Negative record
                        {
                            negativeProvesWildcard = resource_record_as_denial_of_existence_proves_wildcard_answer(&cr->resrec);
                        }
                    }
                #endif
                }
                q->CurrentAnswers++;
                if (cr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers++;
                if (cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers++;
#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
                cr->LastCachedAnswerTime = m->timenow;
                dnssd_analytics_update_cache_request(mDNSOpaque16IsZero(q->TargetQID) ? CacheRequestType_multicast : CacheRequestType_unicast, CacheState_hit);
#endif
                AnswerCurrentQuestionWithResourceRecord(m, cr, QC_add);
                if (m->CurrentQuestion != q) break;     // If callback deleted q, then we're finished here
            }
            else if (mDNSOpaque16IsZero(q->TargetQID) && RRTypeIsAddressType(cr->resrec.rrtype) && RRTypeIsAddressType(q->qtype))
            {
                ShouldQueryImmediately = mDNSfalse;
            }
        }
    #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        if (m->CurrentQuestion == q && dns_question_is_primary_dnssec_requestor(q) && !ShouldQueryImmediately)
        {
            // If we have decided to hold the query, check if we still need to send the query out because of an
            // incomplete answer set.
            // If we have a wildcard CName answer, then we must have a denial of existence record that proves the
            // existence of this CName. Therefore, we can only be sure that we have a complete set if and only if:
            // answersToValidateCount == positiveRRSetSize + 1, where 1 is the denial of existence record. (which means the
            // corresponding nagative record has expired from cache).
            // Or if we only returned a denial of existence record that proves a wildcard answer (which means the
            // corresponding positive RRSet has expired from cache)
            // In either of two cases above, send out query anyway to get the full RRSets.
            const mDNSBool onlyPositive = (wildcardCNameAnswer && (answersToValidateCount != positiveRRSetSize + 1));
            const mDNSBool onlyNegative = (negativeProvesWildcard && (answersToValidateCount == 1));
            if (onlyPositive || onlyNegative)
            {
                ShouldQueryImmediately = mDNStrue;
                LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_DEFAULT,
                    "[Q%u] Continue sending out query for the primary DNSSEC question due to incomplete answer set - "
                    "only positive: " PUB_BOOL ", only negative: " PUB_BOOL, mDNSVal16(q->TargetQID),
                    BOOL_PARAM(onlyPositive), BOOL_PARAM(onlyNegative));
            }
        }
    #endif
    }
    if (m->CurrentQuestion == q)
    {
        mDNSBool questionStopped = mDNSfalse;
        if (useExpiredRecords)
        {
            if (q->EventHandler)
            {
                mDNS_DropLockBeforeCallback();
                q->EventHandler(q, mDNSQuestionEvent_NoMoreExpiredRecords);
                mDNS_ReclaimLockAfterCallback();
                questionStopped = (m->CurrentQuestion != q);
            }
            if (!questionStopped)
            {
                q->ExpRecordPolicy = mDNSExpiredRecordPolicy_Immortalize;
            }
        }
        if (!questionStopped && q->Suppressed)
        {
            AnswerSuppressedQuestion(m, q);
        }
    }
    // We don't use LogInfo for this "Question deleted" message because it happens so routinely that
    // it's not remotely remarkable, and therefore unlikely to be of much help tracking down bugs.
    if (m->CurrentQuestion != q) { debugf("AnswerNewQuestion: Question deleted while giving cache answers"); goto exit; }

#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
    dnssd_analytics_update_cache_request(mDNSOpaque16IsZero(q->TargetQID) ? CacheRequestType_multicast : CacheRequestType_unicast, CacheState_miss);
#endif
    q->InitialCacheMiss = mDNStrue; // Initial cache check is done, so mark as a miss from now on

    // Note: When a query gets suppressed or retried with search domains, we de-activate the question.
    // Hence we don't execute the following block of code for those cases.
    if (ShouldQueryImmediately && ActiveQuestion(q))
    {
        debugf("[R%d->Q%d] AnswerNewQuestion: ShouldQueryImmediately %##s (%s)", q->request_id, mDNSVal16(q->TargetQID), q->qname.c, DNSTypeName(q->qtype));
        q->ThisQInterval  = InitialQuestionInterval;
        q->LastQTime      = m->timenow - q->ThisQInterval;
        if (mDNSOpaque16IsZero(q->TargetQID))       // For mDNS, spread packets to avoid a burst of simultaneous queries
        {
            // Compute random delay in the range 1-6 seconds, then divide by 50 to get 20-120ms
            if (!m->RandomQueryDelay)
                m->RandomQueryDelay = (mDNSPlatformOneSecond + mDNSRandom(mDNSPlatformOneSecond*5) - 1) / 50 + 1;
            q->LastQTime += m->RandomQueryDelay;
        }
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
    DPCHandleNewQuestion(q);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
    if (ActiveQuestion(q) && dns_question_uses_dns_polling(q))
    {
        q->LongLived = mDNStrue;
        q->state = LLQ_Poll;
        q->ThisQInterval = LLQ_POLL_INTERVAL;
        // No matter whether the answer is cached or nor, DNS polling always follows the same query schedule.
        q->LastQTime = m->timenow - q->ThisQInterval + 1;
    }
#endif

    // IN ALL CASES make sure that m->NextScheduledQuery is set appropriately.
    // In cases where m->NewQuestions->DelayAnswering is set, we may have delayed generating our
    // answers for this question until *after* its scheduled transmission time, in which case
    // m->NextScheduledQuery may now be set to 'never', and in that case -- even though we're *not* doing
    // ShouldQueryImmediately -- we still need to make sure we set m->NextScheduledQuery correctly.
    SetNextQueryTime(m,q);

exit:
    m->CurrentQuestion = mDNSNULL;
    m->lock_rrcache = 0;
}

// When a NewLocalOnlyQuestion is created, AnswerNewLocalOnlyQuestion runs though our ResourceRecords delivering any
// appropriate answers, stopping if it reaches a NewLocalOnlyRecord -- these will be handled by AnswerAllLocalQuestionsWithLocalAuthRecord
mDNSlocal void AnswerNewLocalOnlyQuestion(mDNS *const m)
{
    AuthGroup *ag;
    DNSQuestion *q = m->NewLocalOnlyQuestions;      // Grab the question we're going to answer
    mDNSBool retEv = mDNSfalse;
    m->NewLocalOnlyQuestions = q->next;             // Advance NewLocalOnlyQuestions to the next (if any)

    debugf("AnswerNewLocalOnlyQuestion: Answering %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

    if (m->CurrentQuestion)
        LogMsg("AnswerNewLocalOnlyQuestion ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = q;     // Indicate which question we're answering, so we'll know if it gets deleted

    if (m->CurrentRecord)
        LogMsg("AnswerNewLocalOnlyQuestion ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));

    // 1. First walk the LocalOnly records answering the LocalOnly question
    // 2. As LocalOnly questions should also be answered by any other Auth records local to the machine,
    //    walk the ResourceRecords list delivering the answers
    ag = AuthGroupForName(&m->rrauth, q->qnamehash, &q->qname);
    if (ag)
    {
        m->CurrentRecord = ag->members;
        while (m->CurrentRecord && m->CurrentRecord != ag->NewLocalOnlyRecords)
        {
            AuthRecord *rr = m->CurrentRecord;
            m->CurrentRecord = rr->next;
            if (LocalOnlyRecordAnswersQuestion(rr, q))
            {
                retEv = mDNStrue;
                AnswerLocalQuestionWithLocalAuthRecord(m, rr, QC_add);
                if (m->CurrentQuestion != q) break;     // If callback deleted q, then we're finished here
            }
        }
    }

    if (m->CurrentQuestion == q)
    {
        m->CurrentRecord = m->ResourceRecords;

        while (m->CurrentRecord && m->CurrentRecord != m->NewLocalRecords)
        {
            AuthRecord *ar = m->CurrentRecord;
            m->CurrentRecord = ar->next;
            if (AuthRecordAnswersQuestion(ar, q))
            {
                retEv = mDNStrue;
                AnswerLocalQuestionWithLocalAuthRecord(m, ar, QC_add);
                if (m->CurrentQuestion != q) break;     // If callback deleted q, then we're finished here
            }
        }
    }

    // The local host is the authoritative source for LocalOnly questions
    // so if no records exist and client requested intermediates, then generate a negative response
    if (!retEv && (m->CurrentQuestion == q) && q->ReturnIntermed)
        GenerateNegativeResponse(m, mDNSInterface_LocalOnly, QC_forceresponse);

    m->CurrentQuestion = mDNSNULL;
    m->CurrentRecord   = mDNSNULL;
}

mDNSlocal CacheEntity *GetCacheEntity(mDNS *const m, const CacheGroup *const PreserveCG)
{
    CacheEntity *e = mDNSNULL;

    if (m->lock_rrcache) { LogMsg("GetFreeCacheRR ERROR! Cache already locked!"); return(mDNSNULL); }
    m->lock_rrcache = 1;

    // If we have no free records, ask the client layer to give us some more memory
    if (!m->rrcache_free && m->MainCallback)
    {
        if (m->rrcache_totalused != m->rrcache_size)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "GetFreeCacheRR: count mismatch: m->rrcache_totalused %u != m->rrcache_size %u",
                m->rrcache_totalused, m->rrcache_size);
        }

        // We don't want to be vulnerable to a malicious attacker flooding us with an infinite
        // number of bogus records so that we keep growing our cache until the machine runs out of memory.
        // To guard against this, if our cache grows above 512kB (approx 3168 records at 164 bytes each),
        // and we're actively using less than 1/32 of that cache, then we purge all the unused records
        // and recycle them, instead of allocating more memory.
        if (m->rrcache_size > 5000 && m->rrcache_size / 32 > m->rrcache_active)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "Possible denial-of-service attack in progress: m->rrcache_size %u; m->rrcache_active %u",
                m->rrcache_size, m->rrcache_active);
        }
        else
        {
            mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
            m->MainCallback(m, mStatus_GrowCache);
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
        }
    }

    // If we still have no free records, recycle all the records we can.
    // Enumerating the entire cache is moderately expensive, so when we do it, we reclaim all the records we can in one pass.
    if (!m->rrcache_free)
    {
        mDNSu32 oldtotalused = m->rrcache_totalused;
        mDNSu32 slot;
        for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
        {
            CacheGroup **cp = &m->rrcache_hash[slot];
            while (*cp)
            {
                CacheRecord **rp = &(*cp)->members;
                while (*rp)
                {
                    // Cases where we do not want to recycle cache entry:
                    // 1. Records that answer still-active questions are not candidates for recycling.
                    // 2. Records that are currently linked into the CacheFlushRecords list may not be recycled, or
                    //    we'll crash.
                    // 3. Records that are newly added to the cache but whose callback delivery gets delayed should not
                    //    be recycled, because it will end up with an incomplete RRSet.
                    mDNSBool doNotRecycle = ((*rp)->CRActiveQuestion || (*rp)->NextInCFList
                        || ((*rp)->DelayDelivery != 0));
                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    // 4. Records that are currently saved to a temporary array to calculate the response set size may
                    //    not be recycled, or we'll crash.
                    doNotRecycle = doNotRecycle || ((*rp)->ineligibleForRecycling);
                #endif

                    if (doNotRecycle)
                    {
                        rp=&(*rp)->next;
                    }
                    else
                    {
                        CacheRecord *rr = *rp;
                        *rp = (*rp)->next;          // Cut record from list
                        ReleaseCacheRecord(m, rr);
                    }
                }
                if ((*cp)->rrcache_tail != rp)
                    verbosedebugf("GetFreeCacheRR: Updating rrcache_tail[%lu] from %p to %p", slot, (*cp)->rrcache_tail, rp);
                (*cp)->rrcache_tail = rp;
                if ((*cp)->members || (*cp)==PreserveCG) cp=&(*cp)->next;
                else ReleaseCacheGroup(m, cp);
            }
        }
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "GetCacheEntity recycled %d records to reduce cache from %d to %d",
            oldtotalused - m->rrcache_totalused, oldtotalused, m->rrcache_totalused);
    }

    if (m->rrcache_free)    // If there are records in the free list, take one
    {
        e = m->rrcache_free;
        m->rrcache_free = e->next;
        if (++m->rrcache_totalused >= m->rrcache_report)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "RR Cache now using %u objects", m->rrcache_totalused);
            if      (m->rrcache_report <  100) m->rrcache_report += 10;
            else if (m->rrcache_report < 1000) m->rrcache_report += 100;
            else m->rrcache_report += 1000;
        }
        mDNSPlatformMemZero(e, sizeof(*e));
    }

    m->lock_rrcache = 0;

    return(e);
}

mDNSlocal CacheRecord *GetCacheRecord(mDNS *const m, CacheGroup *cg, mDNSu16 RDLength)
{
    CacheRecord *r = (CacheRecord *)GetCacheEntity(m, cg);
    if (r)
    {
        r->resrec.rdata = (RData*)&r->smallrdatastorage;    // By default, assume we're usually going to be using local storage
        if (RDLength > InlineCacheRDSize)           // If RDLength is too big, allocate extra storage
        {
            r->resrec.rdata = (RData*) mDNSPlatformMemAllocateClear(sizeofRDataHeader + RDLength);
            if (r->resrec.rdata) r->resrec.rdata->MaxRDLength = r->resrec.rdlength = RDLength;
            else { ReleaseCacheEntity(m, (CacheEntity*)r); r = mDNSNULL; }
        }
    }
    return(r);
}

mDNSlocal CacheGroup *GetCacheGroup(mDNS *const m, const mDNSu32 slot, const ResourceRecord *const rr)
{
    mDNSu16 namelen = DomainNameLength(rr->name);
    CacheGroup *cg = (CacheGroup*)GetCacheEntity(m, mDNSNULL);
    if (!cg) { LogMsg("GetCacheGroup: Failed to allocate memory for %##s", rr->name->c); return(mDNSNULL); }
    cg->next         = m->rrcache_hash[slot];
    cg->namehash     = rr->namehash;
    cg->members      = mDNSNULL;
    cg->rrcache_tail = &cg->members;
    if (namelen > sizeof(cg->namestorage))
        cg->name = (domainname *) mDNSPlatformMemAllocate(namelen);
    else
        cg->name = (domainname*)cg->namestorage;
    if (!cg->name)
    {
        LogMsg("GetCacheGroup: Failed to allocate name storage for %##s", rr->name->c);
        ReleaseCacheEntity(m, (CacheEntity*)cg);
        return(mDNSNULL);
    }
    AssignDomainName(cg->name, rr->name);

    if (CacheGroupForRecord(m, rr)) LogMsg("GetCacheGroup: Already have CacheGroup for %##s", rr->name->c);
    m->rrcache_hash[slot] = cg;
    if (CacheGroupForRecord(m, rr) != cg) LogMsg("GetCacheGroup: Not finding CacheGroup for %##s", rr->name->c);

    return(cg);
}

mDNSexport void mDNS_PurgeCacheResourceRecord(mDNS *const m, CacheRecord *rr)
{
    mDNS_CheckLock(m);

    // Make sure we mark this record as thoroughly expired -- we don't ever want to give
    // a positive answer using an expired record (e.g. from an interface that has gone away).
    // We don't want to clear CRActiveQuestion here, because that would leave the record subject to
    // summary deletion without giving the proper callback to any questions that are monitoring it.
    // By setting UnansweredQueries to MaxUnansweredQueries we ensure it won't trigger any further expiration queries.
    rr->TimeRcvd          = m->timenow - mDNSPlatformOneSecond * 60;
    rr->UnansweredQueries = MaxUnansweredQueries;
    rr->resrec.rroriginalttl     = 0;
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) || MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
    rr->DNSPushSubscribed = mDNSfalse;
#endif
    SetNextCacheCheckTimeForRecord(m, rr);
}

mDNSexport mDNSs32 mDNS_TimeNow(const mDNS *const m)
{
    mDNSs32 time;
    mDNSPlatformLock(m);
    if (m->mDNS_busy)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
            "Lock failure: mDNS_TimeNow called while holding mDNS lock. This is incorrect. Code protected by lock should just use m->timenow.");
        if (!m->timenow)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
                "Lock failure: mDNS_TimeNow: m->mDNS_busy is %u but m->timenow not set", m->mDNS_busy);
        }
    }

    if (m->timenow) time = m->timenow;
    else time = mDNS_TimeNow_NoLock(m);
    mDNSPlatformUnlock(m);
    return(time);
}

// To avoid pointless CPU thrash, we use SetSPSProxyListChanged(X) to record the last interface that
// had its Sleep Proxy client list change, and defer to actual BPF reconfiguration to mDNS_Execute().
// (GetNextScheduledEvent() returns "now" when m->SPSProxyListChanged is set)
#define SetSPSProxyListChanged(X) do { \
        if (m->SPSProxyListChanged && m->SPSProxyListChanged != (X)) mDNSPlatformUpdateProxyList(m->SPSProxyListChanged); \
        m->SPSProxyListChanged = (X); } while(0)

// Called from mDNS_Execute() to expire stale proxy records
mDNSlocal void CheckProxyRecords(mDNS *const m, AuthRecord *list)
{
    m->CurrentRecord = list;
    while (m->CurrentRecord)
    {
        AuthRecord *rr = m->CurrentRecord;
        if (rr->resrec.RecordType != kDNSRecordTypeDeregistering && rr->WakeUp.HMAC.l[0])
        {
            // If m->SPSSocket is NULL that means we're not acting as a sleep proxy any more,
            // so we need to cease proxying for *all* records we may have, expired or not.
            if (m->SPSSocket && m->timenow - rr->TimeExpire < 0)    // If proxy record not expired yet, update m->NextScheduledSPS
            {
                if (m->NextScheduledSPS - rr->TimeExpire > 0)
                    m->NextScheduledSPS = rr->TimeExpire;
            }
            else                                                    // else proxy record expired, so remove it
            {
                LogSPS("CheckProxyRecords: Removing %d H-MAC %.6a I-MAC %.6a %d %s",
                       m->ProxyRecords, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, rr->WakeUp.seq, ARDisplayString(m, rr));
                SetSPSProxyListChanged(rr->resrec.InterfaceID);
                mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
                // Don't touch rr after this -- memory may have been free'd
            }
        }
        // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
        // new records could have been added to the end of the list as a result of that call.
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

mDNSlocal void CheckRmvEventsForLocalRecords(mDNS *const m)
{
    while (m->CurrentRecord)
    {
        AuthRecord *rr = m->CurrentRecord;
        if (rr->AnsweredLocalQ && rr->resrec.RecordType == kDNSRecordTypeDeregistering)
        {
            debugf("CheckRmvEventsForLocalRecords: Generating local RMV events for %s", ARDisplayString(m, rr));
            rr->resrec.RecordType = kDNSRecordTypeShared;
            AnswerAllLocalQuestionsWithLocalAuthRecord(m, rr, QC_rmv);
            if (m->CurrentRecord == rr) // If rr still exists in list, restore its state now
            {
                rr->resrec.RecordType = kDNSRecordTypeDeregistering;
                rr->AnsweredLocalQ = mDNSfalse;
                // SendResponses normally calls CompleteDeregistration after sending goodbyes.
                // For LocalOnly records, we don't do that and hence we need to do that here.
                if (RRLocalOnly(rr)) CompleteDeregistration(m, rr);
            }
        }
        if (m->CurrentRecord == rr)     // If m->CurrentRecord was not auto-advanced, do it ourselves now
            m->CurrentRecord = rr->next;
    }
}

mDNSlocal void TimeoutQuestions_internal(mDNS *const m, DNSQuestion* questions, mDNSInterfaceID InterfaceID)
{
    if (m->CurrentQuestion)
        LogMsg("TimeoutQuestions ERROR m->CurrentQuestion already set: %##s (%s)", m->CurrentQuestion->qname.c,
               DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = questions;
    while (m->CurrentQuestion)
    {
        DNSQuestion *const q = m->CurrentQuestion;
        if (q->StopTime)
        {
            if (!q->TimeoutQuestion)
                LogMsg("TimeoutQuestions: ERROR!! TimeoutQuestion not set, but StopTime set for %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

            if (m->timenow - q->StopTime >= 0)
            {
                LogInfo("TimeoutQuestions: question %p %##s timed out, time %d", q, q->qname.c, m->timenow - q->StopTime);
                q->LOAddressAnswers = 0; // unset since timing out the question
                GenerateNegativeResponse(m, InterfaceID, QC_forceresponse);
                if (m->CurrentQuestion == q) q->StopTime = 0;
            }
            else
            {
                if (m->NextScheduledStopTime - q->StopTime > 0)
                    m->NextScheduledStopTime = q->StopTime;
            }
        }
        // If m->CurrentQuestion wasn't modified out from under us, advance it now
        // We can't do this at the start of the loop because GenerateNegativeResponse
        // depends on having m->CurrentQuestion point to the right question
        if (m->CurrentQuestion == q)
            m->CurrentQuestion = q->next;
    }
    m->CurrentQuestion = mDNSNULL;
}

mDNSlocal void TimeoutQuestions(mDNS *const m)
{
    m->NextScheduledStopTime = m->timenow + FutureTime; // push reschedule of TimeoutQuestions to way off into the future
    TimeoutQuestions_internal(m, m->Questions, mDNSInterface_Any);
    TimeoutQuestions_internal(m, m->LocalOnlyQuestions, mDNSInterface_LocalOnly);
}

mDNSlocal void mDNSCoreFreeProxyRR(mDNS *const m)
{
    AuthRecord *rrPtr = m->SPSRRSet, *rrNext = mDNSNULL;
    LogSPS("%s : Freeing stored sleep proxy A/AAAA records", __func__);
    while (rrPtr)
    {
        rrNext = rrPtr->next;
        mDNSPlatformMemFree(rrPtr);
        rrPtr  = rrNext;
    }
    m->SPSRRSet = mDNSNULL;
}

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)

mDNSlocal void mDNSPostResponseDelayMetrics(const mdns_multicast_delay_histogram_t histogram)
{
    const bool posted = analytics_send_event_lazy("com.apple.mDNSResponder.mDNSResponseDelayEvent",
    ^{
        return mdns_multicast_delay_histogram_create_metrics_dictionary(histogram);
    });
    if (!posted)
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_WARNING,
            "com.apple.mDNSResponder.mDNSResponseDelayEvent: Analytic not posted");
    }
}

mDNSlocal void mDNSGenerateResponseDelayReport(mDNS *const m)
{
    mDNSBool analyticPosted = mDNSfalse;
    for (const NetworkInterfaceInfo *intf = GetFirstActiveInterface(m->HostInterfaces); intf;
         intf = GetFirstActiveInterface(intf->next))
    {
        const mdns_multicast_delay_histogram_t delay_histogram = intf->delayHistogram;
        if (!delay_histogram)
        {
            continue;
        }
        if (mdns_multicast_delay_histogram_has_records(delay_histogram))
        {
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "mDNS response delay distribution - interface name: "
                PUB_S ", interface index: %u, report: %@" , intf->ifname,
                mDNSPlatformInterfaceIndexfromInterfaceID(m, intf->InterfaceID, mDNStrue), delay_histogram);
            if (!analyticPosted)
            {
                // Every time when we submit analytics, we only report the first available histogram.
                // The assumption here is that we will only have one infrastructure type interface at any given time,
                // which is Wi-Fi. In the case where we have more than one infrastructure interfaces, which seems
                // to be impossible, we always choose the first one to report.
                mDNSPostResponseDelayMetrics(delay_histogram);
                analyticPosted = mDNStrue;
            }
        }
        mdns_multicast_delay_histogram_reset_records(delay_histogram);
    }
}

#endif

mDNSexport mDNSs32 mDNS_Execute(mDNS *const m)
{
    mDNS_Lock(m);   // Must grab lock before trying to read m->timenow

    if (m->timenow - m->NextScheduledEvent >= 0)
    {
        int i;
        AuthRecord *head, *tail;
        mDNSu32 slot;
        AuthGroup *ag;

        verbosedebugf("mDNS_Execute");

        if (m->CurrentQuestion)
            LogMsg("mDNS_Execute: ERROR m->CurrentQuestion already set: %##s (%s)",
                   m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));

        if (m->CurrentRecord)
            LogMsg("mDNS_Execute: ERROR m->CurrentRecord already set: %s", ARDisplayString(m, m->CurrentRecord));

        // 1. If we're past the probe suppression time, we can clear it
        if (m->SuppressProbes && m->timenow - m->SuppressProbes >= 0) m->SuppressProbes = 0;

        // 2. If it's been more than ten seconds since the last probe failure, we can clear the counter
        if (m->NumFailedProbes && m->timenow - m->ProbeFailTime >= mDNSPlatformOneSecond * 10) m->NumFailedProbes = 0;

        // 3. Purge our cache of stale old records
        if (m->rrcache_size && m->timenow - m->NextCacheCheck >= 0)
        {
            mDNSu32 numchecked = 0;
            m->NextCacheCheck = m->timenow + FutureTime;
            for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
            {
                if (m->timenow - m->rrcache_nextcheck[slot] >= 0)
                {
                    CacheGroup **cp = &m->rrcache_hash[slot];
                    m->rrcache_nextcheck[slot] = m->timenow + FutureTime;
                    while (*cp)
                    {
                        debugf("m->NextCacheCheck %4d Slot %3d %##s", numchecked, slot, *cp ? (*cp)->name : (domainname*)"\x04NULL");
                        numchecked++;
                        CheckCacheExpiration(m, slot, *cp);
                        if ((*cp)->members) cp=&(*cp)->next;
                        else ReleaseCacheGroup(m, cp);
                    }
                }
                // Even if we didn't need to actually check this slot yet, still need to
                // factor its nextcheck time into our overall NextCacheCheck value
                if (m->NextCacheCheck - m->rrcache_nextcheck[slot] > 0)
                    m->NextCacheCheck = m->rrcache_nextcheck[slot];
            }
            debugf("m->NextCacheCheck %4d checked, next in %d", numchecked, m->NextCacheCheck - m->timenow);
        }

        if (m->timenow - m->NextScheduledSPS >= 0)
        {
            m->NextScheduledSPS = m->timenow + FutureTime;
            CheckProxyRecords(m, m->DuplicateRecords);  // Clear m->DuplicateRecords first, then m->ResourceRecords
            CheckProxyRecords(m, m->ResourceRecords);
        }

        SetSPSProxyListChanged(mDNSNULL);       // Perform any deferred BPF reconfiguration now

        // Check to see if we need to send any keepalives. Do this after we called CheckProxyRecords above
        // as records could have expired during that check
        if (m->timenow - m->NextScheduledKA >= 0)
        {
            m->NextScheduledKA = m->timenow + FutureTime;
            mDNS_SendKeepalives(m);
        }

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
        if (m->NextBonjourDisableTime && (m->timenow - m->NextBonjourDisableTime >= 0))
        {
            // Schedule immediate network change processing to leave the multicast group
            // since the delay time has expired since the previous active registration or query.
            m->NetworkChanged = m->timenow;
            m->NextBonjourDisableTime = 0;
            m->BonjourEnabled = 0;

            LogInfo("mDNS_Execute: Scheduled network changed processing to leave multicast group.");
        }
#endif

        // mDNS_SetUpDomainEnumeration() will check if the domain enumeration operation can be stopped.
        for (DomainEnumerationOp *op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
        {
            for (mDNSu32 type = 0; type < mDNS_DomainTypeMaxCount; type++)
            {
                mDNS_SetUpDomainEnumeration(m, op, type);
            }
        }

    #if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
        ResolverDiscovery_PerformPeriodicTasks();
    #endif

    #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
        // Clear AnnounceOwner if necessary. (Do this *before* SendQueries() and SendResponses().)
        if (m->AnnounceOwner && m->timenow - m->AnnounceOwner >= 0)
        {
            m->AnnounceOwner = 0;
        }
    #endif

        if (m->DelaySleep && m->timenow - m->DelaySleep >= 0)
        {
            m->DelaySleep = 0;
            if (m->SleepState == SleepState_Transferring)
            {
                LogSPS("Re-sleep delay passed; now checking for Sleep Proxy Servers");
                BeginSleepProcessing(m);
            }
        }

    #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        // Check if there is any newly DNSSEC-validated record to be added to the cache.
        if (m->NextUpdateDNSSECValidatedCache && m->timenow - m->NextUpdateDNSSECValidatedCache >= 0)
        {
            m->NextUpdateDNSSECValidatedCache = 0;
            dnssec_update_cache_for_validated_records(m);
        }
    #endif

    #if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
        const mDNSs32 NextMDNSResponseDelayReport = m->NextMDNSResponseDelayReport;
        if (NextMDNSResponseDelayReport && ((m->timenow - NextMDNSResponseDelayReport) >= 0))
        {
            mDNSGenerateResponseDelayReport(m);
            m->NextMDNSResponseDelayReport = (m->timenow + RuntimeMDNSMetricsReportInterval);
        }
    #endif

        // 4. See if we can answer any of our new local questions from the cache
        for (i=0; m->NewQuestions && i<1000; i++)
        {
            if (m->NewQuestions->DelayAnswering && m->timenow - m->NewQuestions->DelayAnswering < 0) break;
            AnswerNewQuestion(m);
        }
        if (i >= 1000) LogMsg("mDNS_Execute: AnswerNewQuestion exceeded loop limit");

        // Make sure we deliver *all* local RMV events, and clear the corresponding rr->AnsweredLocalQ flags, *before*
        // we begin generating *any* new ADD events in the m->NewLocalOnlyQuestions and m->NewLocalRecords loops below.
        for (i=0; i<1000 && m->LocalRemoveEvents; i++)
        {
            m->LocalRemoveEvents = mDNSfalse;
            m->CurrentRecord = m->ResourceRecords;
            CheckRmvEventsForLocalRecords(m);
            // Walk the LocalOnly records and deliver the RMV events
            for (slot = 0; slot < AUTH_HASH_SLOTS; slot++)
                for (ag = m->rrauth.rrauth_hash[slot]; ag; ag = ag->next)
                {
                    m->CurrentRecord = ag->members;
                    if (m->CurrentRecord) CheckRmvEventsForLocalRecords(m);
                }
        }

        if (i >= 1000) LogMsg("mDNS_Execute: m->LocalRemoveEvents exceeded loop limit");

        for (i=0; m->NewLocalOnlyQuestions && i<1000; i++) AnswerNewLocalOnlyQuestion(m);
        if (i >= 1000) LogMsg("mDNS_Execute: AnswerNewLocalOnlyQuestion exceeded loop limit");

        head = tail = mDNSNULL;
        for (i=0; i<1000 && m->NewLocalRecords && m->NewLocalRecords != head; i++)
        {
            AuthRecord *rr = m->NewLocalRecords;
            m->NewLocalRecords = m->NewLocalRecords->next;
            if (LocalRecordReady(rr))
            {
                debugf("mDNS_Execute: Delivering Add event with LocalAuthRecord %s", ARDisplayString(m, rr));
                AnswerAllLocalQuestionsWithLocalAuthRecord(m, rr, QC_add);
            }
            else if (!rr->next)
            {
                // If we have just one record that is not ready, we don't have to unlink and
                // reinsert. As the NewLocalRecords will be NULL for this case, the loop will
                // terminate and set the NewLocalRecords to rr.
                debugf("mDNS_Execute: Just one LocalAuthRecord %s, breaking out of the loop early", ARDisplayString(m, rr));
                if (head != mDNSNULL || m->NewLocalRecords != mDNSNULL)
                    LogMsg("mDNS_Execute: ERROR!!: head %p, NewLocalRecords %p", head, m->NewLocalRecords);

                head = rr;
            }
            else
            {
                AuthRecord **p = &m->ResourceRecords;   // Find this record in our list of active records
                debugf("mDNS_Execute: Skipping LocalAuthRecord %s", ARDisplayString(m, rr));
                // if this is the first record we are skipping, move to the end of the list.
                // if we have already skipped records before, append it at the end.
                while (*p && *p != rr) p=&(*p)->next;
                if (*p) *p = rr->next;                  // Cut this record from the list
                else { LogMsg("mDNS_Execute: ERROR!! Cannot find record %s in ResourceRecords list", ARDisplayString(m, rr)); break; }
                if (!head)
                {
                    while (*p) p=&(*p)->next;
                    *p = rr;
                    head = tail = rr;
                }
                else
                {
                    tail->next = rr;
                    tail = rr;
                }
                rr->next = mDNSNULL;
            }
        }
        m->NewLocalRecords = head;
        debugf("mDNS_Execute: Setting NewLocalRecords to %s", (head ? ARDisplayString(m, head) : "NULL"));

        if (i >= 1000) LogMsg("mDNS_Execute: m->NewLocalRecords exceeded loop limit");

        // Check to see if we have any new LocalOnly/P2P records to examine for delivering
        // to our local questions
        if (m->NewLocalOnlyRecords)
        {
            m->NewLocalOnlyRecords = mDNSfalse;
            for (slot = 0; slot < AUTH_HASH_SLOTS; slot++)
            {
                for (ag = m->rrauth.rrauth_hash[slot]; ag; ag = ag->next)
                {
                    for (i=0; i<100 && ag->NewLocalOnlyRecords; i++)
                    {
                        AuthRecord *rr = ag->NewLocalOnlyRecords;
                        ag->NewLocalOnlyRecords = ag->NewLocalOnlyRecords->next;
                        // LocalOnly records should always be ready as they never probe
                        if (LocalRecordReady(rr))
                        {
                            debugf("mDNS_Execute: Delivering Add event with LocalAuthRecord %s", ARDisplayString(m, rr));
                            AnswerAllLocalQuestionsWithLocalAuthRecord(m, rr, QC_add);
                        }
                        else LogMsg("mDNS_Execute: LocalOnlyRecord %s not ready", ARDisplayString(m, rr));
                    }
                    // We limit about 100 per AuthGroup that can be serviced at a time
                    if (i >= 100) LogMsg("mDNS_Execute: ag->NewLocalOnlyRecords exceeded loop limit");
                }
            }
        }

        // 5. See what packets we need to send
        if (m->mDNSPlatformStatus != mStatus_NoError || (m->SleepState == SleepState_Sleeping))
            DiscardDeregistrations(m);
        if (m->mDNSPlatformStatus == mStatus_NoError)
        {
            // If the platform code is ready, and we're not suppressing packet generation right now
            // then send our responses, probes, and questions.
            // We check the cache first, because there might be records close to expiring that trigger questions to refresh them.
            // We send queries next, because there might be final-stage probes that complete their probing here, causing
            // them to advance to announcing state, and we want those to be included in any announcements we send out.
            // Finally, we send responses, including the previously mentioned records that just completed probing.
            if (!m->SuppressQueries || ((m->timenow - m->SuppressQueries) >= 0))
            {
                m->SuppressQueries = 0;

                // 6. Send Query packets. This may cause some probing records to advance to announcing state
                if (m->timenow - m->NextScheduledQuery >= 0 || m->timenow - m->NextScheduledProbe >= 0) SendQueries(m);
                if (m->timenow - m->NextScheduledQuery >= 0)
                {
                    DNSQuestion *q;
                    LogMsg("mDNS_Execute: SendQueries didn't send all its queries (%d - %d = %d) will try again in one second",
                           m->timenow, m->NextScheduledQuery, m->timenow - m->NextScheduledQuery);
                    m->NextScheduledQuery = m->timenow + mDNSPlatformOneSecond;
                    for (q = m->Questions; q && q != m->NewQuestions; q=q->next)
                        if (ActiveQuestion(q) && m->timenow - NextQSendTime(q) >= 0)
                            LogMsg("mDNS_Execute: SendQueries didn't send %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
                }
                if (m->timenow - m->NextScheduledProbe >= 0)
                {
                    LogMsg("mDNS_Execute: SendQueries didn't send all its probes (%d - %d = %d) will try again in one second",
                           m->timenow, m->NextScheduledProbe, m->timenow - m->NextScheduledProbe);
                    m->NextScheduledProbe = m->timenow + mDNSPlatformOneSecond;
                }
            }
            if (!m->SuppressResponses || ((m->timenow - m->SuppressResponses) >= 0))
            {
                m->SuppressResponses = 0;

                // 7. Send Response packets, including probing records just advanced to announcing state
                if (m->timenow - m->NextScheduledResponse >= 0) SendResponses(m);
                if (m->timenow - m->NextScheduledResponse >= 0)
                {
                    LogMsg("mDNS_Execute: SendResponses didn't send all its responses; will try again in one second");
                    m->NextScheduledResponse = m->timenow + mDNSPlatformOneSecond;
                }
            }
        }

        // Clear RandomDelay values, ready to pick a new different value next time
        m->RandomQueryDelay     = 0;
        m->RandomReconfirmDelay = 0;

        // See if any questions (or local-only questions) have timed out
        if (m->NextScheduledStopTime && m->timenow - m->NextScheduledStopTime >= 0) TimeoutQuestions(m);
#ifndef UNICAST_DISABLED
        if (m->NextSRVUpdate && m->timenow - m->NextSRVUpdate >= 0) UpdateAllSRVRecords(m);
        if (m->timenow - m->NextScheduledNATOp >= 0) CheckNATMappings(m);
        if (m->timenow - m->NextuDNSEvent >= 0) uDNS_Tasks(m);
#endif
    }

    // Note about multi-threaded systems:
    // On a multi-threaded system, some other thread could run right after the mDNS_Unlock(),
    // performing mDNS API operations that change our next scheduled event time.
    //
    // On multi-threaded systems (like the current Windows implementation) that have a single main thread
    // calling mDNS_Execute() (and other threads allowed to call mDNS API routines) it is the responsibility
    // of the mDNSPlatformUnlock() routine to signal some kind of stateful condition variable that will
    // signal whatever blocking primitive the main thread is using, so that it will wake up and execute one
    // more iteration of its loop, and immediately call mDNS_Execute() again. The signal has to be stateful
    // in the sense that if the main thread has not yet entered its blocking primitive, then as soon as it
    // does, the state of the signal will be noticed, causing the blocking primitive to return immediately
    // without blocking. This avoids the race condition between the signal from the other thread arriving
    // just *before* or just *after* the main thread enters the blocking primitive.
    //
    // On multi-threaded systems (like the current Mac OS 9 implementation) that are entirely timer-driven,
    // with no main mDNS_Execute() thread, it is the responsibility of the mDNSPlatformUnlock() routine to
    // set the timer according to the m->NextScheduledEvent value, and then when the timer fires, the timer
    // callback function should call mDNS_Execute() (and ignore the return value, which may already be stale
    // by the time it gets to the timer callback function).

    mDNS_Unlock(m);     // Calling mDNS_Unlock is what gives m->NextScheduledEvent its new value
    return(m->NextScheduledEvent);
}

#ifndef UNICAST_DISABLED
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
mDNSlocal void SuspendLLQs(mDNS *m)
{
    DNSQuestion *q;
    for (q = m->Questions; q; q = q->next)
        if (ActiveQuestion(q) && !mDNSOpaque16IsZero(q->TargetQID) && q->LongLived && q->state == LLQ_Established)
        { q->ReqLease = 0; sendLLQRefresh(m, q); }
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
#endif // UNICAST_DISABLED

mDNSlocal mDNSBool QuestionHasLocalAnswers(mDNS *const m, DNSQuestion *q)
{
    AuthRecord *rr;
    AuthGroup *ag;

    ag = AuthGroupForName(&m->rrauth, q->qnamehash, &q->qname);
    if (ag)
    {
        for (rr = ag->members; rr; rr=rr->next)
            // Filter the /etc/hosts records - LocalOnly, Unique, A/AAAA/CNAME
            if (UniqueLocalOnlyRecord(rr) && LocalOnlyRecordAnswersQuestion(rr, q))
            {
                LogInfo("QuestionHasLocalAnswers: Question %p %##s (%s) has local answer %s", q, q->qname.c, DNSTypeName(q->qtype), ARDisplayString(m, rr));
                return mDNStrue;
            }
    }
    return mDNSfalse;
}

// ActivateUnicastQuery() is called from three places:
// 1. When a new question is created
// 2. On wake from sleep
// 3. When the DNS configuration changes
// In case 1 we don't want to mess with our established ThisQInterval and LastQTime (ScheduleImmediately is false)
// In cases 2 and 3 we do want to cause the question to be resent immediately (ScheduleImmediately is true)
mDNSlocal void ActivateUnicastQuery(mDNS *const m, DNSQuestion *const question, mDNSBool ScheduleImmediately)
{
    if (!question->DuplicateOf)
    {
        debugf("ActivateUnicastQuery: %##s %s%s",
               question->qname.c, DNSTypeName(question->qtype), ScheduleImmediately ? " ScheduleImmediately" : "");
        question->CNAMEReferrals = 0;
        if (question->nta) { CancelGetZoneData(m, question->nta); question->nta = mDNSNULL; }
        if (question->LongLived)
        {
            question->state = LLQ_Init;
            question->id = zeroOpaque64;
            question->servPort = zeroIPPort;
            if (question->tcp) { DisposeTCPConn(question->tcp); question->tcp = mDNSNULL; }
        }
        // If the question has local answers, then we don't want answers from outside
        if (ScheduleImmediately && !QuestionHasLocalAnswers(m, question))
        {
            question->ThisQInterval = InitialQuestionInterval;
            question->LastQTime     = m->timenow - question->ThisQInterval;
            SetNextQueryTime(m, question);
        }
    }
}

// Caller should hold the lock
mDNSexport void mDNSCoreRestartAddressQueries(mDNS *const m, mDNSBool SearchDomainsChanged, FlushCache flushCacheRecords,
                                              CallbackBeforeStartQuery BeforeStartCallback, void *context)
{
    DNSQuestion *q;
    DNSQuestion *restart = mDNSNULL;

    mDNS_CheckLock(m);

    // 1. Flush the cache records
    if (flushCacheRecords) flushCacheRecords(m);

    // 2. Even though we may have purged the cache records above, before it can generate RMV event
    // we are going to stop the question. Hence we need to deliver the RMV event before we
    // stop the question.
    //
    // CurrentQuestion is used by RmvEventsForQuestion below. While delivering RMV events, the
    // application callback can potentially stop the current question (detected by CurrentQuestion) or
    // *any* other question which could be the next one that we may process here. RestartQuestion
    // points to the "next" question which will be automatically advanced in mDNS_StopQuery_internal
    // if the "next" question is stopped while the CurrentQuestion is stopped

    if (m->RestartQuestion)
        LogMsg("mDNSCoreRestartAddressQueries: ERROR!! m->RestartQuestion already set: %##s (%s)",
               m->RestartQuestion->qname.c, DNSTypeName(m->RestartQuestion->qtype));

    m->RestartQuestion = m->Questions;
    while (m->RestartQuestion)
    {
        q = m->RestartQuestion;
        m->RestartQuestion = q->next;
        // GetZoneData questions are referenced by other questions (original query that started the GetZoneData
        // question)  through their "nta" pointer. Normally when the original query stops, it stops the
        // GetZoneData question and also frees the memory (See CancelGetZoneData). If we stop the GetZoneData
        // question followed by the original query that refers to this GetZoneData question, we will end up
        // freeing the GetZoneData question and then start the "freed" question at the end.

        if (IsGetZoneDataQuestion(q))
        {
            DNSQuestion *refq = q->next;
            LogInfo("mDNSCoreRestartAddressQueries: Skipping GetZoneDataQuestion %p %##s (%s)", q, q->qname.c, DNSTypeName(q->qtype));
            // debug stuff, we just try to find the referencing question and don't do much with it
            while (refq)
            {
                if (q == &refq->nta->question)
                {
                    LogInfo("mDNSCoreRestartAddressQueries: Question %p %##s (%s) referring to GetZoneDataQuestion %p, not stopping", refq, refq->qname.c, DNSTypeName(refq->qtype), q);
                }
                refq = refq->next;
            }
            continue;
        }

        // This function is called when /etc/hosts changes and that could affect A, AAAA and CNAME queries
        if (q->qtype != kDNSType_A && q->qtype != kDNSType_AAAA && q->qtype != kDNSType_CNAME) continue;

        // If the search domains did not change, then we restart all the queries. Otherwise, only
        // for queries for which we "might" have appended search domains ("might" because we may
        // find results before we apply search domains even though AppendSearchDomains is set to 1)
        if (!SearchDomainsChanged || q->AppendSearchDomains)
        {
            // NOTE: CacheRecordRmvEventsForQuestion will not generate RMV events for queries that have non-zero
            // LOAddressAnswers. Hence it is important that we call CacheRecordRmvEventsForQuestion before
            // LocalRecordRmvEventsForQuestion (which decrements LOAddressAnswers). Let us say that
            // /etc/hosts has an A Record for web.apple.com. Any queries for web.apple.com will be answered locally.
            // But this can't prevent a CNAME/AAAA query to not to be sent on the wire. When it is sent on the wire,
            // it could create cache entries. When we are restarting queries, we can't deliver the cache RMV events
            // for the original query using these cache entries as ADDs were never delivered using these cache
            // entries and hence this order is needed.

            // If the query is suppressed, the RMV events won't be delivered
            if (!CacheRecordRmvEventsForQuestion(m, q)) { LogInfo("mDNSCoreRestartAddressQueries: Question deleted while delivering Cache Record RMV events"); continue; }

            // Suppressed status does not affect questions that are answered using local records
            if (!LocalRecordRmvEventsForQuestion(m, q)) { LogInfo("mDNSCoreRestartAddressQueries: Question deleted while delivering Local Record RMV events"); continue; }

            LogInfo("mDNSCoreRestartAddressQueries: Stop question %p %##s (%s), AppendSearchDomains %d", q,
                    q->qname.c, DNSTypeName(q->qtype), q->AppendSearchDomains);
            mDNS_StopQuery_internal(m, q);
            if (q->ResetHandler) q->ResetHandler(q);
            q->next = restart;
            restart = q;
        }
    }

    // 3. Callback before we start the query
    if (BeforeStartCallback) BeforeStartCallback(m, context);

    // 4. Restart all the stopped queries
    while (restart)
    {
        q = restart;
        restart = restart->next;
        q->next = mDNSNULL;
        LogInfo("mDNSCoreRestartAddressQueries: Start question %p %##s (%s)", q, q->qname.c, DNSTypeName(q->qtype));
        mDNS_StartQuery_internal(m, q);
    }
}

mDNSexport void mDNSCoreRestartQueries(mDNS *const m)
{
    DNSQuestion *q;

#ifndef UNICAST_DISABLED
    // Retrigger all our uDNS questions
    if (m->CurrentQuestion)
        LogMsg("mDNSCoreRestartQueries: ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));
    m->CurrentQuestion = m->Questions;
    while (m->CurrentQuestion)
    {
        q = m->CurrentQuestion;
        m->CurrentQuestion = m->CurrentQuestion->next;
        if (!mDNSOpaque16IsZero(q->TargetQID) && ActiveQuestion(q))
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            mdns_client_forget(&q->client);
#endif
            ActivateUnicastQuery(m, q, mDNStrue);
        }
    }
#endif

    // Retrigger all our mDNS questions
    for (q = m->Questions; q; q=q->next)                // Scan our list of questions
            mDNSCoreRestartQuestion(m, q);
}

// restart question if it's multicast and currently active
mDNSexport void mDNSCoreRestartQuestion(mDNS *const m, DNSQuestion *q)
{
    if (mDNSOpaque16IsZero(q->TargetQID) && ActiveQuestion(q))
    {
        q->ThisQInterval    = InitialQuestionInterval;  // MUST be > zero for an active question
        q->RequestUnicast   = kDefaultRequestUnicastCount;
        q->LastQTime        = m->timenow - q->ThisQInterval;
        q->RecentAnswerPkts = 0;
        ExpireDupSuppressInfo(q->DupSuppress, m->timenow);
        m->NextScheduledQuery = m->timenow;
    }
}

// restart the probe/announce cycle for multicast record
mDNSexport void mDNSCoreRestartRegistration(mDNS *const m, AuthRecord *rr, int announceCount)
{
    if (!AuthRecord_uDNS(rr))
    {
        if (rr->resrec.RecordType == kDNSRecordTypeVerified && !rr->DependentOn) rr->resrec.RecordType = kDNSRecordTypeUnique;
        rr->ProbeCount     = DefaultProbeCountForRecordType(rr->resrec.RecordType);

        if (mDNS_KeepaliveRecord(&rr->resrec))
        {
            rr->AnnounceCount = 0; // Do not announce keepalive records
        }
        else
        {
            // announceCount < 0 indicates default announce count should be used
            if (announceCount < 0)
                announceCount = InitialAnnounceCount;
            if (rr->AnnounceCount < (mDNSu8)announceCount)
                rr->AnnounceCount = (mDNSu8)announceCount;
        }

        rr->SendNSECNow    = mDNSNULL;
        InitializeLastAPTime(m, rr);
    }
}

// ***************************************************************************
// MARK: - Power Management (Sleep/Wake)

mDNSexport void mDNS_UpdateAllowSleep(mDNS *const m)
{
#ifndef IDLESLEEPCONTROL_DISABLED
    mDNSBool allowSleep = mDNStrue;
    char reason[128];

    reason[0] = 0;

    if (m->SystemSleepOnlyIfWakeOnLAN)
    {
        // Don't sleep if we are a proxy for any services
        if (m->ProxyRecords)
        {
            allowSleep = mDNSfalse;
            mDNS_snprintf(reason, sizeof(reason), "sleep proxy for %d records", m->ProxyRecords);
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_UpdateAllowSleep: Sleep disabled because we are proxying %d records", m->ProxyRecords);
        }

        if (allowSleep && mDNSCoreHaveAdvertisedMulticastServices(m))
        {
            // Scan the list of active interfaces
            NetworkInterfaceInfo *intf;
            for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
            {
                if (intf->McastTxRx && !intf->Loopback && !intf->MustNotPreventSleep && !mDNSPlatformInterfaceIsD2D(intf->InterfaceID))
                {
                    // Disallow sleep if this interface doesn't support NetWake
                    if (!intf->NetWake)
                    {
                        allowSleep = mDNSfalse;
                        mDNS_snprintf(reason, sizeof(reason), "%s does not support NetWake", intf->ifname);
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_UpdateAllowSleep: Sleep disabled because " PUB_S " does not support NetWake",
                            intf->ifname);
                        break;
                    }

                    // If the interface can be an in-NIC Proxy, we should check if it can accomodate all the records
                    // that will be offloaded. If not, we should prevent sleep.
                    // This check will be possible once the lower layers provide an API to query the space available for offloads on the NIC.
                    {
                    #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
                        // Disallow sleep if there is no sleep proxy server
                        const CacheRecord *cr = FindSPSInCache1(m, &intf->NetWakeBrowse, mDNSNULL, mDNSNULL);
                        if ( cr == mDNSNULL)
                    #endif
                        {
                            allowSleep = mDNSfalse;
                            mDNS_snprintf(reason, sizeof(reason), "No sleep proxy server on %s", intf->ifname);
                            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_UpdateAllowSleep: Sleep disabled because " PUB_S
                                " has no sleep proxy server", intf->ifname);
                            break;
                        }
                    #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
                        else if (m->SPSType != 0)
                        {
                            mDNSu32 mymetric = LocalSPSMetric(m);
                            mDNSu32 metric   = SPSMetric(cr->resrec.rdata->u.name.c);
                            if (metric >= mymetric)
                            {
                                allowSleep = mDNSfalse;
                                mDNS_snprintf(reason, sizeof(reason), "No sleep proxy server with better metric on %s", intf->ifname);
                                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_UpdateAllowSleep: Sleep disabled because " PUB_S
                                    " has no sleep proxy server with a better metric", intf->ifname);
                                break;
                            }
                        }
                    #endif
                    }
                }
            }
        }
    }

    // Call the platform code to enable/disable sleep
    mDNSPlatformSetAllowSleep(allowSleep, reason);
#else
    (void) m;
#endif /* !defined(IDLESLEEPCONTROL_DISABLED) */
}

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal mDNSBool mDNSUpdateOkToSend(mDNS *const m, AuthRecord *rr, NetworkInterfaceInfo *const intf, mDNSu32 scopeid)
{
    // If it is not a uDNS record, check to see if the updateid is zero. "updateid" is cleared when we have
    // sent the resource record on all the interfaces. If the update id is not zero, check to see if it is time
    // to send.
    if (AuthRecord_uDNS(rr) || (rr->AuthFlags & AuthFlagsWakeOnly) || mDNSOpaque16IsZero(rr->updateid) ||
        m->timenow - (rr->LastAPTime + rr->ThisAPInterval) < 0)
    {
        return mDNSfalse;
    }

    // If we have a pending registration for "scopeid", it is ok to send the update on that interface.
    // If the scopeid is too big to check for validity, we don't check against updateIntID. When
    // we successfully update on all the interfaces (with whatever set in "rr->updateIntID"), we clear
    // updateid and we should have returned from above.
    //
    // Note: scopeid is the same as intf->InterfaceID. It is passed in so that we don't have to call the
    // platform function to extract the value from "intf" every time.

    if ((scopeid >= (sizeof(rr->updateIntID) * mDNSNBBY) || bit_get_opaque64(rr->updateIntID, scopeid)) &&
        (!rr->resrec.InterfaceID || rr->resrec.InterfaceID == intf->InterfaceID))
        return mDNStrue;

    return mDNSfalse;
}
#endif

mDNSexport void UpdateRMAC(mDNS *const m, void *context)
{
    IPAddressMACMapping *addrmap = (IPAddressMACMapping *)context ;
    m->CurrentRecord = m->ResourceRecords;

    if (!addrmap)
    {
        LogMsg("UpdateRMAC: Address mapping is NULL");
        return;
    }

    while (m->CurrentRecord)
    {
        AuthRecord *rr = m->CurrentRecord;
        // If this is a non-sleep proxy keepalive record and the remote IP address matches, update the RData
        if (!rr->WakeUp.HMAC.l[0] && mDNS_KeepaliveRecord(&rr->resrec))
        {
            mDNSAddr raddr;
            getKeepaliveRaddr(m, rr, &raddr);
            if (mDNSSameAddress(&raddr, &addrmap->ipaddr))
            {
                // Update the MAC address only if it is not a zero MAC address
                mDNSEthAddr macAddr;
                mDNSu8 *ptr = GetValueForMACAddr((mDNSu8 *)(addrmap->ethaddr), (mDNSu8 *) (addrmap->ethaddr + sizeof(addrmap->ethaddr)), &macAddr);
                if (ptr != mDNSNULL && !mDNSEthAddressIsZero(macAddr))
                {
                    UpdateKeepaliveRData(m, rr, mDNSNULL, mDNStrue, (char *)(addrmap->ethaddr));
                }
            }
        }
        m->CurrentRecord = rr->next;
    }

    if (addrmap)
        mDNSPlatformMemFree(addrmap);

}

mDNSexport mStatus UpdateKeepaliveRData(mDNS *const m, AuthRecord *rr, NetworkInterfaceInfo *const intf, mDNSBool updateMac, char *ethAddr)
{
    mDNSu16 newrdlength;
    mDNSAddr laddr = zeroAddr;
    mDNSAddr raddr = zeroAddr;
    mDNSEthAddr eth = zeroEthAddr;
    mDNSIPPort lport = zeroIPPort;
    mDNSIPPort rport = zeroIPPort;
    mDNSu32 timeout = 0;
    mDNSu32 seq = 0;
    mDNSu32 ack = 0;
    mDNSu16 win = 0;
    UTF8str255 txt;
    int rdsize;
    RData *newrd;
    mDNSTCPInfo mti;
    mStatus ret;

    // Note: If we fail to update the  DNS NULL  record with additional information in this function, it will be registered
    // with the SPS like any other record. SPS will not send keepalives if it does not have additional information.
    mDNS_ExtractKeepaliveInfo(rr, &timeout, &laddr, &raddr, &eth, &seq, &ack, &lport, &rport, &win);
    if (!timeout || mDNSAddressIsZero(&laddr) || mDNSAddressIsZero(&raddr) || mDNSIPPortIsZero(lport) || mDNSIPPortIsZero(rport))
    {
        LogMsg("UpdateKeepaliveRData: not a valid record %s for keepalive %#a:%d %#a:%d", ARDisplayString(m, rr), &laddr, lport.NotAnInteger, &raddr, rport.NotAnInteger);
        return mStatus_UnknownErr;
    }

    if (updateMac)
    {
        if (laddr.type == mDNSAddrType_IPv4)
            newrdlength = (mDNSu16)mDNS_snprintf((char *)&txt.c[1], sizeof(txt.c) - 1, "t=%d i=%d c=%d h=%#a d=%#a l=%u r=%u m=%s", timeout, kKeepaliveRetryInterval, kKeepaliveRetryCount, &laddr, &raddr, mDNSVal16(lport), mDNSVal16(rport), ethAddr);
        else
            newrdlength = (mDNSu16)mDNS_snprintf((char *)&txt.c[1], sizeof(txt.c) - 1, "t=%d i=%d c=%d H=%#a D=%#a l=%u r=%u m=%s", timeout, kKeepaliveRetryInterval, kKeepaliveRetryCount, &laddr, &raddr,  mDNSVal16(lport), mDNSVal16(rport), ethAddr);

    }
    else
    {
        // If this keepalive packet would be sent on a different interface than the current one that we are processing
        // now, then we don't update the DNS NULL record. But we do not prevent it from registering with the SPS. When SPS sees
        // this DNS NULL record, it does not send any keepalives as it does not have all the information
        mDNSPlatformMemZero(&mti, sizeof (mDNSTCPInfo));
        ret = mDNSPlatformRetrieveTCPInfo(&laddr, &lport, &raddr, &rport, &mti);
        if (ret != mStatus_NoError)
        {
            LogMsg("mDNSPlatformRetrieveTCPInfo: mDNSPlatformRetrieveTCPInfo failed %d", ret);
            return ret;
        }
        if ((intf != mDNSNULL) && (mti.IntfId != intf->InterfaceID))
        {
            LogInfo("mDNSPlatformRetrieveTCPInfo: InterfaceID mismatch mti.IntfId = %p InterfaceID = %p",  mti.IntfId, intf->InterfaceID);
            return mStatus_BadParamErr;
        }

        if (laddr.type == mDNSAddrType_IPv4)
            newrdlength = (mDNSu16)mDNS_snprintf((char *)&txt.c[1], sizeof(txt.c) - 1, "t=%d i=%d c=%d h=%#a d=%#a l=%u r=%u m=%.6a s=%u a=%u w=%u", timeout, kKeepaliveRetryInterval, kKeepaliveRetryCount, &laddr, &raddr, mDNSVal16(lport), mDNSVal16(rport), &eth, mti.seq, mti.ack, mti.window);
        else
            newrdlength = (mDNSu16)mDNS_snprintf((char *)&txt.c[1], sizeof(txt.c) - 1, "t=%d i=%d c=%d H=%#a D=%#a l=%u r=%u m=%.6a s=%u a=%u w=%u", timeout, kKeepaliveRetryInterval, kKeepaliveRetryCount, &laddr, &raddr, mDNSVal16(lport), mDNSVal16(rport), &eth, mti.seq, mti.ack, mti.window);
    }

    // Did we insert a null byte at the end ?
    if (newrdlength == (sizeof(txt.c) - 1))
    {
        LogMsg("UpdateKeepaliveRData: could not allocate memory %s", ARDisplayString(m, rr));
        return mStatus_NoMemoryErr;
    }

    // Include the length for the null byte at the end
    txt.c[0] = (mDNSu8)(newrdlength + 1);
    // Account for the first length byte and the null byte at the end
    newrdlength += 2;

    rdsize = newrdlength > sizeof(RDataBody) ? newrdlength : sizeof(RDataBody);
    newrd = (RData *) mDNSPlatformMemAllocate(sizeof(RData) - sizeof(RDataBody) + rdsize);
    if (!newrd) { LogMsg("UpdateKeepaliveRData: ptr NULL"); return mStatus_NoMemoryErr; }

    newrd->MaxRDLength = (mDNSu16) rdsize;
    mDNSPlatformMemCopy(&newrd->u, txt.c, newrdlength);

    //  If we are updating the record for the first time, rdata points to rdatastorage as the rdata memory
    //  was allocated as part of the AuthRecord itself. We allocate memory when we update the AuthRecord.
    //  If the resource record has data that we allocated in a previous pass (to update MAC address),
    //  free that memory here before copying in the new data.
    if ( rr->resrec.rdata != &rr->rdatastorage)
    {
        LogSPS("UpdateKeepaliveRData: Freed allocated memory for keep alive packet: %s ", ARDisplayString(m, rr));
        mDNSPlatformMemFree(rr->resrec.rdata);
    }
    SetNewRData(&rr->resrec, newrd, newrdlength);    // Update our rdata

    LogSPS("UpdateKeepaliveRData: successfully updated the record %s", ARDisplayString(m, rr));
    return mStatus_NoError;
}

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal void SendSPSRegistrationForOwner(mDNS *const m, NetworkInterfaceInfo *const intf, const mDNSOpaque16 id, const OwnerOptData *const owner)
{
    const int optspace = DNSOpt_Header_Space + DNSOpt_LeaseData_Space + DNSOpt_Owner_Space(&m->PrimaryMAC, &intf->MAC);
    const int sps = intf->NextSPSAttempt / 3;
    AuthRecord *rr;
    mDNSOpaque16 msgid;
    mDNSu32 scopeid;

    scopeid = mDNSPlatformInterfaceIndexfromInterfaceID(m, intf->InterfaceID, mDNStrue);
    if (!intf->SPSAddr[sps].type)
    {
        intf->NextSPSAttemptTime = m->timenow + mDNSPlatformOneSecond;
        if (m->NextScheduledSPRetry - intf->NextSPSAttemptTime > 0)
            m->NextScheduledSPRetry = intf->NextSPSAttemptTime;
        LogSPS("SendSPSRegistration: %s SPS %d (%d) %##s not yet resolved", intf->ifname, intf->NextSPSAttempt, sps, intf->NetWakeResolve[sps].qname.c);
        goto exit;
    }

    // Mark our mDNS records (not unicast records) for transfer to SPS
    if (mDNSOpaque16IsZero(id))
    {
        // We may have to register this record over multiple interfaces and we don't want to
        // overwrite the id. We send the registration over interface X with id "IDX" and before
        // we get a response, we overwrite with id "IDY" for interface Y and we won't accept responses
        // for "IDX". Hence, we want to use the same ID across all interfaces.
        //
        // In the case of sleep proxy server transfering its records when it goes to sleep, the owner
        // option check below will set the same ID across the records from the same owner. Records
        // with different owner option gets different ID.
        msgid = mDNS_NewMessageID(m);
        for (rr = m->ResourceRecords; rr; rr=rr->next)
        {
            if (!(rr->AuthFlags & AuthFlagsWakeOnly) && rr->resrec.RecordType > kDNSRecordTypeDeregistering)
            {
                if (rr->resrec.InterfaceID == intf->InterfaceID || (!rr->resrec.InterfaceID && (rr->ForceMCast || IsLocalDomain(rr->resrec.name))))
                {
                    if (mDNSPlatformMemSame(owner, &rr->WakeUp, sizeof(*owner)))
                    {
                        rr->SendRNow = mDNSInterfaceMark;   // mark it now
                        // When we are registering on the first interface, rr->updateid is zero in which case
                        // initialize with the new ID. For subsequent interfaces, we want to use the same ID.
                        // At the end, all the updates sent across all the interfaces with the same ID.
                        if (mDNSOpaque16IsZero(rr->updateid))
                            rr->updateid = msgid;
                        else
                            msgid = rr->updateid;
                    }
                }
            }
        }
    }
    else
        msgid = id;

    while (1)
    {
        mDNSu8 *p = m->omsg.data;
        // To comply with RFC 2782, PutResourceRecord suppresses name compression for SRV records in unicast updates.
        // For now we follow that same logic for SPS registrations too.
        // If we decide to compress SRV records in SPS registrations in the future, we can achieve that by creating our
        // initial DNSMessage with h.flags set to zero, and then update it to UpdateReqFlags right before sending the packet.
        InitializeDNSMessage(&m->omsg.h, msgid, UpdateReqFlags);

        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->SendRNow || mDNSUpdateOkToSend(m, rr, intf, scopeid))
            {
                if (mDNSPlatformMemSame(owner, &rr->WakeUp, sizeof(*owner)))
                {
                    mDNSu8 *newptr;
                    const mDNSu8 *const limit = m->omsg.data + (m->omsg.h.mDNS_numUpdates ? NormalMaxDNSMessageData : AbsoluteMaxDNSMessageData) - optspace;

                    // If we can't update the keepalive record, don't send it
                    if (mDNS_KeepaliveRecord(&rr->resrec) && (UpdateKeepaliveRData(m, rr, intf, mDNSfalse, mDNSNULL) != mStatus_NoError))
                    {
                        if (scopeid < (sizeof(rr->updateIntID) * mDNSNBBY))
                        {
                            bit_clr_opaque64(rr->updateIntID, scopeid);
                        }
                        rr->SendRNow = mDNSNULL;
                        continue;
                    }

                    if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
                        rr->resrec.rrclass |= kDNSClass_UniqueRRSet;    // Temporarily set the 'unique' bit so PutResourceRecord will set it
                    newptr = PutResourceRecordTTLWithLimit(&m->omsg, p, &m->omsg.h.mDNS_numUpdates, &rr->resrec, rr->resrec.rroriginalttl, limit);
                    rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;       // Make sure to clear 'unique' bit back to normal state
                    if (!newptr)
                        LogSPS("SendSPSRegistration put %s FAILED %d/%d %s", intf->ifname, p - m->omsg.data, limit - m->omsg.data, ARDisplayString(m, rr));
                    else
                    {
                        LogSPS("SendSPSRegistration put %s 0x%x 0x%x (updateid %d)  %s", intf->ifname, rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(m->omsg.h.id), ARDisplayString(m, rr));
                        rr->SendRNow       = mDNSNULL;
                        rr->ThisAPInterval = mDNSPlatformOneSecond;
                        rr->LastAPTime     = m->timenow;
                        // should be initialized above
                        if (mDNSOpaque16IsZero(rr->updateid)) LogMsg("SendSPSRegistration: ERROR!! rr %s updateid is zero", ARDisplayString(m, rr));
                        if (m->NextScheduledResponse - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
                            m->NextScheduledResponse = (rr->LastAPTime + rr->ThisAPInterval);
                        p = newptr;
                    }
                }
            }

        if (!m->omsg.h.mDNS_numUpdates) break;
        else
        {
            AuthRecord opt;
            mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
            opt.resrec.rrclass    = NormalMaxDNSMessageData;
            opt.resrec.rdlength   = sizeof(rdataOPT) * 2;   // Two options in this OPT record
            opt.resrec.rdestimate = sizeof(rdataOPT) * 2;
            opt.resrec.rdata->u.opt[0].opt           = kDNSOpt_Lease;
            opt.resrec.rdata->u.opt[0].optlen        = DNSOpt_LeaseData_Space - 4;
            opt.resrec.rdata->u.opt[0].u.updatelease = DEFAULT_UPDATE_LEASE;
            if (!owner->HMAC.l[0])                                          // If no owner data,
                SetupOwnerOpt(m, intf, &opt.resrec.rdata->u.opt[1]);        // use our own interface information
            else                                                            // otherwise, use the owner data we were given
            {
                opt.resrec.rdata->u.opt[1].u.owner = *owner;
                opt.resrec.rdata->u.opt[1].opt     = kDNSOpt_Owner;
                opt.resrec.rdata->u.opt[1].optlen  = DNSOpt_Owner_Space(&owner->HMAC, &owner->IMAC) - 4;
            }
            LogSPS("SendSPSRegistration put %s %s", intf->ifname, ARDisplayString(m, &opt));
            p = PutResourceRecordTTLWithLimit(&m->omsg, p, &m->omsg.h.numAdditionals, &opt.resrec, opt.resrec.rroriginalttl, m->omsg.data + AbsoluteMaxDNSMessageData);
            if (!p)
                LogMsg("SendSPSRegistration: Failed to put OPT record (%d updates) %s", m->omsg.h.mDNS_numUpdates, ARDisplayString(m, &opt));
            else
            {
                mStatus err;
                // Once we've attempted to register, we need to include our OWNER option in our packets when we re-awaken
                m->SentSleepProxyRegistration = mDNStrue;

                LogSPS("SendSPSRegistration: Sending Update %s %d (%d) id %5d with %d records %d bytes to %#a:%d", intf->ifname, intf->NextSPSAttempt, sps,
                       mDNSVal16(m->omsg.h.id), m->omsg.h.mDNS_numUpdates, p - m->omsg.data, &intf->SPSAddr[sps], mDNSVal16(intf->SPSPort[sps]));
                // if (intf->NextSPSAttempt < 5) m->omsg.h.flags = zeroID;  // For simulating packet loss
                err = mDNSSendDNSMessage(m, &m->omsg, p, intf->InterfaceID, mDNSNULL, mDNSNULL, &intf->SPSAddr[sps], intf->SPSPort[sps], mDNSNULL, mDNSfalse);
                if (err) LogSPS("SendSPSRegistration: mDNSSendDNSMessage err %d", err);
                if (err && intf->SPSAddr[sps].type == mDNSAddrType_IPv4 && intf->NetWakeResolve[sps].ThisQInterval == -1)
                {
                    LogSPS("SendSPSRegistration %d %##s failed to send to IPv4 address; will try IPv6 instead", sps, intf->NetWakeResolve[sps].qname.c);
                    intf->NetWakeResolve[sps].qtype = kDNSType_AAAA;
                    mDNS_StartQuery_internal(m, &intf->NetWakeResolve[sps]);
                    return;
                }
            }
        }
    }

    intf->NextSPSAttemptTime = m->timenow + mDNSPlatformOneSecond * 10;     // If successful, update NextSPSAttemptTime

exit:
    if (mDNSOpaque16IsZero(id) && intf->NextSPSAttempt < 8) intf->NextSPSAttempt++;
}

mDNSlocal mDNSBool RecordIsFirstOccurrenceOfOwner(mDNS *const m, const AuthRecord *const rr)
{
    AuthRecord *ar;
    for (ar = m->ResourceRecords; ar && ar != rr; ar=ar->next)
        if (mDNSPlatformMemSame(&rr->WakeUp, &ar->WakeUp, sizeof(rr->WakeUp))) return mDNSfalse;
    return mDNStrue;
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)

mDNSlocal void mDNSCoreStoreProxyRR(mDNS *const m, const mDNSInterfaceID InterfaceID, AuthRecord *const rr)
{
    AuthRecord *newRR = (AuthRecord *) mDNSPlatformMemAllocateClear(sizeof(*newRR));
    if (newRR == mDNSNULL)
    {
        LogSPS("%s : could not allocate memory for new resource record", __func__);
        return;
    }

    mDNS_SetupResourceRecord(newRR, mDNSNULL, InterfaceID, rr->resrec.rrtype,
                             rr->resrec.rroriginalttl, rr->resrec.RecordType,
                             rr->ARType, mDNSNULL, mDNSNULL);

    AssignDomainName(&newRR->namestorage, &rr->namestorage);
    newRR->resrec.rdlength = DomainNameLength(rr->resrec.name);
    newRR->resrec.namehash = DomainNameHashValue(newRR->resrec.name);
    newRR->resrec.rrclass  = rr->resrec.rrclass;

    if (rr->resrec.rrtype == kDNSType_A)
    {
        newRR->resrec.rdata->u.ipv4 =  rr->resrec.rdata->u.ipv4;
    }
    else if (rr->resrec.rrtype == kDNSType_AAAA)
    {
        newRR->resrec.rdata->u.ipv6 = rr->resrec.rdata->u.ipv6;
    }
    SetNewRData(&newRR->resrec, mDNSNULL, 0);

    // Insert the new node at the head of the list.
    newRR->next        = m->SPSRRSet;
    m->SPSRRSet        = newRR;
    LogSPS("%s : Storing proxy record : %s ", __func__, ARDisplayString(m, rr));
}

// Some records are interface specific and some are not. The ones that are supposed to be registered
// on multiple interfaces need to be initialized with all the valid interfaces on which it will be sent.
// updateIntID bit field tells us on which interfaces we need to register this record. When we get an
// ack from the sleep proxy server, we clear the interface bit. This way, we know when a record completes
// registration on all the interfaces
mDNSlocal void SPSInitRecordsBeforeUpdate(mDNS *const m, mDNSOpaque64 updateIntID, mDNSBool *WakeOnlyService)
{
    AuthRecord *ar;
    LogSPS("SPSInitRecordsBeforeUpdate: UpdateIntID 0x%x 0x%x", updateIntID.l[1], updateIntID.l[0]);

    *WakeOnlyService = mDNSfalse;

    // Before we store the A and AAAA records that we are going to register with the sleep proxy,
    // make sure that the old sleep proxy records are removed.
    mDNSCoreFreeProxyRR(m);

    // For records that are registered only on a specific interface, mark only that bit as it will
    // never be registered on any other interface. For others, it should be sent on all interfaces.
    for (ar = m->ResourceRecords; ar; ar=ar->next)
    {
        ar->updateIntID = zeroOpaque64;
        ar->updateid    = zeroID;
        if (AuthRecord_uDNS(ar))
        {
            continue;
        }
        if (ar->AuthFlags & AuthFlagsWakeOnly)
        {
            if (ar->resrec.RecordType == kDNSRecordTypeShared && ar->RequireGoodbye)
            {
                ar->ImmedAnswer = mDNSInterfaceMark;
                *WakeOnlyService = mDNStrue;
                continue;
            }
        }
        if (!ar->resrec.InterfaceID)
        {
            LogSPS("Setting scopeid (ALL) 0x%x 0x%x for %s", updateIntID.l[1], updateIntID.l[0], ARDisplayString(m, ar));
            ar->updateIntID = updateIntID;
        }
        else
        {
            // Filter records that belong to interfaces that we won't register the records on. UpdateIntID captures
            // exactly this.
            mDNSu32 scopeid = mDNSPlatformInterfaceIndexfromInterfaceID(m, ar->resrec.InterfaceID, mDNStrue);
            if ((scopeid < (sizeof(updateIntID) * mDNSNBBY)) && bit_get_opaque64(updateIntID, scopeid))
            {
                bit_set_opaque64(ar->updateIntID, scopeid);
                LogSPS("SPSInitRecordsBeforeUpdate: Setting scopeid(%d) 0x%x 0x%x for %s", scopeid, ar->updateIntID.l[1],
                    ar->updateIntID.l[0], ARDisplayString(m, ar));
            }
            else
            {
                LogSPS("SPSInitRecordsBeforeUpdate: scopeid %d beyond range or not valid for SPS registration", scopeid);
            }
        }
        // Store the A and AAAA records that we registered with the sleep proxy.
        // We will use this to prevent spurious name conflicts that may occur when we wake up
        if (ar->resrec.rrtype == kDNSType_A || ar->resrec.rrtype == kDNSType_AAAA)
        {
            mDNSCoreStoreProxyRR(m, ar->resrec.InterfaceID, ar);
        }
    }
}

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal void SendSPSRegistration(mDNS *const m, NetworkInterfaceInfo *const intf, const mDNSOpaque16 id)
{
    AuthRecord *ar;
    OwnerOptData owner = zeroOwner;

    SendSPSRegistrationForOwner(m, intf, id, &owner);

    for (ar = m->ResourceRecords; ar; ar=ar->next)
    {
        if (!mDNSPlatformMemSame(&owner, &ar->WakeUp, sizeof(owner)) && RecordIsFirstOccurrenceOfOwner(m, ar))
        {
            owner = ar->WakeUp;
            SendSPSRegistrationForOwner(m, intf, id, &owner);
        }
    }
}

// RetrySPSRegistrations is called from SendResponses, with the lock held
mDNSlocal void RetrySPSRegistrations(mDNS *const m)
{
    AuthRecord *rr;
    NetworkInterfaceInfo *intf;

    // First make sure none of our interfaces' NextSPSAttemptTimes are inadvertently set to m->timenow + mDNSPlatformOneSecond * 10
    for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        if (intf->NextSPSAttempt && intf->NextSPSAttemptTime == m->timenow + mDNSPlatformOneSecond * 10)
            intf->NextSPSAttemptTime++;

    // Retry any record registrations that are due
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (!AuthRecord_uDNS(rr) && !mDNSOpaque16IsZero(rr->updateid) && m->timenow - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
        {
            for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
            {
                // If we still have registrations pending on this interface, send it now
                mDNSu32 scopeid = mDNSPlatformInterfaceIndexfromInterfaceID(m, intf->InterfaceID, mDNStrue);
                if ((scopeid >= (sizeof(rr->updateIntID) * mDNSNBBY) || bit_get_opaque64(rr->updateIntID, scopeid)) &&
                    (!rr->resrec.InterfaceID || rr->resrec.InterfaceID == intf->InterfaceID))
                {
                    LogSPS("RetrySPSRegistrations: 0x%x 0x%x (updateid %d) %s", rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(rr->updateid), ARDisplayString(m, rr));
                    SendSPSRegistration(m, intf, rr->updateid);
                }
            }
        }

    // For interfaces where we did an SPS registration attempt, increment intf->NextSPSAttempt
    for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        if (intf->NextSPSAttempt && intf->NextSPSAttemptTime == m->timenow + mDNSPlatformOneSecond * 10 && intf->NextSPSAttempt < 8)
            intf->NextSPSAttempt++;
}

mDNSlocal void NetWakeResolve(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, QC_result AddRecord)
{
    NetworkInterfaceInfo *intf = (NetworkInterfaceInfo *)question->QuestionContext;
    int sps = (int)(question - intf->NetWakeResolve);
    (void)m;            // Unused
    LogSPS("NetWakeResolve: SPS: %d Add: %d %s", sps, AddRecord, RRDisplayString(m, answer));

    if (!AddRecord) return;                                             // Don't care about REMOVE events
    if (answer->rrtype != question->qtype) return;                      // Don't care about CNAMEs

    // if (answer->rrtype == kDNSType_AAAA && sps == 0) return; // To test failing to resolve sleep proxy's address

    if (answer->rrtype == kDNSType_SRV)
    {
        // 1. Got the SRV record; now look up the target host's IP address
        mDNS_StopQuery(m, question);
        intf->SPSPort[sps] = answer->rdata->u.srv.port;
        AssignDomainName(&question->qname, &answer->rdata->u.srv.target);
        question->qtype = kDNSType_A;
        mDNS_StartQuery(m, question);
    }
    else if (answer->rrtype == kDNSType_A && answer->rdlength == sizeof(mDNSv4Addr))
    {
        // 2. Got an IPv4 address for the target host; record address and initiate an SPS registration if appropriate
        mDNS_StopQuery(m, question);
        question->ThisQInterval = -1;
        intf->SPSAddr[sps].type = mDNSAddrType_IPv4;
        intf->SPSAddr[sps].ip.v4 = answer->rdata->u.ipv4;
        mDNS_Lock(m);
        if (sps == intf->NextSPSAttempt/3) SendSPSRegistration(m, intf, zeroID);    // If we're ready for this result, use it now
        mDNS_Unlock(m);
    }
    else if (answer->rrtype == kDNSType_A && answer->rdlength == 0)
    {
        // 3. Got negative response -- target host apparently has IPv6 disabled -- so try looking up the target host's IPv4 address(es) instead
        mDNS_StopQuery(m, question);
        LogSPS("NetWakeResolve: SPS %d %##s has no IPv4 address, will try IPv6 instead", sps, question->qname.c);
        question->qtype = kDNSType_AAAA;
        mDNS_StartQuery(m, question);
    }
    else if (answer->rrtype == kDNSType_AAAA && answer->rdlength == sizeof(mDNSv6Addr) && mDNSv6AddressIsLinkLocal(&answer->rdata->u.ipv6))
    {
        // 4. Got the target host's IPv6 link-local address; record address and initiate an SPS registration if appropriate
        mDNS_StopQuery(m, question);
        question->ThisQInterval = -1;
        intf->SPSAddr[sps].type = mDNSAddrType_IPv6;
        intf->SPSAddr[sps].ip.v6 = answer->rdata->u.ipv6;
        mDNS_Lock(m);
        if (sps == intf->NextSPSAttempt/3) SendSPSRegistration(m, intf, zeroID);    // If we're ready for this result, use it now
        mDNS_Unlock(m);
    }
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)

mDNSexport mDNSBool mDNSCoreHaveAdvertisedMulticastServices(mDNS *const m)
{
    AuthRecord *rr;
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (mDNS_KeepaliveRecord(&rr->resrec) || (rr->resrec.rrtype == kDNSType_SRV && !AuthRecord_uDNS(rr) && !mDNSSameIPPort(rr->resrec.rdata->u.srv.port, DiscardPort)))
            return mDNStrue;
    return mDNSfalse;
}

#define WAKE_ONLY_SERVICE 1
#define AC_ONLY_SERVICE   2




mDNSlocal void SendSleepGoodbyes(mDNS *const m, mDNSBool AllInterfaces, mDNSBool unicast)
{
    AuthRecord *rr;
    m->SleepState = SleepState_Sleeping;

    // If AllInterfaces is not set, the caller has already marked it appropriately
    // on which interfaces this should be sent.
    if (AllInterfaces)
    {
        NetworkInterfaceInfo *intf;
        for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        {
            intf->SendGoodbyes = 1;
        }
    }
    if (unicast)
    {
#ifndef UNICAST_DISABLED
        SleepRecordRegistrations(m);    // If we have no SPS, need to deregister our uDNS records
#endif /* UNICAST_DISABLED */
    }

    // Mark all the records we need to deregister and send them
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (rr->resrec.RecordType == kDNSRecordTypeShared && rr->RequireGoodbye)
            rr->ImmedAnswer = mDNSInterfaceMark;
    SendResponses(m);
}

/*
 * This function attempts to detect if multiple interfaces are on the same subnet.
 * It makes this determination based only on the IPv4 Addresses and subnet masks.
 * IPv6 link local addresses that are configured by default on all interfaces make
 * it hard to make this determination
 *
 * The 'real' fix for this would be to send out multicast packets over one interface
 * and conclude that multiple interfaces are on the same subnet only if these packets
 * are seen on other interfaces on the same system
 */
mDNSlocal mDNSBool skipSameSubnetRegistration(mDNS *const m, mDNSInterfaceID *regID, mDNSu32 count, mDNSInterfaceID intfid)
{
    NetworkInterfaceInfo *intf;
    NetworkInterfaceInfo *newIntf;
    mDNSu32 i;

    for (newIntf = FirstInterfaceForID(m, intfid); newIntf; newIntf = newIntf->next)
    {
        if ((newIntf->InterfaceID != intfid) ||
            (newIntf->ip.type     != mDNSAddrType_IPv4))
        {
            continue;
        }
        for ( i = 0; i < count; i++)
        {
            for (intf = FirstInterfaceForID(m, regID[i]); intf; intf = intf->next)
            {
                if ((intf->InterfaceID != regID[i]) ||
                    (intf->ip.type     != mDNSAddrType_IPv4))
                {
                    continue;
                }
                if ((intf->ip.ip.v4.NotAnInteger & intf->mask.ip.v4.NotAnInteger) == (newIntf->ip.ip.v4.NotAnInteger & newIntf->mask.ip.v4.NotAnInteger))
                {
                    LogSPS("%s : Already registered for the same subnet (IPv4) for interface %s", __func__, intf->ifname);
                    return (mDNStrue);
                }
            }
        }
    }
    return (mDNSfalse);
}

mDNSlocal void DoKeepaliveCallbacks(mDNS *m)
{
    // Loop through the keepalive records and callback with an error
    m->CurrentRecord = m->ResourceRecords;
    while (m->CurrentRecord)
    {
        AuthRecord *const rr = m->CurrentRecord;
        if ((mDNS_KeepaliveRecord(&rr->resrec)) && (rr->resrec.RecordType != kDNSRecordTypeDeregistering))
        {
            LogSPS("DoKeepaliveCallbacks: Invoking the callback for %s", ARDisplayString(m, rr));
            if (rr->RecordCallback)
                rr->RecordCallback(m, rr, mStatus_BadStateErr);
        }
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

// BeginSleepProcessing is called, with the lock held, from either mDNS_Execute or mDNSCoreMachineSleep
mDNSlocal void BeginSleepProcessing(mDNS *const m)
{
    mDNSBool SendGoodbyes = mDNStrue;
    mDNSBool WakeOnlyService  = mDNSfalse;
    mDNSBool invokeKACallback = mDNStrue;
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    const CacheRecord *sps[3] = { mDNSNULL };
#endif
    mDNSOpaque64 updateIntID = zeroOpaque64;
    mDNSInterfaceID registeredIntfIDS[128] = { 0 };
    mDNSu32 registeredCount = 0;
    int skippedRegistrations = 0;

    m->NextScheduledSPRetry = m->timenow;

    // Clear out the SCDynamic entry that stores the external SPS information
    mDNSPlatformClearSPSData();

    if      (!m->SystemWakeOnLANEnabled) LogSPS("BeginSleepProcessing: m->SystemWakeOnLANEnabled is false");
    else if (!mDNSCoreHaveAdvertisedMulticastServices(m)) LogSPS("BeginSleepProcessing: No advertised services");
    else    // If we have at least one advertised service
    {
        NetworkInterfaceInfo *intf;
        for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        {
            mDNSBool skipFullSleepProxyRegistration = mDNSfalse;
            // Intialize it to false. These values make sense only when SleepState is set to Sleeping.
            intf->SendGoodbyes = 0;

            // If it is not multicast capable, we could not have possibly discovered sleep proxy
            // servers.
            if (!intf->McastTxRx || mDNSPlatformInterfaceIsD2D(intf->InterfaceID))
            {
                LogSPS("BeginSleepProcessing: %-6s Ignoring for registrations", intf->ifname);
                continue;
            }

            // If we are not capable of WOMP, then don't register with sleep proxy.
            //
            // Note: If we are not NetWake capable, we don't browse for the sleep proxy server.
            // We might find sleep proxy servers in the cache and start a resolve on them.
            // But then if the interface goes away, we won't stop these questions because
            // mDNS_DeactivateNetWake_internal assumes that a browse has been started for it
            // to stop both the browse and resolve questions.
            if (!intf->NetWake)
            {
                LogSPS("BeginSleepProcessing: %-6s not capable of magic packet wakeup", intf->ifname);
                intf->SendGoodbyes = 1;
                skippedRegistrations++;
                continue;
            }

            // Check if we have already registered with a sleep proxy for this subnet.
            // If so, then the subsequent in-NIC sleep proxy registration is limited to any keepalive records that belong
            // to the interface.
            if (skipSameSubnetRegistration(m, registeredIntfIDS, registeredCount, intf->InterfaceID))
            {
                LogSPS("%s : Skipping full sleep proxy registration on %s", __func__, intf->ifname);
                skipFullSleepProxyRegistration = mDNStrue;
            }

        #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
            if (!skipFullSleepProxyRegistration)
            {
                FindSPSInCache(m, &intf->NetWakeBrowse, sps);
                if (!sps[0]) LogSPS("BeginSleepProcessing: %-6s %#a No Sleep Proxy Server found (Next Browse Q in %d, interval %d)",
                                    intf->ifname, &intf->ip, NextQSendTime(&intf->NetWakeBrowse) - m->timenow, intf->NetWakeBrowse.ThisQInterval);
                else
                {
                    int i;
                    mDNSu32 scopeid;
                    SendGoodbyes = mDNSfalse;
                    intf->NextSPSAttempt = 0;
                    intf->NextSPSAttemptTime = m->timenow + mDNSPlatformOneSecond;

                    scopeid = mDNSPlatformInterfaceIndexfromInterfaceID(m, intf->InterfaceID, mDNStrue);
                    // Now we know for sure that we have to wait for registration to complete on this interface.
                    if (scopeid < (sizeof(updateIntID) * mDNSNBBY))
                        bit_set_opaque64(updateIntID, scopeid);

                    // Don't need to set m->NextScheduledSPRetry here because we already set "m->NextScheduledSPRetry = m->timenow" above
                    for (i=0; i<3; i++)
                    {
#if ForceAlerts
                        if (intf->SPSAddr[i].type)
                            LogFatalError("BeginSleepProcessing: %s %d intf->SPSAddr[i].type %d", intf->ifname, i, intf->SPSAddr[i].type);
                        if (intf->NetWakeResolve[i].ThisQInterval >= 0)
                            LogFatalError("BeginSleepProcessing: %s %d intf->NetWakeResolve[i].ThisQInterval %d", intf->ifname, i, intf->NetWakeResolve[i].ThisQInterval);
#endif
                        intf->SPSAddr[i].type = mDNSAddrType_None;
                        if (intf->NetWakeResolve[i].ThisQInterval >= 0) mDNS_StopQuery(m, &intf->NetWakeResolve[i]);
                        intf->NetWakeResolve[i].ThisQInterval = -1;
                        if (sps[i])
                        {
                            LogSPS("BeginSleepProcessing: %-6s Found Sleep Proxy Server %d TTL %d %s", intf->ifname, i, sps[i]->resrec.rroriginalttl, CRDisplayString(m, sps[i]));
                            mDNS_SetupQuestion(&intf->NetWakeResolve[i], intf->InterfaceID, &sps[i]->resrec.rdata->u.name, kDNSType_SRV, NetWakeResolve, intf);
                            intf->NetWakeResolve[i].ReturnIntermed = mDNStrue;
                            mDNS_StartQuery_internal(m, &intf->NetWakeResolve[i]);

                            // If we are registering with a Sleep Proxy for a new subnet, add it to our list
                            registeredIntfIDS[registeredCount] = intf->InterfaceID;
                            registeredCount++;
                        }
                    }
                }
            }
        #endif // MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
        }
    }

    // If we have at least one interface on which we are registering with an external sleep proxy,
    // initialize all the records appropriately.
    if (!mDNSOpaque64IsZero(&updateIntID))
        SPSInitRecordsBeforeUpdate(m, updateIntID, &WakeOnlyService);

    // Call the applicaitons that registered a keepalive record to inform them that we failed to offload
    // the records to a sleep proxy.
    if (invokeKACallback)
    {
        LogSPS("BeginSleepProcessing: Did not register with an in-NIC proxy - invoking the callbacks for KA records");
        DoKeepaliveCallbacks(m);
    }

    // SendSleepGoodbyes last two arguments control whether we send goodbyes on all
    // interfaces and also deregister unicast registrations.
    //
    // - If there are no sleep proxy servers, then send goodbyes on all interfaces
    //   for both multicast and unicast.
    //
    // - If we skipped registrations on some interfaces, then we have already marked
    //   them appropriately above. We don't need to send goodbyes for unicast as
    //   we have registered with at least one sleep proxy.
    //
    // - If we are not planning to send any goodbyes, then check for WakeOnlyServices.
    //
    // Note: If we are planning to send goodbyes, we mark the record with mDNSInterfaceAny
    // and call SendResponses which inturn calls ShouldSendGoodbyesBeforeSleep which looks
    // at WakeOnlyServices first.
    if (SendGoodbyes)
    {
        LogSPS("BeginSleepProcessing: Not registering with Sleep Proxy Server");
        SendSleepGoodbyes(m, mDNStrue, mDNStrue);
    }
    else if (skippedRegistrations)
    {
        LogSPS("BeginSleepProcessing: Not registering with Sleep Proxy Server on all interfaces");
        SendSleepGoodbyes(m, mDNSfalse, mDNSfalse);
    }
    else if (WakeOnlyService)
    {
        // If we saw WakeOnly service above, send the goodbyes now.
        LogSPS("BeginSleepProcessing: Sending goodbyes for WakeOnlyService");
        SendResponses(m);
    }
}

// Call mDNSCoreMachineSleep(m, mDNStrue) when the machine is about to go to sleep.
// Call mDNSCoreMachineSleep(m, mDNSfalse) when the machine is has just woken up.
// Normally, the platform support layer below mDNSCore should call this, not the client layer above.
mDNSexport void mDNSCoreMachineSleep(mDNS *const m, mDNSBool sleep)
{
    AuthRecord *rr;

    LogRedact(MDNS_LOG_CATEGORY_SPS, MDNS_LOG_DEFAULT, PUB_S " (old state %d) at %d", sleep ? "Sleeping" : "Waking", m->SleepState, m->timenow);

    if (sleep && !m->SleepState)        // Going to sleep
    {
        mDNS_Lock(m);
        // If we're going to sleep, need to stop advertising that we're a Sleep Proxy Server
        if (m->SPSSocket)
        {
            mDNSu8 oldstate = m->SPSState;
            mDNS_DropLockBeforeCallback();      // mDNS_DeregisterService expects to be called without the lock held, so we emulate that here
            m->SPSState = 2;
#ifndef SPC_DISABLED
            if (oldstate == 1) mDNS_DeregisterService(m, &m->SPSRecords);
#else
            (void)oldstate;
#endif
            mDNS_ReclaimLockAfterCallback();
        }
#ifdef _LEGACY_NAT_TRAVERSAL_
        if (m->SSDPSocket)
        {
            mDNSPlatformUDPClose(m->SSDPSocket);
            m->SSDPSocket = mDNSNULL;
        }
#endif
        m->SleepState = SleepState_Transferring;
        if (m->SystemWakeOnLANEnabled && m->DelaySleep)
        {
            // If we just woke up moments ago, allow ten seconds for networking to stabilize before going back to sleep
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNSCoreMachineSleep: Re-sleeping immediately after waking; will delay for %d ticks",
                m->DelaySleep - m->timenow);
            m->SleepLimit = NonZeroTime(m->DelaySleep + mDNSPlatformOneSecond * 10);
        }
        else
        {
            m->DelaySleep = 0;
            m->SleepLimit = NonZeroTime(m->timenow + mDNSPlatformOneSecond * 10);
            m->mDNSStats.Sleeps++;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            Querier_HandleSleep();
#endif
            BeginSleepProcessing(m);
        }

#ifndef UNICAST_DISABLED
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        SuspendLLQs(m);
#endif
#endif
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,  "mDNSCoreMachineSleep: m->SleepState %d (" PUB_S ") seq %d", m->SleepState,
            m->SleepState == SleepState_Transferring ? "Transferring" :
            m->SleepState == SleepState_Sleeping     ? "Sleeping"     : "?", m->SleepSeqNum);
        mDNS_Unlock(m);
    }
    else if (!sleep)        // Waking up
    {
        mDNSu32 slot;
        CacheGroup *cg;
        CacheRecord *cr;
        mDNSs32 currtime, diff;

        mDNS_Lock(m);
        // Reset SleepLimit back to 0 now that we're awake again.
        m->SleepLimit = 0;

        // If we were previously sleeping, but now we're not, increment m->SleepSeqNum to indicate that we're entering a new period of wakefulness
        if (m->SleepState != SleepState_Awake)
        {
            m->SleepState = SleepState_Awake;
            m->SleepSeqNum++;
        #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
            if (m->SentSleepProxyRegistration)		// Include OWNER option in packets for 60 seconds after waking
            {
                m->SentSleepProxyRegistration = mDNSfalse;
                m->AnnounceOwner = NonZeroTime(m->timenow + 60 * mDNSPlatformOneSecond);
                LogInfo("mDNSCoreMachineSleep: Waking, Setting AnnounceOwner");
            }
        #endif
            // If the machine wakes and then immediately tries to sleep again (e.g. a maintenance wake)
            // then we enforce a minimum delay of five seconds before we begin sleep processing.
            // This is to allow time for the Ethernet link to come up, DHCP to get an address, mDNS to issue queries, etc.,
            // before we make our determination of whether there's a Sleep Proxy out there we should register with.
            m->DelaySleep = NonZeroTime(m->timenow + kDarkWakeDelaySleep);
        }

        if (m->SPSState == 3)
        {
            m->SPSState = 0;
            mDNSCoreBeSleepProxyServer_internal(m, m->SPSType, m->SPSPortability, m->SPSMarginalPower, m->SPSTotalPower, m->SPSFeatureFlags);
        }
        m->mDNSStats.Wakes++;
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
        // ... and the same for NextSPSAttempt
        NetworkInterfaceInfo *intf;
        for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        {
            intf->NextSPSAttempt = -1;
        }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        Querier_HandleWake();
#endif
        // Restart unicast and multicast queries
        mDNSCoreRestartQueries(m);

        // and reactivtate service registrations
        m->NextSRVUpdate = NonZeroTime(m->timenow + mDNSPlatformOneSecond);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNSCoreMachineSleep waking: NextSRVUpdate in %d %d", m->NextSRVUpdate - m->timenow, m->timenow);

        // 2. Re-validate our cache records
        currtime = mDNSPlatformUTC();

        diff = currtime - m->TimeSlept;
        FORALL_CACHERECORDS(slot, cg, cr)
        {
            // Temporary fix: For unicast cache records, look at how much time we slept.
            // Adjust the RecvTime by the amount of time we slept so that we age the
            // cache record appropriately. If it is expired already, purge. If there
            // is a network change that happens after the wakeup, we might purge the
            // cache anyways and this helps only in the case where there are no network
            // changes across sleep/wakeup transition.
            //
            // Note: If there is a network/DNS server change that already happened and
            // these cache entries are already refreshed and we are getting a delayed
            // wake up notification, we might adjust the TimeRcvd based on the time slept
            // now which can cause the cache to purge pre-maturely. As this is not a very
            // common case, this should happen rarely.
            if (!cr->resrec.InterfaceID)
            {
                if (diff > 0)
                {
                    mDNSu32 uTTL = RRUnadjustedTTL(cr->resrec.rroriginalttl);
                    const mDNSs32 remain = uTTL - (m->timenow - cr->TimeRcvd) / mDNSPlatformOneSecond;

                    // -if we have slept longer than the remaining TTL, purge and start fresh.
                    // -if we have been sleeping for a long time, we could reduce TimeRcvd below by
                    //  a sufficiently big value which could cause the value to go into the future
                    //  because of the signed comparison of time. For this to happen, we should have been
                    //  sleeping really long (~24 days). For now, we want to be conservative and flush even
                    //  if we have slept for more than two days.

                    if (diff >= remain || diff > (2 * 24 * 3600))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNSCoreMachineSleep: " PRI_S ": Purging cache entry SleptTime %d, Remaining TTL %d",
                            CRDisplayString(m, cr), diff, remain);
                        mDNS_PurgeCacheResourceRecord(m, cr);
                        continue;
                    }
                    cr->TimeRcvd -= (diff * mDNSPlatformOneSecond);
                    if (m->timenow - (cr->TimeRcvd + ((mDNSs32)uTTL * mDNSPlatformOneSecond)) >= 0)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNSCoreMachineSleep: " PRI_S ": Purging after adjusting the remaining TTL %d by %d seconds",
                            CRDisplayString(m, cr), remain, diff);
                        mDNS_PurgeCacheResourceRecord(m, cr);
                    }
                    else
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNSCoreMachineSleep: " PRI_S ": Adjusted the remain ttl %u by %d seconds",
                            CRDisplayString(m, cr), remain, diff);
                    }
                }
            }
            else
            {
                mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForWake);
            }
        }

        // 3. Retrigger probing and announcing for all our authoritative records
        for (rr = m->ResourceRecords; rr; rr=rr->next)
        {
            if (AuthRecord_uDNS(rr))
            {
                ActivateUnicastRegistration(m, rr);
            }
            else
            {
                mDNSCoreRestartRegistration(m, rr, -1);
            }
        }

        // 4. Refresh NAT mappings
        // We don't want to have to assume that all hardware can necessarily keep accurate
        // track of passage of time while asleep, so on wake we refresh our NAT mappings.
        // We typically wake up with no interfaces active, so there's no need to rush to try to find our external address.
        // But if we do get a network configuration change, mDNSMacOSXNetworkChanged will call uDNS_SetupDNSConfig, which
        // will call mDNS_SetPrimaryInterfaceInfo, which will call RecreateNATMappings to refresh them, potentially sooner
        // than five seconds from now.
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,  "mDNSCoreMachineSleep: recreating NAT mappings in 5 seconds");
        RecreateNATMappings(m, mDNSPlatformOneSecond * 5);
        mDNS_Unlock(m);
    }
}

mDNSexport mDNSBool mDNSCoreReadyForSleep(mDNS *m, mDNSs32 now)
{
    DNSQuestion *q;
    AuthRecord *rr;

    mDNS_Lock(m);

    if (m->DelaySleep) goto notready;

    // If we've not hit the sleep limit time, and it's not time for our next retry, we can skip these checks
    if (m->SleepLimit - now > 0 && m->NextScheduledSPRetry - now > 0) goto notready;

    m->NextScheduledSPRetry = now + 0x40000000UL;

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    NetworkInterfaceInfo *intf;
    // See if we might need to retransmit any lost Sleep Proxy Registrations
    for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
        if (intf->NextSPSAttempt >= 0)
        {
            if (now - intf->NextSPSAttemptTime >= 0)
            {
                LogSPS("mDNSCoreReadyForSleep: retrying for %s SPS %d try %d",
                       intf->ifname, intf->NextSPSAttempt/3, intf->NextSPSAttempt);
                SendSPSRegistration(m, intf, zeroID);
                // Don't need to "goto notready" here, because if we do still have record registrations
                // that have not been acknowledged yet, we'll catch that in the record list scan below.
            }
            else
            if (m->NextScheduledSPRetry - intf->NextSPSAttemptTime > 0)
                m->NextScheduledSPRetry = intf->NextSPSAttemptTime;
        }

    // Scan list of interfaces, and see if we're still waiting for any sleep proxy resolves to complete
    for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
    {
        int sps = (intf->NextSPSAttempt == 0) ? 0 : (intf->NextSPSAttempt-1)/3;
        if (intf->NetWakeResolve[sps].ThisQInterval >= 0)
        {
            LogSPS("mDNSCoreReadyForSleep: waiting for SPS Resolve %s %##s (%s)",
                   intf->ifname, intf->NetWakeResolve[sps].qname.c, DNSTypeName(intf->NetWakeResolve[sps].qtype));
            goto spsnotready;
        }
    }

    // Scan list of registered records
    for (rr = m->ResourceRecords; rr; rr = rr->next)
        if (!AuthRecord_uDNS(rr) && rr->resrec.RecordType > kDNSRecordTypeDeregistering)
            if (!mDNSOpaque64IsZero(&rr->updateIntID))
            { LogSPS("mDNSCoreReadyForSleep: waiting for SPS updateIntID 0x%x 0x%x (updateid %d) %s", rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(rr->updateid), ARDisplayString(m,rr)); goto spsnotready; }
#endif

    // Scan list of private LLQs, and make sure they've all completed their handshake with the server
    for (q = m->Questions; q; q = q->next)
        if (!mDNSOpaque16IsZero(q->TargetQID) && q->LongLived && q->ReqLease == 0 && q->tcp)
        {
            LogSPS("mDNSCoreReadyForSleep: waiting for LLQ %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
            goto notready;
        }

    // Scan list of registered records
    for (rr = m->ResourceRecords; rr; rr = rr->next)
        if (AuthRecord_uDNS(rr))
        {
            if (rr->state == regState_Refresh && rr->tcp)
            { LogSPS("mDNSCoreReadyForSleep: waiting for Record updateIntID 0x%x 0x%x (updateid %d) %s", rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(rr->updateid), ARDisplayString(m,rr)); goto notready; }
        }

    mDNS_Unlock(m);
    return mDNStrue;

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
spsnotready:

    // If we failed to complete sleep proxy registration within ten seconds, we give up on that
    // and allow up to ten seconds more to complete wide-area deregistration instead
    if (now - m->SleepLimit >= 0)
    {
        LogMsg("Failed to register with SPS, now sending goodbyes");

        for (intf = GetFirstActiveInterface(m->HostInterfaces); intf; intf = GetFirstActiveInterface(intf->next))
            if (intf->NetWakeBrowse.ThisQInterval >= 0)
            {
                LogSPS("ReadyForSleep mDNS_DeactivateNetWake %s %##s (%s)",
                       intf->ifname, intf->NetWakeResolve[0].qname.c, DNSTypeName(intf->NetWakeResolve[0].qtype));
                mDNS_DeactivateNetWake_internal(m, intf);
            }

        for (rr = m->ResourceRecords; rr; rr = rr->next)
            if (!AuthRecord_uDNS(rr))
                if (!mDNSOpaque64IsZero(&rr->updateIntID))
                {
                    LogSPS("ReadyForSleep clearing updateIntID 0x%x 0x%x (updateid %d) for %s", rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(rr->updateid), ARDisplayString(m, rr));
                    rr->updateIntID = zeroOpaque64;
                }

        // We'd really like to allow up to ten seconds more here,
        // but if we don't respond to the sleep notification within 30 seconds
        // we'll be put back to sleep forcibly without the chance to schedule the next maintenance wake.
        // Right now we wait 16 sec after wake for all the interfaces to come up, then we wait up to 10 seconds
        // more for SPS resolves and record registrations to complete, which puts us at 26 seconds.
        // If we allow just one more second to send our goodbyes, that puts us at 27 seconds.
        m->SleepLimit = now + mDNSPlatformOneSecond * 1;

        SendSleepGoodbyes(m, mDNStrue, mDNStrue);
    }
#endif

notready:
    mDNS_Unlock(m);
    return mDNSfalse;
}

mDNSexport mDNSs32 mDNSCoreIntervalToNextWake(mDNS *const m, mDNSs32 now, mDNSNextWakeReason *outReason)
{
    AuthRecord *ar;

    // Even when we have no wake-on-LAN-capable interfaces, or we failed to find a sleep proxy, or we have other
    // failure scenarios, we still want to wake up in at most 120 minutes, to see if the network environment has changed.
    // E.g. we might wake up and find no wireless network because the base station got rebooted just at that moment,
    // and if that happens we don't want to just give up and go back to sleep and never try again.
    mDNSs32 e = now + (120 * 60 * mDNSPlatformOneSecond);       // Sleep for at most 120 minutes
    mDNSNextWakeReason reason = mDNSNextWakeReason_UpkeepWake;

    NATTraversalInfo *nat;
    for (nat = m->NATTraversals; nat; nat=nat->next)
    {
        if (nat->Protocol && nat->ExpiryTime && nat->ExpiryTime - now > mDNSPlatformOneSecond*4)
        {
            mDNSs32 t = nat->ExpiryTime - (nat->ExpiryTime - now) / 10;     // Wake up when 90% of the way to the expiry time
            if ((e - t) > 0)
            {
                e = t;
                reason = mDNSNextWakeReason_NATPortMappingRenewal;
            }
            LogSPS("ComputeWakeTime: %p %s Int %5d Ext %5d Err %d Retry %5d Interval %5d Expire %5d Wake %5d",
                   nat, nat->Protocol == NATOp_MapTCP ? "TCP" : "UDP",
                   mDNSVal16(nat->IntPort), mDNSVal16(nat->ExternalPort), nat->Result,
                   nat->retryPortMap ? (nat->retryPortMap - now) / mDNSPlatformOneSecond : 0,
                   nat->retryInterval / mDNSPlatformOneSecond,
                   nat->ExpiryTime ? (nat->ExpiryTime - now) / mDNSPlatformOneSecond : 0,
                   (t - now) / mDNSPlatformOneSecond);
        }
    }
    // This loop checks both the time we need to renew wide-area registrations,
    // and the time we need to renew Sleep Proxy registrations
    for (ar = m->ResourceRecords; ar; ar = ar->next)
    {
        if (ar->expire && ar->expire - now > mDNSPlatformOneSecond*4)
        {
            mDNSs32 t = ar->expire - (ar->expire - now) / 10;       // Wake up when 90% of the way to the expiry time
            if ((e - t) > 0)
            {
                e = t;
                reason = mDNSNextWakeReason_RecordRegistrationRenewal;
            }
            LogSPS("ComputeWakeTime: %p Int %7d Next %7d Expire %7d Wake %7d %s",
                   ar, ar->ThisAPInterval / mDNSPlatformOneSecond,
                   (ar->LastAPTime + ar->ThisAPInterval - now) / mDNSPlatformOneSecond,
                   ar->expire ? (ar->expire - now) / mDNSPlatformOneSecond : 0,
                   (t - now) / mDNSPlatformOneSecond, ARDisplayString(m, ar));
        }
    }
    if (outReason)
    {
        *outReason = reason;
    }
    return(e - now);
}

// ***************************************************************************
// MARK: - Packet Reception Functions

#define MustSendRecord(RR) ((RR)->NR_AnswerTo || (RR)->NR_AdditionalTo)

mDNSlocal mDNSu8 *GenerateUnicastResponse(const DNSMessage *const query, const mDNSu8 *const end,
                                          const mDNSInterfaceID InterfaceID, mDNSBool LegacyQuery, DNSMessage *const response, AuthRecord *ResponseRecords)
{
    mDNSu8          *responseptr     = response->data;
    const mDNSu8    *const limit     = response->data + sizeof(response->data);
    const mDNSu8    *ptr             = query->data;
    AuthRecord  *rr;
    mDNSu32 maxttl = (!InterfaceID) ? mDNSMaximumUnicastTTLSeconds : mDNSMaximumMulticastTTLSeconds;
    int i;

    // Initialize the response fields so we can answer the questions
    InitializeDNSMessage(&response->h, query->h.id, ResponseFlags);

    // ***
    // *** 1. Write out the list of questions we are actually going to answer with this packet
    // ***
    if (LegacyQuery)
    {
        maxttl = kStaticCacheTTL;
        for (i=0; i<query->h.numQuestions; i++)                     // For each question...
        {
            DNSQuestion q;
            ptr = getQuestion(query, ptr, end, InterfaceID, &q);    // get the question...
            if (!ptr) return(mDNSNULL);

            for (rr=ResponseRecords; rr; rr=rr->NextResponse)       // and search our list of proposed answers
            {
                if (rr->NR_AnswerTo == ptr)                         // If we're going to generate a record answering this question
                {                                                   // then put the question in the question section
                    responseptr = putQuestion(response, responseptr, limit, &q.qname, q.qtype, q.qclass);
                    if (!responseptr) { debugf("GenerateUnicastResponse: Ran out of space for questions!"); return(mDNSNULL); }
                    break;      // break out of the ResponseRecords loop, and go on to the next question
                }
            }
        }

        if (response->h.numQuestions == 0) { LogMsg("GenerateUnicastResponse: ERROR! Why no questions?"); return(mDNSNULL); }
    }

    // ***
    // *** 2. Write Answers
    // ***
    for (rr=ResponseRecords; rr; rr=rr->NextResponse)
        if (rr->NR_AnswerTo)
        {
            mDNSu8 *p = PutResourceRecordTTL(response, responseptr, &response->h.numAnswers, &rr->resrec,
                                             maxttl < rr->resrec.rroriginalttl ? maxttl : rr->resrec.rroriginalttl);
            if (p) responseptr = p;
            else { debugf("GenerateUnicastResponse: Ran out of space for answers!"); response->h.flags.b[0] |= kDNSFlag0_TC; }
        }

    // ***
    // *** 3. Write Additionals
    // ***
    for (rr=ResponseRecords; rr; rr=rr->NextResponse)
        if (rr->NR_AdditionalTo && !rr->NR_AnswerTo)
        {
            mDNSu8 *p = PutResourceRecordTTL(response, responseptr, &response->h.numAdditionals, &rr->resrec,
                                             maxttl < rr->resrec.rroriginalttl ? maxttl : rr->resrec.rroriginalttl);
            if (p) responseptr = p;
            else debugf("GenerateUnicastResponse: No more space for additionals");
        }

    return(responseptr);
}

// AuthRecord *our is our Resource Record
// CacheRecord *pkt is the Resource Record from the response packet we've witnessed on the network
// Returns 0 if there is no conflict
// Returns +1 if there was a conflict and we won
// Returns -1 if there was a conflict and we lost and have to rename
mDNSlocal int CompareRData(const AuthRecord *const our, const CacheRecord *const pkt)
{
    mDNSu8 ourdata[256], *ourptr = ourdata, *ourend;
    mDNSu8 pktdata[256], *pktptr = pktdata, *pktend;
    if (!our) { LogMsg("CompareRData ERROR: our is NULL"); return(+1); }
    if (!pkt) { LogMsg("CompareRData ERROR: pkt is NULL"); return(+1); }

#if defined(__clang_analyzer__)
    // Get rid of analyzer warnings about ourptr and pktptr pointing to garbage after retruning from putRData().
    // There are no clear indications from the analyzer of the cause of the supposed problem.
    mDNSPlatformMemZero(ourdata, 1);
    mDNSPlatformMemZero(pktdata, 1);
#endif
    ourend = putRData(mDNSNULL, ourdata, ourdata + sizeof(ourdata), &our->resrec);
    pktend = putRData(mDNSNULL, pktdata, pktdata + sizeof(pktdata), &pkt->resrec);
    while (ourptr < ourend && pktptr < pktend && *ourptr == *pktptr) { ourptr++; pktptr++; }
    if (ourptr >= ourend && pktptr >= pktend) return(0);            // If data identical, not a conflict

    if (ourptr >= ourend) return(-1);                               // Our data ran out first; We lost
    if (pktptr >= pktend) return(+1);                               // Packet data ran out first; We won
    if (*pktptr > *ourptr) return(-1);                              // Our data is numerically lower; We lost
    if (*pktptr < *ourptr) return(+1);                              // Packet data is numerically lower; We won

    LogMsg("CompareRData ERROR: Invalid state");
    return(-1);
}

mDNSlocal mDNSBool PacketRecordMatches(const AuthRecord *const rr, const CacheRecord *const pktrr, const AuthRecord *const master)
{
    if (IdenticalResourceRecord(&rr->resrec, &pktrr->resrec))
    {
        const AuthRecord *r2 = rr;
        while (r2->DependentOn) r2 = r2->DependentOn;
        if (r2 == master) return(mDNStrue);
    }
    return(mDNSfalse);
}

// See if we have an authoritative record that's identical to this packet record,
// whose canonical DependentOn record is the specified master record.
// The DependentOn pointer is typically used for the TXT record of service registrations
// It indicates that there is no inherent conflict detection for the TXT record
// -- it depends on the SRV record to resolve name conflicts
// If we find any identical ResourceRecords in our authoritative list, then follow their DependentOn
// pointer chain (if any) to make sure we reach the canonical DependentOn record
// If the record has no DependentOn, then just return that record's pointer
// Returns NULL if we don't have any local RRs that are identical to the one from the packet
mDNSlocal mDNSBool MatchDependentOn(const mDNS *const m, const CacheRecord *const pktrr, const AuthRecord *const master)
{
    const AuthRecord *r1;
    for (r1 = m->ResourceRecords; r1; r1=r1->next)
    {
        if (PacketRecordMatches(r1, pktrr, master)) return(mDNStrue);
    }
    for (r1 = m->DuplicateRecords; r1; r1=r1->next)
    {
        if (PacketRecordMatches(r1, pktrr, master)) return(mDNStrue);
    }
    return(mDNSfalse);
}

// Find the canonical RRSet pointer for this RR received in a packet.
// If we find any identical AuthRecord in our authoritative list, then follow its RRSet
// pointers (if any) to make sure we return the canonical member of this name/type/class
// Returns NULL if we don't have any local RRs that are identical to the one from the packet
mDNSlocal uintptr_t FindRRSet(const mDNS *const m, const CacheRecord *const pktrr)
{
    const AuthRecord *rr;
    for (rr = m->ResourceRecords; rr; rr=rr->next)
    {
        if (IdenticalResourceRecord(&rr->resrec, &pktrr->resrec))
        {
            return(rr->RRSet ? rr->RRSet : (uintptr_t)rr);
        }
    }
    return(0);
}

// PacketRRConflict is called when we've received an RR (pktrr) which has the same name
// as one of our records (our) but different rdata.
// 1. If our record is not a type that's supposed to be unique, we don't care.
// 2a. If our record is marked as dependent on some other record for conflict detection, ignore this one.
// 2b. If the packet rr exactly matches one of our other RRs, and *that* record's DependentOn pointer
//     points to our record, ignore this conflict (e.g. the packet record matches one of our
//     TXT records, and that record is marked as dependent on 'our', its SRV record).
// 3. If we have some *other* RR that exactly matches the one from the packet, and that record and our record
//    are members of the same RRSet, then this is not a conflict.
mDNSlocal mDNSBool PacketRRConflict(const mDNS *const m, const AuthRecord *const our, const CacheRecord *const pktrr)
{
    // If not supposed to be unique, not a conflict
    if (!(our->resrec.RecordType & kDNSRecordTypeUniqueMask)) return(mDNSfalse);

    // If a dependent record, not a conflict
    if (our->DependentOn || MatchDependentOn(m, pktrr, our)) return(mDNSfalse);
    else
    {
        // If the pktrr matches a member of ourset, not a conflict
        const uintptr_t ourset = our->RRSet ? our->RRSet : (uintptr_t)our;
        const uintptr_t pktset = FindRRSet(m, pktrr);
        if (pktset == ourset) return(mDNSfalse);

        // For records we're proxying, where we don't know the full
        // relationship between the records, having any matching record
        // in our AuthRecords list is sufficient evidence of non-conflict
        if (our->WakeUp.HMAC.l[0] && pktset) return(mDNSfalse);
    }

    // Okay, this is a conflict
    return(mDNStrue);
}

// If we don't have TSR record or probe doesn't have TSR record that has the same name with auth record, return 0;
// If both have TSR, then compare tsr_value in our TSR AuthRecord and the TSR record in probe.
mDNSexport eTSRCheckResult CheckTSRForResourceRecord(const TSROptData *curTSROpt, const ResourceRecord *ourTSRRec)
{
#define TSR_QUANTIZATION_SECS    2
    eTSRCheckResult result = eTSRCheckNoKeyMatch;
    if (curTSROpt && ourTSRRec)
    {
        const TSROptData *ourTSROpt = &ourTSRRec->rdata->u.opt[0].u.tsr;
        if (ourTSROpt->hostkeyHash == curTSROpt->hostkeyHash)
        {
            result = eTSRCheckKeyMatch;
            // tsr_value stored locally is absolute time.
            mDNSs32 ourTimeOfReceipt = ourTSROpt->timeStamp;
            // tsr_value in packet is relative time.
            mDNSs32 pktTimeSinceReceived = curTSROpt->timeStamp;
            mDNSs32 pktTimeOfReceipt;
            // out of range tsr_value in pkt
            if (pktTimeSinceReceived < 0 || pktTimeSinceReceived > MaxTimeSinceReceived)
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "CheckTSR - Out of range pktTimeSinceReceived %d in Pkt record", pktTimeSinceReceived);
                pktTimeSinceReceived = MaxTimeSinceReceived;
            }
            pktTimeOfReceipt = mDNSPlatformContinuousTimeSeconds() - pktTimeSinceReceived;
            // tsr in probe is newer counted as we lose.
            if (abs(ourTimeOfReceipt - pktTimeOfReceipt) > TSR_QUANTIZATION_SECS)
            {
                result =  (ourTimeOfReceipt < pktTimeOfReceipt) ? eTSRCheckLose : eTSRCheckWin;
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                    "CheckTSR - pktTimeOfReceipt: %d %x " PUB_S " ourTimeOfReceipt: %d %x",
                    pktTimeOfReceipt, curTSROpt->hostkeyHash, result < 0 ? "lose" : "win", ourTimeOfReceipt,
                    ourTSROpt->hostkeyHash);
            }
        }
    }
    return result;
}

mDNSlocal eTSRCheckResult CheckTSRForAuthRecord(mDNS *const m, const TSROptData *curTSROpt, const AuthRecord *const ar)
{
    const AuthRecord *ourTSR = mDNSGetTSRForAuthRecord(m, ar);
    if (ourTSR)
    {
        return CheckTSRForResourceRecord(curTSROpt, &ourTSR->resrec);
    }
    return eTSRCheckNoKeyMatch;
}

// Note: ResolveSimultaneousProbe calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void ResolveSimultaneousProbe(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
    DNSQuestion *q, AuthRecord *our, const TSROptData *curTSR)
{
    int i;
    const mDNSu8 *ptr = LocateAuthorities(query, end);
    mDNSBool FoundUpdate = mDNSfalse;

    for (i = 0; i < query->h.numAuthorities; i++)
    {
        ptr = GetLargeResourceRecord(m, query, ptr, end, q->InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
        if (!ptr) break;
        if (m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative && CacheRecordAnswersQuestion(&m->rec.r, q))
        {
            FoundUpdate = mDNStrue;
            if (curTSR)
            {
                // When conflict happens, look for TSR loss.
                eTSRCheckResult tsrResult = CheckTSRForAuthRecord(m, curTSR, our);
                if (tsrResult == eTSRCheckLose)
                {
                    LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                        "ResolveSimultaneousProbe - deregistering " PRI_DM_NAME " type " PUB_S " on interface id: %p due to TSR conflict",
                        DM_NAME_PARAM(our->resrec.name), DNSTypeName(our->resrec.rrtype), our->resrec.InterfaceID);
                    mDNS_Deregister_internal(m, our, mDNS_Dereg_stale);
                    goto exit;
                }
                if (tsrResult != eTSRCheckNoKeyMatch) // No else
                {
                    goto exit;
                }
            }

            if (PacketRRConflict(m, our, &m->rec.r))
            {
                int result = 0;

                if (!result) result = (int)our->resrec.rrclass - (int)m->rec.r.resrec.rrclass;
                if (!result) result = (int)our->resrec.rrtype  - (int)m->rec.r.resrec.rrtype;
                if (!result) result = CompareRData(our, &m->rec.r);
                if (result)
                {
                    const char *const msg = (result < 0) ? "lost:" : (result > 0) ? "won: " : "tie: ";
                    LogMsg("ResolveSimultaneousProbe: %p Pkt Record:        %08lX %s", q->InterfaceID, m->rec.r.resrec.rdatahash, CRDisplayString(m, &m->rec.r));
                    LogMsg("ResolveSimultaneousProbe: %p Our Record %d %s %08lX %s", our->resrec.InterfaceID, our->ProbeCount, msg, our->resrec.rdatahash, ARDisplayString(m, our));
                }
                // If we lost the tie-break for simultaneous probes, we don't immediately give up, because we might be seeing stale packets on the network.
                // Instead we pause for one second, to give the other host (if real) a chance to establish its name, and then try probing again.
                // If there really is another live host out there with the same name, it will answer our probes and we'll then rename.
                if (result < 0)
                {
                    m->SuppressProbes   = NonZeroTime(m->timenow + mDNSPlatformOneSecond);
                    our->ProbeCount     = DefaultProbeCountForTypeUnique;
                    our->AnnounceCount  = InitialAnnounceCount;
                    InitializeLastAPTime(m, our);
                    goto exit;
                }
            }
#if 0
            else
            {
                LogMsg("ResolveSimultaneousProbe: %p Pkt Record:        %08lX %s", q->InterfaceID, m->rec.r.resrec.rdatahash, CRDisplayString(m, &m->rec.r));
                LogMsg("ResolveSimultaneousProbe: %p Our Record %d ign:  %08lX %s", our->resrec.InterfaceID, our->ProbeCount, our->resrec.rdatahash, ARDisplayString(m, our));
            }
#endif
        }
        mDNSCoreResetRecord(m);
    }
    if (!FoundUpdate)
        LogInfo("ResolveSimultaneousProbe: %##s (%s): No Update Record found", our->resrec.name->c, DNSTypeName(our->resrec.rrtype));
exit:
    mDNSCoreResetRecord(m);
}


// Return mDNStrue if the query is a probe and has an identical record in the authority section.
// Otherwise return mDNSfalse.
mDNSlocal mDNSBool ProbeHasIdenticalRR(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
                                       const DNSQuestion *const q, const AuthRecord *const our)
{
    int i;
    const mDNSu8 *ptr = LocateAuthorities(query, end);
    mDNSBool result = mDNSfalse;

    // This is not a probe
    if (!ptr || query->h.numAuthorities == 0)
    {
        goto done;
    }
    for (i = 0; i < query->h.numAuthorities && ptr; i++)
    {
        ptr = GetLargeResourceRecord(m, query, ptr, end, q->InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
        if (!ptr)
        {
            break;
        }
        if (IdenticalSameNameRecord(&m->rec.r.resrec, &our->resrec))
        {
            mDNSCoreResetRecord(m);
            result = mDNStrue;
            goto done;
        }
        mDNSCoreResetRecord(m);
    }
done:
    return result;
}

// Step1: Compare the TSR AuthRecord and the TSR record in probe,
//        skip conflict check if there is no TSR record or no hashkey match
// Step2: Check whether auth names in probe match our auth record;
// Return mDNStrue if the TSR in probe wins, otherwise return mDNSfalse.
mDNSlocal mDNSBool ProbeRRMatchAndTSRCheck(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
    const DNSQuestion *const q, const AuthRecord *const our, const TSROptData *curTSR)
{
    int i;
    const mDNSu8 *ptr = LocateAuthorities(query, end);
    mDNSBool conflict = mDNSfalse;
    mDNSBool probeTSRWin = mDNSfalse;

    // This is not a probe
    if (ptr == mDNSNULL || query->h.numAuthorities == 0)
    {
        goto done;
    }
    if (CheckTSRForAuthRecord(m, curTSR, our) == eTSRCheckLose)
    {
        probeTSRWin = mDNStrue;
    }
    else
    {
        goto done;
    }
    for (i = 0; i < query->h.numAuthorities && ptr; i++)
    {
        ptr = GetLargeResourceRecord(m, query, ptr, end, q->InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
        if (ptr == mDNSNULL)
        {
            break;
        }
        if (PacketRRMatchesSignature(&m->rec.r, our) && (our->resrec.RecordType & kDNSRecordTypeUniqueMask))
        {
            mDNSCoreResetRecord(m);
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "ProbeRRMatchAndTSRCheck: pkt ar on interface  %p rrtype: " PRI_S ", name: " PRI_DM_NAME PRI_S,
                      q->InterfaceID, DNSTypeName(m->rec.r.resrec.rrtype), DM_NAME_PARAM(m->rec.r.resrec.name), CRDisplayString(m, &m->rec.r));
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "ProbeRRMatchAndTSRCheck: Conflict with our ar %p rrtype: " PRI_S ", name: " PRI_DM_NAME PRI_S,
                      our->resrec.InterfaceID, DNSTypeName(our->resrec.rrtype), DM_NAME_PARAM(our->resrec.name), ARDisplayString(m, our));
            conflict = mDNStrue;
            break;
        }
        mDNSCoreResetRecord(m);
    }
done:
    return (conflict && probeTSRWin);
}

mDNSlocal const mDNSu8 *DomainNamePtrAtTSRIndex(const DNSMessage *const msg, const mDNSu8 *const end, mDNSu16 recIndex)
{
    mDNSu16 i = 0;
    const mDNSu8 *ptr = mDNSNULL;
    if (msg->h.numAnswers >= recIndex)
    {
        ptr = LocateAnswers(msg, end);
    }
    else if (msg->h.numAnswers + msg->h.numAuthorities >= recIndex)
    {
        ptr = LocateAuthorities(msg, end);
        i = msg->h.numAnswers;
    }
    else if (msg->h.numAnswers + msg->h.numAuthorities + msg->h.numAdditionals >= recIndex)
    {
        ptr = LocateAdditionals(msg, end);
        i = msg->h.numAnswers + msg->h.numAuthorities;
    }
    while (ptr && i++ < recIndex)
    {
        ptr = skipResourceRecord(msg, ptr, end);
    }
    if (ptr >= end)
    {
        ptr = mDNSNULL;
    }
    return ptr;
}

mDNSlocal CacheRecord *FindIdenticalRecordInCache(const mDNS *const m, const ResourceRecord *const pktrr)
{
    CacheGroup *cg = CacheGroupForRecord(m, pktrr);
    CacheRecord *rr;
    mDNSBool match;
    for (rr = cg ? cg->members : mDNSNULL; rr; rr=rr->next)
    {
        if (!pktrr->InterfaceID)
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            match = mdns_cache_metadata_same_dns_service(pktrr->metadata, rr->resrec.metadata);
#else
            const mDNSu32 id1 = (pktrr->rDNSServer ? pktrr->rDNSServer->resGroupID : 0);
            const mDNSu32 id2 = (rr->resrec.rDNSServer ? rr->resrec.rDNSServer->resGroupID : 0);
            match = (id1 == id2);
#endif
        }
        else match = (pktrr->InterfaceID == rr->resrec.InterfaceID);

        if (match && IdenticalSameNameRecord(pktrr, &rr->resrec)) break;
    }
    return(rr);
}
mDNSlocal void DeregisterProxyRecord(mDNS *const m, AuthRecord *const rr)
{
    rr->WakeUp.HMAC    = zeroEthAddr; // Clear HMAC so that mDNS_Deregister_internal doesn't waste packets trying to wake this host
    rr->RequireGoodbye = mDNSfalse;   // and we don't want to send goodbye for it
    mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
    SetSPSProxyListChanged(m->rec.r.resrec.InterfaceID);
}

mDNSlocal void ClearKeepaliveProxyRecords(mDNS *const m, const OwnerOptData *const owner, AuthRecord *const thelist, const mDNSInterfaceID InterfaceID)
{
    if (m->CurrentRecord)
        LogMsg("ClearKeepaliveProxyRecords ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
    m->CurrentRecord = thelist;

    // Normally, the RDATA of the keepalive record will be different each time and hence we always
    // clean up the keepalive record.
    while (m->CurrentRecord)
    {
        AuthRecord *const rr = m->CurrentRecord;
        if (InterfaceID == rr->resrec.InterfaceID && mDNSSameEthAddress(&owner->HMAC, &rr->WakeUp.HMAC))
        {
            if (mDNS_KeepaliveRecord(&m->rec.r.resrec))
            {
                LogSPS("ClearKeepaliveProxyRecords: Removing %3d H-MAC %.6a I-MAC %.6a %d %d %s",
                       m->ProxyRecords, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, rr->WakeUp.seq, owner->seq, ARDisplayString(m, rr));
                DeregisterProxyRecord(m, rr);
            }
        }
        // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
        // new records could have been added to the end of the list as a result of that call.
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

// Called from mDNSCoreReceiveUpdate when we get a sleep proxy registration request,
// to check our lists and discard any stale duplicates of this record we already have
mDNSlocal void ClearIdenticalProxyRecords(mDNS *const m, const OwnerOptData *const owner, AuthRecord *const thelist)
{
    if (m->CurrentRecord)
        LogMsg("ClearIdenticalProxyRecords ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
    m->CurrentRecord = thelist;
    while (m->CurrentRecord)
    {
        AuthRecord *const rr = m->CurrentRecord;
        if (m->rec.r.resrec.InterfaceID == rr->resrec.InterfaceID && mDNSSameEthAddress(&owner->HMAC, &rr->WakeUp.HMAC))
            if (IdenticalResourceRecord(&rr->resrec, &m->rec.r.resrec))
            {
                LogSPS("ClearIdenticalProxyRecords: Removing %3d H-MAC %.6a I-MAC %.6a %d %d %s",
                       m->ProxyRecords, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, rr->WakeUp.seq, owner->seq, ARDisplayString(m, rr));
                DeregisterProxyRecord(m, rr);
            }
        // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
        // new records could have been added to the end of the list as a result of that call.
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

// Called from ProcessQuery when we get an mDNS packet with an owner record in it
mDNSlocal void ClearProxyRecords(mDNS *const m, const OwnerOptData *const owner, AuthRecord *const thelist)
{
    if (m->CurrentRecord)
        LogMsg("ClearProxyRecords ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
    m->CurrentRecord = thelist;
    while (m->CurrentRecord)
    {
        AuthRecord *const rr = m->CurrentRecord;
        if (m->rec.r.resrec.InterfaceID == rr->resrec.InterfaceID && mDNSSameEthAddress(&owner->HMAC, &rr->WakeUp.HMAC))
            if (owner->seq != rr->WakeUp.seq || m->timenow - rr->TimeRcvd > mDNSPlatformOneSecond * 60)
            {
                if (rr->AddressProxy.type == mDNSAddrType_IPv6)
                {
                    // We don't do this here because we know that the host is waking up at this point, so we don't send
                    // Unsolicited Neighbor Advertisements -- even Neighbor Advertisements agreeing with what the host should be
                    // saying itself -- because it can cause some IPv6 stacks to falsely conclude that there's an address conflict.
                    #if defined(MDNS_USE_Unsolicited_Neighbor_Advertisements) && MDNS_USE_Unsolicited_Neighbor_Advertisements
                    LogSPS("NDP Announcement -- Releasing traffic for H-MAC %.6a I-MAC %.6a %s",
                           &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m,rr));
                    SendNDP(m, NDP_Adv, NDP_Override, rr, &rr->AddressProxy.ip.v6, &rr->WakeUp.IMAC, &AllHosts_v6, &AllHosts_v6_Eth);
                    #endif
                }
                LogSPS("ClearProxyRecords: Removing %3d AC %2d %02X H-MAC %.6a I-MAC %.6a %d %d %s",
                       m->ProxyRecords, rr->AnnounceCount, rr->resrec.RecordType,
                       &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, rr->WakeUp.seq, owner->seq, ARDisplayString(m, rr));
                if (rr->resrec.RecordType == kDNSRecordTypeDeregistering) rr->resrec.RecordType = kDNSRecordTypeShared;
                rr->WakeUp.HMAC = zeroEthAddr;  // Clear HMAC so that mDNS_Deregister_internal doesn't waste packets trying to wake this host
                rr->RequireGoodbye = mDNSfalse; // and we don't want to send goodbye for it, since real host is now back and functional
                mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
                SetSPSProxyListChanged(m->rec.r.resrec.InterfaceID);
            }
        // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
        // new records could have been added to the end of the list as a result of that call.
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

// ProcessQuery examines a received query to see if we have any answers to give
mDNSlocal mDNSu8 *ProcessQuery(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
                               const mDNSAddr *srcaddr, const mDNSInterfaceID InterfaceID, mDNSBool LegacyQuery, mDNSBool QueryWasMulticast,
                               mDNSBool QueryWasLocalUnicast, DNSMessage *const response, mDNSBool *const outHasResponse)
{
    const mDNSBool FromLocalSubnet   = mDNS_AddressIsLocalSubnet(m, InterfaceID, srcaddr);
    AuthRecord   *ResponseRecords    = mDNSNULL;
    AuthRecord  **nrp                = &ResponseRecords;

#if POOF_ENABLED
    mDNSBool    notD2D = !mDNSPlatformInterfaceIsD2D(InterfaceID);  // We don't run the POOF algorithm on D2D interfaces.
    CacheRecord  *ExpectedAnswers    = mDNSNULL;            // Records in our cache we expect to see updated
    CacheRecord **eap                = &ExpectedAnswers;
#endif // POOF_ENABLED

    DNSQuestion  *DupQuestions       = mDNSNULL;            // Our questions that are identical to questions in this packet
    DNSQuestion **dqp                = &DupQuestions;
    mDNSs32 delayresponse      = 0;
    mDNSBool SendLegacyResponse = mDNSfalse;
    const mDNSu8 *ptr;
    mDNSu8       *responseptr        = mDNSNULL;
    AuthRecord   *rr;
    TSRDataRecHead tsrs = SLIST_HEAD_INITIALIZER(tsrs);
    const TSROptData *curTSRForName = mDNSNULL;
    int i;
    mdns_assign(outHasResponse, mDNSfalse);

    // ***
    // *** 1. Look in Additional Section for an OPT record
    // ***
    ptr = LocateOptRR(query, end, Min(DNSOpt_OwnerData_ID_Space, DNSOpt_TSRData_Space));
    if (ptr)
    {
        ptr = GetLargeResourceRecord(m, query, ptr, end, InterfaceID, kDNSRecordTypePacketAdd, &m->rec);
        if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative && m->rec.r.resrec.rrtype == kDNSType_OPT)
        {
            const rdataOPT *opt;
            const rdataOPT *const e = (const rdataOPT *)&m->rec.r.resrec.rdata->u.data[m->rec.r.resrec.rdlength];
            mDNSu8 tsrsCount = 0;
            for (opt = &m->rec.r.resrec.rdata->u.opt[0]; opt < e; opt++)
            {
                // Find owner sub-option(s). We verify that the MAC is non-zero, otherwise we could inadvertently
                // delete all our own AuthRecords (which are identified by having zero MAC tags on them).
                if (opt->opt == kDNSOpt_Owner && opt->u.owner.vers == 0 && opt->u.owner.HMAC.l[0])
                {
                    ClearProxyRecords(m, &opt->u.owner, m->DuplicateRecords);
                    ClearProxyRecords(m, &opt->u.owner, m->ResourceRecords);
                }
                else if (opt->opt == kDNSOpt_TSR)
                {
                    tsrsCount++;
                    const mDNSu8 *name_ptr;
                    if ((name_ptr = DomainNamePtrAtTSRIndex(query, end, opt->u.tsr.recIndex)))
                    {
                        struct TSRDataRec *newTSR = TSRDataRecCreate(query, name_ptr, end, opt);
                        if (!newTSR)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                                "ProcessQuery: Create TSR(%u) failed - if %p tsrTime %d tsrHost %x recIndex %d",
                                tsrsCount, m->rec.r.resrec.InterfaceID, opt->u.tsr.timeStamp, opt->u.tsr.hostkeyHash,
                                opt->u.tsr.recIndex);
                            continue;
                        }
                        SLIST_INSERT_HEAD(&tsrs, newTSR, entries);
                    }
                    else
                    {
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                            "ProcessQuery: No Domain Name for TSR(%u) if %p tsrTime %d tsrHost %x recIndex %d",
                            tsrsCount, m->rec.r.resrec.InterfaceID, opt->u.tsr.timeStamp, opt->u.tsr.hostkeyHash,
                            opt->u.tsr.recIndex);
                    }
                }
            }
            if (!SLIST_EMPTY(&tsrs))
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG,
                    "ProcessQuery: Received TSR(%u) if %p " PUB_S,
                    tsrsCount, m->rec.r.resrec.InterfaceID, RRDisplayString(m, &m->rec.r.resrec));
            }
        }
        mDNSCoreResetRecord(m);
    }

    // ***
    // *** 2. Parse Question Section and mark potential answers
    // ***
    ptr = query->data;
    for (i=0; i<query->h.numQuestions; i++)                     // For each question...
    {
        mDNSBool QuestionNeedsMulticastResponse;
        int NumAnswersForThisQuestion = 0;
        AuthRecord *NSECAnswer = mDNSNULL;
        DNSQuestion pktq, *q;
        ptr = getQuestion(query, ptr, end, InterfaceID, &pktq); // get the question...
        if (!ptr) goto exit;

        // The only queries that *need* a multicast response are:
        // * Queries sent via multicast
        // * from port 5353
        // * that don't have the kDNSQClass_UnicastResponse bit set
        // These queries need multicast responses because other clients will:
        // * suppress their own identical questions when they see these questions, and
        // * expire their cache records if they don't see the expected responses
        // For other queries, we may still choose to send the occasional multicast response anyway,
        // to keep our neighbours caches warm, and for ongoing conflict detection.
        QuestionNeedsMulticastResponse = QueryWasMulticast && !LegacyQuery && !(pktq.qclass & kDNSQClass_UnicastResponse);

        if (pktq.qclass & kDNSQClass_UnicastResponse)
            m->mDNSStats.UnicastBitInQueries++;
        else
            m->mDNSStats.NormalQueries++;

        // Clear the UnicastResponse flag -- don't want to confuse the rest of the code that follows later
        pktq.qclass &= ~kDNSQClass_UnicastResponse;

        // Note: We use the m->CurrentRecord mechanism here because calling ResolveSimultaneousProbe
        // can result in user callbacks which may change the record list and/or question list.
        // Also note: we just mark potential answer records here, without trying to build the
        // "ResponseRecords" list, because we don't want to risk user callbacks deleting records
        // from that list while we're in the middle of trying to build it.
        if (m->CurrentRecord)
            LogMsg("ProcessQuery ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
        m->CurrentRecord = m->ResourceRecords;
        while (m->CurrentRecord)
        {
            rr = m->CurrentRecord;
            m->CurrentRecord = rr->next;
            if (AnyTypeRecordAnswersQuestion(rr, &pktq) && (QueryWasMulticast || QueryWasLocalUnicast || rr->AllowRemoteQuery))
            {
                m->mDNSStats.MatchingAnswersForQueries++;

                const mDNSBool typeMatches = RRTypeAnswersQuestionType(&rr->resrec, pktq.qtype, kRRTypeAnswersQuestionTypeFlagsNone);
                if (typeMatches)
                {
                    curTSRForName = TSRForNameFromDataRec(&tsrs, rr->resrec.name);
                    if (rr->resrec.RecordType == kDNSRecordTypeUnique)
                        ResolveSimultaneousProbe(m, query, end, &pktq, rr, curTSRForName);
                    else if (ProbeHasIdenticalRR(m, query, end, &pktq, rr))
                    {
                        // Don't include this rr in response if this is a probe, and it's authority section has an identical RR.
                        continue;
                    }
                    else if (ProbeRRMatchAndTSRCheck(m, query, end, &pktq, rr, curTSRForName))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                            "ProcessQuery - deregistering " PRI_DM_NAME " type " PUB_S " on interface id: %p due to TSR conflict",
                            DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype), rr->resrec.InterfaceID);
                        mDNS_Deregister_internal(m, rr, mDNS_Dereg_stale);
                        continue;
                    }
                    else if (ResourceRecordIsValidAnswer(rr))
                    {
                        NumAnswersForThisQuestion++;

                        // Note: We should check here if this is a probe-type query, and if so, generate an immediate
                        // unicast answer back to the source, because timeliness in answering probes is important.

                        // Notes:
                        // NR_AnswerTo pointing into query packet means "answer via immediate legacy unicast" (may *also* choose to multicast)
                        // NR_AnswerTo == NR_AnswerUnicast   means "answer via delayed unicast" (to modern querier; may promote to multicast instead)
                        // NR_AnswerTo == NR_AnswerMulticast means "definitely answer via multicast" (can't downgrade to unicast later)
                        // If we're not multicasting this record because the kDNSQClass_UnicastResponse bit was set,
                        // but the multicast querier is not on a matching subnet (e.g. because of overlaid subnets on one link)
                        // then we'll multicast it anyway (if we unicast, the receiver will ignore it because it has an apparently non-local source)
                        if (QuestionNeedsMulticastResponse || (!FromLocalSubnet && QueryWasMulticast && !LegacyQuery))
                        {
                            // We only mark this question for sending if it is at least one second since the last time we multicast it
                            // on this interface. If it is more than a second, or LastMCInterface is different, then we may multicast it.
                            // This is to guard against the case where someone blasts us with queries as fast as they can.
                            if ((mDNSu32)(m->timenow - rr->LastMCTime) >= (mDNSu32)mDNSPlatformOneSecond ||
                                (rr->LastMCInterface != mDNSInterfaceMark && rr->LastMCInterface != InterfaceID))
                                rr->NR_AnswerTo = NR_AnswerMulticast;
                        }
                        else if (!rr->NR_AnswerTo) rr->NR_AnswerTo = LegacyQuery ? ptr : NR_AnswerUnicast;
                    }
                }
                else if ((rr->resrec.RecordType & kDNSRecordTypeActiveUniqueMask) && ResourceRecordIsValidAnswer(rr))
                {
                    // If we don't have any answers for this question, but we do own another record with the same name,
                    // then we'll want to mark it to generate an NSEC record on this interface
                    if (!NSECAnswer) NSECAnswer = rr;
                }
            }
        }

        if (NumAnswersForThisQuestion == 0 && NSECAnswer)
        {
            NumAnswersForThisQuestion++;
            NSECAnswer->SendNSECNow = InterfaceID;
            m->NextScheduledResponse = m->timenow;
        }

        // If we couldn't answer this question, someone else might be able to,
        // so use random delay on response to reduce collisions
        if (NumAnswersForThisQuestion == 0) delayresponse = mDNSPlatformOneSecond;  // Divided by 50 = 20ms

        if (query->h.flags.b[0] & kDNSFlag0_TC)
            m->mDNSStats.KnownAnswerMultiplePkts++;
        // We only do the following accelerated cache expiration and duplicate question suppression processing
        // for non-truncated multicast queries with multicast responses.
        // For any query generating a unicast response we don't do this because we can't assume we will see the response.
        // For truncated queries we don't do this because a response we're expecting might be suppressed by a subsequent
        // known-answer packet, and when there's packet loss we can't safely assume we'll receive *all* known-answer packets.
        if (QuestionNeedsMulticastResponse && !(query->h.flags.b[0] & kDNSFlag0_TC))
        {
#if POOF_ENABLED
            if (notD2D)
            {
                CacheGroup *cg = CacheGroupForName(m, pktq.qnamehash, &pktq.qname);
                CacheRecord *cr;

                // Make a list indicating which of our own cache records we expect to see updated as a result of this query
                // Note: Records larger than 1K are not habitually multicast, so don't expect those to be updated
                for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
                {
                    if (SameNameCacheRecordAnswersQuestion(cr, &pktq) && cr->resrec.rdlength <= SmallRecordLimit)
                    {
                        if (!cr->NextInKAList && eap != &cr->NextInKAList)
                        {
                            *eap = cr;
                            eap = &cr->NextInKAList;
                        }
                    }
                }
            }
#endif // POOF_ENABLED

            // Check if this question is the same as any of mine.
            // We only do this for non-truncated queries. Right now it would be too complicated to try
            // to keep track of duplicate suppression state between multiple packets, especially when we
            // can't guarantee to receive all of the Known Answer packets that go with a particular query.
            for (q = m->Questions; q; q=q->next)
            {
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
                if (QuestionSendsMDNSQueriesViaUnicast(q))
                {
                    continue;
                }
#endif
                if (ActiveQuestion(q) && m->timenow - q->LastQTxTime > mDNSPlatformOneSecond / 4)
                {
                    if (!q->InterfaceID || q->InterfaceID == InterfaceID)
                    {
                        if (q->NextInDQList == mDNSNULL && dqp != &q->NextInDQList)
                        {
                            if (q->qtype == pktq.qtype &&
                                q->qclass == pktq.qclass &&
                                q->qnamehash == pktq.qnamehash && SameDomainName(&q->qname, &pktq.qname))
                            { *dqp = q; dqp = &q->NextInDQList; }
                        }
                    }
                }
            }
        }
    }

    // ***
    // *** 3. Now we can safely build the list of marked answers
    // ***
    for (rr = m->ResourceRecords; rr; rr=rr->next)              // Now build our list of potential answers
        if (rr->NR_AnswerTo)                                    // If we marked the record...
            AddRecordToResponseList(&nrp, rr, mDNSNULL);        // ... add it to the list

    // ***
    // *** 4. Add additional records
    // ***
    AddAdditionalsToResponseList(m, ResponseRecords, &nrp, InterfaceID);

    // ***
    // *** 5. Parse Answer Section and cancel any records disallowed by Known-Answer list
    // ***
    for (i=0; i<query->h.numAnswers; i++)                       // For each record in the query's answer section...
    {
        // Get the record...
        CacheRecord *ourcacherr;
        ptr = GetLargeResourceRecord(m, query, ptr, end, InterfaceID, kDNSRecordTypePacketAns, &m->rec);
        if (!ptr) goto exit;
        if (m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative)
        {
            // See if this Known-Answer suppresses any of our currently planned answers
            for (rr=ResponseRecords; rr; rr=rr->NextResponse)
            {
                if (MustSendRecord(rr) && ShouldSuppressKnownAnswer(&m->rec.r, rr))
                {
                    m->mDNSStats.KnownAnswerSuppressions++;
                    rr->NR_AnswerTo = mDNSNULL;
                    rr->NR_AdditionalTo = mDNSNULL;
                }
            }

            // See if this Known-Answer suppresses any previously scheduled answers (for multi-packet KA suppression)
            for (rr=m->ResourceRecords; rr; rr=rr->next)
            {
                // If we're planning to send this answer on this interface, and only on this interface, then allow KA suppression
                if (rr->ImmedAnswer == InterfaceID && ShouldSuppressKnownAnswer(&m->rec.r, rr))
                {
                    if (srcaddr->type == mDNSAddrType_IPv4)
                    {
                        if (mDNSSameIPv4Address(rr->v4Requester, srcaddr->ip.v4)) rr->v4Requester = zerov4Addr;
                    }
                    else if (srcaddr->type == mDNSAddrType_IPv6)
                    {
                        if (mDNSSameIPv6Address(rr->v6Requester, srcaddr->ip.v6)) rr->v6Requester = zerov6Addr;
                    }
                    if (mDNSIPv4AddressIsZero(rr->v4Requester) && mDNSIPv6AddressIsZero(rr->v6Requester))
                    {
                        m->mDNSStats.KnownAnswerSuppressions++;
                        rr->ImmedAnswer  = mDNSNULL;
                        rr->ImmedUnicast = mDNSfalse;
    #if MDNS_LOG_ANSWER_SUPPRESSION_TIMES
                        LogMsg("Suppressed after%4d: %s", m->timenow - rr->ImmedAnswerMarkTime, ARDisplayString(m, rr));
    #endif
                    }
                }
            }

            ourcacherr = FindIdenticalRecordInCache(m, &m->rec.r.resrec);

#if POOF_ENABLED
            if (notD2D)
            {
                // Having built our ExpectedAnswers list from the questions in this packet, we then remove
                // any records that are suppressed by the Known Answer list in this packet.
                eap = &ExpectedAnswers;
                while (*eap)
                {
                    CacheRecord *cr = *eap;
                    if (cr->resrec.InterfaceID == InterfaceID && IdenticalResourceRecord(&m->rec.r.resrec, &cr->resrec))
                    { *eap = cr->NextInKAList; cr->NextInKAList = mDNSNULL; }
                    else eap = &cr->NextInKAList;
                }
            }
#endif // POOF_ENABLED

            // See if this Known-Answer is a surprise to us. If so, we shouldn't suppress our own query.
            if (!ourcacherr)
            {
                dqp = &DupQuestions;
                while (*dqp)
                {
                    DNSQuestion *q = *dqp;
                    if (CacheRecordAnswersQuestion(&m->rec.r, q))
                    { *dqp = q->NextInDQList; q->NextInDQList = mDNSNULL; }
                    else dqp = &q->NextInDQList;
                }
            }
        }
        mDNSCoreResetRecord(m);
    }

    // ***
    // *** 6. Cancel any additionals that were added because of now-deleted records
    // ***
    for (rr=ResponseRecords; rr; rr=rr->NextResponse)
        if (rr->NR_AdditionalTo && !MustSendRecord(rr->NR_AdditionalTo))
        { rr->NR_AnswerTo = mDNSNULL; rr->NR_AdditionalTo = mDNSNULL; }

    // ***
    // *** 7. Mark the send flags on the records we plan to send
    // ***
    for (rr=ResponseRecords; rr; rr=rr->NextResponse)
    {
        if (rr->NR_AnswerTo)
        {
            mDNSBool SendMulticastResponse = mDNSfalse;     // Send modern multicast response
            mDNSBool SendUnicastResponse   = mDNSfalse;     // Send modern unicast response (not legacy unicast response)

            // If it's been one TTL/4 since we multicast this, then send a multicast response
            // for conflict detection, etc.
            if ((mDNSu32)(m->timenow - rr->LastMCTime) >= (mDNSu32)TicksTTL(rr)/4
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
                && QueryWasMulticast
#endif
                )
            {
                SendMulticastResponse = mDNStrue;
                // If this record was marked for modern (delayed) unicast response, then mark it as promoted to
                // multicast response instead (don't want to end up ALSO setting SendUnicastResponse in the check below).
                // If this record was marked for legacy unicast response, then we mustn't change the NR_AnswerTo value.
                if (rr->NR_AnswerTo == NR_AnswerUnicast)
                {
                    m->mDNSStats.UnicastDemotedToMulticast++;
                    rr->NR_AnswerTo = NR_AnswerMulticast;
                }
            }

            // If the client insists on a multicast response, then we'd better send one
            if      (rr->NR_AnswerTo == NR_AnswerMulticast)
            {
                m->mDNSStats.MulticastResponses++;
                SendMulticastResponse = mDNStrue;
            }
            else if (rr->NR_AnswerTo == NR_AnswerUnicast)
            {
                m->mDNSStats.UnicastResponses++;
                SendUnicastResponse   = mDNStrue;
            }
            else if (rr->NR_AnswerTo)
            {
                SendLegacyResponse    = mDNStrue;
            }

            if (SendMulticastResponse || SendUnicastResponse)
            {
#if MDNS_LOG_ANSWER_SUPPRESSION_TIMES
                rr->ImmedAnswerMarkTime = m->timenow;
#endif
                m->NextScheduledResponse = m->timenow;
                // If we're already planning to send this on another interface, just send it on all interfaces
                if (rr->ImmedAnswer && rr->ImmedAnswer != InterfaceID)
                    rr->ImmedAnswer = mDNSInterfaceMark;
                else
                {
                    rr->ImmedAnswer = InterfaceID;          // Record interface to send it on
                    if (SendUnicastResponse) rr->ImmedUnicast = mDNStrue;
                    if (srcaddr->type == mDNSAddrType_IPv4)
                    {
                        if      (mDNSIPv4AddressIsZero(rr->v4Requester)) rr->v4Requester = srcaddr->ip.v4;
                        else if (!mDNSSameIPv4Address(rr->v4Requester, srcaddr->ip.v4)) rr->v4Requester = onesIPv4Addr;
                    }
                    else if (srcaddr->type == mDNSAddrType_IPv6)
                    {
                        if      (mDNSIPv6AddressIsZero(rr->v6Requester)) rr->v6Requester = srcaddr->ip.v6;
                        else if (!mDNSSameIPv6Address(rr->v6Requester, srcaddr->ip.v6)) rr->v6Requester = onesIPv6Addr;
                    }
                }
                // We have responses for this query.
                mdns_assign(outHasResponse, mDNStrue);
            }
            // If TC flag is set, it means we should expect that additional known answers may be coming in another packet,
            // so we allow roughly half a second before deciding to reply (we've observed inter-packet delays of 100-200ms on 802.11)
            // else, if record is a shared one, spread responses over 100ms to avoid implosion of simultaneous responses
            // else, for a simple unique record reply, we can reply immediately; no need for delay
            if      (query->h.flags.b[0] & kDNSFlag0_TC) delayresponse = mDNSPlatformOneSecond * 20;            // Divided by 50 = 400ms
            else if (rr->resrec.RecordType == kDNSRecordTypeShared) delayresponse = mDNSPlatformOneSecond;      // Divided by 50 = 20ms
        }
        else if (rr->NR_AdditionalTo && rr->NR_AdditionalTo->NR_AnswerTo == NR_AnswerMulticast)
        {
            // Since additional records are an optimization anyway, we only ever send them on one interface at a time
            // If two clients on different interfaces do queries that invoke the same optional additional answer,
            // then the earlier client is out of luck
            rr->ImmedAdditional = InterfaceID;
            // No need to set m->NextScheduledResponse here
            // We'll send these additional records when we send them, or not, as the case may be
        }
    }

    // ***
    // *** 8. If we think other machines are likely to answer these questions, set our response suppression timer,
    // ***    unless we're shutting down. While shutting down, we don't want to delay goodbyes for our auth records.
    // ***
    if (delayresponse && !m->ShutdownTime && (!m->SuppressResponses || ((m->SuppressResponses - m->timenow) < ((delayresponse + 49) / 50))))
    {
#if MDNS_LOG_ANSWER_SUPPRESSION_TIMES
        const mDNSBool alreadySuppressing = (m->SuppressResponses != 0);
        if (alreadySuppressing)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "Current SuppressResponses delay %5d; require %5d", m->SuppressResponses - m->timenow, (delayresponse + 49) / 50);
        }
#endif
        // Pick a random delay:
        // We start with the base delay chosen above (typically either 1 second or 20 seconds),
        // and add a random value in the range 0-5 seconds (making 1-6 seconds or 20-25 seconds).
        // This is an integer value, with resolution determined by the platform clock rate.
        // We then divide that by 50 to get the delay value in ticks. We defer the division until last
        // to get better results on platforms with coarse clock granularity (e.g. ten ticks per second).
        // The +49 before dividing is to ensure we round up, not down, to ensure that even
        // on platforms where the native clock rate is less than fifty ticks per second,
        // we still guarantee that the final calculated delay is at least one platform tick.
        // We want to make sure we don't ever allow the delay to be zero ticks,
        // because if that happens we'll fail the Bonjour Conformance Test.
        // Our final computed delay is 20-120ms for normal delayed replies,
        // or 400-500ms in the case of multi-packet known-answer lists.
        m->SuppressResponses = NonZeroTime(m->timenow + ((delayresponse + (mDNSs32)mDNSRandom((mDNSu32)mDNSPlatformOneSecond * 5) + 49) / 50));
#if MDNS_LOG_ANSWER_SUPPRESSION_TIMES
        if (alreadySuppressing)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "Set SuppressResponses to %d", m->SuppressResponses - m->timenow);
        }
#endif
    }

    // ***
    // *** 9. If query is from a legacy client, or from a new client requesting a unicast reply, then generate a unicast response too
    // ***
    if (SendLegacyResponse)
        responseptr = GenerateUnicastResponse(query, end, InterfaceID, LegacyQuery, response, ResponseRecords);

exit:
    mDNSCoreResetRecord(m);

    // ***
    // *** 10. Finally, clear our link chains ready for use next time
    // ***
    while (ResponseRecords)
    {
        rr = ResponseRecords;
        ResponseRecords = rr->NextResponse;
        rr->NextResponse    = mDNSNULL;
        rr->NR_AnswerTo     = mDNSNULL;
        rr->NR_AdditionalTo = mDNSNULL;
    }

#if POOF_ENABLED
    while (ExpectedAnswers && notD2D)
    {
        CacheRecord *cr = ExpectedAnswers;
        ExpectedAnswers = cr->NextInKAList;
        cr->NextInKAList = mDNSNULL;

        // For non-truncated queries, we can definitively say that we should expect
        // to be seeing a response for any records still left in the ExpectedAnswers list
        if (!(query->h.flags.b[0] & kDNSFlag0_TC))
            if (cr->UnansweredQueries == 0 || m->timenow - cr->LastUnansweredTime >= mDNSPlatformOneSecond * 3/4)
            {
                cr->UnansweredQueries++;
                cr->LastUnansweredTime = m->timenow;
                if (cr->UnansweredQueries > 1)
                        debugf("ProcessQuery: UnansweredQueries %lu %s", cr->UnansweredQueries, CRDisplayString(m, cr));
                SetNextCacheCheckTimeForRecord(m, cr);
            }

        // If we've seen multiple unanswered queries for this record,
        // then mark it to expire in five seconds if we don't get a response by then.
        if (cr->UnansweredQueries >= MaxUnansweredQueries)
        {
            // Only show debugging message if this record was not about to expire anyway
            if (RRExpireTime(cr) - m->timenow > (mDNSs32) kDefaultReconfirmTimeForNoAnswer * 4 / 3 + mDNSPlatformOneSecond)
                    LogInfo("ProcessQuery: UnansweredQueries %lu interface %lu TTL %lu mDNS_Reconfirm() for %s",
                       cr->UnansweredQueries, InterfaceID, (RRExpireTime(cr) - m->timenow + mDNSPlatformOneSecond-1) / mDNSPlatformOneSecond, CRDisplayString(m, cr));

            m->mDNSStats.PoofCacheDeletions++;
            mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
        }
    }
#endif // POOF_ENABLED

    while (DupQuestions)
    {
        DNSQuestion *q = DupQuestions;
        DupQuestions = q->NextInDQList;
        q->NextInDQList = mDNSNULL;
        RecordDupSuppressInfo(q, m->timenow, InterfaceID, srcaddr->type);
        debugf("ProcessQuery: Recorded DSI for %##s (%s) on %p/%s", q->qname.c, DNSTypeName(q->qtype), InterfaceID,
               srcaddr->type == mDNSAddrType_IPv4 ? "v4" : "v6");
    }
    TSRDataRecHeadFreeList(&tsrs);
    return(responseptr);
}

mDNSlocal void mDNSCoreReceiveQuery(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end,
                                    const mDNSAddr *srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, mDNSIPPort dstport,
                                    const mDNSInterfaceID InterfaceID)
{
    mDNSu8    *responseend = mDNSNULL;
    mDNSBool QueryWasLocalUnicast = srcaddr && dstaddr &&
                                    !mDNSAddrIsDNSMulticast(dstaddr) && mDNS_AddressIsLocalSubnet(m, InterfaceID, srcaddr);

    if (!dstaddr || (!InterfaceID && mDNSAddrIsDNSMulticast(dstaddr)))
    {
        const char *const reason = !dstaddr ? "Received over TCP connection" : "Multicast, but no InterfaceID";
        LogMsg("Ignoring Query from %#-15a:%-5d to %#-15a:%-5d on 0x%p with "
               "%2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s %d bytes (%s)",
               srcaddr, mDNSVal16(srcport), dstaddr, mDNSVal16(dstport), InterfaceID,
               msg->h.numQuestions,   msg->h.numQuestions   == 1 ? ", "   : "s,",
               msg->h.numAnswers,     msg->h.numAnswers     == 1 ? ", "   : "s,",
               msg->h.numAuthorities, msg->h.numAuthorities == 1 ? "y,  " : "ies,",
               msg->h.numAdditionals, msg->h.numAdditionals == 1 ? " "    : "s", end - msg->data, reason);
        return;
    }

    verbosedebugf("Received Query from %#-15a:%-5d to %#-15a:%-5d on 0x%p with "
                  "%2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s %d bytes",
                  srcaddr, mDNSVal16(srcport), dstaddr, mDNSVal16(dstport), InterfaceID,
                  msg->h.numQuestions,   msg->h.numQuestions   == 1 ? ", "   : "s,",
                  msg->h.numAnswers,     msg->h.numAnswers     == 1 ? ", "   : "s,",
                  msg->h.numAuthorities, msg->h.numAuthorities == 1 ? "y,  " : "ies,",
                  msg->h.numAdditionals, msg->h.numAdditionals == 1 ? " "    : "s", end - msg->data);

    mDNSBool hasResponse = mDNSfalse;
    responseend = ProcessQuery(m, msg, end, srcaddr, InterfaceID,
        !mDNSSameIPPort(srcport, MulticastDNSPort), mDNSAddrIsDNSMulticast(dstaddr), QueryWasLocalUnicast, &m->omsg,
        &hasResponse);

    if (hasResponse)
    {
        DumpPacket(mStatus_NoError, mDNSfalse, "N/A", srcaddr, srcport, dstaddr, dstport, msg, end, InterfaceID);
    }

    if (responseend)    // If responseend is non-null, that means we built a unicast response packet
    {
        debugf("Unicast Response: %d Question%s, %d Answer%s, %d Additional%s to %#-15a:%d on %p/%ld",
               m->omsg.h.numQuestions,   m->omsg.h.numQuestions   == 1 ? "" : "s",
               m->omsg.h.numAnswers,     m->omsg.h.numAnswers     == 1 ? "" : "s",
               m->omsg.h.numAdditionals, m->omsg.h.numAdditionals == 1 ? "" : "s",
               srcaddr, mDNSVal16(srcport), InterfaceID, srcaddr->type);
        mDNSSendDNSMessage(m, &m->omsg, responseend, InterfaceID, mDNSNULL, mDNSNULL, srcaddr, srcport, mDNSNULL, mDNSfalse);
    }
}

#if 0
mDNSlocal mDNSBool TrustedSource(const mDNS *const m, const mDNSAddr *const srcaddr)
{
    DNSServer *s;
    (void)m; // Unused
    (void)srcaddr; // Unused
    for (s = m->DNSServers; s; s = s->next)
        if (mDNSSameAddress(srcaddr, &s->addr)) return(mDNStrue);
    return(mDNSfalse);
}
#endif

struct UDPSocket_struct
{
    mDNSIPPort port; // MUST BE FIRST FIELD -- mDNSCoreReceive expects every UDPSocket_struct to begin with mDNSIPPort port
};

mDNSlocal DNSQuestion *ExpectingUnicastResponseForQuestion(const mDNS *const m, const mDNSIPPort port,
    const mDNSOpaque16 id, const DNSQuestion *const question, mDNSBool tcp)
{
    DNSQuestion *q;
    for (q = m->Questions; q; q=q->next)
    {
        if (!tcp && !q->LocalSocket) continue;
        if (mDNSSameIPPort(tcp ? q->tcpSrcPort : q->LocalSocket->port, port)       &&
            q->qtype                  == question->qtype     &&
            q->qclass                 == question->qclass    &&
            q->qnamehash              == question->qnamehash &&
            SameDomainName(&q->qname, &question->qname))
        {
            if (mDNSSameOpaque16(q->TargetQID, id)) return(q);
            else
            {
                return(mDNSNULL);
            }
        }
    }
    return(mDNSNULL);
}

// This function is called when we receive a unicast response. This could be the case of a unicast response from the
// DNS server or a response to the QU query. Hence, the cache record's InterfaceId can be both NULL or non-NULL (QU case)
mDNSlocal DNSQuestion *ExpectingUnicastResponseForRecord(mDNS *const m,
                                                         const mDNSAddr *const srcaddr, const mDNSBool SrcLocal, const mDNSIPPort port, const mDNSOpaque16 id, const CacheRecord *const rr, mDNSBool tcp)
{
    DNSQuestion *q;
    (void)id;

    for (q = m->Questions; q; q=q->next)
    {
        if (!q->DuplicateOf && ResourceRecordAnswersUnicastResponse(&rr->resrec, q))
        {
            if (!mDNSOpaque16IsZero(q->TargetQID))
            {
                debugf("ExpectingUnicastResponseForRecord msg->h.id %d q->TargetQID %d for %s", mDNSVal16(id), mDNSVal16(q->TargetQID), CRDisplayString(m, rr));

                if (mDNSSameOpaque16(q->TargetQID, id))
                {
                    mDNSIPPort srcp;
                    if (!tcp)
                    {
                        srcp = q->LocalSocket ? q->LocalSocket->port : zeroIPPort;
                    }
                    else
                    {
                        srcp = q->tcpSrcPort;
                    }
                    if (mDNSSameIPPort(srcp, port)) return(q);

                    //  if (mDNSSameAddress(srcaddr, &q->Target))                   return(mDNStrue);
                    //  if (q->LongLived && mDNSSameAddress(srcaddr, &q->servAddr)) return(mDNStrue); Shouldn't need this now that we have LLQType checking
                    //  if (TrustedSource(m, srcaddr))                              return(mDNStrue);
                    LogInfo("WARNING: Ignoring suspect uDNS response for %##s (%s) from %#a:%d %s",
                            q->qname.c, DNSTypeName(q->qtype), srcaddr, mDNSVal16(port), CRDisplayString(m, rr));
                    return(mDNSNULL);
                }
            }
            else
            {
                if (SrcLocal && q->ExpectUnicastResp && (mDNSu32)(m->timenow - q->ExpectUnicastResp) < (mDNSu32)(mDNSPlatformOneSecond*2))
                    return(q);
            }
        }
    }
    return(mDNSNULL);
}

// Certain data types need more space for in-memory storage than their in-packet rdlength would imply
// Currently this applies only to rdata types containing more than one domainname,
// or types where the domainname is not the last item in the structure.
mDNSlocal mDNSu16 GetRDLengthMem(const ResourceRecord *const rr)
{
    switch (rr->rrtype)
    {
    case kDNSType_SOA: return sizeof(rdataSOA);
    case kDNSType_RP:  return sizeof(rdataRP);
    case kDNSType_PX:  return sizeof(rdataPX);
    default:           return rr->rdlength;
    }
}

mDNSlocal void AddCacheRecordToCacheGroup(CacheGroup *const cg, CacheRecord *const cr)
{

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    if (resource_record_is_to_be_dnssec_validated(&cr->resrec))
    {
        // Insert the newly created cache record before the DNSSEC-aware CNAME records.
        // This operation ensures that mDNSResponder will deliver all the records including RRSIG or denial of
        // existence record before delivering CNAME and following it.
        CacheRecord *node = mDNSNULL;
        CacheRecord **ptr = &cg->members;
        while ((node = *ptr) != mDNSNULL)
        {
            // Only search for CNAME record that is marked as DNSSEC aware.
            if (node->resrec.rrtype == kDNSType_CNAME && resource_record_is_to_be_dnssec_validated(&node->resrec))
            {
                break;
            }
            ptr = &node->next;
        }

        // Insert the new record.
        cr->next = *ptr;
        *ptr = cr;

        // Update the tail pointer.
        if (ptr == cg->rrcache_tail)
        {
            cg->rrcache_tail = &(cr->next);
        }
    }
    else
#endif
    {
        *(cg->rrcache_tail) = cr;               // Append this record to tail of cache slot list
        cg->rrcache_tail = &(cr->next);         // Advance tail pointer
    }
}

mDNSlocal void AddOrUpdateTSRForCacheGroup(mDNS *const m, const TSROptData *curTSROpt, CacheGroup *const cg,
    CacheRecord *ourTsr, mDNSu32 ttl)
{
    mDNSs32 timestampContinuous;
    mDNSBool new = mDNSfalse;
    if (!getValidContinousTSRTime(&timestampContinuous, curTSROpt->timeStamp))
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
            "AddOrUpdateTSRForCacheGroup: tsrTimestamp[%u] out of range (%u) on TSR for " PRI_DM_NAME "",
            curTSROpt->timeStamp, MaxTimeSinceReceived, DM_NAME_PARAM(cg->name));
        return;
    }

    if (!ourTsr)
    {
        ourTsr = GetCacheRecord(m, cg, DNSOpt_TSRData_Space);
        if (!ourTsr)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR,
                "AddOrUpdateTSRForCacheGroup: No cache record for new TSR " PRI_DM_NAME, DM_NAME_PARAM(cg->name));
            return;
        }
        ourTsr->resrec.rrclass          = NormalMaxDNSMessageData;
        ourTsr->resrec.rrtype           = kDNSType_OPT;
        ourTsr->resrec.name             = cg->name;
        ourTsr->resrec.namehash         = cg->namehash;
        ourTsr->resrec.rdlength         = DNSOpt_TSRData_Space;
        ourTsr->resrec.rdestimate       = DNSOpt_TSRData_Space;
        AddCacheRecordToCacheGroup(cg, ourTsr);
        new = mDNStrue;
    }
    ourTsr->TimeRcvd                = m->timenow;
    ourTsr->resrec.rroriginalttl    = Max(ourTsr->resrec.rroriginalttl, ttl);

    rdataOPT * const rdata = &ourTsr->resrec.rdata->u.opt[0];
    if (new || timestampContinuous - rdata->u.tsr.timeStamp > 0) // Always enter if new
    {
        rdata->opt                  = kDNSOpt_TSR;
        rdata->optlen               = DNSOpt_TSRData_Space - 4;
        rdata->u.tsr.timeStamp      = timestampContinuous;
        rdata->u.tsr.hostkeyHash    = curTSROpt->hostkeyHash;
        rdata->u.tsr.recIndex       = 0;
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
            "AddOrUpdateTSRForCacheGroup: %s TSR " PRI_S, new ? "Added" : "Updated", CRDisplayString(m, ourTsr));
    }
}

mDNSexport CacheRecord * CreateNewCacheEntryEx(mDNS *const m, const mDNSu32 slot, CacheGroup *cg, const mDNSs32 delay,
    const mDNSBool add, const mDNSAddr *const sourceAddress, const CreateNewCacheEntryFlags flags)
{
    CacheRecord *rr = mDNSNULL;
    mDNSu16 RDLength = GetRDLengthMem(&m->rec.r.resrec);

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    const mDNSBool toBeDNSSECValidated = ((flags & kCreateNewCacheEntryFlagsDNSSECRRToValidate) != 0);
    const mDNSBool dnssecValidated = ((flags & kCreateNewCacheEntryFlagsDNSSECRRValidatedSecure) != 0)
                                        || ((flags & kCreateNewCacheEntryFlagsDNSSECRRValidatedInsecure) != 0);
    dnssec_result_t validation_result = dnssec_indeterminate;
    if (dnssecValidated)
    {
        if ((flags & kCreateNewCacheEntryFlagsDNSSECRRValidatedSecure) != 0)
        {
            validation_result = dnssec_secure;
        } else if ((flags & kCreateNewCacheEntryFlagsDNSSECRRValidatedInsecure) != 0)
        {
            validation_result = dnssec_insecure;
        }
    }
    if (toBeDNSSECValidated && dnssecValidated)
    {
        return mDNSNULL;
    }
#endif

    if (!m->rec.r.resrec.InterfaceID) debugf("CreateNewCacheEntry %s", CRDisplayString(m, &m->rec.r));

    //if (RDLength > InlineCacheRDSize)
    //  LogInfo("Rdata len %4d > InlineCacheRDSize %d %s", RDLength, InlineCacheRDSize, CRDisplayString(m, &m->rec.r));

    if (!cg) cg = GetCacheGroup(m, slot, &m->rec.r.resrec); // If we don't have a CacheGroup for this name, make one now
    if (cg) rr = GetCacheRecord(m, cg, RDLength);   // Make a cache record, being careful not to recycle cg
    if (!rr) NoCacheAnswer(m, &m->rec.r);
    else
    {
        RData *saveptr              = rr->resrec.rdata;     // Save the rr->resrec.rdata pointer
        *rr                         = m->rec.r;             // Block copy the CacheRecord object
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        mdns_retain_null_safe(rr->resrec.metadata);
#endif
        rr->resrec.rdata            = saveptr;              // Restore rr->resrec.rdata after the structure assignment
        rr->resrec.name             = cg->name;             // And set rr->resrec.name to point into our CacheGroup header
        rr->resrec.mortality        = Mortality_Mortal;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        dnssec_error_t err = DNSSEC_ERROR_NO_ERROR;
        if (toBeDNSSECValidated)
        {
            rr->resrec.dnssec = dnssec_obj_resource_record_member_create_to_validate(rr->resrec.RecordType != kDNSRecordTypePacketNegative, rr, &err);
        }
        else if (dnssecValidated)
        {
            if (validation_result != dnssec_insecure && validation_result != dnssec_secure)
            {
                return mDNSNULL;
            }
            const mDNSBool insecure_validation_usable = ((flags & kCreateNewCacheEntryFlagsDNSSECInsecureValidationUsable) != 0);
            rr->resrec.dnssec = dnssec_obj_resource_record_member_create_validated(rr, validation_result, insecure_validation_usable, &err);
        }
        if (err != DNSSEC_ERROR_NO_ERROR)
        {
            return mDNSNULL;
        }
#endif

        rr->DelayDelivery = delay;

        // If this is an oversized record with external storage allocated, copy rdata to external storage
        if      (rr->resrec.rdata == (RData*)&rr->smallrdatastorage && RDLength > InlineCacheRDSize)
            LogMsg("rr->resrec.rdata == &rr->rdatastorage but length > InlineCacheRDSize %##s", m->rec.r.resrec.name->c);
        else if (rr->resrec.rdata != (RData*)&rr->smallrdatastorage && RDLength <= InlineCacheRDSize)
            LogMsg("rr->resrec.rdata != &rr->rdatastorage but length <= InlineCacheRDSize %##s", m->rec.r.resrec.name->c);
        if (RDLength > InlineCacheRDSize)
            mDNSPlatformMemCopy(rr->resrec.rdata, m->rec.r.resrec.rdata, sizeofRDataHeader + RDLength);

        rr->next = mDNSNULL;                    // Clear 'next' pointer
        rr->soa  = mDNSNULL;

        if (sourceAddress)
            rr->sourceAddress = *sourceAddress;

        if (!rr->resrec.InterfaceID)
        {
            m->rrcache_totalused_unicast += rr->resrec.rdlength;
        }

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) || MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
        if (flags & kCreateNewCacheEntryFlagsDNSPushSubscribed)
        {
            rr->DNSPushSubscribed = mDNStrue;
        }
#else
        (void)flags;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
        rr->PrivacyLevel = mDNSCRLogPrivacyLevel_Default;
#endif

        if (add)
        {
            AddCacheRecordToCacheGroup(cg, rr);
            CacheRecordAdd(m, rr);  // CacheRecordAdd calls SetNextCacheCheckTimeForRecord(m, rr); for us
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
            if (mDNSAddressIsValidNonZero(&rr->sourceAddress)   &&
                !mDNSAddrIsDNSMulticast(&rr->sourceAddress)     &&
                mDNS_AddressIsLocalSubnet(m, rr->resrec.InterfaceID, &rr->sourceAddress))
            {
                unicast_assist_addr_add(rr->resrec.name, rr->resrec.namehash, rr->resrec.rrtype, rr->resrec.RecordType,
                    &rr->sourceAddress, rr->resrec.InterfaceID);
            }
#endif
        }
        else
        {
            // Can't use the "cg->name" if we are not adding to the cache as the
            // CacheGroup may be released anytime if it is empty
            domainname *name = (domainname *) mDNSPlatformMemAllocate(DomainNameLength(cg->name));
            if (name)
            {
                AssignDomainName(name, cg->name);
                rr->resrec.name   = name;
            }
            else
            {
                ReleaseCacheRecord(m, rr);
                NoCacheAnswer(m, &m->rec.r);
                rr = mDNSNULL;
            }
        }
    }
    return(rr);
}

mDNSexport CacheRecord *CreateNewCacheEntry(mDNS *const m, const mDNSu32 slot, CacheGroup *cg, const mDNSs32 delay,
                                            const mDNSBool add, const mDNSAddr *const sourceAddress)
{
    return CreateNewCacheEntryEx(m, slot, cg, delay, add, sourceAddress, kCreateNewCacheEntryFlagsNone);
}

mDNSlocal void RefreshCacheRecordCacheGroupOrder(CacheGroup *cg, CacheRecord *cr)
{   //  Move the cache record to the tail of the cache group to maintain a fresh ordering
    if (cg->rrcache_tail != &cr->next)          // If not already at the tail
    {
        CacheRecord **rp;
        for (rp = &cg->members; *rp; rp = &(*rp)->next)
        {
            if (*rp == cr)                      // This item points to this record
            {
                *rp = cr->next;                 // Remove this record
                break;
            }
        }
        cr->next = mDNSNULL;                    // This record is now last
        AddCacheRecordToCacheGroup(cg, cr);
    }
}

mDNSexport void RefreshCacheRecord(mDNS *const m, CacheRecord *const rr, const mDNSu32 ttl)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
    if (rr->CRActiveQuestion && mDNSOpaque16IsZero(rr->CRActiveQuestion->TargetQID))
    {
        const mDNSBool sourceAddrValidNonZero = mDNSAddressIsValidNonZero(&rr->sourceAddress);
        const mDNSBool sourceAddrIsDNSMulticast = mDNSAddrIsDNSMulticast(&rr->sourceAddress);
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST_ANALYTICS)
        if (rr->LastUnansweredTime != 0                 &&
            sourceAddrValidNonZero)
        {
            const bool is_assist    = (rr->unicastAssistSent != mDNSfalse);
            const bool is_unicast   = (rr->UnansweredQueries == 0 && !sourceAddrIsDNSMulticast);
            dnssd_analytics_update_unicast_assist(is_assist, is_unicast);
        }
#endif
        if (sourceAddrValidNonZero                      &&
            !sourceAddrIsDNSMulticast                   &&
            mDNS_AddressIsLocalSubnet(m, rr->resrec.InterfaceID, &rr->sourceAddress))
        {
            unicast_assist_addr_refresh(rr->resrec.name, rr->resrec.namehash, rr->resrec.rrtype, rr->resrec.RecordType,
                &rr->sourceAddress, rr->resrec.InterfaceID);
        }
        rr->unicastAssistSent = mDNSfalse;
    }
#endif
    rr->TimeRcvd             = m->timenow;
    rr->resrec.rroriginalttl = ttl;
    rr->UnansweredQueries = 0;
    if (rr->resrec.mortality != Mortality_Mortal) rr->resrec.mortality = Mortality_Immortal;

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER) && \
    (MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) || MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH))
    const mdns_dns_service_t newService = mdns_cache_metadata_get_dns_service(m->rec.r.resrec.metadata);
    const mdns_dns_service_t oldService = mdns_cache_metadata_get_dns_service(rr->resrec.metadata);
    const mDNSBool newRecordComesFromPushService = (newService &&
        (mdns_dns_service_get_type(newService) == mdns_dns_service_type_push));
    if (newRecordComesFromPushService && (newService == oldService))
    {
        // A cached record that has been unsubscribed and that comes from the same DNS push service is being refreshed
        // by the same subscription response, so we just need to resubscribe it.
        mdns_cache_metadata_set_subscriber_id(rr->resrec.metadata,
            mdns_cache_metadata_get_subscriber_id(m->rec.r.resrec.metadata));
        rr->DNSPushSubscribed = mDNStrue;
    }
#endif

    SetNextCacheCheckTimeForRecord(m, rr);
}

mDNSexport void GrantCacheExtensions(mDNS *const m, DNSQuestion *q, mDNSu32 lease)
{
    CacheRecord *rr;
    CacheGroup *cg = CacheGroupForName(m, q->qnamehash, &q->qname);
    for (rr = cg ? cg->members : mDNSNULL; rr; rr=rr->next)
        if (rr->CRActiveQuestion == q)
        {
            //LogInfo("GrantCacheExtensions: new lease %d / %s", lease, CRDisplayString(m, rr));
            RefreshCacheRecord(m, rr, lease);
        }
}

// When the response does not match the question directly, we still want to cache them sometimes. The current response is
// in m->rec.
mDNSlocal mDNSBool IsResponseAcceptable(mDNS *const m, const CacheRecord *crlist)
{
    const CacheRecord *const newcr = &m->rec.r;
    const ResourceRecord *rr = &newcr->resrec;
    const CacheRecord *cr;

    for (cr = crlist; cr != (CacheRecord*)1; cr = cr->NextInCFList)
    {
        domainname *target = GetRRDomainNameTarget(&cr->resrec);
        // When we issue a query for A record, the response might contain both a CNAME and A records. Only the CNAME would
        // match the question and we already created a cache entry in the previous pass of this loop. Now when we process
        // the A record, it does not match the question because the record name here is the CNAME. Hence we try to
        // match with the previous records to make it an AcceptableResponse. We have to be careful about setting the
        // DNSServer value that we got in the previous pass. This can happen for other record types like SRV also.

        if (target && cr->resrec.rdatahash == rr->namehash && SameDomainName(target, rr->name))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "Found a matching entry in the CacheFlushRecords - "
                "new rrtype: " PRI_S ", matched name: " PRI_DM_NAME ", description: " PRI_S, DNSTypeName(rr->rrtype),
                DM_NAME_PARAM(rr->name), CRDisplayString(m, cr));
            return (mDNStrue);
        }
    }
    return mDNSfalse;
}

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

#define MAX_CNAME_TRAVERSAL 11 // We follow at most 10 CNAMEs in a CNAME chain.

mDNSlocal void
ParseCNameChainFromMessage(
    const DNSMessage *const      response,
    const mDNSu8 *const          limit,
    const mDNSInterfaceID        InterfaceID,
    const domainname *const      qname,
    domainname                   cnameChain[static const MAX_CNAME_TRAVERSAL],
    mDNSu32  *const              outChainLen)
{
    if (response == mDNSNULL || limit == mDNSNULL || qname == mDNSNULL || cnameChain == mDNSNULL || outChainLen == mDNSNULL)
    {
        return;
    }

    AssignDomainName(&cnameChain[0], qname);
    *outChainLen = 1;

    const mDNSu16 answerCount = response->h.numAnswers;
    if (answerCount == 0)
    {
        return;
    }

    const mDNSu8 *ptr = LocateAnswers(response, limit);
    if (ptr == mDNSNULL)
    {
        return;
    }

    // Parse all CNAMEs in the answer section.
#define MAX_CNAME_TO_PARSE ((MAX_CNAME_TRAVERSAL) * 2)
    // Index 0 is the original owner name, Index 1 is the CNAME.
    domainname cnames[MAX_CNAME_TO_PARSE][2];
    mDNSu16 cnameCount = 0;
    mDNS *const m = &mDNSStorage;

    for (mDNSu32 i = 0; i < answerCount && ptr < limit && cnameCount < countof(cnames); mDNSCoreResetRecord(m), i++)
    {
        ptr = GetLargeResourceRecord(m, response, ptr, limit, InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
        const ResourceRecord *const rr = &(m->rec.r.resrec);
        if (rr->RecordType == kDNSRecordTypePacketNegative)
        {
            continue;
        }

        if (rr->rrtype != kDNSType_CNAME)
        {
            continue;
        }

        // A CNAME cannot be placed at the root domain level.
        if (IsRootDomain(rr->name))
        {
            continue;
        }

        // Found a CNAME.
        if (SameDomainName(rr->name, &rr->rdata->u.name))
        {
            continue;
        }

        // Save the CNAME.
        AssignDomainName(&(cnames[cnameCount][0]), rr->name);
        AssignDomainName(&(cnames[cnameCount][1]), &rr->rdata->u.name);
        cnameCount++;
    }

    // With the CNAME obtained above, construct a CNAME chain.
    // The CNAME chain always starts with the original question name.
    const domainname *nameToSearch = qname;

    // *outChainLen != MAX_CNAME_TRAVERSAL: Do not follow more than 10 CNAMEs.
    // *outChainLen <= cnameCount If all CNAMEs parsed above has been used to form a chain, the chain length would be
    // cnameCount. When *outChainLen == cnameCount + 1, there is not point to continue the search.
    while (*outChainLen != MAX_CNAME_TRAVERSAL && *outChainLen <= cnameCount)
    {
        const domainname *nextCName = mDNSNULL;

        for (mDNSu32 i = 0; i < cnameCount; i++)
        {
            // IsRootDomain(&cnames[i][0]): Since root domain cannot have CNAME, here root domain is used to mark
            // an already traversed CNAME. If a CNAME has been traversed before, skip it to avoid circular dependence.
            if (IsRootDomain(&cnames[i][0]) || !SameDomainName(nameToSearch, &cnames[i][0]))
            {
                continue;
            }

            // We found the next name in the chain.
            nextCName = &cnames[i][1];

            // Mark the current CNAME as already traversed.
            const domainname *const rootDomain = (const domainname *)"\x0";
            AssignDomainName(&cnames[i][0], rootDomain);
            break;
        }

        // If we did not find the next name in the chain, it means we have reached the end.
        if (nextCName == mDNSNULL)
        {
            break;
        }

        // Place the name into the CNAME chain.
        nameToSearch = nextCName;
        AssignDomainName(&cnameChain[*outChainLen], nameToSearch);
        *outChainLen += 1;
    }
}

// A normal denial of existence response should have no more than 3 NSEC/NSEC3 records. Here we use 10 to allow more to
// be able to processed and discarded.
#define MAX_NUM_NSEC_NSEC3_TO_PROCESS   10
// Here we assume that the total number of RRSIG records contained in a response should be no more than 30 (large enough).
#define MAX_NUM_RRSIG_TO_PROCESS        30

#define mDNSInvalidUnicastTTLSeconds (mDNSMaximumUnicastTTLSeconds + 1)

// Parse SOA, NSEC/NSEC3, RRSIG records contained in the DNS message to DNSSEC objects.
mDNSlocal mDNSu32
ParseDenialOfExistenceObjsFromMessage(
    const DNSMessage *const     response,
    const mDNSu8 *const         limit,
    const mDNSInterfaceID       InterfaceID,
    dnssec_obj_rr_soa_t *const  outObjSOA,
    dnssec_obj_rr_rrsig_t       objSOARRSIG[static const MAX_NUM_RRSIG_TO_PROCESS],
    mDNSu8 *const               outSOARRSIGCount,
    dnssec_obj_rr_nsec_t        outObjNSECs[static const MAX_NUM_NSEC_NSEC3_TO_PROCESS],
    mDNSu8 *const               outNSECCount,
    dnssec_obj_rr_nsec3_t       outObjNSEC3s[static const MAX_NUM_NSEC_NSEC3_TO_PROCESS],
    mDNSu8 *const               outNSEC3Count,
    dnssec_obj_rr_rrsig_t       outObjRRSIGs[static const MAX_NUM_RRSIG_TO_PROCESS],
    mDNSu8 *const               outRRSIGCount)
{
    dnssec_error_t err;
    mDNSu32 negativeTTL = mDNSInvalidUnicastTTLSeconds;

    // To parse the authority section, the output pointers should be non-null, in order to get a valid denial of
    // existence response set.
    if (outObjSOA == mDNSNULL || objSOARRSIG == mDNSNULL || outSOARRSIGCount == mDNSNULL ||
        outObjNSECs == mDNSNULL || outNSECCount == mDNSNULL || outObjNSEC3s == mDNSNULL || outNSEC3Count == mDNSNULL ||
        outObjRRSIGs == mDNSNULL || outRRSIGCount == mDNSNULL)
    {
        goto exit;
    }

    *outObjSOA = mDNSNULL;
    *outSOARRSIGCount = 0;
    *outNSECCount = 0;
    *outNSEC3Count = 0;
    *outRRSIGCount = 0;

    // All SOA, NSEC/NSEC3, RRSIG records are in the authority section.
    const mDNSu16 authorityCount = response->h.numAuthorities;
    if (authorityCount == 0)
    {
        goto exit;
    }

    // Go to the authority section.
    const mDNSu8 *ptr = LocateAuthorities(response, limit);
    if (ptr == mDNSNULL)
    {
        goto exit;
    }

    mDNS *const m = &mDNSStorage;
    // For each DNS record contained in the authority section.
    for (mDNSu32 i = 0; i < authorityCount && ptr < limit; mDNSCoreResetRecord(m), i++)
    {
        // Get the record from the DNS message.
        ptr = GetLargeResourceRecord(m, response, ptr, limit, InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
        const ResourceRecord *const rr = &(m->rec.r.resrec);
        if (rr->RecordType == kDNSRecordTypePacketNegative)
        {
            continue;
        }

        if (negativeTTL == mDNSInvalidUnicastTTLSeconds)
        {
            // If the denial of existence record's TTL is equal to mDNSInvalidUnicastTTLSeconds, then we choose
            // mDNSMaximumUnicastTTLSeconds (which is mDNSInvalidUnicastTTLSeconds minus one).
            negativeTTL = (m->rec.r.resrec.rroriginalttl != mDNSInvalidUnicastTTLSeconds) ? m->rec.r.resrec.rroriginalttl : mDNSMaximumUnicastTTLSeconds;
        }
        else
        {
            negativeTTL = MIN(negativeTTL, m->rec.r.resrec.rroriginalttl);
        }

        if (rr->rrtype == kDNSType_SOA)
        {
            if (*outObjSOA != mDNSNULL)
            {
                // Should only have one SOA.
                continue;
            }

            // Cautious!
            // SOA has compression pointers, GetLargeResourceRecord() expands the compression pointer to the full size
            // structure that is represented by mDNSResponder. To construct the uncompressed rdata from the resource
            // record parsed by GetLargeResourceRecord(), use putRData().
            // Other than SOA, NSEC/NSEC3/RRSIG can use the rdata parsed by GetLargeResourceRecord() directly because
            // these records will not have any compression pointer.
        #define MAX_SOA_RD_SIZE (MAX_DOMAIN_NAME + MAX_DOMAIN_NAME + 5 * sizeof(mDNSu32))
            mDNSu8 soaRdata[MAX_SOA_RD_SIZE];
            putRData(mDNSNULL, soaRdata, soaRdata + sizeof(soaRdata), rr);
            *outObjSOA = dnssec_obj_rr_soa_create(rr->name->c, soaRdata, rr->rdlength, mDNStrue, &err);
        }
        else if (rr->rrtype == kDNSType_NSEC)
        {
            if (*outNSECCount == MAX_NUM_NSEC_NSEC3_TO_PROCESS)
            {
                continue;
            }
            outObjNSECs[*outNSECCount] = dnssec_obj_rr_nsec_create(rr->name->c, rr->rdata->u.data, rr->rdlength, mDNStrue, &err);
            if (err == DNSSEC_ERROR_NO_ERROR)
            {
                *outNSECCount += 1;
            }
        }
        else if (rr->rrtype == kDNSType_NSEC3)
        {
            if (*outNSEC3Count == MAX_NUM_NSEC_NSEC3_TO_PROCESS)
            {
                continue;
            }
            outObjNSEC3s[*outNSEC3Count] = dnssec_obj_rr_nsec3_create(rr->name->c, rr->rdata->u.data, rr->rdlength, mDNStrue, &err);
            if (err == DNSSEC_ERROR_NO_ERROR)
            {
                *outNSEC3Count += 1;
            }
        }
        else if (rr->rrtype == kDNSType_RRSIG)
        {
            dnssec_obj_rr_rrsig_t rrsig = dnssec_obj_rr_rrsig_create(rr->name->c, rr->rdata->u.data, rr->rdlength, mDNStrue, &err);
            if (err != DNSSEC_ERROR_NO_ERROR)
            {
                goto rrsig_parsing_exit;
            }

            const mDNSu16 typeCovered = dnssec_obj_rr_rrsig_get_type_covered(rrsig);
            if (typeCovered == kDNSType_SOA)
            {
                if (*outSOARRSIGCount == MAX_NUM_RRSIG_TO_PROCESS)
                {
                    goto rrsig_parsing_exit;
                }
                objSOARRSIG[*outSOARRSIGCount] = rrsig;
                dnssec_obj_retain(objSOARRSIG[*outSOARRSIGCount]);
                *outSOARRSIGCount += 1;
            }
            else if ((typeCovered == kDNSType_NSEC) || (typeCovered == kDNSType_NSEC3))
            {
                if (*outRRSIGCount == MAX_NUM_RRSIG_TO_PROCESS)
                {
                    goto rrsig_parsing_exit;
                }
                outObjRRSIGs[*outRRSIGCount] = rrsig;
                dnssec_obj_retain(outObjRRSIGs[*outRRSIGCount]);
                *outRRSIGCount += 1;
            }

        rrsig_parsing_exit:
            MDNS_DISPOSE_DNSSEC_OBJ(rrsig);
        }
    }

exit:
    return negativeTTL;
}

#endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSlocal mDNSBool IsResponseMDNSEquivalent(const mdns_client_t client, const mdns_dns_service_t service)
{
    // Determine whether the response is mDNS like response, as opposed to non-mDNS.
    mDNSBool ResponseIsMDNSEquivalent;
    const mDNSBool usesQuerier = (mdns_querier_downcast(client) != mDNSNULL);
    const mDNSBool usesSubscriber = (mdns_subscriber_downcast(client) != mDNSNULL);

    if (usesQuerier)
    {
        // Case 1: Message came from a querier, which is inherently non-mDNS, i.e., Do53, DoT, DoH, or ODoH.
        ResponseIsMDNSEquivalent = mDNSfalse;
    }
    else if (usesSubscriber)
    {
        // Case 2: Message came from a subscriber, which means it came via DNS Push. Technically, not mDNS, but we may
        // or may not want to treat messages like mDNS.
        if (!service)
        {
            // Case 2.1: Message came from a subscriber, but no dns service is being used, the record came from a
            // unicast discovery proxy.
            ResponseIsMDNSEquivalent = mDNStrue;
        }
        else if (mdns_dns_service_is_mdns_alternative(service))
        {
            // Case 2.2: Message came from a subscriber, the record is mDNS alternative. (custom DNS push case).
            ResponseIsMDNSEquivalent = mDNStrue;
        }
        else
        {
            // Case 2.2: Message came from a subscriber, the record is non-mDNS (discovered DNS push or configured
            // unicast push service case).
            ResponseIsMDNSEquivalent = mDNSfalse;
        }
    }
    else
    {
        // Case 3: No object provided the message, meaning it came via the mDNS protocol, so this is definitely an mDNS
        // message.
        ResponseIsMDNSEquivalent = mDNStrue;
    }
    return ResponseIsMDNSEquivalent;
}
#endif

mDNSlocal void mDNSCoreReceiveNoUnicastAnswers(mDNS *const m, const DNSMessage *const response, const mDNSu8 *end,
    const mDNSAddr *dstaddr, const mDNSIPPort dstport, const mDNSInterfaceID InterfaceID,
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    const mdns_querier_t querier, const mdns_dns_service_t uDNSService,
#endif
    const uDNS_LLQType LLQType)
{
    int i;
    const mDNSu8 *ptr   = response->data;
    CacheRecord *SOARecord = mDNSNULL;

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    dnssec_obj_denial_of_existence_t denial = NULL;

    dnssec_obj_rr_soa_t objSOA = NULL;
    dnssec_obj_rr_rrsig_t objSOARRSIG[MAX_NUM_RRSIG_TO_PROCESS];
    mDNSu8 soaRRSIGCount = 0;

    dnssec_obj_rr_nsec_t objNSECs[MAX_NUM_NSEC_NSEC3_TO_PROCESS];
    mDNSu8 nsecCount = 0;

    dnssec_obj_rr_nsec3_t objNSEC3s[MAX_NUM_NSEC_NSEC3_TO_PROCESS];
    mDNSu8 nsec3Count = 0;

    dnssec_obj_rr_rrsig_t objRRSIGs[MAX_NUM_RRSIG_TO_PROCESS];
    mDNSu8 rrsigCount = 0;

    mDNSBool hasParsed = mDNSfalse;

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    const mDNSBool ResponseIsMDNS = IsResponseMDNSEquivalent(mdns_client_upcast(querier), uDNSService);
#else
    const mDNSBool ResponseIsMDNS = mDNSOpaque16IsZero(response->h.id);
#endif

    const mDNSBool toBeDNSSECValidated = (!ResponseIsMDNS) &&
                            (
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                                (querier != mDNSNULL) ?
                                (mdns_querier_get_dnssec_ok(querier) && mdns_querier_get_checking_disabled(querier)) :
#endif
                                mDNSfalse
                            );

#endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

    for (i = 0; i < response->h.numQuestions && ptr && ptr < end; i++)
    {
        DNSQuestion q;
        ptr = getQuestion(response, ptr, end, InterfaceID, &q);
        if (ptr)
        {
            DNSQuestion *qptr;
            CacheRecord *cr, *neg = mDNSNULL;
            CacheGroup *cg;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            if (querier)
            {
                qptr = Querier_GetDNSQuestion(querier, mDNSNULL);
            }
            else
#endif
            {
                qptr = ExpectingUnicastResponseForQuestion(m, dstport, response->h.id, &q, !dstaddr);
                if (!qptr)
                {
                    continue;
                }
            }

            // The CNAME chain always starts from the original question name.
            const domainname *currentQNameInCNameChain = &q.qname;
            mDNSu32 currentQNameHashInCNameChain = q.qnamehash;
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

            const uint32_t qid = (qptr != mDNSNULL) ? mDNSVal16(qptr->TargetQID) : 0;
        #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            // When we use querier, it is possible we receive a response whose question has already gone, in which case
            // qptr will be NULL and querier is an orphan. We still want to process the response to not waste the
            // traffic. Therefore, do not rely on qptr to get DNSSEC response matched, instead, use querier.
            // The response will be mDNS if querier is NULL.
            const mDNSBool processDenialOfExistence = toBeDNSSECValidated;
            const uint16_t qclass = (querier != mDNSNULL) ? mdns_querier_get_qclass(querier) : 0;
            const uint16_t qtype = (querier != mDNSNULL) ? mdns_querier_get_qtype(querier) : 0;
        #else
            const mDNSBool processDenialOfExistence = toBeDNSSECValidated && !ResponseIsMDNS && (qptr != mDNSNULL) && dns_question_is_primary_dnssec_requestor(qptr);
            const uint16_t qclass = (qptr != mDNSNULL) ? qptr->qclass : 0;
            const uint16_t qtype = (qptr != mDNSNULL) ? qptr->qtype : 0;
        #endif
            domainname cnameChain[MAX_CNAME_TRAVERSAL];
            mDNSu32 chainLen = 1;
            mDNSu32 cnameIndex = 0;

            // If we need to process denial of existence record for DNSSEC-enabled question, get the CNAME chain from
            // the response answer section.
            if (processDenialOfExistence)
            {
                ParseCNameChainFromMessage(response, end, InterfaceID, &q.qname, cnameChain, &chainLen);
                currentQNameInCNameChain = &cnameChain[cnameIndex];
                currentQNameHashInCNameChain = DomainNameHashValue(currentQNameInCNameChain);
            }
            // If the question has required DNSSEC RRs, iterate through all the CNAMEs in the chain.
        #endif

            // 1. If the question is a normal non-DNSSEC question, currentQNameInCNameChain will be set to the original
            //    question name. It will be set to mDNSNULL after the first iteration.
            // 2. If the question is a DNSSEC question, currentQNameInCNameChain will be set to the original question
            //    name as well, and the question name is the start of a CNAME chain. It will be set to the next name
            //    in the CNAME chain.
            while (currentQNameInCNameChain != mDNSNULL)
            {
            cg = CacheGroupForName(m, currentQNameHashInCNameChain, currentQNameInCNameChain);
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            // The following code checks if the newly added or existing records in the cache that answer the question is
            // a wildcard answer. If it is, then we must create a negative record to deny the existence of the original
            // question name.
            const mDNSBool checkWildcardAnswer = processDenialOfExistence && (response->h.numAnswers > 0);
            mDNSBool wildcardAnswer = mDNSfalse;
            domainname nameAfterWildcardExpansion;
            mDNSu16 typeCoveredToCheck = kDNSRecordType_Invalid;
            mDNSu32 denial_of_existence_ttl = mDNSInvalidUnicastTTLSeconds;

            if (checkWildcardAnswer)
            {
                for (cr = cg ? cg->members : mDNSNULL; cr != mDNSNULL; cr = cr->next)
                {
                    mDNSBool isAnswer;
                #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                    if (querier)
                    {
                        const mdns_dns_service_t dnsservice = mdns_cache_metadata_get_dns_service(cr->resrec.metadata);
                        isAnswer = (dnsservice == uDNSService) && Client_SameNameCacheRecordIsAnswer(cr,
                            mdns_client_upcast(querier));
                    }
                    else
                #endif
                    {
                        isAnswer = SameNameCacheRecordAnswersQuestion(cr, qptr);
                    }

                    const ResourceRecord *const rr = &cr->resrec;
                    // We only care about the non-negative record that answers our question positively.
                    // (It is possible that we have a negative record that denies the existence of exact name match,
                    // while having wildcard expanded positive record in the cache at the same time.)
                    if (!isAnswer || rr->RecordType == kDNSRecordTypePacketNegative)
                    {
                        continue;
                    }

                    denial_of_existence_ttl = rr->rroriginalttl;
                    if (rr->rrtype == kDNSType_RRSIG)
                    {
                        if (!resource_record_as_rrsig_covers_wildcard_rr(rr))
                        {
                            // This RRSIG does not cover wildcard, therefore, it is impossible that we have received wildcard
                            // denial of existence.
                            wildcardAnswer = mDNSfalse;
                            break;
                        }

                        if (typeCoveredToCheck != 0)
                        {
                            if (resource_record_as_rrsig_covers_rr_type(rr, typeCoveredToCheck))
                            {
                                wildcardAnswer = mDNStrue;
                                AssignDomainName(&nameAfterWildcardExpansion, rr->name);
                            }
                            else
                            {
                                // This RRSIG does not cover the expected wildcard RRSet type.
                                wildcardAnswer = mDNSfalse;
                                break;
                            }
                        }
                        else
                        {
                            typeCoveredToCheck = resource_record_as_rrsig_get_covered_type(rr);
                            wildcardAnswer = mDNStrue;
                            AssignDomainName(&nameAfterWildcardExpansion, rr->name);
                        }

                    }
                    else
                    {
                        if (typeCoveredToCheck != 0)
                        {
                            if (typeCoveredToCheck != rr->rrtype)
                            {
                                // The wildcard RRSIG does not cover the expected wildcard RRSet type.
                                wildcardAnswer = mDNSfalse;
                                break;
                            }
                        }
                        else
                        {
                            typeCoveredToCheck = rr->rrtype;
                        }
                    }
                }

                if (!wildcardAnswer)
                {
                    typeCoveredToCheck = 0;
                    denial_of_existence_ttl = 0;
                }
            }
        #endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

            for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
            {
                mDNSBool isAnswer;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                if (querier)
                {
                    const mdns_dns_service_t dnsservice = mdns_cache_metadata_get_dns_service(cr->resrec.metadata);
                    isAnswer = (dnsservice == uDNSService) && Client_SameNameCacheRecordIsAnswer(cr,
                        mdns_client_upcast(querier));
                }
                else
#endif
                {
                    isAnswer = SameNameCacheRecordAnswersQuestion(cr, qptr);
                }
                if (isAnswer)
                {
                    // 1. If we got a fresh answer to this query, then don't need to generate a negative entry
                    if (RRExpireTime(cr) - m->timenow > 0)
                    {
                    #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                        if (!wildcardAnswer)
                    #endif
                        {
                            // If the matched question is a wildcard match (by looking into the labels field of RRSIG)
                            // then we need a new or existing negative record to deny the original question name.
                            break;
                        }
                    }
                    // 2. If we already had a negative entry, keep track of it so we can resurrect it instead of creating a new one
                    if (cr->resrec.RecordType == kDNSRecordTypePacketNegative) neg = cr;
                    else if (cr->resrec.mortality == Mortality_Ghost)
                    {
                        // 3. If the existing entry is expired, mark it to be purged
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                            "mDNSCoreReceiveNoUnicastAnswers: Removing expired record" PRI_S, CRDisplayString(m, cr));
                        mDNS_PurgeCacheResourceRecord(m, cr);
                   }
                }
            }
            // When we're doing parallel unicast and multicast queries for dot-local names (for supporting Microsoft
            // Active Directory sites) we don't want to waste memory making negative cache entries for all the unicast answers.
            // Otherwise we just fill up our cache with negative entries for just about every single multicast name we ever look up
            // (since the Microsoft Active Directory server is going to assert that pretty much every single multicast name doesn't exist).
            // This is not only a waste of memory, but there's also the problem of those negative entries confusing us later -- e.g. we
            // suppress sending our mDNS query packet because we think we already have a valid (negative) answer to that query in our cache.
            // The one exception is that we *DO* want to make a negative cache entry for "local. SOA", for the (common) case where we're
            // *not* on a Microsoft Active Directory network, and there is no authoritative server for "local". Note that this is not
            // in conflict with the mDNS spec, because that spec says, "Multicast DNS Zones have no SOA record," so it's okay to cache
            // negative answers for "local. SOA" from a uDNS server, because the mDNS spec already says that such records do not exist :-)
            //
            // By suppressing negative responses, it might take longer to timeout a .local question as it might be expecting a
            // response e.g., we deliver a positive "A" response and suppress negative "AAAA" response and the upper layer may
            // be waiting longer to get the AAAA response before returning the "A" response to the application. To handle this
            // case without creating the negative cache entries, we generate a negative response and let the layer above us
            // do the appropriate thing. This negative response is also needed for appending new search domains.
            mDNSBool doNotCreateUnicastNegativeRecordForLocalDomains = (!InterfaceID && (q.qtype != kDNSType_SOA) && IsLocalDomain(&q.qname));
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            doNotCreateUnicastNegativeRecordForLocalDomains = doNotCreateUnicastNegativeRecordForLocalDomains && !processDenialOfExistence;
        #endif
            if (doNotCreateUnicastNegativeRecordForLocalDomains)
            {
                if (!cr)
                {
                    if (qptr)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->Q%u] mDNSCoreReceiveNoUnicastAnswers: Generate negative response for "
                            PRI_DM_NAME " (" PUB_S ")",
                            q.request_id, mDNSVal16(q.TargetQID), DM_NAME_PARAM(&q.qname), DNSTypeName(q.qtype));
                        m->CurrentQuestion = qptr;
                        // We are not creating a cache record in this case, we need to pass back
                        // the error we got so that the proxy code can return the right one to
                        // the application
                        if (qptr->ProxyQuestion)
                            qptr->responseFlags = response->h.flags;
                        GenerateNegativeResponseEx(m, mDNSInterface_Any, QC_forceresponse, response->h.flags);
                        m->CurrentQuestion = mDNSNULL;
                    }
                }
                else
                {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->Q%u] mDNSCoreReceiveNoUnicastAnswers: Skipping check and not creating a "
                        "negative cache entry for " PRI_DM_NAME " (" PUB_S ")",
                        q.request_id, mDNSVal16(q.TargetQID), DM_NAME_PARAM(&q.qname), DNSTypeName(q.qtype));
                }
            }
            else
            {
                if (!cr)
                {
                    // We start off assuming a negative caching TTL of 60 seconds
                    // but then look to see if we can find an SOA authority record to tell us a better value we should be using
                    mDNSu32 negttl = 60;
                    int repeat = 0;
                    const domainname *currentQName = currentQNameInCNameChain;
                    mDNSu32 currentQNameHash = currentQNameHashInCNameChain;

                    // Special case for our special Microsoft Active Directory "local SOA" check.
                    // Some cheap home gateways don't include an SOA record in the authority section when
                    // they send negative responses, so we don't know how long to cache the negative result.
                    // Because we don't want to keep hitting the root name servers with our query to find
                    // if we're on a network using Microsoft Active Directory using "local" as a private
                    // internal top-level domain, we make sure to cache the negative result for at least one day.
                    mDNSBool useDefaultTTLForDotLocalDomain = (q.qtype == kDNSType_SOA) && SameDomainName(&q.qname, &localdomain);
                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    useDefaultTTLForDotLocalDomain = (useDefaultTTLForDotLocalDomain && !processDenialOfExistence);
                #endif
                    if (useDefaultTTLForDotLocalDomain)
                    {
                        negttl = 60 * 60 * 24;
                    }

                    // If we're going to make (or update) a negative entry, then look for the appropriate TTL from the SOA record
                    if (response->h.numAuthorities && (ptr = LocateAuthorities(response, end)) != mDNSNULL)
                    {
                        ptr = GetLargeResourceRecord(m, response, ptr, end, InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
                        if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative && m->rec.r.resrec.rrtype == kDNSType_SOA)
                        {
                            CacheGroup *cgSOA = CacheGroupForRecord(m, &m->rec.r.resrec);
                            const rdataSOA *const soa = (const rdataSOA *)m->rec.r.resrec.rdata->u.data;
                            mDNSu32 ttl_s = soa->min;
                            // We use the lesser of the SOA.MIN field and the SOA record's TTL, *except*
                            // for the SOA record for ".", where the record is reported as non-cacheable
                            // (TTL zero) for some reason, so in this case we just take the SOA record's TTL as-is
                            if (ttl_s > m->rec.r.resrec.rroriginalttl && m->rec.r.resrec.name->c[0])
                                ttl_s = m->rec.r.resrec.rroriginalttl;
                            if (negttl < ttl_s) negttl = ttl_s;

                            // Create the SOA record as we may have to return this to the questions
                            // that we are acting as a proxy for currently or in the future.
                            SOARecord = CreateNewCacheEntry(m, HashSlotFromNameHash(m->rec.r.resrec.namehash), cgSOA, 1, mDNSfalse, mDNSNULL);

                            // Special check for SOA queries: If we queried for a.b.c.d.com, and got no answer,
                            // with an Authority Section SOA record for d.com, then this is a hint that the authority
                            // is d.com, and consequently SOA records b.c.d.com and c.d.com don't exist either.
                            // To do this we set the repeat count so the while loop below will make a series of negative cache entries for us
                            //
                            // For ProxyQuestions, we don't do this as we need to create additional SOA records to cache them
                            // along with the negative cache record. For simplicity, we don't create the additional records.
                            mDNSBool deduceNegativeRecordFromSOA = (!qptr || !qptr->ProxyQuestion) && (q.qtype == kDNSType_SOA);
                        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                            // For DNSSEC-aware response, we cannot deduce negative records from SOA, because we need
                            // the NSEC/NSEC3 records to prove that. However, we can use NSEC/NSEC3 to deduce even more
                            // negative records, but that cannot be finished here.
                            deduceNegativeRecordFromSOA = (deduceNegativeRecordFromSOA && !processDenialOfExistence);
                        #endif
                            if (deduceNegativeRecordFromSOA)
                            {
                                int qcount = CountLabels(&q.qname);
                                int scount = CountLabels(m->rec.r.resrec.name);
                                if (qcount - 1 > scount)
                                    if (SameDomainName(SkipLeadingLabels(&q.qname, qcount - scount), m->rec.r.resrec.name))
                                        repeat = qcount - 1 - scount;
                            }
                        }
                        mDNSCoreResetRecord(m);
                    }

                    // If we already had a negative entry in the cache, then we double our existing negative TTL. This is to avoid
                    // the case where the record doesn't exist (e.g. particularly for things like our lb._dns-sd._udp.<domain> query),
                    // and the server returns no SOA record (or an SOA record with a small MIN TTL) so we assume a TTL
                    // of 60 seconds, and we end up polling the server every minute for a record that doesn't exist.
                    // With this fix in place, when this happens, we double the effective TTL each time (up to one hour),
                    // so that we back off our polling rate and don't keep hitting the server continually.
                    if (neg)
                    {
                        if (negttl < neg->resrec.rroriginalttl * 2)
                            negttl = neg->resrec.rroriginalttl * 2;
                        if (negttl > 3600)
                            negttl = 3600;
                    }

                    negttl = GetEffectiveTTL(LLQType, negttl);  // Add 25% grace period if necessary

                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    CacheRecord *denialOfExistenceTarget = mDNSNULL;
                    CacheRecord *const invalidTarget = (CacheRecord *)(-1);

                    // Only parse the SOA, NSEC/NSEC3, RRSIG out if the question requires DNSSEC RRs and we have not
                    // parsed it yet.
                    if (processDenialOfExistence && !hasParsed)
                    {
                        const mDNSu32 possible_denial_ttl = ParseDenialOfExistenceObjsFromMessage(response, end, InterfaceID,
                            &objSOA, objSOARRSIG, &soaRRSIGCount, objNSECs, &nsecCount, objNSEC3s, &nsec3Count,
                            objRRSIGs, &rrsigCount);
                        hasParsed = mDNStrue;

                        // If the positive part of the DNSSEC response does not provide a valid TTL for us to use,
                        // try to use the minimal TTL value of the denial of existence RRSet as our TTL.
                        if (denial_of_existence_ttl == mDNSInvalidUnicastTTLSeconds)
                        {
                            denial_of_existence_ttl = possible_denial_ttl;
                        }
                    }

                    // If the question requires DNSSEC RRs, and we found an existing negative record that denies the
                    // question (which means the negative record is DNSSEC-aware), check to see if we need to make it
                    // expired if its denial of existence record has changed since last time we have processed it.
                    if (processDenialOfExistence && neg != mDNSNULL && resource_record_is_to_be_dnssec_validated(&neg->resrec))
                    {
                        // Compute the new denial of existence record.
                        const mDNSBool containsNSECOrNSEC3Records = (nsecCount > 0) || (nsec3Count > 0);
                        if (denial == mDNSNULL && containsNSECOrNSEC3Records)
                        {
                            dnssec_error_t err;
                            denial = dnssec_obj_denial_of_existence_create(currentQName->c, qclass, qtype,
                                wildcardAnswer ? nameAfterWildcardExpansion.c : mDNSNULL, typeCoveredToCheck,
                                objSOA, objSOARRSIG, soaRRSIGCount, objNSECs, nsecCount,
                                objNSEC3s, nsec3Count, objRRSIGs, rrsigCount, &err);

                            if (denial == mDNSNULL)
                            {
                                LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_FAULT,
                                    "[Q%u] Unable to create the denial of existence record set - "
                                    "error: " PUB_S ", qname: " PRI_DM_NAME ", qtype: " PRI_S ", soaRRSIGCount: %u, "
                                    "nsecCount: %u, nsec3Count: %u, rrsigCount: %u.", qid,
                                    dnssec_error_get_error_description(err), DM_NAME_PARAM(currentQName),
                                    DNSTypeName(qtype), soaRRSIGCount, nsecCount, nsec3Count, rrsigCount);
                            } else {
                                LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_DEFAULT, "[Q%u] Create the denial of existence record set "
                                    "- qname: " PRI_DM_NAME ", qtype: " PRI_S ", denial type: " PUB_S,
                                    qid, DM_NAME_PARAM(currentQName), DNSTypeName(qtype),
                                    dnssec_obj_denial_of_existence_get_denial_type_description(denial));
                            }
                        }

                        const dnssec_obj_denial_of_existence_t neg_denial = resource_record_get_denial_of_existence(&neg->resrec);
                        // Check if the denial of existence record has changed.

                        mDNSBool sameDenialOfExistence;
                        if (neg_denial != mDNSNULL && denial != mDNSNULL) {
                            sameDenialOfExistence = dnssec_obj_equal(neg_denial, denial);
                        } else if (neg_denial == mDNSNULL && denial == mDNSNULL) {
                            sameDenialOfExistence = true;
                        } else {
                            sameDenialOfExistence = false;
                        }

                        if (!sameDenialOfExistence)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_DEBUG, "Denial of existence record changes, purging the old negative record - "
                                "name: " PRI_DM_NAME ", type: " PUB_S, DM_NAME_PARAM(currentQName), DNSTypeName(qtype));
                            // If the denial of existence has changed, the old negative record should be removed and
                            // the question callback needs to be notified about this change, therefore, we mark it
                            // as expired and create a new negative record that contains the latest denial of
                            // existence record.
                            mDNS_PurgeCacheResourceRecord(m, neg);
                            neg = mDNSNULL;
                        }
                        else
                        {
                            LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_DEBUG, "Denial of existence record does not change, rescuing the old negative record - "
                                "name: " PRI_DM_NAME ", type: " PUB_S, DM_NAME_PARAM(currentQName), DNSTypeName(qtype));
                        }
                    }

                    // If negative record is still valid after the checking above, assign it to denialOfExistenceTarget.
                    // Continue the negative record refresh process.
                    if (processDenialOfExistence && neg != mDNSNULL)
                    {
                        denialOfExistenceTarget = neg;
                        denialOfExistenceTarget->ineligibleForRecycling = mDNStrue;
                    }

                    if (processDenialOfExistence && denial_of_existence_ttl != mDNSInvalidUnicastTTLSeconds)
                    {
                        negttl = denial_of_existence_ttl;
                    }

                #endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

                    // If we already had a negative cache entry just update it, else make one or more new negative cache entries.
                    if (neg)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                            "[R%u->Q%u] mDNSCoreReceiveNoUnicastAnswers: Renewing negative TTL from %d to %d " PRI_S,
                            q.request_id, mDNSVal16(q.TargetQID), neg->resrec.rroriginalttl, negttl,
                            CRDisplayString(m, neg));
                        RefreshCacheRecord(m, neg, negttl);
                        // When we created the cache for the first time and answered the question, the question's
                        // interval was set to MaxQuestionInterval. If the cache is about to expire and we are resending
                        // the queries, the interval should still be at MaxQuestionInterval. If the query is being
                        // restarted (setting it to InitialQuestionInterval) for other reasons e.g., wakeup,
                        // we should reset its question interval here to MaxQuestionInterval.
                        if (qptr)
                        {
                            ResetQuestionState(m, qptr);
                        }
                        if (SOARecord)
                        {
                            if (neg->soa)
                                ReleaseCacheRecord(m, neg->soa);
                            neg->soa = SOARecord;
                            SOARecord = mDNSNULL;
                        }
                    }
                    else while (1)
                        {
                            CacheRecord *negcr;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                            const DNSServRef dnsserv = uDNSService;
#else
                            const DNSServRef dnsserv = qptr->qDNSServer;
#endif
                            debugf("mDNSCoreReceiveNoUnicastAnswers making negative cache entry TTL %d for %##s (%s)", negttl, currentQName, DNSTypeName(q.qtype));
                            // Create a negative record for the current name in the CNAME chain.
                            MakeNegativeCacheRecord(m, &m->rec.r, currentQName, currentQNameHash, q.qtype, q.qclass, negttl, mDNSInterface_Any,
                                dnsserv, response->h.flags);
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                            if (querier)
                            {
                                ResourceRecord *const rr = &m->rec.r.resrec;
                                if (rr->metadata)
                                {
                                    const mdns_extended_dns_error_t ede = mdns_querier_get_extended_dns_error(querier);
                                    mdns_cache_metadata_set_extended_dns_error(rr->metadata, ede);
                                }
                            }
#endif
                            // We create SOA records above which might create new cache groups. Earlier
                            // in the function we looked up the cache group for the name and it could have
                            // been NULL. If we pass NULL cg to new cache entries that we create below,
                            // it will create additional cache groups for the same name. To avoid that,
                            // look up the cache group again to re-initialize cg again.
                            cg = CacheGroupForName(m, currentQNameHash, currentQName);

                            CreateNewCacheEntryFlags flags = kCreateNewCacheEntryFlagsNone;
                            mDNSs32 delay = 1;
                        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                            if (toBeDNSSECValidated)
                            {
                                flags |= kCreateNewCacheEntryFlagsDNSSECRRToValidate;
                                delay = NonZeroTime(m->timenow);
                            }
                        #endif
                            // Need to add with a delay so that we can tag the SOA record
                            negcr = CreateNewCacheEntryEx(m, HashSlotFromNameHash(currentQNameHash), cg, delay, mDNStrue, mDNSNULL, flags);

                        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                            // Remember the newly created negative cache record.
                            if (processDenialOfExistence && denialOfExistenceTarget == mDNSNULL)
                            {
                                denialOfExistenceTarget = (negcr != mDNSNULL ? negcr : invalidTarget);
                                // Make this negative record ineligible for cache recycling temporarily, since we have
                                // a reference to it.
                                if (denialOfExistenceTarget != invalidTarget)
                                {
                                    denialOfExistenceTarget->ineligibleForRecycling = mDNStrue;
                                }
                            }
                        #endif

                            if (negcr)
                            {
                                if (SOARecord)
                                {
                                    if (negcr->soa)
                                        ReleaseCacheRecord(m, negcr->soa);
                                    negcr->soa = SOARecord;
                                    SOARecord = mDNSNULL;
                                }

                            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                                if (!processDenialOfExistence)
                                {
                            #endif
                                    negcr->DelayDelivery = 0;
                                    CacheRecordDeferredAdd(m, negcr);
                            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                                }
                                else
                                {
                                    // Always delay deliver DNSSEC response so that we can finish processing the entire response.
                                    ScheduleNextCacheCheckTime(m, HashSlotFromNameHash(currentQNameHash), negcr->DelayDelivery);
                                }
                            #endif
                            }
                            mDNSCoreResetRecord(m);
                            if (!repeat) break;
                            repeat--;
                            currentQName = SkipLeadingLabels(currentQName, 1);
                            currentQNameHash = DomainNameHashValue(currentQName);
                        }

                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    // Do not set denial of existence object:
                    // 1. if the records come from mDNS response.
                    // 2. If we do not know if the question requires DNSSEC RRs.
                    // 3. If the question does not require DNSSEC RRs. (No one will use the denial object)
                    // 4. If no negative is found or created.
                    if (!processDenialOfExistence || denialOfExistenceTarget == invalidTarget ||
                        !resource_record_is_dnssec_aware(&denialOfExistenceTarget->resrec))
                    {
                        goto process_denial_exit;
                    }
                    // Make the negative record eligible for cache recycling since we will finish the work with it.
                    denialOfExistenceTarget->ineligibleForRecycling = mDNSfalse;

                    // Check if we need to update the old denial of existence object.
                    // If the denialOfExistenceTarget already has a denial of existence record, it is an existing
                    // negative record whose denial of existence has not changed since last time. Therefore, we do not
                    // need to assign new denial of existence object to it.
                    if (resource_record_get_denial_of_existence(&denialOfExistenceTarget->resrec) != mDNSNULL)
                    {
                        goto process_denial_exit;
                    }

                    // Create the denial of existence object based on the parsed DNSSEC objects.
                    const mDNSBool containsNSECOrNSEC3Records = (nsecCount > 0) || (nsec3Count > 0);
                    if (denial == mDNSNULL && containsNSECOrNSEC3Records)
                    {
                        dnssec_error_t err;
                        // Here, we are creating the denial of existence object because we cannot find an acceptable
                        // positive record for the question, we can be sure that it is not a wildcard data response.
                        // Therefore we can pass kDNSRecordType_Invalid to skip the wildcard data response checking.
                        denial = dnssec_obj_denial_of_existence_create(currentQName->c, qclass, qtype,
                            wildcardAnswer ? nameAfterWildcardExpansion.c : mDNSNULL, typeCoveredToCheck,
                            objSOA, objSOARRSIG, soaRRSIGCount, objNSECs, nsecCount,
                            objNSEC3s, nsec3Count, objRRSIGs, rrsigCount, &err);

                        if (denial == mDNSNULL)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_FAULT,
                                "[Q%u] Unable to create the denial of existence record set - "
                                "error: " PUB_S ", qname: " PRI_DM_NAME ", qtype: " PRI_S ", soaRRSIGCount: %u, "
                                "nsecCount: %u, nsec3Count: %u, rrsigCount: %u.", qid,
                                dnssec_error_get_error_description(err), DM_NAME_PARAM(currentQName),
                                DNSTypeName(qtype), soaRRSIGCount, nsecCount, nsec3Count, rrsigCount);
                            goto process_denial_exit;
                        }
                        else
                        {
                            LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_DEFAULT, "[Q%u] Create the denial of existence record set "
                                "- qname: " PRI_DM_NAME ", qtype: " PRI_S ", denial type: " PUB_S,
                                qid, DM_NAME_PARAM(currentQName), DNSTypeName(qtype),
                                dnssec_obj_denial_of_existence_get_denial_type_description(denial));
                        }
                    }

                    resource_record_set_denial_of_existence(&denialOfExistenceTarget->resrec, denial);

                process_denial_exit:
                    (void)denialOfExistenceTarget; // To make process_denial_exit a non-empty label.
                #endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                }
            }
            // By default we do not traverse the CNAME chain.
            currentQNameInCNameChain = mDNSNULL;
            currentQNameHashInCNameChain = 0;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            // However, if the question has required DNSSEC RRs, we need to traverse the CNAME chain to create the
            // negative record for each name in the CNAME chain.
            if (processDenialOfExistence)
            {
                cnameIndex++;
                if (cnameIndex < chainLen)
                {
                    currentQNameInCNameChain = &cnameChain[cnameIndex];
                    currentQNameHashInCNameChain = DomainNameHashValue(currentQNameInCNameChain);
                }
            }

            if (!ResponseIsMDNS)
            {
                // Every CNAME in the chain has a separate denial of existence record.
                MDNS_DISPOSE_DNSSEC_OBJ(denial);
            }
#endif
            } // while (currentQNameInCNameChain != mDNSNULL)
        } // if (ptr)
    } // for (i = 0; i < response->h.numQuestions && ptr && ptr < end; i++)
    if (SOARecord)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "mDNSCoreReceiveNoUnicastAnswers: SOARecord not used");
        ReleaseCacheRecord(m, SOARecord);
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    if (!ResponseIsMDNS)
    {
        MDNS_DISPOSE_DNSSEC_OBJ(objSOA);
        for (mDNSu32 index = 0; index < soaRRSIGCount; index++)
        {
            MDNS_DISPOSE_DNSSEC_OBJ(objSOARRSIG[index]);
        }

        for (mDNSu32 index = 0; index < nsecCount; index++)
        {
            MDNS_DISPOSE_DNSSEC_OBJ(objNSECs[index]);
        }
        for (mDNSu32 index = 0; index < nsec3Count; index++)
        {
            MDNS_DISPOSE_DNSSEC_OBJ(objNSEC3s[index]);
        }

        for (mDNSu32 index = 0; index < rrsigCount; index++)
        {
            MDNS_DISPOSE_DNSSEC_OBJ(objRRSIGs[index]);
        }
    }
#endif
}

mDNSlocal void mDNSCorePrintStoredProxyRecords(mDNS *const m)
{
    AuthRecord *rrPtr = mDNSNULL;
    if (!m->SPSRRSet) return;
    LogSPS("Stored Proxy records :");
    for (rrPtr = m->SPSRRSet; rrPtr; rrPtr = rrPtr->next)
    {
        LogSPS("%s", ARDisplayString(m, rrPtr));
    }
}

mDNSlocal mDNSBool mDNSCoreRegisteredProxyRecord(mDNS *const m, AuthRecord *rr)
{
    AuthRecord *rrPtr = mDNSNULL;

    for (rrPtr = m->SPSRRSet; rrPtr; rrPtr = rrPtr->next)
    {
        if (IdenticalResourceRecord(&rrPtr->resrec, &rr->resrec))
        {
            LogSPS("mDNSCoreRegisteredProxyRecord: Ignoring packet registered with sleep proxy : %s ", ARDisplayString(m, rr));
            return mDNStrue;
        }
    }
    mDNSCorePrintStoredProxyRecords(m);
    return mDNSfalse;
}

mDNSexport CacheRecord* mDNSCoreReceiveCacheCheck(mDNS *const m, const DNSMessage *const response, uDNS_LLQType LLQType,
    const mDNSu32 slot, CacheGroup *cg, CacheRecord ***cfp, mDNSInterfaceID InterfaceID)
{
    CacheRecord *cr;
    CacheRecord **cflocal = *cfp;

    for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
    {
        mDNSBool match;
        // Resource record received via unicast, the resGroupID should match ?
        mDNSBool requireMatchedDNSService = !InterfaceID;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        if (!requireMatchedDNSService)
        {
            // If the associated DNS service has local purview and not mDNS-alternative, then it is a record resolved
            // through non-mDNS, although it has non-zero interface ID, it always has a specific DNS service to be
            // matched. In which case, we still need to check if the cached record has a matched DNS service with the
            // newly received record.
            const mdns_dns_service_t newRecordService = mdns_cache_metadata_get_dns_service(m->rec.r.resrec.metadata);
            requireMatchedDNSService = (newRecordService && mdns_dns_service_has_local_purview(newRecordService) &&
                                        !mdns_dns_service_is_mdns_alternative(newRecordService));
        }
#endif
        if (requireMatchedDNSService)
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            match = mdns_cache_metadata_same_dns_service(cr->resrec.metadata, m->rec.r.resrec.metadata);
#else
            const mDNSu32 id1 = (cr->resrec.rDNSServer ? cr->resrec.rDNSServer->resGroupID : 0);
            const mDNSu32 id2 = (m->rec.r.resrec.rDNSServer ? m->rec.r.resrec.rDNSServer->resGroupID : 0);
            match = (id1 == id2);
#endif
        }
        else
            match = (cr->resrec.InterfaceID == InterfaceID);
        // If we found this exact resource record, refresh its TTL
        if (match)
        {
            if (IdenticalSameNameRecord(&m->rec.r.resrec, &cr->resrec))
            {
                if (m->rec.r.resrec.rdlength > InlineCacheRDSize)
                    verbosedebugf("mDNSCoreReceiveCacheCheck: Found record size %5d interface %p already in cache: %s",
                                  m->rec.r.resrec.rdlength, InterfaceID, CRDisplayString(m, &m->rec.r));

                if (m->rec.r.resrec.RecordType & kDNSRecordTypePacketUniqueMask)
                {
                    // If this packet record has the kDNSClass_UniqueRRSet flag set, then add it to our cache flushing list
                    if (cr->NextInCFList == mDNSNULL && *cfp != &cr->NextInCFList && LLQType != uDNS_LLQ_Events)
                    {
                        *cflocal = cr;
                        cflocal = &cr->NextInCFList;
                        *cflocal = (CacheRecord*)1;
                        *cfp = &cr->NextInCFList;
                    }

                    // If this packet record is marked unique, and our previous cached copy was not, then fix it
                    if (!(cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask))
                    {
                        DNSQuestion *q;
                        for (q = m->Questions; q; q=q->next)
                        {
                            if (CacheRecordAnswersQuestion(cr, q))
                                q->UniqueAnswers++;
                        }
                        cr->resrec.RecordType = m->rec.r.resrec.RecordType;
                    }
                }

                if (!SameRDataBody(&m->rec.r.resrec, &cr->resrec.rdata->u, SameDomainNameCS))
                {
                    // If the rdata of the packet record differs in name capitalization from the record in our cache
                    // then mDNSPlatformMemSame will detect this. In this case, throw the old record away, so that clients get
                    // a 'remove' event for the record with the old capitalization, and then an 'add' event for the new one.
                    // <rdar://problem/4015377> mDNS -F returns the same domain multiple times with different casing
                    cr->resrec.rroriginalttl = 0;
                    cr->TimeRcvd = m->timenow;
                    cr->UnansweredQueries = MaxUnansweredQueries;
                    SetNextCacheCheckTimeForRecord(m, cr);
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "mDNSCoreReceiveCacheCheck: Discarding due to domainname case change old: " PRI_S,
                        CRDisplayString(m, cr));
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "mDNSCoreReceiveCacheCheck: Discarding due to domainname case change new: " PRI_S,
                        CRDisplayString(m, &m->rec.r));
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "mDNSCoreReceiveCacheCheck: Discarding due to domainname case change in "
                        "%d slot %3d in %d %d", NextCacheCheckEvent(cr) - m->timenow, slot,
                        m->rrcache_nextcheck[slot] - m->timenow, m->NextCacheCheck - m->timenow);
                    // DO NOT break out here -- we want to continue as if we never found it
                }
                else if (m->rec.r.resrec.rroriginalttl > 0)
                {
                    DNSQuestion *q;

                    m->mDNSStats.CacheRefreshed++;

                    if ((cr->resrec.mortality == Mortality_Ghost) && !cr->DelayDelivery)
                    {
                        cr->DelayDelivery = NonZeroTime(m->timenow);
                        debugf("mDNSCoreReceiveCacheCheck: Reset DelayDelivery for mortalityExpired EXP:%d RR %s", m->timenow - RRExpireTime(cr), CRDisplayString(m, cr));
                    }

                    if (cr->resrec.rroriginalttl == 0) debugf("uDNS rescuing %s", CRDisplayString(m, cr));
                    mDNSu32 newCRTTL = m->rec.r.resrec.rroriginalttl;
                #if MDNSRESPONDER_SUPPORTS(APPLE, AWDL)
                    if (cr->resrec.rroriginalttl == kStandardTTL            &&
                        m->rec.r.resrec.rroriginalttl == kHostNameTTL       &&
                        mDNSPlatformInterfaceIsAWDL(cr->resrec.InterfaceID) &&
                        (cr->resrec.rrtype == kDNSType_SRV || cr->resrec.rrtype == kDNSType_AAAA))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,  "mDNSCoreReceiveCacheCheck: Keeping Standard TTL for " PRI_S " %p",
                                  CRDisplayString(m, cr), cr->resrec.InterfaceID);
                        newCRTTL = kStandardTTL;
                    }
                #endif
                    RefreshCacheRecord(m, cr, newCRTTL);
                    // RefreshCacheRecordCacheGroupOrder will modify the cache group member list that is currently being iterated over in this for-loop.
                    // It is safe to call because the else-if body will unconditionally break out of the for-loop now that it has found the entry to update.
                    RefreshCacheRecordCacheGroupOrder(cg, cr);
                    CacheRecordSetResponseFlags(cr, response->h.flags);

                    // If we may have NSEC records returned with the answer (which we don't know yet as it
                    // has not been processed), we need to cache them along with the first cache
                    // record in the list that answers the question so that it can be used for validation
                    // later. The "type" check below is to make sure that we cache on the cache record
                    // that would answer the question. It is possible that we might cache additional things
                    // e.g., MX question might cache A records also, and we want to cache the NSEC on
                    // the record that answers the question.
                    if (!InterfaceID)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                            "mDNSCoreReceiveCacheCheck: rescuing RR with new TTL %u: " PRI_S,
                            cr->resrec.rroriginalttl, CRDisplayString(m, cr));
                    #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                        if (resource_record_is_to_be_dnssec_validated(&cr->resrec) && cr->CRActiveQuestion != mDNSNULL)
                        {
                            // We have update the "DNSSEC to be validated" record's TTL, schedule a cache update for
                            // the "DNSSEC validated" record so their expiration time can also be updated.
                            schedule_next_validated_cache_update(m);
                        }
                    #endif
                    }
                    // We have to reset the question interval to MaxQuestionInterval so that we don't keep
                    // polling the network once we get a valid response back. For the first time when a new
                    // cache entry is created, AnswerCurrentQuestionWithResourceRecord does that.
                    // Subsequently, if we reissue questions from within the mDNSResponder e.g., DNS server
                    // configuration changed, without flushing the cache, we reset the question interval here.
                    // Currently, we do this for for both multicast and unicast questions as long as the record
                    // type is unique. For unicast, resource record is always unique and for multicast it is
                    // true for records like A etc. but not for PTR.
                    if (cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask)
                    {
                        for (q = m->Questions; q; q=q->next)
                        {
                            if (!q->DuplicateOf && !q->LongLived &&
                                ActiveQuestion(q) && CacheRecordAnswersQuestion(cr, q))
                            {
                                ResetQuestionState(m, q);
                                debugf("mDNSCoreReceiveCacheCheck: Set MaxQuestionInterval for %p %##s (%s)", q, q->qname.c, DNSTypeName(q->qtype));
                                break;      // Why break here? Aren't there other questions we might want to look at?-- SC July 2010
                            }
                        }
                    }
                    break;  // Check usage of RefreshCacheRecordCacheGroupOrder before removing (See note above)
                }
                else
                {
                    // If the packet TTL is zero, that means we're deleting this record.
                    // To give other hosts on the network a chance to protest, we push the deletion
                    // out one second into the future. Also, we set UnansweredQueries to MaxUnansweredQueries.
                    // Otherwise, we'll do final queries for this record at 80% and 90% of its apparent
                    // lifetime (800ms and 900ms from now) which is a pointless waste of network bandwidth.
                    // If record's current expiry time is more than a second from now, we set it to expire in one second.
                    // If the record is already going to expire in less than one second anyway, we leave it alone --
                    // we don't want to let the goodbye packet *extend* the record's lifetime in our cache.

                    if (RRExpireTime(cr) - m->timenow > mDNSPlatformOneSecond)
                    {
                        const ResourceRecord *const resrec = &cr->resrec;

                        // Since the Goodbye packet we care about is the PTR Goodbye packet, which tells us what
                        // host is deregistering the record, only print the domain name in the RDATA if the RDATA type
                        // is PTR.
                        const domainname *ptrDomain = mDNSNULL;
                        if (resrec->rrtype == kDNSType_PTR)
                        {
                            ptrDomain = &resrec->rdata->u.name;
                        }
                        const mDNSu32 nameHash =  mDNS_DomainNameFNV1aHash(resrec->name);
                        const mDNSu32 ptrNameHash = (ptrDomain != mDNSNULL) ? mDNS_DomainNameFNV1aHash(ptrDomain) : 0;
                        struct timeval now;
                        mDNSGetTimeOfDay(&now, mDNSNULL);
                        const mDNSu32 ifIndex = mDNSPlatformInterfaceIndexfromInterfaceID(m, InterfaceID, mDNStrue);
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                            "Received Goodbye packet for cached record -- "
                            "name hash: %x, type: " PUB_DNS_TYPE ", last time received: " PUB_TIMEV
                            ", interface index: %d, source address: " PRI_IP_ADDR ", name hash if PTR: %x",
                            nameHash, DNS_TYPE_PARAM(resrec->rrtype), TIMEV_PARAM(&now), ifIndex, &cr->sourceAddress,
                            ptrNameHash);

                        cr->resrec.rroriginalttl = 1;
                        cr->TimeRcvd = m->timenow;
                        cr->UnansweredQueries = MaxUnansweredQueries;
                        SetNextCacheCheckTimeForRecord(m, cr);
                    }
                    break;
                }
            }
            else
            {
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                // The DNSSEC status has to be the same to be comparable.
                if (!resource_records_have_same_dnssec_rr_category(&m->rec.r.resrec, &cr->resrec))
                {
                    continue;
                }
            #endif

                // If the cache record rrtype doesn't match and one is a CNAME, then flush this record.
                if ((m->rec.r.resrec.rrtype == kDNSType_CNAME || cr->resrec.rrtype == kDNSType_CNAME))
                {
                    // Do not mark it to be flushable if it has already been marked as flushable.
                    if (cr->resrec.rroriginalttl == 0)
                    {
                        continue;
                    }
                    // Do not mark it to be flushable if they have the same name and type, but different data.
                    // The only case we want to handle here is the CNAME exclusion.
                    // The same name and type, but different data case is handled by CacheFlushRecords part of
                    // mDNSCoreReceiveResponse().
                    if (m->rec.r.resrec.rrtype == cr->resrec.rrtype)
                    {
                        continue;
                    }

                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    // IN DNSSEC, a CNAME record can coexist with:
                    // 1. RRSIGs that cover it.
                    // 2. Negative records that indicate the CNAME is a wildcard answer.

                    // Therefore, do not flush the RRSIG that covers the CNAME record that accompanies this CNAME.
                    if (resource_record_as_rrsig_covers_rr_type(&m->rec.r.resrec, kDNSType_CNAME) ||
                        resource_record_as_rrsig_covers_rr_type(&cr->resrec, kDNSType_CNAME))
                    {
                        continue;
                    }
                    // Do not flush the denial of existence record that accompanies the CNAME record.
                    if (resource_record_as_denial_of_existence_proves_wildcard_answer(&m->rec.r.resrec) ||
                        resource_record_as_denial_of_existence_proves_wildcard_answer(&cr->resrec))
                    {
                        continue;
                    }
                #endif

                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                        "mDNSCoreReceiveCacheCheck: Discarding (" PUB_S ") " PRI_S " rrtype change from (" PUB_S PUB_S ") to (" PUB_S PUB_S ")",
                        MortalityDisplayString(cr->resrec.mortality), CRDisplayString(m, cr),
                        DNSTypeName(cr->resrec.rrtype), (cr->resrec.RecordType == kDNSRecordTypePacketNegative) ? ", Negative" : "",
                        DNSTypeName(m->rec.r.resrec.rrtype), (m->rec.r.resrec.RecordType == kDNSRecordTypePacketNegative) ? ", Negative" : "");
                    mDNS_PurgeCacheResourceRecord(m, cr);
                    // DO NOT break out here -- we want to continue iterating the cache entries.
                }
            }
        }
    }
    return cr;
}

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

// A record set is a group of records received in the same response that have the same name and type.
typedef struct
{
    const domainname *name;
    mDNSu32 namehash;
    mDNSu32 sizeOfRecordSet;    // The total number of records with the same name and type added to the cache.
    mDNSu16 rrtype;             // The DNS type that this record set has in common. The rrtype of the RRSIG should be
                                // treated as the type it covers.
    mDNSBool noNewCachedRecordAdded; // If new cached record has been added.
} RecordSet;

mDNSlocal mDNSBool RecordInTheRRSet(const ResourceRecord * const rr, const RecordSet * const rrset)
{
    const mDNSBool typeMatchesDirectly = (rrset->rrtype == rr->rrtype);
    const mDNSBool rrsigCoversType = resource_record_as_rrsig_covers_rr_type(rr, rrset->rrtype);
    const mDNSBool inTheSameSet = typeMatchesDirectly || rrsigCoversType;

    if (!inTheSameSet)
    {
        return mDNSfalse;
    }
    if (rrset->namehash != rr->namehash)
    {
        return mDNSfalse;
    }
    if (!SameDomainName(rrset->name, rr->name))
    {
        return mDNSfalse;
    }

    return mDNStrue;
}

#endif

mDNSlocal mDNSBool SameNameCacheRecordsMatchInSourceTypeClass(const CacheRecord *const cr1, const CacheRecord *const cr2)
{
    const ResourceRecord *const rr1 = &cr1->resrec;
    const ResourceRecord *const rr2 = &cr2->resrec;

    if (rr1->InterfaceID != rr2->InterfaceID)
    {
        return mDNSfalse;
    }

    if (!rr1->InterfaceID)
    {
        // For Unicast (null InterfaceID) the resolver IDs should also match
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        if (!mdns_cache_metadata_same_dns_service(rr1->metadata, rr2->metadata))
        {
            return mDNSfalse;
        }
#else
        const mDNSu32 id1 = (rr1->rDNSServer ? rr1->rDNSServer->resGroupID : 0);
        const mDNSu32 id2 = (rr2->rDNSServer ? rr2->rDNSServer->resGroupID : 0);
        if (id1 != id2)
        {
            return mDNSfalse;
        }
#endif
    }

    if ((rr1->rrtype != rr2->rrtype) || (rr1->rrclass != rr2->rrclass))
    {
        return mDNSfalse;
    }

    return mDNStrue;
}

// Note: mDNSCoreReceiveResponse calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
// InterfaceID non-NULL tells us the interface this multicast response was received on
// InterfaceID NULL tells us this was a unicast response
// dstaddr NULL tells us we received this over an outgoing TCP connection we made
mDNSlocal void mDNSCoreReceiveResponse(mDNS *const m, const DNSMessage *const response, const mDNSu8 *end,
    const mDNSAddr *srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, mDNSIPPort dstport,
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_client_t client, mdns_dns_service_t uDNSService,
#endif
    const mDNSInterfaceID InterfaceID)
{
    int i;
    const mDNSBool ResponseMCast    = dstaddr && mDNSAddrIsDNSMulticast(dstaddr);
    const mDNSBool ResponseSrcLocal = !srcaddr || mDNS_AddressIsLocalSubnet(m, InterfaceID, srcaddr);
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
    DNSQuestion *llqMatch = mDNSNULL;
    uDNS_LLQType LLQType      = uDNS_recvLLQResponse(m, response, end, srcaddr, srcport, &llqMatch);
#else
    uDNS_LLQType LLQType = uDNS_LLQ_Not;
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)

    // "(CacheRecord*)1" is a special (non-zero) end-of-list marker
    // We use this non-zero marker so that records in our CacheFlushRecords list will always have NextInCFList
    // set non-zero, and that tells GetCacheEntity() that they're not, at this moment, eligible for recycling.
    CacheRecord *CacheFlushRecords = (CacheRecord*)1;
    CacheRecord **cfp = &CacheFlushRecords;
    NetworkInterfaceInfo *llintf = FirstIPv4LLInterfaceForID(m, InterfaceID);
    mDNSBool    recordAcceptedInResponse = mDNSfalse; // Set if a record is accepted from a unicast mDNS response that answers an existing question.

    // All records in a DNS response packet are treated as equally valid statements of truth. If we want
    // to guard against spoof responses, then the only credible protection against that is cryptographic
    // security, e.g. DNSSEC., not worrying about which section in the spoof packet contained the record.
    int firstauthority  =                   response->h.numAnswers;
    int firstadditional = firstauthority  + response->h.numAuthorities;
    int totalrecords    = firstadditional + response->h.numAdditionals;
    const mDNSu8 *ptr   = mDNSNULL;
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    DNSServer *uDNSServer = mDNSNULL;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_cache_metadata_t cache_metadata = mDNSNULL;
    const mdns_querier_t querier = mdns_querier_downcast(client);
    const mdns_subscriber_t subscriber = mdns_subscriber_downcast(client);
    const mDNSBool ResponseIsMDNS = IsResponseMDNSEquivalent(client, uDNSService);
    const mDNSBool subscriberResponse = (subscriber != mDNSNULL);
#else
    const mDNSBool ResponseIsMDNS = mDNSOpaque16IsZero(response->h.id);
    const mDNSBool subscriberResponse = mDNSfalse;
#endif

    mDNSBool dumpMDNSPacket = mDNSfalse;

    TSRDataRecHead tsrs = SLIST_HEAD_INITIALIZER(tsrs);
    const TSROptData *curTSRForName = mDNSNULL;

    debugf("Received Response from %#-15a addressed to %#-15a on %p with "
           "%2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s %d bytes LLQType %d",
           srcaddr, dstaddr, InterfaceID,
           response->h.numQuestions,   response->h.numQuestions   == 1 ? ", "   : "s,",
           response->h.numAnswers,     response->h.numAnswers     == 1 ? ", "   : "s,",
           response->h.numAuthorities, response->h.numAuthorities == 1 ? "y,  " : "ies,",
           response->h.numAdditionals, response->h.numAdditionals == 1 ? " "    : "s", end - response->data, LLQType);

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS) && !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    if (mDNSSameIPPort(srcport, UnicastDNSPort))
    {
        bool isForCell  = (m->rec.r.resrec.rDNSServer && m->rec.r.resrec.rDNSServer->isCell);
        dnssd_analytics_update_dns_reply_size(isForCell, dns_transport_Do53, (uint32_t)(end - (mDNSu8 *)response));
    }
#endif

    // According to RFC 2181 <http://www.ietf.org/rfc/rfc2181.txt>
    //    When a DNS client receives a reply with TC
    //    set, it should ignore that response, and query again, using a
    //    mechanism, such as a TCP connection, that will permit larger replies.
    // It feels wrong to be throwing away data after the network went to all the trouble of delivering it to us, but
    // delivering some records of the RRSet first and then the remainder a couple of milliseconds later was causing
    // failures in our Microsoft Active Directory client, which expects to get the entire set of answers at once.
    // <rdar://problem/6690034> Can't bind to Active Directory
    // In addition, if the client immediately canceled its query after getting the initial partial response, then we'll
    // abort our TCP connection, and not complete the operation, and end up with an incomplete RRSet in our cache.
    // Next time there's a query for this RRSet we'll see answers in our cache, and assume we have the whole RRSet already,
    // and not even do the TCP query.
    // Accordingly, if we get a uDNS reply with kDNSFlag0_TC set, we bail out and wait for the TCP response containing the
    // entire RRSet, with the following exception. If the response contains an answer section and one or more records in
    // either the authority section or additional section, then that implies that truncation occurred beyond the answer
    // section, and the answer section is therefore assumed to be complete.
    //
    // From section 6.2 of RFC 1035 <https://tools.ietf.org/html/rfc1035>:
    //    When a response is so long that truncation is required, the truncation
    //    should start at the end of the response and work forward in the
    //    datagram.  Thus if there is any data for the authority section, the
    //    answer section is guaranteed to be unique.
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    if (!InterfaceID && (response->h.flags.b[0] & kDNSFlag0_TC) && !client &&
#else
    if (!InterfaceID && (response->h.flags.b[0] & kDNSFlag0_TC) &&
#endif
        ((response->h.numAnswers == 0) || ((response->h.numAuthorities == 0) && (response->h.numAdditionals == 0))))
    {
        goto exit;
    }

    mdns_require_quiet(LLQType != uDNS_LLQ_Ignore, exit);

    // Look in Additional Section for an OPT record
    ptr = LocateOptRR(response, end, DNSOpt_TSRData_Space);
    if (ptr)
    {
        ptr = GetLargeResourceRecord(m, response, ptr, end, InterfaceID, kDNSRecordTypePacketAdd, &m->rec);
        if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative && m->rec.r.resrec.rrtype == kDNSType_OPT)
        {
            const rdataOPT *opt;
            const rdataOPT *const e = (const rdataOPT *)&m->rec.r.resrec.rdata->u.data[m->rec.r.resrec.rdlength];
            mDNSu8 tsrsCount = 0;
            for (opt = &m->rec.r.resrec.rdata->u.opt[0]; opt < e; opt++)
            {
                if (opt->opt == kDNSOpt_TSR)
                {
                    tsrsCount++;
                    const mDNSu8 *name_ptr;
                    if ((name_ptr = DomainNamePtrAtTSRIndex(response, end, opt->u.tsr.recIndex)))
                    {
                        struct TSRDataRec *newTSR = TSRDataRecCreate(response, name_ptr, end, opt);
                        if (!newTSR)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                                "mDNSCoreReceiveResponse: Create TSR(%u) failed - if %p tsrTime %d tsrHost %x recIndex %d",
                                tsrsCount, m->rec.r.resrec.InterfaceID, opt->u.tsr.timeStamp, opt->u.tsr.hostkeyHash,
                                opt->u.tsr.recIndex);
                            continue;
                        }
                        SLIST_INSERT_HEAD(&tsrs, newTSR, entries);
                    }
                    else
                    {
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                            "mDNSCoreReceiveResponse: No Domain Name for TSR(%u) if %p tsrTime %d tsrHost %x recIndex %d",
                            tsrsCount, m->rec.r.resrec.InterfaceID, opt->u.tsr.timeStamp, opt->u.tsr.hostkeyHash,
                            opt->u.tsr.recIndex);
                    }
                }
            }
            if (!SLIST_EMPTY(&tsrs))
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG,
                    "mDNSCoreReceiveResponse: Received TSR(%u) if %p " PUB_S,
                    tsrsCount, m->rec.r.resrec.InterfaceID, RRDisplayString(m, &m->rec.r.resrec));
            }
       }
        mDNSCoreResetRecord(m);
    }

    // 1. We ignore questions (if any) in mDNS response packets
    // 2. If this is an LLQ response, we handle it much the same
    // Otherwise, this is a authoritative uDNS answer, so arrange for any stale records to be purged
    if (ResponseMCast || LLQType == uDNS_LLQ_Events)
        ptr = LocateAnswers(response, end);
    // Otherwise, for one-shot queries, any answers in our cache that are not also contained
    // in this response packet are immediately deemed to be invalid.
    else
    {
        mDNSBool failure, returnEarly;
        const int rcode = response->h.flags.b[1] & kDNSFlag1_RC_Mask;
        failure = !(rcode == kDNSFlag1_RC_NoErr || rcode == kDNSFlag1_RC_NXDomain || rcode == kDNSFlag1_RC_NotAuth);
        returnEarly = mDNSfalse;
        ptr = response->data;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        // When the QUERIER functionality is enabled, DNS transport is handled exclusively by querier objects. If this
        // response was provided by a querier, but the RCODE is considered a failure, then set failure to false so that
        // we don't return early. The logic of returning early was so that uDNS_CheckCurrentQuestion() could handle
        // resending the query and generate a negative cache record if all servers were tried. If the querier provides a
        // response, then it's the best response that it could provide. If the RCODE is considered a failure,
        // mDNSCoreReceiveResponse() needs to create negative cache entries for the unanwered question, so totalrecords
        // is set to 0 to ignore any records that the response may contain.
        if (querier && failure)
        {
            totalrecords = 0;
            failure = mDNSfalse;
        }
#endif
        // We could possibly combine this with the similar loop at the end of this function --
        // instead of tagging cache records here and then rescuing them if we find them in the answer section,
        // we could instead use the "m->PktNum" mechanism to tag each cache record with the packet number in
        // which it was received (or refreshed), and then at the end if we find any cache records which
        // answer questions in this packet's question section, but which aren't tagged with this packet's
        // packet number, then we deduce they are old and delete them
        // Additional condition to check: a response comes from subscriber does not flush previous records, so skip it.
        //
        // Note: the for loop below must be executed even when we don't need to flush previous records, because we use
        // "getQuestion" to advance the response read pointer.
        for (i = 0; i < response->h.numQuestions && ptr && ptr < end; i++)
        {
            DNSQuestion q;
            DNSQuestion *qptr;
            mDNSBool expectingResponse;
            ptr = getQuestion(response, ptr, end, InterfaceID, &q);
            if (!ptr || subscriberResponse)
            {
                continue;
            }
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            if (client)
            {
                expectingResponse = mDNStrue;
                qptr = mDNSNULL;
            }
            else
#endif
            {
                qptr = ExpectingUnicastResponseForQuestion(m, dstport, response->h.id, &q, !dstaddr);
                expectingResponse = qptr ? mDNStrue : mDNSfalse;
            }
            if (!expectingResponse)
            {
                continue;
            }
            if (!failure)
            {
                CacheRecord *cr;
                CacheGroup *cg = CacheGroupForName(m, q.qnamehash, &q.qname);
                for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
                {
                    mDNSBool isAnswer;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                    if (client)
                    {
                        const mdns_dns_service_t dnsservice = mdns_cache_metadata_get_dns_service(cr->resrec.metadata);
                        isAnswer = (dnsservice == uDNSService) && Client_SameNameCacheRecordIsAnswer(cr, client);
                    }
                    else
#endif
                    {
                        isAnswer = SameNameCacheRecordAnswersQuestion(cr, qptr);
                    }
                    mDNSBool flushable = isAnswer;
                #if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
                    // Push subscribed records will never be flushed by a response (except "remove" push notification).
                    flushable = flushable && (!cr->DNSPushSubscribed);
                #endif
                    if (flushable)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
                            "Making record answered by the current response as expired if it is not refreshed in the response - "
                            "Q interface ID: %p, qname: " PRI_DM_NAME ", qtype: " PRI_S
                            ", RR interface ID: %p, RR description: " PRI_S ".", q.InterfaceID, DM_NAME_PARAM(&q.qname),
                            DNSTypeName(q.qtype), cr->resrec.InterfaceID, CRDisplayString(m, cr));

                        // Don't want to disturb rroriginalttl here, because code below might need it for the exponential backoff doubling algorithm
                        cr->TimeRcvd          = m->timenow - TicksTTL(cr) - 1;
                        cr->UnansweredQueries = MaxUnansweredQueries;
                    }
                }
            }
            else
            {
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->Q%d] mDNSCoreReceiveResponse: Server %p responded with code %d to query "
                    PRI_DM_NAME " (" PUB_S ")", qptr->request_id, mDNSVal16(qptr->TargetQID), qptr->qDNSServer, rcode,
                    DM_NAME_PARAM(&q.qname), DNSTypeName(q.qtype));
                PenalizeDNSServer(m, qptr, response->h.flags);
#endif
                returnEarly = mDNStrue;
            }
        }
        if (returnEarly)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[Q%d] Ignoring %2d Answer" PUB_S " %2d Authorit" PUB_S " %2d Additional" PUB_S,
                mDNSVal16(response->h.id),
                response->h.numAnswers,     response->h.numAnswers     == 1 ? ", " : "s,",
                response->h.numAuthorities, response->h.numAuthorities == 1 ? "y,  " : "ies,",
                response->h.numAdditionals, response->h.numAdditionals == 1 ? "" : "s");
            // not goto exit because we won't have any CacheFlushRecords and we do not want to
            // generate negative cache entries (we want to query the next server)
            goto exit;
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        // Use this CacheRecord array to track all the answer records we have added into cache. Currently, we allow at
        // most 100 records to be contained in the answer section of the response.
#define MAX_RESPONSE_RECORDS 100
        CacheRecord *cachedRecords[MAX_RESPONSE_RECORDS];
        mDNSu32 numOfCachedRecords = 0; // The total number of records added to the cache.
        RecordSet recordSets[MAX_RESPONSE_RECORDS];
        mDNSu32 numOfRecordSets = 0;

        // Because DNSSEC signs record with the same type, when we are counting the number of records with within the
        // same response, we should separate them by names and types.
        // For example, if we received a CNAME response like this:
        // =============================================================================================================
        // www.apple.com.                                       300     IN CNAME www.apple.com.edgekey.net.
        // www.apple.com.edgekey.net.                           17716   IN CNAME www.apple.com.edgekey.net.globalredir.akadns.net.
        // www.apple.com.edgekey.net.globalredir.akadns.net.    3307    IN CNAME e6858.dscx.akamaiedge.net.
        // RRSIG 1 for www.apple.com. CNAME
        // RRSIG 2 for www.apple.com. CNAME
        // RRSIG for www.apple.com.edgekey.net. CNAME
        // RRSIG for www.apple.com.edgekey.net.globalredir.akadns.net. CNAME
        // e6858.dscx.akamaiedge.net.                           6       IN A     104.123.204.248
        // RRSIG for e6858.dscx.akamaiedge.net. A
        // =============================================================================================================

        // Instead of treating the response as a single response set with 9 records inside.
        // It should be treated as 4 different responses:
        // =============================================================================================================
        // www.apple.com.                                       300     IN CNAME www.apple.com.edgekey.net.
        // RRSIG 1 for www.apple.com. CNAME
        // RRSIG 2 for www.apple.com. CNAME
        // =============================================================================================================

        // =============================================================================================================
        // www.apple.com.edgekey.net.                           17716   IN CNAME www.apple.com.edgekey.net.globalredir.akadns.net.
        // RRSIG for www.apple.com.edgekey.net. CNAME
        // =============================================================================================================

        // =============================================================================================================
        // www.apple.com.edgekey.net.globalredir.akadns.net.    3307    IN CNAME e6858.dscx.akamaiedge.net.
        // RRSIG for www.apple.com.edgekey.net.globalredir.akadns.net. CNAME
        // =============================================================================================================

        // =============================================================================================================
        // e6858.dscx.akamaiedge.net.                           6       IN A     104.123.204.248
        // RRSIG for e6858.dscx.akamaiedge.net. A
        // =============================================================================================================

        // A similar example can be found when mDNSResponder receives a MX response that contains the A/AAAA records of
        // the MX name. The A/AAAA record set needs to be separated with the MX record set. Therefore, we need to count
        // the record set size that has the same name and same type, including the RRSIG that covers the same type.

        const mDNSBool toBeDNSSECValidated = (!ResponseIsMDNS) &&
                            (
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                                (querier != mDNSNULL) ?
                                (mdns_querier_get_dnssec_ok(querier) && mdns_querier_get_checking_disabled(querier)) :
#endif
                                mDNSfalse
                            );

#endif  // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

    for (i = 0; i < totalrecords && ptr && ptr < end; i++)
    {
        // All responses sent via LL multicast are acceptable for caching
        // All responses received over our outbound TCP connections are acceptable for caching
        // We accept all records in a unicast response to a multicast query once we find one that
        // answers an active question.
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        mDNSBool AcceptableResponse = ResponseMCast || (!client && !dstaddr) || LLQType || recordAcceptedInResponse;
#else
        mDNSBool AcceptableResponse = ResponseMCast || !dstaddr || LLQType || recordAcceptedInResponse;
#endif
        // (Note that just because we are willing to cache something, that doesn't necessarily make it a trustworthy answer
        // to any specific question -- any code reading records from the cache needs to make that determination for itself.)

        const mDNSu8 RecordType =
            (i < firstauthority ) ? (mDNSu8)kDNSRecordTypePacketAns  :
            (i < firstadditional) ? (mDNSu8)kDNSRecordTypePacketAuth : (mDNSu8)kDNSRecordTypePacketAdd;
        ptr = GetLargeResourceRecord(m, response, ptr, end, InterfaceID, RecordType, &m->rec);
        mdns_require_quiet(ptr, bail); // Break out of the loop and clean up our CacheFlushRecords list before exiting

    #if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
        // If the message being processed is a true mDNS message, i.e., it was received via the mDNS protocol, then
        // don't cache the record if there's already a Discovery Proxy subscription for the resource record set because
        // the Discovery Proxy is our current source of truth.
        if (DPCFeatureEnabled() && ResponseIsMDNS && !subscriber)
        {
            const ResourceRecord *const rr = &m->rec.r.resrec;
            if (DPCHaveSubscriberForRecord(InterfaceID, rr->name, rr->rrtype, rr->rrclass))
            {
                mDNSCoreResetRecord(m);
                continue;
            }
        }
    #endif
    #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        if (querier || subscriber)
        {
            if (!cache_metadata)
            {
                cache_metadata = mdns_cache_metadata_create();
                if (querier)
                {
                    mdns_resolver_type_t resolver_type = mdns_querier_get_resolver_type(querier);
                    if (resolver_type == mdns_resolver_type_normal)
                    {
                        if (mdns_querier_get_over_tcp_reason(querier) != mdns_query_over_tcp_reason_null)
                        {
                            resolver_type = mdns_resolver_type_tcp;
                        }
                    }
                    mdns_cache_metadata_set_protocol(cache_metadata, resolver_type);
                    mdns_cache_metadata_set_extended_dns_error(cache_metadata, mdns_querier_get_extended_dns_error(querier));
                }
                if (subscriber)
                {
                    mdns_cache_metadata_set_subscriber_id(cache_metadata, mdns_subscriber_get_id(subscriber));
                }
                mdns_cache_metadata_set_dns_service(cache_metadata, uDNSService);
            }
            ResourceRecord *const rr = &m->rec.r.resrec;
            mdns_replace(&rr->metadata, cache_metadata);
        }
    #endif
        if (m->rec.r.resrec.RecordType == kDNSRecordTypePacketNegative)
        {
            mDNSCoreResetRecord(m);
            continue;
        }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        // Mark the parsed record as DNSSEC-aware(can be used for DNSSEC validation) if it applies.
        if (toBeDNSSECValidated)
        {
            m->rec.r.resrec.dnssec = dnssec_obj_resource_record_member_create_to_validate(mDNStrue, &m->rec.r, NULL);
        }
#endif

        // Don't want to cache OPT or TSIG pseudo-RRs
        if (m->rec.r.resrec.rrtype == kDNSType_TSIG)
        {
            mDNSCoreResetRecord(m);
            continue;
        }
        if (m->rec.r.resrec.rrtype == kDNSType_OPT)
        {
            const rdataOPT *opt;
            const rdataOPT *const e = (const rdataOPT *)&m->rec.r.resrec.rdata->u.data[m->rec.r.resrec.rdlength];
            // Find owner sub-option(s). We verify that the MAC is non-zero, otherwise we could inadvertently
            // delete all our own AuthRecords (which are identified by having zero MAC tags on them).
            for (opt = &m->rec.r.resrec.rdata->u.opt[0]; opt < e; opt++)
                if (opt->opt == kDNSOpt_Owner && opt->u.owner.vers == 0 && opt->u.owner.HMAC.l[0])
                {
                    ClearProxyRecords(m, &opt->u.owner, m->DuplicateRecords);
                    ClearProxyRecords(m, &opt->u.owner, m->ResourceRecords);
                }
            mDNSCoreResetRecord(m);
            continue;
        }
        // if a CNAME record points to itself, then don't add it to the cache
        if ((m->rec.r.resrec.rrtype == kDNSType_CNAME) && SameDomainName(m->rec.r.resrec.name, &m->rec.r.resrec.rdata->u.name))
        {
            LogInfo("mDNSCoreReceiveResponse: CNAME loop domain name %##s", m->rec.r.resrec.name->c);
            mDNSCoreResetRecord(m);
            continue;
        }

        // When we receive uDNS LLQ responses, we assume a long cache lifetime --
        // In the case of active LLQs, we'll get remove events when the records actually do go away
        // In the case of polling LLQs, we assume the record remains valid until the next poll
        if (!ResponseIsMDNS)
        {
            m->rec.r.resrec.rroriginalttl = GetEffectiveTTL(LLQType, m->rec.r.resrec.rroriginalttl);
        }

        // If response was not sent via LL multicast,
        // then see if it answers a recent query of ours, which would also make it acceptable for caching.
        if (!ResponseMCast)
        {
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
            if (LLQType)
            {
                // For Long Lived queries that are both sent over UDP and Private TCP, LLQType is set.
                // Even though it is AcceptableResponse, we need a matching DNSServer pointer for the
                // queries to get ADD/RMV events. To lookup the question, we can't use
                // ExpectingUnicastResponseForRecord as the port numbers don't match. uDNS_recvLLQRespose
                // has already matched the question using the 64 bit Id in the packet and we use that here.

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                if (llqMatch != mDNSNULL) m->rec.r.resrec.rDNSServer = uDNSServer = llqMatch->qDNSServer;
#endif
            }
            else
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)

                if (!AcceptableResponse || !dstaddr)
            {
                // For responses that come over TCP (Responses that can't fit within UDP) or TLS (Private queries
                // that are not long lived e.g., AAAA lookup in a Private domain), it is indicated by !dstaddr.
                // Even though it is AcceptableResponse, we still need a DNSServer pointer for the resource records that
                // we create.
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                if (client)
                {
                    ResourceRecord *const rr = &m->rec.r.resrec;
                    if (Client_ResourceRecordIsAnswer(rr, client))
                    {
                        AcceptableResponse = mDNStrue;
                    }
                }
                else
#endif
                {
                    const DNSQuestion *q;
                    // Initialize the DNS server on the resource record which will now filter what questions we answer with
                    // this record.
                    //
                    // We could potentially lookup the DNS server based on the source address, but that may not work always
                    // and that's why ExpectingUnicastResponseForRecord does not try to verify whether the response came
                    // from the DNS server that queried. We follow the same logic here. If we can find a matching quetion based
                    // on the "id" and "source port", then this response answers the question and assume the response
                    // came from the same DNS server that we sent the query to.
                    q = ExpectingUnicastResponseForRecord(m, srcaddr, ResponseSrcLocal, dstport, response->h.id, &m->rec.r, !dstaddr);
                    if (q != mDNSNULL)
                    {
                        AcceptableResponse = mDNStrue;
                        if (!InterfaceID)
                        {
                            debugf("mDNSCoreReceiveResponse: InterfaceID %p %##s (%s)", q->InterfaceID, q->qname.c, DNSTypeName(q->qtype));
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                            m->rec.r.resrec.rDNSServer = uDNSServer = q->qDNSServer;
#endif
                        }
                        else
                        {
                            // Accept all remaining records in this unicast response to an mDNS query.
                            recordAcceptedInResponse = mDNStrue;
                            // Response that is a unicast assisted mDNS Response should be logged.
                            dumpMDNSPacket = mDNStrue;
                        }
                    }
                    else
                    {
                        // If we can't find a matching question, we need to see whether we have seen records earlier that matched
                        // the question. The code below does that. So, make this record unacceptable for now
                        if (!InterfaceID)
                        {
                            debugf("mDNSCoreReceiveResponse: Can't find question for record name %##s", m->rec.r.resrec.name->c);
                            AcceptableResponse = mDNSfalse;
                        }
                    }
                }
            }
        }
        else if (llintf && llintf->IgnoreIPv4LL && m->rec.r.resrec.rrtype == kDNSType_A)
        {
            // There are some routers (rare, thankfully) that generate bogus ARP responses for
            // any IPv4 address they don’t recognize, including RFC 3927 IPv4 link-local addresses.
            // To work with these broken routers, client devices need to blacklist these broken
            // routers and ignore their bogus ARP responses. Some devices implement a technique
            // such as the one described in US Patent 7436783, which lets clients detect and
            // ignore these broken routers: <https://www.google.com/patents/US7436783>

            // OS X and iOS do not implement this defensive mechanism, instead taking a simpler
            // approach of just detecting these broken routers and completely disabling IPv4
            // link-local communication on interfaces where a broken router is detected.
            // OS X and iOS set the IFEF_ARPLL interface flag on interfaces
            // that are deemed “safe” for IPv4 link-local communication;
            // the flag is cleared on interfaces where a broken router is detected.

            // OS X and iOS will not even try to communicate with an IPv4
            // link-local destination on an interface without the IFEF_ARPLL flag set.
            // This can cause some badly written applications to freeze for a long time if they
            // attempt to connect to an IPv4 link-local destination address and then wait for
            // that connection attempt to time out before trying other candidate addresses.

            // To mask this client bug, we suppress acceptance of IPv4 link-local address
            // records on interfaces where we know the OS will be unwilling even to attempt
            // communication with those IPv4 link-local destination addresses.
            // <rdar://problem/9400639> kSuppress IPv4LL answers on interfaces without IFEF_ARPLL

            const CacheRecord *const rr = &m->rec.r;
            const RDataBody2 *const rdb = (const RDataBody2 *)rr->smallrdatastorage.data;
            if (mDNSv4AddressIsLinkLocal(&rdb->ipv4))
            {
                LogInfo("mDNSResponder: Dropping LinkLocal packet %s", CRDisplayString(m, &m->rec.r));
                mDNSCoreResetRecord(m);
                continue;
            }
        }

        // 1. Check that this packet resource record does not conflict with any of ours
        if (ResponseIsMDNS && m->rec.r.resrec.rrtype != kDNSType_NSEC)
        {
            if (m->CurrentRecord)
                LogMsg("mDNSCoreReceiveResponse ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
            m->CurrentRecord = m->ResourceRecords;
            while (m->CurrentRecord)
            {
                AuthRecord *rr = m->CurrentRecord;
                m->CurrentRecord = rr->next;
                // We accept all multicast responses, and unicast responses resulting from queries we issued
                // For other unicast responses, this code accepts them only for responses with an
                // (apparently) local source address that pertain to a record of our own that's in probing state
                if (!AcceptableResponse && !(ResponseSrcLocal && rr->resrec.RecordType == kDNSRecordTypeUnique)) continue;

                if (PacketRRMatchesSignature(&m->rec.r, rr))        // If interface, name, type (if shared record) and class match...
                {
                    if ((curTSRForName = TSRForNameFromDataRec(&tsrs, m->rec.r.resrec.name)) != mDNSNULL)
                    {
                        eTSRCheckResult tsrResult = CheckTSRForAuthRecord(m, curTSRForName, rr);
                        if (tsrResult == eTSRCheckLose)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                "mDNSCoreReceiveResponse - deregistering " PRI_DM_NAME " type " PUB_S " on interface %d due to TSR conflict",
                                DM_NAME_PARAM(rr->resrec.name), DNSTypeName(rr->resrec.rrtype), (int)IIDPrintable(InterfaceID));
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
                            // See if this record was also registered with any D2D plugins.
                            D2D_stop_advertising_record(rr);
#endif
                            mDNS_Deregister_internal(m, rr, mDNS_Dereg_stale);
                        }

                        if (tsrResult != eTSRCheckNoKeyMatch) // No else
                        {
                            continue;
                        }
                    }
                    // ... check to see if type and rdata are identical
                    if (IdenticalSameNameRecord(&m->rec.r.resrec, &rr->resrec))
                    {
                        // If the RR in the packet is identical to ours, just check they're not trying to lower the TTL on us
                        if (m->rec.r.resrec.rroriginalttl >= rr->resrec.rroriginalttl/2 || m->SleepState)
                        {
                            // If we were planning to send on this -- and only this -- interface, then we don't need to any more
                            if      (rr->ImmedAnswer == InterfaceID) { rr->ImmedAnswer = mDNSNULL; rr->ImmedUnicast = mDNSfalse; }
                        }
                        else
                        {
                            if      (rr->ImmedAnswer == mDNSNULL)    { rr->ImmedAnswer = InterfaceID;       m->NextScheduledResponse = m->timenow; }
                            else if (rr->ImmedAnswer != InterfaceID) { rr->ImmedAnswer = mDNSInterfaceMark; m->NextScheduledResponse = m->timenow; }
                        }
                    }
                    // else, the packet RR has different type or different rdata -- check to see if this is a conflict
                    else if (m->rec.r.resrec.rroriginalttl > 0 && PacketRRConflict(m, rr, &m->rec.r))
                    {
                        // Having a possible conflict, we should log the packet to check why it has possible
                        // conflicts with us later.
                        dumpMDNSPacket = mDNStrue;

                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                            "mDNSCoreReceiveResponse: Pkt Record: %08X " PRI_S " (interface %d)",
                            m->rec.r.resrec.rdatahash, CRDisplayString(m, &m->rec.r), IIDPrintable(InterfaceID));
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                            "mDNSCoreReceiveResponse: Our Record: %08X " PRI_S, rr->resrec.rdatahash,
                            ARDisplayString(m, rr));

                        // If this record is marked DependentOn another record for conflict detection purposes,
                        // then *that* record has to be bumped back to probing state to resolve the conflict
                        if (rr->DependentOn)
                        {
                            while (rr->DependentOn) rr = rr->DependentOn;
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                "mDNSCoreReceiveResponse: Dep Record: %08X " PRI_S, rr->resrec.rdatahash,
                                ARDisplayString(m, rr));
                        }

                        // If we've just whacked this record's ProbeCount, don't need to do it again
                        if (rr->ProbeCount > DefaultProbeCountForTypeUnique)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                "mDNSCoreReceiveResponse: Already reset to Probing: " PRI_S, ARDisplayString(m, rr));
                        }
                        else if (rr->ProbeCount == DefaultProbeCountForTypeUnique)
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                "mDNSCoreReceiveResponse: Ignoring response received before we even began probing: " PRI_S,
                                ARDisplayString(m, rr));
                        }
                        else
                        {
                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                "mDNSCoreReceiveResponse: Received from " PRI_IP_ADDR ":%d " PRI_S, srcaddr,
                                mDNSVal16(srcport), CRDisplayString(m, &m->rec.r));
                            // If we'd previously verified this record, put it back to probing state and try again
                            if (rr->resrec.RecordType == kDNSRecordTypeVerified)
                            {
                                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                    "mDNSCoreReceiveResponse: Resetting to Probing: " PRI_S, ARDisplayString(m, rr));
                                rr->resrec.RecordType     = kDNSRecordTypeUnique;
                                // We set ProbeCount to one more than the usual value so we know we've already touched this record.
                                // This is because our single probe for "example-name.local" could yield a response with (say) two A records and
                                // three AAAA records in it, and we don't want to call RecordProbeFailure() five times and count that as five conflicts.
                                // This special value is recognised and reset to DefaultProbeCountForTypeUnique in SendQueries().
                                rr->ProbeCount     = DefaultProbeCountForTypeUnique + 1;
                                rr->AnnounceCount  = InitialAnnounceCount;
                                InitializeLastAPTime(m, rr);
                                RecordProbeFailure(m, rr);  // Repeated late conflicts also cause us to back off to the slower probing rate
                            }
                            // If we're probing for this record, we just failed
                            else if (rr->resrec.RecordType == kDNSRecordTypeUnique)
                            {
	                            // At this point in the code, we're probing for uniqueness.
	                            // We've sent at least one probe (rr->ProbeCount < DefaultProbeCountForTypeUnique)
	                            // but we haven't completed probing yet (rr->resrec.RecordType == kDNSRecordTypeUnique).
                                // Before we call deregister, check if this is a packet we registered with the sleep proxy.
                                if (!mDNSCoreRegisteredProxyRecord(m, rr))
                                {
                                    if ((rr->ProbingConflictCount == 0) || (m->MPktNum != rr->LastConflictPktNum))
                                    {
                                        const NetworkInterfaceInfo *const intf = FirstInterfaceForID(m, InterfaceID);
                                        rr->ProbingConflictCount++;
                                        rr->LastConflictPktNum = m->MPktNum;
                                        if (ResponseMCast && (!intf || intf->SupportsUnicastMDNSResponse) &&
                                            (rr->ProbingConflictCount <= kMaxAllowedMCastProbingConflicts))
                                        {
                                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                                "mDNSCoreReceiveResponse: ProbeCount %u; "
                                                "restarting probing after %u-tick pause due to possibly "
                                                "spurious multicast conflict (%d/%u) via interface %d for " PRI_S,
                                                rr->ProbeCount, kProbingConflictPauseDuration, rr->ProbingConflictCount,
                                                kMaxAllowedMCastProbingConflicts, IIDPrintable(InterfaceID),
                                                ARDisplayString(m, rr));

                                            rr->ProbeCount = DefaultProbeCountForTypeUnique;
                                            rr->LastAPTime = m->timenow + kProbingConflictPauseDuration - rr->ThisAPInterval;
                                            SetNextAnnounceProbeTime(m, rr);
                                        }
                                        else
                                        {
                                            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                                "mDNSCoreReceiveResponse: ProbeCount %u; "
                                                "will deregister " PRI_S " due to " PUB_S "cast conflict via interface %d",
                                                rr->ProbeCount, ARDisplayString(m, rr), ResponseMCast ? "multi" : "uni",
                                                IIDPrintable(InterfaceID));

                                            m->mDNSStats.NameConflicts++;
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
                                            // See if this record was also registered with any D2D plugins.
                                            D2D_stop_advertising_record(rr);
#endif
                                            mDNS_Deregister_internal(m, rr, mDNS_Dereg_conflict);
                                        }
                                    }
                                }
                            }
                            // We assumed this record must be unique, but we were wrong. (e.g. There are two mDNSResponders on the
                            // same machine giving different answers for the reverse mapping record, or there are two machines on the
                            // network using the same IP address.) This is simply a misconfiguration, and there's nothing we can do
                            // to fix it -- e.g. it's not our job to be trying to change the machine's IP address. We just discard our
                            // record to avoid continued conflicts (as we do for a conflict on our Unique records) and get on with life.
                            else if (rr->resrec.RecordType == kDNSRecordTypeKnownUnique)
                            {
                                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                                    "mDNSCoreReceiveResponse: Unexpected conflict discarding " PRI_S,
                                    ARDisplayString(m, rr));

                                m->mDNSStats.KnownUniqueNameConflicts++;
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
                                D2D_stop_advertising_record(rr);
#endif
                                mDNS_Deregister_internal(m, rr, mDNS_Dereg_conflict);
                            }
                            else
                            {
                                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR,
                                    "mDNSCoreReceiveResponse: Unexpected record type %X " PRI_S,
                                    rr->resrec.RecordType, ARDisplayString(m, rr));
                            }
                        }
                    }
                    // Else, matching signature, different type or rdata, but not a considered a conflict.
                    // If the packet record has the cache-flush bit set, then we check to see if we
                    // have any record(s) of the same type that we should re-assert to rescue them
                    // (see note about "multi-homing and bridged networks" at the end of this function).
                    else if ((m->rec.r.resrec.rrtype == rr->resrec.rrtype) &&
                        (m->rec.r.resrec.RecordType & kDNSRecordTypePacketUniqueMask) &&
                        ((mDNSu32)(m->timenow - rr->LastMCTime) > (mDNSu32)mDNSPlatformOneSecond/2) &&
                        ResourceRecordIsValidAnswer(rr))
                    {
                        rr->ImmedAnswer = mDNSInterfaceMark;
                        m->NextScheduledResponse = m->timenow;
                    }
                }
            }
        }

        if (!AcceptableResponse)
        {
            AcceptableResponse = IsResponseAcceptable(m, CacheFlushRecords);

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            if (AcceptableResponse) m->rec.r.resrec.rDNSServer = uDNSServer;
#endif
        }

        CacheGroup *cg = mDNSNULL;
        if (AcceptableResponse &&
            (curTSRForName = TSRForNameFromDataRec(&tsrs, m->rec.r.resrec.name)) != mDNSNULL)
        {
            cg = CacheGroupForRecord(m, &m->rec.r.resrec);
            if (cg)
            {
                CacheRecord *ourTSR = mDNSGetTSRForCacheGroup(cg);
                if (ourTSR)
                {
                    eTSRCheckResult tsrResult = CheckTSRForResourceRecord(curTSRForName, &ourTSR->resrec);
                    if (tsrResult == eTSRCheckWin)
                    {
                        AcceptableResponse = mDNSfalse;
                    }
                    else if (tsrResult == eTSRCheckLose)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                            "mDNSCoreReceiveResponse - flushing cache group " PRI_DM_NAME " type " PUB_S " on interface %d due to TSR conflict",
                            DM_NAME_PARAM(m->rec.r.resrec.name), DNSTypeName(m->rec.r.resrec.rrtype), (int)IIDPrintable(InterfaceID));
                        CacheRecord *cr;
                        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
                        {
                            if (cr->resrec.rrtype != kDNSType_OPT)
                            {
                                mDNS_PurgeCacheResourceRecord(m, cr);
                                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                                    "mDNSCoreReceiveResponse - flushed interface %d " PRI_S,
                                    (int)IIDPrintable(cr->resrec.InterfaceID), CRDisplayString(m, cr));
                            }
                        }
                    }
                }
                if (AcceptableResponse)
                {
                    AddOrUpdateTSRForCacheGroup(m, curTSRForName, cg, ourTSR, m->rec.r.resrec.rroriginalttl);
                }
            }
        }

        // 2. See if we want to add this packet resource record to our cache
        // We only try to cache answers if we have a cache to put them in
        // Also, we ignore any apparent attempts at cache poisoning unicast to us that do not answer any outstanding active query
        if (m->rrcache_size && AcceptableResponse)
        {
            const mDNSu32 slot = HashSlotFromNameHash(m->rec.r.resrec.namehash);
            if (!cg)
            {
                cg = CacheGroupForRecord(m, &m->rec.r.resrec);
            }
            CacheRecord *rr = mDNSNULL;
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            // Set to true if no new cache record has been created for the response, either due to existing cached
            // record refresh or the reason that the new response is a subset of the previous one.
            mDNSBool newRecordAdded = mDNSfalse;
        #endif

            // 2a. Check if this packet resource record is already in our cache.
            rr = mDNSCoreReceiveCacheCheck(m, response, LLQType, slot, cg, &cfp, InterfaceID);

            // If packet resource record not in our cache, add it now
            // (unless it is just a deletion of a record we never had, in which case we don't care)
            if (!rr && m->rec.r.resrec.rroriginalttl > 0)
            {
                const mDNSBool AddToCFList = (m->rec.r.resrec.RecordType & kDNSRecordTypePacketUniqueMask) && (LLQType != uDNS_LLQ_Events);
                mDNSs32 delay;

                if (AddToCFList)
                    delay = NonZeroTime(m->timenow + mDNSPlatformOneSecond);
                else
                    delay = CheckForSoonToExpireRecords(m, m->rec.r.resrec.name, m->rec.r.resrec.namehash);

                CreateNewCacheEntryFlags flags = kCreateNewCacheEntryFlagsNone;
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                if (toBeDNSSECValidated)
                {
                    flags |= kCreateNewCacheEntryFlagsDNSSECRRToValidate;
                }
            #endif
            #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                if (subscriber)
                {
                    flags |= kCreateNewCacheEntryFlagsDNSPushSubscribed;
                }
            #endif
                // If unique, assume we may have to delay delivery of this 'add' event.
                // Below, where we walk the CacheFlushRecords list, we either call CacheRecordDeferredAdd()
                // to immediately to generate answer callbacks, or we call ScheduleNextCacheCheckTime()
                // to schedule an mDNS_Execute task at the appropriate time.
                rr = CreateNewCacheEntryEx(m, slot, cg, delay, mDNStrue, srcaddr, flags);
                if (rr)
                {
                    CacheRecordSetResponseFlags(rr, response->h.flags);
                    if (AddToCFList)
                    {
                        *cfp = rr;
                        cfp = &rr->NextInCFList;
                        *cfp = (CacheRecord*)1;
                    }
                    else if (rr->DelayDelivery)
                    {
                        ScheduleNextCacheCheckTime(m, slot, rr->DelayDelivery);
                    }

                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    newRecordAdded = mDNStrue;
                #endif
                }
            }

        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            if (!ResponseIsMDNS) // Only count the answer number for unicast response.
            {
                // Do not increment the counter if rr is not in the cache (either newly added or existing).
                if (rr == mDNSNULL)
                {
                    goto record_set_tracking_exit;
                }
                // Do not increment the counter if rr is a negative record.
                if (rr->resrec.rroriginalttl == 0)
                {
                    goto record_set_tracking_exit;
                }

                // Do not read/write out of bound for the arrays.
                if (numOfCachedRecords == countof(cachedRecords) ||
                    numOfRecordSets == countof(recordSets))
                {
                    goto record_set_tracking_exit;
                }

                // Save the record pointer in a temporary array.
                // Mark the record as "ineligible for cache recycling" because we will save the pointer to a temporary
                // array in mDNSCoreReceiveResponse(). If the saved pointer is recycled while we are still looping
                // through the records in the response, we will crash if we try to deference the recycled records later.
                rr->ineligibleForRecycling = mDNStrue;
                cachedRecords[numOfCachedRecords] = rr;
                numOfCachedRecords++;

                RecordSet *recordSet = mDNSNULL;
                for (mDNSu32 k = 0; k < numOfRecordSets; k++)
                {
                    if (!RecordInTheRRSet(&rr->resrec, &recordSets[k]))
                    {
                        continue;
                    }

                    recordSet = &recordSets[k];
                    break;
                }

                if (recordSet == mDNSNULL)
                {
                    recordSet = &recordSets[numOfRecordSets];
                    numOfRecordSets++;

                    recordSet->name = rr->resrec.name;
                    recordSet->namehash = rr->resrec.namehash;
                    if (rr->resrec.rrtype != kDNSType_RRSIG)
                    {
                        recordSet->rrtype = rr->resrec.rrtype;
                    }
                    else
                    {
                        recordSet->rrtype = resource_record_as_rrsig_get_covered_type(&rr->resrec);
                    }

                    recordSet->sizeOfRecordSet = 0;
                    recordSet->noNewCachedRecordAdded = mDNSfalse;
                }

                recordSet->sizeOfRecordSet++;
                recordSet->noNewCachedRecordAdded = newRecordAdded;

            record_set_tracking_exit:
                (void)0; // No-op for the label above.
            }
        #endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        }
        mDNSCoreResetRecord(m);
    }
    TSRDataRecHeadFreeList(&tsrs);
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // After finishing processing all the records in the response, update the corresponding fields for the records
    // in the cache.
    if (!ResponseIsMDNS) // Only count the answer number for the unicast response.
    {
        if (numOfCachedRecords == countof(cachedRecords) ||
            numOfRecordSets == countof(recordSets))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, "Too many records contained in a single response - "
                "cached records: %u, record set: %u", numOfCachedRecords, numOfRecordSets);
        }

        for (mDNSu32 k = 0; k < numOfCachedRecords; k++)
        {
            CacheRecord *const record = cachedRecords[k];

            // Ensure that the max number of records in a response can be represented by numOfCachedRecords.
            mdns_compile_time_check_local(countof(cachedRecords) < UINT8_MAX);

            const RecordSet *recordSet = mDNSNULL;
            for (mDNSu32 kk = 0; kk < numOfRecordSets; kk++)
            {
                if (!RecordInTheRRSet(&record->resrec, &recordSets[kk]))
                {
                    continue;
                }

                recordSet = &recordSets[kk];
                break;
            }

            if (recordSet == mDNSNULL)
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
                    "Answer set counter not found for the cached record - name: " PRI_DM_NAME ", rrtype: " PRI_S ".",
                    DM_NAME_PARAM(record->resrec.name), DNSTypeName(record->resrec.rrtype));
                continue;
            }

            if (record->resrec.dnssec != NULL)
            {
                dnssec_obj_resource_record_member_set_rrset_size(record->resrec.dnssec, recordSet->sizeOfRecordSet);
                dnssec_obj_resource_record_member_set_no_new_rr_added(record->resrec.dnssec, recordSet->noNewCachedRecordAdded);
            }
        }
    }
#endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

    if (dumpMDNSPacket)
    {
        DumpPacket(mStatus_NoError, mDNSfalse, "N/A", srcaddr, srcport, dstaddr, dstport, response, end, InterfaceID);
    }

bail:
    mDNSCoreResetRecord(m);

    // If we've just received one or more records with their cache flush bits set,
    // then scan that cache slot to see if there are any old stale records we need to flush
    while (CacheFlushRecords != (CacheRecord*)1)
    {
        CacheRecord *r1 = CacheFlushRecords, *r2;
        const mDNSu32 slot = HashSlotFromNameHash(r1->resrec.namehash);
        const CacheGroup *cg = CacheGroupForRecord(m, &r1->resrec);
        mDNSBool purgedRecords = mDNSfalse;
        CacheFlushRecords = CacheFlushRecords->NextInCFList;
        r1->NextInCFList = mDNSNULL;

        // Look for records in the cache with the same signature as this new one with the cache flush
        // bit set, and either (a) if they're fresh, just make sure the whole RRSet has the same TTL
        // (as required by DNS semantics) or (b) if they're old, mark them for deletion in one second.
        // We make these TTL adjustments *only* for records that still have *more* than one second
        // remaining to live. Otherwise, a record that we tagged for deletion half a second ago
        // (and now has half a second remaining) could inadvertently get its life extended, by either
        // (a) if we got an explicit goodbye packet half a second ago, the record would be considered
        // "fresh" and would be incorrectly resurrected back to the same TTL as the rest of the RRSet,
        // or (b) otherwise, the record would not be fully resurrected, but would be reset to expire
        // in one second, thereby inadvertently delaying its actual expiration, instead of hastening it.
        // If this were to happen repeatedly, the record's expiration could be deferred indefinitely.
        // To avoid this, we need to ensure that the cache flushing operation will only act to
        // *decrease* a record's remaining lifetime, never *increase* it.
        // Additional condition to check: a response comes from subscriber does not flush previous records, so skip it.
        for (r2 = cg ? cg->members : mDNSNULL; r2 && !subscriberResponse; r2=r2->next)
        {
            mDNSBool proceed = mDNStrue;
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
            // Push subscribed records will never be flushed by a response (except "remove" push notification).
            proceed = proceed && !r2->DNSPushSubscribed;
        #endif
            proceed = proceed && SameNameCacheRecordsMatchInSourceTypeClass(r1, r2);
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            proceed = proceed && resource_records_is_comparable_for_dnssec(&r1->resrec, &r2->resrec);
        #endif

            if (proceed)
            {
                if (r1->resrec.mortality == Mortality_Mortal && r2->resrec.mortality != Mortality_Mortal)
                {
                    verbosedebugf("mDNSCoreReceiveResponse: R1(%p) is being immortalized by R2(%p)", r1, r2);
                    r1->resrec.mortality = Mortality_Immortal;   //  Immortalize the replacement record
                }

                // If record is recent, just ensure the whole RRSet has the same TTL (as required by DNS semantics)
                // else, if record is old, mark it to be flushed
                if (m->timenow - r2->TimeRcvd < mDNSPlatformOneSecond && RRExpireTime(r2) - m->timenow > mDNSPlatformOneSecond)
                {
                    // If we find mismatched TTLs in an RRSet, correct them.
                    // We only do this for records with a TTL of 2 or higher. It's possible to have a
                    // goodbye announcement with the cache flush bit set (or a case-change on record rdata,
                    // which we treat as a goodbye followed by an addition) and in that case it would be
                    // inappropriate to synchronize all the other records to a TTL of 0 (or 1).

                    // We suppress the message for the specific case of correcting from 240 to 60 for type TXT,
                    // because certain early Bonjour devices are known to have this specific mismatch, and
                    // there's no point filling syslog with messages about something we already know about.
                    // We also don't log this for uDNS responses, since a caching name server is obliged
                    // to give us an aged TTL to correct for how long it has held the record,
                    // so our received TTLs are expected to vary in that case

                    // We also suppress log message in the case of SRV records that are received
                    // with a TTL of 4500 that are already cached with a TTL of 120 seconds, since
                    // this behavior was observed for a number of discoveryd based AppleTV's in iOS 8
                    // GM builds.
                    if (r2->resrec.rroriginalttl != r1->resrec.rroriginalttl && r1->resrec.rroriginalttl > 1)
                    {
                        if (!(r2->resrec.rroriginalttl == 240 && r1->resrec.rroriginalttl == 60 && r2->resrec.rrtype == kDNSType_TXT) &&
                            !(r2->resrec.rroriginalttl == 120 && r1->resrec.rroriginalttl == 4500 && r2->resrec.rrtype == kDNSType_SRV) &&
                            ResponseIsMDNS)
                        {
                            static mDNSs32 lastLogWindowStartTime = 0;
                            static mDNSu32 count = 0;

                            mDNSBool reset = mDNSfalse;
                            if (lastLogWindowStartTime != 0)
                            {
                                const mDNSu32 elapsedTicks = (mDNSu32)(m->timenow - lastLogWindowStartTime);
                                const mDNSu32 limitTicks = MDNS_SECONDS_PER_HOUR * mDNSPlatformOneSecond;
                                if (elapsedTicks >= limitTicks)
                                {
                                    reset = mDNStrue;
                                }
                            }
                            else
                            {
                                reset = mDNStrue;
                            }
                            if (reset)
                            {
                                count = 0;
                                lastLogWindowStartTime = NonZeroTime(m->timenow);
                            }

                            const mDNSu32 ttlCorrectingLogRateLimitCount = 100;
                            const mDNSBool rateLimiting = (count >= ttlCorrectingLogRateLimitCount);
                            count++;

                            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, rateLimiting ? MDNS_LOG_DEBUG : MDNS_LOG_INFO,
                                "Correcting TTL from %4u to %4u from " PRI_IP_ADDR ":%u for records " PRI_S,
                                r2->resrec.rroriginalttl, r1->resrec.rroriginalttl, srcaddr, mDNSVal16(srcport),
                                CRDisplayString(m, r2));
                        }
                        r2->resrec.rroriginalttl = r1->resrec.rroriginalttl;
                    }
                    r2->TimeRcvd = m->timenow;
                    SetNextCacheCheckTimeForRecord(m, r2);
                }
                else if (r2->resrec.InterfaceID) // else, if record is old, mark it to be flushed
                {
                    verbosedebugf("Cache flush new %p age %d expire in %d %s", r1, m->timenow - r1->TimeRcvd, RRExpireTime(r1) - m->timenow, CRDisplayString(m, r1));
                    verbosedebugf("Cache flush old %p age %d expire in %d %s", r2, m->timenow - r2->TimeRcvd, RRExpireTime(r2) - m->timenow, CRDisplayString(m, r2));
                #if MDNSRESPONDER_SUPPORTS(APPLE, AWDL_FAST_CACHE_FLUSH)
                    if (mDNSPlatformInterfaceIsAWDL(r2->resrec.InterfaceID))
                    {
                        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
                            "Fast flushing AWDL cache record -- age: %d ticks, expire: %d ticks, record: " PRI_S,
                            m->timenow - r2->TimeRcvd, RRExpireTime(r2) - m->timenow, CRDisplayString(m, r2));
                        mDNS_PurgeCacheResourceRecord(m, r2);
                    }
                    else
                #endif
                    {
                        // We set stale records to expire in one second.
                        // This gives the owner a chance to rescue it if necessary.
                        // This is important in the case of multi-homing and bridged networks:
                        //   Suppose host X is on Ethernet. X then connects to an AirPort base station, which happens to be
                        //   bridged onto the same Ethernet. When X announces its AirPort IP address with the cache-flush bit
                        //   set, the AirPort packet will be bridged onto the Ethernet, and all other hosts on the Ethernet
                        //   will promptly delete their cached copies of the (still valid) Ethernet IP address record.
                        //   By delaying the deletion by one second, we give X a change to notice that this bridging has
                        //   happened, and re-announce its Ethernet IP address to rescue it from deletion from all our caches.

                        // We set UnansweredQueries to MaxUnansweredQueries to avoid expensive and unnecessary
                        // final expiration queries for this record.

                        // If a record is deleted twice, first with an explicit DE record, then a second time by virtue of the cache
                        // flush bit on the new record replacing it, then we allow the record to be deleted immediately, without the usual
                        // one-second grace period. This improves responsiveness for mDNS_Update(), as used for things like iChat status updates.
                        // <rdar://problem/5636422> Updating TXT records is too slow
                        // We check for "rroriginalttl == 1" because we want to include records tagged by the "packet TTL is zero" check above,
                        // which sets rroriginalttl to 1, but not records tagged by the rdata case-change check, which sets rroriginalttl to 0.
                        if (r2->TimeRcvd == m->timenow && r2->resrec.rroriginalttl == 1 && r2->UnansweredQueries == MaxUnansweredQueries)
                        {
                            LogInfo("Cache flush for DE record %s", CRDisplayString(m, r2));
                            r2->resrec.rroriginalttl = 0;
                        }
                        else if (RRExpireTime(r2) - m->timenow > mDNSPlatformOneSecond)
                        {
                            // We only set a record to expire in one second if it currently has *more* than a second to live
                            // If it's already due to expire in a second or less, we just leave it alone
                            r2->resrec.rroriginalttl = 1;
                            r2->UnansweredQueries = MaxUnansweredQueries;
                            r2->TimeRcvd = m->timenow - 1;
                            // We use (m->timenow - 1) instead of m->timenow, because we use that to identify records
                            // that we marked for deletion via an explicit DE record
                        }
                        SetNextCacheCheckTimeForRecord(m, r2);
                    }
                }
                else
                {
                #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                    // Update the sizeOfRRSet counter of the records that will be flushed to reflect the
                    // response changes, which will be noticed by the callback function.

                    // Only update positive record state, negative record will be handled in
                    // mDNSCoreReceiveNoUnicastAnswers().
                    if (!ResponseIsMDNS && resource_record_is_positive(&r2->resrec))
                    {
                        const RecordSet *recordSet = mDNSNULL;
                        for (mDNSu32 kk = 0; kk < numOfRecordSets; kk++)
                        {
                            if (!RecordInTheRRSet(&r2->resrec, &recordSets[kk]))
                            {
                                continue;
                            }
                            recordSet = &recordSets[kk];
                            break;
                        }

                        if (recordSet == mDNSNULL)
                        {
                            // If we cannot find the records with the same name and type, then it means the records set
                            // has been marked as expired by the authoritative DNS server. Therefore, there will be
                            // 0 answer added (sizeOfRRSet == 0), and we do not need to wait for an add
                            // event(noNewRRAdded == mDNStrue).
                            if (r2->resrec.dnssec != NULL)
                            {
                                dnssec_obj_resource_record_member_set_rrset_size(r2->resrec.dnssec, 0);
                                dnssec_obj_resource_record_member_set_no_new_rr_added(r2->resrec.dnssec, mDNStrue);
                            }
                        }
                        else
                        {
                            if (r2->resrec.dnssec != NULL)
                            {
                                dnssec_obj_resource_record_member_set_rrset_size(r2->resrec.dnssec, recordSet->sizeOfRecordSet);
                                dnssec_obj_resource_record_member_set_no_new_rr_added(r2->resrec.dnssec, recordSet->noNewCachedRecordAdded);
                            }
                        }
                    }
                #endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

                    // Old uDNS records are scheduled to be purged instead of given at most one second to live.
                    mDNS_PurgeCacheResourceRecord(m, r2);
                    purgedRecords = mDNStrue;
                }
            }
        }

        if (r1->DelayDelivery)  // If we were planning to delay delivery of this record, see if we still need to
        {
            if (r1->resrec.InterfaceID)
            {
                r1->DelayDelivery = CheckForSoonToExpireRecords(m, r1->resrec.name, r1->resrec.namehash);
            }
            else
            {
                // If uDNS records from an older RRset were scheduled to be purged, then delay delivery slightly to allow
                // them to be deleted before any ADD events for this record.
                mDNSBool delayDelivery = purgedRecords;
            #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                // We always delay the delivery of DNSSEC-aware response to make sure that we generate the corresponding
                // negative record for the wildcard answer in mDNSCoreReceiveNoUnicastAnswers().
                delayDelivery = delayDelivery || toBeDNSSECValidated;
            #endif
                r1->DelayDelivery = delayDelivery ? NonZeroTime(m->timenow) : 0;
            }
            // If no longer delaying, deliver answer now, else schedule delivery for the appropriate time
            if (!r1->DelayDelivery) CacheRecordDeferredAdd(m, r1);
            else ScheduleNextCacheCheckTime(m, slot, r1->DelayDelivery);
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    if (!ResponseIsMDNS)
    {
        for (mDNSu32 k = 0; k < numOfCachedRecords; k++)
        {
            // Now we have finished the response size counting, the record is now eligible for cache recycling.
            cachedRecords[k]->ineligibleForRecycling = mDNSfalse;
        }
    }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    // If this DNS message is constructed from a subscriber with a push service, then skip the negative record handling
    // below, because push notification never adds negative records.
    if (subscriber)
    {
        goto exit;
    }
#endif

    // See if we need to generate negative cache entries for unanswered unicast questions
    mDNSCoreReceiveNoUnicastAnswers(m, response, end, dstaddr, dstport, InterfaceID,
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        querier, uDNSService,
#endif
        LLQType);
exit:
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_forget(&cache_metadata);
#endif
    return;
}

// ScheduleWakeup causes all proxy records with WakeUp.HMAC matching mDNSEthAddr 'e' to be deregistered, causing
// multiple wakeup magic packets to be sent if appropriate, and all records to be ultimately freed after a few seconds.
// ScheduleWakeup is called on mDNS record conflicts, ARP conflicts, NDP conflicts, or reception of trigger traffic
// that warrants waking the sleeping host.
// ScheduleWakeup must be called with the lock held (ScheduleWakeupForList uses mDNS_Deregister_internal)

mDNSlocal void ScheduleWakeupForList(mDNS *const m, mDNSInterfaceID InterfaceID, mDNSEthAddr *e, AuthRecord *const thelist)
{
    // We need to use the m->CurrentRecord mechanism here when dealing with DuplicateRecords list as
    // mDNS_Deregister_internal deregisters duplicate records immediately as they are not used
    // to send wakeups or goodbyes. See the comment in that function for more details. To keep it
    // simple, we use the same mechanism for both lists.
    if (!e->l[0])
    {
        LogMsg("ScheduleWakeupForList ERROR: Target HMAC is zero");
        return;
    }
    m->CurrentRecord = thelist;
    while (m->CurrentRecord)
    {
        AuthRecord *const rr = m->CurrentRecord;
        if (rr->resrec.InterfaceID == InterfaceID && rr->resrec.RecordType != kDNSRecordTypeDeregistering && mDNSSameEthAddress(&rr->WakeUp.HMAC, e))
        {
            LogInfo("ScheduleWakeupForList: Scheduling wakeup packets for %s", ARDisplayString(m, rr));
            mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
        }
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

mDNSlocal void ScheduleWakeup(mDNS *const m, mDNSInterfaceID InterfaceID, mDNSEthAddr *e)
{
    if (!e->l[0])
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "ScheduleWakeup ERROR: Target HMAC is zero");
        return;
    }
    ScheduleWakeupForList(m, InterfaceID, e, m->DuplicateRecords);
    ScheduleWakeupForList(m, InterfaceID, e, m->ResourceRecords);
}

mDNSlocal void SPSRecordCallback(mDNS *const m, AuthRecord *const ar, mStatus result)
{
    if (result && result != mStatus_MemFree)
    {
        LogRedact(MDNS_LOG_CATEGORY_SPS, MDNS_LOG_DEFAULT, "SPS Callback %d " PRI_S, result, ARDisplayString(m, ar));
    }

    if (result == mStatus_NameConflict)
    {
        mDNS_Lock(m);
        LogRedact(MDNS_LOG_CATEGORY_SPS, MDNS_LOG_DEFAULT, PUB_S " Conflicting mDNS -- waking " PRI_MAC_ADDR " " PRI_S,
            InterfaceNameForID(m, ar->resrec.InterfaceID), &ar->WakeUp.HMAC, ARDisplayString(m, ar));
        if (ar->WakeUp.HMAC.l[0])
        {
            SendWakeup(m, ar->resrec.InterfaceID, &ar->WakeUp.IMAC, &ar->WakeUp.password, mDNSfalse);  // Send one wakeup magic packet
            ScheduleWakeup(m, ar->resrec.InterfaceID, &ar->WakeUp.HMAC);                               // Schedule all other records with the same owner to be woken
        }
        mDNS_Unlock(m);
    }

    if (result == mStatus_NameConflict || result == mStatus_MemFree)
    {
        m->ProxyRecords--;
        mDNSPlatformMemFree(ar);
        mDNS_UpdateAllowSleep(m);
    }
}

mDNSlocal mDNSu8 *GetValueForMACAddr(mDNSu8 *ptr, mDNSu8 *limit, mDNSEthAddr *eth)
{
    int     i;
    mDNSs8  hval   = 0;
    int     colons = 0;
    mDNSu8  val    = 0;

    for (i = 0; ptr < limit && *ptr != ' ' && i < 17; i++, ptr++)
    {
        hval = HexVal(*ptr);
        if (hval != -1)
        {
            val <<= 4;
            val |= hval;
        }
        else if (*ptr == ':')
        {
            if (colons >=5)
            {
                LogMsg("GetValueForMACAddr: Address malformed colons %d val %d", colons, val);
                return mDNSNULL;
            }
            eth->b[colons] = val;
            colons++;
            val = 0;
        }
    }
    if (colons != 5)
    {
        LogMsg("GetValueForMACAddr: Address malformed colons %d", colons);
        return mDNSNULL;
    }
    eth->b[colons] = val;
    return ptr;
}

mDNSlocal mDNSu8 *GetValueForIPv6Addr(mDNSu8 *ptr, mDNSu8 *limit, mDNSv6Addr *v6)
{
    int hval;
    int value;
    int numBytes;
    int digitsProcessed;
    int zeroFillStart;
    int numColons;
    mDNSu8 v6addr[16];

    // RFC 3513: Section 2.2 specifies IPv6 presentation format. The following parsing
    // handles both (1) and (2) and does not handle embedded IPv4 addresses.
    //
    // First forms a address in "v6addr", then expands to fill the zeroes in and returns
    // the result in "v6"

    numColons = numBytes = value = digitsProcessed = zeroFillStart = 0;
    while (ptr < limit && *ptr != ' ')
    {
        hval = HexVal(*ptr);
        if (hval != -1)
        {
            value <<= 4;
            value |= hval;
            digitsProcessed = 1;
        }
        else if (*ptr == ':')
        {
            if (!digitsProcessed)
            {
                // If we have already seen a "::", we should not see one more. Handle the special
                // case of "::"
                if (numColons)
                {
                    // if we never filled any bytes and the next character is space (we have reached the end)
                    // we are done
                    if (!numBytes && (ptr + 1) < limit && *(ptr + 1) == ' ')
                    {
                        mDNSPlatformMemZero(v6->b, 16);
                        return ptr + 1;
                    }
                    LogMsg("GetValueForIPv6Addr: zeroFillStart non-zero %d", zeroFillStart);
                    return mDNSNULL;
                }

                // We processed "::". We need to fill zeroes later. For now, mark the
                // point where we will start filling zeroes from.
                zeroFillStart = numBytes;
                numColons++;
            }
            else if ((ptr + 1) < limit && *(ptr + 1) == ' ')
            {
                // We have a trailing ":" i.e., no more characters after ":"
                LogMsg("GetValueForIPv6Addr: Trailing colon");
                return mDNSNULL;
            }
            else
            {
                // For a fully expanded IPv6 address, we fill the 14th and 15th byte outside of this while
                // loop below as there is no ":" at the end. Hence, the last two bytes that can possibly
                // filled here is 12 and 13.
                if (numBytes > 13) { LogMsg("GetValueForIPv6Addr:1: numBytes is %d", numBytes); return mDNSNULL; }

                v6addr[numBytes++] = (mDNSu8) ((value >> 8) & 0xFF);
                v6addr[numBytes++] = (mDNSu8) (value & 0xFF);
                digitsProcessed = value = 0;

                // Make sure that we did not fill the 13th and 14th byte above
                if (numBytes > 14) { LogMsg("GetValueForIPv6Addr:2: numBytes is %d", numBytes); return mDNSNULL; }
            }
        }
        ptr++;
    }

    // We should be processing the last set of bytes following the last ":" here
    if (!digitsProcessed)
    {
        LogMsg("GetValueForIPv6Addr: no trailing bytes after colon, numBytes is %d", numBytes);
        return mDNSNULL;
    }

    if (numBytes > 14) { LogMsg("GetValueForIPv6Addr:3: numBytes is %d", numBytes); return mDNSNULL; }
    v6addr[numBytes++] = (mDNSu8) ((value >> 8) & 0xFF);
    v6addr[numBytes++] = (mDNSu8) (value & 0xFF);

    if (zeroFillStart)
    {
        int i, j, n;
        for (i = 0; i < zeroFillStart; i++)
            v6->b[i] = v6addr[i];
        for (j = i, n = 0; n < 16 - numBytes; j++, n++)
            v6->b[j] = 0;
        for (; j < 16; i++, j++)
            v6->b[j] = v6addr[i];
    }
    else if (numBytes == 16)
        mDNSPlatformMemCopy(v6->b, v6addr, 16);
    else
    {
        LogMsg("GetValueForIPv6addr: Not enough bytes for IPv6 address, numBytes is %d", numBytes);
        return mDNSNULL;
    }
    return ptr;
}

mDNSlocal mDNSu8 *GetValueForIPv4Addr(mDNSu8 *ptr, mDNSu8 *limit, mDNSv4Addr *v4)
{
    mDNSu32 val;
    int dots = 0;
    val = 0;

    for ( ; ptr < limit && *ptr != ' '; ptr++)
    {
        if (*ptr >= '0' &&  *ptr <= '9')
            val = val * 10 + *ptr - '0';
        else if (*ptr == '.')
        {
            if (val > 255 || dots >= 3)
            {
                LogMsg("GetValueForIPv4Addr: something wrong ptr(%p) %c, limit %p, dots %d", ptr, *ptr, limit, dots);
                return mDNSNULL;
            }
            v4->b[dots++] = (mDNSu8)val;
            val = 0;
        }
        else
        {
            // We have a zero at the end and if we reached that, then we are done.
            if (*ptr == 0 && ptr == limit - 1 && dots == 3)
            {
                v4->b[dots] = (mDNSu8)val;
                return ptr + 1;
            }
            else { LogMsg("GetValueForIPv4Addr: something wrong ptr(%p) %c, limit %p, dots %d", ptr, *ptr, limit, dots); return mDNSNULL; }
        }
    }
    if (dots != 3) { LogMsg("GetValueForIPv4Addr: Address malformed dots %d", dots); return mDNSNULL; }
    v4->b[dots] = (mDNSu8)val;
    return ptr;
}

mDNSlocal mDNSu8 *GetValueForKeepalive(mDNSu8 *ptr, mDNSu8 *limit, mDNSu32 *value)
{
    mDNSu32 val;

    val = 0;
    for ( ; ptr < limit && *ptr != ' '; ptr++)
    {
        if (*ptr < '0' || *ptr > '9')
        {
            // We have a zero at the end and if we reached that, then we are done.
            if (*ptr == 0 && ptr == limit - 1)
            {
                *value = val;
                return ptr + 1;
            }
            else { LogMsg("GetValueForKeepalive: *ptr %d, ptr %p, limit %p, ptr +1 %d", *ptr, ptr, limit, *(ptr + 1)); return mDNSNULL; }
        }
        val = val * 10 + *ptr - '0';
    }
    *value = val;
    return ptr;
}

mDNSexport mDNSBool mDNSValidKeepAliveRecord(AuthRecord *rr)
{
    mDNSAddr    laddr, raddr;
    mDNSEthAddr eth;
    mDNSIPPort  lport, rport;
    mDNSu32     timeout, seq, ack;
    mDNSu16     win;

    if (!mDNS_KeepaliveRecord(&rr->resrec))
    {
        return mDNSfalse;
    }

    timeout = seq = ack = 0;
    win = 0;
    laddr = raddr = zeroAddr;
    lport = rport = zeroIPPort;
    eth = zeroEthAddr;

    mDNS_ExtractKeepaliveInfo(rr, &timeout, &laddr, &raddr, &eth, &seq, &ack, &lport, &rport, &win);

    if (mDNSAddressIsZero(&laddr) || mDNSIPPortIsZero(lport) ||
        mDNSAddressIsZero(&raddr) || mDNSIPPortIsZero(rport) ||
        mDNSEthAddressIsZero(eth))
    {
        return mDNSfalse;
    }

    return mDNStrue;
}


mDNSlocal void mDNS_ExtractKeepaliveInfo(AuthRecord *ar, mDNSu32 *timeout, mDNSAddr *laddr, mDNSAddr *raddr, mDNSEthAddr *eth, mDNSu32 *seq,
                                         mDNSu32 *ack, mDNSIPPort *lport, mDNSIPPort *rport, mDNSu16 *win)
{
    if (ar->resrec.rrtype != kDNSType_NULL)
        return;

    if (mDNS_KeepaliveRecord(&ar->resrec))
    {
        int len = ar->resrec.rdlength;
        mDNSu8 *ptr = &ar->resrec.rdata->u.txt.c[1];
        mDNSu8 *limit = ptr + len - 1; // Exclude the first byte that is the length
        mDNSu32 value = 0;

        while (ptr < limit)
        {
            mDNSu8 param = *ptr;
            ptr += 2;   // Skip the letter and the "="
            if (param == 'h')
            {
                laddr->type = mDNSAddrType_IPv4;
                ptr = GetValueForIPv4Addr(ptr, limit, &laddr->ip.v4);
            }
            else if (param == 'd')
            {
                raddr->type = mDNSAddrType_IPv4;
                ptr = GetValueForIPv4Addr(ptr, limit, &raddr->ip.v4);
            }
            else if (param == 'H')
            {
                laddr->type = mDNSAddrType_IPv6;
                ptr = GetValueForIPv6Addr(ptr, limit, &laddr->ip.v6);
            }
            else if (param == 'D')
            {
                raddr->type = mDNSAddrType_IPv6;
                ptr = GetValueForIPv6Addr(ptr, limit, &raddr->ip.v6);
            }
            else if (param == 'm')
            {
                ptr = GetValueForMACAddr(ptr, limit, eth);
            }
            else
            {
                ptr = GetValueForKeepalive(ptr, limit, &value);
            }
            if (!ptr)
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_ExtractKeepaliveInfo: Cannot parse\n");
                return;
            }

            // Extract everything in network order so that it is easy for sending a keepalive and also
            // for matching incoming TCP packets
            switch (param)
            {
            case 't':
                *timeout = value;
                //if (*timeout < 120) *timeout = 120;
                break;
            case 'h':
            case 'H':
            case 'd':
            case 'D':
            case 'm':
            case 'i':
            case 'c':
                break;
            case 'l':
                lport->NotAnInteger = swap16((mDNSu16)value);
                break;
            case 'r':
                rport->NotAnInteger = swap16((mDNSu16)value);
                break;
            case 's':
                *seq = swap32(value);
                break;
            case 'a':
                *ack = swap32(value);
                break;
            case 'w':
                *win = swap16((mDNSu16)value);
                break;
            default:
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_ExtractKeepaliveInfo: unknown value %c\n", param);
                ptr = limit;
                break;
            }
            ptr++; // skip the space
        }
    }
}

// Matches the proxied auth records to the incoming TCP packet and returns the match and its sequence and ack in "rseq" and "rack" so that
// the clients need not retrieve this information from the auth record again.
mDNSlocal AuthRecord* mDNS_MatchKeepaliveInfo(mDNS *const m, const mDNSAddr* pladdr, const mDNSAddr* praddr, const mDNSIPPort plport,
                                              const mDNSIPPort prport, mDNSu32 *rseq, mDNSu32 *rack)
{
    AuthRecord *ar;
    mDNSAddr laddr, raddr;
    mDNSEthAddr eth;
    mDNSIPPort lport, rport;
    mDNSu32 timeout, seq, ack;
    mDNSu16 win;

    for (ar = m->ResourceRecords; ar; ar=ar->next)
    {
        timeout = seq = ack = 0;
        win = 0;
        laddr = raddr = zeroAddr;
        lport = rport = zeroIPPort;

        if (!ar->WakeUp.HMAC.l[0]) continue;

        mDNS_ExtractKeepaliveInfo(ar, &timeout, &laddr, &raddr, &eth, &seq, &ack, &lport, &rport, &win);

        // Did we parse correctly ?
        if (!timeout || mDNSAddressIsZero(&laddr) || mDNSAddressIsZero(&raddr) || !seq || !ack || mDNSIPPortIsZero(lport) || mDNSIPPortIsZero(rport) || !win)
        {
            debugf("mDNS_MatchKeepaliveInfo: not a valid record %s for keepalive", ARDisplayString(m, ar));
            continue;
        }

        debugf("mDNS_MatchKeepaliveInfo: laddr %#a pladdr %#a, raddr %#a praddr %#a, lport %d plport %d, rport %d prport %d",
               &laddr, pladdr, &raddr, praddr, mDNSVal16(lport), mDNSVal16(plport), mDNSVal16(rport), mDNSVal16(prport));

        // Does it match the incoming TCP packet ?
        if (mDNSSameAddress(&laddr, pladdr) && mDNSSameAddress(&raddr, praddr) && mDNSSameIPPort(lport, plport) && mDNSSameIPPort(rport, prport))
        {
            // returning in network order
            *rseq = seq;
            *rack = ack;
            return ar;
        }
    }
    return mDNSNULL;
}

mDNSlocal void mDNS_SendKeepalives(mDNS *const m)
{
    AuthRecord *ar;

    for (ar = m->ResourceRecords; ar; ar=ar->next)
    {
        mDNSu32 timeout, seq, ack;
        mDNSu16 win;
        mDNSAddr laddr, raddr;
        mDNSEthAddr eth;
        mDNSIPPort lport, rport;

        timeout = seq = ack = 0;
        win = 0;

        laddr = raddr = zeroAddr;
        lport = rport = zeroIPPort;

        if (!ar->WakeUp.HMAC.l[0]) continue;

        mDNS_ExtractKeepaliveInfo(ar, &timeout, &laddr, &raddr, &eth, &seq, &ack, &lport, &rport, &win);

        if (!timeout || mDNSAddressIsZero(&laddr) || mDNSAddressIsZero(&raddr) || !seq || !ack || mDNSIPPortIsZero(lport) || mDNSIPPortIsZero(rport) || !win)
        {
            debugf("mDNS_SendKeepalives: not a valid record %s for keepalive", ARDisplayString(m, ar));
            continue;
        }
        LogMsg("mDNS_SendKeepalives: laddr %#a raddr %#a lport %d rport %d", &laddr, &raddr, mDNSVal16(lport), mDNSVal16(rport));

        // When we receive a proxy update, we set KATimeExpire to zero so that we always send a keepalive
        // immediately (to detect any potential problems). After that we always set it to a non-zero value.
        if (!ar->KATimeExpire || (m->timenow - ar->KATimeExpire >= 0))
        {
            mDNSPlatformSendKeepalive(&laddr, &raddr, &lport, &rport, seq, ack, win);
            ar->KATimeExpire = NonZeroTime(m->timenow + timeout * mDNSPlatformOneSecond);
        }
        if (m->NextScheduledKA - ar->KATimeExpire > 0)
            m->NextScheduledKA = ar->KATimeExpire;
    }
}

mDNSlocal void mDNS_SendKeepaliveACK(mDNS *const m, AuthRecord *ar)
{
    mDNSu32     timeout, seq, ack, seqInc;
    mDNSu16     win;
    mDNSAddr    laddr, raddr;
    mDNSEthAddr eth;
    mDNSIPPort  lport, rport;
    mDNSu8      *ptr;

    if (ar == mDNSNULL)
    {
        LogInfo("mDNS_SendKeepalivesACK: AuthRecord is NULL");
        return;
    }

    timeout = seq = ack = 0;
    win = 0;

    laddr = raddr = zeroAddr;
    lport = rport = zeroIPPort;

    mDNS_ExtractKeepaliveInfo(ar, &timeout, &laddr, &raddr, &eth, &seq, &ack, &lport, &rport, &win);

    if (!timeout || mDNSAddressIsZero(&laddr) || mDNSAddressIsZero(&raddr) || !seq || !ack || mDNSIPPortIsZero(lport) || mDNSIPPortIsZero(rport) || !win)
    {
        LogInfo("mDNS_SendKeepaliveACK: not a valid record %s for keepalive", ARDisplayString(m, ar));
        return;
    }

    // To send a keepalive ACK, we need to add one to the sequence number from the keepalive
    // record, which is the TCP connection's "next" sequence number minus one. Otherwise, the
    // keepalive ACK also ends up being a keepalive probe. Also, seq is in network byte order, so
    // it's converted to host byte order before incrementing it by one.
    ptr = (mDNSu8 *)&seq;
    seqInc = (mDNSu32)((ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]) + 1;
    ptr[0] = (mDNSu8)((seqInc >> 24) & 0xFF);
    ptr[1] = (mDNSu8)((seqInc >> 16) & 0xFF);
    ptr[2] = (mDNSu8)((seqInc >>  8) & 0xFF);
    ptr[3] = (mDNSu8)((seqInc      ) & 0xFF);
    LogMsg("mDNS_SendKeepaliveACK: laddr %#a raddr %#a lport %d rport %d", &laddr, &raddr, mDNSVal16(lport), mDNSVal16(rport));
    mDNSPlatformSendKeepalive(&laddr, &raddr, &lport, &rport, seq, ack, win);
}

mDNSlocal void mDNSCoreReceiveUpdate(mDNS *const m,
                                     const DNSMessage *const msg, const mDNSu8 *end,
                                     const mDNSAddr *srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, mDNSIPPort dstport,
                                     const mDNSInterfaceID InterfaceID)
{
    int i;
    AuthRecord opt;
    mDNSu8 *p = m->omsg.data;
    OwnerOptData owner = zeroOwner;     // Need to zero this, so we'll know if this Update packet was missing its Owner option
    mDNSu32 updatelease = 0;
    const mDNSu8 *ptr;

    LogSPS("Received Update from %#-15a:%-5d to %#-15a:%-5d on 0x%p with "
           "%2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s %d bytes",
           srcaddr, mDNSVal16(srcport), dstaddr, mDNSVal16(dstport), InterfaceID,
           msg->h.numQuestions,   msg->h.numQuestions   == 1 ? ", "   : "s,",
           msg->h.numAnswers,     msg->h.numAnswers     == 1 ? ", "   : "s,",
           msg->h.numAuthorities, msg->h.numAuthorities == 1 ? "y,  " : "ies,",
           msg->h.numAdditionals, msg->h.numAdditionals == 1 ? " "    : "s", end - msg->data);

    if (!InterfaceID || !m->SPSSocket || !mDNSSameIPPort(dstport, m->SPSSocket->port)) return;

    if (mDNS_PacketLoggingEnabled)
        DumpPacket(mStatus_NoError, mDNSfalse, "UDP", srcaddr, srcport, dstaddr, dstport, msg, end, InterfaceID);

    ptr = LocateOptRR(msg, end, DNSOpt_LeaseData_Space + DNSOpt_OwnerData_ID_Space);
    if (ptr)
    {
        ptr = GetLargeResourceRecord(m, msg, ptr, end, 0, kDNSRecordTypePacketAdd, &m->rec);
        if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative && m->rec.r.resrec.rrtype == kDNSType_OPT)
        {
            const rdataOPT *o;
            const rdataOPT *const e = (const rdataOPT *)&m->rec.r.resrec.rdata->u.data[m->rec.r.resrec.rdlength];
            for (o = &m->rec.r.resrec.rdata->u.opt[0]; o < e; o++)
            {
                if      (o->opt == kDNSOpt_Lease) updatelease = o->u.updatelease;
                else if (o->opt == kDNSOpt_Owner && o->u.owner.vers == 0) owner       = o->u.owner;
            }
        }
        mDNSCoreResetRecord(m);
    }

    InitializeDNSMessage(&m->omsg.h, msg->h.id, UpdateRespFlags);

    if (!updatelease || !owner.HMAC.l[0])
    {
        static int msgs = 0;
        if (msgs < 100)
        {
            msgs++;
            LogMsg("Refusing sleep proxy registration from %#a:%d:%s%s", srcaddr, mDNSVal16(srcport),
                   !updatelease ? " No lease" : "", !owner.HMAC.l[0] ? " No owner" : "");
        }
        m->omsg.h.flags.b[1] |= kDNSFlag1_RC_FormErr;
    }
    else if (m->ProxyRecords + msg->h.mDNS_numUpdates > MAX_PROXY_RECORDS)
    {
        static int msgs = 0;
        if (msgs < 100)
        {
            msgs++;
            LogMsg("Refusing sleep proxy registration from %#a:%d: Too many records %d + %d = %d > %d", srcaddr, mDNSVal16(srcport),
                   m->ProxyRecords, msg->h.mDNS_numUpdates, m->ProxyRecords + msg->h.mDNS_numUpdates, MAX_PROXY_RECORDS);
        }
        m->omsg.h.flags.b[1] |= kDNSFlag1_RC_Refused;
    }
    else
    {
        LogSPS("Received Update for H-MAC %.6a I-MAC %.6a Password %.6a seq %d", &owner.HMAC, &owner.IMAC, &owner.password, owner.seq);

        if (updatelease > 24 * 60 * 60)
            updatelease = 24 * 60 * 60;

        if (updatelease > 0x40000000UL / mDNSPlatformOneSecond)
            updatelease = 0x40000000UL / mDNSPlatformOneSecond;

        ptr = LocateAuthorities(msg, end);

        // Clear any stale TCP keepalive records that may exist
        ClearKeepaliveProxyRecords(m, &owner, m->DuplicateRecords, InterfaceID);
        ClearKeepaliveProxyRecords(m, &owner, m->ResourceRecords, InterfaceID);

        for (i = 0; i < msg->h.mDNS_numUpdates && ptr && ptr < end; i++)
        {
            ptr = GetLargeResourceRecord(m, msg, ptr, end, InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
            if (ptr && m->rec.r.resrec.RecordType != kDNSRecordTypePacketNegative)
            {
                mDNSu16 RDLengthMem = GetRDLengthMem(&m->rec.r.resrec);
                AuthRecord *ar = (AuthRecord *) mDNSPlatformMemAllocateClear(sizeof(AuthRecord) - sizeof(RDataBody) + RDLengthMem);
                if (!ar)
                {
                    m->omsg.h.flags.b[1] |= kDNSFlag1_RC_Refused;
                    break;
                }
                else
                {
                    mDNSu8 RecordType = m->rec.r.resrec.RecordType & kDNSRecordTypePacketUniqueMask ? kDNSRecordTypeUnique : kDNSRecordTypeShared;
                    m->rec.r.resrec.rrclass &= ~kDNSClass_UniqueRRSet;
                    // All stale keepalive records have been flushed prior to this loop.
                    if (!mDNS_KeepaliveRecord(&m->rec.r.resrec))
                    {
                        ClearIdenticalProxyRecords(m, &owner, m->DuplicateRecords); // Make sure we don't have any old stale duplicates of this record
                        ClearIdenticalProxyRecords(m, &owner, m->ResourceRecords);
                    }
                    mDNS_SetupResourceRecord(ar, mDNSNULL, InterfaceID, m->rec.r.resrec.rrtype, m->rec.r.resrec.rroriginalttl, RecordType, AuthRecordAny, SPSRecordCallback, ar);
                    AssignDomainName(&ar->namestorage, m->rec.r.resrec.name);
                    ar->resrec.rdlength = GetRDLength(&m->rec.r.resrec, mDNSfalse);
                    ar->resrec.rdata->MaxRDLength = RDLengthMem;
                    mDNSPlatformMemCopy(ar->resrec.rdata->u.data, m->rec.r.resrec.rdata->u.data, RDLengthMem);
                    ar->ForceMCast = mDNStrue;
                    ar->WakeUp     = owner;
                    if (m->rec.r.resrec.rrtype == kDNSType_PTR)
                    {
                        mDNSs32 t = ReverseMapDomainType(m->rec.r.resrec.name);
                        if      (t == mDNSAddrType_IPv4) GetIPv4FromName(&ar->AddressProxy, m->rec.r.resrec.name);
                        else if (t == mDNSAddrType_IPv6) GetIPv6FromName(&ar->AddressProxy, m->rec.r.resrec.name);
                        debugf("mDNSCoreReceiveUpdate: PTR %d %d %#a %s", t, ar->AddressProxy.type, &ar->AddressProxy, ARDisplayString(m, ar));
                        if (ar->AddressProxy.type) SetSPSProxyListChanged(InterfaceID);
                    }
                    ar->TimeRcvd   = m->timenow;
                    ar->TimeExpire = m->timenow + updatelease * mDNSPlatformOneSecond;
                    if (m->NextScheduledSPS - ar->TimeExpire > 0)
                        m->NextScheduledSPS = ar->TimeExpire;
                    ar->KATimeExpire = 0;
                    mDNS_Register_internal(m, ar);

                    m->ProxyRecords++;
                    mDNS_UpdateAllowSleep(m);
                    LogSPS("SPS Registered %4d %X %s", m->ProxyRecords, RecordType, ARDisplayString(m,ar));
                }
            }
            mDNSCoreResetRecord(m);
        }

        if (m->omsg.h.flags.b[1] & kDNSFlag1_RC_Mask)
        {
            LogMsg("Refusing sleep proxy registration from %#a:%d: Out of memory", srcaddr, mDNSVal16(srcport));
            ClearProxyRecords(m, &owner, m->DuplicateRecords);
            ClearProxyRecords(m, &owner, m->ResourceRecords);
        }
        else
        {
            mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
            opt.resrec.rrclass    = NormalMaxDNSMessageData;
            opt.resrec.rdlength   = sizeof(rdataOPT);   // One option in this OPT record
            opt.resrec.rdestimate = sizeof(rdataOPT);
            opt.resrec.rdata->u.opt[0].opt           = kDNSOpt_Lease;
            opt.resrec.rdata->u.opt[0].u.updatelease = updatelease;
            p = PutResourceRecordTTLWithLimit(&m->omsg, p, &m->omsg.h.numAdditionals, &opt.resrec, opt.resrec.rroriginalttl, m->omsg.data + AbsoluteMaxDNSMessageData);
        }
    }

    if (p) mDNSSendDNSMessage(m, &m->omsg, p, InterfaceID, mDNSNULL, m->SPSSocket, srcaddr, srcport, mDNSNULL, mDNSfalse);
    mDNS_SendKeepalives(m);
}

mDNSlocal mDNSu32 mDNSGenerateOwnerOptForInterface(mDNS *const m, const mDNSInterfaceID InterfaceID, DNSMessage *msg)
{
    mDNSu8 *ptr    = msg->data;
    mDNSu8 *end    = mDNSNULL;
    mDNSu32 length = 0;
    AuthRecord opt;
    NetworkInterfaceInfo *intf;

    mDNS_SetupResourceRecord(&opt, mDNSNULL, mDNSInterface_Any, kDNSType_OPT, kStandardTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);
    opt.resrec.rrclass    = NormalMaxDNSMessageData;
    opt.resrec.rdlength   = sizeof(rdataOPT);
    opt.resrec.rdestimate = sizeof(rdataOPT);

    intf = FirstInterfaceForID(m, InterfaceID);
    SetupOwnerOpt(m, intf, &opt.resrec.rdata->u.opt[0]);

    LogSPS("Generated OPT record : %s", ARDisplayString(m, &opt));
    end = PutResourceRecord(msg, ptr, &msg->h.numAdditionals, &opt.resrec);
    if (end != mDNSNULL)
    {
        // Put all the integer values in IETF byte-order (MSB first, LSB second)
        SwapDNSHeaderBytes(msg);
        length = (mDNSu32)(end - msg->data);
    }
    else
        LogSPS("mDNSGenerateOwnerOptForInterface: Failed to generate owner OPT record");

    return length;
}

// Note that this routine is called both for Sleep Proxy Registrations, and for Standard Dynamic
// DNS registrations, but (currently) only has to handle the Sleep Proxy Registration reply case,
// and should ignore Standard Dynamic DNS registration replies, because those are handled elsewhere.
// Really, both should be unified and handled in one place.
mDNSlocal void mDNSCoreReceiveUpdateR(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *end, const mDNSAddr *srcaddr, const mDNSInterfaceID InterfaceID)
{
    if (InterfaceID)
    {
        mDNSu32 pktlease = 0, spsupdates = 0;
        const mDNSBool gotlease = GetPktLease(m, msg, end, &pktlease);
        const mDNSu32 updatelease = gotlease ? pktlease : 60 * 60; // If SPS fails to indicate lease time, assume one hour
        if (gotlease) LogSPS("DNS Update response contains lease option granting %4d seconds, updateid %d, InterfaceID %p", updatelease, mDNSVal16(msg->h.id), InterfaceID);

        if (m->CurrentRecord)
            LogMsg("mDNSCoreReceiveUpdateR ERROR m->CurrentRecord already set %s", ARDisplayString(m, m->CurrentRecord));
        m->CurrentRecord = m->ResourceRecords;
        while (m->CurrentRecord)
        {
            AuthRecord *const rr = m->CurrentRecord;
            if (rr->resrec.InterfaceID == InterfaceID || (!rr->resrec.InterfaceID && (rr->ForceMCast || IsLocalDomain(rr->resrec.name))))
                if (mDNSSameOpaque16(rr->updateid, msg->h.id))
                {
                    // We successfully completed this record's registration on this "InterfaceID". Clear that bit.
                    // Clear the updateid when we are done sending on all interfaces.
                    mDNSu32 scopeid = mDNSPlatformInterfaceIndexfromInterfaceID(m, InterfaceID, mDNStrue);
                    if (scopeid < (sizeof(rr->updateIntID) * mDNSNBBY))
                        bit_clr_opaque64(rr->updateIntID, scopeid);
                    if (mDNSOpaque64IsZero(&rr->updateIntID))
                        rr->updateid = zeroID;
                    rr->expire   = NonZeroTime(m->timenow + updatelease * mDNSPlatformOneSecond);
                    spsupdates++;
                    LogSPS("Sleep Proxy %s record %2d %5d 0x%x 0x%x (%d) %s", rr->WakeUp.HMAC.l[0] ? "transferred" : "registered", spsupdates, updatelease, rr->updateIntID.l[1], rr->updateIntID.l[0], mDNSVal16(rr->updateid), ARDisplayString(m,rr));
                    if (rr->WakeUp.HMAC.l[0])
                    {
                        rr->WakeUp.HMAC = zeroEthAddr;  // Clear HMAC so that mDNS_Deregister_internal doesn't waste packets trying to wake this host
                        rr->RequireGoodbye = mDNSfalse; // and we don't want to send goodbye for it
                        mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
                    }
                }
            // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
            // new records could have been added to the end of the list as a result of that call.
            if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
                m->CurrentRecord = rr->next;
        }
        if (spsupdates) // Only do this dynamic store stuff if this was, in fact, a Sleep Proxy Update response
        {
            char *ifname;
            mDNSAddr spsaddr;
            DNSMessage optMsg;
            int length;
            // Update the dynamic store with the IP Address and MAC address of the sleep proxy
            ifname = InterfaceNameForID(m, InterfaceID);
            mDNSPlatformMemCopy(&spsaddr, srcaddr, sizeof (mDNSAddr));
            mDNSPlatformStoreSPSMACAddr(&spsaddr, ifname);

            // Store the Owner OPT record for this interface.
            // Configd may use the OPT record if it detects a conflict with the BSP when the system wakes up
            InitializeDNSMessage(&optMsg.h, zeroID, ResponseFlags);
            length = mDNSGenerateOwnerOptForInterface(m, InterfaceID, &optMsg);
            if (length != 0)
            {
                length += sizeof(DNSMessageHeader);
                mDNSPlatformStoreOwnerOptRecord(ifname, &optMsg, length);
            }
        }
    }
    // If we were waiting to go to sleep, then this SPS registration or wide-area record deletion
    // may have been the thing we were waiting for, so schedule another check to see if we can sleep now.
    if (m->SleepLimit) m->NextScheduledSPRetry = m->timenow;
}

mDNSlocal void MakeNegativeCacheRecord(mDNS *const m, CacheRecord *const cr, const domainname *const name,
    const mDNSu32 namehash, const mDNSu16 rrtype, const mDNSu16 rrclass, const mDNSu32 ttl,
    const mDNSInterfaceID InterfaceID, const DNSServRef dnsserv, const mDNSOpaque16 responseFlags)
{
    if (cr == &m->rec.r && m->rec.r.resrec.RecordType)
        LogFatalError("MakeNegativeCacheRecord: m->rec appears to be already in use for %s", CRDisplayString(m, &m->rec.r));

    // Create empty resource record
    cr->resrec.RecordType    = kDNSRecordTypePacketNegative;
    cr->resrec.InterfaceID   = InterfaceID;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_forget(&cr->resrec.metadata);
    if (dnsserv)
    {
        cr->resrec.metadata = mdns_cache_metadata_create();
        mdns_cache_metadata_set_dns_service(cr->resrec.metadata, dnsserv);
    }
#else
    cr->resrec.rDNSServer    = dnsserv;
#endif
    cr->resrec.name          = name;    // Will be updated to point to cg->name when we call CreateNewCacheEntry
    cr->resrec.rrtype        = rrtype;
    cr->resrec.rrclass       = rrclass;
    cr->resrec.rroriginalttl = ttl;
    cr->resrec.rdlength      = 0;
    cr->resrec.rdestimate    = 0;
    cr->resrec.namehash      = namehash;
    cr->resrec.rdatahash     = 0;
    cr->resrec.rdata = (RData*)&cr->smallrdatastorage;
    cr->resrec.rdata->MaxRDLength = 0;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    cr->resrec.dnssec = mDNSNULL;
#endif

    cr->NextInKAList         = mDNSNULL;
    cr->TimeRcvd             = m->timenow;
    cr->DelayDelivery        = 0;
    cr->NextRequiredQuery    = m->timenow;
#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
    cr->LastCachedAnswerTime = 0;
#endif
    cr->CRActiveQuestion     = mDNSNULL;
    cr->UnansweredQueries    = 0;
    cr->LastUnansweredTime   = 0;
    cr->NextInCFList         = mDNSNULL;
    cr->soa                  = mDNSNULL;
    CacheRecordSetResponseFlags(cr, responseFlags);
}

mDNSexport void MakeNegativeCacheRecordForQuestion(mDNS *const m, CacheRecord *const cr, const DNSQuestion *const q,
    const mDNSu32 ttl, const mDNSInterfaceID InterfaceID, const mDNSOpaque16 responseFlags)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    const DNSServRef dnsserv = q->dnsservice;
#else
    const DNSServRef dnsserv = q->qDNSServer;
#endif
    MakeNegativeCacheRecord(m, cr, &q->qname, q->qnamehash, q->qtype, q->qclass, ttl, InterfaceID, dnsserv, responseFlags);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSexport void mDNSCoreReceiveForQuerier(mDNS *const m, DNSMessage *const msg, const mDNSu8 *const end,
    const mdns_client_t client, const mdns_dns_service_t dnsservice, const mDNSInterfaceID InterfaceID)
{
    SwapDNSHeaderBytes(msg);
    mDNS_Lock(m);
    mDNSCoreReceiveResponse(m, msg, end, mDNSNULL, zeroIPPort, mDNSNULL, zeroIPPort, client, dnsservice, InterfaceID);
    mDNS_Unlock(m);
}
#endif

mDNSexport void mDNSCoreReceive(mDNS *const m, DNSMessage *const msg, const mDNSu8 *const end,
                                const mDNSAddr *const srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, const mDNSIPPort dstport,
                                const mDNSInterfaceID InterfaceID)
{
    mDNSInterfaceID ifid = InterfaceID;
    const mDNSu8 *const pkt = (mDNSu8 *)msg;
    const mDNSu8 StdQ = kDNSFlag0_QR_Query    | kDNSFlag0_OP_StdQuery;
    const mDNSu8 StdR = kDNSFlag0_QR_Response | kDNSFlag0_OP_StdQuery;
    const mDNSu8 UpdQ = kDNSFlag0_QR_Query    | kDNSFlag0_OP_Update;
    const mDNSu8 UpdR = kDNSFlag0_QR_Response | kDNSFlag0_OP_Update;
    mDNSu8 QR_OP;
    mDNSu8 *ptr = mDNSNULL;
    mDNSBool TLS = (dstaddr == (mDNSAddr *)1);  // For debug logs: dstaddr = 0 means TCP; dstaddr = 1 means TLS
    if (TLS) dstaddr = mDNSNULL;

#ifndef UNICAST_DISABLED
    if (mDNSSameAddress(srcaddr, &m->Router))
    {
#ifdef _LEGACY_NAT_TRAVERSAL_
        if (mDNSSameIPPort(srcport, SSDPPort) || (m->SSDPSocket && mDNSSameIPPort(dstport, m->SSDPSocket->port)))
        {
            mDNS_Lock(m);
            LNT_ConfigureRouterInfo(m, InterfaceID, (mDNSu8 *)msg, (mDNSu16)(end - pkt));
            mDNS_Unlock(m);
            return;
        }
#endif
        if (mDNSSameIPPort(srcport, NATPMPPort))
        {
            mDNS_Lock(m);
            uDNS_ReceiveNATPacket(m, InterfaceID, (mDNSu8 *)msg, (mDNSu16)(end - pkt));
            mDNS_Unlock(m);
            return;
        }
    }
#ifdef _LEGACY_NAT_TRAVERSAL_
    else if (m->SSDPSocket && mDNSSameIPPort(dstport, m->SSDPSocket->port)) { debugf("Ignoring SSDP response from %#a:%d", srcaddr, mDNSVal16(srcport)); return; }
#endif

#endif
    if ((unsigned)(end - pkt) < sizeof(DNSMessageHeader))
    {
        LogMsg("DNS Message from %#a:%d to %#a:%d length %d too short", srcaddr, mDNSVal16(srcport), dstaddr, mDNSVal16(dstport), (int)(end - pkt));
        return;
    }
    QR_OP = (mDNSu8)(msg->h.flags.b[0] & kDNSFlag0_QROP_Mask);
    // Read the integer parts which are in IETF byte-order (MSB first, LSB second)
    ptr = (mDNSu8 *)&msg->h.numQuestions;
    msg->h.numQuestions   = (mDNSu16)((mDNSu16)ptr[0] << 8 | ptr[1]);
    msg->h.numAnswers     = (mDNSu16)((mDNSu16)ptr[2] << 8 | ptr[3]);
    msg->h.numAuthorities = (mDNSu16)((mDNSu16)ptr[4] << 8 | ptr[5]);
    msg->h.numAdditionals = (mDNSu16)((mDNSu16)ptr[6] << 8 | ptr[7]);

    if (!m) { LogMsg("mDNSCoreReceive ERROR m is NULL"); return; }

    // We use zero addresses and all-ones addresses at various places in the code to indicate special values like "no address"
    // If we accept and try to process a packet with zero or all-ones source address, that could really mess things up
    if (!mDNSAddressIsValid(srcaddr)) { debugf("mDNSCoreReceive ignoring packet from %#a", srcaddr); return; }

    mDNS_Lock(m);
    m->PktNum++;
    if (mDNSOpaque16IsZero(msg->h.id))
    {
        m->MPktNum++;
    }

#ifndef UNICAST_DISABLED
    if (!dstaddr || (!mDNSAddressIsAllDNSLinkGroup(dstaddr) && (QR_OP == StdR || QR_OP == UpdR)))
        if (!mDNSOpaque16IsZero(msg->h.id)) // uDNS_ReceiveMsg only needs to get real uDNS responses, not "QU" mDNS responses
        {
            ifid = mDNSInterface_Any;
            if (mDNS_PacketLoggingEnabled)
                DumpPacket(mStatus_NoError, mDNSfalse, TLS ? "TLS" : !dstaddr ? "TCP" : "UDP", srcaddr, srcport, dstaddr, dstport, msg, end, InterfaceID);
            uDNS_ReceiveMsg(m, msg, end, srcaddr, srcport);
            // Note: mDNSCore also needs to get access to received unicast responses
        }
#endif
    if      (QR_OP == StdQ) mDNSCoreReceiveQuery   (m, msg, end, srcaddr, srcport, dstaddr, dstport, ifid);
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    else if (QR_OP == StdR) mDNSCoreReceiveResponse(m, msg, end, srcaddr, srcport, dstaddr, dstport, mDNSNULL, mDNSNULL, ifid);
#else
    else if (QR_OP == StdR) mDNSCoreReceiveResponse(m, msg, end, srcaddr, srcport, dstaddr, dstport, ifid);
#endif
    else if (QR_OP == UpdQ) mDNSCoreReceiveUpdate  (m, msg, end, srcaddr, srcport, dstaddr, dstport, InterfaceID);
    else if (QR_OP == UpdR) mDNSCoreReceiveUpdateR (m, msg, end, srcaddr,                            InterfaceID);
    else
    {
        if (mDNS_LoggingEnabled)
        {
            static int msgCount = 0;
            if (msgCount < 1000) {
                int i = 0;
                msgCount++;
                LogInfo("Unknown DNS packet type %02X%02X from %#-15a:%-5d to %#-15a:%-5d length %d on %p (ignored)",
                        msg->h.flags.b[0], msg->h.flags.b[1], srcaddr, mDNSVal16(srcport), dstaddr, mDNSVal16(dstport), (int)(end - pkt), InterfaceID);
                while (i < (int)(end - pkt))
                {
                    char buffer[128];
                    char *p = buffer + mDNS_snprintf(buffer, sizeof(buffer), "%04X", i);
                    do if (i < (int)(end - pkt)) p += mDNS_snprintf(p, sizeof(buffer), " %02X", pkt[i]);while (++i & 15);
                    LogInfo("%s", buffer);
                }
            }
        }
    }
    // Packet reception often causes a change to the task list:
    // 1. Inbound queries can cause us to need to send responses
    // 2. Conflicing response packets received from other hosts can cause us to need to send defensive responses
    // 3. Other hosts announcing deletion of shared records can cause us to need to re-assert those records
    // 4. Response packets that answer questions may cause our client to issue new questions
    mDNS_Unlock(m);
}

// ***************************************************************************
// MARK: - Searcher Functions

// Note: We explicitly disallow making a public query be a duplicate of a private one. This is to avoid the
// circular deadlock where a client does a query for something like "dns-sd -Q _dns-query-tls._tcp.company.com SRV"
// and we have a key for company.com, so we try to locate the private query server for company.com, which necessarily entails
// doing a standard DNS query for the _dns-query-tls._tcp SRV record for company.com. If we make the latter (public) query
// a duplicate of the former (private) query, then it will block forever waiting for an answer that will never come.
//
// We keep SuppressUnusable questions separate so that we can return a quick response to them and not get blocked behind
// the queries that are not marked SuppressUnusable. But if the query is not suppressed, they are treated the same as
// non-SuppressUnusable questions. This should be fine as the goal of SuppressUnusable is to return quickly only if it
// is suppressed. If it is not suppressed, we do try all the DNS servers for valid answers like any other question.
// The main reason for this design is that cache entries point to a *single* question and that question is responsible
// for keeping the cache fresh as long as it is active. Having multiple active question for a single cache entry
// breaks this design principle.
//

// If IsLLQ(Q) is true, it means the question is both:
// (a) long-lived and
// (b) being performed by a unicast DNS long-lived query (either full LLQ, or polling)
// for multicast questions, we don't want to treat LongLived as anything special
#define IsLLQ(Q)                 ((Q)->LongLived && !mDNSOpaque16IsZero((Q)->TargetQID))
#define AWDLIsIncluded(Q)        (((Q)->flags & kDNSServiceFlagsIncludeAWDL) != 0)
#define SameQuestionKind(Q1, Q2) (mDNSOpaque16IsZero((Q1)->TargetQID) == mDNSOpaque16IsZero((Q2)->TargetQID))

mDNSlocal DNSQuestion *FindDuplicateQuestion(const mDNS *const m, const DNSQuestion *const question)
{
    DNSQuestion *q;
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
    const mDNSBool questionSendsUnicastMDNSQueries = QuestionSendsMDNSQueriesViaUnicast(question);
#endif
    // Note: A question can only be marked as a duplicate of one that occurs *earlier* in the list.
    // This prevents circular references, where two questions are each marked as a duplicate of the other.
    // Accordingly, we break out of the loop when we get to 'question', because there's no point searching
    // further in the list.
    for (q = m->Questions; q && (q != question); q = q->next)
    {
        if (!SameQuestionKind(q, question))                             continue;
        if (q->qnamehash          != question->qnamehash)               continue;
        if (q->InterfaceID        != question->InterfaceID)             continue;
        if (q->qtype              != question->qtype)                   continue;
        if (q->qclass             != question->qclass)                  continue;
        if (IsLLQ(q)              != IsLLQ(question))                   continue;
        if (q->AuthInfo && !question->AuthInfo)                         continue;
        if (!q->Suppressed        != !question->Suppressed)             continue;
        if (q->BrowseThreshold    != question->BrowseThreshold)         continue;
        if (AWDLIsIncluded(q)     != AWDLIsIncluded(question))          continue;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        if (q->dnsservice         != question->dnsservice)              continue;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        // DNSSEC-disabled question cannot be duped to DNSSEC-enabled question, vice versa.
        if (!q->enableDNSSEC      != !question->enableDNSSEC)                   continue;
        // If enables DNSSEC, the question being duped to must be a primary DNSSEC requestor.
        if (q->enableDNSSEC && !dns_question_is_primary_dnssec_requestor(q))    continue;
#endif
        if (!SameDomainName(&q->qname, &question->qname))               continue;
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_DISCOVERY)
        if (questionSendsUnicastMDNSQueries)
        {
            // If question sends mDNS queries via unicast, then q must also send mDNS queries via
            // unicast and to the same mDNS resolver address.
            const mDNSAddr *const addr1 = &question->UnicastMDNSResolver;
            const mDNSAddr *const addr2 = &q->UnicastMDNSResolver;
            if (!QuestionSendsMDNSQueriesViaUnicast(q)) continue;
            if (!mDNSSameAddress(addr1, addr2))         continue;
        }
        else
        {
            // Otherwise, q must also not send mDNS queries via unicast.
            if (QuestionSendsMDNSQueriesViaUnicast(q))  continue;
        }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH)
        // Only the DNS push enabled question can be dupped to a DNS push enabled question.
        if (dns_question_uses_dns_push(q) != dns_question_uses_dns_push(question))
        {
            continue;
        }
#endif

        return(q);
    }
    return(mDNSNULL);
}

// This is called after a question is deleted, in case other identical questions were being suppressed as duplicates
mDNSlocal void UpdateQuestionDuplicates(mDNS *const m, DNSQuestion *const question)
{
    DNSQuestion *q;
    DNSQuestion *first = mDNSNULL;

    // This is referring to some other question as duplicate. No other question can refer to this
    // question as a duplicate.
    if (question->DuplicateOf)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "[R%d->DupQ%d->Q%d] UpdateQuestionDuplicates: question %p " PRI_DM_NAME " (" PUB_S
            ") duplicate of %p " PRI_DM_NAME " (" PUB_S ")",
            question->request_id, mDNSVal16(question->TargetQID), mDNSVal16(question->DuplicateOf->TargetQID),
            question, DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype), question->DuplicateOf,
            DM_NAME_PARAM(&question->DuplicateOf->qname), DNSTypeName(question->DuplicateOf->qtype));
        return;
    }

    for (q = m->Questions; q; q=q->next)        // Scan our list of questions
        if (q->DuplicateOf == question)         // To see if any questions were referencing this as their duplicate
        {
            q->DuplicateOf = first;
            if (!first)
            {
                first = q;
                // If q used to be a duplicate, but now is not,
                // then inherit the state from the question that's going away
                q->LastQTime         = question->LastQTime;
                q->ThisQInterval     = question->ThisQInterval;
                q->ExpectUnicastResp = question->ExpectUnicastResp;
                q->LastAnswerPktNum  = question->LastAnswerPktNum;
                q->RecentAnswerPkts  = question->RecentAnswerPkts;
                q->RequestUnicast    = question->RequestUnicast;
                q->LastQTxTime       = question->LastQTxTime;
                q->CNAMEReferrals    = question->CNAMEReferrals;
                q->nta               = question->nta;
                q->servAddr          = question->servAddr;
                q->servPort          = question->servPort;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                mdns_replace(&q->dnsservice, question->dnsservice);
                mdns_forget(&question->dnsservice);
                mdns_client_forget(&q->client);
                mdns_replace(&q->client, question->client);
                mdns_forget(&question->client);
#else
                q->qDNSServer        = question->qDNSServer;
                q->validDNSServers   = question->validDNSServers;
                q->unansweredQueries = question->unansweredQueries;
                q->noServerResponse  = question->noServerResponse;
                q->triedAllServersOnce = question->triedAllServersOnce;
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
                // Duplicate questions aren't eligible to have Discovery Proxy subscribers, so a simple
                // handoff from the former lead question to the new lead is sufficient.
                q->DPSubscribers = question->DPSubscribers;
                question->DPSubscribers = mDNSNULL;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
                // If this question is a primary DNSSEC question that does the validation work, transfer the work to
                // its duplicate question, by making the duplicate question the primary one.
                if (dns_question_is_primary_dnssec_requestor(question))
                {
                    LogRedact(MDNS_LOG_CATEGORY_DNSSEC, MDNS_LOG_INFO, "[Q%u->Q%u] Non-primary DNSSEC question becomes primary due to primary question cancelation.",
                        mDNSVal16(q->TargetQID), mDNSVal16(question->TargetQID));

                    dns_question_update_primary_dnssec_requestor(q, question);
                }
#endif

                q->TargetQID         = question->TargetQID;
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                q->LocalSocket       = question->LocalSocket;
                // No need to close old q->LocalSocket first -- duplicate questions can't have their own sockets
#endif

                q->state             = question->state;
                //  q->tcp               = question->tcp;
                q->ReqLease          = question->ReqLease;
                q->expire            = question->expire;
                q->ntries            = question->ntries;
                q->id                = question->id;

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH)
                DNSPushUpdateQuestionDuplicate(question, q);
#endif

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                question->LocalSocket = mDNSNULL;
#endif
                question->nta        = mDNSNULL;    // If we've got a GetZoneData in progress, transfer it to the newly active question
                //  question->tcp        = mDNSNULL;

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                if (q->LocalSocket)
                    debugf("UpdateQuestionDuplicates transferred LocalSocket pointer for %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
                // If the question being stopped collects mDNS metric, the new primary should take it.
                if (DNSQuestionCollectsMDNSMetric(question))
                {
                    first->metrics = question->metrics;
                }
#endif
                if (q->nta)
                {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "UpdateQuestionDuplicates transferred nta pointer for " PRI_DM_NAME " (" PUB_S ")",
                        DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype));
                    q->nta->ZoneDataContext = q;
                }

                // Need to work out how to safely transfer this state too -- appropriate context pointers need to be updated or the code will crash
                if (question->tcp) LogInfo("UpdateQuestionDuplicates did not transfer tcp pointer");

                if (question->state == LLQ_Established)
                {
                    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "UpdateQuestionDuplicates transferred LLQ state for " PRI_DM_NAME " (" PUB_S ")",
                        DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype));
                    question->state = LLQ_Invalid;    // Must zero question->state, or mDNS_StopQuery_internal will clean up and cancel our LLQ from the server
                }

                SetNextQueryTime(m,q);
            }
        }
}

mDNSexport McastResolver *mDNS_AddMcastResolver(mDNS *const m, const domainname *d, const mDNSInterfaceID interface, mDNSu32 timeout)
{
    McastResolver **p = &m->McastResolvers;
    McastResolver *tmp = mDNSNULL;

    if (!d) d = (const domainname *)"";

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "mDNS_AddMcastResolver: Adding " PUB_DM_NAME ", InterfaceID %p, timeout %u",
        DM_NAME_PARAM(d), interface, timeout);

    mDNS_CheckLock(m);

    while (*p)  // Check if we already have this {interface, domain} tuple registered
    {
        if ((*p)->interface == interface && SameDomainName(&(*p)->domain, d))
        {
            if (!((*p)->flags & McastResolver_FlagDelete)) LogMsg("Note: Mcast Resolver domain %##s (%p) registered more than once", d->c, interface);
            (*p)->flags &= ~McastResolver_FlagDelete;
            tmp = *p;
            *p = tmp->next;
            tmp->next = mDNSNULL;
        }
        else
            p=&(*p)->next;
    }

    if (tmp) *p = tmp; // move to end of list, to ensure ordering from platform layer
    else
    {
        // allocate, add to list
        *p = (McastResolver *) mDNSPlatformMemAllocateClear(sizeof(**p));
        if (!*p) LogMsg("mDNS_AddMcastResolver: ERROR!! - malloc");
        else
        {
            (*p)->interface = interface;
            (*p)->flags     = McastResolver_FlagNew;
            (*p)->timeout   = timeout;
            AssignDomainName(&(*p)->domain, d);
            (*p)->next = mDNSNULL;
        }
    }
    return(*p);
}

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSinline mDNSs32 PenaltyTimeForServer(mDNS *m, DNSServer *server)
{
    mDNSs32 ptime = 0;
    if (server->penaltyTime != 0)
    {
        ptime = server->penaltyTime - m->timenow;
        if (ptime < 0)
        {
            // This should always be a positive value between 0 and DNSSERVER_PENALTY_TIME
            // If it does not get reset in ResetDNSServerPenalties for some reason, we do it
            // here
            LogMsg("PenaltyTimeForServer: PenaltyTime negative %d, (server penaltyTime %d, timenow %d) resetting the penalty",
                   ptime, server->penaltyTime, m->timenow);
            server->penaltyTime = 0;
            ptime = 0;
        }
    }
    return ptime;
}
#endif

//Checks to see whether the newname is a better match for the name, given the best one we have
//seen so far (given in bestcount).
//Returns -1 if the newname is not a better match
//Returns 0 if the newname is the same as the old match
//Returns 1 if the newname is a better match
mDNSlocal int BetterMatchForName(const domainname *name, int namecount, const domainname *newname, int newcount,
                                 int bestcount)
{
    // If the name contains fewer labels than the new server's domain or the new name
    // contains fewer labels than the current best, then it can't possibly be a better match
    if (namecount < newcount || newcount < bestcount) return -1;

    // If there is no match, return -1 and the caller will skip this newname for
    // selection
    //
    // If we find a match and the number of labels is the same as bestcount, then
    // we return 0 so that the caller can do additional logic to pick one of
    // the best based on some other factors e.g., penaltyTime
    //
    // If we find a match and the number of labels is more than bestcount, then we
    // return 1 so that the caller can pick this over the old one.
    //
    // Note: newcount can either be equal or greater than bestcount beause of the
    // check above.

    if (SameDomainName(SkipLeadingLabels(name, namecount - newcount), newname))
        return bestcount == newcount ? 0 : 1;
    else
        return -1;
}

// Normally, we have McastResolvers for .local, in-addr.arpa and ip6.arpa. But there
// can be queries that can forced to multicast (ForceMCast) even though they don't end in these
// names. In that case, we give a default timeout of 5 seconds
mDNSlocal mDNSu32 GetTimeoutForMcastQuestion(mDNS *m, DNSQuestion *question)
{
    McastResolver *curmatch = mDNSNULL;
    int bestmatchlen = -1, namecount = CountLabels(&question->qname);
    McastResolver *curr;
    int bettermatch, currcount;
    for (curr = m->McastResolvers; curr; curr = curr->next)
    {
        currcount = CountLabels(&curr->domain);
        bettermatch = BetterMatchForName(&question->qname, namecount, &curr->domain, currcount, bestmatchlen);
        // Take the first best match. If there are multiple equally good matches (bettermatch = 0), we take
        // the timeout value from the first one
        if (bettermatch == 1)
        {
            curmatch = curr;
            bestmatchlen = currcount;
        }
    }
    LogInfo("GetTimeoutForMcastQuestion: question %##s curmatch %p, Timeout %d", question->qname.c, curmatch,
            curmatch ? curmatch->timeout : DEFAULT_MCAST_TIMEOUT);
    return ( curmatch ? curmatch->timeout : DEFAULT_MCAST_TIMEOUT);
}

// Returns true if it is a Domain Enumeration Query
mDNSexport mDNSBool DomainEnumQuery(const domainname *qname)
{
    const mDNSu8 *mDNS_DEQLabels[] = { (const mDNSu8 *)"\001b", (const mDNSu8 *)"\002db", (const mDNSu8 *)"\002lb",
                                       (const mDNSu8 *)"\001r", (const mDNSu8 *)"\002dr", (const mDNSu8 *)mDNSNULL, };
    const domainname *d = qname;
    const mDNSu8 *label;
    int i = 0;

    // We need at least 3 labels (DEQ prefix) + one more label to make a meaningful DE query
    if (CountLabels(qname) < 4) { debugf("DomainEnumQuery: question %##s, not enough labels", qname->c); return mDNSfalse; }

    label = (const mDNSu8 *)d;
    while (mDNS_DEQLabels[i] != (const mDNSu8 *)mDNSNULL)
    {
        if (SameDomainLabel(mDNS_DEQLabels[i], label)) {debugf("DomainEnumQuery: DEQ %##s, label1 match", qname->c); break;}
        i++;
    }
    if (mDNS_DEQLabels[i] == (const mDNSu8 *)mDNSNULL)
    {
        debugf("DomainEnumQuery: Not a DEQ %##s, label1 mismatch", qname->c);
        return mDNSfalse;
    }
    debugf("DomainEnumQuery: DEQ %##s, label1 match", qname->c);

    // CountLabels already verified the number of labels
    d = (const domainname *)(d->c + 1 + d->c[0]);   // Second Label
    label = (const mDNSu8 *)d;
    if (!SameDomainLabel(label, (const mDNSu8 *)"\007_dns-sd"))
    {
        debugf("DomainEnumQuery: Not a DEQ %##s, label2 mismatch", qname->c);
        return(mDNSfalse);
    }
    debugf("DomainEnumQuery: DEQ %##s, label2 match", qname->c);

    d = (const domainname *)(d->c + 1 + d->c[0]);   // Third Label
    label = (const mDNSu8 *)d;
    if (!SameDomainLabel(label, (const mDNSu8 *)"\004_udp"))
    {
        debugf("DomainEnumQuery: Not a DEQ %##s, label3 mismatch", qname->c);
        return(mDNSfalse);
    }
    debugf("DomainEnumQuery: DEQ %##s, label3 match", qname->c);

    debugf("DomainEnumQuery: Question %##s is a Domain Enumeration query", qname->c);

    return mDNStrue;
}

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
// Note: InterfaceID is the InterfaceID of the question
mDNSlocal mDNSBool DNSServerMatch(DNSServer *d, mDNSInterfaceID InterfaceID, mDNSs32 ServiceID)
{
    // 1) Unscoped questions (NULL InterfaceID) should consider *only* unscoped DNSServers ( DNSServer
    // with scopeType set to kScopeNone)
    //
    // 2) Scoped questions (non-NULL InterfaceID) should consider *only* scoped DNSServers (DNSServer
    // with scopeType set to kScopeInterfaceID) and their InterfaceIDs should match.
    //
    // 3) Scoped questions (non-zero ServiceID) should consider *only* scoped DNSServers (DNSServer
    // with scopeType set to kScopeServiceID) and their ServiceIDs should match.
    //
    // The first condition in the "if" statement checks to see if both the question and the DNSServer are
    // unscoped. The question is unscoped only if InterfaceID is zero and ServiceID is -1.
    //
    // If the first condition fails, following are the possible cases (the notes below are using
    // InterfaceID for discussion and the same holds good for ServiceID):
    //
    // - DNSServer is not scoped, InterfaceID is not NULL - we should skip the current DNSServer entry
    //   as scoped questions should not pick non-scoped DNSServer entry (Refer to (2) above).
    //
    // - DNSServer is scoped, InterfaceID is NULL - we should skip the current DNSServer entry as
    //   unscoped question should not match scoped DNSServer (Refer to (1) above). The InterfaceID check
    //   would fail in this case.
    //
    // - DNSServer is scoped and InterfaceID is not NULL - the InterfaceID of the question and the DNSServer
    //   should match (Refer to (2) above).

    if (((d->scopeType == kScopeNone) && (!InterfaceID && ServiceID == -1))  ||
        ((d->scopeType == kScopeInterfaceID) && d->interface == InterfaceID) ||
        ((d->scopeType == kScopeServiceID) && d->serviceID == ServiceID))
    {
        return mDNStrue;
    }
    return mDNSfalse;
}

// Sets all the Valid DNS servers for a question
mDNSexport mDNSu32 SetValidDNSServers(mDNS *m, DNSQuestion *question)
{
    int bestmatchlen = -1, namecount = CountLabels(&question->qname);
    DNSServer *curr;
    int bettermatch, currcount;
    int index = 0;
    mDNSu32 timeout = 0;
    mDNSBool DEQuery;

    question->validDNSServers = zeroOpaque128;
    DEQuery = DomainEnumQuery(&question->qname);
    for (curr = m->DNSServers; curr; curr = curr->next)
    {
        debugf("SetValidDNSServers: Parsing DNS server Address %#a (Domain %##s), Scope: %d", &curr->addr, curr->domain.c, curr->scopeType);
        // skip servers that will soon be deleted
        if (curr->flags & DNSServerFlag_Delete)
        {
            debugf("SetValidDNSServers: Delete set for index %d, DNS server %#a (Domain %##s), scoped %d", index, &curr->addr, curr->domain.c, curr->scopeType);
            continue;
        }

        // This happens normally when you unplug the interface where we reset the interfaceID to mDNSInterface_Any for all
        // the DNS servers whose scope match the interfaceID. Few seconds later, we also receive the updated DNS configuration.
        // But any questions that has mDNSInterface_Any scope that are started/restarted before we receive the update
        // (e.g., CheckSuppressUnusableQuestions is called when interfaces are deregistered with the core) should not
        // match the scoped entries by mistake.
        //
        // Note: DNS configuration change will help pick the new dns servers but currently it does not affect the timeout

        // Skip DNSServers that are InterfaceID Scoped but have no valid interfaceid set OR DNSServers that are ServiceID Scoped but have no valid serviceid set
        if (((curr->scopeType == kScopeInterfaceID) && (curr->interface == mDNSInterface_Any)) ||
            ((curr->scopeType == kScopeServiceID) && (curr->serviceID <= 0)))
        {
            LogInfo("SetValidDNSServers: ScopeType[%d] Skipping DNS server %#a (Domain %##s) Interface:[%p] Serviceid:[%d]",
                (int)curr->scopeType, &curr->addr, curr->domain.c, curr->interface, curr->serviceID);
            continue;
        }

        currcount = CountLabels(&curr->domain);
        if ((!DEQuery || !curr->isCell) && DNSServerMatch(curr, question->InterfaceID, question->ServiceID))
        {
            bettermatch = BetterMatchForName(&question->qname, namecount, &curr->domain, currcount, bestmatchlen);

            // If we found a better match (bettermatch == 1) then clear all the bits
            // corresponding to the old DNSServers that we have may set before and start fresh.
            // If we find an equal match, then include that DNSServer also by setting the corresponding
            // bit
            if ((bettermatch == 1) || (bettermatch == 0))
            {
                bestmatchlen = currcount;
                if (bettermatch)
                {
                    debugf("SetValidDNSServers: Resetting all the bits");
                    question->validDNSServers = zeroOpaque128;
                    timeout = 0;
                }
                debugf("SetValidDNSServers: question %##s Setting the bit for DNS server Address %#a (Domain %##s), Scoped:%d index %d,"
                       " Timeout %d, interface %p", question->qname.c, &curr->addr, curr->domain.c, curr->scopeType, index, curr->timeout,
                       curr->interface);
                timeout += curr->timeout;
                if (DEQuery)
                    debugf("DomainEnumQuery: Question %##s, DNSServer %#a, cell %d", question->qname.c, &curr->addr, curr->isCell);
                bit_set_opaque128(question->validDNSServers, index);
            }
        }
        index++;
    }
    question->noServerResponse = 0;

    debugf("SetValidDNSServers: ValidDNSServer bits 0x%08x%08x%08x%08x for question %p %##s (%s)",
           question->validDNSServers.l[3], question->validDNSServers.l[2], question->validDNSServers.l[1], question->validDNSServers.l[0], question, question->qname.c, DNSTypeName(question->qtype));
    // If there are no matching resolvers, then use the default timeout value.
    return (timeout ? timeout : DEFAULT_UDNS_TIMEOUT);
}

// Get the Best server that matches a name. If you find penalized servers, look for the one
// that will come out of the penalty box soon
mDNSlocal DNSServer *GetBestServer(mDNS *m, const domainname *name, mDNSInterfaceID InterfaceID, mDNSs32 ServiceID, mDNSOpaque128 validBits,
    int *selected, mDNSBool nameMatch)
{
    DNSServer *curmatch = mDNSNULL;
    int bestmatchlen = -1, namecount = name ? CountLabels(name) : 0;
    DNSServer *curr;
    mDNSs32 bestPenaltyTime, currPenaltyTime;
    int bettermatch, currcount;
    int index = 0;
    int currindex = -1;

    debugf("GetBestServer: ValidDNSServer bits  0x%x%x", validBits.l[1], validBits.l[0]);
    bestPenaltyTime = DNSSERVER_PENALTY_TIME + 1;
    for (curr = m->DNSServers; curr; curr = curr->next)
    {
        // skip servers that will soon be deleted
        if (curr->flags & DNSServerFlag_Delete)
        {
            debugf("GetBestServer: Delete set for index %d, DNS server %#a (Domain %##s), scoped %d", index, &curr->addr, curr->domain.c, curr->scopeType);
            continue;
        }

        // Check if this is a valid DNSServer
        if (!bit_get_opaque64(validBits, index))
        {
            debugf("GetBestServer: continuing for index %d", index);
            index++;
            continue;
        }

        currcount = CountLabels(&curr->domain);
        currPenaltyTime = PenaltyTimeForServer(m, curr);

        debugf("GetBestServer: Address %#a (Domain %##s), PenaltyTime(abs) %d, PenaltyTime(rel) %d",
               &curr->addr, curr->domain.c, curr->penaltyTime, currPenaltyTime);

        // If there are multiple best servers for a given question, we will pick the first one
        // if none of them are penalized. If some of them are penalized in that list, we pick
        // the least penalized one. BetterMatchForName walks through all best matches and
        // "currPenaltyTime < bestPenaltyTime" check lets us either pick the first best server
        // in the list when there are no penalized servers and least one among them
        // when there are some penalized servers.

        if (DNSServerMatch(curr, InterfaceID, ServiceID))
        {

            // If we know that all the names are already equally good matches, then skip calling BetterMatchForName.
            // This happens when we initially walk all the DNS servers and set the validity bit on the question.
            // Actually we just need PenaltyTime match, but for the sake of readability we just skip the expensive
            // part and still do some redundant steps e.g., InterfaceID match

            if (nameMatch)
                bettermatch = BetterMatchForName(name, namecount, &curr->domain, currcount, bestmatchlen);
            else
                bettermatch = 0;

            // If we found a better match (bettermatch == 1) then we don't need to
            // compare penalty times. But if we found an equal match, then we compare
            // the penalty times to pick a better match

            if ((bettermatch == 1) || ((bettermatch == 0) && currPenaltyTime < bestPenaltyTime))
            {
                currindex = index;
                curmatch = curr;
                bestmatchlen = currcount;
                bestPenaltyTime = currPenaltyTime;
            }
        }
        index++;
    }
    if (selected) *selected = currindex;
    return curmatch;
}

// Look up a DNS Server, matching by name and InterfaceID
mDNSlocal DNSServer *GetServerForName(mDNS *m, const domainname *name, mDNSInterfaceID InterfaceID, mDNSs32 ServiceID)
{
    DNSServer *curmatch = mDNSNULL;
    char *ifname = mDNSNULL;    // for logging purposes only
    mDNSOpaque128 allValid;

    if (InterfaceID == mDNSInterface_LocalOnly)
        InterfaceID = mDNSNULL;

    if (InterfaceID) ifname = InterfaceNameForID(m, InterfaceID);

    // By passing in all ones, we make sure that every DNS server is considered
    allValid.l[0] = allValid.l[1] = allValid.l[2] = allValid.l[3] = 0xFFFFFFFF;

    curmatch = GetBestServer(m, name, InterfaceID, ServiceID, allValid, mDNSNULL, mDNStrue);

    if (curmatch != mDNSNULL)
        LogInfo("GetServerForName: DNS server %#a:%d (Penalty Time Left %d) (Scope %s:%p) for %##s", &curmatch->addr,
                mDNSVal16(curmatch->port), (curmatch->penaltyTime ? (curmatch->penaltyTime - m->timenow) : 0), ifname ? ifname : "None",
                InterfaceID, name);
    else
        LogInfo("GetServerForName: no DNS server (Scope %s:%p) for %##s", ifname ? ifname : "None", InterfaceID, name);

    return(curmatch);
}

// Look up a DNS Server for a question within its valid DNSServer bits
mDNSexport DNSServer *GetServerForQuestion(mDNS *m, DNSQuestion *question)
{
    DNSServer *curmatch = mDNSNULL;
    char *ifname = mDNSNULL;    // for logging purposes only
    mDNSInterfaceID InterfaceID = question->InterfaceID;
    const domainname *name = &question->qname;
    int currindex;

    if (InterfaceID == mDNSInterface_LocalOnly)
        InterfaceID = mDNSNULL;

    if (InterfaceID)
        ifname = InterfaceNameForID(m, InterfaceID);

    if (!mDNSOpaque128IsZero(&question->validDNSServers))
    {
        curmatch = GetBestServer(m, name, InterfaceID, question->ServiceID, question->validDNSServers, &currindex, mDNSfalse);
        if (currindex != -1)
            bit_clr_opaque128(question->validDNSServers, currindex);
    }

    if (curmatch != mDNSNULL)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->Q%d] GetServerForQuestion: %p DNS server (%p) " PRI_IP_ADDR
            ":%d (Penalty Time Left %d) (Scope " PUB_S ":%p:%d) for " PRI_DM_NAME " (" PUB_S ")",
            question->request_id, mDNSVal16(question->TargetQID), question, curmatch, &curmatch->addr,
            mDNSVal16(curmatch->port), (curmatch->penaltyTime ? (curmatch->penaltyTime - m->timenow) : 0),
            ifname ? ifname : "None", InterfaceID, question->ServiceID, DM_NAME_PARAM(name),
            DNSTypeName(question->qtype));
    }
    else
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->Q%d] GetServerForQuestion: %p no DNS server (Scope " PUB_S ":%p:%d) for "
            PRI_DM_NAME " (" PUB_S ")", question->request_id, mDNSVal16(question->TargetQID), question,
            ifname ? ifname : "None", InterfaceID, question->ServiceID, DM_NAME_PARAM(name),
            DNSTypeName(question->qtype));
    }

    return(curmatch);
}
#endif // MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
// Called in normal client context (lock not held)
mDNSlocal void LLQNATCallback(mDNS *m, NATTraversalInfo *n)
{
    DNSQuestion *q;
    mDNS_Lock(m);
    LogInfo("LLQNATCallback external address:port %.4a:%u, NAT result %d", &n->ExternalAddress, mDNSVal16(n->ExternalPort), n->Result);
    n->clientContext = mDNSNULL; // we received at least one callback since starting this NAT-T
    for (q = m->Questions; q; q=q->next)
        if (ActiveQuestion(q) && !mDNSOpaque16IsZero(q->TargetQID) && q->LongLived)
            startLLQHandshake(m, q);    // If ExternalPort is zero, will do StartLLQPolling instead
    mDNS_Unlock(m);
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)

typedef enum
{
    mDNSSuppression_None               = 0,
    mDNSSuppression_BlockedByPolicy    = 1,
    mDNSSuppression_NoDNSService       = 2,
    mDNSSuppression_DenyCellular       = 3,
    mDNSSuppression_DenyExpensive      = 4,
    mDNSSuppression_DenyConstrained    = 5,
    mDNSSuppression_RecordsUnusable    = 6
} mDNSSuppression;

// This function takes the DNSServer as a separate argument because sometimes the
// caller has not yet assigned the DNSServer, but wants to evaluate the Suppressed
// status before switching to it.
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSlocal mDNSSuppression DetermineUnicastQuerySuppression(const DNSQuestion *const q, const mdns_dns_service_t dnsservice)
#else
mDNSlocal mDNSSuppression DetermineUnicastQuerySuppression(const DNSQuestion *const q, const DNSServer *const server)
#endif
{
    mDNSSuppression suppress = mDNSSuppression_None;
    const char *reason = mDNSNULL;

    if (q->BlockedByPolicy)
    {
        suppress = mDNSSuppression_BlockedByPolicy;
        reason   = " (blocked by policy)";
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    else if (!dnsservice)
    {
        if (!q->IsUnicastDotLocal)
        {
            suppress = mDNSSuppression_NoDNSService;
            reason   = " (no DNS service)";
        }
    }
#else
    else if (!server)
    {
        if (!q->IsUnicastDotLocal)
        {
            suppress = mDNSSuppression_NoDNSService;
            reason   = " (no DNS server)";
        }
    }
#endif
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    else if ((q->flags & kDNSServiceFlagsDenyCellular) && mdns_dns_service_interface_is_cellular(dnsservice))
#else
    else if ((q->flags & kDNSServiceFlagsDenyCellular) && server->isCell)
#endif
    {
        suppress = mDNSSuppression_DenyCellular;
        reason   = " (interface is cellular)";
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    else if ((q->flags & kDNSServiceFlagsDenyExpensive) && mdns_dns_service_interface_is_expensive(dnsservice))
#else
    else if ((q->flags & kDNSServiceFlagsDenyExpensive) && server->isExpensive)
#endif
    {
        suppress = mDNSSuppression_DenyExpensive;
        reason   = " (interface is expensive)";
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    else if ((q->flags & kDNSServiceFlagsDenyConstrained) && mdns_dns_service_interface_is_constrained(dnsservice))
#else
    else if ((q->flags & kDNSServiceFlagsDenyConstrained) && server->isConstrained)
#endif
    {
        suppress = mDNSSuppression_DenyConstrained;
        reason   = " (interface is constrained)";
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
    else if (q->SuppressUnusable && !DNS64IsQueryingARecord(q->dns64.state))
#else
    else if (q->SuppressUnusable)
#endif
    {
        if (q->qtype == kDNSType_A)
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            if (!mdns_dns_service_a_queries_advised(dnsservice))
#else
            if (!server->usableA)
#endif
            {
                suppress = mDNSSuppression_RecordsUnusable;
                reason   = " (A records are unusable)";
            }
        }
        else if (q->qtype == kDNSType_AAAA)
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
            if (!mdns_dns_service_aaaa_queries_advised(dnsservice))
#else
            if (!server->usableAAAA)
#endif
            {
                suppress = mDNSSuppression_RecordsUnusable;
                reason   = " (AAAA records are unusable)";
            }
        }
    }
    if (suppress != mDNSSuppression_None)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[Q%u] DetermineUnicastQuerySuppression: Query suppressed for " PRI_DM_NAME " " PUB_S PUB_S,
            mDNSVal16(q->TargetQID), DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype), reason ? reason : "");
    }
    return suppress;
}

mDNSlocal mDNSSuppression DetermineSuppression(const DNSQuestion *const q)
{
    // Multicast queries are never suppressed.
    if (mDNSOpaque16IsZero(q->TargetQID))
    {
        return mDNSSuppression_None;
    }
    else
    {
    #if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        const DNSServRef s = q->dnsservice;
    #else
        const DNSServRef s = q->qDNSServer;
    #endif
        return DetermineUnicastQuerySuppression(q, s);
    }
}

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    #define ShouldSuppressUnicastQueryScope mDNSexport
#else
    #define ShouldSuppressUnicastQueryScope mDNSlocal
#endif

ShouldSuppressUnicastQueryScope mDNSBool ShouldSuppressUnicastQuery(const DNSQuestion *const q, const DNSServRef s)
{
    return (DetermineUnicastQuerySuppression(q, s) != mDNSSuppression_None);
}

mDNSlocal mDNSBool ShouldSuppressQuery(const DNSQuestion *const q)
{
    return (DetermineSuppression(q) != mDNSSuppression_None);
}

mDNSlocal void CacheRecordRmvEventsForCurrentQuestion(mDNS *const m, DNSQuestion *q)
{
    CacheRecord *cr;
    CacheGroup *cg;

    cg = CacheGroupForName(m, q->qnamehash, &q->qname);
    for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
    {
        // Don't deliver RMV events for negative records if the question has not required DNSSEC RRs.
        if (cr->resrec.RecordType == kDNSRecordTypePacketNegative
        #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
            && !dns_question_is_primary_dnssec_requestor(q)
        #endif
            )
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[Q%u] CacheRecordRmvEventsForCurrentQuestion: Suppressing RMV events for question - "
                "rr name: " PRI_DM_NAME ", rr type: " PUB_DNS_TYPE ", current active question: Q%u, current answers: %u", mDNSVal16(q->TargetQID),
                DM_NAME_PARAM(cr->resrec.name), DNS_TYPE_PARAM(cr->resrec.rrtype),
                cr->CRActiveQuestion ? mDNSVal16(cr->CRActiveQuestion->TargetQID) : 0, q->CurrentAnswers);

            continue;
        }

        if (SameNameCacheRecordAnswersQuestion(cr, q))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[Q%u] CacheRecordRmvEventsForCurrentQuestion: Calling AnswerCurrentQuestionWithResourceRecord (RMV) for question - "
                "rr name: " PRI_DM_NAME ", rr type: " PUB_DNS_TYPE ", local answers: %u", mDNSVal16(q->TargetQID),
                DM_NAME_PARAM(cr->resrec.name), DNS_TYPE_PARAM(cr->resrec.rrtype), q->LOAddressAnswers);

            q->CurrentAnswers--;
            if (cr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers--;
            if (cr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers--;
            AnswerCurrentQuestionWithResourceRecord(m, cr, QC_rmv);
            if (m->CurrentQuestion != q) break;     // If callback deleted q, then we're finished here
        }
    }
}

mDNSlocal mDNSBool IsQuestionInList(const DNSQuestion *const list, const DNSQuestion *const question)
{
    for (const DNSQuestion *q = list; q; q = q->next)
    {
        if (q == question)
        {
            return mDNStrue;
        }
    }
    return mDNSfalse;
}

mDNSlocal mDNSBool IsQuestionNew(const mDNS *const m, const DNSQuestion *const question)
{
    return IsQuestionInList(m->NewQuestions, question);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSexport mDNSBool LocalRecordRmvEventsForQuestion(mDNS *const m, DNSQuestion *q)
#else
mDNSlocal mDNSBool LocalRecordRmvEventsForQuestion(mDNS *const m, DNSQuestion *q)
#endif
{
    AuthRecord *rr;
    AuthGroup *ag;

    if (m->CurrentQuestion)
        LogMsg("LocalRecordRmvEventsForQuestion: ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));

    if (IsQuestionNew(m, q))
    {
        LogInfo("LocalRecordRmvEventsForQuestion: New Question %##s (%s)", q->qname.c, DNSTypeName(q->qtype));
        return mDNStrue;
    }
    m->CurrentQuestion = q;
    ag = AuthGroupForName(&m->rrauth, q->qnamehash, &q->qname);
    if (ag)
    {
        for (rr = ag->members; rr; rr=rr->next)
            // Filter the /etc/hosts records - LocalOnly, Unique, A/AAAA/CNAME
            if (UniqueLocalOnlyRecord(rr) && LocalOnlyRecordAnswersQuestion(rr, q))
            {
                LogInfo("LocalRecordRmvEventsForQuestion: Delivering possible Rmv events with record %s",
                        ARDisplayString(m, rr));
                if (q->CurrentAnswers <= 0 || q->LOAddressAnswers <= 0)
                {
                    LogMsg("LocalRecordRmvEventsForQuestion: ERROR!! CurrentAnswers or LOAddressAnswers is zero %p %##s"
                           " (%s) CurrentAnswers %d, LOAddressAnswers %d", q, q->qname.c, DNSTypeName(q->qtype),
                           q->CurrentAnswers, q->LOAddressAnswers);
                    continue;
                }
                AnswerLocalQuestionWithLocalAuthRecord(m, rr, QC_rmv);      // MUST NOT dereference q again
                if (m->CurrentQuestion != q) { m->CurrentQuestion = mDNSNULL; return mDNSfalse; }
            }
    }
    m->CurrentQuestion = mDNSNULL;
    return mDNStrue;
}

// Returns false if the question got deleted while delivering the RMV events
// The caller should handle the case
mDNSexport mDNSBool CacheRecordRmvEventsForQuestion(mDNS *const m, DNSQuestion *q)
{
    if (m->CurrentQuestion)
        LogMsg("CacheRecordRmvEventsForQuestion: ERROR m->CurrentQuestion already set: %##s (%s)",
               m->CurrentQuestion->qname.c, DNSTypeName(m->CurrentQuestion->qtype));

    // If it is a new question, we have not delivered any ADD events yet. So, don't deliver RMV events.
    // If this question was answered using local auth records, then you can't deliver RMVs using cache
    if (!IsQuestionNew(m, q) && !q->LOAddressAnswers)
    {
        m->CurrentQuestion = q;
        CacheRecordRmvEventsForCurrentQuestion(m, q);
        if (m->CurrentQuestion != q) { m->CurrentQuestion = mDNSNULL; return mDNSfalse; }
        m->CurrentQuestion = mDNSNULL;
    }
    else { LogInfo("CacheRecordRmvEventsForQuestion: Question %p %##s (%s) is a new question", q, q->qname.c, DNSTypeName(q->qtype)); }
    return mDNStrue;
}

mDNSlocal void SuppressStatusChanged(mDNS *const m, DNSQuestion *q, DNSQuestion **restart)
{
    // NOTE: CacheRecordRmvEventsForQuestion will not generate RMV events for queries that have non-zero
    // LOAddressAnswers. Hence it is important that we call CacheRecordRmvEventsForQuestion before
    // LocalRecordRmvEventsForQuestion (which decrements LOAddressAnswers)
    if (q->Suppressed)
    {
        q->Suppressed = mDNSfalse;
        if (!CacheRecordRmvEventsForQuestion(m, q))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->Q%u] SuppressStatusChanged: Question deleted while delivering RMV events from cache",
                q->request_id, mDNSVal16(q->TargetQID));
            return;
        }
        q->Suppressed = mDNStrue;
    }

    // SuppressUnusable does not affect questions that are answered from the local records (/etc/hosts)
    // and Suppressed status does not mean anything for these questions. As we are going to stop the
    // question below, we need to deliver the RMV events so that the ADDs that will be delivered during
    // the restart will not be a duplicate ADD
    if (!LocalRecordRmvEventsForQuestion(m, q))
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[R%u->Q%u] SuppressStatusChanged: Question deleted while delivering RMV events from Local AuthRecords",
            q->request_id, mDNSVal16(q->TargetQID));
        return;
    }

    // There are two cases here.
    //
    // 1. Previously it was suppressed and now it is not suppressed, restart the question so
    // that it will start as a new question. Note that we can't just call ActivateUnicastQuery
    // because when we get the response, if we had entries in the cache already, it will not answer
    // this question if the cache entry did not change. Hence, we need to restart
    // the query so that it can be answered from the cache.
    //
    // 2. Previously it was not suppressed and now it is suppressed. We need to restart the questions
    // so that we redo the duplicate checks in mDNS_StartQuery_internal. A SuppressUnusable question
    // is a duplicate of non-SuppressUnusable question if it is not suppressed (Suppressed is false).
    // A SuppressUnusable question is not a duplicate of non-SuppressUnusable question if it is suppressed
    // (Suppressed is true). The reason for this is that when a question is suppressed, we want an
    // immediate response and not want to be blocked behind a question that is querying DNS servers. When
    // the question is not suppressed, we don't want two active questions sending packets on the wire.
    // This affects both efficiency and also the current design where there is only one active question
    // pointed to from a cache entry.
    //
    // We restart queries in a two step process by first calling stop and build a temporary list which we
    // will restart at the end. The main reason for the two step process is to handle duplicate questions.
    // If there are duplicate questions, calling stop inherits the values from another question on the list (which
    // will soon become the real question) including q->ThisQInterval which might be zero if it was
    // suppressed before. At the end when we have restarted all questions, none of them is active as each
    // inherits from one another and we need to reactivate one of the questions here which is a little hacky.
    //
    // It is much cleaner and less error prone to build a list of questions and restart at the end.

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->Q%u] SuppressStatusChanged: Stop question %p " PRI_DM_NAME " (" PUB_S ")",
        q->request_id, mDNSVal16(q->TargetQID), q, DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype));
    mDNS_StopQuery_internal(m, q);
    q->next = *restart;
    *restart = q;
}

#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
mDNSexport void mDNSCoreReceiveD2DResponse(mDNS *const m, const DNSMessage *const response, const mDNSu8 *end,
    const mDNSAddr *srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, mDNSIPPort dstport,
    const mDNSInterfaceID InterfaceID)
{
        mDNSCoreReceiveResponse(m, response, end, srcaddr, srcport, dstaddr, dstport, mDNSNULL, mDNSNULL, InterfaceID);
}
#endif

// The caller should hold the lock
mDNSexport void CheckSuppressUnusableQuestions(mDNS *const m)
{
    DNSQuestion *q;
    DNSQuestion *restart = mDNSNULL;

    // We look through all questions including new questions. During network change events,
    // we potentially restart questions here in this function that ends up as new questions,
    // which may be suppressed at this instance. Before it is handled we get another network
    // event that changes the status e.g., address becomes available. If we did not process
    // new questions, we would never change its Suppressed status.
    //
    // CurrentQuestion is used by RmvEventsForQuestion below. While delivering RMV events, the
    // application callback can potentially stop the current question (detected by CurrentQuestion) or
    // *any* other question which could be the next one that we may process here. RestartQuestion
    // points to the "next" question which will be automatically advanced in mDNS_StopQuery_internal
    // if the "next" question is stopped while the CurrentQuestion is stopped
    if (m->RestartQuestion)
        LogMsg("CheckSuppressUnusableQuestions: ERROR!! m->RestartQuestion already set: %##s (%s)",
               m->RestartQuestion->qname.c, DNSTypeName(m->RestartQuestion->qtype));
    m->RestartQuestion = m->Questions;
    while (m->RestartQuestion)
    {
        q = m->RestartQuestion;
        m->RestartQuestion = q->next;
        if (q->SuppressUnusable)
        {
            const mDNSBool old = q->Suppressed;
            q->Suppressed = ShouldSuppressQuery(q);
            if (q->Suppressed != old)
            {
                // Previously it was not suppressed, Generate RMV events for the ADDs that we might have delivered before
                // followed by a negative cache response. Temporarily turn off suppression so that
                // AnswerCurrentQuestionWithResourceRecord can answer the question
                SuppressStatusChanged(m, q, &restart);
            }
        }
    }
    while (restart)
    {
        q = restart;
        restart = restart->next;
        q->next = mDNSNULL;
        LogInfo("CheckSuppressUnusableQuestions: Start question %p %##s (%s)", q, q->qname.c, DNSTypeName(q->qtype));
        mDNS_StartQuery_internal(m, q);
    }
}

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSlocal void RestartUnicastQuestions(mDNS *const m)
{
    DNSQuestion *q;
    DNSQuestion *restartList = mDNSNULL;

    if (m->RestartQuestion)
        LogMsg("RestartUnicastQuestions: ERROR!! m->RestartQuestion already set: %##s (%s)",
               m->RestartQuestion->qname.c, DNSTypeName(m->RestartQuestion->qtype));
    m->RestartQuestion = m->Questions;
    while (m->RestartQuestion)
    {
        q = m->RestartQuestion;
        m->RestartQuestion = q->next;
        if (q->Restart)
        {
            if (mDNSOpaque16IsZero(q->TargetQID))
                LogMsg("RestartUnicastQuestions: ERROR!! Restart set for multicast question %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

            q->Restart = mDNSfalse;
            SuppressStatusChanged(m, q, &restartList);
        }
    }
    while ((q = restartList) != mDNSNULL)
    {
        restartList = q->next;
        q->next = mDNSNULL;
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->Q%u] RestartUnicastQuestions: Start question %p " PRI_DM_NAME " (" PUB_S ")",
             q->request_id, mDNSVal16(q->TargetQID), q, DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype));
        mDNS_StartQuery_internal(m, q);
    }
}
#endif

// ValidateParameters() is called by mDNS_StartQuery_internal() to check the client parameters of
// DNS Question that are already set by the client before calling mDNS_StartQuery()
mDNSlocal mStatus ValidateParameters(mDNS *const m, DNSQuestion *const question)
{
    if (!ValidateDomainName(&question->qname))
    {
        LogMsg("ValidateParameters: Attempt to start query with invalid qname %##s (%s)", question->qname.c, DNSTypeName(question->qtype));
        return(mStatus_Invalid);
    }

    // If this question is referencing a specific interface, verify it exists
    if (question->InterfaceID && !LocalOnlyOrP2PInterface(question->InterfaceID))
    {
        NetworkInterfaceInfo *intf = FirstInterfaceForID(m, question->InterfaceID);
        if (!intf)
            LogInfo("ValidateParameters: Note: InterfaceID %d for question %##s (%s) not currently found in active interface list",
                    IIDPrintable(question->InterfaceID), question->qname.c, DNSTypeName(question->qtype));
    }

    return(mStatus_NoError);
}

// InitDNSConfig() is called by InitCommonState() to initialize the DNS configuration of the Question.
// These are a subset of the internal uDNS fields. Must be done before ShouldSuppressQuery() & mDNS_PurgeBeforeResolve()
mDNSlocal void InitDNSConfig(mDNS *const m, DNSQuestion *const question)
{
    // First reset all DNS Configuration
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_forget(&question->dnsservice);
    question->NeedUpdatedQuerier  = mDNSfalse;
#else
    question->qDNSServer          = mDNSNULL;
    question->validDNSServers     = zeroOpaque128;
    question->triedAllServersOnce = mDNSfalse;
    question->noServerResponse    = mDNSfalse;
#endif
    question->StopTime            = (question->TimeoutQuestion) ? question->StopTime : 0;
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS)
    mDNSPlatformMemZero(&question->metrics, sizeof(question->metrics));
#endif

    // Need not initialize the DNS Configuration for Local Only OR P2P Questions when timeout not specified
    if (LocalOnlyOrP2PInterface(question->InterfaceID) && !question->TimeoutQuestion)
        return;
    // Proceed to initialize DNS Configuration (some are set in SetValidDNSServers())
    if (!mDNSOpaque16IsZero(question->TargetQID))
    {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        mDNSu32 timeout = 30;
#else
        mDNSu32 timeout = SetValidDNSServers(m, question);
#endif
        // We set the timeout value the first time mDNS_StartQuery_internal is called for a question.
        // So if a question is restarted when a network change occurs, the StopTime is not reset.
        // Note that we set the timeout for all questions. If this turns out to be a duplicate,
        // it gets a full timeout value even if the original question times out earlier.
        if (question->TimeoutQuestion && !question->StopTime)
        {
            question->StopTime = NonZeroTime(m->timenow + timeout * mDNSPlatformOneSecond);
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
                "[Q%u] InitDNSConfig: Setting StopTime on the uDNS question %p " PRI_DM_NAME " (" PUB_S ")",
                mDNSVal16(question->TargetQID), question, DM_NAME_PARAM(&question->qname),
                DNSTypeName(question->qtype));
        }

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        Querier_SetDNSServiceForQuestion(question);
#else
        question->qDNSServer = GetServerForQuestion(m, question);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "[R%u->Q%u] InitDNSConfig: question %p " PRI_DM_NAME " " PUB_S " Timeout %d, DNS Server "
            PRI_IP_ADDR ":%d",
            question->request_id, mDNSVal16(question->TargetQID), question, DM_NAME_PARAM(&question->qname),
            DNSTypeName(question->qtype), timeout, question->qDNSServer ? &question->qDNSServer->addr : mDNSNULL,
            mDNSVal16(question->qDNSServer ? question->qDNSServer->port : zeroIPPort));
#endif
    }
    else if (question->TimeoutQuestion && !question->StopTime)
    {
        // If the question is to be timed out and its a multicast, local-only or P2P case,
        // then set it's stop time.
        mDNSu32 timeout = LocalOnlyOrP2PInterface(question->InterfaceID) ?
                            DEFAULT_LO_OR_P2P_TIMEOUT : GetTimeoutForMcastQuestion(m, question);
        question->StopTime = NonZeroTime(m->timenow + timeout * mDNSPlatformOneSecond);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
            "[R%u->Q%u] InitDNSConfig: Setting StopTime on the uDNS question %p " PRI_DM_NAME " (" PUB_S ")",
            question->request_id, mDNSVal16(question->TargetQID), question, DM_NAME_PARAM(&question->qname),
            DNSTypeName(question->qtype));
    }
    // Set StopTime here since it is a part of DNS Configuration
    if (question->StopTime)
        SetNextQueryStopTime(m, question);
    // Don't call SetNextQueryTime() if a LocalOnly OR P2P Question since those questions
    // will never be transmitted on the wire.
    if (!(LocalOnlyOrP2PInterface(question->InterfaceID)))
        SetNextQueryTime(m,question);
}

// InitCommonState() is called by mDNS_StartQuery_internal() to initialize the common(uDNS/mDNS) internal
// state fields of the DNS Question. These are independent of the Client layer.
mDNSlocal void InitCommonState(mDNS *const m, DNSQuestion *const question)
{
    // Note: In the case where we already have the answer to this question in our cache, that may be all the client
    // wanted, and they may immediately cancel their question. In this case, sending an actual query on the wire would
    // be a waste. For that reason, we schedule our first query to go out in half a second (InitialQuestionInterval).
    // If AnswerNewQuestion() finds that we have *no* relevant answers currently in our cache, then it will accelerate
    // that to go out immediately.
    question->next              = mDNSNULL;
    // ThisQInterval should be initialized before any memory allocations occur. If malloc
    // debugging is turned on within mDNSResponder (see mDNSDebug.h for details) it validates
    // the question list to check if ThisQInterval is negative which means the question has been
    // stopped and can't be on the list. The question is already on the list and ThisQInterval
    // can be negative if the caller just stopped it and starting it again. Hence, it always has to
    // be initialized. CheckForSoonToExpireRecords below prints the cache records when logging is
    // turned ON which can allocate memory e.g., base64 encoding.
    question->ThisQInterval     = InitialQuestionInterval;                  // MUST be > zero for an active question
    question->qnamehash         = DomainNameHashValue(&question->qname);
    mDNSs32 delay = 0;
    if (mDNSOpaque16IsZero(question->TargetQID))
    {
        delay = CheckForSoonToExpireRecordsEx(m, &question->qname, question->qnamehash, question->qtype, question->qclass);
    }
    question->DelayAnswering    = delay;
    question->LastQTime         = m->timenow;
    question->ExpectUnicastResp = 0;
    question->LastAnswerPktNum  = m->PktNum;
    question->RecentAnswerPkts  = 0;
    question->CurrentAnswers    = 0;

   question->BrowseThreshold   = 0;
    question->CachedAnswerNeedsUpdate = mDNSfalse;

    question->LargeAnswers      = 0;
    question->UniqueAnswers     = 0;
    question->LOAddressAnswers  = 0;
    question->FlappingInterface1 = mDNSNULL;
    question->FlappingInterface2 = mDNSNULL;

    // mDNSPlatformGetDNSRoutePolicy() and InitDNSConfig() may set a DNSQuestion's BlockedByPolicy value,
    // so they should be called before calling ShouldSuppressQuery(), which checks BlockedByPolicy.
    question->BlockedByPolicy = mDNSfalse;

    // if kDNSServiceFlagsServiceIndex flag is SET by the client, then do NOT call mDNSPlatformGetDNSRoutePolicy()
    // since we would already have the question->ServiceID in that case.
    if (!(question->flags & kDNSServiceFlagsServiceIndex))
    {
        question->ServiceID = -1;
    }
    else
        LogInfo("InitCommonState: Query for %##s (%s), PID[%d], EUID[%d], ServiceID[%d] is already set by client", question->qname.c,
                DNSTypeName(question->qtype), question->pid, question->euid, question->ServiceID);

    InitDNSConfig(m, question);
    question->AuthInfo          = GetAuthInfoForQuestion(m, question);
    const mDNSSuppression suppress = DetermineSuppression(question);
    question->Suppressed        = (suppress != mDNSSuppression_None);
    question->ForceCNAMEFollows = question->PersistWhenRecordsUnusable && (suppress == mDNSSuppression_RecordsUnusable);
    question->NextInDQList      = mDNSNULL;
    question->SendQNow          = mDNSNULL;
    question->SendOnAll         = mDNSfalse;
    question->RequestUnicast    = kDefaultRequestUnicastCount;


    question->LastQTxTime       = m->timenow;
    question->CNAMEReferrals    = 0;

    question->WakeOnResolveCount = 0;
    if (question->WakeOnResolve)
    {
        question->WakeOnResolveCount = InitialWakeOnResolveCount;
    }
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    question->Restart = mDNSfalse;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    question->dnssec = mDNSNULL;
#endif

    debugf("InitCommonState: Question %##s (%s) Interface %p Now %d Send in %d Answer in %d (%p) %s (%p)",
            question->qname.c, DNSTypeName(question->qtype), question->InterfaceID, m->timenow,
            NextQSendTime(question) - m->timenow,
            question->DelayAnswering ? question->DelayAnswering - m->timenow : 0,
            question, question->DuplicateOf ? "duplicate of" : "not duplicate", question->DuplicateOf);

    if (question->DelayAnswering)
        LogInfo("InitCommonState: Delaying answering for %d ticks while cache stabilizes for %##s (%s)",
                 question->DelayAnswering - m->timenow, question->qname.c, DNSTypeName(question->qtype));
}

// Excludes the DNS Config fields which are already handled by InitDNSConfig()
mDNSlocal void InitWABState(DNSQuestion *const question)
{
    // We'll create our question->LocalSocket on demand, if needed.
    // We won't need one for duplicate questions, or from questions answered immediately out of the cache.
    // We also don't need one for LLQs because (when we're using NAT) we want them all to share a single
    // NAT mapping for receiving inbound add/remove events.
    question->LocalSocket       = mDNSNULL;
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mdns_client_forget(&question->client);
#else
    question->unansweredQueries = 0;
#endif
    question->nta               = mDNSNULL;
    question->servAddr          = zeroAddr;
    question->servPort          = zeroIPPort;
    question->tcp               = mDNSNULL;
}

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
mDNSlocal void InitLLQNATState(mDNS *const m)
{
    // If we don't have our NAT mapping active, start it now
    if (!m->LLQNAT.clientCallback)
    {
        m->LLQNAT.Protocol       = NATOp_MapUDP;
        m->LLQNAT.IntPort        = m->UnicastPort4;
        m->LLQNAT.RequestedPort  = m->UnicastPort4;
        m->LLQNAT.clientCallback = LLQNATCallback;
        m->LLQNAT.clientContext  = (void*)1; // Means LLQ NAT Traversal just started
        mDNS_StartNATOperation_internal(m, &m->LLQNAT);
    }
}
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)

mDNSlocal void InitLLQState(DNSQuestion *const question)
{
    question->state             = LLQ_Init;
    question->ReqLease          = 0;
    question->expire            = 0;
    question->ntries            = 0;
    question->id                = zeroOpaque64;
}

// InitDNSSECProxyState() is called by mDNS_StartQuery_internal() to initialize
// DNSSEC & DNS Proxy fields of the DNS Question.
mDNSlocal void InitDNSSECProxyState(mDNS *const m, DNSQuestion *const question)
{
    (void) m;
    question->responseFlags = zeroID;
}

// Once the question is completely initialized including the duplicate logic, this function
// is called to finalize the unicast question which requires flushing the cache if needed,
// activating the query etc.
mDNSlocal void FinalizeUnicastQuestion(mDNS *const m, DNSQuestion *question)
{
    // Ensure DNS related info of duplicate question is same as the orig question
    if (question->DuplicateOf)
    {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        const DNSQuestion *const duplicateOf = question->DuplicateOf;
        mdns_replace(&question->dnsservice, duplicateOf->dnsservice);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%u->DupQ%u->Q%u] Duplicate question " PRI_DM_NAME " (" PUB_S ")",
           question->request_id, mDNSVal16(question->TargetQID), mDNSVal16(duplicateOf->TargetQID),
           DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype));
#else
        question->validDNSServers = question->DuplicateOf->validDNSServers;
        // If current(dup) question has DNS Server assigned but the original question has no DNS Server assigned to it,
        // then we log a line as it could indicate an issue
        if (question->DuplicateOf->qDNSServer == mDNSNULL)
        {
            if (question->qDNSServer)
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->Q%d] FinalizeUnicastQuestion: Current(dup) question %p has DNSServer(" PRI_IP_ADDR
                    ":%d) but original question(%p) has no DNS Server! " PRI_DM_NAME " (" PUB_S ")",
                    question->request_id, mDNSVal16(question->TargetQID), question,
                    question->qDNSServer ? &question->qDNSServer->addr : mDNSNULL,
                    mDNSVal16(question->qDNSServer ? question->qDNSServer->port : zeroIPPort), question->DuplicateOf,
                    DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype));
            }
        }
        question->qDNSServer = question->DuplicateOf->qDNSServer;
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "[R%d->DupQ%d->Q%d] FinalizeUnicastQuestion: Duplicate question %p (%p) " PRI_DM_NAME " (" PUB_S
            "), DNS Server " PRI_IP_ADDR ":%d",
            question->request_id, mDNSVal16(question->TargetQID), mDNSVal16(question->DuplicateOf->TargetQID),
            question, question->DuplicateOf, DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype),
            question->qDNSServer ? &question->qDNSServer->addr : mDNSNULL,
            mDNSVal16(question->qDNSServer ? question->qDNSServer->port : zeroIPPort));
#endif
    }

    ActivateUnicastQuery(m, question, mDNSfalse);

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
    if (question->LongLived)
    {
        // Unlike other initializations, InitLLQNATState should be done after
        // we determine that it is a unicast question.  LongLived is set for
        // both multicast and unicast browse questions but we should initialize
        // the LLQ NAT state only for LLQ. Otherwise we will unnecessarily
        // start the NAT traversal that is not needed.
        InitLLQNATState(m);
    }
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
}

mDNSlocal void InsertNewQuestionInList(DNSQuestion *const question)
{
    DNSQuestion **qptr;
    DNSQuestion *q;
    const mDNSBool localOnlyOrP2P = LocalOnlyOrP2PInterface(question->InterfaceID);
    mDNS *const m = &mDNSStorage;
    DNSQuestion **const newQSublistPtr = localOnlyOrP2P ? &m->NewLocalOnlyQuestions : &m->NewQuestions;
    const DNSQuestion *const newQSublist = *newQSublistPtr;
    mDNSBool inNewQSublist = mDNSfalse;
    mDNSBool passedPrimary = mDNSfalse;
    for (qptr = localOnlyOrP2P ? &m->LocalOnlyQuestions : &m->Questions; (q = *qptr) != mDNSNULL; qptr = &q->next)
    {
        // If there's no new question sublist, then keep going until the end of the question list so that the new
        // question can be appended. Otherwise, the new question needs to be carefully inserted in the sublist such that
        // it doesn't precede any questions that are supposed to be answered earlier or at the same clock time as
        // itself, and such that it doesn't follow any questions that are supposed to be answered later than itself.
        if (newQSublist)
        {
            // We've entered the new question sublist when we encounter the head of the sublist.
            if (!inNewQSublist && (q == newQSublist))
            {
                inNewQSublist = mDNStrue;
            }
            // If we're in the new question sublist, check if it's appropriate to insert the new question before the
            // current question:
            //
            // 1. The current question must also have a delayed answering time.
            // 2. For questions that are duplicates, make sure that the new question is inserted after its primary
            //    question because the rest of the code expects a duplicate question to always come after its primary
            //    question in a question list.
            if (inNewQSublist && (q->DelayAnswering != 0) && (!question->DuplicateOf || passedPrimary))
            {
                // Determine when the new question's answering should begin.
                const mDNSs32 answeringTime = (question->DelayAnswering != 0) ? question->DelayAnswering : m->timenow;
                // If the current question's answering time is later, then the new question should be inserted before.
                if ((q->DelayAnswering - answeringTime) > 0)
                {
                    break;
                }
            }
            if (!passedPrimary && question->DuplicateOf && (q == question->DuplicateOf))
            {
                passedPrimary = mDNStrue;
            }
        }
    }
    // Insert the new question in the list.
    question->next = q;
    *qptr = question;
    // If there was no new question sublist, or the new question was inserted right before the head of the sublist, then
    // the new question becomes the head of the new question sublist.
    if (!newQSublist || (newQSublist == q))
    {
        *newQSublistPtr = question;
    }
}

mDNSexport mStatus mDNS_StartQuery_internal(mDNS *const m, DNSQuestion *const question)
{
    mStatus err;

    // First check for cache space (can't do queries if there is no cache space allocated)
    mdns_require_action_quiet(m->rrcache_size > 0, exit, err = mStatus_NoCache);

    err = ValidateParameters(m, question);
    mdns_require_noerr_quiet(err, exit);

#ifdef USE_LIBIDN
    // If the TLD includes high-ascii bytes, assume it will need to be converted to Punycode.
    // (In the future the root name servers may answer UTF-8 queries directly, but for now they do not.)
    // This applies to the top label (TLD) only
    // -- for the second level and down we try UTF-8 first, and then fall back to Punycode only if UTF-8 fails.
    if (IsHighASCIILabel(LastLabel(&question->qname)))
    {
        domainname newname;
        if (PerformNextPunycodeConversion(question, &newname))
            AssignDomainName(&question->qname, &newname);
    }
#endif // USE_LIBIDN

#ifndef UNICAST_DISABLED
    question->TargetQID = Question_uDNS(question) ? mDNS_NewMessageID(m) : zeroID;
#else
    question->TargetQID = zeroID;
#endif
    debugf("mDNS_StartQuery_internal: %##s (%s)", question->qname.c, DNSTypeName(question->qtype));

#if MDNSRESPONDER_SUPPORTS(APPLE, TERMINUS_ASSISTED_UNICAST_DISCOVERY)
    // Check if the query meets the requirement of using mDNS alternative service and if we have such a service
    // available.
    if (DNSQuestionIsEligibleForMDNSAlternativeService(question) &&
        Querier_IsMDNSAlternativeServiceAvailableForQuestion(question))
    {
        // If so, we convert it to a non-mDNS query to exclude any mDNS query.
        question->TargetQID = mDNS_NewMessageID(m);
        // After the operation above, DNSQuestionRequestsMDNSAlternativeService returns true indicating that this
        // question is an mDNS question but requests an mDNS alternative service to be used.
    }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
    // This code block must:
    // 1. Be put after question ID is assigned.
    // 2. Be put before any log is printed, or domain name information may leak.
    if (DNSQuestionNeedsSensitiveLogging(question))
    {
        gNumOfSensitiveLoggingEnabledQuestions++;
        if (!gSensitiveLoggingEnabled)
        {
            gSensitiveLoggingEnabled = mDNStrue;
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[Q%u] Question enables sensitive logging, all the sensitive level logs and the state dump of the question will now be redacted.",
                mDNSVal16(question->TargetQID));
        }
        else
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
                "[Q%u] Question enables sensitive logging, redaction already in effect. - "
                "number of enabled questions: %d.", mDNSVal16(question->TargetQID),
                gNumOfSensitiveLoggingEnabledQuestions);
        }
    }
#endif // MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)

    const mDNSBool localOnlyOrP2P = LocalOnlyOrP2PInterface(question->InterfaceID);
#if MDNSRESPONDER_SUPPORTS(APPLE, TRACKER_STATE)
    if (resolved_cache_is_enabled()                             &&
        !mDNSOpaque16IsZero(question->TargetQID)                &&
        question->qclass == kDNSClass_IN                        &&
        !localOnlyOrP2P                                         &&
        (question->qtype == kDNSType_AAAA   ||
         question->qtype == kDNSType_A      ||
         question->qtype == kDNSType_CNAME))
    {
        resolved_cache_prepend_name(question, &question->qname);
    }
#endif
    const DNSQuestion *const qlist = localOnlyOrP2P ? m->LocalOnlyQuestions : m->Questions;
    if (IsQuestionInList(qlist, question))
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartQuery_internal: Error! Tried to add a question " PRI_DM_NAME " (" PUB_S ") %p "
            "that's already in the active list",
            DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype), question);
        err = mStatus_AlreadyRegistered;
        goto exit;
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH) && !defined(UNIT_TEST)
    // If the question has requested using DNS push, check if it already has DNS push context initialized:
    // 1. If it is initialized, then it is a restarted DNS push question, so we do not reinitialize DNS push context.
    // 2. If it is uninitialized, then initialize it and start DNS push bootstrapping.
    if (dns_question_enables_dns_push(question) &&
        !Querier_IsCustomPushServiceAvailableForQuestion(question) &&
        !dns_question_uses_dns_push(question))
    {
        const dns_obj_error_t dns_push_err = dns_push_handle_question_start(m, question);
        mdns_require_noerr_action_quiet(dns_push_err, exit, err = mStatus_BadParamErr);
    }
#endif

    // Intialize the question. The only ordering constraint we have today is that
    // InitDNSSECProxyState should be called after the DNS server is selected (in
    // InitCommonState -> InitDNSConfig) as DNS server selection affects DNSSEC
    // validation.

    InitCommonState(m, question);
    InitWABState(question);
    InitLLQState(question);
    InitDNSSECProxyState(m, question);

    // FindDuplicateQuestion should be called last after all the intialization
    // as the duplicate logic could be potentially based on any field in the
    // question.
    question->DuplicateOf  = FindDuplicateQuestion(m, question);
    if (question->DuplicateOf)
        question->AuthInfo = question->DuplicateOf->AuthInfo;

    InsertNewQuestionInList(question);
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    if (question->enableDNSSEC)
    {
        const dnssec_error_t dnssec_err = dnssec_handle_question_start(m, question);
        mdns_require_noerr_action_quiet(dnssec_err, exit, err = mStatus_UnknownErr);
    }
#endif
    if (question->request_id == 0)
    {
        // After: [Q0] mDNS_StartQuery_internal START -- qname: 70-35-60-63\134.1\134 joey-test-atv._sleep-proxy._udp.local., qtype: SRV
        // request_id == 0 indicates that the query is started by mDNSResponder itself, in which case no request
        // level log is available. Therefore, we need to print the query start log message to indicate that
        // mDNSResponder has started a query internally.
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
            "[Q%u] mDNS_StartQuery_internal START -- qname: " PRI_DM_NAME " (%x), qtype: " PUB_DNS_TYPE,
            mDNSVal16(question->TargetQID), DM_NAME_PARAM_NONNULL(&question->qname),
            mDNS_DomainNameFNV1aHash(&question->qname), DNS_TYPE_PARAM(question->qtype));
    }
    if (!localOnlyOrP2P)
    {
        // If the question's id is non-zero, then it's Wide Area
        // MUST NOT do this Wide Area setup until near the end of
        // mDNS_StartQuery_internal -- this code may itself issue queries (e.g. SOA,
        // NS, etc.) and if we haven't finished setting up our own question and setting
        // m->NewQuestions if necessary then we could end up recursively re-entering
        // this routine with the question list data structures in an inconsistent state.
        if (!mDNSOpaque16IsZero(question->TargetQID))
        {
            FinalizeUnicastQuestion(m, question);
        }
        else
        {
#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
            m->NumAllInterfaceQuestions++;
            if (m->NumAllInterfaceRecords + m->NumAllInterfaceQuestions == 1)
            {
                m->NextBonjourDisableTime = 0;
                if (m->BonjourEnabled == 0)
                {
                    // Enable Bonjour immediately by scheduling network changed processing where
                    // we will join the multicast group on each active interface.
                    m->BonjourEnabled = 1;
                    m->NetworkChanged = m->timenow;
                }
            }
#endif
            if (question->WakeOnResolve)
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                    "[Q%u] mDNS_StartQuery_internal: Purging records before resolving",
                    mDNSVal16(question->TargetQID));
                mDNS_PurgeBeforeResolve(m, question);
            }
        }
    }

#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
    // Only start the resolver discovery if the question meets condition to do so.
    // This ensures that we do not recursively start resolver discovery when we start a query to discover the DNS resolver.
    const domainname *domain_to_discover_resolver = mDNSNULL;
    if (dns_question_requires_resolver_discovery(question, &domain_to_discover_resolver))
    {
        // Inside mDNS_StartQuery_internal(), we already grabbed the lock, so no need to lock it again.
        resolver_discovery_add(domain_to_discover_resolver, mDNSfalse);
    }
#endif // MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)

exit:
    return err;
}

// CancelGetZoneData is an internal routine (i.e. must be called with the lock already held)
mDNSexport void CancelGetZoneData(mDNS *const m, ZoneData *nta)
{
    debugf("CancelGetZoneData %##s (%s)", nta->question.qname.c, DNSTypeName(nta->question.qtype));
    // This function may be called anytime to free the zone information.The question may or may not have stopped.
    // If it was already stopped, mDNS_StopQuery_internal would have set q->ThisQInterval to -1 and should not
    // call it again
    if (nta->question.ThisQInterval != -1)
    {
        mDNS_StopQuery_internal(m, &nta->question);
        if (nta->question.ThisQInterval != -1)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "CancelGetZoneData: Question "PRI_DM_NAME" (" PUB_S ") ThisQInterval %d not -1",
                DM_NAME_PARAM(&nta->question.qname), DNSTypeName(nta->question.qtype), nta->question.ThisQInterval);
        }
    }
    mDNSPlatformMemFree(nta);
}

mDNSexport mStatus mDNS_StopQuery_internal(mDNS *const m, DNSQuestion *const question)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // Must reset the question's parameters to the original values if it is a DNSSEC question.
    dnssec_handle_question_stop(question);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_PUSH) && !defined(UNIT_TEST)
    // Do not stop DNS push activity if we are restarting this question.
    if (m->RestartQuestion != question && dns_question_uses_dns_push(question))
    {
        dns_push_handle_question_stop(m, question);
    }
#endif

    CacheGroup *cg = CacheGroupForName(m, question->qnamehash, &question->qname);
    CacheRecord *cr;
    DNSQuestion **qp = &m->Questions;
    const mDNSu32 request_id = question->request_id;
    const mDNSu16 question_id = mDNSVal16(question->TargetQID);

    //LogInfo("mDNS_StopQuery_internal %##s (%s)", question->qname.c, DNSTypeName(question->qtype));

    if (LocalOnlyOrP2PInterface(question->InterfaceID))
        qp = &m->LocalOnlyQuestions;
    while (*qp && *qp != question) qp=&(*qp)->next;
    if (*qp) *qp = (*qp)->next;
    else
    {
#if !ForceAlerts
        if (question->ThisQInterval >= 0)   // Only log error message if the query was supposed to be active
#endif
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT,
                "[R%u->Q%u] mDNS_StopQuery_internal: Question " PRI_DM_NAME " (" PUB_S ") not found in active list.",
                request_id, question_id, DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype));
        }
        return(mStatus_BadReferenceErr);
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    if (!LocalOnlyOrP2PInterface(question->InterfaceID) && mDNSOpaque16IsZero(question->TargetQID))
    {
        if (m->NumAllInterfaceRecords + m->NumAllInterfaceQuestions == 1)
            m->NextBonjourDisableTime = NonZeroTime(m->timenow + (BONJOUR_DISABLE_DELAY * mDNSPlatformOneSecond));
        m->NumAllInterfaceQuestions--;
    }
#endif
    if (question->request_id == 0)
    {
        // request_id == 0 indicates that the query is stopped by mDNSResponder itself, in which case no request
        // level log is available. Therefore, we need to print the query stop log message to indicate that
        // mDNSResponder has stopped a query internally.
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
            "[Q%u] mDNS_StopQuery_internal STOP -- name hash: %x",
            mDNSVal16(question->TargetQID), mDNS_DomainNameFNV1aHash(&question->qname));
    }

    // Take care to cut question from list *before* calling UpdateQuestionDuplicates
    UpdateQuestionDuplicates(m, question);
    // But don't trash ThisQInterval until afterwards.
    question->ThisQInterval = -1;

    // If there are any cache records referencing this as their active question, then see if there is any
    // other question that is also referencing them, else their CRActiveQuestion needs to get set to NULL.
    for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
    {
        if (cr->CRActiveQuestion == question)
        {
            DNSQuestion *q;
            DNSQuestion *replacement = mDNSNULL;
            // If we find an active question that is answered by this cached record, use it as the cache record's
            // CRActiveQuestion replacement. If there are no such questions, but there's at least one unsuppressed inactive
            // question that is answered by this cache record, then use an inactive one to not forgo generating RMV events
            // via CacheRecordRmv() when the cache record expires.
            for (q = m->Questions; q && (q != m->NewQuestions); q = q->next)
            {
                if (!q->DuplicateOf && !q->Suppressed && CacheRecordAnswersQuestion(cr, q))
                {
                    if (q->ThisQInterval > 0)
                    {
                        replacement = q;
                        break;
                    }
                    else if (!replacement)
                    {
                        replacement = q;
                    }
                }
            }
            if (replacement)
                debugf("mDNS_StopQuery_internal: Updating CRActiveQuestion to %p for cache record %s, Original question CurrentAnswers %d, new question "
                       "CurrentAnswers %d, Suppressed %d", replacement, CRDisplayString(m,cr), question->CurrentAnswers, replacement->CurrentAnswers, replacement->Suppressed);
            cr->CRActiveQuestion = replacement;    // Question used to be active; new value may or may not be null
            if (!replacement) m->rrcache_active--; // If no longer active, decrement rrcache_active count
        }
    }

    // If we just deleted the question that CacheRecordAdd() or CacheRecordRmv() is about to look at,
    // bump its pointer forward one question.
    if (m->CurrentQuestion == question)
    {
        debugf("mDNS_StopQuery_internal: Just deleted the currently active question: %##s (%s)",
               question->qname.c, DNSTypeName(question->qtype));
        m->CurrentQuestion = question->next;
    }

    if (m->NewQuestions == question)
    {
        debugf("mDNS_StopQuery_internal: Just deleted a new question that wasn't even answered yet: %##s (%s)",
               question->qname.c, DNSTypeName(question->qtype));
        m->NewQuestions = question->next;
    }

    if (m->NewLocalOnlyQuestions == question) m->NewLocalOnlyQuestions = question->next;

    if (m->RestartQuestion == question)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StopQuery_internal: Just deleted the current restart question: " PRI_DM_NAME " (" PUB_S ")",
            DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype));
        m->RestartQuestion = question->next;
    }

    // Take care not to trash question->next until *after* we've updated m->CurrentQuestion and m->NewQuestions
    question->next = mDNSNULL;

    // LogMsg("mDNS_StopQuery_internal: Question %##s (%s) removed", question->qname.c, DNSTypeName(question->qtype));

    // And finally, cancel any associated GetZoneData operation that's still running.
    // Must not do this until last, because there's a good chance the GetZoneData question is the next in the list,
    // so if we delete it earlier in this routine, we could find that our "question->next" pointer above is already
    // invalid before we even use it. By making sure that we update m->CurrentQuestion and m->NewQuestions if necessary
    // *first*, then they're all ready to be updated a second time if necessary when we cancel our GetZoneData query.
    if (question->tcp) { DisposeTCPConn(question->tcp); question->tcp = mDNSNULL; }
    if (question->LocalSocket) { mDNSPlatformUDPClose(question->LocalSocket); question->LocalSocket = mDNSNULL; }

    if (!mDNSOpaque16IsZero(question->TargetQID) && question->LongLived)
    {
        // Scan our list to see if any more wide-area LLQs remain. If not, stop our NAT Traversal.
        DNSQuestion *q;
        for (q = m->Questions; q; q=q->next)
            if (!mDNSOpaque16IsZero(q->TargetQID) && q->LongLived) break;
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        if (!q)
        {
            if (!m->LLQNAT.clientCallback)       // Should never happen, but just in case...
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StopQuery ERROR LLQNAT.clientCallback NULL");
            }
            else
            {
                LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "Stopping LLQNAT");
                mDNS_StopNATOperation_internal(m, &m->LLQNAT);
                m->LLQNAT.clientCallback = mDNSNULL; // Means LLQ NAT Traversal not running
            }
        }
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        // https://spc.apple.com/Guidelines.html#_100_16_2_consider_if_0_trick_for_ifelse_if
        if (0) {}
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        // If necessary, tell server it can delete this LLQ state
        else if (question->state == LLQ_Established)
        {
            question->ReqLease = 0;
            sendLLQRefresh(m, question);
            // If we need need to make a TCP connection to cancel the LLQ, that's going to take a little while.
            // We clear the tcp->question backpointer so that when the TCP connection completes, it doesn't
            // crash trying to access our cancelled question, but we don't cancel the TCP operation itself --
            // we let that run out its natural course and complete asynchronously.
            if (question->tcp)
            {
                question->tcp->question = mDNSNULL;
                question->tcp           = mDNSNULL;
            }
        }
#endif

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH)
        else if (question->dnsPushServer != mDNSNULL)
        {
            // UnsubscribeQuestionFromDNSPushServer() must happen before Querier_HandleStoppedDNSQuestion(), because it
            // checks the dnsservice to determine if the cached response answers the current long-lived query.
            UnsubscribeQuestionFromDNSPushServer(m, question, mDNSfalse);
        }
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH)
    }
    // wait until we send the refresh above which needs the nta
    if (question->nta) { CancelGetZoneData(m, question->nta); question->nta = mDNSNULL; }

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    Querier_HandleStoppedDNSQuestion(question);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
    DPCHandleStoppedDNSQuestion(question);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS_ANALYTICS) || MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
    DNSMetricsClear(&question->metrics);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
    DNS64ResetState(question);
#endif

#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
    const domainname *domain_to_discover_resolver = mDNSNULL;
    // Only stop the resolver discover if the question has required the discovery when it is started.
    if (dns_question_requires_resolver_discovery(question, &domain_to_discover_resolver))
    {
        // Inside mDNS_StopQuery_internal(), we already grabbed the lock, so no need to lock it again.
        resolver_discovery_remove(domain_to_discover_resolver, mDNSfalse);
    }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
    if (DNSQuestionNeedsSensitiveLogging(question))
    {
         gNumOfSensitiveLoggingEnabledQuestions--;

        if (gNumOfSensitiveLoggingEnabledQuestions == 0)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[Q%u] Last question that enables sensitive logging is stopped.", mDNSVal16(question->TargetQID));
            gSensitiveLoggingEnabled = mDNSfalse;
        }
        else
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "[Q%u] number of sensitive logging enabled questions: "
                "%u.", mDNSVal16(question->TargetQID), gNumOfSensitiveLoggingEnabledQuestions);
        }
    }
#endif // MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // Must drop the lock before releasing DNSSEC context because its finalizer may stop other questions, which would
    // grab the lock again.
    // For example:
    // Stopping www.ietf.org. AAAA will stop ietf.org. DNSKEY
    // Stopping ietf.org. DNSKEY will stop ietf.org. DS
    // ...until the root trust anchor is reached.
    mDNS_DropLockBeforeCallback();
    MDNS_DISPOSE_DNSSEC_OBJ(question->dnssec);
    mDNS_ReclaimLockAfterCallback();
#endif
    if (question->DupSuppress)
    {
        mDNSPlatformMemFree(question->DupSuppress);
        question->DupSuppress = mDNSNULL;
    }
    return(mStatus_NoError);
}

mDNSexport mStatus mDNS_StartQuery(mDNS *const m, DNSQuestion *const question)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_StartQuery_internal(m, question);
    mDNS_Unlock(m);
    return(status);
}

mDNSexport mStatus mDNS_StopQuery(mDNS *const m, DNSQuestion *const question)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_StopQuery_internal(m, question);
    mDNS_Unlock(m);
    return(status);
}

mDNSlocal mStatus mDNS_CheckQuestionBeforeStoppingQuery(const mDNS *const m, const DNSQuestion *const question)
{
    mStatus err = mStatus_NoError;

    DNSQuestion *const *qp = LocalOnlyOrP2PInterface(question->InterfaceID) ? &m->LocalOnlyQuestions : &m->Questions;

    // Check if the current question is in mDNSResponder's question list.
    for (; *qp != mDNSNULL && *qp != question; qp = &(*qp)->next)
        ;
    if (*qp == mDNSNULL)
    {
    #if !ForceAlerts
        if (question->ThisQInterval >= 0)
    #endif
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, "Question not found in the active list - "
                "qname: " PRI_DM_NAME ", qtype: " PUB_S ".", DM_NAME_PARAM(&question->qname),
                DNSTypeName(question->qtype));
        }
        err = mStatus_BadReferenceErr;
    }

    return err;
}

// Note that mDNS_StopQueryWithRemoves() does not currently implement the full generality of the other APIs
// Specifically, question callbacks invoked as a result of this call cannot themselves make API calls.
// We invoke the callback without using mDNS_DropLockBeforeCallback/mDNS_ReclaimLockAfterCallback
// specifically to catch and report if the client callback does try to make API calls
mDNSexport mStatus mDNS_StopQueryWithRemoves(mDNS *const m, DNSQuestion *const question)
{
    DNSQuestion *qq;
    mDNS_Lock(m);

    // Check if question is new -- don't want to give remove events for a question we haven't even answered yet
    for (qq = m->NewQuestions; qq; qq=qq->next) if (qq == question) break;

    // Ensure that the question is still in the mDNSResponder's question list, or there is no point to deliver RMV
    // event.
    const mStatus err = mDNS_CheckQuestionBeforeStoppingQuery(m, question);
    if (err != mStatus_NoError)
    {
        goto exit;
    }

    if (qq == mDNSNULL) // The question being stopped is not a new question.
    {
        const CacheRecord *cr;
        CacheGroup *const cg = CacheGroupForName(m, question->qnamehash, &question->qname);
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
            "[Q%u] Generating RMV events because the question will be stopped - "
            "qname: " PRI_DM_NAME ", qtype: " PUB_S ".", mDNSVal16(question->TargetQID),
            DM_NAME_PARAM(&question->qname), DNSTypeName(question->qtype));
        for (cr = cg ? cg->members : mDNSNULL; cr; cr=cr->next)
        {
            if (cr->resrec.RecordType != kDNSRecordTypePacketNegative && SameNameCacheRecordAnswersQuestion(cr, question))
            {
                // Don't use mDNS_DropLockBeforeCallback() here, since we don't allow API calls
                if (question->QuestionCallback)
                {
                    question->QuestionCallback(m, question, &cr->resrec, QC_rmv);
                }
            }
        }
    }

    mDNS_StopQuery_internal(m, question);

exit:
    mDNS_Unlock(m);
    return err;
}

mDNSexport mStatus mDNS_Reconfirm(mDNS *const m, CacheRecord *const cr)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
    if (status == mStatus_NoError) ReconfirmAntecedents(m, cr->resrec.name, cr->resrec.namehash, cr->resrec.InterfaceID, 0);
    mDNS_Unlock(m);
    return(status);
}

mDNSexport mStatus mDNS_ReconfirmByValue(mDNS *const m, ResourceRecord *const rr)
{
    mStatus status = mStatus_BadReferenceErr;
    CacheRecord *cr;
    mDNS_Lock(m);
    cr = FindIdenticalRecordInCache(m, rr);
    debugf("mDNS_ReconfirmByValue: %p %s", cr, RRDisplayString(m, rr));
    if (cr) status = mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
    if (status == mStatus_NoError) ReconfirmAntecedents(m, cr->resrec.name, cr->resrec.namehash, cr->resrec.InterfaceID, 0);
    mDNS_Unlock(m);
    return(status);
}

mDNSlocal mStatus mDNS_StartBrowse_internal(mDNS *const m, DNSQuestion *const question,
                                            const domainname *const srv, const domainname *const domain,
                                            const mDNSInterfaceID InterfaceID, mDNSu32 flags,
                                            mDNSBool ForceMCast, mDNSBool useBackgroundTrafficClass,
                                            mDNSQuestionCallback *Callback, void *Context)
{
    question->InterfaceID      = InterfaceID;
    question->flags            = flags;
    question->qtype            = kDNSType_PTR;
    question->qclass           = kDNSClass_IN;
    question->LongLived        = mDNStrue;
    question->ExpectUnique     = mDNSfalse;
    question->ForceMCast       = ForceMCast;
    question->ReturnIntermed   = (flags & kDNSServiceFlagsReturnIntermediates) != 0;
    question->SuppressUnusable = mDNSfalse;
    question->AppendSearchDomains = mDNSfalse;
    question->TimeoutQuestion  = 0;
    question->WakeOnResolve    = 0;
    question->UseBackgroundTraffic = useBackgroundTrafficClass;
    question->ProxyQuestion    = 0;
    question->QuestionCallback = Callback;
    question->QuestionContext  = Context;

    if (!ConstructServiceName(&question->qname, mDNSNULL, srv, domain))
        return(mStatus_BadParamErr);

    if (question->request_id != 0)
    {
        // This is the first place where we know the full name of a browsing query.
        // After: [R6] DNSServiceBrowse -> SubBrowser START -- qname: _companion-link._tcp.local.(334e2060)
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
            "[R%u] DNSServiceBrowse -> SubBrowser START -- qname: " PRI_DM_NAME " (%x)",
            question->request_id, DM_NAME_PARAM(&question->qname), mDNS_DomainNameFNV1aHash(&question->qname));
    }

    return(mDNS_StartQuery_internal(m, question));
}

mDNSexport mStatus mDNS_StartBrowse(mDNS *const m, DNSQuestion *const question,
                                    const domainname *const srv, const domainname *const domain,
                                    const mDNSInterfaceID InterfaceID, mDNSu32 flags,
                                    mDNSBool ForceMCast, mDNSBool useBackgroundTrafficClass,
                                    mDNSQuestionCallback *Callback, void *Context)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_StartBrowse_internal(m, question, srv, domain, InterfaceID, flags, ForceMCast, useBackgroundTrafficClass, Callback, Context);
    mDNS_Unlock(m);
    return(status);
}

mDNSlocal mStatus mDNS_GetDomains_Internal(mDNS *const m, DNSQuestion *const question, const mDNS_DomainType DomainType,
    const domainname * domain, const mDNSInterfaceID InterfaceID, mDNSQuestionCallback *const Callback,
    void *const Context)
{
    question->InterfaceID      = InterfaceID;
    question->flags            = 0;
    question->qtype            = kDNSType_PTR;
    question->qclass           = kDNSClass_IN;
    question->LongLived        = mDNSfalse;
    question->ExpectUnique     = mDNSfalse;
    question->ForceMCast       = mDNSfalse;
    question->ReturnIntermed   = mDNSfalse;
    question->SuppressUnusable = mDNSfalse;
    question->AppendSearchDomains = mDNSfalse;
    question->TimeoutQuestion  = 0;
    question->WakeOnResolve    = 0;
    question->UseBackgroundTraffic = mDNSfalse;
    question->ProxyQuestion    = 0;
    question->pid              = mDNSPlatformGetPID();
    question->euid             = 0;
    question->QuestionCallback = Callback;
    question->QuestionContext  = Context;
    if (DomainType > mDNS_DomainTypeMax) return(mStatus_BadParamErr);
    if (!MakeDomainNameFromDNSNameString(&question->qname, mDNS_DomainTypeNames[DomainType])) return(mStatus_BadParamErr);
    if (!domain) domain = &localdomain;
    if (!AppendDomainName(&question->qname, domain)) return(mStatus_BadParamErr);
    return(mDNS_StartQuery_internal(m, question));
}

mDNSexport mStatus mDNS_GetDomains(mDNS *const m, DNSQuestion *const question, const mDNS_DomainType DomainType,
    const domainname *const domain, const mDNSInterfaceID InterfaceID, mDNSQuestionCallback *const Callback,
    void *const Context)
{
    mDNS_Lock(m);
    const mStatus err = mDNS_GetDomains_Internal(m, question, DomainType, domain, InterfaceID, Callback, Context);
    mDNS_Unlock(m);

    return err;
}

static const char *const mDNS_DomainType_Description[] =
{
    "browse domain",                // Use index mDNS_DomainTypeBrowse to get.
    "default browse domain",        // Use index mDNS_DomainTypeBrowseDefault to get.
    "automatic browse domain",      // Use index mDNS_DomainTypeBrowseAutomatic to get.
    "registration domain",          // Use index mDNS_DomainTypeRegistration to get.
    "default registration domain",  // Use index mDNS_DomainTypeRegistrationDefault to get.
};
#ifdef check_compile_time
    // The index count must match the array size above.
    check_compile_time(countof(mDNS_DomainType_Description) == mDNS_DomainTypeMaxCount);
#endif

mDNSlocal void mDNS_DeregisterDomainsDiscoveredForDomainEnumeration(mDNS *m, DomainEnumerationOp *op, mDNSu32 type);

mDNSlocal mStatus mDNS_SetUpDomainEnumeration(mDNS *const m, DomainEnumerationOp *const op,
    const mDNS_DomainType type)
{
    mStatus err;

    // Currently, we only support doing domain enumeration looking for automatic browse domain.
    if (type != mDNS_DomainTypeBrowseAutomatic)
    {
        err = mStatus_UnsupportedErr;
        goto exit;
    }

    DomainEnumerationWithType *const enumeration = op->enumerations[type];
    if (enumeration == mDNSNULL)
    {
        // No operations have been done for this domain enumeration type.
        err = mStatus_NoError;
        goto exit;
    }

    switch (enumeration->state)
    {
        case DomainEnumerationState_Stopped:
            if (enumeration->activeClientCount == 1)
            {
                // If there is a client who needs to do domain enumeration, start it.
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Starting the " PUB_S " enumeration "
                    "- domain: " PRI_DM_NAME ".", mDNS_DomainType_Description[type], DM_NAME_PARAM(&op->name));

                // Avoid the build failure for the target mDNSNetMonitor and non-Apple builds.
            #if (!defined(NET_MONITOR) || !NET_MONITOR) && defined(__APPLE__) && !defined(POSIX_BUILD)
                err = mDNS_GetDomains_Internal(m, &enumeration->question, type, &op->name, mDNSInterface_Any,
                    FoundNonLocalOnlyAutomaticBrowseDomain, mDNSNULL);
            #else
                err = mStatus_UnsupportedErr;
            #endif
                if (err != mStatus_NoError)
                {
                    goto exit;
                }

                enumeration->state = DomainEnumerationState_Started;
            }
            // If there are more than one clients need to do enumeration, the process has begun, no need to start again.
            break;

        case DomainEnumerationState_Started:
            if (enumeration->activeClientCount == 0)
            {
                // If the number of client that need domain enumeration becomes 0, get ready to stop it.
                // However, we give it a grace period so that the enumeration will not be stopped immediately, to
                // avoid the case where a single client starts and stops request too frequently.
                const mDNSs32 gracePeriodInSeconds = 60;

                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG, "Planning to stop the " PUB_S " enumeration "
                    "- domain: " PRI_DM_NAME ", grace period: %ds.", mDNS_DomainType_Description[type],
                    DM_NAME_PARAM(&op->name), gracePeriodInSeconds);

                enumeration->state = DomainEnumerationState_StopInProgress;

                // Ensure that we will check the stop time when the grace period has passed.
                const mDNSs32 gracePeriodPlatformTime = gracePeriodInSeconds * mDNSPlatformOneSecond;
                enumeration->nextStopTime = NonZeroTime(m->timenow + gracePeriodPlatformTime);
            }
            break;

        case DomainEnumerationState_StopInProgress:
            if (enumeration->activeClientCount == 0 && m->timenow - enumeration->nextStopTime >= 0)
            {
                // If there is no active client that needs the domain enumeration and we have passed the scheduled
                // stop time, stop the enumeration immediately.
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG, "Stopping the " PUB_S " enumeration "
                    "- domain: " PRI_DM_NAME ".", mDNS_DomainType_Description[type], DM_NAME_PARAM(&op->name));

                err = mDNS_StopGetDomains_Internal(m, &enumeration->question);
                if (err != mStatus_NoError)
                {
                    goto exit;
                }

                mDNS_DeregisterDomainsDiscoveredForDomainEnumeration(m, op, type);

                enumeration->state = DomainEnumerationState_Stopped;
            }
            else if (enumeration->activeClientCount == 1)
            {
                // If there is new client that needs the domain enumeration, terminate the stopping process and go back
                // to the active mode.
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Resuming the " PUB_S " enumeration "
                    "- active client count: %u, domain: " PRI_DM_NAME ".", mDNS_DomainType_Description[type],
                        enumeration->activeClientCount, DM_NAME_PARAM(&op->name));

                // Reschedule the next event since our stopping process is terminated.
                enumeration->state = DomainEnumerationState_Started;
            }
            break;

        MDNS_COVERED_SWITCH_DEFAULT:
            break;
    }

    err = mStatus_NoError;
exit:
    return err;
}

mDNSexport mStatus mDNS_StartDomainEnumeration(mDNS *const m, const domainname *const domain, const mDNS_DomainType type)
{
    mStatus err;
    mDNS_Lock(m);

    DomainEnumerationOp *op = mDNSNULL;

    for (op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
    {
        if (SameDomainName(&op->name, domain))
        {
            break;
        }
    }

    if (op == mDNSNULL)
    {
        err = mStatus_BadParamErr;
        goto exit;
    }

    if (op->enumerations[type] == mDNSNULL)
    {
        op->enumerations[type] = mDNSPlatformMemAllocateClear(sizeof(*op->enumerations[type]));
        if (op->enumerations[type] == mDNSNULL)
        {
            err = mStatus_NoMemoryErr;
            goto exit;
        }
    }

    op->enumerations[type]->activeClientCount++;

    err = mDNS_SetUpDomainEnumeration(m, op, type);

exit:
    mDNS_Unlock(m);
    return err;
}

mDNSexport mStatus mDNS_StopDomainEnumeration(mDNS *const m, const domainname *const domain, const mDNS_DomainType type)
{
    mStatus err;
    mDNS_Lock(m);

    DomainEnumerationOp *op = mDNSNULL;

    for (op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
    {
        if (SameDomainName(&op->name, domain))
        {
            break;
        }
    }

    if (op == mDNSNULL || op->enumerations[type] == mDNSNULL)
    {
        err = mStatus_BadParamErr;
        goto exit;
    }

    if ( op->enumerations[type]->activeClientCount == 0)
    {
        err = mStatus_Invalid;
        goto exit;
    }

    op->enumerations[type]->activeClientCount--;

    err = mDNS_SetUpDomainEnumeration(m, op, type);

exit:
    mDNS_Unlock(m);
    return err;
}

mDNSexport mStatus mDNS_AddDomainDiscoveredForDomainEnumeration(mDNS *const m, const domainname *const domain,
    const mDNS_DomainType type, const domainname *const domainDiscovered)
{
    mStatus err;
    mDNS_Lock(m);

    DomainEnumerationOp *op = mDNSNULL;

    for (op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
    {
        if (SameDomainName(&op->name, domain))
        {
            break;
        }
    }

    if (op == mDNSNULL || op->enumerations[type] == mDNSNULL)
    {
        err = mStatus_BadParamErr;
        goto exit;
    }

    // Remember the domain name we discovered so that when the domain enumeration is stopped when we can remove them
    // from the auth record set.

    EnumeratedDomainList *const domainDiscoveredItem = mDNSPlatformMemAllocateClear(sizeof(*domainDiscoveredItem));
    if (domainDiscoveredItem == mDNSNULL)
    {
        err = mStatus_NoMemoryErr;
        goto exit;
    }

    AssignDomainName(&domainDiscoveredItem->name, domainDiscovered);

    domainDiscoveredItem->next = op->enumerations[type]->domainList;
    op->enumerations[type]->domainList = domainDiscoveredItem;
    err = mStatus_NoError;

exit:
    mDNS_Unlock(m);
    return err;
}

mDNSexport mStatus mDNS_RemoveDomainDiscoveredForDomainEnumeration(mDNS *const m, const domainname *const domain,
    const mDNS_DomainType type, const domainname *const domainDiscovered)
{
    mStatus err;
    mDNS_Lock(m);

    DomainEnumerationOp *op = mDNSNULL;

    for (op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
    {
        if (SameDomainName(&op->name, domain))
        {
            break;
        }
    }

    if (op == mDNSNULL || op->enumerations[type] == mDNSNULL)
    {
        err = mStatus_BadParamErr;
        goto exit;
    }

    EnumeratedDomainList *node;
    EnumeratedDomainList **ptr = &op->enumerations[type]->domainList;
    while ((node = *ptr) != mDNSNULL)
    {
        if (SameDomainName(&node->name, domainDiscovered))
        {
            break;
        }
        ptr = &node->next;
    }
    if (node != mDNSNULL)
    {
        *ptr = node->next;
        mDNSPlatformMemFree(node);
    }

    err = mStatus_NoError;

exit:
    mDNS_Unlock(m);
    return err;
}

mDNSlocal void mDNS_DeregisterDomainsDiscoveredForDomainEnumeration(mDNS *const m, DomainEnumerationOp *const op,
                                                                    const mDNSu32 type)
{
    // Given all the domain we have discovered through domain enumeration, deregister them from our cache.

    if (op->enumerations[type] == mDNSNULL)
    {
        return;
    }

    for (EnumeratedDomainList *node = op->enumerations[type]->domainList, *next = mDNSNULL; node != mDNSNULL; node = next)
    {
        next = node->next;

        // Avoid the build failure for the target mDNSNetMonitor and non-Apple builds.
    #if (!defined(NET_MONITOR) || !NET_MONITOR) && defined(__APPLE__) && !defined(POSIX_BUILD)
        DeregisterLocalOnlyDomainEnumPTR_Internal(m, &node->name, type, mDNStrue);
    #else
        (void)m;
    #endif
        mDNSPlatformMemFree(node);
    }
    op->enumerations[type]->domainList = mDNSNULL;
}

// ***************************************************************************
// MARK: - Responder Functions

mDNSexport mStatus mDNS_Register(mDNS *const m, AuthRecord *const rr)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_Register_internal(m, rr);
    mDNS_Unlock(m);
    return(status);
}

mDNSexport mStatus mDNS_Update(mDNS *const m, AuthRecord *const rr, mDNSu32 newttl,
                               const mDNSu16 newrdlength, RData *const newrdata, mDNSRecordUpdateCallback *Callback)
{
    if (!ValidateRData(rr->resrec.rrtype, newrdlength, newrdata))
    {
        LogMsg("Attempt to update record with invalid rdata: %s", GetRRDisplayString_rdb(&rr->resrec, &newrdata->u, m->MsgBuffer));
        return(mStatus_Invalid);
    }

    mDNS_Lock(m);

    // If TTL is unspecified, leave TTL unchanged
    if (newttl == 0) newttl = rr->resrec.rroriginalttl;

    // If we already have an update queued up which has not gone through yet, give the client a chance to free that memory
    if (rr->NewRData)
    {
        RData *n = rr->NewRData;
        rr->NewRData = mDNSNULL;                            // Clear the NewRData pointer ...
        if (rr->UpdateCallback)
            rr->UpdateCallback(m, rr, n, rr->newrdlength);  // ...and let the client free this memory, if necessary
    }

    rr->NewRData             = newrdata;
    rr->newrdlength          = newrdlength;
    rr->UpdateCallback       = Callback;

#ifndef UNICAST_DISABLED
    if (rr->ARType != AuthRecordLocalOnly && rr->ARType != AuthRecordP2P && AuthRecord_uDNS(rr))
    {
        mStatus status = uDNS_UpdateRecord(m, rr);
        // The caller frees the memory on error, don't retain stale pointers
        if (status != mStatus_NoError) { rr->NewRData = mDNSNULL; rr->newrdlength = 0; }
        mDNS_Unlock(m);
        return(status);
    }
#endif

    if (RRLocalOnly(rr) || (rr->resrec.rroriginalttl == newttl && rr->resrec.rdlength == newrdlength &&
                            mDNSPlatformMemSame(rr->resrec.rdata->u.data, newrdata->u.data, newrdlength)))
        CompleteRDataUpdate(m, rr);
    else
    {
        rr->AnnounceCount = InitialAnnounceCount;
        InitializeLastAPTime(m, rr);
        while (rr->NextUpdateCredit && m->timenow - rr->NextUpdateCredit >= 0) GrantUpdateCredit(rr);
        if (!rr->UpdateBlocked && rr->UpdateCredits) rr->UpdateCredits--;
        if (!rr->NextUpdateCredit) rr->NextUpdateCredit = NonZeroTime(m->timenow + kUpdateCreditRefreshInterval);
        if (rr->AnnounceCount > rr->UpdateCredits + 1) rr->AnnounceCount = (mDNSu8)(rr->UpdateCredits + 1);
        if (rr->UpdateCredits <= 5)
        {
            mDNSu32 delay = 6 - rr->UpdateCredits;      // Delay 1 second, then 2, then 3, etc. up to 6 seconds maximum
            if (!rr->UpdateBlocked) rr->UpdateBlocked = NonZeroTime(m->timenow + (mDNSs32)delay * mDNSPlatformOneSecond);
            rr->ThisAPInterval *= 4;
            rr->LastAPTime = rr->UpdateBlocked - rr->ThisAPInterval;
            LogMsg("Excessive update rate for %##s; delaying announcement by %ld second%s",
                   rr->resrec.name->c, delay, delay > 1 ? "s" : "");
        }
        rr->resrec.rroriginalttl = newttl;
    }

    mDNS_Unlock(m);
    return(mStatus_NoError);
}

// Note: mDNS_Deregister calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport mStatus mDNS_Deregister(mDNS *const m, AuthRecord *const rr)
{
    mStatus status;
    mDNS_Lock(m);
    status = mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
    mDNS_Unlock(m);
    return(status);
}

// Circular reference: AdvertiseInterface references mDNS_HostNameCallback, which calls mDNS_SetFQDN, which call AdvertiseInterface
mDNSlocal void mDNS_HostNameCallback(mDNS *const m, AuthRecord *const rr, mStatus result);
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
mDNSlocal void mDNS_RandomizedHostNameCallback(mDNS *m, AuthRecord *rr, mStatus result);
#endif

mDNSlocal AuthRecord *GetInterfaceAddressRecord(NetworkInterfaceInfo *intf, mDNSBool forRandHostname)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
        return(forRandHostname ? &intf->RR_AddrRand : &intf->RR_A);
#else
        (void)forRandHostname; // Unused.
        return(&intf->RR_A);
#endif
}

mDNSlocal AuthRecord *GetFirstAddressRecordEx(const mDNS *const m, const mDNSBool forRandHostname)
{
    NetworkInterfaceInfo *intf;
    for (intf = m->HostInterfaces; intf; intf = intf->next)
    {
        if (!intf->Advertise) continue;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
        if (mDNSPlatformInterfaceIsAWDL(intf->InterfaceID)) continue;
#endif
        return(GetInterfaceAddressRecord(intf, forRandHostname));
    }
    return(mDNSNULL);
}
#if !MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
#define GetFirstAddressRecord(M)    GetFirstAddressRecordEx(M, mDNSfalse)
#endif

// The parameter "set" here refers to the set of AuthRecords used to advertise this interface.
// (It's a set of records, not a set of interfaces.)
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
mDNSlocal void AdvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set, mDNSBool useRandomizedHostname)
#else
mDNSlocal void AdvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set)
#endif
{
    const domainname *hostname;
    mDNSRecordCallback *hostnameCallback;
    AuthRecord *addrAR;
    AuthRecord *ptrAR;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    const mDNSBool interfaceIsAWDL = mDNSPlatformInterfaceIsAWDL(set->InterfaceID);
#endif
    mDNSu8 addrRecordType;
    char buffer[MAX_REVERSE_MAPPING_NAME];

#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (interfaceIsAWDL || useRandomizedHostname)
    {
        hostname         = &m->RandomizedHostname;
        hostnameCallback = mDNS_RandomizedHostNameCallback;
    }
    else
#endif
    {
        hostname         = &m->MulticastHostname;
        hostnameCallback = mDNS_HostNameCallback;
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (!interfaceIsAWDL && useRandomizedHostname)
    {
        addrAR = &set->RR_AddrRand;
        ptrAR  = mDNSNULL;
    }
    else
#endif
    {
        addrAR = &set->RR_A;
        ptrAR  = &set->RR_PTR;
    }
    if (addrAR->resrec.RecordType != kDNSRecordTypeUnregistered) return;

    addrRecordType = set->DirectLink ? kDNSRecordTypeKnownUnique : kDNSRecordTypeUnique;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (hostname == &m->RandomizedHostname) addrRecordType = kDNSRecordTypeKnownUnique;
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "AdvertiseInterface: Advertising " PUB_S " hostname on interface " PUB_S,
        (hostname == &m->RandomizedHostname) ? "randomized" : "normal", set->ifname);
#else
    LogInfo("AdvertiseInterface: Advertising for ifname %s", set->ifname);
#endif

    // Send dynamic update for non-linklocal IPv4 Addresses
    mDNS_SetupResourceRecord(addrAR, mDNSNULL, set->InterfaceID, kDNSType_A, kHostNameTTL, addrRecordType, AuthRecordAny, hostnameCallback, set);
    if (ptrAR) mDNS_SetupResourceRecord(ptrAR, mDNSNULL, set->InterfaceID, kDNSType_PTR, kHostNameTTL, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);

#if ANSWER_REMOTE_HOSTNAME_QUERIES
    addrAR->AllowRemoteQuery = mDNStrue;
    if (ptrAR) ptrAR->AllowRemoteQuery = mDNStrue;
#endif
    // 1. Set up Address record to map from host name ("foo.local.") to IP address
    // 2. Set up reverse-lookup PTR record to map from our address back to our host name
    AssignDomainName(&addrAR->namestorage, hostname);
    if (set->ip.type == mDNSAddrType_IPv4)
    {
        addrAR->resrec.rrtype        = kDNSType_A;
        addrAR->resrec.rdata->u.ipv4 = set->ip.ip.v4;
        // Note: This is reverse order compared to a normal dotted-decimal IP address, so we can't use our customary "%.4a" format code
        mDNS_snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d.in-addr.arpa.",
                      set->ip.ip.v4.b[3], set->ip.ip.v4.b[2], set->ip.ip.v4.b[1], set->ip.ip.v4.b[0]);
    }
    else if (set->ip.type == mDNSAddrType_IPv6)
    {
        int i;
        addrAR->resrec.rrtype        = kDNSType_AAAA;
        addrAR->resrec.rdata->u.ipv6 = set->ip.ip.v6;
        for (i = 0; i < 16; i++)
        {
            static const char hexValues[] = "0123456789ABCDEF";
            buffer[i * 4    ] = hexValues[set->ip.ip.v6.b[15 - i] & 0x0F];
            buffer[i * 4 + 1] = '.';
            buffer[i * 4 + 2] = hexValues[set->ip.ip.v6.b[15 - i] >> 4];
            buffer[i * 4 + 3] = '.';
        }
        mDNS_snprintf(&buffer[64], sizeof(buffer)-64, "ip6.arpa.");
    }

    if (ptrAR)
    {
        MakeDomainNameFromDNSNameString(&ptrAR->namestorage, buffer);
        ptrAR->AutoTarget = Target_AutoHost;    // Tell mDNS that the target of this PTR is to be kept in sync with our host name
        ptrAR->ForceMCast = mDNStrue;           // This PTR points to our dot-local name, so don't ever try to write it into a uDNS server
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    addrAR->RRSet = (uintptr_t)(interfaceIsAWDL ? addrAR : GetFirstAddressRecordEx(m, useRandomizedHostname));
#else
    addrAR->RRSet = (uintptr_t)(void *)GetFirstAddressRecord(m);
#endif
    if (!addrAR->RRSet) addrAR->RRSet = (uintptr_t)addrAR;
    mDNS_Register_internal(m, addrAR);
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "Initialized RRSet for " PRI_S, ARDisplayString(m, addrAR));
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "RRSet:                %" PRIxPTR "", addrAR->RRSet);
    if (ptrAR) mDNS_Register_internal(m, ptrAR);

#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
    // must be after the mDNS_Register_internal() calls so that records have complete rdata fields, etc
    D2D_start_advertising_interface(set);
#endif
}

mDNSlocal void AdvertiseInterfaceIfNeeded(mDNS *const m, NetworkInterfaceInfo *set)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (mDNSPlatformInterfaceIsAWDL(set->InterfaceID))
    {
        if ((m->AutoTargetAWDLIncludedCount > 0) || (m->AutoTargetAWDLOnlyCount > 0))
        {
            AdvertiseInterface(m, set, mDNSfalse);
        }
    }
    else
    {
        if (m->AutoTargetServices          > 0) AdvertiseInterface(m, set, mDNSfalse);
        if (m->AutoTargetAWDLIncludedCount > 0) AdvertiseInterface(m, set, mDNStrue);
    }
#else
    if (m->AutoTargetServices > 0) AdvertiseInterface(m, set);
#endif
}

mDNSlocal void DeadvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set, DeadvertiseFlags flags)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    const mDNSBool interfaceIsAWDL = mDNSPlatformInterfaceIsAWDL(set->InterfaceID);
#endif

    // Unregister these records.
    // When doing the mDNS_Exit processing, we first call DeadvertiseInterface for each interface, so by the time the platform
    // support layer gets to call mDNS_DeregisterInterface, the address and PTR records have already been deregistered for it.
    // Also, in the event of a name conflict, one or more of our records will have been forcibly deregistered.
    // To avoid unnecessary and misleading warning messages, we check the RecordType before calling mDNS_Deregister_internal().
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if ((!interfaceIsAWDL && (flags & kDeadvertiseFlag_NormalHostname)) ||
        ( interfaceIsAWDL && (flags & kDeadvertiseFlag_RandHostname)))
#else
    if (flags & kDeadvertiseFlag_NormalHostname)
#endif
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "DeadvertiseInterface: Deadvertising " PUB_S " hostname on interface " PUB_S,
            (flags & kDeadvertiseFlag_RandHostname) ? "randomized" : "normal", set->ifname);
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
        D2D_stop_advertising_interface(set);
#endif
        if (set->RR_A.resrec.RecordType)   mDNS_Deregister_internal(m, &set->RR_A,   mDNS_Dereg_normal);
        if (set->RR_PTR.resrec.RecordType) mDNS_Deregister_internal(m, &set->RR_PTR, mDNS_Dereg_normal);
    }
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (!interfaceIsAWDL && (flags & kDeadvertiseFlag_RandHostname))
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "DeadvertiseInterface: Deadvertising randomized hostname on interface " PUB_S, set->ifname);
        AuthRecord *const ar = &set->RR_AddrRand;
        if (ar->resrec.RecordType) mDNS_Deregister_internal(m, ar, mDNS_Dereg_normal);
    }
#endif
}

// Change target host name for record.
mDNSlocal void UpdateTargetHostName(mDNS *const m, AuthRecord *const rr)
{
#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
    // If this record was also registered with any D2D plugins, stop advertising
    // the version with the old host name.
    D2D_stop_advertising_record(rr);
#endif

    SetTargetToHostName(m, rr);

#if MDNSRESPONDER_SUPPORTS(APPLE, D2D)
    // Advertise the record with the updated host name with the D2D plugins if appropriate.
    D2D_start_advertising_record(rr);
#endif
}

mDNSlocal void DeadvertiseAllInterfaceRecords(mDNS *const m, DeadvertiseFlags flags)
{
    NetworkInterfaceInfo *intf;
    for (intf = m->HostInterfaces; intf; intf = intf->next)
    {
        if (intf->Advertise) DeadvertiseInterface(m, intf, flags);
    }
}

mDNSexport void mDNS_SetFQDN(mDNS *const m)
{
    domainname newmname;
    AuthRecord *rr;
    newmname.c[0] = 0;

    if (!AppendDomainLabel(&newmname, &m->hostlabel))  { LogMsg("ERROR: mDNS_SetFQDN: Cannot create MulticastHostname"); return; }
    if (!AppendLiteralLabelString(&newmname, "local")) { LogMsg("ERROR: mDNS_SetFQDN: Cannot create MulticastHostname"); return; }

    mDNS_Lock(m);

    if (SameDomainNameCS(&m->MulticastHostname, &newmname)) debugf("mDNS_SetFQDN - hostname unchanged");
    else
    {
        AssignDomainName(&m->MulticastHostname, &newmname);
        DeadvertiseAllInterfaceRecords(m, kDeadvertiseFlag_NormalHostname);
        AdvertiseNecessaryInterfaceRecords(m);
    }

    // 3. Make sure that any AutoTarget SRV records (and the like) get updated
    for (rr = m->ResourceRecords;  rr; rr=rr->next) if (rr->AutoTarget) UpdateTargetHostName(m, rr);
    for (rr = m->DuplicateRecords; rr; rr=rr->next) if (rr->AutoTarget) UpdateTargetHostName(m, rr);

    mDNS_Unlock(m);
}

mDNSlocal void mDNS_HostNameCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
    (void)rr;   // Unused parameter

    #if MDNS_DEBUGMSGS
    {
        char *msg = "Unknown result";
        if      (result == mStatus_NoError) msg = "Name registered";
        else if (result == mStatus_NameConflict) msg = "Name conflict";
        debugf("mDNS_HostNameCallback: %##s (%s) %s (%ld)", rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype), msg, result);
    }
    #endif

    if (result == mStatus_NoError)
    {
        // Notify the client that the host name is successfully registered
        if (m->MainCallback)
            m->MainCallback(m, mStatus_NoError);
    }
    else if (result == mStatus_NameConflict)
    {
        domainlabel oldlabel = m->hostlabel;

        // 1. First give the client callback a chance to pick a new name
        if (m->MainCallback)
            m->MainCallback(m, mStatus_NameConflict);

        // 2. If the client callback didn't do it, add (or increment) an index ourselves
        // This needs to be case-INSENSITIVE compare, because we need to know that the name has been changed so as to
        // remedy the conflict, and a name that differs only in capitalization will just suffer the exact same conflict again.
        if (SameDomainLabel(m->hostlabel.c, oldlabel.c))
            IncrementLabelSuffix(&m->hostlabel, mDNSfalse);

        // 3. Generate the FQDNs from the hostlabel,
        // and make sure all SRV records, etc., are updated to reference our new hostname
        mDNS_SetFQDN(m);
        LogMsg("Local Hostname %#s.local already in use; will try %#s.local instead", oldlabel.c, m->hostlabel.c);
    }
    else if (result == mStatus_MemFree)
    {
        // .local hostnames do not require goodbyes - we ignore the MemFree (which is sent directly by
        // mDNS_Deregister_internal), and allow the caller to deallocate immediately following mDNS_DeadvertiseInterface
        debugf("mDNS_HostNameCallback: MemFree (ignored)");
    }
    else
        LogMsg("mDNS_HostNameCallback: Unknown error %d for registration of record %s", result,  rr->resrec.name->c);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
mDNSlocal void mDNS_RandomizedHostNameCallback(mDNS *const m, AuthRecord *const addrRecord, const mStatus result)
{
    (void)addrRecord;   // Unused parameter

    if (result == mStatus_NameConflict)
    {
        AuthRecord *rr;
        domainlabel newUUIDLabel;

        GetRandomUUIDLabel(&newUUIDLabel);
        if (SameDomainLabel(newUUIDLabel.c, m->RandomizedHostname.c))
        {
            IncrementLabelSuffix(&newUUIDLabel, mDNSfalse);
        }

        mDNS_Lock(m);

        m->RandomizedHostname.c[0] = 0;
        AppendDomainLabel(&m->RandomizedHostname, &newUUIDLabel);
        AppendLiteralLabelString(&m->RandomizedHostname, "local");

        DeadvertiseAllInterfaceRecords(m, kDeadvertiseFlag_RandHostname);
        AdvertiseNecessaryInterfaceRecords(m);
        for (rr = m->ResourceRecords; rr; rr = rr->next)
        {
            if (rr->AutoTarget && AuthRecordIncludesOrIsAWDL(rr)) UpdateTargetHostName(m, rr);
        }
        for (rr = m->DuplicateRecords; rr; rr = rr->next)
        {
            if (rr->AutoTarget && AuthRecordIncludesOrIsAWDL(rr)) UpdateTargetHostName(m, rr);
        }

        mDNS_Unlock(m);
    }
}
#endif

mDNSlocal void UpdateInterfaceProtocols(mDNS *const m, NetworkInterfaceInfo *active)
{
    NetworkInterfaceInfo *intf;
    active->IPv4Available = mDNSfalse;
    active->IPv6Available = mDNSfalse;
    for (intf = m->HostInterfaces; intf; intf = intf->next)
        if (intf->InterfaceID == active->InterfaceID)
        {
            if (intf->ip.type == mDNSAddrType_IPv4 && intf->McastTxRx) active->IPv4Available = mDNStrue;
            if (intf->ip.type == mDNSAddrType_IPv6 && intf->McastTxRx) active->IPv6Available = mDNStrue;
        }
}

mDNSlocal void RestartRecordGetZoneData(mDNS * const m)
{
    AuthRecord *rr;
    LogInfo("RestartRecordGetZoneData: ResourceRecords");
    for (rr = m->ResourceRecords; rr; rr=rr->next)
        if (AuthRecord_uDNS(rr) && rr->state != regState_NoTarget)
        {
            debugf("RestartRecordGetZoneData: StartGetZoneData for %##s", rr->resrec.name->c);
            // Zero out the updateid so that if we have a pending response from the server, it won't
            // be accepted as a valid response. If we accept the response, we might free the new "nta"
            if (rr->nta) { rr->updateid = zeroID; CancelGetZoneData(m, rr->nta); }
            rr->nta = StartGetZoneData(m, rr->resrec.name, ZoneServiceUpdate, RecordRegistrationGotZoneData, rr);
        }
}

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
mDNSlocal void InitializeNetWakeState(mDNS *const m, NetworkInterfaceInfo *set)
{
    int i;
    // We initialize ThisQInterval to -1 indicating that the question has not been started
    // yet. If the question (browse) is started later during interface registration, it will
    // be stopped during interface deregistration. We can't sanity check to see if the
    // question has been stopped or not before initializing it to -1 because we need to
    // initialize it to -1 the very first time.

    set->NetWakeBrowse.ThisQInterval = -1;
    for (i=0; i<3; i++)
    {
        set->NetWakeResolve[i].ThisQInterval = -1;
        set->SPSAddr[i].type = mDNSAddrType_None;
    }
    set->NextSPSAttempt     = -1;
    set->NextSPSAttemptTime = m->timenow;
}
#endif

mDNSexport void mDNS_ActivateNetWake_internal(mDNS *const m, NetworkInterfaceInfo *set)
{
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    NetworkInterfaceInfo *p = m->HostInterfaces;
    while (p && p != set) p=p->next;
    if (!p)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_ActivateNetWake_internal: NetworkInterfaceInfo %p not found in active list", set);
        return;
    }

    if (set->InterfaceActive)
    {
        LogRedact(MDNS_LOG_CATEGORY_SPS, MDNS_LOG_DEFAULT, "ActivateNetWake for " PUB_S " (" PRI_IP_ADDR ")", set->ifname, &set->ip);
        mDNS_StartBrowse_internal(m, &set->NetWakeBrowse, &SleepProxyServiceType, &localdomain, set->InterfaceID, 0, mDNSfalse, mDNSfalse, m->SPSBrowseCallback, set);
    }
#else
    (void)m;
    (void)set;
#endif
}

mDNSexport void mDNS_DeactivateNetWake_internal(mDNS *const m, NetworkInterfaceInfo *set)
{
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    NetworkInterfaceInfo *p = m->HostInterfaces;
    while (p && p != set) p=p->next;
    if (!p)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_DeactivateNetWake_internal: NetworkInterfaceInfo %p not found in active list", set);
        return;
    }

    // Note: We start the browse only if the interface is NetWake capable and we use this to
    // stop the resolves also. Hence, the resolves should not be started without the browse
    // being started i.e, resolves should not happen unless NetWake capable which is
    // guaranteed by BeginSleepProcessing.
    if (set->NetWakeBrowse.ThisQInterval >= 0)
    {
        int i;
        LogRedact(MDNS_LOG_CATEGORY_SPS, MDNS_LOG_DEFAULT, "DeactivateNetWake for " PUB_S " (" PRI_IP_ADDR ")", set->ifname, &set->ip);

        // Stop our browse and resolve operations
        mDNS_StopQuery_internal(m, &set->NetWakeBrowse);
        for (i=0; i<3; i++) if (set->NetWakeResolve[i].ThisQInterval >= 0) mDNS_StopQuery_internal(m, &set->NetWakeResolve[i]);

        // Make special call to the browse callback to let it know it can to remove all records for this interface
        if (m->SPSBrowseCallback)
        {
            mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
            m->SPSBrowseCallback(m, &set->NetWakeBrowse, mDNSNULL, QC_rmv);
            mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
        }

        // Reset our variables back to initial state, so we're ready for when NetWake is turned back on
        // (includes resetting NetWakeBrowse.ThisQInterval back to -1)
        InitializeNetWakeState(m, set);
    }
#else
    (void)m;
    (void)set;
#endif
}

mDNSlocal mDNSBool IsInterfaceValidForQuestion(const DNSQuestion *const q, const NetworkInterfaceInfo *const intf)
{
    if (q->InterfaceID == mDNSInterface_Any)
    {
        return mDNSPlatformValidQuestionForInterface(q, intf);
    }
    else
    {
        return (q->InterfaceID == intf->InterfaceID);
    }
}

mDNSexport mStatus mDNS_RegisterInterface(mDNS *const m, NetworkInterfaceInfo *set, InterfaceActivationSpeed activationSpeed)
{
    AuthRecord *rr;
    mDNSBool FirstOfType = mDNStrue;
    NetworkInterfaceInfo **p = &m->HostInterfaces;

    if (!set->InterfaceID)
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "Tried to register a NetworkInterfaceInfo with zero InterfaceID - ifaddr: " PRI_IP_ADDR,
            &set->ip);
        return(mStatus_Invalid);
    }

    if (!mDNSAddressIsValidNonZero(&set->mask))
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "Tried to register a NetworkInterfaceInfo with invalid mask - ifaddr: "  PRI_IP_ADDR
            ", ifmask: " PUB_IP_ADDR, &set->ip, &set->mask);
        return(mStatus_Invalid);
    }

    mDNS_Lock(m);

    // Assume this interface will be active now, unless we find a duplicate already in the list
    set->InterfaceActive = mDNStrue;
    set->IPv4Available   = (mDNSu8)(set->ip.type == mDNSAddrType_IPv4 && set->McastTxRx);
    set->IPv6Available   = (mDNSu8)(set->ip.type == mDNSAddrType_IPv6 && set->McastTxRx);

#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    InitializeNetWakeState(m, set);
#endif

    // Scan list to see if this InterfaceID is already represented
    while (*p)
    {
        if (*p == set)
        {
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "Tried to register a NetworkInterfaceInfo that's already in the list - "
                "ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR, set->ifname, &set->ip);
            mDNS_Unlock(m);
            return(mStatus_AlreadyRegistered);
        }

        if ((*p)->InterfaceID == set->InterfaceID)
        {
            // This InterfaceID already represented by a different interface in the list, so mark this instance inactive for now
            set->InterfaceActive = mDNSfalse;
            if (set->ip.type == (*p)->ip.type) FirstOfType = mDNSfalse;
            if (set->ip.type == mDNSAddrType_IPv4 && set->McastTxRx) (*p)->IPv4Available = mDNStrue;
            if (set->ip.type == mDNSAddrType_IPv6 && set->McastTxRx) (*p)->IPv6Available = mDNStrue;
        }

        p=&(*p)->next;
    }

    set->next = mDNSNULL;
    *p = set;

    if (set->Advertise) AdvertiseInterfaceIfNeeded(m, set);

    if (set->InterfaceActive)
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Interface not represented in list; marking active and retriggering queries - "
            "ifid: %d, ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR, IIDPrintable(set->InterfaceID), set->ifname, &set->ip);
    }
    else
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Interface already represented in list - ifid: %d, ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR,
            IIDPrintable(set->InterfaceID), set->ifname, &set->ip);
    }

    if (set->NetWake) mDNS_ActivateNetWake_internal(m, set);

    // In early versions of OS X the IPv6 address remains on an interface even when the interface is turned off,
    // giving the false impression that there's an active representative of this interface when there really isn't.
    // Therefore, when registering an interface, we want to re-trigger our questions and re-probe our Resource Records,
    // even if we believe that we previously had an active representative of this interface.
    if (set->McastTxRx && (FirstOfType || set->InterfaceActive))
    {
        DNSQuestion *q;
        // Normally, after an interface comes up, we pause half a second before beginning probing.
        // This is to guard against cases where there's rapid interface changes, where we could be confused by
        // seeing packets we ourselves sent just moments ago (perhaps when this interface had a different address)
        // which are then echoed back after a short delay by some Ethernet switches and some 802.11 base stations.
        // We don't want to do a probe, and then see a stale echo of an announcement we ourselves sent,
        // and think it's a conflicting answer to our probe.
        // In the case of a flapping interface, we pause for five seconds, and reduce the announcement count to one packet.
        mDNSs32 probedelay;
        mDNSu8 numannounce;
        mdns_clang_ignore_warning_begin(-Wswitch-default);
        switch (activationSpeed)
        {
            case FastActivation:
                probedelay = (mDNSs32)0;
                numannounce = InitialAnnounceCount;
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                    "Using fast activation for DirectLink interface - ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR,
                    set->ifname, &set->ip);
                break;

#if MDNSRESPONDER_SUPPORTS(APPLE, SLOW_ACTIVATION)
            case SlowActivation:
                probedelay = mDNSPlatformOneSecond * 5;
                numannounce = (mDNSu8)1;
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Frequent transitions for interface, doing slow activation - "
                    "ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR, set->ifname, &set->ip);
                m->mDNSStats.InterfaceUpFlap++;
                break;
#endif

            case NormalActivation:
                probedelay = mDNSPlatformOneSecond / 2;
                numannounce = InitialAnnounceCount;
                break;
        }
        mdns_clang_ignore_warning_end();

        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Interface probe will be delayed - ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR ", probe delay: %d",
            set->ifname, &set->ip, probedelay);

        // No probe or sending suppression on DirectLink type interfaces.
        if (activationSpeed == FastActivation)
        {
            m->SuppressQueries   = 0;
            m->SuppressResponses = 0;
            m->SuppressProbes    = 0;
        }
        else
        {
	        // Use a small amount of randomness:
	        // In the case of a network administrator turning on an Ethernet hub so that all the
	        // connected machines establish link at exactly the same time, we don't want them all
	        // to go and hit the network with identical queries at exactly the same moment.
	        // We set a random delay of up to InitialQuestionInterval (1/3 second).
	        // We must *never* set m->SuppressQueries to more than that (or set it repeatedly in a way
	        // that causes mDNSResponder to remain in a prolonged state of SuppressQueries, because
	        // suppressing packet sending for more than about 1/3 second can cause protocol correctness
	        // to start to break down (e.g. we don't answer probes fast enough, and get name conflicts).
	        // See <rdar://problem/4073853> mDNS: m->SuppressSending set too enthusiastically
            if (!m->SuppressQueries)
            {
                m->SuppressQueries = NonZeroTime(m->timenow + (mDNSs32)mDNSRandom((mDNSu32)InitialQuestionInterval));
            }
            if (m->SuppressProbes == 0 ||
                m->SuppressProbes - NonZeroTime(m->timenow + probedelay) < 0)
                m->SuppressProbes = NonZeroTime(m->timenow + probedelay);
        }

    #if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
        // Include OWNER option in packets for 60 seconds after connecting to the network. Setting
        // it here also handles the wake up case as the network link comes UP after waking causing
        // us to reconnect to the network. If we do this as part of the wake up code, it is possible
        // that the network link comes UP after 60 seconds and we never set the OWNER option
        m->AnnounceOwner = NonZeroTime(m->timenow + 60 * mDNSPlatformOneSecond);
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG, "Setting AnnounceOwner");
    #endif

        m->mDNSStats.InterfaceUp++;
        for (q = m->Questions; q; q=q->next)                                // Scan our list of questions
        {
            if (mDNSOpaque16IsZero(q->TargetQID))
            {
                // If the DNSQuestion's mDNS queries are supposed to be sent over the interface, then reactivate it.
                if (IsInterfaceValidForQuestion(q, set))
                {
#if MDNSRESPONDER_SUPPORTS(APPLE, SLOW_ACTIVATION)
                    // If flapping, delay between first and second queries is nine seconds instead of one second
                    mDNSBool dodelay = (activationSpeed == SlowActivation) && (q->FlappingInterface1 == set->InterfaceID || q->FlappingInterface2 == set->InterfaceID);
                    mDNSs32 initial  = dodelay ? InitialQuestionInterval * QuestionIntervalStep2 : InitialQuestionInterval;
                    mDNSs32 qdelay   = dodelay ? kDefaultQueryDelayTimeForFlappingInterface : 0;
                    if (dodelay)
                    {
                        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "No cache records expired for the question " PRI_DM_NAME " (" PUB_S ");"
                            " delaying it by %d seconds", DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype), qdelay);
                    }
#else
                    mDNSs32 initial  = InitialQuestionInterval;
                    mDNSs32 qdelay   = 0;
#endif

                    if (!q->ThisQInterval || q->ThisQInterval > initial)
                    {
                        q->ThisQInterval  = initial;
                        q->RequestUnicast = kDefaultRequestUnicastCount;
                    }
                    q->LastQTime = m->timenow - q->ThisQInterval + qdelay;
                    q->RecentAnswerPkts = 0;
                    SetNextQueryTime(m,q);
                }
            }
        }

        // For all our non-specific authoritative resource records (and any dormant records specific to this interface)
        // we now need them to re-probe if necessary, and then re-announce.
        for (rr = m->ResourceRecords; rr; rr=rr->next)
        {
            if (IsInterfaceValidForAuthRecord(rr, set->InterfaceID))
            {
                mDNSCoreRestartRegistration(m, rr, numannounce);
            }
        }
    }

    RestartRecordGetZoneData(m);

    mDNS_UpdateAllowSleep(m);

    mDNS_Unlock(m);
    return(mStatus_NoError);
}

mDNSlocal void AdjustAddressRecordSetsEx(mDNS *const m, NetworkInterfaceInfo *removedIntf, mDNSBool forRandHostname)
{
    NetworkInterfaceInfo *intf;
    const AuthRecord *oldAR;
    AuthRecord *newAR;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    if (mDNSPlatformInterfaceIsAWDL(removedIntf->InterfaceID)) return;
#endif
    oldAR = GetInterfaceAddressRecord(removedIntf, forRandHostname);
    newAR = GetFirstAddressRecordEx(m, forRandHostname);
    for (intf = m->HostInterfaces; intf; intf = intf->next)
    {
        AuthRecord *ar;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
        if (mDNSPlatformInterfaceIsAWDL(intf->InterfaceID)) continue;
#endif
        ar = GetInterfaceAddressRecord(intf, forRandHostname);
        if (ar->RRSet == (uintptr_t)oldAR)
        {
            ar->RRSet = (uintptr_t)(newAR ? newAR : ar);
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "Changed RRSet for " PRI_S, ARDisplayString(m, ar));
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, "New RRSet:        %" PRIxPTR "", ar->RRSet);
        }
    }
}
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
#define AdjustAddressRecordSetsForRandHostname(M, REMOVED_INTF) AdjustAddressRecordSetsEx(M, REMOVED_INTF, mDNStrue)
#endif
#define AdjustAddressRecordSets(M, REMOVED_INTF)                AdjustAddressRecordSetsEx(M, REMOVED_INTF, mDNSfalse)

// Note: mDNS_DeregisterInterface calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport void mDNS_DeregisterInterface(mDNS *const m, NetworkInterfaceInfo *set, InterfaceActivationSpeed activationSpeed)
{
#if !MDNSRESPONDER_SUPPORTS(APPLE, SLOW_ACTIVATION)
    (void)activationSpeed;   // Unused parameter
#endif
    NetworkInterfaceInfo **p = &m->HostInterfaces;
    mDNSBool revalidate = mDNSfalse;
    NetworkInterfaceInfo *intf;

    mDNS_Lock(m);

    // Find this record in our list
    while (*p && *p != set) p=&(*p)->next;
    if (!*p)
    {
        LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEBUG, "NetworkInterfaceInfo not found in list");
        mDNS_Unlock(m);
        return;
    }

    mDNS_DeactivateNetWake_internal(m, set);

    // Unlink this record from our list
    *p = (*p)->next;
    set->next = mDNSNULL;

    if (!set->InterfaceActive)
    {
        // If this interface not the active member of its set, update the v4/v6Available flags for the active member
        for (intf = m->HostInterfaces; intf; intf = intf->next)
            if (intf->InterfaceActive && intf->InterfaceID == set->InterfaceID)
                UpdateInterfaceProtocols(m, intf);
    }
    else
    {
        intf = FirstInterfaceForID(m, set->InterfaceID);
        if (intf)
        {
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT,
                "Another representative of InterfaceID exists - ifid: %d, ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR,
                IIDPrintable(set->InterfaceID), set->ifname, &set->ip);
            if (intf->InterfaceActive)
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_ERROR, "intf->InterfaceActive already set for interface - ifname: "
                    PUB_S ", ifaddr: " PRI_IP_ADDR, set->ifname, &set->ip);
            }
            intf->InterfaceActive = mDNStrue;
            UpdateInterfaceProtocols(m, intf);

            if (intf->NetWake) mDNS_ActivateNetWake_internal(m, intf);

            // See if another representative *of the same type* exists. If not, we mave have gone from
            // dual-stack to v6-only (or v4-only) so we need to reconfirm which records are still valid.
            for (intf = m->HostInterfaces; intf; intf = intf->next)
                if (intf->InterfaceID == set->InterfaceID && intf->ip.type == set->ip.type)
                    break;
            if (!intf) revalidate = mDNStrue;
        }
        else
        {
            mDNSu32 slot;
            CacheGroup *cg;
            CacheRecord *rr;
            DNSQuestion *q;
#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
            mDNSu32     cacheHitMulticastCount = 0;
            mDNSu32     cacheMissMulticastCount = 0;
            mDNSu32     cacheHitUnicastCount = 0;
            mDNSu32     cacheMissUnicastCount = 0;
#endif
            LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Last representative of InterfaceID deregistered; marking questions etc. dormant - "
                "ifid: %d, ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR,
                IIDPrintable(set->InterfaceID), set->ifname, &set->ip);

            m->mDNSStats.InterfaceDown++;

#if MDNSRESPONDER_SUPPORTS(APPLE, SLOW_ACTIVATION)
            if (set->McastTxRx && (activationSpeed == SlowActivation))
            {
                LogRedact(MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_DEFAULT, "Frequent transitions for interface - ifname: " PUB_S ", ifaddr: " PRI_IP_ADDR,
                    set->ifname, &set->ip);
                m->mDNSStats.InterfaceDownFlap++;
            }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
            DPCHandleInterfaceDown(set->InterfaceID);
#endif
            // 1. Deactivate any questions specific to this interface, and tag appropriate questions
            // so that mDNS_RegisterInterface() knows how swiftly it needs to reactivate them
            for (q = m->Questions; q; q=q->next)
            {
                if (mDNSOpaque16IsZero(q->TargetQID))                   // Only deactivate multicast quesstions. (Unicast questions are stopped when/if the associated DNS server group goes away.)
                {
                    if (q->InterfaceID == set->InterfaceID) q->ThisQInterval = 0;
                    if (!q->InterfaceID || q->InterfaceID == set->InterfaceID)
                    {
                        q->FlappingInterface2 = q->FlappingInterface1;
                        q->FlappingInterface1 = set->InterfaceID;       // Keep history of the last two interfaces to go away
                    }
                }
            }

            // 2. Flush any cache records received on this interface
            revalidate = mDNSfalse;     // Don't revalidate if we're flushing the records
            FORALL_CACHERECORDS(slot, cg, rr)
            {
                if (rr->resrec.InterfaceID == set->InterfaceID)
                {
#if MDNSRESPONDER_SUPPORTS(APPLE, SLOW_ACTIVATION)
                    // If this interface is deemed flapping,
                    // postpone deleting the cache records in case the interface comes back again
                    if (set->McastTxRx && (activationSpeed == SlowActivation))
                    {
                        // For a flapping interface we want these records to go away after
                        // kDefaultReconfirmTimeForFlappingInterface seconds if they are not reconfirmed.
                        mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForFlappingInterface);
                        // We set UnansweredQueries = MaxUnansweredQueries so we don't waste time doing any queries for them --
                        // if the interface does come back, any relevant questions will be reactivated anyway
                        rr->UnansweredQueries = MaxUnansweredQueries;
                    }
                    else
#endif
                    {
#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
                        if (rr->LastCachedAnswerTime)
                        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                            const mdns_dns_service_t dnsservice = mdns_cache_metadata_get_dns_service(rr->resrec.metadata);
                            if (dnsservice)             cacheHitUnicastCount++;
#else
                            if (rr->resrec.rDNSServer)  cacheHitUnicastCount++;
#endif
                            else                        cacheHitMulticastCount++;
                        }
                        else
                        {
#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
                            const mdns_dns_service_t dnsservice = mdns_cache_metadata_get_dns_service(rr->resrec.metadata);
                            if (dnsservice)             cacheMissUnicastCount++;
#else
                            if (rr->resrec.rDNSServer)  cacheMissUnicastCount++;
#endif
                            else                        cacheMissMulticastCount++;
                        }
#endif
                        mDNS_PurgeCacheResourceRecord(m, rr);
                    }
                }
            }
#if MDNSRESPONDER_SUPPORTS(APPLE, CACHE_ANALYTICS)
            dnssd_analytics_update_cache_usage_counts(cacheHitMulticastCount, cacheMissMulticastCount, cacheHitUnicastCount, cacheMissUnicastCount);
#endif
        }
    }

    // If we still have address records referring to this one, update them.
    // This is safe, because this NetworkInterfaceInfo has already been unlinked from the list,
    // so the call to AdjustAddressRecordSets*() won’t accidentally find it.
    AdjustAddressRecordSets(m, set);
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    AdjustAddressRecordSetsForRandHostname(m, set);
#endif

    // If we were advertising on this interface, deregister those address and reverse-lookup records now
    if (set->Advertise) DeadvertiseInterface(m, set, kDeadvertiseFlag_All);

    // If we have any cache records received on this interface that went away, then re-verify them.
    // In some versions of OS X the IPv6 address remains on an interface even when the interface is turned off,
    // giving the false impression that there's an active representative of this interface when there really isn't.
    // Don't need to do this when shutting down, because *all* interfaces are about to go away
    if (revalidate && !m->ShutdownTime)
    {
        mDNSu32 slot;
        CacheGroup *cg;
        CacheRecord *rr;
        FORALL_CACHERECORDS(slot, cg, rr)
        if (rr->resrec.InterfaceID == set->InterfaceID)
            mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForFlappingInterface);
    }

    mDNS_UpdateAllowSleep(m);

    mDNS_Unlock(m);
}

mDNSlocal void ServiceCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
    ServiceRecordSet *sr = (ServiceRecordSet *)rr->RecordContext;
    mDNSBool unregistered = mDNSfalse;

    #if MDNS_DEBUGMSGS
    {
        char *msg = "Unknown result";
        if      (result == mStatus_NoError) msg = "Name Registered";
        else if (result == mStatus_NameConflict) msg = "Name Conflict";
        else if (result == mStatus_MemFree) msg = "Memory Free";
        debugf("ServiceCallback: %##s (%s) %s (%d)", rr->resrec.name->c, DNSTypeName(rr->resrec.rrtype), msg, result);
    }
    #endif

    // Only pass on the NoError acknowledgement for the SRV record (when it finishes probing)
    if (result == mStatus_NoError && rr != &sr->RR_SRV) return;

    // If we got a name conflict on either SRV or TXT, forcibly deregister this service, and record that we did that
    if (result == mStatus_NameConflict)
    {
        sr->Conflict = mDNStrue;                // Record that this service set had a conflict
        mDNS_DeregisterService(m, sr);          // Unlink the records from our list
        return;
    }

    if (result == mStatus_MemFree)
    {
        // If the SRV/TXT/PTR records, or the _services._dns-sd._udp record, or any of the subtype PTR records,
        // are still in the process of deregistering, don't pass on the NameConflict/MemFree message until
        // every record is finished cleaning up.
        mDNSu32 i;
        ExtraResourceRecord *e = sr->Extras;

        if (sr->RR_SRV.resrec.RecordType != kDNSRecordTypeUnregistered) return;
        if (sr->RR_TXT.resrec.RecordType != kDNSRecordTypeUnregistered) return;
        if (sr->RR_PTR.resrec.RecordType != kDNSRecordTypeUnregistered) return;
        if (sr->RR_ADV.resrec.RecordType != kDNSRecordTypeUnregistered) return;
        for (i=0; i<sr->NumSubTypes; i++) if (sr->SubTypes[i].resrec.RecordType != kDNSRecordTypeUnregistered) return;

        while (e)
        {
            if (e->r.resrec.RecordType != kDNSRecordTypeUnregistered) return;
            e = e->next;
        }

        // If this ServiceRecordSet was forcibly deregistered, and now its memory is ready for reuse,
        // then we can now report the NameConflict to the client
        if (sr->Conflict) result = mStatus_NameConflict;

        // Complete any pending record updates to avoid memory leaks.
        if (sr->RR_SRV.NewRData) CompleteRDataUpdate(m, &sr->RR_SRV);
        if (sr->RR_TXT.NewRData) CompleteRDataUpdate(m, &sr->RR_TXT);
        if (sr->RR_PTR.NewRData) CompleteRDataUpdate(m, &sr->RR_PTR);
        if (sr->RR_ADV.NewRData) CompleteRDataUpdate(m, &sr->RR_ADV);
        unregistered = mDNStrue;
    }

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
        "ServiceCallback: All records " PUB_S "registered for " PRI_DM_NAME,
        unregistered ? "un" : "", DM_NAME_PARAM(sr->RR_PTR.resrec.name));
    // CAUTION: MUST NOT do anything more with sr after calling sr->Callback(), because the client's callback
    // function is allowed to do anything, including deregistering this service and freeing its memory.
    if (sr->ServiceCallback)
        sr->ServiceCallback(m, sr, result);
}

mDNSlocal void NSSCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
    ServiceRecordSet *sr = (ServiceRecordSet *)rr->RecordContext;
    if (sr->ServiceCallback)
        sr->ServiceCallback(m, sr, result);
}


// Derive AuthRecType from the kDNSServiceFlags* values.
mDNSlocal AuthRecType setAuthRecType(mDNSInterfaceID InterfaceID, mDNSu32 flags)
{
    AuthRecType artype;

    if (InterfaceID == mDNSInterface_LocalOnly)
        artype = AuthRecordLocalOnly;
    else if (InterfaceID == mDNSInterface_P2P || InterfaceID == mDNSInterface_BLE)
        artype = AuthRecordP2P;
    else if ((InterfaceID == mDNSInterface_Any) && (flags & kDNSServiceFlagsIncludeP2P)
            && (flags & kDNSServiceFlagsIncludeAWDL))
        artype = AuthRecordAnyIncludeAWDLandP2P;
    else if ((InterfaceID == mDNSInterface_Any) && (flags & kDNSServiceFlagsIncludeP2P))
        artype = AuthRecordAnyIncludeP2P;
    else if ((InterfaceID == mDNSInterface_Any) && (flags & kDNSServiceFlagsIncludeAWDL))
        artype = AuthRecordAnyIncludeAWDL;
    else
        artype = AuthRecordAny;

    return artype;
}

// Note:
// Name is first label of domain name (any dots in the name are actual dots, not label separators)
// Type is service type (e.g. "_ipp._tcp.")
// Domain is fully qualified domain name (i.e. ending with a null label)
// We always register a TXT, even if it is empty (so that clients are not
// left waiting forever looking for a nonexistent record.)
// If the host parameter is mDNSNULL or the root domain (ASCII NUL),
// then the default host name (m->MulticastHostname) is automatically used
// If the optional target host parameter is set, then the storage it points to must remain valid for the lifetime of the service registration
mDNSexport mStatus mDNS_RegisterService(mDNS *const m, ServiceRecordSet *sr,
                                        const domainlabel *const name, const domainname *const type, const domainname *const domain,
                                        const domainname *const host, mDNSIPPort port, RData *const txtrdata, const mDNSu8 txtinfo[], mDNSu16 txtlen,
                                        AuthRecord *SubTypes, mDNSu32 NumSubTypes,
                                        mDNSInterfaceID InterfaceID, mDNSServiceCallback Callback, void *Context, mDNSu32 flags)
{
    mStatus err;
    mDNSu32 i;
    AuthRecType artype;
    mDNSu8 recordType = (flags & kDNSServiceFlagsKnownUnique) ? kDNSRecordTypeKnownUnique : kDNSRecordTypeUnique;

    sr->ServiceCallback = Callback;
    sr->ServiceContext  = Context;
    sr->Conflict        = mDNSfalse;

    sr->Extras          = mDNSNULL;
    sr->NumSubTypes     = NumSubTypes;
    sr->SubTypes        = SubTypes;
    sr->flags           = flags;

    artype = setAuthRecType(InterfaceID, flags);

    // Initialize the AuthRecord objects to sane values
    // Need to initialize everything correctly *before* making the decision whether to do a RegisterNoSuchService and bail out
    mDNS_SetupResourceRecord(&sr->RR_ADV, mDNSNULL, InterfaceID, kDNSType_PTR, kStandardTTL, kDNSRecordTypeAdvisory, artype, ServiceCallback, sr);
    mDNS_SetupResourceRecord(&sr->RR_PTR, mDNSNULL, InterfaceID, kDNSType_PTR, kStandardTTL, kDNSRecordTypeShared,   artype, ServiceCallback, sr);

    if (flags & kDNSServiceFlagsWakeOnlyService)
    {
        sr->RR_PTR.AuthFlags = AuthFlagsWakeOnly;
    }

    mDNS_SetupResourceRecord(&sr->RR_SRV, mDNSNULL, InterfaceID, kDNSType_SRV, kHostNameTTL, recordType, artype, ServiceCallback, sr);
    mDNS_SetupResourceRecord(&sr->RR_TXT, txtrdata, InterfaceID, kDNSType_TXT, kStandardTTL, recordType, artype, ServiceCallback, sr);

    // If port number is zero, that means the client is really trying to do a RegisterNoSuchService
    if (mDNSIPPortIsZero(port))
        return(mDNS_RegisterNoSuchService(m, &sr->RR_SRV, name, type, domain, mDNSNULL, InterfaceID, NSSCallback, sr, flags));

    // If the caller is registering an oversized TXT record,
    // it is the caller's responsibility to allocate a ServiceRecordSet structure that is large enough for it
    if (sr->RR_TXT.resrec.rdata->MaxRDLength < txtlen)
        sr->RR_TXT.resrec.rdata->MaxRDLength = txtlen;

    // Set up the record names
    // For now we only create an advisory record for the main type, not for subtypes
    // We need to gain some operational experience before we decide if there's a need to create them for subtypes too
    if (ConstructServiceName(&sr->RR_ADV.namestorage, (const domainlabel*)"\x09_services", (const domainname*)"\x07_dns-sd\x04_udp", domain) == mDNSNULL)
        return(mStatus_BadParamErr);
    if (ConstructServiceName(&sr->RR_PTR.namestorage, mDNSNULL, type, domain) == mDNSNULL) return(mStatus_BadParamErr);
    if (ConstructServiceName(&sr->RR_SRV.namestorage, name,     type, domain) == mDNSNULL) return(mStatus_BadParamErr);
    AssignDomainName(&sr->RR_TXT.namestorage, sr->RR_SRV.resrec.name);

    // 1. Set up the ADV record rdata to advertise our service type
    AssignDomainName(&sr->RR_ADV.resrec.rdata->u.name, sr->RR_PTR.resrec.name);

    // 2. Set up the PTR record rdata to point to our service name
    // We set up two additionals, so when a client asks for this PTR we automatically send the SRV and the TXT too
    // Note: uDNS registration code assumes that Additional1 points to the SRV record
    AssignDomainName(&sr->RR_PTR.resrec.rdata->u.name, sr->RR_SRV.resrec.name);
    sr->RR_PTR.Additional1 = &sr->RR_SRV;
    sr->RR_PTR.Additional2 = &sr->RR_TXT;

    // 2a. Set up any subtype PTRs to point to our service name
    // If the client is using subtypes, it is the client's responsibility to have
    // already set the first label of the record name to the subtype being registered
    for (i=0; i<NumSubTypes; i++)
    {
        domainname st;
        AssignDomainName(&st, sr->SubTypes[i].resrec.name);
        st.c[1+st.c[0]] = 0;            // Only want the first label, not the whole FQDN (particularly for mDNS_RenameAndReregisterService())
        AppendDomainName(&st, type);
        mDNS_SetupResourceRecord(&sr->SubTypes[i], mDNSNULL, InterfaceID, kDNSType_PTR, kStandardTTL, kDNSRecordTypeShared, artype, ServiceCallback, sr);
        if (ConstructServiceName(&sr->SubTypes[i].namestorage, mDNSNULL, &st, domain) == mDNSNULL) return(mStatus_BadParamErr);
        AssignDomainName(&sr->SubTypes[i].resrec.rdata->u.name, &sr->RR_SRV.namestorage);
        sr->SubTypes[i].Additional1 = &sr->RR_SRV;
        sr->SubTypes[i].Additional2 = &sr->RR_TXT;
    }

    // 3. Set up the SRV record rdata.
    sr->RR_SRV.resrec.rdata->u.srv.priority = 0;
    sr->RR_SRV.resrec.rdata->u.srv.weight   = 0;
    sr->RR_SRV.resrec.rdata->u.srv.port     = port;

    // Setting AutoTarget tells DNS that the target of this SRV is to be automatically kept in sync with our host name
    if (host && host->c[0]) AssignDomainName(&sr->RR_SRV.resrec.rdata->u.srv.target, host);
    else { sr->RR_SRV.AutoTarget = Target_AutoHost; sr->RR_SRV.resrec.rdata->u.srv.target.c[0] = '\0'; }

    // 4. Set up the TXT record rdata,
    // and set DependentOn because we're depending on the SRV record to find and resolve conflicts for us
    // Note: uDNS registration code assumes that DependentOn points to the SRV record
    if (txtinfo == mDNSNULL) sr->RR_TXT.resrec.rdlength = 0;
    else if (txtinfo != sr->RR_TXT.resrec.rdata->u.txt.c)
    {
        sr->RR_TXT.resrec.rdlength = txtlen;
        if (sr->RR_TXT.resrec.rdlength > sr->RR_TXT.resrec.rdata->MaxRDLength) return(mStatus_BadParamErr);
        mDNSPlatformMemCopy(sr->RR_TXT.resrec.rdata->u.txt.c, txtinfo, txtlen);
    }
    sr->RR_TXT.DependentOn = &sr->RR_SRV;

    mDNS_Lock(m);
    // It is important that we register SRV first. uDNS assumes that SRV is registered first so
    // that if the SRV cannot find a target, rest of the records that belong to this service
    // will not be activated.
    err = mDNS_Register_internal(m, &sr->RR_SRV);
    // If we can't register the SRV record due to errors, bail out. It has not been inserted in
    // any list and hence no need to deregister. We could probably do similar checks for other
    // records below and bail out. For now, this seems to be sufficient to address rdar://9304275
    if (err)
    {
        mDNS_Unlock(m);
        return err;
    }
    if (!err) err = mDNS_Register_internal(m, &sr->RR_TXT);
    // We register the RR_PTR last, because we want to be sure that in the event of a forced call to
    // mDNS_StartExit, the RR_PTR will be the last one to be forcibly deregistered, since that is what triggers
    // the mStatus_MemFree callback to ServiceCallback, which in turn passes on the mStatus_MemFree back to
    // the client callback, which is then at liberty to free the ServiceRecordSet memory at will. We need to
    // make sure we've deregistered all our records and done any other necessary cleanup before that happens.
    if (!err) err = mDNS_Register_internal(m, &sr->RR_ADV);
    for (i=0; i<NumSubTypes; i++) if (!err) err = mDNS_Register_internal(m, &sr->SubTypes[i]);
    if (!err) err = mDNS_Register_internal(m, &sr->RR_PTR);

    mDNS_Unlock(m);

    if (err) mDNS_DeregisterService(m, sr);
#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
    else if (!RRLocalOnly(&sr->RR_PTR) &&
             SameDomainName(domain, &localdomain))
    {
        unicast_assist_auth_add(sr->RR_PTR.resrec.name, sr->RR_PTR.resrec.namehash, InterfaceID);
    }
#endif
    return(err);
}

mDNSexport mStatus mDNS_AddRecordToService(mDNS *const m, ServiceRecordSet *sr,
                                           ExtraResourceRecord *extra, RData *rdata, mDNSu32 ttl,  mDNSu32 flags)
{
    ExtraResourceRecord **e;
    mStatus status;
    AuthRecType artype;
    mDNSInterfaceID InterfaceID = sr->RR_PTR.resrec.InterfaceID;
    ResourceRecord *rr;

    artype = setAuthRecType(InterfaceID, flags);

    extra->next = mDNSNULL;
    mDNS_SetupResourceRecord(&extra->r, rdata, sr->RR_PTR.resrec.InterfaceID,
                             extra->r.resrec.rrtype, ttl, kDNSRecordTypeUnique, artype, ServiceCallback, sr);
    AssignDomainName(&extra->r.namestorage, sr->RR_SRV.resrec.name);

    mDNS_Lock(m);
    rr = mDNSNULL;
    if (extra->r.resrec.rrtype == kDNSType_TXT)
    {
        if (sr->RR_TXT.resrec.RecordType & kDNSRecordTypeUniqueMask) rr = &sr->RR_TXT.resrec;
    }
    else if (extra->r.resrec.rrtype == kDNSType_SRV)
    {
        if (sr->RR_SRV.resrec.RecordType & kDNSRecordTypeUniqueMask) rr = &sr->RR_SRV.resrec;
    }

    if (!rr)
    {
        ExtraResourceRecord *srExtra;

        for (srExtra = sr->Extras; srExtra; srExtra = srExtra->next)
        {
            if ((srExtra->r.resrec.rrtype == extra->r.resrec.rrtype) && (srExtra->r.resrec.RecordType & kDNSRecordTypeUniqueMask))
            {
                rr = &srExtra->r.resrec;
                break;
            }
        }
    }

    if (rr && (extra->r.resrec.rroriginalttl != rr->rroriginalttl))
    {
        LogMsg("mDNS_AddRecordToService: Correcting TTL from %4d to %4d for %s",
            extra->r.resrec.rroriginalttl, rr->rroriginalttl, RRDisplayString(m, &extra->r.resrec));
        extra->r.resrec.rroriginalttl = rr->rroriginalttl;
    }

    e = &sr->Extras;
    while (*e) e = &(*e)->next;

    extra->r.DependentOn = &sr->RR_SRV;

    debugf("mDNS_AddRecordToService adding record to %##s %s %d",
           extra->r.resrec.name->c, DNSTypeName(extra->r.resrec.rrtype), extra->r.resrec.rdlength);

    status = mDNS_Register_internal(m, &extra->r);
    if (status == mStatus_NoError) *e = extra;

    mDNS_Unlock(m);
    return(status);
}

mDNSexport mStatus mDNS_RemoveRecordFromService(mDNS *const m, ServiceRecordSet *sr, ExtraResourceRecord *extra,
                                                mDNSRecordCallback MemFreeCallback, void *Context)
{
    ExtraResourceRecord **e;
    mStatus status;

    mDNS_Lock(m);
    e = &sr->Extras;
    while (*e && *e != extra) e = &(*e)->next;
    if (!*e)
    {
        debugf("mDNS_RemoveRecordFromService failed to remove record from %##s", extra->r.resrec.name->c);
        status = mStatus_BadReferenceErr;
    }
    else
    {
        debugf("mDNS_RemoveRecordFromService removing record from %##s", extra->r.resrec.name->c);
        extra->r.RecordCallback = MemFreeCallback;
        extra->r.RecordContext  = Context;
        *e = (*e)->next;
        status = mDNS_Deregister_internal(m, &extra->r, mDNS_Dereg_normal);
    }
    mDNS_Unlock(m);
    return(status);
}

mDNSexport mStatus mDNS_RenameAndReregisterService(mDNS *const m, ServiceRecordSet *const sr, const domainlabel *newname)
{
    // Note: Don't need to use mDNS_Lock(m) here, because this code is just using public routines
    // mDNS_RegisterService() and mDNS_AddRecordToService(), which do the right locking internally.
    domainlabel name1, name2;
    domainname type, domain;
    const domainname *host = sr->RR_SRV.AutoTarget ? mDNSNULL : &sr->RR_SRV.resrec.rdata->u.srv.target;
    ExtraResourceRecord *extras = sr->Extras;
    mStatus err;

    DeconstructServiceName(sr->RR_SRV.resrec.name, &name1, &type, &domain);
    if (!newname)
    {
        name2 = name1;
        IncrementLabelSuffix(&name2, mDNStrue);
        newname = &name2;
    }

    if (SameDomainName(&domain, &localdomain))
        debugf("%##s service renamed from \"%#s\" to \"%#s\"", type.c, name1.c, newname->c);
    else debugf("%##s service (domain %##s) renamed from \"%#s\" to \"%#s\"",type.c, domain.c, name1.c, newname->c);

    // If there's a pending TXT record update at this point, which can happen if a DNSServiceUpdateRecord() call was made
    // after the TXT record's deregistration, execute it now, otherwise it will be lost during the service re-registration.
    if (sr->RR_TXT.NewRData) CompleteRDataUpdate(m, &sr->RR_TXT);
    err = mDNS_RegisterService(m, sr, newname, &type, &domain,
                               host, sr->RR_SRV.resrec.rdata->u.srv.port,
                               (sr->RR_TXT.resrec.rdata != &sr->RR_TXT.rdatastorage) ? sr->RR_TXT.resrec.rdata : mDNSNULL,
                               sr->RR_TXT.resrec.rdata->u.txt.c, sr->RR_TXT.resrec.rdlength,
                               sr->SubTypes, sr->NumSubTypes,
                               sr->RR_PTR.resrec.InterfaceID, sr->ServiceCallback, sr->ServiceContext, sr->flags);

    // mDNS_RegisterService() just reset sr->Extras to NULL.
    // Fortunately we already grabbed ourselves a copy of this pointer (above), so we can now run
    // through the old list of extra records, and re-add them to our freshly created service registration
    while (!err && extras)
    {
        ExtraResourceRecord *e = extras;
        extras = extras->next;
        err = mDNS_AddRecordToService(m, sr, e, e->r.resrec.rdata, e->r.resrec.rroriginalttl, 0);
    }

    return(err);
}

// Note: mDNS_DeregisterService calls mDNS_Deregister_internal which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport mStatus mDNS_DeregisterService_drt(mDNS *const m, ServiceRecordSet *sr, mDNS_Dereg_type drt)
{
    // If port number is zero, that means this was actually registered using mDNS_RegisterNoSuchService()
    if (mDNSIPPortIsZero(sr->RR_SRV.resrec.rdata->u.srv.port)) return(mDNS_DeregisterNoSuchService(m, &sr->RR_SRV));

    if (sr->RR_PTR.resrec.RecordType == kDNSRecordTypeUnregistered)
    {
        debugf("Service set for %##s already deregistered", sr->RR_SRV.resrec.name->c);
        return(mStatus_BadReferenceErr);
    }
    else if (sr->RR_PTR.resrec.RecordType == kDNSRecordTypeDeregistering)
    {
        LogInfo("Service set for %##s already in the process of deregistering", sr->RR_SRV.resrec.name->c);
        // Avoid race condition:
        // If a service gets a conflict, then we set the Conflict flag to tell us to generate
        // an mStatus_NameConflict message when we get the mStatus_MemFree for our PTR record.
        // If the client happens to deregister the service in the middle of that process, then
        // we clear the flag back to the normal state, so that we deliver a plain mStatus_MemFree
        // instead of incorrectly promoting it to mStatus_NameConflict.
        // This race condition is exposed particularly when the conformance test generates
        // a whole batch of simultaneous conflicts across a range of services all advertised
        // using the same system default name, and if we don't take this precaution then
        // we end up incrementing m->nicelabel multiple times instead of just once.
        // <rdar://problem/4060169> Bug when auto-renaming Computer Name after name collision
        sr->Conflict = mDNSfalse;
        return(mStatus_NoError);
    }
    else
    {
        mDNSu32 i;
        mStatus status;
        ExtraResourceRecord *e;
        mDNS_Lock(m);
        e = sr->Extras;

        // We use mDNS_Dereg_repeat because, in the event of a collision, some or all of the
        // SRV, TXT, or Extra records could have already been automatically deregistered, and that's okay
        mDNS_Deregister_internal(m, &sr->RR_SRV, mDNS_Dereg_repeat);
        mDNS_Deregister_internal(m, &sr->RR_TXT, mDNS_Dereg_repeat);
        mDNS_Deregister_internal(m, &sr->RR_ADV, drt);

        // We deregister all of the extra records, but we leave the sr->Extras list intact
        // in case the client wants to do a RenameAndReregister and reinstate the registration
        while (e)
        {
            mDNS_Deregister_internal(m, &e->r, mDNS_Dereg_repeat);
            e = e->next;
        }

        for (i=0; i<sr->NumSubTypes; i++)
            mDNS_Deregister_internal(m, &sr->SubTypes[i], drt);

#if MDNSRESPONDER_SUPPORTS(APPLE, UNICAST_ASSIST)
        if (!RRLocalOnly(&sr->RR_PTR) &&
            IsLocalDomain(sr->RR_PTR.resrec.name))
        {
            unicast_assist_auth_rmv(sr->RR_PTR.resrec.name, sr->RR_PTR.resrec.namehash, sr->RR_PTR.resrec.InterfaceID);
        }
#endif
        status = mDNS_Deregister_internal(m, &sr->RR_PTR, drt);
        mDNS_Unlock(m);
        return(status);
    }
}

// Create a registration that asserts that no such service exists with this name.
// This can be useful where there is a given function is available through several protocols.
// For example, a printer called "Stuart's Printer" may implement printing via the "pdl-datastream" and "IPP"
// protocols, but not via "LPR". In this case it would be prudent for the printer to assert the non-existence of an
// "LPR" service called "Stuart's Printer". Without this precaution, another printer than offers only "LPR" printing
// could inadvertently advertise its service under the same name "Stuart's Printer", which might be confusing for users.
mDNSexport mStatus mDNS_RegisterNoSuchService(mDNS *const m, AuthRecord *const rr,
                                              const domainlabel *const name, const domainname *const type, const domainname *const domain,
                                              const domainname *const host,
                                              const mDNSInterfaceID InterfaceID, mDNSRecordCallback Callback, void *Context, mDNSu32 flags)
{
    AuthRecType artype;

    artype = setAuthRecType(InterfaceID, flags);

    mDNS_SetupResourceRecord(rr, mDNSNULL, InterfaceID, kDNSType_SRV, kHostNameTTL, kDNSRecordTypeUnique, artype, Callback, Context);
    if (ConstructServiceName(&rr->namestorage, name, type, domain) == mDNSNULL) return(mStatus_BadParamErr);
    rr->resrec.rdata->u.srv.priority    = 0;
    rr->resrec.rdata->u.srv.weight      = 0;
    rr->resrec.rdata->u.srv.port        = zeroIPPort;
    if (host && host->c[0]) AssignDomainName(&rr->resrec.rdata->u.srv.target, host);
    else rr->AutoTarget = Target_AutoHost;
    return(mDNS_Register(m, rr));
}

mDNSexport mStatus mDNS_AdvertiseDomains(mDNS *const m, AuthRecord *rr,
                                         mDNS_DomainType DomainType, const mDNSInterfaceID InterfaceID, char *domname)
{
    AuthRecType artype;

    if (InterfaceID == mDNSInterface_LocalOnly)
        artype = AuthRecordLocalOnly;
    else if (InterfaceID == mDNSInterface_P2P || InterfaceID == mDNSInterface_BLE)
        artype = AuthRecordP2P;
    else
        artype = AuthRecordAny;
    mDNS_SetupResourceRecord(rr, mDNSNULL, InterfaceID, kDNSType_PTR, kStandardTTL, kDNSRecordTypeShared, artype, mDNSNULL, mDNSNULL);
    if (!MakeDomainNameFromDNSNameString(&rr->namestorage, mDNS_DomainTypeNames[DomainType])) return(mStatus_BadParamErr);
    if (!MakeDomainNameFromDNSNameString(&rr->resrec.rdata->u.name, domname)) return(mStatus_BadParamErr);
    return(mDNS_Register(m, rr));
}

mDNSlocal mDNSBool mDNS_IdUsedInResourceRecordsList(mDNS * const m, mDNSOpaque16 id)
{
    AuthRecord *r;
    for (r = m->ResourceRecords; r; r=r->next) if (mDNSSameOpaque16(id, r->updateid)) return mDNStrue;
    return mDNSfalse;
}

mDNSlocal mDNSBool mDNS_IdUsedInQuestionsList(mDNS * const m, mDNSOpaque16 id)
{
    DNSQuestion *q;
    for (q = m->Questions; q; q=q->next) if (mDNSSameOpaque16(id, q->TargetQID)) return mDNStrue;
    return mDNSfalse;
}

mDNSexport mDNSOpaque16 mDNS_NewMessageID(mDNS * const m)
{
    mDNSOpaque16 id = zeroID;
    int i;

    for (i=0; i<10; i++)
    {
        id = mDNSOpaque16fromIntVal(1 + (mDNSu16)mDNSRandom(0xFFFE));
        if (!mDNS_IdUsedInResourceRecordsList(m, id) && !mDNS_IdUsedInQuestionsList(m, id)) break;
    }

    debugf("mDNS_NewMessageID: %5d", mDNSVal16(id));

    return id;
}

// ***************************************************************************
// MARK: - Sleep Proxy Server

mDNSlocal void RestartARPProbing(mDNS *const m, AuthRecord *const rr)
{
    // If we see an ARP from a machine we think is sleeping, then either
    // (i) the machine has woken, or
    // (ii) it's just a stray old packet from before the machine slept
    // To handle the second case, we reset ProbeCount, so we'll suppress our own answers for a while, to avoid
    // generating ARP conflicts with a waking machine, and set rr->LastAPTime so we'll start probing again in 10 seconds.
    // If the machine has just woken then we'll discard our records when we see the first new mDNS probe from that machine.
    // If it was a stray old packet, then after 10 seconds we'll probe again and then start answering ARPs again. In this case we *do*
    // need to send new ARP Announcements, because the owner's ARP broadcasts will have updated neighboring ARP caches, so we need to
    // re-assert our (temporary) ownership of that IP address in order to receive subsequent packets addressed to that IPv4 address.

    rr->resrec.RecordType = kDNSRecordTypeUnique;
    rr->ProbeCount        = DefaultProbeCountForTypeUnique;
    rr->ProbeRestartCount++;

    // If we haven't started announcing yet (and we're not already in ten-second-delay mode) the machine is probably
    // still going to sleep, so we just reset rr->ProbeCount so we'll continue probing until it stops responding.
    // If we *have* started announcing, the machine is probably in the process of waking back up, so in that case
    // we're more cautious and we wait ten seconds before probing it again. We do this because while waking from
    // sleep, some network interfaces tend to lose or delay inbound packets, and without this delay, if the waking machine
    // didn't answer our three probes within three seconds then we'd announce and cause it an unnecessary address conflict.
    if (rr->AnnounceCount == InitialAnnounceCount && m->timenow - rr->LastAPTime >= 0)
        InitializeLastAPTime(m, rr);
    else
    {
        rr->AnnounceCount  = InitialAnnounceCount;
        rr->ThisAPInterval = mDNSPlatformOneSecond;
        rr->LastAPTime     = m->timenow + mDNSPlatformOneSecond * 9;    // Send first packet at rr->LastAPTime + rr->ThisAPInterval, i.e. 10 seconds from now
        SetNextAnnounceProbeTime(m, rr);
    }
}

mDNSlocal void mDNSCoreReceiveRawARP(mDNS *const m, const ARP_EthIP *const arp, const mDNSInterfaceID InterfaceID)
{
    static const mDNSOpaque16 ARP_op_request = { { 0, 1 } };
    AuthRecord *rr;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, InterfaceID);
    if (!intf) return;

    mDNS_Lock(m);

    // Pass 1:
    // Process ARP Requests and Probes (but not Announcements), and generate an ARP Reply if necessary.
    // We also process ARPs from our own kernel (and 'answer' them by injecting a local ARP table entry)
    // We ignore ARP Announcements here -- Announcements are not questions, they're assertions, so we don't need to answer them.
    // The times we might need to react to an ARP Announcement are:
    // (i) as an indication that the host in question has not gone to sleep yet (so we should delay beginning to proxy for it) or
    // (ii) if it's a conflicting Announcement from another host
    // -- and we check for these in Pass 2 below.
    if (mDNSSameOpaque16(arp->op, ARP_op_request) && !mDNSSameIPv4Address(arp->spa, arp->tpa))
    {
        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->resrec.InterfaceID == InterfaceID && rr->resrec.RecordType != kDNSRecordTypeDeregistering &&
                rr->AddressProxy.type == mDNSAddrType_IPv4 && mDNSSameIPv4Address(rr->AddressProxy.ip.v4, arp->tpa))
            {
                static const char msg1[] = "ARP Req from owner -- re-probing";
                static const char msg2[] = "Ignoring  ARP Request from      ";
                static const char msg3[] = "Creating Local ARP Cache entry  ";
                static const char msg4[] = "Answering ARP Request from      ";
                const char *const msg = mDNSSameEthAddress(&arp->sha, &rr->WakeUp.IMAC) ? msg1 :
                                        (rr->AnnounceCount == InitialAnnounceCount)     ? msg2 :
                                        mDNSSameEthAddress(&arp->sha, &intf->MAC)       ? msg3 : msg4;
                LogMsg("Arp %-7s %s %.6a %.4a for %.4a -- H-MAC %.6a I-MAC %.6a %s",
                       intf->ifname, msg, arp->sha.b, arp->spa.b, arp->tpa.b,
                       &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                if (msg == msg1)
                {
                    if ( rr->ProbeRestartCount < MAX_PROBE_RESTARTS)
                        RestartARPProbing(m, rr);
                    else
                        LogSPS("Reached maximum number of restarts for probing - %s", ARDisplayString(m,rr));
                }
                else if (msg == msg3)
                {
                    mDNSPlatformSetLocalAddressCacheEntry(&rr->AddressProxy, &rr->WakeUp.IMAC, InterfaceID);
                }
                else if (msg == msg4)
                {
                    const mDNSv4Addr tpa = arp->tpa;
                    const mDNSv4Addr spa = arp->spa;
                    SendARP(m, 2, rr, &tpa, &arp->sha, &spa, &arp->sha);
                }
            }
    }

    // Pass 2:
    // For all types of ARP packet we check the Sender IP address to make sure it doesn't conflict with any AddressProxy record we're holding.
    // (Strictly speaking we're only checking Announcement/Request/Reply packets, since ARP Probes have zero Sender IP address,
    // so by definition (and by design) they can never conflict with any real (i.e. non-zero) IP address).
    // We ignore ARPs we sent ourselves (Sender MAC address is our MAC address) because our own proxy ARPs do not constitute a conflict that we need to handle.
    // If we see an apparently conflicting ARP, we check the sender hardware address:
    //   If the sender hardware address is the original owner this is benign, so we just suppress our own proxy answering for a while longer.
    //   If the sender hardware address is *not* the original owner, then this is a conflict, and we need to wake the sleeping machine to handle it.
    if (mDNSSameEthAddress(&arp->sha, &intf->MAC))
        debugf("ARP from self for %.4a", arp->tpa.b);
    else
    {
        if (!mDNSSameIPv4Address(arp->spa, zerov4Addr))
            for (rr = m->ResourceRecords; rr; rr=rr->next)
                if (rr->resrec.InterfaceID == InterfaceID && rr->resrec.RecordType != kDNSRecordTypeDeregistering &&
                    rr->AddressProxy.type == mDNSAddrType_IPv4 && mDNSSameIPv4Address(rr->AddressProxy.ip.v4, arp->spa) && (rr->ProbeRestartCount < MAX_PROBE_RESTARTS))
                {
                    if (mDNSSameEthAddress(&zeroEthAddr, &rr->WakeUp.HMAC))
                    {
                        LogMsg("%-7s ARP from %.6a %.4a for %.4a -- Invalid H-MAC %.6a I-MAC %.6a %s", intf->ifname,
                                arp->sha.b, arp->spa.b, arp->tpa.b, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                    }
                    else
                    {
                        RestartARPProbing(m, rr);
                        if (mDNSSameEthAddress(&arp->sha, &rr->WakeUp.IMAC))
                        {
                            LogMsg("%-7s ARP %s from owner %.6a %.4a for %-15.4a -- re-starting probing for %s", intf->ifname,
                                    mDNSSameIPv4Address(arp->spa, arp->tpa) ? "Announcement " : mDNSSameOpaque16(arp->op, ARP_op_request) ? "Request      " : "Response     ",
                                    arp->sha.b, arp->spa.b, arp->tpa.b, ARDisplayString(m, rr));
                        }
                        else
                        {
                            LogMsg("%-7s Conflicting ARP from %.6a %.4a for %.4a -- waking H-MAC %.6a I-MAC %.6a %s", intf->ifname,
                                    arp->sha.b, arp->spa.b, arp->tpa.b, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                            ScheduleWakeup(m, rr->resrec.InterfaceID, &rr->WakeUp.HMAC);
                        }
                    }
                }
    }

    mDNS_Unlock(m);
}

/*
   // Option 1 is Source Link Layer Address Option
   // Option 2 is Target Link Layer Address Option
   mDNSlocal const mDNSEthAddr *GetLinkLayerAddressOption(const IPv6NDP *const ndp, const mDNSu8 *const end, mDNSu8 op)
    {
    const mDNSu8 *options = (mDNSu8 *)(ndp+1);
    while (options < end)
        {
        debugf("NDP Option %02X len %2d %d", options[0], options[1], end - options);
        if (options[0] == op && options[1] == 1) return (const mDNSEthAddr*)(options+2);
        options += options[1] * 8;
        }
    return mDNSNULL;
    }
 */

mDNSlocal void mDNSCoreReceiveRawND(mDNS *const m, const mDNSEthAddr *const sha, const mDNSv6Addr *spa,
                                    const IPv6NDP *const ndp, const mDNSu8 *const end, const mDNSInterfaceID InterfaceID)
{
    AuthRecord *rr;
    NetworkInterfaceInfo *intf = FirstInterfaceForID(m, InterfaceID);
    if (!intf) return;

    mDNS_Lock(m);

    // Pass 1: Process Neighbor Solicitations, and generate a Neighbor Advertisement if necessary.
    if (ndp->type == NDP_Sol)
    {
        //const mDNSEthAddr *const sha = GetLinkLayerAddressOption(ndp, end, NDP_SrcLL);
        (void)end;
        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->resrec.InterfaceID == InterfaceID && rr->resrec.RecordType != kDNSRecordTypeDeregistering &&
                rr->AddressProxy.type == mDNSAddrType_IPv6 && mDNSSameIPv6Address(rr->AddressProxy.ip.v6, ndp->target))
            {
                static const char msg1[] = "NDP Req from owner -- re-probing";
                static const char msg2[] = "Ignoring  NDP Request from      ";
                static const char msg3[] = "Creating Local NDP Cache entry  ";
                static const char msg4[] = "Answering NDP Request from      ";
                static const char msg5[] = "Answering NDP Probe   from      ";
                const char *const msg = mDNSSameEthAddress(sha, &rr->WakeUp.IMAC)   ? msg1 :
                                        (rr->AnnounceCount == InitialAnnounceCount) ? msg2 :
                                        mDNSSameEthAddress(sha, &intf->MAC)         ? msg3 :
                                        mDNSIPv6AddressIsZero(*spa)                 ? msg4 : msg5;
                LogSPS("%-7s %s %.6a %.16a for %.16a -- H-MAC %.6a I-MAC %.6a %s",
                       intf->ifname, msg, sha, spa, &ndp->target, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                if (msg == msg1)
                {
                    if (rr->ProbeRestartCount < MAX_PROBE_RESTARTS)
                        RestartARPProbing(m, rr);
                    else
                        LogSPS("Reached maximum number of restarts for probing - %s", ARDisplayString(m,rr));
                }
                else if (msg == msg3)
                    mDNSPlatformSetLocalAddressCacheEntry(&rr->AddressProxy, &rr->WakeUp.IMAC, InterfaceID);
                else if (msg == msg4)
                    SendNDP(m, NDP_Adv, NDP_Solicited, rr, &ndp->target, mDNSNULL, spa, sha);
                else if (msg == msg5)
                    SendNDP(m, NDP_Adv, 0, rr, &ndp->target, mDNSNULL, &AllHosts_v6, &AllHosts_v6_Eth);
            }
    }

    // Pass 2: For all types of NDP packet we check the Sender IP address to make sure it doesn't conflict with any AddressProxy record we're holding.
    if (mDNSSameEthAddress(sha, &intf->MAC))
        debugf("NDP from self for %.16a", &ndp->target);
    else
    {
        // For Neighbor Advertisements we check the Target address field, not the actual IPv6 source address.
        // When a machine has both link-local and routable IPv6 addresses, it may send NDP packets making assertions
        // about its routable IPv6 address, using its link-local address as the source address for all NDP packets.
        // Hence it is the NDP target address we care about, not the actual packet source address.
        if (ndp->type == NDP_Adv) spa = &ndp->target;
        if (!mDNSSameIPv6Address(*spa, zerov6Addr))
            for (rr = m->ResourceRecords; rr; rr=rr->next)
                if (rr->resrec.InterfaceID == InterfaceID && rr->resrec.RecordType != kDNSRecordTypeDeregistering &&
                    rr->AddressProxy.type == mDNSAddrType_IPv6 && mDNSSameIPv6Address(rr->AddressProxy.ip.v6, *spa) && (rr->ProbeRestartCount < MAX_PROBE_RESTARTS))
                {
                    if (mDNSSameEthAddress(&zeroEthAddr, &rr->WakeUp.HMAC))
                    {
                        LogSPS("%-7s NDP from %.6a %.16a for %.16a -- Invalid H-MAC %.6a I-MAC %.6a %s", intf->ifname,
                                    sha, spa, &ndp->target, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                    }
                    else
                    {
                        RestartARPProbing(m, rr);
                        if (mDNSSameEthAddress(sha, &rr->WakeUp.IMAC))
                        {
                            LogSPS("%-7s NDP %s from owner %.6a %.16a for %.16a -- re-starting probing for %s", intf->ifname,
                                    ndp->type == NDP_Sol ? "Solicitation " : "Advertisement", sha, spa, &ndp->target, ARDisplayString(m, rr));
                        }
                        else
                        {
                            LogMsg("%-7s Conflicting NDP from %.6a %.16a for %.16a -- waking H-MAC %.6a I-MAC %.6a %s", intf->ifname,
                                    sha, spa, &ndp->target, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, rr));
                            ScheduleWakeup(m, rr->resrec.InterfaceID, &rr->WakeUp.HMAC);
                        }
                    }
                }
    }

    mDNS_Unlock(m);
}

mDNSlocal void mDNSCoreReceiveRawTransportPacket(mDNS *const m, const mDNSEthAddr *const sha, const mDNSAddr *const src, const mDNSAddr *const dst, const mDNSu8 protocol,
                                                 const mDNSu8 *const p, const TransportLayerPacket *const t, const mDNSu8 *const end, const mDNSInterfaceID InterfaceID, const mDNSu16 len)
{
    const mDNSIPPort port = (protocol == 0x06) ? t->tcp.dst : (protocol == 0x11) ? t->udp.dst : zeroIPPort;
    mDNSBool wake = mDNSfalse;
    mDNSBool kaWake = mDNSfalse;

    switch (protocol)
    {
        #define XX wake ? "Received" : "Ignoring", end-p
    case 0x01:  LogSPS("Ignoring %d-byte ICMP from %#a to %#a", end-p, src, dst);
        break;

    case 0x06:  {
        AuthRecord *kr;
        mDNSu32 seq, ack;
                    #define TH_FIN  0x01
                    #define TH_SYN  0x02
                    #define TH_RST  0x04
                    #define TH_ACK  0x10

        kr = mDNS_MatchKeepaliveInfo(m, dst, src, port, t->tcp.src, &seq, &ack);
        if (kr)
        {
            LogSPS("mDNSCoreReceiveRawTransportPacket: Found a Keepalive record from %#a:%d  to %#a:%d", src, mDNSVal16(t->tcp.src), dst, mDNSVal16(port));
            // Plan to wake if
            // (a) RST or FIN is set (the keepalive that we sent could have caused a reset)
            // (b) packet that contains new data and acks a sequence number higher than the one
            //     we have been sending in the keepalive

            wake = ((t->tcp.flags & TH_RST) || (t->tcp.flags & TH_FIN)) ;
            if (!wake)
            {
                mDNSu8 *ptr;
                mDNSu32 pseq, pack;
                mDNSBool data = mDNSfalse;
                mDNSu8 tcphlen;

                // Convert to host order
                ptr = (mDNSu8 *)&seq;
                seq = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];

                ptr = (mDNSu8 *)&ack;
                ack = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];

                pseq = t->tcp.seq;
                ptr = (mDNSu8 *)&pseq;
                pseq = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];

                pack = t->tcp.ack;
                ptr = (mDNSu8 *)&pack;
                pack = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];

                // If the other side is acking one more than our sequence number (keepalive is one
                // less than the last valid sequence sent) and it's sequence is more than what we
                // acked before
                //if (end - p - 34  - ((t->tcp.offset >> 4) * 4) > 0) data = mDNStrue;
                tcphlen = ((t->tcp.offset >> 4) * 4);
                if (end - ((const mDNSu8 *)t + tcphlen) > 0) data = mDNStrue;
                wake = ((int)(pack - seq) > 0) && ((int)(pseq - ack) >= 0) && data;

                // If we got a regular keepalive on a connection that was registed with the KeepAlive API, respond with an ACK
                if ((t->tcp.flags & TH_ACK) && (data == mDNSfalse) &&
                    ((int)(ack - pseq) == 1))
                {
                    // Send an ACK;
                    mDNS_SendKeepaliveACK(m, kr);
                }
                LogSPS("mDNSCoreReceiveRawTransportPacket: End %p, hlen %d, Datalen %d, pack %u, seq %u, pseq %u, ack %u, wake %d",
                       end, tcphlen, end - ((const mDNSu8 *)t + tcphlen), pack, seq, pseq, ack, wake);
            }
            else { LogSPS("mDNSCoreReceiveRawTransportPacket: waking because of RST or FIN th_flags %d", t->tcp.flags); }
            kaWake = wake;
        }
        else
        {
            // Plan to wake if
            // (a) RST is not set, AND
            // (b) packet is SYN, SYN+FIN, or plain data packet (no SYN or FIN). We won't wake for FIN alone.
            wake = (!(t->tcp.flags & TH_RST) && (t->tcp.flags & (TH_FIN|TH_SYN)) != TH_FIN);

            // For now, to reduce spurious wakeups, we wake only for TCP SYN,
            // except for ssh connections, where we'll wake for plain data packets too
            if  (!mDNSSameIPPort(port, SSHPort) && !(t->tcp.flags & 2)) wake = mDNSfalse;

            LogSPS("%s %d-byte TCP from %#a:%d to %#a:%d%s%s%s", XX,
                   src, mDNSVal16(t->tcp.src), dst, mDNSVal16(port),
                   (t->tcp.flags & 2) ? " SYN" : "",
                   (t->tcp.flags & 1) ? " FIN" : "",
                   (t->tcp.flags & 4) ? " RST" : "");
        }
        break;
    }

    case 0x11:  {
                    #define ARD_AsNumber 3283
        static const mDNSIPPort ARD = { { ARD_AsNumber >> 8, ARD_AsNumber & 0xFF } };
        const mDNSu16 udplen = (mDNSu16)((mDNSu16)t->bytes[4] << 8 | t->bytes[5]);                  // Length *including* 8-byte UDP header
        if (udplen >= sizeof(UDPHeader))
        {
            const mDNSu16 datalen = udplen - sizeof(UDPHeader);
            wake = mDNStrue;

            // For Back to My Mac UDP port 4500 (IPSEC) packets, we do some special handling
            if (mDNSSameIPPort(port, IPSECPort))
            {
                // Specifically ignore NAT keepalive packets
                if (datalen == 1 && end >= &t->bytes[9] && t->bytes[8] == 0xFF) wake = mDNSfalse;
                else
                {
                    // Skip over the Non-ESP Marker if present
                    const mDNSBool NonESP = (end >= &t->bytes[12] && t->bytes[8] == 0 && t->bytes[9] == 0 && t->bytes[10] == 0 && t->bytes[11] == 0);
                    const IKEHeader *const ike    = (const IKEHeader *)(t + (NonESP ? 12 : 8));
                    const mDNSu16 ikelen = datalen - (NonESP ? 4 : 0);
                    if (ikelen >= sizeof(IKEHeader) && end >= ((const mDNSu8 *)ike) + sizeof(IKEHeader))
                        if ((ike->Version & 0x10) == 0x10)
                        {
                            // ExchangeType ==  5 means 'Informational' <http://www.ietf.org/rfc/rfc2408.txt>
                            // ExchangeType == 34 means 'IKE_SA_INIT'   <http://www.iana.org/assignments/ikev2-parameters>
                            if (ike->ExchangeType == 5 || ike->ExchangeType == 34) wake = mDNSfalse;
                            LogSPS("%s %d-byte IKE ExchangeType %d", XX, ike->ExchangeType);
                        }
                }
            }

            // For now, because we haven't yet worked out a clean elegant way to do this, we just special-case the
            // Apple Remote Desktop port number -- we ignore all packets to UDP 3283 (the "Net Assistant" port),
            // except for Apple Remote Desktop's explicit manual wakeup packet, which looks like this:
            // UDP header (8 bytes)
            // Payload: 13 88 00 6a 41 4e 41 20 (8 bytes) ffffffffffff (6 bytes) 16xMAC (96 bytes) = 110 bytes total
            if (mDNSSameIPPort(port, ARD)) wake = (datalen >= 110 && end >= &t->bytes[10] && t->bytes[8] == 0x13 && t->bytes[9] == 0x88);

            LogSPS("%s %d-byte UDP from %#a:%d to %#a:%d", XX, src, mDNSVal16(t->udp.src), dst, mDNSVal16(port));
        }
    }
    break;

    case 0x3A:  if (&t->bytes[len] <= end)
        {
            mDNSu16 checksum = IPv6CheckSum(&src->ip.v6, &dst->ip.v6, protocol, t->bytes, len);
            if (!checksum) mDNSCoreReceiveRawND(m, sha, &src->ip.v6, &t->ndp, &t->bytes[len], InterfaceID);
            else LogInfo("IPv6CheckSum bad %04X %02X%02X from %#a to %#a", checksum, t->bytes[2], t->bytes[3], src, dst);
        }
        break;

    default:    LogSPS("Ignoring %d-byte IP packet unknown protocol %d from %#a to %#a", end-p, protocol, src, dst);
        break;
    }

    if (wake)
    {
        AuthRecord *rr, *r2;

        mDNS_Lock(m);
        for (rr = m->ResourceRecords; rr; rr=rr->next)
            if (rr->resrec.InterfaceID == InterfaceID &&
                rr->resrec.RecordType != kDNSRecordTypeDeregistering &&
                rr->AddressProxy.type && mDNSSameAddress(&rr->AddressProxy, dst))
            {
                const mDNSu8 *const tp = (protocol == 6) ? (const mDNSu8 *)"\x4_tcp" : (const mDNSu8 *)"\x4_udp";
                for (r2 = m->ResourceRecords; r2; r2=r2->next)
                    if (r2->resrec.InterfaceID == InterfaceID && mDNSSameEthAddress(&r2->WakeUp.HMAC, &rr->WakeUp.HMAC) &&
                        r2->resrec.RecordType != kDNSRecordTypeDeregistering &&
                        r2->resrec.rrtype == kDNSType_SRV && mDNSSameIPPort(r2->resrec.rdata->u.srv.port, port) &&
                        SameDomainLabel(ThirdLabel(r2->resrec.name)->c, tp))
                        break;
                if (!r2 && mDNSSameIPPort(port, IPSECPort)) r2 = rr;    // So that we wake for BTMM IPSEC packets, even without a matching SRV record
                if (!r2 && kaWake) r2 = rr;                             // So that we wake for keepalive packets, even without a matching SRV record
                if (r2)
                {
                    LogMsg("Waking host at %s %#a H-MAC %.6a I-MAC %.6a for %s",
                           InterfaceNameForID(m, rr->resrec.InterfaceID), dst, &rr->WakeUp.HMAC, &rr->WakeUp.IMAC, ARDisplayString(m, r2));
                    ScheduleWakeup(m, rr->resrec.InterfaceID, &rr->WakeUp.HMAC);
                }
                else
                    LogSPS("Sleeping host at %s %#a %.6a has no service on %#s %d",
                           InterfaceNameForID(m, rr->resrec.InterfaceID), dst, &rr->WakeUp.HMAC, tp, mDNSVal16(port));
            }
        mDNS_Unlock(m);
    }
}

mDNSexport void mDNSCoreReceiveRawPacket(mDNS *const m, const mDNSu8 *const p, const mDNSu8 *const end, const mDNSInterfaceID InterfaceID)
{
    static const mDNSOpaque16 Ethertype_ARP  = { { 0x08, 0x06 } };  // Ethertype 0x0806 = ARP
    static const mDNSOpaque16 Ethertype_IPv4 = { { 0x08, 0x00 } };  // Ethertype 0x0800 = IPv4
    static const mDNSOpaque16 Ethertype_IPv6 = { { 0x86, 0xDD } };  // Ethertype 0x86DD = IPv6
    static const mDNSOpaque16 ARP_hrd_eth    = { { 0x00, 0x01 } };  // Hardware address space (Ethernet = 1)
    static const mDNSOpaque16 ARP_pro_ip     = { { 0x08, 0x00 } };  // Protocol address space (IP = 0x0800)

    // Note: BPF guarantees that the NETWORK LAYER header will be word aligned, not the link-layer header.
    // In other words, we can safely assume that pkt below (ARP, IPv4 or IPv6) is properly word aligned,
    // but if pkt is 4-byte aligned, that necessarily means that eth CANNOT also be 4-byte aligned
    // since it points to a an address 14 bytes before pkt.
    const EthernetHeader     *const eth = (const EthernetHeader *)p;
    const NetworkLayerPacket *const pkt = (const NetworkLayerPacket *)(eth+1);
    mDNSAddr src, dst;
    #define RequiredCapLen(P) ((P)==0x01 ? 4 : (P)==0x06 ? 20 : (P)==0x11 ? 8 : (P)==0x3A ? 24 : 0)

    // Is ARP? Length must be at least 14 + 28 = 42 bytes
    if (end >= p+42 && mDNSSameOpaque16(eth->ethertype, Ethertype_ARP) && mDNSSameOpaque16(pkt->arp.hrd, ARP_hrd_eth) && mDNSSameOpaque16(pkt->arp.pro, ARP_pro_ip))
        mDNSCoreReceiveRawARP(m, &pkt->arp, InterfaceID);
    // Is IPv4 with zero fragmentation offset? Length must be at least 14 + 20 = 34 bytes
    else if (end >= p+34 && mDNSSameOpaque16(eth->ethertype, Ethertype_IPv4) && (pkt->v4.flagsfrags.b[0] & 0x1F) == 0 && pkt->v4.flagsfrags.b[1] == 0)
    {
        const mDNSu8 *const trans = p + 14 + (pkt->v4.vlen & 0xF) * 4;
        const mDNSu8 * transEnd = p + 14 + mDNSVal16(pkt->v4.totlen);
        if (transEnd > end) transEnd = end;
        debugf("Got IPv4 %02X from %.4a to %.4a", pkt->v4.protocol, &pkt->v4.src.b, &pkt->v4.dst.b);
        src.type = mDNSAddrType_IPv4; src.ip.v4 = pkt->v4.src;
        dst.type = mDNSAddrType_IPv4; dst.ip.v4 = pkt->v4.dst;
        if (transEnd >= trans + RequiredCapLen(pkt->v4.protocol))
            mDNSCoreReceiveRawTransportPacket(m, &eth->src, &src, &dst, pkt->v4.protocol, p, (const TransportLayerPacket*)trans, transEnd, InterfaceID, 0);
    }
    // Is IPv6? Length must be at least 14 + 28 = 42 bytes
    else if (end >= p+54 && mDNSSameOpaque16(eth->ethertype, Ethertype_IPv6))
    {
        const mDNSu8 *const trans = p + 54;
        debugf("Got IPv6  %02X from %.16a to %.16a", pkt->v6.pro, &pkt->v6.src.b, &pkt->v6.dst.b);
        src.type = mDNSAddrType_IPv6; src.ip.v6 = pkt->v6.src;
        dst.type = mDNSAddrType_IPv6; dst.ip.v6 = pkt->v6.dst;
        if (end >= trans + RequiredCapLen(pkt->v6.pro))
            mDNSCoreReceiveRawTransportPacket(m, &eth->src, &src, &dst, pkt->v6.pro, p, (const TransportLayerPacket*)trans, end, InterfaceID,
                                              (mDNSu16)(pkt->bytes[4] << 8) | pkt->bytes[5]);
    }
}

mDNSlocal void ConstructSleepProxyServerName(mDNS *const m, domainlabel *name)
{
    name->c[0] = (mDNSu8)mDNS_snprintf((char*)name->c+1, 62, "%d-%d-%d-%d.%d %#s",
                                       m->SPSType, m->SPSPortability, m->SPSMarginalPower, m->SPSTotalPower, m->SPSFeatureFlags, &m->nicelabel);
}

#ifndef SPC_DISABLED
mDNSlocal void SleepProxyServerCallback(mDNS *const m, ServiceRecordSet *const srs, mStatus result)
{
    if (result == mStatus_NameConflict)
        mDNS_RenameAndReregisterService(m, srs, mDNSNULL);
    else if (result == mStatus_MemFree)
    {
        if (m->SleepState)
            m->SPSState = 3;
        else
        {
            m->SPSState = (mDNSu8)(m->SPSSocket != mDNSNULL);
            if (m->SPSState)
            {
                domainlabel name;
                ConstructSleepProxyServerName(m, &name);
                mDNS_RegisterService(m, srs,
                                     &name, &SleepProxyServiceType, &localdomain,
                                     mDNSNULL, m->SPSSocket->port, // Host, port
                                     mDNSNULL,
                                     (mDNSu8 *)"", 1,           // TXT data, length
                                     mDNSNULL, 0,               // Subtypes (none)
                                     mDNSInterface_Any,         // Interface ID
                                     SleepProxyServerCallback, mDNSNULL, 0); // Callback, context, flags
            }
            LogSPS("Sleep Proxy Server %#s %s", srs->RR_SRV.resrec.name->c, m->SPSState ? "started" : "stopped");
        }
    }
}
#endif

// Called with lock held
mDNSexport void mDNSCoreBeSleepProxyServer_internal(mDNS *const m, mDNSu8 sps, mDNSu8 port, mDNSu8 marginalpower, mDNSu8 totpower, mDNSu8 features)
{
    // This routine uses mDNS_DeregisterService and calls SleepProxyServerCallback, so we execute in user callback context
    mDNS_DropLockBeforeCallback();

    // If turning off SPS, close our socket
    // (Do this first, BEFORE calling mDNS_DeregisterService below)
    if (!sps && m->SPSSocket) { mDNSPlatformUDPClose(m->SPSSocket); m->SPSSocket = mDNSNULL; }

    // If turning off, or changing type, deregister old name
#ifndef SPC_DISABLED
    if (m->SPSState == 1 && sps != m->SPSType)
    { m->SPSState = 2; mDNS_DeregisterService_drt(m, &m->SPSRecords, sps ? mDNS_Dereg_rapid : mDNS_Dereg_normal); }
#endif // SPC_DISABLED

    // Record our new SPS parameters
    m->SPSType          = sps;
    m->SPSPortability   = port;
    m->SPSMarginalPower = marginalpower;
    m->SPSTotalPower    = totpower;
    m->SPSFeatureFlags  = features;
    // If turning on, open socket and advertise service
    if (sps)
    {
        if (!m->SPSSocket)
        {
            m->SPSSocket = mDNSPlatformUDPSocket(zeroIPPort);
            if (!m->SPSSocket) { LogMsg("mDNSCoreBeSleepProxyServer: Failed to allocate SPSSocket"); goto fail; }
        }
#ifndef SPC_DISABLED
        if (m->SPSState == 0) SleepProxyServerCallback(m, &m->SPSRecords, mStatus_MemFree);
#endif // SPC_DISABLED
    }
    else if (m->SPSState)
    {
        LogSPS("mDNSCoreBeSleepProxyServer turning off from state %d; will wake clients", m->SPSState);
        m->NextScheduledSPS = m->timenow;
    }
fail:
    mDNS_ReclaimLockAfterCallback();
}

// ***************************************************************************
// MARK: - Startup and Shutdown

mDNSlocal void mDNS_GrowCache_internal(mDNS *const m, CacheEntity *storage, mDNSu32 numrecords)
{
    if (storage && numrecords)
    {
        mDNSu32 i;
        debugf("Adding cache storage for %d more records (%d bytes)", numrecords, numrecords*sizeof(CacheEntity));
        for (i=0; i<numrecords; i++) storage[i].next = &storage[i+1];
        storage[numrecords-1].next = m->rrcache_free;
        m->rrcache_free = storage;
        m->rrcache_size += numrecords;
    }
}

mDNSexport void mDNS_GrowCache(mDNS *const m, CacheEntity *storage, mDNSu32 numrecords)
{
    mDNS_Lock(m);
    mDNS_GrowCache_internal(m, storage, numrecords);
    mDNS_Unlock(m);
}

mDNSlocal mStatus mDNS_InitStorage(mDNS *const m, mDNS_PlatformSupport *const p,
                                   CacheEntity *rrcachestorage, mDNSu32 rrcachesize,
                                   mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context)
{
    mDNSu32 slot;
    mDNSs32 timenow;
    mStatus result;

    if (!rrcachestorage) rrcachesize = 0;

    m->p                             = p;
    m->NetworkChanged                = 0;
    m->CanReceiveUnicastOn5353       = mDNSfalse; // Assume we can't receive unicasts on 5353, unless platform layer tells us otherwise
    m->AdvertiseLocalAddresses       = AdvertiseLocalAddresses;
    m->DivertMulticastAdvertisements = mDNSfalse;
    m->mDNSPlatformStatus            = mStatus_Waiting;
    m->UnicastPort4                  = zeroIPPort;
    m->UnicastPort6                  = zeroIPPort;
    m->PrimaryMAC                    = zeroEthAddr;
    m->MainCallback                  = Callback;
    m->MainContext                   = Context;
    m->rec.r.resrec.RecordType       = 0;

    // For debugging: To catch and report locking failures
    m->mDNS_busy               = 0;
    m->mDNS_reentrancy         = 0;
    m->ShutdownTime            = 0;
    m->lock_rrcache            = 0;
    m->lock_Questions          = 0;
    m->lock_Records            = 0;

    // Task Scheduling variables
    result = mDNSPlatformTimeInit();
    if (result != mStatus_NoError) return(result);
    m->timenow_adjust = (mDNSs32)mDNSRandom(0xFFFFFFFF);
    timenow = mDNS_TimeNow_NoLock(m);

    m->timenow                 = 0;     // MUST only be set within mDNS_Lock/mDNS_Unlock section
    m->timenow_last            = timenow;
    m->NextScheduledEvent      = timenow;
    m->SuppressQueries         = timenow;
    m->SuppressResponses       = timenow;
    m->NextCacheCheck          = timenow + FutureTime;
    m->NextScheduledQuery      = timenow + FutureTime;
    m->NextScheduledProbe      = timenow + FutureTime;
    m->NextScheduledResponse   = timenow + FutureTime;
    m->NextScheduledNATOp      = timenow + FutureTime;
    m->NextScheduledSPS        = timenow + FutureTime;
    m->NextScheduledKA         = timenow + FutureTime;
    m->NextScheduledStopTime   = timenow + FutureTime;
    m->NextBLEServiceTime      = 0;    // zero indicates inactive

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    m->NextBonjourDisableTime  = 0; // Timer active when non zero.
    m->BonjourEnabled          = 0; // Set when Bonjour on Demand is enabled and Bonjour is currently enabled.
#endif

    m->RandomQueryDelay        = 0;
    m->RandomReconfirmDelay    = 0;
    m->PktNum                  = 0;
    m->MPktNum                 = 0;
    m->LocalRemoveEvents       = mDNSfalse;
    m->SleepState              = SleepState_Awake;
    m->SleepSeqNum             = 0;
    m->SystemWakeOnLANEnabled  = mDNSfalse;
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    m->SentSleepProxyRegistration = mDNSfalse;
    m->AnnounceOwner           = NonZeroTime(timenow + 60 * mDNSPlatformOneSecond);
#endif
    m->DelaySleep              = 0;
    m->SleepLimit              = 0;


    // These fields only required for mDNS Searcher...
    m->Questions               = mDNSNULL;
    m->NewQuestions            = mDNSNULL;
    m->CurrentQuestion         = mDNSNULL;
    m->LocalOnlyQuestions      = mDNSNULL;
    m->NewLocalOnlyQuestions   = mDNSNULL;
    m->RestartQuestion         = mDNSNULL;
    m->rrcache_size            = 0;
    m->rrcache_totalused       = 0;
    m->rrcache_active          = 0;
    m->rrcache_report          = 10;
    m->rrcache_free            = mDNSNULL;

    for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
    {
        m->rrcache_hash[slot]      = mDNSNULL;
        m->rrcache_nextcheck[slot] = timenow + FutureTime;;
    }

    mDNS_GrowCache_internal(m, rrcachestorage, rrcachesize);
    m->rrauth.rrauth_free            = mDNSNULL;

    for (slot = 0; slot < AUTH_HASH_SLOTS; slot++)
        m->rrauth.rrauth_hash[slot] = mDNSNULL;

    // Fields below only required for mDNS Responder...
    m->hostlabel.c[0]          = 0;
    m->nicelabel.c[0]          = 0;
    m->MulticastHostname.c[0]  = 0;
#if MDNSRESPONDER_SUPPORTS(APPLE, RANDOM_AWDL_HOSTNAME)
    m->RandomizedHostname.c[0] = 0;
#endif
    m->HIHardware.c[0]         = 0;
    m->HISoftware.c[0]         = 0;
    m->ResourceRecords         = mDNSNULL;
    m->DuplicateRecords        = mDNSNULL;
    m->NewLocalRecords         = mDNSNULL;
    m->NewLocalOnlyRecords     = mDNSfalse;
    m->CurrentRecord           = mDNSNULL;
    m->HostInterfaces          = mDNSNULL;
    m->ProbeFailTime           = 0;
    m->NumFailedProbes         = 0;
    m->SuppressProbes          = 0;

#ifndef UNICAST_DISABLED
    m->NextuDNSEvent            = timenow + FutureTime;
    m->NextSRVUpdate            = timenow + FutureTime;

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    m->DNSServers               = mDNSNULL;
#endif

    m->Router                   = zeroAddr;
    m->AdvertisedV4             = zeroAddr;
    m->AdvertisedV6             = zeroAddr;

    m->AuthInfoList             = mDNSNULL;

    m->ReverseMap.ThisQInterval = -1;
    m->StaticHostname.c[0]      = 0;
    m->FQDN.c[0]                = 0;
    m->Hostnames                = mDNSNULL;

    m->WABBrowseQueriesCount    = 0;
    m->WABLBrowseQueriesCount   = 0;
    m->WABRegQueriesCount       = 0;
    m->AutoTargetServices       = 1;

#if MDNSRESPONDER_SUPPORTS(APPLE, BONJOUR_ON_DEMAND)
    m->NumAllInterfaceRecords   = 0;
    m->NumAllInterfaceQuestions = 0;
#endif
    // NAT traversal fields
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
    m->LLQNAT.clientCallback    = mDNSNULL;
    m->LLQNAT.clientContext     = mDNSNULL;
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
    m->NATTraversals            = mDNSNULL;
    m->CurrentNATTraversal      = mDNSNULL;
    m->retryIntervalGetAddr     = 0;    // delta between time sent and retry
    m->retryGetAddr             = timenow + FutureTime; // absolute time when we retry
    m->ExtAddress               = zerov4Addr;
    m->PCPNonce[0]              = mDNSRandom(-1);
    m->PCPNonce[1]              = mDNSRandom(-1);
    m->PCPNonce[2]              = mDNSRandom(-1);

    m->NATMcastRecvskt          = mDNSNULL;
    m->LastNATupseconds         = 0;
    m->LastNATReplyLocalTime    = timenow;
    m->LastNATMapResultCode     = NATErr_None;

    m->UPnPInterfaceID          = 0;
    m->SSDPSocket               = mDNSNULL;
    m->SSDPWANPPPConnection     = mDNSfalse;
    m->UPnPRouterPort           = zeroIPPort;
    m->UPnPSOAPPort             = zeroIPPort;
    m->UPnPRouterURL            = mDNSNULL;
    m->UPnPWANPPPConnection     = mDNSfalse;
    m->UPnPSOAPURL              = mDNSNULL;
    m->UPnPRouterAddressString  = mDNSNULL;
    m->UPnPSOAPAddressString    = mDNSNULL;
    m->SPSType                  = 0;
    m->SPSPortability           = 0;
    m->SPSMarginalPower         = 0;
    m->SPSTotalPower            = 0;
    m->SPSFeatureFlags          = 0;
    m->SPSState                 = 0;
    m->SPSProxyListChanged      = mDNSNULL;
    m->SPSSocket                = mDNSNULL;
#if MDNSRESPONDER_SUPPORTS(COMMON, SPS_CLIENT)
    m->SPSBrowseCallback        = mDNSNULL;
#endif
    m->ProxyRecords             = 0;

    m->DNSPushServers           = mDNSNULL;
    m->DNSPushZones             = mDNSNULL;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, WEB_CONTENT_FILTER)
    if (WCFConnectionNew)
    {
        m->WCF = WCFConnectionNew();
        if (!m->WCF) { LogMsg("WCFConnectionNew failed"); return -1; }
    }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    // Currently we only support the root trust anchor.
    dnssec_error_t dnssec_err;
    m->DNSSECTrustAnchorManager = dnssec_obj_trust_anchor_manager_with_root_anchor_create(&dnssec_err);
    if (dnssec_err != DNSSEC_ERROR_NO_ERROR)
    {
        return mStatus_UnknownErr;
    }
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
    // The first mDNS report will be generated in "RuntimeMDNSMetricsReportInterval"s.
    m->NextMDNSResponseDelayReport = NonZeroTime(timenow + RuntimeMDNSMetricsReportInterval);
#endif

    // Set .local domain as one of the domain that can do domain enumeration.
    DomainEnumerationOp *const dotLocalDomainToDoEnumeration = mDNSPlatformMemAllocateClear(sizeof(*dotLocalDomainToDoEnumeration));
    if (dotLocalDomainToDoEnumeration != mDNSNULL)
    {
        AssignDomainName(&dotLocalDomainToDoEnumeration->name, &localdomain);
        m->domainsToDoEnumeration = dotLocalDomainToDoEnumeration;
    }

    return(result);
}

mDNSexport mStatus mDNS_Init(mDNS *const m, mDNS_PlatformSupport *const p,
                             CacheEntity *rrcachestorage, mDNSu32 rrcachesize,
                             mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context)
{
    mStatus result = mDNS_InitStorage(m, p, rrcachestorage, rrcachesize, AdvertiseLocalAddresses, Callback, Context);
    if (result != mStatus_NoError)
        return(result);

#if MDNS_MALLOC_DEBUGGING
    static mDNSListValidator lv;
    mDNSPlatformAddListValidator(&lv, mDNS_ValidateLists, "mDNS_ValidateLists", m);
#endif
    result = mDNSPlatformInit(m);

#ifndef UNICAST_DISABLED
    // It's better to do this *after* the platform layer has set up the
    // interface list and security credentials
    uDNS_SetupDNSConfig(m);                     // Get initial DNS configuration
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DISCOVERY_PROXY_CLIENT)
    if (DPCFeatureEnabled())
    {
        mDNSPlatformMemZero(&DPCBrowse, sizeof(DPCBrowse));
        // Note: ConstructServiceName(), which is called by mDNS_StartBrowse_internal(), will turn
        // "_local._dnssd-dp._tcp" to "_local._sub._dnssd-dp._tcp". See <rdar://3588761>.
        const domainname *const serviceType = (const domainname *)"\x6" "_local" "\x9" "_dnssd-dp" "\x4" "_tcp";
        mDNS_StartBrowse_internal(m, &DPCBrowse, serviceType, &localdomain, mDNSInterface_Any, 0, mDNSfalse, mDNSfalse,
            DPCBrowseHandler, mDNSNULL);
        // Set querying interval to -1 to avoid scheduling queries for the browse. They'll instead be sent
        // opportunistically along with other non-probe questions.
        DPCBrowse.ThisQInterval = -1;
    }
#endif
    return(result);
}

mDNSexport void mDNS_ConfigChanged(mDNS *const m)
{
    if (m->SPSState == 1)
    {
        domainlabel name, newname;
#ifndef SPC_DISABLED
        domainname type, domain;
        DeconstructServiceName(m->SPSRecords.RR_SRV.resrec.name, &name, &type, &domain);
#endif // SPC_DISABLED
        ConstructSleepProxyServerName(m, &newname);
        if (!SameDomainLabelCS(name.c, newname.c))
        {
            LogSPS("Renaming SPS from “%#s” to “%#s”", name.c, newname.c);
            // When SleepProxyServerCallback gets the mStatus_MemFree message,
            // it will reregister the service under the new name
            m->SPSState = 2;
#ifndef SPC_DISABLED
            mDNS_DeregisterService_drt(m, &m->SPSRecords, mDNS_Dereg_rapid);
#endif // SPC_DISABLED
        }
    }

    if (m->MainCallback)
        m->MainCallback(m, mStatus_ConfigChanged);
}

mDNSlocal void DynDNSHostNameCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
    (void)m;    // unused
    debugf("NameStatusCallback: result %d for registration of name %##s", result, rr->resrec.name->c);
    mDNSPlatformDynDNSHostNameStatusChanged(rr->resrec.name, result);
}

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSlocal void PurgeOrReconfirmCacheRecord(mDNS *const m, CacheRecord *cr)
{
    mDNSBool purge = cr->resrec.RecordType == kDNSRecordTypePacketNegative ||
                     cr->resrec.rrtype     == kDNSType_A ||
                     cr->resrec.rrtype     == kDNSType_AAAA ||
                     cr->resrec.rrtype     == kDNSType_SRV ||
                     cr->resrec.rrtype     == kDNSType_CNAME;

    debugf("PurgeOrReconfirmCacheRecord: %s cache record due to server %#a:%d (%##s): %s",
           purge    ? "purging"   : "reconfirming",
           cr->resrec.rDNSServer ? &cr->resrec.rDNSServer->addr : mDNSNULL,
           cr->resrec.rDNSServer ? mDNSVal16(cr->resrec.rDNSServer->port) : -1,
           cr->resrec.rDNSServer ? cr->resrec.rDNSServer->domain.c : mDNSNULL, CRDisplayString(m, cr));

    if (purge)
    {
        LogInfo("PurgeorReconfirmCacheRecord: Purging Resourcerecord %s, RecordType %x", CRDisplayString(m, cr), cr->resrec.RecordType);
        mDNS_PurgeCacheResourceRecord(m, cr);
    }
    else
    {
        LogInfo("PurgeorReconfirmCacheRecord: Reconfirming Resourcerecord %s, RecordType %x", CRDisplayString(m, cr), cr->resrec.RecordType);
        mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
    }
}
#endif

mDNSlocal void mDNS_PurgeBeforeResolve(mDNS *const m, DNSQuestion *q)
{
    CacheGroup *const cg = CacheGroupForName(m, q->qnamehash, &q->qname);
    CacheRecord *rp;
    for (rp = cg ? cg->members : mDNSNULL; rp; rp = rp->next)
    {
        if (SameNameCacheRecordAnswersQuestion(rp, q))
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_PurgeBeforeResolve: Flushing " PRI_S, CRDisplayString(m, rp));
            mDNS_PurgeCacheResourceRecord(m, rp);
        }
    }
}

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
mDNSexport void DNSServerChangeForQuestion(mDNS *const m, DNSQuestion *q, DNSServer *new)
{
    DNSQuestion *qptr;

    (void) m;

    if (q->DuplicateOf)
        LogMsg("DNSServerChangeForQuestion: ERROR: Called for duplicate question %##s", q->qname.c);

    // Make sure all the duplicate questions point to the same DNSServer so that delivery
    // of events for all of them are consistent. Duplicates for a question are always inserted
    // after in the list.
    q->qDNSServer = new;
    for (qptr = q->next ; qptr; qptr = qptr->next)
    {
        if (qptr->DuplicateOf == q) { qptr->validDNSServers = q->validDNSServers; qptr->qDNSServer = new; }
    }
}
#endif

mDNSlocal void SetConfigState(mDNS *const m, mDNSBool delete)
{
    McastResolver *mr;
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    DNSServer *ptr;
#endif

    if (delete)
    {
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        for (ptr = m->DNSServers; ptr; ptr = ptr->next)
        {
            ptr->penaltyTime = 0;
            ptr->flags |= DNSServerFlag_Delete;
#if MDNSRESPONDER_SUPPORTS(APPLE, SYMPTOMS)
            if (ptr->flags & DNSServerFlag_Unreachable)
                NumUnreachableDNSServers--;
#endif
        }
#endif
        // We handle the mcast resolvers here itself as mDNSPlatformSetDNSConfig looks at
        // mcast resolvers. Today we get both mcast and ucast configuration using the same
        // API
        for (mr = m->McastResolvers; mr; mr = mr->next)
            mr->flags |= McastResolver_FlagDelete;
    }
    else
    {
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
        for (ptr = m->DNSServers; ptr; ptr = ptr->next)
        {
            ptr->penaltyTime = 0;
            ptr->flags &= ~DNSServerFlag_Delete;
#if MDNSRESPONDER_SUPPORTS(APPLE, SYMPTOMS)
            if (ptr->flags & DNSServerFlag_Unreachable)
                NumUnreachableDNSServers++;
#endif
        }
#endif
        for (mr = m->McastResolvers; mr; mr = mr->next)
            mr->flags &= ~McastResolver_FlagDelete;
    }
}

mDNSlocal void SetDynDNSHostNameIfChanged(mDNS *const m, domainname *const fqdn)
{
    // Did our FQDN change?
    if (!SameDomainName(fqdn, &m->FQDN))
    {
        if (m->FQDN.c[0]) mDNS_RemoveDynDNSHostName(m, &m->FQDN);

        AssignDomainName(&m->FQDN, fqdn);

        if (m->FQDN.c[0])
        {
            mDNSPlatformDynDNSHostNameStatusChanged(&m->FQDN, 1);
            mDNS_AddDynDNSHostName(m, &m->FQDN, DynDNSHostNameCallback, mDNSNULL);
        }
    }
}

// Even though this is called “Setup” it is not called just once at startup.
// It’s actually called multiple times, every time there’s a configuration change.
mDNSexport mStatus uDNS_SetupDNSConfig(mDNS *const m)
{
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    mDNSu32 slot;
    CacheGroup *cg;
    CacheRecord *cr;
#endif
    mDNSAddr v4, v6, r;
    domainname fqdn;
#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    DNSServer   *ptr, **p = &m->DNSServers;
    const DNSServer *oldServers = m->DNSServers;
    DNSQuestion *q;
#endif
    McastResolver *mr, **mres = &m->McastResolvers;
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH) && !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    DNSPushServer **psp;
#endif

    debugf("uDNS_SetupDNSConfig: entry");

    // Let the platform layer get the current DNS information and setup the WAB queries if needed.
    uDNS_SetupWABQueries(m);

    mDNS_Lock(m);

    // We need to first mark all the entries to be deleted. If the configuration changed, then
    // the entries would be undeleted appropriately. Otherwise, we need to clear them.
    //
    // Note: The last argument to mDNSPlatformSetDNSConfig is "mDNStrue" which means ack the
    // configuration. We already processed search domains in uDNS_SetupWABQueries above and
    // hence we are ready to ack the configuration as this is the last call to mDNSPlatformSetConfig
    // for the dns configuration change notification.
    SetConfigState(m, mDNStrue);
    if (!mDNSPlatformSetDNSConfig(mDNStrue, mDNSfalse, &fqdn, mDNSNULL, mDNSNULL, mDNStrue))
    {
        SetDynDNSHostNameIfChanged(m, &fqdn);
        SetConfigState(m, mDNSfalse);
        mDNS_Unlock(m);
        LogRedact(MDNS_LOG_CATEGORY_STATE, MDNS_LOG_DEFAULT, "uDNS_SetupDNSConfig: No configuration change");
        return mStatus_NoError;
    }

    // For now, we just delete the mcast resolvers. We don't deal with cache or
    // questions here. Neither question nor cache point to mcast resolvers. Questions
    // do inherit the timeout values from mcast resolvers. But we don't bother
    // affecting them as they never change.
    while (*mres)
    {
        if (((*mres)->flags & McastResolver_FlagDelete) != 0)
        {
            mr = *mres;
            *mres = (*mres)->next;
            debugf("uDNS_SetupDNSConfig: Deleting mcast resolver %##s", mr, mr->domain.c);
            mDNSPlatformMemFree(mr);
        }
        else
        {
            (*mres)->flags &= ~McastResolver_FlagNew;
            mres = &(*mres)->next;
        }
    }

#if MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
    Querier_ProcessDNSServiceChanges(mDNSfalse);
#else
    // Update our qDNSServer pointers before we go and free the DNSServer object memory
    //
    // All non-scoped resolvers share the same resGroupID. At no point in time a cache entry using DNSServer
    // from scoped resolver will be used to answer non-scoped questions and vice versa, as scoped and non-scoped
    // resolvers don't share the same resGroupID. A few examples to describe the interaction with how we pick
    // DNSServers and flush the cache.
    //
    // - A non-scoped question picks DNSServer X, creates a cache entry with X. If a new resolver gets added later that
    //   is a better match, we pick the new DNSServer for the question and activate the unicast query. We may or may not
    //   flush the cache (See PurgeOrReconfirmCacheRecord). In either case, we don't change the cache record's DNSServer
    //   pointer immediately (qDNSServer and rDNSServer may be different but still share the same resGroupID). If we don't
    //   flush the cache immediately, the record's rDNSServer pointer will be updated (in mDNSCoreReceiveResponse)
    //   later when we get the response. If we purge the cache, we still deliver a RMV when it is purged even though
    //   we don't update the cache record's DNSServer pointer to match the question's DNSSever, as they both point to
    //   the same resGroupID.
    //
    //   Note: If the new DNSServer comes back with a different response than what we have in the cache, we will deliver a RMV
    //   of the old followed by ADD of the new records.
    //
    // - A non-scoped question picks DNSServer X,  creates a cache entry with X. If the resolver gets removed later, we will
    //   pick a new DNSServer for the question which may or may not be NULL and set the cache record's pointer to the same
    //   as in question's qDNSServer if the cache record is not flushed. If there is no active question, it will be set to NULL.
    //
    // - Two questions scoped and non-scoped for the same name will pick two different DNSServer and will end up creating separate
    //   cache records and as the resGroupID is different, you can't use the cache record from the scoped DNSServer to answer the
    //   non-scoped question and vice versa.
    //
#if MDNSRESPONDER_SUPPORTS(APPLE, DNS64)
    DNS64RestartQuestions(m);
#endif

    // First, restart questions whose suppression status will change. The suppression status of each question in a given
    // question set, i.e., a non-duplicate question and all of its duplicates, if any, may or may not change. For example,
    // a suppressed (or non-suppressed) question that is currently a duplicate of a suppressed (or non-suppressed) question
    // may become a non-suppressed (or suppressed) question, while the question that it's a duplicate of may remain
    // suppressed (or non-suppressed).
    for (q = m->Questions; q; q = q->next)
    {
        DNSServer *s;
        const DNSServer *t;
        mDNSBool oldSuppressed;

        if (mDNSOpaque16IsZero(q->TargetQID)) continue;

        SetValidDNSServers(m, q);
        q->triedAllServersOnce = mDNSfalse;
        s = GetServerForQuestion(m, q);
        t = q->qDNSServer;
        if (s != t)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT,
                "[R%u->Q%u] uDNS_SetupDNSConfig: Updating DNS server from " PRI_IP_ADDR ":%d (" PRI_DM_NAME ") to "
                PRI_IP_ADDR ":%d (" PRI_DM_NAME ") for question " PRI_DM_NAME " (" PUB_S ") (scope:%p)",
                q->request_id, mDNSVal16(q->TargetQID),
                t ? &t->addr : mDNSNULL, mDNSVal16(t ? t->port : zeroIPPort), DM_NAME_PARAM(t ? &t->domain : mDNSNULL),
                s ? &s->addr : mDNSNULL, mDNSVal16(s ? s->port : zeroIPPort), DM_NAME_PARAM(s ? &s->domain : mDNSNULL),
                DM_NAME_PARAM(&q->qname), DNSTypeName(q->qtype), q->InterfaceID);
#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH)
            // If this question had a DNS Push server associated with it, substitute the new server for the
            // old one.   If there is no new server, then we'll clean up the push server later.
            if (!q->DuplicateOf && (q->dnsPushServer != mDNSNULL))
            {
                if (q->dnsPushServer->qDNSServer == t)
                {
                    q->dnsPushServer->qDNSServer = s; // which might be null
                }
                // If it is null, cancel the DNS push server.
                if (q->dnsPushServer->qDNSServer == mDNSNULL)
                {
                    DNSPushReconcileConnection(m, q);
                }
            }
#endif
        }
        oldSuppressed = q->Suppressed;
        q->Suppressed = ShouldSuppressUnicastQuery(q, s);
        if (!q->Suppressed != !oldSuppressed) q->Restart = mDNStrue;
    }
    RestartUnicastQuestions(m);

    // Now, change the server for each question set, if necessary. Note that questions whose suppression status changed
    // have already had their server changed by being restarted.
    for (q = m->Questions; q; q = q->next)
    {
        DNSServer *s;
        const DNSServer *t;

        if (mDNSOpaque16IsZero(q->TargetQID) || q->DuplicateOf) continue;

        SetValidDNSServers(m, q);
        q->triedAllServersOnce = mDNSfalse;
        s = GetServerForQuestion(m, q);
        t = q->qDNSServer;
        DNSServerChangeForQuestion(m, q, s);
        if (s == t) continue;

        q->Suppressed = ShouldSuppressUnicastQuery(q, s);
        q->unansweredQueries = 0;
        q->TargetQID = mDNS_NewMessageID(m);
        if (!q->Suppressed) ActivateUnicastQuery(m, q, mDNStrue);
    }

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_PUSH)
    // The above code may have found some DNS Push servers that are no longer valid.   Now that we
    // are done running through the code, we need to drop our connections to those servers.
    // When we get here, any such servers should have zero questions associated with them.
    for (psp = &m->DNSPushServers; *psp != mDNSNULL; )
    {
        DNSPushServer *server = *psp;

        // It's possible that a push server whose DNS server has been deleted could be still connected but
        // not referenced by any questions.  In this case, we just delete the push server rather than trying
        // to figure out with which DNS server (if any) to associate it.
        if (server->qDNSServer == mDNSNULL || (server->qDNSServer->flags & DNSServerFlag_Delete))
        {
            // Since we are changing the m->DNSPushServers that DNSPushServerCancel() will iterate later, we will do the
            // server removal for it. And tell it to not touch the m->DNSPushServers by passing alreadyRemovedFromSystem
            // == true.
            // Unlink from the m->DNSPushServers list.
            *psp = server->next;
            server->next = mDNSNULL;
            // Release all the DNS push zones that use this server from the m->DNSPushZones list.
            DNSPushZoneRemove(m, server);
            // Cancel the server.
            DNSPushServerCancel(server, mDNStrue);
            // Release the reference to the server that m->DNSPushServers list holds.
            DNS_PUSH_RELEASE(server, DNSPushServerFinalize);
        }
        else
        {
            psp = &(server->next);
        }
    }
#endif

    FORALL_CACHERECORDS(slot, cg, cr)
    {
        if (cr->resrec.InterfaceID) continue;

        // We already walked the questions and restarted/reactivated them if the dns server
        // change affected the question. That should take care of updating the cache. But
        // what if there is no active question at this point when the DNS server change
        // happened ? There could be old cache entries lying around and if we don't flush
        // them, a new question after the DNS server change could pick up these stale
        // entries and get a wrong answer.
        //
        // For cache entries that have active questions we might have skipped rescheduling
        // the questions if they were suppressed (see above). To keep it simple, we walk
        // all the cache entries to make sure that there are no stale entries. We use the
        // active question's InterfaceID/ServiceID for looking up the right DNS server.
        //
        // Note: If GetServerForName returns NULL, it could either mean that there are no
        // DNS servers or no matching DNS servers for this question. In either case,
        // the cache should get purged below when we process deleted DNS servers.

        if (cr->CRActiveQuestion)
        {
            // Purge or Reconfirm if this cache entry would use the new DNS server
            ptr = GetServerForName(m, cr->resrec.name, cr->CRActiveQuestion->InterfaceID, cr->CRActiveQuestion->ServiceID);
            if (ptr && (ptr != cr->resrec.rDNSServer))
            {
                LogInfo("uDNS_SetupDNSConfig: Purging/Reconfirming Resourcerecord %s, New DNS server %#a, Old DNS server %#a",
                        CRDisplayString(m, cr), &ptr->addr,
                        cr->resrec.rDNSServer ? &cr->resrec.rDNSServer->addr : mDNSNULL);
                PurgeOrReconfirmCacheRecord(m, cr);

                // If a cache record's DNSServer pointer is NULL, but its active question got a DNSServer in this DNS configuration
                // update, then use its DNSServer. This way, the active question and its duplicates don't miss out on RMV events.
                if (!cr->resrec.rDNSServer && cr->CRActiveQuestion->qDNSServer)
                {
                    LogInfo("uDNS_SetupDNSConfig: Using active question's DNS server %#a for cache record %s", &cr->CRActiveQuestion->qDNSServer->addr, CRDisplayString(m, cr));
                    cr->resrec.rDNSServer = cr->CRActiveQuestion->qDNSServer;
                }
            }

            if (cr->resrec.rDNSServer && cr->resrec.rDNSServer->flags & DNSServerFlag_Delete)
            {
                DNSQuestion *qptr = cr->CRActiveQuestion;
                if (qptr->qDNSServer == cr->resrec.rDNSServer)
                {
                    LogMsg("uDNS_SetupDNSConfig: ERROR!! Cache Record %s  Active question %##s (%s) (scope:%p) pointing to DNSServer Address %#a"
                           " to be freed", CRDisplayString(m, cr),
                           qptr->qname.c, DNSTypeName(qptr->qtype), qptr->InterfaceID,
                           &cr->resrec.rDNSServer->addr);
                    qptr->validDNSServers = zeroOpaque128;
                    qptr->qDNSServer = mDNSNULL;
                    cr->resrec.rDNSServer = mDNSNULL;
                }
                else
                {
                    LogInfo("uDNS_SetupDNSConfig: Cache Record %s,  Active question %##s (%s) (scope:%p), pointing to DNSServer %#a (to be deleted),"
                            " resetting to  question's DNSServer Address %#a", CRDisplayString(m, cr),
                            qptr->qname.c, DNSTypeName(qptr->qtype), qptr->InterfaceID,
                            &cr->resrec.rDNSServer->addr,
                            qptr->qDNSServer ? &qptr->qDNSServer->addr : mDNSNULL);
                    cr->resrec.rDNSServer = qptr->qDNSServer;
                }
                PurgeOrReconfirmCacheRecord(m, cr);
            }
        }
        else if (!cr->resrec.rDNSServer || cr->resrec.rDNSServer->flags & DNSServerFlag_Delete)
        {
            LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG,
                "uDNS_SetupDNSConfig: Purging Resourcerecord " PRI_S ", DNS server " PUB_S " " PRI_IP_ADDR " " PUB_S,
                CRDisplayString(m, cr), !cr->resrec.rDNSServer ? "(to be deleted)" : "",
                cr->resrec.rDNSServer ? &cr->resrec.rDNSServer->addr : mDNSNULL,
                cr->resrec.rDNSServer ? DNSScopeToString(cr->resrec.rDNSServer->scopeType) : "" );
            cr->resrec.rDNSServer = mDNSNULL;
            mDNS_PurgeCacheResourceRecord(m, cr);
        }
    }

    //  Delete all the DNS servers that are flagged for deletion
    while (*p)
    {
        if (((*p)->flags & DNSServerFlag_Delete) != 0)
        {
            ptr = *p;
            *p = (*p)->next;
            LogInfo("uDNS_SetupDNSConfig: Deleting server %p %#a:%d (%##s)", ptr, &ptr->addr, mDNSVal16(ptr->port), ptr->domain.c);
            mDNSPlatformMemFree(ptr);
        }
        else
        {
            p = &(*p)->next;
        }
    }
    LogInfo("uDNS_SetupDNSConfig: CountOfUnicastDNSServers %d", CountOfUnicastDNSServers(m));

    // If we now have no DNS servers at all and we used to have some, then immediately purge all unicast cache records (including for LLQs).
    // This is important for giving prompt remove events when the user disconnects the Ethernet cable or turns off wireless.
    // Otherwise, stale data lingers for 5-10 seconds, which is not the user-experience people expect from Bonjour.
    // Similarly, if we now have some DNS servers and we used to have none, we want to purge any fake negative results we may have generated.
    if ((m->DNSServers != mDNSNULL) != (oldServers != mDNSNULL))
    {
        int count = 0;
        FORALL_CACHERECORDS(slot, cg, cr)
        {
            if (!cr->resrec.InterfaceID)
            {
                mDNS_PurgeCacheResourceRecord(m, cr);
                count++;
            }
        }
        LogInfo("uDNS_SetupDNSConfig: %s available; purged %d unicast DNS records from cache",
                m->DNSServers ? "DNS server became" : "No DNS servers", count);

        // Force anything that needs to get zone data to get that information again
        RestartRecordGetZoneData(m);
    }
#endif // !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)

    SetDynDNSHostNameIfChanged(m, &fqdn);

    mDNS_Unlock(m);

    // handle router and primary interface changes
    v4 = v6 = r = zeroAddr;
    v4.type = r.type = mDNSAddrType_IPv4;

    if (mDNSPlatformGetPrimaryInterface(&v4, &v6, &r) == mStatus_NoError && !mDNSv4AddressIsLinkLocal(&v4.ip.v4))
    {
        mDNS_SetPrimaryInterfaceInfo(m,
                                     !mDNSIPv4AddressIsZero(v4.ip.v4) ? &v4 : mDNSNULL,
                                     !mDNSIPv6AddressIsZero(v6.ip.v6) ? &v6 : mDNSNULL,
                                     !mDNSIPv4AddressIsZero(r.ip.v4) ? &r  : mDNSNULL);
    }
    else
    {
        mDNS_SetPrimaryInterfaceInfo(m, mDNSNULL, mDNSNULL, mDNSNULL);
        if (m->FQDN.c[0]) mDNSPlatformDynDNSHostNameStatusChanged(&m->FQDN, 1); // Set status to 1 to indicate temporary failure
    }
    return mStatus_NoError;
}

mDNSexport void mDNSCoreInitComplete(mDNS *const m, mStatus result)
{
    m->mDNSPlatformStatus = result;
    if (m->MainCallback)
    {
        mDNS_Lock(m);
        mDNS_DropLockBeforeCallback();      // Allow client to legally make mDNS API calls from the callback
        m->MainCallback(m, mStatus_NoError);
        mDNS_ReclaimLockAfterCallback();    // Decrement mDNS_reentrancy to block mDNS API calls again
        mDNS_Unlock(m);
    }
}

mDNSlocal void DeregLoop(mDNS *const m, AuthRecord *const start)
{
    m->CurrentRecord = start;
    while (m->CurrentRecord)
    {
        AuthRecord *rr = m->CurrentRecord;
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "DeregLoop: " PUB_S " deregistration for %p %02X " PRI_S,
            (rr->resrec.RecordType != kDNSRecordTypeDeregistering) ? "Initiating  " : "Accelerating",
            rr, rr->resrec.RecordType, ARDisplayString(m, rr));
        if (rr->resrec.RecordType != kDNSRecordTypeDeregistering)
            mDNS_Deregister_internal(m, rr, mDNS_Dereg_rapid);
        else if (rr->AnnounceCount > 1)
        {
            rr->AnnounceCount = 1;
            rr->LastAPTime = m->timenow - rr->ThisAPInterval;
            SetNextAnnounceProbeTime(m, rr);
        }
        // Mustn't advance m->CurrentRecord until *after* mDNS_Deregister_internal, because
        // new records could have been added to the end of the list as a result of that call.
        if (m->CurrentRecord == rr) // If m->CurrentRecord was not advanced for us, do it now
            m->CurrentRecord = rr->next;
    }
}

mDNSexport void mDNS_StartExit(mDNS *const m)
{
    AuthRecord *rr;

    mDNS_Lock(m);

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit");
    m->ShutdownTime = NonZeroTime(m->timenow + mDNSPlatformOneSecond * 5);

    mDNSCoreBeSleepProxyServer_internal(m, 0, 0, 0, 0, 0);

#if MDNSRESPONDER_SUPPORTS(APPLE, WEB_CONTENT_FILTER)
    if (WCFConnectionDealloc)
    {
        if (m->WCF)
        {
            WCFConnectionDealloc(m->WCF);
            m->WCF = mDNSNULL;
        }
    }
#endif

#ifndef UNICAST_DISABLED
    {
        SearchListElem *s;

#if MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        SuspendLLQs(m);
#endif // MDNSRESPONDER_SUPPORTS(COMMON, DNS_LLQ)
        // Don't need to do SleepRecordRegistrations() here
        // because we deregister all records and services later in this routine
        while (m->Hostnames) mDNS_RemoveDynDNSHostName(m, &m->Hostnames->fqdn);

        // For each member of our SearchList, deregister any records it may have created, and cut them from the list.
        // Otherwise they'll be forcibly deregistered for us (without being cut them from the appropriate list)
        // and we may crash because the list still contains dangling pointers.
        for (s = SearchList; s; s = s->next)
            while (s->AuthRecs)
            {
                ARListElem *dereg = s->AuthRecs;
                s->AuthRecs = s->AuthRecs->next;
                mDNS_Deregister_internal(m, &dereg->ar, mDNS_Dereg_normal); // Memory will be freed in the FreeARElemCallback
            }
    }
#endif

    // When mDNSResponder exits, also deregister all the domains that are discovered through domain enumeration.
    for (DomainEnumerationOp *op = m->domainsToDoEnumeration; op != mDNSNULL; op = op->next)
    {
        for (mDNSu32 type = 0; type < mDNS_DomainTypeMaxCount; type++)
        {
            mDNS_DeregisterDomainsDiscoveredForDomainEnumeration(m, op, type);
        }
    }

    DeadvertiseAllInterfaceRecords(m, kDeadvertiseFlag_All);

    // Shut down all our active NAT Traversals
    while (m->NATTraversals)
    {
        NATTraversalInfo *t = m->NATTraversals;
        mDNS_StopNATOperation_internal(m, t);       // This will cut 't' from the list, thereby advancing m->NATTraversals in the process

        // After stopping the NAT Traversal, we zero out the fields.
        // This has particularly important implications for our AutoTunnel records --
        // when we deregister our AutoTunnel records below, we don't want their mStatus_MemFree
        // handlers to just turn around and attempt to re-register those same records.
        // Clearing t->ExternalPort/t->RequestedPort will cause the mStatus_MemFree callback handlers
        // to not do this.
        t->ExternalAddress = zerov4Addr;
        t->NewAddress      = zerov4Addr;
        t->ExternalPort    = zeroIPPort;
        t->RequestedPort   = zeroIPPort;
        t->Lifetime        = 0;
        t->Result          = mStatus_NoError;
    }

    // Make sure there are nothing but deregistering records remaining in the list
    if (m->CurrentRecord)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: ERROR m->CurrentRecord already set " PRI_S, ARDisplayString(m, m->CurrentRecord));
    }

    // We're in the process of shutting down, so queries, etc. are no longer available.
    // Consequently, determining certain information, e.g. the uDNS update server's IP
    // address, will not be possible.  The records on the main list are more likely to
    // already contain such information, so we deregister the duplicate records first.
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: Deregistering duplicate resource records");
    DeregLoop(m, m->DuplicateRecords);
    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: Deregistering resource records");

    DeregLoop(m, m->ResourceRecords);

    // If we scheduled a response to send goodbye packets, we set NextScheduledResponse to now. Normally when deregistering records,
    // we allow up to 100ms delay (to help improve record grouping) but when shutting down we don't want any such delay.
    if (m->NextScheduledResponse - m->timenow < mDNSPlatformOneSecond)
    {
        m->NextScheduledResponse = m->timenow;
        m->SuppressResponses = 0;
    }

    if (m->ResourceRecords)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: Sending final record deregistrations");
    }
    else
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: No deregistering records remain");
    }

    for (rr = m->DuplicateRecords; rr; rr = rr->next)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: Should not still have Duplicate Records remaining: %02X " PRI_S,
            rr->resrec.RecordType, ARDisplayString(m, rr));
    }

    // If any deregistering records remain, send their deregistration announcements before we exit
    if (m->mDNSPlatformStatus != mStatus_NoError) DiscardDeregistrations(m);

    mDNS_Unlock(m);

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_StartExit: done");
}

mDNSexport void mDNS_FinalExit(mDNS *const m)
{
    mDNSu32 rrcache_active = 0;
    mDNSu32 rrcache_totalused = m->rrcache_totalused;
    mDNSu32 slot;
    AuthRecord *rr;

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_FinalExit: mDNSPlatformClose");
    mDNSPlatformClose(m);

    for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
    {
        while (m->rrcache_hash[slot])
        {
            CacheGroup *cg = m->rrcache_hash[slot];
            while (cg->members)
            {
                CacheRecord *cr = cg->members;
                cg->members = cg->members->next;
                if (cr->CRActiveQuestion) rrcache_active++;
                ReleaseCacheRecord(m, cr);
            }
            cg->rrcache_tail = &cg->members;
            ReleaseCacheGroup(m, &m->rrcache_hash[slot]);
        }
    }
    debugf("mDNS_FinalExit: RR Cache was using %ld records, %lu active", rrcache_totalused, rrcache_active);
    if (rrcache_active != m->rrcache_active)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "*** ERROR *** rrcache_totalused %u; rrcache_active %u != m->rrcache_active %u",
            rrcache_totalused, rrcache_active, m->rrcache_active);
    }

    for (rr = m->ResourceRecords; rr; rr = rr->next)
    {
        LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_FinalExit failed to send goodbye for: %p %02X " PRI_S, rr, rr->resrec.RecordType,
            ARDisplayString(m, rr));
    }

#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)
    tls_cert_dispose();
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    MDNS_DISPOSE_DNSSEC_OBJ(m->DNSSECTrustAnchorManager);
#endif

    LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, "mDNS_FinalExit: done");
}

#ifdef UNIT_TEST
#include "../unittests/mdns_ut.c"
#endif
