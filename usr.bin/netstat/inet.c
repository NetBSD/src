/*	$NetBSD: inet.c,v 1.21 1997/05/22 17:21:28 christos Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
static char *rcsid = "$NetBSD: inet.c,v 1.21 1997/05/22 17:21:28 christos Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_var.h>
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

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

struct	inpcb inpcb;
struct	tcpcb tcpcb;
struct	socket sockb;

char	*inetname __P((struct in_addr *));
void	inetprint __P((struct in_addr *, int, char *));

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
void
protopr(off, name)
	u_long off;
	char *name;
{
	struct inpcbtable table;
	register struct inpcb *head, *next, *prev;
	struct inpcb inpcb;
	int istcp;
	static int first = 1;

	if (off == 0)
		return;
	istcp = strcmp(name, "tcp") == 0;
	kread(off, (char *)&table, sizeof table);
	prev = head =
	    (struct inpcb *)&((struct inpcbtable *)off)->inpt_queue.cqh_first;
	next = table.inpt_queue.cqh_first;

	while (next != head) {
		kread((u_long)next, (char *)&inpcb, sizeof inpcb);
		if (inpcb.inp_queue.cqe_prev != prev) {
			printf("???\n");
			break;
		}
		prev = next;
		next = inpcb.inp_queue.cqe_next;

		if (!aflag &&
		    inet_lnaof(inpcb.inp_laddr) == INADDR_ANY)
			continue;
		kread((u_long)inpcb.inp_socket, (char *)&sockb, sizeof (sockb));
		if (istcp) {
			kread((u_long)inpcb.inp_ppcb,
			    (char *)&tcpcb, sizeof (tcpcb));
		}
		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
				"Proto", "Recv-Q", "Send-Q",
				"Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			if (istcp)
				printf("%8lx ", (u_long) inpcb.inp_ppcb);
			else
				printf("%8lx ", (u_long) prev);
		printf("%-5.5s %6ld %6ld ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		inetprint(&inpcb.inp_laddr, (int)inpcb.inp_lport, name);
		inetprint(&inpcb.inp_faddr, (int)inpcb.inp_fport, name);
		if (istcp) {
			if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
				printf(" %d", tcpcb.t_state);
			else
				printf(" %s", tcpstates[tcpcb.t_state]);
		}
		putchar('\n');
	}
}

/*
 * Dump TCP statistics structure.
 */
void
tcp_stats(off, name)
	u_long off;
	char *name;
{
	struct tcpstat tcpstat;

	if (off == 0)
		return;
	printf ("%s:\n", name);
	kread(off, (char *)&tcpstat, sizeof (tcpstat));

#define	ps(f, m) if (tcpstat.f || sflag <= 1) \
    printf(m, tcpstat.f)
#define	p(f, m) if (tcpstat.f || sflag <= 1) \
    printf(m, tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m) if (tcpstat.f1 || tcpstat.f2 || sflag <= 1) \
    printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2, plural(tcpstat.f2))
#define	p2s(f1, f2, m) if (tcpstat.f1 || tcpstat.f2 || sflag <= 1) \
    printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2)
