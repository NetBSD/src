/*	$NetBSD: client.c,v 1.12 2023/01/25 21:43:30 christos Exp $	*/

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
#include <isc/safe.h>
#include <isc/sockaddr.h>
#include <isc/socket.h>
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
#include <dns/result.h>
#include <dns/tsec.h>
#include <dns/tsig.h>
#include <dns/view.h>

#include <dst/dst.h>

#define DNS_CLIENT_MAGIC    ISC_MAGIC('D', 'N', 'S', 'c')
#define DNS_CLIENT_VALID(c) ISC_MAGIC_VALID(c, DNS_CLIENT_MAGIC)

#define RCTX_MAGIC    ISC_MAGIC('R', 'c', 't', 'x')
#define RCTX_VALID(c) ISC_MAGIC_VALID(c, RCTX_MAGIC)

#define REQCTX_MAGIC	ISC_MAGIC('R', 'q', 'c', 'x')
#define REQCTX_VALID(c) ISC_MAGIC_VALID(c, REQCTX_MAGIC)

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
	isc_socketmgr_t *socketmgr;
	isc_timermgr_t *timermgr;
	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatch_t *dispatchv4;
	dns_dispatch_t *dispatchv6;

	unsigned int update_timeout;
	unsigned int update_udptimeout;
	unsigned int update_udpretries;
	unsigned int find_timeout;
	unsigned int find_udpretries;

	isc_refcount_t references;

	/* Locked */
	dns_viewlist_t viewlist;
	ISC_LIST(struct resctx) resctxs;
	ISC_LIST(struct reqctx) reqctxs;
	ISC_LIST(struct updatectx) updatectxs;
};

/*%
 * Timeout/retry constants for dynamic update borrowed from nsupdate
 */
#define DEF_UPDATE_TIMEOUT    300
#define MIN_UPDATE_TIMEOUT    30
#define DEF_UPDATE_UDPTIMEOUT 3
#define DEF_UPDATE_UDPRETRIES 3

#define DEF_FIND_TIMEOUT    5
#define DEF_FIND_UDPRETRIES 3

#define DNS_CLIENTATTR_OWNCTX 0x01

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

/*%
 * Internal state for a single DNS request
 */
typedef struct reqctx {
	/* Unlocked */
	unsigned int magic;
	isc_mutex_t lock;
	dns_client_t *client;
	unsigned int parseoptions;

	/* Locked */
	ISC_LINK(struct reqctx) link;
	bool canceled;
	dns_tsigkey_t *tsigkey;
	dns_request_t *request;
	dns_clientreqevent_t *event;
} reqctx_t;

/*%
 * Argument of an internal event for synchronous DNS request.
 */
typedef struct reqarg {
	/* Unlocked */
	isc_appctx_t *actx;
	dns_client_t *client;
	isc_mutex_t lock;

	/* Locked */
	isc_result_t result;
	dns_clientreqtrans_t *trans;
	bool canceled;
} reqarg_t;

/*%
 * Argument of an internal event for synchronous name resolution.
 */
typedef struct updatearg {
	/* Unlocked */
	isc_appctx_t *actx;
	dns_client_t *client;
	isc_mutex_t lock;

	/* Locked */
	isc_result_t result;
	dns_clientupdatetrans_t *trans;
	bool canceled;
} updatearg_t;

/*%
 * Internal state for a single dynamic update procedure
 */
typedef struct updatectx {
	/* Unlocked */
	unsigned int magic;
	isc_mutex_t lock;
	dns_client_t *client;
	bool want_tcp;

	/* Locked */
	dns_request_t *updatereq;
	dns_request_t *soareq;
	dns_clientrestrans_t *restrans;
	dns_clientrestrans_t *restrans2;
	bool canceled;

	/* Task Locked */
	ISC_LINK(struct updatectx) link;
	dns_clientupdatestate_t state;
	dns_rdataclass_t rdclass;
	dns_view_t *view;
	dns_message_t *updatemsg;
	dns_message_t *soaquery;
	dns_clientupdateevent_t *event;
	dns_tsigkey_t *tsigkey;
	dst_key_t *sig0key;
	dns_name_t *firstname;
	dns_name_t soaqname;
	dns_fixedname_t zonefname;
	dns_name_t *zonename;
	isc_sockaddrlist_t servers;
	unsigned int nservers;
	isc_sockaddr_t *currentserver;
	struct updatectx *bp4;
	struct updatectx *bp6;
} updatectx_t;

static isc_result_t
request_soa(updatectx_t *uctx);
static void
client_resfind(resctx_t *rctx, dns_fetchevent_t *event);
static isc_result_t
send_update(updatectx_t *uctx);

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
	       isc_socketmgr_t *socketmgr, isc_taskmgr_t *taskmgr,
	       bool is_shared, dns_dispatch_t **dispp,
	       const isc_sockaddr_t *localaddr) {
	unsigned int attrs, attrmask;
	dns_dispatch_t *disp;
	unsigned buffersize, maxbuffers, maxrequests, buckets, increment;
	isc_result_t result;
	isc_sockaddr_t anyaddr;

	attrs = 0;
	attrs |= DNS_DISPATCHATTR_UDP;
	switch (family) {
	case AF_INET:
		attrs |= DNS_DISPATCHATTR_IPV4;
		break;
	case AF_INET6:
		attrs |= DNS_DISPATCHATTR_IPV6;
		break;
	default:
		UNREACHABLE();
	}
	attrmask = 0;
	attrmask |= DNS_DISPATCHATTR_UDP;
	attrmask |= DNS_DISPATCHATTR_TCP;
	attrmask |= DNS_DISPATCHATTR_IPV4;
	attrmask |= DNS_DISPATCHATTR_IPV6;

	if (localaddr == NULL) {
		isc_sockaddr_anyofpf(&anyaddr, family);
		localaddr = &anyaddr;
	}

	buffersize = 4096;
	maxbuffers = is_shared ? 1000 : 8;
	maxrequests = 32768;
	buckets = is_shared ? 16411 : 3;
	increment = is_shared ? 16433 : 5;

	disp = NULL;
	result = dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, localaddr,
				     buffersize, maxbuffers, maxrequests,
				     buckets, increment, attrs, attrmask,
				     &disp);
	if (result == ISC_R_SUCCESS) {
		*dispp = disp;
	}

	return (result);
}

