/*	$NetBSD: parser.c,v 1.97 2016/02/22 19:42:46 christos Exp $	*/

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
static char sccsid[] = "@(#)parser.c	8.7 (Berkeley) 5/16/95";
#else
__RCSID("$NetBSD: parser.c,v 1.97 2016/02/22 19:42:46 christos Exp $");
#endif
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "shell.h"
#include "parser.h"
#include "nodes.h"
#include "expand.h"	/* defines rmescapes() */
#include "eval.h"	/* defines commandname */
#include "redir.h"	/* defines copyfd() */
#include "syntax.h"
#include "options.h"
#include "input.h"
#include "output.h"
#include "var.h"
#include "error.h"
#include "memalloc.h"
#include "mystring.h"
#include "alias.h"
#include "show.h"
#ifndef SMALL
#include "myhistedit.h"
#endif

/*
 * Shell command parser.
 */

#define EOFMARKLEN 79

/* values returned by readtoken */
#include "token.h"

#define OPENBRACE '{'
#define CLOSEBRACE '}'


struct heredoc {
	struct heredoc *next;	/* next here document in list */
	union node *here;		/* redirection node */
	char *eofmark;		/* string indicating end of input */
	int striptabs;		/* if set, strip leading tabs */
};



static int noalias = 0;		/* when set, don't handle aliases */
struct heredoc *heredoclist;	/* list of here documents to read */
int parsebackquote;		/* nonzero if we are inside backquotes */
int doprompt;			/* if set, prompt the user */
int needprompt;			/* true if interactive and at start of line */
int lasttoken;			/* last token read */
MKINIT int tokpushback;		/* last token pushed back */
char *wordtext;			/* text of last word returned by readtoken */
MKINIT int checkkwd;		/* 1 == check for kwds, 2 == also eat newlines */
struct nodelist *backquotelist;
union node *redirnode;
struct heredoc *heredoc;
int quoteflag;			/* set if (part of) last token was quoted */
int startlinno;			/* line # where last token started */
int funclinno;			/* line # where the current function started */


STATIC union node *list(int, int);
STATIC union node *andor(void);
STATIC union node *pipeline(void);
STATIC union node *command(void);
STATIC union node *simplecmd(union node **, union node *);
STATIC union node *makename(void);
STATIC void parsefname(void);
STATIC void parseheredoc(void);
STATIC int peektoken(void);
STATIC int readtoken(void);
STATIC int xxreadtoken(void);
STATIC int readtoken1(int, char const *, char *, int);
STATIC int noexpand(char *);
STATIC void synexpect(int) __dead;
STATIC void synerror(const char *) __dead;
STATIC void setprompt(int);


/*
 * Read and parse a command.  Returns NEOF on end of file.  (NULL is a
 * valid parse tree indicating a blank line.)
 */

union node *
parsecmd(int interact)
{
	int t;

	tokpushback = 0;
	doprompt = interact;
	if (doprompt)
		setprompt(1);
	else
		setprompt(0);
	needprompt = 0;
	t = readtoken();
	if (t == TEOF)
		return NEOF;
	if (t == TNL)
		return NULL;
	tokpushback++;
	return list(1, 0);
}


STATIC union node *
list(int nlflag, int erflag)
{
	union node *n1, *n2, *n3;
	int tok;
	TRACE(("list: entered\n"));

	checkkwd = 2;
	if (nlflag == 0 && tokendlist[peektoken()])
		return NULL;
	n1 = NULL;
	for (;;) {
		n2 = andor();
		tok = readtoken();
		if (tok == TBACKGND) {
			if (n2->type == NCMD || n2->type == NPIPE) {
				n2->ncmd.backgnd = 1;
			} else if (n2->type == NREDIR) {
				n2->type = NBACKGND;
			} else {
				n3 = (union node *)stalloc(sizeof (struct nredir));
				n3->type = NBACKGND;
				n3->nredir.n = n2;
				n3->nredir.redirect = NULL;
				n2 = n3;
			}
		}
		if (n1 == NULL) {
			n1 = n2;
		}
		else {
			n3 = (union node *)stalloc(sizeof (struct nbinary));
			n3->type = NSEMI;
			n3->nbinary.ch1 = n1;
			n3->nbinary.ch2 = n2;
			n1 = n3;
		}
		switch (tok) {
		case TBACKGND:
		case TSEMI:
			tok = readtoken();
			/* fall through */
		case TNL:
			if (tok == TNL) {
				parseheredoc();
				if (nlflag)
					return n1;
			} else {
				tokpushback++;
			}
			checkkwd = 2;
			if (tokendlist[peektoken()])
				return n1;
			break;
		case TEOF:
			if (heredoclist)
				parseheredoc();
			else
				pungetc();		/* push back EOF on input */
			return n1;
		default:
			if (nlflag || erflag)
				synexpect(-1);
			tokpushback++;
			return n1;
		}
	}
}



STATIC union node *
andor(void)
{
	union node *n1, *n2, *n3;
	int t;

	TRACE(("andor: entered\n"));
	n1 = pipeline();
	for (;;) {
		if ((t = readtoken()) == TAND) {
			t = NAND;
		} else if (t == TOR) {
			t = NOR;
		} else {
			tokpushback++;
			return n1;
		}
		n2 = pipeline();
		n3 = (union node *)stalloc(sizeof (struct nbinary));
		n3->type = t;
		n3->nbinary.ch1 = n1;
		n3->nbinary.ch2 = n2;
		n1 = n3;
	}
}



STATIC union node *
pipeline(void)
{
	union node *n1, *n2, *pipenode;
	struct nodelist *lp, *prev;
	int negate;

	TRACE(("pipeline: entered\n"));

	negate = 0;
	checkkwd = 2;
	while (readtoken() == TNOT) {
		TRACE(("pipeline: TNOT recognized\n"));
		negate = !negate;
	}
	tokpushback++;
	n1 = command();
	if (readtoken() == TPIPE) {
		pipenode = (union node *)stalloc(sizeof (struct npipe));
		pipenode->type = NPIPE;
		pipenode->npipe.backgnd = 0;
		lp = (struct nodelist *)stalloc(sizeof (struct nodelist));
		pipenode->npipe.cmdlist = lp;
		lp->n = n1;
		do {
			prev = lp;
			lp = (struct nodelist *)stalloc(sizeof (struct nodelist));
			lp->n = command();
			prev->next = lp;
		} while (readtoken() == TPIPE);
		lp->next = NULL;
		n1 = pipenode;
	}
	tokpushback++;
	if (negate) {
		TRACE(("negate pipeline\n"));
		n2 = (union node *)stalloc(sizeof (struct nnot));
		n2->type = NNOT;
		n2->nnot.com = n1;
		return n2;
	} else
		return n1;
}



