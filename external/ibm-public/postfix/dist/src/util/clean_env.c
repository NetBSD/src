/*	$NetBSD: clean_env.c,v 1.2.6.1 2023/12/25 12:43:36 martin Exp $	*/

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
/*
/*	void	update_env(preserve_list)
/*	const char **preserve_list;
/* DESCRIPTION
/*	clean_env() reduces the process environment to the bare minimum.
/*	The function takes a null-terminated list of arguments.
/*	Each argument specifies the name of an environment variable
/*	that should be preserved, or specifies a name=value that should
/*	be entered into the new environment.
/*
/*	update_env() applies name=value settings, but otherwise does not
/*	change the process environment.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <argv.h>
#include <safe.h>
#include <clean_env.h>
#include <stringops.h>

/* clean_env - clean up the environment */

void    clean_env(char **preserve_list)
{
    extern char **environ;
    ARGV   *save_list;
    char   *value;
    char  **cpp;
    char   *copy;
    char   *key;
    char   *val;
    const char *err;

    /*
     * Preserve or specify selected environment variables.
     */
    save_list = argv_alloc(10);
    for (cpp = preserve_list; *cpp; cpp++) {
	if (strchr(*cpp, '=') != 0) {
	    copy = mystrdup(*cpp);
	    err = split_nameval(copy, &key, &val);
	    if (err != 0)
		msg_fatal("clean_env: %s in: %s", err, *cpp);
	    argv_add(save_list, key, val, (char *) 0);
	    myfree(copy);
	} else if ((value = safe_getenv(*cpp)) != 0) {
	    argv_add(save_list, *cpp, value, (char *) 0);
	}
    }

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

/* update_env - apply name=value settings only */

void    update_env(char **preserve_list)
{
    char  **cpp;
    ARGV   *save_list;
    char   *copy;
    char   *key;
    char   *val;
    const char *err;

    /*
     * Extract name=value settings.
     */
    save_list = argv_alloc(10);
    for (cpp = preserve_list; *cpp; cpp++) {
	if (strchr(*cpp, '=') != 0) {
	    copy = mystrdup(*cpp);
	    err = split_nameval(copy, &key, &val);
	    if (err != 0)
		msg_fatal("update_env: %s in: %s", err, *cpp);
	    argv_add(save_list, key, val, (char *) 0);
	    myfree(copy);
	}
    }

    /*
     * Apply name=value settings.
     */
    for (cpp = save_list->argv; *cpp; cpp += 2)
	if (setenv(cpp[0], cpp[1], 1))
	    msg_fatal("setenv(%s, %s): %m", cpp[0], cpp[1]);

    /*
     * Cleanup.
     */
    argv_free(save_list);
}

#ifdef TEST

#include <stdlib.h>
#include <vstream.h>

int     main(int argc, char **argv)
{
    extern char **environ;
    char  **cpp;

    clean_env(argv + 1);
    for (cpp = environ; *cpp; cpp++)
	vstream_printf("%s\n", *cpp);
    vstream_fflush(VSTREAM_OUT);
    exit(0);
}

#endif
