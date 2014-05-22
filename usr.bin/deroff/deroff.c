/*	$NetBSD: deroff.c,v 1.9.2.1 2014/05/22 11:42:43 yamt Exp $	*/

/* taken from: OpenBSD: deroff.c,v 1.6 2004/06/02 14:58:46 tom Exp */

/*-
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
__RCSID("$NetBSD: deroff.c,v 1.9.2.1 2014/05/22 11:42:43 yamt Exp $");

#include <err.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 *	Deroff command -- strip troff, eqn, and Tbl sequences from
 *	a file.  Has two flags argument, -w, to cause output one word per line
 *	rather than in the original format.
 *	-mm (or -ms) causes the corresponding macro's to be interpreted
 *	so that just sentences are output
 *	-ml  also gets rid of lists.
 *	Deroff follows .so and .nx commands, removes contents of macro
 *	definitions, equations (both .EQ ... .EN and $...$),
 *	Tbl command sequences, and Troff backslash constructions.
 *
 *	All input is through the Cget macro;
 *	the most recently read character is in c.
 *
 *	Modified by Robert Henry to process -me and -man macros.
 */

#define Cget ( (c=getc(infile)) == EOF ? eof() : ((c==ldelim)&&(filesp==files) ? skeqn() : c) )
#define C1get ( (c=getc(infile)) == EOF ? eof() :  c)

#ifdef DEBUG
#  define C	_C()
#  define C1	_C1()
#else /* not DEBUG */
#  define C	Cget
#  define C1	C1get
#endif /* not DEBUG */

#define SKIP while (C != '\n')
#define SKIP_TO_COM SKIP; SKIP; pc=c; while (C != '.' || pc != '\n' || C > 'Z')pc=c

#define	YES 1
#define	NO 0
#define	MS 0	/* -ms */
#define	MM 1	/* -mm */
#define	ME 2	/* -me */
#define	MA 3	/* -man */

#ifdef DEBUG
static char *mactab[] = { "-ms", "-mm", "-me", "-ma" };
#endif /* DEBUG */

#define	ONE 1
#define	TWO 2

#define NOCHAR -2
#define SPECIAL 0
#define APOS 1
#define PUNCT 2
#define DIGIT 3
#define LETTER 4

#define MAXFILES 20

static int	iflag;
static int	wordflag;
static int	msflag;	 /* processing a source written using a mac package */
static int	mac;		/* which package */
static int	disp;
static int	parag;
static int	inmacro;
static int	intable;
static int	keepblock; /* keep blocks of text; normally false when msflag */

static char chars[128];  /* SPECIAL, PUNCT, APOS, DIGIT, or LETTER */

static char line[LINE_MAX];
static char *lp;

static int c;
static int pc;
static int ldelim;
static int rdelim;

static char fname[PATH_MAX];
static FILE *files[MAXFILES];
static FILE **filesp;
static FILE *infile;

static int argc;
static char **argv;

/*
 *	Macro processing
 *
 *	Macro table definitions
 */
typedef	int pacmac;		/* compressed macro name */
static int	argconcat = 0;	/* concat arguments together (-me only) */

#define	tomac(c1, c2)		((((c1) & 0xFF) << 8) | ((c2) & 0xFF))
#define	frommac(src, c1, c2)	(((c1)=((src)>>8)&0xFF),((c2) =(src)&0xFF), __USE(c1), __USE(c2))

struct mactab {
	int	condition;
	pacmac	macname;
	int	(*func)(pacmac);
};

static const struct	mactab	troffmactab[];
static const struct	mactab	ppmactab[];
static const struct	mactab	msmactab[];
static const struct	mactab	mmmactab[];
static const struct	mactab	memactab[];
static const struct	mactab	manmactab[];

/*
 *	Macro table initialization
 */
#define	M(cond, c1, c2, func) {cond, tomac(c1, c2), func}

/*
 *	Flags for matching conditions other than
 *	the macro name
 */
#define	NONE		0
#define	FNEST		1		/* no nested files */
#define	NOMAC		2		/* no macro */
#define	MAC		3		/* macro */
#define	PARAG		4		/* in a paragraph */
#define	MSF		5		/* msflag is on */
#define	NBLK		6		/* set if no blocks to be kept */

/*
 *	Return codes from macro minions, determine where to jump,
 *	how to repeat/reprocess text
 */
#define	COMX		1		/* goto comx */
#define	COM		2		/* goto com */

