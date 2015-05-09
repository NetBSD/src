/*	$NetBSD: rpc_scan.c,v 1.15 2015/05/09 23:28:43 dholland Exp $	*/
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user or with the express written consent of
 * Sun Microsystems, Inc.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rpc_scan.c 1.11 89/02/22 (C) 1987 SMI";
#else
__RCSID("$NetBSD: rpc_scan.c,v 1.15 2015/05/09 23:28:43 dholland Exp $");
#endif
#endif

/*
 * rpc_scan.c, Scanner for the RPC protocol compiler
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "rpc_scan.h"
#include "rpc_parse.h"
#include "rpc_util.h"

#define startcomment(where) (where[0] == '/' && where[1] == '*')
#define endcomment(where) (where[-1] == '*' && where[0] == '/')

static void unget_token(token *);
static void findstrconst(char **, const char **);
static void findchrconst(char **, const char **);
static void findconst(char **, const char **);
static void findkind(char **, token *);
static int cppline(const char *);
static int directive(const char *);
static void printdirective(const char *);
static void docppline(char *, int *, const char **);

static int pushed = 0;		/* is a token pushed */
static token lasttok;		/* last token, if pushed */

/*
 * scan expecting 1 given token
 */
void
scan(tok_kind expect, token *tokp)
{
	get_token(tokp);
	if (tokp->kind != expect) {
		expected1(expect);
	}
}
/*
 * scan expecting any of the 2 given tokens
 */
void
scan2(tok_kind expect1, tok_kind expect2, token *tokp)
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2) {
		expected2(expect1, expect2);
	}
}
/*
 * scan expecting any of the 3 given token
 */
void
scan3(tok_kind expect1, tok_kind expect2, tok_kind expect3, token *tokp)
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2
	    && tokp->kind != expect3) {
		expected3(expect1, expect2, expect3);
	}
}
/*
 * scan expecting a constant, possibly symbolic
 */
void
scan_num(token *tokp)
{
	get_token(tokp);
	switch (tokp->kind) {
	case TOK_IDENT:
		break;
	default:
		error("Expected constant or identifier");
	}
}
/*
 * Peek at the next token
 */
void
peek(token *tokp)
{
	get_token(tokp);
	unget_token(tokp);
}
/*
 * Peek at the next token and scan it if it matches what you expect
 */
int
peekscan(tok_kind expect, token *tokp)
{
	peek(tokp);
	if (tokp->kind == expect) {
		get_token(tokp);
		return (1);
	}
	return (0);
}
/*
 * Get the next token, printing out any directive that are encountered.
 */
void
get_token(token *tokp)
{
	int     commenting;

	if (pushed) {
		pushed = 0;
		*tokp = lasttok;
		return;
	}
	commenting = 0;
	for (;;) {
		if (*where == 0) {
			for (;;) {
				if (!fgets(curline, MAXLINESIZE, fin)) {
					tokp->kind = TOK_EOF;
					*where = 0;
					return;
				}
				linenum++;
				if (commenting) {
					break;
				} else
					if (cppline(curline)) {
						docppline(curline, &linenum,
						    &infilename);
					} else
						if (directive(curline)) {
							printdirective(curline);
						} else {
							break;
						}
			}
			where = curline;
		} else
			if (isspace((unsigned char)*where)) {
				while (isspace((unsigned char)*where)) {
					where++;	/* eat */
				}
			} else
				if (commenting) {
					for (where++; *where; where++) {
						if (endcomment(where)) {
							where++;
							commenting--;
							break;
						}
					}
				} else
					if (startcomment(where)) {
						where += 2;
						commenting++;
					} else {
						break;
					}
	}

	/*
	 * 'where' is not whitespace, comment or directive Must be a token!
	 */
	switch (*where) {
	case ':':
		tokp->kind = TOK_COLON;
		where++;
		break;
	case ';':
		tokp->kind = TOK_SEMICOLON;
		where++;
		break;
	case ',':
		tokp->kind = TOK_COMMA;
		where++;
		break;
	case '=':
		tokp->kind = TOK_EQUAL;
		where++;
		break;
	case '*':
		tokp->kind = TOK_STAR;
		where++;
		break;
	case '[':
		tokp->kind = TOK_LBRACKET;
		where++;
		break;
	case ']':
		tokp->kind = TOK_RBRACKET;
		where++;
		break;
	case '{':
		tokp->kind = TOK_LBRACE;
		where++;
		break;
	case '}':
		tokp->kind = TOK_RBRACE;
		where++;
		break;
	case '(':
		tokp->kind = TOK_LPAREN;
		where++;
		break;
	case ')':
		tokp->kind = TOK_RPAREN;
		where++;
		break;
	case '<':
		tokp->kind = TOK_LANGLE;
		where++;
		break;
	case '>':
		tokp->kind = TOK_RANGLE;
		where++;
		break;

	case '"':
		tokp->kind = TOK_STRCONST;
		findstrconst(&where, &tokp->str);
		break;
	case '\'':
		tokp->kind = TOK_CHARCONST;
		findchrconst(&where, &tokp->str);
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		tokp->kind = TOK_IDENT;
		findconst(&where, &tokp->str);
		break;

	default:
		if (!(isalpha((unsigned char)*where) || *where == '_')) {
			if (isprint((unsigned char)*where)) {
				error("Illegal character '%c' in file", *where);
			} else {
				error("Illegal character %d in file", *where);
			}
		}
		findkind(&where, tokp);
		break;
	}
}

