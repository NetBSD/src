/*++
/* NAME
/*	clean_env 3
/* SUMMARY
/*	clean up the environment
/* SYNOPSIS
/*	#include <clean_env.h>
/*
/*	void	clean_env()
/* DESCRIPTION
/*	clean_env() reduces the process environment to the bare minimum.
/*      In the initial version, rules are hard-coded. This will be
/*	made configurable.
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
#include <clean_env.h>

/* clean_env - clean up the environment */

void    clean_env(void)
{
    char   *TZ;
    extern char **environ;

    /*
     * Preserve selected environment variables. This list will be
     * configurable.
     */
    TZ = getenv("TZ");

    /*
     * Truncate the process environment, if available. On some systems
     * (Ultrix!), environ can be a null pointer.
     */
    if (environ)
	environ[0] = 0;

    /*
     * Restore preserved environment variables.
     */
    if (TZ && setenv("TZ", TZ, 1))
	msg_fatal("setenv: %m");

    /*
     * Update the process environment with configurable initial values.
     */
}
