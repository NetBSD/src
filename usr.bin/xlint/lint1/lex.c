/* $NetBSD: lex.c,v 1.223 2024/03/29 08:35:32 rillig Exp $ */

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
 *	This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
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
__RCSID("$NetBSD: lex.c,v 1.223 2024/03/29 08:35:32 rillig Exp $");
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
pos_t curr_pos = { "", 1, 0 };

/*
 * Current position in C source (not updated when an included file is
 * parsed).
 */
pos_t csrc_pos = { "", 1, 0 };

bool in_gcc_attribute;
bool in_system_header;

/*
 * Valid values for 'since' are 78, 90, 99, 11, 23.
 *
 * The C11 keywords are all taken from the reserved namespace.  They are added
 * in C99 mode as well, to make the parse error messages more useful.  For
 * example, if the keyword '_Generic' were not defined, it would be interpreted
 * as an implicit function call, leading to a parse error.
 *
 * The C23 keywords are not made available in earlier modes, as they may
 * conflict with user-defined identifiers.
 */
#define kwdef(name, token, detail,	since, gcc, deco) \
	{ /* CONSTCOND */ \
		name, token, detail, \
		(since) == 90, \
		(since) == 99 || (since) == 11, \
		(since) == 23, \
		(gcc) > 0, \
		((deco) & 1) != 0, ((deco) & 2) != 0, ((deco) & 4) != 0, \
	}
#define kwdef_token(name, token,		since, gcc, deco) \
	kwdef(name, token, {false},		since, gcc, deco)
#define kwdef_sclass(name, sclass,		since, gcc, deco) \
	kwdef(name, T_SCLASS, .u.kw_scl = (sclass), since, gcc, deco)
#define kwdef_type(name, tspec,			since) \
	kwdef(name, T_TYPE, .u.kw_tspec = (tspec), since, 0, 1)
#define kwdef_tqual(name, tqual,		since, gcc, deco) \
	kwdef(name, T_QUAL, .u.kw_tqual = {.tqual = true}, since, gcc, deco)
#define kwdef_keyword(name, token) \
	kwdef(name, token, {false},		78, 0, 1)

