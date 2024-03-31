/* $NetBSD: lint1.h,v 1.222 2024/03/31 20:28:45 rillig Exp $ */

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

#include "lint.h"
#include "op.h"

/*
 * A memory pool collects allocated objects that must be available until:
 * - the end of a block,
 * - the end of an expression, or
 * - the end of the translation unit.
 */
typedef struct memory_pool {
	struct memory_pool_item {
		void *p;
#ifdef DEBUG_MEM
		size_t size;
		const char *descr;
#endif
	} *items;
	size_t	len;
	size_t	cap;
} memory_pool;

/* See saved_lwarn in cgram.y. */
#define LWARN_ALL	(-2)
#define LWARN_NONE	(-1)

/*
 * Describes the position of a declaration or anything else.
 *
 * FIXME: Just a single file:lineno pair is not enough to accurately describe
 *  the position of a symbol.  The whole inclusion path at that point must be
 *  stored as well.  This makes a difference for symbols from included
 *  headers, see print_stack_trace.
 */
typedef struct {
	const char *p_file;
	int	p_line;
	int	p_uniq;			/* uniquifier */
} pos_t;

typedef struct {
	bool tq_const;
	bool tq_restrict;
	bool tq_volatile;
	bool tq_atomic;
} type_qualifiers;

/* A bool, integer or floating-point value. */
typedef struct {
	tspec_t	v_tspec;
	/*
	 * Set if an integer constant is unsigned only in C90 and later, but
	 * not in traditional C.
	 *
	 * See the operators table in oper.c, columns "l r".
	 */
	bool	v_unsigned_since_c90;
	bool	v_char_constant;
	union {
		int64_t		integer;
		long double	floating;
	} u;
} val_t;

typedef struct sym sym_t;

/*
 * Structures of type struct_or_union uniquely identify structures. This can't
 * be done in structures of type type_t, because these are copied if they must
 * be modified. So it would not be possible to check if two structures are
 * identical by comparing the pointers to the type structures.
 *
 * If the structure has no tag name, its first typedef name is used to identify
 * the structure in lint2.
 */
typedef struct {
	unsigned int sou_size_in_bits;
	unsigned int sou_align_in_bits;
	bool	sou_incomplete:1;
	sym_t	*sou_first_member;
	sym_t	*sou_tag;
	sym_t	*sou_first_typedef;
} struct_or_union;

/* The same as in struct_or_union, only for enums. */
typedef struct {
	bool	en_incomplete:1;
	sym_t	*en_first_enumerator;
	sym_t	*en_tag;
	sym_t	*en_first_typedef;
} enumeration;

/* The type of an expression, object or function. */
struct lint1_type {
	tspec_t	t_tspec;	/* type specifier */
	bool	t_incomplete_array:1;
	bool	t_const:1;
	bool	t_volatile:1;
	bool	t_proto:1;	/* function prototype (u.params valid) */
	bool	t_vararg:1;	/* prototype with '...' */
	bool	t_typedef:1;	/* type defined with typedef */
	bool	t_typeof:1;	/* type defined with GCC's __typeof__ */
	bool	t_bitfield:1;
	/*
	 * Either the type is currently an enum (having t_tspec ENUM), or it
	 * is an integer type (typically INT) that has been implicitly
	 * converted from an enum type.  In both cases, u.enumer is valid.
	 */
	bool	t_is_enum:1;
	bool	t_packed:1;
	union {
		int		dimension;	/* if ARRAY */
		struct_or_union	*sou;
		enumeration	*enumer;
		sym_t		*params;	/* if t_proto */
	} u;
	unsigned int	t_bit_field_width:8;
	unsigned int	t_bit_field_offset:24;
	struct lint1_type *t_subt;	/*- element type (if ARRAY),
					 * return value (if FUNC),
					 * target type (if PTR) */
};

typedef enum {
	SK_VCFT,		/* variable, constant, function, type */
	SK_MEMBER,		/* member of a struct or union */
	SK_TAG,
	SK_LABEL
} symbol_kind;

/* storage classes and related things */
typedef enum {
	NO_SCL,
	EXTERN,			/* external symbols (independent of decl_t) */
	STATIC,			/* static symbols (local and global) */
	AUTO,			/* automatic symbols (except register) */
	REG,			/* register */
	TYPEDEF,
	THREAD_LOCAL,
	STRUCT_TAG,
	UNION_TAG,
	ENUM_TAG,
	STRUCT_MEMBER,
	UNION_MEMBER,
	BOOL_CONST,
	ENUM_CONST,
	ABSTRACT,		/* abstract symbol (sizeof, casts, unnamed
				 * argument) */
} scl_t;

