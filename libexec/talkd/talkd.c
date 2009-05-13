/*	$NetBSD: talkd.c,v 1.20.6.1 2009/05/13 19:18:43 jym Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)talkd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: talkd.c,v 1.20.6.1 2009/05/13 19:18:43 jym Exp $");
#endif
#endif /* not lint */

/*
 * The top level of the daemon, the format is heavily borrowed
 * from rwhod.c. Basically: find out who and where you are; 
 * disconnect all descriptors and ttys, and then endless
 * loop on waiting for and processing requests
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <protocols/talkd.h>

#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "extern.h"

CTL_MSG		request;
CTL_RESPONSE	response;

int	sockt = STDIN_FILENO;
int	debug = 0;
int	logging = 0;
time_t	lastmsgtime;

char	hostname[MAXHOSTNAMELEN + 1];

#define TIMEOUT 30
#define MAXIDLE 120

static void timeout(int);
int	main(int, char *[]);

int
main(int argc, char *argv[])
{
	CTL_MSG *mp = &request;
	int cc, ch;
	struct sockaddr ctl_addr;

	openlog("talkd", LOG_PID, LOG_DAEMON);
	while ((ch = getopt(argc, argv, "dl")) != -1)
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 'l':
			logging = 1;
			break;
		default:
			syslog(LOG_ERR, "Usage: %s [-dl]", getprogname());
			exit(1);
		}

	if (gethostname(hostname, sizeof hostname) < 0) {
		syslog(LOG_ERR, "gethostname: %m");
		_exit(1);
	}
	hostname[MAXHOSTNAMELEN] = '\0';  /* ensure null termination */
	if (chdir(_PATH_DEV) < 0) {
		syslog(LOG_ERR, "chdir: %s: %m", _PATH_DEV);
		_exit(1);
	}
	signal(SIGALRM, timeout);
	alarm(TIMEOUT);
	for (;;) {
		memset(&response, 0, sizeof(response));
		cc = recv(0, (char *)mp, sizeof (*mp), 0);
		if (cc != sizeof (*mp)) {
			if (cc < 0 && errno != EINTR)
				syslog(LOG_WARNING, "recv: %m");
			continue;
		}

		mp->l_name[sizeof(mp->l_name) - 1] = '\0';
		mp->r_name[sizeof(mp->r_name) - 1] = '\0';
		mp->r_tty[sizeof(mp->r_tty) - 1] = '\0';

		lastmsgtime = time(0);
		process_request(mp, &response);

		tsa2sa(&ctl_addr, &mp->ctl_addr);
		if (ctl_addr.sa_family != AF_INET)
			continue;

		/* can block here, is this what I want? */
		cc = sendto(sockt, (char *)&response, sizeof (response), 0,
		    &ctl_addr, sizeof (ctl_addr));
		if (cc != sizeof (response))
			syslog(LOG_WARNING, "sendto: %m");
	}
}

void
timeout(int n)
{
	int save_errno = errno;

	if (time(0) - lastmsgtime >= MAXIDLE)
		_exit(0);
	alarm(TIMEOUT);
	errno = save_errno;
}

void
tsa2sa(struct sockaddr *sa, const struct talkd_sockaddr *tsa)
{
	(void)memcpy(sa, tsa, sizeof(*tsa));
	sa->sa_len = sizeof(*tsa);
	sa->sa_family = tsa->sa_family;
}
