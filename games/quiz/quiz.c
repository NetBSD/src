/*	$NetBSD: quiz.c,v 1.26.6.1 2014/05/22 11:36:23 yamt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jim R. Oldroyd at The Instruction Set and Keith Gabryelski at
 * Commodore Business Machines.
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
__COPYRIGHT("@(#) Copyright (c) 1991, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quiz.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: quiz.c,v 1.26.6.1 2014/05/22 11:36:23 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <time.h>
#include <unistd.h>
#include "quiz.h"
#include "pathnames.h"

static QE qlist;
static int catone, cattwo, tflag;
static unsigned qsize;

static char *appdstr(char *, const char *, size_t);
static void downcase(char *);
static void get_cats(char *, char *);
static void get_file(const char *);
static const char *next_cat(const char *);
static void quiz(void);
static void score(unsigned, unsigned, unsigned);
static void show_index(void);
static void usage(void) __dead;

int
main(int argc, char *argv[])
{
	int ch;
	const char *indexfile;

	/* Revoke setgid privileges */
	setgid(getgid());

	indexfile = _PATH_QUIZIDX;
	while ((ch = getopt(argc, argv, "i:t")) != -1)
		switch(ch) {
		case 'i':
			indexfile = optarg;
			break;
		case 't':
			tflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch(argc) {
	case 0:
		get_file(indexfile);
		show_index();
		break;
	case 2:
		get_file(indexfile);
		get_cats(argv[0], argv[1]);
		quiz();
		break;
	default:
		usage();
	}
	exit(0);
}

static void
get_file(const char *file)
{
	FILE *fp;
	QE *qp;
	size_t len;
	char *lp;

	if ((fp = fopen(file, "r")) == NULL)
		err(1, "%s", file);

	/*
	 * XXX
	 * Should really free up space from any earlier read list
	 * but there are no reverse pointers to do so with.
	 */
	qp = &qlist;
	qsize = 0;
	while ((lp = fgetln(fp, &len)) != NULL) {
		if (lp[len - 1] == '\n')
			lp[--len] = '\0';
		if (qp->q_text && qp->q_text[strlen(qp->q_text) - 1] == '\\')
			qp->q_text = appdstr(qp->q_text, lp, len);
		else {
			if ((qp->q_next = malloc(sizeof(QE))) == NULL)
				errx(1, "malloc");
			qp = qp->q_next;
			if ((qp->q_text = malloc(len + 1)) == NULL)
				errx(1, "malloc");
			strncpy(qp->q_text, lp, len);
			qp->q_text[len] = '\0';
			qp->q_asked = qp->q_answered = FALSE;
			qp->q_next = NULL;
			++qsize;
		}
	}
	(void)fclose(fp);
}

static void
show_index(void)
{
	QE *qp;
	const char *p, *s;
	FILE *pf;
	const char *pager;

	if (!isatty(1))
		pager = "cat";
	else {
		if (!(pager = getenv("PAGER")) || (*pager == 0))
			pager = _PATH_PAGER;
	}
	if ((pf = popen(pager, "w")) == NULL)
		err(1, "%s", pager);
	(void)fprintf(pf, "Subjects:\n\n");
	for (qp = qlist.q_next; qp; qp = qp->q_next) {
		for (s = next_cat(qp->q_text); s; s = next_cat(s)) {
			if (!rxp_compile(s))
				errx(1, "%s", rxperr);
			if ((p = rxp_expand()) != NULL)
				(void)fprintf(pf, "%s ", p);
		}
		(void)fprintf(pf, "\n");
	}
	(void)fprintf(pf, "\n%s\n%s\n%s\n",
"For example, \"quiz victim killer\" prints a victim's name and you reply",
"with the killer, and \"quiz killer victim\" works the other way around.",
"Type an empty line to get the correct answer.");
	(void)pclose(pf);
}

