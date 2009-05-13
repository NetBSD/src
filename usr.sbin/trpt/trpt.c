/*	$NetBSD: trpt.c,v 1.25.6.1 2009/05/13 19:20:43 jym Exp $	*/

/*-
 * Copyright (c) 1997, 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1988, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)trpt.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: trpt.c,v 1.25.6.1 2009/05/13 19:20:43 jym Exp $");
#endif
#endif /* not lint */

#define _CALLOUT_PRIVATE	/* for defs in sys/callout.h */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#define PRUREQUESTS
#include <sys/protosw.h>
#include <sys/file.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet/ip6.h>
#endif

#include <netinet/tcp.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#define	TANAMES
#include <netinet/tcp_debug.h>

#include <arpa/inet.h>

#include <err.h>
#include <stdio.h>
#include <errno.h>
#include <kvm.h>
#include <nlist.h>
#include <paths.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

struct nlist nl[] = {
#define	N_HARDCLOCK_TICKS	0
	{ "_hardclock_ticks", 0, 0, 0, 0 },
#define	N_TCP_DEBUG		1
	{ "_tcp_debug", 0, 0, 0, 0 },
#define	N_TCP_DEBX		2
	{ "_tcp_debx", 0, 0, 0, 0 },
	{ NULL, 0, 0, 0, 0 },
};

static caddr_t tcp_pcbs[TCP_NDEBUG];
static n_time ntime;
static int aflag, follow, sflag, tflag;

/* see sys/netinet/tcp_debug.c */
struct  tcp_debug tcp_debug[TCP_NDEBUG];
int tcp_debx;

int	main(int, char *[]);
void	dotrace(caddr_t);
void	tcp_trace(short, short, struct tcpcb *, struct tcpcb *,
	    int, void *, int);
int	numeric(const void *, const void *);
void	usage(void);

kvm_t	*kd;
int     use_sysctl;

