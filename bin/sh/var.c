/*	$NetBSD: var.c,v 1.69 2017/11/19 03:23:01 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
static char sccsid[] = "@(#)var.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: var.c,v 1.69 2017/11/19 03:23:01 kre Exp $");
#endif
#endif /* not lint */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>

/*
 * Shell variables.
 */

#include "shell.h"
#include "output.h"
#include "expand.h"
#include "nodes.h"	/* for other headers */
#include "eval.h"	/* defines cmdenviron */
#include "exec.h"
#include "syntax.h"
#include "options.h"
#include "builtins.h"
#include "mail.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include "parser.h"
#include "show.h"
#include "machdep.h"
#ifndef SMALL
#include "myhistedit.h"
#endif

#ifdef SMALL
#define VTABSIZE 39
#else
#define VTABSIZE 517
#endif


struct varinit {
	struct var *var;
	int flags;
	const char *text;
	union var_func_union v_u;
};
#define	func v_u.set_func
#define	rfunc v_u.ref_func

char *get_lineno(struct var *);

#ifndef SMALL
char *get_tod(struct var *);
char *get_hostname(struct var *);
char *get_seconds(struct var *);
char *get_euser(struct var *);
char *get_random(struct var *);
#endif

struct localvar *localvars;

#ifndef SMALL
struct var vhistsize;
struct var vterm;
struct var editrc;
struct var ps_lit;
#endif
struct var vifs;
struct var vmail;
struct var vmpath;
struct var vpath;
struct var vps1;
struct var vps2;
struct var vps4;
struct var vvers;
struct var voptind;
struct var line_num;
#ifndef SMALL
struct var tod;
struct var host_name;
struct var seconds;
struct var euname;
struct var random_num;

intmax_t sh_start_time;
#endif

struct var line_num;
int line_number;
int funclinebase = 0;
int funclineabs = 0;

char ifs_default[] = " \t\n";

const struct varinit varinit[] = {
#ifndef SMALL
	{ &vhistsize,	VSTRFIXED|VTEXTFIXED|VUNSET,	"HISTSIZE=",
	   { .set_func= sethistsize } },
#endif
	{ &vifs,	VSTRFIXED|VTEXTFIXED,		"IFS= \t\n",
	   { NULL } },
	{ &vmail,	VSTRFIXED|VTEXTFIXED|VUNSET,	"MAIL=",
	   { NULL } },
	{ &vmpath,	VSTRFIXED|VTEXTFIXED|VUNSET,	"MAILPATH=",
	   { NULL } },
	{ &vvers,	VSTRFIXED|VTEXTFIXED|VNOEXPORT, "NETBSD_SHELL=",
	   { NULL } },
	{ &vpath,	VSTRFIXED|VTEXTFIXED,		"PATH=" _PATH_DEFPATH,
	   { .set_func= changepath } },
	/*
	 * vps1 depends on uid
	 */
	{ &vps2,	VSTRFIXED|VTEXTFIXED,		"PS2=> ",
	   { NULL } },
	{ &vps4,	VSTRFIXED|VTEXTFIXED,		"PS4=+ ",
	   { NULL } },
#ifndef SMALL
	{ &vterm,	VSTRFIXED|VTEXTFIXED|VUNSET,	"TERM=",
	   { .set_func= setterm } },
	{ &editrc, 	VSTRFIXED|VTEXTFIXED|VUNSET,	"EDITRC=",
	   { .set_func= set_editrc } },
	{ &ps_lit, 	VSTRFIXED|VTEXTFIXED|VUNSET,	"PSlit=",
	   { .set_func= set_prompt_lit } },
#endif
	{ &voptind,	VSTRFIXED|VTEXTFIXED|VNOFUNC,	"OPTIND=1",
	   { .set_func= getoptsreset } },
	{ &line_num,	VSTRFIXED|VTEXTFIXED|VFUNCREF,	"LINENO=1",
	   { .ref_func= get_lineno } },
#ifndef SMALL
	{ &tod,		VSTRFIXED|VTEXTFIXED|VFUNCREF,	"ToD=",
	   { .ref_func= get_tod } },
	{ &host_name,	VSTRFIXED|VTEXTFIXED|VFUNCREF,	"HOSTNAME=",
	   { .ref_func= get_hostname } },
	{ &seconds,	VSTRFIXED|VTEXTFIXED|VFUNCREF,	"SECONDS=",
	   { .ref_func= get_seconds } },
	{ &euname,	VSTRFIXED|VTEXTFIXED|VFUNCREF,	"EUSER=",
	   { .ref_func= get_euser } },
	{ &random_num,	VSTRFIXED|VTEXTFIXED|VFUNCREF,	"RANDOM=",
	   { .ref_func= get_random } },
#endif
	{ NULL,	0,				NULL,
	   { NULL } }
};

