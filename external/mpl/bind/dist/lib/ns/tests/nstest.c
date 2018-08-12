/*	$NetBSD: nstest.c,v 1.2 2018/08/12 13:02:41 christos Exp $	*/

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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/entropy.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/socket.h>
#include <isc/stdio.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/result.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <ns/client.h>
#include <ns/interfacemgr.h>
#include <ns/server.h>

#include "../hooks.h"

#include "nstest.h"

isc_mem_t *mctx = NULL;
isc_entropy_t *ectx = NULL;
isc_log_t *lctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *maintask = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
dns_zonemgr_t *zonemgr = NULL;
dns_dispatchmgr_t *dispatchmgr = NULL;
ns_clientmgr_t *clientmgr = NULL;
ns_interfacemgr_t *interfacemgr = NULL;
ns_server_t *sctx = NULL;
isc_boolean_t app_running = ISC_FALSE;
int ncpus;
isc_boolean_t debug_mem_record = ISC_TRUE;
isc_boolean_t run_managers = ISC_FALSE;

static isc_boolean_t hash_active = ISC_FALSE, dst_active = ISC_FALSE;

static dns_zone_t *served_zone = NULL;

/*
 * Logging categories: this needs to match the list in lib/ns/log.c.
 */
static isc_logcategory_t categories[] = {
		{ "",                0 },
		{ "client",          0 },
		{ "network",         0 },
		{ "update",          0 },
		{ "queries",         0 },
		{ "unmatched",       0 },
		{ "update-security", 0 },
		{ "query-errors",    0 },
		{ NULL,              0 }
};

static isc_result_t
matchview(isc_netaddr_t *srcaddr, isc_netaddr_t *destaddr,
	  dns_message_t *message, dns_aclenv_t *env, dns_ecs_t *ecs,
	  isc_result_t *sigresultp, dns_view_t **viewp)
{
	UNUSED(srcaddr);
	UNUSED(destaddr);
	UNUSED(message);
	UNUSED(env);
	UNUSED(ecs);
	UNUSED(sigresultp);
	UNUSED(viewp);

	return (ISC_R_NOTIMPLEMENTED);
}

/*
 * These need to be shut down from a running task.
 */
isc_boolean_t shutdown_done = ISC_FALSE;
static void
shutdown_managers(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	if (clientmgr != NULL) {
		ns_clientmgr_destroy(&clientmgr);
	}

	if (interfacemgr != NULL) {
		ns_interfacemgr_shutdown(interfacemgr);
		ns_interfacemgr_detach(&interfacemgr);
	}

	if (dispatchmgr != NULL) {
		dns_dispatchmgr_destroy(&dispatchmgr);
	}

	shutdown_done = ISC_TRUE;
	run_managers = ISC_FALSE;

	isc_event_free(&event);
}

static void
cleanup_managers(void) {
	if (app_running)
		isc_app_finish();

	shutdown_done = ISC_FALSE;

	if (maintask != NULL) {
		isc_task_shutdown(maintask);
		isc_task_destroy(&maintask);
	}

	while (run_managers && !shutdown_done) {
#ifndef ISC_PLATFORM_USETHREADS
		while (isc__taskmgr_ready(taskmgr))
			isc__taskmgr_dispatch(taskmgr);
#else
		/*
		 * There's no straightforward way to determine
		 * whether all the clients have shut down, so
		 * we'll just sleep for a bit and hope.
		 */
		ns_test_nap(500000);
#endif
	}

	if (sctx != NULL)
		ns_server_detach(&sctx);
	if (socketmgr != NULL)
		isc_socketmgr_destroy(&socketmgr);
	if (taskmgr != NULL)
		isc_taskmgr_destroy(&taskmgr);
	if (timermgr != NULL)
		isc_timermgr_destroy(&timermgr);
}

static void
scan_interfaces(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	ns_interfacemgr_scan(interfacemgr, ISC_TRUE);
	isc_event_free(&event);
}

