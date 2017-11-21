/*	$NetBSD: output.c,v 1.40 2017/11/21 03:42:39 kre Exp $	*/

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
static char sccsid[] = "@(#)output.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: output.c,v 1.40 2017/11/21 03:42:39 kre Exp $");
#endif
#endif /* not lint */

/*
 * Shell output routines.  We use our own output routines because:
 *	When a builtin command is interrupted we have to discard
 *		any pending output.
 *	When a builtin command appears in back quotes, we want to
 *		save the output of the command in a region obtained
 *		via malloc, rather than doing a fork and reading the
 *		output of the command via a pipe.
 *	Our output routines may be smaller than the stdio routines.
 */

#include <sys/types.h>		/* quad_t */
#include <sys/param.h>		/* BSD4_4 */
#include <sys/ioctl.h>

#include <stdio.h>	/* defines BUFSIZ */
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "redir.h"
#include "options.h"
#include "show.h"


#define OUTBUFSIZ BUFSIZ
#define BLOCK_OUT -2		/* output to a fixed block of memory */
#define MEM_OUT -3		/* output to dynamically allocated memory */

#ifdef SMALL
#define	CHAIN
#else
#define	CHAIN	,NULL
#endif


		/*      nextc  nleft  bufsize  buf     fd  flags  chain */
struct output output = {NULL,    0, OUTBUFSIZ, NULL,    1,    0   CHAIN };
struct output errout = {NULL,    0,       100, NULL,    2,    0   CHAIN };
struct output memout = {NULL,    0,         0, NULL, MEM_OUT, 0   CHAIN };
struct output *out1 = &output;
struct output *out2 = &errout;
#ifndef SMALL
struct output *outx = &errout;
struct output *outxtop = NULL;
#endif


#ifdef mkinit

INCLUDE "output.h"
INCLUDE "memalloc.h"

RESET {
	out1 = &output;
	out2 = &errout;
	if (memout.buf != NULL) {
		ckfree(memout.buf);
		memout.buf = NULL;
	}
}

#endif


#ifdef notdef	/* no longer used */
/*
 * Set up an output file to write to memory rather than a file.
 */

void
open_mem(char *block, int length, struct output *file)
{
	file->nextc = block;
	file->nleft = --length;
	file->fd = BLOCK_OUT;
	file->flags = 0;
}
#endif


void
out1str(const char *p)
{
	outstr(p, out1);
}


void
out2str(const char *p)
{
	outstr(p, out2);
}

#ifndef SMALL
void
outxstr(const char *p)
{
	outstr(p, outx);
}
#endif


void
outstr(const char *p, struct output *file)
{
	char c = 0;

	while (*p)
		outc((c = *p++), file);
	if (file == out2 || (file == outx && c == '\n'))
		flushout(file);
}


void
out2shstr(const char *p)
{
	outshstr(p, out2);
}

#ifndef SMALL
void
outxshstr(const char *p)
{
	outshstr(p, outx);
}
#endif

/*
 * ' is in this list, not because it does not require quoting
 * (which applies to all the others) but because '' quoting cannot
 * be used to quote it.
 */
static const char norm_chars [] = \
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/+-=_,.'";

static int
inquote(const char *p)
{
	size_t l = strspn(p, norm_chars);
	char *s = strchr(p, '\'');

	return s == NULL ? p[l] != '\0' : s - p > (off_t)l;
}

void
outshstr(const char *p, struct output *file)
{
	int need_q;
	int inq;
	char c;

	if (strchr(p, '\'') != NULL && p[1] != '\0') {
		/*
		 * string contains ' in it, and is not only the '
		 * see if " quoting will work
		 */
		size_t i = strcspn(p, "\\\"$`");

		if (p[i] == '\0') {
			/*
			 * string contains no $ ` \ or " chars, perfect...
			 *
			 * We know it contains ' so needs quoting, so
			 * this is easy...
			 */
			outc('"', file);
			outstr(p, file);
			outc('"', file);
			return;
		}
	}

	need_q = p[0] == 0 || p[strspn(p, norm_chars)] != 0;

	/*
	 * Don't emit ' unless something needs quoting before closing '
	 */
	if (need_q && (p[0] == 0 || inquote(p) != 0)) {
		outc('\'', file);
		inq = 1;
	} else
		inq = 0;

	while ((c = *p++) != '\0') {
		if (c != '\'') {
			outc(c, file);
			continue;
		}

		if (inq)
			outc('\'', file);	/* inq = 0, implicit */
		outc('\\', file);
		outc(c, file);
		if (need_q && *p != '\0') {
			if ((inq = inquote(p)) != 0)
				outc('\'', file);
		} else
			inq = 0;
	}

	if (inq)
		outc('\'', file);

	if (file == out2)
		flushout(file);
}


