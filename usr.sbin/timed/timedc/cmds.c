/*	$NetBSD: cmds.c,v 1.23 2007/02/04 21:17:01 cbiere Exp $	*/

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
__RCSID("$NetBSD: cmds.c,v 1.23 2007/02/04 21:17:01 cbiere Exp $");
#endif
#endif /* not lint */

#include "timedc.h"
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#define TSPTYPES
#include <protocols/timed.h>

#define	SECHR	(60*60)
#define	SECDAY	(24*SECHR)

int sock_raw;	/* used by measure() */
static int sock;
extern int measure_delta;

void bytenetorder(struct tsp *);
void bytehostorder(struct tsp *);
void set_tsp_name(struct tsp *, const char *);
void get_tsp_name(const struct tsp *, char *, size_t);


#define BU 2208988800UL	/* seconds before UNIX epoch */

enum { CACHED, REFRESH };

static const char *
myname(int refresh)
{
	static char name[MAXHOSTNAMELEN + 1];
	static int initialized;

	if (refresh || !initialized) {
		initialized = 1;
		(void)gethostname(name, sizeof(name));
		name[sizeof(name) - 1] = '\0';
	}
	return name;
}

static in_port_t
udpservport(const char *service)
{
	const struct servent *srvp;

	srvp = getservbyname(service, "udp");
	if (srvp == NULL) {
		warnx("%s/udp: unknown service", service);
		return 0;
	}
	return srvp->s_port;
}

static const char *
getaddr(const char *name, struct sockaddr_in *addr, in_port_t port)
{
	const struct hostent *hp;

	hp = gethostbyname(name);
	if (hp == NULL) {
		warnx("Error resolving %s (%s)", name, hstrerror(h_errno));
		return NULL;
	}
	if (addr) {
		memset(addr, 0, sizeof(*addr));
		addr->sin_family = AF_INET;
		addr->sin_port = port;
		memcpy(&addr->sin_addr.s_addr, hp->h_addr, sizeof(in_addr_t));
	}
	return hp->h_name ? hp->h_name : name;
}

static const char *
tsp_type_to_string(const struct tsp *msg)
{
	unsigned i;

	i = msg->tsp_type;
	return i < TSPTYPENUMBER ? tsptype[i] : "unknown";
}

/* compute the difference between our date and another machine
 */
