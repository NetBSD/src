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
#ifdef INET6
#if (! __GLIBC__ >= 2 && __GLIBC_MINOR__ >=1 )
#include <netinet6/in6.h>
#endif
#endif
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
#ifdef INET6
    struct addrinfo *res, *res0, hints;
    int error;
#else
    struct ai {
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	struct sockaddr *ai_addr;
	SOCKADDR_SIZE ai_addrlen;
	struct ai *ai_next;
    } *res, *res0, resbody;
    struct sockaddr_in sin;
#endif
    int     sock;
    int     t = 1;
    char   *buf;
    char   *host;
    char   *port;
#ifdef INET6
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
#else
    char hbuf[sizeof("255.255.255.255") + 1];
    char pbuf[sizeof("255.255.255.255") + 1];
#endif
    char *cause = "unknown";

    /*
     * Translate address information to internal form.
     */
    buf = inet_parse(addr, &host, &port);
#ifdef INET6
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(*host ? host : NULL, *port ? port : "0", &hints, &res0);
    if (error) {
	msg_fatal("getaddrinfo: %s", gai_strerror(error));
    }
    myfree(buf);
#else
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
#ifdef HAS_SA_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_port = find_inet_port(port, "tcp");
    sin.sin_addr.s_addr = (*host ? find_inet_addr(host) : INADDR_ANY);

    memset(&resbody, 0, sizeof(resbody)); 
    resbody.ai_socktype = SOCK_STREAM;
    resbody.ai_family = AF_INET;
    resbody.ai_addr = (struct sockaddr *)&sin;
    resbody.ai_addrlen = sizeof(sin);

    res0 = &resbody;
#endif

    sock = -1;
    for (res = res0; res; res = res->ai_next) {
	if ((res->ai_family != AF_INET) && (res->ai_family != AF_INET6))
	    continue;

	/*
	 * Create a listener socket.
	 */
	if ((sock = socket(res->ai_family, res->ai_socktype, 0)) < 0) {
	    cause = "socket";
	    continue;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &t, sizeof(t)) < 0) {
	    cause = "setsockopt";
	    close(sock);
	    sock = -1;
	    continue;
	}
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
	    cause = "bind";
	    close(sock);
	    sock = -1;
	    continue;
	}
	break;
    }
    if (sock < 0)
	msg_fatal("%s: %m", cause);
#ifdef INET6
    freeaddrinfo(res0);
#endif
    non_blocking(sock, block_mode);
    if (listen(sock, backlog) < 0)
	msg_fatal("listen: %m");
    return (sock);
}

/* inet_accept - accept connection */

int     inet_accept(int fd)
{
    return (sane_accept(fd, (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0));
}
