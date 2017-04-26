/*	$NetBSD: arith_tokens.h,v 1.1.4.2 2017/04/26 02:52:13 pgoyette Exp $	*/

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
 * From FreeBSD who obtained it from dash (modified both times.)
 */

/*
 * Tokens returned from arith_token()
 *
 * Caution, values are not arbitrary.
 */

#define	ARITH_BAD	0

#define	ARITH_ASS	1

#define	ARITH_OR	2
#define	ARITH_AND	3
#define	ARITH_NUM	5
#define	ARITH_VAR	6
#define	ARITH_NOT	7

#define	ARITH_BINOP_MIN	8

#define	ARITH_LE	8
#define	ARITH_GE	9
#define	ARITH_LT	10
#define	ARITH_GT	11
#define	ARITH_EQ	12	/* Must be ARITH_ASS + ARITH_ASS_GAP */

#define	ARITH_ASS_BASE	13

#define	ARITH_REM	13
#define	ARITH_BAND	14
#define	ARITH_LSHIFT	15
#define	ARITH_RSHIFT	16
#define	ARITH_MUL	17
#define	ARITH_ADD	18
#define	ARITH_BOR	19
#define	ARITH_SUB	20
#define	ARITH_BXOR	21
#define	ARITH_DIV	22

#define	ARITH_NE	23

#define	ARITH_BINOP_MAX	24

#define	ARITH_ASS_MIN	ARITH_BINOP_MAX
#define	ARITH_ASS_GAP	(ARITH_ASS_MIN - ARITH_ASS_BASE)

#define	ARITH_REMASS	(ARITH_ASS_GAP + ARITH_REM)
#define	ARITH_BANDASS	(ARITH_ASS_GAP + ARITH_BAND)
#define	ARITH_LSHIFTASS	(ARITH_ASS_GAP + ARITH_LSHIFT)
#define	ARITH_RSHIFTASS	(ARITH_ASS_GAP + ARITH_RSHIFT)
#define	ARITH_MULASS	(ARITH_ASS_GAP + ARITH_MUL)
#define	ARITH_ADDASS	(ARITH_ASS_GAP + ARITH_ADD)
#define	ARITH_BORASS	(ARITH_ASS_GAP + ARITH_BOR)
#define	ARITH_SUBASS	(ARITH_ASS_GAP + ARITH_SUB)
#define	ARITH_BXORASS	(ARITH_ASS_GAP + ARITH_BXOR)
#define	ARITH_DIVASS	(ARITH_ASS_GAP + ARITH_BXOR)

#define	ARITH_ASS_MAX	34

#define	ARITH_LPAREN	34
#define	ARITH_RPAREN	35
#define	ARITH_BNOT	36
#define	ARITH_QMARK	37
#define	ARITH_COLON	38

/*
 * Globals shared between arith parser, and lexer
 */

extern const char *arith_buf;

union a_token_val {
	intmax_t val;
	char *name;
};

extern union a_token_val a_t_val;

int arith_token(void);
