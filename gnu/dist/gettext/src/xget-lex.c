/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#include "dir-list.h"
#include "error.h"
#include "system.h"
#include "libgettext.h"
#include "str-list.h"
#include "xget-lex.h"

#ifndef errno
extern int errno;
#endif

#define _(s) gettext(s)


/* The ANSI C standard defines several phases of translation:

   1. Terminate line by \n, regardless of the external representation
      of a text line.  Stdio does this for us.

   2. Convert trigraphs to their single character equivalents.

   3. Concatenate each line ending in backslash (\) with the following
      line.

   4. Replace each comment with a space character.

   5. Parse each resulting logical line as preprocessing tokens a
      white space.

   6. Recognize and carry out directives (it also expands macros on
      non-directive lines, which we do not do here).

   7. Replaces escape sequences within character strings with their
      single character equivalents (we do this in step 5, because we
      don't have to worry about the #include argument).

   8. Concatenates adjacent string literals to form single string
      literals (because we don't expand macros, there are a few things
      we will miss).

   9. Converts the remaining preprocessing tokens to C tokens and
      discards any white space from the translation unit.

   This lexer implements the above, and presents the scanner (in
   xgettext.c) with a stream of C tokens.  The comments are
   accumulated in a buffer, and given to xgettext when asked for.  */

enum token_type_ty
{
  token_type_character_constant,
  token_type_eof,
  token_type_eoln,
  token_type_hash,
  token_type_lp,
  token_type_comma,
  token_type_name,
  token_type_number,
  token_type_string_literal,
  token_type_symbol,
  token_type_white_space
};
typedef enum token_type_ty token_type_ty;

typedef struct token_ty token_ty;
struct token_ty
{
  token_type_ty type;
  char *string;
  long number;
  int line_number;
};


static const char *file_name;
static char *logical_file_name;
static int line_number;
static FILE *fp;
static int trigraphs;
static int cplusplus_comments;
static string_list_ty *comment;
static string_list_ty *keywords;
static int default_keywords = 1;

/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line = -1;
static int last_non_comment_line = -1;
static int newline_count = 0;


/* Prototypes for local functions.  */
static int phase1_getc PARAMS ((void));
static void phase1_ungetc PARAMS ((int __c));
static int phase2_getc PARAMS ((void));
static void phase2_ungetc PARAMS ((int __c));
static int phase3_getc PARAMS ((void));
static void phase3_ungetc PARAMS ((int __c));
static int phase4_getc PARAMS ((void));
static void phase4_ungetc PARAMS ((int __c));
static int phase7_getc PARAMS ((void));
static void phase7_ungetc PARAMS ((int __c));
static void phase5_get PARAMS ((token_ty *__tp));
static void phase5_unget PARAMS ((token_ty *__tp));
static void phaseX_get PARAMS ((token_ty *__tp));
static void phase6_get PARAMS ((token_ty *__tp));
static void phase6_unget PARAMS ((token_ty *__tp));
static void phase8_get PARAMS ((token_ty *__tp));



void
xgettext_lex_open (fn)
     const char *fn;
{
  char *new_name;

  if (strcmp (fn, "-") == 0)
    {
      new_name = xstrdup (_("standard input"));
      logical_file_name = xstrdup (new_name);
      fp = stdin;
    }
  else if (*fn == '/')
    {
      new_name = xstrdup (fn);
      fp = fopen (fn, "r");
      if (fp == NULL)
	error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), fn);
      logical_file_name = xstrdup (new_name);
    }
  else
    {
      size_t len1, len2;
      int j;
      const char *dir;

      len2 = strlen (fn);
      for (j = 0; ; ++j)
	{
	  dir = dir_list_nth (j);
	  if (dir == NULL)
	    error (EXIT_FAILURE, ENOENT, _("\
error while opening \"%s\" for reading"), fn);

	  if (dir[0] =='.' && dir[1] == '\0')
	    new_name = xstrdup (fn);
	  else
	    {
	      len1 = strlen (dir);
	      new_name = xmalloc (len1 + len2 + 2);
	      stpcpy (stpcpy (stpcpy (new_name, dir), "/"), fn);
	    }

	  fp = fopen (new_name, "r");
	  if (fp != NULL)
	    break;

	  if (errno != ENOENT)
	    error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), new_name);
	  free (new_name);
	}

      /* Note that the NEW_NAME variable contains the actual file name
	 and the logical file name is what is reported by xgettext.  In
	 this case NEW_NAME is set to the file which was found along the
	 directory search path, and LOGICAL_FILE_NAME is is set to the
	 file name which was searched for.  */
      logical_file_name = xstrdup (fn);
    }

  file_name = new_name;
  line_number = 1;
}


