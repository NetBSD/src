/*	$NetBSD: vmstat.c,v 1.67 2006/05/14 02:56:27 christos Exp $	*/

/*-
 * Copyright (c) 1983, 1989, 1992, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 1/12/94";
#endif
__RCSID("$NetBSD: vmstat.c,v 1.67 2006/05/14 02:56:27 christos Exp $");
#endif /* not lint */

/*
 * Cursed vmstat -- from Robert Elz.
 */

#include <sys/param.h>
#include <sys/uio.h>
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

static struct Info {
	struct	uvmexp_sysctl uvmexp;
	struct	vmtotal Total;
	struct	nchstats nchstats;
	long	nchcount;
	long	*intrcnt;
	u_int64_t	*evcnt;
} s, s1, s2, z;

enum display_mode display_mode = TIME;

static void allocinfo(struct Info *);
static void copyinfo(struct Info *, struct Info *);
static float cputime(int);
static void dinfo(int, int, int);
static void getinfo(struct Info *);
static int ucount(void);

static	char buf[26];
static	u_int64_t temp;
double etime;
static	float hertz;
static	int nintr;
static	long *intrloc;
static	char **intrname;
static	int nextintsrow;
static	int disk_horiz = 1;

WINDOW *
openvmstat(void)
{
	return (stdscr);
}

void
closevmstat(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
}


static struct nlist namelist[] = {
#define	X_NCHSTATS	0
	{ "_nchstats" },
#define	X_INTRNAMES	1
	{ "_intrnames" },
#define	X_EINTRNAMES	2
	{ "_eintrnames" },
#define	X_INTRCNT	3
	{ "_intrcnt" },
#define	X_EINTRCNT	4
	{ "_eintrcnt" },
#define	X_ALLEVENTS	5
	{ "_allevents" },
	{ NULL }
};

/*
 * These constants define where the major pieces are laid out
 */
#define STATROW		 0	/* uses 1 row and 68 cols */
#define STATCOL		 2
#define MEMROW		 9	/* uses 4 rows and 31 cols */
#define MEMCOL		 0
#define PAGEROW		 2	/* uses 4 rows and 26 cols */
#define PAGECOL		54
#define INTSROW		 9	/* uses all rows to bottom and 17 cols */
#define INTSCOL		40
#define INTSCOLEND	(VMSTATCOL - 0)
#define PROCSROW	 2	/* uses 2 rows and 20 cols */
#define PROCSCOL	 0
#define GENSTATROW	 2	/* uses 2 rows and 30 cols */
#define GENSTATCOL	17
#define VMSTATROW	 7	/* uses 17 rows and 15 cols */
#define VMSTATCOL	64
#define GRAPHROW	 5	/* uses 3 rows and 51 cols */
#define GRAPHCOL	 0
#define NAMEIROW	14	/* uses 3 rows and 38 cols */
#define NAMEICOL	 0
#define DISKROW		18	/* uses 5 rows and 50 cols (for 9 drives) */
#define DISKCOL		 0
#define DISKCOLWIDTH	 6
#define DISKCOLEND	INTSCOL

typedef struct intr_evcnt intr_evcnt_t;
struct intr_evcnt {
	char		*ie_group;
	char		*ie_name;
	u_int64_t	*ie_count;	/* kernel address... */
	int		ie_loc;		/* screen row */
} *ie_head;
int nevcnt;

static void
get_interrupt_events(void)
{
	struct evcntlist allevents;
	struct evcnt evcnt, *evptr;
	intr_evcnt_t *ie;
	intr_evcnt_t *n;

	if (!NREAD(X_ALLEVENTS, &allevents, sizeof allevents))
		return;
	evptr = TAILQ_FIRST(&allevents);
	for (; evptr != NULL; evptr = TAILQ_NEXT(&evcnt, ev_list)) {
		if (!KREAD(evptr, &evcnt, sizeof evcnt))
			return;
		if (evcnt.ev_type != EVCNT_TYPE_INTR)
			continue;
		n = realloc(ie_head, sizeof *ie * (nevcnt + 1));
		if (n == NULL) {
			error("realloc failed");
			die(0);
		}
		ie_head = n;
		ie = ie_head + nevcnt;
		ie->ie_group = malloc(evcnt.ev_grouplen + 1);
		ie->ie_name = malloc(evcnt.ev_namelen + 1);
		if (ie->ie_group == NULL || ie->ie_name == NULL)
			return;
		if (!KREAD(evcnt.ev_group, ie->ie_group, evcnt.ev_grouplen + 1))
			return;
		if (!KREAD(evcnt.ev_name, ie->ie_name, evcnt.ev_namelen + 1))
			return;
		ie->ie_count = &evptr->ev_count;
		ie->ie_loc = 0;
		nevcnt++;
	}
}

