/*	$NetBSD: print.c,v 1.85 2004/03/27 12:59:25 simonb Exp $	*/

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
#if 0
static char sccsid[] = "@(#)print.c	8.6 (Berkeley) 4/16/94";
#else
__RCSID("$NetBSD: print.c,v 1.85 2004/03/27 12:59:25 simonb Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ucred.h>
#include <sys/sysctl.h>

#include <err.h>
#include <grp.h>
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

static char *cmdpart(char *);
static void  printval(void *, VAR *, int);
static int   titlecmp(char *, char **);

static void  doubleprintorsetwidth(VAR *, double, int, int);
static void  intprintorsetwidth(VAR *, int, int);
static void  strprintorsetwidth(VAR *, const char *, int);

#define	min(a,b)	((a) <= (b) ? (a) : (b))

static int
iwidth(u_int64_t v)
{
	u_int64_t nlim, lim;
	int w = 1;

	for (lim = 10; v >= lim; lim = nlim) {
		nlim = lim * 10;
		w++;
		if (nlim < lim)
			break;
	}
	return w;
}

static char *
cmdpart(char *arg0)
{
	char *cp;

	return ((cp = strrchr(arg0, '/')) != NULL ? cp + 1 : arg0);
}

void
printheader(void)
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

/*
 * Return 1 if the the command name in the argument vector (u-area) does
 * not match the command name (p_comm)
 */
static int
titlecmp(char *name, char **argv)
{
	char *title;
	int namelen;


	/* no argument vector == no match; system processes/threads do that */
	if (argv == 0 || argv[0] == 0)
		return (1);

	title = cmdpart(argv[0]);

	/* the basename matches */
	if (!strcmp(name, title))
		return (0);

	/* handle login shells, by skipping the leading - */
	if (title[0] == '-' && !strcmp(name, title + 1))
		return (0);

	namelen = strlen(name);

	/* handle daemons that report activity as daemonname: activity */
	if (argv[1] == 0 &&
	    !strncmp(name, title, namelen) &&
	    title[namelen + 0] == ':' &&
	    title[namelen + 1] == ' ')
		return (0);

	return (1);
}

static void
doubleprintorsetwidth(VAR *v, double val, int prec, int mode)
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
		(void)printf("%*.*f", v->width, prec, val);
	}
}

static void
intprintorsetwidth(VAR *v, int val, int mode)
{
	int fmtlen;

	if (mode == WIDTHMODE) {
		if (val < 0 && val < v->longestn) {
			v->longestn = val;
			fmtlen = iwidth(-val) + 1;
			if (fmtlen > v->width)
				v->width = fmtlen;
		} else if (val > 0 && val > v->longestp) {
			v->longestp = val;
			fmtlen = iwidth(val);
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
	} else
		(void)printf("%*d", v->width, val);
}

static void
strprintorsetwidth(VAR *v, const char *str, int mode)
{
	int len;

	if (mode == WIDTHMODE) {
		len = strlen(str);
		if (len > v->width)
			v->width = len;
	} else {
		if (v->flag & LJUST)
			(void)printf("%-*.*s", v->width, v->width, str);
		else
			(void)printf("%*.*s", v->width, v->width, str);
	}
}

void
command(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *ki;
	VAR *v;
	int left;
	char **argv, **p, *name;

	if (mode == WIDTHMODE)
		return;

	ki = arg;
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
			argv = kvm_getargv2(kd, ki, termwidth);
			if ((p = argv) != NULL) {
				while (*p) {
					fmt_puts(*p, &left);
					p++;
					fmt_putc(' ', &left);
				}
				if (titlecmp(name, argv)) {
					/*
					 * append the real command name within
					 * parentheses, if the command name
					 * does not match the one in the
					 * argument vector
					 */
					fmt_putc('(', &left);
					fmt_puts(name, &left);
					fmt_putc(')', &left);
				}
			} else {
				/*
				 * Commands that don't set an argv vector
				 * are printed with square brackets if they
				 * are system commands.  Otherwise they are
				 * printed within parentheses.
				 */
				if (ki->p_flag & P_SYSTEM) {
					fmt_putc('[', &left);
					fmt_puts(name, &left);
					fmt_putc(']', &left);
				} else {
					fmt_putc('(', &left);
					fmt_puts(name, &left);
					fmt_putc(')', &left);
				}
			}
		} else {
			fmt_puts(name, &left);
		}
	}
	if (ve->next && left > 0)
		(void)printf("%*s", left, "");
}