STATIC union node *
command(void)
{
	union node *n1, *n2;
	union node *ap, **app;
	union node *cp, **cpp;
	union node *redir, **rpp;
	int t, negate = 0;

	TRACE(("command: entered\n"));

	checkkwd = 2;
	redir = NULL;
	n1 = NULL;
	rpp = &redir;

	/* Check for redirection which may precede command */
	while (readtoken() == TREDIR) {
		*rpp = n2 = redirnode;
		rpp = &n2->nfile.next;
		parsefname();
	}
	tokpushback++;

	while (readtoken() == TNOT) {
		TRACE(("command: TNOT recognized\n"));
		negate = !negate;
	}
	tokpushback++;

	switch (readtoken()) {
	case TIF:
		n1 = (union node *)stalloc(sizeof (struct nif));
		n1->type = NIF;
		n1->nif.test = list(0, 0);
		if (readtoken() != TTHEN)
			synexpect(TTHEN);
		n1->nif.ifpart = list(0, 0);
		n2 = n1;
		while (readtoken() == TELIF) {
			n2->nif.elsepart = (union node *)stalloc(sizeof (struct nif));
			n2 = n2->nif.elsepart;
			n2->type = NIF;
			n2->nif.test = list(0, 0);
			if (readtoken() != TTHEN)
				synexpect(TTHEN);
			n2->nif.ifpart = list(0, 0);
		}
		if (lasttoken == TELSE)
			n2->nif.elsepart = list(0, 0);
		else {
			n2->nif.elsepart = NULL;
			tokpushback++;
		}
		if (readtoken() != TFI)
			synexpect(TFI);
		checkkwd = 1;
		break;
	case TWHILE:
	case TUNTIL: {
		int got;
		n1 = (union node *)stalloc(sizeof (struct nbinary));
		n1->type = (lasttoken == TWHILE)? NWHILE : NUNTIL;
		n1->nbinary.ch1 = list(0, 0);
		if ((got=readtoken()) != TDO) {
TRACE(("expecting DO got %s %s\n", tokname[got], got == TWORD ? wordtext : ""));
			synexpect(TDO);
		}
		n1->nbinary.ch2 = list(0, 0);
		if (readtoken() != TDONE)
			synexpect(TDONE);
		checkkwd = 1;
		break;
	}
	case TFOR:
		if (readtoken() != TWORD || quoteflag || ! goodname(wordtext))
			synerror("Bad for loop variable");
		n1 = (union node *)stalloc(sizeof (struct nfor));
		n1->type = NFOR;
		n1->nfor.var = wordtext;
		if (readtoken() == TWORD && ! quoteflag && equal(wordtext, "in")) {
			app = &ap;
			while (readtoken() == TWORD) {
				n2 = (union node *)stalloc(sizeof (struct narg));
				n2->type = NARG;
				n2->narg.text = wordtext;
				n2->narg.backquote = backquotelist;
				*app = n2;
				app = &n2->narg.next;
			}
			*app = NULL;
			n1->nfor.args = ap;
			if (lasttoken != TNL && lasttoken != TSEMI)
				synexpect(-1);
		} else {
			static char argvars[5] = {
			    CTLVAR, VSNORMAL|VSQUOTE, '@', '=', '\0'
			};
			n2 = (union node *)stalloc(sizeof (struct narg));
			n2->type = NARG;
			n2->narg.text = argvars;
			n2->narg.backquote = NULL;
			n2->narg.next = NULL;
			n1->nfor.args = n2;
			/*
			 * Newline or semicolon here is optional (but note
			 * that the original Bourne shell only allowed NL).
			 */
			if (lasttoken != TNL && lasttoken != TSEMI)
				tokpushback++;
		}
		checkkwd = 2;
		if ((t = readtoken()) == TDO)
			t = TDONE;
		else if (t == TBEGIN)
			t = TEND;
		else
			synexpect(-1);
		n1->nfor.body = list(0, 0);
		if (readtoken() != t)
			synexpect(t);
		checkkwd = 1;
		break;
	case TCASE:
		n1 = (union node *)stalloc(sizeof (struct ncase));
		n1->type = NCASE;
		if (readtoken() != TWORD)
			synexpect(TWORD);
		n1->ncase.expr = n2 = (union node *)stalloc(sizeof (struct narg));
		n2->type = NARG;
		n2->narg.text = wordtext;
		n2->narg.backquote = backquotelist;
		n2->narg.next = NULL;
		while (readtoken() == TNL);
		if (lasttoken != TWORD || ! equal(wordtext, "in"))
			synerror("expecting \"in\"");
		cpp = &n1->ncase.cases;
		noalias = 1;
		checkkwd = 2, readtoken();
		/*
		 * Both ksh and bash accept 'case x in esac'
		 * so configure scripts started taking advantage of this.
		 * The page: http://pubs.opengroup.org/onlinepubs/\
		 * 009695399/utilities/xcu_chap02.html contradicts itself,
		 * as to if this is legal; the "Case Conditional Format"
		 * paragraph shows one case is required, but the "Grammar"
		 * section shows a grammar that explicitly allows the no
		 * case option.
		 */
		while (lasttoken != TESAC) {
			*cpp = cp = (union node *)stalloc(sizeof (struct nclist));
			if (lasttoken == TLP)
				readtoken();
			cp->type = NCLIST;
			app = &cp->nclist.pattern;
			for (;;) {
				*app = ap = (union node *)stalloc(sizeof (struct narg));
				ap->type = NARG;
				ap->narg.text = wordtext;
				ap->narg.backquote = backquotelist;
				if (checkkwd = 2, readtoken() != TPIPE)
					break;
				app = &ap->narg.next;
				readtoken();
			}
			ap->narg.next = NULL;
			noalias = 0;
			if (lasttoken != TRP) {
				synexpect(TRP);
			}
			cp->nclist.body = list(0, 0);

			checkkwd = 2;
			if ((t = readtoken()) != TESAC) {
				if (t != TENDCASE) {
					noalias = 0;
					synexpect(TENDCASE);
				} else {
					noalias = 1;
					checkkwd = 2;
					readtoken();
				}
			}
			cpp = &cp->nclist.next;
		}
		noalias = 0;
		*cpp = NULL;
		checkkwd = 1;
		break;
	case TLP:
		n1 = (union node *)stalloc(sizeof (struct nredir));
		n1->type = NSUBSHELL;
		n1->nredir.n = list(0, 0);
		n1->nredir.redirect = NULL;
		if (readtoken() != TRP)
			synexpect(TRP);
		checkkwd = 1;
		break;
	case TBEGIN:
		n1 = list(0, 0);
		if (readtoken() != TEND)
			synexpect(TEND);
		checkkwd = 1;
		break;
	/* Handle an empty command like other simple commands.  */
	case TSEMI:
		/*
		 * An empty command before a ; doesn't make much sense, and
		 * should certainly be disallowed in the case of `if ;'.
		 */
		if (!redir)
			synexpect(-1);
	case TAND:
	case TOR:
	case TNL:
	case TEOF:
	case TWORD:
	case TRP:
		tokpushback++;
		n1 = simplecmd(rpp, redir);
		goto checkneg;
	case TENDCASE:
		if (redir) {
			tokpushback++;
			goto checkneg;
		}
		/* FALLTHROUGH */
	default:
		synexpect(-1);
		/* NOTREACHED */
	}

	/* Now check for redirection which may follow command */
	while (readtoken() == TREDIR) {
		*rpp = n2 = redirnode;
		rpp = &n2->nfile.next;
		parsefname();
	}
	tokpushback++;
	*rpp = NULL;
	if (redir) {
		if (n1->type != NSUBSHELL) {
			n2 = (union node *)stalloc(sizeof (struct nredir));
			n2->type = NREDIR;
			n2->nredir.n = n1;
			n1 = n2;
		}
		n1->nredir.redirect = redir;
	}

checkneg:
	if (negate) {
		TRACE(("negate command\n"));
		n2 = (union node *)stalloc(sizeof (struct nnot));
		n2->type = NNOT;
		n2->nnot.com = n1;
		return n2;
	}
	else
		return n1;
}


