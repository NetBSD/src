/*	$NetBSD: morse.c,v 1.23 2024/10/12 19:26:24 rillig Exp $	*/

/*
 * Copyright (c) 1988, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
/*	@(#)morse.c	8.1 (Berkeley) 5/31/93	*/
__RCSID("$NetBSD: morse.c,v 1.23 2024/10/12 19:26:24 rillig Exp $");

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char digit[][6] = {
	"-----",
	".----",
	"..---",
	"...--",
	"....-",
	".....",
	"-....",
	"--...",
	"---..",
	"----.",
};

static const char alph[][5] = {
	".-",
	"-...",
	"-.-.",
	"-..",
	".",
	"..-.",
	"--.",
	"....",
	"..",
	".---",
	"-.-",
	".-..",
	"--",
	"-.",
	"---",
	".--.",
	"--.-",
	".-.",
	"...",
	"-",
	"..-",
	"...-",
	".--",
	"-..-",
	"-.--",
	"--..",
};

static const struct {
	char c;
	const char morse[7];
} other[] = {
	{ '.', ".-.-.-" },
	{ ',', "--..--" },
	{ ':', "---..." },
	{ '?', "..--.." },
	{ '\'', ".----." },
	{ '-', "-....-" },
	{ '/', "-..-." },
	{ '(', "-.--." },
	{ ')', "-.--.-" },
	{ '"', ".-..-." },
	{ '=', "-...-" },
	{ '+', ".-.-." },
	{ '_', "..--.-" },
	{ '@', ".--.-." },
	{ '\0', "" }
};

static void morse(int);
static void decode(const char *);
static void show(const char *);

static int sflag;
static int dflag;

int
main(int argc, char **argv)
{
	int ch;
	char *p;

	/* Revoke setgid privileges */
	setgid(getgid());

	while ((ch = getopt(argc, argv, "ds")) != -1)
		switch ((char)ch) {
		case 'd':
			dflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			fprintf(stderr, "usage: morse [-ds] [string ...]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (dflag) {
		if (*argv) {
			do {
				decode(*argv);
			} while (*++argv);
		} else {
			char foo[10];	/* All morse chars shorter than this */
			int is_blank, i;

			i = 0;
			is_blank = 0;
			while ((ch = getchar()) != EOF) {
				if (ch == '-' || ch == '.') {
					foo[i++] = ch;
					if (i == 10) {
						/* overrun means gibberish--print 'x' and
						 * advance */
						i = 0;
						putchar('x');
						while ((ch = getchar()) != EOF &&
						    (ch == '.' || ch == '-'))
							;
						is_blank = 1;
					}
				} else if (i) {
					foo[i] = '\0';
					decode(foo);
					i = 0;
					is_blank = 0;
				} else if (isspace(ch)) {
					if (is_blank) {
						/* print whitespace for each double blank */
						putchar(' ');
						is_blank = 0;
					} else
						is_blank = 1;
				}
			}
		}
		putchar('\n');
	} else {
		if (*argv)
			do {
				for (p = *argv; *p; ++p)
					morse((unsigned char)*p);
				show("");
			} while (*++argv);
		else while ((ch = getchar()) != EOF)
			morse(ch);
		show("...-.-");	/* SK */
	}

	return 0;
}

void
decode(const char *s)
{
	for (size_t i = 0; i < __arraycount(digit); i++)
		if (strcmp(digit[i], s) == 0) {
			putchar('0' + i);
			return;
		}

	for (size_t i = 0; i < __arraycount(alph); i++)
		if (strcmp(alph[i], s) == 0) {
			putchar('A' + i);
			return;
		}

	for (size_t i = 0; other[i].c; i++)
		if (strcmp(other[i].morse, s) == 0) {
			putchar(other[i].c);
			return;
		}

	if (strcmp("...-.-", s) == 0)	/* SK */
		return;

	putchar('x');	/* line noise */
}

void
morse(int c)
{
	if (isalpha(c))
		show(alph[c - (isupper(c) ? 'A' : 'a')]);
	else if (isdigit(c))
		show(digit[c - '0']);
	else if (isspace(c))
		show("");  /* could show BT for a pause */
	else
		for (int i = 0; other[i].c; i++)
			if (other[i].c == c) {
				show(other[i].morse);
				break;
			}
}

void
show(const char *s)
{
	if (sflag)
		printf(" %s", s);
	else for (; *s; ++s)
		printf(" %s", *s == '.' ? "dit" : "daw");
	printf("\n");
}
