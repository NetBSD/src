/* $NetBSD: vmstat.c,v 1.83 2001/08/26 02:50:37 matt Exp $ */

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

/*
 * Copyright (c) 1980, 1986, 1991, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1986, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 3/1/95";
#else
__RCSID("$NetBSD: vmstat.c,v 1.83 2001/08/26 02:50:37 matt Exp $");
#endif
#endif /* not lint */

#define	__POOL_EXPOSE

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/dkstat.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_stat.h>

#include <err.h>
#include <fcntl.h>
#include <time.h>
#include <nlist.h>
#include <kvm.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <limits.h>
#include "dkstats.h"

struct nlist namelist[] = {
#define	X_BOOTTIME	0
	{ "_boottime" },
#define	X_HZ		1
	{ "_hz" },
#define	X_STATHZ	2
	{ "_stathz" },
#define	X_NCHSTATS	3
	{ "_nchstats" },
#define	X_INTRNAMES	4
	{ "_intrnames" },
#define	X_EINTRNAMES	5
	{ "_eintrnames" },
#define	X_INTRCNT	6
	{ "_intrcnt" },
#define	X_EINTRCNT	7
	{ "_eintrcnt" },
#define	X_KMEMSTAT	8
	{ "_kmemstats" },
#define	X_KMEMBUCKETS	9
	{ "_bucket" },
#define	X_ALLEVENTS	10
	{ "_allevents" },
#define	X_POOLHEAD	11
	{ "_pool_head" },
#define	X_UVMEXP	12
	{ "_uvmexp" },
#define	X_END		13
#if defined(pc532)
#define	X_IVT		(X_END)
	{ "_ivt" },
#endif
	{ "" },
};

struct	uvmexp uvmexp, ouvmexp;
int	ndrives;

int	winlines = 20;

kvm_t *kd;

#define	FORKSTAT	0x01
#define	INTRSTAT	0x02
#define	MEMSTAT		0x04
#define	SUMSTAT		0x08
#define	EVCNTSTAT	0x10
#define	VMSTAT		0x20
#define	HISTLIST	0x40
#define	HISTDUMP	0x80

void	cpustats(void);
void	dkstats(void);
void	doevcnt(int verbose);
void	dointr(int verbose);
void	domem(void);
void	dopool(void);
void	dosum(void);
void	dovmstat(u_int, int);
void	kread(int, void *, size_t);
void	needhdr(int);
long	getuptime(void);
void	printhdr(void);
long	pct(long, long);
void	usage(void);
void	doforkst(void);

void	hist_traverse(int, const char *);
void	hist_dodump(struct uvm_history *);

int	main(int, char **);
char	**choosedrives(char **);

/* Namelist and memory file names. */
char	*nlistf, *memf;

/* allow old usage [vmstat 1] */
#define	BACKWARD_COMPATIBILITY

int
main(int argc, char *argv[])
{
	int c, todo, verbose;
	u_int interval;
	int reps;
	char errbuf[_POSIX2_LINE_MAX];
	gid_t egid = getegid();
	const char *histname = NULL;

	(void)setegid(getgid());
	memf = nlistf = NULL;
	interval = reps = todo = verbose = 0;
	while ((c = getopt(argc, argv, "c:efh:HilM:mN:svw:")) != -1) {
		switch (c) {
		case 'c':
			reps = atoi(optarg);
			break;
		case 'e':
			todo |= EVCNTSTAT;
			break;
		case 'f':
			todo |= FORKSTAT;
			break;
		case 'h':
			histname = optarg;
			/* FALLTHROUGH */
		case 'H':
			todo |= HISTDUMP;
			break;
		case 'i':
			todo |= INTRSTAT;
			break;
		case 'l':
			todo |= HISTLIST;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			todo |= MEMSTAT;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 's':
			todo |= SUMSTAT;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (todo == 0)
		todo = VMSTAT;

	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());
	else
		(void)setegid(egid);

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);
	if (kd == NULL)
		errx(1, "kvm_openfiles: %s\n", errbuf);

	if (nlistf == NULL && memf == NULL) {
		if (todo & VMSTAT)
			(void)setegid(getgid());	/* XXX: dkinit */
		else
			(void)setgid(getgid());
	}

	if ((c = kvm_nlist(kd, namelist)) != 0) {
		if (c > 0) {
			(void)fprintf(stderr,
			    "vmstat: undefined symbols:");
			for (c = 0;
			    c < sizeof(namelist) / sizeof(namelist[0]); c++)
				if (namelist[c].n_type == 0)
					fprintf(stderr, " %s",
					    namelist[c].n_name);
			(void)fputc('\n', stderr);
		} else
			(void)fprintf(stderr, "vmstat: kvm_nlist: %s\n",
			    kvm_geterr(kd));
		exit(1);
	}

	if (todo & VMSTAT) {
		struct winsize winsize;

		dkinit(0, egid); /* Initialize disk stats, no disks selected. */

		(void)setgid(getgid()); /* don't need privs anymore */

		argv = choosedrives(argv);	/* Select disks. */
		winsize.ws_row = 0;
		(void)ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&winsize);
		if (winsize.ws_row > 0)
			winlines = winsize.ws_row;

	}

