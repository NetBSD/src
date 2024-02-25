/*	$NetBSD: test_client.c,v 1.2.2.2 2024/02/25 15:43:13 martin Exp $	*/

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

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/netmgr.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

typedef enum {
	UDP,
	TCP,
	DOT,
	HTTPS_POST,
	HTTPS_GET,
	HTTP_POST,
	HTTP_GET
} protocol_t;

static const char *protocols[] = { "udp",	    "tcp",
				   "dot",	    "https-post",
				   "https-get",	    "http-plain-post",
				   "http-plain-get" };

static isc_mem_t *mctx = NULL;
static isc_nm_t *netmgr = NULL;

static protocol_t protocol;
static const char *address;
static const char *port;
static int family = AF_UNSPEC;
static isc_sockaddr_t sockaddr_local;
static isc_sockaddr_t sockaddr_remote;
static int workers;
static int timeout;
static uint8_t messagebuf[2 * 65536];
static isc_region_t message = { .length = 0, .base = messagebuf };
static int out = -1;

static isc_tlsctx_t *tls_ctx = NULL;

static isc_result_t
parse_port(const char *input) {
	char *endptr = NULL;
	long val = strtol(input, &endptr, 10);

	if ((*endptr != '\0') || (val <= 0) || (val >= 65536)) {
		return (ISC_R_BADNUMBER);
	}

	port = input;

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
		family = AF_INET6;
		address = input;
		return (ISC_R_SUCCESS);
	}

	if (inet_pton(AF_INET, input, &in) == 1) {
		family = AF_INET;
		address = input;
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

static isc_result_t
parse_timeout(const char *input) {
	char *endptr = NULL;
	long val = strtol(input, &endptr, 10);

	if ((*endptr != '\0') || (val <= 0) || (val >= 120)) {
		return (ISC_R_BADNUMBER);
	}

	timeout = (in_port_t)val * 1000;

	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_input(const char *input) {
	int in = -1;

	if (!strcmp(input, "-")) {
		in = 0;
	} else {
		in = open(input, O_RDONLY);
	}
	RUNTIME_CHECK(in >= 0);

	message.length = read(in, message.base, sizeof(messagebuf));

	close(in);

	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_output(const char *input) {
	if (!strcmp(input, "-")) {
		out = 1;
	} else {
		out = open(input, O_WRONLY | O_CREAT,
			   S_IRUSR | S_IRGRP | S_IROTH);
	}
	RUNTIME_CHECK(out >= 0);

	return (ISC_R_SUCCESS);
}

static void
parse_options(int argc, char **argv) {
	char buf[ISC_NETADDR_FORMATSIZE];

	/* Set defaults */
	RUNTIME_CHECK(parse_protocol("UDP") == ISC_R_SUCCESS);
	RUNTIME_CHECK(parse_port("53000") == ISC_R_SUCCESS);
	RUNTIME_CHECK(parse_address("::0") == ISC_R_SUCCESS);
	workers = isc_os_ncpus();

	while (true) {
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{ "port", required_argument, NULL, 'p' },
			{ "address", required_argument, NULL, 'a' },
			{ "protocol", required_argument, NULL, 'P' },
			{ "workers", required_argument, NULL, 'w' },
			{ "timeout", required_argument, NULL, 't' },
			{ "input", required_argument, NULL, 'i' },
			{ "output", required_argument, NULL, 'o' },
			{ 0, 0, NULL, 0 }
		};

		c = getopt_long(argc, argv, "a:p:P:w:t:i:o:", long_options,
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

		case 't':
			RUNTIME_CHECK(parse_timeout(optarg) == ISC_R_SUCCESS);
			break;

		case 'i':
			RUNTIME_CHECK(parse_input(optarg) == ISC_R_SUCCESS);
			break;

		case 'o':
			RUNTIME_CHECK(parse_output(optarg) == ISC_R_SUCCESS);
			break;

		default:
			UNREACHABLE();
		}
	}

	{
		struct addrinfo hints = {
			.ai_family = family,
			.ai_socktype = (protocol == UDP) ? SOCK_DGRAM
							 : SOCK_STREAM,
		};
		struct addrinfo *result = NULL;
		int r = getaddrinfo(address, NULL, &hints, &result);
		RUNTIME_CHECK(r == 0);

		for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
		{
			RUNTIME_CHECK(isc_sockaddr_fromsockaddr(&sockaddr_local,
								rp->ai_addr) ==
				      ISC_R_SUCCESS);
		}
		freeaddrinfo(result);
	}

	{
		struct addrinfo hints = {
			.ai_family = family,
			.ai_socktype = (protocol == UDP) ? SOCK_DGRAM
							 : SOCK_STREAM,
		};
		struct addrinfo *result = NULL;
		int r = getaddrinfo(argv[optind], port, &hints, &result);
		RUNTIME_CHECK(r == 0);

		for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
		{
			RUNTIME_CHECK(isc_sockaddr_fromsockaddr(
					      &sockaddr_remote, rp->ai_addr) ==
				      ISC_R_SUCCESS);
		}
		freeaddrinfo(result);
	}

	isc_sockaddr_format(&sockaddr_local, buf, sizeof(buf));

	printf("Will connect from %s://%s", protocols[protocol], buf);

	isc_sockaddr_format(&sockaddr_remote, buf, sizeof(buf));

	printf(" to %s, %d workers\n", buf, workers);
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
	if (out > 0) {
		close(out);
	}

	isc_managers_destroy(&netmgr, NULL, NULL);
	isc_mem_destroy(&mctx);
	if (tls_ctx) {
		isc_tlsctx_free(&tls_ctx);
	}
}

static void
waitforsignal(void) {
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
	isc_nmhandle_t *readhandle = cbarg;

	REQUIRE(handle != NULL);
	REQUIRE(eresult == ISC_R_SUCCESS || eresult == ISC_R_CANCELED ||
		eresult == ISC_R_EOF);
	REQUIRE(cbarg != NULL);

	fprintf(stderr, "%s(..., %s, ...)\n", __func__,
		isc_result_totext(eresult));

	if (eresult == ISC_R_SUCCESS) {
		printf("RECEIVED %u bytes\n", region->length);
		if (out >= 0) {
			ssize_t len = write(out, region->base, region->length);
			close(out);
			REQUIRE((size_t)len == region->length);
		}
	}

	isc_nmhandle_detach(&readhandle);
	kill(getpid(), SIGTERM);
}

static void
send_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	REQUIRE(handle != NULL);
	REQUIRE(eresult == ISC_R_SUCCESS || eresult == ISC_R_CANCELED ||
		eresult == ISC_R_EOF);
	REQUIRE(cbarg == NULL);
}

static void
connect_cb(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmhandle_t *readhandle = NULL;

	REQUIRE(handle != NULL);
	UNUSED(cbarg);

	fprintf(stderr, "ECHO_CLIENT:%s:%s\n", __func__,
		isc_result_totext(eresult));

	if (eresult != ISC_R_SUCCESS) {
		kill(getpid(), SIGTERM);
		return;
	}

	isc_nmhandle_attach(handle, &readhandle);
	isc_nm_read(handle, read_cb, readhandle);
	isc_nm_send(handle, &message, send_cb, NULL);
}

static void
run(void) {
	switch (protocol) {
	case UDP:
		isc_nm_udpconnect(netmgr, &sockaddr_local, &sockaddr_remote,
				  connect_cb, NULL, timeout, 0);
		break;
	case TCP:
		isc_nm_tcpdnsconnect(netmgr, &sockaddr_local, &sockaddr_remote,
				     connect_cb, NULL, timeout, 0);
		break;
	case DOT: {
		isc_tlsctx_createclient(&tls_ctx);

		isc_nm_tlsdnsconnect(netmgr, &sockaddr_local, &sockaddr_remote,
				     connect_cb, NULL, timeout, 0, tls_ctx,
				     NULL);
		break;
	}
#if HAVE_LIBNGHTTP2
	case HTTP_GET:
	case HTTPS_GET:
	case HTTPS_POST:
	case HTTP_POST: {
		bool is_https = (protocol == HTTPS_POST ||
				 protocol == HTTPS_GET);
		bool is_post = (protocol == HTTPS_POST ||
				protocol == HTTP_POST);
		char req_url[256];
		isc_nm_http_makeuri(is_https, &sockaddr_remote, NULL, 0,
				    ISC_NM_HTTP_DEFAULT_PATH, req_url,
				    sizeof(req_url));
		if (is_https) {
			isc_tlsctx_createclient(&tls_ctx);
		}
		isc_nm_httpconnect(netmgr, &sockaddr_local, &sockaddr_remote,
				   req_url, is_post, connect_cb, NULL, tls_ctx,
				   NULL, timeout, 0);
	} break;
#endif
	default:
		UNREACHABLE();
	}

	waitforsignal();
}

int
main(int argc, char **argv) {
	parse_options(argc, argv);

	setup();

	run();

	teardown();

	exit(EXIT_SUCCESS);
}
