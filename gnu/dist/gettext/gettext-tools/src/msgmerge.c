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
#include <alloca.h>

#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "closeout.h"
#include "dir-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "message.h"
#include "read-po.h"
#include "write-po.h"
#include "format.h"
#include "xalloc.h"
#include "obstack.h"
#include "strstr.h"
#include "exit.h"
#include "c-strcase.h"
#include "stpcpy.h"
#include "stpncpy.h"
#include "msgl-iconv.h"
#include "msgl-equal.h"
#include "plural-count.h"
#include "backupfile.h"
#include "copy-file.h"
#include "gettext.h"

#define _(str) gettext (str)

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free


/* If true do not print unneeded messages.  */
static bool quiet;

/* Verbosity level.  */
static int verbosity_level;

/* Force output of PO file even if empty.  */
static int force_po;

/* Apply the .pot file to each of the domains in the PO file.  */
static bool multi_domain_mode = false;

/* Determines whether to use fuzzy matching.  */
static bool use_fuzzy_matching = true;

/* List of user-specified compendiums.  */
static message_list_list_ty *compendiums;

/* Update mode.  */
static bool update_mode = false;
static const char *version_control_string;
static const char *backup_suffix_string;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-location", no_argument, &line_comment, 1 },
  { "backup", required_argument, NULL, CHAR_MAX + 1 },
  { "compendium", required_argument, NULL, 'C', },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "force-po", no_argument, &force_po, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "multi-domain", no_argument, NULL, 'm' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-fuzzy-matching", no_argument, NULL, 'N' },
  { "no-location", no_argument, &line_comment, 0 },
  { "no-wrap", no_argument, NULL, CHAR_MAX + 4 },
  { "output-file", required_argument, NULL, 'o' },
  { "properties-input", no_argument, NULL, 'P' },
  { "properties-output", no_argument, NULL, 'p' },
  { "quiet", no_argument, NULL, 'q' },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "silent", no_argument, NULL, 'q' },
  { "strict", no_argument, NULL, CHAR_MAX + 2 },
  { "stringtable-input", no_argument, NULL, CHAR_MAX + 5 },
  { "stringtable-output", no_argument, NULL, CHAR_MAX + 6 },
  { "suffix", required_argument, NULL, CHAR_MAX + 3 },
  { "update", no_argument, NULL, 'U' },
  { "verbose", no_argument, NULL, 'v' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { NULL, 0, NULL, 0 }
};


