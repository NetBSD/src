/*	$NetBSD: cc.c,v 1.8 2014/12/10 04:38:01 christos Exp $	*/

/*
 * Portions Copyright (C) 2004-2007, 2012, 2013  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 2001-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NOMINUM DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Portions Copyright (C) 2001  Nominum, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NOMINUM DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: cc.c,v 1.18 2007/08/28 07:20:43 tbox Exp  */

/*! \file */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <isc/assertions.h>
#include <isc/hmacmd5.h>
#include <isc/hmacsha.h>
#include <isc/print.h>
#include <isc/safe.h>
#include <isc/stdlib.h>

#include <isccc/alist.h>
#include <isccc/base64.h>
#include <isccc/cc.h>
#include <isccc/result.h>
#include <isccc/sexpr.h>
#include <isccc/symtab.h>
#include <isccc/symtype.h>
#include <isccc/util.h>

#define MAX_TAGS		256
#define DUP_LIFETIME		900

typedef isccc_sexpr_t *sexpr_ptr;

static unsigned char auth_hmd5[] = {
	0x05, 0x5f, 0x61, 0x75, 0x74, 0x68,		/*%< len + _auth */
	ISCCC_CCMSGTYPE_TABLE,				/*%< message type */
	0x00, 0x00, 0x00, 0x20,				/*%< length == 32 */
	0x04, 0x68, 0x6d, 0x64, 0x35,			/*%< len + hmd5 */
	ISCCC_CCMSGTYPE_BINARYDATA,			/*%< message type */
	0x00, 0x00, 0x00, 0x16,				/*%< length == 22 */
	/*
	 * The base64 encoding of one of our HMAC-MD5 signatures is
	 * 22 bytes.
	 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define HMD5_OFFSET	21		/*%< 21 = 6 + 1 + 4 + 5 + 1 + 4 */
#define HMD5_LENGTH	22

static unsigned char auth_hsha[] = {
	0x05, 0x5f, 0x61, 0x75, 0x74, 0x68,		/*%< len + _auth */
	ISCCC_CCMSGTYPE_TABLE,				/*%< message type */
	0x00, 0x00, 0x00, 0x63,				/*%< length == 99 */
	0x04, 0x68, 0x73, 0x68, 0x61,			/*%< len + hsha */
	ISCCC_CCMSGTYPE_BINARYDATA,			/*%< message type */
	0x00, 0x00, 0x00, 0x59,				/*%< length == 89 */
	0x00,						/*%< algorithm */
	/*
	 * The base64 encoding of one of our HMAC-SHA* signatures is
	 * 88 bytes.
	 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define HSHA_OFFSET	22		/*%< 21 = 6 + 1 + 4 + 5 + 1 + 4 + 1 */
#define HSHA_LENGTH	88

static isc_result_t
table_towire(isccc_sexpr_t *alist, isccc_region_t *target);

static isc_result_t
list_towire(isccc_sexpr_t *alist, isccc_region_t *target);

static isc_result_t
value_towire(isccc_sexpr_t *elt, isccc_region_t *target)
{
	unsigned int len;
	unsigned char *lenp;
	isccc_region_t *vr;
	isc_result_t result;

	if (isccc_sexpr_binaryp(elt)) {
		vr = isccc_sexpr_tobinary(elt);
		len = REGION_SIZE(*vr);
		if (REGION_SIZE(*target) < 1 + 4 + len)
			return (ISC_R_NOSPACE);
		PUT8(ISCCC_CCMSGTYPE_BINARYDATA, target->rstart);
		PUT32(len, target->rstart);
		if (REGION_SIZE(*target) < len)
			return (ISC_R_NOSPACE);
		PUT_MEM(vr->rstart, len, target->rstart);
	} else if (isccc_alist_alistp(elt)) {
		if (REGION_SIZE(*target) < 1 + 4)
			return (ISC_R_NOSPACE);
		PUT8(ISCCC_CCMSGTYPE_TABLE, target->rstart);
		/*
		 * Emit a placeholder length.
		 */
		lenp = target->rstart;
		PUT32(0, target->rstart);
		/*
		 * Emit the table.
		 */
		result = table_towire(elt, target);
		if (result != ISC_R_SUCCESS)
			return (result);
		len = (unsigned int)(target->rstart - lenp);
		/*
		 * 'len' is 4 bytes too big, since it counts
		 * the placeholder length too.  Adjust and
		 * emit.
		 */
		INSIST(len >= 4U);
		len -= 4;
		PUT32(len, lenp);
	} else if (isccc_sexpr_listp(elt)) {
		if (REGION_SIZE(*target) < 1 + 4)
			return (ISC_R_NOSPACE);
		PUT8(ISCCC_CCMSGTYPE_LIST, target->rstart);
		/*
		 * Emit a placeholder length and count.
		 */
		lenp = target->rstart;
		PUT32(0, target->rstart);
		/*
		 * Emit the list.
		 */
		result = list_towire(elt, target);
		if (result != ISC_R_SUCCESS)
			return (result);
		len = (unsigned int)(target->rstart - lenp);
		/*
		 * 'len' is 4 bytes too big, since it counts
		 * the placeholder length.  Adjust and emit.
		 */
		INSIST(len >= 4U);
		len -= 4;
		PUT32(len, lenp);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
table_towire(isccc_sexpr_t *alist, isccc_region_t *target)
{
	isccc_sexpr_t *kv, *elt, *k, *v;
	char *ks;
	isc_result_t result;
	size_t len;

	for (elt = isccc_alist_first(alist);
	     elt != NULL;
	     elt = ISCCC_SEXPR_CDR(elt)) {
		kv = ISCCC_SEXPR_CAR(elt);
		k = ISCCC_SEXPR_CAR(kv);
		ks = isccc_sexpr_tostring(k);
		v = ISCCC_SEXPR_CDR(kv);
		len = strlen(ks);
		INSIST(len <= 255U);
		/*
		 * Emit the key name.
		 */
		if (REGION_SIZE(*target) < 1 + len)
			return (ISC_R_NOSPACE);
		PUT8(len, target->rstart);
		PUT_MEM(ks, len, target->rstart);
		/*
		 * Emit the value.
		 */
		result = value_towire(v, target);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
list_towire(isccc_sexpr_t *list, isccc_region_t *target)
{
	isc_result_t result;

	while (list != NULL) {
		result = value_towire(ISCCC_SEXPR_CAR(list), target);
		if (result != ISC_R_SUCCESS)
			return (result);
		list = ISCCC_SEXPR_CDR(list);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
sign(unsigned char *data, unsigned int length, unsigned char *hmac,
     isc_uint32_t algorithm, isccc_region_t *secret)
{
	union {
		isc_hmacmd5_t hmd5;
		isc_hmacsha1_t hsha;
		isc_hmacsha224_t h224;
		isc_hmacsha256_t h256;
		isc_hmacsha384_t h384;
		isc_hmacsha512_t h512;
	} ctx;
	isc_result_t result;
	isccc_region_t source, target;
	unsigned char digest[ISC_SHA512_DIGESTLENGTH];
	unsigned char digestb64[HSHA_LENGTH + 4];

	source.rstart = digest;

	switch (algorithm) {
	case ISCCC_ALG_HMACMD5:
		isc_hmacmd5_init(&ctx.hmd5, secret->rstart,
				 REGION_SIZE(*secret));
		isc_hmacmd5_update(&ctx.hmd5, data, length);
		isc_hmacmd5_sign(&ctx.hmd5, digest);
		source.rend = digest + ISC_MD5_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA1:
		isc_hmacsha1_init(&ctx.hsha, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha1_update(&ctx.hsha, data, length);
		isc_hmacsha1_sign(&ctx.hsha, digest,
				    ISC_SHA1_DIGESTLENGTH);
		source.rend = digest + ISC_SHA1_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA224:
		isc_hmacsha224_init(&ctx.h224, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha224_update(&ctx.h224, data, length);
		isc_hmacsha224_sign(&ctx.h224, digest,
				    ISC_SHA224_DIGESTLENGTH);
		source.rend = digest + ISC_SHA224_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA256:
		isc_hmacsha256_init(&ctx.h256, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha256_update(&ctx.h256, data, length);
		isc_hmacsha256_sign(&ctx.h256, digest,
				    ISC_SHA256_DIGESTLENGTH);
		source.rend = digest + ISC_SHA256_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA384:
		isc_hmacsha384_init(&ctx.h384, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha384_update(&ctx.h384, data, length);
		isc_hmacsha384_sign(&ctx.h384, digest,
				    ISC_SHA384_DIGESTLENGTH);
		source.rend = digest + ISC_SHA384_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA512:
		isc_hmacsha512_init(&ctx.h512, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha512_update(&ctx.h512, data, length);
		isc_hmacsha512_sign(&ctx.h512, digest,
				    ISC_SHA512_DIGESTLENGTH);
		source.rend = digest + ISC_SHA512_DIGESTLENGTH;
		break;

	default:
		return (ISC_R_FAILURE);
	}

	memset(digestb64, 0, sizeof(digestb64));
	target.rstart = digestb64;
	target.rend = digestb64 + sizeof(digestb64);
	result = isccc_base64_encode(&source, 64, "", &target);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (algorithm == ISCCC_ALG_HMACMD5)
		PUT_MEM(digestb64, HMD5_LENGTH, hmac);
	else
		PUT_MEM(digestb64, HSHA_LENGTH, hmac);
	return (ISC_R_SUCCESS);
}

isc_result_t
isccc_cc_towire(isccc_sexpr_t *alist, isccc_region_t *target,
		isc_uint32_t algorithm, isccc_region_t *secret)
{
	unsigned char *hmac_rstart, *signed_rstart;
	isc_result_t result;

	if (algorithm == ISCCC_ALG_HMACMD5) {
		if (REGION_SIZE(*target) < 4 + sizeof(auth_hmd5))
			return (ISC_R_NOSPACE);
	} else {
		if (REGION_SIZE(*target) < 4 + sizeof(auth_hsha))
			return (ISC_R_NOSPACE);
	}

	/*
	 * Emit protocol version.
	 */
	PUT32(1, target->rstart);
	if (secret != NULL) {
		/*
		 * Emit _auth section with zeroed HMAC signature.
		 * We'll replace the zeros with the real signature once
		 * we know what it is.
		 */
		if (algorithm == ISCCC_ALG_HMACMD5) {
			hmac_rstart = target->rstart + HMD5_OFFSET;
			PUT_MEM(auth_hmd5, sizeof(auth_hmd5), target->rstart);
		} else {
			unsigned char *hmac_alg;

			hmac_rstart = target->rstart + HSHA_OFFSET;
			hmac_alg = hmac_rstart - 1;
			PUT_MEM(auth_hsha, sizeof(auth_hsha), target->rstart);
			PUT8(algorithm, hmac_alg);
		}
	} else
		hmac_rstart = NULL;
	signed_rstart = target->rstart;
	/*
	 * Delete any existing _auth section so that we don't try
	 * to encode it.
	 */
	isccc_alist_delete(alist, "_auth");
	/*
	 * Emit the message.
	 */
	result = table_towire(alist, target);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (secret != NULL)
		return (sign(signed_rstart,
			     (unsigned int)(target->rstart - signed_rstart),
			     hmac_rstart, algorithm, secret));
	return (ISC_R_SUCCESS);
}

static isc_result_t
verify(isccc_sexpr_t *alist, unsigned char *data, unsigned int length,
       isc_uint32_t algorithm, isccc_region_t *secret)
{
	union {
		isc_hmacmd5_t hmd5;
		isc_hmacsha1_t hsha;
		isc_hmacsha224_t h224;
		isc_hmacsha256_t h256;
		isc_hmacsha384_t h384;
		isc_hmacsha512_t h512;
	} ctx;
	isccc_region_t source;
	isccc_region_t target;
	isc_result_t result;
	isccc_sexpr_t *_auth, *hmac;
	unsigned char digest[ISC_SHA512_DIGESTLENGTH];
	unsigned char digestb64[HSHA_LENGTH * 4];

	/*
	 * Extract digest.
	 */
	_auth = isccc_alist_lookup(alist, "_auth");
	if (_auth == NULL)
		return (ISC_R_FAILURE);
	if (algorithm == ISCCC_ALG_HMACMD5)
		hmac = isccc_alist_lookup(_auth, "hmd5");
	else
		hmac = isccc_alist_lookup(_auth, "hsha");
	if (hmac == NULL)
		return (ISC_R_FAILURE);
	/*
	 * Compute digest.
	 */
	source.rstart = digest;
	target.rstart = digestb64;
	switch (algorithm) {
	case ISCCC_ALG_HMACMD5:
		isc_hmacmd5_init(&ctx.hmd5, secret->rstart,
				 REGION_SIZE(*secret));
		isc_hmacmd5_update(&ctx.hmd5, data, length);
		isc_hmacmd5_sign(&ctx.hmd5, digest);
		source.rend = digest + ISC_MD5_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA1:
		isc_hmacsha1_init(&ctx.hsha, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha1_update(&ctx.hsha, data, length);
		isc_hmacsha1_sign(&ctx.hsha, digest,
				    ISC_SHA1_DIGESTLENGTH);
		source.rend = digest + ISC_SHA1_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA224:
		isc_hmacsha224_init(&ctx.h224, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha224_update(&ctx.h224, data, length);
		isc_hmacsha224_sign(&ctx.h224, digest,
				    ISC_SHA224_DIGESTLENGTH);
		source.rend = digest + ISC_SHA224_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA256:
		isc_hmacsha256_init(&ctx.h256, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha256_update(&ctx.h256, data, length);
		isc_hmacsha256_sign(&ctx.h256, digest,
				    ISC_SHA256_DIGESTLENGTH);
		source.rend = digest + ISC_SHA256_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA384:
		isc_hmacsha384_init(&ctx.h384, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha384_update(&ctx.h384, data, length);
		isc_hmacsha384_sign(&ctx.h384, digest,
				    ISC_SHA384_DIGESTLENGTH);
		source.rend = digest + ISC_SHA384_DIGESTLENGTH;
		break;

	case ISCCC_ALG_HMACSHA512:
		isc_hmacsha512_init(&ctx.h512, secret->rstart,
				    REGION_SIZE(*secret));
		isc_hmacsha512_update(&ctx.h512, data, length);
		isc_hmacsha512_sign(&ctx.h512, digest,
				    ISC_SHA512_DIGESTLENGTH);
		source.rend = digest + ISC_SHA512_DIGESTLENGTH;
		break;

	default:
		return (ISC_R_FAILURE);
	}
	target.rstart = digestb64;
	target.rend = digestb64 + sizeof(digestb64);
	memset(digestb64, 0, sizeof(digestb64));
	result = isccc_base64_encode(&source, 64, "", &target);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Verify.
	 */
	if (algorithm == ISCCC_ALG_HMACMD5) {
		unsigned char *value;

		value = (unsigned char *) isccc_sexpr_tostring(hmac);
		if (!isc_safe_memcmp(value, digestb64, HMD5_LENGTH))
			return (ISCCC_R_BADAUTH);
	} else {
		unsigned char *value;
		isc_uint32_t valalg;

		value = (unsigned char *) isccc_sexpr_tostring(hmac);
		GET8(valalg, value);
		if ((valalg != algorithm) ||
		    (!isc_safe_memcmp(value, digestb64, HSHA_LENGTH)))
			return (ISCCC_R_BADAUTH);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
table_fromwire(isccc_region_t *source, isccc_region_t *secret,
	       isc_uint32_t algorithm, isccc_sexpr_t **alistp);

static isc_result_t
list_fromwire(isccc_region_t *source, isccc_sexpr_t **listp);

static isc_result_t
value_fromwire(isccc_region_t *source, isccc_sexpr_t **valuep)
{
	unsigned int msgtype;
	isc_uint32_t len;
	isccc_sexpr_t *value;
	isccc_region_t active;
	isc_result_t result;

	if (REGION_SIZE(*source) < 1 + 4)
		return (ISC_R_UNEXPECTEDEND);
	GET8(msgtype, source->rstart);
	GET32(len, source->rstart);
	if (REGION_SIZE(*source) < len)
		return (ISC_R_UNEXPECTEDEND);
	active.rstart = source->rstart;
	active.rend = active.rstart + len;
	source->rstart = active.rend;
	if (msgtype == ISCCC_CCMSGTYPE_BINARYDATA) {
		value = isccc_sexpr_frombinary(&active);
		if (value != NULL) {
			*valuep = value;
			result = ISC_R_SUCCESS;
		} else
			result = ISC_R_NOMEMORY;
	} else if (msgtype == ISCCC_CCMSGTYPE_TABLE)
		result = table_fromwire(&active, NULL, 0, valuep);
	else if (msgtype == ISCCC_CCMSGTYPE_LIST)
		result = list_fromwire(&active, valuep);
	else
		result = ISCCC_R_SYNTAX;

	return (result);
}

static isc_result_t
table_fromwire(isccc_region_t *source, isccc_region_t *secret,
	       isc_uint32_t algorithm, isccc_sexpr_t **alistp)
{
	char key[256];
	isc_uint32_t len;
	isc_result_t result;
	isccc_sexpr_t *alist, *value;
	isc_boolean_t first_tag;
	unsigned char *checksum_rstart;

	REQUIRE(alistp != NULL && *alistp == NULL);

	checksum_rstart = NULL;
	first_tag = ISC_TRUE;
	alist = isccc_alist_create();
	if (alist == NULL)
		return (ISC_R_NOMEMORY);

	while (!REGION_EMPTY(*source)) {
		GET8(len, source->rstart);
		if (REGION_SIZE(*source) < len) {
			result = ISC_R_UNEXPECTEDEND;
			goto bad;
		}
		GET_MEM(key, len, source->rstart);
		key[len] = '\0';	/* Ensure NUL termination. */
		value = NULL;
		result = value_fromwire(source, &value);
		if (result != ISC_R_SUCCESS)
			goto bad;
		if (isccc_alist_define(alist, key, value) == NULL) {
			result = ISC_R_NOMEMORY;
			goto bad;
		}
		if (first_tag && secret != NULL && strcmp(key, "_auth") == 0)
			checksum_rstart = source->rstart;
		first_tag = ISC_FALSE;
	}

	if (secret != NULL) {
		if (checksum_rstart != NULL)
			result = verify(alist, checksum_rstart,
					(unsigned int)
					(source->rend - checksum_rstart),
					algorithm, secret);
		else
			result = ISCCC_R_BADAUTH;
	} else
		result = ISC_R_SUCCESS;

 bad:
	if (result == ISC_R_SUCCESS)
		*alistp = alist;
	else
		isccc_sexpr_free(&alist);

	return (result);
}

static isc_result_t
list_fromwire(isccc_region_t *source, isccc_sexpr_t **listp)
{
	isccc_sexpr_t *list, *value;
	isc_result_t result;

	list = NULL;
	while (!REGION_EMPTY(*source)) {
		value = NULL;
		result = value_fromwire(source, &value);
		if (result != ISC_R_SUCCESS) {
			isccc_sexpr_free(&list);
			return (result);
		}
		if (isccc_sexpr_addtolist(&list, value) == NULL) {
			isccc_sexpr_free(&value);
			isccc_sexpr_free(&list);
			return (result);
		}
	}

	*listp = list;

	return (ISC_R_SUCCESS);
}

isc_result_t
isccc_cc_fromwire(isccc_region_t *source, isccc_sexpr_t **alistp,
		  isc_uint32_t algorithm, isccc_region_t *secret)
{
	unsigned int size;
	isc_uint32_t version;

	size = REGION_SIZE(*source);
	if (size < 4)
		return (ISC_R_UNEXPECTEDEND);
	GET32(version, source->rstart);
	if (version != 1)
		return (ISCCC_R_UNKNOWNVERSION);

	return (table_fromwire(source, secret, algorithm, alistp));
}

static isc_result_t
createmessage(isc_uint32_t version, const char *from, const char *to,
	      isc_uint32_t serial, isccc_time_t now,
	      isccc_time_t expires, isccc_sexpr_t **alistp,
	      isc_boolean_t want_expires)
{
	isccc_sexpr_t *alist, *_ctrl, *_data;
	isc_result_t result;

	REQUIRE(alistp != NULL && *alistp == NULL);

	if (version != 1)
		return (ISCCC_R_UNKNOWNVERSION);

	alist = isccc_alist_create();
	if (alist == NULL)
		return (ISC_R_NOMEMORY);

	result = ISC_R_NOMEMORY;

	_ctrl = isccc_alist_create();
	if (_ctrl == NULL)
		goto bad;
	if (isccc_alist_define(alist, "_ctrl", _ctrl) == NULL) {
		isccc_sexpr_free(&_ctrl);
		goto bad;
	}

	_data = isccc_alist_create();
	if (_data == NULL)
		goto bad;
	if (isccc_alist_define(alist, "_data", _data) == NULL) {
		isccc_sexpr_free(&_data);
		goto bad;
	}

	if (isccc_cc_defineuint32(_ctrl, "_ser", serial) == NULL ||
	    isccc_cc_defineuint32(_ctrl, "_tim", now) == NULL ||
	    (want_expires &&
	     isccc_cc_defineuint32(_ctrl, "_exp", expires) == NULL))
		goto bad;
	if (from != NULL &&
	    isccc_cc_definestring(_ctrl, "_frm", from) == NULL)
		goto bad;
	if (to != NULL &&
	    isccc_cc_definestring(_ctrl, "_to", to) == NULL)
		goto bad;

	*alistp = alist;

	return (ISC_R_SUCCESS);

 bad:
	isccc_sexpr_free(&alist);

	return (result);
}

isc_result_t
isccc_cc_createmessage(isc_uint32_t version, const char *from, const char *to,
		       isc_uint32_t serial, isccc_time_t now,
		       isccc_time_t expires, isccc_sexpr_t **alistp)
{
	return (createmessage(version, from, to, serial, now, expires,
			      alistp, ISC_TRUE));
}

isc_result_t
isccc_cc_createack(isccc_sexpr_t *message, isc_boolean_t ok,
		   isccc_sexpr_t **ackp)
{
	char *_frm, *_to;
	isc_uint32_t serial;
	isccc_sexpr_t *ack, *_ctrl;
	isc_result_t result;
	isccc_time_t t;

	REQUIRE(ackp != NULL && *ackp == NULL);

	_ctrl = isccc_alist_lookup(message, "_ctrl");
	if (_ctrl == NULL ||
	    isccc_cc_lookupuint32(_ctrl, "_ser", &serial) != ISC_R_SUCCESS ||
	    isccc_cc_lookupuint32(_ctrl, "_tim", &t) != ISC_R_SUCCESS)
		return (ISC_R_FAILURE);
	/*
	 * _frm and _to are optional.
	 */
	_frm = NULL;
	(void)isccc_cc_lookupstring(_ctrl, "_frm", &_frm);
	_to = NULL;
	(void)isccc_cc_lookupstring(_ctrl, "_to", &_to);
	/*
	 * Create the ack.
	 */
	ack = NULL;
	result = createmessage(1, _to, _frm, serial, t, 0, &ack, ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		return (result);

	_ctrl = isccc_alist_lookup(ack, "_ctrl");
	if (_ctrl == NULL) {
		result = ISC_R_FAILURE;
		goto bad;
	}
	if (isccc_cc_definestring(ack, "_ack", (ok) ? "1" : "0") == NULL) {
		result = ISC_R_NOMEMORY;
		goto bad;
	}

	*ackp = ack;

	return (ISC_R_SUCCESS);

 bad:
	isccc_sexpr_free(&ack);

	return (result);
}

isc_boolean_t
isccc_cc_isack(isccc_sexpr_t *message)
{
	isccc_sexpr_t *_ctrl;

	_ctrl = isccc_alist_lookup(message, "_ctrl");
	if (_ctrl == NULL)
		return (ISC_FALSE);
	if (isccc_cc_lookupstring(_ctrl, "_ack", NULL) == ISC_R_SUCCESS)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

isc_boolean_t
isccc_cc_isreply(isccc_sexpr_t *message)
{
	isccc_sexpr_t *_ctrl;

	_ctrl = isccc_alist_lookup(message, "_ctrl");
	if (_ctrl == NULL)
		return (ISC_FALSE);
	if (isccc_cc_lookupstring(_ctrl, "_rpl", NULL) == ISC_R_SUCCESS)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

isc_result_t
isccc_cc_createresponse(isccc_sexpr_t *message, isccc_time_t now,
			isccc_time_t expires, isccc_sexpr_t **alistp)
{
	char *_frm, *_to, *type = NULL;
	isc_uint32_t serial;
	isccc_sexpr_t *alist, *_ctrl, *_data;
	isc_result_t result;

	REQUIRE(alistp != NULL && *alistp == NULL);

	_ctrl = isccc_alist_lookup(message, "_ctrl");
	_data = isccc_alist_lookup(message, "_data");
	if (_ctrl == NULL || _data == NULL ||
	    isccc_cc_lookupuint32(_ctrl, "_ser", &serial) != ISC_R_SUCCESS ||
	    isccc_cc_lookupstring(_data, "type", &type) != ISC_R_SUCCESS)
		return (ISC_R_FAILURE);
	/*
	 * _frm and _to are optional.
	 */
	_frm = NULL;
	(void)isccc_cc_lookupstring(_ctrl, "_frm", &_frm);
	_to = NULL;
	(void)isccc_cc_lookupstring(_ctrl, "_to", &_to);
	/*
	 * Create the response.
	 */
	alist = NULL;
	result = isccc_cc_createmessage(1, _to, _frm, serial, now, expires,
					 &alist);
	if (result != ISC_R_SUCCESS)
		return (result);

	_ctrl = isccc_alist_lookup(alist, "_ctrl");
	if (_ctrl == NULL) {
		result = ISC_R_FAILURE;
		goto bad;
	}

	_data = isccc_alist_lookup(alist, "_data");
	if (_data == NULL) {
		result = ISC_R_FAILURE;
		goto bad;
	}

	if (isccc_cc_definestring(_ctrl, "_rpl", "1") == NULL ||
	    isccc_cc_definestring(_data, "type", type) == NULL)
	{
		result = ISC_R_NOMEMORY;
		goto bad;
	}

	*alistp = alist;

	return (ISC_R_SUCCESS);

 bad:
	isccc_sexpr_free(&alist);
	return (result);
}

isccc_sexpr_t *
isccc_cc_definestring(isccc_sexpr_t *alist, const char *key, const char *str)
{
	size_t len;
	isccc_region_t r;

	len = strlen(str);
	DE_CONST(str, r.rstart);
	r.rend = r.rstart + len;

	return (isccc_alist_definebinary(alist, key, &r));
}

isccc_sexpr_t *
isccc_cc_defineuint32(isccc_sexpr_t *alist, const char *key, isc_uint32_t i)
{
	char b[100];
	size_t len;
	isccc_region_t r;

	snprintf(b, sizeof(b), "%u", i);
	len = strlen(b);
	r.rstart = (unsigned char *)b;
	r.rend = (unsigned char *)b + len;

	return (isccc_alist_definebinary(alist, key, &r));
}

isc_result_t
isccc_cc_lookupstring(isccc_sexpr_t *alist, const char *key, char **strp)
{
	isccc_sexpr_t *kv, *v;

	REQUIRE(strp == NULL || *strp == NULL);

	kv = isccc_alist_assq(alist, key);
	if (kv != NULL) {
		v = ISCCC_SEXPR_CDR(kv);
		if (isccc_sexpr_binaryp(v)) {
			if (strp != NULL)
				*strp = isccc_sexpr_tostring(v);
			return (ISC_R_SUCCESS);
		} else
			return (ISC_R_EXISTS);
	}

	return (ISC_R_NOTFOUND);
}

isc_result_t
isccc_cc_lookupuint32(isccc_sexpr_t *alist, const char *key,
		      isc_uint32_t *uintp)
{
	isccc_sexpr_t *kv, *v;

	kv = isccc_alist_assq(alist, key);
	if (kv != NULL) {
		v = ISCCC_SEXPR_CDR(kv);
		if (isccc_sexpr_binaryp(v)) {
			if (uintp != NULL)
				*uintp = (isc_uint32_t)
					strtoul(isccc_sexpr_tostring(v),
						NULL, 10);
			return (ISC_R_SUCCESS);
		} else
			return (ISC_R_EXISTS);
	}

	return (ISC_R_NOTFOUND);
}

static void
symtab_undefine(char *key, unsigned int type, isccc_symvalue_t value,
		void *arg)
{
	UNUSED(type);
	UNUSED(value);
	UNUSED(arg);

	free(key);
}

static isc_boolean_t
symtab_clean(char *key, unsigned int type, isccc_symvalue_t value,
	     void *arg)
{
	isccc_time_t *now;

	UNUSED(key);
	UNUSED(type);

	now = arg;

	if (*now < value.as_uinteger)
		return (ISC_FALSE);
	if ((*now - value.as_uinteger) < DUP_LIFETIME)
		return (ISC_FALSE);
	return (ISC_TRUE);
}

isc_result_t
isccc_cc_createsymtab(isccc_symtab_t **symtabp)
{
	return (isccc_symtab_create(11897, symtab_undefine, NULL, ISC_FALSE,
				  symtabp));
}

void
isccc_cc_cleansymtab(isccc_symtab_t *symtab, isccc_time_t now)
{
	isccc_symtab_foreach(symtab, symtab_clean, &now);
}

static isc_boolean_t
has_whitespace(const char *str)
{
	char c;

	if (str == NULL)
		return (ISC_FALSE);
	while ((c = *str++) != '\0') {
		if (c == ' ' || c == '\t' || c == '\n')
			return (ISC_TRUE);
	}
	return (ISC_FALSE);
}

isc_result_t
isccc_cc_checkdup(isccc_symtab_t *symtab, isccc_sexpr_t *message,
		  isccc_time_t now)
{
	const char *_frm;
	const char *_to;
	char *_ser = NULL, *_tim = NULL, *tmp;
	isc_result_t result;
	char *key;
	size_t len;
	isccc_symvalue_t value;
	isccc_sexpr_t *_ctrl;

	_ctrl = isccc_alist_lookup(message, "_ctrl");
	if (_ctrl == NULL ||
	    isccc_cc_lookupstring(_ctrl, "_ser", &_ser) != ISC_R_SUCCESS ||
	    isccc_cc_lookupstring(_ctrl, "_tim", &_tim) != ISC_R_SUCCESS)
		return (ISC_R_FAILURE);

	INSIST(_ser != NULL);
	INSIST(_tim != NULL);

	/*
	 * _frm and _to are optional.
	 */
	tmp = NULL;
	if (isccc_cc_lookupstring(_ctrl, "_frm", &tmp) != ISC_R_SUCCESS)
		_frm = "";
	else
		_frm = tmp;
	tmp = NULL;
	if (isccc_cc_lookupstring(_ctrl, "_to", &tmp) != ISC_R_SUCCESS)
		_to = "";
	else
		_to = tmp;
	/*
	 * Ensure there is no newline in any of the strings.  This is so
	 * we can write them to a file later.
	 */
	if (has_whitespace(_frm) || has_whitespace(_to) ||
	    has_whitespace(_ser) || has_whitespace(_tim))
		return (ISC_R_FAILURE);
	len = strlen(_frm) + strlen(_to) + strlen(_ser) + strlen(_tim) + 4;
	key = malloc(len);
	if (key == NULL)
		return (ISC_R_NOMEMORY);
	snprintf(key, len, "%s;%s;%s;%s", _frm, _to, _ser, _tim);
	value.as_uinteger = now;
	result = isccc_symtab_define(symtab, key, ISCCC_SYMTYPE_CCDUP, value,
				   isccc_symexists_reject);
	if (result != ISC_R_SUCCESS) {
		free(key);
		return (result);
	}

	return (ISC_R_SUCCESS);
}
