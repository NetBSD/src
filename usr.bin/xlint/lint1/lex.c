/* $NetBSD: lex.c,v 1.118 2022/04/09 23:41:22 rillig Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All Rights Reserved.
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *    must display the following acknowledgement:
 *      This product includes software developed by Jochen Pohl for
 *      The NetBSD Project.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: lex.c,v 1.118 2022/04/09 23:41:22 rillig Exp $");
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"
#include "cgram.h"

#define CHAR_MASK	((1U << CHAR_SIZE) - 1)


/* Current position (it's also updated when an included file is parsed) */
pos_t	curr_pos = { "", 1, 0 };

/*
 * Current position in C source (not updated when an included file is
 * parsed).
 */
pos_t	csrc_pos = { "", 1, 0 };

bool in_gcc_attribute;
bool in_system_header;

#define kwdef(name, token, scl, tspec, tqual,	c90, c99, gcc, attr, deco) \
	{ \
		name, token, scl, tspec, tqual, \
		(c90) > 0, (c99) > 0, (gcc) > 0, (attr) > 0, \
		((deco) & 1) != 0, ((deco) & 2) != 0, ((deco) & 4) != 0, \
	}
#define kwdef_token(name, token,		c90, c99, gcc, attr, deco) \
	kwdef(name, token, 0, 0, 0,		c90, c99, gcc, attr, deco)
#define kwdef_sclass(name, sclass,		c90, c99, gcc, attr, deco) \
	kwdef(name, T_SCLASS, sclass, 0, 0,	c90, c99, gcc, attr, deco)
#define kwdef_type(name, tspec,			c90, c99, gcc, attr, deco) \
	kwdef(name, T_TYPE, 0, tspec, 0,	c90, c99, gcc, attr, deco)
#define kwdef_tqual(name, tqual,		c90, c99, gcc, attr, deco) \
	kwdef(name, T_QUAL, 0, 0, tqual,	c90, c99, gcc, attr, deco)
#define kwdef_keyword(name, token) \
	kwdef(name, token, 0, 0, 0,		0, 0, 0, 0, 1)
#define kwdef_gcc_attr(name, token) \
	kwdef(name, token, 0, 0, 0,		0, 0, 1, 1, 5)