struct var *vartab[VTABSIZE];

STATIC int strequal(const char *, const char *);
STATIC struct var *find_var(const char *, struct var ***, int *);

/*
 * Initialize the varable symbol tables and import the environment
 */

#ifdef mkinit
INCLUDE <stdio.h>
INCLUDE <unistd.h>
INCLUDE <time.h>
INCLUDE "var.h"
INCLUDE "version.h"
MKINIT char **environ;
INIT {
	char **envp;
	char buf[64];

#ifndef SMALL
	sh_start_time = (intmax_t)time((time_t *)0);
#endif
	/*
	 * Set up our default variables and their values.
	 */
	initvar();

	/*
	 * Import variables from the environment, which will
	 * override anything initialised just previously.
	 */
	for (envp = environ ; *envp ; envp++) {
		if (strchr(*envp, '=')) {
			setvareq(*envp, VEXPORT|VTEXTFIXED);
		}
	}

	/*
	 * Set variables which override anything read from environment.
	 *
	 * PPID is readonly
	 * Always default IFS
	 * PSc indicates the root/non-root status of this shell.
	 * NETBSD_SHELL is a constant (readonly), and is never exported
	 * START_TIME belongs only to this shell.
	 * LINENO is simply magic...
	 */
	snprintf(buf, sizeof(buf), "%d", (int)getppid());
	setvar("PPID", buf, VREADONLY);
	setvar("IFS", ifs_default, VTEXTFIXED);
	setvar("PSc", (geteuid() == 0 ? "#" : "$"), VTEXTFIXED);

#ifndef SMALL
	snprintf(buf, sizeof(buf), "%jd", sh_start_time);
	setvar("START_TIME", buf, VTEXTFIXED);
#endif

	setvar("NETBSD_SHELL", NETBSD_SHELL
#ifdef BUILD_DATE
		" BUILD:" BUILD_DATE
#endif
#ifdef DEBUG
		" DEBUG"
#endif
#if !defined(JOBS) || JOBS == 0
		" -JOBS"
#endif
#ifndef DO_SHAREDVFORK
		" -VFORK"
#endif
#ifdef SMALL
		" SMALL"
#endif
#ifdef TINY
		" TINY"
#endif
#ifdef OLD_TTY_DRIVER
		" OLD_TTY"
#endif
#ifdef SYSV
		" SYSV"
#endif
#ifndef BSD
		" -BSD"
#endif
#ifdef BOGUS_NOT_COMMAND
		" BOGUS_NOT"
#endif
		    , VTEXTFIXED|VREADONLY|VNOEXPORT);

	setvar("LINENO", "1", VTEXTFIXED);
}
#endif


/*
 * This routine initializes the builtin variables.  It is called when the
 * shell is initialized and again when a shell procedure is spawned.
 */

void
initvar(void)
{
	const struct varinit *ip;
	struct var *vp;
	struct var **vpp;

	for (ip = varinit ; (vp = ip->var) != NULL ; ip++) {
		if (find_var(ip->text, &vpp, &vp->name_len) != NULL)
			continue;
		vp->next = *vpp;
		*vpp = vp;
		vp->text = strdup(ip->text);
		vp->flags = (ip->flags & ~VTEXTFIXED) | VSTRFIXED;
		vp->v_u = ip->v_u;
	}
	/*
	 * PS1 depends on uid
	 */
	if (find_var("PS1", &vpp, &vps1.name_len) == NULL) {
		vps1.next = *vpp;
		*vpp = &vps1;
		vps1.flags = VSTRFIXED;
		vps1.text = NULL;
		choose_ps1();
	}
}

void
choose_ps1(void)
{
	if ((vps1.flags & (VTEXTFIXED|VSTACK)) == 0)
		free(vps1.text);
	vps1.text = strdup(geteuid() ? "PS1=$ " : "PS1=# ");
	vps1.flags &= ~(VTEXTFIXED|VSTACK);

	/*
	 * Update PSc whenever we feel the need to update PS1
	 */
	setvarsafe("PSc", (geteuid() == 0 ? "#" : "$"), 0);
}

/*
 * Validate a string as a valid variable name
 * nb: not parameter - special params and such are "invalid" here.
 * Name terminated by either \0 or the term param (usually '=' or '\0').
 *
 * If not NULL, the length of the (intended) name is returned via len
 */

int
validname(const char *name, int term, int *len)
{
	const char *p = name;
	int ok = 1;

	if (p == NULL || *p == '\0' || *p == term) {
		if (len != NULL)
			*len = 0;
		return 0;
	}

	if (!is_name(*p))
		ok = 0;
	p++;
	for (;;) {
		if (*p == '\0' || *p == term)
			break;
		if (!is_in_name(*p))
			ok = 0;
		p++;
	}
	if (len != NULL)
		*len = p - name;

	return ok;
}

