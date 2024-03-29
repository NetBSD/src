%{
/* $NetBSD: cgram.y,v 1.493 2024/03/29 08:35:32 rillig Exp $ */

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

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: cgram.y,v 1.493 2024/03/29 08:35:32 rillig Exp $");
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
int block_level;

/*
 * level for memory allocation. Normally the same as block_level.
 * An exception is the declaration of parameters in prototypes. Memory
 * for these can't be freed after the declaration, but symbols must
 * be removed from the symbol table after the declaration.
 */
size_t mem_block_level;

/*
 * Save the no-warns state and restore it to avoid the problem where
 * if (expr) { stmt } / * NOLINT * / stmt;
 */
#define LWARN_NOTHING_SAVED (-3)
static int saved_lwarn = LWARN_NOTHING_SAVED;

static void cgram_declare(sym_t *, bool, sbuf_t *);
static void read_until_rparen(void);
static sym_t *symbolrename(sym_t *, sbuf_t *);


/* ARGSUSED */
static void
clear_warning_flags_loc(const char *file, size_t line)
{
	debug_step("%s:%zu: clearing flags", file, line);
	reset_suppressions();
	saved_lwarn = LWARN_NOTHING_SAVED;
}

/* ARGSUSED */
static void
save_warning_flags_loc(const char *file, size_t line)
{
	debug_step("%s:%zu: saving flags %d", file, line, lwarn);
	saved_lwarn = lwarn;
}

/* ARGSUSED */
static void
restore_warning_flags_loc(const char *file, size_t line)
{
	if (saved_lwarn != LWARN_NOTHING_SAVED) {
		lwarn = saved_lwarn;
		debug_step("%s:%zu: restoring flags %d", file, line, lwarn);
	} else
		clear_warning_flags_loc(file, line);
}

#define clear_warning_flags()	clear_warning_flags_loc(__FILE__, __LINE__)
#define save_warning_flags()	save_warning_flags_loc(__FILE__, __LINE__)
#define restore_warning_flags()	restore_warning_flags_loc(__FILE__, __LINE__)

static bool
is_either(const char *s, const char *a, const char *b)
{
	return strcmp(s, a) == 0 || strcmp(s, b) == 0;
}

#if YYDEBUG && YYBYACC
#define YYSTYPE_TOSTRING cgram_to_string
#endif

%}

%expect 103

%union {
	val_t	*y_val;
	sbuf_t	*y_name;
	sym_t	*y_sym;
	bool	y_inc;
	op_t	y_op;
	scl_t	y_scl;
	tspec_t	y_tspec;
	type_qualifiers y_type_qualifiers;
	function_specifier y_function_specifier;
	parameter_list y_parameter_list;
	function_call *y_arguments;
	type_t	*y_type;
	tnode_t	*y_tnode;
	range_t	y_range;
	buffer	*y_string;
	qual_ptr *y_qual_ptr;
	bool	y_seen_statement;
	struct generic_association *y_generic;
	array_size y_array_size;
	bool	y_in_system_header;
	designation y_designation;
};

/* for Bison:
%printer {
	if (is_integer($$->v_tspec))
		fprintf(yyo, "%lld", (long long)$$->u.integer);
	else
		fprintf(yyo, "%Lg", $$->u.floating);
} <y_val>
%printer { fprintf(yyo, "'%s'", $$ != NULL ? $$->sb_name : "<null>"); } <y_name>
%printer {
	bool indented = debug_push_indented(true);
	debug_sym("", $$, "");
	debug_pop_indented(indented);
} <y_sym>
%printer { fprintf(yyo, "%s", $$ ? "++" : "--"); } <y_inc>
%printer { fprintf(yyo, "%s", op_name($$)); } <y_op>
%printer { fprintf(yyo, "%s", scl_name($$)); } <y_scl>
%printer { fprintf(yyo, "%s", tspec_name($$)); } <y_tspec>
%printer { fprintf(yyo, "%s", type_qualifiers_string($$)); } <y_type_qualifiers>
%printer {
	fprintf(yyo, "%s", function_specifier_name($$));
} <y_function_specifier>
%printer {
	size_t n = 0;
	for (const sym_t *p = $$.first; p != NULL; p = p->s_next)
		n++;
	fprintf(yyo, "%zu parameter%s", n, n != 1 ? "s" : "");
} <y_parameter_list>
%printer { fprintf(yyo, "%s", type_name($$)); } <y_type>
%printer {
	if ($$ == NULL)
		fprintf(yyo, "<null>");
	else
		fprintf(yyo, "%s '%s'",
		    op_name($$->tn_op), type_name($$->tn_type));
} <y_tnode>
%printer { fprintf(yyo, "%zu to %zu", $$.lo, $$.hi); } <y_range>
%printer { fprintf(yyo, "length %zu", $$->len); } <y_string>
%printer {
	fprintf(yyo, "%s *", type_qualifiers_string($$->qualifiers));
} <y_qual_ptr>
%printer { fprintf(yyo, "%s", $$ ? "yes" : "no"); } <y_seen_statement>
%printer { fprintf(yyo, "%s", type_name($$->ga_arg)); } <y_generic>
%printer { fprintf(yyo, "%d", $$.dim); } <y_array_size>
%printer { fprintf(yyo, "%s", $$ ? "yes" : "no"); } <y_in_system_header>
%printer {
	if ($$.dn_len == 0)
		fprintf(yyo, "(empty)");
	for (size_t i = 0; i < $$.dn_len; i++) {
		const designator *dr = $$.dn_items + i;
		if (dr->dr_kind == DK_MEMBER)
			fprintf(yyo, ".%s", dr->dr_member->s_name);
		else if (dr->dr_kind == DK_SUBSCRIPT)
			fprintf(yyo, "[%zu]", dr->dr_subscript);
		else
			fprintf(yyo, "<scalar>");
	}
} <y_designation>
*/

%token			T_LBRACE T_RBRACE T_LBRACK T_RBRACK T_LPAREN T_RPAREN
%token			T_POINT T_ARROW
%token			T_COMPLEMENT T_LOGNOT
%token	<y_inc>		T_INCDEC
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

/* storage classes (extern, static, auto, register and typedef) */
%token	<y_scl>		T_SCLASS
%token	<y_function_specifier> T_FUNCTION_SPECIFIER

/*
 * predefined type keywords (char, int, short, long, unsigned, signed,
 * float, double, void); see T_TYPENAME for types from typedef
 */
%token	<y_tspec>	T_TYPE

%token	<y_type_qualifiers>	T_QUAL
%token	<y_type_qualifiers>	T_ATOMIC

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

