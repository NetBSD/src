/* $NetBSD: test.c,v 1.31 2007/03/28 01:47:25 christos Exp $ */

/*
 * test(1); version 7-like  --  author Erik Baalbergen
 * modified by Eric Gisin to be used as built-in.
 * modified by Arnold Robbins to add SVR3 compatibility
 * (-x -c -b -p -u -g -k) plus Korn's -L -nt -ot -ef and new -S (socket).
 * modified by J.T. Conklin for NetBSD.
 *
 * This program is in the Public Domain.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: test.c,v 1.31 2007/03/28 01:47:25 christos Exp $");
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

/* test(1) accepts the following grammar:
	oexpr	::= aexpr | aexpr "-o" oexpr ;
	aexpr	::= nexpr | nexpr "-a" aexpr ;
	nexpr	::= primary | "!" primary
	primary	::= unary-operator operand
		| operand binary-operator operand
		| operand
		| "(" oexpr ")"
		;
	unary-operator ::= "-r"|"-w"|"-x"|"-f"|"-d"|"-c"|"-b"|"-p"|
		"-u"|"-g"|"-k"|"-s"|"-t"|"-z"|"-n"|"-o"|"-O"|"-G"|"-L"|"-S";

	binary-operator ::= "="|"!="|"-eq"|"-ne"|"-ge"|"-gt"|"-le"|"-lt"|
			"-nt"|"-ot"|"-ef";
	operand ::= <any legal UNIX file name>
*/

enum token {
	EOI,
	FILRD,
	FILWR,
	FILEX,
	FILEXIST,
	FILREG,
	FILDIR,
	FILCDEV,
	FILBDEV,
	FILFIFO,
	FILSOCK,
	FILSYM,
	FILGZ,
	FILTT,
	FILSUID,
	FILSGID,
	FILSTCK,
	FILNT,
	FILOT,
	FILEQ,
	FILUID,
	FILGID,
	STREZ,
	STRNZ,
	STREQ,
	STRNE,
	STRLT,
	STRGT,
	INTEQ,
	INTNE,
	INTGE,
	INTGT,
	INTLE,
	INTLT,
	UNOT,
	BAND,
	BOR,
	LPAREN,
	RPAREN,
	OPERAND
};

enum token_types {
	UNOP,
	BINOP,
	BUNOP,
	BBINOP,
	PAREN
};

struct t_op {
	const char *op_text;
	short op_num, op_type;
};

static const struct t_op cop[] = {
	{"!",	UNOT,	BUNOP},
	{"(",	LPAREN,	PAREN},
	{")",	RPAREN,	PAREN},
	{"<",	STRLT,	BINOP},
	{"=",	STREQ,	BINOP},
	{">",	STRGT,	BINOP},
};

static const struct t_op cop2[] = {
	{"!=",	STRNE,	BINOP},
};

static const struct t_op mop3[] = {
	{"ef",	FILEQ,	BINOP},
	{"eq",	INTEQ,	BINOP},
	{"ge",	INTGE,	BINOP},
	{"gt",	INTGT,	BINOP},
	{"le",	INTLE,	BINOP},
	{"lt",	INTLT,	BINOP},
	{"ne",	INTNE,	BINOP},
	{"nt",	FILNT,	BINOP},
	{"ot",	FILOT,	BINOP},
};

static const struct t_op mop2[] = {
	{"G",	FILGID,	UNOP},
	{"L",	FILSYM,	UNOP},
	{"O",	FILUID,	UNOP},
	{"S",	FILSOCK,UNOP},
	{"a",	BAND,	BBINOP},
	{"b",	FILBDEV,UNOP},
	{"c",	FILCDEV,UNOP},
	{"d",	FILDIR,	UNOP},
	{"e",	FILEXIST,UNOP},
	{"f",	FILREG,	UNOP},
	{"g",	FILSGID,UNOP},
	{"h",	FILSYM,	UNOP},		/* for backwards compat */
	{"k",	FILSTCK,UNOP},
	{"n",	STRNZ,	UNOP},
	{"o",	BOR,	BBINOP},
	{"p",	FILFIFO,UNOP},
	{"r",	FILRD,	UNOP},
	{"s",	FILGZ,	UNOP},
	{"t",	FILTT,	UNOP},
	{"u",	FILSUID,UNOP},
	{"w",	FILWR,	UNOP},
	{"x",	FILEX,	UNOP},
	{"z",	STREZ,	UNOP},
};

static char **t_wp;
static struct t_op const *t_wp_op;

