/* $NetBSD: config_yacc.y,v 1.2 2003/08/06 18:07:53 jmmv Exp $ */

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

%{

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: config_yacc.y,v 1.2 2003/08/06 18:07:53 jmmv Exp $");
#endif /* not lint */

#include <sys/time.h>
#include <dev/wscons/wsconsio.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "wsmoused.h"

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);
void yyrestart(FILE *);
struct block *config_parse(FILE *);

int yyline;
static struct block *Conf;

%}

%token TK_EOL
%token TK_EQUAL TK_LBRACE TK_RBRACE
%token TK_STRING TK_EVENT TK_EVENTPROP TK_MODE TK_MODEPROP
%type <string> TK_STRING TK_EVENTPROP TK_MODEPROP
%type <prop> eventprop modeprop
%type <block> main outermode mode outerevent event
%union {
	char *string;
	struct prop *prop;
	struct block *block;
}

%%

Config :
	  main			{ Conf = $1;
				  Conf->b_name = strdup("global"); }
;

/* Matches the whole configuration file and constructs a block defining it.
   Can contain mode properties (common to all modes) and mode definitions. */
main :
	  modeprop		{ struct block *b = block_new(BLOCK_GLOBAL);
				  block_add_prop(b, $1);
				  $$ = b; }
	| main modeprop		{ block_add_prop($1, $2); }
	| outermode		{ struct block *b = block_new(BLOCK_GLOBAL);
				  block_add_child(b, $1);
				  $$ = b; }
	| main outermode	{ block_add_child($1, $2); }
	| error TK_EOL		{ yyerrok; }
;

/* Defines the aspect of a mode definition. Returns the block given by the
   mode definition itself. */
outermode :
	  TK_MODE TK_STRING TK_LBRACE mode TK_RBRACE
				{ $4->b_name = strdup($2);
				  $$ = $4; }
	| TK_MODE TK_STRING TK_LBRACE TK_RBRACE
				{ $$ = block_new(BLOCK_MODE);
				  $$->b_name = strdup($2); }
;

/* Matches a mode and returns a block defining it. Can contain properties
   and event definitions. */
mode :
	  modeprop		{ struct block *b = block_new(BLOCK_MODE);
				  block_add_prop(b, $1);
				  $$ = b; }
	| mode modeprop		{ block_add_prop($1, $2); }
	| outerevent		{ struct block *b = block_new(BLOCK_MODE);
				  block_add_child(b, $1);
				  $$ = b; }
	| mode outerevent	{ block_add_child($1, $2); }
	| error TK_EOL		{ yyerrok; }
;

/* Defines the aspect of an event definition. Returns the block given by the
   event definition itself. */
outerevent :
	  TK_EVENT TK_LBRACE event TK_RBRACE
				{ $$ = $3; }
	  TK_EVENT TK_LBRACE TK_RBRACE
				{ $$ = block_new(BLOCK_EVENT); }
;

/* Matches an event and returns a block defining it. Contains properties. */
event :
	  eventprop		{ struct block *b = block_new(BLOCK_EVENT);
				  block_add_prop(b, $1);
				  $$ = b; }
	| event eventprop	{ block_add_prop($1, $2); }
	| error TK_EOL		{ yyerrok; }
	| error			{ yyerrok; }
;

/* Matches a mode property and returns a prop defining it. */
modeprop :
	TK_MODEPROP TK_EQUAL TK_STRING TK_EOL
				{ struct prop *p = prop_new();
				  p->p_name = strdup($1);
				  p->p_value = strdup($3);
				  $$ = p; }
;

/* Matches an event property and returns a prop defining it. */
eventprop :
	TK_EVENTPROP TK_EQUAL TK_STRING TK_EOL
				{ struct prop *p = prop_new();
				  p->p_name = strdup($1);
				  p->p_value = strdup($3);
				  $$ = p; }
;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", getprogname());
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, " in line %d\n", yyline);
	va_end(ap);
	return (0);
}

struct block *
config_parse(FILE *f)
{
	Conf = NULL;
	yyrestart(f);
	yyline = 1;
	yyparse();

	return Conf;
}
