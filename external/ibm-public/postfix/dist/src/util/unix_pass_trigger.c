/*	$NetBSD: unix_pass_trigger.c,v 1.1.1.2 2012/08/10 12:35:59 tron Exp $	*/

/*++
/* NAME
/*	unix_pass_trigger 3
/* SUMMARY
/*	wakeup UNIX-domain file descriptor listener
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	unix_pass_trigger(service, buf, len, timeout)
/*	const char *service;
/*	const char *buf;
/*	ssize_t	len;
/*	int	timeout;
/* DESCRIPTION
/*	unix_pass_trigger() wakes up the named UNIX-domain server by sending
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
/*	unix_pass_connect(3), UNIX-domain client
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

struct unix_pass_trigger {
    int     fd;
    char   *service;
    int     pair[2];
};

/* unix_pass_trigger_event - disconnect from peer */

static void unix_pass_trigger_event(int event, char *context)
{
    struct unix_pass_trigger *up = (struct unix_pass_trigger *) context;
    static const char *myname = "unix_pass_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, up->service);
    event_disable_readwrite(up->fd);
    event_cancel_timer(unix_pass_trigger_event, context);
    /* Don't combine multiple close() calls into one boolean expression. */
    if (close(up->fd) < 0)
	msg_warn("%s: close %s: %m", myname, up->service);
    if (close(up->pair[0]) < 0)
	msg_warn("%s: close pipe: %m", myname);
    if (close(up->pair[1]) < 0)
	msg_warn("%s: close pipe: %m", myname);
    myfree(up->service);
    myfree((char *) up);
}

/* unix_pass_trigger - wakeup UNIX-domain server */

int     unix_pass_trigger(const char *service, const char *buf, ssize_t len, int timeout)
{
    const char *myname = "unix_pass_trigger";
    int     pair[2];
    struct unix_pass_trigger *up;
    int     fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((fd = unix_pass_connect(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * Create a pipe, and send one pipe end to the server.
     */
    if (pipe(pair) < 0)
	msg_fatal("%s: pipe: %m", myname);
    close_on_exec(pair[0], CLOSE_ON_EXEC);
    close_on_exec(pair[1], CLOSE_ON_EXEC);
    if (unix_send_fd(fd, pair[0]) < 0)
	msg_fatal("%s: send file descriptor: %m", myname);

    /*
     * Stash away context.
     */
    up = (struct unix_pass_trigger *) mymalloc(sizeof(*up));
    up->fd = fd;
    up->service = mystrdup(service);
    up->pair[0] = pair[0];
    up->pair[1] = pair[1];

    /*
     * Write the request...
     */
    if (write_buf(pair[1], buf, len, timeout) < 0
	|| write_buf(pair[1], "", 1, timeout) < 0)
	if (msg_verbose)
	    msg_warn("%s: write to %s: %m", myname, service);

    /*
     * Wakeup when the peer disconnects, or when we lose patience.
     */
    if (timeout > 0)
	event_request_timer(unix_pass_trigger_event, (char *) up, timeout + 100);
    event_enable_read(fd, unix_pass_trigger_event, (char *) up);
    return (0);
}
