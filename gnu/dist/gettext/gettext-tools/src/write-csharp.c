/* Writing C# satellite assemblies.
   Copyright (C) 2003-2005 Free Software Foundation, Inc.
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
#include <alloca.h>

/* Specification.  */
#include "write-csharp.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#if STAT_MACROS_BROKEN
# undef S_ISDIR
#endif
#if !defined S_ISDIR && defined S_IFDIR
# define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#if !S_IRUSR && S_IREAD
# define S_IRUSR S_IREAD
#endif
#if !S_IRUSR
# define S_IRUSR 00400
#endif
#if !S_IWUSR && S_IWRITE
# define S_IWUSR S_IWRITE
#endif
#if !S_IWUSR
# define S_IWUSR 00200
#endif
#if !S_IXUSR && S_IEXEC
# define S_IXUSR S_IEXEC
#endif
#if !S_IXUSR
# define S_IXUSR 00100
#endif
#if !S_IRGRP
# define S_IRGRP (S_IRUSR >> 3)
#endif
#if !S_IWGRP
# define S_IWGRP (S_IWUSR >> 3)
#endif
#if !S_IXGRP
# define S_IXGRP (S_IXUSR >> 3)
#endif
#if !S_IROTH
# define S_IROTH (S_IRUSR >> 6)
#endif
#if !S_IWOTH
# define S_IWOTH (S_IWUSR >> 6)
#endif
#if !S_IXOTH
# define S_IXOTH (S_IXUSR >> 6)
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef __MINGW32__
/* mingw's mkdir() function has 1 argument, but we pass 2 arguments.
   Therefore we have to disable the argument count checking.  */
# define mkdir ((int (*)()) mkdir)
#endif

#include "c-ctype.h"
#include "error.h"
#include "relocatable.h"
#include "csharpcomp.h"
#include "message.h"
#include "mkdtemp.h"
#include "msgfmt.h"
#include "msgl-iconv.h"
#include "pathmax.h"
#include "plural-exp.h"
#include "po-charset.h"
#include "xalloc.h"
#include "xallocsa.h"
#include "pathname.h"
#include "fatal-signal.h"
#include "fwriteerror.h"
#include "tmpdir.h"
#include "utf8-ucs4.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Convert a resource name to a class name.
   Return a nonempty string consisting of alphanumerics and underscores
   and starting with a letter or underscore.  */
static char *
construct_class_name (const char *resource_name)
{
  /* This code must be kept consistent with intl.cs, function
     GettextResourceManager.ConstructClassName.  */
  /* We could just return an arbitrary fixed class name, like "Messages",
     assuming that every assembly will only ever contain one
     GettextResourceSet subclass, but this assumption would break the day
     we want to support multi-domain PO files in the same format...  */
  bool valid;
  const char *p;

  /* Test for a valid ASCII identifier:
     - nonempty,
     - first character is A..Za..z_ - see x-csharp.c:is_identifier_start.
     - next characters are A..Za..z_0..9 - see x-csharp.c:is_identifier_part.
   */
  valid = (resource_name[0] != '\0');
  for (p = resource_name; valid && *p != '\0'; p++)
    {
      char c = *p;
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_')
	    || (p > resource_name && c >= '0' && c <= '9')))
	valid = false;
    }
  if (valid)
    return xstrdup (resource_name);
  else
    {
      static const char hexdigit[] = "0123456789abcdef";
      const char *str = resource_name;
      const char *str_limit = str + strlen (str);
      char *class_name = (char *) xmalloc (12 + 6 * (str_limit - str) + 1);
      char *b;

      b = class_name;
      memcpy (b, "__UESCAPED__", 12); b += 12;
      while (str < str_limit)
	{
	  unsigned int uc;
	  str += u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
	  if (uc >= 0x10000)
	    {
	      *b++ = '_';
	      *b++ = 'U';
	      *b++ = hexdigit[(uc >> 28) & 0x0f];
	      *b++ = hexdigit[(uc >> 24) & 0x0f];
	      *b++ = hexdigit[(uc >> 20) & 0x0f];
	      *b++ = hexdigit[(uc >> 16) & 0x0f];
	      *b++ = hexdigit[(uc >> 12) & 0x0f];
	      *b++ = hexdigit[(uc >> 8) & 0x0f];
	      *b++ = hexdigit[(uc >> 4) & 0x0f];
	      *b++ = hexdigit[uc & 0x0f];
	    }
	  else if (!((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z')
		     || (uc >= '0' && uc <= '9')))
	    {
	      *b++ = '_';
	      *b++ = 'u';
	      *b++ = hexdigit[(uc >> 12) & 0x0f];
	      *b++ = hexdigit[(uc >> 8) & 0x0f];
	      *b++ = hexdigit[(uc >> 4) & 0x0f];
	      *b++ = hexdigit[uc & 0x0f];
	    }
	  else
	    *b++ = uc;
	}
      *b++ = '\0';
      return (char *) xrealloc (class_name, b - class_name);
    }
}


