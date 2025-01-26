/*	$NetBSD: xfrin.c,v 1.16 2025/01/26 16:25:26 christos Exp $	*/

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

#include <isc/async.h>
#include <isc/atomic.h>
#include <isc/mem.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>
#include <isc/work.h>

#include <dns/callbacks.h>
#include <dns/catz.h>
#include <dns/db.h>
#include <dns/diff.h>
#include <dns/dispatch.h>
#include <dns/journal.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/peer.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/soa.h>
#include <dns/trace.h>
#include <dns/transport.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/xfrin.h>
#include <dns/zone.h>

#include <dst/dst.h>

#include "probes.h"

/*
 * Incoming AXFR and IXFR.
 */

#define CHECK(op)                              \
	{                                      \
		result = (op);                 \
		if (result != ISC_R_SUCCESS) { \
			goto failure;          \
		}                              \
	}

/*%
 * The states of the *XFR state machine.  We handle both IXFR and AXFR
 * with a single integrated state machine because they cannot be distinguished
 * immediately - an AXFR response to an IXFR request can only be detected
 * when the first two (2) response RRs have already been received.
 */
typedef enum {
	XFRST_SOAQUERY,
	XFRST_GOTSOA,
	XFRST_ZONEXFRREQUEST,
	XFRST_FIRSTDATA,
	XFRST_IXFR_DELSOA,
	XFRST_IXFR_DEL,
	XFRST_IXFR_ADDSOA,
	XFRST_IXFR_ADD,
	XFRST_IXFR_END,
	XFRST_AXFR,
	XFRST_AXFR_END
} xfrin_state_t;

/*%
 * Incoming zone transfer context.
 */

struct dns_xfrin {
	unsigned int magic;
	isc_mem_t *mctx;
	dns_zone_t *zone;
	dns_view_t *view;

	isc_refcount_t references;

	atomic_bool shuttingdown;

	isc_result_t shutdown_result;

	dns_name_t name; /*%< Name of zone to transfer */
	dns_rdataclass_t rdclass;

	dns_messageid_t id;

	/*%
	 * Requested transfer type (dns_rdatatype_axfr or
	 * dns_rdatatype_ixfr).  The actual transfer type
	 * may differ due to IXFR->AXFR fallback.
	 */
	dns_rdatatype_t reqtype;

	isc_sockaddr_t primaryaddr;
	isc_sockaddr_t sourceaddr;

	dns_dispatch_t *disp;
	dns_dispentry_t *dispentry;

	/*% Buffer for IXFR/AXFR request message */
	isc_buffer_t qbuffer;
	unsigned char qbuffer_data[512];

	/*%
	 * Whether the zone originally had a database attached at the time this
	 * transfer context was created.  Used by xfrin_destroy() when making
	 * logging decisions.
	 */
	bool zone_had_db;

	dns_db_t *db;
	dns_dbversion_t *ver;
	dns_diff_t diff; /*%< Pending database changes */

	/* Diff queue */
	bool diff_running;
	struct __cds_wfcq_head diff_head;
	struct cds_wfcq_tail diff_tail;

	_Atomic xfrin_state_t state;
	uint32_t expireopt;
	bool edns, expireoptset;
	atomic_bool is_ixfr;

	/*
	 * Following variable were made atomic only for loading the values for
	 * the statistics channel, thus all accesses can be **relaxed** because
	 * all store and load operations that affect XFR are done on the same
	 * thread and only the statistics channel thread could perform a load
	 * operation from a different thread and it's ok to not be precise in
	 * the statistics.
	 */
	atomic_uint nmsg;	     /*%< Number of messages recvd */
	atomic_uint nrecs;	     /*%< Number of records recvd */
	atomic_uint_fast64_t nbytes; /*%< Number of bytes received */
	_Atomic(isc_time_t) start;   /*%< Start time of the transfer */
	_Atomic(dns_transport_type_t) soa_transport_type;
	atomic_uint_fast32_t end_serial;

	unsigned int maxrecords; /*%< The maximum number of
				  *   records set for the zone */

	dns_tsigkey_t *tsigkey; /*%< Key used to create TSIG */
	isc_buffer_t *lasttsig; /*%< The last TSIG */
	dst_context_t *tsigctx; /*%< TSIG verification context */
	unsigned int sincetsig; /*%< recvd since the last TSIG */

	dns_transport_t *transport;

	dns_xfrindone_t done;

	/*%
	 * AXFR- and IXFR-specific data.  Only one is used at a time
	 * according to the is_ixfr flag, so this could be a union,
	 * but keeping them separate makes it a bit simpler to clean
	 * things up when destroying the context.
	 */
	dns_rdatacallbacks_t axfr;

	struct {
		uint32_t request_serial;
		uint32_t current_serial;
		dns_journal_t *journal;
	} ixfr;

	dns_rdata_t firstsoa;
	unsigned char *firstsoa_data;

	isc_tlsctx_cache_t *tlsctx_cache;

	isc_loop_t *loop;

	isc_timer_t *max_time_timer;
	isc_timer_t *max_idle_timer;

	char info[DNS_NAME_MAXTEXT + 32];
};

#define XFRIN_MAGIC    ISC_MAGIC('X', 'f', 'r', 'I')
#define VALID_XFRIN(x) ISC_MAGIC_VALID(x, XFRIN_MAGIC)

#define XFRIN_WORK_MAGIC    ISC_MAGIC('X', 'f', 'r', 'W')
#define VALID_XFRIN_WORK(x) ISC_MAGIC_VALID(x, XFRIN_WORK_MAGIC)

typedef struct xfrin_work {
	unsigned int magic;
	isc_result_t result;
	dns_xfrin_t *xfr;
} xfrin_work_t;

/**************************************************************************/
/*
 * Forward declarations.
 */

static void
xfrin_create(isc_mem_t *mctx, dns_zone_t *zone, dns_db_t *db, isc_loop_t *loop,
	     dns_name_t *zonename, dns_rdataclass_t rdclass,
	     dns_rdatatype_t reqtype, const isc_sockaddr_t *primaryaddr,
	     const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
	     dns_transport_type_t soa_transport_type,
	     dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
	     dns_xfrin_t **xfrp);

static isc_result_t
axfr_init(dns_xfrin_t *xfr);
static isc_result_t
axfr_putdata(dns_xfrin_t *xfr, dns_diffop_t op, dns_name_t *name, dns_ttl_t ttl,
	     dns_rdata_t *rdata);
static void
axfr_commit(dns_xfrin_t *xfr);
static isc_result_t
axfr_finalize(dns_xfrin_t *xfr);

static isc_result_t
ixfr_init(dns_xfrin_t *xfr);
static isc_result_t
ixfr_putdata(dns_xfrin_t *xfr, dns_diffop_t op, dns_name_t *name, dns_ttl_t ttl,
	     dns_rdata_t *rdata);
static isc_result_t
ixfr_commit(dns_xfrin_t *xfr);

static isc_result_t
xfr_rr(dns_xfrin_t *xfr, dns_name_t *name, uint32_t ttl, dns_rdata_t *rdata);

static isc_result_t
xfrin_start(dns_xfrin_t *xfr);

static void
xfrin_connect_done(isc_result_t result, isc_region_t *region, void *arg);
static isc_result_t
xfrin_send_request(dns_xfrin_t *xfr);
static void
xfrin_send_done(isc_result_t eresult, isc_region_t *region, void *arg);
static void
xfrin_recv_done(isc_result_t result, isc_region_t *region, void *arg);

static void
xfrin_end(dns_xfrin_t *xfr, isc_result_t result);

static void
xfrin_destroy(dns_xfrin_t *xfr);

static void
xfrin_timedout(void *);
static void
xfrin_idledout(void *);
static void
xfrin_fail(dns_xfrin_t *xfr, isc_result_t result, const char *msg);
static isc_result_t
render(dns_message_t *msg, isc_mem_t *mctx, isc_buffer_t *buf);

