/*	$NetBSD: keytable_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/md.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/name.h>
#include <dns/nta.h>
#include <dns/rdataclass.h>
#include <dns/rdatastruct.h>
#include <dns/rootns.h>
#include <dns/view.h>

#include <dst/dst.h>

#include <tests/dns.h>

dns_keytable_t *keytable = NULL;
dns_ntatable_t *ntatable = NULL;

static const char *keystr1 = "BQEAAAABok+vaUC9neRv8yeT/"
			     "FEGgN7svR8s7VBUVSBd8NsAiV8AlaAg "
			     "O5FHar3JQd95i/puZos6Vi6at9/"
			     "JBbN8qVmO2AuiXxVqfxMKxIcy+LEB "
			     "0Vw4NaSJ3N3uaVREso6aTSs98H/"
			     "25MjcwLOr7SFfXA7bGhZatLtYY/xu kp6Km5hMfkE=";

static const char *keystr2 = "BQEAAAABwuHz9Cem0BJ0JQTO7C/a3McR6hMaufljs1dfG/"
			     "inaJpYv7vH "
			     "XTrAOm/MeKp+/x6eT4QLru0KoZkvZJnqTI8JyaFTw2OM/"
			     "ItBfh/hL2lm "
			     "Cft2O7n3MfeqYtvjPnY7dWghYW4sVfH7VVEGm958o9nfi7953"
			     "2Qeklxh x8pXWdeAaRU=";

static dns_view_t *view = NULL;

/*
 * Test utilities.  In general, these assume input parameters are valid
 * (checking with assert_int_equal, thus aborting if not) and unlikely run time
 * errors (such as memory allocation failure) won't happen.  This helps keep
 * the test code concise.
 */

/*
 * Utility to convert C-string to dns_name_t.  Return a pointer to
 * static data, and so is not thread safe.
 */
static dns_name_t *
str2name(const char *namestr) {
	static dns_fixedname_t fname;
	static dns_name_t *name;
	static isc_buffer_t namebuf;
	void *deconst_namestr;

	name = dns_fixedname_initname(&fname);
	DE_CONST(namestr, deconst_namestr); /* OK, since we don't modify it */
	isc_buffer_init(&namebuf, deconst_namestr, strlen(deconst_namestr));
	isc_buffer_add(&namebuf, strlen(namestr));
	assert_int_equal(
		dns_name_fromtext(name, &namebuf, dns_rootname, 0, NULL),
		ISC_R_SUCCESS);

	return (name);
}

static void
create_keystruct(uint16_t flags, uint8_t proto, uint8_t alg, const char *keystr,
		 dns_rdata_dnskey_t *keystruct) {
	unsigned char keydata[4096];
	isc_buffer_t keydatabuf;
	isc_region_t r;
	const dns_rdataclass_t rdclass = dns_rdataclass_in;

	keystruct->common.rdclass = rdclass;
	keystruct->common.rdtype = dns_rdatatype_dnskey;
	keystruct->mctx = mctx;
	ISC_LINK_INIT(&keystruct->common, link);
	keystruct->flags = flags;
	keystruct->protocol = proto;
	keystruct->algorithm = alg;

	isc_buffer_init(&keydatabuf, keydata, sizeof(keydata));
	assert_int_equal(isc_base64_decodestring(keystr, &keydatabuf),
			 ISC_R_SUCCESS);
	isc_buffer_usedregion(&keydatabuf, &r);
	keystruct->datalen = r.length;
	keystruct->data = isc_mem_allocate(mctx, r.length);
	memmove(keystruct->data, r.base, r.length);
}

static void
create_dsstruct(dns_name_t *name, uint16_t flags, uint8_t proto, uint8_t alg,
		const char *keystr, unsigned char *digest,
		dns_rdata_ds_t *dsstruct) {
	isc_result_t result;
	unsigned char rrdata[4096];
	isc_buffer_t rrdatabuf;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_dnskey_t dnskey;

	/*
	 * Populate DNSKEY rdata structure.
	 */
	create_keystruct(flags, proto, alg, keystr, &dnskey);

	/*
	 * Convert to wire format.
	 */
	isc_buffer_init(&rrdatabuf, rrdata, sizeof(rrdata));
	result = dns_rdata_fromstruct(&rdata, dnskey.common.rdclass,
				      dnskey.common.rdtype, &dnskey,
				      &rrdatabuf);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Build DS rdata struct.
	 */
	result = dns_ds_fromkeyrdata(name, &rdata, DNS_DSDIGEST_SHA256, digest,
				     dsstruct);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdata_freestruct(&dnskey);
}

