/*++
/* NAME
/*	error 8
/* SUMMARY
/*	Postfix error mailer
/* SYNOPSIS
/*	\fBerror\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix error mailer processes message delivery requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, a domain or host name that is treated as the reason for
/*	non-delivery, and recipient information.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The error mailer client forces all recipients to bounce, using the
/*	domain or host information as the reason for non-delivery, updates
/*	the queue file and marks recipients as finished, or it informs the
/*	queue manager that delivery should be tried again at a later time.
/*
/*	Delivery problem reports are sent to the \fBbounce\fR(8) or
/*	\fBdefer\fR(8) daemon as appropriate.
/* SECURITY
/* .ad
/* .fi
/*	The error mailer is not security-sensitive. It does not talk
/*	to the network, and can be run chrooted at fixed low privilege.
/* STANDARDS
/*	None.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces and of other trouble.
/* BUGS
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBbounce_notice_recipient\fR
/*	Postmaster for bounce error notices.
/* .IP \fBnotify_classes\fR
/*	When this parameter includes the \fBbounce\fR class, send mail to the
/*	postmaster with with the headers of the bounced mail.
/* SEE ALSO
/*	bounce(8) non-delivery status reports
/*	master(8) process manager
/*	qmgr(8) queue manager
/*	syslogd(8) system logging
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
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <deliver_request.h>
#include <mail_queue.h>
#include <bounce.h>
#include <deliver_completed.h>

/* Single server skeleton. */

#include <mail_server.h>

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(DELIVER_REQUEST *request)
{
    char   *myname = "deliver_message";
    VSTREAM *src;
    int     result = 0;
    int     status;
    RECIPIENT *rcpt;
    int     nrcpt;

    if (msg_verbose)
	msg_info("deliver_message: from %s", request->sender);

    /*
     * Sanity checks.
     */
    if (request->nexthop[0] == 0)
	msg_fatal("empty nexthop hostname");
    if (request->rcpt_list.len <= 0)
	msg_fatal("recipient count: %d", request->rcpt_list.len);

    /*
     * Open the queue file. Opening the file can fail for a variety of
     * reasons, such as the system running out of resources. Instead of
     * throwing away mail, we're raising a fatal error which forces the mail
     * system to back off, and retry later.
     */
    src = mail_queue_open(request->queue_name, request->queue_id,
			  O_RDWR, 0);
    if (src == 0)
	msg_fatal("%s: open %s %s: %m", myname,
		  request->queue_name, request->queue_id);
    if (msg_verbose)
	msg_info("%s: file %s", myname, VSTREAM_PATH(src));

    /*
     * Bounce all recipients.
     */
    for (nrcpt = 0; nrcpt < request->rcpt_list.len; nrcpt++) {
	rcpt = request->rcpt_list.info + nrcpt;
	if (rcpt->offset >= 0) {
	    status = bounce_append(BOUNCE_FLAG_KEEP, request->queue_id,
				   rcpt->address, "error",
				   request->arrival_time,
				   "%s", request->nexthop);
	    if (status == 0)
		deliver_completed(src, rcpt->offset);
	    result |= status;
	}
    }

    /*
     * Clean up.
     */
    if (vstream_fclose(src))
	msg_warn("close %s %s: %m", request->queue_name, request->queue_id);

    return (result);
}

/* error_service - perform service for client */

static void error_service(VSTREAM *client_stream, char *unused_service, char **argv)
{
    DELIVER_REQUEST *request;
    int     status;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to the error mailer. What we see below is a little protocol
     * to (1) tell the queue manager that we are ready, (2) read a request
     * from the queue manager, and (3) report the completion status of that
     * request. All connection-management stuff is handled by the common code
     * in single_server.c.
     */
    if ((request = deliver_request_read(client_stream)) != 0) {
	status = deliver_message(request);
	deliver_request_done(client_stream, request, status);
    }
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    single_server_main(argc, argv, error_service, 0);
}