#define	p3(f, m) if (tcpstat.f || sflag <= 1) \
    printf(m, tcpstat.f, plurales(tcpstat.f))

	p(tcps_sndtotal, "\t%lu packet%s sent\n");
	p2(tcps_sndpack,tcps_sndbyte,
		"\t\t%lu data packet%s (%lu byte%s)\n");
	p2(tcps_sndrexmitpack, tcps_sndrexmitbyte,
		"\t\t%lu data packet%s (%lu byte%s) retransmitted\n");
	p2s(tcps_sndacks, tcps_delack,
		"\t\t%lu ack-only packet%s (%lu delayed)\n");
	p(tcps_sndurg, "\t\t%lu URG only packet%s\n");
	p(tcps_sndprobe, "\t\t%lu window probe packet%s\n");
	p(tcps_sndwinup, "\t\t%lu window update packet%s\n");
	p(tcps_sndctrl, "\t\t%lu control packet%s\n");
	p(tcps_rcvtotal, "\t%lu packet%s received\n");
	p2(tcps_rcvackpack, tcps_rcvackbyte, "\t\t%lu ack%s (for %lu byte%s)\n");
	p(tcps_rcvdupack, "\t\t%lu duplicate ack%s\n");
	p(tcps_rcvacktoomuch, "\t\t%lu ack%s for unsent data\n");
	p2(tcps_rcvpack, tcps_rcvbyte,
		"\t\t%lu packet%s (%lu byte%s) received in-sequence\n");
	p2(tcps_rcvduppack, tcps_rcvdupbyte,
		"\t\t%lu completely duplicate packet%s (%lu byte%s)\n");
	p(tcps_pawsdrop, "\t\t%lu old duplicate packet%s\n");
	p2(tcps_rcvpartduppack, tcps_rcvpartdupbyte,
		"\t\t%lu packet%s with some dup. data (%lu byte%s duped)\n");
	p2(tcps_rcvoopack, tcps_rcvoobyte,
		"\t\t%lu out-of-order packet%s (%lu byte%s)\n");
	p2(tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
		"\t\t%lu packet%s (%lu byte%s) of data after window\n");
	p(tcps_rcvwinprobe, "\t\t%lu window probe%s\n");
	p(tcps_rcvwinupd, "\t\t%lu window update packet%s\n");
	p(tcps_rcvafterclose, "\t\t%lu packet%s received after close\n");
	p(tcps_rcvbadsum, "\t\t%lu discarded for bad checksum%s\n");
	p(tcps_rcvbadoff, "\t\t%lu discarded for bad header offset field%s\n");
	ps(tcps_rcvshort, "\t\t%lu discarded because packet too short\n");
	p(tcps_connattempt, "\t%lu connection request%s\n");
	p(tcps_accepts, "\t%lu connection accept%s\n");
	p(tcps_connects, "\t%lu connection%s established (including accepts)\n");
	p2(tcps_closed, tcps_drops,
		"\t%lu connection%s closed (including %lu drop%s)\n");
	p(tcps_conndrops, "\t%lu embryonic connection%s dropped\n");
	p2(tcps_rttupdated, tcps_segstimed,
		"\t%lu segment%s updated rtt (of %lu attempt%s)\n");
	p(tcps_rexmttimeo, "\t%lu retransmit timeout%s\n");
	p(tcps_timeoutdrop, "\t\t%lu connection%s dropped by rexmit timeout\n");
	p(tcps_persisttimeo, "\t%lu persist timeout%s\n");
	p(tcps_keeptimeo, "\t%lu keepalive timeout%s\n");
	p(tcps_keepprobe, "\t\t%lu keepalive probe%s sent\n");
	p(tcps_keepdrops, "\t\t%lu connection%s dropped by keepalive\n");
	p(tcps_predack, "\t%lu correct ACK header prediction%s\n");
	p(tcps_preddat, "\t%lu correct data packet header prediction%s\n");
	p3(tcps_pcbhashmiss, "\t%lu PCB hash miss%s\n");
	ps(tcps_noport, "\t%lu dropped due to no socket\n");

#undef p
#undef ps
#undef p2
#undef p2s
#undef p3
}

/*
 * Dump UDP statistics structure.
 */
