/*++
/* NAME
/*	inet_listen 3
/* SUMMARY
/*	start INET-domain listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	inet_listen(addr, backlog, block_mode)
/*	const char *addr;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	inet_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	The \fBinet_listen\fR routine starts a listener in the INET domain
/*	on the specified address, with the specified backlog, and returns
/*	the resulting file descriptor.
/*
/*	inet_accept() accepts a connection and sanitizes error results.
/*
/*	Arguments:
/* .IP addr
/*	The communication endpoint to listen on. The syntax is "host:port".
/*	Host and port may be specified in symbolic form or numerically.
/*	A null host field means listen on all network interfaces.
/* .IP backlog
/*	This argument is passed on to the \fIlisten(2)\fR routine.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode.
/* .IP fd
/*	File descriptor returned by inet_listen().
/* DIAGNOSTICS
/*	Fatal errors: inet_listen() aborts upon any system call failure.
/*	inet_accept() leaves all error handling up to the caller.
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

/* System libraries. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef MAXHOSTNAMELEN
#include <sys/param.h>
#endif
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "find_inet.h"
#include "inet_util.h"
#include "iostuff.h"
#include "listen.h"
#include "sane_accept.h"

/* Application-specific stuff. */

#ifndef INADDR_ANY
#define INADDR_ANY	0xffffffff
#endif

/* inet_listen - create inet-domain listener */

int     inet_listen(const char *addr, int backlog, int block_mode)
{
    struct sockaddr_in sin;
    int     sock;
    int     t = 1;
    char   *buf;
    char   *host;
    char   *port;

    /*
     * Translate address information to internal form.
     */
    buf = inet_parse(addr, &host, &port);
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = find_inet_port(port, "tcp");
    sin.sin_addr.s_addr = (*host ? find_inet_addr(host) : INADDR_ANY);
    myfree(buf);

    /*
     * Create a listener socket.
     */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	msg_fatal("socket: %m");
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &t, sizeof(t)) < 0)
	msg_fatal("setsockopt: %m");
    if (bind(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
	msg_fatal("bind %s port %d: %m", sin.sin_addr.s_addr == INADDR_ANY ?
	       "INADDR_ANY" : inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    non_blocking(sock, block_mode);
    if (listen(sock, backlog) < 0)
	msg_fatal("listen: %m");
    return (sock);
}

/* inet_accept - accept connection */

int     inet_accept(int fd)
{
    struct sockaddr_in sin;
    SOCKADDR_SIZE len = sizeof(sin);

    return (sane_accept(fd, (struct sockaddr *) & sin, &len));
}
