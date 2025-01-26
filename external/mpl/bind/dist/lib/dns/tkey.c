/*	$NetBSD: tkey.c,v 1.15 2025/01/26 16:25:25 christos Exp $	*/

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

#if HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#elif HAVE_GSSAPI_H
#include <gssapi.h>
#endif

#include <isc/buffer.h>
#include <isc/hex.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/nonce.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/tkey.h>
#include <dns/tsig.h>

#include <dst/dst.h>
#include <dst/gssapi.h>

#include "dst_internal.h"
#include "tsig_p.h"

#define TEMP_BUFFER_SZ	   8192
#define TKEY_RANDOM_AMOUNT 16

#define RETERR(x)                            \
	do {                                 \
		result = (x);                \
		if (result != ISC_R_SUCCESS) \
			goto failure;        \
	} while (0)

static void
tkey_log(const char *fmt, ...) ISC_FORMAT_PRINTF(1, 2);

static void
tkey_log(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_REQUEST,
		       ISC_LOG_DEBUG(4), fmt, ap);
	va_end(ap);
}

isc_result_t
dns_tkeyctx_create(isc_mem_t *mctx, dns_tkeyctx_t **tctxp) {
	REQUIRE(mctx != NULL);
	REQUIRE(tctxp != NULL && *tctxp == NULL);

	dns_tkeyctx_t *tctx = isc_mem_get(mctx, sizeof(*tctx));
	*tctx = (dns_tkeyctx_t){
		.mctx = NULL,
	};
	isc_mem_attach(mctx, &tctx->mctx);

	*tctxp = tctx;
	return ISC_R_SUCCESS;
}

void
dns_tkeyctx_destroy(dns_tkeyctx_t **tctxp) {
	isc_mem_t *mctx = NULL;
	dns_tkeyctx_t *tctx = NULL;

	REQUIRE(tctxp != NULL && *tctxp != NULL);

	tctx = *tctxp;
	*tctxp = NULL;
	mctx = tctx->mctx;

	if (tctx->domain != NULL) {
		if (dns_name_dynamic(tctx->domain)) {
			dns_name_free(tctx->domain, mctx);
		}
		isc_mem_put(mctx, tctx->domain, sizeof(dns_name_t));
	}
	if (tctx->gssapi_keytab != NULL) {
		isc_mem_free(mctx, tctx->gssapi_keytab);
	}
	if (tctx->gsscred != NULL) {
		dst_gssapi_releasecred(&tctx->gsscred);
	}
	isc_mem_putanddetach(&mctx, tctx, sizeof(dns_tkeyctx_t));
}

static void
add_rdata_to_list(dns_message_t *msg, dns_name_t *name, dns_rdata_t *rdata,
		  uint32_t ttl, dns_namelist_t *namelist) {
	isc_region_t r, newr;
	dns_rdata_t *newrdata = NULL;
	dns_name_t *newname = NULL;
	dns_rdatalist_t *newlist = NULL;
	dns_rdataset_t *newset = NULL;
	isc_buffer_t *tmprdatabuf = NULL;

	dns_message_gettemprdata(msg, &newrdata);

	dns_rdata_toregion(rdata, &r);
	isc_buffer_allocate(msg->mctx, &tmprdatabuf, r.length);
	isc_buffer_availableregion(tmprdatabuf, &newr);
	memmove(newr.base, r.base, r.length);
	dns_rdata_fromregion(newrdata, rdata->rdclass, rdata->type, &newr);
	dns_message_takebuffer(msg, &tmprdatabuf);

	dns_message_gettempname(msg, &newname);
	dns_name_copy(name, newname);

	dns_message_gettemprdatalist(msg, &newlist);
	newlist->rdclass = newrdata->rdclass;
	newlist->type = newrdata->type;
	newlist->ttl = ttl;
	ISC_LIST_APPEND(newlist->rdata, newrdata, link);

	dns_message_gettemprdataset(msg, &newset);
	dns_rdatalist_tordataset(newlist, newset);

	ISC_LIST_INIT(newname->list);
	ISC_LIST_APPEND(newname->list, newset, link);

	ISC_LIST_APPEND(*namelist, newname, link);
}

