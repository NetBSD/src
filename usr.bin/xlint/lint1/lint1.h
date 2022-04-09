/* $NetBSD: lint1.h,v 1.146 2022/04/09 14:50:18 rillig Exp $ */

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

#include "lint.h"
#include "err-msgs.h"
#include "op.h"

#define LWARN_BAD	(-3)
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
	const	char *p_file;
	int	p_line;
	int	p_uniq;			/* uniquifier */
} pos_t;

/* Copies curr_pos, keeping things unique. */
#define	UNIQUE_CURR_POS(pos)						\
	do {								\
		(pos) = curr_pos;					\
		curr_pos.p_uniq++;					\
		if (curr_pos.p_file == csrc_pos.p_file)			\
			csrc_pos.p_uniq++;				\
	} while (false)

/*
 * Strings cannot be referenced simply by a pointer to their first
 * char. This is because strings can contain NUL characters other than the
 * trailing NUL.
 *
 * Strings are stored with a trailing NUL.
 */
typedef	struct strg {
	bool	st_char;	/* string doesn't have an 'L' prefix */
	size_t	st_len;		/* length without trailing NUL */
	void	*st_mem;	/* char[] for st_char, or wchar_t[] */
} strg_t;

/*
 * qualifiers (only for lex/yacc interface)
 */
typedef enum {
	CONST, VOLATILE, RESTRICT, THREAD
} tqual_t;

/* An integer or floating-point value. */
typedef struct {
	tspec_t	v_tspec;
	/*
	 * Set if an integer constant is unsigned only in C90 and later, but
	 * not in traditional C.
	 *
	 * See the operators table in ops.def, columns "l r".
	 */
	bool	v_unsigned_since_c90;
	union {
		int64_t	_v_quad;	/* integers */
		ldbl_t	_v_ldbl;	/* floats */
	} v_u;
} val_t;

#define v_quad	v_u._v_quad
#define v_ldbl	v_u._v_ldbl

/*
 * Structures of type struct_or_union uniquely identify structures. This can't
 * be done in structures of type type_t, because these are copied
 * if they must be modified. So it would not be possible to check
 * if two structures are identical by comparing the pointers to
 * the type structures.
 *
 * The typename is used if the structure is unnamed to identify
 * the structure type in pass 2.
 */
typedef	struct {
	unsigned int sou_size_in_bits;
	unsigned short sou_align_in_bits;
	bool	sou_incomplete:1;
	struct	sym *sou_first_member;
	struct	sym *sou_tag;
	struct	sym *sou_first_typedef;
} struct_or_union;

/*
 * same as above for enums
 */
typedef	struct {
	bool	en_incomplete:1;
	struct	sym *en_first_enumerator;
	struct	sym *en_tag;
	struct	sym *en_first_typedef;
} enumeration;

/*
 * The type of an expression or object. Complex types are formed via t_subt
 * (for arrays, pointers and functions), as well as t_str.
 */
struct lint1_type {
	tspec_t	t_tspec;	/* type specifier */
	bool	t_incomplete_array:1;
	bool	t_const:1;	/* const modifier */
	bool	t_volatile:1;	/* volatile modifier */
	bool	t_proto:1;	/* function prototype (t_args valid) */
	bool	t_vararg:1;	/* prototype with '...' */
	bool	t_typedef:1;	/* type defined with typedef */
	bool	t_bitfield:1;
	/*
	 * Either the type is currently an enum (having t_tspec ENUM), or
	 * it is an integer type (typically INT) that has been implicitly
	 * converted from an enum type.  In both cases, t_enum is valid.
	 *
	 * The information about a former enum type is retained to allow
	 * type checks in expressions such as ((var1 & 0x0001) == var2), to
	 * detect when var1 and var2 are from incompatible enum types.
	 */
	bool	t_is_enum:1;
	bool	t_packed:1;
	union {
		int	_t_dim;		/* dimension (if ARRAY) */
		struct_or_union	*_t_str;
		enumeration	*_t_enum;
		struct	sym *_t_args;	/* arguments (if t_proto) */
	} t_u;
	struct {
		unsigned int	_t_flen:8;	/* length of bit-field */
		unsigned int	_t_foffs:24;	/* offset of bit-field */
	} t_b;
	struct	lint1_type *t_subt; /* element type (if ARRAY),
				 * return value (if FUNC),
				 * target type (if PTR) */
};

#define	t_dim	t_u._t_dim
/* TODO: rename t_str to t_sou, to avoid confusing it with strings. */
#define	t_str	t_u._t_str
#define	t_enum	t_u._t_enum
#define	t_args	t_u._t_args
#define	t_flen	t_b._t_flen
#define	t_foffs	t_b._t_foffs

