/* Extracts strings from C source file to Uniforum style .po file.
   Copyright (C) 1995-1998, 2000-2005 Free Software Foundation, Inc.
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
#include <alloca.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <limits.h>

#include "xgettext.h"
#include "closeout.h"
#include "dir-list.h"
#include "file-list.h"
#include "str-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "xerror.h"
#include "xalloc.h"
#include "xallocsa.h"
#include "strstr.h"
#include "xerror.h"
#include "exit.h"
#include "pathname.h"
#include "c-strcase.h"
#include "stpcpy.h"
#include "open-po.h"
#include "read-po-abstract.h"
#include "message.h"
#include "po-charset.h"
#include "msgl-iconv.h"
#include "msgl-ascii.h"
#include "po-time.h"
#include "write-po.h"
#include "format.h"
#include "gettext.h"

/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


#ifdef __cplusplus
extern "C" {
#endif

#include "x-c.h"
#include "x-po.h"
#include "x-sh.h"
#include "x-python.h"
#include "x-lisp.h"
#include "x-elisp.h"
#include "x-librep.h"
#include "x-scheme.h"
#include "x-smalltalk.h"
#include "x-java.h"
#include "x-properties.h"
#include "x-csharp.h"
#include "x-awk.h"
#include "x-ycp.h"
#include "x-tcl.h"
#include "x-perl.h"
#include "x-php.h"
#include "x-stringtable.h"
#include "x-rst.h"
#include "x-glade.h"

#ifdef __cplusplus
}
#endif


/* If nonzero add all comments immediately preceding one of the keywords. */
static bool add_all_comments = false;

/* Tag used in comment of prevailing domain.  */
static char *comment_tag;

/* Name of default domain file.  If not set defaults to messages.po.  */
static const char *default_domain;

/* If called with --debug option the output reflects whether format
   string recognition is done automatically or forced by the user.  */
static int do_debug;

/* Content of .po files with symbols to be excluded.  */
message_list_ty *exclude;

/* Force output of PO file even if empty.  */
static int force_po;

/* Copyright holder of the output file and the translations.  */
static const char *copyright_holder = "THE PACKAGE'S COPYRIGHT HOLDER";

/* Email address or URL for reports of bugs in msgids.  */
static const char *msgid_bugs_address = NULL;

/* String used as prefix for msgstr.  */
static const char *msgstr_prefix;

/* String used as suffix for msgstr.  */
static const char *msgstr_suffix;

/* Directory in which output files are created.  */
static char *output_dir;

/* The output syntax: .pot or .properties or .strings.  */
static input_syntax_ty output_syntax = syntax_po;

/* If nonzero omit header with information about this run.  */
int xgettext_omit_header;

/* Table of flag_context_list_ty tables.  */
static flag_context_list_table_ty flag_table_c;
static flag_context_list_table_ty flag_table_objc;
static flag_context_list_table_ty flag_table_gcc_internal;
static flag_context_list_table_ty flag_table_sh;
static flag_context_list_table_ty flag_table_python;
static flag_context_list_table_ty flag_table_lisp;
static flag_context_list_table_ty flag_table_elisp;
static flag_context_list_table_ty flag_table_librep;
static flag_context_list_table_ty flag_table_scheme;
static flag_context_list_table_ty flag_table_java;
static flag_context_list_table_ty flag_table_csharp;
static flag_context_list_table_ty flag_table_awk;
static flag_context_list_table_ty flag_table_ycp;
static flag_context_list_table_ty flag_table_tcl;
static flag_context_list_table_ty flag_table_perl;
static flag_context_list_table_ty flag_table_php;

/* If true, recognize Qt format strings.  */
static bool recognize_format_qt;

/* Canonicalized encoding name for all input files.  */
const char *xgettext_global_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_global_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
iconv_t xgettext_global_source_iconv;
#endif

/* Canonicalized encoding name for the current input file.  */
const char *xgettext_current_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_current_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
iconv_t xgettext_current_source_iconv;
#endif

/* Long options.  */
static const struct option long_options[] =
{
  { "add-comments", optional_argument, NULL, 'c' },
  { "add-location", no_argument, &line_comment, 1 },
  { "c++", no_argument, NULL, 'C' },
  { "copyright-holder", required_argument, NULL, CHAR_MAX + 1 },
  { "debug", no_argument, &do_debug, 1 },
  { "default-domain", required_argument, NULL, 'd' },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "exclude-file", required_argument, NULL, 'x' },
  { "extract-all", no_argument, NULL, 'a' },
  { "files-from", required_argument, NULL, 'f' },
  { "flag", required_argument, NULL, CHAR_MAX + 8 },
  { "force-po", no_argument, &force_po, 1 },
  { "foreign-user", no_argument, NULL, CHAR_MAX + 2 },
  { "from-code", required_argument, NULL, CHAR_MAX + 3 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "join-existing", no_argument, NULL, 'j' },
  { "keyword", optional_argument, NULL, 'k' },
  { "language", required_argument, NULL, 'L' },
  { "msgid-bugs-address", required_argument, NULL, CHAR_MAX + 5 },
  { "msgstr-prefix", optional_argument, NULL, 'm' },
  { "msgstr-suffix", optional_argument, NULL, 'M' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, &line_comment, 0 },
  { "no-wrap", no_argument, NULL, CHAR_MAX + 4 },
  { "omit-header", no_argument, &xgettext_omit_header, 1 },
  { "output", required_argument, NULL, 'o' },
  { "output-dir", required_argument, NULL, 'p' },
  { "properties-output", no_argument, NULL, CHAR_MAX + 6 },
  { "qt", no_argument, NULL, CHAR_MAX + 9 },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "strict", no_argument, NULL, 'S' },
  { "string-limit", required_argument, NULL, 'l' },
  { "stringtable-output", no_argument, NULL, CHAR_MAX + 7 },
  { "trigraphs", no_argument, NULL, 'T' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { NULL, 0, NULL, 0 }
};


/* The extractors must all be functions returning void and taking three
   arguments designating the input stream and one message domain list argument
   in which to add the messages.  */
typedef void (*extractor_func) (FILE *fp, const char *real_filename,
				const char *logical_filename,
				flag_context_list_table_ty *flag_table,
				msgdomain_list_ty *mdlp);

typedef struct extractor_ty extractor_ty;
struct extractor_ty
{
  extractor_func func;
  flag_context_list_table_ty *flag_table;
  struct formatstring_parser *formatstring_parser1;
  struct formatstring_parser *formatstring_parser2;
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ > 4) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;
static void read_exclusion_file (char *file_name);
static void extract_from_file (const char *file_name, extractor_ty extractor,
			       msgdomain_list_ty *mdlp);
static message_ty *construct_header (void);
static void finalize_header (msgdomain_list_ty *mdlp);
static extractor_ty language_to_extractor (const char *name);
static const char *extension_to_language (const char *extension);


