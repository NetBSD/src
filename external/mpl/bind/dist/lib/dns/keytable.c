/*	$NetBSD: keytable.c,v 1.9 2023/06/26 22:03:00 christos Exp $	*/

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

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/rbt.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>

#define KEYTABLE_MAGIC	   ISC_MAGIC('K', 'T', 'b', 'l')
#define VALID_KEYTABLE(kt) ISC_MAGIC_VALID(kt, KEYTABLE_MAGIC)

#define KEYNODE_MAGIC	  ISC_MAGIC('K', 'N', 'o', 'd')
#define VALID_KEYNODE(kn) ISC_MAGIC_VALID(kn, KEYNODE_MAGIC)

struct dns_keytable {
	/* Unlocked. */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_rwlock_t rwlock;
	/* Locked by rwlock. */
	dns_rbt_t *table;
};

struct dns_keynode {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t refcount;
	isc_rwlock_t rwlock;
	dns_rdatalist_t *dslist;
	dns_rdataset_t dsset;
	bool managed;
	bool initial;
};

static dns_keynode_t *
new_keynode(dns_rdata_ds_t *ds, dns_keytable_t *keytable, bool managed,
	    bool initial);

static void
keynode_disassociate(dns_rdataset_t *rdataset);
static isc_result_t
keynode_first(dns_rdataset_t *rdataset);
static isc_result_t
keynode_next(dns_rdataset_t *rdataset);
static void
keynode_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);
static void
keynode_clone(dns_rdataset_t *source, dns_rdataset_t *target);

static dns_rdatasetmethods_t methods = {
	keynode_disassociate,
	keynode_first,
	keynode_next,
	keynode_current,
	keynode_clone,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* settrust */
	NULL, /* expire */
	NULL, /* clearprefetch */
	NULL,
	NULL,
	NULL /* addglue */
};

static void
keynode_attach(dns_keynode_t *source, dns_keynode_t **target) {
	REQUIRE(VALID_KEYNODE(source));
	isc_refcount_increment(&source->refcount);
	*target = source;
}

static void
keynode_detach(isc_mem_t *mctx, dns_keynode_t **keynodep) {
	REQUIRE(keynodep != NULL && VALID_KEYNODE(*keynodep));
	dns_keynode_t *knode = *keynodep;
	*keynodep = NULL;

	if (isc_refcount_decrement(&knode->refcount) == 1) {
		dns_rdata_t *rdata = NULL;
		isc_refcount_destroy(&knode->refcount);
		isc_rwlock_destroy(&knode->rwlock);
		if (knode->dslist != NULL) {
			for (rdata = ISC_LIST_HEAD(knode->dslist->rdata);
			     rdata != NULL;
			     rdata = ISC_LIST_HEAD(knode->dslist->rdata))
			{
				ISC_LIST_UNLINK(knode->dslist->rdata, rdata,
						link);
				isc_mem_put(mctx, rdata->data,
					    DNS_DS_BUFFERSIZE);
				isc_mem_put(mctx, rdata, sizeof(*rdata));
			}

			isc_mem_put(mctx, knode->dslist,
				    sizeof(*knode->dslist));
			knode->dslist = NULL;
		}
		isc_mem_putanddetach(&knode->mctx, knode,
				     sizeof(dns_keynode_t));
	}
}

static void
free_keynode(void *node, void *arg) {
	dns_keynode_t *keynode = node;
	isc_mem_t *mctx = arg;

	keynode_detach(mctx, &keynode);
}

isc_result_t
dns_keytable_create(isc_mem_t *mctx, dns_keytable_t **keytablep) {
	dns_keytable_t *keytable;
	isc_result_t result;

	/*
	 * Create a keytable.
	 */

	REQUIRE(keytablep != NULL && *keytablep == NULL);

	keytable = isc_mem_get(mctx, sizeof(*keytable));

	keytable->table = NULL;
	result = dns_rbt_create(mctx, free_keynode, mctx, &keytable->table);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_keytable;
	}

	isc_rwlock_init(&keytable->rwlock, 0, 0);
	isc_refcount_init(&keytable->references, 1);

	keytable->mctx = NULL;
	isc_mem_attach(mctx, &keytable->mctx);
	keytable->magic = KEYTABLE_MAGIC;
	*keytablep = keytable;

	return (ISC_R_SUCCESS);