void
xgettext_lex_close ()
{
  if (fp != stdin)
    fclose (fp);
  free ((char *) file_name);
  free (logical_file_name);
  fp = NULL;
  file_name = NULL;
  logical_file_name = NULL;
  line_number = 0;
}


/* 1. Terminate line by \n, regardless of the external representation of
   a text line.  Stdio does this for us, we just need to check that
   there are no I/O errors, and cope with potentially 2 characters of
   pushback, not just the one that ungetc can cope with.  */

/* Maximum used guaranteed to be < 4.  */
static unsigned char phase1_pushback[4];
static int phase1_pushback_length;


static int
phase1_getc ()
{
  int c;

  if (phase1_pushback_length)
    {
      c = phase1_pushback[--phase1_pushback_length];
      if (c == '\n')
	++line_number;
      return c;
    }
  while (1)
    {
      c = getc (fp);
      switch (c)
	{
	case EOF:
	  if (ferror (fp))
	    {
	    bomb:
	      error (EXIT_FAILURE, errno, _("\
error while reading \"%s\""), file_name);
	    }
	  return EOF;

	case '\n':
	  ++line_number;
	  return '\n';

	case '\\':
	  c = getc (fp);
	  if (c == EOF)
	    {
	      if (ferror (fp))
		goto bomb;
	      return '\\';
	    }
	  if (c != '\n')
	    {
	      ungetc (c, fp);
	      return '\\';
	    }
	  ++line_number;
	  break;

	default:
	  return c;
	}
    }
}


static void
phase1_ungetc (c)
     int c;
{
  switch (c)
    {
    case EOF:
      break;

    case '\n':
      --line_number;
      /* FALLTHROUGH */

    default:
      phase1_pushback[phase1_pushback_length++] = c;
      break;
    }
}


/* 2. Convert trigraphs to their single character equivalents.  Most
   sane human beings vomit copiously at the mention of trigraphs, which
   is why they are on option.  */

/* Maximum used guaranteed to be < 4.  */
static unsigned char phase2_pushback[4];
static int phase2_pushback_length;


static int
phase2_getc ()
{
  int c;

  if (phase2_pushback_length)
    return phase2_pushback[--phase2_pushback_length];
  if (!trigraphs)
    return phase1_getc ();

  c = phase1_getc ();
  if (c != '?')
    return c;
  c = phase1_getc ();
  if (c != '?')
    {
      phase1_ungetc (c);
      return '?';
    }
  c = phase1_getc ();
  switch (c)
    {
    case '(':
      return '[';
    case '/':
      return '\\';
    case ')':
      return ']';
    case '\'':
      return '^';
    case '<':
      return '{';
    case '!':
      return '|';
    case '>':
      return '}';
    case '-':
      return '~';
    case '#':
      return '=';
    }
  phase1_ungetc (c);
  phase1_ungetc ('?');
  return '?';
}


static void
phase2_ungetc (c)
     int c;
{
  if (c != EOF)
    phase2_pushback[phase2_pushback_length++] = c;
}


/* 3. Concatenate each line ending in backslash (\) with the following
   line.  Basically, all you need to do is elide "\\\n" sequences from
   the input.  */

/* Maximum used guaranteed to be < 4.  */
static unsigned char phase3_pushback[4];
static int phase3_pushback_length;


