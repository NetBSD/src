/*++
/* NAME
/*	post_mail 3
/* SUMMARY
/*	convenient mail posting interface
/* SYNOPSIS
/*	#include <post_mail.h>
/*
/*	VSTREAM	*post_mail_fopen(sender, recipient, cleanup_flags, trace_flags)
/*	const char *sender;
/*	const char *recipient;
/*	int	cleanup_flags;
/*	int	trace_flags;
/*
/*	VSTREAM	*post_mail_fopen_nowait(sender, recipient,
/*					cleanup_flags, trace_flags)
/*	const char *sender;
/*	const char *recipient;
/*	int	cleanup_flags;
/*	int	trace_flags;
/*
/*	void	post_mail_fopen_async(sender, recipient,
/*					cleanup_flags, trace_flags,
/*					notify, context)
/*	const char *sender;
/*	const char *recipient;
/*	int	cleanup_flags;
/*	int	trace_flags;
/*	void	(*notify)(VSTREAM *stream, char *context);
/*	char	*context;
/*
/*	int	post_mail_fprintf(stream, format, ...)
/*	VSTREAM	*stream;
/*	const char *format;
/*
/*	int	post_mail_fputs(stream, str)
/*	VSTREAM	*stream;
/*	const char *str;
/*
/*	int	post_mail_buffer(stream, buf, len)
/*	VSTREAM	*stream;
/*	const char *buffer;
/*
/*	int	POST_MAIL_BUFFER(stream, buf)
/*	VSTREAM	*stream;
/*	VSTRING	*buffer;
/*
/*	int	post_mail_fclose(stream)
/*	VSTREAM	*STREAM;
/* DESCRIPTION
/*	This module provides a convenient interface for the most
/*	common case of sending one message to one recipient. It
/*	allows the application to concentrate on message content,
/*	without having to worry about queue file structure details.
/*
/*	post_mail_fopen() opens a connection to the cleanup service
/*	and waits until the service is available, does some option
/*	negotiation, generates message envelope records, and generates
/*	Received: and Date: message headers.  The result is a stream
/*	handle that can be used for sending message records.
/*
/*	post_mail_fopen_nowait() tries to contact the cleanup service
/*	only once, and does not wait until the cleanup service is
/*	available.  Otherwise it is identical to post_mail_fopen().
/*
/*	post_mail_fopen_async() contacts the cleanup service and
/*	invokes the caller-specified notify routine, with the
/*	open stream and the caller-specified context when the
/*	service responds, or with a null stream and the caller-specified
/*	context when the request could not be completed. It is the
/*	responsability of the application to close an open stream.
/*
/*	post_mail_fprintf() formats message content (header or body)
/*	and sends it to the cleanup service.
/*
/*	post_mail_fputs() sends pre-formatted content (header or body)
/*	to the cleanup service.
/*
/*	post_mail_buffer() sends a pre-formatted buffer to the
/*	cleanup service.
/*
/*	POST_MAIL_BUFFER() is a wrapper for post_mail_buffer() that
/*	evaluates its buffer argument more than once.
/*
/*	post_mail_fclose() completes the posting of a message.
/*
/*	Arguments:
/* .IP sender
/*	The sender envelope address. It is up to the application
/*	to produce From: headers.
/* .IP recipient
/*	The recipient envelope address. It is up to the application
/*	to produce To: headers.
/* .IP cleanup_flags
/*	The binary OR of zero or more of the options defined in
/*	\fB<cleanup_user.h>\fR.
/* .IP trace_flags
/*	Message tracing flags as specified in \fB<deliver_request.h>\fR.
/* .IP via
/*	The name of the service responsible for posting this message.
/* .IP stream
/*	A stream opened by mail_post_fopen().
/* .IP notify
/*	Application call-back routine.
/* .IP context
/*	Application call-back context.
/* DIAGNOSTICS
/*	post_mail_fopen_nowait() returns a null pointer when the
/*	cleanup service is not available immediately.
/*
/*	post_mail_fopen_async() returns a null pointer when the
/*	attempt to contact the cleanup service fails immediately.
/*
/*	post_mail_fprintf(), post_mail_fputs() post_mail_fclose(),
/*	and post_mail_buffer() return the binary OR of the error
/*	status codes defined in \fI<cleanup_user.h>\fR.
/*
/*	Fatal errors: cleanup initial handshake errors. This means
/*	the client and server speak incompatible protocols.
/* SEE ALSO
/*	cleanup_user(3h) cleanup options and results
/*	cleanup_strerror(3) translate results to text
/*	cleanup(8) cleanup service
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
#include <time.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <mymalloc.h>
#include <events.h>

/* Global library. */