static isc_result_t
create_managers(void) {
	isc_result_t result;
	ns_listenlist_t *listenon = NULL;
	isc_event_t *event = NULL;
#ifdef ISC_PLATFORM_USETHREADS
	ncpus = isc_os_ncpus();
#else
	ncpus = 1;
#endif

	CHECK(isc_taskmgr_create(mctx, ncpus, 0, &taskmgr));
	CHECK(isc_task_create(taskmgr, 0, &maintask));
	isc_taskmgr_setexcltask(taskmgr, maintask);
	CHECK(isc_task_onshutdown(maintask, shutdown_managers, NULL));

	CHECK(isc_timermgr_create(mctx, &timermgr));

	CHECK(isc_socketmgr_create(mctx, &socketmgr));

	CHECK(ns_server_create(mctx, ectx, matchview, &sctx));

	CHECK(dns_dispatchmgr_create(mctx, ectx, &dispatchmgr));

	CHECK(ns_interfacemgr_create(mctx, sctx, taskmgr, timermgr,
				     socketmgr, dispatchmgr, maintask,
				     ncpus, NULL, &interfacemgr));

	CHECK(ns_clientmgr_create(mctx, sctx, taskmgr, timermgr,
				  &clientmgr));

	CHECK(ns_listenlist_default(mctx, 5300, -1, ISC_TRUE, &listenon));
	ns_interfacemgr_setlistenon4(interfacemgr, listenon);
	ns_listenlist_detach(&listenon);

	event = isc_event_allocate(mctx, maintask, ISC_TASKEVENT_TEST,
				   scan_interfaces, NULL,
				   sizeof (isc_event_t));
	isc_task_send(maintask, &event);
#ifndef ISC_PLATFORM_USETHREADS
	while (isc__taskmgr_ready(taskmgr))
		isc__taskmgr_dispatch(taskmgr);
#else
	/*
	 * There's no straightforward way to determine
	 * whether the interfaces have been scanned,
	 * we'll just sleep for a bit and hope.
	 */
	ns_test_nap(500000);
#endif

	run_managers = ISC_TRUE;

	return (ISC_R_SUCCESS);

  cleanup:
	cleanup_managers();
	return (result);
}

isc_result_t
ns_test_begin(FILE *logfile, isc_boolean_t start_managers) {
	isc_result_t result;

	if (start_managers)
		CHECK(isc_app_start());
	if (debug_mem_record)
		isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	CHECK(isc_mem_create(0, 0, &mctx));
	CHECK(isc_entropy_create(mctx, &ectx));

	CHECK(dst_lib_init(mctx, ectx, ISC_ENTROPY_BLOCKING));
	dst_active = ISC_TRUE;

	CHECK(isc_hash_create(mctx, ectx, DNS_NAME_MAXWIRE));
	hash_active = ISC_TRUE;

	if (logfile != NULL) {
		isc_logdestination_t destination;
		isc_logconfig_t *logconfig = NULL;

		CHECK(isc_log_create(mctx, &lctx, &logconfig));
		isc_log_registercategories(lctx, categories);
		isc_log_setcontext(lctx);
		dns_log_init(lctx);
		dns_log_setcontext(lctx);

		destination.file.stream = logfile;
		destination.file.name = NULL;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		CHECK(isc_log_createchannel(logconfig, "stderr",
					    ISC_LOG_TOFILEDESC,
					    ISC_LOG_DYNAMIC,
					    &destination, 0));
		CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));
	}

	dns_result_register();

	if (start_managers)
		CHECK(create_managers());

	/*
	 * atf-run changes us to a /tmp directory, so tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	if (chdir(TESTS) == -1)
		CHECK(ISC_R_FAILURE);

	ns__hook_table = NULL;

	return (ISC_R_SUCCESS);

  cleanup:
	ns_test_end();
	return (result);
}

void
ns_test_end(void) {
	if (dst_active) {
		dst_lib_destroy();
		dst_active = ISC_FALSE;
	}

	cleanup_managers();

	if (hash_active) {
		isc_hash_destroy();
		hash_active = ISC_FALSE;
	}

	if (ectx != NULL)
		isc_entropy_detach(&ectx);

	if (lctx != NULL)
		isc_log_destroy(&lctx);

	if (mctx != NULL)
		isc_mem_destroy(&mctx);
}

isc_result_t
ns_test_makeview(const char *name, isc_boolean_t with_cache,
		 dns_view_t **viewp)
{
	dns_cache_t *cache = NULL;
	dns_view_t *view = NULL;
	isc_result_t result;

	CHECK(dns_view_create(mctx, dns_rdataclass_in, name, &view));

	if (with_cache) {
		CHECK(dns_cache_create(mctx, taskmgr, timermgr,
				       dns_rdataclass_in, "rbt", 0, NULL,
				       &cache));
		dns_view_setcache(view, cache);
		/*
		 * Reference count for "cache" is now at 2, so decrement it in
		 * order for the cache to be automatically freed when "view"
		 * gets freed.
		 */
		dns_cache_detach(&cache);
	}

	*viewp = view;

	return (ISC_R_SUCCESS);

 cleanup:
	if (view != NULL)
		dns_view_detach(&view);
	return (result);
}