/* During initialization, these keywords are written to the symbol table. */
static const struct keyword {
	const	char *kw_name;	/* keyword */
	int	kw_token;	/* token returned by yylex() */
	scl_t	kw_scl;		/* storage class if kw_token T_SCLASS */
	tspec_t	kw_tspec;	/* type spec. if kw_token
				 * T_TYPE or T_STRUCT_OR_UNION */
	tqual_t	kw_tqual;	/* type qual. if kw_token T_QUAL */
	bool	kw_c90:1;	/* C90 keyword */
	bool	kw_c99:1;	/* C99 keyword */
	bool	kw_gcc:1;	/* GCC keyword */
	bool	kw_attr:1;	/* GCC attribute, keyword */
	bool	kw_plain:1;	/* 'name' */
	bool	kw_leading:1;	/* '__name' */
	bool	kw_both:1;	/* '__name__' */
} keywords[] = {
	kwdef_gcc_attr(	"alias",	T_AT_ALIAS),
	kwdef_keyword(	"_Alignas",	T_ALIGNAS),
	kwdef_keyword(	"_Alignof",	T_ALIGNOF),
	kwdef_gcc_attr(	"aligned",	T_AT_ALIGNED),
	kwdef_token(	"__alignof__",	T_ALIGNOF,		0,0,0,0,1),
	kwdef_gcc_attr(	"alloc_size",	T_AT_ALLOC_SIZE),
	kwdef_gcc_attr(	"always_inline",T_AT_ALWAYS_INLINE),
	kwdef_token(	"asm",		T_ASM,			0,0,1,0,7),
	kwdef_token(	"attribute",	T_ATTRIBUTE,		0,0,1,0,6),
	kwdef_sclass(	"auto",		AUTO,			0,0,0,0,1),
	kwdef_type(	"_Bool",	BOOL,			0,1,0,0,1),
	kwdef_gcc_attr(	"bounded",	T_AT_BOUNDED),
	kwdef_keyword(	"break",	T_BREAK),
	kwdef_gcc_attr(	"buffer",	T_AT_BUFFER),
	kwdef_token(	"__builtin_offsetof", T_BUILTIN_OFFSETOF, 0,0,1,0,1),
	kwdef_keyword(	"case",		T_CASE),
	kwdef_type(	"char",		CHAR,			0,0,0,0,1),
	kwdef_gcc_attr(	"cold",		T_AT_COLD),
	kwdef_gcc_attr(	"common",	T_AT_COMMON),
	kwdef_type(	"_Complex",	COMPLEX,		0,1,0,0,1),
	kwdef_tqual(	"const",	CONST,			1,0,0,0,7),
	kwdef_gcc_attr(	"constructor",	T_AT_CONSTRUCTOR),
	kwdef_keyword(	"continue",	T_CONTINUE),
	kwdef_keyword(	"default",	T_DEFAULT),
	kwdef_gcc_attr(	"deprecated",	T_AT_DEPRECATED),
	kwdef_gcc_attr(	"destructor",	T_AT_DESTRUCTOR),
	kwdef_gcc_attr(	"disable_sanitizer_instrumentation",
	    T_AT_DISABLE_SANITIZER_INSTRUMENTATION),
	kwdef_keyword(	"do",		T_DO),
	kwdef_type(	"double",	DOUBLE,			0,0,0,0,1),
	kwdef_keyword(	"else",		T_ELSE),
	kwdef_keyword(	"enum",		T_ENUM),
	kwdef_token(	"__extension__",T_EXTENSION,		0,0,1,0,1),
	kwdef_sclass(	"extern",	EXTERN,			0,0,0,0,1),
	kwdef_gcc_attr(	"fallthrough",	T_AT_FALLTHROUGH),
	kwdef_type(	"float",	FLOAT,			0,0,0,0,1),
	kwdef_keyword(	"for",		T_FOR),
	kwdef_gcc_attr(	"format",	T_AT_FORMAT),
	kwdef_gcc_attr(	"format_arg",	T_AT_FORMAT_ARG),
	kwdef_token(	"_Generic",	T_GENERIC,		0,1,0,0,1),
	kwdef_gcc_attr(	"gnu_inline",	T_AT_GNU_INLINE),
	kwdef_gcc_attr(	"gnu_printf",	T_AT_FORMAT_GNU_PRINTF),
	kwdef_keyword(	"goto",		T_GOTO),
	kwdef_gcc_attr(	"hot",		T_AT_HOT),
	kwdef_keyword(	"if",		T_IF),
	kwdef_token(	"__imag__",	T_IMAG,			0,0,1,0,1),
	kwdef_sclass(	"inline",	INLINE,			0,1,0,0,7),
	kwdef_type(	"int",		INT,			0,0,0,0,1),
#ifdef INT128_SIZE
	kwdef_type(	"__int128_t",	INT128,			0,1,0,0,1),
#endif
	kwdef_type(	"long",		LONG,			0,0,0,0,1),
	kwdef_gcc_attr(	"malloc",	T_AT_MALLOC),
	kwdef_gcc_attr(	"may_alias",	T_AT_MAY_ALIAS),
	kwdef_gcc_attr(	"minbytes",	T_AT_MINBYTES),
	kwdef_gcc_attr(	"mode",		T_AT_MODE),
	kwdef_gcc_attr("no_instrument_function",
					T_AT_NO_INSTRUMENT_FUNCTION),
	kwdef_gcc_attr(	"no_sanitize",	T_AT_NO_SANITIZE),
	kwdef_gcc_attr(	"no_sanitize_thread",	T_AT_NO_SANITIZE_THREAD),
	kwdef_gcc_attr(	"noinline",	T_AT_NOINLINE),
	kwdef_gcc_attr(	"nonnull",	T_AT_NONNULL),
	kwdef_gcc_attr(	"nonstring",	T_AT_NONSTRING),
	kwdef_token(	"_Noreturn",	T_NORETURN,		0,1,0,0,1),
	kwdef_gcc_attr(	"noreturn",	T_AT_NORETURN),
	kwdef_gcc_attr(	"nothrow",	T_AT_NOTHROW),
	kwdef_gcc_attr(	"optimize",	T_AT_OPTIMIZE),
	kwdef_gcc_attr(	"optnone",	T_AT_OPTNONE),
	kwdef_gcc_attr(	"packed",	T_AT_PACKED),
	kwdef_token(	"__packed",	T_PACKED,		0,0,0,0,1),
	kwdef_gcc_attr(	"pcs",		T_AT_PCS),
	kwdef_gcc_attr(	"printf",	T_AT_FORMAT_PRINTF),
	kwdef_gcc_attr(	"pure",		T_AT_PURE),
	kwdef_token(	"__real__",	T_REAL,			0,0,1,0,1),
	kwdef_sclass(	"register",	REG,			0,0,0,0,1),
	kwdef_gcc_attr(	"regparm",	T_AT_REGPARM),
	kwdef_tqual(	"restrict",	RESTRICT,		0,1,0,0,7),
	kwdef_keyword(	"return",	T_RETURN),
	kwdef_gcc_attr(	"returns_nonnull",T_AT_RETURNS_NONNULL),
	kwdef_gcc_attr(	"returns_twice",T_AT_RETURNS_TWICE),
	kwdef_gcc_attr(	"scanf",	T_AT_FORMAT_SCANF),
	kwdef_token(	"section",	T_AT_SECTION,		0,0,1,1,7),
	kwdef_gcc_attr(	"sentinel",	T_AT_SENTINEL),
	kwdef_type(	"short",	SHORT,			0,0,0,0,1),
	kwdef_type(	"signed",	SIGNED,			1,0,0,0,3),
	kwdef_keyword(	"sizeof",	T_SIZEOF),
	kwdef_sclass(	"static",	STATIC,			0,0,0,0,1),
	kwdef_keyword(	"_Static_assert",	T_STATIC_ASSERT),
	kwdef_gcc_attr(	"strfmon",	T_AT_FORMAT_STRFMON),
	kwdef_gcc_attr(	"strftime",	T_AT_FORMAT_STRFTIME),
	kwdef_gcc_attr(	"string",	T_AT_STRING),
	kwdef("struct",	T_STRUCT_OR_UNION, 0,	STRUCT,	0,	0,0,0,0,1),
	kwdef_keyword(	"switch",	T_SWITCH),
	kwdef_token(	"__symbolrename",	T_SYMBOLRENAME,	0,0,0,0,1),
	kwdef_gcc_attr(	"syslog",	T_AT_FORMAT_SYSLOG),
	kwdef_gcc_attr(	"target",	T_AT_TARGET),
	kwdef_tqual(	"__thread",	THREAD,			0,0,1,0,1),
	kwdef_tqual(	"_Thread_local", THREAD,		0,1,0,0,1),
	kwdef_gcc_attr(	"tls_model",	T_AT_TLS_MODEL),
	kwdef_gcc_attr(	"transparent_union", T_AT_TUNION),
	kwdef_sclass(	"typedef",	TYPEDEF,		0,0,0,0,1),
	kwdef_token(	"typeof",	T_TYPEOF,		0,0,1,0,7),
#ifdef INT128_SIZE
	kwdef_type(	"__uint128_t",	UINT128,		0,1,0,0,1),
#endif
	kwdef("union",	T_STRUCT_OR_UNION, 0,	UNION,	0,	0,0,0,0,1),
	kwdef_type(	"unsigned",	UNSIGN,			0,0,0,0,1),
	kwdef_gcc_attr(	"unused",	T_AT_UNUSED),
	kwdef_gcc_attr(	"used",		T_AT_USED),
	kwdef_gcc_attr(	"visibility",	T_AT_VISIBILITY),
	kwdef_type(	"void",		VOID,			0,0,0,0,1),
	kwdef_tqual(	"volatile",	VOLATILE,		1,0,0,0,7),
	kwdef_gcc_attr(	"warn_unused_result", T_AT_WARN_UNUSED_RESULT),
	kwdef_gcc_attr(	"weak",		T_AT_WEAK),
	kwdef_keyword(	"while",	T_WHILE),
	kwdef(NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0),
#undef kwdef
#undef kwdef_token
#undef kwdef_sclass
#undef kwdef_type
#undef kwdef_tqual
#undef kwdef_keyword
#undef kwdef_gcc_attr
};

