/*	$NetBSD: bog.c,v 1.29 2014/03/22 23:39:04 dholland Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Barry Brachman.
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
__COPYRIGHT("@(#) Copyright (c) 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)bog.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: bog.c,v 1.29 2014/03/22 23:39:04 dholland Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/tty.h>

#include "bog.h"
#include "extern.h"

static char *batchword(FILE *);
static void playgame(void);
static int validword(const char *);
static void checkdict(void);
static void newgame(const char *);
static int compar(const void *, const void *);
static void clearwordpath(int full);
static void usage(void) __dead;

struct dictindex dictindex[26];

/*
 * Cube position numbering:
 *
 *	0 1 2 3
 *	4 5 6 7
 *	8 9 A B
 *	C D E F
 */
static int adjacency[16][16] = {
/*        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
	{ 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },		/* 0 */
	{ 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },		/* 1 */
	{ 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },		/* 2 */
	{ 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },		/* 3 */
	{ 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },		/* 4 */
	{ 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 },		/* 5 */
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0 },		/* 6 */
	{ 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0 },		/* 7 */
	{ 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0 },		/* 8 */
	{ 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0 },		/* 9 */
	{ 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1 },		/* A */
	{ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1 },		/* B */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 },		/* C */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0 },		/* D */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1 },		/* E */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0 }		/* F */
};

static int letter_map[26][16];

char board[17];
int wordpath[MAXWORDLEN + 1];
int wordlen;		/* Length of last word returned by nextword() */
int usedbits;

const char *pword[MAXPWORDS];
static char pwords[MAXPSPACE], *pwordsp;
int npwords;

const char *mword[MAXMWORDS];
static char mwords[MAXMSPACE], *mwordsp;
int nmwords;

int ngames = 0;
int tnmwords = 0, tnpwords = 0;

#include <setjmp.h>
jmp_buf env;

time_t start_t;
int debug;
int tlimit;

static FILE *dictfp;
static int batch;
static int minlength;
static int reuse;

