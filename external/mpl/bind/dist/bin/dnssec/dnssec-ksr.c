/*	$NetBSD: dnssec-ksr.c,v 1.1.1.1 2025/01/26 16:12:19 christos Exp $	*/

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
#include <stdio.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/fips.h>
#include <isc/lex.h>
#include <isc/mem.h>

#include <dns/callbacks.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keymgr.h>
#include <dns/keyvalues.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/time.h>
#include <dns/ttl.h>

#include "dnssectool.h"

const char *program = "dnssec-ksr";

/*
 * Infrastructure
 */
static isc_log_t *lctx = NULL;
static isc_mem_t *mctx = NULL;
const char *engine = NULL;
/*
 * The domain we are working on
 */
static const char *namestr = NULL;
static dns_fixedname_t fname;
static dns_name_t *name = NULL;
/*
 * KSR context
 */
struct ksr_ctx {
	const char *policy;
	const char *configfile;
	const char *file;
	const char *keydir;
	dns_keystore_t *keystore;
	isc_stdtime_t now;
	isc_stdtime_t start;
	isc_stdtime_t end;
	bool setstart;
	bool setend;
	/* keygen */
	bool ksk;
	dns_ttl_t ttl;
	dns_secalg_t alg;
	int size;
	time_t lifetime;
	time_t parentpropagation;
	time_t propagation;
	time_t publishsafety;
	time_t retiresafety;
	time_t sigrefresh;
	time_t sigvalidity;
	time_t signdelay;
	time_t ttlds;
	time_t ttlsig;
};
typedef struct ksr_ctx ksr_ctx_t;

/*
 * These are set here for backwards compatibility.
 * They are raised to 2048 in FIPS mode.
 */
static int min_rsa = 1024;
static int min_dh = 128;

#define KSR_LINESIZE   1500 /* should be long enough for any DNSKEY record */
#define DATETIME_INDEX 25

#define MAXWIRE (64 * 1024)

#define STR(t) ((t).value.as_textregion.base)

#define READLINE(lex, opt, token)

#define NEXTTOKEN(lex, opt, token)                       \
	{                                                \
		ret = isc_lex_gettoken(lex, opt, token); \
		if (ret != ISC_R_SUCCESS)                \
			goto cleanup;                    \
	}

#define BADTOKEN()                           \
	{                                    \
		ret = ISC_R_UNEXPECTEDTOKEN; \
		goto cleanup;                \
	}

#define CHECK(r)                    \
	ret = (r);                  \
	if (ret != ISC_R_SUCCESS) { \
		goto fail;          \
	}

isc_bufferlist_t cleanup_list = ISC_LIST_INITIALIZER;

static void
usage(int ret) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s options [options] <command> <zone>\n", program);
	fprintf(stderr, "\n");
	fprintf(stderr, "Version: %s\n", PACKAGE_VERSION);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -E <engine>: name of an OpenSSL engine to use\n");
	fprintf(stderr, "    -e <date/offset>: end date\n");
	fprintf(stderr, "    -F: FIPS mode\n");
	fprintf(stderr, "    -f: KSR file to sign\n");
	fprintf(stderr, "    -i <date/offset>: start date\n");
	fprintf(stderr, "    -K <directory>: key directory\n");
	fprintf(stderr, "    -k <policy>: name of a DNSSEC policy\n");
	fprintf(stderr, "    -l <file>: file with dnssec-policy config\n");
	fprintf(stderr, "    -h: print usage and exit\n");
	fprintf(stderr, "    -V: print version information\n");
	fprintf(stderr, "    -v <level>: set verbosity level\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "    keygen:  pregenerate ZSKs\n");
	fprintf(stderr, "    request: create a Key Signing Request (KSR)\n");
	fprintf(stderr, "    sign:    sign a KSR, creating a Signed Key "
			"Response (SKR)\n");
	exit(ret);
}

static isc_stdtime_t
between(isc_stdtime_t t, isc_stdtime_t start, isc_stdtime_t end) {
	isc_stdtime_t r = end;
	if (t > 0 && t > start && t < end) {
		r = t;
	}
	return r;
}

static void
checkparams(ksr_ctx_t *ksr, const char *command) {
	if (ksr->configfile == NULL) {
		fatal("%s requires a configuration file", command);
	}
	if (ksr->policy == NULL) {
		fatal("%s requires a dnssec-policy", command);
	}
	if (!ksr->setend) {
		fatal("%s requires an end date", command);
	}
	if (!ksr->setstart) {
		ksr->start = ksr->now;
	}
	if (ksr->keydir == NULL) {
		ksr->keydir = ".";
	}
}

static void
getkasp(ksr_ctx_t *ksr, dns_kasp_t **kasp) {
	cfg_parser_t *parser = NULL;
	cfg_obj_t *config = NULL;

	RUNTIME_CHECK(cfg_parser_create(mctx, lctx, &parser) == ISC_R_SUCCESS);
	if (cfg_parse_file(parser, ksr->configfile, &cfg_type_namedconf,
			   &config) != ISC_R_SUCCESS)
	{
		fatal("unable to load dnssec-policy '%s' from '%s'",
		      ksr->policy, ksr->configfile);
	}
	kasp_from_conf(config, mctx, lctx, ksr->policy, ksr->keydir, engine,
		       kasp);
	if (*kasp == NULL) {
		fatal("failed to load dnssec-policy '%s'", ksr->policy);
	}
	if (ISC_LIST_EMPTY(dns_kasp_keys(*kasp))) {
		fatal("dnssec-policy '%s' has no keys configured", ksr->policy);
	}
	cfg_obj_destroy(parser, &config);
	cfg_parser_destroy(&parser);
}

static int
keyalgtag_cmp(const void *k1, const void *k2) {
	dns_dnsseckey_t **key1 = (dns_dnsseckey_t **)k1;
	dns_dnsseckey_t **key2 = (dns_dnsseckey_t **)k2;
	if (dst_key_alg((*key1)->key) < dst_key_alg((*key2)->key)) {
		return -1;
	} else if (dst_key_alg((*key1)->key) > dst_key_alg((*key2)->key)) {
		return 1;
	} else if (dst_key_id((*key1)->key) < dst_key_id((*key2)->key)) {
		return -1;
	} else if (dst_key_id((*key1)->key) > dst_key_id((*key2)->key)) {
		return 1;
	}
	return 0;
}

