/*	$NetBSD: dnssec-cds.c,v 1.9 2023/01/25 21:43:23 christos Exp $	*/

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

/*
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at Cambridge University Information Services
 */

/*! \file */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/serial.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/dnssec.h>
#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/time.h>

#include <dst/dst.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif /* if USE_PKCS11 */

#include "dnssectool.h"

const char *program = "dnssec-cds";

/*
 * Infrastructure
 */
static isc_log_t *lctx = NULL;
static isc_mem_t *mctx = NULL;

/*
 * The domain we are working on
 */
static const char *namestr = NULL;
static dns_fixedname_t fixed;
static dns_name_t *name = NULL;
static dns_rdataclass_t rdclass = dns_rdataclass_in;

static const char *startstr = NULL; /* from which we derive notbefore */
static isc_stdtime_t notbefore = 0; /* restrict sig inception times */
static dns_rdata_rrsig_t oldestsig; /* for recording inception time */

static int nkey; /* number of child zone DNSKEY records */

/*
 * The validation strategy of this program is top-down.
 *
 * We start with an implicitly trusted authoritative dsset.
 *
 * The child DNSKEY RRset is scanned to find out which keys are
 * authenticated by DS records, and the result is recorded in a key
 * table as described later in this comment.
 *
 * The key table is used up to three times to verify the signatures on
 * the child DNSKEY, CDNSKEY, and CDS RRsets. In this program, only keys
 * that have matching DS records are used for validating signatures.
 *
 * For replay attack protection, signatures are ignored if their inception
 * time is before the previously recorded inception time. We use the earliest
 * signature so that another run of dnssec-cds with the same records will
 * still accept all the signatures.
 *
 * A key table is an array of nkey keyinfo structures, like
 *
 *	keyinfo_t key_tbl[nkey];
 *
 * Each key is decoded into more useful representations, held in
 *	keyinfo->rdata
 *	keyinfo->dst
 *
 * If a key has no matching DS record then keyinfo->dst is NULL.
 *
 * The key algorithm and ID are saved in keyinfo->algo and
 * keyinfo->tag for quicky skipping DS and RRSIG records that can't
 * match.
 */
typedef struct keyinfo {
	dns_rdata_t rdata;
	dst_key_t *dst;
	dns_secalg_t algo;
	dns_keytag_t tag;
} keyinfo_t;

/* A replaceable function that can generate a DS RRset from some input */
typedef isc_result_t
ds_maker_func_t(dns_rdatalist_t *dslist, isc_buffer_t *buf, dns_rdata_t *rdata);

static dns_rdataset_t cdnskey_set, cdnskey_sig;
static dns_rdataset_t cds_set, cds_sig;
static dns_rdataset_t dnskey_set, dnskey_sig;
static dns_rdataset_t old_ds_set, new_ds_set;

static keyinfo_t *old_key_tbl, *new_key_tbl;

isc_buffer_t *new_ds_buf = NULL; /* backing store for new_ds_set */

static void
verbose_time(int level, const char *msg, isc_stdtime_t time) {
	isc_result_t result;
	isc_buffer_t timebuf;
	char timestr[32];

	if (verbose < level) {
		return;
	}

	isc_buffer_init(&timebuf, timestr, sizeof(timestr));
	result = dns_time64_totext(time, &timebuf);
	check_result(result, "dns_time64_totext()");
	isc_buffer_putuint8(&timebuf, 0);
	if (verbose < 3) {
		vbprintf(level, "%s %s\n", msg, timestr);
	} else {
		vbprintf(level, "%s %s (%" PRIu32 ")\n", msg, timestr, time);
	}
}

static void
initname(char *setname) {
	isc_result_t result;
	isc_buffer_t buf;

	name = dns_fixedname_initname(&fixed);
	namestr = setname;

	isc_buffer_init(&buf, setname, strlen(setname));
	isc_buffer_add(&buf, strlen(setname));
	result = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("could not initialize name %s", setname);
	}
}

static void
findset(dns_db_t *db, dns_dbnode_t *node, dns_rdatatype_t type,
	dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	isc_result_t result;

	dns_rdataset_init(rdataset);
	if (sigrdataset != NULL) {
		dns_rdataset_init(sigrdataset);
	}
	result = dns_db_findrdataset(db, node, NULL, type, 0, 0, rdataset,
				     sigrdataset);
	if (result != ISC_R_NOTFOUND) {
		check_result(result, "dns_db_findrdataset()");
	}
}

static void
freeset(dns_rdataset_t *rdataset) {
	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}
}

