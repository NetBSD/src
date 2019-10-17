/*	$NetBSD: dst_api.c,v 1.4.4.1 2019/10/17 19:34:20 martin Exp $	*/

/*
 * Portions Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 *
 * Portions Copyright (C) Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/fsaccess.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/random.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>
#include <isc/file.h>

#include <pk11/site.h>

#define DST_KEY_INTERNAL

#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/ttl.h>
#include <dns/types.h>

#include <dst/result.h>

#include "dst_internal.h"

#define DST_AS_STR(t) ((t).value.as_textregion.base)

static dst_func_t *dst_t_func[DST_MAX_ALGS];

static bool dst_initialized = false;

void gss_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

/*
 * Static functions.
 */
static dst_key_t *	get_key_struct(const dns_name_t *name,
				       unsigned int alg,
				       unsigned int flags,
				       unsigned int protocol,
				       unsigned int bits,
				       dns_rdataclass_t rdclass,
				       dns_ttl_t ttl,
				       isc_mem_t *mctx);
static isc_result_t	write_public_key(const dst_key_t *key, int type,
					 const char *directory);
static isc_result_t	buildfilename(dns_name_t *name,
				      dns_keytag_t id,
				      unsigned int alg,
				      unsigned int type,
				      const char *directory,
				      isc_buffer_t *out);
static isc_result_t	computeid(dst_key_t *key);
static isc_result_t	frombuffer(const dns_name_t *name,
				   unsigned int alg,
				   unsigned int flags,
				   unsigned int protocol,
				   dns_rdataclass_t rdclass,
				   isc_buffer_t *source,
				   isc_mem_t *mctx,
				   dst_key_t **keyp);

static isc_result_t	algorithm_status(unsigned int alg);

static isc_result_t	addsuffix(char *filename, int len,
				  const char *dirname, const char *ofilename,
				  const char *suffix);

#define RETERR(x)				\
	do {					\
		result = (x);			\
		if (result != ISC_R_SUCCESS)	\
			goto out;		\
	} while (/*CONSTCOND*/0)

#define CHECKALG(alg)				\
	do {					\
		isc_result_t _r;		\
		_r = algorithm_status(alg);	\
		if (_r != ISC_R_SUCCESS)	\
			return (_r);		\
	} while (/*CONSTCOND*/0);				\

isc_result_t
dst_lib_init(isc_mem_t *mctx, const char *engine) {
	isc_result_t result;

	REQUIRE(mctx != NULL);
	REQUIRE(dst_initialized == false);

	UNUSED(engine);

	dst_result_register();

	memset(dst_t_func, 0, sizeof(dst_t_func));
	RETERR(dst__hmacmd5_init(&dst_t_func[DST_ALG_HMACMD5]));
	RETERR(dst__hmacsha1_init(&dst_t_func[DST_ALG_HMACSHA1]));
	RETERR(dst__hmacsha224_init(&dst_t_func[DST_ALG_HMACSHA224]));
	RETERR(dst__hmacsha256_init(&dst_t_func[DST_ALG_HMACSHA256]));
	RETERR(dst__hmacsha384_init(&dst_t_func[DST_ALG_HMACSHA384]));
	RETERR(dst__hmacsha512_init(&dst_t_func[DST_ALG_HMACSHA512]));
	RETERR(dst__openssl_init(mctx, engine));
	RETERR(dst__openssldh_init(&dst_t_func[DST_ALG_DH]));
#if USE_OPENSSL
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_RSASHA1],
				    DST_ALG_RSASHA1));
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_NSEC3RSASHA1],
				    DST_ALG_NSEC3RSASHA1));
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_RSASHA256],
				    DST_ALG_RSASHA256));
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_RSASHA512],
				    DST_ALG_RSASHA512));
	RETERR(dst__opensslecdsa_init(&dst_t_func[DST_ALG_ECDSA256]));
	RETERR(dst__opensslecdsa_init(&dst_t_func[DST_ALG_ECDSA384]));
#ifdef HAVE_OPENSSL_ED25519
	RETERR(dst__openssleddsa_init(&dst_t_func[DST_ALG_ED25519]));
#endif
#ifdef HAVE_OPENSSL_ED448
	RETERR(dst__openssleddsa_init(&dst_t_func[DST_ALG_ED448]));
#endif
#endif /* USE_OPENSSL */

#if USE_PKCS11
	RETERR(dst__pkcs11_init(mctx, engine));
	RETERR(dst__pkcs11rsa_init(&dst_t_func[DST_ALG_RSASHA1]));
	RETERR(dst__pkcs11rsa_init(&dst_t_func[DST_ALG_NSEC3RSASHA1]));
	RETERR(dst__pkcs11rsa_init(&dst_t_func[DST_ALG_RSASHA256]));
	RETERR(dst__pkcs11rsa_init(&dst_t_func[DST_ALG_RSASHA512]));
	RETERR(dst__pkcs11ecdsa_init(&dst_t_func[DST_ALG_ECDSA256]));
	RETERR(dst__pkcs11ecdsa_init(&dst_t_func[DST_ALG_ECDSA384]));
#ifdef HAVE_PKCS11_ED25519
	RETERR(dst__pkcs11eddsa_init(&dst_t_func[DST_ALG_ED25519]));
#endif
#ifdef HAVE_PKCS11_ED448
	RETERR(dst__pkcs11eddsa_init(&dst_t_func[DST_ALG_ED448]));
#endif
#endif /* USE_PKCS11 */
#ifdef GSSAPI
	RETERR(dst__gssapi_init(&dst_t_func[DST_ALG_GSSAPI]));
#endif

	dst_initialized = true;
	return (ISC_R_SUCCESS);

 out:
	/* avoid immediate crash! */
	dst_initialized = true;
	dst_lib_destroy();
	return (result);
}

void
dst_lib_destroy(void) {
	int i;
	RUNTIME_CHECK(dst_initialized == true);
	dst_initialized = false;

	for (i = 0; i < DST_MAX_ALGS; i++)
		if (dst_t_func[i] != NULL && dst_t_func[i]->cleanup != NULL)
			dst_t_func[i]->cleanup();
	dst__openssl_destroy();
#if USE_PKCS11
	(void) dst__pkcs11_destroy();
#endif /* USE_PKCS11 */
}

bool
dst_algorithm_supported(unsigned int alg) {
	REQUIRE(dst_initialized == true);

	if (alg >= DST_MAX_ALGS || dst_t_func[alg] == NULL)
		return (false);
	return (true);
}

