/* Converts Uniforum style .po files to binary .mo files
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

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>

#include "closeout.h"
#include "dir-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "xerror.h"
#include "format.h"
#include "xalloc.h"
#include "plural-exp.h"
#include "plural-table.h"
#include "strstr.h"
#include "stpcpy.h"
#include "exit.h"
#include "msgfmt.h"
#include "write-mo.h"
#include "write-java.h"
#include "write-csharp.h"
#include "write-resources.h"
#include "write-tcl.h"
#include "write-qt.h"

#include "gettext.h"
#include "message.h"
#include "open-po.h"
#include "read-po.h"
#include "po-charset.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

/* Some platforms don't have the sigjmp_buf type in <setjmp.h>.  */
#if defined _MSC_VER || defined __MINGW32__
/* Native Woe32 API.  */
# define sigjmp_buf jmp_buf
# define sigsetjmp(env,savesigs) setjmp (env)
# define siglongjmp longjmp
#endif

/* We use siginfo to get precise information about the signal.
   But siginfo doesn't work on Irix 6.5.  */
#if HAVE_SIGINFO && !defined (__sgi)
# define USE_SIGINFO 1
#endif

/* Contains exit status for case in which no premature exit occurs.  */
static int exit_status;

/* If true include even fuzzy translations in output file.  */
static bool include_all = false;

/* Specifies name of the output file.  */
static const char *output_file_name;

/* Java mode output file specification.  */
static bool java_mode;
static bool assume_java2;
static const char *java_resource_name;
static const char *java_locale_name;
static const char *java_class_directory;

/* C# mode output file specification.  */
static bool csharp_mode;
static const char *csharp_resource_name;
static const char *csharp_locale_name;
static const char *csharp_base_directory;

/* C# resources mode output file specification.  */
static bool csharp_resources_mode;

/* Tcl mode output file specification.  */
static bool tcl_mode;
static const char *tcl_locale_name;
static const char *tcl_base_directory;

/* Qt mode output file specification.  */
static bool qt_mode;

/* We may have more than one input file.  Domains with same names in
   different files have to merged.  So we need a list of tables for
   each output file.  */
struct msg_domain
{
  /* List for mapping message IDs to message strings.  */
  message_list_ty *mlp;
  /* Name of domain these ID/String pairs are part of.  */
  const char *domain_name;
  /* Output file name.  */
  const char *file_name;
  /* Link to the next domain.  */
  struct msg_domain *next;
};
static struct msg_domain *domain_list;
static struct msg_domain *current_domain;

/* Be more verbose.  Use only 'fprintf' and 'multiline_warning' but not
   'error' or 'multiline_error' to emit verbosity messages, because 'error'
   and 'multiline_error' during PO file parsing cause the program to exit
   with EXIT_FAILURE.  See function lex_end().  */
bool verbose = false;

/* If true check strings according to format string rules for the
   language.  */
static bool check_format_strings = false;

/* If true check the header entry is present and complete.  */
static bool check_header = false;

/* Check that domain directives can be satisfied.  */
static bool check_domain = false;

/* Check that msgfmt's behaviour is semantically compatible with
   X/Open msgfmt or XView msgfmt.  */
static bool check_compatibility = false;

/* If true, consider that strings containing an '&' are menu items and
   the '&' designates a keyboard accelerator, and verify that the translations
   also have a keyboard accelerator.  */
static bool check_accelerators = false;
static char accelerator_char = '&';

/* Counters for statistics on translations for the processed files.  */
static int msgs_translated;
static int msgs_untranslated;
static int msgs_fuzzy;

/* If not zero print statistics about translation at the end.  */
static int do_statistics;

