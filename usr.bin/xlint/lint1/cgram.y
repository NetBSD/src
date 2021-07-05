%{
/* $NetBSD: cgram.y,v 1.252 2021/07/05 19:59:10 rillig Exp $ */

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
__RCSID("$NetBSD: cgram.y,v 1.252 2021/07/05 19:59:10 rillig Exp $");
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
int	mem_block_level;

/*
 * Save the no-warns state and restore it to avoid the problem where
 * if (expr) { stmt } / * NOLINT * / stmt;
 */
static int olwarn = LWARN_BAD;

static	void	cgram_declare(sym_t *, bool, sbuf_t *);
static	void	ignore_up_to_rparen(void);
static	sym_t	*symbolrename(sym_t *, sbuf_t *);


#ifdef DEBUG
static void
CLEAR_WARN_FLAGS(const char *file, size_t line)
{
	printf("%s:%d: %s:%zu: clearing flags\n",
	    curr_pos.p_file, curr_pos.p_line, file, line);
	clear_warn_flags();
	olwarn = LWARN_BAD;
}

static void
SAVE_WARN_FLAGS(const char *file, size_t line)
{
	lint_assert(olwarn == LWARN_BAD);
	printf("%s:%d: %s:%zu: saving flags %d\n",
	    curr_pos.p_file, curr_pos.p_line, file, line, lwarn);
	olwarn = lwarn;
}

static void
RESTORE_WARN_FLAGS(const char *file, size_t line)
{
	if (olwarn != LWARN_BAD) {
		lwarn = olwarn;
		printf("%s:%d: %s:%zu: restoring flags %d\n",
		    curr_pos.p_file, curr_pos.p_line, file, line, lwarn);
		olwarn = LWARN_BAD;
	} else
		CLEAR_WARN_FLAGS(file, line);
}
#define cgram_debug(fmt, args...) printf("cgram_debug: " fmt "\n", ##args)
#else
#define CLEAR_WARN_FLAGS(f, l)	clear_warn_flags(), olwarn = LWARN_BAD
#define SAVE_WARN_FLAGS(f, l)	olwarn = lwarn
#define RESTORE_WARN_FLAGS(f, l) \
	(void)(olwarn == LWARN_BAD ? (clear_warn_flags(), 0) : (lwarn = olwarn))
#define cgram_debug(fmt, args...) do { } while (false)
#endif

#define clear_warning_flags()	CLEAR_WARN_FLAGS(__FILE__, __LINE__)
#define save_warning_flags()	SAVE_WARN_FLAGS(__FILE__, __LINE__)
#define restore_warning_flags()	RESTORE_WARN_FLAGS(__FILE__, __LINE__)

/* unbind the anonymous struct members from the struct */
static void
anonymize(sym_t *s)
{
	for ( ; s != NULL; s = s->s_next)
		s->s_styp = NULL;
}
%}

%expect 182

%union {
	val_t	*y_val;
	sbuf_t	*y_sb;
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
	struct generic_association_types *y_types;
};

%token			T_LBRACE T_RBRACE T_LBRACK T_RBRACK T_LPAREN T_RPAREN
%token			T_POINT T_ARROW
%token	<y_op>		T_UNARY
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
 * float, double, void); see T_TYPENAME
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
/* Type Attributes */
%token <y_type>		T_ATTRIBUTE
%token <y_type>		T_AT_ALIAS
%token <y_type>		T_AT_ALIGNED
%token <y_type>		T_AT_ALLOC_SIZE
%token <y_type>		T_AT_ALWAYS_INLINE
%token <y_type>		T_AT_BOUNDED
%token <y_type>		T_AT_BUFFER
%token <y_type>		T_AT_COLD
%token <y_type>		T_AT_COMMON
%token <y_type>		T_AT_CONSTRUCTOR
%token <y_type>		T_AT_DEPRECATED
%token <y_type>		T_AT_DESTRUCTOR
%token <y_type>		T_AT_FALLTHROUGH
%token <y_type>		T_AT_FORMAT
%token <y_type>		T_AT_FORMAT_ARG
%token <y_type>		T_AT_FORMAT_GNU_PRINTF
%token <y_type>		T_AT_FORMAT_PRINTF
%token <y_type>		T_AT_FORMAT_SCANF
%token <y_type>		T_AT_FORMAT_STRFMON
%token <y_type>		T_AT_FORMAT_STRFTIME
%token <y_type>		T_AT_FORMAT_SYSLOG
%token <y_type>		T_AT_GNU_INLINE
%token <y_type>		T_AT_MALLOC
%token <y_type>		T_AT_MAY_ALIAS
%token <y_type>		T_AT_MINBYTES
%token <y_type>		T_AT_MODE
%token <y_type>		T_AT_NOINLINE
%token <y_type>		T_AT_NONNULL
%token <y_type>		T_AT_NONSTRING
%token <y_type>		T_AT_NORETURN
%token <y_type>		T_AT_NOTHROW
%token <y_type>		T_AT_NO_INSTRUMENT_FUNCTION
%token <y_type>		T_AT_OPTIMIZE
%token <y_type>		T_AT_PACKED
%token <y_type>		T_AT_PCS
%token <y_type>		T_AT_PURE
%token <y_type>		T_AT_RETURNS_TWICE
%token <y_type>		T_AT_SECTION
%token <y_type>		T_AT_SENTINEL
%token <y_type>		T_AT_STRING
%token <y_type>		T_AT_TLS_MODEL
%token <y_type>		T_AT_TUNION
%token <y_type>		T_AT_UNUSED
%token <y_type>		T_AT_USED
%token <y_type>		T_AT_VISIBILITY
%token <y_type>		T_AT_WARN_UNUSED_RESULT
%token <y_type>		T_AT_WEAK