static void
get_dnskeys(ksr_ctx_t *ksr, dns_dnsseckeylist_t *keys) {
	dns_dnsseckeylist_t keys_read;
	dns_dnsseckey_t **keys_sorted;
	int i = 0, n = 0;
	isc_result_t ret;

	ISC_LIST_INIT(*keys);
	ISC_LIST_INIT(keys_read);
	ret = dns_dnssec_findmatchingkeys(name, NULL, ksr->keydir, NULL,
					  ksr->now, mctx, &keys_read);
	if (ret != ISC_R_SUCCESS && ret != ISC_R_NOTFOUND) {
		fatal("failed to load existing keys from %s: %s", ksr->keydir,
		      isc_result_totext(ret));
	}
	/* Sort on keytag. */
	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(keys_read); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link))
	{
		n++;
	}
	keys_sorted = isc_mem_cget(mctx, n, sizeof(dns_dnsseckey_t *));
	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(keys_read); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link), i++)
	{
		keys_sorted[i] = dk;
	}
	qsort(keys_sorted, n, sizeof(dns_dnsseckey_t *), keyalgtag_cmp);
	while (!ISC_LIST_EMPTY(keys_read)) {
		dns_dnsseckey_t *key = ISC_LIST_HEAD(keys_read);
		ISC_LIST_UNLINK(keys_read, key, link);
	}
	/* Save sorted list in 'keys' */
	for (i = 0; i < n; i++) {
		ISC_LIST_APPEND(*keys, keys_sorted[i], link);
	}
	INSIST(ISC_LIST_EMPTY(keys_read));
	isc_mem_cput(mctx, keys_sorted, n, sizeof(dns_dnsseckey_t *));
}

static void
setcontext(ksr_ctx_t *ksr, dns_kasp_t *kasp) {
	ksr->parentpropagation = dns_kasp_parentpropagationdelay(kasp);
	ksr->propagation = dns_kasp_zonepropagationdelay(kasp);
	ksr->publishsafety = dns_kasp_publishsafety(kasp);
	ksr->retiresafety = dns_kasp_retiresafety(kasp);
	ksr->sigvalidity = dns_kasp_sigvalidity_dnskey(kasp);
	ksr->sigrefresh = dns_kasp_sigrefresh(kasp);
	ksr->signdelay = dns_kasp_signdelay(kasp);
	ksr->ttl = dns_kasp_dnskeyttl(kasp);
	ksr->ttlds = dns_kasp_dsttl(kasp);
	ksr->ttlsig = dns_kasp_zonemaxttl(kasp, true);
}

static void
cleanup(dns_dnsseckeylist_t *keys, dns_kasp_t *kasp) {
	while (!ISC_LIST_EMPTY(*keys)) {
		dns_dnsseckey_t *key = ISC_LIST_HEAD(*keys);
		ISC_LIST_UNLINK(*keys, key, link);
		dst_key_free(&key->key);
		dns_dnsseckey_destroy(mctx, &key);
	}
	dns_kasp_detach(&kasp);

	isc_buffer_t *cbuf = ISC_LIST_HEAD(cleanup_list);
	while (cbuf != NULL) {
		isc_buffer_t *nbuf = ISC_LIST_NEXT(cbuf, link);
		ISC_LIST_UNLINK(cleanup_list, cbuf, link);
		isc_buffer_free(&cbuf);
		cbuf = nbuf;
	}
}

static void
progress(int p) {
	char c = '*';
	switch (p) {
	case 0:
		c = '.';
		break;
	case 1:
		c = '+';
		break;
	case 2:
		c = '*';
		break;
	case 3:
		c = ' ';
		break;
	default:
		break;
	}
	(void)putc(c, stderr);
	(void)fflush(stderr);
}

static void
freerrset(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdlist;
	dns_rdata_t *rdata;

	if (!dns_rdataset_isassociated(rdataset)) {
		return;
	}

	dns_rdatalist_fromrdataset(rdataset, &rdlist);

	for (rdata = ISC_LIST_HEAD(rdlist->rdata); rdata != NULL;
	     rdata = ISC_LIST_HEAD(rdlist->rdata))
	{
		ISC_LIST_UNLINK(rdlist->rdata, rdata, link);
		isc_mem_put(mctx, rdata, sizeof(*rdata));
	}
	isc_mem_put(mctx, rdlist, sizeof(*rdlist));
	dns_rdataset_disassociate(rdataset);
}

