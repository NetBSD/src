/*	$NetBSD: pac.c,v 1.24 2016/01/17 14:50:31 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "@(#)pac.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: pac.c,v 1.24 2016/01/17 14:50:31 christos Exp $");
#endif
#endif /* not lint */

/*
 * Do Printer accounting summary.
 * Currently, usage is
 *	pac [-Pprinter] [-pprice] [-s] [-r] [-c] [-m] [user ...]
 * to print the usage information for the named people.
 */

#include <sys/param.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "lp.h"
#include "lp.local.h"

static char	*acctfile;	/* accounting file (input data) */
static int	 allflag = 1;	/* Get stats on everybody */
static int	 errs;
static int	 hcount;	/* Count of hash entries */
static int	 mflag = 0;	/* disregard machine names */
static int	 pflag = 0;	/* 1 if -p on cmd line */
static float	 price = 0.02;	/* cost per page (or what ever) */
static long	 price100;	/* per-page cost in 100th of a cent */
static int	 reverse;	/* Reverse sort order */
static int	 sort;		/* Sort by cost */
static char	*sumfile;	/* summary file */
static int	 summarize;	/* Compress accounting file */

/*
 * Grossness follows:
 *  Names to be accumulated are hashed into the following
 *  table.
 */

#define	HSHSIZE	97			/* Number of hash buckets */

struct hent {
	struct	hent *h_link;		/* Forward hash link */
	char	*h_name;		/* Name of this user */
	float	h_feetpages;		/* Feet or pages of paper */
	int	h_count;		/* Number of runs */
};

static struct	hent	*hashtab[HSHSIZE];	/* Hash table proper */

static void	account(FILE *);
static int	chkprinter(const char *);
static void	dumpit(void);
static int	hash(const char *);
static struct	hent *enter(const char *);
static struct	hent *lookup(const char *);
static int	qucmp(const void *, const void *);
static void	rewrite(void);
static void	usage(void) __dead;