#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval) {
		if (!reps)
			reps = -1;
	} else if (reps)
		interval = 1;


	/*
	 * Statistics dumping is incompatible with the default
	 * VMSTAT/dovmstat() output. So perform the interval/reps handling
	 * for it here.
	 */
	if ((todo & VMSTAT) == 0)
	    for (;;) {
	    	if (todo & (HISTLIST|HISTDUMP)) {
	    		if ((todo & (HISTLIST|HISTDUMP)) ==
			    (HISTLIST|HISTDUMP))
	    			errx(1, "you may list or dump, but not both!");
	    		hist_traverse(todo, histname);
	    	}
	    	if (todo & FORKSTAT)
	    		doforkst();
	    	if (todo & MEMSTAT) {
	    		domem();
	    		dopool();
	    	}
	    	if (todo & SUMSTAT)
	    		dosum();
	    	if (todo & INTRSTAT)
	    		dointr(verbose);
	    	if (todo & EVCNTSTAT)
	    		doevcnt(verbose);
	    	
	    	if (reps >= 0 && --reps <=0) 
	    	    break;
	    	sleep(interval);
	    	puts("");
	    }
	else
		dovmstat(interval, reps);
	exit(0);
}

char **
choosedrives(char **argv)
{
	int i;

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments, default drives.  If everything isn't filled
	 * in and there are drives not taken care of, display the first few
	 * that fit.
	 */
#define	BACKWARD_COMPATIBILITY
	for (ndrives = 0; *argv; ++argv) {
#ifdef	BACKWARD_COMPATIBILITY
		if (isdigit(**argv))
			break;
#endif
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], *argv))
				continue;
			dk_select[i] = 1;
			++ndrives;
			break;
		}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dk_select[i])
			continue;
		dk_select[i] = 1;
		++ndrives;
	}
	return (argv);
}

long
getuptime(void)
{
	static time_t now;
	static struct timeval boottime;
	time_t uptime;

	if (boottime.tv_sec == 0)
		kread(X_BOOTTIME, &boottime, sizeof(boottime));
	(void)time(&now);
	uptime = now - boottime.tv_sec;
	if (uptime <= 0 || uptime > 60*60*24*365*10) {
		(void)fprintf(stderr,
		    "vmstat: time makes no sense; namelist must be wrong.\n");
		exit(1);
	}
	return (uptime);
}

int	hz, hdrcnt;

void
dovmstat(u_int interval, int reps)
{
	struct vmtotal total;
	time_t uptime, halfuptime;
	int mib[2];
	size_t size;
	int pagesize = getpagesize();

	uptime = getuptime();
	halfuptime = uptime / 2;
	(void)signal(SIGCONT, needhdr);

	if (namelist[X_STATHZ].n_type != 0 && namelist[X_STATHZ].n_value != 0)
		kread(X_STATHZ, &hz, sizeof(hz));
	if (!hz)
		kread(X_HZ, &hz, sizeof(hz));

	for (hdrcnt = 1;;) {
		if (!--hdrcnt)
			printhdr();
		/* Read new disk statistics */
		dkreadstats();
		kread(X_UVMEXP, &uvmexp, sizeof(uvmexp));
		if (memf != NULL) {
			/*
			 * XXX Can't do this if we're reading a crash
			 * XXX dump because they're lazily-calculated.
			 */
			printf("Unable to get vmtotals from crash dump.\n");
			memset(&total, 0, sizeof(total));
		} else {
			size = sizeof(total);
			mib[0] = CTL_VM;
			mib[1] = VM_METER;
			if (sysctl(mib, 2, &total, &size, NULL, 0) < 0) {
				printf("Can't get vmtotals: %s\n",
				    strerror(errno));
				memset(&total, 0, sizeof(total));
			}
		}
		(void)printf("%2d%2d%2d",
		    total.t_rq - 1, total.t_dw + total.t_pw, total.t_sw);
#define	pgtok(a) (long)((a) * (pagesize >> 10))
#define	rate(x)	(u_long)(((x) + halfuptime) / uptime)	/* round */
		(void)printf(" %5ld %5ld ",
		    pgtok(total.t_avm), pgtok(total.t_free));
		(void)printf("%4lu ", rate(uvmexp.faults - ouvmexp.faults));
		(void)printf("%3lu ", rate(uvmexp.pdreact - ouvmexp.pdreact));
		(void)printf("%3lu ", rate(uvmexp.pageins - ouvmexp.pageins));
		(void)printf("%4lu ",
		    rate(uvmexp.pgswapout - ouvmexp.pgswapout));
		(void)printf("%4lu ", rate(uvmexp.pdfreed - ouvmexp.pdfreed));
		(void)printf("%4lu ", rate(uvmexp.pdscans - ouvmexp.pdscans));
		dkstats();
		(void)printf("%4lu %4lu %3lu ",
		    rate(uvmexp.intrs - ouvmexp.intrs),
		    rate(uvmexp.syscalls - ouvmexp.syscalls),
		    rate(uvmexp.swtch - ouvmexp.swtch));
		cpustats();
		(void)printf("\n");
		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		ouvmexp = uvmexp;
		uptime = interval;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfuptime = uptime == 1 ? 0 : (uptime + 1) / 2;
		(void)sleep(interval);
	}
}

