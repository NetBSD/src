/*	$NetBSD: ipsec.c,v 1.10 2008/04/23 06:09:04 thorpej Exp $	*/

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
__RCSID("$NetBSD: ipsec.c,v 1.10 2008/04/23 06:09:04 thorpej Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet6/ipsec.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str);
#define SHOW(stat, row, col) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat.stat)

struct mystat {
	uint64_t i4[IPSEC_NSTATS];
#ifdef INET6
	uint64_t i6[IPSEC_NSTATS];
#endif
};

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static struct mystat curstat;
static struct mystat newstat;
static struct mystat oldstat;

static struct nlist namelist[] = {
	{ .n_name = "_ipsecstat" },
#ifdef INET6
	{ .n_name = "_ipsec6stat" },
#endif
	{ .n_name = NULL }
};

WINDOW *
openipsec(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closeipsec(WINDOW *w)
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
		SHOW(i4[IPSEC_STAT_IN_SUCCESS],		1, 0);
		SHOW(i4[IPSEC_STAT_IN_POLVIO],		2, 0);
		SHOW(i4[IPSEC_STAT_IN_NOSA],		3, 0);
		SHOW(i4[IPSEC_STAT_IN_INVAL],		4, 0);
		SHOW(i4[IPSEC_STAT_IN_BADSPI],		5, 0);
		SHOW(i4[IPSEC_STAT_IN_AHREPLAY],	6, 0);
		SHOW(i4[IPSEC_STAT_IN_ESPREPLAY],	7, 0);
		SHOW(i4[IPSEC_STAT_IN_AHAUTHSUCC],	8, 0);
		SHOW(i4[IPSEC_STAT_IN_AHAUTHFAIL],	9, 0);

		SHOW(i4[IPSEC_STAT_OUT_SUCCESS],	12, 0);
		SHOW(i4[IPSEC_STAT_OUT_POLVIO],		13, 0);
		SHOW(i4[IPSEC_STAT_OUT_NOSA],		14, 0);
		SHOW(i4[IPSEC_STAT_OUT_INVAL],		15, 0);
		SHOW(i4[IPSEC_STAT_OUT_NOROUTE],	16, 0);
	}

#ifdef INET6
	if (namelist[1].n_type) {
		SHOW(i6[IPSEC_STAT_IN_SUCCESS],		1, 35);
		SHOW(i6[IPSEC_STAT_IN_POLVIO],		2, 35);
		SHOW(i6[IPSEC_STAT_IN_NOSA],		3, 35);
		SHOW(i6[IPSEC_STAT_IN_INVAL],		4, 35);
		SHOW(i6[IPSEC_STAT_IN_BADSPI],		5, 35);
		SHOW(i6[IPSEC_STAT_IN_AHREPLAY],	6, 35);
		SHOW(i6[IPSEC_STAT_IN_ESPREPLAY],	7, 35);
		SHOW(i6[IPSEC_STAT_IN_AHAUTHSUCC],	8, 35);
		SHOW(i6[IPSEC_STAT_IN_AHAUTHFAIL],	9, 35);

		SHOW(i6[IPSEC_STAT_OUT_SUCCESS],	12, 35);
		SHOW(i6[IPSEC_STAT_OUT_POLVIO],		13, 35);
		SHOW(i6[IPSEC_STAT_OUT_NOSA],		14, 35);
		SHOW(i6[IPSEC_STAT_OUT_INVAL],		15, 35);
		SHOW(i6[IPSEC_STAT_OUT_NOROUTE],	16, 35);
	}
#endif
}

int
initipsec(void)
{

	if (! use_sysctl) {
		int n;

		if (namelist[0].n_type == 0) {
			n = kvm_nlist(kd, namelist);
			if (n < 0) {
				nlisterr(namelist);
				return(0);
			} else if (n == sizeof(namelist) /
					sizeof(namelist[0]) - 1) {
				error("No namelist");
				return(0);
			}
		}
	}
	return 1;
}

void
fetchipsec(void)
{
	bool do_ipsec4 = false;
#ifdef INET6
	bool do_ipsec6 = false;
#endif
	u_int i;

	if (use_sysctl) {
		size_t size = sizeof(newstat.i4);
		
		if (sysctlbyname("net.inet.ipsec.stats", newstat.i4, &size,
				 NULL, 0) == 0) {
			do_ipsec4 = true;
		}
	} else {
		if (namelist[0].n_type) {
			KREAD((void *)namelist[0].n_value, &newstat.i4,
			    sizeof(newstat.i4));
			do_ipsec4 = true;
		}
	}

	if (do_ipsec4) {
		for (i = 0; i < IPSEC_NSTATS; i++) {
			ADJINETCTR(curstat, oldstat, newstat, i4[i]);
		}
	}

#ifdef INET6
	if (use_sysctl) {
		size_t size = sizeof(newstat.i6);
		
		if (sysctlbyname("net.inet6.ipsec6.stats", newstat.i6, &size,
				 NULL, 0) == 0) {
			do_ipsec6 = true;
		}
	} else {
		if (namelist[1].n_type) {
			KREAD((void *)namelist[1].n_value, &newstat.i6,
			    sizeof(newstat.i6));
			do_ipsec6 = true;
		}
	}

	if (do_ipsec6) {
		for (i = 0; i < IPSEC_NSTATS; i++) {
			ADJINETCTR(curstat, oldstat, newstat, i6[i]);
		}
	}
#endif

	if (update == UPDATE_TIME)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}

void
ipsec_boot(char *args)
{

	memset(&oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
ipsec_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
ipsec_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
ipsec_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}
