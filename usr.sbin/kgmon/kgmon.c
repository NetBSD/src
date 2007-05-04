/*	$NetBSD: kgmon.c,v 1.21 2007/05/04 16:19:43 christos Exp $	*/

/*
 * Copyright (c) 1983, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)kgmon.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: kgmon.c,v 1.21 2007/05/04 16:19:43 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/sysctl.h>
#include <sys/gmon.h>

#include <ctype.h>
#include <errno.h>
#include <kvm.h>
#include <err.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct nlist nl[] = {
#define	N_GMONPARAM	0
	{ "__gmonparam" },
#define	N_PROFHZ	1
	{ "_profhz" },
	{ 0, }
};

struct kvmvars {
	kvm_t	*kd;
	struct gmonparam gpm;
};

static int	bflag, hflag, kflag, rflag, pflag;
static int	debug = 0;
static void	setprof(struct kvmvars *kvp, int state);
static void	dumpstate(struct kvmvars *kvp);
static void	reset(struct kvmvars *kvp);
static int	openfiles(char *, char *, struct kvmvars *);
static int	getprof(struct kvmvars *);
static void	kern_readonly(int);
static int	getprofhz(struct kvmvars *);

int
main(int argc, char **argv)
{
	int ch, mode, disp, accessmode;
	struct kvmvars kvmvars;
	char *sys, *kmemf;

	setprogname(argv[0]);
	(void)seteuid(getuid());
	kmemf = NULL;
	sys = NULL;
	while ((ch = getopt(argc, argv, "M:N:bdhpr")) != -1) {
		switch((char)ch) {

		case 'M':
			kmemf = optarg;
			kflag = 1;
			break;

		case 'N':
			sys = optarg;
			break;

		case 'b':
			bflag = 1;
			break;

		case 'h':
			hflag = 1;
			break;

		case 'p':
			pflag = 1;
			break;

		case 'r':
			rflag = 1;
			break;

		case 'd':
			debug = 1;
			break;

		default:
			(void)fprintf(stderr,
			    "usage: %s [-bdhrp] [-M core] [-N system]\n",
			    getprogname());
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

#define BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		sys = *argv;
		if (*++argv) {
			kmemf = *argv;
			++kflag;
		}
	}
#endif
	accessmode = openfiles(sys, kmemf, &kvmvars);
	mode = getprof(&kvmvars);
	if (hflag)
		disp = GMON_PROF_OFF;
	else if (bflag)
		disp = GMON_PROF_ON;
	else
		disp = mode;
	if (pflag)
		dumpstate(&kvmvars);
	if (rflag)
		reset(&kvmvars);
	if (accessmode == O_RDWR)
		setprof(&kvmvars, disp);
	(void)fprintf(stdout, "%s: kernel profiling is %s.\n",
	     getprogname(), disp == GMON_PROF_OFF ? "off" : "running");
	return (0);
}

/*
 * Check that profiling is enabled and open any ncessary files.
 */
static int
openfiles(char *sys, char *kmemf, struct kvmvars *kvp)
{
	int mib[3], state, openmode;
	size_t size;
	char errbuf[_POSIX2_LINE_MAX];

	if (!kflag) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROF;
		mib[2] = GPROF_STATE;
		size = sizeof state;
		if (sysctl(mib, 3, &state, &size, NULL, 0) < 0)
			err(EXIT_FAILURE, "profiling not defined in kernel");
		if (!(bflag || hflag || rflag ||
		    (pflag && state == GMON_PROF_ON)))
			return (O_RDONLY);
		(void)seteuid(0);
		if (sysctl(mib, 3, NULL, NULL, &state, size) >= 0)
			return (O_RDWR);
		(void)seteuid(getuid());
		kern_readonly(state);
		return (O_RDONLY);
	}
	openmode = (bflag || hflag || pflag || rflag) ? O_RDWR : O_RDONLY;
	kvp->kd = kvm_openfiles(sys, kmemf, NULL, openmode, errbuf);
	if (kvp->kd == NULL) {
		if (openmode == O_RDWR) {
			openmode = O_RDONLY;
			kvp->kd = kvm_openfiles(sys, kmemf, NULL, O_RDONLY,
			    errbuf);
		}
		if (kvp->kd == NULL)
			errx(EXIT_FAILURE, "kvm_openfiles: %s", errbuf);
		kern_readonly(GMON_PROF_ON);
	}
	if (kvm_nlist(kvp->kd, nl) < 0)
		errx(EXIT_FAILURE, "%s: no namelist", sys);
	if (!nl[N_GMONPARAM].n_value)
		errx(EXIT_FAILURE, "profiling not defined in kernel");
	return (openmode);
}

