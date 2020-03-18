/*	$NetBSD: postscreen_haproxy.c,v 1.3 2020/03/18 19:05:19 christos Exp $	*/

/*++
/* NAME
/*	postscreen_haproxy 3
/* SUMMARY
/*	haproxy protocol adapter
/* SYNOPSIS
/*	#include <postscreen_haproxy.h>
/*
/*	void	psc_endpt_haproxy_lookup(smtp_client_stream, lookup_done)
/*	VSTRING	*smtp_client_stream;
/*	void	(*lookup_done)(status, smtp_client_stream,
/*				smtp_client_addr, smtp_client_port,
/*				smtp_server_addr, smtp_server_port)
/*	int	status;
/*	MAI_HOSTADDR_STR *smtp_client_addr;
/*	MAI_SERVPORT_STR *smtp_client_port;
/*	MAI_HOSTADDR_STR *smtp_server_addr;
/*	MAI_SERVPORT_STR *smtp_server_port;
/* DESCRIPTION
/*	psc_endpt_haproxy_lookup() looks up connection endpoint
/*	information via the haproxy protocol, or looks up local
/*	information if the haproxy handshake indicates that a
/*	connection is not proxied. Arguments and results conform
/*	to the postscreen_endpt(3) API.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <myaddrinfo.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>

/* Global library. */

#include <haproxy_srvr.h>
#include <mail_params.h>

/* Application-specific. */

#include <postscreen.h>
#include <postscreen_haproxy.h>

 /*
  * Per-session state.
  */
typedef struct {
    VSTREAM *stream;
    PSC_ENDPT_LOOKUP_FN notify;
} PSC_HAPROXY_STATE;

/* psc_endpt_haproxy_event - read or time event */

static void psc_endpt_haproxy_event(int event, void *context)
{
    const char *myname = "psc_endpt_haproxy_event";
    PSC_HAPROXY_STATE *state = (PSC_HAPROXY_STATE *) context;
    int     status = 0;
    MAI_HOSTADDR_STR smtp_client_addr;
    MAI_SERVPORT_STR smtp_client_port;
    MAI_HOSTADDR_STR smtp_server_addr;
    MAI_SERVPORT_STR smtp_server_port;
    int     non_proxy = 0;

    switch (event) {
    case EVENT_TIME:
	msg_warn("haproxy read: time limit exceeded");
	status = -1;
	break;
    case EVENT_READ:
	status = haproxy_srvr_receive(vstream_fileno(state->stream), &non_proxy,
				      &smtp_client_addr, &smtp_client_port,
				      &smtp_server_addr, &smtp_server_port);
    }

    /*
     * Terminate this pseudo thread, and notify the caller.
     */
    PSC_CLEAR_EVENT_REQUEST(vstream_fileno(state->stream),
			    psc_endpt_haproxy_event, context);
    if (status == 0 && non_proxy)
	psc_endpt_local_lookup(state->stream, state->notify);
    else
	state->notify(status, state->stream,
		      &smtp_client_addr, &smtp_client_port,
		      &smtp_server_addr, &smtp_server_port);
    /* Note: the stream may be closed at this point. */
    myfree((void *) state);
}

/* psc_endpt_haproxy_lookup - event-driven haproxy client */

void    psc_endpt_haproxy_lookup(VSTREAM *stream,
				         PSC_ENDPT_LOOKUP_FN notify)
{
    const char *myname = "psc_endpt_haproxy_lookup";
    PSC_HAPROXY_STATE *state;

    /*
     * Prepare the per-session state. XXX To improve overload behavior,
     * maintain a pool of these so that we can reduce memory allocator
     * activity.
     */
    state = (PSC_HAPROXY_STATE *) mymalloc(sizeof(*state));
    state->stream = stream;
    state->notify = notify;

    /*
     * Read the haproxy line.
     */
    PSC_READ_EVENT_REQUEST(vstream_fileno(stream), psc_endpt_haproxy_event,
			   (void *) state, var_psc_uproxy_tmout);
}