/* No type for program. */
%type	<y_sym>		identifier_sym
%type	<y_name>	identifier
%type	<y_string>	string
%type	<y_tnode>	primary_expression
%type	<y_designation>	member_designator
%type	<y_tnode>	generic_selection
%type	<y_generic>	generic_assoc_list
%type	<y_generic>	generic_association
%type	<y_tnode>	postfix_expression
%type	<y_tnode>	gcc_statement_expr_list
%type	<y_tnode>	gcc_statement_expr_item
%type	<y_op>		point_or_arrow
%type	<y_arguments>	argument_expression_list
%type	<y_tnode>	unary_expression
%type	<y_tnode>	cast_expression
%type	<y_tnode>	expression_opt
%type	<y_tnode>	conditional_expression
%type	<y_tnode>	assignment_expression
%type	<y_tnode>	expression
%type	<y_tnode>	constant_expression
/* No type for declaration_or_error. */
/* No type for declaration. */
/* No type for begin_type_declaration_specifiers. */
/* No type for begin_type_declmods. */
/* No type for begin_type_specifier_qualifier_list. */
/* No type for begin_type_specifier_qualifier_list_postfix. */
%type	<y_type>	begin_type_typespec
/* No type for begin_type_qualifier_list. */
/* No type for declmod. */
/* No type for type_attribute_list_opt. */
/* No type for type_attribute_list. */
/* No type for type_attribute_opt. */
/* No type for type_attribute. */
/* No type for begin_type. */
/* No type for end_type. */
%type	<y_type>	type_specifier
%type	<y_type>	notype_type_specifier
%type	<y_type>	atomic_type_specifier
%type	<y_type>	struct_or_union_specifier
%type	<y_tspec>	struct_or_union
%type	<y_sym>		braced_member_declaration_list
%type	<y_sym>		member_declaration_list_with_rbrace
%type	<y_sym>		member_declaration_list
%type	<y_sym>		member_declaration
%type	<y_sym>		notype_member_declarators
%type	<y_sym>		type_member_declarators
%type	<y_sym>		notype_member_declarator
%type	<y_sym>		type_member_declarator
%type	<y_type>	enum_specifier
/* No type for enum. */
%type	<y_sym>		enum_declaration
%type	<y_sym>		enums_with_opt_comma
%type	<y_sym>		enumerator_list
%type	<y_sym>		enumerator
%type	<y_type_qualifiers>	type_qualifier
/* No type for atomic. */
%type	<y_qual_ptr>	pointer
%type	<y_type_qualifiers>	type_qualifier_list_opt
%type	<y_type_qualifiers>	type_qualifier_list
/* No type for notype_init_declarators. */
/* No type for type_init_declarators. */
/* No type for notype_init_declarator. */
/* No type for type_init_declarator. */
%type	<y_sym>		notype_declarator
%type	<y_sym>		type_declarator
%type	<y_sym>		notype_direct_declarator
%type	<y_sym>		type_direct_declarator
%type	<y_sym>		type_param_declarator
%type	<y_sym>		notype_param_declarator
%type	<y_sym>		direct_param_declarator
%type	<y_sym>		direct_notype_param_declarator
%type	<y_parameter_list>	param_list
%type	<y_array_size>	array_size_opt
%type	<y_sym>		identifier_list
%type	<y_type>	type_name
%type	<y_sym>		abstract_declaration
%type	<y_sym>		abstract_declarator
%type	<y_sym>		direct_abstract_declarator
%type	<y_parameter_list>	abstract_decl_param_list
/* No type for abstract_decl_lparen. */
%type	<y_parameter_list>	vararg_parameter_type_list
%type	<y_parameter_list>	parameter_type_list
%type	<y_sym>		parameter_declaration
/* No type for braced_initializer. */
/* No type for initializer. */
/* No type for initializer_list. */
/* No type for designation. */
/* No type for designator_list. */
/* No type for designator. */
/* No type for static_assert_declaration. */
%type	<y_range>	range
/* No type for init_lbrace. */
/* No type for init_rbrace. */
%type	<y_name>	asm_or_symbolrename_opt
/* No type for statement. */
/* No type for no_attr_statement. */
/* No type for non_expr_statement. */
/* No type for no_attr_non_expr_statement. */
/* No type for labeled_statement. */
/* No type for label. */
/* No type for compound_statement. */
/* No type for compound_statement_lbrace. */
/* No type for compound_statement_rbrace. */
%type	<y_seen_statement> block_item_list
%type	<y_seen_statement> block_item
/* No type for expression_statement. */
/* No type for selection_statement. */
/* No type for if_without_else. */
/* No type for if_expr. */
/* No type for switch_expr. */
/* No type for iteration_statement. */
/* No type for while_expr. */
/* No type for do_statement. */
/* No type for do. */
/* No type for for_start. */
/* No type for for_exprs. */
/* No type for jump_statement. */
/* No type for goto. */
/* No type for asm_statement. */
/* No type for read_until_rparen. */
/* No type for translation_unit. */
/* No type for external_declaration. */
/* No type for top_level_declaration. */
/* No type for function_definition. */
%type	<y_sym>		func_declarator
/* No type for arg_declaration_list_opt. */
/* No type for arg_declaration_list. */
/* No type for arg_declaration. */
/* No type for gcc_attribute_specifier_list_opt. */
/* No type for gcc_attribute_specifier_list. */
/* No type for gcc_attribute_specifier. */
/* No type for gcc_attribute_list. */
/* No type for gcc_attribute. */
/* No type for gcc_attribute_parameters. */
%type	<y_in_system_header> sys

%%

program:
	/* empty */ {
		/* TODO: Make this an error in C99 mode as well. */
		if (!allow_trad && !allow_c99)
			/* empty translation unit */
			error(272);
		else if (allow_c90)
			/* empty translation unit */
			warning(272);
	}
|	translation_unit
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
|	T_TYPENAME {
		debug_step("cgram: typename '%s'", $1->sb_name);
		$$ = $1;
	}
;

/* see C99 6.4.5, string literals are joined by 5.1.1.2 */
string:
	T_STRING
