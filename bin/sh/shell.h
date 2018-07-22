/*	$NetBSD: shell.h,v 1.26 2018/07/22 20:38:06 kre Exp $	*/

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
 *	@(#)shell.h	8.2 (Berkeley) 5/4/95
 */

/*
 * The follow should be set to reflect the type of system you have:
 *	JOBS -> 1 if you have Berkeley job control, 0 otherwise.
 *	define BSD if you are running 4.2 BSD or later.
 *	define SYSV if you are running under System V.
 *	define DEBUG=1 to compile in debugging ('set -o debug' to turn on)
 *	define DEBUG=2 to compile in and enable debugging.
 *	define DEBUG=3 for DEBUG==2 + enable most standard debug output
 *	define DEBUG=4 for DEBUG==2 + enable absolutely everything
 *	define DO_SHAREDVFORK to indicate that vfork(2) shares its address
 *	       with its parent.
 *	define BOGUS_NOT_COMMAND to allow ! reserved words in weird places
 *		(traditional ash behaviour.)
 *
 * When debugging is on, debugging info will be written to ./trace and
 * a quit signal will generate a core dump.
 */

#ifndef SHELL_H
#define SHELL_H
#include <sys/param.h>

#define JOBS 1
#ifndef BSD
#define BSD 1
#endif

#ifndef DO_SHAREDVFORK
#if defined(__NetBSD_Version__) &&  __NetBSD_Version__ >= 104000000
#define DO_SHAREDVFORK
#endif
#endif

typedef void *pointer;
#ifndef NULL
#define NULL (void *)0
#endif
#ifndef STATIC
#define STATIC	/* empty */
#endif
#define MKINIT	/* empty */

#include <sys/cdefs.h>

extern const char nullstr[1];		/* null string */

#ifdef	SMALL
#undef	DEBUG
#endif

#ifdef DEBUG

extern	uint64_t	DFlags;
extern	int		ShNest;

/*
 * This is selected as there are 26 letters in ascii - not that that
 * matters for anything, just makes it easier to assign a different
 * command letter to each debug option.  We currently use only 18
 * so this could be reduced, but that is of no real benefit.  It can also
 * be increased, but that both limits the maximum value tha can be
 * used with DBG_EXTRAS(), and causes problems with verbose option naming.
 */
#define	DBG_VBOSE_SHIFT		26
#define	DBG_EXTRAS(n)	((DBG_VBOSE_SHIFT * 2) + (n))

/*
 * Unconditional tracing for compatibility with old tracing setup.
 */
#define TRACE(param)		do {					\
					trace param;			\
				} while (/*CONSTCOND*/ 0)
#define TRACEV(param)		do {					\
					tracev param;			\
				} while (/*CONSTCOND*/ 0)

/*
 * and the newer conditional tracing, so the mainainer can control
 * just how much debug output is dumped to the trace file
 * (once the rest of the shell is converted to use it).
 *
 * in the X forms, "xtra" can be any legal C statement(s) without (bare) commas
 * executed if the relevant debug flag is enabled, after any tracing output.
 */
#define CTRACE(when, param)	do {					\
				    if ((DFlags & (when)) != 0)		\
					trace param;			\
				} while (/*CONSTCOND*/ 0)

#define CCTRACE(when,cond,param) do {					\
				    if ((cond) && (DFlags & (when)) != 0) \
					trace param;			\
				} while (/*CONSTCOND*/ 0)

#define CTRACEV(when, param)	do {					\
				    if ((DFlags & (when)) != 0)		\
					tracev param;			\
				} while (/*CONSTCOND*/ 0)

#define XTRACE(when, param, xtra) do {					\
				    if ((DFlags & (when)) != 0) {	\
					trace param;			\
					xtra;				\
				    }					\
				} while (/*CONSTCOND*/ 0)

#define VTRACE(when, param)	do {					\
				    if ((DFlags &			\
					(when)<<DBG_VBOSE_SHIFT) != 0)	\
					    trace param;		\
				} while (/*CONSTCOND*/ 0)

