/* $NetBSD: vmstat.c,v 1.117 2003/08/07 11:17:11 agc Exp $ */

/*-
 * Copyright (c) 1998, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation by:
 *	- Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 *	  NASA Ames Research Center.
 *	- Simon Burge and Luke Mewburn of Wasabi Systems, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1986, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 3/1/95";
#else
__RCSID("$NetBSD: vmstat.c,v 1.117 2003/08/07 11:17:11 agc Exp $");
#endif
#endif /* not lint */

#define	__POOL_EXPOSE

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/uio.h>

#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mallocvar.h>
#include <sys/namei.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_stat.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <ufs/ufs/inode.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsnode.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#undef n_hash
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include "dkstats.h"

/*
 * General namelist
 */
struct nlist namelist[] =
{
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
	{ "_kmemstatistics" },
#define	X_KMEMBUCKETS	9
	{ "_bucket" },
#define	X_ALLEVENTS	10
	{ "_allevents" },
#define	X_POOLHEAD	11
	{ "_pool_head" },
#define	X_UVMEXP	12
	{ "_uvmexp" },
#define	X_TIME		13
	{ "_time" },
#define	X_END		14
	{ NULL },
};

/*
 * Namelist for hash statistics
 */
struct nlist hashnl[] =
{
#define	X_NFSNODE	0
	{ "_nfsnodehash" },
#define	X_NFSNODETBL	1
	{ "_nfsnodehashtbl" },
#define	X_IHASH		2
	{ "_ihash" },
#define	X_IHASHTBL	3
	{ "_ihashtbl" },
#define	X_BUFHASH	4
	{ "_bufhash" },
#define	X_BUFHASHTBL	5
	{ "_bufhashtbl" },
#define	X_PIDHASH	6
	{ "_pidhash" },
#define	X_PIDHASHTBL	7
	{ "_pidhashtbl" },
#define	X_PGRPHASH	8
	{ "_pgrphash" },
#define	X_PGRPHASHTBL	9
	{ "_pgrphashtbl" },
#define	X_UIHASH	10
	{ "_uihash" },
#define	X_UIHASHTBL	11
	{ "_uihashtbl" },
#define	X_IFADDRHASH	12
	{ "_in_ifaddrhash" },
#define	X_IFADDRHASHTBL	13
	{ "_in_ifaddrhashtbl" },
#define	X_NCHASH	14
	{ "_nchash" },
#define	X_NCHASHTBL	15
	{ "_nchashtbl" },
#define	X_NCVHASH	16
	{ "_ncvhash" },
#define	X_NCVHASHTBL	17
	{ "_ncvhashtbl" },
#define X_HASHNL_SIZE	18	/* must be last */
	{ NULL },

};

/*
 * Namelist for UVM histories
 */
struct nlist histnl[] =
{
	{ "_uvm_histories" },
#define	X_UVM_HISTORIES		0
	{ NULL },
};



struct	uvmexp uvmexp, ouvmexp;
int	ndrives;

int	winlines = 20;

kvm_t *kd;

#define	FORKSTAT	1<<0
#define	INTRSTAT	1<<1
#define	MEMSTAT		1<<2
#define	SUMSTAT		1<<3
#define	EVCNTSTAT	1<<4
#define	VMSTAT		1<<5
#define	HISTLIST	1<<6
#define	HISTDUMP	1<<7
#define	HASHSTAT	1<<8
#define	HASHLIST	1<<9

