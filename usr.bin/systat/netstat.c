/*	$NetBSD: netstat.c,v 1.24 2003/08/07 11:15:59 agc Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)netstat.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: netstat.c,v 1.24 2003/08/07 11:15:59 agc Exp $");
#endif /* not lint */

/*
 * netstat
 */
#include <sys/param.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <net/route.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#endif
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "systat.h"
#include "extern.h"

static void fetchnetstat4(void *, int);
static void enter(struct inpcb *, struct socket *, int, char *);
static const char *inetname(struct in_addr);
static void inetprint(struct in_addr *, int, char *);
#ifdef INET6
static void fetchnetstat6(void *, int);
static void enter6(struct in6pcb *, struct socket *, int, char *);
static const char *inet6name(struct in6_addr *);
static void inet6print(struct in6_addr *, int, char *);
#endif

#define	streq(a,b)	(strcmp(a,b)==0)

struct netinfo {
	struct	netinfo *ni_forw, *ni_prev;
	int	ni_family;
	short	ni_line;		/* line on screen */
	short	ni_seen;		/* 0 when not present in list */
	short	ni_flags;
#define	NIF_LACHG	0x1		/* local address changed */
#define	NIF_FACHG	0x2		/* foreign address changed */
	short	ni_state;		/* tcp state */
	char	*ni_proto;		/* protocol */
	struct	in_addr ni_laddr;	/* local address */
#ifdef INET6
	struct	in6_addr ni_laddr6;	/* local address */
#endif
	long	ni_lport;		/* local port */
	struct	in_addr	ni_faddr;	/* foreign address */
#ifdef INET6
	struct	in6_addr ni_faddr6;	/* foreign address */
#endif
	long	ni_fport;		/* foreign port */
	long	ni_rcvcc;		/* rcv buffer character count */
	long	ni_sndcc;		/* snd buffer character count */
};

static struct {
	struct	netinfo *ni_forw, *ni_prev;
} netcb;

static	int aflag = 0;
int nflag = 0;
static	int lastrow = 1;

