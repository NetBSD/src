/*	$NetBSD: xfrin.c,v 1.11.2.2 2024/02/25 15:46:54 martin Exp $	*/

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

#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/catz.h>
#include <dns/db.h>
#include <dns/diff.h>
#include <dns/events.h>
#include <dns/journal.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/soa.h>
#include <dns/transport.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/xfrin.h>
#include <dns/zone.h>

#include <dst/dst.h>

/*
 * Incoming AXFR and IXFR.
 */

/*%
 * It would be non-sensical (or at least obtuse) to use FAIL() with an
 * ISC_R_SUCCESS code, but the test is there to keep the Solaris compiler
 * from complaining about "end-of-loop code not reached".
 */
#define FAIL(code)                           \
	do {                                 \
		result = (code);             \
		if (result != ISC_R_SUCCESS) \
			goto failure;        \
	} while (0)

#define CHECK(op)                            \
	do {                                 \
		result = (op);               \
		if (result != ISC_R_SUCCESS) \
			goto failure;        \
	} while (0)

/*%
 * The states of the *XFR state machine.  We handle both IXFR and AXFR
 * with a single integrated state machine because they cannot be distinguished
 * immediately - an AXFR response to an IXFR request can only be detected
 * when the first two (2) response RRs have already been received.
 */
