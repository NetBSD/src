/* 	$NetBSD: config_yacc.y,v 1.2.2.3 2008/01/09 02:01:58 matt Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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

%{

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: config_yacc.y,v 1.2.2.3 2008/01/09 02:01:58 matt Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>

#include <prop/proplib.h>

#include "envstat.h"

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);
void yyrestart(FILE *);

int yyline;
char *yytext;
static prop_dictionary_t kdict;

%}

%token EOL EQUAL LBRACE RBRACE
%token STRING NUMBER SENSOR
%token SENSOR_PROP DEVICE_PROP
%token <string>	SENSOR STRING SENSOR_PROP DEVICE_PROP
%union {
	char *string;
}

%%

main	:	devices
     	|			{ exit(EXIT_SUCCESS); }
     	;

devices	:	device
	|	devices device
	;

device	:	STRING LBRACE props RBRACE
				{ config_devblock_add($1, kdict); }
	;

props	:	sensor
	|	props sensor
	|	devprop
	|	props devprop
	;

sensor	:	SENSOR LBRACE params RBRACE
				{ config_dict_add_prop("index", $1);
				  config_dict_mark(); }
	;

params	:	prop
	|	params prop
	;

prop	:	SENSOR_PROP EQUAL STRING EOL
     				{ config_dict_add_prop($1, $3); }
	;

devprop	:	DEVICE_PROP EQUAL STRING EOL
				{ config_dict_adddev_prop($1, $3, yyline); }
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

	exit(EXIT_FAILURE);
}

void
config_parse(FILE *f, prop_dictionary_t d)
{
	kdict = prop_dictionary_copy(d);
	yyrestart(f);
	yyline = 1;
	yyparse();
}
