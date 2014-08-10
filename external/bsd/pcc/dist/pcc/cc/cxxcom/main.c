/*	Id: main.c,v 1.5 2014/04/09 10:01:24 plunky Exp 	*/	
/*	$NetBSD: main.c,v 1.1.1.2.12.1 2014/08/10 07:10:07 tls Exp $	*/

/*
 * Copyright (c) 2002 Anders Magnusson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "pass1.h"
#include "pass2.h"

int bdebug, ddebug, edebug, idebug, ndebug;
int odebug, pdebug, sdebug, tdebug, xdebug, cppdebug;
int b2debug, c2debug, e2debug, f2debug, g2debug, o2debug;
int r2debug, s2debug, t2debug, u2debug, x2debug;
int gflag, kflag, pflag, sflag;
int sspflag;
int xssa, xtailcall, xtemps, xdeljumps, xdce, xinline, xccp, xgnu89, xgnu99;
int xuchar;
int freestanding;
char *prgname;

static void prtstats(void);

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [option] [infile] [outfile]...\n",
	    prgname);
	exit(1);
}

static void
segvcatch(int a)
{
	char buf[1024];

	snprintf(buf, sizeof buf, "%sinternal compiler error: %s, line %d\n",
	    nerrors ? "" : "major ", ftitle, lineno);
	(void)write(STDERR_FILENO, buf, strlen(buf));
	_exit(1);
}

static void
xopt(char *str)
{
	if (strcmp(str, "ssa") == 0)
		xssa++;
	else if (strcmp(str, "tailcall") == 0)
		xtailcall++;
	else if (strcmp(str, "temps") == 0)
		xtemps++;
	else if (strcmp(str, "deljumps") == 0)
		xdeljumps++;
	else if (strcmp(str, "dce") == 0)
		xdce++;
	else if (strcmp(str, "inline") == 0)
		xinline++;
	else if (strcmp(str, "ccp") == 0)
		xccp++;
	else if (strcmp(str, "gnu89") == 0)
		xgnu89++;
	else if (strcmp(str, "gnu99") == 0)
		xgnu99++;
	else if (strcmp(str, "uchar") == 0)
		xuchar++;
	else {
		fprintf(stderr, "unknown -x option '%s'\n", str);
		usage();
	}
}

static void
fflags(char *str)
{
	int flagval = 1;

	if (strncmp("no-", str, 3) == 0) {
		str += 3;
		flagval = 0;
	}

	if (strcmp(str, "stack-protector") == 0)
		sspflag = flagval;
	else if (strcmp(str, "stack-protector-all") == 0)
		sspflag = flagval;
	else if (strncmp(str, "pack-struct", 11) == 0)
		pragma_allpacked = (strlen(str) > 12 ? atoi(str+12) : 1);
	else if (strcmp(str, "freestanding") == 0)
		freestanding = flagval;
	else {
		fprintf(stderr, "unknown -f option '%s'\n", str);
		usage();
	}
}

/* control multiple files */
int
main(int argc, char *argv[])
{
	int ch;

#ifdef TIMING
	struct timeval t1, t2;

	(void)gettimeofday(&t1, NULL);
#endif

	prgname = argv[0];

	while ((ch = getopt(argc, argv, "OT:VW:X:Z:f:gkm:psvwx:")) != -1) {
		switch (ch) {
#if !defined(MULTIPASS) || defined(PASS1)
		case 'X':	/* pass1 debugging */
			while (*optarg)
				switch (*optarg++) {
				case 'b': ++bdebug; break; /* buildtree */
				case 'd': ++ddebug; break; /* declarations */
				case 'e': ++edebug; break; /* pass1 exit */
				case 'i': ++idebug; break; /* initializations */
				case 'n': ++ndebug; break; /* node allocation */
				case 'o': ++odebug; break; /* optim */
				case 'p': ++pdebug; break; /* prototype */
				case 's': ++sdebug; break; /* inline */
				case 't': ++tdebug; break; /* type match */
				case 'x': ++xdebug; break; /* MD code */
				case '+': ++cppdebug; break; /* C++ */
				default:
					fprintf(stderr, "unknown -X flag '%c'\n",
					    optarg[-1]);
					exit(1);
				}
			break;
#endif
#if !defined(MULTIPASS) || defined(PASS2)
		case 'Z':	/* pass2 debugging */
			while (*optarg)
				switch (*optarg++) {
				case 'b': /* basic block and SSA building */
					++b2debug;
					break;
				case 'c': /* code printout */
					++c2debug;
					break;
				case 'e': /* print tree upon pass2 enter */
					++e2debug;
					break;
				case 'f': /* instruction matching */
					++f2debug;
					break;
				case 'g': /* print flow graphs */
					++g2debug;
					break;
				case 'n': /* node allocation */
					++ndebug;
					break;
				case 'o': /* instruction generator */
					++o2debug;
					break;
				case 'r': /* register alloc/graph coloring */
					++r2debug;
					break;
				case 's': /* shape matching */
					++s2debug;
					break;
				case 't': /* type matching */
					++t2debug;
					break;
				case 'u': /* Sethi-Ullman debugging */
					++u2debug;
					break;
				case 'x': /* target specific */
					++x2debug;
					break;
				default:
					fprintf(stderr, "unknown -Z flag '%c'\n",
					    optarg[-1]);
					exit(1);
				}
			break;
#endif
		case 'f': /* Language */
			fflags(optarg);
			break;

		case 'g': /* Debugging */
			gflag = 1;
			break;

		case 'k': /* PIC code */
			++kflag;
			break;

		case 'm': /* Target-specific */
			mflags(optarg);
			break;

		case 'p': /* Profiling */
			pflag = 1;
			break;

		case 's': /* Statistics */
			++sflag;
			break;

		case 'W': /* Enable different warnings */
			Wflags(optarg);
			break;

		case 'x': /* Different settings */
			xopt(optarg);
			break;

		case 'v':
			printf("ccom: %s\n", VERSSTR);
			break;

		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0 && strcmp(argv[0], "-") != 0) {
		if (freopen(argv[0], "r", stdin) == NULL) {
			fprintf(stderr, "open input file '%s':",
			    argv[0]);
			perror(NULL);
			exit(1);
		}
	}
	if (argc > 1 && strcmp(argv[1], "-") != 0) {
		if (freopen(argv[1], "w", stdout) == NULL) {
			fprintf(stderr, "open output file '%s':",
			    argv[1]);
			perror(NULL);
			exit(1);
		}
	}

	mkdope();
	signal(SIGSEGV, segvcatch);
#ifdef SIGBUS
	signal(SIGBUS, segvcatch);
#endif
	fregs = FREGS;	/* number of free registers */
	lineno = 1;
#ifdef GCC_COMPAT
	gcc_init();
#endif

	/* starts past any of the above */
	reached = 1;

	bjobcode();
#ifndef TARGET_VALIST
	{
		NODE *p = block(NAME, NIL, NIL, PTR|CHAR, NULL, 0);
		struct symtab *sp = lookup(addname("__builtin_va_list"), 0);
		p->n_sp = sp;
		defid(p, TYPEDEF);
		nfree(p);
	}
#endif
	complinit();

#ifdef STABS
	if (gflag) {
		stabs_file(argc ? argv[0] : "");
		stabs_init();
	}
#endif

	if (sspflag)
		sspinit();

	(void) yyparse();
	yyaccpt();

	if (!nerrors)
		lcommprint();

#ifdef STABS
	if (gflag)
		stabs_efile(argc ? argv[0] : "");
#endif

	ejobcode( nerrors ? 1 : 0 );

#ifdef TIMING
	(void)gettimeofday(&t2, NULL);
	t2.tv_sec -= t1.tv_sec;
	t2.tv_usec -= t1.tv_usec;
	if (t2.tv_usec < 0) {
		t2.tv_usec += 1000000;
		t2.tv_sec -= 1;
	}
	fprintf(stderr, "ccom total time: %ld s %ld us\n",
	    t2.tv_sec, t2.tv_usec);
#endif

	if (sflag)
		prtstats();

	return(nerrors?1:0);
}

void
prtstats(void)
{
	extern int nametabs, namestrlen, tmpallocsize, permallocsize;
	extern int lostmem, arglistcnt, dimfuncnt, inlnodecnt, inlstatcnt;
	extern int symtabcnt, suedefcnt;

	fprintf(stderr, "Name table entries:		%d pcs\n", nametabs);
	fprintf(stderr, "Name string size:		%d B\n", namestrlen);
	fprintf(stderr, "Permanent allocated memory:	%d B\n", permallocsize);
	fprintf(stderr, "Temporary allocated memory:	%d B\n", tmpallocsize);
	fprintf(stderr, "Lost memory:			%d B\n", lostmem);
	fprintf(stderr, "Argument list unions:		%d pcs\n", arglistcnt);
	fprintf(stderr, "Dimension/function unions:	%d pcs\n", dimfuncnt);
	fprintf(stderr, "Struct/union/enum blocks:	%d pcs\n", suedefcnt);
	fprintf(stderr, "Inline node count:		%d pcs\n", inlnodecnt);
	fprintf(stderr, "Inline control blocks:		%d pcs\n", inlstatcnt);
	fprintf(stderr, "Permanent symtab entries:	%d pcs\n", symtabcnt);
}
