/*++
/* NAME
/*	master_avail 3
/* SUMMARY
/*	Postfix master - process creation policy
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_avail_listen(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_avail_cleanup(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_avail_more(serv, proc)
/*	MASTER_SERV *serv;
/*	MASTER_PROC *proc;
/*
/*	void	master_avail_less(serv, proc)
/*	MASTER_SERV *serv;
/*	MASTER_PROC *proc;
/* DESCRIPTION
/*	This module implements the process creation policy. As long as
/*	the allowed number of processes for the given service is not
/*	exceeded, a connection request is either handled by an existing
/*	available process, or this module causes a new process to be
/*	created to service the request.
/*
/*	master_avail_listen() ensures that someone monitors the service's
/*	listen socket for connection requests (as long as resources
/*	to handle connection requests are available).  This function may
/*	be called at random. When the maximum number of servers is running,
/*	connection requests are left in the system queue.
/*
/*	master_avail_cleanup() should be called when the named service
/*	is taken out of operation. It terminates child processes by
/*	sending SIGTERM.
/*
/*	master_avail_more() should be called when the named process
/*	has become available for servicing new connection requests.
/*
/*	master_avail_less() should be called when the named process
/*	has become unavailable for servicing new connection requests.
/* DIAGNOSTICS
/*	Panic: internal inconsistencies.
/* BUGS
/* SEE ALSO
/*	master_spawn(3), child process birth and death
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

/* Utility library. */

#include <events.h>
#include <msg.h>

/* Application-specific. */

#include "master_proto.h"
#include "master.h"

/* master_avail_event - create child process to handle connection request */

static void master_avail_event(int event, char *context)
{
    MASTER_SERV *serv = (MASTER_SERV *) context;
    int     n;

    if (event == 0)				/* XXX Can this happen? */
	return;
    if (MASTER_THROTTLED(serv)) {		/* XXX interface botch */
	for (n = 0; n < serv->listen_fd_count; n++)
	    event_disable_readwrite(serv->listen_fd[n]);
    } else {
	master_spawn(serv);
    }
}

/* master_avail_listen - make sure that someone monitors the listen socket */

void    master_avail_listen(MASTER_SERV *serv)
{
    char   *myname = "master_avail_listen";
    int     n;

    /*
     * When no-one else is monitoring the service's listen socket, start
     * monitoring the socket for connection requests. All this under the
     * restriction that we have sufficient resources to service a connection
     * request.
     */
    if (msg_verbose)
	msg_info("%s: avail %d total %d max %d", myname,
		 serv->avail_proc, serv->total_proc, serv->max_proc);
    if (serv->avail_proc < 1
	&& MASTER_LIMIT_OK(serv->max_proc, serv->total_proc)
	&& !MASTER_THROTTLED(serv)) {
	if (msg_verbose)
	    msg_info("%s: enable events %s", myname, serv->name);
	for (n = 0; n < serv->listen_fd_count; n++)
	    event_enable_read(serv->listen_fd[n], master_avail_event,
			      (char *) serv);
    }
}

/* master_avail_cleanup - cleanup */

void    master_avail_cleanup(MASTER_SERV *serv)
{
    int     n;

    master_delete_children(serv);		/* XXX calls
						 * master_avail_listen */
    for (n = 0; n < serv->listen_fd_count; n++)
	event_disable_readwrite(serv->listen_fd[n]);	/* XXX must be last */
}

/* master_avail_more - one more available child process */

void    master_avail_more(MASTER_SERV *serv, MASTER_PROC *proc)
{
    char   *myname = "master_avail_more";
    int     n;

    /*
     * This child process has become available for servicing connection
     * requests, so we can stop monitoring the service's listen socket. The
     * child will do it for us.
     */
    if (msg_verbose)
	msg_info("%s: pid %d (%s)", myname, proc->pid, proc->serv->name);
    if (proc->avail == MASTER_STAT_AVAIL)
	msg_panic("%s: process already available", myname);
    serv->avail_proc++;
    proc->avail = MASTER_STAT_AVAIL;
    if (msg_verbose)
	msg_info("%s: disable events %s", myname, serv->name);
    for (n = 0; n < serv->listen_fd_count; n++)
	event_disable_readwrite(serv->listen_fd[n]);
}

/* master_avail_less - one less available child process */

void    master_avail_less(MASTER_SERV *serv, MASTER_PROC *proc)
{
    char   *myname = "master_avail_less";

    /*
     * This child is no longer available for servicing connection requests.
     * When no child processes are available, start monitoring the service's
     * listen socket for new connection requests.
     */
    if (msg_verbose)
	msg_info("%s: pid %d (%s)", myname, proc->pid, proc->serv->name);
    if (proc->avail != MASTER_STAT_AVAIL)
	msg_panic("%s: process not available", myname);
    serv->avail_proc--;
    proc->avail = MASTER_STAT_TAKEN;
    master_avail_listen(serv);
}
