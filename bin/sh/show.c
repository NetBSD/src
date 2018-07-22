/*	$NetBSD: show.c,v 1.48 2018/07/22 20:38:06 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Copyright (c) 2017 The NetBSD Foundation, Inc.  All rights reserved.
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
__RCSID("$NetBSD: show.c,v 1.48 2018/07/22 20:38:06 kre Exp $");
#endif
#endif /* not lint */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/uio.h>

#include "shell.h"
#include "parser.h"
#include "nodes.h"
#include "mystring.h"
#include "show.h"
#include "options.h"
#include "redir.h"
#include "error.h"
#include "syntax.h"
#include "input.h"
#include "output.h"
#include "var.h"
#include "builtins.h"

#if defined(DEBUG) && !defined(DBG_PID)
/*
 * If this is compiled, it means this is being compiled in a shell that still
 * has an older shell.h (a simpler TRACE() mechanism than is coming soon.)
 *
 * Compensate for as much of that as is missing and is needed here
 * to compile and operate at all.   After the other changes have appeared,
 * this little block can (and should be) deleted (sometime).
 *
 * Try to avoid waiting 22 years...
 */
#define	DBG_PID		1
#define	DBG_NEST	2
#endif

#define DEFINE_NODENAMES
#include "nodenames.h"		/* does almost nothing if !defined(DEBUG) */

#define	TR_STD_WIDTH	60	/* tend to fold lines wider than this */
#define	TR_IOVECS	10	/* number of lines or trace (max) / write */

typedef struct traceinfo {
	int	tfd;	/* file descriptor for open trace file */
	int	nxtiov;	/* the buffer we should be writing to */
	char	lastc;	/* the last non-white character output */
	uint8_t	supr;	/* char classes to suppress after \n */
	pid_t	pid;	/* process id of process that opened that file */
	size_t	llen;	/* number of chars in current output line */
	size_t	blen;	/* chars used in current buffer being filled */
	char *	tracefile;		/* name of the tracefile */
	struct iovec lines[TR_IOVECS];	/* filled, flling, pending buffers */
} TFILE;

/* These are auto turned off when non white space is printed */
#define	SUP_NL	0x01	/* don't print \n */
#define	SUP_SP	0x03	/* suppress spaces */
#define	SUP_WSP	0x04	/* suppress all white space */

#ifdef DEBUG		/* from here to end of file ... */

TFILE tracedata, *tracetfile;
FILE *tracefile;		/* just for histedit */

uint64_t	DFlags;		/* currently enabled debug flags */
int ShNest;			/* depth of shell (internal) nesting */

static void shtree(union node *, int, int, int, TFILE *);
static void shcmd(union node *, TFILE *);
static void shsubsh(union node *, TFILE *);
static void shredir(union node *, TFILE *, int);
static void sharg(union node *, TFILE *);
static void indent(int, TFILE *);
static void trstring(const char *);
static void trace_putc(char, TFILE *);
static void trace_puts(const char *, TFILE *);
static void trace_flush(TFILE *, int);
static char *trace_id(TFILE *);
static void trace_fd_swap(int, int);

inline static int trlinelen(TFILE *);


/*
 * These functions are the externally visible interface
 *  (but only for a DEBUG shell.)
 */