static void syntax(const char *, const char *);
static int oexpr(enum token);
static int aexpr(enum token);
static int nexpr(enum token);
static int primary(enum token);
static int binop(void);
static int filstat(char *, enum token);
static enum token t_lex(char *);
static int isoperand(void);
static int getn(const char *);
static int newerf(const char *, const char *);
static int olderf(const char *, const char *);
static int equalf(const char *, const char *);

#if defined(SHELL)
extern void error(const char *, ...) __attribute__((__noreturn__));
#else
static void error(const char *, ...) __attribute__((__noreturn__));

static void
error(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	verrx(2, msg, ap);
	/*NOTREACHED*/
	va_end(ap);
}
#endif

#ifdef SHELL
int testcmd(int, char **);

int
testcmd(int argc, char **argv)
#else
int main(int, char *[]);

int
main(int argc, char *argv[])
#endif
{
	int res;
	const char *argv0;

#ifdef SHELL
	argv0 = argv[0];
#else
	setprogname(argv[0]);
	argv0 = getprogname();
#endif
	if (strcmp(argv0, "[") == 0) {
		if (strcmp(argv[--argc], "]"))
			error("missing ]");
		argv[argc] = NULL;
	}

	if (argc < 2)
		return 1;

	t_wp = &argv[1];
	res = !oexpr(t_lex(*t_wp));

	if (*t_wp != NULL && *++t_wp != NULL)
		syntax(*t_wp, "unexpected operator");

	return res;
}

static void
syntax(const char *op, const char *msg)
{

	if (op && *op)
		error("%s: %s", op, msg);
	else
		error("%s", msg);
}

static int
oexpr(enum token n)
{
	int res;

	res = aexpr(n);
	if (t_lex(*++t_wp) == BOR)
		return oexpr(t_lex(*++t_wp)) || res;
	t_wp--;
	return res;
}

static int
aexpr(enum token n)
{
	int res;

	res = nexpr(n);
	if (t_lex(*++t_wp) == BAND)
		return aexpr(t_lex(*++t_wp)) && res;
	t_wp--;
	return res;
}

static int
nexpr(enum token n)
{

	if (n == UNOT)
		return !nexpr(t_lex(*++t_wp));
	return primary(n);
}

static int
primary(enum token n)
{
	enum token nn;
	int res;

	if (n == EOI)
		return 0;		/* missing expression */
	if (n == LPAREN) {
		if ((nn = t_lex(*++t_wp)) == RPAREN)
			return 0;	/* missing expression */
		res = oexpr(nn);
		if (t_lex(*++t_wp) != RPAREN)
			syntax(NULL, "closing paren expected");
		return res;
	}
	if (t_wp_op && t_wp_op->op_type == UNOP) {
		/* unary expression */
		if (*++t_wp == NULL)
			syntax(t_wp_op->op_text, "argument expected");
		switch (n) {
		case STREZ:
			return strlen(*t_wp) == 0;
		case STRNZ:
			return strlen(*t_wp) != 0;
		case FILTT:
			return isatty(getn(*t_wp));
		default:
			return filstat(*t_wp, n);
		}
	}

	if (t_lex(t_wp[1]), t_wp_op && t_wp_op->op_type == BINOP) {
		return binop();
	}	  

	return strlen(*t_wp) > 0;
}

static int
binop(void)
{
	const char *opnd1, *opnd2;
	struct t_op const *op;

	opnd1 = *t_wp;
	(void) t_lex(*++t_wp);
	op = t_wp_op;

	if ((opnd2 = *++t_wp) == NULL)
		syntax(op->op_text, "argument expected");
		
	switch (op->op_num) {
	case STREQ:
		return strcmp(opnd1, opnd2) == 0;
	case STRNE:
		return strcmp(opnd1, opnd2) != 0;
	case STRLT:
		return strcmp(opnd1, opnd2) < 0;
	case STRGT:
		return strcmp(opnd1, opnd2) > 0;
	case INTEQ:
		return getn(opnd1) == getn(opnd2);
	case INTNE:
		return getn(opnd1) != getn(opnd2);
	case INTGE:
		return getn(opnd1) >= getn(opnd2);
	case INTGT:
		return getn(opnd1) > getn(opnd2);
	case INTLE:
		return getn(opnd1) <= getn(opnd2);
	case INTLT:
		return getn(opnd1) < getn(opnd2);
	case FILNT:
		return newerf(opnd1, opnd2);
	case FILOT:
		return olderf(opnd1, opnd2);
	case FILEQ:
		return equalf(opnd1, opnd2);
	default:
		abort();
		/* NOTREACHED */
	}
}

