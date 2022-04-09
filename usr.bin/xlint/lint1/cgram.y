%{
/* $NetBSD: cgram.y,v 1.392 2022/04/09 21:19:52 rillig Exp $ */

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

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: cgram.y,v 1.392 2022/04/09 21:19:52 rillig Exp $");
#endif

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"

extern char *yytext;

/*
 * Contains the level of current declaration, used for symbol table entries.
 * 0 is the top-level, > 0 is inside a function body.
 */
int	block_level;

/*
 * level for memory allocation. Normally the same as block_level.
 * An exception is the declaration of arguments in prototypes. Memory
 * for these can't be freed after the declaration, but symbols must
 * be removed from the symbol table after the declaration.
 */
size_t	mem_block_level;

/*
 * Save the no-warns state and restore it to avoid the problem where
 * if (expr) { stmt } / * NOLINT * / stmt;
 */
static int olwarn = LWARN_BAD;

static	void	cgram_declare(sym_t *, bool, sbuf_t *);
static	void	read_until_rparen(void);
static	sym_t	*symbolrename(sym_t *, sbuf_t *);


#ifdef DEBUG
static void
CLEAR_WARN_FLAGS(const char *file, size_t line)
{
	debug_step("%s:%zu: clearing flags", file, line);
	clear_warn_flags();
	olwarn = LWARN_BAD;
}

static void
SAVE_WARN_FLAGS(const char *file, size_t line)
{
	lint_assert(olwarn == LWARN_BAD);
	debug_step("%s:%zu: saving flags %d", file, line, lwarn);
	olwarn = lwarn;
}

static void
RESTORE_WARN_FLAGS(const char *file, size_t line)
{
	if (olwarn != LWARN_BAD) {
		lwarn = olwarn;
		debug_step("%s:%zu: restoring flags %d", file, line, lwarn);
		olwarn = LWARN_BAD;
	} else
		CLEAR_WARN_FLAGS(file, line);
}
#else
#define CLEAR_WARN_FLAGS(f, l)	clear_warn_flags(), olwarn = LWARN_BAD
#define SAVE_WARN_FLAGS(f, l)	olwarn = lwarn
#define RESTORE_WARN_FLAGS(f, l) \
	(void)(olwarn == LWARN_BAD ? (clear_warn_flags(), 0) : (lwarn = olwarn))
#endif

#define clear_warning_flags()	CLEAR_WARN_FLAGS(__FILE__, __LINE__)
#define save_warning_flags()	SAVE_WARN_FLAGS(__FILE__, __LINE__)
#define restore_warning_flags()	RESTORE_WARN_FLAGS(__FILE__, __LINE__)

/* unbind the anonymous struct members from the struct */
static void
anonymize(sym_t *s)
{
	for ( ; s != NULL; s = s->s_next)
		s->u.s_member.sm_sou_type = NULL;
}

#if defined(YYDEBUG) && (defined(YYBYACC) || defined(YYBISON))
#define YYSTYPE_TOSTRING cgram_to_string
#endif
#if defined(YYDEBUG) && defined(YYBISON)
#define YYPRINT cgram_print
#endif

%}

%expect 150

%union {
	val_t	*y_val;
	sbuf_t	*y_name;
	sym_t	*y_sym;
	op_t	y_op;
	scl_t	y_scl;
	tspec_t	y_tspec;
	tqual_t	y_tqual;
	type_t	*y_type;
	tnode_t	*y_tnode;
	range_t	y_range;
	strg_t	*y_string;
	qual_ptr *y_qual_ptr;
	bool	y_seen_statement;
	struct generic_association *y_generic;
	struct array_size y_array_size;
	bool	y_in_system_header;
};

%token			T_LBRACE T_RBRACE T_LBRACK T_RBRACK T_LPAREN T_RPAREN
%token			T_POINT T_ARROW
%token			T_COMPLEMENT T_LOGNOT
%token	<y_op>		T_INCDEC
%token			T_SIZEOF
%token			T_BUILTIN_OFFSETOF
%token			T_TYPEOF
%token			T_EXTENSION
%token			T_ALIGNAS
%token			T_ALIGNOF
%token			T_ASTERISK
%token	<y_op>		T_MULTIPLICATIVE
%token	<y_op>		T_ADDITIVE
%token	<y_op>		T_SHIFT
%token	<y_op>		T_RELATIONAL
%token	<y_op>		T_EQUALITY
%token			T_AMPER
%token			T_BITXOR
%token			T_BITOR
%token			T_LOGAND
%token			T_LOGOR
%token			T_QUEST
%token			T_COLON
%token			T_ASSIGN
%token	<y_op>		T_OPASSIGN
%token			T_COMMA
%token			T_SEMI
%token			T_ELLIPSIS
%token			T_REAL
%token			T_IMAG
%token			T_GENERIC
%token			T_NORETURN

/* storage classes (extern, static, auto, register and typedef) */
%token	<y_scl>		T_SCLASS

/*
 * predefined type keywords (char, int, short, long, unsigned, signed,
 * float, double, void); see T_TYPENAME for types from typedef
 */
%token	<y_tspec>	T_TYPE

/* qualifiers (const, volatile, restrict, _Thread_local) */
%token	<y_tqual>	T_QUAL

/* struct or union */
%token	<y_tspec>	T_STRUCT_OR_UNION

/* remaining keywords */
%token			T_ASM
%token			T_BREAK
%token			T_CASE
%token			T_CONTINUE
%token			T_DEFAULT
%token			T_DO
%token			T_ELSE
%token			T_ENUM
%token			T_FOR
%token			T_GOTO
%token			T_IF
%token			T_PACKED
%token			T_RETURN
%token			T_SWITCH
%token			T_SYMBOLRENAME
%token			T_WHILE
%token			T_STATIC_ASSERT

%token			T_ATTRIBUTE
%token			T_AT_ALIAS
%token			T_AT_ALIGNED
%token			T_AT_ALLOC_SIZE
%token			T_AT_ALWAYS_INLINE
%token			T_AT_BOUNDED
%token			T_AT_BUFFER
%token			T_AT_COLD
%token			T_AT_COMMON
%token			T_AT_CONSTRUCTOR
%token			T_AT_DEPRECATED
%token			T_AT_DESTRUCTOR
%token			T_AT_DISABLE_SANITIZER_INSTRUMENTATION
%token			T_AT_FALLTHROUGH
%token			T_AT_FORMAT
%token			T_AT_FORMAT_ARG
%token			T_AT_FORMAT_GNU_PRINTF
%token			T_AT_FORMAT_PRINTF
%token			T_AT_FORMAT_SCANF
%token			T_AT_FORMAT_STRFMON
%token			T_AT_FORMAT_STRFTIME
%token			T_AT_FORMAT_SYSLOG
%token			T_AT_GNU_INLINE
%token			T_AT_HOT
%token			T_AT_MALLOC
%token			T_AT_MAY_ALIAS
%token			T_AT_MINBYTES
%token			T_AT_MODE
%token			T_AT_NO_SANITIZE
%token			T_AT_NO_SANITIZE_THREAD
%token			T_AT_NOINLINE
%token			T_AT_NONNULL
%token			T_AT_NONSTRING
%token			T_AT_NORETURN
%token			T_AT_NOTHROW
%token			T_AT_NO_INSTRUMENT_FUNCTION
%token			T_AT_OPTIMIZE
%token			T_AT_OPTNONE
%token			T_AT_PACKED
%token			T_AT_PCS
%token			T_AT_PURE
%token			T_AT_REGPARM
%token			T_AT_RETURNS_NONNULL
%token			T_AT_RETURNS_TWICE
%token			T_AT_SECTION
%token			T_AT_SENTINEL
%token			T_AT_STRING
%token			T_AT_TARGET
%token			T_AT_TLS_MODEL
%token			T_AT_TUNION
%token			T_AT_UNUSED
%token			T_AT_USED
%token			T_AT_VISIBILITY
%token			T_AT_WARN_UNUSED_RESULT
%token			T_AT_WEAK

