/*	$NetBSD: main.c,v 1.62 2006/10/13 16:33:57 elad Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1993\n\
	Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)main.c	8.4 (Berkeley) 3/1/94";
#else
__RCSID("$NetBSD: main.c,v 1.62 2006/10/13 16:33:57 elad Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

struct nlist nl[] = {
#define	N_MBSTAT	0
	{ "_mbstat" },
#define	N_IPSTAT	1
	{ "_ipstat" },
#define	N_TCBTABLE	2
	{ "_tcbtable" },
#define	N_TCPSTAT	3
	{ "_tcpstat" },
#define	N_UDBTABLE	4
	{ "_udbtable" },
#define	N_UDPSTAT	5
	{ "_udpstat" },
#define	N_IFNET		6
	{ "_ifnet" },
#define	N_IMP		7
	{ "_imp_softc" },
#define	N_ICMPSTAT	8
	{ "_icmpstat" },
#define	N_RTSTAT	9
	{ "_rtstat" },
#define	N_UNIXSW	10
	{ "_unixsw" },
#define N_IDP		11
	{ "_nspcb"},
#define N_IDPSTAT	12
	{ "_idpstat"},
#define N_SPPSTAT	13
	{ "_spp_istat"},
#define N_NSERR		14
	{ "_ns_errstat"},
#define	N_CLNPSTAT	15
	{ "_clnp_stat"},
#define	IN_NOTUSED	16
	{ "_tp_inpcb" },
#define	ISO_TP		17
	{ "_tp_refinfo" },
#define	N_TPSTAT	18
	{ "_tp_stat" },
#define	N_ESISSTAT	19
	{ "_esis_stat"},
#define N_NIMP		20
	{ "_nimp"},
#define N_RTREE		21
	{ "_rt_tables"},
#define N_CLTP		22
	{ "_cltb"},
#define N_CLTPSTAT	23
	{ "_cltpstat"},
#define	N_NFILE		24
	{ "_nfile" },
#define	N_FILE		25
	{ "_file" },
#define N_IGMPSTAT	26
	{ "_igmpstat" },
#define N_MRTPROTO	27
	{ "_ip_mrtproto" },
#define N_MRTSTAT	28
	{ "_mrtstat" },
#define N_MFCHASHTBL	29
	{ "_mfchashtbl" },
#define	N_MFCHASH	30
	{ "_mfchash" },
#define N_VIFTABLE	31
	{ "_viftable" },
#define N_MSIZE		32
	{ "_msize" },
#define N_MCLBYTES	33
	{ "_mclbytes" },
#define N_DDPSTAT	34
	{ "_ddpstat"},
#define N_DDPCB		35
	{ "_ddpcb"},
#define N_MBPOOL	36
	{ "_mbpool" },
#define N_MCLPOOL	37
	{ "_mclpool" },
#define N_DIVPCB	38
	{ "_divcb"},
#define N_DIVSTAT	39
	{ "_divstat"},
#define N_IP6STAT	40
	{ "_ip6stat" },
#define N_TCP6STAT	41
	{ "_tcp6stat" },
#define N_UDP6STAT	42
	{ "_udp6stat" },
#define N_ICMP6STAT	43
	{ "_icmp6stat" },
#define N_IPSECSTAT	44
	{ "_ipsecstat" },
#define N_IPSEC6STAT	45
	{ "_ipsec6stat" },
#define N_PIM6STAT	46
	{ "_pim6stat" },
#define N_MRT6PROTO	47
	{ "_ip6_mrtproto" },
#define N_MRT6STAT	48
	{ "_mrt6stat" },
#define N_MF6CTABLE	49
	{ "_mf6ctable" },
#define N_MIF6TABLE	50
	{ "_mif6table" },
#define N_PFKEYSTAT	51
	{ "_pfkeystat" },
#define N_ARPSTAT	52
	{ "_arpstat" },
#define N_RIP6STAT	53
	{ "_rip6stat" },
#define	N_ARPINTRQ	54
	{ "_arpintrq" },
#define	N_IPINTRQ	55
	{ "_ipintrq" },
#define	N_IP6INTRQ	56
	{ "_ip6intrq" },
#define	N_ATINTRQ1	57
	{ "_atintrq1" },
#define	N_ATINTRQ2	58
	{ "_atintrq2" },
#define	N_NSINTRQ	59
	{ "_nsintrq" },
#define	N_CLNLINTRQ	60
	{ "_clnlintrq" },
#define	N_LLCINTRQ	61
	{ "_llcintrq" },
#define	N_HDINTRQ	62
	{ "_hdintrq" },
#define	N_NATMINTRQ	63
	{ "_natmintrq" },
#define	N_PPPOEDISCINQ	64
	{ "_ppoediscinq" },
#define	N_PPPOEINQ	65
	{ "_ppoeinq" },
#define	N_PKINTRQ	66
	{ "_pkintrq" },
#define	N_HARDCLOCK_TICKS 67
	{ "_hardclock_ticks" },
#define N_PIMSTAT	68
	{ "_pimstat" },
#define N_CARPSTAT	69
	{ "_carpstats" },
	{ "" },
};

struct protox {
	u_char	pr_index;		/* index into nlist of cb head */
	u_char	pr_sindex;		/* index into nlist of stat block */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	void	(*pr_cblocks)		/* control blocks printing routine */
			__P((u_long, char *));
	void	(*pr_stats)		/* statistics printing routine */
			__P((u_long, char *));
	void	(*pr_istats)
			__P((char *));	/* per/if statistics printing routine */
	void	(*pr_dump)		/* PCB state dump routine */
			__P((u_long));
	char	*pr_name;		/* well-known name */
} protox[] = {
	{ N_TCBTABLE,	N_TCPSTAT,	1,	protopr,
	  tcp_stats,	NULL,		tcp_dump,	"tcp" },
	{ N_UDBTABLE,	N_UDPSTAT,	1,	protopr,
	  udp_stats,	NULL,		0,	"udp" },
	{ -1,		N_IPSTAT,	1,	0,
	  ip_stats,	NULL,		0,	"ip" },
	{ -1,		N_ICMPSTAT,	1,	0,
	  icmp_stats,	NULL,		0,	"icmp" },
	{ -1,		N_IGMPSTAT,	1,	0,
	  igmp_stats,	NULL,		0,	"igmp" },
	{ -1,		N_CARPSTAT,	1,	0,
	  carp_stats,	NULL,		0,	"carp" },
#ifdef IPSEC
	{ -1,		N_IPSECSTAT,	1,	0,
	  ipsec_switch,	NULL,		0,	"ipsec" },
#endif
	{ -1,		N_PIMSTAT,	1,	0,
	  pim_stats,	NULL,		0,	"pim" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0,	0 }
};

