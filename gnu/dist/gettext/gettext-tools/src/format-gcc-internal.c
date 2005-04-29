/* GCC internal format strings.
   Copyright (C) 2003-2004 Free Software Foundation, Inc.
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

#include <stdbool.h>
#include <stdlib.h>

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xerror.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* GCC internal format strings consist of language frontend independent
   format directives, implemented in gcc-3.3/gcc/diagnostic.c (function
   output_format), plus some frontend dependent extensions:
     - for the C/ObjC frontend in gcc-3.3/gcc/c-objc-common.c
     - for the C++ frontend in gcc-3.3/gcc/cp/error.c
   Taking these together, GCC internal format strings are specified as follows.
   A directive
   - starts with '%',
   - is optionally followed by a size specifier 'l',
   - is optionally followed by '+' (only the specifiers of gcc/cp/error.c),
   - is optionally followed by '#' (only the specifiers of gcc/cp/error.c),
   - is finished by a specifier

       - '%', that needs no argument,
       - 'c', that needs a character argument,
       - 's', that needs a string argument,
       - 'i', 'd', that need a signed integer argument,
       - 'o', 'u', 'x', that need an unsigned integer argument,
       - '.*s', that needs a signed integer argument and a string argument,
       - 'H', that needs a 'location_t *' argument,
         [see gcc/diagnostic.c]

       - 'D', that needs a general declaration argument,
       - 'F', that needs a function declaration argument,
       - 'T', that needs a type argument,
         [see gcc/c-objc-common.c and gcc/cp/error.c]

       - 'A', that needs a function argument list argument,
       - 'C', that needs a tree code argument,
       - 'E', that needs an expression argument,
       - 'L', that needs a language argument,
       - 'O', that needs a binary operator argument,
       - 'P', that needs a function parameter argument,
       - 'Q', that needs an assignment operator argument,
       - 'V', that needs a const/volatile qualifier argument.
         [see gcc/cp/error.c]
 */

enum format_arg_type
{
  FAT_NONE		= 0,
  /* Basic types */
  FAT_INTEGER		= 1,
  FAT_CHAR		= 2,
  FAT_STRING		= 3,
  FAT_LOCATION		= 4,
  FAT_TREE		= 5,
  FAT_TREE_CODE		= 6,
  FAT_LANGUAGES		= 7,
  /* Flags */
  FAT_UNSIGNED		= 1 << 3,
  FAT_SIZE_LONG		= 1 << 4,
  FAT_TREE_DECL		= 1 << 5,
  FAT_TREE_FUNCDECL	= 2 << 5,
  FAT_TREE_TYPE		= 3 << 5,
  FAT_TREE_ARGUMENT	= 4 << 5,
  FAT_TREE_EXPRESSION	= 5 << 5,
  FAT_TREE_CV		= 6 << 5,
  FAT_TREE_CODE_BINOP	= 1 << 8,
  FAT_TREE_CODE_ASSOP	= 2 << 8,
  FAT_FUNCPARAM		= 1 << 10
};

struct unnumbered_arg
{
  enum format_arg_type type;
};

struct spec
{
  unsigned int directives;
  unsigned int unnumbered_arg_count;
  unsigned int allocated;
  struct unnumbered_arg *unnumbered;
};