/* Long options.  */
static const struct option long_options[] =
{
  { "alignment", required_argument, NULL, 'a' },
  { "check", no_argument, NULL, 'c' },
  { "check-accelerators", optional_argument, NULL, CHAR_MAX + 1 },
  { "check-compatibility", no_argument, NULL, 'C' },
  { "check-domain", no_argument, NULL, CHAR_MAX + 2 },
  { "check-format", no_argument, NULL, CHAR_MAX + 3 },
  { "check-header", no_argument, NULL, CHAR_MAX + 4 },
  { "csharp", no_argument, NULL, CHAR_MAX + 10 },
  { "csharp-resources", no_argument, NULL, CHAR_MAX + 11 },
  { "directory", required_argument, NULL, 'D' },
  { "help", no_argument, NULL, 'h' },
  { "java", no_argument, NULL, 'j' },
  { "java2", no_argument, NULL, CHAR_MAX + 5 },
  { "locale", required_argument, NULL, 'l' },
  { "no-hash", no_argument, NULL, CHAR_MAX + 6 },
  { "output-file", required_argument, NULL, 'o' },
  { "properties-input", no_argument, NULL, 'P' },
  { "qt", no_argument, NULL, CHAR_MAX + 9 },
  { "resource", required_argument, NULL, 'r' },
  { "statistics", no_argument, &do_statistics, 1 },
  { "strict", no_argument, NULL, 'S' },
  { "stringtable-input", no_argument, NULL, CHAR_MAX + 8 },
  { "tcl", no_argument, NULL, CHAR_MAX + 7 },
  { "use-fuzzy", no_argument, NULL, 'f' },
  { "verbose", no_argument, NULL, 'v' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
	__attribute__ ((noreturn))
#endif
;
static const char *add_mo_suffix (const char *);
static struct msg_domain *new_domain (const char *name, const char *file_name);
static bool is_nonobsolete (const message_ty *mp);
static void check_plural (message_list_ty *mlp);
static void read_po_file_msgfmt (char *filename);


int
main (int argc, char *argv[])
{
  int opt;
  bool do_help = false;
  bool do_version = false;
  bool strict_uniforum = false;
  const char *canon_encoding;
  struct msg_domain *domain;

  /* Set default value for global variables.  */
  alignment = DEFAULT_OUTPUT_ALIGNMENT;

  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  error_one_per_line = 1;
  exit_status = EXIT_SUCCESS;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, "a:cCd:D:fhjl:o:Pr:vV", long_options,
			     NULL))
	 != EOF)
    switch (opt)
      {
      case '\0':		/* Long option.  */
	break;
      case 'a':
	{
	  char *endp;
	  size_t new_align = strtoul (optarg, &endp, 0);

	  if (endp != optarg)
	    alignment = new_align;
	}
	break;
      case 'c':
	check_domain = true;
	check_format_strings = true;
	check_header = true;
	break;
      case 'C':
	check_compatibility = true;
	break;
      case 'd':
	java_class_directory = optarg;
	csharp_base_directory = optarg;
	tcl_base_directory = optarg;
	break;
      case 'D':
	dir_list_append (optarg);
	break;
      case 'f':
	include_all = true;
	break;
      case 'h':
	do_help = true;
	break;
      case 'j':
	java_mode = true;
	break;
      case 'l':
	java_locale_name = optarg;
	csharp_locale_name = optarg;
	tcl_locale_name = optarg;
	break;
      case 'o':
	output_file_name = optarg;
	break;
      case 'P':
	input_syntax = syntax_properties;
	break;
      case 'r':
	java_resource_name = optarg;
	csharp_resource_name = optarg;
	break;
      case 'S':
	strict_uniforum = true;
	break;
      case 'v':
	verbose = true;
	break;
      case 'V':
	do_version = true;
	break;
      case CHAR_MAX + 1: /* --check-accelerators */
	check_accelerators = true;
	if (optarg != NULL)
	  {
	    if (optarg[0] != '\0' && ispunct ((unsigned char) optarg[0])
		&& optarg[1] == '\0')
	      accelerator_char = optarg[0];
	    else
	      error (EXIT_FAILURE, 0,
		     _("the argument to %s should be a single punctuation character"),
		     "--check-accelerators");
	  }
	break;
      case CHAR_MAX + 2: /* --check-domain */
	check_domain = true;
	break;
      case CHAR_MAX + 3: /* --check-format */
	check_format_strings = true;
	break;
      case CHAR_MAX + 4: /* --check-header */
	check_header = true;
	break;
      case CHAR_MAX + 5: /* --java2 */
	java_mode = true;
	assume_java2 = true;
	break;
      case CHAR_MAX + 6: /* --no-hash */
	no_hash_table = true;
	break;
      case CHAR_MAX + 7: /* --tcl */
	tcl_mode = true;
	break;
      case CHAR_MAX + 8: /* --stringtable-input */
	input_syntax = syntax_stringtable;
	break;
      case CHAR_MAX + 9: /* --qt */
	qt_mode = true;
	break;
      case CHAR_MAX + 10: /* --csharp */
	csharp_mode = true;
	break;
      case CHAR_MAX + 11: /* --csharp-resources */
	csharp_resources_mode = true;
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
      printf (_("Written by %s.\n"), "Ulrich Drepper");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test whether we have a .po file name as argument.  */
  if (optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input file given"));
      usage (EXIT_FAILURE);
    }

  /* Check for contradicting options.  */
  {
    unsigned int modes =
      (java_mode ? 1 : 0)
      | (csharp_mode ? 2 : 0)
      | (csharp_resources_mode ? 4 : 0)
      | (tcl_mode ? 8 : 0)
      | (qt_mode ? 16 : 0);
    static const char *mode_options[] =
      { "--java", "--csharp", "--csharp-resources", "--tcl", "--qt" };
    /* More than one bit set?  */
    if (modes & (modes - 1))
      {
	const char *first_option;
	const char *second_option;
	unsigned int i;
	for (i = 0; ; i++)
	  if (modes & (1 << i))
	    break;
	first_option = mode_options[i];
	for (i = i + 1; ; i++)
	  if (modes & (1 << i))
	    break;
	second_option = mode_options[i];
	error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
	       first_option, second_option);
      }
  }
  if (java_mode)
    {
      if (output_file_name != NULL)
	{
	  error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
		 "--java", "--output-file");
	}
      if (java_class_directory == NULL)
	{
	  error (EXIT_SUCCESS, 0,
		 _("%s requires a \"-d directory\" specification"),
		 "--java");
	  usage (EXIT_FAILURE);
	}
    }
  else if (csharp_mode)
    {
      if (output_file_name != NULL)
	{
	  error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
		 "--csharp", "--output-file");
	}
      if (csharp_locale_name == NULL)
	{
	  error (EXIT_SUCCESS, 0,
		 _("%s requires a \"-l locale\" specification"),
		 "--csharp");
	  usage (EXIT_FAILURE);
	}
      if (csharp_base_directory == NULL)
	{
	  error (EXIT_SUCCESS, 0,
		 _("%s requires a \"-d directory\" specification"),
		 "--csharp");
	  usage (EXIT_FAILURE);
	}
    }
  else if (tcl_mode)
    {
      if (output_file_name != NULL)
	{
	  error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
		 "--tcl", "--output-file");
	}
      if (tcl_locale_name == NULL)
	{
	  error (EXIT_SUCCESS, 0,
		 _("%s requires a \"-l locale\" specification"),
		 "--tcl");
	  usage (EXIT_FAILURE);
	}
      if (tcl_base_directory == NULL)
	{
	  error (EXIT_SUCCESS, 0,
		 _("%s requires a \"-d directory\" specification"),
		 "--tcl");
	  usage (EXIT_FAILURE);
	}
    }
  else
    {
      if (java_resource_name != NULL)
	{
	  error (EXIT_SUCCESS, 0, _("%s is only valid with %s or %s"),
		 "--resource", "--java", "--csharp");
	  usage (EXIT_FAILURE);
	}
      if (java_locale_name != NULL)
	{
	  error (EXIT_SUCCESS, 0, _("%s is only valid with %s, %s or %s"),
		 "--locale", "--java", "--csharp", "--tcl");
	  usage (EXIT_FAILURE);
	}
      if (java_class_directory != NULL)
	{
	  error (EXIT_SUCCESS, 0, _("%s is only valid with %s, %s or %s"),
		 "-d", "--java", "--csharp", "--tcl");
	  usage (EXIT_FAILURE);
	}
    }

  /* The -o option determines the name of the domain and therefore
     the output file.  */
  if (output_file_name != NULL)
    current_domain =
      new_domain (output_file_name,
		  strict_uniforum && !csharp_resources_mode && !qt_mode
		  ? add_mo_suffix (output_file_name)
		  : output_file_name);

  /* Process all given .po files.  */
  while (argc > optind)
    {
      /* Remember that we currently have not specified any domain.  This
	 is of course not true when we saw the -o option.  */
      if (output_file_name == NULL)
	current_domain = NULL;

      /* And process the input file.  */
      read_po_file_msgfmt (argv[optind]);

      ++optind;
    }

  /* We know a priori that properties_parse() and stringtable_parse() convert
     strings to UTF-8.  */
  canon_encoding =
    (input_syntax == syntax_properties || input_syntax == syntax_stringtable
     ? po_charset_utf8
     : NULL);

  /* Remove obsolete messages.  They were only needed for duplicate
     checking.  */
  for (domain = domain_list; domain != NULL; domain = domain->next)
    message_list_remove_if_not (domain->mlp, is_nonobsolete);

  /* Check the plural expression is present if needed and has valid syntax.  */
  if (check_header)
    for (domain = domain_list; domain != NULL; domain = domain->next)
      check_plural (domain->mlp);

  /* Now write out all domains.  */
  for (domain = domain_list; domain != NULL; domain = domain->next)
    {
      if (java_mode)
	{
	  if (msgdomain_write_java (domain->mlp, canon_encoding,
				    java_resource_name, java_locale_name,
				    java_class_directory, assume_java2))
	    exit_status = EXIT_FAILURE;
	}
      else if (csharp_mode)
	{
	  if (msgdomain_write_csharp (domain->mlp, canon_encoding,
				      csharp_resource_name, csharp_locale_name,
				      csharp_base_directory))
	    exit_status = EXIT_FAILURE;
	}
      else if (csharp_resources_mode)
	{
	  if (msgdomain_write_csharp_resources (domain->mlp, canon_encoding,
						domain->domain_name,
						domain->file_name))
	    exit_status = EXIT_FAILURE;
	}
      else if (tcl_mode)
	{
	  if (msgdomain_write_tcl (domain->mlp, canon_encoding,
				   tcl_locale_name, tcl_base_directory))
	    exit_status = EXIT_FAILURE;
	}
      else if (qt_mode)
	{
	  if (msgdomain_write_qt (domain->mlp, canon_encoding,
				  domain->domain_name, domain->file_name))
	    exit_status = EXIT_FAILURE;
	}
      else
	{
	  if (msgdomain_write_mo (domain->mlp, domain->domain_name,
				  domain->file_name))
	    exit_status = EXIT_FAILURE;
	}

      /* List is not used anymore.  */
      message_list_free (domain->mlp);
    }

  /* Print statistics if requested.  */
  if (verbose || do_statistics)
    {
      fprintf (stderr,
	       ngettext ("%d translated message", "%d translated messages",
			 msgs_translated),
	       msgs_translated);
      if (msgs_fuzzy > 0)
	fprintf (stderr,
		 ngettext (", %d fuzzy translation", ", %d fuzzy translations",
			   msgs_fuzzy),
		 msgs_fuzzy);
      if (msgs_untranslated > 0)
	fprintf (stderr,
		 ngettext (", %d untranslated message",
			   ", %d untranslated messages",
			   msgs_untranslated),
		 msgs_untranslated);
      fputs (".\n", stderr);
    }

  exit (exit_status);
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
Usage: %s [OPTION] filename.po ...\n\
"), program_name);
      printf ("\n");
      printf (_("\
Generate binary message catalog from textual translation description.\n\
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
  filename.po ...             input files\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If input file is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -j, --java                  Java mode: generate a Java ResourceBundle class\n"));
      printf (_("\
      --java2                 like --java, and assume Java2 (JDK 1.2 or higher)\n"));
      printf (_("\
      --csharp                C# mode: generate a .NET .dll file\n"));
      printf (_("\
      --csharp-resources      C# resources mode: generate a .NET .resources file\n"));
      printf (_("\
      --tcl                   Tcl mode: generate a tcl/msgcat .msg file\n"));
      printf (_("\
      --qt                    Qt mode: generate a Qt .qm file\n"));
      printf ("\n");
      printf (_("\
Output file location:\n"));
      printf (_("\
  -o, --output-file=FILE      write output to specified file\n"));
      printf (_("\
      --strict                enable strict Uniforum mode\n"));
      printf (_("\
If output file is -, output is written to standard output.\n"));
      printf ("\n");
      printf (_("\
Output file location in Java mode:\n"));
      printf (_("\
  -r, --resource=RESOURCE     resource name\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
  -d DIRECTORY                base directory of classes directory hierarchy\n"));
      printf (_("\
The class name is determined by appending the locale name to the resource name,\n\
separated with an underscore.  The -d option is mandatory.  The class is\n\
written under the specified directory.\n\
"));
      printf ("\n");
      printf (_("\
Output file location in C# mode:\n"));
      printf (_("\
  -r, --resource=RESOURCE     resource name\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
  -d DIRECTORY                base directory for locale dependent .dll files\n"));
      printf (_("\
The -l and -d options are mandatory.  The .dll file is written in a\n\
subdirectory of the specified directory whose name depends on the locale.\n"));
      printf ("\n");
      printf (_("\
Output file location in Tcl mode:\n"));
      printf (_("\
  -l, --locale=LOCALE         locale name, either language or language_COUNTRY\n"));
      printf (_("\
  -d DIRECTORY                base directory of .msg message catalogs\n"));
      printf (_("\
The -l and -d options are mandatory.  The .msg file is written in the\n\
specified directory.\n"));
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
Input file interpretation:\n"));
      printf (_("\
  -c, --check                 perform all the checks implied by\n\
                                --check-format, --check-header, --check-domain\n"));
      printf (_("\
      --check-format          check language dependent format strings\n"));
      printf (_("\
      --check-header          verify presence and contents of the header entry\n"));
      printf (_("\
      --check-domain          check for conflicts between domain directives\n\
                                and the --output-file option\n"));
      printf (_("\
  -C, --check-compatibility   check that GNU msgfmt behaves like X/Open msgfmt\n"));
      printf (_("\
      --check-accelerators[=CHAR]  check presence of keyboard accelerators for\n\
                                menu items\n"));
      printf (_("\
  -f, --use-fuzzy             use fuzzy entries in output\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
  -a, --alignment=NUMBER      align strings to NUMBER bytes (default: %d)\n"), DEFAULT_OUTPUT_ALIGNMENT);
      printf (_("\
      --no-hash               binary file will not include the hash table\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf (_("\
      --statistics            print statistics about translations\n"));
      printf (_("\
  -v, --verbose               increase verbosity level\n"));
      printf ("\n");
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"), stdout);
    }

  exit (status);
}


static const char *
add_mo_suffix (const char *fname)
{
  size_t len;
  char *result;

  len = strlen (fname);
  if (len > 3 && memcmp (fname + len - 3, ".mo", 3) == 0)
    return fname;
  if (len > 4 && memcmp (fname + len - 4, ".gmo", 4) == 0)
    return fname;
  result = (char *) xmalloc (len + 4);
  stpcpy (stpcpy (result, fname), ".mo");
  return result;
}


static struct msg_domain *
new_domain (const char *name, const char *file_name)
{
  struct msg_domain **p_dom = &domain_list;

  while (*p_dom != NULL && strcmp (name, (*p_dom)->domain_name) != 0)
    p_dom = &(*p_dom)->next;

  if (*p_dom == NULL)
    {
      struct msg_domain *domain;

      domain = (struct msg_domain *) xmalloc (sizeof (struct msg_domain));
      domain->mlp = message_list_alloc (true);
      domain->domain_name = name;
      domain->file_name = file_name;
      domain->next = NULL;
      *p_dom = domain;
    }

  return *p_dom;
}


static bool
is_nonobsolete (const message_ty *mp)
{
  return !mp->obsolete;
}


static sigjmp_buf sigfpe_exit;

#if USE_SIGINFO

static int sigfpe_code;

/* Signal handler called in case of arithmetic exception (e.g. division
   by zero) during plural_eval.  */
static void
sigfpe_handler (int sig, siginfo_t *sip, void *scp)
{
  sigfpe_code = sip->si_code;
  siglongjmp (sigfpe_exit, 1);
}

#else

/* Signal handler called in case of arithmetic exception (e.g. division
   by zero) during plural_eval.  */
static void
sigfpe_handler (int sig)
{
  siglongjmp (sigfpe_exit, 1);
}

#endif

static void
install_sigfpe_handler ()
{
#if USE_SIGINFO
  struct sigaction action;
  action.sa_sigaction = sigfpe_handler;
  action.sa_flags = SA_SIGINFO;
  sigemptyset (&action.sa_mask);
  sigaction (SIGFPE, &action, (struct sigaction *) NULL);
#else
  signal (SIGFPE, sigfpe_handler);
#endif
}

static void
uninstall_sigfpe_handler ()
{
#if USE_SIGINFO
  struct sigaction action;
  action.sa_handler = SIG_DFL;
  action.sa_flags = 0;
  sigemptyset (&action.sa_mask);
  sigaction (SIGFPE, &action, (struct sigaction *) NULL);
#else
  signal (SIGFPE, SIG_DFL);
#endif
}

/* Check the values returned by plural_eval.  */
static void
check_plural_eval (struct expression *plural_expr,
		   unsigned long nplurals_value,
		   const lex_pos_ty *header_pos)
{
  if (sigsetjmp (sigfpe_exit, 1) == 0)
    {
      unsigned long n;

      /* Protect against arithmetic exceptions.  */
      install_sigfpe_handler ();

      for (n = 0; n <= 1000; n++)
	{
	  unsigned long val = plural_eval (plural_expr, n);

	  if ((long) val < 0)
	    {
	      /* End of protection against arithmetic exceptions.  */
	      uninstall_sigfpe_handler ();

	      error_with_progname = false;
	      error_at_line (0, 0,
			     header_pos->file_name, header_pos->line_number,
			     _("plural expression can produce negative values"));
	      error_with_progname = true;
	      exit_status = EXIT_FAILURE;
	      return;
	    }
	  else if (val >= nplurals_value)
	    {
	      /* End of protection against arithmetic exceptions.  */
	      uninstall_sigfpe_handler ();

	      error_with_progname = false;
	      error_at_line (0, 0,
			     header_pos->file_name, header_pos->line_number,
			     _("nplurals = %lu but plural expression can produce values as large as %lu"),
			     nplurals_value, val);
	      error_with_progname = true;
	      exit_status = EXIT_FAILURE;
	      return;
	    }
	}

      /* End of protection against arithmetic exceptions.  */
      uninstall_sigfpe_handler ();
    }
  else
    {
      /* Caught an arithmetic exception.  */
      const char *msg;

      /* End of protection against arithmetic exceptions.  */
      uninstall_sigfpe_handler ();

#if USE_SIGINFO
      switch (sigfpe_code)
#endif
	{
#if USE_SIGINFO
# ifdef FPE_INTDIV
	case FPE_INTDIV:
	  /* xgettext: c-format */
	  msg = _("plural expression can produce division by zero");
	  break;
# endif
# ifdef FPE_INTOVF
	case FPE_INTOVF:
	  /* xgettext: c-format */
	  msg = _("plural expression can produce integer overflow");
	  break;
# endif
	default:
#endif
	  /* xgettext: c-format */
	  msg = _("plural expression can produce arithmetic exceptions, possibly division by zero");
	}

      error_with_progname = false;
      error_at_line (0, 0, header_pos->file_name, header_pos->line_number, msg);
      error_with_progname = true;
      exit_status = EXIT_FAILURE;
    }
}


/* Perform plural expression checking.  */
static void
check_plural (message_list_ty *mlp)
{
  const lex_pos_ty *has_plural;
  unsigned long min_nplurals;
  const lex_pos_ty *min_pos;
  unsigned long max_nplurals;
  const lex_pos_ty *max_pos;
  size_t j;
  message_ty *header;

  /* Determine whether mlp has plural entries.  */
  has_plural = NULL;
  min_nplurals = ULONG_MAX;
  min_pos = NULL;
  max_nplurals = 0;
  max_pos = NULL;
  for (j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (mp->msgid_plural != NULL)
	{
	  const char *p;
	  const char *p_end;
	  unsigned long n;

	  if (has_plural == NULL)
	    has_plural = &mp->pos;

	  n = 0;
	  for (p = mp->msgstr, p_end = p + mp->msgstr_len;
	       p < p_end;
	       p += strlen (p) + 1)
	    n++;
	  if (min_nplurals > n)
	    {
	      min_nplurals = n;
	      min_pos = &mp->pos;
	    }
	  if (max_nplurals > n)
	    {
	      max_nplurals = n;
	      min_pos = &mp->pos;
	    }
	}
    }

  /* Look at the plural entry for this domain.
     Cf, function extract_plural_expression.  */
  header = message_list_search (mlp, "");
  if (header != NULL)
    {
      const char *nullentry;
      const char *plural;
      const char *nplurals;
      bool try_to_help = false;

      nullentry = header->msgstr;

      plural = strstr (nullentry, "plural=");
      nplurals = strstr (nullentry, "nplurals=");
      if (plural == NULL && has_plural != NULL)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, has_plural->file_name, has_plural->line_number,
			 _("message catalog has plural form translations..."));
	  --error_message_count;
	  error_at_line (0, 0, header->pos.file_name, header->pos.line_number,
			 _("...but header entry lacks a \"plural=EXPRESSION\" attribute"));
	  error_with_progname = true;
	  try_to_help = true;
	  exit_status = EXIT_FAILURE;
	}
      if (nplurals == NULL && has_plural != NULL)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, has_plural->file_name, has_plural->line_number,
			 _("message catalog has plural form translations..."));
	  --error_message_count;
	  error_at_line (0, 0, header->pos.file_name, header->pos.line_number,
			 _("...but header entry lacks a \"nplurals=INTEGER\" attribute"));
	  error_with_progname = true;
	  try_to_help = true;
	  exit_status = EXIT_FAILURE;
	}
      if (plural != NULL && nplurals != NULL)
	{
	  const char *endp;
	  unsigned long int nplurals_value;
	  struct parse_args args;
	  struct expression *plural_expr;

	  /* First check the number.  */
	  nplurals += 9;
	  while (*nplurals != '\0' && isspace ((unsigned char) *nplurals))
	    ++nplurals;
	  endp = nplurals;
	  nplurals_value = 0;
	  if (*nplurals >= '0' && *nplurals <= '9')
	    nplurals_value = strtoul (nplurals, (char **) &endp, 10);
	  if (nplurals == endp)
	    {
	      error_with_progname = false;
	      error_at_line (0, 0,
			     header->pos.file_name, header->pos.line_number,
			     _("invalid nplurals value"));
	      error_with_progname = true;
	      try_to_help = true;
	      exit_status = EXIT_FAILURE;
	    }

	  /* Then check the expression.  */
	  plural += 7;
	  args.cp = plural;
	  if (parse_plural_expression (&args) != 0)
	    {
	      error_with_progname = false;
	      error_at_line (0, 0,
			     header->pos.file_name, header->pos.line_number,
			     _("invalid plural expression"));
	      error_with_progname = true;
	      try_to_help = true;
	      exit_status = EXIT_FAILURE;
	    }
	  plural_expr = args.res;

	  /* See whether nplurals and plural fit together.  */
	  if (exit_status != EXIT_FAILURE)
	    check_plural_eval (plural_expr, nplurals_value, &header->pos);

	  /* Check the number of plurals of the translations.  */
	  if (exit_status != EXIT_FAILURE)
	    {
	      if (min_nplurals < nplurals_value)
		{
		  error_with_progname = false;
		  error_at_line (0, 0,
				 header->pos.file_name, header->pos.line_number,
				 _("nplurals = %lu..."), nplurals_value);
		  --error_message_count;
		  error_at_line (0, 0, min_pos->file_name, min_pos->line_number,
				 ngettext ("...but some messages have only one plural form",
					   "...but some messages have only %lu plural forms",
					   min_nplurals),
				 min_nplurals);
		  error_with_progname = true;
		  exit_status = EXIT_FAILURE;
		}
	      else if (max_nplurals > nplurals_value)
		{
		  error_with_progname = false;
		  error_at_line (0, 0,
				 header->pos.file_name, header->pos.line_number,
				 _("nplurals = %lu..."), nplurals_value);
		  --error_message_count;
		  error_at_line (0, 0, max_pos->file_name, max_pos->line_number,
				 ngettext ("...but some messages have one plural form",
					   "...but some messages have %lu plural forms",
					   max_nplurals),
				 max_nplurals);
		  error_with_progname = true;
		  exit_status = EXIT_FAILURE;
		}
	      /* The only valid case is max_nplurals <= n <= min_nplurals,
		 which means either has_plural == NULL or
		 max_nplurals = n = min_nplurals.  */
	    }
	}
      /* Try to help the translator by looking up the right plural formula
	 for her.  */
      if (try_to_help)
	{
	  const char *language;

	  language = strstr (nullentry, "Language-Team: ");
	  if (language != NULL)
	    {
	      language += 15;
	      for (j = 0; j < plural_table_size; j++)
		if (strncmp (language,
			     plural_table[j].language,
			     strlen (plural_table[j].language)) == 0)
		  {
		    char *recommended =
		      xasprintf ("Plural-Forms: %s\\n", plural_table[j].value);
		    fprintf (stderr,
			     _("Try using the following, valid for %s:\n"),
			     plural_table[j].language);
		    fprintf (stderr, "\"%s\"\n", recommended);
		    free (recommended);
		    break;
		  }
	    }
	}
    }
  else if (has_plural != NULL)
    {
      error_with_progname = false;
      error_at_line (0, 0, has_plural->file_name, has_plural->line_number,
		     _("message catalog has plural form translations, but lacks a header entry with \"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\""));
      error_with_progname = true;
      exit_status = EXIT_FAILURE;
    }
}


/* Signal an error when checking format strings.  */
static lex_pos_ty curr_msgid_pos;
static void
formatstring_error_logger (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  fprintf (stderr, "%s:%lu: ",
	   curr_msgid_pos.file_name,
	   (unsigned long) curr_msgid_pos.line_number);
  vfprintf (stderr, format, args);
  putc ('\n', stderr);
  fflush (stderr);
  va_end (args);
  ++error_message_count;
}


/* Perform miscellaneous checks on a message.  */
static void
check_pair (const char *msgid,
	    const lex_pos_ty *msgid_pos,
	    const char *msgid_plural,
	    const char *msgstr, size_t msgstr_len,
	    const lex_pos_ty *msgstr_pos, enum is_format is_format[NFORMATS])
{
  int has_newline;
  unsigned int j;
  const char *p;

  /* If the msgid string is empty we have the special entry reserved for
     information about the translation.  */
  if (msgid[0] == '\0')
    return;

  /* Test 1: check whether all or none of the strings begin with a '\n'.  */
  has_newline = (msgid[0] == '\n');
#define TEST_NEWLINE(p) (p[0] == '\n')
  if (msgid_plural != NULL)
    {
      if (TEST_NEWLINE(msgid_plural) != has_newline)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			 _("\
`msgid' and `msgid_plural' entries do not both begin with '\\n'"));
	  error_with_progname = true;
	  exit_status = EXIT_FAILURE;
	}
      for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
	if (TEST_NEWLINE(p) != has_newline)
	  {
	    error_with_progname = false;
	    error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			   _("\
`msgid' and `msgstr[%u]' entries do not both begin with '\\n'"), j);
	    error_with_progname = true;
	    exit_status = EXIT_FAILURE;
	  }
    }
  else
    {
      if (TEST_NEWLINE(msgstr) != has_newline)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			 _("\
`msgid' and `msgstr' entries do not both begin with '\\n'"));
	  error_with_progname = true;
	  exit_status = EXIT_FAILURE;
	}
    }