#ifdef INET6
struct protox ip6protox[] = {
	{ -1,		N_IP6STAT,	1,	0,
	  ip6_stats,	ip6_ifstats,	0,	"ip6" },
	{ -1,		N_ICMP6STAT,	1,	0,
	  icmp6_stats,	icmp6_ifstats,	0,	"icmp6" },
#ifdef TCP6
	{ N_TCBTABLE,	N_TCP6STAT,	1,	ip6protopr,
	  tcp6_stats,	NULL,		tcp6_dump,	"tcp6" },
#else
	{ N_TCBTABLE,	N_TCP6STAT,	1,	ip6protopr,
	  tcp_stats,	NULL,		tcp_dump,	"tcp6" },
#endif
	{ N_UDBTABLE,	N_UDP6STAT,	1,	ip6protopr,
	  udp6_stats,	NULL,		0,	"udp6" },
#ifdef IPSEC
	{ -1,		N_IPSEC6STAT,	1,	0,
	  ipsec_switch,	NULL,		0,	"ipsec6" },
#endif
	{ -1,		N_PIM6STAT,	1,	0,
	  pim6_stats,	NULL,		0,	"pim6" },
	{ -1,		N_RIP6STAT,	1,	0,
	  rip6_stats,	NULL,		0,	"rip6" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0,	0 }
};
#endif

