/*	$NetBSD: ip6.c,v 1.1 2000/01/08 23:12:37 itojun Exp $ */

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
__RCSID("$NetBSD: ip6.c,v 1.1 2000/01/08 23:12:37 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <nlist.h>
#include <kvm.h>
#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str);
#define SHOW(stat, row, col) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat.stat)

struct mystat {
	struct ip6stat i;
};

static struct mystat curstat;

static struct nlist namelist[] = {
	{ "_ip6stat" },
	{ "" }
};

WINDOW *
openip6(void)
{

	return (subwin(stdscr, LINES-5-1, 0, 5, 0));
}

void
closeip6(w)
	WINDOW *w;
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
	RHD(13,	"call to m_pulldown");
	RHD(14,	"mbuf allocation in m_pulldown");
	RHD(15,	"mbuf copy in m_pulldown");
}
	
void
showip6(void)
{
#if 0
	u_quad_t m2m;
	int i;

	m2m = 0;
	for (i = 0;
	     i < sizeof(curstat.i.ip6s_m2m)/sizeof(curstat.i.ip6s_m2m[0]);
	     i++) {
		m2m += curstat.i.ip6s_m2m[i];
	}
#endif

	SHOW(i.ip6s_total, 0, 0);
	SHOW(i.ip6s_toosmall, 1, 0);
	SHOW(i.ip6s_tooshort, 2, 0);
	SHOW(i.ip6s_badoptions, 3, 0);
	SHOW(i.ip6s_badvers, 4, 0);
	SHOW(i.ip6s_exthdrtoolong, 5, 0);
	SHOW(i.ip6s_delivered, 6, 0);
	SHOW(i.ip6s_notmember, 7, 0);
	SHOW(i.ip6s_toomanyhdr, 8, 0);
	SHOW(i.ip6s_nogif, 9, 0);

	SHOW(i.ip6s_fragments, 11, 0);
	SHOW(i.ip6s_fragdropped, 12, 0);
	SHOW(i.ip6s_fragtimeout, 13, 0);
	SHOW(i.ip6s_fragoverflow, 14, 0);
	SHOW(i.ip6s_reassembled, 15, 0);

#if 0
	SHOW(i.ip6s_m1, 17, 0);
	SHOW(i.ip6s_mext1, 18, 0);
	SHOW(i.ip6s_mext2m, 19, 0);
	mvwprintw(wnd, 20, 0, "%9llu", (unsigned long long)m2m);
#endif

	SHOW(i.ip6s_forward, 0, 35);
	SHOW(i.ip6s_cantforward, 1, 35);
	SHOW(i.ip6s_redirectsent, 2, 35);

	SHOW(i.ip6s_localout, 4, 35);
	SHOW(i.ip6s_rawout, 5, 35);
	SHOW(i.ip6s_odropped, 6, 35);
	SHOW(i.ip6s_noroute, 7, 35);
	SHOW(i.ip6s_fragmented, 8, 35);
	SHOW(i.ip6s_ofragments, 9, 35);
	SHOW(i.ip6s_cantfrag, 10, 35);

	SHOW(i.ip6s_badscope, 12, 35);
	SHOW(i.ip6s_pulldown, 13, 35);
	SHOW(i.ip6s_pulldown_alloc, 14, 35);
	SHOW(i.ip6s_pulldown_copy, 15, 35);
}

int
initip6(void)
{

	if (namelist[0].n_type == 0) {
		if (kvm_nlist(kd, namelist)) {
			nlisterr(namelist);
			return(0);
		}
		if (namelist[0].n_type == 0) {
			error("No namelist");
			return(0);
		}
	}
	return 1;
}

void
fetchip6(void)
{

	KREAD((void *)namelist[0].n_value, &curstat.i, sizeof(curstat.i));
}
