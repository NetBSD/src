/*++
/* NAME
/*	cleanup 8
/* SUMMARY
/*	canonicalize and enqueue Postfix message
/* SYNOPSIS
/*	\fBcleanup\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBcleanup\fR daemon processes inbound mail, inserts it
/*	into the \fBincoming\fR mail queue, and informs the queue
/*	manager of its arrival.
/*
/*	The \fBcleanup\fR daemon always performs the following transformations:
/* .IP \(bu
/*	Insert missing message headers: (\fBResent-\fR) \fBFrom:\fR,
/*	\fBTo:\fR, \fBMessage-Id:\fR, and \fBDate:\fR.
/* .IP \(bu
/*	Extract envelope recipient addresses from (\fBResent-\fR) \fBTo:\fR,
/*	\fBCc:\fR and \fBBcc:\fR message headers when no recipients are
/*	specified in the message envelope.
/* .IP \(bu
/*	Transform envelope and header addresses to the standard
/*	\fIuser@fully-qualified-domain\fR form that is expected by other
/*	Postfix programs.
/*	This task is delegated to the \fBtrivial-rewrite\fR(8) daemon.
/* .IP \(bu
/*	Eliminate duplicate envelope recipient addresses.
/* .PP
/*	The following address transformations are optional:
/* .IP \(bu
/*	Optionally, rewrite all envelope and header addresses according
/*	to the mappings specified in the \fBcanonical\fR(5) lookup tables.
/* .IP \(bu
/*	Optionally, masquerade envelope sender addresses and message
/*	header addresses (i.e. strip host or domain information below
/*	all domains listed in the \fBmasquerade_domains\fR parameter,
/*	except for user names listed in \fBmasquerade_exceptions\fR).
/*	By default, address masquerading does not affect envelope recipients.
/* .IP \(bu
/*	Optionally, expand envelope recipients according to information
/*	found in the \fBvirtual\fR(5) lookup tables.
/* .PP
/*	The \fBcleanup\fR daemon performs sanity checks on the content of
/*	each message. When it finds a problem, by default it returns a
/*	diagnostic status to the client, and leaves it up to the client
/*	to deal with the problem. Alternatively, the client can request
/*	the \fBcleanup\fR daemon to bounce the message back to the sender
/*	in case of trouble.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 2045 (MIME: Format of Internet Message Bodies)
/*	RFC 2046 (MIME: Media Types)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	Table-driven rewriting rules make it hard to express \fBif then
/*	else\fR and other logical relationships.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Content filtering
/* .IP \fBbody_checks\fR
/*	Lookup tables with content filters for message body lines.
/*	These filters see physical lines one at a time, in chunks of
/*	at most line_length_limit bytes.
/* .IP \fBbody_checks_size_limit\fP
/*	The amount of content per message body segment that is 
/*	subjected to \fB$body_checks\fR filtering.
/* .IP \fBheader_checks\fR
/* .IP "\fBmime_header_checks\fR (default: \fB$header_checks\fR)"
/* .IP "\fBnested_header_checks\fR (default: \fB$header_checks\fR)"
/*	Lookup tables with content filters for message header lines:
/*	respectively, these are applied to the primary message headers 
/*	(not including MIME headers), to the MIME headers anywhere in 
/*	the message, and to the initial headers of attached messages.
/*	These filters see logical headers one at a time, including headers
/*	that span multiple lines.
/* .SH MIME Processing
/* .ad
/* .fi
/* .IP \fBdisable_mime_input_processing\fR
/*	While receiving, give no special treatment to \fBContent-Type:\fR 
/*	message headers; all text after the initial message headers is 
/*	considered to be part of the message body.
/* .IP \fBmime_boundary_length_limit\fR
/*	The amount of space that will be allocated for MIME multipart
/*	boundary strings. The MIME processor is unable to distinguish
/*	between boundary strings that do not differ in the first 
/*	\fB$mime_boundary_length_limit\fR characters.
/* .IP \fBmime_nesting_limit\fR
/*	The maximal nesting level of multipart mail that the MIME
/*	processor can handle. Refuse mail that is nested deeper.
/* .IP \fBstrict_8bitmime\fR
/*	Turn on both \fBstrict_7bit_headers\fR and \fBstrict_8bitmime_body\fR.
/* .IP \fBstrict_7bit_headers\fR
/*	Reject mail with 8-bit text in message headers. This blocks
/*	mail from poorly written applications.
/* .IP \fBstrict_8bitmime_body\fR
/*	Reject mail with 8-bit text in content that claims to be 7-bit, 
/*	or in content that has no explicit content encoding information. 
/*	This blocks mail from poorly written mail software. Unfortunately, 
/*	this also breaks majordomo approval requests when the included 
/*	request contains valid 8-bit MIME mail, and it breaks bounces from
/*	mailers that do not properly encapsulate 8-bit content (for example,
/*	bounces from qmail or from old versions of Postfix).
/* .IP \fBstrict_mime_encoding_domain\fR
/*	Reject mail with invalid \fBContent-Transfer-Encoding:\fR
/*	information for message/* or multipart/*. This blocks mail
/*	from poorly written software.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalways_bcc\fR
/*	Address to send a copy of each message that enters the system.
/* .IP \fBhopcount_limit\fR
/*	Limit the number of \fBReceived:\fR message headers.
/* .IP \fBundisclosed_recipients_header\fR
/*	The header line that is inserted when no recipients were
/*	specified in (Resent-)To: or (Resent-)Cc: message headers.
/* .SH "Address transformations"
/* .ad
/* .fi
/* .IP \fBempty_address_recipient\fR
/*	The destination for undeliverable mail from <>. This
/*	substitution is done before all other address rewriting.
/* .IP \fBcanonical_maps\fR
/*	Address mapping lookup table for sender and recipient addresses
/*	in envelopes and headers.
/* .IP \fBrecipient_canonical_maps\fR
/*	Address mapping lookup table for envelope and header recipient
/*	addresses.
/* .IP \fBsender_canonical_maps\fR
/*	Address mapping lookup table for envelope and header sender
/*	addresses.
/* .IP \fBcanonicalize_envelope_recipient\fR
/*	By default (recipient address) canonicalization is applied even
/*	to the envelope recipient. To prevent delivery loops when using
/*	external canonical addresses, while still having recipient headers
/*	rewritten to the canonical addresses, set this to 'no'.
/* .IP \fBmasquerade_classes\fR
/*      List of address classes subject to masquerading: zero or
/*      more of \fBenvelope_sender\fR, \fBenvelope_recipient\fR,
/*	\fBheader_sender\fR, \fBheader_recipient\fR.
/* .IP \fBmasquerade_domains\fR
/*	List of domains that hide their subdomain structure.
/* .IP \fBmasquerade_exceptions\fR
/*	List of user names that are not subject to address masquerading.
/* .IP \fBvirtual_alias_maps\fR
/*	Address mapping lookup table for envelope recipient addresses.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBduplicate_filter_limit\fR
/*	Limits the number of envelope recipients that are remembered.
/* .IP \fBheader_address_token_limit\fR
/*	Limits the number of address tokens used to process a message header.
/* .IP \fBheader_size_limit\fR
/*	Limits the amount of memory in bytes used to store a message header.
/* .IP \fBin_flow_delay\fR
/*	Amount of time to pause before accepting a message, when the
/*	message arrival rate exceeds the message delivery rate.
/* .IP \fBextract_recipient_limit\fR
/*	Limit the amount of recipients extracted from message headers.
/* SEE ALSO
/*	canonical(5) canonical address lookup table format
/*	qmgr(8) queue manager daemon
/*	syslogd(8) system logging
/*	trivial-rewrite(8) address rewriting
/*	virtual(5) virtual alias lookup table format
/* FILES
/*	/etc/postfix/canonical*, canonical mapping table
/*	/etc/postfix/virtual*, virtual mapping table
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
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <dict.h>

/* Global library. */

