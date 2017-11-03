/* $NetBSD: vmstat.c,v 1.220 2017/11/03 22:45:14 pgoyette Exp $ */

/*-
 * Copyright (c) 1998, 2000, 2001, 2007 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1986, 1991, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 3/1/95";
#else
__RCSID("$NetBSD: vmstat.c,v 1.220 2017/11/03 22:45:14 pgoyette Exp $");
#endif
#endif /* not lint */

#define	__POOL_EXPOSE

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/uio.h>

#include <sys/buf.h>
#include <sys/evcnt.h>
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
#include <sys/queue.h>
#include <sys/kernhist.h>

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

#include "drvstats.h"

/*
 * All this mess will go away once everything is converted.
 */
#ifdef __HAVE_CPU_DATA_FIRST

# include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;
};
#else
# include <sys/cpu.h>
#endif

/*
 * General namelist
 */
struct nlist namelist[] =
{
#define	X_BOOTTIME	0
	{ .n_name = "_boottime" },
#define	X_HZ		1
	{ .n_name = "_hz" },
#define	X_STATHZ	2
	{ .n_name = "_stathz" },
#define	X_NCHSTATS	3
	{ .n_name = "_nchstats" },
#define	X_ALLEVENTS	4
	{ .n_name = "_allevents" },
#define	X_POOLHEAD	5
	{ .n_name = "_pool_head" },
#define	X_UVMEXP	6
	{ .n_name = "_uvmexp" },
#define	X_TIME_SECOND	7
	{ .n_name = "_time_second" },
#define X_TIME		8
	{ .n_name = "_time" },
#define X_CPU_INFOS	9
	{ .n_name = "_cpu_infos" },
#define	X_NL_SIZE	10
	{ .n_name = NULL },
};

/*
 * Namelist for pre-evcnt interrupt counters.
 */
struct nlist intrnl[] =
{
#define	X_INTRNAMES	0
	{ .n_name = "_intrnames" },
#define	X_EINTRNAMES	1
	{ .n_name = "_eintrnames" },
#define	X_INTRCNT	2
	{ .n_name = "_intrcnt" },
#define	X_EINTRCNT	3
	{ .n_name = "_eintrcnt" },
#define	X_INTRNL_SIZE	4
	{ .n_name = NULL },
};


/*
 * Namelist for hash statistics
 */
struct nlist hashnl[] =
{
#define	X_NFSNODE	0
	{ .n_name = "_nfsnodehash" },
#define	X_NFSNODETBL	1
	{ .n_name = "_nfsnodehashtbl" },
#define	X_IHASH		2
	{ .n_name = "_ihash" },
#define	X_IHASHTBL	3
	{ .n_name = "_ihashtbl" },
#define	X_BUFHASH	4
	{ .n_name = "_bufhash" },
#define	X_BUFHASHTBL	5
	{ .n_name = "_bufhashtbl" },
#define	X_UIHASH	6
	{ .n_name = "_uihash" },
#define	X_UIHASHTBL	7
	{ .n_name = "_uihashtbl" },
#define	X_IFADDRHASH	8
	{ .n_name = "_in_ifaddrhash" },
#define	X_IFADDRHASHTBL	9
	{ .n_name = "_in_ifaddrhashtbl" },
#define	X_NCHASH	10
	{ .n_name = "_nchash" },
#define	X_NCHASHTBL	11
	{ .n_name = "_nchashtbl" },
#define	X_NCVHASH	12
	{ .n_name = "_ncvhash" },
#define	X_NCVHASHTBL	13
	{ .n_name = "_ncvhashtbl" },
#define X_HASHNL_SIZE	14	/* must be last */
	{ .n_name = NULL },
};

/*
 * Namelist for kernel histories
 */
struct nlist histnl[] =
{
	{ .n_name = "_kern_histories" },
#define	X_KERN_HISTORIES		0
	{ .n_name = NULL },
};


#define KILO	1024

struct cpu_counter {
	uint64_t nintr;
	uint64_t nsyscall;
	uint64_t nswtch;
	uint64_t nfault;
	uint64_t ntrap;
	uint64_t nsoft;
} cpucounter, ocpucounter;

struct	uvmexp_sysctl uvmexp, ouvmexp;
int	ndrives;

int	winlines = 20;

kvm_t *kd;


#define	FORKSTAT	0x001
#define	INTRSTAT	0x002
#define	MEMSTAT		0x004
#define	SUMSTAT		0x008
#define	EVCNTSTAT	0x010
#define	VMSTAT		0x020
#define	HISTLIST	0x040
#define	HISTDUMP	0x080
#define	HASHSTAT	0x100
#define	HASHLIST	0x200
#define	VMTOTAL		0x400
#define	POOLCACHESTAT	0x800

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

void	cpustats(int *);
void	cpucounters(struct cpu_counter *);
void	deref_kptr(const void *, void *, size_t, const char *);
void	drvstats(int *);
void	doevcnt(int verbose, int type);
void	dohashstat(int, int, const char *);
void	dointr(int verbose);
void	dopool(int, int);
void	dopoolcache(int);
void	dosum(void);
void	dovmstat(struct timespec *, int);
void	print_total_hdr(void);
void	dovmtotal(struct timespec *, int);
void	kread(struct nlist *, int, void *, size_t);
int	kreadc(struct nlist *, int, void *, size_t);
void	needhdr(int);
void	getnlist(int);
long	getuptime(void);
void	printhdr(void);
long	pct(u_long, u_long);
__dead static void	usage(void);
void	doforkst(void);

void	hist_traverse(int, const char *);
void	hist_dodump(struct kern_history *);
void	hist_traverse_sysctl(int, const char *);
void	hist_dodump_sysctl(int[], unsigned int);

char	**choosedrives(char **);

/* Namelist and memory file names. */
char	*nlistf, *memf;

/* allow old usage [vmstat 1] */
#define	BACKWARD_COMPATIBILITY

static const int clockrate_mib[] = { CTL_KERN, KERN_CLOCKRATE };
static const int vmmeter_mib[] = { CTL_VM, VM_METER };
static const int uvmexp2_mib[] = { CTL_VM, VM_UVMEXP2 };
static const int boottime_mib[] = { CTL_KERN, KERN_BOOTTIME };
static char kvm_errbuf[_POSIX2_LINE_MAX];