static int
filstat(char *nm, enum token mode)
{
	struct stat s;

	if (mode == FILSYM ? lstat(nm, &s) : stat(nm, &s))
		return 0;

	switch (mode) {
	case FILRD:
		return access(nm, R_OK) == 0;
	case FILWR:
		return access(nm, W_OK) == 0;
	case FILEX:
		return access(nm, X_OK) == 0;
	case FILEXIST:
		return access(nm, F_OK) == 0;
	case FILREG:
		return S_ISREG(s.st_mode);
	case FILDIR:
		return S_ISDIR(s.st_mode);
	case FILCDEV:
		return S_ISCHR(s.st_mode);
	case FILBDEV:
		return S_ISBLK(s.st_mode);
	case FILFIFO:
		return S_ISFIFO(s.st_mode);
	case FILSOCK:
		return S_ISSOCK(s.st_mode);
	case FILSYM:
		return S_ISLNK(s.st_mode);
	case FILSUID:
		return (s.st_mode & S_ISUID) != 0;
	case FILSGID:
		return (s.st_mode & S_ISGID) != 0;
	case FILSTCK:
		return (s.st_mode & S_ISVTX) != 0;
	case FILGZ:
		return s.st_size > (off_t)0;
	case FILUID:
		return s.st_uid == geteuid();
	case FILGID:
		return s.st_gid == getegid();
	default:
		return 1;
	}
}

#define VTOC(x)	(const unsigned char *)((const struct t_op *)x)->op_text

static int
compare1(const void *va, const void *vb)
{
	const unsigned char *a = va;
	const unsigned char *b = VTOC(vb);

	return a[0] - b[0];
}

static int
compare2(const void *va, const void *vb)
{
	const unsigned char *a = va;
	const unsigned char *b = VTOC(vb);
	int z = a[0] - b[0];

	return z ? z : (a[1] - b[1]);
}

static struct t_op const *
findop(const char *s)
{
	if (s[0] == '-') {
		if (s[1] == '\0')
			return NULL;
		if (s[2] == '\0')
			return bsearch(s + 1, mop2, __arraycount(mop2),
			    sizeof(*mop2), compare1);
		else if (s[3] != '\0')
			return NULL;
		else
			return bsearch(s + 1, mop3, __arraycount(mop3),
			    sizeof(*mop3), compare2);
	} else {
		if (s[1] == '\0')
			return bsearch(s, cop, __arraycount(cop), sizeof(*cop),
			    compare1);
		else if (strcmp(s, cop2[0].op_text) == 0)
			return cop2;
		else
			return NULL;
	}
}

static enum token
t_lex(char *s)
{
	struct t_op const *op;

	if (s == NULL) {
		t_wp_op = NULL;
		return EOI;
	}

	if ((op = findop(s)) != NULL) {
		if (!((op->op_type == UNOP && isoperand()) ||
		    (op->op_num == LPAREN && *(t_wp+1) == 0))) {
			t_wp_op = op;
			return op->op_num;
		}
	}
	t_wp_op = NULL;
	return OPERAND;
}

static int
isoperand(void)
{
	struct t_op const *op;
	char *s, *t;

	if ((s  = *(t_wp+1)) == 0)
		return 1;
	if ((t = *(t_wp+2)) == 0)
		return 0;
	if ((op = findop(s)) != NULL)
		return op->op_type == BINOP && (t[0] != ')' || t[1] != '\0'); 
	return 0;
}

/* atoi with error detection */
static int
getn(const char *s)
{
	char *p;
	long r;

	errno = 0;
	r = strtol(s, &p, 10);

	if (errno != 0)
	      error("%s: out of range", s);

	while (isspace((unsigned char)*p))
	      p++;
	
	if (*p)
	      error("%s: bad number", s);

	return (int) r;
}

static int
newerf(const char *f1, const char *f2)
{
	struct stat b1, b2;

	return (stat(f1, &b1) == 0 &&
		stat(f2, &b2) == 0 &&
		b1.st_mtime > b2.st_mtime);
}

static int
olderf(const char *f1, const char *f2)
{
	struct stat b1, b2;

	return (stat(f1, &b1) == 0 &&
		stat(f2, &b2) == 0 &&
		b1.st_mtime < b2.st_mtime);
}

static int
equalf(const char *f1, const char *f2)
{
	struct stat b1, b2;

	return (stat(f1, &b1) == 0 &&
		stat(f2, &b2) == 0 &&
		b1.st_dev == b2.st_dev &&
		b1.st_ino == b2.st_ino);
}
