/*	$NetBSD: grep.c,v 1.2 2004/05/05 14:34:55 cjep Exp $	*/

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coïdan Smørgrav
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
 


#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: grep.c,v 1.2 2004/05/05 14:34:55 cjep Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grep.h"

/* 
 * Upper bound of number of digits to represent an int in decimal
 *  2^8n <= 10^3n. Allow a terminator.
 */
#define MAX_BUF_DIGITS	(sizeof(int) * 3) + 1

/* Flags passed to regcomp() and regexec() */
int cflags = REG_BASIC;
int eflags = REG_STARTEND;

int matchall;	/* shortcut */
int patterns, pattern_sz;
char **pattern;
regex_t	*r_pattern;

/* For regex errors  */
char re_error[RE_ERROR_BUF + 1];

/* Command-line flags */
int Aflag;		/* -A x: print x lines trailing each match */
int Bflag;		/* -B x: print x lines leading each match */
int Eflag;		/* -E: interpret pattern as extended regexp */
int Fflag;		/* -F: interpret pattern as list of fixed strings */
int Gflag;		/* -G: interpret pattern as basic regexp */
int Hflag;		/* -H: Always print filenames */
int Lflag;		/* -L: only show names of files with no matches */
/*int Pflag;		*//* -P: if -r, no symlinks are followed */
/*int Sflag;		*//* -S: if -r, follow all symlinks */
int bflag;		/* -b: show block numbers for each match */
int cflag;		/* -c: only show a count of matching lines */
int hflag;		/* -h: Never print filenames. -H overrides */
int lflag;		/* -l: only show names of files with matches */
int mflag;		/* -m: specify maximum line matches (per file) */
int nflag;		/* -n: show line numbers in front of matching lines */
int oflag;		/* -o: only print out matches */
int qflag;		/* -q: quiet mode (don't output anything) */
int sflag;		/* -s: silent mode (ignore errors) */
int vflag;		/* -v: only show non-matching lines */
int wflag;		/* -w: pattern must start and end on word boundaries */
int xflag;		/* -x: pattern must match entire line */
int lbflag;		/* --line-buffered */

int colours = 0;	/* Attempt to use terminal colours */
const char *grep_colour = "01;32"; /* Default colour string, green */
char *uc;

/* Characters to print after filenames */
char fn_endchar = '\n';
char fn_colonchar = ':';
char fn_dashchar = '-';
char line_endchar = '\n';	/* End of line character */

int maxcount = 0;		/* Maximum line matches per file */
int output_filenames = 0;

/* Argv[0] flags */
int zgrep;		/* If we are invoked as zgrep */

int binbehave = BIN_FILE_BIN;
int dirbehave = GREP_READ;
int devbehave = GREP_READ;
/*int linkbehave = LINK_FOLLOW;*/
char *stdin_label;

enum {
	BIN_OPT = CHAR_MAX + 1,
	HELP_OPT,
	LABEL_OPT,
	MMAP_OPT,
	LINK_OPT,
	COLOUR_OPT,
	LINEBUF_OPT
};

/* Housekeeping */
int first;		/* flag whether or not this is our first match */
int tail;		/* lines left to print */

static void
usage(void)
{
	fprintf(stderr, "usage: %s %s %s\n",
		getprogname(),
		"[-[ABC] num] [-EFGHILVZabcdhilnoqrsvwxz]",
		"[-D action] [-d action] [-e pattern] [-f file]");
	exit(2);
}

static char *optstr = "0123456789A:B:C:D:EFGHILUVZabcd:e:f:hilm:noqrsuvwxyz";

struct option long_options[] =
{
	{"binary-files",	required_argument, NULL, BIN_OPT},
	{"help",		no_argument, 	   NULL, HELP_OPT},
	{"label",		required_argument, NULL, LABEL_OPT},
	{"mmap",		no_argument, 	   NULL, MMAP_OPT},
	{"line-buffered",	no_argument, 	   NULL, LINEBUF_OPT},
/*	{"links",		required_argument, NULL, LINK_OPT},*/
	{"after-context",       required_argument, NULL, 'A'},
	{"before-context",      required_argument, NULL, 'B'},
	{"color",		optional_argument, NULL, COLOUR_OPT},
	{"colour",		optional_argument, NULL, COLOUR_OPT},
	{"context",             optional_argument, NULL, 'C'},
	{"devices",		required_argument, NULL, 'D'},
	{"extended-regexp",     no_argument,       NULL, 'E'},
	{"fixed-strings",       no_argument,       NULL, 'F'},
	{"fixed-regexp",	no_argument,       NULL, 'F'},
	{"basic-regexp",        no_argument,       NULL, 'G'},
	{"with-filename",	no_argument,	   NULL, 'H'},
	{"files-without-match", no_argument,       NULL, 'L'},
	{"binary",              no_argument,       NULL, 'U'},
	{"version",             no_argument,       NULL, 'V'},
	{"null",		no_argument,	   NULL, 'Z'},
	{"text",                no_argument,       NULL, 'a'},
	{"byte-offset",         no_argument,       NULL, 'b'},
	{"count",               no_argument,       NULL, 'c'},
	{"directories",		required_argument, NULL, 'd'},
	{"regexp",              required_argument, NULL, 'e'},
	{"file",                required_argument, NULL, 'f'},
	{"no-filename",         no_argument,       NULL, 'h'},
	{"ignore-case",         no_argument,       NULL, 'i'},
	{"files-with-matches",  no_argument,       NULL, 'l'},
	{"max-count",		required_argument, NULL, 'm'},
	{"line-number",         no_argument,       NULL, 'n'},
	{"only-matching",	no_argument,	   NULL, 'o'},
	{"quiet",               no_argument,       NULL, 'q'},
	{"silent",              no_argument,       NULL, 'q'},
	{"recursive",           no_argument,       NULL, 'r'},
	{"no-messages",         no_argument,       NULL, 's'},
	{"unix-byte-offsets",   no_argument,       NULL, 'u'},
	{"invert-match",        no_argument,       NULL, 'v'},
	{"word-regexp",         no_argument,       NULL, 'w'},
	{"line-regexp",         no_argument,       NULL, 'x'},
	{"null-data",		no_argument,	   NULL, 'z'},
		
