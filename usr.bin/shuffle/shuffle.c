/*	$NetBSD: shuffle.c,v 1.9 2001/03/16 08:02:32 mycroft Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: shuffle.c,v 1.9 2001/03/16 08:02:32 mycroft Exp $");
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

static void enomem __P((void));
static void *emalloc __P((size_t));
static void *erealloc __P((void *, size_t));

static size_t *get_shuffle __P((size_t));
static void usage __P((void));
static void get_lines __P((const char *, char ***, size_t *));
static size_t get_number __P((const char *, int));

int main __P((int, char *[]));

/*
 * enomem --
 *	die when out of memory.
 */
static void
enomem()
{
	errx(2, "Cannot allocate memory.");
}

/*
 * emalloc --
 *	malloc, but die on error.
 */
static void *
emalloc(len)
	size_t len;
{
	void *p;

	if ((p = malloc(len)) == NULL)
		enomem();
	return p;
}

/*
 * erealloc --
 *	realloc, but die on error.
 */
void *
erealloc(ptr, size)
	void *ptr;
	size_t size;
{
	if ((ptr = realloc(ptr, size)) == NULL)
		enomem();
	return ptr;
}

/*
 * get_shuffle --
 *	Construct a random shuffle array of t elements
 */
static size_t *
get_shuffle(t)
	size_t t;
{
	size_t *shuffle;
	size_t i, j, k, temp;
	
	shuffle = emalloc(t * sizeof(size_t));

	for (i = 0; i < t; i++)
		shuffle[i] = i;
	
	/*
	 * This algorithm taken from Knuth, Seminumerical Algorithms,
	 * page 139.
	 */

	for (j = t - 1; j > 0; j--) {
		k = random() % (j + 1);
		temp = shuffle[j];
		shuffle[j] = shuffle[k];
		shuffle[k] = temp;
	}

	return shuffle;
}

/*
 * usage --
 *	Print a usage message and exit
 */
static void
usage()
{

	(void) fprintf(stderr,
    "Usage: %s [-f <filename>] [-n <number>] [-p <number>] [<arg> ...]\n",
		getprogname());
	exit(1);
}


/*
 * get_lines --
 *	Return an array of lines read from input
 */
static void
get_lines(fname, linesp, nlinesp)
	const char *fname;
	char ***linesp;
	size_t *nlinesp;
{
	FILE *fp;
	char *line;
	size_t size, nlines = 0, maxlines = 128;
	char **lines = emalloc(sizeof(char *) * maxlines);

	if (strcmp(fname, "-") == 0)
		fp = stdin;
	else
		if ((fp = fopen(fname, "r")) == NULL)
			err(1, "Cannot open `%s'", fname);

	while ((line = fgetln(fp, &size)) != NULL) {
		if (size > 0 && line[size - 1] == '\n')
			size--;
		lines[nlines] = emalloc(size + 1);
		(void)memcpy(lines[nlines], line, size);
		lines[nlines++][size] = '\0';
		if (nlines >= maxlines) {
			maxlines *= 2;
			lines = erealloc(lines, (sizeof(char *) * maxlines));
		}
	}
	lines[nlines] = NULL;

	*linesp = lines;
	*nlinesp = nlines;
	if (strcmp(fname, "-") != 0)
		(void)fclose(fp);
}

/*
 * get_number --
 *	Return a number or exit on error
 */
static size_t
get_number(str, ch)
	const char *str;
	int ch;
{
	char *estr;
	long number = strtol(str, &estr, 0);

	if ((number == LONG_MIN || number == LONG_MAX) && errno == ERANGE)
		err(1, "bad -%c argument `%s'", ch, str);
	if (*estr)
		errx(1, "non numeric -%c argument `%s'", ch, str);
	if (number < 0)
		errx(1, "negative -%c argument `%s'", ch, str);
	return (size_t) number;
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int i, nflag = 0, pflag = 0, ch;
	char *fname = NULL;
	size_t *shuffle = NULL;
	struct timeval tv;
	char **lines = NULL;
	size_t nlines = 0, pick = 0;
	
	while ((ch = getopt(argc, argv, "f:n:p:")) != -1) {
		switch(ch) {
		case 'f':
			fname = optarg;
			break;
		case 'n':
			nlines = get_number(optarg, ch);
			nflag = 1;
			break;
		case 'p':
			pick = get_number(optarg, ch);
			pflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((fname && nflag) || (nflag && (argc > 0)))
		usage();

	if (fname != NULL)
		get_lines(fname, &lines, &nlines);
	else if (nflag == 0) {
		lines = argv;
		nlines = argc;
	}

	gettimeofday(&tv, NULL);
	srandom(getpid() ^ ~getuid() ^ tv.tv_sec ^ tv.tv_usec);
	if (nlines > 0)
		shuffle = get_shuffle(nlines);

	if (pflag) {
		if (pick > nlines)
			errx(1, "-p specified more components than exist.");
		nlines = pick;
	}

	for (i = 0; i < nlines; i++) {
		if (nflag)
			printf("%ld\n", (long)shuffle[i]);
		else
			printf("%s\n", lines[shuffle[i]]);
	}

	return 0;
}
