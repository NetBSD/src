/*	$OpenBSD: main.c,v 1.77 2009/10/14 17:19:47 sthen Exp $	*/
/*	$NetBSD: main.c,v 1.48 2019/03/26 16:41:06 christos Exp $	*/

/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ozan Yigit at York University.
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

/*
 * main.c
 * Facility: m4 macro processor
 * by: oz
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif
#include <sys/cdefs.h>
__RCSID("$NetBSD: main.c,v 1.48 2019/03/26 16:41:06 christos Exp $");
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <ohash.h>
#include "mdef.h"
#include "stdd.h"
#include "extern.h"
#include "pathnames.h"

ndptr hashtab[HASHSIZE];	/* hash table for macros etc.  */
stae *mstack;		 	/* stack of m4 machine         */
char *sstack;		 	/* shadow stack, for string space extension */
static size_t STACKMAX;		/* current maximum size of stack */
int sp; 			/* current m4  stack pointer   */
int fp; 			/* m4 call frame pointer       */
struct input_file infile[MAXINP];/* input file stack (0=stdin)  */
FILE **outfile;			/* diversion array(0=bitbucket)*/
int maxout;
FILE *active;			/* active output file pointer  */
int ilevel = 0; 		/* input file stack pointer    */
int oindex = 0; 		/* diversion index..	       */
const char *null = "";          /* as it says.. just a null..  */
char **m4wraps = NULL;		/* m4wraps array.     	       */
int maxwraps = 0;		/* size of m4wraps array       */
int wrapindex = 0;		/* current offset in m4wraps   */
char lquote[MAXCCHARS+1] = {LQUOTE};	/* left quote character  (`)   */
char rquote[MAXCCHARS+1] = {RQUOTE};	/* right quote character (')   */
char scommt[MAXCCHARS+1] = {SCOMMT};	/* start character for comment */
char ecommt[MAXCCHARS+1] = {ECOMMT};	/* end character for comment   */
int  synch_lines = 0;		/* line synchronisation for C preprocessor */
int  prefix_builtins = 0;	/* -P option to prefix builtin keywords */
int  fatal_warnings = 0;	/* -E option to exit on warnings */
int  quiet = 0;			/* -Q option to silence warnings */
int  nesting_limit = -1;	/* -L for nesting limit */
const char *freeze = NULL;	/* -F to freeze state */
const char *reload = NULL;	/* -R to reload state */
#ifndef REAL_FREEZE
FILE *freezef = NULL;
int thawing = 0;
#endif

struct keyblk {
        const char *knam;	/* keyword name */
        int	ktyp;           /* keyword type */
};

struct keyblk keywrds[] = {	/* m4 keywords to be installed */
	{ "include",      INCLTYPE },
	{ "sinclude",     SINCTYPE },
	{ "define",       DEFITYPE },
	{ "defn",         DEFNTYPE },
	{ "divert",       DIVRTYPE | NOARGS },
	{ "expr",         EXPRTYPE },
	{ "eval",         EXPRTYPE },
	{ "substr",       SUBSTYPE },
	{ "ifelse",       IFELTYPE },
	{ "ifdef",        IFDFTYPE },
	{ "len",          LENGTYPE },
	{ "incr",         INCRTYPE },
	{ "decr",         DECRTYPE },
	{ "dnl",          DNLNTYPE | NOARGS },
	{ "changequote",  CHNQTYPE | NOARGS },
	{ "changecom",    CHNCTYPE | NOARGS },
	{ "index",        INDXTYPE },
#ifdef EXTENDED
	{ "paste",        PASTTYPE },
	{ "spaste",       SPASTYPE },
    	/* Newer extensions, needed to handle gnu-m4 scripts */
	{ "indir",        INDIRTYPE},
	{ "builtin",      BUILTINTYPE},
	{ "patsubst",	  PATSTYPE},
	{ "regexp",	  REGEXPTYPE},
	{ "esyscmd",	  ESYSCMDTYPE},
	{ "__file__",	  FILENAMETYPE | NOARGS},
	{ "__line__",	  LINETYPE | NOARGS},
#endif
	{ "popdef",       POPDTYPE },
	{ "pushdef",      PUSDTYPE },
	{ "dumpdef",      DUMPTYPE | NOARGS },
	{ "shift",        SHIFTYPE | NOARGS },
	{ "translit",     TRNLTYPE },
	{ "undefine",     UNDFTYPE },
	{ "undivert",     UNDVTYPE | NOARGS },
	{ "divnum",       DIVNTYPE | NOARGS },
	{ "maketemp",     MKTMTYPE },
	{ "errprint",     ERRPTYPE | NOARGS },
	{ "m4wrap",       M4WRTYPE | NOARGS },
	{ "m4exit",       EXITTYPE | NOARGS },
	{ "syscmd",       SYSCTYPE },
	{ "sysval",       SYSVTYPE | NOARGS },
	{ "traceon",	  TRACEONTYPE | NOARGS },
	{ "traceoff",	  TRACEOFFTYPE | NOARGS },

#if defined(unix) || defined(__unix__) 
	{ "unix",         SELFTYPE | NOARGS },
#else
#ifdef vms
	{ "vms",          SELFTYPE | NOARGS },
#endif
#endif
};