typedef enum {
	XFRST_SOAQUERY,
	XFRST_GOTSOA,
	XFRST_INITIALSOA,
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

struct dns_xfrin_ctx {
	unsigned int magic;
	isc_mem_t *mctx;
	dns_zone_t *zone;

	isc_refcount_t references;

	isc_nm_t *netmgr;

	isc_refcount_t connects; /*%< Connect in progress */
	isc_refcount_t sends;	 /*%< Send in progress */
	isc_refcount_t recvs;	 /*%< Receive in progress */

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

	isc_nmhandle_t *handle;
	isc_nmhandle_t *readhandle;
	isc_nmhandle_t *sendhandle;

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
	int difflen;	 /*%< Number of pending tuples */

	xfrin_state_t state;
	uint32_t end_serial;
	bool is_ixfr;

	unsigned int nmsg;  /*%< Number of messages recvd */
	unsigned int nrecs; /*%< Number of records recvd */
	uint64_t nbytes;    /*%< Number of bytes received */

	unsigned int maxrecords; /*%< The maximum number of
				  *   records set for the zone */

	isc_time_t start; /*%< Start time of the transfer */
	isc_time_t end;	  /*%< End time of the transfer */

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

	isc_timer_t *max_time_timer;
	isc_timer_t *max_idle_timer;
};

#define XFRIN_MAGIC    ISC_MAGIC('X', 'f', 'r', 'I')
#define VALID_XFRIN(x) ISC_MAGIC_VALID(x, XFRIN_MAGIC)

/**************************************************************************/
/*
 * Forward declarations.
 */

static void
xfrin_create(isc_mem_t *mctx, dns_zone_t *zone, dns_db_t *db, isc_nm_t *netmgr,
	     dns_name_t *zonename, dns_rdataclass_t rdclass,
	     dns_rdatatype_t reqtype, const isc_sockaddr_t *primaryaddr,
	     const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
	     dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
	     dns_xfrin_ctx_t **xfrp);

static isc_result_t
axfr_init(dns_xfrin_ctx_t *xfr);
static isc_result_t
axfr_makedb(dns_xfrin_ctx_t *xfr, dns_db_t **dbp);
static isc_result_t
axfr_putdata(dns_xfrin_ctx_t *xfr, dns_diffop_t op, dns_name_t *name,
	     dns_ttl_t ttl, dns_rdata_t *rdata);
static isc_result_t
axfr_apply(dns_xfrin_ctx_t *xfr);
static isc_result_t
axfr_commit(dns_xfrin_ctx_t *xfr);
static isc_result_t
axfr_finalize(dns_xfrin_ctx_t *xfr);

static isc_result_t
ixfr_init(dns_xfrin_ctx_t *xfr);
static isc_result_t
ixfr_apply(dns_xfrin_ctx_t *xfr);
static isc_result_t
ixfr_putdata(dns_xfrin_ctx_t *xfr, dns_diffop_t op, dns_name_t *name,
	     dns_ttl_t ttl, dns_rdata_t *rdata);
static isc_result_t
ixfr_commit(dns_xfrin_ctx_t *xfr);

static isc_result_t
xfr_rr(dns_xfrin_ctx_t *xfr, dns_name_t *name, uint32_t ttl,
       dns_rdata_t *rdata);

static isc_result_t
xfrin_start(dns_xfrin_ctx_t *xfr);

static void
xfrin_connect_done(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);
static isc_result_t
xfrin_send_request(dns_xfrin_ctx_t *xfr);
static void
xfrin_send_done(isc_nmhandle_t *handle, isc_result_t result, void *cbarg);
static void
xfrin_recv_done(isc_nmhandle_t *handle, isc_result_t result,
		isc_region_t *region, void *cbarg);

static void
xfrin_destroy(dns_xfrin_ctx_t *xfr);

static void
xfrin_timedout(struct isc_task *, struct isc_event *);
static void
xfrin_idledout(struct isc_task *, struct isc_event *);
static void
xfrin_fail(dns_xfrin_ctx_t *xfr, isc_result_t result, const char *msg);
static isc_result_t
render(dns_message_t *msg, isc_mem_t *mctx, isc_buffer_t *buf);

static void
xfrin_logv(int level, const char *zonetext, const isc_sockaddr_t *primaryaddr,
	   const char *fmt, va_list ap) ISC_FORMAT_PRINTF(4, 0);

static void
xfrin_log1(int level, const char *zonetext, const isc_sockaddr_t *primaryaddr,
	   const char *fmt, ...) ISC_FORMAT_PRINTF(4, 5);

static void
xfrin_log(dns_xfrin_ctx_t *xfr, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

/**************************************************************************/
/*
 * AXFR handling
 */

static isc_result_t
axfr_init(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;

	xfr->is_ixfr = false;

	if (xfr->db != NULL) {
		dns_db_detach(&xfr->db);
	}

	CHECK(axfr_makedb(xfr, &xfr->db));
	dns_rdatacallbacks_init(&xfr->axfr);
	CHECK(dns_db_beginload(xfr->db, &xfr->axfr));
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

static isc_result_t
axfr_makedb(dns_xfrin_ctx_t *xfr, dns_db_t **dbp) {
	isc_result_t result;

	result = dns_db_create(xfr->mctx, /* XXX */
			       "rbt",	  /* XXX guess */
			       &xfr->name, dns_dbtype_zone, xfr->rdclass, 0,
			       NULL, /* XXX guess */
			       dbp);
	if (result == ISC_R_SUCCESS) {
		dns_zone_rpz_enable_db(xfr->zone, *dbp);
		dns_zone_catz_enable_db(xfr->zone, *dbp);
	}
	return (result);
}

static isc_result_t
axfr_putdata(dns_xfrin_ctx_t *xfr, dns_diffop_t op, dns_name_t *name,
	     dns_ttl_t ttl, dns_rdata_t *rdata) {
	isc_result_t result;

	dns_difftuple_t *tuple = NULL;

	if (rdata->rdclass != xfr->rdclass) {
		return (DNS_R_BADCLASS);
	}

	CHECK(dns_zone_checknames(xfr->zone, name, rdata));
	CHECK(dns_difftuple_create(xfr->diff.mctx, op, name, ttl, rdata,
				   &tuple));
	dns_diff_append(&xfr->diff, &tuple);
	if (++xfr->difflen > 100) {
		CHECK(axfr_apply(xfr));
	}
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

/*
 * Store a set of AXFR RRs in the database.
 */
static isc_result_t
axfr_apply(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;
	uint64_t records;

	CHECK(dns_diff_load(&xfr->diff, xfr->axfr.add, xfr->axfr.add_private));
	xfr->difflen = 0;
	dns_diff_clear(&xfr->diff);
	if (xfr->maxrecords != 0U) {
		result = dns_db_getsize(xfr->db, xfr->ver, &records, NULL);
		if (result == ISC_R_SUCCESS && records > xfr->maxrecords) {
			result = DNS_R_TOOMANYRECORDS;
			goto failure;
		}
	}
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

static isc_result_t
axfr_commit(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;

	CHECK(axfr_apply(xfr));
	CHECK(dns_db_endload(xfr->db, &xfr->axfr));
	CHECK(dns_zone_verifydb(xfr->zone, xfr->db, NULL));

	result = ISC_R_SUCCESS;
failure:
	return (result);
}

static isc_result_t
axfr_finalize(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;

	CHECK(dns_zone_replacedb(xfr->zone, xfr->db, true));

	result = ISC_R_SUCCESS;
failure:
	return (result);
}

/**************************************************************************/
/*
 * IXFR handling
 */

static isc_result_t
ixfr_init(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;
	char *journalfile = NULL;

	if (xfr->reqtype != dns_rdatatype_ixfr) {
		xfrin_log(xfr, ISC_LOG_NOTICE,
			  "got incremental response to AXFR request");
		return (DNS_R_FORMERR);
	}

	xfr->is_ixfr = true;
	INSIST(xfr->db != NULL);
	xfr->difflen = 0;

	journalfile = dns_zone_getjournal(xfr->zone);
	if (journalfile != NULL) {
		CHECK(dns_journal_open(xfr->mctx, journalfile,
				       DNS_JOURNAL_CREATE, &xfr->ixfr.journal));
	}

	result = ISC_R_SUCCESS;
failure:
	return (result);
}

static isc_result_t
ixfr_putdata(dns_xfrin_ctx_t *xfr, dns_diffop_t op, dns_name_t *name,
	     dns_ttl_t ttl, dns_rdata_t *rdata) {
	isc_result_t result;
	dns_difftuple_t *tuple = NULL;

	if (rdata->rdclass != xfr->rdclass) {
		return (DNS_R_BADCLASS);
	}

	if (op == DNS_DIFFOP_ADD) {
		CHECK(dns_zone_checknames(xfr->zone, name, rdata));
	}
	CHECK(dns_difftuple_create(xfr->diff.mctx, op, name, ttl, rdata,
				   &tuple));
	dns_diff_append(&xfr->diff, &tuple);
	if (++xfr->difflen > 100) {
		CHECK(ixfr_apply(xfr));
	}
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

/*
 * Apply a set of IXFR changes to the database.
 */
static isc_result_t
ixfr_apply(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;
	uint64_t records;

	if (xfr->ver == NULL) {
		CHECK(dns_db_newversion(xfr->db, &xfr->ver));
		if (xfr->ixfr.journal != NULL) {
			CHECK(dns_journal_begin_transaction(xfr->ixfr.journal));
		}
	}
	CHECK(dns_diff_apply(&xfr->diff, xfr->db, xfr->ver));
	if (xfr->maxrecords != 0U) {
		result = dns_db_getsize(xfr->db, xfr->ver, &records, NULL);
		if (result == ISC_R_SUCCESS && records > xfr->maxrecords) {
			result = DNS_R_TOOMANYRECORDS;
			goto failure;
		}
	}
	if (xfr->ixfr.journal != NULL) {
		result = dns_journal_writediff(xfr->ixfr.journal, &xfr->diff);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
	}
	dns_diff_clear(&xfr->diff);
	xfr->difflen = 0;
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

static isc_result_t
ixfr_commit(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;

	CHECK(ixfr_apply(xfr));
	if (xfr->ver != NULL) {
		CHECK(dns_zone_verifydb(xfr->zone, xfr->db, xfr->ver));
		/* XXX enter ready-to-commit state here */
		if (xfr->ixfr.journal != NULL) {
			CHECK(dns_journal_commit(xfr->ixfr.journal));
		}
		dns_db_closeversion(xfr->db, &xfr->ver, true);
		dns_zone_markdirty(xfr->zone);
	}
	result = ISC_R_SUCCESS;
failure:
	return (result);
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
xfr_rr(dns_xfrin_ctx_t *xfr, dns_name_t *name, uint32_t ttl,
       dns_rdata_t *rdata) {
	isc_result_t result;

	xfr->nrecs++;

	if (rdata->type == dns_rdatatype_none ||
	    dns_rdatatype_ismeta(rdata->type))
	{
		char buf[64];
		dns_rdatatype_format(rdata->type, buf, sizeof(buf));
		xfrin_log(xfr, ISC_LOG_NOTICE,
			  "Unexpected %s record in zone transfer", buf);
		FAIL(DNS_R_FORMERR);
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
		FAIL(DNS_R_NOTZONETOP);
	}

redo:
	switch (xfr->state) {
	case XFRST_SOAQUERY:
		if (rdata->type != dns_rdatatype_soa) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "non-SOA response to SOA query");
			FAIL(DNS_R_FORMERR);
		}
		xfr->end_serial = dns_soa_getserial(rdata);
		if (!DNS_SERIAL_GT(xfr->end_serial, xfr->ixfr.request_serial) &&
		    !dns_zone_isforced(xfr->zone))
		{
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "requested serial %u, "
				  "primary has %u, not updating",
				  xfr->ixfr.request_serial, xfr->end_serial);
			FAIL(DNS_R_UPTODATE);
		}
		xfr->state = XFRST_GOTSOA;
		break;

	case XFRST_GOTSOA:
		/*
		 * Skip other records in the answer section.
		 */
		break;

	case XFRST_INITIALSOA:
		if (rdata->type != dns_rdatatype_soa) {
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "first RR in zone transfer must be SOA");
			FAIL(DNS_R_FORMERR);
		}
		/*
		 * Remember the serial number in the initial SOA.
		 * We need it to recognize the end of an IXFR.
		 */
		xfr->end_serial = dns_soa_getserial(rdata);
		if (xfr->reqtype == dns_rdatatype_ixfr &&
		    !DNS_SERIAL_GT(xfr->end_serial, xfr->ixfr.request_serial) &&
		    !dns_zone_isforced(xfr->zone))
		{
			/*
			 * This must be the single SOA record that is
			 * sent when the current version on the primary
			 * is not newer than the version in the request.
			 */
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "requested serial %u, "
				  "primary has %u, not updating",
				  xfr->ixfr.request_serial, xfr->end_serial);
			FAIL(DNS_R_UPTODATE);
		}
		xfr->firstsoa = *rdata;
		if (xfr->firstsoa_data != NULL) {
			isc_mem_free(xfr->mctx, xfr->firstsoa_data);
		}
		xfr->firstsoa_data = isc_mem_allocate(xfr->mctx, rdata->length);
		memcpy(xfr->firstsoa_data, rdata->data, rdata->length);
		xfr->firstsoa.data = xfr->firstsoa_data;
		xfr->state = XFRST_FIRSTDATA;
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
			xfr->state = XFRST_IXFR_DELSOA;
		} else {
			xfrin_log(xfr, ISC_LOG_DEBUG(3),
				  "got nonincremental response");
			CHECK(axfr_init(xfr));
			xfr->state = XFRST_AXFR;
		}
		goto redo;

	case XFRST_IXFR_DELSOA:
		INSIST(rdata->type == dns_rdatatype_soa);
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_DEL, name, ttl, rdata));
		xfr->state = XFRST_IXFR_DEL;
		break;

	case XFRST_IXFR_DEL:
		if (rdata->type == dns_rdatatype_soa) {
			uint32_t soa_serial = dns_soa_getserial(rdata);
			xfr->state = XFRST_IXFR_ADDSOA;
			xfr->ixfr.current_serial = soa_serial;
			goto redo;
		}
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_DEL, name, ttl, rdata));
		break;

	case XFRST_IXFR_ADDSOA:
		INSIST(rdata->type == dns_rdatatype_soa);
		CHECK(ixfr_putdata(xfr, DNS_DIFFOP_ADD, name, ttl, rdata));
		xfr->state = XFRST_IXFR_ADD;
		break;

	case XFRST_IXFR_ADD:
		if (rdata->type == dns_rdatatype_soa) {
			uint32_t soa_serial = dns_soa_getserial(rdata);
			if (soa_serial == xfr->end_serial) {
				CHECK(ixfr_commit(xfr));
				xfr->state = XFRST_IXFR_END;
				break;
			} else if (soa_serial != xfr->ixfr.current_serial) {
				xfrin_log(xfr, ISC_LOG_NOTICE,
					  "IXFR out of sync: "
					  "expected serial %u, got %u",
					  xfr->ixfr.current_serial, soa_serial);
				FAIL(DNS_R_FORMERR);
			} else {
				CHECK(ixfr_commit(xfr));
				xfr->state = XFRST_IXFR_DELSOA;
				goto redo;
			}
		}
		if (rdata->type == dns_rdatatype_ns &&
		    dns_name_iswildcard(name))
		{
			FAIL(DNS_R_INVALIDNS);
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
				FAIL(DNS_R_FORMERR);
			}
			CHECK(axfr_commit(xfr));
			xfr->state = XFRST_AXFR_END;
			break;
		}
		break;
	case XFRST_AXFR_END:
	case XFRST_IXFR_END:
		FAIL(DNS_R_EXTRADATA);
		FALLTHROUGH;
	default:
		UNREACHABLE();
	}
	result = ISC_R_SUCCESS;