int
initvmstat(void)
{
	static char *intrnamebuf;
	char *cp;
	int i;

	if (intrnamebuf)
		free(intrnamebuf);
	if (intrname)
		free(intrname);
	if (intrloc)
		free(intrloc);

	if (namelist[0].n_type == 0) {
		if (kvm_nlist(kd, namelist) &&
		    (namelist[X_NCHSTATS].n_type == 0 ||
		     namelist[X_ALLEVENTS].n_type == 0)) {
			nlisterr(namelist);
			return(0);
		}
		if (namelist[0].n_type == 0) {
			error("No namelist");
			return(0);
		}
	}
	hertz = stathz ? stathz : hz;
	if (!drvinit(1))
		return(0);

	/* Old style interrupt counts - deprecated */
	nintr = (namelist[X_EINTRCNT].n_value -
		namelist[X_INTRCNT].n_value) / sizeof (long);
	if (nintr) {
		intrloc = calloc(nintr, sizeof (long));
		intrname = calloc(nintr, sizeof (long));
		intrnamebuf = malloc(namelist[X_EINTRNAMES].n_value -
				     namelist[X_INTRNAMES].n_value);
		if (intrnamebuf == NULL || intrname == 0 || intrloc == 0) {
			error("Out of memory\n");
			nintr = 0;
			return(0);
		}
		NREAD(X_INTRNAMES, intrnamebuf, NVAL(X_EINTRNAMES) -
		      NVAL(X_INTRNAMES));
		for (cp = intrnamebuf, i = 0; i < nintr; i++) {
			intrname[i] = cp;
			cp += strlen(cp) + 1;
		}
	}

	/* event counter interrupt counts */
	get_interrupt_events();

	nextintsrow = INTSROW + 1;
	allocinfo(&s);
	allocinfo(&s1);
	allocinfo(&s2);
	allocinfo(&z);

	getinfo(&s2);
	copyinfo(&s2, &s1);
	return(1);
}

void
fetchvmstat(void)
{
	time_t now;

	time(&now);
	strlcpy(buf, ctime(&now), sizeof(buf));
	buf[19] = '\0';
	getinfo(&s);
}

static void
print_ie_title(int i)
{
	int width, name_width, group_width;

	width = INTSCOLEND - (INTSCOL + 9);
	if (width <= 0)
		return;

	move(ie_head[i].ie_loc, INTSCOL + 9);
	group_width = strlen(ie_head[i].ie_group);
	name_width = strlen(ie_head[i].ie_name);
	width -= group_width + 1 + name_width;
	if (width < 0) {
		/*
		 * Screen to narrow for full strings
		 * This is all rather horrid, in some cases there are a lot
		 * of events in the same group, and in others the event
		 * name is "intr".  There are also names which need 7 or 8
		 * columns before they become meaningful.
		 * This is a bad compromise.
		 */
		width = -width;
		group_width -= (width + 1) / 2;
		name_width -= width / 2;
		/* some have the 'useful' name "intr", display their group */
		if (strcasecmp(ie_head[i].ie_name, "intr") == 0) {
			 group_width += name_width + 1;
			 name_width = 0;
		} else {
			if (group_width <= 3 || name_width < 0) {
				/* don't display group */
				name_width += group_width + 1;
				group_width = 0;
			}
		}
	}

	if (group_width != 0) {
		printw("%-.*s", group_width, ie_head[i].ie_group);
		if (name_width != 0)
			printw(" ");
	}
	if (name_width != 0)
		printw("%-.*s", name_width, ie_head[i].ie_name);
}

