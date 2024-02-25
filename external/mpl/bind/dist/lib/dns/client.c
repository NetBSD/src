/*	$NetBSD: client.c,v 1.11.2.2 2024/02/25 15:46:48 martin Exp $	*/

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

#include <stdbool.h>
#include <stddef.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/portset.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/sockaddr.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/client.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/forward.h>
#include <dns/keytable.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/request.h>
#include <dns/resolver.h>
#include <dns/tsec.h>
#include <dns/tsig.h>
#include <dns/view.h>

#include <dst/dst.h>

#define DNS_CLIENT_MAGIC    ISC_MAGIC('D', 'N', 'S', 'c')
#define DNS_CLIENT_VALID(c) ISC_MAGIC_VALID(c, DNS_CLIENT_MAGIC)

#define RCTX_MAGIC    ISC_MAGIC('R', 'c', 't', 'x')
#define RCTX_VALID(c) ISC_MAGIC_VALID(c, RCTX_MAGIC)

#define UCTX_MAGIC    ISC_MAGIC('U', 'c', 't', 'x')
#define UCTX_VALID(c) ISC_MAGIC_VALID(c, UCTX_MAGIC)

#define MAX_RESTARTS 16

#ifdef TUNE_LARGE
#define RESOLVER_NTASKS 523
#else /* ifdef TUNE_LARGE */
#define RESOLVER_NTASKS 31
#endif /* TUNE_LARGE */

