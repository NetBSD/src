/*	$NetBSD: timed_write.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	timed_write 3
/* SUMMARY
/*	write operation with pre-write timeout
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	ssize_t	timed_write(fd, buf, len, timeout, context)
/*	int	fd;
/*	const void *buf;
/*	size_t	len;
/*	int	timeout;
/*	void	*context;
/* DESCRIPTION
/*	timed_write() performs a write() operation when the specified
/*	descriptor becomes writable within a user-specified deadline.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE.
/* .IP buf
/*	Write buffer pointer.
/* .IP len
/*	Write buffer size.
/* .IP timeout
/*	The deadline in seconds. If this is <= 0, the deadline feature
/*	is disabled.
/* .IP context
/*	Application context. This parameter is unused. It exists only
/*	for the sake of VSTREAM compatibility.
/* DIAGNOSTICS
/*	When the operation does not complete within the deadline, the
/*	result value is -1, and errno is set to ETIMEDOUT.
/*	All other returns are identical to those of a write(2) operation.
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

/* timed_write - write with deadline */

ssize_t timed_write(int fd, const void *buf, size_t len,
		            int timeout, void *unused_context)
{
    ssize_t ret;

    /*
     * Wait for a limited amount of time for something to happen. If nothing
     * happens, report an ETIMEDOUT error.
     * 
     * XXX Solaris 8 read() fails with EAGAIN after read-select() returns
     * success. The code below exists just in case their write implementation
     * is equally broken.
     * 
     * This condition may also be found on systems where select() returns
     * success on pipes with less than PIPE_BUF bytes of space, and with
     * badly designed software where multiple writers are fighting for access
     * to the same resource.
     */
    for (;;) {
	if (timeout > 0 && write_wait(fd, timeout) < 0)
	    return (-1);
	if ((ret = write(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) {
	    msg_warn("write() returns EAGAIN on a writable file descriptor!");
	    msg_warn("pausing to avoid going into a tight select/write loop!");
	    sleep(1);
	    continue;
	} else if (ret < 0 && errno == EINTR) {
	    continue;
	} else {
	    return (ret);
	}
    }
}
