/*	$NetBSD: unix_trigger.c,v 1.1.1.1.2.2 2009/09/15 06:04:04 snj Exp $	*/

/*++
/* NAME
/*	unix_trigger 3
/* SUMMARY
/*	wakeup UNIX-domain server
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	unix_trigger(service, buf, len, timeout)
/*	const char *service;
/*	const char *buf;
/*	ssize_t	len;
/*	int	timeout;
/* DESCRIPTION
/*	unix_trigger() wakes up the named UNIX-domain server by making
/*	a brief connection to it and writing the named buffer.
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
/* SEE ALSO
/*	unix_connect(3), UNIX-domain client
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

struct unix_trigger {
    int     fd;
    char   *service;
};

/* unix_trigger_event - disconnect from peer */

static void unix_trigger_event(int event, char *context)
{
    struct unix_trigger *up = (struct unix_trigger *) context;
    static const char *myname = "unix_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, up->service);
    event_disable_readwrite(up->fd);
    event_cancel_timer(unix_trigger_event, context);
    if (close(up->fd) < 0)
	msg_warn("%s: close %s: %m", myname, up->service);
    myfree(up->service);
    myfree((char *) up);
}

/* unix_trigger - wakeup UNIX-domain server */

int     unix_trigger(const char *service, const char *buf, ssize_t len, int timeout)
{
    const char *myname = "unix_trigger";
    struct unix_trigger *up;
    int     fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((fd = unix_connect(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * Stash away context.
     */
    up = (struct unix_trigger *) mymalloc(sizeof(*up));
    up->fd = fd;
    up->service = mystrdup(service);

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
	event_request_timer(unix_trigger_event, (char *) up, timeout + 100);
    event_enable_read(fd, unix_trigger_event, (char *) up);
    return (0);
}
