/*	$NetBSD: spellprog.c,v 1.6.8.1 2008/01/09 02:01:03 matt Exp $	*/

/* derived from OpenBSD: spellprog.c,v 1.4 2003/06/03 02:56:16 millert Exp */

/*
 * Copyright (c) 1991, 1993
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
 *
 *	@(#)spell.h	8.1 (Berkeley) 6/6/93
 */
/*
 * Copyright (C) Caldera International Inc.  2001-2002.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code and documentation must retain the above
 *    copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed or owned by Caldera
 *	International, Inc.
 * 4. Neither the name of Caldera International, Inc. nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE FOR ANY DIRECT,
 * INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)spell.c	8.1 (Berkeley) 6/6/93";
#else
#endif
static const char rcsid[] = "$OpenBSD: spellprog.c,v 1.4 2003/06/03 02:56:16 millert Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

#define DLEV 2

static int	 dict(char *, char *);
static int	 trypref(char *, const char *, size_t);
static int	 tryword(char *, char *, size_t);
static int	 suffix(char *, size_t);
static int	 vowel(int);
static const char *lookuppref(char **, char *);
static char	*skipv(char *);
static void	 ise(void);
static void	 print_word(FILE *);
static void	 ztos(char *);
static int	 monosyl(char *, char *);
static void 	 usage(void) __dead;
static void	 getderiv(size_t);

static int	 an(char *, const char *, const char *, size_t);
static int	 bility(char *, const char *, const char *, size_t);
static int	 es(char *, const char *, const char *, size_t);
static int	 i_to_y(char *, const char *, const char *, size_t);
static int	 ily(char *, const char *, const char *, size_t);
static int	 ize(char *, const char *, const char *, size_t);
static int	 metry(char *, const char *, const char *, size_t);
static int	 ncy(char *, const char *, const char *, size_t);
static int	 nop(char *, const char *, const char *, size_t);
static int	 s(char *, const char *, const char *, size_t);
static int	 strip(char *, const char *, const char *, size_t);
static int	 tion(char *, const char *, const char *, size_t);
static int	 y_to_e(char *, const char *, const char *, size_t);
static int	 CCe(char *, const char *, const char *, size_t);
static int	 VCe(char *, const char *, const char *, size_t);

/*
 * This cannot be const because we modify it when we choose british
 * spelling.
 */
