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

#ifndef __DNSCOMMON_H_
#define __DNSCOMMON_H_

#include "mDNSEmbeddedAPI.h"

// For gettimeofday
#if !defined(_WIN32)
#include <sys/time.h>
#else
#include "PosixCompat.h"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
#include "dnssec_mdns_core.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

//*************************************************************************************************************
// Macros

// Note: The C preprocessor stringify operator ('#') makes a string from its argument, without macro expansion
// e.g. If "version" is #define'd to be "4", then STRINGIFY_AWE(version) will return the string "version", not "4"
// To expand "version" to its value before making the string, use STRINGIFY(version) instead
#define STRINGIFY_ARGUMENT_WITHOUT_EXPANSION(s) # s
#define STRINGIFY(s) STRINGIFY_ARGUMENT_WITHOUT_EXPANSION(s)

#define ReadField16(PTR) ((mDNSu16)((((mDNSu16)((const mDNSu8 *)(PTR))[0]) << 8) | ((mDNSu16)((const mDNSu8 *)(PTR))[1])))
#define ReadField32(PTR) \
    ((mDNSu32)( \
        (((mDNSu32)((const mDNSu8 *)(PTR))[0]) << 24) | \
        (((mDNSu32)((const mDNSu8 *)(PTR))[1]) << 16) | \
        (((mDNSu32)((const mDNSu8 *)(PTR))[2]) <<  8) | \
         ((mDNSu32)((const mDNSu8 *)(PTR))[3])))

#ifdef UINT64_MAX

#define ReadField64(PTR) \
    ((uint64_t)( \
        (((uint64_t)((const mDNSu8 *)(PTR))[0]) << 56) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[1]) << 48) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[2]) << 40) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[3]) << 32) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[4]) << 24) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[5]) << 16) | \
        (((uint64_t)((const mDNSu8 *)(PTR))[6]) <<  8) | \
         ((uint64_t)((const mDNSu8 *)(PTR))[7])))

#endif

// ***************************************************************************
// MARK: - DNS Protocol Constants

typedef enum
{
    kDNSFlag0_QR_Mask        = 0x80,    // Query or response?
    kDNSFlag0_QR_Query       = 0x00,
    kDNSFlag0_QR_Response    = 0x80,

    kDNSFlag0_OP_Mask        = 0xF << 3, // Operation type
    kDNSFlag0_OP_StdQuery    = 0x0 << 3,
    kDNSFlag0_OP_Iquery      = 0x1 << 3,
    kDNSFlag0_OP_Status      = 0x2 << 3,
    kDNSFlag0_OP_Unused3     = 0x3 << 3,
    kDNSFlag0_OP_Notify      = 0x4 << 3,
    kDNSFlag0_OP_Update      = 0x5 << 3,
    kDNSFlag0_OP_DSO         = 0x6 << 3,

    kDNSFlag0_QROP_Mask   = kDNSFlag0_QR_Mask | kDNSFlag0_OP_Mask,

    kDNSFlag0_AA          = 0x04,       // Authoritative Answer?
    kDNSFlag0_TC          = 0x02,       // Truncated?
    kDNSFlag0_RD          = 0x01,       // Recursion Desired?
    kDNSFlag1_RA          = 0x80,       // Recursion Available?

    kDNSFlag1_Zero        = 0x40,       // Reserved; must be zero
    kDNSFlag1_AD          = 0x20,       // Authentic Data [RFC 2535]
    kDNSFlag1_CD          = 0x10,       // Checking Disabled [RFC 2535]

    kDNSFlag1_RC_Mask     = 0x0F,       // Response code
    kDNSFlag1_RC_NoErr    = 0x00,
    kDNSFlag1_RC_FormErr  = 0x01,
    kDNSFlag1_RC_ServFail = 0x02,
    kDNSFlag1_RC_NXDomain = 0x03,
    kDNSFlag1_RC_NotImpl  = 0x04,
    kDNSFlag1_RC_Refused  = 0x05,
    kDNSFlag1_RC_YXDomain = 0x06,
    kDNSFlag1_RC_YXRRSet  = 0x07,
    kDNSFlag1_RC_NXRRSet  = 0x08,
    kDNSFlag1_RC_NotAuth  = 0x09,
    kDNSFlag1_RC_NotZone  = 0x0A,
	kDNSFlag1_RC_DSOTypeNI = 0x0B
} DNS_Flags;

