/* Extracts strings from C source file to Uniforum style .po file.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

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

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sys/param.h>
#include <pwd.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef errno
extern int errno;
#endif

#include "dir-list.h"
#include "error.h"
#include "hash.h"
#include "getline.h"
#include "system.h"
#include "po.h"
#include "message.h"
#include "xget-lex.h"
#include "printf-parse.h"

#include "gettext.h"
#include "domain.h"
#include <libintl.h>

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
#endif


/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


/* If nonzero add all comments immediately preceding one of the keywords. */
static int add_all_comments;

/* If nonzero add comments for file name and line number for each msgid.  */
static int line_comment;

/* Tag used in comment of prevailing domain.  */
static unsigned char *comment_tag;

/* Name of default domain file.  If not set defaults to messages.po.  */
static char *default_domain;

/* If called with --debug option the output reflects whether format
   string recognition is done automatically or forced by the user.  */
static int do_debug;

/* Content of .po files with symbols to be excluded.  */
static message_list_ty *exclude;

/* If nonzero extract all strings.  */
static int extract_all;

/* Force output of PO file even if empty.  */
static int force_po;

/* If nonzero a non GNU related user wants to use this.  Omit the FSF
   copyright in the output.  */
static int foreign_user;

/* String used as prefix for msgstr.  */
static char *msgstr_prefix;

/* String used as suffix for msgstr.  */
static char *msgstr_suffix;

/* Directory in which output files are created.  */
static char *output_dir;

/* If nonzero omit header with information about this run.  */
static int omit_header;

/* String containing name the program is called with.  */
const char *program_name;

/* String length from with on warning are given for possible problem
   while exceeding tools limits.  */
