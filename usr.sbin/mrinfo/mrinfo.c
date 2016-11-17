/*	$NetBSD: mrinfo.c,v 1.30 2016/11/17 09:29:01 shm Exp $	*/

/*
 * This tool requests configuration info from a multicast router
 * and prints the reply (if any).  Invoke it as:
 *
 *	mrinfo router-name-or-address
 *
 * Written Wed Mar 24 1993 by Van Jacobson (adapted from the
 * multicast mapper written by Pavel Curtis).
 *
 * The lawyers insist we include the following UC copyright notice.
 * The mapper from which this is derived contained a Xerox copyright
 * notice which follows the UC one.  Try not to get depressed noting
 * that the legal gibberish is larger than the program.
 *
 * Copyright (c) 1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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
 * ---------------------------------
 * Copyright (c) 1992, 2001 Xerox Corporation.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * Neither name of the Xerox, PARC, nor the names of its contributors may be used
 * to endorse or promote products derived from this software 
 * without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE XEROX CORPORATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char rcsid[] =
    "@(#) Header: mrinfo.c,v 1.6 93/04/08 15:14:16 van Exp (LBL)";
#else
__RCSID("$NetBSD: mrinfo.c,v 1.30 2016/11/17 09:29:01 shm Exp $");
#endif
#endif

#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <poll.h>
#include "defs.h"
#include <arpa/inet.h>
#include <stdarg.h>

#define DEFAULT_TIMEOUT	4	/* How long to wait before retrying requests */
#define DEFAULT_RETRIES 3	/* How many times to ask each router */

u_int32_t	our_addr, target_addr = 0;	/* in NET order */
int     debug = 0;
int	nflag = 0;
int     retries = DEFAULT_RETRIES;
int     timeout = DEFAULT_TIMEOUT;
int	target_level = 0;
vifi_t  numvifs;		/* to keep loader happy */
				/* (see COPY_TABLES macro called in kern.c) */

const char *		inet_name(u_int32_t addr);
void			ask(u_int32_t dst);
void			ask2(u_int32_t dst);
int			get_number(int *var, int deflt, char ***pargv,
				   int *pargc);
u_int32_t		host_addr(char *name);
__dead void			usage(void);

/* logit() prototyped in defs.h */


const char *
inet_name(u_int32_t addr)
{
	struct hostent *e;
	struct in_addr in;

	if (addr == 0)
		return "local";

	if (nflag ||
	    (e = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) == NULL) {
		in.s_addr = addr;
		return (inet_ntoa(in));
	}
	return (e->h_name);
}

/*
 * Log errors and other messages to stderr, according to the severity of the
 * message and the current debug level.  For errors of severity LOG_ERR or
 * worse, terminate the program.
 */
void
logit(int severity, int syserr, const char *format, ...)
{
	va_list ap;

	switch (debug) {
	case 0:
		if (severity > LOG_WARNING)
			return;
		/* FALLTHROUGH */
	case 1:
		if (severity > LOG_NOTICE)
			return;
		/* FALLTHROUGH */
	case 2:
		if (severity > LOG_INFO)
			return;
		/* FALLTHROUGH */
	default:
		if (severity == LOG_WARNING)
			fprintf(stderr, "warning - ");
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		if (syserr == 0)
			fprintf(stderr, "\n");
		else
			fprintf(stderr, ": %s\n", strerror(syserr));
	}

	if (severity <= LOG_ERR)
		exit(1);
}

/*
 * Send a neighbors-list request.
 */
void 
ask(u_int32_t dst)
{
	send_igmp(our_addr, dst, IGMP_DVMRP, DVMRP_ASK_NEIGHBORS,
			htonl(MROUTED_LEVEL), 0);
}

void 
ask2(u_int32_t dst)
{
	send_igmp(our_addr, dst, IGMP_DVMRP, DVMRP_ASK_NEIGHBORS2,
			htonl(MROUTED_LEVEL), 0);
}

/*
 * Process an incoming neighbor-list message.
 */
