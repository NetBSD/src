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
#include <sys/types.h>

#include <libintl.h>
#define _(str) gettext(str)

#if HAVE_VPRINTF || HAVE_DOPRNT
# if __STDC__
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif
#else
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
#endif

#include "po-lex.h"
#include "po-gram.h"
#include "system.h"
#include "error.h"
#include "po-gram.gen.h"


static FILE *fp;
lex_pos_ty gram_pos;
size_t gram_max_allowed_errors = 20;
static int pass_comments = 0;
static int pass_obsolete_entries = 0;


/* Prototypes for local functions.  */
static int lex_getc PARAMS ((void));
static void lex_ungetc PARAMS ((int __ch));
static int keyword_p PARAMS ((char *__s));
static int control_sequence PARAMS ((void));


void
lex_open (fname)
     const char *fname;
{
  fp = open_po_file (fname, &gram_pos.file_name);
  if (!fp)
    error (EXIT_FAILURE, errno,
	   _("error while opening \"%s\" for reading"), fname);

  gram_pos.line_number = 1;
}


void
lex_close ()
{
  if (error_message_count > 0)
    error (EXIT_FAILURE, 0, _("found %d fatal errors"), error_message_count);

  if (fp != stdin)
    fclose (fp);
  fp = NULL;
  gram_pos.file_name = 0;
  gram_pos.line_number = 0;
  error_message_count = 0;
}


/* CAUTION: If you change this function, you must also make identical
   changes to the macro of the same name in src/po-lex.h  */

#if !__STDC__ || !defined __GNUC__ || __GNUC__ == 1
/* VARARGS1 */
void
# if defined VA_START && __STDC__
po_gram_error (const char *fmt, ...)
# else
po_gram_error (fmt, va_alist)
     const char *fmt;
     va_dcl
# endif
{
# ifdef VA_START
  va_list ap;
  char *buffer;

  VA_START (ap, fmt);

  vasprintf (&buffer, fmt, ap);
  va_end (ap);
  error_at_line (0, 0, gram_pos.file_name, gram_pos.line_number, "%s", buffer);
# else
  error_at_line (0, 0, gram_pos.file_name, gram_pos.line_number, fmt,
		 a1, a2, a3, a4, a5, a6, a7, a8);
# endif

  /* Some messages need more than one line.  Continuation lines are
     indicated by using "..." at the start of the string.  We don't
     increment the error counter for these continuation lines.  */
  if (*fmt == '.')
    --error_message_count;
  else if (error_message_count >= gram_max_allowed_errors)
    error (EXIT_FAILURE, 0, _("too many errors, aborting"));
}


/* CAUTION: If you change this function, you must also make identical
   changes to the macro of the same name in src/po-lex.h  */

/* VARARGS2 */
void
# if defined VA_START && __STDC__
gram_error_at_line (const lex_pos_ty *pp, const char *fmt, ...)
# else
gram_error_at_line (pp, fmt, va_alist)
     const lex_pos_ty *pp;
     const char *fmt;
     va_dcl
# endif
{
# ifdef VA_START
  va_list ap;
  char *buffer;

  VA_START (ap, fmt);

  vasprintf (&buffer, fmt, ap);
  va_end (ap);
  error_at_line (0, 0, pp->file_name, pp->line_number, "%s", buffer);
# else
  error_at_line (0, 0, pp->file_name, pp->line_number, fmt,
		 a1, a2, a3, a4, a5, a6, a7, a8);
# endif

  /* Some messages need more than one line, or more than one location.
     Continuation lines are indicated by using "..." at the start of the
     string.  We don't increment the error counter for these
     continuation lines.  */
  if (*fmt == '.')
    --error_message_count;
  else if (error_message_count >= gram_max_allowed_errors)
    error (EXIT_FAILURE, 0, _("too many errors, aborting"));
}
#endif


static int
lex_getc ()
{
  int c;

  for (;;)
    {
      c = getc (fp);
      switch (c)
	{
	case EOF:
	  if (ferror (fp))
	    error (EXIT_FAILURE, errno,	_("error while reading \"%s\""),
		   gram_pos.file_name);
	  return EOF;

	case '\n':
	  ++gram_pos.line_number;
	  return '\n';

	case '\\':
	  c = getc (fp);
	  if (c != '\n')
	    {
	      if (c != EOF)
		ungetc (c, fp);
	      return '\\';
	    }
	  ++gram_pos.line_number;
	  break;

	default:
	  return c;
	}
    }
}


static void
lex_ungetc (c)
     int c;
{
  switch (c)
    {
    case EOF:
      break;

    case '\n':
      --gram_pos.line_number;
      /* FALLTHROUGH */

    default:
      ungetc (c, fp);
      break;
    }
}


static int
keyword_p (s)
     char *s;
{
  if (!strcmp (s, "domain"))
    return DOMAIN;
  if (!strcmp (s, "msgid"))
    return MSGID;
  if (!strcmp (s, "msgstr"))
    return MSGSTR;
  po_gram_error (_("keyword \"%s\" unknown"), s);
  return NAME;
}


