/*	$NetBSD: dnssec-dsfromkey.c,v 1.4 2019/02/24 20:01:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatatype.h>
#include <dns/result.h>

#include <dst/dst.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif

#include "dnssectool.h"

#ifndef PATH_MAX
#define PATH_MAX 1024   /* WIN32, and others don't define this. */
#endif

const char *program = "dnssec-dsfromkey";
int verbose;

static dns_rdataclass_t rdclass;
static dns_fixedname_t	fixed;
static dns_name_t	*name = NULL;
static isc_mem_t	*mctx = NULL;
static uint32_t	ttl;
static bool	emitttl = false;

static isc_result_t
initname(char *setname) {
	isc_result_t result;
	isc_buffer_t buf;

	name = dns_fixedname_initname(&fixed);

	isc_buffer_init(&buf, setname, strlen(setname));
	isc_buffer_add(&buf, strlen(setname));
	result = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
	return (result);
}

static void
db_load_from_stream(dns_db_t *db, FILE *fp) {
	isc_result_t result;
	dns_rdatacallbacks_t callbacks;

	dns_rdatacallbacks_init(&callbacks);
	result = dns_db_beginload(db, &callbacks);
	if (result != ISC_R_SUCCESS)
		fatal("dns_db_beginload failed: %s", isc_result_totext(result));

	result = dns_master_loadstream(fp, name, name, rdclass, 0,
				       &callbacks, mctx);
	if (result != ISC_R_SUCCESS)
		fatal("can't load from input: %s", isc_result_totext(result));

	result = dns_db_endload(db, &callbacks);
	if (result != ISC_R_SUCCESS)
		fatal("dns_db_endload failed: %s", isc_result_totext(result));
}

static isc_result_t
loadset(const char *filename, dns_rdataset_t *rdataset) {
	isc_result_t	 result;
	dns_db_t	 *db = NULL;
	dns_dbnode_t	 *node = NULL;
	char setname[DNS_NAME_FORMATSIZE];

	dns_name_format(name, setname, sizeof(setname));

	result = dns_db_create(mctx, "rbt", name, dns_dbtype_zone,
			       rdclass, 0, NULL, &db);
	if (result != ISC_R_SUCCESS)
		fatal("can't create database");

	if (strcmp(filename, "-") == 0) {
		db_load_from_stream(db, stdin);
		filename = "input";
	} else {
		result = dns_db_load(db, filename, dns_masterformat_text, 0);
		if (result != ISC_R_SUCCESS && result != DNS_R_SEENINCLUDE)
			fatal("can't load %s: %s", filename,
			      isc_result_totext(result));
	}

	result = dns_db_findnode(db, name, false, &node);
	if (result != ISC_R_SUCCESS)
		fatal("can't find %s node in %s", setname, filename);

	result = dns_db_findrdataset(db, node, NULL, dns_rdatatype_dnskey,
				     0, 0, rdataset, NULL);

	if (result == ISC_R_NOTFOUND)
		fatal("no DNSKEY RR for %s in %s", setname, filename);
	else if (result != ISC_R_SUCCESS)
		fatal("dns_db_findrdataset");

	if (node != NULL)
		dns_db_detachnode(db, &node);
	if (db != NULL)
		dns_db_detach(&db);
	return (result);
}

static isc_result_t
loadkeyset(char *dirname, dns_rdataset_t *rdataset) {
	isc_result_t	 result;
	char		 filename[PATH_MAX + 1];
	isc_buffer_t	 buf;

	dns_rdataset_init(rdataset);

	isc_buffer_init(&buf, filename, sizeof(filename));
	if (dirname != NULL) {
		/* allow room for a trailing slash */
		if (strlen(dirname) >= isc_buffer_availablelength(&buf))
			return (ISC_R_NOSPACE);
		isc_buffer_putstr(&buf, dirname);
		if (dirname[strlen(dirname) - 1] != '/')
			isc_buffer_putstr(&buf, "/");
	}

	if (isc_buffer_availablelength(&buf) < 7)
		return (ISC_R_NOSPACE);
	isc_buffer_putstr(&buf, "keyset-");

	result = dns_name_tofilenametext(name, false, &buf);
	check_result(result, "dns_name_tofilenametext()");
	if (isc_buffer_availablelength(&buf) == 0)
		return (ISC_R_NOSPACE);
	isc_buffer_putuint8(&buf, 0);

	return (loadset(filename, rdataset));
}

