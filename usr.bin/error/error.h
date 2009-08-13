/*	$NetBSD: error.h,v 1.15 2009/08/13 06:59:37 dholland Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
 *	@(#)error.h	8.1 (Berkeley) 6/6/93
 */

#include <stdbool.h>

typedef int boolean;

/*
 * Descriptors for the various languages we know about.
 * If you touch these, also touch lang_table
 */
#define INUNKNOWN	0
#define INCPP		1
#define INCC		2
#define INAS		3
#define INLD		4
#define INLINT		5
#define INF77		6
#define INPI		7
#define INPC		8
#define INFRANZ		9
#define INLISP		10
#define INVAXIMA	11
#define INRATFOR	12
#define INLEX		13
#define INYACC		14
#define INAPL		15
#define INMAKE		16
#define INRI		17
#define INTROFF		18
#define INMOD2		19

/*
 * We analyze each line in the error message file, and
 * attempt to categorize it by type, as well as language.
 * Here are the type descriptors.
 */
typedef int Errorclass;

#define C_FIRST		  0	/* first error category */
#define C_UNKNOWN	0	/* must be zero */
#define C_IGNORE	1	/* ignore the message; used for pi */
#define C_SYNC		2	/* synchronization errors */
#define C_DISCARD	3	/* touches dangerous files, so discard */
#define C_NONSPEC	4	/* not specific to any file */
#define C_THISFILE	5	/* specific to this file, but at no line */
#define C_NULLED	6	/* refers to special func; so null */
#define C_TRUE		7	/* fits into true error format */
#define C_DUPL		8	/* sub class only; duplicated error message */
#define C_LAST		  9	/* last error category */

#define SORTABLE(x)	(!(NOTSORTABLE(x)))
#define NOTSORTABLE(x)	(x <= C_NONSPEC)

/*
 * Resources to count and print out the error categories
 */
extern const char *class_table[];
extern int class_count[];

#define nunknown	class_count[C_UNKNOWN]
#define nignore		class_count[C_IGNORE]
#define nsyncerrors	class_count[C_SYNC]
#define ndiscard	class_count[C_DISCARD]
#define nnonspec	class_count[C_NONSPEC]
#define nthisfile	class_count[C_THISFILE]
#define nnulled		class_count[C_NULLED]
#define ntrue		class_count[C_TRUE]
#define ndupl		class_count[C_DUPL]

/* places to put the error complaints */

#define TOTHEFILE	1	/* touch the file */
#define TOSTDOUT	2	/* just print them out (ho-hum) */

extern FILE *errorfile;	/* where error file comes from */
extern FILE *queryfile;	/* where the query responses from the user come from*/

extern char *processname;
extern char *scriptname;

extern const char *suffixlist;

extern boolean query;
extern boolean terse;
int inquire(const char *, ...);	/* inquire for yes/no */

/*
 * codes for inquire() to return
 */
#define Q_error	-1			/* an error occurred */
#define Q_NO	1			/* 'N' */
#define Q_no	2			/* 'n' */
#define Q_YES	3			/* 'Y' */
#define Q_yes	4			/* 'y' */

/*
 * Describes attributes about a language
 */
struct lang_desc {
	const char *lang_name;
	const char *lang_incomment;	/* one of the following defines */
	const char *lang_outcomment;	/* one of the following defines */
};
extern struct lang_desc lang_table[];

#define CINCOMMENT	"/*###"
#define COUTCOMMENT	"%%%*/\n"
#define FINCOMMENT	"C###"
#define FOUTCOMMENT	"%%%\n"
#define NEWLINE		"%%%\n"
#define PIINCOMMENT	"(*###"
#define PIOUTCOMMENT	"%%%*)\n"
#define LISPINCOMMENT	";###"
#define ASINCOMMENT	"####"
#define RIINCOMMENT	CINCOMMENT
#define RIOUTCOMMENT	COUTCOMMENT
#define TROFFINCOMMENT	".\\\"###"
#define TROFFOUTCOMMENT	NEWLINE
#define MOD2INCOMMENT	"(*###"
#define MOD2OUTCOMMENT	"%%%*)\n"

/*
 * Defines and resources for determing if a given line
 * is to be discarded because it refers to a file not to
 * be touched, or if the function reference is to a
 * function the user doesn't want recorded.
 */

#define ERRORNAME	"/.errorrc"
extern int nignored;
extern char **names_ignored;

/*
 * Structure definition for a full error
 */
typedef struct edesc Edesc;
typedef Edesc *Eptr;

struct edesc {
	Eptr error_next;		/* linked together */
	int error_lgtext;		/* how many on the right hand side */
	char **error_text;		/* the right hand side proper */
	Errorclass error_e_class;	/* error category of this error */
	Errorclass error_s_class;	/* sub descriptor of error_e_class */
	int error_language;		/* the language for this error */
	int error_position;		/* oridinal position */
	int error_line;			/* discovered line number */
	int error_no;			/* sequence number on input */
};

