/*	$NetBSD: client.c,v 1.15 2025/01/26 16:25:22 christos Exp $	*/

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

#include <isc/async.h>
#include <isc/buffer.h>
#include <isc/loop.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/netmgr.h>
#include <isc/portset.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/client.h>
#include <dns/db.h>
#include <dns/dispatch.h>
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
#include <dns/tsig.h>
#include <dns/view.h>

#include <dst/dst.h>

#define DNS_CLIENT_MAGIC    ISC_MAGIC('D', 'N', 'S', 'c')
#define DNS_CLIENT_VALID(c) ISC_MAGIC_VALID(c, DNS_CLIENT_MAGIC)

#define RCTX_MAGIC    ISC_MAGIC('R', 'c', 't', 'x')
#define RCTX_VALID(c) ISC_MAGIC_VALID(c, RCTX_MAGIC)

#define UCTX_MAGIC    ISC_MAGIC('U', 'c', 't', 'x')
#define UCTX_VALID(c) ISC_MAGIC_VALID(c, UCTX_MAGIC)

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
	unsigned int magic;
	unsigned int attributes;
	isc_mem_t *mctx;
	isc_loop_t *loop;
	isc_nm_t *nm;
	dns_dispatchmgr_t *dispatchmgr;
	dns_dispatch_t *dispatchv4;
	dns_dispatch_t *dispatchv6;

	unsigned int find_timeout;
	unsigned int find_udpretries;
	uint8_t max_restarts;

	isc_refcount_t references;

	dns_view_t *view;
	ISC_LIST(struct resctx) resctxs;
};

#define DEF_FIND_TIMEOUT    5
#define DEF_FIND_UDPRETRIES 3
#define DEF_MAX_RESTARTS    11

/*%
 * Internal state for a single name resolution procedure
 */
typedef struct resctx {
	unsigned int magic;
	dns_client_t *client;
	bool want_dnssec;
	bool want_validation;
	bool want_cdflag;
	bool want_tcp;

	ISC_LINK(struct resctx) link;
	dns_view_t *view;
	unsigned int restarts;
	dns_fixedname_t name;
	dns_rdatatype_t type;
	dns_fetch_t *fetch;
	dns_namelist_t namelist;
	isc_result_t result;
	dns_clientresume_t *rev;
	dns_rdataset_t *rdataset;
	dns_rdataset_t *sigrdataset;
} resctx_t;

/*%
 * Argument of an internal event for synchronous name resolution.
 */
typedef struct resarg {
	isc_mem_t *mctx;
	dns_client_t *client;
	const dns_name_t *name;

	isc_result_t result;
	isc_result_t vresult;
	dns_namelist_t *namelist;
	dns_clientrestrans_t *trans;
	dns_client_resolve_cb resolve_cb;
} resarg_t;

static void
client_resfind(resctx_t *rctx, dns_fetchresponse_t *event);
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

	return result;
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

	return result;
}

static isc_result_t
createview(isc_mem_t *mctx, dns_rdataclass_t rdclass, isc_nm_t *nm,
	   isc_tlsctx_cache_t *tlsctx_client_cache, isc_loopmgr_t *loopmgr,
	   dns_dispatchmgr_t *dispatchmgr, dns_dispatch_t *dispatchv4,
	   dns_dispatch_t *dispatchv6, dns_view_t **viewp) {
	isc_result_t result;
	dns_view_t *view = NULL;

	result = dns_view_create(mctx, loopmgr, dispatchmgr, rdclass,
				 DNS_CLIENTVIEW_NAME, &view);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	/* Initialize view security roots */
	dns_view_initsecroots(view);

	CHECK(dns_view_createresolver(view, nm, 0, tlsctx_client_cache,
				      dispatchv4, dispatchv6));
	CHECK(dns_db_create(mctx, CACHEDB_DEFAULT, dns_rootname,
			    dns_dbtype_cache, rdclass, 0, NULL,
			    &view->cachedb));

	*viewp = view;
	return ISC_R_SUCCESS;

cleanup:
	dns_view_detach(&view);
	return result;
}

isc_result_t
dns_client_create(isc_mem_t *mctx, isc_loopmgr_t *loopmgr, isc_nm_t *nm,
		  unsigned int options, isc_tlsctx_cache_t *tlsctx_client_cache,
		  dns_client_t **clientp, const isc_sockaddr_t *localaddr4,
		  const isc_sockaddr_t *localaddr6) {
	isc_result_t result;
	dns_client_t *client = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_dispatch_t *dispatchv6 = NULL;
	dns_view_t *view = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(nm != NULL);
	REQUIRE(tlsctx_client_cache != NULL);
	REQUIRE(clientp != NULL && *clientp == NULL);

	UNUSED(options);

	client = isc_mem_get(mctx, sizeof(*client));
	*client = (dns_client_t){
		.loop = isc_loop_get(loopmgr, 0),
		.nm = nm,
		.max_restarts = DEF_MAX_RESTARTS,
	};

	result = dns_dispatchmgr_create(mctx, loopmgr, nm,
					&client->dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_client;
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
	result = createview(mctx, dns_rdataclass_in, nm, tlsctx_client_cache,
			    isc_loop_getloopmgr(client->loop),
			    client->dispatchmgr, dispatchv4, dispatchv6, &view);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_references;
	}

	client->view = view;

	dns_view_freeze(view); /* too early? */

	ISC_LIST_INIT(client->resctxs);

	isc_mem_attach(mctx, &client->mctx);

	client->find_timeout = DEF_FIND_TIMEOUT;
	client->find_udpretries = DEF_FIND_UDPRETRIES;

	client->magic = DNS_CLIENT_MAGIC;

	*clientp = client;

	return ISC_R_SUCCESS;

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
cleanup_client:
	isc_mem_put(mctx, client, sizeof(*client));

	return result;
}

