/*	$NetBSD: panic.c,v 1.12 2007/07/18 01:06:08 lukem Exp $	*/

/*
 * panic.c - terminate fast in case of error
 * Copyright (c) 1993 by Thomas Koenig
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* System Headers */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Local headers */

#include "panic.h"
#include "at.h"
#include "privs.h"

/* File scope variables */

#ifndef lint
#if 0
static char rcsid[] = "$OpenBSD: panic.c,v 1.4 1997/03/01 23:40:09 millert Exp $";
#else
__RCSID("$NetBSD: panic.c,v 1.12 2007/07/18 01:06:08 lukem Exp $");
#endif
#endif

/* External variables */

/* Global functions */

void
panic(char *a)
{

	/*
	 * Something fatal has happened, print error message and exit.
	 */
	if (fcreated) {
		PRIV_START
		(void)unlink(atfile);
		PRIV_END
	}

	errx(EXIT_FAILURE, "%s", a);
}

void
perr(char *a)
{

	/*
	 * Some operating system error; print error message and exit.
	 */
	perror(a);
	if (fcreated) {
		PRIV_START
		(void)unlink(atfile);
		PRIV_END
	}

	exit(EXIT_FAILURE);
}

void 
perr2(char *a, char *b)
{

	(void)fputs(a, stderr);
	perr(b);
}

void
usage(void)
{

	/* Print usage and exit.  */
	(void)fprintf(stderr,   "usage: at [-bdlmrVv] [-f file] [-q queue] -t [[CC]YY]MMDDhhmm[.SS]\n"
				"       at [-bdlmrVv] [-f file] [-q queue] time\n"
				"       at [-V] -c job [job ...]\n"
				"       atq [-Vv] [-q queue]\n"
				"       atrm [-V] job [job ...]\n"
				"       batch [-mVv] [-f file] [-q queue] [-t [[CC]YY]MMDDhhmm[.SS]]\n"
				"       batch [-mVv] [-f file] [-q queue] [time]\n");
	exit(EXIT_FAILURE);
}
