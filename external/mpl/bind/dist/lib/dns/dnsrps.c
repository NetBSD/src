/*	$NetBSD: dnsrps.c,v 1.9 2023/01/25 21:43:30 christos Exp $	*/

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

#ifdef USE_DNSRPS

#include <stdlib.h>

#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#define LIBRPZ_LIB_OPEN DNSRPS_LIB_OPEN
#include <dns/dnsrps.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/result.h>
#include <dns/rpz.h>

librpz_t *librpz;
librpz_emsg_t librpz_lib_open_emsg;
static void *librpz_handle;

#define RPSDB_MAGIC	   ISC_MAGIC('R', 'P', 'Z', 'F')
#define VALID_RPSDB(rpsdb) ((rpsdb)->common.impmagic == RPSDB_MAGIC)

#define RD_DB(r)      ((r)->private1)
#define RD_CUR_RR(r)  ((r)->private2)
#define RD_NEXT_RR(r) ((r)->resign)
#define RD_COUNT(r)   ((r)->privateuint4)

typedef struct {
	dns_rdatasetiter_t common;
	dns_rdatatype_t type;
	dns_rdataclass_t class;
	uint32_t ttl;
	uint count;
	librpz_idx_t next_rr;
} rpsdb_rdatasetiter_t;

static dns_dbmethods_t rpsdb_db_methods;
static dns_rdatasetmethods_t rpsdb_rdataset_methods;
static dns_rdatasetitermethods_t rpsdb_rdatasetiter_methods;

static librpz_clist_t *clist;

static isc_mutex_t dnsrps_mutex;

static void
dnsrps_lock(void *mutex0) {
	isc_mutex_t *mutex = mutex0;

	LOCK(mutex);
}

static void
dnsrps_unlock(void *mutex0) {
	isc_mutex_t *mutex = mutex0;

	UNLOCK(mutex);
}

static void
dnsrps_mutex_destroy(void *mutex0) {
	isc_mutex_t *mutex = mutex0;

	isc_mutex_destroy(mutex);
}

static void
dnsrps_log_fnc(librpz_log_level_t level, void *ctxt, const char *buf) {
	int isc_level;

	UNUSED(ctxt);

	/* Setting librpz_log_level in the configuration overrides the
	 * BIND9 logging levels. */
	if (level > LIBRPZ_LOG_TRACE1 &&
	    level <= librpz->log_level_val(LIBRPZ_LOG_INVALID))
	{
		level = LIBRPZ_LOG_TRACE1;
	}

	switch (level) {
	case LIBRPZ_LOG_FATAL:
	case LIBRPZ_LOG_ERROR: /* errors */
	default:
		isc_level = DNS_RPZ_ERROR_LEVEL;
		break;

	case LIBRPZ_LOG_TRACE1: /* big events such as dnsrpzd starts */
		isc_level = DNS_RPZ_INFO_LEVEL;
		break;

	case LIBRPZ_LOG_TRACE2: /* smaller dnsrpzd zone transfers */
		isc_level = DNS_RPZ_DEBUG_LEVEL1;
		break;

	case LIBRPZ_LOG_TRACE3: /* librpz hits */
		isc_level = DNS_RPZ_DEBUG_LEVEL2;
		break;

	case LIBRPZ_LOG_TRACE4: /* librpz lookups */
		isc_level = DNS_RPZ_DEBUG_LEVEL3;
		break;
	}
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ, DNS_LOGMODULE_RBTDB,
		      isc_level, "dnsrps: %s", buf);
}

/*
 * Start dnsrps for the entire server.
 *	This is not thread safe, but it is called by a single thread.
 */