static int
phase3_getc ()
{
  if (phase3_pushback_length)
    return phase3_pushback[--phase3_pushback_length];
  for (;;)
    {
      int c = phase2_getc ();
      if (c != '\\')
	return c;
      c = phase2_getc ();
      if (c != '\n')
	{
	  phase2_ungetc (c);
	  return '\\';
	}
    }
}


static void
phase3_ungetc (c)
     int c;
{
  if (c != EOF)
    phase3_pushback[phase3_pushback_length++] = c;
}


/* 4. Replace each comment that is not inside a character constant or
   string literal with a space character.  We need to remember the
   comment for later, because it may be attached to a keyword string.
   We also optionally understand C++ comments.  */

static int
phase4_getc ()
{
  static char *buffer;
  static size_t bufmax;
  size_t buflen;
  int c;
  int state;

  c = phase3_getc ();
  if (c != '/')
    return c;
  c = phase3_getc ();
  switch (c)
    {
    default:
      phase3_ungetc (c);
      return '/';

    case '*':
      /* C comment.  */
      buflen = 0;
      state = 0;
      if (comment == NULL)
	comment = string_list_alloc ();
      while (1)
	{
	  c = phase3_getc ();
	  if (c == EOF)
	    break;
	  /* We skip all leading white space, but not EOLs.  */
	  if (buflen == 0 && isspace (c) && c != '\n')
	    continue;
	  if (buflen >= bufmax)
	    {
	      bufmax += 100;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[buflen++] = c;
	  switch (c)
	    {
	    case '\n':
	      --buflen;
	      while (buflen >= 1 && (buffer[buflen - 1] == ' '
				     || buffer[buflen - 1] == '\t'))
		--buflen;
	      buffer[buflen] = 0;
	      string_list_append (comment, buffer);
	      buflen = 0;
	      state = 0;
	      continue;

	    case '*':
	      state = 1;
	      continue;

	    case '/':
	      if (state == 1)
		{
		  buflen -= 2;
		  while (buflen >= 1 && (buffer[buflen - 1] == ' '
					 || buffer[buflen - 1] == '\t'))
		    --buflen;
		  buffer[buflen] = 0;
		  string_list_append (comment, buffer);
		  break;
		}
	      /* FALLTHROUGH */

	    default:
	      state = 0;
	      continue;
	    }
	  break;
	}
      last_comment_line = newline_count;
      return ' ';

    case '/':
      /* C++ comment.  */
      if (!cplusplus_comments)
	{
	  phase3_ungetc ('/');
	  return '/';
	}
      buflen = 0;
      while (1)
	{
	  c = phase3_getc ();
	  if (c == '\n' || c == EOF)
	    break;
	  if (buflen >= bufmax)
	    {
	      bufmax += 100;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[buflen++] = c;
	}
      if (buflen >= bufmax)
	{
	  bufmax += 100;
	  buffer = xrealloc (buffer, bufmax);
	}
      buffer[buflen] = 0;
      if (comment == NULL)
	comment = string_list_alloc ();
      string_list_append (comment, buffer);
      last_comment_line = newline_count;
      return '\n';
    }
}


static void
phase4_ungetc (c)
     int c;
{
  phase3_ungetc (c);
}


/* 7. Replace escape sequences within character strings with their
   single character equivalents.  This is called from phase 5, because
   we don't have to worry about the #include argument.  There are
   pathological cases which could bite us (like the DOS directory
   separator), but just pretend it can't happen.  */

#define P7_QUOTES (1000 + '"')
#define P7_QUOTE (1000 + '\'')
#define P7_NEWLINE (1000 + '\n')

static int
phase7_getc ()
{
  int c, n, j;

  /* Use phase 3, because phase 4 elides comments.  */
  c = phase3_getc ();

  /* Return a magic newline indicator, so that we can distinguish
     between the user requesting a newline in the string (e.g. using
     "\n" or "\15") from the user failing to terminate the string or
     character constant.  The ANSI C standard says: 3.1.3.4 Character
     Constants contain ``any character except single quote, backslash or
     newline; or an escape sequence'' and 3.1.4 String Literals contain
     ``any character except double quote, backslash or newline; or an
     escape sequence''.

     Most compilers give a fatal error in this case, however gcc is
     stupidly silent, even though this is a very common typo.  OK, so
     gcc --pedantic will tell me, but that gripes about too much other
     stuff.  Could I have a ``gcc -Wnewline-in-string'' option, or
     better yet a ``gcc -fno-newline-in-string'' option, please?  Gcc is
     also inconsistent between string literals and character constants:
     you may not embed newlines in character constants; try it, you get
     a useful diagnostic.  --PMiller  */
  if (c == '\n')
    return P7_NEWLINE;

  if (c == '"')
    return P7_QUOTES;
  if (c == '\'')
    return P7_QUOTE;
  if (c != '\\')
    return c;
  c = phase3_getc ();
  switch (c)
    {
    default:
      /* Unknown escape sequences really should be an error, but just
	 ignore them, and let the real compiler complain.  */
      phase3_ungetc (c);
      return '\\';

    case '"':
    case '\'':
    case '?':
    case '\\':
      return c;

      /* The \a and \v escapes were added by the ANSI C Standard.
	 Prior to the Standard, most compilers did not have them.
	 Because we need the same program on all platforms we don't
	 provide support for them here.

	 The gcc sources comment that \a is commonly available in
	 pre-ANSI compilers.  --PMiller  */

    case 'b':
      return '\b';

      /* The \e escape is preculiar to gcc, and assumes an ASCII
         character set (or superset).  We don't provide support for it
         here.  */

    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';

    case 'x':
      c = phase3_getc ();
      switch (c)
	{
	default:
	  phase3_ungetc (c);
	  phase3_ungetc ('x');
	  return '\\';

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	  break;
	}
      n = 0;
      for (;;)
	{
	  switch (c)
	    {
	    default:
	      phase3_ungetc (c);
	      return n;
	      break;

	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      n = n * 16 + c - '0';
	      break;;

	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	      n = n * 16 + 10 + c - 'A';
	      break;

	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	      n = n * 16 + 10 + c - 'a';
	      break;
	    }
	  c = phase3_getc ();
	}
      return n;

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      n = 0;
      for (j = 0; j < 3; ++j)
	{
	  n = n * 8 + c - '0';
	  c = phase3_getc ();
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
      phase3_ungetc (c);
      return n;
    }
}


static void
phase7_ungetc (c)
     int c;
{
  phase3_ungetc (c);
}


/* 5. Parse each resulting logical line as preprocessing tokens and
   white space.  Preprocessing tokens and C tokens don't always match.  */

/* Maximum used guaranteed to be < 4.  */
static token_ty phase5_pushback[4];
static int phase5_pushback_length;


static void
phase5_get (tp)
     token_ty *tp;
{
  static char *buffer;
  static int bufmax;
  int bufpos;
  int c;

  if (phase5_pushback_length)
    {
      *tp = phase5_pushback[--phase5_pushback_length];
      return;
    }
  tp->string = 0;
  tp->number = 0;
  tp->line_number = line_number;
  c = phase4_getc ();
  switch (c)
    {
    case EOF:
      tp->type = token_type_eof;
      return;

    case '\n':
      tp->type = token_type_eoln;
      return;

    case ' ':
    case '\f':
    case '\t':
      for (;;)
	{
	  c = phase4_getc ();
	  switch (c)
	    {
	    case ' ':
	    case '\f':
	    case '\t':
	      continue;

	    default:
	      phase4_ungetc (c);
	      break;
	    }
	  break;
	}
      tp->type = token_type_white_space;
      return;

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
	      bufmax += 100;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos++] = c;
	  c = phase4_getc ();
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
	      phase4_ungetc (c);
	      break;
	    }
	  break;
	}
      if (bufpos >= bufmax)
	{
	  bufmax += 100;
	  buffer = xrealloc (buffer, bufmax);
	}
      buffer[bufpos] = 0;
      tp->string = xstrdup (buffer);
      tp->type = token_type_name;
      return;

    case '.':
      c = phase4_getc ();
      phase4_ungetc (c);
      switch (c)
	{
	default:
	  tp->type = token_type_symbol;
	  return;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  c = '.';
	  break;
	}
      /* FALLTHROUGH */

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      /* The preprocessing number token is more "generous" than the C
	 number tokens.  This is mostly due to token pasting (another
	 thing we can ignore here).  */
      bufpos = 0;
      while (1)
	{
	  if (bufpos >= bufmax)
	    {
	      bufmax += 100;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos++] = c;
	  c = phase4_getc ();
	  switch (c)
	    {
	    case 'e':
	    case 'E':
	      if (bufpos >= bufmax)
		{
		  bufmax += 100;
		  buffer = xrealloc (buffer, bufmax);
		}
	      buffer[bufpos++] = c;
	      c = phase4_getc ();
	      if (c != '+' || c != '-')
		{
		  phase4_ungetc (c);
		  break;
		}
	      continue;

	    case 'A': case 'B': case 'C': case 'D':           case 'F':
	    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	    case 'Y': case 'Z':
	    case 'a': case 'b': case 'c': case 'd':           case 'f':
	    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	    case 'y': case 'z':
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	    case '.':
	      continue;

	    default:
	      phase4_ungetc (c);
	      break;
	    }
	  break;
	}
      if (bufpos >= bufmax)
	{
	  bufmax += 100;
	  buffer = xrealloc (buffer, bufmax);
	}
      buffer[bufpos] = 0;
      tp->type = token_type_number;
      tp->number = atol (buffer);
      return;

    case '\'':
      /* We could worry about the 'L' before wide character constants,
	 but ignoring it has no effect unless one of the keywords is
	 "L".  Just pretend it won't happen.  Also, we don't need to
	 remember the character constant.  */
      while (1)
	{
	  c = phase7_getc ();
	  if (c == P7_NEWLINE)
	    {
	      error (0, 0, _("%s:%d: warning: unterminated character constant"),
	        logical_file_name, line_number - 1);
	      phase7_ungetc ('\n');
	      break;
	    }
	  if (c == EOF || c == P7_QUOTE)
	    break;
	}
      tp->type = token_type_character_constant;
      return;

    case '"':
      /* We could worry about the 'L' before wide string constants,
	 but since gettext's argument is not a wide character string,
	 let the compiler complain about the argument not matching the
	 prototype.  Just pretend it won't happen.  */
      bufpos = 0;
      while (1)
	{
	  c = phase7_getc ();
	  if (c == P7_NEWLINE)
	    {
	      error (0, 0, _("%s:%d: warning: unterminated string literal"),
	        logical_file_name, line_number - 1);
	      phase7_ungetc ('\n');
	      break;
	    }
	  if (c == EOF || c == P7_QUOTES)
	    break;
	  if (c == P7_QUOTE)
	    c = '\'';
	  if (bufpos >= bufmax)
	    {
	      bufmax += 100;
	      buffer = xrealloc (buffer, bufmax);
	    }
	  buffer[bufpos++] = c;
	}
      if (bufpos >= bufmax)
	{
	  bufmax += 100;
	  buffer = xrealloc (buffer, bufmax);
	}
      buffer[bufpos] = 0;
      tp->type = token_type_string_literal;
      tp->string = xstrdup (buffer);
      return;

    case '(':
      tp->type = token_type_lp;
      return;

    case ',':
      tp->type = token_type_comma;
      return;

    case '#':
      tp->type = token_type_hash;
      return;

    default:
      /* We could carefully recognize each of the 2 and 3 character
        operators, but it is not necessary, as we only need to recognize
        gettext invocations.  Don't bother.  */
      tp->type = token_type_symbol;
      return;
    }
}