void	cpustats(void);
void	deref_kptr(const void *, void *, size_t, const char *);
void	dkstats(void);
void	doevcnt(int verbose);
void	dohashstat(int, int, const char *);
void	dointr(int verbose);
void	domem(void);
void	dopool(int);
void	dopoolcache(struct pool *, int);
void	dosum(void);
void	dovmstat(struct timespec *, int);
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
	struct timespec interval;
	int reps;
	char errbuf[_POSIX2_LINE_MAX];
	gid_t egid = getegid();
	const char *histname, *hashname;

	histname = hashname = NULL;
	(void)setegid(getgid());
	memf = nlistf = NULL;
	reps = todo = verbose = 0;
	interval.tv_sec = 0;
	interval.tv_nsec = 0;
	while ((c = getopt(argc, argv, "c:efh:HilLM:mN:su:Uvw:")) != -1) {
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
			hashname = optarg;
			/* FALLTHROUGH */
		case 'H':
			todo |= HASHSTAT;
			break;
		case 'i':
			todo |= INTRSTAT;
			break;
		case 'l':
			todo |= HISTLIST;
			break;
		case 'L':
			todo |= HASHLIST;
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
		case 'u':
			histname = optarg;
			/* FALLTHROUGH */
		case 'U':
			todo |= HISTDUMP;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			interval.tv_sec = atol(optarg);
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
		errx(1, "kvm_openfiles: %s", errbuf);

	if (nlistf == NULL && memf == NULL)
		(void)setgid(getgid());

	if ((c = kvm_nlist(kd, namelist)) != 0) {
		if (c == -1)
			errx(1, "kvm_nlist: %s %s", "namelist", kvm_geterr(kd));
		(void)fprintf(stderr, "vmstat: undefined symbols:");
		for (c = 0; c < sizeof(namelist) / sizeof(namelist[0])-1; c++)
			if (namelist[c].n_type == 0)
				fprintf(stderr, " %s", namelist[c].n_name);
		(void)fputc('\n', stderr);
		exit(1);
	}
	if ((c = kvm_nlist(kd, hashnl)) == -1 || c == X_HASHNL_SIZE)
		errx(1, "kvm_nlist: %s %s", "hashnl", kvm_geterr(kd));
	if (kvm_nlist(kd, histnl) == -1)
		errx(1, "kvm_nlist: %s %s", "histnl", kvm_geterr(kd));

	if (todo & VMSTAT) {
		struct winsize winsize;

		dkinit(0);	/* Initialize disk stats, no disks selected. */

		(void)setgid(getgid()); /* don't need privs anymore */

		argv = choosedrives(argv);	/* Select disks. */
		winsize.ws_row = 0;
		(void)ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
		if (winsize.ws_row > 0)
			winlines = winsize.ws_row;

	}

#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval.tv_sec = atol(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval.tv_sec) {
		if (!reps)
			reps = -1;
	} else if (reps)
		interval.tv_sec = 1;


	/*
	 * Statistics dumping is incompatible with the default
	 * VMSTAT/dovmstat() output. So perform the interval/reps handling
	 * for it here.
	 */
	if ((todo & VMSTAT) == 0) {
		for (;;) {
			if (todo & (HISTLIST|HISTDUMP)) {
				if ((todo & (HISTLIST|HISTDUMP)) ==
				    (HISTLIST|HISTDUMP))
					errx(1, "you may list or dump,"
					    " but not both!");
				hist_traverse(todo, histname);
				putchar('\n');
			}
			if (todo & FORKSTAT) {
				doforkst();
				putchar('\n');
			}
			if (todo & MEMSTAT) {
				domem();
				dopool(verbose);
				putchar('\n');
			}
			if (todo & SUMSTAT) {
				dosum();
				putchar('\n');
			}
			if (todo & INTRSTAT) {
				dointr(verbose);
				putchar('\n');
			}
			if (todo & EVCNTSTAT) {
				doevcnt(verbose);
				putchar('\n');
			}
			if (todo & (HASHLIST|HASHSTAT)) {
				if ((todo & (HASHLIST|HASHSTAT)) ==
				    (HASHLIST|HASHSTAT))
					errx(1, "you may list or display,"
					    " but not both!");
				dohashstat(verbose, todo, hashname);
				putchar('\n');
			}

			if (reps >= 0 && --reps <=0)
				break;
			nanosleep(&interval, NULL);
		}
	} else
		dovmstat(&interval, reps);
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
	static struct timeval boottime;
	struct timeval now, diff;
	time_t uptime;

	if (boottime.tv_sec == 0)
		kread(X_BOOTTIME, &boottime, sizeof(boottime));
	kread(X_TIME, &now, sizeof(now));
	timersub(&now, &boottime, &diff);
	uptime = diff.tv_sec;
	if (uptime <= 0 || uptime > 60*60*24*365*10)
		errx(1, "time makes no sense; namelist must be wrong.");
	return (uptime);
}

int	hz, hdrcnt;