static int	 skeqn(void);
static int	 eof(void);
#ifdef DEBUG
static int	 _C1(void);
static int	 _C(void);
#endif
static int	 EQ(pacmac);
static int	 domacro(pacmac);
static int	 PS(pacmac);
static int	 skip(pacmac);
static int	 intbl(pacmac);
static int	 outtbl(pacmac);
static int	 so(pacmac);
static int	 nx(pacmac);
static int	 skiptocom(pacmac);
static int	 PP(pacmac);
static int	 AU(pacmac);
static int	 SH(pacmac);
static int	 UX(pacmac);
static int	 MMHU(pacmac);
static int	 mesnblock(pacmac);
static int	 mssnblock(pacmac);
static int	 nf(pacmac);
static int	 ce(pacmac);
static int	 meip(pacmac);
static int	 mepp(pacmac);
static int	 mesh(pacmac);
static int	 mefont(pacmac);
static int	 manfont(pacmac);
static int	 manpp(pacmac);
static int	 macsort(const void *, const void *);
static int	 sizetab(const struct mactab *);
static void	 getfname(void);
static void	 textline(char *, int);
static void	 work(void) __dead;
static void	 regline(void (*)(char *, int), int);
static void	 macro(void);
static void	 tbl(void);
static void	 stbl(void);
static void	 eqn(void);
static void	 backsl(void);
static void	 sce(void);
static void	 refer(int);
static void	 inpic(void);
static void	 msputmac(char *, int);
static void	 msputwords(int);
static void	 meputmac(char *, int);
static void	 meputwords(int);
static void	 noblock(char, char);
static void	 defcomline(pacmac);
static void	 comline(void);
static void	 buildtab(const struct mactab **, int *);
static FILE	*opn(char *);
static struct mactab *macfill(struct mactab *, const struct mactab *);
static void usage(void) __dead;

int
main(int ac, char **av)
{
	int	i, ch;
	int	errflg = 0;
	int	kflag = NO;

	iflag = NO;
	wordflag = NO;
	msflag = NO;
	mac = ME;
	disp = NO;
	parag = NO;
	inmacro = NO;
	intable = NO;
	ldelim	= NOCHAR;
	rdelim	= NOCHAR;
	keepblock = YES;

	while ((ch = getopt(ac, av, "ikpwm:")) != -1) {
		switch (ch) {
		case 'i':
			iflag = YES;
			break;
		case 'k':
			kflag = YES;
			break;
		case 'm':
			msflag = YES;
			keepblock = NO;
			switch (optarg[0]) {
			case 'm':
				mac = MM;
				break;
			case 's':
				mac = MS;
				break;
			case 'e':
				mac = ME;
				break;
			case 'a':
				mac = MA;
				break;
			case 'l':
				disp = YES;
				break;
			default:
				errflg++;
				break;
			}
			if (errflg == 0 && optarg[1] != '\0')
				errflg++;
			break;
		case 'p':
			parag = YES;
			break;
		case 'w':
			wordflag = YES;
			kflag = YES;
			break;
		default:
			errflg++;
		}
	}
	argc = ac - optind;
	argv = av + optind;

	if (kflag)
		keepblock = YES;
	if (errflg)
		usage();

#ifdef DEBUG
	printf("msflag = %d, mac = %s, keepblock = %d, disp = %d\n",
		msflag, mactab[mac], keepblock, disp);
#endif /* DEBUG */
	if (argc == 0) {
		infile = stdin;
	} else {
		infile = opn(argv[0]);
		--argc;
		++argv;
	}
	files[0] = infile;
	filesp = &files[0];

	for (i = 'a'; i <= 'z' ; ++i)
		chars[i] = LETTER;
	for (i = 'A'; i <= 'Z'; ++i)
		chars[i] = LETTER;
	for (i = '0'; i <= '9'; ++i)
		chars[i] = DIGIT;
	chars['\''] = APOS;
	chars['&'] = APOS;
	chars['.'] = PUNCT;
	chars[','] = PUNCT;
	chars[';'] = PUNCT;
	chars['?'] = PUNCT;
	chars[':'] = PUNCT;
	work();
	return 0;
}

static int
skeqn(void)
{

	while ((c = getc(infile)) != rdelim) {
		if (c == EOF)
			c = eof();
		else if (c == '"') {
			while ((c = getc(infile)) != '"') {
				if (c == EOF ||
				    (c == '\\' && (c = getc(infile)) == EOF))
					c = eof();
			}
		}
	}
	if (msflag)
		return c == 'x';
	return c == ' ';
}

static FILE *
opn(char *p)
{
	FILE *fd;

	if ((fd = fopen(p, "r")) == NULL)
		err(1, "fopen %s", p);

	return fd;
}