struct protox arpprotox[] = {
	{ -1,		N_ARPSTAT,	1,	0,
	  arp_stats,	NULL,		0,	"arp" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0,	0 }
};

#ifdef IPSEC
struct protox pfkeyprotox[] = {
	{ -1,		N_PFKEYSTAT,	1,	0,
	  pfkey_stats,	NULL,		0,	"pfkey" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0,	0 }
};
#endif

#ifndef SMALL
struct protox atalkprotox[] = {
	{ N_DDPCB,	N_DDPSTAT,	1,	atalkprotopr,
	  ddp_stats,	NULL,		0,	"ddp" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0 }
};

#ifdef NS
struct protox nsprotox[] = {
	{ N_IDP,	N_IDPSTAT,	1,	nsprotopr,
	  idp_stats,	NULL,		0,	"idp" },
	{ N_IDP,	N_SPPSTAT,	1,	nsprotopr,
	  spp_stats,	NULL,		0,	"spp" },
	{ -1,		N_NSERR,	1,	0,
	  nserr_stats,	NULL,		0,	"ns_err" },
	{ -1,		-1,		0,	0,
	  0,		NULL,		0 }
};
#endif

struct protox isoprotox[] = {
	{ ISO_TP,	N_TPSTAT,	1,	iso_protopr,
	  tp_stats,	NULL,		0,	"tp" },
	{ N_CLTP,	N_CLTPSTAT,	1,	iso_protopr,
	  cltp_stats,	NULL,		0,	"cltp" },
	{ -1,		N_CLNPSTAT,	1,	 0,
	  clnp_stats,	NULL,		0,	"clnp"},
	{ -1,		N_ESISSTAT,	1,	 0,
	  esis_stats,	NULL,		0,	"esis"},
	{ -1,		-1,		0,	0,
	  0,		NULL,		0,	0 }
};
#endif

struct protox *protoprotox[] = { protox,
#ifdef INET6
				 ip6protox,
#endif
				 arpprotox,
#ifdef IPSEC
				 pfkeyprotox,
#endif
#ifndef SMALL
				 atalkprotox,
#ifdef NS
				 nsprotox,
#endif
				 isoprotox,
#endif
				 NULL };

const struct softintrq {
	const char *siq_name;
	int siq_index;
} softintrq[] = {
	{ "arpintrq", N_ARPINTRQ },
	{ "ipintrq", N_IPINTRQ },
	{ "ip6intrq", N_IP6INTRQ },
	{ "atintrq1", N_ATINTRQ1 },
	{ "atintrq2", N_ATINTRQ2 },
#ifdef NS
	{ "nsintrq", N_NSINTRQ },
#endif
	{ "clnlintrq", N_CLNLINTRQ },
	{ "llcintrq", N_LLCINTRQ },
	{ "hdintrq", N_HDINTRQ },
	{ "natmintrq", N_NATMINTRQ },
	{ "ppoediscinq", N_PPPOEDISCINQ },
	{ "ppoeinq", N_PPPOEINQ },
	{ "pkintrq", N_PKINTRQ },
	{ NULL, -1 },
};

int main __P((int, char *[]));
static void printproto __P((struct protox *, char *));
static void print_softintrq __P((void));
static void usage __P((void));
static struct protox *name2protox __P((char *));
static struct protox *knownname __P((char *));
static void prepare(char *, char *);

kvm_t *kvmd;
gid_t egid;