int
main(int argc, char *argv[])
{
	int c, todo, verbose, wide;
	struct timespec interval;
	int reps;
	gid_t egid = getegid();
	const char *histname, *hashname;

	histname = hashname = NULL;
	(void)setegid(getgid());
	memf = nlistf = NULL;
	reps = todo = verbose = wide = 0;
	interval.tv_sec = 0;
	interval.tv_nsec = 0;
	while ((c = getopt(argc, argv, "Cc:efh:HilLM:mN:stu:UvWw:")) != -1) {
		switch (c) {
		case 'c':
			reps = atoi(optarg);
			break;
		case 'C':
			todo |= POOLCACHESTAT;
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
		case 't':
			todo |= VMTOTAL;
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
		case 'W':
			wide++;
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

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, kvm_errbuf);
	if (kd == NULL) {
		if (nlistf != NULL || memf != NULL) {
			errx(1, "kvm_openfiles: %s", kvm_errbuf);
		}
	}

	if (nlistf == NULL && memf == NULL)
		(void)setgid(getgid());


	if (todo & VMSTAT) {
		struct winsize winsize;

		(void)drvinit(0);/* Initialize disk stats, no disks selected. */

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


	getnlist(todo);
	/*
	 * Statistics dumping is incompatible with the default
	 * VMSTAT/dovmstat() output. So perform the interval/reps handling
	 * for it here.
	 */
	if ((todo & (VMSTAT|VMTOTAL)) == 0) {
		for (;;) {
			if (todo & (HISTLIST|HISTDUMP)) {
				if ((todo & (HISTLIST|HISTDUMP)) ==
				    (HISTLIST|HISTDUMP))
					errx(1, "you may list or dump,"
					    " but not both!");
				if (memf != NULL)
					hist_traverse(todo, histname);
				else
					hist_traverse_sysctl(todo, histname);
				(void)putchar('\n');
			}
			if (todo & FORKSTAT) {
				doforkst();
				(void)putchar('\n');
			}
			if (todo & MEMSTAT) {
				dopool(verbose, wide);
				(void)putchar('\n');
			}
			if (todo & POOLCACHESTAT) {
				dopoolcache(verbose);
				(void)putchar('\n');
			}
			if (todo & SUMSTAT) {
				dosum();
				(void)putchar('\n');
			}
			if (todo & INTRSTAT) {
				dointr(verbose);
				(void)putchar('\n');
			}
			if (todo & EVCNTSTAT) {
				doevcnt(verbose, EVCNT_TYPE_ANY);
				(void)putchar('\n');
			}
			if (todo & (HASHLIST|HASHSTAT)) {
				if ((todo & (HASHLIST|HASHSTAT)) ==
				    (HASHLIST|HASHSTAT))
					errx(1, "you may list or display,"
					    " but not both!");
				dohashstat(verbose, todo, hashname);
				(void)putchar('\n');
			}

			fflush(stdout);
			if (reps >= 0 && --reps <=0)
				break;
			(void)nanosleep(&interval, NULL);
		}
	} else {
		if ((todo & (VMSTAT|VMTOTAL)) == (VMSTAT|VMTOTAL)) {
			errx(1, "you may not both do vmstat and vmtotal");
		}
		if (todo & VMSTAT)
			dovmstat(&interval, reps);
		if (todo & VMTOTAL)
			dovmtotal(&interval, reps);
	}
	return 0;
}

void
getnlist(int todo)
{
	static int namelist_done = 0;
	static int done = 0;
	int c;
	size_t i;

	if (kd == NULL)
		errx(1, "kvm_openfiles: %s", kvm_errbuf);

	if (!namelist_done) {
		namelist_done = 1;
		if ((c = kvm_nlist(kd, namelist)) != 0) {
			int doexit = 0;
			if (c == -1)
				errx(1, "kvm_nlist: %s %s",
				    "namelist", kvm_geterr(kd));
			for (i = 0; i < __arraycount(namelist)-1; i++)
				if (namelist[i].n_type == 0 &&
				    i != X_TIME_SECOND &&
				    i != X_TIME) {
					if (doexit++ == 0)
						(void)fprintf(stderr,
						    "%s: undefined symbols:",
						    getprogname());
					(void)fprintf(stderr, " %s",
					    namelist[i].n_name);
				}
			if (doexit) {
				(void)fputc('\n', stderr);
				exit(1);
			}
		}
	}
	if ((todo & (SUMSTAT|INTRSTAT)) && !(done & (SUMSTAT|INTRSTAT))) {
		done |= SUMSTAT|INTRSTAT;
		(void) kvm_nlist(kd, intrnl);
	}
	if ((todo & (HASHLIST|HASHSTAT)) && !(done & (HASHLIST|HASHSTAT))) {
		done |= HASHLIST|HASHSTAT;
		if ((c = kvm_nlist(kd, hashnl)) == -1 || c == X_HASHNL_SIZE)
			errx(1, "kvm_nlist: %s %s", "hashnl", kvm_geterr(kd));
	}
	if ((todo & (HISTLIST|HISTDUMP)) && !(done & (HISTLIST|HISTDUMP))) {
		done |= HISTLIST|HISTDUMP;
		if (kvm_nlist(kd, histnl) == -1)
			errx(1, "kvm_nlist: %s %s", "histnl", kvm_geterr(kd));
	}
}

char **
choosedrives(char **argv)
{
	size_t i;

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments, default drives.  If everything isn't filled
	 * in and there are drives not taken care of, display the first few
	 * that fit.
	 */
#define	BACKWARD_COMPATIBILITY
	for (ndrives = 0; *argv; ++argv) {
#ifdef	BACKWARD_COMPATIBILITY
		if (isdigit((unsigned char)**argv))
			break;
#endif
		for (i = 0; i < ndrive; i++) {
			if (strcmp(dr_name[i], *argv))
				continue;
			drv_select[i] = 1;
			++ndrives;
			break;
		}
	}
	for (i = 0; i < ndrive && ndrives < 2; i++) {
		if (drv_select[i])
			continue;
		drv_select[i] = 1;
		++ndrives;
	}

	return (argv);
}

long
getuptime(void)
{
	static struct timespec boottime;
	struct timespec now;
	time_t uptime, nowsec;

	if (memf == NULL) {
		if (boottime.tv_sec == 0) {
			size_t buflen = sizeof(boottime);
			if (sysctl(boottime_mib, __arraycount(boottime_mib),
			    &boottime, &buflen, NULL, 0) == -1)
				warn("Can't get boottime");
		}
		clock_gettime(CLOCK_REALTIME, &now);
	} else {
		if (boottime.tv_sec == 0)
			kread(namelist, X_BOOTTIME, &boottime,
			    sizeof(boottime));
		if (kreadc(namelist, X_TIME_SECOND, &nowsec, sizeof(nowsec))) {
			/*
			 * XXX this assignment dance can be removed once
			 * timeval tv_sec is SUS mandated time_t
			 */
			now.tv_sec = nowsec;
			now.tv_nsec = 0;
		} else {
			kread(namelist, X_TIME, &now, sizeof(now));
		}
	}
	uptime = now.tv_sec - boottime.tv_sec;
	if (uptime <= 0 || uptime > 60*60*24*365*10)
		errx(1, "time makes no sense; namelist must be wrong.");
	return (uptime);
}

int	hz, hdrcnt;

void
print_total_hdr(void)
{

	(void)printf("procs         memory\n");
	(void)printf("ru dw pw sl");
	(void)printf("   total-v  active-v  active-r");
	(void)printf(" vm-sh avm-sh rm-sh arm-sh free\n");
	hdrcnt = winlines - 2;
}

void
dovmtotal(struct timespec *interval, int reps)
{
	struct vmtotal total;
	size_t size;

	(void)signal(SIGCONT, needhdr);

	for (hdrcnt = 1;;) {
		if (!--hdrcnt)
			print_total_hdr();
		if (memf != NULL) {
			warnx("Unable to get vmtotals from crash dump.");
			(void)memset(&total, 0, sizeof(total));
		} else {
			size = sizeof(total);
			if (sysctl(vmmeter_mib, __arraycount(vmmeter_mib),
			    &total, &size, NULL, 0) == -1) {
				warn("Can't get vmtotals");
				(void)memset(&total, 0, sizeof(total));
			}
		}
		(void)printf("%2d ", total.t_rq);
		(void)printf("%2d ", total.t_dw);
		(void)printf("%2d ", total.t_pw);
		(void)printf("%2d ", total.t_sl);

		(void)printf("%9d ", total.t_vm);
		(void)printf("%9d ", total.t_avm);
		(void)printf("%9d ", total.t_arm);
		(void)printf("%5d ", total.t_vmshr);
		(void)printf("%6d ", total.t_avmshr);
		(void)printf("%5d ", total.t_rmshr);
		(void)printf("%6d ", total.t_armshr);
		(void)printf("%5d",  total.t_free);

		(void)putchar('\n');

		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;

		(void)nanosleep(interval, NULL);
	}
}

void
dovmstat(struct timespec *interval, int reps)
{
	struct vmtotal total;
	time_t uptime, halfuptime;
	size_t size;
	int pagesize = getpagesize();
	int ovflw;

	uptime = getuptime();
	halfuptime = uptime / 2;
	(void)signal(SIGCONT, needhdr);

	if (memf != NULL) {
		if (namelist[X_STATHZ].n_type != 0 && namelist[X_STATHZ].n_value != 0)
			kread(namelist, X_STATHZ, &hz, sizeof(hz));
		if (!hz)
			kread(namelist, X_HZ, &hz, sizeof(hz));
	} else {
		struct clockinfo clockinfo;
		size = sizeof(clockinfo);
		if (sysctl(clockrate_mib, 2, &clockinfo, &size, NULL, 0) == -1)
			err(1, "sysctl kern.clockrate failed");
		hz = clockinfo.stathz;
		if (!hz)
			hz = clockinfo.hz;
	}

	for (hdrcnt = 1;;) {
		if (!--hdrcnt)
			printhdr();
		/* Read new disk statistics */
		cpureadstats();
		drvreadstats();
		tkreadstats();
		if (memf != NULL) {
			struct uvmexp uvmexp_kernel;
			/*
			 * XXX Can't do this if we're reading a crash
			 * XXX dump because they're lazily-calculated.
			 */
			warnx("Unable to get vmtotals from crash dump.");
			(void)memset(&total, 0, sizeof(total));
			kread(namelist, X_UVMEXP, &uvmexp_kernel, sizeof(uvmexp_kernel));
#define COPY(field) uvmexp.field = uvmexp_kernel.field
			COPY(pdreact);
			COPY(pageins);
			COPY(pgswapout);
			COPY(pdfreed);
			COPY(pdscans);
#undef COPY
		} else {
			size = sizeof(total);
			if (sysctl(vmmeter_mib, __arraycount(vmmeter_mib),
			    &total, &size, NULL, 0) == -1) {
				warn("Can't get vmtotals");
				(void)memset(&total, 0, sizeof(total));
			}
			size = sizeof(uvmexp);
			if (sysctl(uvmexp2_mib, __arraycount(uvmexp2_mib), &uvmexp,
			    &size, NULL, 0) == -1)
				warn("sysctl vm.uvmexp2 failed");
		}
		cpucounters(&cpucounter);
		ovflw = 0;
		PRWORD(ovflw, " %*d", 2, 1, total.t_rq - 1);
		PRWORD(ovflw, " %*d", 2, 1, total.t_dw + total.t_pw);
#define	pgtok(a) (long)((a) * ((uint32_t)pagesize >> 10))
#define	rate(x)	(u_long)(((x) + halfuptime) / uptime)	/* round */
		PRWORD(ovflw, " %*ld", 9, 1, pgtok(total.t_avm));
		PRWORD(ovflw, " %*ld", 7, 1, pgtok(total.t_free));
		PRWORD(ovflw, " %*ld", 5, 1,
		    rate(cpucounter.nfault - ocpucounter.nfault));
		PRWORD(ovflw, " %*ld", 4, 1,
		    rate(uvmexp.pdreact - ouvmexp.pdreact));
		PRWORD(ovflw, " %*ld", 4, 1,
		    rate(uvmexp.pageins - ouvmexp.pageins));
		PRWORD(ovflw, " %*ld", 5, 1,
		    rate(uvmexp.pgswapout - ouvmexp.pgswapout));
		PRWORD(ovflw, " %*ld", 5, 1,
		    rate(uvmexp.pdfreed - ouvmexp.pdfreed));
		PRWORD(ovflw, " %*ld", 6, 2,
		    rate(uvmexp.pdscans - ouvmexp.pdscans));
		drvstats(&ovflw);
		PRWORD(ovflw, " %*ld", 5, 1,
		    rate(cpucounter.nintr - ocpucounter.nintr));
		PRWORD(ovflw, " %*ld", 5, 1,
		    rate(cpucounter.nsyscall - ocpucounter.nsyscall));
		PRWORD(ovflw, " %*ld", 4, 1,
		    rate(cpucounter.nswtch - ocpucounter.nswtch));
		cpustats(&ovflw);
		(void)putchar('\n');
		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		ouvmexp = uvmexp;
		ocpucounter = cpucounter;
		uptime = interval->tv_sec;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfuptime = uptime == 1 ? 0 : (uptime + 1) / 2;
		(void)nanosleep(interval, NULL);
	}
}

void
printhdr(void)
{
	size_t i;

	(void)printf(" procs    memory      page%*s", 23, "");
	if (ndrives > 0)
		(void)printf("%s %*sfaults      cpu\n",
		    ((ndrives > 1) ? "disks" : "disk"),
		    ((ndrives > 1) ? ndrives * 3 - 4 : 0), "");
	else
		(void)printf("%*s  faults   cpu\n",
		    ndrives * 3, "");

	(void)printf(" r b      avm    fre  flt  re  pi   po   fr   sr ");
	for (i = 0; i < ndrive; i++)
		if (drv_select[i])
			(void)printf("%c%c ", dr_name[i][0],
			    dr_name[i][strlen(dr_name[i]) - 1]);
	(void)printf("  in   sy  cs us sy id\n");
	hdrcnt = winlines - 2;
}

/*
 * Force a header to be prepended to the next output.
 */
void
/*ARGSUSED*/
needhdr(int dummy)
{

	hdrcnt = 1;
}

long
pct(u_long top, u_long bot)
{
	long ans;

	if (bot == 0)
		return (0);
	ans = (long)((quad_t)top * 100 / bot);
	return (ans);
}

#define	PCT(top, bot) (int)pct((u_long)(top), (u_long)(bot))

void
dosum(void)
{
	struct nchstats nch_stats;
	uint64_t nchtotal;
	size_t ssize;
	int active_kernel;
	struct cpu_counter cc;

	/*
	 * The "active" and "inactive" variables
	 * are now estimated by the kernel and sadly
	 * can not easily be dug out of a crash dump.
	 */
	ssize = sizeof(uvmexp);
	memset(&uvmexp, 0, ssize);
	active_kernel = (memf == NULL);
	if (active_kernel) {
		/* only on active kernel */
		if (sysctl(uvmexp2_mib, __arraycount(uvmexp2_mib), &uvmexp,
		    &ssize, NULL, 0) == -1)
			warn("sysctl vm.uvmexp2 failed");
	} else {
		struct uvmexp uvmexp_kernel;
		kread(namelist, X_UVMEXP, &uvmexp_kernel, sizeof(uvmexp_kernel));
#define COPY(field) uvmexp.field = uvmexp_kernel.field
		COPY(pagesize);
		COPY(ncolors);
		COPY(npages);
		COPY(free);
		COPY(paging);
		COPY(wired);
		COPY(zeropages);
		COPY(reserve_pagedaemon);
		COPY(reserve_kernel);
		COPY(anonpages);
		COPY(filepages);
		COPY(execpages);
		COPY(freemin);
		COPY(freetarg);
		COPY(wiredmax);
		COPY(nswapdev);
		COPY(swpages);
		COPY(swpginuse);
		COPY(nswget);
		COPY(pageins);
		COPY(pdpageouts);
		COPY(pgswapin);
		COPY(pgswapout);
		COPY(forks);
		COPY(forks_ppwait);
		COPY(forks_sharevm);
		COPY(pga_zerohit);
		COPY(pga_zeromiss);
		COPY(zeroaborts);
		COPY(colorhit);
		COPY(colormiss);
		COPY(cpuhit);
		COPY(cpumiss);
		COPY(fltnoram);
		COPY(fltnoanon);
		COPY(fltpgwait);
		COPY(fltpgrele);
		COPY(fltrelck);
		COPY(fltrelckok);
		COPY(fltanget);
		COPY(fltanretry);
		COPY(fltamcopy);
		COPY(fltamcopy);
		COPY(fltnomap);
		COPY(fltlget);
		COPY(fltget);
		COPY(flt_anon);
		COPY(flt_acow);
		COPY(flt_obj);
		COPY(flt_prcopy);
		COPY(flt_przero);
		COPY(pdwoke);
		COPY(pdrevs);
		COPY(pdfreed);
		COPY(pdscans);
		COPY(pdanscan);
		COPY(pdobscan);
		COPY(pdreact);
		COPY(pdbusy);
		COPY(pdpending);
		COPY(pddeact);
#undef COPY
	}


	(void)printf("%9" PRIu64 " bytes per page\n", uvmexp.pagesize);

	(void)printf("%9" PRIu64 " page color%s\n",
	    uvmexp.ncolors, uvmexp.ncolors == 1 ? "" : "s");

	(void)printf("%9" PRIu64 " pages managed\n", uvmexp.npages);
	(void)printf("%9" PRIu64 " pages free\n", uvmexp.free);
	if (active_kernel) {
		(void)printf("%9" PRIu64 " pages active\n", uvmexp.active);
		(void)printf("%9" PRIu64 " pages inactive\n", uvmexp.inactive);
	}
	(void)printf("%9" PRIu64 " pages paging\n", uvmexp.paging);
	(void)printf("%9" PRIu64 " pages wired\n", uvmexp.wired);
	(void)printf("%9" PRIu64 " zero pages\n", uvmexp.zeropages);
	(void)printf("%9" PRIu64 " reserve pagedaemon pages\n",
	    uvmexp.reserve_pagedaemon);
	(void)printf("%9" PRIu64 " reserve kernel pages\n", uvmexp.reserve_kernel);
	(void)printf("%9" PRIu64 " anonymous pages\n", uvmexp.anonpages);
	(void)printf("%9" PRIu64 " cached file pages\n", uvmexp.filepages);
	(void)printf("%9" PRIu64 " cached executable pages\n", uvmexp.execpages);

	(void)printf("%9" PRIu64 " minimum free pages\n", uvmexp.freemin);
	(void)printf("%9" PRIu64 " target free pages\n", uvmexp.freetarg);
	(void)printf("%9" PRIu64 " maximum wired pages\n", uvmexp.wiredmax);

	(void)printf("%9" PRIu64 " swap devices\n", uvmexp.nswapdev);
	(void)printf("%9" PRIu64 " swap pages\n", uvmexp.swpages);
	(void)printf("%9" PRIu64 " swap pages in use\n", uvmexp.swpginuse);
	(void)printf("%9" PRIu64 " swap allocations\n", uvmexp.nswget);

	cpucounters(&cc);

	(void)printf("%9" PRIu64 " total faults taken\n", cc.nfault);
	(void)printf("%9" PRIu64 " traps\n", cc.ntrap);
	(void)printf("%9" PRIu64 " device interrupts\n", cc.nintr);
	(void)printf("%9" PRIu64 " CPU context switches\n", cc.nswtch);
	(void)printf("%9" PRIu64 " software interrupts\n", cc.nsoft);
	(void)printf("%9" PRIu64 " system calls\n", cc.nsyscall);
	(void)printf("%9" PRIu64 " pagein requests\n", uvmexp.pageins);
	(void)printf("%9" PRIu64 " pageout requests\n", uvmexp.pdpageouts);
	(void)printf("%9" PRIu64 " pages swapped in\n", uvmexp.pgswapin);
	(void)printf("%9" PRIu64 " pages swapped out\n", uvmexp.pgswapout);
	(void)printf("%9" PRIu64 " forks total\n", uvmexp.forks);
	(void)printf("%9" PRIu64 " forks blocked parent\n", uvmexp.forks_ppwait);
	(void)printf("%9" PRIu64 " forks shared address space with parent\n",
	    uvmexp.forks_sharevm);
	(void)printf("%9" PRIu64 " pagealloc zero wanted and avail\n",
	    uvmexp.pga_zerohit);
	(void)printf("%9" PRIu64 " pagealloc zero wanted and not avail\n",
	    uvmexp.pga_zeromiss);
	(void)printf("%9" PRIu64 " aborts of idle page zeroing\n",
	    uvmexp.zeroaborts);
	(void)printf("%9" PRIu64 " pagealloc desired color avail\n",
	    uvmexp.colorhit);
	(void)printf("%9" PRIu64 " pagealloc desired color not avail\n",
	    uvmexp.colormiss);
	(void)printf("%9" PRIu64 " pagealloc local cpu avail\n",
	    uvmexp.cpuhit);
	(void)printf("%9" PRIu64 " pagealloc local cpu not avail\n",
	    uvmexp.cpumiss);

	(void)printf("%9" PRIu64 " faults with no memory\n", uvmexp.fltnoram);
	(void)printf("%9" PRIu64 " faults with no anons\n", uvmexp.fltnoanon);
	(void)printf("%9" PRIu64 " faults had to wait on pages\n", uvmexp.fltpgwait);
	(void)printf("%9" PRIu64 " faults found released page\n", uvmexp.fltpgrele);
	(void)printf("%9" PRIu64 " faults relock (%" PRIu64 " ok)\n", uvmexp.fltrelck,
	    uvmexp.fltrelckok);
	(void)printf("%9" PRIu64 " anon page faults\n", uvmexp.fltanget);
	(void)printf("%9" PRIu64 " anon retry faults\n", uvmexp.fltanretry);
	(void)printf("%9" PRIu64 " amap copy faults\n", uvmexp.fltamcopy);
	(void)printf("%9" PRIu64 " neighbour anon page faults\n", uvmexp.fltnamap);
	(void)printf("%9" PRIu64 " neighbour object page faults\n", uvmexp.fltnomap);
	(void)printf("%9" PRIu64 " locked pager get faults\n", uvmexp.fltlget);
	(void)printf("%9" PRIu64 " unlocked pager get faults\n", uvmexp.fltget);
	(void)printf("%9" PRIu64 " anon faults\n", uvmexp.flt_anon);
	(void)printf("%9" PRIu64 " anon copy on write faults\n", uvmexp.flt_acow);
	(void)printf("%9" PRIu64 " object faults\n", uvmexp.flt_obj);
	(void)printf("%9" PRIu64 " promote copy faults\n", uvmexp.flt_prcopy);
	(void)printf("%9" PRIu64 " promote zero fill faults\n", uvmexp.flt_przero);

	(void)printf("%9" PRIu64 " times daemon wokeup\n",uvmexp.pdwoke);
	(void)printf("%9" PRIu64 " revolutions of the clock hand\n", uvmexp.pdrevs);
	(void)printf("%9" PRIu64 " pages freed by daemon\n", uvmexp.pdfreed);
	(void)printf("%9" PRIu64 " pages scanned by daemon\n", uvmexp.pdscans);
	(void)printf("%9" PRIu64 " anonymous pages scanned by daemon\n",
	    uvmexp.pdanscan);
	(void)printf("%9" PRIu64 " object pages scanned by daemon\n", uvmexp.pdobscan);
	(void)printf("%9" PRIu64 " pages reactivated\n", uvmexp.pdreact);
	(void)printf("%9" PRIu64 " pages found busy by daemon\n", uvmexp.pdbusy);
	(void)printf("%9" PRIu64 " total pending pageouts\n", uvmexp.pdpending);
	(void)printf("%9" PRIu64 " pages deactivated\n", uvmexp.pddeact);

	if (active_kernel) {
		ssize = sizeof(nch_stats);
		if (sysctlbyname("vfs.namecache_stats", &nch_stats, &ssize,
		    NULL, 0)) {
			warn("vfs.namecache_stats failed");
			memset(&nch_stats, 0, sizeof(nch_stats));
		}
	} else {
		kread(namelist, X_NCHSTATS, &nch_stats, sizeof(nch_stats));
	}

	nchtotal = nch_stats.ncs_goodhits + nch_stats.ncs_neghits +
	    nch_stats.ncs_badhits + nch_stats.ncs_falsehits +
	    nch_stats.ncs_miss + nch_stats.ncs_long;
	(void)printf("%9" PRIu64 " total name lookups\n", nchtotal);
	(void)printf("%9" PRIu64 " good hits\n", nch_stats.ncs_goodhits);
	(void)printf("%9" PRIu64 " negative hits\n", nch_stats.ncs_neghits);
	(void)printf("%9" PRIu64 " bad hits\n", nch_stats.ncs_badhits);
	(void)printf("%9" PRIu64 " false hits\n", nch_stats.ncs_falsehits);
	(void)printf("%9" PRIu64 " miss\n", nch_stats.ncs_miss);
	(void)printf("%9" PRIu64 " too long\n", nch_stats.ncs_long);
	(void)printf("%9" PRIu64 " pass2 hits\n", nch_stats.ncs_pass2);
	(void)printf("%9" PRIu64 " 2passes\n", nch_stats.ncs_2passes);
	(void)printf(
	    "%9s cache hits (%d%% pos + %d%% neg) system %d%% per-process\n",
	    "", PCT(nch_stats.ncs_goodhits, nchtotal),
	    PCT(nch_stats.ncs_neghits, nchtotal),
	    PCT(nch_stats.ncs_pass2, nchtotal));
	(void)printf("%9s deletions %d%%, falsehits %d%%, toolong %d%%\n", "",
	    PCT(nch_stats.ncs_badhits, nchtotal),
	    PCT(nch_stats.ncs_falsehits, nchtotal),
	    PCT(nch_stats.ncs_long, nchtotal));
}

void
doforkst(void)
{
	if (memf != NULL) {
		struct uvmexp uvmexp_kernel;
		kread(namelist, X_UVMEXP, &uvmexp_kernel, sizeof(uvmexp_kernel));
#define COPY(field) uvmexp.field = uvmexp_kernel.field
		COPY(forks);
		COPY(forks_ppwait);
		COPY(forks_sharevm);
#undef COPY
	} else {
		size_t size = sizeof(uvmexp);
		if (sysctl(uvmexp2_mib, __arraycount(uvmexp2_mib), &uvmexp,
		    &size, NULL, 0) == -1)
			warn("sysctl vm.uvmexp2 failed");
	}

	(void)printf("%" PRIu64 " forks total\n", uvmexp.forks);
	(void)printf("%" PRIu64 " forks blocked parent\n", uvmexp.forks_ppwait);
	(void)printf("%" PRIu64 " forks shared address space with parent\n",
	    uvmexp.forks_sharevm);
}

void
drvstats(int *ovflwp)
{
	size_t dn;
	double dtime;
	int ovflw = *ovflwp;

	/* Calculate disk stat deltas. */
	cpuswap();
	drvswap();
	tkswap();

	for (dn = 0; dn < ndrive; ++dn) {
		/* elapsed time for disk stats */
		dtime = cur.cp_etime;
		if (cur.timestamp[dn].tv_sec || cur.timestamp[dn].tv_usec) {
			dtime = (double)cur.timestamp[dn].tv_sec +
				((double)cur.timestamp[dn].tv_usec / (double)1000000);
		}

		if (!drv_select[dn])
	 		continue;
		PRWORD(ovflw, " %*.0f", 3, 1,
		    (cur.rxfer[dn] + cur.wxfer[dn]) / dtime);
	}
	*ovflwp = ovflw;
}

void
cpucounters(struct cpu_counter *cc)
{
	static struct cpu_info **cpu_infos;
	static int initialised;
	struct cpu_info **slot;

	if (memf == NULL) {
		cc->nintr = uvmexp.intrs;
		cc->nsyscall = uvmexp.syscalls;
		cc->nswtch = uvmexp.swtch;
		cc->nfault = uvmexp.faults;
		cc->ntrap = uvmexp.traps;
		cc->nsoft = uvmexp.softs;
		return;
	}

	if (!initialised) {
		kread(namelist, X_CPU_INFOS, &cpu_infos, sizeof(cpu_infos));
		initialised = 1;
	}

	slot = cpu_infos;

	memset(cc, 0, sizeof(*cc));

	for (;;) {
		struct cpu_info tci, *ci = NULL;

		deref_kptr(slot++, &ci, sizeof(ci), "CPU array trashed");
		if (!ci) {
			break;
		}

		if ((size_t)kvm_read(kd, (u_long)ci, &tci, sizeof(tci))
		    != sizeof(tci)) {
			warnx("Can't read cpu info from %p (%s)",
			    ci, kvm_geterr(kd));
			memset(cc, 0, sizeof(*cc));
			return;
		}
		cc->nintr += tci.ci_data.cpu_nintr;
		cc->nsyscall += tci.ci_data.cpu_nsyscall;
		cc->nswtch = tci.ci_data.cpu_nswtch;
		cc->nfault = tci.ci_data.cpu_nfault;
		cc->ntrap = tci.ci_data.cpu_ntrap;
		cc->nsoft = tci.ci_data.cpu_nsoft;
	}
}

void
cpustats(int *ovflwp)
{
	int state;
	double pcnt, total;
	double stat_us, stat_sy, stat_id;
	int ovflw = *ovflwp;

	total = 0;
	for (state = 0; state < CPUSTATES; ++state)
		total += cur.cp_time[state];
	if (total)
		pcnt = 100 / total;
	else
		pcnt = 0;
	stat_us = (cur.cp_time[CP_USER] + cur.cp_time[CP_NICE]) * pcnt;
	stat_sy = (cur.cp_time[CP_SYS] + cur.cp_time[CP_INTR]) * pcnt;
	stat_id = cur.cp_time[CP_IDLE] * pcnt;
	PRWORD(ovflw, " %*.0f", ((stat_sy >= 100) ? 2 : 3), 1, stat_us);
	PRWORD(ovflw, " %*.0f", ((stat_us >= 100 || stat_id >= 100) ? 2 : 3), 1,
	    stat_sy);
	PRWORD(ovflw, " %*.0f", 3, 1, stat_id);
	*ovflwp = ovflw;
}

void
dointr(int verbose)
{
	unsigned long *intrcnt, *ointrcnt;
	unsigned long long inttotal, uptime;
	int nintr, inamlen;
	char *intrname, *ointrname;

	inttotal = 0;
	uptime = getuptime();
	nintr = intrnl[X_EINTRCNT].n_value - intrnl[X_INTRCNT].n_value;
	inamlen = intrnl[X_EINTRNAMES].n_value - intrnl[X_INTRNAMES].n_value;
	if (nintr != 0 && inamlen != 0) {
		(void)printf("%-34s %16s %8s\n", "interrupt", "total", "rate");

		ointrcnt = intrcnt = malloc((size_t)nintr);
		ointrname = intrname = malloc((size_t)inamlen);
		if (intrcnt == NULL || intrname == NULL)
			errx(1, "%s", "");
		kread(intrnl, X_INTRCNT, intrcnt, (size_t)nintr);
		kread(intrnl, X_INTRNAMES, intrname, (size_t)inamlen);
		nintr /= sizeof(long);
		while (--nintr >= 0) {
			if (*intrcnt || verbose)
				(void)printf("%-34s %16llu %8llu\n", intrname,
					     (unsigned long long)*intrcnt,
					     (unsigned long long)
					     (*intrcnt / uptime));
			intrname += strlen(intrname) + 1;
			inttotal += *intrcnt++;
		}
		free(ointrcnt);
		free(ointrname);
	}

	doevcnt(verbose, EVCNT_TYPE_INTR);
}

void
doevcnt(int verbose, int type)
{
	static const char * const evtypes [] = { "misc", "intr", "trap" };
	uint64_t counttotal, uptime;
	struct evcntlist allevents;
	struct evcnt evcnt, *evptr;
	size_t evlen_max, total_max, rate_max;
	char evgroup[EVCNT_STRING_MAX], evname[EVCNT_STRING_MAX];

	counttotal = 0;
	uptime = getuptime();

	if (memf == NULL) do {
		const int mib[4] = { CTL_KERN, KERN_EVCNT, type,
		    verbose ? KERN_EVCNT_COUNT_ANY : KERN_EVCNT_COUNT_NONZERO };
		size_t buflen0, buflen = 0;
		void *buf0, *buf = NULL;
		const struct evcnt_sysctl *evs, *last_evs;
		for (;;) {
			size_t newlen;
			int error;
			if (buflen)
				buf = malloc(buflen);
			error = sysctl(mib, __arraycount(mib),
			    buf, &newlen, NULL, 0);
			if (error) {
				err(1, "kern.evcnt");
				if (buf)
					free(buf);
				return;
			}
			if (newlen <= buflen) {
				buflen = newlen;
				break;
			}
			if (buf)
				free(buf);
			buflen = newlen;
		}
		buflen0 = buflen;
		evs = buf0 = buf;
		last_evs = (void *)((char *)buf + buflen);
		buflen /= sizeof(uint64_t);
		/* calc columns */
		evlen_max = 0;
		total_max = sizeof("total") - 1;
		rate_max = sizeof("rate") - 1;
		while (evs < last_evs
		    && buflen >= sizeof(*evs)/sizeof(uint64_t)
		    && buflen >= evs->ev_len) {
			char cbuf[64];
			size_t len;
			len = strlen(evs->ev_strings + evs->ev_grouplen + 1);
			len += evs->ev_grouplen + 1;
			if (evlen_max < len)
				evlen_max= len;
			len = snprintf(cbuf, sizeof(cbuf), "%"PRIu64,
			    evs->ev_count);
			if (total_max < len)
				total_max = len;
			len = snprintf(cbuf, sizeof(cbuf), "%"PRIu64,
			    evs->ev_count / uptime);
			if (rate_max < len)
				rate_max = len;
			buflen -= evs->ev_len;
			evs = (const void *)
			    ((const uint64_t *)evs + evs->ev_len);
		}

		(void)printf(type == EVCNT_TYPE_ANY ?
		    "%-*s  %*s %*s %s\n" :
		    "%-*s  %*s %*s\n",
		    (int)evlen_max, "interrupt",
		    (int)total_max, "total",
		    (int)rate_max, "rate",
		    "type");

		buflen = buflen0;
		evs = buf0;
		last_evs = (void *)((char *)buf + buflen);
		buflen /= sizeof(uint64_t);
		while (evs < last_evs
		    && buflen >= sizeof(*evs)/sizeof(uint64_t)
		    && buflen >= evs->ev_len) {
			(void)printf(type == EVCNT_TYPE_ANY ?
			    "%s %s%*s  %*"PRIu64" %*"PRIu64" %s\n" :
			    "%s %s%*s  %*"PRIu64" %*"PRIu64"\n",
			    evs->ev_strings,
			    evs->ev_strings + evs->ev_grouplen + 1,
			    (int)evlen_max - (evs->ev_grouplen + 1
			    + evs->ev_namelen), "",
			    (int)total_max, evs->ev_count,
			    (int)rate_max, evs->ev_count / uptime,
			    (evs->ev_type < __arraycount(evtypes) ?
			    evtypes[evs->ev_type] : "?"));
			buflen -= evs->ev_len;
			counttotal += evs->ev_count;
			evs = (const void *)
			    ((const uint64_t *)evs + evs->ev_len);
		}
		free(buf);
		if (type != EVCNT_TYPE_ANY)
			(void)printf("%-*s  %*"PRIu64" %*"PRIu64"\n",
			    (int)evlen_max, "Total",
			    (int)total_max, counttotal,
			    (int)rate_max, counttotal / uptime);
		return;
	} while (/*CONSTCOND*/ 0);

	if (type == EVCNT_TYPE_ANY)
		(void)printf("%-34s %16s %8s %s\n", "event", "total", "rate",
		    "type");

	kread(namelist, X_ALLEVENTS, &allevents, sizeof allevents);
	evptr = TAILQ_FIRST(&allevents);
	while (evptr) {
		deref_kptr(evptr, &evcnt, sizeof(evcnt), "event chain trashed");

		evptr = TAILQ_NEXT(&evcnt, ev_list);
		if (evcnt.ev_count == 0 && !verbose)
			continue;
		if (type != EVCNT_TYPE_ANY && evcnt.ev_type != type)
			continue;

		deref_kptr(evcnt.ev_group, evgroup,
		    (size_t)evcnt.ev_grouplen + 1, "event chain trashed");
		deref_kptr(evcnt.ev_name, evname,
		    (size_t)evcnt.ev_namelen + 1, "event chain trashed");

		(void)printf(type == EVCNT_TYPE_ANY ?
		    "%s %s%*s %16"PRIu64" %8"PRIu64" %s\n" :
		    "%s %s%*s %16"PRIu64" %8"PRIu64"\n",
		    evgroup, evname,
		    34 - (evcnt.ev_grouplen + 1 + evcnt.ev_namelen), "",
		    evcnt.ev_count,
		    (evcnt.ev_count / uptime),
		    (evcnt.ev_type < __arraycount(evtypes) ?
			evtypes[evcnt.ev_type] : "?"));

		counttotal += evcnt.ev_count;
	}
	if (type != EVCNT_TYPE_ANY)
		(void)printf("%-34s %16"PRIu64" %8"PRIu64"\n",
		    "Total", counttotal, counttotal / uptime);
}

static void
dopool_sysctl(int verbose, int wide)
{
	uint64_t total, inuse, this_total, this_inuse;
	struct {
		uint64_t pt_nget;
		uint64_t pt_nfail;
		uint64_t pt_nput;
		uint64_t pt_nout;
		uint64_t pt_nitems;
		uint64_t pt_npagealloc;
		uint64_t pt_npagefree;
		uint64_t pt_npages;
	} pool_totals;
	size_t i, len;
	int name_len, ovflw;
	struct pool_sysctl *pp, *data;
	char in_use[8], avail[8], maxp[32];

	data = asysctlbyname("kern.pool", &len);
	if (data == NULL)
		err(1, "failed to reead kern.pool");

	memset(&pool_totals, 0, sizeof pool_totals);
	total = inuse = 0;
	len /= sizeof(*data);

	(void)printf("Memory resource pool statistics\n");
	(void)printf(
	    "%-*s%*s%*s%5s%*s%s%s%*s%*s%6s%s%6s%6s%6s%5s%s%s\n",
	    wide ? 16 : 11, "Name",
	    wide ? 6 : 5, "Size",
	    wide ? 12 : 9, "Requests",
	    "Fail",
	    wide ? 12 : 9, "Releases",
	    wide ? "  InUse" : "",
	    wide ? " Avail" : "",
	    wide ? 7 : 6, "Pgreq",
	    wide ? 7 : 6, "Pgrel",
	    "Npage",
	    wide ? " PageSz" : "",
	    "Hiwat",
	    "Minpg",
	    "Maxpg",
	    "Idle",
	    wide ? " Flags" : "",
	    wide ? "   Util" : "");

	name_len = MIN((int)sizeof(pp->pr_wchan), wide ? 16 : 11);
	for (i = 0; i < len; ++i) {
		pp = &data[i];
		if (pp->pr_nget == 0 && !verbose)
			continue;
		if (pp->pr_maxpages == UINT_MAX)
			(void)snprintf(maxp, sizeof(maxp), "inf");
		else
			(void)snprintf(maxp, sizeof(maxp), "%" PRIu64,
			    pp->pr_maxpages);
		ovflw = 0;
		PRWORD(ovflw, "%-*s", name_len, 0, pp->pr_wchan);
		PRWORD(ovflw, " %*" PRIu64, wide ? 6 : 5, 1, pp->pr_size);
		PRWORD(ovflw, " %*" PRIu64, wide ? 12 : 9, 1, pp->pr_nget);
		pool_totals.pt_nget += pp->pr_nget;
		PRWORD(ovflw, " %*" PRIu64, 5, 1, pp->pr_nfail);
		pool_totals.pt_nfail += pp->pr_nfail;
		PRWORD(ovflw, " %*" PRIu64, wide ? 12 : 9, 1, pp->pr_nput);
		pool_totals.pt_nput += pp->pr_nput;
		if (wide) {
			PRWORD(ovflw, " %*" PRIu64, 7, 1, pp->pr_nout);
			pool_totals.pt_nout += pp->pr_nout;
		}
		if (wide) {
			PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_nitems);
			pool_totals.pt_nitems += pp->pr_nitems;
		}
		PRWORD(ovflw, " %*" PRIu64, wide ? 7 : 6, 1, pp->pr_npagealloc);
		pool_totals.pt_npagealloc += pp->pr_npagealloc;
		PRWORD(ovflw, " %*" PRIu64, wide ? 7 : 6, 1, pp->pr_npagefree);
		pool_totals.pt_npagefree += pp->pr_npagefree;
		PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_npages);
		pool_totals.pt_npages += pp->pr_npages;
		if (wide)
			PRWORD(ovflw, " %*" PRIu64, 7, 1, pp->pr_pagesize);
		PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_hiwat);
		PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_minpages);
		PRWORD(ovflw, " %*s", 6, 1, maxp);
		PRWORD(ovflw, " %*" PRIu64, 5, 1, pp->pr_nidle);
		if (wide)
			PRWORD(ovflw, " 0x%0*" PRIx64, 4, 1,
			    pp->pr_flags);

		this_inuse = pp->pr_nout * pp->pr_size;
		this_total = pp->pr_npages * pp->pr_pagesize;
		if (pp->pr_flags & PR_RECURSIVE) {
			/*
			 * Don't count in-use memory, since it's part
			 * of another pool and will be accounted for
			 * there.
			 */
			total += (this_total - this_inuse);
		} else {
			inuse += this_inuse;
			total += this_total;
		}
		if (wide) {
			if (this_total == 0)
				(void)printf("   ---");
			else
				(void)printf(" %5.1f%%",
				    (100.0 * this_inuse) / this_total);
		}
		(void)printf("\n");
	}
	if (wide) {
		snprintf(in_use, sizeof in_use, "%7"PRId64, pool_totals.pt_nout);
		snprintf(avail, sizeof avail, "%6"PRId64, pool_totals.pt_nitems);
	} else {
		in_use[0] = '\0';
		avail[0] = '\0';
	}
	(void)printf(
	    "%-*s%*s%*"PRId64"%5"PRId64"%*"PRId64"%s%s%*"PRId64"%*"PRId64"%6"PRId64"\n",
	    wide ? 16 : 11, "Totals",
	    wide ? 6 : 5, "",
	    wide ? 12 : 9, pool_totals.pt_nget,
	    pool_totals.pt_nfail,
	    wide ? 12 : 9, pool_totals.pt_nput,
	    in_use,
	    avail,
	    wide ? 7 : 6, pool_totals.pt_npagealloc,
	    wide ? 7 : 6, pool_totals.pt_npagefree,
	    pool_totals.pt_npages);

	inuse /= KILO;
	total /= KILO;
	(void)printf(
	    "\nIn use %" PRIu64 "K, "
	    "total allocated %" PRIu64 "K; utilization %.1f%%\n",
	    inuse, total, (100.0 * inuse) / total);

	free(data);
}

