/*	$Id: cgram.y,v 1.1.1.2 2009/09/04 00:27:33 gmcgarry Exp $	*/

/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Comments for this grammar file. Ragge 021123
 *
 * ANSI support required rewrite of the function header and declaration
 * rules almost totally.
 *
 * The lex/yacc shared keywords are now split from the keywords used
 * in the rest of the compiler, to simplify use of other frontends.
 */

/*
 * At last count, there were 3 shift/reduce and no reduce/reduce conflicts
 * Two was funct_idn and the third was "dangling else".
 */

/*
 * Token used in C lex/yacc communications.
 */
%token	C_STRING	/* a string constant */
%token	C_ICON		/* an integer constant */
%token	C_FCON		/* a floating point constant */
%token	C_NAME		/* an identifier */
%token	C_TYPENAME	/* a typedef'd name */
%token	C_ANDAND	/* && */
%token	C_OROR		/* || */
%token	C_GOTO		/* unconditional goto */
%token	C_RETURN	/* return from function */
%token	C_TYPE		/* a type */
%token	C_CLASS		/* a storage class */
%token	C_ASOP		/* assignment ops */
%token	C_RELOP		/* <=, <, >=, > */
%token	C_EQUOP		/* ==, != */
%token	C_DIVOP		/* /, % */
%token	C_SHIFTOP	/* <<, >> */
%token	C_INCOP		/* ++, -- */
%token	C_UNOP		/* !, ~ */
%token	C_STROP		/* ., -> */
%token	C_STRUCT
%token	C_IF
%token	C_ELSE
%token	C_SWITCH
%token	C_BREAK
%token	C_CONTINUE
%token	C_WHILE	
%token	C_DO
%token	C_FOR
%token	C_DEFAULT
%token	C_CASE
%token	C_SIZEOF
%token	C_ENUM
%token	C_ELLIPSIS
%token	C_QUALIFIER
%token	C_FUNSPEC
%token	C_ASM
%token	NOMATCH
%token	C_TYPEOF	/* COMPAT_GCC */
%token	C_ATTRIBUTE	/* COMPAT_GCC */

/*
 * Precedence
 */
%left ','
%right '=' C_ASOP
%right '?' ':'
%left C_OROR
%left C_ANDAND
%left '|'
%left '^'
%left '&'
%left C_EQUOP
%left C_RELOP
%left C_SHIFTOP
%left '+' '-'
%left '*' C_DIVOP
%right C_UNOP
%right C_INCOP C_SIZEOF
%left '[' '(' C_STROP
%{
# include "pass1.h"
# include <stdarg.h>
# include <string.h>
# include <stdlib.h>

int fun_inline;	/* Reading an inline function */
int oldstyle;	/* Current function being defined */
static struct symtab *xnf;
extern int enummer, tvaloff, inattr;
extern struct rstack *rpole;
static int ctval, widestr;
NODE *cftnod;
static int attrwarn = 0;

#define	NORETYP	SNOCREAT /* no return type, save in unused field in symtab */

       NODE *bdty(int op, ...);
static void fend(void);
static void fundef(NODE *tp, NODE *p);
static void olddecl(NODE *p, NODE *a);
static struct symtab *init_declarator(NODE *tn, NODE *p, int assign, NODE *a);
static void resetbc(int mask);
static void swend(void);
static void addcase(NODE *p);
#ifdef GCC_COMPAT
static void gcccase(NODE *p, NODE *);
#endif
static void adddef(void);
static void savebc(void);
static void swstart(int, TWORD);
static void genswitch(int, TWORD, struct swents **, int);
static char *mkpstr(char *str);
static struct symtab *clbrace(NODE *);
static NODE *cmop(NODE *l, NODE *r);
static NODE *xcmop(NODE *out, NODE *in, NODE *str);
static void mkxasm(char *str, NODE *p);
static NODE *xasmop(char *str, NODE *p);
static int maxstlen(char *str);
static char *stradd(char *old, char *new);
static NODE *biop(int op, NODE *l, NODE *r);
static void flend(void);
static char * simname(char *s);
#ifdef GCC_COMPAT
static NODE *tyof(NODE *);	/* COMPAT_GCC */
static NODE *voidcon(void);	/* COMPAT_GCC */
#endif
static void funargs(NODE *p);
static void oldargs(NODE *p);
static void bfix(int a);


/*
 * State for saving current switch state (when nested switches).
 */
struct savbc {
	struct savbc *next;
	int brklab;
	int contlab;
	int flostat;
	int swx;
} *savbc, *savctx;

%}

%union {
	int intval;
	NODE *nodep;
	struct symtab *symp;
	struct rstack *rp;
	char *strp;
}

	/* define types */
%start ext_def_list

%type <intval> con_e ifelprefix ifprefix whprefix forprefix doprefix switchpart
		xbegin incblev
%type <nodep> e .e term enum_dcl struct_dcl cast_type declarator
		elist type_sq cf_spec merge_attribs
		parameter_declaration abstract_declarator initializer
		parameter_type_list parameter_list addrlbl
		declaration_specifiers
		specifier_qualifier_list merge_specifiers nocon_e
		identifier_list arg_param_list type_qualifier_list
		designator_list designator xasm oplist oper cnstr funtype
		typeof attribute attribute_specifier /* COMPAT_GCC */
		attribute_list attr_spec_list attr_var /* COMPAT_GCC */
%type <strp>	string C_STRING
%type <rp>	str_head
%type <symp>	xnfdeclarator clbrace enum_head

%type <intval>  C_STRUCT C_RELOP C_DIVOP C_SHIFTOP
		C_ANDAND C_OROR C_STROP C_INCOP C_UNOP C_ASOP C_EQUOP

%type <nodep>   C_TYPE C_QUALIFIER C_ICON C_FCON C_CLASS
%type <strp>	C_NAME C_TYPENAME
%%

ext_def_list:	   ext_def_list external_def
		| { ftnend(); }
		;

external_def:	   funtype kr_args compoundstmt { fend(); }
		|  declaration  { blevel = 0; symclear(0); }
		|  asmstatement ';'
		|  ';'
		|  error { blevel = 0; }
		;

funtype:	  /* no type given */ declarator {
		    fundef(mkty(INT, 0, MKSUE(INT)), $1);
		    cftnsp->sflags |= NORETYP;
		}
		| declaration_specifiers declarator { fundef($1,$2); }
		;

kr_args:	  /* empty */
		| arg_dcl_list
		;

/*
 * Returns a node pointer or NULL, if no types at all given.
 * Type trees are checked for correctness and merged into one
 * type node in typenode().
 */
declaration_specifiers:
		   merge_attribs { $$ = typenode($1); }
		;

merge_attribs:	   type_sq { $$ = $1; }
		|  type_sq merge_attribs { $$ = cmop($2, $1); }
		|  cf_spec { $$ = $1; }
		|  cf_spec merge_attribs { $$ = cmop($2, $1); }
		;

type_sq:	   C_TYPE { $$ = $1; }
		|  C_TYPENAME { 
			struct symtab *sp = lookup($1, 0);
			if (sp->stype == ENUMTY)
				sp->stype = sp->ssue->suem->stype;
			$$ = mkty(sp->stype, sp->sdf, sp->ssue);
			$$->n_sp = sp;
		}
		|  struct_dcl { $$ = $1; }
		|  enum_dcl { $$ = $1; }
		|  C_QUALIFIER { $$ = $1; }
		|  attribute_specifier { $$ = biop(ATTRIB, $1, 0); }
		|  typeof { $$ = $1; }
		;