isc_result_t
dns_dnsrps_server_create(void) {
	librpz_emsg_t emsg;

	INSIST(clist == NULL);
	INSIST(librpz == NULL);
	INSIST(librpz_handle == NULL);

	/*
	 * Notice if librpz is available.
	 */
	librpz = librpz_lib_open(&librpz_lib_open_emsg, &librpz_handle,
				 DNSRPS_LIBRPZ_PATH);
	/*
	 * Stop now without complaining if librpz is not available.
	 * Complain later if and when librpz is needed for a view with
	 * "dnsrps-enable yes" (including the default view).
	 */
	if (librpz == NULL) {
		return (ISC_R_SUCCESS);
	}

	isc_mutex_init(&dnsrps_mutex);

	librpz->set_log(dnsrps_log_fnc, NULL);

	clist = librpz->clist_create(&emsg, dnsrps_lock, dnsrps_unlock,
				     dnsrps_mutex_destroy, &dnsrps_mutex,
				     dns_lctx);
	if (clist == NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "dnsrps: %s", emsg.c);
		return (ISC_R_NOMEMORY);
	}
	return (ISC_R_SUCCESS);
}

/*
 * Stop dnsrps for the entire server.
 *	This is not thread safe.
 */
void
dns_dnsrps_server_destroy(void) {
	if (clist != NULL) {
		librpz->clist_detach(&clist);
	}

#ifdef LIBRPZ_USE_DLOPEN
	if (librpz != NULL) {
		INSIST(librpz_handle != NULL);
		if (dlclose(librpz_handle) != 0) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
				      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
				      "dnsrps: dlclose(): %s", dlerror());
		}
		librpz_handle = NULL;
	}
#endif /* ifdef LIBRPZ_USE_DLOPEN */
}

/*
 * Ready dnsrps for a view.
 */
isc_result_t
dns_dnsrps_view_init(dns_rpz_zones_t *new, char *rps_cstr) {
	librpz_emsg_t emsg;

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ, DNS_LOGMODULE_RBTDB,
		      DNS_RPZ_DEBUG_LEVEL3, "dnsrps configuration \"%s\"",
		      rps_cstr);

	new->rps_client = librpz->client_create(&emsg, clist, rps_cstr, false);
	if (new->rps_client == NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "librpz->client_create(): %s", emsg.c);
		new->p.dnsrps_enabled = false;
		return (ISC_R_FAILURE);
	}

	new->p.dnsrps_enabled = true;
	return (ISC_R_SUCCESS);
}

/*
 * Connect to and start the dnsrps daemon, dnsrpzd.
 */
isc_result_t
dns_dnsrps_connect(dns_rpz_zones_t *rpzs) {
	librpz_emsg_t emsg;

	if (rpzs == NULL || !rpzs->p.dnsrps_enabled) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * Fail only if we failed to link to librpz.
	 */
	if (librpz == NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "librpz->connect(): %s", librpz_lib_open_emsg.c);
		return (ISC_R_FAILURE);
	}

	if (!librpz->connect(&emsg, rpzs->rps_client, true)) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ,
			      DNS_LOGMODULE_RBTDB, DNS_RPZ_ERROR_LEVEL,
			      "librpz->connect(): %s", emsg.c);
		return (ISC_R_SUCCESS);
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RPZ, DNS_LOGMODULE_RBTDB,
		      DNS_RPZ_INFO_LEVEL, "dnsrps: librpz version %s",
		      librpz->version);

	return (ISC_R_SUCCESS);
}

/*
 * Get ready to try RPZ rewriting.
 */
isc_result_t
dns_dnsrps_rewrite_init(librpz_emsg_t *emsg, dns_rpz_st_t *st,
			dns_rpz_zones_t *rpzs, const dns_name_t *qname,
			isc_mem_t *mctx, bool have_rd) {
	rpsdb_t *rpsdb;

	rpsdb = isc_mem_get(mctx, sizeof(*rpsdb));
	memset(rpsdb, 0, sizeof(*rpsdb));

	if (!librpz->rsp_create(emsg, &rpsdb->rsp, NULL, rpzs->rps_client,
				have_rd, false))
	{
		isc_mem_put(mctx, rpsdb, sizeof(*rpsdb));
		return (DNS_R_SERVFAIL);
	}
	if (rpsdb->rsp == NULL) {
		isc_mem_put(mctx, rpsdb, sizeof(*rpsdb));
		return (DNS_R_DISALLOWED);
	}

	rpsdb->common.magic = DNS_DB_MAGIC;
	rpsdb->common.impmagic = RPSDB_MAGIC;
	rpsdb->common.methods = &rpsdb_db_methods;
	rpsdb->common.rdclass = dns_rdataclass_in;
	dns_name_init(&rpsdb->common.origin, NULL);
	isc_mem_attach(mctx, &rpsdb->common.mctx);

	rpsdb->ref_cnt = 1;
	rpsdb->qname = qname;

	st->rpsdb = &rpsdb->common;
	return (ISC_R_SUCCESS);
}