#define MAXKEYS	(sizeof(keywrds)/sizeof(struct keyblk))

#define MAXRECORD 50
static struct position {
	char *name;
	unsigned long line;
} quotes[MAXRECORD], paren[MAXRECORD];

static void record(struct position *, int);
static void dump_stack(struct position *, int);

static void macro(void);
static void initkwds(void);
static ndptr inspect(int, char *);
static int do_look_ahead(int, const char *);
static void reallyoutputstr(const char *);
static void reallyputchar(int);

static void enlarge_stack(void);
static void help(void);

static void
usage(FILE *f)
{
	fprintf(f, "Usage: %s [-EGgiPQsv] [-Dname[=value]] [-d flags] "
	    "[-I dirname] [-o filename] [-L limit]\n"
	    "\t[-t macro] [-Uname] [file ...]\n", getprogname());
}

__dead static void
onintr(int signo)
{
	char intrmessage[] = "m4: interrupted.\n";
	write(STDERR_FILENO, intrmessage, sizeof(intrmessage)-1);
	_exit(1);
}

#define OPT_HELP 1

struct option longopts[] = {
	{ "debug",		optional_argument,	0, 'd' },
	{ "define",		required_argument,	0, 'D' },
	{ "error-output",	required_argument,	0, 'e' },
	{ "fatal-warnings",	no_argument,		0, 'E' },
	{ "freeze-state",	required_argument,	0, 'F' },
	{ "gnu",		no_argument,		0, 'g' },
	{ "help",		no_argument,		0, OPT_HELP },
	{ "include",		required_argument,	0, 'I' },
	{ "interactive",	no_argument,		0, 'i' },
	{ "nesting-limit",	required_argument,	0, 'L' },
	{ "prefix-builtins",	no_argument,		0, 'P' },
	{ "quiet",		no_argument,		0, 'Q' },
	{ "reload-state",	required_argument,	0, 'R' },
	{ "silent",		no_argument,		0, 'Q' },
	{ "synclines",		no_argument,		0, 's' },
	{ "trace",		required_argument,	0, 't' },
	{ "traditional",	no_argument,		0, 'G' },
	{ "undefine",		required_argument,	0, 'U' },
	{ "version",		no_argument,		0, 'v' },
#ifdef notyet
	{ "arglength",		required_argument,	0, 'l' },
	{ "debugfile",		optional_argument, 	0, OPT_DEBUGFILE },
	{ "hashsize",		required_argument,	0, 'H' },
	{ "warn-macro-sequence",optional_argument,	0, OPT_WARN_SEQUENCE },
#endif
	{ 0,			0,			0, 0 },
};

