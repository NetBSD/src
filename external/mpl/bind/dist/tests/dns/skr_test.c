/*	$NetBSD: skr_test.c,v 1.1.1.1 2025/01/26 16:12:37 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/file.h>
#include <isc/hex.h>
#include <isc/lex.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/name.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/secalg.h>
#include <dns/skr.h>
#include <dns/time.h>

#include "zone_p.h"

#include <tests/dns.h>

typedef struct {
	isc_stdtime_t btime;
	isc_stdtime_t inception;
	isc_stdtime_t expiration;

	char kskbuf[1024];
	dns_rdata_t ksk;

	char zsk1buf[1024];
	dns_rdata_t zsk1;

	char zsk2buf[1024];
	dns_rdata_t zsk2;

	char cdnskeybuf[1024];
	dns_rdata_t cdnskey;

	char cdsbuf[1024];
	dns_rdata_t cds;

	char rrsig1buf[1024];
	dns_rdata_t dnskey_rrsig;

	char rrsig2buf[1024];
	dns_rdata_t cdnskey_rrsig;

	char rrsig3buf[1024];
	dns_rdata_t cds_rrsig;
} skr__testbundle_t;

static skr__testbundle_t test_bundles[42];

static dns_dnsseckeylist_t keys;

static const char *testskr = TESTS_DIR "/testdata/skr/test.skr";
static const char *kskstr =
	"257 3 13 evPZ03dt9VeWNQKqw1fpuL0V1RcyPRge4s306hGOVYg1a1IttOf3ZKIm "
	"McMgdT1K4nxJ+S7BtX6RVECqzp1rAA==";
static const char *zsk1str =
	"256 3 13 GIyBcxr9uBJvybXw2eOeZ1nWjRd+0INxUPlKaWI1KQxJwWRJTOJMw33g "
	"SSCz++TBmKyXm5ghl+56vOkoO33ppg==";
static const char *zsk2str =
	"256 3 13 1oC9YpShKeL5HQnYIMX7y77b9qbnAsKIjVuU0AUoo2kTA1D2fXxB9W95 "
	"+uqIiJuiteHK/oGmeDy4UsiTd2W1DQ==";
static const char *cdsstr =
	"52433 13 2 90C4668A53D8BE06049BABD2DFC93F4C6B46C9055E20D91166381E22 "
	"11BD9615";

static dns_name_t *dname = NULL;

#define BUNDLE_HAS_ZSK1 10
#define BUNDLE_HAS_ZSK2 20

#define OFFSET	 3600
#define TTL	 3600
#define LIFETIME 864000

#define SIG_FORMATSIZE \
	(DNS_NAME_FORMATSIZE + DNS_SECALG_FORMATSIZE + sizeof("65535"))

static void
print_rdata(FILE *fp, dns_rdata_t *rdata) {
	dns_rdataset_t rrset = DNS_RDATASET_INIT;

	dns_rdatalist_t *rdatalist = isc_mem_get(mctx, sizeof(*rdatalist));
	dns_rdatalist_init(rdatalist);
	rdatalist->rdclass = dns_rdataclass_in;
	rdatalist->type = rdata->type;
	rdatalist->ttl = TTL;

	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	dns_rdatalist_tordataset(rdatalist, &rrset);

	isc_buffer_t target;
	isc_region_t r;
	isc_result_t ret;
	char buf[4096];

	isc_buffer_init(&target, buf, sizeof(buf));
	ret = dns_rdataset_totext(&rrset, dname, false, false, &target);
	assert_int_equal(ret, ISC_R_SUCCESS);
	isc_buffer_usedregion(&target, &r);
	fprintf(fp, "%.*s", (int)r.length, (char *)r.base);

	for (dns_rdata_t *rd = ISC_LIST_HEAD(rdatalist->rdata); rd != NULL;
	     rd = ISC_LIST_HEAD(rdatalist->rdata))
	{
		ISC_LIST_UNLINK(rdatalist->rdata, rdata, link);
	}
	isc_mem_put(mctx, rdatalist, sizeof(*rdatalist));
}

static void
sign_rrset(FILE *fp, isc_stdtime_t inception, isc_stdtime_t expiration,
	   dns_rdataset_t *rrset, char *target_mem, dns_rdata_t *rrsig) {
	dns_dnsseckey_t *ksk = ISC_LIST_HEAD(keys);
	isc_stdtime_t clockskew = inception - OFFSET;
	isc_result_t ret;
	isc_buffer_t target;

	isc_buffer_init(&target, target_mem, 1024);
	ret = dns_dnssec_sign(dname, rrset, ksk->key, &clockskew, &expiration,
			      mctx, &target, rrsig);
	assert_int_equal(ret, ISC_R_SUCCESS);

	print_rdata(fp, rrsig);
}

static void
write_record(FILE *fp, dns_rdatatype_t rdtype, const char *rdatastr,
	     char *target_mem, dns_rdata_t *rdata) {
	isc_buffer_t source, target;
	isc_lex_t *lex = NULL;
	isc_lexspecials_t specials = { 0 };
	isc_result_t ret;

	/* Set up source to hold the input string. */
	isc_buffer_init(&target, target_mem, 1024);
	isc_buffer_constinit(&source, rdatastr, strlen(rdatastr));
	isc_buffer_add(&source, strlen(rdatastr));

	/* Create a lexer as one is required by dns_rdata_fromtext(). */
	isc_lex_create(mctx, 64, &lex);
	specials[0] = 1;
	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);
	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);
	ret = isc_lex_openbuffer(lex, &source);
	assert_int_equal(ret, ISC_R_SUCCESS);

	ret = dns_rdata_fromtext(rdata, dns_rdataclass_in, rdtype, lex, dname,
				 0, mctx, &target, NULL);
	assert_int_equal(ret, ISC_R_SUCCESS);

	print_rdata(fp, rdata);

	isc_lex_destroy(&lex);
}