static int
eof(void)
{

	if (infile != stdin)
		fclose(infile);
	if (filesp > files)
		infile = *--filesp;
	else if (argc > 0) {
		infile = opn(argv[0]);
		--argc;
		++argv;
	} else
		exit(0);
	return C;
}

static void
getfname(void)
{
	char *p;
	struct chain {
		struct chain *nextp;
		char *datap;
	} *q;
	static struct chain *namechain= NULL;

	while (C == ' ')
		;	/* nothing */

	for (p = fname ; p - fname < (ptrdiff_t)sizeof(fname) &&
	    (*p = c) != '\n' &&
	    c != ' ' && c != '\t' && c != '\\'; ++p)
		C;
	*p = '\0';
	while (c != '\n')
		C;

	/* see if this name has already been used */
	for (q = namechain ; q; q = q->nextp)
		if (strcmp(fname, q->datap) == 0) {
			fname[0] = '\0';
			return;
		}

	q = (struct chain *) malloc(sizeof(struct chain));
	if (q == NULL)
		err(1, NULL);
	q->nextp = namechain;
	q->datap = strdup(fname);
	if (q->datap == NULL)
		err(1, NULL);
	namechain = q;
}

/*ARGSUSED*/
static void
textline(char *str, int constant)
{

	if (wordflag) {
		msputwords(0);
		return;
	}
	puts(str);
}

static void
work(void)
{

	for (;;) {
		C;
#ifdef FULLDEBUG
		printf("Starting work with `%c'\n", c);
#endif /* FULLDEBUG */
		if (c == '.' || c == '\'')
			comline();
		else
			regline(textline, TWO);
	}
}

static void
regline(void (*pfunc)(char *, int), int constant)
{

	line[0] = c;
	lp = line;
	while (lp - line < (ptrdiff_t)sizeof(line)) {
		if (c == '\\') {
			*lp = ' ';
			backsl();
		}
		if (c == '\n')
			break;
		if (intable && c == 'T') {
			*++lp = C;
			if (c == '{' || c == '}') {
				lp[-1] = ' ';
				*lp = C;
			}
		} else {
			*++lp = C;
		}
	}
	*lp = '\0';

	if (line[0] != '\0')
		(*pfunc)(line, constant);
}

static void
macro(void)
{

	if (msflag) {
		do {
			SKIP;
		} while (C!='.' || C!='.' || C=='.');	/* look for  .. */
		if (c != '\n')
			SKIP;
		return;
	}
	SKIP;
	inmacro = YES;
}

static void
tbl(void)
{

	while (C != '.')
		;	/* nothing */
	SKIP;
	intable = YES;
}

static void
stbl(void)
{

	while (C != '.')
		;	/* nothing */
	SKIP_TO_COM;
	if (c != 'T' || C != 'E') {
		SKIP;
		pc = c;
		while (C != '.' || pc != '\n' || C != 'T' || C != 'E')
			pc = c;
	}
}

static void
eqn(void)
{
	int c1, c2;
	int dflg;
	char last;

	last=0;
	dflg = 1;
	SKIP;

	for (;;) {
		if (C1 == '.'  || c == '\'') {
			while (C1 == ' ' || c == '\t')
				;
			if (c == 'E' && C1 == 'N') {
				SKIP;
				if (msflag && dflg) {
					putchar('x');
					putchar(' ');
					if (last) {
						putchar(last);
						putchar('\n');
					}
				}
				return;
			}
		} else if (c == 'd') {
			/* look for delim */
			if (C1 == 'e' && C1 == 'l')
				if (C1 == 'i' && C1 == 'm') {
					while (C1 == ' ')
						;	/* nothing */

					if ((c1 = c) == '\n' ||
					    (c2 = C1) == '\n' ||
					    (c1 == 'o' && c2 == 'f' && C1=='f')) {
						ldelim = NOCHAR;
						rdelim = NOCHAR;
					} else {
						ldelim = c1;
						rdelim = c2;
					}
				}
			dflg = 0;
		}

		if (c != '\n')
			while (C1 != '\n') {
				if (chars[c] == PUNCT)
					last = c;
				else if (c != ' ')
					last = 0;
			}
	}
}

