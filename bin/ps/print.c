/*	$NetBSD: print.c,v 1.62 2001/01/09 01:21:59 itojun Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__RCSID("$NetBSD: print.c,v 1.62 2001/01/09 01:21:59 itojun Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ucred.h>
#include <sys/sysctl.h>

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
static void  printval __P((void *, VAR *, int));
static int   titlecmp __P((char *, char **));

static void  doubleprintorsetwidth __P((VAR *, double, int, int));
static void  intprintorsetwidth __P((VAR *, int, int));
static void  strprintorsetwidth __P((VAR *, const char *, int));

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
	int len;
	VAR *v;
	struct varent *vent;
	static int firsttime = 1;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (firsttime) {
			len = strlen(v->header);
			if (len > v->width)
				v->width = len;
			totwidth += v->width + 1;	/* +1 for space */
		}
		if (v->flag & LJUST) {
			if (vent->next == NULL)	/* last one */
				(void)printf("%s", v->header);
			else
				(void)printf("%-*s", v->width,
				    v->header);
		} else
			(void)printf("%*s", v->width, v->header);
		if (vent->next != NULL)
			(void)putchar(' ');
	}
	(void)putchar('\n');
	if (firsttime) {
		firsttime = 0;
		totwidth--;	/* take off last space */
	}
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
	    title[namelen + 0] == ':' &&
	    title[namelen + 1] == ' ')
		return (0);

	return (1);
}

static void
doubleprintorsetwidth(v, val, prec, mode)
	VAR *v;
	double val;
	int prec;
	int mode;
{
	int fmtlen;

	if (mode == WIDTHMODE) {
		if (val < 0.0 && val < v->longestnd) {
			fmtlen = (int)log10(-val) + prec + 2;
			v->longestnd = val;
			if (fmtlen > v->width)
				v->width = fmtlen;
		} else if (val > 0.0 && val > v->longestpd) {
			fmtlen = (int)log10(val) + prec + 1;
			v->longestpd = val;
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
	} else {
		printf("%*.*f", v->width, prec, val);
	}
}

static void
intprintorsetwidth(v, val, mode)
	VAR *v;
	int val;
	int mode;
{
	int fmtlen;

	if (mode == WIDTHMODE) {
		if (val < 0 && val < v->longestn) {
			fmtlen = (int)log10((double)-val) + 2;
			v->longestn = val;
			if (fmtlen > v->width)
				v->width = fmtlen;
		} else if (val > 0 && val > v->longestp) {
			fmtlen = (int)log10((double)val) + 1;
			v->longestp = val;
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
	} else
		printf("%*d", v->width, val);
}

static void
strprintorsetwidth(v, str, mode)
	VAR *v;
	const char *str;
{
	int len;

	if (mode == WIDTHMODE) {
		len = strlen(str);
		if (len > v->width)
			v->width = len;
	} else {
		if (v->flag & LJUST)
			printf("%-*.*s", v->width, v->width, str);
		else
			printf("%*.*s", v->width, v->width, str);
	}
}

void
command(ki, ve, mode)
	struct kinfo_proc2 *ki;
	VARENT *ve;
	int mode;
{
	VAR *v;
	int left;
	char **argv, **p, *name;

	if (mode == WIDTHMODE)
		return;

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
ucomm(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	strprintorsetwidth(v, k->p_comm, mode);
}

void
logname(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	strprintorsetwidth(v, k->p_login, mode);
}

void
state(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
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
	if (flag & P_SYSTEM)
		*cp++ = 'K';
	/* system process might have this too, don't need to double up */
	else if (k->p_holdcnt)
		*cp++ = 'L';
	if (k->p_eflag & EPROC_SLEADER)
		*cp++ = 's';
	if ((flag & P_CONTROLT) && k->p__pgid == k->p_tpgid)
		*cp++ = '+';
	*cp = '\0';
	strprintorsetwidth(v, buf, mode);
}

void
pnice(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v, k->p_nice - NZERO, mode);
}

void
pri(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v, k->p_priority - PZERO, mode);
}

void
uname(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	strprintorsetwidth(v, user_from_uid(k->p_uid, 0), mode);
}

void
runame(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	strprintorsetwidth(v, user_from_uid(k->p_ruid, 0), mode);
}

void
tdev(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	dev_t dev;
	char buff[16];

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV) {
		/*
		 * Minimum width is width of header - we don't
		 * need to check it every time.
		 */
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "??");
	} else {
		(void)snprintf(buff, sizeof(buff),
		    "%d/%d", major(dev), minor(dev));
		strprintorsetwidth(v, buff, mode);
	}
}