cleanup_keytable:
	isc_mem_putanddetach(&mctx, keytable, sizeof(*keytable));

	return (result);
}

void
dns_keytable_attach(dns_keytable_t *source, dns_keytable_t **targetp) {
	REQUIRE(VALID_KEYTABLE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
dns_keytable_detach(dns_keytable_t **keytablep) {
	REQUIRE(keytablep != NULL && VALID_KEYTABLE(*keytablep));
	dns_keytable_t *keytable = *keytablep;
	*keytablep = NULL;

	if (isc_refcount_decrement(&keytable->references) == 1) {
		isc_refcount_destroy(&keytable->references);
		dns_rbt_destroy(&keytable->table);
		isc_rwlock_destroy(&keytable->rwlock);
		keytable->magic = 0;
		isc_mem_putanddetach(&keytable->mctx, keytable,
				     sizeof(*keytable));
	}
}

static void
add_ds(dns_keynode_t *knode, dns_rdata_ds_t *ds, isc_mem_t *mctx) {
	isc_result_t result;
	dns_rdata_t *dsrdata = NULL, *rdata = NULL;
	void *data = NULL;
	bool exists = false;
	isc_buffer_t b;

	dsrdata = isc_mem_get(mctx, sizeof(*dsrdata));
	dns_rdata_init(dsrdata);

	data = isc_mem_get(mctx, DNS_DS_BUFFERSIZE);
	isc_buffer_init(&b, data, DNS_DS_BUFFERSIZE);

	result = dns_rdata_fromstruct(dsrdata, dns_rdataclass_in,
				      dns_rdatatype_ds, ds, &b);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	RWLOCK(&knode->rwlock, isc_rwlocktype_write);

	if (knode->dslist == NULL) {
		knode->dslist = isc_mem_get(mctx, sizeof(*knode->dslist));
		dns_rdatalist_init(knode->dslist);
		knode->dslist->rdclass = dns_rdataclass_in;
		knode->dslist->type = dns_rdatatype_ds;

		INSIST(knode->dsset.methods == NULL);
		knode->dsset.methods = &methods;
		knode->dsset.rdclass = knode->dslist->rdclass;
		knode->dsset.type = knode->dslist->type;
		knode->dsset.covers = knode->dslist->covers;
		knode->dsset.ttl = knode->dslist->ttl;
		knode->dsset.private1 = knode;
		knode->dsset.private2 = NULL;
		knode->dsset.private3 = NULL;
		knode->dsset.privateuint4 = 0;
		knode->dsset.private5 = NULL;
		knode->dsset.trust = dns_trust_ultimate;
	}

	for (rdata = ISC_LIST_HEAD(knode->dslist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		if (dns_rdata_compare(rdata, dsrdata) == 0) {
			exists = true;
			break;
		}
	}

	if (exists) {
		isc_mem_put(mctx, dsrdata->data, DNS_DS_BUFFERSIZE);
		isc_mem_put(mctx, dsrdata, sizeof(*dsrdata));
	} else {
		ISC_LIST_APPEND(knode->dslist->rdata, dsrdata, link);
	}

	RWUNLOCK(&knode->rwlock, isc_rwlocktype_write);
}

static isc_result_t
delete_ds(dns_keytable_t *keytable, dns_rbtnode_t *node, dns_rdata_ds_t *ds) {
	dns_keynode_t *knode = node->data;
	isc_result_t result;
	dns_rdata_t dsrdata = DNS_RDATA_INIT;
	dns_rdata_t *rdata = NULL;
	unsigned char data[DNS_DS_BUFFERSIZE];
	bool found = false;
	isc_buffer_t b;

	RWLOCK(&knode->rwlock, isc_rwlocktype_read);
	if (knode->dslist == NULL) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);
		return (ISC_R_SUCCESS);
	}

	isc_buffer_init(&b, data, DNS_DS_BUFFERSIZE);

	result = dns_rdata_fromstruct(&dsrdata, dns_rdataclass_in,
				      dns_rdatatype_ds, ds, &b);
	if (result != ISC_R_SUCCESS) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_write);
		return (result);
	}

	for (rdata = ISC_LIST_HEAD(knode->dslist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		if (dns_rdata_compare(rdata, &dsrdata) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);
		/*
		 * The keyname must have matched or we wouldn't be here,
		 * so we use DNS_R_PARTIALMATCH instead of ISC_R_NOTFOUND.
		 */
		return (DNS_R_PARTIALMATCH);
	}

	/*
	 * Replace knode with a new instance without the DS.
	 */
	node->data = new_keynode(NULL, keytable, knode->managed,
				 knode->initial);
	for (rdata = ISC_LIST_HEAD(knode->dslist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		if (dns_rdata_compare(rdata, &dsrdata) != 0) {
			dns_rdata_ds_t ds0;
			result = dns_rdata_tostruct(rdata, &ds0, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			add_ds(node->data, &ds0, keytable->mctx);
		}
	}
	RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);

	keynode_detach(keytable->mctx, &knode);

	return (ISC_R_SUCCESS);
}

/*%
 * Create a keynode for "ds" (or a null key node if "ds" is NULL), set
 * "managed" and "initial" as requested and attach the keynode to
 * to "node" in "keytable".
 */
static dns_keynode_t *
new_keynode(dns_rdata_ds_t *ds, dns_keytable_t *keytable, bool managed,
	    bool initial) {
	dns_keynode_t *knode = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(!initial || managed);

	knode = isc_mem_get(keytable->mctx, sizeof(dns_keynode_t));
	*knode = (dns_keynode_t){ .magic = KEYNODE_MAGIC };

	dns_rdataset_init(&knode->dsset);
	isc_refcount_init(&knode->refcount, 1);
	isc_rwlock_init(&knode->rwlock, 0, 0);

	/*
	 * If a DS was supplied, initialize an rdatalist.
	 */
	if (ds != NULL) {
		add_ds(knode, ds, keytable->mctx);
	}

	isc_mem_attach(keytable->mctx, &knode->mctx);
	knode->managed = managed;
	knode->initial = initial;

	return (knode);
}

/*%
 * Add key trust anchor "ds" at "keyname" in "keytable".  If an anchor
 * already exists at the requested name does not contain "ds", update it.
 * If "ds" is NULL, add a null key to indicate that "keyname" should be
 * treated as a secure domain without supplying key data which would allow
 * the domain to be validated.
 */
static isc_result_t
insert(dns_keytable_t *keytable, bool managed, bool initial,
       const dns_name_t *keyname, dns_rdata_ds_t *ds) {
	dns_rbtnode_t *node = NULL;
	isc_result_t result;

	REQUIRE(VALID_KEYTABLE(keytable));

	RWLOCK(&keytable->rwlock, isc_rwlocktype_write);

	result = dns_rbt_addnode(keytable->table, keyname, &node);
	if (result == ISC_R_SUCCESS) {
		/*
		 * There was no node for "keyname" in "keytable" yet, so one
		 * was created.  Create a new key node for the supplied
		 * trust anchor (or a null key node if "ds" is NULL)
		 * and attach it to the created node.
		 */
		node->data = new_keynode(ds, keytable, managed, initial);
	} else if (result == ISC_R_EXISTS) {
		/*
		 * A node already exists for "keyname" in "keytable".
		 */
		if (ds != NULL) {
			dns_keynode_t *knode = node->data;
			if (knode == NULL) {
				node->data = new_keynode(ds, keytable, managed,
							 initial);
			} else {
				add_ds(knode, ds, keytable->mctx);
			}
		}
		result = ISC_R_SUCCESS;
	}

	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_write);

	return (result);
}