/*
 * Create a zone with origin 'name', return a pointer to the zone object in
 * 'zonep'.  If 'view' is set, add the zone to that view; otherwise, create
 * a new view for the purpose.
 *
 * If the created view is going to be needed by the caller subsequently,
 * then 'keepview' should be set to true; this will prevent the view
 * from being detached.  In this case, the caller is responsible for
 * detaching the view.
 */
isc_result_t
ns_test_makezone(const char *name, dns_zone_t **zonep, dns_view_t *view,
		 isc_boolean_t keepview)
{
	isc_result_t result;
	dns_zone_t *zone = NULL;
	isc_buffer_t buffer;
	dns_fixedname_t fixorigin;
	dns_name_t *origin;

	if (view == NULL)
		CHECK(dns_view_create(mctx, dns_rdataclass_in, "view", &view));
	else if (!keepview)
		keepview = ISC_TRUE;

	zone = *zonep;
	if (zone == NULL)
		CHECK(dns_zone_create(&zone, mctx));

	isc_buffer_constinit(&buffer, name, strlen(name));
	isc_buffer_add(&buffer, strlen(name));
	origin = dns_fixedname_initname(&fixorigin);
	CHECK(dns_name_fromtext(origin, &buffer, dns_rootname, 0, NULL));
	CHECK(dns_zone_setorigin(zone, origin));
	dns_zone_setview(zone, view);
	dns_zone_settype(zone, dns_zone_master);
	dns_zone_setclass(zone, view->rdclass);
	dns_view_addzone(view, zone);

	if (!keepview)
		dns_view_detach(&view);

	*zonep = zone;

	return (ISC_R_SUCCESS);

  cleanup:
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (view != NULL)
		dns_view_detach(&view);
	return (result);
}

isc_result_t
ns_test_setupzonemgr(void) {
	isc_result_t result;
	REQUIRE(zonemgr == NULL);

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &zonemgr);
	return (result);
}

isc_result_t
ns_test_managezone(dns_zone_t *zone) {
	isc_result_t result;
	REQUIRE(zonemgr != NULL);

	result = dns_zonemgr_setsize(zonemgr, 1);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_zonemgr_managezone(zonemgr, zone);
	return (result);
}

void
ns_test_releasezone(dns_zone_t *zone) {
	REQUIRE(zonemgr != NULL);
	dns_zonemgr_releasezone(zonemgr, zone);
}

void
ns_test_closezonemgr(void) {
	REQUIRE(zonemgr != NULL);

	dns_zonemgr_shutdown(zonemgr);
	dns_zonemgr_detach(&zonemgr);
}

