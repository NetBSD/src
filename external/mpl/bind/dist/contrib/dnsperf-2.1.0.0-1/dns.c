/*	$NetBSD: dns.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2000, 2001  Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (C) 2004 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ISC_BUFFER_USEINLINE

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/hmacmd5.h>
#include <isc/hmacsha.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/ttl.h>

#include "dns.h"
#include "log.h"
#include "opt.h"

#define WHITESPACE			" \t\n"

#define MAX_RDATA_LENGTH		65535
#define EDNSLEN				11

const char *perf_dns_rcode_strings[] = {
	"NOERROR", "FORMERR", "SERVFAIL", "NXDOMAIN",
	"NOTIMP", "REFUSED", "YXDOMAIN", "YXRRSET",
	"NXRRSET", "NOTAUTH", "NOTZONE", "rcode11",
	"rcode12", "rcode13", "rcode14", "rcode15"
};

#define TSIG_HMACMD5_NAME		"\010hmac-md5\007sig-alg\003reg\003int"
#define TSIG_HMACSHA1_NAME		"\011hmac-sha1"
#define TSIG_HMACSHA224_NAME		"\013hmac-sha224"
#define TSIG_HMACSHA256_NAME		"\013hmac-sha256"
#define TSIG_HMACSHA384_NAME		"\013hmac-sha384"
#define TSIG_HMACSHA512_NAME		"\013hmac-sha512"

typedef enum {
	TSIG_HMACMD5,
	TSIG_HMACSHA1,
	TSIG_HMACSHA224,
	TSIG_HMACSHA256,
	TSIG_HMACSHA384,
	TSIG_HMACSHA512
} hmac_type_t;

typedef union {
	isc_hmacmd5_t hmacmd5;
	isc_hmacsha1_t hmacsha1;
	isc_hmacsha224_t hmacsha224;
	isc_hmacsha256_t hmacsha256;
	isc_hmacsha384_t hmacsha384;
	isc_hmacsha512_t hmacsha512;
} hmac_ctx_t;

struct perf_dnstsigkey {
	isc_mem_t *mctx;
	isc_constregion_t alg;
	hmac_type_t hmactype;
	unsigned int digestlen;
	dns_fixedname_t fname;
	dns_name_t *name;
	unsigned char secretdata[256];
	isc_buffer_t secret;
};

struct perf_dnsctx {
	isc_mem_t *mctx;
	dns_compress_t compress;
	isc_lex_t *lexer;
};

perf_dnsctx_t *
perf_dns_createctx(isc_boolean_t updates)
{
	isc_mem_t *mctx;
	perf_dnsctx_t *ctx;
	isc_result_t result;

	if (!updates)
		return NULL;

	mctx = NULL;
	result = isc_mem_create(0, 0, &mctx);
	if (result != ISC_R_SUCCESS)
		perf_log_fatal("creating memory context: %s",
			       isc_result_totext(result));

	ctx = isc_mem_get(mctx, sizeof(*ctx));
	if (ctx == NULL)
		perf_log_fatal("out of memory");

	memset(ctx, 0, sizeof(*ctx));
	ctx->mctx = mctx;

	result = dns_compress_init(&ctx->compress, 0, ctx->mctx);
	if (result != ISC_R_SUCCESS) {
		perf_log_fatal("creating compression context: %s",
			       isc_result_totext(result));
	}
	dns_compress_setmethods(&ctx->compress, DNS_COMPRESS_GLOBAL14);

	result = isc_lex_create(ctx->mctx, 1024, &ctx->lexer);
	if (result != ISC_R_SUCCESS) {
		perf_log_fatal("creating lexer: %s",
			       isc_result_totext(result));
	}

	return (ctx);
}

void
perf_dns_destroyctx(perf_dnsctx_t **ctxp)
{
	perf_dnsctx_t *ctx;
	isc_mem_t *mctx;

	INSIST(ctxp != NULL);
	ctx = *ctxp;
	*ctxp = NULL;

	if (ctx == NULL)
		return;

	mctx = ctx->mctx;
	isc_lex_destroy(&ctx->lexer);
	dns_compress_invalidate(&ctx->compress);
	isc_mem_put(mctx, ctx, sizeof(*ctx));
	isc_mem_destroy(&mctx);
}

static isc_result_t
name_fromstring(dns_name_t *name, dns_name_t *origin,
		const char *str, unsigned int len,
		isc_buffer_t *target, const char *type)
{
	isc_buffer_t buffer;
	isc_result_t result;

	isc_buffer_init(&buffer, str, len);
	isc_buffer_add(&buffer, len);
	result = dns_name_fromtext(name, &buffer, origin, 0, target);
	if (result != ISC_R_SUCCESS)
		perf_log_warning("invalid %s name: %.*s", type, (int)len, str);
	return result;
}

#define SET_KEY(key, type) do {					\
	(key)->alg.base = TSIG_HMAC ## type ## _NAME;		\
	(key)->alg.length = sizeof(TSIG_HMAC ## type ## _NAME);	\
	(key)->hmactype = TSIG_HMAC ## type;			\
	(key)->digestlen = ISC_ ## type ## _DIGESTLENGTH;	\
	} while (0)

perf_dnstsigkey_t *
perf_dns_parsetsigkey(const char *arg, isc_mem_t *mctx)
{
	perf_dnstsigkey_t *tsigkey;
	const char *sep1, *sep2, *alg, *name, *secret;
	int alglen, namelen;
	isc_result_t result;

	tsigkey = isc_mem_get(mctx, sizeof (*tsigkey));
	if (tsigkey == NULL)
		perf_log_fatal("out of memory");
	memset(tsigkey, 0, sizeof (*tsigkey));
	tsigkey->mctx = mctx;

	sep1 = strchr(arg, ':');
	if (sep1 == NULL) {
		perf_log_warning("invalid TSIG [alg:]name:secret");
		perf_opt_usage();
		exit(1);
	}

	sep2 = strchr(sep1 + 1, ':');
	if (sep2 == NULL) {
		/* name:key */
		alg = NULL;
		alglen = 0;
		name = arg;
		namelen = sep1 - arg;
		secret = sep1 + 1;
	} else {
		/* [alg:]name:secret */
		alg = arg;
		alglen = sep1 - arg;
		name = sep1 + 1;
		namelen = sep2 - sep1 - 1;
		secret = sep2 + 1;
	}

	/* Algorithm */

	if (alg == NULL || strncasecmp(alg, "hmac-md5:", 9) == 0) {
		SET_KEY(tsigkey, MD5);
	} else if (strncasecmp(alg, "hmac-sha1:", 10) == 0) {
		SET_KEY(tsigkey, SHA1);
	} else if (strncasecmp(alg, "hmac-sha224:", 12) == 0) {
		SET_KEY(tsigkey, SHA224);
	} else if (strncasecmp(alg, "hmac-sha256:", 12) == 0) {
		SET_KEY(tsigkey, SHA256);
	} else if (strncasecmp(alg, "hmac-sha384:", 12) == 0) {
		SET_KEY(tsigkey, SHA384);
	} else if (strncasecmp(alg, "hmac-sha512:", 12) == 0) {
		SET_KEY(tsigkey, SHA512);
	} else {
		perf_log_warning("invalid TSIG algorithm %.*s", alglen, alg);
		perf_opt_usage();
		exit(1);
	}

	/* Name */

	tsigkey->name = dns_fixedname_initname(&tsigkey->fname);
	result = name_fromstring(tsigkey->name, dns_rootname, name, namelen,
				 NULL, "TSIG key");
	if (result != ISC_R_SUCCESS) {
		perf_opt_usage();
		exit(1);
	}
	(void)dns_name_downcase(tsigkey->name, tsigkey->name, NULL);

	/* Secret */

	isc_buffer_init(&tsigkey->secret, tsigkey->secretdata,
			sizeof(tsigkey->secretdata));
	result = isc_base64_decodestring(secret, &tsigkey->secret);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("invalid TSIG secret '%s'", secret);
		perf_opt_usage();
		exit(1);
	}

	return tsigkey;
}

