/*	$NetBSD: unix_listen.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	unix_listen 3
/* SUMMARY
/*	start UNIX-domain listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	unix_listen(addr, backlog, block_mode)
/*	const char *addr;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	unix_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	The \fBunix_listen\fR() routine starts a listener in the UNIX domain
/*	on the specified address, with the specified backlog, and returns
/*	the resulting file descriptor.
/*
/*	unix_accept() accepts a connection and sanitizes error results.
/*
/*	Arguments:
/* .IP addr
/*	Null-terminated string with connection destination.
/* .IP backlog
/*	This argument is passed on to the \fIlisten(2)\fR routine.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* .IP fd
/*	File descriptor returned by unix_listen().
/* DIAGNOSTICS
/*	Fatal errors: unix_listen() aborts upon any system call failure.
/*	unix_accept() leaves all error handling up to the caller.
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
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"
#include "listen.h"
#include "sane_accept.h"

/* unix_listen - create UNIX-domain listener */

int     unix_listen(const char *addr, int backlog, int block_mode)
{
#undef sun
    struct sockaddr_un sun;
    int     len = strlen(addr);
    int     sock;

    /*
     * Translate address information to internal form.
     */
    if (len >= (int) sizeof(sun.sun_path))
	msg_fatal("unix-domain name too long: %s", addr);
    memset((char *) &sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
    sun.sun_len = len + 1;
#endif
    memcpy(sun.sun_path, addr, len + 1);

    /*
     * Create a listener socket. Do whatever we can so we don't run into
     * trouble when this process is restarted after crash.
     */
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	msg_fatal("socket: %m");
    if (unlink(addr) < 0 && errno != ENOENT)
	msg_fatal("remove %s: %m", addr);
    if (bind(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0)
	msg_fatal("bind: %s: %m", addr);
#ifdef FCHMOD_UNIX_SOCKETS
    if (fchmod(sock, 0666) < 0)
	msg_fatal("fchmod socket %s: %m", addr);
#else
    if (chmod(addr, 0666) < 0)
	msg_fatal("chmod socket %s: %m", addr);
#endif
    non_blocking(sock, block_mode);
    if (listen(sock, backlog) < 0)
	msg_fatal("listen: %m");
    return (sock);
}

/* unix_accept - accept connection */

int     unix_accept(int fd)
{
    return (sane_accept(fd, (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0));
}
