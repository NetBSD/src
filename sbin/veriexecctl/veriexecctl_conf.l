%{
/*	$NetBSD: veriexecctl_conf.l,v 1.11 2007/05/15 19:47:47 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <prop/proplib.h>

#include "veriexecctl_parse.h"

int yylex(void);

extern size_t line;
extern int verbose;

static char *
dequote(const char *s)
{
	size_t len;
	const char *p;
	char *buf, *q;

	len = 0;
	p = s;
	while (*p) {
		if (*p == '\\' && *(p+1))
			++p;
		++len;
		++p;
	}

	buf = malloc(len + 1);
	if (buf == NULL)
		return NULL;

	p = s;
	q = buf;
	while (*p) {
		if (*p == '\\' && *(p+1))
			++p;
		*q++ = *p++;
	}
	*q++ = '\0';

	return buf;
}
%}

STRING		[0-9a-zA-Z]+
PCHAR           (\\.|[^ \t])

%%

	/* path */
\/{PCHAR}+	{
			if ((yylval.string = dequote(yytext)) == NULL)
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
^#.*\n		|
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

.		{
			if (verbose)
				warnx("Invalid character '%c' in line %zu",
				    *yytext, line);
		}

%%

int
yywrap(void)
{
	return 1;
}