/*
 * Convert a dnsrps policy to a classic BIND9 RPZ policy.
 */
dns_rpz_policy_t
dns_dnsrps_2policy(librpz_policy_t rps_policy) {
	switch (rps_policy) {
	case LIBRPZ_POLICY_UNDEFINED:
		return (DNS_RPZ_POLICY_MISS);
	case LIBRPZ_POLICY_PASSTHRU:
		return (DNS_RPZ_POLICY_PASSTHRU);
	case LIBRPZ_POLICY_DROP:
		return (DNS_RPZ_POLICY_DROP);
	case LIBRPZ_POLICY_TCP_ONLY:
		return (DNS_RPZ_POLICY_TCP_ONLY);
	case LIBRPZ_POLICY_NXDOMAIN:
		return (DNS_RPZ_POLICY_NXDOMAIN);
	case LIBRPZ_POLICY_NODATA:
		return (DNS_RPZ_POLICY_NODATA);
	case LIBRPZ_POLICY_RECORD:
	case LIBRPZ_POLICY_CNAME:
		return (DNS_RPZ_POLICY_RECORD);

	case LIBRPZ_POLICY_DELETED:
	case LIBRPZ_POLICY_GIVEN:
	case LIBRPZ_POLICY_DISABLED:
	default:
		UNREACHABLE();
	}
}

/*
 * Convert a dnsrps trigger to a classic BIND9 RPZ rewrite or trigger type.
 */
dns_rpz_type_t
dns_dnsrps_trig2type(librpz_trig_t trig) {
	switch (trig) {
	case LIBRPZ_TRIG_BAD:
	default:
		return (DNS_RPZ_TYPE_BAD);
	case LIBRPZ_TRIG_CLIENT_IP:
		return (DNS_RPZ_TYPE_CLIENT_IP);
	case LIBRPZ_TRIG_QNAME:
		return (DNS_RPZ_TYPE_QNAME);
	case LIBRPZ_TRIG_IP:
		return (DNS_RPZ_TYPE_IP);
	case LIBRPZ_TRIG_NSDNAME:
		return (DNS_RPZ_TYPE_NSDNAME);
	case LIBRPZ_TRIG_NSIP:
		return (DNS_RPZ_TYPE_NSIP);
	}
}

/*
 * Convert a classic BIND9 RPZ rewrite or trigger type to a librpz trigger type.
 */
librpz_trig_t
dns_dnsrps_type2trig(dns_rpz_type_t type) {
	switch (type) {
	case DNS_RPZ_TYPE_BAD:
	default:
		return (LIBRPZ_TRIG_BAD);
	case DNS_RPZ_TYPE_CLIENT_IP:
		return (LIBRPZ_TRIG_CLIENT_IP);
	case DNS_RPZ_TYPE_QNAME:
		return (LIBRPZ_TRIG_QNAME);
	case DNS_RPZ_TYPE_IP:
		return (LIBRPZ_TRIG_IP);
	case DNS_RPZ_TYPE_NSDNAME:
		return (LIBRPZ_TRIG_NSDNAME);
	case DNS_RPZ_TYPE_NSIP:
		return (LIBRPZ_TRIG_NSIP);
	}
}

static void
rpsdb_attach(dns_db_t *source, dns_db_t **targetp) {
	rpsdb_t *rpsdb = (rpsdb_t *)source;

	REQUIRE(VALID_RPSDB(rpsdb));

	/*
	 * Use a simple count because only one thread uses any single rpsdb_t
	 */
	++rpsdb->ref_cnt;
	*targetp = source;
}

