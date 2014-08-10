/*	$NetBSD: ip6.c,v 1.15.40.1 2014/08/10 06:58:59 tls Exp $	*/

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
__RCSID("$NetBSD: ip6.c,v 1.15.40.1 2014/08/10 06:58:59 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str);
#define SHOW(stat, row, col) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat[stat])

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static uint64_t curstat[IP6_NSTATS];
static uint64_t newstat[IP6_NSTATS];
static uint64_t oldstat[IP6_NSTATS];

WINDOW *
openip6(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closeip6(WINDOW *w)
{

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

void
labelip6(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);

	LHD(0,	"total packet received");
	LHD(1,	"  smaller than minimum");
	LHD(2,	"  data size < data length");
	LHD(3,	"  bad options");
	LHD(4,	"  incorrect version no");
	LHD(5,	"  headers not continuous");
	LHD(6,	"  packet for this host");
	LHD(7,	"  multicast we don't join");
	LHD(8,	"  too many headers");
	LHD(9,	"  tunneled packet w/o gif");

	LHD(11,	"  fragment received");
	LHD(12,	"  fragment dropped");
	LHD(13,	"  fragment timeout");
	LHD(14,	"  fragment exceeded limit");
	LHD(15,	"  packet reassembled ok");

#if 0
	LHD(17,	"one mbuf");
	LHD(18,	"one ext mbuf");
	LHD(19,	"two or more ext mbuf");
	LHD(20,	"two or more mbuf");
#endif

	RHD(0,	"packet forwarded");
	RHD(1,	"  packet not forwardable");
	RHD(2,	"  redirect sent");

	RHD(4,	"packet sent from this host");
	RHD(5,	"  fabricated ip header");
	RHD(6,	"  dropped output (no bufs)");
	RHD(7,	"  dropped output (no route)");
	RHD(8,	"  output datagram fragmented");
	RHD(9,	"  fragment created");
	RHD(10,	"  can't be fragmented");

	RHD(12,	"violated scope rules");
}
	
void
showip6(void)
{
#if 0
	u_quad_t m2m;
	int i;

	m2m = 0;
	for (i = 0; i < 32; i++) {
		m2m += curstat[IP6_STAT_M2M + i];
	}
#endif

	SHOW(IP6_STAT_TOTAL, 0, 0);
	SHOW(IP6_STAT_TOOSMALL, 1, 0);
	SHOW(IP6_STAT_TOOSHORT, 2, 0);
	SHOW(IP6_STAT_BADOPTIONS, 3, 0);
	SHOW(IP6_STAT_BADVERS, 4, 0);
	SHOW(IP6_STAT_EXTHDRTOOLONG, 5, 0);
	SHOW(IP6_STAT_DELIVERED, 6, 0);
	SHOW(IP6_STAT_NOTMEMBER, 7, 0);
	SHOW(IP6_STAT_TOOMANYHDR, 8, 0);
	SHOW(IP6_STAT_NOGIF, 9, 0);

	SHOW(IP6_STAT_FRAGMENTS, 11, 0);
	SHOW(IP6_STAT_FRAGDROPPED, 12, 0);
	SHOW(IP6_STAT_FRAGTIMEOUT, 13, 0);
	SHOW(IP6_STAT_FRAGOVERFLOW, 14, 0);
	SHOW(IP6_STAT_REASSEMBLED, 15, 0);

#if 0
	SHOW(IP6_STAT_M1, 17, 0);
	SHOW(IP6_STAT_MEXT1, 18, 0);
	SHOW(IP6_STAT_MEXT2M, 19, 0);
	mvwprintw(wnd, 20, 0, "%9llu", (unsigned long long)m2m);
#endif

	SHOW(IP6_STAT_FORWARD, 0, 35);
	SHOW(IP6_STAT_CANTFORWARD, 1, 35);
	SHOW(IP6_STAT_REDIRECTSENT, 2, 35);

	SHOW(IP6_STAT_LOCALOUT, 4, 35);
	SHOW(IP6_STAT_RAWOUT, 5, 35);
	SHOW(IP6_STAT_ODROPPED, 6, 35);
	SHOW(IP6_STAT_NOROUTE, 7, 35);
	SHOW(IP6_STAT_FRAGMENTED, 8, 35);
	SHOW(IP6_STAT_OFRAGMENTS, 9, 35);
	SHOW(IP6_STAT_CANTFRAG, 10, 35);

	SHOW(IP6_STAT_BADSCOPE, 12, 35);
}

int
initip6(void)
{

	return 1;
}

void
fetchip6(void)
{
	size_t i, size = sizeof(newstat);

	if (sysctlbyname("net.inet6.ip6.stats", newstat, &size, NULL, 0) == -1)
		return;

	for (i = 0; i < IP6_NSTATS; i++)
		xADJINETCTR(curstat, oldstat, newstat, i);

	if (update == UPDATE_TIME)
		memcpy(oldstat, newstat, sizeof(oldstat));
}

void
ip6_boot(char *args)
{

	memset(oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
ip6_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
ip6_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
ip6_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(oldstat, newstat, sizeof(oldstat));
}
