/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "dir-list.h"
#include "error.h"
#include "message.h"
#include <system.h>
#include <libintl.h>
#include "po.h"
#include "str-list.h"

#define _(str) gettext (str)


/* This structure defines a derived class of the po_ty class.  (See
   po.h for an explanation.)  */
typedef struct compare_class_ty compare_class_ty;
struct compare_class_ty
{
  /* inherited instance variables, etc */
  PO_BASE_TY

  /* Name of domain we are currently examining.  */
  char *domain;

  /* List of domains already appeared in the current file.  */
  string_list_ty *domain_list;

  /* List of messages already appeared in the current file.  */
  message_list_ty *mlp;
};

/* String containing name the program is called with.  */
const char *program_name;

/* Long options.  */
static const struct option long_options[] =
{
  { "directory", required_argument, NULL, 'D' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};


/* Prototypes for local functions.  */
static void usage PARAMS ((int __status));
static void error_print PARAMS ((void));
static void compare PARAMS ((char *, char *));
static message_list_ty *grammar PARAMS ((char *__filename));
static void compare_constructor PARAMS ((po_ty *__that));
static void compare_destructor PARAMS ((po_ty *__that));
static void compare_directive_domain PARAMS ((po_ty *__that, char *__name));
static void compare_directive_message PARAMS ((po_ty *__that, char *__msgid,
					       lex_pos_ty *msgid_pos,
					       char *__msgstr,
					       lex_pos_ty *__msgstr_pos));
static void compare_parse_debrief PARAMS ((po_ty *__that));


int
main (argc, argv)
     int argc;
     char *argv[];
{
  int optchar;
  int do_help;
  int do_version;

  /* Set program name for messages.  */
  program_name = argv[0];
  error_print_progname = error_print;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  do_help = 0;
  do_version = 0;
  while ((optchar = getopt_long (argc, argv, "D:hV", long_options, NULL))
	 != EOF)
    switch (optchar)
      {
      case '\0':		/* long option */
	break;

      case 'D':
	dir_list_append (optarg);
	break;

      case 'h':
	do_help = 1;
	break;

      case 'V':
	do_version = 1;
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
	      "1995, 1996, 1997, 1998");
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
usage (status)
     int status;
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      /* xgettext: no-wrap */
      printf (_("\
Usage: %s [OPTION] def.po ref.po\n\
Mandatory arguments to long options are mandatory for short options too.\n\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n\
  -h, --help                  display this help and exit\n\
  -V, --version               output version information and exit\n\
\n\
Compare two Uniforum style .po files to check that both contain the same\n\
set of msgid strings.  The def.po file is an existing PO file with the\n\
old translations.  The ref.po file is the last created PO file\n\
(generally by xgettext).  This is useful for checking that you have\n\
translated each and every message in your program.  Where an exact match\n\
cannot be found, fuzzy matching is used to produce better diagnostics.\n"),
	      program_name);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"), stdout);
    }

  exit (status);
}


/* The address of this function will be assigned to the hook in the error
   functions.  */
static void
error_print ()
{
  /* We don't want the program name to be printed in messages.  Emacs'
     compile.el does not like this.  */
}


static void
compare (fn1, fn2)
     char *fn1;
     char *fn2;
{
  message_list_ty *list1;
  message_list_ty *list2;
  int nerrors;
  message_ty *mp1;
  size_t j, k;

  /* This is the master file, created by a human.  */
  list1 = grammar (fn1);

  /* This is the generated file, created by groping the sources with
     the xgettext program.  */
  list2 = grammar (fn2);

  /* Every entry in the xgettext generated file must be matched by a
     (single) entry in the human created file.  */
  nerrors = 0;
  for (j = 0; j < list2->nitems; ++j)
    {
      message_ty *mp2;

      mp2 = list2->item[j];

      /* See if it is in the other file.  */
      mp1 = message_list_search (list1, mp2->msgid);
      if (mp1)
	{
	  mp1->used = 1;
	  continue;
	}

      /* If the message was not defined at all, try to find a very
	 similar message, it could be a typo, or the suggestion may
	 help.  */
      ++nerrors;
      mp1 = message_list_search_fuzzy (list1, mp2->msgid);
      if (mp1)
	{
	  gram_error_at_line (&mp2->variant[0].pos, _("\
this message is used but not defined..."));
	  gram_error_at_line (&mp1->variant[0].pos, _("\
...but this definition is similar"));
	  mp1->used = 1;
	}
      else
	{
	  gram_error_at_line (&mp2->variant[0].pos, _("\
this message is used but not defined in %s"), fn1);
	}
    }

  /* Look for messages in the human generated file, which are not
     present in the xgettext generated file, indicating messages which
     are not used in the program.  */
  for (k = 0; k < list1->nitems; ++k)
    {
      mp1 = list1->item[k];
      if (mp1->used)
	continue;
      gram_error_at_line (&mp1->variant[0].pos,
			   _("warning: this message is not used"));
    }

  /* Exit with status 1 on any error.  */
  if (nerrors > 0)
    error (EXIT_FAILURE, 0, "found %d fatal errors", nerrors);
}


