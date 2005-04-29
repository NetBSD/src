/* GNU gettext - internationalization aids
   Copyright (C) 1995-1998, 2000-2005 Free Software Foundation, Inc.

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
# include <config.h>
#endif
#include <alloca.h>

/* Specification.  */
#include "write-po.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_ICONV
# include <iconv.h>
#endif

#include "c-ctype.h"
#include "po-charset.h"
#include "linebreak.h"
#include "msgl-ascii.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "xalloc.h"
#include "xallocsa.h"
#include "strstr.h"
#include "fwriteerror.h"
#include "exit.h"
#include "error-progname.h"
#include "xerror.h"
#include "po-error.h"
#include "gettext.h"

/* Our regular abbreviation.  */
#define _(str) gettext (str)

#if HAVE_DECL_PUTC_UNLOCKED
# undef putc
# define putc putc_unlocked
#endif


/* =================== Putting together a #, flags line. =================== */


/* Convert IS_FORMAT in the context of programming language LANG to a flag
   string for use in #, flags.  */

const char *
make_format_description_string (enum is_format is_format, const char *lang,
				bool debug)
{
  static char result[100];

  switch (is_format)
    {
    case possible:
      if (debug)
	{
	  sprintf (result, " possible-%s-format", lang);
	  break;
	}
      /* FALLTHROUGH */
    case yes_according_to_context:
    case yes:
      sprintf (result, " %s-format", lang);
      break;
    case no:
      sprintf (result, " no-%s-format", lang);
      break;
    default:
      /* The others have already been filtered out by significant_format_p.  */
      abort ();
    }

  return result;
}


/* Return true if IS_FORMAT is worth mentioning in a #, flags list.  */

bool
significant_format_p (enum is_format is_format)
{
  return is_format != undecided && is_format != impossible;
}


/* Return true if one of IS_FORMAT is worth mentioning in a #, flags list.  */

static bool
has_significant_format_p (const enum is_format is_format[NFORMATS])
{
  size_t i;

  for (i = 0; i < NFORMATS; i++)
    if (significant_format_p (is_format[i]))
      return true;
  return false;
}


/* Convert a wrapping flag DO_WRAP to a string for use in #, flags.  */

static const char *
make_c_width_description_string (enum is_wrap do_wrap)
{
  const char *result = NULL;

  switch (do_wrap)
    {
    case yes:
      result = " wrap";
      break;
    case no:
      result = " no-wrap";
      break;
    default:
      abort ();
    }

  return result;
}


/* ================ Output parts of a message, as comments. ================ */


/* Output mp->comment as a set of comment lines.  */

void
message_print_comment (const message_ty *mp, FILE *fp)
{
  if (mp->comment != NULL)
    {
      size_t j;

      for (j = 0; j < mp->comment->nitems; ++j)
	{
	  const char *s = mp->comment->item[j];
	  do
	    {
	      const char *e;
	      putc ('#', fp);
	      if (*s != '\0' && *s != ' ')
		putc (' ', fp);
	      e = strchr (s, '\n');
	      if (e == NULL)
		{
		  fputs (s, fp);
		  s = NULL;
		}
	      else
		{
		  fwrite (s, 1, e - s, fp);
		  s = e + 1;
		}
	      putc ('\n', fp);
	    }
	  while (s != NULL);
	}
    }
}


/* Output mp->comment_dot as a set of comment lines.  */

void
message_print_comment_dot (const message_ty *mp, FILE *fp)
{
  if (mp->comment_dot != NULL)
    {
      size_t j;

      for (j = 0; j < mp->comment_dot->nitems; ++j)
	{
	  const char *s = mp->comment_dot->item[j];
	  putc ('#', fp);
	  putc ('.', fp);
	  if (*s != '\0' && *s != ' ')
	    putc (' ', fp);
	  fputs (s, fp);
	  putc ('\n', fp);
	}
    }
}


/* Output mp->filepos as a set of comment lines.  */