|	string T_STRING {
		if (!allow_c90)
			/* concatenated strings are illegal in traditional C */
			warning(219);
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
|	T_CON {
		$$ = build_constant(gettyp($1->v_tspec), $1);
	}
|	string {
		$$ = build_string($1);
	}
|	T_LPAREN expression T_RPAREN {
		if ($2 != NULL)
			$2->tn_parenthesized = true;
		$$ = $2;
	}
|	generic_selection
	/* GCC primary-expression, see c_parser_postfix_expression */
|	T_BUILTIN_OFFSETOF T_LPAREN type_name T_COMMA {
		set_sym_kind(SK_MEMBER);
	} member_designator T_RPAREN {
		$$ = build_offsetof($3, $6);
	}
;

/* K&R ---, C90 ---, C99 7.17p3, C11 7.19p3, C23 7.21p4 */
member_designator:
	identifier {
		$$ = (designation) { .dn_len = 0 };
		designation_push(&$$, DK_MEMBER, getsym($1), 0);
	}
|	member_designator T_LBRACK range T_RBRACK {
		$$ = $1;
		designation_push(&$$, DK_SUBSCRIPT, NULL, $3.lo);
	}
|	member_designator T_POINT {
		set_sym_kind(SK_MEMBER);
	} identifier {
		$$ = $1;
		designation_push(&$$, DK_MEMBER, getsym($4), 0);
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
|	generic_assoc_list T_COMMA generic_association {
		$3->ga_prev = $1;
		$$ = $3;
	}
;

/* K&R ---, C90 ---, C99 ---, C11 6.5.1.1 */
generic_association:
	type_name T_COLON assignment_expression {
		$$ = block_zero_alloc(sizeof(*$$), "generic");
		$$->ga_arg = $1;
		$$->ga_result = $3;
	}
|	T_DEFAULT T_COLON assignment_expression {
		$$ = block_zero_alloc(sizeof(*$$), "generic");
		$$->ga_arg = NULL;
		$$->ga_result = $3;
	}
;

/* K&R 7.1, C90 ???, C99 6.5.2, C11 6.5.2, C23 6.5.2 */
postfix_expression:
	primary_expression
|	postfix_expression T_LBRACK sys expression T_RBRACK {
		$$ = build_unary(INDIR, $3, build_binary($1, PLUS, $3, $4));
	}
|	postfix_expression T_LPAREN sys T_RPAREN {
		function_call *call =
		    expr_zero_alloc(sizeof(*call), "function_call");
		$$ = build_function_call($1, $3, call);
	}
|	postfix_expression T_LPAREN sys argument_expression_list T_RPAREN {
		$$ = build_function_call($1, $3, $4);
	}
|	postfix_expression point_or_arrow sys T_NAME {
		$$ = build_member_access($1, $2, $3, $4);
	}
|	postfix_expression T_INCDEC sys {
		$$ = build_unary($2 ? INCAFT : DECAFT, $3, $1);
	}
|	T_LPAREN type_name T_RPAREN {	/* C99 6.5.2.5 "Compound literals" */
		sym_t *tmp = mktempsym($2);
		begin_initialization(tmp);
		cgram_declare(tmp, true, NULL);
	} braced_initializer {
		if (!allow_c99)
			 /* compound literals are a C99/GCC extension */
			 gnuism(319);
		$$ = build_name(current_initsym(), false);
		end_initialization();
	}
|	T_LPAREN compound_statement_lbrace {
		begin_statement_expr();
	} gcc_statement_expr_list {
		do_statement_expr($4);
	} compound_statement_rbrace T_RPAREN {
		$$ = end_statement_expr();
	}
;

/*
 * The inner part of a GCC statement-expression of the form ({ ... }).
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
 */
gcc_statement_expr_list:
	gcc_statement_expr_item
|	gcc_statement_expr_list gcc_statement_expr_item {
		$$ = $2;
	}
;

gcc_statement_expr_item:
	declaration_or_error {
		clear_warning_flags();
		$$ = NULL;
	}
|	non_expr_statement {
		$$ = expr_alloc_tnode();
		$$->tn_type = gettyp(VOID);
	}
|	T_SEMI {
		$$ = expr_alloc_tnode();
		$$->tn_type = gettyp(VOID);
	}
|	expression T_SEMI {
		if ($1 == NULL) {	/* in case of syntax errors */
			$$ = expr_alloc_tnode();
			$$->tn_type = gettyp(VOID);
		} else {
			/* XXX: do that only on the last name */
			if ($1->tn_op == NAME)
				$1->u.sym->s_used = true;
			expr($1, false, false, false, false);
			suppress_fallthrough = false;
			$$ = $1;
		}
	}
;

point_or_arrow:			/* helper for 'postfix_expression' */
	T_POINT {
		set_sym_kind(SK_MEMBER);
		$$ = POINT;
	}
|	T_ARROW {
		set_sym_kind(SK_MEMBER);
		$$ = ARROW;
	}
;

/* K&R 7.1, C90 ???, C99 6.5.2, C11 6.5.2 */
argument_expression_list:
	assignment_expression {
		$$ = expr_zero_alloc(sizeof(*$$), "function_call");
		add_function_argument($$, $1);
	}
|	argument_expression_list T_COMMA assignment_expression {
		$$ = $1;
		add_function_argument($1, $3);
	}
;

/* K&R 7.2, C90 ???, C99 6.5.3, C11 6.5.3 */
unary_expression:
	postfix_expression
|	T_INCDEC sys unary_expression {
		$$ = build_unary($1 ? INCBEF : DECBEF, $2, $3);
	}
|	T_AMPER sys cast_expression {
		$$ = build_unary(ADDR, $2, $3);
	}
|	T_ASTERISK sys cast_expression {
		$$ = build_unary(INDIR, $2, $3);
	}
|	T_ADDITIVE sys cast_expression {
		if (!allow_c90 && $1 == PLUS)
			/* unary '+' is illegal in traditional C */
			warning(100);
		$$ = build_unary($1 == PLUS ? UPLUS : UMINUS, $2, $3);
	}
|	T_COMPLEMENT sys cast_expression {
		$$ = build_unary(COMPL, $2, $3);
	}
|	T_LOGNOT sys cast_expression {
		$$ = build_unary(NOT, $2, $3);
	}
|	T_REAL sys cast_expression {	/* GCC c_parser_unary_expression */
		$$ = build_unary(REAL, $2, $3);
	}
|	T_IMAG sys cast_expression {	/* GCC c_parser_unary_expression */
		$$ = build_unary(IMAG, $2, $3);
	}
|	T_EXTENSION cast_expression {	/* GCC c_parser_unary_expression */
		$$ = $2;
	}
|	T_SIZEOF unary_expression {
		$$ = $2 == NULL ? NULL : build_sizeof($2->tn_type);
		if ($$ != NULL)
			check_expr_misc($2,
			    false, false, false, false, false, true);
	}
|	T_SIZEOF T_LPAREN type_name T_RPAREN {
		$$ = build_sizeof($3);
	}
|	T_ALIGNOF unary_expression {
		/* non type argument to alignof is a GCC extension */
		gnuism(349);
		lint_assert($2 != NULL);
		$$ = build_alignof($2->tn_type);
	}
	/* K&R ---, C90 ---, C99 ---, C11 6.5.3 */
|	T_ALIGNOF T_LPAREN type_name T_RPAREN {
		/* TODO: c11ism */
		$$ = build_alignof($3);
	}
;

/* The rule 'unary_operator' is inlined into unary_expression. */

/* K&R 7.2, C90 ???, C99 6.5.4, C11 6.5.4 */
cast_expression:
	unary_expression
|	T_LPAREN type_name T_RPAREN sys cast_expression {
		$$ = cast($5, $4, $2);
	}
;

expression_opt:
	/* empty */ {
		$$ = NULL;
	}
|	expression
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
|	conditional_expression T_ASTERISK sys conditional_expression {
		$$ = build_binary($1, MULT, $3, $4);
	}
|	conditional_expression T_MULTIPLICATIVE sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
|	conditional_expression T_ADDITIVE sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
|	conditional_expression T_SHIFT sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
|	conditional_expression T_RELATIONAL sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
|	conditional_expression T_EQUALITY sys conditional_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
|	conditional_expression T_AMPER sys conditional_expression {
		$$ = build_binary($1, BITAND, $3, $4);
	}
|	conditional_expression T_BITXOR sys conditional_expression {
		$$ = build_binary($1, BITXOR, $3, $4);
	}
|	conditional_expression T_BITOR sys conditional_expression {
		$$ = build_binary($1, BITOR, $3, $4);
	}
|	conditional_expression T_LOGAND sys conditional_expression {
		$$ = build_binary($1, LOGAND, $3, $4);
	}
|	conditional_expression T_LOGOR sys conditional_expression {
		$$ = build_binary($1, LOGOR, $3, $4);
	}
|	conditional_expression T_QUEST sys
	    expression T_COLON sys conditional_expression {
		$$ = build_binary($1, QUEST, $3,
		    build_binary($4, COLON, $6, $7));
	}
;

/* K&R ???, C90 ???, C99 6.5.16, C11 6.5.16 */
assignment_expression:
	conditional_expression
|	unary_expression T_ASSIGN sys assignment_expression {
		$$ = build_binary($1, ASSIGN, $3, $4);
	}
|	unary_expression T_OPASSIGN sys assignment_expression {
		$$ = build_binary($1, $2, $3, $4);
	}
;

/* K&R ???, C90 ???, C99 6.5.17, C11 6.5.17 */
expression:
	assignment_expression
|	expression T_COMMA sys assignment_expression {
		$$ = build_binary($1, COMMA, $3, $4);
	}
;

/* K&R ???, C90 ???, C99 6.6, C11 ???, C23 6.6 */
constant_expression:
	conditional_expression
;

declaration_or_error:
	declaration
|	error T_SEMI
;

/* K&R ???, C90 ???, C99 6.7, C11 ???, C23 6.7 */
declaration:
	begin_type_declmods end_type T_SEMI {
		if (dcs->d_scl == TYPEDEF)
			/* typedef declares no type name */
			warning(72);
		else
			/* empty declaration */
			warning(2);
	}
|	begin_type_declmods end_type notype_init_declarators T_SEMI {
		if (dcs->d_scl == TYPEDEF)
			/* syntax error '%s' */
			error(249, "missing base type for typedef");
		else
			/* old-style declaration; add 'int' */
			error(1);
	}
|	begin_type_declaration_specifiers end_type T_SEMI {
		if (dcs->d_scl == TYPEDEF)
			/* typedef declares no type name */
			warning(72);
		else if (!dcs->d_nonempty_decl)
			/* empty declaration */
			warning(2);
	}
|	begin_type_declaration_specifiers end_type
	    type_init_declarators T_SEMI
|	static_assert_declaration
;

begin_type_declaration_specifiers:	/* see C99 6.7 */
	begin_type_typespec {
		dcs_add_type($1);
	}
|	begin_type_declmods type_specifier {
		dcs_add_type($2);
	}
|	type_attribute begin_type_declaration_specifiers
|	begin_type_declaration_specifiers declmod
|	begin_type_declaration_specifiers notype_type_specifier {
		dcs_add_type($2);
	}
;

begin_type_declmods:		/* see C99 6.7 */
	begin_type type_qualifier {
		dcs_add_qualifiers($2);
	}
|	begin_type T_SCLASS {
		dcs_add_storage_class($2);
	}
|	begin_type T_FUNCTION_SPECIFIER {
		dcs_add_function_specifier($2);
	}
|	begin_type_declmods declmod
;

begin_type_specifier_qualifier_list:	/* see C11 6.7.2.1 */
	begin_type_specifier_qualifier_list_postfix
|	type_attribute_list begin_type_specifier_qualifier_list_postfix
;

begin_type_specifier_qualifier_list_postfix:
	begin_type_typespec {
		dcs_add_type($1);
	}
|	begin_type_qualifier_list type_specifier {
		dcs_add_type($2);
	}
|	begin_type_specifier_qualifier_list_postfix type_qualifier {
		dcs_add_qualifiers($2);
	}
|	begin_type_specifier_qualifier_list_postfix notype_type_specifier {
		dcs_add_type($2);
	}
|	begin_type_specifier_qualifier_list_postfix type_attribute
;

begin_type_typespec:
	begin_type notype_type_specifier {
		$$ = $2;
	}
|	begin_type T_TYPENAME {
		$$ = getsym($2)->s_type;
	}
;

begin_type_qualifier_list:
	begin_type type_qualifier {
		dcs_add_qualifiers($2);
	}
|	begin_type_qualifier_list type_qualifier {
		dcs_add_qualifiers($2);
	}
;

declmod:
	type_qualifier {
		dcs_add_qualifiers($1);
	}
|	T_SCLASS {
		dcs_add_storage_class($1);
	}
|	T_FUNCTION_SPECIFIER {
		dcs_add_function_specifier($1);
	}
|	type_attribute_list
;

type_attribute_list_opt:
	/* empty */
|	type_attribute_list
;

type_attribute_list:
	type_attribute
|	type_attribute_list type_attribute
;

type_attribute_opt:
	/* empty */
|	type_attribute
;

type_attribute:			/* See C11 6.7 declaration-specifiers */
	gcc_attribute_specifier
|	T_ALIGNAS T_LPAREN type_specifier T_RPAREN	/* C11 6.7.5 */
|	T_ALIGNAS T_LPAREN constant_expression T_RPAREN	/* C11 6.7.5 */
|	T_PACKED {
		dcs_add_packed();
	}
;

begin_type:
	/* empty */ {
		dcs_begin_type();
	}
;

end_type:
	/* empty */ {
		dcs_end_type();
	}
;

type_specifier:			/* C99 6.7.2 */
	notype_type_specifier
|	T_TYPENAME {
		$$ = getsym($1)->s_type;
	}
;

notype_type_specifier:		/* see C99 6.7.2 */
	T_TYPE {
		$$ = gettyp($1);
	}
|	T_TYPEOF T_LPAREN expression T_RPAREN {	/* GCC extension */
		$$ = $3 != NULL ? block_dup_type($3->tn_type) : gettyp(INT);
		$$->t_typeof = true;
	}
|	atomic_type_specifier
|	struct_or_union_specifier {
		end_declaration_level();
		$$ = $1;
	}
|	enum_specifier {
		end_declaration_level();
		$$ = $1;
	}
;

/* K&R ---, C90 ---, C99 ---, C11 6.7.2.4 */
atomic_type_specifier:
	atomic T_LPAREN type_name T_RPAREN {
		$$ = $3;
	}
;

/* K&R ---, C90 ---, C99 6.7.2.1, C11 ???, C23 6.7.2.1 */
struct_or_union_specifier:
	struct_or_union identifier_sym {
		/*
		 * STDC requires that "struct a;" always introduces
		 * a new tag if "a" is not declared at current level
		 *
		 * yychar is valid because otherwise the parser would not
		 * have been able to decide if it must shift or reduce
		 */
		$$ = make_tag_type($2, $1, false, yychar == T_SEMI);
	}
|	struct_or_union identifier_sym {
		dcs->d_tag_type = make_tag_type($2, $1, true, false);
	} braced_member_declaration_list {
		$$ = complete_struct_or_union($4);
	}
|	struct_or_union {
		dcs->d_tag_type = make_tag_type(NULL, $1, true, false);
	} braced_member_declaration_list {
		$$ = complete_struct_or_union($3);
	}
|	struct_or_union error {
		set_sym_kind(SK_VCFT);
		$$ = gettyp(INT);
	}
;

/* K&R ---, C90 ---, C99 6.7.2.1, C11 ???, C23 6.7.2.1 */
struct_or_union:
	T_STRUCT_OR_UNION {
		set_sym_kind(SK_TAG);
		begin_declaration_level($1 == STRUCT ? DLK_STRUCT : DLK_UNION);
		dcs->d_sou_size_in_bits = 0;
		dcs->d_sou_align_in_bits = CHAR_SIZE;
		$$ = $1;
	}
|	struct_or_union type_attribute
;

braced_member_declaration_list:	/* see C99 6.7.2.1 */
	T_LBRACE {
		set_sym_kind(SK_VCFT);
	} member_declaration_list_with_rbrace {
		$$ = $3;
	}
;

member_declaration_list_with_rbrace:	/* see C99 6.7.2.1 */
	member_declaration_list T_RBRACE
|	T_RBRACE {
		/* XXX: Allowed since C23. */
		$$ = NULL;
	}
;

/* K&R ???, C90 ???, C99 6.7.2.1, C11 6.7.2.1, C23 6.7.2.1 */
/* Was named struct_declaration_list until C11. */
member_declaration_list:
	member_declaration
|	member_declaration_list member_declaration {
		$$ = concat_symbols($1, $2);
	}
;

/* Was named struct_declaration until C11. */
/* K&R ???, C90 ???, C99 6.7.2.1, C11 6.7.2.1, C23 6.7.2.1 */
member_declaration:
	begin_type_qualifier_list end_type {
		/* ^^ There is no check for the missing type-specifier. */
		/* too late, i know, but getsym() compensates it */
		set_sym_kind(SK_MEMBER);
	} notype_member_declarators T_SEMI {
		set_sym_kind(SK_VCFT);
		$$ = $4;
	}
|	begin_type_specifier_qualifier_list end_type {
		set_sym_kind(SK_MEMBER);
	} type_member_declarators T_SEMI {
		set_sym_kind(SK_VCFT);
		$$ = $4;
	}
|	begin_type_qualifier_list end_type type_attribute_opt T_SEMI {
		/* syntax error '%s' */
		error(249, "member without type");
		$$ = NULL;
	}
|	begin_type_specifier_qualifier_list end_type type_attribute_opt
	    T_SEMI {
		set_sym_kind(SK_VCFT);
		if (!allow_c11 && !allow_gcc)
			/* anonymous struct/union members is a C11 feature */
			warning(49);
		if (is_struct_or_union(dcs->d_type->t_tspec))
			$$ = declare_unnamed_member();
		else {
			/* syntax error '%s' */
			error(249, "unnamed member");
			$$ = NULL;
		}
	}
|	static_assert_declaration {
		$$ = NULL;
	}
|	error T_SEMI {
		set_sym_kind(SK_VCFT);
		$$ = NULL;
	}
;

/* Was named struct_declarators until C11. */
notype_member_declarators:
	notype_member_declarator {
		$$ = declare_member($1);
	}
|	notype_member_declarators {
		set_sym_kind(SK_MEMBER);
	} T_COMMA type_member_declarator {
		$$ = concat_symbols($1, declare_member($4));
	}
;

/* Was named struct_declarators until C11. */
type_member_declarators:
	type_member_declarator {
		$$ = declare_member($1);
	}
|	type_member_declarators {
		set_sym_kind(SK_MEMBER);
	} T_COMMA type_member_declarator {
		$$ = concat_symbols($1, declare_member($4));
	}
;

/* Was named struct_declarator until C11. */
notype_member_declarator:
	notype_declarator
	/* C99 6.7.2.1 */
|	notype_declarator T_COLON constant_expression {
		$$ = set_bit_field_width($1, to_int_constant($3, true));
	}
	/* C99 6.7.2.1 */
|	{
		set_sym_kind(SK_VCFT);
	} T_COLON constant_expression {
		$$ = set_bit_field_width(NULL, to_int_constant($3, true));
	}
;

/* Was named struct_declarator until C11. */
type_member_declarator:
	type_declarator
|	type_declarator T_COLON constant_expression type_attribute_list_opt {
		$$ = set_bit_field_width($1, to_int_constant($3, true));
	}
|	{
		set_sym_kind(SK_VCFT);
	} T_COLON constant_expression type_attribute_list_opt {
		$$ = set_bit_field_width(NULL, to_int_constant($3, true));
	}
;

/* K&R ---, C90 6.5.2.2, C99 6.7.2.2, C11 6.7.2.2 */
enum_specifier:
	enum gcc_attribute_specifier_list_opt identifier_sym {
		$$ = make_tag_type($3, ENUM, false, false);
	}
|	enum gcc_attribute_specifier_list_opt identifier_sym {
		dcs->d_tag_type = make_tag_type($3, ENUM, true, false);
	} enum_declaration /*gcc_attribute_specifier_list_opt*/ {
		$$ = complete_enum($5);
	}
|	enum gcc_attribute_specifier_list_opt {
		dcs->d_tag_type = make_tag_type(NULL, ENUM, true, false);
	} enum_declaration /*gcc_attribute_specifier_list_opt*/ {
		$$ = complete_enum($4);
	}
|	enum error {
		set_sym_kind(SK_VCFT);
		$$ = gettyp(INT);
	}
;

enum:				/* helper for C99 6.7.2.2 */
	T_ENUM {
		set_sym_kind(SK_TAG);
		begin_declaration_level(DLK_ENUM);
	}
;

enum_declaration:		/* helper for C99 6.7.2.2 */
	T_LBRACE {
		set_sym_kind(SK_VCFT);
		enumval = 0;
	} enums_with_opt_comma T_RBRACE {
		$$ = $3;
	}
;

enums_with_opt_comma:		/* helper for C99 6.7.2.2 */
	enumerator_list
|	enumerator_list T_COMMA {
		if (!allow_c99 && !allow_trad)
			/* trailing ',' in enum declaration requires C99 ... */
			error(54);
		else
			/* trailing ',' in enum declaration requires C99 ... */
			c99ism(54);
		$$ = $1;
	}
;

enumerator_list:		/* C99 6.7.2.2 */
	enumerator
|	enumerator_list T_COMMA enumerator {
		$$ = concat_symbols($1, $3);
	}
|	error {
		$$ = NULL;
	}
;

enumerator:			/* C99 6.7.2.2 */
	identifier_sym gcc_attribute_specifier_list_opt {
		$$ = enumeration_constant($1, enumval, true);
	}
|	identifier_sym gcc_attribute_specifier_list_opt
	    T_ASSIGN constant_expression {
		$$ = enumeration_constant($1, to_int_constant($4, true),
		    false);
	}
;

type_qualifier:			/* C99 6.7.3 */
	T_QUAL
|	atomic {
		$$ = (type_qualifiers){ .tq_atomic = true };
	}
;

atomic:				/* helper */
	T_ATOMIC {
		/* TODO: First fix c11ism, then use it here. */
		if (!allow_c11)
			/* '_Atomic' requires C11 or later */
			error(350);
	}
;

pointer:			/* C99 6.7.5 */
	T_ASTERISK type_qualifier_list_opt {
		$$ = xcalloc(1, sizeof(*$$));
		add_type_qualifiers(&$$->qualifiers, $2);
	}
|	T_ASTERISK type_qualifier_list_opt pointer {
		$$ = xcalloc(1, sizeof(*$$));
		add_type_qualifiers(&$$->qualifiers, $2);
		$$ = append_qualified_pointer($$, $3);
	}
;

type_qualifier_list_opt:	/* see C99 6.7.5 */
	/* empty */ {
		$$ = (type_qualifiers){ .tq_const = false };
	}
|	type_qualifier_list
;

type_qualifier_list:		/* C99 6.7.5 */
	type_qualifier
|	type_qualifier_list type_qualifier {
		$$ = $1;
		add_type_qualifiers(&$$, $2);
	}
;

/*
 * For an explanation of 'notype' in the following rules, see
 * https://www.gnu.org/software/bison/manual/bison.html#Semantic-Tokens.
 */

notype_init_declarators:
	notype_init_declarator
|	notype_init_declarators T_COMMA type_init_declarator
;

type_init_declarators:
	type_init_declarator
|	type_init_declarators T_COMMA type_init_declarator
;

notype_init_declarator:
	notype_declarator asm_or_symbolrename_opt {
		cgram_declare($1, false, $2);
		check_size($1);
	}
|	notype_declarator asm_or_symbolrename_opt {
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
|	type_declarator asm_or_symbolrename_opt {
		begin_initialization($1);
		cgram_declare($1, true, $2);
	} T_ASSIGN initializer {
		check_size($1);
		end_initialization();
	}
;

notype_declarator:
	notype_direct_declarator
|	pointer notype_direct_declarator {
		$$ = add_pointer($2, $1);
	}
;

type_declarator:
	type_direct_declarator
|	pointer type_direct_declarator {
		$$ = add_pointer($2, $1);
	}
;

notype_direct_declarator:
	type_attribute_list_opt T_NAME {
		$$ = declarator_name(getsym($2));
	}
|	type_attribute_list_opt T_LPAREN type_declarator T_RPAREN {
		$$ = $3;
	}
|	notype_direct_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	}
|	notype_direct_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	}
|	notype_direct_declarator type_attribute
;

type_direct_declarator:
	type_attribute_list_opt identifier {
		$$ = declarator_name(getsym($2));
	}
|	type_attribute_list_opt T_LPAREN type_declarator T_RPAREN {
		$$ = $3;
	}
|	type_direct_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	}
|	type_direct_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	}
|	type_direct_declarator type_attribute
;