isc_result_t
dns_keytable_add(dns_keytable_t *keytable, bool managed, bool initial,
		 dns_name_t *name, dns_rdata_ds_t *ds) {
	REQUIRE(ds != NULL);
	REQUIRE(!initial || managed);

	return (insert(keytable, managed, initial, name, ds));
}

isc_result_t
dns_keytable_marksecure(dns_keytable_t *keytable, const dns_name_t *name) {
	return (insert(keytable, true, false, name, NULL));
}

isc_result_t
dns_keytable_delete(dns_keytable_t *keytable, const dns_name_t *keyname) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(keyname != NULL);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_write);
	result = dns_rbt_findnode(keytable->table, keyname, NULL, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);
	if (result == ISC_R_SUCCESS) {
		if (node->data != NULL) {
			result = dns_rbt_deletenode(keytable->table, node,
						    false);
		} else {
			result = ISC_R_NOTFOUND;
		}
	} else if (result == DNS_R_PARTIALMATCH) {
		result = ISC_R_NOTFOUND;
	}
	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_write);

	return (result);
}

isc_result_t
dns_keytable_deletekey(dns_keytable_t *keytable, const dns_name_t *keyname,
		       dns_rdata_dnskey_t *dnskey) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;
	dns_keynode_t *knode = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned char data[4096], digest[DNS_DS_BUFFERSIZE];
	dns_rdata_ds_t ds;
	isc_buffer_t b;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dnskey != NULL);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_write);
	result = dns_rbt_findnode(keytable->table, keyname, NULL, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);

	if (result == DNS_R_PARTIALMATCH) {
		result = ISC_R_NOTFOUND;
	}
	if (result != ISC_R_SUCCESS) {
		goto finish;
	}

	if (node->data == NULL) {
		result = ISC_R_NOTFOUND;
		goto finish;
	}

	knode = node->data;

	RWLOCK(&knode->rwlock, isc_rwlocktype_read);
	if (knode->dslist == NULL) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);
		result = DNS_R_PARTIALMATCH;
		goto finish;
	}
	RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);

	isc_buffer_init(&b, data, sizeof(data));
	result = dns_rdata_fromstruct(&rdata, dnskey->common.rdclass,
				      dns_rdatatype_dnskey, dnskey, &b);
	if (result != ISC_R_SUCCESS) {
		goto finish;
	}

	result = dns_ds_fromkeyrdata(keyname, &rdata, DNS_DSDIGEST_SHA256,
				     digest, &ds);
	if (result != ISC_R_SUCCESS) {
		goto finish;
	}

	result = delete_ds(keytable, node, &ds);

