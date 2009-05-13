/*	$NetBSD: iso.c,v 1.30.8.1 2009/05/13 19:19:59 jym Exp $	*/

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
static char sccsid[] = "from: @(#)iso.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: iso.c,v 1.30.8.1 2009/05/13 19:19:59 jym Exp $");
#endif
#endif /* not lint */

/*******************************************************************************
	          Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*******************************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <errno.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netiso/iso.h>
#include <netiso/iso_errno.h>
#include <netiso/clnp.h>
#include <netiso/esis.h>
#include <netiso/clnp_stat.h>
#include <netiso/argo_debug.h>
#undef satosiso
#include <netiso/tp_param.h>
#include <netiso/tp_states.h>
#include <netiso/tp_pcb.h>
#include <netiso/tp_stat.h>
#include <netiso/iso_pcb.h>
#include <netiso/cltp_var.h>
#include <netiso/cons.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <kvm.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "netstat.h"

static void tprintstat __P((struct tp_stat *, int));
static void isonetprint __P((struct sockaddr_iso *, int));
static void hexprint __P((int, const char *, const char *));
extern void inetprint __P((struct in_addr *, u_int16_t, const char *, int));

/*
 *	Dump esis stats
 */
void
esis_stats(off, name)
	u_long	off;
	const char	*name;
{
	struct esis_stat esis_stat;

	if (off == 0 ||
	    kread(off, (char *)&esis_stat, sizeof (struct esis_stat)))
		return;
	printf("%s:\n", name);

#define	ps(f, m) if (esis_stat.f || sflag <= 1) \
    printf(m, (unsigned long long)esis_stat.f)
#define	ps2(f1, f2, m) if (esis_stat.f1 || esis_stat.f2 || sflag <= 1) \
    printf(m, (unsigned long long)esis_stat.f1, (unsigned long long)esis_stat.f2)

	ps2(es_eshsent, es_eshrcvd, "\t%llu esh sent, %llu esh received\n");
	ps2(es_ishsent, es_ishrcvd, "\t%llu ish sent, %llu ish received\n");
	ps2(es_rdsent, es_rdrcvd, "\t%llu rd sent, %llu rd received\n");
	ps(es_nomem, "\t%llu pdus not sent due to insufficient memory\n");
	ps(es_badcsum, "\t%llu pdus received with bad checksum\n");
	ps(es_badvers, "\t%llu pdus received with bad version number\n");
	ps(es_badtype, "\t%llu pdus received with bad type field\n");
	ps(es_toosmall, "\t%llu short pdus received\n");

#undef ps
#undef ps2
}

/*
 * Dump clnp statistics structure.
 */
void
clnp_stats(off, name)
	u_long off;
	const char *name;
{
	struct clnp_stat clnp_stat;

	if (off == 0 ||
	    kread(off, (char *)&clnp_stat, sizeof (clnp_stat)))
		return;

	printf("%s:\n", name);

#define	ps(f, m) if (clnp_stat.f || sflag <= 1) \
    printf(m, (unsigned long long)clnp_stat.f)
#define	p(f, m) if (clnp_stat.f || sflag <= 1) \
    printf(m, (unsigned long long)clnp_stat.f, plural(clnp_stat.f))

	ps(cns_sent, "\t%llu total packets sent\n");
	ps(cns_fragments, "\t%llu total fragments sent\n");
	ps(cns_total, "\t%llu total packets received\n");
	ps(cns_toosmall, "\t%llu with fixed part of header too small\n");
	ps(cns_badhlen, "\t%llu with header length not reasonable\n");
	p(cns_badcsum, "\t%llu incorrect checksum%s\n");
	ps(cns_badaddr, "\t%llu with unreasonable address lengths\n");
	ps(cns_noseg, "\t%llu with forgotten segmentation information\n");
	ps(cns_noproto, "\t%llu with an incorrect protocol identifier\n");
	ps(cns_badvers, "\t%llu with an incorrect version\n");
	ps(cns_ttlexpired, "\t%llu dropped because the ttl has expired\n");
	ps(cns_cachemiss, "\t%llu clnp cache misses\n");
	ps(cns_congest_set, "\t%llu clnp congestion experience bits set\n");
	ps(cns_congest_rcvd,
	    "\t%llu clnp congestion experience bits received\n");

#undef ps
#undef p
}
/*
 * Dump CLTP statistics structure.
 */