void
perf_dns_destroytsigkey(perf_dnstsigkey_t **tsigkeyp)
{
	perf_dnstsigkey_t *tsigkey;

	INSIST(tsigkeyp != NULL && *tsigkeyp != NULL);

	tsigkey = *tsigkeyp;
	*tsigkeyp = NULL;

	isc_mem_put(tsigkey->mctx, tsigkey, sizeof(*tsigkey));
}

/*
 * Appends an OPT record to the packet.
 */
static isc_result_t
add_edns(isc_buffer_t *packet, isc_boolean_t dnssec) {
	unsigned char *base;

	if (isc_buffer_availablelength(packet) < EDNSLEN) {
		perf_log_warning("failed to add OPT to query packet");
		return (ISC_R_NOSPACE);
	}

	base = isc_buffer_base(packet);

	isc_buffer_putuint8(packet, 0);			/* root name */
	isc_buffer_putuint16(packet, dns_rdatatype_opt);/* type */
	isc_buffer_putuint16(packet, MAX_EDNS_PACKET);	/* class */
	isc_buffer_putuint8(packet, 0);			/* xrcode */
	isc_buffer_putuint8(packet, 0);			/* version */
	if (dnssec)					/* flags */
		isc_buffer_putuint16(packet, 0x8000);
	else
		isc_buffer_putuint16(packet, 0);
	isc_buffer_putuint16(packet, 0);		/* rdlen */

	base[11]++;				/* increment record count */

	return (ISC_R_SUCCESS);
}

