/*	$NetBSD: tcp.c,v 1.12 2006/10/22 16:43:24 christos Exp $	*/

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
__RCSID("$NetBSD: tcp.c,v 1.12 2006/10/22 16:43:24 christos Exp $");
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
    mvwprintw(wnd, row, col, "%9llu", (unsigned long long)curstat.stat)

enum update {
	UPDATE_TIME,
	UPDATE_BOOT,
	UPDATE_RUN,
};

static enum update update = UPDATE_TIME;
static struct tcpstat curstat;
static struct tcpstat newstat;
static struct tcpstat oldstat;

static struct nlist namelist[] = {
	{ .n_name = "_tcpstat" },
	{ .n_name = NULL }
};

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
	
	SHOW(0, 0, tcps_sc_added);
	SHOW(1, 0, tcps_sc_completed);
	SHOW(2, 0, tcps_sc_timed_out);
	SHOW(3, 0, tcps_sc_dupesyn);
	SHOW(4, 0, tcps_sc_collisions);
	SHOW(5, 0, tcps_sc_retransmitted);
	SHOW(6, 0, tcps_sc_aborted);
	SHOW(7, 0, tcps_sc_overflowed);
	SHOW(8, 0, tcps_sc_reset);
	SHOW(9, 0, tcps_sc_unreach);
	SHOW(10, 0, tcps_sc_bucketoverflow);
	SHOW(11, 0, tcps_sc_dropped);
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
	
	SHOW(0, 0, tcps_connattempt);		
	SHOW(1, 0, tcps_accepts);		
	SHOW(2, 0, tcps_connects);		

	SHOW(4, 0, tcps_drops);	
	SHOW(5, 0, tcps_conndrops);
	SHOW(6, 0, tcps_timeoutdrop);
	SHOW(7, 0, tcps_keepdrops);
	SHOW(8, 0, tcps_persistdrops);

	SHOW(10, 0, tcps_segstimed);
	SHOW(11, 0, tcps_rttupdated);
	SHOW(12, 0, tcps_delack);
	SHOW(13, 0, tcps_rexmttimeo);
	SHOW(14, 0, tcps_persisttimeo);
	SHOW(15, 0, tcps_keepprobe);
	SHOW(16, 0, tcps_keeptimeo);

	SHOW(0, 35, tcps_sndtotal);
	SHOW(1, 35, tcps_sndpack);
	SHOW(2, 35, tcps_sndrexmitpack);

	SHOW(3, 35, tcps_sndacks);
	SHOW(4, 35, tcps_sndprobe);
	SHOW(5, 35, tcps_sndwinup);
	SHOW(6, 35, tcps_sndurg);
	SHOW(7, 35, tcps_sndctrl);

	SHOW(9, 35, tcps_rcvtotal);
	SHOW(10, 35, tcps_rcvpack);
	SHOW(11, 35, tcps_rcvduppack);
	SHOW(12, 35, tcps_rcvpartduppack);
	SHOW(13, 35, tcps_rcvoopack);
	SHOW(14, 35, tcps_rcvdupack);
	SHOW(15, 35, tcps_rcvackpack);
	SHOW(16, 35, tcps_rcvwinprobe);
	SHOW(17, 35, tcps_rcvwinupd);
}

int
inittcp(void)
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
fetchtcp(void)
{

	KREAD((void *)namelist[0].n_value, &newstat, sizeof(newstat));

	ADJINETCTR(curstat, oldstat, newstat, tcps_connattempt);
	ADJINETCTR(curstat, oldstat, newstat, tcps_accepts);
	ADJINETCTR(curstat, oldstat, newstat, tcps_connects);
	ADJINETCTR(curstat, oldstat, newstat, tcps_drops);
	ADJINETCTR(curstat, oldstat, newstat, tcps_conndrops);
	ADJINETCTR(curstat, oldstat, newstat, tcps_timeoutdrop);
	ADJINETCTR(curstat, oldstat, newstat, tcps_keepdrops);
	ADJINETCTR(curstat, oldstat, newstat, tcps_persistdrops);
	ADJINETCTR(curstat, oldstat, newstat, tcps_segstimed);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rttupdated);
	ADJINETCTR(curstat, oldstat, newstat, tcps_delack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rexmttimeo);
	ADJINETCTR(curstat, oldstat, newstat, tcps_persisttimeo);
	ADJINETCTR(curstat, oldstat, newstat, tcps_keepprobe);
	ADJINETCTR(curstat, oldstat, newstat, tcps_keeptimeo);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndtotal);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndpack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndrexmitpack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndacks);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndprobe);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndwinup);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndurg);
	ADJINETCTR(curstat, oldstat, newstat, tcps_sndctrl);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvtotal);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvpack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvduppack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvpartduppack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvoopack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvdupack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvackpack);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvwinprobe);
	ADJINETCTR(curstat, oldstat, newstat, tcps_rcvwinupd);

	if (update == UPDATE_TIME)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}

void
tcp_boot(char *args)
{

	memset(&oldstat, 0, sizeof(oldstat));
	update = UPDATE_BOOT;
}

void
tcp_run(char *args)
{

	if (update != UPDATE_RUN) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_RUN;
	}
}

void
tcp_time(char *args)
{

	if (update != UPDATE_TIME) {
		memcpy(&oldstat, &newstat, sizeof(oldstat));
		update = UPDATE_TIME;
	}
}

void
tcp_zero(char *args)
{

	if (update == UPDATE_RUN)
		memcpy(&oldstat, &newstat, sizeof(oldstat));
}
