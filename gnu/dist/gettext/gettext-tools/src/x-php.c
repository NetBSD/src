/* xgettext PHP backend.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.

   This file was written by Bruno Haible <bruno@clisp.org>, 2002.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "message.h"
#include "xgettext.h"
#include "x-php.h"
#include "error.h"
#include "xalloc.h"
#include "exit.h"
#include "gettext.h"

#define _(s) gettext(s)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* The PHP syntax is defined in phpdoc/manual/langref.html.
   See also php-4.1.0/Zend/zend_language_scanner.l.  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_php_extract_all ()
{
  extract_all = true;
}


void
x_php_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      const char *end;
      int argnum1;
      int argnum2;
      const char *colon;

      if (keywords.table == NULL)
	init_hash (&keywords, 100);

      split_keywordspec (name, &end, &argnum1, &argnum2);

      /* The characters between name and end should form a valid C identifier.
	 A colon means an invalid parse in split_keywordspec().  */
      colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
	{
	  if (argnum1 == 0)
	    argnum1 = 1;
	  insert_entry (&keywords, name, end - name,
			(void *) (long) (argnum1 + (argnum2 << 10)));
	}
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      x_php_keyword ("_");
      x_php_keyword ("gettext");
      x_php_keyword ("dgettext:2");
      x_php_keyword ("dcgettext:2");
      /* The following were added in PHP 4.2.0.  */
      x_php_keyword ("ngettext:1,2");
      x_php_keyword ("dngettext:2,3");
      x_php_keyword ("dcngettext:2,3");
      default_keywords = false;
    }
}

void
init_flag_table_php ()
{
  xgettext_record_flag ("_:1:pass-php-format");
  xgettext_record_flag ("gettext:1:pass-php-format");
  xgettext_record_flag ("dgettext:2:pass-php-format");
  xgettext_record_flag ("dcgettext:2:pass-php-format");
  xgettext_record_flag ("ngettext:1:pass-php-format");
  xgettext_record_flag ("ngettext:2:pass-php-format");
  xgettext_record_flag ("dngettext:2:pass-php-format");
  xgettext_record_flag ("dngettext:3:pass-php-format");
  xgettext_record_flag ("dcngettext:2:pass-php-format");
  xgettext_record_flag ("dcngettext:3:pass-php-format");
  xgettext_record_flag ("sprintf:1:php-format");
  xgettext_record_flag ("printf:1:php-format");
}


/* ======================== Reading of characters.  ======================== */


/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* Logical filename and line number, used to label the extracted messages.  */
static char *logical_file_name;
static int line_number;

/* The input file stream.  */
static FILE *fp;


/* 1. line_number handling.  */

static unsigned char phase1_pushback[2];
static int phase1_pushback_length;

static int
phase1_getc ()
{
  int c;

  if (phase1_pushback_length)
    c = phase1_pushback[--phase1_pushback_length];
  else
    {
      c = getc (fp);

      if (c == EOF)
	{
	  if (ferror (fp))
	    error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
		   real_file_name);
	  return EOF;
	}
    }

  if (c == '\n')
    line_number++;

  return c;
}

/* Supports 2 characters of pushback.  */
static void
phase1_ungetc (int c)
{
  if (c != EOF)
    {
      if (c == '\n')
	--line_number;

      if (phase1_pushback_length == SIZEOF (phase1_pushback))
	abort ();
      phase1_pushback[phase1_pushback_length++] = c;
    }
}


/* 2. Ignore HTML sections.  They are equivalent to PHP echo commands and
   therefore don't contain translatable strings.  */