failure:
	return (result);
}

isc_result_t
dns_xfrin_create(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		 const isc_sockaddr_t *primaryaddr,
		 const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
		 dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
		 isc_mem_t *mctx, isc_nm_t *netmgr, dns_xfrindone_t done,
		 dns_xfrin_ctx_t **xfrp) {
	dns_name_t *zonename = dns_zone_getorigin(zone);
	dns_xfrin_ctx_t *xfr = NULL;
	isc_result_t result;
	dns_db_t *db = NULL;

	REQUIRE(xfrp != NULL && *xfrp == NULL);
	REQUIRE(done != NULL);
	REQUIRE(isc_sockaddr_getport(primaryaddr) != 0);

	(void)dns_zone_getdb(zone, &db);

	if (xfrtype == dns_rdatatype_soa || xfrtype == dns_rdatatype_ixfr) {
		REQUIRE(db != NULL);
	}

	xfrin_create(mctx, zone, db, netmgr, zonename, dns_zone_getclass(zone),
		     xfrtype, primaryaddr, sourceaddr, tsigkey, transport,
		     tlsctx_cache, &xfr);

	if (db != NULL) {
		xfr->zone_had_db = true;
	}

	xfr->done = done;

	isc_refcount_init(&xfr->references, 1);

	/*
	 * Set *xfrp now, before calling xfrin_start(). Asynchronous
	 * netmgr processing could cause the 'done' callback to run in
	 * another thread before we reached the end of the present
	 * function. In that case, if *xfrp hadn't already been
	 * attached, the 'done' function would be unable to detach it.
	 */
	*xfrp = xfr;

	result = xfrin_start(xfr);
	if (result != ISC_R_SUCCESS) {
		atomic_store(&xfr->shuttingdown, true);
		xfr->shutdown_result = result;
		dns_xfrin_detach(xfrp);
	}

	if (db != NULL) {
		dns_db_detach(&db);
	}

	if (result != ISC_R_SUCCESS) {
		char zonetext[DNS_NAME_MAXTEXT + 32];
		dns_zone_name(zone, zonetext, sizeof(zonetext));
		xfrin_log1(ISC_LOG_ERROR, zonetext, primaryaddr,
			   "zone transfer setup failed");
	}

	return (result);
}

static void
xfrin_cancelio(dns_xfrin_ctx_t *xfr);

static void
xfrin_timedout(struct isc_task *task, struct isc_event *event) {
	UNUSED(task);

	dns_xfrin_ctx_t *xfr = event->ev_arg;
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_TIMEDOUT, "maximum transfer time exceeded");
	isc_event_free(&event);
}

static void
xfrin_idledout(struct isc_task *task, struct isc_event *event) {
	UNUSED(task);

	dns_xfrin_ctx_t *xfr = event->ev_arg;
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_TIMEDOUT, "maximum idle time exceeded");
	isc_event_free(&event);
}

void
dns_xfrin_shutdown(dns_xfrin_ctx_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_fail(xfr, ISC_R_CANCELED, "shut down");
}

void
dns_xfrin_attach(dns_xfrin_ctx_t *source, dns_xfrin_ctx_t **target) {
	REQUIRE(VALID_XFRIN(source));
	REQUIRE(target != NULL && *target == NULL);
	(void)isc_refcount_increment(&source->references);

	*target = source;
}

void
dns_xfrin_detach(dns_xfrin_ctx_t **xfrp) {
	dns_xfrin_ctx_t *xfr = NULL;

	REQUIRE(xfrp != NULL && VALID_XFRIN(*xfrp));

	xfr = *xfrp;
	*xfrp = NULL;

	if (isc_refcount_decrement(&xfr->references) == 1) {
		xfrin_destroy(xfr);
	}
}

