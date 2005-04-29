/* xgettext Python backend.
   Copyright (C) 2002-2003 Free Software Foundation, Inc.

   This file was written by Bruno Haible <haible@clisp.cons.org>, 2002.

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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "xgettext.h"
#include "x-python.h"
#include "error.h"
#include "error-progname.h"
#include "xalloc.h"
#include "exit.h"
#include "po-charset.h"
#include "uniname.h"
#include "utf16-ucs4.h"
#include "ucs4-utf8.h"
#include "gettext.h"

#define _(s) gettext(s)

#define max(a,b) ((a) > (b) ? (a) : (b))

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* The Python syntax is defined in the Python Reference Manual
   /usr/share/doc/packages/python/html/ref/index.html.
   See also Python-2.0/Parser/tokenizer.c, Python-2.0/Python/compile.c,
   Python-2.0/Objects/unicodeobject.c.  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_python_extract_all ()
{
  extract_all = true;
}


void
x_python_keyword (const char *name)
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
      x_python_keyword ("gettext");
      x_python_keyword ("ugettext");
      x_python_keyword ("dgettext:2");
      x_python_keyword ("ngettext:1,2");
      x_python_keyword ("ungettext:1,2");
      x_python_keyword ("dngettext:2,3");
      x_python_keyword ("_");
      default_keywords = false;
    }
}

void
init_flag_table_python ()
{
  xgettext_record_flag ("gettext:1:pass-python-format");
  xgettext_record_flag ("ugettext:1:pass-python-format");
  xgettext_record_flag ("dgettext:2:pass-python-format");
  xgettext_record_flag ("ngettext:1:pass-python-format");
  xgettext_record_flag ("ngettext:2:pass-python-format");
  xgettext_record_flag ("ungettext:1:pass-python-format");
  xgettext_record_flag ("ungettext:2:pass-python-format");
  xgettext_record_flag ("dngettext:2:pass-python-format");
  xgettext_record_flag ("dngettext:3:pass-python-format");
  xgettext_record_flag ("_:1:pass-python-format");
  /* xgettext_record_flag ("%:1:python-format"); // % is an infix operator! */
}


/* ======================== Reading of characters.  ======================== */

/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* Logical filename and line number, used to label the extracted messages.  */
static char *logical_file_name;
static int line_number;

/* The input file stream.  */
static FILE *fp;


/* 1. line_number handling.  Also allow a lookahead.  */

static unsigned char phase1_pushback[max (9, UNINAME_MAX + 3)];
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

/* Supports max (9, UNINAME_MAX + 3) characters of pushback.  */
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
  /* We assume the program source is in ISO-8859-1 (for consistency with
     Python's \ooo and \xnn syntax inside strings), but we produce a POT
     file in UTF-8 encoding.  */
  size_t len = ((unsigned char) c < 0x80 ? 1 : 2);
  if (buflen + len > bufmax)
    {
      bufmax = 2 * bufmax + 10;
      buffer = xrealloc (buffer, bufmax);
    }
  if ((unsigned char) c < 0x80)
    buffer[buflen++] = c;
  else
    {
      buffer[buflen++] = 0xc0 | ((unsigned char) c >> 6);
      buffer[buflen++] = 0x80 | ((unsigned char) c & 0x3f);
    }
}

static inline void
comment_line_end ()
{
  while (buflen >= 1
	 && (buffer[buflen - 1] == ' ' || buffer[buflen - 1] == '\t'))
    --buflen;
  if (buflen >= bufmax)
    {
      bufmax = 2 * bufmax + 10;
      buffer = xrealloc (buffer, bufmax);
    }
  buffer[buflen] = '\0';
  savable_comment_add (buffer);
}

/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;


/* 2. Outside strings, replace backslash-newline with nothing and a comment
      with nothing.  */

