/*	$NetBSD: xstr.c,v 1.9 1998/12/20 19:13:19 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)xstr.c	8.1 (Berkeley) 6/9/93";
#else
__RCSID("$NetBSD: xstr.c,v 1.9 1998/12/20 19:13:19 christos Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "pathnames.h"

/*
 * xstr - extract and hash strings in a C program
 *
 * Bill Joy UCB
 * November, 1978
 */

static off_t	hashit __P((char *, int));
static void	onintr __P((int));
static off_t	yankstr __P((char **));
static int	octdigit __P((char));
static void	inithash __P((void));
static int	fgetNUL __P((char *, int, FILE *));
static int	xgetc __P((FILE *));
static void	flushsh __P((void));
static void	found __P((int, off_t, char *));
static void	prstr __P((char *));
static void	xsdotc __P((void));
static char	lastchr __P((char *));
static int	istail __P((char *, char *));
static void	process __P((char *));
static void	usage __P((void));

static off_t	tellpt;
static off_t	mesgpt;
static char	*strings =	"strings";
static char	*array =	0;
static int	cflg;
static int	vflg;
static int	readstd;
static char	linebuf[BUFSIZ];

#define	BUCKETS	128

static struct	hash {
	off_t	hpt;
	char	*hstr;
	struct	hash *hnext;
	short	hnew;
} bucket[BUCKETS];

int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c;

	while ((c = getopt(argc, argv, "-cvl:")) != -1)
		switch (c) {
		case '-':
			readstd++;
			break;
		case 'c':
			cflg++;
			break;
		case 'v':
			vflg++;
			break;
		case 'l':
			array = optarg;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (array == 0)
		array = "xstr";

	if (signal(SIGINT, SIG_IGN) == SIG_DFL)
		(void)signal(SIGINT, onintr);
	if (cflg || (argc == 0 && !readstd))
		inithash();
	else
		strings = mktemp(strdup(_PATH_TMP));
	while (readstd || argc > 0) {
		if (freopen("x.c", "w", stdout) == NULL)
			err(1, "Cannot open `%s'", "x.c");
		if (!readstd && freopen(argv[0], "r", stdin) == NULL)
			err(1, "Cannot open `%s'", argv[0]);
		process("x.c");
		if (readstd == 0)
			argc--, argv++;
		else
			readstd = 0;
	};
	flushsh();
	if (cflg == 0)
		xsdotc();
	if (strings[0] == '/')
		(void)unlink(strings);
	exit(0);
}

static void
process(name)
	char *name;
{
	char *cp;
	int c;
	int incomm = 0;
	int ret;

	printf("extern char\t%s[];\n", array);
	for (;;) {
		if (fgets(linebuf, sizeof linebuf, stdin) == NULL) {
			if (ferror(stdin))
				err(1, "Error reading `%s'", name);
			break;
		}
		if (linebuf[0] == '#') {
			if (linebuf[1] == ' ' &&
			    isdigit((unsigned char)linebuf[2]))
				printf("#line%s", &linebuf[1]);
			else
				printf("%s", linebuf);
			continue;
		}
		for (cp = linebuf; (c = *cp++);) switch (c) {

		case '"':
			if (incomm)
				goto def;
			if ((ret = (int) yankstr(&cp)) == -1)
				goto out;
			printf("(&%s[%d])", array, ret);
			break;

		case '\'':
			if (incomm)
				goto def;
			putchar(c);
			if (*cp)
				putchar(*cp++);
			break;

		case '/':
			if (incomm || *cp != '*')
				goto def;
			incomm = 1;
			cp++;
			printf("/*");
			continue;

		case '*':
			if (incomm && *cp == '/') {
				incomm = 0;
				cp++;
				printf("*/");
				continue;
			}
			goto def;

def:
		default:
			putchar(c);
			break;
		}
	}
out:
	if (ferror(stdout)) {
		warn("Error reading `%s'", "x.c");
		onintr(1);
	}
}

static off_t
yankstr(cpp)
	char **cpp;
{
	char *cp = *cpp;
	int c, ch;
	char dbuf[BUFSIZ];
	char *dp = dbuf;
	char *tp;

	while ((c = *cp++)) {
		switch (c) {

		case '"':
			cp++;
			goto out;

		case '\\':
			c = *cp++;
			if (c == 0)
				break;
			if (c == '\n') {
				if (fgets(linebuf, sizeof linebuf, stdin)
				    == NULL) {
					if (ferror(stdin))
						err(1, "Error reading `x.c'");
					return(-1);
				}
				cp = linebuf;
				continue;
			}
			for (tp = "b\bt\tr\rn\nf\f\\\\\"\""; (ch = *tp++); tp++)
				if (c == ch) {
					c = *tp;
					goto gotc;
				}
			if (!octdigit(c)) {
				*dp++ = '\\';
				break;
			}
			c -= '0';
			if (!octdigit(*cp))
				break;
			c <<= 3, c += *cp++ - '0';
			if (!octdigit(*cp))
				break;
			c <<= 3, c += *cp++ - '0';
			break;
		}
gotc:
		*dp++ = c;
	}
out:
	*cpp = --cp;
	*dp = 0;
	return (hashit(dbuf, 1));
}

static int
octdigit(c)
	char c;
{

	return (isdigit((unsigned char)c) && c != '8' && c != '9');
}