bool
dst_ds_digest_supported(unsigned int digest_type) {
	return  (digest_type == DNS_DSDIGEST_SHA1 ||
		 digest_type == DNS_DSDIGEST_SHA256 ||
		 digest_type == DNS_DSDIGEST_SHA384);
}

isc_result_t
dst_context_create(dst_key_t *key, isc_mem_t *mctx,
		   isc_logcategory_t *category, bool useforsigning,
		   int maxbits, dst_context_t **dctxp)
{
	dst_context_t *dctx;
	isc_result_t result;

	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(mctx != NULL);
	REQUIRE(dctxp != NULL && *dctxp == NULL);

	if (key->func->createctx == NULL &&
	    key->func->createctx2 == NULL)
		return (DST_R_UNSUPPORTEDALG);
	if (key->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	dctx = isc_mem_get(mctx, sizeof(dst_context_t));
	if (dctx == NULL)
		return (ISC_R_NOMEMORY);
	memset(dctx, 0, sizeof(*dctx));
	dst_key_attach(key, &dctx->key);
	isc_mem_attach(mctx, &dctx->mctx);
	dctx->category = category;
	if (useforsigning)
		dctx->use = DO_SIGN;
	else
		dctx->use = DO_VERIFY;
	if (key->func->createctx2 != NULL)
		result = key->func->createctx2(key, maxbits, dctx);
	else
		result = key->func->createctx(key, dctx);
	if (result != ISC_R_SUCCESS) {
		if (dctx->key != NULL)
			dst_key_free(&dctx->key);
		isc_mem_putanddetach(&dctx->mctx, dctx, sizeof(dst_context_t));
		return (result);
	}
	dctx->magic = CTX_MAGIC;
	*dctxp = dctx;
	return (ISC_R_SUCCESS);
}

void
dst_context_destroy(dst_context_t **dctxp) {
	dst_context_t *dctx;

	REQUIRE(dctxp != NULL && VALID_CTX(*dctxp));

	dctx = *dctxp;
	INSIST(dctx->key->func->destroyctx != NULL);
	dctx->key->func->destroyctx(dctx);
	if (dctx->key != NULL)
		dst_key_free(&dctx->key);
	dctx->magic = 0;
	isc_mem_putanddetach(&dctx->mctx, dctx, sizeof(dst_context_t));
	*dctxp = NULL;
}

isc_result_t
dst_context_adddata(dst_context_t *dctx, const isc_region_t *data) {
	REQUIRE(VALID_CTX(dctx));
	REQUIRE(data != NULL);
	INSIST(dctx->key->func->adddata != NULL);

	return (dctx->key->func->adddata(dctx, data));
}

isc_result_t
dst_context_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_key_t *key;

	REQUIRE(VALID_CTX(dctx));
	REQUIRE(sig != NULL);

	key = dctx->key;
	CHECKALG(key->key_alg);
	if (key->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	if (key->func->sign == NULL)
		return (DST_R_NOTPRIVATEKEY);
	if (key->func->isprivate == NULL ||
	    key->func->isprivate(key) == false)
		return (DST_R_NOTPRIVATEKEY);

	return (key->func->sign(dctx, sig));
}

isc_result_t
dst_context_verify(dst_context_t *dctx, isc_region_t *sig) {
	REQUIRE(VALID_CTX(dctx));
	REQUIRE(sig != NULL);

	CHECKALG(dctx->key->key_alg);
	if (dctx->key->keydata.generic == NULL)
		return (DST_R_NULLKEY);
	if (dctx->key->func->verify == NULL)
		return (DST_R_NOTPUBLICKEY);

	return (dctx->key->func->verify(dctx, sig));
}

isc_result_t
dst_context_verify2(dst_context_t *dctx, unsigned int maxbits,
		    isc_region_t *sig)
{
	REQUIRE(VALID_CTX(dctx));
	REQUIRE(sig != NULL);

	CHECKALG(dctx->key->key_alg);
	if (dctx->key->keydata.generic == NULL)
		return (DST_R_NULLKEY);
	if (dctx->key->func->verify == NULL &&
	    dctx->key->func->verify2 == NULL)
		return (DST_R_NOTPUBLICKEY);

	return (dctx->key->func->verify2 != NULL ?
		dctx->key->func->verify2(dctx, maxbits, sig) :
		dctx->key->func->verify(dctx, sig));
}

isc_result_t
dst_key_computesecret(const dst_key_t *pub, const dst_key_t *priv,
		      isc_buffer_t *secret)
{
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(pub) && VALID_KEY(priv));
	REQUIRE(secret != NULL);

	CHECKALG(pub->key_alg);
	CHECKALG(priv->key_alg);

	if (pub->keydata.generic == NULL || priv->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	if (pub->key_alg != priv->key_alg ||
	    pub->func->computesecret == NULL ||
	    priv->func->computesecret == NULL)
		return (DST_R_KEYCANNOTCOMPUTESECRET);

	if (dst_key_isprivate(priv) == false)
		return (DST_R_NOTPRIVATEKEY);

	return (pub->func->computesecret(pub, priv, secret));
}

isc_result_t
dst_key_tofile(const dst_key_t *key, int type, const char *directory) {
	isc_result_t ret = ISC_R_SUCCESS;

	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);

	CHECKALG(key->key_alg);

	if (key->func->tofile == NULL)
		return (DST_R_UNSUPPORTEDALG);

	if ((type & DST_TYPE_PUBLIC) != 0) {
		ret = write_public_key(key, type, directory);
		if (ret != ISC_R_SUCCESS)
			return (ret);
	}

	if (((type & DST_TYPE_PRIVATE) != 0) &&
	    (key->key_flags & DNS_KEYFLAG_TYPEMASK) != DNS_KEYTYPE_NOKEY)
		return (key->func->tofile(key, directory));
	else
		return (ret);
}

void
dst_key_setexternal(dst_key_t *key, bool value) {
	key->external = value;
}

bool
dst_key_isexternal(dst_key_t *key) {
	return (key->external);
}

isc_result_t
dst_key_getfilename(dns_name_t *name, dns_keytag_t id,
		    unsigned int alg, int type, const char *directory,
		    isc_mem_t *mctx, isc_buffer_t *buf)
{
	isc_result_t result;

	REQUIRE(dst_initialized == true);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);
	REQUIRE(mctx != NULL);
	REQUIRE(buf != NULL);

	CHECKALG(alg);

	result = buildfilename(name, id, alg, type, directory, buf);
	if (result == ISC_R_SUCCESS) {
		if (isc_buffer_availablelength(buf) > 0)
			isc_buffer_putuint8(buf, 0);
		else
			result = ISC_R_NOSPACE;
	}

	return (result);
}