static void *
format_parse (const char *format, bool translated, char **invalid_reason)
{
  struct spec spec;
  struct spec *result;

  spec.directives = 0;
  spec.unnumbered_arg_count = 0;
  spec.allocated = 0;
  spec.unnumbered = NULL;

  for (; *format != '\0';)
    if (*format++ == '%')
      {
	/* A directive.  */
	enum format_arg_type size;

	spec.directives++;

	/* Parse size.  */
	size = 0;
	if (*format == 'l')
	  {
	    format++;
	    size = FAT_SIZE_LONG;
	  }

	if (*format != '%')
	  {
	    enum format_arg_type type;

	    if (*format == 'c')
	      type = FAT_CHAR;
	    else if (*format == 's')
	      type = FAT_STRING;
	    else if (*format == 'i' || *format == 'd')
	      type = FAT_INTEGER | size;
	    else if (*format == 'o' || *format == 'u' || *format == 'x')
	      type = FAT_INTEGER | FAT_UNSIGNED | size;
	    else if (*format == '.' && format[1] == '*' && format[2] == 's')
	      {
		if (spec.allocated == spec.unnumbered_arg_count)
		  {
		    spec.allocated = 2 * spec.allocated + 1;
		    spec.unnumbered = (struct unnumbered_arg *) xrealloc (spec.unnumbered, spec.allocated * sizeof (struct unnumbered_arg));
		  }
		spec.unnumbered[spec.unnumbered_arg_count].type = FAT_INTEGER;
		spec.unnumbered_arg_count++;
		type = FAT_STRING;
	      }
	    else if (*format == 'H')
	      type = FAT_LOCATION;
	    else
	      {
		if (*format == '+')
		  format++;
		if (*format == '#')
		  format++;
		if (*format == 'D')
		  type = FAT_TREE | FAT_TREE_DECL;
		else if (*format == 'F')
		  type = FAT_TREE | FAT_TREE_FUNCDECL;
		else if (*format == 'T')
		  type = FAT_TREE | FAT_TREE_TYPE;
		else if (*format == 'A')
		  type = FAT_TREE | FAT_TREE_ARGUMENT;
		else if (*format == 'C')
		  type = FAT_TREE_CODE;
		else if (*format == 'E')
		  type = FAT_TREE | FAT_TREE_EXPRESSION;
		else if (*format == 'L')
		  type = FAT_LANGUAGES;
		else if (*format == 'O')
		  type = FAT_TREE_CODE | FAT_TREE_CODE_BINOP;
		else if (*format == 'P')
		  type = FAT_INTEGER | FAT_FUNCPARAM;
		else if (*format == 'Q')
		  type = FAT_TREE_CODE | FAT_TREE_CODE_ASSOP;
		else if (*format == 'V')
		  type = FAT_TREE | FAT_TREE_CV;
		else
		  {
		    *invalid_reason =
		      (*format == '\0'
		       ? INVALID_UNTERMINATED_DIRECTIVE ()
		       : (*format == 'c'
			  || *format == 's'
			  || *format == 'i' || *format == 'd'
			  || *format == 'o' || *format == 'u' || *format == 'x'
			  || *format == 'H'
			  ? xasprintf (_("In the directive number %u, flags are not allowed before '%c'."), spec.directives, *format)
			  : INVALID_CONVERSION_SPECIFIER (spec.directives,
							  *format)));
		    goto bad_format;
		  }
	      }

	    if (spec.allocated == spec.unnumbered_arg_count)
	      {
		spec.allocated = 2 * spec.allocated + 1;
		spec.unnumbered = (struct unnumbered_arg *) xrealloc (spec.unnumbered, spec.allocated * sizeof (struct unnumbered_arg));
	      }
	    spec.unnumbered[spec.unnumbered_arg_count].type = type;
	    spec.unnumbered_arg_count++;
	  }

	format++;
      }

  result = (struct spec *) xmalloc (sizeof (struct spec));
  *result = spec;
  return result;

 bad_format:
  if (spec.unnumbered != NULL)
    free (spec.unnumbered);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->unnumbered != NULL)
    free (spec->unnumbered);
  free (spec);
}

static int
format_get_number_of_directives (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->directives;
}