cf_spec:	   C_CLASS { $$ = $1; }
		|  C_FUNSPEC { fun_inline = 1;  /* XXX - hack */
			$$ = block(QUALIFIER, NIL, NIL, 0, 0, 0); }
		;

typeof:		   C_TYPEOF '(' term ')' {
#ifdef GCC_COMPAT
			struct symtab *sp;

			if ($3->n_op == NAME) {
				sp = lookup((char *)$3->n_sp, SNOCREAT);
				if (sp != NULL) {
					$3->n_type = sp->stype;
					$3->n_df = sp->sdf;
					$3->n_sue = sp->ssue;
				} else
					uerror("typeof on undeclared variable");
			}
			$$ = tyof($3);
#endif
		} /* COMPAT_GCC */
 /*COMPAT_GCC*/	|  C_TYPEOF '(' cast_type ')' {
#ifdef GCC_COMPAT
			 $$ = tyof($3);
#endif
		}
 /*COMPAT_GCC*/	;

attribute_specifier :
		   C_ATTRIBUTE '(' '(' attribute_list ')' ')' { $$ = $4; }
 /*COMPAT_GCC*/	;

attribute_list:	   attribute
		|  attribute ',' attribute_list { $$ = cmop($3, $1); }
		;

attribute:	   {
#ifdef GCC_COMPAT
			 $$ = voidcon();
#endif
		}
		|  C_NAME { $$ = bdty(NAME, $1); }
		|  C_NAME '(' elist ')' {
			$$ = bdty($3 == NIL ? UCALL : CALL, bdty(NAME, $1), $3);
		}
		;

/*
 * Adds a pointer list to front of the declarators.
 */
declarator:	   '*' declarator { $$ = bdty(UMUL, $2); }
		|  '*' type_qualifier_list declarator {
			$$ = bdty(UMUL, $3);
			if ($2->n_op == QUALIFIER)
				$$->n_qual = $2->n_type;
			else
				werror("FIXME: attributes discarding qualifiers");
			tfree($2);
		}
		|  C_NAME { $$ = bdty(NAME, $1); }
		|  '(' declarator ')' { $$ = $2; }
		|  declarator '[' nocon_e ']' {
			$3 = optim($3);
			if ((blevel == 0 || rpole != NULL) && !nncon($3))
				uerror("array size not constant");
			/*
			 * Checks according to 6.7.5.2 clause 1:
			 * "...the expression shall have an integer type."
			 * "If the expression is a constant expression,
			 * it shall have a value greater than zero."
			 */
			if (!ISINTEGER($3->n_type))
				werror("array size is not an integer");
			else if ($3->n_op == ICON) {
				if ($3->n_lval < 0) {
					uerror("array size cannot be negative");
					$3->n_lval = 1;
				}
#ifdef notyet
				if ($3->n_lval == 0 && Wgcc)
					werror("gcc extension; zero size");
#endif
			}
			$$ = biop(LB, $1, $3);
		}
		|  declarator '[' ']' {
			$$ = biop(LB, $1, bcon(NOOFFSET));
		}
		|  declarator '(' incblev parameter_type_list ')' {
			$$ = bdty(CALL, $1, $4);
			bfix($3);
			if (blevel > 0)
				symclear(blevel);
		}
		|  declarator '(' incblev identifier_list ')' {
			$$ = bdty(CALL, $1, $4);
			if (blevel != 1)
				uerror("function declaration in bad context");
			oldstyle = 1;
			bfix($3);
		}
		|  declarator '(' incblev ')' {
			$$ = bdty(UCALL, $1);
			bfix($3);
		}
		;

incblev:	   {
			++blevel;
			if (blevel == 1) {
				ctval = tvaloff;
				argoff = ARGINIT;
				$$ = Wshadow;
				Wshadow = 0;
			} else if (blevel == 2)
				$$ = argoff;
		}
		;

type_qualifier_list:
		   C_QUALIFIER { $$ = $1; }
		|  type_qualifier_list C_QUALIFIER {
			$$ = $1;
			$$->n_type |= $2->n_type;
			nfree($2);
		}
		|  attribute_specifier { $$ = $1; }
		|  type_qualifier_list attribute_specifier {
			if ($1->n_op == QUALIFIER) {
				$$ = $2;
				$$->n_type = $1->n_type;
				tfree($1);
			} else {
				$$ = cmop($1, $2);
				$$->n_type = $1->n_type;
			}
		}
		;

identifier_list:   C_NAME { $$ = bdty(NAME, $1); oldargs($$); }
		|  identifier_list ',' C_NAME { $$ = cmop($1, bdty(NAME, $3)); oldargs($$->n_right); }
		;

/*
 * Returns as parameter_list, but can add an additional ELLIPSIS node.
 */
parameter_type_list:
		   parameter_list { $$ = $1; }
		|  parameter_list ',' C_ELLIPSIS {
			$$ = cmop($1, biop(ELLIPSIS, NIL, NIL));
		}
		;

/*
 * Returns a linked lists of nodes of op CM with parameters on
 * its right and additional CM nodes of its left pointer.
 * No CM nodes if only one parameter.
 */
parameter_list:	   parameter_declaration { $$ = $1; }
		|  parameter_list ',' parameter_declaration {
			$$ = cmop($1, $3);
		}
		;

/*
 * Returns a node pointer to the declaration.
 */
parameter_declaration:
		   declaration_specifiers declarator attr_var {
			if ($1->n_lval != SNULL && $1->n_lval != REGISTER)
				uerror("illegal parameter class");
			$2->n_sue = NULL; /* no attributes */
			$$ = tymerge($1, $2);
			nfree($1);
			if ($3) {
				if (attrwarn)
					werror("unhandled parameter_declaration attribute");
				tfree($3);
			}
			funargs($$);
		}
		|  declaration_specifiers abstract_declarator { 
			$2->n_sue = NULL; /* no attributes */
			$$ = tymerge($1, $2);
			tfree($1);
		}
		|  declaration_specifiers {
			$$ = bdty(NAME, NULL);
			$$->n_sue = NULL; /* no attributes */
			$$ = tymerge($1, $$);
			tfree($1);
		}
		;

