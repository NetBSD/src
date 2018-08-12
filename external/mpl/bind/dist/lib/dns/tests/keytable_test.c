/*	$NetBSD: keytable_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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

#include <atf-c.h>

#include <unistd.h>
#include <stdio.h>

#if defined(OPENSSL) || defined(PKCS11CRYPTO)

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

dns_keytable_t *keytable = NULL;
dns_ntatable_t *ntatable = NULL;

static const char *keystr1 = "BQEAAAABok+vaUC9neRv8yeT/FEGgN7svR8s7VBUVSBd8NsAiV8AlaAg O5FHar3JQd95i/puZos6Vi6at9/JBbN8qVmO2AuiXxVqfxMKxIcy+LEB 0Vw4NaSJ3N3uaVREso6aTSs98H/25MjcwLOr7SFfXA7bGhZatLtYY/xu kp6Km5hMfkE=";
static const dns_keytag_t keytag1 = 30591;

static const char *keystr2 = "BQEAAAABwuHz9Cem0BJ0JQTO7C/a3McR6hMaufljs1dfG/inaJpYv7vH XTrAOm/MeKp+/x6eT4QLru0KoZkvZJnqTI8JyaFTw2OM/ItBfh/hL2lm Cft2O7n3MfeqYtvjPnY7dWghYW4sVfH7VVEGm958o9nfi79532Qeklxh x8pXWdeAaRU=";

static dns_view_t *view = NULL;

/*
 * Test utilities.  In general, these assume input parameters are valid
 * (checking with ATF_REQUIRE_EQ, thus aborting if not) and unlikely run time
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
	ATF_REQUIRE_EQ(dns_name_fromtext(name, &namebuf, dns_rootname, 0,
					 NULL), ISC_R_SUCCESS);

	return (name);
}

static void
create_key(isc_uint16_t flags, isc_uint8_t proto, isc_uint8_t alg,
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
	ATF_REQUIRE_EQ(isc_base64_decodestring(keystr, &keydatabuf),
		       ISC_R_SUCCESS);
	isc_buffer_usedregion(&keydatabuf, &r);
	keystruct.datalen = r.length;
	keystruct.data = r.base;
	ATF_REQUIRE_EQ(dns_rdata_fromstruct(NULL, keystruct.common.rdclass,
					    keystruct.common.rdtype,
					    &keystruct, &rrdatabuf),
		       ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(dst_key_fromdns(str2name(keynamestr), rdclass,
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
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(dns_keytable_create(mctx, &keytable), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_ntatable_create(view, taskmgr, timermgr,
					   &ntatable), ISC_R_SUCCESS);

	/* Add a normal key */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_FALSE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);

	/* Add an initializing managed key */
	create_key(257, 3, 5, "managed.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_TRUE, &key),
		       ISC_R_SUCCESS);

	/* Add a null key */
	ATF_REQUIRE_EQ(dns_keytable_marksecure(keytable,
					       str2name("null.example")),
		       ISC_R_SUCCESS);

	/* Add a negative trust anchor, duration 1 hour */
	isc_stdtime_get(&now);
	ATF_REQUIRE_EQ(dns_ntatable_add(ntatable,
					str2name("insecure.example"),
					ISC_FALSE, now, 3600),
		       ISC_R_SUCCESS);
}

static void
destroy_tables() {
	if (ntatable != NULL)
		dns_ntatable_detach(&ntatable);
	if (keytable != NULL)
		dns_keytable_detach(&keytable);

	dns_view_detach(&view);
}

/*
 * Individual unit tests
 */

