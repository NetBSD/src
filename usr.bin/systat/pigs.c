/*	$NetBSD: pigs.c,v 1.22 2002/05/04 18:44:27 thorpej Exp $	*/

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
static char sccsid[] = "@(#)pigs.c	8.2 (Berkeley) 9/23/93";
#endif
__RCSID("$NetBSD: pigs.c,v 1.22 2002/05/04 18:44:27 thorpej Exp $");
#endif /* not lint */

/*
 * Pigs display from Bill Reeves at Lucasfilm
 */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>

#include <curses.h>
#include <math.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"
#include "systat.h"
#include "ps.h"

int compare_pctcpu(const void *, const void *);

int nproc;
struct p_times *pt;

u_int64_t stime[CPUSTATES];
long	mempages;
int     fscale;
double  lccpu;

#ifndef P_ZOMBIE
#define P_ZOMBIE(p)	((p)->p_stat == SZOMB)
#endif

WINDOW *
openpigs(void)
{

	return (subwin(stdscr, LINES-5-1, 0, 5, 0));
}

void
closepigs(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}


void
showpigs(void)
{
	int i, y, k;
	struct kinfo_proc2 *kp;
	float total;
	int factor;
	const char *pname;
	char pidname[30], pidstr[7], usrstr[9];

	if (pt == NULL)
		return;
	/* Accumulate the percent of cpu per user. */
	total = 0.0;
	for (i = 0; i <= nproc; i++) {
		/* Accumulate the percentage. */
		total += pt[i].pt_pctcpu;
	}

	if (total < 1.0)
 		total = 1.0;
	factor = 50.0/total;

	qsort(pt, nproc + 1, sizeof (struct p_times), compare_pctcpu);
	y = 1;
	i = nproc + 1;
	if (i > getmaxy(wnd)-1)
		i = getmaxy(wnd)-1;
	for (k = 0; i > 0 && pt[k].pt_pctcpu > 0.01; i--, y++, k++) {
		if (pt[k].pt_kp == NULL) {
			pname = "<idle>";
			pidstr[0] = '\0';
			usrstr[0] = '\0';
		}
		else {
			kp = pt[k].pt_kp;
			pname = kp->p_comm;
			snprintf(pidstr, sizeof(pidstr), "%5d", kp->p_pid);
			snprintf(usrstr, sizeof(usrstr), "%8s",
			    user_from_uid(kp->p_uid, 0));
		}
		wmove(wnd, y, 0);
		wclrtoeol(wnd);
		mvwaddstr(wnd, y, 0, usrstr);
		mvwaddstr(wnd, y, 9, pidstr);
		(void)snprintf(pidname, sizeof(pidname), "%9.9s", pname);
		mvwaddstr(wnd, y, 15, pidname);
		mvwhline(wnd, y, 25, 'X', pt[k].pt_pctcpu*factor + 0.5);
	}
	wmove(wnd, y, 0); wclrtobot(wnd);
}

static struct nlist namelist[] = {
#define X_FIRST		0
#define X_CCPU          0
	{ "_ccpu" },
#define X_FSCALE        1
	{ "_fscale" },
#define X_PHYSMEM	2
	{ "_physmem" },
	{ "" }
};

int
initpigs(void)
{
	fixpt_t ccpu;

	if (namelist[X_FIRST].n_type == 0) {
		if (kvm_nlist(kd, namelist)) {
			nlisterr(namelist);
		        return(0);
		}
		if (namelist[X_FIRST].n_type == 0) {
			error("namelist failed");
			return(0);
		}
	}
	(void) fetch_cptime(stime);
	KREAD(NPTR(X_PHYSMEM), &mempages, sizeof (mempages));
	NREAD(X_CCPU, &ccpu, sizeof ccpu);
	NREAD(X_FSCALE,  &fscale, sizeof fscale);
	lccpu = log((double) ccpu / fscale);

	return(1);
}

void
fetchpigs(void)
{
	int i;
	float time;
	float *pctp;
	struct kinfo_proc2 *kpp, *k;
	u_int64_t ctime[CPUSTATES];
	double t;
	static int lastnproc = 0;

	if (namelist[X_FIRST].n_type == 0)
		return;
	if ((kpp = kvm_getproc2(kd, KERN_PROC_ALL, 0, sizeof(*kpp),
				&nproc)) == NULL) {
		error("%s", kvm_geterr(kd));
		if (pt)
			free(pt);
		return;
	}
	if (nproc > lastnproc) {
		free(pt);
		if ((pt =
		    malloc((nproc + 1) * sizeof(struct p_times))) == NULL) {
			error("Out of memory");
			die(0);
		}
	}
	lastnproc = nproc;
	/*
	 * calculate %cpu for each proc
	 */
	for (i = 0; i < nproc; i++) {
		pt[i].pt_kp = k = &kpp[i];
		pctp = &pt[i].pt_pctcpu;
		time = k->p_swtime;
		/* XXX - I don't like this */
		if (k->p_swtime == 0 || (k->p_flag & P_INMEM) == 0 ||
		    k->p_stat == SZOMB || k->p_stat == SDEAD)
			*pctp = 0;
		else
			*pctp = ((double) k->p_pctcpu / 
					fscale) / (1.0 - exp(time * lccpu));
	}
	/*
	 * and for the imaginary "idle" process
	 */
	(void) fetch_cptime(ctime);
	t = 0;
	for (i = 0; i < CPUSTATES; i++)
		t += ctime[i] - stime[i];
	if (t == 0.0)
		t = 1.0;
	pt[nproc].pt_kp = NULL;
	pt[nproc].pt_pctcpu = (ctime[CP_IDLE] - stime[CP_IDLE]) / t;
	for (i = 0; i < CPUSTATES; i++)
		stime[i] = ctime[i];
}

void
labelpigs(void)
{
	wmove(wnd, 0, 0);
	wclrtoeol(wnd);
	mvwaddstr(wnd, 0, 25, "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
}

int
compare_pctcpu(const void *a, const void *b)
{
	return (((struct p_times *) a)->pt_pctcpu >
		((struct p_times *) b)->pt_pctcpu)? -1: 1;
}
