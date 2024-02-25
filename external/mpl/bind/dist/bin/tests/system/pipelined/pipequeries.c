/*	$NetBSD: pipequeries.c,v 1.7.2.1 2024/02/25 15:44:56 martin Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/commandline.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/netmgr.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdataset.h>
#include <dns/request.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/types.h>
#include <dns/view.h>

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
static dns_requestmgr_t *requestmgr = NULL;
static bool have_src = false;
static isc_sockaddr_t srcaddr;
static isc_sockaddr_t dstaddr;
static int onfly;

static void
recvresponse(isc_task_t *task, isc_event_t *event) {
	dns_requestevent_t *reqev = (dns_requestevent_t *)event;
	isc_result_t result;
	dns_message_t *query = NULL;
	dns_message_t *response = NULL;
	isc_buffer_t outbuf;
	char output[1024];

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
		result = dns_result_fromrcode(response->rcode);
		fprintf(stderr, "I:response rcode: %s\n",
			isc_result_totext(result));
		exit(-1);
	}
	if (response->counts[DNS_SECTION_ANSWER] != 1U) {
		fprintf(stderr, "I:response answer count (%u!=1)\n",
			response->counts[DNS_SECTION_ANSWER]);
	}

	isc_buffer_init(&outbuf, output, sizeof(output));
	result = dns_message_sectiontotext(
		response, DNS_SECTION_ANSWER, &dns_master_style_simple,
		DNS_MESSAGETEXTFLAG_NOCOMMENTS, &outbuf);
	CHECK("dns_message_sectiontotext", result);
	printf("%.*s", (int)isc_buffer_usedlength(&outbuf),
	       (char *)isc_buffer_base(&outbuf));
	fflush(stdout);

	dns_message_detach(&query);
	dns_message_detach(&response);
	dns_request_destroy(&reqev->request);
	isc_event_free(&event);

	if (--onfly == 0) {
		isc_app_shutdown();
	}
	return;
}

static isc_result_t
sendquery(isc_task_t *task) {
	dns_request_t *request = NULL;
	dns_message_t *message = NULL;
	dns_name_t *qname = NULL;
	dns_rdataset_t *qrdataset = NULL;
	isc_result_t result;
	dns_fixedname_t queryname;
	isc_buffer_t buf;
	static char host[256];
	int c;

	c = scanf("%255s", host);
	if (c == EOF) {
		return (ISC_R_NOMORE);
	}

	onfly++;

	dns_fixedname_init(&queryname);
	isc_buffer_init(&buf, host, strlen(host));
	isc_buffer_add(&buf, strlen(host));
	result = dns_name_fromtext(dns_fixedname_name(&queryname), &buf,
				   dns_rootname, 0, NULL);
	CHECK("dns_name_fromtext", result);

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &message);

	message->opcode = dns_opcode_query;
	message->flags |= DNS_MESSAGEFLAG_RD;
	message->rdclass = dns_rdataclass_in;
	message->id = (unsigned short)(random() & 0xFFFF);

	result = dns_message_gettempname(message, &qname);
	CHECK("dns_message_gettempname", result);

	result = dns_message_gettemprdataset(message, &qrdataset);
	CHECK("dns_message_gettemprdataset", result);

	dns_name_clone(dns_fixedname_name(&queryname), qname);
	dns_rdataset_makequestion(qrdataset, dns_rdataclass_in,
				  dns_rdatatype_a);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	dns_message_addname(message, qname, DNS_SECTION_QUESTION);

	result = dns_request_create(requestmgr, message,
				    have_src ? &srcaddr : NULL, &dstaddr,
				    DNS_REQUESTOPT_TCP, NULL, TIMEOUT, 0, 0,
				    task, recvresponse, message, &request);
	CHECK("dns_request_create", result);

	return (ISC_R_SUCCESS);
}

static void
sendqueries(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	isc_event_free(&event);

	do {
		result = sendquery(task);
	} while (result == ISC_R_SUCCESS);

	if (onfly == 0) {
		isc_app_shutdown();
	}
	return;
}

int
main(int argc, char *argv[]) {
	isc_sockaddr_t bind_any;
	struct in_addr inaddr;
	isc_result_t result;
	isc_log_t *lctx = NULL;
	isc_logconfig_t *lcfg = NULL;
	isc_nm_t *netmgr = NULL;
	isc_taskmgr_t *taskmgr = NULL;
	isc_task_t *task = NULL;
	dns_dispatchmgr_t *dispatchmgr = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_view_t *view = NULL;
	uint16_t port = PORT;
	int c;

	RUNCHECK(isc_app_start());

	isc_commandline_errprint = false;
	while ((c = isc_commandline_parse(argc, argv, "p:r:")) != -1) {
		switch (c) {
		case 'p':
			result = isc_parse_uint16(&port,
						  isc_commandline_argument, 10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "bad port '%s'\n",
					isc_commandline_argument);
				exit(1);
			}
			break;
		case 'r':
			fprintf(stderr, "The -r option has been deprecated.\n");
			break;
		case '?':
			fprintf(stderr, "%s: invalid argument '%c'", argv[0],
				c);
			break;
		default:
			break;
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;
	POST(argv);

	if (argc > 0) {
		have_src = true;
	}

	isc_sockaddr_any(&bind_any);

	result = ISC_R_FAILURE;
	if (inet_pton(AF_INET, "10.53.0.7", &inaddr) != 1) {
		CHECK("inet_pton", result);
	}
	isc_sockaddr_fromin(&srcaddr, &inaddr, 0);

	result = ISC_R_FAILURE;
	if (inet_pton(AF_INET, "10.53.0.4", &inaddr) != 1) {
		CHECK("inet_pton", result);
	}
	isc_sockaddr_fromin(&dstaddr, &inaddr, port);

	isc_mem_create(&mctx);

	isc_log_create(mctx, &lctx, &lcfg);

	RUNCHECK(dst_lib_init(mctx, NULL));

	isc_managers_create(mctx, 1, 0, &netmgr, &taskmgr, NULL);
	RUNCHECK(isc_task_create(taskmgr, 0, &task));
	RUNCHECK(dns_dispatchmgr_create(mctx, netmgr, &dispatchmgr));

	RUNCHECK(dns_dispatch_createudp(
		dispatchmgr, have_src ? &srcaddr : &bind_any, &dispatchv4));
	RUNCHECK(dns_requestmgr_create(mctx, taskmgr, dispatchmgr, dispatchv4,
				       NULL, &requestmgr));

	RUNCHECK(dns_view_create(mctx, 0, "_test", &view));
	RUNCHECK(isc_app_onrun(mctx, task, sendqueries, NULL));

	(void)isc_app_run();

	dns_view_detach(&view);

	dns_requestmgr_shutdown(requestmgr);
	dns_requestmgr_detach(&requestmgr);

	dns_dispatch_detach(&dispatchv4);
	dns_dispatchmgr_detach(&dispatchmgr);

	isc_task_shutdown(task);
	isc_task_detach(&task);

	isc_managers_destroy(&netmgr, &taskmgr, NULL);

	dst_lib_destroy();

	isc_log_destroy(&lctx);

	isc_mem_destroy(&mctx);

	isc_app_finish();

	return (0);
}
