/*	$NetBSD: fuzz.c,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

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

#include "config.h"

#include <named/fuzz.h>

#ifdef ENABLE_AFL
#include <named/globals.h>
#include <named/server.h>
#include <sys/errno.h>

#include <isc/app.h>
#include <isc/condition.h>
#include <isc/mutex.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <named/log.h>
#include <dns/log.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#ifndef __AFL_LOOP
#error To use American Fuzzy Lop you have to set CC to afl-clang-fast!!!
#endif

/*
 * We are using pthreads directly because we might be using it with
 * unthreaded version of BIND, where all thread functions are
 * mocks. Since AFL for now only works on Linux it's not a problem.
 */
static pthread_cond_t cond;
static pthread_mutex_t mutex;
static isc_boolean_t ready;

/*
 * In "client:" mode, this thread reads fuzzed query messages from AFL
 * from standard input and sends it to named's listening port (DNS) that
 * is passed in the -A client:<address>:<port> option. It can be used to
 * test named from the client side.
 */
static void *
fuzz_thread_client(void *arg) {
	char *host;
	char *port;
	struct sockaddr_in servaddr;
	int sockfd;
	int loop;
	void *buf;

	UNUSED(arg);

	/*
	 * Parse named -A argument in the "address:port" syntax. Due to
	 * the syntax used, this only supports IPv4 addresses.
	 */
	host = strdup(named_g_fuzz_addr);
	RUNTIME_CHECK(host != NULL);

	port = strchr(host, ':');
	RUNTIME_CHECK(port != NULL);
	*port = 0;
	++port;

	memset(&servaddr, 0, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	RUNTIME_CHECK(inet_pton(AF_INET, host, &servaddr.sin_addr) == 1);
	servaddr.sin_port = htons(atoi(port));

	free(host);

	/*
	 * Wait for named to start. This is set in run_server() in the
	 * named thread.
	 */
	while (!named_g_run_done) {
		usleep(10000);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	RUNTIME_CHECK(sockfd != -1);

	buf = malloc(65536);
	RUNTIME_CHECK(buf != NULL);

	/*
	 * Processing fuzzed packets 100,000 times before shutting down
	 * the app.
	 */
	for (loop = 0; loop < 100000; loop++) {
		ssize_t length;
		ssize_t sent;

		length = read(0, buf, 65536);
		if (length <= 0) {
			usleep(1000000);
			continue;
		}

		/*
		 * Ignore packets that are larger than 4096 bytes.
		 */
		if (length > 4096) {
			/*
			 * AFL_CMIN doesn't support persistent mode, so
			 * shutdown the server.
			 */
			if (getenv("AFL_CMIN")) {
				free(buf);
				close(sockfd);
				named_server_flushonshutdown(named_g_server,
							     ISC_FALSE);
				isc_app_shutdown();
				return (NULL);
			}
			raise(SIGSTOP);
			continue;
		}

		RUNTIME_CHECK(pthread_mutex_lock(&mutex) == 0);

		ready = ISC_FALSE;

		sent = sendto(sockfd, buf, length, 0,
			      (struct sockaddr *) &servaddr, sizeof(servaddr));
		RUNTIME_CHECK(sent == length);

		/*
		 * Read the reply message from named to unclog it. Don't
		 * bother if there isn't a reply.
		 */
		recvfrom(sockfd, buf, 65536, MSG_DONTWAIT, NULL, NULL);

		while (!ready)
			pthread_cond_wait(&cond, &mutex);

		RUNTIME_CHECK(pthread_mutex_unlock(&mutex) == 0);
	}

	free(buf);
	close(sockfd);

	named_server_flushonshutdown(named_g_server, ISC_FALSE);
	isc_app_shutdown();

	return (NULL);
}

/*
 * In "resolver:" mode, this thread reads fuzzed reply messages from AFL
 * from standard input. It also sets up a listener as a remote
 * authoritative server and sends a driver query to the client side of
 * named(resolver).  When named(resolver) connects to this authoritative
 * server, this thread writes the fuzzed reply message from AFL to it.
 *
 * -A resolver:<saddress>:<sport>:<raddress>:<rport>
 *
 * Here, <saddress>:<sport> is where named(resolver) is listening on.
 * <raddress>:<rport> is where the thread is supposed to setup the
 * authoritative server. This address should be configured via the root
 * zone to be the authoritiative server for aaaaaaaaaa.example.
 *
 * named(resolver) when being fuzzed will not cache answers.
 */
static void *
fuzz_thread_resolver(void *arg) {
	char *sqtype, *shost, *sport, *rhost, *rport;
	struct sockaddr_in servaddr, recaddr, recvaddr;
	/*
	 * Query for aaaaaaaaaa.example./A in wire format with RD=1,
	 * EDNS and DO=1. 0x88, 0x0c at the start is the ID field which
	 * will be updated for each query.
	 */
	char respacket[] = {
		 0x88, 0x0c, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x01, 0x0a, 0x61, 0x61, 0x61,
		 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x07,
		 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x00,
		 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x29, 0x10,
		 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00
	};
	/*
	 * Response for example./DNSKEY in wire format. Note that RRSIGs
	 * were generated with this DNSKEY that are used as seeds for
	 * AFL in the DNSSEC fuzzing job. So the DNSKEY content of this
	 * message must not change, or the corresponding RRSIGs will
	 * have to be updated. 0x8d, 0xf6 at the start is the ID field
	 * which will be made to match the query.
	 */
	const isc_uint8_t dnskey_wf[] = {
		0x8d, 0xf6, 0x84, 0x00, 0x00, 0x01, 0x00, 0x02,
		0x00, 0x00, 0x00, 0x01, 0x07, 0x65, 0x78, 0x61,
		0x6d, 0x70, 0x6c, 0x65, 0x00, 0x00, 0x30, 0x00,
		0x01, 0xc0, 0x0c, 0x00, 0x30, 0x00, 0x01, 0x00,
		0x00, 0x01, 0x2c, 0x01, 0x08, 0x01, 0x00, 0x03,
		0x08, 0x03, 0x01, 0x00, 0x01, 0xbd, 0x81, 0xdc,
		0x7f, 0x16, 0xd4, 0x81, 0x7c, 0x1f, 0x9f, 0x6a,
		0x68, 0xdd, 0xd4, 0xda, 0x48, 0xd9, 0x1c, 0xbd,
		0xa6, 0x46, 0x1a, 0xf0, 0xb4, 0xb9, 0xec, 0x3d,
		0x6c, 0x0b, 0x57, 0xc7, 0xd6, 0x54, 0x66, 0xe6,
		0x6c, 0xd5, 0x90, 0x3a, 0x78, 0x7d, 0x7f, 0x78,
		0x80, 0xa2, 0x89, 0x61, 0x6d, 0x8a, 0x2b, 0xcd,
		0x0a, 0x77, 0x7a, 0xad, 0xc9, 0x61, 0x53, 0x53,
		0x8c, 0x99, 0x72, 0x86, 0x14, 0x74, 0x9c, 0x49,
		0x2a, 0x47, 0x23, 0xf7, 0x02, 0x07, 0x73, 0x1c,
		0x5c, 0x2e, 0xb4, 0x9a, 0xa4, 0xd7, 0x98, 0x42,
		0xc3, 0xd2, 0xfe, 0xbf, 0xf3, 0xb3, 0x6a, 0x52,
		0x92, 0xd5, 0xfa, 0x47, 0x00, 0xe3, 0xd9, 0x59,
		0x31, 0x95, 0x48, 0x40, 0xfc, 0x06, 0x73, 0x90,
		0xc6, 0x73, 0x96, 0xba, 0x29, 0x91, 0xe2, 0xac,
		0xa3, 0xa5, 0x6d, 0x91, 0x6d, 0x52, 0xb9, 0x34,
		0xba, 0x68, 0x4f, 0xad, 0xf0, 0xc3, 0xf3, 0x1d,
		0x6d, 0x61, 0x76, 0xe5, 0x3d, 0xa3, 0x9b, 0x2a,
		0x0c, 0x92, 0xb3, 0x78, 0x6b, 0xf1, 0x20, 0xd6,
		0x90, 0xb7, 0xac, 0xe2, 0xf8, 0x2b, 0x94, 0x10,
		0x79, 0xce, 0xa8, 0x60, 0x42, 0xea, 0x6a, 0x18,
		0x2f, 0xc0, 0xd8, 0x05, 0x0a, 0x3b, 0x06, 0x0f,
		0x02, 0x7e, 0xff, 0x33, 0x46, 0xee, 0xb6, 0x21,
		0x25, 0x90, 0x63, 0x4b, 0x3b, 0x5e, 0xb2, 0x72,
		0x3a, 0xcb, 0x91, 0x41, 0xf4, 0x20, 0x50, 0x78,
		0x1c, 0x93, 0x95, 0xda, 0xfa, 0xae, 0x85, 0xc5,
		0xd7, 0x6b, 0x92, 0x0c, 0x70, 0x6b, 0xe4, 0xb7,
		0x29, 0x3a, 0x2e, 0x18, 0x88, 0x82, 0x33, 0x7c,
		0xa8, 0xea, 0xb8, 0x31, 0x8f, 0xaf, 0x50, 0xc5,
		0x9c, 0x08, 0x56, 0x8f, 0x09, 0x76, 0x4e, 0xdf,
		0x97, 0x75, 0x9d, 0x00, 0x52, 0x7f, 0xdb, 0xec,
		0x30, 0xcb, 0x1c, 0x4c, 0x2a, 0x21, 0x93, 0xc4,
		0x6d, 0x85, 0xa9, 0x40, 0x3b, 0xc0, 0x0c, 0x00,
		0x2e, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2c, 0x01,
		0x1b, 0x00, 0x30, 0x08, 0x01, 0x00, 0x00, 0x01,
		0x2c, 0x67, 0x74, 0x85, 0x80, 0x58, 0xb3, 0xc5,
		0x17, 0x36, 0x90, 0x07, 0x65, 0x78, 0x61, 0x6d,
		0x70, 0x6c, 0x65, 0x00, 0x45, 0xac, 0xd3, 0x82,
		0x69, 0xf3, 0x10, 0x3a, 0x97, 0x2c, 0x6a, 0xa9,
		0x78, 0x99, 0xea, 0xb0, 0xcc, 0xf7, 0xaf, 0x33,
		0x51, 0x5b, 0xdf, 0x77, 0x04, 0x18, 0x14, 0x99,
		0x61, 0xeb, 0x8d, 0x76, 0x3f, 0xd1, 0x71, 0x14,
		0x43, 0x80, 0x53, 0xc2, 0x3b, 0x9f, 0x09, 0x4f,
		0xb3, 0x51, 0x04, 0x89, 0x0e, 0xc8, 0x54, 0x12,
		0xcd, 0x07, 0x20, 0xbe, 0x94, 0xc2, 0xda, 0x99,
		0xdd, 0x1e, 0xf8, 0xb0, 0x84, 0x2e, 0xf9, 0x19,
		0x35, 0x36, 0xf5, 0xd0, 0x5d, 0x82, 0x18, 0x74,
		0xa0, 0x00, 0xb6, 0x15, 0x57, 0x40, 0x5f, 0x78,
		0x2d, 0x27, 0xac, 0xc7, 0x8a, 0x29, 0x55, 0xa9,
		0xcd, 0xbc, 0xf7, 0x3e, 0xff, 0xae, 0x1a, 0x5a,
		0x1d, 0xac, 0x0d, 0x78, 0x0e, 0x08, 0x33, 0x6c,
		0x59, 0x70, 0x40, 0xb9, 0x65, 0xbd, 0x35, 0xbb,
		0x9a, 0x70, 0xdc, 0x93, 0x66, 0xb0, 0xef, 0xfe,
		0xf0, 0x32, 0xa6, 0xee, 0xb7, 0x03, 0x89, 0xa2,
		0x4d, 0xe0, 0xf1, 0x20, 0xdf, 0x39, 0xe8, 0xe3,
		0xcc, 0x95, 0xe9, 0x9a, 0xad, 0xbf, 0xbd, 0x7c,
		0xf7, 0xd7, 0xde, 0x47, 0x9e, 0xf6, 0x17, 0xbb,
		0x84, 0xa9, 0xed, 0xf2, 0x45, 0x61, 0x6d, 0x13,
		0x0b, 0x06, 0x29, 0x50, 0xde, 0xfd, 0x42, 0xb0,
		0x66, 0x2c, 0x1c, 0x2b, 0x63, 0xcb, 0x4e, 0xb9,
		0x31, 0xc4, 0xea, 0xd2, 0x07, 0x3a, 0x08, 0x79,
		0x19, 0x4b, 0x4c, 0x50, 0x97, 0x02, 0xd7, 0x26,
		0x41, 0x2f, 0xdd, 0x57, 0xaa, 0xb0, 0xa0, 0x21,
		0x4e, 0x74, 0xb6, 0x97, 0x4b, 0x8b, 0x09, 0x9c,
		0x3d, 0x29, 0xfb, 0x12, 0x27, 0x47, 0x8f, 0xb8,
		0xc5, 0x8e, 0x65, 0xcd, 0xca, 0x2f, 0xba, 0xf5,
		0x3e, 0xec, 0x56, 0xc3, 0xc9, 0xa1, 0x62, 0x7d,
		0xf2, 0x9f, 0x90, 0x16, 0x1d, 0xbf, 0x97, 0x28,
		0xe1, 0x92, 0xb1, 0x53, 0xab, 0xc4, 0xe0, 0x99,
		0xbb, 0x19, 0x90, 0x7c, 0x00, 0x00, 0x29, 0x10,
		0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00
	};

	int sockfd;
	int listenfd;
	int loop;
	isc_uint16_t qtype;
	char *buf, *rbuf;
	char *nameptr;
	unsigned int i;
	isc_uint8_t llen;
	isc_uint64_t seed;

	UNUSED(arg);

	/*
	 * Parse named -A argument in the "qtype:saddress:sport:raddress:rport"
	 * syntax.  Due to the syntax used, this only supports IPv4 addresses.
	 */
	sqtype = strdup(named_g_fuzz_addr);
	RUNTIME_CHECK(sqtype != NULL);

	shost = strchr(sqtype, ':');
	RUNTIME_CHECK(shost != NULL);
	*shost = 0;
	shost++;

	sport = strchr(shost, ':');
	RUNTIME_CHECK(sport != NULL);
	*sport = 0;
	sport++;

	rhost = strchr(sport, ':');
	RUNTIME_CHECK(rhost != NULL);
	*rhost = 0;
	rhost++;

	rport = strchr(rhost, ':');
	RUNTIME_CHECK(rport != NULL);
	*rport = 0;
	rport++;

	/*
	 * Patch in the qtype into the question section of respacket.
	 */
	qtype = atoi(sqtype);
	respacket[32] = (qtype >> 8) & 0xff;
	respacket[33] = qtype & 0xff;

	memset(&servaddr, 0, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	RUNTIME_CHECK(inet_pton(AF_INET, shost, &servaddr.sin_addr) == 1);
	servaddr.sin_port = htons(atoi(sport));

	memset(&recaddr, 0, sizeof (recaddr));
	recaddr.sin_family = AF_INET;
	RUNTIME_CHECK(inet_pton(AF_INET, rhost, &recaddr.sin_addr) == 1);
	recaddr.sin_port = htons(atoi(rport));

	free(sqtype);

	/*
	 * Wait for named to start. This is set in run_server() in the
	 * named thread.
	 */
	while (!named_g_run_done) {
		usleep(10000);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	RUNTIME_CHECK(sockfd != -1);

	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	RUNTIME_CHECK(listenfd != -1);

	RUNTIME_CHECK(bind(listenfd, (struct sockaddr *)&recaddr,
			   sizeof(struct sockaddr_in)) == 0);

	buf = malloc(65536);
	rbuf = malloc(65536);
	RUNTIME_CHECK(buf != NULL);
	RUNTIME_CHECK(rbuf != NULL);

	seed = 42;

	/*
	 * Processing fuzzed packets 100,000 times before shutting down
	 * the app.
	 */
	for (loop = 0; loop < 100000; loop++) {
		ssize_t length;
		ssize_t sent;
		unsigned short id;
		socklen_t socklen;

		memset(buf, 0, 12);
		length = read(0, buf, 65536);
		if (length <= 0) {
			usleep(1000000);
			continue;
		}

		if (length > 4096) {
			if (getenv("AFL_CMIN")) {
				free(buf);
				free(rbuf);
				close(sockfd);
				close(listenfd);
				named_server_flushonshutdown(named_g_server,
							     ISC_FALSE);
				isc_app_shutdown();
				return (NULL);
			}
			raise(SIGSTOP);
			continue;
		}

		if (length < 12) {
		    length = 12;
		}

		RUNTIME_CHECK(pthread_mutex_lock(&mutex) == 0);

		ready = ISC_FALSE;

		/* Use a unique query ID. */
		seed = 1664525 * seed + 1013904223;
		id = seed & 0xffff;
		respacket[0] = (id >> 8) & 0xff;
		respacket[1] = id & 0xff;

		/*
		 * Flush any pending data on the authoritative server.
		 */
		socklen = sizeof(recvaddr);
		sent = recvfrom(listenfd, rbuf, 65536, MSG_DONTWAIT,
			(struct sockaddr *) &recvaddr, &socklen);

		/*
		 * Send a fixed client query to named(resolver) of
		 * aaaaaaaaaa.example./A. This is the starting query
		 * driver.
		 */
		sent = sendto(sockfd, respacket, sizeof(respacket), 0,
		       (struct sockaddr *) &servaddr, sizeof(servaddr));
		RUNTIME_CHECK(sent == sizeof(respacket));

		/*
		 * named(resolver) will process the query above and send
		 * an upstream query to the authoritative server. We
		 * handle that here as the upstream authoritative server
		 * on listenfd.
		 */
		socklen = sizeof(recvaddr);
		sent = recvfrom(listenfd, rbuf, 65536, 0,
				(struct sockaddr *) &recvaddr, &socklen);
		RUNTIME_CHECK(sent > 0);

		/*
		 * Copy QID and set QR so that response is always
		 * accepted by named(resolver).
		 */
		buf[0] = rbuf[0];
		buf[1] = rbuf[1];
		buf[2] |= 0x80;

		/*
		 * NOTE: We are not copying the QNAME or setting
		 * rcode=NOERROR each time. So the resolver may fail the
		 * client query (driver) / wander due to this. AA flag
		 * may also not be set based on the contents of the AFL
		 * fuzzed packet.
		 */

		/*
		 * A hack - set QTYPE to the one from query so that we
		 * can easily share packets between instances. If we
		 * write over something else we'll get FORMERR anyway.
		 */

		/* Skip DNS header to get to the name */
		nameptr = buf + 12;

		/* Skip the name to get to the qtype */
		i = 0;
		while (((llen = nameptr[i]) != 0) &&
		       (i < 255) &&
		       (((nameptr + i + 1 + llen) - buf) < length))
			i += 1 + llen;

		if (i <= 255) {
			nameptr += 1 + i;
			/* Patch the qtype */
			if ((nameptr - buf) < (length - 2)) {
				*nameptr++ = (qtype >> 8) & 0xff;
				*nameptr++ = qtype & 0xff;
			}
			/* Patch the qclass */
			if ((nameptr - buf) < (length - 2)) {
				*nameptr++ = 0;
				*nameptr++ = 1;
			}
		}

		/*
		 * Send the reply to named(resolver).
		 */
		sent = sendto(listenfd, buf, length, 0,
			      (struct sockaddr *) &recvaddr, sizeof(recvaddr));
		RUNTIME_CHECK(sent == length);

		/* We might get additional questions here (e.g. for CNAME). */
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int rv;
			int max;

			FD_ZERO(&fds);
			FD_SET(listenfd, &fds);
			FD_SET(sockfd, &fds);
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			max = (listenfd > sockfd ? listenfd : sockfd)+1;

			rv = select(max, &fds, NULL, NULL, &tv);
			RUNTIME_CHECK(rv > 0);

			if (FD_ISSET(sockfd, &fds)) {
				/*
				 * It's the reply from named(resolver)
				 * to the client(query driver), so we're
				 * done.
				 */
				recvfrom(sockfd, buf, 65536, 0, NULL, NULL);
				break;
			}

			/*
			 * We've got additional question (eg. due to
			 * CNAME). Bounce it - setting QR flag and
			 * NOERROR rcode and sending it back.
			 */
			length = recvfrom(listenfd, buf, 65536, 0,
				   (struct sockaddr *) &recvaddr, &socklen);

			/*
			 * If this is a DNSKEY query, send the DNSKEY,
			 * otherwise, bounce the query.
			 */

			/* Skip DNS header to get to the name */
			nameptr = buf + 12;

			/* Skip the name to get to the qtype */
			i = 0;
			while (((llen = nameptr[i]) != 0) &&
			       (i < 255) &&
			       (((nameptr + i + 1 + llen) - buf) < length))
				i += 1 + llen;

			if (i <= 255) {
				nameptr += 1 + i;
				/*
				 * Patch in the DNSKEY reply without
				 * touching the ID field. Note that we
				 * don't compare the name in the
				 * question section in the query, but we
				 * don't expect to receive any query for
				 * type DNSKEY but for the name
				 * "example."
				 */
				if ((nameptr - buf) < (length - 2)) {
					isc_uint8_t hb, lb;
					hb = *nameptr++;
					lb = *nameptr++;
					qtype = (hb << 8) | lb;

					if (qtype == 48) {
						memmove(buf + 2, dnskey_wf + 2,
							sizeof (dnskey_wf) - 2);
						length = sizeof (dnskey_wf);
					}
				}
			}

			buf[2] |= 0x80;
			buf[3] &= 0xF0;
			sent = sendto(listenfd, buf, length, 0,
				      (struct sockaddr *) &recvaddr,
				      sizeof(recvaddr));
			RUNTIME_CHECK(sent == length);
		}

		while (!ready)
			pthread_cond_wait(&cond, &mutex);

		RUNTIME_CHECK(pthread_mutex_unlock(&mutex) == 0);
	}

	free(buf);
	close(sockfd);
	close(listenfd);
	named_server_flushonshutdown(named_g_server, ISC_FALSE);
	isc_app_shutdown();

	/*
	 * This is here just for the signature, that's how AFL detects
	 * if it's a 'persistent mode' binary. It has to occur somewhere
	 * in the file, that's all. < wpk_> AFL checks the binary for
	 * this signature ("##SIG_AFL_PERSISTENT##") and runs the binary
	 * in persistent mode if it's present.
	 */
	__AFL_LOOP(0);

	return (NULL);
}

/*
 * In "tcp:", "http:" and "rndc:" modes, this thread reads fuzzed query
 * blobs from AFL from standard input and sends it to the corresponding
 * TCP listening port of named (port 53 DNS, or the HTTP statistics
 * channels listener or the rndc port) that is passed in the -A
 * <mode>:<address>:<port> option. It can be used to test named from the
 * client side.
 */
static void *
fuzz_thread_tcp(void *arg) {
	char *host;
	char *port;
	struct sockaddr_in servaddr;
	int sockfd;
	char *buf;
	int loop;

	UNUSED(arg);

	/*
	 * Parse named -A argument in the "address:port" syntax. Due to
	 * the syntax used, this only supports IPv4 addresses.
	 */
	host = strdup(named_g_fuzz_addr);
	RUNTIME_CHECK(host != NULL);

	port = strchr(host, ':');
	RUNTIME_CHECK(port != NULL);
	*port = 0;
	++port;

	memset(&servaddr, 0, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	RUNTIME_CHECK(inet_pton(AF_INET, host, &servaddr.sin_addr) == 1);
	servaddr.sin_port = htons(atoi(port));

	free(host);

	/*
	 * Wait for named to start. This is set in run_server() in the
	 * named thread.
	 */
	while (!named_g_run_done) {
		usleep(10000);
	}

	buf = malloc(65539);
	RUNTIME_CHECK(buf != NULL);

	/*
	 * Processing fuzzed packets 100,000 times before shutting down
	 * the app.
	 */
	for (loop = 0; loop < 100000; loop++) {
		ssize_t length;
		ssize_t sent;
		int yes;
		int r;

		if (named_g_fuzz_type == isc_fuzz_tcpclient) {
			/*
			 * To fuzz DNS TCP client we have to put 16-bit
			 * message length preceding the start of packet.
			 */
			length = read(0, buf+2, 65535);
			buf[0] = (length >> 8) & 0xff;
			buf[1] = length & 0xff;
			length += 2;
		} else {
			/*
			 * Other types of TCP clients such as HTTP, etc.
			 */
			length = read(0, buf, 65535);
		}
		if (length <= 0) {
			usleep(1000000);
			continue;
		}
		if (named_g_fuzz_type == isc_fuzz_http) {
			/*
			 * This guarantees that the request will be
			 * processed.
			 */
			INSIST(length <= 65535);
			buf[length++]='\r';
			buf[length++]='\n';
			buf[length++]='\r';
			buf[length++]='\n';
		}

		RUNTIME_CHECK(pthread_mutex_lock(&mutex) == 0);

		ready = ISC_FALSE;
		yes = 1;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		RUNTIME_CHECK(sockfd != -1);
		RUNTIME_CHECK(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
					 &yes, sizeof(int)) == 0);

		do {
			r = connect(sockfd, (struct sockaddr*)&servaddr,
				    sizeof(servaddr));
			if (r != 0)
				usleep(10000);
		} while (r != 0);

		/*
		 * Send the fuzzed query blob to the target server.
		 */
		sent = write(sockfd, buf, length);
		RUNTIME_CHECK(sent == length);

		close(sockfd);

		while (!ready)
			pthread_cond_wait(&cond, &mutex);

		RUNTIME_CHECK(pthread_mutex_unlock(&mutex) == 0);
	}

	free(buf);
	close(sockfd);
	named_server_flushonshutdown(named_g_server, ISC_FALSE);
	isc_app_shutdown();

	return (NULL);
}

#endif /* ENABLE_AFL */

/*
 * named has finished processing a message and has sent the
 * reply. Signal the fuzz thread using the condition variable, to read
 * and process the next item from AFL.
 */
void
named_fuzz_notify(void) {
#ifdef ENABLE_AFL
	if (getenv("AFL_CMIN")) {
		named_server_flushonshutdown(named_g_server, ISC_FALSE);
		isc_app_shutdown();
		return;
	}

	raise(SIGSTOP);

	RUNTIME_CHECK(pthread_mutex_lock(&mutex) == 0);

	ready = ISC_TRUE;

	RUNTIME_CHECK(pthread_cond_signal(&cond) == 0);
	RUNTIME_CHECK(pthread_mutex_unlock(&mutex) == 0);
#endif /* ENABLE_AFL */
}

void
named_fuzz_setup(void) {
#ifdef ENABLE_AFL
	if (getenv("__AFL_PERSISTENT") || getenv("AFL_CMIN")) {
		pthread_t thread;
		void *(fn) = NULL;

		switch (named_g_fuzz_type) {
		case isc_fuzz_client:
			fn = fuzz_thread_client;
			break;

		case isc_fuzz_http:
		case isc_fuzz_tcpclient:
		case isc_fuzz_rndc:
			fn = fuzz_thread_tcp;
			break;

		case isc_fuzz_resolver:
			fn = fuzz_thread_resolver;
			break;

		default:
			RUNTIME_CHECK(fn != NULL);
		}

		RUNTIME_CHECK(pthread_mutex_init(&mutex, NULL) == 0);
		RUNTIME_CHECK(pthread_cond_init(&cond, NULL) == 0);
		RUNTIME_CHECK(pthread_create(&thread, NULL, fn, NULL) == 0);
	}
#endif /* ENABLE_AFL */
}
