/*++
/* NAME
/*	qmgr_scan 3
/* SUMMARY
/*	queue scanning
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_SCAN *qmgr_scan_create(queue_name)
/*	const char *queue_name;
/*
/*	char	*qmgr_scan_next(scan_info)
/*	QMGR_SCAN *scan_info;
/*
/*	void	qmgr_scan_request(scan_info, flags)
/*	QMGR_SCAN *scan_info;
/*	int	flags;
/* DESCRIPTION
/*	This module implements queue scans. A queue scan always runs
/*	to completion, so that all files get a fair chance. The caller
/*	can request that a queue scan be restarted once it completes.
/*
/*	qmgr_scan_create() creates a context for scanning the named queue,
/*	but does not start a queue scan.
/*
/*	qmgr_scan_next() returns the base name of the next queue file.
/*	A null pointer means that no file was found. qmgr_scan_next()
/*	automagically restarts a queue scan when a scan request had
/*	arrived while the scan was in progress.
/*
/*	qmgr_scan_request() records a request for the next queue scan. The
/*	flags argument is the bit-wise OR of zero or more of the following,
/*	unrecognized flags being ignored:
/* .IP QMGR_FLUSH_DEAD
/*	Forget state information about dead hosts or transports. This
/*	request takes effect upon the next queue scan.
/* .IP QMGR_SCAN_ALL
/*	Ignore queue file time stamps.
/*	This flag is passed on to the qmgr_active_feed() routine.
/* .IP QMGR_SCAN_START
/*	Start a queue scan when none is in progress, or restart the
/*	current scan upon completion.
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*	Panic: interface violations, internal consistency errors.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <scan_dir.h>

/* Global library. */

#include <mail_scan_dir.h>

/* Application-specific. */

#include "qmgr.h"

/* qmgr_scan_start - start queue scan */

static void qmgr_scan_start(QMGR_SCAN *scan_info)
{
    char   *myname = "qmgr_scan_start";

    /*
     * Sanity check.
     */
    if (scan_info->handle)
	msg_panic("%s: %s queue scan in progress",
		  myname, scan_info->queue);

    /*
     * Give the poor tester a clue.
     */
    if (msg_verbose)
	msg_info("%s: %sstart %s queue scan",
		 myname,
		 scan_info->nflags & QMGR_SCAN_START ? "re" : "",
		 scan_info->queue);

    /*
     * Optionally forget all dead host information.
     */
    if (scan_info->nflags & QMGR_FLUSH_DEAD)
	qmgr_enable_all();

    /*
     * Start or restart the scan.
     */
    scan_info->flags = scan_info->nflags;
    scan_info->nflags = 0;
    scan_info->handle = scan_dir_open(scan_info->queue);
}

/* qmgr_scan_request - request for future scan */

void    qmgr_scan_request(QMGR_SCAN *scan_info, int flags)
{

    /*
     * If a scan is in progress, just record the request.
     */
    scan_info->nflags |= flags;
    if (scan_info->handle == 0 && (flags & QMGR_SCAN_START) != 0) {
	scan_info->nflags &= ~QMGR_SCAN_START;
	qmgr_scan_start(scan_info);
    }
}

/* qmgr_scan_next - look for next queue file */

char   *qmgr_scan_next(QMGR_SCAN *scan_info)
{
    char   *path = 0;

    /*
     * Restart the scan if we reach the end and a queue scan request has
     * arrived in the mean time.
     */
    if (scan_info->handle && (path = mail_scan_dir_next(scan_info->handle)) == 0) {
	scan_info->handle = scan_dir_close(scan_info->handle);
	if (msg_verbose && (scan_info->nflags & QMGR_SCAN_START) == 0)
	    msg_info("done %s queue scan", scan_info->queue);
    }
    if (!scan_info->handle && (scan_info->nflags & QMGR_SCAN_START)) {
	qmgr_scan_start(scan_info);
	path = mail_scan_dir_next(scan_info->handle);
    }
    return (path);
}

/* qmgr_scan_create - create queue scan context */

QMGR_SCAN *qmgr_scan_create(const char *queue)
{
    QMGR_SCAN *scan_info;

    scan_info = (QMGR_SCAN *) mymalloc(sizeof(*scan_info));
    scan_info->queue = mystrdup(queue);
    scan_info->flags = scan_info->nflags = 0;
    scan_info->handle = 0;
    return (scan_info);
}