/*
 * The two distinct rules type_param_declarator and notype_param_declarator
 * avoid a conflict in parameter lists. A typename enclosed in parentheses is
 * always treated as a typename, not an argument name. For example, after
 * "typedef double a;", the declaration "f(int (a));" is interpreted as
 * "f(int (double));", not "f(int a);".
 */
type_param_declarator:
	direct_param_declarator
|	pointer direct_param_declarator {
		$$ = add_pointer($2, $1);
	}
;

notype_param_declarator:
	direct_notype_param_declarator
|	pointer direct_notype_param_declarator {
		$$ = add_pointer($2, $1);
	}
;

direct_param_declarator:
	identifier type_attribute_list {
		$$ = declarator_name(getsym($1));
	}
|	identifier {
		$$ = declarator_name(getsym($1));
	}
|	T_LPAREN notype_param_declarator T_RPAREN {
		$$ = $2;
	}
|	direct_param_declarator T_LBRACK array_size_opt T_RBRACK
	    gcc_attribute_specifier_list_opt {
		$$ = add_array($1, $3.has_dim, $3.dim);
	}
|	direct_param_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	}
;

direct_notype_param_declarator:
	identifier {
		$$ = declarator_name(getsym($1));
	}
|	T_LPAREN notype_param_declarator T_RPAREN {
		$$ = $2;
	}
