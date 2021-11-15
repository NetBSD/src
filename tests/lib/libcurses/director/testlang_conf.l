%{
/*	$NetBSD: testlang_conf.l,v 1.26 2021/11/15 21:45:46 blymn Exp $ 	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#include <curses.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <err.h>
#include "returns.h"
#include "testlang_parse.h"

#define MAX_INCLUDES 32 /* limit for the number of nested includes */

int yylex(void);

extern size_t line;
extern char *cur_file;		/* from director.c */

static int include_stack[MAX_INCLUDES];
static char *include_files[MAX_INCLUDES];
static int include_ptr = 0;

static char *
dequote(const char *s, size_t *len)
{
	const unsigned char *p;
	char *buf, *q;

	*len = 0;
	p = (const unsigned char *)s;
	while (*p) {
		if (*p == '\\' && p[1]) {
			if (isdigit(p[1]) && isdigit(p[2]) && isdigit(p[3]))
				p += 3;
			else
				++p;
		}
		++(*len);
		++p;
	}

	buf = malloc(*len + 1);
	if (buf == NULL)
		return NULL;

	p = (const unsigned char *)s;
	q = buf;
	while (*p) {
		if (*p == '\\' && p[1]) {
			++p;

			if (isdigit(p[0])) {
				if (isdigit(p[1]) && isdigit(p[2])) {
					*q++ = (p[0] - '0') * 64 +
					    (p[1] - '0') * 8 +
					    (p[2] - '0');
					p += 3;
				} else {
					*q++ = *p++;
				}
				continue;
			}

			switch (*p) {
			case 'b':
				/* backspace */
				*q++ = '\b';
				p++;
				break;

			case 'e':
				/* escape */
				*q++ = '\e';
				p++;
				break;

			case 'n':
				/* newline */
				*q++ = '\n';
				p++;
				break;

			case 'r':
				/* carriage return */
				*q++ = '\r';
				p++;
				break;

			case 't':
				/* tab */
				*q++ = '\t';
				p++;
				break;

			case '\\':
				/* backslash */
				*q++ = '\\';
				p++;
				break;

			default:
				if (isalpha(*p))
					errx(2,
					    "%s:%zu: Invalid escape sequence "
					    "'\\%c' in string literal",
					    cur_file, line, *p);
				*q++ = *p++;
			}
		} else
			*q++ = *p++;
	}
	*q++ = '\0';

	return buf;
}
%}

HEX		0[xX][0-9a-zA-Z]+
STRING		[0-9a-z!#-&(-^ \t%._\\]+
numeric		[-0-9]+
PCHAR           (\\.|[!-~])
ASSIGN		assign
CALL2		call2
CALL3		call3
CALL4		call4
CALL		call
CHECK		check
DELAY		delay
INPUT		input
NOINPUT		noinput
OK_RET		OK
ERR_RET		ERR
COMPARE		compare
COMPAREND	comparend
FILENAME	[A-Za-z0-9.][A-Za-z0-9./_-]+
VARNAME		[A-Za-z][A-Za-z0-9_-]+
NULL_RET	NULL
NON_NULL	NON_NULL
CCHAR		cchar
WCHAR		wchar
BYTE		BYTE
OR		\|
LPAREN		\(
RPAREN		\)
LBRACK		\[
RBRACK		\]
MULTIPLIER	\*
COMMA		,

%x incl
%option noinput nounput

%%

include		BEGIN(incl);

<incl>[ \t]*	/* eat the whitespace */
<incl>[^ \t\n]+ { /* got the include file name */
		char *inc_file;

		if (include_ptr > MAX_INCLUDES) {
			errx(2,
			    "%s:%zu: Maximum number of nested includes "
			    "exceeded", cur_file, line);
		}

		const char *dir_begin;
		int dir_len;
		if (yytext[0] == '/') {
			dir_begin = "";
			dir_len = 0;
		} else {
			dir_begin = cur_file;
			const char *dir_end = strrchr(cur_file, '/');
			if (dir_end != NULL) {
				dir_len = (int)(dir_end + 1 - dir_begin);
			} else {
				dir_begin = ".";
				dir_len = 1;
			}
		}

		if (asprintf(&inc_file, "%.*s%s",
		    dir_len, dir_begin, yytext) == -1)
			err(2, "Cannot construct include path");

		yyin = fopen(inc_file, "r");

		if (!yyin)
			err(1, "Error opening %s", inc_file);

		yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));

		include_stack[include_ptr] = line;
		include_files[include_ptr++] = cur_file;
		cur_file = inc_file;
		line = 1;
		BEGIN(INITIAL);
	}

<<EOF>>	{
		yypop_buffer_state();

		if (!YY_CURRENT_BUFFER)
			yyterminate();

		if (--include_ptr < 0)
			errx(2, "Include stack underflow");

		free(cur_file);
		cur_file = include_files[include_ptr];
		line = include_stack[include_ptr];
	}