int
main (int argc, char *argv[])
{
  int cnt;
  int optchar;
  bool do_help = false;
  bool do_version = false;
  msgdomain_list_ty *mdlp;
  bool join_existing = false;
  bool no_default_keywords = false;
  bool some_additional_keywords = false;
  bool sort_by_msgid = false;
  bool sort_by_filepos = false;
  const char *file_name;
  const char *files_from = NULL;
  string_list_ty *file_list;
  char *output_file = NULL;
  const char *language = NULL;
  extractor_ty extractor = { NULL, NULL, NULL, NULL };

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

  /* Set initial value of variables.  */
  default_domain = MESSAGE_DOMAIN_DEFAULT;
  xgettext_global_source_encoding = po_charset_ascii;
  init_flag_table_c ();
  init_flag_table_objc ();
  init_flag_table_gcc_internal ();
  init_flag_table_sh ();
  init_flag_table_python ();
  init_flag_table_lisp ();
  init_flag_table_elisp ();
  init_flag_table_librep ();
  init_flag_table_scheme ();
  init_flag_table_java ();
  init_flag_table_csharp ();
  init_flag_table_awk ();
  init_flag_table_ycp ();
  init_flag_table_tcl ();
  init_flag_table_perl ();
  init_flag_table_php ();

  while ((optchar = getopt_long (argc, argv,
				 "ac::Cd:D:eEf:Fhijk::l:L:m::M::no:p:sTVw:x:",
				 long_options, NULL)) != EOF)
    switch (optchar)
      {
      case '\0':		/* Long option.  */
	break;
      case 'a':
	x_c_extract_all ();
	x_sh_extract_all ();
	x_python_extract_all ();
	x_lisp_extract_all ();
	x_elisp_extract_all ();
	x_librep_extract_all ();
	x_scheme_extract_all ();
	x_java_extract_all ();
	x_csharp_extract_all ();
	x_awk_extract_all ();
	x_tcl_extract_all ();
	x_perl_extract_all ();
	x_php_extract_all ();
	x_glade_extract_all ();
	break;
      case 'c':
	if (optarg == NULL)
	  {
	    add_all_comments = true;
	    comment_tag = NULL;
	  }
	else
	  {
	    add_all_comments = false;
	    comment_tag = optarg;
	    /* We ignore leading white space.  */
	    while (isspace ((unsigned char) *comment_tag))
	      ++comment_tag;
	  }
	break;
      case 'C':
	language = "C++";
	break;
      case 'd':
	default_domain = optarg;
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
      case 'j':
	join_existing = true;
	break;
      case 'k':
	if (optarg == NULL || *optarg != '\0')
	  {
	    x_c_keyword (optarg);
	    x_objc_keyword (optarg);
	    x_sh_keyword (optarg);
	    x_python_keyword (optarg);
	    x_lisp_keyword (optarg);
	    x_elisp_keyword (optarg);
	    x_librep_keyword (optarg);
	    x_scheme_keyword (optarg);
	    x_java_keyword (optarg);
	    x_csharp_keyword (optarg);
	    x_awk_keyword (optarg);
	    x_tcl_keyword (optarg);
	    x_perl_keyword (optarg);
	    x_php_keyword (optarg);
	    x_glade_keyword (optarg);
	    if (optarg == NULL)
	      no_default_keywords = true;
	    else
	      some_additional_keywords = true;
	  }
	break;
      case 'l':
	/* Accepted for backward compatibility with 0.10.35.  */
	break;
      case 'L':
	language = optarg;
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
	    output_dir = xasprintf ("%s/", optarg);
	}
	break;
      case 's':
	sort_by_msgid = true;
	break;
      case 'S':
	message_print_style_uniforum ();
	break;
      case 'T':
	x_c_trigraphs ();
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
      case 'x':
	read_exclusion_file (optarg);
	break;
      case CHAR_MAX + 1:	/* --copyright-holder */
	copyright_holder = optarg;
	break;
      case CHAR_MAX + 2:	/* --foreign-user */
	copyright_holder = "";
	break;
      case CHAR_MAX + 3:	/* --from-code */
	xgettext_global_source_encoding = po_charset_canonicalize (optarg);
	if (xgettext_global_source_encoding == NULL)
	  xgettext_global_source_encoding = po_charset_ascii;
	break;
      case CHAR_MAX + 4:	/* --no-wrap */
	message_page_width_ignore ();
	break;
      case CHAR_MAX + 5:	/* --msgid-bugs-address */
	msgid_bugs_address = optarg;
	break;
      case CHAR_MAX + 6:	/* --properties-output */
	message_print_syntax_properties ();
	output_syntax = syntax_properties;
	break;
      case CHAR_MAX + 7:	/* --stringtable-output */
	message_print_syntax_stringtable ();
	output_syntax = syntax_stringtable;
	break;
      case CHAR_MAX + 8:	/* --flag */
	xgettext_record_flag (optarg);
	break;
      case CHAR_MAX + 9:	/* --qt */
	recognize_format_qt = true;
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
      printf (_("Written by %s.\n"), "Ulrich Drepper");
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

  if (join_existing && strcmp (default_domain, "-") == 0)
    error (EXIT_FAILURE, 0, _("\
--join-existing cannot be used when output is written to stdout"));

  if (no_default_keywords && !some_additional_keywords)
    {
      error (0, 0, _("\
xgettext cannot work without keywords to look for"));
      usage (EXIT_FAILURE);
    }

  /* Test whether we have some input files given.  */
  if (files_from == NULL && optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input file given"));
      usage (EXIT_FAILURE);
    }

  /* Determine extractor from language.  */
  if (language != NULL)
    extractor = language_to_extractor (language);

  /* Canonize msgstr prefix/suffix.  */
  if (msgstr_prefix != NULL && msgstr_suffix == NULL)
    msgstr_suffix = "";
  else if (msgstr_prefix == NULL && msgstr_suffix != NULL)
    msgstr_prefix = "";

  /* Default output directory is the current directory.  */
  if (output_dir == NULL)
    output_dir = ".";

  /* Construct the name of the output file.  If the default domain has
     the special name "-" we write to stdout.  */
  if (output_file)
    {
      if (IS_ABSOLUTE_PATH (output_file) || strcmp (output_file, "-") == 0)
	file_name = xstrdup (output_file);
      else
	/* Please do NOT add a .po suffix! */
	file_name = concatenated_pathname (output_dir, output_file, NULL);
    }
  else if (strcmp (default_domain, "-") == 0)
    file_name = "-";
  else
    file_name = concatenated_pathname (output_dir, default_domain, ".po");

  /* Determine list of files we have to process.  */
  if (files_from != NULL)
    file_list = read_names_from_file (files_from);
  else
    file_list = string_list_alloc ();
  /* Append names from command line.  */
  for (cnt = optind; cnt < argc; ++cnt)
    string_list_append_unique (file_list, argv[cnt]);

  /* Allocate converter from xgettext_global_source_encoding to UTF-8 (except
     from ASCII or UTF-8, when this conversion is a no-op).  */
  if (xgettext_global_source_encoding != po_charset_ascii
      && xgettext_global_source_encoding != po_charset_utf8)
    {
#if HAVE_ICONV
      iconv_t cd;

      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
      if (strcmp (xgettext_global_source_encoding, "EUC-KR") == 0)
	cd = (iconv_t)(-1);
      else
# endif
      cd = iconv_open (po_charset_utf8, xgettext_global_source_encoding);
      if (cd == (iconv_t)(-1))
	error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(), \
and iconv() does not support this conversion."),
	       xgettext_global_source_encoding, po_charset_utf8,
	       basename (program_name));
      xgettext_global_source_iconv = cd;
#else
      error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(). \
This version was built without iconv()."),
	     xgettext_global_source_encoding, po_charset_utf8,
	     basename (program_name));
