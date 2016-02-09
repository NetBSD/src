/*	Id: cgram.y,v 1.411 2016/01/10 18:08:13 ragge Exp 	*/	
/*	$NetBSD: cgram.y,v 1.3 2016/02/09 20:37:32 plunky Exp $	*/

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
 * At last count, there were 5 shift/reduce and no reduce/reduce conflicts
 * All are accounted for;
 * One is "dangling else"
 * Two is in attribute parsing
 * Two is in ({ }) parsing
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
%token	PCC_OFFSETOF
%token	GCC_DESIG

/* C11 keywords */
%token	C_STATICASSERT
%token	C_ALIGNAS
%token	C_ALIGNOF
%token	C_GENERIC
%token	C_ATOMIC

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
static int alwinl;
P1ND *cftnod;
static int attrwarn = 1;

#define	NORETYP	SNOCREAT /* no return type, save in unused field in symtab */

struct genlist {
	struct genlist *next;
	P1ND *p;
	TWORD t;
};

       P1ND *bdty(int op, ...);
static void fend(void);
static void fundef(P1ND *tp, P1ND *p);
static void olddecl(P1ND *p, P1ND *a);
static struct symtab *init_declarator(P1ND *tn, P1ND *p, int assign, P1ND *a,
	char *as);
static void resetbc(int mask);
static void swend(void);
static void addcase(P1ND *p);
#ifdef GCC_COMPAT
static void gcccase(P1ND *p, P1ND *);
#endif
static struct attr *gcc_attr_wrapper(P1ND *p);
static void adddef(void);
static void savebc(void);
static void swstart(int, TWORD);
static void genswitch(int, TWORD, struct swents **, int);
static char *mkpstr(char *str);
static struct symtab *clbrace(P1ND *);
static P1ND *cmop(P1ND *l, P1ND *r);
static P1ND *xcmop(P1ND *out, P1ND *in, P1ND *str);
static void mkxasm(char *str, P1ND *p);
static P1ND *xasmop(char *str, P1ND *p);
static P1ND *biop(int op, P1ND *l, P1ND *r);
static void flend(void);
static P1ND *gccexpr(int bn, P1ND *q);
static char * simname(char *s);
static P1ND *tyof(P1ND *);	/* COMPAT_GCC */
static P1ND *voidcon(void);
static P1ND *funargs(P1ND *p);
static void oldargs(P1ND *p);
static void uawarn(P1ND *p, char *s);
static int con_e(P1ND *p);
static void dainit(P1ND *d, P1ND *a);
static P1ND *tymfix(P1ND *p);
static P1ND *namekill(P1ND *p, int clr);
static P1ND *aryfix(P1ND *p);
static P1ND *dogen(struct genlist *g, P1ND *e);
static struct genlist *newgen(P1ND *p, P1ND *q);
static struct genlist *addgen(struct genlist *g, struct genlist *h);

static void savlab(int);
static void xcbranch(P1ND *, int);
extern int *mkclabs(void);

#define	TYMFIX(inp) { \
	P1ND *pp = inp; \
	inp = tymerge(pp->n_left, pp->n_right); \
	p1nfree(pp->n_left); p1nfree(pp); }

struct xalloc;
extern struct xalloc *bkpole, *sapole;
extern int cbkp, cstp;
extern int usdnodes;
struct bks {
	struct xalloc *ptr;
	int off;
};

/*
 * State for saving current switch state (when nested switches).
 */
struct savbc {
	struct savbc *next;
	int brklab;
	int contlab;
	int flostat;
	int swx;
	struct xalloc *bkptr;
	int bkoff;
	struct xalloc *stptr;
	int stoff;
	int numnode;
} *savbc, *savctx;

%}

%union {
	TWORD type;
	int intval;
	P1ND *nodep;
	struct symtab *symp;
	struct rstack *rp;
	char *strp;
	struct bks *bkp;
	union flt *flt;
	struct genlist *g;
}

	/* define types */
%start ext_def_list

%type <intval> ifelprefix ifprefix whprefix forprefix doprefix switchpart
		xbegin
%type <nodep> e .e term enum_dcl struct_dcl cast_type declarator
		elist type_sq cf_spec merge_attribs e2 ecq
		parameter_declaration abstract_declarator initializer
		parameter_type_list parameter_list
		declaration_specifiers designation
		specifier_qualifier_list merge_specifiers
		identifier_list arg_param_list type_qualifier_list
		designator_list designator xasm oplist oper cnstr funtype
		typeof attribute attribute_specifier /* COMPAT_GCC */
		attribute_list attr_spec_list attr_var /* COMPAT_GCC */

%type <g>	gen_ass_list gen_assoc
%type <strp>	string C_STRING GCC_DESIG svstr
%type <rp>	str_head
%type <symp>	xnfdeclarator clbrace enum_head

%type <intval>  C_STRUCT C_RELOP C_DIVOP C_SHIFTOP
		C_ANDAND C_OROR C_STROP C_INCOP C_UNOP C_ASOP C_EQUOP

%type <type>	C_TYPE C_QUALIFIER C_CLASS C_FUNSPEC

%type <nodep>   C_ICON

%type <flt>	C_FCON 

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
		    fundef(mkty(INT, 0, 0), $1);
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

type_sq:	   C_TYPE { $$ = mkty($1, 0, 0); }
		|  C_TYPENAME { 
			struct symtab *sp = lookup($1, 0);
			if (sp->stype == ENUMTY) {
				sp->stype = strmemb(sp->sap)->stype;
			}
			$$ = mkty(sp->stype, sp->sdf, sp->sap);
			$$->n_sp = sp;
		}
		|  struct_dcl { $$ = $1; }
		|  enum_dcl { $$ = $1; }
		|  C_QUALIFIER { $$ = block(QUALIFIER, NULL, NULL, 0, 0, 0); $$->n_qual = $1; }
		|  attribute_specifier { $$ = biop(ATTRIB, $1, 0); }
		|  C_ALIGNAS '(' e ')' { 
			$$ = biop(ALIGN, NULL, NULL);
			slval($$, con_e($3));
		}
		|  C_ALIGNAS '(' cast_type ')' {
			TYMFIX($3);
			$$ = biop(ALIGN, NULL, NULL);
			slval($$, talign($3->n_type, $3->n_ap)/SZCHAR);
			p1tfree($3);
		}
		|  C_ATOMIC { uerror("_Atomic not supported"); $$ = bcon(0); }
		|  C_ATOMIC '(' cast_type ')' {
			uerror("_Atomic not supported"); $$ = $3;
		}
		|  typeof { $$ = $1; }
		;

cf_spec:	   C_CLASS { $$ = block(CLASS, NULL, NULL, $1, 0, 0); }
		|  C_FUNSPEC { $$ = block(FUNSPEC, NULL, NULL, $1, 0, 0); }
		;

typeof:		   C_TYPEOF '(' e ')' { $$ = tyof(eve($3)); }
		|  C_TYPEOF '(' cast_type ')' { TYMFIX($3); $$ = tyof($3); }
		;

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
			$$ = bdty($3 == NULL ? UCALL : CALL, bdty(NAME, $1), $3);
		}
		;

/*
 * Adds a pointer list to front of the declarators.
 */