#undef TEST_NEWLINE

  /* Test 2: check whether all or none of the strings end with a '\n'.  */
  has_newline = (msgid[strlen (msgid) - 1] == '\n');
#define TEST_NEWLINE(p) (p[0] != '\0' && p[strlen (p) - 1] == '\n')
  if (msgid_plural != NULL)
    {
      if (TEST_NEWLINE(msgid_plural) != has_newline)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			 _("\
`msgid' and `msgid_plural' entries do not both end with '\\n'"));
	  error_with_progname = true;
	  exit_status = EXIT_FAILURE;
	}
      for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
	if (TEST_NEWLINE(p) != has_newline)
	  {
	    error_with_progname = false;
	    error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			   _("\
`msgid' and `msgstr[%u]' entries do not both end with '\\n'"), j);
	    error_with_progname = true;
	    exit_status = EXIT_FAILURE;
	  }
    }
  else
    {
      if (TEST_NEWLINE(msgstr) != has_newline)
	{
	  error_with_progname = false;
	  error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			 _("\
`msgid' and `msgstr' entries do not both end with '\\n'"));
	  error_with_progname = true;
	  exit_status = EXIT_FAILURE;
	}
    }
#undef TEST_NEWLINE

  if (check_compatibility && msgid_plural != NULL)
    {
      error_with_progname = false;
      error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
		     _("plural handling is a GNU gettext extension"));
      error_with_progname = true;
      exit_status = EXIT_FAILURE;
    }

  if (check_format_strings)
    /* Test 3: Check whether both formats strings contain the same number
       of format specifications.  */
    {
      curr_msgid_pos = *msgid_pos;
      if (check_msgid_msgstr_format (msgid, msgid_plural, msgstr, msgstr_len,
				     is_format, formatstring_error_logger))
	exit_status = EXIT_FAILURE;
    }

  if (check_accelerators && msgid_plural == NULL)
    /* Test 4: Check that if msgid is a menu item with a keyboard accelerator,
       the msgstr has an accelerator as well.  A keyboard accelerator is
       designated by an immediately preceding '&'.  We cannot check whether
       two accelerators collide, only whether the translator has bothered
       thinking about them.  */
    {
      const char *p;

      /* We are only interested in msgids that contain exactly one '&'.  */
      p = strchr (msgid, accelerator_char);
      if (p != NULL && strchr (p + 1, accelerator_char) == NULL)
	{
	  /* Count the number of '&' in msgstr, but ignore '&&'.  */
	  unsigned int count = 0;

	  for (p = msgstr; (p = strchr (p, accelerator_char)) != NULL; p++)
	    if (p[1] == accelerator_char)
	      p++;
	    else
	      count++;

	  if (count == 0)
	    {
	      error_with_progname = false;
	      error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			     _("msgstr lacks the keyboard accelerator mark '%c'"),
			     accelerator_char);
	      error_with_progname = true;
	    }
	  else if (count > 1)
	    {
	      error_with_progname = false;
	      error_at_line (0, 0, msgid_pos->file_name, msgid_pos->line_number,
			     _("msgstr has too many keyboard accelerator marks '%c'"),
			     accelerator_char);
	      error_with_progname = true;
	    }
	}
    }
}


