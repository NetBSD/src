/*	$NetBSD: arithmetic.c,v 1.5 2018/04/21 23:01:29 kre Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2007
 *	Herbert Xu <herbert@gondor.apana.org.au>.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
 *
 * From FreeBSD, who obtained it from dash, modified both times...
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: arithmetic.c,v 1.5 2018/04/21 23:01:29 kre Exp $");
#endif /* not lint */

#include <limits.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "arithmetic.h"
#include "arith_tokens.h"
#include "expand.h"
#include "error.h"
#include "memalloc.h"
#include "output.h"
#include "options.h"
#include "var.h"
#include "show.h"
#include "syntax.h"

#if ARITH_BOR + ARITH_ASS_GAP != ARITH_BORASS || \
	ARITH_ASS + ARITH_ASS_GAP != ARITH_EQ
#error Arithmetic tokens are out of order.
#endif

static const char *arith_startbuf;

const char *arith_buf;
union a_token_val a_t_val;

static int last_token;

int arith_lno, arith_var_lno;

#define ARITH_PRECEDENCE(op, prec) [op - ARITH_BINOP_MIN] = prec

static const char prec[ARITH_BINOP_MAX - ARITH_BINOP_MIN] = {
	ARITH_PRECEDENCE(ARITH_MUL, 0),
	ARITH_PRECEDENCE(ARITH_DIV, 0),
	ARITH_PRECEDENCE(ARITH_REM, 0),
	ARITH_PRECEDENCE(ARITH_ADD, 1),
	ARITH_PRECEDENCE(ARITH_SUB, 1),
	ARITH_PRECEDENCE(ARITH_LSHIFT, 2),
	ARITH_PRECEDENCE(ARITH_RSHIFT, 2),
	ARITH_PRECEDENCE(ARITH_LT, 3),
	ARITH_PRECEDENCE(ARITH_LE, 3),
	ARITH_PRECEDENCE(ARITH_GT, 3),
	ARITH_PRECEDENCE(ARITH_GE, 3),
	ARITH_PRECEDENCE(ARITH_EQ, 4),
	ARITH_PRECEDENCE(ARITH_NE, 4),
	ARITH_PRECEDENCE(ARITH_BAND, 5),
	ARITH_PRECEDENCE(ARITH_BXOR, 6),
	ARITH_PRECEDENCE(ARITH_BOR, 7),
};

#define ARITH_MAX_PREC 8

int expcmd(int, char **);

static void __dead
arith_err(const char *s)
{
	error("arithmetic expression: %s: \"%s\"", s, arith_startbuf);
	/* NOTREACHED */
}

static intmax_t
arith_lookupvarint(char *varname)
{
	const char *str;
	char *p;
	intmax_t result;
	const int oln = line_number;

	VTRACE(DBG_ARITH, ("Arith var lookup(\"%s\") with lno=%d\n", varname,
	    arith_var_lno));

	line_number = arith_var_lno;
	str = lookupvar(varname);
	line_number = oln;

	if (uflag && str == NULL)
		arith_err("variable not set");
	if (str == NULL || *str == '\0')
		str = "0";
	errno = 0;
	result = strtoimax(str, &p, 0);
	if (errno != 0 || *p != '\0') {
		if (errno == 0) {
			while (*p != '\0' && is_space(*p))
				p++;
			if (*p == '\0')
				return result;
		}
		arith_err("variable contains non-numeric value");
	}
	return result;
}

static inline int
arith_prec(int op)
{

	return prec[op - ARITH_BINOP_MIN];
}

static inline int
higher_prec(int op1, int op2)
{

	return arith_prec(op1) < arith_prec(op2);
}

static intmax_t
do_binop(int op, intmax_t a, intmax_t b)
{

	VTRACE(DBG_ARITH, ("Arith do binop %d (%jd, %jd)\n", op, a, b));
	switch (op) {
	default:
		arith_err("token error");
	case ARITH_REM:
	case ARITH_DIV:
		if (b == 0)
			arith_err("division by zero");
		if (a == INTMAX_MIN && b == -1)
			arith_err("divide error");
		return op == ARITH_REM ? a % b : a / b;
	case ARITH_MUL:
		return (uintmax_t)a * (uintmax_t)b;
	case ARITH_ADD:
		return (uintmax_t)a + (uintmax_t)b;
	case ARITH_SUB:
		return (uintmax_t)a - (uintmax_t)b;
	case ARITH_LSHIFT:
		return (uintmax_t)a << (b & (sizeof(uintmax_t) * CHAR_BIT - 1));
	case ARITH_RSHIFT:
		return a >> (b & (sizeof(uintmax_t) * CHAR_BIT - 1));
	case ARITH_LT:
		return a < b;
	case ARITH_LE:
		return a <= b;
	case ARITH_GT:
		return a > b;
	case ARITH_GE:
		return a >= b;
	case ARITH_EQ:
		return a == b;
	case ARITH_NE:
		return a != b;
	case ARITH_BAND:
		return a & b;
	case ARITH_BXOR:
		return a ^ b;
	case ARITH_BOR:
		return a | b;
	}
}