void
prepare(char *nlistf, char *memf)
{
	char buf[_POSIX2_LINE_MAX];

	/*
	 * Try to figure out if we can use sysctl or not.
	 */
	if (nlistf != NULL && memf != NULL) {
		/* If we have -M and -N, we're not dealing with live memory. */
		use_sysctl = 0;
	} else if (qflag ||
		   rflag ||
		   iflag ||
		   gflag ||
		   Pflag) {
		/* These flags are not yet supported via sysctl(3). */
		use_sysctl = 0;
	} else {
		/* We can use sysctl(3). */
		use_sysctl = 1;
	}

	if (!use_sysctl) {
		(void)setegid(egid);
		kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf);
		(void)setgid(getgid());
		if (kvmd == NULL)
			err(1, "kvm error: %s", buf);
	
		if (kvm_nlist(kvmd, nl) < 0 || nl[0].n_type == 0) {
			if (nlistf)
				errx(1, "%s: no namelist", nlistf);
			else
				errx(1, "no namelist");
		}
	} else
		(void)setgid(getgid());
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct protoent *p;
	struct protox *tp;	/* for printing cblocks & stats */
	int ch;
	char *nlistf = NULL, *memf = NULL;
	char *cp;
	u_long pcbaddr;

	egid = getegid();
	(void)setegid(getgid());
	tp = NULL;
	af = AF_UNSPEC;
	pcbaddr = 0;

	while ((ch = getopt(argc, argv,
	    "AabBdf:ghI:LliM:mN:nP:p:qrsStuvw:")) != -1)
		switch (ch) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'B':
			Bflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
#ifdef NS
			if (strcmp(optarg, "ns") == 0) {
				af = AF_NS;
				break;
			}
#endif
			if (strcmp(optarg, "inet") == 0)
				af = AF_INET;
			else if (strcmp(optarg, "inet6") == 0)
				af = AF_INET6;
			else if (strcmp(optarg, "arp") == 0)
				af = AF_ARP;
			else if (strcmp(optarg, "pfkey") == 0)
				af = PF_KEY;
			else if (strcmp(optarg, "unix") == 0
			    || strcmp(optarg, "local") == 0)
				af = AF_LOCAL;
			else if (strcmp(optarg, "iso") == 0)
				af = AF_ISO;
			else if (strcmp(optarg, "atalk") == 0)
				af = AF_APPLETALK;
			else
				errx(1, "%s: unknown address family",
				    optarg);
			break;
#ifndef SMALL
		case 'g':
			gflag = 1;
			break;
#endif
		case 'I':
			iflag = 1;
			interface = optarg;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'L':
			Lflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			numeric_addr = numeric_port = nflag = 1;
			break;
		case 'P':
			errno = 0;
			pcbaddr = strtoul(optarg, &cp, 16);
			if (*cp != '\0' || errno == ERANGE)
				errx(1, "invalid PCB address %s",
				    optarg);
			Pflag = 1;
			break;
		case 'p':
			if ((tp = name2protox(optarg)) == NULL)
				errx(1, "%s: unknown or uninstrumented protocol",
				    optarg);
			pflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			++sflag;
			break;
		case 'S':
			numeric_addr = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'u':
			af = AF_LOCAL;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			interval = atoi(optarg);
			iflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		if (isdigit((unsigned char)**argv)) {
			interval = atoi(*argv);
			if (interval <= 0)
				usage();
			++argv;
			iflag = 1;
		}
		if (*argv) {
			nlistf = *argv;
			if (*++argv)
				memf = *argv;
		}
	}
#endif

	prepare(nlistf, memf);

#ifndef SMALL
	if (Bflag) {
		if (sflag)
			bpf_stats();
		else
			bpf_dump(interface);
		exit(0);
	}