declarator:	   '*' declarator { $$ = bdty(UMUL, $2); }
		|  '*' type_qualifier_list declarator {
			$$ = $2;
			$$->n_left = $3;
		}
		|  C_NAME { $$ = bdty(NAME, $1); }
		|  '(' attr_spec_list declarator ')' {
			$$ = $3;
			$$->n_ap = attr_add($$->n_ap, gcc_attr_wrapper($2));
		}
		|  '(' declarator ')' { $$ = $2; }
		|  declarator '[' ecq ']' { $$ = biop(LB, $1, $3); }
		|  declarator '(' parameter_type_list ')' {
			$$ = bdty(CALL, $1, $3);
		}
		|  declarator '(' identifier_list ')' {
			$$ = bdty(CALL, $1, $3);
			oldstyle = 1;
		}
		|  declarator '(' ')' { $$ = bdty(UCALL, $1); }
		;

ecq:		   maybe_r { $$ = bcon(NOOFFSET); }
		|  e  { $$ = $1; }
		|  r e { $$ = $2; }
		|  c maybe_r e { $$ = $3; }
		|  r c e { $$ = $3; }
		|  '*' { $$ = bcon(NOOFFSET); }
		|  r '*' { $$ = bcon(NOOFFSET); }
		;

r:		  C_QUALIFIER {
			if ($1 != 0)
				uerror("bad qualifier");
		}
		;

c:		  C_CLASS {
			if ($1 != STATIC)
				uerror("bad class keyword");
		}
		;

type_qualifier_list:
		   C_QUALIFIER { $$ = biop(UMUL, 0, 0); $$->n_qual = $1; }
		|  type_qualifier_list C_QUALIFIER {
			$$ = $1;
			$$->n_qual |= $2;
		}
		|  attribute_specifier {
			$$ = block(UMUL, NULL, NULL, 0, 0, gcc_attr_wrapper($1));
		}
		|  type_qualifier_list attribute_specifier {
			$1->n_ap = attr_add($1->n_ap, gcc_attr_wrapper($2));
		}
		;

identifier_list:   C_NAME { $$ = bdty(NAME, $1); oldargs($$); }
		|  identifier_list ',' C_NAME {
			$$ = cmop($1, bdty(NAME, $3));
			oldargs($$->n_right);
		}
		;

/*
 * Returns as parameter_list, but can add an additional ELLIPSIS node.
 */
parameter_type_list:
		   parameter_list { $$ = $1; }
		|  parameter_list ',' C_ELLIPSIS {
			$$ = cmop($1, biop(ELLIPSIS, NULL, NULL));
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
			if (glval($1) != SNULL && glval($1) != REGISTER)
				uerror("illegal parameter class");
			$$ = block(TYMERGE, $1, $2, INT, 0,
			    gcc_attr_wrapper($3));
		}
		|  declaration_specifiers abstract_declarator { 
			$1->n_ap = attr_add($1->n_ap, $2->n_ap);
			$$ = block(TYMERGE, $1, $2, INT, 0, 0);
		}
		|  declaration_specifiers {
			$$ = block(TYMERGE, $1, bdty(NAME, NULL), INT, 0, 0);
		}
		;

abstract_declarator:
		   '*' { $$ = bdty(UMUL, bdty(NAME, NULL)); }
		|  '*' type_qualifier_list {
			$$ = $2;
			$$->n_left = bdty(NAME, NULL);
		}
		|  '*' abstract_declarator { $$ = bdty(UMUL, $2); }
		|  '*' type_qualifier_list abstract_declarator {
			$$ = $2;
			$$->n_left = $3;
		}
		|  '(' abstract_declarator ')' { $$ = $2; }
		|  '[' maybe_r ']' attr_var {
			$$ = block(LB, bdty(NAME, NULL), bcon(NOOFFSET),
			    INT, 0, gcc_attr_wrapper($4));
		}
		|  '[' e ']' attr_var {
			$$ = block(LB, bdty(NAME, NULL), $2,
			    INT, 0, gcc_attr_wrapper($4));
		}
		|  abstract_declarator '[' maybe_r ']' attr_var {
			$$ = block(LB, $1, bcon(NOOFFSET),
			    INT, 0, gcc_attr_wrapper($5));
		}
		|  abstract_declarator '[' e ']' attr_var {
			$$ = block(LB, $1, $3, INT, 0, gcc_attr_wrapper($5));
		}
		|  '(' ')' attr_var {
			$$ = bdty(UCALL, bdty(NAME, NULL));
			$$->n_ap = gcc_attr_wrapper($3);
		}
		|  '(' ib2 parameter_type_list ')' attr_var {
			$$ = block(CALL, bdty(NAME, NULL), $3, INT, 0,
			    gcc_attr_wrapper($5));
		}
		|  abstract_declarator '(' ')' attr_var {
			$$ = block(UCALL, $1, NULL, INT, 0, gcc_attr_wrapper($4));
		}
		|  abstract_declarator '(' ib2 parameter_type_list ')' attr_var {
			$$ = block(CALL, $1, $4, INT, 0, gcc_attr_wrapper($6));
		}
		;

ib2:		   { }
		;

maybe_r:	   { }
		|  C_QUALIFIER { }
		;

/*
 * K&R arg declaration, between ) and {
 */
arg_dcl_list:	   arg_declaration
		|  arg_dcl_list arg_declaration
		;


arg_declaration:   declaration_specifiers arg_param_list ';' {
			p1nfree($1);
		}
		;

arg_param_list:	   declarator attr_var {
			olddecl(block(TYMERGE, p1tcopy($<nodep>0), $1,
			    INT, 0, 0), $2);
		}
		|  arg_param_list ',' declarator attr_var {
			olddecl(block(TYMERGE, p1tcopy($<nodep>0), $3,
			    INT, 0, 0), $4);
		}
		;

/*
 * Declarations in beginning of blocks.
 */
block_item_list:   block_item
		|  block_item_list block_item
		;

block_item:	   declaration
		|  statement { stmtfree(); }
		;

/*
 * Here starts the old YACC code.
 */

/*
 * Variables are declared in init_declarator.
 */
declaration:	   declaration_specifiers ';' { p1tfree($1); fun_inline = 0; }
		|  declaration_specifiers init_declarator_list ';' {
			p1tfree($1);
			fun_inline = 0;
		}
		|  C_STATICASSERT '(' e ',' string ')' ';' {
			int r = con_e($3);
			if (r == 0) /* false */
				uerror($5);
		}
		;

/*
 * Normal declaration of variables. curtype contains the current type node.
 * Returns nothing, variables are declared in init_declarator.
 */