static void
unget_token(token *tokp)
{
	lasttok = *tokp;
	pushed = 1;
}

static void
findstrconst(char **str, const char **val)
{
	char *p;
	int     size;
	char *tmp;

	p = *str;
	do {
		p++;
	} while (*p && *p != '"');
	if (*p == 0) {
		error("Unterminated string constant");
	}
	p++;
	size = p - *str;
	tmp = alloc(size + 1);
	(void) strncpy(tmp, *str, size);
	tmp[size] = 0;
	*val = tmp;
	*str = p;
}

static void
findchrconst(char **str, const char **val)
{
	char *p;
	int     size;
	char *tmp;

	p = *str;
	do {
		p++;
	} while (*p && *p != '\'');
	if (*p == 0) {
		error("Unterminated string constant");
	}
	p++;
	size = p - *str;
	if (size != 3) {
		error("Empty character");
	}
	tmp = alloc(size + 1);
	(void) strncpy(tmp, *str, size);
	tmp[size] = 0;
	*val = tmp;
	*str = p;
}

static void
findconst(char **str, const char **val)
{
	char *p;
	int     size;
	char *tmp;

	p = *str;
	if (*p == '0' && *(p + 1) == 'x') {
		p++;
		do {
			p++;
		} while (isxdigit((unsigned char)*p));
	} else {
		do {
			p++;
		} while (isdigit((unsigned char)*p));
	}
	size = p - *str;
	tmp = alloc(size + 1);
	(void) strncpy(tmp, *str, size);
	tmp[size] = 0;
	*val = tmp;
	*str = p;
}

static const token symbols[] = {
	{TOK_CONST, "const"},
	{TOK_UNION, "union"},
	{TOK_SWITCH, "switch"},
	{TOK_CASE, "case"},
	{TOK_DEFAULT, "default"},
	{TOK_STRUCT, "struct"},
	{TOK_TYPEDEF, "typedef"},
	{TOK_ENUM, "enum"},
	{TOK_OPAQUE, "opaque"},
	{TOK_BOOL, "bool"},
	{TOK_VOID, "void"},
	{TOK_CHAR, "char"},
	{TOK_INT, "int"},
	{TOK_UNSIGNED, "unsigned"},
	{TOK_SHORT, "short"},
	{TOK_LONG, "long"},
	{TOK_HYPER, "hyper"},
	{TOK_FLOAT, "float"},
	{TOK_DOUBLE, "double"},
	{TOK_QUAD, "quadruple"},
	{TOK_STRING, "string"},
	{TOK_PROGRAM, "program"},
	{TOK_VERSION, "version"},
	{TOK_EOF, "??????"},
};

static void
findkind(char **mark, token *tokp)
{
	int     len;
	const token *s;
	char *str;
	char *tmp;

	str = *mark;
	for (s = symbols; s->kind != TOK_EOF; s++) {
		len = strlen(s->str);
		if (strncmp(str, s->str, len) == 0) {
			if (!isalnum((unsigned char)str[len]) &&
			    str[len] != '_') {
				tokp->kind = s->kind;
				tokp->str = s->str;
				*mark = str + len;
				return;
			}
		}
	}
	tokp->kind = TOK_IDENT;
	for (len = 0; isalnum((unsigned char)str[len]) ||
	    str[len] == '_'; len++);
	tmp = alloc(len + 1);
	(void) strncpy(tmp, str, len);
	tmp[len] = 0;
	tokp->str = tmp;
	*mark = str + len;
}

static int
cppline(const char *line)
{
	return (line == curline && *line == '#');
}

static int
directive(const char *line)
{
	return (line == curline && *line == '%');
}

static void
printdirective(const char *line)
{
	f_print(fout, "%s", line + 1);
}

static void
docppline(char *line, int *lineno, const char **fname)
{
	char   *file;
	int     num;
	char   *p;

	line++;
	while (isspace((unsigned char)*line)) {
		line++;
	}
	num = atoi(line);
	while (isdigit((unsigned char)*line)) {
		line++;
	}
	while (isspace((unsigned char)*line)) {
		line++;
	}
	if (*line != '"') {
		error("Preprocessor error");
	}
	line++;
	p = file = alloc(strlen(line) + 1);
	while (*line && *line != '"') {
		*p++ = *line++;
	}
	if (*line == 0) {
		error("Preprocessor error");
	}
	*p = 0;
	if (*file == 0) {
		*fname = NULL;
		free(file);
	} else {
		*fname = file;
	}
	*lineno = num - 1;
}