static struct suftab {
	const char *suf;
	int (*p1)(char *, const char *, const char *, size_t);
	int n1;
	const char *d1;
	const char *a1;
	int (*p2)(char *, const char *, const char *, size_t);
	int n2;
	const char *d2;
	const char *a2;
} suftab[] = {
	{ .suf = "ssen",	.p1 = ily,	.n1 = 4,
	  .d1 = "-y+iness", 	.a1 = "+ness" },
	{ .suf = "ssel",	.p1 = ily,	.n1 = 4,
	  .d1 = "-y+i+less", 	.a1 = "+less" },
	{ .suf = "se",		.p1 = s,	.n1 = 1,
	  .d1 = "", 		.a1 = "+s",	.p2 = es,
	  .n2 = 2,		.d2 = "-y+ies",	.a2 = "+es" },
	{ .suf = "s'",		.p1 = s,	.n1 = 2,
	  .d1 = "", 		.a1 = "+'s" },
	{ .suf = "s",		.p1 = s,	.n1 = 1,
	  .d1 = "", 		.a1 = "+s" },
	{ .suf = "ecn",		.p1 = ncy,	.n1 = 1,
	  .d1 = "", 		.a1 = "-t+ce" },
	{ .suf = "ycn",		.p1 = ncy,	.n1 = 1,
	  .d1 = "", 		.a1 = "-cy+t" },
	{ .suf = "ytilb",	.p1 = nop,	.n1 = 0,
	  .d1 = "", 		.a1 = "" },
	{ .suf = "ytilib",	.p1 = bility,	.n1 = 5,
	  .d1 = "-le+ility", 	.a1 = "" },
	{ .suf = "elbaif",	.p1 = i_to_y,	.n1 = 4,
	  .d1 = "-y+iable", 	.a1 = "" },
	{ .suf = "elba",	.p1 = CCe,	.n1 = 4,
	  .d1 = "-e+able", 	.a1 = "+able" },
	{ .suf = "yti",		.p1 = CCe,	.n1 = 3,
	  .d1 = "-e+ity", 	.a1 = "+ity" },
	{ .suf = "ylb",		.p1 = y_to_e,	.n1 = 1,
	  .d1 = "-e+y", 	.a1 = "" },
	{ .suf = "yl",		.p1 = ily,	.n1 = 2,
	  .d1 = "-y+ily", 	.a1 = "+ly" },
	{ .suf = "laci",	.p1 = strip,	.n1 = 2,
	  .d1 = "", 		.a1 = "+al" },
	{ .suf = "latnem",	.p1 = strip,	.n1 = 2,
	  .d1 = "", 		.a1 = "+al" },
	{ .suf = "lanoi",	.p1 = strip,	.n1 = 2,
	  .d1 = "", 		.a1 = "+al" },
	{ .suf = "tnem",	.p1 = strip,	.n1 = 4,
	  .d1 = "", 		.a1 = "+ment" },
	{ .suf = "gni",		.p1 = CCe,	.n1 = 3,
	  .d1 = "-e+ing", 	.a1 = "+ing" },
	{ .suf = "reta",	.p1 = nop,	.n1 = 0,
	  .d1 = "", 		.a1 = "" },
	{ .suf = "re",		.p1 = strip,	.n1 = 1,
	  .d1 = "", 		.a1 = "+r",	.p2 = i_to_y,
	  .n2 = 2,		.d2 = "-y+ier",	.a2 = "+er" },
	{ .suf = "de",		.p1 = strip,	.n1 = 1,
	  .d1 = "", 		.a1 = "+d",	.p2 = i_to_y,
	  .n2 = 2,		.d2 = "-y+ied",	.a2 = "+ed" },
	{ .suf = "citsi",	.p1 = strip,	.n1 = 2,
	  .d1 = "", 		.a1 = "+ic" },
	{ .suf = "cihparg",	.p1 = i_to_y,	.n1 = 1,
	  .d1 = "-y+ic", 	.a1 = "" },
	{ .suf = "tse",		.p1 = strip,	.n1 = 2,
	  .d1 = "", 		.a1 = "+st",	.p2 = i_to_y,
	  .n2 = 3,		.d2 = "-y+iest",.a2 = "+est" },
	{ .suf = "cirtem",	.p1 = i_to_y,	.n1 = 1,
	  .d1 = "-y+ic", 	.a1 = "" },
	{ .suf = "yrtem",	.p1 = metry,	.n1 = 0,
	  .d1 = "-ry+er", 	.a1 = "" },
	{ .suf = "cigol",	.p1 = i_to_y,	.n1 = 1,
	  .d1 = "-y+ic", 	.a1 = "" },
	{ .suf = "tsigol",	.p1 = i_to_y,	.n1 = 2,
	  .d1 = "-y+ist", 	.a1 = "" },
	{ .suf = "tsi",		.p1 = VCe,	.n1 = 3,
	  .d1 = "-e+ist", 	.a1 = "+ist" },
	{ .suf = "msi",		.p1 = VCe,	.n1 = 3,
	  .d1 = "-e+ism", 	.a1 = "+ist" },
	{ .suf = "noitacif",	.p1 = i_to_y,	.n1 = 6,
	  .d1 = "-y+ication", 	.a1 = "" },
	{ .suf = "noitazi",	.p1 = ize,	.n1 = 5,
	  .d1 = "-e+ation", 	.a1 = "" },
	{ .suf = "rota",	.p1 = tion,	.n1 = 2,
	  .d1 = "-e+or", 	.a1 = "" },
	{ .suf = "noit",	.p1 = tion,	.n1 = 3,
	  .d1 = "-e+ion", 	.a1 = "+ion" },
	{ .suf = "naino",	.p1 = an,	.n1 = 3,
	  .d1 = "", 		.a1 = "+ian" },
	{ .suf = "na",		.p1 = an,	.n1 = 1,
	  .d1 = "", 		.a1 = "+n" },
	{ .suf = "evit",	.p1 = tion,	.n1 = 3,
	  .d1 = "-e+ive", 	.a1 = "+ive" },
	{ .suf = "ezi",		.p1 = CCe,	.n1 = 3,
	  .d1 = "-e+ize", 	.a1 = "+ize" },
	{ .suf = "pihs",	.p1 = strip,	.n1 = 4,
	  .d1 = "", 		.a1 = "+ship" },
	{ .suf = "dooh",	.p1 = ily,	.n1 = 4,
	  .d1 = "-y+hood", 	.a1 = "+hood" },
	{ .suf = "ekil",	.p1 = strip,	.n1 = 4,
	  .d1 = "", 		.a1 = "+like" },
	{ .suf = NULL, }
};

