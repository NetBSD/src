/*	$NetBSD: dot_lockfile_as.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	dot_lockfile_as 3
/* SUMMARY
/*	dotlock file as user
/* SYNOPSIS
/*	#include <dot_lockfile_as.h>
/*
/*	int	dot_lockfile_as(path, why, euid, egid)
/*	const char *path;
/*	VSTRING *why;
/*	uid_t	euid;
/*	gid_t	egid;
/*
/*	void	dot_unlockfile_as(path, euid, egid)
/*	const char *path;
/*	uid_t	euid;
/*	gid_t	egid;
/* DESCRIPTION
/*	dot_lockfile_as() and dot_unlockfile_as() are wrappers around
/*	the dot_lockfile() and dot_unlockfile() routines. The routines
/*	change privilege to the designated privilege, perform the
/*	requested operation, and restore privileges.
/* DIAGNOSTICS
/*	Fatal error: no permission to change privilege level.
/* SEE ALSO
/*	dot_lockfile(3) dotlock file management
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
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "set_eugid.h"
#include "dot_lockfile.h"
#include "dot_lockfile_as.h"

/* dot_lockfile_as - dotlock file as user */

int     dot_lockfile_as(const char *path, VSTRING *why, uid_t euid, gid_t egid)
{
    uid_t   saved_euid = geteuid();
    gid_t   saved_egid = getegid();
    int     result;

    /*
     * Switch to the target user privileges.
     */
    set_eugid(euid, egid);

    /*
     * Lock that file.
     */
    result = dot_lockfile(path, why);

    /*
     * Restore saved privileges.
     */
    set_eugid(saved_euid, saved_egid);

    return (result);
}

/* dot_unlockfile_as - dotlock file as user */

void     dot_unlockfile_as(const char *path, uid_t euid, gid_t egid)
{
    uid_t   saved_euid = geteuid();
    gid_t   saved_egid = getegid();

    /*
     * Switch to the target user privileges.
     */
    set_eugid(euid, egid);

    /*
     * Lock that file.
     */
    dot_unlockfile(path);

    /*
     * Restore saved privileges.
     */
    set_eugid(saved_euid, saved_egid);
}
