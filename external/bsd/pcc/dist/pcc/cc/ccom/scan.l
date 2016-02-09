%{
/*	Id: scan.l,v 1.145 2016/02/07 18:04:28 ragge Exp 	*/	
/*	$NetBSD: scan.l,v 1.6 2016/02/09 20:37:32 plunky Exp $	*/

/*
 * Copyright (c) 2002 Anders Magnusson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
%}


B			[0-1]
D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
P			[Pp][+-]?{D}+
FS			((f|F|l|L)?i|i?(f|F|l|L))
IS			(u|U|l|L|i)*
UL			({L}|[\x80-\xFF])
UC			(L|u|U)
US			(L|u|U|u8)

%{
#include <stdlib.h>
#include <errno.h>  
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "pass1.h"
#include "cgram.h"
#include "unicode.h"

#define	bdebug flexbdebug

static P1ND *cvtdig(int radix);
static P1ND *charcon(void);
static P1ND *wcharcon(void);
static void control(int);
static void pragma(void);
int notype, parbal, inattr, parlvl, nodinit, inoso;
int kwdecode(struct symtab *s);

#define	CPP_IDENT 	2
#define	CPP_LINE 	3
#define	CPP_HASH	4

#ifdef STABS
#define	STABS_LINE(x) if (gflag && cftnsp) stabs_line(x)
#else
#define STABS_LINE(x)
#endif
#if defined(FLEX_SCANNER) && YY_FLEX_SUBMINOR_VERSION == 31
/* Hack to avoid unnecessary warnings */
FILE *yyget_in  (void);
FILE *yyget_out  (void);
int yyget_leng  (void);
char *yyget_text  (void);
void yyset_in (FILE *);
void yyset_out (FILE *);
int yyget_debug  (void);
void yyset_debug (int);
int yylex_destroy  (void);
extern int yyget_lineno (void);
extern void yyset_lineno (int);
#endif

%}

%%

{UL}({UL}|{D})*	{ 	struct symtab *s;
			int i = 0;

			yylval.strp = addname(yytext);
			if ((s = lookup(yylval.strp, SNOCREAT)) != NULL &&
			    s->sclass == KEYWORD)
				return kwdecode(s);

#ifdef GCC_COMPAT
			if (doing_init && nodinit == 0) {
				/* check for name: for old gcc compat */
				while ((i = input()) == ' ' || i == '\t')
					;
				if (i == ':')
					return(GCC_DESIG);
				unput(i);
			}
			if ((i = gcc_keyword(yylval.strp)) > 0) {
				if (i == PCC_OFFSETOF)
					inoso = 1;
				return i;
			}
#endif
			if (i == 0) {
				if (notype)
					return(C_NAME);
				return s && s->sclass == TYPEDEF ?
				    notype=1, C_TYPENAME : C_NAME;
			}
		}

0[xX]{H}+{IS}?		{ yylval.nodep = cvtdig(16); return(C_ICON); }
0{D}+{IS}?		{ yylval.nodep = cvtdig(8); return(C_ICON); }
0[bB]{B}+{IS}?		{ yylval.nodep = cvtdig(2); return(C_ICON); }
{D}+{IS}?		{ yylval.nodep = cvtdig(10); return(C_ICON); }
{UC}'(\\.|[^\\'])*'	{ yylval.nodep = wcharcon(); return(C_ICON); }
'(\\.|[^\\'])*'		{ yylval.nodep = charcon(); return(C_ICON); }

{D}+{E}{FS}?		{ yylval.flt = floatcon(yytext); return(C_FCON); }
{D}*"."{D}+({E})?{FS}?	{ yylval.flt = floatcon(yytext); return(C_FCON); }
{D}+"."{D}*({E})?{FS}?	{ yylval.flt = floatcon(yytext); return(C_FCON); }
0[xX]{H}*"."{H}+{P}{FS}? { yylval.flt = fhexcon(yytext); return(C_FCON); }
0[xX]{H}+"."{P}{FS}?	{ yylval.flt = fhexcon(yytext); return(C_FCON); }
0[xX]{H}+{P}{FS}?	{ yylval.flt = fhexcon(yytext); return(C_FCON); }