%left	T_COMMA
%right	T_ASSIGN T_OPASSIGN
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
%right	T_UNARY T_INCDEC T_SIZEOF T_REAL T_IMAG
%left	T_LPAREN T_LBRACK T_POINT T_ARROW

%token	<y_sb>		T_NAME
%token	<y_sb>		T_TYPENAME
%token	<y_val>		T_CON
%token	<y_string>	T_STRING

%type	<y_sym>		func_decl
%type	<y_sym>		notype_decl
%type	<y_sym>		type_decl
%type	<y_type>	typespec
%type	<y_type>	clrtyp_typespec
%type	<y_type>	notype_typespec
%type	<y_type>	struct_spec
%type	<y_type>	enum_spec
%type	<y_sym>		struct_tag
%type	<y_sym>		enum_tag
%type	<y_tspec>	struct
%type	<y_sym>		struct_declaration
%type	<y_sb>		identifier
%type	<y_sym>		member_declaration_list_with_rbrace
%type	<y_sym>		member_declaration_list
%type	<y_sym>		member_declaration
%type	<y_sym>		notype_member_decls
%type	<y_sym>		type_member_decls
%type	<y_sym>		notype_member_decl
%type	<y_sym>		type_member_decl
%type	<y_tnode>	constant_expr
%type	<y_tnode>	array_size
%type	<y_sym>		enum_declaration
%type	<y_sym>		enums_with_opt_comma
%type	<y_sym>		enums
%type	<y_sym>		enumerator
%type	<y_sym>		enumeration_constant
%type	<y_sym>		notype_direct_decl
%type	<y_sym>		type_direct_decl
%type	<y_qual_ptr>	pointer
%type	<y_qual_ptr>	asterisk
%type	<y_sym>		param_decl
%type	<y_sym>		param_list
%type	<y_sym>		abstract_decl_param_list
%type	<y_sym>		direct_param_decl
%type	<y_sym>		notype_param_decl
%type	<y_sym>		direct_notype_param_decl
%type	<y_qual_ptr>	type_qualifier_list_opt
%type	<y_qual_ptr>	type_qualifier_list
%type	<y_qual_ptr>	type_qualifier
%type	<y_sym>		identifier_list
%type	<y_sym>		abstract_declarator
%type	<y_sym>		direct_abstract_declarator
%type	<y_sym>		vararg_parameter_type_list
%type	<y_sym>		parameter_type_list
%type	<y_sym>		parameter_declaration
%type	<y_tnode>	expr
%type	<y_tnode>	assignment_expression
%type	<y_tnode>	gcc_statement_expr_list
%type	<y_tnode>	gcc_statement_expr_item
%type	<y_tnode>	term
%type	<y_tnode>	generic_selection
%type	<y_tnode>	func_arg_list
%type	<y_op>		point_or_arrow
%type	<y_type>	type_name
%type	<y_sym>		abstract_declaration
%type	<y_tnode>	do_while_expr
%type	<y_tnode>	expr_opt
%type	<y_string>	string
%type	<y_string>	string2
%type	<y_sb>		asm_or_symbolrename_opt
%type	<y_range>	range
%type	<y_seen_statement> block_item_list
%type	<y_seen_statement> block_item
%type	<y_types>	generic_assoc_list
%type	<y_types>	generic_association


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

translation_unit:		/* C99 6.9 */
	  external_declaration
	| translation_unit external_declaration
	;

external_declaration:		/* C99 6.9 */
	  asm_statement
	| function_definition {
		global_clean_up_decl(false);
		clear_warning_flags();
	  }
	| top_level_declaration {
		global_clean_up_decl(false);
		clear_warning_flags();
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
	  T_SEMI {
		if (sflag) {
			/* empty declaration */
			error(0);
		} else if (!tflag) {
			/* empty declaration */
			warning(0);
		}
	  }
	| clrtyp deftyp notype_init_decls T_SEMI {
		if (sflag) {
			/* old style declaration; add 'int' */
			error(1);
		} else if (!tflag) {
			/* old style declaration; add 'int' */
			warning(1);
		}
	  }
	| declmods deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else {
			/* empty declaration */
			warning(2);
		}
	  }
	| declmods deftyp notype_init_decls T_SEMI
	| declaration_specifiers deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else if (!dcs->d_nonempty_decl) {
			/* empty declaration */
			warning(2);
		}
	  }
	| declaration_specifiers deftyp type_init_decls T_SEMI
	| error T_SEMI {
		global_clean_up();
	  }
	| error T_RBRACE {
		global_clean_up();
	  }
	;

function_definition:		/* C99 6.9.1 */
	  func_decl {
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
		begin_declaration_level(ARG);
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

func_decl:
	  clrtyp deftyp notype_decl {
		$$ = $3;
	  }
	| declmods deftyp notype_decl {
		$$ = $3;
	  }
	| declaration_specifiers deftyp type_decl {
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
	  declmods deftyp T_SEMI {
		/* empty declaration */
		warning(2);
	  }
	| declmods deftyp notype_init_decls T_SEMI
	| declaration_specifiers deftyp T_SEMI {
		if (!dcs->d_nonempty_decl) {
			/* empty declaration */
			warning(2);
		} else {
			/* '%s' declared in argument declaration list */
			warning(3, type_name(dcs->d_type));
		}
	  }
	| declaration_specifiers deftyp type_init_decls T_SEMI {
		if (dcs->d_nonempty_decl) {
			/* '%s' declared in argument declaration list */
			warning(3, type_name(dcs->d_type));
		}
	  }
	| declmods error
	| declaration_specifiers error
	;

declaration:			/* C99 6.7 */
	  declmods deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else {
			/* empty declaration */
			warning(2);
		}
	  }
	| declmods deftyp notype_init_decls T_SEMI
	| declaration_specifiers deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else if (!dcs->d_nonempty_decl) {
			/* empty declaration */
			warning(2);
		}
	  }
	| declaration_specifiers deftyp type_init_decls T_SEMI
	| error T_SEMI
	;