static void
rpsdb_detach(dns_db_t **dbp) {
	rpsdb_t *rpsdb = (rpsdb_t *)*dbp;

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(rpsdb->ref_cnt > 0);

	*dbp = NULL;

	/*
	 * Simple count because only one thread uses a rpsdb_t.
	 */
	if (--rpsdb->ref_cnt != 0) {
		return;
	}

	librpz->rsp_detach(&rpsdb->rsp);
	rpsdb->common.impmagic = 0;
	isc_mem_putanddetach(&rpsdb->common.mctx, rpsdb, sizeof(*rpsdb));
}

static void
rpsdb_attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(targetp != NULL && *targetp == NULL);
	REQUIRE(source == &rpsdb->origin_node || source == &rpsdb->data_node);

	/*
	 * Simple count because only one thread uses a rpsdb_t.
	 */
	++rpsdb->ref_cnt;
	*targetp = source;
}

static void
rpsdb_detachnode(dns_db_t *db, dns_dbnode_t **targetp) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(*targetp == &rpsdb->origin_node ||
		*targetp == &rpsdb->data_node);

	*targetp = NULL;
	rpsdb_detach(&db);
}

static isc_result_t
rpsdb_findnode(dns_db_t *db, const dns_name_t *name, bool create,
	       dns_dbnode_t **nodep) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;
	dns_db_t *dbp;

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(nodep != NULL && *nodep == NULL);
	REQUIRE(!create);

	/*
	 * A fake/shim rpsdb has two nodes.
	 * One is the origin to support query_addsoa() in bin/named/query.c.
	 * The other contains rewritten RRs.
	 */
	if (dns_name_equal(name, &db->origin)) {
		*nodep = &rpsdb->origin_node;
	} else {
		*nodep = &rpsdb->data_node;
	}
	dbp = NULL;
	rpsdb_attach(db, &dbp);

	return (ISC_R_SUCCESS);
}

static void
rpsdb_bind_rdataset(dns_rdataset_t *rdataset, uint count, librpz_idx_t next_rr,
		    dns_rdatatype_t type, uint16_t class, uint32_t ttl,
		    rpsdb_t *rpsdb) {
	dns_db_t *dbp;

	INSIST(rdataset->methods == NULL); /* We must be disassociated. */
	REQUIRE(type != dns_rdatatype_none);

	rdataset->methods = &rpsdb_rdataset_methods;
	rdataset->rdclass = class;
	rdataset->type = type;
	rdataset->ttl = ttl;
	dbp = NULL;
	dns_db_attach(&rpsdb->common, &dbp);
	RD_DB(rdataset) = dbp;
	RD_COUNT(rdataset) = count;
	RD_NEXT_RR(rdataset) = next_rr;
	RD_CUR_RR(rdataset) = NULL;
}

static isc_result_t
rpsdb_bind_soa(dns_rdataset_t *rdataset, rpsdb_t *rpsdb) {
	uint32_t ttl;
	librpz_emsg_t emsg;

	if (!librpz->rsp_soa(&emsg, &ttl, NULL, NULL, &rpsdb->result,
			     rpsdb->rsp))
	{
		librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
		return (DNS_R_SERVFAIL);
	}
	rpsdb_bind_rdataset(rdataset, 1, LIBRPZ_IDX_BAD, dns_rdatatype_soa,
			    dns_rdataclass_in, ttl, rpsdb);
	return (ISC_R_SUCCESS);
}

/*
 * Forge an rdataset of the desired type from a librpz result.
 * This is written for simplicity instead of speed, because RPZ rewriting
 * should be rare compared to normal BIND operations.
 */