void
dovmstat(struct timespec *interval, int reps)
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
		(void)printf(" %6ld %6ld ",
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
		putchar('\n');
		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		ouvmexp = uvmexp;
		uptime = interval->tv_sec;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfuptime = uptime == 1 ? 0 : (uptime + 1) / 2;
		nanosleep(interval, NULL);
	}
}

void
printhdr(void)
{
	int i;

	(void)printf(" procs    memory      page%*s", 23, "");
	if (ndrives > 0)
		(void)printf("%s %*sfaults      cpu\n",
		    ((ndrives > 1) ? "disks" : "disk"),
		    ((ndrives > 1) ? ndrives * 3 - 4 : 0), "");
	else
		(void)printf("%*s  faults   cpu\n",
		    ndrives * 3, "");

	(void)printf(" r b w    avm    fre  flt  re  pi   po   fr   sr ");
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
	(void)printf("%9u anonymous pages\n", uvmexp.anonpages);
	(void)printf("%9u cached file pages\n", uvmexp.filepages);
	(void)printf("%9u cached executable pages\n", uvmexp.execpages);

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
	int dn;
	double etime;

	/* Calculate disk stat deltas. */
	dkswap();
	etime = cur.cp_etime;

	for (dn = 0; dn < dk_ndrive; ++dn) {
		if (!dk_select[dn])
			continue;
		(void)printf("%2.0f ",
		    (cur.dk_rxfer[dn] + cur.dk_wxfer[dn]) / etime);
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

void
dointr(int verbose)
{
	unsigned long *intrcnt;
	unsigned long long inttotal, uptime;
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
	if (intrcnt == NULL || intrname == NULL)
		errx(1, "%s", "");
	kread(X_INTRCNT, intrcnt, (size_t)nintr);
	kread(X_INTRNAMES, intrname, (size_t)inamlen);
	(void)printf("%-34s %16s %8s\n", "interrupt", "total", "rate");
	inttotal = 0;
	nintr /= sizeof(long);
	while (--nintr >= 0) {
		if (*intrcnt || verbose)
			(void)printf("%-34s %16llu %8llu\n", intrname,
			    (unsigned long long)*intrcnt,
			    (unsigned long long)(*intrcnt / uptime));
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	kread(X_ALLEVENTS, &allevents, sizeof allevents);
	evptr = allevents.tqh_first;
	while (evptr) {
		deref_kptr(evptr, &evcnt, sizeof(evcnt), "event chain trashed");
		evptr = evcnt.ev_list.tqe_next;
		if (evcnt.ev_type != EVCNT_TYPE_INTR)
			continue;

		if (evcnt.ev_count == 0 && !verbose)
			continue;

		deref_kptr(evcnt.ev_group, evgroup, evcnt.ev_grouplen + 1,
		    "event chain trashed");
		deref_kptr(evcnt.ev_name, evname, evcnt.ev_namelen + 1,
		    "event chain trashed");

		(void)printf("%s %s%*s %16llu %8llu\n", evgroup, evname,
		    34 - (evcnt.ev_grouplen + 1 + evcnt.ev_namelen), "",
		    (unsigned long long)evcnt.ev_count,
		    (unsigned long long)(evcnt.ev_count / uptime));

		inttotal += evcnt.ev_count++;
	}
	(void)printf("%-34s %16llu %8llu\n", "Total", inttotal,
	    (unsigned long long)(inttotal / uptime));
}

void
doevcnt(int verbose)
{
	static const char * evtypes [] = { "misc", "intr", "trap" };
	unsigned long long uptime;
	struct evcntlist allevents;
	struct evcnt evcnt, *evptr;
	char evgroup[EVCNT_STRING_MAX], evname[EVCNT_STRING_MAX];

	/* XXX should print type! */

	uptime = getuptime();
	(void)printf("%-34s %16s %8s %s\n", "event", "total", "rate", "type");
	kread(X_ALLEVENTS, &allevents, sizeof allevents);
	evptr = allevents.tqh_first;
	while (evptr) {
		deref_kptr(evptr, &evcnt, sizeof(evcnt), "event chain trashed");

		evptr = evcnt.ev_list.tqe_next;
		if (evcnt.ev_count == 0 && !verbose)
			continue;

		deref_kptr(evcnt.ev_group, evgroup, evcnt.ev_grouplen + 1,
		    "event chain trashed");
		deref_kptr(evcnt.ev_name, evname, evcnt.ev_namelen + 1,
		    "event chain trashed");

		(void)printf("%s %s%*s %16llu %8llu %s\n", evgroup, evname,
		    34 - (evcnt.ev_grouplen + 1 + evcnt.ev_namelen), "",
		    (unsigned long long)evcnt.ev_count,
		    (unsigned long long)(evcnt.ev_count / uptime),
		    (evcnt.ev_type < sizeof(evtypes)/sizeof(evtypes[0]) ?
			evtypes[evcnt.ev_type] : "?"));
	}
}

static char memname[64];

void
domem(void)
{
	struct kmembuckets *kp;
	struct malloc_type ks, *ksp;
	int i, j;
	int len, size, first;
	long totuse = 0, totfree = 0, totreq = 0;
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
		warnx("Kmem statistics are not being gathered by the kernel.");
		return;
	}

	(void)printf("\nMemory usage type by bucket size\n");
	(void)printf("    Size  Type(s)\n");
	kp = &buckets[MINBUCKET];
	for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1, kp++) {
		if (kp->kb_calls == 0)
			continue;
		first = 1;
		len = 8;
		for (kread(X_KMEMSTAT, &ksp, sizeof(ksp));
		     ksp != NULL; ksp = ks.ks_next) {
			deref_kptr(ksp, &ks, sizeof(ks), "malloc type");
			if (ks.ks_calls == 0)
				continue;
			if ((ks.ks_size & j) == 0)
				continue;
			deref_kptr(ks.ks_shortdesc, memname,
			    sizeof(memname), "malloc type name");
			len += 2 + strlen(memname);
			if (first)
				printf("%8d  %s", j, memname);
			else
				printf(",");
			if (len >= 80) {
				printf("\n\t ");
				len = 10 + strlen(memname);
			}
			if (!first)
				printf(" %s", memname);
			first = 0;
		}
		putchar('\n');
	}

	(void)printf(
	    "\nMemory statistics by type                        Type  Kern\n");
	(void)printf(
"         Type  InUse MemUse HighUse  Limit Requests Limit Limit Size(s)\n");
	for (kread(X_KMEMSTAT, &ksp, sizeof(ksp));
	     ksp != NULL; ksp = ks.ks_next) {
		deref_kptr(ksp, &ks, sizeof(ks), "malloc type");
		if (ks.ks_calls == 0)
			continue;
		deref_kptr(ks.ks_shortdesc, memname,
		    sizeof(memname), "malloc type name");
		(void)printf("%14s%6ld%6ldK%7ldK%6ldK%9ld%5u%6u",
		    memname,
		    ks.ks_inuse, (ks.ks_memuse + 1023) / 1024,
		    (ks.ks_maxused + 1023) / 1024,
		    (ks.ks_limit + 1023) / 1024, ks.ks_calls,
		    ks.ks_limblocks, ks.ks_mapblocks);
		first = 1;
		for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1) {
			if ((ks.ks_size & j) == 0)
				continue;
			if (first)
				printf("  %d", j);
			else
				printf(",%d", j);
			first = 0;
		}
		printf("\n");
		totuse += ks.ks_memuse;
		totreq += ks.ks_calls;
	}
	(void)printf("\nMemory totals:  In Use    Free    Requests\n");
	(void)printf("              %7ldK %6ldK    %8ld\n\n",
	    (totuse + 1023) / 1024, (totfree + 1023) / 1024, totreq);
}