typedef enum
{
    TSIG_ErrBadSig  = 16,
    TSIG_ErrBadKey  = 17,
    TSIG_ErrBadTime = 18
} TSIG_ErrorCode;


// ***************************************************************************
// MARK: - General Utility Functions

extern NetworkInterfaceInfo *GetFirstActiveInterface(NetworkInterfaceInfo *intf);
extern mDNSInterfaceID GetNextActiveInterfaceID(const NetworkInterfaceInfo *intf);

extern mDNSu32 mDNSRandom(mDNSu32 max);     // Returns pseudo-random result from zero to max inclusive

#if !MDNSRESPONDER_SUPPORTS(APPLE, QUERIER)
extern mDNSu32 mDNS_GetNextResolverGroupID(void);
#endif

MDNS_CLOSED_ENUM(mDNSNonCryptoHash, mDNSu8,
    mDNSNonCryptoHash_FNV1a   = 0,
    mDNSNonCryptoHash_SDBM    = 1,
);

/*!
 *  @brief
 *      Calculate hash given previous calculated hash and new bytes, with given hash algorithm.
 *
 *  @param algorithm
 *      The hash algorithm to use.
 *
 *  @param previousHash
 *      The hash of previous bytes that has been calculated.
 *
 *  @param bytes
 *      Bytes to update the hash.
 *
 *  @param len
 *      The length of the bytes.
 *
 *  @result
 *      The hash value of (previous bytes + new bytes).
 */
extern mDNSu32 mDNS_NonCryptoHashUpdateBytes(mDNSNonCryptoHash algorithm, mDNSu32 previousHash, const mDNSu8 *bytes,
    mDNSu32 len);

/*!
 *  @brief
 *      Calculate hash of the bytes.
 *
 *  @param algorithm
 *      The hash algorithm to use.
 *
 *  @param bytes
 *      Bytes to calculate the hash.
 *
 *  @param len
 *      The length of the bytes.
 *
 *  @result
 *      The hash value.
 */
extern mDNSu32 mDNS_NonCryptoHash(mDNSNonCryptoHash algorithm, const mDNSu8 *bytes, mDNSu32 len);

/*!
 *    @brief
 *      Computes the 32-bit FNV-1a (non-cryptographic) hash value for an domain name.
 *
 *    @param name
 *      The domain name.
 *
 *    @result
 *      The hash value.
 *
 *    @discussion
 *      Since domain name is case-insensitive, to make sure that hash values still match when the case changes,
 *      the hash value is calculated with the normalized domain name by treating uppercase ASCII letters to their
 *      lowercase counterparts.
 *
 *      For more information about FNV Non-Cryptographic Hash ,
 *      see <https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-21>.
 */
extern mDNSu32 mDNS_DomainNameFNV1aHash(const domainname *name);

extern mDNSs32 mDNSGetTimeOfDay(struct timeval *tv, struct timezone *tz);

// ***************************************************************************
// MARK: - Domain Name Utility Functions

#define mDNSSubTypeLabel   "\x04_sub"

#define mDNSIsDigit(X)      ((X) >= '0' && (X) <= '9')
#define mDNSIsUpperCase(X)  ((X) >= 'A' && (X) <= 'Z')
#define mDNSIsLowerCase(X)  ((X) >= 'a' && (X) <= 'z')
#define mDNSIsLetter(X)     (mDNSIsUpperCase(X) || mDNSIsLowerCase(X))
#define mDNSIsPrintASCII(X) (((X) >= 32) && ((X) <= 126))

/*!
 *  @brief
 *      Convert ASCII uppercase character to its lowercase counterparts.
 *
 *  @param c
 *      The ASCII character.
 *
 *  @result
 *      The lowercase value of the character, if the original one is uppercase, otherwise, the original value.
 */