/* Symbol table */
static	sym_t	*symtab[HSHSIZ1];

/* type of next expected symbol */
symt_t	symtyp;


static	int	get_escaped_char(int);


static unsigned int
hash(const char *s)
{
	unsigned int v;
	const char *p;

	v = 0;
	for (p = s; *p != '\0'; p++) {
		v = (v << 4) + (unsigned char)*p;
		v ^= v >> 28;
	}
	return v % HSHSIZ1;
}

static void
symtab_add(sym_t *sym)
{
	unsigned int h;

	h = hash(sym->s_name);
	if ((sym->s_symtab_next = symtab[h]) != NULL)
		symtab[h]->s_symtab_ref = &sym->s_symtab_next;
	sym->s_symtab_ref = &symtab[h];
	symtab[h] = sym;
}

static sym_t *
symtab_search(sbuf_t *sb)
{

	unsigned int h = hash(sb->sb_name);
	for (sym_t *sym = symtab[h]; sym != NULL; sym = sym->s_symtab_next) {
		if (strcmp(sym->s_name, sb->sb_name) != 0)
			continue;

		const struct keyword *kw = sym->s_keyword;
		if (kw != NULL && !kw->kw_attr)
			return sym;
		if (kw != NULL && in_gcc_attribute)
			return sym;
		if (kw == NULL && !in_gcc_attribute && sym->s_kind == symtyp)
			return sym;
	}

	return NULL;
}

static void
symtab_remove(sym_t *sym)
{

	if ((*sym->s_symtab_ref = sym->s_symtab_next) != NULL)
		sym->s_symtab_next->s_symtab_ref = sym->s_symtab_ref;
	sym->s_symtab_next = NULL;
}

static void
symtab_remove_locals(void)
{

	for (size_t i = 0; i < HSHSIZ1; i++) {
		for (sym_t *sym = symtab[i]; sym != NULL; ) {
			sym_t *next = sym->s_symtab_next;
			if (sym->s_block_level >= 1)
				symtab_remove(sym);
			sym = next;
		}
	}
}

#ifdef DEBUG
static int
sym_by_name(const void *va, const void *vb)
{
	const sym_t *a = *(const sym_t *const *)va;
	const sym_t *b = *(const sym_t *const *)vb;

	return strcmp(a->s_name, b->s_name);
}

struct syms {
	const sym_t **items;
	size_t len;
	size_t cap;
};

static void
syms_add(struct syms *syms, const sym_t *sym)
{
	while (syms->len >= syms->cap) {
		syms->cap *= 2;
		syms->items = xrealloc(syms->items,
		    syms->cap * sizeof(syms->items[0]));
	}
	syms->items[syms->len++] = sym;
}

void
debug_symtab(void)
{
	struct syms syms = { xcalloc(64, sizeof(syms.items[0])), 0, 64 };

	for (int level = -1;; level++) {
		bool more = false;
		size_t n = sizeof(symtab) / sizeof(symtab[0]);

		syms.len = 0;
		for (size_t i = 0; i < n; i++) {
			for (sym_t *sym = symtab[i]; sym != NULL;) {
				if (sym->s_block_level == level &&
				    sym->s_keyword == NULL)
					syms_add(&syms, sym);
				if (sym->s_block_level > level)
					more = true;
				sym = sym->s_symtab_next;
			}
		}

		if (syms.len > 0) {
			debug_printf("symbol table level %d\n", level);
			debug_indent_inc();
			qsort(syms.items, syms.len, sizeof(syms.items[0]),
			    sym_by_name);
			for (size_t i = 0; i < syms.len; i++)
				debug_sym("", syms.items[i], "\n");
			debug_indent_dec();

			lint_assert(level != -1);
		}

		if (!more)
			break;
	}

	free(syms.items);
}
#endif

static void
add_keyword(const struct keyword *kw, bool leading, bool trailing)
{
	sym_t *sym;
	char buf[256];
	const char *name;

	if (!leading && !trailing) {
		name = kw->kw_name;
	} else {
		(void)snprintf(buf, sizeof(buf), "%s%s%s",
		    leading ? "__" : "", kw->kw_name, trailing ? "__" : "");
		name = xstrdup(buf);
	}

	sym = block_zero_alloc(sizeof(*sym));
	sym->s_name = name;
	sym->s_keyword = kw;
	sym->u.s_keyword.sk_token = kw->kw_token;
	if (kw->kw_token == T_TYPE || kw->kw_token == T_STRUCT_OR_UNION) {
		sym->u.s_keyword.sk_tspec = kw->kw_tspec;
	} else if (kw->kw_token == T_SCLASS) {
		sym->s_scl = kw->kw_scl;
	} else if (kw->kw_token == T_QUAL) {
		sym->u.s_keyword.sk_qualifier = kw->kw_tqual;
	}

	symtab_add(sym);
}

/*
 * All keywords are written to the symbol table. This saves us looking
 * in an extra table for each name we found.
 */
void
initscan(void)
{
	const struct keyword *kw;

	for (kw = keywords; kw->kw_name != NULL; kw++) {
		if ((kw->kw_c90 || kw->kw_c99) && tflag)
			continue;
		if (kw->kw_c99 && !(Sflag || gflag))
			continue;
		if (kw->kw_gcc && !gflag)
			continue;
		if (kw->kw_plain)
			add_keyword(kw, false, false);
		if (kw->kw_leading)
			add_keyword(kw, true, false);
		if (kw->kw_both)
			add_keyword(kw, true, true);
	}
}

/*
 * Read a character and ensure that it is positive (except EOF).
 * Increment line count(s) if necessary.
 */