finish:
	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_write);
	return (result);
}

isc_result_t
dns_keytable_find(dns_keytable_t *keytable, const dns_name_t *keyname,
		  dns_keynode_t **keynodep) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(keyname != NULL);
	REQUIRE(keynodep != NULL && *keynodep == NULL);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_read);
	result = dns_rbt_findnode(keytable->table, keyname, NULL, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);
	if (result == ISC_R_SUCCESS) {
		if (node->data != NULL) {
			keynode_attach(node->data, keynodep);
		} else {
			result = ISC_R_NOTFOUND;
		}
	} else if (result == DNS_R_PARTIALMATCH) {
		result = ISC_R_NOTFOUND;
	}
	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_read);

	return (result);
}

isc_result_t
dns_keytable_finddeepestmatch(dns_keytable_t *keytable, const dns_name_t *name,
			      dns_name_t *foundname) {
	isc_result_t result;
	void *data;

	/*
	 * Search for the deepest match in 'keytable'.
	 */

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(foundname != NULL);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_read);

	data = NULL;
	result = dns_rbt_findname(keytable->table, name, 0, foundname, &data);

	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		result = ISC_R_SUCCESS;
	}

	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_read);

	return (result);
}

void
dns_keytable_detachkeynode(dns_keytable_t *keytable, dns_keynode_t **keynodep) {
	/*
	 * Give back a keynode found via dns_keytable_findkeynode().
	 */

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(keynodep != NULL && VALID_KEYNODE(*keynodep));

	keynode_detach(keytable->mctx, keynodep);
}

isc_result_t
dns_keytable_issecuredomain(dns_keytable_t *keytable, const dns_name_t *name,
			    dns_name_t *foundname, bool *wantdnssecp) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	/*
	 * Is 'name' at or beneath a trusted key?
	 */

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(wantdnssecp != NULL);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_read);

	result = dns_rbt_findnode(keytable->table, name, foundname, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		INSIST(node->data != NULL);
		*wantdnssecp = true;
		result = ISC_R_SUCCESS;
	} else if (result == ISC_R_NOTFOUND) {
		*wantdnssecp = false;
		result = ISC_R_SUCCESS;
	}

	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_read);

	return (result);
}

