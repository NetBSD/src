%{
/*	$NetBSD: gram.y,v 1.2.2.3 2001/03/12 13:28:10 bouyer Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <strings.h>

#include "platid_gen.h"

#define LIST_NEW(l)	{ \
	(l) = new_node(N_LIST, 0, NULL, NULL, NULL); \
}
#define LIST_ADD(l, i)	{ \
	if ((l)->ptr1 == NULL) { \
		(l)->ptr1 = (i); \
		(l)->ptr2 = (i); \
	} else { \
		((node_t*)(l)->ptr2)->link = (i); \
		(l)->ptr2 = (i); \
	} \
	(i)->link = NULL; \
	(l)->val++; \
}

%}

%union {
	struct node_s *node;
	const char *str;
	int	val;
}

%token '{' '}' '=' ':'
%token <str>FSYM
%token <str>SYM
%token <str>MOD
%token <str>NAME
%token <str>DIRECTIVE

%type <str>sym
%type <val>name_prefix
%type <node>name_opt
%type <node>ent
%type <node>sub_list
%type <node>sub_item
%type <node>list
%type <node>item

%%

start: list { def_tree = $1; };

list:
  list item { LIST_ADD($1, $2); $$ = $1; } |
  /* empty */ { LIST_NEW($$); };

item:
sym ':' { $$ = new_node(N_LABEL, 0, $1, NULL, NULL); } |
sym '=' sym  { $$ = new_node(N_MODIFIER, 0, $1, $3, NULL); } |
ent { $$ = $1; }|
'{' sub_list '}' { $$ = $2; } |
DIRECTIVE { $$ = new_node(N_DIRECTIVE, 0, $1, NULL, NULL); };

sub_list:
  sub_list sub_item { LIST_ADD($1, $2); $$ = $1; } |
  /* empty */ { LIST_NEW($$); };

sub_item:
  sym '=' sym { $$ = new_node(N_MODIFIER, 0, $1, $3, NULL); }|
  ent { $$ = $1; } |
  '{' sub_list '}' { $$ = $2; } |
   DIRECTIVE { $$ = new_node(N_DIRECTIVE, 0, $1, NULL, NULL); };

ent : sym name_opt {
	$2->ptr1 = $1;
	/*
	if ($2->ptr2 == NULL) {
		$2->ptr2 = strdup($1);
	}
	touppers((char*)$2->ptr1);
	*/
	$$ = $2;
};

name_opt:
    name_prefix NAME { $$ = new_node(N_ENTRY, $1, NULL, $2, NULL); } |
    name_prefix { $$ = new_node(N_ENTRY, $1, NULL, NULL, NULL); };

name_prefix:
  name_prefix '-' { $$ = $1 + 1; } |
  /* empty */ { $$ = 0; }

sym:
  FSYM  { $$ = $1; } |
  SYM { $$ = $1; } |
  MOD  { $$ = $1; };

%%

char*
touppers(s)
	char *s;
{
	char *p;

	for (p = s; *p != '\0'; p++)
		*p = toupper(*p);
	return (s);
}

void*
mem_alloc(size)
	int size;
{
	void *res;

	if ((res = malloc(size)) == NULL) {
		fprintf(stderr, "memory allocation failed.\n");
		exit(1);
	}
	return (res);
}

node_t*
new_node(type, val, ptr1, ptr2, link)
	int type;
	int val;
	const void *ptr1, *ptr2;
	node_t *link;
{
	node_t *res;

	res = mem_alloc(sizeof(node_t));
	res->type = type;
	res->val = val;
	res->ptr1 = ptr1;
	res->ptr2 = ptr2;
	res->link = link;
	return (res);
}

void
dump_node(prefix, n)
	char *prefix;
	node_t* n;
{
	char prefix2[1024];
	node_t *np;

	sprintf(prefix2, "%s    ", prefix);

	switch (n->type) {
	case N_LABEL:
		printf("%s%s:\n", prefix, n->ptr1);
		break;
	case N_MODIFIER:
		printf("%s%s=%s\n", prefix, n->ptr1, n->ptr2);
		break;
	case N_ENTRY:
		if (n->val == 0)
			printf("%s%s(%s)\n", prefix, n->ptr1, n->ptr2);
		else
			printf("%s%s(-%d, %s)\n",
			       prefix, n->ptr1, n->val, n->ptr2);
		break;
	case N_LIST:
		printf("%s{\n", prefix);
	       	for (np = (node_t*)n->ptr1; np; np = np->link) {
			dump_node(prefix2, np);
		}
		printf("%s}\n", prefix);
		break;
	case N_DIRECTIVE:
		printf("%s", n->ptr1);
		break;
		break;
	default:
		printf("%s???\n", prefix);
		break;
	}
}

void
yyerror(s)
	const char *s;
{
	extern int yyline;

	fprintf(stderr, "%d: %s\n", yyline, s);
}
