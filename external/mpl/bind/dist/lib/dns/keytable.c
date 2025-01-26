/*	$NetBSD: keytable.c,v 1.11 2025/01/26 16:25:23 christos Exp $	*/

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
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/qp.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/view.h>

#define KEYTABLE_MAGIC	   ISC_MAGIC('K', 'T', 'b', 'l')
#define VALID_KEYTABLE(kt) ISC_MAGIC_VALID(kt, KEYTABLE_MAGIC)

#define KEYNODE_MAGIC	  ISC_MAGIC('K', 'N', 'o', 'd')
#define VALID_KEYNODE(kn) ISC_MAGIC_VALID(kn, KEYNODE_MAGIC)

struct dns_keytable {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_rwlock_t rwlock;
	dns_qpmulti_t *table;
};

struct dns_keynode {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_rwlock_t rwlock;
	dns_name_t name;
	dns_rdatalist_t *dslist;
	dns_rdataset_t dsset;
	bool managed;
	bool initial;
};

static dns_keynode_t *
new_keynode(const dns_name_t *name, dns_rdata_ds_t *ds,
	    dns_keytable_t *keytable, bool managed, bool initial);

/* QP trie methods */
static void
qp_attach(void *uctx, void *pval, uint32_t ival);
static void
qp_detach(void *uctx, void *pval, uint32_t ival);
static size_t
qp_makekey(dns_qpkey_t key, void *uctx, void *pval, uint32_t ival);
static void
qp_triename(void *uctx, char *buf, size_t size);

static dns_qpmethods_t qpmethods = {
	qp_attach,
	qp_detach,
	qp_makekey,
	qp_triename,
};

/* rdataset methods */
static void
keynode_disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG);
static isc_result_t
keynode_first(dns_rdataset_t *rdataset);
static isc_result_t
keynode_next(dns_rdataset_t *rdataset);
static void
keynode_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);
static void
keynode_clone(dns_rdataset_t *source, dns_rdataset_t *target DNS__DB_FLARG);

static dns_rdatasetmethods_t methods = {
	.disassociate = keynode_disassociate,
	.first = keynode_first,
	.next = keynode_next,
	.current = keynode_current,
	.clone = keynode_clone,
};

static void
destroy_keynode(dns_keynode_t *knode) {
	dns_rdata_t *rdata = NULL;

	isc_rwlock_destroy(&knode->rwlock);
	if (knode->dslist != NULL) {
		for (rdata = ISC_LIST_HEAD(knode->dslist->rdata); rdata != NULL;
		     rdata = ISC_LIST_HEAD(knode->dslist->rdata))
		{
			ISC_LIST_UNLINK(knode->dslist->rdata, rdata, link);
			isc_mem_put(knode->mctx, rdata->data,
				    DNS_DS_BUFFERSIZE);
			isc_mem_put(knode->mctx, rdata, sizeof(*rdata));
		}

		isc_mem_put(knode->mctx, knode->dslist, sizeof(*knode->dslist));
		knode->dslist = NULL;
	}

	dns_name_free(&knode->name, knode->mctx);
	isc_mem_putanddetach(&knode->mctx, knode, sizeof(dns_keynode_t));
}

ISC_REFCOUNT_IMPL(dns_keynode, destroy_keynode);

void
dns_keytable_create(dns_view_t *view, dns_keytable_t **keytablep) {
	dns_keytable_t *keytable = NULL;

	/*
	 * Create a keytable.
	 */

	REQUIRE(keytablep != NULL && *keytablep == NULL);

	keytable = isc_mem_get(view->mctx, sizeof(*keytable));
	*keytable = (dns_keytable_t){
		.magic = KEYTABLE_MAGIC,
	};

	isc_mem_attach(view->mctx, &keytable->mctx);
	dns_qpmulti_create(view->mctx, &qpmethods, view, &keytable->table);
	isc_refcount_init(&keytable->references, 1);
	*keytablep = keytable;
}

