%{
/* $NetBSD: cgram.y,v 1.54.2.2 2014/08/20 00:05:06 tls Exp $ */

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
__RCSID("$NetBSD: cgram.y,v 1.54.2.2 2014/08/20 00:05:06 tls Exp $");
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "lint1.h"

extern char *yytext;
/*
 * Contains the level of current declaration. 0 is extern.
 * Used for symbol table entries.
 */
int	blklev;

/*
 * level for memory allocation. Normaly the same as blklev.
 * An exception is the declaration of arguments in prototypes. Memory
 * for these can't be freed after the declaration, but symbols must
 * be removed from the symbol table after the declaration.
 */
int	mblklev;

/*
 * Save the no-warns state and restore it to avoid the problem where
 * if (expr) { stmt } / * NOLINT * / stmt;
 */
static int olwarn = LWARN_BAD;

static	int	toicon(tnode_t *, int);
static	void	idecl(sym_t *, int, sbuf_t *);
static	void	ignuptorp(void);

#ifdef DEBUG
static inline void CLRWFLGS(const char *file, size_t line);
static inline void CLRWFLGS(const char *file, size_t line)
{
	printf("%s, %d: clear flags %s %zu\n", curr_pos.p_file,
	    curr_pos.p_line, file, line);
	clrwflgs();
	olwarn = LWARN_BAD;
}

static inline void SAVE(const char *file, size_t line);
static inline void SAVE(const char *file, size_t line)
{
	if (olwarn != LWARN_BAD)
		abort();
	printf("%s, %d: save flags %s %zu = %d\n", curr_pos.p_file,
	    curr_pos.p_line, file, line, lwarn);
	olwarn = lwarn;
}

static inline void RESTORE(const char *file, size_t line);
static inline void RESTORE(const char *file, size_t line)
{
	if (olwarn != LWARN_BAD) {
		lwarn = olwarn;
		printf("%s, %d: restore flags %s %zu = %d\n", curr_pos.p_file,
		    curr_pos.p_line, file, line, lwarn);
		olwarn = LWARN_BAD;
	} else
		CLRWFLGS(file, line);
}
#else
#define CLRWFLGS(f, l) clrwflgs(), olwarn = LWARN_BAD
#define SAVE(f, l)	olwarn = lwarn
#define RESTORE(f, l) (void)(olwarn == LWARN_BAD ? (clrwflgs(), 0) : (lwarn = olwarn))
#endif
%}

%expect 75

%union {
	int	y_int;
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
	strg_t	*y_strg;
	pqinf_t	*y_pqinf;
};

%token			T_LBRACE T_RBRACE T_LBRACK T_RBRACK T_LPARN T_RPARN
%token	<y_op>		T_STROP
%token	<y_op>		T_UNOP
%token	<y_op>		T_INCDEC
%token			T_SIZEOF
%token			T_TYPEOF
%token			T_EXTENSION
%token			T_ALIGNOF
%token	<y_op>		T_MULT
%token	<y_op>		T_DIVOP
%token	<y_op>		T_ADDOP
%token	<y_op>		T_SHFTOP
%token	<y_op>		T_RELOP
%token	<y_op>		T_EQOP
%token	<y_op>		T_AND
%token	<y_op>		T_XOR
%token	<y_op>		T_OR
%token	<y_op>		T_LOGAND
%token	<y_op>		T_LOGOR
%token			T_QUEST
%token			T_COLON
%token	<y_op>		T_ASSIGN
%token	<y_op>		T_OPASS
%token			T_COMMA
%token			T_SEMI
%token			T_ELLIPSE
%token			T_REAL
%token			T_IMAG

/* storage classes (extern, static, auto, register and typedef) */
%token	<y_scl>		T_SCLASS

/* types (char, int, short, long, unsigned, signed, float, double, void) */
%token	<y_tspec>	T_TYPE

/* qualifiers (const, volatile) */
%token	<y_tqual>	T_QUAL

/* struct or union */
%token	<y_tspec>	T_SOU

/* enum */
%token			T_ENUM

/* remaining keywords */
%token			T_CASE
%token			T_DEFAULT
%token			T_IF
%token			T_ELSE
%token			T_SWITCH
%token			T_DO
%token			T_WHILE
%token			T_FOR
%token			T_GOTO
%token			T_CONTINUE
%token			T_BREAK
%token			T_RETURN
%token			T_ASM
%token			T_SYMBOLRENAME
%token			T_PACKED
/* Type Attributes */
%token <y_type>		T_ATTRIBUTE
%token <y_type>		T_AT_ALIGNED
%token <y_type>		T_AT_DEPRECATED
%token <y_type>		T_AT_NORETURN
%token <y_type>		T_AT_MAY_ALIAS
%token <y_type>		T_AT_PACKED
%token <y_type>		T_AT_PURE
%token <y_type>		T_AT_TUINION
%token <y_type>		T_AT_TUNION
%token <y_type>		T_AT_UNUSED
%token <y_type>		T_AT_FORMAT
%token <y_type>		T_AT_FORMAT_PRINTF
%token <y_type>		T_AT_FORMAT_SCANF
%token <y_type>		T_AT_FORMAT_STRFTIME
%token <y_type>		T_AT_FORMAT_ARG
%token <y_type>		T_AT_SENTINEL
%token <y_type>		T_AT_RETURNS_TWICE
%token <y_type>		T_AT_COLD

%left	T_COMMA
%right	T_ASSIGN T_OPASS
%right	T_QUEST T_COLON
%left	T_LOGOR
%left	T_LOGAND
%left	T_OR
%left	T_XOR
%left	T_AND
%left	T_EQOP
%left	T_RELOP
%left	T_SHFTOP
%left	T_ADDOP
%left	T_MULT T_DIVOP
%right	T_UNOP T_INCDEC T_SIZEOF T_ALIGNOF T_REAL T_IMAG
%left	T_LPARN T_LBRACK T_STROP

