/* ed.c: This file contains the main control and user-interface routines
   for the ed line editor. */
/*-
 * Copyright (c) 1993 Andrew Moore, Talke Studio.
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
 */

#ifndef lint
char copyright1[] =
"@(#) Copyright (c) 1993 Andrew Moore, Talke Studio.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/* static char sccsid[] = "@(#)ed.c	5.5 (Talke Studio) 3/28/93"; */
static char rcsid[] = "$Id: ed.c,v 1.19 1993/11/23 04:41:51 alm Exp $";
#endif /* not lint */

/*
 * CREDITS
 *
 *	This program is based on the editor algorithm described in
 *	Brian W. Kernighan and P. J. Plauger's book "Software Tools 
 *	in Pascal," Addison-Wesley, 1981.
 *
 *	The buffering algorithm is attributed to Rodney Ruddock of
 *	the University of Guelph, Guelph, Ontario.
 *
 *	The cbc.c encryption code is adapted from
 *	the bdes program by Matt Bishop of Dartmouth College,
 *	Hanover, NH.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <pwd.h>
#include <sys/ioctl.h>

#include "ed.h"

#ifdef _POSIX_SOURCE
sigjmp_buf env;
#else
jmp_buf env;
#endif

/* static buffers */
char *shcmd;			/* shell command buffer */
int shcmdsz;			/* shell command buffer size */
int shcmdi;			/* shell command buffer index */
char *cvbuf;			/* global command buffer */
int cvbufsz;			/* global command buffer size */
char *lhbuf;			/* lhs substitution buffer */
int lhbufsz;			/* lhs substitution buffer size */
char *rhbuf;			/* rhs substitution buffer */
int rhbufsz;			/* rhs substitution buffer size */
int rhbufi;			/* rhs substitution buffer index */
char *rbuf;			/* substitute_matching_text buffer */
int rbufsz;			/* substitute_matching_text buffer size */
char *sbuf;			/* file i/o buffer */
int sbufsz;			/* file i/o buffer size */
char *ibuf;			/* ed command-line buffer */
int ibufsz;			/* ed command-line buffer size */
char *ibufp;			/* pointer to ed command-line buffer */

/* global flags */
int isglobal;			/* if set, doing a global command */
int isbinary;			/* if set, buffer contains ASCII NULs */
int modified;			/* if set, buffer modified since last write */
int garrulous = 0;		/* if set, print all error messages */
int scripted = 0;		/* if set, suppress diagnostics */
int des = 0;			/* if set, use crypt(3) for i/o */
int mutex = 0;			/* if set, signals set "sigflags" */
int sigflags = 0;		/* if set, signals received while mutex set */
int sigactive = 0;		/* if set, signal handlers are enabled */
int red = 0;			/* if set, restrict shell/directory access */

char old_filename[MAXFNAME + 1] = "";	/* default filename */
long current_addr;		/* current address */
long addr_last;			/* last address */
int lineno;			/* script line number */
char *prompt;			/* command-line prompt */
char *dps = "*";		/* default command-line prompt */

char *usage = "usage: %s [-] [-sx] [-p string] [name]\n";

extern char errmsg[];
extern int optind;
extern char *optarg;

/* ed: line editor */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int c, n;
	long status = 0;

	red = (n = strlen(argv[0])) > 2 && argv[0][n - 3] == 'r';
top:
	while ((c = getopt(argc, argv, "p:sx")) != EOF)
		switch(c) {
		case 'p':				/* set prompt */
			prompt = optarg;
			break;
		case 's':				/* run script */
			scripted = 1;
			break;
		case 'x':				/* use crypt */
#ifdef DES
			des = get_keyword();
#else
			fprintf(stderr, "crypt unavailable\n?\n");
#endif
			break;

		default:
			fprintf(stderr, usage, argv[0]);
			exit(1);
		}
	argv += optind;
	argc -= optind;
	if (argc && **argv == '-') {
		scripted = 1;
		if (argc > 1) {
			optind = 1;
			goto top;
		}
		argv++;
		argc--;
	}
	/* assert: reliable signals! */
#ifdef SIGWINCH
	handle_winch(SIGWINCH);
	if (isatty(0)) signal(SIGWINCH, handle_winch);
#endif
	signal(SIGHUP, signal_hup);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, signal_int);
#ifdef _POSIX_SOURCE
	if (status = sigsetjmp(env, 1))
#else
	if (status = setjmp(env))
#endif
	{
		fputs("\n?\n", stderr);
		sprintf(errmsg, "interrupt");
	} else {
		init_buffers();
		sigactive = 1;			/* enable signal handlers */
		if (argc && **argv && is_legal_filename(*argv)) {
			if (read_file(0, *argv) < 0 && !isatty(0))
				quit(2);
			else if (**argv != '!')
				strcpy(old_filename, *argv);
		} else if (argc) {
			fputs("?\n", stderr);
			if (**argv == '\0')
				sprintf(errmsg, "invalid filename");
			if (!isatty(0))
				quit(2);
		}
	}
	for (;;) {
		if (status < 0 && garrulous)
			fprintf(stderr, "%s\n", errmsg);
		if (prompt) {
			printf("%s", prompt);
			fflush(stdout);
		}
		if ((n = get_input_line()) < 0) {
			status = ERR;
			continue;
		} else if (n == 0) {
			if (modified && !scripted) {
				fputs("?\n", stderr);
				sprintf(errmsg, "warning: file modified");
				if (!isatty(0)) {
					fprintf(stderr, garrulous ? 
					    "script, line %d: %s\n" :
					    "", lineno, errmsg);
					quit(2);
				}
				clearerr(stdin);
				modified = 0;
				status = EMOD;
				continue;
			} else
				quit(0);
		} else if (ibuf[n - 1] != '\n') {
			/* discard line */
			sprintf(errmsg, "unexpected end-of-file");
			clearerr(stdin);
			status = ERR;
			continue;
		}
		isglobal = 0;
		if ((status = extract_addr_range()) >= 0 &&
		    (status = exec_command()) >= 0)
			if (!status || status &&
			    (status = display_lines(current_addr, current_addr,
			        status)) >= 0)
				continue;
		switch (status) {
		case EOF:
			quit(0);
		case EMOD:
			modified = 0;
			fputs("?\n", stderr);		/* give warning */
			sprintf(errmsg, "warning: file modified");
			if (!isatty(0)) {
				fprintf(stderr, garrulous ? 
				    "script, line %d: %s\n" : 
				    "", lineno, errmsg);
				quit(2);
			}
			break;
		case FATAL:
			if (!isatty(0))
				fprintf(stderr, garrulous ? 
				    "script, line %d: %s\n" : 
				    "", lineno, errmsg);
			else
				fprintf(stderr, garrulous ? "%s\n" : 
				    "", errmsg);
			quit(3);
		default:
			fputs("?\n", stderr);
			if (!isatty(0)) {
				fprintf(stderr, garrulous ? 
				    "script, line %d: %s\n" : 
				    "", lineno, errmsg);
				quit(2);
			}
			break;
		}
	}
	/*NOTREACHED*/
}


long first_addr, second_addr, addr_cnt;

/* extract_addr_range: get line addresses from the command buffer until an 
   illegal address is seen; return status */
int
extract_addr_range()
{
	long addr;

	addr_cnt = 0;
	first_addr = second_addr = current_addr;
	while ((addr = next_addr()) >= 0) {
		addr_cnt++;
		first_addr = second_addr;
		second_addr = addr;
		if (*ibufp != ',' && *ibufp != ';')
			break;
		else if (*ibufp++ == ';')
			current_addr = addr;
	}
	if ((addr_cnt = min(addr_cnt, 2)) == 1 || second_addr != addr)
		first_addr = second_addr;
	return (addr == ERR) ? ERR : 0;
}


