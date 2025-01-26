/*	$NetBSD: tsig.c,v 1.14 2025/01/26 16:25:25 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/hashmap.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/serial.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/tsig.h>

#include "tsig_p.h"

#define TSIGKEYRING_MAGIC    ISC_MAGIC('T', 'K', 'R', 'g')
#define VALID_TSIGKEYRING(x) ISC_MAGIC_VALID(x, TSIGKEYRING_MAGIC)

#define TSIG_MAGIC	 ISC_MAGIC('T', 'S', 'I', 'G')
#define VALID_TSIGKEY(x) ISC_MAGIC_VALID(x, TSIG_MAGIC)

#define is_response(msg) ((msg->flags & DNS_MESSAGEFLAG_QR) != 0)

#define BADTIMELEN 6

static unsigned char hmacmd5_ndata[] = "\010hmac-md5\007sig-alg\003reg\003int";
static unsigned char hmacmd5_offsets[] = { 0, 9, 17, 21, 25 };

static dns_name_t const hmacmd5 = DNS_NAME_INITABSOLUTE(hmacmd5_ndata,
							hmacmd5_offsets);
const dns_name_t *dns_tsig_hmacmd5_name = &hmacmd5;

static unsigned char gsstsig_ndata[] = "\010gss-tsig";
static unsigned char gsstsig_offsets[] = { 0, 9 };
static dns_name_t const gsstsig = DNS_NAME_INITABSOLUTE(gsstsig_ndata,
							gsstsig_offsets);
const dns_name_t *dns_tsig_gssapi_name = &gsstsig;

static unsigned char hmacsha1_ndata[] = "\011hmac-sha1";
static unsigned char hmacsha1_offsets[] = { 0, 10 };
static dns_name_t const hmacsha1 = DNS_NAME_INITABSOLUTE(hmacsha1_ndata,
							 hmacsha1_offsets);
const dns_name_t *dns_tsig_hmacsha1_name = &hmacsha1;

static unsigned char hmacsha224_ndata[] = "\013hmac-sha224";
static unsigned char hmacsha224_offsets[] = { 0, 12 };
static dns_name_t const hmacsha224 = DNS_NAME_INITABSOLUTE(hmacsha224_ndata,
							   hmacsha224_offsets);
const dns_name_t *dns_tsig_hmacsha224_name = &hmacsha224;

static unsigned char hmacsha256_ndata[] = "\013hmac-sha256";
static unsigned char hmacsha256_offsets[] = { 0, 12 };
static dns_name_t const hmacsha256 = DNS_NAME_INITABSOLUTE(hmacsha256_ndata,
							   hmacsha256_offsets);
const dns_name_t *dns_tsig_hmacsha256_name = &hmacsha256;

static unsigned char hmacsha384_ndata[] = "\013hmac-sha384";
static unsigned char hmacsha384_offsets[] = { 0, 12 };
static dns_name_t const hmacsha384 = DNS_NAME_INITABSOLUTE(hmacsha384_ndata,
							   hmacsha384_offsets);
const dns_name_t *dns_tsig_hmacsha384_name = &hmacsha384;

static unsigned char hmacsha512_ndata[] = "\013hmac-sha512";
static unsigned char hmacsha512_offsets[] = { 0, 12 };
static dns_name_t const hmacsha512 = DNS_NAME_INITABSOLUTE(hmacsha512_ndata,
							   hmacsha512_offsets);
const dns_name_t *dns_tsig_hmacsha512_name = &hmacsha512;

static const struct {
	const dns_name_t *name;
	unsigned int dstalg;
} known_algs[] = { { &hmacmd5, DST_ALG_HMACMD5 },
		   { &gsstsig, DST_ALG_GSSAPI },
		   { &hmacsha1, DST_ALG_HMACSHA1 },
		   { &hmacsha224, DST_ALG_HMACSHA224 },
		   { &hmacsha256, DST_ALG_HMACSHA256 },
		   { &hmacsha384, DST_ALG_HMACSHA384 },
		   { &hmacsha512, DST_ALG_HMACSHA512 } };

static isc_result_t
tsig_verify_tcp(isc_buffer_t *source, dns_message_t *msg);

static void
tsig_log(dns_tsigkey_t *key, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

bool
dns__tsig_algvalid(unsigned int alg) {
	return alg == DST_ALG_HMACMD5 || alg == DST_ALG_HMACSHA1 ||
	       alg == DST_ALG_HMACSHA224 || alg == DST_ALG_HMACSHA256 ||
	       alg == DST_ALG_HMACSHA384 || alg == DST_ALG_HMACSHA512;
}

static void
tsig_log(dns_tsigkey_t *key, int level, const char *fmt, ...) {
	va_list ap;
	char message[4096];
	char namestr[DNS_NAME_FORMATSIZE];
	char creatorstr[DNS_NAME_FORMATSIZE];

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}
	if (key != NULL) {
		dns_name_format(key->name, namestr, sizeof(namestr));
	} else {
		strlcpy(namestr, "<null>", sizeof(namestr));
	}

	if (key != NULL && key->generated && key->creator != NULL) {
		dns_name_format(key->creator, creatorstr, sizeof(creatorstr));
	} else {
		strlcpy(creatorstr, "<null>", sizeof(creatorstr));
	}

	va_start(ap, fmt);
	vsnprintf(message, sizeof(message), fmt, ap);
	va_end(ap);
	if (key != NULL && key->generated) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_TSIG, level,
			      "tsig key '%s' (%s): %s", namestr, creatorstr,
			      message);
	} else {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_TSIG, level, "tsig key '%s': %s",
			      namestr, message);
	}
}

static bool
tkey_match(void *node, const void *key) {
	dns_tsigkey_t *tkey = node;

	return dns_name_equal(tkey->name, key);
}

static bool
match_ptr(void *node, const void *key) {
	return node == key;
}

static void
rm_hashmap(dns_tsigkey_t *tkey) {
	REQUIRE(VALID_TSIGKEY(tkey));
	REQUIRE(VALID_TSIGKEYRING(tkey->ring));

	(void)isc_hashmap_delete(tkey->ring->keys, dns_name_hash(tkey->name),
				 match_ptr, tkey);
	dns_tsigkey_detach(&tkey);
}

static void
rm_lru(dns_tsigkey_t *tkey) {
	REQUIRE(VALID_TSIGKEY(tkey));
	REQUIRE(VALID_TSIGKEYRING(tkey->ring));

	if (tkey->generated && ISC_LINK_LINKED(tkey, link)) {
		ISC_LIST_UNLINK(tkey->ring->lru, tkey, link);
		tkey->ring->generated--;
		dns_tsigkey_unref(tkey);
	}
}

static void
adjust_lru(dns_tsigkey_t *tkey) {
	if (tkey->generated) {
		RWLOCK(&tkey->ring->lock, isc_rwlocktype_write);
		/*
		 * We may have been removed from the LRU list between
		 * removing the read lock and acquiring the write lock.
		 */
		if (ISC_LINK_LINKED(tkey, link) && tkey->ring->lru.tail != tkey)
		{
			ISC_LIST_UNLINK(tkey->ring->lru, tkey, link);
			ISC_LIST_APPEND(tkey->ring->lru, tkey, link);
		}
		RWUNLOCK(&tkey->ring->lock, isc_rwlocktype_write);
	}
}

