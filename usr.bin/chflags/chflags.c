/*	$NetBSD: chflags.c,v 1.10 2003/08/07 11:13:17 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)chflags.c	8.5 (Berkeley) 4/1/94";
#else
__RCSID("$NetBSD: chflags.c,v 1.10 2003/08/07 11:13:17 agc Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stat_flags.h"

int	main __P((int, char **));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FTS *ftsp;
	FTSENT *p;
	u_long clear, set, newflags;
	long val;
	int Hflag, Lflag, Rflag, ch, fts_options, hflag, oct, rval;
	char *flags, *ep;
	int (*change_flags) __P((const char *, u_long));

	Hflag = Lflag = Rflag = hflag = 0;
	while ((ch = getopt(argc, argv, "HLPRh")) != -1)
		switch (ch) {
		case 'H':
			Hflag = 1;
			Lflag = 0;
			break;
		case 'L':
			Lflag = 1;
			Hflag = 0;
			break;
		case 'P':
			Hflag = Lflag = 0;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc < 2)
		usage();

	fts_options = FTS_PHYSICAL;
	if (Rflag) {
		if (Hflag)
			fts_options |= FTS_COMFOLLOW;
		if (Lflag) {
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;
		}
	} else if (!hflag)
		fts_options |= FTS_COMFOLLOW;

	flags = *argv;
	if (*flags >= '0' && *flags <= '7') {
		errno = 0;
		val = strtol(flags, &ep, 8);
		if (val < 0)
			errno = ERANGE;
		if (errno)
			err(1, "invalid flags: %s", flags);
		if (*ep)
			errx(1, "invalid flags: %s", flags);
		set = val;
		oct = 1;
	} else {
		if (string_to_flags(&flags, &set, &clear))
			errx(1, "invalid flag: %s", flags);
		clear = ~clear;
		oct = 0;
	}

	if ((ftsp = fts_open(++argv, fts_options, NULL)) == NULL)
		err(1, "fts_open");

	for (rval = 0; (p = fts_read(ftsp)) != NULL;) {
		change_flags = chflags;
		switch (p->fts_info) {
		case FTS_D:
			if (Rflag)		/* Change it at FTS_DP. */
				continue;
			fts_set(ftsp, p, FTS_SKIP);
			break;
		case FTS_DNR:			/* Warn, chflag, continue. */
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			rval = 1;
			break;
		case FTS_ERR:			/* Warn, continue. */
		case FTS_NS:
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			rval = 1;
			continue;
		case FTS_SL:			/* Ignore unless -h. */
			/*
			 * All symlinks we found while doing a physical
			 * walk end up here.
			 */
			if (!hflag)
				continue;
			/*
			 * Note that if we follow a symlink, fts_info is
			 * not FTS_SL but FTS_F or whatever.  And we should
			 * use lchflags only for FTS_SL and should use chflags
			 * for others.
			 */
			change_flags = lchflags;
			break;
		case FTS_SLNONE:		/* Ignore. */
			/*
			 * The only symlinks that end up here are ones that
			 * don't point to anything.  Note that if we are
			 * doing a phisycal walk, we never reach here unless
			 * we asked to follow explicitly.
			 */
			continue;
		default:
			break;
		}
		if (oct)
			newflags = set;
		else {
			newflags = p->fts_statp->st_flags;
			newflags |= set;
			newflags &= clear;
		}
		if ((*change_flags)(p->fts_accpath, newflags)) {
			warn("%s", p->fts_path);
			rval = 1;
		}
	}
	if (errno)
		err(1, "fts_read");
	exit(rval);
}

void
usage()
{

	(void)fprintf(stderr,
	    "usage: chflags [-R [-H | -L | -P]] [-h] flags file ...\n");
	exit(1);
}
