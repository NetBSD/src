/*	$NetBSD: vmstat.c,v 1.49 2003/02/18 14:55:05 dsl Exp $	*/

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
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 1/12/94";
#endif
__RCSID("$NetBSD: vmstat.c,v 1.49 2003/02/18 14:55:05 dsl Exp $");
#endif /* not lint */

/*
 * Cursed vmstat -- from Robert Elz.
 */

#include <sys/param.h>
#include <sys/dkstat.h>
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

static struct Info {
	struct	uvmexp_sysctl uvmexp;
	struct	vmtotal Total;
	struct	nchstats nchstats;
	long	nchcount;
	long	*intrcnt;
	u_int64_t	*evcnt;
} s, s1, s2, z;

#define	cnt s.Cnt
#define oldcnt s1.Cnt
#define	total s.Total
#define	nchtotal s.nchstats
#define	oldnchtotal s1.nchstats

static	enum state { BOOT, TIME, RUN } state = TIME;

static void allocinfo(struct Info *);
static void copyinfo(struct Info *, struct Info *);
static float cputime(int);
static void dinfo(int, int, int);
static void getinfo(struct Info *, enum state);
static void putint(int, int, int, int);
static void putfloat(double, int, int, int, int, int);
static int ucount(void);

static	char buf[26];
static	u_int64_t t;
static	double etime;
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
	{ "" },
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
#define GENSTATCOL	18
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

	if (!NREAD(X_ALLEVENTS, &allevents, sizeof allevents))
		return;
	evptr = allevents.tqh_first;
	for (; evptr != NULL; evptr = evcnt.ev_list.tqe_next) {
		if (!KREAD(evptr, &evcnt, sizeof evcnt))
			return;
		if (evcnt.ev_type != EVCNT_TYPE_INTR)
			continue;
		ie_head = realloc(ie_head, sizeof *ie * (nevcnt + 1));
		if (ie_head == NULL) {
			error("realloc failed");
			die(0);
		}
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
	char *intrnamebuf, *cp;
	int i;

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
	hertz = stathz ? stathz : hz;
	if (!dkinit(1))
		return(0);

	/* Old style interrupt counts - deprecated */
	nintr = (namelist[X_EINTRCNT].n_value -
		namelist[X_INTRCNT].n_value) / sizeof (long);
	intrloc = calloc(nintr, sizeof (long));
	intrname = calloc(nintr, sizeof (long));
	intrnamebuf = malloc(namelist[X_EINTRNAMES].n_value -
		namelist[X_INTRNAMES].n_value);
	if (intrnamebuf == NULL || intrname == 0 || intrloc == 0) {
		error("Out of memory\n");
		if (intrnamebuf)
			free(intrnamebuf);
		if (intrname)
			free(intrname);
		if (intrloc)
			free(intrloc);
		nintr = 0;
		return(0);
	}
	NREAD(X_INTRNAMES, intrnamebuf, NVAL(X_EINTRNAMES) -
		NVAL(X_INTRNAMES));
	for (cp = intrnamebuf, i = 0; i < nintr; i++) {
		intrname[i] = cp;
		cp += strlen(cp) + 1;
	}

	/* event counter interrupt counts */
	get_interrupt_events();

	nextintsrow = INTSROW + 1;
	allocinfo(&s);
	allocinfo(&s1);
	allocinfo(&s2);
	allocinfo(&z);

	getinfo(&s2, RUN);
	copyinfo(&s2, &s1);
	return(1);
}

