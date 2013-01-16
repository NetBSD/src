/*	$NetBSD: npf_scan.l,v 1.1.4.4 2013/01/16 05:34:10 yamt Exp $	*/

/*-
 * Copyright (c) 2011-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann.
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
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include "npfctl.h"
#include "npf_parse.h"

int	yycolumn;

#define	YY_USER_ACTION	yycolumn += yyleng;

%}

%option noyywrap nounput noinput

ID	[a-zA-Z_][a-zA-Z_0-9]*
NID	[a-zA-Z_0-9]+
NUMBER	[0-9]+

%%
table			return TABLE;
type			return TYPE;
hash			return HASH;
tree			return TREE;
static			return TSTATIC;
dynamic			return TDYNAMIC;
file			return TFILE;
map			return MAP;
"<->"			return ARROWBOTH;
"<-"			return ARROWLEFT;
"->"			return ARROWRIGHT;
"-"			return MINUS;
procedure		return PROCEDURE;
\\\n			yylineno++; yycolumn = 0;
\n			yylineno++; yycolumn = 0; return SEPLINE;
;			return SEPLINE;
name			return NAME;
group			return GROUP;
default			return DEFAULT;
in			return IN;
out			return OUT;
interface		return INTERFACE;
all			return ALL;
block			return BLOCK;
pass			return PASS;
stateful		return STATEFUL;
apply			return APPLY;
final			return FINAL;
quick			return FINAL;
on			return ON;
ifnet			return IFNET;
inet6			return INET6;
inet4			return INET;
inet			return INET;
proto			return PROTO;
family			return FAMILY;
tcp			return TCP;
icmp			{ yylval.num = IPPROTO_ICMP; return ICMP; }
ipv6-icmp		{ yylval.num = IPPROTO_ICMPV6; return ICMP6; }
\"ipv6-icmp\"		{ yylval.num = IPPROTO_ICMPV6; return ICMP6; }
return-rst		return RETURNRST;
return-icmp		return RETURNICMP;
return			return RETURN;
from			return FROM;
to			return TO;
port			return PORT;
flags			return FLAGS;
icmp-type		return ICMPTYPE;
code			return CODE;
any			return ANY;

"/"			return SLASH;
"{"			return CURLY_OPEN;
"}"			return CURLY_CLOSE;
"("			return PAR_OPEN;
")"			return PAR_CLOSE;
","			return COMMA;
"="			return EQ;

"0x"[0-9a-fA-F]+ {
			char *endp, *buf = ecalloc(1, yyleng + 1);
			buf[yyleng] = 0;
			yylval.num = strtoul(buf+2, &endp, 16);
			free(buf);
			return HEX;
		}

{NUMBER}"."{NUMBER} {
			char *endp, *buf = estrndup(yytext, yyleng);
			yylval.fpnum = strtod(buf, &endp);
			free(buf);
			return FPNUM;
		}

[0-9a-fA-F]+":"[0-9a-fA-F:]* {
			yylval.str = estrndup(yytext, yyleng);
			return IPV6ADDR;
		}

{NUMBER}"."[0-9][0-9.]* {
			yylval.str = estrndup(yytext, yyleng);
			return IPV4ADDR;
		}

{NUMBER}	{
			char *endp, *buf = estrndup(yytext, yyleng);
			yylval.num = strtoul(buf, &endp, 10);
			free(buf);
			return NUM;
		}

"<"{NID}">"	{
			yylval.str = estrndup(yytext + 1, yyleng - 2);
			return TABLE_ID;
		}

"$"{NID}	{
			yylval.str = estrndup(yytext + 1, yyleng - 1);
			return VAR_ID;
		}

{ID}		{
			yylval.str = estrndup(yytext, yyleng);
			return IDENTIFIER;
		}

\"[^\"]*\"	{
			yylval.str = estrndup(yytext + 1, yyleng - 2);
			return STRING;
		}

#.*$		/* drop comment until end of line */
[ \t]		/* eat whitespace */

:		return COLON;
