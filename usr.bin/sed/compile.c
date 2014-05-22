/*	$NetBSD: compile.c,v 1.37.6.2 2014/05/22 11:42:49 yamt Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis of Imperial College, University of London.
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

/*-
 * Copyright (c) 1992 Diomidis Spinellis.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis of Imperial College, University of London.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)compile.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: compile.c,v 1.37.6.2 2014/05/22 11:42:49 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "extern.h"

#ifndef _POSIX2_LINE_MAX
#define _POSIX2_LINE_MAX (2 * BUFSIZ)
#endif

#define LHSZ	128
#define	LHMASK	(LHSZ - 1)
static struct labhash {
	struct	labhash *lh_next;
	u_int	lh_hash;
	struct	s_command *lh_cmd;
	int	lh_ref;
} *labels[LHSZ];

static char	 *compile_addr(char *, struct s_addr *);
static char	 *compile_ccl(char **, char *);
static char	 *compile_delimited(char *, char *);
static char	 *compile_flags(char *, struct s_subst *);
static char	 *compile_re(char *, regex_t **);
static char	 *compile_subst(char *, struct s_subst *);
static char	 *compile_text(void);
static char	 *compile_tr(char *, char **);
static struct s_command
		**compile_stream(struct s_command **);
static char	 *duptoeol(char *, const char *);
static void	  enterlabel(struct s_command *);
static struct s_command
		 *findlabel(char *);
static void	  fixuplabel(struct s_command *, struct s_command *);
static void	  uselabel(void);

/*
 * Command specification.  This is used to drive the command parser.
 */
struct s_format {
	char code;				/* Command code */
	int naddr;				/* Number of address args */
	enum e_args args;			/* Argument type */
};

static struct s_format cmd_fmts[] = {
	{'{', 2, GROUP},
	{'}', 0, ENDGROUP},
	{'a', 1, TEXT},
	{'b', 2, BRANCH},
	{'c', 2, TEXT},
	{'d', 2, EMPTY},
	{'D', 2, EMPTY},
	{'g', 2, EMPTY},
	{'G', 2, EMPTY},
	{'h', 2, EMPTY},
	{'H', 2, EMPTY},
	{'i', 1, TEXT},
	{'l', 2, EMPTY},
	{'n', 2, EMPTY},
	{'N', 2, EMPTY},
	{'p', 2, EMPTY},
	{'P', 2, EMPTY},
	{'q', 1, EMPTY},
	{'r', 1, RFILE},
	{'s', 2, SUBST},
	{'t', 2, BRANCH},
	{'w', 2, WFILE},
	{'x', 2, EMPTY},
	{'y', 2, TR},
	{'!', 2, NONSEL},
	{':', 0, LABEL},
	{'#', 0, COMMENT},
	{'=', 1, EMPTY},
	{'\0', 0, COMMENT},
};

/* The compiled program. */
struct s_command *prog;

/*
 * Compile the program into prog.
 * Initialise appends.
 */
void
compile(void)
{
	*compile_stream(&prog) = NULL;
	fixuplabel(prog, NULL);
	uselabel();
	if (appendnum > 0)
		appends = xmalloc(sizeof(struct s_appends) * appendnum);
	match = xmalloc((maxnsub + 1) * sizeof(regmatch_t));
}

#define EATSPACE() 						\
	while (*p && isascii((unsigned char)*p) &&		\
	    isspace((unsigned char)*p))				\
		p++						\