char out_junk[16];


void
emptyoutbuf(struct output *dest)
{
	int offset;

	if (dest->fd == BLOCK_OUT) {
		dest->nextc = out_junk;
		dest->nleft = sizeof out_junk;
		dest->flags |= OUTPUT_ERR;
	} else if (dest->buf == NULL) {
		INTOFF;
		dest->buf = ckmalloc(dest->bufsize);
		dest->nextc = dest->buf;
		dest->nleft = dest->bufsize;
		INTON;
		VTRACE(DBG_OUTPUT, ("emptyoutbuf now %d @%p for fd %d\n",
		    dest->nleft, dest->buf, dest->fd));
	} else if (dest->fd == MEM_OUT) {
		offset = dest->bufsize;
		INTOFF;
		dest->bufsize <<= 1;
		dest->buf = ckrealloc(dest->buf, dest->bufsize);
		dest->nleft = dest->bufsize - offset;
		dest->nextc = dest->buf + offset;
		INTON;
	} else {
		flushout(dest);
	}
	dest->nleft--;
}


void
flushall(void)
{
	flushout(&output);
	flushout(&errout);
}


void
flushout(struct output *dest)
{

	if (dest->buf == NULL || dest->nextc == dest->buf || dest->fd < 0)
		return;
	VTRACE(DBG_OUTPUT, ("flushout fd=%d %zd to write\n", dest->fd,
	    (size_t)(dest->nextc - dest->buf)));
	if (xwrite(dest->fd, dest->buf, dest->nextc - dest->buf) < 0)
		dest->flags |= OUTPUT_ERR;
	dest->nextc = dest->buf;
	dest->nleft = dest->bufsize;
}


void
freestdout(void)
{
	INTOFF;
	if (output.buf) {
		ckfree(output.buf);
		output.buf = NULL;
		output.nleft = 0;
	}
	INTON;
}


void
outfmt(struct output *file, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	doformat(file, fmt, ap);
	va_end(ap);
}


void
out1fmt(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	doformat(out1, fmt, ap);
	va_end(ap);
}

#ifdef DEBUG
void
debugprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	doformat(out2, fmt, ap);
	va_end(ap);
	flushout(out2);
}
#endif

void
fmtstr(char *outbuf, size_t length, const char *fmt, ...)
{
	va_list ap;
	struct output strout;

	va_start(ap, fmt);
	strout.nextc = outbuf;
	strout.nleft = length;
	strout.fd = BLOCK_OUT;
	strout.flags = 0;
	doformat(&strout, fmt, ap);
	outc('\0', &strout);
	if (strout.flags & OUTPUT_ERR)
		outbuf[length - 1] = '\0';
	va_end(ap);
}

/*
 * Formatted output.  This routine handles a subset of the printf formats:
 * - Formats supported: d, u, o, p, X, s, and c.
 * - The x format is also accepted but is treated like X.
 * - The l, ll and q modifiers are accepted.
 * - The - and # flags are accepted; # only works with the o format.
 * - Width and precision may be specified with any format except c.
 * - An * may be given for the width or precision.
 * - The obsolete practice of preceding the width with a zero to get
 *   zero padding is not supported; use the precision field.
 * - A % may be printed by writing %% in the format string.
 */

#define TEMPSIZE 24

#ifdef BSD4_4
#define HAVE_VASPRINTF 1
#endif

