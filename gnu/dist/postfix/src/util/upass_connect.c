/*++
/* NAME
/*	upass_connect 3
/* SUMMARY
/*	connect to UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <connect.h>
/*
/*	int	upass_connect(addr, block_mode, timeout)
/*	const char *addr;
/*	int	block_mode;
/*	int	timeout;
/* DESCRIPTION
/*	upass_connect() connects to a file descriptor listener in
/*	the UNIX domain at the specified address, sends one half
/*	of a socketpair to the listener, and returns the other half
/*	to the caller.
/*
/*	The file descriptor transporting connection is closed by
/*	a background thread. Some kernels might otherwise discard
/*	the descriptor before the server has received it.
/*
/*	Arguments:
/* .IP addr
/*	Null-terminated string with connection destination.
/* .IP block_mode
/*	Either NON_BLOCKING for a non-blocking socket, or BLOCKING for
/*	blocking mode. This setting has no effect on the connection
/*	establishment process.
/* .IP timeout
/*	Bounds the number of seconds that the operation may take. Specify
/*	a value <= 0 to disable the time limit.
/* DIAGNOSTICS
/*	The result is -1 in case the connection could not be made.
/*	Fatal errors: other system call failures.
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

/* System interfaces. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>
#include <sane_connect.h>
#include <connect.h>
#include <timed_connect.h>
#include <events.h>
#include <mymalloc.h>
#include <sane_socketpair.h>

 /*
  * Workaround for hostile kernels that don't support graceful shutdown.
  */
struct upass_connect {
    int     fd;
    char   *service;
};

/* upass_connect_event - disconnect from peer */

static void upass_connect_event(int event, char *context)
{
    struct upass_connect *up = (struct upass_connect *) context;
    static const char *myname = "upass_connect_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, up->service);
    event_disable_readwrite(up->fd);
    event_cancel_timer(upass_connect_event, context);
    if (close(up->fd) < 0)
	msg_warn("%s: close %s: %m", myname, up->service);
    myfree(up->service);
    myfree((char *) up);
}

/* upass_connect - connect to UNIX-domain file descriptor listener */

int     upass_connect(const char *addr, int block_mode, int timeout)
{
    struct upass_connect *up;
    int     pair[2];
    int     sock;

    /*
     * Connect.
     */
    if ((sock = unix_connect(addr, BLOCKING, timeout)) < 0)
	return (-1);

    /*
     * Send one socket pair half to the server.
     */
#define OUR_HALF	0
#define THEIR_HALF	1

    if (sane_socketpair(AF_UNIX, SOCK_STREAM, 0, pair) < 0) {
	close(sock);
	return (-1);
    }
    if (unix_send_fd(sock, pair[THEIR_HALF]) < 0) {
	close(pair[THEIR_HALF]);
	close(pair[OUR_HALF]);
	close(sock);
	return (-1);
    }
    close(pair[THEIR_HALF]);

    /*
     * Return the other socket pair half to the caller. Don't close the
     * control socket just yet, but wait until the receiver closes it first.
     * Otherwise, some hostile kernel might discard the socket that we just
     * sent.
     */
    up = (struct upass_connect *) mymalloc(sizeof(*up));
    up->fd = sock;
    up->service = mystrdup(addr);
    if (timeout > 0)
	event_request_timer(upass_connect_event, (char *) up, timeout + 100);
    event_enable_read(sock, upass_connect_event, (char *) up);
    non_blocking(pair[OUR_HALF], block_mode);
    return (pair[OUR_HALF]);
}
