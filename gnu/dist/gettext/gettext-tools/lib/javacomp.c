/* Compile a Java program.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
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
# include <config.h>
#endif
#include <alloca.h>

/* Specification.  */
#include "javacomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "execute.h"
#include "pipe.h"
#include "wait-process.h"
#include "classpath.h"
#include "xsetenv.h"
#include "sh-quote.h"
#include "safe-read.h"
#include "xalloc.h"
#include "xallocsa.h"
#include "error.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Survey of Java compilers.

   A = does it work without CLASSPATH being set
   C = option to set CLASSPATH, other than setting it in the environment
   O = option for optimizing
   g = option for debugging
   T = test for presence

   Program  from        A  C               O  g  T

   $JAVAC   unknown     N  n/a            -O -g  true
   gcj -C   GCC 3.2     Y  --classpath=P  -O -g  gcj --version | sed -e 's,^[^0-9]*,,' -e 1q | sed -e '/^3\.[01]/d' | grep '^[3-9]' >/dev/null
   javac    JDK 1.1.8   Y  -classpath P   -O -g  javac 2>/dev/null; test $? = 1
   javac    JDK 1.3.0   Y  -classpath P   -O -g  javac 2>/dev/null; test $? -le 2
   jikes    Jikes 1.14  N  -classpath P   -O -g  jikes 2>/dev/null; test $? = 1

   All compilers support the option "-d DIRECTORY" for the base directory
   of the classes to be written.

   The CLASSPATH is a colon separated list of pathnames. (On Windows: a
   semicolon separated list of pathnames.)

   We try the Java compilers in the following order:
     1. getenv ("JAVAC"), because the user must be able to override our
	preferences,
     2. "gcj -C", because it is a completely free compiler,
     3. "javac", because it is a standard compiler,
     4. "jikes", comes last because it has some deviating interpretation
	of the Java Language Specification and because it requires a
	CLASSPATH environment variable.

   We unset the JAVA_HOME environment variable, because a wrong setting of
   this variable can confuse the JDK's javac.
 */