type_attribute_format_type:
	  T_AT_FORMAT_GNU_PRINTF
	| T_AT_FORMAT_PRINTF
	| T_AT_FORMAT_SCANF
	| T_AT_FORMAT_STRFMON
	| T_AT_FORMAT_STRFTIME
	| T_AT_FORMAT_SYSLOG
	;

type_attribute_bounded_type:
	  T_AT_MINBYTES
	| T_AT_STRING
	| T_AT_BUFFER
	;


type_attribute_spec:
	  /* empty */
	| T_AT_ALWAYS_INLINE
	| T_AT_ALIAS T_LPAREN string T_RPAREN
	| T_AT_ALIGNED T_LPAREN constant_expr T_RPAREN
	| T_AT_ALIGNED
	| T_AT_ALLOC_SIZE T_LPAREN constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_ALLOC_SIZE T_LPAREN constant_expr T_RPAREN
	| T_AT_BOUNDED T_LPAREN type_attribute_bounded_type
	  T_COMMA constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_COLD
	| T_AT_COMMON
	| T_AT_CONSTRUCTOR T_LPAREN constant_expr T_RPAREN
	| T_AT_CONSTRUCTOR
	| T_AT_DEPRECATED T_LPAREN string T_RPAREN
	| T_AT_DEPRECATED
	| T_AT_DESTRUCTOR T_LPAREN constant_expr T_RPAREN
	| T_AT_DESTRUCTOR
	| T_AT_FALLTHROUGH {
		fallthru(1);
	}
	| T_AT_FORMAT T_LPAREN type_attribute_format_type T_COMMA
	    constant_expr T_COMMA constant_expr T_RPAREN
	| T_AT_FORMAT_ARG T_LPAREN constant_expr T_RPAREN
	| T_AT_GNU_INLINE
	| T_AT_MALLOC
	| T_AT_MAY_ALIAS
	| T_AT_MODE T_LPAREN T_NAME T_RPAREN
	| T_AT_NOINLINE
	| T_AT_NONNULL T_LPAREN constant_expr_list_opt T_RPAREN
	| T_AT_NONNULL
	| T_AT_NONSTRING
	| T_AT_NORETURN
	| T_AT_NOTHROW
	| T_AT_NO_INSTRUMENT_FUNCTION
	| T_AT_OPTIMIZE T_LPAREN string T_RPAREN
	| T_AT_PACKED {
		addpacked();
	  }
	| T_AT_PCS T_LPAREN string T_RPAREN
	| T_AT_PURE
	| T_AT_RETURNS_TWICE
	| T_AT_SECTION T_LPAREN string T_RPAREN
	| T_AT_SENTINEL T_LPAREN constant_expr T_RPAREN
	| T_AT_SENTINEL
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

type_attribute_spec_list:
	  type_attribute_spec
	| type_attribute_spec_list T_COMMA type_attribute_spec
	;

align_as:
	  typespec
	| constant_expr
	;

type_attribute:
	  T_ATTRIBUTE T_LPAREN T_LPAREN {
	    attron = true;
	  } type_attribute_spec_list {
	    attron = false;
	  } T_RPAREN T_RPAREN
	| T_ALIGNAS T_LPAREN align_as T_RPAREN {
	  }
	| T_PACKED {
		addpacked();
	  }
	| T_NORETURN {
	  }
	;

type_attribute_list:
	  type_attribute
	| type_attribute_list type_attribute
	;

clrtyp:
	  /* empty */ {
		clrtyp();
	  }
	;

deftyp:
	  /* empty */ {
		deftyp();
	  }
	;

declaration_specifiers:		/* C99 6.7 */
	  clrtyp_typespec {
		add_type($1);
	  }
	| declmods typespec {
		add_type($2);
	  }
	| type_attribute declaration_specifiers
	| declaration_specifiers declmod
	| declaration_specifiers notype_typespec {
		add_type($2);
	  }
	;

declmods:
	  clrtyp T_QUAL {
		add_qualifier($2);
	  }
	| clrtyp T_SCLASS {
		add_storage_class($2);
	  }
	| declmods declmod
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

clrtyp_typespec:
	  clrtyp notype_typespec {
		$$ = $2;
	  }
	| T_TYPENAME clrtyp {
		$$ = getsym($1)->s_type;
	  }
	;

typespec:
	  notype_typespec
	| T_TYPENAME {
		$$ = getsym($1)->s_type;
	  }
	;

notype_typespec:
	  T_TYPE {
		$$ = gettyp($1);
	  }
	| T_TYPEOF term {
		$$ = $2->tn_type;
	  }
	| struct_spec {
		end_declaration_level();
		$$ = $1;
	  }
	| enum_spec {
		end_declaration_level();
		$$ = $1;
	  }
	;

