/*	$NetBSD: parser.c,v 1.179 2022/04/17 21:24:52 andvar Exp $	*/

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
__RCSID("$NetBSD: parser.c,v 1.179 2022/04/17 21:24:52 andvar Exp $");
#endif
#endif /* not lint */

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "parser.h"
#include "nodes.h"
#include "expand.h"	/* defines rmescapes() */
#include "eval.h"	/* defines commandname */
#include "syntax.h"
#include "options.h"
#include "input.h"
#include "output.h"
#include "redir.h"	/* defines max_user_fd */
#include "var.h"
#include "error.h"
#include "memalloc.h"
#include "mystring.h"
#include "alias.h"
#include "show.h"
#ifndef SMALL
#include "myhistedit.h"
#endif
#ifdef DEBUG
#include "nodenames.h"
#endif

/*
 * Shell command parser.
 */

/* values returned by readtoken */
#include "token.h"

#define OPENBRACE '{'
#define CLOSEBRACE '}'

struct HereDoc {
	struct HereDoc *next;	/* next here document in list */
	union node *here;		/* redirection node */
	char *eofmark;		/* string indicating end of input */
	int striptabs;		/* if set, strip leading tabs */
	int startline;		/* line number where << seen */
};

MKINIT struct parse_state parse_state;
union parse_state_p psp = { .c_current_parser = &parse_state };

static const struct parse_state init_parse_state = {	/* all 0's ... */
	.ps_heredoclist = NULL,
	.ps_parsebackquote = 0,
	.ps_doprompt = 0,
	.ps_needprompt = 0,
	.ps_lasttoken = 0,
	.ps_tokpushback = 0,
	.ps_wordtext = NULL,
	.ps_checkkwd = 0,
	.ps_redirnode = NULL,
	.ps_heredoc = NULL,
	.ps_quoteflag = 0,
	.ps_startlinno = 0,
	.ps_funclinno = 0,
	.ps_elided_nl = 0,
};

STATIC union node *list(int);
STATIC union node *andor(void);
STATIC union node *pipeline(void);
STATIC union node *command(void);
STATIC union node *simplecmd(union node **, union node *);
STATIC union node *makeword(int);
STATIC void parsefname(void);
STATIC int slurp_heredoc(char *const, const int, const int);
STATIC void readheredocs(void);
STATIC int peektoken(void);
STATIC int readtoken(void);
STATIC int xxreadtoken(void);
STATIC int readtoken1(int, char const *, int);
STATIC int noexpand(char *);
STATIC void linebreak(void);
STATIC void consumetoken(int);
STATIC void synexpect(int, const char *) __dead;
STATIC void synerror(const char *) __dead;
STATIC void setprompt(int);
STATIC int pgetc_linecont(void);

static const char EOFhere[] = "EOF reading here (<<) document";

#ifdef DEBUG
int parsing = 0;
#endif

/*
 * Read and parse a command.  Returns NEOF on end of file.  (NULL is a
 * valid parse tree indicating a blank line.)
 */

union node *
parsecmd(int interact)
{
	int t;
	union node *n;

#ifdef DEBUG
	parsing++;
#endif
	tokpushback = 0;
	checkkwd = 0;
	doprompt = interact;
	if (doprompt)
		setprompt(1);
	else
		setprompt(0);
	needprompt = 0;
	t = readtoken();
#ifdef DEBUG
	parsing--;
#endif
	if (t == TEOF)
		return NEOF;
	if (t == TNL)
		return NULL;

#ifdef DEBUG
	parsing++;
#endif
	tokpushback++;
	n = list(1);
#ifdef DEBUG
	parsing--;
#endif
	if (heredoclist)
		error("%d: Here document (<<%s) expected but not present",
			heredoclist->startline, heredoclist->eofmark);
	return n;
}


STATIC union node *
list(int nlflag)
{
	union node *ntop, *n1, *n2, *n3;
	int tok;

	CTRACE(DBG_PARSE, ("list(%d): entered @%d\n",nlflag,plinno));

	checkkwd = CHKNL | CHKKWD | CHKALIAS;
	if (nlflag == 0 && tokendlist[peektoken()])
		return NULL;
	ntop = n1 = NULL;
	for (;;) {
		n2 = andor();
		tok = readtoken();
		if (tok == TBACKGND) {
			if (n2->type == NCMD || n2->type == NPIPE)
				n2->ncmd.backgnd = 1;
			else if (n2->type == NREDIR)
				n2->type = NBACKGND;
			else {
				n3 = stalloc(sizeof(struct nredir));
				n3->type = NBACKGND;
				n3->nredir.n = n2;
				n3->nredir.redirect = NULL;
				n2 = n3;
			}
		}

		if (ntop == NULL)
			ntop = n2;
		else if (n1 == NULL) {
			n1 = stalloc(sizeof(struct nbinary));
			n1->type = NSEMI;
			n1->nbinary.ch1 = ntop;
			n1->nbinary.ch2 = n2;
			ntop = n1;
		} else {
			n3 = stalloc(sizeof(struct nbinary));
			n3->type = NSEMI;
			n3->nbinary.ch1 = n1->nbinary.ch2;
			n3->nbinary.ch2 = n2;
			n1->nbinary.ch2 = n3;
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
					return ntop;
			} else if (tok == TEOF && nlflag)
				return ntop;
			else
				tokpushback++;

			checkkwd = CHKNL | CHKKWD | CHKALIAS;
			if (!nlflag && tokendlist[peektoken()])
				return ntop;
			break;
		case TEOF:
			pungetc();	/* push back EOF on input */
			return ntop;
		default:
			if (nlflag)
				synexpect(-1, 0);
			tokpushback++;
			return ntop;
		}
	}
}