#define CHECK(r)                             \
	do {                                 \
		result = (r);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

/*%
 * DNS client object
 */
struct dns_client {
	/* Unlocked */
	unsigned int magic;
	unsigned int attributes;
	isc_mutex_t lock;
	isc_mem_t *mctx;
	isc_appctx_t *actx;
	isc_taskmgr_t *taskmgr;
	isc_task_t *task;
	isc_nm_t *nm;
	isc_timermgr_t *timermgr;
	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatch_t *dispatchv4;
	dns_dispatch_t *dispatchv6;

	unsigned int find_timeout;
	unsigned int find_udpretries;

	isc_refcount_t references;

	/* Locked */
	dns_viewlist_t viewlist;
	ISC_LIST(struct resctx) resctxs;
};

#define DEF_FIND_TIMEOUT    5
#define DEF_FIND_UDPRETRIES 3

/*%
 * Internal state for a single name resolution procedure
 */
typedef struct resctx {
	/* Unlocked */
	unsigned int magic;
	isc_mutex_t lock;
	dns_client_t *client;
	bool want_dnssec;
	bool want_validation;
	bool want_cdflag;
	bool want_tcp;

	/* Locked */
	ISC_LINK(struct resctx) link;
	isc_task_t *task;
	dns_view_t *view;
	unsigned int restarts;
	dns_fixedname_t name;
	dns_rdatatype_t type;
	dns_fetch_t *fetch;
	dns_namelist_t namelist;
	isc_result_t result;
	dns_clientresevent_t *event;
	bool canceled;
	dns_rdataset_t *rdataset;
	dns_rdataset_t *sigrdataset;
} resctx_t;

/*%
 * Argument of an internal event for synchronous name resolution.
 */
typedef struct resarg {
	/* Unlocked */
	isc_appctx_t *actx;
	dns_client_t *client;
	isc_mutex_t lock;

	/* Locked */
	isc_result_t result;
	isc_result_t vresult;
	dns_namelist_t *namelist;
	dns_clientrestrans_t *trans;
	bool canceled;
} resarg_t;

static void
client_resfind(resctx_t *rctx, dns_fetchevent_t *event);
static void
cancelresolve(dns_clientrestrans_t *trans);
static void
destroyrestrans(dns_clientrestrans_t **transp);

/*
 * Try honoring the operating system's preferred ephemeral port range.
 */
static isc_result_t
setsourceports(isc_mem_t *mctx, dns_dispatchmgr_t *manager) {
	isc_portset_t *v4portset = NULL, *v6portset = NULL;
	in_port_t udpport_low, udpport_high;
	isc_result_t result;

	result = isc_portset_create(mctx, &v4portset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = isc_net_getudpportrange(AF_INET, &udpport_low, &udpport_high);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	isc_portset_addrange(v4portset, udpport_low, udpport_high);

	result = isc_portset_create(mctx, &v6portset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = isc_net_getudpportrange(AF_INET6, &udpport_low, &udpport_high);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	isc_portset_addrange(v6portset, udpport_low, udpport_high);

	result = dns_dispatchmgr_setavailports(manager, v4portset, v6portset);

cleanup:
	if (v4portset != NULL) {
		isc_portset_destroy(mctx, &v4portset);
	}
	if (v6portset != NULL) {
		isc_portset_destroy(mctx, &v6portset);
	}

	return (result);
}

static isc_result_t
getudpdispatch(int family, dns_dispatchmgr_t *dispatchmgr,
	       dns_dispatch_t **dispp, const isc_sockaddr_t *localaddr) {
	dns_dispatch_t *disp = NULL;
	isc_result_t result;
	isc_sockaddr_t anyaddr;

	if (localaddr == NULL) {
		isc_sockaddr_anyofpf(&anyaddr, family);
		localaddr = &anyaddr;
	}

	result = dns_dispatch_createudp(dispatchmgr, localaddr, &disp);
	if (result == ISC_R_SUCCESS) {
		*dispp = disp;
	}

	return (result);
}

static isc_result_t
createview(isc_mem_t *mctx, dns_rdataclass_t rdclass, isc_taskmgr_t *taskmgr,
	   unsigned int ntasks, isc_nm_t *nm, isc_timermgr_t *timermgr,
	   dns_dispatchmgr_t *dispatchmgr, dns_dispatch_t *dispatchv4,
	   dns_dispatch_t *dispatchv6, dns_view_t **viewp) {
	isc_result_t result;
	dns_view_t *view = NULL;

	result = dns_view_create(mctx, rdclass, DNS_CLIENTVIEW_NAME, &view);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/* Initialize view security roots */
	result = dns_view_initsecroots(view, mctx);
	if (result != ISC_R_SUCCESS) {
		dns_view_detach(&view);
		return (result);
	}

	result = dns_view_createresolver(view, taskmgr, ntasks, 1, nm, timermgr,
					 0, dispatchmgr, dispatchv4,
					 dispatchv6);
	if (result != ISC_R_SUCCESS) {
		dns_view_detach(&view);
		return (result);
	}

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_cache,
			       rdclass, 0, NULL, &view->cachedb);
	if (result != ISC_R_SUCCESS) {
		dns_view_detach(&view);
		return (result);
	}

	*viewp = view;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_client_create(isc_mem_t *mctx, isc_appctx_t *actx, isc_taskmgr_t *taskmgr,
		  isc_nm_t *nm, isc_timermgr_t *timermgr, unsigned int options,
		  dns_client_t **clientp, const isc_sockaddr_t *localaddr4,
		  const isc_sockaddr_t *localaddr6) {
	isc_result_t result;
	dns_client_t *client = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_dispatch_t *dispatchv6 = NULL;
	dns_view_t *view = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(timermgr != NULL);
	REQUIRE(nm != NULL);
	REQUIRE(clientp != NULL && *clientp == NULL);

	UNUSED(options);

	client = isc_mem_get(mctx, sizeof(*client));
	*client = (dns_client_t){
		.actx = actx, .taskmgr = taskmgr, .timermgr = timermgr, .nm = nm
	};

	isc_mutex_init(&client->lock);

	result = isc_task_create(client->taskmgr, 0, &client->task);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	result = dns_dispatchmgr_create(mctx, nm, &client->dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}
	(void)setsourceports(mctx, client->dispatchmgr);

	/*
	 * If only one address family is specified, use it.
	 * If neither family is specified, or if both are, use both.
	 */
	client->dispatchv4 = NULL;
	if (localaddr4 != NULL || localaddr6 == NULL) {
		result = getudpdispatch(AF_INET, client->dispatchmgr,
					&dispatchv4, localaddr4);
		if (result == ISC_R_SUCCESS) {
			client->dispatchv4 = dispatchv4;
		}
	}

	client->dispatchv6 = NULL;
	if (localaddr6 != NULL || localaddr4 == NULL) {
		result = getudpdispatch(AF_INET6, client->dispatchmgr,
					&dispatchv6, localaddr6);
		if (result == ISC_R_SUCCESS) {
			client->dispatchv6 = dispatchv6;
		}
	}

	/* We need at least one of the dispatchers */
	if (dispatchv4 == NULL && dispatchv6 == NULL) {
		INSIST(result != ISC_R_SUCCESS);
		goto cleanup_dispatchmgr;
	}

	isc_refcount_init(&client->references, 1);

	/* Create the default view for class IN */
	result = createview(mctx, dns_rdataclass_in, taskmgr, RESOLVER_NTASKS,
			    nm, timermgr, client->dispatchmgr, dispatchv4,
			    dispatchv6, &view);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_references;
	}

	ISC_LIST_INIT(client->viewlist);
	ISC_LIST_APPEND(client->viewlist, view, link);

	dns_view_freeze(view); /* too early? */

	ISC_LIST_INIT(client->resctxs);

	isc_mem_attach(mctx, &client->mctx);

	client->find_timeout = DEF_FIND_TIMEOUT;
	client->find_udpretries = DEF_FIND_UDPRETRIES;

	client->magic = DNS_CLIENT_MAGIC;

	*clientp = client;

	return (ISC_R_SUCCESS);

cleanup_references:
	isc_refcount_decrementz(&client->references);
	isc_refcount_destroy(&client->references);
cleanup_dispatchmgr:
	if (dispatchv4 != NULL) {
		dns_dispatch_detach(&dispatchv4);
	}
	if (dispatchv6 != NULL) {
		dns_dispatch_detach(&dispatchv6);
	}
	dns_dispatchmgr_detach(&client->dispatchmgr);
cleanup_task:
	isc_task_detach(&client->task);
cleanup_lock:
	isc_mutex_destroy(&client->lock);
	isc_mem_put(mctx, client, sizeof(*client));

	return (result);
}

static void
destroyclient(dns_client_t *client) {
	dns_view_t *view = NULL;

	isc_refcount_destroy(&client->references);

	while ((view = ISC_LIST_HEAD(client->viewlist)) != NULL) {
		ISC_LIST_UNLINK(client->viewlist, view, link);
		dns_view_detach(&view);
	}

	if (client->dispatchv4 != NULL) {
		dns_dispatch_detach(&client->dispatchv4);
	}
	if (client->dispatchv6 != NULL) {
		dns_dispatch_detach(&client->dispatchv6);
	}

	dns_dispatchmgr_detach(&client->dispatchmgr);

	isc_task_detach(&client->task);

	isc_mutex_destroy(&client->lock);
	client->magic = 0;

	isc_mem_putanddetach(&client->mctx, client, sizeof(*client));
}

void
dns_client_detach(dns_client_t **clientp) {
	dns_client_t *client = NULL;

	REQUIRE(clientp != NULL);
	REQUIRE(DNS_CLIENT_VALID(*clientp));

	client = *clientp;
	*clientp = NULL;

	if (isc_refcount_decrement(&client->references) == 1) {
		destroyclient(client);
	}
}

isc_result_t
dns_client_setservers(dns_client_t *client, dns_rdataclass_t rdclass,
		      const dns_name_t *name_space, isc_sockaddrlist_t *addrs) {
	isc_result_t result;
	dns_view_t *view = NULL;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(addrs != NULL);

	if (name_space == NULL) {
		name_space = dns_rootname;
	}

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   rdclass, &view);
	if (result != ISC_R_SUCCESS) {
		UNLOCK(&client->lock);
		return (result);
	}
	UNLOCK(&client->lock);

	result = dns_fwdtable_add(view->fwdtable, name_space, addrs,
				  dns_fwdpolicy_only);

	dns_view_detach(&view);

	return (result);
}

isc_result_t
dns_client_clearservers(dns_client_t *client, dns_rdataclass_t rdclass,
			const dns_name_t *name_space) {
	isc_result_t result;
	dns_view_t *view = NULL;

	REQUIRE(DNS_CLIENT_VALID(client));

	if (name_space == NULL) {
		name_space = dns_rootname;
	}

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   rdclass, &view);
	if (result != ISC_R_SUCCESS) {
		UNLOCK(&client->lock);
		return (result);
	}
	UNLOCK(&client->lock);

	result = dns_fwdtable_delete(view->fwdtable, name_space);

	dns_view_detach(&view);

	return (result);
}