static struct s_command **
compile_stream(struct s_command **link)
{
	char *p;
	static char *lbuf;	/* To avoid excessive malloc calls */
	static size_t bufsize;
	struct s_command *cmd, *cmd2, *stack;
	struct s_format *fp;
	int naddr;				/* Number of addresses */

	stack = 0;
	for (;;) {
		if ((p = cu_fgets(&lbuf, &bufsize)) == NULL) {
			if (stack != 0)
				err(COMPILE, "unexpected EOF (pending }'s)");
			return (link);
		}

semicolon:	EATSPACE();
		if (*p == '#' || *p == '\0')
			continue;
		else if (*p == ';') {
			p++;
			goto semicolon;
		}
		*link = cmd = xmalloc(sizeof(struct s_command));
		link = &cmd->next;
		cmd->nonsel = cmd->inrange = 0;
		/* First parse the addresses */
		naddr = 0;

/* Valid characters to start an address */
#define	addrchar(c)	(strchr("0123456789/\\$", (c)))
		if (addrchar(*p)) {
			naddr++;
			cmd->a1 = xmalloc(sizeof(struct s_addr));
			p = compile_addr(p, cmd->a1);
			EATSPACE();				/* EXTENSION */
			if (*p == ',') {
				p++;
				EATSPACE();			/* EXTENSION */
				naddr++;
				cmd->a2 = xmalloc(sizeof(struct s_addr));
				p = compile_addr(p, cmd->a2);
				EATSPACE();
			} else
				cmd->a2 = 0;
		} else
			cmd->a1 = cmd->a2 = 0;

nonsel:		/* Now parse the command */
		if (!*p)
			err(COMPILE, "command expected");
		cmd->code = *p;
		for (fp = cmd_fmts; fp->code; fp++)
			if (fp->code == *p)
				break;
		if (!fp->code)
			err(COMPILE, "invalid command code %c", *p);
		if (naddr > fp->naddr)
			err(COMPILE,
"command %c expects up to %d address(es), found %d", *p, fp->naddr, naddr);
		switch (fp->args) {
		case NONSEL:			/* ! */
			p++;
			EATSPACE();
			cmd->nonsel = ! cmd->nonsel;
			goto nonsel;
		case GROUP:			/* { */
			p++;
			EATSPACE();
			cmd->next = stack;
			stack = cmd;
			link = &cmd->u.c;
			if (*p)
				goto semicolon;
			break;
		case ENDGROUP:
			/*
			 * Short-circuit command processing, since end of
			 * group is really just a noop.
			 */
			cmd->nonsel = 1;
			if (stack == 0)
				err(COMPILE, "unexpected }");
			cmd2 = stack;
			stack = cmd2->next;
			cmd2->next = cmd;
			/*FALLTHROUGH*/
		case EMPTY:		/* d D g G h H l n N p P q x = \0 */
			p++;
			EATSPACE();
			switch (*p) {
			case ';':
				p++;
				link = &cmd->next;
				goto semicolon;
			case '}':
				goto semicolon;
			case '\0':
				break;
			default:
				err(COMPILE,
"extra characters at the end of %c command", cmd->code);
			}
			break;
		case TEXT:			/* a c i */
			p++;
			EATSPACE();
			if (*p != '\\')
				err(COMPILE,
"command %c expects \\ followed by text", cmd->code);
			p++;
			EATSPACE();
			if (*p)
				err(COMPILE,
"extra characters after \\ at the end of %c command", cmd->code);
			cmd->t = compile_text();
			break;
		case COMMENT:			/* \0 # */
			break;
		case WFILE:			/* w */
			p++;
			EATSPACE();
			if (*p == '\0')
				err(COMPILE, "filename expected");
			cmd->t = duptoeol(p, "w command");
			if (aflag)
				cmd->u.fd = -1;
			else if ((cmd->u.fd = open(p, 
			    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,
			    DEFFILEMODE)) == -1)
				err(FATAL, "%s: %s", p, strerror(errno));
			break;
		case RFILE:			/* r */
			p++;
			EATSPACE();
			if (*p == '\0')
				err(COMPILE, "filename expected");
			else
				cmd->t = duptoeol(p, "read command");
			break;
		case BRANCH:			/* b t */
			p++;
			EATSPACE();
			if (*p == '\0')
				cmd->t = NULL;
			else
				cmd->t = duptoeol(p, "branch");
			break;
		case LABEL:			/* : */
			p++;
			EATSPACE();
			cmd->t = duptoeol(p, "label");
			if (strlen(p) == 0)
				err(COMPILE, "empty label");
			enterlabel(cmd);
			break;
		case SUBST:			/* s */
			p++;
			if (*p == '\0' || *p == '\\')
				err(COMPILE,
"substitute pattern can not be delimited by newline or backslash");
			cmd->u.s = xmalloc(sizeof(struct s_subst));
			p = compile_re(p, &cmd->u.s->re);
			if (p == NULL)
				err(COMPILE, "unterminated substitute pattern");
			--p;
			p = compile_subst(p, cmd->u.s);
			p = compile_flags(p, cmd->u.s);
			EATSPACE();
			if (*p == ';') {
				p++;
				link = &cmd->next;
				goto semicolon;
			}
			break;
		case TR:			/* y */
			p++;
			p = compile_tr(p, (char **)(void *)&cmd->u.y);
			EATSPACE();
			switch (*p) {
			case ';':
				p++;
				link = &cmd->next;
				goto semicolon;
			case '}':
				goto semicolon;
			case '\0':
				break;
			default:
				err(COMPILE,
"extra text at the end of a transform command");
			}
			break;
		}
	}
}