static int
inpc(void)
{
	int	c;

	if ((c = lex_input()) == EOF)
		return c;
	c &= CHAR_MASK;
	if (c == '\0')
		return EOF;	/* lex returns 0 on EOF. */
	if (c == '\n')
		lex_next_line();
	return c;
}

static int
lex_keyword(sym_t *sym)
{
	int	t;

	if ((t = sym->u.s_keyword.sk_token) == T_SCLASS) {
		yylval.y_scl = sym->s_scl;
	} else if (t == T_TYPE || t == T_STRUCT_OR_UNION) {
		yylval.y_tspec = sym->u.s_keyword.sk_tspec;
	} else if (t == T_QUAL) {
		yylval.y_tqual = sym->u.s_keyword.sk_qualifier;
	}
	return t;
}

/*
 * Lex has found a letter followed by zero or more letters or digits.
 * It looks for a symbol in the symbol table with the same name. This
 * symbol must either be a keyword or a symbol of the type required by
 * symtyp (label, member, tag, ...).
 *
 * If it is a keyword, the token is returned. In some cases it is described
 * more deeply by data written to yylval.
 *
 * If it is a symbol, T_NAME is returned and the name is stored in yylval.
 * If there is already a symbol of the same name and type in the symbol
 * table, yylval.y_name->sb_sym points there.
 */
extern int
lex_name(const char *yytext, size_t yyleng)
{
	char	*s;
	sbuf_t	*sb;
	sym_t	*sym;
	int	tok;

	sb = xmalloc(sizeof(*sb));
	sb->sb_name = yytext;
	sb->sb_len = yyleng;
	if ((sym = symtab_search(sb)) != NULL && sym->s_keyword != NULL) {
		free(sb);
		return lex_keyword(sym);
	}

	sb->sb_sym = sym;

	if (sym != NULL) {
		lint_assert(block_level >= sym->s_block_level);
		sb->sb_name = sym->s_name;
		tok = sym->s_scl == TYPEDEF ? T_TYPENAME : T_NAME;
	} else {
		s = block_zero_alloc(yyleng + 1);
		(void)memcpy(s, yytext, yyleng + 1);
		sb->sb_name = s;
		tok = T_NAME;
	}

	yylval.y_name = sb;
	return tok;
}

/*
 * Convert a string representing an integer into internal representation.
 * Return T_CON, storing the numeric value in yylval, for yylex.
 */
int
lex_integer_constant(const char *yytext, size_t yyleng, int base)
{
	int	l_suffix, u_suffix;
	size_t	len;
	const	char *cp;
	char	c, *eptr;
	tspec_t	typ;
	bool	ansiu;
	bool	warned = false;
	uint64_t uq = 0;

	/* C11 6.4.4.1p5 */
	static const tspec_t suffix_type[2][3] = {
		{ INT,  LONG,  QUAD, },
		{ UINT, ULONG, UQUAD, }
	};

	cp = yytext;
	len = yyleng;

	/* skip 0[xX] or 0[bB] */
	if (base == 16 || base == 2) {
		cp += 2;
		len -= 2;
	}

	/* read suffixes */
	l_suffix = u_suffix = 0;
	for (;;) {
		if ((c = cp[len - 1]) == 'l' || c == 'L') {
			l_suffix++;
		} else if (c == 'u' || c == 'U') {
			u_suffix++;
		} else {
			break;
		}
		len--;
	}
	if (l_suffix > 2 || u_suffix > 1) {
		/* malformed integer constant */
		warning(251);
		if (l_suffix > 2)
			l_suffix = 2;
		if (u_suffix > 1)
			u_suffix = 1;
	}
	if (tflag && u_suffix != 0) {
		/* suffix U is illegal in traditional C */
		warning(97);
	}
	typ = suffix_type[u_suffix][l_suffix];

	errno = 0;

	uq = (uint64_t)strtoull(cp, &eptr, base);
	lint_assert(eptr == cp + len);
	if (errno != 0) {
		/* integer constant out of range */
		warning(252);
		warned = true;
	}

	/*
	 * If the value is too big for the current type, we must choose
	 * another type.
	 */
	ansiu = false;
	switch (typ) {
	case INT:
		if (uq <= TARG_INT_MAX) {
			/* ok */
		} else if (uq <= TARG_UINT_MAX && base != 10) {
			typ = UINT;
		} else if (uq <= TARG_LONG_MAX) {
			typ = LONG;
		} else {
			typ = ULONG;
			if (uq > TARG_ULONG_MAX && !warned) {
				/* integer constant out of range */
				warning(252);
			}
		}
		if (typ == UINT || typ == ULONG) {
			if (tflag) {
				typ = LONG;
			} else if (!sflag) {
				/*
				 * Remember that the constant is unsigned
				 * only in ANSI C
				 */
				ansiu = true;
			}
		}
		break;
	case UINT:
		if (uq > TARG_UINT_MAX) {
			typ = ULONG;
			if (uq > TARG_ULONG_MAX && !warned) {
				/* integer constant out of range */
				warning(252);
			}
		}
		break;
	case LONG:
		if (uq > TARG_LONG_MAX && !tflag) {
			typ = ULONG;
			if (!sflag)
				ansiu = true;
			if (uq > TARG_ULONG_MAX && !warned) {
				/* integer constant out of range */
				warning(252);
			}
		}
		break;
	case ULONG:
		if (uq > TARG_ULONG_MAX && !warned) {
			/* integer constant out of range */
			warning(252);
		}
		break;
	case QUAD:
		if (uq > TARG_QUAD_MAX && !tflag) {
			typ = UQUAD;
			if (!sflag)
				ansiu = true;
		}
		break;
	case UQUAD:
		if (uq > TARG_UQUAD_MAX && !warned) {
			/* integer constant out of range */
			warning(252);
		}
		break;
	default:
		break;
	}

	uq = (uint64_t)convert_integer((int64_t)uq, typ, 0);

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = typ;
	yylval.y_val->v_unsigned_since_c90 = ansiu;
	yylval.y_val->v_quad = (int64_t)uq;

	return T_CON;
}