void
dopool(int verbose, int wide)
{
	int first, ovflw;
	void *addr;
	long total, inuse, this_total, this_inuse;
	struct {
		uint64_t pt_nget;
		uint64_t pt_nfail;
		uint64_t pt_nput;
		uint64_t pt_nout;
		uint64_t pt_nitems;
		uint64_t pt_npagealloc;
		uint64_t pt_npagefree;
		uint64_t pt_npages;
	} pool_totals;
	char in_use[8];
	char avail[8];
	TAILQ_HEAD(,pool) pool_head;
	struct pool pool, *pp = &pool;
	struct pool_allocator pa;
	char name[32], maxp[32];

	if (memf == NULL)
		return dopool_sysctl(verbose, wide);

	memset(&pool_totals, 0, sizeof pool_totals);
	kread(namelist, X_POOLHEAD, &pool_head, sizeof(pool_head));
	addr = TAILQ_FIRST(&pool_head);

	total = inuse = 0;

	for (first = 1; addr != NULL; addr = TAILQ_NEXT(pp, pr_poollist) ) {
		deref_kptr(addr, pp, sizeof(*pp), "pool chain trashed");
		deref_kptr(pp->pr_alloc, &pa, sizeof(pa),
		    "pool allocator trashed");
		deref_kptr(pp->pr_wchan, name, sizeof(name),
		    "pool wait channel trashed");
		name[sizeof(name)-1] = '\0';

		if (first) {
			(void)printf("Memory resource pool statistics\n");
			(void)printf(
			    "%-*s%*s%*s%5s%*s%s%s%*s%*s%6s%s%6s%6s%6s%5s%s%s\n",
			    wide ? 16 : 11, "Name",
			    wide ? 6 : 5, "Size",
			    wide ? 12 : 9, "Requests",
			    "Fail",
			    wide ? 12 : 9, "Releases",
			    wide ? "  InUse" : "",
			    wide ? " Avail" : "",
			    wide ? 7 : 6, "Pgreq",
			    wide ? 7 : 6, "Pgrel",
			    "Npage",
			    wide ? " PageSz" : "",
			    "Hiwat",
			    "Minpg",
			    "Maxpg",
			    "Idle",
			    wide ? " Flags" : "",
			    wide ? "   Util" : "");
			first = 0;
		}
		if (pp->pr_nget == 0 && !verbose)
			continue;
		if (pp->pr_maxpages == UINT_MAX)
			(void)snprintf(maxp, sizeof(maxp), "inf");
		else
			(void)snprintf(maxp, sizeof(maxp), "%u",
			    pp->pr_maxpages);
		ovflw = 0;
		PRWORD(ovflw, "%-*s", wide ? 16 : 11, 0, name);
		PRWORD(ovflw, " %*u", wide ? 6 : 5, 1, pp->pr_size);
		PRWORD(ovflw, " %*lu", wide ? 12 : 9, 1, pp->pr_nget);
		pool_totals.pt_nget += pp->pr_nget;
		PRWORD(ovflw, " %*lu", 5, 1, pp->pr_nfail);
		pool_totals.pt_nfail += pp->pr_nfail;
		PRWORD(ovflw, " %*lu", wide ? 12 : 9, 1, pp->pr_nput);
		pool_totals.pt_nput += pp->pr_nput;
		if (wide) {
			PRWORD(ovflw, " %*u", 7, 1, pp->pr_nout);
			pool_totals.pt_nout += pp->pr_nout;
		}
		if (wide) {
			PRWORD(ovflw, " %*u", 6, 1, pp->pr_nitems);
			pool_totals.pt_nitems += pp->pr_nitems;
		}
		PRWORD(ovflw, " %*lu", wide ? 7 : 6, 1, pp->pr_npagealloc);
		pool_totals.pt_npagealloc += pp->pr_npagealloc;
		PRWORD(ovflw, " %*lu", wide ? 7 : 6, 1, pp->pr_npagefree);
		pool_totals.pt_npagefree += pp->pr_npagefree;
		PRWORD(ovflw, " %*u", 6, 1, pp->pr_npages);
		pool_totals.pt_npages += pp->pr_npages;
		if (wide)
			PRWORD(ovflw, " %*u", 7, 1, pa.pa_pagesz);
		PRWORD(ovflw, " %*u", 6, 1, pp->pr_hiwat);
		PRWORD(ovflw, " %*u", 6, 1, pp->pr_minpages);
		PRWORD(ovflw, " %*s", 6, 1, maxp);
		PRWORD(ovflw, " %*lu", 5, 1, pp->pr_nidle);
		if (wide)
			PRWORD(ovflw, " 0x%0*x", 4, 1,
			    pp->pr_flags | pp->pr_roflags);

		this_inuse = pp->pr_nout * pp->pr_size;
		this_total = pp->pr_npages * pa.pa_pagesz;
		if (pp->pr_roflags & PR_RECURSIVE) {
			/*
			 * Don't count in-use memory, since it's part
			 * of another pool and will be accounted for
			 * there.
			 */
			total += (this_total - this_inuse);
		} else {
			inuse += this_inuse;
			total += this_total;
		}
		if (wide) {
			if (this_total == 0)
				(void)printf("   ---");
			else
				(void)printf(" %5.1f%%",
				    (100.0 * this_inuse) / this_total);
		}
		(void)printf("\n");
	}
	if (wide) {
		snprintf(in_use, sizeof in_use, "%7"PRId64, pool_totals.pt_nout);
		snprintf(avail, sizeof avail, "%6"PRId64, pool_totals.pt_nitems);
	} else {
		in_use[0] = '\0';
		avail[0] = '\0';
	}
	(void)printf(
	    "%-*s%*s%*"PRId64"%5"PRId64"%*"PRId64"%s%s%*"PRId64"%*"PRId64"%6"PRId64"\n",
	    wide ? 16 : 11, "Totals",
	    wide ? 6 : 5, "",
	    wide ? 12 : 9, pool_totals.pt_nget,
	    pool_totals.pt_nfail,
	    wide ? 12 : 9, pool_totals.pt_nput,
	    in_use,
	    avail,
	    wide ? 7 : 6, pool_totals.pt_npagealloc,
	    wide ? 7 : 6, pool_totals.pt_npagefree,
	    pool_totals.pt_npages);

	inuse /= KILO;
	total /= KILO;
	(void)printf(
	    "\nIn use %ldK, total allocated %ldK; utilization %.1f%%\n",
	    inuse, total, (100.0 * inuse) / total);
}