#endif
    }

  /* Allocate a message list to remember all the messages.  */
  mdlp = msgdomain_list_alloc (true);

  /* Generate a header, so that we know how and when this PO file was
     created.  */
  if (!xgettext_omit_header)
    message_list_append (mdlp->item[0]->messages, construct_header ());

  /* Read in the old messages, so that we can add to them.  */
  if (join_existing)
    {
      /* Temporarily reset the directory list to empty, because file_name
	 is an output file and therefore should not be searched for.  */
      void *saved_directory_list = dir_list_save_reset ();
      extractor_ty po_extractor = { extract_po, NULL, NULL, NULL };

      extract_from_file (file_name, po_extractor, mdlp);
      if (!is_ascii_msgdomain_list (mdlp))
	mdlp = iconv_msgdomain_list (mdlp, "UTF-8", file_name);

      dir_list_restore (saved_directory_list);
    }

  /* Process all input files.  */
  for (cnt = 0; cnt < file_list->nitems; ++cnt)
    {
      const char *filename;
      extractor_ty this_file_extractor;

      filename = file_list->item[cnt];

      if (extractor.func)
	this_file_extractor = extractor;
      else
	{
	  const char *base;
	  char *reduced;
	  const char *extension;
	  const char *language;

	  base = strrchr (filename, '/');
	  if (!base)
	    base = filename;

	  reduced = xstrdup (base);
	  /* Remove a trailing ".in" - it's a generic suffix.  */
	  if (strlen (reduced) >= 3
	      && memcmp (reduced + strlen (reduced) - 3, ".in", 3) == 0)
	    reduced[strlen (reduced) - 3] = '\0';

	  /* Work out what the file extension is.  */
	  extension = strrchr (reduced, '.');
	  if (extension)
	    ++extension;
	  else
	    extension = "";

	  /* Derive the language from the extension, and the extractor
	     function from the language.  */
	  language = extension_to_language (extension);
	  if (language == NULL)
	    {
	      error (0, 0, _("\
warning: file `%s' extension `%s' is unknown; will try C"), filename, extension);
	      language = "C";
	    }
	  this_file_extractor = language_to_extractor (language);

	  free (reduced);
	}

      /* Extract the strings from the file.  */
      extract_from_file (filename, this_file_extractor, mdlp);
    }
  string_list_free (file_list);

  /* Finalize the constructed header.  */
  if (!xgettext_omit_header)
    finalize_header (mdlp);

  /* Free the allocated converter.  */
#if HAVE_ICONV
  if (xgettext_global_source_encoding != po_charset_ascii
      && xgettext_global_source_encoding != po_charset_utf8)
    iconv_close (xgettext_global_source_iconv);
