/* GNU gettext - internationalization aids
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.

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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "dir-list.h"
#include "error.h"
#include "getline.h"
#include "libgettext.h"
#include "message.h"
#include "po.h"
#include "system.h"


/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


/* If nonzero add comments for file name and line number for each msgid.  */
static int line_comment = 1;

/* Name of default domain file.  If not set defaults to messages.po.  */
static char *default_domain;

/* Force output of PO file even if empty.  */
static int force_po;

/* Directory in which output files are created.  */
static char *output_dir;

/* If nonzero omit header with information about this run.  */
static int omit_header;

/* String containing name the program is called with.  */
const char *program_name;

/* These variables control which messages are selected.  */
static int more_than = -1;
static int less_than = -1;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-comments", optional_argument, NULL, 'c' },
  { "add-location", no_argument, &line_comment, 1 },
  { "default-domain", required_argument, NULL, 'd' },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "files-from", required_argument, NULL, 'f' },
  { "force-po", no_argument, &force_po, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "join-existing", no_argument, NULL, 'j' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, &line_comment, 0 },
  { "omit-header", no_argument, &omit_header, 1 },
  { "output", required_argument, NULL, 'o' },
  { "output-dir", required_argument, NULL, 'p' },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "strict", no_argument, NULL, 'S' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { "more-than", required_argument, NULL, '>', },
  { "less-than", required_argument, NULL, '<', },
  { NULL, 0, NULL, 0 }
};


/* Prototypes for local functions.  */
static void usage PARAMS ((int status))
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ > 4) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;
static void error_print PARAMS ((void));
static string_list_ty *read_name_from_file PARAMS ((const char *__file_name));
static void extract_constructor PARAMS ((po_ty *__that));
static void extract_directive_domain PARAMS ((po_ty *__that, char *__name));
static void extract_directive_message PARAMS ((po_ty *__that, char *__msgid,
					       lex_pos_ty *__msgid_pos,
					       char *__msgstr,
					       lex_pos_ty *__msgstr_pos));
static void extract_parse_brief PARAMS ((po_ty *__that));
static void extract_comment PARAMS ((po_ty *__that, const char *__s));
static void extract_comment_dot PARAMS ((po_ty *__that, const char *__s));
static void extract_comment_filepos PARAMS ((po_ty *__that, const char *__name,
					     int __line));
static void extract_comment_special PARAMS ((po_ty *that, const char *s));
static void read_po_file PARAMS ((const char *__file_name,
				  message_list_ty *__mlp));


