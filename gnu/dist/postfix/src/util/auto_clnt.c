/*++
/* NAME
/*	auto_clnt 3
/* SUMMARY
/*	client endpoint maintenance
/* SYNOPSIS
/*	#include <auto_clnt.h>
/*
/*	AUTO_CLNT *auto_clnt_create(max_idle, max_ttl, open_action, context)
/*	int	max_idle;
/*	int	max_ttl;
/*	VSTREAM *(open_action)(void *context)
/*	void	*context;
/*
/*	VSTREAM	*auto_clnt_access(auto_clnt)
/*	AUTO_CLNT *auto_clnt;
/*
/*	void	auto_clnt_recover(auto_clnt)
/*	AUTO_CLNT *auto_clnt;
/*
/*	void	auto_clnt_free(auto_clnt)
/*	AUTO_CLNT *auto_clnt;
/* DESCRIPTION
/*	This module maintains IPC client endpoints that automatically
/*	disconnect after a being idle for a configurable amount of time,
/*	that disconnect after a configurable time to live,
/*	and that transparently handle most server-initiated disconnects.
/*
/*	auto_clnt_create() instantiates a client endpoint.
/*
/*	auto_clnt_access() returns an open stream to the service specified
/*	to auto_clnt_create(). The stream instance may change between calls.
/*	The result is a null pointer in case of failure.
/*
/*	auto_clnt_recover() recovers from a server-initiated disconnect
/*	that happened in the middle of an I/O operation.
/*
/*	auto_clnt_free() destroys of the specified client endpoint.
/*
/*	Arguments:
/* .IP max_idle
/*	Idle time after which the client disconnects. Specify 0 to disable
/*	the limit.
/* .IP max_ttl
/*	Upper bound on the time that a connection is allowed to persist.
/*	Specify 0 to disable the limit.
/* .IP open_action
/*	Application call-back routine that opens a stream or returns a
/*	null pointer upon failure. In case of success, the call-back routine
/*	is expected to set the stream pathname to the server endpoint name.
/* .IP context
/*	Application context that is passed to the open_action routine.
/* DIAGNOSTICS
/*	Warnings: communication failure. Fatal error: out of memory.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <events.h>
#include <iostuff.h>
#include <auto_clnt.h>

/* Application-specific. */

 /*
  * AUTO_CLNT is an opaque structure. None of the access methods can easily
  * be implemented as a macro, and access is not performance critical anyway.
  */
struct AUTO_CLNT {
    VSTREAM *vstream;			/* buffered I/O */
    int     max_idle;			/* time before client disconnect */
    int     max_ttl;			/* time before client disconnect */
    VSTREAM *(*open_action) (void *);	/* callback */
    void   *context;			/* callback context */
};

static void auto_clnt_close(AUTO_CLNT *);

/* auto_clnt_event - server-initiated disconnect or client-side max_idle */

static void auto_clnt_event(int unused_event, char *context)
{
    AUTO_CLNT *auto_clnt = (AUTO_CLNT *) context;

    /*
     * Sanity check. This routine causes the stream to be closed, so it
     * cannot be called when the stream is already closed.
     */
    if (auto_clnt->vstream == 0)
	msg_panic("auto_clnt_event: stream is closed");

    auto_clnt_close(auto_clnt);
}

/* auto_clnt_ttl_event - client-side expiration */

static void auto_clnt_ttl_event(int event, char *context)
{

    /*
     * XXX This function is needed only because event_request_timer() cannot
     * distinguish between requests that specify the same call-back routine
     * and call-back context. The fix is obvious: specify a request ID along
     * with the call-back routine, but there is too much code that would have
     * to be changed.
     * 
     * XXX Should we be concerned that an overly agressive optimizer will
     * eliminate this function and replace calls to auto_clnt_ttl_event() by
     * direct calls to auto_clnt_event()? It should not, because there exists
     * code that takes the address of both functions.
     */
    auto_clnt_event(event, context);
}