{US}?\"(\\.|[^\\"])*\"	{ yylval.strp = yytext; return C_STRING; }

"..."			{ return(C_ELLIPSIS); }
">>="			{ yylval.intval = RSEQ; return(C_ASOP); }
"<<="			{ yylval.intval = LSEQ; return(C_ASOP); }
"+="			{ yylval.intval = PLUSEQ; return(C_ASOP); }
"-="			{ yylval.intval = MINUSEQ; return(C_ASOP); }
"*="			{ yylval.intval = MULEQ; return(C_ASOP); }
"/="			{ yylval.intval = DIVEQ; return(C_ASOP); }
"%="			{ yylval.intval = MODEQ; return(C_ASOP); }
"&="			{ yylval.intval = ANDEQ; return(C_ASOP); }
"^="			{ yylval.intval = EREQ; return(C_ASOP); }
"|="			{ yylval.intval = OREQ; return(C_ASOP); }
">>"			{ yylval.intval = RS; return(C_SHIFTOP); }
"<<"			{ yylval.intval = LS; return(C_SHIFTOP); }
"++"			{ yylval.intval = INCR; return(C_INCOP); }
"--"			{ yylval.intval = DECR; return(C_INCOP); }
"->"			{ yylval.intval = STREF; return(C_STROP); }
"&&"			{ yylval.intval = ANDAND; return(C_ANDAND); }
"||"			{ yylval.intval = OROR; return(C_OROR); }
"<="			{ yylval.intval = LE; return(C_RELOP); }
">="			{ yylval.intval = GE; return(C_RELOP); }
"=="			{ yylval.intval = EQ; return(C_EQUOP); }
"!="			{ yylval.intval = NE; return(C_EQUOP); }
";"			{ notype = 0; return(';'); }
("{"|"<%")		{ notype = 0; return('{'); }
("}"|"%>")		{ if (rpole) notype = 1; return('}'); }
","			{ if (parbal && !inoso) notype = 0;
				if (parbal == 0) notype = 1; return(','); }
":"			{ if (doing_init) nodinit--; return(':'); }
"="			{ return('='); }
"("			{ parbal++; notype = 0; return('('); }
")"			{	parbal--;
				inoso = 0;
				if (parbal==0) { notype = 0; }
				if (inattr && parlvl == parbal)
					inattr = 0;
				return(')'); }
("["|"<:")		{ return('['); }
("]"|":>")		{ return(']'); }
"."			{ yylval.intval = DOT; return(C_STROP); }
"&"			{ return('&'); }
"!"			{ yylval.intval = NOT; return(C_UNOP); }
"~"			{ yylval.intval = COMPL; return(C_UNOP); }
"-"			{ return('-'); }
"+"			{ return('+'); }
"*"			{ if (parbal && notype == 0) notype = 1; return('*'); }
"/"			{ yylval.intval = DIV; return(C_DIVOP); }
"%"			{ yylval.intval = MOD; return(C_DIVOP); }
"<"			{ yylval.intval = LT; return(C_RELOP); }
">"			{ yylval.intval = GT; return(C_RELOP); }
"^"			{ return('^'); }
"|"			{ return('|'); }
"?"			{ if (doing_init) nodinit++; return('?'); }
^#pragma[ \t].*		{ pragma(); }
^#ident[ \t].*		{ control(CPP_IDENT); }
^#line[ \t].*		{ control(CPP_LINE); }
^#.*			{ control(CPP_HASH); }

[ \t\v\f]		{ }
"\n"			{ ++lineno; STABS_LINE(lineno); }
.			{ /* ignore bad characters */ }

%%

int lineno, issyshdr;