STATIC union node *
simplecmd(union node **rpp, union node *redir)
{
	union node *args, **app;
	union node **orig_rpp = rpp;
	union node *n = NULL, *n2;
	int negate = 0;

	/* If we don't have any redirections already, then we must reset */
	/* rpp to be the address of the local redir variable.  */
	if (redir == 0)
		rpp = &redir;

	args = NULL;
	app = &args;
	/*
	 * We save the incoming value, because we need this for shell
	 * functions.  There can not be a redirect or an argument between
	 * the function name and the open parenthesis.
	 */
	orig_rpp = rpp;

	while (readtoken() == TNOT) {
		TRACE(("simplcmd: TNOT recognized\n"));
		negate = !negate;
	}
	tokpushback++;

	for (;;) {
		if (readtoken() == TWORD) {
			n = (union node *)stalloc(sizeof (struct narg));
			n->type = NARG;
			n->narg.text = wordtext;
			n->narg.backquote = backquotelist;
			*app = n;
			app = &n->narg.next;
		} else if (lasttoken == TREDIR) {
			*rpp = n = redirnode;
			rpp = &n->nfile.next;
			parsefname();	/* read name of redirection file */
		} else if (lasttoken == TLP && app == &args->narg.next
					    && rpp == orig_rpp) {
			/* We have a function */
			if (readtoken() != TRP)
				synexpect(TRP);
			funclinno = plinno;
			rmescapes(n->narg.text);
			if (!goodname(n->narg.text))
				synerror("Bad function name");
			n->type = NDEFUN;
			n->narg.next = command();
			funclinno = 0;
			goto checkneg;
		} else {
			tokpushback++;
			break;
		}
	}
	*app = NULL;
	*rpp = NULL;
	n = (union node *)stalloc(sizeof (struct ncmd));
	n->type = NCMD;
	n->ncmd.backgnd = 0;
	n->ncmd.args = args;
	n->ncmd.redirect = redir;

checkneg:
	if (negate) {
		TRACE(("negate simplecmd\n"));
		n2 = (union node *)stalloc(sizeof (struct nnot));
		n2->type = NNOT;
		n2->nnot.com = n;
		return n2;
	}
	else
		return n;
}

STATIC union node *
makename(void)
{
	union node *n;

	n = (union node *)stalloc(sizeof (struct narg));
	n->type = NARG;
	n->narg.next = NULL;
	n->narg.text = wordtext;
	n->narg.backquote = backquotelist;
	return n;
}

void
fixredir(union node *n, const char *text, int err)
{
	TRACE(("Fix redir %s %d\n", text, err));
	if (!err)
		n->ndup.vname = NULL;

	if (is_number(text))
		n->ndup.dupfd = number(text);
	else if (text[0] == '-' && text[1] == '\0')
		n->ndup.dupfd = -1;
	else {

		if (err)
			synerror("Bad fd number");
		else
			n->ndup.vname = makename();
	}
}


STATIC void
parsefname(void)
{
	union node *n = redirnode;

	if (readtoken() != TWORD)
		synexpect(-1);
	if (n->type == NHERE) {
		struct heredoc *here = heredoc;
		struct heredoc *p;
		int i;

		if (quoteflag == 0)
			n->type = NXHERE;
		TRACE(("Here document %d\n", n->type));
		if (here->striptabs) {
			while (*wordtext == '\t')
				wordtext++;
		}
		if (! noexpand(wordtext) || (i = strlen(wordtext)) == 0 || i > EOFMARKLEN)
			synerror("Illegal eof marker for << redirection");
		rmescapes(wordtext);
		here->eofmark = wordtext;
		here->next = NULL;
		if (heredoclist == NULL)
			heredoclist = here;
		else {
			for (p = heredoclist ; p->next ; p = p->next)
				continue;
			p->next = here;
		}
	} else if (n->type == NTOFD || n->type == NFROMFD) {
		fixredir(n, wordtext, 0);
	} else {
		n->nfile.fname = makename();
	}
}