/*
 * Suppress options that require a writable kernel.
 */
static void
kern_readonly(int mode)
{

	(void)fprintf(stderr, "%s: kernel read-only: ", getprogname());
	if (pflag && mode == GMON_PROF_ON)
		(void)fprintf(stderr, "data may be inconsistent\n");
	if (rflag)
		(void)fprintf(stderr, "-r supressed\n");
	if (bflag)
		(void)fprintf(stderr, "-b supressed\n");
	if (hflag)
		(void)fprintf(stderr, "-h supressed\n");
	rflag = bflag = hflag = 0;
}

/*
 * Get the state of kernel profiling.
 */
static int
getprof(struct kvmvars *kvp)
{
	int mib[3];
	size_t size;

	if (kflag) {
		size = kvm_read(kvp->kd, nl[N_GMONPARAM].n_value, &kvp->gpm,
		    sizeof kvp->gpm);
	} else {
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROF;
		mib[2] = GPROF_GMONPARAM;
		size = sizeof kvp->gpm;
		if (sysctl(mib, 3, &kvp->gpm, &size, NULL, 0) < 0)
			size = 0;
	}
	if (size != sizeof kvp->gpm)
		errx(EXIT_FAILURE, "cannot get gmonparam: %s",
		    kflag ? kvm_geterr(kvp->kd) : strerror(errno));
	return (kvp->gpm.state);
}

/*
 * Enable or disable kernel profiling according to the state variable.
 */
static void
setprof(struct kvmvars *kvp, int state)
{
	struct gmonparam *p = (struct gmonparam *)nl[N_GMONPARAM].n_value;
	int mib[3], oldstate;
	size_t sz;

	sz = sizeof(state);
	if (!kflag) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROF;
		mib[2] = GPROF_STATE;
		if (sysctl(mib, 3, &oldstate, &sz, NULL, 0) < 0)
			goto bad;
		if (oldstate == state)
			return;
		(void)seteuid(0);
		if (sysctl(mib, 3, NULL, NULL, &state, sz) >= 0) {
			(void)seteuid(getuid());
			return;
		}
		(void)seteuid(getuid());
	} else if (kvm_write(kvp->kd, (u_long)&p->state, (void *)&state, sz) 
	    == sz)
		return;
bad:
	warnx("cannot turn profiling %s", state == GMON_PROF_OFF ?
	    "off" : "on");
}

/*
 * Build the gmon.out file.
 */