/*
 * Safe version of setvar, returns 1 on success 0 on failure.
 */

int
setvarsafe(const char *name, const char *val, int flags)
{
	struct jmploc jmploc;
	struct jmploc * const savehandler = handler;
	int volatile err = 0;

	if (setjmp(jmploc.loc))
		err = 1;
	else {
		handler = &jmploc;
		setvar(name, val, flags);
	}
	handler = savehandler;
	return err;
}

/*
 * Set the value of a variable.  The flags argument is ored with the
 * flags of the variable.  If val is NULL, the variable is unset.
 *
 * This always copies name and val when setting a variable, so
 * the source strings can be from anywhere, and are no longer needed
 * after this function returns.  The VTEXTFIXED and VSTACK flags should
 * not be used (but just in case they were, clear them.)
 */

void
setvar(const char *name, const char *val, int flags)
{
	const char *p;
	const char *q;
	char *d;
	int len;
	int namelen;
	char *nameeq;

	p = name;

	if (!validname(p, '=', &namelen))
		error("%.*s: bad variable name", namelen, name);
	len = namelen + 2;		/* 2 is space for '=' and '\0' */
	if (val == NULL) {
		flags |= VUNSET;
	} else {
		len += strlen(val);
	}
	d = nameeq = ckmalloc(len);
	q = name;
	while (--namelen >= 0)
		*d++ = *q++;
	*d++ = '=';
	*d = '\0';
	if (val)
		scopy(val, d);
	setvareq(nameeq, flags & ~(VTEXTFIXED | VSTACK));
}



/*
 * Same as setvar except that the variable and value are passed in
 * the first argument as name=value.  Since the first argument will
 * be actually stored in the table, it should not be a string that
 * will go away.   The flags (VTEXTFIXED or VSTACK) can be used to
 * indicate the source of the string (if neither is set, the string will
 * eventually be free()d when a replacement value is assigned.)
 */

void
setvareq(char *s, int flags)
{
	struct var *vp, **vpp;
	int nlen;

	VTRACE(DBG_VARS, ("setvareq([%s],%#x) aflag=%d ", s, flags, aflag));
	if (aflag && !(flags & VNOEXPORT))
		flags |= VEXPORT;
	vp = find_var(s, &vpp, &nlen);
	if (vp != NULL) {
		VTRACE(DBG_VARS, ("was [%s] fl:%#x\n", vp->text,
		    vp->flags));
		if (vp->flags & VREADONLY) {
			if ((flags & (VTEXTFIXED|VSTACK)) == 0)
				ckfree(s);
			if (flags & VNOERROR)
				return;
			error("%.*s: is read only", vp->name_len, vp->text);
		}
		if (flags & VNOSET) {
			if ((flags & (VTEXTFIXED|VSTACK)) == 0)
				ckfree(s);
			return;
		}

		INTOFF;

		if (vp->func && !(vp->flags & VFUNCREF) && !(flags & VNOFUNC))
			(*vp->func)(s + vp->name_len + 1);

		if ((vp->flags & (VTEXTFIXED|VSTACK)) == 0)
			ckfree(vp->text);

		vp->flags &= ~(VTEXTFIXED|VSTACK|VUNSET);
		if (flags & VNOEXPORT)
			vp->flags &= ~VEXPORT;
		if (vp->flags & VNOEXPORT)
			flags &= ~VEXPORT;
		vp->flags |= flags & ~VNOFUNC;
		vp->text = s;

		/*
		 * We could roll this to a function, to handle it as
		 * a regular variable function callback, but why bother?
		 */
		if (vp == &vmpath || (vp == &vmail && ! mpathset()))
			chkmail(1);

		INTON;
		return;
	}
	VTRACE(DBG_VARS, ("new\n"));
	/* not found */
	if (flags & VNOSET) {
		if ((flags & (VTEXTFIXED|VSTACK)) == 0)
			ckfree(s);
		return;
	}
	vp = ckmalloc(sizeof (*vp));
	vp->flags = flags & ~(VNOFUNC|VFUNCREF);
	vp->text = s;
	vp->name_len = nlen;
	vp->next = *vpp;
	vp->func = NULL;
	*vpp = vp;
}



/*
 * Process a linked list of variable assignments.
 */

void
listsetvar(struct strlist *list, int flags)
{
	struct strlist *lp;

	INTOFF;
	for (lp = list ; lp ; lp = lp->next) {
		setvareq(savestr(lp->text), flags);
	}
	INTON;
}