static void
inithash()
{
	char buf[BUFSIZ];
	FILE *mesgread = fopen(strings, "r");

	if (mesgread == NULL)
		return;
	for (;;) {
		mesgpt = tellpt;
		if (fgetNUL(buf, sizeof buf, mesgread) == 0)
			break;
		(void)hashit(buf, 0);
	}
	(void)fclose(mesgread);
}

static int
fgetNUL(obuf, rmdr, file)
	char *obuf;
	int rmdr;
	FILE *file;
{
	int c;
	char *buf = obuf;

	while (--rmdr > 0 && (c = xgetc(file)) != 0 && c != EOF)
		*buf++ = c;
	*buf++ = 0;
	return ((feof(file) || ferror(file)) ? 0 : 1);
}

static int
xgetc(file)
	FILE *file;
{

	tellpt++;
	return (getc(file));
}


static off_t
hashit(str, new)
	char *str;
	int new;
{
	int i;
	struct hash *hp, *hp0;

	hp = hp0 = &bucket[lastchr(str) & 0177];
	while (hp->hnext) {
		hp = hp->hnext;
		i = istail(str, hp->hstr);
		if (i >= 0)
			return (hp->hpt + i);
	}
	if ((hp = (struct hash *) calloc(1, sizeof (*hp))) == NULL)
		err(1, "%s", "");
	hp->hpt = mesgpt;
	if ((hp->hstr = strdup(str)) == NULL)
		err(1, "%s", "");
	mesgpt += strlen(hp->hstr) + 1;
	hp->hnext = hp0->hnext;
	hp->hnew = new;
	hp0->hnext = hp;
	return (hp->hpt);
}

static void
flushsh()
{
	int i;
	struct hash *hp;
	FILE *mesgwrit;
	int old = 0, new = 0;

	for (i = 0; i < BUCKETS; i++)
		for (hp = bucket[i].hnext; hp != NULL; hp = hp->hnext)
			if (hp->hnew)
				new++;
			else
				old++;
	if (new == 0 && old != 0)
		return;
	mesgwrit = fopen(strings, old ? "r+" : "w");
	if (mesgwrit == NULL)
		err(1, "Cannot open `%s'", strings);
	for (i = 0; i < BUCKETS; i++)
		for (hp = bucket[i].hnext; hp != NULL; hp = hp->hnext) {
			found(hp->hnew, hp->hpt, hp->hstr);
			if (hp->hnew) {
				fseek(mesgwrit, hp->hpt, 0);
				(void)fwrite(hp->hstr, strlen(hp->hstr) + 1, 1,
				    mesgwrit);
				if (ferror(mesgwrit))
					err(1, "Error writing `%s'", strings);
			}
		}
	if (fclose(mesgwrit) == EOF)
		err(1, "Error closing `%s'", strings);
}

static void
found(new, off, str)
	int new;
	off_t off;
	char *str;
{
	if (vflg == 0)
		return;
	if (!new)
		(void)fprintf(stderr, "found at %d:", (int) off);
	else
		(void)fprintf(stderr, "new at %d:", (int) off);
	prstr(str);
	(void)fprintf(stderr, "\n");
}

static void
prstr(cp)
	char *cp;
{
	int c;

	while ((c = (*cp++ & 0377)))
		if (c < ' ')
			fprintf(stderr, "^%c", c + '`');
		else if (c == 0177)
			fprintf(stderr, "^?");
		else if (c > 0200)
			fprintf(stderr, "\\%03o", c);
		else
			fprintf(stderr, "%c", c);
}

static void
xsdotc()
{
	FILE *strf = fopen(strings, "r");
	FILE *xdotcf;

	if (strf == NULL)
		err(1, "Cannot open `%s'", strings);
	xdotcf = fopen("xs.c", "w");
	if (xdotcf == NULL)
		err(1, "Cannot open `%s'", "xs.c");
	fprintf(xdotcf, "char\t%s[] = {\n", array);
	for (;;) {
		int i, c;

		for (i = 0; i < 8; i++) {
			c = getc(strf);
			if (ferror(strf)) {
				warn("Error reading `%s'", strings);
				onintr(1);
			}
			if (feof(strf)) {
				fprintf(xdotcf, "\n");
				goto out;
			}
			fprintf(xdotcf, "0x%02x,", c);
		}
		fprintf(xdotcf, "\n");
	}
out:
	fprintf(xdotcf, "};\n");
	(void)fclose(xdotcf);
	(void)fclose(strf);
}

static char
lastchr(cp)
	char *cp;
{

	while (cp[0] && cp[1])
		cp++;
	return (*cp);
}

static int
istail(str, of)
	char *str, *of;
{
	int d = strlen(of) - strlen(str);

	if (d < 0 || strcmp(&of[d], str) != 0)
		return (-1);
	return (d);
}

static void
onintr(dummy)
	int dummy;
{

	(void)signal(SIGINT, SIG_IGN);
	if (strings[0] == '/')
		(void)unlink(strings);
	(void)unlink("x.c");
	(void)unlink("xs.c");
	exit(dummy);
}

static void
usage()
{
	extern char *__progname;
	(void)fprintf(stderr, "Usage: %s [-vc] [-l array] [-] [<name> ...]\n",
	    __progname);
	exit(1);
}