/* Common setup: create a keytable and ntatable to test with a few keys */
static void
create_tables(void) {
	unsigned char digest[ISC_MAX_MD_SIZE];
	dns_rdata_ds_t ds;
	dns_fixedname_t fn;
	dns_name_t *keyname = dns_fixedname_name(&fn);
	isc_stdtime_t now;

	assert_int_equal(dns_test_makeview("view", false, &view),
			 ISC_R_SUCCESS);

	assert_int_equal(dns_keytable_create(mctx, &keytable), ISC_R_SUCCESS);
	assert_int_equal(
		dns_ntatable_create(view, taskmgr, timermgr, &ntatable),
		ISC_R_SUCCESS);

	/* Add a normal key */
	dns_test_namefromstring("example.com", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr1, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, false, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);

	/* Add an initializing managed key */
	dns_test_namefromstring("managed.com", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr1, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, true, true, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);

	/* Add a null key */
	assert_int_equal(
		dns_keytable_marksecure(keytable, str2name("null.example")),
		ISC_R_SUCCESS);

	/* Add a negative trust anchor, duration 1 hour */
	isc_stdtime_get(&now);
	assert_int_equal(dns_ntatable_add(ntatable,
					  str2name("insecure.example"), false,
					  now, 3600),
			 ISC_R_SUCCESS);
}

static void
destroy_tables(void) {
	if (ntatable != NULL) {
		dns_ntatable_detach(&ntatable);
	}
	if (keytable != NULL) {
		dns_keytable_detach(&keytable);
	}

	dns_view_detach(&view);
}

/* add keys to the keytable */
ISC_RUN_TEST_IMPL(dns_keytable_add) {
	dns_keynode_t *keynode = NULL;
	dns_keynode_t *null_keynode = NULL;
	unsigned char digest[ISC_MAX_MD_SIZE];
	dns_rdata_ds_t ds;
	dns_fixedname_t fn;
	dns_name_t *keyname = dns_fixedname_name(&fn);

	UNUSED(state);

	create_tables();

	/*
	 * Getting the keynode for the example.com key should succeed.
	 */
	assert_int_equal(
		dns_keytable_find(keytable, str2name("example.com"), &keynode),
		ISC_R_SUCCESS);

	/*
	 * Try to add the same key.  This should have no effect but
	 * report success.
	 */
	dns_test_namefromstring("example.com", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr1, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, false, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("example.com"), &keynode),
		ISC_R_SUCCESS);

	/* Add another key (different keydata) */
	dns_keytable_detachkeynode(keytable, &keynode);
	create_dsstruct(keyname, 257, 3, 5, keystr2, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, false, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("example.com"), &keynode),
		ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Get the keynode for the managed.com key. Ensure the
	 * retrieved key is an initializing key, then mark it as trusted using
	 * dns_keynode_trust() and ensure the latter works as expected.
	 */
	assert_int_equal(
		dns_keytable_find(keytable, str2name("managed.com"), &keynode),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keynode_trust(keynode);
	assert_int_equal(dns_keynode_initial(keynode), false);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for managed.com, marking it as an
	 * initializing key. Since there is already a trusted key at the
	 * node, the node should *not* be marked as initializing.
	 */
	dns_test_namefromstring("managed.com", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr2, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, true, true, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("managed.com"), &keynode),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), false);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add the same managed key again, but this time mark it as a
	 * non-initializing key.  Ensure the previously added key is upgraded
	 * to a non-initializing key and make sure there are still two key
	 * nodes for managed.com, both containing non-initializing keys.
	 */
	assert_int_equal(dns_keytable_add(keytable, true, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("managed.com"), &keynode),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), false);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a managed key at a new node, two.com, marking it as an
	 * initializing key.
	 */
	dns_test_namefromstring("two.com", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr1, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, true, true, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("two.com"), &keynode),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for two.com, marking it as a
	 * non-initializing key. Since there is already an iniitalizing
	 * trust anchor for two.com and we haven't run dns_keynode_trust(),
	 * the initialization status should not change.
	 */
	create_dsstruct(keyname, 257, 3, 5, keystr2, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, true, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("two.com"), &keynode),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a normal key to a name that has a null key.  The null key node
	 * will be updated with the normal key.
	 */
	assert_int_equal(dns_keytable_find(keytable, str2name("null.example"),
					   &null_keynode),
			 ISC_R_SUCCESS);
	dns_test_namefromstring("null.example", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr2, digest, &ds);
	assert_int_equal(dns_keytable_add(keytable, false, false, keyname, &ds,
					  NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("null.example"), &keynode),
		ISC_R_SUCCESS);
	assert_ptr_equal(keynode, null_keynode); /* should be the same node */
	dns_keytable_detachkeynode(keytable, &null_keynode);

	/*
	 * Try to add a null key to a name that already has a key.  It's
	 * effectively no-op, so the same key node is still there.
	 * (Note: this and above checks confirm that if a name has a null key
	 * that's the only key for the name).
	 */
	assert_int_equal(
		dns_keytable_marksecure(keytable, str2name("null.example")),
		ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("null.example"),
					   &null_keynode),
			 ISC_R_SUCCESS);
	assert_ptr_equal(keynode, null_keynode);
	dns_keytable_detachkeynode(keytable, &null_keynode);

	dns_keytable_detachkeynode(keytable, &keynode);
	destroy_tables();
}