|	direct_notype_param_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	}
|	direct_notype_param_declarator param_list asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	}
;

param_list:
	T_LPAREN {
		block_level++;
		begin_declaration_level(DLK_PROTO_PARAMS);
	} identifier_list T_RPAREN {
		$$ = (parameter_list){ .first = $3 };
	}
|	abstract_decl_param_list
;

array_size_opt:
	/* empty */ {
		$$.has_dim = false;
		$$.dim = 0;
	}
|	T_ASTERISK {
		/* since C99; variable length array of unspecified size */
		$$.has_dim = false; /* TODO: maybe change to true */
		$$.dim = 0;	/* just as a placeholder */
	}
|	type_qualifier_list_opt T_SCLASS constant_expression {
		/* C11 6.7.6.3p7 */
		if ($2 != STATIC)
			yyerror("Bad attribute");
		/* static array size requires C11 or later */
		c11ism(343);
		$$.has_dim = true;
		$$.dim = $3 == NULL ? 0 : to_int_constant($3, false);
	}
|	type_qualifier {
		/* C11 6.7.6.2 */
		if (!$1.tq_restrict)
			yyerror("Bad attribute");
		$$.has_dim = true;
		$$.dim = 0;
	}
|	constant_expression {
		$$.has_dim = true;
		$$.dim = $1 == NULL ? 0 : to_int_constant($1, false);
	}
