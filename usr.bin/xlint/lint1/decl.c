/* $NetBSD: decl.c,v 1.398 2024/03/30 19:51:00 rillig Exp $ */

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
__RCSID("$NetBSD: decl.c,v 1.398 2024/03/30 19:51:00 rillig Exp $");
#endif

#include <sys/param.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"

const char unnamed[] = "<unnamed>";

/* shared type structures for arithmetic types and void */
static type_t typetab[NTSPEC];

/* value of next enumerator during declaration of enum types */
int enumval;

/*
 * Points to the innermost element of a stack that contains information about
 * nested declarations, such as struct declarations, function prototypes,
 * local variables.
 */
decl_level *dcs;


void
init_decl(void)
{

	/* declaration stack */
	dcs = xcalloc(1, sizeof(*dcs));
	dcs->d_kind = DLK_EXTERN;
	dcs->d_last_dlsym = &dcs->d_first_dlsym;

	if (!pflag) {
		for (size_t i = 0; i < NTSPEC; i++) {
			if (ttab[i].tt_rank_kind != RK_NONE)
				ttab[i].tt_rank_value =
				    ttab[i].tt_size_in_bits;
		}
		ttab[BOOL].tt_rank_value = 1;
	}

	if (Tflag) {
		ttab[BOOL].tt_is_integer = false;
		ttab[BOOL].tt_is_uinteger = false;
		ttab[BOOL].tt_is_arithmetic = false;
	}

	/* struct, union, enum, ptr, array and func are not shared. */
	for (int i = (int)SIGNED; i < (int)STRUCT; i++)
		typetab[i].t_tspec = (tspec_t)i;
}

/*
 * Returns a shared type structure for arithmetic types and void.  The returned
 * type must not be modified; use block_dup_type or expr_dup_type if necessary.
 */
type_t *
gettyp(tspec_t t)
{

	lint_assert((int)t < (int)STRUCT);
	/* TODO: make the return type 'const' */
	return &typetab[t];
}

type_t *
block_dup_type(const type_t *tp)
{

	debug_step("%s '%s'", __func__, type_name(tp));
	type_t *ntp = block_zero_alloc(sizeof(*ntp), "type");
	*ntp = *tp;
	return ntp;
}

/* Duplicate a type, free the allocated memory after the expression. */
type_t *
expr_dup_type(const type_t *tp)
{

	debug_step("%s '%s'", __func__, type_name(tp));
	type_t *ntp = expr_zero_alloc(sizeof(*ntp), "type");
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

	type_t *ntp = expr_zero_alloc(sizeof(*ntp), "type");
	*ntp = *tp;
	ntp->t_const = false;
	ntp->t_volatile = false;

	/*
	 * In case of a struct or union type, the members should lose their
	 * qualifiers as well, but that would require a deep copy of the struct
	 * or union type.  This in turn would defeat the type comparison in
	 * types_compatible, which simply tests whether tp1->u.sou ==
	 * tp2->u.sou.
	 */

	debug_step("%s '%s'", __func__, type_name(ntp));
	return ntp;
}

/*
 * Returns whether the type is 'void' or an incomplete array, struct, union
 * or enum.
 */
bool
is_incomplete(const type_t *tp)
{
	tspec_t t = tp->t_tspec;

	if (t == VOID)
		return true;
	if (t == ARRAY)
		return tp->t_incomplete_array;
	if (is_struct_or_union(t))
		return tp->u.sou->sou_incomplete;
	if (t == ENUM)
		return tp->u.enumer->en_incomplete;
	return false;
}

void
dcs_add_function_specifier(function_specifier fs)
{
	debug_step("%s: %s", __func__, function_specifier_name(fs));
	if (fs == FS_INLINE) {
		if (dcs->d_inline)
			/* duplicate '%s' */
			warning(10, "inline");
		dcs->d_inline = true;
	}
}

/*
 * Remember the storage class of the current declaration and detect multiple
 * storage classes.
 */
void
dcs_add_storage_class(scl_t sc)
{

	if (dcs->d_type != NULL || dcs->d_abstract_type != NO_TSPEC ||
	    dcs->d_sign_mod != NO_TSPEC || dcs->d_rank_mod != NO_TSPEC)
		/* storage class after type is obsolescent */
		warning(83);

	if (dcs->d_scl == NO_SCL)
		dcs->d_scl = sc;
	else if ((dcs->d_scl == EXTERN && sc == THREAD_LOCAL)
	    || (dcs->d_scl == THREAD_LOCAL && sc == EXTERN))
		dcs->d_scl = EXTERN;	/* ignore thread_local */
	else if ((dcs->d_scl == STATIC && sc == THREAD_LOCAL)
	    || (dcs->d_scl == THREAD_LOCAL && sc == STATIC))
		dcs->d_scl = STATIC;	/* ignore thread_local */
	else
		dcs->d_multiple_storage_classes = true;
	debug_printf("%s: ", __func__);
	debug_dcs();
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
	    : t == LLONG ? ULLONG
	    : t;
}

/*
 * Called if a list of declaration specifiers contains a typedef name
 * and other specifiers (except struct, union, enum, typedef name).
 */
static type_t *
typedef_error(type_t *td, tspec_t t)
{

	debug_step("%s: '%s' %s", __func__, type_name(td), tspec_name(t));

	tspec_t t2 = td->t_tspec;

	if ((t == SIGNED || t == UNSIGN) &&
	    (t2 == CHAR || t2 == SHORT || t2 == INT ||
	     t2 == LONG || t2 == LLONG)) {
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

	if (t != LONG)
		goto invalid;

	tspec_t lt;
	if (t2 == INT)
		lt = LONG;
	else if (t2 == UINT)
		lt = ULONG;
	else if (t2 == LONG)
		lt = LLONG;
	else if (t2 == ULONG)
		lt = ULLONG;
	else if (t2 == FLOAT)
		lt = DOUBLE;
	else if (t2 == DOUBLE)
		lt = LDOUBLE;
	else if (t2 == DCOMPLEX)
		lt = LCOMPLEX;
	else
		goto invalid;

	/* modifying typedef with '%s'; only qualifiers allowed */
	warning(5, "long");
	td = block_dup_type(gettyp(lt));
	td->t_typedef = true;
	return td;

invalid:
	dcs->d_invalid_type_combination = true;
	return td;
}

/*
 * Remember the type, modifier or typedef name returned by the parser in the
 * top element of the declaration stack. This information is used in
 * dcs_end_type to build the type used for all declarators in this declaration.
 *
 * If tp->t_typedef is true, the type comes from a previously defined typename.
 * Otherwise, it comes from a type specifier (int, long, ...) or a
 * struct/union/enum tag.
 */
void
dcs_add_type(type_t *tp)
{

	debug_step("%s: %s", __func__, type_name(tp));
	debug_dcs();
	if (tp->t_typedef) {
		/*-
		 * something like "typedef int a; int a b;"
		 * This should not happen with current grammar.
		 */
		lint_assert(dcs->d_type == NULL);
		lint_assert(dcs->d_abstract_type == NO_TSPEC);
		lint_assert(dcs->d_sign_mod == NO_TSPEC);
		lint_assert(dcs->d_rank_mod == NO_TSPEC);

		dcs->d_type = tp;
		return;
	}

	tspec_t t = tp->t_tspec;
	if (is_struct_or_union(t) || t == ENUM) {
		/* something like "int struct a ..." */
		if (dcs->d_type != NULL || dcs->d_abstract_type != NO_TSPEC ||
		    dcs->d_rank_mod != NO_TSPEC || dcs->d_sign_mod != NO_TSPEC) {
			dcs->d_invalid_type_combination = true;
			dcs->d_abstract_type = NO_TSPEC;
			dcs->d_sign_mod = NO_TSPEC;
			dcs->d_rank_mod = NO_TSPEC;
		}
		dcs->d_type = tp;
		debug_dcs();
		return;
	}

	if (dcs->d_type != NULL && !dcs->d_type->t_typedef) {
		/* something like "struct a int" */
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
			t = DCOMPLEX;	/* just as a fallback */
		}
		dcs->d_complex_mod = NO_TSPEC;
	}

	if (t == LONG && dcs->d_rank_mod == LONG) {
		/* "long long" or "long ... long" */
		t = LLONG;
		dcs->d_rank_mod = NO_TSPEC;
		if (!suppress_longlong)
			/* %s does not support 'long long' */
			c99ism(265, allow_c90 ? "C90" : "traditional C");
	}

	if (dcs->d_type != NULL && dcs->d_type->t_typedef) {
		/* something like "typedef int a; a long ..." */
		dcs->d_type = typedef_error(dcs->d_type, t);
		return;
	}

	/* now it can be only a combination of arithmetic types and void */
	if (t == SIGNED || t == UNSIGN) {
		if (dcs->d_sign_mod != NO_TSPEC)
			dcs->d_invalid_type_combination = true;
		dcs->d_sign_mod = t;
	} else if (t == SHORT || t == LONG || t == LLONG) {
		if (dcs->d_rank_mod != NO_TSPEC)
			dcs->d_invalid_type_combination = true;
		dcs->d_rank_mod = t;
	} else if (t == FLOAT || t == DOUBLE) {
		if (dcs->d_rank_mod == NO_TSPEC || dcs->d_rank_mod == LONG) {
			if (dcs->d_complex_mod != NO_TSPEC
			    || (t == FLOAT && dcs->d_rank_mod == LONG))
				dcs->d_invalid_type_combination = true;
			dcs->d_complex_mod = t;
		} else {
			if (dcs->d_abstract_type != NO_TSPEC)
				dcs->d_invalid_type_combination = true;
			dcs->d_abstract_type = t;
		}
	} else if (t == PTR) {
		dcs->d_type = tp;
	} else {
		if (dcs->d_abstract_type != NO_TSPEC)
			dcs->d_invalid_type_combination = true;
		dcs->d_abstract_type = t;
	}
	debug_dcs();
}

static void
set_first_typedef(type_t *tp, sym_t *sym)
{

	tspec_t t = tp->t_tspec;
	if (is_struct_or_union(t) && tp->u.sou->sou_first_typedef == NULL)
		tp->u.sou->sou_first_typedef = sym;
	if (t == ENUM && tp->u.enumer->en_first_typedef == NULL)
		tp->u.enumer->en_first_typedef = sym;
}

static unsigned int
bit_fields_width(const sym_t **mem, bool *named)
{
	unsigned int width = 0;
	unsigned int align = 0;
	while (*mem != NULL && (*mem)->s_type->t_bitfield) {
		if ((*mem)->s_name != unnamed)
			*named = true;
		width += (*mem)->s_type->t_bit_field_width;
		unsigned int mem_align = alignment_in_bits((*mem)->s_type);
		if (mem_align > align)
			align = mem_align;
		*mem = (*mem)->s_next;
	}
	return (width + align - 1) & -align;
}