/* C23 6.7.4 */
typedef enum {
	FS_INLINE,		/* since C99 */
	FS_NORETURN,		/* since C11 */
} function_specifier;

/* A type, variable, keyword; basically anything that has a name. */
struct sym {
	const char *s_name;
	const char *s_rename;	/* renamed symbol's given name */
	pos_t	s_def_pos;	/* position of last (prototype) definition,
				 * prototype declaration, no-prototype-def.,
				 * tentative definition or declaration, in this
				 * order */
	pos_t	s_set_pos;	/* position of first initialization */
	pos_t	s_use_pos;	/* position of first use */
	symbol_kind s_kind;
	const struct keyword *s_keyword;
	bool	s_bitfield:1;
	bool	s_set:1;
	bool	s_used:1;
	bool	s_param:1;
	bool	s_register:1;
	bool	s_defparam:1;	/* undefined symbol in old-style function
				 * definition */
	bool	s_return_type_implicit_int:1;
	bool	s_osdef:1;	/* symbol stems from old-style function def. */
	bool	s_inline:1;
	sym_t	*s_ext_sym;	/* for locally declared external symbols, the
				 * pointer to the external symbol with the same
				 * name */
	def_t	s_def;		/* declared, tentative defined, defined */
	scl_t	s_scl;
	int	s_block_level;	/* level of declaration, -1 if not in symbol
				 * table */
	type_t	*s_type;
	union {
		bool s_bool_constant;
		int s_enum_constant;
		struct {
			struct_or_union *sm_containing_type;
			unsigned int sm_offset_in_bits;
		} s_member;
		struct {
			int sk_token;
			union {
				/* if T_TYPE or T_STRUCT_OR_UNION */
				tspec_t sk_tspec;
				/* if T_QUAL */
				type_qualifiers sk_type_qualifier;
				/* if T_FUNCTION_SPECIFIER */
				function_specifier function_specifier;
			} u;
		} s_keyword;
		sym_t	*s_old_style_params;	/* parameters in an old-style
						 * function definition */
	} u;
	sym_t	*s_symtab_next;	/* next symbol in the same symtab bucket */
	sym_t	**s_symtab_ref;	/* pointer to s_symtab_next of the previous
				 * symbol */
	sym_t	*s_next;	/* next struct/union member, enumerator,
				 * parameter */
	sym_t	*s_level_next;	/* next symbol declared on the same level */
};

/*
 * Used to keep some information about symbols before they are entered
 * into the symbol table.
 */
typedef struct {
	const char *sb_name;		/* name of symbol */
	size_t	sb_len;			/* length (without '\0') */
	sym_t	*sb_sym;		/* symbol table entry */
} sbuf_t;


typedef struct {
	struct tnode *func;
	struct tnode **args;
	size_t args_len;
	size_t args_cap;
} function_call;

typedef struct tnode tnode_t;

/* An expression, forming an abstract syntax tree. */
struct tnode {
	op_t	tn_op;
	type_t	*tn_type;
	bool	tn_lvalue:1;
	bool	tn_cast:1;	/* if tn_op == CVT, it's an explicit cast */
	bool	tn_parenthesized:1;
	bool	tn_sys:1;	/* comes from a system header */
	bool	tn_system_dependent:1; /* depends on sizeof or offsetof */
	union {
		struct {
			tnode_t *left;	/* (left) operand */
			tnode_t *right;	/* right operand */
		} ops;
		sym_t	*sym;		/* if NAME */
		val_t	value;		/* if CON */
		buffer	*str_literals;	/* if STRING; for
					 * character strings, 'data' points to
					 * the concatenated string literals in
					 * source form, and 'len' is the
					 * length of the concatenation; for
					 * wide strings, 'data' is NULL and
					 * 'len' is the number of resulting
					 * characters */
		function_call *call;	/* if CALL */
	} u;
};

struct generic_association {
	type_t *ga_arg;		/* NULL means default or error */
	tnode_t *ga_result;	/* NULL means error */
	struct generic_association *ga_prev;
};

typedef struct {
	bool has_dim;
	int dim;
} array_size;