int
yywrap(void)
{
	if (0) unput(0); /* quiet gcc */
	return(1);
}

#define	DEFKW	500
#define	KWFUNC	501
#define	KWNOT	502

struct keywords {
	char *name;
	int cword, wclass;
} keywords[] = {
	{ "__func__", 0, KWFUNC },
	{ "_Alignas", C_ALIGNAS, DEFKW },
	{ "_Alignof", C_ALIGNOF, DEFKW },
	{ "_Atomic", C_ATOMIC, DEFKW },
	{ "asm", C_ASM, DEFKW },
	{ "auto", AUTO, C_CLASS },
	{ "_Bool", BOOL, C_TYPE },
	{ "break", C_BREAK, DEFKW },
	{ "case", C_CASE, DEFKW },
	{ "char", CHAR, C_TYPE },
	{ "continue", C_CONTINUE, DEFKW },
	{ "_Complex", COMPLEX, C_TYPE },
	{ "const", CON, C_QUALIFIER },
	{ "default", C_DEFAULT, DEFKW },
	{ "do", C_DO, DEFKW },
	{ "double", DOUBLE, C_TYPE },
	{ "else", C_ELSE, DEFKW },
	{ "enum", C_ENUM, KWNOT },
	{ "extern", EXTERN, C_CLASS },
	{ "float", FLOAT, C_TYPE },
	{ "for", C_FOR, DEFKW },
	{ "_Generic", C_GENERIC, DEFKW },
	{ "goto", C_GOTO, KWNOT },
	{ "if", C_IF, DEFKW },
	{ "_Imaginary", IMAG, C_TYPE },
	{ "inline", INLINE, C_FUNSPEC },
	{ "int", INT, C_TYPE },
	{ "long", LONG, C_TYPE },
	{ "_Noreturn", NORETURN, C_FUNSPEC },
	{ "register", REGISTER, C_CLASS },
	{ "restrict", 0, C_QUALIFIER },
	{ "return", C_RETURN, DEFKW },
	{ "short", SHORT, C_TYPE },
	{ "signed", SIGNED, C_TYPE },
	{ "sizeof", C_SIZEOF, DEFKW },
	{ "static", STATIC, C_CLASS },
	{ "_Static_assert", C_STATICASSERT, DEFKW },
	{ "struct", STNAME, C_STRUCT },
	{ "switch", C_SWITCH, DEFKW },
	{ "_Thread_local", THLOCAL, C_CLASS },
	{ "typedef", TYPEDEF, C_CLASS },
	{ "union", UNAME, C_STRUCT },
	{ "unsigned", UNSIGNED, C_TYPE },
	{ "void", VOID, C_TYPE },
	{ "volatile", VOL, C_QUALIFIER },
	{ "while", C_WHILE, DEFKW },
};
	

void
kwinit(void)
{
	struct symtab *s;
	int i, n = sizeof(keywords) / sizeof(keywords[0]);

	for (i = 0; i < n; i++) {
		s = lookup(addname(keywords[i].name), 0);
		s->sclass = KEYWORD;
		s->soffset = i;
	}
}

int
kwdecode(struct symtab *s)
{
	struct keywords *kw = &keywords[s->soffset];

	if (inattr && kw->cword != C_SIZEOF)
		return C_NAME;

	switch (kw->wclass) {
	case C_TYPE:
		yylval.type = kw->cword;
		notype=1;
		break;

	case C_CLASS:
		if (kw->cword == THLOCAL)
			uerror("_Thread_local not supported");
		yylval.type = kw->cword;
		break;

	case C_QUALIFIER:
		yylval.type = kw->cword;
		break;

	case C_FUNSPEC:
		yylval.type = kw->cword;
		break;

	case DEFKW:
		return kw->cword;

	case KWFUNC:
		if (cftnsp == NULL)
			uerror("__func__ outside function");
		yylval.strp = cftnsp->sname;
		return(C_STRING);

	case KWNOT:
		notype = 1;
		return kw->cword;

	case C_STRUCT:
		notype = 1;
		yylval.intval = kw->cword;
		break;

	default:
		cerror("keyword %s not found", kw->name);
	}
	return kw->wclass;
}

