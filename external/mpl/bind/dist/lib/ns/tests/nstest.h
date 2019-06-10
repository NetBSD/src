/*	$NetBSD: nstest.h,v 1.3.2.2 2019/06/10 22:04:50 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/result.h>
#include <dns/zone.h>

#include <ns/interfacemgr.h>
#include <ns/client.h>
#include <ns/hooks.h>

typedef struct ns_test_id {
	const char *description;
	int lineno;
} ns_test_id_t;

#define NS_TEST_ID(desc)	{ .description = desc, .lineno = __LINE__ }

#define CHECK(r) \
	do { \
		result = (r); \
		if (result != ISC_R_SUCCESS) \
			goto cleanup; \
	} while (0)

extern isc_mem_t *mctx;
extern isc_log_t *lctx;
extern isc_taskmgr_t *taskmgr;
extern isc_task_t *maintask;
extern isc_timermgr_t *timermgr;
extern isc_socketmgr_t *socketmgr;
extern dns_zonemgr_t *zonemgr;
extern dns_dispatchmgr_t *dispatchmgr;
extern ns_clientmgr_t *clientmgr;
extern ns_interfacemgr_t *interfacemgr;
extern ns_server_t *sctx;
extern bool app_running;
extern int ncpus;
extern bool debug_mem_record;

isc_result_t
ns_test_begin(FILE *logfile, bool create_managers);

void
ns_test_end(void);

/*%
 * Create a view.  If "with_cache" is set to true, a cache database will
 * also be created and attached to the created view.
 */
isc_result_t
ns_test_makeview(const char *name, bool with_cache,
		 dns_view_t **viewp);

isc_result_t
ns_test_makezone(const char *name, dns_zone_t **zonep, dns_view_t *view,
				  bool keepview);

isc_result_t
ns_test_setupzonemgr(void);

isc_result_t
ns_test_managezone(dns_zone_t *zone);

void
ns_test_releasezone(dns_zone_t *zone);

void
ns_test_closezonemgr(void);

/*%
 * Load data for zone "zonename" from file "filename" and start serving it to
 * clients matching "view".  Only one zone loaded using this function can be
 * served at any given time.
 */
isc_result_t
ns_test_serve_zone(const char *zonename, const char *filename,
		   dns_view_t *view);

/*%
 * Release the zone loaded by ns_test_serve_zone().
 */
void
ns_test_cleanup_zone(void);

void
ns_test_nap(uint32_t usec);

isc_result_t
ns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
	       const char *testfile);

isc_result_t
ns_test_getdata(const char *file, unsigned char *buf,
		size_t bufsiz, size_t *sizep);

isc_result_t
ns_test_getclient(ns_interface_t *ifp0, bool tcp,
		  ns_client_t **clientp);

/*%
 * Structure containing parameters for ns_test_qctx_create().
 */
typedef struct ns_test_qctx_create_params {
	const char *qname;
	dns_rdatatype_t qtype;
	unsigned int qflags;
	bool with_cache;
} ns_test_qctx_create_params_t;

/*%
 * Prepare a query context identical with one that would be prepared if a query
 * with given QNAME, QTYPE and flags was received from a client.  Recursion is
 * assumed to be allowed for this client.  If "with_cache" is set to true,
 * a cache database will be created and associated with the view matching the
 * incoming query.
 */
isc_result_t
ns_test_qctx_create(const ns_test_qctx_create_params_t *params,
		    query_ctx_t **qctxp);

/*%
 * Destroy a query context created by ns_test_qctx_create().
 */
void
ns_test_qctx_destroy(query_ctx_t **qctxp);

/*%
 * A hook callback interrupting execution at given hook's insertion point.
 */
ns_hookresult_t
ns_test_hook_catch_call(void *arg, void *data, isc_result_t *resultp);