static isc_result_t
getrdataset(isc_mem_t *mctx, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset;

	REQUIRE(mctx != NULL);
	REQUIRE(rdatasetp != NULL && *rdatasetp == NULL);

	rdataset = isc_mem_get(mctx, sizeof(*rdataset));

	dns_rdataset_init(rdataset);

	*rdatasetp = rdataset;

	return (ISC_R_SUCCESS);
}

static void
putrdataset(isc_mem_t *mctx, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset;

	REQUIRE(rdatasetp != NULL);
	rdataset = *rdatasetp;
	*rdatasetp = NULL;
	REQUIRE(rdataset != NULL);

	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}

	isc_mem_put(mctx, rdataset, sizeof(*rdataset));
}

static void
fetch_done(isc_task_t *task, isc_event_t *event) {
	resctx_t *rctx = event->ev_arg;
	dns_fetchevent_t *fevent;

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	REQUIRE(RCTX_VALID(rctx));
	REQUIRE(rctx->task == task);
	fevent = (dns_fetchevent_t *)event;

	client_resfind(rctx, fevent);
}

static isc_result_t
start_fetch(resctx_t *rctx) {
	isc_result_t result;
	int fopts = 0;

	/*
	 * The caller must be holding the rctx's lock.
	 */

	REQUIRE(rctx->fetch == NULL);

	if (!rctx->want_cdflag) {
		fopts |= DNS_FETCHOPT_NOCDFLAG;
	}
	if (!rctx->want_validation) {
		fopts |= DNS_FETCHOPT_NOVALIDATE;
	}
	if (rctx->want_tcp) {
		fopts |= DNS_FETCHOPT_TCP;
	}

	result = dns_resolver_createfetch(
		rctx->view->resolver, dns_fixedname_name(&rctx->name),
		rctx->type, NULL, NULL, NULL, NULL, 0, fopts, 0, NULL,
		rctx->task, fetch_done, rctx, rctx->rdataset, rctx->sigrdataset,
		&rctx->fetch);

	return (result);
}