abstract_declarator:
		   '*' { $$ = bdty(UMUL, bdty(NAME, NULL)); }
		|  '*' type_qualifier_list {
			$$ = bdty(UMUL, bdty(NAME, NULL)); tfree($2);
		}
		|  '*' abstract_declarator { $$ = bdty(UMUL, $2); }
		|  '*' type_qualifier_list abstract_declarator {
			$$ = bdty(UMUL, $3); tfree($2);
		}
		|  '(' abstract_declarator ')' { $$ = $2; }
		|  '[' ']' attr_var {
			$$ = biop(LB, bdty(NAME, NULL), bcon(NOOFFSET));
			if ($3) tfree($3);
		}
		|  '[' nocon_e ']' attr_var {
			$$ = biop(LB, bdty(NAME, NULL), $2);
			if ($4) {
				if (attrwarn)
					werror("unhandled abstract_declarator attribute");
				tfree($4);
			}
		}
		|  abstract_declarator '[' ']' attr_var {
			$$ = biop(LB, $1, bcon(NOOFFSET));
			if ($4) {
				if (attrwarn)
					werror("unhandled abstract_declarator2 attribute");
				tfree($4);
			}
		}
		|  abstract_declarator '[' nocon_e ']' attr_var {
			$$ = biop(LB, $1, $3);
			if ($5) {
				if (attrwarn)
					werror("unhandled abstract_declarator3 attribute");
				tfree($5);
			}
		}
		|  '(' ')' { $$ = bdty(UCALL, bdty(NAME, NULL)); }
		|  '(' ib2 parameter_type_list ')' {
			$$ = bdty(CALL, bdty(NAME, NULL), $3);
			if (--blevel > 0)
				symclear(blevel);
		}
		|  abstract_declarator '(' ')' {
			$$ = bdty(UCALL, $1);
		}
		|  abstract_declarator '(' ib2 parameter_type_list ')' {
			$$ = bdty(CALL, $1, $4);
			if (--blevel > 0)
				symclear(blevel);
		}
		;

ib2:		  { blevel++; }
		;
/*
 * K&R arg declaration, between ) and {
 */
arg_dcl_list:	   arg_declaration
		|  arg_dcl_list arg_declaration
		;


arg_declaration:   declaration_specifiers arg_param_list ';' {
			nfree($1);
		}
		;

arg_param_list:	   declarator attr_var { olddecl(tymerge($<nodep>0, $1), $2); }
		|  arg_param_list ',' declarator attr_var {
			olddecl(tymerge($<nodep>0, $3), $4);
		}
		;

/*
 * Declarations in beginning of blocks.
 */
block_item_list:   block_item
		|  block_item_list block_item
		;

block_item:	   declaration
		|  statement
		;

/*
 * Here starts the old YACC code.
 */

/*
 * Variables are declared in init_declarator.
 */
declaration:	   declaration_specifiers ';' { tfree($1); fun_inline = 0; }
		|  declaration_specifiers init_declarator_list ';' {
			tfree($1);
			fun_inline = 0;
		}
		;

/*
 * Normal declaration of variables. curtype contains the current type node.
 * Returns nothing, variables are declared in init_declarator.
 */
init_declarator_list:
		   init_declarator
		|  init_declarator_list ',' attr_var { $<nodep>$ = $<nodep>0; } init_declarator {
			if ($3) {
				if (attrwarn)
					werror("unhandled init_declarator attribute");
				tfree($3);
			}
		}
		;

enum_dcl:	   enum_head '{' moe_list optcomma '}' { $$ = enumdcl($1); }
		|  C_ENUM C_NAME {  $$ = enumref($2); }
		;

enum_head:	   C_ENUM { $$ = enumhd(NULL); }
		|  C_ENUM C_NAME {  $$ = enumhd($2); }
		;

moe_list:	   moe
		|  moe_list ',' moe
		;

moe:		   C_NAME {  moedef($1); }
		|  C_TYPENAME {  moedef($1); }
		|  C_NAME '=' con_e { enummer = $3; moedef($1); }
		|  C_TYPENAME '=' con_e { enummer = $3; moedef($1); }
		;

struct_dcl:	   str_head '{' struct_dcl_list '}' { $$ = dclstruct($1); }
		|  C_STRUCT attr_var C_NAME {  $$ = rstruct($3,$1);
			if ($2) {
				if (attrwarn)
					werror("unhandled struct_dcl attribute");
				tfree($2);
			}
		}
 /*COMPAT_GCC*/	|  str_head '{' '}' { $$ = dclstruct($1); }
		;

attr_var:	   {	
			NODE *q, *p;

			p = pragma_aligned ? bdty(CALL, bdty(NAME, "aligned"),
			    bcon(pragma_aligned)) : NIL;
			if (pragma_packed) {
				q = bdty(NAME, "packed");
				p = (p == NIL ? q : cmop(p, q));
			}
			pragma_aligned = pragma_packed = 0;
			$$ = p;
		}
 /*COMPAT_GCC*/	|  attr_spec_list
		;

attr_spec_list:	   attribute_specifier 
		|  attr_spec_list attribute_specifier { $$ = cmop($1, $2); }
		;

str_head:	   C_STRUCT attr_var {  $$ = bstruct(NULL, $1, $2);  }
		|  C_STRUCT attr_var C_NAME {  $$ = bstruct($3,$1, $2);  }
		;

struct_dcl_list:   struct_declaration
		|  struct_dcl_list struct_declaration
		;

struct_declaration:
		   specifier_qualifier_list struct_declarator_list optsemi {
			nfree($1);
		}
		;

optsemi:	   ';' { }
		|  optsemi ';' { werror("extra ; in struct"); }
		;

specifier_qualifier_list:
		   merge_specifiers { $$ = typenode($1); }
		;

merge_specifiers:  type_sq merge_specifiers { $$ = cmop($2, $1); }
		|  type_sq { $$ = $1; }
		;

struct_declarator_list:
		   struct_declarator { }
		|  struct_declarator_list ',' { $<nodep>$=$<nodep>0; } 
			struct_declarator { }
		;

struct_declarator: declarator attr_var {
			tymerge($<nodep>0, $1);
			soumemb($1, (char *)$1->n_sp, 0);
			nfree($1);
			if ($2) {
				if (attrwarn)
					werror("unhandled struct_declarator attribute");
				tfree($2);
			}
		}
		|  ':' con_e {
			if (fldchk($2))
				$2 = 1;
			falloc(NULL, $2, $<nodep>0);
		}
		|  declarator ':' con_e {
			if (fldchk($3))
				$3 = 1;
			if ($1->n_op == NAME) {
				tymerge($<nodep>0, $1);
				soumemb($1, (char *)$1->n_sp, FIELD | $3);
				nfree($1);
			} else
				uerror("illegal declarator");
		}
		| /* unnamed member */ {
			NODE *p = $<nodep>0;
			char *c = permalloc(10);

			if (p->n_type != STRTY && p->n_type != UNIONTY)
				uerror("bad unnamed member type");
			snprintf(c, 10, "*%dFAKE", getlab());
			soumemb(p, c, 0);
		}
		;

		/* always preceeded by attributes */
xnfdeclarator:	   declarator attr_var {
			$$ = xnf = init_declarator($<nodep>0, $1, 1, $2);
		}
		;

/*
 * Handles declarations and assignments.
 * Returns nothing.
 */
init_declarator:   declarator attr_var { init_declarator($<nodep>0, $1, 0, $2);}
		|  declarator C_ASM '(' string ')' {
#ifdef GCC_COMPAT
			pragma_renamed = newstring($4, strlen($4));
			init_declarator($<nodep>0, $1, 0, NULL);
#else
			werror("gcc extension");
			init_declarator($<nodep>0, $1, 0, NULL);
#endif
		}
		|  xnfdeclarator '=' e { simpleinit($1, eve($3)); xnf = NULL; }
		|  xnfdeclarator '=' begbr init_list optcomma '}' {
			endinit();
			xnf = NULL;
		}
 /*COMPAT_GCC*/	|  xnfdeclarator '=' begbr '}' { endinit(); xnf = NULL; }
		|  xnfdeclarator '=' addrlbl { simpleinit($1, $3); xnf = NULL; }
		;