static void
skip_html ()
{
  for (;;)
    {
      int c = phase1_getc ();

      if (c == EOF)
	return;

      if (c == '<')
	{
	  int c2 = phase1_getc ();

	  if (c2 == EOF)
	    break;

	  if (c2 == '?')
	    {
	      /* <?php is the normal way to enter PHP mode. <? and <?= are
		 recognized by PHP depending on a configuration setting.  */
	      int c3 = phase1_getc ();

	      if (c3 != '=')
		phase1_ungetc (c3);

	      return;
	    }

	  if (c2 == '%')
	    {
	      /* <% and <%= are recognized by PHP depending on a configuration
		 setting.  */
	      int c3 = phase1_getc ();

	      if (c3 != '=')
		phase1_ungetc (c3);

	      return;
	    }

	  if (c2 == '<')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }

	  /* < script language = php >
	     < script language = "php" >
	     < script language = 'php' >
	     are always recognized.  */
	  while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	    c2 = phase1_getc ();
	  if (c2 != 's' && c2 != 'S')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'c' && c2 != 'C')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'r' && c2 != 'R')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'i' && c2 != 'I')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'p' && c2 != 'P')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 't' && c2 != 'T')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (!(c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r'))
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  do
	    c2 = phase1_getc ();
	  while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
	  if (c2 != 'l' && c2 != 'L')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'a' && c2 != 'A')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'n' && c2 != 'N')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'g' && c2 != 'G')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'u' && c2 != 'U')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'a' && c2 != 'A')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'g' && c2 != 'G')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  if (c2 != 'e' && c2 != 'E')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	    c2 = phase1_getc ();
	  if (c2 != '=')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  c2 = phase1_getc ();
	  while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	    c2 = phase1_getc ();
	  if (c2 == '"')
	    {
	      c2 = phase1_getc ();
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'h')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != '"')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	    }
	  else if (c2 == '\'')
	    {
	      c2 = phase1_getc ();
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'h')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != '\'')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	    }
	  else
	    {
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'h')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	      c2 = phase1_getc ();
	      if (c2 != 'p')
		{
		  phase1_ungetc (c2);
		  continue;
		}
	    }
	  c2 = phase1_getc ();
	  while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	    c2 = phase1_getc ();
	  if (c2 != '>')
	    {
	      phase1_ungetc (c2);
	      continue;
	    }
	  return;
	}
    }
}

#if 0

static unsigned char phase2_pushback[1];
static int phase2_pushback_length;

static int
phase2_getc ()
{
  int c;

  if (phase2_pushback_length)
    return phase2_pushback[--phase2_pushback_length];

  c = phase1_getc ();
  switch (c)
    {
    case '?':
    case '%':
      {
	int c2 = phase1_getc ();
	if (c2 == '>')
	  {
	    /* ?> and %> terminate PHP mode and switch back to HTML mode.  */
	    skip_html ();
	    return ' ';
	  }
	phase1_ungetc (c2);
      }
      break;

    case '<':
      {
	int c2 = phase1_getc ();

	/* < / script > terminates PHP mode and switches back to HTML mode.  */
	while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	  c2 = phase1_getc ();
	if (c2 == '/')
	  {
	    do
	      c2 = phase1_getc ();
	    while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
	    if (c2 == 's' || c2 == 'S')
	      {
		c2 = phase1_getc ();
		if (c2 == 'c' || c2 == 'C')
		  {
		    c2 = phase1_getc ();
		    if (c2 == 'r' || c2 == 'R')
		      {
			c2 = phase1_getc ();
			if (c2 == 'i' || c2 == 'I')
			  {
			    c2 = phase1_getc ();
			    if (c2 == 'p' || c2 == 'P')
			      {
				c2 = phase1_getc ();
				if (c2 == 't' || c2 == 'T')
				  {
				    do
				      c2 = phase1_getc ();
				    while (c2 == ' ' || c2 == '\t'
					   || c2 == '\n' || c2 == '\r');
				    if (c2 == '>')
				      {
					skip_html ();
					return ' ';
				      }
				  }
			      }
			  }
		      }
		  }
	      }
	  }
	phase1_ungetc (c2);
      }
      break;
    }

  return c;
}

static void
phase2_ungetc (int c)
{
  if (c != EOF)
    {
      if (phase2_pushback_length == SIZEOF (phase2_pushback))
	abort ();
      phase2_pushback[phase2_pushback_length++] = c;
    }
}

