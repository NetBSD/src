/*	$NetBSD: keydelete.c,v 1.1.1.6 2022/09/23 12:09:13 christos Exp $	*/

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

#include <stdlib.h>
#include <string.h>

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/sockaddr.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/request.h>
#include <dns/result.h>
#include <dns/tkey.h>
#include <dns/tsig.h>
#include <dns/view.h>

#include <dst/result.h>

#define CHECK(str, x)                                        \
	{                                                    \
		if ((x) != ISC_R_SUCCESS) {                  \
			fprintf(stderr, "I:%s: %s\n", (str), \
				isc_result_totext(x));       \
			exit(-1);                            \
		}                                            \
	}

#define RUNCHECK(x) RUNTIME_CHECK((x) == ISC_R_SUCCESS)

#define PORT	5300
#define TIMEOUT 30

static isc_mem_t *mctx = NULL;
static dns_tsigkey_t *tsigkey = NULL;
static dns_tsig_keyring_t *ring = NULL;
static dns_requestmgr_t *requestmgr = NULL;

static void
recvquery(isc_task_t *task, isc_event_t *event) {
	dns_requestevent_t *reqev = (dns_requestevent_t *)event;
	isc_result_t result;
	dns_message_t *query = NULL, *response = NULL;

	UNUSED(task);

	REQUIRE(reqev != NULL);

	if (reqev->result != ISC_R_SUCCESS) {
		fprintf(stderr, "I:request event result: %s\n",
			isc_result_totext(reqev->result));
		exit(-1);
	}

	query = reqev->ev_arg;

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &response);

	result = dns_request_getresponse(reqev->request, response,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	CHECK("dns_request_getresponse", result);

	if (response->rcode != dns_rcode_noerror) {
		result = ISC_RESULTCLASS_DNSRCODE + response->rcode;
		fprintf(stderr, "I:response rcode: %s\n",
			isc_result_totext(result));
		exit(-1);
	}

	result = dns_tkey_processdeleteresponse(query, response, ring);
	CHECK("dns_tkey_processdhresponse", result);

	dns_message_detach(&query);
	dns_message_detach(&response);
	dns_request_destroy(&reqev->request);
	isc_event_free(&event);
	isc_app_shutdown();
	return;
}

static void
sendquery(isc_task_t *task, isc_event_t *event) {
	struct in_addr inaddr;
	isc_sockaddr_t address;
	isc_result_t result;
	dns_message_t *query = NULL;
	dns_request_t *request = NULL;

	isc_event_free(&event);

	result = ISC_R_FAILURE;
	if (inet_pton(AF_INET, "10.53.0.1", &inaddr) != 1) {
		CHECK("inet_pton", result);
	}
	isc_sockaddr_fromin(&address, &inaddr, PORT);

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &query);

	result = dns_tkey_builddeletequery(query, tsigkey);
	CHECK("dns_tkey_builddeletequery", result);

	result = dns_request_create(requestmgr, query, &address,
				    DNS_REQUESTOPT_TCP, tsigkey, TIMEOUT, task,
				    recvquery, query, &request);
	CHECK("dns_request_create", result);
}

int
main(int argc, char **argv) {
	char *keyname = NULL;
	isc_nm_t *netmgr = NULL;
	isc_taskmgr_t *taskmgr = NULL;
	isc_timermgr_t *timermgr = NULL;
	isc_socketmgr_t *socketmgr = NULL;
	isc_socket_t *sock = NULL;
	unsigned int attrs, attrmask;
	isc_sockaddr_t bind_any;
	dns_dispatchmgr_t *dispatchmgr = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_view_t *view = NULL;
	dns_tkeyctx_t *tctx = NULL;
	dst_key_t *dstkey = NULL;
	isc_log_t *log = NULL;
	isc_logconfig_t *logconfig = NULL;
	isc_task_t *task = NULL;
	isc_result_t result;
	int type;

	RUNCHECK(isc_app_start());

	if (argc < 2) {
		fprintf(stderr, "I:no key to delete\n");
		exit(-1);
	}
	if (strcmp(argv[1], "-r") == 0) {
		fprintf(stderr, "I:The -r options has been deprecated\n");
		exit(-1);
	}
	keyname = argv[1];

	dns_result_register();

	isc_mem_create(&mctx);

	isc_log_create(mctx, &log, &logconfig);

	RUNCHECK(dst_lib_init(mctx, NULL));

	RUNCHECK(isc_managers_create(mctx, 1, 0, &netmgr, &taskmgr));
	RUNCHECK(isc_task_create(taskmgr, 0, &task));
	RUNCHECK(isc_timermgr_create(mctx, &timermgr));
	RUNCHECK(isc_socketmgr_create(mctx, &socketmgr));
	RUNCHECK(dns_dispatchmgr_create(mctx, &dispatchmgr));
	isc_sockaddr_any(&bind_any);
	attrs = DNS_DISPATCHATTR_UDP | DNS_DISPATCHATTR_MAKEQUERY |
		DNS_DISPATCHATTR_IPV4;
	attrmask = DNS_DISPATCHATTR_UDP | DNS_DISPATCHATTR_TCP |
		   DNS_DISPATCHATTR_IPV4 | DNS_DISPATCHATTR_IPV6;
	RUNCHECK(dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr, &bind_any,
				     4096, 4, 2, 3, 5, attrs, attrmask,
				     &dispatchv4));
	RUNCHECK(dns_requestmgr_create(mctx, timermgr, socketmgr, taskmgr,
				       dispatchmgr, dispatchv4, NULL,
				       &requestmgr));

	RUNCHECK(dns_tsigkeyring_create(mctx, &ring));
	RUNCHECK(dns_tkeyctx_create(mctx, &tctx));

	RUNCHECK(dns_view_create(mctx, 0, "_test", &view));
	dns_view_setkeyring(view, ring);

	RUNCHECK(isc_socket_create(socketmgr, PF_INET, isc_sockettype_udp,
				   &sock));

	RUNCHECK(isc_app_onrun(mctx, task, sendquery, NULL));

	type = DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY;
	result = dst_key_fromnamedfile(keyname, NULL, type, mctx, &dstkey);
	CHECK("dst_key_fromnamedfile", result);
	result = dns_tsigkey_createfromkey(dst_key_name(dstkey),
					   DNS_TSIG_HMACMD5_NAME, dstkey, true,
					   NULL, 0, 0, mctx, ring, &tsigkey);
	dst_key_free(&dstkey);
	CHECK("dns_tsigkey_createfromkey", result);

	(void)isc_app_run();

	dns_requestmgr_shutdown(requestmgr);
	dns_requestmgr_detach(&requestmgr);
	dns_dispatch_detach(&dispatchv4);
	dns_dispatchmgr_destroy(&dispatchmgr);
	isc_task_shutdown(task);
	isc_task_detach(&task);
	isc_managers_destroy(&netmgr, &taskmgr);
	isc_socket_detach(&sock);
	isc_socketmgr_destroy(&socketmgr);
	isc_timermgr_destroy(&timermgr);

	dns_tsigkeyring_detach(&ring);

	dns_tsigkey_detach(&tsigkey);

	dns_tkeyctx_destroy(&tctx);

	dns_view_detach(&view);

	isc_log_destroy(&log);

	dst_lib_destroy();

	isc_mem_destroy(&mctx);

	isc_app_finish();

	return (0);
}
