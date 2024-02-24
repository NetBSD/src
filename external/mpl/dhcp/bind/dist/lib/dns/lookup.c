/*	$NetBSD: lookup.c,v 1.1.2.2 2024/02/24 13:06:59 martin Exp $	*/

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
#include <isc/netaddr.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/events.h>
#include <dns/lookup.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/view.h>

struct dns_lookup {
	/* Unlocked. */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_mutex_t lock;
	dns_rdatatype_t type;
	dns_fixedname_t name;
	/* Locked by lock. */
	unsigned int options;
	isc_task_t *task;
	dns_view_t *view;
	dns_lookupevent_t *event;
	dns_fetch_t *fetch;
	unsigned int restarts;
	bool canceled;
	dns_rdataset_t rdataset;
	dns_rdataset_t sigrdataset;
};

#define LOOKUP_MAGIC	ISC_MAGIC('l', 'o', 'o', 'k')
#define VALID_LOOKUP(l) ISC_MAGIC_VALID((l), LOOKUP_MAGIC)

#define MAX_RESTARTS 16

static void
lookup_find(dns_lookup_t *lookup, dns_fetchevent_t *event);

static void
fetch_done(isc_task_t *task, isc_event_t *event) {
	dns_lookup_t *lookup = event->ev_arg;
	dns_fetchevent_t *fevent;

	UNUSED(task);
	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	REQUIRE(VALID_LOOKUP(lookup));
	REQUIRE(lookup->task == task);
	fevent = (dns_fetchevent_t *)event;
	REQUIRE(fevent->fetch == lookup->fetch);

	lookup_find(lookup, fevent);
}

static isc_result_t
start_fetch(dns_lookup_t *lookup) {
	isc_result_t result;

	/*
	 * The caller must be holding the lookup's lock.
	 */

	REQUIRE(lookup->fetch == NULL);

	result = dns_resolver_createfetch(
		lookup->view->resolver, dns_fixedname_name(&lookup->name),
		lookup->type, NULL, NULL, NULL, NULL, 0, 0, 0, NULL,
		lookup->task, fetch_done, lookup, &lookup->rdataset,
		&lookup->sigrdataset, &lookup->fetch);

	return (result);
}

static isc_result_t
build_event(dns_lookup_t *lookup) {
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_rdataset_t *sigrdataset = NULL;

	name = isc_mem_get(lookup->mctx, sizeof(dns_name_t));
	dns_name_init(name, NULL);
	dns_name_dup(dns_fixedname_name(&lookup->name), lookup->mctx, name);

	if (dns_rdataset_isassociated(&lookup->rdataset)) {
		rdataset = isc_mem_get(lookup->mctx, sizeof(dns_rdataset_t));
		dns_rdataset_init(rdataset);
		dns_rdataset_clone(&lookup->rdataset, rdataset);
	}

	if (dns_rdataset_isassociated(&lookup->sigrdataset)) {
		sigrdataset = isc_mem_get(lookup->mctx, sizeof(dns_rdataset_t));
		dns_rdataset_init(sigrdataset);
		dns_rdataset_clone(&lookup->sigrdataset, sigrdataset);
	}

	lookup->event->name = name;
	lookup->event->rdataset = rdataset;
	lookup->event->sigrdataset = sigrdataset;

	return (ISC_R_SUCCESS);
}

static isc_result_t
view_find(dns_lookup_t *lookup, dns_name_t *foundname) {
	isc_result_t result;
	dns_name_t *name = dns_fixedname_name(&lookup->name);
	dns_rdatatype_t type;

	if (lookup->type == dns_rdatatype_rrsig) {
		type = dns_rdatatype_any;
	} else {
		type = lookup->type;
	}

	result = dns_view_find(lookup->view, name, type, 0, 0, false, false,
			       &lookup->event->db, &lookup->event->node,
			       foundname, &lookup->rdataset,
			       &lookup->sigrdataset);
	return (result);
}