static isc_result_t
createview(isc_mem_t *mctx, dns_rdataclass_t rdclass, unsigned int options,
	   isc_taskmgr_t *taskmgr, unsigned int ntasks,
	   isc_socketmgr_t *socketmgr, isc_timermgr_t *timermgr,
	   dns_dispatchmgr_t *dispatchmgr, dns_dispatch_t *dispatchv4,
	   dns_dispatch_t *dispatchv6, dns_view_t **viewp) {
	isc_result_t result;
	dns_view_t *view = NULL;
	const char *dbtype;

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

	result = dns_view_createresolver(view, taskmgr, ntasks, 1, socketmgr,
					 timermgr, 0, dispatchmgr, dispatchv4,
					 dispatchv6);
	if (result != ISC_R_SUCCESS) {
		dns_view_detach(&view);
		return (result);
	}

	/*
	 * Set cache DB.
	 * XXX: it may be better if specific DB implementations can be
	 * specified via some configuration knob.
	 */
	if ((options & DNS_CLIENTCREATEOPT_USECACHE) != 0) {
		dbtype = "rbt";
	} else {
		dbtype = "ecdb";
	}
	result = dns_db_create(mctx, dbtype, dns_rootname, dns_dbtype_cache,
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
		  isc_socketmgr_t *socketmgr, isc_timermgr_t *timermgr,
		  unsigned int options, dns_client_t **clientp,
		  const isc_sockaddr_t *localaddr4,
		  const isc_sockaddr_t *localaddr6) {
	isc_result_t result;
	dns_client_t *client;
	dns_dispatchmgr_t *dispatchmgr = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_dispatch_t *dispatchv6 = NULL;
	dns_view_t *view = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(timermgr != NULL);
	REQUIRE(socketmgr != NULL);
	REQUIRE(clientp != NULL && *clientp == NULL);

	client = isc_mem_get(mctx, sizeof(*client));

	isc_mutex_init(&client->lock);

	client->actx = actx;
	client->taskmgr = taskmgr;
	client->socketmgr = socketmgr;
	client->timermgr = timermgr;

	client->task = NULL;
	result = isc_task_create(client->taskmgr, 0, &client->task);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	result = dns_dispatchmgr_create(mctx, &dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}
	client->dispatchmgr = dispatchmgr;
	(void)setsourceports(mctx, dispatchmgr);

	/*
	 * If only one address family is specified, use it.
	 * If neither family is specified, or if both are, use both.
	 */
	client->dispatchv4 = NULL;
	if (localaddr4 != NULL || localaddr6 == NULL) {
		result = getudpdispatch(AF_INET, dispatchmgr, socketmgr,
					taskmgr, true, &dispatchv4, localaddr4);
		if (result == ISC_R_SUCCESS) {
			client->dispatchv4 = dispatchv4;
		}
	}

	client->dispatchv6 = NULL;
	if (localaddr6 != NULL || localaddr4 == NULL) {
		result = getudpdispatch(AF_INET6, dispatchmgr, socketmgr,
					taskmgr, true, &dispatchv6, localaddr6);
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
	result = createview(mctx, dns_rdataclass_in, options, taskmgr,
			    RESOLVER_NTASKS, socketmgr, timermgr, dispatchmgr,
			    dispatchv4, dispatchv6, &view);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_references;
	}

	ISC_LIST_INIT(client->viewlist);
	ISC_LIST_APPEND(client->viewlist, view, link);

	dns_view_freeze(view); /* too early? */

	ISC_LIST_INIT(client->resctxs);
	ISC_LIST_INIT(client->reqctxs);
	ISC_LIST_INIT(client->updatectxs);

	client->mctx = NULL;
	isc_mem_attach(mctx, &client->mctx);

	client->update_timeout = DEF_UPDATE_TIMEOUT;
	client->update_udptimeout = DEF_UPDATE_UDPTIMEOUT;
	client->update_udpretries = DEF_UPDATE_UDPRETRIES;
	client->find_timeout = DEF_FIND_TIMEOUT;
	client->find_udpretries = DEF_FIND_UDPRETRIES;
	client->attributes = 0;

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
	dns_dispatchmgr_destroy(&dispatchmgr);
cleanup_task:
	isc_task_detach(&client->task);
cleanup_lock:
	isc_mutex_destroy(&client->lock);
	isc_mem_put(mctx, client, sizeof(*client));

	return (result);
}

static void
destroyclient(dns_client_t *client) {
	dns_view_t *view;

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

	dns_dispatchmgr_destroy(&client->dispatchmgr);

	isc_task_detach(&client->task);
	client->magic = 0;

	isc_mem_putanddetach(&client->mctx, client, sizeof(*client));
}

void
dns_client_destroy(dns_client_t **clientp) {
	dns_client_t *client;

	REQUIRE(clientp != NULL);
	client = *clientp;
	*clientp = NULL;
	REQUIRE(DNS_CLIENT_VALID(client));

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
	dns_name_t *name, *prefix;
	dns_fixedname_t foundname, fixed;
	dns_rdataset_t *trdataset;
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
			fname = dns_fixedname_name(&event->foundname);
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
			dns_name_copynf(&cname.cname, name);
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
	dns_name_t *name;
	isc_result_t result;

	UNUSED(task);

	LOCK(&resarg->lock);

	resarg->result = rev->result;
	resarg->vresult = rev->vresult;
	while ((name = ISC_LIST_HEAD(rev->answerlist)) != NULL) {
		ISC_LIST_UNLINK(rev->answerlist, name, link);
		ISC_LIST_APPEND(*resarg->namelist, name, link);
	}

	dns_client_destroyrestrans(&resarg->trans);
	isc_event_free(&event);

	if (!resarg->canceled) {
		UNLOCK(&resarg->lock);

		/*
		 * We may or may not be running.  isc__appctx_onrun will
		 * fail if we are currently running otherwise we post a
		 * action to call isc_app_ctxsuspend when we do start
		 * running.
		 */
		result = isc_app_ctxonrun(resarg->actx, resarg->client->mctx,
					  task, suspend, resarg->actx);
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
		isc_mem_put(resarg->client->mctx, resarg, sizeof(*resarg));
	}
}

isc_result_t
dns_client_resolve(dns_client_t *client, const dns_name_t *name,
		   dns_rdataclass_t rdclass, dns_rdatatype_t type,
		   unsigned int options, dns_namelist_t *namelist) {
	isc_result_t result;
	isc_appctx_t *actx;
	resarg_t *resarg;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(namelist != NULL && ISC_LIST_EMPTY(*namelist));

	if ((client->attributes & DNS_CLIENTATTR_OWNCTX) == 0 &&
	    (options & DNS_CLIENTRESOPT_ALLOWRUN) == 0)
	{
		/*
		 * If the client is run under application's control, we need
		 * to create a new running (sub)environment for this
		 * particular resolution.
		 */
		return (ISC_R_NOTIMPLEMENTED); /* XXXTBD */
	} else {
		actx = client->actx;
	}

	resarg = isc_mem_get(client->mctx, sizeof(*resarg));

	isc_mutex_init(&resarg->lock);

	resarg->actx = actx;
	resarg->client = client;
	resarg->result = DNS_R_SERVFAIL;
	resarg->namelist = namelist;
	resarg->trans = NULL;
	resarg->canceled = false;
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
	result = isc_app_ctxrun(actx);

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
		dns_client_cancelresolve(resarg->trans);

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
	dns_name_copynf(name, dns_fixedname_name(&rctx->name));

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

void
dns_client_cancelresolve(dns_clientrestrans_t *trans) {
	resctx_t *rctx;

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

void
dns_client_destroyrestrans(dns_clientrestrans_t **transp) {
	resctx_t *rctx;
	isc_mem_t *mctx;
	dns_client_t *client;

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

	dns_client_destroy(&client);
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

	CHECK(dns_keytable_add(secroots, false, false, name, &ds));

cleanup:
	if (view != NULL) {
		dns_view_detach(&view);
	}
	if (secroots != NULL) {
		dns_keytable_detach(&secroots);
	}
	return (result);
}

/*%
 * Simple request routines
 */
static void
request_done(isc_task_t *task, isc_event_t *event) {
	dns_requestevent_t *reqev = NULL;
	dns_request_t *request;
	isc_result_t result, eresult;
	reqctx_t *ctx;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_REQUESTDONE);
	reqev = (dns_requestevent_t *)event;
	request = reqev->request;
	result = eresult = reqev->result;
	ctx = reqev->ev_arg;
	REQUIRE(REQCTX_VALID(ctx));

	isc_event_free(&event);

	LOCK(&ctx->lock);

	if (eresult == ISC_R_SUCCESS) {
		result = dns_request_getresponse(request, ctx->event->rmessage,
						 ctx->parseoptions);
	}

	if (ctx->tsigkey != NULL) {
		dns_tsigkey_detach(&ctx->tsigkey);
	}

	if (ctx->canceled) {
		ctx->event->result = ISC_R_CANCELED;
	} else {
		ctx->event->result = result;
	}
	task = ctx->event->ev_sender;
	ctx->event->ev_sender = ctx;
	isc_task_sendanddetach(&task, ISC_EVENT_PTR(&ctx->event));

	UNLOCK(&ctx->lock);
}

static void
localrequest_done(isc_task_t *task, isc_event_t *event) {
	reqarg_t *reqarg = event->ev_arg;
	dns_clientreqevent_t *rev = (dns_clientreqevent_t *)event;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_CLIENTREQDONE);

	LOCK(&reqarg->lock);

	reqarg->result = rev->result;
	dns_client_destroyreqtrans(&reqarg->trans);
	isc_event_free(&event);

	if (!reqarg->canceled) {
		UNLOCK(&reqarg->lock);

		/* Exit from the internal event loop */
		isc_app_ctxsuspend(reqarg->actx);
	} else {
		/*
		 * We have already exited from the loop (due to some
		 * unexpected event).  Just clean the arg up.
		 */
		UNLOCK(&reqarg->lock);
		isc_mutex_destroy(&reqarg->lock);
		isc_mem_put(reqarg->client->mctx, reqarg, sizeof(*reqarg));
	}
}

isc_result_t
dns_client_request(dns_client_t *client, dns_message_t *qmessage,
		   dns_message_t *rmessage, const isc_sockaddr_t *server,
		   unsigned int options, unsigned int parseoptions,
		   dns_tsec_t *tsec, unsigned int timeout,
		   unsigned int udptimeout, unsigned int udpretries) {
	isc_appctx_t *actx;
	reqarg_t *reqarg;
	isc_result_t result;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(qmessage != NULL);
	REQUIRE(rmessage != NULL);

	if ((client->attributes & DNS_CLIENTATTR_OWNCTX) == 0 &&
	    (options & DNS_CLIENTREQOPT_ALLOWRUN) == 0)
	{
		/*
		 * If the client is run under application's control, we need
		 * to create a new running (sub)environment for this
		 * particular resolution.
		 */
		return (ISC_R_NOTIMPLEMENTED); /* XXXTBD */
	} else {
		actx = client->actx;
	}

	reqarg = isc_mem_get(client->mctx, sizeof(*reqarg));

	isc_mutex_init(&reqarg->lock);

	reqarg->actx = actx;
	reqarg->client = client;
	reqarg->trans = NULL;
	reqarg->canceled = false;

	result = dns_client_startrequest(
		client, qmessage, rmessage, server, options, parseoptions, tsec,
		timeout, udptimeout, udpretries, client->task,
		localrequest_done, reqarg, &reqarg->trans);
	if (result != ISC_R_SUCCESS) {
		isc_mutex_destroy(&reqarg->lock);
		isc_mem_put(client->mctx, reqarg, sizeof(*reqarg));
		return (result);
	}

	/*
	 * Start internal event loop.  It blocks until the entire process
	 * is completed.
	 */
	result = isc_app_ctxrun(actx);

	LOCK(&reqarg->lock);
	if (result == ISC_R_SUCCESS || result == ISC_R_SUSPEND) {
		result = reqarg->result;
	}
	if (reqarg->trans != NULL) {
		/*
		 * Unusual termination (perhaps due to signal).  We need some
		 * tricky cleanup process.
		 */
		reqarg->canceled = true;
		dns_client_cancelresolve(reqarg->trans);

		UNLOCK(&reqarg->lock);

		/* reqarg will be freed in the event handler. */
	} else {
		UNLOCK(&reqarg->lock);

		isc_mutex_destroy(&reqarg->lock);
		isc_mem_put(client->mctx, reqarg, sizeof(*reqarg));
	}

	return (result);
}

isc_result_t
dns_client_startrequest(dns_client_t *client, dns_message_t *qmessage,
			dns_message_t *rmessage, const isc_sockaddr_t *server,
			unsigned int options, unsigned int parseoptions,
			dns_tsec_t *tsec, unsigned int timeout,
			unsigned int udptimeout, unsigned int udpretries,
			isc_task_t *task, isc_taskaction_t action, void *arg,
			dns_clientreqtrans_t **transp) {
	isc_result_t result;
	dns_view_t *view = NULL;
	isc_task_t *tclone = NULL;
	dns_clientreqevent_t *event = NULL;
	reqctx_t *ctx = NULL;
	dns_tsectype_t tsectype = dns_tsectype_none;
	unsigned int reqoptions;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(qmessage != NULL);
	REQUIRE(rmessage != NULL);
	REQUIRE(transp != NULL && *transp == NULL);

	if (tsec != NULL) {
		tsectype = dns_tsec_gettype(tsec);
		if (tsectype != dns_tsectype_tsig) {
			return (ISC_R_NOTIMPLEMENTED); /* XXX */
		}
	}

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   qmessage->rdclass, &view);
	UNLOCK(&client->lock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	reqoptions = 0;
	if ((options & DNS_CLIENTREQOPT_TCP) != 0) {
		reqoptions |= DNS_REQUESTOPT_TCP;
	}

	tclone = NULL;
	isc_task_attach(task, &tclone);
	event = (dns_clientreqevent_t *)isc_event_allocate(
		client->mctx, tclone, DNS_EVENT_CLIENTREQDONE, action, arg,
		sizeof(*event));

	ctx = isc_mem_get(client->mctx, sizeof(*ctx));
	isc_mutex_init(&ctx->lock);

	ctx->client = client;
	ISC_LINK_INIT(ctx, link);
	ctx->parseoptions = parseoptions;
	ctx->canceled = false;
	ctx->event = event;
	ctx->event->rmessage = rmessage;
	ctx->tsigkey = NULL;
	if (tsec != NULL) {
		dns_tsec_getkey(tsec, &ctx->tsigkey);
	}

	ctx->magic = REQCTX_MAGIC;

	LOCK(&client->lock);
	ISC_LIST_APPEND(client->reqctxs, ctx, link);
	isc_refcount_increment(&client->references);
	UNLOCK(&client->lock);

	ctx->request = NULL;
	result = dns_request_createvia(view->requestmgr, qmessage, NULL, server,
				       -1, reqoptions, ctx->tsigkey, timeout,
				       udptimeout, udpretries, client->task,
				       request_done, ctx, &ctx->request);
	if (result == ISC_R_SUCCESS) {
		dns_view_detach(&view);
		*transp = (dns_clientreqtrans_t *)ctx;
		return (ISC_R_SUCCESS);
	}

	isc_refcount_decrement1(&client->references);

	LOCK(&client->lock);
	ISC_LIST_UNLINK(client->reqctxs, ctx, link);
	UNLOCK(&client->lock);
	isc_mutex_destroy(&ctx->lock);
	isc_mem_put(client->mctx, ctx, sizeof(*ctx));

	isc_event_free(ISC_EVENT_PTR(&event));
	isc_task_detach(&tclone);
	dns_view_detach(&view);

	return (result);
}

void
dns_client_cancelrequest(dns_clientreqtrans_t *trans) {
	reqctx_t *ctx;

	REQUIRE(trans != NULL);
	ctx = (reqctx_t *)trans;
	REQUIRE(REQCTX_VALID(ctx));

	LOCK(&ctx->lock);

	if (!ctx->canceled) {
		ctx->canceled = true;
		if (ctx->request != NULL) {
			dns_request_cancel(ctx->request);
		}
	}

	UNLOCK(&ctx->lock);
}

void
dns_client_destroyreqtrans(dns_clientreqtrans_t **transp) {
	reqctx_t *ctx;
	isc_mem_t *mctx;
	dns_client_t *client;

	REQUIRE(transp != NULL);
	ctx = (reqctx_t *)*transp;
	*transp = NULL;
	REQUIRE(REQCTX_VALID(ctx));
	client = ctx->client;
	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(ctx->event == NULL);
	REQUIRE(ctx->request != NULL);

	dns_request_destroy(&ctx->request);
	mctx = client->mctx;

	LOCK(&client->lock);

	INSIST(ISC_LINK_LINKED(ctx, link));
	ISC_LIST_UNLINK(client->reqctxs, ctx, link);

	UNLOCK(&client->lock);

	isc_mutex_destroy(&ctx->lock);
	ctx->magic = 0;

	isc_mem_put(mctx, ctx, sizeof(*ctx));

	dns_client_destroy(&client);
}

/*%
 * Dynamic update routines
 */
static isc_result_t
rcode2result(dns_rcode_t rcode) {
	/* XXX: isn't there a similar function? */
	switch (rcode) {
	case dns_rcode_formerr:
		return (DNS_R_FORMERR);
	case dns_rcode_servfail:
		return (DNS_R_SERVFAIL);
	case dns_rcode_nxdomain:
		return (DNS_R_NXDOMAIN);
	case dns_rcode_notimp:
		return (DNS_R_NOTIMP);
	case dns_rcode_refused:
		return (DNS_R_REFUSED);
	case dns_rcode_yxdomain:
		return (DNS_R_YXDOMAIN);
	case dns_rcode_yxrrset:
		return (DNS_R_YXRRSET);
	case dns_rcode_nxrrset:
		return (DNS_R_NXRRSET);
	case dns_rcode_notauth:
		return (DNS_R_NOTAUTH);
	case dns_rcode_notzone:
		return (DNS_R_NOTZONE);
	case dns_rcode_badvers:
		return (DNS_R_BADVERS);
	}

	return (ISC_R_FAILURE);
}

static void
update_sendevent(updatectx_t *uctx, isc_result_t result) {
	isc_task_t *task;

	dns_message_detach(&uctx->updatemsg);
	if (uctx->tsigkey != NULL) {
		dns_tsigkey_detach(&uctx->tsigkey);
	}
	if (uctx->sig0key != NULL) {
		dst_key_free(&uctx->sig0key);
	}

	if (uctx->canceled) {
		uctx->event->result = ISC_R_CANCELED;
	} else {
		uctx->event->result = result;
	}
	uctx->event->state = uctx->state;
	task = uctx->event->ev_sender;
	uctx->event->ev_sender = uctx;
	isc_task_sendanddetach(&task, ISC_EVENT_PTR(&uctx->event));
}

static void
update_done(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_requestevent_t *reqev = NULL;
	dns_request_t *request;
	dns_message_t *answer = NULL;
	updatectx_t *uctx = event->ev_arg;
	dns_client_t *client;
	unsigned int timeout, reqoptions;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_REQUESTDONE);
	reqev = (dns_requestevent_t *)event;
	request = reqev->request;
	REQUIRE(UCTX_VALID(uctx));
	client = uctx->client;
	REQUIRE(DNS_CLIENT_VALID(client));

	result = reqev->result;
	if (result != ISC_R_SUCCESS) {
		goto out;
	}

	dns_message_create(client->mctx, DNS_MESSAGE_INTENTPARSE, &answer);
	uctx->state = dns_clientupdatestate_done;
	result = dns_request_getresponse(request, answer,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	if (result == ISC_R_SUCCESS && answer->rcode != dns_rcode_noerror) {
		result = rcode2result(answer->rcode);
	}

out:
	if (answer != NULL) {
		dns_message_detach(&answer);
	}
	isc_event_free(&event);

	LOCK(&uctx->lock);
	uctx->currentserver = ISC_LIST_NEXT(uctx->currentserver, link);
	dns_request_destroy(&uctx->updatereq);
	/*
	 * Moving on to the next server shouldn't change the result
	 * for NXDOMAIN, YXDOMAIN, NXRRSET and YXRRSET as they
	 * indicate a prerequisite failure.  REFUSED should also
	 * be consistent across all servers but often isn't as that
	 * is policy rather that zone content driven (slaves that
	 * aren't willing to forward should return NOTIMPL).  NOTZONE
	 * indicates that we stuffed up the request construction so
	 * don't retry.
	 */
	if (result != ISC_R_SUCCESS && result != DNS_R_NXDOMAIN &&
	    result != DNS_R_YXDOMAIN && result != DNS_R_YXRRSET &&
	    result != DNS_R_NXRRSET && result != DNS_R_NOTZONE &&
	    !uctx->canceled && uctx->currentserver != NULL)
	{
		dns_message_renderreset(uctx->updatemsg);
		dns_message_settsigkey(uctx->updatemsg, NULL);

		timeout = client->update_timeout / uctx->nservers;
		if (timeout < MIN_UPDATE_TIMEOUT) {
			timeout = MIN_UPDATE_TIMEOUT;
		}
		reqoptions = 0;
		if (uctx->want_tcp) {
			reqoptions |= DNS_REQUESTOPT_TCP;
		}
		result = dns_request_createvia(
			uctx->view->requestmgr, uctx->updatemsg, NULL,
			uctx->currentserver, -1, reqoptions, uctx->tsigkey,
			timeout, client->update_udptimeout,
			client->update_udpretries, client->task, update_done,
			uctx, &uctx->updatereq);
		UNLOCK(&uctx->lock);

		if (result == ISC_R_SUCCESS) {
			/* XXX: should we keep the 'done' state here? */
			uctx->state = dns_clientupdatestate_sent;
			return;
		}
	} else {
		UNLOCK(&uctx->lock);
	}

	update_sendevent(uctx, result);
}

static isc_result_t
send_update(updatectx_t *uctx) {
	isc_result_t result;
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_client_t *client = uctx->client;
	unsigned int timeout, reqoptions;

	REQUIRE(uctx->zonename != NULL && uctx->currentserver != NULL);

	result = dns_message_gettempname(uctx->updatemsg, &name);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	dns_name_init(name, NULL);
	dns_name_clone(uctx->zonename, name);
	result = dns_message_gettemprdataset(uctx->updatemsg, &rdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(uctx->updatemsg, &name);
		return (result);
	}
	dns_rdataset_makequestion(rdataset, uctx->rdclass, dns_rdatatype_soa);
	ISC_LIST_INIT(name->list);
	ISC_LIST_APPEND(name->list, rdataset, link);
	dns_message_addname(uctx->updatemsg, name, DNS_SECTION_ZONE);
	if (uctx->tsigkey == NULL && uctx->sig0key != NULL) {
		result = dns_message_setsig0key(uctx->updatemsg, uctx->sig0key);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	timeout = client->update_timeout / uctx->nservers;
	if (timeout < MIN_UPDATE_TIMEOUT) {
		timeout = MIN_UPDATE_TIMEOUT;
	}
	reqoptions = 0;
	if (uctx->want_tcp) {
		reqoptions |= DNS_REQUESTOPT_TCP;
	}
	result = dns_request_createvia(
		uctx->view->requestmgr, uctx->updatemsg, NULL,
		uctx->currentserver, -1, reqoptions, uctx->tsigkey, timeout,
		client->update_udptimeout, client->update_udpretries,
		client->task, update_done, uctx, &uctx->updatereq);
	if (result == ISC_R_SUCCESS &&
	    uctx->state == dns_clientupdatestate_prepare) {
		uctx->state = dns_clientupdatestate_sent;
	}

	return (result);
}

static void
resolveaddr_done(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	int family;
	dns_rdatatype_t qtype;
	dns_clientresevent_t *rev = (dns_clientresevent_t *)event;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	updatectx_t *uctx;
	bool completed = false;

	UNUSED(task);

	REQUIRE(event->ev_arg != NULL);
	uctx = *(updatectx_t **)event->ev_arg;
	REQUIRE(UCTX_VALID(uctx));

	if (event->ev_arg == &uctx->bp4) {
		family = AF_INET;
		qtype = dns_rdatatype_a;
		LOCK(&uctx->lock);
		dns_client_destroyrestrans(&uctx->restrans);
		UNLOCK(&uctx->lock);
	} else {
		INSIST(event->ev_arg == &uctx->bp6);
		family = AF_INET6;
		qtype = dns_rdatatype_aaaa;
		LOCK(&uctx->lock);
		dns_client_destroyrestrans(&uctx->restrans2);
		UNLOCK(&uctx->lock);
	}

	result = rev->result;
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	for (name = ISC_LIST_HEAD(rev->answerlist); name != NULL;
	     name = ISC_LIST_NEXT(name, link))
	{
		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (!dns_rdataset_isassociated(rdataset)) {
				continue;
			}
			if (rdataset->type != qtype) {
				continue;
			}

			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset))
			{
				dns_rdata_t rdata;
				dns_rdata_in_a_t rdata_a;
				dns_rdata_in_aaaa_t rdata_aaaa;
				isc_sockaddr_t *sa;

				sa = isc_mem_get(uctx->client->mctx,
						 sizeof(*sa));

				dns_rdata_init(&rdata);
				switch (family) {
				case AF_INET:
					dns_rdataset_current(rdataset, &rdata);
					result = dns_rdata_tostruct(
						&rdata, &rdata_a, NULL);
					RUNTIME_CHECK(result == ISC_R_SUCCESS);
					isc_sockaddr_fromin(
						sa, &rdata_a.in_addr, 53);
					dns_rdata_freestruct(&rdata_a);
					break;
				case AF_INET6:
					dns_rdataset_current(rdataset, &rdata);
					result = dns_rdata_tostruct(
						&rdata, &rdata_aaaa, NULL);
					RUNTIME_CHECK(result == ISC_R_SUCCESS);
					isc_sockaddr_fromin6(
						sa, &rdata_aaaa.in6_addr, 53);
					dns_rdata_freestruct(&rdata_aaaa);
					break;
				}

				ISC_LINK_INIT(sa, link);
				ISC_LIST_APPEND(uctx->servers, sa, link);
				uctx->nservers++;
			}
		}
	}