void
printhdr(void)
{
	int i;

	(void)printf(" procs   memory     page%*s", 23, "");
	if (ndrives > 0)
		(void)printf("%s %*sfaults      cpu\n",
		    ((ndrives > 1) ? "disks" : "disk"),
		    ((ndrives > 1) ? ndrives * 3 - 4 : 0), "");
	else
		(void)printf("%*s  faults   cpu\n",
		    ndrives * 3, "");

	(void)printf(" r b w   avm   fre  flt  re  pi   po   fr   sr ");
	for (i = 0; i < dk_ndrive; i++)
		if (dk_select[i])
			(void)printf("%c%c ", dr_name[i][0],
			    dr_name[i][strlen(dr_name[i]) - 1]);
	(void)printf("  in   sy  cs us sy id\n");
	hdrcnt = winlines - 2;
}

/*
 * Force a header to be prepended to the next output.
 */
void
needhdr(int dummy)
{

	hdrcnt = 1;
}

long
pct(long top, long bot)
{
	long ans;

	if (bot == 0)
		return (0);
	ans = (quad_t)top * 100 / bot;
	return (ans);
}

#define	PCT(top, bot) (int)pct((long)(top), (long)(bot))

void
dosum(void)
{
	struct nchstats nchstats;
	long nchtotal;

	kread(X_UVMEXP, &uvmexp, sizeof(uvmexp));

	(void)printf("%9u bytes per page\n", uvmexp.pagesize);

	(void)printf("%9u page color%s\n",
	    uvmexp.ncolors, uvmexp.ncolors == 1 ? "" : "s");

	(void)printf("%9u pages managed\n", uvmexp.npages);
	(void)printf("%9u pages free\n", uvmexp.free);
	(void)printf("%9u pages active\n", uvmexp.active);
	(void)printf("%9u pages inactive\n", uvmexp.inactive);
	(void)printf("%9u pages paging\n", uvmexp.paging);
	(void)printf("%9u pages wired\n", uvmexp.wired);
	(void)printf("%9u zero pages\n", uvmexp.zeropages);
	(void)printf("%9u reserve pagedaemon pages\n",
	    uvmexp.reserve_pagedaemon);
	(void)printf("%9u reserve kernel pages\n", uvmexp.reserve_kernel);
	(void)printf("%9u anon pager pages\n", uvmexp.anonpages);
	(void)printf("%9u vnode page cache pages\n", uvmexp.vnodepages);
	(void)printf("%9u executable pages\n", uvmexp.vtextpages);

	(void)printf("%9u minimum free pages\n", uvmexp.freemin);
	(void)printf("%9u target free pages\n", uvmexp.freetarg);
	(void)printf("%9u target inactive pages\n", uvmexp.inactarg);
	(void)printf("%9u maximum wired pages\n", uvmexp.wiredmax);

	(void)printf("%9u swap devices\n", uvmexp.nswapdev);
	(void)printf("%9u swap pages\n", uvmexp.swpages);
	(void)printf("%9u swap pages in use\n", uvmexp.swpginuse);
	(void)printf("%9u swap allocations\n", uvmexp.nswget);
	(void)printf("%9u anons\n", uvmexp.nanon);
	(void)printf("%9u free anons\n", uvmexp.nfreeanon);

	(void)printf("%9u total faults taken\n", uvmexp.faults);
	(void)printf("%9u traps\n", uvmexp.traps);
	(void)printf("%9u device interrupts\n", uvmexp.intrs);
	(void)printf("%9u cpu context switches\n", uvmexp.swtch);
	(void)printf("%9u software interrupts\n", uvmexp.softs);
	(void)printf("%9u system calls\n", uvmexp.syscalls);
	(void)printf("%9u pagein requests\n", uvmexp.pageins);
	(void)printf("%9u pageout requests\n", uvmexp.pdpageouts);
	(void)printf("%9u swap ins\n", uvmexp.swapins);
	(void)printf("%9u swap outs\n", uvmexp.swapouts);
	(void)printf("%9u pages swapped in\n", uvmexp.pgswapin);
	(void)printf("%9u pages swapped out\n", uvmexp.pgswapout);
	(void)printf("%9u forks total\n", uvmexp.forks);
	(void)printf("%9u forks blocked parent\n", uvmexp.forks_ppwait);
	(void)printf("%9u forks shared address space with parent\n",
	    uvmexp.forks_sharevm);
	(void)printf("%9u pagealloc zero wanted and avail\n",
	    uvmexp.pga_zerohit);
	(void)printf("%9u pagealloc zero wanted and not avail\n",
	    uvmexp.pga_zeromiss);
	(void)printf("%9u aborts of idle page zeroing\n",
	    uvmexp.zeroaborts);
	(void)printf("%9u pagealloc desired color avail\n",
	    uvmexp.colorhit);
	(void)printf("%9u pagealloc desired color not avail\n",
	    uvmexp.colormiss);

	(void)printf("%9u faults with no memory\n", uvmexp.fltnoram);
	(void)printf("%9u faults with no anons\n", uvmexp.fltnoanon);
	(void)printf("%9u faults had to wait on pages\n", uvmexp.fltpgwait);
	(void)printf("%9u faults found released page\n", uvmexp.fltpgrele);
	(void)printf("%9u faults relock (%u ok)\n", uvmexp.fltrelck,
	    uvmexp.fltrelckok);
	(void)printf("%9u anon page faults\n", uvmexp.fltanget);
	(void)printf("%9u anon retry faults\n", uvmexp.fltanretry);
	(void)printf("%9u amap copy faults\n", uvmexp.fltamcopy);
	(void)printf("%9u neighbour anon page faults\n", uvmexp.fltnamap);
	(void)printf("%9u neighbour object page faults\n", uvmexp.fltnomap);
	(void)printf("%9u locked pager get faults\n", uvmexp.fltlget);
	(void)printf("%9u unlocked pager get faults\n", uvmexp.fltget);
	(void)printf("%9u anon faults\n", uvmexp.flt_anon);
	(void)printf("%9u anon copy on write faults\n", uvmexp.flt_acow);
	(void)printf("%9u object faults\n", uvmexp.flt_obj);
	(void)printf("%9u promote copy faults\n", uvmexp.flt_prcopy);
	(void)printf("%9u promote zero fill faults\n", uvmexp.flt_przero);

	(void)printf("%9u times daemon wokeup\n",uvmexp.pdwoke);
	(void)printf("%9u revolutions of the clock hand\n", uvmexp.pdrevs);
	(void)printf("%9u times daemon attempted swapout\n", uvmexp.pdswout);
	(void)printf("%9u pages freed by daemon\n", uvmexp.pdfreed);
	(void)printf("%9u pages scanned by daemon\n", uvmexp.pdscans);
	(void)printf("%9u anonymous pages scanned by daemon\n",
	    uvmexp.pdanscan);
	(void)printf("%9u object pages scanned by daemon\n", uvmexp.pdobscan);
	(void)printf("%9u pages reactivated\n", uvmexp.pdreact);
	(void)printf("%9u pages found busy by daemon\n", uvmexp.pdbusy);
	(void)printf("%9u total pending pageouts\n", uvmexp.pdpending);
	(void)printf("%9u pages deactivated\n", uvmexp.pddeact);
	kread(X_NCHSTATS, &nchstats, sizeof(nchstats));
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_neghits +
	    nchstats.ncs_badhits + nchstats.ncs_falsehits +
	    nchstats.ncs_miss + nchstats.ncs_long;
	(void)printf("%9ld total name lookups\n", nchtotal);
	(void)printf(
	    "%9s cache hits (%d%% pos + %d%% neg) system %d%% per-process\n",
	    "", PCT(nchstats.ncs_goodhits, nchtotal),
	    PCT(nchstats.ncs_neghits, nchtotal),
	    PCT(nchstats.ncs_pass2, nchtotal));
	(void)printf("%9s deletions %d%%, falsehits %d%%, toolong %d%%\n", "",
	    PCT(nchstats.ncs_badhits, nchtotal),
	    PCT(nchstats.ncs_falsehits, nchtotal),
	    PCT(nchstats.ncs_long, nchtotal));
}

