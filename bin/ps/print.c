/*	$NetBSD: print.c,v 1.52 2000/06/02 03:39:02 simonb Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
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
static char sccsid[] = "@(#)print.c	8.6 (Berkeley) 4/16/94";
#else
__RCSID("$NetBSD: print.c,v 1.52 2000/06/02 03:39:02 simonb Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ucred.h>
#include <sys/sysctl.h>

#include <vm/vm.h>

#include <err.h>
#include <kvm.h>
#include <math.h>
#include <nlist.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "ps.h"

static char *cmdpart __P((char *));
static void  printval __P((char *, VAR *));
static int   titlecmp __P((char *, char **));

#define	min(a,b)	((a) <= (b) ? (a) : (b))
#define	max(a,b)	((a) >= (b) ? (a) : (b))

static char *
cmdpart(arg0)
	char *arg0;
{
	char *cp;

	return ((cp = strrchr(arg0, '/')) != NULL ? cp + 1 : arg0);
}

void
printheader()
{
	VAR *v;
	struct varent *vent;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (v->flag & LJUST) {
			if (vent->next == NULL)	/* last one */
				(void)printf("%s", v->header);
			else
				(void)printf("%-*s", v->width, v->header);
		} else
			(void)printf("%*s", v->width, v->header);
		if (vent->next != NULL)
			(void)putchar(' ');
	}
	(void)putchar('\n');
}

static int
titlecmp(name, argv)
	char *name;
	char **argv;
{
	char *title;
	int namelen;

	if (argv == 0 || argv[0] == 0)
		return (1);

	title = cmdpart(argv[0]);

	if (!strcmp(name, title))
		return (0);

	if (title[0] == '-' && !strcmp(name, title+1))
		return (0);

	namelen = strlen(name);

	if (argv[1] == 0 &&
	    !strncmp(name, title, namelen) &&
	    title[namelen+0] == ':' &&
	    title[namelen+1] == ' ')
		return (0);

	return (1);
}

void
command(ki, ve)
	struct kinfo_proc2 *ki;
	VARENT *ve;
{
	VAR *v;
	int left;
	char **argv, **p, *name;

	v = ve->var;
	if (ve->next != NULL || termwidth != UNLIMITED) {
		if (ve->next == NULL) {
			left = termwidth - (totwidth - v->width);
			if (left < 1) /* already wrapped, just use std width */
				left = v->width;
		} else
			left = v->width;
	} else
		left = -1;
	if (needenv && kd) {
		argv = kvm_getenvv2(kd, ki, termwidth);
		if ((p = argv) != NULL) {
			while (*p) {
				fmt_puts(*p, &left);
				p++;
				fmt_putc(' ', &left);
			}
		}
	}
	if (needcomm) {
		name = ki->p_comm;
		if (!commandonly) {
			argv = NULL;
			if (!use_procfs)
				argv = kvm_getargv2(kd, ki, termwidth);
			else
				argv = procfs_getargv(ki, termwidth);
			if ((p = argv) != NULL) {
				while (*p) {
					fmt_puts(*p, &left);
					p++;
					fmt_putc(' ', &left);
				}
			}
			if (titlecmp(name, argv)) {
				fmt_putc('(', &left);
				fmt_puts(name, &left);
				fmt_putc(')', &left);
			}
			if (use_procfs && argv) {
				free(argv[0]);
				free(argv);
			}
		} else {
			fmt_puts(name, &left);
		}
	}
	if (ve->next && left > 0)
		printf("%*s", left, "");
}

void
ucomm(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%-*s", v->width, k->p_comm);
}

void
logname(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	int n;

	v = ve->var;
	n = min(v->width, MAXLOGNAME);
	(void)printf("%-*.*s", n, n, k->p_login);
	if (v->width > n)
		(void)printf("%*s", v->width - n, "");
}

void
state(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	int flag, is_zombie;
	char *cp;
	VAR *v;
	char buf[16];

	is_zombie = 0;
	v = ve->var;
	flag = k->p_flag;
	cp = buf;

	switch (k->p_stat) {

	case SSTOP:
		*cp = 'T';
		break;

	case SSLEEP:
		if (flag & P_SINTR)	/* interuptable (long) */
			*cp = k->p_slptime >= MAXSLP ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case SRUN:
	case SIDL:
	case SONPROC:
		*cp = 'R';
		break;

	case SZOMB:
	case SDEAD:
		*cp = 'Z';
		is_zombie = 1;
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & P_INMEM) {
	} else
		*cp++ = 'W';
	if (k->p_nice < NZERO)
		*cp++ = '<';
	else if (k->p_nice > NZERO)
		*cp++ = 'N';
	if (flag & P_TRACED)
		*cp++ = 'X';
	if (flag & P_WEXIT && !is_zombie)
		*cp++ = 'E';
	if (flag & P_PPWAIT)
		*cp++ = 'V';
	if ((flag & P_SYSTEM) || k->p_holdcnt)
		*cp++ = 'L';
	if (k->p_eflag & EPROC_SLEADER)
		*cp++ = 's';
	if ((flag & P_CONTROLT) && k->p__pgid == k->p_tpgid)
		*cp++ = '+';
	*cp = '\0';
	(void)printf("%-*s", v->width, buf);
}