void
opentrace(void)
{
	char *s;
	int fd;
	int i;
	pid_t pid;

	if (debug != 1) {
		/* leave fd open because libedit might be using it */
		if (tracefile)
			fflush(tracefile);
		if (tracetfile)
			trace_flush(tracetfile, 1);
		return;
	}
#if DBG_PID == 1		/* using old shell.h, old tracing method */
	DFlags = DBG_PID;	/* just force DBG_PID on, and leave it ... */
#endif
	pid = getpid();
	if (asprintf(&s, "trace.%jd", (intmax_t)pid) <= 0) {
		debug = 0;
		error("Cannot asprintf tracefilename");
	};

	fd = open(s, O_WRONLY|O_APPEND|O_CREAT, 0666);
	if (fd == -1) {
		debug = 0;
		error("Can't open tracefile: %s (%s)\n", s, strerror(errno));
	}
	fd = to_upper_fd(fd);
	if (fd <= 2) {
		(void) close(fd);
		debug = 0;
		error("Attempt to use fd %d as tracefile thwarted\n", fd);
	}
	register_sh_fd(fd, trace_fd_swap);

	/*
	 * This stuff is just so histedit has a FILE * to use
	 */
	if (tracefile)
		(void) fclose(tracefile);	/* also closes tfd */
	tracefile = fdopen(fd, "a");	/* don't care if it is NULL */
	if (tracefile)			/* except here... */
		setlinebuf(tracefile);

	/*
	 * Now the real tracing setup
	 */
	if (tracedata.tfd > 0 && tracedata.tfd != fd)
		(void) close(tracedata.tfd);	/* usually done by fclose() */

	tracedata.tfd = fd;
	tracedata.pid = pid;
	tracedata.nxtiov = 0;
	tracedata.blen = 0;
	tracedata.llen = 0;
	tracedata.lastc = '\0';
	tracedata.supr = SUP_NL | SUP_WSP;

#define	replace(f, v) do {				\
		if (tracedata.f != NULL)		\
			free(tracedata.f);		\
		tracedata.f = v;			\
	} while (/*CONSTCOND*/ 0)

	replace(tracefile, s);

	for (i = 0; i < TR_IOVECS; i++) {
		replace(lines[i].iov_base, NULL);
		tracedata.lines[i].iov_len = 0;
	}

#undef replace

	tracetfile = &tracedata;

	trace_puts("\nTracing started.\n", tracetfile);
}

void
trace(const char *fmt, ...)
{
	va_list va;
	char *s;

	if (debug != 1 || !tracetfile)
		return;
	va_start(va, fmt);
	(void) vasprintf(&s, fmt, va);
	va_end(va);

	trace_puts(s, tracetfile);
	free(s);
	if (tracetfile->llen == 0)
		trace_flush(tracetfile, 0);
}

void
tracev(const char *fmt, va_list va)
{
	va_list ap;
	char *s;

	if (debug != 1 || !tracetfile)
		return;
	va_copy(ap, va);
	(void) vasprintf(&s, fmt, ap);
	va_end(ap);

	trace_puts(s, tracetfile);
	free(s);
	if (tracetfile->llen == 0)
		trace_flush(tracetfile, 0);
}


void
trputs(const char *s)
{
	if (debug != 1 || !tracetfile)
		return;
	trace_puts(s, tracetfile);
}

void
trputc(int c)
{
	if (debug != 1 || !tracetfile)
		return;
	trace_putc(c, tracetfile);
}

void
showtree(union node *n)
{
	TFILE *fp;

	if ((fp = tracetfile) == NULL)
		return;

	trace_puts("showtree(", fp);
		if (n == NULL)
			trace_puts("NULL", fp);
		else if (n == NEOF)
			trace_puts("NEOF", fp);
		else
			trace("%p", n);
	trace_puts(") called\n", fp);
	if (n != NULL && n != NEOF)
		shtree(n, 1, 1, 1, fp);
}

void
trargs(char **ap)
{
	if (debug != 1 || !tracetfile)
		return;
	while (*ap) {
		trstring(*ap++);
		if (*ap)
			trace_putc(' ', tracetfile);
	}
	trace_putc('\n', tracetfile);
}

void
trargstr(union node *n)
{
	sharg(n, tracetfile);
}


/*
 * Beyond here we just have the implementation of all of that
 */


inline static int
trlinelen(TFILE * fp)
{
	return fp->llen;
}