static void
create_bundle(FILE *fp, isc_stdtime_t btime, int bnum) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	char utc[sizeof("YYYYMMDDHHSSMM")];
	dns_rdatalist_t *dnskeylist, *cdnskeylist, *cdslist;
	dns_rdataset_t *dnskeyset = NULL;
	dns_rdataset_t *cdnskeyset = NULL;
	dns_rdataset_t *cdsset = NULL;
	isc_buffer_t b, timebuf;
	isc_region_t r;
	isc_result_t ret;

	/* Write header to file. */
	test_bundles[bnum].btime = btime;
	isc_buffer_init(&timebuf, timestr, sizeof(timestr));
	isc_stdtime_tostring(btime, timestr, sizeof(timestr));
	isc_buffer_init(&b, utc, sizeof(utc));
	ret = dns_time32_totext(btime, &b);
	assert_int_equal(ret, ISC_R_SUCCESS);

	isc_buffer_usedregion(&b, &r);
	fprintf(fp, ";; SignedKeyResponse 1.0 %.*s (%s)\n", (int)r.length,
		r.base, timestr);

	/* Write records to file. */
	dns_rdata_init(&test_bundles[bnum].ksk);
	write_record(fp, dns_rdatatype_dnskey, kskstr,
		     test_bundles[bnum].kskbuf, &test_bundles[bnum].ksk);

	if (bnum < BUNDLE_HAS_ZSK2) {
		dns_rdata_init(&test_bundles[bnum].zsk1);
		write_record(fp, dns_rdatatype_dnskey, zsk1str,
			     test_bundles[bnum].zsk1buf,
			     &test_bundles[bnum].zsk1);
	}
	if (bnum > BUNDLE_HAS_ZSK1) {
		dns_rdata_init(&test_bundles[bnum].zsk2);
		write_record(fp, dns_rdatatype_dnskey, zsk2str,
			     test_bundles[bnum].zsk2buf,
			     &test_bundles[bnum].zsk2);
	}
	/* Create the DNSKEY signature. */
	dnskeylist = isc_mem_get(mctx, sizeof(*dnskeylist));
	dnskeyset = isc_mem_get(mctx, sizeof(*dnskeyset));
	dns_rdatalist_init(dnskeylist);
	dns_rdataset_init(dnskeyset);
	dnskeylist->rdclass = dns_rdataclass_in;
	dnskeylist->type = dns_rdatatype_dnskey;
	dnskeylist->ttl = TTL;
	ISC_LIST_APPEND(dnskeylist->rdata, &test_bundles[bnum].ksk, link);
	dns_rdatalist_tordataset(dnskeylist, dnskeyset);
	dns_rdata_init(&test_bundles[bnum].dnskey_rrsig);
	sign_rrset(fp, btime, (btime + LIFETIME), dnskeyset,
		   test_bundles[bnum].rrsig1buf,
		   &test_bundles[bnum].dnskey_rrsig);
	for (dns_rdata_t *rd = ISC_LIST_HEAD(dnskeylist->rdata); rd != NULL;
	     rd = ISC_LIST_HEAD(dnskeylist->rdata))
	{
		ISC_LIST_UNLINK(dnskeylist->rdata, rd, link);
	}
	isc_mem_put(mctx, dnskeylist, sizeof(*dnskeylist));
	isc_mem_put(mctx, dnskeyset, sizeof(*dnskeyset));

	/* CDNSKEY */
	dns_rdata_init(&test_bundles[bnum].cdnskey);
	write_record(fp, dns_rdatatype_cdnskey, kskstr,
		     test_bundles[bnum].cdnskeybuf,
		     &test_bundles[bnum].cdnskey);

	cdnskeylist = isc_mem_get(mctx, sizeof(*cdnskeylist));
	cdnskeyset = isc_mem_get(mctx, sizeof(*cdnskeyset));
	dns_rdatalist_init(cdnskeylist);
	dns_rdataset_init(cdnskeyset);
	cdnskeylist->rdclass = dns_rdataclass_in;
	cdnskeylist->type = dns_rdatatype_cdnskey;
	cdnskeylist->ttl = TTL;
	ISC_LIST_APPEND(cdnskeylist->rdata, &test_bundles[bnum].cdnskey, link);
	dns_rdatalist_tordataset(cdnskeylist, cdnskeyset);
	dns_rdata_init(&test_bundles[bnum].cdnskey_rrsig);
	sign_rrset(fp, btime, (btime + LIFETIME), cdnskeyset,
		   test_bundles[bnum].rrsig2buf,
		   &test_bundles[bnum].cdnskey_rrsig);
	for (dns_rdata_t *rd = ISC_LIST_HEAD(cdnskeylist->rdata); rd != NULL;
	     rd = ISC_LIST_HEAD(cdnskeylist->rdata))
	{
		ISC_LIST_UNLINK(cdnskeylist->rdata, rd, link);
	}
	isc_mem_put(mctx, cdnskeylist, sizeof(*cdnskeylist));
	isc_mem_put(mctx, cdnskeyset, sizeof(*cdnskeyset));

	/* CDS */
	dns_rdata_init(&test_bundles[bnum].cds);
	write_record(fp, dns_rdatatype_cds, cdsstr, test_bundles[bnum].cdsbuf,
		     &test_bundles[bnum].cds);

	cdslist = isc_mem_get(mctx, sizeof(*cdslist));
	cdsset = isc_mem_get(mctx, sizeof(*cdsset));
	dns_rdatalist_init(cdslist);
	dns_rdataset_init(cdsset);
	cdslist->rdclass = dns_rdataclass_in;
	cdslist->type = dns_rdatatype_cds;
	cdslist->ttl = TTL;
	ISC_LIST_APPEND(cdslist->rdata, &test_bundles[bnum].cds, link);
	dns_rdatalist_tordataset(cdslist, cdsset);
	dns_rdata_init(&test_bundles[bnum].cds_rrsig);
	sign_rrset(fp, btime, (btime + LIFETIME), cdsset,
		   test_bundles[bnum].rrsig3buf, &test_bundles[bnum].cds_rrsig);
	for (dns_rdata_t *rd = ISC_LIST_HEAD(cdslist->rdata); rd != NULL;
	     rd = ISC_LIST_HEAD(cdslist->rdata))
	{
		ISC_LIST_UNLINK(cdslist->rdata, rd, link);
	}
	isc_mem_put(mctx, cdslist, sizeof(*cdslist));
	isc_mem_put(mctx, cdsset, sizeof(*cdsset));

	/* Signature times. */
	test_bundles[bnum].btime = btime;
	test_bundles[bnum].inception = (btime - OFFSET);
	test_bundles[bnum].expiration = (btime + LIFETIME);
}