static void
dopoolcache_sysctl(int verbose)
{
	struct pool_sysctl *data, *pp;
	size_t i, len;
	bool first = true;
	int ovflw;
	uint64_t tot;
	double p;

	data = asysctlbyname("kern.pool", &len);
	if (data == NULL)
		err(1, "failed to reead kern.pool");
	len /= sizeof(*data);

	for (i = 0; i < len; ++i) {
		pp = &data[i];
		if (pp->pr_cache_meta_size == 0)
			continue;

		if (pp->pr_cache_nmiss_global == 0 && !verbose)
			continue;

		if (first) {
			(void)printf("Pool cache statistics.\n");
			(void)printf("%-*s%*s%*s%*s%*s%*s%*s%*s%*s%*s\n",
			    12, "Name",
			    6, "Spin",
			    6, "GrpSz",
			    5, "Full",
			    5, "Emty",
			    10, "PoolLayer",
			    11, "CacheLayer",
			    6, "Hit%",
			    12, "CpuLayer",
			    6, "Hit%"
			);
			first = false;
		}

		ovflw = 0;
		PRWORD(ovflw, "%-*s", MIN((int)sizeof(pp->pr_wchan), 13), 1,
		    pp->pr_wchan);
		PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_cache_ncontended);
		PRWORD(ovflw, " %*" PRIu64, 6, 1, pp->pr_cache_meta_size);
		PRWORD(ovflw, " %*" PRIu64, 5, 1, pp->pr_cache_nfull);
		PRWORD(ovflw, " %*" PRIu64, 5, 1, pp->pr_cache_nempty);
		PRWORD(ovflw, " %*" PRIu64, 10, 1, pp->pr_cache_nmiss_global);

		tot = pp->pr_cache_nhit_global + pp->pr_cache_nmiss_global;
		p = pp->pr_cache_nhit_global * 100.0 / tot;
		PRWORD(ovflw, " %*" PRIu64, 11, 1, tot);
		PRWORD(ovflw, " %*.1f", 6, 1, p);

		tot = pp->pr_cache_nhit_pcpu + pp->pr_cache_nmiss_pcpu;
		p = pp->pr_cache_nhit_pcpu * 100.0 / tot;
		PRWORD(ovflw, " %*" PRIu64, 12, 1, tot);
		PRWORD(ovflw, " %*.1f", 6, 1, p);
		printf("\n");
	}
}

