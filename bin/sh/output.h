/*	$NetBSD: output.h,v 1.27 2017/11/21 03:42:39 kre Exp $	*/

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
 *	@(#)output.h	8.2 (Berkeley) 5/4/95
 */

#ifndef OUTPUT_INCL

#include <stdarg.h>

struct output {
	char *nextc;
	int nleft;
	int bufsize;
	char *buf;
	short fd;
	short flags;
#ifndef SMALL
	struct output *chain;
#endif
};

/* flags for ->flags */
#define OUTPUT_ERR	0x01		/* error occurred on output */
#define OUTPUT_CLONE	0x02		/* this is a clone of another */

extern struct output output;
extern struct output errout;
extern struct output memout;
extern struct output *out1;
extern struct output *out2;
#ifdef SMALL
#define outx out2
#else
extern struct output *outx;
#endif

void open_mem(char *, int, struct output *);
void out1str(const char *);
void out2str(const char *);
void outstr(const char *, struct output *);
void out2shstr(const char *);
#ifdef SMALL
#define outxstr out2str
#define outxshstr out2shstr
#else
void outxstr(const char *);
void outxshstr(const char *);
#endif
void outshstr(const char *, struct output *);
void emptyoutbuf(struct output *);
void flushall(void);
void flushout(struct output *);
void freestdout(void);
void outfmt(struct output *, const char *, ...) __printflike(2, 3);
void out1fmt(const char *, ...) __printflike(1, 2);
#ifdef DEBUG
void debugprintf(const char *, ...) __printflike(1, 2);
#endif
void fmtstr(char *, size_t, const char *, ...) __printflike(3, 4);
void doformat(struct output *, const char *, va_list) __printflike(2, 0);
int xwrite(int, char *, int);
#ifdef SMALL
#define xtracefdsetup(x)	do { break; } while (0)
#define xtrace_clone(x)		do { break; } while (0)
#define xtrace_pop()		do { break; } while (0)
#else
void xtracefdsetup(int);
void xtrace_clone(int);
void xtrace_pop(void);
#endif

#define outc(c, file)	(--(file)->nleft < 0? (emptyoutbuf(file), *(file)->nextc++ = (c)) : (*(file)->nextc++ = (c)))
#define out1c(c)	outc(c, out1)
#define out2c(c)	outc(c, out2)
#define outxc(c)	outc(c, outx)

#define OUTPUT_INCL
#endif