void
udp_stats(off, name)
	u_long off;
	char *name;
{
	struct udpstat udpstat;
	u_long delivered;

	if (off == 0)
		return;
	printf("%s:\n", name);
	kread(off, (char *)&udpstat, sizeof (udpstat));

#define	ps(f, m) if (udpstat.f || sflag <= 1) \
    printf(m, udpstat.f)
#define	p(f, m) if (udpstat.f || sflag <= 1) \
    printf(m, udpstat.f, plural(udpstat.f))
#define	p3(f, m) if (udpstat.f || sflag <= 1) \
    printf(m, udpstat.f, plurales(udpstat.f))

	p(udps_ipackets, "\t%lu datagram%s received\n");
	ps(udps_hdrops, "\t%lu with incomplete header\n");
	ps(udps_badlen, "\t%lu with bad data length field\n");
	ps(udps_badsum, "\t%lu with bad checksum\n");
	ps(udps_noport, "\t%lu dropped due to no socket\n");
	p(udps_noportbcast, "\t%lu broadcast/multicast datagram%s dropped due to no socket\n");
	ps(udps_fullsock, "\t%lu dropped due to full socket buffers\n");
	delivered = udpstat.udps_ipackets -
		    udpstat.udps_hdrops -
		    udpstat.udps_badlen -
		    udpstat.udps_badsum -
		    udpstat.udps_noport -
		    udpstat.udps_noportbcast -
		    udpstat.udps_fullsock;
	if (delivered || sflag <= 1)
		printf("\t%lu delivered\n", delivered);
	p3(udps_pcbhashmiss, "\t%lu PCB hash miss%s\n");
	p(udps_opackets, "\t%lu datagram%s output\n");

#undef ps
#undef p
#undef p3
}

/*
 * Dump IP statistics structure.
 */
void
ip_stats(off, name)
	u_long off;
	char *name;
{
	struct ipstat ipstat;

	if (off == 0)
		return;
	kread(off, (char *)&ipstat, sizeof (ipstat));
	printf("%s:\n", name);

#define	ps(f, m) if (ipstat.f || sflag <= 1) \
    printf(m, ipstat.f)
#define	p(f, m) if (ipstat.f || sflag <= 1) \
    printf(m, ipstat.f, plural(ipstat.f))

	p(ips_total, "\t%lu total packet%s received\n");
	p(ips_badsum, "\t%lu bad header checksum%s\n");
	ps(ips_toosmall, "\t%lu with size smaller than minimum\n");
	ps(ips_tooshort, "\t%lu with data size < data length\n");
	ps(ips_toolong, "\t%lu with length > max ip packet size\n");
	ps(ips_badhlen, "\t%lu with header length < data size\n");
	ps(ips_badlen, "\t%lu with data length < header length\n");
	ps(ips_badoptions, "\t%lu with bad options\n");
	ps(ips_badvers, "\t%lu with incorrect version number\n");
	p(ips_fragments, "\t%lu fragment%s received\n");
	p(ips_fragdropped, "\t%lu fragment%s dropped (dup or out of space)\n");
	p(ips_badfrags, "\t%lu malformed fragment%s dropped\n");
	p(ips_fragtimeout, "\t%lu fragment%s dropped after timeout\n");
	p(ips_reassembled, "\t%lu packet%s reassembled ok\n");
	p(ips_delivered, "\t%lu packet%s for this host\n");
	p(ips_noproto, "\t%lu packet%s for unknown/unsupported protocol\n");
	p(ips_forward, "\t%lu packet%s forwarded\n");
	p(ips_cantforward, "\t%lu packet%s not forwardable\n");
	p(ips_redirectsent, "\t%lu redirect%s sent\n");
	p(ips_localout, "\t%lu packet%s sent from this host\n");
	p(ips_rawout, "\t%lu packet%s sent with fabricated ip header\n");
	p(ips_odropped, "\t%lu output packet%s dropped due to no bufs, etc.\n");
	p(ips_noroute, "\t%lu output packet%s discarded due to no route\n");
	p(ips_fragmented, "\t%lu output datagram%s fragmented\n");
	p(ips_ofragments, "\t%lu fragment%s created\n");
	p(ips_cantfrag, "\t%lu datagram%s that can't be fragmented\n");
#undef ps
#undef p
}