done:
	dns_client_freeresanswer(uctx->client, &rev->answerlist);
	isc_event_free(&event);

	LOCK(&uctx->lock);
	if (uctx->restrans == NULL && uctx->restrans2 == NULL) {
		completed = true;
	}
	UNLOCK(&uctx->lock);

	if (completed) {
		INSIST(uctx->currentserver == NULL);
		uctx->currentserver = ISC_LIST_HEAD(uctx->servers);
		if (uctx->currentserver != NULL && !uctx->canceled) {
			send_update(uctx);
		} else {
			if (result == ISC_R_SUCCESS) {
				result = ISC_R_NOTFOUND;
			}
			update_sendevent(uctx, result);
		}
	}
}

static isc_result_t
process_soa(updatectx_t *uctx, dns_rdataset_t *soaset,
	    const dns_name_t *soaname) {
	isc_result_t result;
	dns_rdata_t soarr = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	dns_name_t primary;
	unsigned int resoptions;

	result = dns_rdataset_first(soaset);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	dns_rdata_init(&soarr);
	dns_rdataset_current(soaset, &soarr);
	result = dns_rdata_tostruct(&soarr, &soa, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_name_init(&primary, NULL);
	dns_name_clone(&soa.origin, &primary);

	if (uctx->zonename == NULL) {
		uctx->zonename = dns_fixedname_name(&uctx->zonefname);
		dns_name_copynf(soaname, uctx->zonename);
	}

	if (uctx->currentserver != NULL) {
		result = send_update(uctx);
	} else {
		/*
		 * Get addresses of the primary server.  We don't use the ADB
		 * feature so that we could avoid caching data.
		 */
		LOCK(&uctx->lock);
		uctx->bp4 = uctx;
		resoptions = 0;
		if (uctx->want_tcp) {
			resoptions |= DNS_CLIENTRESOPT_TCP;
		}
		result = dns_client_startresolve(
			uctx->client, &primary, uctx->rdclass, dns_rdatatype_a,
			resoptions, uctx->client->task, resolveaddr_done,
			&uctx->bp4, &uctx->restrans);
		if (result == ISC_R_SUCCESS) {
			uctx->bp6 = uctx;
			result = dns_client_startresolve(
				uctx->client, &primary, uctx->rdclass,
				dns_rdatatype_aaaa, resoptions,
				uctx->client->task, resolveaddr_done,
				&uctx->bp6, &uctx->restrans2);
		}
		UNLOCK(&uctx->lock);
	}

	dns_rdata_freestruct(&soa);

	return (result);
}

static void
receive_soa(isc_task_t *task, isc_event_t *event) {
	dns_requestevent_t *reqev = NULL;
	updatectx_t *uctx;
	dns_client_t *client;
	isc_result_t result, eresult;
	dns_request_t *request;
	dns_message_t *rcvmsg = NULL;
	dns_section_t section;
	dns_rdataset_t *soaset = NULL;
	int pass = 0;
	dns_name_t *name;
	dns_message_t *soaquery = NULL;
	isc_sockaddr_t *addr;
	bool seencname = false;
	bool droplabel = false;
	dns_name_t tname;
	unsigned int nlabels, reqoptions;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_REQUESTDONE);
	reqev = (dns_requestevent_t *)event;
	request = reqev->request;
	result = eresult = reqev->result;
	POST(result);
	uctx = reqev->ev_arg;
	client = uctx->client;
	soaquery = uctx->soaquery;
	addr = uctx->currentserver;
	INSIST(addr != NULL);

	isc_event_free(&event);

	if (eresult != ISC_R_SUCCESS) {
		result = eresult;
		goto out;
	}

	dns_message_create(uctx->client->mctx, DNS_MESSAGE_INTENTPARSE,
			   &rcvmsg);
	result = dns_request_getresponse(request, rcvmsg,
					 DNS_MESSAGEPARSE_PRESERVEORDER);

	if (result == DNS_R_TSIGERRORSET) {
		dns_request_t *newrequest = NULL;

		/* Retry SOA request without TSIG */
		dns_message_detach(&rcvmsg);
		dns_message_renderreset(uctx->soaquery);
		reqoptions = 0;
		if (uctx->want_tcp) {
			reqoptions |= DNS_REQUESTOPT_TCP;
		}
		result = dns_request_createvia(
			uctx->view->requestmgr, uctx->soaquery, NULL, addr, -1,
			reqoptions, NULL, client->find_timeout * 20,
			client->find_timeout, 3, uctx->client->task,
			receive_soa, uctx, &newrequest);
		if (result == ISC_R_SUCCESS) {
			LOCK(&uctx->lock);
			dns_request_destroy(&uctx->soareq);
			uctx->soareq = newrequest;
			UNLOCK(&uctx->lock);

			return;
		}
		goto out;
	}

	section = DNS_SECTION_ANSWER;
	POST(section);

	if (rcvmsg->rcode != dns_rcode_noerror &&
	    rcvmsg->rcode != dns_rcode_nxdomain) {
		result = rcode2result(rcvmsg->rcode);
		goto out;
	}

lookforsoa:
	if (pass == 0) {
		section = DNS_SECTION_ANSWER;
	} else if (pass == 1) {
		section = DNS_SECTION_AUTHORITY;
	} else {
		droplabel = true;
		goto out;
	}

	result = dns_message_firstname(rcvmsg, section);
	if (result != ISC_R_SUCCESS) {
		pass++;
		goto lookforsoa;
	}
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(rcvmsg, section, &name);
		soaset = NULL;
		result = dns_message_findtype(name, dns_rdatatype_soa, 0,
					      &soaset);
		if (result == ISC_R_SUCCESS) {
			break;
		}
		if (section == DNS_SECTION_ANSWER) {
			dns_rdataset_t *tset = NULL;
			if (dns_message_findtype(name, dns_rdatatype_cname, 0,
						 &tset) == ISC_R_SUCCESS ||
			    dns_message_findtype(name, dns_rdatatype_dname, 0,
						 &tset) == ISC_R_SUCCESS)
			{
				seencname = true;
				break;
			}
		}

		result = dns_message_nextname(rcvmsg, section);
	}

	if (soaset == NULL && !seencname) {
		pass++;
		goto lookforsoa;
	}

	if (seencname) {
		droplabel = true;
		goto out;
	}

	result = process_soa(uctx, soaset, name);