struct_spec:
	  struct struct_tag {
		/*
		 * STDC requires that "struct a;" always introduces
		 * a new tag if "a" is not declared at current level
		 *
		 * yychar is valid because otherwise the parser would not
		 * have been able to decide if it must shift or reduce
		 */
		$$ = mktag($2, $1, false, yychar == T_SEMI);
	  }
	| struct struct_tag {
		dcs->d_tagtyp = mktag($2, $1, true, false);
	  } struct_declaration {
		$$ = complete_tag_struct_or_union(dcs->d_tagtyp, $4);
	  }
	| struct {
		dcs->d_tagtyp = mktag(NULL, $1, true, false);
	  } struct_declaration {
		$$ = complete_tag_struct_or_union(dcs->d_tagtyp, $3);
	  }
	| struct error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

struct:
	  struct type_attribute
	| T_STRUCT_OR_UNION {
		symtyp = FTAG;
		begin_declaration_level($1 == STRUCT ? MOS : MOU);
		dcs->d_offset = 0;
		dcs->d_sou_align_in_bits = CHAR_SIZE;
		$$ = $1;
	  }
	;

struct_tag:
	  identifier {
		$$ = getsym($1);
	  }
	;

struct_declaration:
	  struct_decl_lbrace member_declaration_list_with_rbrace {
		$$ = $2;
	  }
	;

struct_decl_lbrace:
	  T_LBRACE {
		symtyp = FVFT;
	  }
	;

member_declaration_list_with_rbrace:
	  member_declaration_list T_SEMI T_RBRACE
	| member_declaration_list T_RBRACE {
		if (sflag) {
			/* syntax req. ';' after last struct/union member */
			error(66);
		} else {
			/* syntax req. ';' after last struct/union member */
			warning(66);
		}
		$$ = $1;
	  }
	| T_RBRACE {
		$$ = NULL;
	  }
	;

type_attribute_opt:
	  /* empty */
	| type_attribute
	;

member_declaration_list:
	  member_declaration
	| member_declaration_list T_SEMI member_declaration {
		$$ = lnklst($1, $3);
	  }
	;

