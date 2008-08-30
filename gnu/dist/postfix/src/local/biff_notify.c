/*++
/* NAME
/*	biff_notify 3
/* SUMMARY
/*	send biff notification
/* SYNOPSIS
/*	#include <biff_notify.h>
/*
/*	void	biff_notify(text, len)
/*	const char *text;
/*	ssize_t	len;
/* DESCRIPTION
/*	biff_notify() sends a \fBBIFF\fR notification request to the
/*	\fBcomsat\fR daemon.
/*
/*	Arguments:
/* .IP text
/*	Null-terminated text (username@mailbox-offset).
/* .IP len
/*	Length of text, including null terminator.
/* BUGS
/*	The \fBBIFF\fR "service" can be a noticeable load for
/*	systems that have many logged-in users.
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

#include "sys_defs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

/* Utility library. */

#include <msg.h>

/* Application-specific. */

#include <biff_notify.h>

/* biff_notify - notify recipient via the biff "protocol" */

void    biff_notify(const char *text, ssize_t len)
{
    static struct sockaddr_in sin;
    static int sock = -1;
    struct hostent *hp;
    struct servent *sp;

    /*
     * Initialize a socket address structure, or re-use an existing one.
     */
    if (sin.sin_family == 0) {
	if ((sp = getservbyname("biff", "udp")) == 0) {
	    msg_warn("service not found: biff/udp");
	    return;
	}
	if ((hp = gethostbyname("localhost")) == 0) {
	    msg_warn("host not found: localhost");
	    return;
	}
	if ((int) hp->h_length > (int) sizeof(sin.sin_addr)) {
	    msg_warn("bad address size %d for localhost", hp->h_length);
	    return;
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = sp->s_port;
	memcpy((char *) &sin.sin_addr, hp->h_addr_list[0], hp->h_length);
    }

    /*
     * Open a socket, or re-use an existing one.
     */
    if (sock < 0 && (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	msg_warn("socket: %m");
	return;
    }

    /*
     * Biff!
     */
    if (sendto(sock, text, len, 0, (struct sockaddr *) & sin, sizeof(sin)) != len)
	msg_warn("biff_notify: %m");
}