int
main(int argc, char *argv[])
{
	int c;
	int n;
	char *p;
	FILE *sfp;

	setprogname(argv[0]);

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);

	init_macros();
	initspaces();
	STACKMAX = INITSTACKMAX;

	mstack = (stae *)xalloc(sizeof(stae) * STACKMAX, NULL);
	sstack = (char *)xalloc(STACKMAX, NULL);

	maxout = 0;
	outfile = NULL;
	resizedivs(MAXOUT);

	while ((c = getopt_long(argc, argv, "D:d:e:EF:GgI:iL:o:PR:Qst:U:v",
	    longopts, NULL)) != -1)
		switch(c) {
		case 'D':               /* define something..*/
			for (p = optarg; *p; p++)
				if (*p == '=')
					break;
			if (*p)
				*p++ = EOS;
			dodefine(optarg, p);
			break;
		case 'd':
			set_trace_flags(optarg);
			break;
		case 'E':
			fatal_warnings++;
			break;
		case 'e':
			/*
			 * Don't use freopen here because if it fails
			 * we lose stderr, instead trash it.
			 */
			if ((sfp = fopen(optarg, "w+")) == NULL) {
				warn("Can't redirect errors to `%s'", optarg);
				break;
			}
			fclose(stderr);
			memcpy(stderr, sfp, sizeof(*sfp));
			break;
		case 'F':
			freeze = optarg;
#ifndef REAL_FREEZE
			if ((freezef = fopen(freeze, "w")) == NULL)
				err(EXIT_FAILURE, "Can't open `%s'", freeze);
#endif
			break;
		case 'I':
			addtoincludepath(optarg);
			break;
		case 'i':
			setvbuf(stdout, NULL, _IONBF, 0);
			signal(SIGINT, SIG_IGN);
			break;
		case 'G':
			mimic_gnu = 0;
			break;
		case 'g':
			mimic_gnu = 1;
			break;
		case 'L':
			nesting_limit = atoi(optarg);
			break;
		case 'o':
			trace_file(optarg);
                        break;
		case 'P':
			prefix_builtins = 1;
			break;
		case 'Q':
			quiet++;
			break;
		case 'R':
			reload = optarg;
			break;
		case 's':
			synch_lines = 1;
			break;
		case 't':
			mark_traced(optarg, 1);
			break;
		case 'U':               /* undefine...       */
			macro_popdef(optarg);
			break;
		case 'v':
			fprintf(stderr, "%s version %d\n", getprogname(),
			    VERSION);
			return EXIT_SUCCESS;
		case OPT_HELP:
			help();
			return EXIT_SUCCESS;
		case '?':
		default:
			usage(stderr);
			return EXIT_FAILURE;
		}

#ifdef REDIRECT
	/*
	 * This is meant only for debugging; it makes all output
	 * go to a known file, even if the command line options
	 * send it elsewhere. It should not be turned of in production code.
	 */
	if (freopen("/tmp/m4", "w+", stderr) == NULL)
		err(EXIT_FAILURE, "Can't redirect errors to `%s'",
		    "/tmp/m4");
#endif
        argc -= optind;
        argv += optind;


	initkwds();
	if (mimic_gnu)
		setup_builtin("format", FORMATTYPE);

	active = stdout;		/* default active output     */
	bbase[0] = bufbase;

	if (reload) {
#ifdef REAL_FREEZE
		thaw_state(reload);
#else
		if (fopen_trypath(infile, reload) == NULL)
			err(1, "Can't open `%s'", reload);
		sp = -1;
		fp = 0;
		thawing = 1;
		macro();
		thawing = 0;
		release_input(infile);
#endif
	}

        if (!argc) {
 		sp = -1;		/* stack pointer initialized */
		fp = 0; 		/* frame pointer initialized */
		set_input(infile+0, stdin, "stdin");
					/* default input (naturally) */
		macro();
	} else
		for (; argc--; ++argv) {
			p = *argv;
			if (p[0] == '-' && p[1] == EOS)
				set_input(infile, stdin, "stdin");
			else if (fopen_trypath(infile, p) == NULL)
				err(1, "%s", p);
			sp = -1;
			fp = 0; 
			macro();
		    	release_input(infile);
		}

	if (wrapindex) {
		int i;

		ilevel = 0;		/* in case m4wrap includes.. */
		bufbase = bp = buf;	/* use the entire buffer   */
		if (mimic_gnu) {
			while (wrapindex != 0) {
				for (i = 0; i < wrapindex; i++)
					pbstr(m4wraps[i]);
				wrapindex =0;
				macro();
			}
		} else {
			for (i = 0; i < wrapindex; i++) {
				pbstr(m4wraps[i]);
				macro();
		    	}
		}
	}

	if (active != stdout)
		active = stdout;	/* reset output just in case */
	for (n = 1; n < maxout; n++)	/* default wrap-up: undivert */
		if (outfile[n] != NULL)
			getdiv(n);
					/* remove bitbucket if used  */
	if (outfile[0] != NULL) {
		(void) fclose(outfile[0]);
	}