/* skip over a complete backslash construction */
static void
backsl(void)
{
	int bdelim;

sw:
	switch (C) {
	case '"':
		SKIP;
		return;

	case 's':
		if (C == '\\')
			backsl();
		else {
			while (C >= '0' && c <= '9')
				;	/* nothing */
			ungetc(c, infile);
			c = '0';
		}
		--lp;
		return;

	case 'f':
	case 'n':
	case '*':
		if (C != '(')
			return;

	case '(':
		if (msflag) {
			if (C == 'e') {
				if (C == 'm') {
					*lp = '-';
					return;
				}
			}
			else if (c != '\n')
				C;
			return;
		}
		if (C != '\n')
			C;
		return;

	case '$':
		C;	/* discard argument number */
		return;

	case 'b':
	case 'x':
	case 'v':
	case 'h':
	case 'w':
	case 'o':
	case 'l':
	case 'L':
		if ((bdelim = C) == '\n')
			return;
		while (C != '\n' && c != bdelim)
			if (c == '\\')
				backsl();
		return;

	case '\\':
		if (inmacro)
			goto sw;

	default:
		return;
	}
}

static void
sce(void)
{
	char *ap;
	int n, i;
	char a[10];

	for (ap = a; C != '\n'; ap++) {
		*ap = c;
		if (ap == &a[9]) {
			SKIP;
			ap = a;
			break;
		}
	}
	if (ap != a)
		n = atoi(a);
	else
		n = 1;
	for (i = 0; i < n;) {
		if (C == '.') {
			if (C == 'c') {
				if (C == 'e') {
					while (C == ' ')
						;	/* nothing */
					if (c == '0') {
						SKIP;
						break;
					} else
						SKIP;
				}
				else
					SKIP;
			} else if (c == 'P' || C == 'P') {
				if (c != '\n')
					SKIP;
				break;
			} else if (c != '\n')
				SKIP;
		} else {
			SKIP;
			i++;
		}
	}
}

static void
refer(int c1)
{
	int c2;

	if (c1 != '\n')
		SKIP;

	for (c2 = -1;;) {
		if (C != '.')
			SKIP;
		else {
			if (C != ']')
				SKIP;
			else {
				while (C != '\n')
					c2 = c;
				if (c2 != -1 && chars[c2] == PUNCT)
					putchar(c2);
				return;
			}
		}
	}
}

static void
inpic(void)
{
	int c1;
	char *p1;

	SKIP;
	p1 = line;
	c = '\n';
	for (;;) {
		c1 = c;
		if (C == '.' && c1 == '\n') {
			if (C != 'P') {
				if (c == '\n')
					continue;
				else {
					SKIP;
					c = '\n';
					continue;
				}
			}
			if (C != 'E') {
				if (c == '\n')
					continue;
				else {
					SKIP;
					c = '\n';
					continue;
				}
			}
			SKIP;
			return;
		}
		else if (c == '\"') {
			while (C != '\"') {
				if (c == '\\') {
					if (C == '\"')
						continue;
					ungetc(c, infile);
					backsl();
				} else
					*p1++ = c;
			}
			*p1++ = ' ';
		}
		else if (c == '\n' && p1 != line) {
			*p1 = '\0';
			if (wordflag)
				msputwords(NO);
			else {
				puts(line);
				putchar('\n');
			}
			p1 = line;
		}
	}
}

#ifdef DEBUG
static int
_C1(void)
{

	return C1get;
}

static int
_C(void)
{

	return Cget;
}
#endif /* DEBUG */

/*
 *	Put out a macro line, using ms and mm conventions.
 */
static void
msputmac(char *s, int constant)
{
	char *t;
	int found;
	int last;

	last = 0;
	found = 0;
	if (wordflag) {
		msputwords(YES);
		return;
	}
	while (*s) {
		while (*s == ' ' || *s == '\t')
			putchar(*s++);
		for (t = s ; *t != ' ' && *t != '\t' && *t != '\0' ; ++t)
			;	/* nothing */
		if (*s == '\"')
			s++;
		if (t > s + constant && chars[(unsigned char)s[0]] == LETTER &&
		    chars[(unsigned char)s[1]] == LETTER) {
			while (s < t)
				if (*s == '\"')
					s++;
				else
					putchar(*s++);
			last = *(t-1);
			found++;
		} else if (found && chars[(unsigned char)s[0]] == PUNCT &&
		    s[1] == '\0') {
			putchar(*s++);
		} else {
			last = *(t - 1);
			s = t;
		}
	}
	putchar('\n');
	if (msflag && chars[last] == PUNCT) {
		putchar(last);
		putchar('\n');
	}
}

/*
 *	put out words (for the -w option) with ms and mm conventions
 */
