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

#include <system.h>
#include "po-hash.h"
#include "po.h"

%}

%token STRING
%token NUMBER
%token COLON
%token COMMA
%token FILE_KEYWORD
%token LINE_KEYWORD
%token NUMBER_KEYWORD

%union
{
  char *string;
  int number;
}

%type <number> NUMBER
%type <string> STRING

%{

static const char *cur;


void yyerror PARAMS ((char *));
int yylex PARAMS ((void));


int
po_hash (s)
     const char *s;
{
  extern int yyparse PARAMS ((void));

  cur = s;
  return yyparse ();
}


void
yyerror (s)
     char *s;
{
  /* Do nothing, the grammar is used as a recogniser.  */
}
%}

%%

filepos_line
	: /* empty */
	| filepos_line filepos
	;

filepos
	: STRING COLON NUMBER
		{
		  /* GNU style */
		  po_callback_comment_filepos ($1, $3);
		  free ($1);
		}
	| FILE_KEYWORD COLON STRING COMMA LINE_KEYWORD COLON NUMBER
		{
		  /* SunOS style */
		  po_callback_comment_filepos ($3, $7);
		  free ($3);
		}
	| FILE_KEYWORD COLON STRING COMMA LINE_KEYWORD NUMBER_KEYWORD COLON NUMBER
		{
		  /* Solaris style */
		  po_callback_comment_filepos ($3, $8);
		  free ($3);
		}
	| FILE_KEYWORD COLON NUMBER
		{
		  /* GNU style, but STRING is `file'.  Esoteric, but it
		     happened.  */
		  po_callback_comment_filepos ("file", $3);
		}
	;

%%


int
yylex ()
{
  static char *buf;
  static size_t bufmax;
  size_t bufpos;
  int n;
  int c;

  for (;;)
    {
      c = *cur++;
      switch (c)
	{
	case 0:
	  --cur;
	  return 0;

	case ' ':
	case '\t':
	case '\n':
	  break;

	case ':':
	  return COLON;

	case ',':
	  return COMMA;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  /* Accumulate a number.  */
	  n = 0;
	  for (;;)
	    {
	      n = n * 10 + c - '0';
	      c = *cur++;
	      switch (c)
		{
		default:
		  break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		  continue;
		}
	      break;
	    }
	  --cur;
	  yylval.number = n;
	  return NUMBER;

	default:
	  /* Accumulate a string.  */
	  bufpos = 0;
	  for (;;)
	    {
	      if (bufpos >= bufmax)
		{
		  bufmax += 100;
		  buf = xrealloc (buf, bufmax);
		}
	      buf[bufpos++] = c;

	      c = *cur++;
	      switch (c)
	        {
	        default:
	          continue;

	        case 0:
	        case ':':
	        case ',':
	        case ' ':
	        case '\t':
	          --cur;
	          break;
	        }
	      break;
	    }

	  if (bufpos >= bufmax)
	    {
	      bufmax += 100;
	      buf = xrealloc (buf, bufmax);
	    }
	  buf[bufpos] = 0;

	  if (strcmp (buf, "file") == 0 || strcmp (buf, "File") == 0)
	    return FILE_KEYWORD;
	  if (strcmp (buf, "line") == 0)
	    return LINE_KEYWORD;
	  if (strcmp (buf, "number") == 0)
	    return NUMBER_KEYWORD;
	  yylval.string = xstrdup (buf);
	  return STRING;
	}
    }
}