bool
compile_java_class (const char * const *java_sources,
		    unsigned int java_sources_count,
		    const char * const *classpaths,
		    unsigned int classpaths_count,
		    const char *directory,
		    bool optimize, bool debug,
		    bool use_minimal_classpath,
		    bool verbose)
{
  bool err = false;
  char *old_JAVA_HOME;

  {
    const char *javac = getenv ("JAVAC");
    if (javac != NULL && javac[0] != '\0')
      {
	/* Because $JAVAC may consist of a command and options, we use the
	   shell.  Because $JAVAC has been set by the user, we leave all
	   all environment variables in place, including JAVA_HOME, and
	   we don't erase the user's CLASSPATH.  */
	char *old_classpath;
	unsigned int command_length;
	char *command;
	char *argv[4];
	int exitstatus;
	unsigned int i;
	char *p;

	/* Set CLASSPATH.  */
	old_classpath =
	  set_classpath (classpaths, classpaths_count, false,
			 verbose);

	command_length = strlen (javac);
	if (optimize)
	  command_length += 3;
	if (debug)
	  command_length += 3;
	if (directory != NULL)
	  command_length += 4 + shell_quote_length (directory);
	for (i = 0; i < java_sources_count; i++)
	  command_length += 1 + shell_quote_length (java_sources[i]);
	command_length += 1;

	command = (char *) xallocsa (command_length);
	p = command;
	/* Don't shell_quote $JAVAC, because it may consist of a command
	   and options.  */
	memcpy (p, javac, strlen (javac));
	p += strlen (javac);
	if (optimize)
	  {
	    memcpy (p, " -O", 3);
	    p += 3;
	  }
	if (debug)
	  {
	    memcpy (p, " -g", 3);
	    p += 3;
	  }
	if (directory != NULL)
	  {
	    memcpy (p, " -d ", 4);
	    p += 4;
	    p = shell_quote_copy (p, directory);
	  }
	for (i = 0; i < java_sources_count; i++)
	  {
	    *p++ = ' ';
	    p = shell_quote_copy (p, java_sources[i]);
	  }
	*p++ = '\0';
	/* Ensure command_length was correctly calculated.  */
	if (p - command > command_length)
	  abort ();

	if (verbose)
	  printf ("%s\n", command);

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = command;
	argv[3] = NULL;
	exitstatus = execute (javac, "/bin/sh", argv, false, false, false,
			      false, true, true);
	err = (exitstatus != 0);

	freesa (command);

	/* Reset CLASSPATH.  */
	reset_classpath (old_classpath);

	goto done1;
      }
  }

  /* Unset the JAVA_HOME environment variable.  */
  old_JAVA_HOME = getenv ("JAVA_HOME");
  if (old_JAVA_HOME != NULL)
    {
      old_JAVA_HOME = xstrdup (old_JAVA_HOME);
      unsetenv ("JAVA_HOME");
    }

  {
    static bool gcj_tested;
    static bool gcj_present;

    if (!gcj_tested)
      {
	/* Test for presence of gcj:
	   "gcj --version 2> /dev/null | \
	    sed -e 's,^[^0-9]*,,' -e 1q | \
	    sed -e '/^3\.[01]/d' | grep '^[3-9]' > /dev/null"  */
	char *argv[3];
	pid_t child;
	int fd[1];
	int exitstatus;

	argv[0] = "gcj";
	argv[1] = "--version";
	argv[2] = NULL;
	child = create_pipe_in ("gcj", "gcj", argv, DEV_NULL, true, true,
				false, fd);
	gcj_present = false;
	if (child != -1)
	  {
	    /* Read the subprocess output, drop all lines except the first,
	       drop all characters before the first digit, and test whether
	       the remaining string starts with a digit >= 3, but not with
	       "3.0" or "3.1".  */
	    char c[3];
	    size_t count = 0;

	    while (safe_read (fd[0], &c[count], 1) > 0)
	      {
		if (c[count] == '\n')
		  break;
		if (count == 0)
		  {
		    if (!(c[0] >= '0' && c[0] <= '9'))
		      continue;
		    gcj_present = (c[0] >= '3');
		  }
		count++;
		if (count == 3)
		  {
		    if (c[0] == '3' && c[1] == '.'
			&& (c[2] == '0' || c[2] == '1'))
		      gcj_present = false;
		    break;
		  }
	      }
	    while (safe_read (fd[0], &c[0], 1) > 0)
	      ;

	    close (fd[0]);

	    /* Remove zombie process from process list, and retrieve exit
	       status.  */
	    exitstatus =
	      wait_subprocess (child, "gcj", false, true, true, false);
	    if (exitstatus != 0)
	      gcj_present = false;
	  }
	gcj_tested = true;
      }

    if (gcj_present)
      {
	char *old_classpath;
	unsigned int argc;
	char **argv;
	char **argp;
	int exitstatus;
	unsigned int i;

	/* Set CLASSPATH.  We could also use the --CLASSPATH=... option
	   of gcj.  Note that --classpath=... option is different: its
	   argument should also contain gcj's libgcj.jar, but we don't
	   know its location.  */
	old_classpath =
	  set_classpath (classpaths, classpaths_count, use_minimal_classpath,
			 verbose);

	argc =
	  2 + (optimize ? 1 : 0) + (debug ? 1 : 0) + (directory != NULL ? 2 : 0)
	  + java_sources_count;
	argv = (char **) xallocsa ((argc + 1) * sizeof (char *));

	argp = argv;
	*argp++ = "gcj";
	*argp++ = "-C";
	if (optimize)
	  *argp++ = "-O";
	if (debug)
	  *argp++ = "-g";
	if (directory != NULL)
	  {
	    *argp++ = "-d";
	    *argp++ = (char *) directory;
	  }
	for (i = 0; i < java_sources_count; i++)
	  *argp++ = (char *) java_sources[i];
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

	exitstatus = execute ("gcj", "gcj", argv, false, false, false, false,
			      true, true);
	err = (exitstatus != 0);

	freesa (argv);

	/* Reset CLASSPATH.  */
	reset_classpath (old_classpath);

	goto done2;
      }
  }

  {
    static bool javac_tested;
    static bool javac_present;

    if (!javac_tested)
      {
	/* Test for presence of javac: "javac 2> /dev/null ; test $? -le 2"  */
	char *argv[2];
	int exitstatus;

	argv[0] = "javac";
	argv[1] = NULL;
	exitstatus = execute ("javac", "javac", argv, false, false, true, true,
			      true, false);
	javac_present = (exitstatus == 0 || exitstatus == 1 || exitstatus == 2);
	javac_tested = true;
      }

    if (javac_present)
      {
	char *old_classpath;
	unsigned int argc;
	char **argv;
	char **argp;
	int exitstatus;
	unsigned int i;

	/* Set CLASSPATH.  We don't use the "-classpath ..." option because
	   in JDK 1.1.x its argument should also contain the JDK's classes.zip,
	   but we don't know its location.  (In JDK 1.3.0 it would work.)  */
	old_classpath =
	  set_classpath (classpaths, classpaths_count, use_minimal_classpath,
			 verbose);

	argc =
	  1 + (optimize ? 1 : 0) + (debug ? 1 : 0) + (directory != NULL ? 2 : 0)
	  + java_sources_count;
	argv = (char **) xallocsa ((argc + 1) * sizeof (char *));

	argp = argv;
	*argp++ = "javac";
	if (optimize)
	  *argp++ = "-O";
	if (debug)
	  *argp++ = "-g";
	if (directory != NULL)
	  {
	    *argp++ = "-d";
	    *argp++ = (char *) directory;
	  }
	for (i = 0; i < java_sources_count; i++)
	  *argp++ = (char *) java_sources[i];
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

	exitstatus = execute ("javac", "javac", argv, false, false, false,
			      false, true, true);
	err = (exitstatus != 0);

	freesa (argv);

	/* Reset CLASSPATH.  */
	reset_classpath (old_classpath);

	goto done2;
      }
  }

  {
    static bool jikes_tested;
    static bool jikes_present;

    if (!jikes_tested)
      {
	/* Test for presence of jikes: "jikes 2> /dev/null ; test $? = 1"  */
	char *argv[2];
	int exitstatus;

	argv[0] = "jikes";
	argv[1] = NULL;
	exitstatus = execute ("jikes", "jikes", argv, false, false, true, true,
			      true, false);
	jikes_present = (exitstatus == 0 || exitstatus == 1);
	jikes_tested = true;
      }

    if (jikes_present)
      {
	char *old_classpath;
	unsigned int argc;
	char **argv;
	char **argp;
	int exitstatus;
	unsigned int i;

	/* Set CLASSPATH.  We could also use the "-classpath ..." option.
	   Since jikes doesn't come with its own standard library, it
	   needs a classes.zip or rt.jar or libgcj.jar in the CLASSPATH.
	   To increase the chance of success, we reuse the current CLASSPATH
	   if the user has set it.  */
	old_classpath =
	  set_classpath (classpaths, classpaths_count, false,
			 verbose);

	argc =
	  1 + (optimize ? 1 : 0) + (debug ? 1 : 0) + (directory != NULL ? 2 : 0)
	  + java_sources_count;
	argv = (char **) xallocsa ((argc + 1) * sizeof (char *));

	argp = argv;
	*argp++ = "jikes";
	if (optimize)
	  *argp++ = "-O";
	if (debug)
	  *argp++ = "-g";
	if (directory != NULL)
	  {
	    *argp++ = "-d";
	    *argp++ = (char *) directory;
	  }
	for (i = 0; i < java_sources_count; i++)
	  *argp++ = (char *) java_sources[i];
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

	exitstatus = execute ("jikes", "jikes", argv, false, false, false,
			      false, true, true);
	err = (exitstatus != 0);

	freesa (argv);

	/* Reset CLASSPATH.  */
	reset_classpath (old_classpath);

	goto done2;
      }
  }

  error (0, 0, _("Java compiler not found, try installing gcj or set $JAVAC"));
  err = true;

 done2:
  if (old_JAVA_HOME != NULL)
    {
      xsetenv ("JAVA_HOME", old_JAVA_HOME, 1);
      free (old_JAVA_HOME);
    }

 done1:
  return err;
}
