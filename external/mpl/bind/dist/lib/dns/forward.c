/*	$NetBSD: forward.c,v 1.10 2025/01/26 16:25:22 christos Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/forward.h>
#include <dns/name.h>
#include <dns/qp.h>
#include <dns/types.h>
#include <dns/view.h>

struct dns_fwdtable {
	/* Unlocked. */
	unsigned int magic;
	isc_mem_t *mctx;
	dns_qpmulti_t *table;
};

#define FWDTABLEMAGIC	   ISC_MAGIC('F', 'w', 'd', 'T')
#define VALID_FWDTABLE(ft) ISC_MAGIC_VALID(ft, FWDTABLEMAGIC)

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

void
dns_fwdtable_create(isc_mem_t *mctx, dns_view_t *view,
		    dns_fwdtable_t **fwdtablep) {
	dns_fwdtable_t *fwdtable = NULL;

	REQUIRE(fwdtablep != NULL && *fwdtablep == NULL);

	fwdtable = isc_mem_get(mctx, sizeof(*fwdtable));
	*fwdtable = (dns_fwdtable_t){ .magic = FWDTABLEMAGIC };

	dns_qpmulti_create(mctx, &qpmethods, view, &fwdtable->table);

	isc_mem_attach(mctx, &fwdtable->mctx);
	*fwdtablep = fwdtable;
}

static dns_forwarders_t *
new_forwarders(isc_mem_t *mctx, const dns_name_t *name,
	       dns_fwdpolicy_t fwdpolicy) {
	dns_forwarders_t *forwarders = NULL;

	forwarders = isc_mem_get(mctx, sizeof(*forwarders));
	*forwarders = (dns_forwarders_t){
		.fwdpolicy = fwdpolicy,
		.name = DNS_NAME_INITEMPTY,
		.fwdrs = ISC_LIST_INITIALIZER,
	};
	isc_mem_attach(mctx, &forwarders->mctx);
	isc_refcount_init(&forwarders->references, 1);

	dns_name_dupwithoffsets(name, mctx, &forwarders->name);

	return forwarders;
}

isc_result_t
dns_fwdtable_addfwd(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		    dns_forwarderlist_t *fwdrs, dns_fwdpolicy_t fwdpolicy) {
	isc_result_t result;
	dns_forwarders_t *forwarders = NULL;
	dns_forwarder_t *fwd = NULL, *nfwd = NULL;
	dns_qp_t *qp = NULL;

	REQUIRE(VALID_FWDTABLE(fwdtable));

	forwarders = new_forwarders(fwdtable->mctx, name, fwdpolicy);

	for (fwd = ISC_LIST_HEAD(*fwdrs); fwd != NULL;
	     fwd = ISC_LIST_NEXT(fwd, link))
	{
		nfwd = isc_mem_get(fwdtable->mctx, sizeof(*nfwd));
		*nfwd = *fwd;

		if (fwd->tlsname != NULL) {
			nfwd->tlsname = isc_mem_get(fwdtable->mctx,
						    sizeof(*nfwd->tlsname));
			dns_name_init(nfwd->tlsname, NULL);
			dns_name_dup(fwd->tlsname, fwdtable->mctx,
				     nfwd->tlsname);
		}

		ISC_LINK_INIT(nfwd, link);
		ISC_LIST_APPEND(forwarders->fwdrs, nfwd, link);
	}

	dns_qpmulti_write(fwdtable->table, &qp);
	result = dns_qp_insert(qp, forwarders, 0);
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(fwdtable->table, &qp);

	dns_forwarders_detach(&forwarders);

	return result;
}