typedef enum decl_level_kind {
	DLK_EXTERN,		/* global types, variables or functions */
	DLK_STRUCT,		/* struct members */
	DLK_UNION,		/* union members */
	DLK_ENUM,		/* enum constants */
	DLK_OLD_STYLE_PARAMS,	/* parameters in an old-style function
				 * definition */
	DLK_PROTO_PARAMS,	/* parameters in a prototype function
				 * definition */
	DLK_AUTO,		/* local types or variables */
	DLK_ABSTRACT		/* abstract (unnamed) declaration; type name;
				 * used in casts and sizeof */
} decl_level_kind;

/*
 * A declaration level collects information for a declarator in a struct,
 * union or enum declaration, a parameter declaration list, or a plain
 * declaration in or outside a function body.
 *
 * For nested declarations, the global 'dcs' holds all information needed for
 * the current level, the outer levels are available via 'd_enclosing'.
 */
typedef struct decl_level {
	decl_level_kind d_kind;
	tspec_t	d_abstract_type;/* VOID, BOOL, CHAR, INT or COMPLEX */
	tspec_t	d_complex_mod;	/* FLOAT or DOUBLE */
	tspec_t	d_sign_mod;	/* SIGNED or UNSIGN */
	tspec_t	d_rank_mod;	/* SHORT, LONG or LLONG */
	scl_t	d_scl;		/* storage class */
	type_t	*d_type;	/* after dcs_end_type, the pointer to the type
				 * used for all declarators */
	sym_t	*d_redeclared_symbol;
	unsigned int d_sou_size_in_bits;	/* size of the structure or
						 * union being built, without
						 * trailing padding */
	unsigned int d_sou_align_in_bits;	/* alignment of the structure
						 * or union being built */
	type_qualifiers d_qual;	/* in declaration specifiers */
	bool	d_inline:1;	/* inline in declaration specifiers */
	bool	d_multiple_storage_classes:1; /* reported in dcs_end_type */
	bool	d_invalid_type_combination:1;
	bool	d_nonempty_decl:1; /* in a function declaration, whether at
				 * least one tag was declared */
	bool	d_no_type_specifier:1;
	bool	d_asm:1;	/* set if d_ctx == AUTO and asm() present */
	bool	d_packed:1;
	bool	d_used:1;
	type_t	*d_tag_type;	/* during a member or enumerator declaration,
				 * the tag type to which the member belongs */
	sym_t	*d_func_params;	/* during a function declaration, the
				 * parameters, stored in the enclosing level */
	pos_t	d_func_def_pos;	/* position of the function definition */
	sym_t	*d_first_dlsym;	/* first symbol declared at this level */
	sym_t	**d_last_dlsym;	/* points to s_level_next in the last symbol
				   declaration at this level */
	sym_t	*d_func_proto_syms;	/* symbols defined in prototype, such
					 * as tagged types or parameter names,
					 * may overlap d_func_params */
	struct decl_level *d_enclosing; /* the enclosing declaration level */
} decl_level;

typedef struct {
	sym_t	*first;
	bool	vararg:1;
	bool	prototype:1;
} parameter_list;

/*
 * A sequence of asterisks and qualifiers, from right to left.  For example,
 * 'const ***volatile **const volatile' results in [c-v-, ----, --v-, ----,
 * ----].  The leftmost 'const' is not included in this list, it is stored in
 * dcs->d_qual instead.
 */
typedef struct qual_ptr {
	type_qualifiers qualifiers;
	struct qual_ptr *p_next;
} qual_ptr;

/* The values of the 'case' labels. */
typedef struct {
	val_t	*vals;
	size_t	len;
	size_t	cap;
} case_labels;

typedef enum {
	CS_DO_WHILE,
	CS_FOR,
	CS_FUNCTION_BODY,
	CS_IF,
	CS_SWITCH,
	CS_WHILE
} control_statement_kind;

/*
 * Used to keep information about nested control statements.
 */