static inline int
mDNSASCIITolower(const int c)
{
    if (mDNSIsUpperCase(c))
    {
        return (c + ('a' - 'A'));
    }
    else
    {
        return c;
    }
}

/*!
 *  @brief
 *      Check if the string consists of all valid UTF-8 characters.
 *
 *  @param str
 *      The string ending with NULL.
 *
 *  @result
 *      True if the string consists of valid UTF-8 characters, otherwise, false.
 */
extern mDNSBool mDNSAreUTF8String(const char *str);

// We believe we have adequate safeguards to protect against cache poisoning.
// In the event that someone does find a workable cache poisoning attack, we want to limit the lifetime of the poisoned entry.
// We set the maximum allowable TTL to one hour.
// With the 25% correction factor to avoid the DNS Zeno's paradox bug, that gives us an actual maximum lifetime of 75 minutes.

#define mDNSMaximumMulticastTTLSeconds  (mDNSu32)4500
#define mDNSMaximumUnicastTTLSeconds    (mDNSu32)3600

// Adjustment factor to avoid race condition (used for unicast cache entries) :
// Suppose real record has TTL of 3600, and our local caching server has held it for 3500 seconds, so it returns an aged TTL of 100.
// If we do our normal refresh at 80% of the TTL, our local caching server will return 20 seconds, so we'll do another
// 80% refresh after 16 seconds, and then the server will return 4 seconds, and so on, in the fashion of Zeno's paradox.
// To avoid this, we extend the record's effective TTL to give it a little extra grace period.
// We adjust the 100 second TTL to 127. This means that when we do our 80% query after 102 seconds,
// the cached copy at our local caching server will already have expired, so the server will be forced
// to fetch a fresh copy from the authoritative server, and then return a fresh record with the full TTL of 3600 seconds.

#define RRAdjustTTL(ttl) ((ttl) + ((ttl)/4) + 2)
#define RRUnadjustedTTL(ttl) ((((ttl) - 2) * 4) / 5)

typedef enum
{
    uDNS_LLQ_Not = 0,   // Normal uDNS answer: Flush any stale records from cache, and respect record TTL
    uDNS_LLQ_Ignore,    // LLQ initial challenge packet: ignore -- has no useful records for us
    uDNS_LLQ_Entire,    // LLQ initial set of answers: Flush any stale records from cache, but assume TTL is 2 x LLQ refresh interval
    uDNS_LLQ_Events     // LLQ event packet: don't flush cache; assume TTL is 2 x LLQ refresh interval
} uDNS_LLQType;

extern mDNSu32 GetEffectiveTTL(uDNS_LLQType LLQType, mDNSu32 ttl);

#define mDNSValidHostChar(X, notfirst, notlast) (mDNSIsLetter(X) || mDNSIsDigit(X) || ((notfirst) && (notlast) && (X) == '-') )

extern mDNSu16 CompressedDomainNameLength(const domainname *const name, const domainname *parent);
extern int CountLabels(const domainname *d);
extern const domainname *SkipLeadingLabels(const domainname *d, int skip);

extern mDNSu32 TruncateUTF8ToLength(mDNSu8 *string, mDNSu32 length, mDNSu32 max);
extern mDNSBool LabelContainsSuffix(const domainlabel *const name, const mDNSBool RichText);
extern mDNSu32 RemoveLabelSuffix(domainlabel *name, mDNSBool RichText);
extern void AppendLabelSuffix(domainlabel *const name, mDNSu32 val, const mDNSBool RichText);
#define ValidateDomainName(N) (DomainNameLength(N) <= MAX_DOMAIN_NAME)

extern mDNSBool IsSubdomain(const domainname *const subdomain, const domainname *const domain);

// ***************************************************************************
// MARK: - Resource Record Utility Functions

// IdenticalResourceRecord returns true if two resources records have
// the same name, type, class, and identical rdata (InterfaceID and TTL may differ)

