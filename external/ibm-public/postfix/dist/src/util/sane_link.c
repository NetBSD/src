/*	$NetBSD: sane_link.c,v 1.1.1.2 2013/01/02 18:59:14 tron Exp $	*/

/*++
/* NAME
/*	sane_link 3
/* SUMMARY
/*	sanitize link() error returns
/* SYNOPSIS
/*	#include <sane_fsops.h>
/*
/*	int	sane_link(from, to)
/*	const char *from;
/*	const char *to;
/* DESCRIPTION
/*	sane_link() implements the link(2) system call, and works
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
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "sane_fsops.h"
#include "warn_stat.h"

/* sane_link - sanitize link() error returns */

int     sane_link(const char *from, const char *to)
{
    const char *myname = "sane_link";
    int     saved_errno;
    struct stat from_st;
    struct stat to_st;

    /*
     * Normal case: link() succeeds.
     */
    if (link(from, to) >= 0)
	return (0);

    /*
     * Woops. Save errno, and see if the error is an NFS artefact. If it is,
     * pretend the error never happened.
     */
    saved_errno = errno;
    if (stat(from, &from_st) >= 0 && stat(to, &to_st) >= 0
	&& from_st.st_dev == to_st.st_dev
	&& from_st.st_ino == to_st.st_ino) {
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