#ifdef REAL_FREEZE
	if (freeze)
		freeze_state(freeze);
#else
	if (freezef)
		fclose(freezef);
#endif

	return 0;
}

/*
 * Look ahead for `token'.
 * (on input `t == token[0]')
 * Used for comment and quoting delimiters.
 * Returns 1 if `token' present; copied to output.
 *         0 if `token' not found; all characters pushed back
 */
static int
do_look_ahead(int t, const char *token)
{
	int i;

	assert((unsigned char)t == (unsigned char)token[0]);

	for (i = 1; *++token; i++) {
		t = gpbc();
		if (t == EOF || (unsigned char)t != (unsigned char)*token) {
			pushback(t);
			while (--i)
				pushback(*--token);
			return 0;
		}
	}
	return 1;
}

#define LOOK_AHEAD(t, token) (t != EOF && 		\
    (unsigned char)(t)==(unsigned char)(token)[0] && 	\
    do_look_ahead(t,token))

/*
 * macro - the work horse..
 */
static void
macro(void)
{
	char token[MAXTOK+1];
	int t, l;
	ndptr p;
	int  nlpar;

	cycle {
		t = gpbc();

		if (LOOK_AHEAD(t,lquote)) {	/* strip quotes */
			nlpar = 0;
			record(quotes, nlpar++);
			/*
			 * Opening quote: scan forward until matching
			 * closing quote has been found.
			 */
			do {

				l = gpbc();
				if (LOOK_AHEAD(l,rquote)) {
					if (--nlpar > 0)
						outputstr(rquote);
				} else if (LOOK_AHEAD(l,lquote)) {
					record(quotes, nlpar++);
					outputstr(lquote);
				} else if (l == EOF) {
					if (!quiet) {
						if (nlpar == 1)
							warnx("unclosed quote:");
						else
							warnx(
							    "%d unclosed quotes:",
							    nlpar);
						dump_stack(quotes, nlpar);
					}
					exit(EXIT_FAILURE);
				} else {
					if (nlpar > 0) {
						if (sp < 0)
							reallyputchar(l);
						else
							CHRSAVE(l);
					}
				}
			}
			while (nlpar != 0);
		} else if (sp < 0 && LOOK_AHEAD(t, scommt)) {
			reallyoutputstr(scommt);

			for(;;) {
				t = gpbc();
				if (LOOK_AHEAD(t, ecommt)) {
					reallyoutputstr(ecommt);
					break;
				}
				if (t == EOF)
					break;
				reallyputchar(t);
			}
		} else if (t == '_' || isalpha(t)) {
			p = inspect(t, token);
			if (p != NULL)
				pushback(l = gpbc());
			if (p == NULL || (l != LPAREN && 
			    (macro_getdef(p)->type & NEEDARGS) != 0))
				outputstr(token);
			else {
		/*
		 * real thing.. First build a call frame:
		 */
				pushf(fp);	/* previous call frm */
				pushf(macro_getdef(p)->type); /* type of the call  */
				pushf(is_traced(p));
				pushf(0);	/* parenthesis level */
				fp = sp;	/* new frame pointer */
		/*
		 * now push the string arguments:
		 * XXX: Copy the macro definition. This leaks, but too
		 * lazy to fix properly.
		 * The problem is that if we evaluate a pushdef'ed
		 * macro and then popdef it while it the definition 
		 * is still on the stack we are going to reference
		 * free memory.
		 */
				pushs1(xstrdup(macro_getdef(p)->defn));	/* defn string */
				pushs1((char *)macro_name(p));	/* macro name  */
				pushs(ep);	      	/* start next..*/

				if (l != LPAREN && PARLEV == 0)  {   
				    /* no bracks  */
					chrsave(EOS);

					if ((size_t)sp == STACKMAX)
						errx(1, "internal stack overflow");
					eval((const char **) mstack+fp+1, 2, 
					    CALTYP, TRACESTATUS);

					ep = PREVEP;	/* flush strspace */
					sp = PREVSP;	/* previous sp..  */
					fp = PREVFP;	/* rewind stack...*/
				}
			}
		} else if (t == EOF) {
			if (sp > -1 && ilevel <= 0) {
				if (!quiet) {
					warnx("unexpected end of input, "
					    "unclosed parenthesis:");
					dump_stack(paren, PARLEV);
				}
				exit(EXIT_FAILURE);
			}
			if (ilevel <= 0)
				break;			/* all done thanks.. */
			release_input(infile+ilevel--);
			emit_synchline();
			bufbase = bbase[ilevel];
			continue;
		} else if (sp < 0) {		/* not in a macro at all */
			reallyputchar(t);	/* output directly..	 */
		}

		else switch(t) {

		case LPAREN:
			if (PARLEV > 0)
				chrsave(t);
			while (isspace(l = gpbc())) /* skip blank, tab, nl.. */
				if (PARLEV > 0)
					chrsave(l);
			pushback(l);
			record(paren, PARLEV++);
			break;

		case RPAREN:
			if (--PARLEV > 0)
				chrsave(t);
			else {			/* end of argument list */
				chrsave(EOS);

				if ((size_t)sp == STACKMAX)
					errx(1, "internal stack overflow");

				eval((const char **) mstack+fp+1, sp-fp, 
				    CALTYP, TRACESTATUS);

				ep = PREVEP;	/* flush strspace */
				sp = PREVSP;	/* previous sp..  */
				fp = PREVFP;	/* rewind stack...*/
			}
			break;

		case COMMA:
			if (PARLEV == 1) {
				chrsave(EOS);		/* new argument   */
				while (isspace(l = gpbc()))
					;
				pushback(l);
				pushs(ep);
			} else
				chrsave(t);
			break;

		default:
			if (LOOK_AHEAD(t, scommt)) {
				char *q;
				for (q = scommt; *q; q++)
					chrsave(*q);
				for(;;) {
					t = gpbc();
					if (LOOK_AHEAD(t, ecommt)) {
						for (q = ecommt; *q; q++)
							chrsave(*q);
						break;
					}
					if (t == EOF)
					    break;
					CHRSAVE(t);
				}
			} else
				CHRSAVE(t);		/* stack the char */
			break;
		}
	}
}

