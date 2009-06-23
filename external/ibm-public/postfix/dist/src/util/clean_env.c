/*	$NetBSD: clean_env.c,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

/*++
/* NAME
/*	clean_env 3
/* SUMMARY
/*	clean up the environment
/* SYNOPSIS
/*	#include <clean_env.h>
/*
/*	void	clean_env(preserve_list)
/*	const char **preserve_list;
/* DESCRIPTION
/*	clean_env() reduces the process environment to the bare minimum.
/*	The function takes a null-terminated list of arguments.
/*	Each argument specifies the name of an environment variable
/*	that should be preserved, or specifies a name=value that should
/*	be entered into the new environment.
/* DIAGNOSTICS
/*	Fatal error: out of memory.
/* SEE ALSO
/*	safe_getenv(3), guarded getenv()
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <safe.h>
#include <clean_env.h>

/* clean_env - clean up the environment */

void    clean_env(char **preserve_list)
{
    extern char **environ;
    ARGV   *save_list;
    char   *value;
    char  **cpp;
    char   *eq;

    /*
     * Preserve or specify selected environment variables.
     */
#define STRING_AND_LENGTH(x, y) (x), (ssize_t) (y)

    save_list = argv_alloc(10);
    for (cpp = preserve_list; *cpp; cpp++)
	if ((eq = strchr(*cpp, '=')) != 0)
	    argv_addn(save_list, STRING_AND_LENGTH(*cpp, eq - *cpp),
		      STRING_AND_LENGTH(eq + 1, strlen(eq + 1)), (char *) 0);
	else if ((value = safe_getenv(*cpp)) != 0)
	    argv_add(save_list, *cpp, value, (char *) 0);

    /*
     * Truncate the process environment, if available. On some systems
     * (Ultrix!), environ can be a null pointer.
     */
    if (environ)
	environ[0] = 0;

    /*
     * Restore preserved environment variables.
     */
    for (cpp = save_list->argv; *cpp; cpp += 2)
	if (setenv(cpp[0], cpp[1], 1))
	    msg_fatal("setenv(%s, %s): %m", cpp[0], cpp[1]);

    /*
     * Cleanup.
     */
    argv_free(save_list);
}