static void
hmac_init(perf_dnstsigkey_t *tsigkey, hmac_ctx_t *ctx)
{
	unsigned char *secret;
	unsigned int length;

	secret = isc_buffer_base(&tsigkey->secret);
	length = isc_buffer_usedlength(&tsigkey->secret);

	switch (tsigkey->hmactype) {
	case TSIG_HMACMD5:
		isc_hmacmd5_init(&ctx->hmacmd5, secret, length);
		break;
	case TSIG_HMACSHA1:
		isc_hmacsha1_init(&ctx->hmacsha1, secret, length);
		break;
	case TSIG_HMACSHA224:
		isc_hmacsha224_init(&ctx->hmacsha224, secret, length);
		break;
	case TSIG_HMACSHA256:
		isc_hmacsha256_init(&ctx->hmacsha256, secret, length);
		break;
	case TSIG_HMACSHA384:
		isc_hmacsha384_init(&ctx->hmacsha384, secret, length);
		break;
	case TSIG_HMACSHA512:
		isc_hmacsha512_init(&ctx->hmacsha512, secret, length);
		break;
	}
}

static void
hmac_update(perf_dnstsigkey_t *tsigkey, hmac_ctx_t *ctx,
	    unsigned char *data, unsigned int length)
{
	switch (tsigkey->hmactype) {
	case TSIG_HMACMD5:
		isc_hmacmd5_update(&ctx->hmacmd5, data, length);
		break;
	case TSIG_HMACSHA1:
		isc_hmacsha1_update(&ctx->hmacsha1, data, length);
		break;
	case TSIG_HMACSHA224:
		isc_hmacsha224_update(&ctx->hmacsha224, data, length);
		break;
	case TSIG_HMACSHA256:
		isc_hmacsha256_update(&ctx->hmacsha256, data, length);
		break;
	case TSIG_HMACSHA384:
		isc_hmacsha384_update(&ctx->hmacsha384, data, length);
		break;
	case TSIG_HMACSHA512:
		isc_hmacsha512_update(&ctx->hmacsha512, data, length);
		break;
	}
}

