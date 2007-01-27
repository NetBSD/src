/*	$NetBSD: cmds.c,v 1.22 2007/01/27 00:37:56 cbiere Exp $	*/

/*-
 * Copyright (c) 1985, 1993 The Regents of the University of California.
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
static char sccsid[] = "@(#)cmds.c	8.2 (Berkeley) 3/26/95";
#else
__RCSID("$NetBSD: cmds.c,v 1.22 2007/01/27 00:37:56 cbiere Exp $");
#endif
#endif /* not lint */

#include "timedc.h"
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <rpc/rpc.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#define TSPTYPES
#include <protocols/timed.h>

#define	SECHR	(60*60)
#define	SECDAY	(24*SECHR)

# define DATE_PROTO "udp"
# define DATE_PORT "time"


int sock;
int sock_raw;
char myname[MAXHOSTNAMELEN + 1];
struct hostent *hp;
struct sockaddr_in server;
static struct sockaddr_in dayaddr;
extern int measure_delta;

void bytenetorder(struct tsp *);
void bytehostorder(struct tsp *);
void set_tsp_name(struct tsp *, const char *);
void get_tsp_name(const struct tsp *, char *, size_t);


#define BU 2208988800UL	/* seconds before UNIX epoch */


/* compute the difference between our date and another machine
 */
static int				/* difference in days from our time */
daydiff(char *hostname)
{
	int i;
	int trials;
	int tout;
	struct timeval now;
	struct pollfd set[1];
	struct sockaddr_in from;
	socklen_t fromlen;
	uint32_t sec;


	/* wait 2 seconds between 10 tries */
	tout = 2000;
	set[0].fd = sock;
	set[0].events = POLLIN;
	for (trials = 0; trials < 10; trials++) {
		ssize_t ret;

		/* ask for the time */
		sec = 0;
		ret = sendto(sock, &sec, sizeof(sec), 0,
			(struct sockaddr*)&dayaddr, sizeof(dayaddr));
		if (ret < sizeof(sec)) {
			if (ret < 0) 
				warn("sendto(sock)");
			else
				warnx("sendto(sock): incomplete");
			return 0;
		}

		for (;;) {
			i = poll(set, 1, tout);
			if (i < 0) {
				if (errno == EINTR)
					continue;
				warn("poll(date read)");
				return 0;
			}
			if (0 == i)
				break;

			fromlen = sizeof(from);
			ret = recvfrom(sock, &sec, sizeof(sec), 0,
				(struct sockaddr*)&from, &fromlen);
			if (ret >= 0 && (
			    from.sin_family != dayaddr.sin_family ||
			    from.sin_addr.s_addr != dayaddr.sin_addr.s_addr ||
			    from.sin_port != dayaddr.sin_port))
				continue;

			if (ret < sizeof(sec)) {
				if (ret < 0)
					warn("recvfrom(date read)");
				else
					warnx("recvfrom(date read): incomplete");
				return 0;
			}

			sec = ntohl(sec);
			if (sec < BU) {
				warnx("%s says it is before 1970: %lu",
					hostname, (unsigned long)sec);
				return 0;
			}
			sec -= BU;

			(void)gettimeofday(&now, (struct timezone*)0);
			return (sec - now.tv_sec);
		}
	}

	/* if we get here, we tried too many times */
	warnx("%s will not tell us the date", hostname);
	return 0;
}


/*
 * Clockdiff computes the difference between the time of the machine on
 * which it is called and the time of the machines given as argument.
 * The time differences measured by clockdiff are obtained using a sequence
 * of ICMP TSTAMP messages which are returned to the sender by the IP module
 * in the remote machine.
 * In order to compare clocks of machines in different time zones, the time
 * is transmitted (as a 32-bit value) in milliseconds since midnight UT.
 * If a hosts uses a different time format, it should set the high order
 * bit of the 32-bit quantity it transmits.
 * However, VMS apparently transmits the time in milliseconds since midnight
 * local time (rather than GMT) without setting the high order bit.
 * Furthermore, it does not understand daylight-saving time.  This makes
 * clockdiff behaving inconsistently with hosts running VMS.
 *
 * In order to reduce the sensitivity to the variance of message transmission
 * time, clockdiff sends a sequence of messages.  Yet, measures between
 * two `distant' hosts can be affected by a small error. The error can,
 * however, be reduced by increasing the number of messages sent in each
 * measurement.
 */
