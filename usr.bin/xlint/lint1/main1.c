/*	$NetBSD: main1.c,v 1.74 2023/07/13 08:40:38 rillig Exp $	*/

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
 *	This product includes software developed by Jochen Pohl for
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
__RCSID("$NetBSD: main1.c,v 1.74 2023/07/13 08:40:38 rillig Exp $");
#endif

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lint1.h"

int	aflag;
bool	bflag;
bool	cflag;
bool	eflag;
bool	Fflag;
bool	hflag;
bool	Pflag;
bool	pflag;
bool	rflag;
bool	Tflag;
bool	uflag;
bool	vflag;
bool	wflag;
bool	yflag;
bool	zflag;

/*
 * The default language level is the one that checks for compatibility
 * between traditional C and C90.  As of 2022, this default is no longer
 * useful since most traditional C code has already been migrated.
 */
bool	allow_trad = true;
bool	allow_c90 = true;
bool	allow_c99;
bool	allow_c11;
bool	allow_c23;
bool	allow_gcc;

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

int
main(int argc, char *argv[])
{
	int c;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "abceghmpq:rstuvwyzA:FPR:STX:")) != -1) {
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
		case 'q':	enable_queries(optarg);	break;
		case 'r':	rflag = true;	break;
		case 's':
			allow_trad = false;
			allow_c90 = true;
			allow_c99 = false;
			allow_c11 = false;
			allow_c23 = false;
			break;
		case 'S':
			allow_trad = false;
			allow_c90 = true;
			allow_c99 = true;
			allow_c11 = false;
			allow_c23 = false;
			break;
		case 'T':	Tflag = true;	break;
		case 't':
			allow_trad = true;
			allow_c90 = false;
			allow_c99 = false;
			allow_c11 = false;
			allow_c23 = false;
			break;
		case 'u':	uflag = true;	break;
		case 'w':	wflag = true;	break;
		case 'v':	vflag = true;	break;
		case 'y':	yflag = true;	break;
		case 'z':	zflag = true;	break;

		case 'A':
			if (strcmp(optarg, "c23") == 0) {
				allow_trad = false;
				allow_c90 = true;
				allow_c99 = true;
				allow_c11 = true;
				allow_c23 = true;
			} else if (strcmp(optarg, "c11") == 0) {
				allow_trad = false;
				allow_c90 = true;
				allow_c99 = true;
				allow_c11 = true;
				allow_c23 = false;
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
	outopen(any_query_enabled ? "/dev/null" : argv[1]);

#ifdef DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
#if YYDEBUG
	if (yflag)
		yydebug = 1;
#endif

	(void)signal(SIGFPE, sigfpe);
	initdecl();
	initscan();

	if (allow_gcc && allow_c90) {
		if ((yyin = gcc_builtins()) == NULL)
			err(1, "cannot open builtins");
		curr_pos.p_file = "<gcc-builtins>";
		curr_pos.p_line = 0;
		lex_next_line();
		yyparse();
		(void)fclose(yyin);
	}

	/* open the input file */
	if ((yyin = fopen(argv[0], "r")) == NULL)
		err(1, "cannot open '%s'", argv[0]);
	curr_pos.p_file = argv[0];
	curr_pos.p_line = 0;
	lex_next_line();
	yyparse();
	(void)fclose(yyin);

	/* Following warnings cannot be suppressed by LINTED */
	lwarn = LWARN_ALL;
	debug_step("main lwarn = %d", lwarn);

	check_global_symbols();

	outclose();

	return seen_error || (wflag && seen_warning) ? 1 : 0;
}

static void __attribute__((noreturn))
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-abceghmprstuvwyzFPST] [-Alevel] [-ddirectory] "
	    "[-R old=new]\n"
	    "       %*s [-X id,...] [-q id,...] src dest\n",
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