static void
pack_struct_or_union(type_t *tp)
{

	if (!is_struct_or_union(tp->t_tspec)) {
		/* attribute '%s' ignored for '%s' */
		warning(326, "packed", type_name(tp));
		return;
	}

	unsigned int bits = 0;
	bool named = false;
	for (const sym_t *mem = tp->u.sou->sou_first_member;
	    mem != NULL; mem = mem->s_next) {
		// TODO: Maybe update mem->u.s_member.sm_offset_in_bits.
		if (mem->s_type->t_bitfield) {
			bits += bit_fields_width(&mem, &named);
			if (mem == NULL)
				break;
		}
		unsigned int mem_bits = type_size_in_bits(mem->s_type);
		if (tp->t_tspec == STRUCT)
			bits += mem_bits;
		else if (mem_bits > bits)
			bits = mem_bits;
	}
	tp->u.sou->sou_size_in_bits = bits;
	debug_dcs();
}

void
dcs_add_packed(void)
{
	if (dcs->d_type == NULL)
		dcs->d_packed = true;
	else
		pack_struct_or_union(dcs->d_type);
}

void
dcs_set_used(void)
{
	dcs->d_used = true;
}

/*
 * Remember a qualifier that is part of the declaration specifiers (and not the
 * declarator). The remembered qualifier is used by dcs_end_type for all
 * declarators.
 */
void
dcs_add_qualifiers(type_qualifiers qs)
{
	add_type_qualifiers(&dcs->d_qual, qs);
}

void
begin_declaration_level(decl_level_kind kind)
{

	decl_level *dl = xcalloc(1, sizeof(*dl));
	dl->d_enclosing = dcs;
	dl->d_kind = kind;
	dl->d_last_dlsym = &dl->d_first_dlsym;
	dcs = dl;
	debug_enter();
	debug_dcs_all();
}

