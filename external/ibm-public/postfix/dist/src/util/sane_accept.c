/*	$NetBSD: sane_accept.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	sane_accept 3
/* SUMMARY
/*	sanitize accept() error returns
/* SYNOPSIS
/*	#include <sane_accept.h>
/*
/*	int	sane_accept(sock, buf, len)
/*	int	sock;
/*	struct sockaddr	*buf;
/*	SOCKADDR_SIZE *len;
/* DESCRIPTION
/*	sane_accept() implements the accept(2) socket call, and maps
/*	known harmless error results to EAGAIN.
/*
/*	If the buf and len arguments are not null, then additional
/*	workarounds may be enabled that depend on the socket type.
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
#include "sane_accept.h"

/* sane_accept - sanitize accept() error returns */

int     sane_accept(int sock, struct sockaddr * sa, SOCKADDR_SIZE *len)
{
    static int accept_ok_errors[] = {
	EAGAIN,
	ECONNREFUSED,
	ECONNRESET,
	EHOSTDOWN,
	EHOSTUNREACH,
	EINTR,
	ENETDOWN,
	ENETUNREACH,
	ENOTCONN,
	EWOULDBLOCK,
	ENOBUFS,			/* HPUX11 */
	ECONNABORTED,
#ifdef EPROTO
	EPROTO,				/* SunOS 5.5.1 */
#endif
	0,
    };
    int     count;
    int     err;
    int     fd;

    /*
     * XXX Solaris 2.4 accept() returns EPIPE when a UNIX-domain client has
     * disconnected in the mean time. From then on, UNIX-domain sockets are
     * hosed beyond recovery. There is no point treating this as a beneficial
     * error result because the program would go into a tight loop.
     * 
     * XXX Solaris 2.5.1 accept() returns EPROTO when a TCP client has
     * disconnected in the mean time. Since there is no connection, it is
     * safe to map the error code onto EAGAIN.
     * 
     * XXX LINUX < 2.1 accept() wakes up before the three-way handshake is
     * complete, so it can fail with ECONNRESET and other "false alarm"
     * indications.
     * 
     * XXX FreeBSD 4.2-STABLE accept() returns ECONNABORTED when a UNIX-domain
     * client has disconnected in the mean time. The data that was sent with
     * connect() write() close() is lost, even though the write() and close()
     * reported successful completion. This was fixed shortly before FreeBSD
     * 4.3.
     * 
     * XXX HP-UX 11 returns ENOBUFS when the client has disconnected in the mean
     * time.
     */
    if ((fd = accept(sock, sa, len)) < 0) {
	for (count = 0; (err = accept_ok_errors[count]) != 0; count++) {
	    if (errno == err) {
		errno = EAGAIN;
		break;
	    }
	}
    }

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
    else if (sa && (sa->sa_family == AF_INET
#ifdef HAS_IPV6
		    || sa->sa_family == AF_INET6
#endif
		    )) {
	int     on = 1;

	(void) setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
			  (char *) &on, sizeof(on));
    }
    return (fd);
}
