/*	$NetBSD: sig0_test.c,v 1.7 2022/09/23 12:15:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <isc/app.h>
#include <isc/assertions.h>
#include <isc/commandline.h>
#include <isc/error.h>
#include <isc/log.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/masterdump.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/types.h>

#include <dst/dst.h>
#include <dst/result.h>

#define CHECK(str, x)                                                    \
	{                                                                \
		if ((x) != ISC_R_SUCCESS) {                              \
			printf("%s: %s\n", (str), isc_result_totext(x)); \
			exit(-1);                                        \
		}                                                        \
	}

isc_mutex_t lock;
dst_key_t *key = NULL;
isc_mem_t *mctx = NULL;
unsigned char qdata[1024], rdata[1024];
isc_buffer_t qbuffer, rbuffer;
isc_nm_t *netmgr = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *task1 = NULL;
isc_log_t *lctx = NULL;
isc_logconfig_t *logconfig = NULL;
isc_socket_t *s = NULL;
isc_sockaddr_t address;
char output[10 * 1024];
isc_buffer_t outbuf;
static const dns_master_style_t *style = &dns_master_style_debug;

static void
senddone(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;

	REQUIRE(sevent != NULL);
	REQUIRE(sevent->ev_type == ISC_SOCKEVENT_SENDDONE);
	REQUIRE(task == task1);

	printf("senddone\n");

	isc_event_free(&event);
}

static void
recvdone(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	isc_buffer_t source;
	isc_result_t result;
	dns_message_t *response = NULL;

	REQUIRE(sevent != NULL);
	REQUIRE(sevent->ev_type == ISC_SOCKEVENT_RECVDONE);
	REQUIRE(task == task1);

	printf("recvdone\n");
	if (sevent->result != ISC_R_SUCCESS) {
		printf("failed\n");
		exit(-1);
	}

	isc_buffer_init(&source, sevent->region.base, sevent->region.length);
	isc_buffer_add(&source, sevent->n);

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &response);
	result = dns_message_parse(response, &source, 0);
	CHECK("dns_message_parse", result);

	isc_buffer_init(&outbuf, output, sizeof(output));
	result = dns_message_totext(response, style, 0, &outbuf);
	CHECK("dns_message_totext", result);
	printf("%.*s\n", (int)isc_buffer_usedlength(&outbuf),
	       (char *)isc_buffer_base(&outbuf));

	dns_message_detach(&response);
	isc_event_free(&event);

	isc_app_shutdown();
}

static void
buildquery(void) {
	isc_result_t result;
	dns_rdataset_t *question = NULL;
	dns_name_t *qname = NULL;
	isc_region_t r, inr;
	dns_message_t *query = NULL;
	char nametext[] = "host.example";
	isc_buffer_t namesrc, namedst;
	unsigned char namedata[256];
	isc_sockaddr_t sa;
	dns_compress_t cctx;

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &query);
	result = dns_message_setsig0key(query, key);
	CHECK("dns_message_setsig0key", result);

	result = dns_message_gettemprdataset(query, &question);
	CHECK("dns_message_gettemprdataset", result);
	dns_rdataset_makequestion(question, dns_rdataclass_in, dns_rdatatype_a);
	result = dns_message_gettempname(query, &qname);
	CHECK("dns_message_gettempname", result);
	isc_buffer_init(&namesrc, nametext, strlen(nametext));
	isc_buffer_add(&namesrc, strlen(nametext));
	isc_buffer_init(&namedst, namedata, sizeof(namedata));
	dns_name_init(qname, NULL);
	result = dns_name_fromtext(qname, &namesrc, dns_rootname, 0, &namedst);
	CHECK("dns_name_fromtext", result);
	ISC_LIST_APPEND(qname->list, question, link);
	dns_message_addname(query, qname, DNS_SECTION_QUESTION);

	isc_buffer_init(&qbuffer, qdata, sizeof(qdata));

	result = dns_compress_init(&cctx, -1, mctx);
	CHECK("dns_compress_init", result);
	result = dns_message_renderbegin(query, &cctx, &qbuffer);
	CHECK("dns_message_renderbegin", result);
	result = dns_message_rendersection(query, DNS_SECTION_QUESTION, 0);
	CHECK("dns_message_rendersection(question)", result);
	result = dns_message_rendersection(query, DNS_SECTION_ANSWER, 0);
	CHECK("dns_message_rendersection(answer)", result);
	result = dns_message_rendersection(query, DNS_SECTION_AUTHORITY, 0);
	CHECK("dns_message_rendersection(auth)", result);
	result = dns_message_rendersection(query, DNS_SECTION_ADDITIONAL, 0);
	CHECK("dns_message_rendersection(add)", result);
	result = dns_message_renderend(query);
	CHECK("dns_message_renderend", result);
	dns_compress_invalidate(&cctx);

	isc_buffer_init(&outbuf, output, sizeof(output));
	result = dns_message_totext(query, style, 0, &outbuf);
	CHECK("dns_message_totext", result);
	printf("%.*s\n", (int)isc_buffer_usedlength(&outbuf),
	       (char *)isc_buffer_base(&outbuf));

	isc_buffer_usedregion(&qbuffer, &r);
	isc_sockaddr_any(&sa);
	result = isc_socket_bind(s, &sa, 0);
	CHECK("isc_socket_bind", result);
	result = isc_socket_sendto(s, &r, task1, senddone, NULL, &address,
				   NULL);
	CHECK("isc_socket_sendto", result);

	inr.base = rdata;
	inr.length = sizeof(rdata);
	result = isc_socket_recv(s, &inr, 1, task1, recvdone, NULL);
	CHECK("isc_socket_recv", result);
	dns_message_detach(&query);
}

int
main(int argc, char *argv[]) {
	bool verbose = false;
	isc_socketmgr_t *socketmgr = NULL;
	isc_timermgr_t *timermgr = NULL;
	struct in_addr inaddr;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	isc_buffer_t b;
	int ch;
	isc_result_t result;
	in_port_t port = 53;

	RUNTIME_CHECK(isc_app_start() == ISC_R_SUCCESS);

	isc_mutex_init(&lock);

	mctx = NULL;
	isc_mem_create(&mctx);

	while ((ch = isc_commandline_parse(argc, argv, "vp:")) != -1) {
		switch (ch) {
		case 'v':
			verbose = true;
			break;
		case 'p':
			port = (unsigned int)atoi(isc_commandline_argument);
			break;
		}
	}

	RUNTIME_CHECK(dst_lib_init(mctx, NULL) == ISC_R_SUCCESS);

	dns_result_register();
	dst_result_register();

	RUNTIME_CHECK(isc_managers_create(mctx, 2, 0, &netmgr, &taskmgr) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &task1) == ISC_R_SUCCESS);

	RUNTIME_CHECK(isc_timermgr_create(mctx, &timermgr) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_socketmgr_create(mctx, &socketmgr) == ISC_R_SUCCESS);

	isc_log_create(mctx, &lctx, &logconfig);

	RUNTIME_CHECK(isc_socket_create(socketmgr, PF_INET, isc_sockettype_udp,
					&s) == ISC_R_SUCCESS);

	inaddr.s_addr = htonl(INADDR_LOOPBACK);
	isc_sockaddr_fromin(&address, &inaddr, port);

	name = dns_fixedname_initname(&fname);
	isc_buffer_constinit(&b, "child.example.", strlen("child.example."));
	isc_buffer_add(&b, strlen("child.example."));
	result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
	CHECK("dns_name_fromtext", result);

	result = dst_key_fromfile(name, 33180, DNS_KEYALG_RSASHA1,
				  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, NULL,
				  mctx, &key);
	CHECK("dst_key_fromfile", result);

	buildquery();

	(void)isc_app_run();

	isc_task_shutdown(task1);
	isc_task_detach(&task1);
	isc_managers_destroy(&netmgr, &taskmgr);

	isc_socket_detach(&s);
	isc_socketmgr_destroy(&socketmgr);
	isc_timermgr_destroy(&timermgr);

	dst_key_free(&key);

	dst_lib_destroy();

	isc_log_destroy(&lctx);

	if (verbose) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	isc_mutex_destroy(&lock);

	isc_app_finish();

	return (0);
}