STATIC union node *
andor(void)
{
	union node *n1, *n2, *n3;
	int t;

	CTRACE(DBG_PARSE, ("andor: entered @%d\n", plinno));

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

	CTRACE(DBG_PARSE, ("pipeline: entered @%d\n", plinno));

	negate = 0;
	checkkwd = CHKNL | CHKKWD | CHKALIAS;
	while (readtoken() == TNOT) {
		CTRACE(DBG_PARSE, ("pipeline: TNOT recognized\n"));
#ifndef BOGUS_NOT_COMMAND
		if (posix && negate)
			synerror("2nd \"!\" unexpected");
#endif
		negate++;
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
		CTRACE(DBG_PARSE, ("%snegate pipeline\n",
		    (negate&1) ? "" : "double "));
		n2 = stalloc(sizeof(struct nnot));
		n2->type = (negate & 1) ? NNOT : NDNOT;
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
	int t;
#ifdef BOGUS_NOT_COMMAND
	int negate = 0;
#endif

	CTRACE(DBG_PARSE, ("command: entered @%d\n", plinno));

	checkkwd = CHKNL | CHKKWD | CHKALIAS;
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

#ifdef BOGUS_NOT_COMMAND		/* only in pipeline() */
	while (readtoken() == TNOT) {
		CTRACE(DBG_PARSE, ("command: TNOT (bogus) recognized\n"));
		negate++;
	}
	tokpushback++;
#endif

	switch (readtoken()) {
	case TIF:
		n1 = stalloc(sizeof(struct nif));
		n1->type = NIF;
		n1->nif.test = list(0);
		consumetoken(TTHEN);
		n1->nif.ifpart = list(0);
		n2 = n1;
		while (readtoken() == TELIF) {
			n2->nif.elsepart = stalloc(sizeof(struct nif));
			n2 = n2->nif.elsepart;
			n2->type = NIF;
			n2->nif.test = list(0);
			consumetoken(TTHEN);
			n2->nif.ifpart = list(0);
		}
		if (lasttoken == TELSE)
			n2->nif.elsepart = list(0);
		else {
			n2->nif.elsepart = NULL;
			tokpushback++;
		}
		consumetoken(TFI);
		checkkwd = CHKKWD | CHKALIAS;
		break;
	case TWHILE:
	case TUNTIL:
		n1 = stalloc(sizeof(struct nbinary));
		n1->type = (lasttoken == TWHILE)? NWHILE : NUNTIL;
		n1->nbinary.ch1 = list(0);
		consumetoken(TDO);
		n1->nbinary.ch2 = list(0);
		consumetoken(TDONE);
		checkkwd = CHKKWD | CHKALIAS;
		break;
	case TFOR:
		if (readtoken() != TWORD || quoteflag || ! goodname(wordtext))
			synerror("Bad for loop variable");
		n1 = stalloc(sizeof(struct nfor));
		n1->type = NFOR;
		n1->nfor.var = wordtext;
		n1->nfor.lineno = startlinno;
		linebreak();
		if (lasttoken==TWORD && !quoteflag && equal(wordtext,"in")) {
			app = &ap;
			while (readtoken() == TWORD) {
				n2 = makeword(startlinno);
				*app = n2;
				app = &n2->narg.next;
			}
			*app = NULL;
			n1->nfor.args = ap;
			if (lasttoken != TNL && lasttoken != TSEMI)
				synexpect(TSEMI, 0);
			if (lasttoken == TNL)
				readheredocs();
		} else {
			static char argvars[5] = {
			    CTLVAR, VSNORMAL|VSQUOTE, '@', '=', '\0'
			};

			n2 = stalloc(sizeof(struct narg));
			n2->type = NARG;
			n2->narg.text = argvars;
			n2->narg.backquote = NULL;
			n2->narg.next = NULL;
			n2->narg.lineno = startlinno;
			n1->nfor.args = n2;
			/*
			 * Newline or semicolon here is optional (but note
			 * that the original Bourne shell only allowed NL).
			 */
			if (lasttoken != TNL && lasttoken != TSEMI)
				tokpushback++;
		}
		checkkwd = CHKNL | CHKKWD | CHKALIAS;
		if ((t = readtoken()) == TDO)
			t = TDONE;
		else if (t == TBEGIN)
			t = TEND;
		else
			synexpect(TDO, 0);
		n1->nfor.body = list(0);
		consumetoken(t);
		checkkwd = CHKKWD | CHKALIAS;
		break;
	case TCASE:
		n1 = stalloc(sizeof(struct ncase));
		n1->type = NCASE;
		n1->ncase.lineno = startlinno - elided_nl;
		consumetoken(TWORD);
		n1->ncase.expr = makeword(startlinno);
		linebreak();
		if (lasttoken != TWORD || !equal(wordtext, "in"))
			synexpect(-1, "in");
		cpp = &n1->ncase.cases;
		checkkwd = CHKNL | CHKKWD;
		readtoken();
		/*
		 * Both ksh and bash accept 'case x in esac'
		 * so configure scripts started taking advantage of this.
		 * The page: http://pubs.opengroup.org/onlinepubs/\
		 * 009695399/utilities/xcu_chap02.html contradicts itself,
		 * as to if this is legal; the "Case Conditional Format"
		 * paragraph shows one case is required, but the "Grammar"
		 * section shows a grammar that explicitly allows the no
		 * case option.
		 *
		 * The standard also says (section 2.10):
		 *   This formal syntax shall take precedence over the
		 *   preceding text syntax description.
		 * ie: the "Grammar" section wins.  The text is just
		 * a rough guide (introduction to the common case.)
		 */
		while (lasttoken != TESAC) {
			*cpp = cp = stalloc(sizeof(struct nclist));
			cp->type = NCLIST;
			app = &cp->nclist.pattern;
			if (lasttoken == TLP)
				readtoken();
			for (;;) {
				if (lasttoken < TWORD)
					synexpect(TWORD, 0);
				*app = ap = makeword(startlinno);
				checkkwd = CHKNL | CHKKWD;
				if (readtoken() != TPIPE)
					break;
				app = &ap->narg.next;
				readtoken();
			}
			if (lasttoken != TRP)
				synexpect(TRP, 0);
			cp->nclist.lineno = startlinno;
			cp->nclist.body = list(0);

			checkkwd = CHKNL | CHKKWD | CHKALIAS;
			if ((t = readtoken()) != TESAC) {
				if (t != TENDCASE && t != TCASEFALL) {
					synexpect(TENDCASE, 0);
				} else {
					if (t == TCASEFALL)
						cp->type = NCLISTCONT;
					checkkwd = CHKNL | CHKKWD;
					readtoken();
				}
			}
			cpp = &cp->nclist.next;
		}
		*cpp = NULL;
		checkkwd = CHKKWD | CHKALIAS;
		break;
	case TLP:
		n1 = stalloc(sizeof(struct nredir));
		n1->type = NSUBSHELL;
		n1->nredir.n = list(0);
		n1->nredir.redirect = NULL;
		if (n1->nredir.n == NULL)
			synexpect(-1, 0);
		consumetoken(TRP);
		checkkwd = CHKKWD | CHKALIAS;
		break;
	case TBEGIN:
		n1 = list(0);
		if (posix && n1 == NULL)
			synexpect(-1, 0);
		consumetoken(TEND);
		checkkwd = CHKKWD | CHKALIAS;
		break;

	case TBACKGND:
	case TSEMI:
	case TAND:
	case TOR:
	case TPIPE:
	case TNL:
	case TEOF:
	case TRP:
	case TENDCASE:
	case TCASEFALL:
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
		 * expansion that produces nothing:
		 *	X= ; $X && $X
		 * -->          &&
		 * That is OK and is handled after word expansions.
		 */
		if (!redir)
			synexpect(-1, 0);
		/*
		 * continue to build a node containing the redirect.
		 * the tokpushback means that our ending token will be
		 * read again in simplecmd, causing it to terminate,
		 * so only the redirect(s) will be contained in the
		 * returned n1
		 */
		/* FALLTHROUGH */
	case TWORD:
		tokpushback++;
		n1 = simplecmd(rpp, redir);
		goto checkneg;
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
		if (n1 == NULL || n1->type != NSUBSHELL) {
			n2 = stalloc(sizeof(struct nredir));
			n2->type = NREDIR;
			n2->nredir.n = n1;
			n1 = n2;
		}
		n1->nredir.redirect = redir;
	}

 checkneg:
#ifdef BOGUS_NOT_COMMAND
	if (negate) {
		VTRACE(DBG_PARSE, ("bogus %snegate command\n",
		    (negate&1) ? "" : "double "));
		n2 = stalloc(sizeof(struct nnot));
		n2->type = (negate & 1) ? NNOT : NDNOT;
		n2->nnot.com = n1;
		return n2;
	}
	else
#endif
		return n1;
}


STATIC union node *
simplecmd(union node **rpp, union node *redir)
{
	union node *args, **app;
	union node *n = NULL;
	int line = 0;
	int savecheckkwd;
#ifdef BOGUS_NOT_COMMAND
	union node *n2;
	int negate = 0;
#endif

	CTRACE(DBG_PARSE, ("simple command with%s redir already @%d\n",
	    redir ? "" : "out", plinno));

	/* If we don't have any redirections already, then we must reset */
	/* rpp to be the address of the local redir variable.  */
	if (redir == 0)
		rpp = &redir;

	args = NULL;
	app = &args;

#ifdef BOGUS_NOT_COMMAND	/* pipelines get negated, commands do not */
	while (readtoken() == TNOT) {
		VTRACE(DBG_PARSE, ("simplcmd: bogus TNOT recognized\n"));
		negate++;
	}
	tokpushback++;
#endif

	savecheckkwd = CHKALIAS;
	for (;;) {
		checkkwd = savecheckkwd;
		if (readtoken() == TWORD) {
			if (line == 0)
				line = startlinno;
			n = makeword(startlinno);
			*app = n;
			app = &n->narg.next;
			if (savecheckkwd != 0 && !isassignment(wordtext))
				savecheckkwd = 0;
		} else if (lasttoken == TREDIR) {
			if (line == 0)
				line = startlinno;
			*rpp = n = redirnode;
			rpp = &n->nfile.next;
			parsefname();	/* read name of redirection file */
		} else if (lasttoken == TLP && app == &args->narg.next
					    && redir == 0) {
			/* We have a function */
			consumetoken(TRP);
			funclinno = plinno;
			/*
			 * Make sure there are no unquoted $'s in the
			 * name (allowing those, not expanding them,
			 * simply treating '$' as a character, is desirable
			 * but the parser has converted them to CTLxxx
			 * chars, and that's not what we want
			 *
			 * Fortunately here the user can simply quote
			 * the name to avoid this restriction.
			 */
			if (!noexpand(n->narg.text))
				synerror("Bad function name (use quotes)");
			rmescapes(n->narg.text);
			if (strchr(n->narg.text, '/'))
				synerror("Bad function name");
			VTRACE(DBG_PARSE, ("Function '%s' seen @%d\n",
			    n->narg.text, plinno));
			n->type = NDEFUN;
			n->narg.lineno = plinno - elided_nl;
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
	n->ncmd.lineno = line - elided_nl;
	n->ncmd.backgnd = 0;
	n->ncmd.args = args;
	n->ncmd.redirect = redir;
	n->ncmd.lineno = startlinno;

 checkneg:
#ifdef BOGUS_NOT_COMMAND
	if (negate) {
		VTRACE(DBG_PARSE, ("bogus %snegate simplecmd\n",
		    (negate&1) ? "" : "double "));
		n2 = stalloc(sizeof(struct nnot));
		n2->type = (negate & 1) ? NNOT : NDNOT;
		n2->nnot.com = n;
		return n2;
	}
	else
#endif
		return n;
}

STATIC union node *
makeword(int lno)
{
	union node *n;

	n = stalloc(sizeof(struct narg));
	n->type = NARG;
	n->narg.next = NULL;
	n->narg.text = wordtext;
	n->narg.backquote = backquotelist;
	n->narg.lineno = lno;
	return n;
}

void
fixredir(union node *n, const char *text, int err)
{

	VTRACE(DBG_PARSE, ("Fix redir %s %d\n", text, err));
	if (!err)
		n->ndup.vname = NULL;

	if (is_number(text)) {
		n->ndup.dupfd = number(text);
		if (n->ndup.dupfd < user_fd_limit &&
		    n->ndup.dupfd > max_user_fd)
			max_user_fd = n->ndup.dupfd;
	} else if (text[0] == '-' && text[1] == '\0')
		n->ndup.dupfd = -1;
	else {

		if (err)
			synerror("Bad fd number");
		else
			n->ndup.vname = makeword(startlinno - elided_nl);
	}
}


STATIC void
parsefname(void)
{
	union node *n = redirnode;

	if (readtoken() != TWORD)
		synexpect(-1, 0);
	if (n->type == NHERE) {
		struct HereDoc *here = heredoc;
		struct HereDoc *p;

		if (quoteflag == 0)
			n->type = NXHERE;
		VTRACE(DBG_PARSE, ("Here document %d @%d\n", n->type, plinno));
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
		if (!noexpand(wordtext))
			synerror("Unimplemented form of eof marker"
			    " for << redirection");

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
		n->nfile.fname = makeword(startlinno - elided_nl);
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
			if (c2 == '\n') {
				plinno++;
				needprompt = doprompt;
			}
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

STATIC int
slurp_heredoc(char *const eofmark, const int striptabs, const int sq)
{
	int c;
	char *out;
	int lines = plinno;

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
				 * for the eofmark - everything saved literally.
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
				STPUTC(c, out);
				c = pgetc();		/* followed by */
				if (c == '\n') {	/* a newline?  */
					STPUTC(c, out);
					plinno++;
					continue;	/* don't break */
				}
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

	VTRACE(DBG_PARSE,
	   ("Slurped a %d line %sheredoc (to '%s')%s: len %d, \"%.*s%s\" @%d\n",
		plinno - lines, sq ? "quoted " : "",  eofmark,
		striptabs ? " tab stripped" : "", c, (c > 16 ? 16 : c),
		wordtext, (c > 16 ? "..." : ""), plinno));

	return (plinno - lines);
}

static char *
insert_elided_nl(char *str)
{
	while (elided_nl > 0) {
		STPUTC(CTLNONL, str);
		elided_nl--;
	}
	return str;
}

STATIC void
readheredocs(void)
{
	struct HereDoc *here;
	union node *n;
	int line, l;

	line = 0;		/*XXX - gcc!  obviously unneeded */
	if (heredoclist)
		line = heredoclist->startline + 1;
	l = 0;
	while (heredoclist) {
		line += l;
		here = heredoclist;
		heredoclist = here->next;
		if (needprompt) {
			setprompt(2);
			needprompt = 0;
		}

		l = slurp_heredoc(here->eofmark, here->striptabs,
		    here->here->nhere.type == NHERE);

		here->here->nhere.doc = n = makeword(line);

		if (here->here->nhere.type == NHERE)
			continue;

		/*
		 * Now "parse" here docs that have unquoted eofmarkers.
		 */
		setinputstring(wordtext, 1, line);
		VTRACE(DBG_PARSE, ("Reprocessing %d line here doc from %d\n",
			l, line));
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
#ifdef DEBUG
	int alreadyseen = tokpushback;
	int savecheckkwd = checkkwd;
#endif
	struct alias *ap;

 top:
	t = xxreadtoken();

	if (checkkwd & CHKNL) {
		while (t == TNL) {
			readheredocs();
			t = xxreadtoken();
		}
	}

	/*
	 * check for keywords and aliases
	 */
	if (t == TWORD && !quoteflag) {
		const char *const *pp;

		if (checkkwd & CHKKWD)
			for (pp = parsekwd; *pp; pp++) {
				if (**pp == *wordtext && equal(*pp, wordtext)) {
					lasttoken = t = pp -
					    parsekwd + KWDOFFSET;
					VTRACE(DBG_PARSE,
					    ("keyword %s recognized @%d\n",
					    tokname[t], plinno));
					goto out;
				}
			}

		if (checkkwd & CHKALIAS &&
		    (ap = lookupalias(wordtext, 1)) != NULL) {
			VTRACE(DBG_PARSE,
			    ("alias '%s' recognized -> <:%s:>\n",
			    wordtext, ap->val));
			pushstring(ap->val, strlen(ap->val), ap);
			goto top;
		}
	}
 out:
	if (t != TNOT)
		checkkwd = 0;

	VTRACE(DBG_PARSE, ("%stoken %s %s @%d (chkkwd %x->%x)\n",
	    alreadyseen ? "reread " : "", tokname[t],
	    t == TWORD ? wordtext : "", plinno, savecheckkwd, checkkwd));
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

#define RETURN(token)	return lasttoken = (token)

STATIC int
xxreadtoken(void)
{
	int c;

	if (tokpushback) {
		tokpushback = 0;
		CTRACE(DBG_LEXER,
		    ("xxreadtoken() returns %s (%d) again\n",
			tokname[lasttoken], lasttoken));
		return lasttoken;
	}
	if (needprompt) {
		setprompt(2);
		needprompt = 0;
	}
	elided_nl = 0;
	startlinno = plinno;
	for (;;) {	/* until token or start of word found */
		c = pgetc_macro();
		CTRACE(DBG_LEXER, ("xxreadtoken() sees '%c' (%#.2x) ",
		    c&0xFF, c&0x1FF));
		switch (c) {
		case ' ': case '\t': case PFAKE:
			CTRACE(DBG_LEXER, (" ignored\n"));
			continue;
		case '#':
			while ((c = pgetc()) != '\n' && c != PEOF)
				continue;
			CTRACE(DBG_LEXER,
			    ("skipped comment to (not incl) \\n\n"));
			pungetc();
			continue;

		case '\n':
			plinno++;
			CTRACE(DBG_LEXER, ("newline now @%d\n", plinno));
			needprompt = doprompt;
			RETURN(TNL);
		case PEOF:
			CTRACE(DBG_LEXER, ("EOF -> TEOF (return)\n"));
			RETURN(TEOF);

		case '&':
			if (pgetc_linecont() == '&') {
				CTRACE(DBG_LEXER,
				    ("and another  -> TAND (return)\n"));
				RETURN(TAND);
			}
			pungetc();
			CTRACE(DBG_LEXER, (" -> TBACKGND (return)\n"));
			RETURN(TBACKGND);
		case '|':
			if (pgetc_linecont() == '|') {
				CTRACE(DBG_LEXER,
				    ("and another  -> TOR (return)\n"));
				RETURN(TOR);
			}
			pungetc();
			CTRACE(DBG_LEXER, (" -> TPIPE (return)\n"));
			RETURN(TPIPE);
		case ';':
			switch (pgetc_linecont()) {
			case ';':
				CTRACE(DBG_LEXER,
				    ("and another -> TENDCASE (return)\n"));
				RETURN(TENDCASE);
			case '&':
				CTRACE(DBG_LEXER,
				    ("and '&' -> TCASEFALL (return)\n"));
				RETURN(TCASEFALL);
			default:
				pungetc();
				CTRACE(DBG_LEXER, (" -> TSEMI (return)\n"));
				RETURN(TSEMI);
			}
		case '(':
			CTRACE(DBG_LEXER, (" -> TLP (return)\n"));
			RETURN(TLP);
		case ')':
			CTRACE(DBG_LEXER, (" -> TRP (return)\n"));
			RETURN(TRP);

		case '\\':
			switch (pgetc()) {
			case '\n':
				startlinno = ++plinno;
				CTRACE(DBG_LEXER, ("\\\n ignored, now @%d\n",
				    plinno));
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
				continue;
			case PEOF:
				CTRACE(DBG_LEXER,
				  ("then EOF -> TEOF (return) '\\' dropped\n"));
				RETURN(TEOF);
			default:
				CTRACE(DBG_LEXER, ("not \\\n or EOF: "));
				pungetc();
				break;
			}
			/* FALLTHROUGH */
		default:
			CTRACE(DBG_LEXER, ("getting a word\n"));
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
	unsigned short ts_magicq;	/* heredoc or word expand */
};

#define	NQ	0x00	/* Unquoted */
#define	SQ	0x01	/* Single Quotes */
#define	DQ	0x02	/* Double Quotes (or equivalent) */
#define	CQ	0x03	/* C style Single Quotes */
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

#ifdef notdef
static inline struct tokenstate *
prevstate(VSS *stack)
{
	if (stack->cur != 0)
		return &stack->tokenstate[stack->cur - 1];
	if (stack->prev == NULL)	/* cannot drop below base */
		return &stack->tokenstate[0];
	return &stack->prev->tokenstate[LEVELS_PER_BLOCK - 1];
}
#endif

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
	ts->ts_magicq  = os->ts_magicq;

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
 * The following macros all assume the existence of a local var "stack"
 * which contains a pointer to the current struct stackstate
 */

/*
 * These are macros rather than inline funcs to avoid code churn as much
 * as possible - they replace macros of the same name used previously.
 */
#define	ISDBLQUOTE()	(currentstate(stack)->ts_quoted & QS)
#define	SETDBLQUOTE()	(currentstate(stack)->ts_quoted = QS | DQ)
#ifdef notdef
#define	CLRDBLQUOTE()	(currentstate(stack)->ts_quoted =		\
			    stack->cur != 0 || stack->prev ?		\
				prevstate(stack)->ts_quoted & QF : 0)
#endif

/*
 * This set are just to avoid excess typing and line lengths...
 * The ones that "look like" var names must be implemented to be lvalues
 */
#define	syntax		(currentstate(stack)->ts_syntax)
#define	parenlevel	(currentstate(stack)->ts_parenlevel)
#define	varnest		(currentstate(stack)->ts_varnest)
#define	arinest		(currentstate(stack)->ts_arinest)
#define	quoted		(currentstate(stack)->ts_quoted)
#define	magicq		(currentstate(stack)->ts_magicq)
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
	struct parsefile *const savetopfile = getcurrentfile();
	const int savelen = in - stackblock();
	int saveprompt;
	int lno;

	if (setjmp(jmploc.loc)) {
		popfilesupto(savetopfile);
		if (sstr)
			ckfree(__UNVOLATILE(sstr));
		cleanup_state_stack(stack);
		parsebackquote = 0;
		handler = savehandler;
		CTRACE(DBG_LEXER, ("parsebackq() err (%d), unwinding\n",
		    exception));
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
		/*
		 * We must read until the closing backquote, giving special
		 * treatment to some slashes, and then push the string and
		 * reread it as input, interpreting it normally.
		 */
		int pc;
		int psavelen;
		char *pstr;
		int line1 = plinno;

		VTRACE(DBG_PARSE|DBG_LEXER,
		    ("parsebackq: repackaging `` as $( )"));
		/*
		 * Because the entire `...` is read here, we don't
		 * need to bother the state stack.  That will be used
		 * (as appropriate) when the processed string is re-read.
		 */
		STARTSTACKSTR(out);
#ifdef DEBUG
		for (psavelen = 0;;psavelen++) {	/* } */
#else
		for (;;) {
#endif
			if (needprompt) {
				setprompt(2);
				needprompt = 0;
			}
			pc = pgetc();
			VTRACE(DBG_LEXER,
			    ("parsebackq() got '%c'(%#.2x) in `` %s", pc&0xFF,
				pc&0x1FF, pc == '`' ? "terminator\n" : ""));
			if (pc == '`')
				break;
			switch (pc) {
			case '\\':
				pc = pgetc();
				VTRACE(DBG_LEXER, ("then '%c'(%#.2x) ",
				    pc&0xFF, pc&0x1FF));
#ifdef DEBUG
				psavelen++;
#endif
				if (pc == '\n') {   /* keep \ \n for later */
					plinno++;
					VTRACE(DBG_LEXER, ("@%d ", plinno));
					needprompt = doprompt;
				}
				if (pc != '\\' && pc != '`' && pc != '$'
				    && (!ISDBLQUOTE() || pc != '"')) {
					VTRACE(DBG_LEXER, ("keep '\\' "));
					STPUTC('\\', out);
				}
				break;

			case '\n':
				plinno++;
				VTRACE(DBG_LEXER, ("@%d ", plinno));
				needprompt = doprompt;
				break;

			case PEOF:
			        startlinno = line1;
				VTRACE(DBG_LEXER, ("EOF\n", plinno));
				synerror("EOF in backquote substitution");
 				break;

			default:
				break;
			}
			VTRACE(DBG_LEXER, (".\n", plinno));
			STPUTC(pc, out);
		}
		STPUTC('\0', out);
		VTRACE(DBG_LEXER, ("parsebackq() ``:"));
		VTRACE(DBG_PARSE|DBG_LEXER, (" read %d", psavelen));
		psavelen = out - stackblock();
		VTRACE(DBG_PARSE|DBG_LEXER, (" produced %d\n", psavelen));
		if (psavelen > 0) {
			pstr = grabstackstr(out);
			CTRACE(DBG_LEXER, 
			    ("parsebackq() reprocessing as $(%s)\n", pstr));
			setinputstring(pstr, 1, line1);
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

	lno = -plinno;
	CTRACE(DBG_LEXER, ("parsebackq() parsing embedded command list\n"));
	n = list(0);
	CTRACE(DBG_LEXER, ("parsebackq() parsed $() (%d -> %d)\n", -lno,
	    lno + plinno));
	lno += plinno;

	if (oldstyle) {
		if (peektoken() != TEOF)
			synexpect(-1, 0);
		doprompt = saveprompt;
	} else
		consumetoken(TRP);

	(*nlpp)->n = n;
	if (oldstyle) {
		/*
		 * Start reading from old file again, ignoring any pushed back
		 * tokens left from the backquote parsing
		 */
		CTRACE(DBG_LEXER, ("parsebackq() back to previous input\n"));
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
	if (arinest || ISDBLQUOTE()) {
		STPUTC(CTLBACKQ | CTLQUOTE, out);
		while (--lno >= 0)
			STPUTC(CTLNONL, out);
	} else
		STPUTC(CTLBACKQ, out);

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

	np = stalloc(sizeof(struct nfile));

	fd = (*out == '\0') ? -1 : number(out);		/* number(out) >= 0 */
	np->nfile.fd = fd;	/* do this again later with updated fd */
	if (fd != np->nfile.fd)
		error("file descriptor (%d) out of range (max %ld)",
		    fd, user_fd_limit - 1);
	if (fd < user_fd_limit && fd > max_user_fd)
		max_user_fd = fd;

	VTRACE(DBG_LEXER, ("parseredir after '%s%c' ", out, c));
	if (c == '>') {
		if (fd < 0)
			fd = 1;
		c = pgetc_linecont();
		VTRACE(DBG_LEXER, ("is '%c'(%#.2x) ", c&0xFF, c&0x1FF));
		if (c == '>')
			np->type = NAPPEND;
		else if (c == '|')
			np->type = NCLOBBER;
		else if (c == '&')
			np->type = NTOFD;
		else {
			np->type = NTO;
			VTRACE(DBG_LEXER, ("unwanted ", c));
			pungetc();
		}
	} else {	/* c == '<' */
		if (fd < 0)
			fd = 0;
		c = pgetc_linecont();
		VTRACE(DBG_LEXER, ("is '%c'(%#.2x) ", c&0xFF, c&0x1FF));
		switch (c) {
		case '<':
			/* if sizes differ, just discard the old one */
			if (sizeof (struct nfile) != sizeof (struct nhere))
				np = stalloc(sizeof(struct nhere));
			np->type = NHERE;
			np->nhere.fd = 0;
			heredoc = stalloc(sizeof(struct HereDoc));
			heredoc->here = np;
			heredoc->startline = plinno;
			if ((c = pgetc_linecont()) == '-') {
				CTRACE(DBG_LEXER, ("and '%c'(%#.2x) ",
				    c & 0xFF, c & 0x1FF));
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
			VTRACE(DBG_LEXER, ("unwanted('%c'0#.2x)", c&0xFF,
			    c&0x1FF));
			pungetc();
			break;
		}
	}
	np->nfile.fd = fd;

	VTRACE(DBG_LEXER, (" ->%"PRIdsNT" fd=%d\n", NODETYPENAME(np->type),fd));

	redirnode = np;		/* this is the "value" of TRENODE */
}

/*
 * Called to parse a backslash escape sequence inside $'...'.
 * The backslash has already been read.
 */
static char *
readcstyleesc(char *out)
{
	int c, vc, i, n;
	unsigned int v;

	c = pgetc();
	VTRACE(DBG_LEXER, ("CSTR(\\%c)(\\%#x)", c&0xFF, c&0x1FF));
	switch (c) {
	case '\0':
	case PEOF:
		synerror("Unterminated quoted string ($'...)");
	case '\n':
		plinno++;
		VTRACE(DBG_LEXER, ("@%d ", plinno));
		if (doprompt)
			setprompt(2);
		else
			setprompt(0);
		return out;

	case '\\':
	case '\'':
	case '"':
		v = c;
		break;

	case 'a': v = '\a'; break;
	case 'b': v = '\b'; break;
	case 'e': v = '\033'; break;
	case 'f': v = '\f'; break;
	case 'n': v = '\n'; break;
	case 'r': v = '\r'; break;
	case 't': v = '\t'; break;
	case 'v': v = '\v'; break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		v = c - '0';
		c = pgetc();
		if (c >= '0' && c <= '7') {
			v <<= 3;
			v += c - '0';
			c = pgetc();
			if (c >= '0' && c <= '7') {
				v <<= 3;
				v += c - '0';
			} else
				pungetc();
		} else
			pungetc();
		break;

	case 'c':
		c = pgetc();
		if (c < 0x3f || c > 0x7a || c == 0x60)
			synerror("Bad \\c escape sequence");
		if (c == '\\' && pgetc() != '\\')
			synerror("Bad \\c\\ escape sequence");
		if (c == '?')
			v = 127;
		else
			v = c & 0x1f;
		break;

	case 'x':
		n = 2;
		goto hexval;
	case 'u':
		n = 4;
		goto hexval;
	case 'U':
		n = 8;
	hexval:
		v = 0;
		for (i = 0; i < n; i++) {
			c = pgetc();
			if (c >= '0' && c <= '9')
				v = (v << 4) + c - '0';
			else if (c >= 'A' && c <= 'F')
				v = (v << 4) + c - 'A' + 10;
			else if (c >= 'a' && c <= 'f')
				v = (v << 4) + c - 'a' + 10;
			else {
				pungetc();
				break;
			}
		}
		if (n > 2 && v > 127) {
			if (v >= 0xd800 && v <= 0xdfff)
				synerror("Invalid \\u escape sequence");

			/* XXX should we use iconv here. What locale? */
			CHECKSTRSPACE(12, out);

/*
 * Add a byte to output string, while checking if it needs to
 * be escaped -- if its value happens to match the value of one
 * of our internal CTL* chars - which would (at a minimum) be
 * summarily removed later, if not escaped.
 *
 * The current definition of ISCTL() allows the compiler to
 * optimise away either half, or all, of the test in most of
 * the cases here (0xc0 | anything) cannot be between 0x80 and 0x9f
 * for example, so there a test is not needed).
 *
 * Which tests can be removed depends upon the actual values
 * selected for the CTL* chars.
 */
#define	ESC_USTPUTC(c, o) do {				\
		char _ch = (c);				\
							\
		if (ISCTL(_ch))				\
			USTPUTC(CTLESC, o);		\
		USTPUTC(_ch, o);			\
	} while (0)

			VTRACE(DBG_LEXER, ("CSTR(\\%c%8.8x)", n==4?'u':'U', v));
			if (v <= 0x7ff) {
				ESC_USTPUTC(0xc0 | v >> 6, out);
				ESC_USTPUTC(0x80 | (v & 0x3f), out);
				return out;
			} else if (v <= 0xffff) {
				ESC_USTPUTC(0xe0 | v >> 12, out);
				ESC_USTPUTC(0x80 | ((v >> 6) & 0x3f), out);
				ESC_USTPUTC(0x80 | (v & 0x3f), out);
				return out;
			} else if (v <= 0x10ffff) {
				ESC_USTPUTC(0xf0 | v >> 18, out);
				ESC_USTPUTC(0x80 | ((v >> 12) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 6) & 0x3f), out);
				ESC_USTPUTC(0x80 | (v & 0x3f), out);
				return out;

	/* these next two are not very likely, but we may as well be complete */
			} else if (v <= 0x3FFFFFF) {
				ESC_USTPUTC(0xf8 | v >> 24, out);
				ESC_USTPUTC(0x80 | ((v >> 18) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 12) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 6) & 0x3f), out);
				ESC_USTPUTC(0x80 | (v & 0x3f), out);
				return out;
			} else if (v <= 0x7FFFFFFF) {
				ESC_USTPUTC(0xfC | v >> 30, out);
				ESC_USTPUTC(0x80 | ((v >> 24) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 18) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 12) & 0x3f), out);
				ESC_USTPUTC(0x80 | ((v >> 6) & 0x3f), out);
				ESC_USTPUTC(0x80 | (v & 0x3f), out);
				return out;
			}
			if (v > 127)
				v = '?';
		}
		break;
	default:
		synerror("Unknown $'' escape sequence");
	}
	vc = (char)v;
	VTRACE(DBG_LEXER, ("->%u(%#x)['%c']", v, v, vc&0xFF));

	/*
	 * If we managed to create a \n from a \ sequence (no matter how)
	 * then we replace it with the magic CRTCNL control char, which
	 * will turn into a \n again later, but in the meantime, never
	 * causes LINENO increments.
	 */
	if (vc == '\n') {
		VTRACE(DBG_LEXER, ("CTLCNL."));
		USTPUTC(CTLCNL, out);
		return out;
	}

	/*
	 * We can't handle NUL bytes.
	 * POSIX says we should skip till the closing quote.
	 */
	if (vc == '\0') {
		CTRACE(DBG_LEXER, ("\\0: skip to '", v, v, vc&0xFF));
		while ((c = pgetc()) != '\'') {
			if (c == '\\')
				c = pgetc();
			if (c == PEOF)
				synerror("Unterminated quoted string ($'...)");
			if (c == '\n') {
				plinno++;
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
			}
		}
		pungetc();
		return out;
	}
	CVTRACE(DBG_LEXER, NEEDESC(vc), ("CTLESC-"));
	VTRACE(DBG_LEXER, ("'%c'(%#.2x)", vc&0xFF, vc&0x1FF));
	if (NEEDESC(vc))
		USTPUTC(CTLESC, out);
	USTPUTC(vc, out);
	return out;
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
readtoken1(int firstc, char const *syn, int oneword)
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

#ifdef DEBUG
#define SYNTAX      (	syntax == BASESYNTAX ? "BASE"	:		\
			syntax == DQSYNTAX   ? "DQ"	:		\
			syntax == SQSYNTAX   ? "SQ"	:		\
			syntax == ARISYNTAX  ? "ARI"	:		\
					"???"			)
#endif

	startlinno = plinno;
	varnest = 0;
	quoted = 0;
	if (syntax == DQSYNTAX)
		SETDBLQUOTE();
	quotef = 0;
	bqlist = NULL;
	arinest = 0;
	parenlevel = 0;
	elided_nl = 0;
	magicq = oneword;

	CTRACE(DBG_LEXER, ("readtoken1(%c) syntax=%s %s%s(quoted=%x)\n",
	    firstc&0xFF, SYNTAX, magicq ? "magic quotes" : "",
	    ISDBLQUOTE()?" ISDBLQUOTE":"", quoted));

	STARTSTACKSTR(out);

	for (c = firstc ;; c = pgetc_macro()) {	/* until of token */
		if (syntax == ARISYNTAX)
			out = insert_elided_nl(out);
		CHECKSTRSPACE(6, out);	/* permit 6 calls to USTPUTC */
		switch (syntax[c]) {
		case CFAKE:
			VTRACE(DBG_LEXER, ("CFAKE"));
			if (syntax == BASESYNTAX && varnest == 0)
				break;
			VTRACE(DBG_LEXER, (","));
			continue;
		case CNL:	/* '\n' */
			VTRACE(DBG_LEXER, ("CNL"));
			if (syntax == BASESYNTAX && varnest == 0)
				break;	/* exit loop */
			USTPUTC(c, out);
			plinno++;
			VTRACE(DBG_LEXER, ("@%d,", plinno));
			if (doprompt)
				setprompt(2);
			else
				setprompt(0);
			continue;

		case CSBACK:	/* single quoted backslash */
			if ((quoted & QF) == CQ) {
				out = readcstyleesc(out);
				continue;
			}
			VTRACE(DBG_LEXER, ("ESC:"));
			USTPUTC(CTLESC, out);
			/* FALLTHROUGH */
		case CWORD:
			VTRACE(DBG_LEXER, ("'%c'", c));
			USTPUTC(c, out);
			continue;

		case CCTL:
			CVTRACE(DBG_LEXER, !magicq || ISDBLQUOTE(),
			    ("%s%sESC:",!magicq?"!m":"",ISDBLQUOTE()?"DQ":""));
			if (!magicq || ISDBLQUOTE())
				USTPUTC(CTLESC, out);
			VTRACE(DBG_LEXER, ("'%c'", c));
			USTPUTC(c, out);
			continue;
		case CBACK:	/* backslash */
			c = pgetc();
			VTRACE(DBG_LEXER, ("\\'%c'(%#.2x)", c&0xFF, c&0x1FF));
			if (c == PEOF) {
				VTRACE(DBG_LEXER, ("EOF, keep \\ "));
				USTPUTC('\\', out);
				pungetc();
				continue;
			}
			if (c == '\n') {
				plinno++;
				elided_nl++;
				VTRACE(DBG_LEXER, ("eli \\n (%d) @%d ",
				    elided_nl, plinno));
				if (doprompt)
					setprompt(2);
				else
					setprompt(0);
				continue;
			}
			CVTRACE(DBG_LEXER, quotef==0, (" QF=1 "));
			quotef = 1;	/* current token is quoted */
			if (quoted && c != '\\' && c != '`' &&
			    (c != '}' || varnest == 0) &&
			    c != '$' && (c != '"' || magicq)) {
				/*
				 * retain the \ (which we *know* needs CTLESC)
				 * when in "..." and the following char is
				 * not one of the magic few.)
				 * Otherwise the \ has done its work, and
				 * is dropped.
				 */
				VTRACE(DBG_LEXER, ("ESC:'\\'"));
				USTPUTC(CTLESC, out);
				USTPUTC('\\', out);
			}
			CVTRACE(DBG_LEXER, NEEDESC(c) || !magicq,
			    ("%sESC:", NEEDESC(c) ? "+" : "m"));
			VTRACE(DBG_LEXER, ("'%c'(%#.2x)", c&0xFF, c&0x1FF));
			if (NEEDESC(c))
				USTPUTC(CTLESC, out);
			else if (!magicq) {
				USTPUTC(CTLESC, out);
				USTPUTC(c, out);
				continue;
			}
			USTPUTC(c, out);
			continue;
		case CSQUOTE:
			if (syntax != SQSYNTAX) {
				CVTRACE(DBG_LEXER, !magicq, (" CQM "));
				if (!magicq)
					USTPUTC(CTLQUOTEMARK, out);
				CVTRACE(DBG_LEXER, quotef==0, (" QF=1 "));
				quotef = 1;
				TS_PUSH();
				syntax = SQSYNTAX;
				quoted = SQ;
				VTRACE(DBG_LEXER, (" TS_PUSH(SQ)"));
				continue;
			}
			if (magicq && arinest == 0 && varnest == 0) {
				/* Ignore inside quoted here document */
				VTRACE(DBG_LEXER, ("<<'>>"));
				USTPUTC(c, out);
				continue;
			}
			/* End of single quotes... */
			TS_POP();
			VTRACE(DBG_LEXER, ("SQ TS_POP->%s ", SYNTAX));
			CVTRACE(DBG_LEXER, syntax == BASESYNTAX, (" CQE "));
			if (syntax == BASESYNTAX)
				USTPUTC(CTLQUOTEEND, out);
			continue;
		case CDQUOTE:
			if (magicq && arinest == 0 /* && varnest == 0 */) {
				VTRACE(DBG_LEXER, ("<<\">>"));
				/* Ignore inside here document */
				USTPUTC(c, out);
				continue;
			}
			CVTRACE(DBG_LEXER, quotef==0, (" QF=1 "));
			quotef = 1;
			if (arinest) {
				if (ISDBLQUOTE()) {
					VTRACE(DBG_LEXER,
					    (" CQE ari(%d", arinest));
					USTPUTC(CTLQUOTEEND, out);
					TS_POP();
					VTRACE(DBG_LEXER, ("%d)TS_POP->%s ",
					    arinest, SYNTAX));
				} else {
					VTRACE(DBG_LEXER,
					  (" ari(%d) %s TS_PUSH->DQ CQM ",
					   arinest, SYNTAX));
					TS_PUSH();
					syntax = DQSYNTAX;
					SETDBLQUOTE();
					USTPUTC(CTLQUOTEMARK, out);
				}
				continue;
			}
			CVTRACE(DBG_LEXER, magicq, (" MQignDQ "));
			if (magicq)
				continue;
			if (ISDBLQUOTE()) {
				TS_POP();
				VTRACE(DBG_LEXER,
				    (" DQ TS_POP->%s CQE ", SYNTAX));
				USTPUTC(CTLQUOTEEND, out);
			} else {
				VTRACE(DBG_LEXER,
				    (" %s TS_POP->DQ CQM ", SYNTAX));
				TS_PUSH();
				syntax = DQSYNTAX;
				SETDBLQUOTE();
				USTPUTC(CTLQUOTEMARK, out);
			}
			continue;
		case CVAR:	/* '$' */
			VTRACE(DBG_LEXER, ("'$'..."));
			out = insert_elided_nl(out);
			PARSESUB();		/* parse substitution */
			continue;
		case CENDVAR:	/* CLOSEBRACE */
			if (varnest > 0 && !ISDBLQUOTE()) {
				VTRACE(DBG_LEXER, ("vn=%d !DQ", varnest));
				TS_POP();
				VTRACE(DBG_LEXER, (" TS_POP->%s CEV ", SYNTAX));
				USTPUTC(CTLENDVAR, out);
			} else {
				VTRACE(DBG_LEXER, ("'%c'", c));
				USTPUTC(c, out);
			}
			out = insert_elided_nl(out);
			continue;
		case CLP:	/* '(' in arithmetic */
			parenlevel++;
			VTRACE(DBG_LEXER, ("'('(%d)", parenlevel));
			USTPUTC(c, out);
			continue;
		case CRP:	/* ')' in arithmetic */
			if (parenlevel > 0) {
				USTPUTC(c, out);
				--parenlevel;
				VTRACE(DBG_LEXER, ("')'(%d)", parenlevel));
			} else {
				VTRACE(DBG_LEXER, ("')'(%d)", parenlevel));
				if (pgetc_linecont() == /*(*/ ')') {
					out = insert_elided_nl(out);
					if (--arinest == 0) {
						TS_POP();
						USTPUTC(CTLENDARI, out);
					} else
						USTPUTC(/*(*/ ')', out);
				} else {
					break;	/* to synerror() just below */
#if 0	/* the old way, causes weird errors on bad input */
					/*
					 * unbalanced parens
					 *  (don't 2nd guess - no error)
					 */
					pungetc();
					USTPUTC(/*(*/ ')', out);
#endif
				}
			}
			continue;
		case CBQUOTE:	/* '`' */
			VTRACE(DBG_LEXER, ("'`' -> parsebackq()\n"));
			out = parsebackq(stack, out, &bqlist, 1);
			VTRACE(DBG_LEXER, ("parsebackq() -> readtoken1: "));
			continue;
		case CEOF:		/* --> c == PEOF */
			VTRACE(DBG_LEXER, ("EOF "));
			break;		/* will exit loop */
		default:
			VTRACE(DBG_LEXER, ("['%c'(%#.2x)]", c&0xFF, c&0x1FF));
			if (varnest == 0 && !ISDBLQUOTE())
				break;	/* exit loop */
			USTPUTC(c, out);
			VTRACE(DBG_LEXER, (","));
			continue;
		}
		VTRACE(DBG_LEXER, (" END TOKEN\n", c&0xFF, c&0x1FF));
		break;	/* break from switch -> break from for loop too */
	}

	if (syntax == ARISYNTAX) {
		cleanup_state_stack(stack);
		synerror(/*((*/ "Missing '))'");
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

	STPUTC('\0', out);
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

	VTRACE(DBG_PARSE|DBG_LEXER,
	    ("readtoken1 %sword \"%s\", completed%s (%d) left %d enl\n",
	    (quotef ? "quoted " : ""), out, (bqlist ? " with cmdsubs" : ""),
	    len, elided_nl));

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
	int subtype;
	int typeloc;
	int flags;
	char *p;
	static const char types[] = "}-+?=";

	c = pgetc_linecont();
	VTRACE(DBG_LEXER, ("\"$%c\"(%#.2x)", c&0xFF, c&0x1FF));
	if (c == '(' /*)*/) {	/* $(command) or $((arith)) */
		if (pgetc_linecont() == '(' /*')'*/ ) {
			VTRACE(DBG_LEXER, ("\"$((\" ARITH "));
			out = insert_elided_nl(out);
			PARSEARITH();
		} else {
			VTRACE(DBG_LEXER, ("\"$(\" CSUB->parsebackq()\n"));
			out = insert_elided_nl(out);
			pungetc();
			out = parsebackq(stack, out, &bqlist, 0);
			VTRACE(DBG_LEXER, ("parseback()->readtoken1(): "));
		}
	} else if (c == OPENBRACE || is_name(c) || is_special(c)) {
		VTRACE(DBG_LEXER, (" $EXP:CTLVAR "));
		USTPUTC(CTLVAR, out);
		typeloc = out - stackblock();
		USTPUTC(VSNORMAL, out);
		subtype = VSNORMAL;
		flags = 0;
		if (c == OPENBRACE) {
			c = pgetc_linecont();
			if (c == '#') {
				if ((c = pgetc_linecont()) == CLOSEBRACE)
					c = '#';
				else if (is_name(c) || isdigit(c))
					subtype = VSLENGTH;
				else if (is_special(c)) {
					/*
					 * ${#} is $# - the number of sh params
					 * ${##} is the length of ${#}
					 * ${###} is ${#} with as much nothing
					 *        as possible removed from start
					 * ${##1} is ${#} with leading 1 gone
					 * ${##\#} is ${#} with leading # gone
					 *
					 * this stuff is UGLY!
					 */
					if (pgetc_linecont() == CLOSEBRACE) {
						pungetc();
						subtype = VSLENGTH;
					} else {
						static char cbuf[2];

						pungetc();   /* would like 2 */
						cbuf[0] = c; /* so ... */
						cbuf[1] = '\0';
						pushstring(cbuf, 1, NULL);
						c = '#';     /* ${#:...} */
						subtype = 0; /* .. or similar */
					}
				} else {
					pungetc();
					c = '#';
					subtype = 0;
				}
			}
			else
				subtype = 0;
			VTRACE(DBG_LEXER, ("${ st=%d ", subtype));
		}
		if (is_name(c)) {
			p = out;
			do {
				VTRACE(DBG_LEXER, ("%c", c));
				STPUTC(c, out);
				c = pgetc_linecont();
			} while (is_in_name(c));

#if 0
			if (out - p == 6 && strncmp(p, "LINENO", 6) == 0) {
				int i;
				int linno;
				char buf[10];

				/*
				 * The "LINENO hack"
				 *
				 * Replace the variable name with the
				 * current line number.
				 */
				linno = plinno;
				if (funclinno != 0)
					linno -= funclinno - 1;
				snprintf(buf, sizeof(buf), "%d", linno);
				STADJUST(-6, out);
				for (i = 0; buf[i] != '\0'; i++)
					STPUTC(buf[i], out);
				flags |= VSLINENO;
			}
#endif
		} else if (is_digit(c)) {
			do {
				VTRACE(DBG_LEXER, ("%c", c));
				STPUTC(c, out);
				c = pgetc_linecont();
			} while (subtype != VSNORMAL && is_digit(c));
		}
		else if (is_special(c)) {
			VTRACE(DBG_LEXER, ("\"$%c", c));
			USTPUTC(c, out);
			c = pgetc_linecont();
		}
		else {
			VTRACE(DBG_LEXER, ("\"$%c(%#.2x)??\n", c&0xFF,c&0x1FF));
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
			if (subtype == VSLENGTH && c != /*{*/ '}')
				synerror("no modifiers allowed with ${#var}");
			pungetc();
		}
		if (quoted || arinest)
			flags |= VSQUOTE;
		if (subtype >= VSTRIMLEFT && subtype <= VSTRIMRIGHTMAX)
			flags |= VSPATQ;
		VTRACE(DBG_LEXER, (" st%d:%x", subtype, flags));
		*(stackblock() + typeloc) = subtype | flags;
		if (subtype != VSNORMAL) {
			TS_PUSH();
			varnest++;
			arinest = 0;
			if (subtype > VSASSIGN) {	/* # ## % %% */
				syntax = BASESYNTAX;
				quoted = 0;
				magicq = 0;
			}
			VTRACE(DBG_LEXER, (" TS_PUSH->%s vn=%d%s ",
			    SYNTAX, varnest, quoted ? " Q" : ""));
		}
	} else if (c == '\'' && syntax == BASESYNTAX) {
		USTPUTC(CTLQUOTEMARK, out);
		VTRACE(DBG_LEXER, (" CSTR \"$'\" CQM "));
		CVTRACE(DBG_LEXER, quotef==0, ("QF=1 "));
		quotef = 1;
		TS_PUSH();
		syntax = SQSYNTAX;
		quoted = CQ;
		VTRACE(DBG_LEXER, ("%s->TS_PUSH()->SQ ", SYNTAX));
	} else {
		VTRACE(DBG_LEXER, ("$unk -> '$' (pushback '%c'%#.2x)",
			c & 0xFF, c & 0x1FF));
		USTPUTC('$', out);
		pungetc();
	}
	goto parsesub_return;
}


/*
 * Parse an arithmetic expansion (indicate start of one and set state)
 */
parsearith: {

#if 0
	if (syntax == ARISYNTAX) {
		/*
		 * we collapse embedded arithmetic expansion to
		 * parentheses, which should be equivalent
		 *
		 *	XXX It isn't, must fix, soonish...
		 */
		USTPUTC('(' /*)*/, out);
		USTPUTC('(' /*)*/, out);
		/*
		 * Need 2 of them because there will (should be)
		 * two closing ))'s to follow later.
		 */
		parenlevel += 2;
	} else
#endif
	{
		VTRACE(DBG_LEXER, (" CTLARI%c ", ISDBLQUOTE()?'"':'_'));
		USTPUTC(CTLARI, out);
		if (ISDBLQUOTE())
			USTPUTC('"',out);
		else
			USTPUTC(' ',out);

		VTRACE(DBG_LEXER, ("%s->TS_PUSH->ARI(1)", SYNTAX));
		TS_PUSH();
		syntax = ARISYNTAX;
		arinest = 1;
		varnest = 0;
		magicq = 1;
	}
	goto parsearith_return;
}

} /* end of readtoken */




#ifdef mkinit
INCLUDE "parser.h"

RESET {
	psp.v_current_parser = &parse_state;

	parse_state.ps_tokpushback = 0;
	parse_state.ps_checkkwd = 0;
	parse_state.ps_heredoclist = NULL;
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
		if (c == CTLQUOTEMARK || c == CTLQUOTEEND)
			continue;
		if (c == CTLESC)
			p++;
		else if (ISCTL(c))
			return 0;
	}
	return 1;
}


/*
 * Return true if the argument is a legal variable name (a letter or
 * underscore followed by zero or more letters, underscores, and digits).
 */

int
goodname(const char *name)
{
	const char *p;

	p = name;
	if (! is_name(*p))
		return 0;
	while (*++p) {
		if (! is_in_name(*p))
			return 0;
	}
	return 1;
}

int
isassignment(const char *p)
{
	if (!is_name(*p))
		return 0;
	while (*++p != '=')
		if (*p == '\0' || !is_in_name(*p))
			return 0;
	return 1;
}

/*
 * skip past any \n's, and leave lasttoken set to whatever follows
 */
STATIC void
linebreak(void)
{
	while (readtoken() == TNL)
		readheredocs();
}

/*
 * The next token must be "token" -- check, then move past it
 */
STATIC void
consumetoken(int token)
{
	if (readtoken() != token) {
		VTRACE(DBG_PARSE, ("consumetoken(%d): expecting %s got %s",
		    token, tokname[token], tokname[lasttoken]));
		CVTRACE(DBG_PARSE, (lasttoken==TWORD), (" \"%s\"", wordtext));
		VTRACE(DBG_PARSE, ("\n"));
		synexpect(token, NULL);
	}
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

	while ((c = pgetc()) == '\\') {
		c = pgetc();
		if (c == '\n') {
			plinno++;
			elided_nl++;
			VTRACE(DBG_LEXER, ("\"\\n\"drop(el=%d@%d)",
			    elided_nl, plinno));
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
	char *p;
	const char *cp;
	int wp;

	if (!doprompt)
		return "";

	VTRACE(DBG_PARSE|DBG_EXPAND, ("getprompt %d\n", whichprompt));

	switch (wp = whichprompt) {
	case 0:
		return "";
	case 1:
		p = ps1val();
		break;
	case 2:
		p = ps2val();
		break;
	default:
		return "<internal prompt error>";
	}
	if (p == NULL)
		return "";

	VTRACE(DBG_PARSE|DBG_EXPAND, ("prompt <<%s>>\n", p));

	cp = expandstr(p, plinno);
	whichprompt = wp;	/* history depends on it not changing */

	VTRACE(DBG_PARSE|DBG_EXPAND, ("prompt -> <<%s>>\n", cp));

	return cp;
}

/*
 * Expand a string ... used for expanding prompts (PS1...)
 *
 * Never return NULL, always some string (return input string if invalid)
 *
 * The internal routine does the work, leaving the result on the
 * stack (or in a static string, or even the input string) and
 * handles parser recursion, and cleanup after an error while parsing.
 *
 * The visible interface copies the result off the stack (if it is there),
 * and handles stack management, leaving the stack in the exact same
 * state it was when expandstr() was called (so it can be used part way
 * through building a stack data structure - as in when PS2 is being
 * expanded half way through reading a "command line")
 *
 * on error, expandonstack() cleans up the parser state, but then
 * simply jumps out through expandstr() without doing any stack cleanup,
 * which is OK, as the error handler must deal with that anyway.
 *
 * The split into two funcs is to avoid problems with setjmp/longjmp
 * and local variables which could otherwise be optimised into bizarre
 * behaviour.
 */
static const char *
expandonstack(char *ps, int cmdsub, int lineno)
{
	union node n;
	struct jmploc jmploc;
	struct jmploc *const savehandler = handler;
	struct parsefile *const savetopfile = getcurrentfile();
	const int save_x = xflag;
	const int save_e_s = errors_suppressed;
	struct parse_state new_state = init_parse_state;
	struct parse_state *const saveparser = psp.v_current_parser;
	const char *result = NULL;

	if (!setjmp(jmploc.loc)) {
		handler = &jmploc;
		errors_suppressed = 1;

		psp.v_current_parser = &new_state;
		setinputstring(ps, 1, lineno);

		readtoken1(pgetc(), DQSYNTAX, 1);
		if (backquotelist != NULL) {
			if (!cmdsub) 
				result = ps;
			else if (!promptcmds)
				result = "-o promptcmds not set: ";
		}
		if (result == NULL) {
			n.narg.type = NARG;
			n.narg.next = NULL;
			n.narg.text = wordtext;
			n.narg.lineno = lineno;
			n.narg.backquote = backquotelist;

			xflag = 0;	/* we might be expanding PS4 ... */
			expandarg(&n, NULL, 0);
			result = stackblock();
		}
	} else {
		psp.v_current_parser = saveparser;
		xflag = save_x;
		popfilesupto(savetopfile);
		handler = savehandler;
		errors_suppressed = save_e_s;

		if (exception == EXEXIT)
			longjmp(handler->loc, 1);
		if (exception == EXINT)
			exraise(SIGINT);
		return "";
	}
	psp.v_current_parser = saveparser;
	xflag = save_x;
	popfilesupto(savetopfile);
	handler = savehandler;
	errors_suppressed = save_e_s;

	if (result == NULL)
		result = ps;

	return result;
}

const char *
expandstr(char *ps, int lineno)
{
	const char *result = NULL;
	struct stackmark smark;
	static char *buffer = NULL;	/* storage for prompt, never freed */
	static size_t bufferlen = 0;

	setstackmark(&smark);
	/*
	 * At this point we anticipate that there may be a string
	 * growing on the stack, but we have no idea how big it is.
	 * However we know that it cannot be bigger than the current
	 * allocated stack block, so simply reserve the whole thing,
	 * then we can use the stack without barfing all over what
	 * is there already...   (the stack mark undoes this later.)
	 */
	(void) stalloc(stackblocksize());

	result = expandonstack(ps, 1, lineno);

	if (__predict_true(result == stackblock())) {
		size_t len = strlen(result) + 1;

		/*
		 * the result (usual case) is on the stack, which we
		 * are just about to discard (popstackmark()) so we
		 * need to move it somewhere safe first.
		 */

		if (__predict_false(len > bufferlen)) {
			char *new;
			size_t newlen = bufferlen;

			if (__predict_false(len > (SIZE_MAX >> 4))) {
				result = "huge prompt: ";
				goto getout;
			}

			if (newlen == 0)
				newlen = 32;
			while (newlen <= len)
				newlen <<= 1;

			new = (char *)realloc(buffer, newlen);

			if (__predict_false(new == NULL)) {
				/*
				 * this should rarely (if ever) happen
				 * but we must do something when it does...
				 */
				result = "No mem for prompt: ";
				goto getout;
			} else {
				buffer = new;
				bufferlen = newlen;
			}
		}
		(void)memcpy(buffer, result, len);
		result = buffer;
	}

  getout:;
	popstackmark(&smark);

	return result;
}

/*
 * and a simpler version, which does no $( ) expansions, for
 * use during shell startup when we know we are not parsing,
 * and so the stack is not in use - we can do what we like,
 * and do not need to clean up (that's handled externally).
 *
 * Simply return the result, even if it is on the stack
 */
const char *
expandenv(char *arg)
{
	return expandonstack(arg, 0, 0);
}
