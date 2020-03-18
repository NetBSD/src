/*	$NetBSD: unsafe.c,v 1.2 2020/03/18 19:05:22 christos Exp $	*/

/*++
/* NAME
/*	unsafe 3
/* SUMMARY
/*	are we running at non-user privileges
/* SYNOPSIS
/*	#include <safe.h>
/*
/*	int	unsafe()
/* DESCRIPTION
/*	The \fBunsafe()\fR routine attempts to determine if the process
/*	(runs with privileges or has access to information) that the
/*	controlling user has no access to. The purpose is to prevent
/*	misuse of privileges, including access to protected information.
/*
/*	The result is always false when both of the following conditions
/*	are true:
/* .IP \(bu
/*	The real UID is zero.
/* .IP \(bu
/*	The effective UID is zero.
/* .PP
/*	Otherwise, the result is true if any of the following conditions
/*	is true:
/* .IP \(bu
/*	The issetuid kernel flag is non-zero (on systems that support
/*	this concept).
/* .IP \(bu
/*	The real and effective user id differ.
/* .IP \(bu
/*	The real and effective group id differ.
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
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>

/* Utility library. */

#include "safe.h"

/* unsafe - can we trust user-provided environment, working directory, etc. */

int     unsafe(void)
{

    /*
     * The super-user is trusted.
     */
    if (getuid() == 0 && geteuid() == 0)
	return (0);

    /*
     * Danger: don't trust inherited process attributes, and don't leak
     * privileged info that the parent has no access to.
     */
    return (geteuid() != getuid()
#ifdef HAS_ISSETUGID
	    || issetugid()
#endif
	    || getgid() != getegid());
}