WINDOW *
opennetstat(void)
{

	sethostent(1);
	setnetent(1);
	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closenetstat(WINDOW *w)
{
	struct netinfo *p;

	endhostent();
	endnetent();
	p = (struct netinfo *)netcb.ni_forw;
	while (p != (struct netinfo *)&netcb) {
		if (p->ni_line != -1)
			lastrow--;
		p->ni_line = -1;
		p = p->ni_forw;
	}
	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

static struct nlist namelist[] = {
#define	X_TCBTABLE	0
	{ "_tcbtable" },
#define	X_UDBTABLE	1
	{ "_udbtable" },
#ifdef INET6
#define	X_TCB6		2
	{ "_tcb6" },
#define	X_UDB6		3
	{ "_udb6" },
#endif
	{ "" },
};

int
initnetstat(void)
{
	int n;

	n = kvm_nlist(kd, namelist);
	if (n < 0) {
		nlisterr(namelist);
		return(0);
	} else if (n == sizeof(namelist) / sizeof(namelist[0]) - 1) {
		error("No symbols in namelist");
		return(0);
	}

	netcb.ni_forw = netcb.ni_prev = (struct netinfo *)&netcb;
	protos = TCP|UDP;
	return(1);
}

void
fetchnetstat(void)
{
	struct netinfo *p;

	if (namelist[X_TCBTABLE].n_value == 0)
		return;
	for (p = netcb.ni_forw; p != (struct netinfo *)&netcb; p = p->ni_forw)
		p->ni_seen = 0;

	if ((protos & (TCP | UDP)) == 0) {
		error("No protocols to display");
		return;
	}
	if ((protos & TCP) && namelist[X_TCBTABLE].n_type)
		fetchnetstat4(NPTR(X_TCBTABLE), 1);
	if ((protos & UDP) && namelist[X_UDBTABLE].n_type)
		fetchnetstat4(NPTR(X_UDBTABLE), 0);
#ifdef INET6
	if ((protos & TCP) && namelist[X_TCB6].n_type)
		fetchnetstat6(NPTR(X_TCB6), 1);
	if ((protos & UDP) && namelist[X_UDB6].n_type)
		fetchnetstat6(NPTR(X_UDB6), 0);
#endif
}

static void
fetchnetstat4(void *off, int istcp)
{
	struct inpcbtable pcbtable;
	struct inpcb *head, *prev, *next;
	struct netinfo *p;
	struct inpcb inpcb;
	struct socket sockb;
	struct tcpcb tcpcb;

	KREAD(off, &pcbtable, sizeof pcbtable);
	prev = head = (struct inpcb *)&((struct inpcbtable *)off)->inpt_queue;
	next = pcbtable.inpt_queue.cqh_first;
	while (next != head) {
		KREAD(next, &inpcb, sizeof (inpcb));
		if (inpcb.inp_queue.cqe_prev != prev) {
printf("prev = %p, head = %p, next = %p, inpcb...prev = %p\n", prev, head, next, inpcb.inp_queue.cqe_prev);
			p = netcb.ni_forw;
			for (; p != (struct netinfo *)&netcb; p = p->ni_forw)
				p->ni_seen = 1;
			error("Kernel state in transition");
			return;
		}
		prev = next;
		next = inpcb.inp_queue.cqe_next;

		if (!aflag && inet_lnaof(inpcb.inp_laddr) == INADDR_ANY)
			continue;
		if (nhosts && !checkhost(&inpcb))
			continue;
		if (nports && !checkport(&inpcb))
			continue;
		KREAD(inpcb.inp_socket, &sockb, sizeof (sockb));
		if (istcp) {
			KREAD(inpcb.inp_ppcb, &tcpcb, sizeof (tcpcb));
			enter(&inpcb, &sockb, tcpcb.t_state, "tcp");
		} else
			enter(&inpcb, &sockb, 0, "udp");
	}
}

#ifdef INET6
static void
fetchnetstat6(void *off, int istcp)
{
	struct netinfo *p;
	struct socket sockb;
	struct tcpcb tcpcb;
	struct in6pcb in6pcb;
	struct in6pcb *head6, *prev6, *next6;

	KREAD(off, &in6pcb, sizeof (struct in6pcb));
	prev6 = head6 = (struct in6pcb *)off;
	next6 = in6pcb.in6p_next;
	while (next6 != head6) {
		KREAD(next6, &in6pcb, sizeof (in6pcb));
		if (in6pcb.in6p_prev != prev6) {
printf("prev = %p, head = %p, next = %p, in6pcb...prev = %p\n", prev6, head6, next6, in6pcb.in6p_prev);
			p = netcb.ni_forw;
			for (; p != (struct netinfo *)&netcb; p = p->ni_forw)
				p->ni_seen = 1;
			error("Kernel state in transition");
			return;
		}
		prev6 = next6;
		next6 = in6pcb.in6p_next;

		if (!aflag && IN6_IS_ADDR_UNSPECIFIED(&in6pcb.in6p_laddr))
			continue;
		if (nhosts && !checkhost6(&in6pcb))
			continue;
		if (nports && !checkport6(&in6pcb))
			continue;
		KREAD(in6pcb.in6p_socket, &sockb, sizeof (sockb));
		if (istcp) {
			KREAD(in6pcb.in6p_ppcb, &tcpcb, sizeof (tcpcb));
			enter6(&in6pcb, &sockb, tcpcb.t_state, "tcp");
		} else
			enter6(&in6pcb, &sockb, 0, "udp");
	}
}
#endif /*INET6*/

static void
enter(struct inpcb *inp, struct socket *so, int state, char *proto)
{
	struct netinfo *p;

	/*
	 * Only take exact matches, any sockets with
	 * previously unbound addresses will be deleted
	 * below in the display routine because they
	 * will appear as ``not seen'' in the kernel
	 * data structures.
	 */
	for (p = netcb.ni_forw; p != (struct netinfo *)&netcb; p = p->ni_forw) {
		if (p->ni_family != AF_INET)
			continue;
		if (!streq(proto, p->ni_proto))
			continue;
		if (p->ni_lport != inp->inp_lport ||
		    p->ni_laddr.s_addr != inp->inp_laddr.s_addr)
			continue;
		if (p->ni_faddr.s_addr == inp->inp_faddr.s_addr &&
		    p->ni_fport == inp->inp_fport)
			break;
	}
	if (p == (struct netinfo *)&netcb) {
		if ((p = malloc(sizeof(*p))) == NULL) {
			error("Out of memory");
			return;
		}
		p->ni_prev = (struct netinfo *)&netcb;
		p->ni_forw = netcb.ni_forw;
		netcb.ni_forw->ni_prev = p;
		netcb.ni_forw = p;
		p->ni_line = -1;
		p->ni_laddr = inp->inp_laddr;
		p->ni_lport = inp->inp_lport;
		p->ni_faddr = inp->inp_faddr;
		p->ni_fport = inp->inp_fport;
		p->ni_proto = proto;
		p->ni_flags = NIF_LACHG | NIF_FACHG;
		p->ni_family = AF_INET;
	}
	p->ni_rcvcc = so->so_rcv.sb_cc;
	p->ni_sndcc = so->so_snd.sb_cc;
	p->ni_state = state;
	p->ni_seen = 1;
}

#ifdef INET6
static void
enter6(struct in6pcb *in6p, struct socket *so, int state, char *proto)
{
	struct netinfo *p;

	/*
	 * Only take exact matches, any sockets with
	 * previously unbound addresses will be deleted
	 * below in the display routine because they
	 * will appear as ``not seen'' in the kernel
	 * data structures.
	 */
	for (p = netcb.ni_forw; p != (struct netinfo *)&netcb; p = p->ni_forw) {
		if (p->ni_family != AF_INET6)
			continue;
		if (!streq(proto, p->ni_proto))
			continue;
		if (p->ni_lport != in6p->in6p_lport ||
		    !IN6_ARE_ADDR_EQUAL(&p->ni_laddr6, &in6p->in6p_laddr))
			continue;
		if (IN6_ARE_ADDR_EQUAL(&p->ni_faddr6, &in6p->in6p_faddr) &&
		    p->ni_fport == in6p->in6p_fport)
			break;
	}
	if (p == (struct netinfo *)&netcb) {
		if ((p = malloc(sizeof(*p))) == NULL) {
			error("Out of memory");
			return;
		}
		p->ni_prev = (struct netinfo *)&netcb;
		p->ni_forw = netcb.ni_forw;
		netcb.ni_forw->ni_prev = p;
		netcb.ni_forw = p;
		p->ni_line = -1;
		p->ni_laddr6 = in6p->in6p_laddr;
		p->ni_lport = in6p->in6p_lport;
		p->ni_faddr6 = in6p->in6p_faddr;
		p->ni_fport = in6p->in6p_fport;
		p->ni_proto = proto;
		p->ni_flags = NIF_LACHG | NIF_FACHG;
		p->ni_family = AF_INET6;
	}
	p->ni_rcvcc = so->so_rcv.sb_cc;
	p->ni_sndcc = so->so_snd.sb_cc;
	p->ni_state = state;
	p->ni_seen = 1;
}
#endif

/* column locations */
#define	LADDR	0
#define	FADDR	LADDR+23
#define	PROTO	FADDR+23
#define	RCVCC	PROTO+6
#define	SNDCC	RCVCC+7
#define	STATE	SNDCC+7

void
labelnetstat(void)
{
	struct netinfo *p;

	if (namelist[X_TCBTABLE].n_type == 0)
		return;
	wmove(wnd, 0, 0); wclrtobot(wnd);
	mvwaddstr(wnd, 0, LADDR, "Local Address");
	mvwaddstr(wnd, 0, FADDR, "Foreign Address");
	mvwaddstr(wnd, 0, PROTO, "Proto");
	mvwaddstr(wnd, 0, RCVCC, "Recv-Q");
	mvwaddstr(wnd, 0, SNDCC, "Send-Q");
	mvwaddstr(wnd, 0, STATE, "(state)"); 
	
	p = netcb.ni_forw;
	for (; p != (struct netinfo *)&netcb; p = p->ni_forw) {
		if (p->ni_line == -1)
			continue;
		p->ni_flags |= NIF_LACHG | NIF_FACHG;
	}
}

void
shownetstat(void)
{
	struct netinfo *p, *q;

	/*
	 * First, delete any connections that have gone
	 * away and adjust the position of connections
	 * below to reflect the deleted line.
	 */
	p = netcb.ni_forw;
	while (p != (struct netinfo *)&netcb) {
		if (p->ni_line == -1 || p->ni_seen) {
			p = p->ni_forw;
			continue;
		}
		wmove(wnd, p->ni_line, 0); wdeleteln(wnd);
		q = netcb.ni_forw;
		for (; q != (struct netinfo *)&netcb; q = q->ni_forw)
			if (q != p && q->ni_line > p->ni_line) {
				q->ni_line--;
				/* this shouldn't be necessary */
				q->ni_flags |= NIF_LACHG | NIF_FACHG;
			}
		lastrow--;
		q = p->ni_forw;
		p->ni_prev->ni_forw = p->ni_forw;
		p->ni_forw->ni_prev = p->ni_prev;
		free(p);
		p = q;
	}
	/*
	 * Update existing connections and add new ones.
	 */
	for (p = netcb.ni_forw; p != (struct netinfo *)&netcb; p = p->ni_forw) {
		if (p->ni_line == -1) {
			/*
			 * Add a new entry if possible.
			 */
			if (lastrow > getmaxy(wnd))
				continue;
			p->ni_line = lastrow++;
			p->ni_flags |= NIF_LACHG | NIF_FACHG;
		}
		if (p->ni_flags & NIF_LACHG) {
			wmove(wnd, p->ni_line, LADDR);
			switch (p->ni_family) {
			case AF_INET:
				inetprint(&p->ni_laddr, p->ni_lport,
					p->ni_proto);
				break;
#ifdef INET6
			case AF_INET6:
				inet6print(&p->ni_laddr6, p->ni_lport,
					p->ni_proto);
				break;
#endif
			}
			p->ni_flags &= ~NIF_LACHG;
		}
		if (p->ni_flags & NIF_FACHG) {
			wmove(wnd, p->ni_line, FADDR);
			switch (p->ni_family) {
			case AF_INET:
				inetprint(&p->ni_faddr, p->ni_fport,
					p->ni_proto);
				break;
#ifdef INET6
			case AF_INET6:
				inet6print(&p->ni_faddr6, p->ni_fport,
					p->ni_proto);
				break;
#endif
			}
			p->ni_flags &= ~NIF_FACHG;
		}
		mvwaddstr(wnd, p->ni_line, PROTO, p->ni_proto);
#ifdef INET6
		if (p->ni_family == AF_INET6)
			waddstr(wnd, "6");
#endif
		mvwprintw(wnd, p->ni_line, RCVCC, "%6ld", p->ni_rcvcc);
		mvwprintw(wnd, p->ni_line, SNDCC, "%6ld", p->ni_sndcc);
		if (streq(p->ni_proto, "tcp")) {
			if (p->ni_state < 0 || p->ni_state >= TCP_NSTATES)
				mvwprintw(wnd, p->ni_line, STATE, "%d",
				    p->ni_state);
			else
				mvwaddstr(wnd, p->ni_line, STATE,
				    tcpstates[p->ni_state]);
		}
		wclrtoeol(wnd);
	}
	if (lastrow < getmaxy(wnd)) {
		wmove(wnd, lastrow, 0); wclrtobot(wnd);
		wmove(wnd, getmaxy(wnd), 0); wdeleteln(wnd);	/* XXX */
	}
}

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
static void
inetprint(struct in_addr *in, int port, char *proto)
{
	struct servent *sp = 0;
	char line[80], *cp;

	(void)snprintf(line, sizeof line, "%.*s.", 16, inetname(*in));
	cp = strchr(line, '\0');
	if (!nflag && port)
		sp = getservbyport(port, proto);
	if (sp || port == 0)
		(void)snprintf(cp, line + sizeof line - cp, "%.8s",
		     sp ? sp->s_name : "*");
	else
		(void)snprintf(cp, line + sizeof line - cp, "%d",
		     ntohs((u_short)port));
	/* pad to full column to clear any garbage */
	cp = strchr(line, '\0');
	while (cp - line < 22)
		*cp++ = ' ';
	*cp = '\0';
	waddstr(wnd, line);
}

#ifdef INET6
static void
inet6print(struct in6_addr *in6, int port, char *proto)
{
	struct servent *sp = 0;
	char line[80], *cp;

	(void)snprintf(line, sizeof line, "%.*s.", 16, inet6name(in6));
	cp = strchr(line, '\0');
	if (!nflag && port)
		sp = getservbyport(port, proto);
	if (sp || port == 0)
		(void)snprintf(cp, line + sizeof line - cp, "%.8s",
		     sp ? sp->s_name : "*");
	else
		(void)snprintf(cp, line + sizeof line - cp, "%d",
		     ntohs((u_short)port));
	/* pad to full column to clear any garbage */
	cp = strchr(line, '\0');
	while (cp - line < 22)
		*cp++ = ' ';
	*cp = '\0';
	waddstr(wnd, line);
}
#endif

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give 
 * numeric value, otherwise try for symbolic name.
 */
static const char *
inetname(struct in_addr in)
{
	char *cp = 0;
	static char line[50];
	struct hostent *hp;
	struct netent *np;

	if (!nflag && in.s_addr != INADDR_ANY) {
		int net = inet_netof(in);
		int lna = inet_lnaof(in);

		if (lna == INADDR_ANY) {
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp == 0) {
			hp = gethostbyaddr((char *)&in, sizeof (in), AF_INET);
			if (hp)
				cp = hp->h_name;
		}
	}
	if (in.s_addr == INADDR_ANY)
		strlcpy(line, "*", sizeof(line));
	else if (cp)
		strlcpy(line, cp, sizeof(line));
	else {
		in.s_addr = ntohl(in.s_addr);
#define C(x)	((x) & 0xff)
		(void)snprintf(line, sizeof line, "%u.%u.%u.%u",
		    C(in.s_addr >> 24), C(in.s_addr >> 16),
		    C(in.s_addr >> 8), C(in.s_addr));
#undef C
	}
	return (line);
}

#ifdef INET6
static const char *
inet6name(struct in6_addr *in6)
{
	static char line[NI_MAXHOST];
	struct sockaddr_in6 sin6;
	int flags;

	if (nflag)
		flags = NI_NUMERICHOST;
	else
		flags = 0;
	if (IN6_IS_ADDR_UNSPECIFIED(in6))
		return "*";
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_addr = *in6;
	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
			line, sizeof(line), NULL, 0, flags) == 0)
		return line;
	return "?";
}
#endif

/* please note: there are also some netstat commands in netcmds.c */

void
netstat_all(char *args)
{
	aflag = !aflag;
	fetchnetstat();
	shownetstat();
	refresh();
}

void
netstat_names(char *args)
{

	if (nflag == 0)
		return;
	
	nflag = 0;
	wclear(wnd);
	labelnetstat();
	shownetstat();
	refresh();
}

void
netstat_numbers(char *args)
{

	if (nflag != 0)
		return;
	
	nflag = 1;
	wclear(wnd);
	labelnetstat();
	shownetstat();
	refresh();
}