isc_result_t
ns_test_serve_zone(const char *zonename, const char *filename,
		   dns_view_t *view)
{
	isc_result_t result;
	dns_db_t *db = NULL;

	/*
	 * Prepare zone structure for further processing.
	 */
	result = ns_test_makezone(zonename, &served_zone, view, ISC_TRUE);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Start zone manager.
	 */
	result = ns_test_setupzonemgr();
	if (result != ISC_R_SUCCESS) {
		goto free_zone;
	}

	/*
	 * Add the zone to the zone manager.
	 */
	result = ns_test_managezone(served_zone);
	if (result != ISC_R_SUCCESS) {
		goto close_zonemgr;
	}

	view->nocookieudp = 512;

	/*
	 * Set path to the master file for the zone and then load it.
	 */
	dns_zone_setfile(served_zone, filename);
	result = dns_zone_load(served_zone);
	if (result != ISC_R_SUCCESS) {
		goto release_zone;
	}

	/*
	 * The zone should now be loaded; test it.
	 */
	result = dns_zone_getdb(served_zone, &db);
	if (result != ISC_R_SUCCESS) {
		goto release_zone;
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}

	return (ISC_R_SUCCESS);

release_zone:
	ns_test_releasezone(served_zone);
close_zonemgr:
	ns_test_closezonemgr();
free_zone:
	dns_zone_detach(&served_zone);

	return (result);
}

void
ns_test_cleanup_zone(void) {
	ns_test_releasezone(served_zone);
	ns_test_closezonemgr();

	dns_zone_detach(&served_zone);
}

isc_result_t
ns_test_getclient(ns_interface_t *ifp0, isc_boolean_t tcp,
		  ns_client_t **clientp)
{
	isc_result_t result;
	ns_interface_t *ifp = ifp0;

	if (ifp == NULL) {
		ifp = ns__interfacemgr_getif(interfacemgr);
	}
	if (ifp == NULL) {
		return (ISC_R_FAILURE);
	}

	result = ns__clientmgr_getclient(clientmgr, ifp, tcp, clientp);
	return (result);
}

/*%
 * Synthesize a DNS message based on supplied QNAME, QTYPE and flags, then
 * parse it and store the results in client->message.
 */
static isc_result_t
attach_query_msg_to_client(ns_client_t *client, const char *qnamestr,
			   dns_rdatatype_t qtype, unsigned int qflags)
{
	dns_rdataset_t *qrdataset = NULL;
	dns_message_t *message = NULL;
	unsigned char query[65536];
	dns_name_t *qname = NULL;
	isc_buffer_t querybuf;
	dns_compress_t cctx;
	isc_result_t result;
	isc_uint32_t qid;

	REQUIRE(client != NULL);
	REQUIRE(qnamestr != NULL);

	/*
	 * Create a new DNS message holding a query.
	 */
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &message);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Set query ID to a random value.
	 */
	isc_random_get(&qid);
	message->id = (dns_messageid_t)(qid & 0xffff);

	/*
	 * Set query flags as requested by the caller.
	 */
	message->flags = qflags;

	/*
	 * Allocate structures required to construct the query.
	 */
	result = dns_message_gettemprdataset(message, &qrdataset);
	if (result != ISC_R_SUCCESS) {
		goto destroy_message;
	}
	result = dns_message_gettempname(message, &qname);
	if (result != ISC_R_SUCCESS) {
		goto put_rdataset;
	}

	/*
	 * Convert "qnamestr" to a DNS name, create a question rdataset of
	 * class IN and type "qtype", link the two and add the result to the
	 * QUESTION section of the query.
	 */
	result = dns_name_fromstring(qname, qnamestr, 0, mctx);
	if (result != ISC_R_SUCCESS) {
		goto put_name;
	}
	dns_rdataset_makequestion(qrdataset, dns_rdataclass_in, qtype);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	dns_message_addname(message, qname, DNS_SECTION_QUESTION);

	/*
	 * Render the query.
	 */
	dns_compress_init(&cctx, -1, mctx);
	isc_buffer_init(&querybuf, query, sizeof(query));
	result = dns_message_renderbegin(message, &cctx, &querybuf);
	if (result != ISC_R_SUCCESS) {
		goto destroy_message;
	}
	result = dns_message_rendersection(message, DNS_SECTION_QUESTION, 0);
	if (result != ISC_R_SUCCESS) {
		goto destroy_message;
	}
	result = dns_message_renderend(message);
	if (result != ISC_R_SUCCESS) {
		goto destroy_message;
	}
	dns_compress_invalidate(&cctx);

	/*
	 * Destroy the created message as it was rendered into "querybuf" and
	 * the latter is all we are going to need from now on.
	 */
	dns_message_destroy(&message);

	/*
	 * Parse the rendered query, storing results in client->message.
	 */
	isc_buffer_first(&querybuf);
	return (dns_message_parse(client->message, &querybuf, 0));