/* delete keys from the keytable */
ISC_RUN_TEST_IMPL(dns_keytable_delete) {
	create_tables();

	/* dns_keytable_delete requires exact match */
	assert_int_equal(dns_keytable_delete(keytable, str2name("example.org"),
					     NULL, NULL),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("s.example.com"), NULL,
					     NULL),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_delete(keytable, str2name("example.com"),
					     NULL, NULL),
			 ISC_R_SUCCESS);

	/* works also for nodes with a null key */
	assert_int_equal(dns_keytable_delete(keytable, str2name("null.example"),
					     NULL, NULL),
			 ISC_R_SUCCESS);

	/* or a negative trust anchor */
	assert_int_equal(
		dns_ntatable_delete(ntatable, str2name("insecure.example")),
		ISC_R_SUCCESS);

	destroy_tables();
}

/* delete key nodes from the keytable */
ISC_RUN_TEST_IMPL(dns_keytable_deletekey) {
	dns_rdata_dnskey_t dnskey;
	dns_fixedname_t fn;
	dns_name_t *keyname = dns_fixedname_name(&fn);

	UNUSED(state);

	create_tables();

	/* key name doesn't match */
	dns_test_namefromstring("example.org", &fn);
	create_keystruct(257, 3, 5, keystr1, &dnskey);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 ISC_R_NOTFOUND);
	dns_rdata_freestruct(&dnskey);

	/* subdomain match is the same as no match */
	dns_test_namefromstring("sub.example.org", &fn);
	create_keystruct(257, 3, 5, keystr1, &dnskey);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 ISC_R_NOTFOUND);
	dns_rdata_freestruct(&dnskey);

	/* name matches but key doesn't match (resulting in PARTIALMATCH) */
	dns_test_namefromstring("example.com", &fn);
	create_keystruct(257, 3, 5, keystr2, &dnskey);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 DNS_R_PARTIALMATCH);
	dns_rdata_freestruct(&dnskey);

	/*
	 * exact match: should return SUCCESS on the first try, then
	 * PARTIALMATCH on the second (because the name existed but
	 * not a matching key).
	 */
	create_keystruct(257, 3, 5, keystr1, &dnskey);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 DNS_R_PARTIALMATCH);

	/*
	 * after deleting the node, any deletekey or delete attempt should
	 * result in NOTFOUND.
	 */
	assert_int_equal(dns_keytable_delete(keytable, keyname, NULL, NULL),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 ISC_R_NOTFOUND);
	dns_rdata_freestruct(&dnskey);

	/*
	 * A null key node for a name is not deleted when searched by key;
	 * it must be deleted by dns_keytable_delete()
	 */
	dns_test_namefromstring("null.example", &fn);
	create_keystruct(257, 3, 5, keystr1, &dnskey);
	assert_int_equal(dns_keytable_deletekey(keytable, keyname, &dnskey),
			 DNS_R_PARTIALMATCH);
	assert_int_equal(dns_keytable_delete(keytable, keyname, NULL, NULL),
			 ISC_R_SUCCESS);
	dns_rdata_freestruct(&dnskey);

	destroy_tables();
}