#endif

	if (mflag) {
		mbpr(nl[N_MBSTAT].n_value,  nl[N_MSIZE].n_value,
		    nl[N_MCLBYTES].n_value, nl[N_MBPOOL].n_value,
		    nl[N_MCLPOOL].n_value);
		exit(0);
	}
	if (Pflag) {
		if (tp == NULL) {
			/* Default to TCP. */
			tp = name2protox("tcp");
		}
		if (tp->pr_dump)
			(*tp->pr_dump)(pcbaddr);
		else
			printf("%s: no PCB dump routine\n", tp->pr_name);
		exit(0);
	}
	if (pflag) {
		if (iflag && tp->pr_istats)
			intpr(interval, nl[N_IFNET].n_value, tp->pr_istats);
		else if (tp->pr_stats)
			(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
				tp->pr_name);
		else
			printf("%s: no stats routine\n", tp->pr_name);
		exit(0);
	}
	if (qflag) {
		print_softintrq();
		exit(0);
	}
	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	sethostent(1);
	setnetent(1);
	if (iflag) {
		if (af != AF_UNSPEC)
			goto protostat;

		intpr(interval, nl[N_IFNET].n_value, NULL);
		exit(0);
	}
	if (rflag) {
		if (sflag)
			rt_stats(use_sysctl ? 0 : nl[N_RTSTAT].n_value);
		else {
			if (use_sysctl)
				p_rttables(af);
			else
				routepr(nl[N_RTREE].n_value);
		}
		exit(0);
	}
#ifndef SMALL
	if (gflag) {
		if (sflag) {
			if (af == AF_INET || af == AF_UNSPEC)
				mrt_stats(nl[N_MRTPROTO].n_value,
					  nl[N_MRTSTAT].n_value);
#ifdef INET6
			if (af == AF_INET6 || af == AF_UNSPEC)
				mrt6_stats(nl[N_MRT6PROTO].n_value,
					   nl[N_MRT6STAT].n_value);
#endif
		}
		else {
			if (af == AF_INET || af == AF_UNSPEC)
				mroutepr(nl[N_MRTPROTO].n_value,
					 nl[N_MFCHASHTBL].n_value,
					 nl[N_MFCHASH].n_value,
					 nl[N_VIFTABLE].n_value);
#ifdef INET6
			if (af == AF_INET6 || af == AF_UNSPEC)
				mroute6pr(nl[N_MRT6PROTO].n_value,
					  nl[N_MF6CTABLE].n_value,
					  nl[N_MIF6TABLE].n_value);
#endif
		}
		exit(0);
	}
#endif
  protostat:
	if (af == AF_INET || af == AF_UNSPEC) {
		setprotoent(1);
		setservent(1);
		/* ugh, this is O(MN) ... why do we do this? */
		while ((p = getprotoent()) != NULL) {
			for (tp = protox; tp->pr_name; tp++)
				if (strcmp(tp->pr_name, p->p_name) == 0)
					break;
			if (tp->pr_name == 0 || tp->pr_wanted == 0)
				continue;
			printproto(tp, p->p_name);
			tp->pr_wanted = 0;
		}
		endprotoent();
		for (tp = protox; tp->pr_name; tp++)
			if (tp->pr_wanted)
				printproto(tp, tp->pr_name);
	}
