/* $NetBSD: lex.c,v 1.147 2023/01/29 13:57:35 rillig Exp $ */

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
#if defined(__RCSID)
__RCSID("$NetBSD: lex.c,v 1.147 2023/01/29 13:57:35 rillig Exp $");
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

/*
 * Valid values for 'since' are 78, 90, 99, 11.
 *
 * The C11 keywords are added in C99 mode as well, to provide good error
 * messages instead of a simple parse error.  If the keyword '_Generic' were
 * not defined, it would be interpreted as an implicit function call, leading
 * to a parse error.
 */
#define kwdef(name, token, scl, tspec, tqual,	since, gcc, deco) \
	{ \
		name, token, scl, tspec, tqual, \
		(since) == 90, \
		/* CONSTCOND */ (since) == 99 || (since) == 11, \
		(gcc) > 0, \
		((deco) & 1) != 0, ((deco) & 2) != 0, ((deco) & 4) != 0, \
	}
#define kwdef_token(name, token,		since, gcc, deco) \
	kwdef(name, token, 0, 0, 0,		since, gcc, deco)
#define kwdef_sclass(name, sclass,		since, gcc, deco) \
	kwdef(name, T_SCLASS, sclass, 0, 0,	since, gcc, deco)
#define kwdef_type(name, tspec,			since) \
	kwdef(name, T_TYPE, 0, tspec, 0,	since, 0, 1)
#define kwdef_tqual(name, tqual,		since, gcc, deco) \
	kwdef(name, T_QUAL, 0, 0, tqual,	since, gcc, deco)
#define kwdef_keyword(name, token) \
	kwdef(name, token, 0, 0, 0,		78, 0, 1)

/* During initialization, these keywords are written to the symbol table. */
static const struct keyword {
	const	char *kw_name;
	int	kw_token;	/* token returned by yylex() */
	scl_t	kw_scl;		/* storage class if kw_token is T_SCLASS */
	tspec_t	kw_tspec;	/* type specifier if kw_token is T_TYPE or
				 * T_STRUCT_OR_UNION */
	tqual_t	kw_tqual;	/* type qualifier if kw_token is T_QUAL */
	bool	kw_c90:1;	/* available in C90 mode */
	bool	kw_c99_or_c11:1; /* available in C99 or C11 mode */
	bool	kw_gcc:1;	/* available in GCC mode */
	bool	kw_plain:1;	/* 'name' */
	bool	kw_leading:1;	/* '__name' */
	bool	kw_both:1;	/* '__name__' */
} keywords[] = {
	kwdef_keyword(	"_Alignas",	T_ALIGNAS),
	kwdef_keyword(	"_Alignof",	T_ALIGNOF),
	kwdef_token(	"alignof",	T_ALIGNOF,		78,0,6),
	kwdef_token(	"asm",		T_ASM,			78,1,7),
	kwdef_token(	"_Atomic",	T_ATOMIC,		11,0,1),
	kwdef_token(	"attribute",	T_ATTRIBUTE,		78,1,6),
	kwdef_sclass(	"auto",		AUTO,			78,0,1),
	kwdef_type(	"_Bool",	BOOL,			99),
	kwdef_keyword(	"break",	T_BREAK),
	kwdef_token(	"__builtin_offsetof", T_BUILTIN_OFFSETOF, 78,1,1),
	kwdef_keyword(	"case",		T_CASE),
	kwdef_type(	"char",		CHAR,			78),
	kwdef_type(	"_Complex",	COMPLEX,		99),
	kwdef_tqual(	"const",	CONST,			90,0,7),
	kwdef_keyword(	"continue",	T_CONTINUE),
	kwdef_keyword(	"default",	T_DEFAULT),
	kwdef_keyword(	"do",		T_DO),
	kwdef_type(	"double",	DOUBLE,			78),
	kwdef_keyword(	"else",		T_ELSE),
	kwdef_keyword(	"enum",		T_ENUM),
	kwdef_token(	"__extension__",T_EXTENSION,		78,1,1),
	kwdef_sclass(	"extern",	EXTERN,			78,0,1),
	kwdef_type(	"float",	FLOAT,			78),
	kwdef_keyword(	"for",		T_FOR),
	kwdef_token(	"_Generic",	T_GENERIC,		11,0,1),
	kwdef_keyword(	"goto",		T_GOTO),
	kwdef_keyword(	"if",		T_IF),
	kwdef_token(	"__imag__",	T_IMAG,			78,1,1),
	kwdef_sclass(	"inline",	INLINE,			99,0,7),
	kwdef_type(	"int",		INT,			78),
#ifdef INT128_SIZE
	kwdef_type(	"__int128_t",	INT128,			99),
#endif
	kwdef_type(	"long",		LONG,			78),
	kwdef_token(	"_Noreturn",	T_NORETURN,		11,0,1),
	kwdef_token(	"__packed",	T_PACKED,		78,0,1),
	kwdef_token(	"__real__",	T_REAL,			78,1,1),
	kwdef_sclass(	"register",	REG,			78,0,1),
	kwdef_tqual(	"restrict",	RESTRICT,		99,0,7),
	kwdef_keyword(	"return",	T_RETURN),
	kwdef_type(	"short",	SHORT,			78),
	kwdef(		"signed",	T_TYPE, 0, SIGNED, 0,	90,0,3),
	kwdef_keyword(	"sizeof",	T_SIZEOF),
	kwdef_sclass(	"static",	STATIC,			78,0,1),
	kwdef_keyword(	"_Static_assert",	T_STATIC_ASSERT),
	kwdef("struct",	T_STRUCT_OR_UNION, 0,	STRUCT,	0,	78,0,1),
	kwdef_keyword(	"switch",	T_SWITCH),
	kwdef_token(	"__symbolrename",	T_SYMBOLRENAME,	78,0,1),
	kwdef_tqual(	"__thread",	THREAD,			78,1,1),
	/* XXX: _Thread_local is a storage-class-specifier, not tqual. */
	kwdef_tqual(	"_Thread_local", THREAD,		11,0,1),
	kwdef_sclass(	"typedef",	TYPEDEF,		78,0,1),
	kwdef_token(	"typeof",	T_TYPEOF,		78,1,7),
#ifdef INT128_SIZE
	kwdef_type(	"__uint128_t",	UINT128,		99),
#endif
	kwdef("union",	T_STRUCT_OR_UNION, 0,	UNION,	0,	78,0,1),
	kwdef_type(	"unsigned",	UNSIGN,			78),
	kwdef_type(	"void",		VOID,			78),
	kwdef_tqual(	"volatile",	VOLATILE,		90,0,7),
	kwdef_keyword(	"while",	T_WHILE),
#undef kwdef
#undef kwdef_token
#undef kwdef_sclass
#undef kwdef_type
#undef kwdef_tqual
#undef kwdef_keyword
};