/*
 * Extend or truncate q to match t.  If t is signed, sign-extend.
 *
 * len is the number of significant bits. If len is -1, len is set
 * to the width of type t.
 */
int64_t
convert_integer(int64_t q, tspec_t t, unsigned int len)
{
	uint64_t vbits;

	if (len == 0)
		len = size_in_bits(t);

	vbits = value_bits(len);
	return t == PTR || is_uinteger(t) || ((q & bit(len - 1)) == 0)
	    ? (int64_t)(q & vbits)
	    : (int64_t)(q | ~vbits);
}

/*
 * Convert a string representing a floating point value into its numerical
 * representation. Type and value are returned in yylval.
 *
 * XXX Currently it is not possible to convert constants of type
 * long double which are greater than DBL_MAX.
 */
int
lex_floating_constant(const char *yytext, size_t yyleng)
{
	const	char *cp;
	size_t	len;
	tspec_t typ;
	char	c, *eptr;
	double	d;
	float	f = 0;

	cp = yytext;
	len = yyleng;

	if (cp[len - 1] == 'i')
		len--;		/* imaginary, do nothing for now */

	if ((c = cp[len - 1]) == 'f' || c == 'F') {
		typ = FLOAT;
		len--;
	} else if (c == 'l' || c == 'L') {
		typ = LDOUBLE;
		len--;
	} else {
		if (c == 'd' || c == 'D')
			len--;
		typ = DOUBLE;
	}

	if (tflag && typ != DOUBLE) {
		/* suffixes F and L are illegal in traditional C */
		warning(98);
	}

	errno = 0;
	d = strtod(cp, &eptr);
	if (eptr != cp + len) {
		switch (*eptr) {
			/*
			 * XXX: non-native non-current strtod() may not handle hex
			 * floats, ignore the rest if we find traces of hex float
			 * syntax...
			 */
		case 'p':
		case 'P':
		case 'x':
		case 'X':
			d = 0;
			errno = 0;
			break;
		default:
			INTERNAL_ERROR("lex_floating_constant(%s->%s)",
			    cp, eptr);
		}
	}
	if (errno != 0)
		/* floating-point constant out of range */
		warning(248);

	if (typ == FLOAT) {
		f = (float)d;
		if (isfinite(f) == 0) {
			/* floating-point constant out of range */
			warning(248);
			f = f > 0 ? FLT_MAX : -FLT_MAX;
		}
	}

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = typ;
	if (typ == FLOAT) {
		yylval.y_val->v_ldbl = f;
	} else {
		yylval.y_val->v_ldbl = d;
	}

	return T_CON;
}

int
lex_operator(int t, op_t o)
{

	yylval.y_op = o;
	return t;
}

/* Called if lex found a leading "'". */
int
lex_character_constant(void)
{
	size_t	n;
	int val, c;

	n = 0;
	val = 0;
	while ((c = get_escaped_char('\'')) >= 0) {
		val = (val << CHAR_SIZE) + c;
		n++;
	}
	if (c == -2) {
		/* unterminated character constant */
		error(253);
	} else if (n > sizeof(int) || (n > 1 && (pflag || hflag))) {
		/* XXX: should rather be sizeof(TARG_INT) */

		/* too many characters in character constant */
		error(71);
	} else if (n > 1) {
		/* multi-character character constant */
		warning(294);
	} else if (n == 0) {
		/* empty character constant */
		error(73);
	}
	if (n == 1)
		val = (int)convert_integer(val, CHAR, CHAR_SIZE);

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = INT;
	yylval.y_val->v_quad = val;

	return T_CON;
}

/*
 * Called if lex found a leading L\'
 */
int
lex_wide_character_constant(void)
{
	static	char buf[MB_LEN_MAX + 1];
	size_t	n, nmax;
	int c;
	wchar_t	wc;

	nmax = MB_CUR_MAX;

	n = 0;
	while ((c = get_escaped_char('\'')) >= 0) {
		if (n < nmax)
			buf[n] = (char)c;
		n++;
	}

	wc = 0;

	if (c == -2) {
		/* unterminated character constant */
		error(253);
	} else if (n == 0) {
		/* empty character constant */
		error(73);
	} else if (n > nmax) {
		n = nmax;
		/* too many characters in character constant */
		error(71);
	} else {
		buf[n] = '\0';
		(void)mbtowc(NULL, NULL, 0);
		if (mbtowc(&wc, buf, nmax) < 0)
			/* invalid multibyte character */
			error(291);
	}

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = WCHAR;
	yylval.y_val->v_quad = wc;

	return T_CON;
}

/*
 * Read a character which is part of a character constant or of a string
 * and handle escapes.
 *
 * The argument is the character which delimits the character constant or
 * string.
 *
 * Returns -1 if the end of the character constant or string is reached,
 * -2 if the EOF is reached, and the character otherwise.
 */