static isc_result_t
view_find(resctx_t *rctx, dns_db_t **dbp, dns_dbnode_t **nodep,
	  dns_name_t *foundname) {
	isc_result_t result;
	dns_name_t *name = dns_fixedname_name(&rctx->name);
	dns_rdatatype_t type;

	if (rctx->type == dns_rdatatype_rrsig) {
		type = dns_rdatatype_any;
	} else {
		type = rctx->type;
	}

	result = dns_view_find(rctx->view, name, type, 0, 0, false, false, dbp,
			       nodep, foundname, rctx->rdataset,
			       rctx->sigrdataset);

	return (result);
}

static void
client_resfind(resctx_t *rctx, dns_fetchevent_t *event) {
	isc_mem_t *mctx;
	isc_result_t tresult, result = ISC_R_SUCCESS;
	isc_result_t vresult = ISC_R_SUCCESS;
	bool want_restart;
	bool send_event = false;
	dns_name_t *name = NULL, *prefix = NULL;
	dns_fixedname_t foundname, fixed;
	dns_rdataset_t *trdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int nlabels;
	int order;
	dns_namereln_t namereln;
	dns_rdata_cname_t cname;
	dns_rdata_dname_t dname;

	REQUIRE(RCTX_VALID(rctx));

	LOCK(&rctx->lock);

	mctx = rctx->view->mctx;

	name = dns_fixedname_name(&rctx->name);

	do {
		dns_name_t *fname = NULL;
		dns_name_t *ansname = NULL;
		dns_db_t *db = NULL;
		dns_dbnode_t *node = NULL;

		rctx->restarts++;
		want_restart = false;

		if (event == NULL && !rctx->canceled) {
			fname = dns_fixedname_initname(&foundname);
			INSIST(!dns_rdataset_isassociated(rctx->rdataset));
			INSIST(rctx->sigrdataset == NULL ||
			       !dns_rdataset_isassociated(rctx->sigrdataset));
			result = view_find(rctx, &db, &node, fname);
			if (result == ISC_R_NOTFOUND) {
				/*
				 * We don't know anything about the name.
				 * Launch a fetch.
				 */
				if (node != NULL) {
					INSIST(db != NULL);
					dns_db_detachnode(db, &node);
				}
				if (db != NULL) {
					dns_db_detach(&db);
				}
				result = start_fetch(rctx);
				if (result != ISC_R_SUCCESS) {
					putrdataset(mctx, &rctx->rdataset);
					if (rctx->sigrdataset != NULL) {
						putrdataset(mctx,
							    &rctx->sigrdataset);
					}
					send_event = true;
				}
				goto done;
			}
		} else {
			INSIST(event != NULL);
			INSIST(event->fetch == rctx->fetch);
			dns_resolver_destroyfetch(&rctx->fetch);
			db = event->db;
			node = event->node;
			result = event->result;
			vresult = event->vresult;
			fname = event->foundname;
			INSIST(event->rdataset == rctx->rdataset);
			INSIST(event->sigrdataset == rctx->sigrdataset);
		}

		/*
		 * If we've been canceled, forget about the result.
		 */
		if (rctx->canceled) {
			result = ISC_R_CANCELED;
		} else {
			/*
			 * Otherwise, get some resource for copying the
			 * result.
			 */
			dns_name_t *aname = dns_fixedname_name(&rctx->name);

			ansname = isc_mem_get(mctx, sizeof(*ansname));
			dns_name_init(ansname, NULL);

			dns_name_dup(aname, mctx, ansname);
		}

		switch (result) {
		case ISC_R_SUCCESS:
			send_event = true;
			/*
			 * This case is handled in the main line below.
			 */
			break;
		case DNS_R_CNAME:
			/*
			 * Add the CNAME to the answer list.
			 */
			trdataset = rctx->rdataset;
			ISC_LIST_APPEND(ansname->list, rctx->rdataset, link);
			rctx->rdataset = NULL;
			if (rctx->sigrdataset != NULL) {
				ISC_LIST_APPEND(ansname->list,
						rctx->sigrdataset, link);
				rctx->sigrdataset = NULL;
			}
			ISC_LIST_APPEND(rctx->namelist, ansname, link);
			ansname = NULL;

			/*
			 * Copy the CNAME's target into the lookup's
			 * query name and start over.
			 */
			tresult = dns_rdataset_first(trdataset);
			if (tresult != ISC_R_SUCCESS) {
				goto done;
			}
			dns_rdataset_current(trdataset, &rdata);
			tresult = dns_rdata_tostruct(&rdata, &cname, NULL);
			dns_rdata_reset(&rdata);
			if (tresult != ISC_R_SUCCESS) {
				goto done;
			}
			dns_name_copy(&cname.cname, name);
			dns_rdata_freestruct(&cname);
			want_restart = true;
			goto done;
		case DNS_R_DNAME:
			/*
			 * Add the DNAME to the answer list.
			 */
			trdataset = rctx->rdataset;
			ISC_LIST_APPEND(ansname->list, rctx->rdataset, link);
			rctx->rdataset = NULL;
			if (rctx->sigrdataset != NULL) {
				ISC_LIST_APPEND(ansname->list,
						rctx->sigrdataset, link);
				rctx->sigrdataset = NULL;
			}
			ISC_LIST_APPEND(rctx->namelist, ansname, link);
			ansname = NULL;

			namereln = dns_name_fullcompare(name, fname, &order,
							&nlabels);
			INSIST(namereln == dns_namereln_subdomain);
			/*
			 * Get the target name of the DNAME.
			 */
			tresult = dns_rdataset_first(trdataset);
			if (tresult != ISC_R_SUCCESS) {
				result = tresult;
				goto done;
			}
			dns_rdataset_current(trdataset, &rdata);
			tresult = dns_rdata_tostruct(&rdata, &dname, NULL);
			dns_rdata_reset(&rdata);
			if (tresult != ISC_R_SUCCESS) {
				result = tresult;
				goto done;
			}
			/*
			 * Construct the new query name and start over.
			 */
			prefix = dns_fixedname_initname(&fixed);
			dns_name_split(name, nlabels, prefix, NULL);
			tresult = dns_name_concatenate(prefix, &dname.dname,
						       name, NULL);
			dns_rdata_freestruct(&dname);
			if (tresult == ISC_R_SUCCESS) {
				want_restart = true;
			} else {
				result = tresult;
			}
			goto done;
		case DNS_R_NCACHENXDOMAIN:
		case DNS_R_NCACHENXRRSET:
			ISC_LIST_APPEND(ansname->list, rctx->rdataset, link);
			ISC_LIST_APPEND(rctx->namelist, ansname, link);
			ansname = NULL;
			rctx->rdataset = NULL;
			/* What about sigrdataset? */
			if (rctx->sigrdataset != NULL) {
				putrdataset(mctx, &rctx->sigrdataset);
			}
			send_event = true;
			goto done;
		default:
			if (rctx->rdataset != NULL) {
				putrdataset(mctx, &rctx->rdataset);
			}
			if (rctx->sigrdataset != NULL) {
				putrdataset(mctx, &rctx->sigrdataset);
			}
			send_event = true;
			goto done;
		}

		if (rctx->type == dns_rdatatype_any) {
			int n = 0;
			dns_rdatasetiter_t *rdsiter = NULL;

			tresult = dns_db_allrdatasets(db, node, NULL, 0, 0,
						      &rdsiter);
			if (tresult != ISC_R_SUCCESS) {
				result = tresult;
				goto done;
			}

			tresult = dns_rdatasetiter_first(rdsiter);
			while (tresult == ISC_R_SUCCESS) {
				dns_rdatasetiter_current(rdsiter,
							 rctx->rdataset);
				if (rctx->rdataset->type != 0) {
					ISC_LIST_APPEND(ansname->list,
							rctx->rdataset, link);
					n++;
					rctx->rdataset = NULL;
				} else {
					/*
					 * We're not interested in this
					 * rdataset.
					 */
					dns_rdataset_disassociate(
						rctx->rdataset);
				}
				tresult = dns_rdatasetiter_next(rdsiter);

				if (tresult == ISC_R_SUCCESS &&
				    rctx->rdataset == NULL)
				{
					tresult = getrdataset(mctx,
							      &rctx->rdataset);
					if (tresult != ISC_R_SUCCESS) {
						result = tresult;
						POST(result);
						break;
					}
				}
			}
			if (rctx->rdataset != NULL) {
				putrdataset(mctx, &rctx->rdataset);
			}
			if (rctx->sigrdataset != NULL) {
				putrdataset(mctx, &rctx->sigrdataset);
			}
			if (n == 0) {
				/*
				 * We didn't match any rdatasets (which means
				 * something went wrong in this
				 * implementation).
				 */
				result = DNS_R_SERVFAIL; /* better code? */
				POST(result);
			} else {
				ISC_LIST_APPEND(rctx->namelist, ansname, link);
				ansname = NULL;
			}
			dns_rdatasetiter_destroy(&rdsiter);
			if (tresult != ISC_R_NOMORE) {
				result = DNS_R_SERVFAIL; /* ditto */
			} else {
				result = ISC_R_SUCCESS;
			}
			goto done;
		} else {
			/*
			 * This is the "normal" case -- an ordinary question
			 * to which we've got the answer.
			 */
			ISC_LIST_APPEND(ansname->list, rctx->rdataset, link);
			rctx->rdataset = NULL;
			if (rctx->sigrdataset != NULL) {
				ISC_LIST_APPEND(ansname->list,
						rctx->sigrdataset, link);
				rctx->sigrdataset = NULL;
			}
			ISC_LIST_APPEND(rctx->namelist, ansname, link);
			ansname = NULL;
		}

	done:
		/*
		 * Free temporary resources
		 */
		if (ansname != NULL) {
			dns_rdataset_t *rdataset;

			while ((rdataset = ISC_LIST_HEAD(ansname->list)) !=
			       NULL)
			{
				ISC_LIST_UNLINK(ansname->list, rdataset, link);
				putrdataset(mctx, &rdataset);
			}
			dns_name_free(ansname, mctx);
			isc_mem_put(mctx, ansname, sizeof(*ansname));
		}

		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		if (db != NULL) {
			dns_db_detach(&db);
		}
		if (event != NULL) {
			isc_event_free(ISC_EVENT_PTR(&event));
		}

		/*
		 * Limit the number of restarts.
		 */
		if (want_restart && rctx->restarts == MAX_RESTARTS) {
			want_restart = false;
			result = ISC_R_QUOTA;
			send_event = true;
		}

		/*
		 * Prepare further find with new resources
		 */
		if (want_restart) {
			INSIST(rctx->rdataset == NULL &&
			       rctx->sigrdataset == NULL);

			result = getrdataset(mctx, &rctx->rdataset);
			if (result == ISC_R_SUCCESS && rctx->want_dnssec) {
				result = getrdataset(mctx, &rctx->sigrdataset);
				if (result != ISC_R_SUCCESS) {
					putrdataset(mctx, &rctx->rdataset);
				}
			}

			if (result != ISC_R_SUCCESS) {
				want_restart = false;
				send_event = true;
			}
		}
	} while (want_restart);

	if (send_event) {
		isc_task_t *task;

		while ((name = ISC_LIST_HEAD(rctx->namelist)) != NULL) {
			ISC_LIST_UNLINK(rctx->namelist, name, link);
			ISC_LIST_APPEND(rctx->event->answerlist, name, link);
		}

		rctx->event->result = result;
		rctx->event->vresult = vresult;
		task = rctx->event->ev_sender;
		rctx->event->ev_sender = rctx;
		isc_task_sendanddetach(&task, ISC_EVENT_PTR(&rctx->event));
	}

	UNLOCK(&rctx->lock);
}