// IdenticalSameNameRecord is the same, except it skips the expensive SameDomainName() check,
// which is at its most expensive and least useful in cases where we know in advance that the names match

// Note: The dominant use of IdenticalResourceRecord is from ProcessQuery(), handling known-answer lists. In this case
// it's common to have a whole bunch or records with exactly the same name (e.g. "_http._tcp.local") but different RDATA.
// The SameDomainName() check is expensive when the names match, and in this case *all* the names match, so we
// used to waste a lot of CPU time verifying that the names match, only then to find that the RDATA is different.
// We observed mDNSResponder spending 30% of its total CPU time on this single task alone.
// By swapping the checks so that we check the RDATA first, we can quickly detect when it's different
// (99% of the time) and then bail out before we waste time on the expensive SameDomainName() check.

extern mDNSBool SameRDataBody(const ResourceRecord *const r1, const RDataBody *const r2, DomainNameComparisonFn *samename);

static inline mDNSBool IdenticalSameNameRecord(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
    return
    (
    #if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
        // Other than the ordinary non-DNSSEC records, there are two types of DNSSEC records:
        // 1. DNSSEC to be validated: Records that come from DNSSEC-enabled response (with DNSSEC OK/Checking Disabled bits set).
        // 2. DNSSEC validated: Records that come from the "DNSSEC to be validated" records, and has passed the DNSSEC validation.
        // Only the records that have the same type can be compared.
         (resource_records_have_same_dnssec_rr_category(r1, r2))     &&
    #endif
         r1->rrtype         == r2->rrtype       &&
         r1->rrclass        == r2->rrclass      &&
         r1->rdlength       == r2->rdlength     &&
         r1->rdatahash      == r2->rdatahash    &&
         SameRDataBody(r1, &r2->rdata->u, SameDomainName)
    );
}

static inline mDNSBool IdenticalResourceRecord(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
    return
    (
        r1->namehash == r2->namehash        &&
        IdenticalSameNameRecord(r1, r2)     &&
        SameDomainName(r1->name, r2->name)
    );
}

// A given RRType answers a QuestionType if RRType is CNAME, or types match, or QuestionType is ANY,
// or the RRType is NSEC and positively asserts the nonexistence of the type being requested from multicast,
// or the question requires the corresponding DNSSEC RRs,
// or the RRType is RRSIG that covers the the type being requested.

typedef mDNSu32 RRTypeAnswersQuestionTypeFlags;
#define kRRTypeAnswersQuestionTypeFlagsNone 0
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
#define kRRTypeAnswersQuestionTypeFlagsRequiresDNSSECRRToValidate   (1U << 0)   // Use this flag to indicate that question needs "DNSSEC to be validated" records to do validation.
#define kRRTypeAnswersQuestionTypeFlagsRequiresDNSSECRRValidated    (1U << 1)   // Use this flag to indicate that question needs "DNSSEC validated" records to return to the client.
#endif
extern mDNSBool RRTypeAnswersQuestionType(const ResourceRecord *rr, mDNSu16 qtype, RRTypeAnswersQuestionTypeFlags flags);

// Unicast NSEC records have the NSEC bit set whereas the multicast NSEC ones don't
#define UNICAST_NSEC(rr) ((rr)->rrtype == kDNSType_NSEC && RRAssertsExistence((rr), kDNSType_NSEC))
#define MULTICAST_NSEC(rr) ((rr)->rrtype == kDNSType_NSEC && RRAssertsNonexistence((rr), kDNSType_NSEC))