member_declaration:
	  noclass_declmods deftyp {
		/* too late, i know, but getsym() compensates it */
		symtyp = FMEMBER;
	  } notype_member_decls type_attribute_opt {
		symtyp = FVFT;
		$$ = $4;
	  }
	| noclass_declspecs deftyp {
		symtyp = FMEMBER;
	  } type_member_decls type_attribute_opt {
		symtyp = FVFT;
		$$ = $4;
	  }
	| noclass_declmods deftyp type_attribute_opt {
		symtyp = FVFT;
		/* struct or union member must be named */
		if (!Sflag)
			/* anonymous struct/union members is a C9X feature */
			warning(49);
		/* add all the members of the anonymous struct/union */
		lint_assert(is_struct_or_union(dcs->d_type->t_tspec));
		$$ = dcs->d_type->t_str->sou_first_member;
		anonymize($$);
	  }
	| noclass_declspecs deftyp type_attribute_opt {
		symtyp = FVFT;
		/* struct or union member must be named */
		if (!Sflag)
			/* anonymous struct/union members is a C9X feature */
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
	| error {
		symtyp = FVFT;
		$$ = NULL;
	  }
	;

noclass_declspecs:
	  clrtyp_typespec {
		add_type($1);
	  }
	| type_attribute noclass_declspecs
	| noclass_declmods typespec {
		add_type($2);
	  }
	| noclass_declspecs T_QUAL {
		add_qualifier($2);
	  }
	| noclass_declspecs notype_typespec {
		add_type($2);
	  }
	| noclass_declspecs type_attribute
	;

noclass_declmods:
	  clrtyp T_QUAL {
		add_qualifier($2);
	  }
	| noclass_declmods T_QUAL {
		add_qualifier($2);
	  }
	;

notype_member_decls:
	  notype_member_decl {
		$$ = declarator_1_struct_union($1);
	  }
	| notype_member_decls {
		symtyp = FMEMBER;
	  } T_COMMA type_member_decl {
		$$ = lnklst($1, declarator_1_struct_union($4));
	  }
	;

type_member_decls:
	  type_member_decl {
		$$ = declarator_1_struct_union($1);
	  }
	| type_member_decls {
		symtyp = FMEMBER;
	  } T_COMMA type_member_decl {
		$$ = lnklst($1, declarator_1_struct_union($4));
	  }
	;

notype_member_decl:
	  notype_decl
	| notype_decl T_COLON constant_expr {		/* C99 6.7.2.1 */
		$$ = bitfield($1, to_int_constant($3, true));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant_expr {			/* C99 6.7.2.1 */
		$$ = bitfield(NULL, to_int_constant($3, true));
	  }
	;

type_member_decl:
	  type_decl
	| type_decl T_COLON constant_expr {
		$$ = bitfield($1, to_int_constant($3, true));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant_expr {
		$$ = bitfield(NULL, to_int_constant($3, true));
	  }
	;

enum_spec:
	  enum enum_tag {
		$$ = mktag($2, ENUM, false, false);
	  }
	| enum enum_tag {
		dcs->d_tagtyp = mktag($2, ENUM, true, false);
	  } enum_declaration {
		$$ = complete_tag_enum(dcs->d_tagtyp, $4);
	  }
	| enum {
		dcs->d_tagtyp = mktag(NULL, ENUM, true, false);
	  } enum_declaration {
		$$ = complete_tag_enum(dcs->d_tagtyp, $3);
	  }
	| enum error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

enum:
	  T_ENUM {
		symtyp = FTAG;
		begin_declaration_level(CTCONST);
	  }
	;

enum_tag:
	  identifier {
		$$ = getsym($1);
	  }
	;

enum_declaration:
	  enum_decl_lbrace enums_with_opt_comma T_RBRACE {
		$$ = $2;
	  }
	;

enum_decl_lbrace:
	  T_LBRACE {
		symtyp = FVFT;
		enumval = 0;
	  }
	;

enums_with_opt_comma:
	  enums
	| enums T_COMMA {
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

enums:
	  enumerator
	| enums T_COMMA enumerator {
		$$ = lnklst($1, $3);
	  }
	| error {
		$$ = NULL;
	  }
	;

enumerator:
	  enumeration_constant {
		$$ = enumeration_constant($1, enumval, true);
	  }
	| enumeration_constant T_ASSIGN constant_expr {
		$$ = enumeration_constant($1, to_int_constant($3, true), false);
	  }
	;

enumeration_constant:		/* C99 6.4.4.3 */
	  identifier {
		$$ = getsym($1);
	  }
	;


notype_init_decls:
	  notype_init_decl
	| notype_init_decls T_COMMA type_init_decl
	;

type_init_decls:
	  type_init_decl
	| type_init_decls T_COMMA type_init_decl
	;

/* See the Bison manual, section 7.1 "Semantic Info in Token Kinds". */
notype_init_decl:
	  notype_decl asm_or_symbolrename_opt {
		cgram_declare($1, false, $2);
		check_size($1);
	  }
	| notype_decl asm_or_symbolrename_opt {
		begin_initialization($1);
		cgram_declare($1, true, $2);
	  } T_ASSIGN initializer {
		check_size($1);
		end_initialization();
	  }
	;

type_init_decl:
	  type_decl asm_or_symbolrename_opt {
		cgram_declare($1, false, $2);
		check_size($1);
	  }
	| type_decl asm_or_symbolrename_opt {
		begin_initialization($1);
		cgram_declare($1, true, $2);
	  } T_ASSIGN initializer {
		check_size($1);
		end_initialization();
	  }
	;

notype_decl:
	  notype_direct_decl
	| pointer notype_direct_decl {
		$$ = add_pointer($2, $1);
	  }
	;

type_decl:
	  type_direct_decl
	| pointer type_direct_decl {
		$$ = add_pointer($2, $1);
	  }
	;

notype_direct_decl:
	  T_NAME {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN type_decl T_RPAREN {
		$$ = $2;
	  }
	| type_attribute notype_direct_decl {
		$$ = $2;
	  }
	| notype_direct_decl T_LBRACK T_RBRACK {
		$$ = add_array($1, false, 0);
	  }
	| notype_direct_decl T_LBRACK array_size T_RBRACK {
		$$ = add_array($1, true, to_int_constant($3, false));
	  }
	| notype_direct_decl param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| notype_direct_decl type_attribute_list
	;

type_direct_decl:
	  identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN type_decl T_RPAREN {
		$$ = $2;
	  }
	| type_attribute type_direct_decl {
		$$ = $2;
	  }
	| type_direct_decl T_LBRACK T_RBRACK {
		$$ = add_array($1, false, 0);
	  }
	| type_direct_decl T_LBRACK array_size T_RBRACK {
		$$ = add_array($1, true, to_int_constant($3, false));
	  }
	| type_direct_decl param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| type_direct_decl type_attribute_list
	;

/*
 * param_decl and notype_param_decl exist to avoid a conflict in
 * argument lists. A typename enclosed in parens should always be
 * treated as a typename, not an argument.
 * "typedef int a; f(int (a));" is  "typedef int a; f(int foo(a));"
 *				not "typedef int a; f(int a);"
 */
param_decl:
	  direct_param_decl
	| pointer direct_param_decl {
		$$ = add_pointer($2, $1);
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
	| constant_expr
	;

direct_param_decl:
	  identifier type_attribute_list {
		$$ = declarator_name(getsym($1));
	  }
	| identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN notype_param_decl T_RPAREN {
		$$ = $2;
	  }
	| direct_param_decl T_LBRACK T_RBRACK {
		$$ = add_array($1, false, 0);
	  }
	| direct_param_decl T_LBRACK array_size T_RBRACK {
		$$ = add_array($1, true, to_int_constant($3, false));
	  }
	| direct_param_decl param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	;

notype_param_decl:
	  direct_notype_param_decl
	| pointer direct_notype_param_decl {
		$$ = add_pointer($2, $1);
	  }
	;

direct_notype_param_decl:
	  identifier {
		$$ = declarator_name(getsym($1));
	  }
	| T_LPAREN notype_param_decl T_RPAREN {
		$$ = $2;
	  }
	| direct_notype_param_decl T_LBRACK T_RBRACK {
		$$ = add_array($1, false, 0);
	  }
	| direct_notype_param_decl T_LBRACK array_size T_RBRACK {
		$$ = add_array($1, true, to_int_constant($3, false));
	  }
	| direct_notype_param_decl param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
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

asterisk:
	  T_ASTERISK {
		$$ = xcalloc(1, sizeof(*$$));
		$$->p_pointer = true;
	  }
	;

type_qualifier_list_opt:
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

type_qualifier:
	  T_QUAL {
		$$ = xcalloc(1, sizeof(*$$));
		if ($1 == CONST) {
			$$->p_const = true;
		} else if ($1 == VOLATILE) {
			$$->p_volatile = true;
		} else {
			lint_assert($1 == RESTRICT || $1 == THREAD);
		}
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

identifier_list:
	  T_NAME {
		$$ = old_style_function_name(getsym($1));
	  }
	| identifier_list T_COMMA T_NAME {
		$$ = lnklst($1, old_style_function_name(getsym($3)));
	  }
	| identifier_list error
	;

abstract_decl_param_list:
	  abstract_decl_lparen T_RPAREN type_attribute_opt {
		$$ = NULL;
	  }
	| abstract_decl_lparen vararg_parameter_type_list T_RPAREN type_attribute_opt {
		dcs->d_proto = true;
		$$ = $2;
	  }
	| abstract_decl_lparen error T_RPAREN type_attribute_opt {
		$$ = NULL;
	  }
	;

abstract_decl_lparen:
	  T_LPAREN {
		block_level++;
		begin_declaration_level(PROTO_ARG);
	  }
	;

vararg_parameter_type_list:
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

parameter_type_list:
	  parameter_declaration
	| parameter_type_list T_COMMA parameter_declaration {
		$$ = lnklst($1, $3);
	  }
	;

parameter_declaration:
	  declmods deftyp {
		$$ = declare_argument(abstract_name(), false);
	  }
	| declaration_specifiers deftyp {
		$$ = declare_argument(abstract_name(), false);
	  }
	| declmods deftyp notype_param_decl {
		$$ = declare_argument($3, false);
	  }
	/*
	 * param_decl is needed because of following conflict:
	 * "typedef int a; f(int (a));" could be parsed as
	 * "function with argument a of type int", or
	 * "function with an abstract argument of type function".
	 * This grammar realizes the second case.
	 */
	| declaration_specifiers deftyp param_decl {
		$$ = declare_argument($3, false);
	  }
	| declmods deftyp abstract_declarator {
		$$ = declare_argument($3, false);
	  }
	| declaration_specifiers deftyp abstract_declarator {
		$$ = declare_argument($3, false);
	  }
	;

asm_or_symbolrename_opt:		/* expect only one */
	  /* empty */ {
		$$ = NULL;
	  }
	| T_ASM T_LPAREN T_STRING T_RPAREN {
		freeyyv(&$3, T_STRING);
		$$ = NULL;
	  }
	| T_SYMBOLRENAME T_LPAREN T_NAME T_RPAREN {
		$$ = $3;
	  }
	;

initializer:			/* C99 6.7.8 "Initialization" */
	  expr				%prec T_COMMA {
		init_expr($1);
	  }
	| init_lbrace init_rbrace {
		/* XXX: Empty braces are not covered by C99 6.7.8. */
	  }
	| init_lbrace initializer_list comma_opt init_rbrace
	| error
	;

initializer_list:		/* C99 6.7.8 "Initialization" */
	  initializer_list_item
	| initializer_list T_COMMA initializer_list_item
	;

initializer_list_item:
	  designation initializer
	| initializer
	;

designation:			/* C99 6.7.8 "Initialization" */
	  designator_list T_ASSIGN
	| identifier T_COLON {
		/* GCC style struct or union member name in initializer */
		gnuism(315);
		add_designator_member($1);
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
			/* array initializer with des.s is a C9X feature */
			warning(321);
	  }
	| T_POINT identifier {
		if (!Sflag)
			/* struct or union member name in initializer is ... */
			warning(313);
		add_designator_member($2);
	  }
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

init_lbrace:
	  T_LBRACE {
		init_lbrace();
	  }
	;

init_rbrace:
	  T_RBRACE {
		init_rbrace();
	  }
	;

type_name:			/* C99 6.7.6 */
	  {
		begin_declaration_level(ABSTRACT);
	  } abstract_declaration {
		end_declaration_level();
		$$ = $2->s_type;
	  }
	;

abstract_declaration:
	  noclass_declmods deftyp {
		$$ = declare_1_abstract(abstract_name());
	  }
	| noclass_declspecs deftyp {
		$$ = declare_1_abstract(abstract_name());
	  }
	| noclass_declmods deftyp abstract_declarator {
		$$ = declare_1_abstract($3);
	  }
	| noclass_declspecs deftyp abstract_declarator {
		$$ = declare_1_abstract($3);
	  }
	;

abstract_declarator:		/* C99 6.7.6 */
	  pointer {
		$$ = add_pointer(abstract_name(), $1);
	  }
	| direct_abstract_declarator
	| pointer direct_abstract_declarator {
		$$ = add_pointer($2, $1);
	  }
	| T_TYPEOF term {	/* GCC extension */
		$$ = mktempsym($2->tn_type);
	  }
	;

direct_abstract_declarator:		/* C99 6.7.6 */
	  T_LPAREN abstract_declarator T_RPAREN {
		$$ = $2;
	  }
	| T_LBRACK T_RBRACK {
		$$ = add_array(abstract_name(), false, 0);
	  }
	| T_LBRACK array_size T_RBRACK {
		$$ = add_array(abstract_name(), true, to_int_constant($2, false));
	  }
	| type_attribute direct_abstract_declarator {
		$$ = $2;
	  }
	| direct_abstract_declarator T_LBRACK T_RBRACK {
		$$ = add_array($1, false, 0);
	  }
	| direct_abstract_declarator T_LBRACK T_ASTERISK T_RBRACK { /* C99 */
		$$ = add_array($1, false, 0);
	  }
	| direct_abstract_declarator T_LBRACK array_size T_RBRACK {
		$$ = add_array($1, true, to_int_constant($3, false));
	  }
	| abstract_decl_param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename(abstract_name(), $2), $1);
		end_declaration_level();
		block_level--;
	  }
	| direct_abstract_declarator abstract_decl_param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	  }
	| direct_abstract_declarator type_attribute_list
	;

non_expr_statement:
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

statement:			/* C99 6.8 */
	  expr_statement
	| non_expr_statement
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
		freeblk();
		mem_block_level--;
		block_level--;
		seen_fallthrough = false;
	  }
	;

block_item_list:
	  block_item
	| block_item_list block_item {
		if (!Sflag && $1 && !$2)
			/* declarations after statements is a C99 feature */
			c99ism(327);
		$$ = $1 || $2;
	}
	;

block_item:
	  statement {
		$$ = true;
		restore_warning_flags();
	  }
	| declaration {
		$$ = false;
		restore_warning_flags();
	  }
	;

expr_statement:
	  expr T_SEMI {
		expr($1, false, false, false, false);
		seen_fallthrough = false;
	  }
	| T_SEMI {
		seen_fallthrough = false;
	  }
	;

selection_statement:		/* C99 6.8.4 */
	  if_without_else {
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

if_without_else:
	  if_expr statement
	| if_expr error
	;

if_expr:
	  T_IF T_LPAREN expr T_RPAREN {
		if1($3);
		clear_warning_flags();
	  }
	;

switch_expr:
	  T_SWITCH T_LPAREN expr T_RPAREN {
		switch1($3);
		clear_warning_flags();
	  }
	;

generic_selection:		/* C11 6.5.1.1 */
	  T_GENERIC T_LPAREN assignment_expression T_COMMA
	    generic_assoc_list T_RPAREN {
	  	/* generic selection requires C11 or later */
	  	c11ism(345);
		$$ = build_generic_selection($3, $5);
	  }
	;

generic_assoc_list:		/* C11 6.5.1.1 */
	  generic_association
	| generic_assoc_list T_COMMA generic_association {
		$3->gat_prev = $1;
		$$ = $3;
	  }
	;

generic_association:		/* C11 6.5.1.1 */
	  type_name T_COLON assignment_expression {
		$$ = getblk(sizeof(*$$));
		$$->gat_arg = $1;
		$$->gat_result = $3;
	  }
	| T_DEFAULT T_COLON assignment_expression {
		$$ = getblk(sizeof(*$$));
		$$->gat_arg = NULL;
		$$->gat_result = $3;
	  }
	;

do_statement:			/* C99 6.8.5 */
	  do statement {
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

while_expr:
	  T_WHILE T_LPAREN expr T_RPAREN {
		while1($3);
		clear_warning_flags();
	  }
	;

do:
	  T_DO {
		do1();
	  }
	;

do_while_expr:
	  T_WHILE T_LPAREN expr T_RPAREN T_SEMI {
		$$ = $3;
	  }
	;

for_start:
	  T_FOR T_LPAREN {
		begin_declaration_level(AUTO);
		block_level++;
	  }
	;
for_exprs:
	  for_start declaration_specifiers deftyp notype_init_decls T_SEMI
	    expr_opt T_SEMI expr_opt T_RPAREN {
		/* variable declaration in for loop */
		c99ism(325);
		for1(NULL, $6, $8);
		clear_warning_flags();
	    }
	  | for_start expr_opt T_SEMI expr_opt T_SEMI expr_opt T_RPAREN {
		for1($2, $4, $6);
		clear_warning_flags();
	  }
	;

expr_opt:
	  /* empty */ {
		$$ = NULL;
	  }
	| expr
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
	| T_RETURN T_SEMI {
		do_return(NULL);
	  }
	| T_RETURN expr T_SEMI {
		do_return($2);
	  }
	;

goto:
	  T_GOTO {
		symtyp = FLABEL;
	  }
	;

asm_statement:
	  T_ASM T_LPAREN read_until_rparen T_SEMI {
		setasm();
	  }
	| T_ASM T_QUAL T_LPAREN read_until_rparen T_SEMI {
		setasm();
	  }
	| T_ASM error
	;

read_until_rparen:
	  /* empty */ {
		ignore_up_to_rparen();
	  }
	;

constant_expr_list_opt:
	  /* empty */
	| constant_expr_list
	;

constant_expr_list:
	  constant_expr
	| constant_expr_list T_COMMA constant_expr
	;

constant_expr:			/* C99 6.6 */
	  expr %prec T_ASSIGN
	;

expr:
	  expr T_ASTERISK expr {
		$$ = build(MULT, $1, $3);
	  }
	| expr T_MULTIPLICATIVE expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_ADDITIVE expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_SHIFT expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_RELATIONAL expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_EQUALITY expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_AMPER expr {
		$$ = build(BITAND, $1, $3);
	  }
	| expr T_BITXOR expr {
		$$ = build(BITXOR, $1, $3);
	  }
	| expr T_BITOR expr {
		$$ = build(BITOR, $1, $3);
	  }
	| expr T_LOGAND expr {
		$$ = build(LOGAND, $1, $3);
	  }
	| expr T_LOGOR expr {
		$$ = build(LOGOR, $1, $3);
	  }
	| expr T_QUEST expr T_COLON expr {
		$$ = build(QUEST, $1, build(COLON, $3, $5));
	  }
	| expr T_ASSIGN expr {
		$$ = build(ASSIGN, $1, $3);
	  }
	| expr T_OPASSIGN expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_COMMA expr {
		$$ = build(COMMA, $1, $3);
	  }
	| term
	| generic_selection
	;

assignment_expression:		/* C99 6.5.16 */
	  expr %prec T_ASSIGN
	;

term:
	  T_NAME {
		/* XXX really necessary? */
		if (yychar < 0)
			yychar = yylex();
		$$ = new_name_node(getsym($1), yychar);
	  }
	| string {
		$$ = new_string_node($1);
	  }
	| T_CON {
		$$ = expr_new_constant(gettyp($1->v_tspec), $1);
	  }
	| T_LPAREN expr T_RPAREN {
		if ($2 != NULL)
			$2->tn_parenthesized = true;
		$$ = $2;
	  }
	| T_LPAREN compound_statement_lbrace gcc_statement_expr_list {
		block_level--;
		mem_block_level--;
		begin_initialization(mktempsym(dup_type($3->tn_type)));
		mem_block_level++;
		block_level++;
		/* ({ }) is a GCC extension */
		gnuism(320);
	 } compound_statement_rbrace T_RPAREN {
		$$ = new_name_node(*current_initsym(), 0);
		end_initialization();
	 }
	| term T_INCDEC {
		$$ = build($2 == INC ? INCAFT : DECAFT, $1, NULL);
	  }
	| T_INCDEC term {
		$$ = build($1 == INC ? INCBEF : DECBEF, $2, NULL);
	  }
	| T_ASTERISK term {
		$$ = build(INDIR, $2, NULL);
	  }
	| T_AMPER term {
		$$ = build(ADDR, $2, NULL);
	  }
	| T_UNARY term {
		$$ = build($1, $2, NULL);
	  }
	| T_ADDITIVE term {
		if (tflag && $1 == PLUS) {
			/* unary + is illegal in traditional C */
			warning(100);
		}
		$$ = build($1 == PLUS ? UPLUS : UMINUS, $2, NULL);
	  }
	| term T_LBRACK expr T_RBRACK {
		$$ = build(INDIR, build(PLUS, $1, $3), NULL);
	  }
	| term T_LPAREN T_RPAREN {
		$$ = new_function_call_node($1, NULL);
	  }
	| term T_LPAREN func_arg_list T_RPAREN {
		$$ = new_function_call_node($1, $3);
	  }
	| term point_or_arrow T_NAME {
		if ($1 != NULL) {
			sym_t	*msym;
			/*
			 * XXX struct_or_union_member should be integrated
			 * in build()
			 */
			if ($2 == ARROW) {
				/*
				 * must do this before struct_or_union_member
				 * is called
				 */
				$1 = cconv($1);
			}
			msym = struct_or_union_member($1, $2, getsym($3));
			$$ = build($2, $1, new_name_node(msym, 0));
		} else {
			$$ = NULL;
		}
	  }
	| T_REAL term {
		$$ = build(REAL, $2, NULL);
	  }
	| T_IMAG term {
		$$ = build(IMAG, $2, NULL);
	  }
	| T_EXTENSION term {
		$$ = $2;
	  }
	| T_REAL T_LPAREN term T_RPAREN {
		$$ = build(REAL, $3, NULL);
	  }
	| T_IMAG T_LPAREN term T_RPAREN {
		$$ = build(IMAG, $3, NULL);
	  }
	| T_BUILTIN_OFFSETOF T_LPAREN type_name T_COMMA identifier T_RPAREN {
		symtyp = FMEMBER;
		$$ = build_offsetof($3, getsym($5));
	  }
	| T_SIZEOF term	{
		$$ = $2 == NULL ? NULL : build_sizeof($2->tn_type);
		if ($$ != NULL)
			check_expr_misc($2, false, false, false, false, false, true);
	  }
	| T_SIZEOF T_LPAREN type_name T_RPAREN		%prec T_SIZEOF {
		$$ = build_sizeof($3);
	  }
	| T_ALIGNOF T_LPAREN type_name T_RPAREN {
		$$ = build_alignof($3);
	  }
	| T_LPAREN type_name T_RPAREN term		%prec T_UNARY {
		$$ = cast($4, $2);
	  }
	| T_LPAREN type_name T_RPAREN {	/* C99 6.5.2.5 "Compound literals" */
		sym_t *tmp = mktempsym($2);
		begin_initialization(tmp);
		cgram_declare(tmp, true, NULL);
	  } init_lbrace initializer_list comma_opt init_rbrace {
		if (!Sflag)
			 /* compound literals are a C9X/GCC extension */
			 gnuism(319);
		$$ = new_name_node(*current_initsym(), 0);
		end_initialization();
	  }
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
	  declaration {
		clear_warning_flags();
		$$ = NULL;
	  }
	| non_expr_statement {
		$$ = expr_zalloc_tnode();
		$$->tn_type = gettyp(VOID);
	  }
	| expr T_SEMI {
		if ($1 == NULL) {	/* in case of syntax errors */
			$$ = expr_zalloc_tnode();
			$$->tn_type = gettyp(VOID);
		} else {
			/* XXX: do that only on the last name */
			if ($1->tn_op == NAME)
				$1->tn_sym->s_used = true;
			$$ = $1;
			expr($1, false, false, false, false);
			seen_fallthrough = false;
		}
	  }
	;

string:
	  T_STRING
	| T_STRING string2 {
		$$ = cat_strings($1, $2);
	  }
	;

string2:
	  T_STRING {
		if (tflag) {
			/* concatenated strings are illegal in traditional C */
			warning(219);
		}
		$$ = $1;
	  }
	| string2 T_STRING {
		$$ = cat_strings($1, $2);
	  }
	;

func_arg_list:
	  expr						%prec T_COMMA {
		$$ = new_function_argument_node(NULL, $1);
	  }
	| func_arg_list T_COMMA expr {
		$$ = new_function_argument_node($1, $3);
	  }
	;

point_or_arrow:
	  T_POINT {
		symtyp = FMEMBER;
		$$ = POINT;
	  }
	| T_ARROW {
		symtyp = FMEMBER;
		$$ = ARROW;
	  }
	;

identifier:			/* C99 6.4.2.1 */
	  T_NAME {
		$$ = $1;
		cgram_debug("name '%s'", $$->sb_name);
	  }
	| T_TYPENAME {
		$$ = $1;
		cgram_debug("typename '%s'", $$->sb_name);
	  }
	;

comma_opt:
	  T_COMMA
	| /* empty */
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
ignore_up_to_rparen(void)
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