/*
 * Get a delimited string.  P points to the delimiter of the string; d points
 * to a buffer area.  Newline and delimiter escapes are processed; other
 * escapes are ignored.
 *
 * Returns a pointer to the first character after the final delimiter or NULL
 * in the case of a non-terminated string.  The character array d is filled
 * with the processed string.
 */
static char *
compile_delimited(char *p, char *d)
{
	char c;

	c = *p++;
	if (c == '\0')
		return (NULL);
	else if (c == '\\')
		err(COMPILE, "\\ can not be used as a string delimiter");
	else if (c == '\n')
		err(COMPILE, "newline can not be used as a string delimiter");
	while (*p) {
		if (*p == '[') {
			if ((d = compile_ccl(&p, d)) == NULL)
				err(COMPILE, "unbalanced brackets ([])");
			continue;
		} else if (*p == '\\' && p[1] == '[') {
			*d++ = *p++;
		} else if (*p == '\\' && p[1] == c)
			p++;
		else if (*p == '\\' && p[1] == 'n') {
			*d++ = '\n';
			p += 2;
			continue;
		} else if (*p == '\\' && p[1] == '\\')
			*d++ = *p++;
		else if (*p == c) {
			*d = '\0';
			return (p + 1);
		}
		*d++ = *p++;
	}
	return (NULL);
}


/* compile_ccl: expand a POSIX character class */
static char *
compile_ccl(char **sp, char *t)
{
	int c, d;
	char *s = *sp;

	*t++ = *s++;
	if (*s == '^')
		*t++ = *s++;
	if (*s == ']')
		*t++ = *s++;
	for (; *s && (*t = *s) != ']'; s++, t++)
		if (*s == '[' && ((d = *(s+1)) == '.' || d == ':' || d == '=')) {
			*++t = *++s, t++, s++;
			for (c = *s; (*t = *s) != ']' || c != d; s++, t++)
				if ((c = *s) == '\0')
					return NULL;
		} else if (*s == '\\' && s[1] == 'n')
			    *t = '\n', s++;
	return (*s == ']') ? *sp = ++s, ++t : NULL;
}

/*
 * Get a regular expression.  P points to the delimiter of the regular
 * expression; repp points to the address of a regexp pointer.  Newline
 * and delimiter escapes are processed; other escapes are ignored.
 * Returns a pointer to the first character after the final delimiter
 * or NULL in the case of a non terminated regular expression.  The regexp
 * pointer is set to the compiled regular expression.
 * Cflags are passed to regcomp.
 */
static char *
compile_re(char *p, regex_t **repp)
{
	int eval;
	char *re;

	re = xmalloc(strlen(p) + 1); /* strlen(re) <= strlen(p) */
	p = compile_delimited(p, re);
	if (p && strlen(re) == 0) {
		*repp = NULL;
		free(re);
		return (p);
	}
	*repp = xmalloc(sizeof(regex_t));
	if (p && (eval = regcomp(*repp, re, ere)) != 0)
		err(COMPILE, "RE error: %s", strregerror(eval, *repp));
	if (maxnsub < (*repp)->re_nsub)
		maxnsub = (*repp)->re_nsub;
	free(re);
	return (p);
}

/*
 * Compile the substitution string of a regular expression and set res to
 * point to a saved copy of it.  Nsub is the number of parenthesized regular
 * expressions.
 */
static char *
compile_subst(char *p, struct s_subst *s)
{
	static char *lbuf;
	static size_t bufsize;
	int asize, ref, size, len;
	char c, *text, *op, *sp;
	int sawesc = 0;

	c = *p++;			/* Terminator character */
	if (c == '\0')
		return (NULL);

	s->maxbref = 0;
	s->linenum = linenum;
	text = NULL;
	asize = size = 0;
	do {
		len = ROUNDLEN(strlen(p) + 1);
		if (asize - size < len) {
			do {
				asize += len;
			} while (asize - size < len);
			text = xrealloc(text, asize);
		}
		op = sp = text + size;
		for (; *p; p++) {
			if (*p == '\\' || sawesc) {
				/*
				 * If this is a continuation from the last
				 * buffer, we won't have a character to
				 * skip over.
				 */
				if (sawesc)
					sawesc = 0;
				else
					p++;

				if (*p == '\0') {
					/*
					 * This escaped character is continued
					 * in the next part of the line.  Note
					 * this fact, then cause the loop to
					 * exit w/ normal EOL case and reenter
					 * above with the new buffer.
					 */
					sawesc = 1;
					p--;
					continue;
				} else if (strchr("123456789", *p) != NULL) {
					*sp++ = '\\';
					ref = *p - '0';
					if (s->re != NULL &&
					    (size_t)ref > s->re->re_nsub)
						err(COMPILE,
"\\%c not defined in the RE", *p);
					if (s->maxbref < ref)
						s->maxbref = ref;
				} else if (*p == '&' || *p == '\\')
					*sp++ = '\\';
			} else if (*p == c) {
				p++;
				*sp++ = '\0';
				size += sp - op;
				s->new = xrealloc(text, size);
				return (p);
			} else if (*p == '\n') {
				err(COMPILE,
"unescaped newline inside substitute pattern");
				/* NOTREACHED */
			}
			*sp++ = *p;
		}
		size += sp - op;
	} while ((p = cu_fgets(&lbuf, &bufsize)));
	err(COMPILE, "unterminated substitute in regular expression");
	/* NOTREACHED */
	return (NULL);
}

