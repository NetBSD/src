/* Get the contents of an URL.
   Copyright (C) 2001-2003, 2005 Free Software Foundation, Inc.
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
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "closeout.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "full-write.h"
#include "execute.h"
#include "javaexec.h"
#include "exit.h"
#include "binary-io.h"
#include "gettext.h"

#define _(str) gettext (str)

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif


/* Only high-level toolkits, written in languages with exception handling,
   have an URL datatype and operations to fetch an URL's contents.  Such
   toolkits are Java (class java.net.URL), Qt (classes QUrl and QUrlOperator).
   We use the Java toolkit.
   Note that this program doesn't handle redirection pages; programs which
   wish to process HTML redirection tags need to include a HTML parser,
   and only full-fledged browsers like w3m, lynx, links have have both
   an URL fetcher (which covers at least the protocols "http", "ftp", "file")
   and a HTML parser.  */


/* Long options.  */
static const struct option long_options[] =
{
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
     __attribute__ ((noreturn))
#endif
;
static void fetch (const char *url, const char *file);


int
main (int argc, char *argv[])
{
  int optchar;
  bool do_help;
  bool do_version;

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

  /* Parse command line options.  */
  while ((optchar = getopt_long (argc, argv, "hV", long_options, NULL)) != EOF)
    switch (optchar)
    {
    case '\0':		/* Long option.  */
      break;
    case 'h':
      do_help = true;
      break;
    case 'V':
      do_version = true;
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
	      "2001-2003");
      printf (_("Written by %s.\n"), "Bruno Haible");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test argument count.  */
  if (optind + 2 != argc)
    error (EXIT_FAILURE, 0, _("expected two arguments"));

  /* Fetch the contents.  */
  fetch (argv[optind], argv[optind + 1]);

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
Usage: %s [OPTION] URL FILE\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Fetches and outputs the contents of an URL.  If the URL cannot be accessed,\n\
the locally accessible FILE is used instead.\n\
"));
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

/* Copy a file's contents to stdout.  */
static void
cat_file (const char *src_filename)
{
  int src_fd;
  char buf[4096];
  const int buf_size = sizeof (buf);

  src_fd = open (src_filename, O_RDONLY | O_BINARY);
  if (src_fd < 0)
    error (EXIT_FAILURE, errno, _("error while opening \"%s\" for reading"),
	   src_filename);

  for (;;)
    {
      ssize_t n_read = read (src_fd, buf, buf_size);
      if (n_read < 0)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
#endif
	  error (EXIT_FAILURE, errno, _("error reading \"%s\""), src_filename);
	}
      if (n_read == 0)
	break;

      if (full_write (STDOUT_FILENO, buf, n_read) < n_read)
	error (EXIT_FAILURE, errno, _("error writing stdout"));
    }

  if (close (src_fd) < 0)
    error (EXIT_FAILURE, errno, _("error after reading \"%s\""), src_filename);
}

static bool
execute_it (const char *progname,
	    const char *prog_path, char **prog_argv,
	    void *private_data)
{
  (void) private_data;

  return execute (progname, prog_path, prog_argv, true, true, false, false,
		  true, false)
	 != 0;
}

/* Fetch the URL.  Upon error, use the FILE as fallback.  */
static void
fetch (const char *url, const char *file)
{
  /* First try: using Java.  */
  {
    const char *class_name = "gnu.gettext.GetURL";
    const char *gettextjexedir;
    const char *gettextjar;
    const char *args[2];

#if USEJEXE
    /* Make it possible to override the executable's location.  This is
       necessary for running the testsuite before "make install".  */
    gettextjexedir = getenv ("GETTEXTJEXEDIR");
    if (gettextjexedir == NULL || gettextjexedir[0] == '\0')
      gettextjexedir = relocate (GETTEXTJEXEDIR);
#else
    gettextjexedir = NULL;
#endif

    /* Make it possible to override the gettext.jar location.  This is
       necessary for running the testsuite before "make install".  */
    gettextjar = getenv ("GETTEXTJAR");
    if (gettextjar == NULL || gettextjar[0] == '\0')
      gettextjar = relocate (GETTEXTJAR);

    /* Prepare arguments.  */
    args[0] = url;
    args[1] = NULL;

    /* Fetch the URL's contents.  */
    if (execute_java_class (class_name, &gettextjar, 1, true, gettextjexedir,
			    args,
			    false, true,
			    execute_it, NULL) == 0)
      return;
  }

  /* Second try: using "wget -q -O - url".  */
  {
    static bool wget_tested;
    static bool wget_present;

    if (!wget_tested)
      {
	/* Test for presence of wget: "wget --version > /dev/null"  */
	char *argv[3];
	int exitstatus;

	argv[0] = "wget";
	argv[1] = "--version";
	argv[2] = NULL;
	exitstatus = execute ("wget", "wget", argv, false, false, true, true,
			      true, false);
	wget_present = (exitstatus == 0);
	wget_tested = true;
      }

    if (wget_present)
      {
	char *argv[8];
	int exitstatus;

	argv[0] = "wget";
	argv[1] = "-q";
	argv[2] = "-O"; argv[3] = "-";
	argv[4] = "-T"; argv[5] = "30";
	argv[6] = (char *) url;
	argv[7] = NULL;
	exitstatus = execute ("wget", "wget", argv, true, false, false, false,
			      true, false);
	if (exitstatus != 127)
	  {
	    if (exitstatus != 0)
	      /* Use the file as fallback.  */
	      cat_file (file);
	    return;
	  }
      }
  }

  /* Third try: using "lynx -source url".  */
  {
    static bool lynx_tested;
    static bool lynx_present;

    if (!lynx_tested)
      {
	/* Test for presence of lynx: "lynx --version > /dev/null"  */
	char *argv[3];
	int exitstatus;

	argv[0] = "lynx";
	argv[1] = "--version";
	argv[2] = NULL;
	exitstatus = execute ("lynx", "lynx", argv, false, false, true, true,
			      true, false);
	lynx_present = (exitstatus == 0);
	lynx_tested = true;
      }

    if (lynx_present)
      {
	char *argv[4];
	int exitstatus;

	argv[0] = "lynx";
	argv[1] = "-source";
	argv[2] = (char *) url;
	argv[3] = NULL;
	exitstatus = execute ("lynx", "lynx", argv, true, false, false, false,
			      true, false);
	if (exitstatus != 127)
	  {
	    if (exitstatus != 0)
	      /* Use the file as fallback.  */
	      cat_file (file);
	    return;
	  }
      }
  }

  /* Fourth try: using "curl --silent url".  */
  {
    static bool curl_tested;
    static bool curl_present;

    if (!curl_tested)
      {
	/* Test for presence of curl: "curl --version > /dev/null"  */
	char *argv[3];
	int exitstatus;

	argv[0] = "curl";
	argv[1] = "--version";
	argv[2] = NULL;
	exitstatus = execute ("curl", "curl", argv, false, false, true, true,
			      true, false);
	curl_present = (exitstatus == 0 || exitstatus == 2);
	curl_tested = true;
      }

    if (curl_present)
      {
	char *argv[4];
	int exitstatus;

	argv[0] = "curl";
	argv[1] = "--silent";
	argv[2] = (char *) url;
	argv[3] = NULL;
	exitstatus = execute ("curl", "curl", argv, true, false, false, false,
			      true, false);
	if (exitstatus != 127)
	  {
	    if (exitstatus != 0)
	      /* Use the file as fallback.  */
	      cat_file (file);
	    return;
	  }
      }
  }

  /* Use the file as fallback.  */
  cat_file (file);
}
