/* 	$NetBSD: config_lex.l,v 1.5 2009/10/28 22:37:50 christos Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
 * All rights reserved.
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

%{

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: config_lex.l,v 1.5 2009/10/28 22:37:50 christos Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <prop/proplib.h>

#include "config_yacc.h"

extern int yyline;
extern int yyerror(const char *, ...);
int yylex(void);

#define YY_NO_UNPUT

%}

%option noyywrap

DEVICEPROP	refresh-timeout
SENSOR		sensor[0-9]+
SENSORPROP	warning-max|warning-min|warning-capacity|critical-max|critical-min|critical-capacity|rfact|description
SP_STRING	[(+|\-)\$A-Za-z\.\/_\-0-9 ]*
STRING		[\$A-Za-z\.\/_\-0-9]*

%%

#.*$ 		/* Eat up comments */
[ \t]+ 		/* Eat up whitespace */
\n		{ yyline++; }
=               { return EQUAL; }
;               { return EOL; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
{DEVICEPROP}	{ yylval.string = strdup(yytext); return DEVICE_PROP; }
{SENSOR}	{ yylval.string = strdup(yytext); return SENSOR; }
{SENSORPROP}	{ yylval.string = strdup(yytext); return SENSOR_PROP; }
\"{SP_STRING}\" { yylval.string = strdup(yytext + 1);
		  yylval.string[strlen(yytext) - 2] = '\0'; return STRING; }
{STRING}	{ yylval.string = strdup(yytext); return STRING; }
.		{ yyerror("illegal token `%s'", yytext); }
