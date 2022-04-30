/* $NetBSD: decl.c,v 1.280 2022/04/30 22:31:23 rillig Exp $ */

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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: decl.c,v 1.280 2022/04/30 22:31:23 rillig Exp $");
#endif

#include <sys/param.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"

const	char *unnamed = "<unnamed>";

/* shared type structures for arithmetic types and void */
static	type_t	*typetab;

/* value of next enumerator during declaration of enum types */
int	enumval;

/*
 * pointer to top element of a stack which contains information local
 * to nested declarations
 */
dinfo_t	*dcs;

static	type_t	*tdeferr(type_t *, tspec_t);
static	void	settdsym(type_t *, sym_t *);
static	void	align(unsigned int, unsigned int);
static	sym_t	*newtag(sym_t *, scl_t, bool, bool);
static	bool	eqargs(const type_t *, const type_t *, bool *);
static	bool	mnoarg(const type_t *, bool *);
static	bool	check_old_style_definition(sym_t *, sym_t *);
static	bool	check_prototype_declaration(sym_t *, sym_t *);
static	sym_t	*new_style_function(sym_t *);
static	void	old_style_function(sym_t *, sym_t *);
static	void	declare_external_in_block(sym_t *);
static	bool	check_init(sym_t *);
static	void	check_argument_usage(bool, sym_t *);
static	void	check_variable_usage(bool, sym_t *);
static	void	check_label_usage(sym_t *);
static	void	check_tag_usage(sym_t *);
static	void	check_global_variable(const sym_t *);
static	void	check_global_variable_size(const sym_t *);

/*
 * initializes all global vars used in declarations
 */
void
#ifdef __sh3__
/* XXX port-sh3/56311 */
__attribute__((optimize("O0")))
#endif
initdecl(void)
{
	int i;

	/* declaration stack */
	dcs = xcalloc(1, sizeof(*dcs));
	dcs->d_kind = DK_EXTERN;
	dcs->d_ldlsym = &dcs->d_dlsyms;

	/* type information and classification */
	inittyp();

	/* shared type structures */
	typetab = xcalloc(NTSPEC, sizeof(*typetab));
	for (i = 0; i < NTSPEC; i++)
		typetab[i].t_tspec = NOTSPEC;

	/*
	 * The following two are not real types. They are only used by the
	 * parser to handle the keywords "signed" and "unsigned".
	 */
	typetab[SIGNED].t_tspec = SIGNED;
	typetab[UNSIGN].t_tspec = UNSIGN;

	typetab[BOOL].t_tspec = BOOL;
	typetab[CHAR].t_tspec = CHAR;
	typetab[SCHAR].t_tspec = SCHAR;
	typetab[UCHAR].t_tspec = UCHAR;
	typetab[SHORT].t_tspec = SHORT;
	typetab[USHORT].t_tspec = USHORT;
	typetab[INT].t_tspec = INT;
	typetab[UINT].t_tspec = UINT;
	typetab[LONG].t_tspec = LONG;
	typetab[ULONG].t_tspec = ULONG;
	typetab[QUAD].t_tspec = QUAD;
	typetab[UQUAD].t_tspec = UQUAD;
#ifdef INT128_SIZE
	typetab[INT128].t_tspec = INT128;
	typetab[UINT128].t_tspec = UINT128;
#endif
	typetab[FLOAT].t_tspec = FLOAT;
	typetab[DOUBLE].t_tspec = DOUBLE;
	typetab[LDOUBLE].t_tspec = LDOUBLE;
	typetab[VOID].t_tspec = VOID;
	/* struct, union, enum, ptr, array and func are not shared. */
	typetab[COMPLEX].t_tspec = COMPLEX;
	typetab[FCOMPLEX].t_tspec = FCOMPLEX;
	typetab[DCOMPLEX].t_tspec = DCOMPLEX;
	typetab[LCOMPLEX].t_tspec = LCOMPLEX;
}

/*
 * Returns a shared type structure for arithmetic types and void.
 *
 * It's important to duplicate this structure using block_dup_type or
 * expr_dup_type if it is to be modified (adding qualifiers or anything
 * else).
 */
type_t *
gettyp(tspec_t t)
{

	/* TODO: make the return type 'const' */
	return &typetab[t];
}

type_t *
block_dup_type(const type_t *tp)
{
	type_t	*ntp;

	ntp = block_zero_alloc(sizeof(*ntp));
	*ntp = *tp;
	return ntp;
}

/* Duplicate a type, free the allocated memory after the expression. */
type_t *
expr_dup_type(const type_t *tp)
{
	type_t	*ntp;

	ntp = expr_zero_alloc(sizeof(*ntp));
	*ntp = *tp;
	return ntp;
}

/*
 * Return the unqualified version of the type.  The returned type is freed at
 * the end of the current expression.
 *
 * See C99 6.2.5p25.
 */
type_t *
expr_unqualified_type(const type_t *tp)
{
	type_t *ntp;

	ntp = expr_zero_alloc(sizeof(*ntp));
	*ntp = *tp;
	ntp->t_const = false;
	ntp->t_volatile = false;

	/*
	 * In case of a struct or union type, the members should lose their
	 * qualifiers as well, but that would require a deep copy of the
	 * struct or union type.  This in turn would defeat the type
	 * comparison in eqtype, which simply tests whether tp1->t_str ==
	 * tp2->t_str.
	 */

	return ntp;
}

/*
 * Returns whether the argument is void or an incomplete array,
 * struct, union or enum type.
 */
bool
is_incomplete(const type_t *tp)
{
	tspec_t	t;

	if ((t = tp->t_tspec) == VOID) {
		return true;
	} else if (t == ARRAY) {
		return tp->t_incomplete_array;
	} else if (t == STRUCT || t == UNION) {
		return tp->t_str->sou_incomplete;
	} else if (t == ENUM) {
		return tp->t_enum->en_incomplete;
	}
	return false;
}

/*
 * Remember the storage class of the current declaration in dcs->d_scl
 * (the top element of the declaration stack) and detect multiple
 * storage classes.
 */
void
add_storage_class(scl_t sc)
{

	if (sc == INLINE) {
		if (dcs->d_inline)
			/* duplicate '%s' */
			warning(10, "inline");
		dcs->d_inline = true;
		return;
	}
	if (dcs->d_type != NULL || dcs->d_abstract_type != NOTSPEC ||
	    dcs->d_sign_mod != NOTSPEC || dcs->d_rank_mod != NOTSPEC) {
		/* storage class after type is obsolescent */
		warning(83);
	}
	if (dcs->d_scl == NOSCL) {
		dcs->d_scl = sc;
	} else {
		dcs->d_multiple_storage_classes = true;
	}
}

/*
 * Remember the type, modifier or typedef name returned by the parser
 * in *dcs (top element of decl stack). This information is used in
 * end_type() to build the type used for all declarators in this
 * declaration.
 *
 * If tp->t_typedef is 1, the type comes from a previously defined typename.
 * Otherwise it comes from a type specifier (int, long, ...) or a
 * struct/union/enum tag.
 */
void
add_type(type_t *tp)
{
	tspec_t	t;

	debug_step("%s: %s", __func__, type_name(tp));
	if (tp->t_typedef) {
		/*
		 * something like "typedef int a; int a b;"
		 * This should not happen with current grammar.
		 */
		lint_assert(dcs->d_type == NULL);
		lint_assert(dcs->d_abstract_type == NOTSPEC);
		lint_assert(dcs->d_sign_mod == NOTSPEC);
		lint_assert(dcs->d_rank_mod == NOTSPEC);

		dcs->d_type = tp;
		return;
	}

	t = tp->t_tspec;

	if (t == STRUCT || t == UNION || t == ENUM) {
		/*
		 * something like "int struct a ..."
		 * struct/union/enum with anything else is not allowed
		 */
		if (dcs->d_type != NULL || dcs->d_abstract_type != NOTSPEC ||
		    dcs->d_rank_mod != NOTSPEC || dcs->d_sign_mod != NOTSPEC) {
			dcs->d_invalid_type_combination = true;
			dcs->d_abstract_type = NOTSPEC;
			dcs->d_sign_mod = NOTSPEC;
			dcs->d_rank_mod = NOTSPEC;
		}
		dcs->d_type = tp;
		return;
	}

	if (dcs->d_type != NULL && !dcs->d_type->t_typedef) {
		/*
		 * something like "struct a int"
		 * struct/union/enum with anything else is not allowed
		 */
		dcs->d_invalid_type_combination = true;
		return;
	}

	if (t == COMPLEX) {
		if (dcs->d_complex_mod == FLOAT)
			t = FCOMPLEX;
		else if (dcs->d_complex_mod == DOUBLE)
			t = DCOMPLEX;
		else {
			/* invalid type for _Complex */
			error(308);
			t = DCOMPLEX; /* just as a fallback */
		}
		dcs->d_complex_mod = NOTSPEC;
	}

	if (t == LONG && dcs->d_rank_mod == LONG) {
		/* "long long" or "long ... long" */
		t = QUAD;
		dcs->d_rank_mod = NOTSPEC;
		if (!quadflg)
			/* %s does not support 'long long' */
			c99ism(265, allow_c90 ? "C90" : "traditional C");
	}

	if (dcs->d_type != NULL && dcs->d_type->t_typedef) {
		/* something like "typedef int a; a long ..." */
		dcs->d_type = tdeferr(dcs->d_type, t);
		return;
	}

	/* now it can be only a combination of arithmetic types and void */
	if (t == SIGNED || t == UNSIGN) {
		/*
		 * remember specifiers "signed" & "unsigned" in
		 * dcs->d_sign_mod
		 */
		if (dcs->d_sign_mod != NOTSPEC)
			/* more than one "signed" and/or "unsigned" */
			dcs->d_invalid_type_combination = true;
		dcs->d_sign_mod = t;
	} else if (t == SHORT || t == LONG || t == QUAD) {
		/*
		 * remember specifiers "short", "long" and "long long" in
		 * dcs->d_rank_mod
		 */
		if (dcs->d_rank_mod != NOTSPEC)
			dcs->d_invalid_type_combination = true;
		dcs->d_rank_mod = t;
	} else if (t == FLOAT || t == DOUBLE) {
		if (dcs->d_rank_mod == NOTSPEC || dcs->d_rank_mod == LONG) {
			if (dcs->d_complex_mod != NOTSPEC
			    || (t == FLOAT && dcs->d_rank_mod == LONG))
				dcs->d_invalid_type_combination = true;
			dcs->d_complex_mod = t;
		} else {
			if (dcs->d_abstract_type != NOTSPEC)
				dcs->d_invalid_type_combination = true;
			dcs->d_abstract_type = t;
		}
	} else if (t == PTR) {
		dcs->d_type = tp;
	} else {
		/*
		 * remember specifiers "void", "char", "int",
		 * or "_Complex" in dcs->d_abstract_type
		 */
		if (dcs->d_abstract_type != NOTSPEC)
			dcs->d_invalid_type_combination = true;
		dcs->d_abstract_type = t;
	}
}

/* Merge the signedness into the abstract type. */
static tspec_t
merge_signedness(tspec_t t, tspec_t s)
{

	if (s == SIGNED)
		return t == CHAR ? SCHAR : t;
	if (s != UNSIGN)
		return t;
	return t == CHAR ? UCHAR
	    : t == SHORT ? USHORT
	    : t == INT ? UINT
	    : t == LONG ? ULONG
	    : t == QUAD ? UQUAD
	    : t;
}

/*
 * called if a list of declaration specifiers contains a typedef name
 * and other specifiers (except struct, union, enum, typedef name)
 */