/*
 * Input any here documents.
 */

STATIC void
parseheredoc(void)
{
	struct heredoc *here;
	union node *n;

	while (heredoclist) {
		here = heredoclist;
		heredoclist = here->next;
		if (needprompt) {
			setprompt(2);
			needprompt = 0;
		}
		readtoken1(pgetc(), here->here->type == NHERE? SQSYNTAX : DQSYNTAX,
				here->eofmark, here->striptabs);
		n = (union node *)stalloc(sizeof (struct narg));
		n->narg.type = NARG;
		n->narg.next = NULL;
		n->narg.text = wordtext;
		n->narg.backquote = backquotelist;
		here->here->nhere.doc = n;
	}
}

STATIC int
peektoken(void)
{
	int t;

	t = readtoken();
	tokpushback++;
	return (t);
}

STATIC int
readtoken(void)
{
	int t;
	int savecheckkwd = checkkwd;
#ifdef DEBUG
	int alreadyseen = tokpushback;
#endif
	struct alias *ap;

	top:
	t = xxreadtoken();

	if (checkkwd) {
		/*
		 * eat newlines
		 */
		if (checkkwd == 2) {
			checkkwd = 0;
			while (t == TNL) {
				parseheredoc();
				t = xxreadtoken();
			}
		} else
			checkkwd = 0;
		/*
		 * check for keywords and aliases
		 */
		if (t == TWORD && !quoteflag) {
			const char *const *pp;

			for (pp = parsekwd; *pp; pp++) {
				if (**pp == *wordtext && equal(*pp, wordtext)) {
					lasttoken = t = pp -
					    parsekwd + KWDOFFSET;
					TRACE(("keyword %s recognized\n", tokname[t]));
					goto out;
				}
			}
			if (!noalias &&
			    (ap = lookupalias(wordtext, 1)) != NULL) {
				pushstring(ap->val, strlen(ap->val), ap);
				checkkwd = savecheckkwd;
				goto top;
			}
		}
out:
		checkkwd = (t == TNOT) ? savecheckkwd : 0;
	}
	TRACE(("%stoken %s %s\n", alreadyseen ? "reread " : "", tokname[t], t == TWORD ? wordtext : ""));
	return (t);
}


/*
 * Read the next input token.
 * If the token is a word, we set backquotelist to the list of cmds in
 *	backquotes.  We set quoteflag to true if any part of the word was
 *	quoted.
 * If the token is TREDIR, then we set redirnode to a structure containing
 *	the redirection.
 * In all cases, the variable startlinno is set to the number of the line
 *	on which the token starts.
 *
 * [Change comment:  here documents and internal procedures]
 * [Readtoken shouldn't have any arguments.  Perhaps we should make the
 *  word parsing code into a separate routine.  In this case, readtoken
 *  doesn't need to have any internal procedures, but parseword does.
 *  We could also make parseoperator in essence the main routine, and
 *  have parseword (readtoken1?) handle both words and redirection.]
 */

#define RETURN(token)	return lasttoken = token

STATIC int
xxreadtoken(void)
{
	int c;

	if (tokpushback) {
		tokpushback = 0;
		return lasttoken;
	}
	if (needprompt) {
		setprompt(2);
		needprompt = 0;
	}
	startlinno = plinno;
	for (;;) {	/* until token or start of word found */
		c = pgetc_macro();
		switch (c) {
		case ' ': case '\t':
			continue;
		case '#':
			while ((c = pgetc()) != '\n' && c != PEOF)
				continue;
			pungetc();
			continue;
		case '\\':
			switch (pgetc()) {
			case '\n':
				startlinno = ++plinno;
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
				continue;
			case PEOF:
				RETURN(TEOF);
			default:
				pungetc();
				break;
			}
			goto breakloop;
		case '\n':
			plinno++;
			needprompt = doprompt;
			RETURN(TNL);
		case PEOF:
			RETURN(TEOF);
		case '&':
			if (pgetc() == '&')
				RETURN(TAND);
			pungetc();
			RETURN(TBACKGND);
		case '|':
			if (pgetc() == '|')
				RETURN(TOR);
			pungetc();
			RETURN(TPIPE);
		case ';':
			if (pgetc() == ';')
				RETURN(TENDCASE);
			pungetc();
			RETURN(TSEMI);
		case '(':
			RETURN(TLP);
		case ')':
			RETURN(TRP);
		default:
			goto breakloop;
		}
	}
breakloop:
	return readtoken1(c, BASESYNTAX, NULL, 0);
#undef RETURN
}



/*
 * If eofmark is NULL, read a word or a redirection symbol.  If eofmark
 * is not NULL, read a here document.  In the latter case, eofmark is the
 * word which marks the end of the document and striptabs is true if
 * leading tabs should be stripped from the document.  The argument firstc
 * is the first character of the input token or document.
 *
 * Because C does not have internal subroutines, I have simulated them
 * using goto's to implement the subroutine linkage.  The following macros
 * will run code that appears at the end of readtoken1.
 */

/*
 * We used to remember only the current syntax, variable nesting level,
 * double quote state for each var nexting level, and arith nesting
 * level (unrelated to var nesting) and one prev syntax when in arith
 * syntax.  This worked for simple cases, but can't handle arith inside
 * var expansion inside arith inside var with some quoted and some not.
 *
 * Inspired by FreeBSD's implementation (though it was the obvious way)
 * though implemented differently, we now have a stack that keeps track
 * of what we are doing now, and what we were doing previously.
 * Every time something changes, which will eventually end and should
 * revert to the previous state, we push this stack, and then pop it
 * again later (that is every ${} with an operator (to parse the word
 * or pattern that follows) ${x} and $x are too * simple to need it)
 * $(( )) $( ) and "...".   Always.   Really, always!
 *
 * The stack is implemented as one static (on the C stack) base block
 * containing LEVELS_PER_BLOCK (8) stack entries, which should be
 * enough for the vast majority of cases.  For torture tests, we
 * malloc more blocks as needed.  All accesses through the inline
 * functions below.
 */

