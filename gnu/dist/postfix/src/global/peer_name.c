/*++
/* NAME
/*	peer_name 3
/* SUMMARY
/*	produce printable peer name and address
/* SYNOPSIS
/*	#include <peer_name.h>
/*
/*	typedef struct {
/* .in +4
/*		int	type;
/*		char	name;
/*		char	addr;
/* .in -4
/*	} PEER_NAME;
/*
/*	PEER_NAME *peer_name(sock)
/*	int	sock;
/* DESCRIPTION
/*	The \fIpeer_name\fR() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	The result is in static memory that will be overwritten.
/*	Make a copy if the result is to be used for an appreciable
/*	amount of time.
/*
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*	The \fItype\fR result field specifies how the name and address
/*	should be interpreted:
/* .IP PEER_TYPE_INET
/*	The socket specifies a TCP/IP endpoint.
/*	The result is a hostname (from the DNS, a local hosts file or
/*	other); the address a dotted quad.
/* .IP PEER_TYPE_LOCAL
/*	The socket argument specifies a local transport.
/*	The result name is "localhost"; the result address is "127.0.0.1".
/* .IP PEER_TYPE_UNKNOWN
/*	The socket argument does not specify a socket.
/*	The result name is "localhost"; the result address is "127.0.0.1".
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
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

/* Utility library. */

#include <msg.h>
#include <valid_hostname.h>
#include <peer_name.h>

/* peer_name - produce printable peer name and address */

PEER_NAME *peer_name(int sock)
{
    static PEER_NAME peer;
    struct sockaddr_in sin;
    SOCKADDR_SIZE len = sizeof(sin);
    struct hostent *hp;

    if (getpeername(sock, (struct sockaddr *) & sin, &len) == 0) {
	switch (sin.sin_family) {
	case AF_INET:
	    peer.type = PEER_TYPE_INET;
	    hp = gethostbyaddr((char *) &(sin.sin_addr),
			       sizeof(sin.sin_addr), AF_INET);
	    peer.name = (hp && valid_hostname(hp->h_name, DO_GRIPE) ?
			 hp->h_name : "unknown");
	    peer.addr = inet_ntoa(sin.sin_addr);
	    return (&peer);
	case AF_UNSPEC:
	case AF_UNIX:
	    peer.type = PEER_TYPE_LOCAL;
	    peer.name = "localhost";
	    peer.addr = "127.0.0.1";
	    return (&peer);
	}
    }
    peer.type = PEER_TYPE_UNKNOWN;
    peer.name = "localhost";
    peer.addr = "127.0.0.1";
    return (&peer);

}

#ifdef TEST

#include <unistd.h>

int     main(int unused_argc, char **unused_argv)
{
    PEER_NAME *peer;

    peer = peer_name(STDIN_FILENO);
    msg_info("name %s addr %s", peer->name, peer->addr);
}

#endif