/*
 * Compile the flags of the s command
 */
static char *
compile_flags(char *p, struct s_subst *s)
{
	int gn;			/* True if we have seen g or n */
	char wfile[PATH_MAX], *q;

	s->n = 1;				/* Default */
	s->p = 0;
	s->wfile = NULL;
	s->wfd = -1;
	for (gn = 0;;) {
		EATSPACE();			/* EXTENSION */
		switch (*p) {
		case 'g':
			if (gn)
				err(COMPILE,
"more than one number or 'g' in substitute flags");
			gn = 1;
			s->n = 0;
			break;
		case '\0':
		case '\n':
		case ';':
			return (p);
		case 'p':
			s->p = 1;
			break;
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			if (gn)
				err(COMPILE,
"more than one number or 'g' in substitute flags");
			gn = 1;
			/* XXX Check for overflow */
			s->n = (int)strtol(p, &p, 10);
			p--;
			break;
		case 'w':
			p++;
#ifdef HISTORIC_PRACTICE
			if (*p != ' ') {
				err(WARNING, "space missing before w wfile");
				return (p);
			}
#endif
			EATSPACE();
			q = wfile;
			while (*p) {
				if (*p == '\n')
					break;
				*q++ = *p++;
			}
			*q = '\0';
			if (q == wfile)
				err(COMPILE, "no wfile specified");
			s->wfile = strdup(wfile);
			if (!aflag && (s->wfd = open(wfile,
			    O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,
			    DEFFILEMODE)) == -1)
				err(FATAL, "%s: %s", wfile, strerror(errno));
			return (p);
		default:
			err(COMPILE,
			    "bad flag in substitute command: '%c'", *p);
			break;
		}
		p++;
	}
}

/*
 * Compile a translation set of strings into a lookup table.
 */
static char *
compile_tr(char *p, char **transtab)
{
	int i;
	char *lt, *op, *np;
	char *old = NULL, *new = NULL;

	if (*p == '\0' || *p == '\\')
		err(COMPILE,
"transform pattern can not be delimited by newline or backslash");
	old = xmalloc(strlen(p) + 1);
	p = compile_delimited(p, old);
	if (p == NULL) {
		err(COMPILE, "unterminated transform source string");
		goto bad;
	}
	new = xmalloc(strlen(p) + 1);
	p = compile_delimited(p - 1, new);
	if (p == NULL) {
		err(COMPILE, "unterminated transform target string");
		goto bad;
	}
	EATSPACE();
	if (strlen(new) != strlen(old)) {
		err(COMPILE, "transform strings are not the same length");
		goto bad;
	}
	/* We assume characters are 8 bits */
	lt = xmalloc(UCHAR_MAX+1);
	for (i = 0; i <= UCHAR_MAX; i++)
		lt[i] = (char)i;
	for (op = old, np = new; *op; op++, np++)
		lt[(u_char)*op] = *np;
	*transtab = lt;
	free(old);
	free(new);
	return (p);
bad:
	free(old);
	free(new);
	return (NULL);
}

/*
 * Compile the text following an a, c, or i command.
 */
static char *
compile_text(void)
{
	int asize, size, len;
	char *lbuf, *text, *p, *op, *s;
	size_t bufsize;

	lbuf = text = NULL;
	asize = size = 0;
	while ((p = cu_fgets(&lbuf, &bufsize))) {
		len = ROUNDLEN(strlen(p) + 1);
		if (asize - size < len) {
			do {
				asize += len;
			} while (asize - size < len);
			text = xrealloc(text, asize);
		}
		op = s = text + size;
		for (; *p; p++) {
			if (*p == '\\')
				p++;
			*s++ = *p;
		}
		size += s - op;
		if (p[-2] != '\\') {
			*s = '\0';
			break;
		}
	}
	free(lbuf);
	return (xrealloc(text, size + 1));
}