void
listmklocal(struct strlist *list, int flags)
{
	struct strlist *lp;

	for (lp = list ; lp ; lp = lp->next)
		mklocal(lp->text, flags);
}


/*
 * Find the value of a variable.  Returns NULL if not set.
 */

char *
lookupvar(const char *name)
{
	struct var *v;

	v = find_var(name, NULL, NULL);
	if (v == NULL || v->flags & VUNSET)
		return NULL;
	if (v->rfunc && (v->flags & VFUNCREF) != 0)
		return (*v->rfunc)(v) + v->name_len + 1;
	return v->text + v->name_len + 1;
}



/*
 * Search the environment of a builtin command.  If the second argument
 * is nonzero, return the value of a variable even if it hasn't been
 * exported.
 */

char *
bltinlookup(const char *name, int doall)
{
	struct strlist *sp;
	struct var *v;

	for (sp = cmdenviron ; sp ; sp = sp->next) {
		if (strequal(sp->text, name))
			return strchr(sp->text, '=') + 1;
	}

	v = find_var(name, NULL, NULL);

	if (v == NULL || v->flags & VUNSET || (!doall && !(v->flags & VEXPORT)))
		return NULL;
	if (v->rfunc && (v->flags & VFUNCREF) != 0)
		return (*v->rfunc)(v) + v->name_len + 1;
	return v->text + v->name_len + 1;
}



/*
 * Generate a list of exported variables.  This routine is used to construct
 * the third argument to execve when executing a program.
 */

char **
environment(void)
{
	int nenv;
	struct var **vpp;
	struct var *vp;
	char **env;
	char **ep;

	nenv = 0;
	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next)
			if ((vp->flags & (VEXPORT|VUNSET)) == VEXPORT)
				nenv++;
	}
	CTRACE(DBG_VARS, ("environment: %d vars to export\n", nenv));
	ep = env = stalloc((nenv + 1) * sizeof *env);
	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next)
			if ((vp->flags & (VEXPORT|VUNSET)) == VEXPORT) {
				if (vp->rfunc && (vp->flags & VFUNCREF))
					*ep++ = (*vp->rfunc)(vp);
				else
					*ep++ = vp->text;
				VTRACE(DBG_VARS, ("environment: %s\n", ep[-1]));
			}
	}
	*ep = NULL;
	return env;
}


/*
 * Called when a shell procedure is invoked to clear out nonexported
 * variables.  It is also necessary to reallocate variables of with
 * VSTACK set since these are currently allocated on the stack.
 */

#ifdef mkinit
void shprocvar(void);

SHELLPROC {
	shprocvar();
}
#endif

void
shprocvar(void)
{
	struct var **vpp;
	struct var *vp, **prev;

	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (prev = vpp ; (vp = *prev) != NULL ; ) {
			if ((vp->flags & VEXPORT) == 0) {
				*prev = vp->next;
				if ((vp->flags & VTEXTFIXED) == 0)
					ckfree(vp->text);
				if ((vp->flags & VSTRFIXED) == 0)
					ckfree(vp);
			} else {
				if (vp->flags & VSTACK) {
					vp->text = savestr(vp->text);
					vp->flags &=~ VSTACK;
				}
				prev = &vp->next;
			}
		}
	}
	initvar();
}



/*
 * Command to list all variables which are set.  Currently this command
 * is invoked from the set command when the set command is called without
 * any variables.
 */

void
print_quoted(const char *p)
{
	const char *q;

	if (p[0] == '\0') {
		out1fmt("''");
		return;
	}
	if (strcspn(p, "|&;<>()$`\\\"' \t\n*?[]#~=%") == strlen(p)) {
		out1fmt("%s", p);
		return;
	}
	while (*p) {
		if (*p == '\'') {
			out1fmt("\\'");
			p++;
			continue;
		}
		q = strchr(p, '\'');
		if (!q) {
			out1fmt("'%s'", p );
			return;
		}
		out1fmt("'%.*s'", (int)(q - p), p );
		p = q;
	}
}

static int
sort_var(const void *v_v1, const void *v_v2)
{
	const struct var * const *v1 = v_v1;
	const struct var * const *v2 = v_v2;
	char *t1 = (*v1)->text, *t2 = (*v2)->text;

	if (*t1 == *t2) {
		char *p, *s;

		STARTSTACKSTR(p);

		/*
		 * note: if lengths are equal, strings must be different
		 * so we don't care which string we pick for the \0 in
		 * that case.
		 */
		if ((strchr(t1, '=') - t1) <= (strchr(t2, '=') - t2)) {
			s = t1;
			t1 = p;
		} else {
			s = t2;
			t2 = p;
		}

		while (*s && *s != '=') {
			STPUTC(*s, p);
			s++;
		}
		STPUTC('\0', p);
	}

	return strcoll(t1, t2);
}