static void
xfrin_cancelio(dns_xfrin_ctx_t *xfr) {
	if (xfr->readhandle == NULL) {
		return;
	}

	isc_nm_cancelread(xfr->readhandle);
	/* The xfr->readhandle detach will happen in xfrin_recv_done callback */
}

static void
xfrin_reset(dns_xfrin_ctx_t *xfr) {
	REQUIRE(VALID_XFRIN(xfr));

	xfrin_log(xfr, ISC_LOG_INFO, "resetting");

	REQUIRE(xfr->readhandle == NULL);
	REQUIRE(xfr->sendhandle == NULL);

	if (xfr->lasttsig != NULL) {
		isc_buffer_free(&xfr->lasttsig);
	}

	dns_diff_clear(&xfr->diff);
	xfr->difflen = 0;

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
xfrin_fail(dns_xfrin_ctx_t *xfr, isc_result_t result, const char *msg) {
	/* Make sure only the first xfrin_fail() trumps */
	if (atomic_compare_exchange_strong(&xfr->shuttingdown, &(bool){ false },
					   true))
	{
		(void)isc_timer_reset(xfr->max_time_timer,
				      isc_timertype_inactive, NULL, NULL, true);
		(void)isc_timer_reset(xfr->max_idle_timer,
				      isc_timertype_inactive, NULL, NULL, true);

		if (result != DNS_R_UPTODATE && result != DNS_R_TOOMANYRECORDS)
		{
			xfrin_log(xfr, ISC_LOG_ERROR, "%s: %s", msg,
				  isc_result_totext(result));
			if (xfr->is_ixfr) {
				/* Pass special result code to force AXFR retry
				 */
				result = DNS_R_BADIXFR;
			}
		}
		xfrin_cancelio(xfr);
		/*
		 * Close the journal.
		 */
		if (xfr->ixfr.journal != NULL) {
			dns_journal_destroy(&xfr->ixfr.journal);
		}
		if (xfr->done != NULL) {
			(xfr->done)(xfr->zone, result);
			xfr->done = NULL;
		}
		xfr->shutdown_result = result;
	}
}

static void
xfrin_create(isc_mem_t *mctx, dns_zone_t *zone, dns_db_t *db, isc_nm_t *netmgr,
	     dns_name_t *zonename, dns_rdataclass_t rdclass,
	     dns_rdatatype_t reqtype, const isc_sockaddr_t *primaryaddr,
	     const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
	     dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
	     dns_xfrin_ctx_t **xfrp) {
	dns_xfrin_ctx_t *xfr = NULL;
	dns_zonemgr_t *zmgr = dns_zone_getmgr(zone);
	isc_timermgr_t *timermgr = dns_zonemgr_gettimermgr(zmgr);
	isc_task_t *ztask = NULL;

	xfr = isc_mem_get(mctx, sizeof(*xfr));
	*xfr = (dns_xfrin_ctx_t){ .netmgr = netmgr,
				  .shutdown_result = ISC_R_UNSET,
				  .rdclass = rdclass,
				  .reqtype = reqtype,
				  .id = (dns_messageid_t)isc_random16(),
				  .maxrecords = dns_zone_getmaxrecords(zone),
				  .primaryaddr = *primaryaddr,
				  .sourceaddr = *sourceaddr,
				  .firstsoa = DNS_RDATA_INIT,
				  .magic = XFRIN_MAGIC };

	isc_mem_attach(mctx, &xfr->mctx);
	dns_zone_iattach(zone, &xfr->zone);
	dns_name_init(&xfr->name, NULL);

	isc_refcount_init(&xfr->connects, 0);
	isc_refcount_init(&xfr->sends, 0);
	isc_refcount_init(&xfr->recvs, 0);

	atomic_init(&xfr->shuttingdown, false);

	if (db != NULL) {
		dns_db_attach(db, &xfr->db);
	}

	dns_diff_init(xfr->mctx, &xfr->diff);

	if (reqtype == dns_rdatatype_soa) {
		xfr->state = XFRST_SOAQUERY;
	} else {
		xfr->state = XFRST_INITIALSOA;
	}

	isc_time_now(&xfr->start);

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

	dns_zone_gettask(zone, &ztask);
	isc_timer_create(timermgr, isc_timertype_inactive, NULL, NULL, ztask,
			 xfrin_timedout, xfr, &xfr->max_time_timer);
	isc_timer_create(timermgr, isc_timertype_inactive, NULL, NULL, ztask,
			 xfrin_idledout, xfr, &xfr->max_idle_timer);
	isc_task_detach(&ztask); /* dns_zone_task() attaches to the task */

	*xfrp = xfr;
}

static isc_result_t
get_create_tlsctx(const dns_xfrin_ctx_t *xfr, isc_tlsctx_t **pctx,
		  isc_tlsctx_client_session_cache_t **psess_cache) {
	isc_result_t result = ISC_R_FAILURE;
	isc_tlsctx_t *tlsctx = NULL, *found = NULL;
	isc_tls_cert_store_t *store = NULL, *found_store = NULL;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL,
					  *found_sess_cache = NULL;
	uint32_t tls_versions;
	const char *ciphers = NULL;
	bool prefer_server_ciphers;
	const uint16_t family = isc_sockaddr_pf(&xfr->primaryaddr) == PF_INET6
					? AF_INET6
					: AF_INET;
	const char *tlsname = NULL;

	REQUIRE(psess_cache != NULL && *psess_cache == NULL);
	REQUIRE(pctx != NULL && *pctx == NULL);

	INSIST(xfr->transport != NULL);
	tlsname = dns_transport_get_tlsname(xfr->transport);
	INSIST(tlsname != NULL && *tlsname != '\0');

	/*
	 * Let's try to re-use the already created context. This way
	 * we have a chance to resume the TLS session, bypassing the
	 * full TLS handshake procedure, making establishing
	 * subsequent TLS connections for XoT faster.
	 */
	result = isc_tlsctx_cache_find(xfr->tlsctx_cache, tlsname,
				       isc_tlsctx_cache_tls, family, &found,
				       &found_store, &found_sess_cache);
	if (result != ISC_R_SUCCESS) {
		const char *hostname =
			dns_transport_get_remote_hostname(xfr->transport);
		const char *ca_file = dns_transport_get_cafile(xfr->transport);
		const char *cert_file =
			dns_transport_get_certfile(xfr->transport);
		const char *key_file =
			dns_transport_get_keyfile(xfr->transport);
		char primary_addr_str[INET6_ADDRSTRLEN] = { 0 };
		isc_netaddr_t primary_netaddr = { 0 };
		bool hostname_ignore_subject;
		/*
		 * So, no context exists. Let's create one using the
		 * parameters from the configuration file and try to
		 * store it for further reuse.
		 */
		result = isc_tlsctx_createclient(&tlsctx);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
		tls_versions = dns_transport_get_tls_versions(xfr->transport);
		if (tls_versions != 0) {
			isc_tlsctx_set_protocols(tlsctx, tls_versions);
		}
		ciphers = dns_transport_get_ciphers(xfr->transport);
		if (ciphers != NULL) {
			isc_tlsctx_set_cipherlist(tlsctx, ciphers);
		}

		if (dns_transport_get_prefer_server_ciphers(
			    xfr->transport, &prefer_server_ciphers))
		{
			isc_tlsctx_prefer_server_ciphers(tlsctx,
							 prefer_server_ciphers);
		}

		if (hostname != NULL || ca_file != NULL) {
			/*
			 * The situation when 'found_store != NULL' while 'found
			 * == NULL' might appear as there is one to many
			 * relation between per transport TLS contexts and cert
			 * stores. That is, there could be one store shared
			 * between multiple contexts.
			 */
			if (found_store == NULL) {
				/*
				 * 'ca_file' can equal 'NULL' here, in
				 * that case the store with system-wide
				 * CA certificates will be created, just
				 * as planned.
				 */
				result = isc_tls_cert_store_create(ca_file,
								   &store);

				if (result != ISC_R_SUCCESS) {
					goto failure;
				}
			} else {
				store = found_store;
			}

			INSIST(store != NULL);
			if (hostname == NULL) {
				/*
				 * If CA bundle file is specified, but
				 * hostname is not, then use the primary
				 * IP address for validation, just like
				 * dig does.
				 */
				INSIST(ca_file != NULL);
				isc_netaddr_fromsockaddr(&primary_netaddr,
							 &xfr->primaryaddr);
				isc_netaddr_format(&primary_netaddr,
						   primary_addr_str,
						   sizeof(primary_addr_str));
				hostname = primary_addr_str;
			}
			/*
			 * According to RFC 8310, Subject field MUST NOT
			 * be inspected when verifying hostname for DoT.
			 * Only SubjectAltName must be checked.
			 */
			hostname_ignore_subject = true;
			result = isc_tlsctx_enable_peer_verification(
				tlsctx, false, store, hostname,
				hostname_ignore_subject);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}

			/*
			 * Let's load client certificate and enable
			 * Mutual TLS. We do that only in the case when
			 * Strict TLS is enabled, because Mutual TLS is
			 * an extension of it.
			 */
			if (cert_file != NULL) {
				INSIST(key_file != NULL);

				result = isc_tlsctx_load_certificate(
					tlsctx, key_file, cert_file);
				if (result != ISC_R_SUCCESS) {
					goto failure;
				}
			}
		}

		isc_tlsctx_enable_dot_client_alpn(tlsctx);

		isc_tlsctx_client_session_cache_create(
			xfr->mctx, tlsctx,
			ISC_TLSCTX_CLIENT_SESSION_CACHE_DEFAULT_SIZE,
			&sess_cache);

		found_store = NULL;
		result = isc_tlsctx_cache_add(xfr->tlsctx_cache, tlsname,
					      isc_tlsctx_cache_tls, family,
					      tlsctx, store, sess_cache, &found,
					      &found_store, &found_sess_cache);
		if (result == ISC_R_EXISTS) {
			/*
			 * It seems the entry has just been created from within
			 * another thread while we were initialising
			 * ours. Although this is unlikely, it could happen
			 * after startup/re-initialisation. In such a case,
			 * discard the new context and associated data and use
			 * the already established one from now on.
			 *
			 * Such situation will not occur after the
			 * initial 'warm-up', so it is not critical
			 * performance-wise.
			 */
			INSIST(found != NULL);
			isc_tlsctx_free(&tlsctx);
			/*
			 * The 'store' variable can be 'NULL' when remote server
			 * verification is not enabled (that is, when Strict or
			 * Mutual TLS are not used).
			 *
			 * The 'found_store' might be equal to 'store' as there
			 * is one-to-many relation between a store and
			 * per-transport TLS contexts. In that case, the call to
			 * 'isc_tlsctx_cache_find()' above could have returned a
			 * store via the 'found_store' variable, whose value we
			 * can assign to 'store' later. In that case,
			 * 'isc_tlsctx_cache_add()' will return the same value.
			 * When that happens, we should not free the store
			 * object, as it is managed by the TLS context cache.
			 */
			if (store != NULL && store != found_store) {
				isc_tls_cert_store_free(&store);
			}
			isc_tlsctx_client_session_cache_detach(&sess_cache);
			/* Let's return the data from the cache. */
			*psess_cache = found_sess_cache;
			*pctx = found;
		} else {
			/*
			 * Adding the fresh values into the cache has been
			 * successful, let's return them
			 */
			INSIST(result == ISC_R_SUCCESS);
			*psess_cache = sess_cache;
			*pctx = tlsctx;
		}
	} else {
		/*
		 * The cache lookup has been successful, let's return the
		 * results.
		 */
		INSIST(result == ISC_R_SUCCESS);
		*psess_cache = found_sess_cache;
		*pctx = found;
	}

	return (ISC_R_SUCCESS);

failure:
	if (tlsctx != NULL) {
		isc_tlsctx_free(&tlsctx);
	}

	/*
	 * The 'found_store' is being managed by the TLS context
	 * cache. Thus, we should keep it as it is, as it will get
	 * destroyed alongside the cache. As there is one store per
	 * multiple TLS contexts, we need to handle store deletion in a
	 * special way.
	 */
	if (store != NULL && store != found_store) {
		isc_tls_cert_store_free(&store);
	}

	return (result);
}