isc_result_t
dst_key_fromfile(dns_name_t *name, dns_keytag_t id,
		 unsigned int alg, int type, const char *directory,
		 isc_mem_t *mctx, dst_key_t **keyp)
{
	isc_result_t result;
	char filename[NAME_MAX];
	isc_buffer_t buf;
	dst_key_t *key;

	REQUIRE(dst_initialized == true);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	CHECKALG(alg);

	key = NULL;

	isc_buffer_init(&buf, filename, NAME_MAX);
	result = dst_key_getfilename(name, id, alg, type, NULL, mctx, &buf);
	if (result != ISC_R_SUCCESS)
		goto out;

	result = dst_key_fromnamedfile(filename, directory, type, mctx, &key);
	if (result != ISC_R_SUCCESS)
		goto out;

	result = computeid(key);
	if (result != ISC_R_SUCCESS)
		goto out;

	if (!dns_name_equal(name, key->key_name) || id != key->key_id ||
	    alg != key->key_alg) {
		result = DST_R_INVALIDPRIVATEKEY;
		goto out;
	}

	*keyp = key;
	result = ISC_R_SUCCESS;

 out:
	if ((key != NULL) && (result != ISC_R_SUCCESS))
		dst_key_free(&key);

	return (result);
}

isc_result_t
dst_key_fromnamedfile(const char *filename, const char *dirname,
		      int type, isc_mem_t *mctx, dst_key_t **keyp)
{
	isc_result_t result;
	dst_key_t *pubkey = NULL, *key = NULL;
	char *newfilename = NULL;
	int newfilenamelen = 0;
	isc_lex_t *lex = NULL;

	REQUIRE(dst_initialized == true);
	REQUIRE(filename != NULL);
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	/* If an absolute path is specified, don't use the key directory */
#ifndef WIN32
	if (filename[0] == '/')
		dirname = NULL;
#else /* WIN32 */
	if (filename[0] == '/' || filename[0] == '\\')
		dirname = NULL;
#endif

	newfilenamelen = strlen(filename) + 5;
	if (dirname != NULL)
		newfilenamelen += strlen(dirname) + 1;
	newfilename = isc_mem_get(mctx, newfilenamelen);
	if (newfilename == NULL)
		return (ISC_R_NOMEMORY);
	result = addsuffix(newfilename, newfilenamelen,
			   dirname, filename, ".key");
	INSIST(result == ISC_R_SUCCESS);

	result = dst_key_read_public(newfilename, type, mctx, &pubkey);
	isc_mem_put(mctx, newfilename, newfilenamelen);
	newfilename = NULL;
	RETERR(result);

	if ((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) == DST_TYPE_PUBLIC ||
	    (pubkey->key_flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY) {
		result = computeid(pubkey);
		if (result != ISC_R_SUCCESS) {
			dst_key_free(&pubkey);
			return (result);
		}

		*keyp = pubkey;
		return (ISC_R_SUCCESS);
	}

	result = algorithm_status(pubkey->key_alg);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&pubkey);
		return (result);
	}

	key = get_key_struct(pubkey->key_name, pubkey->key_alg,
			     pubkey->key_flags, pubkey->key_proto, 0,
			     pubkey->key_class, pubkey->key_ttl, mctx);
	if (key == NULL) {
		dst_key_free(&pubkey);
		return (ISC_R_NOMEMORY);
	}

	if (key->func->parse == NULL)
		RETERR(DST_R_UNSUPPORTEDALG);

	newfilenamelen = strlen(filename) + 9;
	if (dirname != NULL)
		newfilenamelen += strlen(dirname) + 1;
	newfilename = isc_mem_get(mctx, newfilenamelen);
	if (newfilename == NULL)
		RETERR(ISC_R_NOMEMORY);
	result = addsuffix(newfilename, newfilenamelen,
			   dirname, filename, ".private");
	INSIST(result == ISC_R_SUCCESS);

	RETERR(isc_lex_create(mctx, 1500, &lex));
	RETERR(isc_lex_openfile(lex, newfilename));
	isc_mem_put(mctx, newfilename, newfilenamelen);

	RETERR(key->func->parse(key, lex, pubkey));
	isc_lex_destroy(&lex);

	RETERR(computeid(key));

	if (pubkey->key_id != key->key_id)
		RETERR(DST_R_INVALIDPRIVATEKEY);
	dst_key_free(&pubkey);

	*keyp = key;
	return (ISC_R_SUCCESS);

 out:
	if (pubkey != NULL)
		dst_key_free(&pubkey);
	if (newfilename != NULL)
		isc_mem_put(mctx, newfilename, newfilenamelen);
	if (lex != NULL)
		isc_lex_destroy(&lex);
	if (key != NULL)
		dst_key_free(&key);
	return (result);
}

isc_result_t
dst_key_todns(const dst_key_t *key, isc_buffer_t *target) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(target != NULL);

	CHECKALG(key->key_alg);

	if (key->func->todns == NULL)
		return (DST_R_UNSUPPORTEDALG);

	if (isc_buffer_availablelength(target) < 4)
		return (ISC_R_NOSPACE);
	isc_buffer_putuint16(target, (uint16_t)(key->key_flags & 0xffff));
	isc_buffer_putuint8(target, (uint8_t)key->key_proto);
	isc_buffer_putuint8(target, (uint8_t)key->key_alg);

	if ((key->key_flags & DNS_KEYFLAG_EXTENDED) != 0) {
		if (isc_buffer_availablelength(target) < 2)
			return (ISC_R_NOSPACE);
		isc_buffer_putuint16(target,
				     (uint16_t)((key->key_flags >> 16)
						    & 0xffff));
	}

	if (key->keydata.generic == NULL) /*%< NULL KEY */
		return (ISC_R_SUCCESS);

	return (key->func->todns(key, target));
}

