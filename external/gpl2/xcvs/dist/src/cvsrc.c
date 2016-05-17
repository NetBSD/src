/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1993 david d zuhn
 * 
 * Written by david d `zoo' zuhn while at Cygnus Support
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 *
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: cvsrc.c,v 1.2 2016/05/17 14:00:09 christos Exp $");

#include "cvs.h"
#include "getline.h"

/* this file is to be found in the user's home directory */

#ifndef	CVSRC_FILENAME
#define	CVSRC_FILENAME	".cvsrc"
#endif
char cvsrc[] = CVSRC_FILENAME;

#define	GROW	10

/* Read cvsrc, processing options matching CMDNAME ("cvs" for global
   options, and update *ARGC and *ARGV accordingly.  */

void
read_cvsrc (int *argc, char ***argv, const char *cmdname)
{
    char *homedir;
    char *homeinit;
    FILE *cvsrcfile;

    char *line;
    int line_length;
    size_t line_chars_allocated;

    char *optstart;

    int command_len;
    int found = 0;

    int i;

    int new_argc;
    int max_new_argv;
    char **new_argv;

    /* old_argc and old_argv hold the values returned from the
       previous invocation of read_cvsrc and are used to free the
       allocated memory.  The first invocation of read_cvsrc gets argv
       from the system, this memory must not be free'd.  */
    static int old_argc = 0;
    static char **old_argv = NULL;

    /* don't do anything if argc is -1, since that implies "help" mode */
    if (*argc == -1)
	return;

    /* determine filename for ~/.cvsrc */

    homedir = get_homedir ();
    /* If we can't find a home directory, ignore ~/.cvsrc.  This may
       make tracking down problems a bit of a pain, but on the other
       hand it might be obnoxious to complain when CVS will function
       just fine without .cvsrc (and many users won't even know what
       .cvsrc is).  */
    if (!homedir)
	return;

    homeinit = strcat_filename_onto_homedir (homedir, cvsrc);

    /* if it can't be read, there's no point to continuing */

    if (!isreadable (homeinit))
    {
	free (homeinit);
	return;
    }

    /* now scan the file until we find the line for the command in question */

    line = NULL;
    line_chars_allocated = 0;
    command_len = strlen (cmdname);
    cvsrcfile = xfopen (homeinit, "r");
    while ((line_length = getline (&line, &line_chars_allocated, cvsrcfile))
	   >= 0)
    {
	/* skip over comment lines */
	if (line[0] == '#')
	    continue;

	/* stop if we match the current command */
	if (!strncmp (line, cmdname, command_len)
	    && isspace ((unsigned char) *(line + command_len)))
	{
	    found = 1;
	    break;
	}
    }

    if (line_length < 0 && !feof (cvsrcfile))
	error (0, errno, "cannot read %s", homeinit);

    fclose (cvsrcfile);

    /* setup the new options list */

    new_argc = 1;
    max_new_argv = (*argc) + GROW;
    new_argv = xnmalloc (max_new_argv, sizeof (char *));
    new_argv[0] = xstrdup ((*argv)[0]);

    if (found)
    {
	/* skip over command in the options line */
	for (optstart = strtok (line + command_len, "\t \n");
	     optstart;
	     optstart = strtok (NULL, "\t \n"))
	{
	    new_argv [new_argc++] = xstrdup (optstart);
	  
	    if (new_argc >= max_new_argv)
	    {
		max_new_argv += GROW;
		new_argv = xnrealloc (new_argv, max_new_argv, sizeof (char *));
	    }
	}
    }

    if (line != NULL)
	free (line);

    /* now copy the remaining arguments */
  
    if (new_argc + *argc > max_new_argv)
    {
	max_new_argv = new_argc + *argc;
	new_argv = xnrealloc (new_argv, max_new_argv, sizeof (char *));
    }
    for (i = 1; i < *argc; i++)
	new_argv [new_argc++] = xstrdup ((*argv)[i]);

    if (old_argv != NULL)
    {
	/* Free the memory which was allocated in the previous
           read_cvsrc call.  */
	free_names (&old_argc, old_argv);
    }

    old_argc = *argc = new_argc;
    old_argv = *argv = new_argv;

    free (homeinit);
    return;
}
