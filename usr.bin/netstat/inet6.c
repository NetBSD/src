/*	$NetBSD: inet6.c,v 1.4 1999/11/19 10:44:33 bouyer Exp $	*/

/*	BSDI inet.c,v 2.3 1995/10/24 02:19:29 prb Exp	*/
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
__RCSID("$NetBSD: inet6.c,v 1.4 1999/11/19 10:44:33 bouyer Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_systm.h>
#ifndef TCP6
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#endif
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#ifdef TCP6
#include <netinet6/tcp6.h>
#include <netinet6/tcp6_seq.h>
#define TCP6STATES
#include <netinet6/tcp6_fsm.h>
#define TCP6TIMERS
#include <netinet6/tcp6_timer.h>
#include <netinet6/tcp6_var.h>
#include <netinet6/tcp6_debug.h>
#else
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
/*#define TCPSTATES*/
#include <netinet/tcp_fsm.h>
extern char *tcpstates[];
/*#define	TCPTIMERS*/
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#endif /*TCP6*/
#include <netinet6/udp6.h>
#include <netinet6/udp6_var.h>
#include <netinet6/pim6_var.h>

#include <arpa/inet.h>
#if 0
#include "gethostbyname2.h"
#endif
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

#ifdef INET6

struct	in6pcb in6pcb;
#ifdef TCP6
struct	tcp6cb tcp6cb;
#else
struct	tcpcb tcpcb;
#endif
struct	socket sockb;

char	*inet6name __P((struct in6_addr *));
void	inet6print __P((struct in6_addr *, int, char *));

static char ntop_buf[INET6_ADDRSTRLEN];

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
void
ip6protopr(off, name)
	u_long off;
	char *name;
{
	struct in6pcb cb;
	register struct in6pcb *prev, *next;
	int istcp;
	static int first = 1;

	if (off == 0)
		return;
	istcp = strcmp(name, "tcp6") == 0;
	kread(off, (char *)&cb, sizeof (struct in6pcb));
	in6pcb = cb;
	prev = (struct in6pcb *)off;
	if (in6pcb.in6p_next == (struct in6pcb *)off)
		return;
	while (in6pcb.in6p_next != (struct in6pcb *)off) {
		next = in6pcb.in6p_next;
		kread((u_long)next, (char *)&in6pcb, sizeof (in6pcb));
		if (in6pcb.in6p_prev != prev) {
			printf("???\n");
			break;
		}
		if (!aflag && IN6_IS_ADDR_ANY(&in6pcb.in6p_laddr)) {
			prev = next;
			continue;
		}
		kread((u_long)in6pcb.in6p_socket, (char *)&sockb, sizeof (sockb));
		if (istcp) {
#ifdef TCP6
			kread((u_long)in6pcb.in6p_ppcb,
			    (char *)&tcp6cb, sizeof (tcp6cb));
#else
			kread((u_long)in6pcb.in6p_ppcb,
			    (char *)&tcpcb, sizeof (tcpcb));
#endif
		}
		if (first) {
			printf("Active Internet6 connections");
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
		if (Aflag) {
			if (istcp)
				printf("%8p ", in6pcb.in6p_ppcb);
			else
				printf("%8p ", next);
		}
		printf("%-5.5s %6ld %6ld ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		/* xxx */
		inet6print(&in6pcb.in6p_laddr, (int)in6pcb.in6p_lport, name);
		inet6print(&in6pcb.in6p_faddr, (int)in6pcb.in6p_fport, name);
		if (istcp) {
#ifdef TCP6
			if (tcp6cb.t_state < 0 || tcp6cb.t_state >= TCP6_NSTATES)
				printf(" %d", tcp6cb.t_state);
			else
				printf(" %s", tcp6states[tcp6cb.t_state]);
#else
			if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
				printf(" %d", tcpcb.t_state);
			else
				printf(" %s", tcpstates[tcpcb.t_state]);
#endif
		}
		putchar('\n');
		prev = next;
	}
}

#ifdef TCP6
/*
 * Dump TCP6 statistics structure.
 */
void
tcp6_stats(off, name)
	u_long off;
	char *name;
{
	struct tcp6stat tcp6stat;

	if (off == 0)
		return;
	printf ("%s:\n", name);
	kread(off, (char *)&tcp6stat, sizeof (tcp6stat));

#define	p(f, m) if (tcp6stat.f || sflag <= 1) \
    printf(m, tcp6stat.f, plural(tcp6stat.f))
#define	p2(f1, f2, m) if (tcp6stat.f1 || tcp6stat.f2 || sflag <= 1) \
    printf(m, tcp6stat.f1, plural(tcp6stat.f1), tcp6stat.f2, plural(tcp6stat.f2))
#define	p3(f, m) if (tcp6stat.f || sflag <= 1) \
    printf(m, tcp6stat.f, plurales(tcp6stat.f))

	p(tcp6s_sndtotal, "\t%ld packet%s sent\n");
	p2(tcp6s_sndpack,tcp6s_sndbyte,
		"\t\t%ld data packet%s (%ld byte%s)\n");
	p2(tcp6s_sndrexmitpack, tcp6s_sndrexmitbyte,
		"\t\t%ld data packet%s (%ld byte%s) retransmitted\n");
	p2(tcp6s_sndacks, tcp6s_delack,
		"\t\t%ld ack-only packet%s (%ld packet%s delayed)\n");
	p(tcp6s_sndurg, "\t\t%ld URG only packet%s\n");
	p(tcp6s_sndprobe, "\t\t%ld window probe packet%s\n");
	p(tcp6s_sndwinup, "\t\t%ld window update packet%s\n");
	p(tcp6s_sndctrl, "\t\t%ld control packet%s\n");
	p(tcp6s_rcvtotal, "\t%ld packet%s received\n");
	p2(tcp6s_rcvackpack, tcp6s_rcvackbyte, "\t\t%ld ack%s (for %ld byte%s)\n");
	p(tcp6s_rcvdupack, "\t\t%ld duplicate ack%s\n");
	p(tcp6s_rcvacktoomuch, "\t\t%ld ack%s for unsent data\n");
	p2(tcp6s_rcvpack, tcp6s_rcvbyte,
		"\t\t%ld packet%s (%ld byte%s) received in-sequence\n");
	p2(tcp6s_rcvduppack, tcp6s_rcvdupbyte,
		"\t\t%ld completely duplicate packet%s (%ld byte%s)\n");
	p(tcp6s_pawsdrop, "\t\t%ld old duplicate packet%s\n");
	p2(tcp6s_rcvpartduppack, tcp6s_rcvpartdupbyte,
		"\t\t%ld packet%s with some dup. data (%ld byte%s duped)\n");
	p2(tcp6s_rcvoopack, tcp6s_rcvoobyte,
		"\t\t%ld out-of-order packet%s (%ld byte%s)\n");
	p2(tcp6s_rcvpackafterwin, tcp6s_rcvbyteafterwin,
		"\t\t%ld packet%s (%ld byte%s) of data after window\n");
	p(tcp6s_rcvwinprobe, "\t\t%ld window probe%s\n");
	p(tcp6s_rcvwinupd, "\t\t%ld window update packet%s\n");
	p(tcp6s_rcvafterclose, "\t\t%ld packet%s received after close\n");
	p(tcp6s_rcvbadsum, "\t\t%ld discarded for bad checksum%s\n");
	p(tcp6s_rcvbadoff, "\t\t%ld discarded for bad header offset field%s\n");
	p(tcp6s_rcvshort, "\t\t%ld discarded because packet%s too short\n");
	p(tcp6s_connattempt, "\t%ld connection request%s\n");
	p(tcp6s_accepts, "\t%ld connection accept%s\n");
	p(tcp6s_badsyn, "\t%ld bad connection attempt%s\n");
	p(tcp6s_connects, "\t%ld connection%s established (including accepts)\n");
	p2(tcp6s_closed, tcp6s_drops,
		"\t%ld connection%s closed (including %ld drop%s)\n");
	p(tcp6s_conndrops, "\t%ld embryonic connection%s dropped\n");
	p2(tcp6s_rttupdated, tcp6s_segstimed,
		"\t%ld segment%s updated rtt (of %ld attempt%s)\n");
	p(tcp6s_rexmttimeo, "\t%ld retransmit timeout%s\n");
	p(tcp6s_timeoutdrop, "\t\t%ld connection%s dropped by rexmit timeout\n");
	p(tcp6s_persisttimeo, "\t%ld persist timeout%s\n");
	p(tcp6s_persistdrop, "\t%ld connection%s timed out in persist\n");
	p(tcp6s_keeptimeo, "\t%ld keepalive timeout%s\n");
	p(tcp6s_keepprobe, "\t\t%ld keepalive probe%s sent\n");
	p(tcp6s_keepdrops, "\t\t%ld connection%s dropped by keepalive\n");
	p(tcp6s_predack, "\t%ld correct ACK header prediction%s\n");
	p(tcp6s_preddat, "\t%ld correct data packet header prediction%s\n");
	p3(tcp6s_pcbcachemiss, "\t%ld PCB cache miss%s\n");
#undef p
#undef p2
#undef p3
}
#endif

/*
 * Dump UDP6 statistics structure.
 */
void
udp6_stats(off, name)
	u_long off;
	char *name;
{
	struct udp6stat udp6stat;
	u_quad_t delivered;

	if (off == 0)
		return;
	kread(off, (char *)&udp6stat, sizeof (udp6stat));
	printf("%s:\n", name);
#define	p(f, m) if (udp6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)udp6stat.f, plural(udp6stat.f))
#define	p1(f, m) if (udp6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)udp6stat.f)
	p(udp6s_ipackets, "\t%llu datagram%s received\n");
	p1(udp6s_hdrops, "\t%llu with incomplete header\n");
	p1(udp6s_badlen, "\t%llu with bad data length field\n");
	p1(udp6s_badsum, "\t%llu with bad checksum\n");
	p1(udp6s_nosum, "\t%llu with no checksum\n");
	p1(udp6s_noport, "\t%llu dropped due to no socket\n");
	p(udp6s_noportmcast, "\t%llu multicast datagram%s dropped due to no socket\n");
	p1(udp6s_fullsock, "\t%llu dropped due to full socket buffers\n");
	delivered = udp6stat.udp6s_ipackets -
		    udp6stat.udp6s_hdrops -
		    udp6stat.udp6s_badlen -
		    udp6stat.udp6s_badsum -
		    udp6stat.udp6s_noport -
		    udp6stat.udp6s_noportmcast -
		    udp6stat.udp6s_fullsock;
	if (delivered || sflag <= 1)
		printf("\t%llu delivered\n", (unsigned long long)delivered);
	p(udp6s_opackets, "\t%llu datagram%s output\n");
#undef p
#undef p1
}

static	char *ip6nh[] = {
	"hop by hop",
	"ICMP",
	"IGMP",
	"#3",
	"IP",
	"#5",
	"TCP",
	"#7",
	"#8",
	"#9",
	"#10",
	"#11",
	"#12",
	"#13",
	"#14",
	"#15",
	"#16",
	"UDP",
	"#18",
	"#19",	
	"#20",
	"#21",
	"IDP",
	"#23",
	"#24",
	"#25",
	"#26",
	"#27",
	"#28",
	"TP",	
	"#30",
	"#31",
	"#32",
	"#33",
	"#34",
	"#35",
	"#36",
	"#37",
	"#38",
	"#39",	
	"#40",
	"IP6",
	"#42",
	"routing",
	"fragment",
	"#45",
	"#46",
	"#47",
	"#48",
	"#49",	
	"ESP",
	"AH",
	"#52",
	"#53",
	"#54",
	"#55",
	"#56",
	"#57",
	"ICMP6",
	"no next header",	
	"destination option",
	"#61",
	"#62",
	"#63",
	"#64",
	"#65",
	"#66",
	"#67",
	"#68",
	"#69",	
	"#70",
	"#71",
	"#72",
	"#73",
	"#74",
	"#75",
	"#76",
	"#77",
	"#78",
	"#79",	
	"ISOIP",
	"#81",
	"#82",
	"#83",
	"#84",
	"#85",
	"#86",
	"#87",
	"#88",
	"#89",	
	"#80",
	"#91",
	"#92",
	"#93",
	"#94",
	"#95",
	"#96",
	"Ethernet",
	"#98",
	"#99",	
	"#100",
	"#101",
	"#102",
	"PIM",
	"#104",
	"#105",
	"#106",
	"#107",
	"#108",
	"#109",	
	"#110",
	"#111",
	"#112",
	"#113",
	"#114",
	"#115",
	"#116",
	"#117",
	"#118",
	"#119",	
	"#120",
	"#121",
	"#122",
	"#123",
	"#124",
	"#125",
	"#126",
	"#127",
	"#128",
	"#129",	
	"#130",
	"#131",
	"#132",
	"#133",
	"#134",
	"#135",
	"#136",
	"#137",
	"#138",
	"#139",	
	"#140",
	"#141",
	"#142",
	"#143",
	"#144",
	"#145",
	"#146",
	"#147",
	"#148",
	"#149",	
	"#150",
	"#151",
	"#152",
	"#153",
	"#154",
	"#155",
	"#156",
	"#157",
	"#158",
	"#159",	
	"#160",
	"#161",
	"#162",
	"#163",
	"#164",
	"#165",
	"#166",
	"#167",
	"#168",
	"#169",	
	"#170",
	"#171",
	"#172",
	"#173",
	"#174",
	"#175",
	"#176",
	"#177",
	"#178",
	"#179",	
	"#180",
	"#181",
	"#182",
	"#183",
	"#184",
	"#185",
	"#186",
	"#187",
	"#188",
	"#189",	
	"#180",
	"#191",
	"#192",
	"#193",
	"#194",
	"#195",
	"#196",
	"#197",
	"#198",
	"#199",	
	"#200",
	"#201",
	"#202",
	"#203",
	"#204",
	"#205",
	"#206",
	"#207",
	"#208",
	"#209",	
	"#210",
	"#211",
	"#212",
	"#213",
	"#214",
	"#215",
	"#216",
	"#217",
	"#218",
	"#219",	
	"#220",
	"#221",
	"#222",
	"#223",
	"#224",
	"#225",
	"#226",
	"#227",
	"#228",
	"#229",	
	"#230",
	"#231",
	"#232",
	"#233",
	"#234",
	"#235",
	"#236",
	"#237",
	"#238",
	"#239",	
	"#240",
	"#241",
	"#242",
	"#243",
	"#244",
	"#245",
	"#246",
	"#247",
	"#248",
	"#249",	
	"#250",
	"#251",
	"#252",
	"#253",
	"#254",
	"#255",
};

/*
 * Dump IP6 statistics structure.
 */
void
ip6_stats(off, name)
	u_long off;
	char *name;
{
	struct ip6stat ip6stat;
	int first, i;

	if (off == 0)
		return;

	kread(off, (char *)&ip6stat, sizeof (ip6stat));
	printf("%s:\n", name);

#define	p(f, m) if (ip6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)ip6stat.f, plural(ip6stat.f))
#define	p1(f, m) if (ip6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)ip6stat.f)

	p(ip6s_total, "\t%llu total packet%s received\n");
	p1(ip6s_toosmall, "\t%llu with size smaller than minimum\n");
	p1(ip6s_tooshort, "\t%llu with data size < data length\n");
	p1(ip6s_badoptions, "\t%llu with bad options\n");
	p1(ip6s_badvers, "\t%llu with incorrect version number\n");
	p(ip6s_fragments, "\t%llu fragment%s received\n");
	p(ip6s_fragdropped,
		"\t%llu fragment%s dropped (dup or out of space)\n");
	p(ip6s_fragtimeout, "\t%llu fragment%s dropped after timeout\n");
	p(ip6s_fragoverflow, "\t%llu fragment%s that exceeded limit\n");
	p(ip6s_reassembled, "\t%llu packet%s reassembled ok\n");
	p(ip6s_delivered, "\t%llu packet%s for this host\n");
	p(ip6s_forward, "\t%llu packet%s forwarded\n");
	p(ip6s_cantforward, "\t%llu packet%s not forwardable\n");
	p(ip6s_redirectsent, "\t%llu redirect%s sent\n");
	p(ip6s_localout, "\t%llu packet%s sent from this host\n");
	p(ip6s_rawout, "\t%llu packet%s sent with fabricated ip header\n");
	p(ip6s_odropped,
		"\t%llu output packet%s dropped due to no bufs, etc.\n");
	p(ip6s_noroute, "\t%llu output packet%s discarded due to no route\n");
	p(ip6s_fragmented, "\t%llu output datagram%s fragmented\n");
	p(ip6s_ofragments, "\t%llu fragment%s created\n");
	p(ip6s_cantfrag, "\t%llu datagram%s that can't be fragmented\n");
	p(ip6s_badscope, "\t%llu packet%s that violated scope rules\n");
	p(ip6s_notmember, "\t%llu multicast packet%s which we don't join\n");
	for (first = 1, i = 0; i < 256; i++)
		if (ip6stat.ip6s_nxthist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %llu\n", ip6nh[i],
			       (unsigned long long)ip6stat.ip6s_nxthist[i]);
		}
	printf("\tMbuf statics:\n");
	printf("\t\t%llu one mbuf\n", (unsigned long long)ip6stat.ip6s_m1);
	for (first = 1, i = 0; i < 32; i++) {
		char ifbuf[IFNAMSIZ];
		if (ip6stat.ip6s_m2m[i] != 0) {		
			if (first) {
				printf("\t\ttwo or more mbuf:\n");
				first = 0;
			}
			printf("\t\t\t%s = %llu\n",
			       if_indextoname(i, ifbuf),
			       (unsigned long long)ip6stat.ip6s_m2m[i]);
		}
	}
	printf("\t\t%llu one ext mbuf\n",
	        (unsigned long long)ip6stat.ip6s_mext1);
	printf("\t\t%llu two or more ext mbuf\n",
		(unsigned long long)ip6stat.ip6s_mext2m);	
	p(ip6s_exthdrtoolong,
		"\t%llu packet%s whose headers are not continuous\n");
	p(ip6s_nogif, "\t%llu tunneling packet%s that can't find gif\n");
#undef p
#undef p1
}

static	char *icmp6names[] = {
	"#0",
	"unreach",
	"packet too big",
	"time exceed",
	"parameter problem",
	"#5",
	"#6",
	"#7",
	"#8",
	"#9",
	"#10",
	"#11",
	"#12",
	"#13",
	"#14",
	"#15",
	"#16",
	"#17",
	"#18",
	"#19",	
	"#20",
	"#21",
	"#22",
	"#23",
	"#24",
	"#25",
	"#26",
	"#27",
	"#28",
	"#29",	
	"#30",
	"#31",
	"#32",
	"#33",
	"#34",
	"#35",
	"#36",
	"#37",
	"#38",
	"#39",	
	"#40",
	"#41",
	"#42",
	"#43",
	"#44",
	"#45",
	"#46",
	"#47",
	"#48",
	"#49",	
	"#50",
	"#51",
	"#52",
	"#53",
	"#54",
	"#55",
	"#56",
	"#57",
	"#58",
	"#59",	
	"#60",
	"#61",
	"#62",
	"#63",
	"#64",
	"#65",
	"#66",
	"#67",
	"#68",
	"#69",	
	"#70",
	"#71",
	"#72",
	"#73",
	"#74",
	"#75",
	"#76",
	"#77",
	"#78",
	"#79",	
	"#80",
	"#81",
	"#82",
	"#83",
	"#84",
	"#85",
	"#86",
	"#87",
	"#88",
	"#89",	
	"#80",
	"#91",
	"#92",
	"#93",
	"#94",
	"#95",
	"#96",
	"#97",
	"#98",
	"#99",	
	"#100",
	"#101",
	"#102",
	"#103",
	"#104",
	"#105",
	"#106",
	"#107",
	"#108",
	"#109",	
	"#110",
	"#111",
	"#112",
	"#113",
	"#114",
	"#115",
	"#116",
	"#117",
	"#118",
	"#119",	
	"#120",
	"#121",
	"#122",
	"#123",
	"#124",
	"#125",
	"#126",
	"#127",
	"echo",
	"echo reply",	
	"group member query",
	"group member report",
	"group member termination",
	"router solicitation",
	"router advertisment",
	"neighbor solicitation",
	"neighbor advertisment",
	"redirect",
	"router renumbering",
	"node information request",
	"node information reply",
	"#141",
	"#142",
	"#143",
	"#144",
	"#145",
	"#146",
	"#147",
	"#148",
	"#149",	
	"#150",
	"#151",
	"#152",
	"#153",
	"#154",
	"#155",
	"#156",
	"#157",
	"#158",
	"#159",	
	"#160",
	"#161",
	"#162",
	"#163",
	"#164",
	"#165",
	"#166",
	"#167",
	"#168",
	"#169",	
	"#170",
	"#171",
	"#172",
	"#173",
	"#174",
	"#175",
	"#176",
	"#177",
	"#178",
	"#179",	
	"#180",
	"#181",
	"#182",
	"#183",
	"#184",
	"#185",
	"#186",
	"#187",
	"#188",
	"#189",	
	"#180",
	"#191",
	"#192",
	"#193",
	"#194",
	"#195",
	"#196",
	"#197",
	"#198",
	"#199",	
	"#200",
	"#201",
	"#202",
	"#203",
	"#204",
	"#205",
	"#206",
	"#207",
	"#208",
	"#209",	
	"#210",
	"#211",
	"#212",
	"#213",
	"#214",
	"#215",
	"#216",
	"#217",
	"#218",
	"#219",	
	"#220",
	"#221",
	"#222",
	"#223",
	"#224",
	"#225",
	"#226",
	"#227",
	"#228",
	"#229",	
	"#230",
	"#231",
	"#232",
	"#233",
	"#234",
	"#235",
	"#236",
	"#237",
	"#238",
	"#239",	
	"#240",
	"#241",
	"#242",
	"#243",
	"#244",
	"#245",
	"#246",
	"#247",
	"#248",
	"#249",	
	"#250",
	"#251",
	"#252",
	"#253",
	"#254",
	"#255",
};

/*
 * Dump ICMP6 statistics.
 */
void
icmp6_stats(off, name)
	u_long off;
	char *name;
{
	struct icmp6stat icmp6stat;
	register int i, first;

	if (off == 0)
		return;
	kread(off, (char *)&icmp6stat, sizeof (icmp6stat));
	printf("%s:\n", name);

#define	p(f, m) if (icmp6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)icmp6stat.f, plural(icmp6stat.f))

	p(icp6s_error, "\t%llu call%s to icmp_error\n");
	p(icp6s_canterror, "\t%llu error%s not generated because old message "
		"was icmp or so\n");
	for (first = 1, i = 0; i < 256; i++)
		if (icmp6stat.icp6s_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %llu\n", icmp6names[i],
				(unsigned long long)icmp6stat.icp6s_outhist[i]);
		}
	p(icp6s_badcode, "\t%llu message%s with bad code fields\n");
	p(icp6s_tooshort, "\t%llu message%s < minimum length\n");
	p(icp6s_checksum, "\t%llu bad checksum%s\n");
	p(icp6s_badlen, "\t%llu message%s with bad length\n");
	for (first = 1, i = 0; i < ICMP6_MAXTYPE; i++)
		if (icmp6stat.icp6s_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %llu\n", icmp6names[i],
				(unsigned long long)icmp6stat.icp6s_inhist[i]);
		}
	p(icp6s_reflect, "\t%llu message response%s generated\n");
#undef p
}

