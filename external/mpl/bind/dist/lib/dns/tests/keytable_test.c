/*	$NetBSD: keytable_test.c,v 1.3.2.3 2020/04/13 08:02:57 martin Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/nta.h>
#include <dns/rdataclass.h>
#include <dns/rdatastruct.h>
#include <dns/rootns.h>
#include <dns/view.h>

#include <dst/dst.h>

#include "dnstest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

dns_keytable_t *keytable = NULL;
dns_ntatable_t *ntatable = NULL;

static const char *keystr1 = "BQEAAAABok+vaUC9neRv8yeT/FEGgN7svR8s7VBUVSBd8NsAiV8AlaAg O5FHar3JQd95i/puZos6Vi6at9/JBbN8qVmO2AuiXxVqfxMKxIcy+LEB 0Vw4NaSJ3N3uaVREso6aTSs98H/25MjcwLOr7SFfXA7bGhZatLtYY/xu kp6Km5hMfkE=";
static const dns_keytag_t keytag1 = 30591;

static const char *keystr2 = "BQEAAAABwuHz9Cem0BJ0JQTO7C/a3McR6hMaufljs1dfG/inaJpYv7vH XTrAOm/MeKp+/x6eT4QLru0KoZkvZJnqTI8JyaFTw2OM/ItBfh/hL2lm Cft2O7n3MfeqYtvjPnY7dWghYW4sVfH7VVEGm958o9nfi79532Qeklxh x8pXWdeAaRU=";

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
	assert_int_equal(dns_name_fromtext(name, &namebuf, dns_rootname,
					   0, NULL),
			 ISC_R_SUCCESS);

	return (name);
}

static void
create_key(uint16_t flags, uint8_t proto, uint8_t alg,
	   const char *keynamestr, const char *keystr, dst_key_t **target)
{
	dns_rdata_dnskey_t keystruct;
	unsigned char keydata[4096];
	isc_buffer_t keydatabuf;
	unsigned char rrdata[4096];
	isc_buffer_t rrdatabuf;
	isc_region_t r;
	const dns_rdataclass_t rdclass = dns_rdataclass_in; /* for brevity */

	keystruct.common.rdclass = rdclass;
	keystruct.common.rdtype = dns_rdatatype_dnskey;
	keystruct.mctx = NULL;
	ISC_LINK_INIT(&keystruct.common, link);
	keystruct.flags = flags;
	keystruct.protocol = proto;
	keystruct.algorithm = alg;

	isc_buffer_init(&keydatabuf, keydata, sizeof(keydata));
	isc_buffer_init(&rrdatabuf, rrdata, sizeof(rrdata));
	assert_int_equal(isc_base64_decodestring(keystr, &keydatabuf),
			 ISC_R_SUCCESS);
	isc_buffer_usedregion(&keydatabuf, &r);
	keystruct.datalen = r.length;
	keystruct.data = r.base;
	assert_int_equal(dns_rdata_fromstruct(NULL, keystruct.common.rdclass,
					      keystruct.common.rdtype,
					      &keystruct, &rrdatabuf),
			 ISC_R_SUCCESS);

	assert_int_equal(dst_key_fromdns(str2name(keynamestr), rdclass,
					 &rrdatabuf, mctx, target),
			 ISC_R_SUCCESS);
}