void
pnice(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, k->p_nice - NZERO);
}

void
pri(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, k->p_priority - PZERO);
}

void
uname(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%-*s",
	    (int)v->width, user_from_uid(k->p_uid, 0));
}

void
runame(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%-*s",
	    (int)v->width, user_from_uid(k->p_ruid, 0));
}

void
tdev(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	char buff[16];

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV)
		(void)printf("%*s", v->width, "??");
	else {
		(void)snprintf(buff, sizeof(buff),
		    "%d/%d", major(dev), minor(dev));
		(void)printf("%*s", v->width, buff);
	}
}

void
tname(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	const char *ttname;

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL)
		(void)printf("%-*s", v->width, "??");
	else {
		if (strncmp(ttname, "tty", 3) == 0 ||
		    strncmp(ttname, "dty", 3) == 0)
			ttname += 3;
		(void)printf("%*.*s%c", v->width-1, v->width-1, ttname,
			k->p_eflag & EPROC_CTTY ? ' ' : '-');
	}
}

void
longtname(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	const char *ttname;

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL)
		(void)printf("%-*s", v->width, "??");
	else
		(void)printf("%-*s", v->width, ttname);
}

void
started(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	static time_t now;
	time_t startt;
	struct tm *tp;
	char buf[100];

	v = ve->var;
	if (!k->p_uvalid) {
		(void)printf("%-*s", v->width, "-");
		return;
	}

	startt = k->p_ustart_sec;
	tp = localtime(&startt);
	if (!now)
		(void)time(&now);
	if (now - k->p_ustart_sec < 24 * SECSPERHOUR) {
		/* I *hate* SCCS... */
		static char fmt[] = __CONCAT("%l:%", "M%p");
		(void)strftime(buf, sizeof(buf) - 1, fmt, tp);
	} else if (now - k->p_ustart_sec < 7 * SECSPERDAY) {
		/* I *hate* SCCS... */
		static char fmt[] = __CONCAT("%a%", "I%p");
		(void)strftime(buf, sizeof(buf) - 1, fmt, tp);
	} else
		(void)strftime(buf, sizeof(buf) - 1, "%e%b%y", tp);
	(void)printf("%-*s", v->width, buf);
}

void
lstarted(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	time_t startt;
	char buf[100];

	v = ve->var;
	if (!k->p_uvalid) {
		(void)printf("%-*s", v->width, "-");
		return;
	}
	startt = k->p_ustart_sec;
	(void)strftime(buf, sizeof(buf) -1, "%c",
	    localtime(&startt));
	(void)printf("%-*s", v->width, buf);
}

void
wchan(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	int n;

	v = ve->var;
	if (k->p_wchan) {
		if (k->p_wmesg) {
			n = min(v->width, WMESGLEN);
			(void)printf("%-*.*s", n, n, k->p_wmesg);
			if (v->width > n)
				(void)printf("%*s", v->width - n, "");
		} else
			(void)printf("%-*lx", v->width,
			    (long)k->p_wchan);
	} else
		(void)printf("%-*s", v->width, "-");
}

#define pgtok(a)        (((a)*getpagesize())/1024)

void
vsize(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width,
	    pgtok(k->p_vm_dsize + k->p_vm_ssize + k->p_vm_tsize));
}

void
rssize(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	/* XXX don't have info about shared */
	(void)printf("%*d", v->width, pgtok(k->p_vm_rssize));
}

void
p_rssize(k, ve)		/* doesn't account for text */
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, pgtok(k->p_vm_rssize));
}

void
cputime(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;
	long secs;
	long psecs;	/* "parts" of a second. first micro, then centi */
	char obuff[128];

	v = ve->var;
	if (P_ZOMBIE(k) || k->p_uvalid == 0) {
		secs = 0;
		psecs = 0;
	} else {
		/*
		 * This counts time spent handling interrupts.  We could
		 * fix this, but it is not 100% trivial (and interrupt
		 * time fractions only work on the sparc anyway).	XXX
		 */
		secs = k->p_rtime_sec;
		psecs = k->p_rtime_usec;
		if (sumrusage) {
			secs += k->p_uctime_sec;
			psecs += k->p_uctime_usec;
		}
		/*
		 * round and scale to 100's
		 */
		psecs = (psecs + 5000) / 10000;
		secs += psecs / 100;
		psecs = psecs % 100;
	}
	(void)snprintf(obuff, sizeof(obuff),
	    "%3ld:%02ld.%02ld", secs/60, secs%60, psecs);
	(void)printf("%*s", v->width, obuff);
}

double
getpcpu(k)
	struct kinfo_proc2 *k;
{
	static int failure;