/* Perform miscellaneous checks on a header entry.  */
static void
check_header_entry (const char *msgstr_string)
{
  static const char *required_fields[] =
  {
    "Project-Id-Version", "PO-Revision-Date", "Last-Translator",
    "Language-Team", "MIME-Version", "Content-Type",
    "Content-Transfer-Encoding"
  };
  static const char *default_values[] =
  {
    "PACKAGE VERSION", "YEAR-MO-DA", "FULL NAME", "LANGUAGE", NULL,
    "text/plain; charset=CHARSET", "ENCODING"
  };
  const size_t nfields = SIZEOF (required_fields);
  int initial = -1;
  int cnt;

  for (cnt = 0; cnt < nfields; ++cnt)
    {
      char *endp = strstr (msgstr_string, required_fields[cnt]);

      if (endp == NULL)
	multiline_error (xasprintf ("%s: ", gram_pos.file_name),
			 xasprintf (_("headerfield `%s' missing in header\n"),
				    required_fields[cnt]));
      else if (endp != msgstr_string && endp[-1] != '\n')
	multiline_error (xasprintf ("%s: ", gram_pos.file_name),
			 xasprintf (_("\
header field `%s' should start at beginning of line\n"),
				    required_fields[cnt]));
      else if (default_values[cnt] != NULL
	       && strncmp (default_values[cnt],
			   endp + strlen (required_fields[cnt]) + 2,
			   strlen (default_values[cnt])) == 0)
	{
	  if (initial != -1)
	    {
	      multiline_error (xasprintf ("%s: ", gram_pos.file_name),
			       xstrdup (_("\
some header fields still have the initial default value\n")));
	      initial = -1;
	      break;
	    }
	  else
	    initial = cnt;
	}
    }

  if (initial != -1)
    multiline_error (xasprintf ("%s: ", gram_pos.file_name),
		     xasprintf (_("\
field `%s' still has initial default value\n"),
				required_fields[initial]));
}


