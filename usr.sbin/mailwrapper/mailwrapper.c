/*	$NetBSD: mailwrapper.c,v 1.1 1998/12/25 22:06:59 perry Exp $	*/

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

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define _PATH_MAILERCONF	"/etc/mailer.conf"

int main __P((int, char *[], char *[]));

int
main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	FILE *config;
	char buf[BUFSIZ], from[BUFSIZ], to[BUFSIZ], orig[BUFSIZ];
	char *tmp;
	
	if ((config = fopen(_PATH_MAILERCONF, "r")) == NULL)
		err(1, "mailswitch: can't open %s", _PATH_MAILERCONF);

	if (strlen(argv[0]) > (BUFSIZ - 1))
		errx(1, "mailswitch: invoking command pathname is too long.");
	
	tmp = strrchr(argv[0], '/');
	if (tmp == NULL)
		strcpy(orig, argv[0]);
	else
		strcpy(orig, ++tmp);

	while (1) {
		if (fgets(buf, BUFSIZ, config) == NULL)
			errx(1, "mailswitch: %s contains no mapping for %s",
			    _PATH_MAILERCONF, orig);
		
		if (buf[0] == '#')
			continue;

		if (sscanf(buf, "%s %s", from, to) != 2)
			continue;

		if (strcmp(from, orig) == 0)
			break;
	}

	fclose(config);

	execve(to, argv, envp);
	
	err(1, "mailswitch: execing %s:", to);
}
