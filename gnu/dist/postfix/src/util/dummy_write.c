/*	$NetBSD: dummy_write.c,v 1.1.1.1 2005/08/18 21:10:54 rpaulo Exp $	*/

/*++
/* NAME
/*	dummy_write 3
/* SUMMARY
/*	dummy write operation
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	dummy_write(fd, buf, buf_len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	unsigned len;
/*	int	timeout;
/*	void	*context;
/* DESCRIPTION
/*	dummy_write() implements a data sink without side effects.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor in the range 0..FD_SETSIZE. Its value is logged
/*	when verbose logging is turned on.
/* .IP buf
/*	Write buffer pointer. Not used.
/* .IP buf_len
/*	Write buffer size. Its value is logged when verbose logging is
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

/* dummy_write - dummy write operation */

int     dummy_write(int fd, void *unused_buf, unsigned len,
		           int unused_timeout, void *unused_context)
{
    if (msg_verbose)
	msg_info("dummy_write: fd %d, len %d", fd, len);
    return (len);
}