void
dopool(int verbose)
{
	int first, ovflw;
	void *addr;
	long total = 0, inuse = 0;
	TAILQ_HEAD(,pool) pool_head;
	struct pool pool, *pp = &pool;
	struct pool_allocator pa;
	char name[32], maxp[32];

	kread(X_POOLHEAD, &pool_head, sizeof(pool_head));
	addr = TAILQ_FIRST(&pool_head);

	for (first = 1; addr != NULL; addr = TAILQ_NEXT(pp, pr_poollist) ) {
		deref_kptr(addr, pp, sizeof(*pp), "pool chain trashed");
		deref_kptr(pp->pr_alloc, &pa, sizeof(pa),
		    "pool allocatior trashed");
		deref_kptr(pp->pr_wchan, name, sizeof(name),
		    "pool wait channel trashed");
		name[sizeof(name)-1] = '\0';

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
		if (pp->pr_nget == 0 && !verbose)
			continue;
		if (pp->pr_maxpages == UINT_MAX)
			snprintf(maxp, sizeof(maxp), "inf");
		else
			snprintf(maxp, sizeof(maxp), "%u", pp->pr_maxpages);
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

		if (pp->pr_roflags & PR_RECURSIVE) {
			/*
			 * Don't count in-use memory, since it's part
			 * of another pool and will be accounted for
			 * there.
			 */
			total += pp->pr_npages * pa.pa_pagesz -
			     (pp->pr_nget - pp->pr_nput) * pp->pr_size;
		} else {
			inuse += (pp->pr_nget - pp->pr_nput) * pp->pr_size;
			total += pp->pr_npages * pa.pa_pagesz;
		}
		dopoolcache(pp, verbose);
	}

	inuse /= 1024;
	total /= 1024;
	printf("\nIn use %ldK, total allocated %ldK; utilization %.1f%%\n",
	    inuse, total, (double)(100 * inuse) / total);
}

