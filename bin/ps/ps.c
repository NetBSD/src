/*	$NetBSD: ps.c,v 1.46.2.2 2002/04/23 20:41:14 nathanw Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ps.c	8.4 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: ps.c,v 1.46.2.2 2002/04/23 20:41:14 nathanw Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ps.h"

/*
 * ARGOPTS must contain all option characters that take arguments
 * (except for 't'!) - it is used in kludge_oldps_options()
 */
#define	GETOPTSTR	"acCeghjLlM:mN:O:o:p:rSsTt:U:uvW:wx"
#define	ARGOPTS		"MNOopUW"

struct kinfo_proc2 *kinfo;
struct varent *vhead, *vtail;

int	eval;			/* exit value */
int	rawcpu;			/* -C */
int	sumrusage;		/* -S */
int	termwidth;		/* width of screen (0 == infinity) */
int	totwidth;		/* calculated width of requested variables */

int	needcomm, needenv, commandonly;
uid_t	myuid;

enum sort { DEFAULT, SORTMEM, SORTCPU } sortby = DEFAULT;

static struct kinfo_lwp
		*pick_representative_lwp __P((struct kinfo_proc2 *, 
		    struct kinfo_lwp *, int));
static struct kinfo_proc2
		*getkinfo_kvm __P((kvm_t *, int, int, int *));
static char	*kludge_oldps_options __P((char *));
static int	 pscomp __P((const void *, const void *));
static void	 scanvars __P((void));
static void	 usage __P((void));
int		 main __P((int, char *[]));

char dfmt[] = "pid tt state time command";
char jfmt[] = "user pid ppid pgid sess jobc state tt time command";
char lfmt[] = "uid pid ppid cpu pri nice vsz rss wchan state tt time command";
char   o1[] = "pid";
char   o2[] = "tt state time command";
char sfmt[] = "uid pid ppid cpu lid nlwp pri nice vsz rss wchan state tt time command";
char ufmt[] = "user pid %cpu %mem vsz rss tt state start time command";
char vfmt[] = "pid state time sl re pagein vsz rss lim tsiz %cpu %mem command";

kvm_t *kd;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct varent *vent;
	struct winsize ws;
	struct kinfo_lwp *kl, *l;
	int ch, flag, i, j, fmt, lineno, nentries, nlwps;
	int prtheader, wflag, what, xflg, mode, showlwps;
	char *nlistf, *memf, *swapf, errbuf[_POSIX2_LINE_MAX];
	char *ttname;

	if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDERR_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDIN_FILENO,  TIOCGWINSZ, (char *)&ws) == -1) ||
	     ws.ws_col == 0)
		termwidth = 79;
	else
		termwidth = ws.ws_col - 1;

	if (argc > 1)
		argv[1] = kludge_oldps_options(argv[1]);

	fmt = prtheader = wflag = xflg = showlwps = 0;
	what = KERN_PROC_UID;
	flag = myuid = getuid();
	memf = nlistf = swapf = NULL;
	mode = PRINTMODE;
	while ((ch = getopt(argc, argv, GETOPTSTR)) != -1)
		switch((char)ch) {
		case 'a':
			what = KERN_PROC_ALL;
			flag = 0;
			break;
		case 'c':
			commandonly = 1;
			break;
		case 'e':			/* XXX set ufmt */
			needenv = 1;
			break;
		case 'C':
			rawcpu = 1;
			break;
		case 'g':
			break;			/* no-op */
		case 'h':
			prtheader = ws.ws_row > 5 ? ws.ws_row : 22;
			break;
		case 'j':
			parsefmt(jfmt);
			fmt = 1;
			jfmt[0] = '\0';
			break;
		case 'L':
			showkey();
			exit(0);
			/* NOTREACHED */
		case 'l':
			parsefmt(lfmt);
			fmt = 1;
			lfmt[0] = '\0';
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			sortby = SORTMEM;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'O':
			parsefmt(o1);
			parsefmt(optarg);
			parsefmt(o2);
			o1[0] = o2[0] = '\0';
			fmt = 1;
			break;
		case 'o':
			parsefmt(optarg);
			fmt = 1;
			break;
		case 'p':
			what = KERN_PROC_PID;
			flag = atol(optarg);
			xflg = 1;
			break;
		case 'r':
			sortby = SORTCPU;
			break;
		case 'S':
			sumrusage = 1;
			break;
		case 's':
			/* -L was already taken... */
			showlwps = 1;
			parsefmt(sfmt);
			fmt = 1;
			sfmt[0] = '\0';
			break;
		case 'T':
			if ((ttname = ttyname(STDIN_FILENO)) == NULL)
				errx(1, "stdin: not a terminal");
			goto tty;
		case 't':
			ttname = optarg;
		tty: {
			struct stat sb;
			char *ttypath, pathbuf[MAXPATHLEN];

			flag = 0;
			if (strcmp(ttname, "?") == 0)
				flag = KERN_PROC_TTY_NODEV;
			else if (strcmp(ttname, "-") == 0)
				flag = KERN_PROC_TTY_REVOKE;
			else if (strcmp(ttname, "co") == 0)
				ttypath = _PATH_CONSOLE;
			else if (*ttname != '/')
				(void)snprintf(ttypath = pathbuf,
				    sizeof(pathbuf), "%s%s", _PATH_TTY, ttname);
			else
				ttypath = ttname;
			what = KERN_PROC_TTY;
			if (flag == 0) {
				if (stat(ttypath, &sb) == -1)
					err(1, "%s", ttypath);
				if (!S_ISCHR(sb.st_mode))
					errx(1, "%s: not a terminal", ttypath);
				flag = sb.st_rdev;
			}
			break;
		}
		case 'U':
			if (*optarg != '\0') {
				struct passwd *pw;
				char *ep;

				what = KERN_PROC_UID;
				pw = getpwnam(optarg);
				if (pw == NULL) {
					errno = 0;
					flag = strtoul(optarg, &ep, 10);
					if (errno)
						err(1, "%s", optarg);
					if (*ep != '\0')
						errx(1, "%s: illegal user name",
						    optarg);
				} else
					flag = pw->pw_uid;
			}
			break;
		case 'u':
			parsefmt(ufmt);
			sortby = SORTCPU;
			fmt = 1;
			ufmt[0] = '\0';
			break;
		case 'v':
			parsefmt(vfmt);
			sortby = SORTMEM;
			fmt = 1;
			vfmt[0] = '\0';
			break;
		case 'W':
			swapf = optarg;
			break;
		case 'w':
			if (wflag)
				termwidth = UNLIMITED;
			else if (termwidth < 131)
				termwidth = 131;
			wflag++;
			break;
		case 'x':
			xflg = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		nlistf = *argv;
		if (*++argv) {
			memf = *argv;
			if (*++argv)
				swapf = *argv;
		}
	}
