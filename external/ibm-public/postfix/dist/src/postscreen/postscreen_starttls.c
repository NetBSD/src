/*	$NetBSD: postscreen_starttls.c,v 1.4 2022/10/08 16:12:48 christos Exp $	*/

/*++
/* NAME
/*	postscreen_starttls 3
/* SUMMARY
/*	postscreen TLS proxy support
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	int	psc_starttls_open(state, resume_event)
/*	PSC_STATE *state;
/*	void	(*resume_event)(int unused_event, char *context);
/* DESCRIPTION
/*	This module inserts the tlsproxy(8) proxy between the
/*	postscreen(8) server and the remote SMTP client. The entire
/*	process happens in the background, including notification
/*	of completion to the remote SMTP client and to the calling
/*	application.
/*
/*	Before calling psc_starttls_open() the caller must turn off
/*	all pending timer and I/O event requests on the SMTP client
/*	stream.
/*
/*	psc_starttls_open() starts the first transaction in the
/*	tlsproxy(8) hand-off protocol, and sets up event handlers
/*	for the successive protocol stages.
/*
/*	Upon completion, the event handlers call resume_event()
/*	which must reset the SMTP helo/sender/etc. state when the
/*	PSC_STATE_FLAG_USING_TLS is set, and set up timer and read
/*	event requests to receive the next SMTP command.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <connect.h>
#include <stringops.h>			/* concatenate() */
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>

/* TLS library. */

#include <tls_proxy.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * For now, this code is built into the postscreen(8) daemon. In the future
  * it may be abstracted into a reusable library module for use by other
  * event-driven programs (perhaps smtp-source and smtp-sink).
  */

 /*
  * Transient state for the postscreen(8)-to-tlsproxy(8) hand-off protocol.
  */
typedef struct {
    VSTREAM *tlsproxy_stream;		/* hand-off negotiation */
    EVENT_NOTIFY_FN resume_event;	/* call-back handler */
    PSC_STATE *smtp_state;		/* SMTP session state */
} PSC_STARTTLS;

#define TLSPROXY_INIT_TIMEOUT		10

static char *psc_tlsp_service = 0;

/* Resume the dummy SMTP engine after an event handling error */

#define PSC_STARTTLS_EVENT_ERR_RESUME_RETURN() do { \
	event_disable_readwrite(vstream_fileno(tlsproxy_stream)); \
	PSC_STARTTLS_EVENT_RESUME_RETURN(starttls_state); \
    } while (0);

/* Resume the dummy SMTP engine, possibly after swapping streams */

#define PSC_STARTTLS_EVENT_RESUME_RETURN(starttls_state) do { \
	vstream_fclose(tlsproxy_stream); \
	starttls_state->resume_event(event, (void *) smtp_state); \
	myfree((void *) starttls_state); \
	return; \
    } while (0)

/* psc_starttls_finish - complete negotiation with TLS proxy */