/* During initialization, these keywords are written to the symbol table. */
static const struct keyword {
	const	char kw_name[20];
	int	kw_token;	/* token to be returned by yylex() */
	union {
		bool kw_dummy;
		scl_t kw_scl;		/* if kw_token is T_SCLASS */
		tspec_t kw_tspec;	/* if kw_token is T_TYPE or
					 * T_STRUCT_OR_UNION */
		type_qualifiers kw_tqual;	/* if kw_token is T_QUAL */
		function_specifier kw_fs;	/* if kw_token is
						 * T_FUNCTION_SPECIFIER */
	} u;
	bool	kw_added_in_c90:1;
	bool	kw_added_in_c99_or_c11:1;
	bool	kw_added_in_c23:1;
	bool	kw_gcc:1;	/* available in GCC mode */
	bool	kw_plain:1;	/* 'name' */
	bool	kw_leading:1;	/* '__name' */
	bool	kw_both:1;	/* '__name__' */
} keywords[] = {
	// TODO: _Alignas is not available in C99.
	kwdef_keyword(	"_Alignas",	T_ALIGNAS),
	// TODO: _Alignof is not available in C99.
	kwdef_keyword(	"_Alignof",	T_ALIGNOF),
	// TODO: alignof is not available in C99.
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
	kwdef_tqual(	"const",	tq_const,		90,0,7),
	kwdef_keyword(	"continue",	T_CONTINUE),
	kwdef_keyword(	"default",	T_DEFAULT),
	kwdef_keyword(	"do",		T_DO),
	kwdef_type(	"double",	DOUBLE,			78),
	kwdef_keyword(	"else",		T_ELSE),
	// XXX: enum is not available in traditional C.
	kwdef_keyword(	"enum",		T_ENUM),
	kwdef_token(	"__extension__",T_EXTENSION,		78,1,1),
	kwdef_sclass(	"extern",	EXTERN,			78,0,1),
	kwdef_type(	"float",	FLOAT,			78),
	kwdef_keyword(	"for",		T_FOR),
	kwdef_token(	"_Generic",	T_GENERIC,		11,0,1),
	kwdef_keyword(	"goto",		T_GOTO),
	kwdef_keyword(	"if",		T_IF),
	kwdef_token(	"__imag__",	T_IMAG,			78,1,1),
	kwdef("inline",	T_FUNCTION_SPECIFIER, .u.kw_fs = FS_INLINE, 99,0,7),
	kwdef_type(	"int",		INT,			78),
#ifdef INT128_SIZE
	kwdef_type(	"__int128_t",	INT128,			99),
#endif
	kwdef_type(	"long",		LONG,			78),
	kwdef("_Noreturn", T_FUNCTION_SPECIFIER, .u.kw_fs = FS_NORETURN, 11,0,1),
	// XXX: __packed is GCC-specific.
	kwdef_token(	"__packed",	T_PACKED,		78,0,1),
	kwdef_token(	"__real__",	T_REAL,			78,1,1),
	kwdef_sclass(	"register",	REG,			78,0,1),
	kwdef_tqual(	"restrict",	tq_restrict,		99,0,7),
	kwdef_keyword(	"return",	T_RETURN),
	kwdef_type(	"short",	SHORT,			78),
	kwdef(		"signed", T_TYPE, .u.kw_tspec = SIGNED,	90,0,3),
	kwdef_keyword(	"sizeof",	T_SIZEOF),
	kwdef_sclass(	"static",	STATIC,			78,0,1),
	// XXX: _Static_assert was added in C11.
	kwdef_keyword(	"_Static_assert",	T_STATIC_ASSERT),
	kwdef("struct",	T_STRUCT_OR_UNION, .u.kw_tspec = STRUCT, 78,0,1),
	kwdef_keyword(	"switch",	T_SWITCH),
	kwdef_token(	"__symbolrename",	T_SYMBOLRENAME,	78,0,1),
	kwdef_sclass(	"__thread",	THREAD_LOCAL,		78,1,1),
	kwdef_sclass(	"_Thread_local", THREAD_LOCAL,		11,0,1),
	kwdef_sclass(	"thread_local", THREAD_LOCAL,		23,0,1),
	kwdef_sclass(	"typedef",	TYPEDEF,		78,0,1),
	kwdef_token(	"typeof",	T_TYPEOF,		78,1,7),
#ifdef INT128_SIZE
	kwdef_type(	"__uint128_t",	UINT128,		99),
#endif
	kwdef("union",	T_STRUCT_OR_UNION, .u.kw_tspec = UNION,	78,0,1),
	kwdef_type(	"unsigned",	UNSIGN,			78),
	// XXX: void is not available in traditional C.
	kwdef_type(	"void",		VOID,			78),
	kwdef_tqual(	"volatile",	tq_volatile,		90,0,7),
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
static sym_t *symtab[503];

/*
 * The kind of the next expected symbol, to distinguish the namespaces of
 * members, labels, type tags and other identifiers.
 */
symbol_kind sym_kind;


static unsigned int
hash(const char *s)
{
	unsigned int v = 0;
	for (const char *p = s; *p != '\0'; p++) {
		v = (v << 4) + (unsigned char)*p;
		v ^= v >> 28;
	}
	return v % (sizeof(symtab) / sizeof(symtab[0]));
}

static void
symtab_add(sym_t *sym)
{
	unsigned int h = hash(sym->s_name);
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
		if (sym->s_keyword != NULL ||
		    sym->s_kind == sym_kind ||
		    in_gcc_attribute)
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

	for (size_t i = 0; i < sizeof(symtab) / sizeof(symtab[0]); i++) {
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

	debug_enter();
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
			debug_step("symbol table level %d", level);
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
	debug_leave();

	free(syms.items);
}
#endif

static void
register_keyword(const struct keyword *kw, bool leading, bool trailing)
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

	sym_t *sym = block_zero_alloc(sizeof(*sym), "sym");
	sym->s_name = name;
	sym->s_keyword = kw;
	int tok = kw->kw_token;
	sym->u.s_keyword.sk_token = tok;
	if (tok == T_TYPE || tok == T_STRUCT_OR_UNION)
		sym->u.s_keyword.u.sk_tspec = kw->u.kw_tspec;
	if (tok == T_SCLASS)
		sym->s_scl = kw->u.kw_scl;
	if (tok == T_QUAL)
		sym->u.s_keyword.u.sk_type_qualifier = kw->u.kw_tqual;
	if (tok == T_FUNCTION_SPECIFIER)
		sym->u.s_keyword.u.function_specifier = kw->u.kw_fs;

	symtab_add(sym);
}

static bool
is_keyword_known(const struct keyword *kw)
{

	if (kw->kw_added_in_c23 && !allow_c23)
		return false;
	if ((kw->kw_added_in_c90 || kw->kw_added_in_c99_or_c11) && !allow_c90)
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

	if (kw->kw_added_in_c99_or_c11 && !allow_c99)
		return false;
	return true;
}

/* Write all keywords to the symbol table. */
void
init_lex(void)
{

	size_t n = sizeof(keywords) / sizeof(keywords[0]);
	for (size_t i = 0; i < n; i++) {
		const struct keyword *kw = keywords + i;
		if (!is_keyword_known(kw))
			continue;
		if (kw->kw_plain)
			register_keyword(kw, false, false);
		if (kw->kw_leading)
			register_keyword(kw, true, false);
		if (kw->kw_both)
			register_keyword(kw, true, true);
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
	int c = lex_input();

	if (c == '\n')
		lex_next_line();
	return c == '\0' ? EOF : c;	/* lex returns 0 on EOF. */
}

static int
lex_keyword(sym_t *sym)
{
	int tok = sym->u.s_keyword.sk_token;

	if (tok == T_SCLASS)
		yylval.y_scl = sym->s_scl;
	if (tok == T_TYPE || tok == T_STRUCT_OR_UNION)
		yylval.y_tspec = sym->u.s_keyword.u.sk_tspec;
	if (tok == T_QUAL)
		yylval.y_type_qualifiers =
		    sym->u.s_keyword.u.sk_type_qualifier;
	if (tok == T_FUNCTION_SPECIFIER)
		yylval.y_function_specifier =
		    sym->u.s_keyword.u.function_specifier;
	return tok;
}

/*
 * Look up the definition of a name in the symbol table. This symbol must
 * either be a keyword or a symbol of the type required by sym_kind (label,
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

	char *name = block_zero_alloc(yyleng + 1, "string");
	(void)memcpy(name, yytext, yyleng + 1);
	sb->sb_name = name;
	return T_NAME;
}

static tspec_t
integer_constant_type_signed(unsigned ls, uint64_t ui, int base, bool warned)
{
	if (ls == 0 && ui <= TARG_INT_MAX)
		return INT;
	if (ls == 0 && ui <= TARG_UINT_MAX && base != 10 && allow_c90)
		return UINT;
	if (ls == 0 && ui <= TARG_LONG_MAX)
		return LONG;

	if (ls <= 1 && ui <= TARG_LONG_MAX)
		return LONG;
	if (ls <= 1 && ui <= TARG_ULONG_MAX && base != 10)
		return allow_c90 ? ULONG : LONG;
	if (ls <= 1 && !allow_c99) {
		if (!warned)
			/* integer constant out of range */
			warning(252);
		return allow_c90 ? ULONG : LONG;
	}

	if (ui <= TARG_LLONG_MAX)
		return LLONG;
	if (ui <= TARG_ULLONG_MAX && base != 10)
		return allow_c90 ? ULLONG : LLONG;
	if (!warned)
		/* integer constant out of range */
		warning(252);
	return allow_c90 ? ULLONG : LLONG;
}