static void
free_namelist(dns_message_t *msg, dns_namelist_t *namelist) {
	dns_name_t *name = NULL;

	while ((name = ISC_LIST_HEAD(*namelist)) != NULL) {
		dns_rdataset_t *set = NULL;
		ISC_LIST_UNLINK(*namelist, name, link);
		while ((set = ISC_LIST_HEAD(name->list)) != NULL) {
			ISC_LIST_UNLINK(name->list, set, link);
			if (dns_rdataset_isassociated(set)) {
				dns_rdataset_disassociate(set);
			}
			dns_message_puttemprdataset(msg, &set);
		}
		dns_message_puttempname(msg, &name);
	}
}

static isc_result_t
process_gsstkey(dns_message_t *msg, dns_name_t *name, dns_rdata_tkey_t *tkeyin,
		dns_tkeyctx_t *tctx, dns_rdata_tkey_t *tkeyout,
		dns_tsigkeyring_t *ring) {
	isc_result_t result = ISC_R_SUCCESS;
	dst_key_t *dstkey = NULL;
	dns_tsigkey_t *tsigkey = NULL;
	dns_fixedname_t fprincipal;
	dns_name_t *principal = dns_fixedname_initname(&fprincipal);
	isc_stdtime_t now = isc_stdtime_now();
	isc_region_t intoken;
	isc_buffer_t *outtoken = NULL;
	dns_gss_ctx_id_t gss_ctx = NULL;

	/*
	 * You have to define either a gss credential (principal) to
	 * accept with tkey-gssapi-credential, or you have to
	 * configure a specific keytab (with tkey-gssapi-keytab) in
	 * order to use gsstkey.
	 */
	if (tctx->gsscred == NULL && tctx->gssapi_keytab == NULL) {
		tkey_log("process_gsstkey(): no tkey-gssapi-credential "
			 "or tkey-gssapi-keytab configured");
		return DNS_R_REFUSED;
	}

	if (!dns_name_equal(&tkeyin->algorithm, DNS_TSIG_GSSAPI_NAME)) {
		tkeyout->error = dns_tsigerror_badalg;
		tkey_log("process_gsstkey(): dns_tsigerror_badalg");
		return ISC_R_SUCCESS;
	}

	/*
	 * XXXDCL need to check for key expiry per 4.1.1
	 * XXXDCL need a way to check fully established, perhaps w/key_flags
	 */
	result = dns_tsigkey_find(&tsigkey, name, &tkeyin->algorithm, ring);
	if (result == ISC_R_SUCCESS) {
		gss_ctx = dst_key_getgssctx(tsigkey->key);
	}

	/*
	 * Note that tctx->gsscred may be NULL if tctx->gssapi_keytab is set
	 */
	intoken = (isc_region_t){ tkeyin->key, tkeyin->keylen };
	result = dst_gssapi_acceptctx(tctx->gsscred, tctx->gssapi_keytab,
				      &intoken, &outtoken, &gss_ctx, principal,
				      tctx->mctx);
	if (result == DNS_R_INVALIDTKEY) {
		if (tsigkey != NULL) {
			dns_tsigkey_detach(&tsigkey);
		}
		tkeyout->error = dns_tsigerror_badkey;
		tkey_log("process_gsstkey(): dns_tsigerror_badkey");
		return ISC_R_SUCCESS;
	}
	if (result != DNS_R_CONTINUE && result != ISC_R_SUCCESS) {
		goto failure;
	}

	/*
	 * XXXDCL Section 4.1.3: Limit GSS_S_CONTINUE_NEEDED to 10 times.
	 */
	if (dns_name_countlabels(principal) == 0U) {
		if (tsigkey != NULL) {
			dns_tsigkey_detach(&tsigkey);
		}
	} else if (tsigkey == NULL) {
#if HAVE_GSSAPI
		OM_uint32 gret, minor, lifetime;
#endif /* HAVE_GSSAPI */
		uint32_t expire;

		RETERR(dst_key_fromgssapi(name, gss_ctx, ring->mctx, &dstkey,
					  &intoken));
		/*
		 * Limit keys to 1 hour or the context's lifetime whichever
		 * is smaller.
		 */
		expire = now + 3600;
#if HAVE_GSSAPI
		gret = gss_context_time(&minor, gss_ctx, &lifetime);
		if (gret == GSS_S_COMPLETE && now + lifetime < expire) {
			expire = now + lifetime;
		}
#endif /* HAVE_GSSAPI */
		RETERR(dns_tsigkey_createfromkey(
			name, dns__tsig_algfromname(&tkeyin->algorithm), dstkey,
			true, false, principal, now, expire, ring->mctx,
			&tsigkey));
		RETERR(dns_tsigkeyring_add(ring, tsigkey));
		dst_key_free(&dstkey);
		tkeyout->inception = now;
		tkeyout->expire = expire;
	} else {
		tkeyout->inception = tsigkey->inception;
		tkeyout->expire = tsigkey->expire;
	}

	if (outtoken != NULL) {
		tkeyout->key = isc_mem_get(tkeyout->mctx,
					   isc_buffer_usedlength(outtoken));
		tkeyout->keylen = isc_buffer_usedlength(outtoken);
		memmove(tkeyout->key, isc_buffer_base(outtoken),
			isc_buffer_usedlength(outtoken));
		isc_buffer_free(&outtoken);
	} else {
		tkeyout->key = isc_mem_get(tkeyout->mctx, tkeyin->keylen);
		tkeyout->keylen = tkeyin->keylen;
		memmove(tkeyout->key, tkeyin->key, tkeyin->keylen);
	}

	/*
	 * We found a TKEY to respond with.  If the request is not TSIG signed,
	 * we need to make sure the response is signed (see RFC 3645, Section
	 * 2.2).
	 */
	if (tsigkey != NULL) {
		if (msg->tsigkey == NULL && msg->sig0key == NULL) {
			dns_message_settsigkey(msg, tsigkey);
		}
		dns_tsigkey_detach(&tsigkey);
	}

	return ISC_R_SUCCESS;

failure:
	if (tsigkey != NULL) {
		dns_tsigkey_detach(&tsigkey);
	}
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	if (outtoken != NULL) {
		isc_buffer_free(&outtoken);
	}

	tkey_log("process_gsstkey(): %s", isc_result_totext(result));
	return result;
}