void
tname(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	dev_t dev;
	const char *ttname;
	int noctty;

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL) {
		/*
		 * Minimum width is width of header - we don't
		 * need to check it every time.
		 */
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "??");
	} else {
		if (strncmp(ttname, "tty", 3) == 0 ||
		    strncmp(ttname, "dty", 3) == 0)
			ttname += 3;
		noctty = !(k->p_eflag & EPROC_CTTY) ? 1 : 0;
		if (mode == WIDTHMODE) {
			int fmtlen;

			fmtlen = strlen(ttname) + noctty;
			if (v->width < fmtlen)
				v->width = fmtlen;
		} else {
			if (noctty)
				printf("%-*s-", v->width - 1, ttname);
			else
				printf("%-*s", v->width, ttname);
		}
	}
}

void
longtname(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	dev_t dev;
	const char *ttname;

	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL) {
		/*
		 * Minimum width is width of header - we don't
		 * need to check it every time.
		 */
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "??");
	}
	else {
		strprintorsetwidth(v, ttname, mode);
	}
}

void
started(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	static time_t now;
	time_t startt;
	struct tm *tp;
	char buf[100], *cp;

	/*
	 * XXX: The maximum width of this field is the same as the header
	 *      "STARTED" for locales that have 3 letter abbreviated month
	 *      names and 2 letter am/pm descriptions.
	 */
	if (mode == WIDTHMODE)
		return;

	v = ve->var;
	if (!k->p_uvalid) {
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "-");
		return;
	}

	startt = k->p_ustart_sec;
	tp = localtime(&startt);
	if (!now)
		(void)time(&now);
	if (now - k->p_ustart_sec < SECSPERDAY)
		/* I *hate* SCCS... */
		(void)strftime(buf, sizeof(buf) - 1, "%l:%" "M%p", tp);
	else if (now - k->p_ustart_sec < DAYSPERWEEK * SECSPERDAY)
		/* I *hate* SCCS... */
		(void)strftime(buf, sizeof(buf) - 1, "%a%" "I%p", tp);
	else
		(void)strftime(buf, sizeof(buf) - 1, "%e%b%y", tp);
	/* %e and %l can start with a space. */
	cp = buf;
	if (*cp == ' ')
		cp++;
	strprintorsetwidth(v, cp, mode);
}

void
lstarted(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	time_t startt;
	char buf[100];

	v = ve->var;
	if (!k->p_uvalid) {
		/*
		 * Minimum width is less than header - we don't
		 * need to check it every time.
		 */
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "-");
		return;
	}
	startt = k->p_ustart_sec;

	/* assume all times are the same length */
	if (mode != WIDTHMODE || v->width == 0) {
		(void)strftime(buf, sizeof(buf) -1, "%c",
		    localtime(&startt));
		strprintorsetwidth(v, buf, mode);
	}
}

void
wchan(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	char *buf;

	v = ve->var;
	if (k->p_wchan) {
		if (k->p_wmesg) {
			strprintorsetwidth(v, k->p_wmesg, mode);
			v->width = min(v->width, WMESGLEN);
		} else {
			(void)asprintf(&buf, "%-*llx", v->width,
			    (long long)k->p_wchan);
			if (buf == NULL)
				err(1, "%s", "");
			strprintorsetwidth(v, buf, mode);
			v->width = min(v->width, WMESGLEN);
			free(buf);
		}
	} else {
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "-");
	}
}

#define pgtok(a)        (((a)*getpagesize())/1024)

void
vsize(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v,
	    pgtok(k->p_vm_dsize + k->p_vm_ssize + k->p_vm_tsize), mode);
}

void
rssize(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	/* XXX don't have info about shared */
	intprintorsetwidth(v, pgtok(k->p_vm_rssize), mode);
}

void
p_rssize(k, ve, mode)		/* doesn't account for text */
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v, pgtok(k->p_vm_rssize), mode);
}