void
groups(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *ki;
	VAR *v;
	int left, i;
	char buf[16], *p;

	if (mode == WIDTHMODE)
		return;

	ki = arg;
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

	if (ki->p_ngroups == 0) {
		fmt_putc('-', &left);
		return;
	}

	for (i = 0; i < ki->p_ngroups; i++) {
		(void)snprintf(buf, sizeof(buf), "%d", ki->p_groups[i]);
		if (i)
			fmt_putc(' ', &left);
		for (p = &buf[0]; *p; p++)
			fmt_putc(*p, &left);
	}

	if (ve->next && left > 0)
		(void)printf("%*s", left, "");
}

void
groupnames(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *ki;
	VAR *v;
	int left, i;
	const char *p;

	if (mode == WIDTHMODE)
		return;

	ki = arg;
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

	if (ki->p_ngroups == 0) {
		fmt_putc('-', &left);
		return;
	}

	for (i = 0; i < ki->p_ngroups; i++) {
		if (i)
			fmt_putc(' ', &left);
		for (p = group_from_gid(ki->p_groups[i], 0); *p; p++)
			fmt_putc(*p, &left);
	}

	if (ve->next && left > 0)
		(void)printf("%*s", left, "");
}

void
ucomm(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, k->p_comm, mode);
}

void
logname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, k->p_login, mode);
}

void
state(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	int flag, is_zombie;
	char *cp;
	VAR *v;
	char buf[16];

	k = arg;
	is_zombie = 0;
	v = ve->var;
	flag = k->p_flag;
	cp = buf;

	switch (k->p_stat) {

	case LSSTOP:
		*cp = 'T';
		break;

	case LSSLEEP:
		if (flag & L_SINTR)	/* interruptable (long) */
			*cp = k->p_slptime >= maxslp ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case LSRUN:
	case LSIDL:
	case LSONPROC:
		*cp = 'R';
		break;

	case LSZOMB:
	case LSDEAD:
		*cp = 'Z';
		is_zombie = 1;
		break;

	case LSSUSPENDED:
		*cp = 'U';
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & L_INMEM) {
	} else
		*cp++ = 'W';
	if (k->p_nice < NZERO)
		*cp++ = '<';
	else if (k->p_nice > NZERO)
		*cp++ = 'N';
	if (flag & P_TRACED)
		*cp++ = 'X';
	if (flag & P_SYSTRACE)
		*cp++ = 'x';
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
	if (flag & P_SA)
		*cp++ = 'a';
	else if (k->p_nlwps > 1)
		*cp++ = 'l';
	if ((flag & P_CONTROLT) && k->p__pgid == k->p_tpgid)
		*cp++ = '+';
	*cp = '\0';
	strprintorsetwidth(v, buf, mode);
}

void
lstate(void *arg, VARENT *ve, int mode)
{
	struct kinfo_lwp *k;
	int flag, is_zombie;
	char *cp;
	VAR *v;
	char buf[16];

	k = arg;
	is_zombie = 0;
	v = ve->var;
	flag = k->l_flag;
	cp = buf;

	switch (k->l_stat) {

	case LSSTOP:
		*cp = 'T';
		break;

	case LSSLEEP:
		if (flag & L_SINTR)	/* interruptible (long) */
			*cp = k->l_slptime >= maxslp ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case LSRUN:
	case LSIDL:
	case LSONPROC:
		*cp = 'R';
		break;

	case LSZOMB:
	case LSDEAD:
		*cp = 'Z';
		is_zombie = 1;
		break;

	case LSSUSPENDED:
		*cp = 'U';
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & L_INMEM) {
	} else
		*cp++ = 'W';
	if (k->l_holdcnt)
		*cp++ = 'L';
	if (flag & L_DETACHED)
		*cp++ = '-';
	*cp = '\0';
	strprintorsetwidth(v, buf, mode);
}

void
pnice(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	intprintorsetwidth(v, k->p_nice - NZERO, mode);
}

void
pri(void *arg, VARENT *ve, int mode)
{
	struct kinfo_lwp *l;
	VAR *v;

	l = arg;
	v = ve->var;
	intprintorsetwidth(v, l->l_priority - PZERO, mode);
}

void
uname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, user_from_uid(k->p_uid, 0), mode);
}

void
runame(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, user_from_uid(k->p_ruid, 0), mode);
}

void
svuname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, user_from_uid(k->p_svuid, 0), mode);
}

void
gname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, group_from_gid(k->p_gid, 0), mode);
}