static isc_result_t
process_deletetkey(dns_name_t *signer, dns_name_t *name,
		   dns_rdata_tkey_t *tkeyin, dns_rdata_tkey_t *tkeyout,
		   dns_tsigkeyring_t *ring) {
	isc_result_t result;
	dns_tsigkey_t *tsigkey = NULL;
	const dns_name_t *identity = NULL;

	result = dns_tsigkey_find(&tsigkey, name, &tkeyin->algorithm, ring);
	if (result != ISC_R_SUCCESS) {
		tkeyout->error = dns_tsigerror_badname;
		return ISC_R_SUCCESS;
	}

	/*
	 * Only allow a delete if the identity that created the key is the
	 * same as the identity that signed the message.
	 */
	identity = dns_tsigkey_identity(tsigkey);
	if (identity == NULL || !dns_name_equal(identity, signer)) {
		dns_tsigkey_detach(&tsigkey);
		return DNS_R_REFUSED;
	}

	/*
	 * Set the key to be deleted when no references are left.  If the key
	 * was not generated with TKEY and is in the config file, it may be
	 * reloaded later.
	 */
	dns_tsigkey_delete(tsigkey);

	/* Release the reference */
	dns_tsigkey_detach(&tsigkey);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_tkey_processquery(dns_message_t *msg, dns_tkeyctx_t *tctx,
		      dns_tsigkeyring_t *ring) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_tkey_t tkeyin, tkeyout;
	dns_name_t *qname = NULL, *name = NULL;
	dns_name_t *keyname = NULL, *signer = NULL;
	dns_name_t tsigner = DNS_NAME_INITEMPTY;
	dns_fixedname_t fkeyname;
	dns_rdataset_t *tkeyset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_namelist_t namelist = ISC_LIST_INITIALIZER;
	char tkeyoutdata[512];
	isc_buffer_t tkeyoutbuf;
	dns_tsigkey_t *tsigkey = NULL;

	REQUIRE(msg != NULL);
	REQUIRE(tctx != NULL);
	REQUIRE(ring != NULL);

	/*
	 * Interpret the question section.
	 */
	result = dns_message_firstname(msg, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS) {
		return DNS_R_FORMERR;
	}

	dns_message_currentname(msg, DNS_SECTION_QUESTION, &qname);

	/*
	 * Look for a TKEY record that matches the question.
	 */
	result = dns_message_findname(msg, DNS_SECTION_ADDITIONAL, qname,
				      dns_rdatatype_tkey, 0, &name, &tkeyset);
	if (result != ISC_R_SUCCESS) {
		result = DNS_R_FORMERR;
		tkey_log("dns_tkey_processquery: couldn't find a TKEY "
			 "matching the question");
		goto failure;
	}

	result = dns_rdataset_first(tkeyset);
	if (result != ISC_R_SUCCESS) {
		result = DNS_R_FORMERR;
		goto failure;
	}

	dns_rdataset_current(tkeyset, &rdata);
	RETERR(dns_rdata_tostruct(&rdata, &tkeyin, NULL));

	if (tkeyin.error != dns_rcode_noerror) {
		result = DNS_R_FORMERR;
		goto failure;
	}

	/*
	 * Before we go any farther, verify that the message was signed.
	 * DNS_TKEYMODE_GSSAPI doesn't require a signature, but other
	 * modes do.
	 */
	result = dns_message_signer(msg, &tsigner);
	if (result == ISC_R_SUCCESS) {
		signer = &tsigner;
	} else if (result != ISC_R_NOTFOUND ||
		   tkeyin.mode != DNS_TKEYMODE_GSSAPI)
	{
		tkey_log("dns_tkey_processquery: query was not "
			 "properly signed - rejecting");
		result = DNS_R_FORMERR;
		goto failure;
	}

	tkeyout = (dns_rdata_tkey_t){
		.common.rdclass = tkeyin.common.rdclass,
		.common.rdtype = tkeyin.common.rdtype,
		.common.link = ISC_LINK_INITIALIZER,
		.mctx = msg->mctx,
		.algorithm = DNS_NAME_INITEMPTY,
		.mode = tkeyin.mode,
	};
	dns_name_clone(&tkeyin.algorithm, &tkeyout.algorithm);

	switch (tkeyin.mode) {
	case DNS_TKEYMODE_DELETE:
		/*
		 * A delete operation uses the fully specified qname.
		 */
		RETERR(process_deletetkey(signer, qname, &tkeyin, &tkeyout,
					  ring));
		break;
	case DNS_TKEYMODE_GSSAPI:
		/*
		 * For non-delete operations we do this:
		 *
		 * if (qname != ".")
		 *	keyname = qname + defaultdomain
		 * else
		 *	keyname = <random hex> + defaultdomain
		 */
		if (tctx->domain == NULL && tkeyin.mode != DNS_TKEYMODE_GSSAPI)
		{
			tkey_log("dns_tkey_processquery: tkey-domain not set");
			result = DNS_R_REFUSED;
			goto failure;
		}

		keyname = dns_fixedname_initname(&fkeyname);

		if (!dns_name_equal(qname, dns_rootname)) {
			unsigned int n = dns_name_countlabels(qname);
			dns_name_copy(qname, keyname);
			dns_name_getlabelsequence(keyname, 0, n - 1, keyname);
		} else {
			unsigned char randomdata[16];
			char randomtext[32];
			isc_buffer_t b;
			isc_region_t r = {
				.base = randomdata,
				.length = sizeof(randomdata),
			};

			isc_nonce_buf(randomdata, sizeof(randomdata));
			isc_buffer_init(&b, randomtext, sizeof(randomtext));
			RETERR(isc_hex_totext(&r, 2, "", &b));
			RETERR(dns_name_fromtext(keyname, &b, NULL, 0, NULL));
		}
		RETERR(dns_name_concatenate(keyname, dns_rootname, keyname,
					    NULL));

		result = dns_tsigkey_find(&tsigkey, keyname, NULL, ring);
		if (result == ISC_R_SUCCESS) {
			tkeyout.error = dns_tsigerror_badname;
			dns_tsigkey_detach(&tsigkey);
			break;
		} else if (result == ISC_R_NOTFOUND) {
			RETERR(process_gsstkey(msg, keyname, &tkeyin, tctx,
					       &tkeyout, ring));
			break;
		}
		goto failure;
	case DNS_TKEYMODE_SERVERASSIGNED:
	case DNS_TKEYMODE_RESOLVERASSIGNED:
		result = DNS_R_NOTIMP;
		goto failure;
	default:
		tkeyout.error = dns_tsigerror_badmode;
	}

	dns_rdata_init(&rdata);
	isc_buffer_init(&tkeyoutbuf, tkeyoutdata, sizeof(tkeyoutdata));
	result = dns_rdata_fromstruct(&rdata, tkeyout.common.rdclass,
				      tkeyout.common.rdtype, &tkeyout,
				      &tkeyoutbuf);
	if (tkeyout.key != NULL) {
		isc_mem_put(tkeyout.mctx, tkeyout.key, tkeyout.keylen);
	}
	RETERR(result);

	RETERR(dns_message_reply(msg, true));
	add_rdata_to_list(msg, keyname, &rdata, 0, &namelist);
	while ((name = ISC_LIST_HEAD(namelist)) != NULL) {
		ISC_LIST_UNLINK(namelist, name, link);
		dns_message_addname(msg, name, DNS_SECTION_ANSWER);
	}
	return ISC_R_SUCCESS;

failure:
	free_namelist(msg, &namelist);
	return result;
}