/*
 * types of symbols
 */
typedef	enum {
	FVFT,		/* variables, functions, type names, enums */
	FMEMBER,	/* members of structs or unions */
	FTAG,		/* tags */
	FLABEL		/* labels */
} symt_t;

/*
 * storage classes and related things
 */
typedef enum {
	NOSCL,
	EXTERN,		/* external symbols (independent of decl_t) */
	STATIC,		/* static symbols (local and global) */
	AUTO,		/* automatic symbols (except register) */
	REG,		/* register */
	TYPEDEF,	/* typedef */
	STRUCT_TAG,
	UNION_TAG,
	ENUM_TAG,
	MOS,		/* member of struct */
	MOU,		/* member of union */
	BOOL_CONST,
	ENUM_CONST,
	ABSTRACT,	/* abstract symbol (sizeof, casts, unnamed argument) */
	OLD_STYLE_ARG,	/* old-style function argument declarations */
	PROTO_ARG,	/* used in declaration stack during prototype
			   declaration */
	INLINE		/* only used by the parser */
} scl_t;

/*
 * symbol table entry
 */
typedef	struct sym {
	const	char *s_name;
	const	char *s_rename;	/* renamed symbol's given name */
	pos_t	s_def_pos;	/* position of last (prototype) definition,
				   prototype declaration, no-prototype-def.,
				   tentative definition or declaration,
				   in this order */
	pos_t	s_set_pos;	/* position of first initialization */
	pos_t	s_use_pos;	/* position of first use */
	symt_t	s_kind;		/* type of symbol */
	const struct keyword *s_keyword;
	bool	s_bitfield:1;
	bool	s_set:1;	/* variable set, label defined */
	bool	s_used:1;	/* variable/label used */
	bool	s_arg:1;	/* symbol is function argument */
	bool	s_register:1;	/* symbol is register variable */
	bool	s_defarg:1;	/* undefined symbol in old style function
				   definition */
	bool	s_return_type_implicit_int:1;
	bool	s_osdef:1;	/* symbol stems from old style function def. */
	bool	s_inline:1;	/* true if this is an inline function */
	struct	sym *s_ext_sym;	/* for locally declared external symbols, the
				 * pointer to the external symbol with the
				 * same name */
	def_t	s_def;		/* declared, tentative defined, defined */
	scl_t	s_scl;		/* storage class */
	int	s_block_level;	/* level of declaration, -1 if not in symbol
				   table */
	type_t	*s_type;
	val_t	s_value;	/* value (if enum or bool constant) */
	union {
		/* XXX: what is the difference to s_type->t_str? */
		struct_or_union	*s_sou_type;
		tspec_t	s_tspec;	/* type (only for keywords) */
		tqual_t	s_qualifier;	/* qualifier (only for keywords) */
		struct	sym *s_old_style_args; /* arguments in old style
				 	* function definitions */
	} u;
	struct	sym *s_symtab_next;	/* next symbol with same hash value */
	struct	sym **s_symtab_ref;	/* pointer to s_symtab_next of the
					 * previous symbol */
	struct	sym *s_next;	/* next struct/union member, enumerator,
				   argument */
	struct	sym *s_level_next;	/* next symbol declared on the same
					 * level */
} sym_t;

/*
 * Used to keep some information about symbols before they are entered
 * into the symbol table.
 */
typedef	struct sbuf {
	const	char *sb_name;		/* name of symbol */
	size_t	sb_len;			/* length (without '\0') */
	sym_t	*sb_sym;		/* symbol table entry */
} sbuf_t;


/*
 * tree node
 */
typedef	struct tnode {
	op_t	tn_op;		/* operator */
	type_t	*tn_type;	/* type */
	bool	tn_lvalue:1;	/* node is lvalue */
	bool	tn_cast:1;	/* if tn_op == CVT, it's an explicit cast */
	bool	tn_parenthesized:1;
	bool	tn_sys:1;	/* in strict bool mode, allow mixture between
				 * bool and scalar, for code from system
				 * headers that may be a mixture between
				 * scalar types and bool
				 */
	bool	tn_system_dependent:1; /* depends on sizeof or offsetof */
	union {
		struct {
			struct	tnode *_tn_left;	/* (left) operand */
			struct	tnode *_tn_right;	/* right operand */
		} tn_s;
		sym_t	*_tn_sym;	/* symbol if op == NAME */
		val_t	*_tn_val;	/* value if op == CON */
		strg_t	*_tn_string;	/* string if op == STRING */
	} tn_u;
} tnode_t;