static isc_result_t
putstr(isc_buffer_t **b, const char *str) {
	isc_result_t result;

	result = isc_buffer_reserve(b, strlen(str));
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	isc_buffer_putstr(*b, str);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_keytable_dump(dns_keytable_t *keytable, FILE *fp) {
	isc_result_t result;
	isc_buffer_t *text = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(fp != NULL);

	isc_buffer_allocate(keytable->mctx, &text, 4096);

	result = dns_keytable_totext(keytable, &text);

	if (isc_buffer_usedlength(text) != 0) {
		(void)putstr(&text, "\n");
	} else if (result == ISC_R_SUCCESS) {
		(void)putstr(&text, "none");
	} else {
		(void)putstr(&text, "could not dump key table: ");
		(void)putstr(&text, isc_result_totext(result));
	}

	fprintf(fp, "%.*s", (int)isc_buffer_usedlength(text),
		(char *)isc_buffer_base(text));

	isc_buffer_free(&text);
	return (result);
}

static isc_result_t
keynode_dslist_totext(dns_name_t *name, dns_keynode_t *keynode,
		      isc_buffer_t **text) {
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];
	char obuf[DNS_NAME_FORMATSIZE + 200];
	dns_rdataset_t dsset;

	dns_name_format(name, namebuf, sizeof(namebuf));

	dns_rdataset_init(&dsset);
	if (!dns_keynode_dsset(keynode, &dsset)) {
		return (ISC_R_SUCCESS);
	}

	for (result = dns_rdataset_first(&dsset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&dsset))
	{
		char algbuf[DNS_SECALG_FORMATSIZE];
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_ds_t ds;

		dns_rdataset_current(&dsset, &rdata);
		result = dns_rdata_tostruct(&rdata, &ds, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		dns_secalg_format(ds.algorithm, algbuf, sizeof(algbuf));

		RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
		snprintf(obuf, sizeof(obuf), "%s/%s/%d ; %s%s\n", namebuf,
			 algbuf, ds.key_tag,
			 keynode->initial ? "initializing " : "",
			 keynode->managed ? "managed" : "static");
		RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

		result = putstr(text, obuf);
		if (result != ISC_R_SUCCESS) {
			dns_rdataset_disassociate(&dsset);
			return (result);
		}
	}
	dns_rdataset_disassociate(&dsset);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_keytable_totext(dns_keytable_t *keytable, isc_buffer_t **text) {
	isc_result_t result;
	dns_keynode_t *knode;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	dns_name_t *foundname, *origin, *fullname;
	dns_fixedname_t fixedfoundname, fixedorigin, fixedfullname;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(text != NULL && *text != NULL);

	origin = dns_fixedname_initname(&fixedorigin);
	fullname = dns_fixedname_initname(&fixedfullname);
	foundname = dns_fixedname_initname(&fixedfoundname);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_read);
	dns_rbtnodechain_init(&chain);
	result = dns_rbtnodechain_first(&chain, keytable->table, NULL, NULL);
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		if (result == ISC_R_NOTFOUND) {
			result = ISC_R_SUCCESS;
		}
		goto cleanup;
	}
	for (;;) {
		dns_rbtnodechain_current(&chain, foundname, origin, &node);

		knode = node->data;
		if (knode != NULL && knode->dslist != NULL) {
			result = dns_name_concatenate(foundname, origin,
						      fullname, NULL);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}

			result = keynode_dslist_totext(fullname, knode, text);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
		}

		result = dns_rbtnodechain_next(&chain, NULL, NULL);
		if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
			if (result == ISC_R_NOMORE) {
				result = ISC_R_SUCCESS;
			}
			break;
		}
	}

cleanup:
	dns_rbtnodechain_invalidate(&chain);
	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_read);
	return (result);
}