static bool
format_check (void *msgid_descr, void *msgstr_descr, bool equality,
	      formatstring_error_logger_t error_logger,
	      const char *pretty_msgstr)
{
  struct spec *spec1 = (struct spec *) msgid_descr;
  struct spec *spec2 = (struct spec *) msgstr_descr;
  bool err = false;
  unsigned int i;

  /* Check the argument types are the same.  */
  if (equality
      ? spec1->unnumbered_arg_count != spec2->unnumbered_arg_count
      : spec1->unnumbered_arg_count < spec2->unnumbered_arg_count)
    {
      if (error_logger)
	error_logger (_("number of format specifications in 'msgid' and '%s' does not match"),
		      pretty_msgstr);
      err = true;
    }
  else
    for (i = 0; i < spec2->unnumbered_arg_count; i++)
      if (spec1->unnumbered[i].type != spec2->unnumbered[i].type)
	{
	  if (error_logger)
	    error_logger (_("format specifications in 'msgid' and '%s' for argument %u are not the same"),
			  pretty_msgstr, i + 1);
	  err = true;
	}

  return err;
}


struct formatstring_parser formatstring_gcc_internal =
{
  format_parse,
  format_free,
  format_get_number_of_directives,
  format_check
};


#ifdef TEST

/* Test program: Print the argument list specification returned by
   format_parse for strings read from standard input.  */

#include <stdio.h>
#include "getline.h"

static void
format_print (void *descr)
{
  struct spec *spec = (struct spec *) descr;
  unsigned int i;

  if (spec == NULL)
    {
      printf ("INVALID");
      return;
    }

  printf ("(");
  for (i = 0; i < spec->unnumbered_arg_count; i++)
    {
      if (i > 0)
	printf (" ");
      if (spec->unnumbered[i].type & FAT_UNSIGNED)
	printf ("[unsigned]");
      if (spec->unnumbered[i].type & FAT_SIZE_LONG)
	printf ("[long]");
      switch (spec->unnumbered[i].type & ~(FAT_UNSIGNED | FAT_SIZE_LONG))
	{
	case FAT_INTEGER:
	  printf ("i");
	  break;
	case FAT_INTEGER | FAT_FUNCPARAM:
	  printf ("P");
	  break;
	case FAT_CHAR:
	  printf ("c");
	  break;
	case FAT_STRING:
	  printf ("s");
	  break;
	case FAT_LOCATION:
	  printf ("H");
	  break;
	case FAT_TREE | FAT_TREE_DECL:
	  printf ("D");
	  break;
	case FAT_TREE | FAT_TREE_FUNCDECL:
	  printf ("F");
	  break;
	case FAT_TREE | FAT_TREE_TYPE:
	  printf ("T");
	  break;
	case FAT_TREE | FAT_TREE_ARGUMENT:
	  printf ("A");
	  break;
	case FAT_TREE | FAT_TREE_EXPRESSION:
	  printf ("E");
	  break;
	case FAT_TREE | FAT_TREE_CV:
	  printf ("V");
	  break;
	case FAT_TREE_CODE:
	  printf ("C");
	  break;
	case FAT_TREE_CODE | FAT_TREE_CODE_BINOP:
	  printf ("O");
	  break;
	case FAT_TREE_CODE | FAT_TREE_CODE_ASSOP:
	  printf ("Q");
	  break;
	case FAT_LANGUAGES:
	  printf ("L");
	  break;
	default:
	  abort ();
	}
    }
  printf (")");
}

int
main ()
{
  for (;;)
    {
      char *line = NULL;
      size_t line_size = 0;
      int line_len;
      char *invalid_reason;
      void *descr;

      line_len = getline (&line, &line_size, stdin);
      if (line_len < 0)
	break;
      if (line_len > 0 && line[line_len - 1] == '\n')
	line[--line_len] = '\0';

      invalid_reason = NULL;
      descr = format_parse (line, false, &invalid_reason);

      format_print (descr);
      printf ("\n");
      if (descr == NULL)
	printf ("%s\n", invalid_reason);

      free (invalid_reason);
      free (line);
    }

  return 0;
}

/*
 * For Emacs M-x compile
 * Local Variables:
 * compile-command: "/bin/sh ../libtool --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../lib -I../intl -DHAVE_CONFIG_H -DTEST format-gcc-internal.c ../lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
