/*	$NetBSD: keyword.c,v 1.26.2.1 2002/04/23 20:41:12 nathanw Exp $	*/

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
static char sccsid[] = "@(#)keyword.c	8.5 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: keyword.c,v 1.26.2.1 2002/04/23 20:41:12 nathanw Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "ps.h"

static VAR *findvar __P((char *));
static int  vcmp __P((const void *, const void *));

#ifdef NOTINUSE
	{"stime", "STIME", NULL, 0, cputime},	/* XXX add stime, utime ... */
	{"utime", "UTIME", NULL, 0, cputime},	/* ... display to cputime() */
	{"idrss", "IDRSS", NULL, 0, pvar, 0, POFF(p_uru_idrss), ULONG, "d"},
	{"isrss", "ISRSS", NULL, 0, pvar, 0, POFF(p_uru_isrss), ULONG, "d"},
	{"ixrss", "IXRSS", NULL, 0, pvar, 0, POFF(p_uru_ixrss), ULONG, "d"},
#endif

/* Compute offset in common structures. */
#define	POFF(x)	offsetof(struct kinfo_proc2, x)
#define	LOFF(x)	offsetof(struct kinfo_lwp, x)

#define	UIDFMT	"u"
#define	UID(n1, n2, fn, off) \
	{ n1, n2, NULL, 0, fn, 0, off, UINT32, UIDFMT }
#define	GID(n1, n2, fn, off)	UID(n1, n2, fn, off)

#define	PIDFMT	"d"
#define	PID(n1, n2, fn, off) \
	{ n1, n2, NULL, 0, fn, 0, off, INT32, PIDFMT }

VAR var[] = {
	{"%cpu", "%CPU", NULL, 0, pcpu},
	{"%mem", "%MEM", NULL, 0, pmem},
	{"acflag", "ACFLG", NULL, 0, pvar, 0, POFF(p_acflag), USHORT, "x"},
	{"acflg", "", "acflag"},
	{"blocked", "", "sigmask"},
	{"caught", "", "sigcatch"},
	{"command", "COMMAND", NULL, COMM|LJUST, command},
	{"cpu", "CPU", NULL, 0, pvar, 0, POFF(p_estcpu), UINT, "d"},
	{"cputime", "", "time"},
	{"f", "F", NULL, 0, pvar, 0, POFF(p_flag), INT, "x"},
	{"flags", "", "f"},
	{"holdcnt", "HOLDCNT", NULL, LWP, pvar, 0, LOFF(l_holdcnt), INT, "d"},
	{"ignored", "", "sigignore"},
	{"inblk", "INBLK", NULL, 0, pvar, 0, POFF(p_uru_inblock), ULONG, "d"},
	{"inblock", "", "inblk"},
	{"jobc", "JOBC", NULL, 0, pvar, 0, POFF(p_jobc), SHORT, "d"},
	{"ktrace", "KTRACE", NULL, 0, pvar, 0, POFF(p_traceflag), INT, "x"},
	/* XXX */
	{"ktracep", "KTRACEP", NULL, 0, pvar, 0, POFF(p_tracep), KPTR, "llx"},
	{"lid", "LID", NULL, LWP, pvar, 0, LOFF(l_lid), ULONG, "d"},
	{"lim", "LIM", NULL, 0, maxrss},
	{"login", "LOGIN", NULL, LJUST, logname},
	{"logname", "", "login"},
	{"lstart", "STARTED", NULL, LJUST, lstarted},
	{"majflt", "MAJFLT", NULL, 0, pvar, 0, POFF(p_uru_majflt), ULONG, "d"},
	{"minflt", "MINFLT", NULL, 0, pvar, 0, POFF(p_uru_minflt), ULONG, "d"},
	{"msgrcv", "MSGRCV", NULL, 0, pvar, 0, POFF(p_uru_msgrcv), ULONG, "d"},
	{"msgsnd", "MSGSND", NULL, 0, pvar, 0, POFF(p_uru_msgsnd), ULONG, "d"},
	{"ni", "", "nice"},
	{"nice", "NI", NULL, 0, pnice},
	{"nivcsw", "NIVCSW", NULL, 0, pvar, 0, POFF(p_uru_nivcsw), ULONG, "d"},
	{"nlwp", "NLWP", NULL, 0, pvar, 0, POFF(p_nlwps), ULONG, "d"},
	{"nsignals", "", "nsigs"},
	{"nsigs", "NSIGS", NULL, 0, pvar, 0, POFF(p_uru_nsignals), ULONG, "d"},
	{"nswap", "NSWAP", NULL, 0, pvar, 0, POFF(p_uru_nswap), ULONG, "d"},
	{"nvcsw", "NVCSW", NULL, 0, pvar, 0, POFF(p_uru_nvcsw), ULONG, "d"},
	/* XXX */
	{"nwchan", "WCHAN", NULL, LWP, pvar, 0, LOFF(l_wchan), KPTR, "llx"},
	{"oublk", "OUBLK", NULL, 0, pvar, 0, POFF(p_uru_oublock), ULONG, "d"},
	{"oublock", "", "oublk"},
	/* XXX */
	{"p_ru", "P_RU", NULL, 0, pvar, 0, POFF(p_ru), KPTR, "llx"},
	/* XXX */
	{"paddr", "PADDR", NULL, 0, pvar, 0, POFF(p_paddr), KPTR, "llx"},
	{"pagein", "PAGEIN", NULL, 0, pagein},
	{"pcpu", "", "%cpu"},
	{"pending", "", "sig"},
	PID("pgid", "PGID", pvar, POFF(p__pgid)),
	PID("pid", "PID", pvar, POFF(p_pid)),
	{"pmem", "", "%mem"},
	PID("ppid", "PPID", pvar, POFF(p_ppid)),
	{"pri", "PRI", NULL, LWP, pri},
	{"re", "RE", NULL, INF127|LWP, pvar, 0, LOFF(l_swtime), UINT, "d"},
	GID("rgid", "RGID", pvar, POFF(p_rgid)),
	/* XXX */
	{"rlink", "RLINK", NULL, LWP, pvar, 0, LOFF(l_back), KPTR, "llx"},
	{"rlwp", "RLWP", NULL, 0, pvar, 0, POFF(p_nrlwps), ULONG, "d"},
	{"rss", "RSS", NULL, 0, p_rssize},
	{"rssize", "", "rsz"},
	{"rsz", "RSZ", NULL, 0, rssize},
	UID("ruid", "RUID", pvar, POFF(p_ruid)),
	{"ruser", "RUSER", NULL, LJUST, runame},
	{"sess", "SESS", NULL, 0, pvar, 0, POFF(p_sess), KPTR24, "llx"},
	PID("sid", "SID", pvar, POFF(p_sid)),
	{"sig", "PENDING",
	    NULL, 0, pvar, 0, POFF(p_siglist), SIGLIST, "s"},
	{"sigcatch", "CAUGHT",
	    NULL, 0, pvar, 0, POFF(p_sigcatch), SIGLIST, "s"},
	{"sigignore", "IGNORED",
	    NULL, 0, pvar, 0, POFF(p_sigignore), SIGLIST, "s"},
	{"sigmask", "BLOCKED",
	    NULL, 0, pvar, 0, POFF(p_sigmask), SIGLIST, "s"},
	{"sl", "SL", NULL, INF127|LWP, pvar, 0, LOFF(l_slptime), UINT, "d"},
	{"start", "STARTED", NULL, 0, started},
	{"stat", "", "state"},
	{"state", "STAT", NULL, LJUST, state},
	GID("svgid", "SVGID", pvar, POFF(p_gid)),
	UID("svuid", "SVUID", pvar, POFF(p_uid)),
	{"tdev", "TDEV", NULL, 0, tdev},
	{"time", "TIME", NULL, 0, cputime},
	PID("tpgid", "TGPID", pvar, POFF(p_tpgid)),
	{"tsess", "TSESS", NULL, 0, pvar, 0, POFF(p_tsess), KPTR, "llx"},
	{"tsiz", "TSIZ", NULL, 0, tsize},
	{"tt", "TT", NULL, LJUST, tname},
	{"tty", "TTY", NULL, LJUST, longtname},
	{"ucomm", "UCOMM", NULL, LJUST, ucomm},
	UID("uid", "UID", pvar, POFF(p_uid)),
	{"upr", "UPR", NULL, LWP, pvar, 0, LOFF(l_usrpri), UCHAR, "d"},
	{"user", "USER", NULL, LJUST, uname},
	{"usrpri", "", "upr"},
	{"vsize", "", "vsz"},
	{"vsz", "VSZ", NULL, 0, vsize},
	{"wchan", "WCHAN", NULL, LJUST|LWP, wchan},
	{"xstat", "XSTAT", NULL, 0, pvar, 0, POFF(p_xstat), USHORT, "x"},
	{""},
};