static isc_result_t
rpsdb_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		   dns_rdatatype_t type, dns_rdatatype_t covers,
		   isc_stdtime_t now, dns_rdataset_t *rdataset,
		   dns_rdataset_t *sigrdataset) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;
	dns_rdatatype_t foundtype;
	dns_rdataclass_t class;
	uint32_t ttl;
	uint count;
	librpz_emsg_t emsg;

	UNUSED(version);
	UNUSED(covers);
	UNUSED(now);
	UNUSED(sigrdataset);

	REQUIRE(VALID_RPSDB(rpsdb));

	if (node == &rpsdb->origin_node) {
		if (type == dns_rdatatype_any) {
			return (ISC_R_SUCCESS);
		}
		if (type == dns_rdatatype_soa) {
			return (rpsdb_bind_soa(rdataset, rpsdb));
		}
		return (DNS_R_NXRRSET);
	}

	REQUIRE(node == &rpsdb->data_node);

	switch (rpsdb->result.policy) {
	case LIBRPZ_POLICY_UNDEFINED:
	case LIBRPZ_POLICY_DELETED:
	case LIBRPZ_POLICY_PASSTHRU:
	case LIBRPZ_POLICY_DROP:
	case LIBRPZ_POLICY_TCP_ONLY:
	case LIBRPZ_POLICY_GIVEN:
	case LIBRPZ_POLICY_DISABLED:
	default:
		librpz->log(LIBRPZ_LOG_ERROR, NULL,
			    "impossible dnsrps policy %d at %s:%d",
			    rpsdb->result.policy, __FILE__, __LINE__);
		return (DNS_R_SERVFAIL);

	case LIBRPZ_POLICY_NXDOMAIN:
		return (DNS_R_NXDOMAIN);

	case LIBRPZ_POLICY_NODATA:
		return (DNS_R_NXRRSET);

	case LIBRPZ_POLICY_RECORD:
	case LIBRPZ_POLICY_CNAME:
		break;
	}

	if (type == dns_rdatatype_soa) {
		return (rpsdb_bind_soa(rdataset, rpsdb));
	}

	/*
	 * There is little to do for an ANY query.
	 */
	if (type == dns_rdatatype_any) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * Reset to the start of the RRs.
	 * This function is only used after a policy has been chosen,
	 * and so without caring whether it is after recursion.
	 */
	if (!librpz->rsp_result(&emsg, &rpsdb->result, true, rpsdb->rsp)) {
		librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
		return (DNS_R_SERVFAIL);
	}
	if (!librpz->rsp_rr(&emsg, &foundtype, &class, &ttl, NULL,
			    &rpsdb->result, rpsdb->qname->ndata,
			    rpsdb->qname->length, rpsdb->rsp))
	{
		librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
		return (DNS_R_SERVFAIL);
	}
	REQUIRE(foundtype != dns_rdatatype_none);

	/*
	 * Ho many of the target RR type are available?
	 */
	count = 0;
	do {
		if (type == foundtype || type == dns_rdatatype_any) {
			++count;
		}

		if (!librpz->rsp_rr(&emsg, &foundtype, NULL, NULL, NULL,
				    &rpsdb->result, rpsdb->qname->ndata,
				    rpsdb->qname->length, rpsdb->rsp))
		{
			librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
			return (DNS_R_SERVFAIL);
		}
	} while (foundtype != dns_rdatatype_none);
	if (count == 0) {
		return (DNS_R_NXRRSET);
	}
	rpsdb_bind_rdataset(rdataset, count, rpsdb->result.next_rr, type, class,
			    ttl, rpsdb);
	return (ISC_R_SUCCESS);
}

static isc_result_t
rpsdb_finddb(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
	     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	     dns_dbnode_t **nodep, dns_name_t *foundname,
	     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	dns_dbnode_t *node;

	UNUSED(version);
	UNUSED(options);
	UNUSED(now);
	UNUSED(sigrdataset);

	if (nodep == NULL) {
		node = NULL;
		nodep = &node;
	}
	rpsdb_findnode(db, name, false, nodep);
	dns_name_copynf(name, foundname);
	return (rpsdb_findrdataset(db, *nodep, NULL, type, 0, 0, rdataset,
				   sigrdataset));
}

static isc_result_t
rpsdb_allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		   unsigned int options, isc_stdtime_t now,
		   dns_rdatasetiter_t **iteratorp) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;
	rpsdb_rdatasetiter_t *rpsdb_iter;

	UNUSED(version);
	UNUSED(now);

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(node == &rpsdb->origin_node || node == &rpsdb->data_node);

	rpsdb_iter = isc_mem_get(rpsdb->common.mctx, sizeof(*rpsdb_iter));

	memset(rpsdb_iter, 0, sizeof(*rpsdb_iter));
	rpsdb_iter->common.magic = DNS_RDATASETITER_MAGIC;
	rpsdb_iter->common.methods = &rpsdb_rdatasetiter_methods;
	rpsdb_iter->common.db = db;
	rpsdb_attachnode(db, node, &rpsdb_iter->common.node);

	*iteratorp = &rpsdb_iter->common;

	return (ISC_R_SUCCESS);
}

