/* Execute a C# program.
   Copyright (C) 2003-2004 Free Software Foundation, Inc.
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
#include <alloca.h>

/* Specification.  */
#include "csharpexec.h"

#include <stdio.h>
#include <stdlib.h>

#include "execute.h"
#include "sh-quote.h"
#include "xallocsa.h"
#include "error.h"
#include "gettext.h"

/* Handling of MONO_PATH is just like Java CLASSPATH.  */
#define CLASSPATHVAR "MONO_PATH"
#define new_classpath new_monopath
#define set_classpath set_monopath
#define reset_classpath reset_monopath
#include "classpath.h"
#include "classpath.c"

#define _(str) gettext (str)


/* Survey of CIL interpreters.

   Program    from

   ilrun      pnet
   mono       mono

   With Mono, the MONO_PATH is a colon separated list of pathnames. (On
   Windows: semicolon separated list of pathnames.)

   We try the CIL interpreters in the following order:
     1. "ilrun", because it is a completely free system.
     2. "mono", because it is a partially free system but doesn't integrate
        well with Unix.
   But the order can be changed through the --enable-csharp configuration
   option.
 */

static int
execute_csharp_using_pnet (const char *assembly_path,
			   const char * const *libdirs,
			   unsigned int libdirs_count,
			   const char * const *args, unsigned int nargs,
			   bool verbose, bool quiet,
			   execute_fn *executer, void *private_data)
{
  static bool ilrun_tested;
  static bool ilrun_present;

  if (!ilrun_tested)
    {
      /* Test for presence of ilrun:
	 "ilrun --version >/dev/null 2>/dev/null"  */
      char *argv[3];
      int exitstatus;

      argv[0] = "ilrun";
      argv[1] = "--version";
      argv[2] = NULL;
      exitstatus = execute ("ilrun", "ilrun", argv, false, false, true, true,
			    true, false);
      ilrun_present = (exitstatus == 0);
      ilrun_tested = true;
    }

  if (ilrun_present)
    {
      unsigned int argc;
      char **argv;
      char **argp;
      unsigned int i;
      bool err;

      argc = 1 + 2 * libdirs_count + 1 + nargs;
      argv = (char **) xallocsa ((argc + 1) * sizeof (char *));

      argp = argv;
      *argp++ = "ilrun";
      for (i = 0; i < libdirs_count; i++)
	{
	  *argp++ = "-L";
	  *argp++ = (char *) libdirs[i];
	}
      *argp++ = (char *) assembly_path;
      for (i = 0; i < nargs; i++)
	*argp++ = (char *) args[i];
      *argp = NULL;
      /* Ensure argv length was correctly calculated.  */
      if (argp - argv != argc)
	abort ();

      if (verbose)
	{
	  char *command = shell_quote_argv (argv);
	  printf ("%s\n", command);
	  free (command);
	}

      err = executer ("ilrun", "ilrun", argv, private_data);

      freesa (argv);

      return err;
    }
  else
    return -1;
}

static int
execute_csharp_using_mono (const char *assembly_path,
			   const char * const *libdirs,
			   unsigned int libdirs_count,
			   const char * const *args, unsigned int nargs,
			   bool verbose, bool quiet,
			   execute_fn *executer, void *private_data)
{
  static bool mono_tested;
  static bool mono_present;

  if (!mono_tested)
    {
      /* Test for presence of mono:
	 "mono --version >/dev/null 2>/dev/null"  */
      char *argv[3];
      int exitstatus;

      argv[0] = "mono";
      argv[1] = "--version";
      argv[2] = NULL;
      exitstatus = execute ("mono", "mono", argv, false, false, true, true,
			    true, false);
      mono_present = (exitstatus == 0);
      mono_tested = true;
    }

  if (mono_present)
    {
      char *old_monopath;
      char **argv = (char **) xallocsa ((2 + nargs + 1) * sizeof (char *));
      unsigned int i;
      bool err;

      /* Set MONO_PATH.  */
      old_monopath = set_monopath (libdirs, libdirs_count, false, verbose);

      argv[0] = "mono";
      argv[1] = (char *) assembly_path;
      for (i = 0; i <= nargs; i++)
	argv[2 + i] = (char *) args[i];

      if (verbose)
	{
	  char *command = shell_quote_argv (argv);
	  printf ("%s\n", command);
	  free (command);
	}

      err = executer ("mono", "mono", argv, private_data);

      /* Reset MONO_PATH.  */
      reset_monopath (old_monopath);

      freesa (argv);

      return err;
    }
  else
    return -1;
}

bool
execute_csharp_program (const char *assembly_path,
			const char * const *libdirs,
			unsigned int libdirs_count,
			const char * const *args,
			bool verbose, bool quiet,
			execute_fn *executer, void *private_data)
{
  unsigned int nargs;
  int result;

  /* Count args.  */
  {
    const char * const *arg;

    for (nargs = 0, arg = args; *arg != NULL; nargs++, arg++)
     ;
  }

  /* First try the C# implementation specified through --enable-csharp.  */
#if CSHARP_CHOICE_PNET
  result = execute_csharp_using_pnet (assembly_path, libdirs, libdirs_count,
				      args, nargs, verbose, quiet,
				      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

#if CSHARP_CHOICE_MONO
  result = execute_csharp_using_mono (assembly_path, libdirs, libdirs_count,
				      args, nargs, verbose, quiet,
				      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

  /* Then try the remaining C# implementations in our standard order.  */
#if !CSHARP_CHOICE_PNET
  result = execute_csharp_using_pnet (assembly_path, libdirs, libdirs_count,
				      args, nargs, verbose, quiet,
				      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

#if !CSHARP_CHOICE_MONO
  result = execute_csharp_using_mono (assembly_path, libdirs, libdirs_count,
				      args, nargs, verbose, quiet,
				      executer, private_data);
  if (result >= 0)
    return (bool) result;
#endif

  if (!quiet)
    error (0, 0, _("C# virtual machine not found, try installing pnet"));
  return true;
}
