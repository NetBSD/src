%{
/* $NetBSD: scan.l,v 1.138 2023/01/09 00:07:30 rillig Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All Rights Reserved.
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *      The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#if defined(__RCSID)
__RCSID("$NetBSD: scan.l,v 1.138 2023/01/09 00:07:30 rillig Exp $");
#endif

#include "lint1.h"
#include "cgram.h"

%}


HEX	[0-9A-Fa-f]
EXP	([eE][+-]?[0-9]+)
PEXP	(p[+-]?[0-9A-Fa-f]+)
FSUF	([fFlL]?[i]?)

%option nounput

%%

[_A-Za-z][_A-Za-z0-9]*		return lex_name(yytext, yyleng);
0[bB][01]+[lLuU]*		return lex_integer_constant(yytext, yyleng, 2);
0[0-7]*[lLuU]*			return lex_integer_constant(yytext, yyleng, 8);
[1-9][0-9]*[lLuU]*		return lex_integer_constant(yytext, yyleng, 10);
0[xX]{HEX}+[lLuU]*		return lex_integer_constant(yytext, yyleng, 16);
[0-9]+\.[0-9]*{EXP}?{FSUF}	|
[0-9]+{EXP}{FSUF}		|
0[xX]{HEX}+\.{HEX}*{PEXP}{FSUF}	|
0[xX]{HEX}+{PEXP}{FSUF}		|
\.[0-9]+{EXP}?{FSUF}		return lex_floating_constant(yytext, yyleng);
"="				return T_ASSIGN;
"*="				return lex_operator(T_OPASSIGN, MULASS);
"/="				return lex_operator(T_OPASSIGN, DIVASS);
"%="				return lex_operator(T_OPASSIGN, MODASS);
"+="				return lex_operator(T_OPASSIGN, ADDASS);
"-="				return lex_operator(T_OPASSIGN, SUBASS);
"<<="				return lex_operator(T_OPASSIGN, SHLASS);
">>="				return lex_operator(T_OPASSIGN, SHRASS);
"&="				return lex_operator(T_OPASSIGN, ANDASS);
"^="				return lex_operator(T_OPASSIGN, XORASS);
"|="				return lex_operator(T_OPASSIGN, ORASS);
"||"				return T_LOGOR;
"&&"				return T_LOGAND;
"|"				return T_BITOR;
"&"				return T_AMPER;
"^"				return T_BITXOR;
"=="				return lex_operator(T_EQUALITY, EQ);
"!="				return lex_operator(T_EQUALITY, NE);
"<"				return lex_operator(T_RELATIONAL, LT);
">"				return lex_operator(T_RELATIONAL, GT);
"<="				return lex_operator(T_RELATIONAL, LE);
">="				return lex_operator(T_RELATIONAL, GE);
"<<"				return lex_operator(T_SHIFT, SHL);
">>"				return lex_operator(T_SHIFT, SHR);
"++"				return lex_operator(T_INCDEC, INC);
"--"				return lex_operator(T_INCDEC, DEC);
"->"				return T_ARROW;
"."				return T_POINT;
"+"				return lex_operator(T_ADDITIVE, PLUS);
"-"				return lex_operator(T_ADDITIVE, MINUS);
"*"				return T_ASTERISK;
"/"				return lex_operator(T_MULTIPLICATIVE, DIV);
"%"				return lex_operator(T_MULTIPLICATIVE, MOD);
"!"				return T_LOGNOT;
"~"				return T_COMPLEMENT;
"\""				return lex_string();
"L\""				return lex_wide_string();
";"				return T_SEMI;
"{"				return T_LBRACE;
"}"				return T_RBRACE;
","				return T_COMMA;
":"				return T_COLON;
"?"				return T_QUEST;
"["				return T_LBRACK;
"]"				return T_RBRACK;
"("				return T_LPAREN;
")"				return T_RPAREN;
"..."				return T_ELLIPSIS;
"'"				return lex_character_constant();
"L'"				return lex_wide_character_constant();
^#.*$				lex_directive(yytext);
\n				lex_next_line();
\t|" "|\f|\v			;
"/*"				lex_comment();
"//"				lex_slash_slash_comment();
.				lex_unknown_character(yytext[0]);

%%

/*
 * In the above list of regular expressions, the tokens for character
 * constants, string literals and comments are incomplete; they only match
 * a prefix.  The remainder of these tokens is scanned by reading bytes
 * directly from the input stream.
 */
int
lex_input(void)
{
	return input();
}