#define	tn_left		tn_u.tn_s._tn_left
#define tn_right	tn_u.tn_s._tn_right
#define tn_sym		tn_u._tn_sym
#define	tn_val		tn_u._tn_val
#define	tn_string	tn_u._tn_string

struct generic_association {
	type_t *ga_arg;		/* NULL means default or error */
	tnode_t *ga_result;	/* NULL means error */
	struct generic_association *ga_prev;
};

struct array_size {
	bool has_dim;
	int dim;
};

/*
 * For nested declarations there is a stack that holds all information
 * needed for the current level. dcs points to the innermost element of this
 * stack.
 *
 * d_ctx describes the context of the current declaration. Its value is
 * one of
 *	EXTERN		global declarations
 *	MOS or MOU	declarations of struct or union members
 *	CTCONST		declarations of enums or boolean constants
 *	OLD_STYLE_ARG	declaration of arguments in old-style function
 *			definitions
 *	PROTO_ARG	declaration of arguments in function prototypes
 *	AUTO		declaration of local symbols
 *	ABSTRACT	abstract declarations (sizeof, casts)
 *
 */
typedef	struct dinfo {
	tspec_t	d_abstract_type;/* VOID, BOOL, CHAR, INT or COMPLEX */
	tspec_t	d_complex_mod;	/* FLOAT or DOUBLE */
	tspec_t	d_sign_mod;	/* SIGNED or UNSIGN */
	tspec_t	d_rank_mod;	/* SHORT, LONG or QUAD */
	scl_t	d_scl;		/* storage class */
	type_t	*d_type;	/* after end_type() pointer to the type used
				   for all declarators */
	sym_t	*d_redeclared_symbol;
	unsigned int d_offset;	/* offset of next structure member */
	unsigned short d_sou_align_in_bits; /* alignment required for current
				 * structure */
	scl_t	d_ctx;		/* context of declaration */
	bool	d_const:1;	/* const in declaration specifiers */
	bool	d_volatile:1;	/* volatile in declaration specifiers */
	bool	d_inline:1;	/* inline in declaration specifiers */
	bool	d_multiple_storage_classes:1; /* reported in end_type */
	bool	d_invalid_type_combination:1;
	bool	d_nonempty_decl:1; /* if at least one tag is declared
				 * ... in the current function decl. */
	bool	d_vararg:1;
	bool	d_proto:1;	/* current function decl. is prototype */
	bool	d_notyp:1;	/* set if no type specifier was present */
	bool	d_asm:1;	/* set if d_ctx == AUTO and asm() present */
	bool	d_packed:1;
	bool	d_used:1;
	type_t	*d_tagtyp;	/* tag during member declaration */
	sym_t	*d_func_args;	/* list of arguments during function def. */
	pos_t	d_func_def_pos;	/* position of function definition */
	sym_t	*d_dlsyms;	/* first symbol declared at this level */
	sym_t	**d_ldlsym;	/* points to s_level_next in the last symbol
				   declaration at this level */
	sym_t	*d_func_proto_syms; /* symbols defined in prototype */
	struct dinfo *d_enclosing; /* the enclosing declaration level */
} dinfo_t;

/* One level of pointer indirection in declarators, including qualifiers. */
typedef	struct qual_ptr {
	bool	p_const: 1;
	bool	p_volatile: 1;
	bool	p_pointer: 1;
	struct	qual_ptr *p_next;
} qual_ptr;

/*
 * The values of the 'case' labels, linked via cl_next in reverse order of
 * appearance in the code, that is from bottom to top.
 */
typedef	struct case_label {
	val_t	cl_val;
	struct case_label *cl_next;
} case_label_t;

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
	bool	c_loop:1;		/* 'continue' and 'break' are valid */
	bool	c_switch:1;		/* 'case' and 'break' are valid */
	bool	c_break:1;		/* the loop/switch has a reachable
					 * 'break' statement */
	bool	c_continue:1;		/* the loop has a reachable 'continue'
					 * statement */
	bool	c_default:1;		/* the switch has a 'default' label */
	bool	c_maybe_endless:1;	/* the controlling expression is
					 * always true (as in 'for (;;)' or
					 * 'while (1)'), there may be break
					 * statements though */
	bool	c_always_then:1;
	bool	c_reached_end_of_then:1;
	bool	c_had_return_noval:1;	/* had "return;" */
	bool	c_had_return_value:1;	/* had "return expr;" */

	type_t	*c_switch_type;		/* type of switch expression */
	tnode_t	*c_switch_expr;
	case_label_t *c_case_labels;	/* list of case values */

	struct	memory_block *c_for_expr3_mem; /* saved memory for end of loop
					 * expression in for() */
	tnode_t	*c_for_expr3;		/* end of loop expr in for() */
	pos_t	c_for_expr3_pos;	/* position of end of loop expr */
	pos_t	c_for_expr3_csrc_pos;	/* same for csrc_pos */

	struct control_statement *c_surrounding;
} control_statement;

