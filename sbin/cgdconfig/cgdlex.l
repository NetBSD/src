%{
/* $NetBSD: cgdlex.l,v 1.1 2003/03/24 02:02:50 elric Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: cgdlex.l,v 1.1 2003/03/24 02:02:50 elric Exp $");
#endif

#include <err.h>
#include <stdio.h>

#include "utils.h"
#include "cgdparse.h"

/*
 * We use macros here to separate the C from the tokeniser, to
 * ease reading each.
 */

#define	RETSTRING(x)	do {					\
		yylval.string = string_new(yytext, yyleng);	\
		return (x);					\
	} while (0)
#define RETTOKEN(x)	do {					\
		yylval.token.text = yytext;			\
		yylval.token.length = yyleng;			\
		return (x);					\
	} while (0)
#define RETINTEGER(x)	do {					\
		yylval.integer = atoi(yytext);			\
		return (x);					\
	} while (0)

int yylineno;

void	yyerror(const char *);
int	yylex(void);
%}

%%

[0-9]+					{ RETINTEGER(INTEGER); }
algorithm				{ RETTOKEN(ALGORITHM); }
keylength				{ RETTOKEN(KEYLENGTH); }
iv-method				{ RETTOKEN(IVMETHOD); }
verify_method				{ RETTOKEN(VERIFY_METHOD); }
keygen					{ RETTOKEN(KEYGEN); }
salt					{ RETTOKEN(SALT); }
iterations				{ RETTOKEN(ITERATIONS); }
key					{ RETTOKEN(KEY); }
keygen_method				{ RETTOKEN(KEYGEN_METHOD); }
keygen_salt				{ RETTOKEN(KEYGEN_SALT); }
keygen_iterations			{ RETTOKEN(KEYGEN_ITERATIONS); }
xor_key					{ RETTOKEN(XOR_KEY); }
[;\n]					{ return EOL; }
\\\n					/* ignore a quoted nl */
[ \t]					/* ignore spaces and tabs */
#[^;\n]*				/* ignore comments */
[^ }{\t\n;]+				{ RETSTRING(STRINGLIT); }
.					{ return yytext[0]; }

%%

void
yyerror(const char *msg)
{

         warnx("%s line %d: %s at '%s'", "foo", yylineno, msg, yytext);
}