typedef struct control_statement {
	control_statement_kind c_kind;	/* to ensure proper nesting */
	bool	c_loop:1;	/* 'continue' and 'break' are valid */
	bool	c_switch:1;	/* 'case' and 'break' are valid */
	bool	c_break:1;	/* the loop/switch has a reachable 'break'
				 * statement */
	bool	c_continue:1;	/* the loop has a reachable 'continue'
				 * statement */
	bool	c_default:1;	/* the switch has a 'default' label */
	bool	c_maybe_endless:1;	/* the controlling expression is
					 * always true (as in 'for (;;)' or
					 * 'while (1)'), there may be break
					 * statements though */
	bool	c_always_then:1;
	bool	c_reached_end_of_then:1;
	bool	c_had_return_noval:1;	/* had "return;" */
	bool	c_had_return_value:1;	/* had "return expr;" */

	type_t	*c_switch_type;	/* type of switch expression */
	tnode_t	*c_switch_expr;
	case_labels c_case_labels;	/* list of case values */

	memory_pool c_for_expr3_mem;	/* saved memory for end of loop
					 * expression in for() */
	tnode_t	*c_for_expr3;	/* end of loop expr in for() */
	pos_t	c_for_expr3_pos;	/* position of end of loop expr */
	pos_t	c_for_expr3_csrc_pos;	/* same for csrc_pos */

	struct control_statement *c_surrounding;
} control_statement;

typedef struct {
	size_t lo;		/* inclusive */
	size_t hi;		/* inclusive */
} range_t;

typedef enum designator_kind {
	DK_MEMBER,		/* .member */
	DK_SUBSCRIPT,		/* [subscript] */
	DK_SCALAR		/* no textual representation, not generated by
				 * the parser; used for scalar initializer
				 * expressions surrounded by braces */
} designator_kind;

/*
 * A single component on the path from the "current object" of a brace level
 * to the sub-object that is initialized by an expression.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designator {
	designator_kind	dr_kind;
	const sym_t	*dr_member;	/* for DK_MEMBER */
	size_t		dr_subscript;	/* for DK_SUBSCRIPT */
	bool		dr_done;
} designator;

/*
 * The path from the "current object" of a brace level to the sub-object that
 * is initialized by an expression.  Examples of designations are '.member'
 * or '.member[123].member.member[1][1]'.
 *
 * C99 6.7.8p6, 6.7.8p7
 */
typedef struct designation {
	designator	*dn_items;
	size_t		dn_len;
	size_t		dn_cap;
} designation;

typedef enum {
	LC_ARGSUSED,
	LC_BITFIELDTYPE,
	LC_CONSTCOND,
	LC_FALLTHROUGH,
	LC_LINTLIBRARY,
	LC_LINTED,
	LC_LONGLONG,
	LC_NOTREACHED,
	LC_PRINTFLIKE,
	LC_PROTOLIB,
	LC_SCANFLIKE,
	LC_VARARGS,
} lint_comment;

typedef struct {
	size_t start;
	size_t end;
	uint64_t value;
	bool escaped;		/* \n, \003, \x24 */
	bool named_escape;	/* \a, \n, etc. */
	bool literal_escape;	/* \?, \\, etc. */
	uint8_t octal_digits;	/* 1 to 3; 0 means not applicable */
	uint8_t hex_digits;	/* 1 to 3; 0 means not applicable */
	bool next_literal;	/* when a new string literal begins */
	bool invalid_escape;	/* single-character escape, recoverable */
	bool overflow;		/* for octal and hex escapes */
	bool missing_hex_digits;
	bool unescaped_newline;	/* stops iterating */
} quoted_iterator;

#include "externs1.h"