void
rgname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, group_from_gid(k->p_rgid, 0), mode);
}

void
svgname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	strprintorsetwidth(v, group_from_gid(k->p_svgid, 0), mode);
}

void
tdev(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	dev_t dev;
	char buff[16];

	k = arg;
	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV) {
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "??");
		else
			if (v->width < 2)
				v->width = 2;
	} else {
		(void)snprintf(buff, sizeof(buff),
		    "%d/%d", major(dev), minor(dev));
		strprintorsetwidth(v, buff, mode);
	}
}

void
tname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	dev_t dev;
	const char *ttname;
	int noctty;

	k = arg;
	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL) {
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "??");
		else
			if (v->width < 2)
				v->width = 2;
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
				(void)printf("%-*s-", v->width - 1, ttname);
			else
				(void)printf("%-*s", v->width, ttname);
		}
	}
}

void
longtname(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	dev_t dev;
	const char *ttname;

	k = arg;
	v = ve->var;
	dev = k->p_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL) {
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "??");
		else
			if (v->width < 2)
				v->width = 2;
	} else {
		strprintorsetwidth(v, ttname, mode);
	}
}

void
started(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	static time_t now;
	time_t startt;
	struct tm *tp;
	char buf[100], *cp;

	k = arg;
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
lstarted(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	time_t startt;
	char buf[100];

	k = arg;
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
wchan(void *arg, VARENT *ve, int mode)
{
	struct kinfo_lwp *l;
	VAR *v;
	char *buf;

	l = arg;
	v = ve->var;
	if (l->l_wchan) {
		if (l->l_wmesg) {
			strprintorsetwidth(v, l->l_wmesg, mode);
			v->width = min(v->width, KI_WMESGLEN);
		} else {
			(void)asprintf(&buf, "%-*" PRIx64, v->width,
			    l->l_wchan);
			if (buf == NULL)
				err(1, "%s", "");
			strprintorsetwidth(v, buf, mode);
			v->width = min(v->width, KI_WMESGLEN);
			free(buf);
		}
	} else {
		if (mode == PRINTMODE)
			(void)printf("%-*s", v->width, "-");
	}
}

#define pgtok(a)        (((a)*getpagesize())/1024)

void
vsize(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	intprintorsetwidth(v,
	    pgtok(k->p_vm_dsize + k->p_vm_ssize + k->p_vm_tsize), mode);
}

void
rssize(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	/* XXX don't have info about shared */
	intprintorsetwidth(v, pgtok(k->p_vm_rssize), mode);
}

void
p_rssize(void *arg, VARENT *ve, int mode)	/* doesn't account for text */
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	intprintorsetwidth(v, pgtok(k->p_vm_rssize), mode);
}

void
cputime(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;
	int32_t secs;
	int32_t psecs;	/* "parts" of a second. first micro, then centi */
	int fmtlen;

	k = arg;
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
		 * Ugg, this is the only field where a value of 0 is longer
		 * than the column title.
		 * Use SECSPERMIN, because secs is divided by that when
		 * passed to iwidth().
		 */
		if (secs == 0)
			secs = SECSPERMIN;

		if (secs > v->longestp) {
			v->longestp = secs;
			/* "+6" for the ":%02ld.%02ld" in the printf() below */
			fmtlen = iwidth(secs / SECSPERMIN) + 6;
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
	} else {
		(void)printf("%*ld:%02ld.%02ld", v->width - 6,
		    (long)(secs / SECSPERMIN), (long)(secs % SECSPERMIN),
		    (long)psecs);
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
	if (k->p_swtime == 0 || (k->p_flag & L_INMEM) == 0 ||
	    k->p_stat == SZOMB)
		return (0.0);
	if (rawcpu)
		return (100.0 * fxtofl(k->p_pctcpu));
	return (100.0 * fxtofl(k->p_pctcpu) /
		(1.0 - exp(k->p_swtime * log(ccpu))));
}

void
pcpu(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
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

	if ((k->p_flag & L_INMEM) == 0)
		return (0.0);
	/* XXX want pmap ptpages, segtab, etc. (per architecture) */
	szptudot = uspace/getpagesize();
	/* XXX don't have info about shared */
	fracmem = ((float)k->p_vm_rssize + szptudot)/mempages;
	return (100.0 * fracmem);
}

void
pmem(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	doubleprintorsetwidth(v, getpmem(k), 1, mode);
}

void
pagein(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
	v = ve->var;
	intprintorsetwidth(v, k->p_uvalid ? k->p_uru_majflt : 0, mode);
}

void
maxrss(void *arg, VARENT *ve, int mode)
{
	VAR *v;

	v = ve->var;
	/* No need to check width! */
	if (mode == PRINTMODE)
		(void)printf("%*s", v->width, "-");
}

void
tsize(void *arg, VARENT *ve, int mode)
{
	struct kinfo_proc2 *k;
	VAR *v;

	k = arg;
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
	char *fcp, *cp;
	int64_t val;
	u_int64_t uval;

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
			uval = GET(u_int64_t);
			vok = VPTR;
			break;
		case KPTR24:
			uval = GET(u_int64_t);
			uval &= 0xffffff;
			vok = VPTR;
			break;
		case INT64:
			val = GET(int64_t);
			vok = VSIGN;
			break;
		case UINT64:
			uval = CHK_INF127(GET(u_int64_t));
			vok = VUNSIGN;
			break;

		case SIGLIST:
		default:
			/* nothing... */;
		}
		switch (vok) {
		case VSIGN:
			if (val < 0 && val < v->longestn) {
				v->longestn = val;
				fmtlen = iwidth(-val) + 1;
				if (fmtlen > v->width)
					v->width = fmtlen;
			} else if (val > 0 && val > v->longestp) {
				v->longestp = val;
				fmtlen = iwidth(val);
				if (fmtlen > v->width)
					v->width = fmtlen;
			}
			return;
		case VUNSIGN:
			if (uval > v->longestu) {
				v->longestu = uval;
				v->width = iwidth(uval);
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
		(void)printf(ofmt, width, GET(u_int64_t));
		return;
	case KPTR24:
		(void)printf(ofmt, width, GET(u_int64_t) & 0xffffff);
		return;
	case INT32:
		(void)printf(ofmt, width, GET(int32_t));
		return;
	case UINT32:
		(void)printf(ofmt, width, CHK_INF127(GET(u_int32_t)));
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
			for (i = 0; buf[i] == '0'; i++)
				continue;

			if (buf[i] == '\0')
				i--;
			strprintorsetwidth(v, buf + i, mode);
#undef SIGSETSIZE
		}
		return;
	case INT64:
		(void)printf(ofmt, width, GET(int64_t));
		return;
	case UINT64:
		(void)printf(ofmt, width, CHK_INF127(GET(u_int64_t)));
		return;
	default:
		errx(1, "unknown type %d", v->type);
	}
#undef GET
#undef CHK_INF127
}

void
pvar(void *arg, VARENT *ve, int mode)
{
	VAR *v;

	v = ve->var;
	if (v->flag & UAREA && !((struct kinfo_proc2 *)arg)->p_uvalid) {
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "-");
		return;
	}

	(void)printval((char *)arg + v->off, v, mode);
}

