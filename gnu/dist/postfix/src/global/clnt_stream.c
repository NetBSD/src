/*++
/* NAME
/*	clnt_stream 3
/* SUMMARY
/*	client endpoint maintenance
/* SYNOPSIS
/*	#include <clnt_stream.h>
/*
/*	CLNT_STREAM *clnt_stream_create(class, service, timeout, ttl)
/*	const char *class;
/*	const char *service;
/*	int	timeout;
/*	int	ttl;
/*
/*	VSTREAM	*clnt_stream_access(clnt_stream)
/*	CLNT_STREAM *clnt_stream;
/*
/*	void	clnt_stream_recover(clnt_stream)
/*	CLNT_STREAM *clnt_stream;
/*
/*	void	clnt_stream_free(clnt_stream)
/*	CLNT_STREAM *clnt_stream;
/* DESCRIPTION
/*	This module maintains local IPC client endpoints that automatically
/*	disconnect after a being idle for a configurable amount of time,
/*	that disconnect after a configurable time to live,
/*	and that transparently handle most server-initiated disconnects.
/*	Server disconnect is detected by read-selecting the client endpoint.
/*	The code assumes that the server has disconnected when the endpoint
/*	becomes readable.
/*
/*	clnt_stream_create() instantiates a client endpoint.
/*
/*	clnt_stream_access() returns an open stream to the service specified
/*	to clnt_stream_create(). The stream instance may change between calls.
/*
/*	clnt_stream_recover() recovers from a server-initiated disconnect
/*	that happened in the middle of an I/O operation.
/*
/*	clnt_stream_free() destroys of the specified client endpoint.
/*
/*	Arguments:
/* .IP class
/*	The service class, private or public.
/* .IP service
/*	The service endpoint name. The name is limited to local IPC
/*	over sockets or equivalent.
/* .IP timeout
/*	Idle time after which the client disconnects.
/* .IP ttl
/*	Upper bound on the time that a connection is allowed to persist.
/* DIAGNOSTICS
/*	Warnings: communication failure. Fatal error: mail system is down,
/*	out of memory.
/* SEE ALSO
/*	mail_proto(3h) low-level mail component glue.
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

/* Global library. */

#include "mail_proto.h"
#include "mail_params.h"
#include "clnt_stream.h"

/* Application-specific. */

 /*
  * CLNT_STREAM is an opaque structure. None of the access methods can easily
  * be implemented as a macro, and access is not performance critical anyway.
  */
struct CLNT_STREAM {
    VSTREAM *vstream;			/* buffered I/O */
    int     timeout;			/* time before client disconnect */
    int     ttl;			/* time before client disconnect */
    char   *class;			/* server class */
    char   *service;			/* server name */
};

static void clnt_stream_close(CLNT_STREAM *);

/* clnt_stream_event - server-initiated disconnect or client-side timeout */

static void clnt_stream_event(int unused_event, char *context)
{
    CLNT_STREAM *clnt_stream = (CLNT_STREAM *) context;

    /*
     * Sanity check. This routine causes the stream to be closed, so it
     * cannot be called when the stream is already closed.
     */
    if (clnt_stream->vstream == 0)
	msg_panic("clnt_stream_event: stream is closed");

    clnt_stream_close(clnt_stream);
}

/* clnt_stream_ttl_event - client-side expiration */

static void clnt_stream_ttl_event(int event, char *context)
{

    /*
     * XXX This function is needed only because event_request_timer() cannot
     * distinguish between requests that specify the same call-back routine
     * and call-back context. The fix is obvious: specify a request ID along
     * with the call-back routine, but there is too much code that would have
     * to be changed.
     * 
     * XXX Should we be concerned that an overly agressive optimizer will
     * eliminate this function and replace calls to clnt_stream_ttl_event()
     * by direct calls to clnt_stream_event()? It should not, because there
     * exists code that takes the address of both functions.
     */
    clnt_stream_event(event, context);
}

