/*	$NetBSD: mtree.c,v 1.19 2001/10/04 04:51:27 lukem Exp $	*/

/*-
 * Copyright (c) 1989, 1990, 1993
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
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1989, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mtree.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: mtree.c,v 1.19 2001/10/04 04:51:27 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mtree.h"
#include "extern.h"

int	ftsoptions = FTS_PHYSICAL;
int	cflag, dflag, Dflag, eflag, iflag, lflag, mflag,
    	rflag, sflag, tflag, uflag, Uflag;
int	keys;
char	fullpath[MAXPATHLEN];
char	**excludetags, **includetags;

static	void	addtag(char ***, int *, char *);
	int	main(int, char **);
static	void	parsetags(char ***, int *, char *);
static	void	usage(void);

int
main(int argc, char **argv)
{
	int	ch, status, etagc, itagc;
	char	*dir, *p;

	setprogname(argv[0]);

	etagc = itagc = 0;
	dir = NULL;
	keys = KEYDEFAULT;
	while ((ch = getopt(argc, argv, "cdDeE:f:I:iK:k:lmp:rR:s:tUux")) != -1)
		switch((char)ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'D':
			Dflag = 1;
			break;
		case 'E':
			parsetags(&excludetags, &etagc, optarg);
			break;
		case 'e':
			eflag = 1;
			break;
		case 'f':
			if (!(freopen(optarg, "r", stdin)))
				mtree_err("%s: %s", optarg, strerror(errno));
			break;
		case 'I':
			parsetags(&includetags, &itagc, optarg);
			break;
		case 'i':
			iflag = 1;
			break;
		case 'K':
			while ((p = strsep(&optarg, " \t,")) != NULL)
				if (*p != '\0')
					keys |= parsekey(p, NULL);
			break;
		case 'k':
			keys = F_TYPE;
			while ((p = strsep(&optarg, " \t,")) != NULL)
				if (*p != '\0')
					keys |= parsekey(p, NULL);
			break;
		case 'l':
			lflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'p':
			dir = optarg;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'R':
			while ((p = strsep(&optarg, " \t,")) != NULL)
				if (*p != '\0')
					keys &= ~parsekey(p, NULL);
			break;
		case 's':
			sflag = 1;
			crc_total = ~strtol(optarg, &p, 0);
			if (*p)
				mtree_err("illegal seed value -- %s", optarg);
			break;
		case 't':
			tflag = 1;
			break;
		case 'U':
			Uflag = uflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'x':
			ftsoptions |= FTS_XDEV;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	addtag(&excludetags, &etagc, NULL);
	addtag(&includetags, &itagc, NULL);

	if (dir && chdir(dir))
		mtree_err("%s: %s", dir, strerror(errno));

	if ((cflag || sflag) && !getcwd(fullpath, MAXPATHLEN))
		mtree_err("%s", strerror(errno));

	if (cflag == 1 && Dflag == 1)
		mtree_err("-c and -D flags are mutually exclusive");

	if (iflag == 1 && mflag == 1)
		mtree_err("-i and -m flags are mutually exclusive");

	if (lflag == 1 && uflag == 1)
		mtree_err("-l and -u flags are mutually exclusive");

	if (cflag) {
		cwalk();
		exit(0);
	}
	if (Dflag) {
		dump_nodes("", spec());
		exit(0);
	}
	status = verify();
	if (Uflag & (status == MISMATCHEXIT))
		status = 0;
	exit(status);
}


static void
addtag(char ***list, int *count, char *elem)
{

#define	TAG_CHUNK 20

	if ((*count % TAG_CHUNK) == 0) {
		char **new;

		new = (char **)realloc(*list, (*count + TAG_CHUNK)
		    * sizeof(char *));
		if (new == NULL)
			mtree_err("memory allocation error");
		*list = new;
	}
	(*list)[*count] = elem;
	(*count)++;
}

static void
parsetags(char ***list, int *count, char *args)
{
	char	*p, *e;
	int	len;

	if (args == NULL) {
		addtag(list, count, NULL);
		return;
	}
	while ((p = strsep(&args, ",")) != NULL) {
		if (*p == '\0')
			continue;
		len = strlen(p) + 3;	/* "," + p + ",\0" */
		if ((e = malloc(len)) == NULL)
			mtree_err("memory allocation error");
		snprintf(e, len, ",%s,", p);
		addtag(list, count, e);
	}
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: mtree [-cdDelrUux] [-i|-m] [-f spec]"
	    " [-k key] [-K addkey] [-R removekey]\n"
	    "\t\t[-I inctags] [-E exctags] [-p path] [-s seed]\n");
	exit(1);
}