static void
hmac_sign(perf_dnstsigkey_t *tsigkey, hmac_ctx_t *ctx, unsigned char *digest,
	  unsigned int digestlen)
{
	switch (tsigkey->hmactype) {
	case TSIG_HMACMD5:
		isc_hmacmd5_sign(&ctx->hmacmd5, digest);
		break;
	case TSIG_HMACSHA1:
		isc_hmacsha1_sign(&ctx->hmacsha1, digest, digestlen);
		break;
	case TSIG_HMACSHA224:
		isc_hmacsha224_sign(&ctx->hmacsha224, digest, digestlen);
		break;
	case TSIG_HMACSHA256:
		isc_hmacsha256_sign(&ctx->hmacsha256, digest, digestlen);
		break;
	case TSIG_HMACSHA384:
		isc_hmacsha384_sign(&ctx->hmacsha384, digest, digestlen);
		break;
	case TSIG_HMACSHA512:
		isc_hmacsha512_sign(&ctx->hmacsha512, digest, digestlen);
		break;
	}
}

/*
 * Appends a TSIG record to the packet.
 */
static isc_result_t
add_tsig(isc_buffer_t *packet, perf_dnstsigkey_t *tsigkey)
{
	unsigned char *base;
	hmac_ctx_t hmac;
	isc_region_t name_r;
	isc_region_t *alg_r;
	unsigned int rdlen, totallen;
	unsigned char tmpdata[512];
	isc_buffer_t tmp;
	isc_uint32_t now;
	unsigned char digest[ISC_SHA256_DIGESTLENGTH];

	hmac_init(tsigkey, &hmac);
	now = time(NULL);
	dns_name_toregion(tsigkey->name, &name_r);
	alg_r = (isc_region_t *) &tsigkey->alg;

	/* Make sure everything will fit */
	rdlen = alg_r->length + 16 + tsigkey->digestlen;
	totallen = name_r.length + 10 + rdlen;
	if (totallen > isc_buffer_availablelength(packet)) {
		perf_log_warning("adding TSIG: out of space");
		return (ISC_R_NOSPACE);
	}

	base = isc_buffer_base(packet);

	/* Digest the message */
	hmac_update(tsigkey, &hmac, isc_buffer_base(packet),
		    isc_buffer_usedlength(packet));

	/* Digest the TSIG record */
	isc_buffer_init(&tmp, tmpdata, sizeof tmpdata);
	isc_buffer_copyregion(&tmp, &name_r);		/* name */
	isc_buffer_putuint16(&tmp, dns_rdataclass_any);	/* class */
	isc_buffer_putuint32(&tmp, 0);			/* ttl */
	isc_buffer_copyregion(&tmp, alg_r);		/* alg */
	isc_buffer_putuint16(&tmp, 0);			/* time high */
	isc_buffer_putuint32(&tmp, now);		/* time low */
	isc_buffer_putuint16(&tmp, 300);		/* fudge */
	isc_buffer_putuint16(&tmp, 0);			/* error */
	isc_buffer_putuint16(&tmp, 0);			/* other length */
	hmac_update(tsigkey, &hmac, isc_buffer_base(&tmp),
		    isc_buffer_usedlength(&tmp));
	hmac_sign(tsigkey, &hmac, digest, tsigkey->digestlen);

	/* Add the TSIG record. */
	isc_buffer_copyregion(packet, &name_r);			/* name */
	isc_buffer_putuint16(packet, dns_rdatatype_tsig);	/* type */
	isc_buffer_putuint16(packet, dns_rdataclass_any);	/* class */
	isc_buffer_putuint32(packet, 0);			/* ttl */
	isc_buffer_putuint16(packet, rdlen);			/* rdlen */
	isc_buffer_copyregion(packet, alg_r);			/* alg */
	isc_buffer_putuint16(packet, 0);			/* time high */
	isc_buffer_putuint32(packet, now);			/* time low */
	isc_buffer_putuint16(packet, 300);			/* fudge */
	isc_buffer_putuint16(packet, tsigkey->digestlen);	/* digest len */
	isc_buffer_putmem(packet, digest, tsigkey->digestlen);	/* digest */
	isc_buffer_putmem(packet, base, 2);			/* orig ID */
	isc_buffer_putuint16(packet, 0);			/* error */
	isc_buffer_putuint16(packet, 0);			/* other len */

	base[11]++;				/* increment record count */

	return (ISC_R_SUCCESS);
}