static isc_result_t
xfrin_start(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;
	dns_xfrin_ctx_t *connect_xfr = NULL;
	dns_transport_type_t transport_type = DNS_TRANSPORT_TCP;
	isc_tlsctx_t *tlsctx = NULL;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL;
	isc_interval_t interval;
	isc_time_t next;

	(void)isc_refcount_increment0(&xfr->connects);
	dns_xfrin_attach(xfr, &connect_xfr);

	if (xfr->transport != NULL) {
		transport_type = dns_transport_get_type(xfr->transport);
	}

	/* Set the maximum timer */
	isc_interval_set(&interval, dns_zone_getmaxxfrin(xfr->zone), 0);
	isc_time_nowplusinterval(&next, &interval);
	result = isc_timer_reset(xfr->max_time_timer, isc_timertype_once, &next,
				 NULL, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/* Set the idle timer */
	isc_interval_set(&interval, dns_zone_getidlein(xfr->zone), 0);
	isc_time_nowplusinterval(&next, &interval);
	result = isc_timer_reset(xfr->max_idle_timer, isc_timertype_once, &next,
				 NULL, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/*
	 * XXX: timeouts are hard-coded to 30 seconds; this needs to be
	 * configurable.
	 */
	switch (transport_type) {
	case DNS_TRANSPORT_TCP:
		isc_nm_tcpdnsconnect(xfr->netmgr, &xfr->sourceaddr,
				     &xfr->primaryaddr, xfrin_connect_done,
				     connect_xfr, 30000, 0);
		break;
	case DNS_TRANSPORT_TLS: {
		result = get_create_tlsctx(xfr, &tlsctx, &sess_cache);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
		INSIST(tlsctx != NULL);
		isc_nm_tlsdnsconnect(xfr->netmgr, &xfr->sourceaddr,
				     &xfr->primaryaddr, xfrin_connect_done,
				     connect_xfr, 30000, 0, tlsctx, sess_cache);
	} break;
	default:
		UNREACHABLE();
	}

	return (ISC_R_SUCCESS);

failure:
	isc_refcount_decrement0(&xfr->connects);
	dns_xfrin_detach(&connect_xfr);
	return (result);
}

/* XXX the resolver could use this, too */

static isc_result_t
render(dns_message_t *msg, isc_mem_t *mctx, isc_buffer_t *buf) {
	dns_compress_t cctx;
	bool cleanup_cctx = false;
	isc_result_t result;

	CHECK(dns_compress_init(&cctx, -1, mctx));
	cleanup_cctx = true;
	CHECK(dns_message_renderbegin(msg, &cctx, buf));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_QUESTION, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_ANSWER, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_AUTHORITY, 0));
	CHECK(dns_message_rendersection(msg, DNS_SECTION_ADDITIONAL, 0));
	CHECK(dns_message_renderend(msg));
	result = ISC_R_SUCCESS;
failure:
	if (cleanup_cctx) {
		dns_compress_invalidate(&cctx);
	}
	return (result);
}