/* The rest of the file defines a subclass msgfmt_po_reader_ty of
   default_po_reader_ty.  Its particularities are:
   - The header entry check is performed on-the-fly.
   - Comments are not stored, they are discarded right away.
     (This is achieved by setting handle_comments = false and
     handle_filepos_comments = false.)
   - The multi-domain handling is adapted to our domain_list.
 */


/* This structure defines a derived class of the default_po_reader_ty class.
   (See read-po-abstract.h for an explanation.)  */
typedef struct msgfmt_po_reader_ty msgfmt_po_reader_ty;
struct msgfmt_po_reader_ty
{
  /* inherited instance variables, etc */
  DEFAULT_PO_READER_TY

  bool has_header_entry;
  bool has_nonfuzzy_header_entry;
};


/* Prepare for first message.  */
static void
msgfmt_constructor (abstract_po_reader_ty *that)
{
  msgfmt_po_reader_ty *this = (msgfmt_po_reader_ty *) that;

  /* Invoke superclass constructor.  */
  default_constructor (that);

  this->has_header_entry = false;
  this->has_nonfuzzy_header_entry = false;
}


/* Some checks after whole file is read.  */
static void
msgfmt_parse_debrief (abstract_po_reader_ty *that)
{
  msgfmt_po_reader_ty *this = (msgfmt_po_reader_ty *) that;

  /* Invoke superclass method.  */
  default_parse_debrief (that);

  /* Test whether header entry was found.  */
  if (check_header)
    {
      if (!this->has_header_entry)
	{
	  multiline_error (xasprintf ("%s: ", gram_pos.file_name),
			   xasprintf (_("\
warning: PO file header missing or invalid\n")));
	  multiline_error (NULL,
			   xasprintf (_("\
warning: charset conversion will not work\n")));
	}
      else if (!this->has_nonfuzzy_header_entry)
	{
	  /* Has only a fuzzy header entry.  Since the versions 0.10.xx
	     ignore a fuzzy header entry and even give an error on it, we
	     give a warning, to increase operability with these older
	     msgfmt versions.  This warning can go away in January 2003.  */
	  multiline_warning (xasprintf ("%s: ", gram_pos.file_name),
			     xasprintf (_("warning: PO file header fuzzy\n")));
	  multiline_warning (NULL,
			     xasprintf (_("\
warning: older versions of msgfmt will give an error on this\n")));
	}
    }
}


