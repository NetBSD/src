/*	$NetBSD: timedc.c,v 1.21.2.1 2012/04/17 00:09:54 yamt Exp $	*/

/*-
 * Copyright (c) 1985, 1993 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1985, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)timedc.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: timedc.c,v 1.21.2.1 2012/04/17 00:09:54 yamt Exp $");
#endif
#endif /* not lint */

#include "timedc.h"
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <err.h>

int trace = 0;
FILE *fd = 0;
int	margc;
int	fromatty;
#define MAX_MARGV	20
char	*margv[MAX_MARGV];
char	cmdline[200];
jmp_buf	toplevel;
static const struct cmd *getcmd(char *);
static int drop_privileges(void);

int
main(int argc, char *argv[])
{
	const struct cmd *c;

	fcntl(3, F_CLOSEM);
	openlog("timedc", 0, LOG_AUTH);

	/*
	 * security dictates!
	 */
	if (priv_resources() < 0)
		errx(EXIT_FAILURE, "Could not get privileged resources");
	if (drop_privileges() < 0)
		errx(EXIT_FAILURE, "Could not drop privileges");

	if (--argc > 0) {
		c = getcmd(*++argv);
		if (c == (struct cmd *)-1) {
			printf("?Ambiguous command\n");
			exit(EXIT_FAILURE);
		}
		if (c == 0) {
			printf("?Invalid command\n");
			exit(EXIT_FAILURE);
		}
		(*c->c_handler)(argc, argv);
		exit(EXIT_SUCCESS);
	}

	fromatty = isatty(fileno(stdin));
	if (setjmp(toplevel))
		putchar('\n');
	(void) signal(SIGINT, intr);
	for (;;) {
		if (fromatty) {
			printf("timedc> ");
			(void) fflush(stdout);
		}
		if (fgets(cmdline, sizeof(cmdline), stdin) == NULL)
			quit(0, NULL);
		if (cmdline[0] == 0)
			break;
		if (makeargv()) {
			printf("?Too many arguments\n");
			continue;
		}
		if (margv[0] == 0)
			continue;
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			printf("?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			printf("?Invalid command\n");
			continue;
		}
		(*c->c_handler)(margc, margv);
	}
	return 0;
}

void
intr(int signo)
{
	(void) signo;
	if (!fromatty)
		exit(EXIT_SUCCESS);
	longjmp(toplevel, 1);
}


static const struct cmd *
getcmd(char *name)
{
	const char *p;
	char *q;
	const struct cmd *c, *found;
	int nmatches, longest;
	extern const struct cmd cmdtab[];
	extern int NCMDS;

	longest = 0;
	nmatches = 0;
	found = 0;
	for (c = cmdtab; c < &cmdtab[NCMDS]; c++) {
		p = c->c_name;
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return(c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return((struct cmd *)-1);
	return(found);
}

/*
 * Slice a string up into argc/argv.
 */
int
makeargv(void)
{
	char *cp;
	char **argp = margv;

	margc = 0;
	for (cp = cmdline; argp < &margv[MAX_MARGV - 1] && *cp;) {
		while (isspace((unsigned char)*cp))
			cp++;
		if (*cp == '\0')
			break;
		*argp++ = cp;
		margc += 1;
		while (*cp != '\0' && !isspace((unsigned char)*cp))
			cp++;
		if (*cp == '\0')
			break;
		*cp++ = '\0';
	}
	if (margc == MAX_MARGV - 1)
		return 1;
	*argp++ = 0;
	return 0;
}

#define HELPINDENT (sizeof ("directory"))

/*
 * Help command.
 */
void
help(int argc, char *argv[])
{
	const struct cmd *c;
	extern const struct cmd cmdtab[];

	if (argc == 1) {
		int i, j, w;
		int columns, width = 0, lines;
		extern int NCMDS;

		printf("Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c < &cmdtab[NCMDS]; c++) {
			int len = strlen(c->c_name);

			if (len > width)
				width = len;
		}
		width = (width + 8) &~ 7;
		columns = 80 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			for (j = 0; j < columns; j++) {
				c = cmdtab + j * lines + i;
				printf("%s", c->c_name);
				if (c + lines >= &cmdtab[NCMDS]) {
					printf("\n");
					break;
				}
				w = strlen(c->c_name);
				while (w < width) {
					w = (w + 8) &~ 7;
					putchar('\t');
				}
			}
		}
		return;
	}
	while (--argc > 0) {
		char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			printf("?Ambiguous help command %s\n", arg);
		else if (c == (struct cmd *)0)
			printf("?Invalid help command %s\n", arg);
		else
			printf("%-*s\t%s\n", (int)HELPINDENT,
				c->c_name, c->c_help);
	}
}

static int
drop_privileges(void)
{
	static const char user[] = "_timedc";
	const struct passwd *pw;
	uid_t uid;
	gid_t gid;

	if ((pw = getpwnam(user)) == NULL) {
		warnx("getpwnam(\"%s\") failed", user);
		return -1;
	}
	uid = pw->pw_uid;
	gid = pw->pw_gid;
	if (setgroups(1, &gid)) {
		warn("setgroups");
		return -1;
	}
	if (setgid(gid)) {
		warn("setgid");
		return -1;
	}
	if (setuid(uid)) {
		warn("setuid");
		return -1;
	}
	return 0;
}
