/*      $NetBSD: ps.c,v 1.9 1999/12/16 04:41:49 jwise Exp $  */

/*-
 * Copyright (c) 1999
 *      The NetBSD Foundation, Inc.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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

/*
 * XXX Notes XXX
 * showps -- print data needed at each refresh
 * fetchps -- get data used by mode (done at each refresh)
 * labelps -- print labels (ie info not needing refreshing)
 * initps -- prepare once-only data structures for mode
 * openps -- prepare per-run information for mode, return window
 * closeps -- close mode to prepare to switch modes
 * cmdps -- optional, handle commands
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ps.c,v 1.9 1999/12/16 04:41:49 jwise Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/dkstat.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <curses.h>
#include <math.h>
#include <nlist.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>

#include "extern.h"
#include "systat.h"
#include "ps.h"

int compare_pctcpu_noidle __P((const void *, const void *));
char *state2str __P((struct kinfo_proc *));
char *tty2str __P((struct kinfo_proc *));
int rss2int __P((struct kinfo_proc *));
int vsz2int __P((struct kinfo_proc *));
char *comm2str __P((struct kinfo_proc *));
double pmem2float __P((struct kinfo_proc *));
char *start2str __P((struct kinfo_proc *));
char *time2str __P((struct kinfo_proc *));

static time_t now;

void
labelps ()
{
	mvwaddstr(wnd, 0, 0, "USER      PID %CPU %MEM    VSZ   RSS TT  STAT STARTED       TIME COMMAND");
}

void
showps ()
{
	int i, k, y, vsz, rss;
	const char *user, *comm, *state, *tty, *start, *time;
	pid_t pid;
	double pctcpu, pctmem;
	struct  eproc *ep;

	now = 0;	/* force start2str to reget current time */

	qsort(pt, nproc + 1, sizeof (struct p_times), compare_pctcpu_noidle);

	y = 1;
	i = nproc + 1;
	if (i > getmaxy(wnd)-2)
		i = getmaxy(wnd)-1;
	for (k = 0; i > 0 ; i--, y++, k++) {
		if (pt[k].pt_kp == NULL) /* We're all the way down to the imaginary idle proc */
			return;

		ep = &pt[k].pt_kp->kp_eproc;
		user = user_from_uid(ep->e_ucred.cr_uid, 0);
		pid = pt[k].pt_kp->kp_proc.p_pid;
		pctcpu = 100.0 * pt[k].pt_pctcpu;
		pctmem = pmem2float(pt[k].pt_kp);
		vsz = vsz2int(pt[k].pt_kp);
		rss = rss2int(pt[k].pt_kp);
		tty = tty2str(pt[k].pt_kp);
		state = state2str(pt[k].pt_kp);
		start = start2str(pt[k].pt_kp);
		time = time2str(pt[k].pt_kp);
		comm = comm2str(pt[k].pt_kp);
		/*comm = pt[k].pt_kp->kp_proc.p_comm; */

		wmove(wnd, y, 0);
		wclrtoeol(wnd);
		mvwprintw(wnd, y, 0, "%-8.8s%5d %4.1f %4.1f %6d %5d %-3s %-4s %7s %10.10s %s",
			user, pid, pctcpu, pctmem, vsz, rss, tty, state, start, time, comm);
	}
}

int
compare_pctcpu_noidle (a, b)
	const void *a, *b;
{
	if (((struct p_times *) a)->pt_kp == NULL)
		return 1;

	if (((struct p_times *) b)->pt_kp == NULL)
	 	return -1;

	return (((struct p_times *) a)->pt_pctcpu >
		((struct p_times *) b)->pt_pctcpu)? -1: 1;
}

/* from here down adapted from .../src/usr.bin/ps/print.c .  Any mistakes are my own, however. */
char *
state2str(kp)
	struct kinfo_proc *kp;
{       
	struct proc *p;
	struct eproc *e;
	int flag; 
	char *cp;
	char buf[5];
	static char statestr[4];

	p = &(kp->kp_proc);
	e = &(kp->kp_eproc);

	flag = p->p_flag;
	cp = buf;

	switch (p->p_stat) {
	case SSTOP:
		*cp = 'T';
		break;

	case SSLEEP:
		if (flag & P_SINTR)     /* interuptable (long) */
			*cp = p->p_slptime >= MAXSLP ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case SRUN:
	case SIDL:
		*cp = 'R';
		break;

	case SZOMB:
	case SDEAD:
		*cp = 'Z';
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & P_INMEM) {
	} else
		*cp++ = 'W';
	if (p->p_nice < NZERO)
		*cp++ = '<';
	else if (p->p_nice > NZERO)
		*cp++ = 'N';
	if (flag & P_TRACED)
		*cp++ = 'X';
	if (flag & P_WEXIT && P_ZOMBIE(p) == 0)
		*cp++ = 'E';
	if (flag & P_PPWAIT)
		*cp++ = 'V';
	if ((flag & P_SYSTEM) || p->p_holdcnt)
		*cp++ = 'L';
	if (e->e_flag & EPROC_SLEADER)
		*cp++ = 's'; 
	if ((flag & P_CONTROLT) && e->e_pgid == e->e_tpgid)
		*cp++ = '+';
	*cp = '\0';
	sprintf(statestr, "%-s",  buf);