out:
	if (droplabel) {
		result = dns_message_firstname(soaquery, DNS_SECTION_QUESTION);
		INSIST(result == ISC_R_SUCCESS);
		name = NULL;
		dns_message_currentname(soaquery, DNS_SECTION_QUESTION, &name);
		nlabels = dns_name_countlabels(name);
		if (nlabels == 1) {
			result = DNS_R_SERVFAIL; /* is there a better error? */
		} else {
			dns_name_init(&tname, NULL);
			dns_name_getlabelsequence(name, 1, nlabels - 1, &tname);
			dns_name_clone(&tname, name);
			dns_request_destroy(&request);
			LOCK(&uctx->lock);
			uctx->soareq = NULL;
			UNLOCK(&uctx->lock);
			dns_message_renderreset(soaquery);
			dns_message_settsigkey(soaquery, NULL);
			reqoptions = 0;
			if (uctx->want_tcp) {
				reqoptions |= DNS_REQUESTOPT_TCP;
			}
			result = dns_request_createvia(
				uctx->view->requestmgr, soaquery, NULL,
				uctx->currentserver, -1, reqoptions,
				uctx->tsigkey, client->find_timeout * 20,
				client->find_timeout, 3, client->task,
				receive_soa, uctx, &uctx->soareq);
		}
	}

	if (!droplabel || result != ISC_R_SUCCESS) {
		dns_message_detach(&uctx->soaquery);
		LOCK(&uctx->lock);
		dns_request_destroy(&uctx->soareq);
		UNLOCK(&uctx->lock);
	}

	if (rcvmsg != NULL) {
		dns_message_detach(&rcvmsg);
	}

	if (result != ISC_R_SUCCESS) {
		update_sendevent(uctx, result);
	}
}

