/*	$NetBSD: ns.c,v 1.13 2001/06/19 13:42:22 wiz Exp $	*/

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
static char sccsid[] = "from: @(#)ns.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: ns.c,v 1.13 2001/06/19 13:42:22 wiz Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/tcp_fsm.h>

#include <netns/ns.h>
#include <netns/ns_pcb.h>
#include <netns/idp.h>
#include <netns/idp_var.h>
#include <netns/ns_error.h>
#include <netns/sp.h>
#include <netns/spidp.h>
#include <netns/spp_timer.h>
#include <netns/spp_var.h>
#define SANAMES
#include <netns/spp_debug.h>

#include <nlist.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "netstat.h"

struct	nspcb nspcb;
struct	sppcb sppcb;
struct	socket sockb;

static char *ns_prpr __P((struct ns_addr *));
static void ns_erputil __P((int, int));

static	int first = 1;

/*
 * Print a summary of connections related to a Network Systems
 * protocol.  For SPP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */

void
nsprotopr(off, name)
	u_long off;
	char *name;
{
	struct nspcb cb;
	struct nspcb *prev, *next;
	int isspp;
	int width = 22;
	if (off == 0)
		return;
	isspp = strcmp(name, "spp") == 0;
	kread(off, (char *)&cb, sizeof (struct nspcb));
	nspcb = cb;
	prev = (struct nspcb *)off;
	if (nspcb.nsp_next == (struct nspcb *)off)
		return;
	for (;nspcb.nsp_next != (struct nspcb *)off; prev = next) {
		u_long ppcb;

		next = nspcb.nsp_next;
		kread((u_long)next, (char *)&nspcb, sizeof (nspcb));
		if (nspcb.nsp_prev != prev) {
			printf("???\n");
			break;
		}
		if (!aflag && ns_nullhost(nspcb.nsp_faddr) ) {
			continue;
		}
		kread((u_long)nspcb.nsp_socket,
				(char *)&sockb, sizeof (sockb));
		ppcb = (u_long) nspcb.nsp_pcb;
		if (ppcb) {
			if (isspp) {
				kread(ppcb, (char *)&sppcb, sizeof (sppcb));
			} else
				continue;
		} else
			if (isspp)
				continue;
		if (first) {
			printf("Active NS connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag) {
				printf("%-8.8s ", "PCB");
				width = 18;
			}
			printf("%-5.5s %-6.6s %-6.6s  %-*.*s %-*.*s %s\n",
			       "Proto", "Recv-Q", "Send-Q",
			       width, width, "Local Address", 
			       width, width, "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			printf("%8lx ", ppcb);
		printf("%-5.5s %6ld %6ld ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		printf("  %-*.*s", width, width, ns_prpr(&nspcb.nsp_laddr));
		printf(" %-*.*s", width, width, ns_prpr(&nspcb.nsp_faddr));
		if (isspp) {
			extern char *tcpstates[];
			if (sppcb.s_state >= TCP_NSTATES)
				printf(" %d", sppcb.s_state);
			else
				printf(" %s", tcpstates[sppcb.s_state]);
		}
		putchar('\n');
		prev = next;
	}
}
#define ANY(x,y,z) \
	((x) ? printf("\t%d %s%s%s -- %s\n",x,y,plural(x),z,"x") : 0)
#define ANYL(x,y,z) \
	((x) ? printf("\t%ld %s%s%s -- %s\n",x,y,plural(x),z,"x") : 0)

/*
 * Dump SPP statistics structure.
 */
void
spp_stats(off, name)
	u_long off;
	char *name;
{
	struct spp_istat spp_istat;
#define sppstat spp_istat.newstats

	if (off == 0)
		return;
	kread(off, (char *)&spp_istat, sizeof (spp_istat));
	printf("%s:\n", name);
	ANY(spp_istat.nonucn, "connection", " dropped due to no new sockets ");
	ANY(spp_istat.gonawy, "connection", " terminated due to our end dying");
	ANY(spp_istat.nonucn, "connection",
	    " dropped due to inability to connect");
	ANY(spp_istat.noconn, "connection",
	    " dropped due to inability to connect");
	ANY(spp_istat.notme, "connection",
	    " incompleted due to mismatched id's");
	ANY(spp_istat.wrncon, "connection", " dropped due to mismatched id's");
	ANY(spp_istat.bdreas, "packet", " dropped out of sequence");
	ANY(spp_istat.lstdup, "packet", " duplicating the highest packet");
	ANY(spp_istat.notyet, "packet", " refused as exceeding allocation");
	ANYL(sppstat.spps_connattempt, "connection", " initiated");
	ANYL(sppstat.spps_accepts, "connection", " accepted");
	ANYL(sppstat.spps_connects, "connection", " established");
	ANYL(sppstat.spps_drops, "connection", " dropped");
	ANYL(sppstat.spps_conndrops, "embryonic connection", " dropped");
	ANYL(sppstat.spps_closed, "connection", " closed (includes drops)");
	ANYL(sppstat.spps_segstimed, "packet", " where we tried to get rtt");
	ANYL(sppstat.spps_rttupdated, "time", " we got rtt");
	ANYL(sppstat.spps_delack, "delayed ack", " sent");
	ANYL(sppstat.spps_timeoutdrop, "connection", " dropped in rxmt timeout");
	ANYL(sppstat.spps_rexmttimeo, "retransmit timeout", "");
	ANYL(sppstat.spps_persisttimeo, "persist timeout", "");
	ANYL(sppstat.spps_keeptimeo, "keepalive timeout", "");
	ANYL(sppstat.spps_keepprobe, "keepalive probe", " sent");
	ANYL(sppstat.spps_keepdrops, "connection", " dropped in keepalive");
	ANYL(sppstat.spps_sndtotal, "total packet", " sent");
	ANYL(sppstat.spps_sndpack, "data packet", " sent");
	ANYL(sppstat.spps_sndbyte, "data byte", " sent");
	ANYL(sppstat.spps_sndrexmitpack, "data packet", " retransmitted");
	ANYL(sppstat.spps_sndrexmitbyte, "data byte", " retransmitted");
	ANYL(sppstat.spps_sndacks, "ack-only packet", " sent");
	ANYL(sppstat.spps_sndprobe, "window probe", " sent");
	ANYL(sppstat.spps_sndurg, "packet", " sent with URG only");
	ANYL(sppstat.spps_sndwinup, "window update-only packet", " sent");
	ANYL(sppstat.spps_sndctrl, "control (SYN|FIN|RST) packet", " sent");
	ANYL(sppstat.spps_sndvoid, "request", " to send a non-existent packet");
	ANYL(sppstat.spps_rcvtotal, "total packet", " received");
	ANYL(sppstat.spps_rcvpack, "packet", " received in sequence");
	ANYL(sppstat.spps_rcvbyte, "byte", " received in sequence");
	ANYL(sppstat.spps_rcvbadsum, "packet", " received with ccksum errs");
	ANYL(sppstat.spps_rcvbadoff, "packet", " received with bad offset");
	ANYL(sppstat.spps_rcvshort, "packet", " received too short");
	ANYL(sppstat.spps_rcvduppack, "duplicate-only packet", " received");
	ANYL(sppstat.spps_rcvdupbyte, "duplicate-only byte", " received");
	ANYL(sppstat.spps_rcvpartduppack, "packet", " with some duplicate data");
	ANYL(sppstat.spps_rcvpartdupbyte, "dup. byte", " in part-dup. packet");
	ANYL(sppstat.spps_rcvoopack, "out-of-order packet", " received");
	ANYL(sppstat.spps_rcvoobyte, "out-of-order byte", " received");
	ANYL(sppstat.spps_rcvpackafterwin, "packet", " with data after window");
	ANYL(sppstat.spps_rcvbyteafterwin, "byte", " rcvd after window");
	ANYL(sppstat.spps_rcvafterclose, "packet", " rcvd after 'close'");
	ANYL(sppstat.spps_rcvwinprobe, "rcvd window probe packet", "");
	ANYL(sppstat.spps_rcvdupack, "rcvd duplicate ack", "");
	ANYL(sppstat.spps_rcvacktoomuch, "rcvd ack", " for unsent data");
	ANYL(sppstat.spps_rcvackpack, "rcvd ack packet", "");
	ANYL(sppstat.spps_rcvackbyte, "byte", " acked by rcvd acks");
	ANYL(sppstat.spps_rcvwinupd, "rcvd window update packet", "");
}
#undef ANY
#undef ANYL
#define ANY(x,y,z)  ((x) ? printf("\t%d %s%s%s\n",x,y,plural(x),z) : 0)

/*
 * Dump IDP statistics structure.
 */
void
idp_stats(off, name)
	u_long off;
	char *name;
{
	struct idpstat idpstat;

	if (off == 0)
		return;
	kread(off, (char *)&idpstat, sizeof (idpstat));
	printf("%s:\n", name);
	ANY(idpstat.idps_toosmall, "packet", " smaller than a header");
	ANY(idpstat.idps_tooshort, "packet", " smaller than advertised");
	ANY(idpstat.idps_badsum, "packet", " with bad checksums");
}

static	struct {
	u_short code;
	char *name;
	char *where;
} ns_errnames[] = {
	{0, "Unspecified Error", " at Destination"},
	{1, "Bad Checksum", " at Destination"},
	{2, "No Listener", " at Socket"},
	{3, "Packet", " Refused due to lack of space at Destination"},
	{01000, "Unspecified Error", " while gatewayed"},
	{01001, "Bad Checksum", " while gatewayed"},
	{01002, "Packet", " forwarded too many times"},
	{01003, "Packet", " too large to be forwarded"},
	{-1, 0, 0},
};

/*
 * Dump NS Error statistics structure.
 */
/*ARGSUSED*/
void
nserr_stats(off, name)
	u_long off;
	char *name;
{
	struct ns_errstat ns_errstat;
	int j;
	int histoprint = 1;
	int z;

	if (off == 0)
		return;
	kread(off, (char *)&ns_errstat, sizeof (ns_errstat));
	printf("NS error statistics:\n");
	ANY(ns_errstat.ns_es_error, "call", " to ns_error");
	ANY(ns_errstat.ns_es_oldshort, "error",
		" ignored due to insufficient addressing");
	ANY(ns_errstat.ns_es_oldns_err, "error request",
		" in response to error packets");
	ANY(ns_errstat.ns_es_tooshort, "error packet",
		" received incomplete");
	ANY(ns_errstat.ns_es_badcode, "error packet",
		" received of unknown type");
	for(j = 0; j < NS_ERR_MAX; j ++) {
		z = ns_errstat.ns_es_outhist[j];
		if (z && histoprint) {
			printf("Output Error Histogram:\n");
			histoprint = 0;
		}
		ns_erputil(z, ns_errstat.ns_es_codes[j]);

	}
	histoprint = 1;
	for(j = 0; j < NS_ERR_MAX; j ++) {
		z = ns_errstat.ns_es_inhist[j];
		if (z && histoprint) {
			printf("Input Error Histogram:\n");
			histoprint = 0;
		}
		ns_erputil(z, ns_errstat.ns_es_codes[j]);
	}
}

static void
ns_erputil(z, c)
	int z, c;
{
	int j;
	char codebuf[30];
	char *name, *where;

	for (j = 0;; j ++) {
		if ((name = ns_errnames[j].name) == 0)
			break;
		if (ns_errnames[j].code == c)
			break;
	}
	if (name == 0) {
		if (c > 01000)
			where = "in transit";
		else
			where = "at destination";
		(void)snprintf(codebuf, sizeof codebuf,
		    "Unknown XNS error code 0%o", c);
		name = codebuf;
	} else
		where =  ns_errnames[j].where;
	ANY(z, name, where);
}

static struct sockaddr_ns ssns = {AF_NS};

static
char *ns_prpr(x)
	struct ns_addr *x;
{
	struct sockaddr_ns *sns = &ssns;

	sns->sns_addr = *x;
	return (ns_print((struct sockaddr *)sns));
}