static bool
rpsdb_issecure(dns_db_t *db) {
	UNUSED(db);

	return (false);
}

static isc_result_t
rpsdb_getoriginnode(dns_db_t *db, dns_dbnode_t **nodep) {
	rpsdb_t *rpsdb = (rpsdb_t *)db;

	REQUIRE(VALID_RPSDB(rpsdb));
	REQUIRE(nodep != NULL && *nodep == NULL);

	rpsdb_attachnode(db, &rpsdb->origin_node, nodep);
	return (ISC_R_SUCCESS);
}

static void
rpsdb_rdataset_disassociate(dns_rdataset_t *rdataset) {
	dns_db_t *db;

	/*
	 * Detach the last RR delivered.
	 */
	if (RD_CUR_RR(rdataset) != NULL) {
		free(RD_CUR_RR(rdataset));
		RD_CUR_RR(rdataset) = NULL;
	}

	db = RD_DB(rdataset);
	RD_DB(rdataset) = NULL;
	dns_db_detach(&db);
}

static isc_result_t
rpsdb_rdataset_next(dns_rdataset_t *rdataset) {
	rpsdb_t *rpsdb;
	uint16_t type;
	dns_rdataclass_t class;
	librpz_rr_t *rr;
	librpz_emsg_t emsg;

	rpsdb = RD_DB(rdataset);

	/*
	 * Detach the previous RR.
	 */
	if (RD_CUR_RR(rdataset) != NULL) {
		free(RD_CUR_RR(rdataset));
		RD_CUR_RR(rdataset) = NULL;
	}

	/*
	 * Get the next RR of the specified type.
	 * SOAs differ.
	 */
	if (rdataset->type == dns_rdatatype_soa) {
		if (RD_NEXT_RR(rdataset) == LIBRPZ_IDX_NULL) {
			return (ISC_R_NOMORE);
		}
		RD_NEXT_RR(rdataset) = LIBRPZ_IDX_NULL;
		if (!librpz->rsp_soa(&emsg, NULL, &rr, NULL, &rpsdb->result,
				     rpsdb->rsp))
		{
			librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
			return (DNS_R_SERVFAIL);
		}
		RD_CUR_RR(rdataset) = rr;
		return (ISC_R_SUCCESS);
	}

	rpsdb->result.next_rr = RD_NEXT_RR(rdataset);
	for (;;) {
		if (!librpz->rsp_rr(&emsg, &type, &class, NULL, &rr,
				    &rpsdb->result, rpsdb->qname->ndata,
				    rpsdb->qname->length, rpsdb->rsp))
		{
			librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
			return (DNS_R_SERVFAIL);
		}
		if (rdataset->type == type && rdataset->rdclass == class) {
			RD_CUR_RR(rdataset) = rr;
			RD_NEXT_RR(rdataset) = rpsdb->result.next_rr;
			return (ISC_R_SUCCESS);
		}
		if (type == dns_rdatatype_none) {
			return (ISC_R_NOMORE);
		}
		free(rr);
	}
}

static isc_result_t
rpsdb_rdataset_first(dns_rdataset_t *rdataset) {
	rpsdb_t *rpsdb;
	librpz_emsg_t emsg;

	rpsdb = RD_DB(rdataset);
	REQUIRE(VALID_RPSDB(rpsdb));

	if (RD_CUR_RR(rdataset) != NULL) {
		free(RD_CUR_RR(rdataset));
		RD_CUR_RR(rdataset) = NULL;
	}

	if (!librpz->rsp_result(&emsg, &rpsdb->result, true, rpsdb->rsp)) {
		librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
		return (DNS_R_SERVFAIL);
	}
	if (rdataset->type == dns_rdatatype_soa) {
		RD_NEXT_RR(rdataset) = LIBRPZ_IDX_BAD;
	} else {
		RD_NEXT_RR(rdataset) = rpsdb->result.next_rr;
	}

	return (rpsdb_rdataset_next(rdataset));
}