void
clockdiff(int argc, char *argv[])
{
	int measure_status;
	extern int measure(u_long, u_long, char *, struct sockaddr_in*, int);
	int avg_cnt;
	long avg;
	struct servent *sp;

	if (argc < 2)  {
		printf("Usage: clockdiff host ... \n");
		return;
	}

	(void)gethostname(myname,sizeof(myname));
	myname[sizeof(myname) - 1] = '\0';

	/* get the address for the date ready */
	sp = getservbyname(DATE_PORT, DATE_PROTO);
	if (!sp) {
		warnx("%s/%s is an unknown service", DATE_PORT, DATE_PROTO);
		dayaddr.sin_port = 0;
	} else {
		dayaddr.sin_port = sp->s_port;
	}

	while (argc > 1) {
		argc--; argv++;
		hp = gethostbyname(*argv);
		if (hp == NULL) {
			warnx("Error resolving %s (%s)", *argv,
			    hstrerror(h_errno));
			continue;
		}

		server.sin_family = hp->h_addrtype;
		memcpy(&server.sin_addr.s_addr, hp->h_addr, hp->h_length);
		for (avg_cnt = 0, avg = 0; avg_cnt < 16; avg_cnt++) {
			measure_status = measure(10000,100, *argv, &server, 1);
			if (measure_status != GOOD)
				break;
			avg += measure_delta;
		}
		if (measure_status == GOOD)
			measure_delta = avg/avg_cnt;

		switch (measure_status) {
		case HOSTDOWN:
			printf("%s is down\n", hp->h_name);
			continue;
		case NONSTDTIME:
			printf("%s transmits a non-standard time format\n",
			       hp->h_name);
			continue;
		case UNREACHABLE:
			printf("%s is unreachable\n", hp->h_name);
			continue;
		}

		/*
		 * Try to get the date only after using ICMP timestamps to
		 * get the time.  This is because the date protocol
		 * is optional.
		 */
		if (dayaddr.sin_port != 0) {
			dayaddr.sin_family = hp->h_addrtype;
			memcpy(&dayaddr.sin_addr.s_addr, hp->h_addr,
			      hp->h_length);
			avg = daydiff(*argv);
			if (avg > SECDAY) {
				printf("time on %s is %ld days ahead %s\n",
				       hp->h_name, avg/SECDAY, myname);
				continue;
			} else if (avg < -SECDAY) {
				printf("time on %s is %ld days behind %s\n",
				       hp->h_name, -avg/SECDAY, myname);
				continue;
			}
		}

		if (measure_delta > 0) {
			printf("time on %s is %d ms. ahead of time on %s\n",
			       hp->h_name, measure_delta, myname);
		} else if (measure_delta == 0) {
			printf("%s and %s have the same time\n",
			       hp->h_name, myname);
		} else {
			printf("time on %s is %d ms. behind time on %s\n",
			       hp->h_name, -measure_delta, myname);
		}
	}
	return;
}


/*
 * finds location of master timedaemon
 */