static void
suspend(isc_task_t *task, isc_event_t *event) {
	isc_appctx_t *actx = event->ev_arg;

	UNUSED(task);

	isc_app_ctxsuspend(actx);
	isc_event_free(&event);
}

static void
resolve_done(isc_task_t *task, isc_event_t *event) {
	resarg_t *resarg = event->ev_arg;
	dns_clientresevent_t *rev = (dns_clientresevent_t *)event;
	dns_name_t *name = NULL;
	dns_client_t *client = resarg->client;
	isc_result_t result;

	UNUSED(task);

	LOCK(&resarg->lock);

	resarg->result = rev->result;
	resarg->vresult = rev->vresult;
	while ((name = ISC_LIST_HEAD(rev->answerlist)) != NULL) {
		ISC_LIST_UNLINK(rev->answerlist, name, link);
		ISC_LIST_APPEND(*resarg->namelist, name, link);
	}

	destroyrestrans(&resarg->trans);
	isc_event_free(&event);
	resarg->client = NULL;

	if (!resarg->canceled) {
		UNLOCK(&resarg->lock);

		/*
		 * We may or may not be running.  isc__appctx_onrun will
		 * fail if we are currently running otherwise we post a
		 * action to call isc_app_ctxsuspend when we do start
		 * running.
		 */
		result = isc_app_ctxonrun(resarg->actx, client->mctx, task,
					  suspend, resarg->actx);
		if (result == ISC_R_ALREADYRUNNING) {
			isc_app_ctxsuspend(resarg->actx);
		}
	} else {
		/*
		 * We have already exited from the loop (due to some
		 * unexpected event).  Just clean the arg up.
		 */
		UNLOCK(&resarg->lock);
		isc_mutex_destroy(&resarg->lock);
		isc_mem_put(client->mctx, resarg, sizeof(*resarg));
	}

	dns_client_detach(&client);
}