isc_result_t
dst_key_fromdns(const dns_name_t *name, dns_rdataclass_t rdclass,
		isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	uint8_t alg, proto;
	uint32_t flags, extflags;
	dst_key_t *key = NULL;
	dns_keytag_t id, rid;
	isc_region_t r;
	isc_result_t result;

	REQUIRE(dst_initialized);

	isc_buffer_remainingregion(source, &r);

	if (isc_buffer_remaininglength(source) < 4)
		return (DST_R_INVALIDPUBLICKEY);
	flags = isc_buffer_getuint16(source);
	proto = isc_buffer_getuint8(source);
	alg = isc_buffer_getuint8(source);

	id = dst_region_computeid(&r);
	rid = dst_region_computerid(&r);

	if ((flags & DNS_KEYFLAG_EXTENDED) != 0) {
		if (isc_buffer_remaininglength(source) < 2)
			return (DST_R_INVALIDPUBLICKEY);
		extflags = isc_buffer_getuint16(source);
		flags |= (extflags << 16);
	}

	result = frombuffer(name, alg, flags, proto, rdclass, source,
			    mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);
	key->key_id = id;
	key->key_rid = rid;

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_frombuffer(const dns_name_t *name, unsigned int alg,
		   unsigned int flags, unsigned int protocol,
		   dns_rdataclass_t rdclass,
		   isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key = NULL;
	isc_result_t result;

	REQUIRE(dst_initialized);

	result = frombuffer(name, alg, flags, protocol, rdclass, source,
			    mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_tobuffer(const dst_key_t *key, isc_buffer_t *target) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(target != NULL);

	CHECKALG(key->key_alg);

	if (key->func->todns == NULL)
		return (DST_R_UNSUPPORTEDALG);

	return (key->func->todns(key, target));
}

isc_result_t
dst_key_privatefrombuffer(dst_key_t *key, isc_buffer_t *buffer) {
	isc_lex_t *lex = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(!dst_key_isprivate(key));
	REQUIRE(buffer != NULL);

	if (key->func->parse == NULL)
		RETERR(DST_R_UNSUPPORTEDALG);

	RETERR(isc_lex_create(key->mctx, 1500, &lex));
	RETERR(isc_lex_openbuffer(lex, buffer));
	RETERR(key->func->parse(key, lex, NULL));
 out:
	if (lex != NULL)
		isc_lex_destroy(&lex);
	return (result);
}

gss_ctx_id_t
dst_key_getgssctx(const dst_key_t *key)
{
	REQUIRE(key != NULL);

	return (key->keydata.gssctx);
}

isc_result_t
dst_key_fromgssapi(const dns_name_t *name, gss_ctx_id_t gssctx,
		   isc_mem_t *mctx, dst_key_t **keyp, isc_region_t *intoken)
{
	dst_key_t *key;
	isc_result_t result;

	REQUIRE(gssctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = get_key_struct(name, DST_ALG_GSSAPI, 0, DNS_KEYPROTO_DNSSEC,
			     0, dns_rdataclass_in, 0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (intoken != NULL) {
		/*
		 * Keep the token for use by external ssu rules. They may need
		 * to examine the PAC in the kerberos ticket.
		 */
		RETERR(isc_buffer_allocate(key->mctx, &key->key_tkeytoken,
		       intoken->length));
		RETERR(isc_buffer_copyregion(key->key_tkeytoken, intoken));
	}

	key->keydata.gssctx = gssctx;
	*keyp = key;
	result = ISC_R_SUCCESS;
out:
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
	}
	return result;
}

isc_result_t
dst_key_buildinternal(const dns_name_t *name, unsigned int alg,
		      unsigned int bits, unsigned int flags,
		      unsigned int protocol, dns_rdataclass_t rdclass,
		      void *data, isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t result;

	REQUIRE(dst_initialized == true);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);
	REQUIRE(data != NULL);

	CHECKALG(alg);

	key = get_key_struct(name, alg, flags, protocol, bits, rdclass,
			     0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	key->keydata.generic = data;

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_fromlabel(const dns_name_t *name, int alg, unsigned int flags,
		  unsigned int protocol, dns_rdataclass_t rdclass,
		  const char *engine, const char *label, const char *pin,
		  isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t result;

	REQUIRE(dst_initialized == true);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);
	REQUIRE(label != NULL);

	CHECKALG(alg);

	key = get_key_struct(name, alg, flags, protocol, 0, rdclass, 0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (key->func->fromlabel == NULL) {
		dst_key_free(&key);
		return (DST_R_UNSUPPORTEDALG);
	}

	result = key->func->fromlabel(key, engine, label, pin);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_generate(const dns_name_t *name, unsigned int alg,
		 unsigned int bits, unsigned int param,
		 unsigned int flags, unsigned int protocol,
		 dns_rdataclass_t rdclass,
		 isc_mem_t *mctx, dst_key_t **keyp,
		 void (*callback)(int))
{
	dst_key_t *key;
	isc_result_t ret;

	REQUIRE(dst_initialized == true);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	CHECKALG(alg);

	key = get_key_struct(name, alg, flags, protocol, bits,
			     rdclass, 0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (bits == 0) { /*%< NULL KEY */
		key->key_flags |= DNS_KEYTYPE_NOKEY;
		*keyp = key;
		return (ISC_R_SUCCESS);
	}

	if (key->func->generate == NULL) {
		dst_key_free(&key);
		return (DST_R_UNSUPPORTEDALG);
	}

	ret = key->func->generate(key, param, callback);
	if (ret != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (ret);
	}

	ret = computeid(key);
	if (ret != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (ret);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_getnum(const dst_key_t *key, int type, uint32_t *valuep)
{
	REQUIRE(VALID_KEY(key));
	REQUIRE(valuep != NULL);
	REQUIRE(type <= DST_MAX_NUMERIC);
	if (!key->numset[type])
		return (ISC_R_NOTFOUND);
	*valuep = key->nums[type];
	return (ISC_R_SUCCESS);
}

void
dst_key_setnum(dst_key_t *key, int type, uint32_t value)
{
	REQUIRE(VALID_KEY(key));
	REQUIRE(type <= DST_MAX_NUMERIC);
	key->nums[type] = value;
	key->numset[type] = true;
}

void
dst_key_unsetnum(dst_key_t *key, int type)
{
	REQUIRE(VALID_KEY(key));
	REQUIRE(type <= DST_MAX_NUMERIC);
	key->numset[type] = false;
}

isc_result_t
dst_key_gettime(const dst_key_t *key, int type, isc_stdtime_t *timep) {
	REQUIRE(VALID_KEY(key));
	REQUIRE(timep != NULL);
	REQUIRE(type <= DST_MAX_TIMES);
	if (!key->timeset[type])
		return (ISC_R_NOTFOUND);
	*timep = key->times[type];
	return (ISC_R_SUCCESS);
}

void
dst_key_settime(dst_key_t *key, int type, isc_stdtime_t when) {
	REQUIRE(VALID_KEY(key));
	REQUIRE(type <= DST_MAX_TIMES);
	key->times[type] = when;
	key->timeset[type] = true;
}

void
dst_key_unsettime(dst_key_t *key, int type) {
	REQUIRE(VALID_KEY(key));
	REQUIRE(type <= DST_MAX_TIMES);
	key->timeset[type] = false;
}

isc_result_t
dst_key_getprivateformat(const dst_key_t *key, int *majorp, int *minorp) {
	REQUIRE(VALID_KEY(key));
	REQUIRE(majorp != NULL);
	REQUIRE(minorp != NULL);
	*majorp = key->fmt_major;
	*minorp = key->fmt_minor;
	return (ISC_R_SUCCESS);
}

void
dst_key_setprivateformat(dst_key_t *key, int major, int minor) {
	REQUIRE(VALID_KEY(key));
	key->fmt_major = major;
	key->fmt_minor = minor;
}

static bool
comparekeys(const dst_key_t *key1, const dst_key_t *key2,
	    bool match_revoked_key,
	    bool (*compare)(const dst_key_t *key1,
				     const dst_key_t *key2))
{
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key1));
	REQUIRE(VALID_KEY(key2));

	if (key1 == key2)
		return (true);

	if (key1->key_alg != key2->key_alg)
		return (false);

	if (key1->key_id != key2->key_id) {
		if (!match_revoked_key)
			return (false);
		if ((key1->key_flags & DNS_KEYFLAG_REVOKE) ==
		    (key2->key_flags & DNS_KEYFLAG_REVOKE))
			return (false);
		if (key1->key_id != key2->key_rid &&
		    key1->key_rid != key2->key_id)
			return (false);
	}

	if (compare != NULL)
		return (compare(key1, key2));
	else
		return (false);
}


/*
 * Compares only the public portion of two keys, by converting them
 * both to wire format and comparing the results.
 */
static bool
pub_compare(const dst_key_t *key1, const dst_key_t *key2) {
	isc_result_t result;
	unsigned char buf1[DST_KEY_MAXSIZE], buf2[DST_KEY_MAXSIZE];
	isc_buffer_t b1, b2;
	isc_region_t r1, r2;

	isc_buffer_init(&b1, buf1, sizeof(buf1));
	result = dst_key_todns(key1, &b1);
	if (result != ISC_R_SUCCESS)
		return (false);
	/* Zero out flags. */
	buf1[0] = buf1[1] = 0;
	if ((key1->key_flags & DNS_KEYFLAG_EXTENDED) != 0)
		isc_buffer_subtract(&b1, 2);

	isc_buffer_init(&b2, buf2, sizeof(buf2));
	result = dst_key_todns(key2, &b2);
	if (result != ISC_R_SUCCESS)
		return (false);
	/* Zero out flags. */
	buf2[0] = buf2[1] = 0;
	if ((key2->key_flags & DNS_KEYFLAG_EXTENDED) != 0)
		isc_buffer_subtract(&b2, 2);

	isc_buffer_usedregion(&b1, &r1);
	/* Remove extended flags. */
	if ((key1->key_flags & DNS_KEYFLAG_EXTENDED) != 0) {
		memmove(&buf1[4], &buf1[6], r1.length - 6);
		r1.length -= 2;
	}

	isc_buffer_usedregion(&b2, &r2);
	/* Remove extended flags. */
	if ((key2->key_flags & DNS_KEYFLAG_EXTENDED) != 0) {
		memmove(&buf2[4], &buf2[6], r2.length - 6);
		r2.length -= 2;
	}
	return (isc_region_compare(&r1, &r2) == 0);
}

bool
dst_key_compare(const dst_key_t *key1, const dst_key_t *key2) {
	return (comparekeys(key1, key2, false, key1->func->compare));
}

bool
dst_key_pubcompare(const dst_key_t *key1, const dst_key_t *key2,
		   bool match_revoked_key)
{
	return (comparekeys(key1, key2, match_revoked_key, pub_compare));
}


bool
dst_key_paramcompare(const dst_key_t *key1, const dst_key_t *key2) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key1));
	REQUIRE(VALID_KEY(key2));

	if (key1 == key2)
		return (true);
	if (key1->key_alg == key2->key_alg &&
	    key1->func->paramcompare != NULL &&
	    key1->func->paramcompare(key1, key2) == true)
		return (true);
	else
		return (false);
}

void
dst_key_attach(dst_key_t *source, dst_key_t **target) {

	REQUIRE(dst_initialized == true);
	REQUIRE(target != NULL && *target == NULL);
	REQUIRE(VALID_KEY(source));

	isc_refcount_increment(&source->refs);
	*target = source;
}

void
dst_key_free(dst_key_t **keyp) {
	REQUIRE(dst_initialized == true);
	REQUIRE(keyp != NULL && VALID_KEY(*keyp));
	dst_key_t *key = *keyp;
	*keyp = NULL;

	if (isc_refcount_decrement(&key->refs) == 1) {
		isc_refcount_destroy(&key->refs);
		isc_mem_t *mctx = key->mctx;
		if (key->keydata.generic != NULL) {
			INSIST(key->func->destroy != NULL);
			key->func->destroy(key);
		}
		if (key->engine != NULL)
			isc_mem_free(mctx, key->engine);
		if (key->label != NULL)
			isc_mem_free(mctx, key->label);
		dns_name_free(key->key_name, mctx);
		isc_mem_put(mctx, key->key_name, sizeof(dns_name_t));
		if (key->key_tkeytoken) {
			isc_buffer_free(&key->key_tkeytoken);
		}
		isc_safe_memwipe(key, sizeof(*key));
		isc_mem_putanddetach(&mctx, key, sizeof(*key));
	}
}

bool
dst_key_isprivate(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	INSIST(key->func->isprivate != NULL);
	return (key->func->isprivate(key));
}

isc_result_t
dst_key_buildfilename(const dst_key_t *key, int type,
		      const char *directory, isc_buffer_t *out) {

	REQUIRE(VALID_KEY(key));
	REQUIRE(type == DST_TYPE_PRIVATE || type == DST_TYPE_PUBLIC ||
		type == 0);

	return (buildfilename(key->key_name, key->key_id, key->key_alg,
			      type, directory, out));
}

isc_result_t
dst_key_sigsize(const dst_key_t *key, unsigned int *n) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(n != NULL);

	/* XXXVIX this switch statement is too sparse to gen a jump table. */
	switch (key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
	case DST_ALG_RSASHA256:
	case DST_ALG_RSASHA512:
		*n = (key->key_size + 7) / 8;
		break;
	case DST_ALG_ECDSA256:
		*n = DNS_SIG_ECDSA256SIZE;
		break;
	case DST_ALG_ECDSA384:
		*n = DNS_SIG_ECDSA384SIZE;
		break;
	case DST_ALG_ED25519:
		*n = DNS_SIG_ED25519SIZE;
		break;
	case DST_ALG_ED448:
		*n = DNS_SIG_ED448SIZE;
		break;
	case DST_ALG_HMACMD5:
		*n = isc_md_type_get_size(ISC_MD_MD5);
		break;
	case DST_ALG_HMACSHA1:
		*n = isc_md_type_get_size(ISC_MD_SHA1);
		break;
	case DST_ALG_HMACSHA224:
		*n = isc_md_type_get_size(ISC_MD_SHA224);
		break;
	case DST_ALG_HMACSHA256:
		*n = isc_md_type_get_size(ISC_MD_SHA256);
		break;
	case DST_ALG_HMACSHA384:
		*n = isc_md_type_get_size(ISC_MD_SHA384);
		break;
	case DST_ALG_HMACSHA512:
		*n = isc_md_type_get_size(ISC_MD_SHA512);
		break;
	case DST_ALG_GSSAPI:
		*n = 128; /*%< XXX */
		break;
	case DST_ALG_DH:
	default:
		return (DST_R_UNSUPPORTEDALG);
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_secretsize(const dst_key_t *key, unsigned int *n) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));
	REQUIRE(n != NULL);

	if (key->key_alg == DST_ALG_DH) {
		*n = (key->key_size + 7) / 8;
		return (ISC_R_SUCCESS);
	}
	return (DST_R_UNSUPPORTEDALG);
}

/*%
 * Set the flags on a key, then recompute the key ID
 */
isc_result_t
dst_key_setflags(dst_key_t *key, uint32_t flags) {
	REQUIRE(VALID_KEY(key));
	key->key_flags = flags;
	return (computeid(key));
}

void
dst_key_format(const dst_key_t *key, char *cp, unsigned int size) {
	char namestr[DNS_NAME_FORMATSIZE];
	char algstr[DNS_NAME_FORMATSIZE];

	dns_name_format(dst_key_name(key), namestr, sizeof(namestr));
	dns_secalg_format((dns_secalg_t) dst_key_alg(key), algstr,
			  sizeof(algstr));
	snprintf(cp, size, "%s/%s/%d", namestr, algstr, dst_key_id(key));
}

isc_result_t
dst_key_dump(dst_key_t *key, isc_mem_t *mctx, char **buffer, int *length) {

	REQUIRE(buffer != NULL && *buffer == NULL);
	REQUIRE(length != NULL && *length == 0);
	REQUIRE(VALID_KEY(key));

	if (key->func->dump == NULL)
		return (ISC_R_NOTIMPLEMENTED);
	return (key->func->dump(key, mctx, buffer, length));
}

isc_result_t
dst_key_restore(dns_name_t *name, unsigned int alg, unsigned int flags,
		unsigned int protocol, dns_rdataclass_t rdclass,
		isc_mem_t *mctx, const char *keystr, dst_key_t **keyp)
{
	isc_result_t result;
	dst_key_t *key;

	REQUIRE(dst_initialized == true);
	REQUIRE(keyp != NULL && *keyp == NULL);

	if (alg >= DST_MAX_ALGS || dst_t_func[alg] == NULL)
		return (DST_R_UNSUPPORTEDALG);

	if (dst_t_func[alg]->restore == NULL)
		return (ISC_R_NOTIMPLEMENTED);

	key = get_key_struct(name, alg, flags, protocol, 0, rdclass, 0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	result = (dst_t_func[alg]->restore)(key, keystr);
	if (result == ISC_R_SUCCESS)
		*keyp = key;
	else
		dst_key_free(&key);

	return (result);
}

/***
 *** Static methods
 ***/

/*%
 * Allocates a key structure and fills in some of the fields.
 */
static dst_key_t *
get_key_struct(const dns_name_t *name, unsigned int alg,
	       unsigned int flags, unsigned int protocol,
	       unsigned int bits, dns_rdataclass_t rdclass,
	       dns_ttl_t ttl, isc_mem_t *mctx)
{
	dst_key_t *key;
	isc_result_t result;
	int i;

	key = (dst_key_t *) isc_mem_get(mctx, sizeof(dst_key_t));
	if (key == NULL)
		return (NULL);

	memset(key, 0, sizeof(dst_key_t));

	key->key_name = isc_mem_get(mctx, sizeof(dns_name_t));
	if (key->key_name == NULL) {
		isc_mem_put(mctx, key, sizeof(dst_key_t));
		return (NULL);
	}

	dns_name_init(key->key_name, NULL);
	result = dns_name_dup(name, mctx, key->key_name);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, key->key_name, sizeof(dns_name_t));
		isc_mem_put(mctx, key, sizeof(dst_key_t));
		return (NULL);
	}

	isc_refcount_init(&key->refs, 1);
	isc_mem_attach(mctx, &key->mctx);
	key->key_alg = alg;
	key->key_flags = flags;
	key->key_proto = protocol;
	key->keydata.generic = NULL;
	key->key_size = bits;
	key->key_class = rdclass;
	key->key_ttl = ttl;
	key->func = dst_t_func[alg];
	key->fmt_major = 0;
	key->fmt_minor = 0;
	for (i = 0; i < (DST_MAX_TIMES + 1); i++) {
		key->times[i] = 0;
		key->timeset[i] = false;
	}
	key->inactive = false;
	key->magic = KEY_MAGIC;
	return (key);
}

