/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1998 Free Software Foundation, Inc.

   This file was written by Peter Miller <millerp@canb.auug.org.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "po-lex.h"
#include "po-gram.h"
#include "error.h"
#include "system.h"
#include <libintl.h>
#include "po.h"

#define _(str) gettext (str)
%}

%token	COMMENT
%token	DOMAIN
%token	JUNK
%token	MSGID
%token	MSGSTR
%token	NAME
%token	NUMBER
%token	STRING

%union
{
  char *string;
  long number;
  lex_pos_ty pos;
}

%type <string> STRING COMMENT string_list
%type <number> NUMBER
%type <pos> msgid msgstr

%right MSGSTR

%%

msgfmt
	: /* empty */
	| msgfmt comment
	| msgfmt domain
	| msgfmt message
	| msgfmt error
	;

domain
	: DOMAIN STRING
		{
		   po_callback_domain ($2);
		}
	;

message
	: msgid string_list msgstr string_list
		{
		  po_callback_message ($2, &$1, $4, &$3);
		}
	| msgid string_list
		{
		  gram_error_at_line (&$1, _("missing `msgstr' section"));
		  free ($2);
		}
	;

msgid
	: MSGID
		{
		  $$ = gram_pos;
		}
	;

msgstr
	: MSGSTR
		{
		  $$ = gram_pos;
		}
	;

string_list
	: STRING
		{
		  $$ = $1;
		}
	| string_list STRING
		{
		  size_t len1;
		  size_t len2;

		  len1 = strlen ($1);
		  len2 = strlen ($2);
		  $$ = (char *) xmalloc (len1 + len2 + 1);
		  stpcpy (stpcpy ($$, $1), $2);
		  free ($1);
		  free ($2);
		}
	;

comment
	: COMMENT
		{
		  po_callback_comment ($1);
		}
	;
