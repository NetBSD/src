/* Writing NeXTstep/GNUstep .strings files.
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
#include "write-stringtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "msgl-ascii.h"
#include "msgl-iconv.h"
#include "po-charset.h"
#include "strstr.h"
#include "write-po.h"

/* The format of NeXTstep/GNUstep .strings files is documented in
     gnustep-base-1.8.0/Tools/make_strings/Using.txt
   and in the comments of method propertyListFromStringsFileFormat in
     gnustep-base-1.8.0/Source/NSString.m
   In summary, it's a Objective-C like file with pseudo-assignments of the form
          "key" = "value";
   where the key is the msgid and the value is the msgstr.
 */

/* Handling of comments: We copy all comments from the PO file to the
   .strings file. This is not really needed; it's a service for translators
   who don't like PO files and prefer to maintain the .strings file.  */

/* Since the interpretation of text files in GNUstep depends on the locale's
   encoding if they don't have a BOM, we choose one of three encodings with
   a BOM: UCS-2BE, UCS-2LE, UTF-8.  Since the first two of these don't cope
   with all of Unicode and we don't know whether GNUstep will switch to
   UTF-16 instead of UCS-2, we use UTF-8 with BOM.  BOMs are bad because they
   get in the way when concatenating files, but here we have no choice.  */

/* Writes a key or value to the file, without newline.  */
static void
write_escaped_string (FILE *fp, const char *str)
{
  const char *str_limit = str + strlen (str);

  putc ('"', fp);
  while (str < str_limit)
    {
      unsigned char c = (unsigned char) *str++;

      if (c == '\t')
	{
	  putc ('\\', fp);
	  putc ('t', fp);
	}
      else if (c == '\n')
	{
	  putc ('\\', fp);
	  putc ('n', fp);
	}
      else if (c == '\r')
	{
	  putc ('\\', fp);
	  putc ('r', fp);
	}
      else if (c == '\f')
	{
	  putc ('\\', fp);
	  putc ('f', fp);
	}
      else if (c == '\\' || c == '"')
	{
	  putc ('\\', fp);
	  putc (c, fp);
	}
      else
	putc (c, fp);
    }
  putc ('"', fp);
}

/* Writes a message to the file.  */
static void
write_message (FILE *fp, const message_ty *mp, size_t page_width, bool debug)
{
  /* Print translator comment if available.  */
  if (mp->comment != NULL)
    {
      size_t j;

      for (j = 0; j < mp->comment->nitems; ++j)
	{
	  const char *s = mp->comment->item[j];

	  /* Test whether it is safe to output the comment in C style, or
	     whether we need C++ style for it.  */
	  if (strstr (s, "*/") == NULL)
	    {
	      fputs ("/*", fp);
	      if (*s != '\0' && *s != '\n' && *s != ' ')
		putc (' ', fp);
	      fputs (s, fp);
	      fputs (" */\n", fp);
	    }
	  else
	    do
	      {
		const char *e;
		fputs ("//", fp);
		if (*s != '\0' && *s != '\n' && *s != ' ')
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

  /* Print xgettext extracted comments.  */
  if (mp->comment_dot != NULL)
    {
      size_t j;

      for (j = 0; j < mp->comment_dot->nitems; ++j)
	{
	  const char *s = mp->comment_dot->item[j];

	  /* Test whether it is safe to output the comment in C style, or
	     whether we need C++ style for it.  */
	  if (strstr (s, "*/") == NULL)
	    {
	      fputs ("/* Comment: ", fp);
	      fputs (s, fp);
	      fputs (" */\n", fp);
	    }
	  else
	    {
	      bool first = true;
	      do
		{
		  const char *e;
		  fputs ("//", fp);
		  if (first || (*s != '\0' && *s != '\n' && *s != ' '))
		    putc (' ', fp);
		  if (first)
		    fputs ("Comment: ", fp);
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
		  first = false;
		}
	      while (s != NULL);
	    }
	}
    }

  /* Print the file position comments.  */
  if (mp->filepos_count != 0)
    {
      size_t j;

      for (j = 0; j < mp->filepos_count; ++j)
	{
	  lex_pos_ty *pp = &mp->filepos[j];
	  char *cp = pp->file_name;
	  while (cp[0] == '.' && cp[1] == '/')
	    cp += 2;
	  fprintf (fp, "/* File: %s:%ld */\n", cp, (long) pp->line_number);
	}
    }

  /* Print flag information in special comment.  */
  if (mp->is_fuzzy || mp->msgstr[0] == '\0')
    fputs ("/* Flag: untranslated */\n", fp);
  if (mp->obsolete)
    fputs ("/* Flag: unmatched */\n", fp);
  {
    size_t i;
    for (i = 0; i < NFORMATS; i++)
      if (significant_format_p (mp->is_format[i]))
	{
	  fputs ("/* Flag:", fp);
	  fputs (make_format_description_string (mp->is_format[i],
						 format_language[i], debug),
		 fp);
	  fputs (" */\n", fp);
	}
  }

  /* Now write the untranslated string and the translated string.  */
  write_escaped_string (fp, mp->msgid);
  fputs (" = ", fp);
  if (mp->msgstr[0] != '\0')
    {
      if (mp->is_fuzzy)
	{
	  /* Output the msgid as value, so that at runtime the untranslated
	     string is returned.  */
	  write_escaped_string (fp, mp->msgid);

	  /* Output the msgstr as a comment, so that at runtime
	     propertyListFromStringsFileFormat ignores it.  */
	  if (strstr (mp->msgstr, "*/") == NULL)
	    {
	      fputs (" /* = ", fp);
	      write_escaped_string (fp, mp->msgstr);
	      fputs (" */", fp);
	    }
	  else
	    {
	      fputs ("; // = ", fp);
	      write_escaped_string (fp, mp->msgstr);
	    }
	}
      else
	write_escaped_string (fp, mp->msgstr);
    }
  else
    {
      /* Output the msgid as value, so that at runtime the untranslated
	 string is returned.  */
      write_escaped_string (fp, mp->msgid);
    }
  putc (';', fp);

  putc ('\n', fp);
}

/* Writes an entire message list to the file.  */
static void
write_stringtable (FILE *fp, message_list_ty *mlp, const char *canon_encoding,
		   size_t page_width, bool debug)
{
  bool blank_line;
  size_t j;

  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL);

  /* Output the BOM.  */
  if (!is_ascii_message_list (mlp))
    fputs ("\xef\xbb\xbf", fp);

  /* Loop through the messages.  */
  blank_line = false;
  for (j = 0; j < mlp->nitems; ++j)
    {
      const message_ty *mp = mlp->item[j];

      if (mp->msgid_plural == NULL)
	{
	  if (blank_line)
	    putc ('\n', fp);

	  write_message (fp, mp, page_width, debug);

	  blank_line = true;
	}
    }
}

/* Output the contents of a PO file in .strings syntax.  */
void
msgdomain_list_print_stringtable (msgdomain_list_ty *mdlp, FILE *fp,
				  size_t page_width, bool debug)
{
  message_list_ty *mlp;

  if (mdlp->nitems == 1)
    mlp = mdlp->item[0]->messages;
  else
    mlp = message_list_alloc (false);
  write_stringtable (fp, mlp, mdlp->encoding, page_width, debug);
}