%token	<y_sb>		T_NAME
%token	<y_sb>		T_TYPENAME
%token	<y_val>		T_CON
%token	<y_strg>	T_STRING

%type	<y_sym>		func_decl
%type	<y_sym>		notype_decl
%type	<y_sym>		type_decl
%type	<y_type>	typespec
%type	<y_type>	clrtyp_typespec
%type	<y_type>	notype_typespec
%type	<y_type>	struct_spec
%type	<y_type>	enum_spec
%type	<y_type>	type_attribute
%type	<y_type>	type_attribute_spec
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
%type	<y_tnode>	constant
%type	<y_sym>		enum_declaration
%type	<y_sym>		enums_with_opt_comma
%type	<y_sym>		enums
%type	<y_sym>		enumerator
%type	<y_sym>		ename
%type	<y_sym>		notype_direct_decl
%type	<y_sym>		type_direct_decl
%type	<y_pqinf>	pointer
%type	<y_pqinf>	asterisk
%type	<y_sym>		param_decl
%type	<y_sym>		param_list
%type	<y_sym>		abs_decl_param_list
%type	<y_sym>		direct_param_decl
%type	<y_sym>		notype_param_decl
%type	<y_sym>		direct_notype_param_decl
%type	<y_pqinf>	type_qualifier_list
%type	<y_pqinf>	type_qualifier
%type	<y_sym>		identifier_list
%type	<y_sym>		abs_decl
%type	<y_sym>		direct_abs_decl
%type	<y_sym>		vararg_parameter_type_list
%type	<y_sym>		parameter_type_list
%type	<y_sym>		parameter_declaration
%type	<y_tnode>	expr
%type	<y_tnode>	expr_stmnt_val
%type	<y_tnode>	expr_stmnt_list
%type	<y_tnode>	term
%type	<y_tnode>	func_arg_list
%type	<y_op>		point_or_arrow
%type	<y_type>	type_name
%type	<y_sym>		abstract_declaration
%type	<y_tnode>	do_while_expr
%type	<y_tnode>	opt_expr
%type	<y_strg>	string
%type	<y_strg>	string2
%type	<y_sb>		opt_asm_or_symbolrename
%type	<y_range>	range
%type	<y_range>	lorange


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

translation_unit:
	  ext_decl
	| translation_unit ext_decl
	;