/* check find-variant operations */
ISC_RUN_TEST_IMPL(dns_keytable_find) {
	dns_keynode_t *keynode = NULL;
	dns_fixedname_t fname;
	dns_name_t *name;

	UNUSED(state);

	create_tables();

	/*
	 * dns_keytable_find() requires exact name match.  It matches node
	 * that has a null key, too.
	 */
	assert_int_equal(
		dns_keytable_find(keytable, str2name("example.org"), &keynode),
		ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_find(keytable,
					   str2name("sub.example.com"),
					   &keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("example.com"), &keynode),
		ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	assert_int_equal(
		dns_keytable_find(keytable, str2name("null.example"), &keynode),
		ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * dns_keytable_finddeepestmatch() allows partial match.  Also match
	 * nodes with a null key.
	 */
	name = dns_fixedname_initname(&fname);
	assert_int_equal(dns_keytable_finddeepestmatch(
				 keytable, str2name("example.com"), name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("example.com")));
	assert_int_equal(dns_keytable_finddeepestmatch(
				 keytable, str2name("s.example.com"), name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("example.com")));
	assert_int_equal(dns_keytable_finddeepestmatch(
				 keytable, str2name("example.org"), name),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_finddeepestmatch(
				 keytable, str2name("null.example"), name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("null.example")));

	destroy_tables();
}

/* check issecuredomain() */
ISC_RUN_TEST_IMPL(dns_keytable_issecuredomain) {
	bool issecure;
	const char **n;
	const char *names[] = { "example.com", "sub.example.com",
				"null.example", "sub.null.example", NULL };

	UNUSED(state);
	create_tables();

	/*
	 * Domains that are an exact or partial match of a key name are
	 * considered secure.  It's the case even if the key is null
	 * (validation will then fail, but that's actually the intended effect
	 * of installing a null key).
	 */
	for (n = names; *n != NULL; n++) {
		assert_int_equal(dns_keytable_issecuredomain(keytable,
							     str2name(*n), NULL,
							     &issecure),
				 ISC_R_SUCCESS);
		assert_true(issecure);
	}

	/*
	 * If the key table has no entry (not even a null one) for a domain or
	 * any of its ancestors, that domain is considered insecure.
	 */
	assert_int_equal(dns_keytable_issecuredomain(keytable,
						     str2name("example.org"),
						     NULL, &issecure),
			 ISC_R_SUCCESS);
	assert_false(issecure);

	destroy_tables();
}

/* check dns_keytable_dump() */
ISC_RUN_TEST_IMPL(dns_keytable_dump) {
	FILE *f = fopen("/dev/null", "w");

	UNUSED(state);

	create_tables();

	/*
	 * Right now, we only confirm the dump attempt doesn't cause disruption
	 * (so we don't check the dump content).
	 */
	assert_int_equal(dns_keytable_dump(keytable, f), ISC_R_SUCCESS);
	fclose(f);

	destroy_tables();
}

/* check negative trust anchors */
ISC_RUN_TEST_IMPL(dns_keytable_nta) {
	isc_result_t result;
	bool issecure, covered;
	dns_fixedname_t fn;
	dns_name_t *keyname = dns_fixedname_name(&fn);
	unsigned char digest[ISC_MAX_MD_SIZE];
	dns_rdata_ds_t ds;
	dns_view_t *myview = NULL;
	isc_stdtime_t now;

	UNUSED(state);

	result = dns_test_makeview("view", false, &myview);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &myview->task);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_view_initsecroots(myview, mctx);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_view_getsecroots(myview, &keytable);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_view_initntatable(myview, taskmgr, timermgr);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_view_getntatable(myview, &ntatable);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_test_namefromstring("example", &fn);
	create_dsstruct(keyname, 257, 3, 5, keystr1, digest, &ds);
	result = dns_keytable_add(keytable, false, false, keyname, &ds, NULL,
				  NULL),
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_stdtime_get(&now);
	result = dns_ntatable_add(ntatable, str2name("insecure.example"), false,
				  now, 1);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Should be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.secure.example"), now,
					 true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_false(covered);
	assert_true(issecure);

	/* Should not be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.insecure.example"), now,
					 true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(covered);
	assert_false(issecure);

	/* NTA covered */
	covered = dns_view_ntacovers(myview, now, str2name("insecure.example"),
				     dns_rootname);
	assert_true(covered);

	/* Not NTA covered */
	covered = dns_view_ntacovers(myview, now, str2name("secure.example"),
				     dns_rootname);
	assert_false(covered);

	/* As of now + 2, the NTA should be clear */
	result = dns_view_issecuredomain(myview,
					 str2name("test.insecure.example"),
					 now + 2, true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_false(covered);
	assert_true(issecure);

	/* Now check deletion */
	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_false(covered);
	assert_true(issecure);

	result = dns_ntatable_add(ntatable, str2name("new.example"), false, now,
				  3600);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(covered);
	assert_false(issecure);

	result = dns_ntatable_delete(ntatable, str2name("new.example"));
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_false(covered);
	assert_true(issecure);

	/* Clean up */
	dns_ntatable_detach(&ntatable);
	dns_keytable_detach(&keytable);
	dns_view_detach(&myview);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(dns_keytable_add, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_delete, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_deletekey, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_find, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_issecuredomain, setup_managers,
		      teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_dump, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(dns_keytable_nta, setup_managers, teardown_managers)

ISC_TEST_LIST_END

ISC_TEST_MAIN