static intmax_t assignment(int, int);
static intmax_t comma_list(int, int);

static intmax_t
primary(int token, union a_token_val *val, int op, int noeval)
{
	intmax_t result;
	char sresult[DIGITS(result) + 1];

	VTRACE(DBG_ARITH, ("Arith primary: token %d op %d%s\n",
	    token, op, noeval ? " noeval" : ""));

	switch (token) {
	case ARITH_LPAREN:
		result = comma_list(op, noeval);
		if (last_token != ARITH_RPAREN)
			arith_err("expecting ')'");
		last_token = arith_token();
		return result;
	case ARITH_NUM:
		last_token = op;
		return val->val;
	case ARITH_VAR:
		result = noeval ? val->val : arith_lookupvarint(val->name);
		if (op == ARITH_INCR || op == ARITH_DECR) {
			last_token = arith_token();
			if (noeval)
				return val->val;

			snprintf(sresult, sizeof(sresult), ARITH_FORMAT_STR,
			    result + (op == ARITH_INCR ? 1 : -1));
			setvar(val->name, sresult, 0);
		} else
			last_token = op;
		return result;
	case ARITH_ADD:
		*val = a_t_val;
		return primary(op, val, arith_token(), noeval);
	case ARITH_SUB:
		*val = a_t_val;
		return -primary(op, val, arith_token(), noeval);
	case ARITH_NOT:
		*val = a_t_val;
		return !primary(op, val, arith_token(), noeval);
	case ARITH_BNOT:
		*val = a_t_val;
		return ~primary(op, val, arith_token(), noeval);
	case ARITH_INCR:
	case ARITH_DECR:
		if (op != ARITH_VAR)
			arith_err("incr/decr require var name");
		last_token = arith_token();
		if (noeval)
			return val->val;
		result = arith_lookupvarint(a_t_val.name);
		snprintf(sresult, sizeof(sresult), ARITH_FORMAT_STR,
			    result += (token == ARITH_INCR ? 1 : -1));
		setvar(a_t_val.name, sresult, 0);
		return result;
	default:
		arith_err("expecting primary");
	}
	return 0;	/* never reached */
}

static intmax_t
binop2(intmax_t a, int op, int precedence, int noeval)
{
	union a_token_val val;
	intmax_t b;
	int op2;
	int token;

	VTRACE(DBG_ARITH, ("Arith: binop2 %jd op %d (P:%d)%s\n",
	    a, op, precedence, noeval ? " noeval" : ""));

	for (;;) {
		token = arith_token();
		val = a_t_val;

		b = primary(token, &val, arith_token(), noeval);

		op2 = last_token;
		if (op2 >= ARITH_BINOP_MIN && op2 < ARITH_BINOP_MAX &&
		    higher_prec(op2, op)) {
			b = binop2(b, op2, arith_prec(op), noeval);
			op2 = last_token;
		}

		a = noeval ? b : do_binop(op, a, b);

		if (op2 < ARITH_BINOP_MIN || op2 >= ARITH_BINOP_MAX ||
		    arith_prec(op2) >= precedence)
			return a;

		op = op2;
	}
}

static intmax_t
binop(int token, union a_token_val *val, int op, int noeval)
{
	intmax_t a = primary(token, val, op, noeval);

	op = last_token;
	if (op < ARITH_BINOP_MIN || op >= ARITH_BINOP_MAX)
		return a;

	return binop2(a, op, ARITH_MAX_PREC, noeval);
}

static intmax_t
and(int token, union a_token_val *val, int op, int noeval)
{
	intmax_t a = binop(token, val, op, noeval);
	intmax_t b;

	op = last_token;
	if (op != ARITH_AND)
		return a;

	VTRACE(DBG_ARITH, ("Arith: AND %jd%s\n", a, noeval ? " noeval" : ""));

	token = arith_token();
	*val = a_t_val;

	b = and(token, val, arith_token(), noeval | !a);

	return a && b;
}