#endif

  /* Sorting the list of messages.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (mdlp);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (mdlp);

  /* Write the PO file.  */
  msgdomain_list_print (mdlp, file_name, force_po, do_debug);

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
      printf (_("\
Extract translatable strings from given input files.\n\
"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
Similarly for optional arguments.\n\
"));
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
  -d, --default-domain=NAME   use NAME.po for output (instead of messages.po)\n"));
      printf (_("\
  -o, --output=FILE           write output to specified file\n"));
      printf (_("\
  -p, --output-dir=DIR        output files will be placed in directory DIR\n"));
      printf (_("\
If output file is -, output is written to standard output.\n"));
      printf ("\n");
      printf (_("\
Choice of input file language:\n"));
      printf (_("\
  -L, --language=NAME         recognise the specified language\n\
                                (C, C++, ObjectiveC, PO, Shell, Python, Lisp,\n\
                                EmacsLisp, librep, Scheme, Smalltalk, Java,\n\
                                JavaProperties, C#, awk, YCP, Tcl, Perl, PHP,\n\
                                GCC-source, NXStringTable, RST, Glade)\n"));
      printf (_("\
  -C, --c++                   shorthand for --language=C++\n"));
      printf (_("\
By default the language is guessed depending on the input file name extension.\n"));
      printf ("\n");
      printf (_("\
Input file interpretation:\n"));
      printf (_("\
      --from-code=NAME        encoding of input files\n\
                                (except for Python, Tcl, Glade)\n"));
      printf (_("\
By default the input files are assumed to be in ASCII.\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -j, --join-existing         join messages with existing file\n"));
      printf (_("\
  -x, --exclude-file=FILE.po  entries from FILE.po are not extracted\n"));
      printf (_("\
  -c, --add-comments[=TAG]    place comment block with TAG (or those\n\
                              preceding keyword lines) in output file\n"));
      printf ("\n");
      printf (_("\
Language specific options:\n"));
      printf (_("\
  -a, --extract-all           extract all strings\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, Tcl, Perl, PHP, GCC-source, Glade)\n"));
      printf (_("\
  -k, --keyword[=WORD]        additional keyword to be looked for (without\n\
                              WORD means not to use default keywords)\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, Tcl, Perl, PHP, GCC-source, Glade)\n"));
      printf (_("\
      --flag=WORD:ARG:FLAG    additional flag for strings inside the argument\n\
                              number ARG of keyword WORD\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, YCP, Tcl, Perl, PHP, GCC-source)\n"));
      printf (_("\
  -T, --trigraphs             understand ANSI C trigraphs for input\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC)\n"));
      printf (_("\
      --qt                    recognize Qt format strings\n"));
      printf (_("\
                                (only language C++)\n"));
      printf (_("\
      --debug                 more detailed formatstring recognition result\n"));
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
      --properties-output     write out a Java .properties file\n"));
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
      printf (_("\
      --copyright-holder=STRING  set copyright holder in output\n"));
      printf (_("\
      --foreign-user          omit FSF copyright in output for foreign user\n"));
      printf (_("\
      --msgid-bugs-address=EMAIL@ADDRESS  set report address for msgid bugs\n"));
      printf (_("\
  -m, --msgstr-prefix[=STRING]  use STRING or \"\" as prefix for msgstr entries\n"));
      printf (_("\
  -M, --msgstr-suffix[=STRING]  use STRING or \"\" as suffix for msgstr entries\n"));
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


static void
exclude_directive_domain (abstract_po_reader_ty *pop, char *name)
{
  po_gram_error_at_line (&gram_pos,
			 _("this file may not contain domain directives"));
}


static void
exclude_directive_message (abstract_po_reader_ty *pop,
			   char *msgid,
			   lex_pos_ty *msgid_pos,
			   char *msgid_plural,
			   char *msgstr, size_t msgstr_len,
			   lex_pos_ty *msgstr_pos,
			   bool force_fuzzy, bool obsolete)
{
  message_ty *mp;

  /* See if this message ID has been seen before.  */
  if (exclude == NULL)
    exclude = message_list_alloc (true);
  mp = message_list_search (exclude, msgid);
  if (mp != NULL)
    free (msgid);
  else
    {
      mp = message_alloc (msgid, msgid_plural, "", 1, msgstr_pos);
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

static abstract_po_reader_class_ty exclude_methods =
{
  sizeof (abstract_po_reader_ty),
  NULL, /* constructor */
  NULL, /* destructor */
  NULL, /* parse_brief */
  NULL, /* parse_debrief */
  exclude_directive_domain,
  exclude_directive_message,
  NULL, /* comment */
  NULL, /* comment_dot */
  NULL, /* comment_filepos */
  NULL, /* comment_special */
};


static void
read_exclusion_file (char *filename)
{
  char *real_filename;
  FILE *fp = open_po_file (filename, &real_filename, true);
  abstract_po_reader_ty *pop;

  pop = po_reader_alloc (&exclude_methods);
  po_scan (pop, fp, real_filename, filename, input_syntax);
  po_reader_free (pop);

  if (fp != stdin)
    fclose (fp);
}


void
split_keywordspec (const char *spec,
		   const char **endp, int *argnum1p, int *argnum2p)
{
  const char *p;

  /* Start parsing from the end.  */
  p = spec + strlen (spec);
  if (p > spec && isdigit ((unsigned char) p[-1]))
    {
      const char *last_arg;

      do
	p--;
      while (p > spec && isdigit ((unsigned char) p[-1]));

      last_arg = p;

      if (p > spec && p[-1] == ',')
	{
	  p--;

	  if (p > spec && isdigit ((unsigned char) p[-1]))
	    {
	      const char *first_arg;

	      do
		p--;
	      while (p > spec && isdigit ((unsigned char) p[-1]));

	      first_arg = p;

	      if (p > spec && p[-1] == ':')
		{
		  /* Parsed "KEYWORD:ARGNUM1,ARGNUM2".  */
		  char *dummy;

		  *endp = p - 1;
		  *argnum1p = strtol (first_arg, &dummy, 10);
		  *argnum2p = strtol (last_arg, &dummy, 10);
		  return;
		}
	    }
	}
      else if (p > spec && p[-1] == ':')
	{
	  /* Parsed "KEYWORD:ARGNUM1.  */
	  char *dummy;

	  *endp = p - 1;
	  *argnum1p = strtol (last_arg, &dummy, 10);
	  *argnum2p = 0;
	  return;
	}
    }
  /* Parsed "KEYWORD".  */
  *endp = p + strlen (p);
  *argnum1p = 0;
  *argnum2p = 0;
}


/* Null context.  */
flag_context_ty null_context = { undecided, false, undecided, false };

/* Transparent context.  */
flag_context_ty passthrough_context = { undecided, true, undecided, true };


flag_context_ty
inherited_context (flag_context_ty outer_context,
		   flag_context_ty modifier_context)
{
  flag_context_ty result = modifier_context;

  if (result.pass_format1)
    {
      result.is_format1 = outer_context.is_format1;
      result.pass_format1 = false;
    }
  if (result.pass_format2)
    {
      result.is_format2 = outer_context.is_format2;
      result.pass_format2 = false;
    }
  return result;
}


/* Null context list iterator.  */
flag_context_list_iterator_ty null_context_list_iterator = { 1, NULL };

/* Transparent context list iterator.  */
static flag_context_list_ty passthrough_context_circular_list =
  {
    1,
    { undecided, true, undecided, true },
    &passthrough_context_circular_list
  };
flag_context_list_iterator_ty passthrough_context_list_iterator =
  {
    1,
    &passthrough_context_circular_list
  };


flag_context_list_iterator_ty
flag_context_list_iterator (flag_context_list_ty *list)
{
  flag_context_list_iterator_ty result;

  result.argnum = 1;
  result.head = list;
  return result;
}


flag_context_ty
flag_context_list_iterator_advance (flag_context_list_iterator_ty *iter)
{
  if (iter->head == NULL)
    return null_context;
  if (iter->argnum == iter->head->argnum)
    {
      flag_context_ty result = iter->head->flags;

      /* Special casing of circular list.  */
      if (iter->head != iter->head->next)
	{
	  iter->head = iter->head->next;
	  iter->argnum++;
	}

      return result;
    }
  else
    {
      iter->argnum++;
      return null_context;
    }
}


flag_context_list_ty *
flag_context_list_table_lookup (flag_context_list_table_ty *flag_table,
				const void *key, size_t keylen)
{
  void *entry;

  if (flag_table->table != NULL
      && find_entry (flag_table, key, keylen, &entry) == 0)
    return (flag_context_list_ty *) entry;
  else
    return NULL;
}


static void
flag_context_list_table_insert (flag_context_list_table_ty *table,
				unsigned int index,
				const char *name_start, const char *name_end,
				int argnum, enum is_format value, bool pass)
{
  char *allocated_name = NULL;

  if (table == &flag_table_lisp)
    {
      /* Convert NAME to upper case.  */
      size_t name_len = name_end - name_start;
      char *name = allocated_name = (char *) xallocsa (name_len);
      size_t i;

      for (i = 0; i < name_len; i++)
	name[i] = (name_start[i] >= 'a' && name_start[i] <= 'z'
		   ? name_start[i] - 'a' + 'A'
		   : name_start[i]);
      name_start = name;
      name_end = name + name_len;
    }
  else if (table == &flag_table_tcl)
    {
      /* Remove redundant "::" prefix.  */
      if (name_end - name_start > 2
	  && name_start[0] == ':' && name_start[1] == ':')
	name_start += 2;
    }

  /* Insert the pair (VALUE, PASS) at INDEX in the element numbered ARGNUM
     of the list corresponding to NAME in the TABLE.  */
  if (table->table == NULL)
    init_hash (table, 100);
  {
    void *entry;

    if (find_entry (table, name_start, name_end - name_start, &entry) != 0)
      {
	/* Create new hash table entry.  */
	flag_context_list_ty *list =
	  (flag_context_list_ty *) xmalloc (sizeof (flag_context_list_ty));
	list->argnum = argnum;
	memset (&list->flags, '\0', sizeof (list->flags));
	switch (index)
	  {
	  case 0:
	    list->flags.is_format1 = value;
	    list->flags.pass_format1 = pass;
	    break;
	  case 1:
	    list->flags.is_format2 = value;
	    list->flags.pass_format2 = pass;
	    break;
	  default:
	    abort ();
	  }
	list->next = NULL;
	insert_entry (table, name_start, name_end - name_start, list);
      }
    else
      {
	flag_context_list_ty *list = (flag_context_list_ty *)entry;
	flag_context_list_ty **lastp = NULL;

	while (list != NULL && list->argnum < argnum)
	  {
	    lastp = &list->next;
	    list = *lastp;
	  }
	if (list != NULL && list->argnum == argnum)
	  {
	    /* Add this flag to the current argument number.  */
	    switch (index)
	      {
	      case 0:
		list->flags.is_format1 = value;
		list->flags.pass_format1 = pass;
		break;
	      case 1:
		list->flags.is_format2 = value;
		list->flags.pass_format2 = pass;
		break;
	      default:
		abort ();
	      }
	  }
	else if (lastp != NULL)
	  {
	    /* Add a new list entry for this argument number.  */
	    list =
	      (flag_context_list_ty *) xmalloc (sizeof (flag_context_list_ty));
	    list->argnum = argnum;
	    memset (&list->flags, '\0', sizeof (list->flags));
	    switch (index)
	      {
	      case 0:
		list->flags.is_format1 = value;
		list->flags.pass_format1 = pass;
		break;
	      case 1:
		list->flags.is_format2 = value;
		list->flags.pass_format2 = pass;
		break;
	      default:
		abort ();
	      }
	    list->next = *lastp;
	    *lastp = list;
	  }
	else
	  {
	    /* Add a new list entry for this argument number, at the beginning
	       of the list.  Since we don't have an API for replacing the
	       value of a key in the hash table, we have to copy the first
	       list element.  */
	    flag_context_list_ty *copy =
	      (flag_context_list_ty *) xmalloc (sizeof (flag_context_list_ty));
	    *copy = *list;

	    list->argnum = argnum;
	    memset (&list->flags, '\0', sizeof (list->flags));
	    switch (index)
	      {
	      case 0:
		list->flags.is_format1 = value;
		list->flags.pass_format1 = pass;
		break;
	      case 1:
		list->flags.is_format2 = value;
		list->flags.pass_format2 = pass;
		break;
	      default:
		abort ();
	      }
	    list->next = copy;
	  }
      }
  }

  if (allocated_name != NULL)
    freesa (allocated_name);
}


void
xgettext_record_flag (const char *optionstring)
{
  /* Check the string has at least two colons.  (Colons in the name are
     allowed, needed for the Lisp and the Tcl backends.)  */
  const char *colon1;
  const char *colon2;

  for (colon2 = optionstring + strlen (optionstring); ; )
    {
      if (colon2 == optionstring)
	goto err;
      colon2--;
      if (*colon2 == ':')
	break;
    }
  for (colon1 = colon2; ; )
    {
      if (colon1 == optionstring)
	goto err;
      colon1--;
      if (*colon1 == ':')
	break;
    }
  {
    const char *name_start = optionstring;
    const char *name_end = colon1;
    const char *argnum_start = colon1 + 1;
    const char *argnum_end = colon2;
    const char *flag = colon2 + 1;
    int argnum;

    /* Check the parts' syntax.  */
    if (name_end == name_start)
      goto err;
    if (argnum_end == argnum_start)
      goto err;
    {
      char *endp;
      argnum = strtol (argnum_start, &endp, 10);
      if (endp != argnum_end)
	goto err;
    }
    if (argnum <= 0)
      goto err;

    /* Analyze the flag part.  */
    {
      bool pass;

      pass = false;
      if (strlen (flag) >= 5 && memcmp (flag, "pass-", 5) == 0)
	{
	  pass = true;
	  flag += 5;
	}

      /* Unlike po_parse_comment_special(), we don't accept "fuzzy" or "wrap"
	 here - it has no sense.  */
      if (strlen (flag) >= 7
	  && memcmp (flag + strlen (flag) - 7, "-format", 7) == 0)
	{
	  const char *p;
	  size_t n;
	  enum is_format value;
	  size_t type;

	  p = flag;
	  n = strlen (flag) - 7;

	  if (n >= 3 && memcmp (p, "no-", 3) == 0)
	    {
	      p += 3;
	      n -= 3;
	      value = no;
	    }
	  else if (n >= 9 && memcmp (p, "possible-", 9) == 0)
	    {
	      p += 9;
	      n -= 9;
	      value = possible;
	    }
	  else if (n >= 11 && memcmp (p, "impossible-", 11) == 0)
	    {
	      p += 11;
	      n -= 11;
	      value = impossible;
	    }
	  else
	    value = yes_according_to_context;

	  for (type = 0; type < NFORMATS; type++)
	    if (strlen (format_language[type]) == n
		&& memcmp (format_language[type], p, n) == 0)
	      {
		switch (type)
		  {
		  case format_c:
		    flag_context_list_table_insert (&flag_table_c, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    flag_context_list_table_insert (&flag_table_objc, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_objc:
		    flag_context_list_table_insert (&flag_table_objc, 1,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_sh:
		    flag_context_list_table_insert (&flag_table_sh, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_python:
		    flag_context_list_table_insert (&flag_table_python, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_lisp:
		    flag_context_list_table_insert (&flag_table_lisp, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_elisp:
		    flag_context_list_table_insert (&flag_table_elisp, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_librep:
		    flag_context_list_table_insert (&flag_table_librep, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_scheme:
		    flag_context_list_table_insert (&flag_table_scheme, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_smalltalk:
		    break;
		  case format_java:
		    flag_context_list_table_insert (&flag_table_java, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_csharp:
		    flag_context_list_table_insert (&flag_table_csharp, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_awk:
		    flag_context_list_table_insert (&flag_table_awk, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_pascal:
		    break;
		  case format_ycp:
		    flag_context_list_table_insert (&flag_table_ycp, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_tcl:
		    flag_context_list_table_insert (&flag_table_tcl, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_perl:
		    flag_context_list_table_insert (&flag_table_perl, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_perl_brace:
		    flag_context_list_table_insert (&flag_table_perl, 1,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_php:
		    flag_context_list_table_insert (&flag_table_php, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_gcc_internal:
		    flag_context_list_table_insert (&flag_table_gcc_internal, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  case format_qt:
		    flag_context_list_table_insert (&flag_table_c, 0,
						    name_start, name_end,
						    argnum, value, pass);
		    break;
		  default:
		    abort ();
		  }
		return;
	      }
	  /* If the flag is not among the valid values, the optionstring is
	     invalid.  */
	}
    }
  }

err:
  error (EXIT_FAILURE, 0, _("\
A --flag argument doesn't have the <keyword>:<argnum>:[pass-]<flag> syntax: %s"),
	 optionstring);
}


static string_list_ty *comment;

void
xgettext_comment_add (const char *str)
{
  if (comment == NULL)
    comment = string_list_alloc ();
  string_list_append (comment, str);
}

const char *
xgettext_comment (size_t n)
{
  if (comment == NULL || n >= comment->nitems)
    return NULL;
  return comment->item[n];
}

void
xgettext_comment_reset ()
{
  if (comment != NULL)
    {
      string_list_free (comment);
      comment = NULL;
    }
}


refcounted_string_list_ty *savable_comment;

void
savable_comment_add (const char *str)
{
  if (savable_comment == NULL)
    {
      savable_comment =
	(refcounted_string_list_ty *) xmalloc (sizeof (*savable_comment));
      savable_comment->refcount = 1;
      string_list_init (&savable_comment->contents);
    }
  else if (savable_comment->refcount > 1)
    {
      /* Unshare the list by making copies.  */
      struct string_list_ty *oldcontents;
      size_t i;

      savable_comment->refcount--;
      oldcontents = &savable_comment->contents;

      savable_comment =
	(refcounted_string_list_ty *) xmalloc (sizeof (*savable_comment));
      savable_comment->refcount = 1;
      string_list_init (&savable_comment->contents);
      for (i = 0; i < oldcontents->nitems; i++)
	string_list_append (&savable_comment->contents, oldcontents->item[i]);
    }
  string_list_append (&savable_comment->contents, str);
}

void
savable_comment_reset ()
{
  drop_reference (savable_comment);
  savable_comment = NULL;
}

void
savable_comment_to_xgettext_comment (refcounted_string_list_ty *rslp)
{
  xgettext_comment_reset ();
  if (rslp != NULL)
    {
      size_t i;

      for (i = 0; i < rslp->contents.nitems; i++)
	xgettext_comment_add (rslp->contents.item[i]);
    }
}



static FILE *
xgettext_open (const char *fn,
	       char **logical_file_name_p, char **real_file_name_p)
{
  FILE *fp;
  char *new_name;
  char *logical_file_name;

  if (strcmp (fn, "-") == 0)
    {
      new_name = xstrdup (_("standard input"));
      logical_file_name = xstrdup (new_name);
      fp = stdin;
    }
  else if (IS_ABSOLUTE_PATH (fn))
    {
      new_name = xstrdup (fn);
      fp = fopen (fn, "r");
      if (fp == NULL)
	error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), fn);
      logical_file_name = xstrdup (new_name);
    }
  else
    {
      int j;

      for (j = 0; ; ++j)
	{
	  const char *dir = dir_list_nth (j);

	  if (dir == NULL)
	    error (EXIT_FAILURE, ENOENT, _("\
error while opening \"%s\" for reading"), fn);

	  new_name = concatenated_pathname (dir, fn, NULL);

	  fp = fopen (new_name, "r");
	  if (fp != NULL)
	    break;

	  if (errno != ENOENT)
	    error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), new_name);
	  free (new_name);
	}

      /* Note that the NEW_NAME variable contains the actual file name
	 and the logical file name is what is reported by xgettext.  In
	 this case NEW_NAME is set to the file which was found along the
	 directory search path, and LOGICAL_FILE_NAME is is set to the
	 file name which was searched for.  */
      logical_file_name = xstrdup (fn);
    }

  *logical_file_name_p = logical_file_name;
  *real_file_name_p = new_name;
  return fp;
}


/* Language dependent format string parser.
   NULL if the language has no notion of format strings.  */
static struct formatstring_parser *current_formatstring_parser1;
static struct formatstring_parser *current_formatstring_parser2;


static void
extract_from_file (const char *file_name, extractor_ty extractor,
		   msgdomain_list_ty *mdlp)
{
  char *logical_file_name;
  char *real_file_name;
  FILE *fp = xgettext_open (file_name, &logical_file_name, &real_file_name);

  /* Set the default for the source file encoding.  May be overridden by
     the extractor function.  */
  xgettext_current_source_encoding = xgettext_global_source_encoding;
#if HAVE_ICONV
  xgettext_current_source_iconv = xgettext_global_source_iconv;
#endif

  current_formatstring_parser1 = extractor.formatstring_parser1;
  current_formatstring_parser2 = extractor.formatstring_parser2;
  extractor.func (fp, real_file_name, logical_file_name, extractor.flag_table,
		  mdlp);

  if (fp != stdin)
    fclose (fp);
  free (logical_file_name);
  free (real_file_name);
}



#if !HAVE_ICONV
/* If we don't have iconv(), the only supported values for
   xgettext_global_source_encoding and thus also for
   xgettext_current_source_encoding are ASCII and UTF-8.
   convert_string() should not be called in this case.  */
#define convert_string(cd,string) (abort (), (string))
#endif

/* Convert the given string from xgettext_current_source_encoding to
   the output file encoding (i.e. ASCII or UTF-8).
   The resulting string is either the argument string, or freshly allocated.
   The file_name and line_number are only used for error message purposes.  */
char *
from_current_source_encoding (const char *string,
			      const char *file_name, size_t line_number)
{
  if (xgettext_current_source_encoding == po_charset_ascii)
    {
      if (!is_ascii_string (string))
	{
	  char buffer[21];

	  if (line_number == (size_t)(-1))
	    buffer[0] = '\0';
	  else
	    sprintf (buffer, ":%ld", (long) line_number);
	  multiline_error (xstrdup (""),
			   xasprintf (_("\
Non-ASCII string at %s%s.\n\
Please specify the source encoding through --from-code.\n"),
				      file_name, buffer));
	  exit (EXIT_FAILURE);
	}
    }
  else if (xgettext_current_source_encoding != po_charset_utf8)
    string = convert_string (xgettext_current_source_iconv, string);

  return (char *) string;
}

#define CONVERT_STRING(string) \
  string = from_current_source_encoding (string, pos->file_name, \
					 pos->line_number);


/* Update the is_format[] flags depending on the information given in the
   context.  */
static void
set_format_flags_from_context (enum is_format is_format[NFORMATS],
			       flag_context_ty context, const char *string,
			       lex_pos_ty *pos, const char *pretty_msgstr)
{
  size_t i;

  if (context.is_format1 != undecided || context.is_format2 != undecided)
    for (i = 0; i < NFORMATS; i++)
      {
	if (is_format[i] == undecided)
	  {
	    if (formatstring_parsers[i] == current_formatstring_parser1
		&& context.is_format1 != undecided)
	      is_format[i] = (enum is_format) context.is_format1;
	    if (formatstring_parsers[i] == current_formatstring_parser2
		&& context.is_format2 != undecided)
	      is_format[i] = (enum is_format) context.is_format2;
	  }
	if (possible_format_p (is_format[i]))
	  {
	    struct formatstring_parser *parser = formatstring_parsers[i];
	    char *invalid_reason = NULL;
	    void *descr = parser->parse (string, false, &invalid_reason);

	    if (descr != NULL)
	      parser->free (descr);
	    else
	      {
		/* The string is not a valid format string.  */
		if (is_format[i] != possible)
		  {
		    char buffer[21];

		    error_with_progname = false;
		    if (pos->line_number == (size_t)(-1))
		      buffer[0] = '\0';
		    else
		      sprintf (buffer, ":%ld", (long) pos->line_number);
		    multiline_warning (xasprintf (_("%s%s: warning: "),
						  pos->file_name, buffer),
				       xasprintf (is_format[i] == yes_according_to_context ? _("Although being used in a format string position, the %s is not a valid %s format string. Reason: %s\n") : _("Although declared as such, the %s is not a valid %s format string. Reason: %s\n"),
						  pretty_msgstr,
						  format_language_pretty[i],
						  invalid_reason));
		    error_with_progname = true;
		  }

		is_format[i] = impossible;
		free (invalid_reason);
	      }
	  }
      }
}


message_ty *
remember_a_message (message_list_ty *mlp, char *string,
		    flag_context_ty context, lex_pos_ty *pos)
{
  enum is_format is_format[NFORMATS];
  enum is_wrap do_wrap;
  char *msgid;
  message_ty *mp;
  char *msgstr;
  size_t i;

  msgid = string;

  /* See whether we shall exclude this message.  */
  if (exclude != NULL && message_list_search (exclude, msgid) != NULL)
    {
      /* Tell the lexer to reset its comment buffer, so that the next
	 message gets the correct comments.  */
      xgettext_comment_reset ();

      return NULL;
    }

  for (i = 0; i < NFORMATS; i++)
    is_format[i] = undecided;
  do_wrap = undecided;

  CONVERT_STRING (msgid);

  if (msgid[0] == '\0' && !xgettext_omit_header)
    {
      char buffer[21];

      error_with_progname = false;
      if (pos->line_number == (size_t)(-1))
	buffer[0] = '\0';
      else
	sprintf (buffer, ":%ld", (long) pos->line_number);
      multiline_warning (xasprintf (_("%s%s: warning: "), pos->file_name,
				    buffer),
			 xstrdup (_("\
Empty msgid.  It is reserved by GNU gettext:\n\
gettext(\"\") returns the header entry with\n\
meta information, not the empty string.\n")));
      error_with_progname = true;
    }

  /* See if we have seen this message before.  */
  mp = message_list_search (mlp, msgid);
  if (mp != NULL)
    {
      free (msgid);
      for (i = 0; i < NFORMATS; i++)
	is_format[i] = mp->is_format[i];
      do_wrap = mp->do_wrap;
    }
  else
    {
      static lex_pos_ty dummypos = { __FILE__, __LINE__ };

      /* Construct the msgstr from the prefix and suffix, otherwise use the
	 empty string.  */
      if (msgstr_prefix)
	{
	  msgstr = (char *) xmalloc (strlen (msgstr_prefix)
				     + strlen (msgid)
				     + strlen (msgstr_suffix) + 1);
	  stpcpy (stpcpy (stpcpy (msgstr, msgstr_prefix), msgid),
		  msgstr_suffix);
	}
      else
	msgstr = "";

      /* Allocate a new message and append the message to the list.  */
      mp = message_alloc (msgid, NULL, msgstr, strlen (msgstr) + 1, &dummypos);
      /* Do not free msgid.  */
      message_list_append (mlp, mp);
    }

  /* Determine whether the context specifies that the msgid is a format
     string.  */
  set_format_flags_from_context (is_format, context, mp->msgid, pos, "msgid");

  /* Ask the lexer for the comments it has seen.  */
  {
    size_t nitems_before;
    size_t nitems_after;
    int j;
    bool add_all_remaining_comments;

    nitems_before = (mp->comment_dot != NULL ? mp->comment_dot->nitems : 0);

    add_all_remaining_comments = add_all_comments;
    for (j = 0; ; ++j)
      {
	const char *s = xgettext_comment (j);
	const char *t;
	if (s == NULL)
	  break;

	CONVERT_STRING (s);

	/* To reduce the possibility of unwanted matches we do a two
	   step match: the line must contain `xgettext:' and one of
	   the possible format description strings.  */
	if ((t = strstr (s, "xgettext:")) != NULL)
	  {
	    bool tmp_fuzzy;
	    enum is_format tmp_format[NFORMATS];
	    enum is_wrap tmp_wrap;
	    bool interesting;

	    t += strlen ("xgettext:");

	    po_parse_comment_special (t, &tmp_fuzzy, tmp_format, &tmp_wrap);

	    interesting = false;
	    for (i = 0; i < NFORMATS; i++)
	      if (tmp_format[i] != undecided)
		{
		  is_format[i] = tmp_format[i];
		  interesting = true;
		}
	    if (tmp_wrap != undecided)
	      {
		do_wrap = tmp_wrap;
		interesting = true;
	      }

	    /* If the "xgettext:" marker was followed by an interesting
	       keyword, and we updated our is_format/do_wrap variables,
	       we don't print the comment as a #. comment.  */
	    if (interesting)
	      continue;
	  }
	/* When the comment tag is seen, it drags in not only the line
	   which it starts, but all remaining comment lines.  */
	if (add_all_remaining_comments
	    || (add_all_remaining_comments =
		  (comment_tag != NULL
		   && strncmp (s, comment_tag, strlen (comment_tag)) == 0)))
	  message_comment_dot_append (mp, s);
      }

    nitems_after = (mp->comment_dot != NULL ? mp->comment_dot->nitems : 0);

    /* Don't add the comments if they are a repetition of the tail of the
       already present comments.  This avoids unneeded duplication if the
       same message appears several times, each time with the same comment.  */
    if (nitems_before < nitems_after)
      {
	size_t added = nitems_after - nitems_before;

	if (added <= nitems_before)
	  {
	    bool repeated = true;

	    for (i = 0; i < added; i++)
	      if (strcmp (mp->comment_dot->item[nitems_before - added + i],
			  mp->comment_dot->item[nitems_before + i]) != 0)
		{
		  repeated = false;
		  break;
		}

	    if (repeated)
	      {
		for (i = 0; i < added; i++)
		  free ((char *) mp->comment_dot->item[nitems_before + i]);
		mp->comment_dot->nitems = nitems_before;
	      }
	  }
      }
  }

  /* If it is not already decided, through programmer comments, whether the
     msgid is a format string, examine the msgid.  This is a heuristic.  */
  for (i = 0; i < NFORMATS; i++)
    {
      if (is_format[i] == undecided
	  && (formatstring_parsers[i] == current_formatstring_parser1
	      || formatstring_parsers[i] == current_formatstring_parser2)
	  /* But avoid redundancy: objc-format is stronger than c-format.  */
	  && !(i == format_c && possible_format_p (is_format[format_objc]))
	  && !(i == format_objc && possible_format_p (is_format[format_c])))
	{
	  struct formatstring_parser *parser = formatstring_parsers[i];
	  char *invalid_reason = NULL;
	  void *descr = parser->parse (mp->msgid, false, &invalid_reason);

	  if (descr != NULL)
	    {
	      /* msgid is a valid format string.  We mark only those msgids
		 as format strings which contain at least one format directive
		 and thus are format strings with a high probability.  We
		 don't mark strings without directives as format strings,
		 because that would force the programmer to add
		 "xgettext: no-c-format" anywhere where a translator wishes
		 to use a percent sign.  So, the msgfmt checking will not be
		 perfect.  Oh well.  */
	      if (parser->get_number_of_directives (descr) > 0)
		is_format[i] = possible;

	      parser->free (descr);
	    }
	  else
	    {
	      /* msgid is not a valid format string.  */
	      is_format[i] = impossible;
	      free (invalid_reason);
	    }
	}
      mp->is_format[i] = is_format[i];
    }

  mp->do_wrap = do_wrap == no ? no : yes;	/* By default we wrap.  */

  /* Remember where we saw this msgid.  */
  if (line_comment)
    message_comment_filepos (mp, pos->file_name, pos->line_number);

  /* Tell the lexer to reset its comment buffer, so that the next
     message gets the correct comments.  */
  xgettext_comment_reset ();

  return mp;
}


void
remember_a_message_plural (message_ty *mp, char *string,
			   flag_context_ty context, lex_pos_ty *pos)
{
  char *msgid_plural;
  char *msgstr1;
  size_t msgstr1_len;
  char *msgstr;
  size_t i;

  msgid_plural = string;

  CONVERT_STRING (msgid_plural);

  /* See if the message is already a plural message.  */
  if (mp->msgid_plural == NULL)
    {
      mp->msgid_plural = msgid_plural;

      /* Construct the first plural form from the prefix and suffix,
	 otherwise use the empty string.  The translator will have to
	 provide additional plural forms.  */
      if (msgstr_prefix)
	{
	  msgstr1 = (char *) xmalloc (strlen (msgstr_prefix)
				      + strlen (msgid_plural)
				      + strlen (msgstr_suffix) + 1);
	  stpcpy (stpcpy (stpcpy (msgstr1, msgstr_prefix), msgid_plural),
		  msgstr_suffix);
	}
      else
	msgstr1 = "";
      msgstr1_len = strlen (msgstr1) + 1;
      msgstr = (char *) xmalloc (mp->msgstr_len + msgstr1_len);
      memcpy (msgstr, mp->msgstr, mp->msgstr_len);
      memcpy (msgstr + mp->msgstr_len, msgstr1, msgstr1_len);
      mp->msgstr = msgstr;
      mp->msgstr_len = mp->msgstr_len + msgstr1_len;

      /* Determine whether the context specifies that the msgid_plural is a
	 format string.  */
      set_format_flags_from_context (mp->is_format, context, mp->msgid_plural,
				     pos, "msgid_plural");

      /* If it is not already decided, through programmer comments or
	 the msgid, whether the msgid is a format string, examine the
	 msgid_plural.  This is a heuristic.  */
      for (i = 0; i < NFORMATS; i++)
	if ((formatstring_parsers[i] == current_formatstring_parser1
	     || formatstring_parsers[i] == current_formatstring_parser2)
	    && (mp->is_format[i] == undecided || mp->is_format[i] == possible)
	    /* But avoid redundancy: objc-format is stronger than c-format.  */
	    && !(i == format_c
		 && possible_format_p (mp->is_format[format_objc]))
	    && !(i == format_objc
		 && possible_format_p (mp->is_format[format_c])))
	  {
	    struct formatstring_parser *parser = formatstring_parsers[i];
	    char *invalid_reason = NULL;
	    void *descr =
	      parser->parse (mp->msgid_plural, false, &invalid_reason);

	    if (descr != NULL)
	      {
		/* Same heuristic as in remember_a_message.  */
		if (parser->get_number_of_directives (descr) > 0)
		  mp->is_format[i] = possible;

		parser->free (descr);
	      }
	    else
	      {
		/* msgid_plural is not a valid format string.  */
		mp->is_format[i] = impossible;
		free (invalid_reason);
	      }
	  }
    }
  else
    free (msgid_plural);
}


static message_ty *
construct_header ()
{
  time_t now;
  char *timestring;
  message_ty *mp;
  char *msgstr;
  static lex_pos_ty pos = { __FILE__, __LINE__ };

  if (msgid_bugs_address != NULL && msgid_bugs_address[0] == '\0')
    multiline_warning (xasprintf (_("warning: ")),
		       xstrdup (_("\
The option --msgid-bugs-address was not specified.\n\
If you are using a `Makevars' file, please specify\n\
the MSGID_BUGS_ADDRESS variable there; otherwise please\n\
specify an --msgid-bugs-address command line option.\n\
")));

  time (&now);
  timestring = po_strftime (&now);

  msgstr = xasprintf ("\
Project-Id-Version: PACKAGE VERSION\n\
Report-Msgid-Bugs-To: %s\n\
POT-Creation-Date: %s\n\
PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n\
Last-Translator: FULL NAME <EMAIL@ADDRESS>\n\
Language-Team: LANGUAGE <LL@li.org>\n\
MIME-Version: 1.0\n\
Content-Type: text/plain; charset=CHARSET\n\
Content-Transfer-Encoding: 8bit\n",
		      msgid_bugs_address != NULL ? msgid_bugs_address : "",
		      timestring);
  free (timestring);

  mp = message_alloc ("", NULL, msgstr, strlen (msgstr) + 1, &pos);

  message_comment_append (mp,
			  copyright_holder[0] != '\0'
			  ? xasprintf ("\
SOME DESCRIPTIVE TITLE.\n\
Copyright (C) YEAR %s\n\
This file is distributed under the same license as the PACKAGE package.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n",
				       copyright_holder)
			  : "\
SOME DESCRIPTIVE TITLE.\n\
This file is put in the public domain.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n");

  mp->is_fuzzy = true;

  return mp;
}

static void
finalize_header (msgdomain_list_ty *mdlp)
{
  /* If the generated PO file has plural forms, add a Plural-Forms template
     to the constructed header.  */
  {
    bool has_plural;
    size_t i, j;

    has_plural = false;
    for (i = 0; i < mdlp->nitems; i++)
      {
	message_list_ty *mlp = mdlp->item[i]->messages;

	for (j = 0; j < mlp->nitems; j++)
	  {
	    message_ty *mp = mlp->item[j];

	    if (mp->msgid_plural != NULL)
	      {
		has_plural = true;
		break;
	      }
	  }
	if (has_plural)
	  break;
      }

    if (has_plural)
      {
	message_ty *header = message_list_search (mdlp->item[0]->messages, "");
	if (header != NULL
	    && strstr (header->msgstr, "Plural-Forms:") == NULL)
	  {
	    size_t insertpos = strlen (header->msgstr);
	    const char *suffix;
	    size_t suffix_len;
	    char *new_msgstr;

	    suffix = "\nPlural-Forms: nplurals=INTEGER; plural=EXPRESSION;\n";
	    if (insertpos == 0 || header->msgstr[insertpos-1] == '\n')
	      suffix++;
	    suffix_len = strlen (suffix);
	    new_msgstr = (char *) xmalloc (header->msgstr_len + suffix_len);
	    memcpy (new_msgstr, header->msgstr, insertpos);
	    memcpy (new_msgstr + insertpos, suffix, suffix_len);
	    memcpy (new_msgstr + insertpos + suffix_len,
		    header->msgstr + insertpos,
		    header->msgstr_len - insertpos);
	    header->msgstr = new_msgstr;
	    header->msgstr_len = header->msgstr_len + suffix_len;
	  }
      }
  }

  /* If not all the strings were plain ASCII, or if the output syntax
     requires a charset conversion, set the charset in the header to UTF-8.
     All messages have already been converted to UTF-8 in remember_a_message
     and remember_a_message_plural.  */
  {
    bool has_nonascii = false;
    size_t i;

    for (i = 0; i < mdlp->nitems; i++)
      {
	message_list_ty *mlp = mdlp->item[i]->messages;

	if (!is_ascii_message_list (mlp))
	  has_nonascii = true;
      }

    if (has_nonascii
	|| output_syntax == syntax_properties
	|| output_syntax == syntax_stringtable)
      {
	message_list_ty *mlp = mdlp->item[0]->messages;

	iconv_message_list (mlp, po_charset_utf8, po_charset_utf8, NULL);
      }
  }
}


#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))
#define ENDOF(a) ((a) + SIZEOF(a))


static extractor_ty
language_to_extractor (const char *name)
{
  struct table_ty
  {
    const char *name;
    extractor_func func;
    flag_context_list_table_ty *flag_table;
    struct formatstring_parser *formatstring_parser1;
    struct formatstring_parser *formatstring_parser2;
  };
  typedef struct table_ty table_ty;

  static table_ty table[] =
  {
    SCANNERS_C
    SCANNERS_PO
    SCANNERS_SH
    SCANNERS_PYTHON
    SCANNERS_LISP
    SCANNERS_ELISP
    SCANNERS_LIBREP
    SCANNERS_SCHEME
    SCANNERS_SMALLTALK
    SCANNERS_JAVA
    SCANNERS_PROPERTIES
    SCANNERS_CSHARP
    SCANNERS_AWK
    SCANNERS_YCP
    SCANNERS_TCL
    SCANNERS_PERL
    SCANNERS_PHP
    SCANNERS_STRINGTABLE
    SCANNERS_RST
    SCANNERS_GLADE
    /* Here may follow more languages and their scanners: pike, etc...
       Make sure new scanners honor the --exclude-file option.  */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
    if (c_strcasecmp (name, tp->name) == 0)
      {
	extractor_ty result;

	result.func = tp->func;
	result.flag_table = tp->flag_table;
	result.formatstring_parser1 = tp->formatstring_parser1;
	result.formatstring_parser2 = tp->formatstring_parser2;

	/* Handle --qt.  It's preferrable to handle this facility here rather
	   than through an option --language=C++/Qt because the latter would
	   conflict with the language "C++" regarding the file extensions.  */
	if (recognize_format_qt && strcmp (tp->name, "C++") == 0)
	  result.formatstring_parser2 = &formatstring_qt;

	return result;
      }

  error (EXIT_FAILURE, 0, _("language `%s' unknown"), name);
  /* NOTREACHED */
  {
    extractor_ty result = { NULL, NULL, NULL, NULL };
    return result;
  }
}


static const char *
extension_to_language (const char *extension)
{
  struct table_ty
  {
    const char *extension;
    const char *language;
  };
  typedef struct table_ty table_ty;

  static table_ty table[] =
  {
    EXTENSIONS_C
    EXTENSIONS_PO
    EXTENSIONS_SH
    EXTENSIONS_PYTHON
    EXTENSIONS_LISP
    EXTENSIONS_ELISP
    EXTENSIONS_LIBREP
    EXTENSIONS_SCHEME
    EXTENSIONS_SMALLTALK
    EXTENSIONS_JAVA
    EXTENSIONS_PROPERTIES
    EXTENSIONS_CSHARP
    EXTENSIONS_AWK
    EXTENSIONS_YCP
    EXTENSIONS_TCL
    EXTENSIONS_PERL
    EXTENSIONS_PHP
    EXTENSIONS_STRINGTABLE
    EXTENSIONS_RST
    EXTENSIONS_GLADE
    /* Here may follow more file extensions... */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
    if (strcmp (extension, tp->extension) == 0)
      return tp->language;
  return NULL;
}
