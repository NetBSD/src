/* Pass translations to a subprocess.
   Copyright (C) 2001-2005 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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
# include "config.h"
#endif

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "closeout.h"
#include "dir-list.h"
#include "error.h"
#include "xerror.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "message.h"
#include "read-po.h"
#include "xalloc.h"
#include "exit.h"
#include "full-write.h"
#include "findprog.h"
#include "pipe.h"
#include "wait-process.h"
#include "xsetenv.h"
#include "gettext.h"

#define _(str) gettext (str)

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif


/* Name of the subprogram.  */
static const char *sub_name;

/* Pathname of the subprogram.  */
static const char *sub_path;

/* Argument list for the subprogram.  */
static char **sub_argv;
static int sub_argc;

/* Maximum exit code encountered.  */
static int exitcode;

/* Long options.  */
static const struct option long_options[] =
{
  { "directory", required_argument, NULL, 'D' },
  { "help", no_argument, NULL, 'h' },
  { "input", required_argument, NULL, 'i' },
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
static void process_msgdomain_list (const msgdomain_list_ty *mdlp);


int
main (int argc, char **argv)
{
  int opt;
  bool do_help;
  bool do_version;
  const char *input_file;
  msgdomain_list_ty *result;
  size_t i;

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
  do_help = false;
  do_version = false;
  input_file = NULL;

  /* The '+' in the options string causes option parsing to terminate when
     the first non-option, i.e. the subprogram name, is encountered.  */
  while ((opt = getopt_long (argc, argv, "+D:hi:PV", long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case '\0':		/* Long option.  */
	break;

      case 'D':
	dir_list_append (optarg);
	break;

      case 'h':
	do_help = true;
	break;

      case 'i':
	if (input_file != NULL)
	  {
	    error (EXIT_SUCCESS, 0, _("at most one input file allowed"));
	    usage (EXIT_FAILURE);
	  }
	input_file = optarg;
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
	      "2001-2005");
      printf (_("Written by %s.\n"), "Bruno Haible");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test for the subprogram name.  */
  if (optind == argc)
    error (EXIT_FAILURE, 0, _("missing command name"));
  sub_name = argv[optind];

  /* Build argument list for the program.  */
  sub_argc = argc - optind;
  sub_argv = (char **) xmalloc ((sub_argc + 1) * sizeof (char *));
  for (i = 0; i < sub_argc; i++)
    sub_argv[i] = argv[optind + i];
  sub_argv[i] = NULL;

  /* By default, input comes from standard input.  */
  if (input_file == NULL)
    input_file = "-";

  /* Read input file.  */
  result = read_po_file (input_file);

  if (strcmp (sub_name, "0") != 0)
    {
      /* Attempt to locate the program.
	 This is an optimization, to avoid that spawn/exec searches the PATH
	 on every call.  */
      sub_path = find_in_path (sub_name);

      /* Finish argument list for the program.  */
      sub_argv[0] = (char *) sub_path;
    }

  exitcode = 0; /* = EXIT_SUCCESS */

  /* Apply the subprogram.  */
  process_msgdomain_list (result);

  exit (exitcode);
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
Usage: %s [OPTION] COMMAND [COMMAND-OPTION]\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Applies a command to all translations of a translation catalog.\n\
The COMMAND can be any program that reads a translation from standard\n\
input.  It is invoked once for each translation.  Its output becomes\n\
msgexec's output.  msgexec's return code is the maximum return code\n\
across all invocations.\n\
"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
A special builtin command called '0' outputs the translation, followed by a\n\
null byte.  The output of \"msgexec 0\" is suitable as input for \"xargs -0\".\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  -i, --input=INPUTFILE       input PO file\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If no input file is given or if it is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Input file syntax:\n"));
      printf (_("\
  -P, --properties-input      input file is in Java .properties syntax\n"));
      printf (_("\
      --stringtable-input     input file is in NeXTstep/GNUstep .strings syntax\n"));
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


#ifdef EINTR

/* EINTR handling for close().
   These functions can return -1/EINTR even though we don't have any
   signal handlers set up, namely when we get interrupted via SIGSTOP.  */

static inline int
nonintr_close (int fd)
{
  int retval;

  do
    retval = close (fd);
  while (retval < 0 && errno == EINTR);

  return retval;
}
#define close nonintr_close

#endif


/* Pipe a string STR of size LEN bytes to the subprogram.
   The byte after STR is known to be a '\0' byte.  */
static void
process_string (const message_ty *mp, const char *str, size_t len)
{
  if (strcmp (sub_name, "0") == 0)
    {
      /* Built-in command "0".  */
      if (full_write (STDOUT_FILENO, str, len + 1) < len + 1)
	error (EXIT_FAILURE, errno, _("write to stdout failed"));
    }
  else
    {
      /* General command.  */
      char *location;
      pid_t child;
      int fd[1];
      int exitstatus;

      /* Set environment variables for the subprocess.  */
      xsetenv ("MSGEXEC_MSGID", mp->msgid, 1);
      location = xasprintf ("%s:%ld", mp->pos.file_name,
			    (long) mp->pos.line_number);
      xsetenv ("MSGEXEC_LOCATION", location, 1);
      free (location);

      /* Open a pipe to a subprocess.  */
      child = create_pipe_out (sub_name, sub_path, sub_argv, NULL, false, true,
			       true, fd);

      if (full_write (fd[0], str, len) < len)
	error (EXIT_FAILURE, errno,
	       _("write to %s subprocess failed"), sub_name);

      close (fd[0]);

      /* Remove zombie process from process list, and retrieve exit status.  */
      /* FIXME: Should ignore_sigpipe be set to true here? It depends on the
	 semantics of the subprogram...  */
      exitstatus = wait_subprocess (child, sub_name, false, false, true, true);
      if (exitcode < exitstatus)
	exitcode = exitstatus;
    }
}


static void
process_message (const message_ty *mp)
{
  const char *msgstr = mp->msgstr;
  size_t msgstr_len = mp->msgstr_len;
  const char *p;

  /* Process each NUL delimited substring separately.  */
  for (p = msgstr; p < msgstr + msgstr_len; )
    {
      size_t length = strlen (p);

      process_string (mp, p, length);

      p += length + 1;
    }
}


static void
process_message_list (const message_list_ty *mlp)
{
  size_t j;

  for (j = 0; j < mlp->nitems; j++)
    process_message (mlp->item[j]);
}


static void
process_msgdomain_list (const msgdomain_list_ty *mdlp)
{
  size_t k;

  for (k = 0; k < mdlp->nitems; k++)
    process_message_list (mdlp->item[k]->messages);
}
