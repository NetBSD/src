/*	$NetBSD: assert.c,v 1.10 2000/12/19 14:32:59 kleink Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)assert.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: assert.c,v 1.10 2000/12/19 14:32:59 kleink Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

extern char *__progname;

void
__assert13(file, line, function, failedexpr)
	const char *file, *function, *failedexpr;
	int line;
{
	(void)fprintf(stderr,
	    "assertion \"%s\" failed: file \"%s\", line %d%s%s%s\n",
	    failedexpr, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");
	abort();
	/* NOTREACHED */
}

void
__assert(file, line, failedexpr)
	const char *file, *failedexpr;
	int line;
{
	(void)fprintf(stderr,
	    "assertion \"%s\" failed: file \"%s\", line %d\n",
	    failedexpr, file, line);
	abort();
	/* NOTREACHED */
}

void
__diagassert13(file, line, function, failedexpr)
	const char *file, *function, *failedexpr;
	int line;
{
		/*
		 * XXX: check $DIAGASSERT here, and do user-defined actions
		 */
	(void)fprintf(stderr,
	 "%s: assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
	     __progname, failedexpr, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");
	syslog(LOG_DEBUG|LOG_USER,
	    "assertion \"%s\" failed: file \"%s\", line %d%s%s%s",
	    failedexpr, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");
	return;
}

void
__diagassert(file, line, failedexpr)
	const char *file, *failedexpr;
	int line;
{
		/*
		 * XXX: check $DIAGASSERT here, and do user-defined actions
		 */
	(void)fprintf(stderr,
	    "%s: assertion \"%s\" failed: file \"%s\", line %d\n",
	    __progname, failedexpr, file, line);
	syslog(LOG_DEBUG|LOG_USER,
	    "assertion \"%s\" failed: file \"%s\", line %d",
	    failedexpr, file, line);
	return;
}
