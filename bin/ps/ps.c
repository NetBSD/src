/*	$NetBSD: ps.c,v 1.28.4.1 1999/12/27 18:27:11 wrstuden Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ps.c	8.4 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: ps.c,v 1.28.4.1 1999/12/27 18:27:11 wrstuden Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
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

#ifdef P_PPWAIT
#define NEWVM
#endif

KINFO *kinfo;
struct varent *vhead, *vtail;

int	eval;			/* exit value */
int	rawcpu;			/* -C */
int	sumrusage;		/* -S */
int	dontuseprocfs=0;	/* -K */
int	termwidth;		/* width of screen (0 == infinity) */
int	totwidth;		/* calculated width of requested variables */

int	needuser, needcomm, needenv, commandonly, use_procfs;
uid_t	myuid;

enum sort { DEFAULT, SORTMEM, SORTCPU } sortby = DEFAULT;

static KINFO	*getkinfo_kvm __P((kvm_t *, int, int, int *, int));
static char	*kludge_oldps_options __P((char *));
static int	 pscomp __P((const void *, const void *));
static void	 saveuser __P((KINFO *));
static void	 scanvars __P((void));
static void	 usage __P((void));
int		 main __P((int, char *[]));

char dfmt[] = "pid tt state time command";
char jfmt[] = "user pid ppid pgid sess jobc state tt time command";
char lfmt[] = "uid pid ppid cpu pri nice vsz rss wchan state tt time command";
char   o1[] = "pid";
char   o2[] = "tt state time command";
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
	gid_t egid = getegid();
	int ch, flag, i, fmt, lineno, nentries;
	int prtheader, wflag, what, xflg;
	char *nlistf, *memf, *swapf, errbuf[_POSIX2_LINE_MAX];
	char *ttname;

	(void)setegid(getgid());
	if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDERR_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDIN_FILENO,  TIOCGWINSZ, (char *)&ws) == -1) ||
	     ws.ws_col == 0)
		termwidth = 79;
	else
		termwidth = ws.ws_col - 1;

	if (argc > 1)
		argv[1] = kludge_oldps_options(argv[1]);

	fmt = prtheader = wflag = xflg = 0;
	what = KERN_PROC_UID;
	flag = myuid = getuid();
	memf = nlistf = swapf = NULL;
	while ((ch = getopt(argc, argv,
	    "acCeghjKLlM:mN:O:o:p:rSTt:U:uvW:wx")) != -1)
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
		case 'K':
			dontuseprocfs=1;
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
		case 'T':
			if ((ttname = ttyname(STDIN_FILENO)) == NULL)
				errx(1, "stdin: not a terminal");
			goto tty;
		case 't':
			ttname = optarg;
		tty: {
			struct stat sb;
			char *ttypath, pathbuf[MAXPATHLEN];

			if (strcmp(ttname, "co") == 0)
				ttypath = _PATH_CONSOLE;
			else if (*ttname != '/')
				(void)snprintf(ttypath = pathbuf,
				    sizeof(pathbuf), "%s%s", _PATH_TTY, ttname);
			else
				ttypath = ttname;
			if (stat(ttypath, &sb) == -1)
				err(1, "%s", ttypath);
			if (!S_ISCHR(sb.st_mode))
				errx(1, "%s: not a terminal", ttypath);
			what = KERN_PROC_TTY;
			flag = sb.st_rdev;
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
	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 */
	if (nlistf != NULL || memf != NULL || swapf != NULL)
		(void)setgid(getgid());
	else
		(void)setegid(egid);

	kd = kvm_openfiles(nlistf, memf, swapf, O_RDONLY, errbuf);
	if (kd == 0) {
		if (dontuseprocfs)
			errx(1, "%s", errbuf);
		else {
			warnx("kvm_openfiles: %s", errbuf);
			fprintf(stderr, "ps: falling back to /proc-based lookup\n");
		}
	}

	if (nlistf == NULL && memf == NULL && swapf == NULL)
		(void)setgid(getgid());

	if (!fmt)
		parsefmt(dfmt);

	/*
	 * scan requested variables, noting what structures are needed,
	 * and adjusting header widths as appropiate.
	 */
	scanvars();

	/*
	 * select procs
	 */
	if (!kd || !(kinfo = getkinfo_kvm(kd, what, flag, &nentries, needuser)))
	{
		/*  If/when the /proc-based code is ripped out
		 *  again, make sure all references to the -K
		 *  option are also pulled (getopt(), usage(),
		 *  man page).  See the man page comments about
		 *  this for more details.  */
	  	/*  sysctl() ought to provide some sort of
		 *  always-working-but-minimal-functionality
		 *  method of providing at least some of the
		 *  process information.  Unfortunately, such a
		 *  change will require too much work to be put
		 *  into 1.4.  For now, enable this experimental
		 *  /proc-based support instead (if /proc is
		 *  mounted) to grab as much information as we can.  
		 *  The guts of emulating kvm_getprocs() is in
		 *  the file procfs_ops.c.  */
		if (kd)
			warnx("%s.", kvm_geterr(kd));
		if (dontuseprocfs) {
			exit(1);
		}
		/*  procfs_getprocs supports all but the
		 *  KERN_PROC_RUID flag.  */
		kinfo = getkinfo_procfs(what, flag, &nentries);
		if (kinfo == 0) {
		  errx(1, "fallback /proc-based lookup also failed.  %s",
				  "Giving up...");
		}
		fprintf(stderr, "%s%s",
		    "Warning:  /proc does not provide ",
		    "valid data for all fields.\n");
		use_procfs = 1;
	}

	/*
	 * print header
	 */
	printheader();
	if (nentries == 0)
		exit(0);
	/*
	 * sort proc list
	 */
	qsort(kinfo, nentries, sizeof(KINFO), pscomp);
	/*
	 * for each proc, call each variable output function.
	 */
	for (i = lineno = 0; i < nentries; i++) {
		KINFO *ki = &kinfo[i];

		if (xflg == 0 && (KI_EPROC(ki)->e_tdev == NODEV ||
		    (KI_PROC(ki)->p_flag & P_CONTROLT ) == 0))
			continue;
		for (vent = vhead; vent; vent = vent->next) {
			(vent->var->oproc)(ki, vent);
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
	exit(eval);
	/* NOTREACHED */
}

static KINFO *
getkinfo_kvm(kd, what, flag, nentriesp, needuser)
	kvm_t *kd;
	int what, flag, *nentriesp, needuser;
{
	struct kinfo_proc *kp;
	KINFO *kinfo=NULL;
	size_t i;

	if ((kp = kvm_getprocs(kd, what, flag, nentriesp)) != 0)
	{
		if ((kinfo = malloc((*nentriesp) * sizeof(*kinfo))) == NULL)
			err(1, NULL);
		for (i = (*nentriesp); i-- > 0; kp++) {
			kinfo[i].ki_p = kp;
			if (needuser)
				saveuser(&kinfo[i]);
		}
	}

	return (kinfo);
}

static void
scanvars()
{
	struct varent *vent;
	VAR *v;
	int i;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		i = strlen(v->header);
		if (v->width < i)
			v->width = i;
		totwidth += v->width + 1;	/* +1 for space */
		if (v->flag & USER)
			needuser = 1;
		if (v->flag & COMM)
			needcomm = 1;
	}
	totwidth--;
}

static void
saveuser(ki)
	KINFO *ki;
{
	struct pstats pstats;
	struct usave *usp;

	usp = &ki->ki_u;
	if (kvm_read(kd, (u_long)&KI_PROC(ki)->p_addr->u_stats,
	    (char *)&pstats, sizeof(pstats)) == sizeof(pstats)) {
		/*
		 * The u-area might be swapped out, and we can't get
		 * at it because we have a crashdump and no swap.
		 * If it's here fill in these fields, otherwise, just
		 * leave them 0.
		 */
		usp->u_start = pstats.p_start;
		usp->u_ru = pstats.p_ru;
		usp->u_cru = pstats.p_cru;
		usp->u_valid = 1;
	} else
		usp->u_valid = 0;
}

static int
pscomp(a, b)
	const void *a, *b;
{
	int i;
#ifdef NEWVM
#define VSIZE(k) (KI_EPROC(k)->e_vm.vm_dsize + KI_EPROC(k)->e_vm.vm_ssize + \
		  KI_EPROC(k)->e_vm.vm_tsize)
#else
#define VSIZE(k) ((k)->ki_p->p_dsize + (k)->ki_p->p_ssize + (k)->ki_e->e_xsize)
#endif

	if (sortby == SORTCPU)
		return (getpcpu((KINFO *)b) - getpcpu((KINFO *)a));
	if (sortby == SORTMEM)
		return (VSIZE((KINFO *)b) - VSIZE((KINFO *)a));
	i =  KI_EPROC((KINFO *)a)->e_tdev - KI_EPROC((KINFO *)b)->e_tdev;
	if (i == 0)
		i = KI_PROC((KINFO *)a)->p_pid - KI_PROC((KINFO *)b)->p_pid;
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
	 * if last letter is a 't' flag with no argument (in the context
	 * of the oldps options -- option string NOT starting with a '-' --
	 * then convert to 'T' (meaning *this* terminal, i.e. ttyname(0)).
	 */
	if (*cp == 't' && *s != '-')
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
	(void)strcpy(ns, cp);		/* XXX strcpy is safe */

	return (newopts);
}

static void
usage()
{

	(void)fprintf(stderr,
	    "usage:\t%s\n\t   %s\n\t%s\n",
	    "ps [-aChjKlmrSTuvwx] [-O|o fmt] [-p pid] [-t tty]",
	    "[-M core] [-N system] [-W swap] [-U username]",
	    "ps [-L]");
	exit(1);
	/* NOTREACHED */
}