int
main (argc, argv)
     int argc;
     char *argv[];
{
  int cnt;
  int optchar;
  int do_help = 0;
  int do_version = 0;
  message_list_ty *mlp;
  int sort_output = 0;
  int sort_by_file = 0;
  char *file_name;
  const char *files_from = NULL;
  string_list_ty *file_list;
  char *output_file = NULL;
  size_t j;

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

  /* Set initial value of variables.  */
  line_comment = -1;
  default_domain = MESSAGE_DOMAIN_DEFAULT;

  while ((optchar = getopt_long (argc, argv,
				 "<>ac::Cd:D:eEf:Fhijk::l:L:m::M::no:p:sTuVw:x:",
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
      case 'd':
	default_domain = optarg;
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
      case 'f':
	files_from = optarg;
	break;
      case 'F':
	sort_by_file = 1;
        break;
      case 'h':
	do_help = 1;
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
	{
	  size_t len = strlen (optarg);

	  if (output_dir != NULL)
	    free (output_dir);

	  if (optarg[len - 1] == '/')
	    output_dir = xstrdup (optarg);
	  else
	    {
	      asprintf (&output_dir, "%s/", optarg);
	      if (output_dir == NULL)
		/* We are about to construct the absolute path to the
		   directory for the output files but asprintf failed.  */
		error (EXIT_FAILURE, errno, _("while preparing output"));
	    }
	}
	break;
      case 's':
	sort_output = 1;
	break;
      case 'S':
	message_print_style_uniforum ();
	break;
      case 'u':
        less_than = 2;
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
	/* NOTREACHED */
      }

  /* Normalize selected options.  */
  if (omit_header != 0 && line_comment < 0)
    line_comment = 0;

  if (!line_comment && sort_by_file)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--no-location", "--sort-by-file");

  if (sort_output && sort_by_file)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--sort-output", "--sort-by-file");

  /* Version information requested.  */
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

  /* Default output directory is the current directory.  */
  if (output_dir == NULL)
    output_dir = ".";

  /* Construct the name of the ouput file.  If the default domain has
     the special name "-" we write to stdout.  */
  if (output_file)
  {
    if (output_file[0] == '/' ||
      strcmp(output_dir, ".") == 0 ||
      strcmp(output_file, "-") == 0
    )
      file_name = xstrdup (output_file);
    else
    {
      /* Please do NOT add a .po suffix! */
      file_name = xmalloc (strlen (output_dir) + strlen (default_domain) + 2);
      strcat (strcat (strcpy(file_name, output_dir), "/"), output_file);
    }
  }
  else if (strcmp (default_domain, "-") == 0)
    file_name = "-";
  else
    {
      file_name = (char *) xmalloc (strlen (output_dir)
				    + strlen (default_domain)
				    + sizeof (".po") + 2);
      stpcpy (stpcpy (stpcpy (stpcpy (file_name, output_dir), "/"),
	default_domain), ".po");
    }

  /* Determine list of files we have to process.  */
  if (files_from != NULL)
    file_list = read_name_from_file (files_from);
  else
    file_list = string_list_alloc ();
  /* Append names from command line.  */
  for (cnt = optind; cnt < argc; ++cnt)
    string_list_append_unique (file_list, argv[cnt]);

  /* Test whether sufficient input files weregiven.  */
  if (file_list->nitems < 2)
    {
      error (EXIT_SUCCESS, 0, _("at least two files must be specified"));
      usage (EXIT_FAILURE);
    }

  /* Allocate a message list to remember all the messages.  */
  mlp = message_list_alloc ();

  /* Process all input files.  */
  for (cnt = 0; cnt < file_list->nitems; ++cnt)
    read_po_file (file_list->item[cnt], mlp);
  string_list_free (file_list);

  /* Default the message selection criteria, and check them for sanity.  */
  if (more_than < 0)
    more_than = (less_than < 0 ? 1 : 0);
  if (less_than < 0)
    less_than = INT_MAX;
  if (more_than >= less_than || less_than < 2 || more_than >= mlp->nitems)
    error (EXIT_FAILURE, 0,
           _("impossible selection criteria specified (%d < n < %d)"),
           more_than, less_than);

  /* Remove messages which do not fit the criteria.  */
  j = 0;
  while (j < mlp->nitems)
    {
      message_ty *mp;

      mp = mlp->item[j];
      if (mp->used > more_than && mp->used < less_than)
        ++j;
      else
        message_list_delete_nth(mlp, j);
    }

  /* Sorting the list of messages.  */
  if (sort_by_file)
    message_list_sort_by_filepos (mlp);
  else if (sort_output)
    message_list_sort_by_msgid (mlp);

  /* Write the PO file.  */
  message_list_print (mlp, file_name, force_po, 0);

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
Usage: %s [OPTION] INPUTFILE ...\n\
Mandatory arguments to long options are mandatory for short options too.\n\
  -d, --default-domain=NAME      use NAME.po for output (instead of messages.po)\n\
  -D, --directory=DIRECTORY      add DIRECTORY to list for input files search\n\
  -e, --no-escape                do not use C escapes in output (default)\n\
  -E, --escape                   use C escapes in output, no extended chars\n\
  -f, --files-from=FILE          get list of input files from FILE\n\
      --force-po                 write PO file even if empty\n\
  -F, --sort-by-file             sort output by file location\n\
  -h, --help                     display this help and exit\n"),
	      program_name);
      fputs (_("\
  -i, --indent                   write the .po file using indented style\n\
      --no-location              do not write '#: filename:line' lines\n\
  -n, --add-location             generate '#: filename:line' lines (default)\n\
      --omit-header              don't write header with `msgid \"\"' entry\n\
  -o, --output=FILE              write output to specified file\n\
  -p, --output-dir=DIR           output files will be placed in directory DIR\n\
  -s, --sort-output              generate sorted output and remove duplicates\n\
      --strict                   write out strict Uniforum conforming .po file\n\
  -T, --trigraphs                understand ANSI C trigraphs for input\n\
  -u, --unique                   shorthand for --less-than=2, requests\n\
                                 that only unique messages be printed\n"),
	     stdout);
      fputs (_("\
  -V, --version                  output version information and exit\n\
  -w, --width=NUMBER             set output page width\n\
  -<, --less-than=NUMBER         print messages with less than this many\n\
                                 definitions, defaults to infinite if not\n\
                                 set\n\
  ->, --more-than=NUMBER         print messages with more than this many\n\
                                 definitions, defaults to 1 if not set\n\
\n\
Find messages which are common to two or more of the specified PO files.\n\
By using the --more-than option, greater commonality may be requested\n\
before messages are printed.  Conversely, the --less-than option may be\n\
used to specify less commonality before messages are printed (i.e.\n\
--less-than=2 will only print the unique messages).  Translations,\n\
comments and extract comments will be preserved, but only from the first\n\
PO file to define them.  File positions from all PO files will be\n\
preserved.\n"), stdout);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"),
	     stdout);
    }

  exit (status);
}


