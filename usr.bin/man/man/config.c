/*
 * Copyright (c) 1989 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 * PATCHES MAGIC                LEVEL   PATCH THAT GOT US HERE
 * --------------------         -----   ----------------------
 * CURRENT PATCH LEVEL:         1       00008
 * --------------------         -----   ----------------------
 *
 * 03 Sep 92	James Dolter		Fixed 1K pathbuf bug
 */

#ifndef lint
static char sccsid[] = "@(#)config.c	5.6 (Berkeley) 3/1/91";
#endif /* not lint */

#include <sys/param.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <err.h>
#include "pathnames.h"

extern void enomem();
extern char *progname;
char *pathbuf, **arorder;

static FILE *cfp;

static void
openconfig()
{
	if (cfp) {
		rewind(cfp);
		return;
	}
	if (!(cfp = fopen(_PATH_MANCONF, "r"))) {
		err (1, "can't open configuration file %s.", _PATH_MANCONF);
	}
}


/*
 * getpath --
 *	read in the configuration file, calling a function with the line
 *	from each matching section.
 */
char *
getpath(sects)
	char **sects;
{
	register char **av, *p;
	size_t len;
	char *line;

	openconfig();
	while ((line = fgetline (cfp, (size_t *) 0))) {
		p = strtok(line, " \t\n");
		if (!p || *p == '#')
			continue;
		for (av = sects; *av; ++av)
			if (!strcmp(p, *av))
				break;
		if (!*av)
			continue;
		while (p = strtok((char *)NULL, " \t")) {
			len = strlen(p);
			if (p[len - 1] == '/')
				for (av = arorder; *av; ++av)
                			cadd(p, len, *av);
			else
				cadd(p, len, (char *)NULL);
		}
	}
	return(pathbuf);
}

cadd(add1, len1, add2)
char *add1, *add2;
register size_t len1;
{
	static size_t buflen;
	static char *bp, *endp;
	register size_t len2;
	register size_t	oldlen;				/* 03 Sep 92*/

	len2 = add2 ? strlen(add2) : 0;
	if (!bp || bp + len1 + len2 + 2 >= endp) {
		oldlen = bp - pathbuf;			/* 03 Sep 92*/
		if (!(pathbuf = realloc(pathbuf, buflen += 1024)))
			enomem();
		bp = pathbuf + oldlen;			/* 03 Sep 92*/
		endp = pathbuf + buflen;
	}
	bcopy(add1, bp, len1);
	bp += len1;
	if (len2) {
		bcopy(add2, bp, len2);
		bp += len2;
	}
	*bp++ = ':';
	*bp = '\0';
}

char **
getdb()
{
	register char *p;
	int cnt, num;
	char **ar, *line;

	ar = NULL;
	num = 0;
	cnt = -1;

	openconfig();
	while ((line = fgetline(cfp, (size_t *) 0))) {
		p = strtok(line, " \t");
#define	WHATDB	"_whatdb"
		if (!p || *p == '#' || strcmp(p, WHATDB))
			continue;
		while (p = strtok((char *)NULL, " \t")) {
			if (cnt == num - 1 &&
			    !(ar = realloc(ar, (num += 30) * sizeof(char **))))
				enomem();
			if (!(ar[++cnt] = strdup(p)))
				enomem();
		}
	}
	if (ar) {
		if (cnt == num - 1 &&
		    !(ar = realloc(ar, ++num * sizeof(char **))))
			enomem();
		ar[++cnt] = NULL;
	}
	return(ar);
}

char **
getorder()
{
	register char *p;
	int cnt, num;
	char **ar, *line;

	ar = NULL;
	num = 0;
	cnt = -1;

	openconfig();
	while ((line = fgetline(cfp, (size_t *) 0))) {
		p = strtok(line, " \t");
#define	SUBDIR	"_subdir"
		if (!p || *p == '#' || strcmp(p, SUBDIR))
			continue;
		while (p = strtok((char *)NULL, " \t")) {
			if (cnt == num - 1 &&
			    !(ar = realloc(ar, (num += 30) * sizeof(char **))))
				enomem();
			if (!(ar[++cnt] = strdup(p)))
				enomem();
		}
	}
	if (ar) {
		if (cnt == num - 1 &&
		    !(ar = realloc(ar, ++num * sizeof(char **))))
			enomem();
		ar[++cnt] = NULL;
	}
	return(ar);
}

getsection(sect)
	char *sect;
{
	register char *p;
	char *line;

	openconfig();
	while ((line = fgetline(cfp, (size_t *) 0))) {
		p = strtok(line, " \t");
		if (!p || *p == '#')
			continue;
		if (!strcmp(p, sect))
			return(1);
	}
	return(0);
}

void
enomem()
{
	(void)fprintf(stderr, "%s: %s\n", progname, strerror(ENOMEM));
	exit(1);
}
