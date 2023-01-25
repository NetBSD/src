/*	$NetBSD: dnssec.c,v 1.13 2023/01/25 21:43:30 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/serial.h>
#include <isc/string.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/kasp.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/stats.h>
#include <dns/tsig.h> /* for DNS_TSIG_FUDGE */

#include <dst/result.h>

LIBDNS_EXTERNAL_DATA isc_stats_t *dns_dnssec_stats;

#define is_response(msg) ((msg->flags & DNS_MESSAGEFLAG_QR) != 0)

#define RETERR(x)                            \
	do {                                 \
		result = (x);                \
		if (result != ISC_R_SUCCESS) \
			goto failure;        \
	} while (0)

#define TYPE_SIGN   0
#define TYPE_VERIFY 1

static isc_result_t
digest_callback(void *arg, isc_region_t *data);

static int
rdata_compare_wrapper(const void *rdata1, const void *rdata2);

static isc_result_t
rdataset_to_sortedarray(dns_rdataset_t *set, isc_mem_t *mctx,
			dns_rdata_t **rdata, int *nrdata);

static isc_result_t
digest_callback(void *arg, isc_region_t *data) {
	dst_context_t *ctx = arg;

	return (dst_context_adddata(ctx, data));
}

static void
inc_stat(isc_statscounter_t counter) {
	if (dns_dnssec_stats != NULL) {
		isc_stats_increment(dns_dnssec_stats, counter);
	}
}

/*
 * Make qsort happy.
 */
static int
rdata_compare_wrapper(const void *rdata1, const void *rdata2) {
	return (dns_rdata_compare((const dns_rdata_t *)rdata1,
				  (const dns_rdata_t *)rdata2));
}

/*
 * Sort the rdataset into an array.
 */
static isc_result_t
rdataset_to_sortedarray(dns_rdataset_t *set, isc_mem_t *mctx,
			dns_rdata_t **rdata, int *nrdata) {
	isc_result_t ret;
	int i = 0, n;
	dns_rdata_t *data;
	dns_rdataset_t rdataset;

	n = dns_rdataset_count(set);

	data = isc_mem_get(mctx, n * sizeof(dns_rdata_t));

	dns_rdataset_init(&rdataset);
	dns_rdataset_clone(set, &rdataset);
	ret = dns_rdataset_first(&rdataset);
	if (ret != ISC_R_SUCCESS) {
		dns_rdataset_disassociate(&rdataset);
		isc_mem_put(mctx, data, n * sizeof(dns_rdata_t));
		return (ret);
	}

	/*
	 * Put them in the array.
	 */
	do {
		dns_rdata_init(&data[i]);
		dns_rdataset_current(&rdataset, &data[i++]);
	} while (dns_rdataset_next(&rdataset) == ISC_R_SUCCESS);

	/*
	 * Sort the array.
	 */
	qsort(data, n, sizeof(dns_rdata_t), rdata_compare_wrapper);
	*rdata = data;
	*nrdata = n;
	dns_rdataset_disassociate(&rdataset);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dnssec_keyfromrdata(const dns_name_t *name, const dns_rdata_t *rdata,
			isc_mem_t *mctx, dst_key_t **key) {
	isc_buffer_t b;
	isc_region_t r;

	INSIST(name != NULL);
	INSIST(rdata != NULL);
	INSIST(mctx != NULL);
	INSIST(key != NULL);
	INSIST(*key == NULL);
	REQUIRE(rdata->type == dns_rdatatype_key ||
		rdata->type == dns_rdatatype_dnskey);

	dns_rdata_toregion(rdata, &r);
	isc_buffer_init(&b, r.base, r.length);
	isc_buffer_add(&b, r.length);
	return (dst_key_fromdns(name, rdata->rdclass, &b, mctx, key));
}

static isc_result_t
digest_sig(dst_context_t *ctx, bool downcase, dns_rdata_t *sigrdata,
	   dns_rdata_rrsig_t *rrsig) {
	isc_region_t r;
	isc_result_t ret;
	dns_fixedname_t fname;

	dns_rdata_toregion(sigrdata, &r);
	INSIST(r.length >= 19);

	r.length = 18;
	ret = dst_context_adddata(ctx, &r);
	if (ret != ISC_R_SUCCESS) {
		return (ret);
	}
	if (downcase) {
		dns_fixedname_init(&fname);

		RUNTIME_CHECK(dns_name_downcase(&rrsig->signer,
						dns_fixedname_name(&fname),
						NULL) == ISC_R_SUCCESS);
		dns_name_toregion(dns_fixedname_name(&fname), &r);
	} else {
		dns_name_toregion(&rrsig->signer, &r);
	}

	return (dst_context_adddata(ctx, &r));
}

isc_result_t
dns_dnssec_sign(const dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		isc_stdtime_t *inception, isc_stdtime_t *expire,
		isc_mem_t *mctx, isc_buffer_t *buffer, dns_rdata_t *sigrdata) {
	dns_rdata_rrsig_t sig;
	dns_rdata_t tmpsigrdata;
	dns_rdata_t *rdatas;
	int nrdatas, i;
	isc_buffer_t sigbuf, envbuf;
	isc_region_t r;
	dst_context_t *ctx = NULL;
	isc_result_t ret;
	isc_buffer_t *databuf = NULL;
	char data[256 + 8];
	uint32_t flags;
	unsigned int sigsize;
	dns_fixedname_t fnewname;
	dns_fixedname_t fsigner;

	REQUIRE(name != NULL);
	REQUIRE(dns_name_countlabels(name) <= 255);
	REQUIRE(set != NULL);
	REQUIRE(key != NULL);
	REQUIRE(inception != NULL);
	REQUIRE(expire != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(sigrdata != NULL);

	if (*inception >= *expire) {
		return (DNS_R_INVALIDTIME);
	}

	/*
	 * Is the key allowed to sign data?
	 */
	flags = dst_key_flags(key);
	if ((flags & DNS_KEYTYPE_NOAUTH) != 0) {
		return (DNS_R_KEYUNAUTHORIZED);
	}
	if ((flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE) {
		return (DNS_R_KEYUNAUTHORIZED);
	}

	sig.mctx = mctx;
	sig.common.rdclass = set->rdclass;
	sig.common.rdtype = dns_rdatatype_rrsig;
	ISC_LINK_INIT(&sig.common, link);

	/*
	 * Downcase signer.
	 */
	dns_name_init(&sig.signer, NULL);
	dns_fixedname_init(&fsigner);
	RUNTIME_CHECK(dns_name_downcase(dst_key_name(key),
					dns_fixedname_name(&fsigner),
					NULL) == ISC_R_SUCCESS);
	dns_name_clone(dns_fixedname_name(&fsigner), &sig.signer);

	sig.covered = set->type;
	sig.algorithm = dst_key_alg(key);
	sig.labels = dns_name_countlabels(name) - 1;
	if (dns_name_iswildcard(name)) {
		sig.labels--;
	}
	sig.originalttl = set->ttl;
	sig.timesigned = *inception;
	sig.timeexpire = *expire;
	sig.keyid = dst_key_id(key);
	ret = dst_key_sigsize(key, &sigsize);
	if (ret != ISC_R_SUCCESS) {
		return (ret);
	}
	sig.siglen = sigsize;
	/*
	 * The actual contents of sig.signature are not important yet, since
	 * they're not used in digest_sig().
	 */
	sig.signature = isc_mem_get(mctx, sig.siglen);

	isc_buffer_allocate(mctx, &databuf, sigsize + 256 + 18);

	dns_rdata_init(&tmpsigrdata);
	ret = dns_rdata_fromstruct(&tmpsigrdata, sig.common.rdclass,
				   sig.common.rdtype, &sig, databuf);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_databuf;
	}

	ret = dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC, true, 0,
				 &ctx);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_databuf;
	}

	/*
	 * Digest the SIG rdata.
	 */
	ret = digest_sig(ctx, false, &tmpsigrdata, &sig);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_context;
	}

	dns_fixedname_init(&fnewname);
	RUNTIME_CHECK(dns_name_downcase(name, dns_fixedname_name(&fnewname),
					NULL) == ISC_R_SUCCESS);
	dns_name_toregion(dns_fixedname_name(&fnewname), &r);

	/*
	 * Create an envelope for each rdata: <name|type|class|ttl>.
	 */
	isc_buffer_init(&envbuf, data, sizeof(data));
	memmove(data, r.base, r.length);
	isc_buffer_add(&envbuf, r.length);
	isc_buffer_putuint16(&envbuf, set->type);
	isc_buffer_putuint16(&envbuf, set->rdclass);
	isc_buffer_putuint32(&envbuf, set->ttl);

	ret = rdataset_to_sortedarray(set, mctx, &rdatas, &nrdatas);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_context;
	}
	isc_buffer_usedregion(&envbuf, &r);

	for (i = 0; i < nrdatas; i++) {
		uint16_t len;
		isc_buffer_t lenbuf;
		isc_region_t lenr;

		/*
		 * Skip duplicates.
		 */
		if (i > 0 && dns_rdata_compare(&rdatas[i], &rdatas[i - 1]) == 0)
		{
			continue;
		}

		/*
		 * Digest the envelope.
		 */
		ret = dst_context_adddata(ctx, &r);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}

		/*
		 * Digest the length of the rdata.
		 */
		isc_buffer_init(&lenbuf, &len, sizeof(len));
		INSIST(rdatas[i].length < 65536);
		isc_buffer_putuint16(&lenbuf, (uint16_t)rdatas[i].length);
		isc_buffer_usedregion(&lenbuf, &lenr);
		ret = dst_context_adddata(ctx, &lenr);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}

		/*
		 * Digest the rdata.
		 */
		ret = dns_rdata_digest(&rdatas[i], digest_callback, ctx);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}
	}

	isc_buffer_init(&sigbuf, sig.signature, sig.siglen);
	ret = dst_context_sign(ctx, &sigbuf);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_array;
	}
	isc_buffer_usedregion(&sigbuf, &r);
	if (r.length != sig.siglen) {
		ret = ISC_R_NOSPACE;
		goto cleanup_array;
	}

	ret = dns_rdata_fromstruct(sigrdata, sig.common.rdclass,
				   sig.common.rdtype, &sig, buffer);