/* Write a string in C# Unicode notation to the given stream.  */
static void
write_csharp_string (FILE *stream, const char *str)
{
  static const char hexdigit[] = "0123456789abcdef";
  const char *str_limit = str + strlen (str);

  fprintf (stream, "\"");
  while (str < str_limit)
    {
      unsigned int uc;
      str += u8_mbtouc (&uc, (const unsigned char *) str, str_limit - str);
      if (uc == 0x0000)
	fprintf (stream, "\\0");
      else if (uc == 0x0007)
	fprintf (stream, "\\a");
      else if (uc == 0x0008)
	fprintf (stream, "\\b");
      else if (uc == 0x0009)
	fprintf (stream, "\\t");
      else if (uc == 0x000a)
	fprintf (stream, "\\n");
      else if (uc == 0x000b)
	fprintf (stream, "\\v");
      else if (uc == 0x000c)
	fprintf (stream, "\\f");
      else if (uc == 0x000d)
	fprintf (stream, "\\r");
      else if (uc == 0x0022)
	fprintf (stream, "\\\"");
      else if (uc == 0x005c)
	fprintf (stream, "\\\\");
      else if (uc >= 0x0020 && uc < 0x007f)
	fprintf (stream, "%c", uc);
      else if (uc < 0x10000)
	fprintf (stream, "\\u%c%c%c%c",
		 hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
		 hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
      else
	fprintf (stream, "\\U%c%c%c%c%c%c%c%c",
		 hexdigit[(uc >> 28) & 0x0f], hexdigit[(uc >> 24) & 0x0f],
		 hexdigit[(uc >> 20) & 0x0f], hexdigit[(uc >> 16) & 0x0f],
		 hexdigit[(uc >> 12) & 0x0f], hexdigit[(uc >> 8) & 0x0f],
		 hexdigit[(uc >> 4) & 0x0f], hexdigit[uc & 0x0f]);
    }
  fprintf (stream, "\"");
}


/* Write C# code that returns the value for a message.  If the message
   has plural forms, it is an expression of type System.String[], otherwise it
   is an expression of type System.String.  */
static void
write_csharp_msgstr (FILE *stream, message_ty *mp)
{
  if (mp->msgid_plural != NULL)
    {
      bool first;
      const char *p;

      fprintf (stream, "new System.String[] { ");
      for (p = mp->msgstr, first = true;
	   p < mp->msgstr + mp->msgstr_len;
	   p += strlen (p) + 1, first = false)
	{
	  if (!first)
	    fprintf (stream, ", ");
	  write_csharp_string (stream, p);
	}
      fprintf (stream, " }");
    }
  else
    {
      if (mp->msgstr_len != strlen (mp->msgstr) + 1)
	abort ();

      write_csharp_string (stream, mp->msgstr);
    }
}


/* Tests whether a plural expression, evaluated according to the C rules,
   can only produce the values 0 and 1.  */
static bool
is_expression_boolean (struct expression *exp)
{
  switch (exp->operation)
    {
    case var:
    case mult:
    case divide:
    case module:
    case plus:
    case minus:
      return false;
    case lnot:
    case less_than:
    case greater_than:
    case less_or_equal:
    case greater_or_equal:
    case equal:
    case not_equal:
    case land:
    case lor:
      return true;
    case num:
      return (exp->val.num == 0 || exp->val.num == 1);
    case qmop:
      return is_expression_boolean (exp->val.args[1])
	     && is_expression_boolean (exp->val.args[2]);
    default:
      abort ();
    }
}


/* Write C# code that evaluates a plural expression according to the C rules.
   The variable is called 'n'.  */
static void
write_csharp_expression (FILE *stream, struct expression *exp, bool as_boolean)
{
  /* We use parentheses everywhere.  This frees us from tracking the priority
     of arithmetic operators.  */
  if (as_boolean)
    {
      /* Emit a C# expression of type 'bool'.  */
      switch (exp->operation)
	{
	case num:
	  fprintf (stream, "%s", exp->val.num ? "true" : "false");
	  return;
	case lnot:
	  fprintf (stream, "(!");
	  write_csharp_expression (stream, exp->val.args[0], true);
	  fprintf (stream, ")");
	  return;
	case less_than:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " < ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case greater_than:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " > ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case less_or_equal:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " <= ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case greater_or_equal:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " >= ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case equal:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " == ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case not_equal:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " != ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case land:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], true);
	  fprintf (stream, " && ");
	  write_csharp_expression (stream, exp->val.args[1], true);
	  fprintf (stream, ")");
	  return;
	case lor:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], true);
	  fprintf (stream, " || ");
	  write_csharp_expression (stream, exp->val.args[1], true);
	  fprintf (stream, ")");
	  return;
	case qmop:
	  if (is_expression_boolean (exp->val.args[1])
	      && is_expression_boolean (exp->val.args[2]))
	    {
	      fprintf (stream, "(");
	      write_csharp_expression (stream, exp->val.args[0], true);
	      fprintf (stream, " ? ");
	      write_csharp_expression (stream, exp->val.args[1], true);
	      fprintf (stream, " : ");
	      write_csharp_expression (stream, exp->val.args[2], true);
	      fprintf (stream, ")");
	      return;
	    }
	  /*FALLTHROUGH*/
	case var:
	case mult:
	case divide:
	case module:
	case plus:
	case minus:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp, false);
	  fprintf (stream, " != 0)");
	  return;
	default:
	  abort ();
	}
    }
  else
    {
      /* Emit a C# expression of type 'long'.  */
      switch (exp->operation)
	{
	case var:
	  fprintf (stream, "n");
	  return;
	case num:
	  fprintf (stream, "%lu", exp->val.num);
	  return;
	case mult:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " * ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case divide:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " / ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case module:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " %% ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case plus:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " + ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case minus:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], false);
	  fprintf (stream, " - ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, ")");
	  return;
	case qmop:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp->val.args[0], true);
	  fprintf (stream, " ? ");
	  write_csharp_expression (stream, exp->val.args[1], false);
	  fprintf (stream, " : ");
	  write_csharp_expression (stream, exp->val.args[2], false);
	  fprintf (stream, ")");
	  return;
	case lnot:
	case less_than:
	case greater_than:
	case less_or_equal:
	case greater_or_equal:
	case equal:
	case not_equal:
	case land:
	case lor:
	  fprintf (stream, "(");
	  write_csharp_expression (stream, exp, true);
	  fprintf (stream, " ? 1 : 0)");
	  return;
	default:
	  abort ();
	}
    }
}