static int
phase2_getc ()
{
  int c;

  for (;;)
    {
      c = phase1_getc ();
      if (c == '\\')
	{
	  c = phase1_getc ();
	  if (c != '\n')
	    {
	      phase1_ungetc (c);
	      /* This shouldn't happen usually, because "A backslash is
		 illegal elsewhere on a line outside a string literal."  */
	      return '\\';
	    }
	  /* Eat backslash-newline.  */
	}
      else if (c == '#')
	{
	  /* Eat a comment.  */
	  last_comment_line = line_number;
	  comment_start ();
	  for (;;)
	    {
	      c = phase1_getc ();
	      if (c == EOF || c == '\n')
		break;
	      /* We skip all leading white space, but not EOLs.  */
	      if (!(buflen == 0 && (c == ' ' || c == '\t')))
		comment_add (c);
	    }
	  comment_line_end ();
	  return c;
	}
      else
	return c;
    }
}

/* Supports only one pushback character.  */
static void
phase2_ungetc (int c)
{
  phase1_ungetc (c);
}


/* ========================== Reading of tokens.  ========================== */


enum token_type_ty
{
  token_type_eof,
  token_type_lparen,		/* ( */
  token_type_rparen,		/* ) */
  token_type_comma,		/* , */
  token_type_string,		/* "abc", 'abc', """abc""", '''abc''' */
  token_type_symbol,		/* symbol, number */
  token_type_other		/* misc. operator */
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;
struct token_ty
{
  token_type_ty type;
  char *string;		/* for token_type_string, token_type_symbol */
  refcounted_string_list_ty *comment;	/* for token_type_string */
  int line_number;
};


/* There are two different input syntaxes for strings, "abc" and r"abc",
   and two different input syntaxes for Unicode strings, u"abc" and ur"abc".
   Which escape sequences are understood, i.e. what is interpreted specially
   after backslash?
    "abc"     \<nl> \\ \' \" \a\b\f\n\r\t\v \ooo \xnn
    r"abc"
    u"abc"    \<nl> \\ \' \" \a\b\f\n\r\t\v \ooo \xnn \unnnn \Unnnnnnnn \N{...}
    ur"abc"                                           \unnnn
   The \unnnn values are UTF-16 values; a single \Unnnnnnnn can expand to two
   \unnnn items.  The \ooo and \xnn values are ISO-8859-1 values: u"\xff" and
   u"\u00ff" are the same.  */

#define P7_EOF (-1)
#define P7_STRING_END (-2)

static int
phase7_getuc (int quote_char,
	      bool triple, bool interpret_ansic, bool interpret_unicode,
	      unsigned int *backslash_counter)
{
  int c;

  for (;;)
    {
      /* Use phase 1, because phase 2 elides comments.  */
      c = phase1_getc ();

      if (c == EOF)
	return P7_EOF;

      if (c == quote_char && (interpret_ansic || (*backslash_counter & 1) == 0))
	{
	  if (triple)
	    {
	      int c1 = phase1_getc ();
	      if (c1 == quote_char)
		{
		  int c2 = phase1_getc ();
		  if (c2 == quote_char)
		    return P7_STRING_END;
		  phase1_ungetc (c2);
		}
	      phase1_ungetc (c1);
	      return c;
	    }
	  else
	    return P7_STRING_END;
	}

      if (c == '\n')
	{
	  if (triple)
	    {
	      *backslash_counter = 0;
	      return '\n';
	    }
	  /* In r"..." and ur"..." strings, newline is only allowed
	     immediately after an odd number of backslashes (although the
	     backslashes are not interpreted!).  */
	  if (!(interpret_ansic || (*backslash_counter & 1) == 0))
	    {
	      *backslash_counter = 0;
	      return '\n';
	    }
	  phase1_ungetc (c);
	  error_with_progname = false;
	  error (0, 0, _("%s:%d: warning: unterminated string"),
		 logical_file_name, line_number);
	  error_with_progname = true;
	  return P7_STRING_END;
	}

      if (c != '\\')
	{
	  *backslash_counter = 0;
	  return c;
	}

      /* Backslash handling.  */

      if (!interpret_ansic && !interpret_unicode)
	{
	  ++*backslash_counter;
	  return '\\';
	}

      /* Dispatch according to the character following the backslash.  */
      c = phase1_getc ();
      if (c == EOF)
	{
	  ++*backslash_counter;
	  return '\\';
	}

      if (interpret_ansic)
	switch (c)
	  {
	  case '\n':
	    continue;
	  case '\\':
	    ++*backslash_counter;
	    return c;
	  case '\'': case '"':
	    *backslash_counter = 0;
	    return c;
	  case 'a':
	    *backslash_counter = 0;
	    return '\a';
	  case 'b':
	    *backslash_counter = 0;
	    return '\b';
	  case 'f':
	    *backslash_counter = 0;
	    return '\f';
	  case 'n':
	    *backslash_counter = 0;
	    return '\n';
	  case 'r':
	    *backslash_counter = 0;
	    return '\r';
	  case 't':
	    *backslash_counter = 0;
	    return '\t';
	  case 'v':
	    *backslash_counter = 0;
	    return '\v';
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7':
	    {
	      int n = c - '0';

	      c = phase1_getc ();
	      if (c != EOF)
		{
		  if (c >= '0' && c <= '7')
		    {
		      n = (n << 3) + (c - '0');
		      c = phase1_getc ();
		      if (c != EOF)
			{
			  if (c >= '0' && c <= '7')
			    n = (n << 3) + (c - '0');
			  else
			    phase1_ungetc (c);
			}
		    }
		  else
		    phase1_ungetc (c);
		}
	      *backslash_counter = 0;
	      return (unsigned char) n;
	    }
	  case 'x':
	    {
	      int c1 = phase1_getc ();
	      int n1;

	      if (c1 >= '0' && c1 <= '9')
		n1 = c1 - '0';
	      else if (c1 >= 'A' && c1 <= 'F')
		n1 = c1 - 'A' + 10;
	      else if (c1 >= 'a' && c1 <= 'f')
		n1 = c1 - 'a' + 10;
	      else
		n1 = -1;

	      if (n1 >= 0)
		{
		  int c2 = phase1_getc ();
		  int n2;

		  if (c2 >= '0' && c2 <= '9')
		    n2 = c2 - '0';
		  else if (c2 >= 'A' && c2 <= 'F')
		    n2 = c2 - 'A' + 10;
		  else if (c2 >= 'a' && c2 <= 'f')
		    n2 = c2 - 'a' + 10;
		  else
		    n2 = -1;

		  if (n2 >= 0)
		    {
		      *backslash_counter = 0;
		      return (unsigned char) ((n1 << 4) + n2);
		    }

		  phase1_ungetc (c2);
		}
	      phase1_ungetc (c1);
	      phase1_ungetc (c);
	      ++*backslash_counter;
	      return '\\';
	    }
	  }

      if (interpret_unicode)
	{
	  if (c == 'u')
	    {
	      unsigned char buf[4];
	      unsigned int n = 0;
	      int i;

	      for (i = 0; i < 4; i++)
		{
		  int c1 = phase1_getc ();

		  if (c1 >= '0' && c1 <= '9')
		    n = (n << 4) + (c1 - '0');
		  else if (c1 >= 'A' && c1 <= 'F')
		    n = (n << 4) + (c1 - 'A' + 10);
		  else if (c1 >= 'a' && c1 <= 'f')
		    n = (n << 4) + (c1 - 'a' + 10);
		  else
		    {
		      phase1_ungetc (c1);
		      while (--i >= 0)
			phase1_ungetc (buf[i]);
		      phase1_ungetc (c);
		      ++*backslash_counter;
		      return '\\';
		    }

		  buf[i] = c1;
		}
	      *backslash_counter = 0;
	      return n;
	    }

	  if (interpret_ansic)
	    {
	      if (c == 'U')
		{
		  unsigned char buf[8];
		  unsigned int n = 0;
		  int i;

		  for (i = 0; i < 8; i++)
		    {
		      int c1 = phase1_getc ();

		      if (c1 >= '0' && c1 <= '9')
			n = (n << 4) + (c1 - '0');
		      else if (c1 >= 'A' && c1 <= 'F')
			n = (n << 4) + (c1 - 'A' + 10);
		      else if (c1 >= 'a' && c1 <= 'f')
			n = (n << 4) + (c1 - 'a' + 10);
		      else
			{
			  phase1_ungetc (c1);
			  while (--i >= 0)
			    phase1_ungetc (buf[i]);
			  phase1_ungetc (c);
			  ++*backslash_counter;
			  return '\\';
			}

		      buf[i] = c1;
		    }
		  if (n < 0x110000)
		    {
		      *backslash_counter = 0;
		      return n;
		    }

		  error_with_progname = false;
		  error (0, 0, _("%s:%d: warning: invalid Unicode character"),
			 logical_file_name, line_number);
		  error_with_progname = true;

		  while (--i >= 0)
		    phase1_ungetc (buf[i]);
		  phase1_ungetc (c);
		  ++*backslash_counter;
		  return '\\';
		}

	      if (c == 'N')
		{
		  int c1 = phase1_getc ();
		  if (c1 == '{')
		    {
		      unsigned char buf[UNINAME_MAX + 1];
		      int i;
		      unsigned int n;

		      for (i = 0; i < UNINAME_MAX; i++)
			{
			  int c2 = phase1_getc ();
			  if (!(c2 >= ' ' && c2 <= '~'))
			    {
			      phase1_ungetc (c2);
			      while (--i >= 0)
				phase1_ungetc (buf[i]);
			      phase1_ungetc (c1);
			      phase1_ungetc (c);
			      ++*backslash_counter;
			      return '\\';
			    }
			  if (c2 == '}')
			    break;
			  buf[i] = c2;
			}
		      buf[i] = '\0';

		      n = unicode_name_character ((char *) buf);
		      if (n != UNINAME_INVALID)
			{
			  *backslash_counter = 0;
			  return n;
			}

		      phase1_ungetc ('}');
		      while (--i >= 0)
			phase1_ungetc (buf[i]);
		    }
		  phase1_ungetc (c1);
		  phase1_ungetc (c);
		  ++*backslash_counter;
		  return '\\';
		}
	    }
	}

      phase1_ungetc (c);
      ++*backslash_counter;
      return '\\';
    }
}


/* Combine characters into tokens.  Discard whitespace except newlines at
   the end of logical lines.  */

/* Number of pending open parentheses/braces/brackets.  */
static int open_pbb;

static token_ty phase5_pushback[1];
static int phase5_pushback_length;

static void
phase5_get (token_ty *tp)
{
  int c;

  if (phase5_pushback_length)
    {
      *tp = phase5_pushback[--phase5_pushback_length];
      return;
    }

  for (;;)
    {
      tp->line_number = line_number;
      c = phase2_getc ();

      switch (c)
	{
	case EOF:
	  tp->type = token_type_eof;
	  return;

	case ' ':
	case '\t':
	case '\f':
	  /* Ignore whitespace and comments.  */
	  continue;

	case '\n':
	  if (last_non_comment_line > last_comment_line)
	    savable_comment_reset ();
	  /* Ignore newline if and only if it is used for implicit line
	     joining.  */
	  if (open_pbb > 0)
	    continue;
	  tp->type = token_type_other;
	  return;
	}

      last_non_comment_line = tp->line_number;

      switch (c)
	{
	case '.':
	  {
	    int c1 = phase2_getc ();
	    phase2_ungetc (c1);
	    if (!(c1 >= '0' && c1 <= '9'))
	      {

		tp->type = token_type_other;
		return;
	      }
	  }
	  /* FALLTHROUGH */
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q':
	case 'S': case 'T':           case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
	case '_':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p': case 'q':
	case 's': case 't':           case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	symbol:
	  /* Symbol, or part of a number.  */
	  {
	    static char *buffer;
	    static int bufmax;
	    int bufpos;

	    bufpos = 0;
	    for (;;)
	      {
		if (bufpos >= bufmax)
		  {
		    bufmax = 2 * bufmax + 10;
		    buffer = xrealloc (buffer, bufmax);
		  }
		buffer[bufpos++] = c;
		c = phase2_getc ();
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
		    phase2_ungetc (c);
		    break;
		  }
		break;
	      }
	    if (bufpos >= bufmax)
	      {
		bufmax = 2 * bufmax + 10;
		buffer = xrealloc (buffer, bufmax);
	      }
	    buffer[bufpos] = '\0';
	    tp->string = xstrdup (buffer);
	    tp->type = token_type_symbol;
	    return;
	  }

	/* Strings.  */
	  {
	    static unsigned short *buffer;
	    static int bufmax;
	    int bufpos;
	    int quote_char;
	    bool interpret_ansic;
	    bool interpret_unicode;
	    bool triple;
	    unsigned int backslash_counter;

	    case 'R': case 'r':
	      {
		int c1 = phase1_getc ();
		if (c1 == '"' || c1 == '\'')
		  {
		    quote_char = c1;
		    interpret_ansic = false;
		    interpret_unicode = false;
		    goto string;
		  }
		phase1_ungetc (c1);
		goto symbol;
	      }

	    case 'U': case 'u':
	      {
		int c1 = phase1_getc ();
		if (c1 == '"' || c1 == '\'')
		  {
		    quote_char = c1;
		    interpret_ansic = true;
		    interpret_unicode = true;
		    goto string;
		  }
		if (c1 == 'R' || c1 == 'r')
		  {
		    int c2 = phase1_getc ();
		    if (c2 == '"' || c2 == '\'')
		      {
			quote_char = c2;
			interpret_ansic = false;
			interpret_unicode = true;
			goto string;
		      }
		    phase1_ungetc (c2);
		  }
		phase1_ungetc (c1);
		goto symbol;
	      }

	    case '"': case '\'':
	      quote_char = c;
	      interpret_ansic = true;
	      interpret_unicode = false;
	    string:
	      triple = false;
	      {
		int c1 = phase1_getc ();
		if (c1 == quote_char)
		  {
		    int c2 = phase1_getc ();
		    if (c2 == quote_char)
		      triple = true;
		    else
		      {
			phase1_ungetc (c2);
			phase1_ungetc (c1);
		      }
		  }
		else
		  phase1_ungetc (c1);
	      }
	      backslash_counter = 0;
	      /* Start accumulating the string.  We store the string in
		 UTF-16 before converting it to UTF-8.  Why not converting
		 every character directly to UTF-8? Because a string can
		 contain surrogates like u"\uD800\uDF00", and we must
		 combine them to a single UTF-8 character.  */
	      bufpos = 0;
	      for (;;)
		{
		  int uc = phase7_getuc (quote_char, triple, interpret_ansic,
					 interpret_unicode, &backslash_counter);
		  unsigned int len;

		  if (uc == P7_EOF || uc == P7_STRING_END)
		    break;

		  assert (uc >= 0 && uc < 0x110000);
		  len = (uc < 0x10000 ? 1 : 2);
		  if (bufpos + len > bufmax)
		    {
		      bufmax = 2 * bufmax + 10;
		      buffer =
			xrealloc (buffer, bufmax * sizeof (unsigned short));
		    }
		  if (uc < 0x10000)
		    buffer[bufpos++] = uc;
		  else
		    {
		      buffer[bufpos++] = 0xd800 + ((uc - 0x10000) >> 10);
		      buffer[bufpos++] = 0xdc00 + ((uc - 0x10000) & 0x3ff);
		    }
		}
	      /* Now convert from UTF-16 to UTF-8.  */
	      {
		int pos;
		unsigned char *utf8_string;
		unsigned char *q;

		/* Each UTF-16 word needs 3 bytes at worst.  */
		utf8_string = (unsigned char *) xmalloc (3 * bufpos + 1);
		for (pos = 0, q = utf8_string; pos < bufpos; )
		  {
		    unsigned int uc;
		    int n;

		    pos += u16_mbtouc (&uc, buffer + pos, bufpos - pos);
		    n = u8_uctomb (q, uc, 6);
		    assert (n > 0);
		    q += n;
		  }
		*q = '\0';
		assert (q - utf8_string <= 3 * bufpos);
		tp->string = (char *) utf8_string;
	      }
	      tp->comment = add_reference (savable_comment);
	      tp->type = token_type_string;
	      return;
	  }

	case '(':
	  open_pbb++;
	  tp->type = token_type_lparen;
	  return;

	case ')':
	  if (open_pbb > 0)
	    open_pbb--;
	  tp->type = token_type_rparen;
	  return;

	case ',':
	  tp->type = token_type_comma;
	  return;

	case '[': case '{':
	  open_pbb++;
	  tp->type = token_type_other;
	  return;

	case ']': case '}':
	  if (open_pbb > 0)
	    open_pbb--;
	  tp->type = token_type_other;
	  return;

	default:
	  /* We could carefully recognize each of the 2 and 3 character
	     operators, but it is not necessary, as we only need to recognize
	     gettext invocations.  Don't bother.  */
	  tp->type = token_type_other;
	  return;
	}
    }
}

