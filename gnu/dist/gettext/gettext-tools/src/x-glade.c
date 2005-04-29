/* xgettext glade backend.
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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if DYNLOAD_LIBEXPAT
# include <dlfcn.h>
#else
# if HAVE_LIBEXPAT
#  include <expat.h>
# endif
#endif

#include "message.h"
#include "xgettext.h"
#include "x-glade.h"
#include "error.h"
#include "xerror.h"
#include "basename.h"
#include "progname.h"
#include "xalloc.h"
#include "exit.h"
#include "hash.h"
#include "po-charset.h"
#include "gettext.h"

#define _(s) gettext(s)


/* glade is an XML based format.  Some example files are contained in
   libglade-0.16.  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

static hash_table keywords;
static bool default_keywords = true;


void
x_glade_extract_all ()
{
  extract_all = true;
}


void
x_glade_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (keywords.table == NULL)
	init_hash (&keywords, 100);

      insert_entry (&keywords, name, strlen (name), NULL);
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      x_glade_keyword ("label");
      x_glade_keyword ("title");
      x_glade_keyword ("text");
      x_glade_keyword ("format");
      x_glade_keyword ("copyright");
      x_glade_keyword ("comments");
      x_glade_keyword ("preview_text");
      x_glade_keyword ("tooltip");
      default_keywords = false;
    }
}


/* ===================== Dynamic loading of libexpat.  ===================== */

#if DYNLOAD_LIBEXPAT

typedef void *XML_Parser;
typedef char XML_Char;
typedef char XML_LChar;
enum XML_Error { XML_ERROR_NONE };
typedef void (*XML_StartElementHandler) (void *userData, const XML_Char *name, const XML_Char **atts);
typedef void (*XML_EndElementHandler) (void *userData, const XML_Char *name);
typedef void (*XML_CharacterDataHandler) (void *userData, const XML_Char *s, int len);
typedef void (*XML_CommentHandler) (void *userData, const XML_Char *data);

static XML_Parser (*p_XML_ParserCreate) (const XML_Char *encoding);
static void (*p_XML_SetElementHandler) (XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end);
static void (*p_XML_SetCharacterDataHandler) (XML_Parser parser, XML_CharacterDataHandler handler);
static void (*p_XML_SetCommentHandler) (XML_Parser parser, XML_CommentHandler handler);
static int (*p_XML_Parse) (XML_Parser parser, const char *s, int len, int isFinal);
static enum XML_Error (*p_XML_GetErrorCode) (XML_Parser parser);
static int (*p_XML_GetCurrentLineNumber) (XML_Parser parser);
static int (*p_XML_GetCurrentColumnNumber) (XML_Parser parser);
static void (*p_XML_ParserFree) (XML_Parser parser);
static const XML_LChar * (*p_XML_ErrorString) (int code);

#define XML_ParserCreate (*p_XML_ParserCreate)
#define XML_SetElementHandler (*p_XML_SetElementHandler)
#define XML_SetCharacterDataHandler (*p_XML_SetCharacterDataHandler)
#define XML_SetCommentHandler (*p_XML_SetCommentHandler)
#define XML_Parse (*p_XML_Parse)
#define XML_GetErrorCode (*p_XML_GetErrorCode)
#define XML_GetCurrentLineNumber (*p_XML_GetCurrentLineNumber)
#define XML_GetCurrentColumnNumber (*p_XML_GetCurrentColumnNumber)
#define XML_ParserFree (*p_XML_ParserFree)
#define XML_ErrorString (*p_XML_ErrorString)

static int libexpat_loaded = 0;