/* Set 'domain' directive when seen in .po file.  */
static void
msgfmt_set_domain (default_po_reader_ty *this, char *name)
{
  /* If no output file was given, we change it with each `domain'
     directive.  */
  if (!java_mode && !csharp_mode && !csharp_resources_mode && !tcl_mode
      && !qt_mode && output_file_name == NULL)
    {
      size_t correct;

      correct = strcspn (name, INVALID_PATH_CHAR);
      if (name[correct] != '\0')
	{
	  exit_status = EXIT_FAILURE;
	  if (correct == 0)
	    {
	      error (0, 0, _("\
domain name \"%s\" not suitable as file name"), name);
	      return;
	    }
	  else
	    error (0, 0, _("\
domain name \"%s\" not suitable as file name: will use prefix"), name);
	  name[correct] = '\0';
	}

      /* Set new domain.  */
      current_domain = new_domain (name, add_mo_suffix (name));
      this->domain = current_domain->domain_name;
      this->mlp = current_domain->mlp;
    }
  else
    {
      if (check_domain)
	po_gram_error_at_line (&gram_pos,
			       _("`domain %s' directive ignored"), name);

      /* NAME was allocated in po-gram-gen.y but is not used anywhere.  */
      free (name);
    }
}


static void
msgfmt_add_message (default_po_reader_ty *this,
		    char *msgid,
		    lex_pos_ty *msgid_pos,
		    char *msgid_plural,
		    char *msgstr, size_t msgstr_len,
		    lex_pos_ty *msgstr_pos,
		    bool force_fuzzy, bool obsolete)
{
  /* Check whether already a domain is specified.  If not, use default
     domain.  */
  if (current_domain == NULL)
    {
      current_domain = new_domain (MESSAGE_DOMAIN_DEFAULT,
				   add_mo_suffix (MESSAGE_DOMAIN_DEFAULT));
      /* Keep current_domain and this->domain synchronized.  */
      this->domain = current_domain->domain_name;
      this->mlp = current_domain->mlp;
    }

  /* Invoke superclass method.  */
  default_add_message (this, msgid, msgid_pos, msgid_plural,
		       msgstr, msgstr_len, msgstr_pos, force_fuzzy, obsolete);
}