extern mDNSu32 RDataHashValue(const ResourceRecord *const rr);
extern mDNSBool SameRDataBody(const ResourceRecord *const r1, const RDataBody *const r2, DomainNameComparisonFn *samename);
extern mDNSBool SameNameCacheRecordAnswersQuestion(const CacheRecord *const cr, const DNSQuestion *const q);
extern mDNSBool ResourceRecordAnswersQuestion(const ResourceRecord *const rr, const DNSQuestion *const q);
extern mDNSBool AuthRecordAnswersQuestion(const AuthRecord *const ar, const DNSQuestion *const q);
extern mDNSBool CacheRecordAnswersQuestion(const CacheRecord *const cr, const DNSQuestion *const q);
extern mDNSBool AnyTypeRecordAnswersQuestion (const AuthRecord *const ar, const DNSQuestion *const q);
extern mDNSBool ResourceRecordAnswersUnicastResponse(const ResourceRecord *const rr, const DNSQuestion *const q);
extern mDNSBool LocalOnlyRecordAnswersQuestion(AuthRecord *const rr, const DNSQuestion *const q);
extern mDNSu16 GetRDLength(const ResourceRecord *const rr, mDNSBool estimate);
extern mDNSBool ValidateRData(const mDNSu16 rrtype, const mDNSu16 rdlength, const RData *const rd);
extern mStatus DNSNameToLowerCase(domainname *d, domainname *result);

/*!
 *  @brief
 *      Gets a pointer to a resource record's record data in wire format.
 *
 *  @param rr
 *      The resource record object.
 *
 *  @param bytesBuffer
 *      The buffer to be used as a temporary space to hold a resource record's record data in wire format if no
 *      existing wire-format rdata is available.
 *
 *  @param bufferSize
 *      The size of the buffer.
 *
 *  @param outRDataLen
 *      If non-NULL, the address of a variable to set to the length of the resource record's record data in
 *      wire format.
 *
 *  @param outError
 *      If non-NULL, the address of a variable to set to either a non-zero error code if this function fails, or
 *      `mStatus_NoError` if this function succeeds.
 *
 *  @result
 *      The pointer to the resource record's record data in wire format if no error occurs. Otherwise mDNSNULL
 *      and `outError` is set to a non-zero error code.
 */
extern const mDNSu8 * ResourceRecordGetRDataBytesPointer(const ResourceRecord *rr, mDNSu8 *bytesBuffer,
    mDNSu16 bufferSize, mDNSu16 *outRDataLen, mStatus *outError);

#define GetRRDomainNameTarget(RR) (                                                                          \
        ((RR)->rrtype == kDNSType_NS || (RR)->rrtype == kDNSType_CNAME || (RR)->rrtype == kDNSType_PTR || (RR)->rrtype == kDNSType_DNAME) ? &(RR)->rdata->u.name        : \
        ((RR)->rrtype == kDNSType_MX || (RR)->rrtype == kDNSType_AFSDB || (RR)->rrtype == kDNSType_RT  || (RR)->rrtype == kDNSType_KX   ) ? &(RR)->rdata->u.mx.exchange : \
        ((RR)->rrtype == kDNSType_SRV                                  ) ? &(RR)->rdata->u.srv.target : mDNSNULL )

#define LocalRecordReady(X) ((X)->resrec.RecordType != kDNSRecordTypeUnique)

// ***************************************************************************
// MARK: - DNS Message Creation Functions

extern void InitializeDNSMessage(DNSMessageHeader *h, mDNSOpaque16 id, mDNSOpaque16 flags);
extern const mDNSu8 *FindCompressionPointer(const mDNSu8 *const base, const mDNSu8 *const end, const mDNSu8 *const domname);
extern mDNSu8 *putDomainNameAsLabels(const DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, const domainname *const name);
extern mDNSu8 *putRData(const DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, const ResourceRecord *const rr);

// If we have a single large record to put in the packet, then we allow the packet to be up to 9K bytes,
// but in the normal case we try to keep the packets below 1500 to avoid IP fragmentation on standard Ethernet

#define AllowedRRSpace(msg) (((msg)->h.numAnswers || (msg)->h.numAuthorities || (msg)->h.numAdditionals) ? NormalMaxDNSMessageData : AbsoluteMaxDNSMessageData)

extern mDNSu8 *PutResourceRecordTTLWithLimit(DNSMessage *const msg, mDNSu8 *ptr, mDNSu16 *count, const ResourceRecord *rr,
    mDNSu32 ttl, const mDNSu8 *limit);

