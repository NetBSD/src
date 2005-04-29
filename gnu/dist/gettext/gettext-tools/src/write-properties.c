/* Writing Java .properties files.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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

/* Specification.  */
#include "write-properties.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "message.h"
#include "msgl-ascii.h"
#include "msgl-iconv.h"
#include "po-charset.h"
#include "utf8-ucs4.h"
#include "write-po.h"
#include "xalloc.h"

/* The format of the Java .properties files is documented in the JDK
   documentation for class java.util.Properties.  In the case of .properties
   files for PropertyResourceBundle, for each message, the msgid becomes the
   key (left-hand side) and the msgstr becomes the value (right-hand side)
   of a "key=value" line.  Messages with plurals are not supported in this
   format.  */

/* Handling of comments: We copy all comments from the PO file to the
   .properties file. This is not really needed; it's a service for translators
   who don't like PO files and prefer to maintain the .properties file.  */

/* Converts a string to JAVA encoding (with \uxxxx sequences for non-ASCII
   characters).  */
static const char *
conv_to_java (const char *string)
{
  /* We cannot use iconv to "JAVA" because not all iconv() implementations
     know about the "JAVA" encoding.  */
  static const char hexdigit[] = "0123456789abcdef";
  size_t length;
  char *result;

  if (is_ascii_string (string))
    return string;

  length = 0;
  {
    const char *str = string;
    const char *str_limit = str + strlen (str);

    while (str < str_limit)
      {
	unsigned int uc;
	str += u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
	length += (uc <= 0x007f ? 1 : uc < 0x10000 ? 6 : 12);
      }
  }

  result = (char *) xmalloc (length + 1);

  {
    char *newstr = result;
    const char *str = string;
    const char *str_limit = str + strlen (str);

    while (str < str_limit)
      {
	unsigned int uc;
	str += u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
	if (uc <= 0x007f)
	  /* ASCII characters can be output literally.
	     We could treat non-ASCII ISO-8859-1 characters (0x0080..0x00FF)
	     the same way, but there is no point in doing this; Sun's
	     nativetoascii doesn't do it either.  */
	  *newstr++ = uc;
	else if (uc < 0x10000)
	  {
	    /* Single UCS-2 'char'  */
	    sprintf (newstr, "\\u%c%c%c%c",
		     hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
		     hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
	    newstr += 6;
	  }
	else
	  {
	    /* UTF-16 surrogate: two 'char's.  */
	    unsigned int uc1 = 0xd800 + ((uc - 0x10000) >> 10);
	    unsigned int uc2 = 0xdc00 + ((uc - 0x10000) & 0x3ff);
	    sprintf (newstr, "\\u%c%c%c%c",
		     hexdigit[(uc1 >> 12) & 0x0f], hexdigit[(uc1 >> 8) & 0x0f],
		     hexdigit[(uc1 >> 4) & 0x0f], hexdigit[uc1 & 0x0f]);
	    newstr += 6;
	    sprintf (newstr, "\\u%c%c%c%c",
		     hexdigit[(uc2 >> 12) & 0x0f], hexdigit[(uc2 >> 8) & 0x0f],
		     hexdigit[(uc2 >> 4) & 0x0f], hexdigit[uc2 & 0x0f]);
	    newstr += 6;
	  }
      }
    *newstr = '\0';
  }

  return result;
}

/* Writes a key or value to the file, without newline.  */
static void
write_escaped_string (FILE *fp, const char *str, bool in_key)
{
  static const char hexdigit[] = "0123456789abcdef";
  const char *str_limit = str + strlen (str);
  bool first = true;

  while (str < str_limit)
    {
      unsigned int uc;
      str += u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
      /* Whitespace must be escaped.  */
      if (uc == 0x0020 && (first || in_key))
	{
	  putc ('\\', fp);
	  putc (' ', fp);
	}
      else if (uc == 0x0009)
	{
	  putc ('\\', fp);
	  putc ('t', fp);
	}
      else if (uc == 0x000a)
	{
	  putc ('\\', fp);
	  putc ('n', fp);
	}
      else if (uc == 0x000d)
	{
	  putc ('\\', fp);
	  putc ('r', fp);
	}
      else if (uc == 0x000c)
	{
	  putc ('\\', fp);
	  putc ('f', fp);
	}
      else if (/* Backslash must be escaped.  */
	       uc == '\\'
	       /* Possible comment introducers must be escaped.  */
	       || uc == '#' || uc == '!'
	       /* Key terminators must be escaped.  */
	       || uc == '=' || uc == ':')
	{
	  putc ('\\', fp);
	  putc (uc, fp);
	}
      else if (uc >= 0x0020 && uc <= 0x007e)
	{
	  /* ASCII characters can be output literally.
	     We could treat non-ASCII ISO-8859-1 characters (0x0080..0x00FF)
	     the same way, but there is no point in doing this; Sun's
	     nativetoascii doesn't do it either.  */
	  putc (uc, fp);
	}
      else if (uc < 0x10000)
	{
	  /* Single UCS-2 'char'  */
	  fprintf (fp, "\\u%c%c%c%c",
		   hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
		   hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
	}
      else
	{
	  /* UTF-16 surrogate: two 'char's.  */
	  unsigned int uc1 = 0xd800 + ((uc - 0x10000) >> 10);
	  unsigned int uc2 = 0xdc00 + ((uc - 0x10000) & 0x3ff);
	  fprintf (fp, "\\u%c%c%c%c",
		   hexdigit[(uc1 >> 12) & 0x0f], hexdigit[(uc1 >> 8) & 0x0f],
		   hexdigit[(uc1 >> 4) & 0x0f], hexdigit[uc1 & 0x0f]);
	  fprintf (fp, "\\u%c%c%c%c",
		   hexdigit[(uc2 >> 12) & 0x0f], hexdigit[(uc2 >> 8) & 0x0f],
		   hexdigit[(uc2 >> 4) & 0x0f], hexdigit[uc2 & 0x0f]);
	}
      first = false;
    }
}

/* Writes a message to the file.  */
static void
write_message (FILE *fp, const message_ty *mp, size_t page_width, bool debug)
{
  /* Print translator comment if available.  */
  message_print_comment (mp, fp);

  /* Print xgettext extracted comments.  */
  message_print_comment_dot (mp, fp);

  /* Print the file position comments.  */
  message_print_comment_filepos (mp, fp, false, page_width);

  /* Print flag information in special comment.  */
  message_print_comment_flags (mp, fp, debug);

  /* Put a comment mark if the message is the header or untranslated or
     fuzzy.  */
  if (mp->msgid[0] == '\0'
      || mp->msgstr[0] == '\0'
      || (mp->is_fuzzy && mp->msgid[0] != '\0'))
    putc ('!', fp);

  /* Now write the untranslated string and the translated string.  */
  write_escaped_string (fp, mp->msgid, true);
  putc ('=', fp);
  write_escaped_string (fp, mp->msgstr, false);

  putc ('\n', fp);
}

/* Writes an entire message list to the file.  */
static void
write_properties (FILE *fp, message_list_ty *mlp, const char *canon_encoding,
		  size_t page_width, bool debug)
{
  bool blank_line;
  size_t j, i;

  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL);
  for (j = 0; j < mlp->nitems; ++j)
    {
      message_ty *mp = mlp->item[j];

      if (mp->comment != NULL)
	for (i = 0; i < mp->comment->nitems; ++i)
	  mp->comment->item[i] = conv_to_java (mp->comment->item[i]);
      if (mp->comment_dot != NULL)
	for (i = 0; i < mp->comment_dot->nitems; ++i)
	  mp->comment_dot->item[i] = conv_to_java (mp->comment_dot->item[i]);
    }

  /* Loop through the messages.  */
  blank_line = false;
  for (j = 0; j < mlp->nitems; ++j)
    {
      const message_ty *mp = mlp->item[j];

      if (mp->msgid_plural == NULL && !mp->obsolete)
	{
	  if (blank_line)
	    putc ('\n', fp);

	  write_message (fp, mp, page_width, debug);

	  blank_line = true;
	}
    }
}

/* Output the contents of a PO file in Java .properties syntax.  */
void
msgdomain_list_print_properties (msgdomain_list_ty *mdlp, FILE *fp,
				 size_t page_width, bool debug)
{
  message_list_ty *mlp;

  if (mdlp->nitems == 1)
    mlp = mdlp->item[0]->messages;
  else
    mlp = message_list_alloc (false);
  write_properties (fp, mlp, mdlp->encoding, page_width, debug);
}