/* The address of this function will be assigned to the hook in the error
   functions.  */
static void
error_print ()
{
  /* We don't want the program name to be printed in messages.  */
}


/* Read list of files to process from file.  */
static string_list_ty *
read_name_from_file (file_name)
     const char *file_name;
{
  size_t line_len = 0;
  char *line_buf = NULL;
  FILE *fp;
  string_list_ty *result;

  if (strcmp (file_name, "-") == 0)
    fp = stdin;
  else
    {
      fp = fopen (file_name, "r");
      if (fp == NULL)
	error (EXIT_FAILURE, errno,
	       _("error while opening \"%s\" for reading"), file_name);
    }

  result = string_list_alloc ();

  while (!feof (fp))
    {
      /* Read next line from file.  */
      int len = getline (&line_buf, &line_len, fp);

      /* In case of an error leave loop.  */
      if (len < 0)
	break;

      /* Remove trailing '\n'.  */
      if (len > 0 && line_buf[len - 1] == '\n')
	line_buf[--len] = '\0';

      /* Test if we have to ignore the line.  */
      if (*line_buf == '\0' || *line_buf == '#')
	continue;

      string_list_append_unique (result, line_buf);
    }

  /* Free buffer allocated through getline.  */
  if (line_buf != NULL)
    free (line_buf);

  /* Close input stream.  */
  if (fp != stdin)
    fclose (fp);

  return result;
}


typedef struct extract_class_ty extract_class_ty;
struct extract_class_ty
{
  /* Inherited instance variables and methods.  */
  PO_BASE_TY

  /* Cumulative list of messages.  */
  message_list_ty *mlp;

  /* Cumulative comments for next message.  */
  string_list_ty *comment;
  string_list_ty *comment_dot;

  int is_fuzzy;
  int is_c_format;
  int do_wrap;

  int filepos_count;
  lex_pos_ty *filepos;
};


static void
extract_constructor (that)
     po_ty *that;
{
  extract_class_ty *this = (extract_class_ty *) that;

  this->mlp = NULL; /* actually set in read_po_file, below */
  this->comment = NULL;
  this->comment_dot = NULL;
  this->is_fuzzy = 0;
  this->is_c_format = undecided;
  this->do_wrap = undecided;
  this->filepos_count = 0;
  this->filepos = NULL;
}


static void
extract_directive_domain (that, name)
     po_ty *that;
     char *name;
{
  po_gram_error (_("this file may not contain domain directives"));
}


