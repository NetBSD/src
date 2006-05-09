/*	$NetBSD: mount_portal.c,v 1.29 2006/05/09 20:18:08 mrg Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_portal.c	8.6 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_portal.c,v 1.29 2006/05/09 20:18:08 mrg Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/mount.h>

#include <err.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mntopts.h>
#include "pathnames.h"
#include "portald.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	{ NULL }
};

static char mountpt[MAXPATHLEN];  /* made available to signal handler */

static	void	sigchld(int);
static	void	sighup(int);
static	void	sigterm(int);
static	void	usage(void);

static sig_atomic_t readcf;	/* Set when SIGHUP received */

static void
sigchld(int sig)
{
	pid_t pid;

	while ((pid = waitpid((pid_t) -1, (int *) 0, WNOHANG)) > 0)
		;
	if (pid < 0 && errno != ECHILD)
		syslog(LOG_WARNING, "waitpid: %m");
}

static void
sighup(int sig)
{

	readcf = 1;
}

static void
sigterm(int sig)
{

	if (unmount(mountpt, MNT_FORCE) < 0)
		syslog(LOG_WARNING, "sigterm: unmounting %s failed: %m",
		    mountpt);
}

int
main(int argc, char *argv[])
{
	struct portal_args args;
	struct sockaddr_un un;
	char *conf;
	int mntflags = 0;
	char tag[32];
	char tmpdir[PATH_MAX];
	mntoptparse_t mp;

	qelem q;
	int rc;
	int so;
	int error = 0;

	/*
	 * Crack command line args
	 */
	int ch;

	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		default:
			error = 1;
			break;
		}
	}

	if (optind != (argc - 2))
		error = 1;

	if (error)
		usage();

	/*
	 * Get config file and mount point
	 */
	conf = argv[optind];
	if (realpath(argv[optind+1], mountpt) == NULL) /* Check device path */
		err(1, "realpath %s", argv[optind+1]);
	if (strncmp(argv[optind+1], mountpt, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[optind+1]);
		warnx("using \"%s\" instead.", mountpt);
	}

	/*
	 * If configuration file is not specified with an
	 * absolute pathname, complain and die.  We want to
	 * check for this before we call daemon() and lose
	 * access to stderr.
	 */
	if (conf[0] != '/') {
	  errx(-1, "Error:  the configuration file must be specified as an\n"
	      "absolute path, as the daemon chdir's to / immediately.");
	}
	/*
	 * Construct the listening socket
	 */
	un.sun_family = AF_LOCAL;
	strlcpy(tmpdir, _PATH_TMPPORTAL, sizeof(tmpdir));
	mkdtemp(tmpdir);
	un.sun_len = snprintf(un.sun_path, sizeof(un.sun_path),
			"%s/%s", tmpdir, _PATH_PORTAL_FILE);
	if (un.sun_len >= sizeof(un.sun_path))
		errx(1, "portal socket name too long");

	so = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (so < 0)
		err(1, "socket");
	if (bind(so, (struct sockaddr *) &un, sizeof(un)) < 0)
		err(1, "bind() call");

	/* path no longer needed */
	(void) unlink(un.sun_path);
	if (rmdir(tmpdir) != 0)
		warn("failed to rmdir(%s)", tmpdir);

	(void) listen(so, 5);

	/*
	 * Now it's good time to fork - all but the actual mount(2)
	 * call is performed, and we need the new pid to fill in
	 * the tag contents correctly. Since we need to print error message
	 * in case mount(2) fails or DEBUG case, we don't let daemon(3) to
	 * close the standard streams and handle them on our own later.
	 */
	daemon(0, 1);

	args.pa_socket = so;
	snprintf(tag, sizeof(tag), "portal:%d", getpid());
	args.pa_config = tag;

	rc = mount(MOUNT_PORTAL, mountpt, mntflags, &args);
	if (rc < 0)
		err(1, "mount attempt on %s", mountpt);

	/* mount(2) call succeeded, redirect standard streams to /dev/null */
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
#ifndef DEBUG
	freopen("/dev/null", "w", stderr);
#endif

	/*
	 * Start logging (and change name)
	 */
	openlog("portald", LOG_PID, LOG_DAEMON);

	q.q_forw = q.q_back = &q;
	readcf = 1;

	signal(SIGCHLD, sigchld);
	signal(SIGHUP, sighup);
	signal(SIGTERM, sigterm);

	/*
	 * Just loop waiting for new connections and activating them
	 */
	for (;;) {
		struct sockaddr_un un2;
		socklen_t len2 = sizeof(un2);
		int so2;
		pid_t pid;
		struct pollfd fdset[1];

		/*
		 * Check whether we need to re-read the configuration file
		 */
		if (readcf) {
			readcf = 0;
			conf_read(&q, conf);
			continue;
		}
	
		/*
		 * Accept a new connection
		 * Will get EINTR if a signal has arrived, so just
		 * ignore that error code
		 */
		fdset[0].fd = so;
		fdset[0].events = POLLIN;
		rc = poll(fdset, 1, INFTIM);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "poll: %m");
			exit(1);
		}
		if (rc == 0)
			break;
		so2 = accept(so, (struct sockaddr *) &un2, &len2);
		if (so2 < 0) {
			/*
			 * The unmount function does a shutdown on the socket
			 * which will generated ECONNABORTED on the accept.
			 */
			if (errno == ECONNABORTED)
				break;
			if (errno != EINTR) {
				syslog(LOG_ERR, "accept: %m");
				exit(1);
			}
			continue;
		}

		/*
		 * Now fork a new child to deal with the connection
		 */
	eagain:;
		switch (pid = fork()) {
		case -1:
			if (errno == EAGAIN) {
				sleep(1);
				goto eagain;
			}
			syslog(LOG_WARNING, "fork: %m");
			break;
		case 0:
			(void) close(so);
			activate(&q, so2);
			exit(0);
		default:
			(void) close(so2);
			break;
		}
	}
	exit(0);
}

static void
usage(void)
{

	(void)fprintf(stderr,
		"usage: mount_portal [-o options] config mount-point\n");
	exit(1);
}
