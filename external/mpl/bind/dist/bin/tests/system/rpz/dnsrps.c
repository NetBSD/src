/*	$NetBSD: dnsrps.c,v 1.9 2025/01/26 16:25:00 christos Exp $	*/

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

/*
 * -a		exit(EXIT_SUCCESS) if dnsrps is available or dlopen() msg if not
 * -n domain	print the serial number of a domain to check if a new
 *		    version of a policy zone is ready.
 *		    Exit(1) if dnsrps is not available
 * -w sec.ond	wait for seconds, because `sleep 0.1` is not portable
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/util.h>

#ifdef USE_DNSRPS
#include <dns/librpz.h>
#else  /* ifdef USE_DNSRPS */
typedef struct {
	char c[120];
} librpz_emsg_t;
#endif /* ifdef USE_DNSRPS */

static bool
link_dnsrps(librpz_emsg_t *emsg);

#define USAGE "usage: [-a] [-n domain] [-w sec.onds]\n"

int
main(int argc, char **argv) {
#ifdef USE_DNSRPS
	char cstr[sizeof("zone ") + 1024 + 10];
	librpz_clist_t *clist = NULL;
	librpz_client_t *client = NULL;
	librpz_rsp_t *rsp = NULL;
	uint32_t serial;
#endif /* ifdef USE_DNSRPS */
	double seconds;
	librpz_emsg_t emsg;
	char *p;
	int i;

	while ((i = getopt(argc, argv, "an:w:")) != -1) {
		switch (i) {
		case 'a':
			if (!link_dnsrps(&emsg)) {
				printf("I:%s\n", emsg.c);
				return 1;
			}
			return 0;

		case 'n':
			if (!link_dnsrps(&emsg)) {
				fprintf(stderr, "## %s\n", emsg.c);
				return 1;
			}
#ifdef USE_DNSRPS
			/*
			 * Get the serial number of a policy zone.
			 */
			clist = librpz->clist_create(&emsg, NULL, NULL, NULL,
						     NULL, NULL);
			if (clist == NULL) {
				fprintf(stderr, "## %s: %s\n", optarg, emsg.c);
				return 1;
			}
			snprintf(cstr, sizeof(cstr), "zone %s;", optarg);
			client = librpz->client_create(&emsg, clist, cstr,
						       true);
			if (client == NULL) {
				fprintf(stderr, "## %s\n", emsg.c);
				librpz->clist_detach(&clist);
				return 1;
			}

			rsp = NULL;
			if (!librpz->rsp_create(&emsg, &rsp, NULL, client, true,
						false) ||
			    rsp == NULL)
			{
				fprintf(stderr, "## %s\n", emsg.c);
				librpz->client_detach(&client);
				librpz->clist_detach(&clist);
				return 1;
			}

			if (!librpz->soa_serial(&emsg, &serial, optarg, rsp)) {
				fprintf(stderr, "## %s\n", emsg.c);
				librpz->rsp_detach(&rsp);
				librpz->client_detach(&client);
				librpz->clist_detach(&clist);
				return 1;
			}
			librpz->rsp_detach(&rsp);
			librpz->client_detach(&client);
			librpz->clist_detach(&clist);
			printf("%u\n", serial);
#else  /* ifdef USE_DNSRPS */
			UNREACHABLE();
#endif /* ifdef USE_DNSRPS */
			return 0;

		case 'w':
			seconds = strtod(optarg, &p);
			if (seconds <= 0 || *p != '\0') {
				fprintf(stderr, USAGE);
				return 1;
			}
			usleep((int)(seconds * 1000.0 * 1000.0));
			return 0;

		default:
			fprintf(stderr, USAGE);
			return 1;
		}
	}
	fprintf(stderr, USAGE);
	return 1;
}

static bool
link_dnsrps(librpz_emsg_t *emsg) {
#ifdef USE_DNSRPS
	librpz = librpz_lib_open(emsg, NULL, LIBRPZ_LIB_OPEN);
	if (librpz == NULL) {
		return false;
	}

	return true;
#else  /* ifdef USE_DNSRPS */
	snprintf(emsg->c, sizeof(emsg->c), "DNSRPS not configured");
	return false;
#endif /* ifdef USE_DNSRPS */
}