/* Common setup: create a keytable and ntatable to test with a few keys */
static void
create_tables() {
	isc_result_t result;
	dst_key_t *key = NULL;
	isc_stdtime_t now;

	result = dns_test_makeview("view", &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(dns_keytable_create(mctx, &keytable), ISC_R_SUCCESS);
	assert_int_equal(dns_ntatable_create(view, taskmgr, timermgr,
					     &ntatable), ISC_R_SUCCESS);

	/* Add a normal key */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	assert_int_equal(dns_keytable_add(keytable, false, false, &key),
			 ISC_R_SUCCESS);

	/* Add an initializing managed key */
	create_key(257, 3, 5, "managed.com", keystr1, &key);
	assert_int_equal(dns_keytable_add(keytable, true, true, &key),
			 ISC_R_SUCCESS);

	/* Add a null key */
	assert_int_equal(dns_keytable_marksecure(keytable,
						 str2name("null.example")),
			 ISC_R_SUCCESS);

	/* Add a negative trust anchor, duration 1 hour */
	isc_stdtime_get(&now);
	assert_int_equal(dns_ntatable_add(ntatable,
					  str2name("insecure.example"),
					  false, now, 3600),
			 ISC_R_SUCCESS);
}

static void
destroy_tables() {
	if (ntatable != NULL) {
		dns_ntatable_detach(&ntatable);
	}
	if (keytable != NULL) {
		dns_keytable_detach(&keytable);
	}

	dns_view_detach(&view);
}

/* add keys to the keytable */
static void
add_test(void **state) {
	dst_key_t *key = NULL;
	dns_keynode_t *keynode = NULL;
	dns_keynode_t *next_keynode = NULL;
	dns_keynode_t *null_keynode = NULL;

	UNUSED(state);

	create_tables();

	/*
	 * Get the keynode for the example.com key.  There's no other key for
	 * the name, so nextkeynode() should return NOTFOUND.
	 */
	assert_int_equal(dns_keytable_find(keytable, str2name("example.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);

	/*
	 * Try to add the same key.  This should have no effect, so
	 * nextkeynode() should still return NOTFOUND.
	 */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	assert_int_equal(dns_keytable_add(keytable, false, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);

	/* Add another key (different keydata) */
	dns_keytable_detachkeynode(keytable, &keynode);
	create_key(257, 3, 5, "example.com", keystr2, &key);
	assert_int_equal(dns_keytable_add(keytable, false, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("example.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Get the keynode for the managed.com key.  There's no other key for
	 * the name, so nextkeynode() should return NOTFOUND.  Ensure the
	 * retrieved key is an initializing key, then mark it as trusted using
	 * dns_keynode_trust() and ensure the latter works as expected.
	 */
	assert_int_equal(dns_keytable_find(keytable, str2name("managed.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keynode_trust(keynode);
	assert_int_equal(dns_keynode_initial(keynode), false);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for managed.com, marking it as an
	 * initializing key.  Ensure nextkeynode() no longer returns
	 * ISC_R_NOTFOUND and that the added key is an initializing key.
	 */
	create_key(257, 3, 5, "managed.com", keystr2, &key);
	assert_int_equal(dns_keytable_add(keytable, true, true, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("managed.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add the same managed key again, but this time mark it as a
	 * non-initializing key.  Ensure the previously added key is upgraded
	 * to a non-initializing key and make sure there are still two key
	 * nodes for managed.com, both containing non-initializing keys.
	 */
	create_key(257, 3, 5, "managed.com", keystr2, &key);
	assert_int_equal(dns_keytable_add(keytable, true, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("managed.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), false);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	keynode = next_keynode;
	next_keynode = NULL;
	assert_int_equal(dns_keynode_initial(keynode), false);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a managed key at a new node, two.com, marking it as an
	 * initializing key.  Ensure nextkeynode() returns ISC_R_NOTFOUND and
	 * that the added key is an initializing key.
	 */
	create_key(257, 3, 5, "two.com", keystr1, &key);
	assert_int_equal(dns_keytable_add(keytable, true, true, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("two.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keynode_initial(keynode), true);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for two.com, marking it as a
	 * non-initializing key.  Ensure nextkeynode() no longer returns
	 * ISC_R_NOTFOUND and that the added key is not an initializing key.
	 */
	create_key(257, 3, 5, "two.com", keystr2, &key);
	assert_int_equal(dns_keytable_add(keytable, true, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("two.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), false);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add the first managed key again, but this time mark it as a
	 * non-initializing key.  Ensure the previously added key is upgraded
	 * to a non-initializing key and make sure there are still two key
	 * nodes for two.com, both containing non-initializing keys.
	 */
	create_key(257, 3, 5, "two.com", keystr1, &key);
	assert_int_equal(dns_keytable_add(keytable, true, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("two.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_initial(keynode), false);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	keynode = next_keynode;
	next_keynode = NULL;
	assert_int_equal(dns_keynode_initial(keynode), false);
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a normal key to a name that has a null key.  The null key node
	 * will be updated with the normal key.
	 */
	assert_int_equal(dns_keytable_find(keytable, str2name("null.example"),
					   &null_keynode),
			 ISC_R_SUCCESS);
	create_key(257, 3, 5, "null.example", keystr2, &key);
	assert_int_equal(dns_keytable_add(keytable, false, false, &key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("null.example"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(keynode, null_keynode); /* should be the same node */
	assert_non_null(dns_keynode_key(keynode)); /* now have a key */
	dns_keytable_detachkeynode(keytable, &null_keynode);

	/*
	 * Try to add a null key to a name that already has a key.  It's
	 * effectively no-op, so the same key node is still there, with no
	 * no next node.
	 * (Note: this and above checks confirm that if a name has a null key
	 * that's the only key for the name).
	 */
	assert_int_equal(dns_keytable_marksecure(keytable,
						 str2name("null.example")),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_find(keytable, str2name("null.example"),
					   &null_keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(keynode, null_keynode);
	assert_non_null(dns_keynode_key(keynode));
	assert_int_equal(dns_keytable_nextkeynode(keytable, keynode,
						  &next_keynode),
			 ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &null_keynode);

	dns_keytable_detachkeynode(keytable, &keynode);
	destroy_tables();
}

/* delete keys from the keytable */
static void
delete_test(void **state) {
	UNUSED(state);

	create_tables();

	/* dns_keytable_delete requires exact match */
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("example.org")),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("s.example.com")),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("example.com")),
			 ISC_R_SUCCESS);

	/* works also for nodes with a null key */
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("null.example")),
			 ISC_R_SUCCESS);

	/* or a negative trust anchor */
	assert_int_equal(dns_ntatable_delete(ntatable,
					     str2name("insecure.example")),
			 ISC_R_SUCCESS);

	destroy_tables();
}

/* delete key nodes from the keytable */
static void
deletekeynode_test(void **state) {
	dst_key_t *key = NULL;

	UNUSED(state);

	create_tables();

	/* key name doesn't match */
	create_key(257, 3, 5, "example.org", keystr1, &key);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 ISC_R_NOTFOUND);
	dst_key_free(&key);

	/* subdomain match is the same as no match */
	create_key(257, 3, 5, "sub.example.com", keystr1, &key);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 ISC_R_NOTFOUND);
	dst_key_free(&key);

	/* name matches but key doesn't match (resulting in PARTIALMATCH) */
	create_key(257, 3, 5, "example.com", keystr2, &key);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 DNS_R_PARTIALMATCH);
	dst_key_free(&key);

	/*
	 * exact match.  after deleting the node the internal rbt node will be
	 * empty, and any delete or deletekeynode attempt should result in
	 * NOTFOUND.
	 */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_delete(keytable,
					     str2name("example.com")),
			 ISC_R_NOTFOUND);
	dst_key_free(&key);

	/*
	 * A null key node for a name is not deleted when searched by key;
	 * it must be deleted by dns_keytable_delete()
	 */
	create_key(257, 3, 5, "null.example", keystr1, &key);
	assert_int_equal(dns_keytable_deletekeynode(keytable, key),
			 DNS_R_PARTIALMATCH);
	assert_int_equal(dns_keytable_delete(keytable, dst_key_name(key)),
			 ISC_R_SUCCESS);
	dst_key_free(&key);

	destroy_tables();
}

/* check find-variant operations */
static void
find_test(void **state) {
	dns_keynode_t *keynode = NULL;
	dns_fixedname_t fname;
	dns_name_t *name;

	UNUSED(state);

	create_tables();

	/*
	 * dns_keytable_find() requires exact name match.  It matches node
	 * that has a null key, too.
	 */
	assert_int_equal(dns_keytable_find(keytable, str2name("example.org"),
					   &keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_find(keytable,
					   str2name("sub.example.com"),
					   &keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_find(keytable,
					   str2name("example.com"),
					   &keynode),
			 ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	assert_int_equal(dns_keytable_find(keytable,
					   str2name("null.example"),
					   &keynode),
			 ISC_R_SUCCESS);
	assert_int_equal(dns_keynode_key(keynode), NULL);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * dns_keytable_finddeepestmatch() allows partial match.  Also match
	 * nodes with a null key.
	 */
	name = dns_fixedname_initname(&fname);
	assert_int_equal(dns_keytable_finddeepestmatch(keytable,
						       str2name("example.com"),
						       name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("example.com")));
	assert_int_equal(dns_keytable_finddeepestmatch(keytable,
					       str2name("s.example.com"),
					       name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("example.com")));
	assert_int_equal(dns_keytable_finddeepestmatch(keytable,
						       str2name("example.org"),
						       name),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_finddeepestmatch(keytable,
						       str2name("null.example"),
						       name),
			 ISC_R_SUCCESS);
	assert_true(dns_name_equal(name, str2name("null.example")));

	/*
	 * dns_keytable_findkeynode() requires exact name, algorithm, keytag
	 * match.  If algorithm or keytag doesn't match, should result in
	 * PARTIALMATCH.  Same for a node with a null key.
	 */
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("example.org"),
						  5, keytag1, &keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("sub.example.com"),
						  5, keytag1, &keynode),
			 ISC_R_NOTFOUND);
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("example.com"),
						  4, keytag1, &keynode),
			 DNS_R_PARTIALMATCH); /* different algorithm */
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("example.com"),
						  5, keytag1 + 1, &keynode),
			 DNS_R_PARTIALMATCH); /* different keytag */
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("null.example"),
						  5, 0, &keynode),
			 DNS_R_PARTIALMATCH); /* null key */
	assert_int_equal(dns_keytable_findkeynode(keytable,
						  str2name("example.com"),
						  5, keytag1, &keynode),
			 ISC_R_SUCCESS); /* complete match */
	dns_keytable_detachkeynode(keytable, &keynode);

	destroy_tables();
}

/* check issecuredomain() */
static void
issecuredomain_test(void **state) {
	bool issecure;
	const char **n;
	const char *names[] = {"example.com", "sub.example.com",
			       "null.example", "sub.null.example", NULL};

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
							     str2name(*n),
							     NULL,
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
						     NULL,
						     &issecure),
			 ISC_R_SUCCESS);
	assert_false(issecure);

	destroy_tables();
}

/* check dns_keytable_dump() */
static void
dump_test(void **state) {
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
static void
nta_test(void **state) {
	isc_result_t result;
	dst_key_t *key = NULL;
	bool issecure, covered;
	dns_view_t *myview = NULL;
	isc_stdtime_t now;

	UNUSED(state);

	result = dns_test_makeview("view", &myview);
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

	create_key(257, 3, 5, "example", keystr1, &key);
	result = dns_keytable_add(keytable, false, false, &key);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_stdtime_get(&now);
	result = dns_ntatable_add(ntatable,
				  str2name("insecure.example"),
				  false, now, 1);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Should be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.secure.example"),
					 now, true, &covered, &issecure);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_false(covered);
	assert_true(issecure);

	/* Should not be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.insecure.example"),
					 now, true, &covered, &issecure);
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

	result = dns_ntatable_add(ntatable, str2name("new.example"),
				  false, now, 3600);
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

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(add_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(delete_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(deletekeynode_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(find_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(issecuredomain_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(dump_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(nta_test,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
