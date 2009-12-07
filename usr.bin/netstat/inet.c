/*	$NetBSD: inet.c,v 1.92 2009/12/07 18:48:45 christos Exp $	*/

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
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
__RCSID("$NetBSD: inet.c,v 1.92 2009/12/07 18:48:45 christos Exp $");
#endif
#endif /* not lint */

#define	_CALLOUT_PRIVATE	/* for defs in sys/callout.h */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_icmp.h>

#ifdef INET6
#include <netinet/ip6.h>
#endif

#include <netinet/icmp_var.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/pim_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/ip_carp.h>
#include <netinet/udp_var.h>
#include <net/pfvar.h>
#include <net/if_pfsync.h>

#include <arpa/inet.h>
#include <kvm.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include "netstat.h"

char	*inetname(struct in_addr *);
void	inetprint(struct in_addr *, u_int16_t, const char *, int);

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
static int width;
static int compact;

static void
protoprhdr(void)
{
	printf("Active Internet connections");
	if (aflag)
		printf(" (including servers)");
	putchar('\n');
	if (Aflag)
		printf("%-8.8s ", "PCB");
	printf("%-5.5s %-6.6s %-6.6s %s%-*.*s %-*.*s %s\n",
		"Proto", "Recv-Q", "Send-Q", compact ? "" : " ",
		width, width, "Local Address",
		width, width, "Foreign Address",
		"State");
}

static void
protopr0(intptr_t ppcb, u_long rcv_sb_cc, u_long snd_sb_cc,
	 struct in_addr *laddr, u_int16_t lport,
	 struct in_addr *faddr, u_int16_t fport,
	 short t_state, const char *name, int inp_flags)
{
	static const char *shorttcpstates[] = {
		"CLOSED",	"LISTEN",	"SYNSEN",	"SYSRCV",
		"ESTABL",	"CLWAIT",	"FWAIT1",	"CLOSNG",
		"LASTAK",	"FWAIT2",	"TMWAIT",
	};
	int istcp;

	istcp = strcmp(name, "tcp") == 0;

	if (Aflag) {
		printf("%8" PRIxPTR " ", ppcb);
	}
	printf("%-5.5s %6ld %6ld%s", name, rcv_sb_cc, snd_sb_cc,
	       compact ? "" : " ");
	if (numeric_port) {
		inetprint(laddr, lport, name, 1);
		inetprint(faddr, fport, name, 1);
	} else if (inp_flags & INP_ANONPORT) {
		inetprint(laddr, lport, name, 1);
		inetprint(faddr, fport, name, 0);
	} else {
		inetprint(laddr, lport, name, 0);
		inetprint(faddr, fport, name, 0);
	}
	if (istcp) {
		if (t_state < 0 || t_state >= TCP_NSTATES)
			printf(" %d", t_state);
		else
			printf(" %s", compact ? shorttcpstates[t_state] :
			       tcpstates[t_state]);
	}
	putchar('\n');
}