void
doforkst(void)
{

	kread(X_UVMEXP, &uvmexp, sizeof(uvmexp));

	(void)printf("%u forks total\n", uvmexp.forks);
	(void)printf("%u forks blocked parent\n", uvmexp.forks_ppwait);
	(void)printf("%u forks shared address space with parent\n",
	    uvmexp.forks_sharevm);
}

void
dkstats(void)
{
	int dn, state;
	double etime;

	/* Calculate disk stat deltas. */
	dkswap();
	etime = 0;
	for (state = 0; state < CPUSTATES; ++state) {
		etime += cur.cp_time[state];
	}
	if (etime == 0)
		etime = 1;
	etime /= hz;
	for (dn = 0; dn < dk_ndrive; ++dn) {
		if (!dk_select[dn])
			continue;
		(void)printf("%2.0f ", cur.dk_xfer[dn] / etime);
	}
}

void
cpustats(void)
{
	int state;
	double pct, total;

	total = 0;
	for (state = 0; state < CPUSTATES; ++state)
		total += cur.cp_time[state];
	if (total)
		pct = 100 / total;
	else
		pct = 0;
	(void)printf("%2.0f ",
	    (cur.cp_time[CP_USER] + cur.cp_time[CP_NICE]) * pct);
	(void)printf("%2.0f ",
	    (cur.cp_time[CP_SYS] + cur.cp_time[CP_INTR]) * pct);
	(void)printf("%2.0f", cur.cp_time[CP_IDLE] * pct);
}