bool
dst_key_inactive(const dst_key_t *key) {

	REQUIRE(VALID_KEY(key));

	return (key->inactive);
}

void
dst_key_setinactive(dst_key_t *key, bool inactive) {

	REQUIRE(VALID_KEY(key));

	key->inactive = inactive;
}

/*%
 * Reads a public key from disk
 */
isc_result_t
dst_key_read_public(const char *filename, int type,
		    isc_mem_t *mctx, dst_key_t **keyp)
{
	u_char rdatabuf[DST_KEY_MAXSIZE];
	isc_buffer_t b;
	dns_fixedname_t name;
	isc_lex_t *lex = NULL;
	isc_token_t token;
	isc_result_t ret;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int opt = ISC_LEXOPT_DNSMULTILINE;
	dns_rdataclass_t rdclass = dns_rdataclass_in;
	isc_lexspecials_t specials;
	uint32_t ttl = 0;
	isc_result_t result;
	dns_rdatatype_t keytype;

	/*
	 * Open the file and read its formatted contents
	 * File format:
	 *    domain.name [ttl] [class] [KEY|DNSKEY] <flags> <protocol> <algorithm> <key>
	 */

	/* 1500 should be large enough for any key */
	ret = isc_lex_create(mctx, 1500, &lex);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	memset(specials, 0, sizeof(specials));
	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);
	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);

	ret = isc_lex_openfile(lex, filename);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