static tspec_t
integer_constant_type_unsigned(unsigned l, uint64_t ui, bool warned)
{
	if (l == 0 && ui <= TARG_UINT_MAX)
		return UINT;

	if (l <= 1 && ui <= TARG_ULONG_MAX)
		return ULONG;
	if (l <= 1 && !allow_c99) {
		if (!warned)
			/* integer constant out of range */
			warning(252);
		return ULONG;
	}

	if (ui <= TARG_ULLONG_MAX)
		return ULLONG;
	if (!warned)
		/* integer constant out of range */
		warning(252);
	return ULLONG;
}

int
lex_integer_constant(const char *yytext, size_t yyleng, int base)
{
	const char *cp = yytext;
	size_t len = yyleng;

	/* skip 0[xX] or 0[bB] */
	if (base == 16 || base == 2) {
		cp += 2;
		len -= 2;
	}

	/* read suffixes */
	unsigned l_suffix = 0, u_suffix = 0;
	for (;; len--) {
		char c = cp[len - 1];
		if (c == 'l' || c == 'L')
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
	if (!allow_c90 && u_suffix > 0)
		/* suffix 'U' is illegal in traditional C */
		warning(97);

	bool warned = false;
	errno = 0;
	char *eptr;
	uint64_t ui = (uint64_t)strtoull(cp, &eptr, base);
	lint_assert(eptr == cp + len);
	if (errno != 0) {
		/* integer constant out of range */
		warning(252);
		warned = true;
	}

	if (any_query_enabled && base == 8 && ui != 0)
		/* octal number '%.*s' */
		query_message(8, (int)len, cp);

	bool unsigned_since_c90 = allow_trad && allow_c90 && u_suffix == 0
	    && ui > TARG_INT_MAX
	    && ((l_suffix == 0 && base != 10 && ui <= TARG_UINT_MAX)
		|| (l_suffix <= 1 && ui > TARG_LONG_MAX));

	tspec_t t = u_suffix > 0
	    ? integer_constant_type_unsigned(l_suffix, ui, warned)
	    : integer_constant_type_signed(l_suffix, ui, base, warned);
	ui = (uint64_t)convert_integer((int64_t)ui, t, size_in_bits(t));

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = t;
	yylval.y_val->v_unsigned_since_c90 = unsigned_since_c90;
	yylval.y_val->u.integer = (int64_t)ui;

	return T_CON;
}

/* Extend or truncate si to match t.  If t is signed, sign-extend. */
int64_t
convert_integer(int64_t si, tspec_t t, unsigned int bits)
{

	uint64_t vbits = value_bits(bits);
	uint64_t ui = (uint64_t)si;
	return t == PTR || is_uinteger(t) || ((ui & bit(bits - 1)) == 0)
	    ? (int64_t)(ui & vbits)
	    : (int64_t)(ui | ~vbits);
}

int
lex_floating_constant(const char *yytext, size_t yyleng)
{
	const char *cp = yytext;
	size_t len = yyleng;

	bool imaginary = cp[len - 1] == 'i';
	if (imaginary)
		len--;

	char c = cp[len - 1];
	tspec_t t;
	if (c == 'f' || c == 'F') {
		t = imaginary ? FCOMPLEX : FLOAT;
		len--;
	} else if (c == 'l' || c == 'L') {
		t = imaginary ? LCOMPLEX : LDOUBLE;
		len--;
	} else
		t = imaginary ? DCOMPLEX : DOUBLE;

	if (!allow_c90 && t != DOUBLE)
		/* suffixes 'F' and 'L' are illegal in traditional C */
		warning(98);

	errno = 0;
	char *eptr;
	long double ld = strtold(cp, &eptr);
	lint_assert(eptr == cp + len);
	if (errno != 0)
		/* floating-point constant out of range */
		warning(248);
	else if (t == FLOAT) {
		ld = (float)ld;
		if (isfinite(ld) == 0) {
			/* floating-point constant out of range */
			warning(248);
			ld = ld > 0 ? FLT_MAX : -FLT_MAX;
		}
	} else if (t == DOUBLE
	    || /* CONSTCOND */ LDOUBLE_SIZE == DOUBLE_SIZE) {
		ld = (double)ld;
		if (isfinite(ld) == 0) {
			/* floating-point constant out of range */
			warning(248);
			ld = ld > 0 ? DBL_MAX : -DBL_MAX;
		}
	}

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = t;
	yylval.y_val->u.floating = ld;

	return T_CON;
}

int
lex_operator(int t, op_t o)
{

	yylval.y_op = o;
	return t;
}

static buffer *
read_quoted(bool *complete, char delim, bool wide)
{
	buffer *buf = xcalloc(1, sizeof(*buf));
	buf_init(buf);
	if (wide)
		buf_add_char(buf, 'L');
	buf_add_char(buf, delim);

	for (;;) {
		int c = read_byte();
		if (c <= 0)
			break;
		buf_add_char(buf, (char)c);
		if (c == '\n')
			break;
		if (c == delim) {
			*complete = true;
			return buf;
		}
		if (c == '\\') {
			c = read_byte();
			buf_add_char(buf, (char)(c <= 0 ? ' ' : c));
			if (c <= 0)
				break;
		}
	}
	*complete = false;
	buf_add_char(buf, delim);
	return buf;
}

/*
 * Analyze the lexical representation of the next character in the string
 * literal list. At the end, only update the position information.
 */
bool
quoted_next(const buffer *lit, quoted_iterator *it)
{
	const char *s = lit->data;

	*it = (quoted_iterator){ .start = it->end };

	char delim = s[s[0] == 'L' ? 1 : 0];

	bool in_the_middle = it->start > 0;
	if (!in_the_middle) {
		it->start = s[0] == 'L' ? 2 : 1;
		it->end = it->start;
	}

	while (s[it->start] == delim) {
		if (it->start + 1 == lit->len) {
			it->end = it->start;
			return false;
		}
		it->next_literal = in_the_middle;
		it->start += 2;
	}
	it->end = it->start;

again:
	switch (s[it->end]) {
	case '\\':
		it->end++;
		goto backslash;
	case '\n':
		it->unescaped_newline = true;
		return false;
	default:
		it->value = (unsigned char)s[it->end++];
		return true;
	}

backslash:
	it->escaped = true;
	if ('0' <= s[it->end] && s[it->end] <= '7')
		goto octal_escape;
	switch (s[it->end++]) {
	case '\n':
		goto again;
	case 'a':
		it->named_escape = true;
		it->value = '\a';
		it->invalid_escape = !allow_c90;
		return true;
	case 'b':
		it->named_escape = true;
		it->value = '\b';
		return true;
	case 'e':
		it->named_escape = true;
		it->value = '\033';
		it->invalid_escape = !allow_gcc;
		return true;
	case 'f':
		it->named_escape = true;
		it->value = '\f';
		return true;
	case 'n':
		it->named_escape = true;
		it->value = '\n';
		return true;
	case 'r':
		it->named_escape = true;
		it->value = '\r';
		return true;
	case 't':
		it->named_escape = true;
		it->value = '\t';
		return true;
	case 'v':
		it->named_escape = true;
		it->value = '\v';
		it->invalid_escape = !allow_c90;
		return true;
	case 'x':
		goto hex_escape;
	case '"':
		it->literal_escape = true;
		it->value = '"';
		it->invalid_escape = !allow_c90 && delim == '\'';
		return true;
	case '?':
		it->literal_escape = true;
		it->value = '?';
		it->invalid_escape = !allow_c90;
		return true;
	default:
		it->invalid_escape = true;
		/* FALLTHROUGH */
	case '\'':
	case '\\':
		it->literal_escape = true;
		it->value = (unsigned char)s[it->end - 1];
		return true;
	}

octal_escape:
	it->octal_digits++;
	it->value = s[it->end++] - '0';
	if ('0' <= s[it->end] && s[it->end] <= '7') {
		it->octal_digits++;
		it->value = 8 * it->value + (s[it->end++] - '0');
		if ('0' <= s[it->end] && s[it->end] <= '7') {
			it->octal_digits++;
			it->value = 8 * it->value + (s[it->end++] - '0');
			it->overflow = it->value > TARG_UCHAR_MAX
			    && s[0] != 'L';
		}
	}
	return true;

hex_escape:
	for (;;) {
		char ch = s[it->end];
		unsigned digit_value;
		if ('0' <= ch && ch <= '9')
			digit_value = ch - '0';
		else if ('A' <= ch && ch <= 'F')
			digit_value = 10 + (ch - 'A');
		else if ('a' <= ch && ch <= 'f')
			digit_value = 10 + (ch - 'a');
		else
			break;

		it->end++;
		it->value = 16 * it->value + digit_value;
		uint64_t limit = s[0] == 'L' ? TARG_UINT_MAX : TARG_UCHAR_MAX;
		if (it->value > limit)
			it->overflow = true;
		if (it->hex_digits < 3)
			it->hex_digits++;
	}
	it->missing_hex_digits = it->hex_digits == 0;
	return true;
}

static void
check_quoted(const buffer *buf, bool complete, char delim)
{
	quoted_iterator it = { .end = 0 }, prev = it;
	for (; quoted_next(buf, &it); prev = it) {
		if (it.missing_hex_digits)
			/* no hex digits follow \x */
			error(74);
		if (it.hex_digits > 0 && !allow_c90)
			/* \x undefined in traditional C */
			warning(82);
		else if (!it.invalid_escape)
			;
		else if (it.value == '8' || it.value == '9')
			/* bad octal digit '%c' */
			warning(77, (int)it.value);
		else if (it.literal_escape && it.value == '?')
			/* \? undefined in traditional C */
			warning(263);
		else if (it.literal_escape && it.value == '"')
			/* \" inside character constants undefined in ... */
			warning(262);
		else if (it.named_escape && it.value == '\a')
			/* \a undefined in traditional C */
			warning(81);
		else if (it.named_escape && it.value == '\v')
			/* \v undefined in traditional C */
			warning(264);
		else {
			unsigned char ch = buf->data[it.end - 1];
			if (isprint(ch))
				/* dubious escape \%c */
				warning(79, ch);
			else
				/* dubious escape \%o */
				warning(80, ch);
		}
		if (it.overflow && it.hex_digits > 0)
			/* overflow in hex escape */
			warning(75);
		if (it.overflow && it.octal_digits > 0)
			/* character escape does not fit in character */
			warning(76);
		if (it.value < ' ' && !it.escaped && complete)
			/* invisible character U+%04X in %s */
			query_message(17, (unsigned)it.value, delim == '"'
			    ? "string literal" : "character constant");
		if (prev.octal_digits > 0 && prev.octal_digits < 3
		    && !it.escaped && it.value >= '8' && it.value <= '9')
			/* short octal escape '%.*s' followed by digit '%c' */
			warning(356, (int)(prev.end - prev.start),
			    buf->data + prev.start, buf->data[it.start]);
	}
	if (it.unescaped_newline)
		/* newline in string or char constant */
		error(254);
	if (!complete && delim == '"')
		/* unterminated string constant */
		error(258);
	if (!complete && delim == '\'')
		/* unterminated character constant */
		error(253);
}

static buffer *
lex_quoted(char delim, bool wide)
{
	bool complete;
	buffer *buf = read_quoted(&complete, delim, wide);
	check_quoted(buf, complete, delim);
	return buf;
}

/* Called if lex found a leading "'". */
int
lex_character_constant(void)
{
	buffer *buf = lex_quoted('\'', false);

	size_t n = 0;
	uint64_t val = 0;
	quoted_iterator it = { .end = 0 };
	while (quoted_next(buf, &it)) {
		val = (val << CHAR_SIZE) + it.value;
		n++;
	}
	if (n > sizeof(int) || (n > 1 && (pflag || hflag))) {
		/*
		 * XXX: ^^ should rather be sizeof(TARG_INT). Luckily,
		 * sizeof(int) is the same on all supported platforms.
		 */
		/* too many characters in character constant */
		error(71);
	} else if (n > 1)
		/* multi-character character constant */
		warning(294);
	else if (n == 0 && !it.unescaped_newline)
		/* empty character constant */
		error(73);

	int64_t cval = n == 1
	    ? convert_integer((int64_t)val, CHAR, CHAR_SIZE)
	    : (int64_t)val;

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = INT;
	yylval.y_val->v_char_constant = true;
	yylval.y_val->u.integer = cval;

	return T_CON;
}

/* Called if lex found a leading "L'". */
int
lex_wide_character_constant(void)
{
	buffer *buf = lex_quoted('\'', true);

	static char wbuf[MB_LEN_MAX + 1];
	size_t n = 0, nmax = MB_CUR_MAX;

	quoted_iterator it = { .end = 0 };
	while (quoted_next(buf, &it)) {
		if (n < nmax)
			wbuf[n] = (char)it.value;
		n++;
	}

	wchar_t wc = 0;
	if (n == 0)
		/* empty character constant */
		error(73);
	else if (n > nmax) {
		n = nmax;
		/* too many characters in character constant */
		error(71);
	} else {
		wbuf[n] = '\0';
		(void)mbtowc(NULL, NULL, 0);
		if (mbtowc(&wc, wbuf, nmax) < 0)
			/* invalid multibyte character */
			error(291);
	}

	yylval.y_val = xcalloc(1, sizeof(*yylval.y_val));
	yylval.y_val->v_tspec = WCHAR_TSPEC;
	yylval.y_val->v_char_constant = true;
	yylval.y_val->u.integer = wc;

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
		while (isspace((unsigned char)*p))
			p++;

		const char *word = p;
		while (*p != '\0' && !isspace((unsigned char)*p))
			p++;
		size_t len = (size_t)(p - word);

		if (len == 1 && word[0] == '1')
			*is_begin = true;
		if (len == 1 && word[0] == '2')
			*is_end = true;
		if (len == 1 && word[0] == '3')
			*is_system = true;
		/* Flag '4' is only interesting for C++. */
	}
}

