/*	$NetBSD: dnssec-keygen.c,v 1.10 2023/01/25 21:43:23 christos Exp $	*/

/*
 * Portions Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/kasp.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/result.h>
#include <dns/secalg.h>

#include <dst/dst.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/kaspconf.h>
#include <isccfg/namedconf.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif /* if USE_PKCS11 */

#include "dnssectool.h"

#define MAX_RSA 4096 /* should be long enough... */

const char *program = "dnssec-keygen";

isc_log_t *lctx = NULL;

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
progress(int p);

struct keygen_ctx {
	const char *predecessor;
	const char *policy;
	const char *configfile;
	const char *directory;
	char *algname;
	char *nametype;
	char *type;
	int generator;
	int protocol;
	int size;
	int signatory;
	dns_rdataclass_t rdclass;
	int options;
	int dbits;
	dns_ttl_t ttl;
	uint16_t kskflag;
	uint16_t revflag;
	dns_secalg_t alg;
	/* timing data */
	int prepub;
	isc_stdtime_t now;
	isc_stdtime_t publish;
	isc_stdtime_t activate;
	isc_stdtime_t inactive;
	isc_stdtime_t revokekey;
	isc_stdtime_t deltime;
	isc_stdtime_t syncadd;
	isc_stdtime_t syncdel;
	bool setpub;
	bool setact;
	bool setinact;
	bool setrev;
	bool setdel;
	bool setsyncadd;
	bool setsyncdel;
	bool unsetpub;
	bool unsetact;
	bool unsetinact;
	bool unsetrev;
	bool unsetdel;
	/* how to generate the key */
	bool setttl;
	bool use_nsec3;
	bool genonly;
	bool showprogress;
	bool quiet;
	bool oldstyle;
	/* state */
	time_t lifetime;
	bool ksk;
	bool zsk;
};

typedef struct keygen_ctx keygen_ctx_t;

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s [options] name\n\n", program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "    name: owner of the key\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -K <directory>: write keys into directory\n");
	fprintf(stderr, "    -k <policy>: generate keys for dnssec-policy\n");
	fprintf(stderr, "    -l <file>: configuration file with dnssec-policy "
			"statement\n");
	fprintf(stderr, "    -a <algorithm>:\n");
	fprintf(stderr, "        RSASHA1 | NSEC3RSASHA1 |\n");
	fprintf(stderr, "        RSASHA256 | RSASHA512 |\n");
	fprintf(stderr, "        ECDSAP256SHA256 | ECDSAP384SHA384 |\n");
	fprintf(stderr, "        ED25519 | ED448 | DH\n");
	fprintf(stderr, "    -3: use NSEC3-capable algorithm\n");
	fprintf(stderr, "    -b <key size in bits>:\n");
	fprintf(stderr, "        RSASHA1:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        NSEC3RSASHA1:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        RSASHA256:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        RSASHA512:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        DH:\t\t[128..4096]\n");
	fprintf(stderr, "        ECDSAP256SHA256:\tignored\n");
	fprintf(stderr, "        ECDSAP384SHA384:\tignored\n");
	fprintf(stderr, "        ED25519:\tignored\n");
	fprintf(stderr, "        ED448:\tignored\n");
	fprintf(stderr, "        (key size defaults are set according to\n"
			"        algorithm and usage (ZSK or KSK)\n");
	fprintf(stderr, "    -n <nametype>: ZONE | HOST | ENTITY | "
			"USER | OTHER\n");
	fprintf(stderr, "        (DNSKEY generation defaults to ZONE)\n");
	fprintf(stderr, "    -c <class>: (default: IN)\n");
	fprintf(stderr, "    -d <digest bits> (0 => max, default)\n");
	fprintf(stderr, "    -E <engine>:\n");
#if USE_PKCS11
	fprintf(stderr,
		"        path to PKCS#11 provider library "
		"(default is %s)\n",
		PK11_LIB_LOCATION);
#else  /* if USE_PKCS11 */
	fprintf(stderr, "        name of an OpenSSL engine to use\n");