void
dopoolcache(int verbose)
{
	struct pool_cache pool_cache, *pc = &pool_cache;
	pool_cache_cpu_t cache_cpu, *cc = &cache_cpu;
	TAILQ_HEAD(,pool) pool_head;
	struct pool pool, *pp = &pool;
	char name[32];
	uint64_t cpuhit, cpumiss, tot;
	void *addr;
	int first, ovflw;
	size_t i;
	double p;

	if (memf == NULL)
		return dopoolcache_sysctl(verbose);

	kread(namelist, X_POOLHEAD, &pool_head, sizeof(pool_head));
	addr = TAILQ_FIRST(&pool_head);

	for (first = 1; addr != NULL; addr = TAILQ_NEXT(pp, pr_poollist) ) {
		deref_kptr(addr, pp, sizeof(*pp), "pool chain trashed");
		if (pp->pr_cache == NULL)
			continue;
		deref_kptr(pp->pr_wchan, name, sizeof(name),
		    "pool wait channel trashed");
		deref_kptr(pp->pr_cache, pc, sizeof(*pc), "pool cache trashed");
		if (pc->pc_misses == 0 && !verbose)
			continue;
		name[sizeof(name)-1] = '\0';

		cpuhit = 0;
		cpumiss = 0;
		for (i = 0; i < __arraycount(pc->pc_cpus); i++) {
		    	if ((addr = pc->pc_cpus[i]) == NULL)
		    		continue;
			deref_kptr(addr, cc, sizeof(*cc),
			    "pool cache cpu trashed");
			cpuhit += cc->cc_hits;
			cpumiss += cc->cc_misses;
		}

		if (first) {
			(void)printf("Pool cache statistics.\n");
			(void)printf("%-*s%*s%*s%*s%*s%*s%*s%*s%*s%*s\n",
			    12, "Name",
			    6, "Spin",
			    6, "GrpSz",
			    5, "Full",
			    5, "Emty",
			    10, "PoolLayer",
			    11, "CacheLayer",
			    6, "Hit%",
			    12, "CpuLayer",
			    6, "Hit%"
			);
			first = 0;
		}

		ovflw = 0;
		PRWORD(ovflw, "%-*s", 13, 1, name);
		PRWORD(ovflw, " %*llu", 6, 1, (long long)pc->pc_contended);
		PRWORD(ovflw, " %*u", 6, 1, pc->pc_pcgsize);
		PRWORD(ovflw, " %*u", 5, 1, pc->pc_nfull);
		PRWORD(ovflw, " %*u", 5, 1, pc->pc_nempty);
		PRWORD(ovflw, " %*llu", 10, 1, (long long)pc->pc_misses);

		tot = pc->pc_hits + pc->pc_misses;
		p = pc->pc_hits * 100.0 / (tot);
		PRWORD(ovflw, " %*llu", 11, 1, (long long)tot);
		PRWORD(ovflw, " %*.1f", 6, 1, p);

		tot = cpuhit + cpumiss;
		p = cpuhit * 100.0 / (tot);
		PRWORD(ovflw, " %*llu", 12, 1, (long long)tot);
		PRWORD(ovflw, " %*.1f", 6, 1, p);
		printf("\n");
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
	void	*hashaddr, *hashbuf, *nhashbuf, *nextaddr;
	size_t	elemsize, hashbufsize, thissize;
	u_long	hashsize, i;
	int	used, items, chain, maxchain;

	hashbuf = NULL;
	hashbufsize = 0;

	if (todo & HASHLIST) {
		(void)printf("Supported hashes:\n");
		for (curhash = khashes; curhash->description; curhash++) {
			if (hashnl[curhash->hashsize].n_value == 0 ||
			    hashnl[curhash->hashtbl].n_value == 0)
				continue;
			(void)printf("\t%-16s%s\n",
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

	(void)printf(
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
			(void)printf(
			    "%s %lu, %s %p, offset %ld, elemsize %llu\n",
			    hashnl[curhash->hashsize].n_name + 1, hashsize,
			    hashnl[curhash->hashtbl].n_name + 1, hashaddr,
			    (long)curhash->offset,
			    (unsigned long long)elemsize);
		thissize = hashsize * elemsize;
		if (hashbuf == NULL || thissize > hashbufsize) {
			if ((nhashbuf = realloc(hashbuf, thissize)) == NULL)
				errx(1, "malloc hashbuf %llu",
				    (unsigned long long)hashbufsize);
			hashbuf = nhashbuf;
			hashbufsize = thissize;
		}
		deref_kptr(hashaddr, hashbuf, thissize,
		    hashnl[curhash->hashtbl].n_name);
		used = 0;
		items = maxchain = 0;
		if (curhash->type == HASH_LIST) {
			hashtbl_list = hashbuf;
			hashtbl_tailq = NULL;
		} else {
			hashtbl_list = NULL;
			hashtbl_tailq = hashbuf;
		}
		for (i = 0; i < hashsize; i++) {
			if (curhash->type == HASH_LIST)
				nextaddr = LIST_FIRST(&hashtbl_list[i]);
			else
				nextaddr = TAILQ_FIRST(&hashtbl_tailq[i]);
			if (nextaddr == NULL)
				continue;
			if (verbose)
				(void)printf("%5lu: %p\n", i, nextaddr);
			used++;
			chain = 0;
			do {
				chain++;
				deref_kptr((char *)nextaddr + curhash->offset,
				    &nextaddr, sizeof(void *),
				    "hash chain corrupted");
				if (verbose > 1)
					(void)printf("got nextaddr as %p\n",
					    nextaddr);
			} while (nextaddr != NULL);
			items += chain;
			if (verbose && chain > 1)
				(void)printf("\tchain = %d\n", chain);
			if (chain > maxchain)
				maxchain = chain;
		}
		(void)printf("%-16s %8ld %8d %8.2f %8d %8.2f %8d\n",
		    hashnl[curhash->hashsize].n_name + 1,
		    hashsize, used, used * 100.0 / hashsize,
		    items, used ? (double)items / used : 0.0, maxchain);
	}
}

/*
 * kreadc like kread but returns 1 if sucessful, 0 otherwise
 */
int
kreadc(struct nlist *nl, int nlx, void *addr, size_t size)
{
	const char *sym;

	sym = nl[nlx].n_name;
	if (*sym == '_')
		++sym;
	if (nl[nlx].n_type == 0 || nl[nlx].n_value == 0)
		return 0;
	deref_kptr((void *)nl[nlx].n_value, addr, size, sym);
	return 1;
}

/*
 * kread reads something from the kernel, given its nlist index in namelist[].
 */
void
kread(struct nlist *nl, int nlx, void *addr, size_t size)
{
	const char *sym;

	sym = nl[nlx].n_name;
	if (*sym == '_')
		++sym;
	if (nl[nlx].n_type == 0 || nl[nlx].n_value == 0)
		errx(1, "symbol %s not defined", sym);
	deref_kptr((void *)nl[nlx].n_value, addr, size, sym);
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
	if ((size_t)kvm_read(kd, (u_long)kptr, (char *)ptr, len) != len)
		errx(1, "kptr %lx: %s: %s", (u_long)kptr, msg, kvm_geterr(kd));
}

/*
 * Traverse the kernel history buffers, performing the requested action.
 *
 * Note, we assume that if we're not listing, we're dumping.
 */
void
hist_traverse(int todo, const char *histname)
{
	struct kern_history_head histhead;
	struct kern_history hist, *histkva;
	char *name = NULL;
	size_t namelen = 0;

	if (histnl[0].n_value == 0) {
		warnx("kernel history is not compiled into the kernel.");
		return;
	}

	deref_kptr((void *)histnl[X_KERN_HISTORIES].n_value, &histhead,
	    sizeof(histhead), histnl[X_KERN_HISTORIES].n_name);

	if (histhead.lh_first == NULL) {
		warnx("No active kernel history logs.");
		return;
	}

	if (todo & HISTLIST)
		(void)printf("Active kernel histories:");

	for (histkva = LIST_FIRST(&histhead); histkva != NULL;
	    histkva = LIST_NEXT(&hist, list)) {
		deref_kptr(histkva, &hist, sizeof(hist), "histkva");
		if (name == NULL || hist.namelen > namelen) {
			if (name != NULL)
				free(name);
			namelen = hist.namelen;
			if ((name = malloc(namelen + 1)) == NULL)
				err(1, "malloc history name");
		}

		deref_kptr(hist.name, name, namelen, "history name");
		name[namelen] = '\0';
		if (todo & HISTLIST)
			(void)printf(" %s", name);
		else {
			/*
			 * If we're dumping all histories, do it, else
			 * check to see if this is the one we want.
			 */
			if (histname == NULL || strcmp(histname, name) == 0) {
				if (histname == NULL)
					(void)printf(
					    "\nkernel history `%s':\n", name);
				hist_dodump(&hist);
			}
		}
	}

	if (todo & HISTLIST)
		(void)putchar('\n');

	if (name != NULL)
		free(name);
}

/*
 * Actually dump the history buffer at the specified KVA.
 */
void
hist_dodump(struct kern_history *histp)
{
	struct kern_history_ent *histents, *e;
	struct timeval tv;
	size_t histsize;
	char *fmt = NULL, *fn = NULL;
	size_t fmtlen = 0, fnlen = 0;
	unsigned i;

	histsize = sizeof(struct kern_history_ent) * histp->n;

	if ((histents = malloc(histsize)) == NULL)
		err(1, "malloc history entries");

	(void)memset(histents, 0, histsize);

	(void)printf("%"PRIu32" entries, next is %"PRIu32"\n",
	    histp->n, histp->f);

	deref_kptr(histp->e, histents, histsize, "history entries");
	i = histp->f;
	do {
		e = &histents[i];
		if (e->fmt != NULL) {
			if (fmt == NULL || e->fmtlen > fmtlen) {
				if (fmt != NULL)
					free(fmt);
				fmtlen = e->fmtlen;
				if ((fmt = malloc(fmtlen + 1)) == NULL)
					err(1, "malloc printf format");
			}
			if (fn == NULL || e->fnlen > fnlen) {
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

			bintime2timeval(&e->bt, &tv);
			(void)printf("%06ld.%06ld ", (long int)tv.tv_sec,
			    (long int)tv.tv_usec);
			(void)printf("%s#%" PRId32 "@%" PRId32 "d: ",
			    fn, e->call, e->cpunum);
			(void)printf(fmt, e->v[0], e->v[1], e->v[2], e->v[3]);
			(void)putchar('\n');
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
hist_traverse_sysctl(int todo, const char *histname)
{
	int error;
	int mib[4];
	unsigned int i;
	size_t len, miblen;
	struct sysctlnode query, histnode[32];

	/* retrieve names of available histories */
	miblen = __arraycount(mib);
	error = sysctlnametomib("kern.hist", mib, &miblen);
	if (error != 0) {
		if (errno == ENOENT) {
 			warnx("kernel history is not compiled into the kernel.");
			return;
		} else
			err(EXIT_FAILURE, "nametomib failed");
	}
 
	/* get the list of nodenames below kern.hist */
	mib[2] = CTL_QUERY;
	memset(&query, 0, sizeof(query));
	query.sysctl_flags = SYSCTL_VERSION;
	len = sizeof(histnode);
	error = sysctl(mib, 3, &histnode[0], &len, &query, sizeof(query));
	if (error != 0) {
		err(1, "query failed");
		return;
	}
	if (len == 0) {
 		warnx("No active kernel history logs.");
 		return;
 	}
 
	len = len / sizeof(histnode[0]);	/* get # of entries returned */

 	if (todo & HISTLIST)
 		(void)printf("Active kernel histories:");
 
	for (i = 0; i < len; i++) {
 		if (todo & HISTLIST)
			(void)printf(" %s", histnode[i].sysctl_name);
 		else {
 			/*
 			 * If we're dumping all histories, do it, else
 			 * check to see if this is the one we want.
 			 */
			if (histname == NULL ||
			    strcmp(histname, histnode[i].sysctl_name) == 0) {
 				if (histname == NULL)
 					(void)printf(
					    "\nkernel history `%s':\n",
					    histnode[i].sysctl_name);
				mib[2] = histnode[i].sysctl_num;
				mib[3] = CTL_EOL;
				hist_dodump_sysctl(mib, 4);
 			}
 		}
 	}
 
 	if (todo & HISTLIST)
 		(void)putchar('\n');
	else if (mib[2] == CTL_QUERY)
		warnx("history %s not found", histname);
 }
 
 /*
  * Actually dump the history buffer at the specified KVA.
  */
 void
hist_dodump_sysctl(int mib[], unsigned int miblen)
 {
	struct sysctl_history *hist;
	struct timeval tv;
	struct sysctl_history_event *e;
 	size_t histsize;
	char *strp;
 	unsigned i;
	char *fmt = NULL, *fn = NULL;
 
	hist = NULL;
	histsize = 0;
 	do {
		errno = 0;
		if (sysctl(mib, miblen, hist, &histsize, NULL, 0) == 0)
			break;
		if (errno != ENOMEM)
			break;
		if ((hist = realloc(hist, histsize)) == NULL)
			errx(1, "realloc history buffer");
	} while (errno == ENOMEM);
	if (errno != 0)
		err(1, "sysctl failed");
 
	strp = (char *)(&hist->sh_events[hist->sh_numentries]);
 
	(void)printf("%"PRIu32" entries, next is %"PRIu32"\n",
	    hist->sh_numentries,
	    hist->sh_nextfree);
 
	i = hist->sh_nextfree;

	do {
		e = &hist->sh_events[i];
		if (e->she_fmtoffset != 0) {
			fmt = &strp[e->she_fmtoffset];
			fn = &strp[e->she_funcoffset];
			bintime2timeval(&e->she_bintime, &tv);
			(void)printf("%06ld.%06ld %s#%"PRIu32"@%"PRIu32": ",
			    (long int)tv.tv_sec, (long int)tv.tv_usec,
			    fn, e->she_callnumber, e->she_cpunum);
			(void)printf(fmt, e->she_values[0], e->she_values[1],
			     e->she_values[2], e->she_values[3]);
 			(void)putchar('\n');
 		}
		i = (i + 1) % hist->sh_numentries;
	} while (i != hist->sh_nextfree);
 
	free(hist);
 }

static void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-CefHiLlmstUvW] [-c count] [-h hashname] [-M core] [-N system]\n"
	    "\t\t[-u histname] [-w wait] [disks]\n", getprogname());
	exit(1);
}