static void
xfrin_log(dns_xfrin_t *xfr, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

/**************************************************************************/
/*
 * AXFR handling
 */

static isc_result_t
axfr_init(dns_xfrin_t *xfr) {
	isc_result_t result;

	atomic_store(&xfr->is_ixfr, false);

	if (xfr->db != NULL) {
		dns_db_detach(&xfr->db);
	}

	CHECK(dns_zone_makedb(xfr->zone, &xfr->db));

	dns_zone_rpz_enable_db(xfr->zone, xfr->db);
	dns_zone_catz_enable_db(xfr->zone, xfr->db);

	dns_rdatacallbacks_init(&xfr->axfr);
	CHECK(dns_db_beginload(xfr->db, &xfr->axfr));
	result = ISC_R_SUCCESS;
failure:
	return result;
}

static void
axfr_apply(void *arg);

static isc_result_t
axfr_putdata(dns_xfrin_t *xfr, dns_diffop_t op, dns_name_t *name, dns_ttl_t ttl,
	     dns_rdata_t *rdata) {
	isc_result_t result;

	dns_difftuple_t *tuple = NULL;

	if (rdata->rdclass != xfr->rdclass) {
		return DNS_R_BADCLASS;
	}

	CHECK(dns_zone_checknames(xfr->zone, name, rdata));

	if (dns_diff_size(&xfr->diff) > 128 &&
	    dns_diff_is_boundary(&xfr->diff, name))
	{
		xfrin_work_t work = (xfrin_work_t){
			.magic = XFRIN_WORK_MAGIC,
			.result = ISC_R_UNSET,
			.xfr = xfr,
		};
		axfr_apply((void *)&work);
		CHECK(work.result);
	}

	CHECK(dns_difftuple_create(xfr->diff.mctx, op, name, ttl, rdata,
				   &tuple));
	dns_diff_append(&xfr->diff, &tuple);

	result = ISC_R_SUCCESS;
failure:
	return result;
}

/*
 * Store a set of AXFR RRs in the database.
 */
static void
axfr_apply(void *arg) {
	xfrin_work_t *work = arg;
	REQUIRE(VALID_XFRIN_WORK(work));

	dns_xfrin_t *xfr = work->xfr;
	REQUIRE(VALID_XFRIN(xfr));

	isc_result_t result = ISC_R_SUCCESS;
	uint64_t records;

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
		goto failure;
	}

	CHECK(dns_diff_load(&xfr->diff, &xfr->axfr));
	if (xfr->maxrecords != 0U) {
		result = dns_db_getsize(xfr->db, xfr->ver, &records, NULL);
		if (result == ISC_R_SUCCESS && records > xfr->maxrecords) {
			result = DNS_R_TOOMANYRECORDS;
			goto failure;
		}
	}

failure:
	dns_diff_clear(&xfr->diff);
	work->result = result;
}

static void
axfr_apply_done(void *arg) {
	xfrin_work_t *work = arg;
	REQUIRE(VALID_XFRIN_WORK(work));

	dns_xfrin_t *xfr = work->xfr;
	isc_result_t result = work->result;

	REQUIRE(VALID_XFRIN(xfr));

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	if (result == ISC_R_SUCCESS) {
		CHECK(dns_db_endload(xfr->db, &xfr->axfr));
		CHECK(dns_zone_verifydb(xfr->zone, xfr->db, NULL));
		CHECK(axfr_finalize(xfr));
	} else {
		(void)dns_db_endload(xfr->db, &xfr->axfr);
	}

failure:
	xfr->diff_running = false;

	isc_mem_put(xfr->mctx, work, sizeof(*work));

	if (result == ISC_R_SUCCESS) {
		if (atomic_load(&xfr->state) == XFRST_AXFR_END) {
			xfrin_end(xfr, result);
		}
	} else {
		xfrin_fail(xfr, result, "failed while processing responses");
	}

	dns_xfrin_detach(&xfr);
}

static void
axfr_commit(dns_xfrin_t *xfr) {
	REQUIRE(!xfr->diff_running);

	xfrin_work_t *work = isc_mem_get(xfr->mctx, sizeof(*work));
	*work = (xfrin_work_t){
		.magic = XFRIN_WORK_MAGIC,
		.result = ISC_R_UNSET,
		.xfr = dns_xfrin_ref(xfr),
	};
	xfr->diff_running = true;
	isc_work_enqueue(xfr->loop, axfr_apply, axfr_apply_done, work);
}

static isc_result_t
axfr_finalize(dns_xfrin_t *xfr) {
	isc_result_t result;

	LIBDNS_XFRIN_AXFR_FINALIZE_BEGIN(xfr, xfr->info);
	result = dns_zone_replacedb(xfr->zone, xfr->db, true);
	LIBDNS_XFRIN_AXFR_FINALIZE_END(xfr, xfr->info, result);

	return result;
}

/**************************************************************************/
/*
 * IXFR handling
 */

typedef struct ixfr_apply_data {
	dns_diff_t diff; /*%< Pending database changes */
	struct cds_wfcq_node wfcq_node;
} ixfr_apply_data_t;

static isc_result_t
ixfr_init(dns_xfrin_t *xfr) {
	isc_result_t result;
	char *journalfile = NULL;

	if (xfr->reqtype != dns_rdatatype_ixfr) {
		xfrin_log(xfr, ISC_LOG_NOTICE,
			  "got incremental response to AXFR request");
		return DNS_R_FORMERR;
	}

	atomic_store(&xfr->is_ixfr, true);
	INSIST(xfr->db != NULL);

	journalfile = dns_zone_getjournal(xfr->zone);
	if (journalfile != NULL) {
		CHECK(dns_journal_open(xfr->mctx, journalfile,
				       DNS_JOURNAL_CREATE, &xfr->ixfr.journal));
	}

	result = ISC_R_SUCCESS;
failure:
	return result;
}

static isc_result_t
ixfr_putdata(dns_xfrin_t *xfr, dns_diffop_t op, dns_name_t *name, dns_ttl_t ttl,
	     dns_rdata_t *rdata) {
	isc_result_t result;
	dns_difftuple_t *tuple = NULL;

	if (rdata->rdclass != xfr->rdclass) {
		return DNS_R_BADCLASS;
	}

	if (op == DNS_DIFFOP_ADD) {
		CHECK(dns_zone_checknames(xfr->zone, name, rdata));
	}
	CHECK(dns_difftuple_create(xfr->diff.mctx, op, name, ttl, rdata,
				   &tuple));
	dns_diff_append(&xfr->diff, &tuple);
	result = ISC_R_SUCCESS;
failure:
	return result;
}

static isc_result_t
ixfr_begin_transaction(dns_xfrin_t *xfr) {
	isc_result_t result = ISC_R_SUCCESS;

	if (xfr->ixfr.journal != NULL) {
		CHECK(dns_journal_begin_transaction(xfr->ixfr.journal));
	}
failure:
	return result;
}

static isc_result_t
ixfr_end_transaction(dns_xfrin_t *xfr) {
	isc_result_t result = ISC_R_SUCCESS;

	CHECK(dns_zone_verifydb(xfr->zone, xfr->db, xfr->ver));
	/* XXX enter ready-to-commit state here */
	if (xfr->ixfr.journal != NULL) {
		CHECK(dns_journal_commit(xfr->ixfr.journal));
	}
failure:
	return result;
}

static isc_result_t
ixfr_apply_one(dns_xfrin_t *xfr, ixfr_apply_data_t *data) {
	isc_result_t result = ISC_R_SUCCESS;
	uint64_t records;

	CHECK(ixfr_begin_transaction(xfr));

	CHECK(dns_diff_apply(&data->diff, xfr->db, xfr->ver));
	if (xfr->maxrecords != 0U) {
		result = dns_db_getsize(xfr->db, xfr->ver, &records, NULL);
		if (result == ISC_R_SUCCESS && records > xfr->maxrecords) {
			result = DNS_R_TOOMANYRECORDS;
			goto failure;
		}
	}
	if (xfr->ixfr.journal != NULL) {
		CHECK(dns_journal_writediff(xfr->ixfr.journal, &data->diff));
	}

	result = ixfr_end_transaction(xfr);

	return result;
failure:
	/* We need to end the transaction, but keep the previous error */
	(void)ixfr_end_transaction(xfr);

	return result;
}

static void
ixfr_apply(void *arg) {
	xfrin_work_t *work = arg;
	dns_xfrin_t *xfr = work->xfr;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_XFRIN(xfr));
	REQUIRE(VALID_XFRIN_WORK(work));

	struct __cds_wfcq_head diff_head;
	struct cds_wfcq_tail diff_tail;

	/* Initialize local wfcqueue */
	__cds_wfcq_init(&diff_head, &diff_tail);

	enum cds_wfcq_ret ret = __cds_wfcq_splice_blocking(
		&diff_head, &diff_tail, &xfr->diff_head, &xfr->diff_tail);
	INSIST(ret == CDS_WFCQ_RET_DEST_EMPTY);

	struct cds_wfcq_node *node, *next;
	__cds_wfcq_for_each_blocking_safe(&diff_head, &diff_tail, node, next) {
		ixfr_apply_data_t *data =
			caa_container_of(node, ixfr_apply_data_t, wfcq_node);

		if (atomic_load(&xfr->shuttingdown)) {
			result = ISC_R_SHUTTINGDOWN;
		}

		/* Apply only until first failure */
		if (result == ISC_R_SUCCESS) {
			/* This also checks for shuttingdown condition */
			result = ixfr_apply_one(xfr, data);
		}

		/* We need to clear and free all data chunks */
		dns_diff_clear(&data->diff);
		isc_mem_put(xfr->mctx, data, sizeof(*data));
	}

	work->result = result;
}