ext_decl:
	  asm_stmnt
	| func_def {
		glclup(0);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	| data_def {
		glclup(0);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

data_def:
	  T_SEMI {
		if (sflag) {
			/* syntax error: empty declaration */
			error(0);
		} else if (!tflag) {
			/* syntax error: empty declaration */
			warning(0);
		}
	  }
	| clrtyp deftyp notype_init_decls T_SEMI {
		if (sflag) {
			/* old style declaration; add "int" */
			error(1);
		} else if (!tflag) {
			/* old style declaration; add "int" */
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
	| declspecs deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else if (!dcs->d_nedecl) {
			/* empty declaration */
			warning(2);
		}
	  }
	| declspecs deftyp type_init_decls T_SEMI
	| error T_SEMI {
		globclup();
	  }
	| error T_RBRACE {
		globclup();
	  }
	;

func_def:
	  func_decl {
		if ($1->s_type->t_tspec != FUNC) {
			/* syntax error */
			error(249, yytext);
			YYERROR;
		}
		if ($1->s_type->t_typedef) {
			/* ()-less function definition */
			error(64);
			YYERROR;
		}
		funcdef($1);
		blklev++;
		pushdecl(ARG);
		if (lwarn == LWARN_NONE)
			$1->s_used = 1;
	  } opt_arg_declaration_list {
		popdecl();
		blklev--;
		cluparg();
		pushctrl(0);
	  } comp_stmnt {
		funcend();
		popctrl(0);
	  }
	;

func_decl:
	  clrtyp deftyp notype_decl {
		$$ = $3;
	  }
	| declmods deftyp notype_decl {
		$$ = $3;
	  }
	| declspecs deftyp type_decl {
		$$ = $3;
	  }
	;

opt_arg_declaration_list:
	  /* empty */
	| arg_declaration_list
	;

arg_declaration_list:
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
	| declspecs deftyp T_SEMI {
		if (!dcs->d_nedecl) {
			/* empty declaration */
			warning(2);
		} else {
			tspec_t	ts = dcs->d_type->t_tspec;
			/* %s declared in argument declaration list */
			warning(3, ts == STRUCT ? "struct" :
				(ts == UNION ? "union" : "enum"));
		}
	  }
	| declspecs deftyp type_init_decls T_SEMI {
		if (dcs->d_nedecl) {
			tspec_t	ts = dcs->d_type->t_tspec;
			/* %s declared in argument declaration list */
			warning(3, ts == STRUCT ? "struct" :
				(ts == UNION ? "union" : "enum"));
		}
	  }
	| declmods error
	| declspecs error
	;

declaration:
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
	| declspecs deftyp T_SEMI {
		if (dcs->d_scl == TYPEDEF) {
			/* typedef declares no type name */
			warning(72);
		} else if (!dcs->d_nedecl) {
			/* empty declaration */
			warning(2);
		}
	  }
	| declspecs deftyp type_init_decls T_SEMI
	| error T_SEMI
	;

type_attribute_format_type:
	  T_AT_FORMAT_PRINTF
	| T_AT_FORMAT_SCANF
	| T_AT_FORMAT_STRFTIME
	;

type_attribute_spec:
	  T_AT_DEPRECATED
	| T_AT_ALIGNED T_LPARN constant T_RPARN
	| T_AT_SENTINEL T_LPARN constant T_RPARN
	| T_AT_FORMAT_ARG T_LPARN constant T_RPARN
	| T_AT_MAY_ALIAS
	| T_AT_NORETURN
	| T_AT_COLD
	| T_AT_RETURNS_TWICE
	| T_AT_PACKED {
		addpacked();
	}
	| T_AT_PURE
	| T_AT_TUNION
	| T_AT_FORMAT T_LPARN type_attribute_format_type T_COMMA
	    constant T_COMMA constant T_RPARN
	| T_AT_UNUSED
	| T_QUAL {
		if ($1 != CONST)	
			yyerror("Bad attribute");
	}
	;

type_attribute:
	  T_ATTRIBUTE T_LPARN T_LPARN {
	    attron = 1;
	} type_attribute_spec {
	    attron = 0;
	} T_RPARN T_RPARN
	| T_PACKED {
		addpacked();
	}
	;

clrtyp:
	  {
		clrtyp();
	  }
	;

deftyp:
	  /* empty */ {
		deftyp();
	  }
	;

declspecs:
	  clrtyp_typespec {
		addtype($1);
	  }
	| declmods typespec {
		addtype($2);
	  }
	| type_attribute declspecs
	| declspecs type_attribute
	| declspecs declmod
	| declspecs notype_typespec {
		addtype($2);
	  }
	;

declmods:
	  clrtyp T_QUAL {
		addqual($2);
	  }
	| clrtyp T_SCLASS {
		addscl($2);
	  }
	| declmods declmod
	;

declmod:
	  T_QUAL {
		addqual($1);
	  }
	| T_SCLASS {
		addscl($1);
	  }
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
	  notype_typespec {
		$$ = $1;
	  }
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
		popdecl();
		$$ = $1;
	  }
	| enum_spec {
		popdecl();
		$$ = $1;
	  }
	;

struct_spec:
	  struct struct_tag {
		/*
		 * STDC requires that "struct a;" always introduces
		 * a new tag if "a" is not declared at current level
		 *
		 * yychar is valid because otherwise the parser would
		 * not been able to decide if he must shift or reduce
		 */
		$$ = mktag($2, $1, 0, yychar == T_SEMI);
	  }
	| struct struct_tag {
		dcs->d_tagtyp = mktag($2, $1, 1, 0);
	  } struct_declaration {
		$$ = compltag(dcs->d_tagtyp, $4);
	  }
	| struct {
		dcs->d_tagtyp = mktag(NULL, $1, 1, 0);
	  } struct_declaration {
		$$ = compltag(dcs->d_tagtyp, $3);
	  }
	| struct error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

struct:
	  struct type_attribute
	| T_SOU {
		symtyp = FTAG;
		pushdecl($1 == STRUCT ? MOS : MOU);
		dcs->d_offset = 0;
		dcs->d_stralign = CHAR_BIT;
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
	  member_declaration_list T_SEMI T_RBRACE {
		$$ = $1;
	  }
	| member_declaration_list T_RBRACE {
		if (sflag) {
			/* syntax req. ";" after last struct/union member */
			error(66);
		} else {
			/* syntax req. ";" after last struct/union member */
			warning(66);
		}
		$$ = $1;
	  }
	| T_RBRACE {
		$$ = NULL;
	  }
	;

member_declaration_list:
	  member_declaration {
		$$ = $1;
	  }
	| member_declaration_list T_SEMI member_declaration {
		$$ = lnklst($1, $3);
	  }
	;

member_declaration:
	  noclass_declmods deftyp {
		/* too late, i know, but getsym() compensates it */
		symtyp = FMOS;
	  } notype_member_decls {
		symtyp = FVFT;
		$$ = $4;
	  }
	| noclass_declspecs deftyp {
		symtyp = FMOS;
	  } type_member_decls {
		symtyp = FVFT;
		$$ = $4;
	  }
	| noclass_declmods deftyp {
		/* struct or union member must be named */
		warning(49);
		$$ = NULL;
	  }
	| noclass_declspecs deftyp {
		/* struct or union member must be named */
		warning(49);
		$$ = NULL;
	  }
	| error {
		symtyp = FVFT;
		$$ = NULL;
	  }
	;

noclass_declspecs:
	  clrtyp_typespec {
		addtype($1);
	  }
	| type_attribute noclass_declspecs
	| noclass_declmods typespec {
		addtype($2);
	  }
	| noclass_declspecs T_QUAL {
		addqual($2);
	  }
	| noclass_declspecs notype_typespec {
		addtype($2);
	  }
	| noclass_declspecs type_attribute
	;

noclass_declmods:
	  clrtyp T_QUAL {
		addqual($2);
	  }
	| noclass_declmods T_QUAL {
		addqual($2);
	  }
	;

notype_member_decls:
	  notype_member_decl {
		$$ = decl1str($1);
	  }
	| notype_member_decls {
		symtyp = FMOS;
	  } T_COMMA type_member_decl {
		$$ = lnklst($1, decl1str($4));
	  }
	;

type_member_decls:
	  type_member_decl {
		$$ = decl1str($1);
	  }
	| type_member_decls {
		symtyp = FMOS;
	  } T_COMMA type_member_decl {
		$$ = lnklst($1, decl1str($4));
	  }
	;

notype_member_decl:
	  notype_decl {
		$$ = $1;
	  }
	| notype_decl T_COLON constant {
		$$ = bitfield($1, toicon($3, 1));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant {
		$$ = bitfield(NULL, toicon($3, 1));
	  }
	;

type_member_decl:
	  type_decl {
		$$ = $1;
	  }
	| type_decl T_COLON constant {
		$$ = bitfield($1, toicon($3, 1));
	  }
	| {
		symtyp = FVFT;
	  } T_COLON constant {
		$$ = bitfield(NULL, toicon($3, 1));
	  }
	;

enum_spec:
	  enum enum_tag {
		$$ = mktag($2, ENUM, 0, 0);
	  }
	| enum enum_tag {
		dcs->d_tagtyp = mktag($2, ENUM, 1, 0);
	  } enum_declaration {
		$$ = compltag(dcs->d_tagtyp, $4);
	  }
	| enum {
		dcs->d_tagtyp = mktag(NULL, ENUM, 1, 0);
	  } enum_declaration {
		$$ = compltag(dcs->d_tagtyp, $3);
	  }
	| enum error {
		symtyp = FVFT;
		$$ = gettyp(INT);
	  }
	;

enum:
	  T_ENUM {
		symtyp = FTAG;
		pushdecl(ENUMCON);
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
	  enums {
		$$ = $1;
	  }
	| enums T_COMMA {
		if (sflag) {
			/* trailing "," prohibited in enum declaration */
			error(54);
		} else {
			/* trailing "," prohibited in enum declaration */
			c99ism(54);
		}
		$$ = $1;
	  }
	;

enums:
	  enumerator {
		$$ = $1;
	  }
	| enums T_COMMA enumerator {
		$$ = lnklst($1, $3);
	  }
	| error {
		$$ = NULL;
	  }
	;

enumerator:
	  ename {
		$$ = ename($1, enumval, 1);
	  }
	| ename T_ASSIGN constant {
		$$ = ename($1, toicon($3, 1), 0);
	  }
	;

ename:
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

notype_init_decl:
	  notype_decl opt_asm_or_symbolrename {
		idecl($1, 0, $2);
		chksz($1);
	  }
	| notype_decl opt_asm_or_symbolrename {
		idecl($1, 1, $2);
	  } T_ASSIGN initializer {
		chksz($1);
	  }
	;

type_init_decl:
	  type_decl opt_asm_or_symbolrename {
		idecl($1, 0, $2);
		chksz($1);
	  }
	| type_decl opt_asm_or_symbolrename {
		idecl($1, 1, $2);
	  } T_ASSIGN initializer {
		chksz($1);
	  }
	;

notype_decl:
	  notype_direct_decl {
		$$ = $1;
	  }
	| pointer notype_direct_decl {
		$$ = addptr($2, $1);
	  }
	;

notype_direct_decl:
	  T_NAME {
		$$ = dname(getsym($1));
	  }
	| T_LPARN type_decl T_RPARN {
		$$ = $2;
	  }
	| type_attribute notype_direct_decl {
		$$ = $2;
	}
	| notype_direct_decl T_LBRACK T_RBRACK {
		$$ = addarray($1, 0, 0);
	  }
	| notype_direct_decl T_LBRACK constant T_RBRACK {
		$$ = addarray($1, 1, toicon($3, 0));
	  }
	| notype_direct_decl param_list opt_asm_or_symbolrename {
		$$ = addfunc($1, $2);
		popdecl();
		blklev--;
	  }
	| notype_direct_decl type_attribute
	;

type_decl:
	  type_direct_decl {
		$$ = $1;
	  }
	| pointer type_direct_decl {
		$$ = addptr($2, $1);
	  }
	;

type_direct_decl:
	  identifier {
		$$ = dname(getsym($1));
	  }
	| T_LPARN type_decl T_RPARN {
		$$ = $2;
	  }
	| type_attribute type_direct_decl {
		$$ = $2;
	}
	| type_direct_decl T_LBRACK T_RBRACK {
		$$ = addarray($1, 0, 0);
	  }
	| type_direct_decl T_LBRACK constant T_RBRACK {
		$$ = addarray($1, 1, toicon($3, 0));
	  }
	| type_direct_decl param_list opt_asm_or_symbolrename {
		$$ = addfunc($1, $2);
		popdecl();
		blklev--;
	  }
	| type_direct_decl type_attribute
	;

/*
 * param_decl and notype_param_decl exist to avoid a conflict in
 * argument lists. A typename enclosed in parens should always be
 * treated as a typename, not an argument.
 * "typedef int a; f(int (a));" is  "typedef int a; f(int foo(a));"
 *				not "typedef int a; f(int a);"
 */
param_decl:
	  direct_param_decl {
		$$ = $1;
	  }
	| pointer direct_param_decl {
		$$ = addptr($2, $1);
	  }
	;

direct_param_decl:
	  identifier {
		$$ = dname(getsym($1));
	  }
	| T_LPARN notype_param_decl T_RPARN {
		$$ = $2;
	  }
	| direct_param_decl T_LBRACK T_RBRACK {
		$$ = addarray($1, 0, 0);
	  }
	| direct_param_decl T_LBRACK constant T_RBRACK {
		$$ = addarray($1, 1, toicon($3, 0));
	  }
	| direct_param_decl param_list opt_asm_or_symbolrename {
		$$ = addfunc($1, $2);
		popdecl();
		blklev--;
	  }
	;

notype_param_decl:
	  direct_notype_param_decl {
		$$ = $1;
	  }
	| pointer direct_notype_param_decl {
		$$ = addptr($2, $1);
	  }
	;

direct_notype_param_decl:
	  T_NAME {
		$$ = dname(getsym($1));
	  }
	| T_LPARN notype_param_decl T_RPARN {
		$$ = $2;
	  }
	| direct_notype_param_decl T_LBRACK T_RBRACK {
		$$ = addarray($1, 0, 0);
	  }
	| direct_notype_param_decl T_LBRACK constant T_RBRACK {
		$$ = addarray($1, 1, toicon($3, 0));
	  }
	| direct_notype_param_decl param_list opt_asm_or_symbolrename {
		$$ = addfunc($1, $2);
		popdecl();
		blklev--;
	  }
	;

pointer:
	  asterisk {
		$$ = $1;
	  }
	| asterisk type_qualifier_list {
		$$ = mergepq($1, $2);
	  }
	| asterisk pointer {
		$$ = mergepq($1, $2);
	  }
	| asterisk type_qualifier_list pointer {
		$$ = mergepq(mergepq($1, $2), $3);
	  }
	;

asterisk:
	  T_MULT {
		$$ = xcalloc(1, sizeof (pqinf_t));
		$$->p_pcnt = 1;
	  }
	;

type_qualifier_list:
	  type_qualifier {
		$$ = $1;
	  }
	| type_qualifier_list type_qualifier {
		$$ = mergepq($1, $2);
	  }
	;

type_qualifier:
	  T_QUAL {
		$$ = xcalloc(1, sizeof (pqinf_t));
		if ($1 == CONST) {
			$$->p_const = 1;
		} else {
			$$->p_volatile = 1;
		}
	  }
	;

param_list:
	  id_list_lparn identifier_list T_RPARN {
		$$ = $2;
	  }
	| abs_decl_param_list {
		$$ = $1;
	  }
	;

id_list_lparn:
	  T_LPARN {
		blklev++;
		pushdecl(PARG);
	  }
	;

identifier_list:
	  T_NAME {
		$$ = iname(getsym($1));
	  }
	| identifier_list T_COMMA T_NAME {
		$$ = lnklst($1, iname(getsym($3)));
	  }
	| identifier_list error {
		$$ = $1;
	  }
	;

abs_decl_param_list:
	  abs_decl_lparn T_RPARN {
		$$ = NULL;
	  }
	| abs_decl_lparn vararg_parameter_type_list T_RPARN {
		dcs->d_proto = 1;
		$$ = $2;
	  }
	| abs_decl_lparn error T_RPARN {
		$$ = NULL;
	  }
	;

abs_decl_lparn:
	  T_LPARN {
		blklev++;
		pushdecl(PARG);
	  }
	;

vararg_parameter_type_list:
	  parameter_type_list {
		$$ = $1;
	  }
	| parameter_type_list T_COMMA T_ELLIPSE {
		dcs->d_vararg = 1;
		$$ = $1;
	  }
	| T_ELLIPSE {
		if (sflag) {
			/* ANSI C requires formal parameter before "..." */
			error(84);
		} else if (!tflag) {
			/* ANSI C requires formal parameter before "..." */
			warning(84);
		}
		dcs->d_vararg = 1;
		$$ = NULL;
	  }
	;

parameter_type_list:
	  parameter_declaration {
		$$ = $1;
	  }
	| parameter_type_list T_COMMA parameter_declaration {
		$$ = lnklst($1, $3);
	  }
	;

parameter_declaration:
	  declmods deftyp {
		$$ = decl1arg(aname(), 0);
	  }
	| declspecs deftyp {
		$$ = decl1arg(aname(), 0);
	  }
	| declmods deftyp notype_param_decl {
		$$ = decl1arg($3, 0);
	  }
	/*
	 * param_decl is needed because of following conflict:
	 * "typedef int a; f(int (a));" could be parsed as
	 * "function with argument a of type int", or
	 * "function with an abstract argument of type function".
	 * This grammar realizes the second case.
	 */
	| declspecs deftyp param_decl {
		$$ = decl1arg($3, 0);
	  }
	| declmods deftyp abs_decl {
		$$ = decl1arg($3, 0);
	  }
	| declspecs deftyp abs_decl {
		$$ = decl1arg($3, 0);
	  }
	;

opt_asm_or_symbolrename:		/* expect only one */
	  /* empty */ {
		$$ = NULL;
	  }
	| T_ASM T_LPARN T_STRING T_RPARN {
		freeyyv(&$3, T_STRING);
		$$ = NULL;
	  }
	| T_SYMBOLRENAME T_LPARN T_NAME T_RPARN {
		$$ = $3;
	  }
	;

initializer:
	  init_expr
	;

init_expr:
	  expr				%prec T_COMMA {
		mkinit($1);
	  }
	| init_by_name init_expr	%prec T_COMMA
	| init_lbrace init_rbrace
	| init_lbrace init_expr_list init_rbrace
	| init_lbrace init_expr_list T_COMMA init_rbrace
	| error
	;

init_expr_list:
	  init_expr			%prec T_COMMA
	| init_expr_list T_COMMA init_expr
	;

lorange: 
	  constant T_ELLIPSE {
		$$.lo = toicon($1, 1);
	  }
	;
range:
	constant {
		$$.lo = toicon($1, 1);
		$$.hi = $$.lo + 1;
	  }
	| lorange constant {
		$$.lo = $1.lo;
		$$.hi = toicon($2, 1);
	  }
	;

init_by_name:
	  T_LBRACK range T_RBRACK T_ASSIGN {
		if (!Sflag)
			warning(321);
	  }
	| point identifier T_ASSIGN {
		if (!Sflag)
			warning(313);
		memberpush($2);
	  }
	| identifier T_COLON {
		gnuism(315);
		memberpush($1);
	  }
	;

init_lbrace:
	  T_LBRACE {
		initlbr();
	  }
	;

init_rbrace:
	  T_RBRACE {
		initrbr();
	  }
	;

type_name:
	  {
		pushdecl(ABSTRACT);
	  } abstract_declaration {
		popdecl();
		$$ = $2->s_type;
	  }
	;

abstract_declaration:
	  noclass_declmods deftyp {
		$$ = decl1abs(aname());
	  }
	| noclass_declspecs deftyp {
		$$ = decl1abs(aname());
	  }
	| noclass_declmods deftyp abs_decl {
		$$ = decl1abs($3);
	  }
	| noclass_declspecs deftyp abs_decl {
		$$ = decl1abs($3);
	  }
	;

abs_decl:
	  pointer {
		$$ = addptr(aname(), $1);
	  }
	| direct_abs_decl {
		$$ = $1;
	  }
	| pointer direct_abs_decl {
		$$ = addptr($2, $1);
	  }
	;

direct_abs_decl:
	  T_LPARN abs_decl T_RPARN {
		$$ = $2;
	  }
	| T_LBRACK T_RBRACK {
		$$ = addarray(aname(), 0, 0);
	  }
	| T_LBRACK constant T_RBRACK {
		$$ = addarray(aname(), 1, toicon($2, 0));
	  }
	| type_attribute direct_abs_decl {
		$$ = $2;
	}
	| direct_abs_decl T_LBRACK T_RBRACK {
		$$ = addarray($1, 0, 0);
	  }
	| direct_abs_decl T_LBRACK constant T_RBRACK {
		$$ = addarray($1, 1, toicon($3, 0));
	  }
	| abs_decl_param_list opt_asm_or_symbolrename {
		$$ = addfunc(aname(), $1);
		popdecl();
		blklev--;
	  }
	| direct_abs_decl abs_decl_param_list opt_asm_or_symbolrename {
		$$ = addfunc($1, $2);
		popdecl();
		blklev--;
	  }
	| direct_abs_decl type_attribute
	;

non_expr_stmnt:
	  labeled_stmnt
	| comp_stmnt
	| selection_stmnt
	| iteration_stmnt
	| jump_stmnt {
		ftflg = 0;
	  }
	| asm_stmnt

stmnt:
	  expr_stmnt
	| non_expr_stmnt
	;

labeled_stmnt:
	  label stmnt
	;

label:
	  T_NAME T_COLON {
		symtyp = FLAB;
		label(T_NAME, getsym($1), NULL);
	  }
	| T_CASE constant T_COLON {
		label(T_CASE, NULL, $2);
		ftflg = 1;
	}
	| T_CASE constant T_ELLIPSE constant T_COLON {
		/* XXX: We don't fill all cases */
		label(T_CASE, NULL, $2);
		ftflg = 1;
	}
	| T_DEFAULT T_COLON {
		label(T_DEFAULT, NULL, NULL);
		ftflg = 1;
	  }
	;

stmnt_d_list:
	  stmnt_list
	| stmnt_d_list declaration_list stmnt_list {
		if (!Sflag)
			c99ism(327);
	}
	;

comp_stmnt:
	  comp_stmnt_lbrace comp_stmnt_rbrace
	| comp_stmnt_lbrace stmnt_d_list comp_stmnt_rbrace
	| comp_stmnt_lbrace declaration_list comp_stmnt_rbrace
	| comp_stmnt_lbrace declaration_list stmnt_d_list comp_stmnt_rbrace
	;

comp_stmnt_lbrace:
	  T_LBRACE {
		blklev++;
		mblklev++;
		pushdecl(AUTO);
	  }
	;

comp_stmnt_rbrace:
	  T_RBRACE {
		popdecl();
		freeblk();
		mblklev--;
		blklev--;
		ftflg = 0;
	  }
	;

stmnt_list:
	  stmnt
	| stmnt_list stmnt {
		RESTORE(__FILE__, __LINE__);
	  }
	| stmnt_list error T_SEMI
	;

expr_stmnt:
	  expr T_SEMI {
		expr($1, 0, 0, 1);
		ftflg = 0;
	  }
	| T_SEMI {
		ftflg = 0;
	  }
	;

/*
 * The following two productions are used to implement 
 * ({ [[decl-list] stmt-list] }).
 * XXX: This is not well tested.
 */
expr_stmnt_val:
	  expr T_SEMI {
		/* XXX: We should really do that only on the last name */
		if ($1->tn_op == NAME)
			$1->tn_sym->s_used = 1;
		$$ = $1;
		expr($1, 0, 0, 0);
		ftflg = 0;
	  }
	| non_expr_stmnt {
		$$ = getnode();
		$$->tn_type = gettyp(VOID);
	}
	;

expr_stmnt_list:
	  expr_stmnt_val
	| expr_stmnt_list expr_stmnt_val {
		$$ = $2;
	}
	;

selection_stmnt:
	  if_without_else {
		SAVE(__FILE__, __LINE__);
		if2();
		if3(0);
	  }
	| if_without_else T_ELSE {
		SAVE(__FILE__, __LINE__);
		if2();
	  } stmnt {
		CLRWFLGS(__FILE__, __LINE__);
		if3(1);
	  }
	| if_without_else T_ELSE error {
		CLRWFLGS(__FILE__, __LINE__);
		if3(0);
	  }
	| switch_expr stmnt {
		CLRWFLGS(__FILE__, __LINE__);
		switch2();
	  }
	| switch_expr error {
		CLRWFLGS(__FILE__, __LINE__);
		switch2();
	  }
	;

if_without_else:
	  if_expr stmnt
	| if_expr error
	;

if_expr:
	  T_IF T_LPARN expr T_RPARN {
		if1($3);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

switch_expr:
	  T_SWITCH T_LPARN expr T_RPARN {
		switch1($3);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

do_stmnt:
	  do stmnt {
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

iteration_stmnt:
	  while_expr stmnt {
		CLRWFLGS(__FILE__, __LINE__);
		while2();
	  }
	| while_expr error {
		CLRWFLGS(__FILE__, __LINE__);
		while2();
	  }
	| do_stmnt do_while_expr {
		do2($2);
		ftflg = 0;
	  }
	| do error {
		CLRWFLGS(__FILE__, __LINE__);
		do2(NULL);
	  }
	| for_exprs stmnt {
		CLRWFLGS(__FILE__, __LINE__);
		for2();
	  }
	| for_exprs error {
		CLRWFLGS(__FILE__, __LINE__);
		for2();
	  }
	;

while_expr:
	  T_WHILE T_LPARN expr T_RPARN {
		while1($3);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

do:
	  T_DO {
		do1();
	  }
	;

do_while_expr:
	  T_WHILE T_LPARN expr T_RPARN T_SEMI {
		$$ = $3;
	  }
	;

for_exprs:
	    T_FOR T_LPARN declspecs deftyp notype_init_decls T_SEMI opt_expr
	    T_SEMI opt_expr T_RPARN {
		c99ism(325);
		for1(NULL, $7, $9);
		CLRWFLGS(__FILE__, __LINE__);
	    }
	  | T_FOR T_LPARN opt_expr T_SEMI opt_expr T_SEMI opt_expr T_RPARN {
		for1($3, $5, $7);
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

opt_expr:
	  /* empty */ {
		$$ = NULL;
	  }
	| expr {
		$$ = $1;
	  }
	;

jump_stmnt:
	  goto identifier T_SEMI {
		dogoto(getsym($2));
	  }
	| goto error T_SEMI {
		symtyp = FVFT;
	  }
	| T_CONTINUE T_SEMI {
		docont();
	  }
	| T_BREAK T_SEMI {
		dobreak();
	  }
	| T_RETURN T_SEMI {
		doreturn(NULL);
	  }
	| T_RETURN expr T_SEMI {
		doreturn($2);
	  }
	;

goto:
	  T_GOTO {
		symtyp = FLAB;
	  }
	;

asm_stmnt:
	  T_ASM T_LPARN read_until_rparn T_SEMI {
		setasm();
	  }
	| T_ASM T_QUAL T_LPARN read_until_rparn T_SEMI {
		setasm();
	  }
	| T_ASM error
	;

read_until_rparn:
	  /* empty */ {
		ignuptorp();
	  }
	;

declaration_list:
	  declaration {
		CLRWFLGS(__FILE__, __LINE__);
	  }
	| declaration_list declaration {
		CLRWFLGS(__FILE__, __LINE__);
	  }
	;

constant:
	  expr				%prec T_COMMA {
		  $$ = $1;
	  }
	;

expr:
	  expr T_MULT expr {
		$$ = build(MULT, $1, $3);
	  }
	| expr T_DIVOP expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_ADDOP expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_SHFTOP expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_RELOP expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_EQOP expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_AND expr {
		$$ = build(AND, $1, $3);
	  }
	| expr T_XOR expr {
		$$ = build(XOR, $1, $3);
	  }
	| expr T_OR expr {
		$$ = build(OR, $1, $3);
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
	| expr T_OPASS expr {
		$$ = build($2, $1, $3);
	  }
	| expr T_COMMA expr {
		$$ = build(COMMA, $1, $3);
	  }
	| term {
		$$ = $1;
	  }
	;

term:
	  T_NAME {
		/* XXX really necessary? */
		if (yychar < 0)
			yychar = yylex();
		$$ = getnnode(getsym($1), yychar);
	  }
	| string {
		$$ = getsnode($1);
	  }
	| T_CON {
		$$ = getcnode(gettyp($1->v_tspec), $1);
	  }
	| T_LPARN expr T_RPARN {
		if ($2 != NULL)
			$2->tn_parn = 1;
		$$ = $2;
	  }
	| T_LPARN comp_stmnt_lbrace declaration_list expr_stmnt_list {
		blklev--;
		mblklev--;
		initsym = mktempsym(duptyp($4->tn_type));
		mblklev++;
		blklev++;
		gnuism(320);
	} comp_stmnt_rbrace T_RPARN {
		$$ = getnnode(initsym, 0);
	}
	| T_LPARN comp_stmnt_lbrace expr_stmnt_list {
		blklev--;
		mblklev--;
		initsym = mktempsym($3->tn_type);
		mblklev++;
		blklev++;
		gnuism(320);
	} comp_stmnt_rbrace T_RPARN {
		$$ = getnnode(initsym, 0);
	}
	| term T_INCDEC {
		$$ = build($2 == INC ? INCAFT : DECAFT, $1, NULL);
	  }
	| T_INCDEC term {
		$$ = build($1 == INC ? INCBEF : DECBEF, $2, NULL);
	  }
	| T_MULT term {
		$$ = build(STAR, $2, NULL);
	  }
	| T_AND term {
		$$ = build(AMPER, $2, NULL);
	  }
	| T_UNOP term {
		$$ = build($1, $2, NULL);
	  }
	| T_ADDOP term {
		if (tflag && $1 == PLUS) {
			/* unary + is illegal in traditional C */
			warning(100);
		}
		$$ = build($1 == PLUS ? UPLUS : UMINUS, $2, NULL);
	  }
	| term T_LBRACK expr T_RBRACK {
		$$ = build(STAR, build(PLUS, $1, $3), NULL);
	  }
	| term T_LPARN T_RPARN {
		$$ = funccall($1, NULL);
	  }
	| term T_LPARN func_arg_list T_RPARN {
		$$ = funccall($1, $3);
	  }
	| term point_or_arrow T_NAME {
		if ($1 != NULL) {
			sym_t	*msym;
			/* XXX strmemb should be integrated in build() */
			if ($2 == ARROW) {
				/* must to this before strmemb is called */
				$1 = cconv($1);
			}
			msym = strmemb($1, $2, getsym($3));
			$$ = build($2, $1, getnnode(msym, 0));
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
	| T_REAL T_LPARN term T_RPARN {
		$$ = build(REAL, $3, NULL);
	  }
	| T_IMAG T_LPARN term T_RPARN {
		$$ = build(IMAG, $3, NULL);
	  }
	| T_SIZEOF term					%prec T_SIZEOF {
		if (($$ = $2 == NULL ? NULL : bldszof($2->tn_type)) != NULL)
			chkmisc($2, 0, 0, 0, 0, 0, 1);
	  }
	| T_SIZEOF T_LPARN type_name T_RPARN		%prec T_SIZEOF {
		$$ = bldszof($3);
	  }
	| T_ALIGNOF T_LPARN type_name T_RPARN		%prec T_ALIGNOF {
		$$ = bldalof($3);
	  }
	| T_LPARN type_name T_RPARN term		%prec T_UNOP {
		$$ = cast($4, $2);
	  }
	| T_LPARN type_name T_RPARN 			%prec T_UNOP {
		sym_t *tmp = mktempsym($2);
		idecl(tmp, 1, NULL);
	  } init_lbrace init_expr_list init_rbrace {
		if (!Sflag)
			gnuism(319);
		$$ = getnnode(initsym, 0);
	  }
	;

string:
	  T_STRING {
		$$ = $1;
	  }
	| T_STRING string2 {
		$$ = catstrg($1, $2);
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
		$$ = catstrg($1, $2);
	  }
	;

func_arg_list:
	  expr						%prec T_COMMA {
		$$ = funcarg(NULL, $1);
	  }
	| func_arg_list T_COMMA expr {
		$$ = funcarg($1, $3);
	  }
	;

point_or_arrow:
	  T_STROP {
		symtyp = FMOS;
		$$ = $1;
	  }
	;

point:
	  T_STROP {
		if ($1 != POINT) {
			error(249, yytext);
		}
	  }
	;

identifier:
	  T_NAME {
		$$ = $1;
	  }
	| T_TYPENAME {
		$$ = $1;
	  }
	;

%%

/* ARGSUSED */
int
yyerror(const char *msg)
{
	error(249, yytext);
	if (++sytxerr >= 5)
		norecover();
	return (0);
}

static __inline int uq_gt(uint64_t, uint64_t);
static __inline int q_gt(int64_t, int64_t);

static __inline int
uq_gt(uint64_t a, uint64_t b)
{

	return (a > b);
}

static __inline int
q_gt(int64_t a, int64_t b)
{

	return (a > b);
}

#define	q_lt(a, b)	q_gt(b, a)

/*
 * Gets a node for a constant and returns the value of this constant
 * as integer.
 * Is the node not constant or too large for int or of type float,
 * a warning will be printed.
 *
 * toicon() should be used only inside declarations. If it is used in
 * expressions, it frees the memory used for the expression.
 */
static int
toicon(tnode_t *tn, int required)
{
	int	i;
	tspec_t	t;
	val_t	*v;

	v = constant(tn, required);

	/*
	 * Abstract declarations are used inside expression. To free
	 * the memory would be a fatal error.
	 */
	if (dcs->d_ctx != ABSTRACT)
		tfreeblk();

	if ((t = v->v_tspec) == FLOAT || t == DOUBLE || t == LDOUBLE) {
		i = (int)v->v_ldbl;
		/* integral constant expression expected */
		error(55);
	} else {
		i = (int)v->v_quad;
		if (isutyp(t)) {
			if (uq_gt((uint64_t)v->v_quad,
				  (uint64_t)TARG_INT_MAX)) {
				/* integral constant too large */
				warning(56);
			}
		} else {
			if (q_gt(v->v_quad, (int64_t)TARG_INT_MAX) ||
			    q_lt(v->v_quad, (int64_t)TARG_INT_MIN)) {
				/* integral constant too large */
				warning(56);
			}
		}
	}
	free(v);
	return (i);
}

static void
idecl(sym_t *decl, int initflg, sbuf_t *renaming)
{
	char *s;

	initerr = 0;
	initsym = decl;

	switch (dcs->d_ctx) {
	case EXTERN:
		if (renaming != NULL) {
			if (decl->s_rename != NULL)
				LERROR("idecl()");

			s = getlblk(1, renaming->sb_len + 1);
	                (void)memcpy(s, renaming->sb_name, renaming->sb_len + 1);
			decl->s_rename = s;
			freeyyv(&renaming, T_NAME);
		}
		decl1ext(decl, initflg);
		break;
	case ARG:
		if (renaming != NULL) {
			/* symbol renaming can't be used on function arguments */
			error(310);
			freeyyv(&renaming, T_NAME);
			break;
		}
		(void)decl1arg(decl, initflg);
		break;
	case AUTO:
		if (renaming != NULL) {
			/* symbol renaming can't be used on automatic variables */
			error(311);
			freeyyv(&renaming, T_NAME);
			break;
		}
		decl1loc(decl, initflg);
		break;
	default:
		LERROR("idecl()");
	}

	if (initflg && !initerr)
		prepinit();
}

/*
 * Discard all input tokens up to and including the next
 * unmatched right paren
 */
static void
ignuptorp(void)
{
	int	level;

	if (yychar < 0)
		yychar = yylex();
	freeyyv(&yylval, yychar);

	level = 1;
	while (yychar != T_RPARN || --level > 0) {
		if (yychar == T_LPARN) {
			level++;
		} else if (yychar <= 0) {
			break;
		}
		freeyyv(&yylval, yychar = yylex());
	}

	yyclearin;
}