static void
phase5_unget (tp)
     token_ty *tp;
{
  if (tp->type != token_type_eof)
    phase5_pushback[phase5_pushback_length++] = *tp;
}


/* X. Recognize a leading # symbol.  Leave leading hash as a hash, but
   turn hash in the middle of a line into a plain symbol token.  This
   makes the phase 6 easier.  */

static void
phaseX_get (tp)
     token_ty *tp;
{
  static int middle;
  token_ty tmp;

  phase5_get (tp);
  if (middle)
    {
      switch (tp->type)
	{
	case token_type_eoln:
	case token_type_eof:
	  middle = 0;
	  break;

	case token_type_hash:
	  tp->type = token_type_symbol;
	  break;

	default:
	  break;
	}
    }
  else
    {
      switch (tp->type)
	{
	case token_type_eoln:
	case token_type_eof:
	  break;

	case token_type_white_space:
	  tmp = *tp;
	  phase5_get (tp);
	  if (tp->type != token_type_hash)
	    {
	      phase5_unget (tp);
	      *tp = tmp;
	      middle = 1;
	      return;
	    }

	  /* Discard the leading white space token, the hash is all
	     phase 6 is interested in.  */
	  if (tp->type != token_type_eof && tp->type != token_type_eoln)
	    middle = 1;
	  break;

	default:
	  middle = 1;
	  break;
	}
    }
}