#include <mail_params.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <post_mail.h>
#include <mail_date.h>

 /*
  * Call-back state for asynchronous connection requests.
  */
typedef struct {
    char   *sender;
    char   *recipient;
    int     cleanup_flags;
    int     trace_flags;
    POST_MAIL_NOTIFY notify;
    void   *context;
    VSTREAM *stream;
} POST_MAIL_STATE;

/* post_mail_init - initial negotiations */

static void post_mail_init(VSTREAM *stream, const char *sender,
			           const char *recipient,
			           int cleanup_flags, int trace_flags)
{
    VSTRING *id = vstring_alloc(100);
    long    now = time((time_t *) 0);
    const char *date = mail_date(now);

    /*
     * Negotiate with the cleanup service. Give up if we can't agree.
     */
    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
		  ATTR_TYPE_END) != 1
	|| attr_print(stream, ATTR_FLAG_NONE,
		      ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, cleanup_flags,
		      ATTR_TYPE_END) != 0)
	msg_fatal("unable to contact the %s service", var_cleanup_service);

    /*
     * Generate a minimal envelope section. The cleanup service will add a
     * size record.
     */
    rec_fprintf(stream, REC_TYPE_TIME, "%ld", (long) now);
    rec_fprintf(stream, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_ORIGIN, MAIL_ATTR_ORG_LOCAL);
    rec_fprintf(stream, REC_TYPE_ATTR, "%s=%d",
		MAIL_ATTR_TRACE_FLAGS, trace_flags);
    rec_fputs(stream, REC_TYPE_FROM, sender);
    rec_fputs(stream, REC_TYPE_RCPT, recipient);
    rec_fputs(stream, REC_TYPE_MESG, "");

    /*
     * Do the Received: and Date: header lines. This allows us to shave a few
     * cycles by using the expensive date conversion result for both.
     */
    post_mail_fprintf(stream, "Received: by %s (%s)",
		      var_myhostname, var_mail_name);
    post_mail_fprintf(stream, "\tid %s; %s", vstring_str(id), date);
    post_mail_fprintf(stream, "Date: %s", date);
    vstring_free(id);
}

/* post_mail_fopen - prepare for posting a message */

VSTREAM *post_mail_fopen(const char *sender, const char *recipient,
			         int cleanup_flags, int trace_flags)
{
    VSTREAM *stream;

    stream = mail_connect_wait(MAIL_CLASS_PUBLIC, var_cleanup_service);
    post_mail_init(stream, sender, recipient, cleanup_flags, trace_flags);
    return (stream);
}

/* post_mail_fopen_nowait - prepare for posting a message */

VSTREAM *post_mail_fopen_nowait(const char *sender, const char *recipient,
				        int cleanup_flags, int trace_flags)
{
    VSTREAM *stream;

    if ((stream = mail_connect(MAIL_CLASS_PUBLIC, var_cleanup_service,
			       BLOCKING)) != 0)
	post_mail_init(stream, sender, recipient, cleanup_flags, trace_flags);
    return (stream);
}

/* post_mail_open_event - handle asynchronous connection events */