#endif /* if USE_PKCS11 */
	fprintf(stderr, "    -f <keyflag>: KSK | REVOKE\n");
	fprintf(stderr, "    -g <generator>: use specified generator "
			"(DH only)\n");
	fprintf(stderr, "    -L <ttl>: default key TTL\n");
	fprintf(stderr, "    -p <protocol>: (default: 3 [dnssec])\n");
	fprintf(stderr, "    -s <strength>: strength value this key signs DNS "
			"records with (default: 0)\n");
	fprintf(stderr, "    -T <rrtype>: DNSKEY | KEY (default: DNSKEY; "
			"use KEY for SIG(0))\n");
	fprintf(stderr, "    -t <type>: "
			"AUTHCONF | NOAUTHCONF | NOAUTH | NOCONF "
			"(default: AUTHCONF)\n");
	fprintf(stderr, "    -h: print usage and exit\n");
	fprintf(stderr, "    -m <memory debugging mode>:\n");
	fprintf(stderr, "       usage | trace | record | size | mctx\n");
	fprintf(stderr, "    -v <level>: set verbosity level (0 - 10)\n");
	fprintf(stderr, "    -V: print version information\n");
	fprintf(stderr, "Timing options:\n");
	fprintf(stderr, "    -P date/[+-]offset/none: set key publication date "
			"(default: now)\n");
	fprintf(stderr, "    -P sync date/[+-]offset/none: set CDS and CDNSKEY "
			"publication date\n");
	fprintf(stderr, "    -A date/[+-]offset/none: set key activation date "
			"(default: now)\n");
	fprintf(stderr, "    -R date/[+-]offset/none: set key "
			"revocation date\n");
	fprintf(stderr, "    -I date/[+-]offset/none: set key "
			"inactivation date\n");
	fprintf(stderr, "    -D date/[+-]offset/none: set key deletion date\n");
	fprintf(stderr, "    -D sync date/[+-]offset/none: set CDS and CDNSKEY "
			"deletion date\n");

	fprintf(stderr, "    -G: generate key only; do not set -P or -A\n");
	fprintf(stderr, "    -C: generate a backward-compatible key, omitting "
			"all dates\n");
	fprintf(stderr, "    -S <key>: generate a successor to an existing "
			"key\n");
	fprintf(stderr, "    -i <interval>: prepublication interval for "
			"successor key "
			"(default: 30 days)\n");
	fprintf(stderr, "Output:\n");
	fprintf(stderr, "     K<name>+<alg>+<id>.key, "
			"K<name>+<alg>+<id>.private\n");

	exit(-1);
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
kasp_from_conf(cfg_obj_t *config, isc_mem_t *mctx, const char *name,
	       dns_kasp_t **kaspp) {
	const cfg_listelt_t *element;
	const cfg_obj_t *kasps = NULL;
	dns_kasp_t *kasp = NULL, *kasp_next;
	isc_result_t result = ISC_R_NOTFOUND;
	dns_kasplist_t kasplist;

	ISC_LIST_INIT(kasplist);

	(void)cfg_map_get(config, "dnssec-policy", &kasps);
	for (element = cfg_list_first(kasps); element != NULL;
	     element = cfg_list_next(element))
	{
		cfg_obj_t *kconfig = cfg_listelt_value(element);
		kasp = NULL;
		if (strcmp(cfg_obj_asstring(cfg_tuple_get(kconfig, "name")),
			   name) != 0)
		{
			continue;
		}

		result = cfg_kasp_fromconfig(kconfig, NULL, mctx, lctx,
					     &kasplist, &kasp);
		if (result != ISC_R_SUCCESS) {
			fatal("failed to configure dnssec-policy '%s': %s",
			      cfg_obj_asstring(cfg_tuple_get(kconfig, "name")),
			      isc_result_totext(result));
		}
		INSIST(kasp != NULL);
		dns_kasp_freeze(kasp);
		break;
	}

	*kaspp = kasp;

	/*
	 * Cleanup kasp list.
	 */
	for (kasp = ISC_LIST_HEAD(kasplist); kasp != NULL; kasp = kasp_next) {
		kasp_next = ISC_LIST_NEXT(kasp, link);
		ISC_LIST_UNLINK(kasplist, kasp, link);
		dns_kasp_detach(&kasp);
	}
}

