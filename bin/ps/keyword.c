/*	$NetBSD: keyword.c,v 1.33 2003/03/08 06:46:22 christos Exp $	*/

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
__RCSID("$NetBSD: keyword.c,v 1.33 2003/03/08 06:46:22 christos Exp $");
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

static VAR *findvar(const char *);
static int  vcmp(const void *, const void *);

#if 0 	/* kernel doesn't calculate these */
	PUVAR("idrss", "IDRSS", 0, p_uru_idrss, UINT64, PRIu64),
	PUVAR("isrss", "ISRSS", 0, p_uru_isrss, UINT64, PRId64),
	PUVAR("ixrss", "IXRSS", 0, p_uru_ixrss, UINT64, PRId64),
	PUVAR("maxrss", "MAXRSS", 0, p_uru_maxrss, UINT64, PRIu64),
#endif

/* Compute offset in common structures. */
#define	POFF(x)	offsetof(struct kinfo_proc2, x)
#define	LOFF(x)	offsetof(struct kinfo_lwp, x)

#define	UIDFMT	"u"
#define	UID(n1, n2, off) \
	{ n1, n2, 0, pvar, POFF(off), UINT32, UIDFMT }
#define	GID(n1, n2, off)	UID(n1, n2, off)

#define	PIDFMT	"d"
#define	PID(n1, n2, off) \
	{ n1, n2, 0, pvar, POFF(off), INT32, PIDFMT }

#define	LVAR(n1, n2, fl, off, type, fmt) \
	{ n1, n2, (fl) | LWP, pvar, LOFF(off), type, fmt }
#define	PVAR(n1, n2, fl, off, type, fmt) \
	{ n1, n2, (fl) | 0, pvar, POFF(off), type, fmt }
#define	PUVAR(n1, n2, fl, off, type, fmt) \
	{ n1, n2, (fl) | UAREA, pvar, POFF(off), type, fmt }

/* NB: table must be sorted, in vi use:
 *	:/^VAR/,/end_sort/! sort -t\" +1
 * breaking long lines just makes the sort harder
 */
