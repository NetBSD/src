/*++
/* NAME
/*	post_mail 3
/* SUMMARY
/*	convenient mail posting interface
/* SYNOPSIS
/*	#include <post_mail.h>
/*
/*	VSTREAM	*post_mail_fopen(sender, recipient, flags)
/*	const char *sender;
/*	const char *recipient;
/*	int	flags;
/*
/*	VSTREAM	*post_mail_fopen_nowait(sender, recipient, flags)
/*	const char *sender;
/*	const char *recipient;
/*	int	flags;
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
/* .IP flags
/*	The binary OR of zero or more of the options defined in
/*	\fI<cleanup_user.h>\fR.
/* .IP via
/*	The name of the service responsible for posting this message.
/* .IP stream
/*	A stream opened by mail_post_fopen().
/* DIAGNOSTICS
/*	post_mail_fopen_nowait() returns a null pointer when the
/*	cleanup service is not available immediately.
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

/* Global library. */

#include <mail_params.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <post_mail.h>
#include <mail_date.h>

/* post_mail_init - initial negotiations */

static void post_mail_init(VSTREAM *stream, const char *sender,
			           const char *recipient, int flags)
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
		      ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
		      ATTR_TYPE_END) != 0)
	msg_fatal("unable to contact the %s service", MAIL_SERVICE_CLEANUP);

    /*
     * Generate a minimal envelope section. The cleanup service will add a
     * size record.
     */
    rec_fprintf(stream, REC_TYPE_TIME, "%ld", (long) now);
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

VSTREAM *post_mail_fopen(const char *sender, const char *recipient, int flags)
{
    VSTREAM *stream;

    stream = mail_connect_wait(MAIL_CLASS_PUBLIC, MAIL_SERVICE_CLEANUP);
    post_mail_init(stream, sender, recipient, flags);
    return (stream);
}

/* post_mail_fopen_nowait - prepare for posting a message */

VSTREAM *post_mail_fopen_nowait(const char *sender, const char *recipient,
				        int flags)
{
    VSTREAM *stream;

    if ((stream = mail_connect(MAIL_CLASS_PUBLIC, MAIL_SERVICE_CLEANUP,
			       BLOCKING)) != 0)
	post_mail_init(stream, sender, recipient, flags);
    return (stream);
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