struct statistics
{
  size_t merged;
  size_t fuzzied;
  size_t missing;
  size_t obsolete;
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;
static void compendium (const char *filename);
static msgdomain_list_ty *merge (const char *fn1, const char *fn2,
				 msgdomain_list_ty **defp);


int
main (int argc, char **argv)
{
  int opt;
  bool do_help;
  bool do_version;
  char *output_file;
  msgdomain_list_ty *def;
  msgdomain_list_ty *result;
  bool sort_by_filepos = false;
  bool sort_by_msgid = false;

  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  verbosity_level = 0;
  quiet = false;
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

  /* Set default values for variables.  */
  do_help = false;
  do_version = false;
  output_file = NULL;

  while ((opt = getopt_long (argc, argv, "C:D:eEFhimNo:pPqsUvVw:",
			     long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case '\0':		/* Long option.  */
	break;

      case 'C':
	compendium (optarg);
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

      case 'F':
	sort_by_filepos = true;
	break;

      case 'h':
	do_help = true;
	break;

      case 'i':
	message_print_style_indent ();
	break;

      case 'm':
	multi_domain_mode = true;
	break;

      case 'N':
	use_fuzzy_matching = false;
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

      case 'q':
	quiet = true;
	break;

      case 's':
	sort_by_msgid = true;
	break;

      case 'U':
	update_mode = true;
	break;

      case 'v':
	++verbosity_level;
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

      case CHAR_MAX + 1: /* --backup */
	version_control_string = optarg;
	break;

      case CHAR_MAX + 2: /* --strict */
	message_print_style_uniforum ();
	break;

      case CHAR_MAX + 3: /* --suffix */
	backup_suffix_string = optarg;
	break;

      case CHAR_MAX + 4: /* --no-wrap */
	message_page_width_ignore ();
	break;

      case CHAR_MAX + 5: /* --stringtable-input */
	input_syntax = syntax_stringtable;
	break;

      case CHAR_MAX + 6: /* --stringtable-output */
	message_print_syntax_stringtable ();
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

  /* Verify selected options.  */
  if (update_mode)
    {
      if (output_file != NULL)
	{
	  error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
		 "--update", "--output-file");
	}
    }
  else
    {
      if (version_control_string != NULL)
	{
	  error (EXIT_SUCCESS, 0, _("%s is only valid with %s"),
		 "--backup", "--update");
	  usage (EXIT_FAILURE);
	}
      if (backup_suffix_string != NULL)
	{
	  error (EXIT_SUCCESS, 0, _("%s is only valid with %s"),
		 "--suffix", "--update");
	  usage (EXIT_FAILURE);
	}
    }

  if (!line_comment && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--no-location", "--sort-by-file");

  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	   "--sort-output", "--sort-by-file");

  /* In update mode, --properties-input implies --properties-output.  */
  if (update_mode && input_syntax == syntax_properties)
    message_print_syntax_properties ();
  /* In update mode, --stringtable-input implies --stringtable-output.  */
  if (update_mode && input_syntax == syntax_stringtable)
    message_print_syntax_stringtable ();

  /* Merge the two files.  */
  result = merge (argv[optind], argv[optind + 1], &def);

  /* Sort the results.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (result);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (result);

  if (update_mode)
    {
      /* Do nothing if the original file and the result are equal.  Also do
	 nothing if the original file and the result differ only by the
	 POT-Creation-Date in the header entry; this is needed for projects
	 which don't put the .pot file under CVS.  */
      if (!msgdomain_list_equal (def, result, true))
	{
	  /* Back up def.po.  */
	  enum backup_type backup_type;
	  char *backup_file;

	  output_file = argv[optind];

	  if (backup_suffix_string == NULL)
	    {
	      backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");
	      if (backup_suffix_string != NULL
		  && backup_suffix_string[0] == '\0')
		backup_suffix_string = NULL;
	    }
	  if (backup_suffix_string != NULL)
	    simple_backup_suffix = backup_suffix_string;

	  backup_type = xget_version (_("backup type"), version_control_string);
	  if (backup_type != none)
	    {
	      backup_file = find_backup_file_name (output_file, backup_type);
	      copy_file_preserving (output_file, backup_file);
	    }

	  /* Write the merged message list out.  */
	  msgdomain_list_print (result, output_file, true, false);
	}
    }
  else
    {
      /* Write the merged message list out.  */
      msgdomain_list_print (result, output_file, force_po, false);
    }

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
Merges two Uniforum style .po files together.  The def.po file is an\n\
existing PO file with translations which will be taken over to the newly\n\
created file as long as they still match; comments will be preserved,\n\
but extracted comments and file positions will be discarded.  The ref.pot\n\
file is the last created PO file with up-to-date source references but\n\
old translations, or a PO Template file (generally created by xgettext);\n\
any translations or comments in the file will be discarded, however dot\n\
comments and file positions will be preserved.  Where an exact match\n\
cannot be found, fuzzy matching is used to produce better results.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  def.po                      translations referring to old sources\n"));
      printf (_("\
  ref.pot                     references to new sources\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
  -C, --compendium=FILE       additional library of message translations,\n\
                              may be specified more than once\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -U, --update                update def.po,\n\
                              do nothing if def.po already up to date\n"));
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
Output file location in update mode:\n"));
      printf (_("\
The result is written back to def.po.\n"));
      printf (_("\
      --backup=CONTROL        make a backup of def.po\n"));
      printf (_("\
      --suffix=SUFFIX         override the usual backup suffix\n"));
      printf (_("\
The version control method may be selected via the --backup option or through\n\
the VERSION_CONTROL environment variable.  Here are the values:\n\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n"));
      printf (_("\
The backup suffix is `~', unless set with --suffix or the SIMPLE_BACKUP_SUFFIX\n\
environment variable.\n\
"));
      printf ("\n");
      printf (_("\
Operation modifiers:\n"));
      printf (_("\
  -m, --multi-domain          apply ref.pot to each of the domains in def.po\n"));
      printf (_("\
  -N, --no-fuzzy-matching     do not use fuzzy matching\n"));
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
  -i, --indent                indented output style\n"));
      printf (_("\
      --no-location           suppress '#: filename:line' lines\n"));
      printf (_("\
      --add-location          preserve '#: filename:line' lines (default)\n"));
      printf (_("\
      --strict                strict Uniforum output style\n"));
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
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf (_("\
  -v, --verbose               increase verbosity level\n"));
      printf (_("\
  -q, --quiet, --silent       suppress progress indicators\n"));
      printf ("\n");
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"),
	     stdout);
    }

  exit (status);
}


static void
compendium (const char *filename)
{
  msgdomain_list_ty *mdlp;
  size_t k;

  mdlp = read_po_file (filename);
  if (!compendiums)
    compendiums = message_list_list_alloc ();
  for (k = 0; k < mdlp->nitems; k++)
    message_list_list_append (compendiums, mdlp->item[k]->messages);
}


static bool
msgfmt_check_pair_fails (const lex_pos_ty *pos,
			 const char *msgid, const char *msgid_plural,
			 const char *msgstr, size_t msgstr_len,
			 size_t fmt)
{
  bool failure;
  struct formatstring_parser *parser = formatstring_parsers[fmt];
  char *invalid_reason = NULL;
  void *msgid_descr =
    parser->parse (msgid_plural != NULL ? msgid_plural : msgid, false,
		   &invalid_reason);

  failure = false;
  if (msgid_descr != NULL)
    {
      const char *p_end = msgstr + msgstr_len;
      const char *p;

      for (p = msgstr; p < p_end; p += strlen (p) + 1)
	{
	  void *msgstr_descr = parser->parse (msgstr, true, &invalid_reason);

	  if (msgstr_descr != NULL)
	    {
	      failure = parser->check (msgid_descr, msgstr_descr,
				       msgid_plural == NULL, NULL, NULL);
	      parser->free (msgstr_descr);
	    }
	  else
	    {
	      failure = true;
	      free (invalid_reason);
	    }

	  if (failure)
	    break;
	}

      parser->free (msgid_descr);
    }
  else
    free (invalid_reason);

  return failure;
}


static message_ty *
message_merge (message_ty *def, message_ty *ref)
{
  const char *msgstr;
  size_t msgstr_len;
  message_ty *result;
  size_t j, i;

  /* Take the msgid from the reference.  When fuzzy matches are made,
     the definition will not be unique, but the reference will be -
     usually because it has only been slightly changed.  */

  /* Take the msgstr from the definition.  The msgstr of the reference
     is usually empty, as it was generated by xgettext.  If we currently
     process the header entry we have to merge the msgstr by using the
     Report-Msgid-Bugs-To and POT-Creation-Date fields from the reference.  */
  if (ref->msgid[0] == '\0')
    {
      /* Oh, oh.  The header entry and we have something to fill in.  */
      static const struct
      {
	const char *name;
	size_t len;
      } known_fields[] =
      {
	{ "Project-Id-Version:", sizeof ("Project-Id-Version:") - 1 },
#define PROJECT_ID		0
	{ "Report-Msgid-Bugs-To:", sizeof ("Report-Msgid-Bugs-To:") - 1 },
#define REPORT_MSGID_BUGS_TO	1
	{ "POT-Creation-Date:", sizeof ("POT-Creation-Date:") - 1 },
#define POT_CREATION_DATE	2
	{ "PO-Revision-Date:", sizeof ("PO-Revision-Date:") - 1 },
#define PO_REVISION_DATE	3
	{ "Last-Translator:", sizeof ("Last-Translator:") - 1 },
#define LAST_TRANSLATOR		4
	{ "Language-Team:", sizeof ("Language-Team:") - 1 },
#define LANGUAGE_TEAM		5
	{ "MIME-Version:", sizeof ("MIME-Version:") - 1 },
#define MIME_VERSION		6
	{ "Content-Type:", sizeof ("Content-Type:") - 1 },
#define CONTENT_TYPE		7
	{ "Content-Transfer-Encoding:",
	  sizeof ("Content-Transfer-Encoding:") - 1 }
#define CONTENT_TRANSFER	8
      };
#define UNKNOWN	9
      struct
      {
	const char *string;
	size_t len;
      } header_fields[UNKNOWN + 1];
      struct obstack pool;
      const char *cp;
      char *newp;
      size_t len, cnt;

      /* Clear all fields.  */
      memset (header_fields, '\0', sizeof (header_fields));

      /* Prepare a temporary memory pool.  */
      obstack_init (&pool);

      cp = def->msgstr;
      while (*cp != '\0')
	{
	  const char *endp = strchr (cp, '\n');
	  int terminated = endp != NULL;

	  if (!terminated)
	    {
	      /* Add a trailing newline.  */
	      char *copy;
	      endp = strchr (cp, '\0');

	      len = endp - cp + 1;

	      copy = (char *) obstack_alloc (&pool, len + 1);
	      stpcpy (stpcpy (copy, cp), "\n");
	      cp = copy;
	    }
	  else
	    {
	      len = (endp - cp) + 1;
	      ++endp;
	    }

	  /* Compare with any of the known fields.  */
	  for (cnt = 0;
	       cnt < sizeof (known_fields) / sizeof (known_fields[0]);
	       ++cnt)
	    if (c_strncasecmp (cp, known_fields[cnt].name, known_fields[cnt].len)
		== 0)
	      break;

	  if (cnt < sizeof (known_fields) / sizeof (known_fields[0]))
	    {
	      header_fields[cnt].string = &cp[known_fields[cnt].len];
	      header_fields[cnt].len = len - known_fields[cnt].len;
	    }
	  else
	    {
	      /* It's an unknown field.  Append content to what is already
		 known.  */
	      char *extended =
		(char *) obstack_alloc (&pool,
					header_fields[UNKNOWN].len + len + 1);
	      memcpy (extended, header_fields[UNKNOWN].string,
		      header_fields[UNKNOWN].len);
	      memcpy (&extended[header_fields[UNKNOWN].len], cp, len);
	      extended[header_fields[UNKNOWN].len + len] = '\0';
	      header_fields[UNKNOWN].string = extended;
	      header_fields[UNKNOWN].len += len;
	    }

	  cp = endp;
	}

      {
	const char *msgid_bugs_ptr;

	msgid_bugs_ptr = strstr (ref->msgstr, "Report-Msgid-Bugs-To:");
	if (msgid_bugs_ptr != NULL)
	  {
	    size_t msgid_bugs_len;
	    const char *endp;

	    msgid_bugs_ptr += sizeof ("Report-Msgid-Bugs-To:") - 1;

	    endp = strchr (msgid_bugs_ptr, '\n');
	    if (endp == NULL)
	      {
		/* Add a trailing newline.  */
		char *extended;
		endp = strchr (msgid_bugs_ptr, '\0');
		msgid_bugs_len = (endp - msgid_bugs_ptr) + 1;
		extended = (char *) obstack_alloc (&pool, msgid_bugs_len + 1);
		stpcpy (stpcpy (extended, msgid_bugs_ptr), "\n");
		msgid_bugs_ptr = extended;
	      }
	    else
	      msgid_bugs_len = (endp - msgid_bugs_ptr) + 1;

	    header_fields[REPORT_MSGID_BUGS_TO].string = msgid_bugs_ptr;
	    header_fields[REPORT_MSGID_BUGS_TO].len = msgid_bugs_len;
	  }
      }

      {
	const char *pot_date_ptr;

	pot_date_ptr = strstr (ref->msgstr, "POT-Creation-Date:");
	if (pot_date_ptr != NULL)
	  {
	    size_t pot_date_len;
	    const char *endp;

	    pot_date_ptr += sizeof ("POT-Creation-Date:") - 1;

	    endp = strchr (pot_date_ptr, '\n');
	    if (endp == NULL)
	      {
		/* Add a trailing newline.  */
		char *extended;
		endp = strchr (pot_date_ptr, '\0');
		pot_date_len = (endp - pot_date_ptr) + 1;
		extended = (char *) obstack_alloc (&pool, pot_date_len + 1);
		stpcpy (stpcpy (extended, pot_date_ptr), "\n");
		pot_date_ptr = extended;
	      }
	    else
	      pot_date_len = (endp - pot_date_ptr) + 1;

	    header_fields[POT_CREATION_DATE].string = pot_date_ptr;
	    header_fields[POT_CREATION_DATE].len = pot_date_len;
	  }
      }

      /* Concatenate all the various fields.  */
      len = 0;
      for (cnt = 0; cnt < UNKNOWN; ++cnt)
	if (header_fields[cnt].string != NULL)
	  len += known_fields[cnt].len + header_fields[cnt].len;
      len += header_fields[UNKNOWN].len;

      cp = newp = (char *) xmalloc (len + 1);
      newp[len] = '\0';

#define IF_FILLED(idx)							      \
      if (header_fields[idx].string)					      \
	newp = stpncpy (stpcpy (newp, known_fields[idx].name),		      \
			header_fields[idx].string, header_fields[idx].len)

      IF_FILLED (PROJECT_ID);
      IF_FILLED (REPORT_MSGID_BUGS_TO);
      IF_FILLED (POT_CREATION_DATE);
      IF_FILLED (PO_REVISION_DATE);
      IF_FILLED (LAST_TRANSLATOR);
      IF_FILLED (LANGUAGE_TEAM);
      IF_FILLED (MIME_VERSION);
      IF_FILLED (CONTENT_TYPE);
      IF_FILLED (CONTENT_TRANSFER);
      if (header_fields[UNKNOWN].string != NULL)
	stpcpy (newp, header_fields[UNKNOWN].string);

#undef IF_FILLED

      /* Free the temporary memory pool.  */
      obstack_free (&pool, NULL);

      msgstr = cp;
      msgstr_len = strlen (cp) + 1;
    }
  else
    {
      msgstr = def->msgstr;
      msgstr_len = def->msgstr_len;
    }

  result = message_alloc (xstrdup (ref->msgid), ref->msgid_plural,
			  msgstr, msgstr_len, &def->pos);

  /* Take the comments from the definition file.  There will be none at
     all in the reference file, as it was generated by xgettext.  */
  if (def->comment)
    for (j = 0; j < def->comment->nitems; ++j)
      message_comment_append (result, def->comment->item[j]);

  /* Take the dot comments from the reference file, as they are
     generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  if (ref->comment_dot)
    for (j = 0; j < ref->comment_dot->nitems; ++j)
      message_comment_dot_append (result, ref->comment_dot->item[j]);

  /* The flags are mixed in a special way.  Some informations come
     from the reference message (such as format/no-format), others
     come from the definition file (fuzzy or not).  */
  result->is_fuzzy = def->is_fuzzy;

  for (i = 0; i < NFORMATS; i++)
    {
      result->is_format[i] = ref->is_format[i];

      /* If the reference message is marked as being a format specifier,
	 but the definition message is not, we check if the resulting
	 message would pass "msgfmt -c".  If yes, then all is fine.  If
	 not, we add a fuzzy marker, because
	 1. the message needs the translator's attention,
	 2. msgmerge must not transform a PO file which passes "msgfmt -c"
	    into a PO file which doesn't.  */
      if (!result->is_fuzzy
	  && possible_format_p (ref->is_format[i])
	  && !possible_format_p (def->is_format[i])
	  && msgfmt_check_pair_fails (&def->pos, ref->msgid, ref->msgid_plural,
				      msgstr, msgstr_len, i))
	result->is_fuzzy = true;
    }

  result->do_wrap = ref->do_wrap;

  /* Take the file position comments from the reference file, as they
     are generated by xgettext.  Any in the definition file are old ones
     collected by previous runs of xgettext and msgmerge.  */
  for (j = 0; j < ref->filepos_count; ++j)
    {
      lex_pos_ty *pp = &ref->filepos[j];
      message_comment_filepos (result, pp->file_name, pp->line_number);
    }

  /* Special postprocessing is needed if the reference message is a
     plural form and the definition message isn't, or vice versa.  */
  if (ref->msgid_plural != NULL)
    {
      if (def->msgid_plural == NULL)
	result->used = 1;
    }
  else
    {
      if (def->msgid_plural != NULL)
	result->used = 2;
    }

  /* All done, return the merged message to the caller.  */
  return result;
}


#define DOT_FREQUENCY 10

static void
match_domain (const char *fn1, const char *fn2,
	      message_list_list_ty *definitions, message_list_ty *refmlp,
	      message_list_ty *resultmlp,
	      struct statistics *stats, unsigned int *processed)
{
  message_ty *header_entry;
  unsigned long int nplurals;
  char *untranslated_plural_msgstr;
  size_t j;

  header_entry = message_list_search (definitions->item[0], "");
  nplurals = get_plural_count (header_entry ? header_entry->msgstr : NULL);
  untranslated_plural_msgstr = (char *) xmalloc (nplurals);
  memset (untranslated_plural_msgstr, '\0', nplurals);

  for (j = 0; j < refmlp->nitems; j++, (*processed)++)
    {
      message_ty *refmsg;
      message_ty *defmsg;

      /* Because merging can take a while we print something to signal
	 we are not dead.  */
      if (!quiet && verbosity_level <= 1 && *processed % DOT_FREQUENCY == 0)
	fputc ('.', stderr);

      refmsg = refmlp->item[j];

      /* See if it is in the other file.  */
      defmsg = message_list_list_search (definitions, refmsg->msgid);
      if (defmsg)
	{
	  /* Merge the reference with the definition: take the #. and
	     #: comments from the reference, take the # comments from
	     the definition, take the msgstr from the definition.  Add
	     this merged entry to the output message list.  */
	  message_ty *mp = message_merge (defmsg, refmsg);

	  message_list_append (resultmlp, mp);

	  /* Remember that this message has been used, when we scan
	     later to see if anything was omitted.  */
	  defmsg->used = 1;
	  stats->merged++;
	}
      else if (refmsg->msgid[0] != '\0')
	{
	  /* If the message was not defined at all, try to find a very
	     similar message, it could be a typo, or the suggestion may
	     help.  */
	  if (use_fuzzy_matching
	      && ((defmsg =
		     message_list_list_search_fuzzy (definitions,
						     refmsg->msgid)) != NULL))
	    {
	      message_ty *mp;

	      if (verbosity_level > 1)
		{
		  po_gram_error_at_line (&refmsg->pos, _("\
this message is used but not defined..."));
		  po_gram_error_at_line (&defmsg->pos, _("\
...but this definition is similar"));
		}

	      /* Merge the reference with the definition: take the #. and
		 #: comments from the reference, take the # comments from
		 the definition, take the msgstr from the definition.  Add
		 this merged entry to the output message list.  */
	      mp = message_merge (defmsg, refmsg);

	      mp->is_fuzzy = true;

	      message_list_append (resultmlp, mp);

	      /* Remember that this message has been used, when we scan
		 later to see if anything was omitted.  */
	      defmsg->used = 1;
	      stats->fuzzied++;
	      if (!quiet && verbosity_level <= 1)
		/* Always print a dot if we handled a fuzzy match.  */
		fputc ('.', stderr);
	    }
	  else
	    {
	      message_ty *mp;
	      bool is_untranslated;
	      const char *p;
	      const char *pend;

	      if (verbosity_level > 1)
		po_gram_error_at_line (&refmsg->pos, _("\
this message is used but not defined in %s"), fn1);

	      mp = message_copy (refmsg);

	      if (mp->msgid_plural != NULL)
		{
		  /* Test if mp is untranslated.  (It most likely is.)  */
		  is_untranslated = true;
		  for (p = mp->msgstr, pend = p + mp->msgstr_len; p < pend; p++)
		    if (*p != '\0')
		      {
			is_untranslated = false;
			break;
		      }
		  if (is_untranslated)
		    {
		      /* Change mp->msgstr_len consecutive empty strings into
			 nplurals consecutive empty strings.  */
		      if (nplurals > mp->msgstr_len)
			mp->msgstr = untranslated_plural_msgstr;
		      mp->msgstr_len = nplurals;
		    }
		}

	      message_list_append (resultmlp, mp);
	      stats->missing++;
	    }
	}
    }

  /* Now postprocess the problematic merges.  This is needed because we
     want the result to pass the "msgfmt -c -v" check.  */
  {
    /* message_merge sets mp->used to 1 or 2, depending on the problem.
       Compute the bitwise OR of all these.  */
    int problematic = 0;

    for (j = 0; j < resultmlp->nitems; j++)
      problematic |= resultmlp->item[j]->used;

    if (problematic)
      {
	unsigned long int nplurals = 0;

	if (problematic & 1)
	  {
	    /* Need to know nplurals of the result domain.  */
	    message_ty *header_entry = message_list_search (resultmlp, "");

	    nplurals = get_plural_count (header_entry
					 ? header_entry->msgstr
					 : NULL);
	  }

	for (j = 0; j < resultmlp->nitems; j++)
	  {
	    message_ty *mp = resultmlp->item[j];

	    if ((mp->used & 1) && (nplurals > 0))
	      {
		/* ref->msgid_plural != NULL but def->msgid_plural == NULL.
		   Use a copy of def->msgstr for each possible plural form.  */
		size_t new_msgstr_len;
		char *new_msgstr;
		char *p;
		unsigned long i;

		if (verbosity_level > 1)
		  {
		    po_gram_error_at_line (&mp->pos, _("\
this message should define plural forms"));
		  }

		new_msgstr_len = nplurals * mp->msgstr_len;
		new_msgstr = (char *) xmalloc (new_msgstr_len);
		for (i = 0, p = new_msgstr; i < nplurals; i++)
		  {
		    memcpy (p, mp->msgstr, mp->msgstr_len);
		    p += mp->msgstr_len;
		  }
		mp->msgstr = new_msgstr;
		mp->msgstr_len = new_msgstr_len;
		mp->is_fuzzy = true;
	      }

	    if ((mp->used & 2) && (mp->msgstr_len > strlen (mp->msgstr) + 1))
	      {
		/* ref->msgid_plural == NULL but def->msgid_plural != NULL.
		   Use only the first among the plural forms.  */

		if (verbosity_level > 1)
		  {
		    po_gram_error_at_line (&mp->pos, _("\
this message should not define plural forms"));
		  }

		mp->msgstr_len = strlen (mp->msgstr) + 1;
		mp->is_fuzzy = true;
	      }

	    /* Postprocessing of this message is done.  */
	    mp->used = 0;
	  }
      }
  }
}

static msgdomain_list_ty *
merge (const char *fn1, const char *fn2, msgdomain_list_ty **defp)
{
  msgdomain_list_ty *def;
  msgdomain_list_ty *ref;
  size_t j, k;
  unsigned int processed;
  struct statistics stats;
  msgdomain_list_ty *result;
  message_list_list_ty *definitions;
  message_list_ty *empty_list;

  stats.merged = stats.fuzzied = stats.missing = stats.obsolete = 0;

  /* This is the definitions file, created by a human.  */
  def = read_po_file (fn1);

  /* Create the set of places to look for message definitions: a list
     whose first element will be definitions for the current domain, and
     whose other elements come from the compendiums.  */
  definitions = message_list_list_alloc ();
  message_list_list_append (definitions, NULL);
  if (compendiums)
    message_list_list_append_list (definitions, compendiums);
  empty_list = message_list_alloc (false);

  /* This is the references file, created by groping the sources with
     the xgettext program.  */
  ref = read_po_file (fn2);
  /* Add a dummy header entry, if the references file contains none.  */
  for (k = 0; k < ref->nitems; k++)
    if (message_list_search (ref->item[k]->messages, "") == NULL)
      {
	static lex_pos_ty pos = { __FILE__, __LINE__ };
	message_ty *refheader = message_alloc ("", NULL, "", 1, &pos);

	message_list_prepend (ref->item[k]->messages, refheader);
      }

  /* The references file can be either in ASCII or in UTF-8.  If it is
     in UTF-8, we have to convert the definitions to UTF-8 as well.  */
  {
    bool was_utf8 = false;
    for (k = 0; k < ref->nitems; k++)
      {
	message_list_ty *mlp = ref->item[k]->messages;

	for (j = 0; j < mlp->nitems; j++)
	  if (mlp->item[j]->msgid[0] == '\0' && !mlp->item[j]->obsolete)
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

  result = msgdomain_list_alloc (false);
  processed = 0;

  /* Every reference must be matched with its definition. */
  if (!multi_domain_mode)
    for (k = 0; k < ref->nitems; k++)
      {
	const char *domain = ref->item[k]->domain;
	message_list_ty *refmlp = ref->item[k]->messages;
	message_list_ty *resultmlp =
	  msgdomain_list_sublist (result, domain, true);

	definitions->item[0] = msgdomain_list_sublist (def, domain, false);
	if (definitions->item[0] == NULL)
	  definitions->item[0] = empty_list;

	match_domain (fn1, fn2, definitions, refmlp, resultmlp,
		      &stats, &processed);
      }
  else
    {
      /* Apply the references messages in the default domain to each of
	 the definition domains.  */
      message_list_ty *refmlp = ref->item[0]->messages;

      for (k = 0; k < def->nitems; k++)
	{
	  const char *domain = def->item[k]->domain;
	  message_list_ty *defmlp = def->item[k]->messages;

	  /* Ignore the default message domain if it has no messages.  */
	  if (k > 0 || defmlp->nitems > 0)
	    {
	      message_list_ty *resultmlp =
		msgdomain_list_sublist (result, domain, true);

	      definitions->item[0] = defmlp;

	      match_domain (fn1, fn2, definitions, refmlp, resultmlp,
			    &stats, &processed);
	    }
	}
    }

  /* Look for messages in the definition file, which are not present
     in the reference file, indicating messages which defined but not
     used in the program.  Don't scan the compendium(s).  */
  for (k = 0; k < def->nitems; ++k)
    {
      const char *domain = def->item[k]->domain;
      message_list_ty *defmlp = def->item[k]->messages;

      for (j = 0; j < defmlp->nitems; j++)
	{
	  message_ty *defmsg = defmlp->item[j];

	  if (!defmsg->used)
	    {
	      /* Remember the old translation although it is not used anymore.
		 But we mark it as obsolete.  */
	      message_ty *mp;

	      mp = message_copy (defmsg);
	      mp->obsolete = true;

	      message_list_append (msgdomain_list_sublist (result, domain, true),
				   mp);
	      stats.obsolete++;
	    }
	}
    }

  /* Determine the known a-priori encoding, if any.  */
  if (def->encoding == ref->encoding)
    result->encoding = def->encoding;

  /* Report some statistics.  */
  if (verbosity_level > 0)
    fprintf (stderr, _("%s\
Read %ld old + %ld reference, \
merged %ld, fuzzied %ld, missing %ld, obsolete %ld.\n"),
	     !quiet && verbosity_level <= 1 ? "\n" : "",
	     (long) def->nitems, (long) ref->nitems,
	     (long) stats.merged, (long) stats.fuzzied, (long) stats.missing,
	     (long) stats.obsolete);
  else if (!quiet)
    fputs (_(" done.\n"), stderr);

  /* Return results.  */
  *defp = def;
  return result;
}