static bool
load_libexpat ()
{
  if (libexpat_loaded == 0)
    {
      void *handle = dlopen ("libexpat.so.0", RTLD_LAZY);
      if (handle != NULL
	  && (p_XML_ParserCreate = dlsym (handle, "XML_ParserCreate")) != NULL
	  && (p_XML_SetElementHandler = dlsym (handle, "XML_SetElementHandler")) != NULL
	  && (p_XML_SetCharacterDataHandler = dlsym (handle, "XML_SetCharacterDataHandler")) != NULL
	  && (p_XML_SetCommentHandler = dlsym (handle, "XML_SetCommentHandler")) != NULL
	  && (p_XML_Parse = dlsym (handle, "XML_Parse")) != NULL
	  && (p_XML_GetErrorCode = dlsym (handle, "XML_GetErrorCode")) != NULL
	  && (p_XML_GetCurrentLineNumber = dlsym (handle, "XML_GetCurrentLineNumber")) != NULL
	  && (p_XML_GetCurrentColumnNumber = dlsym (handle, "XML_GetCurrentColumnNumber")) != NULL
	  && (p_XML_ParserFree = dlsym (handle, "XML_ParserFree")) != NULL
	  && (p_XML_ErrorString = dlsym (handle, "XML_ErrorString")) != NULL)
	libexpat_loaded = 1;
      else
	libexpat_loaded = -1;
    }
  return libexpat_loaded >= 0;
}

#define LIBEXPAT_AVAILABLE() (load_libexpat ())

#elif HAVE_LIBEXPAT

#define LIBEXPAT_AVAILABLE() true

#endif

/* ============================= XML parsing.  ============================= */

#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT

/* Accumulator for the extracted messages.  */
static message_list_ty *mlp;

/* Logical filename, used to label the extracted messages.  */
static char *logical_file_name;

/* XML parser.  */
static XML_Parser parser;

struct element_state
{
  bool extract_string;
  int lineno;
  char *buffer;
  size_t bufmax;
  size_t buflen;
};
static struct element_state *stack;
static size_t stack_size;

/* Ensures stack_size >= size.  */
static void
ensure_stack_size (size_t size)
{
  if (size > stack_size)
    {
      stack_size = 2 * stack_size;
      if (stack_size < size)
	stack_size = size;
      stack =
	(struct element_state *)
	xrealloc (stack, stack_size * sizeof (struct element_state));
    }
}

static size_t stack_depth;

/* Callback called when <element> is seen.  */
static void
start_element_handler (void *userData, const char *name,
		       const char **attributes)
{
  struct element_state *p;
  void *hash_result;

  /* Increase stack depth.  */
  stack_depth++;
  ensure_stack_size (stack_depth + 1);

  /* Don't extract a string for the containing element.  */
  stack[stack_depth - 1].extract_string = false;

  p = &stack[stack_depth];
  p->extract_string = extract_all;
  /* In Glade 1, a few specific elements are translatable.  */
  if (!p->extract_string)
    p->extract_string =
      (find_entry (&keywords, name, strlen (name), &hash_result) == 0);
  /* In Glade 2, all <property> and <atkproperty> elements are translatable
     that have the attribute translatable="yes".  */
  if (!p->extract_string
      && (strcmp (name, "property") == 0 || strcmp (name, "atkproperty") == 0))
    {
      bool has_translatable = false;
      const char **attp = attributes;
      while (*attp != NULL)
	{
	  if (strcmp (attp[0], "translatable") == 0)
	    {
	      has_translatable = (strcmp (attp[1], "yes") == 0);
	      break;
	    }
	  attp += 2;
	}
      p->extract_string = has_translatable;
    }
  if (!p->extract_string
      && strcmp (name, "atkaction") == 0)
    {
      const char **attp = attributes;
      while (*attp != NULL)
	{
	  if (strcmp (attp[0], "description") == 0)
	    {
	      if (strcmp (attp[1], "") != 0)
		{
		  lex_pos_ty pos;

		  pos.file_name = logical_file_name;
		  pos.line_number = XML_GetCurrentLineNumber (parser);

		  remember_a_message (mlp, xstrdup (attp[1]),
				      null_context, &pos);
		}
	      break;
	    }
	  attp += 2;
	}
    }
  p->lineno = XML_GetCurrentLineNumber (parser);
  p->buffer = NULL;
  p->bufmax = 0;
  p->buflen = 0;
  if (!p->extract_string)
    xgettext_comment_reset ();
}

/* Callback called when </element> is seen.  */
static void
end_element_handler (void *userData, const char *name)
{
  struct element_state *p = &stack[stack_depth];

  /* Actually extract string.  */
  if (p->extract_string)
    {
      /* Don't extract the empty string.  */
      if (p->buflen > 0)
	{
	  lex_pos_ty pos;

	  if (p->buflen == p->bufmax)
	    p->buffer = (char *) xrealloc (p->buffer, p->buflen + 1);
	  p->buffer[p->buflen] = '\0';

	  pos.file_name = logical_file_name;
	  pos.line_number = p->lineno;

	  remember_a_message (mlp, p->buffer, null_context, &pos);
	  p->buffer = NULL;
	}
    }

  /* Free memory for this stack level.  */
  if (p->buffer != NULL)
    free (p->buffer);

  /* Decrease stack depth.  */
  stack_depth--;

  xgettext_comment_reset ();
}