#if defined(pc532)
/* To get struct iv ...*/
#define	_KERNEL
#include <machine/psl.h>
#undef _KERNEL
void
dointr(int verbose)
{
	long i, j, inttotal, uptime;
	static char iname[64];
	struct iv ivt[32], *ivp = ivt;

	iname[63] = '\0';
	uptime = getuptime();
	kread(X_IVT, ivp, sizeof(ivt));

	for (i = 0; i < 2; i++) {
		(void)printf("%sware interrupts:\n", i ? "\nsoft" : "hard");
		(void)printf("interrupt       total     rate\n");
		inttotal = 0;
		for (j = 0; j < 16; j++, ivp++) {
			if (ivp->iv_vec && ivp->iv_use &&
			    (ivp->iv_cnt || verbose)) {
				if (kvm_read(kd, (u_long)ivp->iv_use, iname,
				    63) != 63) {
					(void)fprintf(stderr,
					    "vmstat: iv_use: %s\n",
					    kvm_geterr(kd));
					exit(1);
				}
				(void)printf("%-12s %8ld %8ld\n", iname,
				    ivp->iv_cnt, ivp->iv_cnt / uptime);
				inttotal += ivp->iv_cnt;
			}
		}
		(void)printf("Total        %8ld %8ld\n",
		    inttotal, inttotal / uptime);
	}
}
#else
void
dointr(int verbose)
{
	long *intrcnt;
	long long inttotal, uptime;
	int nintr, inamlen;
	char *intrname;
	struct evcntlist allevents;
	struct evcnt evcnt, *evptr;
	char evgroup[EVCNT_STRING_MAX], evname[EVCNT_STRING_MAX];

	uptime = getuptime();
	nintr = namelist[X_EINTRCNT].n_value - namelist[X_INTRCNT].n_value;
	inamlen =
	    namelist[X_EINTRNAMES].n_value - namelist[X_INTRNAMES].n_value;
	intrcnt = malloc((size_t)nintr);
	intrname = malloc((size_t)inamlen);
	if (intrcnt == NULL || intrname == NULL) {
		(void)fprintf(stderr, "vmstat: %s.\n", strerror(errno));
		exit(1);
	}
	kread(X_INTRCNT, intrcnt, (size_t)nintr);
	kread(X_INTRNAMES, intrname, (size_t)inamlen);
	(void)printf("%-34s %16s %8s\n", "interrupt", "total", "rate");
	inttotal = 0;
	nintr /= sizeof(long);
	while (--nintr >= 0) {
		if (*intrcnt || verbose)
			(void)printf("%-34s %16lld %8lld\n", intrname,
			    (long long)*intrcnt,
			    (long long)(*intrcnt / uptime));
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	kread(X_ALLEVENTS, &allevents, sizeof allevents);
	evptr = allevents.tqh_first;
	while (evptr) {
		if (kvm_read(kd, (long)evptr, &evcnt, sizeof evcnt) !=
		    sizeof(evcnt)) {
event_chain_trashed:
			(void)fprintf(stderr,
			    "vmstat: event chain trashed: %s\n",
			    kvm_geterr(kd));
			exit(1);
		}

		evptr = evcnt.ev_list.tqe_next;
		if (evcnt.ev_type != EVCNT_TYPE_INTR)
			continue;

		if (evcnt.ev_count == 0 && !verbose)
			continue;

		if (kvm_read(kd, (long)evcnt.ev_group, evgroup,
		    evcnt.ev_grouplen + 1) != evcnt.ev_grouplen + 1)
			goto event_chain_trashed;
		if (kvm_read(kd, (long)evcnt.ev_name, evname,
		    evcnt.ev_namelen + 1) != evcnt.ev_namelen + 1)
			goto event_chain_trashed;

		(void)printf("%s %s%*s %16lld %8lld\n", evgroup, evname,
		    34 - (evcnt.ev_grouplen + 1 + evcnt.ev_namelen), "",
		    (long long)evcnt.ev_count,
		    (long long)(evcnt.ev_count / uptime));

		inttotal += evcnt.ev_count++;
	}
	(void)printf("%-34s %16lld %8lld\n", "Total", inttotal,
	    (long long)(inttotal / uptime));
}
#endif

void
doevcnt(int verbose)
{
	static const char * evtypes [] = { "misc", "intr", "trap" };
	long long uptime;
	struct evcntlist allevents;
	struct evcnt evcnt, *evptr;
	char evgroup[EVCNT_STRING_MAX], evname[EVCNT_STRING_MAX];

	/* XXX should print type! */

	uptime = getuptime();
	(void)printf("%-34s %16s %8s %s\n", "event", "total", "rate", "type");
	kread(X_ALLEVENTS, &allevents, sizeof allevents);
	evptr = allevents.tqh_first;
	while (evptr) {
		if (kvm_read(kd, (long)evptr, &evcnt, sizeof evcnt) !=
		    sizeof(evcnt)) {
event_chain_trashed:
			(void)fprintf(stderr,
			    "vmstat: event chain trashed: %s\n",
			    kvm_geterr(kd));
			exit(1);
		}

		evptr = evcnt.ev_list.tqe_next;
		if (evcnt.ev_count == 0 && !verbose)
			continue;

		if (kvm_read(kd, (long)evcnt.ev_group, evgroup,
		    evcnt.ev_grouplen + 1) != evcnt.ev_grouplen + 1)
			goto event_chain_trashed;
		if (kvm_read(kd, (long)evcnt.ev_name, evname,
		    evcnt.ev_namelen + 1) != evcnt.ev_namelen + 1)
			goto event_chain_trashed;

		(void)printf("%s %s%*s %16lld %8lld %s\n", evgroup, evname,
		    34 - (evcnt.ev_grouplen + 1 + evcnt.ev_namelen), "",
		    (long long)evcnt.ev_count,
		    (long long)(evcnt.ev_count / uptime),
		    (evcnt.ev_type < sizeof(evtypes)/sizeof(evtypes)
			? evtypes[evcnt.ev_type] : "?"));
	}
}

/*
 * These names are defined in <sys/malloc.h>.
 */
char *kmemnames[] = INITKMEMNAMES;

void
domem(void)
{
	struct kmembuckets *kp;
	struct kmemstats *ks;
	int i, j;
	int len, size, first;
	long totuse = 0, totfree = 0, totreq = 0;
	char *name;
	struct kmemstats kmemstats[M_LAST];
	struct kmembuckets buckets[MINBUCKET + 16];

	kread(X_KMEMBUCKETS, buckets, sizeof(buckets));
	for (first = 1, i = MINBUCKET, kp = &buckets[i]; i < MINBUCKET + 16;
	    i++, kp++) {
		if (kp->kb_calls == 0)
			continue;
		if (first) {
			(void)printf("Memory statistics by bucket size\n");
			(void)printf(
		 "    Size   In Use   Free   Requests  HighWater  Couldfree\n");
			first = 0;
		}
		size = 1 << i;
		(void)printf("%8d %8ld %6ld %10ld %7ld %10ld\n", size,
		    kp->kb_total - kp->kb_totalfree,
		    kp->kb_totalfree, kp->kb_calls,
		    kp->kb_highwat, kp->kb_couldfree);
		totfree += size * kp->kb_totalfree;
	}

	/*
	 * If kmem statistics are not being gathered by the kernel,
	 * first will still be 1.
	 */
	if (first) {
		printf(
		    "Kmem statistics are not being gathered by the kernel.\n");
		return;
	}

	kread(X_KMEMSTAT, kmemstats, sizeof(kmemstats));
	(void)printf("\nMemory usage type by bucket size\n");
	(void)printf("    Size  Type(s)\n");
	kp = &buckets[MINBUCKET];
	for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1, kp++) {
		if (kp->kb_calls == 0)
			continue;
		first = 1;
		len = 8;
		for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
			if (ks->ks_calls == 0)
				continue;
			if ((ks->ks_size & j) == 0)
				continue;
			if (kmemnames[i] == 0) {
				kmemnames[i] = malloc(10);
						/* strlen("undef/")+3+1);*/
				snprintf(kmemnames[i], 10, "undef/%d", i);
						/* same 10 as above!!! */
			}
			name = kmemnames[i];
			len += 2 + strlen(name);
			if (first)
				printf("%8d  %s", j, name);
			else
				printf(",");
			if (len >= 80) {
				printf("\n\t ");
				len = 10 + strlen(name);
			}
			if (!first)
				printf(" %s", name);
			first = 0;
		}
		printf("\n");
	}

	(void)printf(
	    "\nMemory statistics by type                        Type  Kern\n");
	(void)printf(
"         Type  InUse MemUse HighUse  Limit Requests Limit Limit Size(s)\n");
	for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
		if (ks->ks_calls == 0)
			continue;
		(void)printf("%14s%6ld%6ldK%7ldK%6ldK%9ld%5u%6u",
		    kmemnames[i] ? kmemnames[i] : "undefined",
		    ks->ks_inuse, (ks->ks_memuse + 1023) / 1024,
		    (ks->ks_maxused + 1023) / 1024,
		    (ks->ks_limit + 1023) / 1024, ks->ks_calls,
		    ks->ks_limblocks, ks->ks_mapblocks);
		first = 1;
		for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1) {
			if ((ks->ks_size & j) == 0)
				continue;
			if (first)
				printf("  %d", j);
			else
				printf(",%d", j);
			first = 0;
		}
		printf("\n");
		totuse += ks->ks_memuse;
		totreq += ks->ks_calls;
	}
	(void)printf("\nMemory Totals:  In Use    Free    Requests\n");
	(void)printf("              %7ldK %6ldK    %8ld\n",
	    (totuse + 1023) / 1024, (totfree + 1023) / 1024, totreq);
}

