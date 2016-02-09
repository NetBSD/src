/*	Id: cpp.c,v 1.252 2016/02/06 09:39:21 ragge Exp 	*/	
/*	$NetBSD: cpp.c,v 1.4 2016/02/09 20:37:32 plunky Exp $	*/

/*
 * Copyright (c) 2004,2010 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * The C preprocessor.
 * This code originates from the V6 preprocessor with some additions
 * from V7 cpp, and at last ansi/c99 support.
 *
 * 	- kfind() expands the input buffer onto XXX
 *	- exparg() expand one buffer into another.
 *		Recurses into submac() for fun-like macros.
 *	- submac() replaces the given macro.
 *		Recurses into subarg() for fun-like macros.
 *	- subarg() expands fun-like macros.
 *		Create strings, concats args, recurses into exparg.
 */

#include "config.h"

#include <sys/stat.h>

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "compat.h"
#include "cpp.h"

#ifndef S_ISDIR
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

#define	SBSIZE	1000000

static usch	sbf[SBSIZE];
static int	counter;
/* C command */

int tflag;	/* traditional cpp syntax */
#ifdef PCC_DEBUG
int dflag;	/* debug printouts */
//static void imp(const char *);
static void prline(const usch *s);
static void prrep(const usch *s);
#define	DPRINT(x) if (dflag) printf x
#define	DDPRINT(x) if (dflag > 1) printf x
#define	IMP(x) if (dflag > 1) imp(x)
#else
#define DPRINT(x)
#define DDPRINT(x)
#define IMP(x)
#endif

int Aflag, Cflag, Eflag, Mflag, dMflag, Pflag, MPflag, MMDflag;
char *Mfile, *MPfile;
struct initar *initar;
char *Mxfile;
int warnings, Mxlen;
FILE *of;

/* include dirs */
struct incs {
	struct incs *next;
	usch *dir;
	dev_t dev;
	ino_t ino;
} *incdir[2];

static struct symtab *filloc;
static struct symtab *linloc;
static struct symtab *pragloc;
static struct symtab *defloc;
static struct symtab *ctrloc;
int	trulvl;
int	flslvl;
int	elflvl;
int	elslvl;
usch *stringbuf = sbf;

/*
 * Macro replacement list syntax:
 * - For object-type macros, replacement strings are stored as-is.
 * - For function-type macros, macro args are substituted for the
 *   character WARN followed by the argument number.
 * - The value element points to the beginning of the string.
 *
 * The first character in the replacement list is the number of arguments:
 *   VARG  - ends with ellipsis, next char is argcount without ellips.
 *   OBJCT - object-type macro
 *   0	   - empty parenthesis, foo()
 *   1->   - number of args.
 *
 * WARN is used:
 *	- in stored replacement lists to tell that an argument comes
 *	- When expanding replacement lists to tell that the list ended.
 *
 * To ensure that an already expanded identifier won't get expanded
 * again a EBLOCK char + its number is stored directly before any
 * expanded identifier.
 */

/* args for lookup() */
#define	FIND	0
#define	ENTER	1

/*
 * No-replacement array.  If a macro is found and exists in this array
 * then no replacement shall occur.
 */
struct blocker {
	struct blocker *next;
	struct symtab *sp;
};
struct blocker *blkidx[RECMAX];
int blkidp;

static int readargs2(usch **, struct symtab *sp, const usch **args);
static int readargs1(struct symtab *sp, const usch **args);
static struct iobuf *exparg(int, struct iobuf *, struct iobuf *, struct blocker *);
static struct iobuf *subarg(struct symtab *sp, const usch **args, int, struct blocker *);
static void usage(void);
static usch *xstrdup(const usch *str);
static void addidir(char *idir, struct incs **ww);
static void vsheap(const char *, va_list);
static int skipws(struct iobuf *ib);
static int getyp(usch *s);
static void *xrealloc(void *p, int sz);
static void *xmalloc(int sz);

usch locs[] =
	{ FILLOC, LINLOC, PRAGLOC, DEFLOC,
	    'd','e','f','i','n','e','d',0, CTRLOC };

