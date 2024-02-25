/*	$NetBSD: ns.c,v 1.2.2.2 2024/02/25 15:47:54 martin Exp $	*/

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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/resource.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <ns/client.h>
#include <ns/hooks.h>
#include <ns/interfacemgr.h>
#include <ns/server.h>

#include <tests/ns.h>

dns_dispatchmgr_t *dispatchmgr = NULL;
ns_clientmgr_t *clientmgr = NULL;
ns_interfacemgr_t *interfacemgr = NULL;
ns_server_t *sctx = NULL;
bool debug_mem_record = true;

static isc_result_t
matchview(isc_netaddr_t *srcaddr, isc_netaddr_t *destaddr,
	  dns_message_t *message, dns_aclenv_t *env, isc_result_t *sigresultp,
	  dns_view_t **viewp) {
	UNUSED(srcaddr);
	UNUSED(destaddr);
	UNUSED(message);
	UNUSED(env);
	UNUSED(sigresultp);
	UNUSED(viewp);

	return (ISC_R_NOTIMPLEMENTED);
}

int
setup_server(void **state) {
	isc_result_t result;
	ns_listenlist_t *listenon = NULL;
	in_port_t port = 5300 + isc_random8();

	setup_managers(state);

	ns_server_create(mctx, matchview, &sctx);

	result = dns_dispatchmgr_create(mctx, netmgr, &dispatchmgr);
	if (result != ISC_R_SUCCESS) {
		return (-1);
	}

	result = ns_interfacemgr_create(mctx, sctx, taskmgr, timermgr, netmgr,
					dispatchmgr, maintask, NULL, workers,
					false, &interfacemgr);
	if (result != ISC_R_SUCCESS) {
		return (-1);
	}

	result = ns_listenlist_default(mctx, port, true, AF_INET, &listenon);
	if (result != ISC_R_SUCCESS) {
		return (-1);
	}

	ns_interfacemgr_setlistenon4(interfacemgr, listenon);
	ns_listenlist_detach(&listenon);

	clientmgr = ns_interfacemgr_getclientmgr(interfacemgr);

	return (0);
}

int
teardown_server(void **state) {
	if (interfacemgr != NULL) {
		ns_interfacemgr_shutdown(interfacemgr);
		ns_interfacemgr_detach(&interfacemgr);
	}

	if (dispatchmgr != NULL) {
		dns_dispatchmgr_detach(&dispatchmgr);
	}

	if (sctx != NULL) {
		ns_server_detach(&sctx);
	}

	teardown_managers(state);
	return (0);
}

static dns_zone_t *served_zone = NULL;

/*
 * We don't want to use netmgr-based client accounting, we need to emulate it.
 */
atomic_uint_fast32_t client_refs[32];
atomic_uintptr_t client_addrs[32];

void
isc__nmhandle_attach(isc_nmhandle_t *source, isc_nmhandle_t **targetp FLARG) {
	ns_client_t *client = (ns_client_t *)source;
	int i;

	for (i = 0; i < 32; i++) {
		if (atomic_load(&client_addrs[i]) == (uintptr_t)client) {
			break;
		}
	}
	INSIST(i < 32);
	INSIST(atomic_load(&client_refs[i]) > 0);

	atomic_fetch_add(&client_refs[i], 1);

	*targetp = source;
	return;
}

void
isc__nmhandle_detach(isc_nmhandle_t **handlep FLARG) {
	isc_nmhandle_t *handle = *handlep;
	ns_client_t *client = (ns_client_t *)handle;
	int i;

	*handlep = NULL;

	for (i = 0; i < 32; i++) {
		if (atomic_load(&client_addrs[i]) == (uintptr_t)client) {
			break;
		}
	}
	INSIST(i < 32);

	if (atomic_fetch_sub(&client_refs[i], 1) == 1) {
		dns_view_detach(&client->view);
		client->state = 4;
		ns__client_reset_cb(client);
		ns__client_put_cb(client);
		isc_mem_put(mctx, client, sizeof(ns_client_t));
		atomic_store(&client_addrs[i], (uintptr_t)NULL);
	}

	return;
}

