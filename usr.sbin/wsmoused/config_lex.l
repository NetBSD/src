/* $NetBSD: config_lex.l,v 1.4 2003/08/06 22:11:49 jmmv Exp $ */

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

%{

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: config_lex.l,v 1.4 2003/08/06 22:11:49 jmmv Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>

#include "wsmoused.h"
#include "config_yacc.h"

extern int yyline;

extern int yyerror(const char *fmt, ...);
int yylex(void);

%}

%option noyywrap

STRING		[\$A-Za-z\.\/_\-0-9]*
SP_STRING	[\$A-Za-z\.\/_\-0-9 ]*
MODE_PROPS	button_[0-9]+_down|button_[0-9]+_up|device|fifo|lefthanded|modes|nodaemon|pidfile|slowdown_x|slowdown_y|ttystat|xconsole

%%

#.*$		/* Eat up comments */
[ \t]+		/* Eat up whitespace */
\n		{ yyline++; }

=		{ return TK_EQUAL; }
;		{ return TK_EOL; }
"{"		{ return TK_LBRACE; }
"}"		{ return TK_RBRACE; }
mode		{ return TK_MODE; }
{MODE_PROPS}	{ yylval.string = strdup(yytext); return TK_MODEPROP; }
\"{SP_STRING}\"	{ yylval.string = strdup(yytext + 1);
		  yylval.string[strlen(yytext) - 2] = '\0'; return TK_STRING; }
{STRING}	{ yylval.string = strdup(yytext); return TK_STRING; }

.		{ yyerror("illegal token `%s'", yytext); }