static void
loadkey(char *filename, unsigned char *key_buf, unsigned int key_buf_size,
	dns_rdata_t *rdata)
{
	isc_result_t  result;
	dst_key_t     *key = NULL;
	isc_buffer_t  keyb;
	isc_region_t  r;

	dns_rdata_init(rdata);

	isc_buffer_init(&keyb, key_buf, key_buf_size);

	result = dst_key_fromnamedfile(filename, NULL, DST_TYPE_PUBLIC,
				       mctx, &key);
	if (result != ISC_R_SUCCESS)
		fatal("can't load %s.key: %s",
		      filename, isc_result_totext(result));

	if (verbose > 2) {
		char keystr[DST_KEY_FORMATSIZE];

		dst_key_format(key, keystr, sizeof(keystr));
		fprintf(stderr, "%s: %s\n", program, keystr);
	}

	result = dst_key_todns(key, &keyb);
	if (result != ISC_R_SUCCESS)
		fatal("can't decode key");

	isc_buffer_usedregion(&keyb, &r);
	dns_rdata_fromregion(rdata, dst_key_class(key),
			     dns_rdatatype_dnskey, &r);

	rdclass = dst_key_class(key);

	name = dns_fixedname_initname(&fixed);
	result = dns_name_copy(dst_key_name(key), name, NULL);
	if (result != ISC_R_SUCCESS)
		fatal("can't copy name");

	dst_key_free(&key);
}

static void
logkey(dns_rdata_t *rdata)
{
	isc_result_t result;
	dst_key_t    *key = NULL;
	isc_buffer_t buf;
	char	     keystr[DST_KEY_FORMATSIZE];

	isc_buffer_init(&buf, rdata->data, rdata->length);
	isc_buffer_add(&buf, rdata->length);
	result = dst_key_fromdns(name, rdclass, &buf, mctx, &key);
	if (result != ISC_R_SUCCESS)
		return;

	dst_key_format(key, keystr, sizeof(keystr));
	fprintf(stderr, "%s: %s\n", program, keystr);

	dst_key_free(&key);
}