int
main(int argc, char *argv[])
{
	int ch, i, jflag, npcbs;
	char *kernel, *core, *cp, errbuf[_POSIX2_LINE_MAX];
	unsigned long l;

	jflag = npcbs = 0;
	kernel = core = NULL;

	while ((ch = getopt(argc, argv, "afjp:stN:M:")) != -1) {
		switch (ch) {
		case 'a':
			++aflag;
			break;
		case 'f':
			++follow;
			setlinebuf(stdout);
			break;
		case 'j':
			++jflag;
			break;
		case 'p':
			if (npcbs >= TCP_NDEBUG)
				errx(1, "too many pcbs specified");
			errno = 0;
			cp = NULL;
			l = strtoul(optarg, &cp, 16);
			tcp_pcbs[npcbs] = (caddr_t)l;
			if (*optarg == '\0' || *cp != '\0' || errno ||
			    (unsigned long)tcp_pcbs[npcbs] != l)
				errx(1, "invalid address: %s", optarg);
			npcbs++;
			break;
		case 's':
			++sflag;
			break;
		case 't':
			++tflag;
			break;
		case 'N':
			kernel = optarg;
			break;
		case 'M':
			core = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	use_sysctl = (kernel == NULL && core == NULL);

	if (use_sysctl) {
		size_t lenx = sizeof(tcp_debx);
		size_t lend = sizeof(tcp_debug);

		if (sysctlbyname("net.inet.tcp.debx", &tcp_debx, &lenx, 
		    NULL, 0) == -1)
			err(1, "net.inet.tcp.debx");
		if (sysctlbyname("net.inet.tcp.debug", &tcp_debug, &lend,
		    NULL, 0) == -1)
			err(1, "net.inet.tcp.debug");
	} else {
		kd = kvm_openfiles(kernel, core, NULL, O_RDONLY, errbuf);
		if (kd == NULL)
			errx(1, "can't open kmem: %s", errbuf);

		if (kvm_nlist(kd, nl))
			errx(2, "%s: no namelist", kernel);

		if (kvm_read(kd, nl[N_TCP_DEBX].n_value, (char *)&tcp_debx,
		    sizeof(tcp_debx)) != sizeof(tcp_debx))
			errx(3, "tcp_debx: %s", kvm_geterr(kd));

		if (kvm_read(kd, nl[N_TCP_DEBUG].n_value, (char *)tcp_debug,
		    sizeof(tcp_debug)) != sizeof(tcp_debug))
			errx(3, "tcp_debug: %s", kvm_geterr(kd));
	}

	/*
	 * If no control blocks have been specified, figure
	 * out how many distinct one we have and summarize
	 * them in tcp_pcbs for sorting the trace records
	 * below.
	 */
	if (npcbs == 0) {
		for (i = 0; i < TCP_NDEBUG; i++) {
			struct tcp_debug *td = &tcp_debug[i];
			int j;

			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbs[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbs[npcbs++] = td->td_tcb;
		}
		if (npcbs == 0)
			exit(0);
	}
	qsort(tcp_pcbs, npcbs, sizeof(caddr_t), numeric);
	if (jflag) {
		for (i = 0;;) {
			printf("%lx", (long)tcp_pcbs[i]);
			if (++i == npcbs)
				break;
			fputs(", ", stdout);
		}
		putchar('\n');
	} else {
		for (i = 0; i < npcbs; i++) {
			printf("\n%lx:\n", (long)tcp_pcbs[i]);
			dotrace(tcp_pcbs[i]);
		}
	}
	exit(0);
}

void
dotrace(caddr_t tcpcb)
{
	struct tcp_debug *td;
	int prev_debx = tcp_debx;
	int i;

 again:
	if (--tcp_debx < 0)
		tcp_debx = TCP_NDEBUG - 1;
	for (i = prev_debx % TCP_NDEBUG; i < TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		switch (td->td_family) {
		case AF_INET:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, &td->td_ti, td->td_req);
			break;
#ifdef INET6
		case AF_INET6:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, &td->td_ti6, td->td_req);
			break;
#endif
		default:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, NULL, td->td_req);
			break;
		}
		if (i == tcp_debx)
			goto done;
	}
	for (i = 0; i <= tcp_debx % TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		switch (td->td_family) {
		case AF_INET:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, &td->td_ti, td->td_req);
			break;
#ifdef INET6
		case AF_INET6:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, &td->td_ti6, td->td_req);
			break;
#endif
		default:
			tcp_trace(td->td_act, td->td_ostate,
			    (struct tcpcb *)td->td_tcb, &td->td_cb,
			    td->td_family, NULL, td->td_req);
			break;
		}
	}
 done:
	if (follow) {
		prev_debx = tcp_debx + 1;
		if (prev_debx >= TCP_NDEBUG)
			prev_debx = 0;
		do {
			sleep(1);
			if (use_sysctl) {
				size_t len = sizeof(tcp_debx);

				if (sysctlbyname("net.inet.tcp.debx", 
				    &tcp_debx, &len, NULL, 0) == -1)
					err(1, "net.inet.tcp.debx");
			} else
				if (kvm_read(kd, nl[N_TCP_DEBX].n_value,
				    (char *)&tcp_debx, sizeof(tcp_debx)) !=
				    sizeof(tcp_debx))
					errx(3, "tcp_debx: %s", 
					    kvm_geterr(kd));
		} while (tcp_debx == prev_debx);

		if (use_sysctl) {
			size_t len = sizeof(tcp_debug);

			if (sysctlbyname("net.inet.tcp.debug", &tcp_debug, 
			    &len, NULL, 0) == -1)
				err(1, "net.inet.tcp.debug");
		} else
			if (kvm_read(kd, nl[N_TCP_DEBUG].n_value, 
			    (char *)tcp_debug, 
			    sizeof(tcp_debug)) != sizeof(tcp_debug))
				errx(3, "tcp_debug: %s", kvm_geterr(kd));

		goto again;
	}
}

/*
 * Tcp debug routines
 */
