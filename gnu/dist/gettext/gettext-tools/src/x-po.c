/* xgettext PO and JavaProperties backends.
   Copyright (C) 1995-1998, 2000-2003, 2005 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "message.h"
#include "xgettext.h"
#include "x-po.h"
#include "x-properties.h"
#include "x-stringtable.h"
#include "xalloc.h"
#include "read-po.h"
#include "po-lex.h"
#include "gettext.h"

/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


/* The charset found in the header entry.  */
static char *header_charset;

/* Define a subclass extract_po_reader_ty of default_po_reader_ty.  */

static void
extract_add_message (default_po_reader_ty *this,
		     char *msgid,
		     lex_pos_ty *msgid_pos,
		     char *msgid_plural,
		     char *msgstr, size_t msgstr_len,
		     lex_pos_ty *msgstr_pos,
		     bool force_fuzzy, bool obsolete)
{
  /* See whether we shall exclude this message.  */
  if (exclude != NULL && message_list_search (exclude, msgid) != NULL)
    goto discard;

  /* If the msgid is the empty string, it is the old header.  Throw it
     away, we have constructed a new one.  Only remember its charset.
     But if no new one was constructed, keep the old header.  This is useful
     because the old header may contain a charset= directive.  */
  if (*msgid == '\0' && !xgettext_omit_header)
    {
      const char *charsetstr = strstr (msgstr, "charset=");

      if (charsetstr != NULL)
	{
	  size_t len;
	  char *charset;

	  charsetstr += strlen ("charset=");
	  len = strcspn (charsetstr, " \t\n");
	  charset = (char *) xmalloc (len + 1);
	  memcpy (charset, charsetstr, len);
	  charset[len] = '\0';

	  if (header_charset != NULL)
	    free (header_charset);
	  header_charset = charset;
	}

     discard:
      free (msgid);
      free (msgstr);
      return;
    }

  /* Invoke superclass method.  */
  default_add_message (this, msgid, msgid_pos, msgid_plural,
		       msgstr, msgstr_len, msgstr_pos, force_fuzzy, obsolete);
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static default_po_reader_class_ty extract_methods =
{
  {
    sizeof (default_po_reader_ty),
    default_constructor,
    default_destructor,
    default_parse_brief,
    default_parse_debrief,
    default_directive_domain,
    default_directive_message,
    default_comment,
    default_comment_dot,
    default_comment_filepos,
    default_comment_special
  },
  default_set_domain, /* set_domain */
  extract_add_message, /* add_message */
  NULL /* frob_new_message */
};


static void
extract (FILE *fp,
	 const char *real_filename, const char *logical_filename,
	 input_syntax_ty syntax,
	 msgdomain_list_ty *mdlp)
{
  default_po_reader_ty *pop;

  header_charset = NULL;

  pop = default_po_reader_alloc (&extract_methods);
  pop->handle_comments = true;
  pop->handle_filepos_comments = (line_comment != 0);
  pop->allow_domain_directives = false;
  pop->allow_duplicates = false;
  pop->allow_duplicates_if_same_msgstr = true;
  pop->mdlp = NULL;
  pop->mlp = mdlp->item[0]->messages;
  po_scan ((abstract_po_reader_ty *) pop, fp, real_filename, logical_filename,
	   syntax);
  po_reader_free ((abstract_po_reader_ty *) pop);

  if (header_charset != NULL)
    {
      if (!xgettext_omit_header)
	{
	  /* Put the old charset into the freshly constructed header entry.  */
	  message_ty *mp = message_list_search (mdlp->item[0]->messages, "");

	  if (mp != NULL && !mp->obsolete)
	    {
	      const char *header = mp->msgstr;

	      if (header != NULL)
		{
		  const char *charsetstr = strstr (header, "charset=");

		  if (charsetstr != NULL)
		    {
		      size_t len, len1, len2, len3;
		      char *new_header;

		      charsetstr += strlen ("charset=");
		      len = strcspn (charsetstr, " \t\n");

		      len1 = charsetstr - header;
		      len2 = strlen (header_charset);
		      len3 = (header + strlen (header)) - (charsetstr + len);
		      new_header = (char *) xmalloc (len1 + len2 + len3 + 1);
		      memcpy (new_header, header, len1);
		      memcpy (new_header + len1, header_charset, len2);
		      memcpy (new_header + len1 + len2, charsetstr + len, len3 + 1);
		      mp->msgstr = new_header;
		      mp->msgstr_len = len1 + len2 + len3 + 1;
		    }
		}
	    }
	}

      free (header_charset);
    }
}


void
extract_po (FILE *fp,
	    const char *real_filename, const char *logical_filename,
	    flag_context_list_table_ty *flag_table,
	    msgdomain_list_ty *mdlp)
{
  extract (fp, real_filename,  logical_filename, syntax_po, mdlp);
}


void
extract_properties (FILE *fp,
		    const char *real_filename, const char *logical_filename,
		    flag_context_list_table_ty *flag_table,
		    msgdomain_list_ty *mdlp)
{
  extract (fp, real_filename,  logical_filename, syntax_properties, mdlp);
}


void
extract_stringtable (FILE *fp,
		     const char *real_filename, const char *logical_filename,
		     flag_context_list_table_ty *flag_table,
		     msgdomain_list_ty *mdlp)
{
  extract (fp, real_filename,  logical_filename, syntax_stringtable, mdlp);
}
