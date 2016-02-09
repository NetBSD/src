/*	Id	*/	
/*	$NetBSD: driver.h,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

typedef struct list list_t;

struct options {
	const char *	cpp;
	const char *	ccom;
	const char *	cxxcom;
	const char *	fcom;
	const char *	as;
	const char *	ld;

	const char *	sysroot;

	int		version;
	list_t *	prefix;
	int		C;
	list_t *	DIU;
	int		E;
	list_t *	ldargs;
	int		M;
	int		m32;
	int		P;
	int		S;
	list_t *	Wa;
	list_t *	Wl;
	list_t *	Wp;
	int		savetemps;
	int		c;
	int		dynamiclib;
	int		hosted;
	int		uchar;
	int		pic;
	int		ssp;
	int		idirafter;
	const char *	isystem;
	list_t *	include;
	const char *	isysroot;
	int		stdinc;
	int		cxxinc;
	int		stdlib;
	int		startfiles;
	int		defaultlibs;
	const char *	outfile;
	int		pedantic;
	int		profile;
	int		pipe;
	int		print;
	int		pthread;
	int		r;
	int		shared;
	int		ldstatic;
	int		symbolic;
	int		traditional;
	int		stddef;
	int		verbose;

	int		debug;		/* -g */
	int		optim;		/* -O */
};

#define	ARRAYLEN(a)	(sizeof(a)/sizeof((a)[0]))

extern struct options opt;

/* "Warns" flags */
#define W_SET		(1<<0)
#define W_ERR		(1<<1)
#define W_ALL		(1<<2)
#define W_EXTRA		(1<<3)
#define W_CPP		(1<<4)
#define W_CCOM		(1<<5)
#define W_CXXCOM	(1<<6)
#define W_FCOM		(1<<7)
#define W_AS		(1<<8)
#define W_LD		(1<<9)

/* driver.c */
void error(const char *fmt, ...);
void warning(const char *fmt, ...);
int callsys(int, char **);

/* list.c */
const char **list_array(const list_t *);
size_t list_count(const list_t *);
list_t *list_alloc(void);
void list_free(list_t *);
void list_print(const list_t *);
void list_add_nocopy(list_t *, const char *);
void list_add(list_t *, const char *, ...);
void list_add_array(list_t *, const char **);
void list_add_list(list_t *, const list_t *);

/* options.c */
int add_option(char *, char *);

/* prog_asm.c */
int prog_asm(const char *, const char *);

/* prog_ccom.c */
int prog_ccom(const char *, const char *);

/* prog_cpp.c */
int prog_cpp(const char *, const char *);

/* prog_cxxcom.c */
int prog_cxxcom(const char *, const char *);

/* prog_fcom.c */
int prog_fcom(const char *, const char *);

/* prog_link.c */
int prog_link(const char *, const char *);

/* wflag.c */
int Wflag(const char *);
void Wflag_add(list_t *, unsigned int);

/* xalloc.c */
void	*xcalloc(size_t, size_t);
void	*xmalloc(size_t);
void	*xrealloc(void *, size_t);
char	*xstrdup(const char *);