%left	T_THEN
%left	T_ELSE
%right	T_QUEST T_COLON
%left	T_LOGOR
%left	T_LOGAND
%left	T_BITOR
%left	T_BITXOR
%left	T_AMPER
%left	T_EQUALITY
%left	T_RELATIONAL
%left	T_SHIFT
%left	T_ADDITIVE
%left	T_ASTERISK T_MULTIPLICATIVE

%token	<y_name>	T_NAME
%token	<y_name>	T_TYPENAME
%token	<y_val>		T_CON
%token	<y_string>	T_STRING

%type	<y_sym>		identifier_sym
%type	<y_name>	identifier
%type	<y_string>	string

%type	<y_tnode>	primary_expression
%type	<y_tnode>	generic_selection
%type	<y_generic>	generic_assoc_list
%type	<y_generic>	generic_association
%type	<y_tnode>	postfix_expression
%type	<y_tnode>	gcc_statement_expr_list
%type	<y_tnode>	gcc_statement_expr_item
%type	<y_op>		point_or_arrow
%type	<y_tnode>	argument_expression_list
%type	<y_tnode>	unary_expression
%type	<y_tnode>	cast_expression
%type	<y_tnode>	conditional_expression
%type	<y_tnode>	assignment_expression
%type	<y_tnode>	expression_opt
%type	<y_tnode>	expression
%type	<y_tnode>	constant_expr

%type	<y_type>	begin_type_typespec
%type	<y_type>	type_specifier
%type	<y_type>	notype_type_specifier
%type	<y_type>	struct_or_union_specifier
%type	<y_tspec>	struct_or_union
%type	<y_sym>		braced_struct_declaration_list
%type	<y_sym>		struct_declaration_list_with_rbrace
%type	<y_sym>		struct_declaration_list
%type	<y_sym>		struct_declaration
%type	<y_sym>		notype_struct_declarators
%type	<y_sym>		type_struct_declarators
%type	<y_sym>		notype_struct_declarator
%type	<y_sym>		type_struct_declarator
%type	<y_type>	enum_specifier
%type	<y_sym>		enum_declaration
%type	<y_sym>		enums_with_opt_comma
%type	<y_sym>		enumerator_list
%type	<y_sym>		enumerator
%type	<y_qual_ptr>	type_qualifier
%type	<y_qual_ptr>	pointer
%type	<y_qual_ptr>	asterisk
%type	<y_qual_ptr>	type_qualifier_list_opt
%type	<y_qual_ptr>	type_qualifier_list
%type	<y_sym>		notype_declarator
%type	<y_sym>		type_declarator
%type	<y_sym>		notype_direct_declarator
%type	<y_sym>		type_direct_declarator
%type	<y_sym>		type_param_declarator
%type	<y_sym>		notype_param_declarator
%type	<y_sym>		direct_param_declarator
%type	<y_sym>		direct_notype_param_declarator
%type	<y_sym>		param_list
%type	<y_array_size>	array_size_opt
%type	<y_tnode>	array_size
%type	<y_sym>		identifier_list
%type	<y_type>	type_name
%type	<y_sym>		abstract_declaration
%type	<y_sym>		abstract_declarator
%type	<y_sym>		direct_abstract_declarator
%type	<y_sym>		abstract_decl_param_list
%type	<y_sym>		vararg_parameter_type_list
%type	<y_sym>		parameter_type_list
%type	<y_sym>		parameter_declaration
%type	<y_range>	range
%type	<y_name>	asm_or_symbolrename_opt

%type	<y_seen_statement> block_item_list
%type	<y_seen_statement> block_item
%type	<y_tnode>	do_while_expr
%type	<y_sym>		func_declarator
%type	<y_in_system_header> sys

%{
#if defined(YYDEBUG) && defined(YYBISON)
static inline void cgram_print(FILE *, int, YYSTYPE);
#endif
%}

%%

program:
	  /* empty */ {
		if (sflag) {
			/* empty translation unit */
			error(272);
		} else if (!tflag) {
			/* empty translation unit */
			warning(272);
		}
	  }
	| translation_unit
	;

identifier_sym:			/* helper for struct/union/enum */
	  identifier {
		$$ = getsym($1);
	  }
	;

/* K&R ???, C90 ???, C99 6.4.2.1, C11 ??? */
identifier:
	  T_NAME {
		debug_step("cgram: name '%s'", $1->sb_name);
		$$ = $1;
	  }
	| T_TYPENAME {
		debug_step("cgram: typename '%s'", $1->sb_name);
		$$ = $1;
	  }
	;

/* see C99 6.4.5, string literals are joined by 5.1.1.2 */
string:
	  T_STRING
	| string T_STRING {
		if (tflag) {
			/* concatenated strings are illegal in traditional C */
			warning(219);
		}
		$$ = cat_strings($1, $2);
	  }
	;

/* K&R 7.1, C90 ???, C99 6.5.1, C11 6.5.1 */
primary_expression:
	  T_NAME {
	  	bool sys_name, sys_next;
		sys_name = in_system_header;
		if (yychar < 0)
			yychar = yylex();
		sys_next = in_system_header;
		in_system_header = sys_name;
		$$ = build_name(getsym($1), yychar == T_LPAREN);
		in_system_header = sys_next;
	  }
	| T_CON {
		$$ = build_constant(gettyp($1->v_tspec), $1);
	  }
	| string {
		$$ = build_string($1);
	  }
	| T_LPAREN expression T_RPAREN {
		if ($2 != NULL)
			$2->tn_parenthesized = true;
		$$ = $2;
	  }
	| generic_selection
	/* GCC primary-expression, see c_parser_postfix_expression */
	| T_BUILTIN_OFFSETOF T_LPAREN type_name T_COMMA identifier T_RPAREN {
		symtyp = FMEMBER;
		$$ = build_offsetof($3, getsym($5));
	  }
	;

/* K&R ---, C90 ---, C99 ---, C11 6.5.1.1 */
generic_selection:
	  T_GENERIC T_LPAREN assignment_expression T_COMMA
	    generic_assoc_list T_RPAREN {
		/* generic selection requires C11 or later */
		c11ism(345);
		$$ = build_generic_selection($3, $5);
	  }
	;

/* K&R ---, C90 ---, C99 ---, C11 6.5.1.1 */
generic_assoc_list:
	  generic_association
	| generic_assoc_list T_COMMA generic_association {
		$3->ga_prev = $1;
		$$ = $3;
	  }
	;

/* K&R ---, C90 ---, C99 ---, C11 6.5.1.1 */
generic_association:
	  type_name T_COLON assignment_expression {
		$$ = block_zero_alloc(sizeof(*$$));
		$$->ga_arg = $1;
		$$->ga_result = $3;
	  }
	| T_DEFAULT T_COLON assignment_expression {
		$$ = block_zero_alloc(sizeof(*$$));
		$$->ga_arg = NULL;
		$$->ga_result = $3;
	  }
	;

/* K&R 7.1, C90 ???, C99 6.5.2, C11 6.5.2 */
postfix_expression:
	  primary_expression
	| postfix_expression T_LBRACK sys expression T_RBRACK {
		$$ = build_unary(INDIR, $3, build_binary($1, PLUS, $3, $4));
	  }
	| postfix_expression T_LPAREN sys T_RPAREN {
		$$ = build_function_call($1, $3, NULL);
	  }
	| postfix_expression T_LPAREN sys argument_expression_list T_RPAREN {
		$$ = build_function_call($1, $3, $4);
	  }
	| postfix_expression point_or_arrow sys T_NAME {
		$$ = build_member_access($1, $2, $3, $4);
	  }
	| postfix_expression T_INCDEC sys {
		$$ = build_unary($2 == INC ? INCAFT : DECAFT, $3, $1);
	  }
	| T_LPAREN type_name T_RPAREN {	/* C99 6.5.2.5 "Compound literals" */
		sym_t *tmp = mktempsym($2);
		begin_initialization(tmp);
		cgram_declare(tmp, true, NULL);
	  } init_lbrace initializer_list comma_opt init_rbrace {
		if (!Sflag)
			 /* compound literals are a C99/GCC extension */
			 gnuism(319);
		$$ = build_name(*current_initsym(), false);
		end_initialization();
	  }
	| T_LPAREN compound_statement_lbrace {
		begin_statement_expr();
	  } gcc_statement_expr_list {
		do_statement_expr($4);
	  } compound_statement_rbrace T_RPAREN {
		$$ = end_statement_expr();
	  }
	;