static void
check_rrsig(dns_skrbundle_t *bundle, skr__testbundle_t *tb,
	    dns_rdatatype_t rrtype, isc_result_t ret) {
	isc_result_t r;
	dns_dnsseckey_t *key = ISC_LIST_HEAD(keys);
	dns_rdata_t sigrdata = DNS_RDATA_INIT;

	r = dns_skrbundle_getsig(bundle, key->key, rrtype, &sigrdata);
	assert_int_equal(r, ret);

	if (r == ISC_R_SUCCESS) {
		int cmp = 1;
		dns_rdata_rrsig_t sig;

		switch (rrtype) {
		case dns_rdatatype_dnskey:
			cmp = dns_rdata_compare(&sigrdata, &tb->dnskey_rrsig);
			break;
		case dns_rdatatype_cdnskey:
			cmp = dns_rdata_compare(&sigrdata, &tb->cdnskey_rrsig);
			break;
		case dns_rdatatype_cds:
			cmp = dns_rdata_compare(&sigrdata, &tb->cds_rrsig);
			break;
		default:
			cmp = 1;
		}

		assert_int_equal(cmp, 0);

		r = dns_rdata_tostruct(&sigrdata, &sig, NULL);
		assert_int_equal(r, ISC_R_SUCCESS);

		assert_int_equal(sig.timesigned, tb->inception);
		assert_int_equal(sig.timeexpire, tb->expiration);
	}
}