#endif

	if (memf == NULL && swapf == NULL) {
		kd = kvm_openfiles(nlistf, memf, swapf, KVM_NO_FILES, errbuf);
		donlist_sysctl();
	} else
		kd = kvm_openfiles(nlistf, memf, swapf, O_RDONLY, errbuf);
		
	if (kd == 0)
		errx(1, "%s", errbuf);

	if (!fmt)
		parsefmt(dfmt);

	/*
	 * scan requested variables, noting what structures are needed.
	 */
	scanvars();

	/*
	 * select procs
	 */
	if (!(kinfo = getkinfo_kvm(kd, what, flag, &nentries)))
		err(1, "%s.", kvm_geterr(kd));

	if (nentries == 0) {
		printheader();
		exit(1);
	}
	/*
	 * sort proc list
	 */
	qsort(kinfo, nentries, sizeof(struct kinfo_proc2), pscomp);
	/*
	 * For each proc, call each variable output function in
	 * "setwidth" mode to determine the widest element of
	 * the column.
	 */
	if (mode == PRINTMODE)
		for (i = 0; i < nentries; i++) {
			struct kinfo_proc2 *ki = &kinfo[i];

			if (xflg == 0 && (ki->p_tdev == NODEV ||
			    (ki->p_flag & P_CONTROLT) == 0))
				continue;

			kl = kvm_getlwps(kd, ki->p_pid, ki->p_paddr,
			    sizeof(struct kinfo_lwp), &nlwps);
			if (showlwps == 0) {
				l = pick_representative_lwp(ki, kl, nlwps);
				for (vent = vhead; vent; vent = vent->next)
					OUTPUT(vent, ki, l, WIDTHMODE);
			} else {
				/* The printing is done with the loops
				 * reversed, but here we don't need that,
				 * and this improves the code locality a bit.
				 */
				for (vent = vhead; vent; vent = vent->next)
					for (j = 0; j < nlwps; j++)
						OUTPUT(vent, ki, &kl[j], 
						    WIDTHMODE);
			}
			free(kl);
		}
	/*
	 * Print header - AFTER determining process field widths.
	 * printheader() also adds up the total width of all
	 * fields the first time it's called.
	 */
	printheader();
	/*
	 * For each proc, call each variable output function in
	 * print mode.
	 */
	for (i = lineno = 0; i < nentries; i++) {
		struct kinfo_proc2 *ki = &kinfo[i];

		if (xflg == 0 && (ki->p_tdev == NODEV ||
		    (ki->p_flag & P_CONTROLT ) == 0))
			continue;
		kl = kvm_getlwps(kd, ki->p_pid, (u_long)ki->p_paddr,
		    sizeof(struct kinfo_lwp), &nlwps);

		if (showlwps == 0) {
			l = pick_representative_lwp(ki, kl, nlwps);
			for (vent = vhead; vent; vent = vent->next) {
				OUTPUT(vent, ki, l, mode);
				if (vent->next != NULL)
					(void)putchar(' ');
			}
			(void)putchar('\n');
			if (prtheader && lineno++ == prtheader - 4) {
				(void)putchar('\n');
				printheader();
				lineno = 0;
			}
		} else {
			for (j = 0; j < nlwps; j++) {
				for (vent = vhead; vent; vent = vent->next) {
					OUTPUT(vent, ki, &kl[j], mode);
					if (vent->next != NULL)
						(void)putchar(' ');
				}
				(void)putchar('\n');
				if (prtheader && lineno++ == prtheader - 4) {
					(void)putchar('\n');
					printheader();
					lineno = 0;
				}
			}
		}
		free(kl);
	}
	exit(eval);
	/* NOTREACHED */
}