static void
lookup_find(dns_lookup_t *lookup, dns_fetchevent_t *event) {
	isc_result_t result;
	bool want_restart;
	bool send_event;
	dns_name_t *name, *fname, *prefix;
	dns_fixedname_t foundname, fixed;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int nlabels;
	int order;
	dns_namereln_t namereln;
	dns_rdata_cname_t cname;
	dns_rdata_dname_t dname;

	REQUIRE(VALID_LOOKUP(lookup));

	LOCK(&lookup->lock);

	result = ISC_R_SUCCESS;
	name = dns_fixedname_name(&lookup->name);

	do {
		lookup->restarts++;
		want_restart = false;
		send_event = true;

		if (event == NULL && !lookup->canceled) {
			fname = dns_fixedname_initname(&foundname);
			INSIST(!dns_rdataset_isassociated(&lookup->rdataset));
			INSIST(!dns_rdataset_isassociated(
				&lookup->sigrdataset));
			/*
			 * If we have restarted then clear the old node.
			 */
			if (lookup->event->node != NULL) {
				INSIST(lookup->event->db != NULL);
				dns_db_detachnode(lookup->event->db,
						  &lookup->event->node);
			}
			if (lookup->event->db != NULL) {
				dns_db_detach(&lookup->event->db);
			}
			result = view_find(lookup, fname);
			if (result == ISC_R_NOTFOUND) {
				/*
				 * We don't know anything about the name.
				 * Launch a fetch.
				 */
				if (lookup->event->node != NULL) {
					INSIST(lookup->event->db != NULL);
					dns_db_detachnode(lookup->event->db,
							  &lookup->event->node);
				}
				if (lookup->event->db != NULL) {
					dns_db_detach(&lookup->event->db);
				}
				result = start_fetch(lookup);
				if (result == ISC_R_SUCCESS) {
					send_event = false;
				}
				goto done;
			}
		} else if (event != NULL) {
			result = event->result;
			fname = dns_fixedname_name(&event->foundname);
			dns_resolver_destroyfetch(&lookup->fetch);
			INSIST(event->rdataset == &lookup->rdataset);
			INSIST(event->sigrdataset == &lookup->sigrdataset);
		} else {
			fname = NULL; /* Silence compiler warning. */
		}

		/*
		 * If we've been canceled, forget about the result.
		 */
		if (lookup->canceled) {
			result = ISC_R_CANCELED;
		}

		switch (result) {
		case ISC_R_SUCCESS:
			result = build_event(lookup);
			if (event == NULL) {
				break;
			}
			if (event->db != NULL) {
				dns_db_attach(event->db, &lookup->event->db);
			}
			if (event->node != NULL) {
				dns_db_attachnode(lookup->event->db,
						  event->node,
						  &lookup->event->node);
			}
			break;
		case DNS_R_CNAME:
			/*
			 * Copy the CNAME's target into the lookup's
			 * query name and start over.
			 */
			result = dns_rdataset_first(&lookup->rdataset);
			if (result != ISC_R_SUCCESS) {
				break;
			}
			dns_rdataset_current(&lookup->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &cname, NULL);
			dns_rdata_reset(&rdata);
			if (result != ISC_R_SUCCESS) {
				break;
			}
			dns_name_copynf(&cname.cname, name);
			dns_rdata_freestruct(&cname);
			want_restart = true;
			send_event = false;
			break;
		case DNS_R_DNAME:
			namereln = dns_name_fullcompare(name, fname, &order,
							&nlabels);
			INSIST(namereln == dns_namereln_subdomain);
			/*
			 * Get the target name of the DNAME.
			 */
			result = dns_rdataset_first(&lookup->rdataset);
			if (result != ISC_R_SUCCESS) {
				break;
			}
			dns_rdataset_current(&lookup->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &dname, NULL);
			dns_rdata_reset(&rdata);
			if (result != ISC_R_SUCCESS) {
				break;
			}
			/*
			 * Construct the new query name and start over.
			 */
			prefix = dns_fixedname_initname(&fixed);
			dns_name_split(name, nlabels, prefix, NULL);
			result = dns_name_concatenate(prefix, &dname.dname,
						      name, NULL);
			dns_rdata_freestruct(&dname);
			if (result == ISC_R_SUCCESS) {
				want_restart = true;
				send_event = false;
			}
			break;
		default:
			send_event = true;
		}

		if (dns_rdataset_isassociated(&lookup->rdataset)) {
			dns_rdataset_disassociate(&lookup->rdataset);
		}
		if (dns_rdataset_isassociated(&lookup->sigrdataset)) {
			dns_rdataset_disassociate(&lookup->sigrdataset);
		}

	done:
		if (event != NULL) {
			if (event->node != NULL) {
				dns_db_detachnode(event->db, &event->node);
			}
			if (event->db != NULL) {
				dns_db_detach(&event->db);
			}
			isc_event_free(ISC_EVENT_PTR(&event));
		}

		/*
		 * Limit the number of restarts.
		 */
		if (want_restart && lookup->restarts == MAX_RESTARTS) {
			want_restart = false;
			result = ISC_R_QUOTA;
			send_event = true;
		}
	} while (want_restart);

	if (send_event) {
		lookup->event->result = result;
		lookup->event->ev_sender = lookup;
		isc_task_sendanddetach(&lookup->task,
				       (isc_event_t **)(void *)&lookup->event);
		dns_view_detach(&lookup->view);
	}

	UNLOCK(&lookup->lock);
}