void
cltp_stats(off, name)
	u_long off;
	const char *name;
{
	struct cltpstat cltpstat;

	if (off == 0 ||
	    kread(off, (char *)&cltpstat, sizeof (cltpstat)))
		return;


	printf("%s:\n", name);

#define	p(f, m) if (cltpstat.f || sflag <= 1) \
    printf(m, (unsigned long long)cltpstat.f, plural(cltpstat.f))

	p(cltps_hdrops, "\t%llu incomplete header%s\n");
	p(cltps_badlen,"\t%llu bad data length field%s\n");
	p(cltps_badsum,"\t%llu bad checksum%s\n");

#undef p	     
}

struct	tp_pcb tpcb;
struct	isopcb isopcb;
struct	socket sockb;
union	{
	struct sockaddr_iso	siso;
	char	data[128];
} laddr, faddr;
#define kget(o, p) \
	(kread((u_long)(o), (char *)&p, sizeof (p)))

static	int first = 1;

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
void
iso_protopr(off, name)
	u_long off;
	const char *name;
{
	struct isopcb cb;
	struct isopcb *prev, *next;

	if (off == 0) {
		return;
	}
	if (strcmp(name, "tp") == 0) {
		tp_protopr(off, name);
		return;
	}
	if (kread(off, (char *)&cb, sizeof(cb)))
		return;
	isopcb = cb;
	prev = (struct isopcb *)off;
	if (isopcb.isop_next == (struct isopcb *)off)
		return;
	while (isopcb.isop_next != (struct isopcb *)off) {
		next = isopcb.isop_next;
		kget(next, isopcb);
		if (isopcb.isop_prev != prev) {
			printf("prev %p next %p isop_prev %p isop_next %p???\n",
				prev, next, isopcb.isop_prev, isopcb.isop_next);
			break;
		}
		kget(isopcb.isop_socket, sockb);
		iso_protopr1((u_long)next, 0);
		putchar('\n');
		prev = next;
	}
}

void
iso_protopr1(kern_addr, istp)
	u_long kern_addr;
	int istp;
{
	int width = 22;
	if (first) {
		printf("Active ISO net connections");
		if (aflag)
			printf(" (including servers)");
		putchar('\n');
		if (Aflag) {
			width = 18;
			printf("%-8.8s ", "PCB");
		}
		printf( "%-5.5s %-6.6s %-6.6s  %-*.*s %-*.*s %s\n",
			"Proto", "Recv-Q", "Send-Q",
			width, width, "Local Address", 
			width, width, "Foreign Address", "(state)");
		first = 0;
	}
	if (Aflag)
		printf("%8lx ", 
		    (u_long) (sockb.so_pcb ? (void *)sockb.so_pcb :
		    (void *)kern_addr));
	printf("%-5.5s %6ld %6ld ", "tp",
	    sockb.so_rcv.sb_cc, sockb.so_snd.sb_cc);
	if (istp && tpcb.tp_lsuffixlen) {
		hexprint(tpcb.tp_lsuffixlen, tpcb.tp_lsuffix, "()");
		printf("\t");
	} else if (isopcb.isop_laddr == 0)
		printf("*.*\t");
	else {
		if ((char *)isopcb.isop_laddr == ((char *)kern_addr) +
		    offsetof(struct isopcb, isop_sladdr))
			laddr.siso = isopcb.isop_sladdr;
		else
			kget(isopcb.isop_laddr, laddr);
		isonetprint((struct sockaddr_iso *)&laddr, 1);
	}
	if (istp && tpcb.tp_fsuffixlen) {
		hexprint(tpcb.tp_fsuffixlen, tpcb.tp_fsuffix, "()");
		printf("\t");
	} else if (isopcb.isop_faddr == 0)
		printf("*.*\t");
	else {
		if ((char *)isopcb.isop_faddr == ((char *)kern_addr) +
		    offsetof(struct isopcb, isop_sfaddr))
			faddr.siso = isopcb.isop_sfaddr;
		else
			kget(isopcb.isop_faddr, faddr);
		isonetprint((struct sockaddr_iso *)&faddr, 0);
	}
}

