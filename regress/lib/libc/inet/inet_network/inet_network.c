/*	$NetBSD: inet_network.c,v 1.1 2008/01/19 04:12:21 ginsbach Exp $	*/
/*
 * Copyright (c) 2008, The NetBSD Foundation.
 * All Rights Reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#define WS	"\t\n "

#define ISODIGIT(c)	((int)(c) >= '0' && (int)(c) <= '7')

char *unescape(char *);
char *unquote(char *);

int
main(int argc, char *argv[])
{
	size_t len, lineno = 0;
	char *ep, *line, *input = NULL, *result = NULL;
	struct in_addr addr;
	in_addr_t s_addr;

	while ((line = fparseln(stdin, &len, &lineno, NULL, 0)) != NULL) {
		if (strncmp(line, "input:", 6) == 0) {
			if (input)
				free(input);
			input = &line[7 + strspn(&line[7], WS)];
			if (input == NULL)
				errx(1, "missing input at line %ld",
				     (unsigned long)lineno);
			input = strdup(unescape(unquote(input)));
		} else if (strncmp(line, "result:", 7) == 0) {
			if (result)
				free(result);
			result = &line[8 + strspn(&line[8], WS)];
			if (result == NULL)
				errx(1, "missing result at line %ld",
				     (unsigned long)lineno);
			if (result == NULL)
				errx(1,
				     "result without input at line %ld");
			result = strdup(result);

			addr.s_addr = inet_network(input);

			errno = 0;
			s_addr = strtoul(result, &ep, 0);
			if (errno == ERANGE || errno == EINVAL)
				err(1, "line %ld: strtoul(): %s",
			       	    (unsigned long)lineno, result);

			if (addr.s_addr != s_addr)
#ifdef DEBUG
				warnx("0x%08x != 0x%08x [%s]\tFAILED",
			             addr.s_addr, s_addr, input);
			else
				warnx("0x%08x == 0x%08x [%s]\tPASSED",
			             addr.s_addr, s_addr, input);
#else
				errx(1, "0x%08x != 0x%08x [%s]",
			             addr.s_addr, s_addr, input);
#endif
		} else
			errx(1, "unknown directive at line %ld",
			    (unsigned long)lineno);
		free(line);
	}
	exit(0);
}

/*
 * unquote
 */
char *
unquote(char *orig)
{
	char c, *cp, *new = orig;
	int escaped = 0;

	for (cp = orig; (*orig = *cp); cp++) {
		if (*cp == '\'' || *cp == '"')
			if (!escaped)
				continue;
		escaped = (*cp == '\\');
		++orig;
	}

	return (new);
}

/*
 * unescape - handle C escapes in a string
 */
char *
unescape(char *orig)
{
	char c, *cp, *new = orig;
	int i;

	for (cp = orig; (*orig = *cp); cp++, orig++) {
		if (*cp != '\\')
			continue;

		switch (*++cp) {
		case 'a':	/* alert (bell) */
			*orig = '\a';
			continue;
		case 'b':	/* backspace */
			*orig = '\b';
			continue;
		case 'e':	/* escape */
			*orig = '\e';
			continue;
		case 'f':	/* formfeed */
			*orig = '\f';
			continue;
		case 'n':	/* newline */
			*orig = '\n';
			continue;
		case 'r':	/* carriage return */
			*orig = '\r';
			continue;
		case 't':	/* horizontal tab */
			*orig = '\t';
			continue;
		case 'v':	/* vertical tab */
			*orig = '\v';
			continue;
		case '\\':	/* backslash */
			*orig = '\\';
			continue;
		case '\'':	/* single quote */
			*orig = '\'';
			continue;
		case '\"':	/* double quote */
			*orig = '"';
			continue;
		case '0':
		case '1':
		case '2':
		case '3':	/* octal */
		case '4':
		case '5':
		case '6':
		case '7':	/* number */
			for (i = 0, c = 0;
			     ISODIGIT((unsigned char)*cp) && i < 3;
			     i++, cp++) {
				c <<= 3;
				c |= (*cp - '0');
			}
			*orig = c;
			--cp;
			continue;
		case 'x':	/* hexidecimal number */
			cp++;	/* skip 'x' */
			for (i = 0, c = 0;
			     isxdigit((unsigned char)*cp) && i < 2;
			     i++, cp++) {
				c <<= 4;
				if (isdigit((unsigned char)*cp))
					c |= (*cp - '0');
				else
					c |= ((toupper((unsigned char)*cp) -
					    'A') + 10);
			}
			*orig = c;
			--cp;
			continue;
		default:
			--cp;
			break;
		}
	}

	return (new);
}