static void
check_bundle(dns_skrbundle_t *bundle, skr__testbundle_t *tb, int bnum) {
	int dnskey = 0;

	assert_int_equal(bundle->inception, tb->btime);

	dns_difftuple_t *tuple = ISC_LIST_HEAD(bundle->diff.tuples);
	while (tuple != NULL) {
		int cmp = 1;

		switch (tuple->rdata.type) {
		case dns_rdatatype_dnskey:
			switch (dnskey) {
			case 0:
				cmp = dns_rdata_compare(&tuple->rdata,
							&tb->ksk);
				break;
			case 1:
				if (bnum < BUNDLE_HAS_ZSK2) {
					cmp = dns_rdata_compare(&tuple->rdata,
								&tb->zsk1);
				} else {
					cmp = dns_rdata_compare(&tuple->rdata,
								&tb->zsk2);
				}
				break;
			case 2:
				cmp = dns_rdata_compare(&tuple->rdata,
							&tb->zsk2);
				break;
			default:
				cmp = 1;
			}
			dnskey++;
			break;
		case dns_rdatatype_cdnskey:
			cmp = dns_rdata_compare(&tuple->rdata, &tb->cdnskey);
			break;
		case dns_rdatatype_cds:
			cmp = dns_rdata_compare(&tuple->rdata, &tb->cds);
			break;
		case dns_rdatatype_rrsig:
			cmp = 0;
			break;
		default:
			cmp = 1;
		}

		assert_int_equal(cmp, 0);
		tuple = ISC_LIST_NEXT(tuple, link);
	}

	check_rrsig(bundle, tb, dns_rdatatype_dnskey, ISC_R_SUCCESS);
	check_rrsig(bundle, tb, dns_rdatatype_cdnskey, ISC_R_SUCCESS);
	check_rrsig(bundle, tb, dns_rdatatype_cds, ISC_R_SUCCESS);
	check_rrsig(bundle, tb, dns_rdatatype_a, ISC_R_NOTFOUND);
}

