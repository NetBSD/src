/*	$NetBSD: sane_rename.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	sane_rename 3
/* SUMMARY
/*	sanitize rename() error returns
/* SYNOPSIS
/*	#include <sane_fsops.h>
/*
/*	int	sane_rename(old, new)
/*	const char *from;
/*	const char *to;
/* DESCRIPTION
/*	sane_rename() implements the rename(2) system call, and works
/*	around some errors that are possible with NFS file systems.
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

#include "sys_defs.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>			/* rename(2) syscall in stdio.h? */

/* Utility library. */

#include "msg.h"
#include "sane_fsops.h"

/* sane_rename - sanitize rename() error returns */

int     sane_rename(const char *from, const char *to)
{
    const char *myname = "sane_rename";
    int     saved_errno;
    struct stat st;

    /*
     * Normal case: rename() succeeds.
     */
    if (rename(from, to) >= 0)
	return (0);

    /*
     * Woops. Save errno, and see if the error is an NFS artefact. If it is,
     * pretend the error never happened.
     */
    saved_errno = errno;
    if (stat(from, &st) < 0 && stat(to, &st) >= 0) {
	msg_info("%s(%s,%s): worked around spurious NFS error",
		 myname, from, to);
	return (0);
    }

    /*
     * Nope, it didn't. Restore errno and report the error.
     */
    errno = saved_errno;
    return (-1);
}