/* auto_clnt_open - connect to service */

static void auto_clnt_open(AUTO_CLNT *auto_clnt)
{

    /*
     * Sanity check.
     */
    if (auto_clnt->vstream)
	msg_panic("auto_clnt_open: stream is open");

    /*
     * Schedule a read event so that we can clean up when the remote side
     * disconnects, and schedule a timer event so that we can cleanup an idle
     * connection. Note that both events are handled by the same routine.
     * 
     * Finally, schedule an event to force disconnection even when the
     * connection is not idle. This is to prevent one client from clinging on
     * to a server forever.
     */
    auto_clnt->vstream =
	auto_clnt->open_action(auto_clnt->context);

    if (auto_clnt->vstream != 0) {
	close_on_exec(vstream_fileno(auto_clnt->vstream), CLOSE_ON_EXEC);
	event_enable_read(vstream_fileno(auto_clnt->vstream), auto_clnt_event,
			  (char *) auto_clnt);
	if (auto_clnt->max_idle > 0)
	    event_request_timer(auto_clnt_event, (char *) auto_clnt,
				auto_clnt->max_idle);
	if (auto_clnt->max_ttl > 0)
	    event_request_timer(auto_clnt_ttl_event, (char *) auto_clnt,
				auto_clnt->max_ttl);
    }
}

/* auto_clnt_close - disconnect from service */

static void auto_clnt_close(AUTO_CLNT *auto_clnt)
{

    /*
     * Sanity check.
     */
    if (auto_clnt->vstream == 0)
	msg_panic("auto_clnt_close: stream is closed");

    /*
     * Be sure to disable read and timer events.
     */
    if (msg_verbose)
	msg_info("%s stream disconnect", VSTREAM_PATH(auto_clnt->vstream));
    event_disable_readwrite(vstream_fileno(auto_clnt->vstream));
    event_cancel_timer(auto_clnt_event, (char *) auto_clnt);
    event_cancel_timer(auto_clnt_ttl_event, (char *) auto_clnt);
    (void) vstream_fclose(auto_clnt->vstream);
    auto_clnt->vstream = 0;
}

/* auto_clnt_recover - recover from server-initiated disconnect */

void    auto_clnt_recover(AUTO_CLNT *auto_clnt)
{

    /*
     * Clean up. Don't re-connect until the caller needs it.
     */
    if (auto_clnt->vstream)
	auto_clnt_close(auto_clnt);
}

/* auto_clnt_access - access a client stream */

VSTREAM *auto_clnt_access(AUTO_CLNT *auto_clnt)
{

    /*
     * Open a stream or restart the idle timer.
     * 
     * Important! Do not restart the TTL timer!
     */
    if (auto_clnt->vstream == 0) {
	auto_clnt_open(auto_clnt);
    } else {
	if (auto_clnt->max_idle > 0)
	    event_request_timer(auto_clnt_event, (char *) auto_clnt,
				auto_clnt->max_idle);
    }
    return (auto_clnt->vstream);
}

/* auto_clnt_create - create client stream connection */

AUTO_CLNT *auto_clnt_create(int max_idle, int max_ttl,
	               VSTREAM *(*open_action) (void *), void *context)
{
    AUTO_CLNT *auto_clnt;

    /*
     * Don't open the stream until the caller needs it.
     */
    auto_clnt = (AUTO_CLNT *) mymalloc(sizeof(*auto_clnt));
    auto_clnt->vstream = 0;
    auto_clnt->max_idle = max_idle;
    auto_clnt->max_ttl = max_ttl;
    auto_clnt->open_action = open_action;
    auto_clnt->context = context;
    return (auto_clnt);
}

/* auto_clnt_free - destroy client stream instance */

void    auto_clnt_free(AUTO_CLNT *auto_clnt)
{
    if (auto_clnt->vstream)
	auto_clnt_close(auto_clnt);
    myfree((char *) auto_clnt);
}