static void
keygen(keygen_ctx_t *ctx, isc_mem_t *mctx, int argc, char **argv) {
	char filename[255];
	char algstr[DNS_SECALG_FORMATSIZE];
	uint16_t flags = 0;
	int param = 0;
	bool null_key = false;
	bool conflict = false;
	bool show_progress = false;
	isc_buffer_t buf;
	dns_name_t *name;
	dns_fixedname_t fname;
	isc_result_t ret;
	dst_key_t *key = NULL;
	dst_key_t *prevkey = NULL;

	UNUSED(argc);

	dns_secalg_format(ctx->alg, algstr, sizeof(algstr));

	if (ctx->predecessor == NULL) {
		if (ctx->prepub == -1) {
			ctx->prepub = 0;
		}

		name = dns_fixedname_initname(&fname);
		isc_buffer_init(&buf, argv[isc_commandline_index],
				strlen(argv[isc_commandline_index]));
		isc_buffer_add(&buf, strlen(argv[isc_commandline_index]));
		ret = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
		if (ret != ISC_R_SUCCESS) {
			fatal("invalid key name %s: %s",
			      argv[isc_commandline_index],
			      isc_result_totext(ret));
		}

		if (!dst_algorithm_supported(ctx->alg)) {
			fatal("unsupported algorithm: %s", algstr);
		}

		if (ctx->alg == DST_ALG_DH) {
			ctx->options |= DST_TYPE_KEY;
		}

		if (ctx->use_nsec3) {
			switch (ctx->alg) {
			case DST_ALG_RSASHA1:
				ctx->alg = DST_ALG_NSEC3RSASHA1;
				break;
			case DST_ALG_NSEC3RSASHA1:
			case DST_ALG_RSASHA256:
			case DST_ALG_RSASHA512:
			case DST_ALG_ECDSA256:
			case DST_ALG_ECDSA384:
			case DST_ALG_ED25519:
			case DST_ALG_ED448:
				break;
			default:
				fatal("algorithm %s is incompatible with NSEC3"
				      ", do not use the -3 option",
				      algstr);
			}
		}

		if (ctx->type != NULL && (ctx->options & DST_TYPE_KEY) != 0) {
			if (strcasecmp(ctx->type, "NOAUTH") == 0) {
				flags |= DNS_KEYTYPE_NOAUTH;
			} else if (strcasecmp(ctx->type, "NOCONF") == 0) {
				flags |= DNS_KEYTYPE_NOCONF;
			} else if (strcasecmp(ctx->type, "NOAUTHCONF") == 0) {
				flags |= (DNS_KEYTYPE_NOAUTH |
					  DNS_KEYTYPE_NOCONF);
				if (ctx->size < 0) {
					ctx->size = 0;
				}
			} else if (strcasecmp(ctx->type, "AUTHCONF") == 0) {
				/* nothing */
			} else {
				fatal("invalid type %s", ctx->type);
			}
		}

		if (ctx->size < 0) {
			switch (ctx->alg) {
			case DST_ALG_RSASHA1:
			case DST_ALG_NSEC3RSASHA1:
			case DST_ALG_RSASHA256:
			case DST_ALG_RSASHA512:
				ctx->size = 2048;
				if (verbose > 0) {
					fprintf(stderr,
						"key size not "
						"specified; defaulting"
						" to %d\n",
						ctx->size);
				}
				break;
			case DST_ALG_ECDSA256:
			case DST_ALG_ECDSA384:
			case DST_ALG_ED25519:
			case DST_ALG_ED448:
				break;
			default:
				fatal("key size not specified (-b option)");
			}
		}

		if (!ctx->oldstyle && ctx->prepub > 0) {
			if (ctx->setpub && ctx->setact &&
			    (ctx->activate - ctx->prepub) < ctx->publish)
			{
				fatal("Activation and publication dates "
				      "are closer together than the\n\t"
				      "prepublication interval.");
			}

			if (!ctx->setpub && !ctx->setact) {
				ctx->setpub = ctx->setact = true;
				ctx->publish = ctx->now;
				ctx->activate = ctx->now + ctx->prepub;
			} else if (ctx->setpub && !ctx->setact) {
				ctx->setact = true;
				ctx->activate = ctx->publish + ctx->prepub;
			} else if (ctx->setact && !ctx->setpub) {
				ctx->setpub = true;
				ctx->publish = ctx->activate - ctx->prepub;
			}

			if ((ctx->activate - ctx->prepub) < ctx->now) {
				fatal("Time until activation is shorter "
				      "than the\n\tprepublication interval.");
			}
		}
	} else {
		char keystr[DST_KEY_FORMATSIZE];
		isc_stdtime_t when;
		int major, minor;

		if (ctx->prepub == -1) {
			ctx->prepub = (30 * 86400);
		}

		if (ctx->alg != 0) {
			fatal("-S and -a cannot be used together");
		}
		if (ctx->size >= 0) {
			fatal("-S and -b cannot be used together");
		}
		if (ctx->nametype != NULL) {
			fatal("-S and -n cannot be used together");
		}
		if (ctx->type != NULL) {
			fatal("-S and -t cannot be used together");
		}
		if (ctx->setpub || ctx->unsetpub) {
			fatal("-S and -P cannot be used together");
		}
		if (ctx->setact || ctx->unsetact) {
			fatal("-S and -A cannot be used together");
		}
		if (ctx->use_nsec3) {
			fatal("-S and -3 cannot be used together");
		}
		if (ctx->oldstyle) {
			fatal("-S and -C cannot be used together");
		}
		if (ctx->genonly) {
			fatal("-S and -G cannot be used together");
		}

		ret = dst_key_fromnamedfile(
			ctx->predecessor, ctx->directory,
			(DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_STATE),
			mctx, &prevkey);
		if (ret != ISC_R_SUCCESS) {
			fatal("Invalid keyfile %s: %s", ctx->predecessor,
			      isc_result_totext(ret));
		}
		if (!dst_key_isprivate(prevkey)) {
			fatal("%s is not a private key", ctx->predecessor);
		}

		name = dst_key_name(prevkey);
		ctx->alg = dst_key_alg(prevkey);
		ctx->size = dst_key_size(prevkey);
		flags = dst_key_flags(prevkey);

		dst_key_format(prevkey, keystr, sizeof(keystr));
		dst_key_getprivateformat(prevkey, &major, &minor);
		if (major != DST_MAJOR_VERSION || minor < DST_MINOR_VERSION) {
			fatal("Key %s has incompatible format version %d.%d\n\t"
			      "It is not possible to generate a successor key.",
			      keystr, major, minor);
		}

		ret = dst_key_gettime(prevkey, DST_TIME_ACTIVATE, &when);
		if (ret != ISC_R_SUCCESS) {
			fatal("Key %s has no activation date.\n\t"
			      "You must use dnssec-settime -A to set one "
			      "before generating a successor.",
			      keystr);
		}

		ret = dst_key_gettime(prevkey, DST_TIME_INACTIVE,
				      &ctx->activate);
		if (ret != ISC_R_SUCCESS) {
			fatal("Key %s has no inactivation date.\n\t"
			      "You must use dnssec-settime -I to set one "
			      "before generating a successor.",
			      keystr);
		}

		ctx->publish = ctx->activate - ctx->prepub;
		if (ctx->publish < ctx->now) {
			fatal("Key %s becomes inactive\n\t"
			      "sooner than the prepublication period "
			      "for the new key ends.\n\t"
			      "Either change the inactivation date with "
			      "dnssec-settime -I,\n\t"
			      "or use the -i option to set a shorter "
			      "prepublication interval.",
			      keystr);
		}

		ret = dst_key_gettime(prevkey, DST_TIME_DELETE, &when);
		if (ret != ISC_R_SUCCESS) {
			fprintf(stderr,
				"%s: WARNING: Key %s has no removal "
				"date;\n\t it will remain in the zone "
				"indefinitely after rollover.\n\t "
				"You can use dnssec-settime -D to "
				"change this.\n",
				program, keystr);
		}

		ctx->setpub = ctx->setact = true;
	}

	switch (ctx->alg) {
	case DNS_KEYALG_RSASHA1:
	case DNS_KEYALG_NSEC3RSASHA1:
	case DNS_KEYALG_RSASHA256:
		if (ctx->size != 0 && (ctx->size < 1024 || ctx->size > MAX_RSA))
		{
			fatal("RSA key size %d out of range", ctx->size);
		}
		break;
	case DNS_KEYALG_RSASHA512:
		if (ctx->size != 0 && (ctx->size < 1024 || ctx->size > MAX_RSA))
		{
			fatal("RSA key size %d out of range", ctx->size);
		}
		break;
	case DNS_KEYALG_DH:
		if (ctx->size != 0 && (ctx->size < 128 || ctx->size > 4096)) {
			fatal("DH key size %d out of range", ctx->size);
		}
		break;
	case DST_ALG_ECDSA256:
		ctx->size = 256;
		break;
	case DST_ALG_ECDSA384:
		ctx->size = 384;
		break;
	case DST_ALG_ED25519:
		ctx->size = 256;
		break;
	case DST_ALG_ED448:
		ctx->size = 456;
		break;
	}

	if (ctx->alg != DNS_KEYALG_DH && ctx->generator != 0) {
		fatal("specified DH generator for a non-DH key");
	}

	if (ctx->nametype == NULL) {
		if ((ctx->options & DST_TYPE_KEY) != 0) { /* KEY */
			fatal("no nametype specified");
		}
		flags |= DNS_KEYOWNER_ZONE; /* DNSKEY */
	} else if (strcasecmp(ctx->nametype, "zone") == 0) {
		flags |= DNS_KEYOWNER_ZONE;
	} else if ((ctx->options & DST_TYPE_KEY) != 0) { /* KEY */
		if (strcasecmp(ctx->nametype, "host") == 0 ||
		    strcasecmp(ctx->nametype, "entity") == 0)
		{
			flags |= DNS_KEYOWNER_ENTITY;
		} else if (strcasecmp(ctx->nametype, "user") == 0) {
			flags |= DNS_KEYOWNER_USER;
		} else {
			fatal("invalid KEY nametype %s", ctx->nametype);
		}
	} else if (strcasecmp(ctx->nametype, "other") != 0) { /* DNSKEY */
		fatal("invalid DNSKEY nametype %s", ctx->nametype);
	}

	if (ctx->directory == NULL) {
		ctx->directory = ".";
	}

	if ((ctx->options & DST_TYPE_KEY) != 0) { /* KEY */
		flags |= ctx->signatory;
	} else if ((flags & DNS_KEYOWNER_ZONE) != 0) { /* DNSKEY */
		flags |= ctx->kskflag;
		flags |= ctx->revflag;
	}

	if (ctx->protocol == -1) {
		ctx->protocol = DNS_KEYPROTO_DNSSEC;
	} else if ((ctx->options & DST_TYPE_KEY) == 0 &&
		   ctx->protocol != DNS_KEYPROTO_DNSSEC)
	{
		fatal("invalid DNSKEY protocol: %d", ctx->protocol);
	}

	if ((flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY) {
		if (ctx->size > 0) {
			fatal("specified null key with non-zero size");
		}
		if ((flags & DNS_KEYFLAG_SIGNATORYMASK) != 0) {
			fatal("specified null key with signing authority");
		}
	}

	if ((flags & DNS_KEYFLAG_OWNERMASK) == DNS_KEYOWNER_ZONE &&
	    ctx->alg == DNS_KEYALG_DH)
	{
		fatal("a key with algorithm %s cannot be a zone key", algstr);
	}

	switch (ctx->alg) {
	case DNS_KEYALG_RSASHA1:
	case DNS_KEYALG_NSEC3RSASHA1:
	case DNS_KEYALG_RSASHA256:
	case DNS_KEYALG_RSASHA512:
		show_progress = true;
		break;

	case DNS_KEYALG_DH:
		param = ctx->generator;
		break;

	case DST_ALG_ECDSA256:
	case DST_ALG_ECDSA384:
	case DST_ALG_ED25519:
	case DST_ALG_ED448:
		show_progress = true;
		break;
	}

	if ((flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY) {
		null_key = true;
	}

	isc_buffer_init(&buf, filename, sizeof(filename) - 1);

	do {
		conflict = false;

		if (!ctx->quiet && show_progress) {
			fprintf(stderr, "Generating key pair.");
			ret = dst_key_generate(name, ctx->alg, ctx->size, param,
					       flags, ctx->protocol,
					       ctx->rdclass, mctx, &key,
					       &progress);
			putc('\n', stderr);
			fflush(stderr);
		} else {
			ret = dst_key_generate(name, ctx->alg, ctx->size, param,
					       flags, ctx->protocol,
					       ctx->rdclass, mctx, &key, NULL);
		}

		if (ret != ISC_R_SUCCESS) {
			char namestr[DNS_NAME_FORMATSIZE];
			dns_name_format(name, namestr, sizeof(namestr));
			fatal("failed to generate key %s/%s: %s\n", namestr,
			      algstr, isc_result_totext(ret));
		}

		dst_key_setbits(key, ctx->dbits);

		/*
		 * Set key timing metadata (unless using -C)
		 *
		 * Creation date is always set to "now".
		 *
		 * For a new key without an explicit predecessor, publish
		 * and activation dates are set to "now" by default, but
		 * can both be overridden.
		 *
		 * For a successor key, activation is set to match the
		 * predecessor's inactivation date.  Publish is set to 30
		 * days earlier than that (XXX: this should be configurable).
		 * If either of the resulting dates are in the past, that's
		 * an error; the inactivation date of the predecessor key
		 * must be updated before a successor key can be created.
		 */
		if (!ctx->oldstyle) {
			dst_key_settime(key, DST_TIME_CREATED, ctx->now);

			if (ctx->genonly && (ctx->setpub || ctx->setact)) {
				fatal("cannot use -G together with "
				      "-P or -A options");
			}

			if (ctx->setpub) {
				dst_key_settime(key, DST_TIME_PUBLISH,
						ctx->publish);
			} else if (ctx->setact && !ctx->unsetpub) {
				dst_key_settime(key, DST_TIME_PUBLISH,
						ctx->activate - ctx->prepub);
			} else if (!ctx->genonly && !ctx->unsetpub) {
				dst_key_settime(key, DST_TIME_PUBLISH,
						ctx->now);
			}

			if (ctx->setact) {
				dst_key_settime(key, DST_TIME_ACTIVATE,
						ctx->activate);
			} else if (!ctx->genonly && !ctx->unsetact) {
				dst_key_settime(key, DST_TIME_ACTIVATE,
						ctx->now);
			}

			if (ctx->setrev) {
				if (ctx->kskflag == 0) {
					fprintf(stderr,
						"%s: warning: Key is "
						"not flagged as a KSK, but -R "
						"was used. Revoking a ZSK is "
						"legal, but undefined.\n",
						program);
				}
				dst_key_settime(key, DST_TIME_REVOKE,
						ctx->revokekey);
			}

			if (ctx->setinact) {
				dst_key_settime(key, DST_TIME_INACTIVE,
						ctx->inactive);
			}

			if (ctx->setdel) {
				if (ctx->setinact &&
				    ctx->deltime < ctx->inactive)
				{
					fprintf(stderr,
						"%s: warning: Key is "
						"scheduled to be deleted "
						"before it is scheduled to be "
						"made inactive.\n",
						program);
				}
				dst_key_settime(key, DST_TIME_DELETE,
						ctx->deltime);
			}

			if (ctx->setsyncadd) {
				dst_key_settime(key, DST_TIME_SYNCPUBLISH,
						ctx->syncadd);
			}

			if (ctx->setsyncdel) {
				dst_key_settime(key, DST_TIME_SYNCDELETE,
						ctx->syncdel);
			}
		} else {
			if (ctx->setpub || ctx->setact || ctx->setrev ||
			    ctx->setinact || ctx->setdel || ctx->unsetpub ||
			    ctx->unsetact || ctx->unsetrev || ctx->unsetinact ||
			    ctx->unsetdel || ctx->genonly || ctx->setsyncadd ||
			    ctx->setsyncdel)
			{
				fatal("cannot use -C together with "
				      "-P, -A, -R, -I, -D, or -G options");
			}
			/*
			 * Compatibility mode: Private-key-format
			 * should be set to 1.2.
			 */
			dst_key_setprivateformat(key, 1, 2);
		}

		/* Set the default key TTL */
		if (ctx->setttl) {
			dst_key_setttl(key, ctx->ttl);
		}

		/* Set dnssec-policy related metadata */
		if (ctx->policy != NULL) {
			dst_key_setnum(key, DST_NUM_LIFETIME, ctx->lifetime);
			dst_key_setbool(key, DST_BOOL_KSK, ctx->ksk);
			dst_key_setbool(key, DST_BOOL_ZSK, ctx->zsk);
		}

		/*
		 * Do not overwrite an existing key, or create a key
		 * if there is a risk of ID collision due to this key
		 * or another key being revoked.
		 */
		if (key_collision(key, name, ctx->directory, mctx, NULL)) {
			conflict = true;
			if (null_key) {
				dst_key_free(&key);
				break;
			}

			if (verbose > 0) {
				isc_buffer_clear(&buf);
				ret = dst_key_buildfilename(
					key, 0, ctx->directory, &buf);
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

	if (conflict) {
		fatal("cannot generate a null key due to possible key ID "
		      "collision");
	}

	if (ctx->predecessor != NULL && prevkey != NULL) {
		dst_key_setnum(prevkey, DST_NUM_SUCCESSOR, dst_key_id(key));
		dst_key_setnum(key, DST_NUM_PREDECESSOR, dst_key_id(prevkey));

		ret = dst_key_tofile(prevkey, ctx->options, ctx->directory);
		if (ret != ISC_R_SUCCESS) {
			char keystr[DST_KEY_FORMATSIZE];
			dst_key_format(prevkey, keystr, sizeof(keystr));
			fatal("failed to update predecessor %s: %s\n", keystr,
			      isc_result_totext(ret));
		}
	}

	ret = dst_key_tofile(key, ctx->options, ctx->directory);
	if (ret != ISC_R_SUCCESS) {
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key, keystr, sizeof(keystr));
		fatal("failed to write key %s: %s\n", keystr,
		      isc_result_totext(ret));
	}

	isc_buffer_clear(&buf);
	ret = dst_key_buildfilename(key, 0, NULL, &buf);
	if (ret != ISC_R_SUCCESS) {
		fatal("dst_key_buildfilename returned: %s\n",
		      isc_result_totext(ret));
	}
	printf("%s\n", filename);

	dst_key_free(&key);
	if (prevkey != NULL) {
		dst_key_free(&prevkey);
	}
}

int
main(int argc, char **argv) {
	char *algname = NULL, *freeit = NULL;
	char *classname = NULL;
	char *endp;
	isc_mem_t *mctx = NULL;
	isc_result_t ret;
	isc_textregion_t r;
	const char *engine = NULL;
	unsigned char c;
	int ch;

	keygen_ctx_t ctx = {
		.options = DST_TYPE_PRIVATE | DST_TYPE_PUBLIC,
		.prepub = -1,
		.protocol = -1,
		.size = -1,
	};

	if (argc == 1) {
		usage();
	}

#if USE_PKCS11
	pk11_result_register();
#endif /* if USE_PKCS11 */
	dns_result_register();

	isc_commandline_errprint = false;

	/*
	 * Process memory debugging argument first.
	 */
#define CMDLINE_FLAGS                                           \
	"3A:a:b:Cc:D:d:E:eFf:Gg:hI:i:K:k:L:l:m:n:P:p:qR:r:S:s:" \
	"T:t:v:V"
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'm':
			if (strcasecmp(isc_commandline_argument, "record") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			}
			if (strcasecmp(isc_commandline_argument, "trace") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			}
			if (strcasecmp(isc_commandline_argument, "usage") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			}
			if (strcasecmp(isc_commandline_argument, "size") == 0) {
				isc_mem_debugging |= ISC_MEM_DEBUGSIZE;
			}
			if (strcasecmp(isc_commandline_argument, "mctx") == 0) {
				isc_mem_debugging |= ISC_MEM_DEBUGCTX;
			}
			break;
		default:
			break;
		}
	}
	isc_commandline_reset = true;

	isc_mem_create(&mctx);
	isc_stdtime_get(&ctx.now);

	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case '3':
			ctx.use_nsec3 = true;
			break;
		case 'a':
			algname = isc_commandline_argument;
			break;
		case 'b':
			ctx.size = strtol(isc_commandline_argument, &endp, 10);
			if (*endp != '\0' || ctx.size < 0) {
				fatal("-b requires a non-negative number");
			}
			break;
		case 'C':
			ctx.oldstyle = true;
			break;
		case 'c':
			classname = isc_commandline_argument;
			break;
		case 'd':
			ctx.dbits = strtol(isc_commandline_argument, &endp, 10);
			if (*endp != '\0' || ctx.dbits < 0) {
				fatal("-d requires a non-negative number");
			}
			break;
		case 'E':
			engine = isc_commandline_argument;
			break;
		case 'e':
			fprintf(stderr, "phased-out option -e "
					"(was 'use (RSA) large exponent')\n");
			break;
		case 'f':
			c = (unsigned char)(isc_commandline_argument[0]);
			if (toupper(c) == 'K') {
				ctx.kskflag = DNS_KEYFLAG_KSK;
			} else if (toupper(c) == 'R') {
				ctx.revflag = DNS_KEYFLAG_REVOKE;
			} else {
				fatal("unknown flag '%s'",
				      isc_commandline_argument);
			}
			break;
		case 'g':
			ctx.generator = strtol(isc_commandline_argument, &endp,
					       10);
			if (*endp != '\0' || ctx.generator <= 0) {
				fatal("-g requires a positive number");
			}
			break;
		case 'K':
			ctx.directory = isc_commandline_argument;
			ret = try_dir(ctx.directory);
			if (ret != ISC_R_SUCCESS) {
				fatal("cannot open directory %s: %s",
				      ctx.directory, isc_result_totext(ret));
			}
			break;
		case 'k':
			ctx.policy = isc_commandline_argument;
			break;
		case 'L':
			ctx.ttl = strtottl(isc_commandline_argument);
			ctx.setttl = true;
			break;
		case 'l':
			ctx.configfile = isc_commandline_argument;
			break;
		case 'n':
			ctx.nametype = isc_commandline_argument;
			break;
		case 'm':
			break;
		case 'p':
			ctx.protocol = strtol(isc_commandline_argument, &endp,
					      10);
			if (*endp != '\0' || ctx.protocol < 0 ||
			    ctx.protocol > 255)
			{
				fatal("-p must be followed by a number "
				      "[0..255]");
			}
			break;
		case 'q':
			ctx.quiet = true;
			break;
		case 'r':
			fatal("The -r option has been deprecated.\n"
			      "System random data is always used.\n");
			break;
		case 's':
			ctx.signatory = strtol(isc_commandline_argument, &endp,
					       10);
			if (*endp != '\0' || ctx.signatory < 0 ||
			    ctx.signatory > 15)
			{
				fatal("-s must be followed by a number "
				      "[0..15]");
			}
			break;
		case 'T':
			if (strcasecmp(isc_commandline_argument, "KEY") == 0) {
				ctx.options |= DST_TYPE_KEY;
			} else if (strcasecmp(isc_commandline_argument,
					      "DNSKE"
					      "Y") == 0)
			{
				/* default behavior */
			} else {
				fatal("unknown type '%s'",
				      isc_commandline_argument);
			}
			break;
		case 't':
			ctx.type = isc_commandline_argument;
			break;
		case 'v':
			endp = NULL;
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0') {
				fatal("-v must be followed by a number");
			}
			break;
		case 'G':
			ctx.genonly = true;
			break;
		case 'P':
			/* -Psync ? */
			if (isoptarg("sync", argv, usage)) {
				if (ctx.setsyncadd) {
					fatal("-P sync specified more than "
					      "once");
				}

				ctx.syncadd = strtotime(
					isc_commandline_argument, ctx.now,
					ctx.now, &ctx.setsyncadd);
				break;
			}
			(void)isoptarg("dnskey", argv, usage);
			if (ctx.setpub || ctx.unsetpub) {
				fatal("-P specified more than once");
			}

			ctx.publish = strtotime(isc_commandline_argument,
						ctx.now, ctx.now, &ctx.setpub);
			ctx.unsetpub = !ctx.setpub;
			break;
		case 'A':
			if (ctx.setact || ctx.unsetact) {
				fatal("-A specified more than once");
			}

			ctx.activate = strtotime(isc_commandline_argument,
						 ctx.now, ctx.now, &ctx.setact);
			ctx.unsetact = !ctx.setact;
			break;
		case 'R':
			if (ctx.setrev || ctx.unsetrev) {
				fatal("-R specified more than once");
			}

			ctx.revokekey = strtotime(isc_commandline_argument,
						  ctx.now, ctx.now,
						  &ctx.setrev);
			ctx.unsetrev = !ctx.setrev;
			break;
		case 'I':
			if (ctx.setinact || ctx.unsetinact) {
				fatal("-I specified more than once");
			}

			ctx.inactive = strtotime(isc_commandline_argument,
						 ctx.now, ctx.now,
						 &ctx.setinact);
			ctx.unsetinact = !ctx.setinact;
			break;
		case 'D':
			/* -Dsync ? */
			if (isoptarg("sync", argv, usage)) {
				if (ctx.setsyncdel) {
					fatal("-D sync specified more than "
					      "once");
				}

				ctx.syncdel = strtotime(
					isc_commandline_argument, ctx.now,
					ctx.now, &ctx.setsyncdel);
				break;
			}
			(void)isoptarg("dnskey", argv, usage);
			if (ctx.setdel || ctx.unsetdel) {
				fatal("-D specified more than once");
			}

			ctx.deltime = strtotime(isc_commandline_argument,
						ctx.now, ctx.now, &ctx.setdel);
			ctx.unsetdel = !ctx.setdel;
			break;
		case 'S':
			ctx.predecessor = isc_commandline_argument;
			break;
		case 'i':
			ctx.prepub = strtottl(isc_commandline_argument);
			break;
		case 'F':
			/* Reserved for FIPS mode */
			FALLTHROUGH;
		case '?':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
			}
			FALLTHROUGH;
		case 'h':
			/* Does not return. */
			usage();

		case 'V':
			/* Does not return. */
			version(program);

		default:
			fprintf(stderr, "%s: unhandled option -%c\n", program,
				isc_commandline_option);
			exit(1);
		}
	}

	if (!isatty(0)) {
		ctx.quiet = true;
	}

	ret = dst_lib_init(mctx, engine);
	if (ret != ISC_R_SUCCESS) {
		fatal("could not initialize dst: %s", isc_result_totext(ret));
	}

	setup_logging(mctx, &lctx);

	ctx.rdclass = strtoclass(classname);

	if (ctx.configfile == NULL || ctx.configfile[0] == '\0') {
		ctx.configfile = NAMED_CONFFILE;
	}

	if (ctx.predecessor == NULL) {
		if (argc < isc_commandline_index + 1) {
			fatal("the key name was not specified");
		}
		if (argc > isc_commandline_index + 1) {
			fatal("extraneous arguments");
		}
	}

	if (ctx.predecessor == NULL && ctx.policy == NULL) {
		if (algname == NULL) {
			fatal("no algorithm specified");
		}
		r.base = algname;
		r.length = strlen(algname);
		ret = dns_secalg_fromtext(&ctx.alg, &r);
		if (ret != ISC_R_SUCCESS) {
			fatal("unknown algorithm %s", algname);
		}
		if (!dst_algorithm_supported(ctx.alg)) {
			fatal("unsupported algorithm: %s", algname);
		}
	}

	if (ctx.policy != NULL) {
		if (ctx.nametype != NULL) {
			fatal("-k and -n cannot be used together");
		}
		if (ctx.predecessor != NULL) {
			fatal("-k and -S cannot be used together");
		}
		if (ctx.oldstyle) {
			fatal("-k and -C cannot be used together");
		}
		if (ctx.setttl) {
			fatal("-k and -L cannot be used together");
		}
		if (ctx.prepub > 0) {
			fatal("-k and -i cannot be used together");
		}
		if (ctx.size != -1) {
			fatal("-k and -b cannot be used together");
		}
		if (ctx.kskflag || ctx.revflag) {
			fatal("-k and -f cannot be used together");
		}
		if (ctx.options & DST_TYPE_KEY) {
			fatal("-k and -T KEY cannot be used together");
		}
		if (ctx.use_nsec3) {
			fatal("-k and -3 cannot be used together");
		}

		ctx.options |= DST_TYPE_STATE;

		if (strcmp(ctx.policy, "default") == 0) {
			ctx.use_nsec3 = false;
			ctx.alg = DST_ALG_ECDSA256;
			ctx.size = 0;
			ctx.kskflag = DNS_KEYFLAG_KSK;
			ctx.ttl = 3600;
			ctx.setttl = true;
			ctx.ksk = true;
			ctx.zsk = true;
			ctx.lifetime = 0;

			keygen(&ctx, mctx, argc, argv);
		} else {
			cfg_parser_t *parser = NULL;
			cfg_obj_t *config = NULL;
			dns_kasp_t *kasp = NULL;
			dns_kasp_key_t *kaspkey = NULL;

			RUNTIME_CHECK(cfg_parser_create(mctx, lctx, &parser) ==
				      ISC_R_SUCCESS);
			if (cfg_parse_file(parser, ctx.configfile,
					   &cfg_type_namedconf,
					   &config) != ISC_R_SUCCESS)
			{
				fatal("unable to load dnssec-policy '%s' from "
				      "'%s'",
				      ctx.policy, ctx.configfile);
			}

			kasp_from_conf(config, mctx, ctx.policy, &kasp);
			if (kasp == NULL) {
				fatal("failed to load dnssec-policy '%s'",
				      ctx.policy);
			}
			if (ISC_LIST_EMPTY(dns_kasp_keys(kasp))) {
				fatal("dnssec-policy '%s' has no keys "
				      "configured",
				      ctx.policy);
			}

			ctx.ttl = dns_kasp_dnskeyttl(kasp);
			ctx.setttl = true;

			kaspkey = ISC_LIST_HEAD(dns_kasp_keys(kasp));

			while (kaspkey != NULL) {
				ctx.use_nsec3 = false;
				ctx.alg = dns_kasp_key_algorithm(kaspkey);
				ctx.size = dns_kasp_key_size(kaspkey);
				ctx.kskflag = dns_kasp_key_ksk(kaspkey)
						      ? DNS_KEYFLAG_KSK
						      : 0;
				ctx.ksk = dns_kasp_key_ksk(kaspkey);
				ctx.zsk = dns_kasp_key_zsk(kaspkey);
				ctx.lifetime = dns_kasp_key_lifetime(kaspkey);

				keygen(&ctx, mctx, argc, argv);

				kaspkey = ISC_LIST_NEXT(kaspkey, link);
			}

			dns_kasp_detach(&kasp);
			cfg_obj_destroy(parser, &config);
			cfg_parser_destroy(&parser);
		}
	} else {
		keygen(&ctx, mctx, argc, argv);
	}

	cleanup_logging(&lctx);
	dst_lib_destroy();
	if (verbose > 10) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	if (freeit != NULL) {
		free(freeit);
	}

	return (0);
}