void
msite(int argc, char *argv[])
{
	int cc;
	struct pollfd set[1];
	struct sockaddr_in dest;
	int i;
	socklen_t length;
	struct sockaddr from;
	int tout;
	struct tsp msg;
	struct servent *srvp;
	char *tgtname;

	if (argc < 1) {
		printf("Usage: msite [hostname]\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		warnx("udp/timed: unknown service");
		return;
	}
	dest.sin_port = srvp->s_port;
	dest.sin_family = AF_INET;

	(void)gethostname(myname, sizeof(myname));
	i = 1;
	tout = 15000;
	set[0].fd = sock;
	set[0].events = POLLIN;
	do {
		tgtname = (i >= argc) ? myname : argv[i];
		hp = gethostbyname(tgtname);
		if (hp == 0) {
			warnx("Error resolving %s (%s)", tgtname,
			    hstrerror(h_errno));
			continue;
		}
		memcpy(&dest.sin_addr.s_addr, hp->h_addr, hp->h_length);

		set_tsp_name(&msg, myname);
		msg.tsp_type = TSP_MSITE;
		msg.tsp_vers = TSPVERSION;
		bytenetorder(&msg);
		if (sendto(sock, &msg, sizeof(struct tsp), 0,
			   (struct sockaddr*)&dest,
			   sizeof(struct sockaddr)) < 0) {
			warn("sendto");
			continue;
		}

		if (poll(set, 1, tout)) {
			length = sizeof(struct sockaddr);
			cc = recvfrom(sock, &msg, sizeof(struct tsp), 0,
				      &from, &length);
			if (cc < 0) {
				warn("recvfrom");
				continue;
			}
			bytehostorder(&msg);
			if (msg.tsp_type == TSP_ACK) {
				printf("master timedaemon at %s is %s\n",
				       tgtname, msg.tsp_name);
			} else {
				printf("received wrong ack: %s\n",
				       tsptype[msg.tsp_type]);
			}
		} else {
			printf("communication error with %s\n", tgtname);
		}
	} while (++i < argc);
}

/*
 * quits timedc
 */
void
quit(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	exit(0);
}


/*
 * Causes the election timer to expire on the selected hosts
 * It sends just one udp message per machine, relying on
 * reliability of communication channel.
 */
void
testing(int argc, char *argv[])
{
	struct servent *srvp;
	struct sockaddr_in addr;
	struct tsp msg;

	if (argc < 2)  {
		printf("Usage: election host1 [host2 ...]\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		warnx("udp/timed: unknown service");
		return;
	}

	while (argc > 1) {
		argc--; argv++;
		hp = gethostbyname(*argv);
		if (hp == NULL) {
			warnx("Error resolving %s (%s)", *argv,
			    hstrerror(h_errno));
			argc--; argv++;
			continue;
		}
		addr.sin_port = srvp->s_port;
		addr.sin_family = hp->h_addrtype;
		memcpy(&addr.sin_addr.s_addr, hp->h_addr, hp->h_length);

		msg.tsp_type = TSP_TEST;
		msg.tsp_vers = TSPVERSION;
		(void)gethostname(myname, sizeof(myname));
		set_tsp_name(&msg, myname);
		bytenetorder(&msg);
		if (sendto(sock, &msg, sizeof(struct tsp), 0,
			   (struct sockaddr*)&addr,
			   sizeof(struct sockaddr)) < 0) {
			warn("sendto");
		}
	}
}


/*
 * Enables or disables tracing on local timedaemon
 */
void
tracing(int argc, char *argv[])
{
	int onflag;
	socklen_t length;
	int cc;
	struct pollfd set[1];
	struct sockaddr_in dest;
	struct sockaddr from;
	int tout;
	struct tsp msg;
	struct servent *srvp;

	if (argc != 2) {
		printf("Usage: tracing { on | off }\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		warnx("udp/timed: unknown service");
		return;
	}
	dest.sin_port = srvp->s_port;
	dest.sin_family = AF_INET;

	(void)gethostname(myname,sizeof(myname));
	hp = gethostbyname(myname);
	memcpy(&dest.sin_addr.s_addr, hp->h_addr, hp->h_length);

	if (strcmp(argv[1], "on") == 0) {
		msg.tsp_type = TSP_TRACEON;
		onflag = ON;
	} else {
		msg.tsp_type = TSP_TRACEOFF;
		onflag = OFF;
	}

	set_tsp_name(&msg, myname);
	msg.tsp_vers = TSPVERSION;
	bytenetorder(&msg);
	if (sendto(sock, &msg, sizeof(struct tsp), 0,
		   (struct sockaddr*)&dest, sizeof(struct sockaddr)) < 0) {
		warn("sendto");
		return;
	}

	tout = 5000;
	set[0].fd = sock;
	set[0].events = POLLIN;
	if (poll(set, 1, tout)) {
		length = sizeof(struct sockaddr);
		cc = recvfrom(sock, &msg, sizeof(struct tsp), 0,
			      &from, &length);
		if (cc < 0) {
			warn("recvfrom");
			return;
		}
		bytehostorder(&msg);
		if (msg.tsp_type == TSP_ACK)
			if (onflag)
				printf("timed tracing enabled\n");
			else
				printf("timed tracing disabled\n");
		else
			printf("wrong ack received: %s\n",
						tsptype[msg.tsp_type]);
	} else
		printf("communication error\n");
}

int
priv_resources(void)
{
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		warn("Cannot open UDP socket");
		return -1;
	}

	if ((sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		warn("Cannot open raw socket");
		(void)close(sock);
		return -1;
	}
	return 1;
}