static const char *preftab[] = {
	"anti",
	"bio",
	"dis",
	"electro",
	"en",
	"fore",
	"hyper",
	"intra",
	"inter",
	"iso",
	"kilo",
	"magneto",
	"meta",
	"micro",
	"milli",
	"mis",
	"mono",
	"multi",
	"non",
	"out",
	"over",
	"photo",
	"poly",
	"pre",
	"pseudo",
	"re",
	"semi",
	"stereo",
	"sub",
	"super",
	"thermo",
	"ultra",
	"under",	/* must precede un */
	"un",
	NULL
};

static struct wlist {
	int fd;
	unsigned char *front;
	unsigned char *back;
} *wlists;

static int vflag;
static int xflag;
static char word[LINE_MAX];
static char original[LINE_MAX];
static char affix[LINE_MAX];
static struct {
	const char **buf;
	size_t maxlev;
} deriv;

/*
 * The spellprog utility accepts a newline-delimited list of words
 * on stdin.  For arguments it expects the path to a word list and
 * the path to a file in which to store found words.
 *
 * In normal usage, spell is called twice.  The first time it is
 * called with a stop list to flag commonly mispelled words.  The
 * remaining words are then passed to spell again, this time with
 * the dictionary file as the first (non-flag) argument.
 *
 * Unlike historic versions of spellprog, this one does not use
 * hashed files.  Instead it simply requires that files be sorted
 * lexigraphically and uses the same algorithm as the look utility.
 *
 * Note that spellprog should be called via the spell shell script
 * and is not meant to be invoked directly by the user.
 */