static struct kinfo_lwp *
pick_representative_lwp(ki, kl, nlwps)
	struct kinfo_proc2 *ki;
	struct kinfo_lwp *kl;
	int nlwps;
{
	int i, onproc, run, sleep;

	/* Trivial case: only one LWP */
	if (nlwps == 1)
		return kl;

	switch (ki->p_stat) {
	case SSTOP:
		/* Pick the first stopped LWP */
		for (i = 0; i < nlwps; i++) {
			if (kl[i].l_stat == LSSTOP)
				return &kl[i];
		}
		break;
	case SACTIVE:
		/* Pick the most live LWP */
		onproc = run = sleep = 0;
		for (i = 0; i < nlwps; i++) {
			switch (kl[i].l_stat) {
			case LSONPROC:
				onproc = i;
				break;
			case LSRUN:
				run = i;
				break;
			case LSSLEEP:
				sleep = i;
				break;
			}
		}
		if (onproc)
			return &kl[onproc];
		if (run)
			return &kl[run];
		if (sleep)
			return &kl[sleep];
		break;
	case SDEAD:
	case SZOMB:
		/* First will do */
		return kl;
		break;
	}
	/* Error condition! */
	warnx("Inconsistent LWP state for process %d\n", ki->p_pid);
	return kl;
}
	 

static struct kinfo_proc2 *
getkinfo_kvm(kdp, what, flag, nentriesp)
	kvm_t *kdp;
	int what, flag, *nentriesp;
{
	return (kvm_getproc2(kdp, what, flag, sizeof(struct kinfo_proc2),
	    nentriesp));
}

static void
scanvars()
{
	struct varent *vent;
	VAR *v;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (v->flag & COMM) {
			needcomm = 1;
			break;
		}
	}
}

static int
pscomp(a, b)
	const void *a, *b;
{
	struct kinfo_proc2 *ka = (struct kinfo_proc2 *)a;
	struct kinfo_proc2 *kb = (struct kinfo_proc2 *)b;

	int i;
#define VSIZE(k) (k->p_vm_dsize + k->p_vm_ssize + k->p_vm_tsize)

	if (sortby == SORTCPU)
		return (getpcpu(kb) - getpcpu(ka));
	if (sortby == SORTMEM)
		return (VSIZE(kb) - VSIZE(ka));
	i =  ka->p_tdev - kb->p_tdev;

	if (i == 0)
		i = ka->p_pid - kb->p_pid;
	return (i);
}

/*
 * ICK (all for getopt), would rather hide the ugliness
 * here than taint the main code.
 *
 *  ps foo -> ps -foo
 *  ps 34 -> ps -p34
 *
 * The old convention that 't' with no trailing tty arg means the user's
 * tty, is only supported if argv[1] doesn't begin with a '-'.  This same
 * feature is available with the option 'T', which takes no argument.
 */
static char *
kludge_oldps_options(s)
	char *s;
{
	size_t len;
	char *newopts, *ns, *cp;

	len = strlen(s);
	if ((newopts = ns = malloc(len + 3)) == NULL)
		err(1, NULL);
	/*
	 * options begin with '-'
	 */
	if (*s != '-')
		*ns++ = '-';	/* add option flag */
	/*
	 * gaze to end of argv[1]
	 */
	cp = s + len - 1;
	/*
	 * if the last letter is a 't' flag and there are no other option
	 * characters that take arguments (eg U, p, o) in the option
	 * string and the option string doesn't start with a '-' then
	 * convert to 'T' (meaning *this* terminal, i.e. ttyname(0)).
	 */
	if (*cp == 't' && *s != '-' && strpbrk(s, ARGOPTS) == NULL)
		*cp = 'T';
	else {
		/*
		 * otherwise check for trailing number, which *may* be a
		 * pid.
		 */
		while (cp >= s && isdigit(*cp))
			--cp;
	}
	cp++;
	memmove(ns, s, (size_t)(cp - s));	/* copy up to trailing number */
	ns += cp - s;
	/*
	 * if there's a trailing number, and not a preceding 'p' (pid) or
	 * 't' (tty) flag, then assume it's a pid and insert a 'p' flag.
	 */
	if (isdigit(*cp) &&
	    (cp == s || (cp[-1] != 'U' && cp[-1] != 't' && cp[-1] != 'p' &&
	     (cp - 1 == s || cp[-2] != 't'))))
		*ns++ = 'p';
	/* and append the number */
	(void)strcpy(ns, cp);		/* XXX strcpy is safe here */

	return (newopts);
}

static void
usage()
{

	(void)fprintf(stderr,
	    "usage:\t%s\n\t   %s\n\t%s\n",
	    "ps [-acCehjKlmrSTuvwx] [-O|o fmt] [-p pid] [-t tty]",
	    "[-M core] [-N system] [-W swap] [-U username]",
	    "ps [-L]");
	exit(1);
	/* NOTREACHED */
}