begbr:		   '{' { beginit($<symp>-1); }
		;

initializer:	   e %prec ',' {  $$ = eve($1); }
		|  addrlbl {  $$ = $1; }
		|  ibrace init_list optcomma '}' { $$ = NULL; }
		|  ibrace '}' { asginit(bcon(0)); $$ = NULL; }
		;

init_list:	   designation initializer { asginit($2); }
		|  init_list ','  designation initializer { asginit($4); }
		;

designation:	   designator_list '=' { desinit($1); }
		|  { /* empty */ }
		;

designator_list:   designator { $$ = $1; }
		|  designator_list designator { $$ = $2; $$->n_left = $1; }
		;

designator:	   '[' con_e ']' {
			if ($2 < 0) {
				uerror("designator must be non-negative");
				$2 = 0;
			}
			$$ = biop(LB, NIL, bcon($2));
		}
		|  C_STROP C_NAME {
			if ($1 != DOT)
				uerror("invalid designator");
			$$ = bdty(NAME, $2);
		}
		;

optcomma	:	/* VOID */
		|  ','
		;

ibrace:		   '{' {  ilbrace(); }
		;

/*	STATEMENTS	*/

compoundstmt:	   begin block_item_list '}' { flend(); }
		|  begin '}' { flend(); }
		;

begin:		  '{' {
			struct savbc *bc = tmpalloc(sizeof(struct savbc));
			if (blevel == 1) {
#ifdef STABS
				if (gflag)
					stabs_line(lineno);
#endif
				dclargs();
			}
#ifdef STABS
			if (gflag && blevel > 1)
				stabs_lbrac(blevel+1);
#endif
			++blevel;
			oldstyle = 0;
			bc->contlab = autooff;
			bc->next = savctx;
			savctx = bc;
			bccode();
			if (sspflag && blevel == 2)
				sspstart();
		}
		;

statement:	   e ';' { ecomp(eve($1)); symclear(blevel); }
		|  compoundstmt
		|  ifprefix statement { plabel($1); reached = 1; }
		|  ifelprefix statement {
			if ($1 != NOLAB) {
				plabel( $1);
				reached = 1;
			}
		}
		|  whprefix statement {
			branch(contlab);
			plabel( brklab );
			if( (flostat&FBRK) || !(flostat&FLOOP))
				reached = 1;
			else
				reached = 0;
			resetbc(0);
		}
		|  doprefix statement C_WHILE '(' e ')' ';' {
			plabel(contlab);
			if (flostat & FCONT)
				reached = 1;
			if (reached)
				cbranch(eve($5), bcon($1));
			else
				tfree(eve($5));
			plabel( brklab);
			reached = 1;
			resetbc(0);
		}
		|  forprefix .e ')' statement
			{  plabel( contlab );
			    if( flostat&FCONT ) reached = 1;
			    if( $2 ) ecomp( $2 );
			    branch($1);
			    plabel( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP) ) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    }
		| switchpart statement
			{ if( reached ) branch( brklab );
			    plabel( $1 );
			    swend();
			    plabel( brklab);
			    if( (flostat&FBRK) || !(flostat&FDEF) ) reached = 1;
			    resetbc(FCONT);
			    }
		|  C_BREAK  ';' {
			if (brklab == NOLAB)
				uerror("illegal break");
			else if (reached)
				branch(brklab);
			flostat |= FBRK;
			reached = 0;
		}
		|  C_CONTINUE  ';' {
			if (contlab == NOLAB)
				uerror("illegal continue");
			else
				branch(contlab);
			flostat |= FCONT;
			goto rch;
		}
		|  C_RETURN  ';' {
			branch(retlab);
			if (cftnsp->stype != VOID && 
			    (cftnsp->sflags & NORETYP) == 0 &&
			    cftnsp->stype != VOID+FTN)
				uerror("return value required");
			rch:
			if (!reached && Wunreachable_code)
				werror( "statement is not reached");
			reached = 0;
		}
		|  C_RETURN e  ';' {
			NODE *p, *q;

			p = nametree(cftnsp);
			p->n_type = DECREF(p->n_type);
			q = eve($2);
#ifndef NO_COMPLEX
			if (ANYCX(q) || ANYCX(p))
				q = cxret(q, p);
#endif
			p = buildtree(RETURN, p, q);
			if (p->n_type == VOID) {
				ecomp(p->n_right);
			} else {
				if (cftnod == NIL)
					cftnod = tempnode(0, p->n_type,
					    p->n_df, p->n_sue);
				ecomp(buildtree(ASSIGN,
				    ccopy(cftnod), p->n_right));
			}
			tfree(p->n_left);
			nfree(p);
			branch(retlab);
			reached = 0;
		}
		|  C_GOTO C_NAME ';' { gotolabel($2); goto rch; }
		|  C_GOTO '*' e ';' { ecomp(biop(GOTO, eve($3), NIL)); }
		|  asmstatement ';'
		|   ';'
		|  error  ';'
		|  error '}'
		|  label statement
		;

asmstatement:	   C_ASM mvol '(' string ')' { send_passt(IP_ASM, mkpstr($4)); }
		|  C_ASM mvol '(' string xasm ')' { mkxasm($4, $5); }
		;

mvol:		   /* empty */
		|  C_QUALIFIER { nfree($1); }
		;

xasm:		   ':' oplist { $$ = xcmop($2, NIL, NIL); }
		|  ':' oplist ':' oplist { $$ = xcmop($2, $4, NIL); }
		|  ':' oplist ':' oplist ':' cnstr { $$ = xcmop($2, $4, $6); }
		;

oplist:		   /* nothing */ { $$ = NIL; }
		|  oper { $$ = $1; }
		;

oper:		   string '(' e ')' { $$ = xasmop($1, eve($3)); }
		|  oper ',' string '(' e ')' {
			$$ = cmop($1, xasmop($3, eve($5)));
		}
		;

cnstr:		   string { $$ = xasmop($1, bcon(0)); }
		|  cnstr ',' string { $$ = cmop($1, xasmop($3, bcon(0))); }
                ;

label:		   C_NAME ':' { deflabel($1); reached = 1; }
		|  C_TYPENAME ':' { deflabel($1); reached = 1; }
		|  C_CASE e ':' { addcase(eve($2)); reached = 1; }
/* COMPAT_GCC */|  C_CASE e C_ELLIPSIS e ':' {
#ifdef GCC_COMPAT
			gcccase(eve($2), eve($4)); reached = 1;
#endif
		}
		|  C_DEFAULT ':' { reached = 1; adddef(); flostat |= FDEF; }
		;

doprefix:	C_DO {
			savebc();
			brklab = getlab();
			contlab = getlab();
			plabel(  $$ = getlab());
			reached = 1;
		}
		;
ifprefix:	C_IF '(' e ')' {
			cbranch(buildtree(NOT, eve($3), NIL), bcon($$ = getlab()));
			reached = 1;
		}
		;
ifelprefix:	  ifprefix statement C_ELSE {
			if (reached)
				branch($$ = getlab());
			else
				$$ = NOLAB;
			plabel( $1);
			reached = 1;
		}
		;

