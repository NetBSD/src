/*	$NetBSD: morse.c,v 1.10 2000/07/03 03:57:42 matt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)morse.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: morse.c,v 1.10 2000/07/03 03:57:42 matt Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MORSE_COLON	"--..--"
#define MORSE_PERIOD	".-.-.-"


static const char
	*const digit[] = {
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
},
	*const alph[] = {
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

int	main __P((int, char *[]));
void	morse __P((int));
void	decode __P((const char *));
void	show __P((const char *));

static int sflag;
static int dflag;

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	char *s, *p;

	/* Revoke setgid privileges */
	setgid(getgid());

	while ((ch = getopt(argc, argv, "ds")) != -1)
		switch((char)ch) {
		case 'd':
			dflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			fprintf(stderr, "usage: morse [-ds] [string ...]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (dflag) {
		if (*argv) {
			do {
				s=strchr(*argv, ',');
				
				if (s)
					*s='\0';
				
				decode(*argv);
			} while (*++argv);
		}else{
			char buf[1024];

			while (fgets(buf, 1024, stdin)) {
				s=buf;

				while (*s && isspace(*s))
					s++;

				if (*s) {
					p=strtok(s, " \n\t");
					
					while (p) {
						s=strchr(p, ',');

						if (s)
							*s='\0';
						
						decode(p);
						p=strtok(NULL, " \n\t");
					}
				}
			}
		}
		putchar('\n');
	}else{
		if (*argv)
			do {
				for (p = *argv; *p; ++p)
					morse((int)*p);
			} while (*++argv);
		else while ((ch = getchar()) != EOF)
			morse(ch);
	}
	
	return 0;
}

void
decode(s)
	const char *s;
{
	if (strcmp(s, MORSE_COLON) == 0){
		putchar(',');
	} else if (strcmp(s, MORSE_PERIOD) == 0){
		putchar('.');
	} else {
		int found;
		const char *const *a;
		int size;
		int i;

		found=0;
		a=digit;
		size=sizeof(digit)/sizeof(digit[0]);
		for (i=0; i<size; i++) {
			if (strcmp(a[i], s) == 0) {
				found = 1;
				break;
			}
		}

		if (found) {
			putchar('0'+i);
			return;
		}

		found=0;
		a=alph;
		size=sizeof(alph)/sizeof(alph[0]);
		for (i=0; i<size; i++) {
			if (strcmp(a[i], s) == 0) {
				found = 1;
				break;
			}
		}

		if (found)
			putchar('a'+i);
		else
			putchar(' ');
	}
}

void
morse(c)
	int c;
{
	if (isalpha(c))
		show(alph[c - (isupper(c) ? 'A' : 'a')]);
	else if (isdigit(c))
		show(digit[c - '0']);
	else if (c == ',')
		show(MORSE_COLON);
	else if (c == '.')
		show(MORSE_PERIOD);
	else if (isspace(c))
		show(" ...\n");
}

void
show(s)
	const char *s;
{
	if (sflag)
		printf(" %s", s);
	else for (; *s; ++s)
		printf(" %s", *s == '.' ? "dit" : "daw");
	printf(",\n");
}