void
message_print_comment_filepos (const message_ty *mp, FILE *fp,
			       bool uniforum, size_t page_width)
{
  if (mp->filepos_count != 0)
    {
      if (uniforum)
	{
	  size_t j;

	  for (j = 0; j < mp->filepos_count; ++j)
	    {
	      lex_pos_ty *pp = &mp->filepos[j];
	      char *cp = pp->file_name;
	      while (cp[0] == '.' && cp[1] == '/')
		cp += 2;
	      /* There are two Sun formats to choose from: SunOS and
		 Solaris.  Use the Solaris form here.  */
	      fprintf (fp, "# File: %s, line: %ld\n",
		       cp, (long) pp->line_number);
	    }
	}
      else
	{
	  size_t column;
	  size_t j;

	  fputs ("#:", fp);
	  column = 2;
	  for (j = 0; j < mp->filepos_count; ++j)
	    {
	      lex_pos_ty *pp;
	      char buffer[21];
	      char *cp;
	      size_t len;

	      pp = &mp->filepos[j];
	      cp = pp->file_name;
	      while (cp[0] == '.' && cp[1] == '/')
		cp += 2;
	      /* Some xgettext input formats, like RST, lack line numbers.  */
	      if (pp->line_number == (size_t)(-1))
		buffer[0] = '\0';
	      else
		sprintf (buffer, ":%ld", (long) pp->line_number);
	      len = strlen (cp) + strlen (buffer) + 1;
	      if (column > 2 && column + len >= page_width)
		{
		  fputs ("\n#:", fp);
		  column = 2;
		}
	      fprintf (fp, " %s%s", cp, buffer);
	      column += len;
	    }
	  putc ('\n', fp);
	}
    }
}


/* Output mp->is_fuzzy, mp->is_format, mp->do_wrap as a comment line.  */

void
message_print_comment_flags (const message_ty *mp, FILE *fp, bool debug)
{
  if ((mp->is_fuzzy && mp->msgstr[0] != '\0')
      || has_significant_format_p (mp->is_format)
      || mp->do_wrap == no)
    {
      bool first_flag = true;
      size_t i;

      putc ('#', fp);
      putc (',', fp);

      /* We don't print the fuzzy flag if the msgstr is empty.  This
	 might be introduced by the user but we want to normalize the
	 output.  */
      if (mp->is_fuzzy && mp->msgstr[0] != '\0')
	{
	  fputs (" fuzzy", fp);
	  first_flag = false;
	}

      for (i = 0; i < NFORMATS; i++)
	if (significant_format_p (mp->is_format[i]))
	  {
	    if (!first_flag)
	      putc (',', fp);

	    fputs (make_format_description_string (mp->is_format[i],
						   format_language[i], debug),
		   fp);
	    first_flag = false;
	  }

      if (mp->do_wrap == no)
	{
	  if (!first_flag)
	    putc (',', fp);

	  fputs (make_c_width_description_string (mp->do_wrap), fp);
	  first_flag = false;
	}

      putc ('\n', fp);
    }
}


/* =========== Some parameters for use by 'msgdomain_list_print'. ========== */


/* This variable controls the page width when printing messages.
   Defaults to PAGE_WIDTH if not set.  Zero (0) given to message_page_-
   width_set will result in no wrapping being performed.  */
static size_t page_width = PAGE_WIDTH;

void
message_page_width_set (size_t n)
{
  if (n == 0)
    {
      page_width = INT_MAX;
      return;
    }

  if (n < 20)
    n = 20;

  page_width = n;
}


/* This variable controls the extent to which the page width applies.
   True means it applies to message strings and file reference lines.
   False means it applies to file reference lines only.  */
static bool wrap_strings = true;

void
message_page_width_ignore ()
{
  wrap_strings = false;
}


/* These three variables control the output style of the message_print
   function.  Interface functions for them are to be used.  */
static bool indent = false;
static bool uniforum = false;
static bool escape = false;

void
message_print_style_indent ()
{
  indent = true;
}

void
message_print_style_uniforum ()
{
  uniforum = true;
}

void
message_print_style_escape (bool flag)
{
  escape = flag;
}


/* Whether to output a file in Java .properties syntax.  */
static bool use_syntax_properties = false;

void
message_print_syntax_properties ()
{
  use_syntax_properties = true;
}


/* Whether to output a file in NeXTstep/GNUstep .strings syntax.  */
static bool use_syntax_stringtable = false;

void
message_print_syntax_stringtable ()
{
  use_syntax_stringtable = true;
}


/* ================ msgdomain_list_print() and subroutines. ================ */


