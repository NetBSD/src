/* Writing C# .resources files.
   Copyright (C) 2003, 2005 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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

/* Specification.  */
#include "write-resources.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "xerror.h"
#include "relocatable.h"
#include "csharpexec.h"
#include "pipe.h"
#include "wait-process.h"
#include "message.h"
#include "msgfmt.h"
#include "msgl-iconv.h"
#include "po-charset.h"
#include "xalloc.h"
#include "pathname.h"
#include "fwriteerror.h"
#include "exit.h"
#include "gettext.h"

#define _(str) gettext (str)


/* A .resources file has such a complex format that it's most easily generated
   through the C# class ResourceWriter.  So we start a C# process to execute
   the WriteResource program, sending it the msgid/msgstr pairs as
   NUL-terminated UTF-8 encoded strings.  */

struct locals
{
  /* IN */
  message_list_ty *mlp;
};

static bool
execute_writing_input (const char *progname,
		       const char *prog_path, char **prog_argv,
		       void *private_data)
{
  struct locals *l = (struct locals *) private_data;
  pid_t child;
  int fd[1];
  FILE *fp;
  int exitstatus;

  /* Open a pipe to the C# execution engine.  */
  child = create_pipe_out (progname, prog_path, prog_argv, NULL, false,
			   true, true, fd);

  fp = fdopen (fd[0], "wb");
  if (fp == NULL)
    error (EXIT_FAILURE, errno, _("fdopen() failed"));

  /* Write the message list.  */
  {
    message_list_ty *mlp = l->mlp;
    size_t j;

    for (j = 0; j < mlp->nitems; j++)
      {
	message_ty *mp = mlp->item[j];

	fwrite (mp->msgid, 1, strlen (mp->msgid) + 1, fp);
	fwrite (mp->msgstr, 1, strlen (mp->msgstr) + 1, fp);
      }
  }

  if (fwriteerror (fp))
    error (EXIT_FAILURE, 0, _("error while writing to %s subprocess"),
	   progname);

  /* Remove zombie process from process list, and retrieve exit status.  */
  /* He we can ignore SIGPIPE because WriteResource either writes to a file
     - then it never gets SIGPIPE - or to standard output, and in the latter
     case it has no side effects other than writing to standard output.  */
  exitstatus = wait_subprocess (child, progname, true, false, true, true);
  if (exitstatus != 0)
    error (EXIT_FAILURE, 0, _("%s subprocess failed with exit code %d"),
	   progname, exitstatus);

  return false;
}

int
msgdomain_write_csharp_resources (message_list_ty *mlp,
				  const char *canon_encoding,
				  const char *domain_name,
				  const char *file_name)
{
  /* If no entry for this domain don't even create the file.  */
  if (mlp->nitems != 0)
    {
      /* Determine whether mlp has plural entries.  */
      {
	bool has_plural;
	size_t j;

	has_plural = false;
	for (j = 0; j < mlp->nitems; j++)
	  if (mlp->item[j]->msgid_plural != NULL)
	    has_plural = true;
	if (has_plural)
	  {
	    multiline_error (xstrdup (""),
			     xstrdup (_("\
message catalog has plural form translations\n\
but the C# .resources format doesn't support plural handling\n")));
	    return 1;
	  }
      }

      /* Convert the messages to Unicode.  */
      iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL);

      /* Execute the WriteResource program.  */
      {
	const char *args[2];
	const char *gettextexedir;
	char *assembly_path;
	struct locals locals;

	/* Prepare arguments.  */
	args[0] = file_name;
	args[1] = NULL;

	/* Make it possible to override the .exe location.  This is
	   necessary for running the testsuite before "make install".  */
	gettextexedir = getenv ("GETTEXTCSHARPEXEDIR");
	if (gettextexedir == NULL || gettextexedir[0] == '\0')
	  gettextexedir = relocate (LIBDIR "/gettext");

	assembly_path =
	  concatenated_pathname (gettextexedir, "msgfmt.net", ".exe");

	locals.mlp = mlp;

	if (execute_csharp_program (assembly_path, NULL, 0,
				    args,
				    verbose, false,
				    execute_writing_input, &locals))
	  /* An error message should already have been provided.  */
	  exit (EXIT_FAILURE);

	free (assembly_path);
      }
    }

  return 0;
}
