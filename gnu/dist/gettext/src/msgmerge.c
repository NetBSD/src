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
#include <limits.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#if HAVE_LOCALE_H
# include <locale.h>
#endif

#include "dir-list.h"
#include "error.h"
#include "message.h"
#include <system.h>
#include <libintl.h>
#include "po.h"

#define _(str) gettext (str)


/* This structure defines a derived class of the po_ty class.  (See
   po.h for an explanation.)  */
typedef struct merge_class_ty merge_class_ty;
struct merge_class_ty
{
  /* inherited instance variables, etc */
  PO_BASE_TY

  /* Name of domain we are currently examining.  */
  char *domain;

  /* List of domains already appeared in the current file.  */
  string_list_ty *domain_list;

  /* List of messages already appeared in the current file.  */
  message_list_ty *mlp;

  /* Accumulate comments for next message directive */
  string_list_ty *comment;
  string_list_ty *comment_dot;

  /* Flags transported in special comments.  */
  int is_fuzzy;
  enum is_c_format is_c_format;
  enum is_c_format do_wrap;

  /* Accumulate filepos comments for the next message directive.  */
  size_t filepos_count;
  lex_pos_ty *filepos;
};


/* String containing name the program is called with.  */
const char *program_name;

/* If non-zero do not print unneeded messages.  */
static int quiet;

/* Verbosity level.  */
static int verbosity_level;

/* If nonzero, remember comments for file name and line number for each
   msgid, if present in the reference input.  Defaults to true.  */
static int line_comment = 1;

/* Force output of PO file even if empty.  */
static int force_po;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-location", no_argument, &line_comment, 1 },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "force-po", no_argument, &force_po, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, &line_comment, 0 },
  { "output-file", required_argument, NULL, 'o' },
  { "quiet", no_argument, NULL, 'q' },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "silent", no_argument, NULL, 'q' },
  { "strict", no_argument, NULL, 'S' },
  { "verbose", no_argument, NULL, 'v' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { NULL, 0, NULL, 0 }
};


/* Prototypes for local functions.  */
static void usage PARAMS ((int __status));
static void error_print PARAMS ((void));
static void merge_constructor PARAMS ((po_ty *__that));
static void merge_destructor PARAMS ((po_ty *__that));
static void merge_directive_domain PARAMS ((po_ty *__that, char *__name));
static void merge_directive_message PARAMS ((po_ty *__that, char *__msgid,
					     lex_pos_ty *__msgid_pos,
					     char *__msgstr,
					     lex_pos_ty *__msgstr_pos));
static void merge_parse_brief PARAMS ((po_ty *__that));
static void merge_parse_debrief PARAMS ((po_ty *__that));
static void merge_comment PARAMS ((po_ty *__that, const char *__s));
static void merge_comment_dot PARAMS ((po_ty *__that, const char *__s));
static void merge_comment_special PARAMS ((po_ty *__that, const char *__s));
static void merge_comment_filepos PARAMS ((po_ty *__that, const char *__name,
					   int __line));
static message_list_ty *grammar PARAMS ((const char *__filename));
static message_list_ty *merge PARAMS ((const char *__fn1, const char *__fn2));