/* Write the C# code for the GettextResourceSet subclass to the given stream.
   Note that we use fully qualified class names and no "using" statements,
   because applications can have their own classes called X.Y.Hashtable or
   X.Y.String.  */
static void
write_csharp_code (FILE *stream, const char *class_name, message_list_ty *mlp)
{
  const char *last_dot;
  const char *class_name_last_part;
  unsigned int plurals;
  size_t j;

  fprintf (stream,
	   "/* Automatically generated by GNU msgfmt.  Do not modify!  */\n");
  /* We have to use a "using" statement here, to avoid a bug in the pnet-0.6.0
     compiler.  */
  fprintf (stream, "using GNU.Gettext;\n");
  last_dot = strrchr (class_name, '.');
  if (last_dot != NULL)
    {
      fprintf (stream, "namespace ");
      fwrite (class_name, 1, last_dot - class_name, stream);
      fprintf (stream, " {\n");
      class_name_last_part = last_dot + 1;
    }
  else
    class_name_last_part = class_name;
  fprintf (stream, "public class %s : GettextResourceSet {\n",
	   class_name_last_part);

  /* Determine whether there are plural messages.  */
  plurals = 0;
  for (j = 0; j < mlp->nitems; j++)
    if (mlp->item[j]->msgid_plural != NULL)
      plurals++;

  /* Emit the constructor.  */
  fprintf (stream, "  public %s ()\n", class_name_last_part);
  fprintf (stream, "    : base () {\n");
  fprintf (stream, "  }\n");

  /* Emit the ReadResources method.  */
  fprintf (stream, "  protected override void ReadResources () {\n");
  /* In some implementations, the ResourceSet constructor initializes Table
     before calling ReadResources().  In other implementations, the
     ReadResources() method is expected to initialize the Table.  */
  fprintf (stream, "    if (Table == null)\n");
  fprintf (stream, "      Table = new System.Collections.Hashtable();\n");
  fprintf (stream, "    System.Collections.Hashtable t = Table;\n");
  for (j = 0; j < mlp->nitems; j++)
    {
      fprintf (stream, "    t.Add(");
      write_csharp_string (stream, mlp->item[j]->msgid);
      fprintf (stream, ",");
      write_csharp_msgstr (stream, mlp->item[j]);
      fprintf (stream, ");\n");
    }
  fprintf (stream, "  }\n");

  /* Emit the msgid_plural strings.  Only used by msgunfmt.  */
  if (plurals)
    {
      fprintf (stream, "  public static System.Collections.Hashtable GetMsgidPluralTable () {\n");
      fprintf (stream, "    System.Collections.Hashtable t = new System.Collections.Hashtable();\n");
      for (j = 0; j < mlp->nitems; j++)
	if (mlp->item[j]->msgid_plural != NULL)
	  {
	    fprintf (stream, "    t.Add(");
	    write_csharp_string (stream, mlp->item[j]->msgid);
	    fprintf (stream, ",");
	    write_csharp_string (stream, mlp->item[j]->msgid_plural);
	    fprintf (stream, ");\n");
	  }
      fprintf (stream, "    return t;\n");
      fprintf (stream, "  }\n");
    }

  /* Emit the PluralEval function.  It is a subroutine for GetPluralString.  */
  if (plurals)
    {
      message_ty *header_entry;
      struct expression *plural;
      unsigned long int nplurals;

      header_entry = message_list_search (mlp, "");
      extract_plural_expression (header_entry ? header_entry->msgstr : NULL,
				 &plural, &nplurals);

      fprintf (stream, "  protected override long PluralEval (long n) {\n");
      fprintf (stream, "    return ");
      write_csharp_expression (stream, plural, false);
      fprintf (stream, ";\n");
      fprintf (stream, "  }\n");
    }

  /* Terminate the class.  */
  fprintf (stream, "}\n");

  if (last_dot != NULL)
    /* Terminate the namespace.  */
    fprintf (stream, "}\n");
}