static void
destroy_keytable(dns_keytable_t *keytable) {
	dns_qpread_t qpr;
	dns_qpiter_t iter;
	void *pval = NULL;

	keytable->magic = 0;

	dns_qpmulti_query(keytable->table, &qpr);
	dns_qpiter_init(&qpr, &iter);
	while (dns_qpiter_next(&iter, NULL, &pval, NULL) == ISC_R_SUCCESS) {
		dns_keynode_t *n = pval;
		dns_keynode_detach(&n);
	}
	dns_qpread_destroy(keytable->table, &qpr);

	dns_qpmulti_destroy(&keytable->table);

	isc_mem_putanddetach(&keytable->mctx, keytable, sizeof(*keytable));
}

ISC_REFCOUNT_IMPL(dns_keytable, destroy_keytable);

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
		knode->dsset.keytable.node = knode;
		knode->dsset.keytable.iter = NULL;
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
delete_ds(dns_qp_t *qp, dns_keytable_t *keytable, dns_keynode_t *knode,
	  dns_rdata_ds_t *ds) {
	isc_result_t result;
	dns_rdata_t dsrdata = DNS_RDATA_INIT;
	dns_rdata_t *rdata = NULL;
	dns_keynode_t *newnode = NULL;
	unsigned char data[DNS_DS_BUFFERSIZE];
	bool found = false;
	void *pval = NULL;
	isc_buffer_t b;

	RWLOCK(&knode->rwlock, isc_rwlocktype_read);
	if (knode->dslist == NULL) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);
		return ISC_R_SUCCESS;
	}

	isc_buffer_init(&b, data, DNS_DS_BUFFERSIZE);

	result = dns_rdata_fromstruct(&dsrdata, dns_rdataclass_in,
				      dns_rdatatype_ds, ds, &b);
	if (result != ISC_R_SUCCESS) {
		RWUNLOCK(&knode->rwlock, isc_rwlocktype_write);
		return result;
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
		return DNS_R_PARTIALMATCH;
	}

	/*
	 * Replace knode with a new instance without the DS.
	 */
	newnode = new_keynode(&knode->name, NULL, keytable, knode->managed,
			      knode->initial);
	for (rdata = ISC_LIST_HEAD(knode->dslist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		if (dns_rdata_compare(rdata, &dsrdata) != 0) {
			dns_rdata_ds_t ds0;
			result = dns_rdata_tostruct(rdata, &ds0, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			add_ds(newnode, &ds0, keytable->mctx);
		}
	}

	result = dns_qp_deletename(qp, &knode->name, &pval, NULL);
	INSIST(result == ISC_R_SUCCESS);
	INSIST(pval == knode);

	result = dns_qp_insert(qp, newnode, 0);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	RWUNLOCK(&knode->rwlock, isc_rwlocktype_read);

	dns_keynode_detach(&knode);
	return ISC_R_SUCCESS;
}

/*%
 * Create a keynode for "ds" (or a null key node if "ds" is NULL), set
 * "managed" and "initial" as requested and attach the keynode to
 * to "node" in "keytable".
 */
static dns_keynode_t *
new_keynode(const dns_name_t *name, dns_rdata_ds_t *ds,
	    dns_keytable_t *keytable, bool managed, bool initial) {
	dns_keynode_t *knode = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(!initial || managed);

	knode = isc_mem_get(keytable->mctx, sizeof(dns_keynode_t));
	*knode = (dns_keynode_t){ .name = DNS_NAME_INITEMPTY,
				  .magic = KEYNODE_MAGIC };

	dns_rdataset_init(&knode->dsset);
	isc_refcount_init(&knode->references, 1);
	isc_rwlock_init(&knode->rwlock);

	dns_name_dupwithoffsets(name, keytable->mctx, &knode->name);

	/*
	 * If a DS was supplied, initialize an rdatalist.
	 */
	if (ds != NULL) {
		add_ds(knode, ds, keytable->mctx);
	}

	isc_mem_attach(keytable->mctx, &knode->mctx);
	knode->managed = managed;
	knode->initial = initial;

	return knode;
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
       const dns_name_t *keyname, dns_rdata_ds_t *ds,
       dns_keytable_callback_t callback, void *callback_arg) {
	isc_result_t result;
	dns_keynode_t *newnode = NULL;
	dns_qp_t *qp = NULL;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));

	dns_qpmulti_write(keytable->table, &qp);

	result = dns_qp_getname(qp, keyname, &pval, NULL);
	if (result != ISC_R_SUCCESS) {
		/*
		 * There was no match for "keyname" in "keytable" yet, so one
		 * was created.  Create a new key node for the supplied
		 * trust anchor (or a null key node if "ds" is NULL)
		 * and insert it.
		 */
		newnode = new_keynode(keyname, ds, keytable, managed, initial);
		result = dns_qp_insert(qp, newnode, 0);
		if (callback != NULL) {
			(*callback)(keyname, callback_arg);
		}
	} else {
		/*
		 * A node already exists for "keyname" in "keytable".
		 */
		if (ds != NULL) {
			dns_keynode_t *knode = pval;
			add_ds(knode, ds, keytable->mctx);
		}
		result = ISC_R_SUCCESS;
	}

	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(keytable->table, &qp);

	return result;
}

