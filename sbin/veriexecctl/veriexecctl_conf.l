%{
/*	$NetBSD: veriexecctl_conf.l,v 1.7 2006/01/05 20:16:20 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@bsd.org.il>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org> 
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/verified_exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "veriexecctl_parse.h"
#include "veriexecctl.h"

u_int64_t entries;
char *filename;
%}

STRING		[0-9a-zA-Z]+

%%

	/* path */
\/[^ \t]+	{
			if ((yylval.string = strdup(yytext)) == NULL)
				err(1, "Cannot allocate string");
			return PATH;
		}

	/* string (fingerprint, type, options) */
{STRING} 	{
			if ((yylval.string = strdup(yytext)) == NULL)
				err(1, "Cannot allocate string");
			return STRING;
		}

	/* comments, white-outs */
[ \t\r]		|
#.*		;
#.*\n		|
\\\n		|
^\n		{ line++; }

	/* eol on a line with data. need a call to ioctl, return eol */
\n		{
			line++;
			return EOL;
		}

","		{
			return TOKEN_COMMA;
		}

.		{ yyerror("Invalid character"); }

%%

int
yywrap(void)
{
	return 1;
}

void
yyerror(const char *string)
{
	(void)fprintf(stderr, "%s: %s at \"%s\", line %zu\n", getprogname(),
	    string, yytext, line);
}
