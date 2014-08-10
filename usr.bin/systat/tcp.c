/*	$NetBSD: tcp.c,v 1.15.40.1 2014/08/10 06:58:59 tls Exp $	*/

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
__RCSID("$NetBSD: tcp.c,v 1.15.40.1 2014/08/10 06:58:59 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

#define LHD(row, str)		mvwprintw(wnd, row, 10, str)
#define RHD(row, str)		mvwprintw(wnd, row, 45, str)
#define SHOW(row, col, stat) \
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat[stat])

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static uint64_t curstat[TCP_NSTATS];
static uint64_t newstat[TCP_NSTATS];
static uint64_t oldstat[TCP_NSTATS];

WINDOW *
opentcp(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closetcp(WINDOW *w)
{

	if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

void
labeltcp(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);

	LHD(0,  "connections initiated");		
	LHD(1,  "connections accepted");		
	LHD(2,  "connections established");
		
	LHD(4,  "connections dropped");		
	LHD(5,  "  in embryonic state");		
	LHD(6,  "  on retransmit timeout");	
	LHD(7,  "  by keepalive");			
	LHD(8,  "  by persist");		

	LHD(10, "potential rtt updates");
	LHD(11, "successful rtt updates");		
	LHD(12, "delayed acks sent");	
	LHD(13, "retransmit timeouts");	
	LHD(14, "persist timeouts");	
	LHD(15, "keepalive probes");	
	LHD(16, "keepalive timeouts");		
	
	RHD(9,  "total TCP packets received");
	RHD(10, "  in sequence");
	RHD(11, "  completely duplicate");
	RHD(12, "  with some duplicate data");
	RHD(13, "  out of order");
	RHD(14, "  duplicate acks");
	RHD(15, "  acks");	
	RHD(16, "  window probes");
	RHD(17, "  window updates");
	
	RHD(0, "total TCP packets sent");
	RHD(1, "  data");	
	RHD(2, "  data (retransmit)");	
	RHD(3, "  ack-only");
	RHD(4, "  window probes");
	RHD(5, "  window updates");
	RHD(6, "  urgent data only");
	RHD(7, "  control");
}

void
showtcpsyn(void)
{
	
	SHOW(0, 0, TCP_STAT_SC_ADDED);
	SHOW(1, 0, TCP_STAT_SC_COMPLETED);
	SHOW(2, 0, TCP_STAT_SC_TIMED_OUT);
	SHOW(3, 0, TCP_STAT_SC_DUPESYN);
	SHOW(4, 0, TCP_STAT_SC_COLLISIONS);
	SHOW(5, 0, TCP_STAT_SC_RETRANSMITTED);
	SHOW(6, 0, TCP_STAT_SC_ABORTED);
	SHOW(7, 0, TCP_STAT_SC_OVERFLOWED);
	SHOW(8, 0, TCP_STAT_SC_RESET);
	SHOW(9, 0, TCP_STAT_SC_UNREACH);
	SHOW(10, 0, TCP_STAT_SC_BUCKETOVERFLOW);
	SHOW(11, 0, TCP_STAT_SC_DROPPED);
}

void
labeltcpsyn(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);
	LHD(0,  "entries added");		
	LHD(1,  "connections completed");
	LHD(2,  "entries timed out");
	LHD(3,  "duplicate SYNs received");
	LHD(4,  "hash collisions");
	LHD(5,  "retransmissions");
	LHD(6,  "entries aborted (no memory)");
	LHD(7,  "dropped (overflow)");
	LHD(8,  "dropped (RST)");
	LHD(9,  "dropped (ICMP UNRCH)");
	LHD(10,  "dropped (bucket overflow)");
	LHD(11,  "dropped (unreachable/no memory)");
}

void
showtcp(void)
{
	
	SHOW(0, 0, TCP_STAT_CONNATTEMPT);		
	SHOW(1, 0, TCP_STAT_ACCEPTS);		
	SHOW(2, 0, TCP_STAT_CONNECTS);		

	SHOW(4, 0, TCP_STAT_DROPS);	
	SHOW(5, 0, TCP_STAT_CONNDROPS);
	SHOW(6, 0, TCP_STAT_TIMEOUTDROP);
	SHOW(7, 0, TCP_STAT_KEEPDROPS);
	SHOW(8, 0, TCP_STAT_PERSISTDROPS);

	SHOW(10, 0, TCP_STAT_SEGSTIMED);
	SHOW(11, 0, TCP_STAT_RTTUPDATED);
	SHOW(12, 0, TCP_STAT_DELACK);
	SHOW(13, 0, TCP_STAT_REXMTTIMEO);
	SHOW(14, 0, TCP_STAT_PERSISTTIMEO);
	SHOW(15, 0, TCP_STAT_KEEPPROBE);
	SHOW(16, 0, TCP_STAT_KEEPTIMEO);

	SHOW(0, 35, TCP_STAT_SNDTOTAL);
	SHOW(1, 35, TCP_STAT_SNDPACK);
	SHOW(2, 35, TCP_STAT_SNDREXMITPACK);

	SHOW(3, 35, TCP_STAT_SNDACKS);
	SHOW(4, 35, TCP_STAT_SNDPROBE);
	SHOW(5, 35, TCP_STAT_SNDWINUP);
	SHOW(6, 35, TCP_STAT_SNDURG);
	SHOW(7, 35, TCP_STAT_SNDCTRL);

	SHOW(9, 35, TCP_STAT_RCVTOTAL);
	SHOW(10, 35, TCP_STAT_RCVPACK);
	SHOW(11, 35, TCP_STAT_RCVDUPPACK);
	SHOW(12, 35, TCP_STAT_RCVPARTDUPPACK);
	SHOW(13, 35, TCP_STAT_RCVOOPACK);
	SHOW(14, 35, TCP_STAT_RCVDUPACK);
	SHOW(15, 35, TCP_STAT_RCVACKPACK);
	SHOW(16, 35, TCP_STAT_RCVWINPROBE);
	SHOW(17, 35, TCP_STAT_RCVWINUPD);
}

int
inittcp(void)
{
	return 1;
}

void
fetchtcp(void)
{
	size_t i, size = sizeof(newstat);

	if (sysctlbyname("net.inet.tcp.stats", newstat, &size, NULL, 0) == -1)
		return;

	for (i = 0; i < TCP_NSTATS; i++)
		xADJINETCTR(curstat, oldstat, newstat, i);

	if (update == UPDATE_TIME)
		memcpy(oldstat, newstat, sizeof(oldstat));
}

void
tcp_boot(char *args)
{

	memset(oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
tcp_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
tcp_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(oldstat, newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
tcp_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(oldstat, newstat, sizeof(oldstat));
}