whprefix:	  C_WHILE  '('  e  ')' {
			savebc();
			$3 = eve($3);
			if ($3->n_op == ICON && $3->n_lval != 0)
				flostat = FLOOP;
			plabel( contlab = getlab());
			reached = 1;
			brklab = getlab();
			if (flostat == FLOOP)
				tfree($3);
			else
				cbranch(buildtree(NOT, $3, NIL), bcon(brklab));
		}
		;
forprefix:	  C_FOR  '('  .e  ';' .e  ';' {
			if ($3)
				ecomp($3);
			savebc();
			contlab = getlab();
			brklab = getlab();
			plabel( $$ = getlab());
			reached = 1;
			if ($5)
				cbranch(buildtree(NOT, $5, NIL), bcon(brklab));
			else
				flostat |= FLOOP;
		}
		|  C_FOR '(' { ++blevel; } declaration .e ';' {
			blevel--;
			savebc();
			contlab = getlab();
			brklab = getlab();
			plabel( $$ = getlab());
			reached = 1;
			if ($5)
				cbranch(buildtree(NOT, $5, NIL), bcon(brklab));
			else
				flostat |= FLOOP;
		}
		;

switchpart:	   C_SWITCH  '('  e ')' {
			NODE *p;
			int num;
			TWORD t;

			savebc();
			brklab = getlab();
			$3 = eve($3);
			if (($3->n_type != BOOL && $3->n_type > ULONGLONG) ||
			    $3->n_type < CHAR) {
				uerror("switch expression must have integer "
				       "type");
				t = INT;
			} else {
				$3 = intprom($3);
				t = $3->n_type;
			}
			p = tempnode(0, t, 0, MKSUE(t));
			num = regno(p);
			ecomp(buildtree(ASSIGN, p, $3));
			branch( $$ = getlab());
			swstart(num, t);
			reached = 0;
		}
		;
/*	EXPRESSIONS	*/
con_e:		{ $<rp>$ = rpole; rpole = NULL; } e %prec ',' {
			$$ = icons(eve($2));
			rpole = $<rp>1;
		}
		;

nocon_e:	{ $<rp>$ = rpole; rpole = NULL; } e %prec ',' {
			rpole = $<rp>1;
			$$ = eve($2);
		}
		;

.e:		   e { $$ = eve($1); }
		| 	{ $$=0; }
		;

elist:		   { $$ = NIL; }
		|  e %prec ','
		|  elist  ','  e { $$ = biop(CM, $1, $3); }
		|  elist  ','  cast_type { /* hack for stdarg */
			$3->n_op = TYPE;
			$$ = biop(CM, $1, $3);
		}
		;

/*
 * Precedence order of operators.
 */
e:		   e ',' e { $$ = biop(COMOP, $1, $3); }
		|  e '=' e {  $$ = biop(ASSIGN, $1, $3); }
		|  e C_ASOP e {  $$ = biop($2, $1, $3); }
		|  e '?' e ':' e {
			$$=biop(QUEST, $1, biop(COLON, $3, $5));
		}
		|  e C_OROR e { $$ = biop($2, $1, $3); }
		|  e C_ANDAND e { $$ = biop($2, $1, $3); }
		|  e '|' e { $$ = biop(OR, $1, $3); }
		|  e '^' e { $$ = biop(ER, $1, $3); }
		|  e '&' e { $$ = biop(AND, $1, $3); }
		|  e C_EQUOP  e { $$ = biop($2, $1, $3); }
		|  e C_RELOP e { $$ = biop($2, $1, $3); }
		|  e C_SHIFTOP e { $$ = biop($2, $1, $3); }
		|  e '+' e { $$ = biop(PLUS, $1, $3); }
		|  e '-' e { $$ = biop(MINUS, $1, $3); }
		|  e C_DIVOP e { $$ = biop($2, $1, $3); }
		|  e '*' e { $$ = biop(MUL, $1, $3); }
		|  e '=' addrlbl { $$ = biop(ASSIGN, $1, $3); }
		|  term
		;

xbegin:		   begin {
			$$ = getlab(); getlab(); getlab();
			branch($$); plabel(($$)+1); }
		;

addrlbl:	  C_ANDAND C_NAME {
#ifdef GCC_COMPAT
			struct symtab *s = lookup($2, SLBLNAME);
			if (s->soffset == 0)
				s->soffset = -getlab();
			$$ = buildtree(ADDROF, nametree(s), NIL);
#else
			uerror("gcc extension");
#endif
		}
		;

term:		   term C_INCOP {  $$ = biop($2, $1, bcon(1)); }
		|  '*' term { $$ = biop(UMUL, $2, NIL); }
		|  '&' term { $$ = biop(ADDROF, $2, NIL); }
		|  '-' term { $$ = biop(UMINUS, $2, NIL ); }
		|  '+' term { $$ = $2; }
		|  C_UNOP term { $$ = biop($1, $2, NIL); }
		|  C_INCOP term {
			$$ = biop($1 == INCR ? PLUSEQ : MINUSEQ, $2, bcon(1));
		}
		|  C_SIZEOF xa term { $$ = biop(SZOF, $3, bcon(0)); inattr = $<intval>2; }
		|  '(' cast_type ')' term  %prec C_INCOP {
			$$ = biop(CAST, $2, $4);
		}
		|  C_SIZEOF xa '(' cast_type ')'  %prec C_SIZEOF {
			$$ = biop(SZOF, $4, bcon(1));
			inattr = $<intval>2;
		}
		| '(' cast_type ')' clbrace init_list optcomma '}' {
			endinit();
			$$ = bdty(NAME, $4);
			$$->n_op = CLOP;
		}
		|  term '[' e ']' { $$ = biop(LB, $1, $3); }
		|  C_NAME  '(' elist ')' {
			$$ = biop($3 ? CALL : UCALL, bdty(NAME, $1), $3);
		}
		|  term  '(' elist ')' { $$ = biop($3 ? CALL : UCALL, $1, $3); }
		|  term C_STROP C_NAME { $$ = biop($2, $1, bdty(NAME, $3)); }
		|  term C_STROP C_TYPENAME { $$ = biop($2, $1, bdty(NAME, $3));}
		|  C_NAME %prec C_SIZEOF /* below ( */{ $$ = bdty(NAME, $1); }
		|  C_ICON { $$ = $1; }
		|  C_FCON { $$ = $1; }
		|  string { $$ = bdty(STRING, $1, widestr); }
		|   '('  e  ')' { $$=$2; }
		|  '(' xbegin block_item_list e ';' '}' ')' {
			/* XXX - check recursive ({ }) statements */
			branch(($2)+2);
			plabel($2);
			$$ = buildtree(COMOP,
			    biop(GOTO, bcon(($2)+1), NIL), eve($4));
			flend();
		}
		;

xa:		  { $<intval>$ = inattr; inattr = 0; }
		;

clbrace:	   '{'	{ $$ = clbrace($<nodep>-1); }
		;

string:		   C_STRING { widestr = $1[0] == 'L'; $$ = stradd("", $1); }
		|  string C_STRING { $$ = stradd($1, $2); }
		;

cast_type:	   specifier_qualifier_list {
			$$ = tymerge($1, bdty(NAME, NULL));
			nfree($1);
		}
		|  specifier_qualifier_list abstract_declarator {
			$$ = tymerge($1, $2);
			nfree($1);
		}
		;

%%

NODE *
mkty(TWORD t, union dimfun *d, struct suedef *sue)
{
	return block(TYPE, NIL, NIL, t, d, sue);
}