void
fetchvmstat(void)
{
	time_t now;

	time(&now);
	strcpy(buf, ctime(&now));
	buf[19] = '\0';
	getinfo(&s, state);
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
labelvmstat(void)
{
	int i;

	clear();
	mvprintw(STATROW, STATCOL + 4, "users    Load");
	mvprintw(MEMROW, MEMCOL,     "          memory totals (in kB)");
	mvprintw(MEMROW + 1, MEMCOL, "         real   virtual    free");
	mvprintw(MEMROW + 2, MEMCOL, "Active");
	mvprintw(MEMROW + 3, MEMCOL, "All");

	mvprintw(PAGEROW, PAGECOL, "        PAGING   SWAPPING ");
	mvprintw(PAGEROW + 1, PAGECOL, "        in  out   in  out ");
	mvprintw(PAGEROW + 2, PAGECOL, "ops");
	mvprintw(PAGEROW + 3, PAGECOL, "pages");

	mvprintw(INTSROW, INTSCOL + 9, "Interrupts");

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

	mvprintw(GENSTATROW, GENSTATCOL, " Csw   Trp   Sys  Int  Sof   Flt");

	mvprintw(GRAPHROW, GRAPHCOL,
		"    . %% Sy    . %% Us    . %% Ni    . %% In    . %% Id");
	mvprintw(PROCSROW, PROCSCOL, "Proc:r  d  s  w");
	mvprintw(GRAPHROW + 1, GRAPHCOL,
		"|    |    |    |    |    |    |    |    |    |    |");

	mvprintw(NAMEIROW, NAMEICOL, "Namei         Sys-cache     Proc-cache");
	mvprintw(NAMEIROW + 1, NAMEICOL,
		"    Calls     hits    %%     hits     %%");
	mvprintw(DISKROW, DISKCOL, "Discs:");
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
}

#define X(fld)	{t=s.fld[i]; s.fld[i]-=s1.fld[i]; if(state==TIME) s1.fld[i]=t;}
#define Y(fld)	{t = s.fld; s.fld -= s1.fld; if(state == TIME) s1.fld = t;}
#define Z(fld)	{t = s.nchstats.fld; s.nchstats.fld -= s1.nchstats.fld; \
	if(state == TIME) s1.nchstats.fld = t;}
#define PUTRATE(fld, l, c, w) {Y(fld); putint((int)((float)s.fld/etime + 0.5), l, c, w);}
#define MAXFAIL 5

static	char cpuchar[CPUSTATES] = { '=' , '>', '-', '%', ' ' };
static	char cpuorder[CPUSTATES] = { CP_SYS, CP_USER, CP_NICE, CP_INTR, CP_IDLE };