isc_result_t
ns_test_serve_zone(const char *zonename, const char *filename,
		   dns_view_t *view) {
	isc_result_t result;
	dns_db_t *db = NULL;

	/*
	 * Prepare zone structure for further processing.
	 */
	result = dns_test_makezone(zonename, &served_zone, view, false);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Start zone manager.
	 */
	result = dns_test_setupzonemgr();
	if (result != ISC_R_SUCCESS) {
		goto free_zone;
	}

	/*
	 * Add the zone to the zone manager.
	 */
	result = dns_test_managezone(served_zone);
	if (result != ISC_R_SUCCESS) {
		goto close_zonemgr;
	}

	view->nocookieudp = 512;

	/*
	 * Set path to the master file for the zone and then load it.
	 */
	dns_zone_setfile(served_zone, filename, dns_masterformat_text,
			 &dns_master_style_default);
	result = dns_zone_load(served_zone, false);
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
	dns_test_releasezone(served_zone);
close_zonemgr:
	dns_test_closezonemgr();
free_zone:
	dns_zone_detach(&served_zone);

	return (result);
}

void
ns_test_cleanup_zone(void) {
	dns_test_releasezone(served_zone);
	dns_test_closezonemgr();

	dns_zone_detach(&served_zone);
}

isc_result_t
ns_test_getclient(ns_interface_t *ifp0, bool tcp, ns_client_t **clientp) {
	isc_result_t result;
	ns_client_t *client = isc_mem_get(mctx, sizeof(ns_client_t));
	int i;

	UNUSED(ifp0);
	UNUSED(tcp);

	result = ns__client_setup(client, clientmgr, true);

	for (i = 0; i < 32; i++) {
		if (atomic_load(&client_addrs[i]) == (uintptr_t)NULL ||
		    atomic_load(&client_addrs[i]) == (uintptr_t)client)
		{
			break;
		}
	}
	REQUIRE(i < 32);

	atomic_store(&client_refs[i], 2);
	atomic_store(&client_addrs[i], (uintptr_t)client);
	client->handle = (isc_nmhandle_t *)client; /* Hack */
	*clientp = client;

	return (result);
}

/*%
 * Synthesize a DNS message based on supplied QNAME, QTYPE and flags, then
 * parse it and store the results in client->message.
 */
static isc_result_t
attach_query_msg_to_client(ns_client_t *client, const char *qnamestr,
			   dns_rdatatype_t qtype, unsigned int qflags) {
	dns_rdataset_t *qrdataset = NULL;
	dns_message_t *message = NULL;
	unsigned char query[65535];
	dns_name_t *qname = NULL;
	isc_buffer_t querybuf;
	dns_compress_t cctx;
	isc_result_t result;

	REQUIRE(client != NULL);
	REQUIRE(qnamestr != NULL);

	/*
	 * Create a new DNS message holding a query.
	 */
	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &message);

	/*
	 * Set query ID to a random value.
	 */
	message->id = isc_random16();

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
	dns_message_detach(&message);

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
	dns_message_detach(&message);

	return (result);
}

/*%
 * A hook action which stores the query context pointed to by "arg" at
 * "data".  Causes execution to be interrupted at hook insertion
 * point.
 */
