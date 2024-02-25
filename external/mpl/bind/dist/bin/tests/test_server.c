/*	$NetBSD: test_server.c,v 1.2.2.2 2024/02/25 15:43:13 martin Exp $	*/

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

#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/netmgr.h>
#include <isc/os.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

typedef enum { UDP, TCP, DOT, HTTPS, HTTP } protocol_t;

static const char *protocols[] = { "udp", "tcp", "dot", "https", "http-plain" };

static isc_mem_t *mctx = NULL;
static isc_nm_t *netmgr = NULL;

static protocol_t protocol;
static in_port_t port;
static isc_netaddr_t netaddr;
static isc_sockaddr_t sockaddr __attribute__((unused));
static int workers;

static isc_tlsctx_t *tls_ctx = NULL;

static void
read_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	void *cbarg);
static void
send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg);

static isc_result_t
parse_port(const char *input) {
	char *endptr = NULL;
	long val = strtol(input, &endptr, 10);

	if ((*endptr != '\0') || (val <= 0) || (val >= 65536)) {
		return (ISC_R_BADNUMBER);
	}

	port = (in_port_t)val;

	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_protocol(const char *input) {
	for (size_t i = 0; i < ARRAY_SIZE(protocols); i++) {
		if (!strcasecmp(input, protocols[i])) {
			protocol = i;
			return (ISC_R_SUCCESS);
		}
	}

	return (ISC_R_BADNUMBER);
}

static isc_result_t
parse_address(const char *input) {
	struct in6_addr in6;
	struct in_addr in;

	if (inet_pton(AF_INET6, input, &in6) == 1) {
		isc_netaddr_fromin6(&netaddr, &in6);
		return (ISC_R_SUCCESS);
	}

	if (inet_pton(AF_INET, input, &in) == 1) {
		isc_netaddr_fromin(&netaddr, &in);
		return (ISC_R_SUCCESS);
	}

	return (ISC_R_BADADDRESSFORM);
}

static int
parse_workers(const char *input) {
	char *endptr = NULL;
	long val = strtol(input, &endptr, 10);

	if ((*endptr != '\0') || (val <= 0) || (val >= 128)) {
		return (ISC_R_BADNUMBER);
	}

	workers = val;

	return (ISC_R_SUCCESS);
}

static void
parse_options(int argc, char **argv) {
	char buf[ISC_NETADDR_FORMATSIZE];

	/* Set defaults */
	RUNTIME_CHECK(parse_protocol("UDP") == ISC_R_SUCCESS);
	RUNTIME_CHECK(parse_port("53000") == ISC_R_SUCCESS);
	RUNTIME_CHECK(parse_address("::1") == ISC_R_SUCCESS);
	workers = isc_os_ncpus();

	while (true) {
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{ "port", required_argument, NULL, 'p' },
			{ "address", required_argument, NULL, 'a' },
			{ "protocol", required_argument, NULL, 'P' },
			{ "workers", required_argument, NULL, 'w' },
			{ 0, 0, NULL, 0 }
		};

		c = getopt_long(argc, argv, "a:p:P:w:", long_options,
				&option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'a':
			RUNTIME_CHECK(parse_address(optarg) == ISC_R_SUCCESS);
			break;

		case 'p':
			RUNTIME_CHECK(parse_port(optarg) == ISC_R_SUCCESS);
			break;

		case 'P':
			RUNTIME_CHECK(parse_protocol(optarg) == ISC_R_SUCCESS);
			break;

		case 'w':
			RUNTIME_CHECK(parse_workers(optarg) == ISC_R_SUCCESS);
			break;

		default:
			UNREACHABLE();
		}
	}

	isc_sockaddr_fromnetaddr(&sockaddr, &netaddr, port);

	isc_sockaddr_format(&sockaddr, buf, sizeof(buf));

	printf("Will listen at %s://%s, %d workers\n", protocols[protocol], buf,
	       workers);
}

static void
_signal(int sig, void (*handler)(int)) {
	struct sigaction sa = { .sa_handler = handler };

	RUNTIME_CHECK(sigfillset(&sa.sa_mask) == 0);
	RUNTIME_CHECK(sigaction(sig, &sa, NULL) >= 0);
}