/*
 * varnest & arinest will typically be 0 or 1
 * (varnest can increment in usages like ${x=${y}} but probably
 *  does not really need to)
 * parenlevel allows balancing parens inside a $(( )), it is reset
 * at each new nesting level ( $(( ( x + 3 ${unset-)} )) does not work.
 * quoted is special - we need to know 2 things ... are we inside "..."
 * (even if inherited from some previous nesting level) and was there
 * an opening '"' at this level (so the next will be closing).
 * "..." can span nexting levels, but cannot be opened in one and
 * closed in a different one.
 * To handle this, "quoted" has two fields, the bottom 4 (really 2)
 * bits are 0, 1, or 2, for un, single, and double quoted (single quoted
 * is really so special that this setting is not very important)
 * and 0x10 that indicates that an opening quote has been seen.
 * The bottom 4 bits are inherited, the 0x10 bit is not.
 */
struct tokenstate {
	const char *ts_syntax;
	unsigned short ts_parenlevel;	/* counters */
	unsigned short ts_varnest;	/* 64000 levels should be enough! */
	unsigned short ts_arinest;
	unsigned short ts_quoted;	/* 1 -> single, 2 -> double */
};

#define	NQ	0x00
#define	SQ	0x01
#define	DQ	0x02
#define	QF	0x0F
#define	QS	0x10

#define	LEVELS_PER_BLOCK	8
#define	VSS			volatile struct statestack

struct statestack {
	VSS *prev;		/* previous block in list */
	int cur;		/* which of our tokenstates is current */
	struct tokenstate tokenstate[LEVELS_PER_BLOCK];
};

static inline volatile struct tokenstate *
currentstate(VSS *stack)
{
	return &stack->tokenstate[stack->cur];
}

static inline volatile struct tokenstate *
prevstate(VSS *stack)
{
	if (stack->cur != 0)
		return &stack->tokenstate[stack->cur - 1];
	if (stack->prev == NULL)	/* cannot drop below base */
		return &stack->tokenstate[0];
	return &stack->prev->tokenstate[LEVELS_PER_BLOCK - 1];
}

static inline VSS *
bump_state_level(VSS *stack)
{
	volatile struct tokenstate *os, *ts;

	os = currentstate(stack);

	if (++stack->cur >= LEVELS_PER_BLOCK) {
		VSS *ss;

		ss = (VSS *)ckmalloc(sizeof (struct statestack));
		ss->cur = 0;
		ss->prev = stack;
		stack = ss;
	}

	ts = currentstate(stack);

	ts->ts_parenlevel = 0;	/* parens inside never match outside */

	ts->ts_quoted  = os->ts_quoted & QF;	/* these are default settings */
	ts->ts_varnest = os->ts_varnest;
	ts->ts_arinest = os->ts_arinest;	/* when appropriate	   */
	ts->ts_syntax  = os->ts_syntax;		/*    they will be altered */

	return stack;
}

static inline VSS *
drop_state_level(VSS *stack)
{
	if (stack->cur == 0) {
		VSS *ss;

		ss = stack;
		stack = ss->prev;
		if (stack == NULL)
			return ss;
		ckfree(__UNVOLATILE(ss));
	}
	--stack->cur;
	return stack;
}

static inline void
cleanup_state_stack(VSS *stack)
{
	while (stack->prev != NULL) {
		stack->cur = 0;
		stack = drop_state_level(stack);
	}
}

#define	CHECKEND()	{goto checkend; checkend_return:;}
#define	PARSEREDIR()	{goto parseredir; parseredir_return:;}
#define	PARSESUB()	{goto parsesub; parsesub_return:;}
#define	PARSEBACKQOLD()	{oldstyle = 1; goto parsebackq; parsebackq_oldreturn:;}
#define	PARSEBACKQNEW()	{oldstyle = 0; goto parsebackq; parsebackq_newreturn:;}
#define	PARSEARITH()	{goto parsearith; parsearith_return:;}

/*
 * The following macros all assume the existance of a local var "stack"
 * which contains a pointer to the current struct stackstate
 */

/*
 * These are macros rather than inline funcs to avoid code churn as much
 * as possible - they replace macros of the same name used previously.
 */
#define	ISDBLQUOTE()	(currentstate(stack)->ts_quoted & QS)
#define	SETDBLQUOTE()	(currentstate(stack)->ts_quoted = QS | DQ)
#define	CLRDBLQUOTE()	(currentstate(stack)->ts_quoted =		\
			    stack->cur != 0 || stack->prev ?		\
				prevstate(stack)->ts_quoted & QF : 0)

/*
 * This set are just to avoid excess typing and line lengths...
 * The ones that "look like" var names must be implemented to be lvalues
 */
#define	syntax		(currentstate(stack)->ts_syntax)
#define	parenlevel	(currentstate(stack)->ts_parenlevel)
#define	varnest		(currentstate(stack)->ts_varnest)
#define	arinest		(currentstate(stack)->ts_arinest)
#define	quoted		(currentstate(stack)->ts_quoted)
#define	TS_PUSH()	(stack = bump_state_level(stack))
#define	TS_POP()	(stack = drop_state_level(stack))