/* Asynchronously cleaning up temporary files, when we receive any of the
   usually occurring signals whose default action is to terminate the
   program.  */

static struct
{
  const char *tmpdir;
  const char *file_name;
} cleanup_list;

/* The signal handler.  It gets called asynchronously.  */
static void
cleanup ()
{
  /* First cleanup the files in the subdirectory.  */
  {
    const char *filename = cleanup_list.file_name;

    if (filename != NULL)
      unlink (filename);
  }

  /* Then cleanup the main temporary directory.  */
  {
    const char *filename = cleanup_list.tmpdir;

    if (filename != NULL)
      rmdir (filename);
  }
}


int
msgdomain_write_csharp (message_list_ty *mlp, const char *canon_encoding,
			const char *resource_name, const char *locale_name,
			const char *directory)
{
  int retval;
  char *template;
  char *tmpdir;
  char *culture_name;
  char *output_file;
  char *class_name;
  char *csharp_file_name;
  FILE *csharp_file;
  const char *gettextlibdir;
  const char *csharp_sources[1];
  const char *libdirs[1];
  const char *libraries[1];

  /* If no entry for this resource/domain, don't even create the file.  */
  if (mlp->nitems == 0)
    return 0;

  retval = 1;

  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL);

  cleanup_list.tmpdir = NULL;
  cleanup_list.file_name = NULL;
  {
    static bool cleanup_already_registered = false;
    if (!cleanup_already_registered)
      {
	at_fatal_signal (&cleanup);
	cleanup_already_registered = true;
      }
  }

  /* Create a temporary directory where we can put the C# file.
     A simple temporary file would also be possible but would require us to
     define our own variant of mkstemp(): On one hand the functions mktemp(),
     tmpnam(), tempnam() present a security risk, and on the other hand the
     function mkstemp() doesn't allow to specify a fixed suffix of the file.
     It is simpler to create a temporary directory.  */
  template = (char *) xallocsa (PATH_MAX);
  if (path_search (template, PATH_MAX, NULL, "msg", 1))
    {
      error (0, errno,
	     _("cannot find a temporary directory, try setting $TMPDIR"));
      goto quit1;
    }
  block_fatal_signals ();
  tmpdir = mkdtemp (template);
  cleanup_list.tmpdir = tmpdir;
  unblock_fatal_signals ();
  if (tmpdir == NULL)
    {
      error (0, errno,
	     _("cannot create a temporary directory using template \"%s\""),
	     template);
      goto quit1;
    }

  /* Assign a default value to the resource name.  */
  if (resource_name == NULL)
    resource_name = "Messages";

  /* Convert the locale name to a .NET specific culture name.  */
  culture_name = xstrdup (locale_name);
  {
    char *p;
    for (p = culture_name; *p != '\0'; p++)
      if (*p == '_')
	*p = '-';
    if (strncmp (culture_name, "sr-CS", 5) == 0)
      memcpy (culture_name, "sr-SP", 5);
    p = strchr (culture_name, '@');
    if (p != NULL)
      {
	if (strcmp (p, "@latin") == 0)
	  strcpy (p, "-Latn");
	else if (strcmp (p, "@cyrillic") == 0)
	  strcpy (p, "-Cyrl");
      }
    if (strcmp (culture_name, "sr-SP") == 0)
      {
	free (culture_name);
	culture_name = xstrdup ("sr-SP-Latn");
      }
    else if (strcmp (culture_name, "uz-UZ") == 0)
      {
	free (culture_name);
	culture_name = xstrdup ("uz-UZ-Latn");
      }
  }
  

  /* Compute the output file name.  This code must be kept consistent with
     intl.cs, function GetSatelliteAssembly().  */
  {
    char *output_dir = concatenated_pathname (directory, culture_name, NULL);
    struct stat statbuf;

    /* Try to create the output directory if it does not yet exist.  */
    if (stat (output_dir, &statbuf) < 0 && errno == ENOENT)
      if (mkdir (output_dir, S_IRUSR | S_IWUSR | S_IXUSR
			     | S_IRGRP | S_IWGRP | S_IXGRP
			     | S_IROTH | S_IWOTH | S_IXOTH) < 0)
	{
	  error (0, errno, _("failed to create directory \"%s\""), output_dir);
	  free (output_dir);
	  goto quit3;
	}

    output_file =
      concatenated_pathname (output_dir, resource_name, ".resources.dll");

    free (output_dir);
  }

  /* Compute the class name.  This code must be kept consistent with intl.cs,
     function InstantiateResourceSet().  */
  {
    char *class_name_part1 = construct_class_name (resource_name);
    char *p;

    class_name =
      (char *) xmalloc (strlen (class_name_part1) + 1 + strlen (culture_name) + 1);
    sprintf (class_name, "%s_%s", class_name_part1, culture_name);
    for (p = class_name + strlen (class_name_part1) + 1; *p != '\0'; p++)
      if (*p == '-')
	*p = '_';
    free (class_name_part1);
  }

  /* Compute the temporary C# file name.  It must end in ".cs", so that
     the C# compiler recognizes that it is C# source code.  */
  csharp_file_name = concatenated_pathname (tmpdir, "resset.cs", NULL);

  /* Create the C# file.  */
  cleanup_list.file_name = csharp_file_name;
  csharp_file = fopen (csharp_file_name, "w");
  if (csharp_file == NULL)
    {
      error (0, errno, _("failed to create \"%s\""), csharp_file_name);
      goto quit4;
    }

  write_csharp_code (csharp_file, class_name, mlp);

  if (fwriteerror (csharp_file))
    {
      error (0, errno, _("error while writing \"%s\" file"), csharp_file_name);
      fclose (csharp_file);
      goto quit5;
    }

  /* Make it possible to override the .dll location.  This is
     necessary for running the testsuite before "make install".  */
  gettextlibdir = getenv ("GETTEXTCSHARPLIBDIR");
  if (gettextlibdir == NULL || gettextlibdir[0] == '\0')
    gettextlibdir = relocate (LIBDIR);

  /* Compile the C# file to a .dll file.  */
  csharp_sources[0] = csharp_file_name;
  libdirs[0] = gettextlibdir;
  libraries[0] = "GNU.Gettext";
  if (compile_csharp_class (csharp_sources, 1, libdirs, 1, libraries, 1,
			    output_file, true, false, verbose))
    {
      error (0, 0, _("compilation of C# class failed, please try --verbose"));
      goto quit5;
    }

  retval = 0;

 quit5:
  unlink (csharp_file_name);
 quit4:
  cleanup_list.file_name = NULL;
  free (csharp_file_name);
  free (class_name);
  free (output_file);
 quit3:
  free (culture_name);
  rmdir (tmpdir);
 quit1:
  cleanup_list.tmpdir = NULL;
  freesa (template);
  /* Here we could unregister the cleanup() handler.  */
  return retval;
}