cleanup_array:
	isc_mem_put(mctx, rdatas, nrdatas * sizeof(dns_rdata_t));
cleanup_context:
	dst_context_destroy(&ctx);
cleanup_databuf:
	isc_buffer_free(&databuf);
	isc_mem_put(mctx, sig.signature, sig.siglen);

	return (ret);
}

isc_result_t
dns_dnssec_verify(const dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		  bool ignoretime, unsigned int maxbits, isc_mem_t *mctx,
		  dns_rdata_t *sigrdata, dns_name_t *wild) {
	dns_rdata_rrsig_t sig;
	dns_fixedname_t fnewname;
	isc_region_t r;
	isc_buffer_t envbuf;
	dns_rdata_t *rdatas;
	int nrdatas, i;
	isc_stdtime_t now;
	isc_result_t ret;
	unsigned char data[300];
	dst_context_t *ctx = NULL;
	int labels = 0;
	uint32_t flags;
	bool downcase = false;

	REQUIRE(name != NULL);
	REQUIRE(set != NULL);
	REQUIRE(key != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(sigrdata != NULL && sigrdata->type == dns_rdatatype_rrsig);

	ret = dns_rdata_tostruct(sigrdata, &sig, NULL);
	if (ret != ISC_R_SUCCESS) {
		return (ret);
	}

	if (set->type != sig.covered) {
		return (DNS_R_SIGINVALID);
	}

	if (isc_serial_lt(sig.timeexpire, sig.timesigned)) {
		inc_stat(dns_dnssecstats_fail);
		return (DNS_R_SIGINVALID);
	}

	if (!ignoretime) {
		isc_stdtime_get(&now);

		/*
		 * Is SIG temporally valid?
		 */
		if (isc_serial_lt((uint32_t)now, sig.timesigned)) {
			inc_stat(dns_dnssecstats_fail);
			return (DNS_R_SIGFUTURE);
		} else if (isc_serial_lt(sig.timeexpire, (uint32_t)now)) {
			inc_stat(dns_dnssecstats_fail);
			return (DNS_R_SIGEXPIRED);
		}
	}

	/*
	 * NS, SOA and DNSSKEY records are signed by their owner.
	 * DS records are signed by the parent.
	 */
	switch (set->type) {
	case dns_rdatatype_ns:
	case dns_rdatatype_soa:
	case dns_rdatatype_dnskey:
		if (!dns_name_equal(name, &sig.signer)) {
			inc_stat(dns_dnssecstats_fail);
			return (DNS_R_SIGINVALID);
		}
		break;
	case dns_rdatatype_ds:
		if (dns_name_equal(name, &sig.signer)) {
			inc_stat(dns_dnssecstats_fail);
			return (DNS_R_SIGINVALID);
		}
		FALLTHROUGH;
	default:
		if (!dns_name_issubdomain(name, &sig.signer)) {
			inc_stat(dns_dnssecstats_fail);
			return (DNS_R_SIGINVALID);
		}
		break;
	}

	/*
	 * Is the key allowed to sign data?
	 */
	flags = dst_key_flags(key);
	if ((flags & DNS_KEYTYPE_NOAUTH) != 0) {
		inc_stat(dns_dnssecstats_fail);
		return (DNS_R_KEYUNAUTHORIZED);
	}
	if ((flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE) {
		inc_stat(dns_dnssecstats_fail);
		return (DNS_R_KEYUNAUTHORIZED);
	}

again:
	ret = dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC, false,
				 maxbits, &ctx);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_struct;
	}

	/*
	 * Digest the SIG rdata (not including the signature).
	 */
	ret = digest_sig(ctx, downcase, sigrdata, &sig);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_context;
	}

	/*
	 * If the name is an expanded wildcard, use the wildcard name.
	 */
	dns_fixedname_init(&fnewname);
	labels = dns_name_countlabels(name) - 1;
	RUNTIME_CHECK(dns_name_downcase(name, dns_fixedname_name(&fnewname),
					NULL) == ISC_R_SUCCESS);
	if (labels - sig.labels > 0) {
		dns_name_split(dns_fixedname_name(&fnewname), sig.labels + 1,
			       NULL, dns_fixedname_name(&fnewname));
	}

	dns_name_toregion(dns_fixedname_name(&fnewname), &r);

	/*
	 * Create an envelope for each rdata: <name|type|class|ttl>.
	 */
	isc_buffer_init(&envbuf, data, sizeof(data));
	if (labels - sig.labels > 0) {
		isc_buffer_putuint8(&envbuf, 1);
		isc_buffer_putuint8(&envbuf, '*');
		memmove(data + 2, r.base, r.length);
	} else {
		memmove(data, r.base, r.length);
	}
	isc_buffer_add(&envbuf, r.length);
	isc_buffer_putuint16(&envbuf, set->type);
	isc_buffer_putuint16(&envbuf, set->rdclass);
	isc_buffer_putuint32(&envbuf, sig.originalttl);

	ret = rdataset_to_sortedarray(set, mctx, &rdatas, &nrdatas);
	if (ret != ISC_R_SUCCESS) {
		goto cleanup_context;
	}

	isc_buffer_usedregion(&envbuf, &r);

	for (i = 0; i < nrdatas; i++) {
		uint16_t len;
		isc_buffer_t lenbuf;
		isc_region_t lenr;

		/*
		 * Skip duplicates.
		 */
		if (i > 0 && dns_rdata_compare(&rdatas[i], &rdatas[i - 1]) == 0)
		{
			continue;
		}

		/*
		 * Digest the envelope.
		 */
		ret = dst_context_adddata(ctx, &r);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}

		/*
		 * Digest the rdata length.
		 */
		isc_buffer_init(&lenbuf, &len, sizeof(len));
		INSIST(rdatas[i].length < 65536);
		isc_buffer_putuint16(&lenbuf, (uint16_t)rdatas[i].length);
		isc_buffer_usedregion(&lenbuf, &lenr);

		/*
		 * Digest the rdata.
		 */
		ret = dst_context_adddata(ctx, &lenr);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}
		ret = dns_rdata_digest(&rdatas[i], digest_callback, ctx);
		if (ret != ISC_R_SUCCESS) {
			goto cleanup_array;
		}
	}

	r.base = sig.signature;
	r.length = sig.siglen;
	ret = dst_context_verify2(ctx, maxbits, &r);
	if (ret == ISC_R_SUCCESS && downcase) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(&sig.signer, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
			      "successfully validated after lower casing "
			      "signer '%s'",
			      namebuf);
		inc_stat(dns_dnssecstats_downcase);
	} else if (ret == ISC_R_SUCCESS) {
		inc_stat(dns_dnssecstats_asis);
	}

cleanup_array:
	isc_mem_put(mctx, rdatas, nrdatas * sizeof(dns_rdata_t));
cleanup_context:
	dst_context_destroy(&ctx);
	if (ret == DST_R_VERIFYFAILURE && !downcase) {
		downcase = true;
		goto again;
	}
cleanup_struct:
	dns_rdata_freestruct(&sig);

	if (ret == DST_R_VERIFYFAILURE) {
		ret = DNS_R_SIGINVALID;
	}

	if (ret != ISC_R_SUCCESS) {
		inc_stat(dns_dnssecstats_fail);
	}

	if (ret == ISC_R_SUCCESS && labels - sig.labels > 0) {
		if (wild != NULL) {
			RUNTIME_CHECK(dns_name_concatenate(
					      dns_wildcardname,
					      dns_fixedname_name(&fnewname),
					      wild, NULL) == ISC_R_SUCCESS);
		}
		inc_stat(dns_dnssecstats_wildcard);
		ret = DNS_R_FROMWILDCARD;
	}
	return (ret);
}

bool
dns_dnssec_keyactive(dst_key_t *key, isc_stdtime_t now) {
	isc_result_t result;
	isc_stdtime_t publish, active, revoke, remove;
	bool hint_publish, hint_zsign, hint_ksign, hint_revoke, hint_remove;
	int major, minor;
	bool ksk = false, zsk = false;
	isc_result_t ret;

	/* Is this an old-style key? */
	result = dst_key_getprivateformat(key, &major, &minor);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/* Is this a KSK? */
	ret = dst_key_getbool(key, DST_BOOL_KSK, &ksk);
	if (ret != ISC_R_SUCCESS) {
		ksk = ((dst_key_flags(key) & DNS_KEYFLAG_KSK) != 0);
	}
	ret = dst_key_getbool(key, DST_BOOL_ZSK, &zsk);
	if (ret != ISC_R_SUCCESS) {
		zsk = ((dst_key_flags(key) & DNS_KEYFLAG_KSK) == 0);
	}

	/*
	 * Smart signing started with key format 1.3; prior to that, all
	 * keys are assumed active.
	 */
	if (major == 1 && minor <= 2) {
		return (true);
	}

	hint_publish = dst_key_is_published(key, now, &publish);
	hint_zsign = dst_key_is_signing(key, DST_BOOL_ZSK, now, &active);
	hint_ksign = dst_key_is_signing(key, DST_BOOL_KSK, now, &active);
	hint_revoke = dst_key_is_revoked(key, now, &revoke);
	hint_remove = dst_key_is_removed(key, now, &remove);

	if (hint_remove) {
		return (false);
	}
	if (hint_publish && hint_revoke) {
		return (true);
	}
	if (hint_zsign && zsk) {
		return (true);
	}
	if (hint_ksign && ksk) {
		return (true);
	}
	return (false);
}

/*%<
 * Indicate whether a key is scheduled to to have CDS/CDNSKEY records
 * published now.
 *
 * Returns true if.
 *  - kasp says the DS record should be published (e.g. the DS state is in
 *    RUMOURED or OMNIPRESENT state).
 * Or:
 *  - SyncPublish is set and in the past, AND
 *  - SyncDelete is unset or in the future
 */
