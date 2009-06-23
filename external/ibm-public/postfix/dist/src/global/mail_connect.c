/*	$NetBSD: mail_connect.c,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

/*++
/* NAME
/*	mail_connect 3
/* SUMMARY
/*	intra-mail system connection management
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	VSTREAM *mail_connect(class, name, block_mode)
/*	const char *class;
/*	const char *name;
/*	int	block_mode;
/*
/*	VSTREAM *mail_connect_wait(class, name)
/*	const char *class;
/*	const char *name;
/* DESCRIPTION
/*	This module does low-level connection management for intra-mail
/*	communication.  All reads and writes are subject to a time limit
/*	(controlled by the global variable \fIvar_ipc_timeout\fR). This
/*	protects against deadlock conditions that should never happen.
/*
/*	mail_connect() attempts to connect to the UNIX-domain socket of
/*	the named subsystem. The result is a null pointer in case of failure.
/*
/*	mail_connect_wait() is like mail_connect(), but keeps trying until
/*	the connection succeeds. However, mail_connect_wait() terminates
/*	with a fatal error when the service is down. This is to ensure that
/*	processes terminate when the mail system shuts down.
/*
/*	Arguments:
/* .IP class
/*	Name of a class of local transport channel endpoints,
/*	either \fIpublic\fR (accessible by any local user) or
/*	\fIprivate\fR (administrative access only).
/* .IP service
/*	The name of a local transport endpoint within the named class.
/* .IP block_mode
/*	NON_BLOCKING for a non-blocking connection, or BLOCKING.
/* SEE ALSO
/*	timed_ipc(3), enforce IPC timeouts.
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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <connect.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <stringops.h>

/* Global library. */

#include "timed_ipc.h"
#include "mail_proto.h"

/* mail_connect - connect to mail subsystem */

VSTREAM *mail_connect(const char *class, const char *name, int block_mode)
{
    char   *path;
    VSTREAM *stream;
    int     fd;
    char   *sock_name;

    path = mail_pathname(class, name);
    if ((fd = LOCAL_CONNECT(path, block_mode, 0)) < 0) {
	if (msg_verbose)
	    msg_info("connect to subsystem %s: %m", path);
	stream = 0;
    } else {
	if (msg_verbose)
	    msg_info("connect to subsystem %s", path);
	stream = vstream_fdopen(fd, O_RDWR);
	timed_ipc_setup(stream);
	sock_name = concatenate(path, " socket", (char *) 0);
	vstream_control(stream,
			VSTREAM_CTL_PATH, sock_name,
			VSTREAM_CTL_END);
	myfree(sock_name);
    }
    myfree(path);
    return (stream);
}

/* mail_connect_wait - connect to mail service until it succeeds */

VSTREAM *mail_connect_wait(const char *class, const char *name)
{
    VSTREAM *stream;
    int     count = 0;

    /*
     * XXX Solaris workaround for ECONNREFUSED on a busy socket.
     */
    while ((stream = mail_connect(class, name, BLOCKING)) == 0) {
	if (count++ >= 10) {
	    msg_fatal("connect #%d to subsystem %s/%s: %m",
		      count, class, name);
	} else {
	    msg_warn("connect #%d to subsystem %s/%s: %m",
		     count, class, name);
	}
	sleep(10);				/* XXX make configurable */
    }
    return (stream);
}
