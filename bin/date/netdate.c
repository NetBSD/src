/* $NetBSD: netdate.c,v 1.25 2006/06/14 16:35:16 ginsbach Exp $ */

/*-
 * Copyright (c) 1990, 1993
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
#if 0
static char sccsid[] = "@(#)netdate.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: netdate.c,v 1.25 2006/06/14 16:35:16 ginsbach Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>
#define TSPTYPES
#include <protocols/timed.h>

#include <err.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define	WAITACK		2	/* seconds */
#define	WAITDATEACK	5	/* seconds */

extern int retval;

/*
 * Set the date in the machines controlled by timedaemons by communicating the
 * new date to the local timedaemon.  If the timedaemon is in the master state,
 * it performs the correction on all slaves.  If it is in the slave state, it
 * notifies the master that a correction is needed.
 * Returns 0 on success.  Returns > 0 on failure, setting retval to 2;
 */
int
netsettime(time_t tval)
{
	struct sockaddr_in dest, from, nsin;
	struct tsp msg;
	char hostname[MAXHOSTNAMELEN];
	struct servent *sp;
	struct pollfd ready[1];
	long waittime;
	int error, found, s, timed_ack;
	socklen_t length;
#ifdef IP_PORTRANGE
	int on;
#endif

	if ((sp = getservbyname("timed", "udp")) == NULL) {
		warnx("udp/timed: unknown service");
		return (retval = 2);
	}

	(void)memset(&dest, 0, sizeof(dest));
#ifdef BSD4_4
	dest.sin_len = sizeof(struct sockaddr_in);
#endif
	dest.sin_family = AF_INET;
	dest.sin_port = sp->s_port;
	dest.sin_addr.s_addr = htonl(INADDR_ANY);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno != EAFNOSUPPORT)
			warn("timed");
		return (retval = 2);
	}

#ifdef IP_PORTRANGE
	on = IP_PORTRANGE_LOW;
	if (setsockopt(s, IPPROTO_IP, IP_PORTRANGE, &on, sizeof(on)) < 0) {
		warn("setsockopt");
		goto bad;
	}
#endif

	(void)memset(&nsin, 0, sizeof(nsin));
#ifdef BSD4_4
	nsin.sin_len = sizeof(struct sockaddr_in);
#endif
	nsin.sin_family = AF_INET;
	if (bind(s, (struct sockaddr *)&nsin, sizeof(nsin)) < 0) {
		warn("bind");
		goto bad;
	}

	msg.tsp_type = TSP_SETDATE;
	msg.tsp_vers = TSPVERSION;
	if (gethostname(hostname, sizeof(hostname))) {
		warn("gethostname");
		goto bad;
	}
	hostname[sizeof(hostname) - 1] = '\0';
	(void)strlcpy(msg.tsp_name, hostname, sizeof(msg.tsp_name));
	msg.tsp_seq = htons((u_short)0);
	msg.tsp_time.tv_sec = htonl((u_long)tval);
	msg.tsp_time.tv_usec = htonl((u_long)0);
	length = sizeof(struct sockaddr_in);
	if (connect(s, (struct sockaddr *)&dest, length) < 0) {
		warn("connect");
		goto bad;
	}
	if (send(s, (char *)&msg, sizeof(struct tsp), 0) < 0) {
		if (errno != ECONNREFUSED)
			warn("send");
		goto bad;
	}

	timed_ack = -1;
	waittime = WAITACK;
	ready[0].fd = s;
	ready[0].events = POLLIN;
loop:
	found = poll(ready, 1, waittime * 1000);

	length = sizeof(error);
	if (!getsockopt(s,
	    SOL_SOCKET, SO_ERROR, (char *)&error, &length) && error) {
		if (error != ECONNREFUSED)
			warn("send (delayed error)");
		goto bad;
	}

	if (found > 0 && ready[0].revents & POLLIN) {
		length = sizeof(struct sockaddr_in);
		if (recvfrom(s, &msg, sizeof(struct tsp), 0,
		    (struct sockaddr *)&from, &length) < 0) {
			if (errno != ECONNREFUSED)
				warn("recvfrom");
			goto bad;
		}
		msg.tsp_seq = ntohs(msg.tsp_seq);
		msg.tsp_time.tv_sec = ntohl(msg.tsp_time.tv_sec);
		msg.tsp_time.tv_usec = ntohl(msg.tsp_time.tv_usec);
		switch (msg.tsp_type) {
		case TSP_ACK:
			timed_ack = TSP_ACK;
			waittime = WAITDATEACK;
			goto loop;
		case TSP_DATEACK:
			(void)close(s);
			return (0);
		default:
			warnx("wrong ack received from timed: %s", 
			    tsptype[msg.tsp_type]);
			timed_ack = -1;
			break;
		}
	}
	if (timed_ack == -1)
		warnx("can't reach time daemon, time set locally");

bad:
	(void)close(s);
	return (retval = 2);
}