{ASSIGN}	return ASSIGN;
{CALL2}		return CALL2;
{CALL3}		return CALL3;
{CALL4}		return CALL4;
{CALL}		return CALL;
{CHECK}		return CHECK;
{DELAY}		return DELAY;
{INPUT}		return INPUT;
{NOINPUT}	return NOINPUT;
{COMPARE}	return COMPARE;
{COMPAREND}	return COMPAREND;
{NON_NULL}	return NON_NULL;
{NULL_RET}	return NULL_RET;
{OK_RET}	return OK_RET;
{ERR_RET}	return ERR_RET;
{MULTIPLIER}	return MULTIPLIER;
{COMMA}		return COMMA;
{CCHAR}		return CCHAR;
{WCHAR}		return WCHAR;
{OR}		return OR;
{LPAREN}	return LPAREN;
{RPAREN}	return RPAREN;
{LBRACK}	return LBRACK;
{RBRACK}	return RBRACK;

{HEX}		{
			/* Hex value, convert to decimal and return numeric */
			unsigned long val;

			if (sscanf(yytext, "%lx", &val) != 1)
				errx(1, "Bad hex conversion");

			asprintf(&yylval.string, "%ld", val);
			return numeric;
		}

{numeric}	{
			if ((yylval.string = strdup(yytext)) == NULL)
				err(1, "Cannot allocate numeric string");
			return numeric;
		}

{VARNAME}	{
			if ((yylval.string = strdup(yytext)) == NULL)
				err(1, "Cannot allocate string for varname");
			return VARNAME;
		}

{FILENAME}	{
			size_t len;

			if ((yylval.string = dequote(yytext, &len)) == NULL)
				err(1, "Cannot allocate filename string");
			return FILENAME;
		}

	/* path */
\/{PCHAR}+	{
			size_t len;
			if ((yylval.string = dequote(yytext, &len)) == NULL)
				err(1, "Cannot allocate string");
			return PATH;
		}

\'{STRING}\' 	{
			char *p;
			size_t len;

			if ((yylval.retval = malloc(sizeof(ct_data_t))) == NULL)
				err(1, "Cannot allocate return struct");
			p = yytext;
			p++; /* skip the leading ' */
			if ((yylval.retval->data_value = dequote(p, &len))
			     == NULL)
				err(1, "Cannot allocate string");

			yylval.retval->data_type = data_byte;
			/* trim trailing ' */
			yylval.retval->data_len = len - 1;
			return BYTE;
		}

\`{STRING}\` 	{
			char *p, *str;
			size_t len, chlen;
			size_t i;
			chtype *rv;

			if ((yylval.retval = malloc(sizeof(ct_data_t))) == NULL)
				err(1, "Cannot allocate return struct");
			p = yytext;
			p++; /* skip the leading ` */
			if ((str = dequote(p, &len)) == NULL)
				err(1, "Cannot allocate string");
			len--; /* trim trailing ` */
			if ((len % 2) != 0)
				len--;

			chlen = ((len / 2) + 1) * sizeof(chtype);
			if ((yylval.retval->data_value = malloc(chlen))
			    == NULL)
				err(1, "Cannot allocate chtype array");

			rv = yylval.retval->data_value;
			for (i = 0; i < len; i += 2)
				*rv++ = (str[i] << 8) | str[i+1];
			*rv = __NORMAL | '\0'; /* terminates chtype array */
			yylval.retval->data_type = data_byte;
			yylval.retval->data_len = chlen;
			return BYTE;
		}

\"{STRING}\" 	{
			char *p;
			size_t len;

			p = yytext;
			p++; /* skip the leading " */
			if ((yylval.string = dequote(p, &len)) == NULL)
				err(1, "Cannot allocate string");

			/* remove trailing " */
			yylval.string[len - 1] = '\0';
			return STRING;
		}

\${VARNAME}	{
			char *p;

			p = yytext;
			p++; /* skip $ before var name */
			if ((yylval.string = strdup(p)) == NULL)
				err(1, "Cannot allocate string for varname");
			return VARIABLE;
		}

	/* whitespace, comments */
[ \t\r]		|
#.*		;

^[ \t\r]*#.*\n	|
\\\n		|
^\n		line++;

	/* eol on a line with data. need to process, return eol */
#.*\n		|
\n		{
			line++;
			return EOL;
		}

.		{
			if (isprint((unsigned char)yytext[0]))
				errx(1, "%s:%zu: Invalid character '%c'",
				    cur_file, line + 1, yytext[0]);
			else
				errx(1, "%s:%zu: Invalid character '0x%02x'",
				    cur_file, line + 1, yytext[0]);
		}

%%

int
yywrap(void)
{
	return 1;
}