static void
freelist(dns_rdataset_t *rdataset) {
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
free_all_sets(void) {
	freeset(&cdnskey_set);
	freeset(&cdnskey_sig);
	freeset(&cds_set);
	freeset(&cds_sig);
	freeset(&dnskey_set);
	freeset(&dnskey_sig);
	freeset(&old_ds_set);
	freelist(&new_ds_set);
	if (new_ds_buf != NULL) {
		isc_buffer_free(&new_ds_buf);
	}
}

static void
load_db(const char *filename, dns_db_t **dbp, dns_dbnode_t **nodep) {
	isc_result_t result;

	result = dns_db_create(mctx, "rbt", name, dns_dbtype_zone, rdclass, 0,
			       NULL, dbp);
	check_result(result, "dns_db_create()");

	result = dns_db_load(*dbp, filename, dns_masterformat_text,
			     DNS_MASTER_HINT);
	if (result != ISC_R_SUCCESS && result != DNS_R_SEENINCLUDE) {
		fatal("can't load %s: %s", filename, isc_result_totext(result));
	}

	result = dns_db_findnode(*dbp, name, false, nodep);
	if (result != ISC_R_SUCCESS) {
		fatal("can't find %s node in %s", namestr, filename);
	}
}

static void
free_db(dns_db_t **dbp, dns_dbnode_t **nodep) {
	dns_db_detachnode(*dbp, nodep);
	dns_db_detach(dbp);
}

static void
load_child_sets(const char *file) {
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;

	load_db(file, &db, &node);
	findset(db, node, dns_rdatatype_dnskey, &dnskey_set, &dnskey_sig);
	findset(db, node, dns_rdatatype_cdnskey, &cdnskey_set, &cdnskey_sig);
	findset(db, node, dns_rdatatype_cds, &cds_set, &cds_sig);
	free_db(&db, &node);
}

static void
get_dsset_name(char *filename, size_t size, const char *path,
	       const char *suffix) {
	isc_result_t result;
	isc_buffer_t buf;
	size_t len;

	isc_buffer_init(&buf, filename, size);

	len = strlen(path);

	/* allow room for a trailing slash */
	if (isc_buffer_availablelength(&buf) <= len) {
		fatal("%s: pathname too long", path);
	}
	isc_buffer_putstr(&buf, path);

	if (isc_file_isdirectory(path) == ISC_R_SUCCESS) {
		const char *prefix = "dsset-";

		if (path[len - 1] != '/') {
			isc_buffer_putstr(&buf, "/");
		}

		if (isc_buffer_availablelength(&buf) < strlen(prefix)) {
			fatal("%s: pathname too long", path);
		}
		isc_buffer_putstr(&buf, prefix);

		result = dns_name_tofilenametext(name, false, &buf);
		check_result(result, "dns_name_tofilenametext()");
		if (isc_buffer_availablelength(&buf) == 0) {
			fatal("%s: pathname too long", path);
		}
	}
	/* allow room for a trailing nul */
	if (isc_buffer_availablelength(&buf) <= strlen(suffix)) {
		fatal("%s: pathname too long", path);
	}
	isc_buffer_putstr(&buf, suffix);
	isc_buffer_putuint8(&buf, 0);
}

static void
load_parent_set(const char *path) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	isc_time_t modtime;
	char filename[PATH_MAX + 1];

	get_dsset_name(filename, sizeof(filename), path, "");

	result = isc_file_getmodtime(filename, &modtime);
	if (result != ISC_R_SUCCESS) {
		fatal("could not get modification time of %s: %s", filename,
		      isc_result_totext(result));
	}
	notbefore = isc_time_seconds(&modtime);
	if (startstr != NULL) {
		isc_stdtime_t now;
		isc_stdtime_get(&now);
		notbefore = strtotime(startstr, now, notbefore, NULL);
	}
	verbose_time(1, "child records must not be signed before", notbefore);

	load_db(filename, &db, &node);
	findset(db, node, dns_rdatatype_ds, &old_ds_set, NULL);

	if (!dns_rdataset_isassociated(&old_ds_set)) {
		fatal("could not find DS records for %s in %s", namestr,
		      filename);
	}

	free_db(&db, &node);
}

#define MAX_CDS_RDATA_TEXT_SIZE DNS_RDATA_MAXLENGTH * 2

static isc_buffer_t *
formatset(dns_rdataset_t *rdataset) {
	isc_result_t result;
	isc_buffer_t *buf = NULL;
	dns_master_style_t *style = NULL;
	unsigned int styleflags;

	styleflags = (rdataset->ttl == 0) ? DNS_STYLEFLAG_NO_TTL : 0;

	/*
	 * This style is for consistency with the output of dnssec-dsfromkey
	 * which just separates fields with spaces. The huge tab stop width
	 * eliminates any tab characters.
	 */
	result = dns_master_stylecreate(&style, styleflags, 0, 0, 0, 0, 0,
					1000000, 0, mctx);
	check_result(result, "dns_master_stylecreate2 failed");

	isc_buffer_allocate(mctx, &buf, MAX_CDS_RDATA_TEXT_SIZE);
	result = dns_master_rdatasettotext(name, rdataset, style, NULL, buf);

	if ((result == ISC_R_SUCCESS) && isc_buffer_availablelength(buf) < 1) {
		result = ISC_R_NOSPACE;
	}

	check_result(result, "dns_rdataset_totext()");

	isc_buffer_putuint8(buf, 0);

	dns_master_styledestroy(&style, mctx);

	return (buf);
}