#define CVTRACE(when,cond,param) do {					\
				    if ((cond) && (DFlags &		\
					(when)<<DBG_VBOSE_SHIFT) != 0)	\
					    trace param;		\
				} while (/*CONSTCOND*/ 0)

#define VTRACEV(when, param)	do {					\
				    if ((DFlags &			\
					(when)<<DBG_VBOSE_SHIFT) != 0)	\
					    tracev param;		\
				} while (/*CONSTCOND*/ 0)

#define VXTRACE(when, param, xtra) do {					\
				    if ((DFlags &			\
					(when)<<DBG_VBOSE_SHIFT) != 0) {\
					    trace param;		\
					    xtra;			\
				    }					\
				} while (/*CONSTCOND*/ 0)

#define SHELL_FORKED()	ShNest++
#define VFORK_BLOCK	{ const int _ShNest = ShNest;
#define VFORK_END	}
#define VFORK_UNDO()	ShNest = _ShNest

#define	DBG_ALWAYS	(1LL << 0)
#define	DBG_PARSE	(1LL << 1)		/* r (read commands) */
#define	DBG_EVAL	(1LL << 2)		/* e */
#define	DBG_EXPAND	(1LL << 3)		/* x */
#define	DBG_JOBS	(1LL << 4)		/* j */
#define	DBG_PROCS	(1LL << 5)		/* p */
#define	DBG_REDIR	(1LL << 6)		/* f (fds) */
#define	DBG_CMDS	(1LL << 7)		/* c */
#define	DBG_ERRS	(1LL << 8)		/* z (?) */
#define	DBG_WAIT	(1LL << 9)		/* w */
#define	DBG_TRAP	(1LL << 10)		/* t */
#define	DBG_VARS	(1LL << 11)		/* v */
#define	DBG_INPUT	(1LL << 12)		/* i */
#define	DBG_OUTPUT	(1LL << 13)		/* o */
#define	DBG_MEM		(1LL << 14)		/* m */
#define	DBG_ARITH	(1LL << 15)		/* a */
#define	DBG_HISTORY	(1LL << 16)		/* h */
#define	DBG_SIG		(1LL << 17)		/* s */
#define	DBG_MATCH	(1LL << 18)		/* g (glob) */

/*
 * reserved extras: b=builtins l=alias
 * still free:  d k n q u y
 */

	/* use VTRACE(DBG_ALWAYS, (...)) to test this one */
#define	DBG_VERBOSE	(1LL << DBG_VBOSE_SHIFT)

	/* DBG_EXTRAS 0 .. 11 (max) only  - non-alpha options (no VTRACE !!) */
#define	DBG_U0		(1LL << DBG_EXTRAS(0))	/* 0 - ad-hoc extra flags */
#define	DBG_U1		(1LL << DBG_EXTRAS(1))	/* 1 - for short term */
#define	DBG_U2		(1LL << DBG_EXTRAS(2))	/* 2 - extra tracing*/

#define	DBG_LINE	(1LL << DBG_EXTRAS(9))	/* @ ($LINENO) */
#define	DBG_PID		(1LL << DBG_EXTRAS(10))	/* $ ($$) */
#define	DBG_NEST	(1LL << DBG_EXTRAS(11))	/* ^ */

extern void set_debug(const char *, int);

#else	/* DEBUG */

#define TRACE(param)			/* historic normal trace */
#define TRACEV(param)			/* historic varargs trace */

#define CTRACE(when, param)		/* conditional normal trace */
#define CCTRACE(when, cond, param)	/* more conditional normal trace */
#define CTRACEV(when, param)		/* conditional varargs trace */
#define XTRACE(when, param, extra)	/* conditional trace, plus more */
#define VTRACE(when, param)		/* conditional verbose trace */
#define CVTRACE(when, cond, param)	/* more conditional verbose trace */
#define VTRACEV(when, param)		/* conditional verbose varargs trace */
#define VXTRACE(when, param, extra)	/* cond verbose trace, plus more */

#define SHELL_FORKED()
#define VFORK_BLOCK
#define VFORK_END
#define VFORK_UNDO()

#endif	/* DEBUG */

#endif /* SHELL_H */