;

identifier_list:		/* C99 6.7.5 */
	T_NAME {
		$$ = old_style_function_parameter_name(getsym($1));
	}
|	identifier_list T_COMMA T_NAME {
		$$ = concat_symbols($1,
		    old_style_function_parameter_name(getsym($3)));
	}
|	identifier_list error
;

/* XXX: C99 requires an additional specifier-qualifier-list. */
type_name:			/* C99 6.7.6 */
	{
		begin_declaration_level(DLK_ABSTRACT);
	} abstract_declaration {
		end_declaration_level();
		$$ = $2->s_type;
	}
;

abstract_declaration:		/* specific to lint */
	begin_type_qualifier_list end_type {
		$$ = declare_abstract_type(abstract_name());
	}
|	begin_type_specifier_qualifier_list end_type {
		$$ = declare_abstract_type(abstract_name());
	}
|	begin_type_qualifier_list end_type abstract_declarator {
		$$ = declare_abstract_type($3);
	}
|	begin_type_specifier_qualifier_list end_type abstract_declarator {
		$$ = declare_abstract_type($3);
	}
;

/* K&R 8.7, C90 ???, C99 6.7.6, C11 6.7.7 */
/* In K&R, abstract-declarator could be empty and was still simpler. */
abstract_declarator:
	pointer {
		$$ = add_pointer(abstract_name(), $1);
	}
|	direct_abstract_declarator
|	pointer direct_abstract_declarator {
		$$ = add_pointer($2, $1);
	}
|	type_attribute_list direct_abstract_declarator {
		$$ = $2;
	}
|	pointer type_attribute_list direct_abstract_declarator {
		$$ = add_pointer($3, $1);
	}
;

/* K&R ---, C90 ???, C99 6.7.6, C11 6.7.7 */
direct_abstract_declarator:
	/* TODO: sort rules according to C99 */
	T_LPAREN abstract_declarator T_RPAREN {
		$$ = $2;
	}
|	T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array(abstract_name(), $2.has_dim, $2.dim);
	}
|	direct_abstract_declarator T_LBRACK array_size_opt T_RBRACK {
		$$ = add_array($1, $3.has_dim, $3.dim);
	}
|	abstract_decl_param_list asm_or_symbolrename_opt {
		sym_t *name = abstract_enclosing_name();
		$$ = add_function(symbolrename(name, $2), $1);
		end_declaration_level();
		block_level--;
	}
|	direct_abstract_declarator abstract_decl_param_list
	    asm_or_symbolrename_opt {
		$$ = add_function(symbolrename($1, $3), $2);
		end_declaration_level();
		block_level--;
	}
|	direct_abstract_declarator type_attribute_list
;

abstract_decl_param_list:	/* specific to lint */
	abstract_decl_lparen T_RPAREN type_attribute_opt {
		$$ = (parameter_list){ .first = NULL };
	}
|	abstract_decl_lparen vararg_parameter_type_list T_RPAREN
	    type_attribute_opt {
		$$ = $2;
		$$.prototype = true;
	}
|	abstract_decl_lparen error T_RPAREN type_attribute_opt {
		$$ = (parameter_list){ .first = NULL };
	}
;

abstract_decl_lparen:		/* specific to lint */
	T_LPAREN {
		block_level++;
		begin_declaration_level(DLK_PROTO_PARAMS);
	}
;

vararg_parameter_type_list:	/* specific to lint */
	parameter_type_list
|	parameter_type_list T_COMMA T_ELLIPSIS {
		$$ = $1;
		$$.vararg = true;
	}
|	T_ELLIPSIS {
		/* TODO: C99 6.7.5 makes this an error as well. */
		if (!allow_trad && !allow_c99)
			/* C90 to C17 require formal parameter before '...' */
			error(84);
		else if (allow_c90)
			/* C90 to C17 require formal parameter before '...' */
			warning(84);
		$$ = (parameter_list){ .vararg = true };
	}
;

/* XXX: C99 6.7.5 defines the same name, but it looks different. */
parameter_type_list:
	parameter_declaration {
		$$ = (parameter_list){ .first = $1 };
	}
|	parameter_type_list T_COMMA parameter_declaration {
		$$ = $1;
		$$.first = concat_symbols($1.first, $3);
	}
;

/* XXX: C99 6.7.5 defines the same name, but it looks completely different. */
parameter_declaration:
	begin_type_declmods end_type {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_parameter(abstract_name(), false);
	}
|	begin_type_declaration_specifiers end_type {
		$$ = declare_parameter(abstract_name(), false);
	}
|	begin_type_declmods end_type notype_param_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_parameter($3, false);
	}
	/*
	 * type_param_declarator is needed because of following conflict:
	 * "typedef int a; f(int (a));" could be parsed as
	 * "function with argument a of type int", or
	 * "function with an unnamed (abstract) argument of type function".
	 * This grammar realizes the second case.
	 */
|	begin_type_declaration_specifiers end_type type_param_declarator {
		$$ = declare_parameter($3, false);
	}
|	begin_type_declmods end_type abstract_declarator {
		/* ^^ There is no check for the missing type-specifier. */
		$$ = declare_parameter($3, false);
	}
|	begin_type_declaration_specifiers end_type abstract_declarator {
		$$ = declare_parameter($3, false);
	}
;

