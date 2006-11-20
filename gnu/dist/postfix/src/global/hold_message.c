/*	$NetBSD: hold_message.c,v 1.1.1.2.2.2 2006/11/20 13:30:24 tron Exp $	*/

/*++
/* NAME
/*	hold_message 3
/* SUMMARY
/*	move message to hold queue
/* SYNOPSIS
/*	#include <hold_message.h>
/*
/*	int	hold_message(path_buf, queue_name, queue_id)
/*	VSTRING *path_buf;
/*	const char *queue_name;
/*	const char *queue_id;
/* DESCRIPTION
/*	The \fBhold_message\fR() routine moves the specified
/*	queue file to the \fBhold\fR queue, where it will sit
/*	until someone either destroys it or releases it.
/*
/*	Arguments:
/* .IP path_buf
/*	A null pointer, or storage for the new pathname.
/* .IP queue_name
/*	Queue name with the message that needs to be placed on hold.
/* .IP queue_id
/*	Queue file name with the message that needs to be placed on hold.
/* DIAGNOSTICS
/*	The result is -1 in case of failure, 0 in case of success.
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
#include <stdio.h>			/* rename() */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <set_eugid.h>
#include <sane_fsops.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_params.h>
#include <hold_message.h>

#define STR(x)	vstring_str(x)

/* hold_message - move message to hold queue */

int     hold_message(VSTRING *path_buf, const char *queue_name,
		             const char *queue_id)
{
    VSTRING *old_path = vstring_alloc(100);
    VSTRING *new_path = 0;
    uid_t   saved_uid;
    gid_t   saved_gid;
    int     err;

    /*
     * If not running as the mail system, change privileges first.
     */
    if ((saved_uid = geteuid()) != var_owner_uid) {
	saved_gid = getegid();
	set_eugid(var_owner_uid, var_owner_gid);
    }

    /*
     * Your buffer or mine?
     */
    if (path_buf == 0)
	new_path = path_buf = vstring_alloc(100);

    /*
     * This code duplicates mail_queue_rename(), except that it also returns
     * the result pathname to the caller.
     */
    (void) mail_queue_path(old_path, queue_name, queue_id);
    (void) mail_queue_path(path_buf, MAIL_QUEUE_HOLD, queue_id);
    if ((err = sane_rename(STR(old_path), STR(path_buf))) == 0
	|| ((err = mail_queue_mkdirs(STR(path_buf)) == 0)
	    && (err = sane_rename(STR(old_path), STR(path_buf))) == 0)) {
	if (msg_verbose)
	    msg_info("%s: placed on hold", queue_id);
    }

    /*
     * Restore privileges.
     */
    if (saved_uid != var_owner_uid)
	set_eugid(saved_uid, saved_gid);

    /*
     * Cleanup.
     */
    vstring_free(old_path);
    if (new_path)
	vstring_free(new_path);

    return (err);
}