#define NEXTTOKEN(lex, opt, token) { \
	ret = isc_lex_gettoken(lex, opt, token); \
	if (ret != ISC_R_SUCCESS) \
		goto cleanup; \
	}

#define BADTOKEN() { \
	ret = ISC_R_UNEXPECTEDTOKEN; \
	goto cleanup; \
	}

	/* Read the domain name */
	NEXTTOKEN(lex, opt, &token);
	if (token.type != isc_tokentype_string)
		BADTOKEN();

	/*
	 * We don't support "@" in .key files.
	 */
	if (!strcmp(DST_AS_STR(token), "@"))
		BADTOKEN();

	dns_fixedname_init(&name);
	isc_buffer_init(&b, DST_AS_STR(token), strlen(DST_AS_STR(token)));
	isc_buffer_add(&b, strlen(DST_AS_STR(token)));
	ret = dns_name_fromtext(dns_fixedname_name(&name), &b, dns_rootname,
				0, NULL);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	/* Read the next word: either TTL, class, or 'KEY' */
	NEXTTOKEN(lex, opt, &token);

	if (token.type != isc_tokentype_string)
		BADTOKEN();

	/* If it's a TTL, read the next one */
	result = dns_ttl_fromtext(&token.value.as_textregion, &ttl);
	if (result == ISC_R_SUCCESS)
		NEXTTOKEN(lex, opt, &token);

	if (token.type != isc_tokentype_string)
		BADTOKEN();

	ret = dns_rdataclass_fromtext(&rdclass, &token.value.as_textregion);
	if (ret == ISC_R_SUCCESS)
		NEXTTOKEN(lex, opt, &token);

	if (token.type != isc_tokentype_string)
		BADTOKEN();

	if (strcasecmp(DST_AS_STR(token), "DNSKEY") == 0)
		keytype = dns_rdatatype_dnskey;
	else if (strcasecmp(DST_AS_STR(token), "KEY") == 0)
		keytype = dns_rdatatype_key; /*%< SIG(0), TKEY */
	else
		BADTOKEN();

	if (((type & DST_TYPE_KEY) != 0 && keytype != dns_rdatatype_key) ||
	    ((type & DST_TYPE_KEY) == 0 && keytype != dns_rdatatype_dnskey)) {
		ret = DST_R_BADKEYTYPE;
		goto cleanup;
	}

	isc_buffer_init(&b, rdatabuf, sizeof(rdatabuf));
	ret = dns_rdata_fromtext(&rdata, rdclass, keytype, lex, NULL,
				 false, mctx, &b, NULL);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	ret = dst_key_fromdns(dns_fixedname_name(&name), rdclass, &b, mctx,
			      keyp);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	dst_key_setttl(*keyp, ttl);

 cleanup:
	if (lex != NULL)
		isc_lex_destroy(&lex);
	return (ret);
}