static void
destroyclient(dns_client_t *client) {
	isc_refcount_destroy(&client->references);

	dns_view_detach(&client->view);

	if (client->dispatchv4 != NULL) {
		dns_dispatch_detach(&client->dispatchv4);
	}
	if (client->dispatchv6 != NULL) {
		dns_dispatch_detach(&client->dispatchv6);
	}

	dns_dispatchmgr_detach(&client->dispatchmgr);

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

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(addrs != NULL);
	REQUIRE(rdclass == dns_rdataclass_in);

	if (name_space == NULL) {
		name_space = dns_rootname;
	}

	result = dns_fwdtable_add(client->view->fwdtable, name_space, addrs,
				  dns_fwdpolicy_only);

	return result;
}

void
dns_client_setmaxrestarts(dns_client_t *client, uint8_t max_restarts) {
	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(max_restarts > 0);

	client->max_restarts = max_restarts;
}

static isc_result_t
getrdataset(isc_mem_t *mctx, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset;

	REQUIRE(mctx != NULL);
	REQUIRE(rdatasetp != NULL && *rdatasetp == NULL);

	rdataset = isc_mem_get(mctx, sizeof(*rdataset));

	dns_rdataset_init(rdataset);

	*rdatasetp = rdataset;

	return ISC_R_SUCCESS;
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
fetch_done(void *arg) {
	dns_fetchresponse_t *resp = (dns_fetchresponse_t *)arg;
	resctx_t *rctx = resp->arg;

	REQUIRE(RCTX_VALID(rctx));

	client_resfind(rctx, resp);
}

static isc_result_t
start_fetch(resctx_t *rctx) {
	isc_result_t result;
	int fopts = 0;

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
		rctx->client->loop, fetch_done, rctx, rctx->rdataset,
		rctx->sigrdataset, &rctx->fetch);

	return result;
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

	return result;
}