void
end_declaration_level(void)
{

	debug_dcs_all();

	decl_level *dl = dcs;
	dcs = dl->d_enclosing;
	lint_assert(dcs != NULL);

	switch (dl->d_kind) {
	case DLK_STRUCT:
	case DLK_UNION:
	case DLK_ENUM:
		/*
		 * Symbols declared in (nested) structs or enums are part of
		 * the next level (they are removed from the symbol table if
		 * the symbols of the outer level are removed).
		 */
		if ((*dcs->d_last_dlsym = dl->d_first_dlsym) != NULL)
			dcs->d_last_dlsym = dl->d_last_dlsym;
		break;
	case DLK_OLD_STYLE_PARAMS:
		/*
		 * All symbols in dcs->d_first_dlsym are introduced in
		 * old-style parameter declarations (it's not clean, but
		 * possible). They are appended to the list of symbols declared
		 * in an old-style parameter identifier list or a new-style
		 * parameter type list.
		 */
		if (dl->d_first_dlsym != NULL) {
			*dl->d_last_dlsym = dcs->d_func_proto_syms;
			dcs->d_func_proto_syms = dl->d_first_dlsym;
		}
		break;
	case DLK_ABSTRACT:
		/*
		 * Append all symbols declared in the abstract declaration to
		 * the list of symbols declared in the surrounding declaration
		 * or block.
		 *
		 * XXX I'm not sure whether they should be removed from the
		 * symbol table now or later.
		 */
		if ((*dcs->d_last_dlsym = dl->d_first_dlsym) != NULL)
			dcs->d_last_dlsym = dl->d_last_dlsym;
		break;
	case DLK_AUTO:
		check_usage(dl);
		/* FALLTHROUGH */
	case DLK_PROTO_PARAMS:
		/* usage of parameters will be checked by end_function() */
		symtab_remove_level(dl->d_first_dlsym);
		break;
	case DLK_EXTERN:
		/* there is nothing around an external declaration */
		/* FALLTHROUGH */
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	free(dl);
	debug_leave();
}

/*
 * Set flag d_asm in all declaration stack elements up to the outermost one.
 *
 * This is used to mark compound statements which have, possibly in nested
 * compound statements, asm statements. For these compound statements, no
 * warnings about unused or uninitialized variables are printed.
 *
 * There is no need to clear d_asm in decl_level structs with context AUTO, as
 * these structs are freed at the end of the compound statement. But it must be
 * cleared in the outermost decl_level struct, which has context EXTERN. This
 * could be done in dcs_begin_type and would work for C90, but not for C99 or
 * C++ (due to mixed statements and declarations). Thus, we clear it in
 * global_clean_up_decl.
 */
void
dcs_set_asm(void)
{

	for (decl_level *dl = dcs; dl != NULL; dl = dl->d_enclosing)
		dl->d_asm = true;
}

void
dcs_begin_type(void)
{

	debug_enter();

	// keep d_kind
	dcs->d_abstract_type = NO_TSPEC;
	dcs->d_complex_mod = NO_TSPEC;
	dcs->d_sign_mod = NO_TSPEC;
	dcs->d_rank_mod = NO_TSPEC;
	dcs->d_scl = NO_SCL;
	dcs->d_type = NULL;
	dcs->d_redeclared_symbol = NULL;
	// keep d_sou_size_in_bits
	// keep d_sou_align_in_bits
	dcs->d_qual = (type_qualifiers) { .tq_const = false };
	dcs->d_inline = false;
	dcs->d_multiple_storage_classes = false;
	dcs->d_invalid_type_combination = false;
	dcs->d_nonempty_decl = false;
	dcs->d_no_type_specifier = false;
	// keep d_asm
	dcs->d_packed = false;
	dcs->d_used = false;
	// keep d_tag_type
	dcs->d_func_params = NULL;
	dcs->d_func_def_pos = (pos_t){ NULL, 0, 0 };
	// keep d_first_dlsym
	// keep d_last_dlsym
	dcs->d_func_proto_syms = NULL;
	// keep d_enclosing
}

static void
dcs_adjust_storage_class(void)
{
	if (dcs->d_kind == DLK_EXTERN) {
		if (dcs->d_scl == REG || dcs->d_scl == AUTO) {
			/* illegal storage class */
			error(8);
			dcs->d_scl = NO_SCL;
		}
	} else if (dcs->d_kind == DLK_OLD_STYLE_PARAMS ||
	    dcs->d_kind == DLK_PROTO_PARAMS) {
		if (dcs->d_scl != NO_SCL && dcs->d_scl != REG) {
			/* only 'register' is valid as storage class ... */
			error(9);
			dcs->d_scl = NO_SCL;
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
	tspec_t t = dcs->d_abstract_type;
	tspec_t c = dcs->d_complex_mod;
	tspec_t s = dcs->d_sign_mod;
	tspec_t l = dcs->d_rank_mod;
	type_t *tp = dcs->d_type;

	if (tp != NULL) {
		lint_assert(t == NO_TSPEC);
		lint_assert(s == NO_TSPEC);
		lint_assert(l == NO_TSPEC);
		return;
	}

	if (t == NO_TSPEC && s == NO_TSPEC && l == NO_TSPEC && c == NO_TSPEC)
		dcs->d_no_type_specifier = true;
	if (t == NO_TSPEC && s == NO_TSPEC && (l == NO_TSPEC || l == LONG))
		t = c;

	if (t == NO_TSPEC)
		t = INT;
	if (s == NO_TSPEC && t == INT)
		s = SIGNED;
	if (l != NO_TSPEC && t == CHAR) {
		dcs->d_invalid_type_combination = true;
		l = NO_TSPEC;
	}
	if (l == LONG && t == FLOAT) {
		l = NO_TSPEC;
		t = DOUBLE;
		if (allow_c90)
			/* use 'double' instead of 'long float' */
			warning(6);
	}
	if ((l == LONG && t == DOUBLE) || t == LDOUBLE) {
		l = NO_TSPEC;
		t = LDOUBLE;
	}
	if (t == LDOUBLE && !allow_c90)
		/* 'long double' is illegal in traditional C */
		warning(266);
	if (l == LONG && t == DCOMPLEX) {
		l = NO_TSPEC;
		t = LCOMPLEX;
	}

	if (t != INT && t != CHAR && (s != NO_TSPEC || l != NO_TSPEC)) {
		dcs->d_invalid_type_combination = true;
		l = s = NO_TSPEC;
	}
	if (l != NO_TSPEC)
		t = l;
	dcs->d_type = gettyp(merge_signedness(t, s));
	debug_printf("%s: ", __func__);
	debug_dcs();
}

/* Create a type in 'dcs->d_type' from the information gathered in 'dcs'. */
void
dcs_end_type(void)
{

	dcs_merge_declaration_specifiers();

	if (dcs->d_multiple_storage_classes)
		/* only one storage class allowed */
		error(7);
	if (dcs->d_invalid_type_combination)
		/* illegal type combination */
		error(4);

	dcs_adjust_storage_class();

	if (dcs->d_qual.tq_const && dcs->d_type->t_const
	    && !dcs->d_type->t_typeof) {
		lint_assert(dcs->d_type->t_typedef);
		/* typedef already qualified with '%s' */
		warning(68, "const");
	}
	if (dcs->d_qual.tq_volatile && dcs->d_type->t_volatile &&
	    !dcs->d_type->t_typeof) {
		lint_assert(dcs->d_type->t_typedef);
		/* typedef already qualified with '%s' */
		warning(68, "volatile");
	}

	if (dcs->d_qual.tq_const || dcs->d_qual.tq_volatile) {
		dcs->d_type = block_dup_type(dcs->d_type);
		dcs->d_type->t_const |= dcs->d_qual.tq_const;
		dcs->d_type->t_volatile |= dcs->d_qual.tq_volatile;
	}

	debug_dcs();
	debug_leave();
}

/*
 * Return the length of a type in bits. For bit-fields, return the length of
 * the underlying storage type.
 *
 * Printing a message if the outermost dimension of an array is 0 must
 * be done by the caller. All other problems are reported by this function
 * if name is not NULL.
 */
int
length_in_bits(const type_t *tp, const char *name)
{

	if (tp == NULL)
		return -1;

	unsigned int elem = 1;
	while (tp->t_tspec == ARRAY) {
		elem *= tp->u.dimension;
		tp = tp->t_subt;
	}

	if (is_struct_or_union(tp->t_tspec)) {
		if (is_incomplete(tp) && name != NULL)
			/* '%s' has incomplete type '%s' */
			error(31, name, type_name(tp));
		return (int)(elem * tp->u.sou->sou_size_in_bits);
	}

	if (tp->t_tspec == ENUM && is_incomplete(tp) && name != NULL)
		/* incomplete enum type '%s' */
		warning(13, name);

	lint_assert(tp->t_tspec != FUNC);

	unsigned int elsz = size_in_bits(tp->t_tspec);
	/*
	 * Workaround until the type parser (see add_function, add_array,
	 * add_pointer) does not construct the invalid intermediate declaration
	 * 'void b[4]' for the legitimate declaration 'void *b[4]'.
	 */
	if (sytxerr > 0 && elsz == 0)
		elsz = CHAR_SIZE;
	lint_assert(elsz > 0);
	return (int)(elem * elsz);
}

unsigned int
alignment_in_bits(const type_t *tp)
{

	/* Super conservative so that it works for most systems. */
	unsigned int worst_align_in_bits = 2 * LONG_SIZE;

	while (tp->t_tspec == ARRAY)
		tp = tp->t_subt;

	tspec_t t = tp->t_tspec;
	unsigned int a;
	if (is_struct_or_union(t))
		a = tp->u.sou->sou_align_in_bits;
	else {
		lint_assert(t != FUNC);
		if ((a = size_in_bits(t)) == 0)
			a = CHAR_SIZE;
		else if (a > worst_align_in_bits)
			a = worst_align_in_bits;
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
concat_symbols(sym_t *l1, sym_t *l2)
{

	if (l1 == NULL)
		return l2;
	sym_t *l = l1;
	while (l->s_next != NULL)
		l = l->s_next;
	l->s_next = l2;
	return l1;
}

/*
 * Check if the type of the given symbol is valid.
 *
 * Invalid types are:
 * - arrays of incomplete types or functions
 * - functions returning arrays or functions
 * - void types other than type of function or pointer
 */
void
check_type(sym_t *sym)
{

	type_t **tpp = &sym->s_type;
	tspec_t to = NO_TSPEC;
	while (*tpp != NULL) {
		type_t *tp = *tpp;
		tspec_t t = tp->t_tspec;
		/*
		 * If this is the type of an old-style function definition, a
		 * better warning is printed in begin_function().
		 */
		if (t == FUNC && !tp->t_proto &&
		    !(to == NO_TSPEC && sym->s_osdef)) {
			/* TODO: Make this an error in C99 mode as well. */
			if (!allow_trad && !allow_c99 && hflag)
				/* function declaration is not a prototype */
				warning(287);
		}
		if (to == FUNC) {
			if (t == FUNC || t == ARRAY) {
				/* function returns illegal type '%s' */
				error(15, type_name(tp));
				*tpp = block_derive_type(
				    t == FUNC ? *tpp : (*tpp)->t_subt, PTR);
				return;
			}
			if (tp->t_const || tp->t_volatile) {
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
			}
			if (t == ARRAY && tp->u.dimension == 0) {
				/* null dimension */
				error(17);
				return;
			}
			if (t == VOID) {
				/* illegal use of 'void' */
				error(18);
				*tpp = gettyp(INT);
			}
			/*
			 * No need to check for incomplete types here as
			 * length_in_bits already does this.
			 */
		} else if (to == NO_TSPEC && t == VOID) {
			if (dcs->d_kind == DLK_PROTO_PARAMS) {
				if (sym->s_scl != ABSTRACT) {
					lint_assert(sym->s_name != unnamed);
					/* void parameter '%s' cannot ... */
					error(61, sym->s_name);
					*tpp = gettyp(INT);
				}
			} else if (dcs->d_kind == DLK_ABSTRACT) {
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
check_bit_field_type(sym_t *dsym, type_t **inout_tp, tspec_t *inout_t)
{
	type_t *tp = *inout_tp;
	tspec_t t = *inout_t;

	if (t == CHAR || t == UCHAR || t == SCHAR ||
	    t == SHORT || t == USHORT || t == ENUM) {
		if (!suppress_bitfieldtype) {
			/* TODO: Make this an error in C99 mode as well. */
			if (!allow_trad && !allow_c99) {
				type_t *btp = block_dup_type(tp);
				btp->t_bitfield = false;
				/* bit-field type '%s' invalid in C90 or ... */
				warning(273, type_name(btp));
			} else if (pflag) {
				type_t *btp = block_dup_type(tp);
				btp->t_bitfield = false;
				/* nonportable bit-field type '%s' */
				warning(34, type_name(btp));
			}
		}
	} else if (t == INT && dcs->d_sign_mod == NO_TSPEC) {
		if (pflag && !suppress_bitfieldtype) {
			/* bit-field of type plain 'int' has ... */
			warning(344);
		}
	} else if (!(t == INT || t == UINT || t == BOOL
		|| (is_integer(t) && (suppress_bitfieldtype || allow_gcc)))) {

		type_t *btp = block_dup_type(tp);
		btp->t_bitfield = false;
		/* illegal bit-field type '%s' */
		warning(35, type_name(btp));

		unsigned int width = tp->t_bit_field_width;
		dsym->s_type = tp = block_dup_type(gettyp(t = INT));
		if ((tp->t_bit_field_width = width) > size_in_bits(t))
			tp->t_bit_field_width = size_in_bits(t);
		*inout_t = t;
		*inout_tp = tp;
	}
}

static void
check_bit_field(sym_t *dsym, tspec_t *inout_t, type_t **inout_tp)
{

	check_bit_field_type(dsym, inout_tp, inout_t);

	type_t *tp = *inout_tp;
	tspec_t t = *inout_t;
	unsigned int t_width = size_in_bits(t);
	if (tp->t_bit_field_width > t_width) {
		/* illegal bit-field size: %d */
		error(36, (int)tp->t_bit_field_width);
		tp->t_bit_field_width = t_width;
	} else if (tp->t_bit_field_width == 0 && dsym->s_name != unnamed) {
		/* zero size bit-field */
		error(37);
		tp->t_bit_field_width = t_width;
	}
	if (dsym->s_scl == UNION_MEMBER) {
		/* bit-field in union is very unusual */
		warning(41);
		dsym->s_type->t_bitfield = false;
		dsym->s_bitfield = false;
	}
}

/* Aligns the next structure element as required. */
static void
dcs_align(unsigned int member_alignment, unsigned int bit_field_width)
{

	if (member_alignment > dcs->d_sou_align_in_bits)
		dcs->d_sou_align_in_bits = member_alignment;

	unsigned int offset = (dcs->d_sou_size_in_bits + member_alignment - 1)
	    & ~(member_alignment - 1);
	if (bit_field_width == 0
	    || dcs->d_sou_size_in_bits + bit_field_width > offset)
		dcs->d_sou_size_in_bits = offset;
}

/* Add a member to the struct or union type that is being built in 'dcs'. */
static void
dcs_add_member(sym_t *mem)
{
	type_t *tp = mem->s_type;

	unsigned int union_size = 0;
	if (dcs->d_kind == DLK_UNION) {
		union_size = dcs->d_sou_size_in_bits;
		dcs->d_sou_size_in_bits = 0;
	}

	if (mem->s_bitfield) {
		dcs_align(alignment_in_bits(tp), tp->t_bit_field_width);
		// XXX: Why round down?
		mem->u.s_member.sm_offset_in_bits = dcs->d_sou_size_in_bits
		    - dcs->d_sou_size_in_bits % size_in_bits(tp->t_tspec);
		tp->t_bit_field_offset = dcs->d_sou_size_in_bits
		    - mem->u.s_member.sm_offset_in_bits;
		dcs->d_sou_size_in_bits += tp->t_bit_field_width;
	} else {
		dcs_align(alignment_in_bits(tp), 0);
		mem->u.s_member.sm_offset_in_bits = dcs->d_sou_size_in_bits;
		dcs->d_sou_size_in_bits += type_size_in_bits(tp);
	}

	if (union_size > dcs->d_sou_size_in_bits)
		dcs->d_sou_size_in_bits = union_size;

	debug_dcs();
}

sym_t *
declare_unnamed_member(void)
{

	sym_t *mem = block_zero_alloc(sizeof(*mem), "sym");
	mem->s_name = unnamed;
	mem->s_kind = SK_MEMBER;
	mem->s_scl = dcs->d_kind == DLK_STRUCT ? STRUCT_MEMBER : UNION_MEMBER;
	mem->s_block_level = -1;
	mem->s_type = dcs->d_type;
	mem->u.s_member.sm_containing_type = dcs->d_tag_type->u.sou;

	dcs_add_member(mem);
	suppress_bitfieldtype = false;
	return mem;
}

sym_t *
declare_member(sym_t *dsym)
{

	lint_assert(is_member(dsym));

	sym_t *rdsym = dcs->d_redeclared_symbol;
	if (rdsym != NULL) {
		debug_sym("rdsym: ", rdsym, "\n");
		lint_assert(is_member(rdsym));

		if (dsym->u.s_member.sm_containing_type ==
		    rdsym->u.s_member.sm_containing_type) {
			/* duplicate member name '%s' */
			error(33, dsym->s_name);
			symtab_remove_forever(rdsym);
		}
	}

	check_type(dsym);

	type_t *tp = dsym->s_type;
	tspec_t t = tp->t_tspec;
	if (dsym->s_bitfield)
		check_bit_field(dsym, &t, &tp);
	else if (t == FUNC) {
		/* function illegal in structure or union */
		error(38);
		dsym->s_type = tp = block_derive_type(tp, t = PTR);
	}

	/*
	 * bit-fields of length 0 are not warned about because length_in_bits
	 * does not return the length of the bit-field but the length of the
	 * type the bit-field is packed in (it's ok)
	 */
	int sz = length_in_bits(dsym->s_type, dsym->s_name);
	if (sz == 0 && t == ARRAY && dsym->s_type->u.dimension == 0)
		/* zero-sized array '%s' in struct requires C99 or later */
		c99ism(39, dsym->s_name);

	dcs_add_member(dsym);

	check_function_definition(dsym, false);

	suppress_bitfieldtype = false;

	return dsym;
}

sym_t *
set_bit_field_width(sym_t *dsym, int bit_field_width)
{

	if (dsym == NULL) {
		dsym = block_zero_alloc(sizeof(*dsym), "sym");
		dsym->s_name = unnamed;
		dsym->s_kind = SK_MEMBER;
		dsym->s_scl = STRUCT_MEMBER;
		dsym->s_type = gettyp(UINT);
		dsym->s_block_level = -1;
		lint_assert(dcs->d_tag_type->u.sou != NULL);
		dsym->u.s_member.sm_containing_type = dcs->d_tag_type->u.sou;
	}
	dsym->s_type = block_dup_type(dsym->s_type);
	dsym->s_type->t_bitfield = true;
	dsym->s_type->t_bit_field_width = bit_field_width;
	dsym->s_bitfield = true;
	debug_sym("set_bit_field_width: ", dsym, "\n");
	return dsym;
}

void
add_type_qualifiers(type_qualifiers *dst, type_qualifiers src)
{

	if (src.tq_const && dst->tq_const)
		/* duplicate '%s' */
		warning(10, "const");
	if (src.tq_volatile && dst->tq_volatile)
		/* duplicate '%s' */
		warning(10, "volatile");

	dst->tq_const = dst->tq_const | src.tq_const;
	dst->tq_restrict = dst->tq_restrict | src.tq_restrict;
	dst->tq_volatile = dst->tq_volatile | src.tq_volatile;
	dst->tq_atomic = dst->tq_atomic | src.tq_atomic;
	debug_step("%s: '%s'", __func__, type_qualifiers_string(*dst));
}

qual_ptr *
append_qualified_pointer(qual_ptr *p1, qual_ptr *p2)
{

	qual_ptr *tail = p2;
	while (tail->p_next != NULL)
		tail = tail->p_next;
	tail->p_next = p1;
	return p2;
}

static type_t *
block_derive_pointer(type_t *stp, bool is_const, bool is_volatile)
{

	type_t *tp = block_derive_type(stp, PTR);
	tp->t_const = is_const;
	tp->t_volatile = is_volatile;
	debug_step("%s: '%s'", __func__, type_name(tp));
	return tp;
}

sym_t *
add_pointer(sym_t *decl, qual_ptr *p)
{

	debug_dcs();

	type_t **tpp = &decl->s_type;
	lint_assert(*tpp != NULL);
	while (*tpp != dcs->d_type) {
		tpp = &(*tpp)->t_subt;
		lint_assert(*tpp != NULL);
	}

	while (p != NULL) {
		*tpp = block_derive_pointer(dcs->d_type,
		    p->qualifiers.tq_const, p->qualifiers.tq_volatile);

		tpp = &(*tpp)->t_subt;

		qual_ptr *next = p->p_next;
		free(p);
		p = next;
	}
	debug_step("add_pointer: '%s'", type_name(decl->s_type));
	return decl;
}

static type_t *
block_derive_array(type_t *stp, bool has_dim, int dim)
{

	type_t *tp = block_derive_type(stp, ARRAY);
	tp->u.dimension = dim;

#if 0
	/*
	 * When the type of the declaration 'void *b[4]' is constructed (see
	 * add_function, add_array, add_pointer), the intermediate type
	 * 'void[4]' is invalid and only later gets the '*' inserted in the
	 * middle of the type.  Due to the invalid intermediate type, this
	 * check cannot be enabled yet.
	 */
	if (stp->t_tspec == VOID) {
		/* array of incomplete type */
		error(301);
		tp->t_subt = gettyp(CHAR);
	}
#endif
	if (dim < 0)
		/* negative array dimension (%d) */
		error(20, dim);
	else if (dim == 0 && has_dim)
		/* zero sized array requires C99 or later */
		c99ism(322);
	else if (dim == 0 && !has_dim)
		tp->t_incomplete_array = true;

	debug_step("%s: '%s'", __func__, type_name(tp));
	return tp;
}

sym_t *
add_array(sym_t *decl, bool has_dim, int dim)
{

	debug_dcs();

	type_t **tpp = &decl->s_type;
	lint_assert(*tpp != NULL);
	while (*tpp != dcs->d_type) {
		tpp = &(*tpp)->t_subt;
		lint_assert(*tpp != NULL);
	}

	*tpp = block_derive_array(dcs->d_type, has_dim, dim);

	debug_step("%s: '%s'", __func__, type_name(decl->s_type));
	return decl;
}

static type_t *
block_derive_function(type_t *ret, bool proto, sym_t *params, bool vararg)
{

	type_t *tp = block_derive_type(ret, FUNC);
	tp->t_proto = proto;
	if (proto)
		tp->u.params = params;
	tp->t_vararg = vararg;
	debug_step("%s: '%s'", __func__, type_name(tp));
	return tp;
}

static const char *
tag_name(scl_t sc)
{
	return sc == STRUCT_TAG ? "struct"
	    : sc == UNION_TAG ? "union"
	    : "enum";
}

static void
check_prototype_parameters(sym_t *args)
{

	for (sym_t *sym = dcs->d_first_dlsym;
	    sym != NULL; sym = sym->s_level_next) {
		scl_t sc = sym->s_scl;
		if (sc == STRUCT_TAG || sc == UNION_TAG || sc == ENUM_TAG)
			/* dubious tag declaration '%s %s' */
			warning(85, tag_name(sc), sym->s_name);
	}

	for (sym_t *arg = args; arg != NULL; arg = arg->s_next) {
		if (arg->s_type->t_tspec == VOID &&
		    !(arg == args && arg->s_next == NULL)) {
			/* void must be sole parameter */
			error(60);
			arg->s_type = gettyp(INT);
		}
	}
}

static void
old_style_function(sym_t *decl, sym_t *params)
{

	/*
	 * Remember the list of parameters only if this really seems to be a
	 * function definition.
	 */
	if (dcs->d_enclosing->d_kind == DLK_EXTERN &&
	    decl->s_type == dcs->d_enclosing->d_type) {
		/*
		 * Assume that this becomes a function definition. If not, it
		 * will be corrected in check_function_definition.
		 */
		if (params != NULL) {
			decl->s_osdef = true;
			decl->u.s_old_style_params = params;
		}
	} else {
		if (params != NULL)
			/* function prototype parameters must have types */
			warning(62);
	}
}

sym_t *
add_function(sym_t *decl, parameter_list params)
{

	debug_enter();
	debug_dcs_all();
	debug_sym("decl: ", decl, "\n");
#ifdef DEBUG
	for (const sym_t *p = params.first; p != NULL; p = p->s_next)
		debug_sym("param: ", p, "\n");
#endif

	if (params.prototype) {
		if (!allow_c90)
			/* function prototypes are illegal in traditional C */
			warning(270);
		check_prototype_parameters(params.first);
		if (params.first != NULL
		    && params.first->s_type->t_tspec == VOID)
			params.first = NULL;
	} else
		old_style_function(decl, params.first);

	/*
	 * The symbols are removed from the symbol table by
	 * end_declaration_level after add_function. To be able to restore them
	 * if this is a function definition, a pointer to the list of all
	 * symbols is stored in dcs->d_enclosing->d_func_proto_syms. Also, a
	 * list of the parameters (concatenated by s_next) is stored in
	 * dcs->d_enclosing->d_func_params. (dcs->d_enclosing must be used
	 * because *dcs is the declaration stack element created for the list
	 * of params and is removed after add_function.)
	 */
	if (dcs->d_enclosing->d_kind == DLK_EXTERN &&
	    decl->s_type == dcs->d_enclosing->d_type) {
		dcs->d_enclosing->d_func_proto_syms = dcs->d_first_dlsym;
		dcs->d_enclosing->d_func_params = params.first;
		debug_dcs_all();
	}

	type_t **tpp = &decl->s_type;
	lint_assert(*tpp != NULL);
	while (*tpp != dcs->d_enclosing->d_type) {
		tpp = &(*tpp)->t_subt;
		lint_assert(*tpp != NULL);
	}

	*tpp = block_derive_function(dcs->d_enclosing->d_type,
	    params.prototype, params.first, params.vararg);

	debug_step("add_function: '%s'", type_name(decl->s_type));
	debug_dcs_all();
	debug_leave();
	return decl;
}

/*
 * In a function declaration, a list of identifiers (as opposed to a list of
 * types) is allowed only if it's also a function definition.
 */
void
check_function_definition(sym_t *sym, bool msg)
{

	if (sym->s_osdef) {
		if (msg)
			/* incomplete or misplaced function definition */
			error(22);
		sym->s_osdef = false;
		sym->u.s_old_style_params = NULL;
	}
}

/* The symbol gets a storage class and a definedness. */
sym_t *
declarator_name(sym_t *sym)
{
	scl_t sc = NO_SCL;

	if (sym->s_scl == NO_SCL)
		dcs->d_redeclared_symbol = NULL;
	else if (sym->s_defparam) {
		sym->s_defparam = false;
		dcs->d_redeclared_symbol = NULL;
	} else {
		dcs->d_redeclared_symbol = sym;
		if (is_query_enabled[16]
		    && sym->s_scl == STATIC && dcs->d_scl != STATIC) {
			/* '%s' was declared 'static', now non-'static' */
			query_message(16, sym->s_name);
			print_previous_declaration(sym);
		}
		sym = pushdown(sym);
	}

	switch (dcs->d_kind) {
	case DLK_STRUCT:
	case DLK_UNION:
		sym->u.s_member.sm_containing_type = dcs->d_tag_type->u.sou;
		sym->s_def = DEF;
		sc = dcs->d_kind == DLK_STRUCT ? STRUCT_MEMBER : UNION_MEMBER;
		break;
	case DLK_EXTERN:
		/*
		 * Symbols that are 'static' or without any storage class are
		 * tentatively defined. Symbols that are tentatively defined or
		 * declared may later become defined if an initializer is seen
		 * or this is a function definition.
		 */
		sc = dcs->d_scl;
		if (sc == NO_SCL || sc == THREAD_LOCAL) {
			sc = EXTERN;
			sym->s_def = TDEF;
		} else if (sc == STATIC)
			sym->s_def = TDEF;
		else if (sc == TYPEDEF)
			sym->s_def = DEF;
		else {
			lint_assert(sc == EXTERN);
			sym->s_def = DECL;
		}
		break;
	case DLK_PROTO_PARAMS:
		sym->s_param = true;
		/* FALLTHROUGH */
	case DLK_OLD_STYLE_PARAMS:
		lint_assert(dcs->d_scl == NO_SCL || dcs->d_scl == REG);
		sym->s_register = dcs->d_scl == REG;
		sc = AUTO;
		sym->s_def = DEF;
		break;
	case DLK_AUTO:
		if ((sc = dcs->d_scl) == NO_SCL) {
			/*
			 * XXX somewhat ugly because we don't know whether this
			 * is AUTO or EXTERN (functions). If we are wrong, it
			 * must be corrected in declare_local, when the
			 * necessary type information is available.
			 */
			sc = AUTO;
			sym->s_def = DEF;
		} else if (sc == AUTO || sc == STATIC || sc == TYPEDEF
		    || sc == THREAD_LOCAL)
			sym->s_def = DEF;
		else if (sc == REG) {
			sym->s_register = true;
			sc = AUTO;
			sym->s_def = DEF;
		} else {
			lint_assert(sc == EXTERN);
			sym->s_def = DECL;
		}
		break;
	default:
		lint_assert(dcs->d_kind == DLK_ABSTRACT);
		/* try to continue after syntax errors */
		sc = NO_SCL;
	}
	sym->s_scl = sc;

	sym->s_type = dcs->d_type;

	dcs->d_func_proto_syms = NULL;

	debug_sym("declarator_name: ", sym, "\n");
	return sym;
}

sym_t *
old_style_function_parameter_name(sym_t *sym)
{

	if (sym->s_scl != NO_SCL) {
		if (block_level == sym->s_block_level) {
			/* redeclaration of formal parameter '%s' */
			error(21, sym->s_name);
			lint_assert(sym->s_defparam);
		}
		sym = pushdown(sym);
	}
	sym->s_type = gettyp(INT);
	sym->s_scl = AUTO;
	sym->s_def = DEF;
	sym->s_defparam = true;
	sym->s_param = true;
	debug_sym("old_style_function_parameter_name: ", sym, "\n");
	return sym;
}

/*-
 * Checks all possible cases of tag redeclarations.
 *
 * decl		whether T_LBRACE follows
 * semi		whether T_SEMI follows
 */
static sym_t *
new_tag(sym_t *tag, scl_t scl, bool decl, bool semi)
{

	if (tag->s_block_level < block_level) {
		if (semi) {
			/* "struct a;" */
			if (allow_c90) {
				/* XXX: Why is this warning suppressed in C90 mode? */
				if (allow_trad || allow_c99)
					/* declaration of '%s %s' intro... */
					warning(44, tag_name(scl),
					    tag->s_name);
				tag = pushdown(tag);
			} else if (tag->s_scl != scl)
				/* base type is really '%s %s' */
				warning(45, tag_name(tag->s_scl), tag->s_name);
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (decl) {
			/* "struct a { ... } " */
			if (hflag)
				/* redefinition of '%s' hides earlier one */
				warning(43, tag->s_name);
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (tag->s_scl != scl) {
			/* base type is really '%s %s' */
			warning(45, tag_name(tag->s_scl), tag->s_name);
			/* XXX: Why is this warning suppressed in C90 mode? */
			if (allow_trad || allow_c99)
				/* declaration of '%s %s' introduces ... */
				warning(44, tag_name(scl), tag->s_name);
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		}
	} else {
		if (tag->s_scl != scl ||
		    (decl && !is_incomplete(tag->s_type))) {
			/* %s tag '%s' redeclared as %s */
			error(46, tag_name(tag->s_scl),
			    tag->s_name, tag_name(scl));
			print_previous_declaration(tag);
			tag = pushdown(tag);
			dcs->d_enclosing->d_nonempty_decl = true;
		} else if (semi || decl)
			dcs->d_enclosing->d_nonempty_decl = true;
	}
	debug_sym("new_tag: ", tag, "\n");
	return tag;
}

/*-
 * tag		the symbol table entry of the tag
 * kind		the kind of the tag (STRUCT/UNION/ENUM)
 * decl		whether the tag type will be completed in this declaration
 *		(when the following token is T_LBRACE)
 * semi		whether the following token is T_SEMI
 */
type_t *
make_tag_type(sym_t *tag, tspec_t kind, bool decl, bool semi)
{
	scl_t scl;
	type_t *tp;

	if (kind == STRUCT)
		scl = STRUCT_TAG;
	else if (kind == UNION)
		scl = UNION_TAG;
	else {
		lint_assert(kind == ENUM);
		scl = ENUM_TAG;
	}

	if (tag != NULL) {
		if (tag->s_scl != NO_SCL)
			tag = new_tag(tag, scl, decl, semi);
		else {
			/* a new tag, no empty declaration */
			dcs->d_enclosing->d_nonempty_decl = true;
			if (scl == ENUM_TAG && !decl) {
				/* TODO: Make this an error in C99 mode as well. */
				if (allow_c90 &&
				    ((!allow_trad && !allow_c99) || pflag))
					/* forward reference to enum type */
					warning(42);
			}
		}
		if (tag->s_scl == NO_SCL) {
			tag->s_scl = scl;
			tag->s_type = tp =
			    block_zero_alloc(sizeof(*tp), "type");
			tp->t_packed = dcs->d_packed;
		} else
			tp = tag->s_type;

	} else {
		tag = block_zero_alloc(sizeof(*tag), "sym");
		tag->s_name = unnamed;
		tag->s_def_pos = unique_curr_pos();
		tag->s_kind = SK_TAG;
		tag->s_scl = scl;
		tag->s_block_level = -1;
		tag->s_type = tp = block_zero_alloc(sizeof(*tp), "type");
		tp->t_packed = dcs->d_packed;
		dcs->d_enclosing->d_nonempty_decl = true;
	}

	if (tp->t_tspec == NO_TSPEC) {
		tp->t_tspec = kind;
		if (kind != ENUM) {
			tp->u.sou = block_zero_alloc(sizeof(*tp->u.sou),
			    "struct_or_union");
			tp->u.sou->sou_align_in_bits = CHAR_SIZE;
			tp->u.sou->sou_tag = tag;
			tp->u.sou->sou_incomplete = true;
		} else {
			tp->t_is_enum = true;
			tp->u.enumer = block_zero_alloc(
			    sizeof(*tp->u.enumer), "enumeration");
			tp->u.enumer->en_tag = tag;
			tp->u.enumer->en_incomplete = true;
		}
	}
	debug_printf("%s: '%s'", __func__, type_name(tp));
	debug_sym(" ", tag, "\n");
	return tp;
}

static bool
has_named_member(const type_t *tp)
{
	for (const sym_t *mem = tp->u.sou->sou_first_member;
	    mem != NULL; mem = mem->s_next) {
		if (mem->s_name != unnamed)
			return true;
		if (is_struct_or_union(mem->s_type->t_tspec)
		    && has_named_member(mem->s_type))
			return true;
	}
	return false;
}

type_t *
complete_struct_or_union(sym_t *first_member)
{

	type_t *tp = dcs->d_tag_type;
	if (tp == NULL)		/* in case of syntax errors */
		return gettyp(INT);

	dcs_align(dcs->d_sou_align_in_bits, 0);

	struct_or_union *sou = tp->u.sou;
	sou->sou_align_in_bits = dcs->d_sou_align_in_bits;
	sou->sou_incomplete = false;
	sou->sou_first_member = first_member;
	if (tp->t_packed)
		pack_struct_or_union(tp);
	else
		sou->sou_size_in_bits = dcs->d_sou_size_in_bits;

	if (sou->sou_size_in_bits == 0)
		/* zero sized %s is a C99 feature */
		c99ism(47, tspec_name(tp->t_tspec));
	else if (!has_named_member(tp))
		/* '%s' has no named members */
		warning(65, type_name(tp));
	debug_step("%s: '%s'", __func__, type_name(tp));
	return tp;
}

type_t *
complete_enum(sym_t *first_enumerator)
{

	type_t *tp = dcs->d_tag_type;
	tp->u.enumer->en_incomplete = false;
	tp->u.enumer->en_first_enumerator = first_enumerator;
	debug_step("%s: '%s'", __func__, type_name(tp));
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

	if (sym->s_scl != NO_SCL) {
		if (sym->s_block_level == block_level) {
			/* no hflag, because this is illegal */
			if (sym->s_param)
				/* enumeration constant '%s' hides parameter */
				warning(57, sym->s_name);
			else {
				/* redeclaration of '%s' */
				error(27, sym->s_name);
				/*
				 * Inside blocks, it should not be too
				 * complicated to find the position of the
				 * previous declaration
				 */
				if (block_level == 0)
					print_previous_declaration(sym);
			}
		} else {
			if (hflag)
				/* redefinition of '%s' hides earlier one */
				warning(43, sym->s_name);
		}
		sym = pushdown(sym);
	}

	sym->s_scl = ENUM_CONST;
	sym->s_type = dcs->d_tag_type;
	sym->u.s_enum_constant = val;

	if (impl && val == TARG_INT_MIN)
		/* enumeration value '%s' overflows */
		warning(48, sym->s_name);

	enumval = val == TARG_INT_MAX ? TARG_INT_MIN : val + 1;
	debug_sym("enumeration_constant: ", sym, "\n");
	return sym;
}

static bool
ends_with(const char *s, const char *suffix)
{
	size_t s_len = strlen(s);
	size_t suffix_len = strlen(suffix);
	return s_len >= suffix_len &&
	    memcmp(s + s_len - suffix_len, suffix, suffix_len) == 0;
}

void
check_extern_declaration(const sym_t *sym)
{

	if (sym->s_scl == EXTERN &&
	    dcs->d_redeclared_symbol == NULL &&
	    ends_with(curr_pos.p_file, ".c") &&
	    allow_c90 &&
	    !isdigit((unsigned char)sym->s_name[0]) &&	/* see mktempsym */
	    strcmp(sym->s_name, "main") != 0) {
		/* missing%s header declaration for '%s' */
		warning(351, sym->s_type->t_tspec == FUNC ? "" : " 'extern'",
		    sym->s_name);
	}
	if (any_query_enabled &&
	    sym->s_type->t_tspec == FUNC &&
	    sym->s_scl == EXTERN &&
	    sym->s_def == DECL &&
	    !in_system_header)
		/* redundant 'extern' in function declaration of '%s' */
		query_message(13, sym->s_name);
}

/*
 * Check whether the symbol cannot be initialized due to type/storage class.
 * Return whether an error has been detected.
 */
static bool
check_init(sym_t *sym)
{

	if (sym->s_type->t_tspec == FUNC) {
		/* cannot initialize function '%s' */
		error(24, sym->s_name);
		return true;
	}
	if (sym->s_scl == TYPEDEF) {
		/* cannot initialize typedef '%s' */
		error(25, sym->s_name);
		return true;
	}
	if (sym->s_scl == EXTERN && sym->s_def == DECL) {
		if (dcs->d_kind == DLK_EXTERN)
			/* cannot initialize extern declaration '%s' */
			warning(26, sym->s_name);
		else {
			/* cannot initialize extern declaration '%s' */
			error(26, sym->s_name);
			return true;
		}
	}

	return false;
}

/*
 * Compares a prototype declaration with the remembered parameters of a
 * previous old-style function definition.
 */
static bool
check_old_style_definition(const sym_t *rdsym, const sym_t *dsym)
{

	const sym_t *old_params = rdsym->u.s_old_style_params;
	const sym_t *proto_params = dsym->s_type->u.params;

	bool msg = false;

	int old_n = 0;
	for (const sym_t *p = old_params; p != NULL; p = p->s_next)
		old_n++;
	int proto_n = 0;
	for (const sym_t *p = proto_params; p != NULL; p = p->s_next)
		proto_n++;
	if (old_n != proto_n) {
		/* prototype does not match old-style definition */
		error(63);
		msg = true;
		goto end;
	}

	const sym_t *arg = old_params;
	const sym_t *parg = proto_params;
	int n = 1;
	while (old_n-- > 0) {
		bool dowarn = false;
		if (!types_compatible(arg->s_type, parg->s_type,
		    true, true, &dowarn) ||
		    dowarn) {
			/* prototype does not match old-style ... */
			error(299, n);
			msg = true;
		}
		arg = arg->s_next;
		parg = parg->s_next;
		n++;
	}

end:
	if (msg && rflag)
		/* old-style definition */
		message_at(300, &rdsym->s_def_pos);
	return msg;
}

/* Process a single external or 'static' declarator. */
static void
declare_extern(sym_t *dsym, bool has_initializer, sbuf_t *renaming)
{

	if (renaming != NULL) {
		lint_assert(dsym->s_rename == NULL);

		char *s = level_zero_alloc(1, renaming->sb_len + 1, "string");
		(void)memcpy(s, renaming->sb_name, renaming->sb_len + 1);
		dsym->s_rename = s;
	}

	check_extern_declaration(dsym);

	check_function_definition(dsym, true);

	check_type(dsym);

	if (has_initializer && !check_init(dsym))
		dsym->s_def = DEF;

	/*
	 * Declarations of functions are marked as "tentative" in
	 * declarator_name(). This is wrong because there are no tentative
	 * function definitions.
	 */
	if (dsym->s_type->t_tspec == FUNC && dsym->s_def == TDEF)
		dsym->s_def = DECL;

	if (dcs->d_inline) {
		if (dsym->s_type->t_tspec == FUNC) {
			dsym->s_inline = true;
		} else {
			/* variable '%s' declared inline */
			warning(268, dsym->s_name);
		}
	}

	/* Write the declaration into the output file */
	if (plibflg && llibflg &&
	    dsym->s_type->t_tspec == FUNC && dsym->s_type->t_proto) {
		/*
		 * With both LINTLIBRARY and PROTOLIB the prototype is written
		 * as a function definition to the output file.
		 */
		bool rval = dsym->s_type->t_subt->t_tspec != VOID;
		outfdef(dsym, &dsym->s_def_pos, rval, false, NULL);
	} else if (!is_compiler_builtin(dsym->s_name)
	    && !(has_initializer && dsym->s_type->t_incomplete_array)) {
		outsym(dsym, dsym->s_scl, dsym->s_def);
	}

	sym_t *rdsym = dcs->d_redeclared_symbol;
	if (rdsym != NULL) {

		/*
		 * If the old symbol stems from an old-style function
		 * definition, we have remembered the params in
		 * rdsym->s_old_style_params and compare them with the params
		 * of the prototype.
		 */
		bool redec = rdsym->s_osdef && dsym->s_type->t_proto &&
		    check_old_style_definition(rdsym, dsym);

		bool dowarn = false;
		if (!redec && !check_redeclaration(dsym, &dowarn)) {
			if (dowarn) {
				/* TODO: Make this an error in C99 mode as well. */
				if (!allow_trad && !allow_c99)
					/* redeclaration of '%s' */
					error(27, dsym->s_name);
				else
					/* redeclaration of '%s' */
					warning(27, dsym->s_name);
				print_previous_declaration(rdsym);
			}

			/*
			 * Take over the remembered params if the new symbol is
			 * not a prototype.
			 */
			if (rdsym->s_osdef && !dsym->s_type->t_proto) {
				dsym->s_osdef = rdsym->s_osdef;
				dsym->u.s_old_style_params =
				    rdsym->u.s_old_style_params;
				dsym->s_def_pos = rdsym->s_def_pos;
			}

			if (rdsym->s_type->t_proto && !dsym->s_type->t_proto)
				dsym->s_def_pos = rdsym->s_def_pos;
			else if (rdsym->s_def == DEF && dsym->s_def != DEF)
				dsym->s_def_pos = rdsym->s_def_pos;

			copy_usage_info(dsym, rdsym);

			/* Once a name is defined, it remains defined. */
			if (rdsym->s_def == DEF)
				dsym->s_def = DEF;

			/* once a function is inline, it remains inline */
			if (rdsym->s_inline)
				dsym->s_inline = true;

			complete_type(dsym, rdsym);
		}

		symtab_remove_forever(rdsym);
	}

	if (dsym->s_scl == TYPEDEF) {
		dsym->s_type = block_dup_type(dsym->s_type);
		dsym->s_type->t_typedef = true;
		set_first_typedef(dsym->s_type, dsym);
	}
	debug_printf("%s: ", __func__);
	debug_sym("", dsym, "\n");
}

void
declare(sym_t *decl, bool has_initializer, sbuf_t *renaming)
{

	if (dcs->d_kind == DLK_EXTERN)
		declare_extern(decl, has_initializer, renaming);
	else if (dcs->d_kind == DLK_OLD_STYLE_PARAMS ||
	    dcs->d_kind == DLK_PROTO_PARAMS) {
		if (renaming != NULL)
			/* symbol renaming can't be used on function ... */
			error(310);
		else
			(void)declare_parameter(decl, has_initializer);
	} else {
		lint_assert(dcs->d_kind == DLK_AUTO);
		if (renaming != NULL)
			/* symbol renaming can't be used on automatic ... */
			error(311);
		else
			declare_local(decl, has_initializer);
	}
	debug_printf("%s: ", __func__);
	debug_sym("", decl, "\n");
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
 * Otherwise, returns false and, in some cases of minor problems, prints
 * a warning.
 */
bool
check_redeclaration(sym_t *dsym, bool *dowarn)
{

	sym_t *rdsym = dcs->d_redeclared_symbol;
	if (rdsym->s_scl == ENUM_CONST) {
		/* redeclaration of '%s' */
		error(27, dsym->s_name);
		print_previous_declaration(rdsym);
		return true;
	}
	if (rdsym->s_scl == TYPEDEF) {
		/* typedef '%s' redeclared */
		error(89, dsym->s_name);
		print_previous_declaration(rdsym);
		return true;
	}
	if (dsym->s_scl == TYPEDEF) {
		/* redeclaration of '%s' */
		error(27, dsym->s_name);
		print_previous_declaration(rdsym);
		return true;
	}
	if (rdsym->s_def == DEF && dsym->s_def == DEF) {
		/* redefinition of '%s' */
		error(28, dsym->s_name);
		print_previous_declaration(rdsym);
		return true;
	}
	if (!types_compatible(rdsym->s_type, dsym->s_type,
	    false, false, dowarn)) {
		/* redeclaration of '%s' with type '%s', expected '%s' */
		error(347, dsym->s_name,
		    type_name(dsym->s_type), type_name(rdsym->s_type));
		print_previous_declaration(rdsym);
		return true;
	}
	if (rdsym->s_scl == EXTERN && dsym->s_scl == EXTERN)
		return false;
	if (rdsym->s_scl == STATIC && dsym->s_scl == STATIC)
		return false;
	if (rdsym->s_scl == STATIC && dsym->s_def == DECL)
		return false;
	if (rdsym->s_scl == EXTERN && rdsym->s_def == DEF) {
		/*
		 * All cases except "int a = 1; static int a;" are caught above
		 * with or without a warning
		 */
		/* redeclaration of '%s' */
		error(27, dsym->s_name);
		print_previous_declaration(rdsym);
		return true;
	}
	if (rdsym->s_scl == EXTERN) {
		/* '%s' was previously declared extern, becomes static */
		warning(29, dsym->s_name);
		print_previous_declaration(rdsym);
		return false;
	}
	/*-
	 * Now it's one of:
	 * "static a; int a;"
	 * "static a; int a = 1;"
	 * "static a = 1; int a;"
	 */
	/* TODO: Make this an error in C99 mode as well. */
	if (!allow_trad && !allow_c99) {
		/* redeclaration of '%s'; C90 or later require static */
		warning(30, dsym->s_name);
		print_previous_declaration(rdsym);
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
pointer_types_are_compatible(const type_t *tp1, const type_t *tp2, bool ignqual)
{

	return tp1->t_tspec == VOID || tp2->t_tspec == VOID ||
	    qualifiers_correspond(tp1, tp2, ignqual);
}

static bool
prototypes_compatible(const type_t *tp1, const type_t *tp2, bool *dowarn)
{

	if (tp1->t_vararg != tp2->t_vararg)
		return false;

	const sym_t *p1 = tp1->u.params;
	const sym_t *p2 = tp2->u.params;

	for (; p1 != NULL && p2 != NULL; p1 = p1->s_next, p2 = p2->s_next) {
		if (!types_compatible(p1->s_type, p2->s_type,
		    true, false, dowarn))
			return false;
	}
	return p1 == p2;
}

/*
 * Returns whether all parameters of a prototype are compatible with an
 * old-style function declaration.
 *
 * This is the case if the following conditions are met:
 *	1. the prototype has a fixed number of parameters
 *	2. no parameter is of type float
 *	3. no parameter is converted to another type if integer promotion
 *	   is applied on it
 */
static bool
matches_no_arg_function(const type_t *tp, bool *dowarn)
{

	if (tp->t_vararg && dowarn != NULL)
		*dowarn = true;
	for (const sym_t *p = tp->u.params; p != NULL; p = p->s_next) {
		tspec_t t = p->s_type->t_tspec;
		if (t == FLOAT ||
		    t == CHAR || t == SCHAR || t == UCHAR ||
		    t == SHORT || t == USHORT) {
			if (dowarn != NULL)
				*dowarn = true;
		}
	}
	/* FIXME: Always returning true cannot be correct. */
	return true;
}

/*-
 * ignqual	ignore type qualifiers; used for function parameters
 * promot	promote the left type; used for comparison of parameters of
 *		old-style function definitions with parameters of prototypes.
 * *dowarn	is set to true if an old-style function declaration is not
 *		compatible with a prototype
 */
bool
types_compatible(const type_t *tp1, const type_t *tp2,
		     bool ignqual, bool promot, bool *dowarn)
{

	while (tp1 != NULL && tp2 != NULL) {
		tspec_t t = tp1->t_tspec;
		if (promot) {
			if (t == FLOAT)
				t = DOUBLE;
			else if (t == CHAR || t == SCHAR)
				t = INT;
			else if (t == UCHAR)
				t = allow_c90 ? INT : UINT;
			else if (t == SHORT)
				t = INT;
			else if (t == USHORT) {
				/* CONSTCOND */
				t = TARG_INT_MAX < TARG_USHRT_MAX || !allow_c90
				    ? UINT : INT;
			}
		}

		if (t != tp2->t_tspec)
			return false;

		if (!qualifiers_correspond(tp1, tp2, ignqual))
			return false;

		if (is_struct_or_union(t))
			return tp1->u.sou == tp2->u.sou;

		if (t == ENUM && eflag)
			return tp1->u.enumer == tp2->u.enumer;

		if (t == ARRAY && tp1->u.dimension != tp2->u.dimension) {
			if (tp1->u.dimension != 0 && tp2->u.dimension != 0)
				return false;
		}

		/* don't check prototypes for traditional */
		if (t == FUNC && allow_c90) {
			if (tp1->t_proto && tp2->t_proto) {
				if (!prototypes_compatible(tp1, tp2, dowarn))
					return false;
			} else if (tp1->t_proto) {
				if (!matches_no_arg_function(tp1, dowarn))
					return false;
			} else if (tp2->t_proto) {
				if (!matches_no_arg_function(tp2, dowarn))
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
 * Completes a type by copying the dimension and prototype information from a
 * second compatible type.
 *
 * Following lines are legal:
 *  "typedef a[]; a b; a b[10]; a c; a c[20];"
 *  "typedef ft(); ft f; f(int); ft g; g(long);"
 * This means that, if a type is completed, the type structure must be
 * duplicated.
 */
void
complete_type(sym_t *dsym, sym_t *ssym)
{
	type_t **dstp = &dsym->s_type;
	type_t *src = ssym->s_type;

	while (*dstp != NULL) {
		type_t *dst = *dstp;
		lint_assert(src != NULL);
		lint_assert(dst->t_tspec == src->t_tspec);
		if (dst->t_tspec == ARRAY) {
			if (dst->u.dimension == 0 && src->u.dimension != 0) {
				*dstp = dst = block_dup_type(dst);
				dst->u.dimension = src->u.dimension;
				dst->t_incomplete_array = false;
			}
		} else if (dst->t_tspec == FUNC) {
			if (!dst->t_proto && src->t_proto) {
				*dstp = dst = block_dup_type(dst);
				dst->t_proto = true;
				dst->u.params = src->u.params;
			}
		}
		dstp = &dst->t_subt;
		src = src->t_subt;
	}
	debug_printf("%s: ", __func__);
	debug_sym("dsym: ", dsym, "");
	debug_sym("ssym: ", ssym, "\n");
}

sym_t *
declare_parameter(sym_t *sym, bool has_initializer)
{

	check_function_definition(sym, true);

	check_type(sym);

	if (dcs->d_redeclared_symbol != NULL &&
	    dcs->d_redeclared_symbol->s_block_level == block_level) {
		/* redeclaration of formal parameter '%s' */
		error(237, sym->s_name);
		symtab_remove_forever(dcs->d_redeclared_symbol);
		sym->s_param = true;
	}

	if (!sym->s_param) {
		/* declared parameter '%s' is missing */
		error(53, sym->s_name);
		sym->s_param = true;
	}

	if (has_initializer)
		/* cannot initialize parameter '%s' */
		error(52, sym->s_name);

	if (sym->s_type == NULL)	/* for c(void()) */
		sym->s_type = gettyp(VOID);

	tspec_t t = sym->s_type->t_tspec;
	if (t == ARRAY)
		sym->s_type = block_derive_type(sym->s_type->t_subt, PTR);
	if (t == FUNC) {
		if (!allow_c90)
			/* parameter '%s' has function type, should be ... */
			warning(50, sym->s_name);
		sym->s_type = block_derive_type(sym->s_type, PTR);
	}
	if (t == FLOAT && !allow_c90)
		sym->s_type = gettyp(DOUBLE);

	if (dcs->d_inline)
		/* parameter '%s' declared inline */
		warning(269, sym->s_name);

	if (any_query_enabled && sym->s_type->t_const
	    && (sym->s_scl == AUTO || sym->s_scl == REG)) {
		/* const automatic variable '%s' */
		query_message(18, sym->s_name);
	}

	/*
	 * Arguments must have complete types. length_in_bits prints the needed
	 * error messages (null dimension is impossible because arrays are
	 * converted to pointers).
	 */
	if (sym->s_type->t_tspec != VOID)
		(void)length_in_bits(sym->s_type, sym->s_name);

	sym->s_used = dcs->d_used;
	mark_as_set(sym);

	debug_printf("%s: ", __func__);
	debug_sym("", sym, "\n");
	return sym;
}

static bool
is_character_pointer(const type_t *tp)
{
	tspec_t st;

	return tp->t_tspec == PTR &&
	    (st = tp->t_subt->t_tspec,
		st == CHAR || st == SCHAR || st == UCHAR);
}

void
check_func_lint_directives(void)
{

	/* check for illegal combinations of lint directives */
	if (printflike_argnum != -1 && scanflike_argnum != -1) {
		/* ** PRINTFLIKE ** and ** SCANFLIKE ** cannot be combined */
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
	 * check if the numeric argument of a lint directive is compatible with
	 * the number of parameters of the function.
	 */
	int narg = 0;
	for (const sym_t *p = dcs->d_func_params; p != NULL; p = p->s_next)
		narg++;
	if (nargusg > narg) {
		/* parameter number mismatch in comment ** %s ** */
		warning(283, "ARGSUSED");
		nargusg = 0;
	}
	if (nvararg > narg) {
		/* parameter number mismatch in comment ** %s ** */
		warning(283, "VARARGS");
		nvararg = 0;
	}
	if (printflike_argnum > narg) {
		/* parameter number mismatch in comment ** %s ** */
		warning(283, "PRINTFLIKE");
		printflike_argnum = -1;
	} else if (printflike_argnum == 0) {
		printflike_argnum = -1;
	}
	if (scanflike_argnum > narg) {
		/* parameter number mismatch in comment ** %s ** */
		warning(283, "SCANFLIKE");
		scanflike_argnum = -1;
	} else if (scanflike_argnum == 0) {
		scanflike_argnum = -1;
	}
	if (printflike_argnum != -1 || scanflike_argnum != -1) {
		narg = printflike_argnum != -1
		    ? printflike_argnum : scanflike_argnum;
		const sym_t *param = dcs->d_func_params;
		for (int n = 1; n < narg; n++)
			param = param->s_next;
		if (!is_character_pointer(param->s_type)) {
			/* parameter %d must be 'char *' for PRINTFLIKE/... */
			warning(293, narg);
			printflike_argnum = scanflike_argnum = -1;
		}
	}
}

/*
 * Checks compatibility of an old-style function definition with a previous
 * prototype declaration.
 * Returns true if the position of the previous declaration should be reported.
 */
static bool
check_prototype_declaration(const sym_t *old_param, const sym_t *proto_param)
{
	type_t *old_tp = old_param->s_type;
	type_t *proto_tp = proto_param->s_type;
	bool dowarn = false;

	if (!types_compatible(old_tp, proto_tp, true, true, &dowarn)) {
		if (types_compatible(old_tp, proto_tp, true, false, &dowarn)) {
			/* type of '%s' does not match prototype */
			return gnuism(58, old_param->s_name);
		} else {
			/* type of '%s' does not match prototype */
			error(58, old_param->s_name);
			return true;
		}
	}
	if (dowarn) {
		/* TODO: Make this an error in C99 mode as well. */
		if (!allow_trad && !allow_c99)
			/* type of '%s' does not match prototype */
			error(58, old_param->s_name);
		else
			/* type of '%s' does not match prototype */
			warning(58, old_param->s_name);
		return true;
	}

	return false;
}

/*
 * Warn about parameters in old-style function definitions that default to int.
 * Check that an old-style function definition is compatible to a previous
 * prototype.
 */
void
check_func_old_style_parameters(void)
{
	sym_t *old_params = funcsym->u.s_old_style_params;
	sym_t *proto_params = funcsym->s_type->u.params;

	for (sym_t *arg = old_params; arg != NULL; arg = arg->s_next) {
		if (arg->s_defparam) {
			/* type of parameter '%s' defaults to 'int' */
			warning(32, arg->s_name);
			arg->s_defparam = false;
			mark_as_set(arg);
		}
	}

	/*
	 * If this is an old-style function definition and a prototype exists,
	 * compare the types of parameters.
	 */
	if (funcsym->s_osdef && funcsym->s_type->t_proto) {
		/*
		 * If the number of parameters does not match, we need not
		 * continue.
		 */
		int old_n = 0, proto_n = 0;
		bool msg = false;
		for (const sym_t *p = proto_params; p != NULL; p = p->s_next)
			proto_n++;
		for (const sym_t *p = old_params; p != NULL; p = p->s_next)
			old_n++;
		if (old_n != proto_n) {
			/* parameter mismatch: %d declared, %d defined */
			error(51, proto_n, old_n);
			msg = true;
		} else {
			const sym_t *proto_param = proto_params;
			const sym_t *old_param = old_params;
			while (old_n-- > 0) {
				msg |= check_prototype_declaration(old_param, proto_param);
				proto_param = proto_param->s_next;
				old_param = old_param->s_next;
			}
		}
		if (msg && rflag)
			/* prototype declaration */
			message_at(285, &dcs->d_redeclared_symbol->s_def_pos);

		/* from now on the prototype is valid */
		funcsym->s_osdef = false;
		funcsym->u.s_old_style_params = NULL;
	}
}

static void
check_local_hiding(const sym_t *dsym)
{
	switch (dsym->s_scl) {
	case AUTO:
		/* automatic '%s' hides external declaration */
		warning(86, dsym->s_name);
		break;
	case STATIC:
		/* static '%s' hides external declaration */
		warning(87, dsym->s_name);
		break;
	case TYPEDEF:
		/* typedef '%s' hides external declaration */
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
check_local_redeclaration(const sym_t *dsym, sym_t *rdsym)
{
	if (rdsym->s_block_level == 0) {
		if (hflag)
			check_local_hiding(dsym);

	} else if (rdsym->s_block_level == block_level) {

		/* no hflag, because it's illegal! */
		if (rdsym->s_param) {
			/*
			 * if allow_c90, a "redeclaration of '%s'" error is
			 * produced below
			 */
			if (!allow_c90) {
				if (hflag)
					/* declaration of '%s' hides ... */
					warning(91, dsym->s_name);
				symtab_remove_forever(rdsym);
			}
		}

	} else if (rdsym->s_block_level < block_level && hflag)
		/* declaration of '%s' hides earlier one */
		warning(95, dsym->s_name);

	if (rdsym->s_block_level == block_level) {
		/* redeclaration of '%s' */
		error(27, dsym->s_name);
		symtab_remove_forever(rdsym);
	}
}

/* Processes (re)declarations of external symbols inside blocks. */
static void
declare_external_in_block(sym_t *dsym)
{

	/* look for a symbol with the same name */
	sym_t *esym = dcs->d_redeclared_symbol;
	while (esym != NULL && esym->s_block_level != 0) {
		while ((esym = esym->s_symtab_next) != NULL) {
			if (esym->s_kind != SK_VCFT)
				continue;
			if (strcmp(dsym->s_name, esym->s_name) == 0)
				break;
		}
	}
	if (esym == NULL)
		return;
	if (esym->s_scl != EXTERN && esym->s_scl != STATIC) {
		/* gcc accepts this without a warning, pcc prints an error. */
		/* redeclaration of '%s' */
		warning(27, dsym->s_name);
		print_previous_declaration(esym);
		return;
	}

	bool dowarn = false;
	bool compatible = types_compatible(esym->s_type, dsym->s_type,
	    false, false, &dowarn);

	if (!compatible || dowarn) {
		if (esym->s_scl == EXTERN) {
			/* inconsistent redeclaration of extern '%s' */
			warning(90, dsym->s_name);
			print_previous_declaration(esym);
		} else {
			/* inconsistent redeclaration of static '%s' */
			warning(92, dsym->s_name);
			print_previous_declaration(esym);
		}
	}

	if (compatible) {
		/*
		 * Remember the external symbol, so we can update usage
		 * information at the end of the block.
		 */
		dsym->s_ext_sym = esym;
	}
}

/*
 * Completes a single local declaration/definition.
 */
void
declare_local(sym_t *dsym, bool has_initializer)
{

	/* Correct a mistake done in declarator_name(). */
	if (dsym->s_type->t_tspec == FUNC) {
		dsym->s_def = DECL;
		if (dcs->d_scl == NO_SCL)
			dsym->s_scl = EXTERN;
	}

	if (dsym->s_scl == EXTERN)
		/* nested 'extern' declaration of '%s' */
		warning(352, dsym->s_name);

	if (dsym->s_type->t_tspec == FUNC) {
		if (dsym->s_scl == STATIC) {
			/* dubious static function '%s' at block level */
			warning(93, dsym->s_name);
			dsym->s_scl = EXTERN;
		} else if (dsym->s_scl != EXTERN && dsym->s_scl != TYPEDEF) {
			/* function '%s' has illegal storage class */
			error(94, dsym->s_name);
			dsym->s_scl = EXTERN;
		}
	}

	/*
	 * functions may be declared inline at local scope, although this has
	 * no effect for a later definition of the same function.
	 *
	 * XXX it should have an effect if !allow_c90 is set. this would also
	 * be the way gcc behaves.
	 */
	if (dcs->d_inline) {
		if (dsym->s_type->t_tspec == FUNC)
			dsym->s_inline = true;
		else {
			/* variable '%s' declared inline */
			warning(268, dsym->s_name);
		}
	}

	if (any_query_enabled && dsym->s_type->t_const
	    && (dsym->s_scl == AUTO || dsym->s_scl == REG)) {
		/* const automatic variable '%s' */
		query_message(18, dsym->s_name);
	}

	check_function_definition(dsym, true);

	check_type(dsym);

	if (dcs->d_redeclared_symbol != NULL && dsym->s_scl == EXTERN)
		declare_external_in_block(dsym);

	if (dsym->s_scl == EXTERN) {
		/*
		 * XXX if the static variable at level 0 is only defined later,
		 * checking will be possible.
		 */
		if (dsym->s_ext_sym == NULL)
			outsym(dsym, EXTERN, dsym->s_def);
		else
			outsym(dsym, dsym->s_ext_sym->s_scl, dsym->s_def);
	}

	if (dcs->d_redeclared_symbol != NULL)
		check_local_redeclaration(dsym, dcs->d_redeclared_symbol);

	if (has_initializer && !check_init(dsym)) {
		dsym->s_def = DEF;
		mark_as_set(dsym);
	}

	if (dsym->s_scl == TYPEDEF) {
		dsym->s_type = block_dup_type(dsym->s_type);
		dsym->s_type->t_typedef = true;
		set_first_typedef(dsym->s_type, dsym);
	}

	if (dsym->s_scl == STATIC && any_query_enabled)
		/* static variable '%s' in function */
		query_message(11, dsym->s_name);

	debug_printf("%s: ", __func__);
	debug_sym("", dsym, "\n");
}

/* Create a symbol for an abstract declaration. */
static sym_t *
abstract_name_level(bool enclosing)
{

	lint_assert(dcs->d_kind == DLK_ABSTRACT
	    || dcs->d_kind == DLK_PROTO_PARAMS);

	sym_t *sym = block_zero_alloc(sizeof(*sym), "sym");
	sym->s_name = unnamed;
	sym->s_def = DEF;
	sym->s_scl = ABSTRACT;
	sym->s_block_level = -1;
	sym->s_param = dcs->d_kind == DLK_PROTO_PARAMS;

	/*
	 * At this point, dcs->d_type contains only the basic type.  That type
	 * will be updated later, adding pointers, arrays and functions as
	 * necessary.
	 */
	sym->s_type = enclosing ? dcs->d_enclosing->d_type : dcs->d_type;
	dcs->d_redeclared_symbol = NULL;

	debug_printf("%s: ", __func__);
	debug_sym("", sym, "\n");
	return sym;
}

sym_t *
abstract_name(void)
{
	return abstract_name_level(false);
}

sym_t *
abstract_enclosing_name(void)
{
	return abstract_name_level(true);
}

/* Removes anything which has nothing to do on global level. */
void
global_clean_up(void)
{

	while (dcs->d_enclosing != NULL)
		end_declaration_level();

	clean_up_after_error();
	block_level = 0;
	mem_block_level = 0;
	debug_step("%s: mem_block_level = %zu", __func__, mem_block_level);
	global_clean_up_decl(true);
}

sym_t *
declare_abstract_type(sym_t *sym)
{

	check_function_definition(sym, true);
	check_type(sym);
	return sym;
}

/* Checks size after declarations of variables and their initialization. */
void
check_size(const sym_t *dsym)
{

	if (dsym->s_def == DEF &&
	    dsym->s_scl != TYPEDEF &&
	    dsym->s_type->t_tspec != FUNC &&
	    length_in_bits(dsym->s_type, dsym->s_name) == 0 &&
	    dsym->s_type->t_tspec == ARRAY &&
	    dsym->s_type->u.dimension == 0) {
		if (!allow_c90)
			/* empty array declaration for '%s' */
			warning(190, dsym->s_name);
		else
			/* empty array declaration for '%s' */
			error(190, dsym->s_name);
	}
}

/* Mark an object as set if it is not already. */
void
mark_as_set(sym_t *sym)
{

	if (!sym->s_set) {
		sym->s_set = true;
		sym->s_set_pos = unique_curr_pos();
	}
}

/* Mark an object as used if it is not already. */
void
mark_as_used(sym_t *sym, bool fcall, bool szof)
{

	if (!sym->s_used) {
		sym->s_used = true;
		sym->s_use_pos = unique_curr_pos();
	}
	/*
	 * For function calls, another record is written.
	 *
	 * XXX: Should symbols used in sizeof() be treated as used or not?
	 * Probably not, because there is no point in declaring an external
	 * variable only to get its size.
	 */
	if (!fcall && !szof && sym->s_kind == SK_VCFT && sym->s_scl == EXTERN)
		outusg(sym);
}

/* Warns about variables and labels that are not used or only set. */
void
check_usage(const decl_level *dl)
{
	/* for this warning LINTED has no effect */
	int saved_lwarn = lwarn;
	lwarn = LWARN_ALL;

	debug_step("begin lwarn %d", lwarn);
	for (sym_t *sym = dl->d_first_dlsym;
	    sym != NULL; sym = sym->s_level_next)
		check_usage_sym(dl->d_asm, sym);
	lwarn = saved_lwarn;
	debug_step("end lwarn %d", lwarn);
}

static void
check_parameter_usage(bool novar, const sym_t *arg)
{

	lint_assert(arg->s_set);

	if (novar)
		return;

	if (!arg->s_used && !vflag)
		/* parameter '%s' unused in function '%s' */
		warning_at(231, &arg->s_def_pos, arg->s_name, funcsym->s_name);
}

static void
check_variable_usage(bool novar, const sym_t *sym)
{

	lint_assert(block_level != 0);

	/* example at file scope: int c = ({ return 3; }); */
	if (sym->s_block_level == 0 && isdigit((unsigned char)sym->s_name[0]))
		return;

	/* errors in expressions easily cause lots of these warnings */
	if (seen_error)
		return;

	/*
	 * XXX Only variables are checked, although types should probably also
	 * be checked
	 */
	scl_t sc = sym->s_scl;
	if (sc != EXTERN && sc != STATIC && sc != AUTO && sc != REG)
		return;

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
		 * information about usage is taken over into the symbol table
		 * entry at level 0 if the symbol was locally declared as an
		 * external symbol.
		 *
		 * XXX This is wrong for symbols declared static at level 0 if
		 * the usage information stems from sizeof(). This is because
		 * symbols at level 0 only used in sizeof() are considered to
		 * not be used.
		 */
		sym_t *xsym = sym->s_ext_sym;
		if (xsym != NULL) {
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
check_label_usage(const sym_t *lab)
{

	lint_assert(block_level == 1);
	lint_assert(lab->s_block_level == 1);

	if (funcsym == NULL)
		/* syntax error '%s' */
		error(249, "labels are only valid inside a function");
	else if (lab->s_set && !lab->s_used)
		/* label '%s' unused in function '%s' */
		warning_at(232, &lab->s_set_pos, lab->s_name, funcsym->s_name);
	else if (!lab->s_set)
		/* undefined label '%s' */
		warning_at(23, &lab->s_use_pos, lab->s_name);
}

static void
check_tag_usage(const sym_t *sym)
{

	if (!is_incomplete(sym->s_type))
		return;

	/* always complain about incomplete tags declared inside blocks */
	if (zflag || dcs->d_kind != DLK_EXTERN)
		return;

	switch (sym->s_type->t_tspec) {
	case STRUCT:
		/* struct '%s' never defined */
		warning_at(233, &sym->s_def_pos, sym->s_name);
		break;
	case UNION:
		/* union '%s' never defined */
		warning_at(234, &sym->s_def_pos, sym->s_name);
		break;
	default:
		lint_assert(sym->s_type->t_tspec == ENUM);
		/* enum '%s' never defined */
		warning_at(235, &sym->s_def_pos, sym->s_name);
		break;
	}
}

/* Warns about a variable or a label that is not used or only set. */
void
check_usage_sym(bool novar, const sym_t *sym)
{

	if (sym->s_block_level == -1)
		return;

	if (sym->s_kind == SK_VCFT && sym->s_param)
		check_parameter_usage(novar, sym);
	else if (sym->s_kind == SK_VCFT)
		check_variable_usage(novar, sym);
	else if (sym->s_kind == SK_LABEL)
		check_label_usage(sym);
	else if (sym->s_kind == SK_TAG)
		check_tag_usage(sym);
}

static void
check_global_variable_size(const sym_t *sym)
{

	if (sym->s_def != TDEF)
		return;
	if (sym->s_type->t_tspec == FUNC)
		/* Maybe a syntax error after a function declaration. */
		return;
	if (sym->s_def == TDEF && sym->s_type->t_tspec == VOID)
		/* Prevent an internal error in length_in_bits below. */
		return;

	pos_t cpos = curr_pos;
	curr_pos = sym->s_def_pos;
	int len_in_bits = length_in_bits(sym->s_type, sym->s_name);
	curr_pos = cpos;

	if (len_in_bits == 0 &&
	    sym->s_type->t_tspec == ARRAY && sym->s_type->u.dimension == 0) {
		/* TODO: C99 6.7.5.2p1 defines this as an error as well. */
		if (!allow_c90 ||
		    (sym->s_scl == EXTERN && (allow_trad || allow_c99))) {
			/* empty array declaration for '%s' */
			warning_at(190, &sym->s_def_pos, sym->s_name);
		} else {
			/* empty array declaration for '%s' */
			error_at(190, &sym->s_def_pos, sym->s_name);
		}
	}
}

static void
check_unused_static_global_variable(const sym_t *sym)
{
	if (sym->s_type->t_tspec == FUNC) {
		if (sym->s_def == DEF) {
			if (!sym->s_inline)
				/* static function '%s' unused */
				warning_at(236, &sym->s_def_pos, sym->s_name);
		} else
			/* static function '%s' declared but not defined */
			warning_at(290, &sym->s_def_pos, sym->s_name);
	} else if (!sym->s_set)
		/* static variable '%s' unused */
		warning_at(226, &sym->s_def_pos, sym->s_name);
	else
		/* static variable '%s' set but not used */
		warning_at(307, &sym->s_def_pos, sym->s_name);
}

static void
check_static_global_variable(const sym_t *sym)
{
	if (sym->s_type->t_tspec == FUNC && sym->s_used && sym->s_def != DEF)
		/* static function '%s' called but not defined */
		error_at(225, &sym->s_use_pos, sym->s_name);

	if (!sym->s_used)
		check_unused_static_global_variable(sym);

	if (allow_c90 && sym->s_def == TDEF && sym->s_type->t_const)
		/* const object '%s' should have initializer */
		warning_at(227, &sym->s_def_pos, sym->s_name);
}

static void
check_global_variable(const sym_t *sym)
{
	scl_t scl = sym->s_scl;

	if (scl == TYPEDEF || scl == BOOL_CONST || scl == ENUM_CONST)
		return;

	if (scl == NO_SCL)
		return;		/* May be caused by a syntax error. */

	lint_assert(scl == EXTERN || scl == STATIC);

	check_global_variable_size(sym);

	if (scl == STATIC)
		check_static_global_variable(sym);
}

void
end_translation_unit(void)
{

	if (block_level != 0 || dcs->d_enclosing != NULL)
		norecover();

	for (const sym_t *sym = dcs->d_first_dlsym;
	    sym != NULL; sym = sym->s_level_next) {
		if (sym->s_block_level == -1)
			continue;
		if (sym->s_kind == SK_VCFT)
			check_global_variable(sym);
		else if (sym->s_kind == SK_TAG)
			check_tag_usage(sym);
		else
			lint_assert(sym->s_kind == SK_MEMBER);
	}
}

/*
 * Prints information about location of previous definition/declaration.
 */
void
print_previous_declaration(const sym_t *psym)
{

	if (!rflag)
		return;

	if (psym->s_def == DEF || psym->s_def == TDEF)
		/* previous definition of '%s' */
		message_at(261, &psym->s_def_pos, psym->s_name);
	else
		/* previous declaration of '%s' */
		message_at(260, &psym->s_def_pos, psym->s_name);
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

	if (tn == NULL)
		return 1;

	val_t *v = integer_constant(tn, required);
	bool is_unsigned = is_uinteger(v->v_tspec);
	int64_t val = v->u.integer;
	free(v);

	/*
	 * Abstract declarations are used inside expression. To free the memory
	 * would be a fatal error. We don't free blocks that are inside casts
	 * because these will be used later to match types.
	 */
	if (tn->tn_op != CON && dcs->d_kind != DLK_ABSTRACT)
		expr_free_all();

	bool out_of_bounds = is_unsigned
	    ? (uint64_t)val > (uint64_t)TARG_INT_MAX
	    : val > (int64_t)TARG_INT_MAX || val < (int64_t)TARG_INT_MIN;
	if (out_of_bounds)
		/* integral constant too large */
		warning(56);
	return (int)val;
}