/* Supports only one pushback token.  */
static void
phase5_unget (token_ty *tp)
{
  if (tp->type != token_type_eof)
    {
      if (phase5_pushback_length == SIZEOF (phase5_pushback))
	abort ();
      phase5_pushback[phase5_pushback_length++] = *tp;
    }
}


/* Combine adjacent strings to form a single string.  Note that the end
   of a logical line appears as a token of its own, therefore strings that
   belong to different logical lines will not be concatenated.  */

static void
x_python_lex (token_ty *tp)
{
  phase5_get (tp);
  if (tp->type != token_type_string)
    return;
  for (;;)
    {
      token_ty tmp;
      size_t len;

      phase5_get (&tmp);
      if (tmp.type != token_type_string)
	{
	  phase5_unget (&tmp);
	  return;
	}
      len = strlen (tp->string);
      tp->string = xrealloc (tp->string, len + strlen (tmp.string) + 1);
      strcpy (tp->string + len, tmp.string);
      free (tmp.string);
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

      x_python_lex (&token);
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
				     state ? next_plural_commas : 0))
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

	case token_type_string:
	  {
	    lex_pos_ty pos;
	    pos.file_name = logical_file_name;
	    pos.line_number = token.line_number;

	    if (extract_all)
	      {
		savable_comment_to_xgettext_comment (token.comment);
		remember_a_message (mlp, token.string, inner_context, &pos);
		savable_comment_reset ();
	      }
	    else
	      {
		if (commas_to_skip == 0)
		  {
		    if (plural_mp == NULL)
		      {
			/* Seen an msgid.  */
			message_ty *mp;

			savable_comment_to_xgettext_comment (token.comment);
			mp = remember_a_message (mlp, token.string,
						inner_context, &pos);
			savable_comment_reset ();
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
	  drop_reference (token.comment);
	  next_context_iter = null_context_list_iterator;
	  state = 0;
	  continue;

	case token_type_eof:
	  return true;

	case token_type_other:
	  next_context_iter = null_context_list_iterator;
	  state = 0;
	  continue;

	default:
	  abort ();
	}
    }
}


void
extract_python (FILE *f,
		const char *real_filename, const char *logical_filename,
		flag_context_list_table_ty *flag_table,
		msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  /* We convert our strings to UTF-8 encoding.  */
  xgettext_current_source_encoding = po_charset_utf8;

  fp = f;
  real_file_name = real_filename;
  logical_file_name = xstrdup (logical_filename);
  line_number = 1;

  last_comment_line = -1;
  last_non_comment_line = -1;

  open_pbb = 0;

  flag_context_list_table = flag_table;

  init_keywords ();

  /* Eat tokens until eof is seen.  When extract_parenthesized returns
     due to an unbalanced closing parenthesis, just restart it.  */
  while (!extract_parenthesized (mlp, null_context, null_context_list_iterator,
				 -1, 0))
    ;

  fp = NULL;
  real_file_name = NULL;
  logical_file_name = NULL;
  line_number = 0;
}
