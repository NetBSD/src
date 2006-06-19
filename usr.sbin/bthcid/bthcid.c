/*	$NetBSD: bthcid.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
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
	    "@(#) Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>\n"
	    "All rights reserved.\n");
__RCSID("$NetBSD: bthcid.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <sys/param.h>
#include <sys/stat.h>
#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <event.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include "bthcid.h"

const	char	*key_file = "/var/db/bthcid.keys";
const	char	*config_file = NULL;
const	char	*socket_name = BTHCID_SOCKET_NAME;
	int	 detach = 1;

static struct event	sighup_ev;
static struct event	sigint_ev;
static struct event	sigterm_ev;

static void	process_signal(int, short, void *);
static void	usage(void);

int
main(int argc, char *argv[])
{
	bdaddr_t	bdaddr;
	int		ch;
	mode_t		mode;

	bdaddr_copy(&bdaddr, BDADDR_ANY);
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	while ((ch = getopt(argc, argv, "c:d:fm:ns:h")) != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;

		case 'd':
			if (!bt_devaddr(optarg, &bdaddr))
				err(EXIT_FAILURE, "%s", optarg);
			break;

		case 'f':
			detach = 0;
			break;

		case 'm':
			mode = atoi(optarg);
			break;

		case 'n':
			socket_name = NULL;
			break;

		case 's':
			socket_name = optarg;
			break;

		case 'h':
		default:
			usage();
			/* NOT REACHED */
		}
	}

	if (getuid() != 0)
		errx(EXIT_FAILURE,
		    "** ERROR: You should run %s as privileged user!",
		    getprogname());

	if (detach)
		if (daemon(0, 0) < 0)
			err(EXIT_FAILURE, "Could not daemon()ize");

	openlog(getprogname(), LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_DAEMON);

	event_init();

	signal_set(&sigterm_ev, SIGTERM, process_signal, NULL);
	if (signal_add(&sigterm_ev, NULL) < 0) {
		syslog(LOG_ERR, "signal_add(sigterm_ev)");
		exit(EXIT_FAILURE);
	}

	signal_set(&sigint_ev, SIGINT, process_signal, NULL);
	if (signal_add(&sigint_ev, NULL) < 0) {
		syslog(LOG_ERR, "signal_add(sigint_ev)");
		exit(EXIT_FAILURE);
	}

	signal_set(&sighup_ev, SIGHUP, process_signal, NULL);
	if (signal_add(&sighup_ev, NULL) < 0) {
		syslog(LOG_ERR, "signal_add(sighup_ev)");
		exit(EXIT_FAILURE);
	}

	if (init_hci(&bdaddr) < 0) {
		syslog(LOG_ERR, "init_hci(%s)", bt_ntoa(&bdaddr, NULL));
		exit(EXIT_FAILURE);
	}

	if (init_control(socket_name, mode) < 0) {
		syslog(LOG_ERR, "init_control(%s)", socket_name);
		exit(EXIT_FAILURE);
	}

	if (detach && pidfile(NULL) < 0) {
		syslog(LOG_ERR, "Could not create PID file. %s (%d)",
				strerror(errno), errno);

		exit(EXIT_FAILURE);
	}

	event_dispatch();

	/* NOTREACHED */
	/* gcc fodder */
	exit(EXIT_FAILURE);
}

static void
process_signal(int s, short e, void *arg)
{

	syslog(LOG_DEBUG, "Exiting on signal %d", s);

	if (socket_name)
		unlink(socket_name);

	closelog();
	exit(EXIT_FAILURE);

}

/* Display usage and exit */
static void
usage(void)
{

	fprintf(stderr,
	    "Usage: %s [-fhn] [-c config] [-d devaddr] [-m mode] [-s path]\n"
	    "Where:\n"
	    "\t-c config   specify config filename\n"
	    "\t-d device   specify device address\n"
	    "\t-f          run in foreground\n"
	    "\t-m mode     specify socket permissions\n"
	    "\t-n          do not listen for clients\n"
	    "\t-s path     specify client socket pathname\n"
	    "\t-h          display this message\n",
	    getprogname());

	exit(EXIT_FAILURE);
}