static long double
typround(long double dc, char *e, TWORD *tw)
{
	int im = 0;

	*tw = DOUBLE;
	for (; *e; e++) {
		switch (*e) {
		case 'f':
		case 'F':
			*tw = FLOAT;
			dc = (float)dc;
			break;
		case 'l':
		case 'L':
			*tw = ctype(LDOUBLE);
			break;
		case 'i':
		case 'I':
			im = 1;
			break;
		}
	}
	if (*tw == DOUBLE)
		dc = (double)dc;
#ifndef NO_COMPLEX
	if (im)
		*tw += (FIMAG-FLOAT);
#endif
	return dc;
}

/*
 * Return which type current fp string has.
 */
static TWORD
endtyp(char *s)
{
	TWORD tw = DOUBLE;

	for (; *s; s++)
		;
	s--;
	if (*s == 'i' || *s == 'I')
		tw += (FIMAG-FLOAT), s--;
	if (*s == 'f' || *s == 'F')
		tw--, s--;
	else if (*s == 'l' || *s == 'L')
		tw++, s--;
	if ((*s == 'i' || *s == 'I') && ISFTY(tw))
		tw += (FIMAG-FLOAT), s--;
/* XXX complain */
	return tw;
}

#ifdef NATIVE_FLOATING_POINT
/*
 * Convert floating point string to union flt.
 */
static union flt *
strtoflt(char *str)
{
	union flt *flt;
	long double dc;

#ifdef HAVE_STRTOLD
	dc = strtold(str, NULL);
#else
	dc = strtod(str, NULL);
#endif

	flt = stmtalloc(sizeof(union flt));
	flt->fp = dc;
	return flt;
}
#endif

union flt *
floatcon(char *s)
{
	union flt *flt;
	TWORD tw;

	flt = strtoflt(s);			/* parse number */
	tw = flt->fa[FP_TOP] = endtyp(s);	/* parse type */
	if (ISITY(tw))
		tw -= (FIMAG-FLOAT);
	FLOAT_FP2FP(flt, tw);			/* cast accordingly */
	return flt;
}

static int
h2n(int ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return ch - 'A' + 10;
	
}

/*
 * Get exponent.  Max size is 16 bits.
 */
static int
fdecl(char *c, char **ep)
{
	int minus = 0;
	int val = 0;

	if (*c == '+')
		c++;
	else if (*c == '-')
		minus = 1, c++;

	while (isdigit((int)*c)) {
		val *= 10;
		val += *c++ - '0';
		if (val > 32769)
			val = 32769;
	}
	if (minus)
		val = -val;
	*ep = c;
	return val;
}

union flt *
fhexcon(char *c)
{
	union flt *flt;
	TWORD tw;
	char *ep;
	long double d;
	int i, ed;

	d = 0.0;
	ed = 0;
	c+= 2; /* skip 0x */
#define FSET(n) { d *= 2; if (i & n) d += 1.0; }
	for (; *c != '.' && *c != 'p' && *c != 'P'; c++) {
		i = h2n(*c);
		FSET(8); FSET(4); FSET(2); FSET(1);
	}
	if (*c != '.' && *c != 'p' && *c != 'P')
		cerror("fhexcon");
	if (*c == '.') {
		c++;
		for (; *c != 'p' && *c != 'P'; c++) {
			i = h2n(*c);
			FSET(8); FSET(4); FSET(2); FSET(1);
			ed -= 4;
		}
	}
	if (*c != 'P' && *c != 'p')
		cerror("fhexcon2");
	c++;
	ed += fdecl(c, &ep);

	/* avoid looping in vain. Idea from Fred J. Tydeman */
	if (ed > 32769) ed = 32769;
	if (ed < -32769) ed = -32769;

	while (ed > 0)
		d *= 2, ed--;
	while (ed < 0)
		d /= 2, ed++;
	d = typround(d, ep, &tw);
	flt = stmtalloc(sizeof(union flt));
	flt->fp = d;
	flt->fa[FP_TOP] = tw;
	return flt;
}