static void
write_parent_set(const char *path, const char *inplace, bool nsupdate,
		 dns_rdataset_t *rdataset) {
	isc_result_t result;
	isc_buffer_t *buf = NULL;
	isc_region_t r;
	isc_time_t filetime;
	char backname[PATH_MAX + 1];
	char filename[PATH_MAX + 1];
	char tmpname[PATH_MAX + 1];
	FILE *fp = NULL;

	if (nsupdate && inplace == NULL) {
		return;
	}

	buf = formatset(rdataset);
	isc_buffer_usedregion(buf, &r);

	/*
	 * Try to ensure a write error doesn't make a zone go insecure!
	 */
	if (inplace == NULL) {
		printf("%s", (char *)r.base);
		isc_buffer_free(&buf);
		if (fflush(stdout) == EOF) {
			fatal("error writing to stdout: %s", strerror(errno));
		}
		return;
	}

	if (inplace[0] != '\0') {
		get_dsset_name(backname, sizeof(backname), path, inplace);
	}
	get_dsset_name(filename, sizeof(filename), path, "");
	get_dsset_name(tmpname, sizeof(tmpname), path, "-XXXXXXXXXX");

	result = isc_file_openunique(tmpname, &fp);
	if (result != ISC_R_SUCCESS) {
		fatal("open %s: %s", tmpname, isc_result_totext(result));
	}
	fprintf(fp, "%s", (char *)r.base);
	isc_buffer_free(&buf);
	if (fclose(fp) == EOF) {
		int err = errno;
		isc_file_remove(tmpname);
		fatal("error writing to %s: %s", tmpname, strerror(err));
	}

	isc_time_set(&filetime, oldestsig.timesigned, 0);
	result = isc_file_settime(tmpname, &filetime);
	if (result != ISC_R_SUCCESS) {
		isc_file_remove(tmpname);
		fatal("can't set modification time of %s: %s", tmpname,
		      isc_result_totext(result));
	}

	if (inplace[0] != '\0') {
		isc_file_rename(filename, backname);
	}
	isc_file_rename(tmpname, filename);
}

typedef enum { LOOSE, TIGHT } strictness_t;

/*
 * Find out if any (C)DS record matches a particular (C)DNSKEY.
 */
