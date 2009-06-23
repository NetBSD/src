/*	$NetBSD: mail_scan_dir.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	mail_scan_dir 3
/* SUMMARY
/*	mail queue directory scanning support
/* SYNOPSIS
/*	#include <mail_scan_dir.h>
/*
/*	char	*mail_scan_dir_next(scan)
/*	SCAN_DIR *scan;
/* DESCRIPTION
/*	The \fBmail_scan_dir_next\fR() routine is a wrapper around
/*	scan_dir_next() that understands the structure of a Postfix
/*	mail queue.  The result is a queue ID or a null pointer.
/* SEE ALSO
/*	scan_dir(3) directory scanner
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
#include <string.h>

/* Utility library. */

#include <scan_dir.h>

/* Global library. */

#include <mail_scan_dir.h>

/* mail_scan_dir_next - return next queue file */

char   *mail_scan_dir_next(SCAN_DIR *scan)
{
    char   *name;

    /*
     * Exploit the fact that mail queue subdirectories have one-letter names,
     * so we don't have to stat() every file in sight. This is a win because
     * many dirent implementations do not return file type information.
     */
    for (;;) {
	if ((name = scan_dir_next(scan)) == 0) {
	    if (scan_dir_pop(scan) == 0)
		return (0);
	} else if (strlen(name) == 1) {
	    scan_dir_push(scan, name);
	} else {
	    return (name);
	}
    }
}