#define PutResourceRecordTTL(msg, ptr, count, rr, ttl) \
    PutResourceRecordTTLWithLimit((msg), (ptr), (count), (rr), (ttl), (msg)->data + AllowedRRSpace(msg))

#define PutResourceRecordTTLJumbo(msg, ptr, count, rr, ttl) \
    PutResourceRecordTTLWithLimit((msg), (ptr), (count), (rr), (ttl), (msg)->data + AbsoluteMaxDNSMessageData)

#define PutResourceRecord(MSG, P, C, RR) PutResourceRecordTTL((MSG), (P), (C), (RR), (RR)->rroriginalttl)

// Calculate TSR only OPT space
// Assume local variable 'tsrOptsCount'
#define TSR_OPT_SPACE           (tsrOptsCount * DNSOpt_TSRData_Space)
#define TSR_OPT_HEADER_SPACE    (tsrOptsCount ? DNSOpt_Header_Space : 0)
#define TSR_OPT_TOTAL_SPACE     (TSR_OPT_SPACE + TSR_OPT_HEADER_SPACE)

#define PutResourceRecordTSR(msg, ptr, count, rr) \
    PutResourceRecordTTLWithLimit((msg), (ptr), (count), (rr), (rr)->rroriginalttl, (msg)->data + AllowedRRSpace(msg) - TSR_OPT_TOTAL_SPACE)

// Calculate OPT space
// Assume local variables 'OwnerRecordSpace', 'TraceRecordSpace' & 'tsrOptsCount'
#define RR_OPT_SPACE    \
    (OwnerRecordSpace + TraceRecordSpace + TSR_OPT_SPACE +     \
    ((OwnerRecordSpace || TraceRecordSpace) ? 0 : TSR_OPT_HEADER_SPACE))

// The PutRR_OS variants assume a local variable 'm', put build the packet at m->omsg,
#define PutRR_OS_TTL(ptr, count, rr, ttl) \
    PutResourceRecordTTLWithLimit(&m->omsg, (ptr), (count), (rr), (ttl), m->omsg.data + AllowedRRSpace(&m->omsg) - RR_OPT_SPACE)

#define PutRR_OS(P, C, RR) PutRR_OS_TTL((P), (C), (RR), (RR)->rroriginalttl)

extern mDNSu8 *putQuestion(DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, const domainname *const name, mDNSu16 rrtype, mDNSu16 rrclass);
extern mDNSu8 *putZone(DNSMessage *const msg, mDNSu8 *ptr, mDNSu8 *limit, const domainname *zone, mDNSOpaque16 zoneClass);
extern mDNSu8 *putPrereqNameNotInUse(const domainname *const name, DNSMessage *const msg, mDNSu8 *const ptr, mDNSu8 *const end);
extern mDNSu8 *putDeletionRecord(DNSMessage *msg, mDNSu8 *ptr, ResourceRecord *rr);
extern mDNSu8 *putDeletionRecordWithLimit(DNSMessage *msg, mDNSu8 *ptr, ResourceRecord *rr, mDNSu8 *limit);
extern mDNSu8 *putDeleteRRSetWithLimit(DNSMessage *msg, mDNSu8 *ptr, const domainname *name, mDNSu16 rrtype, mDNSu8 *limit);
extern mDNSu8 *putDeleteAllRRSets(DNSMessage *msg, mDNSu8 *ptr, const domainname *name);
extern mDNSu8 *putUpdateLease(DNSMessage *msg, mDNSu8 *ptr, mDNSu32 lease);
extern mDNSu8 *putUpdateLeaseWithLimit(DNSMessage *msg, mDNSu8 *ptr, mDNSu32 lease, mDNSu8 *limit);

extern int baseEncode(char *buffer, int blen, const mDNSu8 *data, int len, int encAlg);
extern void NSEC3Parse(const ResourceRecord *const rr, mDNSu8 **salt, int *hashLength, mDNSu8 **nxtName, int *bitmaplen, mDNSu8 **bitmap);

// ***************************************************************************
// MARK: - DNS Message Parsing Functions