static	char *icmpnames[] = {
	"echo reply",
	"#1",
	"#2",
	"destination unreachable",
	"source quench",
	"routing redirect",
	"#6",
	"#7",
	"echo",
	"#9",
	"#10",
	"time exceeded",
	"parameter problem",
	"time stamp",
	"time stamp reply",
	"information request",
	"information request reply",
	"address mask request",
	"address mask reply",
};

/*
 * Dump ICMP statistics.
 */
void
icmp_stats(off, name)
	u_long off;
	char *name;
{
	struct icmpstat icmpstat;
	register int i, first;

	if (off == 0)
		return;
	kread(off, (char *)&icmpstat, sizeof (icmpstat));
	printf("%s:\n", name);

#define	p(f, m) if (icmpstat.f || sflag <= 1) \
    printf(m, icmpstat.f, plural(icmpstat.f))

	p(icps_error, "\t%lu call%s to icmp_error\n");
	p(icps_oldicmp,
	    "\t%lu error%s not generated because old message was icmp\n");
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
				icmpstat.icps_outhist[i]);
		}
	p(icps_badcode, "\t%lu message%s with bad code fields\n");
	p(icps_tooshort, "\t%lu message%s < minimum length\n");
	p(icps_checksum, "\t%lu bad checksum%s\n");
	p(icps_badlen, "\t%lu message%s with bad length\n");
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
				icmpstat.icps_inhist[i]);
		}
	p(icps_reflect, "\t%lu message response%s generated\n");
#undef p
}

/*
 * Dump IGMP statistics structure.
 */
void
igmp_stats(off, name)
	u_long off;
	char *name;
{
	struct igmpstat igmpstat;

	if (off == 0)
		return;
	kread(off, (char *)&igmpstat, sizeof (igmpstat));
	printf("%s:\n", name);

#define	p(f, m) if (igmpstat.f || sflag <= 1) \
    printf(m, igmpstat.f, plural(igmpstat.f))
#define	py(f, m) if (igmpstat.f || sflag <= 1) \
    printf(m, igmpstat.f, igmpstat.f != 1 ? "ies" : "y")
	p(igps_rcv_total, "\t%lu message%s received\n");
        p(igps_rcv_tooshort, "\t%lu message%s received with too few bytes\n");
        p(igps_rcv_badsum, "\t%lu message%s received with bad checksum\n");
        py(igps_rcv_queries, "\t%lu membership quer%s received\n");
        py(igps_rcv_badqueries, "\t%lu membership quer%s received with invalid field(s)\n");
        p(igps_rcv_reports, "\t%lu membership report%s received\n");
        p(igps_rcv_badreports, "\t%lu membership report%s received with invalid field(s)\n");
        p(igps_rcv_ourreports, "\t%lu membership report%s received for groups to which we belong\n");
        p(igps_snd_reports, "\t%lu membership report%s sent\n");
#undef p
#undef py
}

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
void
inetprint(in, port, proto)
	register struct in_addr *in;
	int port;
	char *proto;
{
	struct servent *sp = 0;
	char line[80], *cp;
	int width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(in));
	cp = index(line, '\0');
	if (!nflag && port)
		sp = getservbyport((int)port, proto);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%u", ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(inp)
	struct in_addr *inp;
{
	register char *cp;
	static char line[50];
	struct hostent *hp;
	struct netent *np;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && inp->s_addr != INADDR_ANY) {
		int net = inet_netof(*inp);
		int lna = inet_lnaof(*inp);

		if (lna == INADDR_ANY) {
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp == 0) {
			hp = gethostbyaddr((char *)inp, sizeof (*inp), AF_INET);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
	}
	if (inp->s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		inp->s_addr = ntohl(inp->s_addr);
#define C(x)	((x) & 0xff)
		sprintf(line, "%u.%u.%u.%u", C(inp->s_addr >> 24),
		    C(inp->s_addr >> 16), C(inp->s_addr >> 8), C(inp->s_addr));
	}
	return (line);
}