void 
accept_neighbors(u_int32_t src, u_int32_t dst, u_char *p, int datalen,
		 u_int32_t level)
{
	u_char *ep = p + datalen;
#define GET_ADDR(a) (a = ((u_int32_t)*p++ << 24), a += ((u_int32_t)*p++ << 16),\
		     a += ((u_int32_t)*p++ << 8), a += *p++)

	printf("%s (%s):\n", inet_fmt(src), inet_name(src));
	while (p < ep) {
		u_int32_t laddr;
		u_char metric;
		u_char thresh;
		int ncount;

		GET_ADDR(laddr);
		laddr = htonl(laddr);
		metric = *p++;
		thresh = *p++;
		ncount = *p++;
		while (--ncount >= 0) {
			u_int32_t neighbor;
			GET_ADDR(neighbor);
			neighbor = htonl(neighbor);
			printf("  %s -> ", inet_fmt(laddr));
			printf("%s (%s) [%d/%d]\n",
				inet_fmt(neighbor),
			       inet_name(neighbor), metric, thresh);
		}
	}
}

void 
accept_neighbors2(u_int32_t src, u_int32_t dst, u_char *p, int datalen,
		  u_int32_t level)
{
	u_char *ep = p + datalen;
	u_int broken_cisco = ((level & 0xffff) == 0x020a); /* 10.2 */
	/* well, only possibly_broken_cisco, but that's too long to type. */

	printf("%s (%s) [version %d.%d", inet_fmt(src),
		inet_name(src), level & 0xff, (level >> 8) & 0xff);
	if ((level >> 16) & NF_LEAF)   { printf (",leaf"); }
	if ((level >> 16) & NF_PRUNE)  { printf (",prune"); }
	if ((level >> 16) & NF_GENID)  { printf (",genid"); }
	if ((level >> 16) & NF_MTRACE) { printf (",mtrace"); }
	printf ("]:\n");
	
	while (p < ep) {
		u_char metric;
		u_char thresh;
		u_char flags;
		int ncount;
		u_int32_t laddr = *(u_int32_t*)p;

		p += 4;
		metric = *p++;
		thresh = *p++;
		flags = *p++;
		ncount = *p++;
		if (broken_cisco && ncount == 0)	/* dumb Ciscos */
			ncount = 1;
		if (broken_cisco && ncount > 15)	/* dumb Ciscos */
			ncount = ncount & 0xf;
		while (--ncount >= 0 && p < ep) {
			u_int32_t neighbor = *(u_int32_t*)p;
			p += 4;
			printf("  %s -> ", inet_fmt(laddr));
			printf("%s (%s) [%d/%d",
				inet_fmt(neighbor),
				inet_name(neighbor), metric, thresh);
			if (flags & DVMRP_NF_TUNNEL)
				printf("/tunnel");
			if (flags & DVMRP_NF_SRCRT)
				printf("/srcrt");
			if (flags & DVMRP_NF_PIM)
				printf("/pim");
			if (flags & DVMRP_NF_QUERIER)
				printf("/querier");
			if (flags & DVMRP_NF_DISABLED)
				printf("/disabled");
			if (flags & DVMRP_NF_DOWN)
				printf("/down");
			if (flags & DVMRP_NF_LEAF)
				printf("/leaf");
			printf("]\n");
		}
	}
}

int 
get_number(int *var, int deflt, char ***pargv, int *pargc)
{
	if ((*pargv)[0][2] == '\0') {	/* Get the value from the next
					 * argument */
		if (*pargc > 1 && isdigit((unsigned char)(*pargv)[1][0])) {
			(*pargv)++, (*pargc)--;
			*var = atoi((*pargv)[0]);
			return 1;
		} else if (deflt >= 0) {
			*var = deflt;
			return 1;
		} else
			return 0;
	} else {		/* Get value from the rest of this argument */
		if (isdigit((unsigned char)(*pargv)[0][2])) {
			*var = atoi((*pargv)[0] + 2);
			return 1;
		} else {
			return 0;
		}
	}
}