/*
 * POSIX requires that 'set' (but not export or readonly) output the
 * variables in lexicographic order - by the locale's collating order (sigh).
 * Maybe we could keep them in an ordered balanced binary tree
 * instead of hashed lists.
 * For now just roll 'em through qsort for printing...
 */

int
showvars(const char *name, int flag, int show_value, const char *xtra)
{
	struct var **vpp;
	struct var *vp;
	const char *p;

	static struct var **list;	/* static in case we are interrupted */
	static int list_len;
	int count = 0;

	if (!list) {
		list_len = 32;
		list = ckmalloc(list_len * sizeof *list);
	}

	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next) {
			if (flag && !(vp->flags & flag))
				continue;
			if (vp->flags & VUNSET && !(show_value & 2))
				continue;
			if (count >= list_len) {
				list = ckrealloc(list,
					(list_len << 1) * sizeof *list);
				list_len <<= 1;
			}
			list[count++] = vp;
		}
	}

	qsort(list, count, sizeof *list, sort_var);

	for (vpp = list; count--; vpp++) {
		vp = *vpp;
		if (name)
			out1fmt("%s ", name);
		if (xtra)
			out1fmt("%s ", xtra);
		p = vp->text;
		if (vp->rfunc && (vp->flags & VFUNCREF) != 0) {
			p = (*vp->rfunc)(vp);
			if (p == NULL)
				p = vp->text;
		}
		for ( ; *p != '=' ; p++)
			out1c(*p);
		if (!(vp->flags & VUNSET) && show_value) {
			out1fmt("=");
			print_quoted(++p);
		}
		out1c('\n');
	}
	return 0;
}



/*
 * The export and readonly commands.
 */

int
exportcmd(int argc, char **argv)
{
	struct var *vp;
	char *name;
	const char *p;
	int flag = argv[0][0] == 'r'? VREADONLY : VEXPORT;
	int pflg = 0;
	int nflg = 0;
	int xflg = 0;
	int res;
	int c;
	int f;

	while ((c = nextopt("npx")) != '\0') {
		switch (c) {
		case 'p':
			if (nflg)
				return 1;
			pflg = 3;
			break;
		case 'n':
			if (pflg || xflg || flag == VREADONLY)
				return 1;
			nflg = 1;
			break;
		case 'x':
			if (nflg || flag == VREADONLY)
				return 1;
			flag = VNOEXPORT;
			xflg = 1;
			break;
		default:
			return 1;
		}
	}

	if (nflg && *argptr == NULL)
		return 1;

	if (pflg || *argptr == NULL) {
		showvars( pflg ? argv[0] : 0, flag, pflg,
		    pflg && xflg ? "-x" : NULL );
		return 0;
	}

	res = 0;
	while ((name = *argptr++) != NULL) {
		f = flag;
		if ((p = strchr(name, '=')) != NULL) {
			p++;
		} else {
			vp = find_var(name, NULL, NULL);
			if (vp != NULL) {
				if (nflg)
					vp->flags &= ~flag;
				else if (flag&VEXPORT && vp->flags&VNOEXPORT)
					res = 1;
				else {
					vp->flags |= flag;
					if (flag == VNOEXPORT)
						vp->flags &= ~VEXPORT;
				}
				continue;
			} else
				f |= VUNSET;
		}
		if (!nflg)
			setvar(name, p, f);
	}
	return res;
}


/*
 * The "local" command.
 */

int
localcmd(int argc, char **argv)
{
	char *name;
	int c;
	int flags = 0;		/*XXX perhaps VUNSET from a -o option value */

	if (! in_function())
		error("Not in a function");

	/* upper case options, as bash stole all the good ones ... */
	while ((c = nextopt("INx")) != '\0')
		switch (c) {
		case 'I':	flags &= ~VUNSET;	break;
		case 'N':	flags |= VUNSET;	break;
		case 'x':	flags |= VEXPORT;	break;
		}

	while ((name = *argptr++) != NULL) {
		mklocal(name, flags);
	}
	return 0;
}


/*
 * Make a variable a local variable.  When a variable is made local, its
 * value and flags are saved in a localvar structure.  The saved values
 * will be restored when the shell function returns.  We handle the name
 * "-" as a special case.
 */