int
main(int argc, char **argv)
{
	char *ep, *cp, *dp;
	char *outfile;
	int ch, fold, i;
	struct stat sb;
	FILE *file, *found;

	setlocale(LC_ALL, "");

	outfile = NULL;
	while ((ch = getopt(argc, argv, "bvxo:")) != -1) {
		switch (ch) {
		case 'b':
			/* Use British dictionary and convert ize -> ise. */
			ise();
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'v':
			/* Also write derivations to "found" file. */
			vflag++;
			break;
		case 'x':
			/* Print plausible stems to stdout. */
			xflag++;
			break;
		default:
			usage();
		}

	}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();

	/* Open and mmap the word/stop lists. */
	if ((wlists = malloc(sizeof(struct wlist) * (argc + 1))) == NULL)
		err(1, "malloc");

	for (i = 0; argc--; i++) {
		wlists[i].fd = open(argv[i], O_RDONLY, 0);
		if (wlists[i].fd == -1 || fstat(wlists[i].fd, &sb) != 0)
			err(1, "%s", argv[i]);
		if (sb.st_size > SIZE_T_MAX)
			errx(1, "%s: %s", argv[i], strerror(EFBIG));
		wlists[i].front = mmap(NULL, (size_t)sb.st_size, PROT_READ,
		    MAP_PRIVATE, wlists[i].fd, (off_t)0);
		if (wlists[i].front == MAP_FAILED)
			err(1, "%s", argv[i]);
		wlists[i].back = wlists[i].front + (size_t)sb.st_size;
	}
	wlists[i].fd = -1;

	/* Open file where found words are to be saved. */
	if (outfile == NULL)
		found = NULL;
	else if ((found = fopen(outfile, "w")) == NULL)
		err(1, "cannot open %s", outfile);

	for (;; print_word(file)) {
		affix[0] = '\0';
		file = found;
		for (ep = word; (*ep = ch = getchar()) != '\n'; ep++) {
			if (ep - word == sizeof(word) - 1) {
				*ep = '\0';
				warnx("word too long (%s)", word);
				while ((ch = getchar()) != '\n')
					;	/* slurp until EOL */
			}
			if (ch == EOF) {
				if (found != NULL)
					fclose(found);
				exit(0);
			}
		}
		for (cp = word, dp = original; cp < ep; )
			*dp++ = *cp++;
		*dp = '\0';
		fold = 0;
		for (cp = word; cp < ep; cp++)
			if (islower((unsigned char)*cp))
				goto lcase;
		if (trypref(ep, ".", 0))
			continue;
		++fold;
		for (cp = original + 1, dp = word + 1; dp < ep; dp++, cp++)
			*dp = tolower((unsigned char)*cp);
lcase:
		if (trypref(ep, ".", 0) || suffix(ep, 0))
			continue;
		if (isupper((unsigned char)word[0])) {
			for (cp = original, dp = word; (*dp = *cp++); dp++) {
				if (fold)
					*dp = tolower((unsigned char)*dp);
			}
			word[0] = tolower((unsigned char)word[0]);
			goto lcase;
		}
		file = stdout;
	}
}

static void
print_word(FILE *f)
{

	if (f != NULL) {
		if (vflag && affix[0] != '\0' && affix[0] != '.')
			fprintf(f, "%s\t%s\n", affix, original);
		else
			fprintf(f, "%s\n", original);
	}
}

/*
 * For each matching suffix in suftab, call the function associated
 * with that suffix (p1 and p2).
 */
static int
suffix(char *ep, size_t lev)
{
	const struct suftab *t;
	char *cp;
	const char *sp;

	lev += DLEV;
	getderiv(lev + 1);
	deriv.buf[lev] = deriv.buf[lev - 1] = 0;
	for (t = suftab; (sp = t->suf) != NULL; t++) {
		cp = ep;
		while (*sp) {
			if (*--cp != *sp++)
				goto next;
		}
		for (sp = cp; --sp >= word && !vowel(*sp);)
			;	/* nothing */
		if (sp < word)
			return 0;
		if ((*t->p1)(ep - t->n1, t->d1, t->a1, lev + 1))
			return 1;
		if (t->p2 != NULL) {
			deriv.buf[lev] = deriv.buf[lev + 1] = '\0';
			return (*t->p2)(ep - t->n2, t->d2, t->a2, lev);
		}
		return 0;
next:		;
	}
	return 0;
}

static int
/*ARGSUSED*/
nop(char *ep, const char *d, const char *a, size_t lev)
{

	return 0;
}

static int
/*ARGSUSED*/
strip(char *ep, const char *d, const char *a, size_t lev)
{

	return trypref(ep, a, lev) || suffix(ep, lev);
}

static int
s(char *ep, const char *d, const char *a, const size_t lev)
{

	if (lev > DLEV + 1)
		return 0;
	if (*ep == 's' && ep[-1] == 's')
		return 0;
	return strip(ep, d, a, lev);
}

static int
/*ARGSUSED*/
an(char *ep, const char *d, const char *a, size_t lev)
{

	if (!isupper((unsigned char)*word))	/* must be proper name */
		return 0;
	return trypref(ep, a, lev);
}

static int
/*ARGSUSED*/
ize(char *ep, const char *d, const char *a, size_t lev)
{

	*ep++ = 'e';
	return strip(ep ,"", d, lev);
}

