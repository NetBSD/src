/*	$NetBSD: syscall.c,v 1.2 2006/05/20 20:07:35 dsl Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: syscall.c,v 1.2 2006/05/20 20:07:35 dsl Exp $");

/* System call stats */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "systat.h"
#include "extern.h"
#include "drvstats.h"
#include "utmpentry.h"
#include "vmstat.h"

#include <sys/syscall.h>
#include <../../sys/kern/syscalls.c>

#define nelem(x) (sizeof (x) / sizeof *(x))

static struct Info {
	struct	 uvmexp_sysctl uvmexp;
	struct	 vmtotal Total;
	uint64_t counts[SYS_NSYSENT];
	uint64_t times[SYS_NSYSENT];
} s, s1, s2, irf;

int syscall_sort[SYS_NSYSENT];

static	enum sort_order { UNSORTED, NAMES, COUNTS } sort_order = NAMES;

#define	SHOW_COUNTS	1
#define	SHOW_TIMES	2
static int show = SHOW_COUNTS;

static void getinfo(struct Info *, int);

static	char buf[26];
static	float hertz;

static size_t counts_mib_len, times_mib_len;
static int counts_mib[4], times_mib[4];

WINDOW *
opensyscall(void)
{
	return (stdscr);
}

void
closesyscall(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
}


/*
 * These constants define where the major pieces are laid out
 */
#define SYSCALLROW	9	/* Below the vmstat header */

int
initsyscall(void)
{
	static char name[] = "name";

	hertz = stathz ? stathz : hz;

	syscall_order(name);

	/* drvinit gets number of cpus! */
	drvinit(1);

	counts_mib_len = nelem(counts_mib);
	if (sysctlnametomib("kern.syscalls.counts", counts_mib, &counts_mib_len))
		counts_mib_len = 0;

	times_mib_len = nelem(times_mib);
	if (sysctlnametomib("kern.syscalls.times", times_mib, &times_mib_len))
		times_mib_len = 0;

	getinfo(&s2, SHOW_COUNTS | SHOW_TIMES);
	s1 = s2;

	return(1);
}

void
fetchsyscall(void)
{
	time_t now;

	time(&now);
	strlcpy(buf, ctime(&now), sizeof(buf));
	buf[19] = '\0';
	getinfo(&s, show);
}

void
labelsyscall(void)
{
	labelvmstat_top();
}

#define MAXFAIL 5

static int
compare_counts(const void *a, const void *b)
{
	int ia = *(const int *)a, ib = *(const int *)b;
	int64_t delta;

	if (display_mode == TIME)
		delta = irf.counts[ib] - irf.counts[ia];
	else
		delta = s.counts[ib] - s1.counts[ib]
		    - (s.counts[ia] - s1.counts[ia]);
	return delta ? delta < 0 ? -1 : 1 : 0;
}

static int
compare_times(const void *a, const void *b)
{
	int ia = *(const int *)a, ib = *(const int *)b;
	int64_t delta;

	if (display_mode == TIME)
		delta = irf.times[ib] - irf.times[ia];
	else
		delta = s.times[ib] - s1.times[ib] - s.times[ia] + s1.times[ia];
	return delta ? delta < 0 ? -1 : 1 : 0;
}

void
showsyscall(void)
{
	int i, ii, l, c;
	uint64_t val;
	static int failcnt = 0;
	static int relabel = 0;
	static char pigs[] = "pigs";

	if (relabel) {
		labelsyscall();
		relabel = 0;
	}

	cpuswap();
	if (display_mode == TIME) {
		etime = cur.cp_etime;
		/* < 5 ticks - ignore this trash */
		if ((etime * hertz) < 1.0) {
			if (failcnt++ > MAXFAIL)
				return;
			failcnt = 0;
			clear();
			mvprintw(2, 10, "The alternate system clock has died!");
			mvprintw(3, 10, "Reverting to ``pigs'' display.");
			move(CMDLINE, 0);
			refresh();
			failcnt = 0;
			sleep(5);
			command(pigs);
			return;
		}
	} else
		etime = 1.0;

	failcnt = 0;

	show_vmstat_top(&s.Total, &s.uvmexp, &s1.uvmexp);

	if (sort_order == COUNTS) {
		/*
		 * We use an 'infinite response filter' in a vague
		 * attempt to stop the data leaping around too much.
		 * I suspect there are other/better methods in use.
		 */
		if (show & SHOW_COUNTS) {
			for (i = 0; i < nelem(s.counts); i++) {
				val = s.counts[i] - s1.counts[i];
				if (irf.counts[i] > 0xfffffff)
				    irf.counts[i] = irf.counts[i] / 8 * 7 + val;
				else
				    irf.counts[i] = irf.counts[i] * 7 / 8 + val;
			}
		}
		if (show & SHOW_TIMES) {
			for (i = 0; i < nelem(s.times); i++) {
				val = s.times[i] - s1.times[i];
				if (irf.times[i] > 0xfffffff)
				    irf.times[i] = irf.times[i] / 8 * 7 + val;
				else
				    irf.times[i] = irf.times[i] * 7 / 8 + val;
			}
		}

		/* mergesort() doesn't swap equal values about... */
		mergesort(syscall_sort, nelem(syscall_sort),
			sizeof syscall_sort[0],
			show & SHOW_COUNTS ? compare_counts : compare_times);
	}

	l = SYSCALLROW;
	c = 0;
	move(l, c);
	for (ii = 0; ii < nelem(s.counts); ii++) {
		i = syscall_sort[ii];
		switch (show) {
		default:
		case SHOW_COUNTS:
			val = s.counts[i] - s1.counts[i];
			break;
		case SHOW_TIMES:
			val = s.times[i] - s1.times[i];
			break;
		case SHOW_COUNTS | SHOW_TIMES:
			val = s.counts[i] - s1.counts[i];
			if (val != 0)
				val = (s.times[i] - s1.times[i]) / val;
		}
		if (val == 0 && irf.counts[i] == 0 && irf.times[i] == 0)
			continue;

		if (i < nelem(syscallnames)) {
			const char *name = syscallnames[i];
			while (name[0] == '_')
				name++;
			if (name[0] == 'c' && !strcmp(name + 1, "ompat_"))
				name += 7;
			mvprintw(l, c, "%17.17s", name);
		} else
			mvprintw(l, c, "syscall #%d       ", i);
			
		putint((unsigned int)((double)val/etime + 0.5), l, c + 17, 9);
		c += 27;
		if (c + 26 > COLS) {
			c = 0;
			l++;
			if (l >= LINES - 1)
				break;
		}
	}
	if (display_mode == TIME) {
		memcpy(s1.counts, s.counts, sizeof s1.counts);
		memcpy(s1.times, s.times, sizeof s1.times);
	}
	while (l < LINES - 1) {
	    clrtoeol();
	    move(++l, 0);
	}
}

