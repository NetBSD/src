/*	$NetBSD: parser.c,v 1.124 2017/05/09 02:47:47 kre Exp $	*/

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
__RCSID("$NetBSD: parser.c,v 1.124 2017/05/09 02:47:47 kre Exp $");
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

/* values returned by readtoken */
#include "token.h"

#define OPENBRACE '{'
#define CLOSEBRACE '}'


struct heredoc {
	struct heredoc *next;	/* next here document in list */
	union node *here;		/* redirection node */
	char *eofmark;		/* string indicating end of input */
	int striptabs;		/* if set, strip leading tabs */
	int startline;		/* line number where << seen */
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
STATIC void slurp_heredoc(char *const, const int, const int);
STATIC void readheredocs(void);
STATIC int peektoken(void);
STATIC int readtoken(void);
STATIC int xxreadtoken(void);
STATIC int readtoken1(int, char const *, int);
STATIC int noexpand(char *);
STATIC void synexpect(int, const char *) __dead;
STATIC void synerror(const char *) __dead;
STATIC void setprompt(int);
STATIC int pgetc_linecont(void);


static const char EOFhere[] = "EOF reading here (<<) document";


/*
 * Read and parse a command.  Returns NEOF on end of file.  (NULL is a
 * valid parse tree indicating a blank line.)
 */

union node *
parsecmd(int interact)
{
	int t;
	union node *n;

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
	n = list(1, 0);
	if (heredoclist)
		error("%d: Here document (<<%s) expected but not present",
			heredoclist->startline, heredoclist->eofmark);
	return n;
}


STATIC union node *
list(int nlflag, int erflag)
{
	union node *n1, *n2, *n3;
	int tok;
	TRACE(("list(%d,%d): entered\n", nlflag, erflag));

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
				n3 = stalloc(sizeof(struct nredir));
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
			n3 = stalloc(sizeof(struct nbinary));
			n3->type = NSEMI;
			n3->nbinary.ch1 = n1;
			n3->nbinary.ch2 = n2;
			n1 = n3;
		}
		switch (tok) {
		case TBACKGND:
		case TSEMI:
			tok = readtoken();
			/* FALLTHROUGH */
		case TNL:
			if (tok == TNL) {
				readheredocs();
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
			pungetc();	/* push back EOF on input */
			return n1;
		default:
			if (nlflag || erflag)
				synexpect(-1, 0);
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
		n3 = stalloc(sizeof(struct nbinary));
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
		pipenode = stalloc(sizeof(struct npipe));
		pipenode->type = NPIPE;
		pipenode->npipe.backgnd = 0;
		lp = stalloc(sizeof(struct nodelist));
		pipenode->npipe.cmdlist = lp;
		lp->n = n1;
		do {
			prev = lp;
			lp = stalloc(sizeof(struct nodelist));
			lp->n = command();
			prev->next = lp;
		} while (readtoken() == TPIPE);
		lp->next = NULL;
		n1 = pipenode;
	}
	tokpushback++;
	if (negate) {
		TRACE(("negate pipeline\n"));
		n2 = stalloc(sizeof(struct nnot));
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
		n1 = stalloc(sizeof(struct nif));
		n1->type = NIF;
		n1->nif.test = list(0, 0);
		if (readtoken() != TTHEN)
			synexpect(TTHEN, 0);
		n1->nif.ifpart = list(0, 0);
		n2 = n1;
		while (readtoken() == TELIF) {
			n2->nif.elsepart = stalloc(sizeof(struct nif));
			n2 = n2->nif.elsepart;
			n2->type = NIF;
			n2->nif.test = list(0, 0);
			if (readtoken() != TTHEN)
				synexpect(TTHEN, 0);
			n2->nif.ifpart = list(0, 0);
		}
		if (lasttoken == TELSE)
			n2->nif.elsepart = list(0, 0);
		else {
			n2->nif.elsepart = NULL;
			tokpushback++;
		}
		if (readtoken() != TFI)
			synexpect(TFI, 0);
		checkkwd = 1;
		break;
	case TWHILE:
	case TUNTIL: {
		int got;
		n1 = stalloc(sizeof(struct nbinary));
		n1->type = (lasttoken == TWHILE)? NWHILE : NUNTIL;
		n1->nbinary.ch1 = list(0, 0);
		if ((got=readtoken()) != TDO) {
TRACE(("expecting DO got %s %s\n", tokname[got], got == TWORD ? wordtext : ""));
			synexpect(TDO, 0);
		}
		n1->nbinary.ch2 = list(0, 0);
		if (readtoken() != TDONE)
			synexpect(TDONE, 0);
		checkkwd = 1;
		break;
	}
	case TFOR:
		if (readtoken() != TWORD || quoteflag || ! goodname(wordtext))
			synerror("Bad for loop variable");
		n1 = stalloc(sizeof(struct nfor));
		n1->type = NFOR;
		n1->nfor.var = wordtext;
		if (readtoken() == TWORD && ! quoteflag && equal(wordtext, "in")) {
			app = &ap;
			while (readtoken() == TWORD) {
				n2 = stalloc(sizeof(struct narg));
				n2->type = NARG;
				n2->narg.text = wordtext;
				n2->narg.backquote = backquotelist;
				*app = n2;
				app = &n2->narg.next;
			}
			*app = NULL;
			n1->nfor.args = ap;
			if (lasttoken != TNL && lasttoken != TSEMI)
				synexpect(-1, 0);
		} else {
			static char argvars[5] = {
			    CTLVAR, VSNORMAL|VSQUOTE, '@', '=', '\0'
			};
			n2 = stalloc(sizeof(struct narg));
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
			synexpect(-1, 0);
		n1->nfor.body = list(0, 0);
		if (readtoken() != t)
			synexpect(t, 0);
		checkkwd = 1;
		break;
	case TCASE:
		n1 = stalloc(sizeof(struct ncase));
		n1->type = NCASE;
		if (readtoken() != TWORD)
			synexpect(TWORD, 0);
		n1->ncase.expr = n2 = stalloc(sizeof(struct narg));
		n2->type = NARG;
		n2->narg.text = wordtext;
		n2->narg.backquote = backquotelist;
		n2->narg.next = NULL;
		while (readtoken() == TNL);
		if (lasttoken != TWORD || ! equal(wordtext, "in"))
			synexpect(-1, "in");
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
			*cpp = cp = stalloc(sizeof(struct nclist));
			if (lasttoken == TLP)
				readtoken();
			cp->type = NCLIST;
			app = &cp->nclist.pattern;
			for (;;) {
				*app = ap = stalloc(sizeof(struct narg));
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
				synexpect(TRP, 0);
			}
			cp->nclist.body = list(0, 0);

			checkkwd = 2;
			if ((t = readtoken()) != TESAC) {
				if (t != TENDCASE && t != TCASEFALL) {
					noalias = 0;
					synexpect(TENDCASE, 0);
				} else {
					if (t == TCASEFALL)
						cp->type = NCLISTCONT;
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
		n1 = stalloc(sizeof(struct nredir));
		n1->type = NSUBSHELL;
		n1->nredir.n = list(0, 0);
		n1->nredir.redirect = NULL;
		if (n1->nredir.n == NULL)
			synexpect(-1, 0);
		if (readtoken() != TRP)
			synexpect(TRP, 0);
		checkkwd = 1;
		break;
	case TBEGIN:
		n1 = list(0, 0);
		if (posix && n1 == NULL)
			synexpect(-1, 0);
		if (readtoken() != TEND)
			synexpect(TEND, 0);
		checkkwd = 1;
		break;

	case TSEMI:
	case TAND:
	case TOR:
	case TPIPE:
	case TNL:
	case TEOF:
	case TRP:
		/*
		 * simple commands must have something in them,
		 * either a word (which at this point includes a=b)
		 * or a redirection.  If we reached the end of the
		 * command (which one of these tokens indicates)
		 * when we are just starting, and have not had a
		 * redirect, then ...
		 *
		 * nb: it is still possible to end up with empty
		 * simple commands, if the "command" is a var
		 * expansion that produces nothing
		 *	X= ; $X && $X
		 * -->          &&
		 * I am not sure if this is intended to be legal or not.
		 */
		if (!redir)
			synexpect(-1, 0);
	case TWORD:
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
		synexpect(-1, 0);
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
			n2 = stalloc(sizeof(struct nredir));
			n2->type = NREDIR;
			n2->nredir.n = n1;
			n1 = n2;
		}
		n1->nredir.redirect = redir;
	}

checkneg:
	if (negate) {
		TRACE(("negate command\n"));
		n2 = stalloc(sizeof(struct nnot));
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
	union node *n = NULL, *n2;
	int negate = 0;

	/* If we don't have any redirections already, then we must reset */
	/* rpp to be the address of the local redir variable.  */
	if (redir == 0)
		rpp = &redir;

	args = NULL;
	app = &args;

	while (readtoken() == TNOT) {
		TRACE(("simplcmd: TNOT recognized\n"));
		negate = !negate;
	}
	tokpushback++;

	for (;;) {
		if (readtoken() == TWORD) {
			n = stalloc(sizeof(struct narg));
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
					    && redir == 0) {
			/* We have a function */
			if (readtoken() != TRP)
				synexpect(TRP, 0);
			funclinno = plinno;
			rmescapes(n->narg.text);
			if (strchr(n->narg.text, '/'))
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

	if (args == NULL && redir == NULL)
		synexpect(-1, 0);
	*app = NULL;
	*rpp = NULL;
	n = stalloc(sizeof(struct ncmd));
	n->type = NCMD;
	n->ncmd.backgnd = 0;
	n->ncmd.args = args;
	n->ncmd.redirect = redir;

checkneg:
	if (negate) {
		TRACE(("negate simplecmd\n"));
		n2 = stalloc(sizeof(struct nnot));
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

	n = stalloc(sizeof(struct narg));
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
		synexpect(-1, 0);
	if (n->type == NHERE) {
		struct heredoc *here = heredoc;
		struct heredoc *p;

		if (quoteflag == 0)
			n->type = NXHERE;
		TRACE(("Here document %d\n", n->type));
		if (here->striptabs) {
			while (*wordtext == '\t')
				wordtext++;
		}

		/*
		 * this test is not really necessary, we are not
		 * required to expand wordtext, but there's no reason
		 * it cannot be $$ or something like that - that would
		 * not mean the pid, but literally two '$' characters.
		 * There is no need for limits on what the word can be.
		 * However, it needs to stay literal as entered, not
		 * have $ converted to CTLVAR or something, which as
		 * the parser is, at the minute, is impossible to prevent.
		 * So, leave it like this until the rest of the parser is fixed.
		 */
		if (! noexpand(wordtext))
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
 * Check to see whether we are at the end of the here document.  When this
 * is called, c is set to the first character of the next input line.  If
 * we are at the end of the here document, this routine sets the c to PEOF.
 * The new value of c is returned.
 */

static int
checkend(int c, char * const eofmark, const int striptabs)
{
	if (striptabs) {
		while (c == '\t')
			c = pgetc();
	}
	if (c == PEOF) {
		if (*eofmark == '\0')
			return (c);
		synerror(EOFhere);
	}
	if (c == *eofmark) {
		int c2;
		char *q;

		for (q = eofmark + 1; c2 = pgetc(), *q != '\0' && c2 == *q; q++)
			;
		if ((c2 == PEOF || c2 == '\n') && *q == '\0') {
			c = PEOF;
			if (c2 == '\n') {
				plinno++;
				needprompt = doprompt;
			}
		} else {
			pungetc();
			pushstring(eofmark + 1, q - (eofmark + 1), NULL);
		}
	} else if (c == '\n' && *eofmark == '\0') {
		c = PEOF;
		plinno++;
		needprompt = doprompt;
	}
	return (c);
}


/*
 * Input any here documents.
 */

STATIC void
slurp_heredoc(char *const eofmark, const int striptabs, const int sq)
{
	int c;
	char *out;

	c = pgetc();

	/*
	 * If we hit EOF on the input, and the eofmark is a null string ('')
	 * we consider this empty line to be the eofmark, and exit without err.
	 */
	if (c == PEOF && *eofmark != '\0')
		synerror(EOFhere);

	STARTSTACKSTR(out);

	while ((c = checkend(c, eofmark, striptabs)) != PEOF) {
		do {
			if (sq) {
				/*
				 * in single quoted mode (eofmark quoted)
				 * all we look for is \n so we can check
				 * for the epfmark - everything saved literally.
				 */
				STPUTC(c, out);
				if (c == '\n') {
					plinno++;
					break;
				}
				continue;
			}
			/*
			 * In double quoted (non-quoted eofmark)
			 * we must handle \ followed by \n here
			 * otherwise we can mismatch the end mark.
			 * All other uses of \ will be handled later
			 * when the here doc is expanded.
			 *
			 * This also makes sure \\ followed by \n does
			 * not suppress the newline (the \ quotes itself)
			 */
			if (c == '\\') {		/* A backslash */
				c = pgetc();		/* followed by */
				if (c == '\n') {	/* a newline?  */
					plinno++;
					continue;	/*    :drop both */
				}
				STPUTC('\\', out);	/* else keep \ */
			}
			STPUTC(c, out);			/* keep the char */
			if (c == '\n') {		/* at end of line */
				plinno++;
				break;			/* look for eofmark */
			}
		} while ((c = pgetc()) != PEOF);

		/*
		 * If we have read a line, and reached EOF, without
		 * finding the eofmark, whether the EOF comes before
		 * or immediately after the \n, that is an error.
		 */
		if (c == PEOF || (c = pgetc()) == PEOF)
			synerror(EOFhere);
	}
	STPUTC('\0', out);

	c = out - stackblock();
	out = stackblock();
	grabstackblock(c);
	wordtext = out;

	TRACE(("Slurped a heredoc (to '%s')%s: len %d, \"%.16s\"...\n",
		eofmark, striptabs ? " tab stripped" : "", c, wordtext));
}

STATIC void
readheredocs(void)
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

		slurp_heredoc(here->eofmark, here->striptabs,
		    here->here->nhere.type == NHERE);

		n = stalloc(sizeof(struct narg));
		n->narg.type = NARG;
		n->narg.next = NULL;
		n->narg.text = wordtext;
		n->narg.backquote = backquotelist;
		here->here->nhere.doc = n;

		if (here->here->nhere.type == NHERE)
			continue;

		/*
		 * Now "parse" here docs that have unquoted eofmarkers.
		 */
		setinputstring(wordtext, 1);
		readtoken1(pgetc(), DQSYNTAX, 1);
		n->narg.text = wordtext;
		n->narg.backquote = backquotelist;
		popfile();
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
				readheredocs();
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

		case '\n':
			plinno++;
			needprompt = doprompt;
			RETURN(TNL);
		case PEOF:
			RETURN(TEOF);

		case '&':
			if (pgetc_linecont() == '&')
				RETURN(TAND);
			pungetc();
			RETURN(TBACKGND);
		case '|':
			if (pgetc_linecont() == '|')
				RETURN(TOR);
			pungetc();
			RETURN(TPIPE);
		case ';':
			switch (pgetc_linecont()) {
			case ';':
				RETURN(TENDCASE);
			case '&':
				RETURN(TCASEFALL);
			default:
				pungetc();
				RETURN(TSEMI);
			}
		case '(':
			RETURN(TLP);
		case ')':
			RETURN(TRP);

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
			/* FALLTHROUGH */
		default:
			return readtoken1(c, BASESYNTAX, 0);
		}
	}
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
 * double quote state for each var nesting level, and arith nesting
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
 * or pattern that follows) ${x} and $x are too simple to need it)
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
 * "..." can span nesting levels, but cannot be opened in one and
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

#define	NQ	0x00	/* Unquoted */
#define	SQ	0x01	/* Single Quotes */
#define	DQ	0x02	/* Double Quotes (or equivalent) */
#define	QF	0x0F		/* Mask to extract previous values */
#define	QS	0x10	/* Quoting started at this level in stack */

#define	LEVELS_PER_BLOCK	8
#define	VSS			struct statestack

struct statestack {
	VSS *prev;		/* previous block in list */
	int cur;		/* which of our tokenstates is current */
	struct tokenstate tokenstate[LEVELS_PER_BLOCK];
};

static inline struct tokenstate *
currentstate(VSS *stack)
{
	return &stack->tokenstate[stack->cur];
}

static inline struct tokenstate *
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
	struct tokenstate *os, *ts;

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
		ckfree(ss);
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

#define	PARSESUB()	{goto parsesub; parsesub_return:;}
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

/*
 * Called to parse command substitutions.  oldstyle is true if the command
 * is enclosed inside `` (otherwise it was enclosed in "$( )")
 *
 * Internally nlpp is a pointer to the head of the linked
 * list of commands (passed by reference), and savelen is the number of
 * characters on the top of the stack which must be preserved.
 */
static char *
parsebackq(VSS *const stack, char * const in,
    struct nodelist **const pbqlist, const int oldstyle)
{
	struct nodelist **nlpp;
	const int savepbq = parsebackquote;
	union node *n;
	char *out;
	char *str = NULL;
	char *volatile sstr = str;
	struct jmploc jmploc;
	struct jmploc *const savehandler = handler;
	const int savelen = in - stackblock();
	int saveprompt;

	if (setjmp(jmploc.loc)) {
		if (sstr)
			ckfree(__UNVOLATILE(sstr));
		cleanup_state_stack(stack);
		parsebackquote = 0;
		handler = savehandler;
		longjmp(handler->loc, 1);
	}
	INTOFF;
	sstr = str = NULL;
	if (savelen > 0) {
		sstr = str = ckmalloc(savelen);
		memcpy(str, stackblock(), savelen);
	}
	handler = &jmploc;
	INTON;
        if (oldstyle) {
                /* We must read until the closing backquote, giving special
                   treatment to some slashes, and then push the string and
                   reread it as input, interpreting it normally.  */
                int pc;
                int psavelen;
                char *pstr;

		/*
		 * Because the entire `...` is read here, we don't
		 * need to bother the state stack.  That will be used
		 * (as appropriate) when the processed string is re-read.
		 */
                STARTSTACKSTR(out);
		for (;;) {
			if (needprompt) {
				setprompt(2);
				needprompt = 0;
			}
			switch (pc = pgetc_linecont()) {
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
                                        STPUTC('\\', out);
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
			STPUTC(pc, out);
                }
done:
                STPUTC('\0', out);
                psavelen = out - stackblock();
                if (psavelen > 0) {
			pstr = grabstackstr(out);
			setinputstring(pstr, 1);
                }
        }
	nlpp = pbqlist;
	while (*nlpp)
		nlpp = &(*nlpp)->next;
	*nlpp = stalloc(sizeof(struct nodelist));
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
			synexpect(TRP, 0);
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
		sstr = str = NULL;
		INTON;
	}
	parsebackquote = savepbq;
	handler = savehandler;
	if (arinest || ISDBLQUOTE())
		USTPUTC(CTLBACKQ | CTLQUOTE, out);
	else
		USTPUTC(CTLBACKQ, out);

	return out;
}

/*
 * Parse a redirection operator.  The parameter "out" points to a string
 * specifying the fd to be redirected.  It is guaranteed to be either ""
 * or a numeric string (for now anyway).  The parameter "c" contains the
 * first character of the redirection operator.
 *
 * Note the string "out" is on the stack, which we are about to clobber,
 * so process it first...
 */

static void
parseredir(const char *out,  int c)
{
	union node *np;
	int fd;

	fd = (*out == '\0') ? -1 : atoi(out);

	np = stalloc(sizeof(struct nfile));
	if (c == '>') {
		if (fd < 0)
			fd = 1;
		c = pgetc_linecont();
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
		if (fd < 0)
			fd = 0;
		switch (c = pgetc_linecont()) {
		case '<':
			if (sizeof (struct nfile) != sizeof (struct nhere)) {
				np = stalloc(sizeof(struct nhere));
				np->nfile.fd = 0;
			}
			np->type = NHERE;
			heredoc = stalloc(sizeof(struct heredoc));
			heredoc->here = np;
			heredoc->startline = plinno;
			if ((c = pgetc_linecont()) == '-') {
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
	np->nfile.fd = fd;

	redirnode = np;		/* this is the "value" of TRENODE */
}


/*
 * The lowest level basic tokenizer.
 *
 * The next input byte (character) is in firstc, syn says which
 * syntax tables we are to use (basic, single or double quoted, or arith)
 * and magicq (used with sqsyntax and dqsyntax only) indicates that the
 * quote character itself is not special (used parsing here docs and similar)
 *
 * The result is the type of the next token (its value, when there is one,
 * is saved in the relevant global var - must fix that someday!) which is
 * also saved for re-reading ("lasttoken").
 *
 * Overall, this routine does far more parsing than it is supposed to.
 * That will also need fixing, someday...
 */
STATIC int
readtoken1(int firstc, char const *syn, int magicq)
{
	int c;
	char * out;
	int len;
	struct nodelist *bqlist;
	int quotef;
	VSS static_stack;
	VSS *stack = &static_stack;

	stack->prev = NULL;
	stack->cur = 0;

	syntax = syn;

	startlinno = plinno;
	varnest = 0;
	quoted = 0;
	if (syntax == DQSYNTAX)
		SETDBLQUOTE();
	quotef = 0;
	bqlist = NULL;
	arinest = 0;
	parenlevel = 0;

	STARTSTACKSTR(out);

	for (c = firstc ;; c = pgetc_macro()) {	/* until of token */
		CHECKSTRSPACE(4, out);	/* permit 4 calls to USTPUTC */
		switch (syntax[c]) {
		case CNL:	/* '\n' */
			if (syntax == BASESYNTAX)
				break;	/* exit loop */
			USTPUTC(c, out);
			plinno++;
			if (doprompt)
				setprompt(2);
			else
				setprompt(0);
			continue;

		case CWORD:
			USTPUTC(c, out);
			continue;
		case CCTL:
			if (!magicq || ISDBLQUOTE())
				USTPUTC(CTLESC, out);
			USTPUTC(c, out);
			continue;
		case CBACK:	/* backslash */
			c = pgetc();
			if (c == PEOF) {
				USTPUTC('\\', out);
				pungetc();
				continue;
			}
			if (c == '\n') {
				plinno++;
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
				continue;
			}
			quotef = 1;	/* current token is quoted */
			if (ISDBLQUOTE() && c != '\\' && c != '`' &&
			    c != '$' && (c != '"' || magicq))
				USTPUTC('\\', out);
			if (SQSYNTAX[c] == CCTL)
				USTPUTC(CTLESC, out);
			else if (!magicq) {
				USTPUTC(CTLQUOTEMARK, out);
				USTPUTC(c, out);
				if (varnest != 0)
					USTPUTC(CTLQUOTEEND, out);
				continue;
			}
			USTPUTC(c, out);
			continue;
		case CSQUOTE:
			if (syntax != SQSYNTAX) {
				if (!magicq)
					USTPUTC(CTLQUOTEMARK, out);
				quotef = 1;
				TS_PUSH();
				syntax = SQSYNTAX;
				quoted = SQ;
				continue;
			}
			if (magicq && arinest == 0 && varnest == 0) {
				/* Ignore inside quoted here document */
				USTPUTC(c, out);
				continue;
			}
			/* End of single quotes... */
			TS_POP();
			if (syntax == BASESYNTAX && varnest != 0)
				USTPUTC(CTLQUOTEEND, out);
			continue;
		case CDQUOTE:
			if (magicq && arinest == 0 && varnest == 0) {
				/* Ignore inside here document */
				USTPUTC(c, out);
				continue;
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
				continue;
			}
			if (magicq)
				continue;
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
			continue;
		case CVAR:	/* '$' */
			PARSESUB();		/* parse substitution */
			continue;
		case CENDVAR:	/* CLOSEBRACE */
			if (varnest > 0 && !ISDBLQUOTE()) {
				TS_POP();
				USTPUTC(CTLENDVAR, out);
			} else {
				USTPUTC(c, out);
			}
			continue;
		case CLP:	/* '(' in arithmetic */
			parenlevel++;
			USTPUTC(c, out);
			continue;;
		case CRP:	/* ')' in arithmetic */
			if (parenlevel > 0) {
				USTPUTC(c, out);
				--parenlevel;
			} else {
				if (pgetc_linecont() == ')') {
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
			continue;
		case CBQUOTE:	/* '`' */
			out = parsebackq(stack, out, &bqlist, 1);
			continue;
		case CEOF:		/* --> c == PEOF */
			break;		/* will exit loop */
		default:
			if (varnest == 0 && !ISDBLQUOTE())
				break;	/* exit loop */
			USTPUTC(c, out);
			continue;
		}
		break;	/* break from switch -> break from for loop too */
	}

	if (syntax == ARISYNTAX) {
		cleanup_state_stack(stack);
		synerror("Missing '))'");
	}
	if (syntax != BASESYNTAX && /* ! parsebackquote && */ !magicq) {
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

	if (!magicq) {
		if ((c == '<' || c == '>')
		 && quotef == 0 && (*out == '\0' || is_number(out))) {
			parseredir(out, c);
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

	c = pgetc_linecont();
	if (c != '(' && c != OPENBRACE && !is_name(c) && !is_special(c)) {
		USTPUTC('$', out);
		pungetc();
	} else if (c == '(') {	/* $(command) or $((arith)) */
		if (pgetc_linecont() == '(') {
			PARSEARITH();
		} else {
			pungetc();
			out = parsebackq(stack, out, &bqlist, 0);
		}
	} else {
		USTPUTC(CTLVAR, out);
		typeloc = out - stackblock();
		USTPUTC(VSNORMAL, out);
		subtype = VSNORMAL;
		flags = 0;
		if (c == OPENBRACE) {
			c = pgetc_linecont();
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
				c = pgetc_linecont();
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
				c = pgetc_linecont();
			} while (subtype != VSNORMAL && is_digit(c));
		}
		else if (is_special(c)) {
			USTPUTC(c, out);
			c = pgetc_linecont();
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
				c = pgetc_linecont();
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
					c = pgetc_linecont();
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
		if (subtype >= VSTRIMLEFT && subtype <= VSTRIMRIGHTMAX)
			flags |= VSPATQ;
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
synexpect(int token, const char *text)
{
	char msg[64];
	char *p;

	if (lasttoken == TWORD) {
		size_t len = strlen(wordtext);

		if (len <= 13)
			fmtstr(msg, 34, "Word \"%.13s\" unexpected", wordtext);
		else
			fmtstr(msg, 34,
			    "Word \"%.10s...\" unexpected", wordtext);
	} else
		fmtstr(msg, 34, "%s unexpected", tokname[lasttoken]);

	p = strchr(msg, '\0');
	if (text)
		fmtstr(p, 30, " (expecting \"%.10s\")", text);
	else if (token >= 0)
		fmtstr(p, 30, " (expecting %s)",  tokname[token]);

	synerror(msg);
	/* NOTREACHED */
}


STATIC void
synerror(const char *msg)
{
	error("%d: Syntax error: %s", startlinno, msg);
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
 * handle getting the next character, while ignoring \ \n
 * (which is a little tricky as we only have one char of pushback
 * and we need that one elsewhere).
 */
STATIC int
pgetc_linecont(void)
{
	int c;

	while ((c = pgetc_macro()) == '\\') {
		c = pgetc();
		if (c == '\n') {
			plinno++;
			if (doprompt)
				setprompt(2);
			else
				setprompt(0);
		} else {
			pungetc();
			/* Allow the backslash to be pushed back. */
			pushstring("\\", 1, NULL);
			return (pgetc());
		}
	}
	return (c);
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