/* 6. Recognize and carry out directives (it also expands macros on
   non-directive lines, which we do not do here).  The only directive
   we care about is the #line directive.  We throw all the others
   away.  */

/* Maximum used guaranteed to be < 4.  */
static token_ty phase6_pushback[4];
static int phase6_pushback_length;


static void
phase6_get (tp)
     token_ty *tp;
{
  static token_ty *buf;
  static int bufmax;
  int bufpos;
  int j;

  if (phase6_pushback_length)
    {
      *tp = phase6_pushback[--phase6_pushback_length];
      return;
    }
  while (1)
    {
      /* Get the next token.  If it is not a '#' at the beginning of a
	 line, return immediately.  Be careful of white space.  */
      phaseX_get (tp);
      if (tp->type != token_type_hash)
	return;

      /* Accumulate the rest of the directive in a buffer.  Work out
	 what it is later.  */
      bufpos = 0;
      while (1)
	{
	  phaseX_get (tp);
	  if (tp->type == token_type_eoln || tp->type == token_type_eof)
	    break;

	  /* White space would be important in the directive, if we
	     were interested in the #define directive.  But we are
	     going to ignore the #define directive, so just throw
	     white space away.  */
	  if (tp->type == token_type_white_space)
	    continue;

	  if (bufpos >= bufmax)
	    {
	      bufmax += 100;
	      buf = xrealloc (buf, bufmax * sizeof (buf[0]));
	    }
	  buf[bufpos++] = *tp;
	}

      /* If it is a #line directive, with no macros to expand, act on
	 it.  Ignore all other directives.  */
      if (bufpos >= 3 && buf[0].type == token_type_name
	  && strcmp (buf[0].string, "line") == 0
	  && buf[1].type == token_type_number
	  && buf[2].type == token_type_string_literal)
	{
	  free (logical_file_name);
	  logical_file_name = xstrdup (buf[2].string);
	  line_number = buf[1].number;
	}
      if (bufpos >= 2 && buf[0].type == token_type_number
	  && buf[1].type == token_type_string_literal)
	{
	  free (logical_file_name);
	  logical_file_name = xstrdup (buf[1].string);
	  line_number = buf[0].number;
	}

      /* Release the storage held by the directive.  */
      for (j = 0; j < bufpos; ++j)
	{
	  switch (buf[j].type)
	    {
	    case token_type_name:
	    case token_type_string_literal:
	      free (buf[j].string);
	      break;

	    default:
	      break;
	    }
	}

      /* We must reset the selected comments.  */
      xgettext_lex_comment_reset ();
    }
}