static const dns_name_t *
namefromalg(dst_algorithm_t alg) {
	switch (alg) {
	case DST_ALG_HMACMD5:
		return dns_tsig_hmacmd5_name;
	case DST_ALG_HMACSHA1:
		return dns_tsig_hmacsha1_name;
	case DST_ALG_HMACSHA224:
		return dns_tsig_hmacsha224_name;
	case DST_ALG_HMACSHA256:
		return dns_tsig_hmacsha256_name;
	case DST_ALG_HMACSHA384:
		return dns_tsig_hmacsha384_name;
	case DST_ALG_HMACSHA512:
		return dns_tsig_hmacsha512_name;
	case DST_ALG_GSSAPI:
		return dns_tsig_gssapi_name;
	default:
		return NULL;
	}
}

isc_result_t
dns_tsigkey_createfromkey(const dns_name_t *name, dst_algorithm_t algorithm,
			  dst_key_t *dstkey, bool generated, bool restored,
			  const dns_name_t *creator, isc_stdtime_t inception,
			  isc_stdtime_t expire, isc_mem_t *mctx,
			  dns_tsigkey_t **keyp) {
	dns_tsigkey_t *tkey = NULL;
	isc_result_t result;

	REQUIRE(keyp != NULL && *keyp == NULL);
	REQUIRE(name != NULL);
	REQUIRE(mctx != NULL);

	tkey = isc_mem_get(mctx, sizeof(dns_tsigkey_t));
	*tkey = (dns_tsigkey_t){
		.generated = generated,
		.restored = restored,
		.inception = inception,
		.expire = expire,
		.link = ISC_LINK_INITIALIZER,
	};

	tkey->name = dns_fixedname_initname(&tkey->fn);
	dns_name_copy(name, tkey->name);
	(void)dns_name_downcase(tkey->name, tkey->name, NULL);

	if (algorithm != DST_ALG_UNKNOWN) {
		if (dstkey != NULL && dst_key_alg(dstkey) != algorithm) {
			result = DNS_R_BADALG;
			goto cleanup_name;
		}
	} else if (dstkey != NULL) {
		result = DNS_R_BADALG;
		goto cleanup_name;
	}

	tkey->algorithm = namefromalg(algorithm);

	if (creator != NULL) {
		tkey->creator = isc_mem_get(mctx, sizeof(dns_name_t));
		dns_name_init(tkey->creator, NULL);
		dns_name_dup(creator, mctx, tkey->creator);
	}

	if (dstkey != NULL) {
		dst_key_attach(dstkey, &tkey->key);
	}

	isc_refcount_init(&tkey->references, 1);
	isc_mem_attach(mctx, &tkey->mctx);

	/*
	 * Ignore this if it's a GSS key, since the key size is meaningless.
	 */
	if (dstkey != NULL && dst_key_size(dstkey) < 64 &&
	    algorithm != DST_ALG_GSSAPI)
	{
		char namestr[DNS_NAME_FORMATSIZE];
		dns_name_format(name, namestr, sizeof(namestr));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_TSIG, ISC_LOG_INFO,
			      "the key '%s' is too short to be secure",
			      namestr);
	}

	tkey->magic = TSIG_MAGIC;

	if (tkey->restored) {
		tsig_log(tkey, ISC_LOG_DEBUG(3), "restored from file");
	} else if (tkey->generated) {
		tsig_log(tkey, ISC_LOG_DEBUG(3), "generated");
	} else {
		tsig_log(tkey, ISC_LOG_DEBUG(3), "statically configured");
	}

	SET_IF_NOT_NULL(keyp, tkey);
	return ISC_R_SUCCESS;

cleanup_name:
	isc_mem_put(mctx, tkey, sizeof(dns_tsigkey_t));

	return result;
}

static void
destroyring(dns_tsigkeyring_t *ring) {
	isc_result_t result;
	isc_hashmap_iter_t *it = NULL;

	RWLOCK(&ring->lock, isc_rwlocktype_write);
	isc_hashmap_iter_create(ring->keys, &it);
	for (result = isc_hashmap_iter_first(it); result == ISC_R_SUCCESS;
	     result = isc_hashmap_iter_delcurrent_next(it))
	{
		dns_tsigkey_t *tkey = NULL;
		isc_hashmap_iter_current(it, (void **)&tkey);
		rm_lru(tkey);
		dns_tsigkey_detach(&tkey);
	}
	isc_hashmap_iter_destroy(&it);
	isc_hashmap_destroy(&ring->keys);
	RWUNLOCK(&ring->lock, isc_rwlocktype_write);

	ring->magic = 0;

	isc_rwlock_destroy(&ring->lock);
	isc_mem_putanddetach(&ring->mctx, ring, sizeof(dns_tsigkeyring_t));
}

#if DNS_TSIG_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_tsigkeyring, destroyring);
#else
ISC_REFCOUNT_IMPL(dns_tsigkeyring, destroyring);
#endif

/*
 * Look up the DST_ALG_ constant for a given name.
 */
dst_algorithm_t
dns__tsig_algfromname(const dns_name_t *algorithm) {
	for (size_t i = 0; i < ARRAY_SIZE(known_algs); ++i) {
		const dns_name_t *name = known_algs[i].name;
		if (algorithm == name || dns_name_equal(algorithm, name)) {
			return known_algs[i].dstalg;
		}
	}
	return DST_ALG_UNKNOWN;
}