comma_opt:			/* helper for 'postfix_expression' */
	  /* empty */
	| T_COMMA
	;

/*
 * The inner part of a GCC statement-expression of the form ({ ... }).
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
 */
gcc_statement_expr_list:
	  gcc_statement_expr_item
	| gcc_statement_expr_list gcc_statement_expr_item {
		$$ = $2;
	  }
	;

gcc_statement_expr_item:
	  declaration_or_error {
		clear_warning_flags();
		$$ = NULL;
	  }
	| non_expr_statement {
		$$ = expr_alloc_tnode();
		$$->tn_type = gettyp(VOID);
	  }
	| expression T_SEMI {
		if ($1 == NULL) {	/* in case of syntax errors */
			$$ = expr_alloc_tnode();
			$$->tn_type = gettyp(VOID);
		} else {
			/* XXX: do that only on the last name */
			if ($1->tn_op == NAME)
				$1->tn_sym->s_used = true;
			expr($1, false, false, false, false);
			seen_fallthrough = false;
			$$ = $1;
		}
	  }
	;

point_or_arrow:			/* helper for 'postfix_expression' */
	  T_POINT {
		symtyp = FMEMBER;
		$$ = POINT;
	  }
	| T_ARROW {
		symtyp = FMEMBER;
		$$ = ARROW;
	  }
	;

/* K&R 7.1, C90 ???, C99 6.5.2, C11 6.5.2 */
argument_expression_list:
	  assignment_expression {
		$$ = build_function_argument(NULL, $1);
	  }
	| argument_expression_list T_COMMA assignment_expression {
		$$ = build_function_argument($1, $3);
	  }
	;

/* K&R 7.2, C90 ???, C99 6.5.3, C11 6.5.3 */
unary_expression:
	  postfix_expression
	| T_INCDEC sys unary_expression {
		$$ = build_unary($1 == INC ? INCBEF : DECBEF, $2, $3);
	  }
	| T_AMPER sys cast_expression {
		$$ = build_unary(ADDR, $2, $3);
	  }
	| T_ASTERISK sys cast_expression {
		$$ = build_unary(INDIR, $2, $3);
	  }
	| T_ADDITIVE sys cast_expression {
		if (tflag && $1 == PLUS) {
			/* unary + is illegal in traditional C */
			warning(100);
		}
		$$ = build_unary($1 == PLUS ? UPLUS : UMINUS, $2, $3);
	  }
	| T_COMPLEMENT sys cast_expression {
		$$ = build_unary(COMPL, $2, $3);
	  }
	| T_LOGNOT sys cast_expression {
		$$ = build_unary(NOT, $2, $3);
	  }
	| T_REAL sys cast_expression {	/* GCC c_parser_unary_expression */
		$$ = build_unary(REAL, $2, $3);
	  }
	| T_IMAG sys cast_expression {	/* GCC c_parser_unary_expression */
		$$ = build_unary(IMAG, $2, $3);
	  }
	| T_EXTENSION cast_expression {	/* GCC c_parser_unary_expression */
		$$ = $2;
	  }
	| T_SIZEOF unary_expression {
		$$ = $2 == NULL ? NULL : build_sizeof($2->tn_type);
		if ($$ != NULL)
			check_expr_misc($2,
			    false, false, false, false, false, true);
	  }
	| T_SIZEOF T_LPAREN type_name T_RPAREN {
		$$ = build_sizeof($3);
	  }
	/* K&R ---, C90 ---, C99 ---, C11 6.5.3 */
	| T_ALIGNOF T_LPAREN type_name T_RPAREN {
		/* TODO: c11ism */
		$$ = build_alignof($3);
	  }
	;

/* The rule 'unary_operator' is inlined into unary_expression. */

/* K&R 7.2, C90 ???, C99 6.5.4, C11 6.5.4 */
cast_expression:
	  unary_expression
	| T_LPAREN type_name T_RPAREN cast_expression {
		$$ = cast($4, $2);
	  }
	;

expression_opt:
	  /* empty */ {
		$$ = NULL;
	  }
	| expression
	;

/* 'conditional_expression' also implements 'multiplicative_expression'. */
/* 'conditional_expression' also implements 'additive_expression'. */
/* 'conditional_expression' also implements 'shift_expression'. */
/* 'conditional_expression' also implements 'relational_expression'. */
/* 'conditional_expression' also implements 'equality_expression'. */
/* 'conditional_expression' also implements 'AND_expression'. */
/* 'conditional_expression' also implements 'exclusive_OR_expression'. */
/* 'conditional_expression' also implements 'inclusive_OR_expression'. */
/* 'conditional_expression' also implements 'logical_AND_expression'. */
/* 'conditional_expression' also implements 'logical_OR_expression'. */
/* K&R ???, C90 ???, C99 6.5.5 to 6.5.15, C11 6.5.5 to 6.5.15 */
conditional_expression:
	  cast_expression
	| conditional_expression T_ASTERISK sys conditional_expression {
		$$ = build_binary($1, MULT, $3, $4);
	  }
	| conditional_expression T_MULTIPLICATIVE sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	| conditional_expression T_ADDITIVE sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	| conditional_expression T_SHIFT sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	| conditional_expression T_RELATIONAL sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	| conditional_expression T_EQUALITY sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	| conditional_expression T_AMPER sys conditional_expression {
		$$ = build_binary($1, BITAND, $3, $4);
	  }
	| conditional_expression T_BITXOR sys conditional_expression {
		$$ = build_binary($1, BITXOR, $3, $4);
	  }
	| conditional_expression T_BITOR sys conditional_expression {
		$$ = build_binary($1, BITOR, $3, $4);
	  }
	| conditional_expression T_LOGAND sys conditional_expression {
		$$ = build_binary($1, LOGAND, $3, $4);
	  }
	| conditional_expression T_LOGOR sys conditional_expression {
		$$ = build_binary($1, LOGOR, $3, $4);
	  }
	| conditional_expression T_QUEST sys
	    expression T_COLON sys conditional_expression {
		$$ = build_binary($1, QUEST, $3,
		    build_binary($4, COLON, $6, $7));
	  }
	;

/* K&R ???, C90 ???, C99 6.5.16, C11 6.5.16 */
assignment_expression:
	  conditional_expression
	| unary_expression T_ASSIGN sys assignment_expression {
		$$ = build_binary($1, ASSIGN, $3, $4);
	  }
	| unary_expression T_OPASSIGN sys assignment_expression {
		$$ = build_binary($1, $2, $3, $4);
	  }
	;

/* K&R ???, C90 ???, C99 6.5.17, C11 6.5.17 */
expression:
	  assignment_expression
	| expression T_COMMA sys assignment_expression {
		$$ = build_binary($1, COMMA, $3, $4);
	  }
	;

constant_expr_list_opt:		/* helper for gcc_attribute */
	  /* empty */
	| constant_expr_list
	;

constant_expr_list:		/* helper for gcc_attribute */
	  constant_expr
	| constant_expr_list T_COMMA constant_expr
	;

constant_expr:			/* C99 6.6 */
	  conditional_expression
	;

declaration_or_error:
	  declaration
	| error T_SEMI
	;

