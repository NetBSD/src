/*	$NetBSD: btdevctl.c,v 1.2 2006/09/10 15:45:56 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2006 Itronix, Inc.\n"
	    "All rights reserved.\n");
__RCSID("$NetBSD: btdevctl.c,v 1.2 2006/09/10 15:45:56 plunky Exp $");

#include <prop/proplib.h>
#include <sys/ioctl.h>

#include <bluetooth.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/bluetooth/btdev.h>

#include "btdevctl.h"

#define BTHUB_PATH		"/dev/bthub"

int main(int, char *[]);
void usage(void);
char *uppercase(const char *);
int bthub_pioctl(unsigned long, prop_dictionary_t);

int
main(int argc, char *argv[])
{
	prop_dictionary_t dev;
	prop_object_t obj;
	bdaddr_t laddr, raddr;
	const char *service;
	int ch, query, verbose, attach, detach;

	bdaddr_copy(&laddr, BDADDR_ANY);
	bdaddr_copy(&raddr, BDADDR_ANY);
	service = NULL;
	query = FALSE;
	verbose = FALSE;
	attach = FALSE;
	detach = FALSE;

	while ((ch = getopt(argc, argv, "Aa:Dd:hqs:v")) != -1) {
		switch (ch) {
		case 'A': /* Attach device */
			attach = TRUE;
			break;

		case 'a': /* remote address */
			if (!bt_aton(optarg, &raddr)) {
				struct hostent  *he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					errx(EXIT_FAILURE, "%s: %s",
						optarg, hstrerror(h_errno));

				bdaddr_copy(&raddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'D': /* Detach device */
			detach = TRUE;
			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &laddr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 'q':
			query = TRUE;
			break;

		case 's': /* service */
			service = uppercase(optarg);
			break;

		case 'v': /* verbose */
			verbose = TRUE;
			break;

		case 'h':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0
	    || (attach == TRUE && detach == TRUE)
	    || bdaddr_any(&laddr)
	    || bdaddr_any(&raddr)
	    || service == NULL)
		usage();

	if (attach == FALSE && detach == FALSE)
		verbose = TRUE;

	dev = db_get(&laddr, &raddr, service);
	if (dev == NULL || query == TRUE) {
		if (verbose == TRUE)
			printf("Performing SDP query for service '%s'..\n", service);

		dev = cfg_query(&laddr, &raddr, service);
		if (dev == NULL)
			errx(EXIT_FAILURE, "%s/%s not found", bt_ntoa(&raddr, NULL), service);

		if (!db_set(dev, &laddr, &raddr, service))
			errx(EXIT_FAILURE, "service store failed");
	}

	/* add binary local-bdaddr */
	obj = prop_data_create_data(&laddr, sizeof(laddr));
	if (obj == NULL || !prop_dictionary_set(dev, BTDEVladdr, obj))
		errx(EXIT_FAILURE, "proplib failure (%s)", BTDEVladdr);

	prop_object_release(obj);

	/* add binary remote-bdaddr */
	obj = prop_data_create_data(&raddr, sizeof(raddr));
	if (obj == NULL || !prop_dictionary_set(dev, BTDEVraddr, obj))
		errx(EXIT_FAILURE, "proplib failure (%s)", BTDEVraddr);

	prop_object_release(obj);

	if (verbose == TRUE)
		cfg_print(dev);

	if (attach == TRUE)
		bthub_pioctl(BTDEV_ATTACH, dev);

	if (detach == TRUE)
		bthub_pioctl(BTDEV_DETACH, dev);

	exit(EXIT_SUCCESS);
}

void
usage(void)
{

	fprintf(stderr,
		"usage: %s [-A | -D] [-qv] -a address -d device -s service\n"
		"Where:\n"
		"\t-A           attach device\n"
		"\t-a address   remote device address\n"
		"\t-D           detach device\n"
		"\t-d device    local device address\n"
		"\t-q           force SDP query\n"
		"\t-s service   remote service\n"
		"\t-v           verbose\n"
		"", getprogname());

	exit(EXIT_FAILURE);
}

char *
uppercase(const char *arg)
{
	char *str, *ptr;

	str = strdup(arg);
	if (str == NULL)
		err(EXIT_FAILURE, "strdup");

	for (ptr = str ; *ptr ; ptr++)
		*ptr = (char)toupper((int)*ptr);

	return str;
}

int
bthub_pioctl(unsigned long cmd, prop_dictionary_t dict)
{
	int fd;

	fd = open(BTHUB_PATH, O_WRONLY, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", BTHUB_PATH);

	if (prop_dictionary_send_ioctl(dict, fd, cmd))
		err(EXIT_FAILURE, "%s", BTHUB_PATH);

	close(fd);
	return EXIT_SUCCESS;
}