void
mklocal(const char *name, int flags)
{
	struct localvar *lvp;
	struct var **vpp;
	struct var *vp;

	INTOFF;
	lvp = ckmalloc(sizeof (struct localvar));
	if (name[0] == '-' && name[1] == '\0') {
		char *p;
		p = ckmalloc(sizeof_optlist);
		lvp->text = memcpy(p, optlist, sizeof_optlist);
		vp = NULL;
		xtrace_clone(0);
	} else {
		vp = find_var(name, &vpp, NULL);
		if (vp == NULL) {
			flags &= ~VNOEXPORT;
			if (strchr(name, '='))
				setvareq(savestr(name),
				    VSTRFIXED | (flags & ~VUNSET));
			else
				setvar(name, NULL, VSTRFIXED|flags);
			vp = *vpp;	/* the new variable */
			lvp->text = NULL;
			lvp->flags = VUNSET;
		} else {
			lvp->text = vp->text;
			lvp->flags = vp->flags;
			vp->flags |= VSTRFIXED|VTEXTFIXED;
			if (vp->flags & VNOEXPORT)
				flags &= ~VEXPORT;
			if (flags & (VNOEXPORT | VUNSET))
				vp->flags &= ~VEXPORT;
			flags &= ~VNOEXPORT;
			if (name[vp->name_len] == '=')
				setvareq(savestr(name), flags & ~VUNSET);
			else if (flags & VUNSET)
				unsetvar(name, 0);
			else
				vp->flags |= flags & (VUNSET|VEXPORT);

			if (vp == &line_num) {
				if (name[vp->name_len] == '=')
					funclinebase = funclineabs -1;
				else
					funclinebase = 0;
			}
		}
	}
	lvp->vp = vp;
	lvp->next = localvars;
	localvars = lvp;
	INTON;
}


/*
 * Called after a function returns.
 */

void
poplocalvars(void)
{
	struct localvar *lvp;
	struct var *vp;

	while ((lvp = localvars) != NULL) {
		localvars = lvp->next;
		vp = lvp->vp;
		VTRACE(DBG_VARS, ("poplocalvar %s", vp ? vp->text : "-"));
		if (vp == NULL) {	/* $- saved */
			memcpy(optlist, lvp->text, sizeof_optlist);
			ckfree(lvp->text);
			xtrace_pop();
			optschanged();
		} else if ((lvp->flags & (VUNSET|VSTRFIXED)) == VUNSET) {
			(void)unsetvar(vp->text, 0);
		} else {
			if (vp->func && (vp->flags & (VNOFUNC|VFUNCREF)) == 0)
				(*vp->func)(lvp->text + vp->name_len + 1);
			if ((vp->flags & VTEXTFIXED) == 0)
				ckfree(vp->text);
			vp->flags = lvp->flags;
			vp->text = lvp->text;
		}
		ckfree(lvp);
	}
}


int
setvarcmd(int argc, char **argv)
{
	if (argc <= 2)
		return unsetcmd(argc, argv);
	else if (argc == 3)
		setvar(argv[1], argv[2], 0);
	else
		error("List assignment not implemented");
	return 0;
}


/*
 * The unset builtin command.  We unset the function before we unset the
 * variable to allow a function to be unset when there is a readonly variable
 * with the same name.
 */

int
unsetcmd(int argc, char **argv)
{
	char **ap;
	int i;
	int flg_func = 0;
	int flg_var = 0;
	int flg_x = 0;
	int ret = 0;

	while ((i = nextopt("efvx")) != '\0') {
		switch (i) {
		case 'f':
			flg_func = 1;
			break;
		case 'e':
		case 'x':
			flg_x = (2 >> (i == 'e'));
			/* FALLTHROUGH */
		case 'v':
			flg_var = 1;
			break;
		}
	}

	if (flg_func == 0 && flg_var == 0)
		flg_var = 1;

	for (ap = argptr; *ap ; ap++) {
		if (flg_func)
			ret |= unsetfunc(*ap);
		if (flg_var)
			ret |= unsetvar(*ap, flg_x);
	}
	return ret;
}


/*
 * Unset the specified variable.
 */

int
unsetvar(const char *s, int unexport)
{
	struct var **vpp;
	struct var *vp;

	vp = find_var(s, &vpp, NULL);
	if (vp == NULL)
		return 0;

	if (vp->flags & VREADONLY && !(unexport & 1))
		return 1;

	INTOFF;
	if (unexport & 1) {
		vp->flags &= ~VEXPORT;
	} else {
		if (vp->text[vp->name_len + 1] != '\0')
			setvar(s, nullstr, 0);
		if (!(unexport & 2))
			vp->flags &= ~VEXPORT;
		vp->flags |= VUNSET;
		if ((vp->flags&(VEXPORT|VSTRFIXED|VREADONLY|VNOEXPORT)) == 0) {
			if ((vp->flags & VTEXTFIXED) == 0)
				ckfree(vp->text);
			*vpp = vp->next;
			ckfree(vp);
		}
	}
	INTON;
	return 0;
}


/*
 * Returns true if the two strings specify the same varable.  The first
 * variable name is terminated by '='; the second may be terminated by
 * either '=' or '\0'.
 */

