/* Reading tcl/msgcat .msg files.
   Copyright (C) 2002-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

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

/* Specification.  */
#include "read-tcl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "msgunfmt.h"
#include "relocatable.h"
#include "pathname.h"
#include "sh-quote.h"
#include "pipe.h"
#include "wait-process.h"
#include "read-po.h"
#include "xallocsa.h"
#include "error.h"
#include "exit.h"
#include "gettext.h"

#define _(str) gettext (str)


/* A Tcl .msg file contains Tcl commands.  It is best interpreted by Tcl
   itself.  But we redirect the msgcat::mcset function so that it passes
   the msgid/msgstr pair to us, instead of storing it in the hash table.  */

msgdomain_list_ty *
msgdomain_read_tcl (const char *locale_name, const char *directory)
{
  const char *gettextdatadir;
  char *tclscript;
  size_t len;
  char *frobbed_locale_name;
  char *p;
  char *file_name;
  char *argv[4];
  pid_t child;
  int fd[1];
  FILE *fp;
  msgdomain_list_ty *mdlp;
  int exitstatus;
  size_t k;

  /* Make it possible to override the msgunfmt.tcl location.  This is
     necessary for running the testsuite before "make install".  */
  gettextdatadir = getenv ("GETTEXTDATADIR");
  if (gettextdatadir == NULL || gettextdatadir[0] == '\0')
    gettextdatadir = relocate (GETTEXTDATADIR);

  tclscript = concatenated_pathname (gettextdatadir, "msgunfmt.tcl", NULL);

  /* Convert the locale name to lowercase and remove any encoding.  */
  len = strlen (locale_name);
  frobbed_locale_name = (char *) xallocsa (len + 1);
  memcpy (frobbed_locale_name, locale_name, len + 1);
  for (p = frobbed_locale_name; *p != '\0'; p++)
    if (*p >= 'A' && *p <= 'Z')
      *p = *p - 'A' + 'a';
    else if (*p == '.')
      {
	*p = '\0';
	break;
      }

  file_name = concatenated_pathname (directory, frobbed_locale_name, ".msg");

  freesa (frobbed_locale_name);

  /* Prepare arguments.  */
  argv[0] = "tclsh";
  argv[1] = tclscript;
  argv[2] = file_name;
  argv[3] = NULL;

  if (verbose)
    {
      char *command = shell_quote_argv (argv);
      printf ("%s\n", command);
      free (command);
    }

  /* Open a pipe to the Tcl interpreter.  */
  child = create_pipe_in ("tclsh", "tclsh", argv, DEV_NULL, false, true, true,
			  fd);

  fp = fdopen (fd[0], "r");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  /* Read the message list.  */
  mdlp = read_po (fp, "(pipe)", "(pipe)");

  fclose (fp);

  /* Remove zombie process from process list, and retrieve exit status.  */
  exitstatus = wait_subprocess (child, "tclsh", false, false, true, true);
  if (exitstatus != 0)
    {
      if (exitstatus == 2)
	/* Special exitcode provided by msgunfmt.tcl.  */
	error (EXIT_FAILURE, ENOENT,
	       _("error while opening \"%s\" for reading"), file_name);
      else
	error (EXIT_FAILURE, 0, _("%s subprocess failed with exit code %d"),
	       "tclsh", exitstatus);
    }

  free (tclscript);

  /* Move the header entry to the beginning.  */
  for (k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;
      size_t j;

      for (j = 0; j < mlp->nitems; j++)
	if (mlp->item[j]->msgid[0] == '\0')
	  {
	    /* Found the header entry.  */
	    if (j > 0)
	      {
		message_ty *header = mlp->item[j];
		size_t i;

		for (i = j; i > 0; i--)
		  mlp->item[i] = mlp->item[i - 1];
		mlp->item[0] = header;
	      }
	    break;
	  }
    }

  return mdlp;
}