void
protopr(u_long off, const char *name)
{
	struct inpcbtable table;
	struct inpcb *head, *next, *prev;
	struct inpcb inpcb;
	struct tcpcb tcpcb;
	struct socket sockb;
	int istcp;
	static int first = 1;

	compact = 0;
	if (Aflag) {
		if (!numeric_addr)
			width = 18;
		else {
			width = 21;
			compact = 1;
		}
	} else
		width = 22;

	if (use_sysctl) {
		struct kinfo_pcb *pcblist;
		int mib[8];
		size_t namelen = 0, size = 0, i;
		char *mibname = NULL;

		memset(mib, 0, sizeof(mib));

		if (asprintf(&mibname, "net.inet.%s.pcblist", name) == -1)
			err(1, "asprintf");

		/* get dynamic pcblist node */
		if (sysctlnametomib(mibname, mib, &namelen) == -1)
			err(1, "sysctlnametomib: %s", mibname);

		if (sysctl(mib, sizeof(mib) / sizeof(*mib), NULL, &size,
			   NULL, 0) == -1)
			err(1, "sysctl (query)");

		if ((pcblist = malloc(size)) == NULL)
			err(1, "malloc");
		memset(pcblist, 0, size);

	        mib[6] = sizeof(*pcblist);
        	mib[7] = size / sizeof(*pcblist);

		if (sysctl(mib, sizeof(mib) / sizeof(*mib), pcblist,
			   &size, NULL, 0) == -1)
			err(1, "sysctl (copy)");

		for (i = 0; i < size / sizeof(*pcblist); i++) {
			struct sockaddr_in src, dst;

			memcpy(&src, &pcblist[i].ki_s, sizeof(src));
			memcpy(&dst, &pcblist[i].ki_d, sizeof(dst));

			if (!aflag &&
			    inet_lnaof(dst.sin_addr) == INADDR_ANY)
				continue;

			if (first) {
				protoprhdr();
				first = 0;
			}

	                protopr0((intptr_t) pcblist[i].ki_ppcbaddr,
				 pcblist[i].ki_rcvq, pcblist[i].ki_sndq,
				 &src.sin_addr, src.sin_port,
				 &dst.sin_addr, dst.sin_port,
				 pcblist[i].ki_tstate, name,
				 pcblist[i].ki_pflags);
		}

		free(pcblist);
		return;
	}

	if (off == 0)
		return;
	istcp = strcmp(name, "tcp") == 0;
	kread(off, (char *)&table, sizeof table);
	prev = head =
	    (struct inpcb *)&((struct inpcbtable *)off)->inpt_queue.cqh_first;
	next = (struct inpcb *)table.inpt_queue.cqh_first;

	while (next != head) {
		kread((u_long)next, (char *)&inpcb, sizeof inpcb);
		if ((struct inpcb *)inpcb.inp_queue.cqe_prev != prev) {
			printf("???\n");
			break;
		}
		prev = next;
		next = (struct inpcb *)inpcb.inp_queue.cqe_next;

		if (inpcb.inp_af != AF_INET)
			continue;

		if (!aflag &&
		    inet_lnaof(inpcb.inp_faddr) == INADDR_ANY)
			continue;
		kread((u_long)inpcb.inp_socket, (char *)&sockb, sizeof (sockb));
		if (istcp) {
			kread((u_long)inpcb.inp_ppcb,
			    (char *)&tcpcb, sizeof (tcpcb));
		}

		if (first) {
			protoprhdr();
			first = 0;
		}

		protopr0(istcp ? (intptr_t) inpcb.inp_ppcb : (intptr_t) prev,
			 sockb.so_rcv.sb_cc, sockb.so_snd.sb_cc,
			 &inpcb.inp_laddr, inpcb.inp_lport,
			 &inpcb.inp_faddr, inpcb.inp_fport,
			 tcpcb.t_state, name, inpcb.inp_flags);
	}
}

/*
 * Dump TCP statistics structure.
 */