/*
 * Resources for the true errors
 */
extern int nerrors;
extern Eptr er_head;

extern int cur_wordc;
extern char **cur_wordv;


/*
 * Resources for each of the files mentioned
 */
extern int nfiles;
extern Eptr **files;			/* array of pointers into errors */
extern boolean *touchedfiles;		/* which files we touched */

/*
 * The language the compilation is in, as intuited from
 * the flavor of error messages analyzed.
 */
extern int language;
extern char *currentfilename;

/*
 * Macros for initializing arrays of string constants.
 * This is a fairly gross set of preprocessor hacks; the idea is
 * that instead of
 *     static char *foo[4] = { "alpha", "beta", "gamma", "delta" };
 * you do
 *     DECL_STRINGS_4(static, foo, "alpha", "beta", "gamma", "delta");
 *
 * and it comes out as
 *     static char foo_0[] = "delta";
 *     static char foo_1[] = "gamma";
 *     static char foo_2[] = "beta";
 *     static char foo_3[] = "alpha";
 *     static char *foo[4] = { foo_3, foo_2, foo_1, foo_0 };
 *
 * none of which is const.
 *
 * Unfortunately, the string arrays this program slings around freely
 * can't be all const (because it munges some of them) and can't be
 * all non-const without something like this to initialize the ones
 * that need to contain string constants. Unfortunately you can't
 * mix (const char *const *) and (char **) in C, only in C++.
 */

#define DECL_STR(sym, num, str) static char sym##_##num[] = str

#define DECL_S1(sym, s, ...) __VA_ARGS__ DECL_STR(sym, 0, s)
#define DECL_S2(sym, s, ...) DECL_STR(sym, 1, s); DECL_S1(sym, __VA_ARGS__)
#define DECL_S3(sym, s, ...) DECL_STR(sym, 2, s); DECL_S2(sym, __VA_ARGS__)
#define DECL_S4(sym, s, ...) DECL_STR(sym, 3, s); DECL_S3(sym, __VA_ARGS__)
#define DECL_S5(sym, s, ...) DECL_STR(sym, 4, s); DECL_S4(sym, __VA_ARGS__)
#define DECL_S6(sym, s, ...) DECL_STR(sym, 5, s); DECL_S5(sym, __VA_ARGS__)

#define USE_S1(sym) sym##_0
#define USE_S2(sym) sym##_1, USE_S1(sym)
#define USE_S3(sym) sym##_2, USE_S2(sym)
#define USE_S4(sym) sym##_3, USE_S3(sym)
#define USE_S5(sym) sym##_4, USE_S4(sym)
#define USE_S6(sym) sym##_5, USE_S5(sym)

#define DECL_STRS(num, class, sym, ...) \
	DECL_S##num(sym, __VA_ARGS__); \
	class char *sym[num] = { USE_S##num(sym) }

#define DECL_STRINGS_1(class, sym, ...) DECL_STRS(1, class, sym, __VA_ARGS__)
#define DECL_STRINGS_2(class, sym, ...) DECL_STRS(2, class, sym, __VA_ARGS__)
#define DECL_STRINGS_3(class, sym, ...) DECL_STRS(3, class, sym, __VA_ARGS__)
#define DECL_STRINGS_4(class, sym, ...) DECL_STRS(4, class, sym, __VA_ARGS__)
#define DECL_STRINGS_5(class, sym, ...) DECL_STRS(5, class, sym, __VA_ARGS__)
#define DECL_STRINGS_6(class, sym, ...) DECL_STRS(6, class, sym, __VA_ARGS__)
	

/*
 *	Functional forwards
 */
void arrayify(int *, Eptr **, Eptr);
void *Calloc(size_t, size_t);
void clob_last(char *,  char);
Errorclass discardit(Eptr);
void eaterrors(int *, Eptr **);
void erroradd(int, char **, Errorclass, Errorclass);
void filenames(int, Eptr **);
void findfiles(int, Eptr *, int *, Eptr ***);
char firstchar(const char *);
void getignored(const char *);
char lastchar(const char *);
char next_lastchar(const char *);
void onintr(int);
bool persperdexplode(char *, char **, char **);
Errorclass pi(void);
int position(const char *, char);
void printerrors(bool, int, Eptr []);
const char *plural(int);
char *Strdup(const char *);
char *substitute(char *, char, char);
bool touchfiles(int, Eptr **, int *, char ***);
const char *verbform(int);
void wordvbuild(char *, int*, char ***);
int wordvcmp(char **, int, char **);
void wordvprint(FILE *, int, char **);
char **wordvsplice(int, int, char **);