STATIC int
readtoken1(int firstc, char const *syn, char *eofmark, int striptabs)
{
	int c = firstc;
	char * volatile out;
	int len;
	char line[EOFMARKLEN + 1];
	struct nodelist *bqlist;
	volatile int quotef;
	volatile int oldstyle;
	VSS static_stack;
	VSS *stack = &static_stack;

	stack->prev = NULL;
	stack->cur = 0;

	syntax = syn;

	startlinno = plinno;
	varnest = 0;
	if (syntax == DQSYNTAX) {
		SETDBLQUOTE();
	}
	quotef = 0;
	bqlist = NULL;
	arinest = 0;
	parenlevel = 0;
	quoted = 0;

	STARTSTACKSTR(out);
	loop: {	/* for each line, until end of word */
#if ATTY
		if (c == '\034' && doprompt
		 && attyset() && ! equal(termval(), "emacs")) {
			attyline();
			if (syntax == BASESYNTAX)
				return readtoken();
			c = pgetc();
			goto loop;
		}
#endif
		CHECKEND();	/* set c to PEOF if at end of here document */
		for (;;) {	/* until end of line or end of word */
			CHECKSTRSPACE(4, out);	/* permit 4 calls to USTPUTC */
			switch(syntax[c]) {
			case CNL:	/* '\n' */
				if (syntax == BASESYNTAX)
					goto endword;	/* exit outer loop */
				USTPUTC(c, out);
				plinno++;
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
				c = pgetc();
				goto loop;		/* continue outer loop */
			case CWORD:
				USTPUTC(c, out);
				break;
			case CCTL:
				if (eofmark == NULL || ISDBLQUOTE())
					USTPUTC(CTLESC, out);
				USTPUTC(c, out);
				break;
			case CBACK:	/* backslash */
				c = pgetc();
				if (c == PEOF) {
					USTPUTC('\\', out);
					pungetc();
					break;
				}
				if (c == '\n') {
					plinno++;
					if (doprompt)
						setprompt(2);
					else
						setprompt(0);
					break;
				}
				quotef = 1;
				if (ISDBLQUOTE() && c != '\\' &&
				    c != '`' && c != '$' &&
				    (c != '"' || eofmark != NULL))
					USTPUTC('\\', out);
				if (SQSYNTAX[c] == CCTL)
					USTPUTC(CTLESC, out);
				else if (eofmark == NULL) {
					USTPUTC(CTLQUOTEMARK, out);
					USTPUTC(c, out);
					if (varnest != 0)
						USTPUTC(CTLQUOTEEND, out);
					break;
				}
				USTPUTC(c, out);
				break;
			case CSQUOTE:
				if (syntax != SQSYNTAX) {
					if (eofmark == NULL)
						USTPUTC(CTLQUOTEMARK, out);
					quotef = 1;
					TS_PUSH();
					syntax = SQSYNTAX;
					quoted = SQ;
					break;
				}
				if (eofmark != NULL && arinest == 0 &&
				    varnest == 0) {
					/* Ignore inside quoted here document */
					USTPUTC(c, out);
					break;
				}
				/* End of single quotes... */
				TS_POP();
				if (syntax == BASESYNTAX && varnest != 0)
					USTPUTC(CTLQUOTEEND, out);
				break;
			case CDQUOTE:
				if (eofmark != NULL && arinest == 0 &&
				    varnest == 0) {
					/* Ignore inside here document */
					USTPUTC(c, out);
					break;
				}
				quotef = 1;
				if (arinest) {
					if (ISDBLQUOTE()) {
						TS_POP();
					} else {
						TS_PUSH();
						syntax = DQSYNTAX;
						SETDBLQUOTE();
						USTPUTC(CTLQUOTEMARK, out);
					}
					break;
				}
				if (eofmark != NULL)
					break;
				if (ISDBLQUOTE()) {
					TS_POP();
					if (varnest != 0)
						USTPUTC(CTLQUOTEEND, out);
				} else {
					TS_PUSH();
					syntax = DQSYNTAX;
					SETDBLQUOTE();
					USTPUTC(CTLQUOTEMARK, out);
				}
				break;
			case CVAR:	/* '$' */
				PARSESUB();		/* parse substitution */
				break;
			case CENDVAR:	/* CLOSEBRACE */
				if (varnest > 0 && !ISDBLQUOTE()) {
					TS_POP();
					USTPUTC(CTLENDVAR, out);
				} else {
					USTPUTC(c, out);
				}
				break;
			case CLP:	/* '(' in arithmetic */
				parenlevel++;
				USTPUTC(c, out);
				break;
			case CRP:	/* ')' in arithmetic */
				if (parenlevel > 0) {
					USTPUTC(c, out);
					--parenlevel;
				} else {
					if (pgetc() == ')') {
						if (--arinest == 0) {
							TS_POP();
							USTPUTC(CTLENDARI, out);
						} else
							USTPUTC(')', out);
					} else {
						/*
						 * unbalanced parens
						 *  (don't 2nd guess - no error)
						 */
						pungetc();
						USTPUTC(')', out);
					}
				}
				break;
			case CBQUOTE:	/* '`' */
				PARSEBACKQOLD();
				break;
			case CEOF:
				goto endword;		/* exit outer loop */
			default:
				if (varnest == 0 && !ISDBLQUOTE())
					goto endword;	/* exit outer loop */
				USTPUTC(c, out);
			}
			c = pgetc_macro();
		}
	}
endword:
	if (syntax == ARISYNTAX) {
		cleanup_state_stack(stack);
		synerror("Missing '))'");
	}
	if (syntax != BASESYNTAX && /* ! parsebackquote && */ eofmark == NULL) {
		cleanup_state_stack(stack);
		synerror("Unterminated quoted string");
	}
	if (varnest != 0) {
		cleanup_state_stack(stack);
		startlinno = plinno;
		/* { */
		synerror("Missing '}'");
	}
	USTPUTC('\0', out);
	len = out - stackblock();
	out = stackblock();
	if (eofmark == NULL) {
		if ((c == '>' || c == '<')
		 && quotef == 0
		 && (*out == '\0' || is_number(out))) {
			PARSEREDIR();
			cleanup_state_stack(stack);
			return lasttoken = TREDIR;
		} else {
			pungetc();
		}
	}
	quoteflag = quotef;
	backquotelist = bqlist;
	grabstackblock(len);
	wordtext = out;
	cleanup_state_stack(stack);
	return lasttoken = TWORD;
/* end of readtoken routine */



/*
 * Check to see whether we are at the end of the here document.  When this
 * is called, c is set to the first character of the next input line.  If
 * we are at the end of the here document, this routine sets the c to PEOF.
 */

checkend: {
	if (eofmark) {
		if (striptabs) {
			while (c == '\t')
				c = pgetc();
		}
		if (c == *eofmark) {
			if (pfgets(line, sizeof line) != NULL) {
				char *p, *q;

				p = line;
				for (q = eofmark + 1 ; *q && *p == *q ; p++, q++)
					continue;
				if ((*p == '\0' || *p == '\n') && *q == '\0') {
					c = PEOF;
					plinno++;
					needprompt = doprompt;
				} else {
					pushstring(line, strlen(line), NULL);
				}
			}
		}
	}
	goto checkend_return;
}


/*
 * Parse a redirection operator.  The variable "out" points to a string
 * specifying the fd to be redirected.  The variable "c" contains the
 * first character of the redirection operator.
 */

parseredir: {
	char fd[64];
	union node *np;
	strlcpy(fd, out, sizeof(fd));

	np = (union node *)stalloc(sizeof (struct nfile));
	if (c == '>') {
		np->nfile.fd = 1;
		c = pgetc();
		if (c == '>')
			np->type = NAPPEND;
		else if (c == '|')
			np->type = NCLOBBER;
		else if (c == '&')
			np->type = NTOFD;
		else {
			np->type = NTO;
			pungetc();
		}
	} else {	/* c == '<' */
		np->nfile.fd = 0;
		switch (c = pgetc()) {
		case '<':
			if (sizeof (struct nfile) != sizeof (struct nhere)) {
				np = (union node *)stalloc(sizeof (struct nhere));
				np->nfile.fd = 0;
			}
			np->type = NHERE;
			heredoc = (struct heredoc *)stalloc(sizeof (struct heredoc));
			heredoc->here = np;
			if ((c = pgetc()) == '-') {
				heredoc->striptabs = 1;
			} else {
				heredoc->striptabs = 0;
				pungetc();
			}
			break;

		case '&':
			np->type = NFROMFD;
			break;

		case '>':
			np->type = NFROMTO;
			break;

		default:
			np->type = NFROM;
			pungetc();
			break;
		}
	}
	if (*fd != '\0')
		np->nfile.fd = number(fd);
	redirnode = np;
	goto parseredir_return;
}


/*
 * Parse a substitution.  At this point, we have read the dollar sign
 * and nothing else.
 */

parsesub: {
	char buf[10];
	int subtype;
	int typeloc;
	int flags;
	char *p;
	static const char types[] = "}-+?=";
	int i;
	int linno;

	c = pgetc();
	if (c != '(' && c != OPENBRACE && !is_name(c) && !is_special(c)) {
		USTPUTC('$', out);
		pungetc();
	} else if (c == '(') {	/* $(command) or $((arith)) */
		if (pgetc() == '(') {
			PARSEARITH();
		} else {
			pungetc();
			PARSEBACKQNEW();
		}
	} else {
		USTPUTC(CTLVAR, out);
		typeloc = out - stackblock();
		USTPUTC(VSNORMAL, out);
		subtype = VSNORMAL;
		flags = 0;
		if (c == OPENBRACE) {
			c = pgetc();
			if (c == '#') {
				if ((c = pgetc()) == CLOSEBRACE)
					c = '#';
				else
					subtype = VSLENGTH;
			}
			else
				subtype = 0;
		}
		if (is_name(c)) {
			p = out;
			do {
				STPUTC(c, out);
				c = pgetc();
			} while (is_in_name(c));
			if (out - p == 6 && strncmp(p, "LINENO", 6) == 0) {
				/* Replace the variable name with the
				 * current line number. */
				linno = plinno;
				if (funclinno != 0)
					linno -= funclinno - 1;
				snprintf(buf, sizeof(buf), "%d", linno);
				STADJUST(-6, out);
				for (i = 0; buf[i] != '\0'; i++)
					STPUTC(buf[i], out);
				flags |= VSLINENO;
			}
		} else if (is_digit(c)) {
			do {
				USTPUTC(c, out);
				c = pgetc();
			} while (is_digit(c));
		}
		else if (is_special(c)) {
			USTPUTC(c, out);
			c = pgetc();
		}
		else {
badsub:
			cleanup_state_stack(stack);
			synerror("Bad substitution");
		}

		STPUTC('=', out);
		if (subtype == 0) {
			switch (c) {
			case ':':
				flags |= VSNUL;
				c = pgetc();
				/*FALLTHROUGH*/
			default:
				p = strchr(types, c);
				if (p == NULL)
					goto badsub;
				subtype = p - types + VSNORMAL;
				break;
			case '%':
			case '#':
				{
					int cc = c;
					subtype = c == '#' ? VSTRIMLEFT :
							     VSTRIMRIGHT;
					c = pgetc();
					if (c == cc)
						subtype++;
					else
						pungetc();
					break;
				}
			}
		} else {
			pungetc();
		}
		if (ISDBLQUOTE() || arinest)
			flags |= VSQUOTE;
		*(stackblock() + typeloc) = subtype | flags;
		if (subtype != VSNORMAL) {
			TS_PUSH();
			varnest++;
			arinest = 0;
			if (subtype > VSASSIGN) {	/* # ## % %% */
				syntax = BASESYNTAX;
				CLRDBLQUOTE();
			}
		}
	}
	goto parsesub_return;
}


/*
 * Called to parse command substitutions.  Newstyle is set if the command
 * is enclosed inside $(...); nlpp is a pointer to the head of the linked
 * list of commands (passed by reference), and savelen is the number of
 * characters on the top of the stack which must be preserved.
 */

parsebackq: {
	struct nodelist **nlpp;
	int savepbq;
	union node *n;
	char *volatile str = NULL;
	struct jmploc jmploc;
	struct jmploc *volatile savehandler = NULL;
	int savelen;
	int saveprompt;

	savepbq = parsebackquote;
	if (setjmp(jmploc.loc)) {
		if (str)
			ckfree(str);
		cleanup_state_stack(stack);
		parsebackquote = 0;
		handler = savehandler;
		longjmp(handler->loc, 1);
	}
	INTOFF;
	str = NULL;
	savelen = out - stackblock();
	if (savelen > 0) {
		str = ckmalloc(savelen);
		memcpy(str, stackblock(), savelen);
	}
	savehandler = handler;
	handler = &jmploc;
	INTON;
        if (oldstyle) {
                /* We must read until the closing backquote, giving special
                   treatment to some slashes, and then push the string and
                   reread it as input, interpreting it normally.  */
                char *pout;
                int pc;
                int psavelen;
                char *pstr;

		/*
		 * Because the entire `...` is read here, we don't
		 * need to bother the state stack.  That will be used
		 * (as appropriate) when the processed string is re-read.
		 */
                STARTSTACKSTR(pout);
		for (;;) {
			if (needprompt) {
				setprompt(2);
				needprompt = 0;
			}
			switch (pc = pgetc()) {
			case '`':
				goto done;

			case '\\':
                                if ((pc = pgetc()) == '\n') {
					plinno++;
					if (doprompt)
						setprompt(2);
					else
						setprompt(0);
					/*
					 * If eating a newline, avoid putting
					 * the newline into the new character
					 * stream (via the STPUTC after the
					 * switch).
					 */
					continue;
				}
                                if (pc != '\\' && pc != '`' && pc != '$'
                                    && (!ISDBLQUOTE() || pc != '"'))
                                        STPUTC('\\', pout);
				break;

			case '\n':
				plinno++;
				needprompt = doprompt;
				break;

			case PEOF:
			        startlinno = plinno;
				synerror("EOF in backquote substitution");
 				break;

			default:
				break;
			}
			STPUTC(pc, pout);
                }
done:
                STPUTC('\0', pout);
                psavelen = pout - stackblock();
                if (psavelen > 0) {
			pstr = grabstackstr(pout);
			setinputstring(pstr, 1);
                }
        }
	nlpp = &bqlist;
	while (*nlpp)
		nlpp = &(*nlpp)->next;
	*nlpp = (struct nodelist *)stalloc(sizeof (struct nodelist));
	(*nlpp)->next = NULL;
	parsebackquote = oldstyle;

	if (oldstyle) {
		saveprompt = doprompt;
		doprompt = 0;
	} else
		saveprompt = 0;

	n = list(0, oldstyle);

	if (oldstyle)
		doprompt = saveprompt;
	else {
		if (readtoken() != TRP) {
			cleanup_state_stack(stack);
			synexpect(TRP);
		}
	}

	(*nlpp)->n = n;
        if (oldstyle) {
		/*
		 * Start reading from old file again, ignoring any pushed back
		 * tokens left from the backquote parsing
		 */
                popfile();
		tokpushback = 0;
	}
	while (stackblocksize() <= savelen)
		growstackblock();
	STARTSTACKSTR(out);
	if (str) {
		memcpy(out, str, savelen);
		STADJUST(savelen, out);
		INTOFF;
		ckfree(str);
		str = NULL;
		INTON;
	}
	parsebackquote = savepbq;
	handler = savehandler;
	if (arinest || ISDBLQUOTE())
		USTPUTC(CTLBACKQ | CTLQUOTE, out);
	else
		USTPUTC(CTLBACKQ, out);
	if (oldstyle)
		goto parsebackq_oldreturn;
	else
		goto parsebackq_newreturn;
}

/*
 * Parse an arithmetic expansion (indicate start of one and set state)
 */
parsearith: {

	if (syntax == ARISYNTAX) {
		/*
		 * we collapse embedded arithmetic expansion to
		 * parentheses, which should be equivalent
		 */
		USTPUTC('(', out);
		USTPUTC('(', out);
		/*
		 * Need 2 of them because there will (should be)
		 * two closing ))'s to follow later.
		 */
		parenlevel += 2;
	} else {
		TS_PUSH();
		syntax = ARISYNTAX;
		++arinest;
		varnest = 0;

		USTPUTC(CTLARI, out);
		if (ISDBLQUOTE())
			USTPUTC('"',out);
		else
			USTPUTC(' ',out);
	}
	goto parsearith_return;
}

} /* end of readtoken */