P1ND *
cvtdig(int radix)
{
	P1ND *p;
	TWORD ntype;
	unsigned long long v;
	char *ch = yytext;
	int n, numl, numu;

	if (radix == 16 || radix == 2)
		ch += 2; /* Skip 0x or 0b */

	v = 0;
	while ((*ch >= '0' && *ch <= '9') || (*ch >= 'a' && *ch <= 'f') ||
	    (*ch >= 'A' && *ch <= 'F')) {
		v *= radix;
		n = *ch;
		n = (n <= '9' ? n - '0' : (n > 'F' ? n - 'a' : n - 'A') + 10);
		ch++;
		v += n;
	}
	/* Parse trailing chars */
	ntype = INT;
	numl = numu = 0;
	for (n = 0; n < 3; n++) {
		if (*ch == 0)
			break;
		if ((*ch == 'l' || *ch == 'L') && numl < 2)
			ntype+=2, numl++;
		else if ((*ch == 'u' || *ch == 'U') && numu < 1)
			ntype = ENUNSIGN(ntype), numu++;
		else if (*ch == 'i')
			ntype = DOUBLE;
		else
			break;
		ch++;
	}
	if (*ch)
		uerror("constant has too many '%c'", *ch);

	switch (ntype) {
	case DOUBLE: /* special case */
		p = block(FCON, NULL, NULL, IMAG, 0, 0);
		p->n_dcon = stmtalloc(sizeof(union flt));
		FLOAT_INT2FP(p->n_dcon, v, ULONGLONG);
		FLOAT_FP2FP(p->n_dcon, DOUBLE);
		return p;

	case INT:
	case LONG:
	case LONGLONG:
		if (radix == 10) {
			if (ntype == LONGLONG)
				break;
			if (v > MAX_LONG)
				ntype = LONGLONG;
			else if (v > MAX_INT)
				ntype = LONG;
		} else {
			if (v > MAX_LONGLONG) {
				ntype = ULONGLONG;
			} else if (v > MAX_ULONG) {
				if (ntype < LONGLONG)
					ntype = LONGLONG;
			} else if (v > MAX_LONG) {
				if (ntype < ULONG)
					ntype = ULONG;
			} else if (v > MAX_UNSIGNED) {
				if (ntype < LONG)
					ntype = LONG;
			} else if (v > MAX_INT) {
				if (ntype < UNSIGNED)
					ntype = UNSIGNED;
			}
		}
		break;
	case UNSIGNED:
	case ULONG:
		if (v > MAX_ULONG) {
			ntype = ULONGLONG;
		} else if (v > MAX_UNSIGNED)
			ntype = ULONG;
		break;	
	}	

	ntype = ctype(ntype);
	p = xbcon(v, NULL, ntype);
	ASGLVAL(p->n_slval, v);

	return p;
}

/*
 * return value of escaped character constant
 */