/* 
 * output string directly, without pushing it for reparses. 
 */
void
outputstr(const char *s)
{
	if (sp < 0)
		reallyoutputstr(s);
	else
		while (*s)
			CHRSAVE(*s++);
}

void
reallyoutputstr(const char *s)
{
	if (synch_lines) {
		while (*s) {
			fputc(*s, active);
			if (*s++ == '\n') {
				infile[ilevel].synch_lineno++;
				if (infile[ilevel].synch_lineno != 
				    infile[ilevel].lineno)
					do_emit_synchline();
			}
		}
	} else
		fputs(s, active);
}

void
reallyputchar(int c)
{
	putc(c, active);
	if (synch_lines && c == '\n') {
		infile[ilevel].synch_lineno++;
		if (infile[ilevel].synch_lineno != infile[ilevel].lineno)
			do_emit_synchline();
	}
}

/*
 * build an input token..
 * consider only those starting with _ or A-Za-z. 
 */
static ndptr
inspect(int c, char *tp) 
{
	char *name = tp;
	char *etp = tp+MAXTOK;
	ndptr p;
	
	*tp++ = c;

	while ((isalnum(c = gpbc()) || c == '_') && tp < etp)
		*tp++ = c;
	if (c != EOF)
		PUSHBACK(c);
	*tp = EOS;
	/* token is too long, it won't match anything, but it can still
	 * be output. */
	if (tp == ep) {
		outputstr(name);
		while (isalnum(c = gpbc()) || c == '_') {
			if (sp < 0)
				reallyputchar(c);
			else
				CHRSAVE(c);
		}
		*name = EOS;
		return NULL;
	}

	p = ohash_find(&macros, ohash_qlookupi(&macros, name, (void *)&tp));
	if (p == NULL)
		return NULL;
	if (macro_getdef(p) == NULL)
		return NULL;
	return p;
}