static void
shtree(union node *n, int ind, int ilvl, int nl, TFILE *fp)
{
	struct nodelist *lp;
	const char *s;

	if (n == NULL) {
		if (nl)
			trace_putc('\n', fp);
		return;
	}

	indent(ind, fp);
	switch (n->type) {
	case NSEMI:
		s = NULL;
		goto binop;
	case NAND:
		s = " && ";
		goto binop;
	case NOR:
		s = " || ";
binop:
		shtree(n->nbinary.ch1, 0, ilvl, 0, fp);
		if (s != NULL)
			trace_puts(s, fp);
		if (trlinelen(fp) >= TR_STD_WIDTH) {
			trace_putc('\n', fp);
			indent(ind < 0 ? 2 : ind + 1, fp);
		} else if (s == NULL) {
			if (fp->lastc != '&')
				trace_puts("; ", fp);
			else
				trace_putc(' ', fp);
		}
		shtree(n->nbinary.ch2, 0, ilvl, nl, fp);
		break;
	case NCMD:
		shcmd(n, fp);
		if (n->ncmd.backgnd)
			trace_puts(" &", fp);
		if (nl && trlinelen(fp) > 0)
			trace_putc('\n', fp);
		break;
	case NPIPE:
		for (lp = n->npipe.cmdlist ; lp ; lp = lp->next) {
			shtree(lp->n, 0, ilvl, 0, fp);
			if (lp->next) {
				trace_puts(" |", fp);
				if (trlinelen(fp) >= TR_STD_WIDTH)  {
					trace_putc('\n', fp);
					indent((ind < 0 ? ilvl : ind) + 1, fp);
				} else
					trace_putc(' ', fp);
			}
		}
		if (n->npipe.backgnd)
			trace_puts(" &", fp);
		if (nl || trlinelen(fp) >= TR_STD_WIDTH)
			trace_putc('\n', fp);
		break;
	case NBACKGND:
	case NSUBSHELL:
		shsubsh(n, fp);
		if (n->type == NBACKGND)
			trace_puts(" &", fp);
		if (nl && trlinelen(fp) > 0)
			trace_putc('\n', fp);
		break;
	case NDEFUN:
		trace_puts(n->narg.text, fp);
		trace_puts("() {\n", fp);
		indent(ind, fp);
		shtree(n->narg.next, (ind < 0 ? ilvl : ind) + 1, ilvl+1, 1, fp);
		indent(ind, fp);
		trace_puts("}\n", fp);
		break;
	case NDNOT:
		trace_puts("! ", fp);
		/* FALLTHROUGH */
	case NNOT:
		trace_puts("! ", fp);
		shtree(n->nnot.com, -1, ilvl, nl, fp);
		break;
	case NREDIR:
		shtree(n->nredir.n, -1, ilvl, 0, fp);
		shredir(n->nredir.redirect, fp, n->nredir.n == NULL);
		if (nl)
			trace_putc('\n', fp);
		break;

	case NIF:
	itsif:
		trace_puts("if ", fp);
		shtree(n->nif.test, -1, ilvl, 0, fp);
		if (trlinelen(fp) > 0 && trlinelen(fp) < TR_STD_WIDTH) {
			if (fp->lastc != '&')
				trace_puts(" ;", fp);
		} else
			indent(ilvl, fp);
		trace_puts(" then ", fp);
		if (nl || trlinelen(fp) > TR_STD_WIDTH - 24)
			indent(ilvl+1, fp);
		shtree(n->nif.ifpart, -1, ilvl + 1, 0, fp);
		if (trlinelen(fp) > 0 && trlinelen(fp) < TR_STD_WIDTH) {
			if (fp->lastc != '&')
				trace_puts(" ;", fp);
		} else
			indent(ilvl, fp);
		if (n->nif.elsepart && n->nif.elsepart->type == NIF) {
			if (nl || trlinelen(fp) > TR_STD_WIDTH - 24)
				indent(ilvl, fp);
			n = n->nif.elsepart;
			trace_puts(" el", fp);
			goto itsif;
		}
		if (n->nif.elsepart) {
			if (nl || trlinelen(fp) > TR_STD_WIDTH - 24)
				indent(ilvl+1, fp);
			trace_puts(" else ", fp);
			shtree(n->nif.elsepart, -1, ilvl + 1, 0, fp);
			if (fp->lastc != '&')
				trace_puts(" ;", fp);
		}
		trace_puts(" fi", fp);
		if (nl)
			trace_putc('\n', fp);
		break;

	case NWHILE:
		trace_puts("while ", fp);
		goto aloop;
	case NUNTIL:
		trace_puts("until ", fp);
	aloop:
		shtree(n->nbinary.ch1, -1, ilvl, 0, fp);
		if (trlinelen(fp) > 0 && trlinelen(fp) < TR_STD_WIDTH) {
			if (fp->lastc != '&')
				trace_puts(" ;", fp);
		} else
			trace_putc('\n', fp);
		trace_puts(" do ", fp);
		shtree(n->nbinary.ch1, -1, ilvl + 1, 1, fp);
		trace_puts(" done ", fp);
		if (nl)
			trace_putc('\n', fp);
		break;

	case NFOR:
		trace_puts("for ", fp);
		trace_puts(n->nfor.var, fp);
		if (n->nfor.args) {
			union node *argp;

			trace_puts(" in ", fp);
			for (argp = n->nfor.args; argp; argp=argp->narg.next) {
				sharg(argp, fp);
				trace_putc(' ', fp);
			}
			if (trlinelen(fp) > 0 && trlinelen(fp) < TR_STD_WIDTH) {
				if (fp->lastc != '&')
					trace_putc(';', fp);
			} else
				trace_putc('\n', fp);
		}
		trace_puts(" do ", fp);
		shtree(n->nfor.body, -1, ilvl + 1, 0, fp);
		if (fp->lastc != '&')
			trace_putc(';', fp);
		trace_puts(" done", fp);
		if (nl)
			trace_putc('\n', fp);
		break;

	case NCASE:
		trace_puts("case ", fp);
		sharg(n->ncase.expr, fp);
		trace_puts(" in", fp);
		if (nl)
			trace_putc('\n', fp);
		{
			union node *cp;

			for (cp = n->ncase.cases ; cp ; cp = cp->nclist.next) {
				union node *patp;

				if (nl || trlinelen(fp) > TR_STD_WIDTH - 16)
					indent(ilvl, fp);
				else
					trace_putc(' ', fp);
				trace_putc('(', fp);
				patp = cp->nclist.pattern;
				while (patp != NULL) {
				    trace_putc(' ', fp);
				    sharg(patp, fp);
				    trace_putc(' ', fp);
				    if ((patp = patp->narg.next) != NULL)
					trace_putc('|', fp);
				}
				trace_putc(')', fp);
				if (nl)
					indent(ilvl + 1, fp);
				else
					trace_putc(' ', fp);
				shtree(cp->nclist.body, -1, ilvl+2, 0, fp);
				if (cp->type == NCLISTCONT)
					trace_puts(" ;&", fp);
				else
					trace_puts(" ;;", fp);
				if (nl)
					trace_putc('\n', fp);
			}
		}
		if (nl) {
			trace_putc('\n', fp);
			indent(ind, fp);
		} else
			trace_putc(' ', fp);
		trace_puts("esac", fp);
		if (nl)
			trace_putc('\n', fp);
		break;

	default: {
			char *str;

			asprintf(&str, "<node type %d [%s]>", n->type,
			    NODETYPENAME(n->type));
			trace_puts(str, fp);
			free(str);
			if (nl)
				trace_putc('\n', fp);
		}
		break;
	}
}