isc_result_t
dns_client_resolve(dns_client_t *client, const dns_name_t *name,
		   dns_rdataclass_t rdclass, dns_rdatatype_t type,
		   unsigned int options, dns_namelist_t *namelist) {
	isc_result_t result;
	resarg_t *resarg = NULL;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(client->actx != NULL);
	REQUIRE(namelist != NULL && ISC_LIST_EMPTY(*namelist));

	resarg = isc_mem_get(client->mctx, sizeof(*resarg));

	*resarg = (resarg_t){
		.actx = client->actx,
		.client = client,
		.result = DNS_R_SERVFAIL,
		.namelist = namelist,
	};

	isc_mutex_init(&resarg->lock);

	result = dns_client_startresolve(client, name, rdclass, type, options,
					 client->task, resolve_done, resarg,
					 &resarg->trans);
	if (result != ISC_R_SUCCESS) {
		isc_mutex_destroy(&resarg->lock);
		isc_mem_put(client->mctx, resarg, sizeof(*resarg));
		return (result);
	}

	/*
	 * Start internal event loop.  It blocks until the entire process
	 * is completed.
	 */
	result = isc_app_ctxrun(client->actx);

	LOCK(&resarg->lock);
	if (result == ISC_R_SUCCESS || result == ISC_R_SUSPEND) {
		result = resarg->result;
	}
	if (result != ISC_R_SUCCESS && resarg->vresult != ISC_R_SUCCESS) {
		/*
		 * If this lookup failed due to some error in DNSSEC
		 * validation, return the validation error code.
		 * XXX: or should we pass the validation result separately?
		 */
		result = resarg->vresult;
	}
	if (resarg->trans != NULL) {
		/*
		 * Unusual termination (perhaps due to signal).  We need some
		 * tricky cleanup process.
		 */
		resarg->canceled = true;
		cancelresolve(resarg->trans);

		UNLOCK(&resarg->lock);

		/* resarg will be freed in the event handler. */
	} else {
		UNLOCK(&resarg->lock);

		isc_mutex_destroy(&resarg->lock);
		isc_mem_put(client->mctx, resarg, sizeof(*resarg));
	}

	return (result);
}

