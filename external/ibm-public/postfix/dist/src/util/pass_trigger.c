/*	$NetBSD: pass_trigger.c,v 1.1.1.1.8.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	pass_trigger 3
/* SUMMARY
/*	trigger file descriptor listener
/* SYNOPSIS
/*	#include <trigger.h>
/*
/*	int	pass_trigger(service, buf, len, timeout)
/*	const char *service;
/*	const char *buf;
/*	ssize_t	len;
/*	int	timeout;
/* DESCRIPTION
/*	pass_trigger() connects to the named local server by sending
/*	a file descriptor to it and writing the named buffer.
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
/*	unix_connect(3), local client
/*	stream_connect(3), streams-based client
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

struct pass_trigger {
    int     connect_fd;
    char   *service;
    int     pass_fd[2];
};

/* pass_trigger_event - disconnect from peer */

static void pass_trigger_event(int event, char *context)
{
    struct pass_trigger *pp = (struct pass_trigger *) context;
    static const char *myname = "pass_trigger_event";

    /*
     * Disconnect.
     */
    if (event == EVENT_TIME)
	msg_warn("%s: read timeout for service %s", myname, pp->service);
    event_disable_readwrite(pp->connect_fd);
    event_cancel_timer(pass_trigger_event, context);
    /* Don't combine multiple close() calls into one boolean expression. */
    if (close(pp->connect_fd) < 0)
	msg_warn("%s: close %s: %m", myname, pp->service);
    if (close(pp->pass_fd[0]) < 0)
	msg_warn("%s: close pipe: %m", myname);
    if (close(pp->pass_fd[1]) < 0)
	msg_warn("%s: close pipe: %m", myname);
    myfree(pp->service);
    myfree((char *) pp);
}

/* pass_trigger - wakeup local server */

int     pass_trigger(const char *service, const char *buf, ssize_t len, int timeout)
{
    const char *myname = "pass_trigger";
    int     pass_fd[2];
    struct pass_trigger *pp;
    int     connect_fd;

    if (msg_verbose > 1)
	msg_info("%s: service %s", myname, service);

    /*
     * Connect...
     */
    if ((connect_fd = LOCAL_CONNECT(service, BLOCKING, timeout)) < 0) {
	if (msg_verbose)
	    msg_warn("%s: connect to %s: %m", myname, service);
	return (-1);
    }
    close_on_exec(connect_fd, CLOSE_ON_EXEC);

    /*
     * Create a pipe, and send one pipe end to the server.
     */
    if (pipe(pass_fd) < 0)
	msg_fatal("%s: pipe: %m", myname);
    close_on_exec(pass_fd[0], CLOSE_ON_EXEC);
    close_on_exec(pass_fd[1], CLOSE_ON_EXEC);
    if (LOCAL_SEND_FD(connect_fd, pass_fd[0]) < 0)
	msg_fatal("%s: send file descriptor: %m", myname);

    /*
     * Stash away context.
     */
    pp = (struct pass_trigger *) mymalloc(sizeof(*pp));
    pp->connect_fd = connect_fd;
    pp->service = mystrdup(service);
    pp->pass_fd[0] = pass_fd[0];
    pp->pass_fd[1] = pass_fd[1];

    /*
     * Write the request...
     */
    if (write_buf(pass_fd[1], buf, len, timeout) < 0
	|| write_buf(pass_fd[1], "", 1, timeout) < 0)
	if (msg_verbose)
	    msg_warn("%s: write to %s: %m", myname, service);

    /*
     * Wakeup when the peer disconnects, or when we lose patience.
     */
    if (timeout > 0)
	event_request_timer(pass_trigger_event, (char *) pp, timeout + 100);
    event_enable_read(connect_fd, pass_trigger_event, (char *) pp);
    return (0);
}