static bool
syncpublish(dst_key_t *key, isc_stdtime_t now) {
	isc_result_t result;
	isc_stdtime_t when;
	dst_key_state_t state;
	int major, minor;
	bool publish;

	/*
	 * Is this an old-style key?
	 */
	result = dst_key_getprivateformat(key, &major, &minor);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/*
	 * Smart signing started with key format 1.3
	 */
	if (major == 1 && minor <= 2) {
		return (false);
	}

	/* Check kasp state first. */
	result = dst_key_getstate(key, DST_KEY_DS, &state);
	if (result == ISC_R_SUCCESS) {
		return (state == DST_KEY_STATE_RUMOURED ||
			state == DST_KEY_STATE_OMNIPRESENT);
	}

	/* If no kasp state, check timings. */
	publish = false;
	result = dst_key_gettime(key, DST_TIME_SYNCPUBLISH, &when);
	if (result == ISC_R_SUCCESS && when <= now) {
		publish = true;
	}
	result = dst_key_gettime(key, DST_TIME_SYNCDELETE, &when);
	if (result == ISC_R_SUCCESS && when < now) {
		publish = false;
	}
	return (publish);
}

/*%<
 * Indicate whether a key is scheduled to to have CDS/CDNSKEY records
 * deleted now.
 *
 * Returns true if:
 *  - kasp says the DS record should be unpublished (e.g. the DS state is in
 *    UNRETENTIVE or HIDDEN state).
 * Or:
 * - SyncDelete is set and in the past.
 */
static bool
syncdelete(dst_key_t *key, isc_stdtime_t now) {
	isc_result_t result;
	isc_stdtime_t when;
	dst_key_state_t state;
	int major, minor;

	/*
	 * Is this an old-style key?
	 */
	result = dst_key_getprivateformat(key, &major, &minor);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/*
	 * Smart signing started with key format 1.3.
	 */
	if (major == 1 && minor <= 2) {
		return (false);
	}

	/* Check kasp state first. */
	result = dst_key_getstate(key, DST_KEY_DS, &state);
	if (result == ISC_R_SUCCESS) {
		return (state == DST_KEY_STATE_UNRETENTIVE ||
			state == DST_KEY_STATE_HIDDEN);
	}

	/* If no kasp state, check timings. */
	result = dst_key_gettime(key, DST_TIME_SYNCDELETE, &when);
	if (result != ISC_R_SUCCESS) {
		return (false);
	}
	if (when <= now) {
		return (true);
	}
	return (false);
}

#define is_zone_key(key) \
	((dst_key_flags(key) & DNS_KEYFLAG_OWNERMASK) == DNS_KEYOWNER_ZONE)

isc_result_t
dns_dnssec_findzonekeys(dns_db_t *db, dns_dbversion_t *ver, dns_dbnode_t *node,
			const dns_name_t *name, const char *directory,
			isc_stdtime_t now, isc_mem_t *mctx,
			unsigned int maxkeys, dst_key_t **keys,
			unsigned int *nkeys) {
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dst_key_t *pubkey = NULL;
	unsigned int count = 0;

	REQUIRE(nkeys != NULL);
	REQUIRE(keys != NULL);

	*nkeys = 0;
	memset(keys, 0, sizeof(*keys) * maxkeys);
	dns_rdataset_init(&rdataset);
	RETERR(dns_db_findrdataset(db, node, ver, dns_rdatatype_dnskey, 0, 0,
				   &rdataset, NULL));
	RETERR(dns_rdataset_first(&rdataset));
	while (result == ISC_R_SUCCESS && count < maxkeys) {
		pubkey = NULL;
		dns_rdataset_current(&rdataset, &rdata);
		RETERR(dns_dnssec_keyfromrdata(name, &rdata, mctx, &pubkey));
		dst_key_setttl(pubkey, rdataset.ttl);

		if (!is_zone_key(pubkey) ||
		    (dst_key_flags(pubkey) & DNS_KEYTYPE_NOAUTH) != 0)
		{
			goto next;
		}
		/* Corrupted .key file? */
		if (!dns_name_equal(name, dst_key_name(pubkey))) {
			goto next;
		}
		keys[count] = NULL;
		result = dst_key_fromfile(
			dst_key_name(pubkey), dst_key_id(pubkey),
			dst_key_alg(pubkey),
			DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_STATE,
			directory, mctx, &keys[count]);

		/*
		 * If the key was revoked and the private file
		 * doesn't exist, maybe it was revoked internally
		 * by named.  Try loading the unrevoked version.
		 */
		if (result == ISC_R_FILENOTFOUND) {
			uint32_t flags;
			flags = dst_key_flags(pubkey);
			if ((flags & DNS_KEYFLAG_REVOKE) != 0) {
				dst_key_setflags(pubkey,
						 flags & ~DNS_KEYFLAG_REVOKE);
				result = dst_key_fromfile(
					dst_key_name(pubkey),
					dst_key_id(pubkey), dst_key_alg(pubkey),
					DST_TYPE_PUBLIC | DST_TYPE_PRIVATE |
						DST_TYPE_STATE,
					directory, mctx, &keys[count]);
				if (result == ISC_R_SUCCESS &&
				    dst_key_pubcompare(pubkey, keys[count],
						       false))
				{
					dst_key_setflags(keys[count], flags);
				}
				dst_key_setflags(pubkey, flags);
			}
		}

		if (result != ISC_R_SUCCESS) {
			char filename[DNS_NAME_FORMATSIZE +
				      DNS_SECALG_FORMATSIZE +
				      sizeof("key file for //65535")];
			isc_result_t result2;
			isc_buffer_t buf;

			isc_buffer_init(&buf, filename, NAME_MAX);
			result2 = dst_key_getfilename(
				dst_key_name(pubkey), dst_key_id(pubkey),
				dst_key_alg(pubkey),
				(DST_TYPE_PUBLIC | DST_TYPE_PRIVATE |
				 DST_TYPE_STATE),
				directory, mctx, &buf);
			if (result2 != ISC_R_SUCCESS) {
				char namebuf[DNS_NAME_FORMATSIZE];
				char algbuf[DNS_SECALG_FORMATSIZE];

				dns_name_format(dst_key_name(pubkey), namebuf,
						sizeof(namebuf));
				dns_secalg_format(dst_key_alg(pubkey), algbuf,
						  sizeof(algbuf));
				snprintf(filename, sizeof(filename) - 1,
					 "key file for %s/%s/%d", namebuf,
					 algbuf, dst_key_id(pubkey));
			}

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
				      "dns_dnssec_findzonekeys2: error "
				      "reading %s: %s",
				      filename, isc_result_totext(result));
		}

		if (result == ISC_R_FILENOTFOUND || result == ISC_R_NOPERM) {
			keys[count] = pubkey;
			pubkey = NULL;
			count++;
			goto next;
		}

		if (result != ISC_R_SUCCESS) {
			goto failure;
		}

		/*
		 * If a key is marked inactive, skip it
		 */
		if (!dns_dnssec_keyactive(keys[count], now)) {
			dst_key_setinactive(pubkey, true);
			dst_key_free(&keys[count]);
			keys[count] = pubkey;
			pubkey = NULL;
			count++;
			goto next;
		}

		/*
		 * Whatever the key's default TTL may have
		 * been, the rdataset TTL takes priority.
		 */
		dst_key_setttl(keys[count], rdataset.ttl);

		if ((dst_key_flags(keys[count]) & DNS_KEYTYPE_NOAUTH) != 0) {
			/* We should never get here. */
			dst_key_free(&keys[count]);
			goto next;
		}
		count++;
	next:
		if (pubkey != NULL) {
			dst_key_free(&pubkey);
		}
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(&rdataset);
	}
	if (result != ISC_R_NOMORE) {
		goto failure;
	}
	if (count == 0) {
		result = ISC_R_NOTFOUND;
	} else {
		result = ISC_R_SUCCESS;
	}

failure:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (pubkey != NULL) {
		dst_key_free(&pubkey);
	}
	if (result != ISC_R_SUCCESS) {
		while (count > 0) {
			dst_key_free(&keys[--count]);
		}
	}
	*nkeys = count;
	return (result);
}