static void
ixfr_apply_done(void *arg) {
	xfrin_work_t *work = arg;
	REQUIRE(VALID_XFRIN_WORK(work));

	dns_xfrin_t *xfr = work->xfr;
	REQUIRE(VALID_XFRIN(xfr));

	isc_result_t result = work->result;

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	/* Reschedule */
	if (!cds_wfcq_empty(&xfr->diff_head, &xfr->diff_tail)) {
		isc_work_enqueue(xfr->loop, ixfr_apply, ixfr_apply_done, work);
		return;
	}

failure:
	xfr->diff_running = false;

	isc_mem_put(xfr->mctx, work, sizeof(*work));

	if (result == ISC_R_SUCCESS) {
		dns_db_closeversion(xfr->db, &xfr->ver, true);
		dns_zone_markdirty(xfr->zone);

		if (atomic_load(&xfr->state) == XFRST_IXFR_END) {
			xfrin_end(xfr, result);
		}
	} else {
		dns_db_closeversion(xfr->db, &xfr->ver, false);

		xfrin_fail(xfr, result, "failed while processing responses");
	}

	dns_xfrin_detach(&xfr);
}

/*
 * Apply a set of IXFR changes to the database.
 */
static isc_result_t
ixfr_commit(dns_xfrin_t *xfr) {
	isc_result_t result = ISC_R_SUCCESS;
	ixfr_apply_data_t *data = isc_mem_get(xfr->mctx, sizeof(*data));

	*data = (ixfr_apply_data_t){ 0 };
	cds_wfcq_node_init(&data->wfcq_node);

	if (xfr->ver == NULL) {
		CHECK(dns_db_newversion(xfr->db, &xfr->ver));
	}

	dns_diff_init(xfr->mctx, &data->diff);
	/* FIXME: Should we add dns_diff_move() */
	ISC_LIST_MOVE(data->diff.tuples, xfr->diff.tuples);

	(void)cds_wfcq_enqueue(&xfr->diff_head, &xfr->diff_tail,
			       &data->wfcq_node);

	if (!xfr->diff_running) {
		xfrin_work_t *work = isc_mem_get(xfr->mctx, sizeof(*work));
		*work = (xfrin_work_t){
			.magic = XFRIN_WORK_MAGIC,
			.result = ISC_R_UNSET,
			.xfr = dns_xfrin_ref(xfr),
		};
		xfr->diff_running = true;
		isc_work_enqueue(xfr->loop, ixfr_apply, ixfr_apply_done, work);
	}

failure:
	return result;
}

/**************************************************************************/
/*
 * Common AXFR/IXFR protocol code
 */

/*
 * Handle a single incoming resource record according to the current
 * state.
 */
static isc_result_t
xfr_rr(dns_xfrin_t *xfr, dns_name_t *name, uint32_t ttl, dns_rdata_t *rdata) {
	isc_result_t result;
	uint_fast32_t end_serial;

	atomic_fetch_add_relaxed(&xfr->nrecs, 1);

	if (rdata->type == dns_rdatatype_none ||
	    dns_rdatatype_ismeta(rdata->type))
	{
		char buf[64];
		dns_rdatatype_format(rdata->type, buf, sizeof(buf));
		xfrin_log(xfr, ISC_LOG_NOTICE,
			  "Unexpected %s record in zone transfer", buf);
		result = DNS_R_FORMERR;
		goto failure;
	}

	/*
	 * Immediately reject the entire transfer if the RR that is currently
	 * being processed is an SOA record that is not placed at the zone
	 * apex.
	 */
	if (rdata->type == dns_rdatatype_soa &&
	    !dns_name_equal(&xfr->name, name))
	{
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(name, namebuf, sizeof(namebuf));
		xfrin_log(xfr, ISC_LOG_DEBUG(3), "SOA name mismatch: '%s'",
			  namebuf);
		result = DNS_R_NOTZONETOP;
		goto failure;
	}

redo:
	switch (atomic_load(&xfr->state)) {
	case XFRST_SOAQUERY:
		if (rdata->type != dns_rdatatype_soa) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "non-SOA response to SOA query");
			result = DNS_R_FORMERR;
			goto failure;
		}
		end_serial = dns_soa_getserial(rdata);
		atomic_store_relaxed(&xfr->end_serial, end_serial);
		if (!DNS_SERIAL_GT(end_serial, xfr->ixfr.request_serial) &&
		    !dns_zone_isforced(xfr->zone))
		{
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "requested serial %u, "
				  "primary has %" PRIuFAST32 ", not updating",
				  xfr->ixfr.request_serial, end_serial);
			result = DNS_R_UPTODATE;
			goto failure;
		}
		atomic_store(&xfr->state, XFRST_GOTSOA);
		break;

	case XFRST_GOTSOA:
		/*
		 * Skip other records in the answer section.
		 */
		break;

	case XFRST_ZONEXFRREQUEST:
		if (rdata->type != dns_rdatatype_soa) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "first RR in zone transfer must be SOA");
			result = DNS_R_FORMERR;
			goto failure;
		}
		/*
		 * Remember the serial number in the initial SOA.
		 * We need it to recognize the end of an IXFR.
		 */
		end_serial = dns_soa_getserial(rdata);
		atomic_store_relaxed(&xfr->end_serial, end_serial);
		if (xfr->reqtype == dns_rdatatype_ixfr &&
		    !DNS_SERIAL_GT(end_serial, xfr->ixfr.request_serial) &&
		    !dns_zone_isforced(xfr->zone))
		{
			/*
			 * This must be the single SOA record that is
			 * sent when the current version on the primary
			 * is not newer than the version in the request.
			 */
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "requested serial %u, "
				  "primary has %" PRIuFAST32 ", not updating",
				  xfr->ixfr.request_serial, end_serial);
			result = DNS_R_UPTODATE;
			goto failure;
		}
		xfr->firstsoa = *rdata;
		if (xfr->firstsoa_data != NULL) {
			isc_mem_free(xfr->mctx, xfr->firstsoa_data);
		}
		xfr->firstsoa_data = isc_mem_allocate(xfr->mctx, rdata->length);
		memcpy(xfr->firstsoa_data, rdata->data, rdata->length);
		xfr->firstsoa.data = xfr->firstsoa_data;
		atomic_store(&xfr->state, XFRST_FIRSTDATA);
		break;

	case XFRST_FIRSTDATA:
		/*
		 * If the transfer begins with one SOA record, it is an AXFR,
		 * if it begins with two SOAs, it is an IXFR.
		 */
		if (xfr->reqtype == dns_rdatatype_ixfr &&
		    rdata->type == dns_rdatatype_soa &&
		    xfr->ixfr.request_serial == dns_soa_getserial(rdata))
		{
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "got incremental response");
			CHECK(ixfr_init(xfr));
			atomic_store(&xfr->state, XFRST_IXFR_DELSOA);
		} else {
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "got nonincremental response");
			CHECK(axfr_init(xfr));
			atomic_store(&xfr->state, XFRST_AXFR);
		}
		goto redo;

	case XFRST_IXFR_DELSOA:
		INSIST(rdata->type == dns_rdatatype_soa);
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_DEL, name, ttl, rdata));
		atomic_store(&xfr->state, XFRST_IXFR_DEL);
		break;

	case XFRST_IXFR_DEL:
		if (rdata->type == dns_rdatatype_soa) {
			uint32_t soa_serial = dns_soa_getserial(rdata);
			atomic_store(&xfr->state, XFRST_IXFR_ADDSOA);
			xfr->ixfr.current_serial = soa_serial;
			goto redo;
		}
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_DEL, name, ttl, rdata));
		break;

	case XFRST_IXFR_ADDSOA:
		INSIST(rdata->type == dns_rdatatype_soa);
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_ADD, name, ttl, rdata));
		atomic_store(&xfr->state, XFRST_IXFR_ADD);
		break;

	case XFRST_IXFR_ADD:
		if (rdata->type == dns_rdatatype_soa) {
			uint32_t soa_serial = dns_soa_getserial(rdata);
			if (soa_serial == atomic_load_relaxed(&xfr->end_serial))
			{
				CHECK(ixfr_commit(xfr));
				atomic_store(&xfr->state, XFRST_IXFR_END);
				break;
			} else if (soa_serial != xfr->ixfr.current_serial) {
				xfrin_log(xfr, ISC_LOG_NOTICE,
					  "IXFR out of sync: "
					  "expected serial %u, got %u",
					  xfr->ixfr.current_serial, soa_serial);
				result = DNS_R_FORMERR;
				goto failure;
			} else {
				CHECK(ixfr_commit(xfr));
				atomic_store(&xfr->state, XFRST_IXFR_DELSOA);
				goto redo;
			}
		}
		if (rdata->type == dns_rdatatype_ns &&
		    dns_name_iswildcard(name))
		{
			result = DNS_R_INVALIDNS;
			goto failure;
		}
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_ADD, name, ttl, rdata));
		break;

	case XFRST_AXFR:
		/*
		 * Old BINDs sent cross class A records for non IN classes.
		 */
		if (rdata->type == dns_rdatatype_a &&
		    rdata->rdclass != xfr->rdclass &&
		    xfr->rdclass != dns_rdataclass_in)
		{
			break;
		}
		CHECK(axfr_putdata(xfr, DNS_DIFFOP_ADD, name, ttl, rdata));
		if (rdata->type == dns_rdatatype_soa) {
			/*
			 * Use dns_rdata_compare instead of memcmp to
			 * allow for case differences.
			 */
			if (dns_rdata_compare(rdata, &xfr->firstsoa) != 0) {
				xfrin_log(xfr, ISC_LOG_NOTICE,
					  "start and ending SOA records "
					  "mismatch");
				result = DNS_R_FORMERR;
				goto failure;
			}
			axfr_commit(xfr);
			atomic_store(&xfr->state, XFRST_AXFR_END);
			break;
		}
		break;
	case XFRST_AXFR_END:
	case XFRST_IXFR_END:
		result = DNS_R_EXTRADATA;
		goto failure;
	default:
		UNREACHABLE();
	}
	result = ISC_R_SUCCESS;
