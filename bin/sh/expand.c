/*	$NetBSD: expand.c,v 1.125 2018/07/22 20:38:06 kre Exp $	*/

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
static char sccsid[] = "@(#)expand.c	8.5 (Berkeley) 5/15/95";
#else
__RCSID("$NetBSD: expand.c,v 1.125 2018/07/22 20:38:06 kre Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <wctype.h>
#include <wchar.h>

/*
 * Routines to expand arguments to commands.  We have to deal with
 * backquotes, shell variables, and file metacharacters.
 */

#include "shell.h"
#include "main.h"
#include "nodes.h"
#include "eval.h"
#include "expand.h"
#include "syntax.h"
#include "arithmetic.h"
#include "parser.h"
#include "jobs.h"
#include "options.h"
#include "builtins.h"
#include "var.h"
#include "input.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include "show.h"

/*
 * Structure specifying which parts of the string should be searched
 * for IFS characters.
 */

struct ifsregion {
	struct ifsregion *next;	/* next region in list */
	int begoff;		/* offset of start of region */
	int endoff;		/* offset of end of region */
	int inquotes;		/* search for nul bytes only */
};


char *expdest;			/* output of current string */
struct nodelist *argbackq;	/* list of back quote expressions */
struct ifsregion ifsfirst;	/* first struct in list of ifs regions */
struct ifsregion *ifslastp;	/* last struct in list */
struct arglist exparg;		/* holds expanded arg list */

STATIC const char *argstr(const char *, int);
STATIC const char *exptilde(const char *, int);
STATIC void expbackq(union node *, int, int);
STATIC const char *expari(const char *);
STATIC int subevalvar(const char *, const char *, int, int, int);
STATIC int subevalvar_trim(const char *, int, int, int, int, int);
STATIC const char *evalvar(const char *, int);
STATIC int varisset(const char *, int);
STATIC void varvalue(const char *, int, int, int);
STATIC void recordregion(int, int, int);
STATIC void removerecordregions(int); 
STATIC void ifsbreakup(char *, struct arglist *);
STATIC void ifsfree(void);
STATIC void expandmeta(struct strlist *, int);
STATIC void expmeta(char *, char *);
STATIC void addfname(char *);
STATIC struct strlist *expsort(struct strlist *);
STATIC struct strlist *msort(struct strlist *, int);
STATIC int patmatch(const char *, const char *, int);
STATIC char *cvtnum(int, char *);
static int collate_range_cmp(wchar_t, wchar_t);
STATIC void add_args(struct strlist *);
STATIC void rmescapes_nl(char *);

#ifdef	DEBUG
#define	NULLTERM_4_TRACE(p)	STACKSTRNUL(p)
#else
#define	NULLTERM_4_TRACE(p)	do { /* nothing */ } while (/*CONSTCOND*/0)
#endif

/*
 * Expand shell variables and backquotes inside a here document.
 */

void
expandhere(union node *arg, int fd)
{

	herefd = fd;
	expandarg(arg, NULL, 0);
	xwrite(fd, stackblock(), expdest - stackblock());
}


static int
collate_range_cmp(wchar_t c1, wchar_t c2)
{
	wchar_t s1[2], s2[2];

	s1[0] = c1;
	s1[1] = L'\0';
	s2[0] = c2;
	s2[1] = L'\0';
	return (wcscoll(s1, s2));
}

/*
 * Perform variable substitution and command substitution on an argument,
 * placing the resulting list of arguments in arglist.  If EXP_FULL is true,
 * perform splitting and file name expansion.  When arglist is NULL, perform
 * here document expansion.
 */

void
expandarg(union node *arg, struct arglist *arglist, int flag)
{
	struct strlist *sp;
	char *p;

	CTRACE(DBG_EXPAND, ("expandarg(fl=%#x)\n", flag));
	if (fflag)		/* no filename expandsion */
		flag &= ~EXP_GLOB;

	argbackq = arg->narg.backquote;
	STARTSTACKSTR(expdest);
	ifsfirst.next = NULL;
	ifslastp = NULL;
	line_number = arg->narg.lineno;
	argstr(arg->narg.text, flag);
	if (arglist == NULL) {
		STACKSTRNUL(expdest);
		CTRACE(DBG_EXPAND, ("expandarg: no arglist, done (%d) \"%s\"\n",
		    expdest - stackblock(), stackblock()));
		return;			/* here document expanded */
	}
	STPUTC('\0', expdest);
	CTRACE(DBG_EXPAND, ("expandarg: arglist got (%d) \"%s\"\n",
		    expdest - stackblock() - 1, stackblock()));
	p = grabstackstr(expdest);
	exparg.lastp = &exparg.list;
	/*
	 * TODO - EXP_REDIR
	 */
	if (flag & EXP_SPLIT) {
		ifsbreakup(p, &exparg);
		*exparg.lastp = NULL;
		exparg.lastp = &exparg.list;
		if (flag & EXP_GLOB)
			expandmeta(exparg.list, flag);
		else
			add_args(exparg.list);
	} else {
		if (flag & EXP_REDIR) /*XXX - for now, just remove escapes */
			rmescapes(p);
		sp = stalloc(sizeof(*sp));
		sp->text = p;
		*exparg.lastp = sp;
		exparg.lastp = &sp->next;
	}
	ifsfree();
	*exparg.lastp = NULL;
	if (exparg.list) {
		*arglist->lastp = exparg.list;
		arglist->lastp = exparg.lastp;
	}
}



/*
 * Perform variable and command substitution.
 * If EXP_GLOB is set, output CTLESC characters to allow for further processing.
 * If EXP_SPLIT is set, remember location of result for later,
 * Otherwise treat $@ like $* since no splitting will be performed.
 */

STATIC const char *
argstr(const char *p, int flag)
{
	char c;
	int quotes = flag & (EXP_GLOB | EXP_CASE | EXP_REDIR);	/* do CTLESC */
	int firsteq = 1;
	const char *ifs = NULL;
	int ifs_split = EXP_IFS_SPLIT;

	if (flag & EXP_IFS_SPLIT)
		ifs = ifsval();

	CTRACE(DBG_EXPAND, ("argstr(\"%s\", %#x) quotes=%#x\n", p,flag,quotes));

	if (*p == '~' && (flag & (EXP_TILDE | EXP_VARTILDE)))
		p = exptilde(p, flag);
	for (;;) {
		switch (c = *p++) {
		case '\0':
			NULLTERM_4_TRACE(expdest);
			VTRACE(DBG_EXPAND, ("argstr returning at \"\" "
			   "added \"%s\" to expdest\n", stackblock()));
			return p - 1;
		case CTLENDVAR: /* end of expanding yyy in ${xxx-yyy} */
		case CTLENDARI: /* end of a $(( )) string */
			NULLTERM_4_TRACE(expdest);
			VTRACE(DBG_EXPAND, ("argstr returning at \"%.6s\"..."
			    " after %2.2X; added \"%s\" to expdest\n", 
			    p, (c&0xff), stackblock()));
			return p;
		case CTLQUOTEMARK:
			/* "$@" syntax adherence hack */
			if (p[0] == CTLVAR && p[1] & VSQUOTE &&
			    p[2] == '@' && p[3] == '=')
				break;
			if ((flag & EXP_SPLIT) != 0)
				STPUTC(c, expdest);
			ifs_split = 0;
			break;
		case CTLNONL:
			if (flag & EXP_NL)
				STPUTC(c, expdest);
			line_number++;
			break;
		case CTLCNL:
			STPUTC('\n', expdest);	/* no line_number++ */
			break;
		case CTLQUOTEEND:
			ifs_split = EXP_IFS_SPLIT;
			break;
		case CTLESC:
			if (quotes)
				STPUTC(c, expdest);
			c = *p++;
			STPUTC(c, expdest);
			if (c == '\n')		/* should not happen, but ... */
				line_number++;
			break;
		case CTLVAR: {
#ifdef DEBUG
			unsigned int pos = expdest - stackblock();
#endif
			p = evalvar(p, (flag & ~EXP_IFS_SPLIT) | (flag & ifs_split));
			NULLTERM_4_TRACE(expdest);
			VTRACE(DBG_EXPAND, ("argstr evalvar "
			   "added \"%s\" to expdest\n", 
			   stackblock() + pos));
			break;
		}
		case CTLBACKQ:
		case CTLBACKQ|CTLQUOTE: {
#ifdef DEBUG
			unsigned int pos = expdest - stackblock();
#endif
			expbackq(argbackq->n, c & CTLQUOTE, flag);
			argbackq = argbackq->next;
			NULLTERM_4_TRACE(expdest);
			VTRACE(DBG_EXPAND, ("argstr expbackq added \"%s\" "
			   "to expdest\n", stackblock() + pos));
			break;
		}
		case CTLARI: {
#ifdef DEBUG
			unsigned int pos = expdest - stackblock();
#endif
			p = expari(p);
			NULLTERM_4_TRACE(expdest);
			VTRACE(DBG_EXPAND, ("argstr expari "
			   "+ \"%s\" to expdest p=\"%.5s...\"\n",
			   stackblock() + pos, p));
			break;
		}
		case ':':
		case '=':
			/*
			 * sort of a hack - expand tildes in variable
			 * assignments (after the first '=' and after ':'s).
			 */
			STPUTC(c, expdest);
			if (flag & EXP_VARTILDE && *p == '~') {
				if (c == '=') {
					if (firsteq)
						firsteq = 0;
					else
						break;
				}
				p = exptilde(p, flag);
			}
			break;
		default:
			if (c == '\n')
				line_number++;
			STPUTC(c, expdest);
			if (flag & ifs_split && strchr(ifs, c) != NULL) {
				/* We need to get the output split here... */
				recordregion(expdest - stackblock() - 1,
						expdest - stackblock(), 0);
			}
			break;
		}
	}
}

STATIC const char *
exptilde(const char *p, int flag)
{
	char c;
	const char *startp = p;
	struct passwd *pw;
	const char *home;
	int quotes = flag & (EXP_GLOB | EXP_CASE);
	char *user;
	struct stackmark smark;
#ifdef DEBUG
	unsigned int offs = expdest - stackblock();
#endif

	setstackmark(&smark);
	(void) grabstackstr(expdest);
	user = stackblock();		/* we will just borrow top of stack */

	while ((c = *++p) != '\0') {
		switch(c) {
		case CTLESC:		/* any of these occurring */
		case CTLVAR:		/* means ~ expansion */
		case CTLBACKQ:		/* does not happen at all */
		case CTLBACKQ | CTLQUOTE:
		case CTLARI:		/* just leave original unchanged */
		case CTLENDARI:
		case CTLQUOTEMARK:
		case '\n':
			popstackmark(&smark);
			return (startp);
		case CTLNONL:
			continue;
		case ':':
			if (!posix || flag & EXP_VARTILDE)
				goto done;
			break;
		case CTLENDVAR:
		case '/':
			goto done;
		}
		STPUTC(c, user);
	}
 done:
	STACKSTRNUL(user);
	user = stackblock();		/* to start of collected username */

	CTRACE(DBG_EXPAND, ("exptilde, found \"~%s\"", user));
	if (*user == '\0') {
		home = lookupvar("HOME");
		/*
		 * if HOME is unset, results are unspecified...
		 * we used to just leave the ~ unchanged, but
		 * (some) other shells do ... and this seems more useful.
		 */
		if (home == NULL && (pw = getpwuid(getuid())) != NULL)
			home = pw->pw_dir;
	} else if ((pw = getpwnam(user)) == NULL) {
		/*
		 * If user does not exist, results are undefined.
		 * so we can abort() here if we want, but let's not!
		 */
		home = NULL;
	} else
		home = pw->pw_dir;

	VTRACE(DBG_EXPAND, (" ->\"%s\"", home ? home : "<<NULL>>"));
	popstackmark(&smark);	/* now expdest is valid again */

	/*
	 * Posix XCU 2.6.1: The value of $HOME (for ~) or the initial
	 *		working directory from getpwnam() for ~user
	 * Nothing there about "except if a null string".  So do what it wants.
	 */
	if (home == NULL /* || *home == '\0' */) {
		CTRACE(DBG_EXPAND, (": returning unused \"%s\"\n", startp));
		return startp;
	} while ((c = *home++) != '\0') {
		if (quotes && SQSYNTAX[(int)c] == CCTL)
			STPUTC(CTLESC, expdest);
		STPUTC(c, expdest);
	}
	CTRACE(DBG_EXPAND, (": added %d \"%.*s\" returning \"%s\"\n",
	      expdest - stackblock() - offs, expdest - stackblock() - offs,
	      stackblock() + offs, p));

	return (p);
}


STATIC void 
removerecordregions(int endoff)
{

	VTRACE(DBG_EXPAND, ("removerecordregions(%d):", endoff));
	if (ifslastp == NULL) {
		VTRACE(DBG_EXPAND, (" none\n", endoff));
		return;
	}

	if (ifsfirst.endoff > endoff) {
		VTRACE(DBG_EXPAND, (" first(%d)", ifsfirst.endoff));
		while (ifsfirst.next != NULL) {
			struct ifsregion *ifsp;
			INTOFF;
			ifsp = ifsfirst.next->next;
			ckfree(ifsfirst.next);
			ifsfirst.next = ifsp;
			INTON;
		}
		if (ifsfirst.begoff > endoff)
			ifslastp = NULL;
		else {
			VTRACE(DBG_EXPAND,("->(%d,%d)",ifsfirst.begoff,endoff));
			ifslastp = &ifsfirst;
			ifsfirst.endoff = endoff;
		}
		VTRACE(DBG_EXPAND, ("\n"));
		return;
	}

	ifslastp = &ifsfirst;
	while (ifslastp->next && ifslastp->next->begoff < endoff)
		ifslastp=ifslastp->next;
	VTRACE(DBG_EXPAND, (" found(%d,%d)", ifslastp->begoff,ifslastp->endoff));
	while (ifslastp->next != NULL) {
		struct ifsregion *ifsp;
		INTOFF;
		ifsp = ifslastp->next->next;
		ckfree(ifslastp->next);
		ifslastp->next = ifsp;
		INTON;
	}
	if (ifslastp->endoff > endoff)
		ifslastp->endoff = endoff;
	VTRACE(DBG_EXPAND, ("->(%d,%d)", ifslastp->begoff,ifslastp->endoff));
}


/*
 * Expand arithmetic expression.
 *
 * In this incarnation, we start at the beginning (yes, "Let's start at the
 * very beginning.  A very good place to start.") and collect the expression
 * until the end - which means expanding anything contained within.
 *
 * Fortunately, argstr() just happens to do that for us...
 */
STATIC const char *
expari(const char *p)
{
	char *q, *start;
	intmax_t result;
	int adjustment;
	int begoff;
	int quoted;
	struct stackmark smark;

	/*	ifsfree(); */

	/*
	 * SPACE_NEEDED is enough for all possible digits (rounded up)
	 * plus possible "-", and the terminating '\0', hence, plus 2
	 *
	 * The calculation produces the number of bytes needed to
	 * represent the biggest possible value, in octal.  We only
	 * generate decimal, which takes (often) less digits (never more)
	 * so this is safe, if occasionally slightly wasteful.
	 */
#define SPACE_NEEDED ((int)((sizeof(intmax_t) * CHAR_BIT + 2) / 3 + 2))

	quoted = *p++ == '"';
	begoff = expdest - stackblock();
	VTRACE(DBG_EXPAND, ("expari%s: \"%s\" begoff %d\n",
	    quoted ? "(quoted)" : "", p, begoff));

	p = argstr(p, EXP_NL);			/* expand $(( )) string */
	STPUTC('\0', expdest);
	start = stackblock() + begoff;

	removerecordregions(begoff);		/* nothing there is kept */
	rmescapes_nl(start);		/* convert CRTNONL back into \n's */

	setstackmark(&smark);
	q = grabstackstr(expdest);	/* keep the expression while eval'ing */
	result = arith(start, line_number);
	popstackmark(&smark);		/* return the stack to before grab */

	start = stackblock() + begoff;		/* block may have moved */
	adjustment = expdest - start;
	STADJUST(-adjustment, expdest);		/* remove the argstr() result */

	CHECKSTRSPACE(SPACE_NEEDED, expdest);	/* nb: stack block might move */
	fmtstr(expdest, SPACE_NEEDED, "%"PRIdMAX, result);

	for (q = expdest; *q++ != '\0'; )	/* find end of what we added */
		;

	if (quoted == 0)			/* allow weird splitting */
		recordregion(begoff, begoff + q - 1 - expdest, 0);
	adjustment = q - expdest - 1;
	STADJUST(adjustment, expdest);		/* move expdest to end */
	VTRACE(DBG_EXPAND, ("expari: adding %d \"%s\", returning \"%.5s...\"\n",
	    adjustment, stackblock() + begoff, p));

	return p;
}


/*
 * Expand stuff in backwards quotes (these days, any command substitution).
 */

STATIC void
expbackq(union node *cmd, int quoted, int flag)
{
	struct backcmd in;
	int i;
	char buf[128];
	char *p;
	char *dest = expdest;	/* expdest may be reused by eval, use an alt */
	struct ifsregion saveifs, *savelastp;
	struct nodelist *saveargbackq;
	char lastc;
	int startloc = dest - stackblock();
	char const *syntax = quoted? DQSYNTAX : BASESYNTAX;
	int saveherefd;
	int quotes = flag & (EXP_GLOB | EXP_CASE);
	int nnl;
	struct stackmark smark;

	VTRACE(DBG_EXPAND, ("expbackq( ..., q=%d flag=%#x) have %d\n",
	    quoted, flag, startloc));
	INTOFF;
	saveifs = ifsfirst;
	savelastp = ifslastp;
	saveargbackq = argbackq;
	saveherefd = herefd;
	herefd = -1;

	setstackmark(&smark);	/* preserve the stack */
	p = grabstackstr(dest);	/* save what we have there currently */
	evalbackcmd(cmd, &in);	/* evaluate the $( ) tree (using stack) */
	popstackmark(&smark);	/* and return stack to when we entered */

	ifsfirst = saveifs;
	ifslastp = savelastp;
	argbackq = saveargbackq;
	herefd = saveherefd;

	p = in.buf;		/* now extract the results */
	nnl = 0;		/* dropping trailing \n's */
	for (;;) {
		if (--in.nleft < 0) {
			if (in.fd < 0)
				break;
			INTON;
			while ((i = read(in.fd, buf, sizeof buf)) < 0 && errno == EINTR)
				continue;
			INTOFF;
			VTRACE(DBG_EXPAND, ("expbackq: read returns %d\n", i));
			if (i <= 0)
				break;
			p = buf;
			in.nleft = i - 1;
		}
		lastc = *p++;
		if (lastc != '\0') {
			if (lastc == '\n')	/* don't save \n yet */
				nnl++;		/* it might be trailing */
			else {
				/*
				 * We have something other than \n
				 *
				 * Before saving it, we need to insert
				 * any \n's that we have just skipped.
				 */

				/* XXX
				 * this hack is just because our
				 * CHECKSTRSPACE() is lazy, and only
				 * ever grows the stack once, even
				 * if that does not allocate the space
				 * we requested.  ie: safe for small
				 * requests, but not large ones.
				 * FIXME someday...
				 */
				if (nnl < 20) {
					CHECKSTRSPACE(nnl + 2, dest);
					while (nnl > 0) {
						nnl--;
						USTPUTC('\n', dest);
					}
				} else {
					/* The slower, safer, way */
					while (nnl > 0) {
						nnl--;
						STPUTC('\n', dest);
					}
					CHECKSTRSPACE(2, dest);
				}
				if (quotes && syntax[(int)lastc] == CCTL)
					USTPUTC(CTLESC, dest);
				USTPUTC(lastc, dest);
			}
		}
	}

	if (in.fd >= 0)
		close(in.fd);
	if (in.buf)
		ckfree(in.buf);
	if (in.jp)
		back_exitstatus = waitforjob(in.jp);
	if (quoted == 0)
		recordregion(startloc, dest - stackblock(), 0);
	CTRACE(DBG_EXPAND, ("evalbackq: size=%d: \"%.*s\"\n",
		(int)((dest - stackblock()) - startloc),
		(int)((dest - stackblock()) - startloc),
		stackblock() + startloc));

	expdest = dest;		/* all done, expdest is all ours again */
	INTON;
}


STATIC int
subevalvar(const char *p, const char *str, int subtype, int startloc,
    int varflags)
{
	char *startp;
	int saveherefd = herefd;
	struct nodelist *saveargbackq = argbackq;
	int amount;

	herefd = -1;
	VTRACE(DBG_EXPAND, ("subevalvar(%d) \"%.20s\" ${%.*s} sloc=%d vf=%x\n",
	    subtype, p, p-str, str, startloc, varflags));
	argstr(p, subtype == VSASSIGN ? EXP_VARTILDE : EXP_TILDE);
	STACKSTRNUL(expdest);
	herefd = saveherefd;
	argbackq = saveargbackq;
	startp = stackblock() + startloc;

	switch (subtype) {
	case VSASSIGN:
		setvar(str, startp, 0);
		amount = startp - expdest;	/* remove what argstr added */
		STADJUST(amount, expdest);
		varflags &= ~VSNUL;	/*XXX Huh?   What's that achieve? */
		return 1;			/* go back and eval var again */

	case VSQUESTION:
		if (*p != CTLENDVAR) {
			outfmt(&errout, "%s\n", startp);
			error(NULL);
		}
		error("%.*s: parameter %snot set",
		      (int)(p - str - 1),
		      str, (varflags & VSNUL) ? "null or "
					      : nullstr);
		/* NOTREACHED */

	default:
		abort();
	}
}

STATIC int
subevalvar_trim(const char *p, int strloc, int subtype, int startloc,
    int varflags, int quotes)
{
	char *startp;
	char *str;
	char *loc = NULL;
	char *q;
	int c = 0;
	int saveherefd = herefd;
	struct nodelist *saveargbackq = argbackq;
	int amount;

	herefd = -1;
	switch (subtype) {
	case VSTRIMLEFT:
	case VSTRIMLEFTMAX:
	case VSTRIMRIGHT:
	case VSTRIMRIGHTMAX:
		break;
	default:
		abort();
		break;
	}

	VTRACE(DBG_EXPAND,
	("subevalvar_trim(\"%.9s\", STR@%d, SUBT=%d, start@%d, vf=%x, q=%x)\n",
		p, strloc, subtype, startloc, varflags, quotes));

	argstr(p, (varflags & (VSQUOTE|VSPATQ)) == VSQUOTE ? 0 : EXP_CASE);
	STACKSTRNUL(expdest);
	herefd = saveherefd;
	argbackq = saveargbackq;
	startp = stackblock() + startloc;
	str = stackblock() + strloc;

	switch (subtype) {

	case VSTRIMLEFT:
		for (loc = startp; loc < str; loc++) {
			c = *loc;
			*loc = '\0';
			if (patmatch(str, startp, quotes))
				goto recordleft;
			*loc = c;
			if (quotes && *loc == CTLESC)
			        loc++;
		}
		return 0;

	case VSTRIMLEFTMAX:
		for (loc = str - 1; loc >= startp;) {
			c = *loc;
			*loc = '\0';
			if (patmatch(str, startp, quotes))
				goto recordleft;
			*loc = c;
			loc--;
			if (quotes && loc > startp &&
			    *(loc - 1) == CTLESC) {
				for (q = startp; q < loc; q++)
					if (*q == CTLESC)
						q++;
				if (q > loc)
					loc--;
			}
		}
		return 0;

	case VSTRIMRIGHT:
	        for (loc = str - 1; loc >= startp;) {
			if (patmatch(str, loc, quotes))
				goto recordright;
			loc--;
			if (quotes && loc > startp &&
			    *(loc - 1) == CTLESC) { 
				for (q = startp; q < loc; q++)
					if (*q == CTLESC)
						q++;
				if (q > loc)
					loc--;
			}
		}
		return 0;

	case VSTRIMRIGHTMAX:
		for (loc = startp; loc < str - 1; loc++) {
			if (patmatch(str, loc, quotes))
				goto recordright;
			if (quotes && *loc == CTLESC)
			        loc++;
		}
		return 0;

	default:
		abort();
	}

recordleft:
	*loc = c;
	amount = ((str - 1) - (loc - startp)) - expdest;
	STADJUST(amount, expdest);
	while (loc != str - 1)
		*startp++ = *loc++;
	return 1;

recordright:
	amount = loc - expdest;
	STADJUST(amount, expdest);
	STPUTC('\0', expdest);
	STADJUST(-1, expdest);
	return 1;
}


/*
 * Expand a variable, and return a pointer to the next character in the
 * input string.
 */

STATIC const char *
evalvar(const char *p, int flag)
{
	int subtype;
	int varflags;
	const char *var;
	char *val;
	int patloc;
	int c;
	int set;
	int special;
	int startloc;
	int varlen;
	int apply_ifs;
	int quotes = flag & (EXP_GLOB | EXP_CASE | EXP_REDIR);

	varflags = (unsigned char)*p++;
	subtype = varflags & VSTYPE;
	var = p;
	special = !is_name(*p);
	p = strchr(p, '=') + 1;

	CTRACE(DBG_EXPAND,
	    ("evalvar \"%.*s\", flag=%#X quotes=%#X vf=%#X subtype=%X\n",
	    p - var - 1, var, flag, quotes, varflags, subtype));

 again: /* jump here after setting a variable with ${var=text} */
	if (varflags & VSLINENO) {
		if (line_num.flags & VUNSET) {
			set = 0;
			val = NULL;
		} else {
			set = 1;
			special = p - var;
			val = NULL;
		}
	} else if (special) {
		set = varisset(var, varflags & VSNUL);
		val = NULL;
	} else {
		val = lookupvar(var);
		if (val == NULL || ((varflags & VSNUL) && val[0] == '\0')) {
			val = NULL;
			set = 0;
		} else
			set = 1;
	}

	varlen = 0;
	startloc = expdest - stackblock();

	if (!set && uflag && *var != '@' && *var != '*') {
		switch (subtype) {
		case VSNORMAL:
		case VSTRIMLEFT:
		case VSTRIMLEFTMAX:
		case VSTRIMRIGHT:
		case VSTRIMRIGHTMAX:
		case VSLENGTH:
			error("%.*s: parameter not set",
			    (int)(p - var - 1), var);
			/* NOTREACHED */
		}
	}

	if (set && subtype != VSPLUS) {
		/* insert the value of the variable */
		if (special) {
			if (varflags & VSLINENO) {
				/*
				 * The LINENO hack (expansion part)
				 */
				while (--special > 0) {
/*						not needed, it is a number...
					if (quotes && syntax[(int)*var] == CCTL)
						STPUTC(CTLESC, expdest);
*/
					STPUTC(*var++, expdest);
				}
			} else
				varvalue(var, varflags&VSQUOTE, subtype, flag);
			if (subtype == VSLENGTH) {
				varlen = expdest - stackblock() - startloc;
				STADJUST(-varlen, expdest);
			}
		} else {
			char const *syntax = (varflags & VSQUOTE) ? DQSYNTAX
								  : BASESYNTAX;

			if (subtype == VSLENGTH) {
				for (;*val; val++)
					varlen++;
			} else {
				while (*val) {
					if (quotes && syntax[(int)*val] == CCTL)
						STPUTC(CTLESC, expdest);
					STPUTC(*val++, expdest);
				}
			}
		}
	}


	if (flag & EXP_IN_QUOTES)
		apply_ifs = 0;
	else if (varflags & VSQUOTE) {
		if  (*var == '@' && shellparam.nparam != 1)
		    apply_ifs = 1;
		else {
		    /*
		     * Mark so that we don't apply IFS if we recurse through
		     * here expanding $bar from "${foo-$bar}".
		     */
		    flag |= EXP_IN_QUOTES;
		    apply_ifs = 0;
		}
	} else
		apply_ifs = 1;

	switch (subtype) {
	case VSLENGTH:
		expdest = cvtnum(varlen, expdest);
		break;

	case VSNORMAL:
		break;

	case VSPLUS:
		set = !set;
		/* FALLTHROUGH */
	case VSMINUS:
		if (!set) {
		        argstr(p, flag | (apply_ifs ? EXP_IFS_SPLIT : 0));
			/*
			 * ${x-a b c} doesn't get split, but removing the
			 * 'apply_ifs = 0' apparently breaks ${1+"$@"}..
			 * ${x-'a b' c} should generate 2 args.
			 */
			if (*p != CTLENDVAR)
			/* We should have marked stuff already */
				apply_ifs = 0;
		}
		break;

	case VSTRIMLEFT:
	case VSTRIMLEFTMAX:
	case VSTRIMRIGHT:
	case VSTRIMRIGHTMAX:
		if (!set) {
			set = 1;  /* allow argbackq to be advanced if needed */
			break;
		}
		/*
		 * Terminate the string and start recording the pattern
		 * right after it
		 */
		STPUTC('\0', expdest);
		patloc = expdest - stackblock();
		if (subevalvar_trim(p, patloc, subtype, startloc, varflags,
		    quotes) == 0) {
			int amount = (expdest - stackblock() - patloc) + 1;
			STADJUST(-amount, expdest);
		}
		/* Remove any recorded regions beyond start of variable */
		removerecordregions(startloc);
		apply_ifs = 1;
		break;

	case VSASSIGN:
	case VSQUESTION:
		if (set)
			break;
		if (subevalvar(p, var, subtype, startloc, varflags)) {
			/* if subevalvar() returns, it always returns 1 */

			varflags &= ~VSNUL;
			/* 
			 * Remove any recorded regions beyond 
			 * start of variable 
			 */
			removerecordregions(startloc);
			goto again;
		}
		apply_ifs = 0;		/* never executed */
		break;

	default:
		abort();
	}

	if (apply_ifs)
		recordregion(startloc, expdest - stackblock(),
			     varflags & VSQUOTE);

	if (subtype != VSNORMAL) {	/* skip to end of alternative */
		int nesting = 1;
		for (;;) {
			if ((c = *p++) == CTLESC)
				p++;
			else if (c == CTLNONL)
				;
			else if (c == CTLBACKQ || c == (CTLBACKQ|CTLQUOTE)) {
				if (set)
					argbackq = argbackq->next;
			} else if (c == CTLVAR) {
				if ((*p++ & VSTYPE) != VSNORMAL)
					nesting++;
			} else if (c == CTLENDVAR) {
				if (--nesting == 0)
					break;
			}
		}
	}
	return p;
}



/*
 * Test whether a special parameter is set.
 */

STATIC int
varisset(const char *name, int nulok)
{
	if (*name == '!')
		return backgndpid != -1;
	else if (*name == '@' || *name == '*') {
		if (*shellparam.p == NULL)
			return 0;

		if (nulok) {
			char **av;

			for (av = shellparam.p; *av; av++)
				if (**av != '\0')
					return 1;
			return 0;
		}
	} else if (is_digit(*name)) {
		char *ap;
		long num;

		/*
		 * handle overflow sensibly (the *ap tests should never fail)
		 */
		errno = 0;
		num = strtol(name, &ap, 10);
		if (errno != 0 || (*ap != '\0' && *ap != '='))
			return 0;

		if (num == 0)
			ap = arg0;
		else if (num > shellparam.nparam)
			return 0;
		else
			ap = shellparam.p[num - 1];

		if (nulok && (ap == NULL || *ap == '\0'))
			return 0;
	}
	return 1;
}



/*
 * Add the value of a specialized variable to the stack string.
 */

STATIC void
varvalue(const char *name, int quoted, int subtype, int flag)
{
	int num;
	char *p;
	int i;
	int sep;
	char **ap;
	char const *syntax;

	if (subtype == VSLENGTH)	/* no magic required ... */
		flag &= ~EXP_FULL;

#define STRTODEST(p) \
	do {\
		if (flag & (EXP_GLOB | EXP_CASE) && subtype != VSLENGTH) { \
			syntax = quoted? DQSYNTAX : BASESYNTAX; \
			while (*p) { \
				if (syntax[(int)*p] == CCTL) \
					STPUTC(CTLESC, expdest); \
				STPUTC(*p++, expdest); \
			} \
		} else \
			while (*p) \
				STPUTC(*p++, expdest); \
	} while (0)


	switch (*name) {
	case '$':
		num = rootpid;
		break;
	case '?':
		num = exitstatus;
		break;
	case '#':
		num = shellparam.nparam;
		break;
	case '!':
		num = backgndpid;
		break;
	case '-':
		for (i = 0; i < option_flags; i++) {
			if (optlist[optorder[i]].val)
				STPUTC(optlist[optorder[i]].letter, expdest);
		}
		return;
	case '@':
		if (flag & EXP_SPLIT && quoted) {
			for (ap = shellparam.p ; (p = *ap++) != NULL ; ) {
				STRTODEST(p);
				if (*ap)
					/* A NUL separates args inside "" */
					STPUTC('\0', expdest);
			}
			return;
		}
		/* fall through */
	case '*':
		sep = ifsval()[0];
		for (ap = shellparam.p ; (p = *ap++) != NULL ; ) {
			STRTODEST(p);
			if (!*ap)
				break;
			if (sep) {
				if (quoted && (flag & (EXP_GLOB|EXP_CASE)) &&
				    (SQSYNTAX[sep] == CCTL || SQSYNTAX[sep] == CSBACK))
					STPUTC(CTLESC, expdest);
				STPUTC(sep, expdest);
			} else
			    if ((flag & (EXP_SPLIT|EXP_IN_QUOTES)) == EXP_SPLIT
			      && !quoted && **ap != '\0')
				STPUTC('\0', expdest);
		}
		return;
	default:
		if (is_digit(*name)) {
			long lnum;

			errno = 0;
			lnum = strtol(name, &p, 10);
			if (errno != 0 || (*p != '\0' && *p != '='))
				return;

			if (lnum == 0)
				p = arg0;
			else if (lnum > 0 && lnum <= shellparam.nparam)
				p = shellparam.p[lnum - 1];
			else
				return;
			STRTODEST(p);
		}
		return;
	}
	/*
	 * only the specials with an int value arrive here
	 */
	expdest = cvtnum(num, expdest);
}



/*
 * Record the fact that we have to scan this region of the
 * string for IFS characters.
 */

STATIC void
recordregion(int start, int end, int inquotes)
{
	struct ifsregion *ifsp;

	VTRACE(DBG_EXPAND, ("recordregion(%d,%d,%d)\n", start, end, inquotes));
	if (ifslastp == NULL) {
		ifsp = &ifsfirst;
	} else {
		if (ifslastp->endoff == start
		    && ifslastp->inquotes == inquotes) {
			/* extend previous area */
			ifslastp->endoff = end;
			return;
		}
		ifsp = (struct ifsregion *)ckmalloc(sizeof (struct ifsregion));
		ifslastp->next = ifsp;
	}
	ifslastp = ifsp;
	ifslastp->next = NULL;
	ifslastp->begoff = start;
	ifslastp->endoff = end;
	ifslastp->inquotes = inquotes;
}



/*
 * Break the argument string into pieces based upon IFS and add the
 * strings to the argument list.  The regions of the string to be
 * searched for IFS characters have been stored by recordregion.
 */
STATIC void
ifsbreakup(char *string, struct arglist *arglist)
{
	struct ifsregion *ifsp;
	struct strlist *sp;
	char *start;
	char *p;
	char *q;
	const char *ifs;
	const char *ifsspc;
	int had_param_ch = 0;

	start = string;

	VTRACE(DBG_EXPAND, ("ifsbreakup(\"%s\")", string)); /* misses \0's */
	if (ifslastp == NULL) {
		/* Return entire argument, IFS doesn't apply to any of it */
		VTRACE(DBG_EXPAND, ("no regions\n", string));
		sp = stalloc(sizeof(*sp));
		sp->text = start;
		*arglist->lastp = sp;
		arglist->lastp = &sp->next;
		return;
	}

	ifs = ifsval();

	for (ifsp = &ifsfirst; ifsp != NULL; ifsp = ifsp->next) {
		p = string + ifsp->begoff;
		VTRACE(DBG_EXPAND, (" !%.*s!(%d)", ifsp->endoff-ifsp->begoff,
		    p, ifsp->endoff-ifsp->begoff));
		while (p < string + ifsp->endoff) {
			had_param_ch = 1;
			q = p;
			if (*p == CTLNONL) {
				p++;
				continue;
			}
			if (*p == CTLESC)
				p++;
			if (ifsp->inquotes) {
				/* Only NULs (should be from "$@") end args */
				if (*p != 0) {
					p++;
					continue;
				}
				ifsspc = NULL;
				VTRACE(DBG_EXPAND, (" \\0 nxt:\"%s\" ", p));
			} else {
				if (!strchr(ifs, *p)) {
					p++;
					continue;
				}
				had_param_ch = 0;
				ifsspc = strchr(" \t\n", *p);

				/* Ignore IFS whitespace at start */
				if (q == start && ifsspc != NULL) {
					p++;
					start = p;
					continue;
				}
			}

			/* Save this argument... */
			*q = '\0';
			VTRACE(DBG_EXPAND, ("<%s>", start));
			sp = stalloc(sizeof(*sp));
			sp->text = start;
			*arglist->lastp = sp;
			arglist->lastp = &sp->next;
			p++;

			if (ifsspc != NULL) {
				/* Ignore further trailing IFS whitespace */
				for (; p < string + ifsp->endoff; p++) {
					q = p;
					if (*p == CTLNONL)
						continue;
					if (*p == CTLESC)
						p++;
					if (strchr(ifs, *p) == NULL) {
						p = q;
						break;
					}
					if (strchr(" \t\n", *p) == NULL) {
						p++;
						break;
					}
				}
			}
			start = p;
		}
	}

	/*
	 * Save anything left as an argument.
	 * Traditionally we have treated 'IFS=':'; set -- x$IFS' as
	 * generating 2 arguments, the second of which is empty.
	 * Some recent clarification of the Posix spec say that it
	 * should only generate one....
	 */
	if (had_param_ch || *start != 0) {
		VTRACE(DBG_EXPAND, (" T<%s>", start));
		sp = stalloc(sizeof(*sp));
		sp->text = start;
		*arglist->lastp = sp;
		arglist->lastp = &sp->next;
	}
	VTRACE(DBG_EXPAND, ("\n"));
}

STATIC void
ifsfree(void)
{
	while (ifsfirst.next != NULL) {
		struct ifsregion *ifsp;
		INTOFF;
		ifsp = ifsfirst.next->next;
		ckfree(ifsfirst.next);
		ifsfirst.next = ifsp;
		INTON;
	}
	ifslastp = NULL;
	ifsfirst.next = NULL;
}



/*
 * Expand shell metacharacters.  At this point, the only control characters
 * should be escapes.  The results are stored in the list exparg.
 */

char *expdir;


STATIC void
expandmeta(struct strlist *str, int flag)
{
	char *p;
	struct strlist **savelastp;
	struct strlist *sp;
	char c;
	/* TODO - EXP_REDIR */

	while (str) {
		p = str->text;
		for (;;) {			/* fast check for meta chars */
			if ((c = *p++) == '\0')
				goto nometa;
			if (c == '*' || c == '?' || c == '[' || c == '!')
				break;
		}
		savelastp = exparg.lastp;
		INTOFF;
		if (expdir == NULL) {
			int i = strlen(str->text);
			expdir = ckmalloc(i < 2048 ? 2048 : i); /* XXX */
		}

		expmeta(expdir, str->text);
		ckfree(expdir);
		expdir = NULL;
		INTON;
		if (exparg.lastp == savelastp) {
			/*
			 * no matches
			 */
 nometa:
			*exparg.lastp = str;
			rmescapes(str->text);
			exparg.lastp = &str->next;
		} else {
			*exparg.lastp = NULL;
			*savelastp = sp = expsort(*savelastp);
			while (sp->next != NULL)
				sp = sp->next;
			exparg.lastp = &sp->next;
		}
		str = str->next;
	}
}

STATIC void
add_args(struct strlist *str)
{
	while (str) {
		*exparg.lastp = str;
		rmescapes(str->text);
		exparg.lastp = &str->next;
		str = str->next;
	}
}


/*
 * Do metacharacter (i.e. *, ?, [...]) expansion.
 */

STATIC void
expmeta(char *enddir, char *name)
{
	char *p;
	const char *cp;
	char *q;
	char *start;
	char *endname;
	int metaflag;
	struct stat statb;
	DIR *dirp;
	struct dirent *dp;
	int atend;
	int matchdot;

	CTRACE(DBG_EXPAND|DBG_MATCH, ("expmeta(\"%s\")\n", name));
	metaflag = 0;
	start = name;
	for (p = name ; ; p++) {
		if (*p == '*' || *p == '?')
			metaflag = 1;
		else if (*p == '[') {
			q = p + 1;
			if (*q == '!')
				q++;
			for (;;) {
				while (*q == CTLQUOTEMARK || *q == CTLNONL)
					q++;
				if (*q == CTLESC)
					q++;
				if (*q == '/' || *q == '\0')
					break;
				if (*++q == ']') {
					metaflag = 1;
					break;
				}
			}
		} else if (*p == '!' && p[1] == '!'	&& (p == name || p[-1] == '/')) {
			metaflag = 1;
		} else if (*p == '\0')
			break;
		else if (*p == CTLQUOTEMARK || *p == CTLNONL)
			continue;
		else if (*p == CTLESC)
			p++;
		if (*p == '/') {
			if (metaflag)
				break;
			start = p + 1;
		}
	}
	if (metaflag == 0) {	/* we've reached the end of the file name */
		if (enddir != expdir)
			metaflag++;
		for (p = name ; ; p++) {
			if (*p == CTLQUOTEMARK || *p == CTLNONL)
				continue;
			if (*p == CTLESC)
				p++;
			*enddir++ = *p;
			if (*p == '\0')
				break;
		}
		if (metaflag == 0 || lstat(expdir, &statb) >= 0)
			addfname(expdir);
		return;
	}
	endname = p;
	if (start != name) {
		p = name;
		while (p < start) {
			while (*p == CTLQUOTEMARK || *p == CTLNONL)
				p++;
			if (*p == CTLESC)
				p++;
			*enddir++ = *p++;
		}
	}
	if (enddir == expdir) {
		cp = ".";
	} else if (enddir == expdir + 1 && *expdir == '/') {
		cp = "/";
	} else {
		cp = expdir;
		enddir[-1] = '\0';
	}
	if ((dirp = opendir(cp)) == NULL)
		return;
	if (enddir != expdir)
		enddir[-1] = '/';
	if (*endname == 0) {
		atend = 1;
	} else {
		atend = 0;
		*endname++ = '\0';
	}
	matchdot = 0;
	p = start;
	while (*p == CTLQUOTEMARK || *p == CTLNONL)
		p++;
	if (*p == CTLESC)
		p++;
	if (*p == '.')
		matchdot++;
	while (! int_pending() && (dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.' && ! matchdot)
			continue;
		if (patmatch(start, dp->d_name, 0)) {
			if (atend) {
				scopy(dp->d_name, enddir);
				addfname(expdir);
			} else {
				for (p = enddir, cp = dp->d_name;
				     (*p++ = *cp++) != '\0';)
					continue;
				p[-1] = '/';
				expmeta(p, endname);
			}
		}
	}
	closedir(dirp);
	if (! atend)
		endname[-1] = '/';
}


/*
 * Add a file name to the list.
 */

STATIC void
addfname(char *name)
{
	char *p;
	struct strlist *sp;

	p = stalloc(strlen(name) + 1);
	scopy(name, p);
	sp = stalloc(sizeof(*sp));
	sp->text = p;
	*exparg.lastp = sp;
	exparg.lastp = &sp->next;
}


/*
 * Sort the results of file name expansion.  It calculates the number of
 * strings to sort and then calls msort (short for merge sort) to do the
 * work.
 */

STATIC struct strlist *
expsort(struct strlist *str)
{
	int len;
	struct strlist *sp;

	len = 0;
	for (sp = str ; sp ; sp = sp->next)
		len++;
	return msort(str, len);
}


STATIC struct strlist *
msort(struct strlist *list, int len)
{
	struct strlist *p, *q = NULL;
	struct strlist **lpp;
	int half;
	int n;

	if (len <= 1)
		return list;
	half = len >> 1;
	p = list;
	for (n = half ; --n >= 0 ; ) {
		q = p;
		p = p->next;
	}
	q->next = NULL;			/* terminate first half of list */
	q = msort(list, half);		/* sort first half of list */
	p = msort(p, len - half);		/* sort second half */
	lpp = &list;
	for (;;) {
		if (strcmp(p->text, q->text) < 0) {
			*lpp = p;
			lpp = &p->next;
			if ((p = *lpp) == NULL) {
				*lpp = q;
				break;
			}
		} else {
			*lpp = q;
			lpp = &q->next;
			if ((q = *lpp) == NULL) {
				*lpp = p;
				break;
			}
		}
	}
	return list;
}


/*
 * See if a character matches a character class, starting at the first colon
 * of "[:class:]".
 * If a valid character class is recognized, a pointer to the next character
 * after the final closing bracket is stored into *end, otherwise a null
 * pointer is stored into *end.
 */
static int
match_charclass(const char *p, wchar_t chr, const char **end)
{
	char name[20];
	char *nameend;
	wctype_t cclass;

	*end = NULL;
	p++;
	nameend = strstr(p, ":]");
	if (nameend == NULL || nameend == p)	/* not a valid class */
		return 0;

	if (!is_alpha(*p) || strspn(p,		/* '_' is a local extension */
	    "0123456789"  "_"
	    "abcdefghijklmnopqrstuvwxyz"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ") != (size_t)(nameend - p))
		return 0;

	*end = nameend + 2;		/* committed to it being a char class */
	if ((size_t)(nameend - p) >= sizeof(name))	/* but too long */
		return 0;				/* so no match */
	memcpy(name, p, nameend - p);
	name[nameend - p] = '\0';
	cclass = wctype(name);
	/* An unknown class matches nothing but is valid nevertheless. */
	if (cclass == 0)
		return 0;
	return iswctype(chr, cclass);
}


/*
 * Returns true if the pattern matches the string.
 */

STATIC int
patmatch(const char *pattern, const char *string, int squoted)
{
	const char *p, *q, *end;
	const char *bt_p, *bt_q;
	char c;
	wchar_t wc, wc2;

	VTRACE(DBG_MATCH, ("patmatch(P=\"%s\", W=\"%s\"%s): ",
	    pattern, string, squoted ? ", SQ" : ""));
	p = pattern;
	q = string;
	bt_p = NULL;
	bt_q = NULL;
	for (;;) {
		switch (c = *p++) {
		case '\0':
			if (*q != '\0')
				goto backtrack;
			VTRACE(DBG_MATCH, ("match\n"));
			return 1;
		case CTLESC:
			if (squoted && *q == CTLESC)
				q++;
			if (*q++ != *p++)
				goto backtrack;
			break;
		case CTLQUOTEMARK:
		case CTLNONL:
			continue;
		case '?':
			if (squoted && *q == CTLESC)
				q++;
			if (*q++ == '\0') {
				VTRACE(DBG_MATCH, ("?fail\n"));
				return 0;
			}
			break;
		case '*':
			c = *p;
			while (c == CTLQUOTEMARK || c == '*')
				c = *++p;
			if (c != CTLESC &&  c != CTLQUOTEMARK && c != CTLNONL &&
			    c != '?' && c != '*' && c != '[') {
				while (*q != c) {
					if (squoted && *q == CTLESC &&
					    q[1] == c)
						break;
					if (*q == '\0') {
						VTRACE(DBG_MATCH, ("*fail\n"));
						return 0;
					}
					if (squoted && *q == CTLESC)
						q++;
					q++;
				}
			}
			/*
			 * First try the shortest match for the '*' that
			 * could work. We can forget any earlier '*' since
			 * there is no way having it match more characters
			 * can help us, given that we are already here.
			 */
			bt_p = p;
			bt_q = q;
			break;
		case '[': {
			const char *savep, *saveq, *endp;
			int invert, found;
			unsigned char chr;

			/*
			 * First quick check to see if there is a
			 * possible matching ']' - if not, then this
			 * is not a char class, and the '[' is just
			 * a literal '['.
			 *
			 * This check will not detect all non classes, but
			 * that's OK - It just means that we execute the
			 * harder code sometimes when it it cannot succeed.
			 */
			endp = p;
			if (*endp == '!' || *endp == '^')
				endp++;
			for (;;) {
				while (*endp == CTLQUOTEMARK || *endp==CTLNONL)
					endp++;
				if (*endp == '\0')
					goto dft;	/* no matching ] */
				if (*endp == CTLESC)
					endp++;
				if (*++endp == ']')
					break;
			}
			/* end shortcut */

			invert = 0;
			savep = p, saveq = q;
			invert = 0;
			if (*p == '!' || *p == '^') {
				invert++;
				p++;
			}
			found = 0;
			if (*q == '\0') {
				VTRACE(DBG_MATCH, ("[]fail\n"));
				return 0;
			}
			chr = (unsigned char)*q++;
			c = *p++;
			do {
				if (c == CTLQUOTEMARK || c == CTLNONL)
					continue;
				if (c == '\0') {
					p = savep, q = saveq;
					c = '[';
					goto dft;
				}
				if (c == '[' && *p == ':') {
					found |= match_charclass(p, chr, &end);
					if (end != NULL) {
						p = end;
						continue;
					}
				}
				if (c == CTLESC || c == '\\')
					c = *p++;
				wc = (unsigned char)c;
				if (*p == '-' && p[1] != ']') {
					p++;
					if (*p == CTLESC || *p == '\\')
						p++;
					wc2 = (unsigned char)*p++;
					if (   collate_range_cmp(chr, wc) >= 0
					    && collate_range_cmp(chr, wc2) <= 0
					   )
						found = 1;
				} else {
					if (chr == wc)
						found = 1;
				}
			} while ((c = *p++) != ']');
			if (found == invert)
				goto backtrack;
			break;
		}
dft:	        default:
			if (squoted && *q == CTLESC)
				q++;
			if (*q++ == c)
				break;
backtrack:
			/*
			 * If we have a mismatch (other than hitting the end
			 * of the string), go back to the last '*' seen and
			 * have it match one additional character.
			 */
			if (bt_p == NULL) {
				VTRACE(DBG_MATCH, ("BTP fail\n"));
				return 0;
			}
			if (*bt_q == '\0') {
				VTRACE(DBG_MATCH, ("BTQ fail\n"));
				return 0;
			}
			bt_q++;
			p = bt_p;
			q = bt_q;
			break;
		}
	}
}



/*
 * Remove any CTLESC or CTLNONL characters from a string.
 */

void
rmescapes(char *str)
{
	char *p, *q;

	p = str;
	while (*p != CTLESC && *p != CTLQUOTEMARK && *p != CTLNONL) {
		if (*p++ == '\0')
			return;
	}
	q = p;
	while (*p) {
		if (*p == CTLQUOTEMARK || *p == CTLNONL) {
			p++;
			continue;
		}
		if (*p == CTLCNL) {
			p++;
			*q++ = '\n';
			continue;
		}
		if (*p == CTLESC)
			p++;
		*q++ = *p++;
	}
	*q = '\0';
}

/*
 * and a special version for dealing with expressions to be parsed
 * by the arithmetic evaluator.   That needs to be able to count \n's
 * even ones that were \newline elided \n's, so we have to put the
 * latter back into the string - just being careful to put them only
 * at a place where white space can reasonably occur in the string
 * -- then the \n we insert will just be white space, and ignored
 * for all purposes except line counting.
 */

void
rmescapes_nl(char *str)
{
	char *p, *q;
	int nls = 0, holdnl = 0, holdlast;

	p = str;
	while (*p != CTLESC && *p != CTLQUOTEMARK && *p != CTLNONL) {
		if (*p++ == '\0')
			return;
	}
	if (p > str)	/* must reprocess char before stopper (if any) */
		--p;	/* so we do not place a \n badly */
	q = p;
	while (*p) {
		if (*p == CTLQUOTEMARK) {
			p++;
			continue;
		}
		if (*p == CTLNONL) {
			p++;
			nls++;
			continue;
		}
		if (*p == CTLCNL) {
			p++;
			*q++ = '\n';
			continue;
		}
		if (*p == CTLESC)
			p++;

		holdlast = holdnl;
		holdnl = is_in_name(*p);	/* letters, digits, _ */
		if (q == str || is_space(q[-1]) || (*p != '=' && q[-1] != *p)) {
			if (nls > 0 && holdnl != holdlast) {
				while (nls > 0)
					*q++ = '\n', nls--;
			}
		}
		*q++ = *p++;
	}
	while (--nls >= 0)
		*q++ = '\n';
	*q = '\0';
}



/*
 * See if a pattern matches in a case statement.
 */

int
casematch(union node *pattern, char *val)
{
	struct stackmark smark;
	int result;
	char *p;

	CTRACE(DBG_MATCH, ("casematch(P=\"%s\", W=\"%s\")\n",
	    pattern->narg.text, val));
	setstackmark(&smark);
	argbackq = pattern->narg.backquote;
	STARTSTACKSTR(expdest);
	ifslastp = NULL;
	argstr(pattern->narg.text, EXP_TILDE | EXP_CASE);
	STPUTC('\0', expdest);
	p = grabstackstr(expdest);
	result = patmatch(p, val, 0);
	popstackmark(&smark);
	return result;
}

/*
 * Our own itoa().   Assumes result buffer is on the stack
 */

STATIC char *
cvtnum(int num, char *buf)
{
	char temp[32];
	int neg = num < 0;
	char *p = temp + sizeof temp - 1;

	if (neg)
		num = -num;

	*p = '\0';
	do {
		*--p = num % 10 + '0';
	} while ((num /= 10) != 0 && p > temp + 1);

	if (neg)
		*--p = '-';

	while (*p)
		STPUTC(*p++, buf);
	return buf;
}

/*
 * Do most of the work for wordexp(3).
 */

int
wordexpcmd(int argc, char **argv)
{
	size_t len;
	int i;

	out1fmt("%d", argc - 1);
	out1c('\0');
	for (i = 1, len = 0; i < argc; i++)
		len += strlen(argv[i]);
	out1fmt("%zu", len);
	out1c('\0');
	for (i = 1; i < argc; i++) {
		out1str(argv[i]);
		out1c('\0');
	}
	return (0);
}