void
usage(void)
{
	fprintf(stderr,
	    "usage: mrinfo [-n] [-t timeout] [-r retries] [router]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int tries;
	int trynew;
	struct timeval et;
	struct hostent *hp;
	struct hostent bogus;
	const char *host;
	int curaddr;

	if (geteuid() != 0) {
		fprintf(stderr, "mrinfo: must be root\n");
		exit(1);
	}
	init_igmp();
	if (setuid(getuid()) == -1)
		logit(LOG_ERR, errno, "setuid");

	setlinebuf(stderr);

	argv++, argc--;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'd':
			if (!get_number(&debug, DEFAULT_DEBUG, &argv, &argc))
				usage();
			break;
		case 'n':
			++nflag;
			break;
		case 'r':
			if (!get_number(&retries, -1, &argv, &argc))
				usage();
			break;
		case 't':
			if (!get_number(&timeout, -1, &argv, &argc))
				usage();
			break;
		default:
			usage();
		}
		argv++, argc--;
	}
	if (argc > 1)
		usage();
	if (argc == 1)
		host = argv[0];
	else
		host = "127.0.0.1";

	if ((target_addr = inet_addr(host)) != (in_addr_t)-1) {
		hp = &bogus;
		hp->h_length = sizeof(target_addr);
		hp->h_addr_list = (char **)malloc(2 * sizeof(char *));
		if (hp->h_addr_list == NULL)
			logit(LOG_ERR, errno, "malloc");
		hp->h_addr_list[0] = malloc(hp->h_length);
		if (hp->h_addr_list[0] == NULL)
			logit(LOG_ERR, errno, "malloc");
		memcpy(hp->h_addr_list[0], &target_addr, hp->h_length);
		hp->h_addr_list[1] = NULL;
	} else
		hp = gethostbyname(host);

	if (hp == NULL || hp->h_length != sizeof(target_addr)) {
		fprintf(stderr, "mrinfo: %s: no such host\n", argv[0]);
		exit(1);
	}
	if (debug)
		fprintf(stderr, "Debug level %u\n", debug);

	/* Check all addresses; mrouters often have unreachable interfaces */
	for (curaddr = 0; hp->h_addr_list[curaddr] != NULL; curaddr++) {
	    memcpy(&target_addr, hp->h_addr_list[curaddr], sizeof(target_addr));
	    {			/* Find a good local address for us. */
		int     udp;
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
#if (defined(BSD) && (BSD >= 199103))
		addr.sin_len = sizeof addr;
#endif
		addr.sin_addr.s_addr = target_addr;
		addr.sin_port = htons(2000);	/* any port over 1024 will
						 * do... */
		if ((udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0
		|| connect(udp, (struct sockaddr *) & addr, sizeof(addr)) < 0
		    || getsockname(udp, (struct sockaddr *) & addr, &addrlen) < 0) {
			perror("Determining local address");
			exit(1);
		}
		close(udp);
		our_addr = addr.sin_addr.s_addr;
	    }

	    tries = 0;
	    trynew = 1;
	    /*
	     * New strategy: send 'ask2' for two timeouts, then fall back
	     * to 'ask', since it's not very likely that we are going to
	     * find someone who only responds to 'ask' these days
	     */
	    ask2(target_addr);

	    gettimeofday(&et, 0);
	    et.tv_sec += timeout;

	    /* Main receive loop */
	    for (;;) {
		struct pollfd set[1];
		struct timeval tv, now;
		int     count, recvlen;
		socklen_t dummy;
		u_int32_t src, dst, group;
		struct ip *ip;
		struct igmp *igmp;
		int     ipdatalen, iphdrlen, igmpdatalen;

		set[0].fd = igmp_socket;
		set[0].events = POLLIN;

		gettimeofday(&now, 0);
		tv.tv_sec = et.tv_sec - now.tv_sec;
		tv.tv_usec = et.tv_usec - now.tv_usec;

		if (tv.tv_usec < 0) {
			tv.tv_usec += 1000000L;
			--tv.tv_sec;
		}
		if (tv.tv_sec < 0)
			tv.tv_sec = tv.tv_usec = 0;

		count = poll(set, 1, tv.tv_sec * 1000 + tv.tv_usec / 1000);

		if (count < 0) {
			if (errno != EINTR)
				perror("select");
			continue;
		} else if (count == 0) {
			logit(LOG_DEBUG, 0, "Timed out receiving neighbor lists");
			if (++tries > retries)
				break;
			/* If we've tried ASK_NEIGHBORS2 twice with
			 * no response, fall back to ASK_NEIGHBORS
			 */
			if (tries == 2 && target_level == 0)
				trynew = 0;
			if (target_level == 0 && trynew == 0)
				ask(target_addr);
			else
				ask2(target_addr);
			gettimeofday(&et, 0);
			et.tv_sec += timeout;
			continue;
		}
		recvlen = recvfrom(igmp_socket, recv_buf, RECV_BUF_SIZE,
				   0, NULL, &dummy);
		if (recvlen <= 0) {
			if (recvlen && errno != EINTR)
				perror("recvfrom");
			continue;
		}

		if (recvlen < (int)sizeof(struct ip)) {
			logit(LOG_WARNING, 0,
			    "packet too short (%u bytes) for IP header",
			    recvlen);
			continue;
		}
		ip = (struct ip *) recv_buf;
		if (ip->ip_p == 0)
			continue;	/* Request to install cache entry */
		src = ip->ip_src.s_addr;
		dst = ip->ip_dst.s_addr;
		iphdrlen = ip->ip_hl << 2;
		ipdatalen = ip->ip_len;
		if (iphdrlen + ipdatalen != recvlen) {
		    logit(LOG_WARNING, 0,
		      "packet shorter (%u bytes) than hdr+data length (%u+%u)",
		      recvlen, iphdrlen, ipdatalen);
		    continue;
		}
		igmp = (struct igmp *) (recv_buf + iphdrlen);
		group = igmp->igmp_group.s_addr;
		igmpdatalen = ipdatalen - IGMP_MINLEN;
		if (igmpdatalen < 0) {
		    logit(LOG_WARNING, 0,
			"IP data field too short (%u bytes) for IGMP, from %s",
			ipdatalen, inet_fmt(src));
		    continue;
		}
		if (igmp->igmp_type != IGMP_DVMRP)
			continue;

		switch (igmp->igmp_code) {
		case DVMRP_NEIGHBORS:
		case DVMRP_NEIGHBORS2:
			if (src != target_addr) {
				fprintf(stderr, "mrinfo: got reply from %s",
					inet_fmt(src));
				fprintf(stderr, " instead of %s\n",
					inet_fmt(target_addr));
				/*continue;*/
			}
			break;
		default:
			continue;	/* ignore all other DVMRP messages */
		}

		switch (igmp->igmp_code) {

		case DVMRP_NEIGHBORS:
			if (group) {
				/* knows about DVMRP_NEIGHBORS2 msg */
				if (target_level == 0) {
					target_level = ntohl(group);
					ask2(target_addr);
				}
			} else {
				accept_neighbors(src, dst, (u_char *)(igmp + 1),
						 igmpdatalen, ntohl(group));
				exit(0);
			}
			break;

		case DVMRP_NEIGHBORS2:
			accept_neighbors2(src, dst, (u_char *)(igmp + 1),
					  igmpdatalen, ntohl(group));
			exit(0);
		}
	    }
	}
	exit(1);
}