ATF_TC(add);
ATF_TC_HEAD(add, tc) {
	atf_tc_set_md_var(tc, "descr", "add keys to the keytable");
}
ATF_TC_BODY(add, tc) {
	dst_key_t *key = NULL;
	dns_keynode_t *keynode = NULL;
	dns_keynode_t *next_keynode = NULL;
	dns_keynode_t *null_keynode = NULL;

	UNUSED(tc);

	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/*
	 * Get the keynode for the example.com key.  There's no other key for
	 * the name, so nextkeynode() should return NOTFOUND.
	 */
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("example.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);

	/*
	 * Try to add the same key.  This should have no effect, so
	 * nextkeynode() should still return NOTFOUND.
	 */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_FALSE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);

	/* Add another key (different keydata) */
	dns_keytable_detachkeynode(keytable, &keynode);
	create_key(257, 3, 5, "example.com", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_FALSE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("example.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Get the keynode for the managed.com key.  There's no other key for
	 * the name, so nextkeynode() should return NOTFOUND.  Ensure the
	 * retrieved key is an initializing key, then mark it as trusted using
	 * dns_keynode_trust() and ensure the latter works as expected.
	 */
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("managed.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_TRUE);
	dns_keynode_trust(keynode);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for managed.com, marking it as an
	 * initializing key.  Ensure nextkeynode() no longer returns
	 * ISC_R_NOTFOUND and that the added key is an initializing key.
	 */
	create_key(257, 3, 5, "managed.com", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_TRUE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("managed.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_TRUE);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add the same managed key again, but this time mark it as a
	 * non-initializing key.  Ensure the previously added key is upgraded
	 * to a non-initializing key and make sure there are still two key
	 * nodes for managed.com, both containing non-initializing keys.
	 */
	create_key(257, 3, 5, "managed.com", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("managed.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	keynode = next_keynode;
	next_keynode = NULL;
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a managed key at a new node, two.com, marking it as an
	 * initializing key.  Ensure nextkeynode() returns ISC_R_NOTFOUND and
	 * that the added key is an initializing key.
	 */
	create_key(257, 3, 5, "two.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_TRUE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("two.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_TRUE);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a different managed key for two.com, marking it as a
	 * non-initializing key.  Ensure nextkeynode() no longer returns
	 * ISC_R_NOTFOUND and that the added key is not an initializing key.
	 */
	create_key(257, 3, 5, "two.com", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("two.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	dns_keytable_detachkeynode(keytable, &next_keynode);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add the first managed key again, but this time mark it as a
	 * non-initializing key.  Ensure the previously added key is upgraded
	 * to a non-initializing key and make sure there are still two key
	 * nodes for two.com, both containing non-initializing keys.
	 */
	create_key(257, 3, 5, "two.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_TRUE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("two.com"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	keynode = next_keynode;
	next_keynode = NULL;
	ATF_REQUIRE_EQ(dns_keynode_initial(keynode), ISC_FALSE);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * Add a normal key to a name that has a null key.  The null key node
	 * will be updated with the normal key.
	 */
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("null.example"),
					 &null_keynode), ISC_R_SUCCESS);
	create_key(257, 3, 5, "null.example", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_add2(keytable, ISC_FALSE, ISC_FALSE, &key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("null.example"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(keynode, null_keynode); /* should be the same node */
	ATF_REQUIRE(dns_keynode_key(keynode) != NULL); /* now have a key */
	dns_keytable_detachkeynode(keytable, &null_keynode);

	/*
	 * Try to add a null key to a name that already has a key.  It's
	 * effectively no-op, so the same key node is still there, with no
	 * no next node.
	 * (Note: this and above checks confirm that if a name has a null key
	 * that's the only key for the name).
	 */
	ATF_REQUIRE_EQ(dns_keytable_marksecure(keytable,
					       str2name("null.example")),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("null.example"),
					 &null_keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(keynode, null_keynode);
	ATF_REQUIRE(dns_keynode_key(keynode) != NULL);
	ATF_REQUIRE_EQ(dns_keytable_nextkeynode(keytable, keynode,
						&next_keynode), ISC_R_NOTFOUND);
	dns_keytable_detachkeynode(keytable, &null_keynode);

	dns_keytable_detachkeynode(keytable, &keynode);
	destroy_tables();
	dns_test_end();
}

ATF_TC(delete);
ATF_TC_HEAD(delete, tc) {
	atf_tc_set_md_var(tc, "descr", "delete keys from the keytable");
}
ATF_TC_BODY(delete, tc) {
	UNUSED(tc);

	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/* dns_keytable_delete requires exact match */
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, str2name("example.org")),
		       ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, str2name("s.example.com")),
		       ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, str2name("example.com")),
		       ISC_R_SUCCESS);

	/* works also for nodes with a null key */
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, str2name("null.example")),
		       ISC_R_SUCCESS);

	/* or a negative trust anchor */
	ATF_REQUIRE_EQ(dns_ntatable_delete(ntatable,
					   str2name("insecure.example")),
		       ISC_R_SUCCESS);

	destroy_tables();
	dns_test_end();
}

ATF_TC(deletekeynode);
ATF_TC_HEAD(deletekeynode, tc) {
	atf_tc_set_md_var(tc, "descr", "delete key nodes from the keytable");
}
ATF_TC_BODY(deletekeynode, tc) {
	dst_key_t *key = NULL;

	UNUSED(tc);

	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/* key name doesn't match */
	create_key(257, 3, 5, "example.org", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       ISC_R_NOTFOUND);
	dst_key_free(&key);

	/* subdomain match is the same as no match */
	create_key(257, 3, 5, "sub.example.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       ISC_R_NOTFOUND);
	dst_key_free(&key);

	/* name matches but key doesn't match (resulting in PARTIALMATCH) */
	create_key(257, 3, 5, "example.com", keystr2, &key);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       DNS_R_PARTIALMATCH);
	dst_key_free(&key);

	/*
	 * exact match.  after deleting the node the internal rbt node will be
	 * empty, and any delete or deletekeynode attempt should result in
	 * NOTFOUND.
	 */
	create_key(257, 3, 5, "example.com", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, str2name("example.com")),
		       ISC_R_NOTFOUND);
	dst_key_free(&key);

	/*
	 * A null key node for a name is not deleted when searched by key;
	 * it must be deleted by dns_keytable_delete()
	 */
	create_key(257, 3, 5, "null.example", keystr1, &key);
	ATF_REQUIRE_EQ(dns_keytable_deletekeynode(keytable, key),
		       DNS_R_PARTIALMATCH);
	ATF_REQUIRE_EQ(dns_keytable_delete(keytable, dst_key_name(key)),
		       ISC_R_SUCCESS);
	dst_key_free(&key);

	destroy_tables();
	dns_test_end();
}

ATF_TC(find);
ATF_TC_HEAD(find, tc) {
	atf_tc_set_md_var(tc, "descr", "check find-variant operations");
}
ATF_TC_BODY(find, tc) {
	dns_keynode_t *keynode = NULL;
	dns_fixedname_t fname;
	dns_name_t *name;

	UNUSED(tc);

	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/*
	 * dns_keytable_find() requires exact name match.  It matches node
	 * that has a null key, too.
	 */
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("example.org"),
					 &keynode), ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("sub.example.com"),
					 &keynode), ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("example.com"),
					 &keynode), ISC_R_SUCCESS);
	dns_keytable_detachkeynode(keytable, &keynode);
	ATF_REQUIRE_EQ(dns_keytable_find(keytable, str2name("null.example"),
					 &keynode), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_keynode_key(keynode), NULL);
	dns_keytable_detachkeynode(keytable, &keynode);

	/*
	 * dns_keytable_finddeepestmatch() allows partial match.  Also match
	 * nodes with a null key.
	 */
	name = dns_fixedname_initname(&fname);
	ATF_REQUIRE_EQ(dns_keytable_finddeepestmatch(keytable,
						     str2name("example.com"),
						     name), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_name_equal(name, str2name("example.com")), ISC_TRUE);
	ATF_REQUIRE_EQ(dns_keytable_finddeepestmatch(keytable,
						     str2name("s.example.com"),
						     name), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_name_equal(name, str2name("example.com")), ISC_TRUE);
	ATF_REQUIRE_EQ(dns_keytable_finddeepestmatch(keytable,
						     str2name("example.org"),
						     name), ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_finddeepestmatch(keytable,
						     str2name("null.example"),
						     name), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(dns_name_equal(name, str2name("null.example")),
		       ISC_TRUE);

	/*
	 * dns_keytable_findkeynode() requires exact name, algorithm, keytag
	 * match.  If algorithm or keytag doesn't match, should result in
	 * PARTIALMATCH.  Same for a node with a null key.
	 */
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("example.org"),
						5, keytag1, &keynode),
		       ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("sub.example.com"),
						5, keytag1, &keynode),
		       ISC_R_NOTFOUND);
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("example.com"),
						4, keytag1, &keynode),
		       DNS_R_PARTIALMATCH); /* different algorithm */
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("example.com"),
						5, keytag1 + 1, &keynode),
		       DNS_R_PARTIALMATCH); /* different keytag */
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("null.example"),
						5, 0, &keynode),
		       DNS_R_PARTIALMATCH); /* null key */
	ATF_REQUIRE_EQ(dns_keytable_findkeynode(keytable,
						str2name("example.com"),
						5, keytag1, &keynode),
		       ISC_R_SUCCESS); /* complete match */
	dns_keytable_detachkeynode(keytable, &keynode);

	destroy_tables();
	dns_test_end();
}