typedef struct {
	size_t lo;			/* inclusive */
	size_t hi;			/* inclusive */
} range_t;

#include "externs1.h"

#define	ERR_SETSIZE	1024
#define __NERRBITS (sizeof(unsigned int))

typedef	struct err_set {
	unsigned int	errs_bits[(ERR_SETSIZE + __NERRBITS-1) / __NERRBITS];
} err_set;

#define	ERR_SET(n, p)	\
	((p)->errs_bits[(n)/__NERRBITS] |= (1 << ((n) % __NERRBITS)))
#define	ERR_CLR(n, p)	\
	((p)->errs_bits[(n)/__NERRBITS] &= ~(1 << ((n) % __NERRBITS)))
#define	ERR_ISSET(n, p)	\
	(((p)->errs_bits[(n)/__NERRBITS] & (1 << ((n) % __NERRBITS))) != 0)
#define	ERR_ZERO(p)	(void)memset((p), 0, sizeof(*(p)))

#define INTERNAL_ERROR(fmt, args...) \
	internal_error(__FILE__, __LINE__, fmt, ##args)

#define lint_assert(cond)						\
	do {								\
		if (!(cond))						\
			assert_failed(__FILE__, __LINE__, __func__, #cond); \
	} while (false)

extern err_set	msgset;


#ifdef DEBUG
#  include "err-msgs.h"

/* ARGSUSED */
static inline void __attribute__((format(printf, 1, 2)))
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

#  define wrap_check_printf(func, msgid, args...)			\
	do {								\
		check_printf(__CONCAT(MSG_, msgid), ##args);		\
		(func)(msgid, ##args);					\
	} while (false)

#  define error(msgid, args...) wrap_check_printf(error, msgid, ##args)
#  define warning(msgid, args...) wrap_check_printf(warning, msgid, ##args)
#  define gnuism(msgid, args...) wrap_check_printf(gnuism, msgid, ##args)
#  define c99ism(msgid, args...) wrap_check_printf(c99ism, msgid, ##args)
#  define c11ism(msgid, args...) wrap_check_printf(c11ism, msgid, ##args)
#endif

static inline bool
is_nonzero_val(const val_t *val)
{
	return is_floating(val->v_tspec)
	    ? val->v_ldbl != 0.0
	    : val->v_quad != 0;
}

static inline bool
constant_is_nonzero(const tnode_t *tn)
{
	lint_assert(tn->tn_op == CON);
	lint_assert(tn->tn_type->t_tspec == tn->tn_val->v_tspec);
	return is_nonzero_val(tn->tn_val);
}

static inline bool
is_zero(const tnode_t *tn)
{
	return tn != NULL && tn->tn_op == CON && !is_nonzero_val(tn->tn_val);
}

static inline bool
is_nonzero(const tnode_t *tn)
{
	return tn != NULL && tn->tn_op == CON && is_nonzero_val(tn->tn_val);
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
	 * TODO: Add proper support for INT128.
	 * This involves changing val_t to 128 bits.
	 */
	if (i >= 64)
		return 0;	/* XXX: not correct for INT128 and UINT128 */

	lint_assert(i < 64);
	return (uint64_t)1 << i;
}

static inline bool
msb(int64_t q, tspec_t t)
{
	return (q & bit((unsigned int)size_in_bits(t) - 1)) != 0;
}

static inline uint64_t
value_bits(unsigned bitsize)
{
	lint_assert(bitsize > 0);

	/* for long double (80 or 128), double _Complex (128) */
	/*
	 * XXX: double _Complex does not have 128 bits of precision,
	 * therefore it should never be necessary to query the value bits
	 * of such a type; see d_c99_complex_split.c to trigger this case.
	 */
	if (bitsize >= 64)
		return ~((uint64_t)0);

	return ~(~(uint64_t)0 << bitsize);
}

/* C99 6.7.8p7 */
static inline bool
is_struct_or_union(tspec_t t)
{
	return t == STRUCT || t == UNION;
}
