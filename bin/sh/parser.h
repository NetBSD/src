/*	$NetBSD: parser.h,v 1.30 2024/10/21 15:57:45 kre Exp $	*/

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
 *
 *	@(#)parser.h	8.3 (Berkeley) 5/4/95
 */

/* control characters in argument strings */
#define CTL_FIRST '\201'	/* first 'special' character */
#define CTLESC '\201'		/* escape next character */
#define CTLVAR '\202'		/* variable defn */
#define CTLENDVAR '\203'
#define CTLBACKQ '\204'
#define CTLQUOTE 01		/* ored with CTLBACKQ code if in quotes */
/*	CTLBACKQ | CTLQUOTE == '\205' */
#define	CTLARI	'\206'		/* arithmetic expression */
#define	CTLENDARI '\207'
#define	CTLQUOTEMARK '\210'
#define	CTLQUOTEEND '\211'	/* only inside ${...} */
#define	CTLNONL '\212'		/* The \n in a deleted \ \n sequence */
			/* pure concidence that (CTLNONL & 0x7f) == '\n' */
#define	CTLCNL	'\213'		/* A $'\n' - newline not counted */
#define	CTLVARMOD '\214'	/* a modifier in a variable expansion */
#define	CTL_LAST '\214'		/* last 'special' character */

/* variable substitution byte (follows CTLVAR) */
#define VSTYPE		0x0f	/* type of variable substitution */
#define VSNUL		0x10	/* colon--treat the empty string as unset */
#define VSLINENO	0x20	/* expansion of $LINENO, the line number
				   follows immediately */
#define VSPATQ		0x40	/* ensure correct pattern quoting in ${x#pat} */
#define VSQUOTE	 	0x80	/* inside double quotes--suppress splitting */

/* values of VSTYPE field (nb: 0 reserved for "not determined yet") */
#define VSNORMAL	0x1		/* normal variable:  $var or ${var} */
#define VSMINUS		0x2		/* ${var-text} */
#define VSPLUS		0x3		/* ${var+text} */
#define VSQUESTION	0x4		/* ${var?message} */
#define VSASSIGN	0x5		/* ${var=text} */
#define VSTRIMLEFT	0x6		/* ${var#pattern} */
#define VSTRIMLEFTMAX	0x7		/* ${var##pattern} */
#define VSTRIMRIGHT	0x8		/* ${var%pattern} */
#define VSTRIMRIGHTMAX 	0x9		/* ${var%%pattern} */
#define VSLENGTH	0xa		/* ${#var} */
#define VSUNKNOWN	0xf		/* unknown modifier */

union node *parsecmd(int);
void fixredir(union node *, const char *, int);
int goodname(const char *);
int isassignment(const char *);
const char *getprompt(void *);
const char *expandstr(char *, int);
const char *expandvar(char *, int);
const char *expandenv(char *);

struct HereDoc;
union node;
struct nodelist;

struct parse_state {
	struct HereDoc *ps_heredoclist;	/* list of here documents to read */
	int ps_parsebackquote;		/* nonzero inside backquotes */
	int ps_doprompt;		/* if set, prompt the user */
	int ps_needprompt;		/* true if interactive at line start */
	int ps_lasttoken;		/* last token read */
	int ps_tokpushback;		/* last token pushed back */
	char *ps_wordtext;	/* text of last word returned by readtoken */
	int ps_checkkwd;		/* word expansion flags, see below */
	struct nodelist *ps_backquotelist; /* list of cmdsubs to process */
	union node *ps_redirnode;	/* node for current redirect */
	struct HereDoc *ps_heredoc;	/* current heredoc << being parsed */
	int ps_quoteflag;		/* set if (part) of token was quoted */
	int ps_startlinno;		/* line # where last token started */
	int ps_funclinno;		/* line # of the current function */
	int ps_elided_nl;		/* count of \ \n pairs we have seen */
};

/*
 * The parser references the elements of struct parse_state quite
 * frequently - they used to be simple globals, so one memory ref
 * per access, adding an indirect through a global ptr would not be
 * nice.   The following gross hack allows most of that cost to be
 * avoided, by allowing the compiler to understand that the global
 * pointer is in fact constant in any function, and so its value can
 * be cached, rather than needing to be fetched every time in case
 * some other called function has changed it.
 *
 * The rule to make this work is that any function that wants
 * to alter the global must restore it before it returns (and thus
 * must have an error trap handler).  That means that the struct
 * used for the new parser state can be a local in that function's
 * stack frame, it never needs to be malloc'd.
 */

union parse_state_p {
	struct parse_state *const	c_current_parser;
	struct parse_state *		v_current_parser;
};

extern union parse_state_p psp;

#define	current_parser (psp.c_current_parser)

/*
 * Perhaps one day emulate "static" by moving most of these definitions into
 * parser.c ...  (only checkkwd & tokpushback are used outside parser.c,
 * and only in init.c as a RESET activity)
 */
#define	tokpushback	(current_parser->ps_tokpushback)
#define	checkkwd	(current_parser->ps_checkkwd)

#define	heredoclist	(current_parser->ps_heredoclist)
#define	parsebackquote	(current_parser->ps_parsebackquote)
#define	doprompt	(current_parser->ps_doprompt)
#define	needprompt	(current_parser->ps_needprompt)
#define	lasttoken	(current_parser->ps_lasttoken)
#define	wordtext	(current_parser->ps_wordtext)
#define	backquotelist	(current_parser->ps_backquotelist)
#define	redirnode	(current_parser->ps_redirnode)
#define	heredoc		(current_parser->ps_heredoc)
#define	quoteflag	(current_parser->ps_quoteflag)
#define	startlinno	(current_parser->ps_startlinno)
#define	funclinno	(current_parser->ps_funclinno)
#define	elided_nl	(current_parser->ps_elided_nl)

/*
 * Values that can be set in checkkwd
 */
#define CHKKWD		0x01		/* turn word into keyword (if it is) */
#define CHKNL		0x02		/* ignore leading \n's */
#define CHKALIAS	0x04		/* lookup words as aliases and ... */

/*
 * NEOF is returned by parsecmd when it encounters an end of file.  It
 * must be distinct from NULL, so we use the address of a variable that
 * happens to be handy.
 */
#define NEOF ((union node *)&psp)

#ifdef DEBUG
extern int parsing;
#endif