#endif


/* Accumulating comments.  */

static char *buffer;
static size_t bufmax;
static size_t buflen;

static inline void
comment_start ()
{
  buflen = 0;
}

static inline void
comment_add (int c)
{
  if (buflen >= bufmax)
    {
      bufmax = 2 * bufmax + 10;
      buffer = xrealloc (buffer, bufmax);
    }
  buffer[buflen++] = c;
}

static inline void
comment_line_end (size_t chars_to_remove)
{
  buflen -= chars_to_remove;
  while (buflen >= 1
	 && (buffer[buflen - 1] == ' ' || buffer[buflen - 1] == '\t'))
    --buflen;
  if (chars_to_remove == 0 && buflen >= bufmax)
    {
      bufmax = 2 * bufmax + 10;
      buffer = xrealloc (buffer, bufmax);
    }
  buffer[buflen] = '\0';
  xgettext_comment_add (buffer);
}


/* 3. Replace each comment that is not inside a string literal with a
   space character.  We need to remember the comment for later, because
   it may be attached to a keyword string.  */

/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;

static unsigned char phase3_pushback[1];
static int phase3_pushback_length;

static int
phase3_getc ()
{
  int lineno;
  int c;

  if (phase3_pushback_length)
    return phase3_pushback[--phase3_pushback_length];

  c = phase1_getc ();

  if (c == '#')
    {
      /* sh comment.  */
      bool last_was_qmark = false;

      comment_start ();
      lineno = line_number;
      for (;;)
	{
	  c = phase1_getc ();
	  if (c == '\n' || c == EOF)
	    {
	      comment_line_end (0);
	      break;
	    }
	  if (last_was_qmark && c == '>')
	    {
	      comment_line_end (1);
	      skip_html ();
	      break;
	    }
	  /* We skip all leading white space, but not EOLs.  */
	  if (!(buflen == 0 && (c == ' ' || c == '\t')))
	    comment_add (c);
	  last_was_qmark = (c == '?' || c == '%');
	}
      last_comment_line = lineno;
      return '\n';
    }
  else if (c == '/')
    {
      c = phase1_getc ();

      switch (c)
	{
	default:
	  phase1_ungetc (c);
	  return '/';

	case '*':
	  {
	    /* C comment.  */
	    bool last_was_star;

	    comment_start ();
	    lineno = line_number;
	    last_was_star = false;
	    for (;;)
	      {
		c = phase1_getc ();
		if (c == EOF)
		  break;
		/* We skip all leading white space, but not EOLs.  */
		if (buflen == 0 && (c == ' ' || c == '\t'))
		  continue;
		comment_add (c);
		switch (c)
		  {
		  case '\n':
		    comment_line_end (1);
		    comment_start ();
		    lineno = line_number;
		    last_was_star = false;
		    continue;

		  case '*':
		    last_was_star = true;
		    continue;

		  case '/':
		    if (last_was_star)
		      {
			comment_line_end (2);
			break;
		      }
		    /* FALLTHROUGH */

		  default:
		    last_was_star = false;
		    continue;
		  }
		break;
	      }
	    last_comment_line = lineno;
	    return ' ';
	  }

	case '/':
	  {
	    /* C++ comment.  */
	    bool last_was_qmark = false;

	    comment_start ();
	    lineno = line_number;
	    for (;;)
	      {
		c = phase1_getc ();
		if (c == '\n' || c == EOF)
		  {
		    comment_line_end (0);
		    break;
		  }
		if (last_was_qmark && c == '>')
		  {
		    comment_line_end (1);
		    skip_html ();
		    break;
		  }
		/* We skip all leading white space, but not EOLs.  */
		if (!(buflen == 0 && (c == ' ' || c == '\t')))
		  comment_add (c);
		last_was_qmark = (c == '?' || c == '%');
	      }
	    last_comment_line = lineno;
	    return '\n';
	  }
	}
    }
  else
    return c;
}