void
putimeval(void *arg, VARENT *ve, int mode)
{
	VAR *v = ve->var;
	struct kinfo_proc2 *k = arg;
	ulong secs = *(uint32_t *)((char *)arg + v->off);
	ulong usec = *(uint32_t *)((char *)arg + v->off + sizeof (uint32_t));
	int fmtlen;

	if (!k->p_uvalid) {
		if (mode == PRINTMODE)
			(void)printf("%*s", v->width, "-");
		return;
	}

	if (mode == WIDTHMODE) {
		if (!secs)
			/* zero doesn't give correct width... */
			secs = 1;
		if (secs > v->longestu) {
			v->longestu = secs;
			if (secs <= 999)
				/* sss.ssssss */
				fmtlen = iwidth(secs) + 6 + 1;
			else
				/* hh:mm:ss.ss */
				fmtlen = iwidth((secs + 1) / SECSPERHOUR)
					+ 2 + 1 + 2 + 1 + 2 + 1;
			if (fmtlen > v->width)
				v->width = fmtlen;
		}
		return;
	}

	if (secs < 999)
		(void)printf( "%*lu.%.6lu", v->width - 6 - 1, secs, usec);
	else {
		uint h, m;
		usec += 5000;
		if (usec >= 1000000) {
			usec -= 1000000;
			secs++;
		}
		m = secs / SECSPERMIN;
		secs -= m * SECSPERMIN;
		h = m / MINSPERHOUR;
		m -= h * MINSPERHOUR;
		(void)printf( "%*u:%.2u:%.2lu.%.2lu", v->width - 9, h, m, secs,
		    usec / 10000u );
	}
}
