/*	$NetBSD: timed_read.c,v 1.1.1.1.2.2 2009/09/15 06:04:04 snj Exp $	*/

/*++
/* NAME
/*	timed_read 3
/* SUMMARY
/*	read operation with pre-read timeout
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	ssize_t	timed_read(fd, buf, len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	size_t	len;
/*	int	timeout;
/*	void	*context;
/* DESCRIPTION
/*	timed_read() performs a read() operation when the specified
/*	descriptor becomes readable within a user-specified deadline.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE.
/* .IP buf
/*	Read buffer pointer.
/* .IP len
/*	Read buffer size.
/* .IP timeout
/*	The deadline in seconds. If this is <= 0, the deadline feature
/*	is disabled.
/* .IP context
/*	Application context. This parameter is unused. It exists only
/*	for the sake of VSTREAM compatibility.
/* DIAGNOSTICS
/*	When the operation does not complete within the deadline, the
/*	result value is -1, and errno is set to ETIMEDOUT.
/*	All other returns are identical to those of a read(2) operation.
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
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* timed_read - read with deadline */

ssize_t timed_read(int fd, void *buf, size_t len,
		           int timeout, void *unused_context)
{
    ssize_t ret;

    /*
     * Wait for a limited amount of time for something to happen. If nothing
     * happens, report an ETIMEDOUT error.
     * 
     * XXX Solaris 8 read() fails with EAGAIN after read-select() returns
     * success.
     */
    for (;;) {
	if (timeout > 0 && read_wait(fd, timeout) < 0)
	    return (-1);
	if ((ret = read(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) {
	    msg_warn("read() returns EAGAIN on a readable file descriptor!");
	    msg_warn("pausing to avoid going into a tight select/read loop!");
	    sleep(1);
	    continue;
	} else if (ret < 0 && errno == EINTR) {
	    continue;
	} else {
	    return (ret);
	}
    }
}