static void psc_starttls_finish(int event, void *context)
{
    const char *myname = "psc_starttls_finish";
    PSC_STARTTLS *starttls_state = (PSC_STARTTLS *) context;
    PSC_STATE *smtp_state = starttls_state->smtp_state;
    VSTREAM *tlsproxy_stream = starttls_state->tlsproxy_stream;
    int     status;

    if (msg_verbose)
	msg_info("%s: send client handle on proxy socket %d"
		 " for smtp socket %d from [%s]:%s flags=%s",
		 myname, vstream_fileno(tlsproxy_stream),
		 vstream_fileno(smtp_state->smtp_client_stream),
		 smtp_state->smtp_client_addr, smtp_state->smtp_client_port,
		 psc_print_state_flags(smtp_state->flags, myname));

    /*
     * We leave read-event notification enabled on the postscreen to TLS
     * proxy stream, to avoid two kqueue/epoll/etc. system calls: one here,
     * and one when resuming the dummy SMTP engine.
     */
    if (event != EVENT_TIME)
	event_cancel_timer(psc_starttls_finish, (void *) starttls_state);

    /*
     * Receive the "TLS is available" indication.
     * 
     * This may seem out of order, but we must have a read transaction between
     * sending the request attributes and sending the SMTP client file
     * descriptor. We can't assume UNIX-domain socket semantics here.
     */
    if (event != EVENT_READ
	|| attr_scan(tlsproxy_stream, ATTR_FLAG_STRICT,
		     RECV_ATTR_INT(MAIL_ATTR_STATUS, &status),
		     ATTR_TYPE_END) != 1 || status == 0) {

	/*
	 * The TLS proxy reports that the TLS engine is not available (due to
	 * configuration error, or other causes).
	 */
	msg_warn("%s receiving status from %s service",
	     event == EVENT_TIME ? "timeout" : "problem", psc_tlsp_service);
	PSC_SEND_REPLY(smtp_state,
		    "454 4.7.0 TLS not available due to local problem\r\n");
	PSC_STARTTLS_EVENT_ERR_RESUME_RETURN();
    }

    /*
     * Send the remote SMTP client file descriptor.
     */
    else if (LOCAL_SEND_FD(vstream_fileno(tlsproxy_stream),
		      vstream_fileno(smtp_state->smtp_client_stream)) < 0) {

	/*
	 * Some error: drop the TLS proxy stream.
	 */
	msg_warn("problem sending file handle to %s service", psc_tlsp_service);
	PSC_SEND_REPLY(smtp_state,
		    "454 4.7.0 TLS not available due to local problem\r\n");
	PSC_STARTTLS_EVENT_ERR_RESUME_RETURN();
    }

    /*
     * After we send the plaintext 220 greeting, the client-side TLS engine
     * is supposed to talk first, then the server-side TLS engine. However,
     * postscreen(8) will not participate in that conversation.
     */
    else {
	PSC_SEND_REPLY(smtp_state, "220 2.0.0 Ready to start TLS\r\n");

	/*
	 * Swap the SMTP client stream and the TLS proxy stream, and close
	 * the direct connection to the SMTP client. The TLS proxy will talk
	 * directly to the SMTP client, and once the TLS handshake is
	 * completed, the TLS proxy will talk plaintext to postscreen(8).
	 * 
	 * Swap the file descriptors from under the VSTREAM so that we don't
	 * have to worry about loss of user-configurable VSTREAM attributes.
	 */
	vstream_fpurge(smtp_state->smtp_client_stream, VSTREAM_PURGE_BOTH);
	vstream_control(smtp_state->smtp_client_stream,
			CA_VSTREAM_CTL_SWAP_FD(tlsproxy_stream),
			CA_VSTREAM_CTL_END);
	smtp_state->flags |= PSC_STATE_FLAG_USING_TLS;
	PSC_STARTTLS_EVENT_RESUME_RETURN(starttls_state);
    }
}

/* psc_starttls_first - start negotiation with TLS proxy */