static isc_result_t
build_query(const isc_textregion_t *line, isc_buffer_t *msg)
{
	char *domain_str;
	int domain_len;
	dns_name_t name;
	dns_offsets_t offsets;
	isc_textregion_t qtype_r;
	dns_rdatatype_t qtype;
	isc_result_t result;

	domain_str = line->base;
	domain_len = strcspn(line->base, WHITESPACE);

	qtype_r.base = line->base + domain_len;
	while (isspace(*qtype_r.base & 0xff))
		qtype_r.base++;
	qtype_r.length = strcspn(qtype_r.base, WHITESPACE);

	/* Create the question section */
	DNS_NAME_INIT(&name, offsets);
	result = name_fromstring(&name, dns_rootname, domain_str, domain_len,
				 msg, "domain");
	if (result != ISC_R_SUCCESS)
		return (result);

	if (qtype_r.length == 0) {
		perf_log_warning("invalid query input format: %s", line->base);
		return (ISC_R_FAILURE);
	}
	result = dns_rdatatype_fromtext(&qtype, &qtype_r);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("invalid query type: %.*s",
				 (int) qtype_r.length, qtype_r.base);
		return (ISC_R_FAILURE);
	}

	isc_buffer_putuint16(msg, qtype);
	isc_buffer_putuint16(msg, dns_rdataclass_in);

	return ISC_R_SUCCESS;
}

static isc_boolean_t
token_equals(const isc_textregion_t *token, const char *str)
{
	return (strlen(str) == token->length &&
		strncasecmp(str, token->base, token->length) == 0);
}

/*
 * Reads one line containing an individual update for a dynamic update message.
 */
static isc_result_t
read_update_line(perf_dnsctx_t *ctx, const isc_textregion_t *line, char *str,
		 dns_name_t *zname, int want_ttl, int need_type,
		 int want_rdata, int need_rdata, dns_name_t *name,
		 isc_uint32_t *ttlp, dns_rdatatype_t *typep,
		 dns_rdata_t *rdata, isc_buffer_t *rdatabuf)
{
	char *curr_str;
	unsigned int curr_len;
	isc_buffer_t buffer;
	isc_textregion_t src;
	dns_rdatacallbacks_t callbacks;
	isc_result_t result;

	while (isspace(*str & 0xff))
		str++;

	/* Read the owner name */
	curr_str = str;
	curr_len = strcspn(curr_str, WHITESPACE);
	result = name_fromstring(name, zname, curr_str, curr_len, NULL,
				 "owner");
	if (result != ISC_R_SUCCESS)
		return (result);
	str += curr_len;
	while (isspace(*str & 0xff))
		str++;

	/* Read the ttl */
	if (want_ttl) {
		curr_str = str;
		curr_len = strcspn(curr_str, WHITESPACE);
		src.base = curr_str;
		src.length = curr_len;
		result = dns_ttl_fromtext(&src, ttlp);
		if (result != ISC_R_SUCCESS) {
			perf_log_warning("invalid ttl: %.*s",
					 curr_len, curr_str);
			return (result);
		}
		str += curr_len;
		while (isspace(*str & 0xff))
			str++;
	}

	/* Read the type */
	curr_str = str;
	curr_len = strcspn(curr_str, WHITESPACE);
	if (curr_len == 0) {
		if (!need_type)
			return (ISC_R_SUCCESS);
		perf_log_warning("invalid update command: %s", line->base);
		return (ISC_R_SUCCESS);
	}
	src.base = curr_str;
	src.length = curr_len;
	result = dns_rdatatype_fromtext(typep, &src);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("invalid type: %.*s", curr_len, curr_str);
		return (result);
	}
	str += curr_len;
	while (isspace(*str & 0xff))
		str++;

	/* Read the rdata */
	if (!want_rdata)
		return (ISC_R_SUCCESS);

	if (*str == 0) {
		if (!need_rdata)
			return (ISC_R_SUCCESS);
		perf_log_warning("invalid update command: %s\n", line->base);
		return (ISC_R_FAILURE);
	}

	isc_buffer_init(&buffer, str, strlen(str));
	isc_buffer_add(&buffer, strlen(str));
	result = isc_lex_openbuffer(ctx->lexer, &buffer);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("setting up lexer: %s",
				 isc_result_totext(result));
		return (result);
	}
	dns_rdatacallbacks_init_stdio(&callbacks);
	result = dns_rdata_fromtext(rdata, dns_rdataclass_in, *typep,
				    ctx->lexer, zname, 0, ctx->mctx, rdatabuf,
				    &callbacks);
	(void)isc_lex_close(ctx->lexer);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("parsing rdata: %s", str);
		return (result);
	}

	return (ISC_R_SUCCESS);
}