void
tp_protopr(off, name)
	u_long off;
	const char *name;
{
	extern const char * const tp_sstring[];	/* from sys/netiso/tp_astring.c */
	struct tp_ref *tpr, *tpr_base;
	struct tp_refinfo tpkerninfo;
	int size;

	kget(off, tpkerninfo);
	size = tpkerninfo.tpr_size * sizeof (*tpr);
	tpr_base = (struct tp_ref *)malloc(size);
	if (tpr_base == 0)
		return;
	kread((u_long)(tpkerninfo.tpr_base), (char *)tpr_base, size);
	for (tpr = tpr_base; tpr < tpr_base + tpkerninfo.tpr_size; tpr++) {
		if (tpr->tpr_pcb == 0)
			continue;
		kget(tpr->tpr_pcb, tpcb);
		if (tpcb.tp_state == ST_ERROR)
			printf("undefined tpcb state: %p\n", tpr->tpr_pcb);
		if (!aflag &&
			(tpcb.tp_state == TP_LISTENING ||
			 tpcb.tp_state == TP_CLOSED ||
			 tpcb.tp_state == TP_REFWAIT)) {
			continue;
		}
		kget(tpcb.tp_sock, sockb);
		if (tpcb.tp_npcb) switch(tpcb.tp_netservice) {
			case IN_CLNS:
				tp_inproto((u_long)tpkerninfo.tpr_base);
				break;
			default:
				kget(tpcb.tp_npcb, isopcb);
				iso_protopr1((u_long)tpcb.tp_npcb, 1);
				break;
		}
		if (tpcb.tp_state >= tp_NSTATES)
			printf(" %d", tpcb.tp_state);
		else
			printf(" %-12.12s", tp_sstring[tpcb.tp_state]);
		putchar('\n');
	}
	free(tpr_base);
}

void
tp_inproto(pcb)
	u_long pcb;
{
	struct inpcb inpcb;

	kget(tpcb.tp_npcb, inpcb);
	if (!aflag && inet_lnaof(inpcb.inp_faddr) == INADDR_ANY)
		return;
	if (Aflag)
		printf("%8lx ", pcb);
	printf("%-5.5s %6ld %6ld ", "tpip",
	    sockb.so_rcv.sb_cc, sockb.so_snd.sb_cc);
	inetprint(&inpcb.inp_laddr, inpcb.inp_lport, "tp", 1);
	inetprint(&inpcb.inp_faddr, inpcb.inp_fport, "tp", 1);
}

/*
 * Pretty print an iso address (net address + port).
 * If the numeric_addr or numeric_port were specified,
 * use numbers instead of names.
 */

#ifdef notdef
char *
isonetname(iso)
	struct iso_addr *iso;
{
	struct sockaddr_iso sa;
	struct iso_hostent *ihe = 0;
	struct iso_hostent *iso_gethostentrybyaddr();
	struct iso_hostent *iso_getserventrybytsel();
	struct iso_hostent Ihe;
	static char line[80];

