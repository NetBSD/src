/*      $NetBSD: ps.c,v 1.23 2003/05/15 01:00:07 itojun Exp $  */

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
 *      This product includes software developed by the NetBSD Foundation.
 * 4. Neither the name of the Foundation nor the names of its contributors
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
__RCSID("$NetBSD: ps.c,v 1.23 2003/05/15 01:00:07 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <curses.h>
#include <math.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>

#include "extern.h"
#include "systat.h"
#include "ps.h"

int compare_pctcpu_noidle(const void *, const void *);
char *state2str(struct kinfo_proc2 *);
char *tty2str(struct kinfo_proc2 *);
int rss2int(struct kinfo_proc2 *);
int vsz2int(struct kinfo_proc2 *);
char *comm2str(struct kinfo_proc2 *);
double pmem2float(struct kinfo_proc2 *);
char *start2str(struct kinfo_proc2 *);
char *time2str(struct kinfo_proc2 *);

static time_t now;

#define SHOWUSER_ANY	(uid_t)-1
static uid_t showuser = SHOWUSER_ANY;

#ifndef P_ZOMBIE
#define P_ZOMBIE(p)	((p)->p_stat == SZOMB)
#endif

void
labelps(void)
{
	mvwaddstr(wnd, 0, 0, "USER      PID %CPU %MEM    VSZ   RSS TT  STAT STARTED       TIME COMMAND");
}

void
showps(void)
{
	int i, k, y, vsz, rss;
	const char *user, *comm, *state, *tty, *start, *time;
	pid_t pid;
	double pctcpu, pctmem;
	struct kinfo_proc2 *kp;

	now = 0;	/* force start2str to reget current time */

	qsort(pt, nproc + 1, sizeof (struct p_times), compare_pctcpu_noidle);

	y = 1;
	i = nproc + 1;
	if (i > getmaxy(wnd)-2)
		i = getmaxy(wnd)-1;
	for (k = 0; i > 0 ; k++) {
		if (pt[k].pt_kp == NULL) /* We're all the way down to the imaginary idle proc */
			break;

		kp = pt[k].pt_kp;
		if (showuser != SHOWUSER_ANY && kp->p_uid != showuser)
			continue;
		user = user_from_uid(kp->p_uid, 0);
		pid = kp->p_pid;
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
		mvwprintw(wnd, y++, 0,
		    "%-8.8s%5d %4.1f %4.1f %6d %5d %-3s %-4s %7s %10.10s %s",
		    user, pid, pctcpu, pctmem, vsz, rss, tty, state, start, time, comm);
		i--;
	}
	wmove(wnd, y, 0);
	wclrtobot(wnd);
}

int
compare_pctcpu_noidle(const void *a, const void *b)
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
state2str(struct kinfo_proc2 *kp)
{       
	int flag; 
	char *cp;
	char buf[5];
	static char statestr[4];

	flag = kp->p_flag;
	cp = buf;

	switch (kp->p_stat) {
	case LSSTOP:
		*cp = 'T';
		break;

	case LSSLEEP:
		if (flag & L_SINTR)     /* interruptable (long) */
			*cp = kp->p_slptime >= maxslp ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case LSRUN:
	case LSIDL:
	case LSONPROC:
		*cp = 'R';
		break;

	case LSZOMB:
#ifdef LSDEAD
	case LSDEAD:
#endif
		*cp = 'Z';
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & L_INMEM) {
	} else
		*cp++ = 'W';
	if (kp->p_nice < NZERO)
		*cp++ = '<';
	else if (kp->p_nice > NZERO)
		*cp++ = 'N';
	if (flag & P_TRACED)
		*cp++ = 'X';
	if (flag & P_WEXIT &&
	    /* XXX - I don't like this */
	    (kp->p_stat == LSZOMB || kp->p_stat == LSDEAD) == 0)
		*cp++ = 'E';
	if (flag & P_PPWAIT)
		*cp++ = 'V';
	if ((flag & P_SYSTEM) || kp->p_holdcnt)
		*cp++ = 'L';
	if (kp->p_eflag & EPROC_SLEADER)
		*cp++ = 's'; 
	if ((flag & P_CONTROLT) && kp->p__pgid == kp->p_tpgid)
		*cp++ = '+';
	*cp = '\0';
	snprintf(statestr, sizeof(statestr), "%-s",  buf);

	return statestr;
}

char *
tty2str(struct kinfo_proc2 *kp)
{
	static char ttystr[4];
	char *ttyname;

	if (kp->p_tdev == NODEV ||
	    (ttyname = devname(kp->p_tdev, S_IFCHR)) == NULL)
		strlcpy(ttystr, "??", sizeof(ttystr));
	else {
		if (strncmp(ttyname, "tty", 3) == 0 ||
		    strncmp(ttyname, "dty", 3) == 0)
			ttyname += 3;
		snprintf(ttystr, sizeof(ttystr), "%s%c", ttyname,
		    kp->p_eflag & EPROC_CTTY ? ' ' : '-');
	}

	return ttystr;
}

#define pgtok(a)	(((a)*getpagesize())/1024)

int
vsz2int(struct kinfo_proc2 *kp)
{
	int     i;

	i = pgtok(kp->p_vm_dsize + kp->p_vm_ssize + kp->p_vm_tsize);

	return ((i < 0) ? 0 : i);
}

int
rss2int(struct kinfo_proc2 *kp)
{
	int	i;
 
	i = pgtok(kp->p_vm_rssize);

	/* XXX don't have info about shared */
	return ((i < 0) ? 0 : i);
}

char *
comm2str(struct kinfo_proc2 *kp)
{
	char **argv, **pt;
	static char commstr[41];

	commstr[0]='\0';

	argv = kvm_getargv2(kd, kp, 40);
	if ((pt = argv) != NULL) {
		while (*pt) {
			strcat(commstr, *pt);
			pt++;
			strcat(commstr, " ");
		}
	} else {
		commstr[0] = '(';
		commstr[1] = '\0';
		strncat(commstr, kp->p_comm, sizeof(commstr) - 1);
		strcat(commstr, ")");
	}

	return commstr;
}

double
pmem2float(struct kinfo_proc2 *kp)
{	                       
	double fracmem;
	int szptudot = 0;

	/* XXX - I don't like this. */
	if ((kp->p_flag & L_INMEM) == 0)
	        return (0.0);
#ifdef USPACE
	/* XXX want pmap ptpages, segtab, etc. (per architecture) */
	szptudot = USPACE/getpagesize();
#endif
	/* XXX don't have info about shared */
	fracmem = ((double)kp->p_vm_rssize + szptudot)/mempages;
	return (fracmem >= 0) ? 100.0 * fracmem : 0;
}

char *
start2str(struct kinfo_proc2 *kp)
{
	struct timeval u_start;
	struct tm *tp;
	time_t startt;
	static char startstr[10];

	u_start.tv_sec = kp->p_ustart_sec;
	u_start.tv_usec = kp->p_ustart_usec;

	startt = u_start.tv_sec;
	tp = localtime(&startt);
	if (now == 0)
	        time(&now);
	if (now - u_start.tv_sec < 24 * SECSPERHOUR) {
		/* I *hate* SCCS... */
	        static char fmt[] = "%l:%" "M%p";
	        strftime(startstr, sizeof(startstr) - 1, fmt, tp);
	} else if (now - u_start.tv_sec < 7 * SECSPERDAY) {
	        /* I *hate* SCCS... */
	        static char fmt[] = "%a%" "I%p";
	        strftime(startstr, sizeof(startstr) - 1, fmt, tp);
	} else  
	        strftime(startstr, sizeof(startstr) - 1, "%e%b%y", tp);

	return startstr;
}

char *    
time2str(struct kinfo_proc2 *kp)
{	       
	long secs;
	long psecs;     /* "parts" of a second. first micro, then centi */
	static char timestr[10];

	/* XXX - I don't like this. */
	if (kp->p_stat == SZOMB || kp->p_stat == SDEAD) {
	        secs = 0;
	        psecs = 0;
	} else {
	        /*
	         * This counts time spent handling interrupts.  We could
	         * fix this, but it is not 100% trivial (and interrupt
	         * time fractions only work on the sparc anyway).       XXX
	         */
	        secs = kp->p_rtime_sec;
	        psecs = kp->p_rtime_usec;
#if 0
	        if (sumrusage) {
	                secs += k->ki_u.u_cru.ru_utime.tv_sec +
	                        k->ki_u.u_cru.ru_stime.tv_sec;
	                psecs += k->ki_u.u_cru.ru_utime.tv_usec +
	                        k->ki_u.u_cru.ru_stime.tv_usec;
	        }
#endif
	        /*
	         * round and scale to 100's
	         */
	        psecs = (psecs + 5000) / 10000;
	        secs += psecs / 100;
	        psecs = psecs % 100;
	}
	snprintf(timestr, sizeof(timestr), "%3ld:%02ld.%02ld", secs/60,
	    secs%60, psecs);

	return timestr;
}

void
ps_user(char *args)
{
	uid_t uid;

	if (args == NULL)
		args = "";
	if (strcmp(args, "+") == 0) {
		uid = SHOWUSER_ANY;
	} else if (uid_from_user(args, &uid) != 0) {
		error("%s: unknown user", args);
		return;
	}

	showuser = uid;
	display(0);
}