/*
 * Reads a complete dynamic update message and sends it.
 */
static isc_result_t
build_update(perf_dnsctx_t *ctx, const isc_textregion_t *record,
	     isc_buffer_t *msg)
{
	isc_textregion_t input;
	char *msgbase;
	isc_buffer_t rdlenbuf, rdatabuf;
	unsigned char rdataarray[MAX_RDATA_LENGTH];
	isc_textregion_t token;
	char *str;
	isc_boolean_t is_update;
	int updates = 0;
	int prereqs = 0;
	dns_fixedname_t fzname, foname;
	dns_name_t *zname, *oname;
	isc_uint32_t ttl;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	dns_rdata_t rdata;
	isc_uint16_t rdlen;
	isc_result_t result;

	/* Reset compression context */
	dns_compress_rollback(&ctx->compress, 0);

	input = *record;
	msgbase = isc_buffer_base(msg);

	/* Initialize */

	oname = dns_fixedname_initname(&foname);

	/* Parse zone name */
	zname = dns_fixedname_initname(&fzname);
	result = name_fromstring(zname, dns_rootname,
				 input.base, strlen(input.base),
				 NULL, "zone");
	if (result != ISC_R_SUCCESS)
		goto done;

	/* Render zone section */
	result = dns_name_towire(zname, &ctx->compress, msg);
	if (result != ISC_R_SUCCESS) {
		perf_log_warning("error rendering zone name: %s",
				 isc_result_totext(result));
		goto done;
	}
	isc_buffer_putuint16(msg, dns_rdatatype_soa);
	isc_buffer_putuint16(msg, dns_rdataclass_in);

	while (ISC_TRUE) {
		input.base += strlen(input.base) + 1;
		if (input.base >= record->base + record->length) {
			perf_log_warning("warning: incomplete update");
			goto done;
		}

		ttl = 0;
		rdtype = dns_rdatatype_any;
		isc_buffer_init(&rdatabuf, rdataarray, sizeof(rdataarray));
		dns_rdata_init(&rdata);
		rdlen = 0;
		rdclass = dns_rdataclass_in;
		is_update = ISC_FALSE;

		token.base = input.base;
		token.length = strcspn(token.base, WHITESPACE);
		str = input.base + token.length;
		if (token_equals(&token, "send")) {
			break;
		} else if (token_equals(&token, "add")) {
			result = read_update_line(ctx, &input, str, zname,
						  ISC_TRUE,
						  ISC_TRUE, ISC_TRUE, ISC_TRUE,
						  oname, &ttl, &rdtype,
						  &rdata, &rdatabuf);
			rdclass = dns_rdataclass_in;
			is_update = ISC_TRUE;
		} else if (token_equals(&token, "delete")) {
			result = read_update_line(ctx, &input, str, zname,
						  ISC_FALSE,
						  ISC_FALSE, ISC_TRUE,
						  ISC_FALSE, oname, &ttl,
						  &rdtype, &rdata, &rdatabuf);
			if (isc_buffer_usedlength(&rdatabuf) > 0)
				rdclass = dns_rdataclass_none;
			else
				rdclass = dns_rdataclass_any;
			is_update = ISC_TRUE;
		} else if (token_equals(&token, "require")) {
			result = read_update_line(ctx, &input, str, zname,
						  ISC_FALSE,
						  ISC_FALSE, ISC_TRUE,
						  ISC_FALSE, oname, &ttl,
						  &rdtype, &rdata, &rdatabuf);
			if (isc_buffer_usedlength(&rdatabuf) > 0)
				rdclass = dns_rdataclass_in;
			else
				rdclass = dns_rdataclass_any;
			is_update = ISC_FALSE;
		} else if (token_equals(&token, "prohibit")) {
			result = read_update_line(ctx, &input, str, zname,
						  ISC_FALSE,
						  ISC_FALSE, ISC_FALSE,
						  ISC_FALSE, oname, &ttl,
						  &rdtype, &rdata, &rdatabuf);
			rdclass = dns_rdataclass_none;
			is_update = ISC_FALSE;
		} else {
			perf_log_warning("invalid update command: %s",
					 input.base);
			result = ISC_R_FAILURE;
		}

		if (result != ISC_R_SUCCESS)
			goto done;

		if (!is_update && updates > 0) {
			perf_log_warning("prereqs must precede updates");
			result = ISC_R_FAILURE;
			goto done;
		}

		/* Render record */
		result = dns_name_towire(oname, &ctx->compress, msg);
		if (result != ISC_R_SUCCESS) {
			perf_log_warning("rendering record name: %s",
					 isc_result_totext(result));
			goto done;
		}
		if (isc_buffer_availablelength(msg) < 10) {
			perf_log_warning("out of space in message buffer");
			result = ISC_R_NOSPACE;
			goto done;
		}

		isc_buffer_putuint16(msg, rdtype);
		isc_buffer_putuint16(msg, rdclass);
		isc_buffer_putuint32(msg, ttl);
		rdlenbuf = *msg;
		isc_buffer_putuint16(msg, 0); /* rdlen */
		rdlen = isc_buffer_usedlength(&rdatabuf);
		if (rdlen > 0) {
			result = dns_rdata_towire(&rdata, &ctx->compress, msg);
			if (result != ISC_R_SUCCESS) {
				perf_log_warning("rendering rdata: %s",
						 isc_result_totext(result));
				goto done;
			}
			rdlen = msg->used - rdlenbuf.used - 2;
			isc_buffer_putuint16(&rdlenbuf, rdlen);
		}
		if (is_update)
			updates++;
		else
			prereqs++;
	}

	msgbase[7] = prereqs; /* ANCOUNT = number of prereqs */
	msgbase[9] = updates; /* AUCOUNT = number of updates */

	result = ISC_R_SUCCESS;

 done:
	return result;
}

