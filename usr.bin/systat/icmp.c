/*	$NetBSD: icmp.c,v 1.12.28.1 2014/08/20 00:05:04 tls Exp $	*/

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
__RCSID("$NetBSD: icmp.c,v 1.12.28.1 2014/08/20 00:05:04 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

#define LHD(row, str) mvwprintw(wnd, row, 10, str)
#define RHD(row, str) mvwprintw(wnd, row, 45, str);
#define BD(row, str) LHD(row, str); RHD(row, str)
#define SHOW(stat, row, col) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat[stat])
#define SHOW2(type, row) SHOW(ICMP_STAT_INHIST + type, row, 0); \
    SHOW(ICMP_STAT_OUTHIST + type, row, 35)

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static uint64_t curstat[ICMP_NSTATS];
static uint64_t newstat[ICMP_NSTATS];
static uint64_t oldstat[ICMP_NSTATS];

WINDOW *
openicmp(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closeicmp(WINDOW *w)
{

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

void
labelicmp(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);

	mvwprintw(wnd, 1, 0,  "------------ ICMP input -----------");
	mvwprintw(wnd, 1, 36, "------------- ICMP output ---------------");

	mvwprintw(wnd, 8, 0,  "---------- Input histogram --------");
	mvwprintw(wnd, 8, 36, "---------- Output histogram -------------");
	
	LHD(3, "with bad code");
	LHD(4, "with bad length");
	LHD(5, "with bad checksum");
	LHD(6, "with insufficient data");
	
	RHD(3, "errors generated");
	RHD(4, "suppressed - original too short");
	RHD(5, "suppressed - original was ICMP");
	RHD(6, "responses sent");

	BD(2, "total messages");
	BD(9, "echo response");
	BD(10, "echo request");
	BD(11, "destination unreachable");
	BD(12, "redirect");
	BD(13, "time-to-live exceeded");
	BD(14, "parameter problem");
	LHD(15, "router advertisement");	
	RHD(15, "router solicitation");
}

void
showicmp(void)
{
	u_long tin, tout;
	int i;

	for (i = tin = tout = 0; i <= ICMP_MAXTYPE; i++) {
		tin += curstat[ICMP_STAT_INHIST + i];
		tout += curstat[ICMP_STAT_OUTHIST + i];
	}

	tin += curstat[ICMP_STAT_BADCODE] + curstat[ICMP_STAT_BADLEN] + 
	    curstat[ICMP_STAT_CHECKSUM] + curstat[ICMP_STAT_TOOSHORT];
	mvwprintw(wnd, 2, 0, "%9lu", tin);
	mvwprintw(wnd, 2, 35, "%9lu", tout);

	SHOW(ICMP_STAT_BADCODE, 3, 0);
	SHOW(ICMP_STAT_BADLEN, 4, 0);
	SHOW(ICMP_STAT_CHECKSUM, 5, 0);
	SHOW(ICMP_STAT_TOOSHORT, 6, 0);
	SHOW(ICMP_STAT_ERROR, 3, 35);
	SHOW(ICMP_STAT_OLDSHORT, 4, 35);
	SHOW(ICMP_STAT_OLDICMP, 5, 35);
	SHOW(ICMP_STAT_REFLECT, 6, 35);

	SHOW2(ICMP_ECHOREPLY, 9);
	SHOW2(ICMP_ECHO, 10);
	SHOW2(ICMP_UNREACH, 11);
	SHOW2(ICMP_REDIRECT, 12);
	SHOW2(ICMP_TIMXCEED, 13);
	SHOW2(ICMP_PARAMPROB, 14);
	SHOW(ICMP_STAT_INHIST + ICMP_ROUTERADVERT, 15, 0);
	SHOW(ICMP_STAT_OUTHIST + ICMP_ROUTERSOLICIT, 15, 35);
}

int
initicmp(void)
{

	return (1);
}

void
fetchicmp(void)
{
	size_t i, size = sizeof(newstat);

	if (sysctlbyname("net.inet.icmp.stats", newstat, &size, NULL, 0) == -1)
		return;

	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_BADCODE);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_BADLEN);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_CHECKSUM);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_TOOSHORT);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_ERROR);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_OLDSHORT);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_OLDICMP);
	xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_REFLECT);

	for (i = 0; i <= ICMP_MAXTYPE; i++) {
		xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_INHIST + i);
		xADJINETCTR(curstat, oldstat, newstat, ICMP_STAT_OUTHIST + i);
	}

	if (update == UPDATE_TIME)
		memcpy(oldstat, newstat, sizeof(oldstat));
}

void
icmp_boot(char *args)
{

	memset(oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
icmp_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
icmp_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
icmp_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(oldstat, newstat, sizeof(oldstat));
}