static void
setup(void) {
	sigset_t sset;

	_signal(SIGPIPE, SIG_IGN);
	_signal(SIGHUP, SIG_DFL);
	_signal(SIGTERM, SIG_DFL);
	_signal(SIGINT, SIG_DFL);

	RUNTIME_CHECK(sigemptyset(&sset) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGHUP) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGINT) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_BLOCK, &sset, NULL) == 0);

	isc_mem_create(&mctx);

	isc_managers_create(mctx, workers, 0, &netmgr, NULL, NULL);
}

static void
teardown(void) {
	isc_managers_destroy(&netmgr, NULL, NULL);
	isc_mem_destroy(&mctx);
	if (tls_ctx) {
		isc_tlsctx_free(&tls_ctx);
	}
}

static void
test_server_yield(void) {
	sigset_t sset;
	int sig;

	RUNTIME_CHECK(sigemptyset(&sset) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGHUP) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGINT) == 0);
	RUNTIME_CHECK(sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(sigwait(&sset, &sig) == 0);

	fprintf(stderr, "Shutting down...\n");
}

static void
read_cb(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	void *cbarg) {
	isc_region_t *reply = NULL;

	REQUIRE(handle != NULL);
	REQUIRE(eresult == ISC_R_SUCCESS);
	UNUSED(cbarg);

	fprintf(stderr, "RECEIVED %u bytes\n", region->length);

	if (region->length >= 12) {
		/* long enough to be a DNS header, set QR bit */
		((uint8_t *)region->base)[2] ^= 0x80;
	}

	reply = isc_mem_get(mctx, sizeof(isc_region_t) + region->length);
	reply->length = region->length;
	reply->base = (uint8_t *)reply + sizeof(isc_region_t);
	memmove(reply->base, region->base, region->length);

	isc_nm_send(handle, reply, send_cb, reply);
	return;
}

static void
send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_region_t *reply = cbarg;

	REQUIRE(handle != NULL);
	REQUIRE(eresult == ISC_R_SUCCESS);

	isc_mem_put(mctx, cbarg, sizeof(isc_region_t) + reply->length);
}

static isc_result_t
accept_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	REQUIRE(handle != NULL);
	REQUIRE(eresult == ISC_R_SUCCESS);
	UNUSED(cbarg);

	return (ISC_R_SUCCESS);
}

static void
run(void) {
	isc_result_t result;
	isc_nmsocket_t *sock = NULL;

	switch (protocol) {
	case UDP:
		result = isc_nm_listenudp(netmgr, &sockaddr, read_cb, NULL, 0,
					  &sock);
		break;
	case TCP:
		result = isc_nm_listentcpdns(netmgr, &sockaddr, read_cb, NULL,
					     accept_cb, NULL, 0, 0, NULL,
					     &sock);
		break;
	case DOT: {
		isc_tlsctx_createserver(NULL, NULL, &tls_ctx);

		result = isc_nm_listentlsdns(netmgr, &sockaddr, read_cb, NULL,
					     accept_cb, NULL, 0, 0, NULL,
					     tls_ctx, &sock);
		break;
	}
#if HAVE_LIBNGHTTP2
	case HTTPS:
	case HTTP: {
		bool is_https = protocol == HTTPS;
		isc_nm_http_endpoints_t *eps = NULL;
		if (is_https) {
			isc_tlsctx_createserver(NULL, NULL, &tls_ctx);
		}
		eps = isc_nm_http_endpoints_new(mctx);
		result = isc_nm_http_endpoints_add(
			eps, ISC_NM_HTTP_DEFAULT_PATH, read_cb, NULL, 0);

		if (result == ISC_R_SUCCESS) {
			result = isc_nm_listenhttp(netmgr, &sockaddr, 0, NULL,
						   tls_ctx, eps, 0, &sock);
		}
		isc_nm_http_endpoints_detach(&eps);
	} break;
#endif
	default:
		UNREACHABLE();
	}
	REQUIRE(result == ISC_R_SUCCESS);

	test_server_yield();

	isc_nm_stoplistening(sock);
	isc_nmsocket_close(&sock);
}

int
main(int argc, char **argv) {
	parse_options(argc, argv);

	setup();

	run();

	teardown();

	exit(EXIT_SUCCESS);
}
