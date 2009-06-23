/*	$NetBSD: qmgr_move.c,v 1.1.1.1 2009/06/23 10:08:53 tron Exp $	*/

/*++
/* NAME
/*	qmgr_move 3
/* SUMMARY
/*	move queue entries to another queue
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	void	qmgr_move(from, to, time_stamp)
/*	const char *from;
/*	const char *to;
/*	time_t	time_stamp;
/* DESCRIPTION
/*	The \fBqmgr_move\fR routine scans the \fIfrom\fR queue for entries
/*	with valid queue names and moves them to the \fIto\fR queue.
/*	If \fItime_stamp\fR is non-zero, the queue file time stamps are
/*	set to the specified value.
/*	Entries with invalid names are left alone. No attempt is made to
/*	look for other badness such as multiple links or weird file types.
/*	These issues are dealt with when a queue file is actually opened.
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
#include <string.h>
#include <utime.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <scan_dir.h>
#include <recipient_list.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_scan_dir.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_move - move queue entries to another queue, leave bad files alone */

void    qmgr_move(const char *src_queue, const char *dst_queue,
		          time_t time_stamp)
{
    const char *myname = "qmgr_move";
    SCAN_DIR *queue_dir;
    char   *queue_id;
    struct utimbuf tbuf;
    const char *path;

    if (strcmp(src_queue, dst_queue) == 0)
	msg_panic("%s: source queue %s is destination", myname, src_queue);
    if (msg_verbose)
	msg_info("start move queue %s -> %s", src_queue, dst_queue);

    queue_dir = scan_dir_open(src_queue);
    while ((queue_id = mail_scan_dir_next(queue_dir)) != 0) {
	if (mail_queue_id_ok(queue_id)) {
	    if (time_stamp > 0) {
		tbuf.actime = tbuf.modtime = time_stamp;
		path = mail_queue_path((VSTRING *) 0, src_queue, queue_id);
		if (utime(path, &tbuf) < 0) {
		    if (errno != ENOENT)
			msg_fatal("%s: update %s time stamps: %m", myname, path);
		    msg_warn("%s: update %s time stamps: %m", myname, path);
		    continue;
		}
	    }
	    if (mail_queue_rename(queue_id, src_queue, dst_queue)) {
		if (errno != ENOENT)
		    msg_fatal("%s: rename %s from %s to %s: %m",
			      myname, queue_id, src_queue, dst_queue);
		msg_warn("%s: rename %s from %s to %s: %m",
			 myname, queue_id, src_queue, dst_queue);
		continue;
	    }
	    if (msg_verbose)
		msg_info("%s: moved %s from %s to %s",
			 myname, queue_id, src_queue, dst_queue);
	} else {
	    msg_warn("%s: ignored: queue %s id %s",
		     myname, src_queue, queue_id);
	}
    }
    scan_dir_close(queue_dir);

    if (msg_verbose)
	msg_info("end move queue %s -> %s", src_queue, dst_queue);
}
