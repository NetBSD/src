/*	$NetBSD: talkd.c,v 1.17 2003/08/07 09:46:51 agc Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)talkd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: talkd.c,v 1.17 2003/08/07 09:46:51 agc Exp $");
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
long	lastmsgtime;

char	hostname[MAXHOSTNAMELEN + 1];

#define TIMEOUT 30
#define MAXIDLE 120

static void timeout __P((int));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
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

		(void)memcpy(&ctl_addr, &mp->ctl_addr, sizeof(ctl_addr));
		ctl_addr.sa_family = mp->ctl_addr.sa_family;
		ctl_addr.sa_len = sizeof(ctl_addr);
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
timeout(n)
	int n;
{
	int save_errno = errno;

	if (time(0) - lastmsgtime >= MAXIDLE)
		_exit(0);
	alarm(TIMEOUT);
	errno = save_errno;
}
