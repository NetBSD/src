/*	$NetBSD: sdpquery.c,v 1.4.6.1 2009/05/13 19:20:04 jym Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc.\
 Copyright (c) 2006 Itronix, Inc.\
 All rights reserved.");
__RCSID("$NetBSD: sdpquery.c,v 1.4.6.1 2009/05/13 19:20:04 jym Exp $");

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <sdp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdpquery.h"

static void usage(void);

const char *control_socket;

bdaddr_t local_addr;
bdaddr_t remote_addr;

bool Nflag;	/* numerical output */
bool Rflag;	/* uncooked output */
bool Xflag;	/* hex output */

static struct command {
	const char	*command;
	int		(*handler)(int, char const **);
	const char	*usage;
} commands[] = {
	{ "Browse",	do_sdp_browse,	"[group]"		},
	{ "Record",	do_sdp_record,	"handle [handle...]"	},
	{ "Search",	do_sdp_search,	"uuid [uuid...]"	},
	{ NULL,		NULL,		NULL			}
};

int
main(int argc, char *argv[])
{
	struct command *cmd;
	int		ch, local;

	bdaddr_copy(&local_addr, BDADDR_ANY);
	bdaddr_copy(&remote_addr, BDADDR_ANY);
	control_socket = NULL;
	Nflag = Rflag = Xflag = false;
	local = 0;

	while ((ch = getopt(argc, argv, "a:c:d:hlNRX")) != -1) {
		switch (ch) {
		case 'a': /* remote address */
			if (!bt_aton(optarg, &remote_addr)) {
				struct hostent  *he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					errx(EXIT_FAILURE, "%s: %s",
						optarg, hstrerror(h_errno));

				bdaddr_copy(&remote_addr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'c':
			control_socket = optarg;
			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &local_addr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 'l': /* local sdpd */
			local = 1;
			break;

		case 'N': /* Numerical output */
			Nflag = true;
			break;

		case 'R': /* Raw output */
			Rflag = true;
			break;

		case 'X': /* Hex output */
			Xflag = true;
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

	if (argc < 1
	    || (bdaddr_any(&remote_addr) && !local)
	    || (!bdaddr_any(&remote_addr) && local))
		usage();

	for (cmd = commands ; cmd->command != NULL; cmd++) {
		if (strcasecmp(*argv, cmd->command) == 0)
			return (*cmd->handler)(--argc, (char const **)++argv);
	}

	usage();
	return EXIT_FAILURE;
}

static void
usage(void)
{
	struct command *cmd;

	fprintf(stderr,
		"Usage: %s [-NRX] [-d device] -a bdaddr <command> [parameters..]\n"
		"       %s [-NRX] [-c path] -l <command> [parameters..]\n"
		"\n", getprogname(), getprogname());

	fprintf(stderr,
		"Where:\n"
		"\t-a bdaddr    remote address\n"
		"\t-c path      path to control socket\n"
		"\t-d device    local device address\n"
		"\t-l           query local SDP server daemon\n"
		"\t-N           print numerical values\n"
		"\t-R           print raw attribute values\n"
		"\t-X           print attribute values in hex\n"
		"\n"
		"Commands:\n");

	for (cmd = commands ; cmd->command != NULL ; cmd++)
		fprintf(stderr, "\t%-13s%s\n", cmd->command, cmd->usage);

	exit(EXIT_FAILURE);
}
