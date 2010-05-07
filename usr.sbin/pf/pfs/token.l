/* $NetBSD: token.l,v 1.1 2010/05/07 17:41:58 degroote Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: token.l,v 1.1 2010/05/07 17:41:58 degroote Exp $");
#endif

#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>

#include "parse.h"
#include "parser.h"

%}

%option nounput 

%%

state		{ return STATE;}
on			{ return ON;}
out			{ return OUT;}
in			{ return IN;}
proto   	{ return PROTO;}
from		{ return FROM;}
to			{ return TO;}
using   	{ return USING;}
id			{ return ID;}
cid			{ return CID;}
expire  	{ return EXPIRE;}
timeout     { return TIMEOUT;}
src			{ return SRC;}
dst			{ return DST;}
seq			{ return SEQ;}
max_win 	{ return MAX_WIN;}
wscale		{ return WSCALE;}
mss			{ return MSS;}
no-scrub	{ return NOSCRUB;}
scrub		{ return SCRUB;}
flags		{ return FLAGS;}
ttl			{ return TTL;}
mode		{ return MODE;}
[0-9]+		{ char *ep;
			  errno = 0;
			  yylval.num = strtoumax(yytext, &ep, 10);
			  if (errno == ERANGE && yylval.num == UINTMAX_MAX)
					yyfatal("Number out of range");
			  return NUMBER;
			}

[A-Za-z0-9:\[][A-Za-z0-9\[\]_:%\.-]* { yylval.str = strdup(yytext);
							    if (yylval.str == NULL)
									yyfatal("Not enough memory");
								return STRING;
							  }


\n			{ lineno ++; }

%%


void
yyfatal(const char *s)
{
	yyerror(s);
	exit(EXIT_FAILURE);
}

void
yyerror(const char *s)
{
	printf("line %d: %s at [%s]\n", lineno, s, yytext);
}


int
parse(FILE *fp, struct pfioc_states* s)
{
	yyin = fp;

	lineno = 1;

	states = s;
	allocated = 0;
	memset(s, 0, sizeof(*s));

	if (yyparse()) {
		printf("parse failed, line %d.\n", lineno);
		return(-1);
	}

	return(0);
}