/*
 * The first directive of the preprocessed translation unit provides the name
 * of the C source file as specified at the command line.
 */
static void
set_csrc_pos(void)
{
	static bool done;

	if (done)
		return;
	done = true;
	csrc_pos.p_file = curr_pos.p_file;
	outsrc(transform_filename(curr_pos.p_file, strlen(curr_pos.p_file)));
}

/*
 * Called for preprocessor directives. Currently implemented are:
 *	# pragma [argument...]
 *	# lineno
 *	# lineno "filename" [GCC-flag...]
 */
void
lex_directive(const char *yytext)
{
	const char *p = yytext + 1;	/* skip '#' */

	while (*p == ' ' || *p == '\t')
		p++;

	if (!isdigit((unsigned char)*p)) {
		if (strncmp(p, "pragma", 6) == 0
		    && isspace((unsigned char)p[6]))
			return;
		goto error;
	}

	char *end;
	long ln = strtol(--p, &end, 10);
	if (end == p)
		goto error;
	p = end;

	if (*p != ' ' && *p != '\t' && *p != '\0')
		goto error;
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p != '\0') {
		if (*p != '"')
			goto error;
		const char *fn = ++p;
		while (*p != '"' && *p != '\0')
			p++;
		if (*p != '"')
			goto error;
		size_t fn_len = p++ - fn;
		if (fn_len > PATH_MAX)
			goto error;
		if (fn_len == 0) {
			fn = "{standard input}";
			fn_len = strlen(fn);
		}
		curr_pos.p_file = record_filename(fn, fn_len);
		set_csrc_pos();

		bool is_begin, is_end, is_system;
		parse_line_directive_flags(p, &is_begin, &is_end, &is_system);
		update_location(curr_pos.p_file, (int)ln, is_begin, is_end);
		in_system_header = is_system;
	}
	curr_pos.p_line = (int)ln - 1;
	curr_pos.p_uniq = 0;
	if (curr_pos.p_file == csrc_pos.p_file) {
		csrc_pos.p_line = (int)ln - 1;
		csrc_pos.p_uniq = 0;
	}
	return;