static bool
match_key_dsset(keyinfo_t *ki, dns_rdataset_t *dsset, strictness_t strictness) {
	isc_result_t result;
	unsigned char dsbuf[DNS_DS_BUFFERSIZE];

	for (result = dns_rdataset_first(dsset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(dsset))
	{
		dns_rdata_ds_t ds;
		dns_rdata_t dsrdata = DNS_RDATA_INIT;
		dns_rdata_t newdsrdata = DNS_RDATA_INIT;
		bool c;

		dns_rdataset_current(dsset, &dsrdata);
		result = dns_rdata_tostruct(&dsrdata, &ds, NULL);
		check_result(result, "dns_rdata_tostruct(DS)");

		if (ki->tag != ds.key_tag || ki->algo != ds.algorithm) {
			continue;
		}

		result = dns_ds_buildrdata(name, &ki->rdata, ds.digest_type,
					   dsbuf, &newdsrdata);
		if (result != ISC_R_SUCCESS) {
			vbprintf(3,
				 "dns_ds_buildrdata("
				 "keytag=%d, algo=%d, digest=%d): %s\n",
				 ds.key_tag, ds.algorithm, ds.digest_type,
				 dns_result_totext(result));
			continue;
		}
		/* allow for both DS and CDS */
		c = dsrdata.type != dns_rdatatype_ds;
		dsrdata.type = dns_rdatatype_ds;
		if (dns_rdata_compare(&dsrdata, &newdsrdata) == 0) {
			vbprintf(1, "found matching %s %d %d %d\n",
				 c ? "CDS" : "DS", ds.key_tag, ds.algorithm,
				 ds.digest_type);
			return (true);
		} else if (strictness == TIGHT) {
			vbprintf(0,
				 "key does not match %s %d %d %d "
				 "when it looks like it should\n",
				 c ? "CDS" : "DS", ds.key_tag, ds.algorithm,
				 ds.digest_type);
			return (false);
		}
	}

	vbprintf(1, "no matching %s for %s %d %d\n",
		 dsset->type == dns_rdatatype_cds ? "CDS" : "DS",
		 ki->rdata.type == dns_rdatatype_cdnskey ? "CDNSKEY" : "DNSKEY",
		 ki->tag, ki->algo);

	return (false);
}

/*
 * Find which (C)DNSKEY records match a (C)DS RRset.
 * This creates a keyinfo_t key_tbl[nkey] array.
 */
static keyinfo_t *
match_keyset_dsset(dns_rdataset_t *keyset, dns_rdataset_t *dsset,
		   strictness_t strictness) {
	isc_result_t result;
	keyinfo_t *keytable;
	int i;

	nkey = dns_rdataset_count(keyset);

	keytable = isc_mem_get(mctx, sizeof(keyinfo_t) * nkey);

	for (result = dns_rdataset_first(keyset), i = 0;
	     result == ISC_R_SUCCESS; result = dns_rdataset_next(keyset), i++)
	{
		keyinfo_t *ki;
		dns_rdata_dnskey_t dnskey;
		dns_rdata_t *keyrdata;
		isc_region_t r;

		INSIST(i < nkey);
		ki = &keytable[i];
		keyrdata = &ki->rdata;

		dns_rdata_init(keyrdata);
		dns_rdataset_current(keyset, keyrdata);

		result = dns_rdata_tostruct(keyrdata, &dnskey, NULL);
		check_result(result, "dns_rdata_tostruct(DNSKEY)");
		ki->algo = dnskey.algorithm;

		dns_rdata_toregion(keyrdata, &r);
		ki->tag = dst_region_computeid(&r);

		ki->dst = NULL;
		if (!match_key_dsset(ki, dsset, strictness)) {
			continue;
		}

		result = dns_dnssec_keyfromrdata(name, keyrdata, mctx,
						 &ki->dst);
		if (result != ISC_R_SUCCESS) {
			vbprintf(3,
				 "dns_dnssec_keyfromrdata("
				 "keytag=%d, algo=%d): %s\n",
				 ki->tag, ki->algo, dns_result_totext(result));
		}
	}

	return (keytable);
}

static void
free_keytable(keyinfo_t **keytable_p) {
	keyinfo_t *keytable = *keytable_p;
	*keytable_p = NULL;
	keyinfo_t *ki;
	int i;

	for (i = 0; i < nkey; i++) {
		ki = &keytable[i];
		if (ki->dst != NULL) {
			dst_key_free(&ki->dst);
		}
	}

	isc_mem_put(mctx, keytable, sizeof(keyinfo_t) * nkey);
}

/*
 * Find out which keys have signed an RRset. Keys that do not match a
 * DS record are skipped.
 *
 * The return value is an array with nkey elements, one for each key,
 * either zero if the key was skipped or did not sign the RRset, or
 * otherwise the key algorithm. This is used by the signature coverage
 * check functions below.
 */
static dns_secalg_t *
matching_sigs(keyinfo_t *keytbl, dns_rdataset_t *rdataset,
	      dns_rdataset_t *sigset) {
	isc_result_t result;
	dns_secalg_t *algo;
	int i;

	algo = isc_mem_get(mctx, nkey);
	memset(algo, 0, nkey);

	for (result = dns_rdataset_first(sigset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigset))
	{
		dns_rdata_t sigrdata = DNS_RDATA_INIT;
		dns_rdata_rrsig_t sig;

		dns_rdataset_current(sigset, &sigrdata);
		result = dns_rdata_tostruct(&sigrdata, &sig, NULL);
		check_result(result, "dns_rdata_tostruct(RRSIG)");

		/*
		 * Replay attack protection: check against current age limit
		 */
		if (isc_serial_lt(sig.timesigned, notbefore)) {
			vbprintf(1, "skip RRSIG by key %d: too old\n",
				 sig.keyid);
			continue;
		}

		for (i = 0; i < nkey; i++) {
			keyinfo_t *ki = &keytbl[i];
			if (sig.keyid != ki->tag || sig.algorithm != ki->algo ||
			    !dns_name_equal(&sig.signer, name))
			{
				continue;
			}
			if (ki->dst == NULL) {
				vbprintf(1,
					 "skip RRSIG by key %d:"
					 " no matching (C)DS\n",
					 sig.keyid);
				continue;
			}

			result = dns_dnssec_verify(name, rdataset, ki->dst,
						   false, 0, mctx, &sigrdata,
						   NULL);

			if (result != ISC_R_SUCCESS &&
			    result != DNS_R_FROMWILDCARD)
			{
				vbprintf(1,
					 "skip RRSIG by key %d:"
					 " verification failed: %s\n",
					 sig.keyid, isc_result_totext(result));
				continue;
			}

			vbprintf(1, "found RRSIG by key %d\n", ki->tag);
			algo[i] = sig.algorithm;

			/*
			 * Replay attack protection: work out next age limit,
			 * only after the signature has been verified
			 */
			if (oldestsig.timesigned == 0 ||
			    isc_serial_lt(sig.timesigned, oldestsig.timesigned))
			{
				verbose_time(2, "this is the oldest so far",
					     sig.timesigned);
				oldestsig = sig;
			}
		}
	}

	return (algo);
}

/*
 * Consume the result of matching_sigs(). When checking records
 * fetched from the child zone, any working signature is enough.
 */
static bool
signed_loose(dns_secalg_t *algo) {
	bool ok = false;
	int i;
	for (i = 0; i < nkey; i++) {
		if (algo[i] != 0) {
			ok = true;
		}
	}
	isc_mem_put(mctx, algo, nkey);
	return (ok);
}

/*
 * Consume the result of matching_sigs(). To ensure that the new DS
 * RRset does not break the chain of trust to the DNSKEY RRset, every
 * key algorithm in the DS RRset must have a signature in the DNSKEY
 * RRset.
 */
static bool
signed_strict(dns_rdataset_t *dsset, dns_secalg_t *algo) {
	isc_result_t result;
	bool all_ok = true;

	for (result = dns_rdataset_first(dsset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(dsset))
	{
		dns_rdata_t dsrdata = DNS_RDATA_INIT;
		dns_rdata_ds_t ds;
		bool ds_ok;
		int i;

		dns_rdataset_current(dsset, &dsrdata);
		result = dns_rdata_tostruct(&dsrdata, &ds, NULL);
		check_result(result, "dns_rdata_tostruct(DS)");

		ds_ok = false;
		for (i = 0; i < nkey; i++) {
			if (algo[i] == ds.algorithm) {
				ds_ok = true;
			}
		}
		if (!ds_ok) {
			vbprintf(0,
				 "missing signature for algorithm %d "
				 "(key %d)\n",
				 ds.algorithm, ds.key_tag);
			all_ok = false;
		}
	}

	isc_mem_put(mctx, algo, nkey);
	return (all_ok);
}

static dns_rdata_t *
rdata_get(void) {
	dns_rdata_t *rdata;

	rdata = isc_mem_get(mctx, sizeof(*rdata));
	dns_rdata_init(rdata);

	return (rdata);
}

static isc_result_t
rdata_put(isc_result_t result, dns_rdatalist_t *rdlist, dns_rdata_t *rdata) {
	if (result == ISC_R_SUCCESS) {
		ISC_LIST_APPEND(rdlist->rdata, rdata, link);
	} else {
		isc_mem_put(mctx, rdata, sizeof(*rdata));
	}

	return (result);
}

/*
 * This basically copies the rdata into the buffer, but going via the
 * unpacked struct has the side-effect of changing the rdatatype. The
 * dns_rdata_cds_t and dns_rdata_ds_t types are aliases.
 */
static isc_result_t
ds_from_cds(dns_rdatalist_t *dslist, isc_buffer_t *buf, dns_rdata_t *cds) {
	isc_result_t result;
	dns_rdata_ds_t ds;
	dns_rdata_t *rdata;

	REQUIRE(buf != NULL);

	rdata = rdata_get();

	result = dns_rdata_tostruct(cds, &ds, NULL);
	check_result(result, "dns_rdata_tostruct(CDS)");
	ds.common.rdtype = dns_rdatatype_ds;

	result = dns_rdata_fromstruct(rdata, rdclass, dns_rdatatype_ds, &ds,
				      buf);

	return (rdata_put(result, dslist, rdata));
}

static isc_result_t
ds_from_cdnskey(dns_rdatalist_t *dslist, isc_buffer_t *buf,
		dns_rdata_t *cdnskey) {
	isc_result_t result;
	unsigned i, n;

	REQUIRE(buf != NULL);

	n = sizeof(dtype) / sizeof(dtype[0]);
	for (i = 0; i < n; i++) {
		if (dtype[i] != 0) {
			dns_rdata_t *rdata;
			isc_region_t r;

			isc_buffer_availableregion(buf, &r);
			if (r.length < DNS_DS_BUFFERSIZE) {
				return (ISC_R_NOSPACE);
			}

			rdata = rdata_get();
			result = dns_ds_buildrdata(name, cdnskey, dtype[i],
						   r.base, rdata);
			if (result == ISC_R_SUCCESS) {
				isc_buffer_add(buf, DNS_DS_BUFFERSIZE);
			}

			result = rdata_put(result, dslist, rdata);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}
		}
	}

	return (ISC_R_SUCCESS);
}

static void
make_new_ds_set(ds_maker_func_t *ds_from_rdata, uint32_t ttl,
		dns_rdataset_t *rdset) {
	unsigned int size = 16;
	for (;;) {
		isc_result_t result;
		dns_rdatalist_t *dslist;

		dslist = isc_mem_get(mctx, sizeof(*dslist));

		dns_rdatalist_init(dslist);
		dslist->rdclass = rdclass;
		dslist->type = dns_rdatatype_ds;
		dslist->ttl = ttl;

		dns_rdataset_init(&new_ds_set);
		result = dns_rdatalist_tordataset(dslist, &new_ds_set);
		check_result(result, "dns_rdatalist_tordataset(dslist)");

		isc_buffer_allocate(mctx, &new_ds_buf, size);

		for (result = dns_rdataset_first(rdset);
		     result == ISC_R_SUCCESS; result = dns_rdataset_next(rdset))
		{
			isc_result_t tresult;
			dns_rdata_t rdata = DNS_RDATA_INIT;

			dns_rdataset_current(rdset, &rdata);

			tresult = ds_from_rdata(dslist, new_ds_buf, &rdata);
			if (tresult == ISC_R_NOSPACE) {
				vbprintf(20, "DS list buffer size %u\n", size);
				freelist(&new_ds_set);
				isc_buffer_free(&new_ds_buf);
				size *= 2;
				break;
			}

			check_result(tresult, "ds_from_rdata()");
		}

		if (result == ISC_R_NOMORE) {
			break;
		}
	}
}

static int
rdata_cmp(const void *rdata1, const void *rdata2) {
	return (dns_rdata_compare((const dns_rdata_t *)rdata1,
				  (const dns_rdata_t *)rdata2));
}

/*
 * Ensure that every key identified by the DS RRset has the same set of
 * digest types.
 */
static bool
consistent_digests(dns_rdataset_t *dsset) {
	isc_result_t result;
	dns_rdata_t *arrdata;
	dns_rdata_ds_t *ds;
	dns_keytag_t key_tag;
	dns_secalg_t algorithm;
	bool match;
	int i, j, n, d;

	/*
	 * First sort the dsset. DS rdata fields are tag, algorithm, digest,
	 * so sorting them brings together all the records for each key.
	 */

	n = dns_rdataset_count(dsset);

	arrdata = isc_mem_get(mctx, n * sizeof(dns_rdata_t));

	for (result = dns_rdataset_first(dsset), i = 0; result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(dsset), i++)
	{
		dns_rdata_init(&arrdata[i]);
		dns_rdataset_current(dsset, &arrdata[i]);
	}

	qsort(arrdata, n, sizeof(dns_rdata_t), rdata_cmp);

	/*
	 * Convert sorted arrdata to more accessible format
	 */
	ds = isc_mem_get(mctx, n * sizeof(dns_rdata_ds_t));

	for (i = 0; i < n; i++) {
		result = dns_rdata_tostruct(&arrdata[i], &ds[i], NULL);
		check_result(result, "dns_rdata_tostruct(DS)");
	}

	/*
	 * Count number of digest types (d) for first key
	 */
	key_tag = ds[0].key_tag;
	algorithm = ds[0].algorithm;
	for (d = 0, i = 0; i < n; i++, d++) {
		if (ds[i].key_tag != key_tag || ds[i].algorithm != algorithm) {
			break;
		}
	}

	/*
	 * Check subsequent keys match the first one
	 */
	match = true;
	while (i < n) {
		key_tag = ds[i].key_tag;
		algorithm = ds[i].algorithm;
		for (j = 0; j < d && i + j < n; j++) {
			if (ds[i + j].key_tag != key_tag ||
			    ds[i + j].algorithm != algorithm ||
			    ds[i + j].digest_type != ds[j].digest_type)
			{
				match = false;
			}
		}
		i += d;
	}

	/*
	 * Done!
	 */
	isc_mem_put(mctx, ds, n * sizeof(dns_rdata_ds_t));
	isc_mem_put(mctx, arrdata, n * sizeof(dns_rdata_t));

	return (match);
}

static void
print_diff(const char *cmd, dns_rdataset_t *rdataset) {
	isc_buffer_t *buf;
	isc_region_t r;
	unsigned char *nl;
	size_t len;

	buf = formatset(rdataset);
	isc_buffer_usedregion(buf, &r);

	while ((nl = memchr(r.base, '\n', r.length)) != NULL) {
		len = nl - r.base + 1;
		printf("update %s %.*s", cmd, (int)len, (char *)r.base);
		isc_region_consume(&r, len);
	}

	isc_buffer_free(&buf);
}

static void
update_diff(const char *cmd, uint32_t ttl, dns_rdataset_t *addset,
	    dns_rdataset_t *delset) {
	isc_result_t result;
	dns_db_t *db;
	dns_dbnode_t *node;
	dns_dbversion_t *ver;
	dns_rdataset_t diffset;
	uint32_t save;

	db = NULL;
	result = dns_db_create(mctx, "rbt", name, dns_dbtype_zone, rdclass, 0,
			       NULL, &db);
	check_result(result, "dns_db_create()");

	ver = NULL;
	result = dns_db_newversion(db, &ver);
	check_result(result, "dns_db_newversion()");

	node = NULL;
	result = dns_db_findnode(db, name, true, &node);
	check_result(result, "dns_db_findnode()");

	dns_rdataset_init(&diffset);

	result = dns_db_addrdataset(db, node, ver, 0, addset, DNS_DBADD_MERGE,
				    NULL);
	check_result(result, "dns_db_addrdataset()");

	result = dns_db_subtractrdataset(db, node, ver, delset, 0, &diffset);
	if (result == DNS_R_UNCHANGED) {
		save = addset->ttl;
		addset->ttl = ttl;
		print_diff(cmd, addset);
		addset->ttl = save;
	} else if (result != DNS_R_NXRRSET) {
		check_result(result, "dns_db_subtractrdataset()");
		diffset.ttl = ttl;
		print_diff(cmd, &diffset);
		dns_rdataset_disassociate(&diffset);
	}

	dns_db_detachnode(db, &node);
	dns_db_closeversion(db, &ver, false);
	dns_db_detach(&db);
}

static void
nsdiff(uint32_t ttl, dns_rdataset_t *oldset, dns_rdataset_t *newset) {
	if (ttl == 0) {
		vbprintf(1, "warning: no TTL in nsupdate script\n");
	}
	update_diff("add", ttl, newset, oldset);
	update_diff("del", 0, oldset, newset);
	if (verbose > 0) {
		printf("show\nsend\nanswer\n");
	} else {
		printf("send\n");
	}
	if (fflush(stdout) == EOF) {
		fatal("write stdout: %s", strerror(errno));
	}
}

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr,
		"    %s options [options] -f <file> -d <path> <domain>\n",
		program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "Options:\n"
			"    -a <algorithm>     digest algorithm (SHA-1 / "
			"SHA-256 / SHA-384)\n"
			"    -c <class>         of domain (default IN)\n"
			"    -D                 prefer CDNSKEY records instead "
			"of CDS\n"
			"    -d <file|dir>      where to find parent dsset- "
			"file\n"
			"    -f <file>          child DNSKEY+CDNSKEY+CDS+RRSIG "
			"records\n"
			"    -i[extension]      update dsset- file in place\n"
			"    -s <start-time>    oldest permitted child "
			"signatures\n"
			"    -u                 emit nsupdate script\n"
			"    -T <ttl>           TTL of DS records\n"
			"    -V                 print version\n"
			"    -v <verbosity>\n");
	exit(1);
}