isc_result_t
dns_keytable_add(dns_keytable_t *keytable, bool managed, bool initial,
		 dns_name_t *name, dns_rdata_ds_t *ds,
		 dns_keytable_callback_t callback, void *callback_arg) {
	REQUIRE(ds != NULL);
	REQUIRE(!initial || managed);

	return insert(keytable, managed, initial, name, ds, callback,
		      callback_arg);
}

isc_result_t
dns_keytable_marksecure(dns_keytable_t *keytable, const dns_name_t *name) {
	return insert(keytable, true, false, name, NULL, NULL, NULL);
}

isc_result_t
dns_keytable_delete(dns_keytable_t *keytable, const dns_name_t *keyname,
		    dns_keytable_callback_t callback, void *callback_arg) {
	isc_result_t result;
	dns_qp_t *qp = NULL;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(keyname != NULL);

	dns_qpmulti_write(keytable->table, &qp);
	result = dns_qp_deletename(qp, keyname, &pval, NULL);
	if (result == ISC_R_SUCCESS) {
		dns_keynode_t *n = pval;
		if (callback != NULL) {
			(*callback)(keyname, callback_arg);
		}
		dns_keynode_detach(&n);
	}
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(keytable->table, &qp);

	return result;
}

isc_result_t
dns_keytable_deletekey(dns_keytable_t *keytable, const dns_name_t *keyname,
		       dns_rdata_dnskey_t *dnskey) {
	isc_result_t result;
	dns_keynode_t *knode = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned char data[4096], digest[DNS_DS_BUFFERSIZE];
	dns_rdata_ds_t ds;
	isc_buffer_t b;
	dns_qp_t *qp = NULL;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dnskey != NULL);

	dns_qpmulti_write(keytable->table, &qp);
	result = dns_qp_getname(qp, keyname, &pval, NULL);
	if (result != ISC_R_SUCCESS) {
		goto finish;
	}

	knode = pval;

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

	result = delete_ds(qp, keytable, knode, &ds);

finish:
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(keytable->table, &qp);

	return result;
}

isc_result_t
dns_keytable_find(dns_keytable_t *keytable, const dns_name_t *keyname,
		  dns_keynode_t **keynodep) {
	isc_result_t result;
	dns_qpread_t qpr;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(keyname != NULL);
	REQUIRE(keynodep != NULL && *keynodep == NULL);

	dns_qpmulti_query(keytable->table, &qpr);
	result = dns_qp_getname(&qpr, keyname, &pval, NULL);
	if (result == ISC_R_SUCCESS) {
		dns_keynode_t *knode = pval;
		dns_keynode_attach(knode, keynodep);
	}
	dns_qpread_destroy(keytable->table, &qpr);

	return result;
}

