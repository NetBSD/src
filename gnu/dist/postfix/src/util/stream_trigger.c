/*++
/* NAME
/*	stream_trigger 3
/* SUMMARY
/*	wakeup stream server
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	stream_trigger(service, buf, len, timeout)
/*	const char *service;
/*	const char *buf;
/*	int	len;
/*	int	timeout;
/* DESCRIPTION
/*	stream_trigger() wakes up the named stream server by making
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
/*	stream_connect(3), stream client
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
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <connect.h>
#include <iostuff.h>
#include <mymalloc.h>
#include <events.h>
#include <trigger.h>

struct stream_trigger {
    int     fd;
    char   *service;
};

/* stream_trigger_event - disconnect from peer */

static void stream_trigger_event(int event, char *context)
{
    struct stream_trigger *sp = (struct stream_trigger *) context;
    static char *myname = "stream_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, sp->service);
    event_disable_readwrite(sp->fd);
    event_cancel_timer(stream_trigger_event, context);
    if (close(sp->fd) < 0)
	msg_warn("%s: close %s: %m", myname, sp->service);
    myfree(sp->service);
    myfree((char *) sp);
}

/* stream_trigger - wakeup stream server */

int     stream_trigger(const char *service, const char *buf, int len, int timeout)
{
    char   *myname = "stream_trigger";
    struct stream_trigger *sp;
    int     fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((fd = stream_connect(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * Stash away context.
     */
    sp = (struct stream_trigger *) mymalloc(sizeof(*sp));
    sp->fd = fd;
    sp->service = mystrdup(service);

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
	event_request_timer(stream_trigger_event, (char *) sp, timeout + 100);
    event_enable_read(fd, stream_trigger_event, (char *) sp);
    return (0);
}