static void
create_key(ksr_ctx_t *ksr, dns_kasp_t *kasp, dns_kasp_key_t *kaspkey,
	   dns_dnsseckeylist_t *keys, isc_stdtime_t inception,
	   isc_stdtime_t active, isc_stdtime_t *expiration) {
	bool conflict = false;
	bool freekey = false;
	bool show_progress = true;
	char algstr[DNS_SECALG_FORMATSIZE];
	char filename[PATH_MAX + 1];
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	dst_key_t *key = NULL;
	int options = (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC | DST_TYPE_STATE);
	isc_buffer_t buf;
	isc_result_t ret;
	isc_stdtime_t prepub;
	uint16_t flags = DNS_KEYOWNER_ZONE;

	isc_stdtime_tostring(inception, timestr, sizeof(timestr));

	/* ZSK or KSK? */
	if (ksr->ksk) {
		flags |= DNS_KEYFLAG_KSK;
	}

	/* Check algorithm and size. */
	dns_secalg_format(ksr->alg, algstr, sizeof(algstr));
	if (!dst_algorithm_supported(ksr->alg)) {
		fatal("unsupported algorithm: %s", algstr);
	}
	INSIST(ksr->size >= 0);
	switch (ksr->alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		if (isc_fips_mode()) {
			/* verify-only in FIPS mode */
			fatal("unsupported algorithm: %s", algstr);
		}
		FALLTHROUGH;
	case DST_ALG_RSASHA256:
	case DST_ALG_RSASHA512:
		if (ksr->size != 0 &&
		    (ksr->size < min_rsa || ksr->size > MAX_RSA))
		{
			fatal("RSA key size %d out of range", ksr->size);
		}
		break;
	case DST_ALG_ECDSA256:
		ksr->size = 256;
		break;
	case DST_ALG_ECDSA384:
		ksr->size = 384;
		break;
	case DST_ALG_ED25519:
		ksr->size = 256;
		break;
	case DST_ALG_ED448:
		ksr->size = 456;
		break;
	default:
		show_progress = false;
		break;
	}

	isc_buffer_init(&buf, filename, sizeof(filename) - 1);

	/* Check existing keys. */
	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(*keys); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link))
	{
		isc_stdtime_t act = 0, inact = 0;

		if (!dns_kasp_key_match(kaspkey, dk)) {
			continue;
		}
		(void)dst_key_gettime(dk->key, DST_TIME_ACTIVATE, &act);
		(void)dst_key_gettime(dk->key, DST_TIME_INACTIVE, &inact);
		/*
		 * If this key's activation time is set after the inception
		 * time, it is not eligble for the current bundle.
		 */
		if (act > inception) {
			continue;
		}
		/*
		 * If this key's inactive time is set before the inception
		 * time, it is not eligble for the current bundle.
		 */
		if (inact > 0 && inception >= inact) {
			continue;
		}

		/* Found matching existing key. */
		if (verbose > 0 && show_progress) {
			fprintf(stderr,
				"Selecting key pair for bundle %s: ", timestr);
			fflush(stderr);
		}
		key = dk->key;
		*expiration = inact;
		goto output;
	}

	/* No existing keys match. */
	do {
		conflict = false;

		if (verbose > 0 && show_progress) {
			fprintf(stderr,
				"Generating key pair for bundle %s: ", timestr);
		}
		if (ksr->keystore != NULL && ksr->policy != NULL) {
			ret = dns_keystore_keygen(
				ksr->keystore, name, ksr->policy,
				dns_rdataclass_in, mctx, ksr->alg, ksr->size,
				flags, &key);
		} else if (show_progress) {
			ret = dst_key_generate(name, ksr->alg, ksr->size, 0,
					       flags, DNS_KEYPROTO_DNSSEC,
					       dns_rdataclass_in, NULL, mctx,
					       &key, &progress);
			fflush(stderr);
		} else {
			ret = dst_key_generate(name, ksr->alg, ksr->size, 0,
					       flags, DNS_KEYPROTO_DNSSEC,
					       dns_rdataclass_in, NULL, mctx,
					       &key, NULL);
		}

		if (ret != ISC_R_SUCCESS) {
			fatal("failed to generate key %s/%s: %s\n", namestr,
			      algstr, isc_result_totext(ret));
		}

		/* Do not overwrite an existing key. */
		if (key_collision(key, name, ksr->keydir, mctx,
				  dns_kasp_key_tagmin(kaspkey),
				  dns_kasp_key_tagmax(kaspkey), NULL))
		{
			conflict = true;
			if (verbose > 0) {
				isc_buffer_clear(&buf);
				ret = dst_key_buildfilename(key, 0, ksr->keydir,
							    &buf);
				if (ret == ISC_R_SUCCESS) {
					fprintf(stderr,
						"%s: %s already exists, or "
						"might collide with another "
						"key upon revokation.  "
						"Generating a new key\n",
						program, filename);
				}
			}
			dst_key_free(&key);
		}
	} while (conflict);

	freekey = true;

	/* Set key timing metadata. */
	prepub = ksr->ttl + ksr->publishsafety + ksr->propagation;
	dst_key_setttl(key, ksr->ttl);
	dst_key_setnum(key, DST_NUM_LIFETIME, ksr->lifetime);
	dst_key_setbool(key, DST_BOOL_KSK, ksr->ksk);
	dst_key_setbool(key, DST_BOOL_ZSK, !ksr->ksk);
	dst_key_settime(key, DST_TIME_CREATED, ksr->now);
	dst_key_settime(key, DST_TIME_PUBLISH, (active - prepub));
	dst_key_settime(key, DST_TIME_ACTIVATE, active);
	if (ksr->ksk) {
		dns_keymgr_settime_syncpublish(key, kasp,
					       (inception == ksr->start));
	}

	if (ksr->lifetime > 0) {
		isc_stdtime_t inactive = (active + ksr->lifetime);
		isc_stdtime_t remove;

		if (ksr->ksk) {
			remove = ksr->ttlds + ksr->parentpropagation +
				 ksr->retiresafety;
			dst_key_settime(key, DST_TIME_SYNCDELETE, inactive);
		} else {
			remove = ksr->ttlsig + ksr->propagation +
				 ksr->retiresafety + ksr->signdelay;
		}
		dst_key_settime(key, DST_TIME_INACTIVE, inactive);
		dst_key_settime(key, DST_TIME_DELETE, (inactive + remove));
		*expiration = inactive;
	} else {
		*expiration = 0;
	}

	ret = dst_key_tofile(key, options, ksr->keydir);
	if (ret != ISC_R_SUCCESS) {
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key, keystr, sizeof(keystr));
		fatal("failed to write key %s: %s\n", keystr,
		      isc_result_totext(ret));
	}

output:
	isc_buffer_clear(&buf);
	ret = dst_key_buildfilename(key, 0, NULL, &buf);
	if (ret != ISC_R_SUCCESS) {
		fatal("dst_key_buildfilename returned: %s\n",
		      isc_result_totext(ret));
	}
	printf("%s\n", filename);
	fflush(stdout);
	if (freekey) {
		dst_key_free(&key);
	}
}

static void
print_rdata(dns_rdataset_t *rrset) {
	isc_buffer_t target;
	isc_region_t r;
	isc_result_t ret;
	char buf[4096];

	isc_buffer_init(&target, buf, sizeof(buf));
	ret = dns_rdataset_totext(rrset, name, false, false, &target);
	if (ret != ISC_R_SUCCESS) {
		fatal("failed to print rdata");
	}
	isc_buffer_usedregion(&target, &r);
	fprintf(stdout, "%.*s", (int)r.length, (char *)r.base);
}