error:
	/* undefined or invalid '#' directive */
	warning(255);
}

/* Handle lint comments such as ARGSUSED. */
void
lex_comment(void)
{
	int c;
	static const struct {
		const	char name[18];
		bool	arg;
		lint_comment comment;
	} keywtab[] = {
		{ "ARGSUSED",		true,	LC_ARGSUSED	},
		{ "BITFIELDTYPE",	false,	LC_BITFIELDTYPE	},
		{ "CONSTCOND",		false,	LC_CONSTCOND	},
		{ "CONSTANTCOND",	false,	LC_CONSTCOND	},
		{ "CONSTANTCONDITION",	false,	LC_CONSTCOND	},
		{ "FALLTHRU",		false,	LC_FALLTHROUGH	},
		{ "FALLTHROUGH",	false,	LC_FALLTHROUGH	},
		{ "FALL THROUGH",	false,	LC_FALLTHROUGH	},
		{ "fallthrough",	false,	LC_FALLTHROUGH	},
		{ "LINTLIBRARY",	false,	LC_LINTLIBRARY	},
		{ "LINTED",		true,	LC_LINTED	},
		{ "LONGLONG",		false,	LC_LONGLONG	},
		{ "NOSTRICT",		true,	LC_LINTED	},
		{ "NOTREACHED",		false,	LC_NOTREACHED	},
		{ "PRINTFLIKE",		true,	LC_PRINTFLIKE	},
		{ "PROTOLIB",		true,	LC_PROTOLIB	},
		{ "SCANFLIKE",		true,	LC_SCANFLIKE	},
		{ "VARARGS",		true,	LC_VARARGS	},
	};
	char keywd[32];

	bool seen_end_of_comment = false;

	while (c = read_byte(), isspace(c))
		continue;

	/* Read the potential keyword to keywd */
	size_t l = 0;
	while (c != EOF && l < sizeof(keywd) - 1 &&
	    (isalpha(c) || isspace(c))) {
		if (islower(c) && l > 0 && isupper((unsigned char)keywd[0]))
			break;
		keywd[l++] = (char)c;
		c = read_byte();
	}
	while (l > 0 && isspace((unsigned char)keywd[l - 1]))
		l--;
	keywd[l] = '\0';

	/* look for the keyword */
	size_t i;
	for (i = 0; i < sizeof(keywtab) / sizeof(keywtab[0]); i++)
		if (strcmp(keywtab[i].name, keywd) == 0)
			goto found_keyword;
	goto skip_rest;

found_keyword:
	while (isspace(c))
		c = read_byte();

	/* read the argument, if the keyword accepts one and there is one */
	char arg[32];
	l = 0;
	if (keywtab[i].arg) {
		while (isdigit(c) && l < sizeof(arg) - 1) {
			arg[l++] = (char)c;
			c = read_byte();
		}
	}
	arg[l] = '\0';
	int a = l != 0 ? atoi(arg) : -1;

	while (isspace(c))
		c = read_byte();

	seen_end_of_comment = c == '*' && (c = read_byte()) == '/';
	if (!seen_end_of_comment && keywtab[i].comment != LC_LINTED)
		/* extra characters in lint comment */
		warning(257);

	handle_lint_comment(keywtab[i].comment, a);

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

	if (!allow_c99 && !allow_gcc)
		/* %s does not support '//' comments */
		gnuism(312, allow_c90 ? "C90" : "traditional C");

	for (int c; c = read_byte(), c != EOF && c != '\n';)
		continue;
}

