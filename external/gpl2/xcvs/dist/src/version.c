/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1994 david d `zoo' zuhn
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with this  CVS source distribution.
 * 
 * version.c - the CVS version number
 */

#include "cvs.h"

#ifdef CLIENT_SUPPORT
#ifdef SERVER_SUPPORT
char *config_string = " (client/server)\n";
#else
char *config_string = " (client)\n";
#endif
#else
#ifdef SERVER_SUPPORT
char *config_string = " (server)\n";
#else
char *config_string = "\n";
#endif
#endif

/* cvsacl patch */
static const char aclpatch_string[] =
    "with CVSACL Patch 1.2.5 (cvsacl.sourceforge.net)\n";

static const char *const version_usage[] =
{
    "Usage: %s %s\n",
    NULL
};



/*
 * Output a version string for the client and server.
 *
 * This function will output the simple version number (for the '--version'
 * option) or the version numbers of the client and server (using the 'version'
 * command).
 */
int
version (int argc, char **argv)
{
    int err = 0;

    if (argc == -1)
	usage (version_usage);

    if (current_parsed_root && current_parsed_root->isremote)
        (void) fputs ("Client: ", stdout);

    /* Having the year here is a good idea, so people have
       some idea of how long ago their version of CVS was
       released.  */
    (void) fputs (PACKAGE_STRING, stdout);
    (void) fputs (config_string, stdout);
    /* cvsacl patch */
    (void) fputs (aclpatch_string, stdout);

#ifdef CLIENT_SUPPORT
    if (current_parsed_root && current_parsed_root->isremote)
    {
	(void) fputs ("Server: ", stdout);
	start_server ();
	if (supported_request ("version"))
	    send_to_server ("version\012", 0);
	else
	{
	    send_to_server ("noop\012", 0);
	    fputs ("(unknown)\n", stdout);
	}
	err = get_responses_and_close ();
    }
#endif
    return err;
}
	