isc_result_t
dns_dnssec_signmessage(dns_message_t *msg, dst_key_t *key) {
	dns_rdata_sig_t sig; /* SIG(0) */
	unsigned char data[512];
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	isc_buffer_t headerbuf, databuf, sigbuf;
	unsigned int sigsize;
	isc_buffer_t *dynbuf = NULL;
	dns_rdata_t *rdata;
	dns_rdatalist_t *datalist;
	dns_rdataset_t *dataset;
	isc_region_t r;
	isc_stdtime_t now;
	dst_context_t *ctx = NULL;
	isc_mem_t *mctx;
	isc_result_t result;

	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);

	if (is_response(msg)) {
		REQUIRE(msg->query.base != NULL);
	}

	mctx = msg->mctx;

	memset(&sig, 0, sizeof(sig));

	sig.mctx = mctx;
	sig.common.rdclass = dns_rdataclass_any;
	sig.common.rdtype = dns_rdatatype_sig; /* SIG(0) */
	ISC_LINK_INIT(&sig.common, link);

	sig.covered = 0;
	sig.algorithm = dst_key_alg(key);
	sig.labels = 0; /* the root name */
	sig.originalttl = 0;

	isc_stdtime_get(&now);
	sig.timesigned = now - DNS_TSIG_FUDGE;
	sig.timeexpire = now + DNS_TSIG_FUDGE;

	sig.keyid = dst_key_id(key);

	dns_name_init(&sig.signer, NULL);
	dns_name_clone(dst_key_name(key), &sig.signer);

	sig.siglen = 0;
	sig.signature = NULL;

	isc_buffer_init(&databuf, data, sizeof(data));

	RETERR(dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC, true, 0,
				  &ctx));

	/*
	 * Digest the fields of the SIG - we can cheat and use
	 * dns_rdata_fromstruct.  Since siglen is 0, the digested data
	 * is identical to dns format.
	 */
	RETERR(dns_rdata_fromstruct(NULL, dns_rdataclass_any,
				    dns_rdatatype_sig /* SIG(0) */, &sig,
				    &databuf));
	isc_buffer_usedregion(&databuf, &r);
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * If this is a response, digest the query.
	 */
	if (is_response(msg)) {
		RETERR(dst_context_adddata(ctx, &msg->query));
	}

	/*
	 * Digest the header.
	 */
	isc_buffer_init(&headerbuf, header, sizeof(header));
	dns_message_renderheader(msg, &headerbuf);
	isc_buffer_usedregion(&headerbuf, &r);
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * Digest the remainder of the message.
	 */
	isc_buffer_usedregion(msg->buffer, &r);
	isc_region_consume(&r, DNS_MESSAGE_HEADERLEN);
	RETERR(dst_context_adddata(ctx, &r));

	RETERR(dst_key_sigsize(key, &sigsize));
	sig.siglen = sigsize;
	sig.signature = isc_mem_get(mctx, sig.siglen);

	isc_buffer_init(&sigbuf, sig.signature, sig.siglen);
	RETERR(dst_context_sign(ctx, &sigbuf));
	dst_context_destroy(&ctx);

	rdata = NULL;
	RETERR(dns_message_gettemprdata(msg, &rdata));
	isc_buffer_allocate(msg->mctx, &dynbuf, 1024);
	RETERR(dns_rdata_fromstruct(rdata, dns_rdataclass_any,
				    dns_rdatatype_sig /* SIG(0) */, &sig,
				    dynbuf));

	isc_mem_put(mctx, sig.signature, sig.siglen);

	dns_message_takebuffer(msg, &dynbuf);

	datalist = NULL;
	RETERR(dns_message_gettemprdatalist(msg, &datalist));
	datalist->rdclass = dns_rdataclass_any;
	datalist->type = dns_rdatatype_sig; /* SIG(0) */
	ISC_LIST_APPEND(datalist->rdata, rdata, link);
	dataset = NULL;
	RETERR(dns_message_gettemprdataset(msg, &dataset));
	RUNTIME_CHECK(dns_rdatalist_tordataset(datalist, dataset) ==
		      ISC_R_SUCCESS);
	msg->sig0 = dataset;

	return (ISC_R_SUCCESS);

failure:
	if (dynbuf != NULL) {
		isc_buffer_free(&dynbuf);
	}
	if (sig.signature != NULL) {
		isc_mem_put(mctx, sig.signature, sig.siglen);
	}
	if (ctx != NULL) {
		dst_context_destroy(&ctx);
	}

	return (result);
}

isc_result_t
dns_dnssec_verifymessage(isc_buffer_t *source, dns_message_t *msg,
			 dst_key_t *key) {
	dns_rdata_sig_t sig; /* SIG(0) */
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_region_t r, source_r, sig_r, header_r;
	isc_stdtime_t now;
	dst_context_t *ctx = NULL;
	isc_mem_t *mctx;
	isc_result_t result;
	uint16_t addcount, addcount_n;
	bool signeedsfree = false;

	REQUIRE(source != NULL);
	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);

	mctx = msg->mctx;

	msg->verify_attempted = 1;
	msg->verified_sig = 0;
	msg->sig0status = dns_tsigerror_badsig;

	if (is_response(msg)) {
		if (msg->query.base == NULL) {
			return (DNS_R_UNEXPECTEDTSIG);
		}
	}

	isc_buffer_usedregion(source, &source_r);

	RETERR(dns_rdataset_first(msg->sig0));
	dns_rdataset_current(msg->sig0, &rdata);

	RETERR(dns_rdata_tostruct(&rdata, &sig, NULL));
	signeedsfree = true;

	if (sig.labels != 0) {
		result = DNS_R_SIGINVALID;
		goto failure;
	}

	if (isc_serial_lt(sig.timeexpire, sig.timesigned)) {
		result = DNS_R_SIGINVALID;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	}

	isc_stdtime_get(&now);
	if (isc_serial_lt((uint32_t)now, sig.timesigned)) {
		result = DNS_R_SIGFUTURE;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	} else if (isc_serial_lt(sig.timeexpire, (uint32_t)now)) {
		result = DNS_R_SIGEXPIRED;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	}

	if (!dns_name_equal(dst_key_name(key), &sig.signer)) {
		result = DNS_R_SIGINVALID;
		msg->sig0status = dns_tsigerror_badkey;
		goto failure;
	}

	RETERR(dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC, false, 0,
				  &ctx));

	/*
	 * Digest the SIG(0) record, except for the signature.
	 */
	dns_rdata_toregion(&rdata, &r);
	r.length -= sig.siglen;
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * If this is a response, digest the query.
	 */
	if (is_response(msg)) {
		RETERR(dst_context_adddata(ctx, &msg->query));
	}

	/*
	 * Extract the header.
	 */
	memmove(header, source_r.base, DNS_MESSAGE_HEADERLEN);

	/*
	 * Decrement the additional field counter.
	 */
	memmove(&addcount, &header[DNS_MESSAGE_HEADERLEN - 2], 2);
	addcount_n = ntohs(addcount);
	addcount = htons((uint16_t)(addcount_n - 1));
	memmove(&header[DNS_MESSAGE_HEADERLEN - 2], &addcount, 2);

	/*
	 * Digest the modified header.
	 */
	header_r.base = (unsigned char *)header;
	header_r.length = DNS_MESSAGE_HEADERLEN;
	RETERR(dst_context_adddata(ctx, &header_r));

	/*
	 * Digest all non-SIG(0) records.
	 */
	r.base = source_r.base + DNS_MESSAGE_HEADERLEN;
	r.length = msg->sigstart - DNS_MESSAGE_HEADERLEN;
	RETERR(dst_context_adddata(ctx, &r));

	sig_r.base = sig.signature;
	sig_r.length = sig.siglen;
	result = dst_context_verify(ctx, &sig_r);
	if (result != ISC_R_SUCCESS) {
		msg->sig0status = dns_tsigerror_badsig;
		goto failure;
	}

	msg->verified_sig = 1;
	msg->sig0status = dns_rcode_noerror;

	dst_context_destroy(&ctx);
	dns_rdata_freestruct(&sig);

	return (ISC_R_SUCCESS);

failure:
	if (signeedsfree) {
		dns_rdata_freestruct(&sig);
	}
	if (ctx != NULL) {
		dst_context_destroy(&ctx);
	}

	return (result);
}

/*%
 * Does this key ('rdata') self sign the rrset ('rdataset')?
 */
bool
dns_dnssec_selfsigns(dns_rdata_t *rdata, const dns_name_t *name,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		     bool ignoretime, isc_mem_t *mctx) {
	INSIST(rdataset->type == dns_rdatatype_key ||
	       rdataset->type == dns_rdatatype_dnskey);
	if (rdataset->type == dns_rdatatype_key) {
		INSIST(sigrdataset->type == dns_rdatatype_sig);
		INSIST(sigrdataset->covers == dns_rdatatype_key);
	} else {
		INSIST(sigrdataset->type == dns_rdatatype_rrsig);
		INSIST(sigrdataset->covers == dns_rdatatype_dnskey);
	}

	return (dns_dnssec_signs(rdata, name, rdataset, sigrdataset, ignoretime,
				 mctx));
}