static void
msputwords(int macline)
{
	char *p, *p1;
	int i, nlet;

	for (p1 = line;;) {
		/*
		 *	skip initial specials ampersands and apostrophes
		 */
		while (chars[(unsigned char)*p1] < DIGIT)
			if (*p1++ == '\0')
				return;
		nlet = 0;
		for (p = p1 ; (i = chars[(unsigned char)*p]) != SPECIAL ; ++p)
			if (i == LETTER)
				++nlet;

		if (nlet > 1 && chars[(unsigned char)p1[0]] == LETTER) {
			/*
			 *	delete trailing ampersands and apostrophes
			 */
			while ((i = chars[(unsigned char)p[-1]]) == PUNCT ||
			    i == APOS )
				--p;
			while (p1 < p)
				putchar(*p1++);
			putchar('\n');
		} else {
			p1 = p;
		}
	}
}

/*
 *	put out a macro using the me conventions
 */
#define SKIPBLANK(cp)	while (*cp == ' ' || *cp == '\t') { cp++; }
#define SKIPNONBLANK(cp) while (*cp !=' ' && *cp !='\cp' && *cp !='\0') { cp++; }

static void
meputmac(char *cp, int constant)
{
	char	*np;
	int	found;
	int	argno;
	int	last;
	int	inquote;

	last = 0;
	found = 0;
	if (wordflag) {
		meputwords(YES);
		return;
	}
	for (argno = 0; *cp; argno++) {
		SKIPBLANK(cp);
		inquote = (*cp == '"');
		if (inquote)
			cp++;
		for (np = cp; *np; np++) {
			switch (*np) {
			case '\n':
			case '\0':
				break;

			case '\t':
			case ' ':
				if (inquote)
					continue;
				else
					goto endarg;

			case '"':
				if (inquote && np[1] == '"') {
					memmove(np, np + 1, strlen(np));
					np++;
					continue;
				} else {
					*np = ' '; 	/* bye bye " */
					goto endarg;
				}

			default:
				continue;
			}
		}
		endarg: ;
		/*
		 *	cp points at the first char in the arg
		 *	np points one beyond the last char in the arg
		 */
		if ((argconcat == 0) || (argconcat != argno))
			putchar(' ');
#ifdef FULLDEBUG
		{
			char	*p;
			printf("[%d,%d: ", argno, np - cp);
			for (p = cp; p < np; p++) {
				putchar(*p);
			}
			printf("]");
		}
#endif /* FULLDEBUG */
		/*
		 *	Determine if the argument merits being printed
		 *
		 *	constant is the cut off point below which something
		 *	is not a word.
		 */
		if (((np - cp) > constant) &&
		    (inquote || (chars[(unsigned char)cp[0]] == LETTER))) {
			for (; cp < np; cp++)
				putchar(*cp);
			last = np[-1];
			found++;
		} else if (found && (np - cp == 1) &&
		    chars[(unsigned char)*cp] == PUNCT) {
			putchar(*cp);
		} else {
			last = np[-1];
		}
		cp = np;
	}
	if (msflag && chars[last] == PUNCT)
		putchar(last);
	putchar('\n');
}

/*
 *	put out words (for the -w option) with ms and mm conventions
 */
static void
meputwords(int macline)
{

	msputwords(macline);
}

/*
 *
 *	Skip over a nested set of macros
 *
 *	Possible arguments to noblock are:
 *
 *	fi	end of unfilled text
 *	PE	pic ending
 *	DE	display ending
 *
 *	for ms and mm only:
 *		KE	keep ending
 *
 *		NE	undocumented match to NS (for mm?)
 *		LE	mm only: matches RL or *L (for lists)
 *
 *	for me:
 *		([lqbzcdf]
 */
static void
noblock(char a1, char a2)
{
	int c1,c2;
	int eqnf;
	int lct;

	lct = 0;
	eqnf = 1;
	SKIP;
	for (;;) {
		while (C != '.')
			if (c == '\n')
				continue;
			else
				SKIP;
		if ((c1 = C) == '\n')
			continue;
		if ((c2 = C) == '\n')
			continue;
		if (c1 == a1 && c2 == a2) {
			SKIP;
			if (lct != 0) {
				lct--;
				continue;
			}
			if (eqnf)
				putchar('.');
			putchar('\n');
			return;
		} else if (a1 == 'L' && c2 == 'L') {
			lct++;
			SKIP;
		}
		/*
		 *	equations (EQ) nested within a display
		 */
		else if (c1 == 'E' && c2 == 'Q') {
			if ((mac == ME && a1 == ')')
			    || (mac != ME && a1 == 'D')) {
				eqn();
				eqnf=0;
			}
		}
		/*
		 *	turning on filling is done by the paragraphing
		 *	macros
		 */
		else if (a1 == 'f') {	/* .fi */
			if  ((mac == ME && (c2 == 'h' || c2 == 'p'))
			    || (mac != ME && (c1 == 'P' || c2 == 'P'))) {
				SKIP;
				return;
			}
		} else {
			SKIP;
		}
	}
}

