/*	$NetBSD: arith_token.c,v 1.7 2017/12/17 04:06:03 kre Exp $	*/

/*-
 * Copyright (c) 2002
 *	Herbert Xu.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * From FreeBSD, from dash
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: arith_token.c,v 1.7 2017/12/17 04:06:03 kre Exp $");
#endif /* not lint */

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "arith_tokens.h"
#include "expand.h"
#include "error.h"
#include "memalloc.h"
#include "parser.h"
#include "syntax.h"
#include "show.h"

#if ARITH_BOR + ARITH_ASS_GAP != ARITH_BORASS || \
	ARITH_ASS + ARITH_ASS_GAP != ARITH_EQ
#error Arithmetic tokens are out of order.
#endif

/*
 * Scan next arithmetic token, return its type,
 * leave its value (when applicable) in (global) a_t_val.
 *
 * Input text is in (global) arith_buf which is updated to
 * refer to the next char after the token returned, except
 * on error (ARITH_BAD returned) where arith_buf is not altered.
 */
int
arith_token(void)
{
	int token;
	const char *buf = arith_buf;
	char *end;
	const char *p;

	for (;;) {
		token = *buf;

		if (isdigit(token)) {
			/*
			 * Numbers all start with a digit, and nothing
			 * else does, the number ends wherever
			 * strtoimax() stops...
			 */
			a_t_val.val = strtoimax(buf, &end, 0);
			if (is_in_name(*end)) {
				token = *end;
				while (is_in_name(*++end))
					continue;
				error("arithmetic: unexpected '%c' "
				      "(out of range) in numeric constant: "
				      "%.*s", token, (int)(end - buf), buf);
			}
			arith_buf = end;
			VTRACE(DBG_ARITH, ("Arith token ARITH_NUM=%jd\n",
			    a_t_val.val));
			return ARITH_NUM;

		} else if (is_name(token)) {
			/*
			 * Variable names all start with an alpha (or '_')
			 * and nothing else does.  They continue for the
			 * longest unbroken sequence of alphanumerics ( + _ )
			 */
			arith_var_lno = arith_lno;
			p = buf;
			while (buf++, is_in_name(*buf))
				;
			a_t_val.name = stalloc(buf - p + 1);
			memcpy(a_t_val.name, p, buf - p);
			a_t_val.name[buf - p] = '\0';
			arith_buf = buf;
			VTRACE(DBG_ARITH, ("Arith token ARITH_VAR=\"%s\"\n",
			    a_t_val.name));
			return ARITH_VAR;

		} else switch (token) {
			/*
			 * everything else must be some kind of
			 * operator, white space, or an error.
			 */
		case '\n':
			arith_lno++;
			VTRACE(DBG_ARITH, ("Arith: newline\n"));
			/* FALLTHROUGH */
		case ' ':
		case '\t':
			buf++;
			continue;

		default:
			error("arithmetic: unexpected '%c' (%#x) in expression",
			    token, token);
			/* NOTREACHED */

		case '=':
			token = ARITH_ASS;
 checkeq:
			buf++;
 checkeqcur:
			if (*buf != '=')
				goto out;
			token += ARITH_ASS_GAP;
			break;

		case '>':
			switch (*++buf) {
			case '=':
				token = ARITH_GE;
				break;
			case '>':
				token = ARITH_RSHIFT;
				goto checkeq;
			default:
				token = ARITH_GT;
				goto out;
			}
			break;

		case '<':
			switch (*++buf) {
			case '=':
				token = ARITH_LE;
				break;
			case '<':
				token = ARITH_LSHIFT;
				goto checkeq;
			default:
				token = ARITH_LT;
				goto out;
			}
			break;

		case '|':
			if (*++buf != '|') {
				token = ARITH_BOR;
				goto checkeqcur;
			}
			token = ARITH_OR;
			break;

		case '&':
			if (*++buf != '&') {
				token = ARITH_BAND;
				goto checkeqcur;
			}
			token = ARITH_AND;
			break;

		case '!':
			if (*++buf != '=') {
				token = ARITH_NOT;
				goto out;
			}
			token = ARITH_NE;
			break;

		case 0:
			goto out;

		case '(':
			token = ARITH_LPAREN;
			break;
		case ')':
			token = ARITH_RPAREN;
			break;

		case '*':
			token = ARITH_MUL;
			goto checkeq;
		case '/':
			token = ARITH_DIV;
			goto checkeq;
		case '%':
			token = ARITH_REM;
			goto checkeq;

		case '+':
			if (buf[1] == '+') {
				buf++;
				token = ARITH_INCR;
				break;
			}
			token = ARITH_ADD;
			goto checkeq;
		case '-':
			if (buf[1] == '-') {
				buf++;
				token = ARITH_DECR;
				break;
			}
			token = ARITH_SUB;
			goto checkeq;
		case '~':
			token = ARITH_BNOT;
			break;
		case '^':
			token = ARITH_BXOR;
			goto checkeq;

		case '?':
			token = ARITH_QMARK;
			break;
		case ':':
			token = ARITH_COLON;
			break;
		case ',':
			token = ARITH_COMMA;
			break;
		}
		break;
	}
	buf++;
 out:
	arith_buf = buf;
	VTRACE(DBG_ARITH, ("Arith token: %d\n", token));
	return token;
}