static void
shcmd(union node *cmd, TFILE *fp)
{
	union node *np;
	int first;

	first = 1;
	for (np = cmd->ncmd.args ; np ; np = np->narg.next) {
		if (! first)
			trace_putc(' ', fp);
		sharg(np, fp);
		first = 0;
	}
	shredir(cmd->ncmd.redirect, fp, first);
}

static void
shsubsh(union node *cmd, TFILE *fp)
{
	trace_puts(" ( ", fp);
	shtree(cmd->nredir.n, -1, 3, 0, fp);
	trace_puts(" ) ", fp);
	shredir(cmd->ncmd.redirect, fp, 1);
}

static void
shredir(union node *np, TFILE *fp, int first)
{
	const char *s;
	int dftfd;
	char buf[106];

	for ( ; np ; np = np->nfile.next) {
		if (! first)
			trace_putc(' ', fp);
		switch (np->nfile.type) {
			case NTO:	s = ">";  dftfd = 1; break;
			case NCLOBBER:	s = ">|"; dftfd = 1; break;
			case NAPPEND:	s = ">>"; dftfd = 1; break;
			case NTOFD:	s = ">&"; dftfd = 1; break;
			case NFROM:	s = "<";  dftfd = 0; break;
			case NFROMFD:	s = "<&"; dftfd = 0; break;
			case NFROMTO:	s = "<>"; dftfd = 0; break;
			case NXHERE:	/* FALLTHROUGH */
			case NHERE:	s = "<<"; dftfd = 0; break;
			default:   s = "*error*"; dftfd = 0; break;
		}
		if (np->nfile.fd != dftfd) {
			sprintf(buf, "%d", np->nfile.fd);
			trace_puts(buf, fp);
		}
		trace_puts(s, fp);
		if (np->nfile.type == NTOFD || np->nfile.type == NFROMFD) {
			if (np->ndup.vname)
				sharg(np->ndup.vname, fp);
			else {
				sprintf(buf, "%d", np->ndup.dupfd);
				trace_puts(buf, fp);
			}
		} else
		    if (np->nfile.type == NHERE || np->nfile.type == NXHERE) {
			if (np->nfile.type == NHERE)
				trace_putc('\\', fp);
			trace_puts("!!!\n", fp);
			s = np->nhere.doc->narg.text;
			if (strlen(s) > 100) {
				memmove(buf, s, 100);
				buf[100] = '\0';
				strcat(buf, " ...\n");
				s = buf;
			}
			trace_puts(s, fp);
			trace_puts("!!! ", fp);
		} else {
			sharg(np->nfile.fname, fp);
		}
		first = 0;
	}
}