static void
create_skr_file(void) {
	isc_result_t ret;
	isc_stdtime_t start_time;
	size_t tempfilelen;
	char *tempfile = NULL;
	FILE *outfp = NULL;

	/* Set up output file */
	tempfilelen = strlen(TESTS_DIR "/testdata/skr/") + 20;
	tempfile = isc_mem_get(mctx, tempfilelen);
	ret = isc_file_mktemplate(testskr, tempfile, tempfilelen);
	assert_int_equal(ret, ISC_R_SUCCESS);
	ret = isc_file_openunique(tempfile, &outfp);
	assert_int_equal(ret, ISC_R_SUCCESS);

	start_time = isc_stdtime_now();
	for (int i = 0; i < 42; i++) {
		create_bundle(outfp, start_time, i);
		start_time += LIFETIME;
	}

	fprintf(outfp, ";; SignedKeyResponse 1.0 generated by test-dev\n");

	ret = isc_stdio_close(outfp);
	assert_int_equal(ret, ISC_R_SUCCESS);
	ret = isc_file_rename(tempfile, testskr);
	assert_int_equal(ret, ISC_R_SUCCESS);

	isc_file_remove(tempfile);
	isc_mem_put(mctx, tempfile, tempfilelen);
}

ISC_RUN_TEST_IMPL(skr_read) {
	char *name = UNCONST("test");
	dns_fixedname_t dfname;
	dns_skr_t *skr = NULL;
	isc_buffer_t b;
	isc_result_t result;
	size_t count = 0;

	dst_lib_init(mctx, NULL);

	/* Owner name */
	dname = dns_fixedname_initname(&dfname);
	isc_buffer_init(&b, name, strlen(name));
	isc_buffer_add(&b, strlen(name));
	result = dns_name_fromtext(dname, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Get the KSK */
	ISC_LIST_INIT(keys);
	result = dns_dnssec_findmatchingkeys(
		dname, NULL, TESTS_DIR "/testdata/skr/", NULL, 0, mctx, &keys);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Create/read the SKR file */
	create_skr_file();
	dns_skr_create(mctx, testskr, dname, dns_rdataclass_in, &skr);
	result = dns_skr_read(mctx, testskr, dname, dns_rdataclass_in, TTL,
			      &skr);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_file_remove(testskr);

	/* Test bundles */
	for (dns_skrbundle_t *bundle = ISC_LIST_HEAD(skr->bundles);
	     bundle != NULL; bundle = ISC_LIST_NEXT(bundle, link))
	{
		count++;
	}
	assert_int_equal(count, 42);

	for (int i = 0; i < 42; i++) {
		skr__testbundle_t tb = test_bundles[i];
		dns_skrbundle_t *lb;

		lb = dns_skr_lookup(skr, tb.btime, LIFETIME);
		check_bundle(lb, &tb, i);

		lb = dns_skr_lookup(skr, tb.btime + 1, LIFETIME);
		check_bundle(lb, &tb, i);
	}

	/* Clean up */
	dns_skr_destroy(skr);

	while (!ISC_LIST_EMPTY(keys)) {
		dns_dnsseckey_t *key = ISC_LIST_HEAD(keys);
		ISC_LIST_UNLINK(keys, key, link);
		dst_key_free(&key->key);
		dns_dnsseckey_destroy(mctx, &key);
	}

	dst_lib_destroy();
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(skr_read)
ISC_TEST_LIST_END

ISC_TEST_MAIN