static int
get_escaped_char(int delim)
{
	static	int pbc = -1;
	int	n, c, v;

	if (pbc == -1) {
		c = inpc();
	} else {
		c = pbc;
		pbc = -1;
	}
	if (c == delim)
		return -1;
	switch (c) {
	case '\n':
		if (tflag) {
			/* newline in string or char constant */
			error(254);
			return -2;
		}
		return c;
	case 0:
		/* syntax error '%s' */
		error(249, "EOF or null byte in literal");
		return -2;
	case EOF:
		return -2;
	case '\\':
		switch (c = inpc()) {
		case '"':
			if (tflag && delim == '\'')
				/* \" inside character constants undef... */
				warning(262);
			return '"';
		case '\'':
			return '\'';
		case '?':
			if (tflag)
				/* \? undefined in traditional C */
				warning(263);
			return '?';
		case '\\':
			return '\\';
		case 'a':
			if (tflag)
				/* \a undefined in traditional C */
				warning(81);
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			if (tflag)
				/* \v undefined in traditional C */
				warning(264);
			return '\v';
		case '8': case '9':
			/* bad octal digit %c */
			warning(77, c);
			/* FALLTHROUGH */
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			n = 3;
			v = 0;
			do {
				v = (v << 3) + (c - '0');
				c = inpc();
			} while (--n > 0 && '0' <= c && c <= '7');
			pbc = c;
			if (v > TARG_UCHAR_MAX) {
				/* character escape does not fit in character */
				warning(76);
				v &= CHAR_MASK;
			}
			return v;
		case 'x':
			if (tflag)
				/* \x undefined in traditional C */
				warning(82);
			v = 0;
			n = 0;
			while (c = inpc(), isxdigit(c)) {
				c = isdigit(c) ?
				    c - '0' : toupper(c) - 'A' + 10;
				v = (v << 4) + c;
				if (n >= 0) {
					if ((v & ~CHAR_MASK) != 0) {
						/* overflow in hex escape */
						warning(75);
						n = -1;
					} else {
						n++;
					}
				}
			}
			pbc = c;
			if (n == 0) {
				/* no hex digits follow \x */
				error(74);
			} if (n == -1) {
				v &= CHAR_MASK;
			}
			return v;
		case '\n':
			return get_escaped_char(delim);
		case EOF:
			return -2;
		default:
			if (isprint(c)) {
				/* dubious escape \%c */
				warning(79, c);
			} else {
				/* dubious escape \%o */
				warning(80, c);
			}
		}
	}
	return c;
}

/* See https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html */
static void
parse_line_directive_flags(const char *p,
			   bool *is_begin, bool *is_end, bool *is_system)
{

	*is_begin = false;
	*is_end = false;
	*is_system = false;

	while (*p != '\0') {
		const char *word_start, *word_end;

		while (ch_isspace(*p))
			p++;

		word_start = p;
		while (*p != '\0' && !ch_isspace(*p))
			p++;
		word_end = p;

		if (word_end - word_start == 1 && word_start[0] == '1')
			*is_begin = true;
		if (word_end - word_start == 1 && word_start[0] == '2')
			*is_end = true;
		if (word_end - word_start == 1 && word_start[0] == '3')
			*is_system = true;
		/* Flag '4' is only interesting for C++. */
	}
}

/*
 * Called for preprocessor directives. Currently implemented are:
 *	# pragma [argument...]
 *	# lineno
 *	# lineno "filename"
 *	# lineno "filename" GCC-flag...
 */
void
lex_directive(const char *yytext)
{
	const	char *cp, *fn;
	char	c, *eptr;
	size_t	fnl;
	long	ln;
	bool	is_begin, is_end, is_system;

	static	bool first = true;

	/* Go to first non-whitespace after # */
	for (cp = yytext + 1; (c = *cp) == ' ' || c == '\t'; cp++)
		continue;

	if (!ch_isdigit(c)) {
		if (strncmp(cp, "pragma", 6) == 0 && ch_isspace(cp[6]))
			return;
	error:
		/* undefined or invalid # directive */
		warning(255);
		return;
	}
	ln = strtol(--cp, &eptr, 10);
	if (eptr == cp)
		goto error;
	if ((c = *(cp = eptr)) != ' ' && c != '\t' && c != '\0')
		goto error;
	while ((c = *cp++) == ' ' || c == '\t')
		continue;
	if (c != '\0') {
		if (c != '"')
			goto error;
		fn = cp;
		while ((c = *cp) != '"' && c != '\0')
			cp++;
		if (c != '"')
			goto error;
		if ((fnl = cp++ - fn) > PATH_MAX)
			goto error;
		/* empty string means stdin */
		if (fnl == 0) {
			fn = "{standard input}";
			fnl = 16;			/* strlen (fn) */
		}
		curr_pos.p_file = record_filename(fn, fnl);
		/*
		 * If this is the first directive, the name is the name
		 * of the C source file as specified at the command line.
		 * It is written to the output file.
		 */
		if (first) {
			csrc_pos.p_file = curr_pos.p_file;
			outsrc(transform_filename(curr_pos.p_file,
			    strlen(curr_pos.p_file)));
			first = false;
		}

		parse_line_directive_flags(cp, &is_begin, &is_end, &is_system);
		update_location(curr_pos.p_file, (int)ln, is_begin, is_end);
		in_system_header = is_system;
	}
	curr_pos.p_line = (int)ln - 1;
	curr_pos.p_uniq = 0;
	if (curr_pos.p_file == csrc_pos.p_file) {
		csrc_pos.p_line = (int)ln - 1;
		csrc_pos.p_uniq = 0;
	}
}

/*
 * Handle lint comments such as ARGSUSED.
 *
 * If one of these comments is recognized, the argument, if any, is
 * parsed and a function which handles this comment is called.
 */