put_name:
	dns_message_puttempname(message, &qname);
put_rdataset:
	dns_message_puttemprdataset(message, &qrdataset);
destroy_message:
	dns_message_destroy(&message);

	return (result);
}

/*%
 * A hook callback which stores the query context pointed to by "hook_data" at
 * "callback_data".  Causes execution to be interrupted at hook insertion
 * point.
 */
static isc_boolean_t
extract_qctx(void *hook_data, void *callback_data, isc_result_t *resultp) {
	query_ctx_t **qctxp;
	query_ctx_t *qctx;

	REQUIRE(hook_data != NULL);
	REQUIRE(callback_data != NULL);
	REQUIRE(resultp != NULL);

	/*
	 * qctx is a stack variable in lib/ns/query.c.  Its contents need to be
	 * duplicated or otherwise they will become invalidated once the stack
	 * gets unwound.
	 */
	qctx = isc_mem_get(mctx, sizeof(*qctx));
	if (qctx != NULL) {
		memmove(qctx, (query_ctx_t *)hook_data, sizeof(*qctx));
	}

	qctxp = (query_ctx_t **)callback_data;
	/*
	 * If memory allocation failed, the supplied pointer will simply be set
	 * to NULL.  We rely on the user of this hook to react properly.
	 */
	*qctxp = qctx;
	*resultp = ISC_R_UNSET;

	return (ISC_TRUE);
}

/*%
 * Initialize a query context for "client" and store it in "qctxp".
 *
 * Requires:
 *
 * 	\li "client->message" to hold a parsed DNS query.
 */
static isc_result_t
create_qctx_for_client(ns_client_t *client, query_ctx_t **qctxp) {
	ns_hook_t *saved_hook_table;
	ns_hook_t query_hooks[NS_QUERY_HOOKS_COUNT + 1] = {
		[NS_QUERY_SETUP_QCTX_INITIALIZED] = {
			.callback = extract_qctx,
			.callback_data = qctxp,
		},
	};

	REQUIRE(client != NULL);
	REQUIRE(qctxp != NULL);
	REQUIRE(*qctxp == NULL);

	/*
	 * Call ns_query_start() to initialize a query context for given
	 * client, but first hook into query_setup() so that we can just
	 * extract an initialized query context, without kicking off any
	 * further processing.  Make sure we do not overwrite any previously
	 * set hooks.
	 */
	saved_hook_table = ns__hook_table;
	ns__hook_table = query_hooks;
	ns_query_start(client);
	ns__hook_table = saved_hook_table;

	if (*qctxp == NULL) {
		return (ISC_R_NOMEMORY);
	} else {
		return (ISC_R_SUCCESS);
	}
}

isc_result_t
ns_test_qctx_create(const ns_test_qctx_create_params_t *params,
		    query_ctx_t **qctxp)
{
	ns_client_t *client = NULL;
	isc_result_t result;

	REQUIRE(params != NULL);
	REQUIRE(params->qname != NULL);
	REQUIRE(qctxp != NULL);
	REQUIRE(*qctxp == NULL);

	/*
	 * Allocate and initialize a client structure.
	 */
	result = ns_test_getclient(NULL, ISC_FALSE, &client);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	TIME_NOW(&client->tnow);

	/*
	 * Every client needs to belong to a view.
	 */
	result = ns_test_makeview("view", params->with_cache, &client->view);
	if (result != ISC_R_SUCCESS) {
		goto detach_client;
	}

	/*
	 * Synthesize a DNS query using given QNAME, QTYPE and flags, storing
	 * it in client->message.
	 */
	result = attach_query_msg_to_client(client, params->qname,
					    params->qtype, params->qflags);
	if (result != ISC_R_SUCCESS) {
		goto detach_client;
	}

	/*
	 * Allow recursion for the client.  As NS_CLIENTATTR_RA normally gets
	 * set in ns__client_request(), i.e. earlier than the unit tests hook
	 * into the call chain, just set it manually.
	 */
	client->attributes |= NS_CLIENTATTR_RA;

	/*
	 * Create a query context for a client sending the previously
	 * synthesized query.
	 */
	result = create_qctx_for_client(client, qctxp);
	if (result != ISC_R_SUCCESS) {
		goto destroy_query;
	}

	/*
	 * Reference count for "client" is now at 2, so decrement it in order
	 * for it to drop to zero when "qctx" gets destroyed.
	 */
	ns_client_detach(&client);

	return (ISC_R_SUCCESS);

destroy_query:
	dns_message_destroy(&client->message);
detach_client:
	ns_client_detach(&client);

	return (result);
}

