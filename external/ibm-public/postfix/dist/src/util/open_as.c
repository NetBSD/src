/*	$NetBSD: open_as.c,v 1.2 2026/05/09 18:49:23 christos Exp $	*/

/*++
/* NAME
/*	open_as 3
/* SUMMARY
/*	open file as user
/* SYNOPSIS
/*	#include <fcntl.h>
/*	#include <open_as.h>
/*
/*	int	open_as(path, flags, mode, euid, egid)
/*	const char *path;
/*	int	mode;
/*	uid_t	euid;
/*
/*	int	vstream_fopen_as(path, flags, mode, euid, egid)
/*	const char *path;
/*	int	flags;
/*	mode_t	mode;
/*	uid_t	euid;
/*	gid_t	egid;
/* DESCRIPTION
/*	open_as() opens the named \fIpath\fR with the named \fIflags\fR
/*	and \fImode\fR, and with the effective rights specified by \fIeuid\fR
/*	and \fIegid\fR.  A -1 result means the open failed.
/*
/*	vstream_fopen_as() implements a VSTREAM wrapper with the same
/*	functionality.
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
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

/* System library. */

#include <sys_defs.h>
#include <fcntl.h>
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "set_eugid.h"
#include "open_as.h"
#include "vstream.h"

/* open_as - open file as user */

int     open_as(const char *path, int flags, int mode, uid_t euid, gid_t egid)
{
    uid_t   saved_euid = geteuid();
    gid_t   saved_egid = getegid();
    int     fd;

    /*
     * Switch to the target user privileges.
     */
    set_eugid(euid, egid);

    /*
     * Open that file.
     */
    fd = open(path, flags, mode);

    /*
     * Restore saved privileges.
     */
    set_eugid(saved_euid, saved_egid);

    return (fd);
}

/* vstream_fopen_as - VSTREAM wrapper */

VSTREAM *vstream_fopen_as(const char *path, int flags, mode_t mode,
			          uid_t euid, gid_t egid)
{
    int     fd;

    if ((fd = open(path, flags, mode)) < 0)
	return (0);
    return (vstream_fdopen(fd, flags));
}