static void
levent_destroy(isc_event_t *event) {
	dns_lookupevent_t *levent;
	isc_mem_t *mctx;

	REQUIRE(event->ev_type == DNS_EVENT_LOOKUPDONE);
	mctx = event->ev_destroy_arg;
	levent = (dns_lookupevent_t *)event;

	if (levent->name != NULL) {
		if (dns_name_dynamic(levent->name)) {
			dns_name_free(levent->name, mctx);
		}
		isc_mem_put(mctx, levent->name, sizeof(dns_name_t));
	}
	if (levent->rdataset != NULL) {
		dns_rdataset_disassociate(levent->rdataset);
		isc_mem_put(mctx, levent->rdataset, sizeof(dns_rdataset_t));
	}
	if (levent->sigrdataset != NULL) {
		dns_rdataset_disassociate(levent->sigrdataset);
		isc_mem_put(mctx, levent->sigrdataset, sizeof(dns_rdataset_t));
	}
	if (levent->node != NULL) {
		dns_db_detachnode(levent->db, &levent->node);
	}
	if (levent->db != NULL) {
		dns_db_detach(&levent->db);
	}
	isc_mem_put(mctx, event, event->ev_size);
}

isc_result_t
dns_lookup_create(isc_mem_t *mctx, const dns_name_t *name, dns_rdatatype_t type,
		  dns_view_t *view, unsigned int options, isc_task_t *task,
		  isc_taskaction_t action, void *arg, dns_lookup_t **lookupp) {
	dns_lookup_t *lookup;
	isc_event_t *ievent;

	lookup = isc_mem_get(mctx, sizeof(*lookup));
	lookup->mctx = NULL;
	isc_mem_attach(mctx, &lookup->mctx);
	lookup->options = options;

	ievent = isc_event_allocate(mctx, lookup, DNS_EVENT_LOOKUPDONE, action,
				    arg, sizeof(*lookup->event));
	lookup->event = (dns_lookupevent_t *)ievent;
	lookup->event->ev_destroy = levent_destroy;
	lookup->event->ev_destroy_arg = mctx;
	lookup->event->result = ISC_R_FAILURE;
	lookup->event->name = NULL;
	lookup->event->rdataset = NULL;
	lookup->event->sigrdataset = NULL;
	lookup->event->db = NULL;
	lookup->event->node = NULL;

	lookup->task = NULL;
	isc_task_attach(task, &lookup->task);

	isc_mutex_init(&lookup->lock);

	dns_fixedname_init(&lookup->name);

	dns_name_copynf(name, dns_fixedname_name(&lookup->name));

	lookup->type = type;
	lookup->view = NULL;
	dns_view_attach(view, &lookup->view);
	lookup->fetch = NULL;
	lookup->restarts = 0;
	lookup->canceled = false;
	dns_rdataset_init(&lookup->rdataset);
	dns_rdataset_init(&lookup->sigrdataset);
	lookup->magic = LOOKUP_MAGIC;

	*lookupp = lookup;

	lookup_find(lookup, NULL);

	return (ISC_R_SUCCESS);
}

void
dns_lookup_cancel(dns_lookup_t *lookup) {
	REQUIRE(VALID_LOOKUP(lookup));

	LOCK(&lookup->lock);

	if (!lookup->canceled) {
		lookup->canceled = true;
		if (lookup->fetch != NULL) {
			INSIST(lookup->view != NULL);
			dns_resolver_cancelfetch(lookup->fetch);
		}
	}

	UNLOCK(&lookup->lock);
}

void
dns_lookup_destroy(dns_lookup_t **lookupp) {
	dns_lookup_t *lookup;

	REQUIRE(lookupp != NULL);
	lookup = *lookupp;
	*lookupp = NULL;
	REQUIRE(VALID_LOOKUP(lookup));
	REQUIRE(lookup->event == NULL);
	REQUIRE(lookup->task == NULL);
	REQUIRE(lookup->view == NULL);
	if (dns_rdataset_isassociated(&lookup->rdataset)) {
		dns_rdataset_disassociate(&lookup->rdataset);
	}
	if (dns_rdataset_isassociated(&lookup->sigrdataset)) {
		dns_rdataset_disassociate(&lookup->sigrdataset);
	}

	isc_mutex_destroy(&lookup->lock);
	lookup->magic = 0;
	isc_mem_putanddetach(&lookup->mctx, lookup, sizeof(*lookup));
}