unsigned int
esccon(char **sptr)
{
	unsigned int val;
	char *wr = *sptr;
	char c;

	wr++;	/* skip \ */
	switch (c = *wr++) {
	case 'a': val = '\a'; break;
	case 'b': val = '\b'; break;
#ifdef GCC_COMPAT
	case 'e': val = '\033'; break;
#endif
	case 'f': val = '\f'; break;
	case 'n': val = '\n'; break;
	case 'r': val = '\r'; break;
	case 't': val = '\t'; break;
	case 'v': val = '\v'; break;

	case '\"': val = '\"'; break;
	case '\'': val = '\''; break;
	case '\?': val = '\?'; break;
	case '\\': val = '\\'; break;

	case 'x':
		val = 0;
		for (;;) {
			c = *wr;
			if (c >= '0' && c <= '9')
				c = c - '0';
			else if (c >= 'a' && c <= 'f')
				c = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				c = c - 'A' + 10;
			else
				break;

			val = (val << 4) + c;
			wr++;
		}
		break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		val = (c - '0');
		c = *wr;
		if (c >= '0' && c <= '7') {
			wr++;
			val = (val << 3) + (c - '0');
			c = *wr;
			if (c >= '0' && c <= '7') {
				wr++;
				val = (val << 3) + (c - '0');
			}
		}
		break;

	default:
		werror("unknown escape sequence \\%c", c);
		val = c;
		break;
	}

	*sptr = wr;
	return val;
}

/*
 * Convert (one) character constant to an int.
 * Handle as unsigned except if only one character and char is signed.
 */
P1ND *
charcon(void)
{
	unsigned int val = 0, i = 0;
	char *pp = yytext;

	pp++;	/* skip ' */
	while (*pp != '\'') {
		val <<= SZCHAR; /* XXX big endian? */
		if (*pp == '\\')
			val |= esccon(&pp);
		else
			val |= (*pp++ & ((1 << SZCHAR)-1));
		i++;
	}

	if (i == 0)
		uerror("empty character constant");
	else if (i == 1 && xuchar == 0)
		val = (signed char)val; /* XXX other sizes of char? */
	else if (i > 1)
		werror("too many characters in character constant");

	return bcon(val);
}

/*
 * Convert a wide-character constant to an unsigned int
 */
P1ND *
wcharcon(void)
{
	unsigned int val = 0, i = 0;
	char *pp = yytext;

	pp++;	/* skip L */
	pp++;	/* skip ' */
	while (*pp != '\'') {
		/*
		 * although u82cp() does handle escaped values, we deal
		 * with them directly since otherwise you can't process
		 * values which might be valid utf8 prefix
		 */
		if (*pp == '\\')
			val = esccon(&pp);
		else
			val = u82cp(&pp);

		i++;
	}

	if (i == 0)
		uerror("empty wide-character constant");
	else if (i > 1)
		werror("too many characters in wide-character constant");

	return xbcon(val, NULL, ctype(UNSIGNED));
}

void
control(int t)
{
	char *wr = yytext;
	char *eptr;
	int val;

	wr++;	/* Skip initial '#' */
	switch (t) {
	case CPP_IDENT:
		return;	/* Just skip these for now. */

	case CPP_LINE:
		wr += 4;
		/* FALLTHROUGH */
	case CPP_HASH:
		val = strtol(wr, &eptr, 10);
		if (wr == eptr)	/* Illegal string */
			goto bad;
		wr = eptr;
		lineno = val - 1;
		while (*wr && *wr != '\"')
			wr++;
		if (*wr == 0)
			return;
		if (*wr++ != '\"')
			goto bad;
		eptr = wr;
		while (*wr && *wr != '\"')
			wr++;
		if (*wr != '\"')
			goto bad;
		*wr++ = 0;
		free(ftitle);
		ftitle = xstrdup(eptr);
		while (*wr == ' ')
			wr++;
		issyshdr = 0;
		if (*wr == '3')
			issyshdr = 1;
#ifdef STABS
		if (gflag)
			stabs_file(ftitle);
#endif
	}
	return;
bad:
	werror("%s: illegal control", yytext);
}

int pragma_allpacked;
int pragma_packed, pragma_aligned;