bool
dns_dnssec_signs(dns_rdata_t *rdata, const dns_name_t *name,
		 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		 bool ignoretime, isc_mem_t *mctx) {
	dst_key_t *dstkey = NULL;
	dns_keytag_t keytag;
	dns_rdata_dnskey_t key;
	dns_rdata_rrsig_t sig;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	isc_result_t result;

	INSIST(sigrdataset->type == dns_rdatatype_rrsig);
	if (sigrdataset->covers != rdataset->type) {
		return (false);
	}

	result = dns_dnssec_keyfromrdata(name, rdata, mctx, &dstkey);
	if (result != ISC_R_SUCCESS) {
		return (false);
	}
	result = dns_rdata_tostruct(rdata, &key, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	keytag = dst_key_id(dstkey);
	for (result = dns_rdataset_first(sigrdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset))
	{
		dns_rdata_reset(&sigrdata);
		dns_rdataset_current(sigrdataset, &sigrdata);
		result = dns_rdata_tostruct(&sigrdata, &sig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		if (sig.algorithm == key.algorithm && sig.keyid == keytag) {
			result = dns_dnssec_verify(name, rdataset, dstkey,
						   ignoretime, 0, mctx,
						   &sigrdata, NULL);
			if (result == ISC_R_SUCCESS) {
				dst_key_free(&dstkey);
				return (true);
			}
		}
	}
	dst_key_free(&dstkey);
	return (false);
}

isc_result_t
dns_dnsseckey_create(isc_mem_t *mctx, dst_key_t **dstkey,
		     dns_dnsseckey_t **dkp) {
	isc_result_t result;
	dns_dnsseckey_t *dk;
	int major, minor;

	REQUIRE(dkp != NULL && *dkp == NULL);
	dk = isc_mem_get(mctx, sizeof(dns_dnsseckey_t));

	dk->key = *dstkey;
	*dstkey = NULL;
	dk->force_publish = false;
	dk->force_sign = false;
	dk->hint_publish = false;
	dk->hint_sign = false;
	dk->hint_revoke = false;
	dk->hint_remove = false;
	dk->first_sign = false;
	dk->is_active = false;
	dk->purge = false;
	dk->prepublish = 0;
	dk->source = dns_keysource_unknown;
	dk->index = 0;

	/* KSK or ZSK? */
	result = dst_key_getbool(dk->key, DST_BOOL_KSK, &dk->ksk);
	if (result != ISC_R_SUCCESS) {
		dk->ksk = ((dst_key_flags(dk->key) & DNS_KEYFLAG_KSK) != 0);
	}
	result = dst_key_getbool(dk->key, DST_BOOL_ZSK, &dk->zsk);
	if (result != ISC_R_SUCCESS) {
		dk->zsk = ((dst_key_flags(dk->key) & DNS_KEYFLAG_KSK) == 0);
	}

	/* Is this an old-style key? */
	result = dst_key_getprivateformat(dk->key, &major, &minor);
	INSIST(result == ISC_R_SUCCESS);

	/* Smart signing started with key format 1.3 */
	dk->legacy = (major == 1 && minor <= 2);

	ISC_LINK_INIT(dk, link);
	*dkp = dk;
	return (ISC_R_SUCCESS);
}

void
dns_dnsseckey_destroy(isc_mem_t *mctx, dns_dnsseckey_t **dkp) {
	dns_dnsseckey_t *dk;

	REQUIRE(dkp != NULL && *dkp != NULL);
	dk = *dkp;
	*dkp = NULL;
	if (dk->key != NULL) {
		dst_key_free(&dk->key);
	}
	isc_mem_put(mctx, dk, sizeof(dns_dnsseckey_t));
}

void
dns_dnssec_get_hints(dns_dnsseckey_t *key, isc_stdtime_t now) {
	isc_stdtime_t publish = 0, active = 0, revoke = 0, remove = 0;

	REQUIRE(key != NULL && key->key != NULL);

	key->hint_publish = dst_key_is_published(key->key, now, &publish);
	key->hint_sign = dst_key_is_signing(key->key, DST_BOOL_ZSK, now,
					    &active);
	key->hint_revoke = dst_key_is_revoked(key->key, now, &revoke);
	key->hint_remove = dst_key_is_removed(key->key, now, &remove);

	/*
	 * Activation date is set (maybe in the future), but publication date
	 * isn't. Most likely the user wants to publish now and activate later.
	 * Most likely because this is true for most rollovers, except for:
	 * 1. The unpopular ZSK Double-RRSIG method.
	 * 2. When introducing a new algorithm.
	 * These two cases are rare enough that we will set hint_publish
	 * anyway when hint_sign is set, because BIND 9 natively does not
	 * support the ZSK Double-RRSIG method, and when introducing a new
	 * algorithm, we strive to publish its signatures and DNSKEY records
	 * at the same time.
	 */
	if (key->hint_sign && publish == 0) {
		key->hint_publish = true;
	}

	/*
	 * If activation date is in the future, make note of how far off.
	 */
	if (key->hint_publish && active > now) {
		key->prepublish = active - now;
	}

	/*
	 * Metadata says revoke.  If the key is published, we *have to* sign
	 * with it per RFC5011 -- even if it was not active before.
	 *
	 * If it hasn't already been done, we should also revoke it now.
	 */
	if (key->hint_publish && key->hint_revoke) {
		uint32_t flags;
		key->hint_sign = true;
		flags = dst_key_flags(key->key);
		if ((flags & DNS_KEYFLAG_REVOKE) == 0) {
			flags |= DNS_KEYFLAG_REVOKE;
			dst_key_setflags(key->key, flags);
		}
	}

	/*
	 * Metadata says delete, so don't publish this key or sign with it
	 * (note that signatures of a removed key may still be reused).
	 */
	if (key->hint_remove) {
		key->hint_publish = false;
		key->hint_sign = false;
	}
}

/*%
 * Get a list of DNSSEC keys from the key repository.
 */
isc_result_t
dns_dnssec_findmatchingkeys(const dns_name_t *origin, const char *directory,
			    isc_stdtime_t now, isc_mem_t *mctx,
			    dns_dnsseckeylist_t *keylist) {
	isc_result_t result = ISC_R_SUCCESS;
	bool dir_open = false;
	dns_dnsseckeylist_t list;
	isc_dir_t dir;
	dns_dnsseckey_t *key = NULL;
	dst_key_t *dstkey = NULL;
	char namebuf[DNS_NAME_FORMATSIZE];
	isc_buffer_t b;
	unsigned int len, i, alg;

	REQUIRE(keylist != NULL);
	ISC_LIST_INIT(list);
	isc_dir_init(&dir);

	isc_buffer_init(&b, namebuf, sizeof(namebuf) - 1);
	RETERR(dns_name_tofilenametext(origin, false, &b));
	len = isc_buffer_usedlength(&b);
	namebuf[len] = '\0';

	if (directory == NULL) {
		directory = ".";
	}
	RETERR(isc_dir_open(&dir, directory));
	dir_open = true;

	while (isc_dir_read(&dir) == ISC_R_SUCCESS) {
		if (dir.entry.name[0] != 'K' || dir.entry.length < len + 1 ||
		    dir.entry.name[len + 1] != '+' ||
		    strncasecmp(dir.entry.name + 1, namebuf, len) != 0)
		{
			continue;
		}

		alg = 0;
		for (i = len + 1 + 1; i < dir.entry.length; i++) {
			if (!isdigit((unsigned char)dir.entry.name[i])) {
				break;
			}
			alg *= 10;
			alg += dir.entry.name[i] - '0';
		}

		/*
		 * Did we not read exactly 3 digits?
		 * Did we overflow?
		 * Did we correctly terminate?
		 */
		if (i != len + 1 + 1 + 3 || i >= dir.entry.length ||
		    dir.entry.name[i] != '+')
		{
			continue;
		}

		for (i++; i < dir.entry.length; i++) {
			if (!isdigit((unsigned char)dir.entry.name[i])) {
				break;
			}
		}

		/*
		 * Did we not read exactly 5 more digits?
		 * Did we overflow?
		 * Did we correctly terminate?
		 */
		if (i != len + 1 + 1 + 3 + 1 + 5 || i >= dir.entry.length ||
		    strcmp(dir.entry.name + i, ".private") != 0)
		{
			continue;
		}

		dstkey = NULL;
		result = dst_key_fromnamedfile(
			dir.entry.name, directory,
			DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_STATE,
			mctx, &dstkey);

		switch (alg) {
		case DST_ALG_HMACMD5:
		case DST_ALG_HMACSHA1:
		case DST_ALG_HMACSHA224:
		case DST_ALG_HMACSHA256:
		case DST_ALG_HMACSHA384:
		case DST_ALG_HMACSHA512:
			if (result == DST_R_BADKEYTYPE) {
				continue;
			}
		}

		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
				      "dns_dnssec_findmatchingkeys: "
				      "error reading key file %s: %s",
				      dir.entry.name,
				      isc_result_totext(result));
			continue;
		}

		RETERR(dns_dnsseckey_create(mctx, &dstkey, &key));
		key->source = dns_keysource_repository;
		dns_dnssec_get_hints(key, now);

		if (key->legacy) {
			dns_dnsseckey_destroy(mctx, &key);
		} else {
			ISC_LIST_APPEND(list, key, link);
			key = NULL;
		}
	}

	if (!ISC_LIST_EMPTY(list)) {
		result = ISC_R_SUCCESS;
		ISC_LIST_APPENDLIST(*keylist, list, link);
	} else {
		result = ISC_R_NOTFOUND;
	}

failure:
	if (dir_open) {
		isc_dir_close(&dir);
	}
	INSIST(key == NULL);
	while ((key = ISC_LIST_HEAD(list)) != NULL) {
		ISC_LIST_UNLINK(list, key, link);
		INSIST(key->key != NULL);
		dst_key_free(&key->key);
		dns_dnsseckey_destroy(mctx, &key);
	}
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	return (result);
}

/*%
 * Add 'newkey' to 'keylist' if it's not already there.
 *
 * If 'savekeys' is true, then we need to preserve all
 * the keys in the keyset, regardless of whether they have
 * metadata indicating they should be deactivated or removed.
 */
static isc_result_t
addkey(dns_dnsseckeylist_t *keylist, dst_key_t **newkey, bool savekeys,
       isc_mem_t *mctx) {
	dns_dnsseckey_t *key;
	isc_result_t result;

	/* Skip duplicates */
	for (key = ISC_LIST_HEAD(*keylist); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		if (dst_key_id(key->key) == dst_key_id(*newkey) &&
		    dst_key_alg(key->key) == dst_key_alg(*newkey) &&
		    dns_name_equal(dst_key_name(key->key),
				   dst_key_name(*newkey)))
		{
			break;
		}
	}

	if (key != NULL) {
		/*
		 * Found a match.  If the old key was only public and the
		 * new key is private, replace the old one; otherwise
		 * leave it.  But either way, mark the key as having
		 * been found in the zone.
		 */
		if (dst_key_isprivate(key->key)) {
			dst_key_free(newkey);
		} else if (dst_key_isprivate(*newkey)) {
			dst_key_free(&key->key);
			key->key = *newkey;
		}

		key->source = dns_keysource_zoneapex;
		return (ISC_R_SUCCESS);
	}

	result = dns_dnsseckey_create(mctx, newkey, &key);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	if (key->legacy || savekeys) {
		key->force_publish = true;
		key->force_sign = dst_key_isprivate(key->key);
	}
	key->source = dns_keysource_zoneapex;
	ISC_LIST_APPEND(*keylist, key, link);
	*newkey = NULL;
	return (ISC_R_SUCCESS);
}

/*%
 * Mark all keys which signed the DNSKEY/SOA RRsets as "active",
 * for future reference.
 */