declaration:			/* C99 6.7 */
	  begin_type_declmods end_type T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else {
			/* empty declaration */
			warning(2);
		}
	  }
	| begin_type_declmods end_type notype_init_declarators T_SEMI
	/* ^^ There is no check for the missing type-specifier. */
	| begin_type_declaration_specifiers end_type T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else if (!dcs->d_nonempty_decl) {
			/* empty declaration */
			warning(2);
		}
	  }
	| begin_type_declaration_specifiers end_type
	    type_init_declarators T_SEMI
	| static_assert_declaration
	;

begin_type_declaration_specifiers:	/* see C99 6.7 */
	  begin_type_typespec {
		add_type($1);
	  }
	| begin_type_declmods type_specifier {
		add_type($2);
	  }
	| type_attribute begin_type_declaration_specifiers
	| begin_type_declaration_specifiers declmod
	| begin_type_declaration_specifiers notype_type_specifier {
		add_type($2);
	  }
	;

begin_type_declmods:		/* see C99 6.7 */
	  begin_type T_QUAL {
		add_qualifier($2);
	  }
	| begin_type T_SCLASS {
		add_storage_class($2);
	  }
	| begin_type_declmods declmod
	;

begin_type_specifier_qualifier_list:	/* see C11 6.7.2.1 */
	  begin_type_specifier_qualifier_list_postfix
	| type_attribute_list begin_type_specifier_qualifier_list_postfix
	;

begin_type_specifier_qualifier_list_postfix:
	  begin_type_typespec {
		add_type($1);
	  }
	| begin_type_qualifier_list type_specifier {
		add_type($2);
	  }
	| begin_type_specifier_qualifier_list_postfix T_QUAL {
		add_qualifier($2);
	  }
	| begin_type_specifier_qualifier_list_postfix notype_type_specifier {
		add_type($2);
	  }
	| begin_type_specifier_qualifier_list_postfix type_attribute
	;

begin_type_typespec:
	  begin_type notype_type_specifier {
		$$ = $2;
	  }
	| T_TYPENAME begin_type {
		$$ = getsym($1)->s_type;
	  }
	;

begin_type_qualifier_list:
	  begin_type T_QUAL {
		add_qualifier($2);
	  }
	| begin_type_qualifier_list T_QUAL {
		add_qualifier($2);
	  }
	;

declmod:
	  T_QUAL {
		add_qualifier($1);
	  }
	| T_SCLASS {
		add_storage_class($1);
	  }
	| type_attribute_list
	;

type_attribute_list:
	  type_attribute
	| type_attribute_list type_attribute
	;

type_attribute_opt:
	  /* empty */
	| type_attribute
	;

type_attribute:			/* See C11 6.7 declaration-specifiers */
	  gcc_attribute
	| T_ALIGNAS T_LPAREN type_specifier T_RPAREN	/* C11 6.7.5 */
	| T_ALIGNAS T_LPAREN constant_expr T_RPAREN	/* C11 6.7.5 */
	| T_PACKED {
		addpacked();
	  }
	| T_NORETURN
	;

begin_type:
	  /* empty */ {
		begin_type();
	  }
	;

end_type:
	  /* empty */ {
		end_type();
	  }
	;

type_specifier:			/* C99 6.7.2 */
	  notype_type_specifier
	| T_TYPENAME {
		$$ = getsym($1)->s_type;
	  }
	;

notype_type_specifier:		/* see C99 6.7.2 */
	  T_TYPE {
		$$ = gettyp($1);
	  }
	| T_TYPEOF T_LPAREN expression T_RPAREN {	/* GCC extension */
		$$ = $3->tn_type;
	  }
	| struct_or_union_specifier {
		end_declaration_level();
		$$ = $1;
	  }
	| enum_specifier {
		end_declaration_level();
		$$ = $1;
	  }
	;

struct_or_union_specifier:	/* C99 6.7.2.1 */
	  struct_or_union identifier_sym {
		/*
		 * STDC requires that "struct a;" always introduces
		 * a new tag if "a" is not declared at current level
		 *
		 * yychar is valid because otherwise the parser would not
		 * have been able to decide if it must shift or reduce
		 */
		$$ = mktag($2, $1, false, yychar == T_SEMI);
	  }
	| struct_or_union identifier_sym {
		dcs->d_tagtyp = mktag($2, $1, true, false);
	  } braced_struct_declaration_list {
		$$ = complete_tag_struct_or_union(dcs->d_tagtyp, $4);
	  }
	| struct_or_union {
		dcs->d_tagtyp = mktag(NULL, $1, true, false);
	  } braced_struct_declaration_list {
		$$ = complete_tag_struct_or_union(dcs->d_tagtyp, $3);
	  }
	| struct_or_union error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

struct_or_union:		/* C99 6.7.2.1 */
	  T_STRUCT_OR_UNION {
		symtyp = FTAG;
		begin_declaration_level($1 == STRUCT ? MOS : MOU);
		dcs->d_offset_in_bits = 0;
		dcs->d_sou_align_in_bits = CHAR_SIZE;
		$$ = $1;
	  }
	| struct_or_union type_attribute
	;

braced_struct_declaration_list:	/* see C99 6.7.2.1 */
	  struct_declaration_lbrace struct_declaration_list_with_rbrace {
		$$ = $2;
	  }
	;

struct_declaration_lbrace:	/* see C99 6.7.2.1 */
	  T_LBRACE {
		symtyp = FVFT;
	  }
	;

struct_declaration_list_with_rbrace:	/* see C99 6.7.2.1 */
	  struct_declaration_list T_RBRACE
	| T_RBRACE {
		/* XXX: This is not allowed by any C standard. */
		$$ = NULL;
	  }
	;

struct_declaration_list:	/* C99 6.7.2.1 */
	  struct_declaration
	| struct_declaration_list struct_declaration {
		$$ = lnklst($1, $2);
	  }
	;

struct_declaration:		/* C99 6.7.2.1 */
	  begin_type_qualifier_list end_type {
		/* ^^ There is no check for the missing type-specifier. */
		/* too late, i know, but getsym() compensates it */
		symtyp = FMEMBER;
	  } notype_struct_declarators type_attribute_opt T_SEMI {
		symtyp = FVFT;
		$$ = $4;
	  }
	| begin_type_specifier_qualifier_list end_type {
		symtyp = FMEMBER;
	  } type_struct_declarators type_attribute_opt T_SEMI {
		symtyp = FVFT;
		$$ = $4;
	  }
	| begin_type_qualifier_list end_type type_attribute_opt T_SEMI {
		/* syntax error '%s' */
		error(249, "member without type");
		$$ = NULL;
	  }
	| begin_type_specifier_qualifier_list end_type type_attribute_opt
	    T_SEMI {
		symtyp = FVFT;
		if (!Sflag)
			/* anonymous struct/union members is a C11 feature */
			warning(49);
		if (is_struct_or_union(dcs->d_type->t_tspec)) {
			$$ = dcs->d_type->t_str->sou_first_member;
			/* add all the members of the anonymous struct/union */
			anonymize($$);
		} else {
			/* syntax error '%s' */
			error(249, "unnamed member");
			$$ = NULL;
		}
	  }
	| static_assert_declaration {
		$$ = NULL;
	  }
	| error T_SEMI {
		symtyp = FVFT;
		$$ = NULL;
	  }
	;

notype_struct_declarators:
	  notype_struct_declarator {
		$$ = declarator_1_struct_union($1);
	  }
	| notype_struct_declarators {
		symtyp = FMEMBER;
	  } T_COMMA type_struct_declarator {
		$$ = lnklst($1, declarator_1_struct_union($4));
	  }
	;

type_struct_declarators:
	  type_struct_declarator {
		$$ = declarator_1_struct_union($1);
	  }
	| type_struct_declarators {
		symtyp = FMEMBER;
	  } T_COMMA type_struct_declarator {
		$$ = lnklst($1, declarator_1_struct_union($4));
	  }
	;

