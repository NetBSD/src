/*	$NetBSD: error.c,v 1.1.1.6.4.1 2007/06/16 16:59:51 snj Exp $	*/

/*++
/* NAME
/*	error 8
/* SUMMARY
/*	Postfix error/retry mail delivery agent
/* SYNOPSIS
/*	\fBerror\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix \fBerror\fR(8) delivery agent processes delivery
/*	requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, the reason for non-delivery (specified as the
/*	next-hop destination), and recipient information.
/*	The reason may be prefixed with an RFC 3463-compatible detail code.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	Depending on the service name in master.cf, \fBerror\fR
/*	or \fBretry\fR, the server bounces or defers all recipients
/*	in the delivery request using the "next-hop" information
/*	as the reason for non-delivery. The \fBretry\fR service name is
/*	supported as of Postfix 2.4.
/*
/*	Delivery status reports are sent to the \fBbounce\fR(8),
/*	\fBdefer\fR(8) or \fBtrace\fR(8) daemon as appropriate.
/* SECURITY
/* .ad
/* .fi
/*	The \fBerror\fR(8) mailer is not security-sensitive. It does not talk
/*	to the network, and can be run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 3463 (Enhanced Status Codes)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces and of other trouble.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBerror\fR(8)
/*      processes run for only a limited amount of time. Use the command
/*      "\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fB2bounce_notice_recipient (postmaster)\fR"
/*	The recipient of undeliverable mail that cannot be returned to
/*	the sender.
/* .IP "\fBbounce_notice_recipient (postmaster)\fR"
/*	The recipient of postmaster notifications with the message headers
/*	of mail that Postfix did not deliver and of SMTP conversation
/*	transcripts of mail that Postfix did not receive.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBdelay_logging_resolution_limit (2)\fR"
/*	The maximal number of digits after the decimal point when logging
/*	sub-second delay values.
/* .IP "\fBdouble_bounce_sender (double-bounce)\fR"
/*	The sender address of postmaster notifications that are generated
/*	by the mail system.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBnotify_classes (resource, software)\fR"
/*	The list of error classes that are reported to the postmaster.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	qmgr(8), queue manager
/*	bounce(8), delivery status reports
/*	discard(8), Postfix discard delivery agent
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	syslogd(8), system logging
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
#include <defer.h>
#include <deliver_completed.h>
#include <flush_clnt.h>
#include <dsn_util.h>
#include <sys_exits.h>
#include <mail_proto.h>
#include <mail_version.h>

/* Single server skeleton. */

#include <mail_server.h>

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(DELIVER_REQUEST *request, const char *def_dsn,
	         int (*append) (int, const char *, MSG_STATS *, RECIPIENT *,
				        const char *, DSN *))
{
    const char *myname = "deliver_message";
    VSTREAM *src;
    int     result = 0;
    int     status;
    RECIPIENT *rcpt;
    int     nrcpt;
    DSN_SPLIT dp;
    DSN     dsn;

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
     * Bounce/defer/whatever all recipients.
     */
#define BOUNCE_FLAGS(request) DEL_REQ_TRACE_FLAGS(request->flags)

    dsn_split(&dp, def_dsn, request->nexthop);
    (void) DSN_SIMPLE(&dsn, DSN_STATUS(dp.dsn), dp.text);
    for (nrcpt = 0; nrcpt < request->rcpt_list.len; nrcpt++) {
	rcpt = request->rcpt_list.info + nrcpt;
	if (rcpt->offset >= 0) {
	    status = append(BOUNCE_FLAGS(request), request->queue_id,
			    &request->msg_stats, rcpt, "none", &dsn);
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

static void error_service(VSTREAM *client_stream, char *service, char **argv)
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
	if (strcmp(service, MAIL_SERVICE_ERROR) == 0)
	    status = deliver_message(request, "5.0.0", bounce_append);
	else if (strcmp(service, MAIL_SERVICE_RETRY) == 0)
	    status = deliver_message(request, "4.0.0", defer_append);
	else
	    msg_fatal("bad error service name: %s", service);
	deliver_request_done(client_stream, request, status);
    }
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{
    flush_init();
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    single_server_main(argc, argv, error_service,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       0);
}