static void
get_cats(char *cat1, char *cat2)
{
	QE *qp;
	int i;
	const char *s;

	downcase(cat1);
	downcase(cat2);
	for (qp = qlist.q_next; qp; qp = qp->q_next) {
		s = next_cat(qp->q_text);
		catone = cattwo = i = 0;
		while (s) {
			if (!rxp_compile(s))
				errx(1, "%s", rxperr);
			i++;
			if (rxp_match(cat1))
				catone = i;
			if (rxp_match(cat2))
				cattwo = i;
			s = next_cat(s);
		}
		if (catone && cattwo && catone != cattwo) {
			if (!rxp_compile(qp->q_text))
				errx(1, "%s", rxperr);
			get_file(rxp_expand());
			return;
		}
	}
	errx(1, "invalid categories");
}

static void
quiz(void)
{
	QE *qp;
	int i;
	size_t len;
	unsigned guesses, rights, wrongs;
	unsigned next, j;
	char *answer, *t, question[LINE_SZ];
	const char *s;

	srandom(time(NULL));
	guesses = rights = wrongs = 0;
	for (;;) {
		if (qsize == 0)
			break;
		next = random() % qsize;
		qp = qlist.q_next;
		for (j = 0; j < next; j++)
			qp = qp->q_next;
		while (qp && qp->q_answered)
			qp = qp->q_next;
		if (!qp) {
			qsize = next;
			continue;
		}
		if (tflag && random() % 100 > 20) {
			/* repeat questions in tutorial mode */
			while (qp && (!qp->q_asked || qp->q_answered))
				qp = qp->q_next;
			if (!qp)
				continue;
		}
		s = qp->q_text;
		for (i = 0; i < catone - 1; i++)
			s = next_cat(s);
		if (!rxp_compile(s))
			errx(1, "%s", rxperr);
		t = rxp_expand();
		if (!t || *t == '\0') {
			qp->q_answered = TRUE;
			continue;
		}
		(void)strcpy(question, t);
		s = qp->q_text;
		for (i = 0; i < cattwo - 1; i++)
			s = next_cat(s);
		if (!rxp_compile(s))
			errx(1, "%s", rxperr);
		t = rxp_expand();
		if (!t || *t == '\0') {
			qp->q_answered = TRUE;
			continue;
		}
		qp->q_asked = TRUE;
		(void)printf("%s?\n", question);
		for (;; ++guesses) {
			if ((answer = fgetln(stdin, &len)) == NULL ||
			    answer[len - 1] != '\n') {
				score(rights, wrongs, guesses);
				exit(0);
			}
			answer[len - 1] = '\0';
			downcase(answer);
			if (rxp_match(answer)) {
				(void)printf("Right!\n");
				++rights;
				qp->q_answered = TRUE;
				break;
			}
			if (*answer == '\0') {
				(void)printf("%s\n", t);
				++wrongs;
				if (!tflag)
					qp->q_answered = TRUE;
				break;
			}
			(void)printf("What?\n");
		}
	}
	score(rights, wrongs, guesses);
}

static const char *
next_cat(const char *s)
{
	int esc;

	esc = 0;
	for (;;)
		switch (*s++) {
		case '\0':
			return (NULL);
		case '\\':
			esc = 1;
			break;
		case ':':
			if (!esc)
				return (s);
		default:
			esc = 0;
			break;
		}
	/* NOTREACHED */
}

static char *
appdstr(char *s, const char *tp, size_t len)
{
	char *mp;
	const char *sp;
	int ch;
	char *m;

	if ((m = malloc(strlen(s) + len + 1)) == NULL)
		errx(1, "malloc");
	for (mp = m, sp = s; (*mp++ = *sp++) != '\0'; )
		;
	--mp;
	if (*(mp - 1) == '\\')
		--mp;

	while ((ch = *mp++ = *tp++) && ch != '\n')
		;
	*mp = '\0';

	free(s);
	return (m);
}

static void
score(unsigned r, unsigned w, unsigned g)
{
	(void)printf("Rights %d, wrongs %d,", r, w);
	if (g)
		(void)printf(" extra guesses %d,", g);
	(void)printf(" score %d%%\n", (r + w + g) ? r * 100 / (r + w + g) : 0);
}

static void
downcase(char *p)
{
	int ch;

	for (; (ch = *p) != '\0'; ++p)
		if (isascii(ch) && isupper(ch))
			*p = tolower(ch);
}

static void
usage(void)
{
	(void)fprintf(stderr, "quiz [-t] [-i file] category1 category2\n");
	exit(1);
}