isc_result_t
dns_client_startresolve(dns_client_t *client, const dns_name_t *name,
			dns_rdataclass_t rdclass, dns_rdatatype_t type,
			unsigned int options, isc_task_t *task,
			isc_taskaction_t action, void *arg,
			dns_clientrestrans_t **transp) {
	dns_view_t *view = NULL;
	dns_clientresevent_t *event = NULL;
	resctx_t *rctx = NULL;
	isc_task_t *tclone = NULL;
	isc_mem_t *mctx;
	isc_result_t result;
	dns_rdataset_t *rdataset, *sigrdataset;
	bool want_dnssec, want_validation, want_cdflag, want_tcp;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(transp != NULL && *transp == NULL);

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   rdclass, &view);
	UNLOCK(&client->lock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	mctx = client->mctx;
	rdataset = NULL;
	sigrdataset = NULL;
	want_dnssec = ((options & DNS_CLIENTRESOPT_NODNSSEC) == 0);
	want_validation = ((options & DNS_CLIENTRESOPT_NOVALIDATE) == 0);
	want_cdflag = ((options & DNS_CLIENTRESOPT_NOCDFLAG) == 0);
	want_tcp = ((options & DNS_CLIENTRESOPT_TCP) != 0);

	/*
	 * Prepare some intermediate resources
	 */
	tclone = NULL;
	isc_task_attach(task, &tclone);
	event = (dns_clientresevent_t *)isc_event_allocate(
		mctx, tclone, DNS_EVENT_CLIENTRESDONE, action, arg,
		sizeof(*event));
	event->result = DNS_R_SERVFAIL;
	ISC_LIST_INIT(event->answerlist);

	rctx = isc_mem_get(mctx, sizeof(*rctx));
	isc_mutex_init(&rctx->lock);

	result = getrdataset(mctx, &rdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	rctx->rdataset = rdataset;

	if (want_dnssec) {
		result = getrdataset(mctx, &sigrdataset);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}
	rctx->sigrdataset = sigrdataset;

	dns_fixedname_init(&rctx->name);
	dns_name_copy(name, dns_fixedname_name(&rctx->name));

	rctx->client = client;
	ISC_LINK_INIT(rctx, link);
	rctx->canceled = false;
	rctx->task = client->task;
	rctx->type = type;
	rctx->view = view;
	rctx->restarts = 0;
	rctx->fetch = NULL;
	rctx->want_dnssec = want_dnssec;
	rctx->want_validation = want_validation;
	rctx->want_cdflag = want_cdflag;
	rctx->want_tcp = want_tcp;
	ISC_LIST_INIT(rctx->namelist);
	rctx->event = event;

	rctx->magic = RCTX_MAGIC;
	isc_refcount_increment(&client->references);

	LOCK(&client->lock);
	ISC_LIST_APPEND(client->resctxs, rctx, link);
	UNLOCK(&client->lock);

	*transp = (dns_clientrestrans_t *)rctx;
	client_resfind(rctx, NULL);

	return (ISC_R_SUCCESS);

cleanup:
	if (rdataset != NULL) {
		putrdataset(client->mctx, &rdataset);
	}
	if (sigrdataset != NULL) {
		putrdataset(client->mctx, &sigrdataset);
	}
	isc_mutex_destroy(&rctx->lock);
	isc_mem_put(mctx, rctx, sizeof(*rctx));
	isc_event_free(ISC_EVENT_PTR(&event));
	isc_task_detach(&tclone);
	dns_view_detach(&view);

	return (result);
}

/*%<
 * Cancel an ongoing resolution procedure started via
 * dns_client_startresolve().
 *
 * If the resolution procedure has not completed, post its CLIENTRESDONE
 * event with a result code of #ISC_R_CANCELED.
 */
static void
cancelresolve(dns_clientrestrans_t *trans) {
	resctx_t *rctx = NULL;

	REQUIRE(trans != NULL);
	rctx = (resctx_t *)trans;
	REQUIRE(RCTX_VALID(rctx));

	LOCK(&rctx->lock);

	if (!rctx->canceled) {
		rctx->canceled = true;
		if (rctx->fetch != NULL) {
			dns_resolver_cancelfetch(rctx->fetch);
		}
	}

	UNLOCK(&rctx->lock);
}

void
dns_client_freeresanswer(dns_client_t *client, dns_namelist_t *namelist) {
	dns_name_t *name;
	dns_rdataset_t *rdataset;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(namelist != NULL);

	while ((name = ISC_LIST_HEAD(*namelist)) != NULL) {
		ISC_LIST_UNLINK(*namelist, name, link);
		while ((rdataset = ISC_LIST_HEAD(name->list)) != NULL) {
			ISC_LIST_UNLINK(name->list, rdataset, link);
			putrdataset(client->mctx, &rdataset);
		}
		dns_name_free(name, client->mctx);
		isc_mem_put(client->mctx, name, sizeof(*name));
	}
}

/*%
 * Destroy name resolution transaction state identified by '*transp'.
 *
 * The caller must have received the CLIENTRESDONE event (either because the
 * resolution completed or because cancelresolve() was called).
 */
static void
destroyrestrans(dns_clientrestrans_t **transp) {
	resctx_t *rctx = NULL;
	isc_mem_t *mctx = NULL;
	dns_client_t *client = NULL;

	REQUIRE(transp != NULL);

	rctx = (resctx_t *)*transp;
	*transp = NULL;

	REQUIRE(RCTX_VALID(rctx));
	REQUIRE(rctx->fetch == NULL);
	REQUIRE(rctx->event == NULL);

	client = rctx->client;

	REQUIRE(DNS_CLIENT_VALID(client));

	mctx = client->mctx;
	dns_view_detach(&rctx->view);

	/*
	 * Wait for the lock in client_resfind to be released before
	 * destroying the lock.
	 */
	LOCK(&rctx->lock);
	UNLOCK(&rctx->lock);

	LOCK(&client->lock);

	INSIST(ISC_LINK_LINKED(rctx, link));
	ISC_LIST_UNLINK(client->resctxs, rctx, link);

	UNLOCK(&client->lock);

	INSIST(ISC_LIST_EMPTY(rctx->namelist));

	isc_mutex_destroy(&rctx->lock);
	rctx->magic = 0;

	isc_mem_put(mctx, rctx, sizeof(*rctx));
}

isc_result_t
dns_client_addtrustedkey(dns_client_t *client, dns_rdataclass_t rdclass,
			 dns_rdatatype_t rdtype, const dns_name_t *keyname,
			 isc_buffer_t *databuf) {
	isc_result_t result;
	dns_view_t *view = NULL;
	dns_keytable_t *secroots = NULL;
	dns_name_t *name = NULL;
	char rdatabuf[DST_KEY_MAXSIZE];
	unsigned char digest[ISC_MAX_MD_SIZE];
	dns_rdata_ds_t ds;
	dns_decompress_t dctx;
	dns_rdata_t rdata;
	isc_buffer_t b;

	REQUIRE(DNS_CLIENT_VALID(client));

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   rdclass, &view);
	UNLOCK(&client->lock);
	CHECK(result);

	CHECK(dns_view_getsecroots(view, &secroots));

	DE_CONST(keyname, name);

	if (rdtype != dns_rdatatype_dnskey && rdtype != dns_rdatatype_ds) {
		result = ISC_R_NOTIMPLEMENTED;
		goto cleanup;
	}

	isc_buffer_init(&b, rdatabuf, sizeof(rdatabuf));
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_NONE);
	dns_rdata_init(&rdata);
	isc_buffer_setactive(databuf, isc_buffer_usedlength(databuf));
	CHECK(dns_rdata_fromwire(&rdata, rdclass, rdtype, databuf, &dctx, 0,
				 &b));
	dns_decompress_invalidate(&dctx);

	if (rdtype == dns_rdatatype_ds) {
		CHECK(dns_rdata_tostruct(&rdata, &ds, NULL));
	} else {
		CHECK(dns_ds_fromkeyrdata(name, &rdata, DNS_DSDIGEST_SHA256,
					  digest, &ds));
	}

	CHECK(dns_keytable_add(secroots, false, false, name, &ds, NULL, NULL));

cleanup:
	if (view != NULL) {
		dns_view_detach(&view);
	}
	if (secroots != NULL) {
		dns_keytable_detach(&secroots);
	}
	return (result);
}