/*
 * Dump PIM statistics structure.
 */
void
pim6_stats(off, name)
	u_long off;
	char *name;
{
	struct pim6stat pim6stat;

	if (off == 0)
		return;
	kread(off, (char *)&pim6stat, sizeof(pim6stat));
	printf("%s:\n", name);

#define	p(f, m) if (pim6stat.f || sflag <= 1) \
    printf(m, (unsigned long long)pim6stat.f, plural(pim6stat.f))
	p(pim6s_rcv_total, "\t%llu message%s received\n");
	p(pim6s_rcv_tooshort, "\t%llu message%s received with too few bytes\n");
	p(pim6s_rcv_badsum, "\t%llu message%s received with bad checksum\n");
	p(pim6s_rcv_badversion, "\t%llu message%s received with bad version\n");
	p(pim6s_rcv_registers, "\t%llu register%s received\n");
	p(pim6s_rcv_badregisters, "\t%llu bad register%s received\n");
	p(pim6s_snd_registers, "\t%llu register%s sent\n");
#undef p
}

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */

void
inet6print(in6, port, proto)
	register struct in6_addr *in6;
	int port;
	char *proto;
{
#define GETSERVBYPORT6(port, proto, ret)\
{\
	if (strcmp((proto), "tcp6") == 0)\
		(ret) = getservbyport((int)(port), "tcp");\
	else if (strcmp((proto), "udp6") == 0)\
		(ret) = getservbyport((int)(port), "udp");\
	else\
		(ret) = getservbyport((int)(port), (proto));\
};
	struct servent *sp = 0;
	char line[80], *cp;
	int width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inet6name(in6));
	cp = index(line, '\0');
	if (!nflag && port)
		GETSERVBYPORT6(port, proto, sp);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%d", ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */

