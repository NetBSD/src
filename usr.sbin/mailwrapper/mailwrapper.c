/*	$NetBSD: mailwrapper.c,v 1.2 1999/02/20 22:10:07 thorpej Exp $	*/

/*
 * Copyright (c) 1998
 * 	Perry E. Metzger.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#define _PATH_MAILERCONF	"/etc/mailer.conf"

int main __P((int, char *[], char *[]));

extern const char *__progname;	/* from crt0.o */

int
main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	FILE *config;
	char *line, *cp, *from, *to;
	size_t len, lineno = 0;

	if ((config = fopen(_PATH_MAILERCONF, "r")) == NULL)
		err(1, "mailwrapper: can't open %s", _PATH_MAILERCONF);

	for (;;) {
		if ((line = fparseln(config, &len, &lineno, NULL, 0)) == NULL) {
			if (feof(config))
				errx(1, "mailwrapper: no mapping in %s",
				    _PATH_MAILERCONF);
			err(1, "mailwrapper");
		}

#define	WHITESPACE	" \t"
#define	PARSE_ERROR \
		errx(1, "mailwrapper: parse error in %s at line %lu", \
		    _PATH_MAILERCONF, (u_long)lineno)

		cp = line;

		cp += strspn(cp, WHITESPACE);
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}

		if ((from = strsep(&cp, WHITESPACE)) == NULL)
			PARSE_ERROR;

		cp += strspn(cp, WHITESPACE);

		if ((to = strsep(&cp, WHITESPACE)) == NULL)
			PARSE_ERROR;

		if (cp != NULL) {
			cp += strspn(cp, WHITESPACE);
			if (cp[0] != '\0')
				PARSE_ERROR;
		}

		if (strcmp(from, __progname) == 0)
			break;

		free(line);
	}

	fclose(config);

	execve(to, argv, envp);

	err(1, "mailwrapper: execing %s", to);
}
