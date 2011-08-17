/*	$NetBSD: main.c,v 1.21 2011/08/17 13:11:22 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: main.c,v 1.21 2011/08/17 13:11:22 christos Exp $");
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "error.h"
#include "pathnames.h"

FILE *errorfile;     /* where error file comes from */
FILE *queryfile;     /* input for the query responses from the user */

int nignored;
char **names_ignored;

size_t filelevel = 0;
int nerrors = 0;
Eptr er_head;
static Eptr *errors;

int nfiles = 0;
Eptr **files;		/* array of pointers into errors*/
boolean *touchedfiles;  /* which files we touched */
int language = INCC;

char default_currentfilename[] = "????";
char *currentfilename = default_currentfilename;

boolean query = false;	/* query the operator if touch files */
boolean terse = false;	/* Terse output */

static char im_on[] = _PATH_TTY;	/* my tty name */
static boolean notouch = false;		/* don't touch ANY files */

const char *suffixlist = ".*";	/* initially, can touch any file */

static int errorsort(const void *, const void *);
static void forkvi(int, char **);
static void try(const char *, int, char **);
static void usage(void) __attribute__((__noreturn__));

/*
 * error [-nqSsTv] [-I <ignorename>] [-t <suffixlist>] [-p <level>] <infile>
 *
 *	-I:	the following name, `ignorename' contains a list of
 *		function names that are not to be treated as hard errors.
 *		Default: ~/.errorsrc
 *
 *	-n:	don't touch ANY files!
 *
 *	-p:	take the next argument as the number of levels to skip
 *		from the filename, like perl.
 *
 *	-q:	The user is to be queried before touching each
 *		file; if not specified, all files with hard, non
 *		ignorable errors are touched (assuming they can be).
 *
 *	-S:	show the errors in unsorted order
 *		(as they come from the error file)
 *
 *	-s:	print a summary of the error's categories.
 *
 *	-T:	terse output
 *
 *	-t:	touch only files ending with the list of suffixes, each
 *		suffix preceded by a dot.
 *		eg, -t .c.y.l
 *		will touch only files ending with .c, .y or .l
 *
 *	-v:	after touching all files, overlay vi(1), ex(1) or ed(1)
 *		on top of error, entered in the first file with
 *		an error in it, with the appropriate editor
 *		set up to use the "next" command to get the other
 *		files containing errors.
 *
 *	infile:	The error messages come from this file.
 *		Default: stdin
 */
int
main(int argc, char **argv)
{
	int c;
	char *ignorename = 0;
	int ed_argc;
	char **ed_argv;		/* return from touchfiles */
	boolean show_errors = false;
	boolean Show_Errors = false;
	boolean pr_summary = false;
	boolean edit_files = false;

	setprogname(argv[0]);

	errorfile = stdin;
	while ((c = getopt(argc, argv, "I:np:qSsTt:v")) != -1)
		switch (c) {
		case 'I':	/*ignore file name*/
			ignorename = optarg;
			break;
		case 'n':
			notouch = true;
			break;
		case 'p':
			filelevel = (size_t)strtol(optarg, NULL, 0);
			break;
		case 'q':
			query = true;
			break;
		case 'S':
			Show_Errors = true;
			break;
		case 's':
			pr_summary = true;
			break;
		case 'T':
			terse = true;
			break;
		case 't':
			suffixlist = optarg;
			break;
		case 'v':
			edit_files = true;
			break;
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	switch (argc) {
	case 0:
		break;
	case 1:
		if ((errorfile = fopen(argv[0], "r")) == NULL)
			err(1, "Cannot open `%s' to read errors", argv[0]);
		break;
	default:
		usage();
	}

	if (notouch)
		suffixlist = 0;


	if ((queryfile = fopen(im_on, "r")) == NULL) {
		if (query)
			err(1, "Cannot open `%s' to query the user", im_on);
	}
	if (signal(SIGINT, onintr) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGTERM, onintr) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	getignored(ignorename);
	eaterrors(&nerrors, &errors);
	if (Show_Errors)
		printerrors(true, nerrors, errors);
	qsort(errors, nerrors, sizeof(Eptr), errorsort);
	if (show_errors)
		printerrors(false, nerrors, errors);
	findfiles(nerrors, errors, &nfiles, &files);
#define P(msg, arg) fprintf(stdout, msg, arg)
	if (pr_summary) {
		if (nunknown)
			P("%d Errors are unclassifiable.\n", nunknown);
		if (nignore)
			P("%d Errors are classifiable, but totally "
			    "discarded.\n", nignore);
		if (nsyncerrors)
			P("%d Errors are synchronization errors.\n",
			    nsyncerrors);
		if (nignore)
			P("%d Errors are discarded because they refer to "
			    "sacrosinct files.\n", ndiscard);
		if (nnulled)
			P("%d Errors are nulled because they refer to specific "
			    "functions.\n", nnulled);
		if (nnonspec)
			P("%d Errors are not specific to any file.\n",
			    nnonspec);
		if (nthisfile)
			P("%d Errors are specific to a given file, but not "
			    "to a line.\n", nthisfile);
		if (ntrue)
			P("%d Errors are true errors, and can be inserted "
			    "into the files.\n", ntrue);
	}
	filenames(nfiles, files);
	fflush(stdout);
	if (touchfiles(nfiles, files, &ed_argc, &ed_argv) && edit_files)
		forkvi(ed_argc, ed_argv);
	return 0;
}

static void
forkvi(int argc, char **argv)
{
	if (query) {
		switch (inquire(terse
		    ? "Edit? "
		    : "Do you still want to edit the files you touched? ")) {
		case Q_error:
		case Q_NO:
		case Q_no:
			return;
		default:
			break;
		}
	}
	/*
	 *	ed_agument's first argument is
	 *	a vi/ex compatible search argument
	 *	to find the first occurrence of ###
	 */
	try("vi", argc, argv);
	try("ex", argc, argv);
	try("ed", argc-1, argv+1);
	fprintf(stdout, "Can't find any editors.\n");
}

static void
try(const char *name, int argc, char **argv)
{
	argv[0] = __UNCONST(name);
	wordvprint(stdout, argc, argv);
	fprintf(stdout, "\n");
	fflush(stderr);
	fflush(stdout);
	sleep(2);
	if (freopen(im_on, "r", stdin) == NULL)
		return;
	if (freopen(im_on, "w", stdout) == NULL)
		return;
	execvp(name, argv);
}

static int
errorsort(const void *x1, const void *x2)
{
	const Eptr *epp1 = x1;
	const Eptr *epp2 = x2;
	Eptr ep1, ep2;
	int order;

	/*
	 * Sort by:
	 *	1) synchronization, non specific, discarded errors first;
	 *	2) nulled and true errors last
	 *	   a) grouped by similar file names
	 *	       1) grouped in ascending line number
	 */
	ep1 = *epp1; ep2 = *epp2;
	if (ep1 == 0 || ep2 == 0)
		return 0;
	if (NOTSORTABLE(ep1->error_e_class) ^ NOTSORTABLE(ep2->error_e_class)) {
		return NOTSORTABLE(ep1->error_e_class) ? -1 : 1;
	}
	if (NOTSORTABLE(ep1->error_e_class))	/* then both are */
		return ep1->error_no - ep2->error_no;
	order = strcmp(ep1->error_text[0], ep2->error_text[0]);
	if (order == 0) {
		return ep1->error_line - ep2->error_line;
	}
	return order;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-nqSsTv] [-I ignorefile] "
	    "[-p filelevel] [-t suffixlist] [name]\n", getprogname());
	exit(1);
}
