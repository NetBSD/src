/* Qt format strings.
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
#include "xalloc.h"
#include "xerror.h"
#include "gettext.h"

#define _(str) gettext (str)

/* Qt format strings are processed by QString::arg and are documented in
   qt-3.0.5/doc/html/qstring.html.
   A directive starts with '%' and is followed by a digit ('0' to '9').
   Each %n must occur only once in the given string.
   The first .arg() invocation replaces the %n with the lowest numbered n,
   the next .arg() invocation then replaces the %n with the second-lowest
   numbered n, and so on.
   (This is inherently buggy because a '%' in the first replacement confuses
   the second .arg() invocation.)
   Although %0 is supported, usually %1 denotes the first argument, %2 the
   second argument etc.  */

struct spec
{
  unsigned int directives;
  unsigned int arg_count;
  bool args_used[10];
};


static void *
format_parse (const char *format, bool translated, char **invalid_reason)
{
  struct spec spec;
  struct spec *result;

  spec.directives = 0;
  spec.arg_count = 0;

  for (; *format != '\0';)
    if (*format++ == '%')
      if (*format >= '0' && *format <= '9')
	{
	  /* A directive.  */
	  unsigned int number;

	  spec.directives++;

	  number = *format - '0';

	  while (spec.arg_count <= number)
	    spec.args_used[spec.arg_count++] = false;
	  if (spec.args_used[number])
	    {
	      *invalid_reason =
		xasprintf (_("Multiple references to %%%c."), *format);
	      goto bad_format;
	    }
	  spec.args_used[number] = true;

	  format++;
	}

  result = (struct spec *) xmalloc (sizeof (struct spec));
  *result = spec;
  return result;

 bad_format:
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

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

  for (i = 0; i < spec1->arg_count || i < spec2->arg_count; i++)
    {
      bool arg_used1 = (i < spec1->arg_count && spec1->args_used[i]);
      bool arg_used2 = (i < spec2->arg_count && spec2->args_used[i]);

      /* The translator cannot omit a %n from the msgstr because that would
	 yield a "Argument missing" warning at runtime.  */
      if (arg_used1 != arg_used2)
	{
	  if (error_logger)
	    error_logger (arg_used1
			  ? _("a format specification for argument %u doesn't exist in '%s'")
			  : _("a format specification for argument %u, as in '%s', doesn't exist in 'msgid'"),
			  i, pretty_msgstr);
	  err = true;
	  break;
	}
    }

  return err;
}


struct formatstring_parser formatstring_qt =
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
  for (i = 0; i < spec->arg_count; i++)
    {
      if (i > 0)
	printf (" ");
      if (spec->args_used[i])
	printf ("*");
      else
	printf ("_");
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
 * compile-command: "/bin/sh ../libtool --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../lib -I../intl -DHAVE_CONFIG_H -DTEST format-qt.c ../lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