/* A version of memcpy optimized for the case n <= 1.  */
static inline void
memcpy_small (void *dst, const void *src, size_t n)
{
  if (n > 0)
    {
      char *q = (char *) dst;
      const char *p = (const char *) src;

      *q = *p;
      if (--n > 0)
	do *++q = *++p; while (--n > 0);
    }
}


static void
wrap (FILE *fp, const char *line_prefix, const char *name, const char *value,
      enum is_wrap do_wrap, const char *charset)
{
  const char *canon_charset;
  const char *s;
  bool first_line;
#if HAVE_ICONV
  const char *envval;
  iconv_t conv;
#endif
  bool weird_cjk;

  canon_charset = po_charset_canonicalize (charset);

#if HAVE_ICONV
  /* The old Solaris/openwin msgfmt and GNU msgfmt <= 0.10.35 don't know
     about multibyte encodings, and require a spurious backslash after
     every multibyte character whose last byte is 0x5C.  Some programs,
     like vim, distribute PO files in this broken format.  It is important
     for such programs that GNU msgmerge continues to support this old
     PO file format when the Makefile requests it.  */
  envval = getenv ("OLD_PO_FILE_OUTPUT");
  if (envval != NULL && *envval != '\0')
    /* Write a PO file in old format, with extraneous backslashes.  */
    conv = (iconv_t)(-1);
  else
    if (canon_charset == NULL)
      /* Invalid PO file encoding.  */
      conv = (iconv_t)(-1);
    else
      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
      if (strcmp (canon_charset, "EUC-KR") == 0)
	conv = (iconv_t)(-1);
      else
# endif
      /* Avoid Solaris 2.9 bug with GB2312, EUC-TW, BIG5, BIG5-HKSCS, GBK,
	 GB18030.  */
# if defined __sun && !defined _LIBICONV_VERSION
      if (   strcmp (canon_charset, "GB2312") == 0
	  || strcmp (canon_charset, "EUC-TW") == 0
	  || strcmp (canon_charset, "BIG5") == 0
	  || strcmp (canon_charset, "BIG5-HKSCS") == 0
	  || strcmp (canon_charset, "GBK") == 0
	  || strcmp (canon_charset, "GB18030") == 0)
	conv = (iconv_t)(-1);
      else
# endif
      /* Use iconv() to parse multibyte characters.  */
      conv = iconv_open ("UTF-8", canon_charset);

  if (conv != (iconv_t)(-1))
    weird_cjk = false;
  else
#endif
    if (canon_charset == NULL)
      weird_cjk = false;
    else
      weird_cjk = po_is_charset_weird_cjk (canon_charset);

  if (canon_charset == NULL)
    canon_charset = po_charset_ascii;