braced_initializer:
	/* K&R ---, C90 ---, C99 ---, C11 ---, C23 6.7.10 */
	init_lbrace init_rbrace {
		/* empty initializer braces require C23 or later */
		c23ism(353);
	}
	/* K&R ---, C90 ---, C99 6.7.8, C11 6.7.9, C23 6.7.10 */
|	init_lbrace initializer_list init_rbrace
|	init_lbrace initializer_list T_COMMA init_rbrace
;

initializer:			/* C99 6.7.8 "Initialization" */
	assignment_expression {
		init_expr($1);
	}
|	init_lbrace init_rbrace {
		/* XXX: Empty braces are not covered by C99 6.7.8. */
	}
|	init_lbrace initializer_list init_rbrace
|	init_lbrace initializer_list T_COMMA init_rbrace
	/* XXX: What is this error handling for? */
|	error
;

initializer_list:		/* C99 6.7.8 "Initialization" */
	initializer
|	designation initializer
|	initializer_list T_COMMA initializer
|	initializer_list T_COMMA designation initializer
;

designation:			/* C99 6.7.8 "Initialization" */
	{
		begin_designation();
	} designator_list T_ASSIGN
|	identifier T_COLON {
		/* GCC style struct or union member name in initializer */
		gnuism(315);
		begin_designation();
		add_designator_member($1);
	}
;

designator_list:		/* C99 6.7.8 "Initialization" */
	designator
|	designator_list designator
;

designator:			/* C99 6.7.8 "Initialization" */
	T_LBRACK range T_RBRACK {
		if (!allow_c99)
			/* array initializer with designators is a C99 ... */
			warning(321);
		add_designator_subscript($2);
	}
|	T_POINT identifier {
		if (!allow_c99)
			/* struct or union member name in initializer is ... */
			warning(313);
		add_designator_member($2);
	}
;

static_assert_declaration:
	T_STATIC_ASSERT T_LPAREN constant_expression T_COMMA T_STRING
	    T_RPAREN T_SEMI {
		/* '_Static_assert' requires C11 or later */
		c11ism(354);
	}
|	T_STATIC_ASSERT T_LPAREN constant_expression T_RPAREN T_SEMI {
		/* '_Static_assert' without message requires C23 or later */
		c23ism(355);
	}
;

range:
	constant_expression {
		$$.lo = to_int_constant($1, true);
		$$.hi = $$.lo;
	}
