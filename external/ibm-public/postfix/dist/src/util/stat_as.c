/*	$NetBSD: stat_as.c,v 1.1.1.2 2013/01/02 18:59:14 tron Exp $	*/

/*++
/* NAME
/*	stat_as 3
/* SUMMARY
/*	stat file as user
/* SYNOPSIS
/*	#include <sys/stat.h>
/*	#include <stat_as.h>
/*
/*	int	stat_as(path, st, euid, egid)
/*	const char *path;
/*	struct stat *st;
/*	uid_t	euid;
/*	gid_t	egid;
/* DESCRIPTION
/*	stat_as() looks up the file status of the named \fIpath\fR,
/*	using the effective rights specified by \fIeuid\fR
/*	and \fIegid\fR, and stores the result into the structure pointed
/*	to by \fIst\fR.  A -1 result means the lookup failed.
/*	This call follows symbolic links.
/* DIAGNOSTICS
/*	Fatal error: no permission to change privilege level.
/* SEE ALSO
/*	set_eugid(3) switch effective rights
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

#include "msg.h"
#include "set_eugid.h"
#include "stat_as.h"
#include "warn_stat.h"

/* stat_as - stat file as user */

int     stat_as(const char *path, struct stat * st, uid_t euid, gid_t egid)
{
    uid_t   saved_euid = geteuid();
    gid_t   saved_egid = getegid();
    int     status;

    /*
     * Switch to the target user privileges.
     */
    set_eugid(euid, egid);

    /*
     * Stat that file.
     */
    status = stat(path, st);

    /*
     * Restore saved privileges.
     */
    set_eugid(saved_euid, saved_egid);

    return (status);
}