static void
extract_directive_message (that, msgid, msgid_pos, msgstr, msgstr_pos)
     po_ty *that;
     char *msgid;
     lex_pos_ty *msgid_pos;
     char *msgstr;
     lex_pos_ty *msgstr_pos;
{
  extract_class_ty *this = (extract_class_ty *)that;
  message_ty *mp;
  message_variant_ty *mvp;
  size_t j;

  /* If the msgid is the empty string, and we are omiting headers, throw
     it away.  */
  if (omit_header && *msgid == '\0')
    {
      free (msgid);
      free (msgstr);
      if (this->comment != NULL)
	string_list_free (this->comment);
      if (this->comment_dot != NULL)
	string_list_free (this->comment_dot);
      if (this->filepos != NULL)
	free (this->filepos);
      this->comment = NULL;
      this->comment_dot = NULL;
      this->filepos_count = 0;
      this->filepos = NULL;
      this->is_fuzzy = 0;
      this->is_c_format = undecided;
      this->do_wrap = undecided;
      return;
    }

  /* See if this message ID has been seen before.  */
  mp = message_list_search (this->mlp, msgid);
  if (mp)
    free (msgid);
  else
    {
      mp = message_alloc (msgid);
      message_list_append (this->mlp, mp);
    }

  /* The ``obsolete'' flag is cleared before reading each PO file.
     If thisflag is clear, set it, and increment the ``used'' counter.
     This allows us to count how many of the PO files use the message.  */
  if (mp->obsolete == 0)
    {
      mp->obsolete = 1;
      mp->used++;
    }

  /* Add the accumulated comments to the message.  Clear the
     accumulation in preparation for the next message. */
  if (this->comment != NULL)
    {
      if (mp->comment == NULL)
        for (j = 0; j < this->comment->nitems; ++j)
	  message_comment_append (mp, this->comment->item[j]);
      string_list_free (this->comment);
      this->comment = NULL;
    }
  if (this->comment_dot != NULL)
    {
      if (mp->comment_dot == NULL)
        for (j = 0; j < this->comment_dot->nitems; ++j)
	  message_comment_dot_append (mp, this->comment_dot->item[j]);
      string_list_free (this->comment_dot);
      this->comment_dot = NULL;
    }
  mp->is_fuzzy |= this->is_fuzzy;
  if (mp->is_c_format == undecided)
    mp->is_c_format = this->is_c_format;
  if (mp->do_wrap == undecided)
    mp->do_wrap = this->do_wrap;
  for (j = 0; j < this->filepos_count; ++j)
    {
      lex_pos_ty *pp;

      pp = &this->filepos[j];
      message_comment_filepos (mp, pp->file_name, pp->line_number);
      free (pp->file_name);
    }
  if (this->filepos != NULL)
    free (this->filepos);
  this->filepos_count = 0;
  this->filepos = NULL;
  this->is_fuzzy = 0;
  this->is_c_format = undecided;
  this->do_wrap = undecided;

  /* See if this domain has been seen for this message ID.  */
  mvp = message_variant_search (mp, MESSAGE_DOMAIN_DEFAULT);
  if (mvp != NULL)
    free (msgstr);
  else
    message_variant_append (mp, MESSAGE_DOMAIN_DEFAULT, msgstr, msgstr_pos);
}


static void
extract_parse_brief (that)
     po_ty *that;
{
  po_lex_pass_comments (1);
}


static void
extract_comment (that, s)
     po_ty *that;
     const char *s;
{
  extract_class_ty *this = (extract_class_ty *) that;

  if (this->comment == NULL)
    this->comment = string_list_alloc ();
  string_list_append (this->comment, s);
}


static void
extract_comment_dot (that, s)
     po_ty *that;
     const char *s;
{
  extract_class_ty *this = (extract_class_ty *) that;

  if (this->comment_dot == NULL)
    this->comment_dot = string_list_alloc ();
  string_list_append (this->comment_dot, s);
}


static void
extract_comment_filepos (that, name, line)
     po_ty *that;
     const char *name;
     int line;
{
  extract_class_ty *this = (extract_class_ty *) that;
  size_t nbytes;
  lex_pos_ty *pp;

  /* Write line numbers only if -n option is given.  */
  if (line_comment != 0)
    {
      nbytes = (this->filepos_count + 1) * sizeof (this->filepos[0]);
      this->filepos = xrealloc (this->filepos, nbytes);
      pp = &this->filepos[this->filepos_count++];
      pp->file_name = xstrdup (name);
      pp->line_number = line;
    }
}


static void
extract_comment_special (that, s)
     po_ty *that;
     const char *s;
{
  extract_class_ty *this = (extract_class_ty *) that;

  if (strstr (s, "fuzzy") != NULL)
    this->is_fuzzy = 1;
  if (strstr (s, "c-format") != NULL)
    this->is_c_format = yes;
  if (strstr (s, "no-c-format") != NULL)
    this->is_c_format = no;
  if (strstr (s, "wrap") != NULL)
    this->do_wrap = yes;
  if (strstr (s, "no-wrap") != NULL)
    this->do_wrap = no;
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static po_method_ty extract_methods =
{
  sizeof (extract_class_ty),
  extract_constructor,
  NULL, /* destructor */
  extract_directive_domain,
  extract_directive_message,
  extract_parse_brief,
  NULL, /* parse_debrief */
  extract_comment,
  extract_comment_dot,
  extract_comment_filepos,
  extract_comment_special
};


/* Read the contents of the specified .po file into a message list.  */

static void
read_po_file (file_name, mlp)
     const char *file_name;
     message_list_ty *mlp;
{
  size_t j;

  po_ty *pop = po_alloc (&extract_methods);
  ((extract_class_ty *) pop)->mlp = mlp;
  po_scan (pop, file_name);
  po_free (pop);

  /* The ``obsolete'' flag of each message is cleared before reading
     each PO file.  As each message is read from a PO file, if this flag
     is clear, it is set, and increment the ``used'' counter.  This
     allows us to count how many of the PO files use each message.  */
  for (j = 0; j < mlp->nitems; ++j)
    mlp->item[j]->obsolete = 0;
}