static void
sharg(union node *arg, TFILE *fp)
{
	char *p, *s;
	struct nodelist *bqlist;
	int subtype = 0;
	int quoted = 0;

	if (arg->type != NARG) {
		asprintf(&s, "<node type %d> ! NARG\n", arg->type);
		trace_puts(s, fp);
		abort();	/* no need to free s, better not to */
	}

	bqlist = arg->narg.backquote;
	for (p = arg->narg.text ; *p ; p++) {
		switch (*p) {
		case CTLESC:
			trace_putc('\\', fp);
			trace_putc(*++p, fp);
			break;

		case CTLNONL:
			trace_putc('\\', fp);
			trace_putc('\n', fp);
			break;

		case CTLVAR:
			subtype = *++p;
			if (!quoted != !(subtype & VSQUOTE))
				trace_putc('"', fp);
			trace_putc('$', fp);
			trace_putc('{', fp);	/*}*/
			if ((subtype & VSTYPE) == VSLENGTH)
				trace_putc('#', fp);
			if (subtype & VSLINENO)
				trace_puts("LINENO=", fp);

			while (*++p != '=')
				trace_putc(*p, fp);

			if (subtype & VSNUL)
				trace_putc(':', fp);

			switch (subtype & VSTYPE) {
			case VSNORMAL:
				/* { */
				trace_putc('}', fp);
				if (!quoted != !(subtype & VSQUOTE))
					trace_putc('"', fp);
				break;
			case VSMINUS:
				trace_putc('-', fp);
				break;
			case VSPLUS:
				trace_putc('+', fp);
				break;
			case VSQUESTION:
				trace_putc('?', fp);
				break;
			case VSASSIGN:
				trace_putc('=', fp);
				break;
			case VSTRIMLEFTMAX:
				trace_putc('#', fp);
				/* FALLTHROUGH */
			case VSTRIMLEFT:
				trace_putc('#', fp);
				break;
			case VSTRIMRIGHTMAX:
				trace_putc('%', fp);
				/* FALLTHROUGH */
			case VSTRIMRIGHT:
				trace_putc('%', fp);
				break;
			case VSLENGTH:
				break;
			default: {
					char str[32];

					snprintf(str, sizeof str,
					    "<subtype %d>", subtype);
					trace_puts(str, fp);
				}
				break;
			}
			break;
		case CTLENDVAR:
			/* { */
			trace_putc('}', fp);
			if (!quoted != !(subtype & VSQUOTE))
				trace_putc('"', fp);
			subtype = 0;
			break;

		case CTLBACKQ|CTLQUOTE:
			if (!quoted)
				trace_putc('"', fp);
			/* FALLTHRU */
		case CTLBACKQ:
			trace_putc('$', fp);
			trace_putc('(', fp);
			if (bqlist) {
				shtree(bqlist->n, -1, 3, 0, fp);
				bqlist = bqlist->next;
			} else
				trace_puts("???", fp);
			trace_putc(')', fp);
			if (!quoted && *p == (CTLBACKQ|CTLQUOTE))
				trace_putc('"', fp);
			break;

		case CTLQUOTEMARK:
			if (subtype != 0 || !quoted) {
				trace_putc('"', fp);
				quoted++;
			}
			break;
		case CTLQUOTEEND:
			trace_putc('"', fp);
			quoted--;
			break;
		case CTLARI:
			if (*p == ' ')
				p++;
			trace_puts("$(( ", fp);
			break;
		case CTLENDARI:
			trace_puts(" ))", fp);
			break;

		default:
			if (*p == '$')
				trace_putc('\\', fp);
			trace_putc(*p, fp);
			break;
		}
	}
	if (quoted)
		trace_putc('"', fp);
}


