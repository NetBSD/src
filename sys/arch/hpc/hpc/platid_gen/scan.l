%{
/*	$NetBSD: scan.l,v 1.2.2.3 2001/03/12 13:28:10 bouyer Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#include <strings.h>
#include "gram.h"

#include "platid_gen.h"

int	yyline;
char	*getstr __P((char*));
void	getcomment __P((void));

#define	yywrap() 1

%}

%%
#[^\n]*\n		{ yylval.str = strdup(yytext); yyline++; return DIRECTIVE; }
\"([^"]|\\\")*\"	{ yylval.str = getstr(yytext); return NAME; }
[a-zA-Z][0-9a-zA-Z]*	{ yylval.str = strdup(yytext); return FSYM; }
[a-zA-Z][_0-9a-zA-Z]*	{ yylval.str = strdup(yytext); return MOD; }
[0-9a-zA-Z]+	{ yylval.str = strdup(yytext); return SYM; }
\n		{ yyline++; }
"/*"		{ getcomment(); }
[ \t]+		{ /* ignored */; }
.		{ return yytext[0]; }

%%

char*
getstr(char*s)
{
	char *res, *p, *p2;
	res = strdup(s + 1);
	for (p = p2 = res; *p != '\"'; p++) {
		if (*p == '\\') {
			*p2++ = *++p;
		} else {
			*p2++ = *p;
		}
	}
	*p2 = '\0';
	return (res);
}

void
getcomment()
{
	int c;

	for ( ; ; ) {
		while ( (c = input()) != '*' && c != EOF ) {
			if (c == '\n') yyline++;
		}
		if (c == '\n') yyline++;

		if ( c == '*' ) {
			while ( (c = input()) == '*' ) {
				if (c == '\n') yyline++;
			}
			if (c == '\n') yyline++;
			if ( c == '/' )
				break;    /* found the end */
		}

		if ( c == EOF ) {
			yyerror( "EOF in comment" );
			break;
		}
	}
}

read_def()
{
	yyline = 1;
	def_tree = NULL;
	return yyparse();
}