failure:
	return result;
}

void
dns_xfrin_create(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		 const isc_sockaddr_t *primaryaddr,
		 const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
		 dns_transport_type_t soa_transport_type,
		 dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
		 isc_mem_t *mctx, dns_xfrin_t **xfrp) {
	dns_name_t *zonename = dns_zone_getorigin(zone);
	dns_xfrin_t *xfr = NULL;
	dns_db_t *db = NULL;
	isc_loop_t *loop = NULL;

	REQUIRE(xfrp != NULL && *xfrp == NULL);
	REQUIRE(isc_sockaddr_getport(primaryaddr) != 0);
	REQUIRE(zone != NULL);
	REQUIRE(dns_zone_getview(zone) != NULL);

	loop = dns_zone_getloop(zone);

	(void)dns_zone_getdb(zone, &db);

	if (xfrtype == dns_rdatatype_soa || xfrtype == dns_rdatatype_ixfr) {
		REQUIRE(db != NULL);
	}

	xfrin_create(mctx, zone, db, loop, zonename, dns_zone_getclass(zone),
		     xfrtype, primaryaddr, sourceaddr, tsigkey,
		     soa_transport_type, transport, tlsctx_cache, &xfr);

	if (db != NULL) {
		xfr->zone_had_db = true;
		dns_db_detach(&db);
	}

	*xfrp = xfr;
}

isc_result_t
dns_xfrin_start(dns_xfrin_t *xfr, dns_xfrindone_t done) {
	isc_result_t result;

	REQUIRE(xfr != NULL);
	REQUIRE(xfr->zone != NULL);
	REQUIRE(done != NULL);

	xfr->done = done;

	result = xfrin_start(xfr);
	if (result != ISC_R_SUCCESS) {
		xfr->done = NULL;
		xfrin_fail(xfr, result, "zone transfer start failed");
	}

	return result;
}

static void
xfrin_timedout(void *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_TIMEDOUT, "maximum transfer time exceeded");
}

static void
xfrin_idledout(void *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_TIMEDOUT, "maximum idle time exceeded");
}

isc_time_t
dns_xfrin_getstarttime(dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	return atomic_load_relaxed(&xfr->start);
}

void
dns_xfrin_getstate(const dns_xfrin_t *xfr, const char **statestr,
		   bool *is_first_data_received, bool *is_ixfr) {
	xfrin_state_t state;

	REQUIRE(VALID_XFRIN(xfr));
	REQUIRE(statestr != NULL && *statestr == NULL);
	REQUIRE(is_ixfr != NULL);

	state = atomic_load(&xfr->state);
	*statestr = "";
	*is_first_data_received = (state > XFRST_FIRSTDATA);
	*is_ixfr = atomic_load(&xfr->is_ixfr);

	switch (state) {
	case XFRST_SOAQUERY:
		*statestr = "SOA Query";
		break;
	case XFRST_GOTSOA:
		*statestr = "Got SOA";
		break;
	case XFRST_ZONEXFRREQUEST:
		*statestr = "Zone Transfer Request";
		break;
	case XFRST_FIRSTDATA:
		*statestr = "First Data";
		break;
	case XFRST_IXFR_DELSOA:
	case XFRST_IXFR_DEL:
	case XFRST_IXFR_ADDSOA:
	case XFRST_IXFR_ADD:
		*statestr = "Receiving IXFR Data";
		break;
	case XFRST_IXFR_END:
		*statestr = "Finalizing IXFR";
		break;
	case XFRST_AXFR:
		*statestr = "Receiving AXFR Data";
		break;
	case XFRST_AXFR_END:
		*statestr = "Finalizing AXFR";
		break;
	}
}

uint32_t
dns_xfrin_getendserial(dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	return atomic_load_relaxed(&xfr->end_serial);
}

void
dns_xfrin_getstats(dns_xfrin_t *xfr, unsigned int *nmsgp, unsigned int *nrecsp,
		   uint64_t *nbytesp) {
	REQUIRE(VALID_XFRIN(xfr));
	REQUIRE(nmsgp != NULL && nrecsp != NULL && nbytesp != NULL);

	SET_IF_NOT_NULL(nmsgp, atomic_load_relaxed(&xfr->nmsg));
	SET_IF_NOT_NULL(nrecsp, atomic_load_relaxed(&xfr->nrecs));
	SET_IF_NOT_NULL(nbytesp, atomic_load_relaxed(&xfr->nbytes));
}

const isc_sockaddr_t *
dns_xfrin_getsourceaddr(const dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	return &xfr->sourceaddr;
}

const isc_sockaddr_t *
dns_xfrin_getprimaryaddr(const dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	return &xfr->primaryaddr;
}

dns_transport_type_t
dns_xfrin_gettransporttype(const dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	if (xfr->transport != NULL) {
		return dns_transport_get_type(xfr->transport);
	}

	return DNS_TRANSPORT_TCP;
}

dns_transport_type_t
dns_xfrin_getsoatransporttype(dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	return atomic_load_relaxed(&xfr->soa_transport_type);
}

const dns_name_t *
dns_xfrin_gettsigkeyname(const dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	if (xfr->tsigkey == NULL || xfr->tsigkey->key == NULL) {
		return NULL;
	}

	return dst_key_name(xfr->tsigkey->key);
}

static void
xfrin_shutdown(void *arg) {
	dns_xfrin_t *xfr = arg;

	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_SHUTTINGDOWN, "shut down");
	dns_xfrin_detach(&xfr);
}

void
dns_xfrin_shutdown(dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	if (xfr->loop != isc_loop()) {
		dns_xfrin_ref(xfr);
		isc_async_run(xfr->loop, xfrin_shutdown, xfr);
	} else {
		xfrin_fail(xfr, ISC_R_SHUTTINGDOWN, "shut down");
	}
}

#if DNS_XFRIN_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_xfrin, xfrin_destroy);
#else
ISC_REFCOUNT_IMPL(dns_xfrin, xfrin_destroy);
#endif

static void
xfrin_cancelio(dns_xfrin_t *xfr) {
	if (xfr->dispentry != NULL) {
		dns_dispatch_done(&xfr->dispentry);
	}
	if (xfr->disp != NULL) {
		dns_dispatch_detach(&xfr->disp);
	}
}

static void
xfrin_reset(dns_xfrin_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_log(xfr, ISC_LOG_INFO, "resetting");

	if (xfr->lasttsig != NULL) {
		isc_buffer_free(&xfr->lasttsig);
	}

	dns_diff_clear(&xfr->diff);

	if (xfr->ixfr.journal != NULL) {
		dns_journal_destroy(&xfr->ixfr.journal);
	}

	if (xfr->axfr.add_private != NULL) {
		(void)dns_db_endload(xfr->db, &xfr->axfr);
	}

	if (xfr->ver != NULL) {
		dns_db_closeversion(xfr->db, &xfr->ver, false);
	}
}

static void
xfrin_fail(dns_xfrin_t *xfr, isc_result_t result, const char *msg) {
	REQUIRE(VALID_XFRIN(xfr));

	dns_xfrin_ref(xfr);

	/* Make sure only the first xfrin_fail() trumps */
	if (atomic_compare_exchange_strong(&xfr->shuttingdown, &(bool){ false },
					   true))
	{
		if (result != DNS_R_UPTODATE) {
			xfrin_log(xfr, ISC_LOG_ERROR, "%s: %s", msg,
				  isc_result_totext(result));
			if (atomic_load(&xfr->is_ixfr) &&
			    result != ISC_R_CANCELED &&
			    result != ISC_R_SHUTTINGDOWN)
			{
				/*
				 * Pass special result code to force AXFR retry
				 */
				result = DNS_R_BADIXFR;
			}
		}

		xfrin_cancelio(xfr);

		xfrin_end(xfr, result);
	}

	dns_xfrin_detach(&xfr);
}

