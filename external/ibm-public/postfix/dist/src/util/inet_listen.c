/*	$NetBSD: inet_listen.c,v 1.1.1.1.2.2 2009/09/15 06:03:59 snj Exp $	*/

/*++
/* NAME
/*	inet_listen 3
/* SUMMARY
/*	start TCP listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	inet_windowsize;
/*
/*	int	inet_listen(addr, backlog, block_mode)
/*	const char *addr;
/*	int	backlog;
/*	int	block_mode;
/*
/*	int	inet_accept(fd)
/*	int	fd;
/* DESCRIPTION
/*	The \fBinet_listen\fR routine starts a TCP listener
/*	on the specified address, with the specified backlog, and returns
/*	the resulting file descriptor.
/*
/*	inet_accept() accepts a connection and sanitizes error results.
/*
/*	Specify an inet_windowsize value > 0 to override the TCP
/*	window size that the server advertises to the client.
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
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "host_port.h"
#include "iostuff.h"
#include "listen.h"
#include "sane_accept.h"
#include "myaddrinfo.h"
#include "sock_addr.h"
#include "inet_proto.h"

/* inet_listen - create TCP listener */

int     inet_listen(const char *addr, int backlog, int block_mode)
{
    struct addrinfo *res;
    struct addrinfo *res0;
    int     aierr;
    int     sock;
    int     on = 1;
    char   *buf;
    char   *host;
    char   *port;
    const char *parse_err;
    MAI_HOSTADDR_STR hostaddr;
    MAI_SERVPORT_STR portnum;
    INET_PROTO_INFO *proto_info;

    /*
     * Translate address information to internal form.
     */
    buf = mystrdup(addr);
    if ((parse_err = host_port(buf, &host, "", &port, (char *) 0)) != 0)
	msg_fatal("%s: %s", addr, parse_err);
    if (*host == 0)
	host = 0;
    if ((aierr = hostname_to_sockaddr(host, port, SOCK_STREAM, &res0)) != 0)
	msg_fatal("%s: %s", addr, MAI_STRERROR(aierr));
    myfree(buf);
    /* No early returns or res0 leaks. */

    proto_info = inet_proto_info();
    for (res = res0; /* see below */ ; res = res->ai_next) {

	/*
	 * No usable address found.
	 */
	if (res == 0)
	    msg_fatal("%s: host found but no usable address", addr);

	/*
	 * Safety net.
	 */
	if (strchr((char *) proto_info->sa_family_list, res->ai_family) != 0)
	    break;

	msg_info("skipping address family %d for %s", res->ai_family, addr);
    }

    /*
     * Show what address we're trying.
     */
    if (msg_verbose) {
	SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
			     &hostaddr, &portnum, 0);
	msg_info("trying... [%s]:%s", hostaddr.buf, portnum.buf);
    }

    /*
     * Create a listener socket.
     */
    if ((sock = socket(res->ai_family, res->ai_socktype, 0)) < 0)
	msg_fatal("socket: %m");
#ifdef HAS_IPV6
# if defined(IPV6_V6ONLY) && !defined(BROKEN_AI_PASSIVE_NULL_HOST)
    if (res->ai_family == AF_INET6
	&& setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
		      (char *) &on, sizeof(on)) < 0)
	msg_fatal("setsockopt(IPV6_V6ONLY): %m");
# endif
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &on, sizeof(on)) < 0)
	msg_fatal("setsockopt(SO_REUSEADDR): %m");
    if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
	SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
			     &hostaddr, &portnum, 0);
	msg_fatal("bind %s port %s: %m", hostaddr.buf, portnum.buf);
    }
    freeaddrinfo(res0);
    non_blocking(sock, block_mode);
    if (inet_windowsize > 0)
	set_inet_windowsize(sock, inet_windowsize);
    if (listen(sock, backlog) < 0)
	msg_fatal("listen: %m");
    return (sock);
}

/* inet_accept - accept connection */

int     inet_accept(int fd)
{
    struct sockaddr_storage ss;
    SOCKADDR_SIZE ss_len = sizeof(ss);

    return (sane_accept(fd, (struct sockaddr *) & ss, &ss_len));
}
