%{
/*	$NetBSD: nslexer.l,v 1.1.4.1 1997/05/23 21:14:50 lukem Exp $	*/

/*-
 * Copyright (c) 1997 Luke Mewburn <lukem@netbsd.org>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by Luke Mewburn.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <string.h>

#include "nsswitch.h"
#include "nsparser.tab.h"
%}

BLANK		[ \t]
CR		\n
DBNAME		[a-zA-Z][a-zA-Z0-9_]*

%option yylineno

%%

{BLANK}+	;	/* skip whitespace */

#.*		;	/* nuke comments */

\\{CR}		;	/* allow continuation */

{CR}		return NL;

[sS][uU][cC][cC][eE][sS][sS]		return SUCCESS;
[uU][nN][aA][vV][aA][iI][lL]		return UNAVAIL;
[nN][oO][tT][fF][oO][uU][nN][dD]	return NOTFOUND;
[tT][rR][yY][aA][gG][aA][iI][nN]	return TRYAGAIN;

[rR][eE][tT][uU][rR][nN]		return RETURN;
[cC][oO][nN][tT][iI][nN][uU][eE]	return CONTINUE;

[fF][iI][lL][eE][sS]			return FILES;
[dD][nN][sS]				return DNS;
[nN][iI][sS]				return NIS;
[nN][iI][sS][pP][lL][uU][sS]		return NISPLUS;
[cC][oO][mM][pP][aA][tT]		return COMPAT;

{DBNAME}	{
			int i, c;

			for (i = 0; i < NS_MAXDBLEN; i++) {
				c = yytext[i];
				if (isupper(c))
					c = tolower(c);
				_nsyylval.dbstr[i] = c;
				if (! yytext[i])
					break;
			}
			_nsyylval.dbstr[NS_MAXDBLEN - 1] = '\0';
			return DATABASE;
		}

[:=\[\]]	return yytext[0];

.		;	/* ignore all else */

%%

#undef _nsyywrap
int
_nsyywrap()
{
	return 1;
} /* _nsyywrap */

void
_nsyyerror(msg)
	char *msg;
{

		/* XXX: this should end up in syslog maybe? */
	 fprintf(stderr, "line %d: %s at '%s'\n", yylineno, msg, yytext);
} /* _nsyyerror */