ATF_TC(issecuredomain);
ATF_TC_HEAD(issecuredomain, tc) {
	atf_tc_set_md_var(tc, "descr", "check issecuredomain()");
}
ATF_TC_BODY(issecuredomain, tc) {
	isc_boolean_t issecure;
	const char **n;
	const char *names[] = {"example.com", "sub.example.com",
			       "null.example", "sub.null.example", NULL};

	UNUSED(tc);
	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/*
	 * Domains that are an exact or partial match of a key name are
	 * considered secure.  It's the case even if the key is null
	 * (validation will then fail, but that's actually the intended effect
	 * of installing a null key).
	 */
	for (n = names; *n != NULL; n++) {
		ATF_REQUIRE_EQ(dns_keytable_issecuredomain(keytable,
							   str2name(*n),
							   NULL,
							   &issecure),
			       ISC_R_SUCCESS);
		ATF_REQUIRE_EQ(issecure, ISC_TRUE);
	}

	/*
	 * If the key table has no entry (not even a null one) for a domain or
	 * any of its ancestors, that domain is considered insecure.
	 */
	ATF_REQUIRE_EQ(dns_keytable_issecuredomain(keytable,
						   str2name("example.org"),
						   NULL,
						   &issecure),
		       ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(issecure, ISC_FALSE);

	destroy_tables();
	dns_test_end();
}

ATF_TC(dump);
ATF_TC_HEAD(dump, tc) {
	atf_tc_set_md_var(tc, "descr", "check dns_keytable_dump()");
}
ATF_TC_BODY(dump, tc) {
	UNUSED(tc);

	ATF_REQUIRE_EQ(dns_test_begin(NULL, ISC_TRUE), ISC_R_SUCCESS);
	create_tables();

	/*
	 * Right now, we only confirm the dump attempt doesn't cause disruption
	 * (so we don't check the dump content).
	 */
	ATF_REQUIRE_EQ(dns_keytable_dump(keytable, stdout), ISC_R_SUCCESS);

	destroy_tables();
	dns_test_end();
}

ATF_TC(nta);
ATF_TC_HEAD(nta, tc) {
	atf_tc_set_md_var(tc, "descr", "check negative trust anchors");
}
ATF_TC_BODY(nta, tc) {
	isc_result_t result;
	dst_key_t *key = NULL;
	isc_boolean_t issecure, covered;
	dns_view_t *myview = NULL;
	isc_stdtime_t now;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_test_makeview("view", &myview);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &myview->task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_view_initsecroots(myview, mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dns_view_getsecroots(myview, &keytable);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_view_initntatable(myview, taskmgr, timermgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dns_view_getntatable(myview, &ntatable);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	create_key(257, 3, 5, "example", keystr1, &key);
	result = dns_keytable_add2(keytable, ISC_FALSE, ISC_FALSE, &key);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_stdtime_get(&now);
	result = dns_ntatable_add(ntatable,
				  str2name("insecure.example"),
				  ISC_FALSE, now, 1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* Should be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.secure.example"),
					 now, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(issecure);

	/* Should not be secure */
	result = dns_view_issecuredomain(myview,
					 str2name("test.insecure.example"),
					 now, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(!issecure);

	/* NTA covered */
	covered = dns_view_ntacovers(myview, now, str2name("insecure.example"),
				     dns_rootname);
	ATF_CHECK(covered);

	/* Not NTA covered */
	covered = dns_view_ntacovers(myview, now, str2name("secure.example"),
				     dns_rootname);
	ATF_CHECK(!covered);

	/* As of now + 2, the NTA should be clear */
	result = dns_view_issecuredomain(myview,
					 str2name("test.insecure.example"),
					 now + 2, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(issecure);

	/* Now check deletion */
	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(issecure);

	result = dns_ntatable_add(ntatable, str2name("new.example"),
				  ISC_FALSE, now, 3600);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(!issecure);

	result = dns_ntatable_delete(ntatable, str2name("new.example"));
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_view_issecuredomain(myview, str2name("test.new.example"),
					 now, ISC_TRUE, &issecure);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(issecure);

	/* Clean up */
	dns_ntatable_detach(&ntatable);
	dns_keytable_detach(&keytable);
	dns_view_detach(&myview);

	dns_test_end();
}

#else
#include <isc/util.h>

ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping keytable test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("DNSSEC not available");
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#if defined(OPENSSL) || defined(PKCS11CRYPTO)
	ATF_TP_ADD_TC(tp, add);
	ATF_TP_ADD_TC(tp, delete);
	ATF_TP_ADD_TC(tp, deletekeynode);
	ATF_TP_ADD_TC(tp, find);
	ATF_TP_ADD_TC(tp, issecuredomain);
	ATF_TP_ADD_TC(tp, dump);
	ATF_TP_ADD_TC(tp, nta);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}