static isc_stdtime_t
print_dnskeys(dns_kasp_key_t *kaspkey, dns_ttl_t ttl, dns_dnsseckeylist_t *keys,
	      isc_stdtime_t inception, isc_stdtime_t next_inception) {
	char algstr[DNS_SECALG_FORMATSIZE];
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataset_t rdataset = DNS_RDATASET_INIT;
	isc_result_t ret = ISC_R_SUCCESS;
	isc_stdtime_t next_bundle = next_inception;

	isc_stdtime_tostring(inception, timestr, sizeof(timestr));
	dns_secalg_format(dns_kasp_key_algorithm(kaspkey), algstr,
			  sizeof(algstr));

	/* Fetch matching key pair. */
	rdatalist = isc_mem_get(mctx, sizeof(*rdatalist));
	dns_rdatalist_init(rdatalist);
	rdatalist->rdclass = dns_rdataclass_in;
	rdatalist->type = dns_rdatatype_dnskey;
	rdatalist->ttl = ttl;
	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(*keys); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link))
	{
		isc_stdtime_t pub = 0, del = 0;

		(void)dst_key_gettime(dk->key, DST_TIME_PUBLISH, &pub);
		(void)dst_key_gettime(dk->key, DST_TIME_DELETE, &del);

		/* Determine next bundle. */
		if (pub > 0 && pub > inception && pub < next_bundle) {
			next_bundle = pub;
		}
		if (del > 0 && del > inception && del < next_bundle) {
			next_bundle = del;
		}
		/* Find matching key. */
		if (!dns_kasp_key_match(kaspkey, dk)) {
			continue;
		}
		if (pub > inception) {
			continue;
		}
		if (del != 0 && inception >= del) {
			continue;
		}
		/* Found matching key pair, add DNSKEY record to RRset. */
		isc_buffer_t buf;
		isc_buffer_t *newbuf = NULL;
		dns_rdata_t *rdata = NULL;
		isc_region_t r;
		unsigned char rdatabuf[DST_KEY_MAXSIZE];

		rdata = isc_mem_get(mctx, sizeof(*rdata));
		dns_rdata_init(rdata);
		isc_buffer_init(&buf, rdatabuf, sizeof(rdatabuf));
		CHECK(dst_key_todns(dk->key, &buf));
		isc_buffer_usedregion(&buf, &r);
		isc_buffer_allocate(mctx, &newbuf, r.length);
		isc_buffer_putmem(newbuf, r.base, r.length);
		isc_buffer_usedregion(newbuf, &r);
		dns_rdata_fromregion(rdata, dns_rdataclass_in,
				     dns_rdatatype_dnskey, &r);
		ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
		ISC_LIST_APPEND(cleanup_list, newbuf, link);
		isc_buffer_clear(newbuf);
	}
	/* Error if no key pair found. */
	if (ISC_LIST_EMPTY(rdatalist->rdata)) {
		fatal("no %s/%s zsk key pair found for bundle %s", namestr,
		      algstr, timestr);
	}

	/* All good, print DNSKEY RRset. */
	dns_rdatalist_tordataset(rdatalist, &rdataset);
	print_rdata(&rdataset);

fail:
	/* Cleanup */
	freerrset(&rdataset);

	if (ret != ISC_R_SUCCESS) {
		fatal("failed to print %s/%s zsk key pair found for bundle %s",
		      namestr, algstr, timestr);
	}

	return next_bundle;
}

static isc_stdtime_t
sign_rrset(ksr_ctx_t *ksr, isc_stdtime_t inception, isc_stdtime_t expiration,
	   dns_rdataset_t *rrset, dns_dnsseckeylist_t *keys) {
	dns_rdatalist_t *rrsiglist = NULL;
	dns_rdataset_t rrsigset = DNS_RDATASET_INIT;
	isc_result_t ret;
	isc_stdtime_t next_bundle = expiration;

	UNUSED(ksr);

	/* Bundle header */
	if (rrset->type == dns_rdatatype_dnskey) {
		char timestr[26]; /* Minimal buf as per ctime_r() spec. */
		char utc[sizeof("YYYYMMDDHHSSMM")];
		isc_buffer_t timebuf;
		isc_buffer_t b;
		isc_region_t r;
		isc_buffer_init(&timebuf, timestr, sizeof(timestr));
		isc_stdtime_tostring(inception, timestr, sizeof(timestr));
		isc_buffer_init(&b, utc, sizeof(utc));
		ret = dns_time32_totext(inception, &b);
		if (ret != ISC_R_SUCCESS) {
			fatal("failed to convert bundle time32 to text: %s",
			      isc_result_totext(ret));
		}
		isc_buffer_usedregion(&b, &r);
		fprintf(stdout, ";; SignedKeyResponse 1.0 %.*s (%s)\n",
			(int)r.length, r.base, timestr);
	}

	/* RRset */
	print_rdata(rrset);

	/* Signatures */
	rrsiglist = isc_mem_get(mctx, sizeof(*rrsiglist));
	dns_rdatalist_init(rrsiglist);
	rrsiglist->rdclass = dns_rdataclass_in;
	rrsiglist->type = dns_rdatatype_rrsig;
	rrsiglist->ttl = rrset->ttl;
	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(*keys); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link))
	{
		isc_buffer_t buf;
		isc_buffer_t *newbuf = NULL;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_t *rrsig = NULL;
		isc_region_t rs;
		unsigned char rdatabuf[SIG_FORMATSIZE];
		isc_stdtime_t clockskew = inception - 3600;

		isc_stdtime_t pub = 0, act = 0, inact = 0, del = 0;

		/* Determine next bundle. */
		(void)dst_key_gettime(dk->key, DST_TIME_PUBLISH, &pub);
		(void)dst_key_gettime(dk->key, DST_TIME_ACTIVATE, &act);
		(void)dst_key_gettime(dk->key, DST_TIME_INACTIVE, &inact);
		(void)dst_key_gettime(dk->key, DST_TIME_DELETE, &del);
		next_bundle = between(pub, inception, next_bundle);
		next_bundle = between(act, inception, next_bundle);
		next_bundle = between(inact, inception, next_bundle);
		next_bundle = between(del, inception, next_bundle);

		if (act > inception) {
			continue;
		}
		if (inact != 0 && inception >= inact) {
			continue;
		}

		rrsig = isc_mem_get(mctx, sizeof(*rrsig));
		dns_rdata_init(rrsig);
		isc_buffer_init(&buf, rdatabuf, sizeof(rdatabuf));
		ret = dns_dnssec_sign(name, rrset, dk->key, &clockskew,
				      &expiration, mctx, &buf, &rdata);
		if (ret != ISC_R_SUCCESS) {
			fatal("failed to sign KSR");
		}
		isc_buffer_usedregion(&buf, &rs);
		isc_buffer_allocate(mctx, &newbuf, rs.length);
		isc_buffer_putmem(newbuf, rs.base, rs.length);
		isc_buffer_usedregion(newbuf, &rs);
		dns_rdata_fromregion(rrsig, dns_rdataclass_in,
				     dns_rdatatype_rrsig, &rs);
		ISC_LIST_APPEND(rrsiglist->rdata, rrsig, link);
		ISC_LIST_APPEND(cleanup_list, newbuf, link);
		isc_buffer_clear(newbuf);
	}
	dns_rdatalist_tordataset(rrsiglist, &rrsigset);
	print_rdata(&rrsigset);
	freerrset(&rrsigset);

	return next_bundle;
}

/*
 * Create the DNSKEY, CDS, and CDNSKEY records beloing to the KSKs
 * listed in 'keys'.
 */