/*
 * A connection has been established.
 */
static void
xfrin_connect_done(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	dns_xfrin_ctx_t *xfr = (dns_xfrin_ctx_t *)cbarg;
	char sourcetext[ISC_SOCKADDR_FORMATSIZE];
	char signerbuf[DNS_NAME_FORMATSIZE];
	const char *signer = "", *sep = "";
	isc_sockaddr_t sockaddr;
	dns_zonemgr_t *zmgr = NULL;

	REQUIRE(VALID_XFRIN(xfr));

	isc_refcount_decrement0(&xfr->connects);

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed to connect");
		goto failure;
	}

	result = isc_nm_xfr_checkperm(handle);
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "connected but unable to transfer");
		goto failure;
	}

	zmgr = dns_zone_getmgr(xfr->zone);
	if (zmgr != NULL) {
		dns_zonemgr_unreachabledel(zmgr, &xfr->primaryaddr,
					   &xfr->sourceaddr);
	}

	xfr->handle = handle;
	sockaddr = isc_nmhandle_peeraddr(handle);
	isc_sockaddr_format(&sockaddr, sourcetext, sizeof(sourcetext));

	if (xfr->tsigkey != NULL && xfr->tsigkey->key != NULL) {
		dns_name_format(dst_key_name(xfr->tsigkey->key), signerbuf,
				sizeof(signerbuf));
		sep = " TSIG ";
		signer = signerbuf;
	}

	xfrin_log(xfr, ISC_LOG_INFO, "connected using %s%s%s", sourcetext, sep,
		  signer);

	result = xfrin_send_request(xfr);
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "connected but unable to send");
	}

failure:
	switch (result) {
	case ISC_R_SUCCESS:
		break;
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
			isc_time_t now;

			TIME_NOW(&now);

			dns_zonemgr_unreachableadd(zmgr, &xfr->primaryaddr,
						   &xfr->sourceaddr, &now);
		}
		break;
	default:
		/* Retry sooner than in 10 minutes */
		break;
	}

	dns_xfrin_detach(&xfr);
}

/*
 * Convert a tuple into a dns_name_t suitable for inserting
 * into the given dns_message_t.
 */
static isc_result_t
tuple2msgname(dns_difftuple_t *tuple, dns_message_t *msg, dns_name_t **target) {
	isc_result_t result;
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *rdl = NULL;
	dns_rdataset_t *rds = NULL;
	dns_name_t *name = NULL;

	REQUIRE(target != NULL && *target == NULL);

	CHECK(dns_message_gettemprdata(msg, &rdata));
	dns_rdata_init(rdata);
	dns_rdata_clone(&tuple->rdata, rdata);

	CHECK(dns_message_gettemprdatalist(msg, &rdl));
	dns_rdatalist_init(rdl);
	rdl->type = tuple->rdata.type;
	rdl->rdclass = tuple->rdata.rdclass;
	rdl->ttl = tuple->ttl;
	ISC_LIST_APPEND(rdl->rdata, rdata, link);

	CHECK(dns_message_gettemprdataset(msg, &rds));
	CHECK(dns_rdatalist_tordataset(rdl, rds));

	CHECK(dns_message_gettempname(msg, &name));
	dns_name_clone(&tuple->name, name);
	ISC_LIST_APPEND(name->list, rds, link);

	*target = name;
	return (ISC_R_SUCCESS);

failure:

	if (rds != NULL) {
		dns_rdataset_disassociate(rds);
		dns_message_puttemprdataset(msg, &rds);
	}
	if (rdl != NULL) {
		ISC_LIST_UNLINK(rdl->rdata, rdata, link);
		dns_message_puttemprdatalist(msg, &rdl);
	}
	if (rdata != NULL) {
		dns_message_puttemprdata(msg, &rdata);
	}

	return (result);
}

/*
 * Build an *XFR request and send its length prefix.
 */
static isc_result_t
xfrin_send_request(dns_xfrin_ctx_t *xfr) {
	isc_result_t result;
	isc_region_t region;
	dns_rdataset_t *qrdataset = NULL;
	dns_message_t *msg = NULL;
	dns_difftuple_t *soatuple = NULL;
	dns_name_t *qname = NULL;
	dns_dbversion_t *ver = NULL;
	dns_name_t *msgsoaname = NULL;
	dns_xfrin_ctx_t *send_xfr = NULL;

	/* Create the request message */
	dns_message_create(xfr->mctx, DNS_MESSAGE_INTENTRENDER, &msg);
	CHECK(dns_message_settsigkey(msg, xfr->tsigkey));

	/* Create a name for the question section. */
	CHECK(dns_message_gettempname(msg, &qname));
	dns_name_clone(&xfr->name, qname);

	/* Formulate the question and attach it to the question name. */
	CHECK(dns_message_gettemprdataset(msg, &qrdataset));
	dns_rdataset_makequestion(qrdataset, xfr->rdclass, xfr->reqtype);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	qrdataset = NULL;

	dns_message_addname(msg, qname, DNS_SECTION_QUESTION);
	qname = NULL;

	if (xfr->reqtype == dns_rdatatype_ixfr) {
		/* Get the SOA and add it to the authority section. */
		/* XXX is using the current version the right thing? */
		dns_db_currentversion(xfr->db, &ver);
		CHECK(dns_db_createsoatuple(xfr->db, ver, xfr->mctx,
					    DNS_DIFFOP_EXISTS, &soatuple));
		xfr->ixfr.request_serial = dns_soa_getserial(&soatuple->rdata);
		xfr->ixfr.current_serial = xfr->ixfr.request_serial;
		xfrin_log(xfr, ISC_LOG_DEBUG(3),
			  "requesting IXFR for serial %u",
			  xfr->ixfr.request_serial);

		CHECK(tuple2msgname(soatuple, msg, &msgsoaname));
		dns_message_addname(msg, msgsoaname, DNS_SECTION_AUTHORITY);
	} else if (xfr->reqtype == dns_rdatatype_soa) {
		CHECK(dns_db_getsoaserial(xfr->db, NULL,
					  &xfr->ixfr.request_serial));
	}

	xfr->id++;
	xfr->nmsg = 0;
	xfr->nrecs = 0;
	xfr->nbytes = 0;
	isc_time_now(&xfr->start);
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

	dns_xfrin_attach(xfr, &send_xfr);
	isc_nmhandle_attach(send_xfr->handle, &xfr->sendhandle);
	isc_refcount_increment0(&send_xfr->sends);
	isc_nm_send(xfr->handle, &region, xfrin_send_done, send_xfr);

failure:
	if (qname != NULL) {
		dns_message_puttempname(msg, &qname);
	}
	if (qrdataset != NULL) {
		dns_message_puttemprdataset(msg, &qrdataset);
	}
	if (msg != NULL) {
		dns_message_detach(&msg);
	}
	if (soatuple != NULL) {
		dns_difftuple_free(&soatuple);
	}
	if (ver != NULL) {
		dns_db_closeversion(xfr->db, &ver, false);
	}

	return (result);
}