	memset(line, 0, sizeof(line));
	if (iso->isoa_afi) {
		sa.siso_family = AF_ISO;
		sa.siso_addr = *iso;
		sa.siso_tsuffix = 0;

		if (!numeric_addr)
			ihe = iso_gethostentrybyaddr(&sa, 0, 0);
		if (ihe) {
			Ihe = *ihe;
			ihe = &Ihe;
			(void)snprintf(line, sizeof line, "%s", ihe->isoh_hname);
		} else {
			(void)snprintf(line, sizeof line, "%s", iso_ntoa(iso));
		}
	} else {
		(void)snprintf(line, sizeof line, "*");
	}
	return (line);
}

static void
isonetprint(iso, sufx, sufxlen, islocal)
	struct iso_addr *iso;
	char *sufx;
	u_short	sufxlen;
	int islocal;
{
	struct iso_hostent *iso_getserventrybytsel(), *ihe;
	struct iso_hostent Ihe;
	char *line, *cp;
	int Alen = Aflag ? 18 : 22;

	line = isonetname(iso);
	cp = strchr(line, '\0');
	ihe = (struct iso_hostent *)0;

	if (islocal)
		islocal = 20;
	else
		islocal = 22 + Alen;

	if (Aflag)
		islocal += 10 ;

	if (!numeric_addr) {
		if ((cp - line) > 10) {
			cp = line + 10;
			memset(cp, 0, sizeof(line)-10);
		}
	}

	*cp++ = '.';
	if (sufxlen) {
		if (!Aflag && !numeric_port &&
		    (ihe = iso_getserventrybytsel(sufx, sufxlen))) {
			Ihe = *ihe;
			ihe = &Ihe;
		}
		if (ihe && (strlen(ihe->isoh_aname) > 0)) {
			sprintf(cp, "%s", ihe->isoh_aname);	/* XXX */
		} else  {
			iso_sprinttsel(cp, sufx, sufxlen);
		}
	} else
		sprintf(cp, "*");
	/*
	fprintf(stdout, Aflag?" %-18.18s":" %-22.22s", line);
	*/

	if( strlen(line) > Alen ) {
		fprintf(stdout, " %s", line);
		fprintf(stdout, "\n %*.s", islocal+Alen," ");
	} else {
		fprintf(stdout, " %-*.*s", Alen, Alen,line);
	}
}
#endif