void
lex_comment(void)
{
	int	c, lc;
	static const struct {
		const	char *keywd;
		bool	arg;
		void	(*func)(int);
	} keywtab[] = {
		{ "ARGSUSED",		true,	argsused	},
		{ "BITFIELDTYPE",	false,	bitfieldtype	},
		{ "CONSTCOND",		false,	constcond	},
		{ "CONSTANTCOND",	false,	constcond	},
		{ "CONSTANTCONDITION",	false,	constcond	},
		{ "FALLTHRU",		false,	fallthru	},
		{ "FALLTHROUGH",	false,	fallthru	},
		{ "FALL THROUGH",	false,	fallthru	},
		{ "fallthrough",	false,	fallthru	},
		{ "LINTLIBRARY",	false,	lintlib		},
		{ "LINTED",		true,	linted		},
		{ "LONGLONG",		false,	longlong	},
		{ "NOSTRICT",		true,	linted		},
		{ "NOTREACHED",		false,	not_reached	},
		{ "PRINTFLIKE",		true,	printflike	},
		{ "PROTOLIB",		true,	protolib	},
		{ "SCANFLIKE",		true,	scanflike	},
		{ "VARARGS",		true,	varargs		},
	};
	char	keywd[32];
	char	arg[32];
	size_t	l, i;
	int	a;
	bool	eoc;

	eoc = false;

	/* Skip whitespace after the start of the comment */
	while (c = inpc(), isspace(c))
		continue;

	/* Read the potential keyword to keywd */
	l = 0;
	while (c != EOF && l < sizeof(keywd) - 1 &&
	    (isalpha(c) || isspace(c))) {
		if (islower(c) && l > 0 && ch_isupper(keywd[0]))
			break;
		keywd[l++] = (char)c;
		c = inpc();
	}
	while (l > 0 && ch_isspace(keywd[l - 1]))
		l--;
	keywd[l] = '\0';

	/* look for the keyword */
	for (i = 0; i < sizeof(keywtab) / sizeof(keywtab[0]); i++) {
		if (strcmp(keywtab[i].keywd, keywd) == 0)
			break;
	}
	if (i == sizeof(keywtab) / sizeof(keywtab[0]))
		goto skip_rest;

	/* skip whitespace after the keyword */
	while (isspace(c))
		c = inpc();

	/* read the argument, if the keyword accepts one and there is one */
	l = 0;
	if (keywtab[i].arg) {
		while (isdigit(c) && l < sizeof(arg) - 1) {
			arg[l++] = (char)c;
			c = inpc();
		}
	}
	arg[l] = '\0';
	a = l != 0 ? atoi(arg) : -1;

	/* skip whitespace after the argument */
	while (isspace(c))
		c = inpc();

	if (c != '*' || (c = inpc()) != '/') {
		if (keywtab[i].func != linted)
			/* extra characters in lint comment */
			warning(257);
	} else {
		/*
		 * remember that we have already found the end of the
		 * comment
		 */
		eoc = true;
	}

	if (keywtab[i].func != NULL)
		(*keywtab[i].func)(a);

skip_rest:
	while (!eoc) {
		lc = c;
		if ((c = inpc()) == EOF) {
			/* unterminated comment */
			error(256);
			break;
		}
		if (lc == '*' && c == '/')
			eoc = true;
	}
}

/*
 * Handle // style comments
 */
void
lex_slash_slash_comment(void)
{
	int c;

	if (!Sflag && !gflag)
		/* %s does not support // comments */
		gnuism(312, tflag ? "traditional C" : "C90");

	while ((c = inpc()) != EOF && c != '\n')
		continue;
}

/*
 * Clear flags for lint comments LINTED, LONGLONG and CONSTCOND.
 * clear_warn_flags is called after function definitions and global and
 * local declarations and definitions. It is also called between
 * the controlling expression and the body of control statements
 * (if, switch, for, while).
 */
void
clear_warn_flags(void)
{

	lwarn = LWARN_ALL;
	quadflg = false;
	constcond_flag = false;
}

/*
 * Strings are stored in a dynamically allocated buffer and passed
 * in yylval.y_string to the parser. The parser or the routines called
 * by the parser are responsible for freeing this buffer.
 */
int
lex_string(void)
{
	unsigned char *s;
	int	c;
	size_t	len, max;
	strg_t	*strg;

	s = xmalloc(max = 64);

	len = 0;
	while ((c = get_escaped_char('"')) >= 0) {
		/* +1 to reserve space for a trailing NUL character */
		if (len + 1 == max)
			s = xrealloc(s, max *= 2);
		s[len++] = (char)c;
	}
	s[len] = '\0';
	if (c == -2)
		/* unterminated string constant */
		error(258);

	strg = xcalloc(1, sizeof(*strg));
	strg->st_char = true;
	strg->st_len = len;
	strg->st_mem = s;

	yylval.y_string = strg;
	return T_STRING;
}

int
lex_wide_string(void)
{
	char	*s;
	int	c, n;
	size_t	i, wi;
	size_t	len, max, wlen;
	wchar_t	*ws;
	strg_t	*strg;

	s = xmalloc(max = 64);
	len = 0;
	while ((c = get_escaped_char('"')) >= 0) {
		/* +1 to save space for a trailing NUL character */
		if (len + 1 >= max)
			s = xrealloc(s, max *= 2);
		s[len++] = (char)c;
	}
	s[len] = '\0';
	if (c == -2)
		/* unterminated string constant */
		error(258);

	/* get length of wide-character string */
	(void)mblen(NULL, 0);
	for (i = 0, wlen = 0; i < len; i += n, wlen++) {
		if ((n = mblen(&s[i], MB_CUR_MAX)) == -1) {
			/* invalid multibyte character */
			error(291);
			break;
		}
		if (n == 0)
			n = 1;
	}

	ws = xmalloc((wlen + 1) * sizeof(*ws));

	/* convert from multibyte to wide char */
	(void)mbtowc(NULL, NULL, 0);
	for (i = 0, wi = 0; i < len; i += n, wi++) {
		if ((n = mbtowc(&ws[wi], &s[i], MB_CUR_MAX)) == -1)
			break;
		if (n == 0)
			n = 1;
	}
	ws[wi] = 0;
	free(s);

	strg = xcalloc(1, sizeof(*strg));
	strg->st_char = false;
	strg->st_len = wlen;
	strg->st_mem = ws;

	yylval.y_string = strg;
	return T_STRING;
}

void
lex_next_line(void)
{
	curr_pos.p_line++;
	curr_pos.p_uniq = 0;
	debug_step("parsing %s:%d", curr_pos.p_file, curr_pos.p_line);
	if (curr_pos.p_file == csrc_pos.p_file) {
		csrc_pos.p_line++;
		csrc_pos.p_uniq = 0;
	}
}

void
lex_unknown_character(int c)
{

	/* unknown character \%o */
	error(250, c);
}

/*
 * The scanner does not create new symbol table entries for symbols it cannot
 * find in the symbol table. This is to avoid putting undeclared symbols into
 * the symbol table if a syntax error occurs.
 *
 * getsym is called as soon as it is probably ok to put the symbol in the
 * symbol table. It is still possible that symbols are put in the symbol
 * table that are not completely declared due to syntax errors. To avoid too
 * many problems in this case, symbols get type 'int' in getsym.
 *
 * XXX calls to getsym should be delayed until declare_1_* is called.
 */