static void
xfrin_create(isc_mem_t *mctx, dns_zone_t *zone, dns_db_t *db, isc_loop_t *loop,
	     dns_name_t *zonename, dns_rdataclass_t rdclass,
	     dns_rdatatype_t reqtype, const isc_sockaddr_t *primaryaddr,
	     const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
	     dns_transport_type_t soa_transport_type,
	     dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
	     dns_xfrin_t **xfrp) {
	dns_xfrin_t *xfr = NULL;

	xfr = isc_mem_get(mctx, sizeof(*xfr));
	*xfr = (dns_xfrin_t){
		.shutdown_result = ISC_R_UNSET,
		.rdclass = rdclass,
		.reqtype = reqtype,
		.maxrecords = dns_zone_getmaxrecords(zone),
		.primaryaddr = *primaryaddr,
		.sourceaddr = *sourceaddr,
		.soa_transport_type = soa_transport_type,
		.firstsoa = DNS_RDATA_INIT,
		.edns = true,
		.references = 1,
		.magic = XFRIN_MAGIC,
	};

	isc_loop_attach(loop, &xfr->loop);
	isc_mem_attach(mctx, &xfr->mctx);
	dns_zone_iattach(zone, &xfr->zone);
	dns_view_weakattach(dns_zone_getview(zone), &xfr->view);
	dns_name_init(&xfr->name, NULL);

	__cds_wfcq_init(&xfr->diff_head, &xfr->diff_tail);

	atomic_init(&xfr->is_ixfr, false);

	if (db != NULL) {
		dns_db_attach(db, &xfr->db);
	}

	dns_diff_init(xfr->mctx, &xfr->diff);

	if (reqtype == dns_rdatatype_soa) {
		atomic_init(&xfr->state, XFRST_SOAQUERY);
	} else {
		atomic_init(&xfr->state, XFRST_ZONEXFRREQUEST);
	}

	atomic_init(&xfr->start, isc_time_now());

	if (tsigkey != NULL) {
		dns_tsigkey_attach(tsigkey, &xfr->tsigkey);
	}

	if (transport != NULL) {
		dns_transport_attach(transport, &xfr->transport);
	}

	dns_name_dup(zonename, mctx, &xfr->name);

	INSIST(isc_sockaddr_pf(primaryaddr) == isc_sockaddr_pf(sourceaddr));
	isc_sockaddr_setport(&xfr->sourceaddr, 0);

	/*
	 * Reserve 2 bytes for TCP length at the beginning of the buffer.
	 */
	isc_buffer_init(&xfr->qbuffer, &xfr->qbuffer_data[2],
			sizeof(xfr->qbuffer_data) - 2);

	isc_tlsctx_cache_attach(tlsctx_cache, &xfr->tlsctx_cache);

	dns_zone_name(xfr->zone, xfr->info, sizeof(xfr->info));

	*xfrp = xfr;
}

static isc_result_t
xfrin_start(dns_xfrin_t *xfr) {
	isc_result_t result = ISC_R_FAILURE;
	isc_interval_t interval;

	dns_xfrin_ref(xfr);

	/* If this is a retry, we need to cancel the previous dispentry */
	xfrin_cancelio(xfr);

	dns_dispatchmgr_t *dispmgr = dns_view_getdispatchmgr(xfr->view);
	if (dispmgr == NULL) {
		result = ISC_R_SHUTTINGDOWN;
		goto failure;
	} else {
		result = dns_dispatch_createtcp(
			dispmgr, &xfr->sourceaddr, &xfr->primaryaddr,
			xfr->transport, DNS_DISPATCHOPT_UNSHARED, &xfr->disp);
		dns_dispatchmgr_detach(&dispmgr);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
	}

	LIBDNS_XFRIN_START(xfr, xfr->info);

	/*
	 * If the transfer is started when the 'state' is XFRST_SOAQUERY, it
	 * means the SOA query will be performed by xfrin. A transfer could also
	 * be initiated starting from the XFRST_ZONEXFRREQUEST state, which
	 * means that the SOA query was already performed by other means (e.g.
	 * by zone.c:soa_query()), or that it's a transfer without a preceding
	 * SOA request, and 'soa_transport_type' is already correctly
	 * set by the creator of the xfrin.
	 */
	if (atomic_load(&xfr->state) == XFRST_SOAQUERY) {
		/*
		 * The "SOA before" mode is used, where the SOA request is
		 * using the same transport as the XFR.
		 */
		atomic_store_relaxed(&xfr->soa_transport_type,
				     dns_xfrin_gettransporttype(xfr));
	}

	CHECK(dns_dispatch_add(
		xfr->disp, xfr->loop, 0, 0, &xfr->primaryaddr, xfr->transport,
		xfr->tlsctx_cache, xfrin_connect_done, xfrin_send_done,
		xfrin_recv_done, xfr, &xfr->id, &xfr->dispentry));

	/* Set the maximum timer */
	if (xfr->max_time_timer == NULL) {
		isc_timer_create(dns_zone_getloop(xfr->zone), xfrin_timedout,
				 xfr, &xfr->max_time_timer);
	}
	isc_interval_set(&interval, dns_zone_getmaxxfrin(xfr->zone), 0);
	isc_timer_start(xfr->max_time_timer, isc_timertype_once, &interval);

	/* Set the idle timer */
	if (xfr->max_idle_timer == NULL) {
		isc_timer_create(dns_zone_getloop(xfr->zone), xfrin_idledout,
				 xfr, &xfr->max_idle_timer);
	}
	isc_interval_set(&interval, dns_zone_getidlein(xfr->zone), 0);
	isc_timer_start(xfr->max_idle_timer, isc_timertype_once, &interval);

	/*
	 * The connect has to be the last thing that is called before returning,
	 * as it can end synchronously and destroy the xfr object.
	 */
	CHECK(dns_dispatch_connect(xfr->dispentry));

	return ISC_R_SUCCESS;

failure:
	xfrin_cancelio(xfr);
	dns_xfrin_detach(&xfr);

	return result;
}

/* XXX the resolver could use this, too */

static isc_result_t
render(dns_message_t *msg, isc_mem_t *mctx, isc_buffer_t *buf) {
	dns_compress_t cctx;
	isc_result_t result;

	dns_compress_init(&cctx, mctx, 0);
	CHECK(dns_message_renderbegin(msg, &cctx, buf));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_QUESTION, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_ANSWER, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_AUTHORITY, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_ADDITIONAL, 0));
	CHECK(dns_message_renderend(msg));
	result = ISC_R_SUCCESS;
failure:
	dns_compress_invalidate(&cctx);
	return result;
}

/*
 * A connection has been established.
 */
static void
xfrin_connect_done(isc_result_t result, isc_region_t *region ISC_ATTR_UNUSED,
		   void *arg) {
	dns_xfrin_t *xfr = (dns_xfrin_t *)arg;
	char addrtext[ISC_SOCKADDR_FORMATSIZE];
	char signerbuf[DNS_NAME_FORMATSIZE];
	const char *signer = "", *sep = "";
	dns_zonemgr_t *zmgr = NULL;

	REQUIRE(VALID_XFRIN(xfr));

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	LIBDNS_XFRIN_CONNECTED(xfr, xfr->info, result);

	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed to connect");
		goto failure;
	}

	result = dns_dispatch_checkperm(xfr->disp);
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "connected but unable to transfer");
		goto failure;
	}

	zmgr = dns_zone_getmgr(xfr->zone);
	if (zmgr != NULL) {
		dns_zonemgr_unreachabledel(zmgr, &xfr->primaryaddr,
					   &xfr->sourceaddr);
	}

	if (xfr->tsigkey != NULL && xfr->tsigkey->key != NULL) {
		dns_name_format(dst_key_name(xfr->tsigkey->key), signerbuf,
				sizeof(signerbuf));
		sep = " TSIG ";
		signer = signerbuf;
	}

	isc_sockaddr_format(&xfr->primaryaddr, addrtext, sizeof(addrtext));
	xfrin_log(xfr, ISC_LOG_INFO, "connected using %s%s%s", addrtext, sep,
		  signer);

	result = xfrin_send_request(xfr);
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "connected but unable to send");
		goto detach;
	}

	return;

