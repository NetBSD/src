/*++
/* NAME
/*	inet_connect 3
/* SUMMARY
/*	connect to INET-domain listener
/* SYNOPSIS
/*	#include <connect.h>
/*
/*	int	inet_connect(addr, block_mode, timeout)
/*	const char *addr;
/*	int	block_mode;
/*	int	timeout;
/* DESCRIPTION
/*	inet_connect connects to a listener in the AF_INET domain at
/*	the specified address, and returns the resulting file descriptor.
/*
/*	Arguments:
/* .IP addr
/*	The destination to connect to. The format is host:port. If no
/*	host is specified, a port on the local host is assumed.
/*	Host and port information may be given in numerical form
/*	or as symbolical names.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* .IP timeout
/*	Bounds the number of seconds that the operation may take. Specify
/*	a value <= 0 to disable the time limit.
/* DIAGNOSTICS
/*	The result is -1 when the connection could not be made.
/*	Th nature of the error is available via the global \fIerrno\fR
/*	variable.
/*	Fatal errors: other system call failures.
/* BUGS
/*	This routine uses find_inet_addr() which ignores all but the
/*	first address listed for the named host.
/* SEE ALSO
/*	find_inet(3), simple inet name service interface
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
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef INET6
#include <netdb.h>
#endif

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "find_inet.h"
#include "inet_util.h"
#include "iostuff.h"
#include "connect.h"
#include "timed_connect.h"

/* inet_connect - connect to AF_INET-domain listener */

int     inet_connect(const char *addr, int block_mode, int timeout)
{
    char   *buf;
    char   *host;
    char   *port;
#ifdef INET6
    struct addrinfo hints, *res, *res0;
    int    error;
#else
    struct sockaddr_in sin;
#endif
    int     sock;

    /*
     * Translate address information to internal form. No host defaults to
     * the local host.
     */
    buf = inet_parse(addr, &host, &port);
#ifdef INET6
    if (*host == 0)
	host = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;	/* find_inet_addr is numeric only */
    if (getaddrinfo(host, port, &hints, &res0))
	msg_fatal("host not found: %s", host);
#else
    if (*host == 0)
	host = "localhost";
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = find_inet_addr(host);
    sin.sin_port = find_inet_port(port, "tcp");
#endif
    myfree(buf);

#ifdef INET6
    sock = -1;
    for (res = res0; res; res = res->ai_next) {
	if ((res->ai_family != AF_INET) && (res->ai_family != AF_INET6))
	    continue;

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock < 0)
	    continue;
	if (timeout > 0) {
	    non_blocking(sock, NON_BLOCKING);
	    if (timed_connect(sock, res->ai_addr, res->ai_addrlen, timeout) < 0) {
		close(sock);
		sock = -1;
		continue;
	    }
	    if (block_mode != NON_BLOCKING)
		non_blocking(sock, block_mode);
	    break;
	} else {
	    non_blocking(sock, block_mode);
	    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0
		&& errno != EINPROGRESS) {
		close(sock);
		sock = -1;
		continue;
	    }
	    break;
	}
    }
    freeaddrinfo(res0);
    return sock;
#else
    /*
     * Create a client socket.
     */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	msg_fatal("socket: %m");

    /*
     * Timed connect.
     */
    if (timeout > 0) {
	non_blocking(sock, NON_BLOCKING);
	if (timed_connect(sock, (struct sockaddr *) & sin, sizeof(sin), timeout) < 0) {
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
	if (connect(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0
	    && errno != EINPROGRESS) {
	    close(sock);
	    return (-1);
	}
	return (sock);
    }
#endif
}