static isc_result_t
request_soa(updatectx_t *uctx) {
	isc_result_t result;
	dns_message_t *soaquery = uctx->soaquery;
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;
	unsigned int reqoptions;

	if (soaquery == NULL) {
		dns_message_create(uctx->client->mctx, DNS_MESSAGE_INTENTRENDER,
				   &soaquery);
	}
	soaquery->flags |= DNS_MESSAGEFLAG_RD;
	result = dns_message_gettempname(soaquery, &name);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	result = dns_message_gettemprdataset(soaquery, &rdataset);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	dns_rdataset_makequestion(rdataset, uctx->rdclass, dns_rdatatype_soa);
	dns_name_clone(uctx->firstname, name);
	ISC_LIST_APPEND(name->list, rdataset, link);
	dns_message_addname(soaquery, name, DNS_SECTION_QUESTION);
	rdataset = NULL;
	name = NULL;
	reqoptions = 0;
	if (uctx->want_tcp) {
		reqoptions |= DNS_REQUESTOPT_TCP;
	}

	result = dns_request_createvia(
		uctx->view->requestmgr, soaquery, NULL, uctx->currentserver, -1,
		reqoptions, uctx->tsigkey, uctx->client->find_timeout * 20,
		uctx->client->find_timeout, 3, uctx->client->task, receive_soa,
		uctx, &uctx->soareq);
	if (result == ISC_R_SUCCESS) {
		uctx->soaquery = soaquery;
		return (ISC_R_SUCCESS);
	}

fail:
	if (rdataset != NULL) {
		ISC_LIST_UNLINK(name->list, rdataset, link); /* for safety */
		dns_message_puttemprdataset(soaquery, &rdataset);
	}
	if (name != NULL) {
		dns_message_puttempname(soaquery, &name);
	}
	dns_message_detach(&soaquery);

	return (result);
}