#ifdef notdef
static void
x25_protopr(off, name)
	u_long off;
	char *name;
{
	static char *xpcb_states[] = {
		"CLOSED",
		"LISTENING",
		"CLOSING",
		"CONNECTING",
		"ACKWAIT",
		"OPEN",
	};
	struct isopcb *prev, *next;
	struct x25_pcb xpcb;

	if (off == 0) {
		return;
	}
	kread(off, &xpcb, sizeof (struct x25_pcb));
	prev = (struct isopcb *)off;
	if (xpcb.x_next == (struct isopcb *)off)
		return;
	while (xpcb.x_next != (struct isopcb *)off) {
		next = isopcb.isop_next;
		kread((u_long)next, &xpcb, sizeof (struct x25_pcb));
		if (xpcb.x_prev != prev) {
			printf("???\n");
			break;
		}
		kread((u_long)xpcb.x_socket, &sockb, sizeof (sockb));

		if (!aflag &&
			xpcb.x_state == LISTENING ||
			xpcb.x_state == TP_CLOSED ) {
			prev = next;
			continue;
		}
		if (first) {
			printf("Active X25 net connections");
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
		printf("%-5.5s %6d %6d ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		isonetprint(&xpcb.x_laddr.siso_addr, &xpcb.x_lport,
			sizeof(xpcb.x_lport), 1);
		isonetprint(&xpcb.x_faddr.siso_addr, &xpcb.x_fport,
			sizeof(xpcb.x_lport), 0);
		if (xpcb.x_state < 0 || xpcb.x_state >= x25_NSTATES)
			printf(" 0x0x0x0x0x0x0x0x0x%x", xpcb.x_state);
		else
			printf(" %-12.12s", xpcb_states[xpcb.x_state]);
		putchar('\n');
		prev = next;
	}
}
#endif

struct	tp_stat tp_stat;

void
tp_stats(off, name)
	u_long off;
	const char *name;
{

	if (off == 0) {
		printf("TP not configured\n\n");
		return;
	}
	printf("%s:\n", name);
	kget(off, tp_stat);
	tprintstat(&tp_stat, 8);
}

struct tpstatpr {
	size_t off;
	const char *text;
};

#define o(f) offsetof(struct tp_stat, f)

static struct tpstatpr	tpstatpr_r[] = {
{ o(ts_param_ignored),	"\t%*s%ld variable parameter%s ignored\n"},
{ o(ts_inv_pcode),	"\t%*s%ld invalid parameter code%s\n"},
{ o(ts_inv_pval),	"\t%*s%ld invalid parameter value%s\n"},
{ o(ts_inv_dutype),	"\t%*s%ld invalid dutype%s\n"},
{ o(ts_negotfailed),	"\t%*s%ld negotiation failure%s\n"},
{ o(ts_inv_dref),	"\t%*s%ld invalid destination reference%s\n"},
{ o(ts_inv_sufx),	"\t%*s%ld invalid suffix parameter%s\n"},
{ o(ts_inv_length),	"\t%*s%ld invalid length%s\n"},
{ o(ts_bad_csum),	"\t%*s%ld invalid checksum%s\n"},
{ o(ts_dt_ooo),		"\t%*s%ld DT%s out of order\n"},
{ o(ts_dt_niw),		"\t%*s%ld DT%s not in window\n"},
{ o(ts_dt_dup),		"\t%*s%ld duplicate DT%s\n"},
{ o(ts_xpd_niw),	"\t%*s%ld XPD%s not in window\n"},
{ o(ts_xpd_dup),	"\t%*s%ld XPD%s w/o credit to stash\n"},
{ o(ts_lcdt_reduced),	"\t%*s%ld time%s local credit reneged\n"},
{ o(ts_concat_rcvd),	"\t%*s%ld concatenated TPDU%s\n"},
{ 0,			NULL }
};

static struct tpstatpr	tpstatpr_s[] = {
{ o(ts_xpdmark_del),	"\t%*s%ld XPD mark%s discarded\n"},
{ o(ts_xpd_intheway),	"\t%*sXPD stopped data flow %ld time%s\n"},
{ o(ts_zfcdt),		"\t%*s%ld time%s foreign window closed\n"},
{ 0,			NULL }
};

static struct tpstatpr	tpstatpr_m[] = {
{ o(ts_mb_small),	"\t%*s%ld small mbuf%s\n"},
{ o(ts_mb_cluster),	"\t%*s%ld cluster%s\n"},
{ o(ts_quench),		"\t%*s%ld source quench%s\n"},
{ o(ts_rcvdecbit),	"\t%*s%ld dec bit%s\n"},
{ o(ts_eot_input),	"\t%*s%ld EOT%s rcvd\n"},
{ o(ts_EOT_sent),	"\t%*s%ld EOT%s sent\n"},
{ o(ts_eot_user),	"\t%*s%ld EOT indication%s\n"},
{ 0,			NULL }
};

static struct tpstatpr	tpstatpr_c[] = {
{ o(ts_xtd_fmt),	"\t%*s%ld connection%s used extended format\n"},
{ o(ts_use_txpd),	"\t%*s%ld connection%s allowed transport expedited data\n"},
{ o(ts_csum_off),	"\t%*s%ld connection%s turned off checksumming\n"},
{ o(ts_conn_gaveup),	"\t%*s%ld connection%s dropped due to retrans limit\n"},
{ o(ts_tp4_conn),	"\t%*s%ld tp 4 connection%s\n"},
{ o(ts_tp0_conn),	"\t%*s%ld tp 0 connection%s\n"},
{ 0,			NULL }
};

static struct tpstatpr	tpstatpr_p[] = {
{ o(ts_zdebug),		"\t%*s%6ld CC%s sent to zero dref\n"},
	/* SAME LINE AS ABOVE */
{ o(ts_ydebug),		"\t%*s%6ld random DT%s dropped\n"},
{ o(ts_vdebug),		"\t%*s%6ld illegally large XPD TPDU%s\n"},
{ o(ts_ldebug),		"\t%*s%6ld faked reneging%s of cdt\n"},
{ 0,			NULL }
};

static struct tpstatpr	tpstatpr_A[] = {
{ o(ts_ackreason[_ACK_DONT_]),		"\t%*s%6ld not acked immediately%s\n"},
{ o(ts_ackreason[_ACK_STRAT_EACH_]),	"\t%*s%6ld strategy==each%s\n"},
{ o(ts_ackreason[_ACK_STRAT_FULLWIN_]),	"\t%*s%6ld strategy==fullwindow%s\n"},
{ o(ts_ackreason[_ACK_DUP_]),		"\t%*s%6ld duplicate DT%s\n"},
{ o(ts_ackreason[_ACK_EOT_]),		"\t%*s%6ld EOTSDU%s\n"},
{ o(ts_ackreason[_ACK_REORDER_]),	"\t%*s%6ld reordered DT%s\n"},
{ o(ts_ackreason[_ACK_USRRCV_]),	"\t%*s%6ld user rcvd%s\n"},
{ o(ts_ackreason[_ACK_FCC_]),		"\t%*s%6ld fcc reqd%s\n"},
{ 0,			NULL }
};
#undef o

static void
tprintstat(s, indent)
	struct tp_stat *s;
	int indent;
{
	int j, tpfirst, tpfirst2;

	static const char *rttname[]= {
		"~LOCAL, PDN",
		"~LOCAL,~PDN",
		" LOCAL,~PDN",
		" LOCAL, PDN"
		};

	/*
	 * Loop through a struct tpstatpr; if any value is non-zero,
	 * or sflag<=1, print the header if first, and then print the value.
	 */
#define pgroup(group, header) \
for (j = 0, tpfirst=1; group[j].text; j++) \
	if (*(u_long*)((u_long)s + group[j].off) || \
	    sflag <=1) { \
		if (tpfirst) { \
			fprintf(stdout, \
			    header,indent," "); \
			tpfirst=0; \
		} \
		fprintf(stdout, group[j].text, indent, " ", \
		    *(u_long*)((u_long)s + group[j].off), \
		    plural(*(u_long*)((u_long)s + group[j].off))); \
	}


	pgroup(tpstatpr_r, "%*sReceived:\n"); 
	pgroup(tpstatpr_s, "%*sSending:\n");

	pgroup(tpstatpr_m, "%*sMiscellaneous:\n");
	/* print the mbuf chain statistics under the Misc. header */
	tpfirst2=1;
	for( j=0, tpfirst2=1; j<=8; j++) {
		if (s->ts_mb_len_distr[j] || s->ts_mb_len_distr[j<<1] ||
		    sflag <=1) {
			if (tpfirst) {
				fprintf(stdout, "%*sMiscellaneous:\n",
				    indent, " ");
				tpfirst=0;
			}
			if (tpfirst2) {
				fprintf(stdout,
				    "\t%*sM:L ( M mbuf chains of length L)\n",
				    indent, " ");
				tpfirst2=0;
			}
			if (j==0)
				fprintf(stdout, "\t%*s%ld: over 16\n",
				    indent, " ", s->ts_mb_len_distr[0]);
			else
				fprintf(stdout,
				    "\t%*s%ld: %d\t\t%ld: %d\n", indent, " ",
				    s->ts_mb_len_distr[j],j,
				    s->ts_mb_len_distr[j<<1],j<<1);
		}
	}

	pgroup(tpstatpr_c, "%*sConnections:\n");

	tpfirst=1;
	for (j = 0; j <= 3; j++) {
		if (s->ts_rtt[j] || s->ts_rtt[j] || s->ts_rtv[j] ||
		    s->ts_rtv[j] || sflag<=1) {
			if (tpfirst) {
				fprintf(stdout,
				    "\n%*sRound trip times, "
				    "listed in ticks:\n"
				    "\t%*s%11.11s  %12.12s | "
				    "%12.12s | %s\n", indent, " ",
				    indent, " ",
				    "Category",
				    "Smoothed avg", "Deviation",
				    "Deviation/Avg");
				tpfirst=0;
			}
			fprintf(stdout,
			    "\t%*s%11.11s: %-11d | %-11d | %-11d | %-11d\n",
			    indent, " ",
			    rttname[j], s->ts_rtt[j],  s->ts_rtt[j],
			    s->ts_rtv[j], s->ts_rtv[j]);
		}
	}
	
	
	if (s->ts_tpdu_rcvd || s->ts_pkt_rcvd || s->ts_recv_drop ||
	    s->ts_DT_rcvd || s->ts_AK_rcvd || s->ts_DR_rcvd || s->ts_CR_rcvd ||
	    s->ts_XPD_rcvd || s->ts_XAK_rcvd || s->ts_DC_rcvd ||
	    s->ts_CC_rcvd || s->ts_ER_rcvd || sflag<=1 ) {
		fprintf(stdout, "\n%*sTpdus RECVD "
		    "[%ld valid, %3.6f %% of total (%ld); %ld dropped]\n",
		    indent," ", 	s->ts_tpdu_rcvd,
		    ((s->ts_pkt_rcvd > 0) ?
			((100 * (float)s->ts_tpdu_rcvd)/(float)s->ts_pkt_rcvd)
			: 0),
		    s->ts_pkt_rcvd, s->ts_recv_drop );
		fprintf(stdout,
		    "\t%*sDT  %6ld   AK  %6ld   DR  %4ld   CR  %4ld \n",
		    indent, " ", s->ts_DT_rcvd, s->ts_AK_rcvd, s->ts_DR_rcvd,
		    s->ts_CR_rcvd);
		fprintf(stdout, "\t%*sXPD %6ld   XAK %6ld   DC  %4ld   "
		    "CC  %4ld   ER  %4ld\n", indent, " ", s->ts_XPD_rcvd,
		    s->ts_XAK_rcvd, s->ts_DC_rcvd, s->ts_CC_rcvd,
		    s->ts_ER_rcvd);
	}

	if (s->ts_tpdu_sent || s->ts_send_drop || s->ts_DT_sent ||
	    s->ts_AK_sent || s->ts_DR_sent || s->ts_CR_sent ||
	    s->ts_XPD_sent || s->ts_XAK_sent || s->ts_DC_sent ||
	    s->ts_CC_sent || s->ts_ER_sent || sflag<=1 ) {
		fprintf(stdout,
		    "\n%*sTpdus SENT [%ld total, %ld dropped]\n",  indent, " ",
		    s->ts_tpdu_sent, s->ts_send_drop);
		fprintf(stdout,
		    "\t%*sDT  %6ld   AK  %6ld   DR  %4ld   CR  %4ld \n",
		    indent, " ", s->ts_DT_sent, s->ts_AK_sent, s->ts_DR_sent,
		    s->ts_CR_sent);
		fprintf(stdout, "\t%*sXPD %6ld   XAK %6ld   DC  %4ld   "
		    "CC  %4ld   ER  %4ld\n",  indent, " ", s->ts_XPD_sent,
		    s->ts_XAK_sent, s->ts_DC_sent, s->ts_CC_sent,
		    s->ts_ER_sent);
	}

	if (s->ts_retrans_cr || s->ts_retrans_cc || s->ts_retrans_dr ||
	    s->ts_retrans_dt || s->ts_retrans_xpd || sflag<=1) {
		fprintf(stdout,
		    "\n%*sRetransmissions:\n", indent, " ");
#define PERCENT(X,Y) (((Y)>0)?((100 *(float)(X)) / (float) (Y)):0)
		fprintf(stdout,
		    "\t%*sCR  %6ld   CC  %6ld   DR  %6ld \n", indent, " ",
		    s->ts_retrans_cr, s->ts_retrans_cc, s->ts_retrans_dr);
		fprintf(stdout,
		    "\t%*sDT  %6ld (%5.2f%%)\n", indent, " ",
		    s->ts_retrans_dt,
		    PERCENT(s->ts_retrans_dt, s->ts_DT_sent));
		fprintf(stdout,
		    "\t%*sXPD %6ld (%5.2f%%)\n",  indent, " ",
		    s->ts_retrans_xpd,
		    PERCENT(s->ts_retrans_xpd, s->ts_XPD_sent));
#undef PERCENT
	}

	if (s->ts_Eticks || s->ts_Eset || s->ts_Eexpired || s->ts_Ecan_act ||
	    sflag<=1) {
		fprintf(stdout,
		    "\n%*sE Timers: [%6ld ticks]\n", indent, " ",
		    s->ts_Eticks);
		fprintf(stdout,
		    "%*s%6ld timer%s set \t%6ld timer%s expired "
		    "\t%6ld timer%s cancelled\n", indent, " ",
		    s->ts_Eset, plural(s->ts_Eset),
		    s->ts_Eexpired, plural(s->ts_Eexpired),
		    s->ts_Ecan_act, plural(s->ts_Ecan_act));
	}

	if (s->ts_Cset || s->ts_Cexpired || s->ts_Ccan_act ||
	    s->ts_Ccan_inact || sflag<=1) {
		fprintf(stdout,
		    "\n%*sC Timers: [%6ld ticks]\n",  indent, " ",
		    s->ts_Cticks);
		fprintf(stdout,
		    "%*s%6ld timer%s set \t%6ld timer%s expired "
		    "\t%6ld timer%s cancelled\n",
		    indent, " ",
		    s->ts_Cset, plural(s->ts_Cset),
		    s->ts_Cexpired, plural(s->ts_Cexpired),
		    s->ts_Ccan_act, plural(s->ts_Ccan_act));
		fprintf(stdout,
		    "%*s%6ld inactive timer%s cancelled\n", indent, " ",
		    s->ts_Ccan_inact, plural(s->ts_Ccan_inact));
	}

	pgroup(tpstatpr_p, "\n%*sPathological debugging activity:\n");

	pgroup(tpstatpr_A, "\n%*sACK reasons:\n");
#undef pgroup
}
#ifndef SSEL
#define SSEL(s) ((s)->siso_tlen + TSEL(s))
#define PSEL(s) ((s)->siso_slen + SSEL(s))
#endif

static void
isonetprint(siso, islocal)
	struct sockaddr_iso *siso;
	int islocal;
{

	hexprint(siso->siso_nlen, siso->siso_addr.isoa_genaddr, "{}");
	if (siso->siso_tlen || siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_tlen, TSEL(siso), "()");
	if (siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_slen, SSEL(siso), "[]");
	if (siso->siso_plen)
		hexprint(siso->siso_plen, PSEL(siso), "<>");
	putchar(' ');
}

static char hexlist[] = "0123456789abcdef", obuf[128];

static void
hexprint(n, buf, delim)
	int n;
	const char *buf;
	const char *delim;
{
	const u_char *in = (const u_char *)buf, *top = in + n;
	char *out = obuf;
	int i;

	if (n == 0)
		return;
	while (in < top) {
		i = *in++;
		*out++ = '.';
		if (i > 0xf) {
			out[1] = hexlist[i & 0xf];
			i >>= 4;
			out[0] = hexlist[i];
			out += 2;
		} else
			*out++ = hexlist[i];
	}
	*obuf = *delim; *out++ = delim[1]; *out = 0;
	printf("%s", obuf);
}