void
dopoolcache(struct pool *pp, int verbose)
{
	struct pool_cache pool_cache, *pc = &pool_cache;
	struct pool_cache_group pool_cache_group, *pcg = &pool_cache_group;
	void *addr, *pcg_addr;
	int i;

	if (verbose < 1)
		return;

	for (addr = TAILQ_FIRST(&pp->pr_cachelist); addr != NULL;
	    addr = TAILQ_NEXT(pc, pc_poollist)) {
		deref_kptr(addr, pc, sizeof(*pc), "pool cache trashed");
		printf("\tcache %p: allocfrom %p freeto %p\n", addr,
		    pc->pc_allocfrom, pc->pc_freeto);
		printf("\t    hits %lu misses %lu ngroups %lu nitems %lu\n",
		    pc->pc_hits, pc->pc_misses, pc->pc_ngroups, pc->pc_nitems);
		if (verbose < 2)
			continue;
		for (pcg_addr = TAILQ_FIRST(&pc->pc_grouplist);
		    pcg_addr != NULL; pcg_addr = TAILQ_NEXT(pcg, pcg_list)) {
			deref_kptr(pcg_addr, pcg, sizeof(*pcg),
			    "pool cache group trashed");
			printf("\t\tgroup %p: avail %d\n", pcg_addr,
			    pcg->pcg_avail);
			for (i = 0; i < PCG_NOBJECTS; i++) {
				if (pcg->pcg_objects[i].pcgo_pa !=
				    POOL_PADDR_INVALID) {
					printf("\t\t\t%p, 0x%llx\n",
					    pcg->pcg_objects[i].pcgo_va,
					    (unsigned long long)
					    pcg->pcg_objects[i].pcgo_pa);
				} else {
					printf("\t\t\t%p\n",
					    pcg->pcg_objects[i].pcgo_va);
				}
			}
		}
	}

}

enum hashtype {			/* from <sys/systm.h> */
	HASH_LIST,
	HASH_TAILQ
};

struct uidinfo {		/* XXX: no kernel header file */
	LIST_ENTRY(uidinfo) ui_hash;
	uid_t	ui_uid;
	long	ui_proccnt;
};