isc_result_t
dns_fwdtable_add(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		 isc_sockaddrlist_t *addrs, dns_fwdpolicy_t fwdpolicy) {
	isc_result_t result;
	dns_forwarders_t *forwarders = NULL;
	dns_forwarder_t *fwd = NULL;
	isc_sockaddr_t *sa = NULL;
	dns_qp_t *qp = NULL;

	REQUIRE(VALID_FWDTABLE(fwdtable));

	forwarders = new_forwarders(fwdtable->mctx, name, fwdpolicy);

	for (sa = ISC_LIST_HEAD(*addrs); sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link))
	{
		fwd = isc_mem_get(fwdtable->mctx, sizeof(*fwd));
		*fwd = (dns_forwarder_t){ .addr = *sa,
					  .link = ISC_LINK_INITIALIZER };
		ISC_LIST_APPEND(forwarders->fwdrs, fwd, link);
	}

	dns_qpmulti_write(fwdtable->table, &qp);
	result = dns_qp_insert(qp, forwarders, 0);
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(fwdtable->table, &qp);

	dns_forwarders_detach(&forwarders);

	return result;
}

isc_result_t
dns_fwdtable_find(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		  dns_forwarders_t **forwardersp) {
	isc_result_t result;
	dns_qpread_t qpr;
	void *pval = NULL;

	REQUIRE(VALID_FWDTABLE(fwdtable));

	dns_qpmulti_query(fwdtable->table, &qpr);
	result = dns_qp_lookup(&qpr, name, NULL, NULL, NULL, &pval, NULL);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		dns_forwarders_t *fwdrs = pval;
		*forwardersp = fwdrs;
		dns_forwarders_ref(fwdrs);
	}
	dns_qpread_destroy(fwdtable->table, &qpr);

	return result;
}

void
dns_fwdtable_destroy(dns_fwdtable_t **fwdtablep) {
	dns_fwdtable_t *fwdtable = NULL;

	REQUIRE(fwdtablep != NULL && VALID_FWDTABLE(*fwdtablep));

	fwdtable = *fwdtablep;
	*fwdtablep = NULL;

	dns_qpmulti_destroy(&fwdtable->table);
	fwdtable->magic = 0;

	isc_mem_putanddetach(&fwdtable->mctx, fwdtable, sizeof(*fwdtable));
}

/***
 *** Private
 ***/

static void
destroy_forwarders(dns_forwarders_t *forwarders) {
	dns_forwarder_t *fwd = NULL;

	while (!ISC_LIST_EMPTY(forwarders->fwdrs)) {
		fwd = ISC_LIST_HEAD(forwarders->fwdrs);
		ISC_LIST_UNLINK(forwarders->fwdrs, fwd, link);
		if (fwd->tlsname != NULL) {
			dns_name_free(fwd->tlsname, forwarders->mctx);
			isc_mem_put(forwarders->mctx, fwd->tlsname,
				    sizeof(*fwd->tlsname));
		}
		isc_mem_put(forwarders->mctx, fwd, sizeof(*fwd));
	}
	dns_name_free(&forwarders->name, forwarders->mctx);
	isc_mem_putanddetach(&forwarders->mctx, forwarders,
			     sizeof(*forwarders));
}

#if DNS_FORWARD_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_forwarders, destroy_forwarders);
#else
ISC_REFCOUNT_IMPL(dns_forwarders, destroy_forwarders);
#endif

static void
qp_attach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_forwarders_t *forwarders = pval;
	dns_forwarders_ref(forwarders);
}

static void
qp_detach(void *uctx ISC_ATTR_UNUSED, void *pval,
	  uint32_t ival ISC_ATTR_UNUSED) {
	dns_forwarders_t *forwarders = pval;
	dns_forwarders_detach(&forwarders);
}

static size_t
qp_makekey(dns_qpkey_t key, void *uctx ISC_ATTR_UNUSED, void *pval,
	   uint32_t ival ISC_ATTR_UNUSED) {
	dns_forwarders_t *fwd = pval;
	return dns_qpkey_fromname(key, &fwd->name);
}

static void
qp_triename(void *uctx, char *buf, size_t size) {
	dns_view_t *view = uctx;
	snprintf(buf, size, "view %s forwarder table", view->name);
}
