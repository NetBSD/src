/*	$NetBSD: gspa.c,v 1.13.20.1 2009/05/13 19:20:23 jym Exp $	*/
/*
 * GSP assembler main program
 *
 * Copyright (c) 1993 Paul Mackerras.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Mackerras.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: gspa.c,v 1.13.20.1 2009/05/13 19:20:23 jym Exp $");
#endif

#include <sys/param.h>
#include <err.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "gsp_ass.h"
#include "gsp_gram.h"

#define YYDEBUG_VALUE	0

extern YYSTYPE yylval;
int err_count;

char line[MAXLINE];
unsigned lineno;

extern int yydebug;
short pass2;

unsigned pc;
unsigned highest_pc;
unsigned line_pc;

FILE *infile;
FILE *current_infile;
FILE *objfile;
FILE *listfile;

char *c_name;
char in_name[PATH_MAX + 1];

struct input {
	FILE	*fp;
	struct input *next;
	int	lineno;
	char	name[128];
} *pending_input;

jmp_buf synerrjmp;

void	setext(char *, const char *, const char *);
void	usage(void);
int	yyparse(void);

void	c_dumpbuf(void);

int
main(int argc, char **argv)
{
	char * volatile hex_name;
	char * volatile list_name;
	int c;

	hex_name = list_name = 0;

	/* parse options */
	while ((c = getopt(argc, argv, "o:l:c:")) != -1) {
		switch (c) {
		case 'o':
			if (hex_name)
				usage();
			hex_name = optarg;
			break;
		case 'c':
			if (c_name)
				usage();
			c_name = optarg;
			break;
		case 'l':
			if (list_name)
				usage();
			list_name = optarg;
			break;
		default:
			usage();
		}
	}

	/* get source file */
	argc -= optind;
	argv += optind;
	if (argc == 0) {
		infile = stdin;
		strlcpy(in_name, "<stdin>", sizeof(in_name));
	} else if (argc == 1) {
		strlcpy(in_name, *argv, sizeof(in_name));
		if ((infile = fopen(in_name, "r")) == NULL)
			err(1, "fopen");
	} else 
		usage();

	/* Pass 1 */
	pending_input = NULL;
	current_infile = infile;

	yydebug = YYDEBUG_VALUE;
	pass2 = 0;
	pc = 0;
	lineno = 0;
	while( get_line(line, MAXLINE) ){
		if( !setjmp(synerrjmp) ){
			lex_init(line);
			yyparse();
		}
	}
	if( err_count > 0 )
		exit(1);

	/* Open output files */
	if (hex_name == 0)
		objfile = stdout;
	else if ((objfile = fopen(hex_name, "w")) == NULL)
		err(1, "fopen");
	if (c_name) {
		fprintf(objfile, "/*\n"
		    " * This file was automatically created from\n"
		    " * a TMS34010 assembler output file.\n"
		    " * Do not edit manually.\n"
		    " */\n"
		    "#include <sys/types.h>\n"
		    "u_int16_t %s[] = {\n\t", c_name);
	}
	if (list_name)
		if ((listfile = fopen(list_name, "w")) == NULL)
			err(1, "fopen");

	/* Pass 2 */
	pass2 = 1;
	rewind(infile);
	current_infile = infile;
	pc = 0;
	lineno = 0;
	reset_numeric_labels();
	while( get_line(line, MAXLINE) ){
		line_pc = pc;
		if( !setjmp(synerrjmp) ){
			lex_init(line);
			yyparse();
		}
		listing();
	}

	if (c_name) {
		c_dumpbuf();
		fprintf(objfile, "\n\t0\n};\n");
	}

	exit(err_count != 0);
}

void
setext(char *out, const char *in, const char *ext)
{
	const char *p;

	p = strrchr(in, '.');
	if( p != NULL ){
		memcpy(out, in, p - in);
		strcpy(out + (p - in), ext);
	} else {
		strcpy(out, in);
		strcat(out, ext);
	}
}

void
push_input(char *fn)
{
	FILE *f;
	struct input *p;

	f = fopen(fn, "r");
	if( f == NULL ){
		p1err("Can't open input file %s", fn);
		return;
	}
	new(p);
	p->fp = current_infile;
	p->lineno = lineno;
	strlcpy(p->name, in_name, sizeof(p->name));
	p->next = pending_input;
	current_infile = f;
	lineno = 1;
	strlcpy(in_name, fn, sizeof(in_name));
	pending_input = p;
}

int
get_line(char *lp, int maxlen)
{
	struct input *p;

	while( fgets(lp, maxlen, current_infile) == NULL ){
		if( (p = pending_input) == NULL )
			return 0;
		/* pop the input stack */
		fclose(current_infile);
		current_infile = p->fp;
		strlcpy(in_name, p->name, sizeof(in_name));
		lineno = p->lineno;
		pending_input = p->next;
		free(p);
	}
	++lineno;
	return 1;
}

void
perr(const char *fmt, ...)
{
	va_list ap;
	char error_string[256];

	if( !pass2 )
		return;
	fprintf(stderr, "Error in line %d: ", lineno);
	va_start(ap, fmt);
	vsprintf(error_string, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n", error_string);
	list_error(error_string);
	++err_count;
}

void
p1err(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Pass 1 error in line %d: ", lineno);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	++err_count;
}

void
yyerror(const char *errs)
{

	perr("%s", errs);
	longjmp(synerrjmp, 1);
}

void
usage()
{
	fprintf(stderr,
		"Usage: gspa [-c c_array_name] [-l list_file] [-o hex_file] [infile]\n");
	exit(1);
}