static void psc_starttls_first(int event, void *context)
{
    const char *myname = "psc_starttls_first";
    PSC_STARTTLS *starttls_state = (PSC_STARTTLS *) context;
    PSC_STATE *smtp_state = starttls_state->smtp_state;
    VSTREAM *tlsproxy_stream = starttls_state->tlsproxy_stream;
    static VSTRING *remote_endpt = 0;

    if (msg_verbose)
	msg_info("%s: receive server protocol on proxy socket %d"
		 " for smtp socket %d from [%s]:%s flags=%s",
		 myname, vstream_fileno(tlsproxy_stream),
		 vstream_fileno(smtp_state->smtp_client_stream),
		 smtp_state->smtp_client_addr, smtp_state->smtp_client_port,
		 psc_print_state_flags(smtp_state->flags, myname));

    /*
     * We leave read-event notification enabled on the postscreen to TLS
     * proxy stream, to avoid two kqueue/epoll/etc. system calls: one here,
     * and one when resuming the dummy SMTP engine.
     */
    if (event != EVENT_TIME)
	event_cancel_timer(psc_starttls_first, (void *) starttls_state);

    /*
     * Receive and verify the server protocol.
     */
    if (event != EVENT_READ
	|| attr_scan(tlsproxy_stream, ATTR_FLAG_STRICT,
		 RECV_ATTR_STREQ(MAIL_ATTR_PROTO, MAIL_ATTR_PROTO_TLSPROXY),
		     ATTR_TYPE_END) != 0) {
	msg_warn("%s receiving %s attribute from %s service: %m",
		 event == EVENT_TIME ? "timeout" : "problem",
		 MAIL_ATTR_PROTO, psc_tlsp_service);
	PSC_SEND_REPLY(smtp_state,
		    "454 4.7.0 TLS not available due to local problem\r\n");
	PSC_STARTTLS_EVENT_ERR_RESUME_RETURN();
    }

    /*
     * Send the data attributes now, and send the client file descriptor in a
     * later transaction. We report all errors asynchronously, to avoid
     * having to maintain multiple error delivery paths.
     * 
     * XXX The formatted endpoint should be a state member. Then, we can
     * simplify all the format strings throughout the program.
     */
    if (remote_endpt == 0)
	remote_endpt = vstring_alloc(20);
    vstring_sprintf(remote_endpt, "[%s]:%s", smtp_state->smtp_client_addr,
		    smtp_state->smtp_client_port);
    attr_print(tlsproxy_stream, ATTR_FLAG_NONE,
	       SEND_ATTR_STR(TLS_ATTR_REMOTE_ENDPT, STR(remote_endpt)),
	       SEND_ATTR_INT(TLS_ATTR_FLAGS, TLS_PROXY_FLAG_ROLE_SERVER),
	       SEND_ATTR_INT(TLS_ATTR_TIMEOUT, psc_normal_cmd_time_limit),
	       SEND_ATTR_INT(TLS_ATTR_TIMEOUT, psc_normal_cmd_time_limit),
	       SEND_ATTR_STR(TLS_ATTR_SERVERID, MAIL_SERVICE_SMTPD),	/* XXX */
	       ATTR_TYPE_END);
    if (vstream_fflush(tlsproxy_stream) != 0) {
	msg_warn("error sending request to %s service: %m", psc_tlsp_service);
	PSC_SEND_REPLY(smtp_state,
		    "454 4.7.0 TLS not available due to local problem\r\n");
	PSC_STARTTLS_EVENT_ERR_RESUME_RETURN();
    }

    /*
     * Set up a read event for the next phase of the TLS proxy handshake.
     */
    PSC_READ_EVENT_REQUEST(vstream_fileno(tlsproxy_stream), psc_starttls_finish,
			   (void *) starttls_state, TLSPROXY_INIT_TIMEOUT);
}

/* psc_starttls_open - open negotiations with TLS proxy */

void    psc_starttls_open(PSC_STATE *smtp_state, EVENT_NOTIFY_FN resume_event)
{
    const char *myname = "psc_starttls_open";
    PSC_STARTTLS *starttls_state;
    VSTREAM *tlsproxy_stream;
    int     fd;

    if (psc_tlsp_service == 0) {
	psc_tlsp_service = concatenate(MAIL_CLASS_PRIVATE "/",
				       var_tlsproxy_service, (char *) 0);
    }

    /*
     * Connect to the tlsproxy(8) daemon. We report all errors
     * asynchronously, to avoid having to maintain multiple delivery paths.
     */
    if ((fd = LOCAL_CONNECT(psc_tlsp_service, NON_BLOCKING, 1)) < 0) {
	msg_warn("connect to %s service: %m", psc_tlsp_service);
	PSC_SEND_REPLY(smtp_state,
		    "454 4.7.0 TLS not available due to local problem\r\n");
	event_request_timer(resume_event, (void *) smtp_state, 0);
	return;
    }
    if (msg_verbose)
	msg_info("%s: connecting to proxy socket %d"
		 " for smtp socket %d from [%s]:%s flags=%s",
		 myname, fd, vstream_fileno(smtp_state->smtp_client_stream),
		 smtp_state->smtp_client_addr, smtp_state->smtp_client_port,
		 psc_print_state_flags(smtp_state->flags, myname));

    tlsproxy_stream = vstream_fdopen(fd, O_RDWR);
    vstream_control(tlsproxy_stream,
		    VSTREAM_CTL_PATH, psc_tlsp_service,
		    VSTREAM_CTL_END);

    /*
     * Set up a read event for the next phase of the TLS proxy handshake.
     */
    starttls_state = (PSC_STARTTLS *) mymalloc(sizeof(*starttls_state));
    starttls_state->tlsproxy_stream = tlsproxy_stream;
    starttls_state->resume_event = resume_event;
    starttls_state->smtp_state = smtp_state;
    PSC_READ_EVENT_REQUEST(vstream_fileno(tlsproxy_stream), psc_starttls_first,
			   (void *) starttls_state, TLSPROXY_INIT_TIMEOUT);
}
