/* $NetBSD: makedefs.c,v 1.7 2001/03/25 20:44:04 jsm Exp $ */

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#ifndef lint
static const char rcsid[] =
    "$NetBSD: makedefs.c,v 1.7 2001/03/25 20:44:04 jsm Exp $";
#endif				/* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* construct definitions of object constants */
#define	LINSZ	1000
#define	STRSZ	40

int             fd;
char            string[STRSZ];

static void readline(void);
static char nextchar(void);
static int skipuntil(const char *);
static int getentry(void);
static void capitalize(char *);
static int letter(int);
static int digit(int);

int main(int, char **);

int
main(argc, argv)
	int             argc;
	char          **argv;
{
	int             i = 0;
	int             propct = 0;
	char           *sp;
	if (argc != 2) {
		(void) fprintf(stderr, "usage: makedefs file\n");
		exit(1);
	}
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror(argv[1]);
		exit(1);
	}
	skipuntil("objects[] = {");
	while (getentry()) {
		if (!*string) {
			i++;
			continue;
		}
		for (sp = string; *sp; sp++)
			if (*sp == ' ' || *sp == '\t' || *sp == '-')
				*sp = '_';
		if (!strncmp(string, "RIN_", 4)) {
			capitalize(string + 4);
			printf("#define	%s	u.uprops[%d].p_flgs\n",
			       string + 4, propct++);
		}
		for (sp = string; *sp; sp++)
			capitalize(sp);
		/* avoid trouble with stupid C preprocessors */
		if (!strncmp(string, "WORTHLESS_PIECE_OF_", 19))
			printf("/* #define %s	%d */\n", string, i);
		else
			printf("#define	%s	%d\n", string, i);
		i++;
	}
	printf("\n#define	CORPSE	DEAD_HUMAN\n");
	printf("#define	LAST_GEM	(JADE+1)\n");
	printf("#define	LAST_RING	%d\n", propct);
	printf("#define	NROFOBJECTS	%d\n", i - 1);
	fflush(stdout);
	if (ferror(stdout)) {
		perror("standard output");
		exit(1);
	}
	exit(0);
}

char            line[LINSZ], *lp = line, *lp0 = line, *lpe = line;
int             eof;

static void
readline()
{
	int             n = read(fd, lp0, (line + LINSZ) - lp0);
	if (n < 0) {
		printf("Input error.\n");
		exit(1);
	}
	if (n == 0)
		eof++;
	lpe = lp0 + n;
}

static char
nextchar()
{
	if (lp == lpe) {
		readline();
		lp = lp0;
	}
	return ((lp == lpe) ? 0 : *lp++);
}

static int
skipuntil(s)
	const char           *s;
{
	const char *sp0;
	char *sp1;
loop:
	while (*s != nextchar())
		if (eof) {
			printf("Cannot skipuntil %s\n", s);
			exit(1);
		}
	if (strlen(s) > lpe - lp + 1) {
		char           *lp1, *lp2;
		lp2 = lp;
		lp1 = lp = lp0;
		while (lp2 != lpe)
			*lp1++ = *lp2++;
		lp2 = lp0;	/* save value */
		lp0 = lp1;
		readline();
		lp0 = lp2;
		if (strlen(s) > lpe - lp + 1) {
			printf("error in skipuntil");
			exit(1);
		}
	}
	sp0 = s + 1;
	sp1 = lp;
	while (*sp0 && *sp0 == *sp1)
		sp0++, sp1++;
	if (!*sp0) {
		lp = sp1;
		return (1);
	}
	goto loop;
}

static int
getentry()
{
	int             inbraces = 0, inparens = 0, stringseen = 0, commaseen = 0;
	int             prefix = 0;
	char            ch;
#define	NSZ	10
	char            identif[NSZ], *ip;
	string[0] = string[4] = 0;
	/*
	 * read until {...} or XXX(...) followed by , skip comment and
	 * #define lines deliver 0 on failure
	 */
	while (1) {
		ch = nextchar();
swi:
		if (letter(ch)) {
			ip = identif;
			do {
				if (ip < identif + NSZ - 1)
					*ip++ = ch;
				ch = nextchar();
			} while (letter(ch) || digit(ch));
			*ip = 0;
			while (ch == ' ' || ch == '\t')
				ch = nextchar();
			if (ch == '(' && !inparens && !stringseen)
				if (!strcmp(identif, "WAND") ||
				    !strcmp(identif, "RING") ||
				    !strcmp(identif, "POTION") ||
				    !strcmp(identif, "SCROLL"))
					(void) strncpy(string, identif, 3),
						string[3] = '_',
						prefix = 4;
		}
		switch (ch) {
		case '/':
			/* watch for comment */
			if ((ch = nextchar()) == '*')
				skipuntil("*/");
			goto swi;
		case '{':
			inbraces++;
			continue;
		case '(':
			inparens++;
			continue;
		case '}':
			inbraces--;
			if (inbraces < 0)
				return (0);
			continue;
		case ')':
			inparens--;
			if (inparens < 0) {
				printf("too many ) ?");
				exit(1);
			}
			continue;
		case '\n':
			/* watch for #define at begin of line */
			if ((ch = nextchar()) == '#') {
				char            pch;
				/* skip until '\n' not preceded by '\\' */
				do {
					pch = ch;
					ch = nextchar();
				} while (ch != '\n' || pch == '\\');
				continue;
			}
			goto swi;
		case ',':
			if (!inparens && !inbraces) {
				if (prefix && !string[prefix])
					string[0] = 0;
				if (stringseen)
					return (1);
				printf("unexpected ,\n");
				exit(1);
			}
			commaseen++;
			continue;
		case '\'':
			if ((ch = nextchar()) == '\\')
				ch = nextchar();
			if (nextchar() != '\'') {
				printf("strange character denotation?\n");
				exit(1);
			}
			continue;
		case '"':
			{
				char           *sp = string + prefix;
				char            pch;
				int             store = (inbraces || inparens)
				&& !stringseen++ && !commaseen;
				do {
					pch = ch;
					ch = nextchar();
					if (store && sp < string + STRSZ)
						*sp++ = ch;
				} while (ch != '"' || pch == '\\');
				if (store)
					*--sp = 0;
				continue;
			}
		}
	}
}

static void
capitalize(sp)
	char           *sp;
{
	if ('a' <= *sp && *sp <= 'z')
		*sp += 'A' - 'a';
}

static int
letter(ch)
	char            ch;
{
	return (('a' <= ch && ch <= 'z') ||
		('A' <= ch && ch <= 'Z'));
}

static int
digit(ch)
	char            ch;
{
	return ('0' <= ch && ch <= '9');
}
