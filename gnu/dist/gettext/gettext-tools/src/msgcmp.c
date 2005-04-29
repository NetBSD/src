/* GNU gettext - internationalization aids
   Copyright (C) 1995-1998, 2000-2005 Free Software Foundation, Inc.
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "closeout.h"
#include "dir-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "message.h"
#include "exit.h"
#include "read-po.h"
#include "msgl-iconv.h"
#include "strstr.h"
#include "c-strcase.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Apply the .pot file to each of the domains in the PO file.  */
static bool multi_domain_mode = false;

/* Long options.  */
static const struct option long_options[] =
{
  { "directory", required_argument, NULL, 'D' },
  { "help", no_argument, NULL, 'h' },
  { "multi-domain", no_argument, NULL, 'm' },
  { "properties-input", no_argument, NULL, 'P' },
  { "stringtable-input", no_argument, NULL, CHAR_MAX + 1 },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;
static void compare (const char *fn1, const char *fn2);


int
main (int argc, char *argv[])
{
  int optchar;
  bool do_help;
  bool do_version;

  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  gram_max_allowed_errors = UINT_MAX;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  do_help = false;
  do_version = false;
  while ((optchar = getopt_long (argc, argv, "D:hmPV", long_options, NULL))
	 != EOF)
    switch (optchar)
      {
      case '\0':		/* long option */
	break;

      case 'D':
	dir_list_append (optarg);
	break;

      case 'h':
	do_help = true;
	break;

      case 'm':
	multi_domain_mode = true;
	break;

      case 'P':
	input_syntax = syntax_properties;
	break;

      case 'V':
	do_version = true;
	break;

      case CHAR_MAX + 1: /* --stringtable-input */
	input_syntax = syntax_stringtable;
	break;

      default:
	usage (EXIT_FAILURE);
	break;
      }

  /* Version information is requested.  */
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

  /* Test whether we have an .po file name as argument.  */
  if (optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input files given"));
      usage (EXIT_FAILURE);
    }
  if (optind + 2 != argc)
    {
      error (EXIT_SUCCESS, 0, _("exactly 2 input files required"));
      usage (EXIT_FAILURE);
    }

  /* compare the two files */
  compare (argv[optind], argv[optind + 1]);
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
Usage: %s [OPTION] def.po ref.pot\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Compare two Uniforum style .po files to check that both contain the same\n\
set of msgid strings.  The def.po file is an existing PO file with the\n\
translations.  The ref.pot file is the last created PO file, or a PO Template\n\
file (generally created by xgettext).  This is useful for checking that\n\
you have translated each and every message in your program.  Where an exact\n\
match cannot be found, fuzzy matching is used to produce better diagnostics.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  def.po                      translations\n"));
      printf (_("\
  ref.pot                     references to the sources\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf ("\n");
      printf (_("\
Operation modifiers:\n"));
      printf (_("\
  -m, --multi-domain          apply ref.pot to each of the domains in def.po\n"));
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
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf ("\n");
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"), stdout);
    }

  exit (status);
}


/* Return true if a message should be kept.  */
static bool
is_message_selected (const message_ty *mp)
{
  /* Always keep the header entry.  */
  if (mp->msgid[0] == '\0')
    return true;

  return !mp->obsolete;
}


/* Remove obsolete messages from a message list.  Return the modified list.  */
static msgdomain_list_ty *
remove_obsoletes (msgdomain_list_ty *mdlp)
{
  size_t k;

  for (k = 0; k < mdlp->nitems; k++)
    message_list_remove_if_not (mdlp->item[k]->messages, is_message_selected);

  return mdlp;
}


static void
match_domain (const char *fn1, const char *fn2,
	      message_list_ty *defmlp, message_list_ty *refmlp,
	      int *nerrors)
{
  size_t j;

  for (j = 0; j < refmlp->nitems; j++)
    {
      message_ty *refmsg;
      message_ty *defmsg;

      refmsg = refmlp->item[j];

      /* See if it is in the other file.  */
      defmsg = message_list_search (defmlp, refmsg->msgid);
      if (defmsg)
	defmsg->used = 1;
      else
	{
	  /* If the message was not defined at all, try to find a very
	     similar message, it could be a typo, or the suggestion may
	     help.  */
	  (*nerrors)++;
	  defmsg = message_list_search_fuzzy (defmlp, refmsg->msgid);
	  if (defmsg)
	    {
	      po_gram_error_at_line (&refmsg->pos, _("\
this message is used but not defined..."));
	      po_gram_error_at_line (&defmsg->pos, _("\
...but this definition is similar"));
	      defmsg->used = 1;
	    }
	  else
	    po_gram_error_at_line (&refmsg->pos, _("\
this message is used but not defined in %s"), fn1);
	}
    }
}


static void
compare (const char *fn1, const char *fn2)
{
  msgdomain_list_ty *def;
  msgdomain_list_ty *ref;
  int nerrors;
  size_t j, k;
  message_list_ty *empty_list;

  /* This is the master file, created by a human.  */
  def = remove_obsoletes (read_po_file (fn1));

  /* This is the generated file, created by groping the sources with
     the xgettext program.  */
  ref = remove_obsoletes (read_po_file (fn2));

  /* The references file can be either in ASCII or in UTF-8.  If it is
     in UTF-8, we have to convert the definitions to UTF-8 as well.  */
  {
    bool was_utf8 = false;
    for (k = 0; k < ref->nitems; k++)
      {
	message_list_ty *mlp = ref->item[k]->messages;

	for (j = 0; j < mlp->nitems; j++)
	  if (mlp->item[j]->msgid[0] == '\0' /* && !mlp->item[j]->obsolete */)
	    {
	      const char *header = mlp->item[j]->msgstr;

	      if (header != NULL)
		{
		  const char *charsetstr = strstr (header, "charset=");

		  if (charsetstr != NULL)
		    {
		      size_t len;

		      charsetstr += strlen ("charset=");
		      len = strcspn (charsetstr, " \t\n");
		      if (len == strlen ("UTF-8")
			  && c_strncasecmp (charsetstr, "UTF-8", len) == 0)
			was_utf8 = true;
		    }
		}
	    }
	}
    if (was_utf8)
      def = iconv_msgdomain_list (def, "UTF-8", fn1);
  }

  empty_list = message_list_alloc (false);

  /* Every entry in the xgettext generated file must be matched by a
     (single) entry in the human created file.  */
  nerrors = 0;
  if (!multi_domain_mode)
    for (k = 0; k < ref->nitems; k++)
      {
	const char *domain = ref->item[k]->domain;
	message_list_ty *refmlp = ref->item[k]->messages;
	message_list_ty *defmlp;

	defmlp = msgdomain_list_sublist (def, domain, false);
	if (defmlp == NULL)
	  defmlp = empty_list;

	match_domain (fn1, fn2, defmlp, refmlp, &nerrors);
      }
  else
    {
      /* Apply the references messages in the default domain to each of
	 the definition domains.  */
      message_list_ty *refmlp = ref->item[0]->messages;

      for (k = 0; k < def->nitems; k++)
	{
	  message_list_ty *defmlp = def->item[k]->messages;

	  /* Ignore the default message domain if it has no messages.  */
	  if (k > 0 || defmlp->nitems > 0)
	    match_domain (fn1, fn2, defmlp, refmlp, &nerrors);
	}
    }

  /* Look for messages in the definition file, which are not present
     in the reference file, indicating messages which defined but not
     used in the program.  */
  for (k = 0; k < def->nitems; ++k)
    {
      message_list_ty *defmlp = def->item[k]->messages;

      for (j = 0; j < defmlp->nitems; j++)
	{
	  message_ty *defmsg = defmlp->item[j];

	  if (!defmsg->used)
	    po_gram_error_at_line (&defmsg->pos,
				   _("warning: this message is not used"));
	}
    }

  /* Exit with status 1 on any error.  */
  if (nerrors > 0)
    error (EXIT_FAILURE, 0,
	   ngettext ("found %d fatal error", "found %d fatal errors", nerrors),
	   nerrors);
}
