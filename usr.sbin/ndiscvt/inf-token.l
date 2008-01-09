%{
/*
 * Copyright (c) 2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/usr.sbin/ndiscvt/inf-token.l,v 1.3 2004/01/11 21:10:35 mdodd Exp $");
#endif
#ifdef __NetBSD__
__RCSID("$NetBSD: inf-token.l,v 1.6.10.1 2008/01/09 02:02:11 matt Exp $");
#endif

#include <regex.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inf-parse.h"

int lineno = 1;
#define YY_NO_UNPUT

int input_is_unicode = 0;
#define IS_UNICODE 	1
#define ISNT_UNICODE	2
static size_t opt_unicode_input(char *buf, size_t max_size);
 
#define YY_INPUT(buf, result, max_size)	\
	result = opt_unicode_input(buf, max_size)

int yylex(void);
void ndiscvt_error(const char *);
int ndiscvt_wrap(void);

static void
update_lineno(const char *cp)
{
	while (*cp)
		if (*cp++ == '\n')
			lineno++;
}

%}

%%

[ \t]+			;
\n			{ lineno++; return EOL; }
\r			;
;.*$			;
\/\/.*$			;
=			{ return EQUALS; }
,			{ return COMMA; }
\"(\\\"|[^"]|\"\")*\"	{
			int len = strlen(yytext) - 2;
			int blen = len + 1;
			char *walker;
			int i;
			update_lineno(yytext);
			ndiscvt_lval.str = (char *)malloc(blen);
			if (ndiscvt_lval.str == NULL)
				goto out;
			walker = ndiscvt_lval.str;
			for (i = 1; i <= len; i++) {
				if (yytext[i] == '\"') {
					switch (yytext[i + 1]) {
					case '\"':
						i++;
						break;
					default:
						break;
					}
				}
				if (yytext[i] == '\\') {
					switch (yytext[i + 1]) {
					case '\n':
						i += 2;
						while(isspace(
						    (unsigned char)yytext[i]))
							i++;
						break;
					case '\"':
						i++;
						break;
					case '(':
						i++;
						break;
					default:
						break;
					}
				}
				*walker++ = yytext[i];
			}
			*walker++ = '\0';
			out:;
			return STRING;
			}
\[[a-zA-Z0-9%&\{\}\-\.\/_\\\*\ ]+\]	{
				int len = strlen(yytext);
				yytext[len-1] = '\0';
				ndiscvt_lval.str = strdup(yytext+1);
				return SECTION;
			}
[a-zA-Z0-9%&\{\}\-\.\/_\\\*]+		{
				ndiscvt_lval.str = strdup(yytext);
				return WORD;
			}
%%

void
ndiscvt_error(const char *s)
{
	errx(1, "line %d: %s%s %s.", lineno, yytext, yytext?":":"", s);
}

int
ndiscvt_wrap(void)
{
	return 1;
}

static size_t
opt_unicode_input(char *buf, size_t max_size)
{
	size_t i, rb;
	char *t = buf;

	if (feof(yyin)) return YY_NULL;
	if (input_is_unicode == ISNT_UNICODE)
		/* standard ASCII .inf file */
		return fread(buf, 1, max_size, yyin);

	if (input_is_unicode != IS_UNICODE) {
		/* we don't know yet if this file is unicode */
		if (max_size < 2) {
			fprintf(stderr, "internal error: can not determine "
			    "unicode with max_size %zd\n", max_size);
			return YY_NULL;
		}
		rb = fread(buf, 2, 1, yyin);
		if (!rb) 
			return YY_NULL;
		if ((unsigned char)buf[0] == 0xff
		    && (unsigned char)buf[1] == 0xfe) {
			input_is_unicode = IS_UNICODE;
		} else {
			input_is_unicode = ISNT_UNICODE;
			return 2;
		}
	}

	if (input_is_unicode != IS_UNICODE)
		return YY_NULL;

	if (max_size < 2) {
		fprintf(stderr, "internal error: can not read small unicode "
		    "buffer\n");
		return YY_NULL;
	}
	/*
	 * read unicode string and skip every second byte if it is zero,
	 * replace it by an innocent 'A' otherwise. Poor man's
	 * version of "iconv -f unicode -t ascii" - should be good enough
	 * for INF files (maybe besides comments, which we don't care
	 * about)
	 */
	rb = fread(buf, 1, max_size&~1, yyin);
	for (i = 0; i < rb; i += 2) {
		if (buf[i+1] != 0)
			*t++ = 'A';
		else
			*t++ = buf[i];
	}
	return t-buf;
}