#ifdef INET6
	if (af == AF_INET6 || af == AF_UNSPEC)
		for (tp = ip6protox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#endif
	if (af == AF_ARP || af == AF_UNSPEC)
		for (tp = arpprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#ifdef IPSEC
	if (af == PF_KEY || af == AF_UNSPEC)
		for (tp = pfkeyprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#endif
#ifndef SMALL
	if (af == AF_APPLETALK || af == AF_UNSPEC)
		for (tp = atalkprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#ifdef NS
	if (af == AF_NS || af == AF_UNSPEC)
		for (tp = nsprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#endif
	if (af == AF_ISO || af == AF_UNSPEC)
		for (tp = isoprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
	if ((af == AF_LOCAL || af == AF_UNSPEC) && !sflag)
		unixpr(nl[N_UNIXSW].n_value);
#endif
	exit(0);
}

/*
 * Print out protocol statistics or control blocks (per sflag).
 * If the interface was not specifically requested, and the symbol
 * is not in the namelist, ignore this one.
 */
static void
printproto(tp, name)
	struct protox *tp;
	char *name;
{
	void (*pr) __P((u_long, char *));
	u_long off;

	if (sflag) {
		if (iflag) {
			if (tp->pr_istats)
				intpr(interval, nl[N_IFNET].n_value,
				      tp->pr_istats);
			return;
		}
		else {
			pr = tp->pr_stats;
			off = nl[tp->pr_sindex].n_value;
		}
	} else {
		pr = tp->pr_cblocks;
		off = nl[tp->pr_index].n_value;
	}
	if (pr != NULL && ((off || af != AF_UNSPEC) || use_sysctl)) {
		(*pr)(off, name);
	}
}

/*
 * Print softintrq status.
 */
void
print_softintrq()
{
	struct ifqueue intrq, *ifq = &intrq;
	const struct softintrq *siq;
	u_long off;

	for (siq = softintrq; siq->siq_name != NULL; siq++) {
		off = nl[siq->siq_index].n_value;
		if (off == 0)
			continue;

		kread(off, (char *)ifq, sizeof(*ifq));
		printf("%s:\n", siq->siq_name);
		printf("\tqueue length: %d\n", ifq->ifq_len);
		printf("\tmaximum queue length: %d\n", ifq->ifq_maxlen);
		printf("\tpackets dropped: %d\n", ifq->ifq_drops);
	}
}

/*
 * Read kernel memory, return 0 on success.
 */
int
kread(addr, buf, size)
	u_long addr;
	char *buf;
	int size;
{

	if (kvm_read(kvmd, addr, buf, size) != size) {
		warnx("%s", kvm_geterr(kvmd));
		return (-1);
	}
	return (0);
}

char *
plural(n)
	int n;
{

	return (n != 1 ? "s" : "");
}

char *
plurales(n)
	int n;
{

	return (n != 1 ? "es" : "");
}

int
get_hardticks(void)
{
	int hardticks;

	kread(nl[N_HARDCLOCK_TICKS].n_value, (char *)&hardticks,
	    sizeof(hardticks));
	return (hardticks);
}

/*
 * Find the protox for the given "well-known" name.
 */
static struct protox *
knownname(name)
	char *name;
{
	struct protox **tpp, *tp;

	for (tpp = protoprotox; *tpp; tpp++)
		for (tp = *tpp; tp->pr_name; tp++)
			if (strcmp(tp->pr_name, name) == 0)
				return (tp);
	return (NULL);
}

/*
 * Find the protox corresponding to name.
 */
static struct protox *
name2protox(name)
	char *name;
{
	struct protox *tp;
	char **alias;			/* alias from p->aliases */
	struct protoent *p;

	/*
	 * Try to find the name in the list of "well-known" names. If that
	 * fails, check if name is an alias for an Internet protocol.
	 */
	if ((tp = knownname(name)) != NULL)
		return (tp);

	setprotoent(1);			/* make protocol lookup cheaper */
	while ((p = getprotoent()) != NULL) {
		/* assert: name not same as p->name */
		for (alias = p->p_aliases; *alias; alias++)
			if (strcmp(name, *alias) == 0) {
				endprotoent();
				return (knownname(p->p_name));
			}
	}
	endprotoent();
	return (NULL);
}

static void
usage()
{
	const char *progname = getprogname();

	(void)fprintf(stderr,
"usage: %s [-Aan] [-f address_family] [-M core] [-N system]\n", progname);
	(void)fprintf(stderr,
"       %s [-bdgiLmnqrsSv] [-f address_family] [-M core] [-N system]\n", 
	progname);
	(void)fprintf(stderr,
"       %s [-dn] [-I interface] [-M core] [-N system] [-w wait]\n", progname);
	(void)fprintf(stderr,
"       %s [-p protocol] [-M core] [-N system]\n", progname);
	(void)fprintf(stderr,
"       %s [-p protocol] [-M core] [-N system] -P pcbaddr\n", progname);
	(void)fprintf(stderr,
"       %s [-p protocol] [-i] [-I Interface] \n", progname);
	(void)fprintf(stderr,
"       %s [-s] [-f address_family] [-i] [-I Interface]\n", progname);
	(void)fprintf(stderr,
"       %s [-s] [-B] [-I interface]\n", progname);
	exit(1);
}