notype_struct_declarator:
	  notype_declarator
	| notype_declarator T_COLON constant_expr {	/* C99 6.7.2.1 */
		$$ = bitfield($1, to_int_constant($3, true));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant_expr {			/* C99 6.7.2.1 */
		$$ = bitfield(NULL, to_int_constant($3, true));
	  }
	;

type_struct_declarator:
	  type_declarator
	| type_declarator T_COLON constant_expr {
		$$ = bitfield($1, to_int_constant($3, true));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant_expr {
		$$ = bitfield(NULL, to_int_constant($3, true));
	  }
	;

enum_specifier:			/* C99 6.7.2.2 */
	  enum gcc_attribute_list_opt identifier_sym {
		$$ = mktag($3, ENUM, false, false);
	  }
	| enum gcc_attribute_list_opt identifier_sym {
		dcs->d_tagtyp = mktag($3, ENUM, true, false);
	  } enum_declaration /*gcc_attribute_list_opt*/ {
		$$ = complete_tag_enum(dcs->d_tagtyp, $5);
	  }
	| enum gcc_attribute_list_opt {
		dcs->d_tagtyp = mktag(NULL, ENUM, true, false);
	  } enum_declaration /*gcc_attribute_list_opt*/ {
		$$ = complete_tag_enum(dcs->d_tagtyp, $4);
	  }
	| enum error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

enum:				/* helper for C99 6.7.2.2 */
	  T_ENUM {
		symtyp = FTAG;
		begin_declaration_level(ENUM_CONST);
	  }
	;

enum_declaration:		/* helper for C99 6.7.2.2 */
	  enum_decl_lbrace enums_with_opt_comma T_RBRACE {
		$$ = $2;
	  }
	;

enum_decl_lbrace:		/* helper for C99 6.7.2.2 */
	  T_LBRACE {
		symtyp = FVFT;
		enumval = 0;
	  }
	;

enums_with_opt_comma:		/* helper for C99 6.7.2.2 */
	  enumerator_list
	| enumerator_list T_COMMA {
		if (sflag) {
			/* trailing ',' prohibited in enum declaration */
			error(54);
		} else {
			/* trailing ',' prohibited in enum declaration */
			c99ism(54);
		}
		$$ = $1;
	  }
	;

enumerator_list:		/* C99 6.7.2.2 */
	  enumerator
	| enumerator_list T_COMMA enumerator {
		$$ = lnklst($1, $3);
	  }
	| error {
		$$ = NULL;
	  }
	;

enumerator:			/* C99 6.7.2.2 */
	  identifier_sym gcc_attribute_list_opt {
		$$ = enumeration_constant($1, enumval, true);
	  }
	| identifier_sym gcc_attribute_list_opt T_ASSIGN constant_expr {
		$$ = enumeration_constant($1, to_int_constant($4, true),
		    false);
	  }
	;

type_qualifier:			/* C99 6.7.3 */
	  T_QUAL {
		$$ = xcalloc(1, sizeof(*$$));
		if ($1 == CONST)
			$$->p_const = true;
		if ($1 == VOLATILE)
			$$->p_volatile = true;
	  }
	;

pointer:			/* C99 6.7.5 */
	  asterisk type_qualifier_list_opt {
		$$ = merge_qualified_pointer($1, $2);
	  }
	| asterisk type_qualifier_list_opt pointer {
		$$ = merge_qualified_pointer($1, $2);
		$$ = merge_qualified_pointer($$, $3);
	  }
	;

asterisk:			/* helper for 'pointer' */
	  T_ASTERISK {
		$$ = xcalloc(1, sizeof(*$$));
		$$->p_pointer = true;
	  }
	;

type_qualifier_list_opt:	/* see C99 6.7.5 */
	  /* empty */ {
		$$ = NULL;
	  }
	| type_qualifier_list
	;

type_qualifier_list:		/* C99 6.7.5 */
	  type_qualifier
	| type_qualifier_list type_qualifier {
		$$ = merge_qualified_pointer($1, $2);
	  }
	;

/*
 * For an explanation of 'notype' in the following rules, see
 * https://www.gnu.org/software/bison/manual/bison.html#Semantic-Tokens.
 */

notype_init_declarators:
	  notype_init_declarator
	| notype_init_declarators T_COMMA type_init_declarator
	;

type_init_declarators:
	  type_init_declarator
	| type_init_declarators T_COMMA type_init_declarator
	;

notype_init_declarator:
	  notype_declarator asm_or_symbolrename_opt {
		cgram_declare($1, false, $2);
		check_size($1);
	  }
	| notype_declarator asm_or_symbolrename_opt {
		begin_initialization($1);
		cgram_declare($1, true, $2);
	  } T_ASSIGN initializer {
		check_size($1);
		end_initialization();
	  }
	;

type_init_declarator:
	  type_declarator asm_or_symbolrename_opt {
		cgram_declare($1, false, $2);
		check_size($1);
	  }
	| type_declarator asm_or_symbolrename_opt {
		begin_initialization($1);
		cgram_declare($1, true, $2);
	  } T_ASSIGN initializer {
		check_size($1);
		end_initialization();
	  }
	;

notype_declarator:
	  notype_direct_declarator
	| pointer notype_direct_declarator {
		$$ = add_pointer($2, $1);
	  }
	;

type_declarator:
	  type_direct_declarator
	| pointer type_direct_declarator {
		$$ = add_pointer($2, $1);
	  }
	;

notype_direct_declarator:
	  T_NAME {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN type_declarator T_RPAREN {
		$$ = $2;
	  }
	| type_attribute notype_direct_declarator {
		$$ = $2;
	  }
	| notype_direct_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	  }
	| notype_direct_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| notype_direct_declarator type_attribute
	;

type_direct_declarator:
	  identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN type_declarator T_RPAREN {
		$$ = $2;
	  }
	| type_attribute type_direct_declarator {
		$$ = $2;
	  }
	| type_direct_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	  }
	| type_direct_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| type_direct_declarator type_attribute
	;

/*
 * The two distinct rules type_param_declarator and notype_param_declarator
 * avoid a conflict in argument lists. A typename enclosed in parentheses is
 * always treated as a typename, not an argument name. For example, after
 * "typedef double a;", the declaration "f(int (a));" is interpreted as
 * "f(int (double));", not "f(int a);".
 */
type_param_declarator:
	  direct_param_declarator
	| pointer direct_param_declarator {
		$$ = add_pointer($2, $1);
	  }
	;

notype_param_declarator:
	  direct_notype_param_declarator
	| pointer direct_notype_param_declarator {
		$$ = add_pointer($2, $1);
	  }
	;

direct_param_declarator:
	  identifier type_attribute_list {
		$$ = declarator_name(getsym($1));
	  }
	| identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN notype_param_declarator T_RPAREN {
		$$ = $2;
	  }
	| direct_param_declarator T_LBRACK array_size_opt T_RBRACK
	    gcc_attribute_list_opt {
		$$ = add_array($1, $3.has_dim, $3.dim);
	  }
	| direct_param_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	;

direct_notype_param_declarator:
	  identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN notype_param_declarator T_RPAREN {
		$$ = $2;
	  }
	| direct_notype_param_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	  }
	| direct_notype_param_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	;

param_list:
	  id_list_lparen identifier_list T_RPAREN {
		$$ = $2;
	  }
	| abstract_decl_param_list
	;

id_list_lparen:
	  T_LPAREN {
		block_level++;
		begin_declaration_level(PROTO_ARG);
	  }
	;

array_size_opt:
	  /* empty */ {
		$$.has_dim = false;
		$$.dim = 0;
	  }
	| T_ASTERISK {
		/* since C99; variable length array of unspecified size */
		$$.has_dim = false; /* TODO: maybe change to true */
		$$.dim = 0;	/* just as a placeholder */
	  }
	| array_size {
		$$.has_dim = true;
		$$.dim = $1 == NULL ? 0 : to_int_constant($1, false);
	  }
	;