int
main(int argc, char *const argv[])
{
	FILE *acf;
	int opt;

	while ((opt = getopt(argc, argv, "P:p:scmr")) != -1) {
		switch(opt) {
		case 'P':
			/*
			 * Printer name.
			 */
			printer = optarg;
			continue;

		case 'p':
			/*
			 * get the price.
			 */
			price = atof(optarg);
			pflag = 1;
			continue;

		case 's':
			/*
			 * Summarize and compress accounting file.
			 */
			summarize++;
			continue;

		case 'c':
			/*
			 * Sort by cost.
			 */
			sort++;
			continue;

		case 'm':
			/*
			 * disregard machine names for each user
			 */
			mflag = 1;
			continue;

		case 'r':
			/*
			 * Reverse sorting order.
			 */
			reverse++;
			continue;

		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * If there are any arguments left, they're names of users
	 * we want to print info for. In that case, put them in the hash
	 * table and unset allflag.
	 */
	for( ; argc > 0; argc--, argv++) {
		(void)enter(*argv);
		allflag = 0;
	}

	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;
	if (!chkprinter(printer)) {
		printf("pac: unknown printer %s\n", printer);
		exit(2);
	}

	if ((acf = fopen(acctfile, "r")) == NULL)
		err(1, "%s", acctfile);
	account(acf);
	fclose(acf);
	if ((acf = fopen(sumfile, "r")) != NULL) {
		account(acf);
		fclose(acf);
	}
	if (summarize)
		rewrite();
	else
		dumpit();
	exit(errs);
}

/*
 * Read the entire accounting file, accumulating statistics
 * for the users that we have in the hash table.  If allflag
 * is set, then just gather the facts on everyone.
 * Note that we must accommodate both the active and summary file
 * formats here.
 * Format of accounting file is
 *	feet_per_page	[runs_count] [hostname:]username
 * Some software relies on whitespace between runs_count and hostname:username
 * being optional (such as Ghostscript's unix-lpr.sh).
 *
 * Host names are ignored if the -m flag is present.
 */
static void
account(FILE *acf)
{
	char who[BUFSIZ];
	char linebuf[BUFSIZ];
	float t;
	char *cp, *cp2;
	struct hent *hp;
	int ic;

	while (fgets(linebuf, BUFSIZ, acf) != NULL) {
		/* XXX sizeof(who) == 1024 */
		if (sscanf(linebuf, "%f %d%1023s", &t, &ic, who) == 0) {
			sscanf(linebuf, "%f %1023s", &t, who);
			ic = 1;
		}
		
		/* if -m was specified, don't use the hostname part */
		if (mflag && (cp2 = strchr(who, ':')))
			cp = cp2 + 1;
		else
			cp = who;

		hp = lookup(cp);
		if (hp == NULL) {
			if (!allflag)
				continue;
			hp = enter(cp);
		}
		hp->h_feetpages += t;
		if (ic)
			hp->h_count += ic;
		else
			hp->h_count++;
	}
}

/*
 * Sort the hashed entries by name or footage
 * and print it all out.
 */
static void
dumpit(void)
{
	struct hent **base;
	struct hent *hp, **ap;
	int hno, c, runs;
	float feet;

	hp = hashtab[0];
	hno = 1;
	base = calloc(sizeof hp, hcount);
	if (base == NULL)
		err(1, "calloc");
	for (ap = base, c = hcount; c--; ap++) {
		while (hp == NULL)
			hp = hashtab[hno++];
		*ap = hp;
		hp = hp->h_link;
	}
	qsort(base, hcount, sizeof hp, qucmp);
	printf(" pages/feet runs price    %s\n",
		(mflag ? "login" : "host name and login"));
	printf(" ---------- ---- -------- ----------------------\n");
	feet = 0.0;
	runs = 0;
	for (ap = base, c = hcount; c--; ap++) {
		hp = *ap;
		runs += hp->h_count;
		feet += hp->h_feetpages;
		printf("    %7.2f %4d $%7.2f %s\n",
			hp->h_feetpages, hp->h_count, 
			hp->h_feetpages * price * hp->h_count,
			hp->h_name);
	}
	if (allflag) {
		printf(" ---------- ---- -------- ----------------------\n");
		printf("Sum:%7.2f %4d $%7.2f\n", feet, runs, 
			feet * price * runs);
	}
	free(base);
}

/*
 * Rewrite the summary file with the summary information we have accumulated.
 */
static void
rewrite(void)
{
	struct hent *hp;
	int i;
	FILE *acf;

	if ((acf = fopen(sumfile, "w")) == NULL) {
		warn("%s", sumfile);
		errs++;
		return;
	}
	for (i = 0; i < HSHSIZE; i++) {
		hp = hashtab[i];
		while (hp != NULL) {
			fprintf(acf, "%7.2f\t%s\t%d\n", hp->h_feetpages,
			    hp->h_name, hp->h_count);
			hp = hp->h_link;
		}
	}
	fflush(acf);
	if (ferror(acf)) {
		warn("%s", sumfile);
		errs++;
	}
	fclose(acf);
	if ((acf = fopen(acctfile, "w")) == NULL)
		warn("%s", acctfile);
	else
		fclose(acf);
}

/*
 * Hashing routines.
 */

/*
 * Enter the name into the hash table and return the pointer allocated.
 */

static struct hent *
enter(const char *name)
{
	struct hent *hp;
	int h;

	if ((hp = lookup(name)) != NULL)
		return(hp);
	h = hash(name);
	hcount++;
	hp = (struct hent *) calloc(sizeof *hp, 1);
	if (hp == NULL)
		err(1, "calloc");
	hp->h_name = strdup(name);
	if (hp->h_name == NULL)
		err(1, "malloc");
	hp->h_feetpages = 0.0;
	hp->h_count = 0;
	hp->h_link = hashtab[h];
	hashtab[h] = hp;
	return(hp);
}

/*
 * Lookup a name in the hash table and return a pointer
 * to it.
 */

static struct hent *
lookup(const char *name)
{
	int h;
	struct hent *hp;

	h = hash(name);
	for (hp = hashtab[h]; hp != NULL; hp = hp->h_link)
		if (strcmp(hp->h_name, name) == 0)
			return(hp);
	return(NULL);
}

/*
 * Hash the passed name and return the index in
 * the hash table to begin the search.
 */
static int
hash(const char *name)
{
	int h;
	const char *cp;

	for (cp = name, h = 0; *cp; h = (h << 2) + *cp++)
		;
	return((h & 0x7fffffff) % HSHSIZE);
}

/*
 * The qsort comparison routine.
 * The comparison is ascii collating order
 * or by feet of typesetter film, according to sort.
 */
static int
qucmp(const void *a, const void *b)
{
	const struct hent *h1, *h2;
	int r;

	h1 = *(const struct hent *const *)a;
	h2 = *(const struct hent *const *)b;
	if (sort)
		r = h1->h_feetpages < h2->h_feetpages ?
		    -1 : h1->h_feetpages > h2->h_feetpages;
	else
		r = strcmp(h1->h_name, h2->h_name);
	return(reverse ? -r : r);
}

/*
 * Perform lookup for printer name or abbreviation --
 */
static int
chkprinter(const char *s)
{
	int stat;

	if ((stat = cgetent(&bp, printcapdb, s)) == -2) {
		printf("pac: can't open printer description file\n");
		exit(3);
	} else if (stat == -1)
		return(0);
	else if (stat == -3)
		fatal("potential reference loop detected in printcap file");

	if (cgetstr(bp, "af", &acctfile) == -1) {
		printf("accounting not enabled for printer %s\n", printer);
		exit(2);
	}
	if (!pflag && (cgetnum(bp, "pc", &price100) == 0))
		price = price100/10000.0;
	asprintf(&sumfile, "%s_sum", acctfile);
	if (sumfile == NULL)
		err(1, "pac");
	return(1);
}

static void
usage(void)
{
	fprintf(stderr,
	  "usage: pac [-Pprinter] [-pprice] [-s] [-c] [-r] [-m] [user ...]\n");
	exit(1);
}