static void
xfrin_send_done(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	dns_xfrin_ctx_t *xfr = (dns_xfrin_ctx_t *)cbarg;
	dns_xfrin_ctx_t *recv_xfr = NULL;

	REQUIRE(VALID_XFRIN(xfr));

	isc_refcount_decrement0(&xfr->sends);
	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	CHECK(result);

	xfrin_log(xfr, ISC_LOG_DEBUG(3), "sent request data");

	dns_xfrin_attach(xfr, &recv_xfr);
	isc_nmhandle_attach(handle, &recv_xfr->readhandle);
	isc_refcount_increment0(&recv_xfr->recvs);
	isc_nm_read(recv_xfr->handle, xfrin_recv_done, recv_xfr);

failure:
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed sending request data");
	}

	isc_nmhandle_detach(&xfr->sendhandle);
	dns_xfrin_detach(&xfr); /* send_xfr */
}

static void
xfrin_recv_done(isc_nmhandle_t *handle, isc_result_t result,
		isc_region_t *region, void *cbarg) {
	dns_xfrin_ctx_t *xfr = (dns_xfrin_ctx_t *)cbarg;
	dns_message_t *msg = NULL;
	dns_name_t *name = NULL;
	const dns_name_t *tsigowner = NULL;
	isc_buffer_t buffer;
	isc_sockaddr_t peer;

	REQUIRE(VALID_XFRIN(xfr));

	isc_refcount_decrement0(&xfr->recvs);

	if (atomic_load(&xfr->shuttingdown)) {
		result = ISC_R_SHUTTINGDOWN;
	}

	/* Stop the idle timer */
	(void)isc_timer_reset(xfr->max_idle_timer, isc_timertype_inactive, NULL,
			      NULL, true);

	CHECK(result);

	xfrin_log(xfr, ISC_LOG_DEBUG(7), "received %u bytes", region->length);

	dns_message_create(xfr->mctx, DNS_MESSAGE_INTENTPARSE, &msg);

	CHECK(dns_message_settsigkey(msg, xfr->tsigkey));
	CHECK(dns_message_setquerytsig(msg, xfr->lasttsig));

	msg->tsigctx = xfr->tsigctx;
	xfr->tsigctx = NULL;

	dns_message_setclass(msg, xfr->rdclass);

	if (xfr->nmsg > 0) {
		msg->tcp_continuation = 1;
	}

	isc_buffer_init(&buffer, region->base, region->length);
	isc_buffer_add(&buffer, region->length);
	peer = isc_nmhandle_peeraddr(handle);

	result = dns_message_parse(msg, &buffer,
				   DNS_MESSAGEPARSE_PRESERVEORDER);
	if (result == ISC_R_SUCCESS) {
		dns_message_logpacket(msg, "received message from", &peer,
				      DNS_LOGCATEGORY_XFER_IN,
				      DNS_LOGMODULE_XFER_IN, ISC_LOG_DEBUG(10),
				      xfr->mctx);
	} else {
		xfrin_log(xfr, ISC_LOG_DEBUG(10), "dns_message_parse: %s",
			  isc_result_totext(result));
	}

	if (result != ISC_R_SUCCESS || msg->rcode != dns_rcode_noerror ||
	    msg->opcode != dns_opcode_query || msg->rdclass != xfr->rdclass ||
	    msg->id != xfr->id)
	{
		if (result == ISC_R_SUCCESS && msg->rcode != dns_rcode_noerror)
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
		isc_nmhandle_detach(&xfr->readhandle);
		dns_message_detach(&msg);
		xfrin_reset(xfr);
		xfr->reqtype = dns_rdatatype_soa;
		xfr->state = XFRST_SOAQUERY;
		result = xfrin_start(xfr);
		if (result != ISC_R_SUCCESS) {
			xfrin_fail(xfr, result, "failed setting up socket");
		}
		dns_xfrin_detach(&xfr); /* recv_xfr */
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

	if ((xfr->state == XFRST_SOAQUERY || xfr->state == XFRST_INITIALSOA) &&
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

		name = NULL;
		dns_message_currentname(msg, DNS_SECTION_QUESTION, &name);
		if (!dns_name_equal(name, &xfr->name)) {
			result = DNS_R_FORMERR;
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question name mismatch");
			goto failure;
		}
		rds = ISC_LIST_HEAD(name->list);
		INSIST(rds != NULL);
		if (rds->type != xfr->reqtype) {
			result = DNS_R_FORMERR;
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question type mismatch");
			goto failure;
		}
		if (rds->rdclass != xfr->rdclass) {
			result = DNS_R_FORMERR;
			xfrin_log(xfr, ISC_LOG_NOTICE,
				  "question class mismatch");
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
	    xfr->state == XFRST_INITIALSOA &&
	    msg->counts[DNS_SECTION_ANSWER] == 0)
	{
		xfrin_log(xfr, ISC_LOG_DEBUG(3),
			  "empty answer section, retrying with AXFR");
		goto try_axfr;
	}

	if (xfr->reqtype == dns_rdatatype_soa &&
	    (msg->flags & DNS_MESSAGEFLAG_AA) == 0)
	{
		FAIL(DNS_R_NOTAUTHORITATIVE);
	}

	result = dns_message_checksig(msg, dns_zone_getview(xfr->zone));
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
	if (result != ISC_R_NOMORE) {
		goto failure;
	}

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
		if (xfr->sincetsig > 100 || xfr->nmsg == 0 ||
		    xfr->state == XFRST_AXFR_END ||
		    xfr->state == XFRST_IXFR_END)
		{
			result = DNS_R_EXPECTEDTSIG;
			goto failure;
		}
	}

	/*
	 * Update the number of messages received.
	 */
	xfr->nmsg++;

	/*
	 * Update the number of bytes received.
	 */
	xfr->nbytes += buffer.used;

	/*
	 * Take the context back.
	 */
	INSIST(xfr->tsigctx == NULL);
	xfr->tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;

	switch (xfr->state) {
	case XFRST_GOTSOA:
		xfr->reqtype = dns_rdatatype_axfr;
		xfr->state = XFRST_INITIALSOA;
		CHECK(xfrin_send_request(xfr));
		break;
	case XFRST_AXFR_END:
		CHECK(axfr_finalize(xfr));
		FALLTHROUGH;
	case XFRST_IXFR_END:
		/*
		 * Close the journal.
		 */
		if (xfr->ixfr.journal != NULL) {
			dns_journal_destroy(&xfr->ixfr.journal);
		}

		/*
		 * Inform the caller we succeeded.
		 */
		if (xfr->done != NULL) {
			(xfr->done)(xfr->zone, ISC_R_SUCCESS);
			xfr->done = NULL;
		}

		atomic_store(&xfr->shuttingdown, true);
		(void)isc_timer_reset(xfr->max_time_timer,
				      isc_timertype_inactive, NULL, NULL, true);
		xfr->shutdown_result = ISC_R_SUCCESS;
		break;
	default:
		/*
		 * Read the next message.
		 */
		/* The readhandle is still attached */
		/* The recv_xfr is still attached */
		dns_message_detach(&msg);
		isc_refcount_increment0(&xfr->recvs);
		isc_nm_read(xfr->handle, xfrin_recv_done, xfr);
		isc_time_t next;
		isc_interval_t interval;
		isc_interval_set(&interval, dns_zone_getidlein(xfr->zone), 0);
		isc_time_nowplusinterval(&next, &interval);
		result = isc_timer_reset(xfr->max_idle_timer,
					 isc_timertype_once, &next, NULL, true);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		return;
	}

failure:
	if (result != ISC_R_SUCCESS) {
		xfrin_fail(xfr, result, "failed while receiving responses");
	}

	if (msg != NULL) {
		dns_message_detach(&msg);
	}
	isc_nmhandle_detach(&xfr->readhandle);
	dns_xfrin_detach(&xfr); /* recv_xfr */
}

static void
xfrin_destroy(dns_xfrin_ctx_t *xfr) {
	uint64_t msecs;
	uint64_t persec;
	const char *result_str;

	REQUIRE(VALID_XFRIN(xfr));

	/* Safe-guards */
	REQUIRE(atomic_load(&xfr->shuttingdown));
	isc_refcount_destroy(&xfr->references);
	isc_refcount_destroy(&xfr->connects);
	isc_refcount_destroy(&xfr->recvs);
	isc_refcount_destroy(&xfr->sends);

	INSIST(xfr->shutdown_result != ISC_R_UNSET);

	/*
	 * If we're called through dns_xfrin_detach() and are not
	 * shutting down, we can't know what the transfer status is as
	 * we are only called when the last reference is lost.
	 */
	result_str = isc_result_totext(xfr->shutdown_result);
	xfrin_log(xfr, ISC_LOG_INFO, "Transfer status: %s", result_str);

	/*
	 * Calculate the length of time the transfer took,
	 * and print a log message with the bytes and rate.
	 */
	isc_time_now(&xfr->end);
	msecs = isc_time_microdiff(&xfr->end, &xfr->start) / 1000;
	if (msecs == 0) {
		msecs = 1;
	}
	persec = (xfr->nbytes * 1000) / msecs;
	xfrin_log(xfr, ISC_LOG_INFO,
		  "Transfer completed: %d messages, %d records, "
		  "%" PRIu64 " bytes, "
		  "%u.%03u secs (%u bytes/sec) (serial %u)",
		  xfr->nmsg, xfr->nrecs, xfr->nbytes,
		  (unsigned int)(msecs / 1000), (unsigned int)(msecs % 1000),
		  (unsigned int)persec, xfr->end_serial);

	if (xfr->readhandle != NULL) {
		isc_nmhandle_detach(&xfr->readhandle);
	}
	if (xfr->sendhandle != NULL) {
		isc_nmhandle_detach(&xfr->sendhandle);
	}

	if (xfr->transport != NULL) {
		dns_transport_detach(&xfr->transport);
	}

	if (xfr->tsigkey != NULL) {
		dns_tsigkey_detach(&xfr->tsigkey);
	}

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

	if (xfr->tsigctx != NULL) {
		dst_context_destroy(&xfr->tsigctx);
	}

	if ((xfr->name.attributes & DNS_NAMEATTR_DYNAMIC) != 0) {
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

	if (xfr->firstsoa_data != NULL) {
		isc_mem_free(xfr->mctx, xfr->firstsoa_data);
	}

	if (xfr->tlsctx_cache != NULL) {
		isc_tlsctx_cache_detach(&xfr->tlsctx_cache);
	}

	isc_timer_destroy(&xfr->max_idle_timer);
	isc_timer_destroy(&xfr->max_time_timer);

	isc_mem_putanddetach(&xfr->mctx, xfr, sizeof(*xfr));
}

/*
 * Log incoming zone transfer messages in a format like
 * transfer of <zone> from <address>: <message>
 */
static void
xfrin_logv(int level, const char *zonetext, const isc_sockaddr_t *primaryaddr,
	   const char *fmt, va_list ap) {
	char primarytext[ISC_SOCKADDR_FORMATSIZE];
	char msgtext[2048];

	isc_sockaddr_format(primaryaddr, primarytext, sizeof(primarytext));
	vsnprintf(msgtext, sizeof(msgtext), fmt, ap);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_XFER_IN, DNS_LOGMODULE_XFER_IN,
		      level, "transfer of '%s' from %s: %s", zonetext,
		      primarytext, msgtext);
}

/*
 * Logging function for use when a xfrin_ctx_t has not yet been created.
 */

static void
xfrin_log1(int level, const char *zonetext, const isc_sockaddr_t *primaryaddr,
	   const char *fmt, ...) {
	va_list ap;

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	xfrin_logv(level, zonetext, primaryaddr, fmt, ap);
	va_end(ap);
}

/*
 * Logging function for use when there is a xfrin_ctx_t.
 */

static void
xfrin_log(dns_xfrin_ctx_t *xfr, int level, const char *fmt, ...) {
	va_list ap;
	char zonetext[DNS_NAME_MAXTEXT + 32];

	if (!isc_log_wouldlog(dns_lctx, level)) {
		return;
	}

	dns_zone_name(xfr->zone, zonetext, sizeof(zonetext));

	va_start(ap, fmt);
	xfrin_logv(level, zonetext, &xfr->primaryaddr, fmt, ap);
	va_end(ap);
}