	return statestr;
}

char *
tty2str(kp)
	struct kinfo_proc *kp;
{
	static char ttystr[4];
	char *ttyname;
	struct eproc *e;

	e = &(kp->kp_eproc);

	if (e->e_tdev == NODEV || (ttyname = devname(e->e_tdev, S_IFCHR)) == NULL)
		strcpy(ttystr, "??");
	else {
		if (strncmp(ttyname, "tty", 3) == 0 ||
		    strncmp(ttyname, "dty", 3) == 0)
			ttyname += 3;
		sprintf(ttystr, "%s%c", ttyname, e->e_flag & EPROC_CTTY ? ' ' : '-');
	}

	return ttystr;
}

#define pgtok(a)	(((a)*getpagesize())/1024)

int
vsz2int(kp)
	struct kinfo_proc *kp;
{
	struct eproc *e;
	int     i;

	e = &(kp->kp_eproc);
	i = pgtok(e->e_vm.vm_dsize + e->e_vm.vm_ssize + e->e_vm.vm_tsize);

	return ((i < 0) ? 0 : i);
} 

int
rss2int(kp)
	struct kinfo_proc *kp;
{
	struct eproc *e;
	int	i;
 
	e = &(kp->kp_eproc);
	i = pgtok(e->e_vm.vm_rssize);

	/* XXX don't have info about shared */
	return ((i < 0) ? 0 : i);
}

char *
comm2str(kp)
	struct kinfo_proc *kp;
{
	char **argv, **pt;
	static char commstr[41];
	struct proc *p;

	p = &(kp->kp_proc);
	commstr[0]='\0';

	argv = kvm_getargv(kd, kp, 40);
	if ((pt = argv) != NULL) {
		while (*pt) {
			strcat(commstr, *pt);
			pt++;
			strcat(commstr, " ");
		}
	} else {
		commstr[0] = '(';
		commstr[1] = '\0';
		strncat(commstr, p->p_comm, sizeof(commstr) - 1);
		strcat(commstr, ")");
	}

	return commstr;
}

double
pmem2float(kp)
	struct kinfo_proc *kp;
{	                       
	struct proc *p;
	struct eproc *e; 
	double fracmem;
	int szptudot;

	p = &(kp->kp_proc);
	e = &(kp->kp_eproc);

	if ((p->p_flag & P_INMEM) == 0)
	        return (0.0);
	/* XXX want pmap ptpages, segtab, etc. (per architecture) */
	szptudot = USPACE/getpagesize();
	/* XXX don't have info about shared */
	fracmem = ((double)e->e_vm.vm_rssize + szptudot)/mempages;
	return (fracmem >= 0) ? 100.0 * fracmem : 0;
}

char *
start2str(kp)
	struct kinfo_proc *kp; 
{
	struct proc *p;
	struct pstats pstats;
	struct timeval u_start;
	struct tm *tp;
	time_t startt;
	static char startstr[10];

	p = &(kp->kp_proc);

	kvm_read(kd, (u_long)&(p->p_addr->u_stats), (char *)&pstats, sizeof(pstats));
	u_start = pstats.p_start;

	startt = u_start.tv_sec;
	tp = localtime(&startt);
	if (now == 0)
	        time(&now);
	if (now - u_start.tv_sec < 24 * SECSPERHOUR) {
		/* I *hate* SCCS... */
	        static char fmt[] = __CONCAT("%l:%", "M%p");
	        strftime(startstr, sizeof(startstr) - 1, fmt, tp);
	} else if (now - u_start.tv_sec < 7 * SECSPERDAY) {
	        /* I *hate* SCCS... */
	        static char fmt[] = __CONCAT("%a%", "I%p");
	        strftime(startstr, sizeof(startstr) - 1, fmt, tp);
	} else  
	        strftime(startstr, sizeof(startstr) - 1, "%e%b%y", tp);

	return startstr;
}

char *    
time2str(kp)
	struct kinfo_proc *kp;
{	       
	long secs;
	long psecs;     /* "parts" of a second. first micro, then centi */
	static char timestr[10];
	struct proc *p;

	p = &(kp->kp_proc);
	        
	if (P_ZOMBIE(p)) {
	        secs = 0;
	        psecs = 0;
	} else {
	        /*
	         * This counts time spent handling interrupts.  We could
	         * fix this, but it is not 100% trivial (and interrupt
	         * time fractions only work on the sparc anyway).       XXX
	         */
	        secs = p->p_rtime.tv_sec;
	        psecs = p->p_rtime.tv_usec;
	        /* if (sumrusage) {
	                secs += k->ki_u.u_cru.ru_utime.tv_sec +
	                        k->ki_u.u_cru.ru_stime.tv_sec;
	                psecs += k->ki_u.u_cru.ru_utime.tv_usec +
	                        k->ki_u.u_cru.ru_stime.tv_usec;
	        } */
	        /*
	         * round and scale to 100's
	         */
	        psecs = (psecs + 5000) / 10000;
	        secs += psecs / 100;
	        psecs = psecs % 100;
	}
	snprintf(timestr, sizeof(timestr), "%3ld:%02ld.%02ld", secs/60, secs%60, psecs);

	return timestr;
}
