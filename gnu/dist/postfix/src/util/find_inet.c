/*++
/* NAME
/*	find_inet 3
/* SUMMARY
/*	inet-domain name services
/* SYNOPSIS
/*	#include <find_inet.h>
/*
/*	unsigned find_inet_addr(host)
/*	const char *host;
/*
/*	int	find_inet_port(port, proto)
/*	const char *port;
/*	const char *proto;
/* DESCRIPTION
/*	These functions translate network address information from
/*	between printable form to the internal the form used by the
/*	BSD TCP/IP network software.
/*
/*	find_inet_addr() translates a symbolic or numerical hostname.
/*
/*	find_inet_port() translates a symbolic or numerical port name.
/* BUGS
/*	find_inet_addr() ignores all but the first address listed for
/*	a symbolic hostname.
/* DIAGNOSTICS
/*	Lookup and conversion errors are fatal.
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
#include <stdlib.h>
#include <string.h>

/* Application-specific. */

#include "msg.h"
#include "stringops.h"
#include "find_inet.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* find_inet_addr - translate numerical or symbolic host name */

unsigned find_inet_addr(const char *host)
{
    struct in_addr addr;
    struct hostent *hp;

    addr.s_addr = inet_addr(host);
    if ((addr.s_addr == INADDR_NONE) || (addr.s_addr == 0)) {
	if ((hp = gethostbyname(host)) == 0)
	    msg_fatal("host not found: %s", host);
	if (hp->h_addrtype != AF_INET)
	    msg_fatal("unexpected address family: %d", hp->h_addrtype);
	if (hp->h_length != sizeof(addr))
	    msg_fatal("unexpected address length %d", hp->h_length);
	memcpy((char *) &addr, hp->h_addr, hp->h_length);
    }
    return (addr.s_addr);
}

/* find_inet_port - translate numerical or symbolic service name */

int     find_inet_port(const char *service, const char *protocol)
{
    struct servent *sp;
    int     port;

    if (alldig(service) && (port = atoi(service)) != 0) {
	return (htons(port));
    } else {
	if ((sp = getservbyname(service, protocol)) == 0)
	    msg_fatal("unknown service: %s/%s", service, protocol);
	return (sp->s_port);
    }
}
