/*	$NetBSD: shuffle.c,v 1.2 1998/09/23 21:45:44 perry Exp $	*/

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
__RCSID("$NetBSD: shuffle.c,v 1.2 1998/09/23 21:45:44 perry Exp $");
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

void enomem(void);
void *emalloc(size_t len);
void *ecalloc(size_t nmemb, size_t size);
char *estrdup(const char *str);
void *erealloc(void *ptr, size_t size);

int *get_shuffle(int t);
void usage(void);
void swallow_input(FILE *input);

char **global_inputbuf;
int global_inputlen;

/*
 * enomem --
 *	die when out of memory.
 */
void
enomem(void)
{
	errx(2, "Cannot allocate memory.");
}

/*
 * emalloc --
 *	malloc, but die on error.
 */
void *
emalloc(size_t len)
{
	void *p;

	if ((p = malloc(len)) == NULL)
		enomem();
	return(p);
}

/*
 * ecalloc --
 *	calloc, but die on error.
 */
void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if ((p = calloc(nmemb, size)) == NULL)
		enomem();
	return(p);
}


/*
 * estrdup --
 *	strdup, but die on error.
 */
char *
estrdup(const char *str)
{
	char *p;

	if ((p = strdup(str)) == NULL)
		enomem();
	return(p);
}

/*
 * erealloc --
 *	realloc, but die on error.
 */
void *
erealloc(void *ptr, size_t size)
{
	if ((ptr = realloc(ptr, size)) == NULL)
		enomem();
	return(ptr);
}

int *get_shuffle(int t)
{
	int *shuffle;
	int i, j, k, temp;
	
	shuffle = (int *)ecalloc(t, sizeof(int));

	for (i = 0; i < t; i++)
		shuffle[i] = i;
	
	/*
	 * This algorithm taken from Knuth, Seminumerical Algorithms,
	 * page 139.
	 */

	/* for (j = 0; j < t; j++) {
	   k = random()%t; */

	for (j = t-1; j > 0 ; j--) {
		k = random() % (j+1);
		temp = shuffle[j];
		shuffle[j] = shuffle[k];
		shuffle[k] = temp;
	}

	return(shuffle);
}

void
usage(void)
{
	errx(1, "usage: shuffle [-c arg ...] [-n number] [-p number] [file]");
}

void
swallow_input(FILE *input)
{
	int insize;
	int numlines;
	char **inputbuf, buf[BUFSIZ];
	

	insize = BUFSIZ;
	numlines = 0;

	inputbuf = emalloc(sizeof(char *) * insize);

	while (fgets(buf, BUFSIZ, input) != NULL) {
		inputbuf[numlines] = estrdup(buf);
		numlines++;
		if (numlines >= insize) {
			insize *= 2;
			inputbuf = erealloc(inputbuf,
			    (sizeof(char *) * insize));
		}
	}
	inputbuf[numlines] = NULL;

	global_inputbuf = inputbuf;
	global_inputlen = numlines;

}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int cflag, nflag, pflag, ch;
	int *shuffle;
	int i, p, t;
	FILE *input;
	struct timeval tv;

	cflag = nflag = pflag = 0;
	p = t = 0;
	
	while ((ch = getopt(argc, argv, "cn:p:")) != -1) {
		switch(ch) {
		case 'c':
			cflag = 1;
			break;
		case 'n':
			if ((t = atoi(optarg)) <= 0)
				errx(1, "%d: number is invalid\n", t);
			nflag = 1;
			break;
		case 'p':
			if ((p = atoi(optarg)) <= 0)
				errx(1, "%d: number is invalid\n", p);
			pflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((cflag && nflag) || (nflag && (argc > 0)))
		usage();

	if (cflag)
		t = argc;

	if (!(cflag || nflag)) {

		if (argc > 1)
			usage();
		if (argc == 1) {
			if ((input = fopen(argv[0], "r")) == NULL)
				err(1, "could not open %s", argv[0]);
		}
		else {
			input = stdin;
		}
		
		swallow_input(input);
		t = global_inputlen;
	}

	gettimeofday(&tv, NULL);
	srandom(getpid() ^ ~getuid() ^ tv.tv_sec ^ tv.tv_usec);

	shuffle = get_shuffle(t);

	if (pflag) {
		if (p > t)
			errx(1, "-p specified more components than exist.");
		t = p;
	}

	for (i = 0; i < t; i++) {
		if (nflag)
			printf("%d\n", shuffle[i]);
		else if (cflag)
			printf("%s\n", argv[shuffle[i]]);
		else
			printf("%s", global_inputbuf[shuffle[i]]);
	}

	return(0);
}