int
main (argc, argv)
     int argc;
     char **argv;
{
  int opt;
  int do_help;
  int do_version;
  char *output_file;
  message_list_ty *result;
  int sort_by_filepos = 0;
  int sort_by_msgid = 0;

  /* Set program name for messages.  */
  program_name = argv[0];
  verbosity_level = 0;
  quiet = 0;
  error_print_progname = error_print;
  gram_max_allowed_errors = INT_MAX;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Set default values for variables.  */
  do_help = 0;
  do_version = 0;
  output_file = NULL;

  while ((opt
	  = getopt_long (argc, argv, "D:eEFhio:qsvVw:", long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case '\0':		/* Long option.  */
	break;

      case 'D':
	dir_list_append (optarg);
	break;

      case 'e':
	message_print_style_escape (0);
	break;

      case 'E':
	message_print_style_escape (1);
	break;

      case 'F':
        sort_by_filepos = 1;
        break;

      case 'h':
	do_help = 1;
	break;

      case 'i':
	message_print_style_indent ();
	break;

      case 'o':
	output_file = optarg;
	break;

      case 'q':
	quiet = 1;
	break;

      case 's':
        sort_by_msgid = 1;
        break;

      case 'S':
	message_print_style_uniforum ();
	break;

      case 'v':
	++verbosity_level;
	break;

      case 'V':
	do_version = 1;
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

  /* merge the two files */
  result = merge (argv[optind], argv[optind + 1]);

  /* Sort the results.  */
  if (sort_by_filepos)
    message_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    message_list_sort_by_msgid (result);

  /* Write the merged message list out.  */
  message_list_print (result, output_file, force_po, 0);

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
  -e, --no-escape             do not use C escapes in output (default)\n\
  -E, --escape                use C escapes in output, no extended chars\n\
      --force-po              write PO file even if empty\n\
  -h, --help                  display this help and exit\n\
  -i, --indent                indented output style\n\
  -o, --output-file=FILE      result will be written to FILE\n\
      --no-location           suppress '#: filename:line' lines\n\
      --add-location          preserve '#: filename:line' lines (default)\n\
      --strict                strict Uniforum output style\n\
  -v, --verbose               increase verbosity level\n\
  -V, --version               output version information and exit\n\
  -w, --width=NUMBER          set output page width\n"),
		program_name);
      /* xgettext: no-wrap */
      fputs (_("\n\
Merges two Uniforum style .po files together.  The def.po file is an\n\
existing PO file with the old translations which will be taken over to\n\
the newly created file as long as they still match; comments will be\n\
preserved, but extract comments and file positions will be discarded.\n\
The ref.po file is the last created PO file (generally by xgettext), any\n\
translations or comments in the file will be discarded, however dot\n\
comments and file positions will be preserved.  Where an exact match\n\
cannot be found, fuzzy matching is used to produce better results.  The\n\
results are written to stdout unless an output file is specified.\n"), stdout);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"),
	     stdout);
    }

  exit (status);
}


/* The address of this function will be assigned to the hook in the
   error functions.  */
static void
error_print ()
{
  /* We don't want the program name to be printed in messages.  Emacs'
     compile.el does not like this.  */

  /* FIXME Why must this program toady to Emacs?  Why can't compile.el
     be enhanced to cope with a leading program name?  --PMiller */
}


static void
merge_constructor (that)
     po_ty *that;
{
  merge_class_ty *this = (merge_class_ty *) that;

  this->mlp = message_list_alloc ();
  this->domain = MESSAGE_DOMAIN_DEFAULT;
  this->domain_list = string_list_alloc ();
  this->comment = NULL;
  this->comment_dot = NULL;
  this->filepos_count = 0;
  this->filepos = NULL;
  this->is_fuzzy = 0;
  this->is_c_format = undecided;
  this->do_wrap = undecided;
}


static void
merge_destructor (that)
     po_ty *that;
{
  merge_class_ty *this = (merge_class_ty *) that;
  size_t j;

  string_list_free (this->domain_list);
  /* Do not free this->mlp.  */
  if (this->comment != NULL)
    string_list_free (this->comment);
  if (this->comment_dot != NULL)
    string_list_free (this->comment_dot);
  for (j = 0; j < this->filepos_count; ++j)
    free (this->filepos[j].file_name);
  if (this->filepos != NULL)
    free (this->filepos);
}


static void
merge_directive_domain (that, name)
     po_ty *that;
     char *name;
{
  size_t j;

  merge_class_ty *this = (merge_class_ty *) that;
  /* Override current domain name.  Don't free memory.  */
  this->domain = name;

  /* If there are accumulated comments, throw them away, they are
     probably part of the file header, or about the domain directive,
     and will be unrelated to the next message.  */
  if (this->comment != NULL)
    {
      string_list_free (this->comment);
      this->comment = NULL;
    }
  if (this->comment_dot != NULL)
    {
      string_list_free (this->comment_dot);
      this->comment_dot = NULL;
    }
  for (j = 0; j < this->filepos_count; ++j)
    free (this->filepos[j].file_name);
  if (this->filepos != NULL)
    free (this->filepos);
  this->filepos_count = 0;
  this->filepos = NULL;
}