#include <mail_conf.h>
#include <cleanup_user.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <record.h>
#include <rec_type.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_service - process one request to inject a message into the queue */

static void cleanup_service(VSTREAM *src, char *unused_service, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    CLEANUP_STATE *state;
    int     flags;
    int     type = 0;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Open a queue file and initialize state.
     */
    state = cleanup_open();

    /*
     * Send the queue id to the client. Read client processing options. If we
     * can't read the client processing options we can pretty much forget
     * about the whole operation.
     */
    attr_print(src, ATTR_FLAG_NONE,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, state->queue_id,
	       ATTR_TYPE_END);
    if (attr_scan(src, ATTR_FLAG_STRICT,
		  ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, &flags,
		  ATTR_TYPE_END) != 1) {
	state->errs |= CLEANUP_STAT_BAD;
	flags = 0;
    }
    cleanup_control(state, flags);

    /*
     * XXX Rely on the front-end programs to enforce record size limits.
     * 
     * First, copy the envelope records to the queue file. Then, copy the
     * message content (headers and body). Finally, attach any information
     * extracted from message headers.
     */
    while (CLEANUP_OUT_OK(state)) {
	if ((type = rec_get(src, buf, 0)) < 0) {
	    state->errs |= CLEANUP_STAT_BAD;
	    break;
	}
	CLEANUP_RECORD(state, type, vstring_str(buf), VSTRING_LEN(buf));
	if (type == REC_TYPE_END)
	    break;
    }

    /*
     * Keep reading in case of problems, until the sender is ready to receive
     * our status report.
     */
    if (CLEANUP_OUT_OK(state) == 0 && type > 0) {
	if ((state->errs & CLEANUP_STAT_CONT) == 0
	    && (state->flags & CLEANUP_FLAG_DISCARD) == 0)
	    msg_warn("%s: skipping further client input", state->queue_id);
	while (type != REC_TYPE_END
	       && (type = rec_get(src, buf, 0)) > 0)
	     /* void */ ;
    }

    /*
     * Log something to make timeout errors easier to debug.
     */
    if (vstream_ftimeout(src))
	msg_warn("%s: read timeout on %s",
		 state->queue_id, VSTREAM_PATH(src));

    /*
     * Finish this message, and report the result status to the client.
     */
    attr_print(src, ATTR_FLAG_NONE,
	       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, cleanup_flush(state),
	       ATTR_TYPE_STR, MAIL_ATTR_WHY, state->reason ?
	       state->reason : "",
	       ATTR_TYPE_END);
    cleanup_free(state);

    /*
     * Cleanup.
     */
    vstring_free(buf);
}

/* cleanup_sig - cleanup after signal */

static void cleanup_sig(int sig)
{
    cleanup_all();
    exit(sig);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* main - the main program */

int     main(int argc, char **argv)
{

    /*
     * Clean up an incomplete queue file in case of a fatal run-time error,
     * or after receiving SIGTERM from the master at shutdown time.
     */
    signal(SIGTERM, cleanup_sig);
    msg_cleanup(cleanup_all);

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, cleanup_service,
		       MAIL_SERVER_BOOL_TABLE, cleanup_bool_table,
		       MAIL_SERVER_INT_TABLE, cleanup_int_table,
		       MAIL_SERVER_STR_TABLE, cleanup_str_table,
		       MAIL_SERVER_TIME_TABLE, cleanup_time_table,
		       MAIL_SERVER_PRE_INIT, cleanup_pre_jail,
		       MAIL_SERVER_POST_INIT, cleanup_post_jail,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_IN_FLOW_DELAY,
		       MAIL_SERVER_UNLIMITED,
		       0);
}