static int
/*ARGSUSED*/
EQ(pacmac unused)
{

	eqn();
	return 0;
}

static int
/*ARGSUSED*/
domacro(pacmac unused)
{

	macro();
	return 0;
}

static int
/*ARGSUSED*/
PS(pacmac unused)
{

	for (C; c == ' ' || c == '\t'; C)
		;	/* nothing */

	if (c == '<') {		/* ".PS < file" -- don't expect a .PE */
		SKIP;
		return 0;
	}
	if (!msflag)
		inpic();
	else
		noblock('P', 'E');
	return 0;
}

static int
/*ARGSUSED*/
skip(pacmac unused)
{

	SKIP;
	return 0;
}

static int
/*ARGSUSED*/
intbl(pacmac unused)
{

	if (msflag)
		stbl();
	else
		tbl();
	return 0;
}

static int
/*ARGSUSED*/
outtbl(pacmac unused)
{

	intable = NO;
	return 0;
}

static int
/*ARGSUSED*/
so(pacmac unused)
{

	if (!iflag) {
		getfname();
		if (fname[0]) {
			if (++filesp - &files[0] > MAXFILES)
				err(1, "too many nested files (max %d)",
				    MAXFILES);
			infile = *filesp = opn(fname);
		}
	}
	return 0;
}

static int
/*ARGSUSED*/
nx(pacmac unused)
{

	if (!iflag) {
		getfname();
		if (fname[0] == '\0')
			exit(0);
		if (infile != stdin)
			fclose(infile);
		infile = *filesp = opn(fname);
	}
	return 0;
}

static int
/*ARGSUSED*/
skiptocom(pacmac unused)
{

	SKIP_TO_COM;
	return COMX;
}

static int
PP(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);
	printf(".%c%c", c1, c2);
	while (C != '\n')
		putchar(c);
	putchar('\n');
	return 0;
}

static int
/*ARGSUSED*/
AU(pacmac unused)
{

	if (mac == MM)
		return 0;
	SKIP_TO_COM;
	return COMX;
}

static int
SH(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);

	if (parag) {
		printf(".%c%c", c1, c2);
		while (C != '\n')
			putchar(c);
		putchar(c);
		putchar('!');
		for (;;) {
			while (C != '\n')
				putchar(c);
			putchar('\n');
			if (C == '.')
				return COM;
			putchar('!');
			putchar(c);
		}
		/*NOTREACHED*/
	} else {
		SKIP_TO_COM;
		return COMX;
	}
}

static int
/*ARGSUSED*/
UX(pacmac unused)
{

	if (wordflag)
		printf("UNIX\n");
	else
		printf("UNIX ");
	return 0;
}

static int
MMHU(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);
	if (parag) {
		printf(".%c%c", c1, c2);
		while (C != '\n')
			putchar(c);
		putchar('\n');
	} else {
		SKIP;
	}
	return 0;
}

static int
mesnblock(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);
	noblock(')', c2);
	return 0;
}

static int
mssnblock(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);
	noblock(c1, 'E');
	return 0;
}

static int
/*ARGUSED*/
nf(pacmac unused)
{

	noblock('f', 'i');
	return 0;
}

static int
/*ARGUSED*/
ce(pacmac unused)
{

	sce();
	return 0;
}

static int
meip(pacmac c12)
{

	if (parag)
		mepp(c12);
	else if (wordflag)	/* save the tag */
		regline(meputmac, ONE);
	else
		SKIP;
	return 0;
}

/*
 *	only called for -me .pp or .sh, when parag is on
 */
static int
mepp(pacmac c12)
{

	PP(c12);		/* eats the line */
	return 0;
}

/*
 *	Start of a section heading; output the section name if doing words
 */
static int
mesh(pacmac c12)
{

	if (parag)
		mepp(c12);
	else if (wordflag)
		defcomline(c12);
	else
		SKIP;
	return 0;
}

/*
 *	process a font setting
 */
static int
mefont(pacmac c12)
{

	argconcat = 1;
	defcomline(c12);
	argconcat = 0;
	return 0;
}

static int
manfont(pacmac c12)
{

	return mefont(c12);
}

static int
manpp(pacmac c12)
{

	return mepp(c12);
}