static isc_stdtime_t
get_keymaterial(ksr_ctx_t *ksr, dns_kasp_t *kasp, isc_stdtime_t inception,
		isc_stdtime_t next_inception, dns_dnsseckeylist_t *keys,
		dns_rdataset_t *dnskeyset, dns_rdataset_t *cdnskeyset,
		dns_rdataset_t *cdsset) {
	dns_kasp_digestlist_t digests = dns_kasp_digests(kasp);
	dns_rdatalist_t *dnskeylist = isc_mem_get(mctx, sizeof(*dnskeylist));
	dns_rdatalist_t *cdnskeylist = isc_mem_get(mctx, sizeof(*cdnskeylist));
	dns_rdatalist_t *cdslist = isc_mem_get(mctx, sizeof(*cdslist));
	isc_result_t ret = ISC_R_SUCCESS;
	isc_stdtime_t next_bundle = next_inception;

	dns_rdatalist_init(dnskeylist);
	dnskeylist->rdclass = dns_rdataclass_in;
	dnskeylist->type = dns_rdatatype_dnskey;
	dnskeylist->ttl = ksr->ttl;

	dns_rdatalist_init(cdnskeylist);
	cdnskeylist->rdclass = dns_rdataclass_in;
	cdnskeylist->type = dns_rdatatype_cdnskey;
	cdnskeylist->ttl = ksr->ttl;

	dns_rdatalist_init(cdslist);
	cdslist->rdclass = dns_rdataclass_in;
	cdslist->type = dns_rdatatype_cds;
	cdslist->ttl = ksr->ttl;

	for (dns_dnsseckey_t *dk = ISC_LIST_HEAD(*keys); dk != NULL;
	     dk = ISC_LIST_NEXT(dk, link))
	{
		bool published = true;
		isc_buffer_t buf;
		isc_buffer_t *newbuf;
		dns_rdata_t *rdata;
		isc_region_t r;
		isc_region_t rcds;
		isc_stdtime_t pub = 0, del = 0;
		unsigned char kskbuf[DST_KEY_MAXSIZE];
		unsigned char cdnskeybuf[DST_KEY_MAXSIZE];
		unsigned char cdsbuf[DNS_DS_BUFFERSIZE];

		/* KSK */
		(void)dst_key_gettime(dk->key, DST_TIME_PUBLISH, &pub);
		(void)dst_key_gettime(dk->key, DST_TIME_DELETE, &del);
		next_bundle = between(pub, inception, next_bundle);
		next_bundle = between(del, inception, next_bundle);

		if (pub > inception) {
			published = false;
		}
		if (del != 0 && inception >= del) {
			published = false;
		}

		if (published) {
			newbuf = NULL;
			rdata = isc_mem_get(mctx, sizeof(*rdata));
			dns_rdata_init(rdata);

			isc_buffer_init(&buf, kskbuf, sizeof(kskbuf));
			CHECK(dst_key_todns(dk->key, &buf));
			isc_buffer_usedregion(&buf, &r);
			isc_buffer_allocate(mctx, &newbuf, r.length);
			isc_buffer_putmem(newbuf, r.base, r.length);
			isc_buffer_usedregion(newbuf, &r);
			dns_rdata_fromregion(rdata, dns_rdataclass_in,
					     dns_rdatatype_dnskey, &r);
			ISC_LIST_APPEND(dnskeylist->rdata, rdata, link);
			ISC_LIST_APPEND(cleanup_list, newbuf, link);
			isc_buffer_clear(newbuf);
		}

		published = true;
		if (dns_kasp_cdnskey(kasp) || !ISC_LIST_EMPTY(digests)) {
			pub = 0;
			del = 0;
			(void)dst_key_gettime(dk->key, DST_TIME_SYNCPUBLISH,
					      &pub);
			(void)dst_key_gettime(dk->key, DST_TIME_SYNCDELETE,
					      &del);

			next_bundle = between(pub, inception, next_bundle);
			next_bundle = between(del, inception, next_bundle);

			if (pub != 0 && pub > inception) {
				published = false;
			}
			if (del != 0 && inception >= del) {
				published = false;
			}
		} else {
			published = false;
		}

		if (!published) {
			continue;
		}

		/* CDNSKEY */
		newbuf = NULL;
		rdata = isc_mem_get(mctx, sizeof(*rdata));
		dns_rdata_init(rdata);

		isc_buffer_init(&buf, cdnskeybuf, sizeof(cdnskeybuf));
		CHECK(dst_key_todns(dk->key, &buf));
		isc_buffer_usedregion(&buf, &r);
		isc_buffer_allocate(mctx, &newbuf, r.length);
		isc_buffer_putmem(newbuf, r.base, r.length);
		isc_buffer_usedregion(newbuf, &r);
		dns_rdata_fromregion(rdata, dns_rdataclass_in,
				     dns_rdatatype_cdnskey, &r);
		if (dns_kasp_cdnskey(kasp)) {
			ISC_LIST_APPEND(cdnskeylist->rdata, rdata, link);
		}
		ISC_LIST_APPEND(cleanup_list, newbuf, link);
		isc_buffer_clear(newbuf);

		/* CDS */
		for (dns_kasp_digest_t *alg = ISC_LIST_HEAD(digests);
		     alg != NULL; alg = ISC_LIST_NEXT(alg, link))
		{
			isc_buffer_t *newbuf2 = NULL;
			dns_rdata_t *rdata2 = NULL;
			dns_rdata_t cds = DNS_RDATA_INIT;

			rdata2 = isc_mem_get(mctx, sizeof(*rdata2));
			dns_rdata_init(rdata2);

			CHECK(dns_ds_buildrdata(name, rdata, alg->digest,
						cdsbuf, &cds));
			cds.type = dns_rdatatype_cds;
			dns_rdata_toregion(&cds, &rcds);
			isc_buffer_allocate(mctx, &newbuf2, rcds.length);
			isc_buffer_putmem(newbuf2, rcds.base, rcds.length);
			isc_buffer_usedregion(newbuf2, &rcds);
			dns_rdata_fromregion(rdata2, dns_rdataclass_in,
					     dns_rdatatype_cds, &rcds);
			ISC_LIST_APPEND(cdslist->rdata, rdata2, link);
			ISC_LIST_APPEND(cleanup_list, newbuf2, link);
			isc_buffer_clear(newbuf2);
		}

		if (!dns_kasp_cdnskey(kasp)) {
			isc_mem_put(mctx, rdata, sizeof(*rdata));
		}
	}
	/* All good */
	dns_rdatalist_tordataset(dnskeylist, dnskeyset);
	dns_rdatalist_tordataset(cdnskeylist, cdnskeyset);
	dns_rdatalist_tordataset(cdslist, cdsset);

	return next_bundle;

fail:
	fatal("failed to create KSK/CDS/CDNSKEY");
	return 0;
}

