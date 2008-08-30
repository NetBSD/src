/*++
/* NAME
/*	dummy_read 3
/* SUMMARY
/*	dummy read operation
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	ssize_t	dummy_read(fd, buf, buf_len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	size_t	len;
/*	int	timeout;
/*	void	*context;
/* DESCRIPTION
/*	dummy_read() reports an EOF condition without side effects.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor whose value is logged when verbose logging
/*	is turned on.
/* .IP buf
/*	Read buffer pointer. Not used.
/* .IP buf_len
/*	Read buffer size. Its value is logged when verbose logging is
/*	turned on.
/* .IP timeout
/*	The deadline in seconds. Not used.
/* .IP context
/*	Application context. Not used.
/* DIAGNOSTICS
/*	None.
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

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* dummy_read - dummy read operation */

ssize_t dummy_read(int fd, void *unused_buf, size_t len,
		           int unused_timeout, void *unused_context)
{
    if (msg_verbose)
	msg_info("dummy_read: fd %d, len %lu", fd, (unsigned long) len);
    return (0);
}