/* Callback called when some text is seen.  */
static void
character_data_handler (void *userData, const char *s, int len)
{
  struct element_state *p = &stack[stack_depth];

  /* Accumulate character data.  */
  if (len > 0)
    {
      if (p->buflen + len > p->bufmax)
	{
	  p->bufmax = 2 * p->bufmax;
	  if (p->bufmax < p->buflen + len)
	    p->bufmax = p->buflen + len;
	  p->buffer = (char *) xrealloc (p->buffer, p->bufmax);
	}
      memcpy (p->buffer + p->buflen, s, len);
      p->buflen += len;
    }
}

/* Callback called when some comment text is seen.  */
static void
comment_handler (void *userData, const char *data)
{
  /* Split multiline comment into lines, and remove leading and trailing
     whitespace.  */
  char *copy = xstrdup (data);
  char *p = copy;
  char *q;

  for (p = copy; (q = strchr (p, '\n')) != NULL; p = q + 1)
    {
      while (p[0] == ' ' || p[0] == '\t')
	p++;
      while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
	q--;
      *q = '\0';
      xgettext_comment_add (p);
    }
  q = p + strlen (p);
  while (p[0] == ' ' || p[0] == '\t')
    p++;
  while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
    q--;
  *q = '\0';
  xgettext_comment_add (p);
  free (copy);
}


static void
do_extract_glade (FILE *fp,
		  const char *real_filename, const char *logical_filename,
		  msgdomain_list_ty *mdlp)
{
  mlp = mdlp->item[0]->messages;

  /* expat feeds us strings in UTF-8 encoding.  */
  xgettext_current_source_encoding = po_charset_utf8;

  logical_file_name = xstrdup (logical_filename);

  init_keywords ();

  parser = XML_ParserCreate (NULL);
  if (parser == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  XML_SetElementHandler (parser, start_element_handler, end_element_handler);
  XML_SetCharacterDataHandler (parser, character_data_handler);
  XML_SetCommentHandler (parser, comment_handler);

  stack_depth = 0;

  while (!feof (fp))
    {
      char buf[4096];
      int count = fread (buf, 1, sizeof buf, fp);

      if (count == 0)
	{
	  if (ferror (fp))
	    error (EXIT_FAILURE, errno, _("\
error while reading \"%s\""), real_filename);
	  /* EOF reached.  */
	  break;
	}

      if (XML_Parse (parser, buf, count, 0) == 0)
	error (EXIT_FAILURE, 0, _("%s:%d:%d: %s"), logical_filename,
	       XML_GetCurrentLineNumber (parser),
	       XML_GetCurrentColumnNumber (parser) + 1,
	       XML_ErrorString (XML_GetErrorCode (parser)));
    }

  if (XML_Parse (parser, NULL, 0, 1) == 0)
    error (EXIT_FAILURE, 0, _("%s:%d:%d: %s"), logical_filename,
	   XML_GetCurrentLineNumber (parser),
	   XML_GetCurrentColumnNumber (parser) + 1,
	   XML_ErrorString (XML_GetErrorCode (parser)));

  XML_ParserFree (parser);

  /* Close scanner.  */
  logical_file_name = NULL;
  parser = NULL;
}

#endif

void
extract_glade (FILE *fp,
	       const char *real_filename, const char *logical_filename,
	       flag_context_list_table_ty *flag_table,
	       msgdomain_list_ty *mdlp)
{
#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT
  if (LIBEXPAT_AVAILABLE ())
    do_extract_glade (fp, real_filename, logical_filename, mdlp);
  else
#endif
    {
      multiline_error (xstrdup (""),
		       xasprintf (_("\
Language \"glade\" is not supported. %s relies on expat.\n\
This version was built without expat.\n"),
				  basename (program_name)));
      exit (EXIT_FAILURE);
    }
}
