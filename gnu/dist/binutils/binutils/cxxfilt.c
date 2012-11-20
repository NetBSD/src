/* Demangler for GNU C++ - main program
   Copyright 1989, 1991, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002, 2003 Free Software Foundation, Inc.
   Written by James Clark (jjc@jclark.uucp)
   Rewritten by Fred Fish (fnf@cygnus.com) for ARM and Lucid demangling
   Modified by Satish Pai (pai@apollo.hp.com) for HP demangling

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "bfd.h"
#include "bucomm.h"
#include "libiberty.h"
#include "demangle.h"
#include "getopt.h"
#include "safe-ctype.h"

static int flags = DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE;

static void demangle_it (char *);
static void usage (FILE *, int) ATTRIBUTE_NORETURN;
static void print_demangler_list (FILE *);

static void
demangle_it (char *mangled_name)
{
  char *result;

  /* For command line args, also try to demangle type encodings.  */
  result = cplus_demangle (mangled_name, flags | DMGL_TYPES);
  if (result == NULL)
    {
      printf ("%s\n", mangled_name);
    }
  else
    {
      printf ("%s\n", result);
      free (result);
    }
}

static void
print_demangler_list (FILE *stream)
{
  const struct demangler_engine *demangler;

  fprintf (stream, "{%s", libiberty_demanglers->demangling_style_name);

  for (demangler = libiberty_demanglers + 1;
       demangler->demangling_style != unknown_demangling;
       ++demangler)
    fprintf (stream, ",%s", demangler->demangling_style_name);

  fprintf (stream, "}");
}

static void
usage (FILE *stream, int status)
{
  fprintf (stream, "\
Usage: %s [-_] [-n] [--strip-underscores] [--no-strip-underscores]\n\
       [-p] [--no-params]\n",
	   program_name);

  fprintf (stream, "\
       [-s ");
  print_demangler_list (stream);
  fprintf (stream, "]\n");

  fprintf (stream, "\
       [--format ");
  print_demangler_list (stream);
  fprintf (stream, "]\n");

  fprintf (stream, "\
       [--help] [--version] [arg...]\n");
  exit (status);
}

#define MBUF_SIZE 32767
char mbuffer[MBUF_SIZE];

int strip_underscore = 0;

static const struct option long_options[] = {
  {"strip-underscores", no_argument, 0, '_'},
  {"format", required_argument, 0, 's'},
  {"help", no_argument, 0, 'h'},
  {"no-params", no_argument, 0, 'p'},
  {"no-strip-underscores", no_argument, 0, 'n'},
  {"version", no_argument, 0, 'v'},
  {0, no_argument, 0, 0}
};

static const char *standard_symbol_characters (void);

static const char *hp_symbol_characters (void);

/* Return the string of non-alnum characters that may occur
   as a valid symbol component, in the standard assembler symbol
   syntax.  */

static const char *
standard_symbol_characters (void)
{
  return "_$.";
}


/* Return the string of non-alnum characters that may occur
   as a valid symbol name component in an HP object file.

   Note that, since HP's compiler generates object code straight from
   C++ source, without going through an assembler, its mangled
   identifiers can use all sorts of characters that no assembler would
   tolerate, so the alphabet this function creates is a little odd.
   Here are some sample mangled identifiers offered by HP:

	typeid*__XT24AddressIndExpClassMember_
	[Vftptr]key:__dt__32OrdinaryCompareIndExpClassMemberFv
	__ct__Q2_9Elf64_Dyn18{unnamed.union.#1}Fv

   This still seems really weird to me, since nowhere else in this
   file is there anything to recognize curly brackets, parens, etc.
   I've talked with Srikanth <srikanth@cup.hp.com>, and he assures me
   this is right, but I still strongly suspect that there's a
   misunderstanding here.

   If we decide it's better for c++filt to use HP's assembler syntax
   to scrape identifiers out of its input, here's the definition of
   the symbol name syntax from the HP assembler manual:

       Symbols are composed of uppercase and lowercase letters, decimal
       digits, dollar symbol, period (.), ampersand (&), pound sign(#) and
       underscore (_). A symbol can begin with a letter, digit underscore or
       dollar sign. If a symbol begins with a digit, it must contain a
       non-digit character.

   So have fun.  */
static const char *
hp_symbol_characters (void)
{
  return "_$.<>#,*&[]:(){}";
}

extern int main (int, char **);

int
main (int argc, char **argv)
{
  char *result;
  int c;
  const char *valid_symbols;
  enum demangling_styles style = auto_demangling;

  program_name = argv[0];
  xmalloc_set_program_name (program_name);

  strip_underscore = TARGET_PREPENDS_UNDERSCORE;

  while ((c = getopt_long (argc, argv, "_nps:", long_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case '?':
	  usage (stderr, 1);
	  break;
	case 'h':
	  usage (stdout, 0);
	case 'n':
	  strip_underscore = 0;
	  break;
	case 'p':
	  flags &= ~ DMGL_PARAMS;
	  break;
	case 'v':
	  print_version ("c++filt");
	  return (0);
	case '_':
	  strip_underscore = 1;
	  break;
	case 's':
	  {
	    style = cplus_demangle_name_to_style (optarg);
	    if (style == unknown_demangling)
	      {
		fprintf (stderr, "%s: unknown demangling style `%s'\n",
			 program_name, optarg);
		return (1);
	      }
	    else
	      cplus_demangle_set_style (style);
	  }
	  break;
	}
    }

  if (optind < argc)
    {
      for ( ; optind < argc; optind++)
	{
	  demangle_it (argv[optind]);
	}
    }
  else
    {
      switch (current_demangling_style)
	{
	case gnu_demangling:
	case lucid_demangling:
	case arm_demangling:
	case java_demangling:
	case edg_demangling:
	case gnat_demangling:
	case gnu_v3_demangling:
	case auto_demangling:
	  valid_symbols = standard_symbol_characters ();
	  break;
	case hp_demangling:
	  valid_symbols = hp_symbol_characters ();
	  break;
	default:
	  /* Folks should explicitly indicate the appropriate alphabet for
	     each demangling.  Providing a default would allow the
	     question to go unconsidered.  */
	  fatal ("Internal error: no symbol alphabet for current style");
	}

      for (;;)
	{
	  int i = 0;
	  c = getchar ();
	  /* Try to read a label.  */
	  while (c != EOF && (ISALNUM (c) || strchr (valid_symbols, c)))
	    {
	      if (i >= MBUF_SIZE-1)
		break;
	      mbuffer[i++] = c;
	      c = getchar ();
	    }
	  if (i > 0)
	    {
	      int skip_first = 0;

	      mbuffer[i] = 0;
	      if (mbuffer[0] == '.' || mbuffer[0] == '$')
		++skip_first;
	      if (strip_underscore && mbuffer[skip_first] == '_')
		++skip_first;

	      if (skip_first > i)
		skip_first = i;

	      flags |= (int) style;
	      result = cplus_demangle (mbuffer + skip_first, flags);
	      if (result)
		{
		  if (mbuffer[0] == '.')
		    putc ('.', stdout);
		  fputs (result, stdout);
		  free (result);
		}
	      else
		fputs (mbuffer, stdout);

	      fflush (stdout);
	    }
	  if (c == EOF)
	    break;
	  putchar (c);
	  fflush (stdout);
	}
    }

  return (0);
}
