/* GNU gettext - internationalization aids
   Copyright (C) 1995-1998, 2000-2006 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "write-catalog.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fwriteerror.h"
#include "error-progname.h"
#include "xvasprintf.h"
#include "po-xerror.h"
#include "gettext.h"

/* Our regular abbreviation.  */
#define _(str) gettext (str)


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


/* ======================== msgdomain_list_print() ======================== */


void
msgdomain_list_print (msgdomain_list_ty *mdlp, const char *filename,
		      catalog_output_format_ty output_syntax,
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
		|| (mlp->nitems == 1 && is_header (mlp->item[0]))))
	    {
	      found_nonempty = true;
	      break;
	    }
	}

      if (!found_nonempty)
	return;
    }

  /* Check whether the output format can accomodate all messages.  */
  if (!output_syntax->supports_multiple_domains && mdlp->nitems > 1)
    {
      if (output_syntax->alternative_is_po)
	po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false, _("\
Cannot output multiple translation domains into a single file with the specified output format. Try using PO file syntax instead."));
      else
	po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false, _("\
Cannot output multiple translation domains into a single file with the specified output format."));
    }
  else
    {
      if (!output_syntax->supports_contexts)
	{
	  const lex_pos_ty *has_context;
	  size_t k;

	  has_context = NULL;
	  for (k = 0; k < mdlp->nitems; k++)
	    {
	      message_list_ty *mlp = mdlp->item[k]->messages;
	      size_t j;

	      for (j = 0; j < mlp->nitems; j++)
		{
		  message_ty *mp = mlp->item[j];

		  if (mp->msgctxt != NULL)
		    {
		      has_context = &mp->pos;
		      break;
		    }
		}
	    }

	  if (has_context != NULL)
	    {
	      error_with_progname = false;
	      po_xerror (PO_SEVERITY_FATAL_ERROR, NULL,
			 has_context->file_name, has_context->line_number,
			 (size_t)(-1), false, _("\
message catalog has context dependent translations, but the output format does not support them."));
	      error_with_progname = true;
	    }
	}

      if (!output_syntax->supports_plurals)
	{
	  const lex_pos_ty *has_plural;
	  size_t k;

	  has_plural = NULL;
	  for (k = 0; k < mdlp->nitems; k++)
	    {
	      message_list_ty *mlp = mdlp->item[k]->messages;
	      size_t j;

	      for (j = 0; j < mlp->nitems; j++)
		{
		  message_ty *mp = mlp->item[j];

		  if (mp->msgid_plural != NULL)
		    {
		      has_plural = &mp->pos;
		      break;
		    }
		}
	    }

	  if (has_plural != NULL)
	    {
	      error_with_progname = false;
	      if (output_syntax->alternative_is_java_class)
		po_xerror (PO_SEVERITY_FATAL_ERROR, NULL,
			   has_plural->file_name, has_plural->line_number,
			   (size_t)(-1), false, _("\
message catalog has plural form translations, but the output format does not support them. Try generating a Java class using \"msgfmt --java\", instead of a properties file."));
	      else
		po_xerror (PO_SEVERITY_FATAL_ERROR, NULL,
			   has_plural->file_name, has_plural->line_number,
			   (size_t)(-1), false, _("\
message catalog has plural form translations, but the output format does not support them."));
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
	{
	  const char *errno_description = strerror (errno);
	  po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
		     xasprintf ("%s: %s",
				xasprintf (_("cannot create output file \"%s\""),
					   filename),
				errno_description));
	}
    }
  else
    {
      fp = stdout;
      /* xgettext:no-c-format */
      filename = _("standard output");
    }

  output_syntax->print (mdlp, fp, page_width, debug);

  /* Make sure nothing went wrong.  */
  if (fwriteerror (fp))
    {
      const char *errno_description = strerror (errno);
      po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
		 xasprintf ("%s: %s",
			    xasprintf (_("error while writing \"%s\" file"),
				       filename),
			    errno_description));
    }
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