#define MUST_BE_FIRST() \
	if (!first) { sprintf(errmsg, "invalid address"); return ERR; }

/*  next_addr: return the next line address in the command buffer */
long
next_addr()
{
	char *hd;
	long addr = current_addr;
	int first = 1;
	int c, n;

	SKIPBLANKS();
	for (hd = ibufp;; first = 0)
		switch (c = *ibufp) {
		case '+':
		case '\t':
		case ' ':
		case '-':
		case '^':
			ibufp++;
			SKIPBLANKS();
			n = (c == '-' || c == '^') ? -1 : 1;
			if (isdigit(*ibufp))
				addr += n * strtol(ibufp, &ibufp, 10);
			else if (!isspace(c))
				addr += n;
			break;
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			MUST_BE_FIRST();
			addr = strtol(ibufp, &ibufp, 10);
			break;
		case '.':
		case '$':
			MUST_BE_FIRST();
			ibufp++;
			addr = (c == '.') ? current_addr : addr_last;
			break;
		case '/':
		case '?':
			MUST_BE_FIRST();
			if ((addr = get_matching_node_addr(
			    get_compiled_pattern(), c == '/')) < 0)
				return ERR;
			else if (c == *ibufp)
				ibufp++;
			break;
		case '\'':
			MUST_BE_FIRST();
			ibufp++;
			if ((addr = get_marked_node_addr(*ibufp++)) < 0)
				return ERR;
			break;
		case '%':
		case ',':
		case ';':
			if (first) {
				ibufp++;
				addr_cnt++;
				second_addr = (c == ';') ? current_addr : 1;
				addr = addr_last;
				break;
			}
			/* FALL THROUGH */
		default:
			if (ibufp == hd)
				return EOF;
			else if (addr < 0 || addr_last < addr) {
				sprintf(errmsg, "invalid address");
				return ERR;
			} else
				return addr;
		}
	/* NOTREACHED */
}


#ifdef BACKWARDS
/* GET_THIRD_ADDR: get a legal address from the command buffer */
#define GET_THIRD_ADDR(addr) \
{ \
	long ol1, ol2; \
\
	ol1 = first_addr, ol2 = second_addr; \
	if (extract_addr_range() < 0) \
		return ERR; \
	else if (addr_cnt == 0) { \
		sprintf(errmsg, "destination expected"); \
		return ERR; \
	} else if (second_addr < 0 || addr_last < second_addr) { \
		sprintf(errmsg, "invalid address"); \
		return ERR; \
	} \
	addr = second_addr; \
	first_addr = ol1, second_addr = ol2; \
}
#else	/* BACKWARDS */
/* GET_THIRD_ADDR: get a legal address from the command buffer */
#define GET_THIRD_ADDR(addr) \
{ \
	long ol1, ol2; \
\
	ol1 = first_addr, ol2 = second_addr; \
	if (extract_addr_range() < 0) \
		return ERR; \
	if (second_addr < 0 || addr_last < second_addr) { \
		sprintf(errmsg, "invalid address"); \
		return ERR; \
	} \
	addr = second_addr; \
	first_addr = ol1, second_addr = ol2; \
}
#endif

/* gflags */
#define GLB 001		/* global command */
#define GPR 002		/* print after command */
#define GLS 004		/* list after command */
#define GNP 010		/* enumerate after command */
#define GSG 020		/* global substitute */


/* GET_COMMAND_SUFFIX: verify the command suffix in the command buffer */
#define GET_COMMAND_SUFFIX() { \
	int done = 0; \
	do { \
		switch(*ibufp) { \
		case 'p': \
			gflag |= GPR, ibufp++; \
			break; \
		case 'l': \
			gflag |= GLS, ibufp++; \
			break; \
		case 'n': \
			gflag |= GNP, ibufp++; \
			break; \
		default: \
			done++; \
		} \
	} while (!done); \
	if (*ibufp++ != '\n') { \
		sprintf(errmsg, "invalid command suffix"); \
		return ERR; \
	} \
}


/* sflags */
#define SGG 001		/* complement previous global substitute suffix */
#define SGP 002		/* complement previous print suffix */
#define SGR 004		/* use last regex instead of last pat */
#define SGF 010		/* newline found */

long u_current_addr = -1;	/* if >= 0, undo enabled */
long u_addr_last = -1;	/* if >= 0, undo enabled */
int patlock = 0;	/* if set, pattern not freed by get_compiled_pattern() */

long rows = 22;		/* scroll length: ws_row - 2 */

/* exec_command: execute the next command in command buffer; return print
   request, if any */
int
exec_command()
{
	static pattern_t *pat = NULL;
	static int sgflag = 0;

	pattern_t *tpat;
	char *fnp;
	int gflag = 0;
	int sflags = 0;
	long addr = 0;
	int n = 0;
	int c;

	SKIPBLANKS();
	switch(c = *ibufp++) {
	case 'a':
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (append_lines(second_addr) < 0)
			return ERR;
		break;
	case 'c':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (delete_lines(first_addr, second_addr) < 0 ||
		    append_lines(current_addr) < 0)
			return ERR;
		break;
	case 'd':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (delete_lines(first_addr, second_addr) < 0)
			return ERR;
		else if ((addr = INC_MOD(current_addr, addr_last)) != 0)
			current_addr = addr;
		modified = 1;
		break;
	case 'e':
		if (modified && !scripted)
			return EMOD;
		/* fall through */
	case 'E':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		} else if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		} else if ((fnp = get_filename()) == NULL)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (delete_lines(1, addr_last) < 0)
			return ERR;
		clear_undo_stack();
		if (close_sbuf() < 0)
			return ERR;
		else if (open_sbuf() < 0)
			return FATAL;
		if (*fnp && *fnp != '!') strcpy(old_filename, fnp);
#ifdef BACKWARDS
		if (*fnp == '\0' && *old_filename == '\0') {
			sprintf(errmsg, "no current filename");
			return ERR;
		}