void
showkey()
{
	VAR *v;
	int i;
	char *p, *sep;

	i = 0;
	sep = "";
	for (v = var; *(p = v->name); ++v) {
		int len = strlen(p);
		if (termwidth && (i += len + 1) > termwidth) {
			i = len;
			sep = "\n";
		}
		(void) printf("%s%s", sep, p);
		sep = " ";
	}
	(void) printf("\n");
}

void
parsefmt(p)
	char *p;
{
	static struct varent *vtail;

#define	FMTSEP	" \t,\n"
	while (p && *p) {
		char *cp;
		VAR *v;
		struct varent *vent;

		while ((cp = strsep(&p, FMTSEP)) != NULL && *cp == '\0')
			/* void */;
		if (cp == NULL || !(v = findvar(cp)))
			continue;
		if ((vent = malloc(sizeof(struct varent))) == NULL)
			err(1, NULL);
		vent->var = v;
		vent->next = NULL;
		if (vhead == NULL)
			vhead = vtail = vent;
		else {
			vtail->next = vent;
			vtail = vent;
		}
	}
	if (!vhead)
		errx(1, "no valid keywords");
}

static VAR *
findvar(p)
	char *p;
{
	VAR *v, key;
	char *hp;

	key.name = p;

	hp = strchr(p, '=');
	if (hp)
		*hp++ = '\0';

	key.name = p;
	v = bsearch(&key, var, sizeof(var)/sizeof(VAR) - 1, sizeof(VAR), vcmp);

	if (v && v->alias) {
		if (hp) {
			warnx("%s: illegal keyword specification", p);
			eval = 1;
		}
		parsefmt(v->alias);
		return ((VAR *)NULL);
	}
	if (!v) {
		warnx("%s: keyword not found", p);
		eval = 1;
		return ((VAR *)NULL);
	}
	if (hp)
		v->header = hp;
	return (v);
}

static int
vcmp(a, b)
        const void *a, *b;
{
        return (strcmp(((VAR *)a)->name, ((VAR *)b)->name));
}