struct kernel_hash {
	const char *	description;	/* description */
	int		hashsize;	/* nlist index for hash size */
	int		hashtbl;	/* nlist index for hash table */
	enum hashtype	type;		/* type of hash table */
	size_t		offset;		/* offset of {LIST,TAILQ}_NEXT */
} khashes[] =
{
	{
		"buffer hash",
		X_BUFHASH, X_BUFHASHTBL,
		HASH_LIST, offsetof(struct buf, b_hash)
	}, {
		"inode cache (ihash)",
		X_IHASH, X_IHASHTBL,
		HASH_LIST, offsetof(struct inode, i_hash)
	}, {
		"ipv4 address -> interface hash",
		X_IFADDRHASH, X_IFADDRHASHTBL,
		HASH_LIST, offsetof(struct in_ifaddr, ia_hash),
	}, {
		"name cache hash",
		X_NCHASH, X_NCHASHTBL,
		HASH_LIST, offsetof(struct namecache, nc_hash),
	}, {
		"name cache directory hash",
		X_NCVHASH, X_NCVHASHTBL,
		HASH_LIST, offsetof(struct namecache, nc_vhash),
	}, {
		"nfs client node cache",
		X_NFSNODE, X_NFSNODETBL,
		HASH_LIST, offsetof(struct nfsnode, n_hash)
	}, {
		"user info (uid -> used processes) hash",
		X_UIHASH, X_UIHASHTBL,
		HASH_LIST, offsetof(struct uidinfo, ui_hash),
	}, {
		NULL, -1, -1, 0, 0,
	}
};

void
dohashstat(int verbose, int todo, const char *hashname)
{
	LIST_HEAD(, generic)	*hashtbl_list;
	TAILQ_HEAD(, generic)	*hashtbl_tailq;
	struct kernel_hash	*curhash;
	void	*hashaddr, *hashbuf, *nextaddr;
	size_t	elemsize, hashbufsize, thissize;
	u_long	hashsize;
	int	i, used, items, chain, maxchain;

	hashbuf = NULL;
	hashbufsize = 0;

	if (todo & HASHLIST) {
		printf("Supported hashes:\n");
		for (curhash = khashes; curhash->description; curhash++) {
			if (hashnl[curhash->hashsize].n_value == 0 ||
			    hashnl[curhash->hashtbl].n_value == 0)
				continue;
			printf("\t%-16s%s\n",
			    hashnl[curhash->hashsize].n_name + 1,
			    curhash->description);
		}
		return;
	}

	if (hashname != NULL) {
		for (curhash = khashes; curhash->description; curhash++) {
			if (strcmp(hashnl[curhash->hashsize].n_name + 1,
			    hashname) == 0 &&
			    hashnl[curhash->hashsize].n_value != 0 &&
			    hashnl[curhash->hashtbl].n_value != 0)
				break;
		}
		if (curhash->description == NULL) {
			warnx("%s: no such hash", hashname);
			return;
		}
	}

	printf(
	    "%-16s %8s %8s %8s %8s %8s %8s\n"
	    "%-16s %8s %8s %8s %8s %8s %8s\n",
	    "", "total", "used", "util", "num", "average", "maximum",
	    "hash table", "buckets", "buckets", "%", "items", "chain",
	    "chain");

	for (curhash = khashes; curhash->description; curhash++) {
		if (hashnl[curhash->hashsize].n_value == 0 ||
		    hashnl[curhash->hashtbl].n_value == 0)
			continue;
		if (hashname != NULL &&
		    strcmp(hashnl[curhash->hashsize].n_name + 1, hashname))
			continue;
		elemsize = curhash->type == HASH_LIST ?
		    sizeof(*hashtbl_list) : sizeof(*hashtbl_tailq);
		deref_kptr((void *)hashnl[curhash->hashsize].n_value,
		    &hashsize, sizeof(hashsize),
		    hashnl[curhash->hashsize].n_name);
		hashsize++;
		deref_kptr((void *)hashnl[curhash->hashtbl].n_value,
		    &hashaddr, sizeof(hashaddr),
		    hashnl[curhash->hashtbl].n_name);
		if (verbose)
			printf("%s %lu, %s %p, offset %ld, elemsize %llu\n",
			    hashnl[curhash->hashsize].n_name + 1, hashsize,
			    hashnl[curhash->hashtbl].n_name + 1, hashaddr,
			    (long)curhash->offset,
			    (unsigned long long)elemsize);
		thissize = hashsize * elemsize;
		if (thissize > hashbufsize) {
			hashbufsize = thissize;
			if ((hashbuf = realloc(hashbuf, hashbufsize)) == NULL)
				errx(1, "malloc hashbuf %llu",
				    (unsigned long long)hashbufsize);
		}
		deref_kptr(hashaddr, hashbuf, thissize,
		    hashnl[curhash->hashtbl].n_name);
		used = 0;
		items = maxchain = 0;
		if (curhash->type == HASH_LIST)
			hashtbl_list = hashbuf;
		else
			hashtbl_tailq = hashbuf;
		for (i = 0; i < hashsize; i++) {
			if (curhash->type == HASH_LIST)
				nextaddr = LIST_FIRST(&hashtbl_list[i]);
			else
				nextaddr = TAILQ_FIRST(&hashtbl_tailq[i]);
			if (nextaddr == NULL)
				continue;
			if (verbose)
				printf("%5d: %p\n", i, nextaddr);
			used++;
			chain = 0;
			do {
				chain++;
				deref_kptr((char *)nextaddr + curhash->offset,
				    &nextaddr, sizeof(void *),
				    "hash chain corrupted");
				if (verbose > 1)
					printf("got nextaddr as %p\n",
					    nextaddr);
			} while (nextaddr != NULL);
			items += chain;
			if (verbose && chain > 1)
				printf("\tchain = %d\n", chain);
			if (chain > maxchain)
				maxchain = chain;
		}
		printf("%-16s %8ld %8d %8.2f %8d %8.2f %8d\n",
		    hashnl[curhash->hashsize].n_name + 1,
		    hashsize, used, used * 100.0 / hashsize,
		    items, used ? (double)items / used : 0.0, maxchain);
	}
}