void
doformat(struct output *dest, const char *f, va_list ap)
{
#if	HAVE_VASPRINTF
	char *s;

	vasprintf(&s, f, ap);
	if (s == NULL)
		error("Could not allocate formatted output buffer");
	outstr(s, dest);
	free(s);     
#else	/* !HAVE_VASPRINTF */
	static const char digit[] = "0123456789ABCDEF";
	char c;
	char temp[TEMPSIZE];
	int flushleft;
	int sharp;
	int width;
	int prec;
	int islong;
	int isquad;
	char *p;
	int sign;
#ifdef BSD4_4
	quad_t l;
	u_quad_t num;
#else
	long l;
	u_long num;
#endif
	unsigned base;
	int len;
	int size;
	int pad;

	while ((c = *f++) != '\0') {
		if (c != '%') {
			outc(c, dest);
			continue;
		}
		flushleft = 0;
		sharp = 0;
		width = 0;
		prec = -1;
		islong = 0;
		isquad = 0;
		for (;;) {
			if (*f == '-')
				flushleft++;
			else if (*f == '#')
				sharp++;
			else
				break;
			f++;
		}
		if (*f == '*') {
			width = va_arg(ap, int);
			f++;
		} else {
			while (is_digit(*f)) {
				width = 10 * width + digit_val(*f++);
			}
		}
		if (*f == '.') {
			if (*++f == '*') {
				prec = va_arg(ap, int);
				f++;
			} else {
				prec = 0;
				while (is_digit(*f)) {
					prec = 10 * prec + digit_val(*f++);
				}
			}
		}
		if (*f == 'l') {
			f++;
			if (*f == 'l') {
				isquad++;
				f++;
			} else
				islong++;
		} else if (*f == 'q') {
			isquad++;
			f++;
		}
		switch (*f) {
		case 'd':
#ifdef BSD4_4
			if (isquad)
				l = va_arg(ap, quad_t);
			else
#endif
			if (islong)
				l = va_arg(ap, long);
			else
				l = va_arg(ap, int);
			sign = 0;
			num = l;
			if (l < 0) {
				num = -l;
				sign = 1;
			}
			base = 10;
			goto number;
		case 'u':
			base = 10;
			goto uns_number;
		case 'o':
			base = 8;
			goto uns_number;
		case 'p':
			outc('0', dest);
			outc('x', dest);
			/*FALLTHROUGH*/
		case 'x':
			/* we don't implement 'x'; treat like 'X' */
		case 'X':
			base = 16;
uns_number:	  /* an unsigned number */
			sign = 0;
#ifdef BSD4_4
			if (isquad)
				num = va_arg(ap, u_quad_t);
			else
#endif
			if (islong)
				num = va_arg(ap, unsigned long);
			else
				num = va_arg(ap, unsigned int);
number:		  /* process a number */
			p = temp + TEMPSIZE - 1;
			*p = '\0';
			while (num) {
				*--p = digit[num % base];
				num /= base;
			}
			len = (temp + TEMPSIZE - 1) - p;
			if (prec < 0)
				prec = 1;
			if (sharp && *f == 'o' && prec <= len)
				prec = len + 1;
			pad = 0;
			if (width) {
				size = len;
				if (size < prec)
					size = prec;
				size += sign;
				pad = width - size;
				if (flushleft == 0) {
					while (--pad >= 0)
						outc(' ', dest);
				}
			}
			if (sign)
				outc('-', dest);
			prec -= len;
			while (--prec >= 0)
				outc('0', dest);
			while (*p)
				outc(*p++, dest);
			while (--pad >= 0)
				outc(' ', dest);
			break;
		case 's':
			p = va_arg(ap, char *);
			pad = 0;
			if (width) {
				len = strlen(p);
				if (prec >= 0 && len > prec)
					len = prec;
				pad = width - len;
				if (flushleft == 0) {
					while (--pad >= 0)
						outc(' ', dest);
				}
			}
			prec++;
			while (--prec != 0 && *p)
				outc(*p++, dest);
			while (--pad >= 0)
				outc(' ', dest);
			break;
		case 'c':
			c = va_arg(ap, int);
			outc(c, dest);
			break;
		default:
			outc(*f, dest);
			break;
		}
		f++;
	}
#endif	/* !HAVE_VASPRINTF */
}