static void
sign_bundle(ksr_ctx_t *ksr, dns_kasp_t *kasp, isc_stdtime_t inception,
	    isc_stdtime_t next_inception, dns_rdatalist_t *zsklist,
	    dns_dnsseckeylist_t *keys) {
	isc_stdtime_t expiration = inception + ksr->sigvalidity;
	isc_stdtime_t next_bundle = next_inception;
	dns_rdataset_t zsk;

	dns_rdataset_init(&zsk);
	dns_rdatalist_tordataset(zsklist, &zsk);

	while (inception <= next_inception) {
		isc_stdtime_t next_time = next_bundle;

		/* DNSKEY RRset */
		dns_rdatalist_t *dnskeylist;
		dnskeylist = isc_mem_get(mctx, sizeof(*dnskeylist));
		dns_rdatalist_init(dnskeylist);
		dnskeylist->rdclass = dns_rdataclass_in;
		dnskeylist->type = dns_rdatatype_dnskey;
		dnskeylist->ttl = ksr->ttl;

		dns_rdataset_t ksk, cdnskey, cds, rrset;
		dns_rdataset_init(&ksk);
		dns_rdataset_init(&cdnskey);
		dns_rdataset_init(&cds);
		dns_rdataset_init(&rrset);
		next_time = get_keymaterial(ksr, kasp, inception, next_time,
					    keys, &ksk, &cdnskey, &cds);
		if (next_bundle > next_time) {
			next_bundle = next_time;
		}

		for (isc_result_t r = dns_rdatalist_first(&ksk);
		     r == ISC_R_SUCCESS; r = dns_rdatalist_next(&ksk))
		{
			dns_rdata_t *clone = isc_mem_get(mctx, sizeof(*clone));
			dns_rdata_init(clone);
			dns_rdatalist_current(&ksk, clone);
			ISC_LIST_APPEND(dnskeylist->rdata, clone, link);
		}

		for (isc_result_t r = dns_rdatalist_first(&zsk);
		     r == ISC_R_SUCCESS; r = dns_rdatalist_next(&zsk))
		{
			dns_rdata_t *clone = isc_mem_get(mctx, sizeof(*clone));
			dns_rdata_init(clone);
			dns_rdatalist_current(&zsk, clone);
			ISC_LIST_APPEND(dnskeylist->rdata, clone, link);
		}

		dns_rdatalist_tordataset(dnskeylist, &rrset);
		next_time = sign_rrset(ksr, inception, expiration, &rrset,
				       keys);
		if (next_bundle > next_time) {
			next_bundle = next_time;
		}
		freerrset(&ksk);
		freerrset(&rrset);

		/* CDNSKEY */
		if (dns_rdataset_count(&cdnskey) > 0) {
			(void)sign_rrset(ksr, inception, expiration, &cdnskey,
					 keys);
		}
		freerrset(&cdnskey);

		/* CDS */
		if (dns_rdataset_count(&cds) > 0) {
			(void)sign_rrset(ksr, inception, expiration, &cds,
					 keys);
		}
		freerrset(&cds);

		/* Next response bundle. */
		inception = expiration - ksr->sigrefresh;
		if (inception > next_bundle) {
			inception = next_bundle;
		}
		expiration = inception + ksr->sigvalidity;
		next_bundle = expiration;
	}

	freerrset(&zsk);
}

static isc_result_t
parse_dnskey(isc_lex_t *lex, char *owner, isc_buffer_t *buf, dns_ttl_t *ttl) {
	dns_fixedname_t dfname;
	dns_name_t *dname = NULL;
	dns_rdataclass_t rdclass = dns_rdataclass_in;
	isc_buffer_t b;
	isc_result_t ret;
	isc_token_t token;
	unsigned int opt = ISC_LEXOPT_EOL;

	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);

	/* Read the domain name */
	if (!strcmp(owner, "@")) {
		BADTOKEN();
	}

	dname = dns_fixedname_initname(&dfname);
	isc_buffer_init(&b, owner, strlen(owner));
	isc_buffer_add(&b, strlen(owner));
	ret = dns_name_fromtext(dname, &b, dns_rootname, 0, NULL);
	if (ret != ISC_R_SUCCESS) {
		return ret;
	}
	if (dns_name_compare(dname, name) != 0) {
		return DNS_R_BADOWNERNAME;
	}
	isc_buffer_clear(&b);

	/* Read the next word: either TTL, class, or type */
	NEXTTOKEN(lex, opt, &token);
	if (token.type != isc_tokentype_string) {
		BADTOKEN();
	}

	/* If it's a TTL, read the next one */
	ret = dns_ttl_fromtext(&token.value.as_textregion, ttl);
	if (ret == ISC_R_SUCCESS) {
		NEXTTOKEN(lex, opt, &token);
	}
	if (token.type != isc_tokentype_string) {
		BADTOKEN();
	}

	/* If it's a class, read the next one */
	ret = dns_rdataclass_fromtext(&rdclass, &token.value.as_textregion);
	if (ret == ISC_R_SUCCESS) {
		NEXTTOKEN(lex, opt, &token);
	}
	if (token.type != isc_tokentype_string) {
		BADTOKEN();
	}

	/* Must be the type */
	if (strcasecmp(STR(token), "DNSKEY") != 0) {
		BADTOKEN();
	}

	ret = dns_rdata_fromtext(NULL, rdclass, dns_rdatatype_dnskey, lex, name,
				 0, mctx, buf, NULL);

cleanup:
	isc_lex_setcomments(lex, 0);
	return ret;
}

