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
/*	sane_connect() implements the accept(2) socket call, and maps
/*	known harmless error results to EAGAIN.
/* BUGS
/*	Bizarre systems may have other harmless error results. Such
/*	systems encourage programers to ignore error results, and
/*	penalizes programmers who code defensively.
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
     */
#if defined(BROKEN_READ_SELECT_ON_TCP_SOCKET) && defined(SO_KEEPALIVE)
    if (sa->sa_family == AF_INET) {
	int     on = 1;

	(void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			  (char *) &on, sizeof(on));
    }
#endif
    return (connect(sock, sa, len));
}