static intmax_t
or(int token, union a_token_val *val, int op, int noeval)
{
	intmax_t a = and(token, val, op, noeval);
	intmax_t b;

	op = last_token;
	if (op != ARITH_OR)
		return a;

	VTRACE(DBG_ARITH, ("Arith: OR %jd%s\n", a, noeval ? " noeval" : ""));

	token = arith_token();
	*val = a_t_val;

	b = or(token, val, arith_token(), noeval | !!a);

	return a || b;
}

static intmax_t
cond(int token, union a_token_val *val, int op, int noeval)
{
	intmax_t a = or(token, val, op, noeval);
	intmax_t b;
	intmax_t c;

	if (last_token != ARITH_QMARK)
		return a;

	VTRACE(DBG_ARITH, ("Arith: ?: %jd%s\n", a, noeval ? " noeval" : ""));

	b = assignment(arith_token(), noeval | !a);

	if (last_token != ARITH_COLON)
		arith_err("expecting ':'");

	token = arith_token();
	*val = a_t_val;

	c = cond(token, val, arith_token(), noeval | !!a);

	return a ? b : c;
}

static intmax_t
assignment(int var, int noeval)
{
	union a_token_val val = a_t_val;
	int op = arith_token();
	intmax_t result;
	char sresult[DIGITS(result) + 1];


	if (var != ARITH_VAR)
		return cond(var, &val, op, noeval);

	if (op != ARITH_ASS && (op < ARITH_ASS_MIN || op >= ARITH_ASS_MAX))
		return cond(var, &val, op, noeval);

	VTRACE(DBG_ARITH, ("Arith: %s ASSIGN %d%s\n", val.name, op,
	    noeval ? " noeval" : ""));

	result = assignment(arith_token(), noeval);
	if (noeval)
		return result;

	if (op != ARITH_ASS)
		result = do_binop(op - ARITH_ASS_GAP,
		    arith_lookupvarint(val.name), result);
	snprintf(sresult, sizeof(sresult), ARITH_FORMAT_STR, result);
	setvar(val.name, sresult, 0);
	return result;
}

static intmax_t
comma_list(int token, int noeval)
{
	intmax_t result = assignment(token, noeval);

	while (last_token == ARITH_COMMA) {
		VTRACE(DBG_ARITH, ("Arith: comma discarding %jd%s\n", result,
		    noeval ? " noeval" : ""));
		result = assignment(arith_token(), noeval);
	}

	return result;
}

intmax_t
arith(const char *s, int lno)
{
	struct stackmark smark;
	intmax_t result;
	const char *p;
	int nls = 0;

	setstackmark(&smark);

	arith_lno = lno;

	CTRACE(DBG_ARITH, ("Arith(\"%s\", %d) @%d\n", s, lno, arith_lno));

	/* check if it is possible we might reference LINENO */
	p = s;
	while ((p = strchr(p, 'L')) != NULL) {
		if (p[1] == 'I' && p[2] == 'N') {
			/* if it is possible, we need to correct airth_lno */
			p = s;
			while ((p = strchr(p, '\n')) != NULL)
				nls++, p++;
			VTRACE(DBG_ARITH, ("Arith found %d newlines\n", nls));
			arith_lno -= nls;
			break;
		}
		p++;
	}

	arith_buf = arith_startbuf = s;

	result = comma_list(arith_token(), 0);

	if (last_token)
		arith_err("expecting end of expression");

	popstackmark(&smark);

	CTRACE(DBG_ARITH, ("Arith result=%jd\n", result));

	return result;
}

/*
 *  The let(1)/exp(1) builtin.
 */
int
expcmd(int argc, char **argv)
{
	const char *p;
	char *concat;
	char **ap;
	intmax_t i;

	if (argc > 1) {
		p = argv[1];
		if (argc > 2) {
			/*
			 * Concatenate arguments.
			 */
			STARTSTACKSTR(concat);
			ap = argv + 2;
			for (;;) {
				while (*p)
					STPUTC(*p++, concat);
				if ((p = *ap++) == NULL)
					break;
				STPUTC(' ', concat);
			}
			STPUTC('\0', concat);
			p = grabstackstr(concat);
		}
	} else
		p = "";

	i = arith(p, line_number);

	out1fmt(ARITH_FORMAT_STR "\n", i);
	return !i;
}