/*
 * kread reads something from the kernel, given its nlist index in namelist[].
 */
void
kread(int nlx, void *addr, size_t size)
{
	const char *sym;

	sym = namelist[nlx].n_name;
	if (*sym == '_')
		++sym;
	if (namelist[nlx].n_type == 0 || namelist[nlx].n_value == 0)
		errx(1, "symbol %s not defined", sym);
	deref_kptr((void *)namelist[nlx].n_value, addr, size, sym);
}

/*
 * Dereference the kernel pointer `kptr' and fill in the local copy
 * pointed to by `ptr'.  The storage space must be pre-allocated,
 * and the size of the copy passed in `len'.
 */
void
deref_kptr(const void *kptr, void *ptr, size_t len, const char *msg)
{

	if (*msg == '_')
		msg++;
	if (kvm_read(kd, (u_long)kptr, (char *)ptr, len) != len)
		errx(1, "kptr %lx: %s: %s", (u_long)kptr, msg, kvm_geterr(kd));
}

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

	if (histnl[0].n_value == 0) {
		warnx("UVM history is not compiled into the kernel.");
		return;
	}

	deref_kptr((void *)histnl[X_UVM_HISTORIES].n_value, &histhead,
	    sizeof(histhead), histnl[X_UVM_HISTORIES].n_name);

	if (histhead.lh_first == NULL) {
		warnx("No active UVM history logs.");
		return;
	}

	if (todo & HISTLIST)
		printf("Active UVM histories:");

	for (histkva = LIST_FIRST(&histhead); histkva != NULL;
	    histkva = LIST_NEXT(&hist, list)) {
		deref_kptr(histkva, &hist, sizeof(hist), "histkva");
		if (hist.namelen > namelen) {
			if (name != NULL)
				free(name);
			namelen = hist.namelen;
			if ((name = malloc(namelen + 1)) == NULL)
				err(1, "malloc history name");
		}

		deref_kptr(hist.name, name, namelen, "history name");
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
		putchar('\n');

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

	deref_kptr(histp->e, histents, histsize, "history entries");
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

			deref_kptr(e->fmt, fmt, fmtlen, "printf format");
			fmt[fmtlen] = '\0';

			deref_kptr(e->fn, fn, fnlen, "function name");
			fn[fnlen] = '\0';

			printf("%06ld.%06ld ", (long int)e->tv.tv_sec,
			    (long int)e->tv.tv_usec);
			printf("%s#%ld: ", fn, e->call);
			printf(fmt, e->v[0], e->v[1], e->v[2], e->v[3]);
			putchar('\n');
		}
		i = (i + 1) % histp->n;
	} while (i != histp->f);

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
	    "usage: %s [-efHilmsUv] [-h hashname] [-u histname] [-c count]\n"
	    "\t\t[-M core] [-N system] [-w wait] [disks]\n", getprogname());
	exit(1);
}