static void
msgfmt_frob_new_message (default_po_reader_ty *that, message_ty *mp,
			 const lex_pos_ty *msgid_pos,
			 const lex_pos_ty *msgstr_pos)
{
  msgfmt_po_reader_ty *this = (msgfmt_po_reader_ty *) that;

  if (!mp->obsolete)
    {
      /* Don't emit untranslated entries.
	 Also don't emit fuzzy entries, unless --use-fuzzy was specified.
	 But ignore fuzziness of the header entry.  */
      if (mp->msgstr[0] == '\0'
	  || (!include_all && mp->is_fuzzy && mp->msgid[0] != '\0'))
	{
	  if (check_compatibility)
	    {
	      error_with_progname = false;
	      error_at_line (0, 0, mp->pos.file_name, mp->pos.line_number,
			     (mp->msgstr[0] == '\0'
			      ? _("empty `msgstr' entry ignored")
			      : _("fuzzy `msgstr' entry ignored")));
	      error_with_progname = true;
	    }

	  /* Increment counter for fuzzy/untranslated messages.  */
	  if (mp->msgstr[0] == '\0')
	    ++msgs_untranslated;
	  else
	    ++msgs_fuzzy;

	  mp->obsolete = true;
	}
      else
	{
	  /* Test for header entry.  */
	  if (mp->msgid[0] == '\0')
	    {
	      this->has_header_entry = true;
	      if (!mp->is_fuzzy)
		this->has_nonfuzzy_header_entry = true;

	      /* Do some more tests on the contents of the header entry.  */
	      if (check_header)
		check_header_entry (mp->msgstr);
	    }
	  else
	    /* We don't count the header entry in the statistic so place
	       the counter incrementation here.  */
	    if (mp->is_fuzzy)
	      ++msgs_fuzzy;
	    else
	      ++msgs_translated;

	  /* Do some more checks on both strings.  */
	  check_pair (mp->msgid, msgid_pos, mp->msgid_plural,
		      mp->msgstr, mp->msgstr_len, msgstr_pos,
		      mp->is_format);
	}
    }
}


