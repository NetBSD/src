/*	$NetBSD: main.c,v 1.7 2009/08/30 19:24:40 plunky Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.sbin/bluetooth/sdpd/main.c,v 1.1 2004/01/20 20:48:26 emax Exp $
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc.\
  Copyright (c) 2006 Itronix, Inc.\
  Copyright (c) 2004 Maksim Yevmenkin m_evmenkin@yahoo.com.\
  All rights reserved.");
__RCSID("$NetBSD: main.c,v 1.7 2009/08/30 19:24:40 plunky Exp $");

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdpd.h"

#define	SDPD			"sdpd"

static bool	drop_root	(char const *user, char const *group);
static void	sighandler	(int s);
static void	usage		(void);

static bool	done;

/*
 * Bluetooth Service Discovery Procotol (SDP) daemon
 */

int
main(int argc, char *argv[])
{
	server_t		 server;
	char const		*control = SDP_LOCAL_PATH;
	char const		*user = "_sdpd", *group = "_sdpd";
	char const		*sgroup = NULL;
	int			 opt;
	bool			 detach = true;
	struct sigaction	 sa;

	while ((opt = getopt(argc, argv, "c:dG:g:hu:")) != -1) {
		switch (opt) {
		case 'c': /* control */
			control = optarg;
			break;

		case 'd': /* do not detach */
			detach = false;
			break;

		case 'G': /* super group */
			sgroup = optarg;
			break;

		case 'g': /* group */
			group = optarg;
			break;

		case 'u': /* user */
			user = optarg;
			break;

		case 'h':
		default:
			usage();
			/* NOT REACHED */
		}
	}

	log_open(SDPD, !detach);

	/* Become daemon if required */
	if (detach && daemon(0, 0) < 0) {
		log_crit("Could not become daemon. %s (%d)",
		    strerror(errno), errno);

		exit(EXIT_FAILURE);
	}

	/* Set signal handlers */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sighandler;

	if (sigaction(SIGTERM, &sa, NULL) < 0
	    || sigaction(SIGHUP,  &sa, NULL) < 0
	    || sigaction(SIGINT,  &sa, NULL) < 0) {
		log_crit("Could not install signal handlers. %s (%d)",
		    strerror(errno), errno);

		exit(EXIT_FAILURE);
	}

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) < 0) {
		log_crit("Could not install signal handlers. %s (%d)",
		    strerror(errno), errno);

		exit(EXIT_FAILURE);
	}

	/* Initialize server */
	if (!server_init(&server, control, sgroup))
		exit(EXIT_FAILURE);

	if ((user != NULL || group != NULL) && !drop_root(user, group))
		exit(EXIT_FAILURE);

	for (done = 0; !done; ) {
		if (!server_do(&server))
			done++;
	}

	server_shutdown(&server);
	log_close();

	exit(EXIT_SUCCESS);
}

/*
 * Drop root
 */

static bool
drop_root(char const *user, char const *group)
{
	int	 uid, gid;
	char	*ep;

	if ((uid = getuid()) != 0) {
		log_notice("Cannot set uid/gid. Not a superuser");
		return true; /* dont do anything unless root */
	}

	gid = getgid();

	if (user != NULL) {
		uid = strtol(user, &ep, 10);
		if (*ep != '\0') {
			struct passwd	*pwd = getpwnam(user);

			if (pwd == NULL) {
				log_err("No passwd entry for user %s", user);
				return false;
			}

			uid = pwd->pw_uid;
		}
	}

	if (group != NULL) {
		gid = strtol(group, &ep, 10);
		if (*ep != '\0') {
			struct group	*grp = getgrnam(group);

			if (grp == NULL) {
				log_err("No group entry for group %s", group);
				return false;
			}

			gid = grp->gr_gid;
		}
	}

	if (setgid(gid) < 0) {
		log_err("Could not setgid(%s). %s (%d)", group,
		    strerror(errno), errno);

		return false;
	}

	if (setgroups(0, NULL) < 0) {
		log_err("Could not setgroups(0). %s (%d)",
		    strerror(errno), errno);

		return false;
	}

	if (setuid(uid) < 0) {
		log_err("Could not setuid(%s). %s (%d)", user,
		    strerror(errno), errno);

		return false;
	}

	return true;
}

/*
 * Signal handler
 */

static void
sighandler(int s)
{

	log_notice("Got signal %d. Total number of signals received %d",
		s, ++done);
}

/*
 * Display usage information and quit
 */

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [options]\n"
			"Where options are:\n"
			"\t-c       specify control socket name (default %s)\n"
			"\t-d       do not detach (run in foreground)\n"
			"\t-G grp   allow privileges to group\n"
			"\t-g grp   specify group\n"
			"\t-h       display usage and exit\n"
			"\t-u usr   specify user\n"
			"", SDPD, SDP_LOCAL_PATH);

	exit(EXIT_FAILURE);
}
