/*	$NetBSD: mark_corrupt.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	mark_corrupt 3
/* SUMMARY
/*	mark queue file as corrupt
/* SYNOPSIS
/*	#include <mark_corrupt.h>
/*
/*	char	*mark_corrupt(src)
/*	VSTREAM *src;
/* DESCRIPTION
/*	The \fBmark_corrupt\fR() routine marks the specified open
/*	queue file as corrupt, and returns a suitable delivery status
/*	so that the queue manager will do the right thing.
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
#include <sys/stat.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <set_eugid.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_params.h>
#include <deliver_request.h>
#include <mark_corrupt.h>

/* mark_corrupt - mark queue file as corrupt */

int     mark_corrupt(VSTREAM *src)
{
    const char *myname = "mark_corrupt";
    uid_t   saved_uid;
    gid_t   saved_gid;

    /*
     * If not running as the mail system, change privileges first.
     */
    if ((saved_uid = geteuid()) != var_owner_uid) {
	saved_gid = getegid();
	set_eugid(var_owner_uid, var_owner_gid);
    }

    /*
     * For now, the result value is -1; this may become a bit mask, or
     * something even more advanced than that, when the delivery status
     * becomes more than just done/deferred.
     */
    msg_warn("corrupted queue file: %s", VSTREAM_PATH(src));
    if (fchmod(vstream_fileno(src), MAIL_QUEUE_STAT_CORRUPT))
	msg_fatal("%s: fchmod %s: %m", myname, VSTREAM_PATH(src));

    /*
     * Restore privileges.
     */
    if (saved_uid != var_owner_uid)
	set_eugid(saved_uid, saved_gid);

    return (DEL_STAT_DEFER);
}