isc_result_t
dns_keytable_forall(dns_keytable_t *keytable,
		    void (*func)(dns_keytable_t *, dns_keynode_t *,
				 dns_name_t *, void *),
		    void *arg) {
	isc_result_t result;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	dns_fixedname_t fixedfoundname, fixedorigin, fixedfullname;
	dns_name_t *foundname, *origin, *fullname;

	REQUIRE(VALID_KEYTABLE(keytable));

	origin = dns_fixedname_initname(&fixedorigin);
	fullname = dns_fixedname_initname(&fixedfullname);
	foundname = dns_fixedname_initname(&fixedfoundname);

	RWLOCK(&keytable->rwlock, isc_rwlocktype_read);
	dns_rbtnodechain_init(&chain);
	result = dns_rbtnodechain_first(&chain, keytable->table, NULL, NULL);
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		if (result == ISC_R_NOTFOUND) {
			result = ISC_R_SUCCESS;
		}
		goto cleanup;
	}

	for (;;) {
		dns_rbtnodechain_current(&chain, foundname, origin, &node);
		if (node->data != NULL) {
			result = dns_name_concatenate(foundname, origin,
						      fullname, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			(*func)(keytable, node->data, fullname, arg);
		}
		result = dns_rbtnodechain_next(&chain, NULL, NULL);
		if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
			if (result == ISC_R_NOMORE) {
				result = ISC_R_SUCCESS;
			}
			break;
		}
	}

cleanup:
	dns_rbtnodechain_invalidate(&chain);
	RWUNLOCK(&keytable->rwlock, isc_rwlocktype_read);
	return (result);
}

bool
dns_keynode_dsset(dns_keynode_t *keynode, dns_rdataset_t *rdataset) {
	bool result;
	REQUIRE(VALID_KEYNODE(keynode));
	REQUIRE(rdataset == NULL || DNS_RDATASET_VALID(rdataset));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	if (keynode->dslist != NULL) {
		if (rdataset != NULL) {
			keynode_clone(&keynode->dsset, rdataset);
		}
		result = true;
	} else {
		result = false;
	}
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);
	return (result);
}

bool
dns_keynode_managed(dns_keynode_t *keynode) {
	bool managed;

	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	managed = keynode->managed;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	return (managed);
}

bool
dns_keynode_initial(dns_keynode_t *keynode) {
	bool initial;

	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	initial = keynode->initial;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	return (initial);
}

void
dns_keynode_trust(dns_keynode_t *keynode) {
	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_write);
	keynode->initial = false;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_write);
}

static void
keynode_disassociate(dns_rdataset_t *rdataset) {
	dns_keynode_t *keynode;

	REQUIRE(rdataset != NULL);
	REQUIRE(rdataset->methods == &methods);

	rdataset->methods = NULL;
	keynode = rdataset->private1;
	rdataset->private1 = NULL;

	keynode_detach(keynode->mctx, &keynode);
}

static isc_result_t
keynode_first(dns_rdataset_t *rdataset) {
	dns_keynode_t *keynode;

	REQUIRE(rdataset != NULL);
	REQUIRE(rdataset->methods == &methods);

	keynode = rdataset->private1;
	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	rdataset->private2 = ISC_LIST_HEAD(keynode->dslist->rdata);
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	if (rdataset->private2 == NULL) {
		return (ISC_R_NOMORE);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
keynode_next(dns_rdataset_t *rdataset) {
	dns_keynode_t *keynode;
	dns_rdata_t *rdata;

	REQUIRE(rdataset != NULL);
	REQUIRE(rdataset->methods == &methods);

	rdata = rdataset->private2;
	if (rdata == NULL) {
		return (ISC_R_NOMORE);
	}

	keynode = rdataset->private1;
	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	rdataset->private2 = ISC_LIST_NEXT(rdata, link);
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	if (rdataset->private2 == NULL) {
		return (ISC_R_NOMORE);
	}

	return (ISC_R_SUCCESS);
}

static void
keynode_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	dns_rdata_t *list_rdata;

	REQUIRE(rdataset != NULL);
	REQUIRE(rdataset->methods == &methods);

	list_rdata = rdataset->private2;
	INSIST(list_rdata != NULL);

	dns_rdata_clone(list_rdata, rdata);
}

static void
keynode_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	dns_keynode_t *keynode;

	REQUIRE(source != NULL);
	REQUIRE(target != NULL);
	REQUIRE(source->methods == &methods);

	keynode = source->private1;
	isc_refcount_increment(&keynode->refcount);

	*target = *source;

	/*
	 * Reset iterator state.
	 */
	target->private2 = NULL;
}