static bool
issymmetric(const dst_key_t *key) {
	REQUIRE(dst_initialized == true);
	REQUIRE(VALID_KEY(key));

	/* XXXVIX this switch statement is too sparse to gen a jump table. */
	switch (key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
	case DST_ALG_RSASHA256:
	case DST_ALG_RSASHA512:
	case DST_ALG_DH:
	case DST_ALG_ECDSA256:
	case DST_ALG_ECDSA384:
	case DST_ALG_ED25519:
	case DST_ALG_ED448:
		return (false);
	case DST_ALG_HMACMD5:
	case DST_ALG_HMACSHA1:
	case DST_ALG_HMACSHA224:
	case DST_ALG_HMACSHA256:
	case DST_ALG_HMACSHA384:
	case DST_ALG_HMACSHA512:
	case DST_ALG_GSSAPI:
		return (true);
	default:
		return (false);
	}
}

/*%
 * Write key timing metadata to a file pointer, preceded by 'tag'
 */
static void
printtime(const dst_key_t *key, int type, const char *tag, FILE *stream) {
	isc_result_t result;
	char output[26]; /* Minimum buffer as per ctime_r() specification. */
	isc_stdtime_t when;
	time_t t;
	char utc[sizeof("YYYYMMDDHHSSMM")];
	isc_buffer_t b;
	isc_region_t r;

	result = dst_key_gettime(key, type, &when);
	if (result == ISC_R_NOTFOUND)
		return;

	/* time_t and isc_stdtime_t might be different sizes */
	t = when;
#ifdef WIN32
	if (ctime_s(output, sizeof(output), &t) != 0)
		goto error;
#else
	if (ctime_r(&t, output) == NULL)
		goto error;
#endif

	isc_buffer_init(&b, utc, sizeof(utc));
	result = dns_time32_totext(when, &b);
	if (result != ISC_R_SUCCESS)
		goto error;

	isc_buffer_usedregion(&b, &r);
	fprintf(stream, "%s: %.*s (%.*s)\n", tag, (int)r.length, r.base,
		 (int)strlen(output) - 1, output);
	return;

 error:
	fprintf(stream, "%s: (set, unable to display)\n", tag);
}

/*%
 * Writes a public key to disk in DNS format.
 */
