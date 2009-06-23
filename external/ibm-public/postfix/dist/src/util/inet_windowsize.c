/*	$NetBSD: inet_windowsize.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	inet_windowsize 3
/* SUMMARY
/*	TCP window scaling control
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	inet_windowsize;
/*
/*	void	set_inet_windowsize(sock, windowsize)
/*	int	sock;
/*	int	windowsize;
/* DESCRIPTION
/*	set_inet_windowsize() overrides the default TCP window size
/*	with the specified value. When called before listen() or
/*	accept(), this works around broken infrastructure that
/*	mis-handles TCP window scaling options.
/*
/*	The global inet_windowsize variable is available for other
/*	routines to remember that they wish to override the default
/*	TCP window size. The variable is not accessed by the
/*	set_inet_windowsize() function itself.
/*
/*	Arguments:
/* .IP sock
/*	TCP communication endpoint, before the connect(2) or listen(2) call.
/* .IP windowsize
/*	The preferred TCP window size. This must be > 0.
/* DIAGNOSTICS
/*	Panic: interface violation.
/*	Warnings: some error return from setsockopt().
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

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* Application storage. */

 /*
  * Tunable to work around broken routers.
  */
int     inet_windowsize = 0;

/* set_inet_windowsize - set TCP send/receive window size */

void    set_inet_windowsize(int sock, int windowsize)
{

    /*
     * Sanity check.
     */
    if (windowsize <= 0)
	msg_panic("inet_windowsize: bad window size %d", windowsize);

    /*
     * Generic implementation: set the send and receive buffer size before
     * listen() or connect().
     */
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &windowsize,
		   sizeof(windowsize)) < 0)
	msg_warn("setsockopt SO_SNDBUF %d: %m", windowsize);
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &windowsize,
		   sizeof(windowsize)) < 0)
	msg_warn("setsockopt SO_RCVBUF %d: %m", windowsize);
}