static void
dumpstate(struct kvmvars *kvp)
{
	FILE *fp;
	struct rawarc rawarc;
	struct tostruct *tos;
	u_long frompc;
	u_short *froms, *tickbuf;
	int mib[3];
	size_t i;
	struct gmonhdr h;
	int fromindex, endfrom, toindex;
	size_t kcountsize;

	setprof(kvp, GMON_PROF_OFF);
	fp = fopen("gmon.out", "w");
	if (fp == NULL) {
		warn("cannot open `gmon.out'");
		return;
	}

	/*
	 * Build the gmon header and write it to a file.
	 */
	bzero(&h, sizeof(h));
	h.lpc = kvp->gpm.lowpc;
	h.hpc = kvp->gpm.highpc;
	h.ncnt = kvp->gpm.kcountsize + sizeof(h);
	h.version = GMONVERSION;
	h.profrate = getprofhz(kvp);
	if (fwrite(&h, sizeof(h), 1, fp) != 1)
		err(EXIT_FAILURE, "writing header to gmon.out");

	kcountsize = (size_t)kvp->gpm.kcountsize;

	/*
	 * Write out the tick buffer.
	 */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROF;
	if ((tickbuf = malloc(kcountsize)) == NULL)
		err(EXIT_FAILURE, "Cannot allocate %zu kcount space",
		    kcountsize);
	if (kflag) {
		i = kvm_read(kvp->kd, (u_long)kvp->gpm.kcount, tickbuf,
		    kcountsize);
	} else {
		mib[2] = GPROF_COUNT;
		i = kcountsize;
		if (sysctl(mib, 3, tickbuf, &i, NULL, 0) < 0)
			i = 0;
	}
	if (i != kcountsize)
		errx(EXIT_FAILURE, "read ticks: read %zu, got %zu: %s",
		    kcountsize, i,
		    kflag ? kvm_geterr(kvp->kd) : strerror(errno));
	if ((fwrite(tickbuf, kcountsize, 1, fp)) != 1)
		err(EXIT_FAILURE, "writing tocks to gmon.out");
	free(tickbuf);

	/*
	 * Write out the arc info.
	 */
	if ((froms = malloc((size_t)kvp->gpm.fromssize)) == NULL)
		err(EXIT_FAILURE, "cannot allocate %zu froms space",
		    (size_t)kvp->gpm.fromssize);
	if (kflag) {
		i = kvm_read(kvp->kd, (u_long)kvp->gpm.froms, froms,
		    (size_t)kvp->gpm.fromssize);
	} else {
		mib[2] = GPROF_FROMS;
		i = kvp->gpm.fromssize;
		if (sysctl(mib, 3, froms, &i, NULL, 0) < 0)
			i = 0;
	}
	if (i != kvp->gpm.fromssize)
		errx(EXIT_FAILURE, "read froms: read %lu, got %lu: %s",
		    kvp->gpm.fromssize, (u_long)i,
		    kflag ? kvm_geterr(kvp->kd) : strerror(errno));
	if ((tos = malloc((size_t)kvp->gpm.tossize)) == NULL)
		err(EXIT_FAILURE, "cannot allocate %zu tos space",
		    (size_t)kvp->gpm.tossize);
	if (kflag) {
		i = kvm_read(kvp->kd, (u_long)kvp->gpm.tos, (void *)tos,
		    (size_t)kvp->gpm.tossize);
	} else {
		mib[2] = GPROF_TOS;
		i = kvp->gpm.tossize;
		if (sysctl(mib, 3, tos, &i, NULL, 0) < 0)
			i = 0;
	}
	if (i != kvp->gpm.tossize)
		errx(EXIT_FAILURE, "read tos: read %zu, got %zu: %s",
		    (size_t)kvp->gpm.tossize, i,
		    kflag ? kvm_geterr(kvp->kd) : strerror(errno));
	if (debug)
		(void)fprintf(stderr, "%s: lowpc 0x%lx, textsize 0x%lx\n",
		     getprogname(), kvp->gpm.lowpc, kvp->gpm.textsize);
	endfrom = kvp->gpm.fromssize / sizeof(*froms);
	for (fromindex = 0; fromindex < endfrom; ++fromindex) {
		if (froms[fromindex] == 0)
			continue;
		frompc = (u_long)kvp->gpm.lowpc +
		    (fromindex * kvp->gpm.hashfraction * sizeof(*froms));
		for (toindex = froms[fromindex]; toindex != 0;
		   toindex = tos[toindex].link) {
			if (debug)
			    (void)fprintf(stderr,
			    "%s: [mcleanup] frompc 0x%lx selfpc 0x%lx count %ld\n",
			    getprogname(), frompc, tos[toindex].selfpc,
			    tos[toindex].count);
			rawarc.raw_frompc = frompc;
			rawarc.raw_selfpc = (u_long)tos[toindex].selfpc;
			rawarc.raw_count = tos[toindex].count;
			if (fwrite(&rawarc, sizeof(rawarc), 1,fp) != 1){
				err(EXIT_FAILURE,
				    "writing raw arc to gmon.out");
			}
		}
	}
	free(tos);
	(void)fclose(fp);
}