failure:
	switch (result) {
	case ISC_R_NETDOWN:
	case ISC_R_HOSTDOWN:
	case ISC_R_NETUNREACH:
	case ISC_R_HOSTUNREACH:
	case ISC_R_CONNREFUSED:
	case ISC_R_TIMEDOUT:
		/*
		 * Add the server to unreachable primaries table if
		 * the server has a permanent networking error or
		 * the connection attempt as timed out.
		 */
		zmgr = dns_zone_getmgr(xfr->zone);
		if (zmgr != NULL) {
			isc_time_t now = isc_time_now();

			dns_zonemgr_unreachableadd(zmgr, &xfr->primaryaddr,
						   &xfr->sourceaddr, &now);
		}
		break;
	default:
		/* Retry sooner than in 10 minutes */
		break;
	}

detach:
	dns_xfrin_detach(&xfr);
}

/*
 * Convert a tuple into a dns_name_t suitable for inserting
 * into the given dns_message_t.
 */
static void
tuple2msgname(dns_difftuple_t *tuple, dns_message_t *msg, dns_name_t **target) {
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *rdl = NULL;
	dns_rdataset_t *rds = NULL;
	dns_name_t *name = NULL;

	REQUIRE(target != NULL && *target == NULL);

	dns_message_gettemprdata(msg, &rdata);
	dns_rdata_init(rdata);
	dns_rdata_clone(&tuple->rdata, rdata);

	dns_message_gettemprdatalist(msg, &rdl);
	dns_rdatalist_init(rdl);
	rdl->type = tuple->rdata.type;
	rdl->rdclass = tuple->rdata.rdclass;
	rdl->ttl = tuple->ttl;
	ISC_LIST_APPEND(rdl->rdata, rdata, link);

	dns_message_gettemprdataset(msg, &rds);
	dns_rdatalist_tordataset(rdl, rds);

	dns_message_gettempname(msg, &name);
	dns_name_clone(&tuple->name, name);
	ISC_LIST_APPEND(name->list, rds, link);

	*target = name;
}

static const char *
request_type(dns_xfrin_t *xfr) {
	switch (xfr->reqtype) {
	case dns_rdatatype_soa:
		return "SOA";
	case dns_rdatatype_axfr:
		return "AXFR";
	case dns_rdatatype_ixfr:
		return "IXFR";
	default:
		ISC_UNREACHABLE();
	}
}

static isc_result_t
add_opt(dns_message_t *message, uint16_t udpsize, bool reqnsid,
	bool reqexpire) {
	isc_result_t result;
	dns_rdataset_t *rdataset = NULL;
	dns_ednsopt_t ednsopts[DNS_EDNSOPTIONS];
	int count = 0;

	/* Set EDNS options if applicable. */
	if (reqnsid) {
		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_NSID;
		ednsopts[count].length = 0;
		ednsopts[count].value = NULL;
		count++;
	}
	if (reqexpire) {
		INSIST(count < DNS_EDNSOPTIONS);
		ednsopts[count].code = DNS_OPT_EXPIRE;
		ednsopts[count].length = 0;
		ednsopts[count].value = NULL;
		count++;
	}
	result = dns_message_buildopt(message, &rdataset, 0, udpsize, 0,
				      ednsopts, count);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	return dns_message_setopt(message, rdataset);
}

/*
 * Build an *XFR request and send its length prefix.
 */
static isc_result_t
xfrin_send_request(dns_xfrin_t *xfr) {
	isc_result_t result;
	isc_region_t region;
	dns_rdataset_t *qrdataset = NULL;
	dns_message_t *msg = NULL;
	dns_difftuple_t *soatuple = NULL;
	dns_name_t *qname = NULL;
	dns_dbversion_t *ver = NULL;
	dns_name_t *msgsoaname = NULL;
	bool edns = xfr->edns;
	bool reqnsid = xfr->view->requestnsid;
	bool reqexpire = dns_zone_getrequestexpire(xfr->zone);
	uint16_t udpsize = dns_view_getudpsize(xfr->view);

	LIBDNS_XFRIN_RECV_SEND_REQUEST(xfr, xfr->info);

	/* Create the request message */
	dns_message_create(xfr->mctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER,
			   &msg);
	CHECK(dns_message_settsigkey(msg, xfr->tsigkey));

	/* Create a name for the question section. */
	dns_message_gettempname(msg, &qname);
	dns_name_clone(&xfr->name, qname);

	/* Formulate the question and attach it to the question name. */
	dns_message_gettemprdataset(msg, &qrdataset);
	dns_rdataset_makequestion(qrdataset, xfr->rdclass, xfr->reqtype);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	qrdataset = NULL;

	dns_message_addname(msg, qname, DNS_SECTION_QUESTION);
	qname = NULL;

	if (xfr->reqtype == dns_rdatatype_ixfr) {
		/* Get the SOA and add it to the authority section. */
		dns_db_currentversion(xfr->db, &ver);
		CHECK(dns_db_createsoatuple(xfr->db, ver, xfr->mctx,
					    DNS_DIFFOP_EXISTS, &soatuple));
		xfr->ixfr.request_serial = dns_soa_getserial(&soatuple->rdata);
		xfr->ixfr.current_serial = xfr->ixfr.request_serial;
		xfrin_log(xfr, ISC_LOG_DEBUG(3),
			  "requesting IXFR for serial %u",
			  xfr->ixfr.request_serial);

		tuple2msgname(soatuple, msg, &msgsoaname);
		dns_message_addname(msg, msgsoaname, DNS_SECTION_AUTHORITY);
	} else if (xfr->reqtype == dns_rdatatype_soa) {
		CHECK(dns_db_getsoaserial(xfr->db, NULL,
					  &xfr->ixfr.request_serial));
	}

	if (edns && xfr->view->peers != NULL) {
		dns_peer_t *peer = NULL;
		isc_netaddr_t primaryip;
		isc_netaddr_fromsockaddr(&primaryip, &xfr->primaryaddr);
		result = dns_peerlist_peerbyaddr(xfr->view->peers, &primaryip,
						 &peer);
		if (result == ISC_R_SUCCESS) {
			(void)dns_peer_getsupportedns(peer, &edns);
			(void)dns_peer_getudpsize(peer, &udpsize);
			(void)dns_peer_getrequestnsid(peer, &reqnsid);
			(void)dns_peer_getrequestexpire(peer, &reqexpire);
		}
	}

	if (edns) {
		CHECK(add_opt(msg, udpsize, reqnsid, reqexpire));
	}

	atomic_store_relaxed(&xfr->nmsg, 0);
	atomic_store_relaxed(&xfr->nrecs, 0);
	atomic_store_relaxed(&xfr->nbytes, 0);
	atomic_store_relaxed(&xfr->start, isc_time_now());

	msg->id = xfr->id;
	if (xfr->tsigctx != NULL) {
		dst_context_destroy(&xfr->tsigctx);
	}

	CHECK(render(msg, xfr->mctx, &xfr->qbuffer));

	/*
	 * Free the last tsig, if there is one.
	 */
	if (xfr->lasttsig != NULL) {
		isc_buffer_free(&xfr->lasttsig);
	}

	/*
	 * Save the query TSIG and don't let message_destroy free it.
	 */
	CHECK(dns_message_getquerytsig(msg, xfr->mctx, &xfr->lasttsig));

	isc_buffer_usedregion(&xfr->qbuffer, &region);
	INSIST(region.length <= 65535);

	dns_xfrin_ref(xfr);
	dns_dispatch_send(xfr->dispentry, &region);
	xfrin_log(xfr, ISC_LOG_DEBUG(3), "sending %s request, QID %d",
		  request_type(xfr), xfr->id);

failure:
	dns_message_detach(&msg);
	if (soatuple != NULL) {
		dns_difftuple_free(&soatuple);
	}
	if (ver != NULL) {
		dns_db_closeversion(xfr->db, &ver, false);
	}

	return result;
}

static void
xfrin_send_done(isc_result_t result, isc_region_t *region, void *arg) {
	dns_xfrin_t *xfr = (dns_xfrin_t *)arg;

	UNUSED(region);

	REQUIRE(VALID_XFRIN(xfr));

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	LIBDNS_XFRIN_SENT(xfr, xfr->info, result);

	CHECK(result);

	xfrin_log(xfr, ISC_LOG_DEBUG(3), "sent request data");

failure:
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed sending request data");
	}

	dns_xfrin_detach(&xfr);
}

static void
get_edns_expire(dns_xfrin_t *xfr, dns_message_t *msg) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_buffer_t optbuf;
	uint16_t optcode;
	uint16_t optlen;

	result = dns_rdataset_first(msg->opt);
	if (result == ISC_R_SUCCESS) {
		dns_rdataset_current(msg->opt, &rdata);
		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		while (isc_buffer_remaininglength(&optbuf) >= 4) {
			optcode = isc_buffer_getuint16(&optbuf);
			optlen = isc_buffer_getuint16(&optbuf);
			/*
			 * A EDNS EXPIRE response has a length of 4.
			 */
			if (optcode != DNS_OPT_EXPIRE || optlen != 4) {
				isc_buffer_forward(&optbuf, optlen);
				continue;
			}
			xfr->expireopt = isc_buffer_getuint32(&optbuf);
			xfr->expireoptset = true;
			dns_zone_log(xfr->zone, ISC_LOG_DEBUG(1),
				     "got EDNS EXPIRE of %u", xfr->expireopt);
			break;
		}
	}
}