static void
indent(int amount, TFILE *fp)
{
	int i;

	if (amount <= 0)
		return;

	amount <<= 2;	/* indent slots -> chars */

	i = trlinelen(fp);
	fp->supr = SUP_NL;
	if (i > amount) {
		trace_putc('\n', fp);
		i = 0;
	}
	fp->supr = 0;
	for (; i < amount - 7 ; i++) {
		trace_putc('\t', fp);
		i |= 7;
	}
	while (i < amount) {
		trace_putc(' ', fp);
		i++;
	}
	fp->supr = SUP_WSP;
}

static void
trace_putc(char c, TFILE *fp)
{
	char *p;

	if (c == '\0')
		return;
	if (debug == 0 || fp == NULL)
		return;

	if (fp->llen == 0) {
		if (fp->blen != 0)
			abort();

		if ((fp->supr & SUP_NL) && c == '\n')
			return;
		if ((fp->supr & (SUP_WSP|SUP_SP)) && c == ' ')
			return;
		if ((fp->supr & SUP_WSP) && c == '\t')
			return;

		if (fp->nxtiov >= TR_IOVECS - 1)	/* should be rare */
			trace_flush(fp, 0);

		p = trace_id(fp);
		if (p != NULL) {
			fp->lines[fp->nxtiov].iov_base = p;
			fp->lines[fp->nxtiov].iov_len = strlen(p);
			fp->nxtiov++;
		}
	} else if (fp->blen && fp->blen >= fp->lines[fp->nxtiov].iov_len) {
		fp->blen = 0;
		if (++fp->nxtiov >= TR_IOVECS)
			trace_flush(fp, 0);
	}

	if (fp->lines[fp->nxtiov].iov_len == 0) {
		p = (char *)malloc(2 * TR_STD_WIDTH);
		if (p == NULL) {
			trace_flush(fp, 1);
			debug = 0;
			return;
		}
		*p = '\0';
		fp->lines[fp->nxtiov].iov_base = p;
		fp->lines[fp->nxtiov].iov_len = 2 * TR_STD_WIDTH;
		fp->blen = 0;
	}

	p = (char *)fp->lines[fp->nxtiov].iov_base + fp->blen++;
	*p++ = c;
	*p = 0;

	if (c != ' ' && c != '\t' && c != '\n') {
		fp->lastc = c;
		fp->supr = 0;
	}

	if (c == '\n') {
		fp->lines[fp->nxtiov++].iov_len = fp->blen;
		fp->blen = 0;
		fp->llen = 0;
		fp->supr |= SUP_NL;
		return;
	}

	if (c == '\t')
		fp->llen |=  7;
	fp->llen++;
}

void
trace_flush(TFILE *fp, int all)
{
	int niov, i;
	ssize_t written;

	niov = fp->nxtiov;
	if (all && fp->blen > 0) {
		fp->lines[niov].iov_len = fp->blen;
		fp->blen = 0;
		fp->llen = 0;
		niov++;
	}
	if (niov == 0)
		return;
	if (fp->blen > 0 && --niov == 0)
		return;
	written = writev(fp->tfd, fp->lines, niov);
	for (i = 0; i < niov; i++) {
		free(fp->lines[i].iov_base);
		fp->lines[i].iov_base = NULL;
		fp->lines[i].iov_len = 0;
	}
	if (written == -1) {
		if (fp->blen > 0) {
			free(fp->lines[niov].iov_base);
			fp->lines[niov].iov_base = NULL;
			fp->lines[niov].iov_len = 0;
		}
		debug = 0;
		fp->blen = 0;
		fp->llen = 0;
		return;
	}
	if (fp->blen > 0) {
		fp->lines[0].iov_base = fp->lines[niov].iov_base;
		fp->lines[0].iov_len = fp->lines[niov].iov_len;
		fp->lines[niov].iov_base = NULL;
		fp->lines[niov].iov_len = 0;
	}
	fp->nxtiov = 0;
}

