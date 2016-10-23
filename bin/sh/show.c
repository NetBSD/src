/*	$NetBSD: show.c,v 1.34 2016/10/23 08:24:27 abhinav Exp $	*/

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
static char sccsid[] = "@(#)show.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: show.c,v 1.34 2016/10/23 08:24:27 abhinav Exp $");
#endif
#endif /* not lint */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "shell.h"
#include "parser.h"
#include "nodes.h"
#include "mystring.h"
#include "show.h"
#include "options.h"
#ifndef SMALL
#define DEFINE_NODENAMES
#include "nodenames.h"
#endif


FILE *tracefile;

#ifdef DEBUG
static int shtree(union node *, int, int, char *, FILE*);
static int shcmd(union node *, FILE *);
static int shsubsh(union node *, FILE *);
static int shredir(union node *, FILE *, int);
static int sharg(union node *, FILE *);
static int indent(int, char *, FILE *);
static void trstring(char *);

void
showtree(union node *n)
{
	FILE *fp;

	fp = tracefile ? tracefile : stdout;

	trputs("showtree(");
		if (n == NULL)
			trputs("NULL");
		else if (n == NEOF)
			trputs("NEOF");
	trputs(") called\n");
	if (n != NULL && n != NEOF)
		shtree(n, 1, 1, NULL, fp);
}


static int
shtree(union node *n, int ind, int nl, char *pfx, FILE *fp)
{
	struct nodelist *lp;
	const char *s;
	int len;

	if (n == NULL) {
		if (nl)
			fputc('\n', fp);
		return 0;
	}

	len = indent(ind, pfx, fp);
	switch (n->type) {
	case NSEMI:
		s = "; ";
		len += 2;
		goto binop;
	case NAND:
		s = " && ";
		len += 4;
		goto binop;
	case NOR:
		s = " || ";
		len += 4;
binop:
		len += shtree(n->nbinary.ch1, 0, 0, NULL, fp);
		fputs(s, fp);
		if (len >= 60) {
			putc('\n', fp);
			len = indent(ind < 0 ? 2 : ind + 1, pfx, fp);
		}
		len += shtree(n->nbinary.ch2, 0, nl, NULL, fp);
		break;
	case NCMD:
		len += shcmd(n, fp);
		if (nl && len > 0)
			len = 0, putc('\n', fp);
		break;
	case NPIPE:
		for (lp = n->npipe.cmdlist ; lp ; lp = lp->next) {
			len += shcmd(lp->n, fp);
			if (lp->next) {
				len += 3, fputs(" | ", fp);
				if (len >= 60)  {
					fputc('\n', fp);
					len = indent(ind < 0 ? 2 : ind + 1,
					    pfx, fp);
				}
			}
		}
		if (n->npipe.backgnd)
			len += 2, fputs(" &", fp);
		if (nl || len >= 60)
			len = 0, fputc('\n', fp);
		break;
	case NSUBSHELL:
		len += shsubsh(n, fp);
		if (nl && len > 0)
			len = 0, putc('\n', fp);
		break;
	default:
#ifdef NODETYPENAME
		len += fprintf(fp, "<node type %d [%s]>", n->type,
		    NODETYPENAME(n->type));
#else
		len += fprintf(fp, "<node type %d>", n->type);
#endif
		if (nl)
			len = 0, putc('\n', fp);
		break;
	}
	return len;
}



static int
shcmd(union node *cmd, FILE *fp)
{
	union node *np;
	int first;
	int len = 0;

	first = 1;
	for (np = cmd->ncmd.args ; np ; np = np->narg.next) {
		if (! first)
			len++, fputc(' ', fp);
		len += sharg(np, fp);
		first = 0;
	}
	return len + shredir(cmd, fp, first);
}

static int
shsubsh(union node *cmd, FILE *fp)
{
	int len = 6;

	fputs(" ( ", fp);
	len += shtree(cmd->nredir.n, -1, 0, NULL, fp);
	fputs(" ) ", fp);
	len += shredir(cmd, fp, 1);

	return len;
}