	{NULL,                  no_argument,       NULL, 0}
};

static void
add_pattern(char *pat, size_t len)
{
	if (len == 0 || matchall) {
		matchall = 1;
		return;
	}
	if (patterns == pattern_sz) {
		pattern_sz *= 2;
		pattern = grep_realloc(pattern, ++pattern_sz * sizeof(*pattern));
	}
	if (pat[len - 1] == '\n')
		--len;
	pattern[patterns] = grep_malloc(len + 1);
	strncpy(pattern[patterns], pat, len);
	pattern[patterns][len] = '\0';
	++patterns;
}

static void
read_patterns(char *fn)
{
	FILE *f;
	char *line;
	size_t len;
	int nl;

	if ((f = fopen(fn, "r")) == NULL)
		err(2, "%s", fn);
	nl = 0;
	while ((line = fgetln(f, &len)) != NULL) {
		if (*line == '\n') {
			++nl;
			continue;
		}
		if (nl) {
			matchall = 1;
			break;
		}
		nl = 0;
		add_pattern(line, len);
	}
	if (ferror(f))
		err(2, "%s", fn);
	fclose(f);
}

static int
check_context_arg(char const *str) {
	char *ep;
	long lval;

	errno = 0;
	lval = strtol(str, &ep, 10);

	if (str[0] == '\0' || *ep != '\0')
		errx(2, "Invalid context argument");

	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
 	    (lval > INT_MAX || lval < INT_MIN))
		errx(2, "Context argument out of range");

	return lval;

}

static int
grep_getopt(int argc, char *const *argv) 
{
	int c, ptr;
	char buffer[MAX_BUF_DIGITS];

	ptr = 0;
	while (c = getopt_long(argc, argv, optstr, long_options, 
		(int *)NULL), '0' <= c && 
		c <= '9' && ptr < MAX_BUF_DIGITS) {
		
		/* Avoid leading zeros */
		if (ptr != 0 || (ptr == 0 && c != '0'))
			buffer[ptr++] = c;
	}

	if (ptr >= MAX_BUF_DIGITS)
		errx(2, "Context argument out of range");

	if (ptr) {
		buffer[ptr] = '\0';	/* We now have a string of digits */
		Aflag = Bflag = check_context_arg(buffer);
	}

	return c;
}