void
showvmstat(void)
{
	float f1, f2;
	int psiz, inttotal;
	int i, l, r, c;
	static int failcnt = 0;
	static int relabel = 0;
	static int last_disks = 0;

	if (relabel) {
		labelvmstat();
		relabel = 0;
	}

	if (state == TIME) {
		dkswap();
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
			command("pigs");
			return;
		}
	} else
		etime = 1.0;

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
		X(intrcnt);
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
		X(evcnt);
		l = (int)((float)s.evcnt[i]/etime + 0.5);
		inttotal += l;
		putint(l, ie_head[i].ie_loc, INTSCOL, 8);
	}
	putint(inttotal, INTSROW, INTSCOL, 8);
	Z(ncs_goodhits); Z(ncs_badhits); Z(ncs_miss);
	Z(ncs_long); Z(ncs_pass2); Z(ncs_2passes);
	s.nchcount = nchtotal.ncs_goodhits + nchtotal.ncs_badhits +
	    nchtotal.ncs_miss + nchtotal.ncs_long;
	if (state == TIME)
		s1.nchcount = s.nchcount;

	psiz = 0;
	f2 = 0.0;

	/*
	 * Last CPU state not calculated yet.
	 */
	for (c = 0; c < CPUSTATES; c++) {
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

	putint(ucount(), STATROW, STATCOL, 3);
	putfloat(avenrun[0], STATROW, STATCOL + 17, 6, 2, 0);
	putfloat(avenrun[1], STATROW, STATCOL + 23, 6, 2, 0);
	putfloat(avenrun[2], STATROW, STATCOL + 29, 6, 2, 0);
	mvaddstr(STATROW, STATCOL + 53, buf);
#define pgtokb(pg)	((pg) * (s.uvmexp.pagesize / 1024))

	putint(pgtokb(s.uvmexp.active), MEMROW + 2, MEMCOL + 6, 7);
	putint(pgtokb(s.uvmexp.active + s.uvmexp.swpginuse),	/* XXX */
	    MEMROW + 2, MEMCOL + 16, 7);
	putint(pgtokb(s.uvmexp.npages - s.uvmexp.free), MEMROW + 3, MEMCOL + 6, 7);
	putint(pgtokb(s.uvmexp.npages - s.uvmexp.free + s.uvmexp.swpginuse),
	    MEMROW + 3, MEMCOL + 16, 7);
	putint(pgtokb(s.uvmexp.free), MEMROW + 2, MEMCOL + 24, 7);
	putint(pgtokb(s.uvmexp.free + s.uvmexp.swpages - s.uvmexp.swpginuse),
	    MEMROW + 3, MEMCOL + 24, 7);
	putint(total.t_rq - 1, PROCSROW + 1, PROCSCOL + 3, 3);
	putint(total.t_dw, PROCSROW + 1, PROCSCOL + 6, 3);
	putint(total.t_sl, PROCSROW + 1, PROCSCOL + 9, 3);
	putint(total.t_sw, PROCSROW + 1, PROCSCOL + 12, 3);
	PUTRATE(uvmexp.forks, VMSTATROW + 0, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.forks_ppwait, VMSTATROW + 1, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.forks_sharevm, VMSTATROW + 2, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.fltpgwait, VMSTATROW + 3, VMSTATCOL + 4, 5);
	PUTRATE(uvmexp.fltrelck, VMSTATROW + 4, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.fltrelckok, VMSTATROW + 5, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.fltnoram, VMSTATROW + 6, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.fltamcopy, VMSTATROW + 7, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.flt_prcopy, VMSTATROW + 8, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.flt_przero, VMSTATROW + 9, VMSTATCOL + 3, 6);
	PUTRATE(uvmexp.flt_acow, VMSTATROW + 10, VMSTATCOL, 9);
	putint(s.uvmexp.freemin, VMSTATROW + 11, VMSTATCOL, 9);
	putint(s.uvmexp.freetarg, VMSTATROW + 12, VMSTATCOL, 9);
	putint(s.uvmexp.inactarg, VMSTATROW + 13, VMSTATCOL, 9);
	putint(s.uvmexp.wired, VMSTATROW + 14, VMSTATCOL, 9);
	PUTRATE(uvmexp.pdfreed, VMSTATROW + 15, VMSTATCOL, 9);
	if (LINES - 1 > VMSTATROW + 16)
		PUTRATE(uvmexp.pdscans, VMSTATROW + 16, VMSTATCOL, 9);

	PUTRATE(uvmexp.pageins, PAGEROW + 2, PAGECOL + 5, 5);
	PUTRATE(uvmexp.pdpageouts, PAGEROW + 2, PAGECOL + 10, 5);
	PUTRATE(uvmexp.swapins, PAGEROW + 2, PAGECOL + 15, 5);
	PUTRATE(uvmexp.swapouts, PAGEROW + 2, PAGECOL + 20, 5);
	PUTRATE(uvmexp.pgswapin, PAGEROW + 3, PAGECOL + 5, 5);
	PUTRATE(uvmexp.pgswapout, PAGEROW + 3, PAGECOL + 10, 5);

	PUTRATE(uvmexp.swtch, GENSTATROW + 1, GENSTATCOL, 4);
	PUTRATE(uvmexp.traps, GENSTATROW + 1, GENSTATCOL + 4, 6);
	PUTRATE(uvmexp.syscalls, GENSTATROW + 1, GENSTATCOL + 10, 6);
	PUTRATE(uvmexp.intrs, GENSTATROW + 1, GENSTATCOL + 16, 5);
	PUTRATE(uvmexp.softs, GENSTATROW + 1, GENSTATCOL + 21, 5);
	PUTRATE(uvmexp.faults, GENSTATROW + 1, GENSTATCOL + 26, 6);
	for (l = 0, i = 0, r = DISKROW, c = DISKCOL; i < dk_ndrive; i++) {
		if (!dk_select[i])
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
			mvprintw(r, c, "%*s", DISKCOLWIDTH, "");
			if (disk_horiz)
				r++;
			else
				c += DISKCOLWIDTH;
		}
	}
	last_disks = l;
	putint(s.nchcount, NAMEIROW + 2, NAMEICOL, 9);
	putint(nchtotal.ncs_goodhits, NAMEIROW + 2, NAMEICOL + 9, 9);