static isc_result_t
restore_key(dns_tsigkeyring_t *ring, isc_stdtime_t now, FILE *fp) {
	dst_key_t *dstkey = NULL;
	char namestr[1024];
	char creatorstr[1024];
	char algorithmstr[1024];
	char keystr[4096];
	unsigned int inception, expire;
	int n;
	isc_buffer_t b;
	dns_name_t *name = NULL, *creator = NULL, *algorithm = NULL;
	dns_fixedname_t fname, fcreator, falgorithm;
	isc_result_t result;
	unsigned int dstalg;
	dns_tsigkey_t *tkey = NULL;

	n = fscanf(fp, "%1023s %1023s %u %u %1023s %4095s\n", namestr,
		   creatorstr, &inception, &expire, algorithmstr, keystr);
	if (n == EOF) {
		return ISC_R_NOMORE;
	}
	if (n != 6) {
		return ISC_R_FAILURE;
	}

	if (isc_serial_lt(expire, now)) {
		return DNS_R_EXPIRED;
	}

	name = dns_fixedname_initname(&fname);
	isc_buffer_init(&b, namestr, strlen(namestr));
	isc_buffer_add(&b, strlen(namestr));
	result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	creator = dns_fixedname_initname(&fcreator);
	isc_buffer_init(&b, creatorstr, strlen(creatorstr));
	isc_buffer_add(&b, strlen(creatorstr));
	result = dns_name_fromtext(creator, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	algorithm = dns_fixedname_initname(&falgorithm);
	isc_buffer_init(&b, algorithmstr, strlen(algorithmstr));
	isc_buffer_add(&b, strlen(algorithmstr));
	result = dns_name_fromtext(algorithm, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	dstalg = dns__tsig_algfromname(algorithm);
	if (dstalg == DST_ALG_UNKNOWN) {
		return DNS_R_BADALG;
	}

	result = dst_key_restore(name, dstalg, DNS_KEYOWNER_ENTITY,
				 DNS_KEYPROTO_DNSSEC, dns_rdataclass_in,
				 ring->mctx, keystr, &dstkey);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = dns_tsigkey_createfromkey(name, dstalg, dstkey, true, true,
					   creator, inception, expire,
					   ring->mctx, &tkey);
	if (result == ISC_R_SUCCESS) {
		result = dns_tsigkeyring_add(ring, tkey);
	}
	dns_tsigkey_detach(&tkey);
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	return result;
}

static void
dump_key(dns_tsigkey_t *tkey, FILE *fp) {
	char *buffer = NULL;
	int length = 0;
	char namestr[DNS_NAME_FORMATSIZE];
	char creatorstr[DNS_NAME_FORMATSIZE];
	char algorithmstr[DNS_NAME_FORMATSIZE];
	isc_result_t result;

	REQUIRE(tkey != NULL);
	REQUIRE(fp != NULL);

	dns_name_format(tkey->name, namestr, sizeof(namestr));
	dns_name_format(tkey->creator, creatorstr, sizeof(creatorstr));
	dns_name_format(tkey->algorithm, algorithmstr, sizeof(algorithmstr));
	result = dst_key_dump(tkey->key, tkey->mctx, &buffer, &length);
	if (result == ISC_R_SUCCESS) {
		fprintf(fp, "%s %s %u %u %s %.*s\n", namestr, creatorstr,
			tkey->inception, tkey->expire, algorithmstr, length,
			buffer);
	}
	if (buffer != NULL) {
		isc_mem_put(tkey->mctx, buffer, length);
	}
}

isc_result_t
dns_tsigkeyring_dump(dns_tsigkeyring_t *ring, FILE *fp) {
	isc_result_t result;
	isc_stdtime_t now = isc_stdtime_now();
	isc_hashmap_iter_t *it = NULL;
	bool found = false;

	REQUIRE(VALID_TSIGKEYRING(ring));

	RWLOCK(&ring->lock, isc_rwlocktype_read);
	isc_hashmap_iter_create(ring->keys, &it);
	for (result = isc_hashmap_iter_first(it); result == ISC_R_SUCCESS;
	     result = isc_hashmap_iter_next(it))
	{
		dns_tsigkey_t *tkey = NULL;
		isc_hashmap_iter_current(it, (void **)&tkey);

		if (tkey->generated && tkey->expire >= now) {
			dump_key(tkey, fp);
			found = true;
		}
	}
	isc_hashmap_iter_destroy(&it);
	RWUNLOCK(&ring->lock, isc_rwlocktype_read);

	return found ? ISC_R_SUCCESS : ISC_R_NOTFOUND;
}

const dns_name_t *
dns_tsigkey_identity(const dns_tsigkey_t *tsigkey) {
	REQUIRE(tsigkey == NULL || VALID_TSIGKEY(tsigkey));

	if (tsigkey == NULL) {
		return NULL;
	}
	if (tsigkey->generated) {
		return tsigkey->creator;
	} else {
		return tsigkey->name;
	}
}

isc_result_t
dns_tsigkey_create(const dns_name_t *name, dst_algorithm_t algorithm,
		   unsigned char *secret, int length, isc_mem_t *mctx,
		   dns_tsigkey_t **key) {
	dst_key_t *dstkey = NULL;
	isc_result_t result;

	REQUIRE(length >= 0);
	if (length > 0) {
		REQUIRE(secret != NULL);
	}

	if (dns__tsig_algvalid(algorithm)) {
		if (secret != NULL) {
			isc_buffer_t b;

			isc_buffer_init(&b, secret, length);
			isc_buffer_add(&b, length);
			result = dst_key_frombuffer(
				name, algorithm, DNS_KEYOWNER_ENTITY,
				DNS_KEYPROTO_DNSSEC, dns_rdataclass_in, &b,
				mctx, &dstkey);
			if (result != ISC_R_SUCCESS) {
				return result;
			}
		}
	} else if (length > 0) {
		return DNS_R_BADALG;
	}

	result = dns_tsigkey_createfromkey(name, algorithm, dstkey, false,
					   false, NULL, 0, 0, mctx, key);
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	return result;
}

static void
destroy_tsigkey(dns_tsigkey_t *key) {
	REQUIRE(VALID_TSIGKEY(key));

	key->magic = 0;
	if (key->key != NULL) {
		dst_key_free(&key->key);
	}
	if (key->creator != NULL) {
		dns_name_free(key->creator, key->mctx);
		isc_mem_put(key->mctx, key->creator, sizeof(dns_name_t));
	}
	isc_mem_putanddetach(&key->mctx, key, sizeof(dns_tsigkey_t));
}

#if DNS_TSIG_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_tsigkey, destroy_tsigkey);
#else
ISC_REFCOUNT_IMPL(dns_tsigkey, destroy_tsigkey);
#endif

void
dns_tsigkey_delete(dns_tsigkey_t *key) {
	REQUIRE(VALID_TSIGKEY(key));

	RWLOCK(&key->ring->lock, isc_rwlocktype_write);
	rm_lru(key);
	rm_hashmap(key);
	RWUNLOCK(&key->ring->lock, isc_rwlocktype_write);
}

isc_result_t
dns_tsig_sign(dns_message_t *msg) {
	dns_tsigkey_t *key = NULL;
	dns_rdata_any_tsig_t tsig, querytsig;
	unsigned char data[128];
	isc_buffer_t databuf, sigbuf;
	isc_buffer_t *dynbuf = NULL;
	dns_name_t *owner = NULL;
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *datalist = NULL;
	dns_rdataset_t *dataset = NULL;
	isc_region_t r;
	isc_stdtime_t now;
	isc_mem_t *mctx = NULL;
	dst_context_t *ctx = NULL;
	isc_result_t result;
	unsigned char badtimedata[BADTIMELEN];
	unsigned int sigsize = 0;
	bool response;

	REQUIRE(msg != NULL);
	key = dns_message_gettsigkey(msg);
	REQUIRE(VALID_TSIGKEY(key));

	/*
	 * If this is a response, there should be a TSIG in the query with the
	 * the exception if this is a TKEY request (see RFC 3645, Section 2.2).
	 */
	response = is_response(msg);
	if (response && msg->querytsig == NULL) {
		if (msg->tkey != 1) {
			return DNS_R_EXPECTEDTSIG;
		}
	}

	mctx = msg->mctx;

	now = msg->fuzzing ? msg->fuzztime : isc_stdtime_now();
	tsig = (dns_rdata_any_tsig_t){
		.mctx = mctx,
		.common.rdclass = dns_rdataclass_any,
		.common.rdtype = dns_rdatatype_tsig,
		.common.link = ISC_LINK_INITIALIZER,
		.timesigned = now + msg->timeadjust,
		.fudge = DNS_TSIG_FUDGE,
		.originalid = msg->id,
		.error = response ? msg->querytsigstatus : dns_rcode_noerror,
	};

	dns_name_init(&tsig.algorithm, NULL);
	dns_name_clone(key->algorithm, &tsig.algorithm);

	isc_buffer_init(&databuf, data, sizeof(data));

	if (tsig.error == dns_tsigerror_badtime) {
		isc_buffer_t otherbuf;

		tsig.otherlen = BADTIMELEN;
		tsig.other = badtimedata;
		isc_buffer_init(&otherbuf, tsig.other, tsig.otherlen);
		isc_buffer_putuint48(&otherbuf, tsig.timesigned);
	}

	if ((key->key != NULL) && (tsig.error != dns_tsigerror_badsig) &&
	    (tsig.error != dns_tsigerror_badkey))
	{
		unsigned char header[DNS_MESSAGE_HEADERLEN];
		isc_buffer_t headerbuf;
		uint16_t digestbits;
		bool querytsig_ok = false;

		/*
		 * If it is a response, we assume that the request MAC
		 * has validated at this point. This is why we include a
		 * MAC length > 0 in the reply.
		 */
		result = dst_context_create(
			key->key, mctx, DNS_LOGCATEGORY_DNSSEC, true, 0, &ctx);
		if (result != ISC_R_SUCCESS) {
			return result;
		}

		/*
		 * If this is a response, and if there was a TSIG in
		 * the query, digest the request's MAC.
		 *
		 * (Note: querytsig should be non-NULL for all
		 * responses except TKEY responses. Those may be signed
		 * with the newly-negotiated TSIG key even if the query
		 * wasn't signed.)
		 */
		if (response && msg->querytsig != NULL) {
			dns_rdata_t querytsigrdata = DNS_RDATA_INIT;

			INSIST(msg->verified_sig);

			result = dns_rdataset_first(msg->querytsig);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
			dns_rdataset_current(msg->querytsig, &querytsigrdata);
			result = dns_rdata_tostruct(&querytsigrdata, &querytsig,
						    NULL);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
			isc_buffer_putuint16(&databuf, querytsig.siglen);
			if (isc_buffer_availablelength(&databuf) <
			    querytsig.siglen)
			{
				result = ISC_R_NOSPACE;
				goto cleanup_context;
			}
			isc_buffer_putmem(&databuf, querytsig.signature,
					  querytsig.siglen);
			isc_buffer_usedregion(&databuf, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
			querytsig_ok = true;
		}

		/*
		 * Digest the header.
		 */
		isc_buffer_init(&headerbuf, header, sizeof(header));
		dns_message_renderheader(msg, &headerbuf);
		isc_buffer_usedregion(&headerbuf, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		/*
		 * Digest the remainder of the message.
		 */
		isc_buffer_usedregion(msg->buffer, &r);
		isc_region_consume(&r, DNS_MESSAGE_HEADERLEN);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		if (msg->tcp_continuation == 0) {
			/*
			 * Digest the name, class, ttl, alg.
			 */
			dns_name_toregion(key->name, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}

			isc_buffer_clear(&databuf);
			isc_buffer_putuint16(&databuf, dns_rdataclass_any);
			isc_buffer_putuint32(&databuf, 0); /* ttl */
			isc_buffer_usedregion(&databuf, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}

			dns_name_toregion(&tsig.algorithm, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
		}
		/* Digest the timesigned and fudge */
		isc_buffer_clear(&databuf);
		if (tsig.error == dns_tsigerror_badtime && querytsig_ok) {
			tsig.timesigned = querytsig.timesigned;
		}
		isc_buffer_putuint48(&databuf, tsig.timesigned);
		isc_buffer_putuint16(&databuf, tsig.fudge);
		isc_buffer_usedregion(&databuf, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		if (msg->tcp_continuation == 0) {
			/*
			 * Digest the error and other data length.
			 */
			isc_buffer_clear(&databuf);
			isc_buffer_putuint16(&databuf, tsig.error);
			isc_buffer_putuint16(&databuf, tsig.otherlen);

			isc_buffer_usedregion(&databuf, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}

			/*
			 * Digest other data.
			 */
			if (tsig.otherlen > 0) {
				r.length = tsig.otherlen;
				r.base = tsig.other;
				result = dst_context_adddata(ctx, &r);
				if (result != ISC_R_SUCCESS) {
					goto cleanup_context;
				}
			}
		}

		result = dst_key_sigsize(key->key, &sigsize);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}
		tsig.signature = isc_mem_get(mctx, sigsize);

		isc_buffer_init(&sigbuf, tsig.signature, sigsize);
		result = dst_context_sign(ctx, &sigbuf);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_signature;
		}
		dst_context_destroy(&ctx);
		digestbits = dst_key_getbits(key->key);
		if (digestbits != 0) {
			unsigned int bytes = (digestbits + 7) / 8;
			if (querytsig_ok && bytes < querytsig.siglen) {
				bytes = querytsig.siglen;
			}
			if (bytes > isc_buffer_usedlength(&sigbuf)) {
				bytes = isc_buffer_usedlength(&sigbuf);
			}
			tsig.siglen = bytes;
		} else {
			tsig.siglen = isc_buffer_usedlength(&sigbuf);
		}
	} else {
		tsig.siglen = 0;
		tsig.signature = NULL;
	}

	dns_message_gettemprdata(msg, &rdata);
	isc_buffer_allocate(msg->mctx, &dynbuf, 512);
	result = dns_rdata_fromstruct(rdata, dns_rdataclass_any,
				      dns_rdatatype_tsig, &tsig, dynbuf);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_dynbuf;
	}

	dns_message_takebuffer(msg, &dynbuf);

	if (tsig.signature != NULL) {
		isc_mem_put(mctx, tsig.signature, sigsize);
		tsig.signature = NULL;
	}

	dns_message_gettempname(msg, &owner);
	dns_name_copy(key->name, owner);

	dns_message_gettemprdatalist(msg, &datalist);

	dns_message_gettemprdataset(msg, &dataset);
	datalist->rdclass = dns_rdataclass_any;
	datalist->type = dns_rdatatype_tsig;
	ISC_LIST_APPEND(datalist->rdata, rdata, link);
	dns_rdatalist_tordataset(datalist, dataset);
	msg->tsig = dataset;
	msg->tsigname = owner;

	/* Windows does not like the tsig name being compressed. */
	msg->tsigname->attributes.nocompress = true;

	return ISC_R_SUCCESS;

cleanup_dynbuf:
	isc_buffer_free(&dynbuf);
	dns_message_puttemprdata(msg, &rdata);
cleanup_signature:
	if (tsig.signature != NULL) {
		isc_mem_put(mctx, tsig.signature, sigsize);
	}
cleanup_context:
	if (ctx != NULL) {
		dst_context_destroy(&ctx);
	}
	return result;
}

isc_result_t
dns_tsig_verify(isc_buffer_t *source, dns_message_t *msg,
		dns_tsigkeyring_t *ring1, dns_tsigkeyring_t *ring2) {
	dns_rdata_any_tsig_t tsig, querytsig;
	isc_region_t r, source_r, header_r, sig_r;
	isc_buffer_t databuf;
	unsigned char data[32];
	dns_name_t *keyname = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_stdtime_t now;
	isc_result_t result;
	dns_tsigkey_t *tsigkey = NULL;
	dst_key_t *key = NULL;
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	dst_context_t *ctx = NULL;
	isc_mem_t *mctx = NULL;
	uint16_t addcount, id;
	unsigned int siglen;
	unsigned int alg;
	bool response;

	REQUIRE(source != NULL);
	REQUIRE(DNS_MESSAGE_VALID(msg));
	tsigkey = dns_message_gettsigkey(msg);
	response = is_response(msg);

	REQUIRE(tsigkey == NULL || VALID_TSIGKEY(tsigkey));

	msg->verify_attempted = 1;
	msg->verified_sig = 0;
	msg->tsigstatus = dns_tsigerror_badsig;

	if (msg->tcp_continuation) {
		if (tsigkey == NULL || msg->querytsig == NULL) {
			return DNS_R_UNEXPECTEDTSIG;
		}
		return tsig_verify_tcp(source, msg);
	}

	/*
	 * There should be a TSIG record...
	 */
	if (msg->tsig == NULL) {
		return DNS_R_EXPECTEDTSIG;
	}

	/*
	 * If this is a response and there's no key or query TSIG, there
	 * shouldn't be one on the response.
	 */
	if (response && (tsigkey == NULL || msg->querytsig == NULL)) {
		return DNS_R_UNEXPECTEDTSIG;
	}

	mctx = msg->mctx;

	/*
	 * If we're here, we know the message is well formed and contains a
	 * TSIG record.
	 */

	keyname = msg->tsigname;
	result = dns_rdataset_first(msg->tsig);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	dns_rdataset_current(msg->tsig, &rdata);
	result = dns_rdata_tostruct(&rdata, &tsig, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	dns_rdata_reset(&rdata);
	if (response) {
		result = dns_rdataset_first(msg->querytsig);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_rdataset_current(msg->querytsig, &rdata);
		result = dns_rdata_tostruct(&rdata, &querytsig, NULL);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}

	/*
	 * Do the key name and algorithm match that of the query?
	 */
	if (response &&
	    (!dns_name_equal(keyname, tsigkey->name) ||
	     !dns_name_equal(&tsig.algorithm, &querytsig.algorithm)))
	{
		msg->tsigstatus = dns_tsigerror_badkey;
		tsig_log(msg->tsigkey, 2,
			 "key name and algorithm do not match");
		return DNS_R_TSIGVERIFYFAILURE;
	}

	/*
	 * Get the current time.
	 */
	if (msg->fuzzing) {
		now = msg->fuzztime;
	} else {
		now = isc_stdtime_now();
	}

	/*
	 * Find dns_tsigkey_t based on keyname.
	 */
	if (tsigkey == NULL) {
		result = ISC_R_NOTFOUND;
		if (ring1 != NULL) {
			result = dns_tsigkey_find(&tsigkey, keyname,
						  &tsig.algorithm, ring1);
		}
		if (result == ISC_R_NOTFOUND && ring2 != NULL) {
			result = dns_tsigkey_find(&tsigkey, keyname,
						  &tsig.algorithm, ring2);
		}
		if (result != ISC_R_SUCCESS) {
			msg->tsigstatus = dns_tsigerror_badkey;
			result = dns_tsigkey_create(
				keyname, dns__tsig_algfromname(&tsig.algorithm),
				NULL, 0, mctx, &msg->tsigkey);
			if (result != ISC_R_SUCCESS) {
				return result;
			}
			tsig_log(msg->tsigkey, 2, "unknown key");
			return DNS_R_TSIGVERIFYFAILURE;
		}
		msg->tsigkey = tsigkey;
	}

	key = tsigkey->key;

	/*
	 * Check digest length.
	 */
	alg = dst_key_alg(key);
	result = dst_key_sigsize(key, &siglen);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	if (dns__tsig_algvalid(alg)) {
		if (tsig.siglen > siglen) {
			tsig_log(msg->tsigkey, 2, "signature length too big");
			return DNS_R_FORMERR;
		}
		if (tsig.siglen > 0 &&
		    (tsig.siglen < 10 || tsig.siglen < ((siglen + 1) / 2)))
		{
			tsig_log(msg->tsigkey, 2,
				 "signature length below minimum");
			return DNS_R_FORMERR;
		}
	}

	if (tsig.siglen > 0) {
		uint16_t addcount_n;

		sig_r.base = tsig.signature;
		sig_r.length = tsig.siglen;

		result = dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC,
					    false, 0, &ctx);
		if (result != ISC_R_SUCCESS) {
			return result;
		}

		if (response) {
			isc_buffer_init(&databuf, data, sizeof(data));
			isc_buffer_putuint16(&databuf, querytsig.siglen);
			isc_buffer_usedregion(&databuf, &r);
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
			if (querytsig.siglen > 0) {
				r.length = querytsig.siglen;
				r.base = querytsig.signature;
				result = dst_context_adddata(ctx, &r);
				if (result != ISC_R_SUCCESS) {
					goto cleanup_context;
				}
			}
		}

		/*
		 * Extract the header.
		 */
		isc_buffer_usedregion(source, &r);
		memmove(header, r.base, DNS_MESSAGE_HEADERLEN);
		isc_region_consume(&r, DNS_MESSAGE_HEADERLEN);

		/*
		 * Decrement the additional field counter.
		 */
		memmove(&addcount, &header[DNS_MESSAGE_HEADERLEN - 2], 2);
		addcount_n = ntohs(addcount);
		addcount = htons((uint16_t)(addcount_n - 1));
		memmove(&header[DNS_MESSAGE_HEADERLEN - 2], &addcount, 2);

		/*
		 * Put in the original id.
		 */
		id = htons(tsig.originalid);
		memmove(&header[0], &id, 2);

		/*
		 * Digest the modified header.
		 */
		header_r.base = (unsigned char *)header;
		header_r.length = DNS_MESSAGE_HEADERLEN;
		result = dst_context_adddata(ctx, &header_r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		/*
		 * Digest all non-TSIG records.
		 */
		isc_buffer_usedregion(source, &source_r);
		r.base = source_r.base + DNS_MESSAGE_HEADERLEN;
		r.length = msg->sigstart - DNS_MESSAGE_HEADERLEN;
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		/*
		 * Digest the key name.
		 */
		dns_name_toregion(tsigkey->name, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		isc_buffer_init(&databuf, data, sizeof(data));
		isc_buffer_putuint16(&databuf, tsig.common.rdclass);
		isc_buffer_putuint32(&databuf, msg->tsig->ttl);
		isc_buffer_usedregion(&databuf, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		/*
		 * Digest the key algorithm.
		 */
		dns_name_toregion(tsigkey->algorithm, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		isc_buffer_clear(&databuf);
		isc_buffer_putuint48(&databuf, tsig.timesigned);
		isc_buffer_putuint16(&databuf, tsig.fudge);
		isc_buffer_putuint16(&databuf, tsig.error);
		isc_buffer_putuint16(&databuf, tsig.otherlen);
		isc_buffer_usedregion(&databuf, &r);
		result = dst_context_adddata(ctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		if (tsig.otherlen > 0) {
			r.base = tsig.other;
			r.length = tsig.otherlen;
			result = dst_context_adddata(ctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
		}

		result = dst_context_verify(ctx, &sig_r);
		if (result == DST_R_VERIFYFAILURE) {
			result = DNS_R_TSIGVERIFYFAILURE;
			tsig_log(msg->tsigkey, 2,
				 "signature failed to verify(1)");
			goto cleanup_context;
		} else if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}
		msg->verified_sig = 1;
	} else if (!response || (tsig.error != dns_tsigerror_badsig &&
				 tsig.error != dns_tsigerror_badkey))
	{
		tsig_log(msg->tsigkey, 2, "signature was empty");
		return DNS_R_TSIGVERIFYFAILURE;
	}

	/*
	 * Here at this point, the MAC has been verified. Even if any of
	 * the following code returns a TSIG error, the reply will be
	 * signed and WILL always include the request MAC in the digest
	 * computation.
	 */

	/*
	 * Is the time ok?
	 */
	if (now + msg->timeadjust > tsig.timesigned + tsig.fudge) {
		msg->tsigstatus = dns_tsigerror_badtime;
		tsig_log(msg->tsigkey, 2, "signature has expired");
		result = DNS_R_CLOCKSKEW;
		goto cleanup_context;
	} else if (now + msg->timeadjust < tsig.timesigned - tsig.fudge) {
		msg->tsigstatus = dns_tsigerror_badtime;
		tsig_log(msg->tsigkey, 2, "signature is in the future");
		result = DNS_R_CLOCKSKEW;
		goto cleanup_context;
	}

	if (dns__tsig_algvalid(alg)) {
		uint16_t digestbits = dst_key_getbits(key);

		if (tsig.siglen > 0 && digestbits != 0 &&
		    tsig.siglen < ((digestbits + 7) / 8))
		{
			msg->tsigstatus = dns_tsigerror_badtrunc;
			tsig_log(msg->tsigkey, 2,
				 "truncated signature length too small");
			result = DNS_R_TSIGVERIFYFAILURE;
			goto cleanup_context;
		}
		if (tsig.siglen > 0 && digestbits == 0 && tsig.siglen < siglen)
		{
			msg->tsigstatus = dns_tsigerror_badtrunc;
			tsig_log(msg->tsigkey, 2, "signature length too small");
			result = DNS_R_TSIGVERIFYFAILURE;
			goto cleanup_context;
		}
	}

	if (response && tsig.error != dns_rcode_noerror) {
		msg->tsigstatus = tsig.error;
		if (tsig.error == dns_tsigerror_badtime) {
			result = DNS_R_CLOCKSKEW;
		} else {
			result = DNS_R_TSIGERRORSET;
		}
		goto cleanup_context;
	}

	msg->tsigstatus = dns_rcode_noerror;
	result = ISC_R_SUCCESS;

cleanup_context:
	if (ctx != NULL) {
		dst_context_destroy(&ctx);
	}

	return result;
}

static isc_result_t
tsig_verify_tcp(isc_buffer_t *source, dns_message_t *msg) {
	dns_rdata_any_tsig_t tsig, querytsig;
	isc_region_t r, source_r, header_r, sig_r;
	isc_buffer_t databuf;
	unsigned char data[32];
	dns_name_t *keyname = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_stdtime_t now;
	isc_result_t result;
	dns_tsigkey_t *tsigkey = NULL;
	dst_key_t *key = NULL;
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	uint16_t addcount, id;
	bool has_tsig = false;
	isc_mem_t *mctx = NULL;
	unsigned int siglen;
	unsigned int alg;

	REQUIRE(source != NULL);
	REQUIRE(msg != NULL);
	REQUIRE(dns_message_gettsigkey(msg) != NULL);
	REQUIRE(msg->tcp_continuation == 1);
	REQUIRE(msg->querytsig != NULL);

	msg->verified_sig = 0;
	msg->tsigstatus = dns_tsigerror_badsig;

	if (!is_response(msg)) {
		return DNS_R_EXPECTEDRESPONSE;
	}

	mctx = msg->mctx;

	tsigkey = dns_message_gettsigkey(msg);
	key = tsigkey->key;

	/*
	 * Extract and parse the previous TSIG
	 */
	result = dns_rdataset_first(msg->querytsig);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	dns_rdataset_current(msg->querytsig, &rdata);
	result = dns_rdata_tostruct(&rdata, &querytsig, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	dns_rdata_reset(&rdata);

	/*
	 * If there is a TSIG in this message, do some checks.
	 */
	if (msg->tsig != NULL) {
		has_tsig = true;

		keyname = msg->tsigname;
		result = dns_rdataset_first(msg->tsig);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_querystruct;
		}
		dns_rdataset_current(msg->tsig, &rdata);
		result = dns_rdata_tostruct(&rdata, &tsig, NULL);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_querystruct;
		}

		/*
		 * Do the key name and algorithm match that of the query?
		 */
		if (!dns_name_equal(keyname, tsigkey->name) ||
		    !dns_name_equal(&tsig.algorithm, &querytsig.algorithm))
		{
			msg->tsigstatus = dns_tsigerror_badkey;
			result = DNS_R_TSIGVERIFYFAILURE;
			tsig_log(msg->tsigkey, 2,
				 "key name and algorithm do not match");
			goto cleanup_querystruct;
		}

		/*
		 * Check digest length.
		 */
		alg = dst_key_alg(key);
		result = dst_key_sigsize(key, &siglen);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_querystruct;
		}
		if (dns__tsig_algvalid(alg)) {
			if (tsig.siglen > siglen) {
				tsig_log(tsigkey, 2,
					 "signature length too big");
				result = DNS_R_FORMERR;
				goto cleanup_querystruct;
			}
			if (tsig.siglen > 0 &&
			    (tsig.siglen < 10 ||
			     tsig.siglen < ((siglen + 1) / 2)))
			{
				tsig_log(tsigkey, 2,
					 "signature length below minimum");
				result = DNS_R_FORMERR;
				goto cleanup_querystruct;
			}
		}
	}

	if (msg->tsigctx == NULL) {
		result = dst_context_create(key, mctx, DNS_LOGCATEGORY_DNSSEC,
					    false, 0, &msg->tsigctx);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_querystruct;
		}

		/*
		 * Digest the length of the query signature
		 */
		isc_buffer_init(&databuf, data, sizeof(data));
		isc_buffer_putuint16(&databuf, querytsig.siglen);
		isc_buffer_usedregion(&databuf, &r);
		result = dst_context_adddata(msg->tsigctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		/*
		 * Digest the data of the query signature
		 */
		if (querytsig.siglen > 0) {
			r.length = querytsig.siglen;
			r.base = querytsig.signature;
			result = dst_context_adddata(msg->tsigctx, &r);
			if (result != ISC_R_SUCCESS) {
				goto cleanup_context;
			}
		}
	}

	/*
	 * Extract the header.
	 */
	isc_buffer_usedregion(source, &r);
	memmove(header, r.base, DNS_MESSAGE_HEADERLEN);
	isc_region_consume(&r, DNS_MESSAGE_HEADERLEN);

	/*
	 * Decrement the additional field counter if necessary.
	 */
	if (has_tsig) {
		uint16_t addcount_n;

		memmove(&addcount, &header[DNS_MESSAGE_HEADERLEN - 2], 2);
		addcount_n = ntohs(addcount);
		addcount = htons((uint16_t)(addcount_n - 1));
		memmove(&header[DNS_MESSAGE_HEADERLEN - 2], &addcount, 2);

		/*
		 * Put in the original id.
		 *
		 * XXX Can TCP transfers be forwarded?  How would that
		 * work?
		 */
		id = htons(tsig.originalid);
		memmove(&header[0], &id, 2);
	}

	/*
	 * Digest the modified header.
	 */
	header_r.base = (unsigned char *)header;
	header_r.length = DNS_MESSAGE_HEADERLEN;
	result = dst_context_adddata(msg->tsigctx, &header_r);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_context;
	}

	/*
	 * Digest all non-TSIG records.
	 */
	isc_buffer_usedregion(source, &source_r);
	r.base = source_r.base + DNS_MESSAGE_HEADERLEN;
	if (has_tsig) {
		r.length = msg->sigstart - DNS_MESSAGE_HEADERLEN;
	} else {
		r.length = source_r.length - DNS_MESSAGE_HEADERLEN;
	}
	result = dst_context_adddata(msg->tsigctx, &r);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_context;
	}

	/*
	 * Digest the time signed and fudge.
	 */
	if (has_tsig) {
		isc_buffer_init(&databuf, data, sizeof(data));
		isc_buffer_putuint48(&databuf, tsig.timesigned);
		isc_buffer_putuint16(&databuf, tsig.fudge);
		isc_buffer_usedregion(&databuf, &r);
		result = dst_context_adddata(msg->tsigctx, &r);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}

		sig_r.base = tsig.signature;
		sig_r.length = tsig.siglen;
		if (tsig.siglen == 0) {
			if (tsig.error != dns_rcode_noerror) {
				msg->tsigstatus = tsig.error;
				if (tsig.error == dns_tsigerror_badtime) {
					result = DNS_R_CLOCKSKEW;
				} else {
					result = DNS_R_TSIGERRORSET;
				}
			} else {
				tsig_log(msg->tsigkey, 2, "signature is empty");
				result = DNS_R_TSIGVERIFYFAILURE;
			}
			goto cleanup_context;
		}

		result = dst_context_verify(msg->tsigctx, &sig_r);
		if (result == DST_R_VERIFYFAILURE) {
			tsig_log(msg->tsigkey, 2,
				 "signature failed to verify(2)");
			result = DNS_R_TSIGVERIFYFAILURE;
			goto cleanup_context;
		} else if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}
		msg->verified_sig = 1;

		/*
		 * Here at this point, the MAC has been verified. Even
		 * if any of the following code returns a TSIG error,
		 * the reply will be signed and WILL always include the
		 * request MAC in the digest computation.
		 */

		/*
		 * Is the time ok?
		 */
		if (msg->fuzzing) {
			now = msg->fuzztime;
		} else {
			now = isc_stdtime_now();
		}

		if (now + msg->timeadjust > tsig.timesigned + tsig.fudge) {
			msg->tsigstatus = dns_tsigerror_badtime;
			tsig_log(msg->tsigkey, 2, "signature has expired");
			result = DNS_R_CLOCKSKEW;
			goto cleanup_context;
		} else if (now + msg->timeadjust < tsig.timesigned - tsig.fudge)
		{
			msg->tsigstatus = dns_tsigerror_badtime;
			tsig_log(msg->tsigkey, 2, "signature is in the future");
			result = DNS_R_CLOCKSKEW;
			goto cleanup_context;
		}

		alg = dst_key_alg(key);
		result = dst_key_sigsize(key, &siglen);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_context;
		}
		if (dns__tsig_algvalid(alg)) {
			uint16_t digestbits = dst_key_getbits(key);

			if (tsig.siglen > 0 && digestbits != 0 &&
			    tsig.siglen < ((digestbits + 7) / 8))
			{
				msg->tsigstatus = dns_tsigerror_badtrunc;
				tsig_log(msg->tsigkey, 2,
					 "truncated signature length "
					 "too small");
				result = DNS_R_TSIGVERIFYFAILURE;
				goto cleanup_context;
			}
			if (tsig.siglen > 0 && digestbits == 0 &&
			    tsig.siglen < siglen)
			{
				msg->tsigstatus = dns_tsigerror_badtrunc;
				tsig_log(msg->tsigkey, 2,
					 "signature length too small");
				result = DNS_R_TSIGVERIFYFAILURE;
				goto cleanup_context;
			}
		}

		if (tsig.error != dns_rcode_noerror) {
			msg->tsigstatus = tsig.error;
			if (tsig.error == dns_tsigerror_badtime) {
				result = DNS_R_CLOCKSKEW;
			} else {
				result = DNS_R_TSIGERRORSET;
			}
			goto cleanup_context;
		}
	}

	msg->tsigstatus = dns_rcode_noerror;
	result = ISC_R_SUCCESS;

cleanup_context:
	/*
	 * Except in error conditions, don't destroy the DST context
	 * for unsigned messages; it is a running sum till the next
	 * TSIG signed message.
	 */
	if ((result != ISC_R_SUCCESS || has_tsig) && msg->tsigctx != NULL) {
		dst_context_destroy(&msg->tsigctx);
	}

cleanup_querystruct:
	dns_rdata_freestruct(&querytsig);

	return result;
}

isc_result_t
dns_tsigkey_find(dns_tsigkey_t **tsigkey, const dns_name_t *name,
		 const dns_name_t *algorithm, dns_tsigkeyring_t *ring) {
	dns_tsigkey_t *key = NULL;
	isc_result_t result;
	isc_rwlocktype_t locktype = isc_rwlocktype_read;
	isc_stdtime_t now = isc_stdtime_now();

	REQUIRE(name != NULL);
	REQUIRE(VALID_TSIGKEYRING(ring));
	REQUIRE(tsigkey != NULL && *tsigkey == NULL);

again:
	RWLOCK(&ring->lock, locktype);
	result = isc_hashmap_find(ring->keys, dns_name_hash(name), tkey_match,
				  name, (void **)&key);
	if (result == ISC_R_NOTFOUND) {
		RWUNLOCK(&ring->lock, locktype);
		return result;
	}
	if (algorithm != NULL && !dns_name_equal(key->algorithm, algorithm)) {
		RWUNLOCK(&ring->lock, locktype);
		return ISC_R_NOTFOUND;
	}
	if (key->inception != key->expire && isc_serial_lt(key->expire, now)) {
		/*
		 * The key has expired.
		 */
		if (locktype == isc_rwlocktype_read) {
			RWUNLOCK(&ring->lock, locktype);
			locktype = isc_rwlocktype_write;
			key = NULL;
			goto again;
		}
		rm_lru(key);
		rm_hashmap(key);
		RWUNLOCK(&ring->lock, locktype);
		return ISC_R_NOTFOUND;
	}
	dns_tsigkey_ref(key);
	RWUNLOCK(&ring->lock, locktype);
	adjust_lru(key);
	*tsigkey = key;
	return ISC_R_SUCCESS;
}

void
dns_tsigkeyring_create(isc_mem_t *mctx, dns_tsigkeyring_t **ringp) {
	dns_tsigkeyring_t *ring = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(ringp != NULL && *ringp == NULL);

	ring = isc_mem_get(mctx, sizeof(dns_tsigkeyring_t));
	*ring = (dns_tsigkeyring_t){
		.lru = ISC_LIST_INITIALIZER,
	};

	isc_hashmap_create(mctx, 12, &ring->keys);
	isc_rwlock_init(&ring->lock);
	isc_mem_attach(mctx, &ring->mctx);
	isc_refcount_init(&ring->references, 1);
	ring->magic = TSIGKEYRING_MAGIC;

	*ringp = ring;
}

isc_result_t
dns_tsigkeyring_add(dns_tsigkeyring_t *ring, dns_tsigkey_t *tkey) {
	isc_result_t result;

	REQUIRE(VALID_TSIGKEY(tkey));
	REQUIRE(VALID_TSIGKEYRING(ring));
	REQUIRE(tkey->ring == NULL);

	RWLOCK(&ring->lock, isc_rwlocktype_write);
	result = isc_hashmap_add(ring->keys, dns_name_hash(tkey->name),
				 tkey_match, tkey->name, tkey, NULL);
	if (result == ISC_R_SUCCESS) {
		dns_tsigkey_ref(tkey);
		tkey->ring = ring;

		/*
		 * If this is a TKEY-generated key, add it to the LRU list,
		 * and if we've exceeded the quota for generated keys,
		 * remove the least recently used one from the both the
		 * list and the RBT.
		 */
		if (tkey->generated) {
			ISC_LIST_APPEND(ring->lru, tkey, link);
			dns_tsigkey_ref(tkey);
			if (ring->generated++ > DNS_TSIG_MAXGENERATEDKEYS) {
				dns_tsigkey_t *key = ISC_LIST_HEAD(ring->lru);
				rm_lru(key);
				rm_hashmap(key);
			}
		}

		tkey->ring = ring;
	}
	RWUNLOCK(&ring->lock, isc_rwlocktype_write);

	return result;
}

void
dns_tsigkeyring_restore(dns_tsigkeyring_t *ring, FILE *fp) {
	isc_stdtime_t now = isc_stdtime_now();
	isc_result_t result;

	do {
		result = restore_key(ring, now, fp);
		if (result == ISC_R_NOMORE) {
			return;
		}
		if (result == DNS_R_BADALG || result == DNS_R_EXPIRED) {
			result = ISC_R_SUCCESS;
		}
	} while (result == ISC_R_SUCCESS);
}