array_size:
	  type_qualifier_list_opt T_SCLASS constant_expr {
		/* C11 6.7.6.3p7 */
		if ($2 != STATIC)
			yyerror("Bad attribute");
		/* static array size is a C11 extension */
		c11ism(343);
		$$ = $3;
	  }
	| T_QUAL {
		/* C11 6.7.6.2 */
		if ($1 != RESTRICT)
			yyerror("Bad attribute");
		$$ = NULL;
	  }
	| constant_expr
	;

identifier_list:		/* C99 6.7.5 */
	  T_NAME {
		$$ = old_style_function_name(getsym($1));
	  }
	| identifier_list T_COMMA T_NAME {
		$$ = lnklst($1, old_style_function_name(getsym($3)));
	  }
	| identifier_list error
	;

/* XXX: C99 requires an additional specifier-qualifier-list. */
type_name:			/* C99 6.7.6 */
	  {
		begin_declaration_level(ABSTRACT);
	  } abstract_declaration {
		end_declaration_level();
		$$ = $2->s_type;
	  }
	;

abstract_declaration:		/* specific to lint */
	  begin_type_qualifier_list end_type {
		$$ = declare_1_abstract(abstract_name());
	  }
	| begin_type_specifier_qualifier_list end_type {
		$$ = declare_1_abstract(abstract_name());
	  }
	| begin_type_qualifier_list end_type abstract_declarator {
		$$ = declare_1_abstract($3);
	  }
	| begin_type_specifier_qualifier_list end_type abstract_declarator {
		$$ = declare_1_abstract($3);
	  }
	;

/* K&R 8.7, C90 ???, C99 6.7.6, C11 6.7.7 */
/* In K&R, abstract-declarator could be empty and was still simpler. */
abstract_declarator:
	  pointer {
		$$ = add_pointer(abstract_name(), $1);
	  }
	| direct_abstract_declarator
	| pointer direct_abstract_declarator {
		$$ = add_pointer($2, $1);
	  }
	;

/* K&R ---, C90 ???, C99 6.7.6, C11 6.7.7 */
direct_abstract_declarator:
	/* TODO: sort rules according to C99 */
	  T_LPAREN abstract_declarator T_RPAREN {
		$$ = $2;
	  }
	| T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array(abstract_name(), $2.has_dim, $2.dim);
	  }
	| type_attribute direct_abstract_declarator {
		$$ = $2;
	  }
	| direct_abstract_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	  }
	| abstract_decl_param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename(abstract_name(), $2), $1);
		end_declaration_level();
		block_level--;
	  }
	| direct_abstract_declarator abstract_decl_param_list
	    asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| direct_abstract_declarator type_attribute_list
	;

abstract_decl_param_list:	/* specific to lint */
	  abstract_decl_lparen T_RPAREN type_attribute_opt {
		$$ = NULL;
	  }
	| abstract_decl_lparen vararg_parameter_type_list T_RPAREN
	    type_attribute_opt {
		dcs->d_proto = true;
		$$ = $2;
	  }
	| abstract_decl_lparen error T_RPAREN type_attribute_opt {
		$$ = NULL;
	  }
	;

abstract_decl_lparen:		/* specific to lint */
	  T_LPAREN {
		block_level++;
		begin_declaration_level(PROTO_ARG);
	  }
	;

vararg_parameter_type_list:	/* specific to lint */
	  parameter_type_list
	| parameter_type_list T_COMMA T_ELLIPSIS {
		dcs->d_vararg = true;
		$$ = $1;
	  }
	| T_ELLIPSIS {
		if (sflag) {
			/* ANSI C requires formal parameter before '...' */
			error(84);
		} else if (!tflag) {
			/* ANSI C requires formal parameter before '...' */
			warning(84);
		}
		dcs->d_vararg = true;
		$$ = NULL;
	  }
	;

/* XXX: C99 6.7.5 defines the same name, but it looks different. */
parameter_type_list:
	  parameter_declaration
	| parameter_type_list T_COMMA parameter_declaration {
		$$ = lnklst($1, $3);
	  }
	;

/* XXX: C99 6.7.5 defines the same name, but it looks completely different. */
parameter_declaration:
	  begin_type_declmods end_type {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_argument(abstract_name(), false);
	  }
	| begin_type_declaration_specifiers end_type {
		$$ = declare_argument(abstract_name(), false);
	  }
	| begin_type_declmods end_type notype_param_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_argument($3, false);
	  }
	/*
	 * type_param_declarator is needed because of following conflict:
	 * "typedef int a; f(int (a));" could be parsed as
	 * "function with argument a of type int", or
	 * "function with an abstract argument of type function".
	 * This grammar realizes the second case.
	 */
	| begin_type_declaration_specifiers end_type type_param_declarator {
		$$ = declare_argument($3, false);
	  }
	| begin_type_declmods end_type abstract_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_argument($3, false);
	  }
	| begin_type_declaration_specifiers end_type abstract_declarator {
		$$ = declare_argument($3, false);
	  }
	;

initializer:			/* C99 6.7.8 "Initialization" */
	  assignment_expression {
		init_expr($1);
	  }
	| init_lbrace init_rbrace {
		/* XXX: Empty braces are not covered by C99 6.7.8. */
	  }
	| init_lbrace initializer_list comma_opt init_rbrace
	  /* XXX: What is this error handling for? */
	| error
	;

initializer_list:		/* C99 6.7.8 "Initialization" */
	  initializer_list_item
	| initializer_list T_COMMA initializer_list_item
	;

initializer_list_item:		/* helper */
	  designation initializer
	| initializer
	;

designation:			/* C99 6.7.8 "Initialization" */
	  begin_designation designator_list T_ASSIGN
	| identifier T_COLON {
		/* GCC style struct or union member name in initializer */
		gnuism(315);
		begin_designation();
		add_designator_member($1);
	  }
	;

begin_designation:		/* lint-specific helper */
	  /* empty */ {
		begin_designation();
	  }
	;

designator_list:		/* C99 6.7.8 "Initialization" */
	  designator
	| designator_list designator
	;

designator:			/* C99 6.7.8 "Initialization" */
	  T_LBRACK range T_RBRACK {
		add_designator_subscript($2);
		if (!Sflag)
			/* array initializer with designators is a C99 ... */
			warning(321);
	  }
	| T_POINT identifier {
		if (!Sflag)
			/* struct or union member name in initializer is ... */
			warning(313);
		add_designator_member($2);
	  }
	;

static_assert_declaration:
	  T_STATIC_ASSERT T_LPAREN constant_expr T_COMMA T_STRING T_RPAREN T_SEMI /* C11 */
	| T_STATIC_ASSERT T_LPAREN constant_expr T_RPAREN T_SEMI /* C23 */
 	;

range:
	  constant_expr {
		$$.lo = to_int_constant($1, true);
		$$.hi = $$.lo;
	  }
	| constant_expr T_ELLIPSIS constant_expr {
		$$.lo = to_int_constant($1, true);
		$$.hi = to_int_constant($3, true);
		/* initialization with '[a...b]' is a GCC extension */
		gnuism(340);
	  }
	;

init_lbrace:			/* helper */
	  T_LBRACE {
		init_lbrace();
	  }
	;

init_rbrace:			/* helper */
	  T_RBRACE {
		init_rbrace();
	  }
	;

asm_or_symbolrename_opt:	/* GCC extensions */
	  /* empty */ {
		$$ = NULL;
	  }
	| T_ASM T_LPAREN T_STRING T_RPAREN gcc_attribute_list_opt {
		freeyyv(&$3, T_STRING);
		$$ = NULL;
	  }
	| T_SYMBOLRENAME T_LPAREN T_NAME T_RPAREN gcc_attribute_list_opt {
		$$ = $3;
	  }
	;

statement:			/* C99 6.8 */
	  expression_statement
	| non_expr_statement
	;

non_expr_statement:		/* helper for C99 6.8 */
	  type_attribute T_SEMI
	| labeled_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement {
		seen_fallthrough = false;
	  }
	| asm_statement
	;

labeled_statement:		/* C99 6.8.1 */
	  label type_attribute_opt statement
	;