char *
inet6name(in6p)
	struct in6_addr *in6p;
{
	register char *cp;
	static char line[50];
	struct hostent *hp;
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
	if (!nflag && !IN6_IS_ADDR_UNSPECIFIED(in6p)) {
		hp = gethostbyaddr((char *)in6p, sizeof(*in6p), AF_INET6);
		if (hp) {
			if ((cp = index(hp->h_name, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (IN6_IS_ADDR_UNSPECIFIED(in6p))
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else 
		sprintf(line, "%s",
			inet_ntop(AF_INET6, (void *)in6p, ntop_buf,
				sizeof(ntop_buf)));
	return (line);
}

#ifdef TCP6
/*
 * Dump the contents of a TCP6 PCB.
 */
void
tcp6_dump(pcbaddr)
	u_long pcbaddr;
{
	struct tcp6cb tcp6cb;
	int i;

	kread(pcbaddr, (char *)&tcp6cb, sizeof(tcp6cb));

	printf("TCP Protocol Control Block at 0x%08lx:\n\n", pcbaddr);

	printf("Timers:\n");
	for (i = 0; i < TCP6T_NTIMERS; i++)
		printf("\t%s: %u", tcp6timers[i], tcp6cb.t_timer[i]);
	printf("\n\n");

	if (tcp6cb.t_state < 0 || tcp6cb.t_state >= TCP6_NSTATES)
		printf("State: %d", tcp6cb.t_state);
	else
		printf("State: %s", tcp6states[tcp6cb.t_state]);
	printf(", flags 0x%x, in6pcb 0x%lx\n\n", tcp6cb.t_flags,
	    (u_long)tcp6cb.t_in6pcb);

	printf("rxtshift %d, rxtcur %d, dupacks %d\n", tcp6cb.t_rxtshift,
	    tcp6cb.t_rxtcur, tcp6cb.t_dupacks);
	printf("peermaxseg %u, maxseg %u, force %d\n\n", tcp6cb.t_peermaxseg,
	    tcp6cb.t_maxseg, tcp6cb.t_force);

	printf("snd_una %u, snd_nxt %u, snd_up %u\n",
	    tcp6cb.snd_una, tcp6cb.snd_nxt, tcp6cb.snd_up);
	printf("snd_wl1 %u, snd_wl2 %u, iss %u, snd_wnd %llu\n\n",
	    tcp6cb.snd_wl1, tcp6cb.snd_wl2, tcp6cb.iss,
	    (unsigned long long)tcp6cb.snd_wnd);

	printf("rcv_wnd %llu, rcv_nxt %u, rcv_up %u, irs %u\n\n",
	    (unsigned long long)cp6cb.rcv_wnd, tcp6cb.rcv_nxt,
	    tcp6cb.rcv_up, tcp6cb.irs);

	printf("rcv_adv %u, snd_max %u, snd_cwnd %llu, snd_ssthresh %llu\n",
	    tcp6cb.rcv_adv, tcp6cb.snd_max, (unsigned long long)tcp6cb.snd_cwnd,
	    (unsigned long long)tcp6cb.snd_ssthresh);

	printf("idle %d, rtt %d, rtseq %u, srtt %d, rttvar %d, rttmin %d, "
	    "max_sndwnd %llu\n\n", tcp6cb.t_idle, tcp6cb.t_rtt, tcp6cb.t_rtseq,
	    tcp6cb.t_srtt, tcp6cb.t_rttvar, tcp6cb.t_rttmin,
	    (unsigned long long)tcp6cb.max_sndwnd);

	printf("oobflags %d, iobc %d, softerror %d\n\n", tcp6cb.t_oobflags,
	    tcp6cb.t_iobc, tcp6cb.t_softerror);

	printf("snd_scale %d, rcv_scale %d, req_r_scale %d, req_s_scale %d\n",
	    tcp6cb.snd_scale, tcp6cb.rcv_scale, tcp6cb.request_r_scale,
	    tcp6cb.requested_s_scale);
	printf("ts_recent %u, ts_regent_age %d, last_ack_sent %u\n",
	    tcp6cb.ts_recent, tcp6cb.ts_recent_age, tcp6cb.last_ack_sent);
}
#endif

#endif /*INET6*/