VAR var[] = {
	{"%cpu", "%CPU", 0, pcpu, 0, PCPU},
	{"%mem", "%MEM", 0, pmem, POFF(p_vm_rssize), INT32},
	PVAR("acflag", "ACFLG", 0, p_acflag, USHORT, "x"),
	{"acflg", "acflag", ALIAS},
	{"blocked", "sigmask", ALIAS},
	{"caught", "sigcatch", ALIAS},
	{"command", "COMMAND", COMM|LJUST, command},
	PVAR("cpu", "CPU", 0, p_estcpu, UINT, "u"),
	{"cputime", "time", ALIAS},
	{"ctime", "CTIME", 0, putimeval, POFF(p_uctime_sec), TIMEVAL},
	GID("egid", "EGID", p_gid),
	{"egroup", "EGROUP", LJUST, gname},
	UID("euid", "EUID", p_uid),
	{"euser", "EUSER", LJUST, uname},
	PVAR("f", "F", 0, p_flag, INT, "x"),
	{"flags", "f", ALIAS},
	GID("gid", "GID", p_gid),
	{"group", "GROUP", LJUST, gname},
	{"groupnames", "GROUPNAMES", LJUST, groupnames},
	{"groups", "GROUPS", LJUST, groups},
	LVAR("holdcnt", "HOLDCNT", 0, l_holdcnt, INT, "d"),
	{"ignored", "sigignore", ALIAS},
	PUVAR("inblk", "INBLK", 0, p_uru_inblock, UINT64, PRIu64),
	{"inblock", "inblk", ALIAS},
	PVAR("jobc", "JOBC", 0, p_jobc, SHORT, "d"),
	PVAR("ktrace", "KTRACE", 0, p_traceflag, INT, "x"),
/*XXX*/	PVAR("ktracep", "KTRACEP", 0, p_tracep, KPTR, PRIx64),
	LVAR("lid", "LID", 0, l_lid, ULONG, "d"),
	{"lim", "LIM", 0, maxrss},
	{"login", "LOGIN", LJUST, logname},
	{"logname", "login", ALIAS},
	{"lstart", "STARTED", LJUST, lstarted, POFF(p_ustart_sec), UINT32},
	{"lstate", "STAT", LJUST|LWP, lstate},
	PUVAR("majflt", "MAJFLT", 0, p_uru_majflt, UINT64, PRIu64),
	PUVAR("minflt", "MINFLT", 0, p_uru_minflt, UINT64, PRIu64),
	PUVAR("msgrcv", "MSGRCV", 0, p_uru_msgrcv, UINT64, PRIu64),
	PUVAR("msgsnd", "MSGSND", 0, p_uru_msgsnd, UINT64, PRIu64),
	{"ni", "nice", ALIAS},
	{"nice", "NI", 0, pnice, POFF(p_nice), UCHAR},
	PUVAR("nivcsw", "NIVCSW", 0, p_uru_nivcsw, UINT64, PRIu64),
	PVAR("nlwp", "NLWP", 0, p_nlwps, UINT64, PRId64),
	{"nsignals", "nsigs", ALIAS},
	PUVAR("nsigs", "NSIGS", 0, p_uru_nsignals, UINT64, PRIu64),
	PUVAR("nswap", "NSWAP", 0, p_uru_nswap, UINT64, PRIu64),
	PUVAR("nvcsw", "NVCSW", 0, p_uru_nvcsw, UINT64, PRIu64),
/*XXX*/	LVAR("nwchan", "WCHAN", 0, l_wchan, KPTR, PRIx64),
	PUVAR("oublk", "OUBLK", 0, p_uru_oublock, UINT64, PRIu64),
	{"oublock", "oublk", ALIAS},
/*XXX*/	PVAR("p_ru", "P_RU", 0, p_ru, KPTR, PRIx64),
/*XXX*/	PVAR("paddr", "PADDR", 0, p_paddr, KPTR, PRIx64),
	PUVAR("pagein", "PAGEIN", 0, p_uru_majflt, UINT64, PRIu64),
	{"pcpu", "%cpu", ALIAS},
	{"pending", "sig", ALIAS},
	PID("pgid", "PGID", p__pgid),
	PID("pid", "PID", p_pid),
	{"pmem", "%mem", ALIAS},
	PID("ppid", "PPID", p_ppid),
	{"pri", "PRI", LWP, pri},
	LVAR("re", "RE", INF127, l_swtime, UINT, "u"),
	GID("rgid", "RGID", p_rgid),
	{"rgroup", "RGROUP", LJUST, rgname},
/*XXX*/	LVAR("rlink", "RLINK", 0, l_back, KPTR, PRIx64),
	PVAR("rlwp", "RLWP", 0, p_nrlwps, UINT64, PRId64),
	{"rss", "RSS", 0, p_rssize, POFF(p_vm_rssize), INT32},
	{"rssize", "rsz", ALIAS},
	{"rsz", "RSZ", 0, rssize, POFF(p_vm_rssize), INT32},
	UID("ruid", "RUID", p_ruid),
	{"ruser", "RUSER", LJUST, runame},
	PVAR("sess", "SESS", 0, p_sess, KPTR24, PRIx64),
	PID("sid", "SID", p_sid),
	PVAR("sig", "PENDING", 0, p_siglist, SIGLIST, "s"),
	PVAR("sigcatch", "CAUGHT", 0, p_sigcatch, SIGLIST, "s"),
	PVAR("sigignore", "IGNORED", 0, p_sigignore, SIGLIST, "s"),
	PVAR("sigmask", "BLOCKED", 0, p_sigmask, SIGLIST, "s"),
	LVAR("sl", "SL", INF127, l_slptime, UINT, "u"),
	{"start", "STARTED", 0, started, POFF(p_ustart_sec), UINT32},
	{"stat", "state", ALIAS},
	{"state", "STAT", LJUST, state},
	{"stime", "STIME", 0, putimeval, POFF(p_ustime_sec), TIMEVAL},
	GID("svgid", "SVGID", p_svgid),
	{"svgroup", "SVGROUP", LJUST, svgname},
	UID("svuid", "SVUID", p_svuid),
	{"svuser", "SVUSER", LJUST, svuname},
	/* tdev is UINT32, but we do this for sorting purposes */
	{"tdev", "TDEV", 0, tdev, POFF(p_tdev), INT32},
	{"time", "TIME", 0, cputime, 0, CPUTIME},
	PID("tpgid", "TGPID", p_tpgid),
	PVAR("tsess", "TSESS", 0, p_tsess, KPTR, PRIx64),
	{"tsiz", "TSIZ", 0, tsize, POFF(p_vm_tsize), INT32},
	{"tt", "TT", LJUST, tname},
	{"tty", "TTY", LJUST, longtname},
	{"ucomm", "UCOMM", LJUST, ucomm},
	UID("uid", "UID", p_uid),
	LVAR("upr", "UPR", 0, l_usrpri, UCHAR, "u"),
	{"user", "USER", LJUST, uname},
	{"usrpri", "upr", ALIAS},
	{"utime", "UTIME", 0, putimeval, POFF(p_uutime_sec), TIMEVAL},
	{"vsize", "vsz", ALIAS},
	{"vsz", "VSZ", 0, vsize, 0, VSIZE},
	{"wchan", "WCHAN", LJUST|LWP, wchan},
	PVAR("xstat", "XSTAT", 0, p_xstat, USHORT, "x"),
/* "zzzz" end_sort */
	{""},
};

void
showkey(void)
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

static void
parsevarlist(const char *pp, struct varent **head, struct varent **tail)
{
	char *p;

	/* dup to avoid zapping arguments, can't free because it
	   might contain a header. */
	p = strdup(pp);

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
		if (*head == NULL)
			*head = vent;
		else
			(*tail)->next = vent;
		*tail = vent;
	}
	if (!*head)
		errx(1, "no valid keywords");
}

void
parsefmt(const char *p)
{
	static struct varent *vtail;

	parsevarlist(p, &vhead, &vtail);
}

void
parsesort(const char *p)
{
	static struct varent *sorttail;

	parsevarlist(p, &sorthead, &sorttail);
}

static VAR *
findvar(const char *p)
{
	VAR *v;
	char *hp;

	hp = strchr(p, '=');
	if (hp)
		*hp++ = '\0';

	v = bsearch(p, var, sizeof(var)/sizeof(VAR) - 1, sizeof(VAR), vcmp);
	if (v && v->flag & ALIAS)
		v = findvar(v->header);
	if (!v) {
		warnx("%s: keyword not found", p);
		eval = 1;
		return NULL;
	}

	if (v && hp)
		v->header = hp;
	return v;
}

static int
vcmp(const void *a, const void *b)
{
        return strcmp(a, ((VAR *)b)->name);
}