#ifdef mkinit
RESET {
	tokpushback = 0;
	checkkwd = 0;
}
#endif

/*
 * Returns true if the text contains nothing to expand (no dollar signs
 * or backquotes).
 */

STATIC int
noexpand(char *text)
{
	char *p;
	char c;

	p = text;
	while ((c = *p++) != '\0') {
		if (c == CTLQUOTEMARK)
			continue;
		if (c == CTLESC)
			p++;
		else if (BASESYNTAX[(int)c] == CCTL)
			return 0;
	}
	return 1;
}


/*
 * Return true if the argument is a legal variable name (a letter or
 * underscore followed by zero or more letters, underscores, and digits).
 */

int
goodname(char *name)
	{
	char *p;

	p = name;
	if (! is_name(*p))
		return 0;
	while (*++p) {
		if (! is_in_name(*p))
			return 0;
	}
	return 1;
}


/*
 * Called when an unexpected token is read during the parse.  The argument
 * is the token that is expected, or -1 if more than one type of token can
 * occur at this point.
 */

STATIC void
synexpect(int token)
{
	char msg[64];

	if (token >= 0) {
		fmtstr(msg, 64, "%s unexpected (expecting %s)",
			tokname[lasttoken], tokname[token]);
	} else {
		fmtstr(msg, 64, "%s unexpected", tokname[lasttoken]);
	}
	synerror(msg);
	/* NOTREACHED */
}


STATIC void
synerror(const char *msg)
{
	if (commandname)
		outfmt(&errout, "%s: %d: ", commandname, startlinno);
	else
		outfmt(&errout, "%s: ", getprogname());
	outfmt(&errout, "Syntax error: %s\n", msg);
	error(NULL);
	/* NOTREACHED */
}

STATIC void
setprompt(int which)
{
	whichprompt = which;

#ifndef SMALL
	if (!el)
#endif
		out2str(getprompt(NULL));
}

/*
 * called by editline -- any expansions to the prompt
 *    should be added here.
 */
const char *
getprompt(void *unused)
	{
	switch (whichprompt) {
	case 0:
		return "";
	case 1:
		return ps1val();
	case 2:
		return ps2val();
	default:
		return "<internal prompt error>";
	}
}