static type_t *
tdeferr(type_t *td, tspec_t t)
{
	tspec_t	t2;

	t2 = td->t_tspec;

	if ((t == SIGNED || t == UNSIGN) &&
	    (t2 == CHAR || t2 == SHORT || t2 == INT ||
	     t2 == LONG || t2 == QUAD)) {
		if (allow_c90)
			/* modifying typedef with '%s'; only qualifiers... */
			warning(5, tspec_name(t));
		td = block_dup_type(gettyp(merge_signedness(t2, t)));
		td->t_typedef = true;
		return td;
	}

	if (t == SHORT && (t2 == INT || t2 == UINT)) {
		/* modifying typedef with '%s'; only qualifiers allowed */
		warning(5, "short");
		td = block_dup_type(gettyp(t2 == INT ? SHORT : USHORT));
		td->t_typedef = true;
		return td;
	}

	if (t == LONG &&
	    (t2 == INT || t2 == UINT || t2 == LONG || t2 == ULONG ||
	     t2 == FLOAT || t2 == DOUBLE || t2 == DCOMPLEX)) {
		/* modifying typedef with '%s'; only qualifiers allowed */
		warning(5, "long");
		if (t2 == INT) {
			td = gettyp(LONG);
		} else if (t2 == UINT) {
			td = gettyp(ULONG);
		} else if (t2 == LONG) {
			td = gettyp(QUAD);
		} else if (t2 == ULONG) {
			td = gettyp(UQUAD);
		} else if (t2 == FLOAT) {
			td = gettyp(DOUBLE);
		} else if (t2 == DOUBLE) {
			td = gettyp(LDOUBLE);
		} else if (t2 == DCOMPLEX) {
			td = gettyp(LCOMPLEX);
		}
		td = block_dup_type(td);
		td->t_typedef = true;
		return td;
	}

	/* Anything else is not accepted. */
	dcs->d_invalid_type_combination = true;
	return td;
}

/*
 * Remember the symbol of a typedef name (2nd arg) in a struct, union
 * or enum tag if the typedef name is the first defined for this tag.
 *
 * If the tag is unnamed, the typedef name is used for identification
 * of this tag in lint2. Although it's possible that more than one typedef
 * name is defined for one tag, the first name defined should be unique
 * if the tag is unnamed.
 */
static void
settdsym(type_t *tp, sym_t *sym)
{
	tspec_t	t;

	if ((t = tp->t_tspec) == STRUCT || t == UNION) {
		if (tp->t_str->sou_first_typedef == NULL)
			tp->t_str->sou_first_typedef = sym;
	} else if (t == ENUM) {
		if (tp->t_enum->en_first_typedef == NULL)
			tp->t_enum->en_first_typedef = sym;
	}
}

static unsigned int
bitfieldsize(sym_t **mem)
{
	unsigned int len = (*mem)->s_type->t_flen;
	while (*mem != NULL && (*mem)->s_type->t_bitfield) {
		len += (*mem)->s_type->t_flen;
		*mem = (*mem)->s_next;
	}
	return len - len % INT_SIZE;
}

static void
setpackedsize(type_t *tp)
{
	struct_or_union *sp;
	sym_t *mem;

	switch (tp->t_tspec) {
	case STRUCT:
	case UNION:
		sp = tp->t_str;
		sp->sou_size_in_bits = 0;
		for (mem = sp->sou_first_member;
		     mem != NULL; mem = mem->s_next) {
			unsigned int x;

			if (mem->s_type->t_bitfield) {
				sp->sou_size_in_bits += bitfieldsize(&mem);
				if (mem == NULL)
					break;
			}
			x = type_size_in_bits(mem->s_type);
			if (tp->t_tspec == STRUCT)
				sp->sou_size_in_bits += x;
			else if (x > sp->sou_size_in_bits)
				sp->sou_size_in_bits = x;
		}
		break;
	default:
		/* %s attribute ignored for %s */
		warning(326, "packed", type_name(tp));
		break;
	}
}

void
addpacked(void)
{
	if (dcs->d_type == NULL)
		dcs->d_packed = true;
	else
		setpackedsize(dcs->d_type);
}

void
add_attr_used(void)
{
	dcs->d_used = true;
}

/*
 * Remember a qualifier which is part of the declaration specifiers
 * (and not the declarator) in the top element of the declaration stack.
 * Also detect multiple qualifiers of the same kind.

 * The remembered qualifier is used by end_type() to construct the type
 * for all declarators.
 */
void
add_qualifier(tqual_t q)
{

	if (q == CONST) {
		if (dcs->d_const) {
			/* duplicate '%s' */
			warning(10, "const");
		}
		dcs->d_const = true;
	} else if (q == VOLATILE) {
		if (dcs->d_volatile) {
			/* duplicate '%s' */
			warning(10, "volatile");
		}
		dcs->d_volatile = true;
	} else {
		lint_assert(q == RESTRICT || q == THREAD);
		/* Silently ignore these qualifiers. */
	}
}

/*
 * Go to the next declaration level (structs, nested structs, blocks,
 * argument declaration lists ...)
 */
void
begin_declaration_level(declaration_kind dk)
{
	dinfo_t	*di;

	/* put a new element on the declaration stack */
	di = xcalloc(1, sizeof(*di));
	di->d_enclosing = dcs;
	dcs = di;
	di->d_kind = dk;
	di->d_ldlsym = &di->d_dlsyms;
	debug_step("%s(%s)", __func__, declaration_kind_name(dk));
}

/*
 * Go back to previous declaration level
 */