#define HashSlotFromNameHash(X) ((X) % CACHE_HASH_SLOTS)
extern mDNSu32 DomainNameHashValue(const domainname *const name);
extern void SetNewRData(ResourceRecord *const rr, RData *NewRData, mDNSu16 rdlength);
extern const mDNSu8 *skipDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end);
extern const mDNSu8 *getDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end,
                                   domainname *const name);
extern const mDNSu8 *skipResourceRecord(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end);
extern const mDNSu8 *GetLargeResourceRecord(mDNS *const m, const DNSMessage * const msg, const mDNSu8 *ptr,
                                            const mDNSu8 * end, const mDNSInterfaceID InterfaceID, mDNSu8 RecordType, LargeCacheRecord *const largecr);
extern mDNSBool SetRData(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *end, ResourceRecord *rr,
    mDNSu16 rdlength);
extern const mDNSu8 *skipQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end);
extern const mDNSu8 *getQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end, const mDNSInterfaceID InterfaceID,
                                 DNSQuestion *question);
extern const mDNSu8 *LocateAnswers(const DNSMessage *const msg, const mDNSu8 *const end);
extern const mDNSu8 *LocateAuthorities(const DNSMessage *const msg, const mDNSu8 *const end);
extern const mDNSu8 *LocateAdditionals(const DNSMessage *const msg, const mDNSu8 *const end);
extern const mDNSu8 *LocateOptRR(const DNSMessage *const msg, const mDNSu8 *const end, int minsize);
extern const rdataOPT *GetLLQOptData(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end);
extern mDNSBool GetPktLease(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end, mDNSu32 *const lease);
extern void DumpPacket(mStatus status, mDNSBool sent, const char *transport, const mDNSAddr *srcaddr, mDNSIPPort srcport,
    const mDNSAddr *dstaddr, mDNSIPPort dstport, const DNSMessage *const msg, const mDNSu8 *const end,
    mDNSInterfaceID interfaceID);
extern mDNSBool RRAssertsNonexistence(const ResourceRecord *const rr, mDNSu16 type);
extern mDNSBool RRAssertsExistence(const ResourceRecord *const rr, mDNSu16 type);
extern mDNSBool BitmapTypeCheck(const mDNSu8 *bmap, int bitmaplen, mDNSu16 type);

extern mDNSu16 swap16(mDNSu16 x);
extern mDNSu32 swap32(mDNSu32 x);

extern mDNSBool GetReverseIPv6Addr(const domainname *inQName, mDNSu8 outIPv6[16]);

// ***************************************************************************
// MARK: - Packet Sending Functions
extern mStatus mDNSSendDNSMessage(mDNS *const m, DNSMessage *const msg, mDNSu8 *end,
                                  mDNSInterfaceID InterfaceID, TCPSocket *tcpSrc, UDPSocket *udpSrc, const mDNSAddr *dst,
                                  mDNSIPPort dstport, DomainAuthInfo *authInfo, mDNSBool useBackgroundTrafficClass);

// ***************************************************************************
// MARK: - DNSQuestion Functions

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
extern mDNSBool DNSQuestionNeedsSensitiveLogging(const DNSQuestion *q);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, RUNTIME_MDNS_METRICS)
extern mDNSBool DNSQuestionCollectsMDNSMetric(const DNSQuestion *q);
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, TERMINUS_ASSISTED_UNICAST_DISCOVERY)
extern mDNSBool DNSQuestionIsEligibleForMDNSAlternativeService(const DNSQuestion *q);
extern mDNSBool DNSQuestionRequestsMDNSAlternativeService(const DNSQuestion *q);
extern mDNSBool DNSQuestionUsesMDNSAlternativeService(const DNSQuestion *q);
#endif

// ***************************************************************************
// MARK: - RR List Management & Task Management

extern void ShowTaskSchedulingError(mDNS *const m);

