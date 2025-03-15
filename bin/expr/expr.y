/* $NetBSD: expr.y,v 1.52 2025/03/15 14:33:39 rillig Exp $ */

/*-
 * Copyright (c) 2000, 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek <jdolecek@NetBSD.org>, J.T. Conklin <jtc@NetBSD.org>
 * and Roland Illig <rillig@NetBSD.org>.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

%{
#include <sys/cdefs.h>
__RCSID("$NetBSD: expr.y,v 1.52 2025/03/15 14:33:39 rillig Exp $");

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char * const *av;
static unsigned skip_level;

static void yyerror(const char *, ...) __dead;
static int yylex(void);
static int is_empty_or_zero(const char *);
static int is_integer(const char *);
static const char *eval_arith(const char *, const char *, const char *);
static int eval_compare(const char *, const char *, const char *);
static const char *eval_match(const char *, const char *);

#define YYSTYPE	const char *

%}

%expect 0

%token STRING LPAREN RPAREN
%left SPEC_OR
%left SPEC_AND
%left COMPARE
%left ADD_SUB_OPERATOR
%left MUL_DIV_MOD_OPERATOR
%left SPEC_REG
%left LENGTH

%%

exp:	expr {
		(void)printf("%s\n", $1);
		return is_empty_or_zero($1);
	}
;

expr:	item
|	expr SPEC_OR {
		$$ = is_empty_or_zero($1) ? NULL : "1";
		if ($$)
			skip_level++;
	} expr {
		$$ = $3 ? $1 : $4;
		if ($3)
			skip_level--;
	}
|	expr SPEC_AND {
		$$ = is_empty_or_zero($1) ? NULL : "1";
		if (!$$)
			skip_level++;
	} expr {
		$$ = $3 && !is_empty_or_zero($4) ? $1 : "0";
		if (!$3)
			skip_level--;
	}
|	expr SPEC_REG expr {
		$$ = skip_level == 0 ? eval_match($1, $3) : "";
	}
|	expr ADD_SUB_OPERATOR expr {
		$$ = skip_level == 0 ? eval_arith($1, $2, $3) : "";
	}
|	expr MUL_DIV_MOD_OPERATOR expr {
		$$ = skip_level == 0 ? eval_arith($1, $2, $3) : "";
	}
|	expr COMPARE expr {
		$$ = skip_level == 0 && eval_compare($1, $2, $3) ? "1" : "0";
	}
|	LPAREN expr RPAREN {
		$$ = $2;
	}
|	LENGTH expr {
		char *ln;

		asprintf(&ln, "%ld", (long)strlen($2));
		if (ln == NULL)
			err(1, NULL);
		$$ = ln;
	}
;

item:	STRING
|	ADD_SUB_OPERATOR
|	MUL_DIV_MOD_OPERATOR
|	COMPARE
|	SPEC_OR
|	SPEC_AND
|	SPEC_REG
|	LENGTH
;

%%

static int
is_empty_or_zero(const char *str)
{
	char *endptr;

	return str[0] == '\0'
		|| (strtoll(str, &endptr, 10) == 0 && endptr[0] == '\0');
}

static int
is_integer(const char *str)
{
	char *endptr;

	(void)strtoll(str, &endptr, 10);
	/* note we treat empty string as valid number */
	return endptr[0] == '\0';
}

static int64_t
to_integer(const char *str)
{
	errno = 0;
	int64_t num = strtoll(str, NULL, 10);
	if (errno == ERANGE) {
		yyerror("value '%s' is too %s is %lld", str,
		    num > 0 ? "big, maximum" : "small, minimum",
		    num > 0 ? LLONG_MAX : LLONG_MIN);
	}
	return num;
}

