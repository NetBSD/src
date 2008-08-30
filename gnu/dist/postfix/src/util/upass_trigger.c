/*++
/* NAME
/*	upass_trigger 3
/* SUMMARY
/*	wakeup UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	upass_trigger(service, buf, len, timeout)
/*	const char *service;
/*	const char *buf;
/*	ssize_t	len;
/*	int	timeout;
/* DESCRIPTION
/*	upass_trigger() wakes up the named UNIX-domain server by sending
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
/*	upass_connect(3), UNIX-domain client
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

struct upass_trigger {
    int     fd;
    char   *service;
};

/* upass_trigger_event - disconnect from peer */

static void upass_trigger_event(int event, char *context)
{
    struct upass_trigger *up = (struct upass_trigger *) context;
    static const char *myname = "upass_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, up->service);
    event_disable_readwrite(up->fd);
    event_cancel_timer(upass_trigger_event, context);
    if (close(up->fd) < 0)
	msg_warn("%s: close %s: %m", myname, up->service);
    myfree(up->service);
    myfree((char *) up);
}

/* upass_trigger - wakeup UNIX-domain server */

int     upass_trigger(const char *service, const char *buf, ssize_t len, int timeout)
{
    const char *myname = "upass_trigger";
    struct upass_trigger *up;
    int     fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((fd = upass_connect(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * Stash away context.
     */
    up = (struct upass_trigger *) mymalloc(sizeof(*up));
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
	event_request_timer(upass_trigger_event, (char *) up, timeout + 100);
    event_enable_read(fd, upass_trigger_event, (char *) up);
    return (0);
}