void
cputime(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;
	long secs;
	long psecs;	/* "parts" of a second. first micro, then centi */
	int fmtlen;

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
	if (mode == WIDTHMODE) {
		/*
		 * Ugg, this is the only field where a value of 0 longer
		 * than the column title, and log10(0) isn't good enough.
		 * Use SECSPERMIN, because secs is divided by that when
		 * passed to log10().
		 */
		if (secs == 0 && v->longestp == 0)
			secs = SECSPERMIN;
		if (secs > v->longestp) {
			/* "+6" for the "%02ld.%02ld" in the printf() below */
			fmtlen = (int)log10((double)secs / SECSPERMIN) + 1 + 6;
			v->longestp = secs;
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
	} else {
		printf("%*ld:%02ld.%02ld", v->width - 6, secs / SECSPERMIN,
		    secs % SECSPERMIN, psecs);
	}
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
pcpu(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	doubleprintorsetwidth(v, getpcpu(k), 1, mode);
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
pmem(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	doubleprintorsetwidth(v, getpmem(k), 1, mode);
}

void
pagein(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v, k->p_uvalid ? k->p_uru_majflt : 0, mode);
}

void
maxrss(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	/* No need to check width! */
	if (mode == PRINTMODE)
		(void)printf("%*s", v->width, "-");
}

void
tsize(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	intprintorsetwidth(v, pgtok(k->p_vm_tsize), mode);
}

/*
 * Generic output routines.  Print fields from various prototype
 * structures.
 */
static void
printval(bp, v, mode)
	void *bp;
	VAR *v;
	int mode;
{
	static char ofmt[32] = "%";
	int width, vok, fmtlen;
	char *fcp, *cp, *obuf;
	enum type type;
	long long val;
	unsigned long long uval;

	/*
	 * Note that the "INF127" check is nonsensical for types
	 * that are or can be signed.
	 */
#define	GET(type)		(*(type *)bp)
#define	CHK_INF127(n)		(((n) > 127) && (v->flag & INF127) ? 127 : (n))

#define	VSIGN	1
#define	VUNSIGN	2
#define	VPTR	3

	if (mode == WIDTHMODE) {
		vok = 0;
		switch (v->type) {
		case CHAR:
			val = GET(char);
			vok = VSIGN;
			break;
		case UCHAR:
			uval = CHK_INF127(GET(u_char));
			vok = VUNSIGN;
			break;
		case SHORT:
			val = GET(short);
			vok = VSIGN;
			break;
		case USHORT:
			uval = CHK_INF127(GET(u_short));
			vok = VUNSIGN;
			break;
		case INT32:
			val = GET(int32_t);
			vok = VSIGN;
			break;
		case INT:
			val = GET(int);
			vok = VSIGN;
			break;
		case UINT:
		case UINT32:
			uval = CHK_INF127(GET(u_int));
			vok = VUNSIGN;
			break;
		case LONG:
			val = GET(long);
			vok = VSIGN;
			break;
		case ULONG:
			uval = CHK_INF127(GET(u_long));
			vok = VUNSIGN;
			break;
		case KPTR:
			uval = (unsigned long long)GET(u_int64_t);
			vok = VPTR;
			break;
		case KPTR24:
			uval = (unsigned long long)GET(u_int64_t);
			uval &= 0xffffff;
			vok = VPTR;
			break;
		default:
			/* nothing... */;
		}
		switch (vok) {
		case VSIGN:
			if (val < 0  && val < v->longestn) {
				fmtlen = (int)log10((double)-val) + 2;
				v->longestn = val;
				if (fmtlen > v->width)
					v->width = fmtlen;
			} else if (val > 0 && val > v->longestp) {
				fmtlen = (int)log10((double)val) + 1;
				v->longestp = val;
				if (fmtlen > v->width)
					v->width = fmtlen;
			}
			return;
		case VUNSIGN:
			if (uval > v->longestu) {
				fmtlen = (int)log10((double)uval) + 1;
				v->longestu = uval;
				v->width = fmtlen;
			}
			return;
		case VPTR:
			fmtlen = 0;
			while (uval > 0) {
				uval >>= 4;
				fmtlen++;
			}
			if (fmtlen > v->width)
				v->width = fmtlen;
			return;
		}
	}

	width = v->width;
	cp = ofmt + 1;
	fcp = v->fmt;
	if (v->flag & LJUST)
		*cp++ = '-';
	*cp++ = '*';
	while ((*cp++ = *fcp++) != '\0')
		continue;

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
		(void)printf(ofmt, width, GET(char));
		return;
	case UCHAR:
		(void)printf(ofmt, width, CHK_INF127(GET(u_char)));
		return;
	case SHORT:
		(void)printf(ofmt, width, GET(short));
		return;
	case USHORT:
		(void)printf(ofmt, width, CHK_INF127(GET(u_short)));
		return;
	case INT:
		(void)printf(ofmt, width, GET(int));
		return;
	case UINT:
		(void)printf(ofmt, width, CHK_INF127(GET(u_int)));
		return;
	case LONG:
		(void)printf(ofmt, width, GET(long));
		return;
	case ULONG:
		(void)printf(ofmt, width, CHK_INF127(GET(u_long)));
		return;
	case KPTR:
		(void)printf(ofmt, width, (unsigned long long)GET(u_int64_t));
		return;
	case KPTR24:
		(void)printf(ofmt, width,
		    (unsigned long long)GET(u_int64_t) & 0xffffff);
		return;
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
			(void)asprintf(&obuf, ofmt, width, &buf[i]);
		}
		break;
	default:
		errx(1, "unknown type %d", v->type);
	}
	if (obuf == NULL)
		err(1, "%s", "");
	if (mode == WIDTHMODE) {
		/* Skip leading spaces. */
		cp = strrchr(obuf, ' ');
		if (cp == NULL)
			cp = obuf;
		else
			cp++;	/* skip last space */
	}
	else
		cp = obuf;
	strprintorsetwidth(v, cp, mode);
	free(obuf);
#undef GET
#undef CHK_INF127
}

void
pvar(k, ve, mode)
	struct kinfo_proc2 *k;
	VARENT *ve;
	int mode;
{
	VAR *v;

	v = ve->var;
	printval((char *)k + v->off, v, mode);
}