static const char *
eval_arith(const char *left, const char *op, const char *right)
{
	int64_t res, l, r;

	res = 0;

	if (!is_integer(left))
		yyerror("non-integer argument '%s'", left);
	if (!is_integer(right))
		yyerror("non-integer argument '%s'", right);

	l = to_integer(left);
	r = to_integer(right);

	switch (op[0]) {
	case '+':
		if ((r > 0 && l > INT64_MAX - r) ||
		    (r < 0 && l < INT64_MIN - r))
			goto integer_overflow;
		res = l + r;
		break;
	case '-':
		if ((r > 0 && l < INT64_MIN + r) ||
		    (r < 0 && l > INT64_MAX + r))
			goto integer_overflow;
		res = l - r;
		break;
	case '/':
		if (r == 0)
			goto invalid_zero;
		if (l == INT64_MIN && r == -1)
			goto integer_overflow;
		res = l / r;
		break;
	case '%':
		if (r == 0)
			goto invalid_zero;
		if (l == INT64_MIN && r == -1)
			goto integer_overflow;
		res = l % r;
		break;
	case '*':
		if (l < 0 && r < 0 && l != INT64_MIN && r != INT64_MIN) {
			l = -l;
			r = -r;
		}

		if (l < 0 && r >= 0) {
			int64_t tmp = l;
			l = r;
			r = tmp;
		}

		if ((l < 0 && r < 0) ||
		    (r > 0 && l > INT64_MAX / r) ||
		    (r <= 0 && l != 0 && r < INT64_MIN / l))
			goto integer_overflow;
		res = l * r;
		break;
	}

	char *val;
	(void)asprintf(&val, "%lld", (long long int)res);
	if (val == NULL)
		err(1, NULL);
	return val;

integer_overflow:
	yyerror("integer overflow or underflow occurred for "
	    "operation '%s %s %s'", left, op, right);

invalid_zero:
	yyerror("second argument to '%s' must not be zero", op);
}

static int
eval_compare(const char *left, const char *op, const char *right)
{
	int64_t l, r;

	if (is_integer(left) && is_integer(right)) {
		l = strtoll(left, NULL, 10);
		r = strtoll(right, NULL, 10);
	} else {
		l = strcoll(left, right);
		r = 0;
	}

	switch (op[0]) {
	case '=':
		return l == r;
	case '>':
		if (op[1] == '=')
			return l >= r;
		else
			return l > r;
	case '<':
		if (op[1] == '=')
			return l <= r;
		else
			return l < r;
	default:
		return l != r;
	}
}

static const char *
eval_match(const char *str, const char *re)
{
	regex_t rp;
	regmatch_t rm[2];
	int rc;

	if ((rc = regcomp(&rp, re, REG_BASIC)) != 0) {
		char errbuf[256];
		(void)regerror(rc, &rp, errbuf, sizeof(errbuf));
		yyerror("%s", errbuf);
	}

	if (regexec(&rp, str, 2, rm, 0) == 0 && rm[0].rm_so == 0) {
		char *val;
		if (rm[1].rm_so >= 0) {
			(void)asprintf(&val, "%.*s",
				(int)(rm[1].rm_eo - rm[1].rm_so),
				str + rm[1].rm_so);
		} else {
			(void)asprintf(&val, "%d",
				(int)(rm[0].rm_eo - rm[0].rm_so));
		}
		if (val == NULL)
			err(1, NULL);
		return val;
	}

	if (rp.re_nsub == 0)
		return "0";
	else
		return "";
}

static const char x[] = "|&=<>+-*/%:()";
static const int x_token[] = {
	SPEC_OR, SPEC_AND, COMPARE, COMPARE, COMPARE, ADD_SUB_OPERATOR,
	ADD_SUB_OPERATOR, MUL_DIV_MOD_OPERATOR, MUL_DIV_MOD_OPERATOR, 
	MUL_DIV_MOD_OPERATOR, SPEC_REG, LPAREN, RPAREN
};

static int handle_ddash = 1;

int
yylex(void)
{
	const char *p = *av++;
	int retval;

	if (p == NULL)
		retval = 0;
	else if (p[0] == '\0')
		retval = STRING;
	else if (p[1] == '\0') {
		const char *w = strchr(x, p[0]);
		retval = w != NULL ? x_token[w - x] : STRING;
	} else if (p[1] == '=' && p[2] == '\0'
		    && (p[0] == '>' || p[0] == '<' || p[0] == '!'))
		retval = COMPARE;
	else if (handle_ddash && strcmp(p, "--") == 0) {
		handle_ddash = 0;
		retval = yylex();
		if (retval != STRING && retval != LPAREN && retval != RPAREN) {
			retval = STRING;
			av--;	/* was increased in call to yylex() above */
			p = "--";
		} else
			p = yylval;
	} else if (strcmp(p, "length") == 0)
		retval = LENGTH;
	else
		retval = STRING;

	handle_ddash = 0;
	yylval = p;

	return retval;
}

/*
 * Print error message and exit with error 2 (syntax error).
 */
static __printflike(1, 2) void
yyerror(const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	verrx(2, fmt, arg);
	va_end(arg);
}

int
main(int argc, const char * const *argv)
{
	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	if (argc == 1) {
		(void)fprintf(stderr, "usage: %s expression\n",
		    getprogname());
		exit(2);
	}

	av = argv + 1;

	return yyparse();
}
