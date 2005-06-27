%{
/* $NetBSD: cgdparse.y,v 1.2 2005/06/27 03:07:45 christos Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: cgdparse.y,v 1.2 2005/06/27 03:07:45 christos Exp $");
#endif

#include <stdio.h>

#include "params.h"
#include "utils.h"
#include "extern.h"

static struct params *yy_global_params;

%}
%union {
	int	 	 integer;
	struct {
		char	*text;
		int	 length;
	} token;
	string_t	*string;
	bits_t		*bits;
	struct params	*params;
	struct keygen	*keygen;
}

%type <params>	entry rules rule
%type <keygen>	kgrule kgbody kgvars kgvar deprecated
%type <string>	stringlit base64 intstr tokstr
%type <bits>	bits
%type <token>	token deptoken

%token <integer> INTEGER
%token <string> STRINGLIT

%token <token> ALGORITHM KEYLENGTH IVMETHOD VERIFY_METHOD
%token <token> KEYGEN SALT ITERATIONS KEY

%token EOL

/* Deprecated tokens */
%token <token> KEYGEN_METHOD KEYGEN_SALT KEYGEN_ITERATIONS XOR_KEY

%%

entry:	  rules				{ yy_global_params = $$; }

rules:	/* empty */			{ $$ = NULL; }
	| rules rule			{ $$ = params_combine($$, $2); }

rule:	  ALGORITHM stringlit EOL	{ $$ = params_algorithm($2); }
	| KEYLENGTH INTEGER EOL		{ $$ = params_keylen($2); }
	| IVMETHOD stringlit EOL	{ $$ = params_ivmeth($2); }
	| VERIFY_METHOD stringlit EOL	{ $$ = params_verify_method($2); }
	| kgrule			{ $$ = params_keygen($1); }
	| deprecated			{ $$ = params_dep_keygen($1); }
	| EOL				{ $$ = NULL; }

kgrule:	  KEYGEN stringlit kgbody EOL	{ $$ = keygen_set_method($3, $2); }

kgbody:	  kgvar				{ $$ = $1; }
	| '{' kgvars '}'		{ $$ = $2; }

kgvars: /* empty */			{ $$ = NULL; }
	| kgvars kgvar			{ $$ = keygen_combine($$, $2); }

kgvar:	  SALT bits EOL			{ $$ = keygen_salt($2); }
	| ITERATIONS INTEGER EOL	{ $$ = keygen_iterations($2); }
	| KEY bits EOL			{ $$ = keygen_key($2); }
	| EOL				{ $$ = NULL; }

stringlit:  STRINGLIT | tokstr | intstr

tokstr:	  token				{ $$ = string_new($1.text, $1.length); }

token:	  ALGORITHM | KEYLENGTH
	| IVMETHOD | VERIFY_METHOD
	| KEYGEN | SALT | ITERATIONS
	| KEY
	| deptoken

intstr:   INTEGER			{ $$ = string_fromint($1); }

bits:	  base64			{ $$ = bits_decode_d($1); }

base64:   stringlit
	| base64 stringlit		{ $$ = string_add_d($1, $2); }

/* The following rules are deprecated */

deprecated:
	  KEYGEN_METHOD stringlit EOL	{ $$ = keygen_method($2); }
	| KEYGEN_SALT bits EOL		{ $$ = keygen_salt($2); }
	| KEYGEN_ITERATIONS INTEGER EOL	{ $$ = keygen_iterations($2); }
	| XOR_KEY bits EOL		{ $$ = keygen_key($2); }

deptoken: KEYGEN_METHOD | KEYGEN_SALT
	| KEYGEN_ITERATIONS | XOR_KEY

%%

struct params *
cgdparsefile(FILE *f)
{

	yyin = f;
	yy_global_params = NULL;
	if (yyparse()) {
		fprintf(stderr, "%s: failed to parse parameters file\n",
		    getprogname());
		params_free(yy_global_params);
		return NULL;
	}
	return yy_global_params;
}
