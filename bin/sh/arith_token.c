/*	$NetBSD: arith_token.c,v 1.1 2017/03/20 11:26:07 kre Exp $	*/

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
__RCSID("$NetBSD: arith_token.c,v 1.1 2017/03/20 11:26:07 kre Exp $");
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

		switch (token) {
		case ' ':
		case '\t':
		case '\n':
			buf++;
			continue;

		default:
			error("arithmetic: unexpected '%c' (%#x) in expression",
			    token, token);
			/* NOTREACHED */

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			a_t_val.val = strtoimax(buf, &end, 0);
			arith_buf = end;
			return ARITH_NUM;

		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		case '_':
			p = buf;
			while (buf++, is_in_name(*buf))
				;
			a_t_val.name = stalloc(buf - p + 1);
			memcpy(a_t_val.name, p, buf - p);
			a_t_val.name[buf - p] = '\0';
			arith_buf = buf;
			return ARITH_VAR;

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
			if (buf[1] == '+')
				error("arithmetic: ++ operator unsupported");
			token = ARITH_ADD;
			goto checkeq;
		case '-':
			if (buf[1] == '-')
				error("arithmetic: -- operator unsupported");
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
		}
		break;
	}
	buf++;
 out:
	arith_buf = buf;
	return token;
}