#ifdef unused
static void
phase3_ungetc (int c)
{
  if (c != EOF)
    {
      if (phase3_pushback_length == SIZEOF (phase3_pushback))
	abort ();
      phase3_pushback[phase3_pushback_length++] = c;
    }
}
#endif


/* ========================== Reading of tokens.  ========================== */


enum token_type_ty
{
  token_type_eof,
  token_type_lparen,		/* ( */
  token_type_rparen,		/* ) */
  token_type_comma,		/* , */
  token_type_string_literal,	/* "abc" */
  token_type_symbol,		/* symbol, number */
  token_type_other		/* misc. operator */
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;
struct token_ty
{
  token_type_ty type;
  char *string;		/* for token_type_string_literal, token_type_symbol */
  int line_number;
};


/* Free the memory pointed to by a 'struct token_ty'.  */
static inline void
free_token (token_ty *tp)
{
  if (tp->type == token_type_string_literal || tp->type == token_type_symbol)
    free (tp->string);
}


/* 4. Combine characters into tokens.  Discard whitespace.  */

static void
x_php_lex (token_ty *tp)
{
  static char *buffer;
  static int bufmax;
  int bufpos;
  int c;

  tp->string = NULL;

  for (;;)
    {
      tp->line_number = line_number;
      c = phase3_getc ();
      switch (c)
	{
	case EOF:
	  tp->type = token_type_eof;
	  return;

	case '\n':
	  if (last_non_comment_line > last_comment_line)
	    xgettext_comment_reset ();
	  /* FALLTHROUGH */
	case ' ':
	case '\t':
	case '\r':
	  /* Ignore whitespace.  */
	  continue;
	}

      last_non_comment_line = tp->line_number;

      switch (c)
	{
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
	case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '_':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
	case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
	case 'v': case 'w': case 'x': case 'y': case 'z':
	  bufpos = 0;
	  for (;;)
	    {
	      if (bufpos >= bufmax)
		{
		  bufmax = 2 * bufmax + 10;
		  buffer = xrealloc (buffer, bufmax);
		}
	      buffer[bufpos++] = c;
	      c = phase1_getc ();
	      switch (c)
		{
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case '_':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		  continue;

		default:
		  phase1_ungetc (c);
		  break;
		}
	      break;
	    }
	  if (bufpos >= bufmax)
	    {
	      bufmax = 2 * bufmax + 10;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos] = 0;
	  tp->string = xstrdup (buffer);
	  tp->type = token_type_symbol;
	  return;

	case '\'':
	  /* Single-quoted string literal.  */
	  bufpos = 0;
	  for (;;)
	    {
	      c = phase1_getc ();
	      if (c == EOF || c == '\'')
		break;
	      if (c == '\\')
		{
		  c = phase1_getc ();
		  if (c != '\\' && c != '\'')
		    {
		      phase1_ungetc (c);
		      c = '\\';
		    }
		}
	      if (bufpos >= bufmax)
		{
		  bufmax = 2 * bufmax + 10;
		  buffer = xrealloc (buffer, bufmax);
		}
	      buffer[bufpos++] = c;
	    }
	  if (bufpos >= bufmax)
	    {
	      bufmax = 2 * bufmax + 10;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos] = 0;
	  tp->type = token_type_string_literal;
	  tp->string = xstrdup (buffer);
	  return;

	case '"':
	  /* Double-quoted string literal.  */
	  tp->type = token_type_string_literal;
	  bufpos = 0;
	  for (;;)
	    {
	      c = phase1_getc ();
	      if (c == EOF || c == '"')
		break;
	      if (c == '$')
		{
		  c = phase1_getc ();
		  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
		      || c == '_' || c == '{' || c >= 0x7f)
		    {
		      /* String with variables.  */
		      tp->type = token_type_other;
		      continue;
		    }
		  phase1_ungetc (c);
		  c = '$';
		}
	      if (c == '{')
		{
		  c = phase1_getc ();
		  if (c == '$')
		    {
		      /* String with expressions.  */
		      tp->type = token_type_other;
		      continue;
		    }
		  phase1_ungetc (c);
		  c = '{';
		}
	      if (c == '\\')
		{
		  int n, j;

		  c = phase1_getc ();
		  switch (c)
		    {
		    case '"':
		    case '\\':
		    case '$':
		      break;

		    case '0': case '1': case '2': case '3':
		    case '4': case '5': case '6': case '7':
		      n = 0;
		      for (j = 0; j < 3; ++j)
			{
			  n = n * 8 + c - '0';
			  c = phase1_getc ();
			  switch (c)
			    {
			    default:
			      break;

			    case '0': case '1': case '2': case '3':
			    case '4': case '5': case '6': case '7':
			      continue;
			    }
			  break;
			}
		      phase1_ungetc (c);
		      c = n;
		      break;

		    case 'x':
		      n = 0;
		      for (j = 0; j < 2; ++j)
			{
			  c = phase1_getc ();
			  switch (c)
			    {
			    case '0': case '1': case '2': case '3': case '4':
			    case '5': case '6': case '7': case '8': case '9':
			      n = n * 16 + c - '0';
			      break;
			    case 'A': case 'B': case 'C': case 'D': case 'E':
			    case 'F':
			      n = n * 16 + 10 + c - 'A';
			      break;
			    case 'a': case 'b': case 'c': case 'd': case 'e':
			    case 'f':
			      n = n * 16 + 10 + c - 'a';
			      break;
			    default:
			      phase1_ungetc (c);
			      c = 0;
			      break;
			    }
			  if (c == 0)
			    break;
			}
		      if (j == 0)
			{
			  phase1_ungetc ('x');
			  c = '\\';
			}
		      else
			c = n;
		      break;

		    case 'n':
		      c = '\n';
		      break;
		    case 't':
		      c = '\t';
		      break;
		    case 'r':
		      c = '\r';
		      break;

		    default:
		      phase1_ungetc (c);
		      c = '\\';
		      break;
		    }
		}
	      if (bufpos >= bufmax)
		{
		  bufmax = 2 * bufmax + 10;
		  buffer = xrealloc (buffer, bufmax);
		}
	      buffer[bufpos++] = c;
	    }
	  if (bufpos >= bufmax)
	    {
	      bufmax = 2 * bufmax + 10;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos] = 0;
	  if (tp->type == token_type_string_literal)
	    tp->string = xstrdup (buffer);
	  return;

	case '?':
	case '%':
	  {
	    int c2 = phase1_getc ();
	    if (c2 == '>')
	      {
		/* ?> and %> terminate PHP mode and switch back to HTML
		   mode.  */
		skip_html ();
	      }
	    else
	      phase1_ungetc (c2);
	    tp->type = token_type_other;
	    return;
	  }

	case '(':
	  tp->type = token_type_lparen;
	  return;

	case ')':
	  tp->type = token_type_rparen;
	  return;

	case ',':
	  tp->type = token_type_comma;
	  return;

	case '<':
	  {
	    int c2 = phase1_getc ();
	    if (c2 == '<')
	      {
		int c3 = phase1_getc ();
		if (c3 == '<')
		  {
		    /* Start of here document.
		       Parse whitespace, then label, then newline.  */
		    do
		      c = phase3_getc ();
		    while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

		    bufpos = 0;
		    do
		      {
			if (bufpos >= bufmax)
			  {
			    bufmax = 2 * bufmax + 10;
			    buffer = xrealloc (buffer, bufmax);
			  }
			buffer[bufpos++] = c;
			c = phase3_getc ();
		      }
		    while (c != EOF && c != '\n' && c != '\r');
		    /* buffer[0..bufpos-1] now contains the label.  */

		    /* Now skip the here document.  */
		    for (;;)
		      {
			c = phase1_getc ();
			if (c == EOF)
			  break;
			if (c == '\n' || c == '\r')
			  {
			    int bufidx = 0;

			    while (bufidx < bufpos)
			      {
				c = phase1_getc ();
				if (c == EOF)
				  break;
				if (c != buffer[bufidx])
				  {
				    phase1_ungetc (c);
				    break;
				  }
			      }
			    c = phase1_getc ();
			    if (c != ';')
			      phase1_ungetc (c);
			    c = phase1_getc ();
			    if (c == '\n' || c == '\r')
			      break;
			  }
		      }

		    /* FIXME: Ideally we should turn the here document into a
		       string literal if it didn't contain $ substitution.  And
		       we should also respect backslash escape sequences like
		       in double-quoted strings.  */
		    tp->type = token_type_other;
		    return;
		  }
		phase1_ungetc (c3);
	      }

	    /* < / script > terminates PHP mode and switches back to HTML
	       mode.  */
	    while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r')
	      c2 = phase1_getc ();
	    if (c2 == '/')
	      {
		do
		  c2 = phase1_getc ();
		while (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
		if (c2 == 's' || c2 == 'S')
		  {
		    c2 = phase1_getc ();
		    if (c2 == 'c' || c2 == 'C')
		      {
			c2 = phase1_getc ();
			if (c2 == 'r' || c2 == 'R')
			  {
			    c2 = phase1_getc ();
			    if (c2 == 'i' || c2 == 'I')
			      {
				c2 = phase1_getc ();
				if (c2 == 'p' || c2 == 'P')
				  {
				    c2 = phase1_getc ();
				    if (c2 == 't' || c2 == 'T')
				      {
					do
					  c2 = phase1_getc ();
					while (c2 == ' ' || c2 == '\t'
					       || c2 == '\n' || c2 == '\r');
					if (c2 == '>')
					  {
					    skip_html ();
					  }
					else
					  phase1_ungetc (c2);
				      }
				    else
				      phase1_ungetc (c2);
				  }
				else
				  phase1_ungetc (c2);
			      }
			    else
			      phase1_ungetc (c2);
			  }
			else
			  phase1_ungetc (c2);
		      }
		    else
		      phase1_ungetc (c2);
		  }
		else
		  phase1_ungetc (c2);
	      }
	    else
	      phase1_ungetc (c2);

	    tp->type = token_type_other;
	    return;
	  }

	case '`':
	  /* Execution operator.  */
	default:
	  /* We could carefully recognize each of the 2 and 3 character
	     operators, but it is not necessary, as we only need to recognize
	     gettext invocations.  Don't bother.  */
	  tp->type = token_type_other;
	  return;
	}
    }
}


/* ========================= Extracting strings.  ========================== */


/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;


/* The file is broken into tokens.  Scan the token stream, looking for
   a keyword, followed by a left paren, followed by a string.  When we
   see this sequence, we have something to remember.  We assume we are
   looking at a valid C or C++ program, and leave the complaints about
   the grammar to the compiler.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */


/* Extract messages until the next balanced closing parenthesis.
   Extracted messages are added to MLP.
   When a specific argument shall be extracted, COMMAS_TO_SKIP >= 0 and,
   if also a plural argument shall be extracted, PLURAL_COMMAS > 0,
   otherwise PLURAL_COMMAS = 0.
   When no specific argument shall be extracted, COMMAS_TO_SKIP < 0.
   Return true upon eof, false upon closing parenthesis.  */
static bool
extract_parenthesized (message_list_ty *mlp,
		       flag_context_ty outer_context,
		       flag_context_list_iterator_ty context_iter,
		       int commas_to_skip, int plural_commas)
{
  /* Remember the message containing the msgid, for msgid_plural.  */
  message_ty *plural_mp = NULL;

  /* 0 when no keyword has been seen.  1 right after a keyword is seen.  */
  int state;
  /* Parameters of the keyword just seen.  Defined only in state 1.  */
  int next_commas_to_skip = -1;
  int next_plural_commas = 0;
  /* Context iterator that will be used if the next token is a '('.  */
  flag_context_list_iterator_ty next_context_iter =
    passthrough_context_list_iterator;
  /* Current context.  */
  flag_context_ty inner_context =
    inherited_context (outer_context,
		       flag_context_list_iterator_advance (&context_iter));

  /* Start state is 0.  */
  state = 0;

  for (;;)
    {
      token_ty token;

      x_php_lex (&token);
      switch (token.type)
	{
	case token_type_symbol:
	  {
	    void *keyword_value;

	    if (find_entry (&keywords, token.string, strlen (token.string),
			    &keyword_value)
		== 0)
	      {
		int argnum1 = (int) (long) keyword_value & ((1 << 10) - 1);
		int argnum2 = (int) (long) keyword_value >> 10;

		next_commas_to_skip = argnum1 - 1;
		next_plural_commas = (argnum2 > argnum1 ? argnum2 - argnum1 : 0);
		state = 1;
	      }
	    else
	      state = 0;
	  }
	  next_context_iter =
	    flag_context_list_iterator (
	      flag_context_list_table_lookup (
		flag_context_list_table,
		token.string, strlen (token.string)));
	  free (token.string);
	  continue;

	case token_type_lparen:
	  if (extract_parenthesized (mlp, inner_context, next_context_iter,
				     state ? next_commas_to_skip : -1,
				     state ? next_plural_commas: 0))
	    return true;
	  next_context_iter = null_context_list_iterator;
	  state = 0;
	  continue;

	case token_type_rparen:
	  return false;

	case token_type_comma:
	  if (commas_to_skip >= 0)
	    {
	      if (commas_to_skip > 0)
		commas_to_skip--;
	      else
		if (plural_mp != NULL && plural_commas > 0)
		  {
		    commas_to_skip = plural_commas - 1;
		    plural_commas = 0;
		  }
		else
		  commas_to_skip = -1;
	    }
	  inner_context =
	    inherited_context (outer_context,
			       flag_context_list_iterator_advance (
				 &context_iter));
	  next_context_iter = passthrough_context_list_iterator;
	  state = 0;
	  continue;

	case token_type_string_literal:
	  {
	    lex_pos_ty pos;
	    pos.file_name = logical_file_name;
	    pos.line_number = token.line_number;

	    if (extract_all)
	      remember_a_message (mlp, token.string, inner_context, &pos);
	    else
	      {
		if (commas_to_skip == 0)
		  {
		    if (plural_mp == NULL)
		      {
			/* Seen an msgid.  */
			message_ty *mp =
			  remember_a_message (mlp, token.string,
					      inner_context, &pos);
			if (plural_commas > 0)
			  plural_mp = mp;
		      }
		    else
		      {
			/* Seen an msgid_plural.  */
			remember_a_message_plural (plural_mp, token.string,
						   inner_context, &pos);
			plural_mp = NULL;
		      }
		  }
		else
		  free (token.string);
	      }
	  }
	  next_context_iter = null_context_list_iterator;
	  state = 0;
	  continue;

	case token_type_other:
	  next_context_iter = null_context_list_iterator;
	  state = 0;
	  continue;

	case token_type_eof:
	  return true;

	default:
	  abort ();
	}
    }
}


void
extract_php (FILE *f,
	     const char *real_filename, const char *logical_filename,
	     flag_context_list_table_ty *flag_table,
	     msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  fp = f;
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  line_number = 1;

  last_comment_line = -1;
  last_non_comment_line = -1;

  flag_context_list_table = flag_table;

  init_keywords ();

  /* Initial mode is HTML mode, not PHP mode.  */
  skip_html ();

  /* Eat tokens until eof is seen.  When extract_parenthesized returns
     due to an unbalanced closing parenthesis, just restart it.  */
  while (!extract_parenthesized (mlp, null_context, null_context_list_iterator,
				 -1, 0))
    ;

  /* Close scanner.  */
  fp = NULL;
  real_file_name = NULL;
  logical_file_name = NULL;
  line_number = 0;
}
