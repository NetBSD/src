/*++
/* NAME
/*	timed_read 3
/* SUMMARY
/*	read operation with pre-read timeout
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	timed_read(fd, buf, buf_len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	unsigned len;
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
/* .IP buf_len
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

/* Utility library. */

#include "iostuff.h"

/* timed_read - read with deadline */

int     timed_read(int fd, void *buf, unsigned len,
		           int timeout, void *unused_context)
{

    /*
     * Wait for a limited amount of time for something to happen. If nothing
     * happens, report an ETIMEDOUT error.
     */
    if (timeout > 0 && read_wait(fd, timeout) < 0)
	return (-1);
    else
	return (read(fd, buf, len));
}