void
labelvmstat_top(void)
{

	clear();

	mvprintw(STATROW, STATCOL + 4, "users    Load");

	mvprintw(GENSTATROW, GENSTATCOL, "   Csw    Trp    Sys   Int   Sof    Flt");

	mvprintw(GRAPHROW, GRAPHCOL,
		"    . %% Sy    . %% Us    . %% Ni    . %% In    . %% Id");
	mvprintw(PROCSROW, PROCSCOL, "Proc:r  d  s  w");
	mvprintw(GRAPHROW + 1, GRAPHCOL,
		"|    |    |    |    |    |    |    |    |    |    |");

	mvprintw(PAGEROW, PAGECOL + 8, "PAGING   SWAPPING ");
	mvprintw(PAGEROW + 1, PAGECOL, "        in  out   in  out ");
	mvprintw(PAGEROW + 2, PAGECOL + 2, "ops");
	mvprintw(PAGEROW + 3, PAGECOL, "pages");
}

void
labelvmstat(void)
{
	int i;

	/* Top few lines first */

	labelvmstat_top();

	/* Left hand column */

	mvprintw(MEMROW, MEMCOL,     "           memory totals (in kB)");
	mvprintw(MEMROW + 1, MEMCOL, "          real  virtual     free");
	mvprintw(MEMROW + 2, MEMCOL, "Active");
	mvprintw(MEMROW + 3, MEMCOL, "All");

	mvprintw(NAMEIROW, NAMEICOL, "Namei         Sys-cache     Proc-cache");
	mvprintw(NAMEIROW + 1, NAMEICOL,
		"    Calls     hits    %%     hits     %%");

	mvprintw(DISKROW, DISKCOL, "Disks:");
	if (disk_horiz) {
		mvprintw(DISKROW + 1, DISKCOL + 1, "seeks");
		mvprintw(DISKROW + 2, DISKCOL + 1, "xfers");
		mvprintw(DISKROW + 3, DISKCOL + 1, "bytes");
		mvprintw(DISKROW + 4, DISKCOL + 1, "%%busy");
	} else {
		mvprintw(DISKROW, DISKCOL + 1 + 1 * DISKCOLWIDTH, "seeks");
		mvprintw(DISKROW, DISKCOL + 1 + 2 * DISKCOLWIDTH, "xfers");
		mvprintw(DISKROW, DISKCOL + 1 + 3 * DISKCOLWIDTH, "bytes");
		mvprintw(DISKROW, DISKCOL + 1 + 4 * DISKCOLWIDTH, "%%busy");
	}

	/* Middle column */

	mvprintw(INTSROW, INTSCOL + 9, "Interrupts");
	for (i = 0; i < nintr; i++) {
		if (intrloc[i] == 0)
			continue;
		mvprintw(intrloc[i], INTSCOL + 9, "%-.*s",
			INTSCOLEND - (INTSCOL + 9), intrname[i]);
	}
	for (i = 0; i < nevcnt; i++) {
		if (ie_head[i].ie_loc == 0)
			continue;
		print_ie_title(i);
	}

	/* Right hand column */

	mvprintw(VMSTATROW + 0, VMSTATCOL + 10, "forks");
	mvprintw(VMSTATROW + 1, VMSTATCOL + 10, "fkppw");
	mvprintw(VMSTATROW + 2, VMSTATCOL + 10, "fksvm");
	mvprintw(VMSTATROW + 3, VMSTATCOL + 10, "pwait");
	mvprintw(VMSTATROW + 4, VMSTATCOL + 10, "relck");
	mvprintw(VMSTATROW + 5, VMSTATCOL + 10, "rlkok");
	mvprintw(VMSTATROW + 6, VMSTATCOL + 10, "noram");
	mvprintw(VMSTATROW + 7, VMSTATCOL + 10, "ndcpy");
	mvprintw(VMSTATROW + 8, VMSTATCOL + 10, "fltcp");
	mvprintw(VMSTATROW + 9, VMSTATCOL + 10, "zfod");
	mvprintw(VMSTATROW + 10, VMSTATCOL + 10, "cow");
	mvprintw(VMSTATROW + 11, VMSTATCOL + 10, "fmin");
	mvprintw(VMSTATROW + 12, VMSTATCOL + 10, "ftarg");
	mvprintw(VMSTATROW + 13, VMSTATCOL + 10, "itarg");
	mvprintw(VMSTATROW + 14, VMSTATCOL + 10, "wired");
	mvprintw(VMSTATROW + 15, VMSTATCOL + 10, "pdfre");

	if (LINES - 1 > VMSTATROW + 16)
		mvprintw(VMSTATROW + 16, VMSTATCOL + 10, "pdscn");
}

