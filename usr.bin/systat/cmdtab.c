/*	$NetBSD: cmdtab.c,v 1.12 1999/12/22 14:46:14 kleink Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)cmdtab.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: cmdtab.c,v 1.12 1999/12/22 14:46:14 kleink Exp $");
#endif /* not lint */

#include "systat.h"
#include "extern.h"

struct	command global_commands[] = {
	{ "help",	global_help,		"show help"},
	{ "interval",	global_interval,	"set update interval"},
	{ "load",	global_load,		"show system load averages"},
	{ "quit",	global_quit,		"exit systat"},
	/* until prefix matching works, handle the same special case */
	{ "q",		global_quit,		"exit systat"},
	{ "start",	global_interval,	"restart updating display"},
	{ "stop",	global_stop,		"stop updating display"},
	{ 0 }
};

struct command	iostat_commands[] = {
	{ "bars",	iostat_bars,	"show io stats as a bar graph"},
	{ "numbers",	iostat_numbers,	"show io stats numerically"},
	{ "secs",	iostat_secs,	"include time statistics"},
	/* from disks.c */
	{ "add",	disks_add,	"add a disk to displayed disks"},
	{ "show",	disks_add,	"add a disk to displayed disks"},
	{ "delete",	disks_delete,	"remove a disk from displayed disks"},
	{ "ignore",	disks_delete,	"remove a disk from displayed disks"},
	{ "drives",	disks_drives,	"list all disks"},
	{ 0 }
};

struct command netstat_commands[] = {
	{ "all",	netstat_all,	 "include server sockets"},
	{ "display",	netstat_display, "show specified hosts or ports"},
	{ "ignore",	netstat_ignore,	 "hide specified hosts or ports"},
	{ "names",	netstat_names,	 "show names instead of addresses"},
	{ "numbers",	netstat_numbers, "show addresses instead of names"},
	{ "reset",	netstat_reset,	 "return to default display"},
	{ "show",	netstat_show,	"show current display/ignore settings"},
	{ "tcp",	netstat_tcp,	 "show only tcp connections"},
	{ "udp",	netstat_udp,	 "show only udp connections"},
	{ 0 }
};

struct command ps_commands[] = {
	{ "user",	ps_user,	"limit displayed processes to a user"},
	{ 0 }
};

struct command	vmstat_commands[] = {
	{ "boot",	vmstat_boot,	"show total vm stats since boot"},
	{ "run",	vmstat_run,	"show running total vm stats"},
	{ "time",	vmstat_time,	"show vm stats for each sample time"},
	{ "zero",	vmstat_zero,	"re-zero running totals"},
	/* from disks.c */
	{ "add",	disks_add,	"add a disk to displayed disks"},
	{ "show",	disks_add,	"add a disk to displayed disks"},
	{ "delete",	disks_delete,	"remove a disk from displayed disks"},
	{ "ignore",	disks_delete,	"remove a disk from displayed disks"},
	{ "drives",	disks_drives,	"list all disks"},
	{ 0 }
};

struct mode modes[] = {
	/* "pigs" is the default, it must be first. */
	{ "pigs",	showpigs,	fetchpigs,	labelpigs,
	  initpigs,	openpigs,	closepigs,	0,
	  CF_LOADAV },
	{ "bufcache",	showbufcache,	fetchbufcache,	labelbufcache,
	  initbufcache,	openbufcache,	closebufcache,	0,
	  CF_LOADAV },
	{ "inet.icmp",	showicmp,	fetchicmp,	labelicmp,
	  initicmp,	openicmp,	closeicmp,	0,
	  CF_LOADAV },
	{ "inet.ip",	showip,		fetchip,	labelip,
	  initip,	openip,		closeip,	0,
	  CF_LOADAV },
	{ "inet.tcp",	showtcp,	fetchtcp,	labeltcp,
	  inittcp,	opentcp,	closetcp,	0,
	  CF_LOADAV },
	{ "inet.tcpsyn",showtcpsyn,	fetchtcp,	labeltcpsyn,
	  inittcp,	opentcp,	closetcp,	0,
	  CF_LOADAV },
	{ "iostat",	showiostat,	fetchiostat,	labeliostat,
	  initiostat,	openiostat,	closeiostat,	iostat_commands,
	  CF_LOADAV },
	{ "mbufs",	showmbufs,	fetchmbufs,	labelmbufs,
	  initmbufs,	openmbufs,	closembufs,	0,
	  CF_LOADAV },
	{ "netstat",	shownetstat,	fetchnetstat,	labelnetstat,
	  initnetstat,	opennetstat,	closenetstat,	netstat_commands,
	  CF_LOADAV },
	{ "ps",		showps,		fetchpigs,	labelps,
	  initpigs,	openpigs,	closepigs,	ps_commands,
	  CF_LOADAV },
	{ "swap",	showswap,	fetchswap,	labelswap,
	  initswap,	openswap,	closeswap,	0,
	  CF_LOADAV },
	{ "vmstat",	showkre,	fetchkre,	labelkre,
	  initkre,	openkre,	closekre,	vmstat_commands,
	  0 },
	{ 0 }
};
struct  mode *curmode = &modes[0];