void
trace_puts(const char *s, TFILE *fp)
{
	char c;

	while ((c = *s++) != '\0')
		trace_putc(c, fp);
}

inline static char *
trace_id(TFILE *tf)
{
	int i;
	char indent[16];
	char *p;
	int lno;
	char c;

	if (DFlags & DBG_NEST) {
		if ((unsigned)ShNest >= sizeof indent - 1) {
			(void) snprintf(indent, sizeof indent,
			    "### %*d ###", (int)(sizeof indent) - 9, ShNest);
			p = strchr(indent, '\0');
		} else {
			p = indent;
			for (i = 0; i < 6; i++)
				*p++ = (i < ShNest) ? '#' : ' ';
			while (i++ < ShNest && p < &indent[sizeof indent - 1])
				*p++ = '#';
			*p = '\0';
		}
	} else
		indent[0] = '\0';

	/*
	 * If we are in the parser, then plinno is the current line
	 * number being processed (parser line no).
	 * If we are elsewhere, then line_number gives the source
	 * line of whatever we are currently doing (close enough.)
	 */
	if (parsing)
		lno = plinno;
	else
		lno = line_number;

	c = ((i = getpid()) == tf->pid) ? ':' : '=';

	if (DFlags & DBG_PID) {
		if (DFlags & DBG_LINE)
			(void) asprintf(&p, "%5d%c%s\t%4d%c@\t", i, c,
			    indent, lno, parsing?'-':'+');
		else
			(void) asprintf(&p, "%5d%c%s\t", i, c, indent);
		return p;
	} else if (DFlags & DBG_NEST) {
		if (DFlags & DBG_LINE)
			(void) asprintf(&p, "%c%s\t%4d%c@\t", c, indent, lno,
			    parsing?'-':'+');
		else
			(void) asprintf(&p, "%c%s\t", c, indent);
		return p;
	} else if (DFlags & DBG_LINE) {
		(void) asprintf(&p, "%c%4d%c@\t", c, lno, parsing?'-':'+');
		return p;
	}
	return NULL;
}

/*
 * Used only from trargs(), which itself is used only to print
 * arg lists (argv[]) either that passed into this shell, or
 * the arg list about to be given to some other command (incl
 * builtin, and function) as their argv[].  If any of the CTL*
 * chars seem to appear, they really should be just treated as data,
 * not special...   But this is just debug, so, who cares!
 */
static void
trstring(const char *s)
{
	const char *p;
	char c;
	TFILE *fp;

	if (debug != 1 || !tracetfile)
		return;
	fp = tracetfile;
	trace_putc('"', fp);
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
backslash:		trace_putc('\\', fp);
			trace_putc(c, fp);
			break;
		default:
			if (*p >= ' ' && *p <= '~')
				trace_putc(*p, fp);
			else {
				trace_putc('\\', fp);
				trace_putc(*p >> 6 & 03, fp);
				trace_putc(*p >> 3 & 07, fp);
				trace_putc(*p & 07, fp);
			}
			break;
		}
	}
	trace_putc('"', fp);
}

/*
 * deal with the user "accidentally" picking our fd to use.
 */
static void
trace_fd_swap(int from, int to)
{
	if (tracetfile == NULL || from == to)
		return;

	tracetfile->tfd = to;

	/*
	 * This is just so histedit has a stdio FILE* to use.
	 */
	if (tracefile)
		fclose(tracefile);
	tracefile = fdopen(to, "a");
	if (tracefile)
		setlinebuf(tracefile);
}