label:
	  T_NAME T_COLON {
		symtyp = FLABEL;
		named_label(getsym($1));
	  }
	| T_CASE constant_expr T_COLON {
		case_label($2);
		seen_fallthrough = true;
	  }
	| T_CASE constant_expr T_ELLIPSIS constant_expr T_COLON {
		/* XXX: We don't fill all cases */
		case_label($2);
		seen_fallthrough = true;
	  }
	| T_DEFAULT T_COLON {
		default_label();
		seen_fallthrough = true;
	  }
	;

compound_statement:		/* C99 6.8.2 */
	  compound_statement_lbrace compound_statement_rbrace
	| compound_statement_lbrace block_item_list compound_statement_rbrace
	;

compound_statement_lbrace:
	  T_LBRACE {
		block_level++;
		mem_block_level++;
		begin_declaration_level(AUTO);
	  }
	;

compound_statement_rbrace:
	  T_RBRACE {
		end_declaration_level();
		level_free_all(mem_block_level);
		mem_block_level--;
		block_level--;
		seen_fallthrough = false;
	  }
	;

block_item_list:		/* C99 6.8.2 */
	  block_item
	| block_item_list block_item {
		if (!Sflag && $1 && !$2)
			/* declarations after statements is a C99 feature */
			c99ism(327);
		$$ = $1 || $2;
	  }
	;

block_item:			/* C99 6.8.2 */
	  declaration_or_error {
		$$ = false;
		restore_warning_flags();
	  }
	| statement {
		$$ = true;
		restore_warning_flags();
	  }
	;

expression_statement:		/* C99 6.8.3 */
	  expression T_SEMI {
		expr($1, false, false, false, false);
		seen_fallthrough = false;
	  }
	| T_SEMI {
		check_statement_reachable();
		seen_fallthrough = false;
	  }
	;

selection_statement:		/* C99 6.8.4 */
	  if_without_else %prec T_THEN {
		save_warning_flags();
		if2();
		if3(false);
	  }
	| if_without_else T_ELSE {
		save_warning_flags();
		if2();
	  } statement {
		clear_warning_flags();
		if3(true);
	  }
	| if_without_else T_ELSE error {
		clear_warning_flags();
		if3(false);
	  }
	| switch_expr statement {
		clear_warning_flags();
		switch2();
	  }
	| switch_expr error {
		clear_warning_flags();
		switch2();
	  }
	;

if_without_else:		/* see C99 6.8.4 */
	  if_expr statement
	| if_expr error
	;

if_expr:			/* see C99 6.8.4 */
	  T_IF T_LPAREN expression T_RPAREN {
		if1($3);
		clear_warning_flags();
	  }
	;

switch_expr:			/* see C99 6.8.4 */
	  T_SWITCH T_LPAREN expression T_RPAREN {
		switch1($3);
		clear_warning_flags();
	  }
	;

iteration_statement:		/* C99 6.8.5 */
	  while_expr statement {
		clear_warning_flags();
		while2();
	  }
	| while_expr error {
		clear_warning_flags();
		while2();
	  }
	| do_statement do_while_expr {
		do2($2);
		seen_fallthrough = false;
	  }
	| do error {
		clear_warning_flags();
		do2(NULL);
	  }
	| for_exprs statement {
		clear_warning_flags();
		for2();
		end_declaration_level();
		block_level--;
	  }
	| for_exprs error {
		clear_warning_flags();
		for2();
		end_declaration_level();
		block_level--;
	  }
	;

while_expr:			/* see C99 6.8.5 */
	  T_WHILE T_LPAREN expression T_RPAREN {
		while1($3);
		clear_warning_flags();
	  }
	;

do_statement:			/* see C99 6.8.5 */
	  do statement {
		clear_warning_flags();
	  }
	;

do:				/* see C99 6.8.5 */
	  T_DO {
		do1();
	  }
	;

do_while_expr:			/* see C99 6.8.5 */
	  T_WHILE T_LPAREN expression T_RPAREN T_SEMI {
		$$ = $3;
	  }
	;

for_start:			/* see C99 6.8.5 */
	  T_FOR T_LPAREN {
		begin_declaration_level(AUTO);
		block_level++;
	  }
	;

for_exprs:			/* see C99 6.8.5 */
	  for_start
	    begin_type_declaration_specifiers end_type
	    notype_init_declarators T_SEMI
	    expression_opt T_SEMI
	    expression_opt T_RPAREN {
		/* variable declaration in for loop */
		c99ism(325);
		for1(NULL, $6, $8);
		clear_warning_flags();
	    }
	| for_start
	    expression_opt T_SEMI
	    expression_opt T_SEMI
	    expression_opt T_RPAREN {
		for1($2, $4, $6);
		clear_warning_flags();
	  }
	;

jump_statement:			/* C99 6.8.6 */
	  goto identifier T_SEMI {
		do_goto(getsym($2));
	  }
	| goto error T_SEMI {
		symtyp = FVFT;
	  }
	| T_CONTINUE T_SEMI {
		do_continue();
	  }
	| T_BREAK T_SEMI {
		do_break();
	  }
	| T_RETURN sys T_SEMI {
		do_return($2, NULL);
	  }
	| T_RETURN sys expression T_SEMI {
		do_return($2, $3);
	  }
	;

goto:				/* see C99 6.8.6 */
	  T_GOTO {
		symtyp = FLABEL;
	  }
	;

asm_statement:			/* GCC extension */
	  T_ASM T_LPAREN read_until_rparen T_SEMI {
		setasm();
	  }
	| T_ASM T_QUAL T_LPAREN read_until_rparen T_SEMI {
		setasm();
	  }
	| T_ASM error
	;

read_until_rparen:		/* helper for 'asm_statement' */
	  /* empty */ {
		read_until_rparen();
	  }
	;

translation_unit:		/* C99 6.9 */
	  external_declaration
	| translation_unit external_declaration
	;

external_declaration:		/* C99 6.9 */
	  function_definition {
		global_clean_up_decl(false);
		clear_warning_flags();
	  }
	| top_level_declaration {
		global_clean_up_decl(false);
		clear_warning_flags();
	  }
	| asm_statement		/* GCC extension */
	| T_SEMI {		/* GCC extension */
		if (sflag) {
			/* empty declaration */
			error(0);
		} else if (!tflag) {
			/* empty declaration */
			warning(0);
		}
	  }
	;

/*
 * On the top level, lint allows several forms of declarations that it doesn't
 * allow in functions.  For example, a single ';' is an empty declaration and
 * is supported by some compilers, but in a function it would be an empty
 * statement, not a declaration.  This makes a difference in C90 mode, where
 * a statement must not be followed by a declaration.
 *
 * See 'declaration' for all other declarations.
 */
top_level_declaration:		/* C99 6.9 calls this 'declaration' */
	  begin_type end_type notype_init_declarators T_SEMI {
		if (sflag) {
			/* old style declaration; add 'int' */
			error(1);
		} else if (!tflag) {
			/* old style declaration; add 'int' */
			warning(1);
		}
	  }
	| declaration
	| error T_SEMI {
		global_clean_up();
	  }
	| error T_RBRACE {
		global_clean_up();
	  }
	;

function_definition:		/* C99 6.9.1 */
	  func_declarator {
		if ($1->s_type->t_tspec != FUNC) {
			/* syntax error '%s' */
			error(249, yytext);
			YYERROR;
		}
		if ($1->s_type->t_typedef) {
			/* ()-less function definition */
			error(64);
			YYERROR;
		}
		funcdef($1);
		block_level++;
		begin_declaration_level(OLD_STYLE_ARG);
		if (lwarn == LWARN_NONE)
			$1->s_used = true;
	  } arg_declaration_list_opt {
		end_declaration_level();
		block_level--;
		check_func_lint_directives();
		check_func_old_style_arguments();
		begin_control_statement(CS_FUNCTION_BODY);
	  } compound_statement {
		funcend();
		end_control_statement(CS_FUNCTION_BODY);
	  }
	;