int
main(int argc, char *argv[]) {
	const char *child_path = NULL;
	const char *ds_path = NULL;
	const char *inplace = NULL;
	isc_result_t result;
	bool prefer_cdnskey = false;
	bool nsupdate = false;
	uint32_t ttl = 0;
	int ch;
	char *endp;

	isc_mem_create(&mctx);

#if USE_PKCS11
	pk11_result_register();
#endif /* if USE_PKCS11 */
	dns_result_register();

	isc_commandline_errprint = false;

#define OPTIONS "a:c:Dd:f:i:ms:T:uv:V"
	while ((ch = isc_commandline_parse(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'a':
			add_dtype(strtodsdigest(isc_commandline_argument));
			break;
		case 'c':
			rdclass = strtoclass(isc_commandline_argument);
			break;
		case 'D':
			prefer_cdnskey = true;
			break;
		case 'd':
			ds_path = isc_commandline_argument;
			break;
		case 'f':
			child_path = isc_commandline_argument;
			break;
		case 'i':
			/*
			 * This is a bodge to make the argument optional,
			 * so that it works just like sed(1).
			 */
			if (isc_commandline_argument ==
			    argv[isc_commandline_index - 1])
			{
				isc_commandline_index--;
				inplace = "";
			} else {
				inplace = isc_commandline_argument;
			}
			break;
		case 'm':
			isc_mem_debugging = ISC_MEM_DEBUGTRACE |
					    ISC_MEM_DEBUGRECORD;
			break;
		case 's':
			startstr = isc_commandline_argument;
			break;
		case 'T':
			ttl = strtottl(isc_commandline_argument);
			break;
		case 'u':
			nsupdate = true;
			break;
		case 'V':
			/* Does not return. */
			version(program);
			break;
		case 'v':
			verbose = strtoul(isc_commandline_argument, &endp, 0);
			if (*endp != '\0') {
				fatal("-v must be followed by a number");
			}
			break;
		default:
			usage();
			break;
		}
	}
	argv += isc_commandline_index;
	argc -= isc_commandline_index;

	if (argc != 1) {
		usage();
	}
	initname(argv[0]);

	/*
	 * Default digest type if none specified.
	 */
	if (dtype[0] == 0) {
		dtype[0] = DNS_DSDIGEST_SHA256;
	}

	setup_logging(mctx, &lctx);

	result = dst_lib_init(mctx, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("could not initialize dst: %s",
		      isc_result_totext(result));
	}

	if (ds_path == NULL) {
		fatal("missing -d DS pathname");
	}
	load_parent_set(ds_path);

	/*
	 * Preserve the TTL if it wasn't overridden.
	 */
	if (ttl == 0) {
		ttl = old_ds_set.ttl;
	}

	if (child_path == NULL) {
		fatal("path to file containing child data must be specified");
	}

	load_child_sets(child_path);

	/*
	 * Check child records have accompanying RRSIGs and DNSKEYs
	 */

	if (!dns_rdataset_isassociated(&dnskey_set) ||
	    !dns_rdataset_isassociated(&dnskey_sig))
	{
		fatal("could not find signed DNSKEY RRset for %s", namestr);
	}

	if (dns_rdataset_isassociated(&cdnskey_set) &&
	    !dns_rdataset_isassociated(&cdnskey_sig))
	{
		fatal("missing RRSIG CDNSKEY records for %s", namestr);
	}
	if (dns_rdataset_isassociated(&cds_set) &&
	    !dns_rdataset_isassociated(&cds_sig))
	{
		fatal("missing RRSIG CDS records for %s", namestr);
	}

	vbprintf(1, "which child DNSKEY records match parent DS records?\n");
	old_key_tbl = match_keyset_dsset(&dnskey_set, &old_ds_set, LOOSE);

	/*
	 * We have now identified the keys that are allowed to authenticate
	 * the DNSKEY RRset (RFC 4035 section 5.2 bullet 2), and CDNSKEY and
	 * CDS RRsets (RFC 7344 section 4.1 bullet 2).
	 */

	vbprintf(1, "verify DNSKEY signature(s)\n");
	if (!signed_loose(matching_sigs(old_key_tbl, &dnskey_set, &dnskey_sig)))
	{
		fatal("could not validate child DNSKEY RRset for %s", namestr);
	}

	if (dns_rdataset_isassociated(&cdnskey_set)) {
		vbprintf(1, "verify CDNSKEY signature(s)\n");
		if (!signed_loose(matching_sigs(old_key_tbl, &cdnskey_set,
						&cdnskey_sig)))
		{
			fatal("could not validate child CDNSKEY RRset for %s",
			      namestr);
		}
	}
	if (dns_rdataset_isassociated(&cds_set)) {
		vbprintf(1, "verify CDS signature(s)\n");
		if (!signed_loose(
			    matching_sigs(old_key_tbl, &cds_set, &cds_sig)))
		{
			fatal("could not validate child CDS RRset for %s",
			      namestr);
		}
	}

	free_keytable(&old_key_tbl);

	/*
	 * Report the result of the replay attack protection checks
	 * used for the output file timestamp
	 */
	if (oldestsig.timesigned != 0 && verbose > 0) {
		char type[32];
		dns_rdatatype_format(oldestsig.covered, type, sizeof(type));
		verbose_time(1, "child signature inception time",
			     oldestsig.timesigned);
		vbprintf(2, "from RRSIG %s by key %d\n", type, oldestsig.keyid);
	}

	/*
	 * Successfully do nothing if there's neither CDNSKEY nor CDS
	 * RFC 7344 section 4.1 first paragraph
	 */
	if (!dns_rdataset_isassociated(&cdnskey_set) &&
	    !dns_rdataset_isassociated(&cds_set))
	{
		vbprintf(1, "%s has neither CDS nor CDNSKEY records\n",
			 namestr);
		write_parent_set(ds_path, inplace, nsupdate, &old_ds_set);
		exit(0);
	}

	/*
	 * Make DS records from the CDS or CDNSKEY records
	 * Prefer CDS if present, unless run with -D
	 */
	if (prefer_cdnskey && dns_rdataset_isassociated(&cdnskey_set)) {
		make_new_ds_set(ds_from_cdnskey, ttl, &cdnskey_set);
	} else if (dns_rdataset_isassociated(&cds_set)) {
		make_new_ds_set(ds_from_cds, ttl, &cds_set);
	} else {
		make_new_ds_set(ds_from_cdnskey, ttl, &cdnskey_set);
	}

	/*
	 * Now we have a candidate DS RRset, we need to check it
	 * won't break the delegation.
	 */
	vbprintf(1, "which child DNSKEY records match new DS records?\n");
	new_key_tbl = match_keyset_dsset(&dnskey_set, &new_ds_set, TIGHT);

	if (!consistent_digests(&new_ds_set)) {
		fatal("CDS records at %s do not cover each key "
		      "with the same set of digest types",
		      namestr);
	}

	vbprintf(1, "verify DNSKEY signature(s)\n");
	if (!signed_strict(&new_ds_set, matching_sigs(new_key_tbl, &dnskey_set,
						      &dnskey_sig)))
	{
		fatal("could not validate child DNSKEY RRset "
		      "with new DS records for %s",
		      namestr);
	}

	free_keytable(&new_key_tbl);

	/*
	 * OK, it's all good!
	 */
	if (nsupdate) {
		nsdiff(ttl, &old_ds_set, &new_ds_set);
	}

	write_parent_set(ds_path, inplace, nsupdate, &new_ds_set);

	free_all_sets();
	cleanup_logging(&lctx);
	dst_lib_destroy();
	if (verbose > 10) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	exit(0);
}