static size_t warn_id_len;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-comments", optional_argument, NULL, 'c' },
  { "add-location", no_argument, &line_comment, 1 },
  { "c++", no_argument, NULL, 'C' },
  { "debug", no_argument, &do_debug, 1 },
  { "default-domain", required_argument, NULL, 'd' },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "exclude-file", required_argument, NULL, 'x' },
  { "extract-all", no_argument, &extract_all, 1 },
  { "files-from", required_argument, NULL, 'f' },
  { "force-po", no_argument, &force_po, 1 },
  { "foreign-user", no_argument, &foreign_user, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "join-existing", no_argument, NULL, 'j' },
  { "keyword", optional_argument, NULL, 'k' },
  { "language", required_argument, NULL, 'L' },
  { "msgstr-prefix", optional_argument, NULL, 'm' },
  { "msgstr-suffix", optional_argument, NULL, 'M' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, &line_comment, 0 },
  { "omit-header", no_argument, &omit_header, 1 },
  { "output", required_argument, NULL, 'o' },
  { "output-dir", required_argument, NULL, 'p' },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "strict", no_argument, NULL, 'S' },
  { "string-limit", required_argument, NULL, 'l' },
  { "trigraphs", no_argument, NULL, 'T' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
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
static void exclude_directive_domain PARAMS ((po_ty *__pop, char *__name));
static void exclude_directive_message PARAMS ((po_ty *__pop, char *__msgid,
					       lex_pos_ty *__msgid_pos,
					       char *__msgstr,
					       lex_pos_ty *__msgstr_pos));
static void read_exclusion_file PARAMS ((char *__file_name));
static void remember_a_message PARAMS ((message_list_ty *__mlp,
					xgettext_token_ty *__tp));
static void scan_c_file PARAMS ((const char *__file_name,
				 message_list_ty *__mlp,  int __is_cpp_file));
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
static long difftm PARAMS ((const struct tm *__a, const struct tm *__b));
static message_ty *construct_header PARAMS ((void));
static enum is_c_format test_whether_c_format PARAMS ((const char *__s));


/* The scanners must all be functions returning void and taking one
   string argument and a message list argument.  */
typedef void (*scanner_fp) PARAMS ((const char *, message_list_ty *));

static void scanner_c PARAMS ((const char *, message_list_ty *));
static void scanner_cxx PARAMS ((const char *, message_list_ty *));
static const char *extension_to_language PARAMS ((const char *));
static scanner_fp language_to_scanner PARAMS ((const char *));


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
  int join_existing = 0;
  int sort_output = 0;
  int sort_by_file = 0;
  char *file_name;
  const char *files_from = NULL;
  string_list_ty *file_list;
  char *output_file = NULL;
  scanner_fp scanner = NULL;

  /* Set program name for messages.  */
  program_name = argv[0];
  error_print_progname = error_print;
  warn_id_len = WARN_ID_LEN;

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
				 "ac::Cd:D:eEf:Fhijk::l:L:m::M::no:p:sTVw:x:",
				 long_options, NULL)) != EOF)
    switch (optchar)
      {
      case '\0':		/* Long option.  */
	break;
      case 'a':
	extract_all = 1;
	break;
      case 'c':
	if (optarg == NULL)
	  {
	    add_all_comments = 1;
	    comment_tag = NULL;
	  }
	else
	  {
	    add_all_comments = 0;
	    comment_tag = optarg;
	    /* We ignore leading white space.  */
	    while (isspace (*comment_tag))
	      ++comment_tag;
	  }
	break;
      case 'C':
	scanner = language_to_scanner ("C++");
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
      case 'j':
	join_existing = 1;
	break;
      case 'k':
	if (optarg == NULL || *optarg != '\0')
	  xgettext_lex_keyword (optarg);
	break;
      case 'l':
	{
	  char *endp;
	  size_t tmp_val = strtoul (optarg, &endp, 0);
	  if (endp[0] == '\0')
	    warn_id_len = tmp_val;
	}
	break;
      case 'L':
	scanner = language_to_scanner (optarg);
	break;
      case 'm':
	/* -m takes an optional argument.  If none is given "" is assumed. */
	msgstr_prefix = optarg == NULL ? "" : optarg;
	break;
      case 'M':
	/* -M takes an optional argument.  If none is given "" is assumed. */
	msgstr_suffix = optarg == NULL ? "" : optarg;
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
      case 'T':
	xgettext_lex_trigraphs ();
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
      case 'x':
	read_exclusion_file (optarg);
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

  if (join_existing && strcmp (default_domain, "-") == 0)
    error (EXIT_FAILURE, 0, _("\
--join-existing cannot be used when output is written to stdout"));

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
      printf (_("Written by %s.\n"), "Ulrich Drepper");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test whether we have some input files given.  */
  if (files_from == NULL && optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input file given"));
      usage (EXIT_FAILURE);
    }

  /* Canonize msgstr prefix/suffix.  */
  if (msgstr_prefix != NULL && msgstr_suffix == NULL)
    msgstr_suffix = "";
  else if (msgstr_prefix == NULL && msgstr_suffix != NULL)
    msgstr_prefix = NULL;

  /* Default output directory is the current directory.  */
  if (output_dir == NULL)
    output_dir = ".";

  /* Construct the name of the ouput file.  If the default domain has
     the special name "-" we write to stdout.  */
  if (output_file)
    {
      if (output_file[0] == '/' ||
	  strcmp(output_dir, ".") == 0 || strcmp(output_file, "-") == 0)
	file_name = xstrdup (output_file);
      else
	{
	  /* Please do NOT add a .po suffix! */
	  file_name = xmalloc (strlen (output_dir)
			       + strlen (default_domain) + 2);
	  stpcpy (stpcpy (stpcpy (file_name, output_dir), "/"), output_file);
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

  /* Allocate a message list to remember all the messages.  */
  mlp = message_list_alloc ();

  /* Generate a header, so that we know how and when this PO file was
     created.  */
  if (!omit_header)
      message_list_append (mlp, construct_header ());

  /* Read in the old messages, so that we can add to them.  */
  if (join_existing)
    read_po_file (file_name, mlp);

  /* Process all input files.  */
  for (cnt = 0; cnt < file_list->nitems; ++cnt)
    {
      const char *fname;
      scanner_fp scan_file;

      fname = file_list->item[cnt];

      if (scanner)
        scan_file = scanner;
      else
	{
	  const char *extension;
	  const char *language;

	  /* Work out what the file extension is.  */
	  extension = strrchr (fname, '/');
	  if (!extension)
	    extension = fname;
	  extension = strrchr (extension, '.');
	  if (extension)
	    ++extension;
	  else
	    extension = "";

	  /* derive the language from the extension, and the scanner
	     function from the language.  */
	  language = extension_to_language (extension);
	  if (language == NULL)
	  {
	    error (0, 0, _("\
warning: file `%s' extension `%s' is unknown; will try C"), fname, extension);
	    language = "C";
	  }
	  scan_file = language_to_scanner (language);
	}

      /* Scan the file.  */
      scan_file (fname, mlp);
    }
  string_list_free (file_list);

  /* Sorting the list of messages.  */
  if (sort_by_file)
    message_list_sort_by_filepos (mlp);
  else if (sort_output)
    message_list_sort_by_msgid (mlp);

  /* Write the PO file.  */
  message_list_print (mlp, file_name, force_po, do_debug);

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
Extract translatable string from given input files.\n\
\n\
Mandatory arguments to long options are mandatory for short options too.\n\
  -a, --extract-all              extract all strings\n\
  -c, --add-comments[=TAG]       place comment block with TAG (or those\n\
                                 preceding keyword lines) in output file\n\
  -C, --c++                      shorthand for --language=C++\n\
      --debug                    more detailed formatstring recognision result\n\
  -d, --default-domain=NAME      use NAME.po for output (instead of messages.po)\n\
  -D, --directory=DIRECTORY      add DIRECTORY to list for input files search\n\
  -e, --no-escape                do not use C escapes in output (default)\n\
  -E, --escape                   use C escapes in output, no extended chars\n\
  -f, --files-from=FILE          get list of input files from FILE\n\
      --force-po                 write PO file even if empty\n\
      --foreign-user             omit FSF copyright in output for foreign user\n\
  -F, --sort-by-file             sort output by file location\n"),
	      program_name);
      /* xgettext: no-wrap */
      printf (_("\
  -h, --help                     display this help and exit\n\
  -i, --indent                   write the .po file using indented style\n\
  -j, --join-existing            join messages with existing file\n\
  -k, --keyword[=WORD]           additonal keyword to be looked for (without\n\
                                 WORD means not to use default keywords)\n\
  -l, --string-limit=NUMBER      set string length limit to NUMBER instead %u\n\
  -L, --language=NAME            recognise the specified language (C, C++, PO),\n\
                                 otherwise is guessed from file extension\n\
  -m, --msgstr-prefix[=STRING]   use STRING or \"\" as prefix for msgstr entries\n\
  -M, --msgstr-suffix[=STRING]   use STRING or \"\" as suffix for msgstr entries\n\
      --no-location              do not write '#: filename:line' lines\n"),
	      WARN_ID_LEN);
      /* xgettext: no-wrap */
      fputs (_("\
  -n, --add-location             generate '#: filename:line' lines (default)\n\
      --omit-header              don't write header with `msgid \"\"' entry\n\
  -o, --output=FILE              write output to specified file\n\
  -p, --output-dir=DIR           output files will be placed in directory DIR\n\
  -s, --sort-output              generate sorted output and remove duplicates\n\
      --strict                   write out strict Uniforum conforming .po file\n\
  -T, --trigraphs                understand ANSI C trigraphs for input\n\
  -V, --version                  output version information and exit\n\
  -w, --width=NUMBER             set output page width\n\
  -x, --exclude-file=FILE        entries from FILE are not extracted\n\
\n\
If INPUTFILE is -, standard input is read.\n"), stdout);
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


static void
exclude_directive_domain (pop, name)
     po_ty *pop;
     char *name;
{
  po_gram_error (_("this file may not contain domain directives"));
}


static void
exclude_directive_message (pop, msgid, msgid_pos, msgstr, msgstr_pos)
     po_ty *pop;
     char *msgid;
     lex_pos_ty *msgid_pos;
     char *msgstr;
     lex_pos_ty *msgstr_pos;
{
  message_ty *mp;

  /* See if this message ID has been seen before.  */
  if (exclude == NULL)
    exclude = message_list_alloc ();
  mp = message_list_search (exclude, msgid);
  if (mp != NULL)
    free (msgid);
  else
    {
      mp = message_alloc (msgid);
      /* Do not free msgid.  */
      message_list_append (exclude, mp);
    }

  /* All we care about is the msgid.  Throw the msgstr away.
     Don't even check for duplicate msgids.  */
  free (msgstr);
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static po_method_ty exclude_methods =
{
  sizeof (po_ty),
  NULL, /* constructor */
  NULL, /* destructor */
  exclude_directive_domain,
  exclude_directive_message,
  NULL, /* parse_brief */
  NULL, /* parse_debrief */
  NULL, /* comment */
  NULL, /* comment_dot */
  NULL, /* comment_filepos */
  NULL, /* comment_special */
};


static void
read_exclusion_file (file_name)
     char *file_name;
{
  po_ty *pop;

  pop = po_alloc (&exclude_methods);
  po_scan (pop, file_name);
  po_free (pop);
}


static void
remember_a_message (mlp, tp)
     message_list_ty *mlp;
     xgettext_token_ty *tp;
{
  enum is_c_format is_c_format = undecided;
  enum is_c_format do_wrap = undecided;
  char *msgid;
  message_ty *mp;
  char *msgstr;

  msgid = tp->string;

  /* See whether we shall exclude this message.  */
  if (exclude != NULL && message_list_search (exclude, msgid) != NULL)
    {
      /* Tell the lexer to reset its comment buffer, so that the next
	 message gets the correct comments.  */
      xgettext_lex_comment_reset ();

      return;
    }

  /* See if we have seen this message before.  */
  mp = message_list_search (mlp, msgid);
  if (mp != NULL)
    {
      free (msgid);
      is_c_format = mp->is_c_format;
      do_wrap = mp->do_wrap;
    }
  else
    {
      static lex_pos_ty pos = { __FILE__, __LINE__ };

      /* Allocate a new message and append the message to the list.  */
      mp = message_alloc (msgid);
      /* Do not free msgid.  */
      message_list_append (mlp, mp);

      /* Construct the msgstr from the prefix and suffix, otherwise use the
	 empty string.  */
      if (msgstr_prefix)
	{
	  msgstr = (char *) xmalloc (strlen (msgstr_prefix)
				     + strlen (msgid)
				     + strlen(msgstr_suffix) + 1);
	  stpcpy (stpcpy (stpcpy (msgstr, msgstr_prefix), msgid),
		  msgstr_suffix);
	}
      else
	msgstr = "";
      message_variant_append (mp, MESSAGE_DOMAIN_DEFAULT, msgstr, &pos);
    }

  /* Ask the lexer for the comments it has seen.  Only do this for the
     first instance, otherwise there could be problems; especially if
     the same comment appears before each.  */
  if (!mp->comment_dot)
    {
      int j;

      for (j = 0; ; ++j)
	{
	  const char *s = xgettext_lex_comment (j);
	  if (s == NULL)
	    break;

	  /* To reduce the possibility of unwanted matches be do a two
	     step match: the line must contains `xgettext:' and one of
	     the possible format description strings.  */
	  if (strstr (s, "xgettext:") != NULL)
	    {
	      is_c_format = parse_c_format_description_string (s);
	      do_wrap = parse_c_width_description_string (s);

	      /* If we found a magic string we don't print it.  */
	      if (is_c_format != undecided || do_wrap != undecided)
		continue;
	    }
	  if (add_all_comments
	      || (comment_tag != NULL && strncmp (s, comment_tag,
						  strlen (comment_tag)) == 0))
	    message_comment_dot_append (mp, s);
	}
    }

  /* If not already decided, examine the msgid.  */
  if (is_c_format == undecided)
    is_c_format = test_whether_c_format (mp->msgid);

  mp->is_c_format = is_c_format;
  mp->do_wrap = do_wrap == no ? no : yes;	/* By default we wrap.  */

  /* Remember where we saw this msgid.  */
  if (line_comment)
    message_comment_filepos (mp, tp->file_name, tp->line_number);

  /* Tell the lexer to reset its comment buffer, so that the next
     message gets the correct comments.  */
  xgettext_lex_comment_reset ();
}


static void
scan_c_file(filename, mlp, is_cpp_file)
     const char *filename;
     message_list_ty *mlp;
     int is_cpp_file;
{
  int state;

  /* Inform scanner whether we have C++ files or not.  */
  if (is_cpp_file)
    xgettext_lex_cplusplus ();

  /* The file is broken into tokens.  Scan the token stream, looking for
     a keyword, followed by a left paren, followed by a string.  When we
     see this sequence, we have something to remember.  We assume we are
     looking at a valid C or C++ program, and leave the complaints about
     the grammar to the compiler.  */
  xgettext_lex_open (filename);

  /* Start state is 0.  */
  state = 0;

  while (1)
   {
     xgettext_token_ty token;

     /* A simple state machine is used to do the recognising:
        State 0 = waiting for something to happen
        State 1 = seen one of our keywords with string in first parameter
        State 2 = was in state 1 and now saw a left paren
	State 3 = seen one of our keywords with string in second parameter
	State 4 = was in state 3 and now saw a left paren
	State 5 = waiting for comma after being in state 4
	State 6 = saw comma after being in state 5  */
     xgettext_lex (&token);
     switch (token.type)
       {
       case xgettext_token_type_keyword1:
	 state = 1;
	 continue;

       case xgettext_token_type_keyword2:
	 state = 3;
	 continue;

       case xgettext_token_type_lp:
	 switch (state)
	   {
	   case 1:
	     state = 2;
	     break;
	   case 3:
	     state = 4;
	     break;
	   default:
	     state = 0;
	   }
	 continue;

       case xgettext_token_type_comma:
	 state = state == 5 ? 6 : 0;
	 continue;

       case xgettext_token_type_string_literal:
	 if (extract_all || state == 2 || state == 6)
	   {
	     remember_a_message (mlp, &token);
	     state = 0;
	   }
	 else
	   {
	     free (token.string);
	     state = (state == 4 || state == 5) ? 5 : 0;
	   }
	 continue;

       case xgettext_token_type_symbol:
	 state = (state == 4 || state == 5) ? 5 : 0;
	 continue;

       default:
	 state = 0;
	 continue;

       case xgettext_token_type_eof:
	 break;
       }
     break;
   }

  /* Close scanner.  */
  xgettext_lex_close ();
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

  /* See whether we shall exclude this message.  */
  if (exclude != NULL && message_list_search (exclude, msgid) != NULL)
    goto discard;

  /* If the msgid is the empty string, it is the old header.
     Throw it away, we have constructed a new one.  */
  if (*msgid == '\0')
    {
      discard:
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

  /* Add the accumulated comments to the message.  Clear the
     accumulation in preparation for the next message. */
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
  mp->is_fuzzy = this->is_fuzzy;
  mp->is_c_format = this->is_c_format;
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
  if (mvp != NULL && strcmp (msgstr, mvp->msgstr) != 0)
    {
      gram_error_at_line (msgid_pos, _("duplicate message definition"));
      gram_error_at_line (&mvp->pos, _("\
...this is the location of the first definition"));
      free (msgstr);
    }
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
  this->is_c_format = parse_c_format_description_string (s);
  this->do_wrap = parse_c_width_description_string (s);
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
  po_ty *pop = po_alloc (&extract_methods);
  ((extract_class_ty *) pop)->mlp = mlp;
  po_scan (pop, file_name);
  po_free (pop);
}


#define TM_YEAR_ORIGIN 1900

/* Yield A - B, measured in seconds.  */
static long
difftm (a, b)
     const struct tm *a;
     const struct tm *b;
{
  int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
  int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
  /* Some compilers cannot handle this as a single return statement.  */
  long days = (
               /* difference in day of year  */
               a->tm_yday - b->tm_yday
               /* + intervening leap days  */
               + ((ay >> 2) - (by >> 2))
               - (ay / 100 - by / 100)
               + ((ay / 100 >> 2) - (by / 100 >> 2))
               /* + difference in years * 365  */
               + (long) (ay - by) * 365l);

  return 60l * (60l * (24l * days + (a->tm_hour - b->tm_hour))
                + (a->tm_min - b->tm_min))
         + (a->tm_sec - b->tm_sec);
}


static message_ty *
construct_header ()
{
  time_t now;
  struct tm local_time;
  message_ty *mp;
  char *msgstr;
  static lex_pos_ty pos = { __FILE__, __LINE__, };
  char tz_sign;
  long tz_min;

  mp = message_alloc ("");

  if (foreign_user)
    message_comment_append (mp, "\
SOME DESCRIPTIVE TITLE.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n");
  else
    message_comment_append (mp, "\
SOME DESCRIPTIVE TITLE.\n\
Copyright (C) YEAR Free Software Foundation, Inc.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n");

  mp->is_fuzzy = 1;

  time (&now);
  local_time = *localtime (&now);
  tz_sign = '+';
  tz_min = difftm (&local_time, gmtime (&now)) / 60;
  if (tz_min < 0)
    {
      tz_min = -tz_min;
      tz_sign = '-';
    }

  asprintf (&msgstr, "\
Project-Id-Version: PACKAGE VERSION\n\
POT-Creation-Date: %d-%02d-%02d %02d:%02d%c%02d%02d\n\
PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n\
Last-Translator: FULL NAME <EMAIL@ADDRESS>\n\
Language-Team: LANGUAGE <LL@li.org>\n\
MIME-Version: 1.0\n\
Content-Type: text/plain; charset=CHARSET\n\
Content-Transfer-Encoding: ENCODING\n",
	    local_time.tm_year + TM_YEAR_ORIGIN,
	    local_time.tm_mon + 1,
	    local_time.tm_mday,
	    local_time.tm_hour,
	    local_time.tm_min,
	    tz_sign, tz_min / 60, tz_min % 60);

  if (msgstr == NULL)
    error (EXIT_FAILURE, errno, _("while preparing output"));

  message_variant_append (mp, MESSAGE_DOMAIN_DEFAULT, msgstr, &pos);

  return mp;
}


/* We make a pessimistic guess whether the given string is a format
   string or not.  Pessimistic means here that with the first
   occurence of an unknown format element we say `impossible'.  */
static enum is_c_format
test_whether_c_format (s)
     const char *s;
{
  struct printf_spec spec;

  if (s == NULL || *(s = find_spec (s)) == '\0')
    /* We return `possible' here because sometimes strings are used
       with printf even if they don't contain any format specifier.
       If the translation in this case would contain a specifier, this
       would result in an error.  */
    return impossible;

  for (s = find_spec (s); *s != '\0'; s = spec.next_fmt)
    {
      size_t dummy;

      (void) parse_one_spec (s, 0, &spec, &dummy);
      if (strchr ("iduoxXeEfgGcspnm", spec.info.spec) == NULL)
	return impossible;
    }

  return possible;
}


static void
  scanner_c (filename, mlp)
  const char *filename;
  message_list_ty *mlp;
{
  scan_c_file (filename, mlp, 0);
}


static void
scanner_cxx (filename, mlp)
  const char *filename;
  message_list_ty *mlp;
{
  scan_c_file (filename, mlp, 1);
}


#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))
#define ENDOF(a) ((a) + SIZEOF(a))


static scanner_fp
language_to_scanner (name)
  const char *name;
{
  typedef struct table_ty table_ty;
  struct table_ty
  {
    const char *name;
    scanner_fp func;
  };

  static table_ty table[] =
  {
    { "C", scanner_c, },
    { "C++", scanner_cxx, },
    { "PO", read_po_file, },
    /* Here will follow more languages and their scanners: awk, perl,
       etc...  Make sure new scanners honor the --exlude-file option.  */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
  {
    if (strcasecmp(name, tp->name) == 0)
      return tp->func;
  }
  error (EXIT_FAILURE, 0, _("language `%s' unknown"), name);
  /* NOTREACHED */
  return NULL;
}


static const char *
extension_to_language (extension)
  const char *extension;
{
  typedef struct table_ty table_ty;
  struct table_ty
  {
    const char *extension;
    const char *language;
  };

  static table_ty table[] =
  {
    { "c",      "C",    },
    { "C",      "C++",  },
    { "c++",    "C++",  },
    { "cc",     "C++",  },
    { "cxx",    "C++",  },
    { "h",      "C",    },
    { "po",     "PO",   },
    { "pot",    "PO",   },
    { "pox",    "PO",   },
    /* Here will follow more file extensions: sh, pl, tcl ... */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
  {
    if (strcmp(extension, tp->extension) == 0)
    return tp->language;
  }
  return NULL;
}
