/*++
/* NAME
/*	master_proto 3
/* SUMMARY
/*	Postfix master - status notification protocol
/* SYNOPSIS
/*	#include <master_proto.h>
/*
/*	int	master_notify(pid, status)
/*	int	pid;
/*	int	status;
/* DESCRIPTION
/*	The master process provides a standard environment for its
/*	child processes. Part of this environment is a pair of file
/*	descriptors that the master process shares with all child
/*	processes that provide the same service.
/* .IP MASTER_LISTEN_FD
/*	The shared file descriptor for accepting client connection
/*	requests. The master process listens on this socket or FIFO
/*	when all child processes are busy.
/* .IP MASTER_STATUS_FD
/*	The shared file descriptor for sending child status updates to
/*	the master process.
/* .PP
/*	A child process uses master_notify() to send a status notification
/*	message to the master process.
/* .IP MASTER_STAT_AVAIL
/*	The child process is ready to accept client connections.
/* .IP MASTER_STAT_TAKEN
/*	Until further notice, the child process is unavailable for
/*	accepting client connections.
/* .PP
/*	When a child process terminates without sending a status update,
/*	the master process will figure out that the child is no longer
/*	available.
/* DIAGNOSTICS
/*	The result is -1 in case of problems. This usually means that
/*	the parent disconnected after a reload request, in order to
/*	force children to commit suicide.
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

/* Utility library. */

#include <msg.h>

/* Global library. */

#include "master_proto.h"

int     master_notify(int pid, int status)
{
    char   *myname = "master_notify";
    MASTER_STATUS stat;

    /*
     * We use a simple binary protocol to minimize security risks. Since this
     * is local IPC, there are no byte order or word length issues. The
     * server treats this information as gossip, so sending a bad PID or a
     * bad status code will only have amusement value.
     */
    stat.pid = pid;
    stat.avail = status;

    if (write(MASTER_STATUS_FD, (char *) &stat, sizeof(stat)) != sizeof(stat)) {
	if (msg_verbose)
	    msg_info("%s: status %d: %m", myname, status);
	return (-1);
    } else {
	if (msg_verbose)
	    msg_info("%s: status %d", myname, status);
	return (0);
    }
}