func_declarator:
	  begin_type end_type notype_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = $3;
	  }
	| begin_type_declmods end_type notype_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = $3;
	  }
	| begin_type_declaration_specifiers end_type type_declarator {
		$$ = $3;
	  }
	;

arg_declaration_list_opt:	/* C99 6.9.1p13 example 1 */
	  /* empty */
	| arg_declaration_list
	;

arg_declaration_list:		/* C99 6.9.1p13 example 1 */
	  arg_declaration
	| arg_declaration_list arg_declaration
	/* XXX or better "arg_declaration error" ? */
	| error
	;

/*
 * "arg_declaration" is separated from "declaration" because it
 * needs other error handling.
 */
arg_declaration:
	  begin_type_declmods end_type T_SEMI {
		/* empty declaration */
		warning(2);
	  }
	| begin_type_declmods end_type notype_init_declarators T_SEMI
	| begin_type_declaration_specifiers end_type T_SEMI {
		if (!dcs->d_nonempty_decl) {
			/* empty declaration */
			warning(2);
		} else {
			/* '%s' declared in argument declaration list */
			warning(3, type_name(dcs->d_type));
		}
	  }
	| begin_type_declaration_specifiers end_type
	    type_init_declarators T_SEMI {
		if (dcs->d_nonempty_decl) {
			/* '%s' declared in argument declaration list */
			warning(3, type_name(dcs->d_type));
		}
	  }
	| begin_type_declmods error
	| begin_type_declaration_specifiers error
	;

gcc_attribute_list_opt:
	  /* empty */
	| gcc_attribute_list
	;

gcc_attribute_list:
	  gcc_attribute
	| gcc_attribute_list gcc_attribute
	;

gcc_attribute:
	  T_ATTRIBUTE T_LPAREN T_LPAREN {
	    in_gcc_attribute = true;
	  } gcc_attribute_spec_list {
	    in_gcc_attribute = false;
	  } T_RPAREN T_RPAREN
	;

gcc_attribute_spec_list:
	  gcc_attribute_spec
	| gcc_attribute_spec_list T_COMMA gcc_attribute_spec
	;

gcc_attribute_spec:
	  /* empty */
	| T_AT_ALWAYS_INLINE
	| T_AT_ALIAS T_LPAREN string T_RPAREN
	| T_AT_ALIGNED T_LPAREN constant_expr T_RPAREN
	| T_AT_ALIGNED
	| T_AT_ALLOC_SIZE T_LPAREN constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_ALLOC_SIZE T_LPAREN constant_expr T_RPAREN
	| T_AT_BOUNDED T_LPAREN gcc_attribute_bounded
	  T_COMMA constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_COLD
	| T_AT_COMMON
	| T_AT_CONSTRUCTOR T_LPAREN constant_expr T_RPAREN
	| T_AT_CONSTRUCTOR
	| T_AT_DEPRECATED T_LPAREN string T_RPAREN
	| T_AT_DEPRECATED
	| T_AT_DESTRUCTOR T_LPAREN constant_expr T_RPAREN
	| T_AT_DESTRUCTOR
	| T_AT_DISABLE_SANITIZER_INSTRUMENTATION
	| T_AT_FALLTHROUGH {
		fallthru(1);
	  }
	| T_AT_FORMAT T_LPAREN gcc_attribute_format T_COMMA
	    constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_FORMAT_ARG T_LPAREN constant_expr T_RPAREN
	| T_AT_GNU_INLINE
	| T_AT_HOT
	| T_AT_MALLOC
	| T_AT_MAY_ALIAS
	| T_AT_MODE T_LPAREN T_NAME T_RPAREN
	| T_AT_NO_SANITIZE T_LPAREN T_NAME T_RPAREN
	| T_AT_NO_SANITIZE_THREAD
	| T_AT_NOINLINE
	| T_AT_NONNULL T_LPAREN constant_expr_list_opt T_RPAREN
	| T_AT_NONNULL
	| T_AT_NONSTRING
	| T_AT_NORETURN
	| T_AT_NOTHROW
	| T_AT_NO_INSTRUMENT_FUNCTION
	| T_AT_OPTIMIZE T_LPAREN string T_RPAREN
	| T_AT_OPTNONE
	| T_AT_PACKED {
		addpacked();
	  }
	| T_AT_PCS T_LPAREN string T_RPAREN
	| T_AT_PURE
	| T_AT_REGPARM T_LPAREN constant_expr T_RPAREN
	| T_AT_RETURNS_NONNULL
	| T_AT_RETURNS_TWICE
	| T_AT_SECTION T_LPAREN string T_RPAREN
	| T_AT_SENTINEL T_LPAREN constant_expr T_RPAREN
	| T_AT_SENTINEL
	| T_AT_TARGET T_LPAREN string T_RPAREN
	| T_AT_TLS_MODEL T_LPAREN string T_RPAREN
	| T_AT_TUNION
	| T_AT_UNUSED {
		add_attr_used();
	  }
	| T_AT_USED {
		add_attr_used();
	  }
	| T_AT_VISIBILITY T_LPAREN constant_expr T_RPAREN
	| T_AT_WARN_UNUSED_RESULT
	| T_AT_WEAK
	| T_QUAL {
		if ($1 != CONST)
			yyerror("Bad attribute");
	  }
	;

gcc_attribute_bounded:
	  T_AT_MINBYTES
	| T_AT_STRING
	| T_AT_BUFFER
	;

gcc_attribute_format:
	  T_AT_FORMAT_GNU_PRINTF
	| T_AT_FORMAT_PRINTF
	| T_AT_FORMAT_SCANF
	| T_AT_FORMAT_STRFMON
	| T_AT_FORMAT_STRFTIME
	| T_AT_FORMAT_SYSLOG
	;

sys:
	  /* empty */ {
		$$ = in_system_header;
	  }
	;

%%

/* ARGSUSED */
int
yyerror(const char *msg)
{
	/* syntax error '%s' */
	error(249, yytext);
	if (++sytxerr >= 5)
		norecover();
	return 0;
}

#if (defined(YYDEBUG) && YYDEBUG > 0 && defined(YYBYACC)) \
    || (defined(YYDEBUG) && defined(YYBISON))
static const char *
cgram_to_string(int token, YYSTYPE val)
{

	switch (token) {
	case T_INCDEC:
	case T_MULTIPLICATIVE:
	case T_ADDITIVE:
	case T_SHIFT:
	case T_RELATIONAL:
	case T_EQUALITY:
	case T_OPASSIGN:
		return modtab[val.y_op].m_name;
	case T_SCLASS:
		return scl_name(val.y_scl);
	case T_TYPE:
	case T_STRUCT_OR_UNION:
		return tspec_name(val.y_tspec);
	case T_QUAL:
		return tqual_name(val.y_tqual);
	case T_NAME:
		return val.y_name->sb_name;
	default:
		return "<none>";
	}
}
#endif

#if defined(YYDEBUG) && defined(YYBISON)
static inline void
cgram_print(FILE *output, int token, YYSTYPE val)
{
	(void)fprintf(output, "%s", cgram_to_string(token, val));
}
#endif

static void
cgram_declare(sym_t *decl, bool initflg, sbuf_t *renaming)
{
	declare(decl, initflg, renaming);
	if (renaming != NULL)
		freeyyv(&renaming, T_NAME);
}

/*
 * Discard all input tokens up to and including the next
 * unmatched right paren
 */
static void
read_until_rparen(void)
{
	int	level;

	if (yychar < 0)
		yychar = yylex();
	freeyyv(&yylval, yychar);

	level = 1;
	while (yychar != T_RPAREN || --level > 0) {
		if (yychar == T_LPAREN) {
			level++;
		} else if (yychar <= 0) {
			break;
		}
		freeyyv(&yylval, yychar = yylex());
	}

	yyclearin;
}

static	sym_t *
symbolrename(sym_t *s, sbuf_t *sb)
{
	if (sb != NULL)
		s->s_rename = sb->sb_name;
	return s;
}