void
syscall_boot(char *args)
{
	memset(&s1, 0, sizeof s1);
	display_mode = BOOT;
}

void
syscall_run(char *args)
{
	s1 = s2;
	display_mode = RUN;
}

void
syscall_time(char *args)
{
	display_mode = TIME;
}

void
syscall_zero(char *args)
{
	s1 = s;
}

static int
compare_names(const void *a, const void *b)
{
	const char *name_a = syscallnames[*(const int *)a];
	const char *name_b = syscallnames[*(const int *)b];

	while (*name_a == '_')
		name_a++;
	while (*name_b == '_')
		name_b++;

	return strcmp(name_a, name_b);
}

void
syscall_order(char *args)
{
	int i, len;

	if (args == NULL)
		goto usage;

	len = strcspn(args, " \t\r\n");

	if (args[len + strspn(args + len, " \t\r\n")])
		goto usage;
	
	if (memcmp(args, "count", len) == 0) {
		sort_order = COUNTS;
		memset(&irf, 0, sizeof irf);
	} else if (memcmp(args, "name", len) == 0)
		sort_order = NAMES;
	else if (memcmp(args, "syscall", len) == 0)
		sort_order = UNSORTED;
	else
		goto usage;

	/* Undo all the sorting */
	for (i = 0; i < nelem(syscall_sort); i++)
		syscall_sort[i] = i;

	if (sort_order == NAMES) {
		/* Only sort the entries we have names for */
		qsort(syscall_sort, nelem(syscallnames), sizeof syscall_sort[0],
			compare_names);
	}
	return;

    usage:
	error("Usage: sort [count|name|syscall]");
}

void
syscall_show(char *args)
{
	int len;

	if (args == NULL)
		goto usage;

	len = strcspn(args, " \t\r\n");

	if (args[len + strspn(args + len, " \t\r\n")])
		goto usage;
	
	if (memcmp(args, "counts", len) == 0)
		show = SHOW_COUNTS;
	else if (memcmp(args, "times", len) == 0)
		show = SHOW_TIMES;
	else if (memcmp(args, "ratio", len) == 0)
		show = SHOW_COUNTS | SHOW_TIMES;
	else
		goto usage;

	return;

    usage:
	error("Usage: show [counts|times|ratio]");
}

static void
getinfo(struct Info *stats, int get_what)
{
	int mib[2];
	size_t size;

	cpureadstats();

	if (get_what & SHOW_COUNTS) {
		size = sizeof stats->counts;
		if (!counts_mib_len ||
		    sysctl(counts_mib, counts_mib_len, &stats->counts, &size,
			    NULL, 0)) {
			error("can't get syscall counts: %s\n", strerror(errno));
			memset(&stats->counts, 0, sizeof stats->counts);
		}
	}

	if (get_what & SHOW_TIMES) {
		size = sizeof stats->times;
		if (!times_mib_len ||
		    sysctl(times_mib, times_mib_len, &stats->times, &size,
			    NULL, 0)) {
			error("can't get syscall times: %s\n", strerror(errno));
			memset(&stats->times, 0, sizeof stats->times);
		}
	}

	size = sizeof(stats->uvmexp);
	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP2;
	if (sysctl(mib, 2, &stats->uvmexp, &size, NULL, 0) < 0) {
		error("can't get uvmexp: %s\n", strerror(errno));
		memset(&stats->uvmexp, 0, sizeof(stats->uvmexp));
	}
	size = sizeof(stats->Total);
	mib[0] = CTL_VM;
	mib[1] = VM_METER;
	if (sysctl(mib, 2, &stats->Total, &size, NULL, 0) < 0) {
		error("Can't get kernel info: %s\n", strerror(errno));
		memset(&stats->Total, 0, sizeof(stats->Total));
	}
}