static void
xfrin_end(dns_xfrin_t *xfr, isc_result_t result) {
	/* Inform the caller. */
	if (xfr->done != NULL) {
		LIBDNS_XFRIN_DONE_CALLBACK_BEGIN(xfr, xfr->info, result);
		(xfr->done)(xfr->zone,
			    xfr->expireoptset ? &xfr->expireopt : NULL, result);
		xfr->done = NULL;
		LIBDNS_XFRIN_DONE_CALLBACK_END(xfr, xfr->info, result);
	}

	atomic_store(&xfr->shuttingdown, true);

	if (xfr->max_time_timer != NULL) {
		isc_timer_stop(xfr->max_time_timer);
		isc_timer_destroy(&xfr->max_time_timer);
	}
	if (xfr->max_idle_timer != NULL) {
		isc_timer_stop(xfr->max_idle_timer);
		isc_timer_destroy(&xfr->max_idle_timer);
	}

	if (xfr->shutdown_result == ISC_R_UNSET) {
		xfr->shutdown_result = result;
	}
}

static void
xfrin_recv_done(isc_result_t result, isc_region_t *region, void *arg) {
	dns_xfrin_t *xfr = (dns_xfrin_t *)arg;
	dns_message_t *msg = NULL;
	dns_name_t *name = NULL;
	const dns_name_t *tsigowner = NULL;
	isc_buffer_t buffer;

	REQUIRE(VALID_XFRIN(xfr));

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	/* Stop the idle timer */
	isc_timer_stop(xfr->max_idle_timer);

	LIBDNS_XFRIN_RECV_START(xfr, xfr->info, result);

	CHECK(result);

	xfrin_log(xfr, ISC_LOG_DEBUG(7), "received %u bytes", region->length);

	dns_message_create(xfr->mctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE,
			   &msg);

	CHECK(dns_message_settsigkey(msg, xfr->tsigkey));
	dns_message_setquerytsig(msg, xfr->lasttsig);

	msg->tsigctx = xfr->tsigctx;
	xfr->tsigctx = NULL;

	dns_message_setclass(msg, xfr->rdclass);

	msg->tcp_continuation = (atomic_load_relaxed(&xfr->nmsg) > 0) ? 1 : 0;

	isc_buffer_init(&buffer, region->base, region->length);
	isc_buffer_add(&buffer, region->length);

	result = dns_message_parse(msg, &buffer,
				   DNS_MESSAGEPARSE_PRESERVEORDER);
	if (result == ISC_R_SUCCESS) {
		dns_message_logpacket(
			msg, "received message from", &xfr->primaryaddr,
			DNS_LOGCATEGORY_XFER_IN, DNS_LOGMODULE_XFER_IN,
			ISC_LOG_DEBUG(10), xfr->mctx);
	} else {
		xfrin_log(xfr, ISC_LOG_DEBUG(10), "dns_message_parse: %s",
			  isc_result_totext(result));
	}

	LIBDNS_XFRIN_RECV_PARSED(xfr, xfr->info, result);

	if (result != ISC_R_SUCCESS || msg->rcode != dns_rcode_noerror ||
	    msg->opcode != dns_opcode_query || msg->rdclass != xfr->rdclass)
	{
		if (result == ISC_R_SUCCESS &&
		    msg->rcode == dns_rcode_formerr && xfr->edns &&
		    (atomic_load(&xfr->state) == XFRST_SOAQUERY ||
		     atomic_load(&xfr->state) == XFRST_ZONEXFRREQUEST))
		{
			xfr->edns = false;
			dns_message_detach(&msg);
			xfrin_reset(xfr);
			goto try_again;
		} else if (result == ISC_R_SUCCESS &&
			   msg->rcode != dns_rcode_noerror)
		{
			result = dns_result_fromrcode(msg->rcode);
		} else if (result == ISC_R_SUCCESS &&
			   msg->opcode != dns_opcode_query)
		{
			result = DNS_R_UNEXPECTEDOPCODE;
		} else if (result == ISC_R_SUCCESS &&
			   msg->rdclass != xfr->rdclass)
		{
			result = DNS_R_BADCLASS;
		} else if (result == ISC_R_SUCCESS || result == DNS_R_NOERROR) {
			result = DNS_R_UNEXPECTEDID;
		}

		if (xfr->reqtype == dns_rdatatype_axfr ||
		    xfr->reqtype == dns_rdatatype_soa)
		{
			goto failure;
		}

		xfrin_log(xfr, ISC_LOG_DEBUG(3), "got %s, retrying with AXFR",
			  isc_result_totext(result));
	try_axfr:
		LIBDNS_XFRIN_RECV_TRY_AXFR(xfr, xfr->info, result);
		dns_message_detach(&msg);
		xfrin_reset(xfr);
		xfr->reqtype = dns_rdatatype_soa;
		atomic_store(&xfr->state, XFRST_SOAQUERY);
	try_again:
		result = xfrin_start(xfr);
		if (result != ISC_R_SUCCESS) {
			xfrin_fail(xfr, result, "failed setting up socket");
		}
		dns_xfrin_detach(&xfr);
		return;
	}

	/*
	 * The question section should exist for SOA and in the first
	 * message of a AXFR or IXFR response.  The question section
	 * may exist in the 2nd and subsequent messages in a AXFR or
	 * IXFR response.  If the question section exists it should
	 * match the question that was sent.
	 */
	if (msg->counts[DNS_SECTION_QUESTION] > 1) {
		xfrin_log(xfr, ISC_LOG_NOTICE, "too many questions (%u)",
			  msg->counts[DNS_SECTION_QUESTION]);
		result = DNS_R_FORMERR;
		goto failure;
	}

	if ((atomic_load(&xfr->state) == XFRST_SOAQUERY ||
	     atomic_load(&xfr->state) == XFRST_ZONEXFRREQUEST) &&
	    msg->counts[DNS_SECTION_QUESTION] != 1)
	{
		xfrin_log(xfr, ISC_LOG_NOTICE, "missing question section");
		result = DNS_R_FORMERR;
		goto failure;
	}

	for (result = dns_message_firstname(msg, DNS_SECTION_QUESTION);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(msg, DNS_SECTION_QUESTION))
	{
		dns_rdataset_t *rds = NULL;

		LIBDNS_XFRIN_RECV_QUESTION(xfr, xfr->info, msg);

		name = NULL;
		dns_message_currentname(msg, DNS_SECTION_QUESTION, &name);
		if (!dns_name_equal(name, &xfr->name)) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question name mismatch");
			result = DNS_R_FORMERR;
			goto failure;
		}
		rds = ISC_LIST_HEAD(name->list);
		INSIST(rds != NULL);
		if (rds->type != xfr->reqtype) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question type mismatch");
			result = DNS_R_FORMERR;
			goto failure;
		}
		if (rds->rdclass != xfr->rdclass) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question class mismatch");
			result = DNS_R_FORMERR;
			goto failure;
		}
	}
	if (result != ISC_R_NOMORE) {
		goto failure;
	}

	/*
	 * Does the server know about IXFR?  If it doesn't we will get
	 * a message with a empty answer section or a potentially a CNAME /
	 * DNAME, the later is handled by xfr_rr() which will return FORMERR
	 * if the first RR in the answer section is not a SOA record.
	 */
	if (xfr->reqtype == dns_rdatatype_ixfr &&
	    atomic_load(&xfr->state) == XFRST_ZONEXFRREQUEST &&
	    msg->counts[DNS_SECTION_ANSWER] == 0)
	{
		xfrin_log(xfr, ISC_LOG_DEBUG(3),
			  "empty answer section, retrying with AXFR");
		goto try_axfr;
	}

	if (xfr->reqtype == dns_rdatatype_soa &&
	    (msg->flags & DNS_MESSAGEFLAG_AA) == 0)
	{
		result = DNS_R_NOTAUTHORITATIVE;
		goto failure;
	}

	result = dns_message_checksig(msg, xfr->view);
	if (result != ISC_R_SUCCESS) {
		xfrin_log(xfr, ISC_LOG_DEBUG(3), "TSIG check failed: %s",
			  isc_result_totext(result));
		goto failure;
	}

	for (result = dns_message_firstname(msg, DNS_SECTION_ANSWER);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(msg, DNS_SECTION_ANSWER))
	{
		dns_rdataset_t *rds = NULL;

		LIBDNS_XFRIN_RECV_ANSWER(xfr, xfr->info, msg);

		name = NULL;
		dns_message_currentname(msg, DNS_SECTION_ANSWER, &name);
		for (rds = ISC_LIST_HEAD(name->list); rds != NULL;
		     rds = ISC_LIST_NEXT(rds, link))
		{
			for (result = dns_rdataset_first(rds);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rds))
			{
				dns_rdata_t rdata = DNS_RDATA_INIT;
				dns_rdataset_current(rds, &rdata);
				CHECK(xfr_rr(xfr, name, rds->ttl, &rdata));
			}
		}
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}
	CHECK(result);

	if (dns_message_gettsig(msg, &tsigowner) != NULL) {
		/*
		 * Reset the counter.
		 */
		xfr->sincetsig = 0;

		/*
		 * Free the last tsig, if there is one.
		 */
		if (xfr->lasttsig != NULL) {
			isc_buffer_free(&xfr->lasttsig);
		}

		/*
		 * Update the last tsig pointer.
		 */
		CHECK(dns_message_getquerytsig(msg, xfr->mctx, &xfr->lasttsig));
	} else if (dns_message_gettsigkey(msg) != NULL) {
		xfr->sincetsig++;
		if (xfr->sincetsig > 100 ||
		    atomic_load_relaxed(&xfr->nmsg) == 0 ||
		    atomic_load(&xfr->state) == XFRST_AXFR_END ||
		    atomic_load(&xfr->state) == XFRST_IXFR_END)
		{
			result = DNS_R_EXPECTEDTSIG;
			goto failure;
		}
	}

	/*
	 * Update the number of messages and bytes received.
	 */
	atomic_fetch_add_relaxed(&xfr->nmsg, 1);
	atomic_fetch_add_relaxed(&xfr->nbytes, buffer.used);

	/*
	 * Take the context back.
	 */
	INSIST(xfr->tsigctx == NULL);
	xfr->tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;

	if (!xfr->expireoptset && msg->opt != NULL) {
		get_edns_expire(xfr, msg);
	}

	switch (atomic_load(&xfr->state)) {
	case XFRST_GOTSOA:
		xfr->reqtype = dns_rdatatype_axfr;
		atomic_store(&xfr->state, XFRST_ZONEXFRREQUEST);
		CHECK(xfrin_start(xfr));
		break;
	case XFRST_AXFR_END:
	case XFRST_IXFR_END:
		/* We are at the end, cancel the timers and IO */
		isc_timer_stop(xfr->max_idle_timer);
		isc_timer_stop(xfr->max_time_timer);
		xfrin_cancelio(xfr);
		break;
	default:
		/*
		 * Read the next message.
		 */
		dns_message_detach(&msg);
		result = dns_dispatch_getnext(xfr->dispentry);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}

		isc_interval_t interval;
		isc_interval_set(&interval, dns_zone_getidlein(xfr->zone), 0);
		isc_timer_start(xfr->max_idle_timer, isc_timertype_once,
				&interval);

		LIBDNS_XFRIN_READ(xfr, xfr->info, result);
		return;
	}

