/*	$NetBSD: btcontrol.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

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
__RCSID("$NetBSD: btcontrol.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <assert.h>
#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btcontrol.h"

const char *config_file = NULL;
const char *control_file = "/dev/bthubctl";

struct command {
	const char	*command;
	int		(*handler)(bdaddr_t *, bdaddr_t *, int, char **);
	const char	*description;
} commands[] = {
	{ "Attach",	dev_attach,	"attach device"		},
	{ "Detach",	dev_detach,	"detach device"		},
	{ "Parse",	hid_parse,	"parse HID descriptor"	},
	{ "Print",	cfg_print,	"print config entry"	},
	{ NULL }
};

static void
usage(void)
{
	struct command *cmd;

	fprintf(stderr,
		"Usage: %s [-c file] [-d device] [-f path] -a bdaddr <command>\n"
		"Where:\n"
		"\t-a bdaddr    remote address\n"
		"\t-c file      the bluetooth config file\n"
		"\t-d device    local device address\n"
		"\t-f path      path of control file\n"
		"\t-h           display usage and quit\n"
		"\n"
		"Commands:\n"
		"", getprogname());

	for (cmd = commands ; cmd->command != NULL ; cmd++)
		fprintf(stderr, "\t%-13s%s\n", cmd->command, cmd->description);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	bdaddr_t	laddr, raddr;
	struct command *cmd;
	int		ch;

	bdaddr_copy(&laddr, BDADDR_ANY);
	bdaddr_copy(&raddr, BDADDR_ANY);

	while ((ch = getopt(argc, argv, "a:c:d:f:h")) != -1) {
		switch (ch) {
		case 'a': /* remote address */
			if (!bt_aton(optarg, &raddr)) {
				struct hostent  *he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					errx(EXIT_FAILURE, "%s: %s",
						optarg, hstrerror(h_errno));

				bdaddr_copy(&raddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'c': /* config file */
			config_file = optarg;
			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &laddr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 'f': /* control file */
			control_file = optarg;
			break;

		case 'h':
		default:
			usage();
			/* NOT REACHED */
		}
	}

	argc -= optind;
	argv += optind;

	optind = 0;
	optreset = 1;

	if (argc == 0)
		usage();

	for (cmd = commands ; cmd->command != NULL; cmd++) {
		if (strcasecmp(*argv, cmd->command) == 0)
			return (cmd->handler)(&laddr, &raddr, --argc, ++argv);
	}

	errx(EXIT_FAILURE, "%s: unknown command", *argv);
}