static isc_result_t
mark_active_keys(dns_dnsseckeylist_t *keylist, dns_rdataset_t *rrsigs) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdataset_t sigs;
	dns_dnsseckey_t *key;

	REQUIRE(rrsigs != NULL && dns_rdataset_isassociated(rrsigs));

	dns_rdataset_init(&sigs);
	dns_rdataset_clone(rrsigs, &sigs);
	for (key = ISC_LIST_HEAD(*keylist); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		uint16_t keyid, sigid;
		dns_secalg_t keyalg, sigalg;
		keyid = dst_key_id(key->key);
		keyalg = dst_key_alg(key->key);

		for (result = dns_rdataset_first(&sigs);
		     result == ISC_R_SUCCESS; result = dns_rdataset_next(&sigs))
		{
			dns_rdata_rrsig_t sig;

			dns_rdata_reset(&rdata);
			dns_rdataset_current(&sigs, &rdata);
			result = dns_rdata_tostruct(&rdata, &sig, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			sigalg = sig.algorithm;
			sigid = sig.keyid;
			if (keyid == sigid && keyalg == sigalg) {
				key->is_active = true;
				break;
			}
		}
	}

	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

	if (dns_rdataset_isassociated(&sigs)) {
		dns_rdataset_disassociate(&sigs);
	}
	return (result);
}

/*%
 * Add the contents of a DNSKEY rdataset 'keyset' to 'keylist'.
 */
isc_result_t
dns_dnssec_keylistfromrdataset(const dns_name_t *origin, const char *directory,
			       isc_mem_t *mctx, dns_rdataset_t *keyset,
			       dns_rdataset_t *keysigs, dns_rdataset_t *soasigs,
			       bool savekeys, bool publickey,
			       dns_dnsseckeylist_t *keylist) {
	dns_rdataset_t keys;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dst_key_t *dnskey = NULL, *pubkey = NULL, *privkey = NULL;
	isc_result_t result;

	REQUIRE(keyset != NULL && dns_rdataset_isassociated(keyset));

	dns_rdataset_init(&keys);

	dns_rdataset_clone(keyset, &keys);
	for (result = dns_rdataset_first(&keys); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&keys))
	{
		dns_rdata_reset(&rdata);
		dns_rdataset_current(&keys, &rdata);

		REQUIRE(rdata.type == dns_rdatatype_key ||
			rdata.type == dns_rdatatype_dnskey);
		REQUIRE(rdata.length > 3);

		/* Skip unsupported algorithms */
		if (!dst_algorithm_supported(rdata.data[3])) {
			goto skip;
		}

		RETERR(dns_dnssec_keyfromrdata(origin, &rdata, mctx, &dnskey));
		dst_key_setttl(dnskey, keys.ttl);

		if (!is_zone_key(dnskey) ||
		    (dst_key_flags(dnskey) & DNS_KEYTYPE_NOAUTH) != 0)
		{
			goto skip;
		}

		/* Corrupted .key file? */
		if (!dns_name_equal(origin, dst_key_name(dnskey))) {
			goto skip;
		}

		if (publickey) {
			RETERR(addkey(keylist, &dnskey, savekeys, mctx));
			goto skip;
		}

		/* Try to read the public key. */
		result = dst_key_fromfile(
			dst_key_name(dnskey), dst_key_id(dnskey),
			dst_key_alg(dnskey), (DST_TYPE_PUBLIC | DST_TYPE_STATE),
			directory, mctx, &pubkey);
		if (result == ISC_R_FILENOTFOUND || result == ISC_R_NOPERM) {
			result = ISC_R_SUCCESS;
		}
		RETERR(result);

		/* Now read the private key. */
		result = dst_key_fromfile(
			dst_key_name(dnskey), dst_key_id(dnskey),
			dst_key_alg(dnskey),
			(DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_STATE),
			directory, mctx, &privkey);

		/*
		 * If the key was revoked and the private file
		 * doesn't exist, maybe it was revoked internally
		 * by named.  Try loading the unrevoked version.
		 */
		if (result == ISC_R_FILENOTFOUND) {
			uint32_t flags;
			flags = dst_key_flags(dnskey);
			if ((flags & DNS_KEYFLAG_REVOKE) != 0) {
				dst_key_setflags(dnskey,
						 flags & ~DNS_KEYFLAG_REVOKE);
				result = dst_key_fromfile(
					dst_key_name(dnskey),
					dst_key_id(dnskey), dst_key_alg(dnskey),
					(DST_TYPE_PUBLIC | DST_TYPE_PRIVATE |
					 DST_TYPE_STATE),
					directory, mctx, &privkey);
				if (result == ISC_R_SUCCESS &&
				    dst_key_pubcompare(dnskey, privkey, false))
				{
					dst_key_setflags(privkey, flags);
				}
				dst_key_setflags(dnskey, flags);
			}
		}

		if (result != ISC_R_SUCCESS) {
			char filename[DNS_NAME_FORMATSIZE +
				      DNS_SECALG_FORMATSIZE +
				      sizeof("key file for //65535")];
			isc_result_t result2;
			isc_buffer_t buf;

			isc_buffer_init(&buf, filename, NAME_MAX);
			result2 = dst_key_getfilename(
				dst_key_name(dnskey), dst_key_id(dnskey),
				dst_key_alg(dnskey),
				(DST_TYPE_PUBLIC | DST_TYPE_PRIVATE |
				 DST_TYPE_STATE),
				directory, mctx, &buf);
			if (result2 != ISC_R_SUCCESS) {
				char namebuf[DNS_NAME_FORMATSIZE];
				char algbuf[DNS_SECALG_FORMATSIZE];

				dns_name_format(dst_key_name(dnskey), namebuf,
						sizeof(namebuf));
				dns_secalg_format(dst_key_alg(dnskey), algbuf,
						  sizeof(algbuf));
				snprintf(filename, sizeof(filename) - 1,
					 "key file for %s/%s/%d", namebuf,
					 algbuf, dst_key_id(dnskey));
			}

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
				      "dns_dnssec_keylistfromrdataset: error "
				      "reading %s: %s",
				      filename, isc_result_totext(result));
		}

		if (result == ISC_R_FILENOTFOUND || result == ISC_R_NOPERM) {
			if (pubkey != NULL) {
				RETERR(addkey(keylist, &pubkey, savekeys,
					      mctx));
			} else {
				RETERR(addkey(keylist, &dnskey, savekeys,
					      mctx));
			}
			goto skip;
		}
		RETERR(result);

		/* This should never happen. */
		if ((dst_key_flags(privkey) & DNS_KEYTYPE_NOAUTH) != 0) {
			goto skip;
		}

		/*
		 * Whatever the key's default TTL may have
		 * been, the rdataset TTL takes priority.
		 */
		dst_key_setttl(privkey, dst_key_getttl(dnskey));

		RETERR(addkey(keylist, &privkey, savekeys, mctx));
	skip:
		if (dnskey != NULL) {
			dst_key_free(&dnskey);
		}
		if (pubkey != NULL) {
			dst_key_free(&pubkey);
		}
		if (privkey != NULL) {
			dst_key_free(&privkey);
		}
	}

	if (result != ISC_R_NOMORE) {
		RETERR(result);
	}

	if (keysigs != NULL && dns_rdataset_isassociated(keysigs)) {
		RETERR(mark_active_keys(keylist, keysigs));
	}

	if (soasigs != NULL && dns_rdataset_isassociated(soasigs)) {
		RETERR(mark_active_keys(keylist, soasigs));
	}

	result = ISC_R_SUCCESS;

failure:
	if (dns_rdataset_isassociated(&keys)) {
		dns_rdataset_disassociate(&keys);
	}
	if (dnskey != NULL) {
		dst_key_free(&dnskey);
	}
	if (pubkey != NULL) {
		dst_key_free(&pubkey);
	}
	if (privkey != NULL) {
		dst_key_free(&privkey);
	}
	return (result);
}

static isc_result_t
make_dnskey(dst_key_t *key, unsigned char *buf, int bufsize,
	    dns_rdata_t *target) {
	isc_result_t result;
	isc_buffer_t b;
	isc_region_t r;

	isc_buffer_init(&b, buf, bufsize);
	result = dst_key_todns(key, &b);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdata_reset(target);
	isc_buffer_usedregion(&b, &r);
	dns_rdata_fromregion(target, dst_key_class(key), dns_rdatatype_dnskey,
			     &r);
	return (ISC_R_SUCCESS);
}

static isc_result_t
addrdata(dns_rdata_t *rdata, dns_diff_t *diff, const dns_name_t *origin,
	 dns_ttl_t ttl, isc_mem_t *mctx) {
	isc_result_t result;
	dns_difftuple_t *tuple = NULL;

	RETERR(dns_difftuple_create(mctx, DNS_DIFFOP_ADD, origin, ttl, rdata,
				    &tuple));
	dns_diff_appendminimal(diff, &tuple);

failure:
	return (result);
}

static isc_result_t
delrdata(dns_rdata_t *rdata, dns_diff_t *diff, const dns_name_t *origin,
	 dns_ttl_t ttl, isc_mem_t *mctx) {
	isc_result_t result;
	dns_difftuple_t *tuple = NULL;

	RETERR(dns_difftuple_create(mctx, DNS_DIFFOP_DEL, origin, ttl, rdata,
				    &tuple));
	dns_diff_appendminimal(diff, &tuple);

failure:
	return (result);
}

static isc_result_t
publish_key(dns_diff_t *diff, dns_dnsseckey_t *key, const dns_name_t *origin,
	    dns_ttl_t ttl, isc_mem_t *mctx, void (*report)(const char *, ...)) {
	isc_result_t result;
	unsigned char buf[DST_KEY_MAXSIZE];
	char keystr[DST_KEY_FORMATSIZE];
	dns_rdata_t dnskey = DNS_RDATA_INIT;

	dns_rdata_reset(&dnskey);
	RETERR(make_dnskey(key->key, buf, sizeof(buf), &dnskey));
	dst_key_format(key->key, keystr, sizeof(keystr));

	report("Fetching %s (%s) from key %s.", keystr,
	       key->ksk ? (key->zsk ? "CSK" : "KSK") : "ZSK",
	       key->source == dns_keysource_user ? "file" : "repository");

	if (key->prepublish && ttl > key->prepublish) {
		isc_stdtime_t now;

		report("Key %s: Delaying activation to match the DNSKEY TTL.",
		       keystr, ttl);

		isc_stdtime_get(&now);
		dst_key_settime(key->key, DST_TIME_ACTIVATE, now + ttl);
	}

	/* publish key */
	result = addrdata(&dnskey, diff, origin, ttl, mctx);

failure:
	return (result);
}