isc_result_t
dns_keytable_finddeepestmatch(dns_keytable_t *keytable, const dns_name_t *name,
			      dns_name_t *foundname) {
	isc_result_t result;
	dns_qpread_t qpr;
	dns_keynode_t *keynode = NULL;
	void *pval = NULL;

	/*
	 * Search for the deepest match in 'keytable'.
	 */

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(foundname != NULL);

	dns_qpmulti_query(keytable->table, &qpr);
	result = dns_qp_lookup(&qpr, name, NULL, NULL, NULL, &pval, NULL);
	keynode = pval;

	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		dns_name_copy(&keynode->name, foundname);
		result = ISC_R_SUCCESS;
	}

	dns_qpread_destroy(keytable->table, &qpr);
	return result;
}

isc_result_t
dns_keytable_issecuredomain(dns_keytable_t *keytable, const dns_name_t *name,
			    dns_name_t *foundname, bool *wantdnssecp) {
	isc_result_t result;
	dns_qpread_t qpr;
	dns_keynode_t *keynode = NULL;
	void *pval = NULL;

	/*
	 * Is 'name' at or beneath a trusted key?
	 */

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(wantdnssecp != NULL);

	dns_qpmulti_query(keytable->table, &qpr);
	result = dns_qp_lookup(&qpr, name, NULL, NULL, NULL, &pval, NULL);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		keynode = pval;
		if (foundname != NULL) {
			dns_name_copy(&keynode->name, foundname);
		}
		*wantdnssecp = true;
		result = ISC_R_SUCCESS;
	} else if (result == ISC_R_NOTFOUND) {
		*wantdnssecp = false;
		result = ISC_R_SUCCESS;
	}

	dns_qpread_destroy(keytable->table, &qpr);

	return result;
}

static isc_result_t
putstr(isc_buffer_t **b, const char *str) {
	isc_result_t result;

	result = isc_buffer_reserve(*b, strlen(str));
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_buffer_putstr(*b, str);
	return ISC_R_SUCCESS;
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
	return result;
}

static isc_result_t
keynode_dslist_totext(dns_keynode_t *keynode, isc_buffer_t **text) {
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];
	char obuf[DNS_NAME_FORMATSIZE + 200];
	dns_rdataset_t dsset;

	dns_rdataset_init(&dsset);
	if (!dns_keynode_dsset(keynode, &dsset)) {
		return ISC_R_SUCCESS;
	}

	dns_name_format(&keynode->name, namebuf, sizeof(namebuf));

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
			return result;
		}
	}
	dns_rdataset_disassociate(&dsset);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_keytable_totext(dns_keytable_t *keytable, isc_buffer_t **text) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_qpread_t qpr;
	dns_qpiter_t iter;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));
	REQUIRE(text != NULL && *text != NULL);

	dns_qpmulti_query(keytable->table, &qpr);
	dns_qpiter_init(&qpr, &iter);

	while (dns_qpiter_next(&iter, NULL, &pval, NULL) == ISC_R_SUCCESS) {
		dns_keynode_t *knode = pval;
		if (knode->dslist != NULL) {
			result = keynode_dslist_totext(knode, text);
			if (result != ISC_R_SUCCESS) {
				break;
			}
		}
	}

	dns_qpread_destroy(keytable->table, &qpr);
	return result;
}

void
dns_keytable_forall(dns_keytable_t *keytable,
		    void (*func)(dns_keytable_t *, dns_keynode_t *,
				 dns_name_t *, void *),
		    void *arg) {
	dns_qpread_t qpr;
	dns_qpiter_t iter;
	void *pval = NULL;

	REQUIRE(VALID_KEYTABLE(keytable));

	dns_qpmulti_query(keytable->table, &qpr);
	dns_qpiter_init(&qpr, &iter);

	while (dns_qpiter_next(&iter, NULL, &pval, NULL) == ISC_R_SUCCESS) {
		dns_keynode_t *knode = pval;
		(*func)(keytable, knode, &knode->name, arg);
	}

	dns_qpread_destroy(keytable->table, &qpr);
}