/*
 * Get the profiling rate.
 */
static int
getprofhz(struct kvmvars *kvp)
{
	int mib[2], profrate;
	size_t size;
	struct clockinfo clockrate;

	if (kflag) {
		profrate = 1;
		if (kvm_read(kvp->kd, nl[N_PROFHZ].n_value, &profrate,
		    sizeof profrate) != sizeof profrate)
			warnx("get clockrate: %s", kvm_geterr(kvp->kd));
		return (profrate);
	}
	mib[0] = CTL_KERN;
	mib[1] = KERN_CLOCKRATE;
	clockrate.profhz = 1;
	size = sizeof clockrate;
	if (sysctl(mib, 2, &clockrate, &size, NULL, 0) < 0)
		warn("get clockrate");
	return (clockrate.profhz);
}

/*
 * Reset the kernel profiling date structures.
 */
static void
reset(struct kvmvars *kvp)
{
	char *zbuf;
	size_t biggest;
	int mib[3];

	setprof(kvp, GMON_PROF_OFF);

	biggest = (size_t)kvp->gpm.kcountsize;
	if ((size_t)kvp->gpm.fromssize > biggest)
		biggest = (size_t)kvp->gpm.fromssize;
	if ((size_t)kvp->gpm.tossize > biggest)
		biggest = (size_t)kvp->gpm.tossize;
	if ((zbuf = malloc(biggest)) == NULL)
		err(EXIT_FAILURE, "cannot allocate zbuf space");
	(void)memset(zbuf, 0, biggest);
	if (kflag) {
		if (kvm_write(kvp->kd, (u_long)kvp->gpm.kcount, zbuf,
		    (size_t)kvp->gpm.kcountsize) != kvp->gpm.kcountsize)
			errx(EXIT_FAILURE, "tickbuf zero: %s",
			     kvm_geterr(kvp->kd));
		if (kvm_write(kvp->kd, (u_long)kvp->gpm.froms, zbuf,
		    (size_t)kvp->gpm.fromssize) != kvp->gpm.fromssize)
			errx(EXIT_FAILURE, "froms zero: %s",
			     kvm_geterr(kvp->kd));
		if (kvm_write(kvp->kd, (u_long)kvp->gpm.tos, zbuf,
		    (size_t)kvp->gpm.tossize) != kvp->gpm.tossize)
			errx(EXIT_FAILURE, "tos zero: %s", kvm_geterr(kvp->kd));
		free(zbuf);
		return;
	}
	(void)seteuid(0);
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROF;
	mib[2] = GPROF_COUNT;
	if (sysctl(mib, 3, NULL, NULL, zbuf, (size_t)kvp->gpm.kcountsize) < 0)
		err(EXIT_FAILURE, "tickbuf zero");
	mib[2] = GPROF_FROMS;
	if (sysctl(mib, 3, NULL, NULL, zbuf, (size_t)kvp->gpm.fromssize) < 0)
		err(EXIT_FAILURE, "froms zero");
	mib[2] = GPROF_TOS;
	if (sysctl(mib, 3, NULL, NULL, zbuf, (size_t)kvp->gpm.tossize) < 0)
		err(EXIT_FAILURE, "tos zero");
	(void)seteuid(getuid());
	free(zbuf);
}