static void
defcomline(pacmac c12)
{
	int c1, c2;

	frommac(c12, c1, c2);
	if (msflag && mac == MM && c2 == 'L') {
		if (disp || c1 == 'R') {
			noblock('L', 'E');
		} else {
			SKIP;
			putchar('.');
		}
	}
	else if (c1 == '.' && c2 == '.') {
		if (msflag) {
			SKIP;
			return;
		}
		while (C == '.')
			/*VOID*/;
	}
	++inmacro;
	/*
	 *	Process the arguments to the macro
	 */
	switch (mac) {
	default:
	case MM:
	case MS:
		if (c1 <= 'Z' && msflag)
			regline(msputmac, ONE);
		else
			regline(msputmac, TWO);
		break;
	case ME:
		regline(meputmac, ONE);
		break;
	}
	--inmacro;
}

static void
comline(void)
{
	int	c1;
	int	c2;
	pacmac	c12;
	int	mid;
	int	lb, ub;
	int	hit;
	static	int	tabsize = 0;
	static	const struct mactab	*mactab = NULL;
	const struct mactab	*mp;

	if (mactab == 0)
		 buildtab(&mactab, &tabsize);
com:
	while (C == ' ' || c == '\t')
		;
comx:
	if ((c1 = c) == '\n')
		return;
	c2 = C;
	if (c1 == '.' && c2 != '.')
		inmacro = NO;
	if (msflag && c1 == '[') {
		refer(c2);
		return;
	}
	if (parag && mac==MM && c1 == 'P' && c2 == '\n') {
		printf(".P\n");
		return;
	}
	if (c2 == '\n')
		return;
	/*
	 *	Single letter macro
	 */
	if (mac == ME && (c2 == ' ' || c2 == '\t') )
		c2 = ' ';
	c12 = tomac(c1, c2);
	/*
	 *	binary search through the table of macros
	 */
	lb = 0;
	ub = tabsize - 1;
	while (lb <= ub) {
		mid = (ub + lb) / 2;
		mp = &mactab[mid];
		if (mp->macname < c12)
			lb = mid + 1;
		else if (mp->macname > c12)
			ub = mid - 1;
		else {
			hit = 1;
#ifdef FULLDEBUG
			printf("preliminary hit macro %c%c ", c1, c2);
#endif /* FULLDEBUG */
			switch (mp->condition) {
			case NONE:
				hit = YES;
				break;
			case FNEST:
				hit = (filesp == files);
				break;
			case NOMAC:
				hit = !inmacro;
				break;
			case MAC:
				hit = inmacro;
				break;
			case PARAG:
				hit = parag;
				break;
			case NBLK:
				hit = !keepblock;
				break;
			default:
				hit = 0;
			}

			if (hit) {
#ifdef FULLDEBUG
				printf("MATCH\n");
#endif /* FULLDEBUG */
				switch ((*(mp->func))(c12)) {
				default:
					return;
				case COMX:
					goto comx;
				case COM:
					goto com;
				}
			}
#ifdef FULLDEBUG
			printf("FAIL\n");
#endif /* FULLDEBUG */
			break;
		}
	}
	defcomline(c12);
}

static int
macsort(const void *p1, const void *p2)
{
	const struct mactab *t1 = p1;
	const struct mactab *t2 = p2;

	return t1->macname - t2->macname;
}

static int
sizetab(const struct mactab *mp)
{
	int i;

	i = 0;
	if (mp) {
		for (; mp->macname; mp++, i++)
			/*VOID*/ ;
	}
	return i;
}

static struct mactab *
macfill(struct mactab *dst, const struct mactab *src)
{

	if (src) {
		while (src->macname)
			*dst++ = *src++;
	}
	return dst;
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-ikpw ] [ -m a | e | l | m | s] [file ...]\n", __progname);
	exit(1);
}

static void
buildtab(const struct mactab **r_back, int *r_size)
{
	size_t	size;
	const struct	mactab	*p1, *p2;
	struct	mactab	*back, *p;

	size = sizetab(troffmactab) + sizetab(ppmactab);
	p1 = p2 = NULL;
	if (msflag) {
		switch (mac) {
		case ME:
			p1 = memactab;
			break;
		case MM:
			p1 = msmactab;
			p2 = mmmactab;
			break;
		case MS:
			p1 = msmactab;
			break;
		case MA:
			p1 = manmactab;
			break;
		default:
			break;
		}
	}
	size += sizetab(p1);
	size += sizetab(p2);
	back = calloc(size + 2, sizeof(struct mactab));
	if (back == NULL)
		err(1, NULL);

	p = macfill(back, troffmactab);
	p = macfill(p, ppmactab);
	p = macfill(p, p1);
	p = macfill(p, p2);

	qsort(back, size, sizeof(struct mactab), macsort);
	*r_size = size;
	*r_back = back;
}

/*
 *	troff commands
 */