static void
resolvesoa_done(isc_task_t *task, isc_event_t *event) {
	dns_clientresevent_t *rev = (dns_clientresevent_t *)event;
	updatectx_t *uctx;
	dns_name_t *name, tname;
	dns_rdataset_t *rdataset = NULL;
	isc_result_t result = rev->result;
	unsigned int nlabels, resoptions;

	UNUSED(task);

	uctx = event->ev_arg;
	REQUIRE(UCTX_VALID(uctx));

	LOCK(&uctx->lock);
	dns_client_destroyrestrans(&uctx->restrans);
	UNLOCK(&uctx->lock);

	uctx = event->ev_arg;
	if (result != ISC_R_SUCCESS && result != DNS_R_NCACHENXDOMAIN &&
	    result != DNS_R_NCACHENXRRSET)
	{
		/* XXX: what about DNSSEC failure? */
		goto out;
	}

	for (name = ISC_LIST_HEAD(rev->answerlist); name != NULL;
	     name = ISC_LIST_NEXT(name, link))
	{
		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (dns_rdataset_isassociated(rdataset) &&
			    rdataset->type == dns_rdatatype_soa) {
				break;
			}
		}
	}

	if (rdataset == NULL) {
		/* Drop one label and retry resolution. */
		nlabels = dns_name_countlabels(&uctx->soaqname);
		if (nlabels == 1) {
			result = DNS_R_SERVFAIL; /* is there a better error? */
			goto out;
		}
		dns_name_init(&tname, NULL);
		dns_name_getlabelsequence(&uctx->soaqname, 1, nlabels - 1,
					  &tname);
		dns_name_clone(&tname, &uctx->soaqname);
		resoptions = 0;
		if (uctx->want_tcp) {
			resoptions |= DNS_CLIENTRESOPT_TCP;
		}

		result = dns_client_startresolve(
			uctx->client, &uctx->soaqname, uctx->rdclass,
			dns_rdatatype_soa, resoptions, uctx->client->task,
			resolvesoa_done, uctx, &uctx->restrans);
	} else {
		result = process_soa(uctx, rdataset, &uctx->soaqname);
	}

out:
	dns_client_freeresanswer(uctx->client, &rev->answerlist);
	isc_event_free(&event);

	if (result != ISC_R_SUCCESS) {
		update_sendevent(uctx, result);
	}
}

static isc_result_t
copy_name(isc_mem_t *mctx, dns_message_t *msg, const dns_name_t *name,
	  dns_name_t **newnamep) {
	isc_result_t result;
	dns_name_t *newname = NULL;
	isc_region_t r;
	isc_buffer_t *namebuf = NULL, *rdatabuf = NULL;
	dns_rdatalist_t *rdatalist;
	dns_rdataset_t *rdataset, *newrdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT, *newrdata;

	result = dns_message_gettempname(msg, &newname);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	isc_buffer_allocate(mctx, &namebuf, DNS_NAME_MAXWIRE);
	dns_name_init(newname, NULL);
	dns_name_setbuffer(newname, namebuf);
	dns_message_takebuffer(msg, &namebuf);
	dns_name_copynf(name, newname);

	for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		rdatalist = NULL;
		result = dns_message_gettemprdatalist(msg, &rdatalist);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
		dns_rdatalist_init(rdatalist);
		rdatalist->type = rdataset->type;
		rdatalist->rdclass = rdataset->rdclass;
		rdatalist->covers = rdataset->covers;
		rdatalist->ttl = rdataset->ttl;

		result = dns_rdataset_first(rdataset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(rdataset, &rdata);

			newrdata = NULL;
			result = dns_message_gettemprdata(msg, &newrdata);
			if (result != ISC_R_SUCCESS) {
				goto fail;
			}
			dns_rdata_toregion(&rdata, &r);
			rdatabuf = NULL;
			isc_buffer_allocate(mctx, &rdatabuf, r.length);
			isc_buffer_putmem(rdatabuf, r.base, r.length);
			isc_buffer_usedregion(rdatabuf, &r);
			dns_rdata_init(newrdata);
			dns_rdata_fromregion(newrdata, rdata.rdclass,
					     rdata.type, &r);
			newrdata->flags = rdata.flags;

			ISC_LIST_APPEND(rdatalist->rdata, newrdata, link);
			dns_message_takebuffer(msg, &rdatabuf);

			result = dns_rdataset_next(rdataset);
		}

		newrdataset = NULL;
		result = dns_message_gettemprdataset(msg, &newrdataset);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
		dns_rdatalist_tordataset(rdatalist, newrdataset);

		ISC_LIST_APPEND(newname->list, newrdataset, link);
	}

	*newnamep = newname;

	return (ISC_R_SUCCESS);

fail:
	dns_message_puttempname(msg, &newname);

	return (result);
}

static void
internal_update_callback(isc_task_t *task, isc_event_t *event) {
	updatearg_t *uarg = event->ev_arg;
	dns_clientupdateevent_t *uev = (dns_clientupdateevent_t *)event;

	UNUSED(task);

	LOCK(&uarg->lock);

	uarg->result = uev->result;

	dns_client_destroyupdatetrans(&uarg->trans);
	isc_event_free(&event);

	if (!uarg->canceled) {
		UNLOCK(&uarg->lock);

		/* Exit from the internal event loop */
		isc_app_ctxsuspend(uarg->actx);
	} else {
		/*
		 * We have already exited from the loop (due to some
		 * unexpected event).  Just clean the arg up.
		 */
		UNLOCK(&uarg->lock);
		isc_mutex_destroy(&uarg->lock);
		isc_mem_put(uarg->client->mctx, uarg, sizeof(*uarg));
	}
}

isc_result_t
dns_client_update(dns_client_t *client, dns_rdataclass_t rdclass,
		  const dns_name_t *zonename, dns_namelist_t *prerequisites,
		  dns_namelist_t *updates, isc_sockaddrlist_t *servers,
		  dns_tsec_t *tsec, unsigned int options) {
	isc_result_t result;
	isc_appctx_t *actx;
	updatearg_t *uarg;

	REQUIRE(DNS_CLIENT_VALID(client));

	if ((client->attributes & DNS_CLIENTATTR_OWNCTX) == 0 &&
	    (options & DNS_CLIENTUPDOPT_ALLOWRUN) == 0)
	{
		/*
		 * If the client is run under application's control, we need
		 * to create a new running (sub)environment for this
		 * particular update.
		 */
		return (ISC_R_NOTIMPLEMENTED); /* XXXTBD */
	} else {
		actx = client->actx;
	}

	uarg = isc_mem_get(client->mctx, sizeof(*uarg));

	isc_mutex_init(&uarg->lock);

	uarg->actx = actx;
	uarg->client = client;
	uarg->result = ISC_R_FAILURE;
	uarg->trans = NULL;
	uarg->canceled = false;

	result = dns_client_startupdate(
		client, rdclass, zonename, prerequisites, updates, servers,
		tsec, options, client->task, internal_update_callback, uarg,
		&uarg->trans);
	if (result != ISC_R_SUCCESS) {
		isc_mutex_destroy(&uarg->lock);
		isc_mem_put(client->mctx, uarg, sizeof(*uarg));
		return (result);
	}

	/*
	 * Start internal event loop.  It blocks until the entire process
	 * is completed.
	 */
	result = isc_app_ctxrun(actx);

	LOCK(&uarg->lock);
	if (result == ISC_R_SUCCESS || result == ISC_R_SUSPEND) {
		result = uarg->result;
	}

	if (uarg->trans != NULL) {
		/*
		 * Unusual termination (perhaps due to signal).  We need some
		 * tricky cleanup process.
		 */
		uarg->canceled = true;
		dns_client_cancelupdate(uarg->trans);

		UNLOCK(&uarg->lock);

		/* uarg will be freed in the event handler. */
	} else {
		UNLOCK(&uarg->lock);

		isc_mutex_destroy(&uarg->lock);
		isc_mem_put(client->mctx, uarg, sizeof(*uarg));
	}

	return (result);
}