static void
keygen(ksr_ctx_t *ksr) {
	dns_kasp_t *kasp = NULL;
	dns_dnsseckeylist_t keys;
	bool noop = true;

	/* Check parameters */
	checkparams(ksr, "keygen");
	/* Get the policy */
	getkasp(ksr, &kasp);
	/* Get existing keys */
	get_dnskeys(ksr, &keys);
	/* Set context */
	setcontext(ksr, kasp);
	/* Key generation */
	for (dns_kasp_key_t *kk = ISC_LIST_HEAD(dns_kasp_keys(kasp));
	     kk != NULL; kk = ISC_LIST_NEXT(kk, link))
	{
		if (dns_kasp_key_ksk(kk) && !ksr->ksk) {
			/* only ZSKs allowed */
			continue;
		} else if (dns_kasp_key_zsk(kk) && ksr->ksk) {
			/* only KSKs allowed */
			continue;
		}
		ksr->alg = dns_kasp_key_algorithm(kk);
		ksr->lifetime = dns_kasp_key_lifetime(kk);
		ksr->keystore = dns_kasp_key_keystore(kk);
		ksr->size = dns_kasp_key_size(kk);
		noop = false;

		for (isc_stdtime_t inception = ksr->start, act = ksr->start;
		     inception < ksr->end; inception += ksr->lifetime)
		{
			create_key(ksr, kasp, kk, &keys, inception, act, &act);
			if (ksr->lifetime == 0) {
				/* unlimited lifetime, but not infinite loop */
				break;
			}
		}
	}
	if (noop) {
		fatal("no keys created for policy '%s'", ksr->policy);
	}
	/* Cleanup */
	cleanup(&keys, kasp);
}

static void
request(ksr_ctx_t *ksr) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	dns_dnsseckeylist_t keys;
	dns_kasp_t *kasp = NULL;
	isc_stdtime_t next = 0;
	isc_stdtime_t inception = 0;

	/* Check parameters */
	checkparams(ksr, "request");
	/* Get the policy */
	getkasp(ksr, &kasp);
	/* Get keys */
	get_dnskeys(ksr, &keys);
	/* Set context */
	setcontext(ksr, kasp);
	/* Create request */
	inception = ksr->start;
	while (inception <= ksr->end) {
		char utc[sizeof("YYYYMMDDHHSSMM")];
		isc_buffer_t b;
		isc_region_t r;
		isc_result_t ret;

		isc_stdtime_tostring(inception, timestr, sizeof(timestr));
		isc_buffer_init(&b, utc, sizeof(utc));
		ret = dns_time32_totext(inception, &b);
		if (ret != ISC_R_SUCCESS) {
			fatal("failed to convert bundle time32 to text: %s",
			      isc_result_totext(ret));
		}
		isc_buffer_usedregion(&b, &r);
		fprintf(stdout, ";; KeySigningRequest 1.0 %.*s (%s)\n",
			(int)r.length, r.base, timestr);

		next = ksr->end + 1;
		for (dns_kasp_key_t *kk = ISC_LIST_HEAD(dns_kasp_keys(kasp));
		     kk != NULL; kk = ISC_LIST_NEXT(kk, link))
		{
			/*
			 * Output the DNSKEY records for the current bundle
			 * that starts at 'inception. The 'next' variable is
			 * updated to the start time of the
			 * next bundle, determined by the earliest publication
			 * or withdrawal of a key that is after the current
			 * inception.
			 */
			if (dns_kasp_key_ksk(kk)) {
				/* We only want ZSKs in the request. */
				continue;
			}

			next = print_dnskeys(kk, ksr->ttl, &keys, inception,
					     next);
		}
		inception = next;
	}

	isc_stdtime_tostring(ksr->now, timestr, sizeof(timestr));
	fprintf(stdout, ";; KeySigningRequest 1.0 generated at %s by %s\n",
		timestr, PACKAGE_VERSION);

	/* Cleanup */
	cleanup(&keys, kasp);
}

static void
sign(ksr_ctx_t *ksr) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	bool have_bundle = false;
	dns_dnsseckeylist_t keys;
	dns_kasp_t *kasp = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	isc_result_t ret;
	isc_stdtime_t inception;
	isc_lex_t *lex = NULL;
	isc_lexspecials_t specials;
	isc_token_t token;
	unsigned int opt = ISC_LEXOPT_EOL;

	/* Check parameters */
	checkparams(ksr, "sign");
	if (ksr->file == NULL) {
		fatal("'sign' requires a KSR file");
	}
	/* Get the policy */
	getkasp(ksr, &kasp);
	/* Get keys */
	get_dnskeys(ksr, &keys);
	/* Set context */
	setcontext(ksr, kasp);
	/* Sign request */
	inception = ksr->start;
	isc_lex_create(mctx, KSR_LINESIZE, &lex);
	memset(specials, 0, sizeof(specials));
	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);
	ret = isc_lex_openfile(lex, ksr->file);
	if (ret != ISC_R_SUCCESS) {
		fatal("unable to open KSR file %s: %s", ksr->file,
		      isc_result_totext(ret));
	}

	for (ret = isc_lex_gettoken(lex, opt, &token); ret == ISC_R_SUCCESS;
	     ret = isc_lex_gettoken(lex, opt, &token))
	{
		if (token.type != isc_tokentype_string) {
			fatal("bad KSR file %s(%lu): syntax error", ksr->file,
			      isc_lex_getsourceline(lex));
		}

		if (strcmp(STR(token), ";;") == 0) {
			char bundle[KSR_LINESIZE];
			isc_stdtime_t next_inception;

			CHECK(isc_lex_gettoken(lex, opt, &token));
			if (token.type != isc_tokentype_string ||
			    strcmp(STR(token), "KeySigningRequest") != 0)
			{
				fatal("bad KSR file %s(%lu): expected "
				      "'KeySigningRequest'",
				      ksr->file, isc_lex_getsourceline(lex));
			}

			CHECK(isc_lex_gettoken(lex, opt, &token));
			if (token.type != isc_tokentype_string) {
				fatal("bad KSR file %s(%lu): expected string",
				      ksr->file, isc_lex_getsourceline(lex));
			}

			if (strcmp(STR(token), "1.0") != 0) {
				fatal("bad KSR file %s(%lu): expected version",
				      ksr->file, isc_lex_getsourceline(lex));
			}

			CHECK(isc_lex_gettoken(lex, opt, &token));
			if (token.type != isc_tokentype_string) {
				fatal("bad KSR file %s(%lu): expected datetime",
				      ksr->file, isc_lex_getsourceline(lex));
			}
			if (strcmp(STR(token), "generated") == 0) {
				/* Final bundle */
				goto readline;
			}

			/* Date and time of bundle */
			sscanf(STR(token), "%s", bundle);
			next_inception = strtotime(bundle, ksr->now, ksr->now,
						   NULL);

			if (have_bundle) {
				/* Sign previous bundle */
				sign_bundle(ksr, kasp, inception,
					    next_inception, rdatalist, &keys);
				fprintf(stdout, "\n");
			}

			/* Start next bundle */
			rdatalist = isc_mem_get(mctx, sizeof(*rdatalist));
			dns_rdatalist_init(rdatalist);
			rdatalist->rdclass = dns_rdataclass_in;
			rdatalist->type = dns_rdatatype_dnskey;
			rdatalist->ttl = ksr->ttl;

			inception = next_inception;
			have_bundle = true;

		readline:
			/* Read remainder of header line */
			do {
				ret = isc_lex_gettoken(lex, opt, &token);
				if (ret != ISC_R_SUCCESS) {
					fatal("bad KSR file %s(%lu): bad "
					      "header (%s)",
					      ksr->file,
					      isc_lex_getsourceline(lex),
					      isc_result_totext(ret));
				}
			} while (token.type != isc_tokentype_eol);
		} else {
			/* Parse DNSKEY */
			dns_ttl_t ttl = ksr->ttl;
			isc_buffer_t buf;
			isc_buffer_t *newbuf = NULL;
			dns_rdata_t *rdata = NULL;
			isc_region_t r;
			u_char rdatabuf[DST_KEY_MAXSIZE];

			INSIST(rdatalist != NULL);

			rdata = isc_mem_get(mctx, sizeof(*rdata));
			dns_rdata_init(rdata);
			isc_buffer_init(&buf, rdatabuf, sizeof(rdatabuf));
			ret = parse_dnskey(lex, STR(token), &buf, &ttl);
			if (ret != ISC_R_SUCCESS) {
				fatal("bad KSR file %s(%lu): bad DNSKEY (%s)",
				      ksr->file, isc_lex_getsourceline(lex),
				      isc_result_totext(ret));
			}
			isc_buffer_usedregion(&buf, &r);
			isc_buffer_allocate(mctx, &newbuf, r.length);
			isc_buffer_putmem(newbuf, r.base, r.length);
			isc_buffer_usedregion(newbuf, &r);
			dns_rdata_fromregion(rdata, dns_rdataclass_in,
					     dns_rdatatype_dnskey, &r);
			if (rdatalist != NULL && ttl < rdatalist->ttl) {
				rdatalist->ttl = ttl;
			}

			ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
			ISC_LIST_APPEND(cleanup_list, newbuf, link);
			isc_buffer_clear(newbuf);
		}
	}

	if (ret != ISC_R_EOF) {
		fatal("bad KSR file %s(%lu): trailing garbage data", ksr->file,
		      isc_lex_getsourceline(lex));
	}

	/* Final bundle */
	if (have_bundle && rdatalist != NULL) {
		sign_bundle(ksr, kasp, inception, ksr->end, rdatalist, &keys);
	} else {
		fatal("bad KSR file %s(%lu): no bundles", ksr->file,
		      isc_lex_getsourceline(lex));
	}

	/* Bundle footer */
	isc_stdtime_tostring(ksr->now, timestr, sizeof(timestr));
	fprintf(stdout, ";; SignedKeyResponse 1.0 generated at %s by %s\n",
		timestr, PACKAGE_VERSION);