isc_result_t
perf_dns_buildrequest(perf_dnsctx_t *ctx, const isc_textregion_t *record,
		      isc_uint16_t qid,
		      isc_boolean_t edns, isc_boolean_t dnssec,
		      perf_dnstsigkey_t *tsigkey, isc_buffer_t *msg)
{
	unsigned int flags;
	isc_result_t result;

	if (ctx != NULL)
		flags = dns_opcode_update << 11;
	else
		flags = DNS_MESSAGEFLAG_RD;

	/* Create the DNS packet header */
	isc_buffer_putuint16(msg, qid);
	isc_buffer_putuint16(msg, flags);	/* flags */
	isc_buffer_putuint16(msg, 1);		/* qdcount */
	isc_buffer_putuint16(msg, 0);		/* ancount */
	isc_buffer_putuint16(msg, 0);		/* aucount */
	isc_buffer_putuint16(msg, 0);		/* arcount */

	if (ctx != NULL) {
		result = build_update(ctx, record, msg);
	} else {
		result = build_query(record, msg);
	}
	if (result != ISC_R_SUCCESS)
		return (result);

	if (edns) {
		result = add_edns(msg, dnssec);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (tsigkey != NULL) {
		result = add_tsig(msg, tsigkey);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (ISC_R_SUCCESS);
}