void
reset_suppressions(void)
{

	lwarn = LWARN_ALL;
	suppress_longlong = false;
	suppress_constcond = false;
}

int
lex_string(void)
{
	yylval.y_string = lex_quoted('"', false);
	return T_STRING;
}

static size_t
wide_length(const buffer *buf)
{

	(void)mblen(NULL, 0);
	size_t len = 0, i = 0;
	while (i < buf->len) {
		int n = mblen(buf->data + i, MB_CUR_MAX);
		if (n == -1) {
			/* invalid multibyte character */
			error(291);
			break;
		}
		i += n > 1 ? n : 1;
		len++;
	}
	return len;
}

int
lex_wide_string(void)
{
	buffer *buf = lex_quoted('"', true);

	buffer str;
	buf_init(&str);
	quoted_iterator it = { .end = 0 };
	while (quoted_next(buf, &it))
		buf_add_char(&str, (char)it.value);

	free(buf->data);
	*buf = (buffer) { .len = wide_length(&str) };

	yylval.y_string = buf;
	return T_STRING;
}

void
lex_next_line(void)
{
	curr_pos.p_line++;
	curr_pos.p_uniq = 0;
	debug_skip_indent();
	debug_printf("parsing %s:%d\n", curr_pos.p_file, curr_pos.p_line);
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
	 * During member declaration it is possible that name() looked for
	 * symbols of type SK_VCFT, although it should have looked for symbols
	 * of type SK_TAG. Same can happen for labels. Both cases are
	 * compensated here.
	 */
	if (sym_kind == SK_MEMBER || sym_kind == SK_LABEL) {
		if (sym == NULL || sym->s_kind == SK_VCFT)
			sym = symtab_search(sb->sb_name);
	}

	if (sym != NULL) {
		lint_assert(sym->s_kind == sym_kind);
		set_sym_kind(SK_VCFT);
		free(sb);
		return sym;
	}

	/* create a new symbol table entry */

	decl_level *dl;
	if (sym_kind == SK_LABEL) {
		sym = level_zero_alloc(1, sizeof(*sym), "sym");
		char *s = level_zero_alloc(1, sb->sb_len + 1, "string");
		(void)memcpy(s, sb->sb_name, sb->sb_len + 1);
		sym->s_name = s;
		sym->s_block_level = 1;
		dl = dcs;
		while (dl->d_enclosing != NULL &&
		    dl->d_enclosing->d_enclosing != NULL)
			dl = dl->d_enclosing;
		lint_assert(dl->d_kind == DLK_AUTO);
	} else {
		sym = block_zero_alloc(sizeof(*sym), "sym");
		sym->s_name = sb->sb_name;
		sym->s_block_level = block_level;
		dl = dcs;
	}

	sym->s_def_pos = unique_curr_pos();
	if ((sym->s_kind = sym_kind) != SK_LABEL)
		sym->s_type = gettyp(INT);

	set_sym_kind(SK_VCFT);

	if (!in_gcc_attribute) {
		debug_printf("%s: symtab_add ", __func__);
		debug_sym("", sym, "\n");
		symtab_add(sym);

		*dl->d_last_dlsym = sym;
		dl->d_last_dlsym = &sym->s_level_next;
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
	char *s = level_zero_alloc((size_t)block_level, 64, "string");
	sym_t *sym = block_zero_alloc(sizeof(*sym), "sym");
	scl_t scl;

	(void)snprintf(s, 64, "%.8u_tmp", n++);

	scl = dcs->d_scl;
	if (scl == NO_SCL)
		scl = block_level > 0 ? AUTO : EXTERN;

	sym->s_name = s;
	sym->s_type = tp;
	sym->s_block_level = block_level;
	sym->s_scl = scl;
	sym->s_kind = SK_VCFT;
	sym->s_used = true;
	sym->s_set = true;

	symtab_add(sym);

	*dcs->d_last_dlsym = sym;
	dcs->d_last_dlsym = &sym->s_level_next;

	return sym;
}

void
symtab_remove_forever(sym_t *sym)
{

	debug_step("%s '%s' %s '%s'", __func__,
	    sym->s_name, symbol_kind_name(sym->s_kind),
	    type_name(sym->s_type));
	symtab_remove(sym);

	/* avoid that the symbol will later be put back to the symbol table */
	sym->s_block_level = -1;
}

/*
 * Remove all symbols from the symbol table that have the same level as the
 * given symbol.
 */
void
symtab_remove_level(sym_t *syms)
{

	if (syms != NULL)
		debug_step("%s %d", __func__, syms->s_block_level);

	/* Note the use of s_level_next instead of s_symtab_next. */
	for (sym_t *sym = syms; sym != NULL; sym = sym->s_level_next) {
		if (sym->s_block_level != -1) {
			debug_step("%s '%s' %s '%s' %d", __func__,
			    sym->s_name, symbol_kind_name(sym->s_kind),
			    type_name(sym->s_type), sym->s_block_level);
			symtab_remove(sym);
			sym->s_symtab_ref = NULL;
		}
	}
}

/* Put a symbol into the symbol table. */
void
inssym(int level, sym_t *sym)
{

	debug_step("%s '%s' %s '%s' %d", __func__,
	    sym->s_name, symbol_kind_name(sym->s_kind),
	    type_name(sym->s_type), level);
	sym->s_block_level = level;
	symtab_add(sym);

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

	debug_step("pushdown '%s' %s '%s'",
	    sym->s_name, symbol_kind_name(sym->s_kind),
	    type_name(sym->s_type));

	sym_t *nsym = block_zero_alloc(sizeof(*nsym), "sym");
	lint_assert(sym->s_block_level <= block_level);
	nsym->s_name = sym->s_name;
	nsym->s_def_pos = unique_curr_pos();
	nsym->s_kind = sym->s_kind;
	nsym->s_block_level = block_level;

	symtab_add(nsym);

	*dcs->d_last_dlsym = nsym;
	dcs->d_last_dlsym = &nsym->s_level_next;

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
		buffer *str = *(buffer **)sp;
		free(str->data);
		free(str);
	}
}