STATIC int
strequal(const char *p, const char *q)
{
	while (*p == *q++) {
		if (*p++ == '=')
			return 1;
	}
	if (*p == '=' && *(q - 1) == '\0')
		return 1;
	return 0;
}

/*
 * Search for a variable.
 * 'name' may be terminated by '=' or a NUL.
 * vppp is set to the pointer to vp, or the list head if vp isn't found
 * lenp is set to the number of charactets in 'name'
 */

STATIC struct var *
find_var(const char *name, struct var ***vppp, int *lenp)
{
	unsigned int hashval;
	int len;
	struct var *vp, **vpp;
	const char *p = name;

	hashval = 0;
	while (*p && *p != '=')
		hashval = 2 * hashval + (unsigned char)*p++;
	len = p - name;

	if (lenp)
		*lenp = len;
	vpp = &vartab[hashval % VTABSIZE];
	if (vppp)
		*vppp = vpp;

	for (vp = *vpp ; vp ; vpp = &vp->next, vp = *vpp) {
		if (vp->name_len != len)
			continue;
		if (memcmp(vp->text, name, len) != 0)
			continue;
		if (vppp)
			*vppp = vpp;
		return vp;
	}
	return NULL;
}

/*
 * The following are the functions that create the values for
 * shell variables that are dynamically produced when needed.
 *
 * The output strings cannot be malloc'd as there is nothing to
 * free them - callers assume these are ordinary variables where
 * the value returned is vp->text
 *
 * Each function needs its own storage space, as the results are
 * used to create processes' environment, and (if exported) all
 * the values will (might) be needed simultaneously.
 *
 * It is not a problem if a var is updated while nominally in use
 * somewhere, all these are intended to be dynamic, the value they
 * return is not guaranteed, an updated vaue is just as good.
 *
 * So, malloc a single buffer for the result of each function,
 * grow, and even shrink, it as needed, but once we have one that
 * is a suitable size for the actual usage, simply hold it forever.
 *
 * For a SMALL shell we implement only LINENO, none of the others,
 * and give it just a fixed length static buffer for its result.
 */

#ifndef SMALL

struct space_reserved {		/* record of space allocated for results */
	char *b;
	int len;
};

/* rough (over-)estimate of the number of bytes needed to hold a number */
static int
digits_in(intmax_t number)
{
	int res = 0;

	if (number & ~((1LL << 62) - 1))
		res = 64;	/* enough for 2^200 and a bit more */
	else if (number & ~((1LL << 32) - 1))
		res = 20;	/* enough for 2^64 */
	else if (number & ~((1 << 23) - 1))
		res = 10;	/* enough for 2^32 */
	else
		res = 8;	/* enough for 2^23 or smaller */

	return res;
}

static int
make_space(struct space_reserved *m, int bytes)
{
	void *p;

	if (m->len >= bytes && m->len <= (bytes<<2))
		return 1;

	bytes = SHELL_ALIGN(bytes);
	/* not ckrealloc() - we want failure, not error() here */
	p = realloc(m->b, bytes);
	if (p == NULL)	/* what we had should still be there */
		return 0;

	m->b = p;
	m->len = bytes;
	m->b[bytes - 1] = '\0';

	return 1;
}
#endif

char *
get_lineno(struct var *vp)
{
#ifdef SMALL
#define length (8 + 10)		/* 10 digits is enough for a 32 bit line num */
	static char result[length];
#else
	static struct space_reserved buf;
#define result buf.b
#define length buf.len
#endif
	int ln = line_number;

	if (vp->flags & VUNSET)
		return NULL;

	ln -= funclinebase;

#ifndef SMALL
	if (!make_space(&buf, vp->name_len + 2 + digits_in(ln)))
		return vp->text;
#endif

	snprintf(result, length - 1, "%.*s=%d", vp->name_len, vp->text, ln);
	return result;
}
#undef result
#undef length

#ifndef SMALL

char *
get_hostname(struct var *vp)
{
	static struct space_reserved buf;

	if (vp->flags & VUNSET)
		return NULL;

	if (!make_space(&buf, vp->name_len + 2 + 256))
		return vp->text;

	memcpy(buf.b, vp->text, vp->name_len + 1);	/* include '=' */
	(void)gethostname(buf.b + vp->name_len + 1,
	    buf.len - vp->name_len - 3);
	return buf.b;
}

