/*	$NetBSD: measure.c,v 1.16.34.1 2013/01/16 05:34:12 yamt Exp $	*/

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
static char sccsid[] = "@(#)measure.c	8.2 (Berkeley) 3/26/95";
#else
__RCSID("$NetBSD: measure.c,v 1.16.34.1 2013/01/16 05:34:12 yamt Exp $");
#endif
#endif /* not lint */

#include "globals.h"
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <util.h>
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif

#define MSEC_DAY	(SECDAY*1000)

#define PACKET_IN	1024

#define MSGS		5		/* timestamps to average */
#define TRIALS		10		/* max # of timestamps sent */

extern int sock_raw;

int measure_delta;

extern int in_cksum(const void *, int);

static n_short seqno = 0;

/*
 * Measures the differences between machines' clocks using
 * ICMP timestamp messages.
 */
int					/* status val defined in globals.h */
measure(u_long maxmsec,			/* wait this many msec at most */
	u_long wmsec,			/* msec to wait for an answer */
	const char *hname,
	const struct sockaddr_in *addr,
	int printerr)			/* print complaints on stderr */
{
	socklen_t length;
	int measure_status;
	int rcvcount, trials;
	int count;
	struct pollfd set[1];
	long sendtime, recvtime, histime1, histime2;
	long idelta, odelta, total;
	long min_idelta, min_odelta;
	struct timeval tdone, tcur, ttrans, twait, tout;
	u_char packet[PACKET_IN];
	struct icmp icp;
	struct icmp oicp;
	struct ip ip;

	min_idelta = min_odelta = 0x7fffffff;
	measure_status = HOSTDOWN;
	measure_delta = HOSTDOWN;
	errno = 0;
	trials = 0;

	/* open raw socket used to measure time differences */
	if (sock_raw < 0) {
		sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		if (sock_raw < 0)  {
			syslog(LOG_ERR, "opening raw socket: %m");
			goto quit;
		}
	}
	    
	set[0].fd = sock_raw;
	set[0].events = POLLIN;

	/*
	 * empty the icmp input queue
	 */
	for (;;) {
		if (poll(set, 1, 0)) {
			ssize_t ret;
			length = sizeof(struct sockaddr_in);
			ret = recvfrom(sock_raw, (char *)packet, PACKET_IN, 0,
				      0,&length);
			if (ret < 0)
				goto quit;
			continue;
		}
		break;
	}

	/*
	 * Choose the smallest transmission time in each of the two
	 * directions. Use these two latter quantities to compute the delta
	 * between the two clocks.
	 */

	oicp.icmp_type = ICMP_TSTAMP;
	oicp.icmp_code = 0;
	oicp.icmp_id = getpid();
	oicp.icmp_rtime = 0;
	oicp.icmp_ttime = 0;
	oicp.icmp_seq = seqno;

	(void)gettimeofday(&tdone, 0);
	mstotvround(&tout, maxmsec);
	timeradd(&tdone, &tout, &tdone);	/* when we give up */

	mstotvround(&twait, wmsec);

	tout.tv_sec = 0;
	tout.tv_usec = 0;
	rcvcount = 0;
	while (rcvcount < MSGS) {
		(void)gettimeofday(&tcur, 0);

		/*
		 * keep sending until we have sent the max
		 */
		if (trials < TRIALS) {
			uint32_t otime;

			trials++;
			otime = (tcur.tv_sec % SECDAY) * 1000  
                                            + tcur.tv_usec / 1000;
			oicp.icmp_otime = htonl(otime);
			oicp.icmp_cksum = 0;
			oicp.icmp_cksum = in_cksum(&oicp, sizeof(oicp));

			count = sendto(sock_raw, &oicp, sizeof(oicp), 0,
				       (const struct sockaddr*)addr,
				       sizeof(struct sockaddr));
			if (count < 0) {
				if (measure_status == HOSTDOWN)
					measure_status = UNREACHABLE;
				goto quit;
			}
			oicp.icmp_seq++;

			timeradd(&tcur, &twait, &ttrans);
		} else {
			ttrans = tdone;
		}

		while (rcvcount < trials) {
			ssize_t ret;

			timersub(&ttrans, &tcur, &tout);
			if (tout.tv_sec < 0)
				tout.tv_sec = 0;

			count = poll(set, 1, tout.tv_sec * 1000 + tout.tv_usec / 1000);
			(void)gettimeofday(&tcur, (struct timezone *)0);
			if (count <= 0)
				break;

			length = sizeof(struct sockaddr_in);
			ret = recvfrom(sock_raw, (char *)packet, PACKET_IN, 0,
				      0,&length);
			if (ret < 0)
				goto quit;

			/* 
			 * got something.  See if it is ours
			 */

			if ((size_t)ret < sizeof(ip))
				continue;
			memcpy(&ip, packet, sizeof(ip));
			if ((size_t)ret < (size_t)ip.ip_hl << 2)
				continue;
			ret -= ip.ip_hl << 2;

			memset(&icp, 0, sizeof(icp));
			memcpy(&icp, &packet[ip.ip_hl << 2],
				MIN((size_t)ret, sizeof(icp)));

			if (icp.icmp_type != ICMP_TSTAMPREPLY
			    || icp.icmp_id != oicp.icmp_id
			    || icp.icmp_seq < seqno
			    || icp.icmp_seq >= oicp.icmp_seq)
				continue;

			sendtime = ntohl(icp.icmp_otime);
			recvtime = ((tcur.tv_sec % SECDAY) * 1000 +
				    tcur.tv_usec / 1000);

			total = recvtime - sendtime;
			if (total < 0)	/* do not hassle midnight */
				continue;

			rcvcount++;
			histime1 = ntohl(icp.icmp_rtime);
			histime2 = ntohl(icp.icmp_ttime);
			/*
			 * a host using a time format different from
			 * msec. since midnight UT (as per RFC792) should
			 * set the high order bit of the 32-bit time
			 * value it transmits.
			 */
			if ((histime1 & 0x80000000) != 0) {
				measure_status = NONSTDTIME;
				goto quit;
			}
			measure_status = GOOD;

			idelta = recvtime-histime2;
			odelta = histime1-sendtime;

			/* do not be confused by midnight */
			if (idelta < -MSEC_DAY/2) idelta += MSEC_DAY;
			else if (idelta > MSEC_DAY/2) idelta -= MSEC_DAY;

			if (odelta < -MSEC_DAY/2) odelta += MSEC_DAY;
			else if (odelta > MSEC_DAY/2) odelta -= MSEC_DAY;

			/* save the quantization error so that we can get a
			 * measurement finer than our system clock.
			 */
			if (total < MIN_ROUND) {
				measure_delta = (odelta - idelta)/2;
				goto quit;
			}

			if (idelta < min_idelta)
				min_idelta = idelta;
			if (odelta < min_odelta)
				min_odelta = odelta;

			measure_delta = (min_odelta - min_idelta)/2;
		}

		if (tcur.tv_sec > tdone.tv_sec
		    || (tcur.tv_sec == tdone.tv_sec
			&& tcur.tv_usec >= tdone.tv_usec))
			break;
	}

quit:
	seqno += TRIALS;		/* allocate our sequence numbers */

	/*
	 * If no answer is received for TRIALS consecutive times,
	 * the machine is assumed to be down
	 */
	if (measure_status == GOOD) {
		if (trace) {
			fprintf(fd,
				"measured delta %4d, %d trials to %-15s %s\n",
			   	measure_delta, trials,
				inet_ntoa(addr->sin_addr), hname);
		}
	} else if (printerr) {
		if (errno != 0)
			fprintf(stderr, "measure %s: %s\n", hname,
				strerror(errno));
	} else {
		if (errno != 0) {
			syslog(LOG_ERR, "measure %s: %m", hname);
		} else {
			syslog(LOG_ERR, "measure: %s did not respond", hname);
		}
		if (trace) {
			fprintf(fd,
				"measure: %s failed after %d trials\n",
				hname, trials);
			(void)fflush(fd);
		}
	}

	return(measure_status);
}





/*
 * round a number of milliseconds into a struct timeval
 */
void
mstotvround(struct timeval *res, long x)
{
	if (x < 0)
		x = -((-x + 3)/5);
	else
		x = (x+3)/5;
	x *= 5;

	res->tv_sec = x/1000;
	res->tv_usec = (x-res->tv_sec*1000)*1000;
	if (res->tv_usec < 0) {
		res->tv_usec += 1000000;
		res->tv_sec--;
	}
}

void
update_time(struct timeval *tv, const struct tsp *msg)
{
#ifdef SUPPORT_UTMP
	logwtmp("|", "date", "");
#endif
#ifdef SUPPORT_UTMPX
	logwtmpx("|", "date", "", 0, OLD_TIME);
#endif
	tv->tv_sec = msg->tsp_time.tv_sec;
	tv->tv_usec = msg->tsp_time.tv_usec;
	(void)settimeofday(tv, 0);
#ifdef SUPPORT_UTMP
	logwtmp("}", "date", "");
#endif
#ifdef SUPPORT_UTMPX
	logwtmpx("}", "date", "", 0, NEW_TIME);
#endif
}
