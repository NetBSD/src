/*	$NetBSD: ipsec.c,v 1.3 2000/01/13 12:39:05 ad Exp $	*/

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
__RCSID("$NetBSD: ipsec.c,v 1.3 2000/01/13 12:39:05 ad Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet6/ipsec.h>

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
	struct ipsecstat i4;
#ifdef INET6
	struct ipsecstat i6;
#endif
};

static struct mystat curstat;

static struct nlist namelist[] = {
	{ "_ipsecstat" },
#ifdef INET6
	{ "_ipsec6stat" },
#endif
	{ "" }
};

WINDOW *
openipsec(void)
{

	return (subwin(stdscr, LINES-5-1, 0, 5, 0));
}

void
closeipsec(w)
	WINDOW *w;
{

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

void
labelipsec(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);

	if (namelist[0].n_type) {
		mvwprintw(wnd, 0, 0,  "------ IPv4 IPsec input ------");
		LHD(1,	"processed successfully");
		LHD(2,	"violated process policy");
		LHD(3,	"with no SA available");
		LHD(4,	"failed due to EINVAL");
		LHD(5,	"failed getting SPI");
		LHD(6,	"failed on AH replay check");
		LHD(7,	"failed on ESP replay check");
		LHD(8,	"considered authentic");
		LHD(9,	"failed on authentication");

		mvwprintw(wnd, 11, 0,  "------ IPv4 IPsec output ------");
		LHD(12,	"processed successfully");
		LHD(13,	"violated process policy");
		LHD(14,	"with no SA available");
		LHD(15,	"failed processing due to EINVAL");
		LHD(16,	"with no route");
	}

#ifdef INET6
	if (namelist[1].n_type) {
		mvwprintw(wnd, 0, 35,  "------ IPv6 IPsec input ------");
		RHD(1,	"processed successfully");
		RHD(2,	"violated process policy");
		RHD(3,	"with no SA available");
		RHD(4,	"failed due to EINVAL");
		RHD(5,	"failed getting SPI");
		RHD(6,	"failed on AH replay check");
		RHD(7,	"failed on ESP replay check");
		RHD(8,	"considered authentic");
		RHD(9,	"failed on authentication");

		mvwprintw(wnd, 11, 35,  "------ IPv6 IPsec output ------");
		RHD(12,	"processed successfully");
		RHD(13,	"violated process policy");
		RHD(14,	"with no SA available");
		RHD(15,	"failed due to EINVAL");
		RHD(16,	"with no route");
	}
#endif
}
	
void
showipsec(void)
{

	if (namelist[0].n_type) {
		SHOW(i4.in_success,	1, 0);
		SHOW(i4.in_polvio,	2, 0);
		SHOW(i4.in_nosa,	3, 0);
		SHOW(i4.in_inval,	4, 0);
		SHOW(i4.in_badspi,	5, 0);
		SHOW(i4.in_ahreplay,	6, 0);
		SHOW(i4.in_espreplay,	7, 0);
		SHOW(i4.in_ahauthsucc,	8, 0);
		SHOW(i4.in_ahauthfail,	9, 0);

		SHOW(i4.out_success,	12, 0);
		SHOW(i4.out_polvio,	13, 0);
		SHOW(i4.out_nosa,	14, 0);
		SHOW(i4.out_inval,	15, 0);
		SHOW(i4.out_noroute,	16, 0);
	}

#ifdef INET6
	if (namelist[1].n_type) {
		SHOW(i6.in_success,	1, 35);
		SHOW(i6.in_polvio,	2, 35);
		SHOW(i6.in_nosa,	3, 35);
		SHOW(i6.in_inval,	4, 35);
		SHOW(i6.in_badspi,	5, 35);
		SHOW(i6.in_ahreplay,	6, 35);
		SHOW(i6.in_espreplay,	7, 35);
		SHOW(i6.in_ahauthsucc,	8, 35);
		SHOW(i6.in_ahauthfail,	9, 35);

		SHOW(i6.out_success,	12, 35);
		SHOW(i6.out_polvio,	13, 35);
		SHOW(i6.out_nosa,	14, 35);
		SHOW(i6.out_inval,	15, 35);
		SHOW(i6.out_noroute,	16, 35);
	}
#endif
}

int
initipsec(void)
{
	int n;

	if (namelist[0].n_type == 0) {
		n = kvm_nlist(kd, namelist);
		if (n < 0) {
			nlisterr(namelist);
			return(0);
		} else if (n == sizeof(namelist) / sizeof(namelist[0]) - 1) {
			error("No namelist");
			return(0);
		}
	}
	return 1;
}

void
fetchipsec(void)
{

	if (namelist[0].n_type) {
		KREAD((void *)namelist[0].n_value, &curstat.i4,
		    sizeof(curstat.i4));
	}
#ifdef INET6
	if (namelist[1].n_type) {
		KREAD((void *)namelist[1].n_value, &curstat.i6,
		    sizeof(curstat.i6));
	}
#endif
}