/* clnt_stream_open - connect to service */

static void clnt_stream_open(CLNT_STREAM *clnt_stream)
{

    /*
     * Sanity check.
     */
    if (clnt_stream->vstream)
	msg_panic("clnt_stream_open: stream is open");

    /*
     * Schedule a read event so that we can clean up when the remote side
     * disconnects, and schedule a timer event so that we can cleanup an idle
     * connection. Note that both events are handled by the same routine.
     * 
     * Finally, schedule an event to force disconnection even when the
     * connection is not idle. This is to prevent one client from clinging on
     * to a server forever.
     */
    clnt_stream->vstream = mail_connect_wait(clnt_stream->class,
					     clnt_stream->service);
    close_on_exec(vstream_fileno(clnt_stream->vstream), CLOSE_ON_EXEC);
    event_enable_read(vstream_fileno(clnt_stream->vstream), clnt_stream_event,
		      (char *) clnt_stream);
    event_request_timer(clnt_stream_event, (char *) clnt_stream,
			clnt_stream->timeout);
    event_request_timer(clnt_stream_ttl_event, (char *) clnt_stream,
			clnt_stream->ttl);
}

/* clnt_stream_close - disconnect from service */

static void clnt_stream_close(CLNT_STREAM *clnt_stream)
{

    /*
     * Sanity check.
     */
    if (clnt_stream->vstream == 0)
	msg_panic("clnt_stream_close: stream is closed");

    /*
     * Be sure to disable read and timer events.
     */
    if (msg_verbose)
	msg_info("%s stream disconnect", clnt_stream->service);
    event_disable_readwrite(vstream_fileno(clnt_stream->vstream));
    event_cancel_timer(clnt_stream_event, (char *) clnt_stream);
    event_cancel_timer(clnt_stream_ttl_event, (char *) clnt_stream);
    (void) vstream_fclose(clnt_stream->vstream);
    clnt_stream->vstream = 0;
}

/* clnt_stream_recover - recover from server-initiated disconnect */

void    clnt_stream_recover(CLNT_STREAM *clnt_stream)
{

    /*
     * Clean up. Don't re-connect until the caller needs it.
     */
    if (clnt_stream->vstream)
	clnt_stream_close(clnt_stream);
}

/* clnt_stream_access - access a client stream */

VSTREAM *clnt_stream_access(CLNT_STREAM *clnt_stream)
{

    /*
     * Open a stream or restart the idle timer.
     * 
     * Important! Do not restart the TTL timer!
     */
    if (clnt_stream->vstream == 0) {
	clnt_stream_open(clnt_stream);
    } else if (readable(vstream_fileno(clnt_stream->vstream))) {
	clnt_stream_close(clnt_stream);
	clnt_stream_open(clnt_stream);
    } else {
	event_request_timer(clnt_stream_event, (char *) clnt_stream,
			    clnt_stream->timeout);
    }
    return (clnt_stream->vstream);
}

/* clnt_stream_create - create client stream connection */

CLNT_STREAM *clnt_stream_create(const char *class, const char *service,
				        int timeout, int ttl)
{
    CLNT_STREAM *clnt_stream;

    /*
     * Don't open the stream until the caller needs it.
     */
    clnt_stream = (CLNT_STREAM *) mymalloc(sizeof(*clnt_stream));
    clnt_stream->vstream = 0;
    clnt_stream->timeout = timeout;
    clnt_stream->ttl = ttl;
    clnt_stream->class = mystrdup(class);
    clnt_stream->service = mystrdup(service);
    return (clnt_stream);
}

/* clnt_stream_free - destroy client stream instance */

void    clnt_stream_free(CLNT_STREAM *clnt_stream)
{
    if (clnt_stream->vstream)
	clnt_stream_close(clnt_stream);
    myfree(clnt_stream->class);
    myfree(clnt_stream->service);
    myfree((char *) clnt_stream);
}