void
end_declaration_level(void)
{
	dinfo_t	*di;

	debug_step("%s(%s)", __func__, declaration_kind_name(dcs->d_kind));

	lint_assert(dcs->d_enclosing != NULL);
	di = dcs;
	dcs = di->d_enclosing;
	switch (di->d_kind) {
	case DK_MOS:
	case DK_MOU:
	case DK_ENUM_CONST:
		/*
		 * Symbols declared in (nested) structs or enums are
		 * part of the next level (they are removed from the
		 * symbol table if the symbols of the outer level are
		 * removed).
		 */
		if ((*dcs->d_ldlsym = di->d_dlsyms) != NULL)
			dcs->d_ldlsym = di->d_ldlsym;
		break;
	case DK_OLD_STYLE_ARG:
		/*
		 * All symbols in dcs->d_dlsyms are introduced in old style
		 * argument declarations (it's not clean, but possible).
		 * They are appended to the list of symbols declared in
		 * an old style argument identifier list or a new style
		 * parameter type list.
		 */
		if (di->d_dlsyms != NULL) {
			*di->d_ldlsym = dcs->d_func_proto_syms;
			dcs->d_func_proto_syms = di->d_dlsyms;
		}
		break;
	case DK_ABSTRACT:
		/*
		 * casts and sizeof
		 * Append all symbols declared in the abstract declaration
		 * to the list of symbols declared in the surrounding
		 * declaration or block.
		 * XXX I'm not sure whether they should be removed from the
		 * symbol table now or later.
		 */
		if ((*dcs->d_ldlsym = di->d_dlsyms) != NULL)
			dcs->d_ldlsym = di->d_ldlsym;
		break;
	case DK_AUTO:
		/* check usage of local vars */
		check_usage(di);
		/* FALLTHROUGH */
	case DK_PROTO_ARG:
		/* usage of arguments will be checked by funcend() */
		rmsyms(di->d_dlsyms);
		break;
	case DK_EXTERN:
		/* there is nothing after external declarations */
		/* FALLTHROUGH */
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	free(di);
}

/*
 * Set flag d_asm in all declaration stack elements up to the
 * outermost one.
 *
 * This is used to mark compound statements which have, possibly in
 * nested compound statements, asm statements. For these compound
 * statements no warnings about unused or uninitialized variables are
 * printed.
 *
 * There is no need to clear d_asm in dinfo structs with context AUTO,
 * because these structs are freed at the end of the compound statement.
 * But it must be cleared in the outermost dinfo struct, which has
 * context EXTERN. This could be done in begin_type() and would work for C90,
 * but not for C99 or C++ (due to mixed statements and declarations). Thus
 * we clear it in global_clean_up_decl(), which is used to do some cleanup
 * after global declarations/definitions.
 */
void
setasm(void)
{
	dinfo_t	*di;

	for (di = dcs; di != NULL; di = di->d_enclosing)
		di->d_asm = true;
}

/*
 * Clean all elements of the top element of declaration stack which
 * will be used by the next declaration
 */
void
begin_type(void)
{

	dcs->d_abstract_type = NOTSPEC;
	dcs->d_complex_mod = NOTSPEC;
	dcs->d_sign_mod = NOTSPEC;
	dcs->d_rank_mod = NOTSPEC;
	dcs->d_scl = NOSCL;
	dcs->d_type = NULL;
	dcs->d_const = false;
	dcs->d_volatile = false;
	dcs->d_inline = false;
	dcs->d_multiple_storage_classes = false;
	dcs->d_invalid_type_combination = false;
	dcs->d_nonempty_decl = false;
	dcs->d_notyp = false;
}

static void
dcs_adjust_storage_class(void)
{
	if (dcs->d_kind == DK_EXTERN) {
		if (dcs->d_scl == REG || dcs->d_scl == AUTO) {
			/* illegal storage class */
			error(8);
			dcs->d_scl = NOSCL;
		}
	} else if (dcs->d_kind == DK_OLD_STYLE_ARG ||
		   dcs->d_kind == DK_PROTO_ARG) {
		if (dcs->d_scl != NOSCL && dcs->d_scl != REG) {
			/* only register valid as formal parameter storage... */
			error(9);
			dcs->d_scl = NOSCL;
		}
	}
}

/*
 * Merge the declaration specifiers from dcs into dcs->d_type.
 *
 * See C99 6.7.2 "Type specifiers".
 */
static void
dcs_merge_declaration_specifiers(void)
{
	tspec_t	t, s, l, c;
	type_t	*tp;

	t = dcs->d_abstract_type; /* VOID, BOOL, CHAR, INT or COMPLEX */
	c = dcs->d_complex_mod;	/* FLOAT or DOUBLE */
	s = dcs->d_sign_mod;	/* SIGNED or UNSIGN */
	l = dcs->d_rank_mod;	/* SHORT, LONG or QUAD */
	tp = dcs->d_type;

	debug_step("%s: %s", __func__, type_name(tp));
	if (t == NOTSPEC && s == NOTSPEC && l == NOTSPEC && c == NOTSPEC &&
	    tp == NULL)
		dcs->d_notyp = true;
	if (t == NOTSPEC && s == NOTSPEC && (l == NOTSPEC || l == LONG) &&
	    tp == NULL)
		t = c;

	if (tp != NULL) {
		lint_assert(t == NOTSPEC);
		lint_assert(s == NOTSPEC);
		lint_assert(l == NOTSPEC);
		return;
	}

	if (t == NOTSPEC)
		t = INT;
	if (s == NOTSPEC && t == INT)
		s = SIGNED;
	if (l != NOTSPEC && t == CHAR) {
		dcs->d_invalid_type_combination = true;
		l = NOTSPEC;
	}
	if (l == LONG && t == FLOAT) {
		l = NOTSPEC;
		t = DOUBLE;
		if (allow_c90)
			/* use 'double' instead of 'long float' */
			warning(6);
	}
	if ((l == LONG && t == DOUBLE) || t == LDOUBLE) {
		l = NOTSPEC;
		t = LDOUBLE;
	}
	if (t == LDOUBLE && !allow_c90) {
		/* 'long double' is illegal in traditional C */
		warning(266);
	}
	if (l == LONG && t == DCOMPLEX) {
		l = NOTSPEC;
		t = LCOMPLEX;
	}

	if (t != INT && t != CHAR && (s != NOTSPEC || l != NOTSPEC)) {
		dcs->d_invalid_type_combination = true;
		l = s = NOTSPEC;
	}
	if (l != NOTSPEC)
		t = l;
	dcs->d_type = gettyp(merge_signedness(t, s));
}

/*
 * Create a type structure from the information gathered in
 * the declaration stack.
 * Complain about storage classes which are not possible in current
 * context.
 */
void
end_type(void)
{

	dcs_merge_declaration_specifiers();

	if (dcs->d_multiple_storage_classes) {
		/* only one storage class allowed */
		error(7);
	}
	if (dcs->d_invalid_type_combination) {
		/* illegal type combination */
		error(4);
	}

	dcs_adjust_storage_class();

	if (dcs->d_const && dcs->d_type->t_const && !dcs->d_type->t_typeof) {
		lint_assert(dcs->d_type->t_typedef);
		/* typedef already qualified with '%s' */
		warning(68, "const");
	}
	if (dcs->d_volatile && dcs->d_type->t_volatile &&
	    !dcs->d_type->t_typeof) {
		lint_assert(dcs->d_type->t_typedef);
		/* typedef already qualified with '%s' */
		warning(68, "volatile");
	}

	if (dcs->d_const || dcs->d_volatile) {
		dcs->d_type = block_dup_type(dcs->d_type);
		dcs->d_type->t_const |= dcs->d_const;
		dcs->d_type->t_volatile |= dcs->d_volatile;
	}
}

/*
 * Return the length of a type in bits.
 *
 * Printing a message if the outermost dimension of an array is 0 must
 * be done by the caller. All other problems are reported by this function
 * if name is not NULL.
 */
int
length_in_bits(const type_t *tp, const char *name)
{
	unsigned int elem, elsz;

	elem = 1;
	while (tp != NULL && tp->t_tspec == ARRAY) {
		elem *= tp->t_dim;
		tp = tp->t_subt;
	}
	if (tp == NULL)
		return -1;

	switch (tp->t_tspec) {
	case FUNC:
		/* compiler takes size of function */
		INTERNAL_ERROR("%s", msgs[12]);
		/* NOTREACHED */
	case STRUCT:
	case UNION:
		if (is_incomplete(tp) && name != NULL) {
			/* '%s' has incomplete type '%s' */
			error(31, name, type_name(tp));
		}
		elsz = tp->t_str->sou_size_in_bits;
		break;
	case ENUM:
		if (is_incomplete(tp) && name != NULL) {
			/* incomplete enum type: %s */
			warning(13, name);
		}
		/* FALLTHROUGH */
	default:
		elsz = size_in_bits(tp->t_tspec);
		/*
		 * Workaround until the type parser (see add_function,
		 * add_array, add_pointer) does not construct the invalid
		 * intermediate declaration 'void b[4]' for the legitimate
		 * declaration 'void *b[4]'.
		 */
		if (sytxerr > 0 && elsz == 0)
			elsz = CHAR_SIZE;
		lint_assert(elsz > 0);
		break;
	}
	return (int)(elem * elsz);
}

unsigned int
alignment_in_bits(const type_t *tp)
{
	unsigned int a;
	tspec_t t;

	/* Super conservative so that it works for most systems. */
	unsigned int worst_align_in_bits = 2 * LONG_SIZE;

	while (tp->t_tspec == ARRAY)
		tp = tp->t_subt;

	if (is_struct_or_union(t = tp->t_tspec)) {
		a = tp->t_str->sou_align_in_bits;
	} else {
		lint_assert(t != FUNC);
		if ((a = size_in_bits(t)) == 0) {
			a = CHAR_SIZE;
		} else if (a > worst_align_in_bits) {
			a = worst_align_in_bits;
		}
	}
	lint_assert(a >= CHAR_SIZE);
	lint_assert(a <= worst_align_in_bits);
	return a;
}

/*
 * Concatenate two lists of symbols by s_next. Used by declarations of
 * struct/union/enum elements and parameters.
 */
sym_t *
lnklst(sym_t *l1, sym_t *l2)
{
	sym_t	*l;

	if ((l = l1) == NULL)
		return l2;
	while (l1->s_next != NULL)
		l1 = l1->s_next;
	l1->s_next = l2;
	return l;
}

/*
 * Check if the type of the given symbol is valid and print an error
 * message if it is not.
 *
 * Invalid types are:
 * - arrays of incomplete types or functions
 * - functions returning arrays or functions
 * - void types other than type of function or pointer
 */
void
check_type(sym_t *sym)
{
	tspec_t	to, t;
	type_t	**tpp, *tp;

	tpp = &sym->s_type;
	to = NOTSPEC;
	while ((tp = *tpp) != NULL) {
		t = tp->t_tspec;
		/*
		 * If this is the type of an old style function definition,
		 * a better warning is printed in funcdef().
		 */
		if (t == FUNC && !tp->t_proto &&
		    !(to == NOTSPEC && sym->s_osdef)) {
			/* TODO: Make this an error in C99 mode as well. */
			if ((!allow_trad && !allow_c99) && hflag)
				/* function declaration is not a prototype */
				warning(287);
		}
		if (to == FUNC) {
			if (t == FUNC || t == ARRAY) {
				/* function returns illegal type '%s' */
				error(15, type_name(tp));
				if (t == FUNC) {
					*tpp = block_derive_type(*tpp, PTR);
				} else {
					*tpp = block_derive_type(
					    (*tpp)->t_subt, PTR);
				}
				return;
			} else if (tp->t_const || tp->t_volatile) {
				/* TODO: Make this a warning in C99 mode as well. */
				if (!allow_trad && !allow_c99) {	/* XXX or better allow_c90? */
					/* function cannot return const... */
					warning(228);
				}
			}
		} else if (to == ARRAY) {
			if (t == FUNC) {
				/* array of function is illegal */
				error(16);
				*tpp = gettyp(INT);
				return;
			} else if (t == ARRAY && tp->t_dim == 0) {
				/* null dimension */
				error(17);
				return;
			} else if (t == VOID) {
				/* illegal use of 'void' */
				error(18);
				*tpp = gettyp(INT);
#if 0	/* errors are produced by length_in_bits */
			} else if (is_incomplete(tp)) {
				/* array of incomplete type */
				/* TODO: Make this an error in C99 mode as well. */
				if (!allow_trad && !allow_c99) {
					/* array of incomplete type */
					error(301);
				} else {
					/* array of incomplete type */
					warning(301);
				}
#endif
			}
		} else if (to == NOTSPEC && t == VOID) {
			if (dcs->d_kind == DK_PROTO_ARG) {
				if (sym->s_scl != ABSTRACT) {
					lint_assert(sym->s_name != unnamed);
					/* void parameter cannot have ... */
					error(61, sym->s_name);
					*tpp = gettyp(INT);
				}
			} else if (dcs->d_kind == DK_ABSTRACT) {
				/* ok */
			} else if (sym->s_scl != TYPEDEF) {
				/* void type for '%s' */
				error(19, sym->s_name);
				*tpp = gettyp(INT);
			}
		}
		if (t == VOID && to != PTR) {
			if (tp->t_const || tp->t_volatile) {
				/* inappropriate qualifiers with 'void' */
				warning(69);
				tp->t_const = tp->t_volatile = false;
			}
		}
		tpp = &tp->t_subt;
		to = t;
	}
}

/*
 * In traditional C, the only portable type for bit-fields is unsigned int.
 *
 * In C90, the only allowed types for bit-fields are int, signed int and
 * unsigned int (3.5.2.1).  There is no mention of implementation-defined
 * types.
 *
 * In C99, the only portable types for bit-fields are _Bool, signed int and
 * unsigned int (6.7.2.1p4).  In addition, C99 allows "or some other
 * implementation-defined type".
 */
static void
check_bit_field_type(sym_t *dsym, type_t **const inout_tp, tspec_t *inout_t)
{
	type_t *tp = *inout_tp;
	tspec_t t = *inout_t;

	if (t == CHAR || t == UCHAR || t == SCHAR ||
	    t == SHORT || t == USHORT || t == ENUM) {
		if (!bitfieldtype_ok) {
			/* TODO: Make this an error in C99 mode as well. */
			if (!allow_trad && !allow_c99) {
				/* bit-field type '%s' invalid in ANSI C */
				warning(273, type_name(tp));
			} else if (pflag) {
				/* nonportable bit-field type '%s' */
				warning(34, type_name(tp));
			}
		}
	} else if (t == INT && dcs->d_sign_mod == NOTSPEC) {
		if (pflag && !bitfieldtype_ok) {
			/* bit-field of type plain 'int' has ... */
			warning(344);
		}
	} else if (!(t == INT || t == UINT || t == BOOL ||
		     (is_integer(t) && (bitfieldtype_ok || allow_gcc)))) {

		/* illegal bit-field type '%s' */
		warning(35, type_name(tp));

		unsigned int sz = tp->t_flen;
		dsym->s_type = tp = block_dup_type(gettyp(t = INT));
		if ((tp->t_flen = sz) > size_in_bits(t))
			tp->t_flen = size_in_bits(t);
		*inout_t = t;
		*inout_tp = tp;
	}
}

static void
declare_bit_field(sym_t *dsym, tspec_t *inout_t, type_t **const inout_tp)
{
	type_t *tp;
	tspec_t t;

	check_bit_field_type(dsym, inout_tp, inout_t);

	tp = *inout_tp;
	t = *inout_t;
	if (tp->t_flen > size_in_bits(t)) {
		/* illegal bit-field size: %d */
		error(36, tp->t_flen);
		tp->t_flen = size_in_bits(t);
	} else if (tp->t_flen == 0 && dsym->s_name != unnamed) {
		/* zero size bit-field */
		error(37);
		tp->t_flen = size_in_bits(t);
	}
	if (dsym->s_scl == MOU) {
		/* bit-field in union is very unusual */
		warning(41);
		dsym->s_type->t_bitfield = false;
		dsym->s_bitfield = false;
	}
}

/*
 * Process the declarator of a struct/union element.
 */
sym_t *
declarator_1_struct_union(sym_t *dsym)
{
	type_t	*tp;
	tspec_t	t;
	int	sz;
	unsigned int o = 0;	/* Appease GCC */

	lint_assert(is_member(dsym));

	if (dcs->d_redeclared_symbol != NULL) {
		lint_assert(is_member(dcs->d_redeclared_symbol));

		if (dsym->u.s_member.sm_sou_type ==
		    dcs->d_redeclared_symbol->u.s_member.sm_sou_type) {
			/* duplicate member name: %s */
			error(33, dsym->s_name);
			rmsym(dcs->d_redeclared_symbol);
		}
	}

	check_type(dsym);

	t = (tp = dsym->s_type)->t_tspec;

	if (dsym->s_bitfield) {
		declare_bit_field(dsym, &t, &tp);
	} else if (t == FUNC) {
		/* function illegal in structure or union */
		error(38);
		dsym->s_type = tp = block_derive_type(tp, t = PTR);
	}

	/*
	 * bit-fields of length 0 are not warned about because length_in_bits
	 * does not return the length of the bit-field but the length
	 * of the type the bit-field is packed in (it's ok)
	 */
	if ((sz = length_in_bits(dsym->s_type, dsym->s_name)) == 0) {
		if (t == ARRAY && dsym->s_type->t_dim == 0) {
			/* zero sized array in struct is a C99 extension: %s */
			c99ism(39, dsym->s_name);
		}
	}

	if (dcs->d_kind == DK_MOU) {
		o = dcs->d_offset_in_bits;
		dcs->d_offset_in_bits = 0;
	}
	if (dsym->s_bitfield) {
		align(alignment_in_bits(tp), tp->t_flen);
		dsym->u.s_member.sm_offset_in_bits = dcs->d_offset_in_bits -
		    dcs->d_offset_in_bits % size_in_bits(t);
		tp->t_foffs = dcs->d_offset_in_bits -
		    dsym->u.s_member.sm_offset_in_bits;
		dcs->d_offset_in_bits += tp->t_flen;
	} else {
		align(alignment_in_bits(tp), 0);
		dsym->u.s_member.sm_offset_in_bits = dcs->d_offset_in_bits;
		dcs->d_offset_in_bits += sz;
	}
	if (dcs->d_kind == DK_MOU) {
		if (o > dcs->d_offset_in_bits)
			dcs->d_offset_in_bits = o;
	}

	check_function_definition(dsym, false);

	/*
	 * Clear the BITFIELDTYPE indicator after processing each
	 * structure element.
	 */
	bitfieldtype_ok = false;

	return dsym;
}

/*
 * Aligns next structure element as required.
 *
 * al contains the required alignment, len the length of a bit-field.
 */
static void
align(unsigned int al, unsigned int len)
{
	unsigned int no;

	/*
	 * The alignment of the current element becomes the alignment of
	 * the struct/union if it is larger than the current alignment
	 * of the struct/union.
	 */
	if (al > dcs->d_sou_align_in_bits)
		dcs->d_sou_align_in_bits = al;

	no = (dcs->d_offset_in_bits + (al - 1)) & ~(al - 1);
	if (len == 0 || dcs->d_offset_in_bits + len > no)
		dcs->d_offset_in_bits = no;
}

/*
 * Remember the width of the field in its type structure.
 */
sym_t *
bitfield(sym_t *dsym, int len)
{

	if (dsym == NULL) {
		dsym = block_zero_alloc(sizeof(*dsym));
		dsym->s_name = unnamed;
		dsym->s_kind = FMEMBER;
		dsym->s_scl = MOS;
		dsym->s_type = gettyp(UINT);
		dsym->s_block_level = -1;
	}
	dsym->s_type = block_dup_type(dsym->s_type);
	dsym->s_type->t_bitfield = true;
	dsym->s_type->t_flen = len;
	dsym->s_bitfield = true;
	return dsym;
}

/*
 * A sequence of asterisks and qualifiers, from right to left.  For example,
 * 'const ***volatile **const volatile' results in [cvp, p, vp, p, p].  The
 * leftmost 'const' is not included in this list, it is stored in dcs->d_const
 * instead.
 */
qual_ptr *
merge_qualified_pointer(qual_ptr *p1, qual_ptr *p2)
{
	qual_ptr *tail;

	if (p2 == NULL)
		return p1;	/* for optional qualifiers */

	if (p2->p_pointer) {
		/* append p1 to p2, keeping p2 */
		for (tail = p2; tail->p_next != NULL; tail = tail->p_next)
			continue;
		tail->p_next = p1;
		return p2;
	}

	/* merge p2 into p1, keeping p1 */
	if (p2->p_const) {
		if (p1->p_const) {
			/* duplicate '%s' */
			warning(10, "const");
		}
		p1->p_const = true;
	}
	if (p2->p_volatile) {
		if (p1->p_volatile) {
			/* duplicate '%s' */
			warning(10, "volatile");
		}
		p1->p_volatile = true;
	}
	free(p2);
	return p1;
}

static type_t *
block_derive_pointer(type_t *stp, bool is_const, bool is_volatile)
{
	type_t *tp;

	tp = block_derive_type(stp, PTR);
	tp->t_const = is_const;
	tp->t_volatile = is_volatile;
	return tp;
}

/*
 * The following 3 functions extend the type of a declarator with
 * pointer, function and array types.
 *
 * The current type is the type built by end_type() (dcs->d_type) and
 * pointer, function and array types already added for this
 * declarator. The new type extension is inserted between both.
 */
sym_t *
add_pointer(sym_t *decl, qual_ptr *p)
{
	type_t **tpp;
	qual_ptr *next;

	debug_dinfo(dcs);

	tpp = &decl->s_type;
	while (*tpp != NULL && *tpp != dcs->d_type)
		tpp = &(*tpp)->t_subt;
	if (*tpp == NULL) {
		debug_step("add_pointer: unchanged '%s'",
		    type_name(decl->s_type));
		return decl;
	}

	while (p != NULL) {
		*tpp = block_derive_pointer(dcs->d_type,
		    p->p_const, p->p_volatile);

		tpp = &(*tpp)->t_subt;

		next = p->p_next;
		free(p);
		p = next;
	}
	debug_step("add_pointer: '%s'", type_name(decl->s_type));
	return decl;
}

static type_t *
block_derive_array(type_t *stp, bool dim, int len)
{
	type_t *tp;

	tp = block_derive_type(stp, ARRAY);
	tp->t_dim = len;

#if 0
	/*
	 * As of 2022-04-03, the implementation of the type parser (see
	 * add_function, add_array, add_pointer) is strange.  When it sees
	 * the type 'void *b[4]', it first creates 'void b[4]' and only later
	 * inserts the '*' in the middle of the type.  Once created, a type
	 * should not be modified anymore.
	 *
	 * Since the intermediate type would be an array of void, but the
	 * final type is valid, this check cannot be enabled yet.
	 */
	if (stp->t_tspec == VOID) {
		/* array of incomplete type */
		error(301);
		tp->t_subt = gettyp(CHAR);
	}
#endif
	if (len < 0) {
		/* negative array dimension (%d) */
		error(20, len);
	} else if (len == 0 && dim) {
		/* zero sized array is a C99 extension */
		c99ism(322);
	} else if (len == 0 && !dim)
		tp->t_incomplete_array = true;

	return tp;
}

/*
 * If a dimension was specified, dim is true, otherwise false
 * n is the specified dimension
 */
sym_t *
add_array(sym_t *decl, bool dim, int n)
{
	type_t	**tpp;

	debug_dinfo(dcs);

	tpp = &decl->s_type;
	while (*tpp != NULL && *tpp != dcs->d_type)
		tpp = &(*tpp)->t_subt;
	if (*tpp == NULL) {
		debug_step("add_array: unchanged '%s'",
		    type_name(decl->s_type));
		return decl;
	}

	*tpp = block_derive_array(dcs->d_type, dim, n);

	debug_step("add_array: '%s'", type_name(decl->s_type));
	return decl;
}

static type_t *
block_derive_function(type_t *ret, bool proto, sym_t *args, bool vararg)
{
	type_t *tp;

	tp = block_derive_type(ret, FUNC);
	tp->t_proto = proto;
	if (proto)
		tp->t_args = args;
	tp->t_vararg = vararg;
	return tp;
}

sym_t *
add_function(sym_t *decl, sym_t *args)
{
	type_t	**tpp;

	debug_enter();
	debug_dinfo(dcs);
	debug_sym("decl: ", decl, "\n");
#ifdef DEBUG
	for (const sym_t *arg = args; arg != NULL; arg = arg->s_next)
		debug_sym("arg: ", arg, "\n");
#endif

	if (dcs->d_proto) {
		if (!allow_c90)
			/* function prototypes are illegal in traditional C */
			warning(270);
		args = new_style_function(args);
	} else {
		old_style_function(decl, args);
	}

	/*
	 * The symbols are removed from the symbol table by
	 * end_declaration_level after add_function. To be able to restore
	 * them if this is a function definition, a pointer to the list of
	 * all symbols is stored in dcs->d_enclosing->d_func_proto_syms. Also
	 * a list of the arguments (concatenated by s_next) is stored in
	 * dcs->d_enclosing->d_func_args. (dcs->d_enclosing must be used
	 * because *dcs is the declaration stack element created for the list
	 * of params and is removed after add_function.)
	 */
	if (dcs->d_enclosing->d_kind == DK_EXTERN &&
	    decl->s_type == dcs->d_enclosing->d_type) {
		dcs->d_enclosing->d_func_proto_syms = dcs->d_dlsyms;
		dcs->d_enclosing->d_func_args = args;
	}

	/*
	 * XXX: What is this code doing on a semantic level, and why?
	 * Returning decl leads to the wrong function types in msg_347.
	 */
	tpp = &decl->s_type;
	if (*tpp == NULL)
		decl->s_type = dcs->d_enclosing->d_type;
	while (*tpp != NULL && *tpp != dcs->d_enclosing->d_type)
		/*
		 * XXX: accessing INT->t_subt feels strange, even though it
		 * may even be guaranteed to be NULL.
		 */
		tpp = &(*tpp)->t_subt;
	if (*tpp == NULL) {
		debug_step("add_function: unchanged '%s'",
		    type_name(decl->s_type));
		debug_leave();
		return decl;	/* see msg_347 */
	}

	*tpp = block_derive_function(dcs->d_enclosing->d_type,
	    dcs->d_proto, args, dcs->d_vararg);

	debug_step("add_function: '%s'", type_name(decl->s_type));
	debug_leave();
	return decl;
}

static sym_t *
new_style_function(sym_t *args)
{
	sym_t	*arg, *sym;
	scl_t	sc;

	/*
	 * Declarations of structs/unions/enums in param lists are legal,
	 * but senseless.
	 */
	for (sym = dcs->d_dlsyms; sym != NULL; sym = sym->s_level_next) {
		sc = sym->s_scl;
		if (sc == STRUCT_TAG || sc == UNION_TAG || sc == ENUM_TAG) {
			/* dubious tag declaration: %s %s */
			warning(85, storage_class_name(sc), sym->s_name);
		}
	}

	for (arg = args; arg != NULL; arg = arg->s_next) {
		if (arg->s_type->t_tspec == VOID &&
		    !(arg == args && arg->s_next == NULL)) {
			/* void must be sole parameter */
			error(60);
			arg->s_type = gettyp(INT);
		}
	}

	if (args == NULL || args->s_type->t_tspec == VOID)
		return NULL;
	return args;
}

/*
 * Called for old style function declarations.
 */
static void
old_style_function(sym_t *decl, sym_t *args)
{

	/*
	 * Remember list of parameters only if this really seems to be a
	 * function definition.
	 */
	if (dcs->d_enclosing->d_kind == DK_EXTERN &&
	    decl->s_type == dcs->d_enclosing->d_type) {
		/*
		 * We assume that this becomes a function definition. If
		 * we are wrong, it's corrected in check_function_definition.
		 */
		if (args != NULL) {
			decl->s_osdef = true;
			decl->u.s_old_style_args = args;
		}
	} else {
		if (args != NULL)
			/* function prototype parameters must have types */
			warning(62);
	}
}

/*
 * Lists of identifiers in functions declarations are allowed only if
 * it's also a function definition. If this is not the case, print an
 * error message.
 */
void
check_function_definition(sym_t *sym, bool msg)
{

	if (sym->s_osdef) {
		if (msg) {
			/* incomplete or misplaced function definition */
			error(22);
		}
		sym->s_osdef = false;
		sym->u.s_old_style_args = NULL;
	}
}

/*
 * Process the name in a declarator.
 * The symbol gets one of the storage classes EXTERN, STATIC, AUTO or
 * TYPEDEF.
 * s_def and s_register are valid after declarator_name().
 */
sym_t *
declarator_name(sym_t *sym)
{
	scl_t	sc = NOSCL;

	if (sym->s_scl == NOSCL) {
		dcs->d_redeclared_symbol = NULL;
	} else if (sym->s_defarg) {
		sym->s_defarg = false;
		dcs->d_redeclared_symbol = NULL;
	} else {
		dcs->d_redeclared_symbol = sym;
		sym = pushdown(sym);
	}

	switch (dcs->d_kind) {
	case DK_MOS:
	case DK_MOU:
		/* Set parent */
		sym->u.s_member.sm_sou_type = dcs->d_tagtyp->t_str;
		sym->s_def = DEF;
		/* XXX: Where is sym->u.s_member.sm_offset_in_bits set? */
		sc = dcs->d_kind == DK_MOS ? MOS : MOU;
		break;
	case DK_EXTERN:
		/*
		 * static and external symbols without "extern" are
		 * considered to be tentatively defined, external
		 * symbols with "extern" are declared, and typedef names
		 * are defined. Tentative defined and declared symbols
		 * may become defined if an initializer is present or
		 * this is a function definition.
		 */
		if ((sc = dcs->d_scl) == NOSCL) {
			sc = EXTERN;
			sym->s_def = TDEF;
		} else if (sc == STATIC) {
			sym->s_def = TDEF;
		} else if (sc == TYPEDEF) {
			sym->s_def = DEF;
		} else {
			lint_assert(sc == EXTERN);
			sym->s_def = DECL;
		}
		break;
	case DK_PROTO_ARG:
		sym->s_arg = true;
		/* FALLTHROUGH */
	case DK_OLD_STYLE_ARG:
		if ((sc = dcs->d_scl) == NOSCL) {
			sc = AUTO;
		} else {
			lint_assert(sc == REG);
			sym->s_register = true;
			sc = AUTO;
		}
		sym->s_def = DEF;
		break;
	case DK_AUTO:
		if ((sc = dcs->d_scl) == NOSCL) {
			/*
			 * XXX somewhat ugly because we dont know whether
			 * this is AUTO or EXTERN (functions). If we are
			 * wrong it must be corrected in declare_local(),
			 * where we have the necessary type information.
			 */
			sc = AUTO;
			sym->s_def = DEF;
		} else if (sc == AUTO || sc == STATIC || sc == TYPEDEF) {
			sym->s_def = DEF;
		} else if (sc == REG) {
			sym->s_register = true;
			sc = AUTO;
			sym->s_def = DEF;
		} else {
			lint_assert(sc == EXTERN);
			sym->s_def = DECL;
		}
		break;
	case DK_ABSTRACT:	/* try to continue after syntax errors */
		sc = NOSCL;
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	sym->s_scl = sc;

	sym->s_type = dcs->d_type;

	dcs->d_func_proto_syms = NULL;

	return sym;
}

/*
 * Process a name in the list of formal parameters in an old style function
 * definition.
 */
sym_t *
old_style_function_name(sym_t *sym)
{

	if (sym->s_scl != NOSCL) {
		if (block_level == sym->s_block_level) {
			/* redeclaration of formal parameter %s */
			error(21, sym->s_name);
			lint_assert(sym->s_defarg);
		}
		sym = pushdown(sym);
	}
	sym->s_type = gettyp(INT);
	sym->s_scl = AUTO;
	sym->s_def = DEF;
	sym->s_defarg = sym->s_arg = true;
	return sym;
}

/*
 * Create the type of a tag.
 *
 * tag points to the symbol table entry of the tag
 * kind is the kind of the tag (STRUCT/UNION/ENUM)
 * decl is true if the type of the tag will be completed in this declaration
 * (the following token is T_LBRACE)
 * semi is true if the following token is T_SEMI
 */
type_t *
mktag(sym_t *tag, tspec_t kind, bool decl, bool semi)
{
	scl_t	scl;
	type_t	*tp;

	if (kind == STRUCT) {
		scl = STRUCT_TAG;
	} else if (kind == UNION) {
		scl = UNION_TAG;
	} else {
		lint_assert(kind == ENUM);
		scl = ENUM_TAG;
	}

	if (tag != NULL) {
		if (tag->s_scl != NOSCL) {
			tag = newtag(tag, scl, decl, semi);
		} else {
			/* a new tag, no empty declaration */
			dcs->d_enclosing->d_nonempty_decl = true;
			if (scl == ENUM_TAG && !decl) {
				/* TODO: Make this an error in C99 mode as well. */
				if (allow_c90 && ((!allow_trad && !allow_c99) || pflag))
					/* forward reference to enum type */
					warning(42);
			}
		}
		if (tag->s_scl == NOSCL) {
			tag->s_scl = scl;
			tag->s_type = tp = block_zero_alloc(sizeof(*tp));
			tp->t_packed = dcs->d_packed;
		} else {
			tp = tag->s_type;
		}
	} else {
		tag = block_zero_alloc(sizeof(*tag));
		tag->s_name = unnamed;
		UNIQUE_CURR_POS(tag->s_def_pos);
		tag->s_kind = FTAG;
		tag->s_scl = scl;
		tag->s_block_level = -1;
		tag->s_type = tp = block_zero_alloc(sizeof(*tp));
		tp->t_packed = dcs->d_packed;
		dcs->d_enclosing->d_nonempty_decl = true;
	}

	if (tp->t_tspec == NOTSPEC) {
		tp->t_tspec = kind;
		if (kind != ENUM) {
			tp->t_str = block_zero_alloc(sizeof(*tp->t_str));
			tp->t_str->sou_align_in_bits = CHAR_SIZE;
			tp->t_str->sou_tag = tag;
			tp->t_str->sou_incomplete = true;
		} else {
			tp->t_is_enum = true;
			tp->t_enum = block_zero_alloc(sizeof(*tp->t_enum));
			tp->t_enum->en_tag = tag;
			tp->t_enum->en_incomplete = true;
		}
	}
	return tp;
}

/*
 * Checks all possible cases of tag redeclarations.
 * decl is true if T_LBRACE follows
 * semi is true if T_SEMI follows
 */
static sym_t *
newtag(sym_t *tag, scl_t scl, bool decl, bool semi)
{

	if (tag->s_block_level < block_level) {
		if (semi) {
			/* "struct a;" */
			if (allow_c90) {
				/* XXX: Why is this warning suppressed in C90 mode? */
				if (allow_trad || allow_c99)
					/* declaration introduces new ... */
					warning(44, storage_class_name(scl),
					    tag->s_name);
				tag = pushdown(tag);
			} else if (tag->s_scl != scl) {
				/* base type is really '%s %s' */
				warning(45, storage_class_name(tag->s_scl),
				    tag->s_name);
			}
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (decl) {
			/* "struct a { ... } " */
			if (hflag)
				/* redefinition hides earlier one: %s */
				warning(43, tag->s_name);
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (tag->s_scl != scl) {
			/* base type is really '%s %s' */
			warning(45, storage_class_name(tag->s_scl),
			    tag->s_name);
			/* XXX: Why is this warning suppressed in C90 mode? */
			if (allow_trad || allow_c99) {
				/* declaration introduces new type in ... */
				warning(44, storage_class_name(scl),
				    tag->s_name);
			}
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		}
	} else {
		if (tag->s_scl != scl ||
		    (decl && !is_incomplete(tag->s_type))) {
			/* %s tag '%s' redeclared as %s */
			error(46, storage_class_name(tag->s_scl),
			    tag->s_name, storage_class_name(scl));
			print_previous_declaration(-1, tag);
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (semi || decl) {
			dcs->d_enclosing->d_nonempty_decl = true;
		}
	}
	return tag;
}

const char *
storage_class_name(scl_t sc)
{
	switch (sc) {
	case EXTERN:	return "extern";
	case STATIC:	return "static";
	case AUTO:	return "auto";
	case REG:	return "register";
	case TYPEDEF:	return "typedef";
	case STRUCT_TAG:return "struct";
	case UNION_TAG:	return "union";
	case ENUM_TAG:	return "enum";
	default:	lint_assert(/*CONSTCOND*/false);
	}
	/* NOTREACHED */
}

/*
 * tp points to the type of the tag, fmem to the list of members.
 */
type_t *
complete_tag_struct_or_union(type_t *tp, sym_t *fmem)
{
	tspec_t	t;
	struct_or_union	*sp;
	int	n;
	sym_t	*mem;

	if (tp == NULL)		/* in case of syntax errors */
		return gettyp(INT);

	if (tp->t_tspec == ENUM)
		tp->t_enum->en_incomplete = false;
	else
		tp->t_str->sou_incomplete = false;

	t = tp->t_tspec;
	align((u_int)dcs->d_sou_align_in_bits, 0);
	sp = tp->t_str;
	sp->sou_align_in_bits = dcs->d_sou_align_in_bits;
	sp->sou_first_member = fmem;
	if (tp->t_packed)
		setpackedsize(tp);
	else
		sp->sou_size_in_bits = dcs->d_offset_in_bits;

	if (sp->sou_size_in_bits == 0) {
		/* zero sized %s is a C99 feature */
		c99ism(47, ttab[t].tt_name);
	}

	n = 0;
	for (mem = fmem; mem != NULL; mem = mem->s_next) {
		/* bind anonymous members to the structure */
		if (mem->u.s_member.sm_sou_type == NULL) {
			mem->u.s_member.sm_sou_type = sp;
			if (mem->s_type->t_bitfield) {
				sp->sou_size_in_bits += bitfieldsize(&mem);
				if (mem == NULL)
					break;
			}
			sp->sou_size_in_bits +=
			    type_size_in_bits(mem->s_type);
		}
		if (mem->s_name != unnamed)
			n++;
	}

	if (n == 0 && sp->sou_size_in_bits != 0) {
		/* %s has no named members */
		warning(65, t == STRUCT ? "structure" : "union");
	}
	return tp;
}

type_t *
complete_tag_enum(type_t *tp, sym_t *fmem)
{

	tp->t_enum->en_incomplete = false;
	tp->t_enum->en_first_enumerator = fmem;
	return tp;
}

/*
 * Processes the name of an enumerator in an enum declaration.
 *
 * sym points to the enumerator
 * val is the value of the enumerator
 * impl is true if the value of the enumerator was not explicitly specified.
 */
sym_t *
enumeration_constant(sym_t *sym, int val, bool impl)
{

	if (sym->s_scl != NOSCL) {
		if (sym->s_block_level == block_level) {
			/* no hflag, because this is illegal!!! */
			if (sym->s_arg) {
				/* enumeration constant hides parameter: %s */
				warning(57, sym->s_name);
			} else {
				/* redeclaration of %s */
				error(27, sym->s_name);
				/*
				 * inside blocks it should not be too
				 * complicated to find the position of the
				 * previous declaration
				 */
				if (block_level == 0)
					print_previous_declaration(-1, sym);
			}
		} else {
			if (hflag)
				/* redefinition hides earlier one: %s */
				warning(43, sym->s_name);
		}
		sym = pushdown(sym);
	}
	sym->s_scl = ENUM_CONST;
	sym->s_type = dcs->d_tagtyp;
	sym->u.s_enum_constant = val;
	if (impl && val == TARG_INT_MIN) {
		/* overflow in enumeration values: %s */
		warning(48, sym->s_name);
	}
	enumval = val == TARG_INT_MAX ? TARG_INT_MIN : val + 1;
	return sym;
}

/*
 * Process a single external declarator.
 */
static void
declare_extern(sym_t *dsym, bool initflg, sbuf_t *renaming)
{
	bool	dowarn, rval, redec;
	sym_t	*rdsym;
	char	*s;

	if (renaming != NULL) {
		lint_assert(dsym->s_rename == NULL);

		s = level_zero_alloc(1, renaming->sb_len + 1);
		(void)memcpy(s, renaming->sb_name, renaming->sb_len + 1);
		dsym->s_rename = s;
	}

	check_function_definition(dsym, true);

	check_type(dsym);

	if (initflg && !check_init(dsym))
		dsym->s_def = DEF;

	/*
	 * Declarations of functions are marked as "tentative" in
	 * declarator_name(). This is wrong because there are no
	 * tentative function definitions.
	 */
	if (dsym->s_type->t_tspec == FUNC && dsym->s_def == TDEF)
		dsym->s_def = DECL;

	if (dcs->d_inline) {
		if (dsym->s_type->t_tspec == FUNC) {
			dsym->s_inline = true;
		} else {
			/* variable declared inline: %s */
			warning(268, dsym->s_name);
		}
	}

	/* Write the declaration into the output file */
	if (plibflg && llibflg &&
	    dsym->s_type->t_tspec == FUNC && dsym->s_type->t_proto) {
		/*
		 * With both LINTLIBRARY and PROTOLIB the prototype is
		 * written as a function definition to the output file.
		 */
		rval = dsym->s_type->t_subt->t_tspec != VOID;
		outfdef(dsym, &dsym->s_def_pos, rval, false, NULL);
	} else if (!is_compiler_builtin(dsym->s_name)) {
		outsym(dsym, dsym->s_scl, dsym->s_def);
	}

	if ((rdsym = dcs->d_redeclared_symbol) != NULL) {

		/*
		 * If the old symbol stems from an old style function
		 * definition, we have remembered the params in
		 * rdsym->s_old_style_args and compare them with the params
		 * of the prototype.
		 */
		if (rdsym->s_osdef && dsym->s_type->t_proto) {
			redec = check_old_style_definition(rdsym, dsym);
		} else {
			redec = false;
		}

		if (!redec &&
		    !check_redeclaration(dsym, (dowarn = false, &dowarn))) {

			if (dowarn) {
				/* TODO: Make this an error in C99 mode as well. */
				if (!allow_trad && !allow_c99)
					/* redeclaration of %s */
					error(27, dsym->s_name);
				else
					/* redeclaration of %s */
					warning(27, dsym->s_name);
				print_previous_declaration(-1, rdsym);
			}

			/*
			 * Take over the remembered params if the new symbol
			 * is not a prototype.
			 */
			if (rdsym->s_osdef && !dsym->s_type->t_proto) {
				dsym->s_osdef = rdsym->s_osdef;
				dsym->u.s_old_style_args =
				    rdsym->u.s_old_style_args;
				dsym->s_def_pos = rdsym->s_def_pos;
			}

			/*
			 * Remember the position of the declaration if the
			 * old symbol was a prototype and the new is not.
			 * Also remember the position if the old symbol
			 * was defined and the new is not.
			 */
			if (rdsym->s_type->t_proto && !dsym->s_type->t_proto) {
				dsym->s_def_pos = rdsym->s_def_pos;
			} else if (rdsym->s_def == DEF && dsym->s_def != DEF) {
				dsym->s_def_pos = rdsym->s_def_pos;
			}

			/*
			 * Copy usage information of the name into the new
			 * symbol.
			 */
			copy_usage_info(dsym, rdsym);

			/* Once a name is defined, it remains defined. */
			if (rdsym->s_def == DEF)
				dsym->s_def = DEF;

			/* once a function is inline, it remains inline */
			if (rdsym->s_inline)
				dsym->s_inline = true;

			complete_type(dsym, rdsym);

		}

		rmsym(rdsym);
	}

	if (dsym->s_scl == TYPEDEF) {
		dsym->s_type = block_dup_type(dsym->s_type);
		dsym->s_type->t_typedef = true;
		settdsym(dsym->s_type, dsym);
	}

}

void
declare(sym_t *decl, bool initflg, sbuf_t *renaming)
{

	if (dcs->d_kind == DK_EXTERN) {
		declare_extern(decl, initflg, renaming);
	} else if (dcs->d_kind == DK_OLD_STYLE_ARG ||
		   dcs->d_kind == DK_PROTO_ARG) {
		if (renaming != NULL) {
			/* symbol renaming can't be used on function arguments */
			error(310);
		} else
			(void)declare_argument(decl, initflg);
	} else {
		lint_assert(dcs->d_kind == DK_AUTO);
		if (renaming != NULL) {
			/* symbol renaming can't be used on automatic variables */
			error(311);
		} else
			declare_local(decl, initflg);
	}
}

/*
 * Copies information about usage into a new symbol table entry of
 * the same symbol.
 */
void
copy_usage_info(sym_t *sym, sym_t *rdsym)
{

	sym->s_set_pos = rdsym->s_set_pos;
	sym->s_use_pos = rdsym->s_use_pos;
	sym->s_set = rdsym->s_set;
	sym->s_used = rdsym->s_used;
}

/*
 * Prints an error and returns true if a symbol is redeclared/redefined.
 * Otherwise returns false and, in some cases of minor problems, prints
 * a warning.
 */
bool
check_redeclaration(sym_t *dsym, bool *dowarn)
{
	sym_t	*rsym;

	rsym = dcs->d_redeclared_symbol;
	if (rsym->s_scl == ENUM_CONST) {
		/* redeclaration of %s */
		error(27, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (rsym->s_scl == TYPEDEF) {
		/* typedef redeclared: %s */
		error(89, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (dsym->s_scl == TYPEDEF) {
		/* redeclaration of %s */
		error(27, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (rsym->s_def == DEF && dsym->s_def == DEF) {
		/* redefinition of %s */
		error(28, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (!eqtype(rsym->s_type, dsym->s_type, false, false, dowarn)) {
		/* redeclaration of '%s' with type '%s', expected '%s' */
		error(347, dsym->s_name,
		    type_name(dsym->s_type), type_name(rsym->s_type));
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (rsym->s_scl == EXTERN && dsym->s_scl == EXTERN)
		return false;
	if (rsym->s_scl == STATIC && dsym->s_scl == STATIC)
		return false;
	if (rsym->s_scl == STATIC && dsym->s_def == DECL)
		return false;
	if (rsym->s_scl == EXTERN && rsym->s_def == DEF) {
		/*
		 * All cases except "int a = 1; static int a;" are caught
		 * above with or without a warning
		 */
		/* redeclaration of %s */
		error(27, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return true;
	}
	if (rsym->s_scl == EXTERN) {
		/* previously declared extern, becomes static: %s */
		warning(29, dsym->s_name);
		print_previous_declaration(-1, rsym);
		return false;
	}
	/*
	 * Now it's one of:
	 * "static a; int a;", "static a; int a = 1;", "static a = 1; int a;"
	 */
	/* redeclaration of %s; ANSI C requires "static" */
	/* TODO: Make this an error in C99 mode as well. */
	if (!allow_trad && !allow_c99) {
		/* redeclaration of %s; ANSI C requires static */
		warning(30, dsym->s_name);
		print_previous_declaration(-1, rsym);
	}
	dsym->s_scl = STATIC;
	return false;
}

static bool
qualifiers_correspond(const type_t *tp1, const type_t *tp2, bool ignqual)
{
	if (tp1->t_const != tp2->t_const && !ignqual && allow_c90)
		return false;

	if (tp1->t_volatile != tp2->t_volatile && !ignqual && allow_c90)
		return false;

	return true;
}

bool
eqptrtype(const type_t *tp1, const type_t *tp2, bool ignqual)
{
	if (tp1->t_tspec != VOID && tp2->t_tspec != VOID)
		return false;

	if (!qualifiers_correspond(tp1, tp2, ignqual))
		return false;

	return true;
}


/*
 * Checks if two types are compatible.
 *
 * ignqual	ignore qualifiers of type; used for function params
 * promot	promote left type; used for comparison of params of
 *		old style function definitions with params of prototypes.
 * *dowarn	set to true if an old style function declaration is not
 *		compatible with a prototype
 */
bool
eqtype(const type_t *tp1, const type_t *tp2,
       bool ignqual, bool promot, bool *dowarn)
{
	tspec_t	t;

	while (tp1 != NULL && tp2 != NULL) {

		t = tp1->t_tspec;
		if (promot) {
			if (t == FLOAT) {
				t = DOUBLE;
			} else if (t == CHAR || t == SCHAR) {
				t = INT;
			} else if (t == UCHAR) {
				t = allow_c90 ? INT : UINT;
			} else if (t == SHORT) {
				t = INT;
			} else if (t == USHORT) {
				/* CONSTCOND */
				t = TARG_INT_MAX < TARG_USHRT_MAX || !allow_c90
				    ? UINT : INT;
			}
		}

		if (t != tp2->t_tspec)
			return false;

		if (!qualifiers_correspond(tp1, tp2, ignqual))
			return false;

		if (t == STRUCT || t == UNION)
			return tp1->t_str == tp2->t_str;

		if (t == ENUM && eflag)
			return tp1->t_enum == tp2->t_enum;

		if (t == ARRAY && tp1->t_dim != tp2->t_dim) {
			if (tp1->t_dim != 0 && tp2->t_dim != 0)
				return false;
		}

		/* don't check prototypes for traditional */
		if (t == FUNC && allow_c90) {
			if (tp1->t_proto && tp2->t_proto) {
				if (!eqargs(tp1, tp2, dowarn))
					return false;
			} else if (tp1->t_proto) {
				if (!mnoarg(tp1, dowarn))
					return false;
			} else if (tp2->t_proto) {
				if (!mnoarg(tp2, dowarn))
					return false;
			}
		}

		tp1 = tp1->t_subt;
		tp2 = tp2->t_subt;
		ignqual = promot = false;

	}

	return tp1 == tp2;
}

/*
 * Compares the parameter types of two prototypes.
 */
static bool
eqargs(const type_t *tp1, const type_t *tp2, bool *dowarn)
{
	sym_t	*a1, *a2;

	if (tp1->t_vararg != tp2->t_vararg)
		return false;

	a1 = tp1->t_args;
	a2 = tp2->t_args;

	while (a1 != NULL && a2 != NULL) {

		if (!eqtype(a1->s_type, a2->s_type, true, false, dowarn))
			return false;

		a1 = a1->s_next;
		a2 = a2->s_next;

	}

	return a1 == a2;
}

/*
 * mnoarg() (matches functions with no argument type information)
 * returns whether all parameters of a prototype are compatible with
 * an old style function declaration.
 * This is the case if the following conditions are met:
 *	1. the prototype has a fixed number of parameters
 *	2. no parameter is of type float
 *	3. no parameter is converted to another type if integer promotion
 *	   is applied on it
 */
static bool
mnoarg(const type_t *tp, bool *dowarn)
{
	sym_t	*arg;
	tspec_t	t;

	if (tp->t_vararg) {
		if (dowarn != NULL)
			*dowarn = true;
	}
	for (arg = tp->t_args; arg != NULL; arg = arg->s_next) {
		if ((t = arg->s_type->t_tspec) == FLOAT ||
		    t == CHAR || t == SCHAR || t == UCHAR ||
		    t == SHORT || t == USHORT) {
			if (dowarn != NULL)
				*dowarn = true;
		}
	}
	return true;
}

/*
 * Compares a prototype declaration with the remembered arguments of
 * a previous old style function definition.
 */
static bool
check_old_style_definition(sym_t *rdsym, sym_t *dsym)
{
	sym_t	*args, *pargs, *arg, *parg;
	int	narg, nparg, n;
	bool	dowarn, msg;

	args = rdsym->u.s_old_style_args;
	pargs = dsym->s_type->t_args;

	msg = false;

	narg = nparg = 0;
	for (arg = args; arg != NULL; arg = arg->s_next)
		narg++;
	for (parg = pargs; parg != NULL; parg = parg->s_next)
		nparg++;
	if (narg != nparg) {
		/* prototype does not match old-style definition */
		error(63);
		msg = true;
		goto end;
	}

	arg = args;
	parg = pargs;
	n = 1;
	while (narg-- > 0) {
		dowarn = false;
		/*
		 * If it does not match due to promotion and lint runs in
		 * "traditional to C90" migration mode, print only a warning.
		 *
		 * XXX: Where is this "only a warning"?
		 */
		if (!eqtype(arg->s_type, parg->s_type, true, true, &dowarn) ||
		    dowarn) {
			/* prototype does not match old style ... */
			error(299, n);
			msg = true;
		}
		arg = arg->s_next;
		parg = parg->s_next;
		n++;
	}

end:
	if (msg)
		/* old style definition */
		print_previous_declaration(300, rdsym);

	return msg;
}

/*
 * Completes a type by copying the dimension and prototype information
 * from a second compatible type.
 *
 * Following lines are legal:
 *  "typedef a[]; a b; a b[10]; a c; a c[20];"
 *  "typedef ft(); ft f; f(int); ft g; g(long);"
 * This means that, if a type is completed, the type structure must
 * be duplicated.
 */
void
complete_type(sym_t *dsym, sym_t *ssym)
{
	type_t	**dstp, *src;
	type_t	*dst;

	dstp = &dsym->s_type;
	src = ssym->s_type;

	while ((dst = *dstp) != NULL) {
		lint_assert(src != NULL);
		lint_assert(dst->t_tspec == src->t_tspec);
		if (dst->t_tspec == ARRAY) {
			if (dst->t_dim == 0 && src->t_dim != 0) {
				*dstp = dst = block_dup_type(dst);
				dst->t_dim = src->t_dim;
				dst->t_incomplete_array = false;
			}
		} else if (dst->t_tspec == FUNC) {
			if (!dst->t_proto && src->t_proto) {
				*dstp = dst = block_dup_type(dst);
				dst->t_proto = true;
				dst->t_args = src->t_args;
			}
		}
		dstp = &dst->t_subt;
		src = src->t_subt;
	}
}

/*
 * Completes the declaration of a single argument.
 */
sym_t *
declare_argument(sym_t *sym, bool initflg)
{
	tspec_t	t;

	check_function_definition(sym, true);

	check_type(sym);

	if (dcs->d_redeclared_symbol != NULL &&
	    dcs->d_redeclared_symbol->s_block_level == block_level) {
		/* redeclaration of formal parameter %s */
		error(237, sym->s_name);
		rmsym(dcs->d_redeclared_symbol);
		sym->s_arg = true;
	}

	if (!sym->s_arg) {
		/* declared argument %s is missing */
		error(53, sym->s_name);
		sym->s_arg = true;
	}

	if (initflg) {
		/* cannot initialize parameter: %s */
		error(52, sym->s_name);
	}

	if (sym->s_type == NULL)	/* for c(void()) */
		sym->s_type = gettyp(VOID);

	if ((t = sym->s_type->t_tspec) == ARRAY) {
		sym->s_type = block_derive_type(sym->s_type->t_subt, PTR);
	} else if (t == FUNC) {
		if (!allow_c90)
			/* a function is declared as an argument: %s */
			warning(50, sym->s_name);
		sym->s_type = block_derive_type(sym->s_type, PTR);
	} else if (t == FLOAT) {
		if (!allow_c90)
			sym->s_type = gettyp(DOUBLE);
	}

	if (dcs->d_inline)
		/* argument declared inline: %s */
		warning(269, sym->s_name);

	/*
	 * Arguments must have complete types. length_in_bits prints the
	 * needed error messages (null dimension is impossible because arrays
	 * are converted to pointers).
	 */
	if (sym->s_type->t_tspec != VOID)
		(void)length_in_bits(sym->s_type, sym->s_name);

	sym->s_used = dcs->d_used;
	mark_as_set(sym);

	return sym;
}

void
check_func_lint_directives(void)
{
	sym_t *arg;
	int narg, n;
	tspec_t t;

	/* check for illegal combinations of lint directives */
	if (printflike_argnum != -1 && scanflike_argnum != -1) {
		/* can't be used together: ** PRINTFLIKE ** ** SCANFLIKE ** */
		warning(289);
		printflike_argnum = scanflike_argnum = -1;
	}
	if (nvararg != -1 &&
	    (printflike_argnum != -1 || scanflike_argnum != -1)) {
		/* dubious use of ** VARARGS ** with ** %s ** */
		warning(288,
		    printflike_argnum != -1 ? "PRINTFLIKE" : "SCANFLIKE");
		nvararg = -1;
	}

	/*
	 * check if the argument of a lint directive is compatible with the
	 * number of arguments.
	 */
	narg = 0;
	for (arg = dcs->d_func_args; arg != NULL; arg = arg->s_next)
		narg++;
	if (nargusg > narg) {
		/* argument number mismatch with directive: ** %s ** */
		warning(283, "ARGSUSED");
		nargusg = 0;
	}
	if (nvararg > narg) {
		/* argument number mismatch with directive: ** %s ** */
		warning(283, "VARARGS");
		nvararg = 0;
	}
	if (printflike_argnum > narg) {
		/* argument number mismatch with directive: ** %s ** */
		warning(283, "PRINTFLIKE");
		printflike_argnum = -1;
	} else if (printflike_argnum == 0) {
		printflike_argnum = -1;
	}
	if (scanflike_argnum > narg) {
		/* argument number mismatch with directive: ** %s ** */
		warning(283, "SCANFLIKE");
		scanflike_argnum = -1;
	} else if (scanflike_argnum == 0) {
		scanflike_argnum = -1;
	}
	if (printflike_argnum != -1 || scanflike_argnum != -1) {
		narg = printflike_argnum != -1
		    ? printflike_argnum : scanflike_argnum;
		arg = dcs->d_func_args;
		for (n = 1; n < narg; n++)
			arg = arg->s_next;
		if (arg->s_type->t_tspec != PTR ||
		    ((t = arg->s_type->t_subt->t_tspec) != CHAR &&
		     t != UCHAR && t != SCHAR)) {
			/* argument %d must be 'char *' for PRINTFLIKE/... */
			warning(293, narg);
			printflike_argnum = scanflike_argnum = -1;
		}
	}
}

/*
 * Warn about arguments in old style function definitions that default to int.
 * Check that an old style function definition is compatible to a previous
 * prototype.
 */
void
check_func_old_style_arguments(void)
{
	sym_t *args, *arg, *pargs, *parg;
	int narg, nparg;
	bool msg;

	args = funcsym->u.s_old_style_args;
	pargs = funcsym->s_type->t_args;

	/*
	 * print a warning for each argument of an old style function
	 * definition which defaults to int
	 */
	for (arg = args; arg != NULL; arg = arg->s_next) {
		if (arg->s_defarg) {
			/* argument type defaults to 'int': %s */
			warning(32, arg->s_name);
			arg->s_defarg = false;
			mark_as_set(arg);
		}
	}

	/*
	 * If this is an old style function definition and a prototype
	 * exists, compare the types of arguments.
	 */
	if (funcsym->s_osdef && funcsym->s_type->t_proto) {
		/*
		 * If the number of arguments does not match, we need not
		 * continue.
		 */
		narg = nparg = 0;
		msg = false;
		for (parg = pargs; parg != NULL; parg = parg->s_next)
			nparg++;
		for (arg = args; arg != NULL; arg = arg->s_next)
			narg++;
		if (narg != nparg) {
			/* parameter mismatch: %d declared, %d defined */
			error(51, nparg, narg);
			msg = true;
		} else {
			parg = pargs;
			arg = args;
			while (narg-- > 0) {
				msg |= check_prototype_declaration(arg, parg);
				parg = parg->s_next;
				arg = arg->s_next;
			}
		}
		if (msg)
			/* prototype declaration */
			print_previous_declaration(285,
			    dcs->d_redeclared_symbol);

		/* from now on the prototype is valid */
		funcsym->s_osdef = false;
		funcsym->u.s_old_style_args = NULL;
	}
}

/*
 * Checks compatibility of an old style function definition with a previous
 * prototype declaration.
 * Returns true if the position of the previous declaration should be reported.
 */
static bool
check_prototype_declaration(sym_t *arg, sym_t *parg)
{
	type_t	*tp, *ptp;
	bool	dowarn;

	tp = arg->s_type;
	ptp = parg->s_type;

	dowarn = false;

	if (!eqtype(tp, ptp, true, true, &dowarn)) {
		if (eqtype(tp, ptp, true, false, &dowarn)) {
			/* type does not match prototype: %s */
			return gnuism(58, arg->s_name);
		} else {
			/* type does not match prototype: %s */
			error(58, arg->s_name);
			return true;
		}
	} else if (dowarn) {
		/* TODO: Make this an error in C99 mode as well. */
		if (!allow_trad && !allow_c99)
			/* type does not match prototype: %s */
			error(58, arg->s_name);
		else
			/* type does not match prototype: %s */
			warning(58, arg->s_name);
		return true;
	}

	return false;
}

static void
check_local_hiding(const sym_t *dsym)
{
	switch (dsym->s_scl) {
	case AUTO:
		/* automatic hides external declaration: %s */
		warning(86, dsym->s_name);
		break;
	case STATIC:
		/* static hides external declaration: %s */
		warning(87, dsym->s_name);
		break;
	case TYPEDEF:
		/* typedef hides external declaration: %s */
		warning(88, dsym->s_name);
		break;
	case EXTERN:
		/* Already checked in declare_external_in_block. */
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
}

static void
check_local_redeclaration(const sym_t *dsym, sym_t *rsym)
{
	if (rsym->s_block_level == 0) {
		if (hflag)
			check_local_hiding(dsym);

	} else if (rsym->s_block_level == block_level) {

		/* no hflag, because it's illegal! */
		if (rsym->s_arg) {
			/*
			 * if allow_c90, a "redeclaration of %s" error
			 * is produced below
			 */
			if (!allow_c90) {
				if (hflag)
					/* declaration hides parameter: %s */
					warning(91, dsym->s_name);
				rmsym(rsym);
			}
		}

	} else if (rsym->s_block_level < block_level) {
		if (hflag)
			/* declaration hides earlier one: %s */
			warning(95, dsym->s_name);
	}

	if (rsym->s_block_level == block_level) {
		/* redeclaration of %s */
		error(27, dsym->s_name);
		rmsym(rsym);
	}
}

/*
 * Completes a single local declaration/definition.
 */
void
declare_local(sym_t *dsym, bool initflg)
{

	/* Correct a mistake done in declarator_name(). */
	if (dsym->s_type->t_tspec == FUNC) {
		dsym->s_def = DECL;
		if (dcs->d_scl == NOSCL)
			dsym->s_scl = EXTERN;
	}

	if (dsym->s_type->t_tspec == FUNC) {
		if (dsym->s_scl == STATIC) {
			/* dubious static function at block level: %s */
			warning(93, dsym->s_name);
			dsym->s_scl = EXTERN;
		} else if (dsym->s_scl != EXTERN && dsym->s_scl != TYPEDEF) {
			/* function has illegal storage class: %s */
			error(94, dsym->s_name);
			dsym->s_scl = EXTERN;
		}
	}

	/*
	 * functions may be declared inline at local scope, although
	 * this has no effect for a later definition of the same
	 * function.
	 * XXX it should have an effect if !allow_c90 is set. this would
	 * also be the way gcc behaves.
	 */
	if (dcs->d_inline) {
		if (dsym->s_type->t_tspec == FUNC) {
			dsym->s_inline = true;
		} else {
			/* variable declared inline: %s */
			warning(268, dsym->s_name);
		}
	}

	check_function_definition(dsym, true);

	check_type(dsym);

	if (dcs->d_redeclared_symbol != NULL && dsym->s_scl == EXTERN)
		declare_external_in_block(dsym);

	if (dsym->s_scl == EXTERN) {
		/*
		 * XXX if the static variable at level 0 is only defined
		 * later, checking will be possible.
		 */
		if (dsym->s_ext_sym == NULL) {
			outsym(dsym, EXTERN, dsym->s_def);
		} else {
			outsym(dsym, dsym->s_ext_sym->s_scl, dsym->s_def);
		}
	}

	if (dcs->d_redeclared_symbol != NULL)
		check_local_redeclaration(dsym, dcs->d_redeclared_symbol);

	if (initflg && !check_init(dsym)) {
		dsym->s_def = DEF;
		mark_as_set(dsym);
	}

	if (dsym->s_scl == TYPEDEF) {
		dsym->s_type = block_dup_type(dsym->s_type);
		dsym->s_type->t_typedef = true;
		settdsym(dsym->s_type, dsym);
	}

	/*
	 * Before we can check the size we must wait for a initialization
	 * which may follow.
	 */
}

/*
 * Processes (re)declarations of external symbols inside blocks.
 */
static void
declare_external_in_block(sym_t *dsym)
{
	bool	eqt, dowarn;
	sym_t	*esym;

	/* look for a symbol with the same name */
	esym = dcs->d_redeclared_symbol;
	while (esym != NULL && esym->s_block_level != 0) {
		while ((esym = esym->s_symtab_next) != NULL) {
			if (esym->s_kind != FVFT)
				continue;
			if (strcmp(dsym->s_name, esym->s_name) == 0)
				break;
		}
	}
	if (esym == NULL)
		return;
	if (esym->s_scl != EXTERN && esym->s_scl != STATIC) {
		/* gcc accepts this without a warning, pcc prints an error. */
		/* redeclaration of %s */
		warning(27, dsym->s_name);
		print_previous_declaration(-1, esym);
		return;
	}

	dowarn = false;
	eqt = eqtype(esym->s_type, dsym->s_type, false, false, &dowarn);

	if (!eqt || dowarn) {
		if (esym->s_scl == EXTERN) {
			/* inconsistent redeclaration of extern: %s */
			warning(90, dsym->s_name);
			print_previous_declaration(-1, esym);
		} else {
			/* inconsistent redeclaration of static: %s */
			warning(92, dsym->s_name);
			print_previous_declaration(-1, esym);
		}
	}

	if (eqt) {
		/*
		 * Remember the external symbol so we can update usage
		 * information at the end of the block.
		 */
		dsym->s_ext_sym = esym;
	}
}

/*
 * Print an error or a warning if the symbol cannot be initialized due
 * to type/storage class. Return whether an error has been detected.
 */
static bool
check_init(sym_t *sym)
{
	bool	erred;

	erred = false;

	if (sym->s_type->t_tspec == FUNC) {
		/* cannot initialize function: %s */
		error(24, sym->s_name);
		erred = true;
	} else if (sym->s_scl == TYPEDEF) {
		/* cannot initialize typedef: %s */
		error(25, sym->s_name);
		erred = true;
	} else if (sym->s_scl == EXTERN && sym->s_def == DECL) {
		/* cannot initialize "extern" declaration: %s */
		if (dcs->d_kind == DK_EXTERN) {
			/* cannot initialize extern declaration: %s */
			warning(26, sym->s_name);
		} else {
			/* cannot initialize extern declaration: %s */
			error(26, sym->s_name);
			erred = true;
		}
	}

	return erred;
}

/*
 * Create a symbol for an abstract declaration.
 */
sym_t *
abstract_name(void)
{
	sym_t	*sym;

	lint_assert(dcs->d_kind == DK_ABSTRACT ||
	    dcs->d_kind == DK_PROTO_ARG);

	sym = block_zero_alloc(sizeof(*sym));

	sym->s_name = unnamed;
	sym->s_def = DEF;
	sym->s_scl = ABSTRACT;
	sym->s_block_level = -1;

	if (dcs->d_kind == DK_PROTO_ARG)
		sym->s_arg = true;

	/*
	 * At this point, dcs->d_type contains only the basic type.  That
	 * type will be updated later, adding pointers, arrays and functions
	 * as necessary.
	 */
	/*
	 * XXX: This is not the correct type.  For example in msg_347, it is
	 * the type of the last prototype parameter, but it should rather be
	 * the return type of the function.
	 */
	sym->s_type = dcs->d_type;
	dcs->d_redeclared_symbol = NULL;
	dcs->d_vararg = false;

	return sym;
}

/*
 * Removes anything which has nothing to do on global level.
 */
void
global_clean_up(void)
{

	while (dcs->d_enclosing != NULL)
		end_declaration_level();

	clean_up_after_error();
	block_level = 0;
	mem_block_level = 0;

	/*
	 * remove all information about pending lint directives without
	 * warnings.
	 */
	global_clean_up_decl(true);
}

/*
 * Process an abstract type declaration
 */
sym_t *
declare_1_abstract(sym_t *sym)
{

	check_function_definition(sym, true);
	check_type(sym);
	return sym;
}

/*
 * Checks size after declarations of variables and their initialization.
 */
void
check_size(sym_t *dsym)
{

	if (dsym->s_def != DEF)
		return;
	if (dsym->s_scl == TYPEDEF)
		return;
	if (dsym->s_type->t_tspec == FUNC)
		return;

	if (length_in_bits(dsym->s_type, dsym->s_name) == 0 &&
	    dsym->s_type->t_tspec == ARRAY && dsym->s_type->t_dim == 0) {
		if (!allow_c90) {
			/* empty array declaration: %s */
			warning(190, dsym->s_name);
		} else {
			/* empty array declaration: %s */
			error(190, dsym->s_name);
		}
	}
}

/*
 * Mark an object as set if it is not already
 */
void
mark_as_set(sym_t *sym)
{

	if (!sym->s_set) {
		sym->s_set = true;
		UNIQUE_CURR_POS(sym->s_set_pos);
	}
}

/*
 * Mark an object as used if it is not already
 */
void
mark_as_used(sym_t *sym, bool fcall, bool szof)
{

	if (!sym->s_used) {
		sym->s_used = true;
		UNIQUE_CURR_POS(sym->s_use_pos);
	}
	/*
	 * for function calls another record is written
	 *
	 * XXX Should symbols used in sizeof() be treated as used or not?
	 * Probably not, because there is no sense to declare an
	 * external variable only to get their size.
	 */
	if (!fcall && !szof && sym->s_kind == FVFT && sym->s_scl == EXTERN)
		outusg(sym);
}

/*
 * Prints warnings for a list of variables and labels (concatenated
 * with s_level_next) if these are not used or only set.
 */
void
check_usage(dinfo_t *di)
{
	sym_t	*sym;
	int	mklwarn;

	/* for this warning LINTED has no effect */
	mklwarn = lwarn;
	lwarn = LWARN_ALL;

	debug_step("begin lwarn %d", lwarn);
	for (sym = di->d_dlsyms; sym != NULL; sym = sym->s_level_next)
		check_usage_sym(di->d_asm, sym);
	lwarn = mklwarn;
	debug_step("end lwarn %d", lwarn);
}

/*
 * Prints a warning for a single variable or label if it is not used or
 * only set.
 */
void
check_usage_sym(bool novar, sym_t *sym)
{

	if (sym->s_block_level == -1)
		return;

	if (sym->s_kind == FVFT && sym->s_arg)
		check_argument_usage(novar, sym);
	else if (sym->s_kind == FVFT)
		check_variable_usage(novar, sym);
	else if (sym->s_kind == FLABEL)
		check_label_usage(sym);
	else if (sym->s_kind == FTAG)
		check_tag_usage(sym);
}

static void
check_argument_usage(bool novar, sym_t *arg)
{

	lint_assert(arg->s_set);

	if (novar)
		return;

	if (!arg->s_used && vflag) {
		/* argument '%s' unused in function '%s' */
		warning_at(231, &arg->s_def_pos, arg->s_name, funcsym->s_name);
	}
}

static void
check_variable_usage(bool novar, sym_t *sym)
{
	scl_t	sc;
	sym_t	*xsym;

	lint_assert(block_level != 0);

	/* example at file scope: int c = ({ return 3; }); */
	if (sym->s_block_level == 0 && ch_isdigit(sym->s_name[0]))
		return;

	/* errors in expressions easily cause lots of these warnings */
	if (nerr != 0)
		return;

	/*
	 * XXX Only variables are checked, although types should
	 * probably also be checked
	 */
	if ((sc = sym->s_scl) != EXTERN && sc != STATIC &&
	    sc != AUTO && sc != REG) {
		return;
	}

	if (novar)
		return;

	if (sc == EXTERN) {
		if (!sym->s_used && !sym->s_set) {
			/* '%s' unused in function '%s' */
			warning_at(192, &sym->s_def_pos,
			    sym->s_name, funcsym->s_name);
		}
	} else {
		if (sym->s_set && !sym->s_used) {
			/* '%s' set but not used in function '%s' */
			warning_at(191, &sym->s_set_pos,
			    sym->s_name, funcsym->s_name);
		} else if (!sym->s_used) {
			/* '%s' unused in function '%s' */
			warning_at(192, &sym->s_def_pos,
			    sym->s_name, funcsym->s_name);
		}
	}

	if (sc == EXTERN) {
		/*
		 * information about usage is taken over into the symbol
		 * table entry at level 0 if the symbol was locally declared
		 * as an external symbol.
		 *
		 * XXX This is wrong for symbols declared static at level 0
		 * if the usage information stems from sizeof(). This is
		 * because symbols at level 0 only used in sizeof() are
		 * considered to not be used.
		 */
		if ((xsym = sym->s_ext_sym) != NULL) {
			if (sym->s_used && !xsym->s_used) {
				xsym->s_used = true;
				xsym->s_use_pos = sym->s_use_pos;
			}
			if (sym->s_set && !xsym->s_set) {
				xsym->s_set = true;
				xsym->s_set_pos = sym->s_set_pos;
			}
		}
	}
}

static void
check_label_usage(sym_t *lab)
{

	lint_assert(block_level == 1);
	lint_assert(lab->s_block_level == 1);

	if (lab->s_set && !lab->s_used) {
		/* label '%s' unused in function '%s' */
		warning_at(232, &lab->s_set_pos, lab->s_name, funcsym->s_name);
	} else if (!lab->s_set) {
		/* undefined label '%s' */
		warning_at(23, &lab->s_use_pos, lab->s_name);
	}
}

static void
check_tag_usage(sym_t *sym)
{

	if (!is_incomplete(sym->s_type))
		return;

	/* always complain about incomplete tags declared inside blocks */
	if (!zflag || dcs->d_kind != DK_EXTERN)
		return;

	switch (sym->s_type->t_tspec) {
	case STRUCT:
		/* struct %s never defined */
		warning_at(233, &sym->s_def_pos, sym->s_name);
		break;
	case UNION:
		/* union %s never defined */
		warning_at(234, &sym->s_def_pos, sym->s_name);
		break;
	case ENUM:
		/* enum %s never defined */
		warning_at(235, &sym->s_def_pos, sym->s_name);
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
}

/*
 * Called after the entire translation unit has been parsed.
 * Changes tentative definitions into definitions.
 * Performs some tests on global symbols. Detected problems are:
 * - defined variables of incomplete type
 * - constant variables which are not initialized
 * - static symbols which are never used
 */
void
check_global_symbols(void)
{
	sym_t	*sym;

	if (block_level != 0 || dcs->d_enclosing != NULL)
		norecover();

	for (sym = dcs->d_dlsyms; sym != NULL; sym = sym->s_level_next) {
		if (sym->s_block_level == -1)
			continue;
		if (sym->s_kind == FVFT) {
			check_global_variable(sym);
		} else if (sym->s_kind == FTAG) {
			check_tag_usage(sym);
		} else {
			lint_assert(sym->s_kind == FMEMBER);
		}
	}
}

static void
check_unused_static_global_variable(const sym_t *sym)
{
	if (sym->s_type->t_tspec == FUNC) {
		if (sym->s_def == DEF) {
			if (!sym->s_inline)
				/* static function %s unused */
				warning_at(236, &sym->s_def_pos, sym->s_name);
		} else {
			/* static function %s declared but not defined */
			warning_at(290, &sym->s_def_pos, sym->s_name);
		}
	} else if (!sym->s_set) {
		/* static variable %s unused */
		warning_at(226, &sym->s_def_pos, sym->s_name);
	} else {
		/* static variable %s set but not used */
		warning_at(307, &sym->s_def_pos, sym->s_name);
	}
}

static void
check_static_global_variable(const sym_t *sym)
{
	if (sym->s_type->t_tspec == FUNC && sym->s_used && sym->s_def != DEF) {
		/* static function called but not defined: %s() */
		error_at(225, &sym->s_use_pos, sym->s_name);
	}

	if (!sym->s_used)
		check_unused_static_global_variable(sym);

	if (allow_c90 && sym->s_def == TDEF && sym->s_type->t_const) {
		/* const object %s should have initializer */
		warning_at(227, &sym->s_def_pos, sym->s_name);
	}
}

static void
check_global_variable(const sym_t *sym)
{
	scl_t scl = sym->s_scl;

	if (scl == TYPEDEF || scl == BOOL_CONST || scl == ENUM_CONST)
		return;

	if (scl == NOSCL)
		return;		/* May be caused by a syntax error. */

	lint_assert(scl == EXTERN || scl == STATIC);

	check_global_variable_size(sym);

	if (scl == STATIC)
		check_static_global_variable(sym);
}

static void
check_global_variable_size(const sym_t *sym)
{
	pos_t cpos;
	int len_in_bits;

	if (sym->s_def != TDEF)
		return;
	if (sym->s_type->t_tspec == FUNC)
		/*
		 * this can happen if a syntax error occurred after a
		 * function declaration
		 */
		return;
	if (sym->s_def == TDEF && sym->s_type->t_tspec == VOID)
		return;		/* prevent internal error in length_in_bits
				 * below */

	cpos = curr_pos;
	curr_pos = sym->s_def_pos;
	len_in_bits = length_in_bits(sym->s_type, sym->s_name);
	curr_pos = cpos;

	if (len_in_bits == 0 &&
	    sym->s_type->t_tspec == ARRAY && sym->s_type->t_dim == 0) {
		/* TODO: C99 6.7.5.2p1 defines this as an error as well. */
		if (!allow_c90 ||
		    (sym->s_scl == EXTERN && (allow_trad || allow_c99))) {
			/* empty array declaration: %s */
			warning_at(190, &sym->s_def_pos, sym->s_name);
		} else {
			/* empty array declaration: %s */
			error_at(190, &sym->s_def_pos, sym->s_name);
		}
	}
}

/*
 * Prints information about location of previous definition/declaration.
 */
void
print_previous_declaration(int msg, const sym_t *psym)
{

	if (!rflag)
		return;

	if (msg != -1) {
		(message_at)(msg, &psym->s_def_pos);
	} else if (psym->s_def == DEF || psym->s_def == TDEF) {
		/* previous definition of %s */
		message_at(261, &psym->s_def_pos, psym->s_name);
	} else {
		/* previous declaration of %s */
		message_at(260, &psym->s_def_pos, psym->s_name);
	}
}

/*
 * Gets a node for a constant and returns the value of this constant
 * as integer.
 *
 * If the node is not constant or too large for int or of type float,
 * a warning will be printed.
 *
 * to_int_constant() should be used only inside declarations. If it is used in
 * expressions, it frees the memory used for the expression.
 */
int
to_int_constant(tnode_t *tn, bool required)
{
	int	i;
	tspec_t	t;
	val_t	*v;

	v = constant(tn, required);

	if (tn == NULL) {
		i = 1;
		goto done;
	}

	/*
	 * Abstract declarations are used inside expression. To free
	 * the memory would be a fatal error.
	 * We don't free blocks that are inside casts because these
	 * will be used later to match types.
	 */
	if (tn->tn_op != CON && dcs->d_kind != DK_ABSTRACT)
		expr_free_all();

	if ((t = v->v_tspec) == FLOAT || t == DOUBLE || t == LDOUBLE) {
		i = (int)v->v_ldbl;
		/* integral constant expression expected */
		error(55);
	} else {
		i = (int)v->v_quad;
		if (is_uinteger(t)) {
			if ((uint64_t)v->v_quad > (uint64_t)TARG_INT_MAX) {
				/* integral constant too large */
				warning(56);
			}
		} else {
			if (v->v_quad > (int64_t)TARG_INT_MAX ||
			    v->v_quad < (int64_t)TARG_INT_MIN) {
				/* integral constant too large */
				warning(56);
			}
		}
	}

done:
	free(v);
	return i;
}
