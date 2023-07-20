/*	$NetBSD: nta.c,v 1.9 2023/06/26 22:03:00 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/nta.h>
#include <dns/rbt.h>
#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/time.h>

struct dns_nta {
	unsigned int magic;
	isc_refcount_t refcount;
	dns_ntatable_t *ntatable;
	bool forced;
	isc_timer_t *timer;
	dns_fetch_t *fetch;
	dns_rdataset_t rdataset;
	dns_rdataset_t sigrdataset;
	dns_fixedname_t fn;
	dns_name_t *name;
	isc_stdtime_t expiry;
};

#define NTA_MAGIC     ISC_MAGIC('N', 'T', 'A', 'n')
#define VALID_NTA(nn) ISC_MAGIC_VALID(nn, NTA_MAGIC)

/*
 * Obtain a reference to the nta object.  Released by
 * nta_detach.
 */
static void
nta_ref(dns_nta_t *nta) {
	isc_refcount_increment(&nta->refcount);
}

static void
nta_detach(isc_mem_t *mctx, dns_nta_t **ntap) {
	REQUIRE(ntap != NULL && VALID_NTA(*ntap));
	dns_nta_t *nta = *ntap;
	*ntap = NULL;

	if (isc_refcount_decrement(&nta->refcount) == 1) {
		isc_refcount_destroy(&nta->refcount);
		nta->magic = 0;
		if (nta->timer != NULL) {
			(void)isc_timer_reset(nta->timer,
					      isc_timertype_inactive, NULL,
					      NULL, true);
			isc_timer_destroy(&nta->timer);
		}
		if (dns_rdataset_isassociated(&nta->rdataset)) {
			dns_rdataset_disassociate(&nta->rdataset);
		}
		if (dns_rdataset_isassociated(&nta->sigrdataset)) {
			dns_rdataset_disassociate(&nta->sigrdataset);
		}
		if (nta->fetch != NULL) {
			dns_resolver_cancelfetch(nta->fetch);
			dns_resolver_destroyfetch(&nta->fetch);
		}
		isc_mem_put(mctx, nta, sizeof(dns_nta_t));
	}
}

static void
free_nta(void *data, void *arg) {
	dns_nta_t *nta = (dns_nta_t *)data;
	isc_mem_t *mctx = (isc_mem_t *)arg;

	nta_detach(mctx, &nta);
}

isc_result_t
dns_ntatable_create(dns_view_t *view, isc_taskmgr_t *taskmgr,
		    isc_timermgr_t *timermgr, dns_ntatable_t **ntatablep) {
	dns_ntatable_t *ntatable;
	isc_result_t result;

	REQUIRE(ntatablep != NULL && *ntatablep == NULL);

	ntatable = isc_mem_get(view->mctx, sizeof(*ntatable));

	ntatable->task = NULL;
	result = isc_task_create(taskmgr, 0, &ntatable->task);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_ntatable;
	}
	isc_task_setname(ntatable->task, "ntatable", ntatable);

	ntatable->table = NULL;
	result = dns_rbt_create(view->mctx, free_nta, view->mctx,
				&ntatable->table);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}

	isc_rwlock_init(&ntatable->rwlock, 0, 0);

	ntatable->shuttingdown = false;
	ntatable->timermgr = timermgr;
	ntatable->taskmgr = taskmgr;

	ntatable->view = view;
	isc_refcount_init(&ntatable->references, 1);

	ntatable->magic = NTATABLE_MAGIC;
	*ntatablep = ntatable;

	return (ISC_R_SUCCESS);

cleanup_task:
	isc_task_detach(&ntatable->task);

cleanup_ntatable:
	isc_mem_put(view->mctx, ntatable, sizeof(*ntatable));

	return (result);
}