void
tcp_stats(u_long off, const char *name)
{
	uint64_t tcpstat[TCP_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(tcpstat);

		if (sysctlbyname("net.inet.tcp.stats", tcpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf ("%s:\n", name);

#define	ps(f, m) if (tcpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)tcpstat[f])
#define	p(f, m) if (tcpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)tcpstat[f], plural(tcpstat[f]))
#define	p2(f1, f2, m) if (tcpstat[f1] || tcpstat[f2] || sflag <= 1) \
    printf(m, (unsigned long long)tcpstat[f1], plural(tcpstat[f1]), \
    (unsigned long long)tcpstat[f2], plural(tcpstat[f2]))
#define	p2s(f1, f2, m) if (tcpstat[f1] || tcpstat[f2] || sflag <= 1) \
    printf(m, (unsigned long long)tcpstat[f1], plural(tcpstat[f1]), \
    (unsigned long long)tcpstat[f2])
#define	p3(f, m) if (tcpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)tcpstat[f], plurales(tcpstat[f]))

	p(TCP_STAT_SNDTOTAL, "\t%llu packet%s sent\n");
	p2(TCP_STAT_SNDPACK,TCP_STAT_SNDBYTE,
		"\t\t%llu data packet%s (%llu byte%s)\n");
	p2(TCP_STAT_SNDREXMITPACK, TCP_STAT_SNDREXMITBYTE,
		"\t\t%llu data packet%s (%llu byte%s) retransmitted\n");
	p2s(TCP_STAT_SNDACKS, TCP_STAT_DELACK,
		"\t\t%llu ack-only packet%s (%llu delayed)\n");
	p(TCP_STAT_SNDURG, "\t\t%llu URG only packet%s\n");
	p(TCP_STAT_SNDPROBE, "\t\t%llu window probe packet%s\n");
	p(TCP_STAT_SNDWINUP, "\t\t%llu window update packet%s\n");
	p(TCP_STAT_SNDCTRL, "\t\t%llu control packet%s\n");
	p(TCP_STAT_SELFQUENCH,
	    "\t\t%llu send attempt%s resulted in self-quench\n");
	p(TCP_STAT_RCVTOTAL, "\t%llu packet%s received\n");
	p2(TCP_STAT_RCVACKPACK, TCP_STAT_RCVACKBYTE,
		"\t\t%llu ack%s (for %llu byte%s)\n");
	p(TCP_STAT_RCVDUPACK, "\t\t%llu duplicate ack%s\n");
	p(TCP_STAT_RCVACKTOOMUCH, "\t\t%llu ack%s for unsent data\n");
	p2(TCP_STAT_RCVPACK, TCP_STAT_RCVBYTE,
		"\t\t%llu packet%s (%llu byte%s) received in-sequence\n");
	p2(TCP_STAT_RCVDUPPACK, TCP_STAT_RCVDUPBYTE,
		"\t\t%llu completely duplicate packet%s (%llu byte%s)\n");
	p(TCP_STAT_PAWSDROP, "\t\t%llu old duplicate packet%s\n");
	p2(TCP_STAT_RCVPARTDUPPACK, TCP_STAT_RCVPARTDUPBYTE,
		"\t\t%llu packet%s with some dup. data (%llu byte%s duped)\n");
	p2(TCP_STAT_RCVOOPACK, TCP_STAT_RCVOOBYTE,
		"\t\t%llu out-of-order packet%s (%llu byte%s)\n");
	p2(TCP_STAT_RCVPACKAFTERWIN, TCP_STAT_RCVBYTEAFTERWIN,
		"\t\t%llu packet%s (%llu byte%s) of data after window\n");
	p(TCP_STAT_RCVWINPROBE, "\t\t%llu window probe%s\n");
	p(TCP_STAT_RCVWINUPD, "\t\t%llu window update packet%s\n");
	p(TCP_STAT_RCVAFTERCLOSE, "\t\t%llu packet%s received after close\n");
	p(TCP_STAT_RCVBADSUM, "\t\t%llu discarded for bad checksum%s\n");
	p(TCP_STAT_RCVBADOFF, "\t\t%llu discarded for bad header offset field%s\n");
	ps(TCP_STAT_RCVSHORT, "\t\t%llu discarded because packet too short\n");
	p(TCP_STAT_CONNATTEMPT, "\t%llu connection request%s\n");
	p(TCP_STAT_ACCEPTS, "\t%llu connection accept%s\n");
	p(TCP_STAT_CONNECTS,
		"\t%llu connection%s established (including accepts)\n");
	p2(TCP_STAT_CLOSED, TCP_STAT_DROPS,
		"\t%llu connection%s closed (including %llu drop%s)\n");
	p(TCP_STAT_CONNDROPS, "\t%llu embryonic connection%s dropped\n");
	p(TCP_STAT_DELAYED_FREE, "\t%llu delayed free%s of tcpcb\n");
	p2(TCP_STAT_RTTUPDATED, TCP_STAT_SEGSTIMED,
		"\t%llu segment%s updated rtt (of %llu attempt%s)\n");
	p(TCP_STAT_REXMTTIMEO, "\t%llu retransmit timeout%s\n");
	p(TCP_STAT_TIMEOUTDROP,
		"\t\t%llu connection%s dropped by rexmit timeout\n");
	p2(TCP_STAT_PERSISTTIMEO, TCP_STAT_PERSISTDROPS,
	   "\t%llu persist timeout%s (resulting in %llu dropped "
		"connection%s)\n");
	p(TCP_STAT_KEEPTIMEO, "\t%llu keepalive timeout%s\n");
	p(TCP_STAT_KEEPPROBE, "\t\t%llu keepalive probe%s sent\n");
	p(TCP_STAT_KEEPDROPS, "\t\t%llu connection%s dropped by keepalive\n");
	p(TCP_STAT_PREDACK, "\t%llu correct ACK header prediction%s\n");
	p(TCP_STAT_PREDDAT, "\t%llu correct data packet header prediction%s\n");
	p3(TCP_STAT_PCBHASHMISS, "\t%llu PCB hash miss%s\n");
	ps(TCP_STAT_NOPORT, "\t%llu dropped due to no socket\n");
	p(TCP_STAT_CONNSDRAINED, "\t%llu connection%s drained due to memory "
		"shortage\n");
	p(TCP_STAT_PMTUBLACKHOLE, "\t%llu PMTUD blackhole%s detected\n");

	p(TCP_STAT_BADSYN, "\t%llu bad connection attempt%s\n");
	ps(TCP_STAT_SC_ADDED, "\t%llu SYN cache entries added\n");
	p(TCP_STAT_SC_COLLISIONS, "\t\t%llu hash collision%s\n");
	ps(TCP_STAT_SC_COMPLETED, "\t\t%llu completed\n");
	ps(TCP_STAT_SC_ABORTED, "\t\t%llu aborted (no space to build PCB)\n");
	ps(TCP_STAT_SC_TIMED_OUT, "\t\t%llu timed out\n");
	ps(TCP_STAT_SC_OVERFLOWED, "\t\t%llu dropped due to overflow\n");
	ps(TCP_STAT_SC_BUCKETOVERFLOW, "\t\t%llu dropped due to bucket overflow\n");
	ps(TCP_STAT_SC_RESET, "\t\t%llu dropped due to RST\n");
	ps(TCP_STAT_SC_UNREACH, "\t\t%llu dropped due to ICMP unreachable\n");
	ps(TCP_STAT_SC_DELAYED_FREE, "\t\t%llu delayed free of SYN cache "
		"entries\n");
	p(TCP_STAT_SC_RETRANSMITTED, "\t%llu SYN,ACK%s retransmitted\n");
	p(TCP_STAT_SC_DUPESYN, "\t%llu duplicate SYN%s received for entries "
		"already in the cache\n");
	p(TCP_STAT_SC_DROPPED, "\t%llu SYN%s dropped (no route or no space)\n");
	p(TCP_STAT_BADSIG, "\t%llu packet%s with bad signature\n");
	p(TCP_STAT_GOODSIG, "\t%llu packet%s with good signature\n");

	p(TCP_STAT_ECN_SHS, "\t%llu sucessful ECN handshake%s\n");
	p(TCP_STAT_ECN_CE, "\t%llu packet%s with ECN CE bit\n");
	p(TCP_STAT_ECN_ECT, "\t%llu packet%s ECN ECT(0) bit\n");
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
udp_stats(u_long off, const char *name)
{
	uint64_t udpstat[UDP_NSTATS];
	u_quad_t delivered;

	if (use_sysctl) {
		size_t size = sizeof(udpstat);

		if (sysctlbyname("net.inet.udp.stats", udpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf ("%s:\n", name);

#define	ps(f, m) if (udpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)udpstat[f])
#define	p(f, m) if (udpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)udpstat[f], plural(udpstat[f]))
#define	p3(f, m) if (udpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)udpstat[f], plurales(udpstat[f]))

	p(UDP_STAT_IPACKETS, "\t%llu datagram%s received\n");
	ps(UDP_STAT_HDROPS, "\t%llu with incomplete header\n");
	ps(UDP_STAT_BADLEN, "\t%llu with bad data length field\n");
	ps(UDP_STAT_BADSUM, "\t%llu with bad checksum\n");
	ps(UDP_STAT_NOPORT, "\t%llu dropped due to no socket\n");
	p(UDP_STAT_NOPORTBCAST,
	  "\t%llu broadcast/multicast datagram%s dropped due to no socket\n");
	ps(UDP_STAT_FULLSOCK, "\t%llu dropped due to full socket buffers\n");
	delivered = udpstat[UDP_STAT_IPACKETS] -
		    udpstat[UDP_STAT_HDROPS] -
		    udpstat[UDP_STAT_BADLEN] -
		    udpstat[UDP_STAT_BADSUM] -
		    udpstat[UDP_STAT_NOPORT] -
		    udpstat[UDP_STAT_NOPORTBCAST] -
		    udpstat[UDP_STAT_FULLSOCK];
	if (delivered || sflag <= 1)
		printf("\t%llu delivered\n", (unsigned long long)delivered);
	p3(UDP_STAT_PCBHASHMISS, "\t%llu PCB hash miss%s\n");
	p(UDP_STAT_OPACKETS, "\t%llu datagram%s output\n");

#undef ps
#undef p
#undef p3
}

/*
 * Dump IP statistics structure.
 */
void
ip_stats(u_long off, const char *name)
{
	uint64_t ipstat[IP_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(ipstat);

		if (sysctlbyname("net.inet.ip.stats", ipstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define	ps(f, m) if (ipstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)ipstat[f])
#define	p(f, m) if (ipstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)ipstat[f], plural(ipstat[f]))

	p(IP_STAT_TOTAL, "\t%llu total packet%s received\n");
	p(IP_STAT_BADSUM, "\t%llu bad header checksum%s\n");
	ps(IP_STAT_TOOSMALL, "\t%llu with size smaller than minimum\n");
	ps(IP_STAT_TOOSHORT, "\t%llu with data size < data length\n");
	ps(IP_STAT_TOOLONG, "\t%llu with length > max ip packet size\n");
	ps(IP_STAT_BADHLEN, "\t%llu with header length < data size\n");
	ps(IP_STAT_BADLEN, "\t%llu with data length < header length\n");
	ps(IP_STAT_BADOPTIONS, "\t%llu with bad options\n");
	ps(IP_STAT_BADVERS, "\t%llu with incorrect version number\n");
	p(IP_STAT_FRAGMENTS, "\t%llu fragment%s received\n");
	p(IP_STAT_FRAGDROPPED, "\t%llu fragment%s dropped (dup or out of space)\n");
	p(IP_STAT_RCVMEMDROP, "\t%llu fragment%s dropped (out of ipqent)\n");
	p(IP_STAT_BADFRAGS, "\t%llu malformed fragment%s dropped\n");
	p(IP_STAT_FRAGTIMEOUT, "\t%llu fragment%s dropped after timeout\n");
	p(IP_STAT_REASSEMBLED, "\t%llu packet%s reassembled ok\n");
	p(IP_STAT_DELIVERED, "\t%llu packet%s for this host\n");
	p(IP_STAT_NOPROTO, "\t%llu packet%s for unknown/unsupported protocol\n");
	p(IP_STAT_FORWARD, "\t%llu packet%s forwarded");
	p(IP_STAT_FASTFORWARD, " (%llu packet%s fast forwarded)");
	if (ipstat[IP_STAT_FORWARD] || sflag <= 1)
		putchar('\n');
	p(IP_STAT_CANTFORWARD, "\t%llu packet%s not forwardable\n");
	p(IP_STAT_REDIRECTSENT, "\t%llu redirect%s sent\n");
	p(IP_STAT_NOGIF, "\t%llu packet%s no matching gif found\n");
	p(IP_STAT_LOCALOUT, "\t%llu packet%s sent from this host\n");
	p(IP_STAT_RAWOUT, "\t%llu packet%s sent with fabricated ip header\n");
	p(IP_STAT_ODROPPED, "\t%llu output packet%s dropped due to no bufs, etc.\n");
	p(IP_STAT_NOROUTE, "\t%llu output packet%s discarded due to no route\n");
	p(IP_STAT_FRAGMENTED, "\t%llu output datagram%s fragmented\n");
	p(IP_STAT_OFRAGMENTS, "\t%llu fragment%s created\n");
	p(IP_STAT_CANTFRAG, "\t%llu datagram%s that can't be fragmented\n");
	p(IP_STAT_BADADDR, "\t%llu datagram%s with bad address in header\n");
#undef ps
#undef p
}

static	const char *icmpnames[] = {
	"echo reply",
	"#1",
	"#2",
	"destination unreachable",
	"source quench",
	"routing redirect",
	"alternate host address",
	"#7",
	"echo",
	"router advertisement",
	"router solicitation",
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
icmp_stats(u_long off, const char *name)
{
	uint64_t icmpstat[ICMP_NSTATS];
	int i, first;

	if (use_sysctl) {
		size_t size = sizeof(icmpstat);

		if (sysctlbyname("net.inet.icmp.stats", icmpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define	p(f, m) if (icmpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)icmpstat[f], plural(icmpstat[f]))

	p(ICMP_STAT_ERROR, "\t%llu call%s to icmp_error\n");
	p(ICMP_STAT_OLDICMP,
	    "\t%llu error%s not generated because old message was icmp\n");
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat[ICMP_STAT_OUTHIST + i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %llu\n", icmpnames[i],
			   (unsigned long long)icmpstat[ICMP_STAT_OUTHIST + i]);
		}
	p(ICMP_STAT_BADCODE, "\t%llu message%s with bad code fields\n");
	p(ICMP_STAT_TOOSHORT, "\t%llu message%s < minimum length\n");
	p(ICMP_STAT_CHECKSUM, "\t%llu bad checksum%s\n");
	p(ICMP_STAT_BADLEN, "\t%llu message%s with bad length\n");
	p(ICMP_STAT_BMCASTECHO, "\t%llu multicast echo request%s ignored\n");
	p(ICMP_STAT_BMCASTTSTAMP, "\t%llu multicast timestamp request%s ignored\n");
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat[ICMP_STAT_INHIST + i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %llu\n", icmpnames[i],
			    (unsigned long long)icmpstat[ICMP_STAT_INHIST + i]);
		}
	p(ICMP_STAT_REFLECT, "\t%llu message response%s generated\n");
	p(ICMP_STAT_PMTUCHG, "\t%llu path MTU change%s\n");
#undef p
}

/*
 * Dump IGMP statistics structure.
 */
void
igmp_stats(u_long off, const char *name)
{
	uint64_t igmpstat[IGMP_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(igmpstat);

		if (sysctlbyname("net.inet.igmp.stats", igmpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define	p(f, m) if (igmpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)igmpstat[f], plural(igmpstat[f]))
#define	py(f, m) if (igmpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)igmpstat[f], igmpstat[f] != 1 ? "ies" : "y")
	p(IGMP_STAT_RCV_TOTAL, "\t%llu message%s received\n");
        p(IGMP_STAT_RCV_TOOSHORT, "\t%llu message%s received with too few bytes\n");
        p(IGMP_STAT_RCV_BADSUM, "\t%llu message%s received with bad checksum\n");
        py(IGMP_STAT_RCV_QUERIES, "\t%llu membership quer%s received\n");
        py(IGMP_STAT_RCV_BADQUERIES, "\t%llu membership quer%s received with invalid field(s)\n");
        p(IGMP_STAT_RCV_REPORTS, "\t%llu membership report%s received\n");
        p(IGMP_STAT_RCV_BADREPORTS, "\t%llu membership report%s received with invalid field(s)\n");
        p(IGMP_STAT_RCV_OURREPORTS, "\t%llu membership report%s received for groups to which we belong\n");
        p(IGMP_STAT_SND_REPORTS, "\t%llu membership report%s sent\n");
#undef p
#undef py
}

/*
 * Dump CARP statistics structure.
 */
void
carp_stats(u_long off, const char *name)
{
	uint64_t carpstat[CARP_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(carpstat);

		if (sysctlbyname("net.inet.carp.stats", carpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define p(f, m) if (carpstat[f] || sflag <= 1) \
	printf(m, carpstat[f], plural(carpstat[f]))
#define p2(f, m) if (carpstat[f] || sflag <= 1) \
	printf(m, carpstat[f])

	p(CARP_STAT_IPACKETS, "\t%" PRIu64 " packet%s received (IPv4)\n");
	p(CARP_STAT_IPACKETS6, "\t%" PRIu64 " packet%s received (IPv6)\n");
	p(CARP_STAT_BADIF,
	    "\t\t%" PRIu64 " packet%s discarded for bad interface\n");
	p(CARP_STAT_BADTTL,
	    "\t\t%" PRIu64 " packet%s discarded for wrong TTL\n");
	p(CARP_STAT_HDROPS, "\t\t%" PRIu64 " packet%s shorter than header\n");
	p(CARP_STAT_BADSUM, "\t\t%" PRIu64
		" packet%s discarded for bad checksum\n");
	p(CARP_STAT_BADVER,
	    "\t\t%" PRIu64 " packet%s discarded with a bad version\n");
	p2(CARP_STAT_BADLEN,
	    "\t\t%" PRIu64 " discarded because packet was too short\n");
	p(CARP_STAT_BADAUTH,
	    "\t\t%" PRIu64 " packet%s discarded for bad authentication\n");
	p(CARP_STAT_BADVHID, "\t\t%" PRIu64 " packet%s discarded for bad vhid\n");
	p(CARP_STAT_BADADDRS, "\t\t%" PRIu64
		" packet%s discarded because of a bad address list\n");
	p(CARP_STAT_OPACKETS, "\t%" PRIu64 " packet%s sent (IPv4)\n");
	p(CARP_STAT_OPACKETS6, "\t%" PRIu64 " packet%s sent (IPv6)\n");
	p2(CARP_STAT_ONOMEM,
	    "\t\t%" PRIu64 " send failed due to mbuf memory error\n");
#undef p
#undef p2
}

/*
 * Dump PFSYNC statistics structure.
 */
void
pfsync_stats(u_long off, const char *name)
{
	uint64_t pfsyncstat[PFSYNC_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(pfsyncstat);

		if (sysctlbyname("net.inet.pfsync.stats", pfsyncstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define p(f, m) if (pfsyncstat[f] || sflag <= 1) \
	printf(m, pfsyncstat[f], plural(pfsyncstat[f]))
#define p2(f, m) if (pfsyncstat[f] || sflag <= 1) \
	printf(m, pfsyncstat[f])

	p(PFSYNC_STAT_IPACKETS, "\t%" PRIu64 " packet%s received (IPv4)\n");
	p(PFSYNC_STAT_IPACKETS6,"\t%" PRIu64 " packet%s received (IPv6)\n");
	p(PFSYNC_STAT_BADIF, "\t\t%" PRIu64 " packet%s discarded for bad interface\n");
	p(PFSYNC_STAT_BADTTL, "\t\t%" PRIu64 " packet%s discarded for bad ttl\n");
	p(PFSYNC_STAT_HDROPS, "\t\t%" PRIu64 " packet%s shorter than header\n");
	p(PFSYNC_STAT_BADVER, "\t\t%" PRIu64 " packet%s discarded for bad version\n");
	p(PFSYNC_STAT_BADAUTH, "\t\t%" PRIu64 " packet%s discarded for bad HMAC\n");
	p(PFSYNC_STAT_BADACT,"\t\t%" PRIu64 " packet%s discarded for bad action\n");
	p(PFSYNC_STAT_BADLEN, "\t\t%" PRIu64 " packet%s discarded for short packet\n");
	p(PFSYNC_STAT_BADVAL, "\t\t%" PRIu64 " state%s discarded for bad values\n");
	p(PFSYNC_STAT_STALE, "\t\t%" PRIu64 " stale state%s\n");
	p(PFSYNC_STAT_BADSTATE, "\t\t%" PRIu64 " failed state lookup/insert%s\n");
	p(PFSYNC_STAT_OPACKETS, "\t%" PRIu64 " packet%s sent (IPv4)\n");
	p(PFSYNC_STAT_OPACKETS6, "\t%" PRIu64 " packet%s sent (IPv6)\n");
	p2(PFSYNC_STAT_ONOMEM, "\t\t%" PRIu64 " send failed due to mbuf memory error\n");
	p2(PFSYNC_STAT_OERRORS, "\t\t%" PRIu64 " send error\n");
#undef p
#undef p2
}

/*
 * Dump PIM statistics structure.
 */
void
pim_stats(u_long off, const char *name)
{
	struct pimstat pimstat;

	if (off == 0)
		return;
	if (kread(off, (char *)&pimstat, sizeof (pimstat)) != 0) {
		/* XXX: PIM is probably not enabled in the kernel */
		return;
	}

	printf("%s:\n", name);

#define	p(f, m) if (pimstat.f || sflag <= 1) \
	printf(m, (unsigned long long)pimstat.f, plural(pimstat.f))

	p(pims_rcv_total_msgs, "\t%llu message%s received\n");
	p(pims_rcv_total_bytes, "\t%llu byte%s received\n");
	p(pims_rcv_tooshort, "\t%llu message%s received with too few bytes\n");
        p(pims_rcv_badsum, "\t%llu message%s received with bad checksum\n");
	p(pims_rcv_badversion, "\t%llu message%s received with bad version\n");
	p(pims_rcv_registers_msgs, "\t%llu data register message%s received\n");
	p(pims_rcv_registers_bytes, "\t%llu data register byte%s received\n");
	p(pims_rcv_registers_wrongiif, "\t%llu data register message%s received on wrong iif\n");
	p(pims_rcv_badregisters, "\t%llu bad register%s received\n");
	p(pims_snd_registers_msgs, "\t%llu data register message%s sent\n");
	p(pims_snd_registers_bytes, "\t%llu data register byte%s sent\n");
#undef p
}

/*
 * Dump the ARP statistics structure.
 */
void
arp_stats(u_long off, const char *name)
{
	uint64_t arpstat[ARP_NSTATS];

	if (use_sysctl) {
		size_t size = sizeof(arpstat);

		if (sysctlbyname("net.inet.arp.stats", arpstat, &size,
				 NULL, 0) == -1)
			return;
	} else {
		warnx("%s stats not available via KVM.", name);
		return;
	}

	printf("%s:\n", name);

#define	ps(f, m) if (arpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)arpstat[f])
#define	p(f, m) if (arpstat[f] || sflag <= 1) \
    printf(m, (unsigned long long)arpstat[f], plural(arpstat[f]))

	p(ARP_STAT_SNDTOTAL, "\t%llu packet%s sent\n");
	p(ARP_STAT_SNDREPLY, "\t\t%llu reply packet%s\n");
	p(ARP_STAT_SENDREQUEST, "\t\t%llu request packet%s\n");

	p(ARP_STAT_RCVTOTAL, "\t%llu packet%s received\n");
	p(ARP_STAT_RCVREPLY, "\t\t%llu reply packet%s\n");
	p(ARP_STAT_RCVREQUEST, "\t\t%llu valid request packet%s\n");
	p(ARP_STAT_RCVMCAST, "\t\t%llu broadcast/multicast packet%s\n");
	p(ARP_STAT_RCVBADPROTO, "\t\t%llu packet%s with unknown protocol type\n");
	p(ARP_STAT_RCVBADLEN, "\t\t%llu packet%s with bad (short) length\n");
	p(ARP_STAT_RCVZEROTPA, "\t\t%llu packet%s with null target IP address\n");
	p(ARP_STAT_RCVZEROSPA, "\t\t%llu packet%s with null source IP address\n");
	ps(ARP_STAT_RCVNOINT, "\t\t%llu could not be mapped to an interface\n");
	p(ARP_STAT_RCVLOCALSHA, "\t\t%llu packet%s sourced from a local hardware "
	    "address\n");
	p(ARP_STAT_RCVBCASTSHA, "\t\t%llu packet%s with a broadcast "
	    "source hardware address\n");
	p(ARP_STAT_RCVLOCALSPA, "\t\t%llu duplicate%s for a local IP address\n");
	p(ARP_STAT_RCVOVERPERM, "\t\t%llu attempt%s to overwrite a static entry\n");
	p(ARP_STAT_RCVOVERINT, "\t\t%llu packet%s received on wrong interface\n");
	p(ARP_STAT_RCVOVER, "\t\t%llu entry%s overwritten\n");
	p(ARP_STAT_RCVLENCHG, "\t\t%llu change%s in hardware address length\n");

	p(ARP_STAT_DFRTOTAL, "\t%llu packet%s deferred pending ARP resolution\n");
	ps(ARP_STAT_DFRSENT, "\t\t%llu sent\n");
	ps(ARP_STAT_DFRDROPPED, "\t\t%llu dropped\n");

	p(ARP_STAT_ALLOCFAIL, "\t%llu failure%s to allocate llinfo\n");

#undef ps
#undef p
}

/*
 * Pretty print an Internet address (net address + port).
 * Take numeric_addr and numeric_port into consideration.
 */
void
inetprint(struct in_addr *in, uint16_t port, const char *proto,
	  int port_numeric)
{
	struct servent *sp = 0;
	char line[80], *cp;
	size_t space;

	(void)snprintf(line, sizeof line, "%.*s.",
	    (Aflag && !numeric_addr) ? 12 : 16, inetname(in));
	cp = strchr(line, '\0');
	if (!port_numeric && port)
		sp = getservbyport((int)port, proto);
	space = sizeof line - (cp-line);
	if (sp || port == 0)
		(void)snprintf(cp, space, "%s", sp ? sp->s_name : "*");
	else
		(void)snprintf(cp, space, "%u", ntohs(port));
	(void)printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If numeric_addr has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(struct in_addr *inp)
{
	char *cp;
	static char line[50];
	struct hostent *hp;
	struct netent *np;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first && !numeric_addr) {
		first = 0;
		if (gethostname(domain, sizeof domain) == 0) {
			domain[sizeof(domain) - 1] = '\0';
			if ((cp = strchr(domain, '.')))
				(void) strlcpy(domain, cp + 1, sizeof(domain));
			else
				domain[0] = 0;
		} else
			domain[0] = 0;
	}
	cp = 0;
	if (!numeric_addr && inp->s_addr != INADDR_ANY) {
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
				if ((cp = strchr(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
	}
	if (inp->s_addr == INADDR_ANY)
		strlcpy(line, "*", sizeof line);
	else if (cp)
		strlcpy(line, cp, sizeof line);
	else {
		inp->s_addr = ntohl(inp->s_addr);
#define C(x)	((x) & 0xff)
		(void)snprintf(line, sizeof line, "%u.%u.%u.%u",
		    C(inp->s_addr >> 24), C(inp->s_addr >> 16),
		    C(inp->s_addr >> 8), C(inp->s_addr));
#undef C
	}
	return (line);
}

/*
 * Dump the contents of a TCP PCB.
 */
void
tcp_dump(u_long pcbaddr)
{
	callout_impl_t *ci;
	struct tcpcb tcpcb;
	int i, hardticks;

	kread(pcbaddr, (char *)&tcpcb, sizeof(tcpcb));
	hardticks = get_hardticks();

	printf("TCP Protocol Control Block at 0x%08lx:\n\n", pcbaddr);

	printf("Timers:\n");
	for (i = 0; i < TCPT_NTIMERS; i++) {
		ci = (callout_impl_t *)&tcpcb.t_timer[i];
		printf("\t%s: %d", tcptimers[i],
		    (ci->c_flags & CALLOUT_PENDING) ?
		    ci->c_time - hardticks : 0);
	}
	printf("\n\n");

	if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
		printf("State: %d", tcpcb.t_state);
	else
		printf("State: %s", tcpstates[tcpcb.t_state]);
	printf(", flags 0x%x, inpcb 0x%lx, in6pcb 0x%lx\n\n", tcpcb.t_flags,
	    (u_long)tcpcb.t_inpcb, (u_long)tcpcb.t_in6pcb);

	printf("rxtshift %d, rxtcur %d, dupacks %d\n", tcpcb.t_rxtshift,
	    tcpcb.t_rxtcur, tcpcb.t_dupacks);
	printf("peermss %u, ourmss %u, segsz %u\n\n", tcpcb.t_peermss,
	    tcpcb.t_ourmss, tcpcb.t_segsz);

	printf("snd_una %u, snd_nxt %u, snd_up %u\n",
	    tcpcb.snd_una, tcpcb.snd_nxt, tcpcb.snd_up);
	printf("snd_wl1 %u, snd_wl2 %u, iss %u, snd_wnd %lu\n\n",
	    tcpcb.snd_wl1, tcpcb.snd_wl2, tcpcb.iss, tcpcb.snd_wnd);

	printf("rcv_wnd %lu, rcv_nxt %u, rcv_up %u, irs %u\n\n",
	    tcpcb.rcv_wnd, tcpcb.rcv_nxt, tcpcb.rcv_up, tcpcb.irs);

	printf("rcv_adv %u, snd_max %u, snd_cwnd %lu, snd_ssthresh %lu\n",
	    tcpcb.rcv_adv, tcpcb.snd_max, tcpcb.snd_cwnd, tcpcb.snd_ssthresh);

	printf("rcvtime %u, rtttime %u, rtseq %u, srtt %d, rttvar %d, "
	    "rttmin %d, max_sndwnd %lu\n\n", tcpcb.t_rcvtime, tcpcb.t_rtttime,
	    tcpcb.t_rtseq, tcpcb.t_srtt, tcpcb.t_rttvar, tcpcb.t_rttmin,
	    tcpcb.max_sndwnd);

	printf("oobflags %d, iobc %d, softerror %d\n\n", tcpcb.t_oobflags,
	    tcpcb.t_iobc, tcpcb.t_softerror);

	printf("snd_scale %d, rcv_scale %d, req_r_scale %d, req_s_scale %d\n",
	    tcpcb.snd_scale, tcpcb.rcv_scale, tcpcb.request_r_scale,
	    tcpcb.requested_s_scale);
	printf("ts_recent %u, ts_regent_age %d, last_ack_sent %u\n",
	    tcpcb.ts_recent, tcpcb.ts_recent_age, tcpcb.last_ack_sent);
}