static int
control_sequence ()
{
  int c;
  int val;
  int max;

  c = lex_getc ();
  switch (c)
    {
    case 'n':
      return '\n';

    case 't':
      return '\t';

    case 'b':
      return '\b';

    case 'r':
      return '\r';

    case 'f':
      return '\f';

    case '\\':
    case '"':
      return c;

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      val = 0;
      for (max = 0; max < 3; ++max)
	{
	  /* Warning: not portable, can't depend on '0'..'7' ordering.  */
	  val = val * 8 + c - '0';
	  c = lex_getc ();
	  switch (c)
	    {
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
	      continue;

	    default:
	      break;
	    }
	  break;
	}
      lex_ungetc (c);
      return val;

    case 'x': case 'X':
      c = lex_getc ();
      if (c == EOF || !isxdigit (c))
	break;

      val = 0;
      for (;;)
	{
	  val *= 16;
	  if (isdigit (c))
	    /* Warning: not portable, can't depend on '0'..'9' ordering */
	    val += c - '0';
	  else if (isupper (c))
	    /* Warning: not portable, can't depend on 'A'..'F' ordering */
	    val += c - 'A' + 10;
	  else
	    /* Warning: not portable, can't depend on 'a'..'f' ordering */
	    val += c - 'a' + 10;

	  c = lex_getc ();
	  switch (c)
	    {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	      continue;

	    default:
	      break;
	    }
	  break;
	}
      return val;
    }
  po_gram_error (_("illegal control sequence"));
  return ' ';
}


int
po_gram_lex ()
{
  static char *buf;
  static size_t bufmax;
  int c;
  size_t bufpos;

  for (;;)
    {
      c = lex_getc ();
      switch (c)
	{
	case EOF:
	  /* Yacc want this for end of file.  */
	  return 0;

	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\f':
	  break;

	case '#':
	  /* Accumulate comments into a buffer.  If we have been asked
 	     to pass comments, generate a COMMENT token, otherwise
 	     discard it.  */
	  c = lex_getc ();
	  if (c == '~' && pass_obsolete_entries)
	    /* A special comment beginning with #~ is found.  This
	       is the format for obsolete entries and if we are
	       asked to return them is entries not as comments be
	       simply stop processing the comment here.  The
	       following characters are expected to be well formed.  */
	    break;

	  if (pass_comments)
	    {
	      bufpos = 0;
	      while (1)
		{
		  if (bufpos >= bufmax)
		    {
		      bufmax += 100;
		      buf = xrealloc (buf, bufmax);
		    }
		  if (c == EOF || c == '\n')
		    break;

		  buf[bufpos++] = c;
		  c = lex_getc ();
		}
	      buf[bufpos] = 0;

	      po_gram_lval.string = buf;
	      return COMMENT;
	    }
	  else
	    /* We do this in separate loop because collecting large
	       comments while they get not passed to the upper layers
	       is not very effective.  */
	    while (c != EOF && c != '\n')
	      c = lex_getc ();
	  break;

	case '"':
	  bufpos = 0;
	  while (1)
	    {
	      if (bufpos >= bufmax)
		{
		  bufmax += 100;
		  buf = xrealloc (buf, bufmax);
		}
	      c = lex_getc ();
	      if (c == '\n')
		{
		  po_gram_error (_("end-of-line within string"));
		  break;
		}
	      if (c == EOF)
		{
		  po_gram_error (_("end-of-file within string"));
		  break;
		}
	      if (c == '"')
		break;

	      if (c == '\\')
		c = control_sequence ();

	      buf[bufpos++] = c;
	    }
	  buf[bufpos] = 0;

	  po_gram_lval.string = xstrdup (buf);
	  return STRING;

	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
	case '_': case '$':
	  bufpos = 0;
	  for (;;)
	    {
	      if (bufpos + 1 >= bufmax)
		{
		  bufmax += 100;
		  buf = xrealloc (buf, bufmax);
		}
	      buf[bufpos++] = c;
	      c = lex_getc ();
	      switch (c)
		{
		default:
		  break;
		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
		case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p':
		case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G': case 'H':
		case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P':
		case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case '_': case '$':
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
		  continue;
		}
	      break;
	    }
	  lex_ungetc (c);

	  buf[bufpos] = 0;

	  c = keyword_p (buf);
	  if (c == NAME)
	    po_gram_lval.string = xstrdup (buf);
	  return c;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  /* I know, we don't need numbers, yet.  */
	  bufpos = 0;
	  for (;;)
	    {
	      if (bufpos + 1 >= bufmax)
		{
		  bufmax += 100;
		  buf = xrealloc (buf, bufmax + 1);
		}
	      buf[bufpos++] = c;
	      c = lex_getc ();
	      switch (c)
		{
		default:
		  break;

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
		  continue;
		}
	      break;
	    }
	  lex_ungetc (c);

	  buf[bufpos] = 0;

	  po_gram_lval.number = atol (buf);
	  return NUMBER;

	default:
	  /* This will cause a syntax error.  */
	  return JUNK;
	}
    }
}


void
po_lex_pass_comments (flag)
     int flag;
{
  pass_comments = (flag != 0);
}


void
po_lex_pass_obsolete_entries (flag)
     int flag;
{
  pass_obsolete_entries = (flag != 0);
}