static int
pragmas_weak(char *str)
{
	struct symtab *sp;
	char *s1, *s2;

	if ((s1 = pragtok(NULL)) == NULL)
		return 1;
	if ((s2 = pragtok(NULL)) == NULL) {
		sp = lookup(addname(s1), SNORMAL);
#ifdef GCC_COMPAT
		sp->sap = attr_add(sp->sap, gcc_attr_parse(bdty(NAME, "weak")));
#else
		sp->sap = NULL;
#endif
	} else if (*s2 == '=') {
		if ((s2 = pragtok(NULL)) == NULL)
			return 1;
		sp = lookup(addname(s2), SNORMAL);
#ifdef GCC_COMPAT
		sp->sap = attr_add(sp->sap, gcc_attr_parse(bdty(CALL,
		    bdty(NAME, "aliasweak"), bdty(STRING, s1, 0))));
#else
		sp->sap = NULL;
#endif
	} else
		return 1;
	return 0;
}

char *pragstore;

/* trivial tokenizer for pragmas */
#define ps pragstore
char *
pragtok(char *sin)
{
	static char ss[2];
	char *rv;

	if (sin)
		ps = sin;

	for (; isspace((int)*ps); ps++)
		;
	if (*ps == 0)
		return NULL;
	for (rv = ps; isalpha((int)*ps) || isdigit((int)*ps) || *ps == '_'; ps++)
		;
	ss[0] = *ps;
	if (rv == ps) {
		rv = ss, ps++;
	} else {
		*ps = 0;
		rv = tmpstrdup(rv);
		*ps = ss[0];
	}
	return rv;
}

/* return 1 on error */
int
eat(int ch)
{
	char *s = pragtok(0);
	return (s == 0 || *s != ch);
}

static int
pragmas_alpack(char *t)
{
	char *s;
	int ap;

	ap = (s = pragtok(0)) ? atoi(s) : 1;
	if (strcmp(t, "packed") == 0)
		pragma_packed = ap;
	else
		pragma_aligned = ap;
	return 0;
}


/*
 * Packing control.
 */
static int
pragmas_pack(char *t)
{
#define	PACKSTKSZ 10
	static int packstk[PACKSTKSZ], packptr;
	char *s;

	if (eat('('))
		return 1;
	s = pragtok(0);
	if (*s == ')')
		return pragma_allpacked = 0;
	if (strcmp(s, "push") == 0) {
		if (packptr == PACKSTKSZ)
			uerror("too many push");
		packstk[packptr++] = pragma_allpacked;
		s = pragtok(0);
		if (*s == ')')
			return 0;
		if (*s != ',')
			return 1;
		s = pragtok(0);
	} else if (strcmp(s, "pop") == 0) {
		if (packptr == 0)
			uerror("stack empty");
		pragma_allpacked = packstk[--packptr];
		return eat(')');
	}

	if (*s < '0' || *s > '9') /* no number */
		return 1;
	pragma_allpacked = atoi(s);
	return eat(')');
}

static int      
pragmas_unsupp(char *t) 
{ 
	werror("#pragma %s unsupported", t);
	return 0; /* Just ignore */
}

static int
pragmas_stdc(char *t)
{
	return 0; /* Just ignore */
}

struct pragmas {
	char *name;
	int (*fun)(char *);
} pragmas[] = {
	{ "pack", pragmas_pack },
	{ "packed", pragmas_alpack },
	{ "aligned", pragmas_alpack },
	{ "rename", pragmas_unsupp },
#ifdef GCC_COMPAT
	{ "GCC", pragmas_gcc },
#endif
	{ "STDC", pragmas_stdc },
	{ "weak", pragmas_weak },
	{ "ident", NULL },
	{ 0 },
};

/*
 * got a full pragma line.  Split it up here.
 */
static void
pragma(void)
{
	struct pragmas *p;
	char *t, *pt;

	if ((t = pragtok(&yytext[7])) != NULL) {
		pt = ps;
		for (p = pragmas; p->name; p++) {
			if (strcmp(t, p->name) == 0) {
				if (p->fun && (*p->fun)(t))
					uerror("bad argument to #pragma");
				return;
			}
		}
		ps = pt;
		if (mypragma(t))
			return;
	}
	warner(Wunknown_pragmas, t, ps);
}

void
cunput(char c)
{
	unput(c);
}