/*ARGSUSED*/
void
tcp_trace(short act, short ostate, struct tcpcb *atp, struct tcpcb *tp,
    int family, void *packet, int req)
{
	tcp_seq seq, ack;
	int flags, len, win, timer;
	struct tcphdr *th = NULL;
	struct ip *ip = NULL;
#ifdef INET6
	struct ip6_hdr *ip6 = NULL;
#endif
	callout_impl_t *ci;
	char hbuf[MAXHOSTNAMELEN];

	len = 0;	/* XXXGCC -Wuninitialized */

	switch (family) {
	case AF_INET:
		if (packet) {
			ip = (struct ip *)packet;
			th = (struct tcphdr *)(ip + 1);
		}
		break;
#ifdef INET6
	case AF_INET6:
		if (packet) {
			ip6 = (struct ip6_hdr *)packet;
			th = (struct tcphdr *)(ip6 + 1);
		}
		break;
#endif
	default:
		return;
	}

	printf("%03d %s:%s ", (ntime/10) % 1000, tcpstates[ostate],
	    tanames[act]);

#ifndef INET6
	if (!ip)
#else
	if (!(ip || ip6))
#endif
		goto skipact;

	switch (act) {
	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (aflag) {
			inet_ntop(family,
#ifndef INET6
				(void *)&ip->ip_src,
#else
				family == AF_INET ? (void *)&ip->ip_src
						  : (void *)&ip6->ip6_src, 
#endif
				hbuf, sizeof(hbuf));
			printf("(src=%s,%u, ",
			    hbuf, ntohs(th->th_sport));
			inet_ntop(family,
#ifndef INET6
				(void *)&ip->ip_dst,
#else
				family == AF_INET ? (void *)&ip->ip_dst
						  : (void *)&ip6->ip6_dst, 
#endif
				hbuf, sizeof(hbuf));
			printf("dst=%s,%u)",
			    hbuf, ntohs(th->th_dport));
		}
		seq = th->th_seq;
		ack = th->th_ack;
		if (ip)
			len = ip->ip_len;
#ifdef INET6
		else if (ip6)
			len = ip6->ip6_plen;
#endif
		win = th->th_win;
		if (act == TA_OUTPUT) {
			NTOHL(seq);
			NTOHL(ack);
			NTOHS(len);
			NTOHS(win);
		}
		if (act == TA_OUTPUT)
			len -= sizeof(struct tcphdr);
		if (len)
			printf("[%x..%x)", seq, seq + len);
		else
			printf("%x", seq);
		printf("@%x", ack);
		if (win)
			printf("(win=%x)", win);
		flags = th->th_flags;
		if (flags) {
			const char *cp = "<";
#define	pf(flag, string) { \
	if (th->th_flags&flag) { \
		(void)printf("%s%s", cp, string); \
		cp = ","; \
	} \
}
			pf(TH_SYN, "SYN");
			pf(TH_ACK, "ACK");
			pf(TH_FIN, "FIN");
			pf(TH_RST, "RST");
			pf(TH_PUSH, "PUSH");
			pf(TH_URG, "URG");
			pf(TH_CWR, "CWR");
			pf(TH_ECE, "ECE");
			printf(">");
		}
		break;
	case TA_USER:
		timer = req >> 8;
		req &= 0xff;
		printf("%s", prurequests[req]);
		if (req == PRU_SLOWTIMO || req == PRU_FASTTIMO)
			printf("<%s>", tcptimers[timer]);
		break;
	}

skipact:
	printf(" -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf("\n");
	if (sflag) {
		printf("\trcv_nxt %x rcv_wnd %lx snd_una %x snd_nxt %x snd_max %x\n",
		    tp->rcv_nxt, tp->rcv_wnd, tp->snd_una, tp->snd_nxt,
		    tp->snd_max);
		printf("\tsnd_wl1 %x snd_wl2 %x snd_wnd %lx\n", tp->snd_wl1,
		    tp->snd_wl2, tp->snd_wnd);
	}
	/* print out timers? */
	if (tflag) {
		const char *cp = "\t";
		int i;
		int hardticks;

		if (use_sysctl) {
			size_t hlen = sizeof(hardticks);

			if (sysctlbyname("kern.hardclock_ticks", &hardticks,
			    &hlen, NULL, 0) == -1)
				err(1, "kern.hardclock_ticks");
		} else {
			if (kvm_read(kd, nl[N_HARDCLOCK_TICKS].n_value,
			    (char *)&hardticks, 
			    sizeof(hardticks)) != sizeof(hardticks))
				errx(3, "hardclock_ticks: %s", kvm_geterr(kd));

			for (i = 0; i < TCPT_NTIMERS; i++) {
				ci = (callout_impl_t *)&tp->t_timer[i];
				if ((ci->c_flags & CALLOUT_PENDING) == 0)
					continue;
				printf("%s%s=%d", cp, tcptimers[i],
				    ci->c_time - hardticks);
				if (i == TCPT_REXMT)
					printf(" (t_rxtshft=%d)", 
					    tp->t_rxtshift);
				cp = ", ";
			}
			if (*cp != '\t')
				putchar('\n');
		}
	}
}

int
numeric(const void *v1, const void *v2)
{
	const caddr_t *c1 = v1;
	const caddr_t *c2 = v2;
	int rv;

	if (*c1 < *c2)
		rv = -1;
	else if (*c1 > *c2)
		rv = 1;
	else
		rv = 0;

	return (rv);
}

void
usage(void)
{

	(void) fprintf(stderr, "usage: %s [-afjst] [-p hex-address]"
	    " [-N system] [-M core]\n", getprogname());
	exit(1);
}