static isc_result_t
buildquery(dns_message_t *msg, const dns_name_t *name, dns_rdata_tkey_t *tkey) {
	dns_name_t *qname = NULL, *aname = NULL;
	dns_rdataset_t *question = NULL, *tkeyset = NULL;
	dns_rdatalist_t *tkeylist = NULL;
	dns_rdata_t *rdata = NULL;
	isc_buffer_t *dynbuf = NULL;
	isc_result_t result;
	unsigned int len;

	REQUIRE(msg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(tkey != NULL);

	len = 16 + tkey->algorithm.length + tkey->keylen + tkey->otherlen;
	isc_buffer_allocate(msg->mctx, &dynbuf, len);
	dns_message_gettemprdata(msg, &rdata);
	result = dns_rdata_fromstruct(rdata, dns_rdataclass_any,
				      dns_rdatatype_tkey, tkey, dynbuf);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttemprdata(msg, &rdata);
		isc_buffer_free(&dynbuf);
		return result;
	}
	dns_message_takebuffer(msg, &dynbuf);

	dns_message_gettempname(msg, &qname);
	dns_message_gettempname(msg, &aname);

	dns_message_gettemprdataset(msg, &question);
	dns_rdataset_makequestion(question, dns_rdataclass_any,
				  dns_rdatatype_tkey);

	dns_message_gettemprdatalist(msg, &tkeylist);
	tkeylist->rdclass = dns_rdataclass_any;
	tkeylist->type = dns_rdatatype_tkey;
	ISC_LIST_APPEND(tkeylist->rdata, rdata, link);

	dns_message_gettemprdataset(msg, &tkeyset);
	dns_rdatalist_tordataset(tkeylist, tkeyset);

	dns_name_copy(name, qname);
	dns_name_copy(name, aname);

	ISC_LIST_APPEND(qname->list, question, link);
	ISC_LIST_APPEND(aname->list, tkeyset, link);

	dns_message_addname(msg, qname, DNS_SECTION_QUESTION);
	dns_message_addname(msg, aname, DNS_SECTION_ADDITIONAL);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_tkey_buildgssquery(dns_message_t *msg, const dns_name_t *name,
		       const dns_name_t *gname, uint32_t lifetime,
		       dns_gss_ctx_id_t *context, isc_mem_t *mctx,
		       char **err_message) {
	dns_rdata_tkey_t tkey;
	isc_result_t result;
	isc_stdtime_t now = isc_stdtime_now();
	isc_buffer_t token;
	unsigned char array[TEMP_BUFFER_SZ];

	REQUIRE(msg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(gname != NULL);
	REQUIRE(context != NULL);
	REQUIRE(mctx != NULL);

	isc_buffer_init(&token, array, sizeof(array));
	result = dst_gssapi_initctx(gname, NULL, &token, context, mctx,
				    err_message);
	if (result != DNS_R_CONTINUE && result != ISC_R_SUCCESS) {
		return result;
	}

	tkey = (dns_rdata_tkey_t){
		.common.rdclass = dns_rdataclass_any,
		.common.rdtype = dns_rdatatype_tkey,
		.common.link = ISC_LINK_INITIALIZER,
		.inception = now,
		.expire = now + lifetime,
		.algorithm = DNS_NAME_INITEMPTY,
		.mode = DNS_TKEYMODE_GSSAPI,
		.key = isc_buffer_base(&token),
		.keylen = isc_buffer_usedlength(&token),
	};
	dns_name_clone(DNS_TSIG_GSSAPI_NAME, &tkey.algorithm);

	return buildquery(msg, name, &tkey);
}

static isc_result_t
find_tkey(dns_message_t *msg, dns_name_t **name, dns_rdata_t *rdata,
	  int section) {
	isc_result_t result;

	result = dns_message_firstname(msg, section);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_t *tkeyset = NULL;
		dns_name_t *cur = NULL;

		dns_message_currentname(msg, section, &cur);
		result = dns_message_findtype(cur, dns_rdatatype_tkey, 0,
					      &tkeyset);
		if (result == ISC_R_SUCCESS) {
			result = dns_rdataset_first(tkeyset);
			if (result != ISC_R_SUCCESS) {
				break;
			}

			dns_rdataset_current(tkeyset, rdata);
			*name = cur;
			return ISC_R_SUCCESS;
		}
		result = dns_message_nextname(msg, section);
	}
	if (result == ISC_R_NOMORE) {
		return ISC_R_NOTFOUND;
	}
	return result;
}

isc_result_t
dns_tkey_gssnegotiate(dns_message_t *qmsg, dns_message_t *rmsg,
		      const dns_name_t *server, dns_gss_ctx_id_t *context,
		      dns_tsigkey_t **outkey, dns_tsigkeyring_t *ring,
		      char **err_message) {
	isc_result_t result;
	dns_rdata_t rtkeyrdata = DNS_RDATA_INIT, qtkeyrdata = DNS_RDATA_INIT;
	dns_name_t *tkeyname = NULL;
	dns_rdata_tkey_t rtkey, qtkey, tkey;
	isc_buffer_t intoken, outtoken;
	dst_key_t *dstkey = NULL;
	unsigned char array[TEMP_BUFFER_SZ];
	dns_tsigkey_t *tsigkey = NULL;

	REQUIRE(qmsg != NULL);
	REQUIRE(rmsg != NULL);
	REQUIRE(server != NULL);
	REQUIRE(outkey == NULL || *outkey == NULL);

	if (rmsg->rcode != dns_rcode_noerror) {
		return dns_result_fromrcode(rmsg->rcode);
	}

	RETERR(find_tkey(rmsg, &tkeyname, &rtkeyrdata, DNS_SECTION_ANSWER));
	RETERR(dns_rdata_tostruct(&rtkeyrdata, &rtkey, NULL));

	RETERR(find_tkey(qmsg, &tkeyname, &qtkeyrdata, DNS_SECTION_ADDITIONAL));
	RETERR(dns_rdata_tostruct(&qtkeyrdata, &qtkey, NULL));

	if (rtkey.error != dns_rcode_noerror ||
	    rtkey.mode != DNS_TKEYMODE_GSSAPI ||
	    !dns_name_equal(&rtkey.algorithm, &qtkey.algorithm))
	{
		tkey_log("dns_tkey_gssnegotiate: tkey mode invalid "
			 "or error set(4)");
		result = DNS_R_INVALIDTKEY;
		goto failure;
	}

	isc_buffer_init(&intoken, rtkey.key, rtkey.keylen);
	isc_buffer_init(&outtoken, array, sizeof(array));

	result = dst_gssapi_initctx(server, &intoken, &outtoken, context,
				    ring->mctx, err_message);
	if (result != DNS_R_CONTINUE && result != ISC_R_SUCCESS) {
		return result;
	}

	if (result == DNS_R_CONTINUE) {
		tkey = (dns_rdata_tkey_t){
			.common.rdclass = dns_rdataclass_any,
			.common.rdtype = dns_rdatatype_tkey,
			.common.link = ISC_LINK_INITIALIZER,
			.inception = qtkey.inception,
			.expire = qtkey.expire,
			.algorithm = DNS_NAME_INITEMPTY,
			.mode = DNS_TKEYMODE_GSSAPI,
			.key = isc_buffer_base(&outtoken),
			.keylen = isc_buffer_usedlength(&outtoken),
		};

		dns_name_clone(DNS_TSIG_GSSAPI_NAME, &tkey.algorithm);

		dns_message_reset(qmsg, DNS_MESSAGE_INTENTRENDER);
		RETERR(buildquery(qmsg, tkeyname, &tkey));
		return DNS_R_CONTINUE;
	}

	RETERR(dst_key_fromgssapi(dns_rootname, *context, rmsg->mctx, &dstkey,
				  NULL));

	/*
	 * XXXSRA This seems confused.  If we got CONTINUE from initctx,
	 * the GSS negotiation hasn't completed yet, so we can't sign
	 * anything yet.
	 */
	RETERR(dns_tsigkey_createfromkey(tkeyname, DST_ALG_GSSAPI, dstkey, true,
					 false, NULL, rtkey.inception,
					 rtkey.expire, ring->mctx, &tsigkey));
	RETERR(dns_tsigkeyring_add(ring, tsigkey));
	if (outkey == NULL) {
		dns_tsigkey_detach(&tsigkey);
	} else {
		*outkey = tsigkey;
	}

	dst_key_free(&dstkey);
	return result;

failure:
	if (tsigkey != NULL) {
		dns_tsigkey_detach(&tsigkey);
	}
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	return result;
}