static void
emit(unsigned int dtype, bool showall, char *lookaside,
     bool cds, dns_rdata_t *rdata)
{
	isc_result_t result;
	unsigned char buf[DNS_DS_BUFFERSIZE];
	char text_buf[DST_KEY_MAXTEXTSIZE];
	char name_buf[DNS_NAME_MAXWIRE];
	char class_buf[10];
	isc_buffer_t textb, nameb, classb;
	isc_region_t r;
	dns_rdata_t ds;
	dns_rdata_dnskey_t dnskey;

	isc_buffer_init(&textb, text_buf, sizeof(text_buf));
	isc_buffer_init(&nameb, name_buf, sizeof(name_buf));
	isc_buffer_init(&classb, class_buf, sizeof(class_buf));

	dns_rdata_init(&ds);

	result = dns_rdata_tostruct(rdata, &dnskey, NULL);
	if (result != ISC_R_SUCCESS)
		fatal("can't convert DNSKEY");

	if ((dnskey.flags & DNS_KEYFLAG_KSK) == 0 && !showall)
		return;

	result = dns_ds_buildrdata(name, rdata, dtype, buf, &ds);
	if (result != ISC_R_SUCCESS)
		fatal("can't build record");

	result = dns_name_totext(name, false, &nameb);
	if (result != ISC_R_SUCCESS)
		fatal("can't print name");

	/* Add lookaside origin, if set */
	if (lookaside != NULL) {
		if (isc_buffer_availablelength(&nameb) < strlen(lookaside))
			fatal("DLV origin '%s' is too long", lookaside);
		isc_buffer_putstr(&nameb, lookaside);
		if (lookaside[strlen(lookaside) - 1] != '.') {
			if (isc_buffer_availablelength(&nameb) < 1)
				fatal("DLV origin '%s' is too long", lookaside);
			isc_buffer_putstr(&nameb, ".");
		}
	}

	result = dns_rdata_tofmttext(&ds, (dns_name_t *) NULL, 0, 0, 0, "",
				     &textb);

	if (result != ISC_R_SUCCESS)
		fatal("can't print rdata");

	result = dns_rdataclass_totext(rdclass, &classb);
	if (result != ISC_R_SUCCESS)
		fatal("can't print class");

	isc_buffer_usedregion(&nameb, &r);
	printf("%.*s ", (int)r.length, r.base);

	if (emitttl)
		printf("%u ", ttl);

	isc_buffer_usedregion(&classb, &r);
	printf("%.*s", (int)r.length, r.base);

	if (lookaside == NULL) {
		if (cds)
			printf(" CDS ");
		else
			printf(" DS ");
	} else
		printf(" DLV ");

	isc_buffer_usedregion(&textb, &r);
	printf("%.*s\n", (int)r.length, r.base);
}

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr,	"    %s [options] keyfile\n\n", program);
	fprintf(stderr, "    %s [options] -f zonefile [zonename]\n\n", program);
	fprintf(stderr, "    %s [options] -s dnsname\n\n", program);
	fprintf(stderr, "    %s [-h|-V]\n\n", program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "Options:\n"
"    -1: digest algorithm SHA-1\n"
"    -2: digest algorithm SHA-256\n"
"    -a algorithm: digest algorithm (SHA-1, SHA-256 or SHA-384)\n"
"    -A: include all keys in DS set, not just KSKs (-f only)\n"
"    -c class: rdata class for DS set (default IN) (-f or -s only)\n"
"    -C: print CDS records\n"
"    -f zonefile: read keys from a zone file\n"
"    -h: print help information\n"
"    -K directory: where to find key or keyset files\n"
"    -l zone: print DLV records in the given lookaside zone\n"
"    -s: read keys from keyset-<dnsname> file\n"
"    -T: TTL of output records (omitted by default)\n"
"    -v level: verbosity\n"
"    -V: print version information\n");
	fprintf(stderr, "Output: DS, DLV, or CDS RRs\n");

	exit (-1);
}