void
dopool(void)
{
	int first, ovflw;
	long addr;
	long total = 0, inuse = 0;
	TAILQ_HEAD(,pool) pool_head;
	struct pool pool, *pp = &pool;

	kread(X_POOLHEAD, &pool_head, sizeof(pool_head));
	addr = (long)TAILQ_FIRST(&pool_head);

	for (first = 1; addr != 0; ) {
		char name[32], maxp[32];
		if (kvm_read(kd, addr, (void *)pp, sizeof *pp) != sizeof *pp) {
			(void)fprintf(stderr,
			    "vmstat: pool chain trashed: %s\n",
			    kvm_geterr(kd));
			exit(1);
		}
		if (kvm_read(kd, (long)pp->pr_wchan, name, sizeof name) < 0) {
			(void)fprintf(stderr,
			    "vmstat: pool name trashed: %s\n",
			    kvm_geterr(kd));
			exit(1);
		}
		name[31] = '\0';

		if (first) {
			(void)printf("Memory resource pool statistics\n");
			(void)printf(
			    "%-11s%5s%9s%5s%9s%6s%6s%6s%6s%6s%6s%5s\n",
			    "Name",
			    "Size",
			    "Requests",
			    "Fail",
			    "Releases",
			    "Pgreq",
			    "Pgrel",
			    "Npage",
			    "Hiwat",
			    "Minpg",
			    "Maxpg",
			    "Idle");
			first = 0;
		}
		if (pp->pr_maxpages == UINT_MAX)
			sprintf(maxp, "inf");
		else
			sprintf(maxp, "%u", pp->pr_maxpages);
/*
 * Print single word.  `ovflow' is number of characters didn't fit
 * on the last word.  `fmt' is a format string to print this word.
 * It must contain asterisk for field width.  `width' is a width
 * occupied by this word.  `fixed' is a number of constant chars in
 * `fmt'.  `val' is a value to be printed using format string `fmt'.
 */
#define	PRWORD(ovflw, fmt, width, fixed, val) do {	\
	(ovflw) += printf((fmt),			\
	    (width) - (fixed) - (ovflw) > 0 ?		\
	    (width) - (fixed) - (ovflw) : 0,		\
	    (val)) - (width);				\
	if ((ovflw) < 0)				\
		(ovflw) = 0;				\
} while (/* CONSTCOND */0)
		ovflw = 0;
		PRWORD(ovflw, "%-*s", 11, 0, name);
		PRWORD(ovflw, " %*u", 5, 1, pp->pr_size);
		PRWORD(ovflw, " %*lu", 9, 1, pp->pr_nget);
		PRWORD(ovflw, " %*lu", 5, 1, pp->pr_nfail);
		PRWORD(ovflw, " %*lu", 9, 1, pp->pr_nput);
		PRWORD(ovflw, " %*lu", 6, 1, pp->pr_npagealloc);
		PRWORD(ovflw, " %*lu", 6, 1, pp->pr_npagefree);
		PRWORD(ovflw, " %*d", 6, 1, pp->pr_npages);
		PRWORD(ovflw, " %*d", 6, 1, pp->pr_hiwat);
		PRWORD(ovflw, " %*d", 6, 1, pp->pr_minpages);
		PRWORD(ovflw, " %*s", 6, 1, maxp);
		PRWORD(ovflw, " %*lu\n", 5, 1, pp->pr_nidle);

		inuse += (pp->pr_nget - pp->pr_nput) * pp->pr_size;
		total += pp->pr_npages * pp->pr_pagesz;
		addr = (long)TAILQ_NEXT(pp, pr_poollist);
	}

	inuse /= 1024;
	total /= 1024;
	printf("\nIn use %ldK, total allocated %ldK; utilization %.1f%%\n",
	    inuse, total, (double)(100 * inuse) / total);
}

