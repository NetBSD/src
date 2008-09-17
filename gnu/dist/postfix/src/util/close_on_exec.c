/*++
/* NAME
/*	close_on_exec 3
/* SUMMARY
/*	set/clear close-on-exec flag
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	close_on_exec(int fd, int on)
/* DESCRIPTION
/*	the \fIclose_on_exec\fR() function manipulates the close-on-exec
/*	flag for the specified open file, and returns the old setting.
/*
/*	Arguments:
/* .IP fd
/*	A file descriptor.
/* .IP on
/*	Use CLOSE_ON_EXEC or PASS_ON_EXEC.
/* DIAGNOSTICS
/*	All errors are fatal.
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

/* System interfaces. */

#include <sys_defs.h>
#include <fcntl.h>

/* Utility library. */

#include "msg.h"

/* Application-specific. */

#include "iostuff.h"

#define PATTERN	FD_CLOEXEC

/* close_on_exec - set/clear close-on-exec flag */

int     close_on_exec(fd, on)
int     fd;
int     on;
{
    int     flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
	msg_fatal("fcntl: get flags: %m");
    if (fcntl(fd, F_SETFD, on ? flags | PATTERN : flags & ~PATTERN) < 0)
	msg_fatal("fcntl: set close-on-exec flag %s: %m", on ? "on" : "off");
    return ((flags & PATTERN) != 0);
}
