/*++
/* NAME
/*	unix_connect 3
/* SUMMARY
/*	connect to UNIX-domain listener
/* SYNOPSIS
/*	#include <unix_connect.h>
/*
/*	int	unix_connect(addr, block_mode, timeout)
/*	const char *addr;
/*	int	block_mode;
/*	int	timeout;
/* DESCRIPTION
/*	unix_connect() connects to a listener in the UNIX domain at the
/*	specified address, and returns the resulting file descriptor.
/*
/*	Arguments:
/* .IP addr
/*	Null-terminated string with connection destination.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* .IP timeout
/*	Bounds the number of seconds that the operation may take. Specify
/*	a value <= 0 to disable the time limit.
/* DIAGNOSTICS
/*	The result is -1 in case the connection could not be made.
/*	Fatal errors: other system call failures.
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
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"
#include "sane_connect.h"
#include "connect.h"
#include "timed_connect.h"

/* unix_connect - connect to UNIX-domain listener */

int     unix_connect(const char *addr, int block_mode, int timeout)
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
     * Create a client socket.
     */
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	msg_fatal("socket: %m");

    /*
     * Timed connect.
     */
    if (timeout > 0) {
	non_blocking(sock, NON_BLOCKING);
	if (timed_connect(sock, (struct sockaddr *) & sun, sizeof(sun), timeout) < 0) {
	    close(sock);
	    return (-1);
	}
	if (block_mode != NON_BLOCKING)
	    non_blocking(sock, block_mode);
	return (sock);
    }

    /*
     * Maybe block until connected.
     */
    else {
	non_blocking(sock, block_mode);
	if (sane_connect(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0
	    && errno != EINPROGRESS) {
	    close(sock);
	    return (-1);
	}
	return (sock);
    }
}