failure:
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed while receiving responses");
	}

	if (msg != NULL) {
		dns_message_detach(&msg);
	}
	dns_xfrin_detach(&xfr);
	LIBDNS_XFRIN_RECV_DONE(xfr, xfr->info, result);
}

static void
xfrin_destroy(dns_xfrin_t *xfr) {
	uint64_t msecs, persec;
	isc_time_t now = isc_time_now();
	char expireopt[sizeof("4000000000")] = { 0 };
	const char *sep = "";

	REQUIRE(VALID_XFRIN(xfr));

	/* Safe-guards */
	REQUIRE(atomic_load(&xfr->shuttingdown));

	INSIST(xfr->shutdown_result != ISC_R_UNSET);

	/*
	 * If we're called through dns_xfrin_detach() and are not
	 * shutting down, we can't know what the transfer status is as
	 * we are only called when the last reference is lost.
	 */
	xfrin_log(xfr, ISC_LOG_INFO, "Transfer status: %s",
		  isc_result_totext(xfr->shutdown_result));

	/*
	 * Calculate the length of time the transfer took,
	 * and print a log message with the bytes and rate.
	 */
	isc_time_t start = atomic_load_relaxed(&xfr->start);
	msecs = isc_time_microdiff(&now, &start) / 1000;
	if (msecs == 0) {
		msecs = 1;
	}
	persec = (atomic_load_relaxed(&xfr->nbytes) * 1000) / msecs;

	if (xfr->expireoptset) {
		sep = ", expire option ";
		snprintf(expireopt, sizeof(expireopt), "%u", xfr->expireopt);
	}

	xfrin_log(xfr, ISC_LOG_INFO,
		  "Transfer completed: %d messages, %d records, "
		  "%" PRIu64 " bytes, "
		  "%u.%03u secs (%u bytes/sec) (serial %" PRIuFAST32 "%s%s)",
		  atomic_load_relaxed(&xfr->nmsg),
		  atomic_load_relaxed(&xfr->nrecs),
		  atomic_load_relaxed(&xfr->nbytes),
		  (unsigned int)(msecs / 1000), (unsigned int)(msecs % 1000),
		  (unsigned int)persec, atomic_load_relaxed(&xfr->end_serial),
		  sep, expireopt);

	/* Cleanup unprocessed IXFR data */
	struct cds_wfcq_node *node, *next;
	__cds_wfcq_for_each_blocking_safe(&xfr->diff_head, &xfr->diff_tail,
					  node, next) {
		ixfr_apply_data_t *data =
			caa_container_of(node, ixfr_apply_data_t, wfcq_node);
		/* We need to clear and free all data chunks */
		dns_diff_clear(&data->diff);
		isc_mem_put(xfr->mctx, data, sizeof(*data));
	}

	/* Cleanup unprocessed AXFR data */
	dns_diff_clear(&xfr->diff);

	xfrin_cancelio(xfr);

	if (xfr->transport != NULL) {
		dns_transport_detach(&xfr->transport);
	}

	if (xfr->tsigkey != NULL) {
		dns_tsigkey_detach(&xfr->tsigkey);
	}

	if (xfr->lasttsig != NULL) {
		isc_buffer_free(&xfr->lasttsig);
	}

	if (xfr->ixfr.journal != NULL) {
		dns_journal_destroy(&xfr->ixfr.journal);
	}

	if (xfr->axfr.add_private != NULL) {
		(void)dns_db_endload(xfr->db, &xfr->axfr);
	}

	if (xfr->tsigctx != NULL) {
		dst_context_destroy(&xfr->tsigctx);
	}

	if (xfr->name.attributes.dynamic) {
		dns_name_free(&xfr->name, xfr->mctx);
	}

	if (xfr->ver != NULL) {
		dns_db_closeversion(xfr->db, &xfr->ver, false);
	}

	if (xfr->db != NULL) {
		dns_db_detach(&xfr->db);
	}

	if (xfr->zone != NULL) {
		if (!xfr->zone_had_db &&
		    xfr->shutdown_result == ISC_R_SUCCESS &&
		    dns_zone_gettype(xfr->zone) == dns_zone_mirror)
		{
			dns_zone_log(xfr->zone, ISC_LOG_INFO,
				     "mirror zone is now in use");
		}
		xfrin_log(xfr, ISC_LOG_DEBUG(99), "freeing transfer context");
		/*
		 * xfr->zone must not be detached before xfrin_log() is called.
		 */
		dns_zone_idetach(&xfr->zone);
	}

	if (xfr->view != NULL) {
		dns_view_weakdetach(&xfr->view);
	}

	if (xfr->firstsoa_data != NULL) {
		isc_mem_free(xfr->mctx, xfr->firstsoa_data);
	}

	if (xfr->tlsctx_cache != NULL) {
		isc_tlsctx_cache_detach(&xfr->tlsctx_cache);
	}

	INSIST(xfr->max_time_timer == NULL);
	INSIST(xfr->max_idle_timer == NULL);

	isc_loop_detach(&xfr->loop);

	isc_mem_putanddetach(&xfr->mctx, xfr, sizeof(*xfr));
}

/*
 * Log incoming zone transfer messages in a format like
 * transfer of <zone> from <address>: <message>
 */

static void
xfrin_log(dns_xfrin_t *xfr, int level, const char *fmt, ...) {
	va_list ap;
	char primarytext[ISC_SOCKADDR_FORMATSIZE];
	char msgtext[2048];

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	isc_sockaddr_format(&xfr->primaryaddr, primarytext,
			    sizeof(primarytext));
	va_start(ap, fmt);
	vsnprintf(msgtext, sizeof(msgtext), fmt, ap);
	va_end(ap);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_XFER_IN, DNS_LOGMODULE_XFER_IN,
		      level, "%p: transfer of '%s' from %s: %s", xfr, xfr->info,
		      primarytext, msgtext);
}