#endif
		if (read_file(0, *fnp ? fnp : old_filename) < 0)
			return ERR;
		clear_undo_stack();
		modified = 0;
		u_current_addr = u_addr_last = -1;
		break;
	case 'f':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		} else if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		} else if ((fnp = get_filename()) == NULL)
			return ERR;
		else if (*fnp == '!') {
			sprintf(errmsg, "invalid redirection");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (*fnp) strcpy(old_filename, fnp);
		printf("%s\n", strip_escapes(old_filename));
		break;
	case 'g':
	case 'v':
	case 'G':
	case 'V':
		if (isglobal) {
			sprintf(errmsg, "cannot nest global commands");
			return ERR;
		} else if (check_addr_range(1, addr_last) < 0)
			return ERR;
		else if (build_active_list(c == 'g' || c == 'G') < 0)
			return ERR;
		else if (n = (c == 'G' || c == 'V'))
			GET_COMMAND_SUFFIX();
		isglobal++;
		if (exec_global(n, gflag) < 0)
			return ERR; 
		break;
	case 'h':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (*errmsg) fprintf(stderr, "%s\n", errmsg);
		break;
	case 'H':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if ((garrulous = 1 - garrulous) && *errmsg)
			fprintf(stderr, "%s\n", errmsg);
		break;
	case 'i':
		if (second_addr == 0) {
			sprintf(errmsg, "invalid address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (append_lines(second_addr - 1) < 0)
			return ERR;
		break;
	case 'j':
		if (check_addr_range(current_addr, current_addr + 1) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (first_addr != second_addr &&
		    join_lines(first_addr, second_addr) < 0)
			return ERR;
		break;
	case 'k':
		c = *ibufp++;
		if (second_addr == 0) {
			sprintf(errmsg, "invalid address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (mark_line_node(get_addressed_line_node(second_addr), c) < 0)
			return ERR;
		break;
	case 'l':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (display_lines(first_addr, second_addr, gflag | GLS) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'm':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_THIRD_ADDR(addr);
		if (first_addr <= addr && addr < second_addr) {
			sprintf(errmsg, "invalid destination");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (move_lines(addr) < 0)
			return ERR;
		else
			modified = 1;
		break;
	case 'n':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (display_lines(first_addr, second_addr, gflag | GNP) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'p':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (display_lines(first_addr, second_addr, gflag | GPR) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'P':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		prompt = prompt ? NULL : optarg ? optarg : dps;
		break;
	case 'q':
	case 'Q':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		gflag =  (modified && !scripted && c == 'q') ? EMOD : EOF;
		break;
	case 'r':
		if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		} else if (addr_cnt == 0)
			second_addr = addr_last;
		if ((fnp = get_filename()) == NULL)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (*old_filename == '\0' && *fnp != '!')
			strcpy(old_filename, fnp);
#ifdef BACKWARDS
		if (*fnp == '\0' && *old_filename == '\0') {
			sprintf(errmsg, "no current filename");
			return ERR;
		}
#endif
		if ((addr = read_file(second_addr, *fnp ? fnp : old_filename)) < 0)
			return ERR;
		else if (addr && addr != addr_last)
			modified = 1;
		break;
	case 's':
		do {
			switch(*ibufp) {
			case '\n':
				sflags |=SGF;
				break;
			case 'g':
				sflags |= SGG;
				ibufp++;
				break;
			case 'p':
				sflags |= SGP;
				ibufp++;
				break;
			case 'r':
				sflags |= SGR;
				ibufp++;
				break;
			default:
				if (sflags) {
					sprintf(errmsg, "invalid command suffix");
					return ERR;
				}
			}
		} while (sflags && *ibufp != '\n');
		if (sflags && !pat) {
			sprintf(errmsg, "no previous substitution");
			return ERR;
		} else if (!(sflags & SGF))
			sgflag &= 0xff;
		if (*ibufp != '\n' && *(ibufp + 1) == '\n') {
			sprintf(errmsg, "invalid pattern delimiter");
			return ERR;
		}
		tpat = pat;
		SPL1();
		if ((!sflags || (sflags & SGR)) &&
		    (tpat = get_compiled_pattern()) == NULL) {
		 	SPL0();
			return ERR;
		} else if (tpat != pat) {
			if (pat) {
				 regfree(pat);
				 free(pat);
			 }
			pat = tpat;
			patlock = 1;		/* reserve pattern */
		}
		SPL0();
		if (!sflags && (sgflag = extract_subst_tail()) < 0)
			return ERR;
		else if (isglobal)
			sgflag |= GLB;
		else
			sgflag &= ~GLB;
		if (sflags & SGG)
			sgflag ^= GSG;
		if (sflags & SGP)
			sgflag ^= GPR, sgflag &= ~(GLS | GNP);
		do {
			switch(*ibufp) {
			case 'p':
				sgflag |= GPR, ibufp++;
				break;
			case 'l':
				sgflag |= GLS, ibufp++;
				break;
			case 'n':
				sgflag |= GNP, ibufp++;
				break;
			default:
				n++;
			}
		} while (!n);
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if ((n = search_and_replace(pat, sgflag)) < 0)
			return ERR;
		else if (n)
			modified = 1;
		break;
	case 't':
		if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		GET_THIRD_ADDR(addr);
		GET_COMMAND_SUFFIX();
		if (!isglobal) clear_undo_stack();
		if (copy_lines(addr) < 0)
			return ERR;
		modified = 1;
		break;
	case 'u':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
		if (pop_undo_stack() < 0)
			return ERR;
		break;
	case 'w':
	case 'W':
		if ((n = *ibufp) == 'q' || n == 'Q') {
			gflag = EOF;
			ibufp++;
		}
		if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		} else if ((fnp = get_filename()) == NULL)
			return ERR;
		if (addr_cnt == 0 && !addr_last)
			first_addr = second_addr = 0;
		else if (check_addr_range(1, addr_last) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (*old_filename == '\0' && *fnp != '!')
			strcpy(old_filename, fnp);
#ifdef BACKWARDS
		if (*fnp == '\0' && *old_filename == '\0') {
			sprintf(errmsg, "no current filename");
			return ERR;
		}
#endif
		if ((addr = write_file(first_addr, second_addr,
		    *fnp ? fnp : old_filename, (c == 'W') ? "a" : "w")) < 0)
			return ERR;
		else if (addr == addr_last)
			modified = 0;
		else if (modified && !scripted && n == 'q')
			gflag = EMOD;
		break;
	case 'x':
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		GET_COMMAND_SUFFIX();
#ifdef DES
		des = get_keyword();
#else
		sprintf(errmsg, "crypt unavailable");
		return ERR;
#endif
		break;
	case 'z':
#ifdef BACKWARDS
		if (check_addr_range(first_addr = 1, current_addr + 1) < 0)
#else
		if (check_addr_range(first_addr = 1, current_addr + !isglobal) < 0)
#endif
			return ERR;
		else if ('0' < *ibufp && *ibufp <= '9')
			rows = strtol(ibufp, &ibufp, 10);
		GET_COMMAND_SUFFIX();
		if (display_lines(second_addr, min(addr_last,
		    second_addr + rows - 1), gflag) < 0)
			return ERR;
		gflag = 0;
		break;
	case '=':
		GET_COMMAND_SUFFIX();
		printf("%d\n", addr_cnt ? second_addr : addr_last);
		break;
	case '!':
#ifndef EX_BANG
		if (addr_cnt > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
#endif
		if ((sflags = get_shell_command()) < 0)
			return ERR;
		GET_COMMAND_SUFFIX();
		if (sflags) printf("%s\n", shcmd + 1);
#ifdef EX_BANG
		if (addr_cnt == 0) {
#endif
			system(shcmd + 1);
			if (!scripted) printf("!\n");
			break;
#ifdef EX_BANG
		}
		if (!addr_last && !first_addr && !second_addr) {
			if (!isglobal) clear_undo_stack();
		} else if (check_addr_range(current_addr, current_addr) < 0)
			return ERR;
		else {
			if (!isglobal) clear_undo_stack();
			if (delete_lines(first_addr, second_addr) < 0)
				return ERR;
			second_addr = current_addr;
			modified = 1;
		}
		if ((addr = read_file(second_addr, shcmd)) < 0)
			return ERR;
		else if (addr && addr != addr_last)
			modified = 1;
		break;
#endif
	case '\n':
#ifdef BACKWARDS
		if (check_addr_range(first_addr = 1, current_addr + 1) < 0
#else
		if (check_addr_range(first_addr = 1, current_addr + !isglobal) < 0
#endif
		 || display_lines(second_addr, second_addr, 0) < 0)
			return ERR;
		break;
	default:
		sprintf(errmsg, "unknown command");
		return ERR;
	}
	return gflag;
}


/* check_addr_range: return status of address range check */
int
check_addr_range(n, m)
	long n, m;
{
	if (addr_cnt == 0) {
		first_addr = n;
		second_addr = m;
	}
	if (first_addr > second_addr || 1 > first_addr ||
	    second_addr > addr_last) {
		sprintf(errmsg, "invalid address");
		return ERR;
	}
	return 0;
}


/* build_active_list:  add line matching a pattern to the global-active list */
int
build_active_list(isgcmd)
	int isgcmd;
{
	pattern_t *pat;
	line_t *lp;
	long n;
	char *s;
	char delimiter;

	if ((delimiter = *ibufp) == ' ' || delimiter == '\n') {
		sprintf(errmsg, "invalid pattern delimiter");
		return ERR;
	} else if ((pat = get_compiled_pattern()) == NULL)
		return ERR;
	else if (*ibufp == delimiter)
		ibufp++;
	clear_active_list();
	lp = get_addressed_line_node(1);
	for (n = 1; n <= addr_last; n++, lp = lp->next) {
		if ((s = get_sbuf_line(lp)) == NULL)
			return ERR;
		if (isbinary)
			s = NUL_TO_NEWLINE(s, lp->len);
		if (first_addr <= n && n <= second_addr &&
		    !regexec(pat, s, 0, NULL, 0) == isgcmd)
			if (set_active_node(lp) < 0)
				return ERR;
	}
	return 0;
}


/* exec_global: apply command list in the command buffer to the active
   lines in a range; return command status */
long
exec_global(interact, gflag)
	int interact;
	int gflag;
{
	static char *ocmd = NULL;
	static int ocmdsz = 0;

	line_t *lp = NULL;
	int status;
	int n;
	char *cmd = NULL;

#ifdef BACKWARDS
	if (!interact)
		if (!strcmp(ibufp, "\n"))
			cmd = "p\n";		/* null cmd-list == `p' */
		else if ((cmd = get_extended_line(&n, 0)) == NULL)
			return ERR;
#else
	if (!interact && (cmd = get_extended_line(&n, 0)) == NULL)
		return ERR;
#endif
	clear_undo_stack();
	while ((lp = next_active_node()) != NULL) {
		if ((current_addr = get_line_node_addr(lp)) < 0)
			return ERR;
		if (interact) {
			/* print current_addr; get a command in global syntax */
			if (display_lines(current_addr, current_addr, gflag) < 0)
				return ERR;
			while ((n = get_input_line()) > 0 &&
			    ibuf[n - 1] != '\n')
				clearerr(stdin);
			if (n < 0)
				return ERR;
			else if (n == 0) {
				sprintf(errmsg, "unexpected end-of-file");
				return ERR;
			} else if (n == 1 && !strcmp(ibuf, "\n"))
				continue;
			else if (n == 2 && !strcmp(ibuf, "&\n")) {
				if (cmd == NULL) {
					sprintf(errmsg, "no previous command");
					return ERR;
				} else cmd = ocmd;
			} else if ((cmd = get_extended_line(&n, 0)) == NULL)
				return ERR;
			else {
				CKBUF(ocmd, ocmdsz, n + 1, ERR);
				memcpy(ocmd, cmd, n + 1);
				cmd = ocmd;
			}

		}
		ibufp = cmd;
		for (; *ibufp;)
			if ((status = extract_addr_range()) < 0 ||
			    (status = exec_command()) < 0 ||
			    status > 0 && (status = display_lines(
			    current_addr, current_addr, status)) < 0)
				return status;
	}
	return 0;
}


/* get_matching_node_addr: return the address of the next line matching a 
   pattern in a given direction.  wrap around begin/end of line queue if
   necessary */
long
get_matching_node_addr(pat, dir)
	pattern_t *pat;
	int dir;
{
	char *s;
	long n = current_addr;
	line_t *lp;

	if (!pat) return ERR;
	do {
		if (n = dir ? INC_MOD(n, addr_last) : DEC_MOD(n, addr_last)) {
			lp = get_addressed_line_node(n);
			if ((s = get_sbuf_line(lp)) == NULL)
				return ERR;
			if (isbinary)
				s = NUL_TO_NEWLINE(s, lp->len);
			if (!regexec(pat, s, 0, NULL, 0))
				return n;
		}
	} while (n != current_addr);
	sprintf(errmsg, "no match");
	return  ERR;
}


/* get_filename: return pointer to copy of filename in the command buffer */
char *
get_filename()
{
	static char *file = NULL;
	static int filesz = 0;

	int n;

	if (*ibufp != '\n') {
		SKIPBLANKS();
		if (*ibufp == '\n') {
			sprintf(errmsg, "invalid filename");
			return NULL;
		} else if ((ibufp = get_extended_line(&n, 1)) == NULL)
			return NULL;
#ifdef EX_BANG
		else if (*ibufp == '!') {
			ibufp++;
			if ((n = get_shell_command()) < 0)
				return NULL;
			if (n) printf("%s\n", shcmd + 1);
			return shcmd;
		}
#endif
		else if (n - 1 > MAXFNAME) {
			sprintf(errmsg, "filename too long");
			return  NULL;
		}
	}
#ifndef BACKWARDS
	else if (*old_filename == '\0') {
		sprintf(errmsg, "no current filename");
		return  NULL;
	}
#endif
	CKBUF(file, filesz, MAXFNAME + 1, NULL);
	for (n = 0; *ibufp != '\n';)
		file[n++] = *ibufp++;
	file[n] = '\0';
	return is_legal_filename(file) ? file : NULL;
}


/* extract_subst_tail: extract substitution tail from the command buffer */
int
extract_subst_tail()
{
	char delimiter;

	if ((delimiter = *ibufp) == '\n') {
		rhbufi = 0;
		return GPR;
	} else if (extract_subst_template() == NULL)
		return  ERR;
	else if (*ibufp == '\n')
		return GPR;
	else if (*ibufp == delimiter)
		ibufp++;
	if ('1' <= *ibufp && *ibufp <= '9')
		return (int) strtol(ibufp, &ibufp, 10) << 8;
	else if (*ibufp == 'g') {
		ibufp++;
		return GSG;
	}
	return  0;
}


/* extract_subst_template: return pointer to copy of substitution template
   in the command buffer */
char *
extract_subst_template()
{
	int n = 0;
	int i = 0;
	char c;
	char delimiter = *ibufp++;

	if (*ibufp == '%' && *(ibufp + 1) == delimiter) {
		ibufp++;
		if (!rhbuf) sprintf(errmsg, "no previous substitution");
		return rhbuf;
	}
	while (*ibufp != delimiter) {
		CKBUF(rhbuf, rhbufsz, i + 2, NULL);
		if ((c = rhbuf[i++] = *ibufp++) == '\n' && *ibufp == '\0') {
			i--, ibufp--;
			break;
		} else if (c != '\\')
			;
		else if ((rhbuf[i++] = *ibufp++) != '\n')
			;
		else if (!isglobal) {
			while ((n = get_input_line()) == 0 ||
			    n > 0 && ibuf[n - 1] != '\n')
				clearerr(stdin);
			if (n < 0)
				return NULL;
		} else
			/*NOTREACHED*/
			;
	}
	CKBUF(rhbuf, rhbufsz, i + 1, NULL);
	rhbuf[rhbufi = i] = '\0';
	return  rhbuf;
}


/* get_shell_command: read a shell command from stdin; return substitution
   status */
int
get_shell_command()
{
	static char *buf = NULL;
	static int n = 0;

	char *s;			/* substitution char pointer */
	int i = 0;
	int j = 0;

	if (red) {
		sprintf(errmsg, "shell access restricted");
		return ERR;
	} else if ((s = ibufp = get_extended_line(&j, 1)) == NULL)
		return ERR;
	CKBUF(buf, n, j + 1, ERR);
	buf[i++] = '!';			/* prefix command w/ bang */
	while (*ibufp != '\n')
		switch (*ibufp) {
		default:
			CKBUF(buf, n, i + 2, ERR);
			buf[i++] = *ibufp;
			if (*ibufp++ == '\\')
				buf[i++] = *ibufp++;
			break;
		case '!':
			if (s != ibufp) {
				CKBUF(buf, n, i + 1, ERR);
				buf[i++] = *ibufp++;
			}
#ifdef BACKWARDS
			else if (shcmd == NULL || *(shcmd + 1) == '\0')
#else
			else if (shcmd == NULL)
#endif
			{
				sprintf(errmsg, "no previous command");
				return ERR;
			} else {
				CKBUF(buf, n, i + shcmdi, ERR);
				for (s = shcmd + 1; s < shcmd + shcmdi;)
					buf[i++] = *s++;
				s = ibufp++;
			}
			break;
		case '%':
			if (*old_filename  == '\0') {
				sprintf(errmsg, "no current filename");
				return ERR;
			}
			j = strlen(s = strip_escapes(old_filename));
			CKBUF(buf, n, i + j, ERR);
			while (j--)
				buf[i++] = *s++;
			s = ibufp++;
			break;
		}
	CKBUF(shcmd, shcmdsz, i + 1, ERR);
	memcpy(shcmd, buf, i);
	shcmd[shcmdi = i] = '\0';
	return *s == '!' || *s == '%';
}


/* append_lines: insert text from stdin to after line n; stop when either a
   single period is read or EOF; return status */
int
append_lines(n)
	long n;
{
	int l;
	char *lp = ibuf;
	char *eot;
	undo_t *up = NULL;

	for (current_addr = n;;) {
		if (!isglobal) {
			if ((l = get_input_line()) < 0)
				return ERR;
			else if (l == 0 || ibuf[l - 1] != '\n') {
				clearerr(stdin);
				return  l ? EOF : 0;
			}
			lp = ibuf;
		} else if (*(lp = ibufp) == '\0')
			return 0;
		else {
			while (*ibufp++ != '\n')
				;
			l = ibufp - lp;
		}
		if (l == 2 && lp[0] == '.' && lp[1] == '\n') {
			return 0;
		}
		eot = lp + l;
		SPL1();
		do {
			if ((lp = put_sbuf_line(lp)) == NULL) {
				SPL0();
				return ERR;
			} else if (up)
				up->t = get_addressed_line_node(current_addr);
			else if ((up = push_undo_stack(UADD, current_addr,
			    current_addr)) == NULL) {
				SPL0();
				return ERR;
			}
		} while (lp != eot);
		SPL0();
		modified = 1;
	}
	/* NOTREACHED */
}


/* search_and_replace: for each line in a range, change text matching a pattern
   according to a substitution template; return status  */
int
search_and_replace(pat, gflag)
	pattern_t *pat;
	int gflag;
{
	undo_t *up;
	char *txt;
	char *eot;
	long lc;
	int nsubs = 0;
	line_t *lp;
	int len;

	current_addr = first_addr - 1;
	for (lc = 0; lc <= second_addr - first_addr; lc++) {
		lp = get_addressed_line_node(++current_addr);
		if ((len = substitute_matching_text(pat, lp, gflag)) < 0)
			return ERR;
		else if (len) {
			up = NULL;
			if (delete_lines(current_addr, current_addr) < 0)
				return ERR;
			txt = rbuf;
			eot = rbuf + len;
			SPL1();
			do {
				if ((txt = put_sbuf_line(txt)) == NULL) {
					SPL0();
					return ERR;
				} else if (up)
					up->t = get_addressed_line_node(current_addr);
				else if ((up = push_undo_stack(UADD,
				    current_addr, current_addr)) == NULL) {
					SPL0();
					return ERR;
				}
			} while (txt != eot);
			SPL0();
			nsubs++;
		}
	}
	if  (nsubs == 0 && !(gflag & GLB)) {
		sprintf(errmsg, "no match");
		return ERR;
	} else if ((gflag & (GPR | GLS | GNP)) &&
	    display_lines(current_addr, current_addr, gflag) < 0)
		return ERR;
	return 1;
}


/* substitute_matching_text: replace text matched by a pattern according to
   a substitution template; return pointer to the modified text */
int
substitute_matching_text(pat, lp, gflag)
	pattern_t *pat;
	line_t *lp;
	int gflag;
{
	int off = 0;
	int kth = gflag >> 8;		/* substitute kth match only */
	int changed = 0;
	int matchno = 0;
	int i = 0;
	regmatch_t rm[SE_MAX];
	char *txt;
	char *eot;

	if ((txt = get_sbuf_line(lp)) == NULL)
		return ERR;
	if (isbinary) txt = NUL_TO_NEWLINE(txt, lp->len);
	eot = txt + lp->len;
	if (!regexec(pat, txt, SE_MAX, rm, 0)) {
		do {
			if (!kth || kth == ++matchno) {
				changed++;
				i = rm[0].rm_so;
				CKBUF(rbuf, rbufsz, off + i, ERR);
				if (isbinary)
					txt = NEWLINE_TO_NUL(txt, rm[0].rm_eo);
				memcpy(rbuf + off, txt, i);
				off += i;
				if ((off = apply_subst_template(txt, rm, off,
				    pat->re_nsub)) < 0)
					return ERR;
			} else {
				i = rm[0].rm_eo;
				CKBUF(rbuf, rbufsz, off + i, ERR);
				if (isbinary)
					txt = NEWLINE_TO_NUL(txt, i);
				memcpy(rbuf + off, txt, i);
				off += i;
			}
			txt += rm[0].rm_eo;
		} while (*txt && (!changed || (gflag & GSG) && rm[0].rm_eo) &&
		    !regexec(pat, txt, SE_MAX, rm, REG_NOTBOL));
		i = eot - txt;
		CKBUF(rbuf, rbufsz, off + i + 2, ERR);
		if (i > 0 && !rm[0].rm_eo && (gflag & GSG)) {
			sprintf(errmsg, "infinite substitution loop");
			return  ERR;
		}
		if (isbinary)
			txt = NEWLINE_TO_NUL(txt, i);
		memcpy(rbuf + off, txt, i);
		memcpy(rbuf + off + i, "\n", 2);
	}
	return changed ? off + i + 1 : 0;
}


/* join_lines: replace a range of lines with the joined text of those lines */
int
join_lines(from, to)
	long from;
	long to;
{
	static char *buf = NULL;
	static int n;

	char *s;
	int size = 0;
	line_t *bp, *ep;

	ep = get_addressed_line_node(INC_MOD(to, addr_last));
	bp = get_addressed_line_node(from);
	for (; bp != ep; bp = bp->next) {
		if ((s = get_sbuf_line(bp)) == NULL)
			return ERR;
		CKBUF(buf, n, size + bp->len, ERR);
		memcpy(buf + size, s, bp->len);
		size += bp->len;
	}
	CKBUF(buf, n, size + 2, ERR);
	memcpy(buf + size, "\n", 2);
	if (delete_lines(from, to) < 0)
		return ERR;
	current_addr = from - 1;
	SPL1();
	if (put_sbuf_line(buf) == NULL ||
	    push_undo_stack(UADD, current_addr, current_addr) == NULL) {
		SPL0();
		return ERR;
	}
	SPL0();
	modified = 1;
	return 0;
}


/* move_lines: move a range of lines */
int
move_lines(addr)
	long addr;
{
	line_t *b1, *a1, *b2, *a2;
	long n = INC_MOD(second_addr, addr_last);
	long p = first_addr - 1;
	int done = (addr == first_addr - 1 || addr == second_addr);

	SPL1();
	if (done) {
		a2 = get_addressed_line_node(n);
		b2 = get_addressed_line_node(p);
		current_addr = second_addr;
	} else if (push_undo_stack(UMOV, p, n) == NULL ||
	    push_undo_stack(UMOV, addr, INC_MOD(addr, addr_last)) == NULL) {
		SPL0();
		return ERR;
	} else {
		a1 = get_addressed_line_node(n);
		if (addr < first_addr) {
			b1 = get_addressed_line_node(p);
			b2 = get_addressed_line_node(addr);
					/* this get_addressed_line_node last! */
		} else {
			b2 = get_addressed_line_node(addr);
			b1 = get_addressed_line_node(p);
					/* this get_addressed_line_node last! */
		}
		a2 = b2->next;
		requeue(b2, b1->next);
		requeue(a1->prev, a2);
		requeue(b1, a1);
		current_addr = addr + ((addr < first_addr) ? 
		    second_addr - first_addr + 1 : 0);
	}
	if (isglobal)
		unset_active_nodes(b2->next, a2);
	SPL0();
	return 0;
}


/* copy_lines: copy a range of lines; return status */
int
copy_lines(addr)
	long addr;
{
	line_t *lp, *np = get_addressed_line_node(first_addr);
	undo_t *up = NULL;
	long n = second_addr - first_addr + 1;
	long m = 0;

	current_addr = addr;
	if (first_addr <= addr && addr < second_addr) {
		n =  addr - first_addr + 1;
		m = second_addr - addr;
	}
	for (; n > 0; n=m, m=0, np = get_addressed_line_node(current_addr + 1))
		for (; n-- > 0; np = np->next) {
			SPL1();
			if ((lp = dup_line_node(np)) == NULL) {
				SPL0();
				return ERR;
			}
			add_line_node(lp);
			if (up)
				up->t = lp;
			else if ((up = push_undo_stack(UADD, current_addr,
			    current_addr)) == NULL) {
				SPL0();
				return ERR;
			}
			SPL0();
		}
	return 0;
}


/* delete_lines: delete a range of lines */
int
delete_lines(from, to)
	long from, to;
{
	line_t *n, *p;

	SPL1();
	if (push_undo_stack(UDEL, from, to) == NULL) {
		SPL0();
		return ERR;
	}
	n = get_addressed_line_node(INC_MOD(to, addr_last));
	p = get_addressed_line_node(from - 1);
					/* this get_addressed_line_node last! */
	if (isglobal)
		unset_active_nodes(p->next, n);
	requeue(p, n);
	addr_last -= to - from + 1;
	current_addr = from - 1;
	SPL0();
	return 0;
}


/* apply_subst_template: modify text according to a substitution template;
   return offset to end of modified text */
int
apply_subst_template(boln, rm, off, re_nsub)
	char *boln;
	regmatch_t *rm;
	int off;
	int re_nsub;
{
	int j = 0;
	int k = 0;
	int n;
	char *sub = rhbuf;

	for (; sub - rhbuf < rhbufi; sub++)
		if (*sub == '&') {
			j = rm[0].rm_so;
			k = rm[0].rm_eo;
			CKBUF(rbuf, rbufsz, off + k - j, ERR);
			while (j < k)
				rbuf[off++] = boln[j++];
		} else if (*sub == '\\' && '1' <= *++sub && *sub <= '9' &&
		    (n = *sub - '0') <= re_nsub) {
			j = rm[n].rm_so;
			k = rm[n].rm_eo;
			CKBUF(rbuf, rbufsz, off + k - j, ERR);
			while (j < k)
				rbuf[off++] = boln[j++];
		} else {
			CKBUF(rbuf, rbufsz, off + 1, ERR);
			rbuf[off++] = *sub;
		}
	CKBUF(rbuf, rbufsz, off + 1, ERR);
	rbuf[off] = '\0';
	return off;
}

/* display_lines: print a range of lines to stdout */
int
display_lines(from, to, gflag)
	long from;
	long to;
	int gflag;
{
	line_t *bp;
	line_t *ep;
	char *s;

	if (!from) {
		sprintf(errmsg, "invalid address");
		return ERR;
	}
	ep = get_addressed_line_node(INC_MOD(to, addr_last));
	bp = get_addressed_line_node(from);
	for (; bp != ep; bp = bp->next) {
		if ((s = get_sbuf_line(bp)) == NULL)
			return ERR;
		output_line(s, bp->len, current_addr = from++, gflag);
	}
	return 0;
}


#define ESCAPES "\a\b\f\n\r\t\v\\"
#define ESCCHARS "abfnrtv\\"

int cols = 72;		/* wrap column: ws_col - 8 */

/* output_line: print text to stdout */
void
output_line(s, l, n, gflag)
	char *s;
	int l;
	long n;
	int gflag;
{
	int col = 0;
	char *cp;

	if (gflag & GNP) {
		printf("%ld\t", n);
		col = 8;
	}
	for (; l--; s++) {
		if ((gflag & GLS) && ++col > cols) {
			fputs("\\\n", stdout);
			col = 1;
		}
		if (gflag & GLS) {
			if (31 < *s && *s < 127 && *s != '\\')
				putchar(*s);
			else {
				putchar('\\');
				col++;
				if (*s && (cp = strchr(ESCAPES, *s)) != NULL)
					putchar(ESCCHARS[cp - ESCAPES]);
				else {
					putchar((((unsigned char) *s & 0300) >> 6) + '0');
					putchar((((unsigned char) *s & 070) >> 3) + '0');
					putchar(((unsigned char) *s & 07) + '0');
					col += 2;
				}
			}

		} else
			putchar(*s);
	}
#ifndef BACKWARDS
	if (gflag & GLS)
		putchar('$');
#endif
	putchar('\n');
}


int newline_added;		/* set if newline appended to input file */

/* read_file: read a text file into the editor buffer; return line count */
long
read_file(n, fn)
	long n;
	char *fn;
{
	FILE *fp;
	line_t *lp = get_addressed_line_node(n);
	unsigned long size = 0;
	undo_t *up = NULL;
	int len;

	isbinary = newline_added = 0;
	if ((fp = (*fn == '!') ? popen(fn + 1, "r") :
	    fopen(strip_escapes(fn), "r")) == NULL) {
		fprintf(stderr, "%s: %s\n", fn, strerror(errno));
		sprintf(errmsg, "cannot open input file");
		return ERR;
	} else if (des)
		init_des_cipher();
	for (current_addr = n; (len = get_file_line(fp)) > 0; size += len) {
		SPL1();
		if (put_sbuf_line(sbuf) == NULL) {
			SPL0();
			return ERR;
		}
		lp = lp->next;
		if (up)
			up->t = lp;
		else if ((up = push_undo_stack(UADD, current_addr,
		    current_addr)) == NULL) {
			SPL0();
			return ERR;
		}
		SPL0();
	}
	if (((*fn == '!') ?  pclose(fp) : fclose(fp)) < 0) {
		fprintf(stderr, "%s: %s\n", fn, strerror(errno));
		sprintf(errmsg, "cannot close input file");
		return ERR;
	}
	if (newline_added && !isbinary)
		fputs("newline appended\n", stderr);
	if (des) size += 8 - size % 8;
	fprintf(stderr, !scripted ? "%lu\n" : "", size);
	return  (len < 0) ? ERR : current_addr - n;
}


/* write_file: write a range of lines to a file; return line count */
long
write_file(n, m, fn, mode)
	long n;
	long m;
	char *fn;
	char *mode;
{
	FILE *fp;
	line_t *lp;
	unsigned long size = 0;
	long lc = n ? m - n + 1 : 0;
	char *s = NULL;
	int len;
	int ct;

	if ((fp = ((*fn == '!') ? popen(fn + 1, "w") : fopen(strip_escapes(fn),
	    mode))) == NULL) {
		fprintf(stderr, "%s: %s\n", fn, strerror(errno));
		sprintf(errmsg, "cannot open output file");
		return ERR;
	} else if (des)
		init_des_cipher();
	if (n && !des)
		for (lp = get_addressed_line_node(n); n <= m; n++, lp = lp->next) {
			if ((s = get_sbuf_line(lp)) == NULL)
				return ERR;
			len = lp->len;
			if (n != addr_last || !isbinary || !newline_added)
				s[len++] = '\n';
			if ((ct = fwrite(s, sizeof(char), len, fp)) < 0 || ct != len) {
				fprintf(stderr, "%s: %s\n", fn, strerror(errno));
				sprintf(errmsg, "cannot write file");
				return ERR;
			}
			size += len;
		}
	else if (n)
		for (lp = get_addressed_line_node(n); n <= m; n++, lp = lp->next) {
			if ((s = get_sbuf_line(lp)) == NULL)
				return ERR;
			len = lp->len;
			while (len--) {
				if (put_des_char(*s++, fp) == EOF && ferror(fp)) {
					fprintf(stderr, "%s: %s\n", fn, strerror(errno));
					sprintf(errmsg, "cannot write file");
					return ERR;
				}
			}
			if (n != addr_last || !isbinary || !newline_added) {
				if (put_des_char('\n', fp) < 0) {
					fprintf(stderr, "%s: %s\n", fn, strerror(errno));
					sprintf(errmsg, "cannot write file");
					return ERR;
				}
				size++;			/* for '\n' */
			}
			size += lp->len;
		}
	if (des) {
		flush_des_file(fp);				/* flush buffer */
		size += 8 - size % 8;			/* adjust DES size */
	}
	if (((*fn == '!') ?  pclose(fp) : fclose(fp)) < 0) {
		fprintf(stderr, "%s: %s\n", fn, strerror(errno));
		sprintf(errmsg, "cannot close output file");
		return ERR;
	}
	fprintf(stderr, !scripted ? "%lu\n" : "", size);
	return  lc;
}


#define USIZE 100				/* undo stack size */
undo_t *ustack = NULL;				/* undo stack */
long usize = 0;					/* stack size variable */
long u_p = 0;					/* undo stack pointer */

/* push_undo_stack: return pointer to intialized undo node */
undo_t *
push_undo_stack(type, from, to)
	int type;
	long from;
	long to;
{
	undo_t *t;

#if defined(sun) || defined(NO_REALLOC_NULL)
	if (ustack == NULL &&
	    (ustack = (undo_t *) malloc((usize = USIZE) * sizeof(undo_t))) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "out of memory");
		return NULL;
	}
#endif
	t = ustack;
	if (u_p < usize ||
	    (t = (undo_t *) realloc(ustack, (usize += USIZE) * sizeof(undo_t))) != NULL) {
		ustack = t;
		ustack[u_p].type = type;
		ustack[u_p].t = get_addressed_line_node(to);
		ustack[u_p].h = get_addressed_line_node(from);
		return ustack + u_p++;
	}
	/* out of memory - release undo stack */
	fprintf(stderr, "%s\n", strerror(errno));
	sprintf(errmsg, "out of memory");
	clear_undo_stack();
	free(ustack);
	ustack = NULL;
	usize = 0;
	return NULL;
}


/* USWAP: swap undo nodes */
#define USWAP(x,y) { \
	undo_t utmp; \
	utmp = x, x = y, y = utmp; \
}


/* pop_undo_stack: undo last change to the editor buffer */
int
pop_undo_stack()
{
	long n;
	long o_current_addr = current_addr;
	long o_addr_last = addr_last;

	if (u_current_addr == -1 || u_addr_last == -1) {
		sprintf(errmsg, "nothing to undo");
		return ERR;
	} else if (u_p)
		modified = 1;
	get_addressed_line_node(0);	/* this get_addressed_line_node last! */
	SPL1();
	for (n = u_p; n-- > 0;) {
		switch(ustack[n].type) {
		case UADD:
			requeue(ustack[n].h->prev, ustack[n].t->next);
			break;
		case UDEL:
			requeue(ustack[n].h->prev, ustack[n].h);
			requeue(ustack[n].t, ustack[n].t->next);
			break;
		case UMOV:
		case VMOV:
			requeue(ustack[n - 1].h, ustack[n].h->next);
			requeue(ustack[n].t->prev, ustack[n - 1].t);
			requeue(ustack[n].h, ustack[n].t);
			n--;
			break;
		default:
			/*NOTREACHED*/
			;
		}
		ustack[n].type ^= 1;
	}
	/* reverse undo stack order */
	for (n = u_p; n-- > (u_p + 1)/ 2;)
		USWAP(ustack[n], ustack[u_p - 1 - n]);
	if (isglobal)
		clear_active_list();
	current_addr = u_current_addr, u_current_addr = o_current_addr;
	addr_last = u_addr_last, u_addr_last = o_addr_last;
	SPL0();
	return 0;
}


/* clear_undo_stack: clear the undo stack */
void
clear_undo_stack()
{
	line_t *lp, *ep, *tl;

	while (u_p--)
		if (ustack[u_p].type == UDEL) {
			ep = ustack[u_p].t->next;
			for (lp = ustack[u_p].h; lp != ep; lp = tl) {
				unmark_line_node(lp);
				tl = lp->next;
				free(lp);
			}
		}
	u_p = 0;
	u_current_addr = current_addr;
	u_addr_last = addr_last;
}


#define MAXMARK 26			/* max number of marks */

line_t	*mark[MAXMARK];			/* line markers */
int markno;				/* line marker count */

/* get_marked_node_addr: return address of a marked line */
long
get_marked_node_addr(n)
	int n;
{
	if (!islower(n)) {
		sprintf(errmsg, "invalid mark character");
		return ERR;
	}
	return get_line_node_addr(mark[n - 'a']);
}


/* mark_line_node: set a line node mark */
int
mark_line_node(lp, n)
	line_t *lp;
	int n;
{
	if (!islower(n)) {
		sprintf(errmsg, "invalid mark character");
		return ERR;
	} else if (mark[n - 'a'] == NULL)
		markno++;
	mark[n - 'a'] = lp;
	return 0;
}


/* unmark_line_node: clear line node mark */
void
unmark_line_node(lp)
	line_t *lp;
{
	int i;

	for (i = 0; markno && i < MAXMARK; i++)
		if (mark[i] == lp) {
			mark[i] = NULL;
			markno--;
		}
}


line_t **active_list;		/* list of lines active in a global command */
long active_last;		/* index of last active line in active_list */
long active_size;		/* size of active_list */
long active_ptr;		/* active_list index (non-decreasing) */
long active_ndx;		/* active_list index (modulo active_last) */


/* set_active_node: add a line node to the global-active list */
int
set_active_node(lp)
	line_t *lp;
{
	if (active_last + 1 > active_size) {
		int ti = active_size;
		line_t **ts;
		SPL1();
#if defined(sun) || defined(NO_REALLOC_NULL)
		if (active_list != NULL) {
#endif
			if ((ts = (line_t **) realloc(active_list, 
			    (ti += MINBUFSZ) * sizeof(line_t **))) == NULL) {
				fprintf(stderr, "%s\n", strerror(errno));
				sprintf(errmsg, "out of memory");
				SPL0();
				return ERR;
			}
#if defined(sun) || defined(NO_REALLOC_NULL)
		} else {
			if ((ts = (line_t **) malloc((ti += MINBUFSZ) * 
			    sizeof(line_t **))) == NULL) {
				fprintf(stderr, "%s\n", strerror(errno));
				sprintf(errmsg, "out of memory");
				SPL0();
				return ERR;
			}
		}
#endif
		active_size = ti;
		active_list = ts;
		SPL0();
	}
	active_list[active_last++] = lp;
	return 0;
}


/* unset_active_nodes: remove a range of lines from the global-active list */
void
unset_active_nodes(np, mp)
	line_t *np, *mp;
{
	line_t *lp;
	long i;

	for (lp = np; lp != mp; lp = lp->next)
		for (i = 0; i < active_last; i++)
			if (active_list[active_ndx] == lp) {
				active_list[active_ndx] = NULL;
				active_ndx = INC_MOD(active_ndx, active_last - 1);
				break;
			} else	active_ndx = INC_MOD(active_ndx, active_last - 1);
}


/* next_active_node: return the next global-active line node */
line_t *
next_active_node()
{
	while (active_ptr < active_last && active_list[active_ptr] == NULL)
		active_ptr++;
	return (active_ptr < active_last) ? active_list[active_ptr++] : NULL;
}


/* clear_active_list: clear the global-active list */
void
clear_active_list()
{
	SPL1();
	active_size = active_last = active_ptr = active_ndx = 0;
	free(active_list);
	active_list = NULL;
	SPL0();
}



/* get_file_line: read a line of text from a file; return line length */
int
get_file_line(fp)
	FILE *fp;
{
	register int c;
	register int i = 0;

	while (((c = des ? get_des_char(fp) : getc(fp)) != EOF || !feof(fp) &&
	    !ferror(fp)) && c != '\n') {
		CKBUF(sbuf, sbufsz, i + 1, ERR);
		if (!(sbuf[i++] = c)) isbinary = 1;
	}
	CKBUF(sbuf, sbufsz, i + 2, ERR);
	if (c == '\n')
		sbuf[i++] = c;
	else if (feof(fp) && i) {
		sbuf[i++] = '\n';
		newline_added = 1;
	} else if (ferror(fp)) {
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "cannot read input file");
		return ERR;
	}
	sbuf[i] = '\0';
	return (isbinary && newline_added && i) ? --i : i;
}


/* get_input_line: read a line of text from stdin; return line length */
int
get_input_line()
{
	register int i = 0;
	register int oi = 0;
	char c;

	/* Read one character at a time to avoid i/o contention with shell
	   escapes invoked by nonterminal input, e.g.,
	   ed - <<EOF
	   !cat
	   hello, world
	   EOF */
	for (;;)
		switch (read(0, &c, 1)) {
		default:
			oi = 0;
			CKBUF(ibuf, ibufsz, i + 2, ERR);
			if (!(ibuf[i++] = c)) isbinary = 1;
			if (c != '\n')
				continue;
			lineno++;		/* script line no. */
			ibuf[i] = '\0';
			ibufp = ibuf;
			return i;
		case 0:
			if (i != oi) {
				oi = i;
				continue;
			} else if (i)
				ibuf[i] = '\0';
			ibufp = ibuf;
			return i;
		case -1:
			fprintf(stderr, "%s\n", strerror(errno));
			sprintf(errmsg, "cannot read standard input");
			clearerr(stdin);
			ibufp = NULL;
			return ERR;
		}
}


/* get_extended_line: get a an extended line from stdin */
char *
get_extended_line(sizep, nonl)
	int *sizep;
	int nonl;
{
	int l, n;
	char *t = ibufp;

	while (*t++ != '\n')
		;
	if ((l = t - ibufp) < 2 || !has_trailing_escape(ibufp, ibufp + l - 1)) {
		*sizep = l;
		return ibufp;
	}
	*sizep = -1;
	CKBUF(cvbuf, cvbufsz, l, NULL);
	memcpy(cvbuf, ibufp, l);
	*(cvbuf + --l - 1) = '\n'; 	/* strip trailing esc */
	if (nonl) l--; 			/* strip newline */
	for (;;) {
		if ((n = get_input_line()) < 0)
			return NULL;
		else if (n == 0 || ibuf[n - 1] != '\n') {
			sprintf(errmsg, "unexpected end-of-file");
			return NULL;
		}
		CKBUF(cvbuf, cvbufsz, l + n, NULL);
		memcpy(cvbuf + l, ibuf, n);
		l += n;
		if (n < 2 || !has_trailing_escape(cvbuf, cvbuf + l - 1))
			break;
		*(cvbuf + --l - 1) = '\n'; 	/* strip trailing esc */
		if (nonl) l--; 			/* strip newline */
	}
	CKBUF(cvbuf, cvbufsz, l + 1, NULL);
	cvbuf[l] = '\0';
	*sizep = l;
	return cvbuf;
}


/* dup_line_node: return a pointer to a copy of a line node */
line_t *
dup_line_node(lp)
	line_t *lp;
{
	line_t *np;

	if ((np = (line_t *) malloc(sizeof(line_t))) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		sprintf(errmsg, "out of memory");
		return NULL;
	}
	np->seek = lp->seek;
	np->len = lp->len;
	return np;
}


/* has_trailing_escape:  return the parity of escapes preceding a character
   in a string */
int
has_trailing_escape(s, t)
	char *s;
	char *t;
{
    return (s == t || *(t - 1) != '\\') ? 0 : !has_trailing_escape(s, t - 1);
}


/* strip_escapes: return copy of escaped string */
char *
strip_escapes(s)
	char *s;
{
	static char *file = NULL;
	static int filesz = 0;

	int i = 0;

	CKBUF(file, filesz, MAXFNAME + 1, NULL);
	/* assert: no trailing escape */
	while (file[i++] = (*s == '\\') ? *++s : *s)
		s++;
	return file;
}


void
signal_hup(signo)
	int signo;
{
	if (mutex)
		sigflags |= (1 << signo);
	else	handle_hup(signo);
}


void
signal_int(signo)
	int signo;
{
	if (mutex)
		sigflags |= (1 << signo);
	else	handle_int(signo);
}


void
handle_hup(signo)
	int signo;
{
	char *hup = NULL;		/* hup filename */
	char *s;
	int n;

	if (!sigactive)
		quit(1);
	sigflags &= ~(1 << signo);
	if (addr_last && write_file(1, addr_last, "ed.hup", "w") < 0 &&
	    (s = getenv("HOME")) != NULL &&
	    (n = strlen(s)) + 8 <= MAXFNAME &&	/* "ed.hup" + '/' */
	    (hup = (char *) malloc(n + 10)) != NULL) {
		strcpy(hup, s);
		if (hup[n - 1] != '/')
			hup[n] = '/', hup[n+1] = '\0';
		strcat(hup, "ed.hup");
		write_file(1, addr_last, hup, "w");
	}
	quit(2);
}


void
handle_int(signo)
	int signo;
{
	if (!sigactive)
		quit(1);
	sigflags &= ~(1 << signo);
#ifdef _POSIX_SOURCE
	siglongjmp(env, -1);
#else
	longjmp(env, -1);
#endif
}


struct winsize ws;		/* window size structure */

void
handle_winch(signo)
	int signo;
{
	sigflags &= ~(1 << signo);
	if (ioctl(0, TIOCGWINSZ, (char *) &ws) >= 0) {
		if (ws.ws_row > 2) rows = ws.ws_row - 2;
		if (ws.ws_col > 8) cols = ws.ws_col - 8;
	}
}


/* is_legal_filename: return a legal filename */
int
is_legal_filename(s)
	char *s;
{
	if (red && (*s == '!' || !strcmp(s, "..") || strchr(s, '/'))) {
		sprintf(errmsg, "shell access restricted");
		return 0;
	}
	return 1;
}