sym_t *
getsym(sbuf_t *sb)
{
	dinfo_t	*di;
	char	*s;
	sym_t	*sym;

	sym = sb->sb_sym;

	/*
	 * During member declaration it is possible that name() looked
	 * for symbols of type FVFT, although it should have looked for
	 * symbols of type FTAG. Same can happen for labels. Both cases
	 * are compensated here.
	 */
	if (symtyp == FMEMBER || symtyp == FLABEL) {
		if (sym == NULL || sym->s_kind == FVFT)
			sym = symtab_search(sb);
	}

	if (sym != NULL) {
		lint_assert(sym->s_kind == symtyp);
		symtyp = FVFT;
		free(sb);
		return sym;
	}

	/* create a new symbol table entry */

	/* labels must always be allocated at level 1 (outermost block) */
	if (symtyp == FLABEL) {
		sym = level_zero_alloc(1, sizeof(*sym));
		s = level_zero_alloc(1, sb->sb_len + 1);
		(void)memcpy(s, sb->sb_name, sb->sb_len + 1);
		sym->s_name = s;
		sym->s_block_level = 1;
		di = dcs;
		while (di->d_enclosing != NULL &&
		    di->d_enclosing->d_enclosing != NULL)
			di = di->d_enclosing;
		lint_assert(di->d_kind == DK_AUTO);
	} else {
		sym = block_zero_alloc(sizeof(*sym));
		sym->s_name = sb->sb_name;
		sym->s_block_level = block_level;
		di = dcs;
	}

	UNIQUE_CURR_POS(sym->s_def_pos);
	if ((sym->s_kind = symtyp) != FLABEL)
		sym->s_type = gettyp(INT);

	symtyp = FVFT;

	symtab_add(sym);

	*di->d_ldlsym = sym;
	di->d_ldlsym = &sym->s_level_next;

	free(sb);
	return sym;
}

/*
 * Construct a temporary symbol. The symbol name starts with a digit, making
 * the name illegal.
 */
sym_t *
mktempsym(type_t *tp)
{
	static unsigned n = 0;
	char *s = level_zero_alloc(block_level, 64);
	sym_t *sym = block_zero_alloc(sizeof(*sym));
	scl_t scl;

	(void)snprintf(s, 64, "%.8u_tmp", n++);

	scl = dcs->d_scl;
	if (scl == NOSCL)
		scl = block_level > 0 ? AUTO : EXTERN;

	sym->s_name = s;
	sym->s_type = tp;
	sym->s_block_level = block_level;
	sym->s_scl = scl;
	sym->s_kind = FVFT;
	sym->s_used = true;
	sym->s_set = true;

	symtab_add(sym);

	*dcs->d_ldlsym = sym;
	dcs->d_ldlsym = &sym->s_level_next;

	return sym;
}

/* Remove a symbol forever from the symbol table. */
void
rmsym(sym_t *sym)
{

	debug_step("rmsym '%s' %s '%s'",
	    sym->s_name, symt_name(sym->s_kind), type_name(sym->s_type));
	symtab_remove(sym);

	/* avoid that the symbol will later be put back to the symbol table */
	sym->s_block_level = -1;
}

/*
 * Remove all symbols from the symbol table that have the same level as the
 * given symbol.
 */
void
rmsyms(sym_t *syms)
{
	sym_t	*sym;

	/* Note the use of s_level_next instead of s_symtab_next. */
	for (sym = syms; sym != NULL; sym = sym->s_level_next) {
		if (sym->s_block_level != -1) {
			debug_step("rmsyms '%s' %s '%s'",
			    sym->s_name, symt_name(sym->s_kind),
			    type_name(sym->s_type));
			symtab_remove(sym);
			sym->s_symtab_ref = NULL;
		}
	}
}

/*
 * Put a symbol into the symbol table.
 */
void
inssym(int level, sym_t *sym)
{

	debug_step("inssym '%s' %s '%s'",
	    sym->s_name, symt_name(sym->s_kind), type_name(sym->s_type));
	symtab_add(sym);
	sym->s_block_level = level;

	/*
	 * Placing the inner symbols to the beginning of the list ensures
	 * that these symbols are preferred over symbols from the outer
	 * blocks that happen to have the same name.
	 */
	lint_assert(sym->s_symtab_next != NULL
	    ? sym->s_block_level >= sym->s_symtab_next->s_block_level
	    : true);
}

/*
 * Called at level 0 after syntax errors.
 *
 * Removes all symbols which are not declared at level 0 from the
 * symbol table. Also frees all memory which is not associated with
 * level 0.
 */
void
clean_up_after_error(void)
{

	symtab_remove_locals();

	for (size_t i = mem_block_level; i > 0; i--)
		level_free_all(i);
}

/* Create a new symbol with the same name as an existing symbol. */
sym_t *
pushdown(const sym_t *sym)
{
	sym_t	*nsym;

	debug_step("pushdown '%s' %s '%s'",
	    sym->s_name, symt_name(sym->s_kind), type_name(sym->s_type));
	nsym = block_zero_alloc(sizeof(*nsym));
	lint_assert(sym->s_block_level <= block_level);
	nsym->s_name = sym->s_name;
	UNIQUE_CURR_POS(nsym->s_def_pos);
	nsym->s_kind = sym->s_kind;
	nsym->s_block_level = block_level;

	symtab_add(nsym);

	*dcs->d_ldlsym = nsym;
	dcs->d_ldlsym = &nsym->s_level_next;

	return nsym;
}

/*
 * Free any dynamically allocated memory referenced by
 * the value stack or yylval.
 * The type of information in yylval is described by tok.
 */
void
freeyyv(void *sp, int tok)
{
	if (tok == T_NAME || tok == T_TYPENAME) {
		sbuf_t *sb = *(sbuf_t **)sp;
		free(sb);
	} else if (tok == T_CON) {
		val_t *val = *(val_t **)sp;
		free(val);
	} else if (tok == T_STRING) {
		strg_t *strg = *(strg_t **)sp;
		free(strg->st_mem);
		free(strg);
	}
}