/* Test for `#, fuzzy' comments and warn.  */
static void
msgfmt_comment_special (abstract_po_reader_ty *that, const char *s)
{
  msgfmt_po_reader_ty *this = (msgfmt_po_reader_ty *) that;

  /* Invoke superclass method.  */
  default_comment_special (that, s);

  if (this->is_fuzzy)
    {
      static bool warned = false;

      if (!include_all && check_compatibility && !warned)
	{
	  warned = true;
	  error (0, 0, _("\
%s: warning: source file contains fuzzy translation"),
		 gram_pos.file_name);
	}
    }
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static default_po_reader_class_ty msgfmt_methods =
{
  {
    sizeof (msgfmt_po_reader_ty),
    msgfmt_constructor,
    default_destructor,
    default_parse_brief,
    msgfmt_parse_debrief,
    default_directive_domain,
    default_directive_message,
    default_comment,
    default_comment_dot,
    default_comment_filepos,
    msgfmt_comment_special
  },
  msgfmt_set_domain, /* set_domain */
  msgfmt_add_message, /* add_message */
  msgfmt_frob_new_message /* frob_new_message */
};


/* Read .po file FILENAME and store translation pairs.  */
static void
read_po_file_msgfmt (char *filename)
{
  char *real_filename;
  FILE *fp = open_po_file (filename, &real_filename, true);
  default_po_reader_ty *pop;

  pop = default_po_reader_alloc (&msgfmt_methods);
  pop->handle_comments = false;
  pop->handle_filepos_comments = false;
  pop->allow_domain_directives = true;
  pop->allow_duplicates = false;
  pop->allow_duplicates_if_same_msgstr = false;
  pop->mdlp = NULL;
  pop->mlp = NULL;
  if (current_domain != NULL)
    {
      /* Keep current_domain and this->domain synchronized.  */
      pop->domain = current_domain->domain_name;
      pop->mlp = current_domain->mlp;
    }
  po_lex_pass_obsolete_entries (true);
  po_scan ((abstract_po_reader_ty *) pop, fp, real_filename, filename,
	   input_syntax);
  po_reader_free ((abstract_po_reader_ty *) pop);

  if (fp != stdin)
    fclose (fp);
}