/*
 * Version of write which resumes after a signal is caught.
 */

int
xwrite(int fd, char *buf, int nbytes)
{
	int ntry;
	int i;
	int n;

	n = nbytes;
	ntry = 0;
	while (n > 0) {
		i = write(fd, buf, n);
		if (i > 0) {
			if ((n -= i) <= 0)
				return nbytes;
			buf += i;
			ntry = 0;
		} else if (i == 0) {
			if (++ntry > 10)
				return nbytes - n;
		} else if (errno != EINTR) {
			return -1;
		}
	}
	return nbytes;
}

#ifndef SMALL
static void
xtrace_fd_swap(int from, int to)
{
	struct output *o = outxtop;

	while (o != NULL) {
		if (o->fd == from)
			o->fd = to;
		o = o->chain;
	}
}

/*
 * the -X flag is to be set or reset (not necessarily changed)
 * Do what is needed to make tracing go to where it should
 *
 * Note: Xflag has not yet been altered, "on" indicates what it will become
 */

void
xtracefdsetup(int on)
{
	if (!on) {
		flushout(outx);

		if (Xflag != 1)		/* Was already +X */
			return;		/* so nothing to do */

		outx = out2;
		CTRACE(DBG_OUTPUT, ("Tracing to stderr\n"));
		return;
	}

	if (Xflag == 1) {				/* was already set */
		/*
		 * This is a change of output file only
		 * Just close the current one, and reuse the struct output
		 */
		if (!(outx->flags & OUTPUT_CLONE))
			sh_close(outx->fd);
	} else if (outxtop == NULL) {
		/*
		 * -X is just turning on, for the forst time,
		 * need a new output struct to become outx
		 */
		xtrace_clone(1);
	} else
		outx = outxtop;

	if (outx != out2) {
		outx->flags &= ~OUTPUT_CLONE;
		outx->fd = to_upper_fd(dup(out2->fd));
		register_sh_fd(outx->fd, xtrace_fd_swap);
	}

	CTRACE(DBG_OUTPUT, ("Tracing now to fd %d (from %d)\n",
	    outx->fd, out2->fd));
}

void
xtrace_clone(int new)
{
	struct output *o;

	CTRACE(DBG_OUTPUT,
	    ("xtrace_clone(%d): %sfd=%d buf=%p nleft=%d flags=%x ",
	    new, (outx == out2 ? "out2: " : ""),
	    outx->fd, outx->buf, outx->nleft, outx->flags));

	flushout(outx);

	if (!new && outxtop == NULL && Xflag == 0) {
		/* following stderr, nothing to save */
		CTRACE(DBG_OUTPUT, ("+X\n"));
		return;
	}

	o = ckmalloc(sizeof(*o));
	o->fd = outx->fd;
	o->flags = OUTPUT_CLONE;
	o->bufsize = outx->bufsize;
	o->nleft = 0;
	o->buf = NULL;
	o->nextc = NULL;
	o->chain = outxtop;
	outx = o;
	outxtop = o;

	CTRACE(DBG_OUTPUT, ("-> fd=%d flags=%x[CLONE]\n", outx->fd, o->flags));
}

void
xtrace_pop(void)
{
	struct output *o;

	CTRACE(DBG_OUTPUT, ("trace_pop: fd=%d buf=%p nleft=%d flags=%x ",
	    outx->fd, outx->buf, outx->nleft, outx->flags));

	flushout(outx);

	if (outxtop == NULL) {
		/*
		 * No -X has been used, so nothing much to do
		 */
		CTRACE(DBG_OUTPUT, ("+X\n"));
		return;
	}

	o = outxtop;
	outx = o->chain;
	if (outx == NULL)
		outx = &errout;
	outxtop = o->chain;
	if (o != &errout) {
		if (o->fd >= 0 && !(o->flags & OUTPUT_CLONE))
			sh_close(o->fd);
		if (o->buf)
			ckfree(o->buf);
		ckfree(o);
	}

	CTRACE(DBG_OUTPUT, ("-> fd=%d buf=%p nleft=%d flags=%x\n",
	    outx->fd, outx->buf, outx->nleft, outx->flags));
}
#endif /* SMALL */