static void
rpsdb_rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	rpsdb_t *rpsdb;
	librpz_rr_t *rr;
	isc_region_t r;

	rpsdb = RD_DB(rdataset);
	REQUIRE(VALID_RPSDB(rpsdb));
	rr = RD_CUR_RR(rdataset);
	REQUIRE(rr != NULL);

	r.length = ntohs(rr->rdlength);
	r.base = rr->rdata;
	dns_rdata_fromregion(rdata, ntohs(rr->class), ntohs(rr->type), &r);
}

static void
rpsdb_rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	rpsdb_t *rpsdb;
	dns_db_t *dbp;

	INSIST(!ISC_LINK_LINKED(target, link));
	*target = *source;
	ISC_LINK_INIT(target, link);
	rpsdb = RD_DB(source);
	REQUIRE(VALID_RPSDB(rpsdb));
	dbp = NULL;
	dns_db_attach(&rpsdb->common, &dbp);
	RD_DB(target) = dbp;
	RD_CUR_RR(target) = NULL;
	RD_NEXT_RR(target) = LIBRPZ_IDX_NULL;
}

static unsigned int
rpsdb_rdataset_count(dns_rdataset_t *rdataset) {
	rpsdb_t *rpsdb;

	rpsdb = RD_DB(rdataset);
	REQUIRE(VALID_RPSDB(rpsdb));

	return (RD_COUNT(rdataset));
}

static void
rpsdb_rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp) {
	rpsdb_t *rpsdb;
	dns_rdatasetiter_t *iterator;
	isc_mem_t *mctx;

	iterator = *iteratorp;
	*iteratorp = NULL;
	rpsdb = (rpsdb_t *)iterator->db;
	REQUIRE(VALID_RPSDB(rpsdb));

	mctx = iterator->db->mctx;
	dns_db_detachnode(iterator->db, &iterator->node);
	isc_mem_put(mctx, iterator, sizeof(rpsdb_rdatasetiter_t));
}

static isc_result_t
rpsdb_rdatasetiter_next(dns_rdatasetiter_t *iter) {
	rpsdb_t *rpsdb;
	rpsdb_rdatasetiter_t *rpsdb_iter;
	dns_rdatatype_t next_type, type;
	dns_rdataclass_t next_class, class;
	uint32_t ttl;
	librpz_emsg_t emsg;

	rpsdb = (rpsdb_t *)iter->db;
	REQUIRE(VALID_RPSDB(rpsdb));
	rpsdb_iter = (rpsdb_rdatasetiter_t *)iter;

	/*
	 * This function is only used after a policy has been chosen,
	 * and so without caring whether it is after recursion.
	 */
	if (!librpz->rsp_result(&emsg, &rpsdb->result, true, rpsdb->rsp)) {
		librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
		return (DNS_R_SERVFAIL);
	}
	/*
	 * Find the next class and type after the current class and type
	 * among the RRs in current result.
	 * As a side effect, count the number of those RRs.
	 */
	rpsdb_iter->count = 0;
	next_class = dns_rdataclass_reserved0;
	next_type = dns_rdatatype_none;
	for (;;) {
		if (!librpz->rsp_rr(&emsg, &type, &class, &ttl, NULL,
				    &rpsdb->result, rpsdb->qname->ndata,
				    rpsdb->qname->length, rpsdb->rsp))
		{
			librpz->log(LIBRPZ_LOG_ERROR, NULL, "%s", emsg.c);
			return (DNS_R_SERVFAIL);
		}
		if (type == dns_rdatatype_none) {
			if (next_type == dns_rdatatype_none) {
				return (ISC_R_NOMORE);
			}
			rpsdb_iter->type = next_type;
			rpsdb_iter->class = next_class;
			return (ISC_R_SUCCESS);
		}
		/*
		 * Skip RRs with the current class and type or before.
		 */
		if (rpsdb_iter->class > class ||
		    (rpsdb_iter->class = class && rpsdb_iter->type >= type))
		{
			continue;
		}
		if (next_type == dns_rdatatype_none || next_class > class ||
		    (next_class == class && next_type > type))
		{
			/*
			 * This is the first of a subsequent class and type.
			 */
			next_type = type;
			next_class = class;
			rpsdb_iter->ttl = ttl;
			rpsdb_iter->count = 1;
			rpsdb_iter->next_rr = rpsdb->result.next_rr;
		} else if (next_type == type && next_class == class) {
			++rpsdb_iter->count;
		}
	}
}