static ns_hookresult_t
extract_qctx(void *arg, void *data, isc_result_t *resultp) {
	query_ctx_t **qctxp;
	query_ctx_t *qctx;

	REQUIRE(arg != NULL);
	REQUIRE(data != NULL);
	REQUIRE(resultp != NULL);

	/*
	 * qctx is a stack variable in lib/ns/query.c.  Its contents need to be
	 * duplicated or otherwise they will become invalidated once the stack
	 * gets unwound.
	 */
	qctx = isc_mem_get(mctx, sizeof(*qctx));
	if (qctx != NULL) {
		memmove(qctx, (query_ctx_t *)arg, sizeof(*qctx));
	}

	qctxp = (query_ctx_t **)data;
	/*
	 * If memory allocation failed, the supplied pointer will simply be set
	 * to NULL.  We rely on the user of this hook to react properly.
	 */
	*qctxp = qctx;
	*resultp = ISC_R_UNSET;

	return (NS_HOOK_RETURN);
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
	ns_hooktable_t *saved_hook_table = NULL, *query_hooks = NULL;
	const ns_hook_t hook = {
		.action = extract_qctx,
		.action_data = qctxp,
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

	ns_hooktable_create(mctx, &query_hooks);
	ns_hook_add(query_hooks, mctx, NS_QUERY_SETUP, &hook);

	saved_hook_table = ns__hook_table;
	ns__hook_table = query_hooks;

	ns_query_start(client, client->handle);

	ns__hook_table = saved_hook_table;
	ns_hooktable_free(mctx, (void **)&query_hooks);

	isc_nmhandle_detach(&client->reqhandle);

	if (*qctxp == NULL) {
		return (ISC_R_NOMEMORY);
	} else {
		return (ISC_R_SUCCESS);
	}
}

isc_result_t
ns_test_qctx_create(const ns_test_qctx_create_params_t *params,
		    query_ctx_t **qctxp) {
	ns_client_t *client = NULL;
	isc_result_t result;
	isc_nmhandle_t *handle = NULL;

	REQUIRE(params != NULL);
	REQUIRE(params->qname != NULL);
	REQUIRE(qctxp != NULL);
	REQUIRE(*qctxp == NULL);

	/*
	 * Allocate and initialize a client structure.
	 */
	result = ns_test_getclient(NULL, false, &client);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	TIME_NOW(&client->tnow);

	/*
	 * Every client needs to belong to a view.
	 */
	result = dns_test_makeview("view", params->with_cache, &client->view);
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
		goto detach_view;
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
		goto detach_query;
	}

	/*
	 * The reference count for "client" is now at 2, so we need to
	 * decrement it in order for it to drop to zero when "qctx" gets
	 * destroyed.
	 */
	handle = client->handle;
	isc_nmhandle_detach(&handle);

	return (ISC_R_SUCCESS);

detach_query:
	dns_message_detach(&client->message);
detach_view:
	dns_view_detach(&client->view);
detach_client:
	isc_nmhandle_detach(&client->handle);

	return (result);
}

void
ns_test_qctx_destroy(query_ctx_t **qctxp) {
	query_ctx_t *qctx;

	REQUIRE(qctxp != NULL);
	REQUIRE(*qctxp != NULL);

	qctx = *qctxp;
	*qctxp = NULL;

	if (qctx->zone != NULL) {
		dns_zone_detach(&qctx->zone);
	}
	if (qctx->db != NULL) {
		dns_db_detach(&qctx->db);
	}
	if (qctx->client != NULL) {
		isc_nmhandle_detach(&qctx->client->handle);
	}

	isc_mem_put(mctx, qctx, sizeof(*qctx));
}

ns_hookresult_t
ns_test_hook_catch_call(void *arg, void *data, isc_result_t *resultp) {
	UNUSED(arg);
	UNUSED(data);

	*resultp = ISC_R_UNSET;

	return (NS_HOOK_RETURN);
}

isc_result_t
ns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
	       const char *testfile) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_create(mctx, "rbt", name, dbtype, dns_rdataclass_in, 0,
			       NULL, db);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_load(*db, testfile, dns_masterformat_text, 0);
	return (result);
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'F') {
		return (c - 'A' + 10);
	}

	printf("bad input format: %02x\n", c);
	exit(3);
}

isc_result_t
ns_test_getdata(const char *file, unsigned char *buf, size_t bufsiz,
		size_t *sizep) {
	isc_result_t result;
	unsigned char *bp;
	char *rp, *wp;
	char s[BUFSIZ];
	size_t len, i;
	FILE *f = NULL;
	int n;

	result = isc_stdio_open(file, "r", &f);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	bp = buf;
	while (fgets(s, sizeof(s), f) != NULL) {
		rp = s;
		wp = s;
		len = 0;
		while (*rp != '\0') {
			if (*rp == '#') {
				break;
			}
			if (*rp != ' ' && *rp != '\t' && *rp != '\r' &&
			    *rp != '\n')
			{
				*wp++ = *rp;
				len++;
			}
			rp++;
		}
		if (len == 0U) {
			continue;
		}
		if (len % 2 != 0U) {
			CHECK(ISC_R_UNEXPECTEDEND);
		}
		if (len > bufsiz * 2) {
			CHECK(ISC_R_NOSPACE);
		}
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