static void
phase6_unget (tp)
     token_ty *tp;
{
  if (tp->type != token_type_eof)
    phase6_pushback[phase6_pushback_length++] = *tp;
}


/* 8. Concatenate adjacent string literals to form single string
   literals (because we don't expand macros, there are a few things we
   will miss).  */

static void
phase8_get (tp)
     token_ty *tp;
{
  phase6_get (tp);
  if (tp->type != token_type_string_literal)
    return;
  while (1)
    {
      token_ty tmp;
      size_t len;

      phase6_get (&tmp);
      if (tmp.type == token_type_white_space)
	continue;
      if (tmp.type == token_type_eoln)
	continue;
      if (tmp.type != token_type_string_literal)
	{
	  phase6_unget (&tmp);
	  return;
	}
      len = strlen (tp->string);
      tp->string = xrealloc (tp->string, len + strlen (tmp.string) + 1);
      strcpy (tp->string + len, tmp.string);
      free (tmp.string);
    }
}


/* 9. Convert the remaining preprocessing tokens to C tokens and
   discards any white space from the translation unit.  */

void
xgettext_lex (tp)
     xgettext_token_ty *tp;
{
  while (1)
    {
      token_ty token;

      phase8_get (&token);
      switch (token.type)
	{
	case token_type_eof:
	  newline_count = 0;
	  last_comment_line = -1;
	  last_non_comment_line = -1;
	  tp->type = xgettext_token_type_eof;
	  return;

	case token_type_white_space:
	  break;

	case token_type_eoln:
	  /* We have to track the last occurrence of a string.  One
	     mode of xgettext allows to group an extracted message
	     with a comment for documentation.  The rule which states
	     which comment is assumed to be grouped with the message
	     says it should immediately precede it.  Our
	     interpretation: between the last line of the comment and
	     the line in which the keyword is found must be no line
	     with non-white space tokens.  */
	  ++newline_count;
	  if (last_non_comment_line > last_comment_line)
	    xgettext_lex_comment_reset ();
	  break;

	case token_type_name:
	  last_non_comment_line = newline_count;

	  if (default_keywords)
	    {
	      xgettext_lex_keyword ("gettext");
	      xgettext_lex_keyword ("dgettext");
	      xgettext_lex_keyword ("dcgettext");
	      xgettext_lex_keyword ("gettext_noop");
	      default_keywords = 0;
	    }

	  if (string_list_member (keywords, token.string))
	    {
	      tp->type = (strcmp (token.string, "dgettext") == 0
			  || strcmp (token.string, "dcgettext") == 0)
		? xgettext_token_type_keyword2 : xgettext_token_type_keyword1;
	    }
	  else
	    tp->type = xgettext_token_type_symbol;
	  free (token.string);
	  return;

	case token_type_lp:
	  last_non_comment_line = newline_count;

	  tp->type = xgettext_token_type_lp;
	  return;

	case token_type_comma:
	  last_non_comment_line = newline_count;

	  tp->type = xgettext_token_type_comma;
	  return;

	case token_type_string_literal:
	  last_non_comment_line = newline_count;

	  tp->type = xgettext_token_type_string_literal;
	  tp->string = token.string;
	  tp->line_number = token.line_number;
	  tp->file_name = logical_file_name;
	  return;

	default:
	  last_non_comment_line = newline_count;

	  tp->type = xgettext_token_type_symbol;
	  return;
	}
    }
}


void
xgettext_lex_keyword (name)
     char *name;
{
  if (name == NULL)
    default_keywords = 0;
  else
    {
      if (keywords == NULL)
	keywords = string_list_alloc ();

      string_list_append_unique (keywords, name);
    }
}


const char *
xgettext_lex_comment (n)
     size_t n;
{
  if (comment == NULL || n >= comment->nitems)
    return NULL;
  return comment->item[n];
}


void
xgettext_lex_comment_reset ()
{
  if (comment != NULL)
    {
      string_list_free (comment);
      comment = NULL;
    }
}


void
xgettext_lex_cplusplus ()
{
  cplusplus_comments = 1;
}


void
xgettext_lex_trigraphs ()
{
  trigraphs = 1;
}