/*
 * initkwds - initialise m4 keywords as fast as possible. 
 * This very similar to install, but without certain overheads,
 * such as calling lookup. Malloc is not used for storing the 
 * keyword strings, since we simply use the static pointers
 * within keywrds block.
 */
static void
initkwds(void)
{
	unsigned int type;
	size_t i;

	for (i = 0; i < MAXKEYS; i++) {
		type = keywrds[i].ktyp;
		if ((keywrds[i].ktyp & NOARGS) == 0)
			type |= NEEDARGS;
		setup_builtin(keywrds[i].knam, type);
	}
}

static void
record(struct position *t, int lev)
{
	if (lev < MAXRECORD) {
		t[lev].name = CURRENT_NAME;
		t[lev].line = CURRENT_LINE;
	}
}

static void
dump_stack(struct position *t, int lev)
{
	int i;

	for (i = 0; i < lev; i++) {
		if (i == MAXRECORD) {
			fprintf(stderr, "   ...\n");
			break;
		}
		fprintf(stderr, "   %s at line %lu\n", 
			t[i].name, t[i].line);
	}
}


static void 
enlarge_stack(void)
{
	STACKMAX += STACKMAX/2;
	mstack = xrealloc(mstack, sizeof(stae) * STACKMAX, 
	    "Evaluation stack overflow (%lu)", 
	    (unsigned long)STACKMAX);
	sstack = xrealloc(sstack, STACKMAX,
	    "Evaluation stack overflow (%lu)", 
	    (unsigned long)STACKMAX);
}

static const struct {
	const char *n;
	const char *d;
} nd [] = {
{ "-d, --debug[=flags]",	"set debug flags" },
{ "-D, --define=name[=value]",	"define macro" },
{ "-e, --error-output=file",	"send error output to file" },
{ "-E, --fatal-warnings",	"exit on warnings" },
{ "-F, --freeze-state=file",	"save state to file" },
{ "-g, --gnu",			"enable gnu extensions" },
{ "    --help",			"print this message and exit" },
{ "-I, --include=file",		"include file" },
{ "-i, --interactive",		"unbuffer output, ignore tty signals" },
{ "-L, --nesting-limit=num",	"macro expansion nesting limit (unimpl)" },
{ "-P, --prefix-builtins",	"prefix builtins with m4_" },
{ "-Q, --quiet",		"don't print warnings" },
{ "-R, --reload-state=file",	"restore state from file" },
{ "-Q, --silent",		"don't print warnings" },
{ "-s, --synclines",		"output line directives for cpp(1)" },
{ "-t, --trace=macro",		"trace the named macro" },
{ "-G, --traditional",		"disable gnu extensions" },
{ "-U, --undefine=name",	"undefine the named macro" },
{ "-v, --version",		"print the version number and exit" },
};

static void
help(void)
{
	size_t i;
	fprintf(stdout, "%s version %d\n\n", getprogname(), VERSION);
	usage(stdout);

	fprintf(stdout, "\nThe long options are:\n");
	for (i = 0; i < __arraycount(nd); i++)
		fprintf(stdout, "\t%-25.25s\t%s\n", nd[i].n, nd[i].d);
}