/*
 * kread reads something from the kernel, given its nlist index.
 */
void
kread(int nlx, void *addr, size_t size)
{
	const char *sym;

	if (namelist[nlx].n_type == 0 || namelist[nlx].n_value == 0) {
		sym = namelist[nlx].n_name;
		if (*sym == '_')
			++sym;
		(void)fprintf(stderr,
		    "vmstat: symbol %s not defined\n", sym);
		exit(1);
	}
	if (kvm_read(kd, namelist[nlx].n_value, addr, size) != size) {
		sym = namelist[nlx].n_name;
		if (*sym == '_')
			++sym;
		(void)fprintf(stderr, "vmstat: %s: %s\n", sym, kvm_geterr(kd));
		exit(1);
	}
}

struct nlist histnl[] = {
	{ "_uvm_histories" },
#define	X_UVM_HISTORIES		0
	{ NULL },
};

/*
 * Traverse the UVM history buffers, performing the requested action.
 *
 * Note, we assume that if we're not listing, we're dumping.
 */
void
hist_traverse(int todo, const char *histname)
{
	struct uvm_history_head histhead;
	struct uvm_history hist, *histkva;
	char *name = NULL;
	size_t namelen = 0;

	if (kvm_nlist(kd, histnl) != 0) {
		printf("UVM history is not compiled into the kernel.\n");
		return;
	}

	if (kvm_read(kd, histnl[X_UVM_HISTORIES].n_value, &histhead,
	    sizeof(histhead)) != sizeof(histhead)) {
		warnx("unable to read %s: %s",
		    histnl[X_UVM_HISTORIES].n_name, kvm_geterr(kd));
		return;
	}

	if (histhead.lh_first == NULL) {
		printf("No active UVM history logs.\n");
		return;
	}

	if (todo & HISTLIST)
		printf("Active UVM histories:");

	for (histkva = LIST_FIRST(&histhead); histkva != NULL;
	    histkva = LIST_NEXT(&hist, list)) {
		if (kvm_read(kd, (u_long)histkva, &hist, sizeof(hist)) !=
		    sizeof(hist)) {
			warnx("unable to read history at %p: %s",
			    histkva, kvm_geterr(kd));
			goto out;
		}

		if (hist.namelen > namelen) {
			if (name != NULL)
				free(name);
			namelen = hist.namelen;
			if ((name = malloc(namelen + 1)) == NULL)
				err(1, "malloc history name");
		}

		if (kvm_read(kd, (u_long)hist.name, name, namelen) !=
		    namelen) {
			warnx("unable to read history name at %p: %s",
			    hist.name, kvm_geterr(kd));
			goto out;
		}
		name[namelen] = '\0';
		if (todo & HISTLIST)
			printf(" %s", name);
		else {
			/*
			 * If we're dumping all histories, do it, else
			 * check to see if this is the one we want.
			 */
			if (histname == NULL || strcmp(histname, name) == 0) {
				if (histname == NULL)
					printf("\nUVM history `%s':\n", name);
				hist_dodump(&hist);
			}
		}
	}

	if (todo & HISTLIST)
		printf("\n");

 out:
	if (name != NULL)
		free(name);
}