char *
get_tod(struct var *vp)
{
	static struct space_reserved buf;	/* space for answers */
	static struct space_reserved tzs;	/* remember TZ last used */
	static timezone_t last_zone;		/* timezone data for tzs zone */
	const char *fmt;
	char *tz;
	time_t now;
	struct tm tm_now, *tmp;
	timezone_t zone = NULL;
	static char t_err[] = "time error";
	int len;

	if (vp->flags & VUNSET)
		return NULL;

	fmt = lookupvar("ToD_FORMAT");
	if (fmt == NULL)
		fmt="%T";
	tz = lookupvar("TZ");
	(void)time(&now);

	if (tz != NULL) {
		if (tzs.b == NULL || strcmp(tzs.b, tz) != 0) {
			if (make_space(&tzs, strlen(tz) + 1)) {
				INTOFF;
				strcpy(tzs.b, tz);
				if (last_zone)
					tzfree(last_zone);
				last_zone = zone = tzalloc(tz);
				INTON;
			} else
				zone = tzalloc(tz);
		} else
			zone = last_zone;

		tmp = localtime_rz(zone, &now, &tm_now);
	} else
		tmp = localtime_r(&now, &tm_now);

	len = (strlen(fmt) * 4) + vp->name_len + 2;
	while (make_space(&buf, len)) {
		memcpy(buf.b, vp->text, vp->name_len+1);
		if (tmp == NULL) {
			if (buf.len >= vp->name_len+2+(int)(sizeof t_err - 1)) {
				strcpy(buf.b + vp->name_len + 1, t_err);
				if (zone && zone != last_zone)
					tzfree(zone);
				return buf.b;
			}
			len = vp->name_len + 4 + sizeof t_err - 1;
			continue;
		}
		if (strftime_z(zone, buf.b + vp->name_len + 1,
		     buf.len - vp->name_len - 2, fmt, tmp)) {
			if (zone && zone != last_zone)
				tzfree(zone);
			return buf.b;
		}
		if (len >= 4096)	/* Let's be reasonable */
			break;
		len <<= 1;
	}
	if (zone && zone != last_zone)
		tzfree(zone);
	return vp->text;
}

char *
get_seconds(struct var *vp)
{
	static struct space_reserved buf;
	intmax_t secs;

	if (vp->flags & VUNSET)
		return NULL;

	secs = (intmax_t)time((time_t *)0) - sh_start_time;
	if (!make_space(&buf, vp->name_len + 2 + digits_in(secs)))
		return vp->text;

	snprintf(buf.b, buf.len-1, "%.*s=%jd", vp->name_len, vp->text, secs);
	return buf.b;
}

char *
get_euser(struct var *vp)
{
	static struct space_reserved buf;
	static uid_t lastuid = 0;
	uid_t euid;
	struct passwd *pw;

	if (vp->flags & VUNSET)
		return NULL;

	euid = geteuid();
	if (buf.b != NULL && lastuid == euid)
		return buf.b;

	pw = getpwuid(euid);
	if (pw == NULL)
		return vp->text;

	if (make_space(&buf, vp->name_len + 2 + strlen(pw->pw_name))) {
		lastuid = euid;
		snprintf(buf.b, buf.len, "%.*s=%s", vp->name_len, vp->text,
		    pw->pw_name);
		return buf.b;
	}

	return vp->text;
}

char *
get_random(struct var *vp)
{
	static struct space_reserved buf;
	static intmax_t random_val = 0;

#ifdef USE_LRAND48
#define random lrand48
#define srandom srand48
#endif

	if (vp->flags & VUNSET)
		return NULL;

	if (vp->text != buf.b) {
		/*
		 * Either initialisation, or a new seed has been set
		 */
		if (vp->text[vp->name_len + 1] == '\0') {
			int fd;

			/*
			 * initialisation (without pre-seeding),
			 * or explictly requesting a truly random seed.
			 */
			fd = open("/dev/urandom", 0);
			if (fd == -1) {
				out2str("RANDOM initialisation failed\n");
				random_val = (getpid()<<3) ^ time((time_t *)0);
			} else {
				int n;

				do {
				    n = read(fd,&random_val,sizeof random_val);
				} while (n != sizeof random_val);
				close(fd);
			}
		} else
			/* good enough for today */
			random_val = atoi(vp->text + vp->name_len + 1);

		srandom((long)random_val);
	}

#if 0
	random_val = (random_val + 1) & 0x7FFF;	/* 15 bit "random" numbers */
#else
	random_val = (random() >> 5) & 0x7FFF;
#endif

	if (!make_space(&buf, vp->name_len + 2 + digits_in(random_val)))
		return vp->text;

	snprintf(buf.b, buf.len-1, "%.*s=%jd", vp->name_len, vp->text,
	    random_val);

	if (buf.b != vp->text && (vp->flags & (VTEXTFIXED|VSTACK)) == 0)
		free(vp->text);
	vp->flags |= VTEXTFIXED;
	vp->text = buf.b;

	return vp->text;
#undef random
#undef srandom
}

#endif /* SMALL */
