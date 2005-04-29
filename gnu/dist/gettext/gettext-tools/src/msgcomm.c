/* GNU gettext - internationalization aids
   Copyright (C) 1997-1998, 2000-2005 Free Software Foundation, Inc.

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

#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "closeout.h"
#include "dir-list.h"
#include "str-list.h"
#include "file-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "message.h"
#include "read-po.h"
#include "write-po.h"
#include "msgl-cat.h"
#include "exit.h"
#include "gettext.h"


/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


/* Force output of PO file even if empty.  */
static int force_po;

/* Target encoding.  */
static const char *to_code;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-location", no_argument, &line_comment, 1 },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "files-from", required_argument, NULL, 'f' },
  { "force-po", no_argument, &force_po, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, &line_comment, 0 },
  { "no-wrap", no_argument, NULL, CHAR_MAX + 2 },
  { "omit-header", no_argument, NULL, CHAR_MAX + 1 },
  { "output", required_argument, NULL, 'o' }, /* for backward compatibility */
  { "output-file", required_argument, NULL, 'o' },
  { "properties-input", no_argument, NULL, 'P' },
  { "properties-output", no_argument, NULL, 'p' },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "strict", no_argument, NULL, 'S' },
  { "stringtable-input", no_argument, NULL, CHAR_MAX + 3 },
  { "stringtable-output", no_argument, NULL, CHAR_MAX + 4 },
  { "to-code", required_argument, NULL, 't' },
  { "unique", no_argument, NULL, 'u' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { "more-than", required_argument, NULL, '>', },
  { "less-than", required_argument, NULL, '<', },
  { NULL, 0, NULL, 0 }
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ > 4) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;