void
ns_test_qctx_destroy(query_ctx_t **qctxp) {
	query_ctx_t *qctx;

	REQUIRE(qctxp != NULL);
	REQUIRE(*qctxp != NULL);

	qctx = *qctxp;

	ns_client_detach(&qctx->client);

	if (qctx->zone != NULL) {
		dns_zone_detach(&qctx->zone);
	}
	if (qctx->db != NULL) {
		dns_db_detach(&qctx->db);
	}

	isc_mem_put(mctx, qctx, sizeof(*qctx));
	*qctxp = NULL;
}

isc_boolean_t
ns_test_hook_catch_call(void *hook_data, void *callback_data,
			isc_result_t *resultp)
{
	UNUSED(hook_data);
	UNUSED(callback_data);

	*resultp = ISC_R_UNSET;

	return (ISC_TRUE);
}

/*
 * Sleep for 'usec' microseconds.
 */
void
ns_test_nap(isc_uint32_t usec) {
#ifdef HAVE_NANOSLEEP
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&ts, NULL);
#elif HAVE_USLEEP
	usleep(usec);
#else
	/*
	 * No fractional-second sleep function is available, so we
	 * round up to the nearest second and sleep instead
	 */
	sleep((usec / 1000000) + 1);
#endif
}

isc_result_t
ns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
	       const char *testfile)
{
	isc_result_t		result;
	dns_fixedname_t		fixed;
	dns_name_t		*name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return(result);

	result = dns_db_create(mctx, "rbt", name, dbtype, dns_rdataclass_in,
			       0, NULL, db);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_load(*db, testfile);
	return (result);
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);

	printf("bad input format: %02x\n", c);
	exit(3);
	/* NOTREACHED */
}

isc_result_t
ns_test_getdata(const char *file, unsigned char *buf,
		 size_t bufsiz, size_t *sizep)
{
	isc_result_t result;
	unsigned char *bp;
	char *rp, *wp;
	char s[BUFSIZ];
	size_t len, i;
	FILE *f = NULL;
	int n;

	result = isc_stdio_open(file, "r", &f);
	if (result != ISC_R_SUCCESS)
		return (result);

	bp = buf;
	while (fgets(s, sizeof(s), f) != NULL) {
		rp = s;
		wp = s;
		len = 0;
		while (*rp != '\0') {
			if (*rp == '#')
				break;
			if (*rp != ' ' && *rp != '\t' &&
			    *rp != '\r' && *rp != '\n') {
				*wp++ = *rp;
				len++;
			}
			rp++;
		}
		if (len == 0U)
			continue;
		if (len % 2 != 0U)
			CHECK(ISC_R_UNEXPECTEDEND);
		if (len > bufsiz * 2)
			CHECK(ISC_R_NOSPACE);
		rp = s;
		for (i = 0; i < len; i += 2) {
			n = fromhex(*rp++);
			n *= 16;
			n += fromhex(*rp++);
			*bp++ = n;
		}
	}


	*sizep = bp - buf;

	result = ISC_R_SUCCESS;

 cleanup:
	isc_stdio_close(f);
	return (result);
}