#define X(s, s1, fld)	{temp = (s).fld[i]; (s).fld[i] -= (s1).fld[i]; \
			if (display_mode == TIME) (s1).fld[i] = temp;}
#define Z(s, s1, fld)	{temp = (s).nchstats.fld; \
			(s).nchstats.fld -= (s1).nchstats.fld; \
			if (display_mode == TIME) (s1).nchstats.fld = temp;}
#define PUTRATE(s, s1, fld, l, c, w) \
			{temp = (s).fld; (s).fld -= (s1).fld; \
			if (display_mode == TIME) (s1).fld = temp; \
			putint((int)((float)(s).fld/etime + 0.5), l, c, w);}
#define MAXFAIL 5

static	char cpuchar[CPUSTATES] = { '=' , '>', '-', '%', ' ' };
static	char cpuorder[CPUSTATES] = { CP_SYS, CP_USER, CP_NICE, CP_INTR, CP_IDLE };

void
show_vmstat_top(vmtotal_t *Total, uvmexp_sysctl_t *uvm, uvmexp_sysctl_t *uvm1)
{
	float f1, f2;
	int psiz;
	int i, l, c;
	struct {
		struct uvmexp_sysctl *uvmexp;
	} us, us1;

	us.uvmexp = uvm;
	us1.uvmexp = uvm1;

	putint(ucount(), STATROW, STATCOL, 3);
	putfloat(avenrun[0], STATROW, STATCOL + 17, 6, 2, 0);
	putfloat(avenrun[1], STATROW, STATCOL + 23, 6, 2, 0);
	putfloat(avenrun[2], STATROW, STATCOL + 29, 6, 2, 0);
	mvaddstr(STATROW, STATCOL + 53, buf);

	putint(Total->t_rq - 1, PROCSROW + 1, PROCSCOL + 3, 3);
	putint(Total->t_dw, PROCSROW + 1, PROCSCOL + 6, 3);
	putint(Total->t_sl, PROCSROW + 1, PROCSCOL + 9, 3);
	putint(Total->t_sw, PROCSROW + 1, PROCSCOL + 12, 3);

	PUTRATE(us, us1, uvmexp->swtch, GENSTATROW + 1, GENSTATCOL - 1, 7);
	PUTRATE(us, us1, uvmexp->traps, GENSTATROW + 1, GENSTATCOL + 6, 7);
	PUTRATE(us, us1, uvmexp->syscalls, GENSTATROW + 1, GENSTATCOL + 13, 7);
	PUTRATE(us, us1, uvmexp->intrs, GENSTATROW + 1, GENSTATCOL + 20, 6);
	PUTRATE(us, us1, uvmexp->softs, GENSTATROW + 1, GENSTATCOL + 26, 6);
	PUTRATE(us, us1, uvmexp->faults, GENSTATROW + 1, GENSTATCOL + 32, 7);

	/* Last CPU state not calculated yet. */
	for (f2 = 0.0, psiz = 0, c = 0; c < CPUSTATES; c++) {
		i = cpuorder[c];
		f1 = cputime(i);
		f2 += f1;
		l = (int) ((f2 + 1.0) / 2.0) - psiz;
		if (c == 0)
			putfloat(f1, GRAPHROW, GRAPHCOL + 1, 5, 1, 0);
		else
			putfloat(f1, GRAPHROW, GRAPHCOL + 10 * c + 1, 5, 1, 0);
		mvhline(GRAPHROW + 2, psiz, cpuchar[c], l);
		psiz += l;
	}

	PUTRATE(us, us1, uvmexp->pageins, PAGEROW + 2, PAGECOL + 5, 5);
	PUTRATE(us, us1, uvmexp->pdpageouts, PAGEROW + 2, PAGECOL + 10, 5);
	PUTRATE(us, us1, uvmexp->swapins, PAGEROW + 2, PAGECOL + 15, 5);
	PUTRATE(us, us1, uvmexp->swapouts, PAGEROW + 2, PAGECOL + 20, 5);
	PUTRATE(us, us1, uvmexp->pgswapin, PAGEROW + 3, PAGECOL + 5, 5);
	PUTRATE(us, us1, uvmexp->pgswapout, PAGEROW + 3, PAGECOL + 10, 5);
}

