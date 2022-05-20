/*	$NetBSD: main1.c,v 1.62 2022/05/20 21:18:55 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: main1.c,v 1.62 2022/05/20 21:18:55 rillig Exp $");
#endif

#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lint1.h"

/* set yydebug to 1*/
bool	yflag;

/*
 * Print warnings if an assignment of an integer type to another integer type
 * causes an implicit narrowing conversion. If aflag is 1, these warnings
 * are printed only if the source type is at least as wide as long. If aflag
 * is greater than 1, they are always printed.
 */
int	aflag;

/* Print a warning if a break statement cannot be reached. */
bool	bflag;

/* Print warnings for pointer casts. */
bool	cflag;

/* Perform stricter checking of enum types and operations on enum types. */
bool	eflag;

/* Print complete pathnames, not only the basename. */
bool	Fflag;

/* Treat warnings as errors */
bool	wflag;

/*
 * Apply a number of heuristic tests to attempt to intuit bugs, improve
 * style, and reduce waste.
 */
bool	hflag;

/* Attempt to check portability to other dialects of C. */
bool	pflag;

/*
 * In case of redeclarations/redefinitions print the location of the
 * previous declaration/definition.
 */
bool	rflag;

bool	Tflag;

/* Picky flag */
bool	Pflag;

/*
 * Complain about functions and external variables used and not defined,
 * or defined and not used.
 */
bool	uflag = true;

/* Complain about unused function arguments. */
bool	vflag = true;

/* Complain about structures which are never defined. */
bool	zflag = true;

/*
 * The default language level is the one that checks for compatibility
 * between traditional C and C90.  As of 2022, this default is no longer
 * useful since most traditional C code has already been migrated.
 */
bool	allow_trad = true;
bool	allow_c90 = true;
bool	allow_c99;
bool	allow_c11;
bool	allow_gcc;

err_set	msgset;

sig_atomic_t fpe;

static	void	usage(void);

static FILE *
gcc_builtins(void)
{
	/* https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html */
	static const char builtins[] =
	    "typedef typeof(sizeof(0)) __lint_size_t;\n"

	    "void *alloca(__lint_size_t);\n"
	    "void *__builtin_alloca(__lint_size_t);\n"
	    "void *__builtin_alloca_with_align"
		"(__lint_size_t, __lint_size_t);\n"
	    "void *__builtin_alloca_with_align_and_max"
		"(__lint_size_t, __lint_size_t, __lint_size_t);\n"

	    "int __builtin_isinf(long double);\n"
	    "int __builtin_isnan(long double);\n"
	    "int __builtin_copysign(long double, long double);\n";
	size_t builtins_len = sizeof(builtins) - 1;

#if HAVE_NBTOOL_CONFIG_H
	char template[] = "/tmp/lint.XXXXXX";
	int fd;
	FILE *fp;
	if ((fd = mkstemp(template)) == -1)
		return NULL;
	(void)unlink(template);
	if ((fp = fdopen(fd, "r+")) == NULL) {
		(void)close(fd);
		return NULL;
	}
	if (fwrite(builtins, 1, builtins_len, fp) != builtins_len) {
		(void)fclose(fp);
		return NULL;
	}
	rewind(fp);
	return fp;
#else
	return fmemopen(__UNCONST(builtins), builtins_len, "r");
#endif
}

/*ARGSUSED*/
static void
sigfpe(int s)
{
	fpe = 1;
}

static void
suppress_messages(char *ids)
{
	char *ptr, *end;
	long id;

	for (ptr = strtok(ids, ","); ptr != NULL; ptr = strtok(NULL, ",")) {
		errno = 0;
		id = strtol(ptr, &end, 0);
		if ((id == TARG_LONG_MIN || id == TARG_LONG_MAX) &&
		    errno == ERANGE)
			err(1, "invalid error message id '%s'", ptr);
		if (*end != '\0' || ptr == end || id < 0 || id >= ERR_SETSIZE)
			errx(1, "invalid error message id '%s'", ptr);
		ERR_SET(id, &msgset);
	}
}

int
main(int argc, char *argv[])
{
	int c;

	setprogname(argv[0]);

	ERR_ZERO(&msgset);
	while ((c = getopt(argc, argv, "abceghmprstuvwyzA:FPR:STX:")) != -1) {
		switch (c) {
		case 'a':	aflag++;	break;
		case 'b':	bflag = true;	break;
		case 'c':	cflag = true;	break;
		case 'e':	eflag = true;	break;
		case 'F':	Fflag = true;	break;
		case 'g':	allow_gcc = true;	break;
		case 'h':	hflag = true;	break;
		case 'p':	pflag = true;	break;
		case 'P':	Pflag = true;	break;
		case 'r':	rflag = true;	break;
		case 's':
			allow_trad = false;
			allow_c90 = true;
			allow_c99 = false;
			allow_c11 = false;
			break;
		case 'S':
			allow_trad = false;
			allow_c90 = true;
			allow_c99 = true;
			allow_c11 = false;
			break;
		case 'T':	Tflag = true;	break;
		case 't':
			allow_trad = true;
			allow_c90 = false;
			allow_c99 = false;
			allow_c11 = false;
			break;
		case 'u':	uflag = false;	break;
		case 'w':	wflag = true;	break;
		case 'v':	vflag = false;	break;
		case 'y':	yflag = true;	break;
		case 'z':	zflag = false;	break;

		case 'A':
			if (strcmp(optarg, "c11") == 0) {
				allow_trad = false;
				allow_c90 = true;
				allow_c99 = true;
				allow_c11 = true;
			} else
				usage();
			break;

		case 'm':
			msglist();
			return 0;

		case 'R':
			add_directory_replacement(optarg);
			break;

		case 'X':
			suppress_messages(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();


	/* initialize output */
	outopen(argv[1]);

#ifdef DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
#ifdef YYDEBUG
	if (yflag)
		yydebug = 1;
#endif

	(void)signal(SIGFPE, sigfpe);
	initmem();
	initdecl();
	initscan();

	if (allow_gcc && allow_c90) {
		if ((yyin = gcc_builtins()) == NULL)
			err(1, "cannot open builtins");
		yyparse();
		(void)fclose(yyin);
	}

	/* open the input file */
	if ((yyin = fopen(argv[0], "r")) == NULL)
		err(1, "cannot open '%s'", argv[0]);
	yyparse();
	(void)fclose(yyin);

	/* Following warnings cannot be suppressed by LINTED */
	lwarn = LWARN_ALL;
	debug_step("main lwarn = %d", lwarn);

	check_global_symbols();

	outclose();

	return nerr != 0 ? 1 : 0;
}

static void __attribute__((noreturn))
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-abceghmprstuvwyzFPST] [-Ac11] [-R old=new]\n"
	    "       %*s [-X <id>[,<id>]...] src dest\n",
	    getprogname(), (int)strlen(getprogname()), "");
	exit(1);
}

void __attribute__((noreturn))
norecover(void)
{
	/* cannot recover from previous errors */
	error(224);
	exit(1);
}
