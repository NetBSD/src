/*	$NetBSD: master_avail.c,v 1.1.1.1 2009/06/23 10:08:49 tron Exp $	*/

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
/*	When the service runs out of process slots, a warning is logged.
/*	When the service is eligible for stress-mode operation, servers
/*	are restarted and new servers are created with stress mode enabled.
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
    time_t  now;
    int     n;

    if (event == 0)				/* XXX Can this happen? */
	return;
    if (MASTER_THROTTLED(serv)) {		/* XXX interface botch */
	for (n = 0; n < serv->listen_fd_count; n++)
	    event_disable_readwrite(serv->listen_fd[n]);
    } else {

	/*
	 * When all servers for a public internet service are busy, we start
	 * creating server processes with "-o stress=yes" on the command
	 * line, and keep creating such processes until the process count is
	 * below the limit for at least 1000 seconds. This provides a minimal
	 * solution that can be adopted into legacy and stable Postfix
	 * releases.
	 * 
	 * This is not the right place to update serv->stress_param_val in
	 * response to stress level changes. Doing so would would contaminate
	 * the "postfix reload" code with stress management implementation
	 * details, creating a source of future bugs. Instead, we update
	 * simple counters or flags here, and use their values to determine
	 * the proper serv->stress_param_val value when exec-ing a server
	 * process.
	 */
	if (serv->stress_param_val != 0
	    && !MASTER_LIMIT_OK(serv->max_proc, serv->total_proc + 1)) {
	    now = event_time();
	    if (serv->stress_expire_time < now)
		master_restart_service(serv);
	    serv->stress_expire_time = now + 1000;
	}
	master_spawn(serv);
    }
}

/* master_avail_listen - make sure that someone monitors the listen socket */

void    master_avail_listen(MASTER_SERV *serv)
{
    const char *myname = "master_avail_listen";
    time_t  now;
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
    if (serv->avail_proc < 1 && !MASTER_THROTTLED(serv)) {
	if (MASTER_LIMIT_OK(serv->max_proc, serv->total_proc)) {
	    if (msg_verbose)
		msg_info("%s: enable events %s", myname, serv->name);
	    for (n = 0; n < serv->listen_fd_count; n++)
		event_enable_read(serv->listen_fd[n], master_avail_event,
				  (char *) serv);
	} else if ((serv->flags & MASTER_FLAG_LOCAL_ONLY) == 0
		   && (now = event_time()) - serv->busy_warn_time > 1000) {
	    serv->busy_warn_time = now;
	    msg_warn("service \"%s\" (%s) has reached its process limit \"%d\": "
		     "new clients may experience noticeable delays",
		     serv->ext_name, serv->name, serv->max_proc);
	    msg_warn("to avoid this condition, increase the process count "
		     "in master.cf or reduce the service time per client");
	    if (serv->stress_param_val)
		msg_warn("see http://www.postfix.org/STRESS_README.html for "
		      "examples of stress-adapting configuration settings");
	}
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
    const char *myname = "master_avail_more";
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
    const char *myname = "master_avail_less";

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