static isc_result_t
remove_key(dns_diff_t *diff, dns_dnsseckey_t *key, const dns_name_t *origin,
	   dns_ttl_t ttl, isc_mem_t *mctx, const char *reason,
	   void (*report)(const char *, ...)) {
	isc_result_t result;
	unsigned char buf[DST_KEY_MAXSIZE];
	dns_rdata_t dnskey = DNS_RDATA_INIT;
	char alg[80];

	dns_secalg_format(dst_key_alg(key->key), alg, sizeof(alg));
	report("Removing %s key %d/%s from DNSKEY RRset.", reason,
	       dst_key_id(key->key), alg);

	RETERR(make_dnskey(key->key, buf, sizeof(buf), &dnskey));
	result = delrdata(&dnskey, diff, origin, ttl, mctx);

failure:
	return (result);
}

static bool
exists(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	isc_result_t result;
	dns_rdataset_t trdataset;

	dns_rdataset_init(&trdataset);
	dns_rdataset_clone(rdataset, &trdataset);
	for (result = dns_rdataset_first(&trdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&trdataset))
	{
		dns_rdata_t current = DNS_RDATA_INIT;

		dns_rdataset_current(&trdataset, &current);
		if (dns_rdata_compare(rdata, &current) == 0) {
			dns_rdataset_disassociate(&trdataset);
			return (true);
		}
	}
	dns_rdataset_disassociate(&trdataset);
	return (false);
}

isc_result_t
dns_dnssec_syncupdate(dns_dnsseckeylist_t *keys, dns_dnsseckeylist_t *rmkeys,
		      dns_rdataset_t *cds, dns_rdataset_t *cdnskey,
		      isc_stdtime_t now, dns_ttl_t ttl, dns_diff_t *diff,
		      isc_mem_t *mctx) {
	unsigned char dsbuf1[DNS_DS_BUFFERSIZE];
	unsigned char dsbuf2[DNS_DS_BUFFERSIZE];
	unsigned char keybuf[DST_KEY_MAXSIZE];
	isc_result_t result;
	dns_dnsseckey_t *key;

	for (key = ISC_LIST_HEAD(*keys); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		dns_rdata_t cds_sha1 = DNS_RDATA_INIT;
		dns_rdata_t cds_sha256 = DNS_RDATA_INIT;
		dns_rdata_t cdnskeyrdata = DNS_RDATA_INIT;
		dns_name_t *origin = dst_key_name(key->key);

		RETERR(make_dnskey(key->key, keybuf, sizeof(keybuf),
				   &cdnskeyrdata));

		/*
		 * We construct the SHA-1 version of the record so we can
		 * delete any old records generated by previous versions of
		 * BIND. We only add SHA-256 records.
		 *
		 * XXXMPA we need to be able to specify the DS algorithms
		 * to be used here and below with rmkeys.
		 */
		RETERR(dns_ds_buildrdata(origin, &cdnskeyrdata,
					 DNS_DSDIGEST_SHA1, dsbuf1, &cds_sha1));
		RETERR(dns_ds_buildrdata(origin, &cdnskeyrdata,
					 DNS_DSDIGEST_SHA256, dsbuf2,
					 &cds_sha256));

		/*
		 * Now that the we have created the DS records convert
		 * the rdata to CDNSKEY and CDS for comparison.
		 */
		cdnskeyrdata.type = dns_rdatatype_cdnskey;
		cds_sha1.type = dns_rdatatype_cds;
		cds_sha256.type = dns_rdatatype_cds;

		if (syncpublish(key->key, now)) {
			char keystr[DST_KEY_FORMATSIZE];
			dst_key_format(key->key, keystr, sizeof(keystr));

			if (!dns_rdataset_isassociated(cdnskey) ||
			    !exists(cdnskey, &cdnskeyrdata))
			{
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_DNSSEC,
					      ISC_LOG_INFO,
					      "CDS for key %s is now published",
					      keystr);
				RETERR(addrdata(&cdnskeyrdata, diff, origin,
						ttl, mctx));
			}
			/* Only publish SHA-256 (SHA-1 is deprecated) */
			if (!dns_rdataset_isassociated(cds) ||
			    !exists(cds, &cds_sha256))
			{
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_GENERAL,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"CDNSKEY for key %s is now published",
					keystr);
				RETERR(addrdata(&cds_sha256, diff, origin, ttl,
						mctx));
			}
		}

		if (syncdelete(key->key, now)) {
			char keystr[DST_KEY_FORMATSIZE];
			dst_key_format(key->key, keystr, sizeof(keystr));

			if (dns_rdataset_isassociated(cds)) {
				/* Delete both SHA-1 and SHA-256 */
				if (exists(cds, &cds_sha1)) {
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_GENERAL,
						      DNS_LOGMODULE_DNSSEC,
						      ISC_LOG_INFO,
						      "CDS (SHA-1) for key %s "
						      "is now deleted",
						      keystr);
					RETERR(delrdata(&cds_sha1, diff, origin,
							cds->ttl, mctx));
				}
				if (exists(cds, &cds_sha256)) {
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_GENERAL,
						      DNS_LOGMODULE_DNSSEC,
						      ISC_LOG_INFO,
						      "CDS (SHA-256) for key "
						      "%s is now deleted",
						      keystr);
					RETERR(delrdata(&cds_sha256, diff,
							origin, cds->ttl,
							mctx));
				}
			}

			if (dns_rdataset_isassociated(cdnskey)) {
				if (exists(cdnskey, &cdnskeyrdata)) {
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_GENERAL,
						      DNS_LOGMODULE_DNSSEC,
						      ISC_LOG_INFO,
						      "CDNSKEY for key %s is "
						      "now deleted",
						      keystr);
					RETERR(delrdata(&cdnskeyrdata, diff,
							origin, cdnskey->ttl,
							mctx));
				}
			}
		}
	}

	if (!dns_rdataset_isassociated(cds) &&
	    !dns_rdataset_isassociated(cdnskey))
	{
		return (ISC_R_SUCCESS);
	}

	/*
	 * Unconditionally remove CDS/DNSKEY records for removed keys.
	 */
	for (key = ISC_LIST_HEAD(*rmkeys); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		dns_rdata_t cds_sha1 = DNS_RDATA_INIT;
		dns_rdata_t cds_sha256 = DNS_RDATA_INIT;
		dns_rdata_t cdnskeyrdata = DNS_RDATA_INIT;
		dns_name_t *origin = dst_key_name(key->key);

		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key->key, keystr, sizeof(keystr));

		RETERR(make_dnskey(key->key, keybuf, sizeof(keybuf),
				   &cdnskeyrdata));

		if (dns_rdataset_isassociated(cds)) {
			RETERR(dns_ds_buildrdata(origin, &cdnskeyrdata,
						 DNS_DSDIGEST_SHA1, dsbuf1,
						 &cds_sha1));
			RETERR(dns_ds_buildrdata(origin, &cdnskeyrdata,
						 DNS_DSDIGEST_SHA256, dsbuf2,
						 &cds_sha256));
			if (exists(cds, &cds_sha1)) {
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_GENERAL,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"CDS (SHA-1) for key %s is now deleted",
					keystr);
				RETERR(delrdata(&cds_sha1, diff, origin,
						cds->ttl, mctx));
			}
			if (exists(cds, &cds_sha256)) {
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
					      DNS_LOGMODULE_DNSSEC,
					      ISC_LOG_INFO,
					      "CDS (SHA-256) for key %s is now "
					      "deleted",
					      keystr);
				RETERR(delrdata(&cds_sha256, diff, origin,
						cds->ttl, mctx));
			}
		}

		if (dns_rdataset_isassociated(cdnskey)) {
			if (exists(cdnskey, &cdnskeyrdata)) {
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_GENERAL,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"CDNSKEY for key %s is now deleted",
					keystr);
				RETERR(delrdata(&cdnskeyrdata, diff, origin,
						cdnskey->ttl, mctx));
			}
		}
	}

	result = ISC_R_SUCCESS;

failure:
	return (result);
}

isc_result_t
dns_dnssec_syncdelete(dns_rdataset_t *cds, dns_rdataset_t *cdnskey,
		      dns_name_t *origin, dns_rdataclass_t zclass,
		      dns_ttl_t ttl, dns_diff_t *diff, isc_mem_t *mctx,
		      bool expect_cds_delete, bool expect_cdnskey_delete) {
	unsigned char dsbuf[5] = { 0, 0, 0, 0, 0 };  /* CDS DELETE rdata */
	unsigned char keybuf[5] = { 0, 0, 3, 0, 0 }; /* CDNSKEY DELETE rdata */
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_rdata_t cds_delete = DNS_RDATA_INIT;
	dns_rdata_t cdnskey_delete = DNS_RDATA_INIT;
	isc_region_t r;
	isc_result_t result;

	r.base = keybuf;
	r.length = sizeof(keybuf);
	dns_rdata_fromregion(&cdnskey_delete, zclass, dns_rdatatype_cdnskey,
			     &r);

	r.base = dsbuf;
	r.length = sizeof(dsbuf);
	dns_rdata_fromregion(&cds_delete, zclass, dns_rdatatype_cds, &r);

	dns_name_format(origin, namebuf, sizeof(namebuf));

	if (expect_cds_delete) {
		if (!dns_rdataset_isassociated(cds) ||
		    !exists(cds, &cds_delete))
		{
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
				      "CDS (DELETE) for zone %s is now "
				      "published",
				      namebuf);
			RETERR(addrdata(&cds_delete, diff, origin, ttl, mctx));
		}
	} else {
		if (dns_rdataset_isassociated(cds) && exists(cds, &cds_delete))
		{
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
				      "CDS (DELETE) for zone %s is now "
				      "deleted",
				      namebuf);
			RETERR(delrdata(&cds_delete, diff, origin, cds->ttl,
					mctx));
		}
	}

	if (expect_cdnskey_delete) {
		if (!dns_rdataset_isassociated(cdnskey) ||
		    !exists(cdnskey, &cdnskey_delete))
		{
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
				      "CDNSKEY (DELETE) for zone %s is now "
				      "published",
				      namebuf);
			RETERR(addrdata(&cdnskey_delete, diff, origin, ttl,
					mctx));
		}
	} else {
		if (dns_rdataset_isassociated(cdnskey) &&
		    exists(cdnskey, &cdnskey_delete))
		{
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
				      "CDNSKEY (DELETE) for zone %s is now "
				      "deleted",
				      namebuf);
			RETERR(delrdata(&cdnskey_delete, diff, origin,
					cdnskey->ttl, mctx));
		}
	}

	result = ISC_R_SUCCESS;