init_declarator_list:
		   init_declarator { symclear(blevel); }
		|  init_declarator_list ',' attr_var { $<nodep>$ = $<nodep>0; } init_declarator {
			uawarn($3, "init_declarator");
			symclear(blevel);
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
		|  C_NAME '=' e { enummer = con_e($3); moedef($1); }
		|  C_TYPENAME '=' e { enummer = con_e($3); moedef($1); }
		;

struct_dcl:	   str_head '{' struct_dcl_list '}' {
			P1ND *p;

			$$ = dclstruct($1);
			if (pragma_allpacked) {
				p = bdty(CALL, bdty(NAME, "packed"),
				    bcon(pragma_allpacked));
				$$->n_ap = attr_add($$->n_ap,gcc_attr_wrapper(p)); }
		}
		|  C_STRUCT attr_var C_NAME { 
			$$ = rstruct($3,$1);
			uawarn($2, "struct_dcl");
		}
 /*COMPAT_GCC*/	|  str_head '{' '}' { $$ = dclstruct($1); }
		;

attr_var:	   {	
			P1ND *q, *p;

			p = pragma_aligned ? bdty(CALL, bdty(NAME, "aligned"),
			    bcon(pragma_aligned)) : NULL;
			if (pragma_packed) {
				q = bdty(NAME, "packed");
				p = (p == NULL ? q : cmop(p, q));
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
		|  C_STRUCT attr_var C_NAME {  $$ = bstruct($3, $1, $2);  }
		;

struct_dcl_list:   struct_declaration
		|  struct_dcl_list struct_declaration
		;

struct_declaration:
		   specifier_qualifier_list struct_declarator_list optsemi {
			p1tfree($1);
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
		   struct_declarator { symclear(blevel); }
		|  struct_declarator_list ',' { $<nodep>$=$<nodep>0; } 
			struct_declarator { symclear(blevel); }
		;

struct_declarator: declarator attr_var {
			P1ND *p;

			$1 = aryfix($1);
			p = tymerge($<nodep>0, tymfix($1));
			if ($2)
				p->n_ap = attr_add(p->n_ap, gcc_attr_wrapper($2));
			soumemb(p, (char *)$1->n_sp, 0);
			p1tfree(p);
		}
		|  ':' e {
			int ie = con_e($2);
			if (fldchk(ie))
				ie = 1;
			falloc(NULL, ie, $<nodep>0);
		}
		|  declarator ':' e {
			int ie = con_e($3);
			if (fldchk(ie))
				ie = 1;
			if ($1->n_op == NAME) {
				/* XXX - tymfix() may alter $1 */
				tymerge($<nodep>0, tymfix($1));
				soumemb($1, (char *)$1->n_sp, FIELD | ie);
				p1nfree($1);
			} else
				uerror("illegal declarator");
		}
		|  declarator ':' e attr_spec_list {
			int ie = con_e($3);
			if (fldchk(ie))
				ie = 1;
			if ($1->n_op == NAME) {
				/* XXX - tymfix() may alter $1 */
				tymerge($<nodep>0, tymfix($1));
				if ($4)
					$1->n_ap = attr_add($1->n_ap,
					    gcc_attr_wrapper($4));
				soumemb($1, (char *)$1->n_sp, FIELD | ie);
				p1nfree($1);
			} else
				uerror("illegal declarator");
		}
		| /* unnamed member */ {
			P1ND *p = $<nodep>0;
			char *c = permalloc(10);

			if (p->n_type != STRTY && p->n_type != UNIONTY)
				uerror("bad unnamed member type");
			snprintf(c, 10, "*%dFAKE", getlab());
			soumemb(p, c, 0);
		}
		;

		/* always preceeded by attributes */
xnfdeclarator:	   declarator attr_var {
			$$ = xnf = init_declarator($<nodep>0, $1, 1, $2, 0);
		}
		|  declarator C_ASM '(' svstr ')' {
			$$ = xnf = init_declarator($<nodep>0, $1, 1, NULL, $4);
		}
		;

/*
 * Handles declarations and assignments.
 * Returns nothing.
 */
init_declarator:   declarator attr_var {
			init_declarator($<nodep>0, $1, 0, $2, 0);
		}
		|  declarator C_ASM '(' svstr ')' attr_var {
			init_declarator($<nodep>0, $1, 0, $6, $4);
		}
		|  xnfdeclarator '=' e { 
			if ($1->sclass == STATIC || $1->sclass == EXTDEF)
				statinit++;
			simpleinit($1, eve($3));
			if ($1->sclass == STATIC || $1->sclass == EXTDEF)
				statinit--;
			xnf = NULL;
		}
		|  xnfdeclarator '=' begbr init_list optcomma '}' {
			endinit(0);
			xnf = NULL;
		}
 /*COMPAT_GCC*/	|  xnfdeclarator '=' begbr '}' { endinit(0); xnf = NULL; }
		;

begbr:		   '{' { beginit($<symp>-1); }
		;

initializer:	   e %prec ',' {  $$ = eve($1); }
		|  ibrace init_list optcomma '}' { $$ = NULL; }
		|  ibrace '}' { asginit(bcon(0)); $$ = NULL; }
		;

init_list:	   designation initializer { dainit($1, $2); }
		|  init_list ','  designation initializer { dainit($3, $4); }
		;

designation:	   designator_list '=' { desinit($1); $$ = NULL; }
		|  GCC_DESIG { desinit(bdty(NAME, $1)); $$ = NULL; }
		|  '[' e C_ELLIPSIS e ']' '=' { $$ = biop(CM, $2, $4); }
		|  { $$ = NULL; }
		;

designator_list:   designator { $$ = $1; }
		|  designator_list designator { $$ = $2; $$->n_left = $1; }
		;

designator:	   '[' e ']' {
			int ie = con_e($2);
			if (ie < 0) {
				uerror("designator must be non-negative");
				ie = 0;
			}
			$$ = biop(LB, NULL, bcon(ie));
		}
		|  C_STROP C_TYPENAME {
			if ($1 != DOT)
				uerror("invalid designator");
			$$ = bdty(NAME, $2);
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
			struct savbc *bc = malloc(sizeof(struct savbc));
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
			bc->bkptr = bkpole;
			bc->bkoff = cbkp;
			bc->stptr = sapole;
			bc->stoff = cstp;
			bc->numnode = usdnodes;
			usdnodes = 0;
			bkpole = sapole = NULL;
			cbkp = cstp = 0;
			savctx = bc;
			if (!isinlining && sspflag && blevel == 2)
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
				cbranch(buildtree(NE, eve($5), bcon(0)),
				    bcon($1));
			else
				p1tfree(eve($5));
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
			    blevel--;
			    symclear(blevel);
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
			if (!reached)
				warner(Wunreachable_code);
			reached = 0;
		}
		|  C_RETURN e  ';' {
			P1ND *p, *q;

			p = nametree(cftnsp);
			p->n_type = DECREF(p->n_type);
			q = eve($2);
#ifdef TARGET_TIMODE  
			P1ND *r;
			if ((r = gcc_eval_ticast(RETURN, p, q)) != NULL)
				q = r;
#endif
#ifndef NO_COMPLEX
			if (ANYCX(q) || ANYCX(p))
				q = cxret(q, p);
			else if (ISITY(p->n_type) || ISITY(q->n_type)) {
				q = imret(q, p);
				if (ISITY(p->n_type))
					p->n_type -= (FIMAG-FLOAT);
				if (ISITY(q->n_type))
					q->n_type -= (FIMAG-FLOAT);
			}
#endif
			p = buildtree(RETURN, p, q);
			if (p->n_type == VOID) {
				ecomp(p->n_right);
			} else {
				if (cftnod == NULL) {
					P1ND *r = tempnode(0, p->n_type,
					    p->n_df, p->n_ap);
					cftnod = tmpalloc(sizeof(P1ND));
					*cftnod = *r;
					p1tfree(r);
				}
				ecomp(buildtree(ASSIGN,
				    p1tcopy(cftnod), p->n_right));
			}
			p1tfree(p->n_left);
			p1nfree(p);
			branch(retlab);
			reached = 0;
		}
		|  C_GOTO C_NAME ';' { gotolabel($2); goto rch; }
		|  C_GOTO '*' e ';' { ecomp(biop(GOTO, eve($3), NULL)); }
		|  asmstatement ';'
		|   ';'
		|  error  ';'
		|  error '}'
		|  label statement
		;

asmstatement:	   C_ASM mvol '(' svstr ')' { send_passt(IP_ASM, mkpstr($4)); }
		|  C_ASM mvol '(' svstr xasm ')' { mkxasm($4, $5); }
		;

svstr:		  string { $$ = addstring($1); }
		;

mvol:		   /* empty */
		|  C_QUALIFIER { }
		;

xasm:		   ':' oplist { $$ = xcmop($2, NULL, NULL); }
		|  ':' oplist ':' oplist { $$ = xcmop($2, $4, NULL); }
		|  ':' oplist ':' oplist ':' cnstr { $$ = xcmop($2, $4, $6); }
		;

oplist:		   /* nothing */ { $$ = NULL; }
		|  oper { $$ = $1; }
		;

oper:		   svstr '(' e ')' { $$ = xasmop($1, pconvert(eve($3))); }
		|  oper ',' svstr '(' e ')' {
			$$ = cmop($1, xasmop($3, pconvert(eve($5))));
		}
		;

cnstr:		   svstr { $$ = xasmop($1, bcon(0)); }
		|  cnstr ',' svstr { $$ = cmop($1, xasmop($3, bcon(0))); }
                ;

label:		   C_NAME ':' attr_var { deflabel($1, $3); reached = 1; }
		|  C_TYPENAME ':' attr_var { deflabel($1, $3); reached = 1; }
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
			xcbranch(eve($3), $$ = getlab());
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
			if ($3->n_op == ICON && glval($3) != 0)
				flostat = FLOOP;
			plabel( contlab = getlab());
			reached = 1;
			brklab = getlab();
			if (flostat == FLOOP)
				p1tfree($3);
			else
				xcbranch($3, brklab);
		}
		;
forprefix:	  C_FOR  '('  .e  ';' .e  ';' {
			++blevel;
			if ($3)
				ecomp($3);
			savebc();
			contlab = getlab();
			brklab = getlab();
			plabel( $$ = getlab());
			reached = 1;
			if ($5)
				xcbranch($5, brklab);
			else
				flostat |= FLOOP;
		}
		|  C_FOR '(' { ++blevel; } declaration .e ';' {
			savebc();
			contlab = getlab();
			brklab = getlab();
			plabel( $$ = getlab());
			reached = 1;
			if ($5)
				xcbranch($5, brklab);
			else
				flostat |= FLOOP;
		}
		;

switchpart:	   C_SWITCH  '('  e ')' {
			P1ND *p;
			int num;
			TWORD t;

			savebc();
			brklab = getlab();
			$3 = eve($3);
			if (!ISINTEGER($3->n_type)) {
				uerror("switch expression must have integer "
				       "type");
				t = INT;
			} else {
				$3 = intprom($3);
				t = $3->n_type;
			}
			p = tempnode(0, t, 0, 0);
			num = regno(p);
			ecomp(buildtree(ASSIGN, p, $3));
			branch( $$ = getlab());
			swstart(num, t);
			reached = 0;
		}
		;
/*	EXPRESSIONS	*/
.e:		   e { $$ = eve($1); }
		| 	{ $$=0; }
		;

elist:		   { $$ = NULL; }
		|  e2 { $$ = $1; }
		;

e2:		   e %prec ','
		|  e2  ','  e { $$ = biop(CM, $1, $3); }
		|  e2  ','  cast_type { /* hack for stdarg */
			TYMFIX($3);
			$3->n_op = TYPE;
			$$ = biop(CM, $1, $3);
		}
		|  cast_type { TYMFIX($1); $1->n_op = TYPE; $$ = $1; }
		;

/*
 * Precedence order of operators.
 */
e:		   e ',' e { $$ = biop(COMOP, $1, $3); }
		|  e '=' e {  $$ = biop(ASSIGN, $1, $3); }
		|  e C_ASOP e {  $$ = biop($2, $1, $3); }
		|  e '?' e ':' e { $$=biop(QUEST, $1, biop(COLON, $3, $5)); }
/* COMPAT_GCC */|  e '?' ':' e { $$ = biop(BIQUEST, $1, $4); }
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
		|  term
		;

xbegin:		   begin {
			$$ = getlab(); getlab(); getlab();
			branch($$); plabel(($$)+2);
		}
		;

term:		   term C_INCOP {  $$ = biop($2, $1, bcon(1)); }
		|  '*' term { $$ = biop(UMUL, $2, NULL); }
		|  '&' term { $$ = biop(ADDROF, $2, NULL); }
		|  '-' term { $$ = biop(UMINUS, $2, NULL ); }
		|  '+' term { $$ = biop(UPLUS, $2, NULL ); }
		|  C_UNOP term { $$ = biop($1, $2, NULL); }
		|  C_INCOP term {
			$$ = biop($1 == INCR ? PLUSEQ : MINUSEQ, $2, bcon(1));
		}
		|  C_SIZEOF xa term { $$ = biop(SZOF, $3, bcon(0)); inattr = $<intval>2; }
		|  '(' cast_type ')' term  %prec C_INCOP {
			TYMFIX($2);
			$$ = biop(CAST, $2, $4);
		}
		|  C_SIZEOF xa '(' cast_type ')'  %prec C_SIZEOF {
			$$ = biop(SZOF, $4, bcon(1));
			inattr = $<intval>2;
		}
		|  C_ALIGNOF xa '(' cast_type ')' {
			int al;
			TYMFIX($4);
			al = talign($4->n_type, $4->n_ap);
			$$ = bcon(al/SZCHAR);
			inattr = $<intval>2;
			p1tfree($4);
		}
		| '(' cast_type ')' clbrace init_list optcomma '}' {
			endinit(0);
			$$ = bdty(NAME, $4);
			$$->n_op = CLOP;
		}
		| '(' cast_type ')' clbrace '}' {
			endinit(0);
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
		|  PCC_OFFSETOF  '(' cast_type ',' term ')' {
			TYMFIX($3);
			$3->n_type = INCREF($3->n_type);
			$3 = biop(CAST, $3, bcon(0));
			if ($5->n_op == NAME) {
				$$ = biop(STREF, $3, $5);
			} else {
				P1ND *p = $5;
				while (p->n_left->n_op != NAME)
					p = p->n_left;
				p->n_left = biop(STREF, $3, p->n_left);
				$$ = $5;
			}
			$$ = biop(ADDROF, $$, NULL);
			$3 = block(NAME, NULL, NULL, ENUNSIGN(INTPTR), 0, 0);
			$$ = biop(CAST, $3, $$);
		}
		|  C_ICON { $$ = $1; }
		|  C_FCON { $$ = bdty(FCON, $1); }
		|  svstr { $$ = bdty(STRING, $1, styp()); }
		|  '(' e ')' { $$=$2; }
		|  '(' xbegin e ';' '}' ')' { $$ = gccexpr($2, eve($3)); }
		|  '(' xbegin block_item_list e ';' '}' ')' {
			$$ = gccexpr($2, eve($4));
		}
		|  '(' xbegin block_item_list '}' ')' { 
			$$ = gccexpr($2, voidcon());
		}
		| C_ANDAND C_NAME {
			struct symtab *s = lookup($2, SLBLNAME|STEMP);
			if (s->soffset == 0) {
				s->soffset = -getlab();
				s->sclass = STATIC;
			}
			savlab(s->soffset);
			$$ = biop(ADDROF, bdty(GOTO, $2), NULL);
		}
		| C_GENERIC '(' e ',' gen_ass_list ')' { $$ = dogen($5, $3); }
		;

gen_ass_list:	  gen_assoc { $$ = $1; }
		| gen_ass_list ',' gen_assoc { $$ = addgen($1, $3); }
		;

gen_assoc:	  cast_type ':' e { TYMFIX($1); $$ = newgen($1, $3); }
		| C_DEFAULT ':' e { $$ = newgen(0, $3); }
		;

xa:		  { $<intval>$ = inattr; inattr = 0; }
		;

clbrace:	   '{'	{ P1ND *q = $<nodep>-1; TYMFIX(q); $$ = clbrace(q); }
		;

string:		   C_STRING { $$ = stradd(NULL, $1); }
		|  string C_STRING { $$ = stradd($1, $2); }
		;

cast_type:	   specifier_qualifier_list {
			$$ = biop(TYMERGE, $1, bdty(NAME, NULL));
		}
		|  specifier_qualifier_list abstract_declarator {
			$$ = biop(TYMERGE, $1, aryfix($2));
		}
		;

%%

P1ND *
mkty(TWORD t, union dimfun *d, struct attr *sue)
{
	return block(TYPE, NULL, NULL, t, d, sue);
}

P1ND *
bdty(int op, ...)
{
	CONSZ c;
	va_list ap;
	int val;
	register P1ND *q;

	va_start(ap, op);
	q = biop(op, NULL, NULL);

	switch (op) {
	case UMUL:
	case UCALL:
		q->n_left = va_arg(ap, P1ND *);
		q->n_rval = 0;
		break;

	case FCON:
		q->n_dcon = va_arg(ap, union flt *);
		q->n_type = q->n_dcon->fa[FP_TOP];
		break;

	case CALL:
		q->n_left = va_arg(ap, P1ND *);
		q->n_right = va_arg(ap, P1ND *);
		break;

	case LB:
		q->n_left = va_arg(ap, P1ND *);
		if ((val = va_arg(ap, int)) <= 0) {
			uerror("array size must be positive");
			val = 1;
		}
		q->n_right = bcon(val);
		break;

	case GOTO: /* for named labels */
		q->n_ap = attr_add(q->n_ap, attr_new(ATTR_P1LABELS, 1));
		/* FALLTHROUGH */
	case NAME:
		q->n_op = NAME;
		q->n_sp = va_arg(ap, struct symtab *); /* XXX survive tymerge */
		break;

	case STRING:
		q->n_type = PTR|CHAR;
		q->n_name = va_arg(ap, char *);
		c = va_arg(ap, TWORD);
		slval(q, c);
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
	struct savbc *sc;

	if (!isinlining && sspflag && blevel == 2)
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
	blkfree();
	stmtfree();
	bkpole = savctx->bkptr;
	cbkp = savctx->bkoff;
	sapole = savctx->stptr;
	cstp = savctx->stoff;
	usdnodes = savctx->numnode;
	sc = savctx->next;
	free(savctx);
	savctx = sc;
}

/*
 * XXX workaround routines for block level cleansing in gcc compat mode.
 * Temporary should be re reserved for this value before.
 */
static P1ND *
p1mcopy(P1ND *p)
{
	P1ND *q;

	q = xmalloc(sizeof(P1ND));
	*q = *p;

	switch (coptype(q->n_op)) {
	case BITYPE:
		q->n_right = p1mcopy(p->n_right);
		/* FALLTHROUGH */
	case UTYPE: 
		q->n_left = p1mcopy(p->n_left);
	}

	return(q);
}

static void
p1mfree(P1ND *p)
{
	int o = coptype(p->n_op);
	if (o == BITYPE)
		p1mfree(p->n_right);
	if (o != LTYPE)
		p1mfree(p->n_left);
	free(p);
}


static P1ND *
gccexpr(int bn, P1ND *q)
{
	P1ND *r, *p, *s;

	branch(bn+4);
	plabel(bn);
	r = buildtree(COMOP, biop(GOTO, bcon(bn+2), NULL), q);
	/* XXX hack to survive flend() */
	s = p1mcopy(r);
	p1tfree(r);
	flend();
	r = p1tcopy(s);
	p1mfree(s);
	q = r->n_right;
	/* XXX end hack */
	if (q->n_op != ICON && q->n_type != STRTY) {
		p = tempnode(0, q->n_type, q->n_df, q->n_ap);
		r = buildtree(ASSIGN, p1tcopy(p), r);
		r = buildtree(COMOP, r, p);
	}
	return r;
}

static void
savebc(void)
{
	struct savbc *bc = malloc(sizeof(struct savbc));

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
	struct savbc *bc;

	flostat = savbc->flostat | (flostat&mask);
	contlab = savbc->contlab;
	brklab = savbc->brklab;
	bc = savbc->next;
	free(savbc);
	savbc = bc;
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
addcase(P1ND *p)
{
	struct swents **put, *w, *sw = malloc(sizeof(struct swents));
	CONSZ val;

	p = optloop(p);  /* change enum to ints */
	if (p->n_op != ICON || p->n_sp != NULL) {
		uerror( "non-constant case expression");
		return;
	}
	if (swpole == NULL) {
		uerror("case not in switch");
		return;
	}

	if (DEUNSIGN(swpole->type) != DEUNSIGN(p->n_type)) {
		val = glval(p);
		p = makety(p, swpole->type, 0, 0, 0);
		if (p->n_op != ICON)
			cerror("could not cast case value to type of switch "
			       "expression");
		if (glval(p) != val)
			werror("case expression truncated");
	}
	sw->sval = glval(p);
	p1tfree(p);
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
gcccase(P1ND *ln, P1ND *hn)
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
	struct swdef *sw = malloc(sizeof(struct swdef));

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
	struct swdef *sp;
	int i;

	sw = FUNALLO(sizeof(struct swents));
	swp = FUNALLO(sizeof(struct swents *) * (swpole->nents+1));

	sw->slab = swpole->deflbl;
	swp[0] = sw;

	for (i = 1; i <= swpole->nents; i++) {
		swp[i] = swpole->ents;
		swpole->ents = swpole->ents->next;
	}
	genswitch(swpole->num, swpole->type, swp, swpole->nents);

	FUNFREE(sw);
	FUNFREE(swp);
	while (swpole->ents) {
		sw = swpole->ents;
		swpole->ents = sw->next;
		free(sw);
	}
	sp = swpole->next;
	free(swpole);
	swpole = sp;
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
	P1ND *r, *q;
	int i;

	if (mygenswitch(num, type, p, n))
		return;

	/* simple switch code */
	for (i = 1; i <= n; ++i) {
		/* already in 1 */
		r = tempnode(num, type, 0, 0);
		q = xbcon(p[i]->sval, NULL, type);
		r = buildtree(NE, r, clocal(q));
		xcbranch(r, p[i]->slab);
	}
	if (p[0]->slab > 0)
		branch(p[0]->slab);
}

/*
 * Declare a variable or prototype.
 */
static struct symtab *
init_declarator(P1ND *tn, P1ND *p, int assign, P1ND *a, char *as)
{
	int class = glval(tn);
	struct symtab *sp;

	p = aryfix(p);
	p = tymerge(tn, p);
	if (a) {
		struct attr *ap = gcc_attr_wrapper(a);
		p->n_ap = attr_add(p->n_ap, ap);
	}

	p->n_sp = sp = lookup((char *)p->n_sp, 0); /* XXX */

	if (fun_inline && ISFTN(p->n_type))
		sp->sflags |= SINLINE;

	if (!ISFTN(p->n_type)) {
		if (assign) {
			defid2(p, class, as);
			sp = p->n_sp;
			sp->sflags |= SASG;
			if (sp->sflags & SDYNARRAY)
				uerror("can't initialize dynamic arrays");
			lcommdel(sp);
		} else
			nidcl2(p, class, as);
	} else {
		extern P1ND *parlink;
		if (assign)
			uerror("cannot initialise function");
		defid2(p, uclass(class), as);
		sp = p->n_sp;
		if (sp->sdf->dfun == 0 && !issyshdr)
			warner(Wstrict_prototypes);
		if (parlink) {
			/* dynamic sized arrays in prototypes */
			p1tfree(parlink); /* Free delayed tree */
			parlink = NULL;
		}
	}
	p1tfree(p);
	if (issyshdr)
		sp->sflags |= SINSYS; /* declared in system header */
	return sp;
}

/*
 * Declare old-stype function arguments.
 */
static void
oldargs(P1ND *p)
{
	blevel++;
	p->n_op = TYPE;
	p->n_type = FARG;
	p->n_sp = lookup((char *)p->n_sp, 0);/* XXX */
	defid(p, PARAM);
	blevel--;
}

/*
 * Set NAME nodes to a null name and index of LB nodes to NOOFFSET
 * unless clr is one, in that case preserve variable name.
 */
static P1ND *
namekill(P1ND *p, int clr)
{
	P1ND *q;
	int o = p->n_op;

	switch (coptype(o)) {
	case LTYPE:
		if (o == NAME) {
			if (clr)
				p->n_sp = NULL;
			else
				p->n_sp = lookup((char *)p->n_sp, 0);/* XXX */
		}
		break;

	case UTYPE:
		p->n_left = namekill(p->n_left, clr);
		break;

        case BITYPE:
                p->n_left = namekill(p->n_left, clr);
		if (o == LB) {
			if (clr) {
				p1tfree(p->n_right);
				p->n_right = bcon(NOOFFSET);
			} else
				p->n_right = eve(p->n_right);
		} else if (o == CALL)
			p->n_right = namekill(p->n_right, 1);
		else
			p->n_right = namekill(p->n_right, clr);
		if (o == TYMERGE) {
			q = tymerge(p->n_left, p->n_right);
			q->n_ap = attr_add(q->n_ap, p->n_ap);
			p1tfree(p->n_left);
			p1nfree(p);
			p = q;
		}
		break;
	}
	return p;
}

/*
 * Declare function arguments.
 */
static P1ND *
funargs(P1ND *p)
{
	extern P1ND *arrstk[10];

	if (p->n_op == ELLIPSIS)
		return p;

	p = namekill(p, 0);
	if (ISFTN(p->n_type))
		p->n_type = INCREF(p->n_type);
	if (ISARY(p->n_type)) {
		p->n_type += (PTR-ARY);
		if (p->n_df->ddim == -1)
			p1tfree(arrstk[0]), arrstk[0] = NULL;
		p->n_df++;
	}
	if (p->n_type == VOID && p->n_sp->sname == NULL)
		return p; /* sanitycheck later */
	else if (p->n_sp->sname == NULL)
		uerror("argument missing");
	else
		defid(p, PARAM);
	return p;
}

static P1ND *
listfw(P1ND *p, P1ND * (*f)(P1ND *))
{
        if (p->n_op == CM) {
                p->n_left = listfw(p->n_left, f);
                p->n_right = (*f)(p->n_right);
        } else
                p = (*f)(p);
	return p;
}


/*
 * Declare a function.
 */
static void
fundef(P1ND *tp, P1ND *p)
{
	extern int prolab;
	struct symtab *s;
	P1ND *q, *typ;
	int class = glval(tp), oclass, ctval;

	/*
	 * We discard all names except for those needed for
	 * parameter declaration. While doing that, also change
	 * non-constant array sizes to unknown.
	 */
	ctval = tvaloff;
	for (q = p; coptype(q->n_op) != LTYPE &&
	    q->n_left->n_op != NAME; q = q->n_left) {
		if (q->n_op == CALL)
			q->n_right = namekill(q->n_right, 1);
	}
	if (q->n_op != CALL && q->n_op != UCALL) {
		uerror("invalid function definition");
		p = bdty(UCALL, p);
	} else if (q->n_op == CALL) {
		blevel = 1;
		argoff = ARGINIT;
		if (oldstyle == 0)
			q->n_right = listfw(q->n_right, funargs);
		ftnarg(q);
		blevel = 0;
	}

	p = typ = tymerge(tp, p);
#ifdef GCC_COMPAT
	/* gcc seems to discard __builtin_ when declaring functions */
	if (strncmp("__builtin_", (char *)typ->n_sp, 10) == 0)
		typ->n_sp = (struct symtab *)((char *)typ->n_sp + 10);
#endif
	s = typ->n_sp = lookup((char *)typ->n_sp, 0); /* XXX */

	oclass = s->sclass;
	if (class == STATIC && oclass == EXTERN)
		werror("%s was first declared extern, then static", s->sname);

	if (fun_inline) {
		/* special syntax for inline functions */
		if (! strcmp(s->sname,"main")) 
			uerror("cannot inline main()");

		s->sflags |= SINLINE;
		inline_start(s, class);
		if (class == EXTERN)
			class = EXTDEF;
	} else if (class == EXTERN)
		class = SNULL; /* same result */

	cftnsp = s;
	defid(p, class);
	if (s->sdf->dfun == 0 && !issyshdr)
		warner(Wstrict_prototypes);
#ifdef GCC_COMPAT
	if (attr_find(p->n_ap, GCC_ATYP_ALW_INL)) {
		/* Temporary turn on temps to make always_inline work */
		alwinl = 1;
		if (xtemps == 0) alwinl |= 2;
		xtemps = 1;
	}
#endif
	prolab = getlab();
	send_passt(IP_PROLOG, -1, getexname(cftnsp), cftnsp->stype,
	    cftnsp->sclass == EXTDEF, prolab, ctval);
	blevel++;
#ifdef STABS
	if (gflag)
		stabs_func(s);
#endif
	p1tfree(tp);
	p1tfree(p);

}

static void
fend(void)
{
	if (blevel)
		cerror("function level error");
	ftnend();
	fun_inline = 0;
	if (alwinl & 2) xtemps = 0;
	alwinl = 0;
	cftnsp = NULL;
}

P1ND *
structref(P1ND *p, int f, char *name)
{
	P1ND *r;

	if (f == DOT)
		p = buildtree(ADDROF, p, NULL);
	r = biop(NAME, NULL, NULL);
	r->n_name = name;
	r = buildtree(STREF, p, r);
	return r;
}

static void
olddecl(P1ND *p, P1ND *a)
{
	struct symtab *s;

	p = namekill(p, 0);
	s = p->n_sp;
	if (s->slevel != 1 || s->stype == UNDEF)
		uerror("parameter '%s' not defined", s->sname);
	else if (s->stype != FARG)
		uerror("parameter '%s' redefined", s->sname);

	s->stype = p->n_type;
	s->sdf = p->n_df;
	s->sap = p->n_ap;
	if (ISARY(s->stype)) {
		s->stype += (PTR-ARY);
		s->sdf++;
	} else if (s->stype == FLOAT)
		s->stype = DOUBLE;
	if (a)
		attr_add(s->sap, gcc_attr_wrapper(a));
	p1nfree(p);
}

void
branch(int lbl)
{
	int r = reached++;
	ecomp(biop(GOTO, bcon(lbl), NULL));
	reached = r;
}

/*
 * Create a printable string based on an encoded string.
 */
static char *
mkpstr(char *str)
{
	char *os, *s;
	int l = strlen(str) + 3; /* \t + \n + \0 */

	os = s = stmtalloc(l);
	*s++ = '\t';
	while (*str) {
		if (*str == '\\')
			*s++ = esccon(&str);
		else
			*s++ = *str++;
	}
	*s++ = '\n';
	*s = 0;

	return os;
}

/*
 * Fake a symtab entry for compound literals.
 */
static struct symtab *
clbrace(P1ND *p)
{
	struct symtab *sp;

	sp = getsymtab(simname("cl"), STEMP);
	sp->stype = p->n_type;
	sp->squal = p->n_qual;
	sp->sdf = p->n_df;
	sp->sap = p->n_ap;
	p1tfree(p);
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
	char *w = tmpalloc(len); /* uncommon */

	snprintf(w, len, "%s%d", s, getlab());
	return w;
}

P1ND *
biop(int op, P1ND *l, P1ND *r)
{
	return block(op, l, r, INT, 0, 0);
}

static P1ND *
cmop(P1ND *l, P1ND *r)
{
	return biop(CM, l, r);
}

static P1ND *
voidcon(void)
{
	return block(ICON, NULL, NULL, STRTY, 0, 0);
}

/* Support for extended assembler a' la' gcc style follows below */

static P1ND *
xmrg(P1ND *out, P1ND *in)
{
	P1ND *p = in;

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
static P1ND *
xcmop(P1ND *out, P1ND *in, P1ND *str)
{
	P1ND *p, *q;

	if (out) {
		/* D out-list sanity check */
		for (p = out; p->n_op == CM; p = p->n_left) {
			q = p->n_right;
			if (q->n_name[0] != '=' && q->n_name[0] != '+')
				uerror("output missing =");
		}
		if (p->n_name[0] != '=' && p->n_name[0] != '+')
			uerror("output missing =");
		if (in == NULL)
			p = out;
		else
			p = xmrg(out, in);
	} else if (in) {
		p = in;
	} else
		p = voidcon();

	if (str == NULL)
		str = voidcon();
	return cmop(p, str);
}

/*
 * Generate a XARG node based on a string and an expression.
 */
static P1ND *
xasmop(char *str, P1ND *p)
{

	p = biop(XARG, p, NULL);
	p->n_name = str;
	return p;
}

/*
 * Generate a XASM node based on a string and an expression.
 */
static void
mkxasm(char *str, P1ND *p)
{
	P1ND *q;

	q = biop(XASM, p->n_left, p->n_right);
	q->n_name = str;
	p1nfree(p);
	ecomp(optloop(q));
}

static struct attr *
gcc_attr_wrapper(P1ND *p)
{
#ifdef GCC_COMPAT
	return gcc_attr_parse(p);
#else
	if (p != NULL)
		uerror("gcc attribute used");
	return NULL;
#endif
}

#ifdef GCC_COMPAT
static P1ND *
tyof(P1ND *p)
{
	static struct symtab spp;
	P1ND *q = block(TYPE, NULL, NULL, p->n_type, p->n_df, p->n_ap);
	q->n_qual = p->n_qual;
	q->n_sp = &spp; /* for typenode */
	p1tfree(p);
	return q;
}

#else
static P1ND *
tyof(P1ND *p)
{
	uerror("typeof gcc extension");
	return bcon(0);
}
#endif

/*
 * Traverse an unhandled expression tree bottom-up and call buildtree()
 * or equivalent as needed.
 */
P1ND *
eve(P1ND *p)
{
	struct symtab *sp;
	P1ND *r, *p1, *p2;
	int x;

	p1 = p->n_left;
	p2 = p->n_right;
	switch (p->n_op) {
	case NAME:
		sp = lookup((char *)p->n_sp,
		    attr_find(p->n_ap, ATTR_P1LABELS) ? SLBLNAME|STEMP : 0);
		if (sp->sflags & SINLINE)
			inline_ref(sp);
		r = nametree(sp);
		if (sp->sflags & SDYNARRAY)
			r = buildtree(UMUL, r, NULL);
#ifdef GCC_COMPAT
		if (attr_find(sp->sap, GCC_ATYP_DEPRECATED))
			warner(Wdeprecated_declarations, sp->sname);
#endif
		break;

	case DOT:
	case STREF:
		r = structref(eve(p1), p->n_op, (char *)p2->n_sp);
		p1nfree(p2);
		break;

	case CAST:
		p2 = eve(p2);
#ifndef NO_COMPLEX
		if (ANYCX(p1) || ANYCX(p2)) {
			r = cxcast(p1, p2);
			break;
		}
#endif
#ifdef TARGET_TIMODE
		if ((r = gcc_eval_ticast(CAST, p1, p2)) != NULL)
			break;
#endif
		p1 = buildtree(CAST, p1, p2);
		p1nfree(p1->n_left);
		r = p1->n_right;
		p1nfree(p1);
		break;


	case SZOF:
		x = xinline; xinline = 0; /* XXX hack */
		if (glval(p2) == 0)
			p1 = eve(p1);
		else
			TYMFIX(p1);
		p1nfree(p2);
		r = doszof(p1);
		xinline = x;
		break;

	case LB:
		p1 = eve(p1);
		p2 = eve(p2);
#ifdef TARGET_TIMODE
		if (isti(p2)) {
			P1ND *s = block(NAME, NULL, NULL, LONG, 0, 0);
			if ((r = gcc_eval_ticast(CAST, s, p2)) != NULL)
				p2 = r;
			p1nfree(s);
		}
#endif
		r = buildtree(UMUL, buildtree(PLUS, p1, p2), NULL);
		break;

	case COMPL:
#ifndef NO_COMPLEX
		p1 = eve(p1);
		if (ANYCX(p1))
			r = cxconj(p1);
		else
			r = buildtree(COMPL, p1, NULL);
		break;
#endif
	case UPLUS:
		r = eve(p1);
		if (r->n_op == FLD || r->n_type < INT)
			r = buildtree(PLUS, r, bcon(0)); /* must be size int */
		break;

	case UMINUS:
#ifndef NO_COMPLEX
		p1 = eve(p1);
		if (ANYCX(p1))
			r = cxop(UMINUS, p1, p1);
		else
			r = buildtree(UMINUS, p1, NULL);
		break;
#endif
	case NOT:
	case UMUL:
		p1 = eve(p1);
#ifdef TARGET_TIMODE
		if ((r = gcc_eval_tiuni(p->n_op, p1)) != NULL)
			break;
#endif
#ifndef NO_COMPLEX
		if (p->n_op == NOT && ANYCX(p1))
			p1 = cxop(NE, p1, bcon(0));
#endif
		r = buildtree(p->n_op, p1, NULL);
		break;

	case ADDROF:
		r = eve(p1);
		if (ISFTN(p->n_type)/* || ISARY(p->n_type) */){
#ifdef notdef
			werror( "& before array or function: ignored" );
#endif
		} else
			r = buildtree(ADDROF, r, NULL);
		break;

	case UCALL:
		p2 = NULL;
		/* FALLTHROUGH */
	case CALL:
		if (p1->n_op == NAME) {
			sp = lookup((char *)p1->n_sp, 0);
#ifndef NO_C_BUILTINS
			if (sp->sflags & SBUILTIN) {
				p1nfree(p1);
				r = builtin_check(sp, p2);
				break;
			}
#endif
			if (sp->stype == UNDEF) {
				p1->n_type = FTN|INT;
				p1->n_sp = sp;
				p1->n_ap = NULL;
				defid(p1, EXTERN);
			}
			p1nfree(p1);
#ifdef GCC_COMPAT
			if (attr_find(sp->sap, GCC_ATYP_DEPRECATED))
				warner(Wdeprecated_declarations, sp->sname);
#endif
			if (p->n_op == CALL)
				p2 = eve(p2);
			r = doacall(sp, nametree(sp), p2);
		} else {
			if (p->n_op == CALL)
				p2 = eve(p2);
			r = doacall(NULL, eve(p1), p2);
		}
		break;

#ifndef NO_COMPLEX
	case XREAL:
	case XIMAG:
		p1 = eve(p1);
		r = cxelem(p->n_op, p1);
		break;
#endif

	case COLON:
	case MUL:
	case DIV:
	case PLUS:
	case MINUS:
	case ASSIGN:
	case EQ:
	case NE:
	case OROR:
	case ANDAND:
#ifndef NO_COMPLEX
		p1 = eve(p1);
		p2 = eve(p2);
#ifdef TARGET_TIMODE
		if ((r = gcc_eval_timode(p->n_op, p1, p2)) != NULL)
			break;
#endif
		if (ANYCX(p1) || ANYCX(p2)) {
			r = cxop(p->n_op, p1, p2);
		} else if (ISITY(p1->n_type) || ISITY(p2->n_type)) {
			r = imop(p->n_op, p1, p2);
		} else
			r = buildtree(p->n_op, p1, p2);
		break;
#endif
	case MOD:
	case CM:
	case GT:
	case GE:
	case LT:
	case LE:
	case RS:
	case LS:
	case RSEQ:
	case LSEQ:
	case AND:
	case OR:
	case ER:
	case EREQ:
	case OREQ:
	case ANDEQ:
	case QUEST:
		p1 = eve(p1);
		p2 = eve(p2);
#ifdef TARGET_TIMODE
		if ((r = gcc_eval_timode(p->n_op, p1, p2)) != NULL)
			break;
#endif
		r = buildtree(p->n_op, p1, p2);
		break;

	case BIQUEST: /* gcc e ?: e op */
		p1 = eve(p1);
		r = tempnode(0, p1->n_type, p1->n_df, p1->n_ap);
		p2 = eve(biop(COLON, p1tcopy(r), p2));
		r = buildtree(QUEST, buildtree(ASSIGN, r, p1), p2);
		break;

	case INCR:
	case DECR:
	case MODEQ:
	case MINUSEQ:
	case PLUSEQ:
	case MULEQ:
	case DIVEQ:
		p1 = eve(p1);
		p2 = eve(p2);
#ifdef TARGET_TIMODE
		if ((r = gcc_eval_timode(p->n_op, p1, p2)) != NULL)
			break;
#endif
#ifndef NO_COMPLEX
		if (ANYCX(p1) || ANYCX(p2)) {
			r = cxop(UNASG p->n_op, p1tcopy(p1), p2);
			r = cxop(ASSIGN, p1, r);
			break;
		} else if (ISITY(p1->n_type) || ISITY(p2->n_type)) {
			r = imop(UNASG p->n_op, p1tcopy(p1), p2);
			r = cxop(ASSIGN, p1, r);
			break;
		}
		/* FALLTHROUGH */
#endif
		r = buildtree(p->n_op, p1, p2);
		break;

	case STRING:
		r = strend(p->n_name, (TWORD)glval(p));
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
	case TEMP:
		return p;

	case CLOP:
		r = nametree(p->n_sp);
		break;

	default:
#ifdef PCC_DEBUG
		p1fwalk(p, eprint, 0);
#endif
		cerror("eve");
		r = NULL;
	}
	p1nfree(p);
	return r;
}

int
con_e(P1ND *p)
{
	return icons(optloop(eve(p)));
}

void
uawarn(P1ND *p, char *s)
{
	if (p == 0)
		return;
	if (attrwarn)
		werror("unhandled %s attribute", s);
	p1tfree(p);
}

static void
dainit(P1ND *d, P1ND *a)
{
	if (d == NULL) {
		asginit(a);
	} else if (d->n_op == CM) {
		int is = con_e(d->n_left);
		int ie = con_e(d->n_right);
		int i;

		p1nfree(d);
		if (ie < is)
			uerror("negative initializer range");
		desinit(biop(LB, NULL, bcon(is)));
		for (i = is; i < ie; i++)
			asginit(p1tcopy(a));
		asginit(a);
	} else {
		cerror("dainit");
	}
}

/*
 * Traverse down and tymerge() where appropriate.
 */
static P1ND *
tymfix(P1ND *p)
{
	P1ND *q;
	int o = coptype(p->n_op);

	switch (o) {
	case LTYPE:
		break;
	case UTYPE:
		p->n_left = tymfix(p->n_left);
		break;
	case BITYPE:
		p->n_left = tymfix(p->n_left);
		p->n_right = tymfix(p->n_right);
		if (p->n_op == TYMERGE) {
			q = tymerge(p->n_left, p->n_right);
			q->n_ap = attr_add(q->n_ap, p->n_ap);
			p1tfree(p->n_left);
			p1nfree(p);
			p = q;
		}
		break;
	}
	return p;
}

static P1ND *
aryfix(P1ND *p)
{
	P1ND *q;

	for (q = p; q->n_op != NAME; q = q->n_left) {
		if (q->n_op == LB) {
			q->n_right = optloop(eve(q->n_right));
			if ((blevel == 0 || rpole != NULL) &&
			    !nncon(q->n_right))
				uerror("array size not constant"); 
			/*
			 * Checks according to 6.7.5.2 clause 1:
			 * "...the expression shall have an integer type."
			 * "If the expression is a constant expression,	 
			 * it shall have a value greater than zero."
			 */
			if (!ISINTEGER(q->n_right->n_type))
				werror("array size is not an integer");
			else if (q->n_right->n_op == ICON &&
			    glval(q->n_right) < 0 &&
			    glval(q->n_right) != NOOFFSET) {
					uerror("array size cannot be negative");
					slval(q->n_right, 1);
			}
		} else if (q->n_op == CALL)
			q->n_right = namekill(q->n_right, 1);
	}
	return p;
}

struct labs {
	struct labs *next;
	int lab;
} *labp;

static void
savlab(int lab)
{
	struct labs *l = tmpalloc(sizeof(struct labs)); /* uncommon */
	l->lab = lab < 0 ? -lab : lab;
	l->next = labp;
	labp = l;
}

int *
mkclabs(void)
{
	struct labs *l;
	int i, *rv;

	for (i = 0, l = labp; l; l = l->next, i++)
		;
	rv = tmpalloc((i+1)*sizeof(int));	/* uncommon */
	for (i = 0, l = labp; l; l = l->next, i++)
		rv[i] = l->lab;
	rv[i] = 0;
	labp = 0;
	return rv;
}

void
xcbranch(P1ND *p, int lab)
{
#ifndef NO_COMPLEX
	if (ANYCX(p))
		p = cxop(NE, p, bcon(0));
#endif
	cbranch(buildtree(NOT, p, NULL), bcon(lab));
}

/*
 * New a case entry to genlist.
 * tn is type, e is expression.
 */
static struct genlist *
newgen(P1ND *tn, P1ND *e)
{
	struct genlist *ng;
	TWORD t;

	if (tn) {
		t = tn->n_type;
		p1tfree(tn);
	} else
		t = 0;

	/* add new entry */
	ng = malloc(sizeof(struct genlist));
	ng->next = NULL;
	ng->t = t;
	ng->p = e;
	return ng;
}

/*
 * Add a case entry to genlist.
 * g is list, ng is new entry.
 */
static struct genlist *
addgen(struct genlist *g, struct genlist *ng)
{
	struct genlist *w;

	/* search for duplicate type */
	for (w = g; w; w = w->next) {
		if (w->t == ng->t)
			uerror("duplicate type in _Generic");
	}
	ng->next = g;
	return ng;
}

static P1ND *
dogen(struct genlist *g, P1ND *e)
{
	struct genlist *ng;
	P1ND *w, *p;

	e = eve(e);

	/* search for direct match */
	for (ng = g, w = p = NULL; ng; ng = ng->next) {
		if (ng->t == 0)
			p = ng->p; /* save default */
		if (e->n_type == ng->t)
			w = ng->p;
	}

	/* if no match, use generic */
	if (w == NULL) {
		if (p == NULL) {
			uerror("_Generic: no default found");
			p = bcon(0);
		}
		w = p;
	}

	/* free tree */
	while (g) {
		if (g->p != w)
			p1tfree(g->p);
		ng = g->next;
		free(g);
		g = ng;
	}

	p1tfree(e);
	return w;
}
