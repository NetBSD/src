/*++
/* NAME
/*	clean_env 3
/* SUMMARY
/*	clean up the environment
/* SYNOPSIS
/*	#include <clean_env.h>
/*
/*	void	clean_env(export_list)
/*	const char **export_list;
/* DESCRIPTION
/*	clean_env() reduces the process environment to the bare minimum.
/*	The function takes a null-terminated list of arguments.
/*	Each argument specifies the name of an environment variable
/*	that should be preserved.
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

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <safe.h>
#include <clean_env.h>

/* clean_env - clean up the environment */

void    clean_env(char **export_list)
{
    extern char **environ;
    ARGV   *save_list;
    char   *value;
    char  **cpp;

    /*
     * Preserve selected environment variables.
     */
    save_list = argv_alloc(10);
    for (cpp = export_list; *cpp; cpp++)
	if ((value = safe_getenv(*cpp)) != 0)
	    argv_add(save_list, *cpp, value, (char *) 0);
    argv_terminate(save_list);

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
	    msg_fatal("setenv: %m");

    /*
     * Cleanup.
     */
    argv_free(save_list);
}