static int
/*ARGSUSED*/
y_to_e(char *ep, const char *d, const char *a, size_t lev)
{
	char c = *ep;

	*ep++ = 'e';
	if (strip(ep, "", d, lev))
		return 1;
	ep[-1] = c;
	return 0;
}

static int
ily(char *ep, const char *d, const char *a, size_t lev)
{

	if (ep[-1] == 'i')
		return i_to_y(ep, d, a, lev);
	else
		return strip(ep, d, a, lev);
}

static int
ncy(char *ep, const char *d, const char *a, size_t lev)
{

	if (skipv(skipv(ep - 1)) < word)
		return 0;
	ep[-1] = 't';
	return strip(ep, d, a, lev);
}

static int
bility(char *ep, const char *d, const char *a, size_t lev)
{

	*ep++ = 'l';
	return y_to_e(ep, d, a, lev);
}

static int
i_to_y(char *ep, const char *d, const char *a, size_t lev)
{

	if (ep[-1] == 'i') {
		ep[-1] = 'y';
		a = d;
	}
	return strip(ep, "", a, lev);
}

static int
es(char *ep, const char *d, const char *a, size_t lev)
{

	if (lev > DLEV)
		return 0;

	switch (ep[-1]) {
	default:
		return 0;
	case 'i':
		return i_to_y(ep, d, a, lev);
	case 's':
	case 'h':
	case 'z':
	case 'x':
		return strip(ep, d, a, lev);
	}
}

static int
metry(char *ep, const char *d, const char *a, size_t lev)
{

	ep[-2] = 'e';
	ep[-1] = 'r';
	return strip(ep, d, a, lev);
}

static int
tion(char *ep, const char *d, const char *a, size_t lev)
{

	switch (ep[-2]) {
	case 'c':
	case 'r':
		return trypref(ep, a, lev);
	case 'a':
		return y_to_e(ep, d, a, lev);
	}
	return 0;
}

/*
 * Possible consonant-consonant-e ending.
 */
static int
CCe(char *ep, const char *d, const char *a, size_t lev)
{

	switch (ep[-1]) {
	case 'l':
		if (vowel(ep[-2]))
			break;
		switch (ep[-2]) {
		case 'l':
		case 'r':
		case 'w':
			break;
		default:
			return y_to_e(ep, d, a, lev);
		}
		break;
	case 's':
		if (ep[-2] == 's')
			break;
		/*FALLTHROUGH*/
	case 'c':
	case 'g':
		if (*ep == 'a')
			return 0;
		/*FALLTHROUGH*/
	case 'v':
	case 'z':
		if (vowel(ep[-2]))
			break;
		/*FALLTHROUGH*/
	case 'u':
		if (y_to_e(ep, d, a, lev))
			return 1;
		if (!(ep[-2] == 'n' && ep[-1] == 'g'))
			return 0;
	}
	return VCe(ep, d, a, lev);
}

/*
 * Possible consonant-vowel-consonant-e ending.
 */
static int
VCe(char *ep, const char *d, const char *a, size_t lev)
{
	char c;

	c = ep[-1];
	if (c == 'e')
		return 0;
	if (!vowel(c) && vowel(ep[-2])) {
		c = *ep;
		*ep++ = 'e';
		if (trypref(ep, d, lev) || suffix(ep, lev))
			return 1;
		ep--;
		*ep = c;
	}
	return strip(ep, d, a, lev);
}

static const char *
lookuppref(char **wp, char *ep)
{
	const char **sp, *cp;
	char *bp;

	for (sp = preftab; *sp; sp++) {
		bp = *wp;
		for (cp = *sp; *cp; cp++, bp++) {
			if (tolower((unsigned char)*bp) != *cp)
				goto next;
		}
		for (cp = bp; cp < ep; cp++) {
			if (vowel(*cp)) {
				*wp = bp;
				return *sp;
			}
		}
next:		;
	}
	return 0;
}

/*
 * If the word is not in the dictionary, try stripping off prefixes
 * until the word is found or we run out of prefixes to check.
 */