/*
 * Get an address and return a pointer to the first character after
 * it.  Fill the structure pointed to according to the address.
 */
static char *
compile_addr(char *p, struct s_addr *a)
{
	char *end;

	switch (*p) {
	case '\\':				/* Context address */
		++p;
		/* FALLTHROUGH */
	case '/':				/* Context address */
		p = compile_re(p, &a->u.r);
		if (p == NULL)
			err(COMPILE, "unterminated regular expression");
		a->type = AT_RE;
		return (p);

	case '$':				/* Last line */
		a->type = AT_LAST;
		return (p + 1);
						/* Line number */
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9':
		a->type = AT_LINE;
		a->u.l = strtol(p, &end, 10);
		return (end);
	default:
		err(COMPILE, "expected context address");
		return (NULL);
	}
}

/*
 * duptoeol --
 *	Return a copy of all the characters up to \n or \0.
 */
static char *
duptoeol(char *s, const char *ctype)
{
	size_t len;
	int ws;
	char *start;

	ws = 0;
	for (start = s; *s != '\0' && *s != '\n'; ++s)
		ws = isspace((unsigned char)*s);
	*s = '\0';
	if (ws)
		err(WARNING, "whitespace after %s", ctype);
	len = s - start + 1;
	return (memmove(xmalloc(len), start, len));
}

/*
 * Convert goto label names to addresses, and count a and r commands, in
 * the given subset of the script.  Free the memory used by labels in b
 * and t commands (but not by :).
 *
 * TODO: Remove } nodes
 */
static void
fixuplabel(struct s_command *cp, struct s_command *end)
{

	for (; cp != end; cp = cp->next)
		switch (cp->code) {
		case 'a':
		case 'r':
			appendnum++;
			break;
		case 'b':
		case 't':
			/* Resolve branch target. */
			if (cp->t == NULL) {
				cp->u.c = NULL;
				break;
			}
			if ((cp->u.c = findlabel(cp->t)) == NULL)
				err(COMPILE2, "undefined label '%s'", cp->t);
			free(cp->t);
			break;
		case '{':
			/* Do interior commands. */
			fixuplabel(cp->u.c, cp->next);
			break;
		}
}

/*
 * Associate the given command label for later lookup.
 */
static void
enterlabel(struct s_command *cp)
{
	struct labhash **lhp, *lh;
	u_char *p;
	u_int h, c;

	for (h = 0, p = (u_char *)cp->t; (c = *p) != 0; p++)
		h = (h << 5) + h + c;
	lhp = &labels[h & LHMASK];
	for (lh = *lhp; lh != NULL; lh = lh->lh_next)
		if (lh->lh_hash == h && strcmp(cp->t, lh->lh_cmd->t) == 0)
			err(COMPILE2, "duplicate label '%s'", cp->t);
	lh = xmalloc(sizeof *lh);
	lh->lh_next = *lhp;
	lh->lh_hash = h;
	lh->lh_cmd = cp;
	lh->lh_ref = 0;
	*lhp = lh;
}

/*
 * Find the label contained in the command l in the command linked
 * list cp.  L is excluded from the search.  Return NULL if not found.
 */
static struct s_command *
findlabel(char *name)
{
	struct labhash *lh;
	u_char *p;
	u_int h, c;

	for (h = 0, p = (u_char *)name; (c = *p) != 0; p++)
		h = (h << 5) + h + c;
	for (lh = labels[h & LHMASK]; lh != NULL; lh = lh->lh_next) {
		if (lh->lh_hash == h && strcmp(name, lh->lh_cmd->t) == 0) {
			lh->lh_ref = 1;
			return (lh->lh_cmd);
		}
	}
	return (NULL);
}

/* 
 * Warn about any unused labels.  As a side effect, release the label hash
 * table space.
 */
static void
uselabel(void)
{
	struct labhash *lh, *next;
	int i;

	for (i = 0; i < LHSZ; i++) {
		for (lh = labels[i]; lh != NULL; lh = next) {
			next = lh->lh_next;
			if (!lh->lh_ref)
				err(WARNING, "unused label '%s'",
				    lh->lh_cmd->t);
			free(lh);
		}
	}
}