/* dummies */
void accept_probe(u_int32_t src, u_int32_t dst, char *p, int datalen,
		  u_int32_t level)
{
}
void accept_group_report(u_int32_t src, u_int32_t dst, u_int32_t group,
			 int r_type)
{
}
void accept_neighbor_request2(u_int32_t src, u_int32_t dst)
{
}
void accept_report(u_int32_t src, u_int32_t dst, char *p, int datalen,
		   u_int32_t level)
{
}
void accept_neighbor_request(u_int32_t src, u_int32_t dst)
{
}
void accept_prune(u_int32_t src, u_int32_t dst, char *p, int datalen)
{
}
void accept_graft(u_int32_t src, u_int32_t dst, char *p, int datalen)
{
}
void accept_g_ack(u_int32_t src, u_int32_t dst, char *p, int datalen)
{
}
void add_table_entry(u_int32_t origin, u_int32_t mcastgrp)
{
}
void check_vif_state(void)
{
}
void accept_leave_message(u_int32_t src, u_int32_t dst, u_int32_t group)
{
}
void accept_mtrace(u_int32_t src, u_int32_t dst, u_int32_t group, char *data,
		   u_int no, int datalen)
{
}
void accept_membership_query(u_int32_t src, u_int32_t dst, u_int32_t group,
			     int tmo)
{
}
void accept_info_request(u_int32_t src, u_int32_t dst, u_char *p, int datalen)
{
}
void accept_info_reply(u_int32_t src, u_int32_t dst, u_char *p, int datalen)
{
}