/*
 * The symbol table containing all keywords, identifiers and labels. The hash
 * entries are linked via sym_t.s_symtab_next.
 */
static sym_t *symtab[HSHSIZ1];

/*
 * The kind of the next expected symbol, to distinguish the namespaces of
 * members, labels, type tags and other identifiers.
 */
symt_t symtyp;


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
symtab_search(const char *name)
{

	unsigned int h = hash(name);
	for (sym_t *sym = symtab[h]; sym != NULL; sym = sym->s_symtab_next) {
		if (strcmp(sym->s_name, name) != 0)
			continue;

		const struct keyword *kw = sym->s_keyword;
		if (kw != NULL || in_gcc_attribute)
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
	if (syms->len >= syms->cap) {
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

	const char *name;
	if (!leading && !trailing) {
		name = kw->kw_name;
	} else {
		char buf[256];
		(void)snprintf(buf, sizeof(buf), "%s%s%s",
		    leading ? "__" : "", kw->kw_name, trailing ? "__" : "");
		name = xstrdup(buf);
	}

	sym_t *sym = block_zero_alloc(sizeof(*sym));
	sym->s_name = name;
	sym->s_keyword = kw;
	int tok = kw->kw_token;
	sym->u.s_keyword.sk_token = tok;
	if (tok == T_TYPE || tok == T_STRUCT_OR_UNION)
		sym->u.s_keyword.sk_tspec = kw->kw_tspec;
	if (tok == T_SCLASS)
		sym->s_scl = kw->kw_scl;
	if (tok == T_QUAL)
		sym->u.s_keyword.sk_qualifier = kw->kw_tqual;

	symtab_add(sym);
}

static bool
is_keyword_known(const struct keyword *kw)
{

	if ((kw->kw_c90 || kw->kw_c99_or_c11) && !allow_c90)
		return false;

	/*
	 * In the 1990s, GCC defined several keywords that were later
	 * incorporated into C99, therefore in GCC mode, all C99 keywords are
	 * made available.  The C11 keywords are made available as well, but
	 * there are so few that they don't matter practically.
	 */
	if (allow_gcc)
		return true;
	if (kw->kw_gcc)
		return false;

	if (kw->kw_c99_or_c11 && !allow_c99)
		return false;
	return true;
}

/* Write all keywords to the symbol table. */
void
initscan(void)
{

	size_t n = sizeof(keywords) / sizeof(keywords[0]);
	for (size_t i = 0; i < n; i++) {
		const struct keyword *kw = keywords + i;
		if (!is_keyword_known(kw))
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
 * When scanning the remainder of a long token (see lex_input), read a byte
 * and return it as an unsigned char or as EOF.
 *
 * Increment the line counts if necessary.
 */
static int
read_byte(void)
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
	int tok = sym->u.s_keyword.sk_token;

	if (tok == T_SCLASS)
		yylval.y_scl = sym->s_scl;
	if (tok == T_TYPE || tok == T_STRUCT_OR_UNION)
		yylval.y_tspec = sym->u.s_keyword.sk_tspec;
	if (tok == T_QUAL)
		yylval.y_tqual = sym->u.s_keyword.sk_qualifier;
	return tok;
}

/*
 * Look up the definition of a name in the symbol table. This symbol must
 * either be a keyword or a symbol of the type required by symtyp (label,
 * member, tag, ...).
 */
extern int
lex_name(const char *yytext, size_t yyleng)
{

	sym_t *sym = symtab_search(yytext);
	if (sym != NULL && sym->s_keyword != NULL)
		return lex_keyword(sym);

	sbuf_t *sb = xmalloc(sizeof(*sb));
	sb->sb_len = yyleng;
	sb->sb_sym = sym;
	yylval.y_name = sb;

	if (sym != NULL) {
		lint_assert(block_level >= sym->s_block_level);
		sb->sb_name = sym->s_name;
		return sym->s_scl == TYPEDEF ? T_TYPENAME : T_NAME;
	}

	char *name = block_zero_alloc(yyleng + 1);
	(void)memcpy(name, yytext, yyleng + 1);
	sb->sb_name = name;
	return T_NAME;
}

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
	for (;; len--) {
		if ((c = cp[len - 1]) == 'l' || c == 'L')
			l_suffix++;
		else if (c == 'u' || c == 'U')
			u_suffix++;
		else
			break;
	}
	if (l_suffix > 2 || u_suffix > 1) {
		/* malformed integer constant */
		warning(251);
		if (l_suffix > 2)
			l_suffix = 2;
		if (u_suffix > 1)
			u_suffix = 1;
	}
	if (!allow_c90 && u_suffix != 0) {
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
			if (!allow_c90) {
				typ = LONG;
			} else if (allow_trad || allow_c99) {
				/*
				 * Remember that the constant is unsigned
				 * only in ANSI C.
				 *
				 * TODO: C99 behaves like C90 here.
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
		if (uq > TARG_LONG_MAX && allow_c90) {
			typ = ULONG;
			/* TODO: C99 behaves like C90 here. */
			if (allow_trad || allow_c99)
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
		if (uq > TARG_QUAD_MAX && allow_c90) {
			typ = UQUAD;
			/* TODO: C99 behaves like C90 here. */
			if (allow_trad || allow_c99)
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
 * len is the number of significant bits. If len is 0, len is set
 * to the width of type t.
 */
int64_t
convert_integer(int64_t q, tspec_t t, unsigned int len)
{

	if (len == 0)
		len = size_in_bits(t);

	uint64_t vbits = value_bits(len);
	return t == PTR || is_uinteger(t) || ((q & bit(len - 1)) == 0)
	    ? (int64_t)(q & vbits)
	    : (int64_t)(q | ~vbits);
}

int
lex_floating_constant(const char *yytext, size_t yyleng)
{
	const	char *cp;
	size_t	len;
	tspec_t typ;
	char	c, *eptr;
	double	d;

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

	if (!allow_c90 && typ != DOUBLE) {
		/* suffixes F and L are illegal in traditional C */
		warning(98);
	}

	/* TODO: Handle precision and exponents of 'long double'. */
	errno = 0;
	d = strtod(cp, &eptr);
	if (eptr != cp + len) {
		switch (*eptr) {
			/*
			 * XXX: Non-native non-current strtod() may not
			 * handle hex floats, ignore the rest if we find
			 * traces of hex float syntax.
			 */
		case 'p':
		case 'P':
		case 'x':
		case 'X':
			d = 0;
			errno = 0;
			break;
		default:
			INTERNAL_ERROR("lex_floating_constant(%.*s)",
			    (int)(eptr - cp), cp);
		}
	}
	if (errno != 0)
		/* floating-point constant out of range */
		warning(248);

	if (typ == FLOAT) {
		d = (float)d;
		if (isfinite(d) == 0) {
			/* floating-point constant out of range */
			warning(248);
			d = d > 0 ? FLT_MAX : -FLT_MAX;
		}
	}

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = typ;
	yylval.y_val->v_ldbl = d;

	return T_CON;
}

int
lex_operator(int t, op_t o)
{

	yylval.y_op = o;
	return t;
}

static int prev_byte = -1;

static int
read_escaped_oct(int c)
{
	int n = 3;
	int value = 0;
	do {
		value = (value << 3) + (c - '0');
		c = read_byte();
	} while (--n > 0 && '0' <= c && c <= '7');
	prev_byte = c;
	if (value > TARG_UCHAR_MAX) {
		/* character escape does not fit in character */
		warning(76);
		value &= CHAR_MASK;
	}
	return value;
}

static unsigned int
read_escaped_hex(int c)
{
	if (!allow_c90)
		/* \x undefined in traditional C */
		warning(82);
	unsigned int value = 0;
	int state = 0;		/* 0 = no digits, 1 = OK, 2 = overflow */
	while (c = read_byte(), isxdigit(c)) {
		c = isdigit(c) ? c - '0' : toupper(c) - 'A' + 10;
		value = (value << 4) + c;
		if (state == 2)
			continue;
		if ((value & ~CHAR_MASK) != 0) {
			/* overflow in hex escape */
			warning(75);
			state = 2;
		} else {
			state = 1;
		}
	}
	prev_byte = c;
	if (state == 0) {
		/* no hex digits follow \x */
		error(74);
	}
	if (state == 2)
		value &= CHAR_MASK;
	return value;
}

static int
read_escaped_backslash(int delim)
{
	int c;

	switch (c = read_byte()) {
	case '"':
		if (!allow_c90 && delim == '\'')
			/* \" inside character constants undef... */
			warning(262);
		return '"';
	case '\'':
		return '\'';
	case '?':
		if (!allow_c90)
			/* \? undefined in traditional C */
			warning(263);
		return '?';
	case '\\':
		return '\\';
	case 'a':
		if (!allow_c90)
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
		if (!allow_c90)
			/* \v undefined in traditional C */
			warning(264);
		return '\v';
	case '8': case '9':
		/* bad octal digit %c */
		warning(77, c);
		/* FALLTHROUGH */
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		return read_escaped_oct(c);
	case 'x':
		return (int)read_escaped_hex(c);
	case '\n':
		return -3;
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
		return c;
	}
}

/*
 * Read a character which is part of a character constant or of a string
 * and handle escapes.
 *
 * 'delim' is '\'' for character constants and '"' for string literals.
 *
 * Returns -1 if the end of the character constant or string is reached,
 * -2 if the EOF is reached, and the character otherwise.
 */
static int
get_escaped_char(int delim)
{

	int c = prev_byte;
	if (c != -1)
		prev_byte = -1;
	else
		c = read_byte();

	if (c == delim)
		return -1;
	switch (c) {
	case '\n':
		if (!allow_c90) {
			/* newline in string or char constant */
			error(254);
			return -2;
		}
		return c;
	case '\0':
		/* syntax error '%s' */
		error(249, "EOF or null byte in literal");
		return -2;
	case EOF:
		return -2;
	case '\\':
		c = read_escaped_backslash(delim);
		if (c == -3)
			return get_escaped_char(delim);
	}
	return c;
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
		val = (int)((unsigned int)val << CHAR_SIZE) + c;
		n++;
	}
	if (c == -2) {
		/* unterminated character constant */
		error(253);
	} else if (n > sizeof(int) || (n > 1 && (pflag || hflag))) {
		/*
		 * XXX: ^^ should rather be sizeof(TARG_INT). Luckily,
		 * sizeof(int) is the same on all supported platforms.
		 */
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
			fnl = 16;	/* strlen (fn) */
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
	int c;
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

	bool seen_end_of_comment = false;

	/* Skip whitespace after the start of the comment */
	while (c = read_byte(), isspace(c))
		continue;

	/* Read the potential keyword to keywd */
	l = 0;
	while (c != EOF && l < sizeof(keywd) - 1 &&
	    (isalpha(c) || isspace(c))) {
		if (islower(c) && l > 0 && ch_isupper(keywd[0]))
			break;
		keywd[l++] = (char)c;
		c = read_byte();
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
		c = read_byte();

	/* read the argument, if the keyword accepts one and there is one */
	l = 0;
	if (keywtab[i].arg) {
		while (isdigit(c) && l < sizeof(arg) - 1) {
			arg[l++] = (char)c;
			c = read_byte();
		}
	}
	arg[l] = '\0';
	a = l != 0 ? atoi(arg) : -1;

	/* skip whitespace after the argument */
	while (isspace(c))
		c = read_byte();

	seen_end_of_comment = c == '*' && (c = read_byte()) == '/';
	if (!seen_end_of_comment && keywtab[i].func != linted)
		/* extra characters in lint comment */
		warning(257);

	if (keywtab[i].func != NULL)
		keywtab[i].func(a);

skip_rest:
	while (!seen_end_of_comment) {
		int lc = c;
		if ((c = read_byte()) == EOF) {
			/* unterminated comment */
			error(256);
			break;
		}
		if (lc == '*' && c == '/')
			seen_end_of_comment = true;
	}
}

void
lex_slash_slash_comment(void)
{
	int c;

	if (!allow_c99 && !allow_gcc)
		/* %s does not support // comments */
		gnuism(312, allow_c90 ? "C90" : "traditional C");

	while ((c = read_byte()) != EOF && c != '\n')
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

int
lex_string(void)
{
	unsigned char *s;
	int	c;
	size_t	len, max;

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

	strg_t *strg = xcalloc(1, sizeof(*strg));
	strg->st_char = true;
	strg->st_len = len;
	strg->st_mem = s;

	yylval.y_string = strg;
	return T_STRING;
}

int
lex_wide_string(void)
{
	int	c, n;

	size_t len = 0, max = 64;
	char *s = xmalloc(max);
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
	size_t wlen = 0;
	for (size_t i = 0; i < len; i += n, wlen++) {
		if ((n = mblen(&s[i], MB_CUR_MAX)) == -1) {
			/* invalid multibyte character */
			error(291);
			break;
		}
		if (n == 0)
			n = 1;
	}

	wchar_t	*ws = xmalloc((wlen + 1) * sizeof(*ws));
	size_t wi = 0;
	/* convert from multibyte to wide char */
	(void)mbtowc(NULL, NULL, 0);
	for (size_t i = 0; i < len; i += n, wi++) {
		if ((n = mbtowc(&ws[wi], &s[i], MB_CUR_MAX)) == -1)
			break;
		if (n == 0)
			n = 1;
	}
	ws[wi] = 0;
	free(s);

	strg_t *strg = xcalloc(1, sizeof(*strg));
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

	sym_t *sym = sb->sb_sym;

	/*
	 * During member declaration it is possible that name() looked
	 * for symbols of type FVFT, although it should have looked for
	 * symbols of type FTAG. Same can happen for labels. Both cases
	 * are compensated here.
	 */
	if (symtyp == FMEMBER || symtyp == FLABEL) {
		if (sym == NULL || sym->s_kind == FVFT)
			sym = symtab_search(sb->sb_name);
	}

	if (sym != NULL) {
		lint_assert(sym->s_kind == symtyp);
		symtyp = FVFT;
		free(sb);
		return sym;
	}

	/* create a new symbol table entry */

	/* labels must always be allocated at level 1 (outermost block) */
	dinfo_t	*di;
	if (symtyp == FLABEL) {
		sym = level_zero_alloc(1, sizeof(*sym));
		char *s = level_zero_alloc(1, sb->sb_len + 1);
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

	if (!in_gcc_attribute) {
		symtab_add(sym);

		*di->d_ldlsym = sym;
		di->d_ldlsym = &sym->s_level_next;
	}

	free(sb);
	return sym;
}

/*
 * Construct a temporary symbol. The symbol name starts with a digit to avoid
 * name clashes with other identifiers.
 */
sym_t *
mktempsym(type_t *tp)
{
	static unsigned n = 0;
	char *s = level_zero_alloc((size_t)block_level, 64);
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

/* Put a symbol into the symbol table. */
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
	const sym_t *next = sym->s_symtab_next;
	if (next != NULL)
		lint_assert(sym->s_block_level >= next->s_block_level);
}

/* Called at level 0 after syntax errors. */
void
clean_up_after_error(void)
{

	symtab_remove_locals();

	while (mem_block_level > 0)
		level_free_all(mem_block_level--);
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
