/*++
/* NAME
/*	write_buf 3
/* SUMMARY
/*	write buffer or bust
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	write_buf(fd, buf, len, timeout)
/*	int	fd;
/*	const char *buf;
/*	int	len;
/*	int	timeout;
/* DESCRIPTION
/*	write_buf() writes a buffer to the named stream in as many
/*	fragments as needed, and returns the number of bytes written,
/*	which is always the number requested or an error indication.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE.
/* .IP buf
/*	Address of data to be written.
/* .IP len
/*	Amount of data to be written.
/* .IP timeout
/*	Bounds the time in seconds to wait until \fIfd\fD becomes writable.
/*	A value <= 0 means do not wait; this is useful only when \fIfd\fR
/*	uses blocking I/O.
/* DIAGNOSTICS
/*	write_buf() returns -1 in case of trouble. The global \fIerrno\fR
/*	variable reflects the nature of the problem.
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

/* write_buf - write buffer or bust */

int     write_buf(int fd, const char *buf, int len, int timeout)
{
    int     count;

    while (len > 0) {
	if (timeout > 0 && write_wait(fd, timeout) < 0)
	    return (-1);
	if ((count = write(fd, buf, len)) < 0) {
	    if (errno == EAGAIN && timeout > 0)
		continue;
	    return (-1);
	}
	if (count == 0)
	    msg_fatal("write returned 0");
	buf += count;
	len -= count;
    }
    return (len);
}