static int
shredir(union node *cmd, FILE *fp, int first)
{
	union node *np;
	const char *s;
	int dftfd;
	int len = 0;
	char buf[106];

	for (np = cmd->ncmd.redirect ; np ; np = np->nfile.next) {
		if (! first)
			len++, fputc(' ', fp);
		switch (np->nfile.type) {
			case NTO:	s = ">";  dftfd = 1; len += 1; break;
			case NCLOBBER:	s = ">|"; dftfd = 1; len += 2; break;
			case NAPPEND:	s = ">>"; dftfd = 1; len += 2; break;
			case NTOFD:	s = ">&"; dftfd = 1; len += 2; break;
			case NFROM:	s = "<";  dftfd = 0; len += 1; break;
			case NFROMFD:	s = "<&"; dftfd = 0; len += 2; break;
			case NFROMTO:	s = "<>"; dftfd = 0; len += 2; break;
			case NXHERE:	/* FALLTHROUGH */ 
			case NHERE:	s = "<<"; dftfd = 0; len += 2; break;
			default:   s = "*error*"; dftfd = 0; len += 7; break;
		}
		if (np->nfile.fd != dftfd)
			len += fprintf(fp, "%d", np->nfile.fd);
		fputs(s, fp);
		if (np->nfile.type == NTOFD || np->nfile.type == NFROMFD) {
			len += fprintf(fp, "%d", np->ndup.dupfd);
		} else
		    if (np->nfile.type == NHERE || np->nfile.type == NXHERE) {
			if (np->nfile.type == NHERE)
				fputc('\\', fp);
			fputs("!!!\n", fp);
			s = np->nhere.doc->narg.text;
			if (strlen(s) > 100) {
				memmove(buf, s, 100);
				buf[100] = '\0';
				strcat(buf, " ...");
				s = buf;
			}
			fputs(s, fp);
			fputs("!!!", fp);
			len = 3;
		} else {
			len += sharg(np->nfile.fname, fp);
		}
		first = 0;
	}
	return len;
}



static int
sharg(union node *arg, FILE *fp)
{
	char *p;
	struct nodelist *bqlist;
	int subtype;
	int len = 0;

	if (arg->type != NARG) {
		fprintf(fp, "<node type %d>\n", arg->type);
		abort();
	}
	bqlist = arg->narg.backquote;
	for (p = arg->narg.text ; *p ; p++) {
		switch (*p) {
		case CTLESC:
			putc(*++p, fp);
			len++;
			break;
		case CTLVAR:
			putc('$', fp);
			putc('{', fp);
			len += 2;
			subtype = *++p;
			if (subtype == VSLENGTH)
				len++, putc('#', fp);

			while (*++p != '=')
				len++, putc(*p, fp);

			if (subtype & VSNUL)
				len++, putc(':', fp);

			switch (subtype & VSTYPE) {
			case VSNORMAL:
				putc('}', fp);
				len++;
				break;
			case VSMINUS:
				putc('-', fp);
				len++;
				break;
			case VSPLUS:
				putc('+', fp);
				len++;
				break;
			case VSQUESTION:
				putc('?', fp);
				len++;
				break;
			case VSASSIGN:
				putc('=', fp);
				len++;
				break;
			case VSTRIMLEFT:
				putc('#', fp);
				len++;
				break;
			case VSTRIMLEFTMAX:
				putc('#', fp);
				putc('#', fp);
				len += 2;
				break;
			case VSTRIMRIGHT:
				putc('%', fp);
				len++;
				break;
			case VSTRIMRIGHTMAX:
				putc('%', fp);
				putc('%', fp);
				len += 2;
				break;
			case VSLENGTH:
				break;
			default:
				len += fprintf(fp, "<subtype %d>", subtype);
			}
			break;
		case CTLENDVAR:
		     putc('}', fp);
		     len++;
		     break;
		case CTLBACKQ:
		case CTLBACKQ|CTLQUOTE:
			putc('$', fp);
			putc('(', fp);
			len += shtree(bqlist->n, -1, 0, NULL, fp) + 3;
			putc(')', fp);
			break;
		default:
			putc(*p, fp);
			len++;
			break;
		}
	}
	return len;
}


