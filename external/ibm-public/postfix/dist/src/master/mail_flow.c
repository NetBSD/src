/*	$NetBSD: mail_flow.c,v 1.1.1.1 2009/06/23 10:08:49 tron Exp $	*/

/*++
/* NAME
/*	mail_flow 3
/* SUMMARY
/*	global mail flow control
/* SYNOPSIS
/*	#include <mail_flow.h>
/*
/*	ssize_t	mail_flow_get(count)
/*	ssize_t	count;
/*
/*	ssize_t	mail_flow_put(count)
/*	ssize_t	count;
/*
/*	ssize_t	mail_flow_count()
/* DESCRIPTION
/*	This module implements a simple flow control mechanism that
/*	is based on tokens that are consumed by mail receiving processes
/*	and that are produced by mail sending processes.
/*
/*	mail_flow_get() attempts to read specified number of tokens. The
/*	result is > 0 for success, < 0 for failure. In the latter case,
/*	the process is expected to slow down a little.
/*
/*	mail_flow_put() produces the specified number of tokens. The
/*	token producing process is expected to produce new tokens
/*	whenever it falls idle and no more tokens are available.
/*
/*	mail_flow_count() returns the number of available tokens.
/* BUGS
/*	The producer needs to wake up periodically to ensure that
/*	tokens are not lost due to leakage.
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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* Global library. */

#include <mail_flow.h>

/* Master library. */

#include <master_proto.h>

#define BUFFER_SIZE	1024

/* mail_flow_get - read N tokens */

ssize_t mail_flow_get(ssize_t len)
{
    const char *myname = "mail_flow_get";
    char    buf[BUFFER_SIZE];
    struct stat st;
    ssize_t count;
    ssize_t n = 0;

    /*
     * Sanity check.
     */
    if (len <= 0)
	msg_panic("%s: bad length %ld", myname, (long) len);

    /*
     * Silence some wild claims.
     */
    if (fstat(MASTER_FLOW_WRITE, &st) < 0)
	msg_fatal("fstat flow pipe write descriptor: %m");

    /*
     * Read and discard N bytes. XXX AIX read() can return 0 when an open
     * pipe is empty.
     */
    for (count = len; count > 0; count -= n)
	if ((n = read(MASTER_FLOW_READ, buf, count > BUFFER_SIZE ?
		      BUFFER_SIZE : count)) <= 0)
	    return (-1);
    if (msg_verbose)
	msg_info("%s: %ld %ld", myname, (long) len, (long) (len - count));
    return (len - count);
}

/* mail_flow_put - put N tokens */

ssize_t mail_flow_put(ssize_t len)
{
    const char *myname = "mail_flow_put";
    char    buf[BUFFER_SIZE];
    ssize_t count;
    ssize_t n = 0;

    /*
     * Sanity check.
     */
    if (len <= 0)
	msg_panic("%s: bad length %ld", myname, (long) len);

    /*
     * Write or discard N bytes.
     */
    memset(buf, 0, len > BUFFER_SIZE ? BUFFER_SIZE : len);

    for (count = len; count > 0; count -= n)
	if ((n = write(MASTER_FLOW_WRITE, buf, count > BUFFER_SIZE ?
		       BUFFER_SIZE : count)) < 0)
	    return (-1);
    if (msg_verbose)
	msg_info("%s: %ld %ld", myname, (long) len, (long) (len - count));
    return (len - count);
}

/* mail_flow_count - return number of available tokens */

ssize_t mail_flow_count(void)
{
    const char *myname = "mail_flow_count";
    ssize_t count;

    if ((count = peekfd(MASTER_FLOW_READ)) < 0)
	msg_warn("%s: %m", myname);
    return (count);
}