fail:
	isc_lex_destroy(&lex);
	cleanup(&keys, kasp);
}

int
main(int argc, char *argv[]) {
	isc_result_t ret;
	isc_buffer_t buf;
	int ch;
	char *endp;
	bool set_fips_mode = false;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
	OSSL_PROVIDER *fips = NULL, *base = NULL;
#endif
	ksr_ctx_t ksr = {
		.now = isc_stdtime_now(),
	};

	isc_mem_create(&mctx);

	isc_commandline_errprint = false;

#define OPTIONS "E:e:Ff:hi:K:k:l:ov:V"
	while ((ch = isc_commandline_parse(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'E':
			engine = isc_commandline_argument;
			break;
		case 'e':
			ksr.end = strtotime(isc_commandline_argument, ksr.now,
					    ksr.now, &ksr.setend);
			break;
		case 'F':
			set_fips_mode = true;
			break;
		case 'f':
			ksr.file = isc_commandline_argument;
			break;
		case 'h':
			usage(0);
			break;
		case 'i':
			ksr.start = strtotime(isc_commandline_argument, ksr.now,
					      ksr.now, &ksr.setstart);
			break;
		case 'K':
			ksr.keydir = isc_commandline_argument;
			ret = try_dir(ksr.keydir);
			if (ret != ISC_R_SUCCESS) {
				fatal("cannot open directory %s: %s",
				      ksr.keydir, isc_result_totext(ret));
			}
			break;
		case 'k':
			ksr.policy = isc_commandline_argument;
			break;
		case 'l':
			ksr.configfile = isc_commandline_argument;
			break;
		case 'o':
			ksr.ksk = true;
			break;
		case 'V':
			version(program);
			break;
		case 'v':
			verbose = strtoul(isc_commandline_argument, &endp, 0);
			if (*endp != '\0') {
				fatal("-v must be followed by a number");
			}
			break;
		default:
			usage(1);
			break;
		}
	}
	argv += isc_commandline_index;
	argc -= isc_commandline_index;

	if (argc != 2) {
		fatal("must provide a command and zone name");
	}

	ret = dst_lib_init(mctx, engine);
	if (ret != ISC_R_SUCCESS) {
		fatal("could not initialize dst: %s", isc_result_totext(ret));
	}

	/*
	 * After dst_lib_init which will set FIPS mode if requested
	 * at build time.  The minumums are both raised to 2048.
	 */
	if (isc_fips_mode()) {
		min_rsa = min_dh = 2048;
	}

	setup_logging(mctx, &lctx);

	if (set_fips_mode) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
		fips = OSSL_PROVIDER_load(NULL, "fips");
		if (fips == NULL) {
			fatal("Failed to load FIPS provider");
		}
		base = OSSL_PROVIDER_load(NULL, "base");
		if (base == NULL) {
			OSSL_PROVIDER_unload(fips);
			fatal("Failed to load base provider");
		}
#endif
		if (!isc_fips_mode()) {
			if (isc_fips_set_mode(1) != ISC_R_SUCCESS) {
				fatal("setting FIPS mode failed");
			}
		}
	}

	/* zone */
	namestr = argv[1];
	name = dns_fixedname_initname(&fname);
	isc_buffer_init(&buf, argv[1], strlen(argv[1]));
	isc_buffer_add(&buf, strlen(argv[1]));
	ret = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
	if (ret != ISC_R_SUCCESS) {
		fatal("invalid zone name %s: %s", argv[1],
		      isc_result_totext(ret));
	}

	/* command */
	if (strcmp(argv[0], "keygen") == 0) {
		keygen(&ksr);
	} else if (strcmp(argv[0], "request") == 0) {
		request(&ksr);
	} else if (strcmp(argv[0], "sign") == 0) {
		sign(&ksr);
	} else {
		fatal("unknown command '%s'", argv[0]);
	}

	exit(0);
}