static void
merge_directive_message (that, msgid, msgid_pos, msgstr, msgstr_pos)
     po_ty *that;
     char *msgid;
     lex_pos_ty *msgid_pos;
     char *msgstr;
     lex_pos_ty *msgstr_pos;
{
  merge_class_ty *this = (merge_class_ty *) that;
  message_ty *mp;
  message_variant_ty *mvp;
  size_t j;

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

  /* Add the accumulated comments to the message.  Clear the
     accumulation in preparation for the next message.  */
  if (this->comment != NULL)
    {
      for (j = 0; j < this->comment->nitems; ++j)
	message_comment_append (mp, this->comment->item[j]);
      string_list_free (this->comment);
      this->comment = NULL;
    }
  if (this->comment_dot != NULL)
    {
      for (j = 0; j < this->comment_dot->nitems; ++j)
	message_comment_dot_append (mp, this->comment_dot->item[j]);
      string_list_free (this->comment_dot);
      this->comment_dot = NULL;
    }
  for (j = 0; j < this->filepos_count; ++j)
    {
      lex_pos_ty *pp;

      pp = &this->filepos[j];
      message_comment_filepos (mp, pp->file_name, pp->line_number);
      free (pp->file_name);
    }
  mp->is_fuzzy = this->is_fuzzy;
  mp->is_c_format = this->is_c_format;
  mp->do_wrap = this->do_wrap;

  if (this->filepos != NULL)
    free (this->filepos);
  this->filepos_count = 0;
  this->filepos = NULL;
  this->is_fuzzy = 0;
  this->is_c_format = undecided;
  this->do_wrap = undecided;

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
merge_parse_brief (that)
     po_ty *that;
{
  po_lex_pass_comments (1);
}


static void
merge_parse_debrief (that)
     po_ty *that;
{
  merge_class_ty *this = (merge_class_ty *) that;
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


static void
merge_comment (that, s)
     po_ty *that;
     const char *s;
{
  merge_class_ty *this = (merge_class_ty *) that;

  if (this->comment == NULL)
    this->comment = string_list_alloc ();
  string_list_append (this->comment, s);
}


static void
merge_comment_dot (that, s)
     po_ty *that;
     const char *s;
{
  merge_class_ty *this = (merge_class_ty *) that;

  if (this->comment_dot == NULL)
    this->comment_dot = string_list_alloc ();
  string_list_append (this->comment_dot, s);
}


static void
merge_comment_special (that, s)
     po_ty *that;
     const char *s;
{
  merge_class_ty *this = (merge_class_ty *) that;

  if (strstr (s, "fuzzy") != NULL)
    this->is_fuzzy = 1;

  this->is_c_format = parse_c_format_description_string (s);
  this->do_wrap = parse_c_width_description_string (s);
}


static void
merge_comment_filepos (that, name, line)
     po_ty *that;
     const char *name;
     int line;
{
  merge_class_ty *this = (merge_class_ty *) that;
  size_t nbytes;
  lex_pos_ty *pp;

  if (!line_comment)
    return;
  nbytes = (this->filepos_count + 1) * sizeof (this->filepos[0]);
  this->filepos = xrealloc (this->filepos, nbytes);
  pp = &this->filepos[this->filepos_count++];
  pp->file_name = xstrdup (name);
  pp->line_number = line;
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invokations of method functions of that object.  */

static po_method_ty merge_methods =
{
  sizeof (merge_class_ty),
  merge_constructor,
  merge_destructor,
  merge_directive_domain,
  merge_directive_message,
  merge_parse_brief,
  merge_parse_debrief,
  merge_comment,
  merge_comment_dot,
  merge_comment_filepos,
  merge_comment_special
};


static message_list_ty *
grammar (filename)
     const char *filename;
{
  po_ty *pop;
  message_list_ty *mlp;

  pop = po_alloc (&merge_methods);
  po_lex_pass_obsolete_entries (1);
  po_scan (pop, filename);
  mlp = ((merge_class_ty *) pop)->mlp;
  po_free (pop);
  return mlp;
}


#define DOT_FREQUENCE 10

static message_list_ty *
merge (fn1, fn2)
     const char *fn1;			/* definitions */
     const char *fn2;			/* references */
{
  message_list_ty *def;
  message_list_ty *ref;
  message_ty *defmsg;
  size_t j, k;
  size_t merged, fuzzied, missing, obsolete;
  message_list_ty *result;

  merged = fuzzied = missing = obsolete = 0;

  /* This is the definitions file, created by a human.  */
  def = grammar (fn1);

  /* This is the references file, created by groping the sources with
     the xgettext program.  */
  ref = grammar (fn2);

  result = message_list_alloc ();

  /* Every reference must be matched with its definition. */
  for (j = 0; j < ref->nitems; ++j)
    {
      message_ty *refmsg;

      /* Because merging can take a while we print something to signal
	 we are not dead.  */
      if (!quiet && verbosity_level <= 1 && j % DOT_FREQUENCE == 0)
	fputc ('.', stderr);

      refmsg = ref->item[j];

      /* See if it is in the other file.  */
      defmsg = message_list_search (def, refmsg->msgid);
      if (defmsg)
	{
	  /* Merge the reference with the definition: take the #. and
	     #: comments from the reference, take the # comments from
	     the definition, take the msgstr from the definition.  Add
	     this merged entry to the output message list.  */
	  message_ty *mp = message_merge (defmsg, refmsg);

	  message_list_append (result, mp);

	  /* Remember that this message has been used, when we scan
	     later to see if anything was omitted.  */
	  defmsg->used = 1;
	  ++merged;
	  continue;
	}

      /* If the message was not defined at all, try to find a very
	 similar message, it could be a typo, or the suggestion may
	 help.  */
      defmsg = message_list_search_fuzzy (def, refmsg->msgid);
      if (defmsg)
	{
	  message_ty *mp;

	  if (verbosity_level > 1)
	    {
	      gram_error_at_line (&refmsg->variant[0].pos, _("\
this message is used but not defined..."));
	      gram_error_at_line (&defmsg->variant[0].pos, _("\
...but this definition is similar"));
	    }

	  /* Merge the reference with the definition: take the #. and
	     #: comments from the reference, take the # comments from
	     the definition, take the msgstr from the definition.  Add
	     this merged entry to the output message list.  */
	  mp = message_merge (defmsg, refmsg);

	  mp->is_fuzzy = 1;

	  message_list_append (result, mp);

	  /* Remember that this message has been used, when we scan
	     later to see if anything was omitted.  */
	  defmsg->used = 1;
	  ++fuzzied;
	  if (!quiet && verbosity_level <= 1)
	    /* Always print a dot if we handled a fuzzy match.  */
	    fputc ('.', stderr);
	}
      else
	{
	  message_ty *mp;

	  if (verbosity_level > 1)
	      gram_error_at_line (&refmsg->variant[0].pos, _("\
this message is used but not defined in %s"), fn1);

	  mp = message_copy (refmsg);

	  message_list_append (result, mp);
	  ++missing;
	}
    }

  /* Look for messages in the definition file, which are not present
     in the reference file, indicating messages which defined but not
     used in the program.  */
  for (k = 0; k < def->nitems; ++k)
    {
      defmsg = def->item[k];
      if (defmsg->used)
	continue;

      /* Remember the old translation although it is not used anymore.
	 But we mark it as obsolete.  */
      defmsg->obsolete = 1;

      message_list_append (result, defmsg);
      ++obsolete;
    }

  /* Report some statistics.  */
  if (verbosity_level > 0)
    fprintf (stderr, _("%s\
Read %d old + %d reference, \
merged %d, fuzzied %d, missing %d, obsolete %d.\n"),
	     !quiet && verbosity_level <= 1 ? "\n" : "",
	     def->nitems, ref->nitems, merged, fuzzied, missing, obsolete);
  else if (!quiet)
    fputs (_(" done.\n"), stderr);

  return result;
}