/*
 * Actually dump the history buffer at the specified KVA.
 */
void
hist_dodump(struct uvm_history *histp)
{
	struct uvm_history_ent *histents, *e;
	size_t histsize;
	char *fmt = NULL, *fn = NULL;
	size_t fmtlen = 0, fnlen = 0;
	int i;

	histsize = sizeof(struct uvm_history_ent) * histp->n;

	if ((histents = malloc(histsize)) == NULL)
		err(1, "malloc history entries");

	memset(histents, 0, histsize);

	if (kvm_read(kd, (u_long)histp->e, histents, histsize) != histsize) {
		warnx("unable to read history entries at %p: %s",
		    histp->e, kvm_geterr(kd));
		goto out;
	}

	i = histp->f;
	do {
		e = &histents[i];
		if (e->fmt != NULL) {
			if (e->fmtlen > fmtlen) {
				if (fmt != NULL)
					free(fmt);
				fmtlen = e->fmtlen;
				if ((fmt = malloc(fmtlen + 1)) == NULL)
					err(1, "malloc printf format");
			}
			if (e->fnlen > fnlen) {
				if (fn != NULL)
					free(fn);
				fnlen = e->fnlen;
				if ((fn = malloc(fnlen + 1)) == NULL)
					err(1, "malloc function name");
			}

			if (kvm_read(kd, (u_long)e->fmt, fmt, fmtlen) !=
			    fmtlen) {
				warnx("unable to read printf format "
				    "at %p: %s", e->fmt, kvm_geterr(kd));
				goto out;
			}
			fmt[fmtlen] = '\0';

			if (kvm_read(kd, (u_long)e->fn, fn, fnlen) != fnlen) {
				warnx("unable to read function name "
				    "at %p: %s", e->fn, kvm_geterr(kd));
				goto out;
			}
			fn[fnlen] = '\0';

			printf("%06ld.%06ld ", (long int)e->tv.tv_sec,
			    (long int)e->tv.tv_usec);
			printf("%s#%ld: ", fn, e->call);
			printf(fmt, e->v[0], e->v[1], e->v[2], e->v[3]);
			printf("\n");
		}
		i = (i + 1) % histp->n;
	} while (i != histp->f);

 out:
	free(histents);
	if (fmt != NULL)
		free(fmt);
	if (fn != NULL)
		free(fn);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: vmstat [-efHilmsv] [-h histname] [-c count] [-M core] "
	    "[-N system] [-w wait] [disks]\n");
	exit(1);
}