static const struct mactab	troffmactab[] = {
	M(NONE,		'\\','"',	skip),	/* comment */
	M(NOMAC,	'd','e',	domacro),	/* define */
	M(NOMAC,	'i','g',	domacro),	/* ignore till .. */
	M(NOMAC,	'a','m',	domacro),	/* append macro */
	M(NBLK,		'n','f',	nf),	/* filled */
	M(NBLK,		'c','e',	ce),	/* centered */

	M(NONE,		's','o',	so),	/* source a file */
	M(NONE,		'n','x',	nx),	/* go to next file */

	M(NONE,		't','m',	skip),	/* print string on tty */
	M(NONE,		'h','w',	skip),	/* exception hyphen words */
	M(NONE,		0,0,		0)
};

/*
 *	Preprocessor output
 */
static const struct mactab	ppmactab[] = {
	M(FNEST,	'E','Q',	EQ),	/* equation starting */
	M(FNEST,	'T','S',	intbl),	/* table starting */
	M(FNEST,	'T','C',	intbl),	/* alternative table? */
	M(FNEST,	'T','&',	intbl),	/* table reformatting */
	M(NONE,		'T','E',	outtbl),/* table ending */
	M(NONE,		'P','S',	PS),	/* picture starting */
	M(NONE,		0,0,		0)
};

/*
 *	Particular to ms and mm
 */
static const struct mactab	msmactab[] = {
	M(NONE,		'T','L',	skiptocom),	/* title follows */
	M(NONE,		'F','S',	skiptocom),	/* start footnote */
	M(NONE,		'O','K',	skiptocom),	/* Other kws */

	M(NONE,		'N','R',	skip),	/* undocumented */
	M(NONE,		'N','D',	skip),	/* use supplied date */

	M(PARAG,	'P','P',	PP),	/* begin parag */
	M(PARAG,	'I','P',	PP),	/* begin indent parag, tag x */
	M(PARAG,	'L','P',	PP),	/* left blocked parag */

	M(NONE,		'A','U',	AU),	/* author */
	M(NONE,		'A','I',	AU),	/* authors institution */

	M(NONE,		'S','H',	SH),	/* section heading */
	M(NONE,		'S','N',	SH),	/* undocumented */
	M(NONE,		'U','X',	UX),	/* unix */

	M(NBLK,		'D','S',	mssnblock),	/* start display text */
	M(NBLK,		'K','S',	mssnblock),	/* start keep */
	M(NBLK,		'K','F',	mssnblock),	/* start float keep */
	M(NONE,		0,0,		0)
};

static const struct mactab	mmmactab[] = {
	M(NONE,		'H',' ',	MMHU),	/* -mm ? */
	M(NONE,		'H','U',	MMHU),	/* -mm ? */
	M(PARAG,	'P',' ',	PP),	/* paragraph for -mm */
	M(NBLK,		'N','S',	mssnblock),	/* undocumented */
	M(NONE,		0,0,		0)
};

static const struct mactab	memactab[] = {
	M(PARAG,	'p','p',	mepp),
	M(PARAG,	'l','p',	mepp),
	M(PARAG,	'n','p',	mepp),
	M(NONE,		'i','p',	meip),

	M(NONE,		's','h',	mesh),
	M(NONE,		'u','h',	mesh),

	M(NBLK,		'(','l',	mesnblock),
	M(NBLK,		'(','q',	mesnblock),
	M(NBLK,		'(','b',	mesnblock),
	M(NBLK,		'(','z',	mesnblock),
	M(NBLK,		'(','c',	mesnblock),

	M(NBLK,		'(','d',	mesnblock),
	M(NBLK,		'(','f',	mesnblock),
	M(NBLK,		'(','x',	mesnblock),

	M(NONE,		'r',' ',	mefont),
	M(NONE,		'i',' ',	mefont),
	M(NONE,		'b',' ',	mefont),
	M(NONE,		'u',' ',	mefont),
	M(NONE,		'q',' ',	mefont),
	M(NONE,		'r','b',	mefont),
	M(NONE,		'b','i',	mefont),
	M(NONE,		'b','x',	mefont),
	M(NONE,		0,0,		0)
};

static const struct mactab	manmactab[] = {
	M(PARAG,	'B','I',	manfont),
	M(PARAG,	'B','R',	manfont),
	M(PARAG,	'I','B',	manfont),
	M(PARAG,	'I','R',	manfont),
	M(PARAG,	'R','B',	manfont),
	M(PARAG,	'R','I',	manfont),

	M(PARAG,	'P','P',	manpp),
	M(PARAG,	'L','P',	manpp),
	M(PARAG,	'H','P',	manpp),
	M(NONE,		0,0,		0)
};