int
main (int argc, char *argv[])
{
  int cnt;
  int optchar;
  bool do_help = false;
  bool do_version = false;
  msgdomain_list_ty *result;
  bool sort_by_msgid = false;
  bool sort_by_filepos = false;
  const char *files_from = NULL;
  string_list_ty *file_list;
  char *output_file = NULL;

  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Set default values for variables.  */
  more_than = -1;
  less_than = -1;
  use_first = false;

  while ((optchar = getopt_long (argc, argv, "<:>:D:eEf:Fhino:pPst:uVw:",
				 long_options, NULL)) != EOF)
    switch (optchar)
      {
      case '\0':		/* Long option.  */
	break;

      case '>':
	{
	  int value;
	  char *endp;
	  value = strtol (optarg, &endp, 10);
	  if (endp != optarg)
	    more_than = value;
	}
	break;

      case '<':
	{
	  int value;
	  char *endp;
	  value = strtol (optarg, &endp, 10);
	  if (endp != optarg)
	    less_than = value;
	}
	break;

      case 'D':
	dir_list_append (optarg);
	break;

      case 'e':
	message_print_style_escape (false);
	break;

      case 'E':
	message_print_style_escape (true);
	break;

      case 'f':
	files_from = optarg;
	break;

      case 'F':
	sort_by_filepos = true;
	break;

      case 'h':
	do_help = true;
	break;

      case 'i':
	message_print_style_indent ();
	break;

      case 'n':
	line_comment = 1;
	break;

      case 'o':
	output_file = optarg;
	break;

      case 'p':
	message_print_syntax_properties ();
	break;

      case 'P':
	input_syntax = syntax_properties;
	break;

      case 's':
	sort_by_msgid = true;
	break;

      case 'S':
	message_print_style_uniforum ();
	break;

      case 't':
	to_code = optarg;
	break;

      case 'u':
	less_than = 2;
	break;

      case 'V':
	do_version = true;
	break;

      case 'w':
	{
	  int value;
	  char *endp;
	  value = strtol (optarg, &endp, 10);
	  if (endp != optarg)
	    message_page_width_set (value);
	}
	break;

      case CHAR_MAX + 1:
	omit_header = true;
	break;

      case CHAR_MAX + 2: /* --no-wrap */
	message_page_width_ignore ();
	break;

      case CHAR_MAX + 3: /* --stringtable-input */
	input_syntax = syntax_stringtable;
	break;

      case CHAR_MAX + 4: /* --stringtable-output */
	message_print_syntax_stringtable ();
	break;

      default:
	usage (EXIT_FAILURE);
	/* NOTREACHED */
      }

  /* Version information requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", basename (program_name), PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"),
	      "1995-1998, 2000-2005");
      printf (_("Written by %s.\n"), "Peter Miller");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Verify selected options.  */
  if (!line_comment && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--no-location", "--sort-by-file");

  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--sort-output", "--sort-by-file");

  /* Determine list of files we have to process.  */
  if (files_from != NULL)
    file_list = read_names_from_file (files_from);
  else
    file_list = string_list_alloc ();
  /* Append names from command line.  */
  for (cnt = optind; cnt < argc; ++cnt)
    string_list_append_unique (file_list, argv[cnt]);

  /* Test whether sufficient input files were given.  */
  if (file_list->nitems < 2)
    {
      error (EXIT_SUCCESS, 0, _("at least two files must be specified"));
      usage (EXIT_FAILURE);
    }

  /* Default the message selection criteria, and check them for sanity.  */
  if (more_than < 0)
    more_than = (less_than < 0 ? 1 : 0);
  if (less_than < 0)
    less_than = INT_MAX;
  if (more_than >= less_than || less_than < 2)
    error (EXIT_FAILURE, 0,
	   _("impossible selection criteria specified (%d < n < %d)"),
	   more_than, less_than);

  /* Read input files, then filter, convert and merge messages.  */
  allow_duplicates = true;
  msgcomm_mode = true;
  result = catenate_msgdomain_list (file_list, to_code);

  string_list_free (file_list);

  /* Sorting the list of messages.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  /* Write the PO file.  */
  msgdomain_list_print (result, output_file, force_po, false);

  exit (EXIT_SUCCESS);
}


/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] [INPUTFILE]...\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Find messages which are common to two or more of the specified PO files.\n\
By using the --more-than option, greater commonality may be requested\n\
before messages are printed.  Conversely, the --less-than option may be\n\
used to specify less commonality before messages are printed (i.e.\n\
--less-than=2 will only print the unique messages).  Translations,\n\
comments and extract comments will be preserved, but only from the first\n\
PO file to define them.  File positions from all PO files will be\n\
cumulated.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  INPUTFILE ...               input files\n"));
      printf (_("\
  -f, --files-from=FILE       get list of input files from FILE\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If input file is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Output file location:\n"));
      printf (_("\
  -o, --output-file=FILE      write output to specified file\n"));
      printf (_("\
The results are written to standard output if no output file is specified\n\
or if it is -.\n"));
      printf ("\n");
      printf (_("\
Message selection:\n"));
      printf (_("\
  -<, --less-than=NUMBER      print messages with less than this many\n\
                              definitions, defaults to infinite if not set\n"));
      printf (_("\
  ->, --more-than=NUMBER      print messages with more than this many\n\
                              definitions, defaults to 1 if not set\n"));
      printf (_("\
  -u, --unique                shorthand for --less-than=2, requests\n\
                              that only unique messages be printed\n"));
      printf ("\n");
      printf (_("\
Input file syntax:\n"));
      printf (_("\
  -P, --properties-input      input files are in Java .properties syntax\n"));
      printf (_("\
      --stringtable-input     input files are in NeXTstep/GNUstep .strings\n\
                              syntax\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
  -e, --no-escape             do not use C escapes in output (default)\n"));
      printf (_("\
  -E, --escape                use C escapes in output, no extended chars\n"));
      printf (_("\
      --force-po              write PO file even if empty\n"));
      printf (_("\
  -i, --indent                write the .po file using indented style\n"));
      printf (_("\
      --no-location           do not write '#: filename:line' lines\n"));
      printf (_("\
  -n, --add-location          generate '#: filename:line' lines (default)\n"));
      printf (_("\
      --strict                write out strict Uniforum conforming .po file\n"));
      printf (_("\
  -p, --properties-output     write out a Java .properties file\n"));
      printf (_("\
      --stringtable-output    write out a NeXTstep/GNUstep .strings file\n"));
      printf (_("\
  -w, --width=NUMBER          set output page width\n"));
      printf (_("\
      --no-wrap               do not break long message lines, longer than\n\
                              the output page width, into several lines\n"));
      printf (_("\
  -s, --sort-output           generate sorted output\n"));
      printf (_("\
  -F, --sort-by-file          sort output by file location\n"));
      printf (_("\
      --omit-header           don't write header with `msgid \"\"' entry\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf ("\n");
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"),
	     stdout);
    }

  exit (status);
}