static int				/* difference in days from our time */
daydiff(const char *hostname, const struct sockaddr_in *addr)
{
	struct pollfd set[1];
	int trials;

	if (connect(sock, (const struct sockaddr *)addr,
		    sizeof(*addr)) == -1) {
		warn("connect");
		return 0;
	}

	set[0].fd = sock;
	set[0].events = POLLIN;
	for (trials = 0; trials < 10; trials++) {
		ssize_t ret;
		uint32_t sec;

		/* ask for the time */
		sec = 0;
		ret = send(sock, &sec, sizeof(sec), 0);
		if (ret < (ssize_t)sizeof(sec)) {
			if (ret < 0) 
				warn("send(sock)");
			else
				warnx("send(sock): incomplete");
			return 0;
		}

		for (;;) {
			int i;

			/* wait 2 seconds between 10 tries */
			i = poll(set, 1, 2000);
			if (i < 0) {
				if (errno == EINTR)
					continue;
				warn("poll(date read)");
				return 0;
			}
			if (0 == i)
				break;

			ret = recv(sock, &sec, sizeof(sec), 0);
			if (ret < (ssize_t)sizeof(sec)) {
				if (ret < 0)
					warn("recv(date read)");
				else
					warnx("recv(date read): incomplete");
				return 0;
			}

			sec = ntohl(sec);
			if (sec < BU) {
				warnx("%s says it is before 1970: %lu",
					hostname, (unsigned long)sec);
				return 0;
			} else {
				struct timeval now;

				sec -= BU;
				(void)gettimeofday(&now, NULL);
				return (sec - now.tv_sec);
			}
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
	extern int measure(u_long, u_long, const char *,
			const struct sockaddr_in*, int);
	in_port_t port;

	if (argc < 2)  {
		printf("Usage: clockdiff host ... \n");
		return;
	}

	(void)myname(REFRESH);

	/* get the address for the date ready */
	port = udpservport("time");

	while (argc > 1) {
		struct sockaddr_in server;
		const char *hostname;
		int measure_status;
		int avg_cnt;
		long avg;

		argc--; argv++;
		if ((hostname = getaddr(*argv, &server, port)) == NULL)
			continue;

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
			printf("%s is down\n", hostname);
			continue;
		case NONSTDTIME:
			printf("%s transmits a non-standard time format\n",
			       hostname);
			continue;
		case UNREACHABLE:
			printf("%s is unreachable\n", hostname);
			continue;
		}

		/*
		 * Try to get the date only after using ICMP timestamps to
		 * get the time.  This is because the date protocol
		 * is optional.
		 */
		if (port != 0) {
			avg = daydiff(*argv, &server);
			if (avg > SECDAY) {
				printf("time on %s is %ld days ahead %s\n",
				       hostname, avg/SECDAY, myname(CACHED));
				continue;
			} else if (avg < -SECDAY) {
				printf("time on %s is %ld days behind %s\n",
				       hostname, -avg/SECDAY, myname(CACHED));
				continue;
			}
		}

		if (measure_delta > 0) {
			printf("time on %s is %d ms. ahead of time on %s\n",
			       hostname, measure_delta, myname(CACHED));
		} else if (measure_delta == 0) {
			printf("%s and %s have the same time\n",
			       hostname, myname(CACHED));
		} else {
			printf("time on %s is %d ms. behind time on %s\n",
			       hostname, -measure_delta, myname(CACHED));
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
	struct pollfd set[1];
	in_port_t port;
	int i;

	if (argc < 1) {
		printf("Usage: msite [hostname]\n");
		return;
	}

	port = udpservport("timed");
	if (port == 0)
		return;

	(void)myname(REFRESH);
	i = 1;
	set[0].fd = sock;
	set[0].events = POLLIN;
	do {
		struct sockaddr_in dest;
		struct tsp msg;
		const char *tgtname;

		tgtname = (i >= argc) ? myname(CACHED) : argv[i];
		if (getaddr(tgtname, &dest, port) == NULL)
			continue;

		if (connect(sock, (const struct sockaddr *)&dest,
		    	    sizeof(dest)) == -1) {
			warn("connect");
			continue;
		}

		set_tsp_name(&msg, myname(CACHED));
		msg.tsp_type = TSP_MSITE;
		msg.tsp_vers = TSPVERSION;
		bytenetorder(&msg);
		if (send(sock, &msg, sizeof(msg), 0) < 0) {
			warn("send");
			continue;
		}

		if (poll(set, 1, 15000)) {
			ssize_t ret;

			ret = recv(sock, &msg, sizeof(msg), 0);
			if (ret < (ssize_t)sizeof(msg)) {
				if (ret < 0)
					warn("recv");
				else
					warnx("recv: incomplete");
				continue;
			}
			bytehostorder(&msg);
			if (msg.tsp_type == TSP_ACK) {
				char name[MAXHOSTNAMELEN];

				get_tsp_name(&msg, name, sizeof(name));
				printf("master timedaemon at %s is %s\n",
				       tgtname, name);
			} else {
				printf("received wrong ack: %s\n",
				       tsp_type_to_string(&msg));
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
	exit(EXIT_SUCCESS);
}


/*
 * Causes the election timer to expire on the selected hosts
 * It sends just one udp message per machine, relying on
 * reliability of communication channel.
 */
void
testing(int argc, char *argv[])
{
	in_port_t port;

	if (argc < 2)  {
		printf("Usage: election host1 [host2 ...]\n");
		return;
	}

	port = udpservport("timed");
	if (port == 0)
		return;

	while (argc > 1) {
		struct sockaddr_in addr;
		struct tsp msg;

		argc--; argv++;
		if (getaddr(*argv, &addr, port) == NULL)
			continue;

		msg.tsp_type = TSP_TEST;
		msg.tsp_vers = TSPVERSION;
		set_tsp_name(&msg, myname(CACHED));
		bytenetorder(&msg);
		if (sendto(sock, &msg, sizeof(msg), 0,
			   (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
			warn("send");
		}
	}
}


/*
 * Enables or disables tracing on local timedaemon
 */
void
tracing(int argc, char *argv[])
{
	struct pollfd set[1];
	struct sockaddr_in dest;
	in_port_t port;
	struct tsp msg;
	int onflag;

	if (argc != 2) {
		printf("Usage: tracing { on | off }\n");
		return;
	}

	port = udpservport("timed");
	if (port == 0)
		return;
	if (getaddr(myname(REFRESH), &dest, port) == NULL)
		return;
	if (connect(sock, (const struct sockaddr *)&dest,
		    sizeof(dest)) == -1) {
		warn("connect");
		return;
	}

	if (strcmp(argv[1], "on") == 0) {
		msg.tsp_type = TSP_TRACEON;
		onflag = ON;
	} else {
		msg.tsp_type = TSP_TRACEOFF;
		onflag = OFF;
	}

	set_tsp_name(&msg, myname(CACHED));
	msg.tsp_vers = TSPVERSION;
	bytenetorder(&msg);
	if (send(sock, &msg, sizeof(msg), 0) < 0) {
		warn("send");
		return;
	}

	set[0].fd = sock;
	set[0].events = POLLIN;
	if (poll(set, 1, 5000)) {
		ssize_t ret;

		ret = recv(sock, &msg, sizeof(msg), 0);
		if (ret < (ssize_t)sizeof(msg)) {
			if (ret < 0)
				warn("recv");
			else
				warnx("recv: incomplete");
			return;
		}
		bytehostorder(&msg);
		if (msg.tsp_type == TSP_ACK)
			printf("timed tracing %s\n",
				onflag ? "enabled" : "disabled");
		else
			printf("wrong ack received: %s\n",
				tsp_type_to_string(&msg));
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