static isc_result_t
rpsdb_rdatasetiter_first(dns_rdatasetiter_t *iterator) {
	rpsdb_t *rpsdb;
	rpsdb_rdatasetiter_t *rpsdb_iter;

	rpsdb = (rpsdb_t *)iterator->db;
	REQUIRE(VALID_RPSDB(rpsdb));
	rpsdb_iter = (rpsdb_rdatasetiter_t *)iterator;

	rpsdb_iter->type = dns_rdatatype_none;
	rpsdb_iter->class = dns_rdataclass_reserved0;
	return (rpsdb_rdatasetiter_next(iterator));
}

static void
rpsdb_rdatasetiter_current(dns_rdatasetiter_t *iterator,
			   dns_rdataset_t *rdataset) {
	rpsdb_t *rpsdb;
	rpsdb_rdatasetiter_t *rpsdb_iter;

	rpsdb = (rpsdb_t *)iterator->db;
	REQUIRE(VALID_RPSDB(rpsdb));
	rpsdb_iter = (rpsdb_rdatasetiter_t *)iterator;
	REQUIRE(rpsdb_iter->type != dns_rdatatype_none);

	rpsdb_bind_rdataset(rdataset, rpsdb_iter->count, rpsdb_iter->next_rr,
			    rpsdb_iter->type, rpsdb_iter->class,
			    rpsdb_iter->ttl, rpsdb);
}

static dns_dbmethods_t rpsdb_db_methods = {
	rpsdb_attach,
	rpsdb_detach,
	NULL, /* beginload */
	NULL, /* endload */
	NULL, /* serialize */
	NULL, /* dump */
	NULL, /* currentversion */
	NULL, /* newversion */
	NULL, /* attachversion */
	NULL, /* closeversion */
	rpsdb_findnode,
	rpsdb_finddb,
	NULL, /* findzonecut*/
	rpsdb_attachnode,
	rpsdb_detachnode,
	NULL, /* expirenode */
	NULL, /* printnode */
	NULL, /* createiterator */
	rpsdb_findrdataset,
	rpsdb_allrdatasets,
	NULL, /* addrdataset */
	NULL, /* subtractrdataset */
	NULL, /* deleterdataset */
	rpsdb_issecure,
	NULL, /* nodecount */
	NULL, /* ispersistent */
	NULL, /* overmem */
	NULL, /* settask */
	rpsdb_getoriginnode,
	NULL, /* transfernode */
	NULL, /* getnsec3parameters */
	NULL, /* findnsec3node */
	NULL, /* setsigningtime */
	NULL, /* getsigningtime */
	NULL, /* resigned */
	NULL, /* isdnssec */
	NULL, /* getrrsetstats */
	NULL, /* rpz_attach */
	NULL, /* rpz_ready */
	NULL, /* findnodeext */
	NULL, /* findext */
	NULL, /* setcachestats */
	NULL, /* hashsize */
	NULL, /* nodefullname */
	NULL, /* getsize */
	NULL, /* setservestalettl */
	NULL, /* getservestalettl */
	NULL, /* setservestalerefresh */
	NULL, /* getservestalerefresh */
	NULL, /* setgluecachestats */
	NULL  /* adjusthashsize */
};

static dns_rdatasetmethods_t rpsdb_rdataset_methods = {
	rpsdb_rdataset_disassociate,
	rpsdb_rdataset_first,
	rpsdb_rdataset_next,
	rpsdb_rdataset_current,
	rpsdb_rdataset_clone,
	rpsdb_rdataset_count,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static dns_rdatasetitermethods_t rpsdb_rdatasetiter_methods = {
	rpsdb_rdatasetiter_destroy, rpsdb_rdatasetiter_first,
	rpsdb_rdatasetiter_next, rpsdb_rdatasetiter_current
};

#endif /* USE_DNSRPS */