/* Local functions.  */

static void
compare_constructor (that)
     po_ty *that;
{
  compare_class_ty *this = (compare_class_ty *) that;

  this->mlp = message_list_alloc ();
  this->domain = MESSAGE_DOMAIN_DEFAULT;
  this->domain_list = string_list_alloc ();
}


static void
compare_destructor (that)
     po_ty *that;
{
  compare_class_ty *this = (compare_class_ty *) that;

  string_list_free (this->domain_list);
  /* Do not free this->mlp!  */
}


static void
compare_directive_domain (that, name)
     po_ty *that;
     char *name;
{
  compare_class_ty *this = (compare_class_ty *)that;
  /* Override current domain name.  Don't free memory.  */
  this->domain = name;
}


static void
compare_directive_message (that, msgid, msgid_pos, msgstr, msgstr_pos)
     po_ty *that;
     char *msgid;
     lex_pos_ty *msgid_pos;
     char *msgstr;
     lex_pos_ty *msgstr_pos;
{
  compare_class_ty *this = (compare_class_ty *) that;
  message_ty *mp;
  message_variant_ty *mvp;

  /* Remember the domain names for later.  */
  string_list_append_unique (this->domain_list, this->domain);

  /* See if this message ID has been seen before.  */
  mp = message_list_search (this->mlp, msgid);
  if (mp)
    free (msgid);
  else
    {
      mp = message_alloc (msgid);
      message_list_append (this->mlp, mp);
    }

  /* See if this domain has been seen for this message ID.  */
  mvp = message_variant_search (mp, this->domain);
  if (mvp)
    {
      gram_error_at_line (msgid_pos, _("duplicate message definition"));
      gram_error_at_line (&mvp->pos, _("\
...this is the location of the first definition"));
      free (msgstr);
    }
  else
    message_variant_append (mp, this->domain, msgstr, msgstr_pos);
}


static void
compare_parse_debrief (that)
     po_ty *that;
{
  compare_class_ty *this = (compare_class_ty *) that;
  message_list_ty *mlp = this->mlp;
  size_t j;

  /* For each domain in the used-domain-list, make sure each message
     defines a msgstr in that domain.  */
  for (j = 0; j < this->domain_list->nitems; ++j)
    {
      const char *domain_name;
      size_t k;

      domain_name = this->domain_list->item[j];
      for (k = 0; k < mlp->nitems; ++k)
	{
	  const message_ty *mp;
	  size_t m;

	  mp = mlp->item[k];
	  for (m = 0; m < mp->variant_count; ++m)
	    {
	      message_variant_ty *mvp;

	      mvp = &mp->variant[m];
	      if (strcmp (domain_name, mvp->domain) == 0)
		break;
	    }
	  if (m >= mp->variant_count)
	    gram_error_at_line (&mp->variant[0].pos, _("\
this message has no definition in the \"%s\" domain"), domain_name);
	}
    }
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invokations of method functions of that object.  */

static po_method_ty compare_methods =
{
  sizeof (compare_class_ty),
  compare_constructor,
  compare_destructor,
  compare_directive_domain,
  compare_directive_message,
  NULL, /* parse_brief */
  compare_parse_debrief,
  NULL, /* comment */
  NULL, /* comment_dot */
  NULL, /* comment_filepos */
  NULL, /* comment_special */
};


static message_list_ty *
grammar (filename)
     char *filename;
{
  po_ty *pop;
  message_list_ty *mlp;

  pop = po_alloc(&compare_methods);
  po_scan(pop, filename);
  mlp = ((compare_class_ty *)pop)->mlp;
  po_free(pop);
  return mlp;
}