static struct debug_flag {
	char		label;
	uint64_t	flag;
} debug_flags[] = {
	{ 'a',	DBG_ARITH	},	/* arithmetic ( $(( )) ) */
	{ 'c',	DBG_CMDS	},	/* command searching, ... */
	{ 'e',	DBG_EVAL	},	/* evaluation of the parse tree */
	{ 'f',	DBG_REDIR	},	/* file descriptors & redirections */
	{ 'g',	DBG_MATCH	},	/* pattern matching (glob) */
	{ 'h',	DBG_HISTORY	},	/* history & cmd line editing */
	{ 'i',	DBG_INPUT	},	/* shell input routines */
	{ 'j',	DBG_JOBS	},	/* job control, structures */
	{ 'm',	DBG_MEM		},	/* memory management */
	{ 'o',	DBG_OUTPUT	},	/* output routines */
	{ 'p',	DBG_PROCS	},	/* process management, fork, ... */
	{ 'r',	DBG_PARSE	},	/* parser, lexer, ... tree building */
	{ 's',	DBG_SIG		},	/* signals and everything related */
	{ 't',	DBG_TRAP	},	/* traps & signals */
	{ 'v',	DBG_VARS	},	/* variables and parameters */
	{ 'w',	DBG_WAIT	},	/* waits for processes to finish */
	{ 'x',	DBG_EXPAND	},	/* word expansion ${} $() $(( )) */
	{ 'z',	DBG_ERRS	},	/* error control, jumps, cleanup */
 
	{ '0',	DBG_U0		},	/* ad-hoc temp debug flag #0 */
	{ '1',	DBG_U1		},	/* ad-hoc temp debug flag #1 */
	{ '2',	DBG_U2		},	/* ad-hoc temp debug flag #2 */
 
	{ '@',	DBG_LINE	},	/* prefix trace lines with line# */
	{ '$',	DBG_PID		},	/* prefix trace lines with sh pid */
	{ '^',	DBG_NEST	},	/* show shell nesting level */

			/* alpha options only */
	{ '_',	DBG_PARSE | DBG_EVAL | DBG_EXPAND | DBG_JOBS |
		    DBG_PROCS | DBG_REDIR | DBG_CMDS | DBG_ERRS |
		    DBG_WAIT | DBG_TRAP | DBG_VARS | DBG_MEM |
		    DBG_INPUT | DBG_OUTPUT | DBG_ARITH | DBG_HISTORY },

   /*   { '*',	DBG_ALLVERBOSE	}, 	   is handled in the code */

	{ '#',	DBG_U0 | DBG_U1 | DBG_U2 },

	{ 0,	0		}
};

void
set_debug(const char * flags, int on)
{
	char f;
	struct debug_flag *df;
	int verbose;

	while ((f = *flags++) != '\0') {
		verbose = 0;
		if (is_upper(f)) {
			verbose = 1;
			f += 'a' - 'A';
		}
		if (f == '*')
			f = '_', verbose = 1;
		if (f == '+') {
			if (*flags == '+')
				flags++, verbose=1;
		}

		/*
		 * Note: turning on any debug option also enables DBG_ALWAYS
		 * turning on any verbose option also enables DBG_VERBOSE
		 * Once enabled, those flags cannot be disabled.
		 * (tracing can still be turned off with "set +o debug")
		 */
		for (df = debug_flags; df->label != '\0'; df++) {
			if (f == '+' || df->label == f) {
				if (on) {
					DFlags |= DBG_ALWAYS | df->flag;
					if (verbose)
					    DFlags |= DBG_VERBOSE |
						(df->flag << DBG_VBOSE_SHIFT);
				} else {
					DFlags &= ~(df->flag<<DBG_VBOSE_SHIFT);
					if (!verbose)
						DFlags &= ~df->flag;
				}
			}
		}
	}
}


int
debugcmd(int argc, char **argv)
{
	if (argc == 1) {
		struct debug_flag *df;

		out1fmt("Debug: %sabled.  Flags: ", debug ? "en" : "dis");
		for (df = debug_flags; df->label != '\0'; df++) {
			if (df->flag & (df->flag - 1))
				continue;
			if (is_alpha(df->label) &&
			    (df->flag << DBG_VBOSE_SHIFT) & DFlags)
				out1c(df->label - ('a' - 'A'));
			else if (df->flag & DFlags)
				out1c(df->label);
		}
		out1c('\n');
		return 0;
	}

	while (*++argv) {
		if (**argv == '-')
			set_debug(*argv + 1, 1);
		else if (**argv == '+')
			set_debug(*argv + 1, 0);
		else
			return 1;
	}
	return 0;
}

#endif /* DEBUG */
