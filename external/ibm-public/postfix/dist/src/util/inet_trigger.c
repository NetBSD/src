/*	$NetBSD: inet_trigger.c,v 1.1.1.1.2.2 2009/09/15 06:03:59 snj Exp $	*/

/*++
/* NAME
/*	inet_trigger 3
/* SUMMARY
/*	wakeup INET-domain server
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	inet_trigger(service, buf, len, timeout)
/*	char	*service;
/*	const char *buf;
/*	ssize_t	len;
/*	int	timeout;
/* DESCRIPTION
/*	inet_trigger() wakes up the named INET-domain server by making
/*	a brief connection to it and by writing the contents of the
/*	named buffer.
/*
/*	The connection is closed by a background thread. Some kernels
/*	cannot handle client-side disconnect before the server has
/*	received the message.
/*
/*	Arguments:
/* .IP service
/*	Name of the communication endpoint.
/* .IP buf
/*	Address of data to be written.
/* .IP len
/*	Amount of data to be written.
/* .IP timeout
/*	Deadline in seconds. Specify a value <= 0 to disable
/*	the time limit.
/* DIAGNOSTICS
/*	The result is zero in case of success, -1 in case of problems.
/* BUGS
/* SEE ALSO
/*	inet_connect(3), INET-domain client
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
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <connect.h>
#include <iostuff.h>
#include <mymalloc.h>
#include <events.h>
#include <trigger.h>

struct inet_trigger {
    int     fd;
    char   *service;
};

/* inet_trigger_event - disconnect from peer */

static void inet_trigger_event(int event, char *context)
{
    struct inet_trigger *ip = (struct inet_trigger *) context;
    static const char *myname = "inet_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, ip->service);
    event_disable_readwrite(ip->fd);
    event_cancel_timer(inet_trigger_event, context);
    if (close(ip->fd) < 0)
	msg_warn("%s: close %s: %m", myname, ip->service);
    myfree(ip->service);
    myfree((char *) ip);
}


/* inet_trigger - wakeup INET-domain server */

int     inet_trigger(const char *service, const char *buf, ssize_t len, int timeout)
{
    const char *myname = "inet_trigger";
    struct inet_trigger *ip;
    int     fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((fd = inet_connect(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(fd, CLOSE_ON_EXEC);
    ip = (struct inet_trigger *) mymalloc(sizeof(*ip));
    ip->fd = fd;
    ip->service = mystrdup(service);

    /*
     * Write the request...
     */
    if (write_buf(fd, buf, len, timeout) < 0
	|| write_buf(fd, "", 1, timeout) < 0)
	if (msg_verbose)
	    msg_warn("%s: write to %s: %m", myname, service);

    /*
     * Wakeup when the peer disconnects, or when we lose patience.
     */
    if (timeout > 0)
	event_request_timer(inet_trigger_event, (char *) ip, timeout + 100);
    event_enable_read(fd, inet_trigger_event, (char *) ip);
    return (0);
}
