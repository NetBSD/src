/*	$NetBSD: syscall.c,v 1.1 2006/03/18 20:31:45 dsl Exp $	*/

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
__RCSID("$NetBSD: syscall.c,v 1.1 2006/03/18 20:31:45 dsl Exp $");

/* System call stats */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "systat.h"
#include "extern.h"
#include "dkstats.h"
#include "utmpentry.h"
#include "vmstat.h"

#include <sys/syscall.h>
#include <../../sys/kern/syscalls.c>

#define nelem(x) (sizeof (x) / sizeof *(x))

static struct Info {
	struct	uvmexp_sysctl uvmexp;
	struct	vmtotal Total;
	int     syscall[SYS_NSYSENT];
} s, s1, s2, irf;

int syscall_sort[SYS_NSYSENT];

static	enum sort_order { UNSORTED, NAMES, COUNTS } sort_order = NAMES;

static void getinfo(struct Info *);

static	char buf[26];
static	float hertz;

static size_t syscall_mib_len;
static int syscall_mib[4];

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

	/* dkinit gets number of cpus! */
	dkinit(1);

	syscall_mib_len = nelem(syscall_mib);
	if (sysctlnametomib("kern.syscalls.counts", syscall_mib, &syscall_mib_len) &&
	    sysctlnametomib("kern.syscalls", syscall_mib, &syscall_mib_len))
		syscall_mib_len = 0;

	getinfo(&s2);
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
	getinfo(&s);
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

	if (display_mode == TIME)
		return irf.syscall[ib] - irf.syscall[ia];

	return s.syscall[ib] - s1.syscall[ib] - s.syscall[ia] + s1.syscall[ia];
}

void
showsyscall(void)
{
	int i, ii, l, c;
	unsigned int val;
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
		for (i = 0; i < nelem(s.syscall); i++) {
			val = s.syscall[i] - s1.syscall[i];
			irf.syscall[i] = irf.syscall[i] * 7 / 8 + val;
		}

		/* mergesort() doesn't swap equal values about... */
		mergesort(syscall_sort, nelem(syscall_sort),
			sizeof syscall_sort[0], compare_counts);
	}

	l = SYSCALLROW;
	c = 0;
	move(l, c);
	for (ii = 0; ii < nelem(s.syscall); ii++) {
		i = syscall_sort[ii];
		val = s.syscall[i] - s1.syscall[i];
		if (val == 0 && irf.syscall[i] == 0)
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
			
		putint((int)((float)val/etime + 0.5), l, c + 17, 9);
		c += 27;
		if (c + 26 > COLS) {
			c = 0;
			l++;
			if (l >= LINES - 1)
				break;
		}
	}
	if (display_mode == TIME)
		memcpy(s1.syscall, s.syscall, sizeof s1.syscall);
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
	
	if (memcmp(args, "count", len) == 0)
		sort_order = COUNTS;
	else if (memcmp(args, "name", len) == 0)
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

static void
getinfo(struct Info *stats)
{
	int mib[2];
	size_t size;

	cpureadstats();

	size = sizeof stats->syscall;
	if (!syscall_mib_len ||
	    sysctl(syscall_mib, syscall_mib_len, &stats->syscall, &size,
		    NULL, 0)) {
		error("can't get syscall counts: %s\n", strerror(errno));
		memset(&stats->syscall, 0, sizeof stats->syscall);
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
