/*	$NetBSD: ip.c,v 1.3 2000/01/04 15:17:00 msaitoh Exp $ */

/*
 * Copyright (c) 1999 Andy Doran <ad@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ip.c,v 1.3 2000/01/04 15:17:00 msaitoh Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <nlist.h>
#include <kvm.h>
#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str);
#define SHOW(stat, row, col)	mvwprintw(wnd, row, col, "%9llu", curstat.stat)

struct mystat {
	struct ipstat i;
	struct udpstat u;
};

static struct mystat curstat;

static struct nlist namelist[] = {
	{ "_ipstat" },
	{ "_udpstat" },
	{ "" }
};

WINDOW *
openip(void)
{

	return (subwin(stdscr, LINES-5-1, 0, 5, 0));
}

void
closeip(w)
	WINDOW *w;
{

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

void
labelip(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);

	LHD(0,  "total packets received");	
	LHD(1,  "  passed to upper layers");
	LHD(2,  "  with bad checksums");		
	LHD(3,  "  too short for header");	
	LHD(4,  "  too short for data");		
	LHD(5,  "  with invalid hlen");		
	LHD(6,  "  with invalid length");	
	LHD(7,  "  with invalid version");
	LHD(8,  "  too large");
	LHD(9,  "  option errors");	
	LHD(10, "  fragments received");
	LHD(11, "  fragments dropped");		
	LHD(12, "  fragments timed out");		
	LHD(13, "  packets reassembled ok");	

	LHD(15, "packets forwarded");	
	LHD(16, "  fast forwarded");	
	LHD(17, "  unreachable dests");	
	LHD(18, "  redirects generated");	

	RHD(0,  "total packets sent");
	RHD(1,  "  generated locally");
	RHD(2,  "  output drops");
	RHD(3,  "  output fragments generated");
	RHD(4,  "  fragmentation failed");	
	RHD(5,  "  destinations unreachable");
	RHD(6,  "  packets output via raw IP");	
	RHD(7,  "  total UDP packets sent");

	RHD(9, "total UDP packets recieved");
	RHD(10, "  too short for header");	
	RHD(11, "  invalid checksum");	
	RHD(12, "  invalid length");	
	RHD(13, "  no socket for dest port");
	RHD(14, "  no socket for broadcast");	
	RHD(15, "  socket buffer full");	
}
	
void
showip(void)
{
	u_quad_t totalout;

	totalout = curstat.i.ips_forward + curstat.i.ips_localout;

	SHOW(i.ips_total, 0, 0);
	mvwprintw(wnd, 0, 35, "%9llu", totalout);
	SHOW(i.ips_delivered, 1, 0);
	SHOW(i.ips_badsum, 2, 0);
	SHOW(i.ips_tooshort, 3, 0);
	SHOW(i.ips_toosmall, 4, 0);
	SHOW(i.ips_badhlen, 5, 0);
	SHOW(i.ips_badlen, 6, 0);
	SHOW(i.ips_badvers, 7, 0);
	SHOW(i.ips_toolong, 8, 0);
	SHOW(i.ips_badoptions, 9, 0);

	SHOW(i.ips_localout, 1, 35);
	SHOW(i.ips_odropped, 2, 35);
	SHOW(i.ips_ofragments, 3, 35);
	SHOW(i.ips_cantfrag, 4, 35);
	SHOW(i.ips_noroute, 5, 35);
	SHOW(i.ips_rawout, 6, 35);
	SHOW(u.udps_opackets, 7, 35);
	
	SHOW(i.ips_fragments, 10, 0);
	SHOW(i.ips_fragdropped, 11, 0);
	SHOW(i.ips_fragtimeout, 12, 0);
	SHOW(i.ips_reassembled, 13, 0);

	SHOW(i.ips_forward, 15, 0);
	SHOW(i.ips_fastforward, 16, 0);
	SHOW(i.ips_cantforward, 17, 0);
	SHOW(i.ips_redirectsent, 18, 0);

	SHOW(u.udps_ipackets, 9, 35);
	SHOW(u.udps_hdrops, 10, 35);
	SHOW(u.udps_badsum, 11, 35);
	SHOW(u.udps_badlen, 12, 35);
	SHOW(u.udps_noport, 13, 35);
	SHOW(u.udps_noportbcast, 14, 35);
	SHOW(u.udps_fullsock, 15, 35);
}

int
initip(void)
{

	if (namelist[0].n_type == 0) {
		if (kvm_nlist(kd, namelist)) {
			nlisterr(namelist);
			return(0);
		}
		if ((namelist[0].n_type | namelist[1].n_type) == 0) {
			error("No namelist");
			return(0);
		}
	}
	return 1;
}

void
fetchip(void)
{

	KREAD((void *)namelist[0].n_value, &curstat.i, sizeof(curstat.i));
	KREAD((void *)namelist[1].n_value, &curstat.u, sizeof(curstat.u));
}