NODE *
bdty(int op, ...)
{
	va_list ap;
	int val;
	register NODE *q;

	va_start(ap, op);
	q = biop(op, NIL, NIL);

	switch (op) {
	case UMUL:
	case UCALL:
		q->n_left = va_arg(ap, NODE *);
		q->n_rval = 0;
		break;

	case CALL:
		q->n_left = va_arg(ap, NODE *);
		q->n_right = va_arg(ap, NODE *);
		break;

	case LB:
		q->n_left = va_arg(ap, NODE *);
		if ((val = va_arg(ap, int)) <= 0) {
			uerror("array size must be positive");
			val = 1;
		}
		q->n_right = bcon(val);
		break;

	case NAME:
		q->n_sp = va_arg(ap, struct symtab *); /* XXX survive tymerge */
		break;

	case STRING:
		q->n_name = va_arg(ap, char *);
		q->n_lval = va_arg(ap, int);
		break;

	default:
		cerror("bad bdty");
	}
	va_end(ap);

	return q;
}

static void
flend(void)
{
	if (sspflag && blevel == 2)
		sspend();
#ifdef STABS
	if (gflag && blevel > 2)
		stabs_rbrac(blevel);
#endif
	--blevel;
	if( blevel == 1 )
		blevel = 0;
	symclear(blevel); /* Clean ut the symbol table */
	if (autooff > maxautooff)
		maxautooff = autooff;
	autooff = savctx->contlab;
	savctx = savctx->next;
}

static void
savebc(void)
{
	struct savbc *bc = tmpalloc(sizeof(struct savbc));

	bc->brklab = brklab;
	bc->contlab = contlab;
	bc->flostat = flostat;
	bc->next = savbc;
	savbc = bc;
	flostat = 0;
}

static void
resetbc(int mask)
{
	flostat = savbc->flostat | (flostat&mask);
	contlab = savbc->contlab;
	brklab = savbc->brklab;
	savbc = savbc->next;
}

struct swdef {
	struct swdef *next;	/* Next in list */
	int deflbl;		/* Label for "default" */
	struct swents *ents;	/* Linked sorted list of case entries */
	int nents;		/* # of entries in list */
	int num;		/* Node value will end up in */
	TWORD type;		/* Type of switch expression */
} *swpole;

/*
 * add case to switch
 */
static void
addcase(NODE *p)
{
	struct swents **put, *w, *sw = tmpalloc(sizeof(struct swents));
	CONSZ val;

	p = optim(p);  /* change enum to ints */
	if (p->n_op != ICON || p->n_sp != NULL) {
		uerror( "non-constant case expression");
		return;
	}
	if (swpole == NULL) {
		uerror("case not in switch");
		return;
	}

	if (DEUNSIGN(swpole->type) != DEUNSIGN(p->n_type)) {
		val = p->n_lval;
		p = makety(p, swpole->type, 0, 0, MKSUE(swpole->type));
		if (p->n_op != ICON)
			cerror("could not cast case value to type of switch "
			       "expression");
		if (p->n_lval != val)
			werror("case expression truncated");
	}
	sw->sval = p->n_lval;
	tfree(p);
	put = &swpole->ents;
	if (ISUNSIGNED(swpole->type)) {
		for (w = swpole->ents;
		     w != NULL && (U_CONSZ)w->sval < (U_CONSZ)sw->sval;
		     w = w->next)
			put = &w->next;
	} else {
		for (w = swpole->ents; w != NULL && w->sval < sw->sval;
		     w = w->next)
			put = &w->next;
	}
	if (w != NULL && w->sval == sw->sval) {
		uerror("duplicate case in switch");
		return;
	}
	plabel(sw->slab = getlab());
	*put = sw;
	sw->next = w;
	swpole->nents++;
}

#ifdef GCC_COMPAT
void
gcccase(NODE *ln, NODE *hn)
{
	CONSZ i, l, h;

	l = icons(optim(ln));
	h = icons(optim(hn));

	if (h < l)
		i = l, l = h, h = i;

	for (i = l; i <= h; i++)
		addcase(xbcon(i, NULL, hn->n_type));
}
#endif

/*
 * add default case to switch
 */
static void
adddef(void)
{
	if (swpole == NULL)
		uerror("default not inside switch");
	else if (swpole->deflbl != 0)
		uerror("duplicate default in switch");
	else
		plabel( swpole->deflbl = getlab());
}

static void
swstart(int num, TWORD type)
{
	struct swdef *sw = tmpalloc(sizeof(struct swdef));

	sw->deflbl = sw->nents = 0;
	sw->ents = NULL;
	sw->next = swpole;
	sw->num = num;
	sw->type = type;
	swpole = sw;
}

/*
 * end a switch block
 */
static void
swend(void)
{
	struct swents *sw, **swp;
	int i;

	sw = tmpalloc(sizeof(struct swents));
	swp = tmpalloc(sizeof(struct swents *) * (swpole->nents+1));

	sw->slab = swpole->deflbl;
	swp[0] = sw;

	for (i = 1; i <= swpole->nents; i++) {
		swp[i] = swpole->ents;
		swpole->ents = swpole->ents->next;
	}
	genswitch(swpole->num, swpole->type, swp, swpole->nents);

	swpole = swpole->next;
}

/*
 * num: tempnode the value of the switch expression is in
 * type: type of the switch expression
 *
 * p points to an array of structures, each consisting
 * of a constant value and a label.
 * The first is >=0 if there is a default label;
 * its value is the label number
 * The entries p[1] to p[n] are the nontrivial cases
 * n is the number of case statements (length of list)
 */
static void
genswitch(int num, TWORD type, struct swents **p, int n)
{
	NODE *r, *q;
	int i;

	if (mygenswitch(num, type, p, n))
		return;

	/* simple switch code */
	for (i = 1; i <= n; ++i) {
		/* already in 1 */
		r = tempnode(num, type, 0, MKSUE(type));
		q = xbcon(p[i]->sval, NULL, type);
		r = buildtree(NE, r, clocal(q));
		cbranch(buildtree(NOT, r, NIL), bcon(p[i]->slab));
	}
	if (p[0]->slab > 0)
		branch(p[0]->slab);
}

static NODE *
trmerg(NODE *p, NODE *a)
{
	NODE *q = p;

	if (p->n_op != CM)
		return cmop(a, p);

	while (q->n_left->n_op == CM)
		q = q->n_left;
	q->n_left = cmop(a, q->n_left);
	return p;
}

/*
 * Declare a variable or prototype.
 */
static struct symtab *
init_declarator(NODE *tn, NODE *p, int assign, NODE *a)
{
	int class = tn->n_lval;
	NODE *typ;

#ifdef GCC_COMPAT
	if (tn->n_op == CM)
		class = tn->n_left->n_lval;
#endif

	p = typ = tymerge(tn, p);
#ifdef GCC_COMPAT
	if (p->n_op == CM) {
		if (a != NULL)
			p->n_right = trmerg(p->n_right, a);
		typ = typ->n_left;
	} else if (a != NULL)
		p = cmop(p, a);
#endif
		
	typ->n_sp = lookup((char *)typ->n_sp, 0); /* XXX */

	if (fun_inline && ISFTN(typ->n_type))
		typ->n_sp->sflags |= SINLINE;

	if (ISFTN(typ->n_type) == 0) {
		if (assign) {
			defid(p, class);
			typ->n_sp->sflags |= SASG;
			if (typ->n_sp->sflags & SDYNARRAY)
				uerror("can't initialize dynamic arrays");
			lcommdel(typ->n_sp);
		} else
			nidcl(p, class);
	} else {
		if (assign)
			uerror("cannot initialise function");
		defid(p, uclass(class));
	}
	tfree(p);
	return typ->n_sp;
}

