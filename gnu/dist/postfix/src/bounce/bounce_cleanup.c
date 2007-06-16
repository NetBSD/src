/*	$NetBSD: bounce_cleanup.c,v 1.1.1.3.4.1 2007/06/16 16:58:41 snj Exp $	*/

/*++
/* NAME
/*	bounce_cleanup 3
/* SUMMARY
/*	cleanup logfile upon error
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int	bounce_cleanup_registered()
/*
/*	void	bounce_cleanup_register(queue_id)
/*	char	*queue_id;
/*
/*	void	bounce_cleanup_log(void)
/*
/*	void	bounce_cleanup_unregister(void)
/* DESCRIPTION
/*	This module implements support for deleting the current
/*	bounce logfile in case of errors, and upon the arrival
/*	of a SIGTERM signal (shutdown).
/*
/*	bounce_cleanup_register() registers a callback routine with the
/*	run-time error handler, for automatic logfile removal in case
/*	of a fatal run-time error.
/*
/*	bounce_cleanup_unregister() cleans up storage used by
/*	bounce_cleanup_register().
/*
/*	In-between bounce_cleanup_register() and bounce_cleanup_unregister()
/*	calls, a call of bounce_cleanup_log() will delete the registered
/*	bounce logfile.
/*
/*	bounce_cleanup_registered() returns non-zero when a cleanup
/*	trap has been set.
/* DIAGNOSTICS
/*	Fatal error: all file access errors. Panic: nested calls of
/*	bounce_cleanup_register(); any calls of bounce_cleanup_unregister()
/*	or bounce_cleanup_log() without preceding bounce_cleanup_register()
/*	call.
/* BUGS
/* SEE ALSO
/*	master(8) process manager
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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <mail_queue.h>

/* Application-specific. */

#include "bounce_service.h"

 /*
  * Support for removing a logfile when an update fails. In order to do this,
  * we save a copy of the currently-open logfile name, and register a
  * callback function pointer with the run-time error handler. The saved
  * pathname is made global so that the application can see whether or not a
  * trap was set up.
  */
static MSG_CLEANUP_FN bounce_cleanup_func;	/* saved callback */
VSTRING *bounce_cleanup_path;		/* saved path name */

/* bounce_cleanup_callback - run-time callback to cleanup logfile */

static void bounce_cleanup_callback(void)
{

    /*
     * Remove the logfile.
     */
    if (bounce_cleanup_path)
	bounce_cleanup_log();

    /*
     * Execute the saved cleanup action.
     */
    if (bounce_cleanup_func)
	bounce_cleanup_func();
}

/* bounce_cleanup_log - clean up the logfile */

void    bounce_cleanup_log(void)
{
    const char *myname = "bounce_cleanup_log";

    /*
     * Sanity checks.
     */
    if (bounce_cleanup_path == 0)
	msg_panic("%s: no cleanup context", myname);

    /*
     * This function may be called before a logfile is created or after it
     * has been deleted, so do not complain.
     */
    (void) unlink(vstring_str(bounce_cleanup_path));
}

/* bounce_cleanup_sig - signal handler */

static void bounce_cleanup_sig(int sig)
{

    /*
     * Running as a signal handler - don't do complicated stuff.
     */
    if (bounce_cleanup_path)
	(void) unlink(vstring_str(bounce_cleanup_path));
    _exit(sig);
}

/* bounce_cleanup_register - register logfile to clean up */

void    bounce_cleanup_register(char *service, char *queue_id)
{
    const char *myname = "bounce_cleanup_register";

    /*
     * Sanity checks.
     */
    if (bounce_cleanup_path)
	msg_panic("%s: nested call", myname);

    /*
     * Save a copy of the logfile path, and of the last callback function
     * pointer registered with the run-time error handler.
     */
    bounce_cleanup_path = vstring_alloc(10);
    (void) mail_queue_path(bounce_cleanup_path, service, queue_id);
    bounce_cleanup_func = msg_cleanup(bounce_cleanup_callback);
    signal(SIGTERM, bounce_cleanup_sig);
}

/* bounce_cleanup_unregister - unregister logfile to clean up */

void    bounce_cleanup_unregister(void)
{
    const char *myname = "bounce_cleanup_unregister";

    /*
     * Sanity checks.
     */
    if (bounce_cleanup_path == 0)
	msg_panic("%s: no cleanup context", myname);

    /*
     * Restore the saved callback function pointer, and release storage for
     * the saved logfile pathname.
     */
    signal(SIGTERM, SIG_DFL);
    (void) msg_cleanup(bounce_cleanup_func);
    vstring_free(bounce_cleanup_path);
    bounce_cleanup_path = 0;
}