static int
trypref(char *ep, const char *a, size_t lev)
{
	const char *cp;
	char *bp;
	char *pp;
	int val = 0;
	char space[20];

	getderiv(lev + 2);
	deriv.buf[lev] = a;
	if (tryword(word, ep, lev))
		return 1;
	bp = word;
	pp = space;
	deriv.buf[lev + 1] = pp;
	while ((cp = lookuppref(&bp, ep)) != NULL) {
		*pp++ = '+';
		while ((*pp = *cp++))
			pp++;
		if (tryword(bp, ep, lev + 1)) {
			val = 1;
			break;
		}
		if (pp - space >= sizeof(space))
			return 0;
	}
	deriv.buf[lev + 1] = deriv.buf[lev + 2] = '\0';
	return val;
}

static int
tryword(char *bp, char *ep, size_t lev)
{
	size_t i, j;
	char duple[3];

	if (ep-bp <= 1)
		return 0;
	if (vowel(*ep) && monosyl(bp, ep))
		return 0;

	i = dict(bp, ep);
	if (i == 0 && vowel(*ep) && ep[-1] == ep[-2] &&
	    monosyl(bp, ep - 1)) {
		ep--;
		getderiv(++lev);
		deriv.buf[lev] = duple;
		duple[0] = '+';
		duple[1] = *ep;
		duple[2] = '\0';
		i = dict(bp, ep);
	}
	if (vflag == 0 || i == 0)
		return i;

	/* Also tack on possible derivations. (XXX - warn on truncation?) */
	for (j = lev; j > 0; j--) {
		if (deriv.buf[j])
			(void)strlcat(affix, deriv.buf[j], sizeof(affix));
	}
	return i;
}

static int
monosyl(char *bp, char *ep)
{

	if (ep < bp + 2)
		return 0;
	if (vowel(*--ep) || !vowel(*--ep) || ep[1] == 'x' || ep[1] == 'w')
		return 0;
	while (--ep >= bp)
		if (vowel(*ep))
			return 0;
	return 1;
}

static char *
skipv(char *st)
{

	if (st >= word && vowel(*st))
		st--;
	while (st >= word && !vowel(*st))
		st--;
	return st;
}

static int
vowel(int c)
{

	switch (tolower(c)) {
	case 'a':
	case 'e':
	case 'i':
	case 'o':
	case 'u':
	case 'y':
		return 1;
	}
	return 0;
}

/*
 * Crummy way to Britishise.
 */
static void
ise(void)
{
	struct suftab *tab;
	char *cp;

	for (tab = suftab; tab->suf; tab++) {
		/* Assume that suffix will contain 'z' if a1 or d1 do */
		if (strchr(tab->suf, 'z')) {
			tab->suf = cp = estrdup(tab->suf);
			ztos(cp);
			if (strchr(tab->d1, 'z')) {
				tab->d1 = cp = estrdup(tab->d1);
				ztos(cp);
			}
			if (strchr(tab->a1, 'z')) {
				tab->a1 = cp = estrdup(tab->a1);
				ztos(cp);
			}
		}
	}
}

static void
ztos(char *st)
{

	for (; *st; st++)
		if (*st == 'z')
			*st = 's';
}

/*
 * Look up a word in the dictionary.
 * Returns 1 if found, 0 if not.
 */
static int
dict(char *bp, char *ep)
{
	char c;
	int i, rval;

	c = *ep;
	*ep = '\0';
	if (xflag)
		printf("=%s\n", bp);
	for (i = rval = 0; wlists[i].fd != -1; i++) {
		if ((rval = look((unsigned char *)bp, wlists[i].front,
		    wlists[i].back)) == 1)
			break;
	}
	*ep = c;
	return rval;
}

static void
getderiv(size_t lev)
{
	if (deriv.maxlev < lev) {
		void *p = realloc(deriv.buf, sizeof(*deriv.buf) * lev);
		if (p == NULL)
			err(1, "Cannot grow array");
		deriv.buf = p;
		deriv.maxlev = lev;
	}
}


static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-bvx] [-o found-words] word-list ...\n",
	    getprogname());
	exit(1);
}