/*
 * Declare function arguments.
 */
static void
funargs(NODE *p)
{
	if (p->n_op == ELLIPSIS)
		return;
	p->n_sp = lookup((char *)p->n_sp, 0);/* XXX */
	if (ISFTN(p->n_type))
		p->n_type = INCREF(p->n_type);
	defid(p, PARAM);
}

/*
 * Declare old-stype function arguments.
 */
static void
oldargs(NODE *p)
{
	p->n_op = TYPE;
	p->n_type = FARG;
	p->n_sp = lookup((char *)p->n_sp, 0);/* XXX */
	defid(p, PARAM);
}

/*
 * Declare a function.
 */
static void
fundef(NODE *tp, NODE *p)
{
	extern int prolab;
	struct symtab *s;
	NODE *q = p, *a = NULL, *typ;
	int class = tp->n_lval, oclass;
	char *c;

#ifdef GCC_COMPAT
	if (tp->n_op == CM)
		class = tp->n_left->n_lval;
#endif
	for (q = p; coptype(q->n_op) != LTYPE && q->n_left->n_op != NAME;
	    q = q->n_left)
		;
	if (q->n_op != CALL && q->n_op != UCALL) {
		uerror("invalid function definition");
		p = bdty(UCALL, p);
	}

	if (q->n_op == CALL && q->n_right->n_type != VOID) {
		/* declare function arguments */
		ftnarg(q);
	}

#ifdef PCC_DEBUG
	if (blevel)
		cerror("blevel != 0");
#endif

	p = typ = tymerge(tp, p);
#ifdef GCC_COMPAT
	if (p->n_op == CM) {
		if (a != NULL)
			p->n_right = trmerg(p->n_right, a);
		typ = typ->n_left;
	} else if (a != NULL)
		p = cmop(p, a);
#endif

	s = typ->n_sp = lookup((char *)typ->n_sp, 0); /* XXX */

	oclass = s->sclass;
	if (class == STATIC && oclass == EXTERN)
		werror("%s was first declared extern, then static", s->sname);

	if (fun_inline) {
		/* special syntax for inline functions */
		s->sflags |= SINLINE;
		inline_start(s);
		if (class == EXTERN)
			class = EXTDEF;
	} else if (class == EXTERN)
		class = SNULL; /* same result */

	cftnsp = s;
	defid(p, class);
	prolab = getlab();
	if ((c = cftnsp->soname) == NULL)
		c = addname(exname(cftnsp->sname));
	send_passt(IP_PROLOG, -1, c, cftnsp->stype,
	    cftnsp->sclass == EXTDEF, prolab, ctval);
	blevel++;
#ifdef STABS
	if (gflag)
		stabs_func(s);
#endif
	tfree(tp);
	tfree(p);

}

static void
fend(void)
{
	if (blevel)
		cerror("function level error");
	ftnend();
	fun_inline = 0;
	cftnsp = NULL;
}

NODE *
structref(NODE *p, int f, char *name)
{
	NODE *r;

	if (f == DOT)
		p = buildtree(ADDROF, p, NIL);
	r = biop(NAME, NIL, NIL);
	r->n_name = name;
	r = buildtree(STREF, p, r);
	return r;
}

static void
olddecl(NODE *p, NODE *a)
{
	struct symtab *s;

	s = lookup((char *)p->n_sp, 0);
	if (s->slevel != 1 || s->stype == UNDEF)
		uerror("parameter '%s' not defined", s->sname);
	else if (s->stype != FARG)
		uerror("parameter '%s' redefined", s->sname);
	s->stype = p->n_type;
	s->sdf = p->n_df;
	s->ssue = p->n_sue;
	nfree(p);
	if (a) {
		if (attrwarn)
			werror("unhandled olddecl attribute");
		tfree(a);
	}
}

void
branch(int lbl)
{
	int r = reached++;
	ecomp(biop(GOTO, bcon(lbl), NIL));
	reached = r;
}

/*
 * Create a printable string based on an encoded string.
 */
static char *
mkpstr(char *str)
{
	char *s, *os;
	int v, l = strlen(str)+3; /* \t + \n + \0 */

	os = s = inlalloc(l);
	*s++ = '\t';
	for (; *str; ) {
		if (*str++ == '\\')
			v = esccon(&str);
		else
			v = str[-1];
		*s++ = v;
	}
	*s++ = '\n';
	*s = 0;
	return os;
}

/*
 * Estimate the max length a string will have in its internal 
 * representation based on number of \ characters.
 */
static int
maxstlen(char *str)
{
	int i;

	for (i = 0; *str; str++, i++)
		if (*str == '\\' || *str < 32 || *str > 0176)
			i += 3;
	return i;
}

static char *
voct(char *d, unsigned int v)
{
	v &= (1 << SZCHAR) - 1;
	*d++ = '\\';
	*d++ = v/64 + '0'; v &= 077;
	*d++ = v/8 + '0'; v &= 7;
	*d++ = v + '0';
	return d;
}
	

/*
 * Convert a string to internal format.  The resulting string may be no
 * more than len characters long.
 */
static void
fixstr(char *d, char *s, int len)
{
	unsigned int v;

	while (*s) {
		if (len <= 0)
			cerror("fixstr");
		if (*s == '\\') {
			s++;
			v = esccon(&s);
			d = voct(d, v);
			len -= 4;
		} else if (*s < ' ' || *s > 0176) {
			d = voct(d, *s++);
			len -= 4;
		} else
			*d++ = *s++, len--;
	}
	*d = 0;
}

/*
 * Add "raw" string new to cleaned string old.
 */
static char *
stradd(char *old, char *new)
{
	char *rv;
	int len;

	if (*new == 'L' && new[1] == '\"')
		widestr = 1, new++;
	if (*new == '\"') {
		new++;			 /* remove first " */
		new[strlen(new) - 1] = 0;/* remove last " */
	}
	len = strlen(old) + maxstlen(new) + 1;
	rv = tmpalloc(len);
	strlcpy(rv, old, len);
	fixstr(rv + strlen(old), new, maxstlen(new) + 1);
	return rv;
}

/*
 * Fake a symtab entry for compound literals.
 */
static struct symtab *
clbrace(NODE *p)
{
	struct symtab *sp;

	sp = getsymtab(simname("cl"), STEMP);
	sp->stype = p->n_type;
	sp->squal = p->n_qual;
	sp->sdf = p->n_df;
	sp->ssue = p->n_sue;
	tfree(p);
	if (blevel == 0 && xnf != NULL) {
		sp->sclass = STATIC;
		sp->slevel = 2;
		sp->soffset = getlab();
	} else {
		sp->sclass = blevel ? AUTO : STATIC;
		if (!ISARY(sp->stype) || sp->sdf->ddim != NOOFFSET) {
			sp->soffset = NOOFFSET;
			oalloc(sp, &autooff);
		}
	}
	beginit(sp);
	return sp;
}