/*!
 *  @brief
 *      Check if the locking state is valid or not by comparing the values of <code> mDNS_busy</code> and <code> mDNS_reentrancy</code>, and it also
 *      remembers the last function (with the source file line number) that succeeds in doing lock operation including "Lock", "Unlock", "Drop", "Reclaim". If any
 *      invalid lock state is detected, an error message with the function name of the last successful lock operator will be printed to help debug.
 *
 *  @param operation
 *      A text description of the lock operation that would be finished after(or before) this lock state checking, possible values are "Lock", "Unlock",
 *      "Drop Lock", "Reclaim Lock" and "Check Lock".
 *
 *  @param checkIfLockHeld
 *      A boolean value to indicate if the caller wants to check if it currently holds the lock. If the lock is not held or the lock state is invalid, an error message will
 *      be printed.
 *
 *  @param mDNS_busy
 *      The mDNS_busy value getting from the mDNS_struct object, its value indicates how many times the lock have been grabbed. Note that the caller can grab
 *      the lock and drop it before the user callback to allow the callback to grab the lock again. There should be only one who has grabbed the lock while not
 *      dropping it.
 *
 *  @param mDNS_reentrancy
 *      The mDNS_reentrancy getting from the mDNS_struct object, its value indicates how many times the lock have been dropped before callback after being
 *      grabbed by others. In other words, it indicates the depth of callback stack.
 *
 *  @param functionName
 *      The name of the function that calls <code>mDNS_VerifyLockState()</code>.
 *
 *  @param lineNumber
 *      The line number in the source code file where <code>mDNS_VerifyLockState()</code> gets called.
 *
 *  @discussion
 *      This function is called whenever mDNSResponder enters/exits the critical section to help avoid the lock-related bug when mDNSResponder is compiled
 *      with multi-thread support. On all Apple platforms, we have only two threads, one is the main queue for the main event loop, the other one is the K queue for
 *      the network configuration event, so we can almost treat mDNSResponder on Apple platform as a single-thread daemon. Such locking issues do not
 *      always happen because the lock cannot be grabbed twice by different process in a single-thread process. However, mDNSResponder core code should
 *      not assume that single-thread model is always available, and it should be aware of the possible locking race condition and avoid those.
 *      <code>mDNS_VerifyLockState()</code> is created to check the state of the lock and make sure the lock is operated correctly even on a single-thread
 *      environment. When it detects any possible lock inconsistency, it will print a log message with the last successful lock operator's name and the line number,
 *      to help debug the lock-related bugs.
 *
 */
void mDNS_VerifyLockState(const char *operation, mDNSBool checkIfLockHeld,
    mDNSu32 mDNS_busy, mDNSu32 mDNS_reentrancy, const char *functionName, mDNSu32 lineNumber);

extern void mDNS_Lock_(mDNS *m, const char *functionname, mDNSu32 lineNumber);
extern void mDNS_Unlock_(mDNS *m, const char *functionname, mDNSu32 lineNumber);

#if defined(_WIN32)
 #define __func__ __FUNCTION__
#endif

#define mDNS_Lock(X) mDNS_Lock_((X), __func__, __LINE__)

#define mDNS_Unlock(X) mDNS_Unlock_((X), __func__, __LINE__)

#define mDNS_CheckLock(X) mDNS_VerifyLockState("Check Lock", mDNStrue, (X)->mDNS_busy, (X)->mDNS_reentrancy,   \
                                               __func__, __LINE__)

#define mDNS_DropLockBeforeCallback()                                                                               \
    do                                                                                                              \
    {                                                                                                               \
        m->mDNS_reentrancy++;                                                                                       \
        mDNS_VerifyLockState("Drop Lock", mDNSfalse, m->mDNS_busy, m->mDNS_reentrancy, __func__, __LINE__);         \
    } while (mDNSfalse)

#define mDNS_ReclaimLockAfterCallback()                                                                             \
    do                                                                                                              \
    {                                                                                                               \
        mDNS_VerifyLockState("Reclaim Lock", mDNSfalse, m->mDNS_busy, m->mDNS_reentrancy, __func__, __LINE__);      \
        m->mDNS_reentrancy--;                                                                                       \
    } while (mDNSfalse)

#ifdef  __cplusplus
}
#endif

#endif // __DNSCOMMON_H_