static void
client_resfind(resctx_t *rctx, dns_fetchresponse_t *resp) {
	isc_mem_t *mctx = NULL;
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

	mctx = rctx->view->mctx;

	name = dns_fixedname_name(&rctx->name);

	do {
		dns_name_t *fname = NULL;
		dns_name_t *ansname = NULL;
		dns_db_t *db = NULL;
		dns_dbnode_t *node = NULL;

		rctx->restarts++;
		want_restart = false;

		if (resp == NULL) {
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
			INSIST(resp != NULL);
			INSIST(resp->fetch == rctx->fetch);
			dns_resolver_destroyfetch(&rctx->fetch);
			db = resp->db;
			node = resp->node;
			result = resp->result;
			vresult = resp->vresult;
			fname = resp->foundname;
			INSIST(resp->rdataset == rctx->rdataset);
			INSIST(resp->sigrdataset == rctx->sigrdataset);
			isc_mem_putanddetach(&resp->mctx, resp, sizeof(*resp));
		}

		/*
		 * Get some resource for copying the
		 * result.
		 */
		dns_name_t *aname = dns_fixedname_name(&rctx->name);

		ansname = isc_mem_get(mctx, sizeof(*ansname));
		dns_name_init(ansname, NULL);

		dns_name_dup(aname, mctx, ansname);

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

		/*
		 * Limit the number of restarts.
		 */
		if (want_restart &&
		    rctx->restarts == rctx->client->max_restarts)
		{
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
		while ((name = ISC_LIST_HEAD(rctx->namelist)) != NULL) {
			ISC_LIST_UNLINK(rctx->namelist, name, link);
			ISC_LIST_APPEND(rctx->rev->answerlist, name, link);
		}

		rctx->rev->result = result;
		rctx->rev->vresult = vresult;
		isc_async_run(rctx->client->loop, rctx->rev->cb, rctx->rev);
	}
}

static void
resolve_done(void *arg) {
	dns_clientresume_t *rev = (dns_clientresume_t *)arg;
	resarg_t *resarg = rev->arg;
	dns_name_t *name = NULL;
	isc_result_t result;

	resarg->result = rev->result;
	resarg->vresult = rev->vresult;
	while ((name = ISC_LIST_HEAD(rev->answerlist)) != NULL) {
		ISC_LIST_UNLINK(rev->answerlist, name, link);
		ISC_LIST_APPEND(*resarg->namelist, name, link);
	}

	isc_mem_put(resarg->mctx, rev, sizeof(*rev));
	destroyrestrans(&resarg->trans);

	result = resarg->result;

	if (result != ISC_R_SUCCESS && resarg->vresult != ISC_R_SUCCESS) {
		/*
		 * If this lookup failed due to some error in DNSSEC
		 * validation, return the validation error code.
		 * XXX: or should we pass the validation result separately?
		 */
		result = resarg->vresult;
	}

	resarg->resolve_cb(resarg->client, resarg->name, resarg->namelist,
			   result);

	dns_client_detach(&resarg->client);

	isc_mem_putanddetach(&resarg->mctx, resarg, sizeof(*resarg));
}

static isc_result_t
startresolve(dns_client_t *client, const dns_name_t *name,
	     dns_rdataclass_t rdclass, dns_rdatatype_t type,
	     unsigned int options, isc_job_cb cb, void *arg,
	     dns_clientrestrans_t **transp) {
	dns_clientresume_t *rev = NULL;
	resctx_t *rctx = NULL;
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	bool want_dnssec, want_validation, want_cdflag, want_tcp;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(transp != NULL && *transp == NULL);
	REQUIRE(rdclass == dns_rdataclass_in);

	mctx = client->mctx;
	want_dnssec = ((options & DNS_CLIENTRESOPT_NODNSSEC) == 0);
	want_validation = ((options & DNS_CLIENTRESOPT_NOVALIDATE) == 0);
	want_cdflag = ((options & DNS_CLIENTRESOPT_NOCDFLAG) == 0);
	want_tcp = ((options & DNS_CLIENTRESOPT_TCP) != 0);

	/*
	 * Prepare some intermediate resources
	 */
	rev = isc_mem_get(mctx, sizeof(*rev));
	*rev = (dns_clientresume_t){
		.result = DNS_R_SERVFAIL,
		.answerlist = ISC_LIST_INITIALIZER,
		.cb = cb,
		.arg = arg,
	};

	rctx = isc_mem_get(mctx, sizeof(*rctx));
	*rctx = (resctx_t){
		.client = client,
		.rev = rev,
		.type = type,
		.want_dnssec = want_dnssec,
		.want_validation = want_validation,
		.want_cdflag = want_cdflag,
		.want_tcp = want_tcp,
		.namelist = ISC_LIST_INITIALIZER,
		.link = ISC_LINK_INITIALIZER,
	};

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

	dns_view_attach(client->view, &rctx->view);

	rctx->magic = RCTX_MAGIC;
	isc_refcount_increment(&client->references);

	ISC_LIST_APPEND(client->resctxs, rctx, link);

	*transp = (dns_clientrestrans_t *)rctx;
	client_resfind(rctx, NULL);

	return ISC_R_SUCCESS;

cleanup:
	if (rdataset != NULL) {
		putrdataset(client->mctx, &rdataset);
	}
	if (sigrdataset != NULL) {
		putrdataset(client->mctx, &sigrdataset);
	}
	isc_mem_put(mctx, rctx, sizeof(*rctx));
	isc_mem_put(mctx, rev, sizeof(*rev));

	return result;
}

isc_result_t
dns_client_resolve(dns_client_t *client, const dns_name_t *name,
		   dns_rdataclass_t rdclass, dns_rdatatype_t type,
		   unsigned int options, dns_namelist_t *namelist,
		   dns_client_resolve_cb resolve_cb) {
	isc_result_t result;
	resarg_t *resarg = NULL;

	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(namelist != NULL && ISC_LIST_EMPTY(*namelist));
	REQUIRE(rdclass == dns_rdataclass_in);

	resarg = isc_mem_get(client->mctx, sizeof(*resarg));

	*resarg = (resarg_t){
		.client = client,
		.name = name,
		.result = DNS_R_SERVFAIL,
		.namelist = namelist,
		.resolve_cb = resolve_cb,
	};

	isc_mem_attach(client->mctx, &resarg->mctx);

	result = startresolve(client, name, rdclass, type, options,
			      resolve_done, resarg, &resarg->trans);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(client->mctx, resarg, sizeof(*resarg));
		return result;
	}

	return result;
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

	client = rctx->client;

	REQUIRE(DNS_CLIENT_VALID(client));

	mctx = client->mctx;
	dns_view_detach(&rctx->view);

	INSIST(ISC_LINK_LINKED(rctx, link));
	ISC_LIST_UNLINK(client->resctxs, rctx, link);

	INSIST(ISC_LIST_EMPTY(rctx->namelist));

	rctx->magic = 0;

	isc_mem_put(mctx, rctx, sizeof(*rctx));
}

isc_result_t
dns_client_addtrustedkey(dns_client_t *client, dns_rdataclass_t rdclass,
			 dns_rdatatype_t rdtype, const dns_name_t *keyname,
			 isc_buffer_t *databuf) {
	REQUIRE(DNS_CLIENT_VALID(client));
	REQUIRE(rdclass == dns_rdataclass_in);

	return dns_view_addtrustedkey(client->view, rdtype, keyname, databuf);
}