static void
startupdate(isc_task_t *task, isc_event_t *event) {
	updatectx_t *uctx;
	isc_result_t result;
	unsigned int resoptions;

	REQUIRE(event != NULL);

	UNUSED(task);

	uctx = event->ev_arg;

	if (uctx->zonename != NULL && uctx->currentserver != NULL) {
		result = send_update(uctx);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	} else if (uctx->currentserver != NULL) {
		result = request_soa(uctx);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	} else {
		resoptions = 0;
		if (uctx->want_tcp) {
			resoptions |= DNS_CLIENTRESOPT_TCP;
		}
		dns_name_clone(uctx->firstname, &uctx->soaqname);
		result = dns_client_startresolve(
			uctx->client, &uctx->soaqname, uctx->rdclass,
			dns_rdatatype_soa, resoptions, uctx->client->task,
			resolvesoa_done, uctx, &uctx->restrans);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	}

	isc_event_free(&event);

fail:
	if (result != ISC_R_SUCCESS) {
		update_sendevent(uctx, result);
	}
}

isc_result_t
dns_client_startupdate(dns_client_t *client, dns_rdataclass_t rdclass,
		       const dns_name_t *zonename,
		       dns_namelist_t *prerequisites, dns_namelist_t *updates,
		       isc_sockaddrlist_t *servers, dns_tsec_t *tsec,
		       unsigned int options, isc_task_t *task,
		       isc_taskaction_t action, void *arg,
		       dns_clientupdatetrans_t **transp) {
	dns_view_t *view = NULL;
	isc_result_t result;
	dns_name_t *name, *newname;
	updatectx_t *uctx;
	isc_task_t *tclone = NULL;
	dns_section_t section = DNS_SECTION_UPDATE;
	isc_sockaddr_t *server, *sa = NULL;
	dns_tsectype_t tsectype = dns_tsectype_none;
	bool want_tcp;

	UNUSED(options);

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(transp != NULL && *transp == NULL);
	REQUIRE(updates != NULL);
	REQUIRE(task != NULL);

	if (tsec != NULL) {
		tsectype = dns_tsec_gettype(tsec);
		if (tsectype != dns_tsectype_tsig) {
			return (ISC_R_NOTIMPLEMENTED); /* XXX */
		}
	}

	LOCK(&client->lock);
	result = dns_viewlist_find(&client->viewlist, DNS_CLIENTVIEW_NAME,
				   rdclass, &view);
	UNLOCK(&client->lock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	want_tcp = ((options & DNS_CLIENTUPDOPT_TCP) != 0);

	/*
	 * Create a context and prepare some resources.
	 */

	uctx = isc_mem_get(client->mctx, sizeof(*uctx));

	isc_mutex_init(&uctx->lock);

	tclone = NULL;
	isc_task_attach(task, &tclone);
	uctx->client = client;
	ISC_LINK_INIT(uctx, link);
	uctx->state = dns_clientupdatestate_prepare;
	uctx->view = view;
	uctx->rdclass = rdclass;
	uctx->canceled = false;
	uctx->updatemsg = NULL;
	uctx->soaquery = NULL;
	uctx->updatereq = NULL;
	uctx->restrans = NULL;
	uctx->restrans2 = NULL;
	uctx->bp4 = NULL;
	uctx->bp6 = NULL;
	uctx->soareq = NULL;
	uctx->event = NULL;
	uctx->tsigkey = NULL;
	uctx->sig0key = NULL;
	uctx->zonename = NULL;
	uctx->want_tcp = want_tcp;
	dns_name_init(&uctx->soaqname, NULL);
	ISC_LIST_INIT(uctx->servers);
	uctx->nservers = 0;
	uctx->currentserver = NULL;
	dns_fixedname_init(&uctx->zonefname);
	if (tsec != NULL) {
		dns_tsec_getkey(tsec, &uctx->tsigkey);
	}
	uctx->event = (dns_clientupdateevent_t *)isc_event_allocate(
		client->mctx, tclone, DNS_EVENT_UPDATEDONE, action, arg,
		sizeof(*uctx->event));
	if (zonename != NULL) {
		uctx->zonename = dns_fixedname_name(&uctx->zonefname);
		dns_name_copynf(zonename, uctx->zonename);
	}
	if (servers != NULL) {
		for (server = ISC_LIST_HEAD(*servers); server != NULL;
		     server = ISC_LIST_NEXT(server, link))
		{
			sa = isc_mem_get(client->mctx, sizeof(*sa));
			sa->type = server->type;
			sa->length = server->length;
			ISC_LINK_INIT(sa, link);
			ISC_LIST_APPEND(uctx->servers, sa, link);
			if (uctx->currentserver == NULL) {
				uctx->currentserver = sa;
			}
			uctx->nservers++;
		}
	}

	/* Make update message */
	dns_message_create(client->mctx, DNS_MESSAGE_INTENTRENDER,
			   &uctx->updatemsg);
	uctx->updatemsg->opcode = dns_opcode_update;

	if (prerequisites != NULL) {
		for (name = ISC_LIST_HEAD(*prerequisites); name != NULL;
		     name = ISC_LIST_NEXT(name, link))
		{
			newname = NULL;
			result = copy_name(client->mctx, uctx->updatemsg, name,
					   &newname);
			if (result != ISC_R_SUCCESS) {
				goto fail;
			}
			dns_message_addname(uctx->updatemsg, newname,
					    DNS_SECTION_PREREQUISITE);
		}
	}

	for (name = ISC_LIST_HEAD(*updates); name != NULL;
	     name = ISC_LIST_NEXT(name, link))
	{
		newname = NULL;
		result = copy_name(client->mctx, uctx->updatemsg, name,
				   &newname);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
		dns_message_addname(uctx->updatemsg, newname,
				    DNS_SECTION_UPDATE);
	}

	uctx->firstname = NULL;
	result = dns_message_firstname(uctx->updatemsg, section);
	if (result == ISC_R_NOMORE) {
		section = DNS_SECTION_PREREQUISITE;
		result = dns_message_firstname(uctx->updatemsg, section);
	}
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	dns_message_currentname(uctx->updatemsg, section, &uctx->firstname);

	uctx->magic = UCTX_MAGIC;

	LOCK(&client->lock);
	ISC_LIST_APPEND(client->updatectxs, uctx, link);
	isc_refcount_increment(&client->references);
	UNLOCK(&client->lock);

	*transp = (dns_clientupdatetrans_t *)uctx;
	result = isc_app_ctxonrun(client->actx, client->mctx, client->task,
				  startupdate, uctx);
	if (result == ISC_R_ALREADYRUNNING) {
		isc_event_t *event;
		event = isc_event_allocate(client->mctx, dns_client_startupdate,
					   DNS_EVENT_STARTUPDATE, startupdate,
					   uctx, sizeof(*event));
		result = ISC_R_SUCCESS;
		isc_task_send(task, &event);
	}
	if (result == ISC_R_SUCCESS) {
		return (result);
	}

	isc_refcount_decrement1(&client->references);
	*transp = NULL;

fail:
	if (ISC_LINK_LINKED(uctx, link)) {
		LOCK(&client->lock);
		ISC_LIST_UNLINK(client->updatectxs, uctx, link);
		UNLOCK(&client->lock);
	}
	if (uctx->updatemsg != NULL) {
		dns_message_detach(&uctx->updatemsg);
	}
	while ((sa = ISC_LIST_HEAD(uctx->servers)) != NULL) {
		ISC_LIST_UNLINK(uctx->servers, sa, link);
		isc_mem_put(client->mctx, sa, sizeof(*sa));
	}
	if (uctx->event != NULL) {
		isc_event_free(ISC_EVENT_PTR(&uctx->event));
	}
	if (uctx->tsigkey != NULL) {
		dns_tsigkey_detach(&uctx->tsigkey);
	}
	isc_task_detach(&tclone);
	isc_mutex_destroy(&uctx->lock);
	uctx->magic = 0;
	isc_mem_put(client->mctx, uctx, sizeof(*uctx));
	dns_view_detach(&view);

	return (result);
}

void
dns_client_cancelupdate(dns_clientupdatetrans_t *trans) {
	updatectx_t *uctx;

	REQUIRE(trans != NULL);
	uctx = (updatectx_t *)trans;
	REQUIRE(UCTX_VALID(uctx));

	LOCK(&uctx->lock);

	if (!uctx->canceled) {
		uctx->canceled = true;
		if (uctx->updatereq != NULL) {
			dns_request_cancel(uctx->updatereq);
		}
		if (uctx->soareq != NULL) {
			dns_request_cancel(uctx->soareq);
		}
		if (uctx->restrans != NULL) {
			dns_client_cancelresolve(uctx->restrans);
		}
		if (uctx->restrans2 != NULL) {
			dns_client_cancelresolve(uctx->restrans2);
		}
	}

	UNLOCK(&uctx->lock);
}

void
dns_client_destroyupdatetrans(dns_clientupdatetrans_t **transp) {
	updatectx_t *uctx;
	isc_mem_t *mctx;
	dns_client_t *client;
	isc_sockaddr_t *sa;

	REQUIRE(transp != NULL);
	uctx = (updatectx_t *)*transp;
	*transp = NULL;
	REQUIRE(UCTX_VALID(uctx));
	client = uctx->client;
	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(uctx->updatereq == NULL && uctx->updatemsg == NULL &&
		uctx->soareq == NULL && uctx->soaquery == NULL &&
		uctx->event == NULL && uctx->tsigkey == NULL &&
		uctx->sig0key == NULL);

	mctx = client->mctx;
	dns_view_detach(&uctx->view);
	while ((sa = ISC_LIST_HEAD(uctx->servers)) != NULL) {
		ISC_LIST_UNLINK(uctx->servers, sa, link);
		isc_mem_put(mctx, sa, sizeof(*sa));
	}

	LOCK(&client->lock);

	INSIST(ISC_LINK_LINKED(uctx, link));
	ISC_LIST_UNLINK(client->updatectxs, uctx, link);

	UNLOCK(&client->lock);

	isc_mutex_destroy(&uctx->lock);
	uctx->magic = 0;

	isc_mem_put(mctx, uctx, sizeof(*uctx));

	dns_client_destroy(&client);
}

isc_mem_t *
dns_client_mctx(dns_client_t *client) {
	REQUIRE(DNS_CLIENT_VALID(client));
	return (client->mctx);
}

typedef struct {
	isc_buffer_t buffer;
	dns_rdataset_t rdataset;
	dns_rdatalist_t rdatalist;
	dns_rdata_t rdata;
	size_t size;
	isc_mem_t *mctx;
	unsigned char data[FLEXIBLE_ARRAY_MEMBER];
} dns_client_updaterec_t;

isc_result_t
dns_client_updaterec(dns_client_updateop_t op, const dns_name_t *owner,
		     dns_rdatatype_t type, dns_rdata_t *source, dns_ttl_t ttl,
		     dns_name_t *target, dns_rdataset_t *rdataset,
		     dns_rdatalist_t *rdatalist, dns_rdata_t *rdata,
		     isc_mem_t *mctx) {
	dns_client_updaterec_t *updaterec = NULL;
	size_t size = offsetof(dns_client_updaterec_t, data);

	REQUIRE(op < updateop_max);
	REQUIRE(owner != NULL);
	REQUIRE((rdataset != NULL && rdatalist != NULL && rdata != NULL) ||
		(rdataset == NULL && rdatalist == NULL && rdata == NULL &&
		 mctx != NULL));
	if (op == updateop_add) {
		REQUIRE(source != NULL);
	}
	if (source != NULL) {
		REQUIRE(source->type == type);
		REQUIRE(op == updateop_add || op == updateop_delete ||
			op == updateop_exist);
	}

	size += owner->length;
	if (source != NULL) {
		size += source->length;
	}

	if (rdataset == NULL) {
		updaterec = isc_mem_get(mctx, size);
		rdataset = &updaterec->rdataset;
		rdatalist = &updaterec->rdatalist;
		rdata = &updaterec->rdata;
		dns_rdataset_init(rdataset);
		dns_rdatalist_init(&updaterec->rdatalist);
		dns_rdata_init(&updaterec->rdata);
		isc_buffer_init(
			&updaterec->buffer, updaterec->data,
			(unsigned int)(size -
				       offsetof(dns_client_updaterec_t, data)));
		dns_name_copy(owner, target, &updaterec->buffer);
		if (source != NULL) {
			isc_region_t r;
			dns_rdata_clone(source, rdata);
			dns_rdata_toregion(rdata, &r);
			rdata->data = isc_buffer_used(&updaterec->buffer);
			isc_buffer_copyregion(&updaterec->buffer, &r);
		}
		updaterec->mctx = NULL;
		isc_mem_attach(mctx, &updaterec->mctx);
	} else if (source != NULL) {
		dns_rdata_clone(source, rdata);
	}

	switch (op) {
	case updateop_add:
		break;
	case updateop_delete:
		if (source != NULL) {
			ttl = 0;
			dns_rdata_makedelete(rdata);
		} else {
			dns_rdata_deleterrset(rdata, type);
		}
		break;
	case updateop_notexist:
		dns_rdata_notexist(rdata, type);
		break;
	case updateop_exist:
		if (source == NULL) {
			ttl = 0;
			dns_rdata_exists(rdata, type);
		}
	case updateop_none:
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	rdatalist->type = rdata->type;
	rdatalist->rdclass = rdata->rdclass;
	if (source != NULL) {
		rdatalist->covers = dns_rdata_covers(rdata);
		rdatalist->ttl = ttl;
	}
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	ISC_LIST_APPEND(target->list, rdataset, link);
	if (updaterec != NULL) {
		target->attributes |= DNS_NAMEATTR_HASUPDATEREC;
		dns_name_setbuffer(target, &updaterec->buffer);
	}
	if (op == updateop_add || op == updateop_delete) {
		target->attributes |= DNS_NAMEATTR_UPDATE;
	} else {
		target->attributes |= DNS_NAMEATTR_PREREQUISITE;
	}
	return (ISC_R_SUCCESS);
}

void
dns_client_freeupdate(dns_name_t **namep) {
	dns_client_updaterec_t *updaterec;
	dns_rdatalist_t *rdatalist;
	dns_rdataset_t *rdataset;
	dns_rdata_t *rdata;
	dns_name_t *name;

	REQUIRE(namep != NULL && *namep != NULL);

	name = *namep;
	for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
	     rdataset = ISC_LIST_HEAD(name->list))
	{
		ISC_LIST_UNLINK(name->list, rdataset, link);
		rdatalist = NULL;
		dns_rdatalist_fromrdataset(rdataset, &rdatalist);
		if (rdatalist == NULL) {
			dns_rdataset_disassociate(rdataset);
			continue;
		}
		for (rdata = ISC_LIST_HEAD(rdatalist->rdata); rdata != NULL;
		     rdata = ISC_LIST_HEAD(rdatalist->rdata))
		{
			ISC_LIST_UNLINK(rdatalist->rdata, rdata, link);
		}
		dns_rdataset_disassociate(rdataset);
	}

	if ((name->attributes & DNS_NAMEATTR_HASUPDATEREC) != 0) {
		updaterec = (dns_client_updaterec_t *)name->buffer;
		INSIST(updaterec != NULL);
		isc_mem_putanddetach(&updaterec->mctx, updaterec,
				     updaterec->size);
		*namep = NULL;
	}
}