  /* Loop over the '\n' delimited portions of value.  */
  s = value;
  first_line = true;
  do
    {
      /* The \a and \v escapes were added by the ANSI C Standard.
	 Prior to the Standard, most compilers did not have them.
	 Because we need the same program on all platforms we don't provide
	 support for them here.  Thus we only support \b\f\n\r\t.  */
#     define is_escape(c) \
       ((c) == '\b' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t')

      const char *es;
      const char *ep;
      size_t portion_len;
      char *portion;
      char *overrides;
      char *linebreaks;
      char *pp;
      char *op;
      int startcol, startcol_after_break, width;
      size_t i;

      for (es = s; *es != '\0'; )
	if (*es++ == '\n')
	  break;

      /* Expand escape sequences in each portion.  */
      for (ep = s, portion_len = 0; ep < es; ep++)
	{
	  char c = *ep;
	  if (is_escape (c))
	    portion_len += 2;
	  else if (escape && !c_isprint ((unsigned char) c))
	    portion_len += 4;
	  else if (c == '\\' || c == '"')
	    portion_len += 2;
	  else
	    {
#if HAVE_ICONV
	      if (conv != (iconv_t)(-1))
		{
		  /* Skip over a complete multi-byte character.  Don't
		     interpret the second byte of a multi-byte character as
		     ASCII.  This is needed for the BIG5, BIG5-HKSCS, GBK,
		     GB18030, SHIFT_JIS, JOHAB encodings.  */
		  char scratchbuf[64];
		  const char *inptr = ep;
		  size_t insize;
		  char *outptr = &scratchbuf[0];
		  size_t outsize = sizeof (scratchbuf);
		  size_t res;

		  res = (size_t)(-1);
		  for (insize = 1; inptr + insize <= es; insize++)
		    {
		      res = iconv (conv,
				   (ICONV_CONST char **) &inptr, &insize,
				   &outptr, &outsize);
		      if (!(res == (size_t)(-1) && errno == EINVAL))
			break;
		      /* We expect that no input bytes have been consumed
			 so far.  */
		      if (inptr != ep)
			abort ();
		    }
		  if (res == (size_t)(-1))
		    {
		      if (errno == EILSEQ)
			{
			  po_error (0, 0, _("invalid multibyte sequence"));
			  continue;
			}
		      else
			abort ();
		    }
		  insize = inptr - ep;
		  portion_len += insize;
		  ep += insize - 1;
		}
	      else
#endif
		{
		  if (weird_cjk
		      /* Special handling of encodings with CJK structure.  */
		      && ep + 2 <= es
		      && (unsigned char) ep[0] >= 0x80
		      && (unsigned char) ep[1] >= 0x30)
		    {
		      portion_len += 2;
		      ep += 1;
		    }
		  else
		    portion_len += 1;
		}
	    }
	}
      portion = (char *) xmalloc (portion_len);
      overrides = (char *) xmalloc (portion_len);
      memset (overrides, UC_BREAK_UNDEFINED, portion_len);
      for (ep = s, pp = portion, op = overrides; ep < es; ep++)
	{
	  char c = *ep;
	  if (is_escape (c))
	    {
	      switch (c)
		{
		case '\b': c = 'b'; break;
		case '\f': c = 'f'; break;
		case '\n': c = 'n'; break;
		case '\r': c = 'r'; break;
		case '\t': c = 't'; break;
		default: abort ();
		}
	      *pp++ = '\\';
	      *pp++ = c;
	      op++;
	      *op++ = UC_BREAK_PROHIBITED;
	      /* We warn about any use of escape sequences beside
		 '\n' and '\t'.  */
	      if (c != 'n' && c != 't')
		po_error (0, 0, _("\
internationalized messages should not contain the `\\%c' escape sequence"),
			  c);
	    }
	  else if (escape && !c_isprint ((unsigned char) c))
	    {
	      *pp++ = '\\';
	      *pp++ = '0' + (((unsigned char) c >> 6) & 7);
	      *pp++ = '0' + (((unsigned char) c >> 3) & 7);
	      *pp++ = '0' + ((unsigned char) c & 7);
	      op++;
	      *op++ = UC_BREAK_PROHIBITED;
	      *op++ = UC_BREAK_PROHIBITED;
	      *op++ = UC_BREAK_PROHIBITED;
	    }
	  else if (c == '\\' || c == '"')
	    {
	      *pp++ = '\\';
	      *pp++ = c;
	      op++;
	      *op++ = UC_BREAK_PROHIBITED;
	    }
	  else
	    {
#if HAVE_ICONV
	      if (conv != (iconv_t)(-1))
		{
		  /* Copy a complete multi-byte character.  Don't
		     interpret the second byte of a multi-byte character as
		     ASCII.  This is needed for the BIG5, BIG5-HKSCS, GBK,
		     GB18030, SHIFT_JIS, JOHAB encodings.  */
		  char scratchbuf[64];
		  const char *inptr = ep;
		  size_t insize;
		  char *outptr = &scratchbuf[0];
		  size_t outsize = sizeof (scratchbuf);
		  size_t res;

		  res = (size_t)(-1);
		  for (insize = 1; inptr + insize <= es; insize++)
		    {
		      res = iconv (conv,
				   (ICONV_CONST char **) &inptr, &insize,
				   &outptr, &outsize);
		      if (!(res == (size_t)(-1) && errno == EINVAL))
			break;
		      /* We expect that no input bytes have been consumed
			 so far.  */
		      if (inptr != ep)
			abort ();
		    }
		  if (res == (size_t)(-1))
		    {
		      if (errno == EILSEQ)
			{
			  po_error (0, 0, _("invalid multibyte sequence"));
			  continue;
			}
		      else
			abort ();
		    }
		  insize = inptr - ep;
		  memcpy_small (pp, ep, insize);
		  pp += insize;
		  op += insize;
		  ep += insize - 1;
		}
	      else
#endif
		{
		  if (weird_cjk
		      /* Special handling of encodings with CJK structure.  */
		      && ep + 2 <= es
		      && (unsigned char) c >= 0x80
		      && (unsigned char) ep[1] >= 0x30)
		    {
		      *pp++ = c;
		      ep += 1;
		      *pp++ = *ep;
		      op += 2;
		    }
		  else
		    {
		      *pp++ = c;
		      op++;
		    }
		}
	    }
	}

      /* Don't break immediately before the "\n" at the end.  */
      if (es > s && es[-1] == '\n')
	overrides[portion_len - 2] = UC_BREAK_PROHIBITED;

      linebreaks = (char *) xmalloc (portion_len);

      /* Subsequent lines after a break are all indented.
	 See INDENT-S.  */
      startcol_after_break = (line_prefix ? strlen (line_prefix) : 0);
      if (indent)
	startcol_after_break = (startcol_after_break + 8) & ~7;
      startcol_after_break++;

      /* The line width.  Allow room for the closing quote character.  */
      width = (wrap_strings && do_wrap != no ? page_width : INT_MAX) - 1;
      /* Adjust for indentation of subsequent lines.  */
      width -= startcol_after_break;

    recompute:
      /* The line starts with different things depending on whether it
	 is the first line, and if we are using the indented style.
	 See INDENT-F.  */
      startcol = (line_prefix ? strlen (line_prefix) : 0);
      if (first_line)
	{
	  startcol += strlen (name);
	  if (indent)
	    startcol = (startcol + 8) & ~7;
	  else
	    startcol++;
	}
      else
	{
	  if (indent)
	    startcol = (startcol + 8) & ~7;
	}
      /* Allow room for the opening quote character.  */
      startcol++;
      /* Adjust for indentation of subsequent lines.  */
      startcol -= startcol_after_break;

      /* Do line breaking on the portion.  */
      mbs_width_linebreaks (portion, portion_len, width, startcol, 0,
			    overrides, canon_charset, linebreaks);

      /* If this is the first line, and we are not using the indented
	 style, and the line would wrap, then use an empty first line
	 and restart.  */
      if (first_line && !indent
	  && portion_len > 0
	  && (*es != '\0'
	      || startcol > width
	      || memchr (linebreaks, UC_BREAK_POSSIBLE, portion_len) != NULL))
	{
	  if (line_prefix != NULL)
	    fputs (line_prefix, fp);
	  fputs (name, fp);
	  fputs (" \"\"\n", fp);
	  first_line = false;
	  /* Recompute startcol and linebreaks.  */
	  goto recompute;
	}

      /* Print the beginning of the line.  This will depend on whether
	 this is the first line, and if the indented style is being
	 used.  INDENT-F.  */
      if (line_prefix != NULL)
	fputs (line_prefix, fp);
      if (first_line)
	{
	  fputs (name, fp);
	  putc (indent ? '\t' : ' ', fp);
	  first_line = false;
	}
      else
	{
	  if (indent)
	    putc ('\t', fp);
	}

      /* Print the portion itself, with linebreaks where necessary.  */
      putc ('"', fp);
      for (i = 0; i < portion_len; i++)
	{
	  if (linebreaks[i] == UC_BREAK_POSSIBLE)
	    {
	      fputs ("\"\n", fp);
	      /* INDENT-S.  */
	      if (line_prefix != NULL)
		fputs (line_prefix, fp);
	      if (indent)
		putc ('\t', fp);
	      putc ('"', fp);
	    }
	  putc (portion[i], fp);
	}
      fputs ("\"\n", fp);

      free (linebreaks);
      free (overrides);
      free (portion);

      s = es;
#     undef is_escape
    }
  while (*s);

#if HAVE_ICONV
  if (conv != (iconv_t)(-1))
    iconv_close (conv);
#endif
}


static void
print_blank_line (FILE *fp)
{
  if (uniforum)
    fputs ("#\n", fp);
  else
    putc ('\n', fp);
}


static void
message_print (const message_ty *mp, FILE *fp, const char *charset,
	       bool blank_line, bool debug)
{
  /* Separate messages with a blank line.  Uniforum doesn't like blank
     lines, so use an empty comment (unless there already is one).  */
  if (blank_line && (!uniforum
		     || mp->comment == NULL
		     || mp->comment->nitems == 0
		     || mp->comment->item[0][0] != '\0'))
    print_blank_line (fp);

  /* Print translator comment if available.  */
  message_print_comment (mp, fp);

  /* Print xgettext extracted comments.  */
  message_print_comment_dot (mp, fp);

  /* Print the file position comments.  This will help a human who is
     trying to navigate the sources.  There is no problem of getting
     repeated positions, because duplicates are checked for.  */
  message_print_comment_filepos (mp, fp, uniforum, page_width);

  /* Print flag information in special comment.  */
  message_print_comment_flags (mp, fp, debug);

  /* Print each of the message components.  Wrap them nicely so they
     are as readable as possible.  If there is no recorded msgstr for
     this domain, emit an empty string.  */
  if (!is_ascii_string (mp->msgid)
      && po_charset_canonicalize (charset) != po_charset_utf8)
    po_multiline_warning (xasprintf (_("warning: ")),
			  xasprintf (_("\
The following msgid contains non-ASCII characters.\n\
This will cause problems to translators who use a character encoding\n\
different from yours. Consider using a pure ASCII msgid instead.\n\
%s\n"), mp->msgid));
  wrap (fp, NULL, "msgid", mp->msgid, mp->do_wrap, charset);
  if (mp->msgid_plural != NULL)
    wrap (fp, NULL, "msgid_plural", mp->msgid_plural, mp->do_wrap, charset);

  if (mp->msgid_plural == NULL)
    wrap (fp, NULL, "msgstr", mp->msgstr, mp->do_wrap, charset);
  else
    {
      char prefix_buf[20];
      unsigned int i;
      const char *p;

      for (p = mp->msgstr, i = 0;
	   p < mp->msgstr + mp->msgstr_len;
	   p += strlen (p) + 1, i++)
	{
	  sprintf (prefix_buf, "msgstr[%u]", i);
	  wrap (fp, NULL, prefix_buf, p, mp->do_wrap, charset);
	}
    }
}


static void
message_print_obsolete (const message_ty *mp, FILE *fp, const char *charset,
			bool blank_line)
{
  /* If msgstr is the empty string we print nothing.  */
  if (mp->msgstr[0] == '\0')
    return;

  /* Separate messages with a blank line.  Uniforum doesn't like blank
     lines, so use an empty comment (unless there already is one).  */
  if (blank_line)
    print_blank_line (fp);

  /* Print translator comment if available.  */
  message_print_comment (mp, fp);

  /* Print flag information in special comment.  */
  if (mp->is_fuzzy)
    {
      bool first = true;

      putc ('#', fp);
      putc (',', fp);

      if (mp->is_fuzzy)
	{
	  fputs (" fuzzy", fp);
	  first = false;
	}

      putc ('\n', fp);
    }

  /* Print each of the message components.  Wrap them nicely so they
     are as readable as possible.  */
  if (!is_ascii_string (mp->msgid)
      && po_charset_canonicalize (charset) != po_charset_utf8)
    po_multiline_warning (xasprintf (_("warning: ")),
			  xasprintf (_("\
The following msgid contains non-ASCII characters.\n\
This will cause problems to translators who use a character encoding\n\
different from yours. Consider using a pure ASCII msgid instead.\n\
%s\n"), mp->msgid));
  wrap (fp, "#~ ", "msgid", mp->msgid, mp->do_wrap, charset);
  if (mp->msgid_plural != NULL)
    wrap (fp, "#~ ", "msgid_plural", mp->msgid_plural, mp->do_wrap, charset);

  if (mp->msgid_plural == NULL)
    wrap (fp, "#~ ", "msgstr", mp->msgstr, mp->do_wrap, charset);
  else
    {
      char prefix_buf[20];
      unsigned int i;
      const char *p;

      for (p = mp->msgstr, i = 0;
	   p < mp->msgstr + mp->msgstr_len;
	   p += strlen (p) + 1, i++)
	{
	  sprintf (prefix_buf, "msgstr[%u]", i);
	  wrap (fp, "#~ ", prefix_buf, p, mp->do_wrap, charset);
	}
    }
}


static void
msgdomain_list_print_po (msgdomain_list_ty *mdlp, FILE *fp, bool debug)
{
  size_t j, k;
  bool blank_line;

  /* Write out the messages for each domain.  */
  blank_line = false;
  for (k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp;
      const char *header;
      char *charset;
      char *allocated_charset;

      /* If the first domain is the default, don't bother emitting
	 the domain name, because it is the default.  */
      if (!(k == 0
	    && strcmp (mdlp->item[k]->domain, MESSAGE_DOMAIN_DEFAULT) == 0))
	{
	  if (blank_line)
	    print_blank_line (fp);
	  fprintf (fp, "domain \"%s\"\n", mdlp->item[k]->domain);
	  blank_line = true;
	}

      mlp = mdlp->item[k]->messages;

      /* Search the header entry.  */
      header = NULL;
      for (j = 0; j < mlp->nitems; ++j)
	if (mlp->item[j]->msgid[0] == '\0' && !mlp->item[j]->obsolete)
	  {
	    header = mlp->item[j]->msgstr;
	    break;
	  }

      /* Extract the charset name.  */
      charset = "ASCII";
      allocated_charset = NULL;
      if (header != NULL)
	{
	  const char *charsetstr = strstr (header, "charset=");

	  if (charsetstr != NULL)
	    {
	      size_t len;

	      charsetstr += strlen ("charset=");
	      len = strcspn (charsetstr, " \t\n");
	      charset = allocated_charset = (char *) xallocsa (len + 1);
	      memcpy (charset, charsetstr, len);
	      charset[len] = '\0';

	      /* Treat the dummy default value as if it were absent.  */
	      if (strcmp (charset, "CHARSET") == 0)
		charset = "ASCII";
	    }
	}

      /* Write out each of the messages for this domain.  */
      for (j = 0; j < mlp->nitems; ++j)
	if (!mlp->item[j]->obsolete)
	  {
	    message_print (mlp->item[j], fp, charset, blank_line, debug);
	    blank_line = true;
	  }

      /* Write out each of the obsolete messages for this domain.  */
      for (j = 0; j < mlp->nitems; ++j)
	if (mlp->item[j]->obsolete)
	  {
	    message_print_obsolete (mlp->item[j], fp, charset, blank_line);
	    blank_line = true;
	  }

      if (allocated_charset != NULL)
	freesa (allocated_charset);
    }
}


void
msgdomain_list_print (msgdomain_list_ty *mdlp, const char *filename,
		      bool force, bool debug)
{
  FILE *fp;

  /* We will not write anything if, for every domain, we have no message
     or only the header entry.  */
  if (!force)
    {
      bool found_nonempty = false;
      size_t k;

      for (k = 0; k < mdlp->nitems; k++)
	{
	  message_list_ty *mlp = mdlp->item[k]->messages;

	  if (!(mlp->nitems == 0
		|| (mlp->nitems == 1 && mlp->item[0]->msgid[0] == '\0')))
	    {
	      found_nonempty = true;
	      break;
	    }
	}

      if (!found_nonempty)
	return;
    }

  /* Check whether the output format can accomodate all messages.  */
  if (use_syntax_properties || use_syntax_stringtable)
    {
      if (mdlp->nitems > 1)
	{
	  if (use_syntax_properties)
	    po_error (EXIT_FAILURE, 0, _("Cannot output multiple translation domains into a single file with Java .properties syntax. Try using PO file syntax instead."));
	  if (use_syntax_stringtable)
	    po_error (EXIT_FAILURE, 0, _("Cannot output multiple translation domains into a single file with NeXTstep/GNUstep .strings syntax."));
	}
      if (mdlp->nitems == 1)
	{
	  message_list_ty *mlp = mdlp->item[0]->messages;
	  const lex_pos_ty *has_plural;
	  size_t j;

	  has_plural = NULL;
	  for (j = 0; j < mlp->nitems; j++)
	    {
	      message_ty *mp = mlp->item[j];

	      if (mp->msgid_plural != NULL)
		{
		  has_plural = &mp->pos;
		  break;
		}
	    }

	  if (has_plural != NULL)
	    {
	      error_with_progname = false;
	      if (use_syntax_properties)
		po_error_at_line (EXIT_FAILURE, 0,
				  has_plural->file_name, has_plural->line_number,
				  _("message catalog has plural form translations, but the output format does not support them. Try generating a Java class using \"msgfmt --java\", instead of a properties file."));
	      if (use_syntax_stringtable)
		po_error_at_line (EXIT_FAILURE, 0,
				  has_plural->file_name, has_plural->line_number,
				  _("message catalog has plural form translations, but the output format does not support them."));
	      error_with_progname = true;
	    }
	}
    }

  /* Open the output file.  */
  if (filename != NULL && strcmp (filename, "-") != 0
      && strcmp (filename, "/dev/stdout") != 0)
    {
      fp = fopen (filename, "w");
      if (fp == NULL)
	po_error (EXIT_FAILURE, errno, _("cannot create output file \"%s\""),
		  filename);
    }
  else
    {
      fp = stdout;
      /* xgettext:no-c-format */
      filename = _("standard output");
    }

  if (use_syntax_properties)
    msgdomain_list_print_properties (mdlp, fp, page_width, debug);
  else if (use_syntax_stringtable)
    msgdomain_list_print_stringtable (mdlp, fp, page_width, debug);
  else
    msgdomain_list_print_po (mdlp, fp, debug);

  /* Make sure nothing went wrong.  */
  if (fwriteerror (fp))
    po_error (EXIT_FAILURE, errno, _("error while writing \"%s\" file"),
	      filename);
}


/* =============================== Sorting. ================================ */


static int
cmp_by_msgid (const void *va, const void *vb)
{
  const message_ty *a = *(const message_ty **) va;
  const message_ty *b = *(const message_ty **) vb;
  /* Because msgids normally contain only ASCII characters, it is OK to
     sort them as if we were in the C locale. And strcoll() in the C locale
     is the same as strcmp().  */
  return strcmp (a->msgid, b->msgid);
}


void
msgdomain_list_sort_by_msgid (msgdomain_list_ty *mdlp)
{
  size_t k;

  for (k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      if (mlp->nitems > 0)
	qsort (mlp->item, mlp->nitems, sizeof (mlp->item[0]), cmp_by_msgid);
    }
}


/* Sort the file positions of every message.  */

static int
cmp_filepos (const void *va, const void *vb)
{
  const lex_pos_ty *a = (const lex_pos_ty *) va;
  const lex_pos_ty *b = (const lex_pos_ty *) vb;
  int cmp;

  cmp = strcmp (a->file_name, b->file_name);
  if (cmp == 0)
    cmp = (int) a->line_number - (int) b->line_number;

  return cmp;
}

static void
msgdomain_list_sort_filepos (msgdomain_list_ty *mdlp)
{
  size_t j, k;

  for (k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      for (j = 0; j < mlp->nitems; j++)
	{
	  message_ty *mp = mlp->item[j];

	  if (mp->filepos_count > 0)
	    qsort (mp->filepos, mp->filepos_count, sizeof (mp->filepos[0]),
		   cmp_filepos);
	}
    }
}


/* Sort the messages according to the file position.  */

static int
cmp_by_filepos (const void *va, const void *vb)
{
  const message_ty *a = *(const message_ty **) va;
  const message_ty *b = *(const message_ty **) vb;
  int cmp;

  /* No filepos is smaller than any other filepos.  */
  if (a->filepos_count == 0)
    {
      if (b->filepos_count != 0)
	return -1;
    }
  if (b->filepos_count == 0)
    return 1;

  /* Compare on the file names...  */
  cmp = strcmp (a->filepos[0].file_name, b->filepos[0].file_name);
  if (cmp != 0)
    return cmp;

  /* If they are equal, compare on the line numbers...  */
  cmp = a->filepos[0].line_number - b->filepos[0].line_number;
  if (cmp != 0)
    return cmp;

  /* If they are equal, compare on the msgid strings.  */
  /* Because msgids normally contain only ASCII characters, it is OK to
     sort them as if we were in the C locale. And strcoll() in the C locale
     is the same as strcmp().  */
  return strcmp (a->msgid, b->msgid);
}


void
msgdomain_list_sort_by_filepos (msgdomain_list_ty *mdlp)
{
  size_t k;

  /* It makes sense to compare filepos[0] of different messages only after
     the filepos[] array of each message has been sorted.  Sort it now.  */
  msgdomain_list_sort_filepos (mdlp);

  for (k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      if (mlp->nitems > 0)
	qsort (mlp->item, mlp->nitems, sizeof (mlp->item[0]), cmp_by_filepos);
    }
}