int
main(int argc, char **argv)
{
	struct initar *it;
	register int ch;
	const usch *fn1, *fn2;

#ifdef TIMING
	struct timeval t1, t2;

	(void)gettimeofday(&t1, NULL);
#endif

	while ((ch = getopt(argc, argv, "ACD:d:EI:i:MPS:tU:Vvx:")) != -1) {
		switch (ch) {
		case 'A': /* assembler input */
			Aflag++;
			break;

		case 'C': /* Do not discard comments */
			Cflag++;
			break;

		case 'E': /* treat warnings as errors */
			Eflag++;
			break;

		case 'D': /* define something */
		case 'i': /* include */
		case 'U': /* undef */
			/* XXX should not need malloc() here */
			if ((it = xmalloc(sizeof(struct initar))) == NULL)
				error("couldn't apply -%c %s", ch, optarg);
			it->type = ch;
			it->str = optarg;
			it->next = initar;
			initar = it;
			break;

		case 'd':
			while (*optarg) {
				switch(*optarg) {
				case 'M': /* display macro definitions */
					dMflag = 1;
					Mflag = 1;
					break;

				default: /* ignore others */
					break;
				}
				optarg++;
			}
			break;

		case 'I':
		case 'S':
			addidir(optarg, &incdir[ch == 'I' ? INCINC : SYSINC]);
			break;

		case 'M': /* Generate dependencies for make */
			Mflag++;
			break;

		case 'P': /* Inhibit generation of line numbers */
			Pflag++;
			break;

		case 't':
			tflag = 1;
			break;

#ifdef PCC_DEBUG
		case 'V':
			dflag++;
			break;
#endif
		case 'v':
			fprintf(stderr, "PCC preprocessor version "VERSSTR"\n");
			break;

		case 'x':
			if (strcmp(optarg, "MMD") == 0) {
				MMDflag++;
			} else if (strcmp(optarg, "MP") == 0) {
				MPflag++;
			} else if (strncmp(optarg, "MT,", 3) == 0 ||
			    strncmp(optarg, "MQ,", 3) == 0) {
				int l = strlen(optarg+3) + 2;
				char *cp, *up;

				if (optarg[1] == 'Q')
					for (cp = optarg+3; *cp; cp++)
						if (*cp == '$')
							l++;
				Mxlen += l;
				Mxfile = cp = realloc(Mxfile, Mxlen);
				for (up = Mxfile; *up; up++)
					;
				if (up != Mxfile)
					*up++ = ' ';
				for (cp = optarg+3; *cp; cp++) {
					*up++ = *cp;
					if (optarg[1] == 'Q' && *cp == '$')
						*up++ = *cp;
				}
				*up = 0;
			} else
				usage();
			break;

		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	filloc = lookup((const usch *)"__FILE__", ENTER);
	linloc = lookup((const usch *)"__LINE__", ENTER);
	pragloc = lookup((const usch *)"_Pragma", ENTER);
	defloc = lookup((const usch *)"defined", ENTER);
	ctrloc = lookup((const usch *)"__COUNTER__", ENTER);
	filloc->value = locs;
	linloc->value = locs+1;
	pragloc->value = locs+2;
	defloc->value = locs+3; /* also have macro name here */
	ctrloc->value = locs+12;

	if (Mflag && !dMflag) {
		char *c;

		if (argc < 1)
			error("-M and no infile");
		if ((c = strrchr(argv[0], '/')) == NULL)
			c = argv[0];
		else
			c++;
		Mfile = (char *)xstrdup((usch *)c);
		if (MPflag)
			MPfile = (char *)xstrdup((usch *)c);
		if (Mxfile)
			Mfile = Mxfile;
		if ((c = strrchr(Mfile, '.')) == NULL)
			error("-M and no extension: ");
		c[1] = 'o';
		c[2] = 0;
	}

	if (argc == 2) {
		if ((of = freopen(argv[1], "w", stdout)) == NULL)
			error("Can't creat %s", argv[1]);
	} else
		of = stdout;

	if (argc && strcmp(argv[0], "-")) {
		fn1 = fn2 = (usch *)argv[0];
	} else {
		fn1 = NULL;
		fn2 = (const usch *)"";
	}
	if (pushfile(fn1, fn2, 0, NULL))
		error("cannot open %s", argv[0]);

	fclose(of);
#ifdef TIMING
	(void)gettimeofday(&t2, NULL);
	t2.tv_sec -= t1.tv_sec;
	t2.tv_usec -= t1.tv_usec;
	if (t2.tv_usec < 0) {
		t2.tv_usec += 1000000;
		t2.tv_sec -= 1;
	}
	fprintf(stderr, "cpp total time: %ld s %ld us\n",
	     (long)t2.tv_sec, (long)t2.tv_usec);
#endif
	if (Eflag && warnings > 0)
		return 2;

	return 0;
}

/*
 * Write a character to an out buffer.
 */
static void
putob(struct iobuf *ob, int ch)
{
	if (ob->cptr == ob->bsz) {
		int sz = ob->bsz - ob->buf;
		ob->buf = xrealloc(ob->buf, sz + BUFSIZ);
		ob->cptr = ob->buf + sz;
		ob->bsz = ob->buf + sz + BUFSIZ;
	}
//	DDPRINT(("putob: iob %p pos %p ch %c (%d)\n", ob, ob->cptr, ch, ch));
	*ob->cptr++ = ch;
}

static int nbufused;
/*
 * Write a character to an out buffer.
 */
static struct iobuf *
getobuf(void)
{
	struct iobuf *iob = xmalloc(sizeof(struct iobuf));

	nbufused++;
	iob->buf = iob->cptr = xmalloc(BUFSIZ);
	iob->bsz = iob->buf + BUFSIZ;
	iob->ro = 0;
	return iob;
}

/*
 * Create a read-only input buffer.
 */
static struct iobuf *
mkrobuf(const usch *s)
{
	struct iobuf *iob = xmalloc(sizeof(struct iobuf));

	nbufused++;
	DPRINT(("mkrobuf %s\n", s));
	iob->buf = iob->cptr = (usch *)s;
	iob->bsz = iob->buf + strlen((char *)iob->buf);
	iob->ro = 1;
	return iob;
}

/*
 * Copy a string to a buffer.
 */
static struct iobuf *
strtobuf(usch *str, struct iobuf *iob)
{
	DPRINT(("strtobuf iob %p buf %p str %s\n", iob, iob->buf, str));
	if (iob == NULL)
		iob = getobuf();
	do {
		putob(iob, *str);
	} while (*str++);
	iob->cptr--;
	return iob;
}

static void
bufree(struct iobuf *iob)
{
	nbufused--;
	if (iob->ro == 0)
		free(iob->buf);
	free(iob);
}

static void
addidir(char *idir, struct incs **ww)
{
	struct incs *w;
	struct stat st;

	if (stat(idir, &st) == -1 || !S_ISDIR(st.st_mode))
		return; /* ignore */
	if (*ww != NULL) {
		for (w = *ww; w->next; w = w->next) {
#ifdef _WIN32
			if (strcmp(w->dir, idir) == 0)
				return;
#else
			if (w->dev == st.st_dev && w->ino == st.st_ino)
				return;
#endif
		}
#ifdef _WIN32
		if (strcmp(w->dir, idir) == 0)
			return;
#else
		if (w->dev == st.st_dev && w->ino == st.st_ino)
			return;
#endif
		ww = &w->next;
	}
	if ((w = calloc(sizeof(struct incs), 1)) == NULL)
		error("couldn't add path %s", idir);
	w->dir = (usch *)idir;
	w->dev = st.st_dev;
	w->ino = st.st_ino;
	*ww = w;
}

void
line(void)
{
	struct symtab *nl;
	int c, n, ln;
	usch *cp;

	cp = stringbuf;
	c = skipws(0);
	if (ISID0(c)) { /* expand macro */
		heapid(c);
		stringbuf = cp;
		if ((nl = lookup(cp, FIND)) == 0 || kfind(nl) == 0)
			goto bad;
	} else {
		do {
			savch(c);
		} while (ISDIGIT(c = cinput()));
		cunput(c);
		savch(0);
	}

	stringbuf = cp;
	n = 0;
	while (ISDIGIT(*cp))
		n = n * 10 + *cp++ - '0';
	if (*cp != 0)
		goto bad;

	/* Can only be decimal number here between 1-2147483647 */
	if (n < 1 || n > 2147483647)
		goto bad;

	ln = n;
	ifiles->escln = 0;
	if ((c = skipws(NULL)) != '\n') {
		if (c == 'L' || c == 'U' || c == 'u') {
			n = c, c = cinput();
			if (n == 'u' && c == '8')
				c = cinput();
			if (c == '\"')
				warning("#line only allows character literals");
		}
		if (c != '\"')
			goto bad;
		/* loses space on heap... does it matter? */
		ifiles->fname = stringbuf+1;
		faststr(c, savch);
		stringbuf--;
		savch(0);

		c = skipws(0);
	}
	if (c != '\n')
		goto bad;

	ifiles->lineno = ln;
	prtline(1);
	ifiles->lineno--;
	cunput('\n');
	return;

bad:	error("bad #line");
}

#ifdef MACHOABI

/*
 * Search for framework header file.
 * Return 1 on success.
 */

static int
fsrch_macos_framework(const usch *fn, const usch *dir)
{
	usch *saved_stringbuf = stringbuf;
	usch *s = (usch *)strchr((const char*)fn, '/');
	usch *nm;
	usch *p;
	int len  = s - fn;

	if (s == NULL)
		return 0;

//	fprintf(stderr, "searching for %s in %s\n", (const char *)fn, (const char *)dir);

	nm = savstr(dir);
	savch(0);
	p = savstr(fn);
	stringbuf = p + len;
	savch(0);
//	fprintf(stderr, "comparing \"%s\" against \"%.*s\"\n", nm, len, fn);
	p = (usch *)strstr((const char *)nm, (const char *)p);
//	fprintf(stderr, "p = %s\n", (const char *)p);
	if (p != NULL) {
		stringbuf = p;
		savch(0);
		return fsrch_macos_framework(fn, nm);
	}

	p = nm + strlen((char *)nm) - 1;
	while (*p == '/')
		p--;
	while (*p != '/')
		p--;
	stringbuf = ++p;
	savstr((const usch *)"Frameworks/");
	stringbuf = savstr(fn) + len;
	savstr((const usch*)".framework/Headers");
	savstr(s);
	savch(0);

//	fprintf(stderr, "nm: %s\n", nm);

	if (pushfile(nm, fn, SYSINC, NULL) == 0)
		return 1;
//	fprintf(stderr, "not found %s, continuing...\n", nm);

	stringbuf = saved_stringbuf;

	return 0;
}

#endif

/*
 * Search for and include next file.
 * Return 1 on success.
 */
static int
fsrch(const usch *fn, int idx, struct incs *w)
{
	int i;

	for (i = idx; i < 2; i++) {
		if (i > idx)
			w = incdir[i];
		for (; w; w = w->next) {
			usch *nm = stringbuf;

			savstr(w->dir); savch('/');
			savstr(fn); savch(0);
			if (pushfile(nm, fn, i, w->next) == 0)
				return 1;
			stringbuf = nm;
		}
	}

#ifdef MACHOABI
	/*
	 * On MacOS, we may have to do some clever stuff
	 * to resolve framework headers.
	 */
	{
		usch *dir = stringbuf;
		savstr(ifiles->orgfn);
		stringbuf = (usch *)strrchr((char *)dir, '/');
		if (stringbuf != NULL) {
			stringbuf++;
			savch(0);
			if (fsrch_macos_framework(fn, dir) == 1)
				return 1;
		}
		stringbuf = dir;

		if (fsrch_macos_framework(fn, (const usch *)"/Library/Frameworks/") == 1)
			return 1;

		if (fsrch_macos_framework(fn, (const usch *)"/System/Library/Frameworks/") == 1)
			return 1;
	}
#endif

	return 0;
}

static void
prem(void)
{
	error("premature EOF");
}

static struct iobuf *
incfn(void)
{
	struct iobuf *ob;
	struct symtab *nl;
	usch *sb;
	int c;

	sb = stringbuf;
	if (spechr[c = skipws(NULL)] & C_ID0) {
		heapid(c);
		if ((nl = lookup(sb, FIND)) == NULL)
			return NULL;

		stringbuf = sb;
		if (kfind(nl) == 0)
			return NULL;
		ob = strtobuf(sb, NULL);
	} else {
		ob = getobuf();
		putob(ob, c);
		while ((c = cinput()) && c != '\n')
			putob(ob, c);
		if (c != '\n')
			return NULL;
		cunput(c);
	}
	putob(ob, 0);
	ob->cptr--;

	/* now we have an (expanded?) filename in obuf */
	while (ob->buf < ob->cptr && ISWS(ob->cptr[-1]))
		ob->cptr--;

	if (ob->buf[0] != '\"' && ob->buf[0] != '<')
		return NULL;
	if (ob->cptr[-1] != '\"' && ob->cptr[-1] != '>')
		return NULL;
	ob->cptr[-1] = 0;
	return ob;
}

/*
 * Include a file. Include order:
 * - For <...> files, first search -I directories, then system directories.
 * - For "..." files, first search "current" dir, then as <...> files.
 */
void
include(void)
{
	struct iobuf *ob;
	usch *fn, *nm = NULL;

	if (flslvl)
		return;

	if ((ob = incfn()) == NULL) /* get include file name in obuf */
		error("bad #include");

	fn = xstrdup(ob->buf) + 1;	/* Save on string heap? */
	bufree(ob);
	/* test absolute path first */
	if (fn[0] == '/' && pushfile(fn, fn, 0, NULL) == 0)
		goto okret;
	if (fn[-1] == '\"') {
		/* nope, failed, try to create a path for it */
		if ((nm = (usch *)strrchr((char *)ifiles->orgfn, '/'))) {
			ob = strtobuf((usch *)ifiles->orgfn, NULL);
			ob->cptr = ob->buf + (nm - ifiles->orgfn) + 1;
			strtobuf(fn, ob);
			putob(ob, 0);
			nm = xstrdup(ob->buf);
			bufree(ob);
		} else {
			nm = xstrdup(fn);
		}
		if (pushfile(nm, nm, 0, NULL) == 0) {
			free(fn-1);
			goto okret;
		}
	}
	if (fsrch(fn, 0, incdir[0]))
		goto okret;

	error("cannot find '%s'", fn);
	/* error() do not return */

okret:
	if (nm)
		free(nm);
	prtline(1);
}

void
include_next(void)
{
	struct iobuf *ob;
	usch *nm;

	if (flslvl)
		return;

	if ((ob = incfn()) == NULL) /* get include file name in obuf */
		error("bad #include_next");

	nm = xstrdup(ob->buf+1);
	bufree(ob);

	if (fsrch(nm, ifiles->idx, ifiles->incs) == 0)
		error("cannot find '%s'", nm);
	prtline(1);
}

/*
 * Compare two replacement lists, taking in account comments etc.
 */
static int
cmprepl(const usch *o, const usch *n)
{
	for (; *o; o++, n++) {
		/* comment skip */
		if (*o == '/' && o[1] == '*') {
			while (*o != '*' || o[1] != '/')
				o++;
			o += 2;
		}
		if (*n == '/' && n[1] == '*') {
			while (*n != '*' || n[1] != '/')
				n++;
			n += 2;
		}
		while (*o == ' ' || *o == '\t')
			o++;
		while (*n == ' ' || *n == '\t')
			n++;
		if (*o != *n)
			return 1;
	}
	return 0;
}

static int
isell(void)
{
	if (cinput() != '.' || cinput() != '.')
		return 0;
	return 1;
}

static int
skipwscmnt(struct iobuf *ib)
{
	/* XXX comment */
	return skipws(ib);
}

static int
findarg(usch *s, usch **args, int narg)
{
	int i;

	for (i = 0; i < narg; i++)
		if (strcmp((char *)s, (char *)args[i]) == 0)
			return i;
	return -1;
}

/*
 * gcc extensions:
 * #define e(a...) f(s, a) ->  a works as __VA_ARGS__
 * #define e(fmt, ...) f(s, fmt , ##__VA_ARGS__) -> remove , if no args
 */
void
define(void)
{
	struct symtab *np;
	usch *args[MAXARGS+1], *sbeg, *bp, cc[2], *vararg;
	int c, i, redef, oCflag, t;
	int narg = -1;
	int wascon;

	if (flslvl)
		return;

	oCflag = Cflag, Cflag = 0; /* Ignore comments here */
	if (!ISID0(c = skipws(0)))
		goto bad;

	bp = heapid(c);
	np = lookup(bp, ENTER);
	if (np->value) {
		stringbuf = bp;
		redef = 1;
	} else
		redef = 0;

	vararg = NULL;
	sbeg = stringbuf++;
	if ((c = cinput()) == '(') {
		narg = 0;
		/* function-like macros, deal with identifiers */
		c = skipws(0);
		for (;;) {
			switch (c) {
			case ')':
				break;
			case '.':
				if (isell() == 0 || (c = skipws(0)) != ')')
					goto bad;
				vararg = (usch *)"__VA_ARGS__";
				break;
			default:
				if (!ISID0(c))
					goto bad;

				bp = heapid(c);
				/* make sure there is no arg of same name */
				if (findarg(bp, args, narg) >= 0)
					error("Duplicate parameter \"%s\"", bp);
				if (narg == MAXARGS)
					error("Too many macro args");
				args[narg++] = xstrdup(bp);
				stringbuf = bp;
				switch ((c = skipws(0))) {
				case ',': break;
				case ')': continue;
				case '.':
					if (isell() == 0 || skipws(0) != ')')
						goto bad;
					vararg = args[--narg];
					c = ')';
					continue;
				default:
					goto bad;
				}
				c = skipws(0);
			}
			if (c == ')')
				break;
		}
		c = skipws(0);
	} else if (c == '\n') {
		/* #define foo */
		;
	} else if (c == 0) {
		prem();
	} else if (!ISWS(c))
		goto bad;

	Cflag = oCflag; /* Enable comments again */

	if (vararg)
		stringbuf++;

	if (ISWS(c))
		c = skipwscmnt(0);

#define	DELEWS() while (stringbuf > sbeg+1+(vararg!=NULL) && ISWS(stringbuf[-1])) stringbuf--

	/* parse replacement-list, substituting arguments */
	wascon = 0;
	while (c != '\n') {
		cc[0] = c, cc[1] = inc2();
		t = getyp(cc);
		cunput(cc[1]);

		switch (t) {
		case ' ':
		case '\t':
			savch(' '); /* save only one space */
			while ((c = cinput()) == ' ' || c == '\t')
				;
			continue;

		case '#':
			if (cc[1] == '#') {
				/* concat op */
				(void)cinput(); /* eat # */
				DELEWS();
				savch(CONC);
				if (ISID0(c = skipws(0)) && narg >= 0)
					wascon = 1;
				if (c == '\n')
					goto bad; /* 6.10.3.3 p1 */
				continue;
			}

			if (narg < 0) {
				/* no meaning in object-type macro */
				savch('#');
				break;
			}

			/* remove spaces between # and arg */
			savch(SNUFF);
			c = skipws(0); /* whitespace, ignore */
			if (!ISID0(c))
				goto bad;
			bp = heapid(c);
			if (vararg && strcmp((char *)bp, (char *)vararg) == 0) {
				stringbuf = bp;
				savch(WARN);
				savch(VARG);
				savch(SNUFF);
				break;
				
			}
			if ((i = findarg(bp, args, narg)) < 0)
				goto bad;
			stringbuf = bp;
			savch(WARN);
			savch(i);
			savch(SNUFF);
			break;

		case NUMBER: 
			c = fastnum(c, savch);
			continue;

		case STRING:
			if (c == 'L' || c == 'u' || c == 'U') {
				savch(c);
				if ((c = cinput()) == '8') {
					savch(c);
					c = cinput();
				}
			}
			if (tflag)
				savch(c);
			else
				faststr(c, savch);
			break;

		case IDENT:
			bp = heapid(c);
			stringbuf--; /* remove \0 */
			if (narg < 0)
				break; /* keep on heap */
			if (vararg && strcmp((char *)bp, (char *)vararg) == 0) {
				stringbuf = bp;
				savch(WARN);
				savch(wascon ? GCCARG : VARG);
				break;
			}

			/* check if its an argument */
			if ((i = findarg(bp, args, narg)) < 0)
				break;
			stringbuf = bp;
			savch(WARN);
			savch(i);
			break;

		case 0:
			goto bad;
			
		default:
			savch(c);
			break;
		}
		wascon = 0;
		c = cinput();
	}
	cunput(c);
	/* remove trailing whitespace */
	DELEWS();

	if (sbeg[1+(vararg != 0)] == CONC)
		goto bad; /* 6.10.3.3 p1 */

	if (vararg) {
		sbeg[0] = VARG;
		sbeg[1] = narg;
	} else
		sbeg[0] = (narg < 0 ? OBJCT : narg);
	savch(0);

	if (redef && ifiles->idx != SYSINC) {
		if (cmprepl(np->value, sbeg)) { /* not equal */
			np->value = sbeg;
			warning("%s redefined (previously defined at \"%s\" line %d)",
			    np->namep, np->file, np->line);
		} else
			stringbuf = sbeg;  /* forget this space */
	} else
		np->value = sbeg;

#ifdef PCC_DEBUG
	if (dflag) {
		const usch *w = np->value;

		printf("!define %s: ", np->namep);
		if (*w == OBJCT)
			printf("[object]");
		else if (*w == VARG)
			printf("[VARG%d]", *++w);
		else
			printf("[%d]", *w);
		putchar('\'');
		prrep(++w);
		printf("\'\n");
	}
#endif
	for (i = 0; i < narg; i++)
		free(args[i]);
	return;

bad:	error("bad #define");
}

void
warning(const char *fmt, ...)
{
	va_list ap;

	if (ifiles != NULL)
		fprintf(stderr, "%s:%d: warning: ",
		    ifiles->fname, ifiles->lineno);

	va_start(ap,fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);

	warnings++;
}

void
error(const char *fmt, ...)
{
	va_list ap;

	if (ifiles != NULL)
		fprintf(stderr, "%s:%d: error: ",
		    ifiles->fname, ifiles->lineno);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

/*
 * store a character into the "define" buffer.
 */
void
savch(int c)
{
	if (stringbuf >= &sbf[SBSIZE])
		error("out of macro space!");

	*stringbuf++ = (usch)c;
}

static int
pragwin(struct iobuf *ib)
{
	return ib ? *ib->cptr++ : cinput();
}

static int
skipws(struct iobuf *ib)
{
	int t;

	while ((t = pragwin(ib)) == ' ' || t == '\t')
		;
	return t;
}

/*
 * convert _Pragma() to #pragma for output.
 * Syntax is already correct.
 */
static void
pragoper(struct iobuf *ib)
{
	int t;
	usch *bp = stringbuf;

	if (skipws(ib) != '(' || ((t = skipws(ib)) != '\"' && t != 'L'))
		goto err;
	if (t == 'L' && (t = pragwin(ib)) != '\"')
		goto err;
	savstr((const usch *)"\n#pragma ");
	while ((t = pragwin(ib)) != '\"') {
		if (t == BLKID) {
			pragwin(ib);
			continue;
		}
		if (t == '\"')
			continue;
		if (t == '\\') {
			if ((t = pragwin(ib)) != '\"' && t != '\\')
				savch('\\');
		}
		savch(t);
	}
	sheap("\n# %d \"%s\"\n", ifiles->lineno, ifiles->fname);
	putstr(bp);
	stringbuf = bp;
	if (skipws(ib) == ')')
		return;

err:	error("_Pragma() syntax error");
}

static int
expok(struct symtab *sp, int l)
{
	struct blocker *w;

	if (l == 0)
		return 1;
#ifdef PCC_DEBUG
if (dflag) { printf("expok blocked: "); for (w = blkidx[l]; w; w = w->next) printf("%s ", w->sp->namep); printf("\n"); }
#endif
	w = blkidx[l];
	while (w) {
		if (w->sp == sp)
			return 0;
		w = w->next;
	}
	return 1;
}

static int
expokb(struct symtab *sp, struct blocker *bl)
{
	struct blocker *w;

	if (bl == 0)
		return 1;
#ifdef PCC_DEBUG
if (dflag) { printf("expokb blocked: "); for (w = bl; w; w = w->next) printf("%s ", w->sp->namep); printf("\n"); }
#endif
	w = bl;
	while (w) {
		if (w->sp == sp)
			return 0;
		w = w->next;
	}
	return 1;
}

static struct blocker *
blkget(struct symtab *sp, struct blocker *obl)
{
	struct blocker *bl = calloc(sizeof(*obl), 1);

	bl->sp = sp;
	bl->next = obl;
	return bl;
}

static int
blkix(struct blocker *obl)
{
	if (blkidp > 1 && blkidx[blkidp-1] == obl)
		return blkidp-1;
	if (blkidp == RECMAX)
		error("blkix");
	blkidx[blkidp] = obl;
	return blkidp++;
}

static struct blocker *
mergeadd(struct blocker *bl, int m)
{
	struct blocker *w, *ww;

	DPRINT(("mergeadd: %p %d\n", bl, m));
	if (bl == 0)
		return blkidx[m];
	if (m == 0)
		return bl;

	blkidx[blkidp] = bl;
	for (w = blkidx[m]; w; w = w->next) {
		ww = calloc(sizeof(*w), 1);
		ww->sp = w->sp;
		ww->next = blkidx[blkidp];
		blkidx[blkidp] = ww;
	}
	DPRINT(("mergeadd return: %d ", blkidp));
#ifdef PCC_DEBUG
	if (dflag) {
		for (w = blkidx[blkidp]; w; w = w->next)
			printf("%s ", w->sp->namep);
		printf("\n");
	}
#endif
	return blkidx[blkidp++];
}

static void
storeblk(int l, struct iobuf *ob)
{
	DPRINT(("storeblk: %d\n", l));
	putob(ob, BLKID);
	putob(ob, l);
}

/*
 * Save filename on heap (with escaped chars).
 */
static usch *
unfname(void)
{
	usch *sb = stringbuf;
	const usch *bp = ifiles->fname;

	savch('\"');
	for (; *bp; bp++) {
		if (*bp == '\"' || *bp == '\'' || *bp == '\\')
			savch('\\');
		savch(*bp);
	}
	savch('\"');
	*stringbuf = 0;
	return sb;
}

/*
 * Version of fastnum that reads from a string and saves in ob.
 * We know that it is a number before calling this routine.
 */
static usch *
fstrnum(usch *s, struct iobuf *ob)  
{	
	if (*s == '.') {
		/* not digit, dot.  Next will be digit */
		putob(ob, *s++);
	}
	for (;;) {
		putob(ob, *s++);
		if ((spechr[*s] & C_EP)) {
			if (s[1] != '-' && s[1] != '+')
				break;
			putob(ob, *s++);
		} else if ((*s != '.') && ((spechr[*s] & C_ID) == 0))
			break;
	}
        return s;
}

/*
 * get a string or character constant.
 * similar to faststr.
 */
static usch *
fstrstr(usch *s, struct iobuf *ob)
{
	int ch;

	if (*s == 'L' || *s == 'U' || *s == 'u')
		putob(ob, *s++);
	if (*s == '8')
		putob(ob, *s++);
	ch = *s;
	putob(ob, *s++);
	while (*s != ch) {
		if (*s == '\\')
			putob(ob, *s++);
		putob(ob, *s++);
	}
	putob(ob, *s++);
	return s;
}

/*
 * Save standard comments if found.
 */
static usch *
fcmnt(usch *s, struct iobuf *ob)
{
	putob(ob, *s++); /* / */
	putob(ob, *s++); /* * */
	for (;;s++) {
		putob(ob, *s);
		if (s[-1] == '*' && *s == '/')
			break;
	}
	return s+1;
}

static int
getyp(usch *s)
{

	if (ISID0(*s)) return IDENT;
	if ((*s == 'L' || *s == 'U' || *s == 'u') &&
	    (s[1] == '\'' || s[1] == '\"')) return STRING;
	if (s[0] == 'u' && s[1] == 'U' && s[2] == '\"') return STRING;
	if (s[0] == '\'' || s[0] == '\"') return STRING;
	if (spechr[*s] & C_DIGIT) return NUMBER;
	if (*s == '.' && (spechr[s[1]] & C_DIGIT)) return NUMBER;
	if (*s == '/' && (s[1] == '/' || s[1] == '*')) return CMNT;
	return *s;
	
}

/*
 * Check ib and print out the symbols there.
 * If expandable symbols found recurse and expand them.
 * If last identifier on the input list is expandable return it.
 * Expect ib to be zero-terminated.
 */
static struct symtab *
loopover(struct iobuf *ib)
{
	struct iobuf *xb, *xob;
	struct symtab *sp;
	usch *cp;
	int l, c, t;

	ib->cptr = ib->buf; /* start from beginning */
#ifdef PCC_DEBUG
	if (dflag) {
		printf("loopover: '");
		prline(ib->cptr);
		printf("'\n");
	}
#endif

	xb = getobuf();
	while ((c = *ib->cptr)) {
		switch (t = getyp(ib->cptr)) {
		case CMNT:
			xb->cptr = xb->buf;
			ib->cptr = fcmnt(ib->cptr, xb);
			*xb->cptr = 0;
			savstr(xb->buf);
			continue;
		case NUMBER:
			xb->cptr = xb->buf;
			ib->cptr = fstrnum(ib->cptr, xb);
			*xb->cptr = 0;
			savstr(xb->buf);
			continue;
		case STRING:
			xb->cptr = xb->buf;
			ib->cptr = fstrstr(ib->cptr,xb);
			*xb->cptr = 0;
			for (cp = xb->buf; *cp; cp++) {
				if (*cp <= BLKID) {
					if (*cp == BLKID)
						cp++;
					continue;
				}
				savch(*cp);
			}
			continue;
		case BLKID:
			l = ib->cptr[1];
			ib->cptr+=2;
			/* FALLTHROUGH */
		case IDENT:
			if (t != BLKID)
				l = 0;
			/*
			 * Tricky: if this is the last identifier
			 * in the expanded list, and it is defined
			 * as a function-like macro, then push it
			 * back on the input stream and let fastscan
			 * handle it as a new macro.
			 * BUT: if this macro is blocked then this
			 * should not be done.
			 */
			for (cp = ib->cptr; ISID(*ib->cptr); ib->cptr++)
				;
			if ((sp = lookup(cp, FIND)) == NULL) {
sstr:				for (; cp < ib->cptr; cp++)
					savch(*cp);
				continue;
			}
			if (expok(sp, l) == 0) {
				/* blocked */
				goto sstr;
			} else {
				if (*sp->value != OBJCT) {
					cp = ib->cptr;
					while (ISWS(*ib->cptr))
						ib->cptr++;
					if (*ib->cptr == 0) {
						bufree(xb);
						return sp;
					}
					ib->cptr = cp;
				}
newmac:				if ((xob = submac(sp, 1, ib, NULL)) == NULL) {
					savstr(sp->namep);
				} else {
					sp = loopover(xob);
					bufree(xob);
					if (sp != NULL)
						goto newmac;
				}
			}
			continue;
		default:
			savch(c);
		}

		ib->cptr++;
	}

	bufree(xb);
	DPRINT(("loopover return 0\n"));
	return 0;
}

/*
 * Handle defined macro keywords found on input stream.
 * When finished print out the full expanded line.
 * Input here is from the lex buffer.
 * Return 1 if success, 0 otherwise.  fastscan restores stringbuf.
 * Scanned data is stored on heap.  Last scan prints out the buffer.
 */
int
kfind(struct symtab *sp)
{
	extern int inexpr;
	struct blocker *bl;
	struct iobuf *ib, *ob;
	const usch *argary[MAXARGS+1], *sbp;
	int c, n = 0;

	blkidp = 1;
	sbp = stringbuf;
	DPRINT(("%d:enter kfind(%s)\n",0,sp->namep));
	switch (*sp->value) {
	case FILLOC:
		unfname();
		return 1;

	case LINLOC:
		sheap("%d", ifiles->lineno);
		return 1;

	case PRAGLOC:
		pragoper(NULL);
		return 1;

	case DEFLOC:
	case OBJCT:
		bl = blkget(sp, NULL);
		ib = mkrobuf(sp->value+1);
		ob = getobuf();
		ob = exparg(1, ib, ob, bl);
		bufree(ib);
		break;

	case CTRLOC:
		sheap("%d", counter++);
		return 1;

	default:
		/* Search for '(' */
		while (ISWSNL(c = cinput()))
			if (c == '\n')
				n++;
		if (c != '(') {
			if (inexpr == 0)
				putstr(sp->namep);
			if (n == 0)
				putch(' ');
			else for (ifiles->lineno += n; n; n--)
				putch('\n');
			cunput(c);
			return 0; /* Failed */
		}

		/* fetch arguments */
again:		if (readargs1(sp, argary))
			error("readargs");

		bl = blkget(sp, NULL);
		ib = subarg(sp, argary, 1, bl);
		ob = getobuf();
		ob = exparg(1, ib, ob, bl);
		bufree(ib);
		break;
	}

	/*
	 * Loop over stringbuf, output the data and remove remaining  
	 * directives.  Start with extracting the last keyword (if any).
	 */
	putob(ob, 0); /* XXX needed? */

	stringbuf = (usch *)sbp; /* XXX should check cleanup */
	if ((sp = loopover(ob))) {
		/* Search for '(' */
		while (ISWSNL(c = cinput()))
			if (c == '\n')
				n++;
		if (c == '(') {
			bufree(ob);
			goto again;
		}
		cunput(c);
		savstr(sp->namep);
	}
	bufree(ob);

	for (ifiles->lineno += n; n; n--)
		savch('\n');
	savch(0);
	stringbuf = (usch *)sbp;
	if (nbufused)
		error("lost buffer");
	return 1;
}

/*
 * Replace and push-back on input stream the eventual replaced macro.
 * The check for whether it can expand or not should already have been done.
 * Blocks for this identifier will be added via insblock() after expansion.
 * The same as kfind but read a string.
 */
struct iobuf *
submac(struct symtab *sp, int lvl, struct iobuf *ib, struct blocker *obl)
{
	struct blocker *bl;
	struct iobuf *ob;
	const usch *argary[MAXARGS+1];
	usch *cp, *pr;

	DPRINT(("%d:submac: trying '%s'\n", lvl, sp->namep));
	switch (*sp->value) {
	case FILLOC:
		ob = strtobuf(unfname(), NULL);
		break;
	case LINLOC:
		ob = strtobuf(sheap("%d", ifiles->lineno), NULL);
		break;
	case PRAGLOC:
		pragoper(ib);
		ob = strtobuf((usch *)"", NULL);
		break;
	case OBJCT:
		bl = blkget(sp, obl);
		ib = mkrobuf(sp->value+1);
		ob = getobuf();
		DPRINT(("%d:submac: calling exparg\n", lvl));
		ob = exparg(lvl+1, ib, ob, bl);
		bufree(ib);
		DPRINT(("%d:submac: return exparg\n", lvl));
		break;
	case CTRLOC:
		ob = strtobuf(sheap("%d", counter++), NULL);
		break;
	default:
		cp = ib->cptr;
		while (ISWSNL(*ib->cptr))
			ib->cptr++;
		if (*ib->cptr != '(') {
			ib->cptr = cp;
			return 0;
		}
		cp = ib->cptr++;
		pr = stringbuf;
		if (readargs2(&ib->cptr, sp, argary)) {
			/* Bailed out in the middle of arg list */
			ib->cptr = cp; /* XXX */
			return 0;
		}
		bl = blkget(sp, obl);
		ib = subarg(sp, argary, lvl+1, bl);
		stringbuf = pr;

		ob = getobuf();
		DPRINT(("%d:submac(: calling exparg\n", lvl));
		ob = exparg(lvl+1, ib, ob, bl);
		bufree(ib);
		DPRINT(("%d:submac(: return exparg\n", lvl));
		break;
	}
	putob(ob, 0);
	ob->cptr--;

	return ob;
}

static int
isdir(void)
{
	usch ch;

	while ((ch = cinput()) == ' ' || ch == '\t')
		;
	if (ch == '#')
		return 1;
	cunput(ch);
	return 0;
}

/*
 * Deal with directives inside a macro.
 * Doing so is really ugly but gcc allows it, so...
 */
static void
chkdir(void)
{
	usch ch;

	for (;;) {
		if (isdir()) {
#ifndef GCC_COMPAT
			warning("conditionals inside macro arg list");
#endif
			ppdir();
		}
		if (flslvl == 0)
			return;
		while ((ch = cinput()) != '\n')
			;
		ifiles->lineno++;
		putch('\n');
	}
}

static int
ra1_wsnl(int sp)
{
	int c;

	while (ISWSNL(c = cinput())) {
		if (c == '\n') {
			putch('\n');
			chkdir();
			ifiles->lineno++;
			if (sp) savch(' ');
		}
	}
	return c;
}

/*
 * Read arguments and put in argument array.
 * If EOF is encountered return 1, otherwise 0.
 */
int
readargs1(struct symtab *sp, const usch **args)
{
	const usch *vp = sp->value;
	int c, i, plev, narg, ellips = 0;

	DPRINT(("readargs1\n"));
	narg = *vp++;
	if (narg == VARG) {
		narg = *vp++;
		ellips = 1;
	}
#ifdef PCC_DEBUG
	if (dflag > 1) {
		printf("narg %d varg %d: ", narg, ellips);
		prrep(vp);
		printf("\n");
	}
#endif

	/*
	 * read arguments and store them on heap.
	 */
	c = '(';
	for (i = 0; i < narg && c != ')'; i++) {
		args[i] = stringbuf;
		plev = 0;

		c = ra1_wsnl(0);
		for (;;) {
			if (plev == 0 && (c == ')' || c == ','))
				break;
			if (c == '(') plev++;
			if (c == ')') plev--;
			if (c == 0)
				error("eof in macro");
			else if (c == '/') Ccmnt(savch);
			else if (c == '\"' || c == '\'') faststr(c, savch);
			else if (ISID0(c)) {
				usch *bp = stringbuf;
				do {
					savch(c);
				} while ((spechr[c = cinput()] & C_ID));
				if ((sp = lookup(bp, FIND)) != NULL) {
					if (sp == linloc) {
						stringbuf = bp;
						sheap("%d", ifiles->lineno);
					} else if (sp == ctrloc) {
						stringbuf = bp;
						sheap("%d", counter++);
					}
				}
				cunput(c);
			} else
				savch(c);
			if ((c = cinput()) == '\n') {
				chkdir();
				ifiles->lineno++, putch(c), c = ' ';
			}
		}

		while (args[i] < stringbuf && ISWSNL(stringbuf[-1]))
			stringbuf--;
		savch('\0');
#ifdef PCC_DEBUG
		if (dflag) {
			printf("readargs: save arg %d '", i);
			prline(args[i]);
			printf("'\n");
		}
#endif
	}

	/* Handle varargs readin */
	if (ellips)
		args[i] = (const usch *)"";
	if (ellips && c != ')') {
		args[i] = stringbuf;
		plev = 0;
		c = ra1_wsnl(0);
		for (;;) {
			if (plev == 0 && c == ')')
				break;
			if (c == '(') plev++;
			if (c == ')') plev--;
			if (c == '\"' || c == '\'') faststr(c, savch);
			else
				savch(c);
			if ((c = cinput()) == '\n')
				ifiles->lineno++, c = ' ';
		}
		while (args[i] < stringbuf && ISWSNL(stringbuf[-1]))
			stringbuf--;
		savch('\0');
#ifdef PCC_DEBUG
		if (dflag) {
			printf("readargs: vararg arg %d '", i);
			prline(args[i]);
			printf("'\n");
		}
#endif

	}
	if (narg == 0 && ellips == 0)
		c = ra1_wsnl(0);

	if (c != ')' || (i != narg && ellips == 0) || (i < narg && ellips == 1))
		error("wrong arg count");
	return 0;
}

static usch *raptr;
static int
raread(void)
{
	int rv;

	if (raptr) {
		if ((rv = *raptr))
			raptr++;
	} else
		rv = cinput();
	return rv;
}


/*
 * Read arguments and put in argument array.
 * If EOF is encountered return 1, otherwise 0.
 */
int
readargs2(usch **inp, struct symtab *sp, const usch **args)
{
	const usch *vp = sp->value;
	usch *bp;
	int c, i, plev, narg, ellips = 0;

	DPRINT(("readargs2 %s '", sp->namep));
#ifdef PCC_DEBUG
	if (dflag && inp) {
		prline(*inp);
		printf("'\n");
	}
#endif
	raptr = inp ? *inp : 0;
	narg = *vp++;
	if (narg == VARG) {
		narg = *vp++;
		ellips = 1;
	}
#ifdef PCC_DEBUG
	if (dflag > 1) {
		prrep(vp);
		printf("\n");
	}
#endif


	/*
	 * read arguments and store them on heap.
	 */
	c = '(';
	for (i = 0; i < narg && c != ')'; i++) {
		args[i] = stringbuf;
		plev = 0;

		while ((c = raread()) == ' ' || c == '\t')
			;
		for (;;) {
			if (plev == 0 && (c == ')' || c == ','))
				break;
			if (c == '(') plev++;
			if (c == ')') plev--;
			if (c == 0) {
				if (raptr) {
					*inp = raptr;
					raptr = 0;
				} else
					error("eof in macro");
			} else if (c == BLKID) {
				savch(c), savch(raread());
			} else if (c == '/') {
				if ((c = raread()) == '*')
					error("FIXME ccmnt");
				savch('/');
				continue;
			} else if (c == '\"' || c == '\'') {
				if (raptr) {
					struct iobuf *xob = getobuf();
					raptr = fstrstr(raptr-1, xob);
					*xob->cptr = 0;
					savstr(xob->buf);
					bufree(xob);
				} else
					faststr(c, savch);
			} else if (ISID0(c)) {
				bp = stringbuf;
				do {
					savch(c);
				} while (ISID(c = raread()));
				*stringbuf = 0;
				if ((sp = lookup(bp, FIND)) && (sp == linloc)) {
					stringbuf = bp;
					sheap("%d", ifiles->lineno);
				}
				continue;
			} else
				savch(c);
			c = raread();
		}

		while (args[i] < stringbuf && ISWSNL(stringbuf[-1]))
			stringbuf--;
		savch('\0');
#ifdef PCC_DEBUG
		if (dflag) {
			printf("readargs2: save arg %d '", i);
			prline(args[i]);
			printf("'\n");
		}
#endif
	}

	/* Handle varargs readin */
	if (ellips)
		args[i] = (const usch *)"";
	if (ellips && c != ')') {
		args[i] = stringbuf;
		plev = 0;
		while ((c = raread()) == ' ' || c == '\t')
			;
		for (;;) {
			if (plev == 0 && c == ')')
				break;
			if (c == '(') plev++;
			if (c == ')') plev--;
			if (c == '\"' || c == '\'') {
				if (raptr) {
					struct iobuf *xob = getobuf();
					raptr = fstrstr(raptr-1, xob);
					*xob->cptr = 0;
					savstr(xob->buf);
					bufree(xob);
				} else
					faststr(c, savch);
			} else
				savch(c);
			c = raread();
		}
		while (args[i] < stringbuf && ISWSNL(stringbuf[-1]))
			stringbuf--;
		savch('\0');

	}
	if (narg == 0 && ellips == 0) {
		while ((c = raread()) == ' ' || c == '\t')
			;
	}

	if (c != ')' || (i != narg && ellips == 0) || (i < narg && ellips == 1))
		error("wrong arg count");
	if (raptr)
		*inp = raptr;
	return 0;
}

/*
 * expand a function-like macro.
 * vp points to end of replacement-list
 * reads function arguments from input stream.
 * result is pushed-back for more scanning.
 */
struct iobuf *
subarg(struct symtab *nl, const usch **args, int lvl, struct blocker *bl)
{
	struct blocker *w;
	struct iobuf *ob, *cb, *nb;
	int narg, instr, snuff;
	const usch *sp, *bp, *ap, *vp, *tp;

	DPRINT(("%d:subarg '%s'\n", lvl, nl->namep));
	ob = getobuf();
	vp = nl->value;
	narg = *vp++;
	if (narg == VARG)
		narg = *vp++;

	sp = vp;
	instr = snuff = 0;
#ifdef PCC_DEBUG
	if (dflag>1) {
		printf("%d:subarg ARGlist for %s: '", lvl, nl->namep);
		prrep(vp);
		printf("' ");
		for (w = bl; w; w = w->next)
			printf("%s ", w->sp->namep);
		printf("\n");
	}
#endif

	/*
	 * walk forward over replacement-list while replacing
	 * arguments.  Arguments are macro-expanded if required.
	 */
	while (*sp) {
		if (*sp == SNUFF)
			putob(ob, '\"'), snuff ^= 1;
		else if (*sp == CONC)
			;
		else if (*sp == WARN) {

			if (sp[1] == VARG) {
				bp = ap = args[narg];
				sp++;
#ifdef GCC_COMPAT
			} else if (sp[1] == GCCARG) {
				/* XXX remove last , not add 0 */
				ap = args[narg];
				if (ap[0] == 0)
					ap = (const usch *)"0";
				bp = ap;
				sp++;
#endif
			} else
				bp = ap = args[(int)*++sp];
#ifdef PCC_DEBUG
			if (dflag>1){
				printf("%d:subarg GOTwarn; arglist '", lvl);
				prline(bp);
				printf("'\n");
			}
#endif
			if (sp[-2] != CONC && !snuff && sp[1] != CONC) {
				/*
				 * Expand an argument; 6.10.3.1:
				 * "A parameter in the replacement list,
				 *  is replaced by the corresponding argument
				 *  after all macros contained therein have
				 *  been expanded.".
				 */
				w = bl ? bl->next : NULL;
				cb = mkrobuf(bp);
				nb = getobuf();
				DPRINT(("%d:subarg: calling exparg\n", lvl));
				nb = exparg(lvl+1, cb, nb, w);
				DPRINT(("%d:subarg: return exparg\n", lvl));
				bufree(cb);
				strtobuf(nb->buf, ob);
				bufree(nb);
			} else {
				while (*bp) {
					if (snuff && !instr && ISWS(*bp)) {
						while (ISWS(*bp))
							bp++;
						putob(ob, ' ');
					}

					if (snuff &&
					    (*bp == '\'' || *bp == '"')) {
						instr ^= 1;
						for (tp = bp - 1; *tp == '\\'; tp--)
							instr ^= 1;
						if (*bp == '"')
							putob(ob, '\\');
					} 
					if (snuff && instr && *bp == '\\')
						putob(ob, '\\');
					putob(ob, *bp);
					bp++;
				}
			}
		} else if (ISID0(*sp)) {
			if (lookup(sp, FIND))
				storeblk(blkix(bl), ob);
			while (ISID(*sp))
				putob(ob, *sp++);
			sp--;
		} else
			putob(ob, *sp);
		sp++;
	}
	putob(ob, 0);
	ob->cptr = ob->buf;
	DPRINT(("%d:subarg retline %s\n", lvl, ob->buf));
	return ob;
}

/*
 * Do a (correct) expansion of a WARN-terminated buffer of tokens.
 * Data is read from the lex buffer, result on lex buffer, WARN-terminated.
 * Expansion blocking is not altered here unless when tokens are
 * concatenated, in which case they are removed.
 */
struct iobuf *
exparg(int lvl, struct iobuf *ib, struct iobuf *ob, struct blocker *bl)
{
	extern int inexpr;
	struct iobuf *nob;
	struct symtab *nl;
	int c, m;
	usch *cp, *bp, *sbp;

	DPRINT(("%d:exparg: entry ib %s\n", lvl, ib->cptr));
#ifdef PCC_DEBUG
	if (dflag > 1) {
		printf("exparg entry: full ");
		prline(ib->cptr);
		printf("\n");
	}
#endif

	while ((c = getyp(ib->cptr)) != 0) {
		ib->cptr++;

		switch (c) {

		case CMNT:
			ib->cptr = fcmnt(ib->cptr-1, ob);
			break;
		case NUMBER:
			ib->cptr = fstrnum(ib->cptr-1, ob);
			break;
		case STRING:
			ib->cptr = fstrstr(ib->cptr-1, ob);
			break;
		case BLKID:
			m = *ib->cptr++;
			ib->cptr++;
			/* FALLTHROUGH */
		case IDENT:
			if (c != BLKID)
				m = 0;
			for (cp = ib->cptr-1; ISID(*cp); cp++)
				;
#ifdef PCC_DEBUG
if (dflag) { printf("!! ident "); prline(ib->cptr-1); printf("\n"); }
#endif
			sbp = stringbuf;
			if (*cp == BLKID) {
				/* concatenation */
				bp = stringbuf;
				for (cp = ib->cptr-1; 
				    ISID(*cp) || *cp == BLKID; cp++) {
					if (*cp == BLKID) {
						/* XXX add to block list */
						cp++;
					} else
						savch(*cp);
				}
				ib->cptr = cp;
				cp = stringbuf;
				savch(0);
			} else {
				bp = ib->cptr-1;
				ib->cptr = cp;
			}
#ifdef PCC_DEBUG
if (dflag) { printf("!! ident2 "); prline(bp); printf("\n"); }
#endif
			if ((nl = lookup(bp, FIND)) == NULL) {
sstr:				for (; bp < cp; bp++)
					putob(ob, *bp);
				stringbuf = sbp;
				break;
			} else if (inexpr && *nl->value == DEFLOC) {
				int gotlp = 0;
				while (ISWS(*ib->cptr)) ib->cptr++;
				if (*ib->cptr == '(')
					gotlp++, ib->cptr++;
				while (ISWS(*ib->cptr)) ib->cptr++;
				if (!ISID0(*ib->cptr))
					error("bad defined");
				putob(ob, lookup(ib->cptr, FIND) ? '1' : '0');
				while (ISID(*ib->cptr)) ib->cptr++;
				while (ISWS(*ib->cptr)) ib->cptr++;
				if (gotlp && *ib->cptr != ')')
					error("bad defined");
				ib->cptr++;
				break;
			}
			stringbuf = sbp;
			if (expokb(nl, bl) && expok(nl, m)) {
				if ((nob = submac(nl, lvl+1, ib, bl))) {
					if (nob->buf[0] == '-' ||
					    nob->buf[0] == '+')
						putob(ob, ' ');
					strtobuf(nob->buf, ob);
					if (ob->cptr[-1] == '-' ||
					    ob->cptr[-1] == '+')
						putob(ob, ' ');
					bufree(nob);
				} else {
					goto sblk;
				}
			} else {
				/* blocked */
sblk:				storeblk(blkix(mergeadd(bl, m)), ob);
				goto sstr;
			}
			break;

		default:
			putob(ob, c);
			break;
		}
	}
	putob(ob, 0);
	ob->cptr--;
	DPRINT(("%d:exparg return: ob %s\n", lvl, ob->buf));
#ifdef PCC_DEBUG
	if (dflag > 1) {
		printf("%d:exparg: full ", lvl);
		prline(ob->buf);
		printf("\n");
	}
#endif
	return ob;
}

#ifdef PCC_DEBUG

static void
prrep(const usch *s)
{
	while (*s) {
		switch (*s) {
		case WARN:
			if (s[1] == VARG) printf("<VARG>");
			else if (s[1] == GCCARG) printf("<GCCARG>");
			else printf("<ARG(%d)>", s[1]);
			s++;
			break;
		case CONC: printf("<CONC>"); break;
		case SNUFF: printf("<SNUFF>"); break;
		case BLKID: printf("<BLKID(%d)>",s[1]); s++; break;
		default: printf("%c", *s); break;
		}
		s++;
	}
}

static void
prline(const usch *s)
{
	while (*s) {
		switch (*s) {
		case BLKID: printf("<BLKID(%d)>", *++s); break;
		case WARN: printf("<WARN>"); break;
		case CONC: printf("<CONC>"); break;
		case SNUFF: printf("<SNUFF>"); break;
		case '\n': printf("<NL>"); break;
		default: 
			if (*s > 0x7f)
				printf("<0x%x>", *s);
			else
				printf("%c", *s);
			break;
		}
		s++;
	}
}
#endif

usch *
savstr(const usch *str)
{
	usch *rv = stringbuf;

	do {
		if (stringbuf >= &sbf[SBSIZE])
			error("out of macro space!");
	} while ((*stringbuf++ = *str++));
	stringbuf--;
	return rv;
}

void
putch(int ch)
{
	if (Mflag)
		return;
	fputc(ch, stdout);
}

void
putstr(const usch *s)
{
	for (; *s; s++) {
		if (Mflag == 0)
			fputc(*s, stdout);
	}
}

/*
 * convert a number to an ascii string. Store it on the heap.
 */
static void
num2str(int num)
{
	static usch buf[12];
	usch *b = buf;
	int m = 0;

	if (num < 0)
		num = -num, m = 1;
	do {
		*b++ = (usch)(num % 10 + '0');
		num /= 10;
	} while (num);
	if (m)
		*b++ = '-';
	while (b > buf)
		savch(*--b);
}

/*
 * similar to sprintf, but only handles %c, %s and %d.
 * saves result on heap.
 */
static void
vsheap(const char *fmt, va_list ap)
{
	for (; *fmt; fmt++) {
		if (*fmt == '%') {
			fmt++;
			switch (*fmt) {
			case 's':
				savstr(va_arg(ap, usch *));
				break;
			case 'd':
				num2str(va_arg(ap, int));
				break;
			case 'c':
				savch(va_arg(ap, int));
				break;
			default:
				error("bad sheap");
			}
		} else
			savch(*fmt);
	}
	*stringbuf = 0;
}

usch *
sheap(const char *fmt, ...)
{
	va_list ap;
	usch *op = stringbuf;

	va_start(ap, fmt);
	vsheap(fmt, ap);
	va_end(ap);

	return op;
}

static void
usage(void)
{
	error("Usage: cpp [-Cdt] [-Dvar=val] [-Uvar] [-Ipath] [-Spath]");
}

#ifdef notyet
/*
 * Symbol table stuff.
 * The data structure used is a patricia tree implementation using only
 * bytes to store offsets.
 * The information stored is (lower address to higher):
 *
 *	unsigned char bitno[2]; bit number in the string
 *	unsigned char left[3];	offset from base to left element
 *	unsigned char right[3];	offset from base to right element
 */
#endif

/*
 * This patricia implementation is more-or-less the same as
 * used in ccom for string matching.
 */
struct tree {
	int bitno;
	struct tree *lr[2];
};

#define BITNO(x)		((x) & ~(LEFT_IS_LEAF|RIGHT_IS_LEAF))
#define LEFT_IS_LEAF		0x80000000
#define RIGHT_IS_LEAF		0x40000000
#define IS_LEFT_LEAF(x)		(((x) & LEFT_IS_LEAF) != 0)
#define IS_RIGHT_LEAF(x)	(((x) & RIGHT_IS_LEAF) != 0)
#define P_BIT(key, bit)		(key[bit >> 3] >> (bit & 7)) & 1
#define CHECKBITS		8

static struct tree *sympole;
static int numsyms;

static struct tree *
gtree(void)
{
	static int ntrees;
	static struct tree *tp;

	if (ntrees == 0) {
		tp = xmalloc(BUFSIZ);
		ntrees = BUFSIZ/sizeof(*tp);
	}
	return &tp[--ntrees];
}

/*
 * Allocate a symtab struct and store the string.
 */
static struct symtab *
getsymtab(const usch *str)
{
	static int nsyms;
	static struct symtab *spp;
	struct symtab *sp;

	if (nsyms == 0) {
		spp = xmalloc(BUFSIZ);
		nsyms = BUFSIZ/sizeof(*sp);
	}
	sp = &spp[--nsyms];

	sp->namep = str;
	sp->value = NULL;
	sp->file = ifiles ? ifiles->orgfn : (const usch *)"<initial>";
	sp->line = ifiles ? ifiles->lineno : 0;
	return sp;
}

/*
 * Do symbol lookup in a patricia tree.
 * Only do full string matching, no pointer optimisations.
 */
struct symtab *
lookup(const usch *key, int enterf)
{
	struct symtab *sp;
	struct tree *w, *new, *last;
	int len, cix, bit, fbit, svbit, ix, bitno;
	const usch *k, *m;

	/* Count full string length */
	for (k = key, len = 0; ISID(*k) & C_ID; k++, len++)
		;

	switch (numsyms) {
	case 0: /* no symbols yet */
		if (enterf != ENTER)
			return NULL;
		sympole = (struct tree *)getsymtab(key);
		numsyms++;
		return (struct symtab *)sympole;

	case 1:
		w = sympole;
		svbit = 0; /* XXX gcc */
		break;

	default:
		w = sympole;
		bitno = len * CHECKBITS;
		for (;;) {
			bit = BITNO(w->bitno);
			fbit = bit >= bitno ? 0 : P_BIT(key, bit);
			svbit = fbit ? IS_RIGHT_LEAF(w->bitno) :
			    IS_LEFT_LEAF(w->bitno);
			w = w->lr[fbit];
			if (svbit)
				break;
		}
	}

	sp = (struct symtab *)w;

	m = sp->namep;
	k = key;

	/* Check for correct string and return */
	for (cix = 0; *m && ISID(*k) && *m == *k; m++, k++, cix += CHECKBITS)
		;
	if (*m == 0 && ISID(*k) == 0) {
		if (enterf != ENTER && sp->value == NULL)
			return NULL;
		return sp;
	}

	if (enterf != ENTER)
		return NULL; /* no string found and do not enter */

	ix = *m ^ *k;
	while ((ix & 1) == 0)
		ix >>= 1, cix++;

	/* Create new node */
	new = gtree();
	bit = P_BIT(key, cix);
	new->bitno = cix | (bit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	new->lr[bit] = (struct tree *)getsymtab(key);

	if (numsyms++ == 1) {
		new->lr[!bit] = sympole;
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
		sympole = new;
		return (struct symtab *)new->lr[bit];
	}

	w = sympole;
	last = NULL;
	for (;;) {
		fbit = w->bitno;
		bitno = BITNO(w->bitno);
		if (bitno == cix)
			error("bitno == cix");
		if (bitno > cix)
			break;
		svbit = P_BIT(key, bitno);
		last = w;
		w = w->lr[svbit];
		if (fbit & (svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF))
			break;
	}

	new->lr[!bit] = w;
	if (last == NULL) {
		sympole = new;
	} else {
		last->lr[svbit] = new;
		last->bitno &= ~(svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	}
	if (bitno < cix)
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
	return (struct symtab *)new->lr[bit];
}

static void *
xmalloc(int sz)
{
	usch *rv;

	if ((rv = (void *)malloc(sz)) == NULL)
		error("xmalloc: out of mem");
	return rv;
}

static void *
xrealloc(void *p, int sz)
{
	usch *rv;

	if ((rv = (void *)realloc(p, sz)) == NULL)
		error("xrealloc: out of mem");
	return rv;
}

static usch *
xstrdup(const usch *str)
{
	usch *rv;

	if ((rv = (usch *)strdup((const char *)str)) == NULL)
		error("xstrdup: out of mem");
	return rv;
}