int
main(int argc, char *argv[])
{
	const char *progname;
	int c, i;
	struct stat sb;
	
	stdin_label = "(standard input)";

	progname = getprogname();
	switch (progname[0]) {
	case 'e':
		Eflag++;
		break;
	case 'f':
		Fflag++;
		break;
	case 'g':
		Gflag++;
		break;
	case 'z':
		zgrep++;
		switch (progname[1]) {
		case 'e':
			Eflag++;
			break;
		case 'f':
			Fflag++;
			break;
		case 'g':
			Gflag++;
			break;
		}
		break;
	}
	
	while ((c = grep_getopt(argc, argv)) != -1) {

		switch (c) {

		case 'A':
			Aflag = check_context_arg(optarg);
			break;
		case 'B':
			Bflag = check_context_arg(optarg);
			break;
		case 'C':
			if (optarg == NULL) 
				Aflag = Bflag = 2;
			else
				Aflag = Bflag = check_context_arg(optarg);
			break;
		case 'D':
			if (strcmp("read", optarg) == 0)
				devbehave = GREP_READ;
			else if (strcmp("skip", optarg) == 0)
				devbehave = GREP_SKIP;
			else {
				 errx(2, "Unknown device option");                             
			}
			break;
			
		case 'E':
			Fflag = Gflag = 0;
			Eflag++;
			break;
		case 'F':
			Eflag = Gflag = 0;
			Fflag++;
			break;
		case 'G':
			Eflag = Fflag = 0;
			Gflag++;
			break;
		case 'H':
			Hflag = 1;
			break;
		case 'I':
			binbehave = BIN_FILE_SKIP;
			break;
		case 'L':
			lflag = 0;
			Lflag = qflag = 1;
			break;
/*		case 'P':
			linkbehave = LINK_SKIP;
			break;
		case 'S':
			linkbehave = LINK_FOLLOW;
			break;*/
		case 'R':
		case 'r':
			dirbehave = GREP_RECURSE;
			break;
		case 'U':
		case 'u':
			/* these are here for compatability */
			break;
		case 'V':
			fprintf(stdout, "grep version %s\n", VERSION);
			exit(0);
			break;
		case 'Z':
			fn_colonchar = fn_endchar = fn_dashchar = 0;
			break;
		case 'a':
			binbehave = BIN_FILE_TEXT;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			if (strcmp("read", optarg) == 0)
				dirbehave = GREP_READ;
			else if (strcmp("skip", optarg) == 0)
				dirbehave = GREP_SKIP;
			else if (strcmp("recurse", optarg) == 0)
				dirbehave = GREP_RECURSE;
			else {
				errx(2, "Unknown directory option\n");
			}
			break;

		case 'e':
			add_pattern(optarg, strlen(optarg));
			break;
		case 'f':
			read_patterns(optarg);
			break;
		case 'h':
			hflag = 1;
			break;
		case 'i':
		case 'y':
			cflags |= REG_ICASE;
			break;
		case 'l':
			Lflag = 0;
			lflag = qflag = 1;
			break;
		case 'm':
			mflag = 1;
			maxcount = strtol(optarg, (char **)NULL, 10);
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			oflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'z':
			line_endchar = 0;
			break;
		case BIN_OPT:
			if (strcmp("binary", optarg) == 0)
				binbehave = BIN_FILE_BIN;
			else if (strcmp("without-match", optarg) == 0)
				binbehave = BIN_FILE_SKIP;
			else if (strcmp("text", optarg) == 0)
				binbehave = BIN_FILE_TEXT;
			else {
                                errx(2, "Unknown binary-files option\n");
			}
                        break;
		
		case COLOUR_OPT:
			if (optarg == NULL || strcmp("auto", optarg) == 0 ||
			      strcmp("tty", optarg) == 0 ||
			      strcmp("if-tty", optarg) == 0) {

				/* Check that stdout is a terminal */
				if (isatty(STDOUT_FILENO) && 
					getenv("TERM") &&
					strcmp(getenv("TERM"), "dumb") != 0)
						colours = 1;
				else
					colours = 0;

			} else if (strcmp("always", optarg) == 0 ||
				   strcmp("yes", optarg) == 0 ||
				   strcmp("force", optarg) == 0)
				colours = 1;
			else if (strcmp("never", optarg) == 0 ||
				 strcmp("no", optarg) == 0 ||
				 strcmp("none", optarg) == 0) 
				colours = 0;
			else 
				errx(2, "Unknown color option\n");

			uc = getenv("GREP_COLOR");
			if (colours == 1 && uc != NULL && *uc != '\0')
				grep_colour = uc;
			break;
		case LABEL_OPT:
			stdin_label = optarg;
			break;
		case MMAP_OPT:
			break;
/*
 * 		case LINK_OPT:
 * 			if (strcmp("explicit", optarg) == 0)
 * 				linkbehave = LINK_EXPLICIT;
 * 			else if (strcmp("follow", optarg) == 0)
 * 				linkbehave = LINK_FOLLOW;
 * 			else if (strcmp("skip", optarg) == 0)
 * 				linkbehave = LINK_SKIP;
 *                         else {
 *                                 errx(2, "Unknown links option\n");
 * 			}
 *                         break;
 */
		case LINEBUF_OPT:
			lbflag = 1;
			break;

		case HELP_OPT:
		default:
			usage();
		}
		
	}
	
	argc -= optind;
	argv += optind;

	if (argc == 0 && patterns == 0)
		usage();
	if (patterns == 0) {
		add_pattern(*argv, strlen(*argv));
		--argc;
		++argv;
	}

	if (Eflag)
		cflags |= REG_EXTENDED;
	else if (Fflag)
		cflags |= REG_NOSPEC;
	r_pattern = grep_malloc(patterns * sizeof(*r_pattern));
	for (i = 0; i < patterns; ++i) {
		if ((c = regcomp(&r_pattern[i], pattern[i], cflags))) {
			regerror(c, &r_pattern[i], re_error, RE_ERROR_BUF);
			errx(2, "%s", re_error);
		}
	}

	if ((argc > 1 && !hflag) || Hflag)
		output_filenames = 1;

	if (argc == 1 && !hflag && dirbehave == GREP_RECURSE)
		if (!stat(*argv, &sb) && (sb.st_mode & S_IFMT) == S_IFDIR)
			output_filenames = 1;
	
	if (argc == 0)
		exit(!procfile(NULL));

	if (lbflag)
		setlinebuf(stdout);

	if (dirbehave == GREP_RECURSE)
		c = grep_tree(argv);
	else
		for (c = 0; argc--; ++argv)
			c += procfile(*argv);

	exit(!c);
}
