/*	$NetBSD: sane_connect.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	sane_connect 3
/* SUMMARY
/*	sanitize connect() results
/* SYNOPSIS
/*	#include <sane_connect.h>
/*
/*	int	sane_connect(sock, buf, len)
/*	int	sock;
/*	struct sockaddr	*buf;
/*	SOCKADDR_SIZE *len;
/* DESCRIPTION
/*	sane_connect() implements the connect(2) socket call, and maps
/*	known harmless error results to EAGAIN.
/* BUGS
/*	Bizarre systems may have other harmless error results. Such
/*	systems encourage programmers to ignore error results, and
/*	penalize programmers who code defensively.
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
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "sane_connect.h"

/* sane_connect - sanitize connect() results */

int     sane_connect(int sock, struct sockaddr * sa, SOCKADDR_SIZE len)
{

    /*
     * XXX Solaris select() produces false read events, so that read() blocks
     * forever on a blocking socket, and fails with EAGAIN on a non-blocking
     * socket. Turning on keepalives will fix a blocking socket provided that
     * the kernel's keepalive timer expires before the Postfix watchdog
     * timer.
     * 
     * XXX Work around NAT induced damage by sending a keepalive before an idle
     * connection is expired. This requires that the kernel keepalive timer
     * is set to a short time, like 100s.
     */
    if (sa->sa_family == AF_INET) {
	int     on = 1;

	(void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			  (char *) &on, sizeof(on));
    }
    return (connect(sock, sa, len));
}