static int
indent(int amount, char *pfx, FILE *fp)
{
	int i;
	int len = 0;

	/*
	 * in practice, pfx is **always** NULL
	 * but here, we assume if it were not, at least strlen(pfx) < 8
	 * if that is invalid, output will look messy
	 */
	for (i = 0 ; i < amount ; i++) {
		if (pfx && i == amount - 1)
			fputs(pfx, fp);
		putc('\t', fp);
		len |= 7;
		len++;
	}
	return len;
}
#endif



/*
 * Debugging stuff.
 */




#ifdef DEBUG
void
trputc(int c)
{
	if (debug != 1 || !tracefile)
		return;
	putc(c, tracefile);
}
#endif

void
trace(const char *fmt, ...)
{
#ifdef DEBUG
	va_list va;

	if (debug != 1 || !tracefile)
		return;
	va_start(va, fmt);
	(void) vfprintf(tracefile, fmt, va);
	va_end(va);
#endif
}

void
tracev(const char *fmt, va_list va)
{
#ifdef DEBUG
	va_list ap;
	if (debug != 1 || !tracefile)
		return;
	va_copy(ap, va);
	(void) vfprintf(tracefile, fmt, ap);
	va_end(ap);
#endif
}


#ifdef DEBUG
void
trputs(const char *s)
{
	if (debug != 1 || !tracefile)
		return;
	fputs(s, tracefile);
}


static void
trstring(char *s)
{
	char *p;
	char c;

	if (debug != 1 || !tracefile)
		return;
	putc('"', tracefile);
	for (p = s ; *p ; p++) {
		switch (*p) {
		case '\n':  c = 'n';  goto backslash;
		case '\t':  c = 't';  goto backslash;
		case '\r':  c = 'r';  goto backslash;
		case '"':  c = '"';  goto backslash;
		case '\\':  c = '\\';  goto backslash;
		case CTLESC:  c = 'e';  goto backslash;
		case CTLVAR:  c = 'v';  goto backslash;
		case CTLVAR+CTLQUOTE:  c = 'V';  goto backslash;
		case CTLBACKQ:  c = 'q';  goto backslash;
		case CTLBACKQ+CTLQUOTE:  c = 'Q';  goto backslash;
backslash:	  putc('\\', tracefile);
			putc(c, tracefile);
			break;
		default:
			if (*p >= ' ' && *p <= '~')
				putc(*p, tracefile);
			else {
				putc('\\', tracefile);
				putc(*p >> 6 & 03, tracefile);
				putc(*p >> 3 & 07, tracefile);
				putc(*p & 07, tracefile);
			}
			break;
		}
	}
	putc('"', tracefile);
}
#endif


void
trargs(char **ap)
{
#ifdef DEBUG
	if (debug != 1 || !tracefile)
		return;
	while (*ap) {
		trstring(*ap++);
		if (*ap)
			putc(' ', tracefile);
		else
			putc('\n', tracefile);
	}
#endif
}


#ifdef DEBUG
void
opentrace(void)
{
	char s[100];
#ifdef O_APPEND
	int flags;
#endif

	if (debug != 1) {
		if (tracefile)
			fflush(tracefile);
		/* leave open because libedit might be using it */
		return;
	}
	snprintf(s, sizeof(s), "./trace.%d", (int)getpid());
	if (tracefile) {
		if (!freopen(s, "a", tracefile)) {
			fprintf(stderr, "Can't re-open %s\n", s);
			tracefile = NULL;
			debug = 0;
			return;
		}
	} else {
		if ((tracefile = fopen(s, "a")) == NULL) {
			fprintf(stderr, "Can't open %s\n", s);
			debug = 0;
			return;
		}
	}
#ifdef O_APPEND
	if ((flags = fcntl(fileno(tracefile), F_GETFL, 0)) >= 0)
		fcntl(fileno(tracefile), F_SETFL, flags | O_APPEND);
#endif
	setlinebuf(tracefile);
	fputs("\nTracing started.\n", tracefile);
}
#endif /* DEBUG */