|	constant_expression T_ELLIPSIS constant_expression {
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
|	T_ASM T_LPAREN T_STRING T_RPAREN gcc_attribute_specifier_list_opt {
		freeyyv(&$3, T_STRING);
		$$ = NULL;
	}
|	T_SYMBOLRENAME T_LPAREN T_NAME T_RPAREN
	    gcc_attribute_specifier_list_opt {
		$$ = $3;
	}
;

/* K&R ???, C90 ???, C99 6.8, C11 ???, C23 6.8 */
statement:
	expression_statement
|	non_expr_statement
;

/* Helper to avoid shift/reduce conflict in 'label: __attribute__ ;'. */
no_attr_statement:
	expression_statement
|	no_attr_non_expr_statement
;

non_expr_statement:		/* helper for C99 6.8 */
	gcc_attribute_specifier /* ((__fallthrough__)) */ T_SEMI
|	no_attr_non_expr_statement
;

/* Helper to avoid shift/reduce conflict in 'label: __attribute__ ;'. */
no_attr_non_expr_statement:
	labeled_statement
|	compound_statement
|	selection_statement
|	iteration_statement
|	jump_statement {
		suppress_fallthrough = false;
	}
|	asm_statement
;

labeled_statement:		/* C99 6.8.1 */
	label gcc_attribute_specifier_list_opt no_attr_statement
;

label:
	T_NAME T_COLON {
		set_sym_kind(SK_LABEL);
		named_label(getsym($1));
	}
|	T_CASE constant_expression T_COLON {
		case_label($2);
		suppress_fallthrough = true;
	}
|	T_CASE constant_expression T_ELLIPSIS constant_expression T_COLON {
		/* XXX: We don't fill all cases */
		case_label($2);
		suppress_fallthrough = true;
	}
|	T_DEFAULT T_COLON {
		default_label();
		suppress_fallthrough = true;
	}
;

compound_statement:		/* C99 6.8.2 */
	compound_statement_lbrace compound_statement_rbrace
|	compound_statement_lbrace block_item_list compound_statement_rbrace
;

compound_statement_lbrace:
	T_LBRACE {
		block_level++;
		mem_block_level++;
		debug_step("%s: mem_block_level = %zu",
		    "compound_statement_lbrace", mem_block_level);
		begin_declaration_level(DLK_AUTO);
	}
;

compound_statement_rbrace:
	T_RBRACE {
		end_declaration_level();
		if (!in_statement_expr())
			level_free_all(mem_block_level);	/* leak */
		mem_block_level--;
		debug_step("%s: mem_block_level = %zu",
		    "compound_statement_rbrace", mem_block_level);
		block_level--;
		suppress_fallthrough = false;
	}
;

block_item_list:		/* C99 6.8.2 */
	block_item
|	block_item_list block_item {
		if ($1 && !$2)
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
|	statement {
		$$ = true;
		restore_warning_flags();
	}
;

expression_statement:		/* C99 6.8.3 */
	expression T_SEMI {
		expr($1, false, false, false, false);
		suppress_fallthrough = false;
	}
|	T_SEMI {
		check_statement_reachable();
		suppress_fallthrough = false;
	}
;

selection_statement:		/* C99 6.8.4 */
	if_without_else %prec T_THEN {
		save_warning_flags();
		stmt_if_then_stmt();
		stmt_if_else_stmt(false);
	}
|	if_without_else T_ELSE {
		save_warning_flags();
		stmt_if_then_stmt();
	} statement {
		clear_warning_flags();
		stmt_if_else_stmt(true);
	}
|	if_without_else T_ELSE error {
		clear_warning_flags();
		stmt_if_else_stmt(false);
	}
|	switch_expr statement {
		clear_warning_flags();
		stmt_switch_expr_stmt();
	}
|	switch_expr error {
		clear_warning_flags();
		stmt_switch_expr_stmt();
	}
;

if_without_else:		/* see C99 6.8.4 */
	if_expr statement
|	if_expr error
;

if_expr:			/* see C99 6.8.4 */
	T_IF T_LPAREN expression T_RPAREN {
		stmt_if_expr($3);
		clear_warning_flags();
	}
;

switch_expr:			/* see C99 6.8.4 */
	T_SWITCH T_LPAREN expression T_RPAREN {
		stmt_switch_expr($3);
		clear_warning_flags();
	}
;

iteration_statement:		/* C99 6.8.5 */
	while_expr statement {
		clear_warning_flags();
		stmt_while_expr_stmt();
	}
|	while_expr error {
		clear_warning_flags();
		stmt_while_expr_stmt();
	}
|	do_statement T_WHILE T_LPAREN expression T_RPAREN T_SEMI {
		stmt_do_while_expr($4);
		suppress_fallthrough = false;
	}
|	do error {
		clear_warning_flags();
		stmt_do_while_expr(NULL);
	}
|	for_exprs statement {
		clear_warning_flags();
		stmt_for_exprs_stmt();
		end_declaration_level();
		block_level--;
	}
|	for_exprs error {
		clear_warning_flags();
		stmt_for_exprs_stmt();
		end_declaration_level();
		block_level--;
	}
;

while_expr:			/* see C99 6.8.5 */
	T_WHILE T_LPAREN expression T_RPAREN {
		stmt_while_expr($3);
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
		stmt_do();
	}
;

for_start:			/* see C99 6.8.5 */
	T_FOR T_LPAREN {
		begin_declaration_level(DLK_AUTO);
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
		stmt_for_exprs(NULL, $6, $8);
		clear_warning_flags();
	}
|	for_start
	    expression_opt T_SEMI
	    expression_opt T_SEMI
	    expression_opt T_RPAREN {
		stmt_for_exprs($2, $4, $6);
		clear_warning_flags();
	}
;

jump_statement:			/* C99 6.8.6 */
	goto identifier T_SEMI {
		stmt_goto(getsym($2));
	}
|	goto error T_SEMI {
		set_sym_kind(SK_VCFT);
	}
|	T_CONTINUE T_SEMI {
		stmt_continue();
	}
|	T_BREAK T_SEMI {
		stmt_break();
	}
|	T_RETURN sys T_SEMI {
		stmt_return($2, NULL);
	}
|	T_RETURN sys expression T_SEMI {
		stmt_return($2, $3);
	}
;

goto:				/* see C99 6.8.6 */
	T_GOTO {
		set_sym_kind(SK_LABEL);
	}
;

asm_statement:			/* GCC extension */
	T_ASM T_LPAREN read_until_rparen T_SEMI {
		dcs_set_asm();
	}
|	T_ASM type_qualifier T_LPAREN read_until_rparen T_SEMI {
		dcs_set_asm();
	}
|	T_ASM error
;

read_until_rparen:		/* helper for 'asm_statement' */
	/* empty */ {
		read_until_rparen();
	}
;

translation_unit:		/* C99 6.9 */
	external_declaration
|	translation_unit external_declaration
;

external_declaration:		/* C99 6.9 */
	function_definition {
		global_clean_up_decl(false);
		clear_warning_flags();
	}
|	top_level_declaration {
		global_clean_up_decl(false);
		clear_warning_flags();
	}
|	asm_statement		/* GCC extension */
|	T_SEMI {		/* GCC extension */
		/*
		 * TODO: Only allow this in GCC mode, not in plain C99.
		 * This is one of the top 10 warnings in the NetBSD build.
		 */
		if (!allow_trad && !allow_c99)
			/* empty declaration */
			error(0);
		else if (allow_c90)
			/* empty declaration */
			warning(0);
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
		/* TODO: Make this an error in C99 mode as well. */
		if (!allow_trad && !allow_c99)
			/* old-style declaration; add 'int' */
			error(1);
		else if (allow_c90)
			/* old-style declaration; add 'int' */
			warning(1);
	}
|	declaration
|	error T_SEMI {
		global_clean_up();
	}
|	error T_RBRACE {
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
		check_extern_declaration($1);
		begin_function($1);
		block_level++;
		begin_declaration_level(DLK_OLD_STYLE_PARAMS);
		if (lwarn == LWARN_NONE)
			$1->s_used = true;
	} arg_declaration_list_opt {
		end_declaration_level();
		block_level--;
		check_func_lint_directives();
		check_func_old_style_parameters();
		begin_control_statement(CS_FUNCTION_BODY);
	} compound_statement {
		end_function();
		end_control_statement(CS_FUNCTION_BODY);
	}
;

func_declarator:
	begin_type end_type notype_declarator {
		if (!allow_trad)
			/* old-style declaration; add 'int' */
			error(1);
		$$ = $3;
	}
|	begin_type_declmods end_type notype_declarator {
		if (!allow_trad)
			/* old-style declaration; add 'int' */
			error(1);
		$$ = $3;
	}
|	begin_type_declaration_specifiers end_type type_declarator {
		$$ = $3;
	}
;

arg_declaration_list_opt:	/* C99 6.9.1p13 example 1 */
	/* empty */
|	arg_declaration_list
;

arg_declaration_list:		/* C99 6.9.1p13 example 1 */
	arg_declaration
|	arg_declaration_list arg_declaration
	/* XXX or better "arg_declaration error" ? */
|	error
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
|	begin_type_declmods end_type notype_init_declarators T_SEMI
|	begin_type_declaration_specifiers end_type T_SEMI {
		if (!dcs->d_nonempty_decl)
			/* empty declaration */
			warning(2);
		else
			/* '%s' declared in parameter declaration list */
			warning(3, type_name(dcs->d_type));
	}
|	begin_type_declaration_specifiers end_type
	    type_init_declarators T_SEMI {
		if (dcs->d_nonempty_decl)
			/* '%s' declared in parameter declaration list */
			warning(3, type_name(dcs->d_type));
	}
|	begin_type_declmods error
|	begin_type_declaration_specifiers error
;

/* https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html */
gcc_attribute_specifier_list_opt:
	/* empty */
|	gcc_attribute_specifier_list
;

gcc_attribute_specifier_list:
	gcc_attribute_specifier
|	gcc_attribute_specifier_list gcc_attribute_specifier
;

gcc_attribute_specifier:
	T_ATTRIBUTE T_LPAREN T_LPAREN {
		in_gcc_attribute = true;
	} gcc_attribute_list {
		in_gcc_attribute = false;
	} T_RPAREN T_RPAREN
;

gcc_attribute_list:
	gcc_attribute
|	gcc_attribute_list T_COMMA gcc_attribute
;

gcc_attribute:
	/* empty */
|	T_NAME {
		const char *name = $1->sb_name;
		if (is_either(name, "packed", "__packed__"))
			dcs_add_packed();
		else if (is_either(name, "used", "__used__") ||
		    is_either(name, "unused", "__unused__"))
			dcs_set_used();
		else if (is_either(name, "fallthrough", "__fallthrough__"))
			suppress_fallthrough = true;
	}
|	T_NAME T_LPAREN T_RPAREN
|	T_NAME T_LPAREN gcc_attribute_parameters T_RPAREN
|	type_qualifier {
		if (!$1.tq_const)
			yyerror("Bad attribute");
	}
;

gcc_attribute_parameters:
	constant_expression
|	gcc_attribute_parameters T_COMMA constant_expression
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

#if YYDEBUG && YYBYACC
static const char *
cgram_to_string(int token, YYSTYPE val)
{

	switch (token) {
	case T_INCDEC:
		return val.y_inc ? "++" : "--";
	case T_MULTIPLICATIVE:
	case T_ADDITIVE:
	case T_SHIFT:
	case T_RELATIONAL:
	case T_EQUALITY:
	case T_OPASSIGN:
		return op_name(val.y_op);
	case T_SCLASS:
		return scl_name(val.y_scl);
	case T_TYPE:
	case T_STRUCT_OR_UNION:
		return tspec_name(val.y_tspec);
	case T_QUAL:
		return type_qualifiers_string(val.y_type_qualifiers);
	case T_FUNCTION_SPECIFIER:
		return function_specifier_name(val.y_function_specifier);
	case T_NAME:
		return val.y_name->sb_name;
	default:
		return "<none>";
	}
}
#endif

static void
cgram_declare(sym_t *decl, bool has_initializer, sbuf_t *renaming)
{
	declare(decl, has_initializer, renaming);
	if (renaming != NULL)
		freeyyv(&renaming, T_NAME);
}

/*
 * Discard all input tokens up to and including the next unmatched right
 * parenthesis.
 */
static void
read_until_rparen(void)
{
	int level;

	if (yychar < 0)
		yychar = yylex();
	freeyyv(&yylval, yychar);

	level = 1;
	while (yychar > 0) {
		if (yychar == T_LPAREN)
			level++;
		if (yychar == T_RPAREN && --level == 0)
			break;
		freeyyv(&yylval, yychar = yylex());
	}

	yyclearin;
}

static sym_t *
symbolrename(sym_t *s, sbuf_t *sb)
{
	if (sb != NULL)
		s->s_rename = sb->sb_name;
	return s;
}