void
showvmstat(void)
{
	int inttotal;
	int i, l, r, c;
	static int failcnt = 0;
	static int relabel = 0;
	static int last_disks = 0;
	static char pigs[] = "pigs";

	if (relabel) {
		labelvmstat();
		relabel = 0;
	}

	cpuswap();
	if (display_mode == TIME) {
		drvswap();
		etime = cur.cp_etime;
		/* < 5 ticks - ignore this trash */
		if ((etime * hertz) < 1.0) {
			if (failcnt++ > MAXFAIL)
				return;
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

	show_vmstat_top(&s.Total, &s.uvmexp, &s1.uvmexp);

	/* Memory totals */
#define pgtokb(pg)	((pg) * (s.uvmexp.pagesize / 1024))
	putint(pgtokb(s.uvmexp.active), MEMROW + 2, MEMCOL + 6, 8);
	putint(pgtokb(s.uvmexp.active + s.uvmexp.swpginuse),	/* XXX */
	    MEMROW + 2, MEMCOL + 15, 8);
	putint(pgtokb(s.uvmexp.npages - s.uvmexp.free), MEMROW + 3, MEMCOL + 6, 8);
	putint(pgtokb(s.uvmexp.npages - s.uvmexp.free + s.uvmexp.swpginuse),
	    MEMROW + 3, MEMCOL + 15, 8);
	putint(pgtokb(s.uvmexp.free), MEMROW + 2, MEMCOL + 24, 8);
	putint(pgtokb(s.uvmexp.free + s.uvmexp.swpages - s.uvmexp.swpginuse),
	    MEMROW + 3, MEMCOL + 24, 8);
#undef pgtokb

	/* Namei cache */
	Z(s, s1, ncs_goodhits); Z(s, s1, ncs_badhits); Z(s, s1, ncs_miss);
	Z(s, s1, ncs_long); Z(s, s1, ncs_pass2); Z(s, s1, ncs_2passes);
	s.nchcount = s.nchstats.ncs_goodhits + s.nchstats.ncs_badhits +
	    s.nchstats.ncs_miss + s.nchstats.ncs_long;
	if (display_mode == TIME)
		s1.nchcount = s.nchcount;

	putint(s.nchcount, NAMEIROW + 2, NAMEICOL, 9);
	putint(s.nchstats.ncs_goodhits, NAMEIROW + 2, NAMEICOL + 9, 9);
#define nz(x)	((x) ? (x) : 1)
	putfloat(s.nchstats.ncs_goodhits * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 19, 4, 0, 1);
	putint(s.nchstats.ncs_pass2, NAMEIROW + 2, NAMEICOL + 23, 9);
	putfloat(s.nchstats.ncs_pass2 * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 34, 4, 0, 1);
#undef nz

	/* Disks */
	for (l = 0, i = 0, r = DISKROW, c = DISKCOL;
	     i < ndrive; i++) {
		if (!drv_select[i])
			continue;

		if (disk_horiz)
			c += DISKCOLWIDTH;
		else
			r++;
		if (c + DISKCOLWIDTH > DISKCOLEND) {
			if (disk_horiz && LINES - 1 - DISKROW >
			    (DISKCOLEND - DISKCOL) / DISKCOLWIDTH) {
				disk_horiz = 0;
				relabel = 1;
			}
			break;
		}
		if (r >= LINES - 1) {
			if (!disk_horiz && LINES - 1 - DISKROW <
			    (DISKCOLEND - DISKCOL) / DISKCOLWIDTH) {
				disk_horiz = 1;
				relabel = 1;
			}
			break;
		}
		l++;

		dinfo(i, r, c);
	}
	/* blank out if we lost any disks */
	for (i = l; i < last_disks; i++) {
		int j;
		if (disk_horiz)
			c += DISKCOLWIDTH;
		else
			r++;
		for (j = 0; j < 5; j++) {
			if (disk_horiz)
				mvprintw(r+j, c, "%*s", DISKCOLWIDTH, "");
			else
				mvprintw(r, c+j*DISKCOLWIDTH, "%*s", DISKCOLWIDTH, "");
		}
	}
	last_disks = l;

	/* Interrupts */
	failcnt = 0;
	inttotal = 0;
	for (i = 0; i < nintr; i++) {
		if (s.intrcnt[i] == 0)
			continue;
		if (intrloc[i] == 0) {
			if (nextintsrow == LINES)
				continue;
			intrloc[i] = nextintsrow++;
			mvprintw(intrloc[i], INTSCOL + 9, "%-.*s",
				INTSCOLEND - (INTSCOL + 9), intrname[i]);
		}
		X(s, s1, intrcnt);
		l = (int)((float)s.intrcnt[i]/etime + 0.5);
		inttotal += l;
		putint(l, intrloc[i], INTSCOL, 8);
	}

	for (i = 0; i < nevcnt; i++) {
		if (s.evcnt[i] == 0)
			continue;
		if (ie_head[i].ie_loc == 0) {
			if (nextintsrow == LINES)
				continue;
			ie_head[i].ie_loc = nextintsrow++;
			print_ie_title(i);
		}
		X(s, s1, evcnt);
		l = (int)((float)s.evcnt[i]/etime + 0.5);
		inttotal += l;
		putint(l, ie_head[i].ie_loc, INTSCOL, 8);
	}
	putint(inttotal, INTSROW, INTSCOL, 8);

	PUTRATE(s, s1, uvmexp.forks, VMSTATROW + 0, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.forks_ppwait, VMSTATROW + 1, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.forks_sharevm, VMSTATROW + 2, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.fltpgwait, VMSTATROW + 3, VMSTATCOL + 4, 5);
	PUTRATE(s, s1, uvmexp.fltrelck, VMSTATROW + 4, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.fltrelckok, VMSTATROW + 5, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.fltnoram, VMSTATROW + 6, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.fltamcopy, VMSTATROW + 7, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.flt_prcopy, VMSTATROW + 8, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.flt_przero, VMSTATROW + 9, VMSTATCOL + 3, 6);
	PUTRATE(s, s1, uvmexp.flt_acow, VMSTATROW + 10, VMSTATCOL, 9);
	putint(s.uvmexp.freemin, VMSTATROW + 11, VMSTATCOL, 9);
	putint(s.uvmexp.freetarg, VMSTATROW + 12, VMSTATCOL, 9);
	putint(s.uvmexp.inactarg, VMSTATROW + 13, VMSTATCOL, 9);
	putint(s.uvmexp.wired, VMSTATROW + 14, VMSTATCOL, 9);
	PUTRATE(s, s1, uvmexp.pdfreed, VMSTATROW + 15, VMSTATCOL, 9);
	if (LINES - 1 > VMSTATROW + 16)
		PUTRATE(s, s1, uvmexp.pdscans, VMSTATROW + 16, VMSTATCOL, 9);

}

void
vmstat_boot(char *args)
{
	copyinfo(&z, &s1);
	display_mode = BOOT;
}

void
vmstat_run(char *args)
{
	copyinfo(&s1, &s2);
	display_mode = RUN;
}

void
vmstat_time(char *args)
{
	display_mode = TIME;
}

void
vmstat_zero(char *args)
{
	if (display_mode == RUN)
		getinfo(&s1);
}

/* calculate number of users on the system */
static int
ucount(void)
{
	static int onusers = -1;
	static struct utmpentry *oehead = NULL;
	int nusers = 0;
	struct utmpentry *ehead;

	nusers = getutentries(NULL, &ehead);
	if (oehead != ehead) {
		freeutentries(oehead);
		oehead = ehead;
	}

	if (nusers != onusers) {
		if (nusers == 1)
			mvprintw(STATROW, STATCOL + 8, " ");
		else
			mvprintw(STATROW, STATCOL + 8, "s");
	}
	onusers = nusers;
	return (nusers);
}

static float
cputime(int indx)
{
	double t;
	int i;

	t = 0;
	for (i = 0; i < CPUSTATES; i++)
		t += cur.cp_time[i];
	if (t == 0.0)
		t = 1.0;
	return (cur.cp_time[indx] * 100.0 / t);
}

void
puthumanint(u_int64_t n, int l, int c, int w)
{
	char b[128];

	if (move(l, c) != OK)
		return;
	if (n == 0) {
		hline(' ', w);
		return;
	}
	if (humanize_number(b, w, n, "", HN_AUTOSCALE, HN_NOSPACE) == -1 ) {
		hline('*', w);
		return;
	}
	printw("%*s", w, b);
}

void
putint(int n, int l, int c, int w)
{
	char b[128];

	if (move(l, c) != OK)
		return;
	if (n == 0) {
		hline(' ', w);
		return;
	}
	(void)snprintf(b, sizeof b, "%*d", w, n);
	if (strlen(b) > w) {
		if (display_mode == TIME)
			hline('*', w);
		else
			puthumanint(n, l, c, w);
		return;
	}
	addstr(b);
}

void
putfloat(double f, int l, int c, int w, int d, int nz)
{
	char b[128];

	if (move(l, c) != OK)
		return;
	if (nz && f == 0.0) {
		hline(' ', w);
		return;
	}
	(void)snprintf(b, sizeof b, "%*.*f", w, d, f);
	if (strlen(b) > w) {
		hline('*', w);
		return;
	}
	addstr(b);
}

static void
getinfo(struct Info *stats)
{
	int mib[2];
	size_t size;
	int i;

	cpureadstats();
	drvreadstats();
	NREAD(X_NCHSTATS, &stats->nchstats, sizeof stats->nchstats);
	if (nintr)
		NREAD(X_INTRCNT, stats->intrcnt, nintr * LONG);
	for (i = 0; i < nevcnt; i++)
		KREAD(ie_head[i].ie_count, &stats->evcnt[i],
		      sizeof stats->evcnt[i]);
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

static void
allocinfo(struct Info *stats)
{

	if (nintr &&
	    (stats->intrcnt = calloc(nintr, sizeof(long))) == NULL) {
		error("calloc failed");
		die(0);
	}
	if ((stats->evcnt = calloc(nevcnt, sizeof(u_int64_t))) == NULL) {
		error("calloc failed");
		die(0);
	}
}

static void
copyinfo(struct Info *from, struct Info *to)
{
	long *intrcnt;
	u_int64_t *evcnt;

	intrcnt = to->intrcnt;
	evcnt = to->evcnt;
	*to = *from;
	memmove(to->intrcnt = intrcnt, from->intrcnt, nintr * sizeof *intrcnt);
	memmove(to->evcnt = evcnt, from->evcnt, nevcnt * sizeof *evcnt);
}

static void
dinfo(int dn, int r, int c)
{
	double atime;
#define ADV if (disk_horiz) r++; else c += DISKCOLWIDTH

	mvprintw(r, c, "%*.*s", DISKCOLWIDTH, DISKCOLWIDTH, dr_name[dn]);
	ADV;

	putint((int)(cur.seek[dn]/etime+0.5), r, c, DISKCOLWIDTH);
	ADV;
	putint((int)((cur.rxfer[dn]+cur.wxfer[dn])/etime+0.5),
	    r, c, DISKCOLWIDTH);
	ADV;
	puthumanint((cur.rbytes[dn] + cur.wbytes[dn]) / etime + 0.5,
		    r, c, DISKCOLWIDTH);
	ADV;

	/* time busy in disk activity */
	atime = cur.time[dn].tv_sec + cur.time[dn].tv_usec / 1000000.0;
	atime = atime * 100.0 / etime;
	if (atime >= 100)
		putint(100, r, c, DISKCOLWIDTH);
	else
		putfloat(atime, r, c, DISKCOLWIDTH, 1, 1);
}