static void post_mail_open_event(int event, char *context)
{
    POST_MAIL_STATE *state = (POST_MAIL_STATE *) context;
    char   *myname = "post_mail_open_event";

    switch (event) {

	/*
	 * Connection established. Request notification when the server sends
	 * the initial response. This intermediate case is necessary for some
	 * versions of LINUX and perhaps Solaris, where UNIX-domain
	 * connect(2) blocks until the server performs an accept(2).
	 */
    case EVENT_WRITE:
	if (msg_verbose)
	    msg_info("%s: write event", myname);
	event_disable_readwrite(vstream_fileno(state->stream));
	non_blocking(vstream_fileno(state->stream), BLOCKING);
	event_enable_read(vstream_fileno(state->stream),
			  post_mail_open_event, (char *) state);
	return;

	/*
	 * Initial server reply. Stop the watchdog timer, disable further
	 * read events that end up calling this function, and notify the
	 * requestor.
	 */
    case EVENT_READ:
	if (msg_verbose)
	    msg_info("%s: read event", myname);
	event_cancel_timer(post_mail_open_event, context);
	event_disable_readwrite(vstream_fileno(state->stream));
	post_mail_init(state->stream, state->sender,
		       state->recipient, state->cleanup_flags,
		       state->trace_flags);
	myfree(state->sender);
	myfree(state->recipient);
	state->notify(state->stream, state->context);
	myfree((char *) state);
	return;

	/*
	 * No connection or no initial reply within a conservative time
	 * limit. The system is broken and we give up.
	 */
    case EVENT_TIME:
	if (state->stream) {
	    msg_warn("timeout connecting to service: %s", var_cleanup_service);
	    event_disable_readwrite(vstream_fileno(state->stream));
	    vstream_fclose(state->stream);
	} else {
	    msg_warn("connect to service: %s: %m", var_cleanup_service);
	}
	myfree(state->sender);
	myfree(state->recipient);
	state->notify((VSTREAM *) 0, state->context);
	myfree((char *) state);
	return;

	/*
	 * Some exception.
	 */
    case EVENT_XCPT:
	msg_warn("error connecting to service: %s", var_cleanup_service);
	event_cancel_timer(post_mail_open_event, context);
	event_disable_readwrite(vstream_fileno(state->stream));
	vstream_fclose(state->stream);
	myfree(state->sender);
	myfree(state->recipient);
	state->notify((VSTREAM *) 0, state->context);
	myfree((char *) state);
	return;

	/*
	 * Broken software or hardware.
	 */
    default:
	msg_panic("%s: unknown event type %d", myname, event);
    }
}

/* post_mail_fopen_async - prepare for posting a message */

void    post_mail_fopen_async(const char *sender, const char *recipient,
			              int cleanup_flags, int trace_flags,
			              void (*notify) (VSTREAM *, void *),
			              void *context)
{
    VSTREAM *stream;
    POST_MAIL_STATE *state;

    stream = mail_connect(MAIL_CLASS_PUBLIC, var_cleanup_service, NON_BLOCKING);
    state = (POST_MAIL_STATE *) mymalloc(sizeof(*state));
    state->sender = mystrdup(sender);
    state->recipient = mystrdup(recipient);
    state->cleanup_flags = cleanup_flags;
    state->trace_flags = trace_flags;
    state->notify = notify;
    state->context = context;
    state->stream = stream;

    /*
     * To keep interfaces as simple as possible we report all errors via the
     * same interface as all successes.
     */
    if (stream != 0) {
	event_enable_write(vstream_fileno(stream), post_mail_open_event,
			   (void *) state);
	event_request_timer(post_mail_open_event, (void *) state,
			    var_daemon_timeout);
    } else {
	event_request_timer(post_mail_open_event, (void *) state, 0);
    }
}

/* post_mail_fprintf - format and send message content */

int     post_mail_fprintf(VSTREAM *cleanup, const char *format,...)
{
    int     status;
    va_list ap;

    va_start(ap, format);
    status = rec_vfprintf(cleanup, REC_TYPE_NORM, format, ap);
    va_end(ap);
    return (status != REC_TYPE_NORM ? CLEANUP_STAT_WRITE : 0);
}

/* post_mail_buffer - send pre-formatted buffer */

int     post_mail_buffer(VSTREAM *cleanup, const char *buf, int len)
{
    return (rec_put(cleanup, REC_TYPE_NORM, buf, len) != REC_TYPE_NORM ?
	    CLEANUP_STAT_WRITE : 0);
}

/* post_mail_fputs - send pre-formatted message content */

int     post_mail_fputs(VSTREAM *cleanup, const char *str)
{
    int     len = str ? strlen(str) : 0;

    return (rec_put(cleanup, REC_TYPE_NORM, str, len) != REC_TYPE_NORM ?
	    CLEANUP_STAT_WRITE : 0);
}

/* post_mail_fclose - finish posting of message */

int     post_mail_fclose(VSTREAM *cleanup)
{
    int     status = 0;

    /*
     * Send the message end marker only when there were no errors.
     */
    if (vstream_ferror(cleanup) != 0) {
	status = CLEANUP_STAT_WRITE;
    } else {
	rec_fputs(cleanup, REC_TYPE_XTRA, "");
	rec_fputs(cleanup, REC_TYPE_END, "");
	if (vstream_fflush(cleanup)
	    || attr_scan(cleanup, ATTR_FLAG_MISSING,
			 ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status,
			 ATTR_TYPE_END) != 1)
	    status = CLEANUP_STAT_WRITE;
    }
    (void) vstream_fclose(cleanup);
    return (status);
}
