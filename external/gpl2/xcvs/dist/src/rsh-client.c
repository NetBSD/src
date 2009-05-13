/* CVS rsh client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <config.h>

#include "cvs.h"
#include "buffer.h"

#ifdef CLIENT_SUPPORT

#include "rsh-client.h"

#ifndef NO_EXT_METHOD

/* Contact the server by starting it with rsh.  */

/* Right now, we have two different definitions for this function,
   depending on whether we start the rsh server using popenRW or not.
   This isn't ideal, and the best thing would probably be to change
   the OS/2 port to be more like the regular Unix client (i.e., by
   implementing piped_child)... but I'm doing something else at the
   moment, and wish to make only one change at a time.  -Karl */

# ifdef START_RSH_WITH_POPEN_RW



/* This is actually a crock -- it's OS/2-specific, for no one else
   uses it.  If I get time, I want to make piped_child and all the
   other stuff in os2/run.c work right.  In the meantime, this gets us
   up and running, and that's most important. */
void
start_rsh_server (cvsroot_t *root, struct buffer **to_server_p,
                  struct buffer **from_server_p)
{
    int pipes[2];
    int child_pid;

    /* If you're working through firewalls, you can set the
       CVS_RSH environment variable to a script which uses rsh to
       invoke another rsh on a proxy machine.  */
    char *cvs_rsh = (root->cvs_rsh != NULL
		     ? root->cvs_rsh : getenv ("CVS_RSH"));
    char *cvs_server = (root->cvs_server != NULL
			? root->cvs_server : getenv ("CVS_SERVER"));
    int i = 0;
    /* This needs to fit "rsh", "-b", "-l", "USER", "host",
       "cmd (w/ args)", and NULL.  We leave some room to grow. */
    char *rsh_argv[10];

    if (!cvs_rsh)
	/* People sometimes suggest or assume that this should default
	   to "remsh" on systems like HPUX in which that is the
	   system-supplied name for the rsh program.  However, that
	   causes various problems (keep in mind that systems such as
	   HPUX might have non-system-supplied versions of "rsh", like
	   a Kerberized one, which one might want to use).  If we
	   based the name on what is found in the PATH of the person
	   who runs configure, that would make it harder to
	   consistently produce the same result in the face of
	   different people producing binary distributions.  If we
	   based it on "remsh" always being the default for HPUX
	   (e.g. based on uname), that might be slightly better but
	   would require us to keep track of what the defaults are for
	   each system type, and probably would cope poorly if the
	   existence of remsh or rsh varies from OS version to OS
	   version.  Therefore, it seems best to have the default
	   remain "rsh", and tell HPUX users to specify remsh, for
	   example in CVS_RSH or other such mechanisms to be devised,
	   if that is what they want (the manual already tells them
	   that).  */
	cvs_rsh = RSH_DFLT;
    if (!cvs_server)
	cvs_server = "cvs";

    /* The command line starts out with rsh. */
    rsh_argv[i++] = cvs_rsh;

#   ifdef RSH_NEEDS_BINARY_FLAG
    /* "-b" for binary, under OS/2. */
    rsh_argv[i++] = "-b";
#   endif /* RSH_NEEDS_BINARY_FLAG */

    /* Then we strcat more things on the end one by one. */
    if (root->username != NULL)
    {
	rsh_argv[i++] = "-l";
	rsh_argv[i++] = root->username;
    }

    rsh_argv[i++] = root->hostname;
    rsh_argv[i++] = cvs_server;
    rsh_argv[i++] = "server";

    /* Mark the end of the arg list. */
    rsh_argv[i]   = NULL;

    if (trace)
    {
	fprintf (stderr, " -> Starting server: ");
	for (i = 0; rsh_argv[i]; i++)
	    fprintf (stderr, "%s ", rsh_argv[i]);
	putc ('\n', stderr);
    }

    /* Do the deed. */
    child_pid = popenRW (rsh_argv, pipes);
    if (child_pid < 0)
	error (1, errno, "cannot start server via rsh");

    /* Give caller the file descriptors in a form it can deal with. */
    make_bufs_from_fds (pipes[0], pipes[1], child_pid, to_server_p,
                        from_server_p, 0);
}

# else /* ! START_RSH_WITH_POPEN_RW */

void
start_rsh_server (cvsroot_t *root, struct buffer **to_server_p,
                  struct buffer **from_server_p)
{
    /* If you're working through firewalls, you can set the
       CVS_RSH environment variable to a script which uses rsh to
       invoke another rsh on a proxy machine.  */
    char *cvs_rsh = (root->cvs_rsh != NULL
		     ? root->cvs_rsh : getenv ("CVS_RSH"));
    char *cvs_server = (root->cvs_server != NULL
			? root->cvs_server : getenv ("CVS_SERVER"));
    char *command;
    int tofd, fromfd;
    int child_pid;

    if (!cvs_rsh)
	cvs_rsh = RSH_DFLT;
    if (!cvs_server)
	cvs_server = "cvs";

    /* Pass the command to rsh as a single string.  This shouldn't
     * affect most rsh servers at all, and will pacify some buggy
     * versions of rsh that grab switches out of the middle of the
     * command (they're calling the GNU getopt routines incorrectly).
     *
     * If you are running a very old (Nov 3, 1994, before 1.5)
     * version of the server, you need to make sure that your .bashrc
     * on the server machine does not set CVSROOT to something
     * containing a colon (or better yet, upgrade the server).
     */
    command = Xasprintf ("%s server", cvs_server);

    {
        char *argv[10];
	char **p = argv;

	*p++ = cvs_rsh;

	/* If the login names differ between client and server
	 * pass it on to rsh.
	 */
	if (root->username != NULL)
	{
	    *p++ = "-l";
	    *p++ = root->username;
	}

	*p++ = root->hostname;
	*p++ = command;
	*p++ = NULL;

	if (trace)
        {
	    int i;

            fprintf (stderr, " -> Starting server: ");
	    for (i = 0; argv[i]; i++)
	        fprintf (stderr, "%s ", argv[i]);
	    putc ('\n', stderr);
	}
	child_pid = piped_child (argv, &tofd, &fromfd, true);

	if (child_pid < 0)
	    error (1, errno, "cannot start server via rsh");
    }
    free (command);

    make_bufs_from_fds (tofd, fromfd, child_pid, root, to_server_p,
                        from_server_p, 0);
}

# endif /* START_RSH_WITH_POPEN_RW */

#endif /* NO_EXT_METHOD */

#endif /* CLIENT_SUPPORT */