#define lint_assert(cond)						\
	do {								\
		if (!(cond))						\
			assert_failed(__FILE__, __LINE__, __func__, #cond); \
	} while (false)

#ifdef DEBUG
#  include "err-msgs.h"

/* ARGSUSED */
static inline void __printflike(1, 2)
check_printf(const char *fmt, ...)
{
}

#  define wrap_check_printf_at(func, msgid, pos, args...)		\
	do {								\
		check_printf(__CONCAT(MSG_, msgid), ##args);		\
		(func)(msgid, pos, ##args);				\
	} while (false)

#  define error_at(msgid, pos, args...) \
	wrap_check_printf_at(error_at, msgid, pos, ##args)
#  define warning_at(msgid, pos, args...) \
	wrap_check_printf_at(warning_at, msgid, pos, ##args)
#  define message_at(msgid, pos, args...) \
	wrap_check_printf_at(message_at, msgid, pos, ##args)

#  define wrap_check_printf(func, cond, msgid, args...)			\
	({								\
		if (/* CONSTCOND */cond)				\
			debug_step("%s:%d: %s %d '%s' in %s",		\
			    __FILE__, __LINE__,	#func, msgid,		\
			    __CONCAT(MSG_, msgid), __func__);		\
		check_printf(__CONCAT(MSG_, msgid), ##args);		\
		(func)(msgid, ##args);					\
		/* LINTED 129 */					\
	})

#  define error(msgid, args...) wrap_check_printf(error, \
    true, msgid, ##args)
#  define warning(msgid, args...) wrap_check_printf(warning, \
    true, msgid, ##args)
#  define gnuism(msgid, args...) wrap_check_printf(gnuism, \
    !allow_gcc || (!allow_trad && !allow_c99), msgid, ##args)
#  define c99ism(msgid, args...) wrap_check_printf(c99ism, \
    !allow_c99 && (!allow_gcc || !allow_trad), msgid, ##args)
#  define c11ism(msgid, args...) wrap_check_printf(c11ism, \
    !allow_c11 && !allow_gcc, msgid, ##args)
#  define c23ism(msgid, args...) wrap_check_printf(c23ism, \
    !allow_c23, msgid, ##args)
#endif

#ifdef DEBUG
#  define query_message(query_id, args...)				\
	do {								\
		debug_step("%s:%d: query %d '%s' in %s",		\
		    __FILE__, __LINE__,					\
		    query_id, __CONCAT(MSG_Q, query_id), __func__);	\
		check_printf(__CONCAT(MSG_Q, query_id), ##args);	\
		(query_message)(query_id, ##args);			\
	} while (false)
#else
#  define query_message(...)						\
	do {								\
		if (any_query_enabled)					\
			(query_message)(__VA_ARGS__);			\
	} while (false)
#endif

/* Copies curr_pos, keeping things unique. */
static inline pos_t
unique_curr_pos(void)
{
	pos_t curr = curr_pos;
	curr_pos.p_uniq++;
	if (curr_pos.p_file == csrc_pos.p_file)
		csrc_pos.p_uniq++;
	return curr;
}

static inline bool
is_nonzero_val(const val_t *val)
{
	return is_floating(val->v_tspec)
	    ? val->u.floating != 0.0
	    : val->u.integer != 0;
}

static inline bool
constant_is_nonzero(const tnode_t *tn)
{
	lint_assert(tn->tn_op == CON);
	lint_assert(tn->tn_type->t_tspec == tn->u.value.v_tspec);
	return is_nonzero_val(&tn->u.value);
}

static inline bool
is_zero(const tnode_t *tn)
{
	return tn != NULL && tn->tn_op == CON && !is_nonzero_val(&tn->u.value);
}

static inline bool
is_nonzero(const tnode_t *tn)
{
	return tn != NULL && tn->tn_op == CON && is_nonzero_val(&tn->u.value);
}

static inline const char *
op_name(op_t op)
{
	return modtab[op].m_name;
}

static inline bool
is_binary(const tnode_t *tn)
{
	return modtab[tn->tn_op].m_binary;
}

static inline uint64_t
bit(unsigned i)
{
	/*
	 * TODO: Add proper support for INT128. This involves changing val_t to
	 * 128 bits.
	 */
	if (i >= 64)
		return 0;	/* XXX: not correct for INT128 and UINT128 */

	lint_assert(i < 64);
	return (uint64_t)1 << i;
}

static inline bool
msb(int64_t si, tspec_t t)
{
	return ((uint64_t)si & bit(size_in_bits(t) - 1)) != 0;
}

static inline uint64_t
value_bits(unsigned bitsize)
{
	lint_assert(bitsize > 0);

	/* for long double (80 or 128), double _Complex (128) */
	/*
	 * XXX: double _Complex does not have 128 bits of precision, therefore
	 * it should never be necessary to query the value bits of such a type;
	 * see d_c99_complex_split.c to trigger this case.
	 */
	if (bitsize >= 64)
		return ~(uint64_t)0;

	return ~(~(uint64_t)0 << bitsize);
}

/* C99 6.7.8p7 */
static inline bool
is_struct_or_union(tspec_t t)
{
	return t == STRUCT || t == UNION;
}

static inline bool
is_member(const sym_t *sym)
{
	return sym->s_scl == STRUCT_MEMBER || sym->s_scl == UNION_MEMBER;
}

static inline void
set_sym_kind(symbol_kind kind)
{
	if (yflag)
		debug_step("%s: %s -> %s", __func__,
		    symbol_kind_name(sym_kind), symbol_kind_name(kind));
	sym_kind = kind;
}