failure:
	return (result);
}

/*
 * Update 'keys' with information from 'newkeys'.
 *
 * If 'removed' is not NULL, any keys that are being removed from
 * the zone will be added to the list for post-removal processing.
 */
isc_result_t
dns_dnssec_updatekeys(dns_dnsseckeylist_t *keys, dns_dnsseckeylist_t *newkeys,
		      dns_dnsseckeylist_t *removed, const dns_name_t *origin,
		      dns_ttl_t hint_ttl, dns_diff_t *diff, isc_mem_t *mctx,
		      void (*report)(const char *, ...)) {
	isc_result_t result;
	dns_dnsseckey_t *key, *key1, *key2, *next;
	bool found_ttl = false;
	dns_ttl_t ttl = hint_ttl;

	/*
	 * First, look through the existing key list to find keys
	 * supplied from the command line which are not in the zone.
	 * Update the zone to include them.
	 *
	 * Also, if there are keys published in the zone already,
	 * use their TTL for all subsequent published keys.
	 */
	for (key = ISC_LIST_HEAD(*keys); key != NULL;
	     key = ISC_LIST_NEXT(key, link))
	{
		if (key->source == dns_keysource_user &&
		    (key->hint_publish || key->force_publish))
		{
			RETERR(publish_key(diff, key, origin, ttl, mctx,
					   report));
		}
		if (key->source == dns_keysource_zoneapex) {
			ttl = dst_key_getttl(key->key);
			found_ttl = true;
		}
	}

	/*
	 * If there were no existing keys, use the smallest nonzero
	 * TTL of the keys found in the repository.
	 */
	if (!found_ttl && !ISC_LIST_EMPTY(*newkeys)) {
		dns_ttl_t shortest = 0;

		for (key = ISC_LIST_HEAD(*newkeys); key != NULL;
		     key = ISC_LIST_NEXT(key, link))
		{
			dns_ttl_t thisttl = dst_key_getttl(key->key);
			if (thisttl != 0 &&
			    (shortest == 0 || thisttl < shortest))
			{
				shortest = thisttl;
			}
		}

		if (shortest != 0) {
			ttl = shortest;
		}
	}

	/*
	 * Second, scan the list of newly found keys looking for matches
	 * with known keys, and update accordingly.
	 */
	for (key1 = ISC_LIST_HEAD(*newkeys); key1 != NULL; key1 = next) {
		bool key_revoked = false;
		char keystr1[DST_KEY_FORMATSIZE];
		char keystr2[DST_KEY_FORMATSIZE];

		next = ISC_LIST_NEXT(key1, link);

		for (key2 = ISC_LIST_HEAD(*keys); key2 != NULL;
		     key2 = ISC_LIST_NEXT(key2, link))
		{
			int f1 = dst_key_flags(key1->key);
			int f2 = dst_key_flags(key2->key);
			int nr1 = f1 & ~DNS_KEYFLAG_REVOKE;
			int nr2 = f2 & ~DNS_KEYFLAG_REVOKE;
			if (nr1 == nr2 &&
			    dst_key_alg(key1->key) == dst_key_alg(key2->key) &&
			    dst_key_pubcompare(key1->key, key2->key, true))
			{
				int r1, r2;
				r1 = dst_key_flags(key1->key) &
				     DNS_KEYFLAG_REVOKE;
				r2 = dst_key_flags(key2->key) &
				     DNS_KEYFLAG_REVOKE;
				key_revoked = (r1 != r2);
				break;
			}
		}

		/* Printable version of key1 (the newly acquired key) */
		dst_key_format(key1->key, keystr1, sizeof(keystr1));

		/* No match found in keys; add the new key. */
		if (key2 == NULL) {
			ISC_LIST_UNLINK(*newkeys, key1, link);
			ISC_LIST_APPEND(*keys, key1, link);

			if (key1->source != dns_keysource_zoneapex &&
			    (key1->hint_publish || key1->force_publish))
			{
				RETERR(publish_key(diff, key1, origin, ttl,
						   mctx, report));
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"DNSKEY %s (%s) is now published",
					keystr1,
					key1->ksk ? (key1->zsk ? "CSK" : "KSK")
						  : "ZSK");
				if (key1->hint_sign || key1->force_sign) {
					key1->first_sign = true;
					isc_log_write(
						dns_lctx,
						DNS_LOGCATEGORY_DNSSEC,
						DNS_LOGMODULE_DNSSEC,
						ISC_LOG_INFO,
						"DNSKEY %s (%s) is now "
						"active",
						keystr1,
						key1->ksk ? (key1->zsk ? "CSK"
								       : "KSK")
							  : "ZSK");
				}
			}

			continue;
		}

		/* Printable version of key2 (the old key, if any) */
		dst_key_format(key2->key, keystr2, sizeof(keystr2));

		/* Copy key metadata. */
		dst_key_copy_metadata(key2->key, key1->key);

		/* Match found: remove or update it as needed */
		if (key1->hint_remove) {
			RETERR(remove_key(diff, key2, origin, ttl, mctx,
					  "expired", report));
			ISC_LIST_UNLINK(*keys, key2, link);

			if (removed != NULL) {
				ISC_LIST_APPEND(*removed, key2, link);
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"DNSKEY %s (%s) is now deleted",
					keystr2,
					key2->ksk ? (key2->zsk ? "CSK" : "KSK")
						  : "ZSK");
			} else {
				dns_dnsseckey_destroy(mctx, &key2);
			}
		} else if (key_revoked &&
			   (dst_key_flags(key1->key) & DNS_KEYFLAG_REVOKE) != 0)
		{
			/*
			 * A previously valid key has been revoked.
			 * We need to remove the old version and pull
			 * in the new one.
			 */
			RETERR(remove_key(diff, key2, origin, ttl, mctx,
					  "revoked", report));
			ISC_LIST_UNLINK(*keys, key2, link);
			if (removed != NULL) {
				ISC_LIST_APPEND(*removed, key2, link);
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"DNSKEY %s (%s) is now revoked; "
					"new ID is %05d",
					keystr2,
					key2->ksk ? (key2->zsk ? "CSK" : "KSK")
						  : "ZSK",
					dst_key_id(key1->key));
			} else {
				dns_dnsseckey_destroy(mctx, &key2);
			}

			RETERR(publish_key(diff, key1, origin, ttl, mctx,
					   report));
			ISC_LIST_UNLINK(*newkeys, key1, link);
			ISC_LIST_APPEND(*keys, key1, link);

			/*
			 * XXX: The revoke flag is only defined for trust
			 * anchors.  Setting the flag on a non-KSK is legal,
			 * but not defined in any RFC.  It seems reasonable
			 * to treat it the same as a KSK: keep it in the
			 * zone, sign the DNSKEY set with it, but not
			 * sign other records with it.
			 */
			key1->ksk = true;
			continue;
		} else {
			if (!key2->is_active &&
			    (key1->hint_sign || key1->force_sign))
			{
				key2->first_sign = true;
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"DNSKEY %s (%s) is now active", keystr1,
					key1->ksk ? (key1->zsk ? "CSK" : "KSK")
						  : "ZSK");
			} else if (key2->is_active && !key1->hint_sign &&
				   !key1->force_sign)
			{
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
					"DNSKEY %s (%s) is now inactive",
					keystr1,
					key1->ksk ? (key1->zsk ? "CSK" : "KSK")
						  : "ZSK");
			}

			key2->hint_sign = key1->hint_sign;
			key2->hint_publish = key1->hint_publish;
		}
	}

	/* Free any leftover keys in newkeys */
	while (!ISC_LIST_EMPTY(*newkeys)) {
		key1 = ISC_LIST_HEAD(*newkeys);
		ISC_LIST_UNLINK(*newkeys, key1, link);
		dns_dnsseckey_destroy(mctx, &key1);
	}

	result = ISC_R_SUCCESS;

failure:
	return (result);
}

isc_result_t
dns_dnssec_matchdskey(dns_name_t *name, dns_rdata_t *dsrdata,
		      dns_rdataset_t *keyset, dns_rdata_t *keyrdata) {
	isc_result_t result;
	unsigned char buf[DNS_DS_BUFFERSIZE];
	dns_keytag_t keytag;
	dns_rdata_dnskey_t key;
	dns_rdata_ds_t ds;
	isc_region_t r;

	result = dns_rdata_tostruct(dsrdata, &ds, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	for (result = dns_rdataset_first(keyset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(keyset))
	{
		dns_rdata_t newdsrdata = DNS_RDATA_INIT;

		dns_rdata_reset(keyrdata);
		dns_rdataset_current(keyset, keyrdata);

		result = dns_rdata_tostruct(keyrdata, &key, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		dns_rdata_toregion(keyrdata, &r);
		keytag = dst_region_computeid(&r);

		if (ds.key_tag != keytag || ds.algorithm != key.algorithm) {
			continue;
		}

		result = dns_ds_buildrdata(name, keyrdata, ds.digest_type, buf,
					   &newdsrdata);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		if (dns_rdata_compare(dsrdata, &newdsrdata) == 0) {
			break;
		}
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_NOTFOUND;
	}

	return (result);
}