bool
dns_keynode_dsset(dns_keynode_t *keynode, dns_rdataset_t *rdataset) {
	bool result;

	REQUIRE(VALID_KEYNODE(keynode));
	REQUIRE(rdataset == NULL || DNS_RDATASET_VALID(rdataset));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	if (keynode->dslist != NULL) {
		if (rdataset != NULL) {
			keynode_clone(&keynode->dsset,
				      rdataset DNS__DB_FILELINE);
		}
		result = true;
	} else {
		result = false;
	}
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);
	return result;
}

bool
dns_keynode_managed(dns_keynode_t *keynode) {
	bool managed;

	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	managed = keynode->managed;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	return managed;
}

bool
dns_keynode_initial(dns_keynode_t *keynode) {
	bool initial;

	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	initial = keynode->initial;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	return initial;
}

void
dns_keynode_trust(dns_keynode_t *keynode) {
	REQUIRE(VALID_KEYNODE(keynode));

	RWLOCK(&keynode->rwlock, isc_rwlocktype_write);
	keynode->initial = false;
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_write);
}

static void
keynode_disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG) {
	dns_keynode_t *keynode = NULL;

	rdataset->methods = NULL;
	keynode = rdataset->keytable.node;
	rdataset->keytable.node = NULL;

	dns_keynode_detach(&keynode);
}

static isc_result_t
keynode_first(dns_rdataset_t *rdataset) {
	dns_keynode_t *keynode = NULL;

	keynode = rdataset->keytable.node;
	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	rdataset->keytable.iter = ISC_LIST_HEAD(keynode->dslist->rdata);
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	if (rdataset->keytable.iter == NULL) {
		return ISC_R_NOMORE;
	}

	return ISC_R_SUCCESS;
}

static isc_result_t
keynode_next(dns_rdataset_t *rdataset) {
	dns_keynode_t *keynode = NULL;
	dns_rdata_t *rdata = NULL;

	rdata = rdataset->keytable.iter;
	if (rdata == NULL) {
		return ISC_R_NOMORE;
	}

	keynode = rdataset->keytable.node;
	RWLOCK(&keynode->rwlock, isc_rwlocktype_read);
	rdataset->keytable.iter = ISC_LIST_NEXT(rdata, link);
	RWUNLOCK(&keynode->rwlock, isc_rwlocktype_read);

	if (rdataset->keytable.iter == NULL) {
		return ISC_R_NOMORE;
	}

	return ISC_R_SUCCESS;
}

static void
keynode_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	dns_rdata_t *list_rdata = NULL;

	list_rdata = rdataset->keytable.iter;
	INSIST(list_rdata != NULL);

	dns_rdata_clone(list_rdata, rdata);
}

static void
keynode_clone(dns_rdataset_t *source, dns_rdataset_t *target DNS__DB_FLARG) {
	dns_keynode_t *keynode = NULL;

	keynode = source->keytable.node;
	isc_refcount_increment(&keynode->references);

	*target = *source;
	target->keytable.iter = NULL;
}

static void
qp_attach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_keynode_t *keynode = pval;
	dns_keynode_ref(keynode);
}

static void
qp_detach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_keynode_t *keynode = pval;
	dns_keynode_detach(&keynode);
}

static size_t
qp_makekey(dns_qpkey_t key, void *uctx ISC_ATTR_UNUSED, void *pval,
	   uint32_t ival ISC_ATTR_UNUSED) {
	dns_keynode_t *keynode = pval;
	return dns_qpkey_fromname(key, &keynode->name);
}

static void
qp_triename(void *uctx, char *buf, size_t size) {
	dns_view_t *view = uctx;
	snprintf(buf, size, "view %s secroots table", view->name);
}