char *
simname(char *s)
{
	int len = strlen(s) + 10 + 1;
	char *w = tmpalloc(len);

	snprintf(w, len, "%s%d", s, getlab());
	return w;
}

NODE *
biop(int op, NODE *l, NODE *r)
{
	return block(op, l, r, INT, 0, MKSUE(INT));
}

static NODE *
cmop(NODE *l, NODE *r)
{
	return biop(CM, l, r);
}

static NODE *
voidcon(void)
{
	return block(ICON, NIL, NIL, STRTY, 0, MKSUE(VOID));
}

/* Support for extended assembler a' la' gcc style follows below */

static NODE *
xmrg(NODE *out, NODE *in)
{
	NODE *p = in;

	if (p->n_op == XARG) {
		in = cmop(out, p);
	} else {
		while (p->n_left->n_op == CM)
			p = p->n_left;
		p->n_left = cmop(out, p->n_left);
	}
	return in;
}

/*
 * Put together in and out node lists in one list, and balance it with
 * the constraints on the right side of a CM node.
 */
static NODE *
xcmop(NODE *out, NODE *in, NODE *str)
{
	NODE *p, *q;

	if (out) {
		/* D out-list sanity check */
		for (p = out; p->n_op == CM; p = p->n_left) {
			q = p->n_right;
			if (q->n_name[0] != '=' && q->n_name[0] != '+')
				uerror("output missing =");
		}
		if (p->n_name[0] != '=' && p->n_name[0] != '+')
			uerror("output missing =");
		if (in == NIL)
			p = out;
		else
			p = xmrg(out, in);
	} else if (in) {
		p = in;
	} else
		p = voidcon();

	if (str == NIL)
		str = voidcon();
	return cmop(p, str);
}

/*
 * Generate a XARG node based on a string and an expression.
 */
static NODE *
xasmop(char *str, NODE *p)
{

	p = biop(XARG, p, NIL);
	p->n_name = isinlining ? newstring(str, strlen(str)+1) : str;
	return p;
}

/*
 * Generate a XASM node based on a string and an expression.
 */
static void
mkxasm(char *str, NODE *p)
{
	NODE *q;

	q = biop(XASM, p->n_left, p->n_right);
	q->n_name = isinlining ? newstring(str, strlen(str)+1) : str;
	nfree(p);
	ecomp(q);
}

#ifdef GCC_COMPAT
static NODE *
tyof(NODE *p)
{
	static struct symtab spp;
	NODE *q = block(TYPE, NIL, NIL, p->n_type, p->n_df, p->n_sue);
	q->n_qual = p->n_qual;
	q->n_sp = &spp; /* for typenode */
	tfree(p);
	return q;
}
#endif

/*
 * Traverse an unhandled expression tree bottom-up and call buildtree()
 * or equivalent as needed.
 */
NODE *
eve(NODE *p)
{
	struct symtab *sp;
	NODE *r, *p1, *p2;
	int x;

	p1 = p->n_left;
	p2 = p->n_right;
	switch (p->n_op) {
	case NAME:
		sp = lookup((char *)p->n_sp, 0);
		if (sp->sflags & SINLINE)
			inline_ref(sp);
		r = nametree(sp);
		if (sp->sflags & SDYNARRAY)
			r = buildtree(UMUL, r, NIL);
		break;

	case DOT:
	case STREF:
		r = structref(eve(p1), p->n_op, (char *)p2->n_sp);
		nfree(p2);
		break;

	case CAST:
		p1 = buildtree(CAST, p1, eve(p2));
		nfree(p1->n_left);
		r = p1->n_right;
		nfree(p1);
		break;


	case SZOF:
		x = xinline; xinline = 0; /* XXX hack */
		if (p2->n_lval == 0)
			p1 = eve(p1);
		nfree(p2);
		r = doszof(p1);
		xinline = x;
		break;

	case LB:
		p1 = eve(p->n_left);
		r = buildtree(UMUL, buildtree(PLUS, p1, eve(p2)), NIL);
		break;

	case COMPL:
#ifndef NO_COMPLEX
		p1 = eve(p1);
		if (ANYCX(p1))
			r = cxconj(p1);
		else
			r = buildtree(COMPL, p1, NIL);
		break;
#endif
	case UMINUS:
	case NOT:
	case UMUL:
		r = buildtree(p->n_op, eve(p->n_left), NIL);
		break;

	case ADDROF:
		r = eve(p1);
		if (ISFTN(p->n_type)/* || ISARY(p->n_type) */){
#ifdef notdef
			werror( "& before array or function: ignored" );
#endif
		} else
			r = buildtree(ADDROF, r, NIL);
		break;

	case CALL:
		p2 = eve(p2);
		/* FALLTHROUGH */
	case UCALL:
		if (p1->n_op == NAME) {
			sp = lookup((char *)p1->n_sp, 0);
			if (sp->stype == UNDEF) {
				p1->n_type = FTN|INT;
				p1->n_sp = sp;
				defid(p1, EXTERN);
			}
			nfree(p1);
			r = doacall(sp, nametree(sp), p2);
		} else
			r = doacall(NULL, eve(p1), p2);
		break;

#ifndef NO_COMPLEX
	case XREAL:
	case XIMAG:
		p1 = eve(p1);
		r = cxelem(p->n_op, p1);
		break;
#endif

	case MUL:
	case DIV:
	case PLUS:
	case MINUS:
	case ASSIGN:
#ifndef NO_COMPLEX
		p1 = eve(p1);
		p2 = eve(p2);
		if (ANYCX(p1) || ANYCX(p2)) {
			r = cxop(p->n_op, p1, p2);
		} else if (ISITY(p1->n_type) || ISITY(p2->n_type)) {
			r = imop(p->n_op, p1, p2);
		} else
			r = buildtree(p->n_op, p1, p2);
		break;
#endif
	case MOD:
	case INCR:
	case DECR:
	case CM:
	case GT:
	case GE:
	case LT:
	case LE:
	case EQ:
	case NE:
	case RS:
	case LS:
	case RSEQ:
	case LSEQ:
	case AND:
	case OR:
	case ER:
	case OROR:
	case ANDAND:
	case EREQ:
	case OREQ:
	case ANDEQ:
	case MINUSEQ:
	case PLUSEQ:
	case MULEQ:
	case DIVEQ:
	case MODEQ:
	case QUEST:
	case COLON:
		p1 = eve(p1);
		r = buildtree(p->n_op, p1, eve(p2));
		break;

	case STRING:
		r = strend(p->n_lval, p->n_name);
		break;

	case COMOP:
		if (p1->n_op == GOTO) {
			/* inside ({ }), eve already called */
			r = buildtree(p->n_op, p1, p2);
		} else {
			p1 = eve(p1);
			r = buildtree(p->n_op, p1, eve(p2));
		}
		break;

	case TYPE:
	case ICON:
	case FCON:
		return p;

	case CLOP:
		r = nametree(p->n_sp);
		break;

	default:
#ifdef PCC_DEBUG
		fwalk(p, eprint, 0);
#endif
		cerror("eve");
	}
	nfree(p);
	return r;
}

void
bfix(int a)
{
        if (blevel == 1) {
                Wshadow = a;
        } else if (blevel == 2)
                argoff = a;
        blevel--;
}
