/*	$NetBSD: dnsrps.c,v 1.3.2.2 2019/06/10 22:04:01 christos Exp $	*/

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

/*
 * -a		exit(0) if dnsrps is available or dlopen() msg if not
 * -p		print the path to dnsrpzd configured in dnsrps so that
 *		    dnsrpzd can be run by a setup.sh script.
 *		    Exit(1) if dnsrps is not available
 * -n domain	print the serial number of a domain to check if a new
 *		    version of a policy zone has been transferred to dnsrpzd.
 *		    Exit(1) if dnsrps is not available
 * -w sec.ond	wait for seconds, because `sleep 0.1` is not portable
 */

#include <config.h>

#include <inttypes.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <isc/print.h>
#include <isc/util.h>

#ifdef USE_DNSRPS
#define LIBRPZ_LIB_OPEN DNSRPS_LIB_OPEN
#include <dns/librpz.h>

librpz_t *librpz;
#else
typedef struct {char c[120];} librpz_emsg_t;
#endif


static bool link_dnsrps(librpz_emsg_t *emsg);


#define USAGE "usage: [-ap] [-n domain] [-w sec.onds]\n"

int
main(int argc, char **argv) {
#ifdef USE_DNSRPS
	char cstr[sizeof("zone ")+1024+10];
	librpz_clist_t *clist;
	librpz_client_t *client;
	librpz_rsp_t *rsp;
	uint32_t serial;
#endif
	double seconds;
	librpz_emsg_t emsg;
	char *p;
	int i;

	while ((i = getopt(argc, argv, "apn:w:")) != -1) {
		switch (i) {
		case 'a':
			if (!link_dnsrps(&emsg)) {
				printf("I:%s\n", emsg.c);
				return (1);
			}
			return (0);

		case 'p':
			if (!link_dnsrps(&emsg)) {
				fprintf(stderr, "## %s\n", emsg.c);
				return (1);
			}
#ifdef USE_DNSRPS
			printf("%s\n", librpz->dnsrpzd_path);
#else
			INSIST(0);
			ISC_UNREACHABLE();
#endif
			return (0);

		case 'n':
			if (!link_dnsrps(&emsg)) {
				fprintf(stderr, "## %s\n", emsg.c);
				return (1);
			}
#ifdef USE_DNSRPS
			/*
			 * Get the serial number of a policy zone from
			 * a running dnsrpzd daemon.
			 */
			clist = librpz->clist_create(&emsg, NULL, NULL,
						     NULL, NULL, NULL);
			if (clist == NULL) {
				fprintf(stderr, "## %s: %s\n", optarg, emsg.c);
				return (1);
			}
			snprintf(cstr, sizeof(cstr),
				 "zone %s; dnsrpzd \"\";"
				 " dnsrpzd-sock dnsrpzd.sock;"
				 " dnsrpzd-rpzf dnsrpzd.rpzf",
				 optarg);
			client = librpz->client_create(&emsg, clist,
						       cstr, true);
			if (client == NULL) {
				fprintf(stderr, "## %s\n", emsg.c);
				return (1);
			}

			rsp = NULL;
			if (!librpz->rsp_create(&emsg, &rsp, NULL,
						client, true, false) ||
			    rsp == NULL) {
				fprintf(stderr, "## %s\n", emsg.c);
				librpz->client_detach(&client);
				return (1);
			}

			if (!librpz->soa_serial(&emsg, &serial, optarg, rsp)) {
				fprintf(stderr, "## %s\n", emsg.c);
				librpz->client_detach(&client);
				return (1);
			}
			librpz->rsp_detach(&rsp);
			librpz->client_detach(&client);
			printf("%u\n", serial);
#else
			INSIST(0);
			ISC_UNREACHABLE();
#endif
			return (0);

		case 'w':
			seconds = strtod(optarg, &p);
			if (seconds <= 0 || *p != '\0') {
				fputs(USAGE, stderr);
				return (1);
			}
			usleep((int)(seconds*1000.0*1000.0));
			return (0);

		default:
			fputs(USAGE, stderr);
			return (1);
		}
	}
	fputs(USAGE, stderr);
	return (1);
}


static bool
link_dnsrps(librpz_emsg_t *emsg) {
#ifdef USE_DNSRPS
	librpz = librpz_lib_open(emsg, NULL, DNSRPS_LIBRPZ_PATH);
	if (librpz == NULL)
		return (false);

	return (true);
#else
	snprintf(emsg->c, sizeof(emsg->c), "DNSRPS not configured");
	return (false);
#endif
}
