/*	$NetBSD: main.c,v 1.31 2015/02/28 21:56:53 asau Exp $	*/

/*-
 * Copyright (c) 2013 Johann 'Myrkraverk' Oskarsson.
 * Copyright (c) 1992 Diomidis Spinellis.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis of Imperial College, University of London.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: main.c,v 1.31 2015/02/28 21:56:53 asau Exp $");
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/usr.bin/sed/main.c 252231 2013-06-26 04:14:19Z pfg $");
#endif

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993\
	The Regents of the University of California.  All rights reserved.");
#endif

#if 0
static const char sccsid[] = "@(#)main.c	8.2 (Berkeley) 1/3/94";
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <regex.h>
#include <stddef.h>
#define _WITH_GETLINE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "extern.h"

/*
 * Linked list pointer to compilation units and pointer to current
 * next pointer.
 */
struct s_compunit *script;
static struct s_compunit **cu_nextp = &script;

/*
 * Linked list pointer to files and pointer to current
 * next pointer.
 */
struct s_flist *files;
static struct s_flist **fl_nextp = &files;

FILE *infile;			/* Current input file */
FILE *outfile;			/* Current output file */

int aflag;
static int eflag;
int nflag;
int rflags = 0;

int ispan;			/* Whether inplace editing spans across files */
const char *inplace;		/* Inplace edit file extension. */

static void add_compunit(enum e_cut, char *);
static void add_file(char *);
static void usage(void) __dead;

int
main(int argc, char *argv[])
{
	int rval;		/* Exit status */
	int c, fflag;
	char *temp_arg;

	setprogname(argv[0]);
	(void) setlocale(LC_ALL, "");

	fflag = 0;
	inplace = NULL;

	while ((c = getopt(argc, argv, "EI::ae:f:i::lnru")) != -1)
		switch (c) {
		case 'r':		/* Gnu sed compat */
		case 'E':
			rflags = REG_EXTENDED;
			break;
		case 'I':
			inplace = optarg ? optarg : __UNCONST("");
			ispan = 1;	/* span across input files */
			break;
		case 'a':
			aflag = 1;
			break;
		case 'e':
			eflag = 1;
			temp_arg = xmalloc(strlen(optarg) + 2);
			strcpy(temp_arg, optarg);
			strcat(temp_arg, "\n");
			add_compunit(CU_STRING, temp_arg);
			break;
		case 'f':
			fflag = 1;
			add_compunit(CU_FILE, optarg);
			break;
		case 'i':
			inplace = optarg ? optarg : __UNCONST("");
			ispan = 0;	/* don't span across input files */
			break;
		case 'l':
#ifdef _IOLBF
			c = setvbuf(stdout, NULL, _IOLBF, 0);
#else
			c = setlinebuf(stdout);
#endif
			if (c)
				warn("setting line buffered output failed");
			break;
		case 'n':
			nflag = 1;
			break;
		case 'u':
#ifdef _IONBF
			c = setvbuf(stdout, NULL, _IONBF, 0);
#else
			c = -1;
			errno = EOPNOTSUPP;
#endif
			if (c)
				warn("setting unbuffered output failed");
			break;
		default:
		case '?':
			usage();
		}
	argc -= optind;
	argv += optind;

	/* First usage case; script is the first arg */
	if (!eflag && !fflag && *argv) {
		add_compunit(CU_STRING, *argv);
		argv++;
	}

	compile();

	/* Continue with first and start second usage */
	if (*argv)
		for (; *argv; argv++)
			add_file(*argv);
	else
		add_file(NULL);
	rval = process();
	cfclose(prog, NULL);
	if (fclose(stdout))
		err(1, "stdout");
	exit(rval);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage:  %s [-aElnru] command [file ...]\n"
	    "\t%s [-aElnru] [-e command] [-f command_file] [-I[extension]]\n"
	    "\t    [-i[extension]] [file ...]\n", getprogname(), getprogname());
	exit(1);
}

/*
 * Add a compilation unit to the linked list
 */
static void
add_compunit(enum e_cut type, char *s)
{
	struct s_compunit *cu;

	cu = xmalloc(sizeof(struct s_compunit));
	cu->type = type;
	cu->s = s;
	cu->next = NULL;
	*cu_nextp = cu;
	cu_nextp = &cu->next;
}

/*
 * Add a file to the linked list
 */
static void
add_file(char *s)
{
	struct s_flist *fp;

	fp = xmalloc(sizeof(struct s_flist));
	fp->next = NULL;
	*fl_nextp = fp;
	fp->fname = s;
	fl_nextp = &fp->next;
}