#define nz(x)	((x) ? (x) : 1)
	putfloat(nchtotal.ncs_goodhits * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 19, 4, 0, 1);
	putint(nchtotal.ncs_pass2, NAMEIROW + 2, NAMEICOL + 23, 9);
	putfloat(nchtotal.ncs_pass2 * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 34, 4, 0, 1);
#undef nz
}

void
vmstat_boot(char *args)
{
	copyinfo(&z, &s1);
	state = BOOT;
}

void
vmstat_run(char *args)
{
	copyinfo(&s1, &s2);
	state = RUN;
}

void
vmstat_time(char *args)
{
	state = TIME;
}

void
vmstat_zero(char *args)
{
	if (state == RUN)
		getinfo(&s1, RUN);
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

static void
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
	hline(' ', w - strlen(b));
	mvaddstr(l, c + w - strlen(b), b);
}

static void
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
		hline('*', w);
		return;
	}
	addstr(b);
}

static void
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
getinfo(struct Info *s, enum state st)
{
	int mib[2];
	size_t size;
	int i;

	dkreadstats();
	NREAD(X_NCHSTATS, &s->nchstats, sizeof s->nchstats);
	NREAD(X_INTRCNT, s->intrcnt, nintr * LONG);
	for (i = 0; i < nevcnt; i++)
		KREAD(ie_head[i].ie_count, &s->evcnt[i], sizeof s->evcnt[i]);
	size = sizeof(s->uvmexp);
	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP2;
	if (sysctl(mib, 2, &s->uvmexp, &size, NULL, 0) < 0) {
		error("can't get uvmexp: %s\n", strerror(errno));
		memset(&s->uvmexp, 0, sizeof(s->uvmexp));
	}
	size = sizeof(s->Total);
	mib[0] = CTL_VM;
	mib[1] = VM_METER;
	if (sysctl(mib, 2, &s->Total, &size, NULL, 0) < 0) {
		error("Can't get kernel info: %s\n", strerror(errno));
		memset(&s->Total, 0, sizeof(s->Total));
	}
}

static void
allocinfo(struct Info *s)
{

	if ((s->intrcnt = calloc(nintr, sizeof(long))) == NULL) {
		error("calloc failed");
		die(0);
	}
	if ((s->evcnt = calloc(nevcnt, sizeof(u_int64_t))) == NULL) {
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
	memmove(to->intrcnt = intrcnt, from->intrcnt, nintr * sizeof (int));
	memmove(to->evcnt = evcnt, from->evcnt, nevcnt * sizeof *evcnt);
}

static void
dinfo(int dn, int r, int c)
{
	double atime;
#define ADV if (disk_horiz) r++; else c += DISKCOLWIDTH

	mvprintw(r, c, "%*.*s", DISKCOLWIDTH, DISKCOLWIDTH, dr_name[dn]);
	ADV;

	putint((int)(cur.dk_seek[dn]/etime+0.5), r, c, DISKCOLWIDTH);
	ADV;
	putint((int)((cur.dk_rxfer[dn]+cur.dk_wxfer[dn])/etime+0.5),
	    r, c, DISKCOLWIDTH);
	ADV;
	puthumanint((cur.dk_rbytes[dn] + cur.dk_wbytes[dn]) / etime + 0.5,
		    r, c, DISKCOLWIDTH);
	ADV;

	/* time busy in disk activity */
	atime = cur.dk_time[dn].tv_sec + cur.dk_time[dn].tv_usec / 1000000.0;
	atime = atime * 100.0 / etime;
	if (atime >= 100)
		putint(100, r, c, DISKCOLWIDTH);
	else
		putfloat(atime, r, c, DISKCOLWIDTH, 1, 1);
#undef ADV
}