int
main(int argc, char **argv) {
	char		*classname = NULL;
	char		*filename = NULL, *dir = NULL, *namestr;
	char		*lookaside = NULL;
	char		*endp;
	int		ch;
	unsigned int	dtype = DNS_DSDIGEST_SHA1;
	bool	cds = false;
	bool	both = true;
	bool	usekeyset = false;
	bool	showall = false;
	isc_result_t	result;
	isc_log_t	*log = NULL;
	dns_rdataset_t	rdataset;
	dns_rdata_t	rdata;

	dns_rdata_init(&rdata);

	if (argc == 1)
		usage();

	result = isc_mem_create(0, 0, &mctx);
	if (result != ISC_R_SUCCESS)
		fatal("out of memory");

#if USE_PKCS11
	pk11_result_register();
#endif
	dns_result_register();

	isc_commandline_errprint = false;

#define OPTIONS "12Aa:Cc:d:Ff:K:l:sT:v:hV"
	while ((ch = isc_commandline_parse(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case '1':
			dtype = DNS_DSDIGEST_SHA1;
			both = false;
			break;
		case '2':
			dtype = DNS_DSDIGEST_SHA256;
			both = false;
			break;
		case 'A':
			showall = true;
			break;
		case 'a':
			dtype = strtodsdigest(isc_commandline_argument);
			both = false;
			break;
		case 'C':
			if (lookaside != NULL)
				fatal("lookaside and CDS are mutually"
				      " exclusive");
			cds = true;
			break;
		case 'c':
			classname = isc_commandline_argument;
			break;
		case 'd':
			fprintf(stderr, "%s: the -d option is deprecated; "
					"use -K\n", program);
			/* fall through */
		case 'K':
			dir = isc_commandline_argument;
			if (strlen(dir) == 0U)
				fatal("directory must be non-empty string");
			break;
		case 'f':
			filename = isc_commandline_argument;
			break;
		case 'l':
			if (cds)
				fatal("lookaside and CDS are mutually"
				      " exclusive");
			lookaside = isc_commandline_argument;
			if (strlen(lookaside) == 0U)
				fatal("lookaside must be a non-empty string");
			break;
		case 's':
			usekeyset = true;
			break;
		case 'T':
			emitttl = true;
			ttl = strtottl(isc_commandline_argument);
			break;
		case 'v':
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0')
				fatal("-v must be followed by a number");
			break;
		case 'F':
			/* Reserved for FIPS mode */
			/* FALLTHROUGH */
		case '?':
			if (isc_commandline_option != '?')
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
			/* FALLTHROUGH */
		case 'h':
			/* Does not return. */
			usage();

		case 'V':
			/* Does not return. */
			version(program);

		default:
			fprintf(stderr, "%s: unhandled option -%c\n",
				program, isc_commandline_option);
			exit(1);
		}
	}

	rdclass = strtoclass(classname);

	if (usekeyset && filename != NULL)
		fatal("cannot use both -s and -f");

	/* When not using -f, -A is implicit */
	if (filename == NULL)
		showall = true;

	if (argc < isc_commandline_index + 1 && filename == NULL)
		fatal("the key file name was not specified");
	if (argc > isc_commandline_index + 1)
		fatal("extraneous arguments");

	result = dst_lib_init(mctx, NULL);
	if (result != ISC_R_SUCCESS)
		fatal("could not initialize dst: %s",
		      isc_result_totext(result));

	setup_logging(mctx, &log);

	dns_rdataset_init(&rdataset);

	if (usekeyset || filename != NULL) {
		if (argc < isc_commandline_index + 1 && filename != NULL) {
			/* using zone name as the zone file name */
			namestr = filename;
		} else
			namestr = argv[isc_commandline_index];

		result = initname(namestr);
		if (result != ISC_R_SUCCESS)
			fatal("could not initialize name %s", namestr);

		if (usekeyset)
			result = loadkeyset(dir, &rdataset);
		else
			result = loadset(filename, &rdataset);

		if (result != ISC_R_SUCCESS)
			fatal("could not load DNSKEY set: %s\n",
			      isc_result_totext(result));

		for (result = dns_rdataset_first(&rdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&rdataset)) {
			dns_rdata_init(&rdata);
			dns_rdataset_current(&rdataset, &rdata);

			if (verbose > 2)
				logkey(&rdata);

			if (both) {
				emit(DNS_DSDIGEST_SHA1, showall, lookaside,
				     cds, &rdata);
				emit(DNS_DSDIGEST_SHA256, showall, lookaside,
				     cds, &rdata);
			} else
				emit(dtype, showall, lookaside, cds, &rdata);
		}
	} else {
		unsigned char key_buf[DST_KEY_MAXSIZE];

		loadkey(argv[isc_commandline_index], key_buf,
			DST_KEY_MAXSIZE, &rdata);

		if (both) {
			emit(DNS_DSDIGEST_SHA1, showall, lookaside, cds,
			     &rdata);
			emit(DNS_DSDIGEST_SHA256, showall, lookaside, cds,
			     &rdata);
		} else
			emit(dtype, showall, lookaside, cds, &rdata);
	}

	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);
	cleanup_logging(&log);
	dst_lib_destroy();
	dns_name_destroy();
	if (verbose > 10)
		isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);

	fflush(stdout);
	if (ferror(stdout)) {
		fprintf(stderr, "write error\n");
		return (1);
	} else
		return (0);
}