void
dns_ntatable_attach(dns_ntatable_t *source, dns_ntatable_t **targetp) {
	REQUIRE(VALID_NTATABLE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
dns_ntatable_detach(dns_ntatable_t **ntatablep) {
	dns_ntatable_t *ntatable;

	REQUIRE(ntatablep != NULL && VALID_NTATABLE(*ntatablep));

	ntatable = *ntatablep;
	*ntatablep = NULL;

	if (isc_refcount_decrement(&ntatable->references) == 1) {
		dns_rbt_destroy(&ntatable->table);
		isc_rwlock_destroy(&ntatable->rwlock);
		isc_refcount_destroy(&ntatable->references);
		if (ntatable->task != NULL) {
			isc_task_detach(&ntatable->task);
		}
		ntatable->timermgr = NULL;
		ntatable->taskmgr = NULL;
		ntatable->magic = 0;
		isc_mem_put(ntatable->view->mctx, ntatable, sizeof(*ntatable));
	}
}

static void
fetch_done(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent = (dns_fetchevent_t *)event;
	dns_nta_t *nta = devent->ev_arg;
	isc_result_t eresult = devent->result;
	dns_ntatable_t *ntatable = nta->ntatable;
	dns_view_t *view = ntatable->view;
	isc_stdtime_t now;

	UNUSED(task);

	if (dns_rdataset_isassociated(&nta->rdataset)) {
		dns_rdataset_disassociate(&nta->rdataset);
	}
	if (dns_rdataset_isassociated(&nta->sigrdataset)) {
		dns_rdataset_disassociate(&nta->sigrdataset);
	}
	if (nta->fetch == devent->fetch) {
		nta->fetch = NULL;
	}
	dns_resolver_destroyfetch(&devent->fetch);

	if (devent->node != NULL) {
		dns_db_detachnode(devent->db, &devent->node);
	}
	if (devent->db != NULL) {
		dns_db_detach(&devent->db);
	}

	isc_event_free(&event);
	isc_stdtime_get(&now);

	switch (eresult) {
	case ISC_R_SUCCESS:
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NXDOMAIN:
	case DNS_R_NCACHENXRRSET:
	case DNS_R_NXRRSET:
		if (nta->expiry > now) {
			nta->expiry = now;
		}
		break;
	default:
		break;
	}

	/*
	 * If we're expiring before the next recheck, we might
	 * as well stop the timer now.
	 */
	if (nta->timer != NULL && nta->expiry - now < view->nta_recheck) {
		(void)isc_timer_reset(nta->timer, isc_timertype_inactive, NULL,
				      NULL, true);
	}
	nta_detach(view->mctx, &nta);
	dns_view_weakdetach(&view);
}

static void
checkbogus(isc_task_t *task, isc_event_t *event) {
	dns_nta_t *nta = event->ev_arg;
	dns_ntatable_t *ntatable = nta->ntatable;
	dns_view_t *view = NULL;
	isc_result_t result;

	if (nta->fetch != NULL) {
		dns_resolver_cancelfetch(nta->fetch);
		nta->fetch = NULL;
	}
	if (dns_rdataset_isassociated(&nta->rdataset)) {
		dns_rdataset_disassociate(&nta->rdataset);
	}
	if (dns_rdataset_isassociated(&nta->sigrdataset)) {
		dns_rdataset_disassociate(&nta->sigrdataset);
	}

	isc_event_free(&event);

	nta_ref(nta);
	dns_view_weakattach(ntatable->view, &view);
	result = dns_resolver_createfetch(
		view->resolver, nta->name, dns_rdatatype_nsec, NULL, NULL, NULL,
		NULL, 0, DNS_FETCHOPT_NONTA, 0, NULL, task, fetch_done, nta,
		&nta->rdataset, &nta->sigrdataset, &nta->fetch);
	if (result != ISC_R_SUCCESS) {
		nta_detach(view->mctx, &nta);
		dns_view_weakdetach(&view);
	}
}

static isc_result_t
settimer(dns_ntatable_t *ntatable, dns_nta_t *nta, uint32_t lifetime) {
	isc_result_t result;
	isc_interval_t interval;
	dns_view_t *view;

	REQUIRE(VALID_NTATABLE(ntatable));
	REQUIRE(VALID_NTA(nta));

	if (ntatable->timermgr == NULL) {
		return (ISC_R_SUCCESS);
	}

	view = ntatable->view;
	if (view->nta_recheck == 0 || lifetime <= view->nta_recheck) {
		return (ISC_R_SUCCESS);
	}

	isc_interval_set(&interval, view->nta_recheck, 0);
	result = isc_timer_create(ntatable->timermgr, isc_timertype_ticker,
				  NULL, &interval, ntatable->task, checkbogus,
				  nta, &nta->timer);
	if (result != ISC_R_SUCCESS) {
		isc_timer_destroy(&nta->timer);
	}
	return (result);
}

static isc_result_t
nta_create(dns_ntatable_t *ntatable, const dns_name_t *name,
	   dns_nta_t **target) {
	dns_nta_t *nta = NULL;
	dns_view_t *view;

	REQUIRE(VALID_NTATABLE(ntatable));
	REQUIRE(target != NULL && *target == NULL);

	view = ntatable->view;

	nta = isc_mem_get(view->mctx, sizeof(dns_nta_t));

	nta->ntatable = ntatable;
	nta->expiry = 0;
	nta->timer = NULL;
	nta->fetch = NULL;
	dns_rdataset_init(&nta->rdataset);
	dns_rdataset_init(&nta->sigrdataset);

	isc_refcount_init(&nta->refcount, 1);

	nta->name = dns_fixedname_initname(&nta->fn);
	dns_name_copynf(name, nta->name);

	nta->magic = NTA_MAGIC;

	*target = nta;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_ntatable_add(dns_ntatable_t *ntatable, const dns_name_t *name, bool force,
		 isc_stdtime_t now, uint32_t lifetime) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_nta_t *nta = NULL;
	dns_rbtnode_t *node;
	dns_view_t *view;

	REQUIRE(VALID_NTATABLE(ntatable));

	view = ntatable->view;

	RWLOCK(&ntatable->rwlock, isc_rwlocktype_write);

	if (ntatable->shuttingdown) {
		goto unlock;
	}

	result = nta_create(ntatable, name, &nta);
	if (result != ISC_R_SUCCESS) {
		goto unlock;
	}

	nta->expiry = now + lifetime;
	nta->forced = force;

	node = NULL;
	result = dns_rbt_addnode(ntatable->table, name, &node);
	if (result == ISC_R_SUCCESS) {
		if (!force) {
			(void)settimer(ntatable, nta, lifetime);
		}
		node->data = nta;
		nta = NULL;
	} else if (result == ISC_R_EXISTS) {
		dns_nta_t *n = node->data;
		if (n == NULL) {
			if (!force) {
				(void)settimer(ntatable, nta, lifetime);
			}
			node->data = nta;
			nta = NULL;
		} else {
			n->expiry = nta->expiry;
			nta_detach(view->mctx, &nta);
		}
		result = ISC_R_SUCCESS;
	}

unlock:
	RWUNLOCK(&ntatable->rwlock, isc_rwlocktype_write);

	if (nta != NULL) {
		nta_detach(view->mctx, &nta);
	}

	return (result);
}

/*
 * Caller must hold a write lock on rwlock.
 */
static isc_result_t
deletenode(dns_ntatable_t *ntatable, const dns_name_t *name) {
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	REQUIRE(VALID_NTATABLE(ntatable));
	REQUIRE(name != NULL);

	result = dns_rbt_findnode(ntatable->table, name, NULL, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);
	if (result == ISC_R_SUCCESS) {
		if (node->data != NULL) {
			result = dns_rbt_deletenode(ntatable->table, node,
						    false);
		} else {
			result = ISC_R_NOTFOUND;
		}
	} else if (result == DNS_R_PARTIALMATCH) {
		result = ISC_R_NOTFOUND;
	}

	return (result);
}

isc_result_t
dns_ntatable_delete(dns_ntatable_t *ntatable, const dns_name_t *name) {
	isc_result_t result;

	RWLOCK(&ntatable->rwlock, isc_rwlocktype_write);
	result = deletenode(ntatable, name);
	RWUNLOCK(&ntatable->rwlock, isc_rwlocktype_write);

	return (result);
}

bool
dns_ntatable_covered(dns_ntatable_t *ntatable, isc_stdtime_t now,
		     const dns_name_t *name, const dns_name_t *anchor) {
	isc_result_t result;
	dns_fixedname_t fn;
	dns_rbtnode_t *node;
	dns_name_t *foundname;
	dns_nta_t *nta = NULL;
	bool answer = false;
	isc_rwlocktype_t locktype = isc_rwlocktype_read;

	REQUIRE(ntatable == NULL || VALID_NTATABLE(ntatable));
	REQUIRE(dns_name_isabsolute(name));

	if (ntatable == NULL) {
		return (false);
	}

	foundname = dns_fixedname_initname(&fn);

relock:
	RWLOCK(&ntatable->rwlock, locktype);
again:
	node = NULL;
	result = dns_rbt_findnode(ntatable->table, name, foundname, &node, NULL,
				  DNS_RBTFIND_NOOPTIONS, NULL, NULL);
	if (result == DNS_R_PARTIALMATCH) {
		if (dns_name_issubdomain(foundname, anchor)) {
			result = ISC_R_SUCCESS;
		}
	}
	if (result == ISC_R_SUCCESS) {
		nta = (dns_nta_t *)node->data;
		answer = (nta->expiry > now);
	}

	/* Deal with expired NTA */
	if (result == ISC_R_SUCCESS && !answer) {
		char nb[DNS_NAME_FORMATSIZE];

		if (locktype == isc_rwlocktype_read) {
			RWUNLOCK(&ntatable->rwlock, locktype);
			locktype = isc_rwlocktype_write;
			goto relock;
		}

		dns_name_format(foundname, nb, sizeof(nb));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_NTA, ISC_LOG_INFO,
			      "deleting expired NTA at %s", nb);

		if (nta->timer != NULL) {
			(void)isc_timer_reset(nta->timer,
					      isc_timertype_inactive, NULL,
					      NULL, true);
			isc_timer_destroy(&nta->timer);
		}

		result = deletenode(ntatable, foundname);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_NTA, ISC_LOG_INFO,
				      "deleting NTA failed: %s",
				      isc_result_totext(result));
		}
		goto again;
	}
	RWUNLOCK(&ntatable->rwlock, locktype);

	return (answer);
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
dns_ntatable_totext(dns_ntatable_t *ntatable, const char *view,
		    isc_buffer_t **buf) {
	isc_result_t result;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	bool first = true;
	isc_stdtime_t now;

	REQUIRE(VALID_NTATABLE(ntatable));

	isc_stdtime_get(&now);

	RWLOCK(&ntatable->rwlock, isc_rwlocktype_read);
	dns_rbtnodechain_init(&chain);
	result = dns_rbtnodechain_first(&chain, ntatable->table, NULL, NULL);
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		if (result == ISC_R_NOTFOUND) {
			result = ISC_R_SUCCESS;
		}
		goto cleanup;
	}
	for (;;) {
		dns_rbtnodechain_current(&chain, NULL, NULL, &node);
		if (node->data != NULL) {
			dns_nta_t *n = (dns_nta_t *)node->data;
			char nbuf[DNS_NAME_FORMATSIZE];
			char tbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
			char obuf[DNS_NAME_FORMATSIZE +
				  ISC_FORMATHTTPTIMESTAMP_SIZE +
				  sizeof("expired:  \n")];
			dns_fixedname_t fn;
			dns_name_t *name;
			isc_time_t t;

			/*
			 * Skip "validate-except" entries.
			 */
			if (n->expiry != 0xffffffffU) {
				name = dns_fixedname_initname(&fn);
				dns_rbt_fullnamefromnode(node, name);
				dns_name_format(name, nbuf, sizeof(nbuf));
				isc_time_set(&t, n->expiry, 0);
				isc_time_formattimestamp(&t, tbuf,
							 sizeof(tbuf));

				snprintf(obuf, sizeof(obuf), "%s%s%s%s: %s %s",
					 first ? "" : "\n", nbuf,
					 view != NULL ? "/" : "",
					 view != NULL ? view : "",
					 n->expiry <= now ? "expired"
							  : "expiry",
					 tbuf);
				first = false;
				result = putstr(buf, obuf);
				if (result != ISC_R_SUCCESS) {
					goto cleanup;
				}
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
	RWUNLOCK(&ntatable->rwlock, isc_rwlocktype_read);
	return (result);
}

isc_result_t
dns_ntatable_dump(dns_ntatable_t *ntatable, FILE *fp) {
	isc_result_t result;
	isc_buffer_t *text = NULL;
	int len = 4096;

	isc_buffer_allocate(ntatable->view->mctx, &text, len);

	result = dns_ntatable_totext(ntatable, NULL, &text);

	if (isc_buffer_usedlength(text) != 0) {
		(void)putstr(&text, "\n");
	} else if (result == ISC_R_SUCCESS) {
		(void)putstr(&text, "none");
	} else {
		(void)putstr(&text, "could not dump NTA table: ");
		(void)putstr(&text, isc_result_totext(result));
	}

	fprintf(fp, "%.*s", (int)isc_buffer_usedlength(text),
		(char *)isc_buffer_base(text));
	isc_buffer_free(&text);
	return (result);
}

isc_result_t
dns_ntatable_save(dns_ntatable_t *ntatable, FILE *fp) {
	isc_result_t result;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	isc_stdtime_t now;
	bool written = false;

	REQUIRE(VALID_NTATABLE(ntatable));

	isc_stdtime_get(&now);

	RWLOCK(&ntatable->rwlock, isc_rwlocktype_read);
	dns_rbtnodechain_init(&chain);
	result = dns_rbtnodechain_first(&chain, ntatable->table, NULL, NULL);
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		goto cleanup;
	}

	for (;;) {
		dns_rbtnodechain_current(&chain, NULL, NULL, &node);
		if (node->data != NULL) {
			isc_buffer_t b;
			char nbuf[DNS_NAME_FORMATSIZE + 1], tbuf[80];
			dns_fixedname_t fn;
			dns_name_t *name;
			dns_nta_t *n = (dns_nta_t *)node->data;

			/*
			 * Skip this node if the expiry is already in the
			 * past, or if this is a "validate-except" entry.
			 */
			if (n->expiry <= now || n->expiry == 0xffffffffU) {
				goto skip;
			}

			name = dns_fixedname_initname(&fn);
			dns_rbt_fullnamefromnode(node, name);

			isc_buffer_init(&b, nbuf, sizeof(nbuf));
			result = dns_name_totext(name, false, &b);
			if (result != ISC_R_SUCCESS) {
				goto skip;
			}

			/* Zero terminate. */
			isc_buffer_putuint8(&b, 0);

			isc_buffer_init(&b, tbuf, sizeof(tbuf));
			dns_time32_totext(n->expiry, &b);

			/* Zero terminate. */
			isc_buffer_putuint8(&b, 0);

			fprintf(fp, "%s %s %s\n", nbuf,
				n->forced ? "forced" : "regular", tbuf);
			written = true;
		}
	skip:
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
	RWUNLOCK(&ntatable->rwlock, isc_rwlocktype_read);

	if (result != ISC_R_SUCCESS) {
		return (result);
	} else {
		return (written ? ISC_R_SUCCESS : ISC_R_NOTFOUND);
	}
}

void
dns_ntatable_shutdown(dns_ntatable_t *ntatable) {
	isc_result_t result;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;

	REQUIRE(VALID_NTATABLE(ntatable));

	RWLOCK(&ntatable->rwlock, isc_rwlocktype_write);
	ntatable->shuttingdown = true;

	dns_rbtnodechain_init(&chain);
	result = dns_rbtnodechain_first(&chain, ntatable->table, NULL, NULL);
	while (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
		dns_rbtnodechain_current(&chain, NULL, NULL, &node);
		if (node->data != NULL) {
			dns_nta_t *nta = (dns_nta_t *)node->data;
			if (nta->timer != NULL) {
				(void)isc_timer_reset(nta->timer,
						      isc_timertype_inactive,
						      NULL, NULL, true);
			}
		}
		result = dns_rbtnodechain_next(&chain, NULL, NULL);
	}

	dns_rbtnodechain_invalidate(&chain);
	RWUNLOCK(&ntatable->rwlock, isc_rwlocktype_write);
}