int
main(int argc, char *argv[])
{
	long seed;
	int ch, done, i, selfuse, sflag;
	char *bspec, *p;

	/* revoke setgid privileges */
	setgid(getgid());

	seed = 0;
	batch = debug = reuse = selfuse = sflag = 0;
	bspec = NULL;
	minlength = 3;
	tlimit = 180;		/* 3 minutes is standard */

	while ((ch = getopt(argc, argv, "bds:t:w:")) != -1)
		switch(ch) {
		case 'b':
			batch = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 's':
			sflag = 1;
			seed = atol(optarg);
			break;
		case 't':
			if ((tlimit = atoi(optarg)) < 1)
				errx(1, "bad time limit");
			break;
		case 'w':
			if ((minlength = atoi(optarg)) < 3)
				errx(1, "min word length must be > 2");
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* process final arguments */
	if (argc > 0) {
		if (strcmp(argv[0], "+") == 0)
			reuse = 1;
		else if (strcmp(argv[0], "++") == 0)
			selfuse = 1;
	}

	if (reuse || selfuse) {
		argc -= 1;
		argv += 1;
	}

	if (argc > 0) {
		if (islower((unsigned char)argv[0][0])) {
			if (strlen(argv[0]) != 16) {
				usage();
			} else {
				/* This board is assumed to be valid... */
				bspec = argv[0];
			}
		} else {
		  	usage();
		}
	}

	if (batch && bspec == NULL)
		errx(1, "must give both -b and a board setup");

	if (selfuse)
		for (i = 0; i < 16; i++)
			adjacency[i][i] = 1;

	if (batch) {
		newgame(bspec);
		while ((p = batchword(stdin)) != NULL)
			(void) printf("%s\n", p);
		exit (0);
	}
	setup(sflag, seed);
	prompt("Loading the dictionary...");
	if ((dictfp = opendict(DICT)) == NULL) {
		warn("%s", DICT);
		cleanup();
		exit(1);
	}
#ifdef LOADDICT
	if (loaddict(dictfp) < 0) {
		warnx("can't load %s", DICT);
		cleanup();
		exit(1);
	}
	(void)fclose(dictfp);
	dictfp = NULL;
#endif
	if (loadindex(DICTINDEX) < 0) {
		warnx("can't load %s", DICTINDEX);
		cleanup();
		exit(1);
	}

	prompt("Type <space> to begin...");
	while (inputch() != ' ');

	for (done = 0; !done;) {
		newgame(bspec);
		bspec = NULL;	/* reset for subsequent games */
		playgame();
#ifdef NEW_STYLE
		prompt("Type <q>uit, <esc> locate, any other key to continue...");
#else
		prompt("Type <space> to continue, any cap to quit...");
#endif
		delay(50);	/* wait for user to quit typing */
		flushin(stdin);
		for (;;) {
			ch = inputch();
#ifdef NEW_STYLE
			if (ch == '\e')
				findword();
			else if (ch == CTRL('l') || ch == CTRL('r'))
				redraw();
			else {
				if (ch == 'q') {
					done = 1;
					break;
				} else {
					break;
				}
			}
#else
			if (ch == '\e')
				findword();
			else if (ch == CTRL('l') || ch == CTRL('r'))
				redraw();
			else {
				if (isupper(ch)) {
					done = 1;
					break;
				}
				if (ch == ' ')
					break;
			}
#endif
		}
	}
	cleanup();
	exit (0);
}

/*
 * Read a line from the given stream and check if it is legal
 * Return a pointer to a legal word or a null pointer when EOF is reached
 */
static char *
batchword(FILE *fp)
{
	char *w;

	clearwordpath(1);
	while ((w = nextword(fp)) != NULL) {
		if (wordlen < minlength)
			continue;
		clearwordpath(0);
		usedbits = 0;
		if (checkword(w, -1, wordpath) != -1)
			return (w);
	}
	return (NULL);
}

/*
 * Play a single game
 * Reset the word lists from last game
 * Keep track of the running stats
 */
static void
playgame(void)
{
	int i;
	time_t t;
	char buf[MAXWORDLEN + 1];

	ngames++;
	npwords = 0;
	pwordsp = pwords;
	nmwords = 0;
	mwordsp = mwords;

	time(&start_t);

	clearwordpath(1);
	showboard(board);
	startwords();
	if (setjmp(env)) {
		badword();
		goto timesup;
	}

	while (1) {
		if (get_line(buf) == NULL) {
			if (feof(stdin))
				clearerr(stdin);
			break;
		}
		time(&t);
		if (t - start_t >= tlimit) {
			badword();
			break;
		}
		if (buf[0] == '\0') {
			int remaining;

			remaining = tlimit - (int) (t - start_t);
			(void)snprintf(buf, sizeof(buf),
			    "%d:%02d", remaining / 60, remaining % 60);
			showstr(buf, 1);
			continue;
		}
		if (strlen(buf) < (size_t)minlength) {
			badword();
			continue;
		}

		clearwordpath(0);
		usedbits = 0;

		if (checkword(buf, -1, wordpath) < 0)
			badword();
		else {
			if (debug) {
				(void) printf("[");
				for (i = 0; wordpath[i] != -1; i++)
					(void) printf(" %d", wordpath[i]);
				(void) printf(" ]\n");
			}
			for (i = 0; i < npwords; i++) {
				if (strcmp(pword[i], buf) == 0)
					break;
			}
			if (i != npwords) {	/* already used the word */
				badword();
				showword(i);
			}
			else if (!validword(buf))
				badword();
			else {
				size_t len;

				len = strlen(buf) + 1;
				if (npwords == MAXPWORDS - 1 ||
				    pwordsp + len >= &pwords[MAXPSPACE]) {
					warnx("Too many words!");
					cleanup();
					exit(1);
				}
				pword[npwords++] = pwordsp;
				(void) strcpy(pwordsp, buf);
				pwordsp += len;
				addword(buf);
			}
		}
	}

timesup: ;

	/*
	 * Sort the player's words and terminate the list with a null
	 * entry to help out checkdict()
	 */
	qsort(pword, npwords, sizeof(pword[0]), compar);
	pword[npwords] = NULL;

	/*
	 * These words don't need to be sorted since the dictionary is sorted
	 */
	checkdict();

	tnmwords += nmwords;
	tnpwords += npwords;

	results();
}

/*
 * Check if the given word is present on the board, with the constraint
 * that the first letter of the word is adjacent to square 'prev'
 * Keep track of the current path of squares for the word
 * A 'q' must be followed by a 'u'
 * Words must end with a null
 * Return 1 on success, -1 on failure
 */
int
checkword(const char *word, int prev, int *path)
{
	const char *p;
	char *q;
	int i, *lm;

	if (debug) {
		(void) printf("checkword(%s, %d, [", word, prev);
			for (i = 0; wordpath[i] != -1; i++)
				(void) printf(" %d", wordpath[i]);
			(void) printf(" ]\n");
	}

	if (*word == '\0')
		return (1);

	lm = letter_map[*word - 'a'];

	if (prev == -1) {
		char subword[MAXWORDLEN + 1];

		/*
		 * Check for letters not appearing in the cube to eliminate some
		 * recursive calls
		 * Fold 'qu' into 'q'
		 */
		p = word;
		q = subword;
		while (*p != '\0') {
			if (*letter_map[*p - 'a'] == -1)
				return (-1);
			*q++ = *p;
			if (*p++ == 'q') {
				if (*p++ != 'u')
					return (-1);
			}
		}
		*q = '\0';
		while (*lm != -1) {
			*path = *lm;
			usedbits |= (1 << *lm);
			if (checkword(subword + 1, *lm, path + 1) > 0)
				return (1);
			usedbits &= ~(1 << *lm);
			lm++;
		}
		return (-1);
	}

	/*
	 * A cube is only adjacent to itself in the adjacency matrix if selfuse
	 * was set, so a cube can't be used twice in succession if only the
	 * reuse flag is set
	 */
	for (i = 0; lm[i] != -1; i++) {
		if (adjacency[prev][lm[i]]) {
			int used;

			used = 1 << lm[i];
			/*
			 * If necessary, check if the square has already
			 * been used.
			 */
			if (!reuse && (usedbits & used))
					continue;
			*path = lm[i];
			usedbits |= used;
			if (checkword(word + 1, lm[i], path + 1) > 0)
				return (1);
			usedbits &= ~used;
		}
	}
	*path = -1;		/* in case of a backtrack */
	return (-1);
}

/*
 * A word is invalid if it is not in the dictionary
 * At this point it is already known that the word can be formed from
 * the current board
 */
static int
validword(const char *word)
{
	int j;
	const char *q, *w;

	j = word[0] - 'a';
	if (dictseek(dictfp, dictindex[j].start, SEEK_SET) < 0) {
		(void) fprintf(stderr, "Seek error\n");
		cleanup();
		exit(1);
	}

	while ((w = nextword(dictfp)) != NULL) {
		int ch;

		if (*w != word[0])	/* end of words starting with word[0] */
			break;
		q = word;
		while ((ch = *w++) == *q++ && ch != '\0')
			;
		if (*(w - 1) == '\0' && *(q - 1) == '\0')
			return (1);
	}
	if (dictfp != NULL && feof(dictfp))	/* Special case for z's */
		clearerr(dictfp);
	return (0);
}

/*
 * Check each word in the dictionary against the board
 * Delete words from the machine list that the player has found
 * Assume both the dictionary and the player's words are already sorted
 */
static void
checkdict(void)
{
	char *p, *w;
	const char **pw;
	int i;
	int prevch, previndex, st;

	mwordsp = mwords;
	nmwords = 0;
	pw = pword;
	prevch ='a';

	(void) dictseek(dictfp, 0L, SEEK_SET);
	while ((w = nextword(dictfp)) != NULL) {
		if (wordlen < minlength)
			continue;
		if (*w != prevch) {
			/*
			 * If we've moved on to a word with a different first
			 * letter then we can speed things up by skipping all
			 * words starting with a letter that doesn't appear in
			 * the cube.
			 */
			i = (int) (*w - 'a');
			while (i < 26 && letter_map[i][0] == -1)
				i++;
			if (i == 26)
				break;
			previndex = prevch - 'a';
			prevch = i + 'a';
			/*
			 * Fall through if the word's first letter appears in
			 * the cube (i.e., if we can't skip ahead), otherwise
			 * seek to the beginning of words in the dictionary
			 * starting with the next letter (alphabetically)
			 * appearing in the cube and then read the first word.
			 */
			if (i != previndex + 1) {
				if (dictseek(dictfp,
				    dictindex[i].start, SEEK_SET) < 0) {
					warnx("seek error in checkdict()");
					cleanup();
					exit(1);
				}
				continue;
			}
		}

		clearwordpath(0);
		usedbits = 0;
		if (checkword(w, -1, wordpath) == -1)
			continue;

		st = 1;
		while (*pw != NULL && (st = strcmp(*pw, w)) < 0)
			pw++;
		if (st == 0)			/* found it */
			continue;
		if (nmwords == MAXMWORDS ||
		    mwordsp + wordlen + 1 >= &mwords[MAXMSPACE]) {
			warnx("too many words!");
			cleanup();
			exit(1);
		}
		mword[nmwords++] = mwordsp;
		p = w;
		while ((*mwordsp++ = *p++) != '\0')
		    	;
	}
}

/*
 * Crank up a new game
 * If the argument is non-null then it is assumed to be a legal board spec
 * in ascending cube order, oth. make a random board
 */
static void
newgame(const char *b)
{
	int i, p, q;
	const char *tmp;
	int *lm[26];
	static const char *cubes[16] = {
		"ednosw", "aaciot", "acelrs", "ehinps",
		"eefhiy", "elpstu", "acdemp", "gilruw",
		"egkluy", "ahmors", "abilty", "adenvz",
		"bfiorx", "dknotu", "abjmoq", "egintv"
	};

	if (b == NULL) {
		/*
		 * Shake the cubes and make the board
		 */
		i = 0;
		while (i < 100) {
			p = (int) (random() % 16);
			q = (int) (random() % 16);
			if (p != q) {
				tmp = cubes[p];
				cubes[p] = cubes[q];
				cubes[q] = tmp;
				i++;
			}
			/* else try again */
		}

		for (i = 0; i < 16; i++)
			board[i] = cubes[i][random() % 6];
	}
	else {
		for (i = 0; i < 16; i++)
			board[i] = b[i];
	}
	board[16] = '\0';

	/*
	 * Set up the map from letter to location(s)
	 * Each list is terminated by a -1 entry
	 */
	for (i = 0; i < 26; i++) {
		lm[i] = letter_map[i];
		*lm[i] = -1;
	}

	for (i = 0; i < 16; i++) {
		int j;

		j = (int) (board[i] - 'a');
		*lm[j] = i;
		*(++lm[j]) = -1;
	}

	if (debug) {
		for (i = 0; i < 26; i++) {
			int ch, j;

			(void) printf("%c:", 'a' + i);
			for (j = 0; (ch = letter_map[i][j]) != -1; j++)
				(void) printf(" %d", ch);
			(void) printf("\n");
		}
	}

}

/*
 * Clear wordpath[].
 */
static void
clearwordpath(int full)
{
	size_t pos;
	const size_t max = MAXWORDLEN + 1;

	for (pos = 0; pos < max && (full || wordpath[pos] != -1); pos++) {
		wordpath[pos] = -1;
	}
}

static int
compar(const void *p, const void *q)
{
	return (strcmp(*(const char *const *)p, *(const char *const *)q));
}

static void
usage(void)
{
	(void) fprintf(stderr,
	    "usage: %s [-bd] [-s#] [-t#] [-w#] [+[+]] [boardspec]\n",
	    getprogname());
	exit(1);
}
