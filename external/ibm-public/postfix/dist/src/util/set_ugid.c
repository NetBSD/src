/*	$NetBSD: set_ugid.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	set_ugid 3
/* SUMMARY
/*	set real, effective and saved user and group attributes
/* SYNOPSIS
/*	#include <set_ugid.h>
/*
/*	void	set_ugid(uid, gid)
/*	uid_t	uid;
/*	gid_t	gid;
/* DESCRIPTION
/*	set_ugid() sets the real, effective and saved user and group process
/*	attributes and updates the process group access list to be just the
/*	user's primary group. This operation is irreversible.
/* DIAGNOSTICS
/*	All system call errors are fatal.
/* SEE ALSO
/*	setuid(2), setgid(2), setgroups(2)
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
#include <grp.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "set_ugid.h"

/* set_ugid - set real, effective and saved user and group attributes */

void    set_ugid(uid_t uid, gid_t gid)
{
    int     saved_errno = errno;

    if (geteuid() != 0)
	if (seteuid(0) < 0)
	    msg_fatal("seteuid(0): %m");
    if (setgid(gid) < 0)
	msg_fatal("setgid(%ld): %m", (long) gid);
    if (setgroups(1, &gid) < 0)
	msg_fatal("setgroups(1, &%ld): %m", (long) gid);
    if (setuid(uid) < 0)
	msg_fatal("setuid(%ld): %m", (long) uid);
    if (msg_verbose > 1)
	msg_info("setugid: uid %ld gid %ld", (long) uid, (long) gid);
    errno = saved_errno;
}