	if (!nlistread)
		failure = (kd) ? donlist() : 1;
	if (failure)
		return (0.0);

#define	fxtofl(fixpt)	((double)(fixpt) / fscale)

	/* XXX - I don't like this */
	if (k->p_swtime == 0 || (k->p_flag & P_INMEM) == 0 ||
	    k->p_stat == SZOMB || k->p_stat == SDEAD)
		return (0.0);
	if (rawcpu)
		return (100.0 * fxtofl(k->p_pctcpu));
	return (100.0 * fxtofl(k->p_pctcpu) /
		(1.0 - exp(k->p_swtime * log(ccpu))));
}

void
pcpu(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*.1f", v->width, getpcpu(k));
}

double
getpmem(k)
	struct kinfo_proc2 *k;
{
	static int failure;
	double fracmem;
	int szptudot;

	if (!nlistread)
		failure = (kd) ? donlist() : 1;
	if (failure)
		return (0.0);

	if ((k->p_flag & P_INMEM) == 0)
		return (0.0);
	/* XXX want pmap ptpages, segtab, etc. (per architecture) */
	szptudot = USPACE/getpagesize();
	/* XXX don't have info about shared */
	fracmem = ((float)k->p_vm_rssize + szptudot)/mempages;
	return (100.0 * fracmem);
}

void
pmem(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*.1f", v->width, getpmem(k));
}

void
pagein(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*lld", v->width, 
	    k->p_uvalid ? (long long)k->p_uru_majflt : 0);
}

void
maxrss(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*s", v->width, "-");
}

void
tsize(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, pgtok(k->p_vm_tsize));
}

/*
 * Generic output routines.  Print fields from various prototype
 * structures.
 */
static void
printval(bp, v)
	char *bp;
	VAR *v;
{
	static char ofmt[32] = "%";
	char *fcp, *cp;
	enum type type;

	cp = ofmt + 1;
	fcp = v->fmt;
	if (v->flag & LJUST)
		*cp++ = '-';
	*cp++ = '*';
	while ((*cp++ = *fcp++) != '\0')
		continue;

	/*
	 * Note that the "INF127" check is nonsensical for types
	 * that are or can be signed.
	 */
#define	GET(type)		(*(type *)bp)
#define	CHK_INF127(n)		(((n) > 127) && (v->flag & INF127) ? 127 : (n))

	switch (v->type) {
	case INT32:
		if (sizeof(int32_t) == sizeof(int))
			type = INT;
		else if (sizeof(int32_t) == sizeof(long))
			type = LONG;
		else
			errx(1, "unknown conversion for type %d", v->type);
		break;
	case UINT32:
		if (sizeof(u_int32_t) == sizeof(u_int))
			type = UINT;
		else if (sizeof(u_int32_t) == sizeof(u_long))
			type = ULONG;
		else
			errx(1, "unknown conversion for type %d", v->type);
		break;
	default:
		type = v->type;
		break;
	}

	switch (type) {
	case CHAR:
		(void)printf(ofmt, v->width, GET(char));
		break;
	case UCHAR:
		(void)printf(ofmt, v->width, CHK_INF127(GET(u_char)));
		break;
	case SHORT:
		(void)printf(ofmt, v->width, GET(short));
		break;
	case USHORT:
		(void)printf(ofmt, v->width, CHK_INF127(GET(u_short)));
		break;
	case INT:
		(void)printf(ofmt, v->width, GET(int));
		break;
	case UINT:
		(void)printf(ofmt, v->width, CHK_INF127(GET(u_int)));
		break;
	case LONG:
		(void)printf(ofmt, v->width, GET(long));
		break;
	case ULONG:
		(void)printf(ofmt, v->width, CHK_INF127(GET(u_long)));
		break;
	case KPTR:
		(void)printf(ofmt, v->width, GET(u_long));
		break;
	case KPTR24:
		(void)printf(ofmt, v->width, GET(u_long) & 0xffffff);
		break;
	case SIGLIST:
		{
			sigset_t *s = (sigset_t *)(void *)bp;
			size_t i;
#define SIGSETSIZE	(sizeof(s->__bits) / sizeof(s->__bits[0]))
			char buf[SIGSETSIZE * 8 + 1];

			for (i = 0; i < SIGSETSIZE; i++)
				(void)snprintf(&buf[i * 8], 9, "%.8x",
				    s->__bits[(SIGSETSIZE - 1) - i]);

			/* Skip leading zeroes */
			for (i = 0; buf[i]; i++)
				if (buf[i] != '0')
					break;

			if (buf[i] == '\0')
				i--;
			(void)printf(ofmt, v->width, &buf[i]);
		}
		break;
	default:
		errx(1, "unknown type %d", v->type);
	}
#undef GET
#undef CHK_INF127
}

void
pvar(k, ve)
	struct kinfo_proc2 *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	printval((char *)k + v->off, v);
}
