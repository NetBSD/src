/*
 * Copyright (c) 2018, 2021 Apple Inc. All rights reserved.
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

#include "DNSCommon.h"                  // Defines general DNS utility routines
#include "unittest_common.h"
#include "mDNSFeatures.h"

mDNSexport mStatus mDNS_InitStorage_ut(mDNS *const m, mDNS_PlatformSupport *const p,
									   CacheEntity *rrcachestorage, mDNSu32 rrcachesize,
									   mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context)
{
	return mDNS_InitStorage(m, p, rrcachestorage, rrcachesize, AdvertiseLocalAddresses, Callback, Context);
}

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

void
ParseCNameChainFromMessage_ut(
	const DNSMessage *const		response,
	const mDNSu8 *const			limit,
	const mDNSInterfaceID		InterfaceID,
	const domainname *const		qname,
	domainname					cnameChain[static const MAX_CNAME_TRAVERSAL],
	mDNSu32  *const				outChainLen)
{
	ParseCNameChainFromMessage(response, limit, InterfaceID, qname, cnameChain, outChainLen);
}

void
ParseDenialOfExistenceObjsFromMessage_ut(
	const DNSMessage *const		response,
	const mDNSu8 *const			limit,
	const mDNSInterfaceID		InterfaceID,
	dnssec_obj_rr_soa_t *const	outObjSOA,
	dnssec_obj_rr_rrsig_t		objSOARRSIG[static const MAX_NUM_RRSIG_TO_PROCESS],
	mDNSu8 *const				outSOARRSIGCount,
	dnssec_obj_rr_nsec_t		outObjNSECs[static const MAX_NUM_NSEC_NSEC3_TO_PROCESS],
	mDNSu8 *const				outNSECCount,
	dnssec_obj_rr_nsec3_t		outObjNSEC3s[static const MAX_NUM_NSEC_NSEC3_TO_PROCESS],
	mDNSu8 *const				outNSEC3Count,
	dnssec_obj_rr_rrsig_t		outObjRRSIGs[static const MAX_NUM_RRSIG_TO_PROCESS],
	mDNSu8 *const				outRRSIGCount)
{
	ParseDenialOfExistenceObjsFromMessage(response, limit, InterfaceID, outObjSOA, objSOARRSIG, outSOARRSIGCount,
		outObjNSECs, outNSECCount, outObjNSEC3s, outNSEC3Count, outObjRRSIGs, outRRSIGCount);
}

// This function extracts the resource record from the answer section of the message, and determine what type of record
// is contained in the answer section:
// 1. If there are only CNAME records, kDNSType_CNAME will be returned.
// 2. If there are DNS record other than CNAME record, the other data type will be returned.
// 3. If no record is contained in the answer section, 0 will be returned.
// This function is only used in the XCTest to help the test of wildcard data response since this kind of denial of
// existence is always accompanied by the No Error data that comes from wildcard matching.
mDNSu16
GetRRTypeFromMessage(const DNSMessage * const response, const mDNSu8 * const limit, const mDNSInterfaceID InterfaceID)
{
	mDNSu16 rrtype = 0;
	const mDNSu16 answerCount = response->h.numAnswers;
	if (answerCount == 0) {
		goto exit;
	}

	const mDNSu8 *ptr = LocateAnswers(response, limit);
	if (ptr == mDNSNULL) {
		goto exit;
	}

	mDNS *const m = &mDNSStorage;
	for (mDNSu32 i = 0; i < answerCount && ptr < limit; mDNSCoreResetRecord(m), i++) {
		ptr = GetLargeResourceRecord(m, response, ptr, limit, InterfaceID, kDNSRecordTypePacketAuth, &m->rec);
		const ResourceRecord *const rr = &(m->rec.r.resrec);
		if (rr->RecordType == kDNSRecordTypePacketNegative) {
			continue;
		}

		if (rrtype == 0 || rrtype == kDNSType_CNAME) {
			rrtype = rr->rrtype;
		}
	}

exit:
	return rrtype;
}

#endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
