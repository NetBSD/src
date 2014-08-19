/*	$NetBSD: ip.c,v 1.17.28.1 2014/08/20 00:05:04 tls Exp $	*/

/*
 * Copyright (c) 1999, 2000 Andrew Doran <ad@NetBSD.org>
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
__RCSID("$NetBSD: ip.c,v 1.17.28.1 2014/08/20 00:05:04 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str);
#define SHOW(stat, row, col) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat.stat)

struct mystat {
	uint64_t i[IP_NSTATS];
	uint64_t u[UDP_NSTATS];
};

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static struct mystat curstat;
static struct mystat oldstat;
static struct mystat newstat;

WINDOW *
openip(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closeip(WINDOW *w)
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

	RHD(9, "total UDP packets received");
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

	totalout = curstat.i[IP_STAT_FORWARD] + curstat.i[IP_STAT_LOCALOUT];

	SHOW(i[IP_STAT_TOTAL], 0, 0);
	mvwprintw(wnd, 0, 35, "%9llu", (unsigned long long)totalout);
	SHOW(i[IP_STAT_DELIVERED], 1, 0);
	SHOW(i[IP_STAT_BADSUM], 2, 0);
	SHOW(i[IP_STAT_TOOSHORT], 3, 0);
	SHOW(i[IP_STAT_TOOSMALL], 4, 0);
	SHOW(i[IP_STAT_BADHLEN], 5, 0);
	SHOW(i[IP_STAT_BADLEN], 6, 0);
	SHOW(i[IP_STAT_BADVERS], 7, 0);
	SHOW(i[IP_STAT_TOOLONG], 8, 0);
	SHOW(i[IP_STAT_BADOPTIONS], 9, 0);

	SHOW(i[IP_STAT_LOCALOUT], 1, 35);
	SHOW(i[IP_STAT_ODROPPED], 2, 35);
	SHOW(i[IP_STAT_OFRAGMENTS], 3, 35);
	SHOW(i[IP_STAT_CANTFRAG], 4, 35);
	SHOW(i[IP_STAT_NOROUTE], 5, 35);
	SHOW(i[IP_STAT_RAWOUT], 6, 35);
	SHOW(u[UDP_STAT_OPACKETS], 7, 35);
	
	SHOW(i[IP_STAT_FRAGMENTS], 10, 0);
	SHOW(i[IP_STAT_FRAGDROPPED], 11, 0);
	SHOW(i[IP_STAT_FRAGTIMEOUT], 12, 0);
	SHOW(i[IP_STAT_REASSEMBLED], 13, 0);

	SHOW(i[IP_STAT_FORWARD], 15, 0);
	SHOW(i[IP_STAT_FASTFORWARD], 16, 0);
	SHOW(i[IP_STAT_CANTFORWARD], 17, 0);
	SHOW(i[IP_STAT_REDIRECTSENT], 18, 0);

	SHOW(u[UDP_STAT_IPACKETS], 9, 35);
	SHOW(u[UDP_STAT_HDROPS], 10, 35);
	SHOW(u[UDP_STAT_BADSUM], 11, 35);
	SHOW(u[UDP_STAT_BADLEN], 12, 35);
	SHOW(u[UDP_STAT_NOPORT], 13, 35);
	SHOW(u[UDP_STAT_NOPORTBCAST], 14, 35);
	SHOW(u[UDP_STAT_FULLSOCK], 15, 35);
}

int
initip(void)
{

	return 1;
}

void
fetchip(void)
{
	size_t size;

	size = sizeof(newstat.i);
	if (sysctlbyname("net.inet.ip.stats", newstat.i, &size, NULL, 0) == -1)
		return;
	size = sizeof(newstat.u);
	if (sysctlbyname("net.inet.udp.stats", newstat.u, &size, NULL, 0) == -1)
		return;

	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_TOTAL]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_DELIVERED]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_BADSUM]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_TOOSHORT]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_TOOSMALL]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_BADHLEN]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_BADLEN]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_BADVERS]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_TOOLONG]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_BADOPTIONS]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_LOCALOUT]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_ODROPPED]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_OFRAGMENTS]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_CANTFRAG]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_NOROUTE]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_RAWOUT]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_FRAGMENTS]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_FRAGDROPPED]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_FRAGTIMEOUT]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_REASSEMBLED]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_FORWARD]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_FASTFORWARD]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_CANTFORWARD]);
	ADJINETCTR(curstat, oldstat, newstat, i[IP_STAT_REDIRECTSENT]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_OPACKETS]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_IPACKETS]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_HDROPS]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_BADSUM]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_BADLEN]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_NOPORT]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_NOPORTBCAST]);
	ADJINETCTR(curstat, oldstat, newstat, u[UDP_STAT_FULLSOCK]);

	if (update == UPDATE_TIME)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}

void
ip_boot(char *args)
{

	memset(&oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
ip_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
ip_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
ip_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}