static isc_result_t
write_public_key(const dst_key_t *key, int type, const char *directory) {
	FILE *fp;
	isc_buffer_t keyb, textb, fileb, classb;
	isc_region_t r;
	char filename[NAME_MAX];
	unsigned char key_array[DST_KEY_MAXSIZE];
	char text_array[DST_KEY_MAXTEXTSIZE];
	char class_array[10];
	isc_result_t ret;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_fsaccess_t access;

	REQUIRE(VALID_KEY(key));

	isc_buffer_init(&keyb, key_array, sizeof(key_array));
	isc_buffer_init(&textb, text_array, sizeof(text_array));
	isc_buffer_init(&classb, class_array, sizeof(class_array));

	ret = dst_key_todns(key, &keyb);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	isc_buffer_usedregion(&keyb, &r);
	dns_rdata_fromregion(&rdata, key->key_class, dns_rdatatype_dnskey, &r);

	ret = dns_rdata_totext(&rdata, (dns_name_t *) NULL, &textb);
	if (ret != ISC_R_SUCCESS)
		return (DST_R_INVALIDPUBLICKEY);

	ret = dns_rdataclass_totext(key->key_class, &classb);
	if (ret != ISC_R_SUCCESS)
		return (DST_R_INVALIDPUBLICKEY);

	/*
	 * Make the filename.
	 */
	isc_buffer_init(&fileb, filename, sizeof(filename));
	ret = dst_key_buildfilename(key, DST_TYPE_PUBLIC, directory, &fileb);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	/*
	 * Create public key file.
	 */
	if ((fp = fopen(filename, "w")) == NULL)
		return (DST_R_WRITEERROR);

	if (issymmetric(key)) {
		access = 0;
		isc_fsaccess_add(ISC_FSACCESS_OWNER,
				 ISC_FSACCESS_READ | ISC_FSACCESS_WRITE,
				 &access);
		(void)isc_fsaccess_set(filename, access);
	}

	/* Write key information in comments */
	if ((type & DST_TYPE_KEY) == 0) {
		fprintf(fp, "; This is a %s%s-signing key, keyid %d, for ",
			(key->key_flags & DNS_KEYFLAG_REVOKE) != 0 ?
				"revoked " :
				"",
			(key->key_flags & DNS_KEYFLAG_KSK) != 0 ?
				"key" :
				"zone",
			key->key_id);
		ret = dns_name_print(key->key_name, fp);
		if (ret != ISC_R_SUCCESS) {
			fclose(fp);
			return (ret);
		}
		fputc('\n', fp);

		printtime(key, DST_TIME_CREATED, "; Created", fp);
		printtime(key, DST_TIME_PUBLISH, "; Publish", fp);
		printtime(key, DST_TIME_ACTIVATE, "; Activate", fp);
		printtime(key, DST_TIME_REVOKE, "; Revoke", fp);
		printtime(key, DST_TIME_INACTIVE, "; Inactive", fp);
		printtime(key, DST_TIME_DELETE, "; Delete", fp);
		printtime(key, DST_TIME_SYNCPUBLISH , "; SyncPublish", fp);
		printtime(key, DST_TIME_SYNCDELETE , "; SyncDelete", fp);
	}

	/* Now print the actual key */
	ret = dns_name_print(key->key_name, fp);
	fprintf(fp, " ");

	if (key->key_ttl != 0)
		fprintf(fp, "%u ", key->key_ttl);

	isc_buffer_usedregion(&classb, &r);
	if ((unsigned) fwrite(r.base, 1, r.length, fp) != r.length)
	       ret = DST_R_WRITEERROR;

	if ((type & DST_TYPE_KEY) != 0)
		fprintf(fp, " KEY ");
	else
		fprintf(fp, " DNSKEY ");

	isc_buffer_usedregion(&textb, &r);
	if ((unsigned) fwrite(r.base, 1, r.length, fp) != r.length)
	       ret = DST_R_WRITEERROR;

	fputc('\n', fp);
	fflush(fp);
	if (ferror(fp))
		ret = DST_R_WRITEERROR;
	fclose(fp);

	return (ret);
}

static isc_result_t
buildfilename(dns_name_t *name, dns_keytag_t id,
	      unsigned int alg, unsigned int type,
	      const char *directory, isc_buffer_t *out)
{
	const char *suffix = "";
	isc_result_t result;

	REQUIRE(out != NULL);
	if ((type & DST_TYPE_PRIVATE) != 0)
		suffix = ".private";
	else if (type == DST_TYPE_PUBLIC)
		suffix = ".key";
	if (directory != NULL) {
		if (isc_buffer_availablelength(out) < strlen(directory))
			return (ISC_R_NOSPACE);
		isc_buffer_putstr(out, directory);
		if (strlen(directory) > 0U &&
		    directory[strlen(directory) - 1] != '/')
			isc_buffer_putstr(out, "/");
	}
	if (isc_buffer_availablelength(out) < 1)
		return (ISC_R_NOSPACE);
	isc_buffer_putstr(out, "K");
	result = dns_name_tofilenametext(name, false, out);
	if (result != ISC_R_SUCCESS)
		return (result);

	return (isc_buffer_printf(out, "+%03d+%05d%s", alg, id, suffix));
}

static isc_result_t
computeid(dst_key_t *key) {
	isc_buffer_t dnsbuf;
	unsigned char dns_array[DST_KEY_MAXSIZE];
	isc_region_t r;
	isc_result_t ret;

	isc_buffer_init(&dnsbuf, dns_array, sizeof(dns_array));
	ret = dst_key_todns(key, &dnsbuf);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	isc_buffer_usedregion(&dnsbuf, &r);
	key->key_id = dst_region_computeid(&r);
	key->key_rid = dst_region_computerid(&r);
	return (ISC_R_SUCCESS);
}

static isc_result_t
frombuffer(const dns_name_t *name, unsigned int alg, unsigned int flags,
	   unsigned int protocol, dns_rdataclass_t rdclass,
	   isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t ret;

	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(source != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = get_key_struct(name, alg, flags, protocol, 0, rdclass, 0, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (isc_buffer_remaininglength(source) > 0) {
		ret = algorithm_status(alg);
		if (ret != ISC_R_SUCCESS) {
			dst_key_free(&key);
			return (ret);
		}
		if (key->func->fromdns == NULL) {
			dst_key_free(&key);
			return (DST_R_UNSUPPORTEDALG);
		}

		ret = key->func->fromdns(key, source);
		if (ret != ISC_R_SUCCESS) {
			dst_key_free(&key);
			return (ret);
		}
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

static isc_result_t
algorithm_status(unsigned int alg) {
	REQUIRE(dst_initialized == true);

	if (dst_algorithm_supported(alg)) {
		return (ISC_R_SUCCESS);
	}
	return (DST_R_UNSUPPORTEDALG);
}

static isc_result_t
addsuffix(char *filename, int len, const char *odirname,
	  const char *ofilename, const char *suffix)
{
	int olen = strlen(ofilename);
	int n;

	if (olen > 1 && ofilename[olen - 1] == '.')
		olen -= 1;
	else if (olen > 8 && strcmp(ofilename + olen - 8, ".private") == 0)
		olen -= 8;
	else if (olen > 4 && strcmp(ofilename + olen - 4, ".key") == 0)
		olen -= 4;

	if (odirname == NULL)
		n = snprintf(filename, len, "%.*s%s", olen, ofilename, suffix);
	else
		n = snprintf(filename, len, "%s/%.*s%s",
			     odirname, olen, ofilename, suffix);
	if (n < 0)
		return (ISC_R_FAILURE);
	if (n >= len)
		return (ISC_R_NOSPACE);
	return (ISC_R_SUCCESS);
}

isc_buffer_t *
dst_key_tkeytoken(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_tkeytoken);
}
