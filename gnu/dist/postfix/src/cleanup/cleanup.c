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
/*	Changes to \fBmain.cf\fR are picked up automatically, as cleanup(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	postconf(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBundisclosed_recipients_header (To: undisclosed-recipients:;)\fR"
/*	Message header that the Postfix cleanup(8) server inserts when a
/*	message contains no To: or Cc: message header.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBenable_errors_to (no)\fR"
/*	Report mail delivery errors to the address specified with the
/*	non-standard Errors-To: message header, instead of the envelope
/*	sender address.
/* BUILT-IN CONTENT FILTERING CONTROLS
/* .ad
/* .fi
/*	Postfix built-in content filtering is meant to stop a flood of
/*	worms or viruses. It is not a general content filter.
/* .IP "\fBbody_checks (empty)\fR"
/*	Optional lookup tables for content inspection as specified in
/*	the body_checks(5) manual page.
/* .IP "\fBheader_checks (empty)\fR"
/*	Optional lookup tables for content inspection of primary non-MIME
/*	message headers, as specified in the header_checks(5) manual page.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBbody_checks_size_limit (51200)\fR"
/*	How much text in a message body segment (or attachment, if you
/*	prefer to use that term) is subjected to body_checks inspection.
/* .IP "\fBmime_header_checks ($header_checks)\fR"
/*	Optional lookup tables for content inspection of MIME related
/*	message headers, as described in the header_checks(5) manual page.
/* .IP "\fBnested_header_checks ($header_checks)\fR"
/*	Optional lookup tables for content inspection of non-MIME message
/*	headers in attached messages, as described in the header_checks(5)
/*	manual page.
/* MIME PROCESSING CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBdisable_mime_input_processing (no)\fR"
/*	Turn off MIME processing while receiving mail.
/* .IP "\fBmime_boundary_length_limit (2048)\fR"
/*	The maximal length of MIME multipart boundary strings.
/* .IP "\fBmime_nesting_limit (100)\fR"
/*	The maximal nesting level of multipart mail that the MIME processor
/*	will handle.
/* .IP "\fBstrict_8bitmime (no)\fR"
/*	Enable both strict_7bit_headers and strict_8bitmime_body.
/* .IP "\fBstrict_7bit_headers (no)\fR"
/*	Reject mail with 8-bit text in message headers.
/* .IP "\fBstrict_8bitmime_body (no)\fR"
/*	Reject 8-bit message body text without 8-bit MIME content encoding
/*	information.
/* .IP "\fBstrict_mime_encoding_domain (no)\fR"
/*	Reject mail with invalid Content-Transfer-Encoding: information
/*	for the message/* or multipart/* MIME content types.
/* AUTOMATIC BCC RECIPIENT CONTROLS
/* .ad
/* .fi
/*	Postfix can automatically add BCC (blind carbon copy)
/*	when mail enters the mail system:
/* .IP "\fBalways_bcc (empty)\fR"
/*	Optional address that receives a "blind carbon copy" of each message
/*	that is received by the Postfix mail system.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsender_bcc_maps (empty)\fR"
/*	Optional BCC (blind carbon-copy) address lookup tables, indexed
/*	by sender address.
/* .IP "\fBrecipient_bcc_maps (empty)\fR"
/*	Optional BCC (blind carbon-copy) address lookup tables, indexed by
/*	recipient address.
/* ADDRESS TRANSFORMATION CONTROLS
/* .ad
/* .fi
/*	Address rewriting is delegated to the trivial-rewrite(8) daemon.
/*	The cleanup(8) server implements table driven address mapping.
/* .IP "\fBempty_address_recipient (MAILER-DAEMON)\fR"
/*	The recipient of mail addressed to the null address.
/* .IP "\fBcanonical_maps (empty)\fR"
/*	Optional address mapping lookup tables for message headers and
/*	envelopes.
/* .IP "\fBrecipient_canonical_maps (empty)\fR"
/*	Optional address mapping lookup tables for envelope and header
/*	recipient addresses.
/* .IP "\fBsender_canonical_maps (empty)\fR"
/*	Optional address mapping lookup tables for envelope and header
/*	sender addresses.
/* .IP "\fBmasquerade_classes (envelope_sender, header_sender, header_recipient)\fR"
/*	What addresses are subject to address masquerading.
/* .IP "\fBmasquerade_domains (empty)\fR"
/*	Optional list of domains whose subdomain structure will be stripped
/*	off in email addresses.
/* .IP "\fBmasquerade_exceptions (empty)\fR"
/*	Optional list of user names that are not subjected to address
/*	masquerading, even when their address matches $masquerade_domains.
/* .IP "\fBpropagate_unmatched_extensions (canonical, virtual)\fR"
/*	What address lookup tables copy an address extension from the lookup
/*	key to the lookup result.
/* .PP
/*	Available before Postfix version 2.0:
/* .IP "\fBvirtual_maps (empty)\fR"
/*	Optional lookup tables with a) names of domains for which all
/*	addresses are aliased to addresses in other local or remote domains,
/*	and b) addresses that are aliased to addresses in other local or
/*	remote domains.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBvirtual_alias_maps ($virtual_maps)\fR"
/*	Optional lookup tables that alias specific mail addresses or domains
/*	to other local or remote address.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBduplicate_filter_limit (1000)\fR"
/*	The maximal number of addresses remembered by the address
/*	duplicate filter for aliases(5) or virtual(5) alias expansion, or
/*	for showq(8) queue displays.
/* .IP "\fBheader_size_limit (102400)\fR"
/*	The maximal amount of memory in bytes for storing a message header.
/* .IP "\fBhopcount_limit (50)\fR"
/*	The maximal number of Received:  message headers that is allowed
/*	in the primary message headers.
/* .IP "\fBin_flow_delay (1s)\fR"
/*	Time to pause before accepting a new message, when the message
/*	arrival rate exceeds the message delivery rate.
/* .IP "\fBmessage_size_limit (10240000)\fR"
/*	The maximal size in bytes of a message, including envelope information.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBheader_address_token_limit (10240)\fR"
/*	The maximal number of address tokens are allowed in an address
/*	message header.
/* .IP "\fBmime_boundary_length_limit (2048)\fR"
/*	The maximal length of MIME multipart boundary strings.
/* .IP "\fBmime_nesting_limit (100)\fR"
/*	The maximal nesting level of multipart mail that the MIME processor
/*	will handle.
/* .IP "\fBqueue_file_attribute_count_limit (100)\fR"
/*	The maximal number of (name=value) attributes that may be stored
/*	in a Postfix queue file.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBvirtual_alias_expansion_limit (1000)\fR"
/*	The maximal number of addresses that virtual alias expansion produces
/*	from each original recipient.
/* .IP "\fBvirtual_alias_recursion_limit (1000)\fR"
/*	The maximal nesting depth of virtual alias expansion.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBdelay_warning_time (0h)\fR"
/*	The time after which the sender receives the message headers of
/*	mail that is still queued.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process
/*	waits for the next service request before exiting.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of connection requests before a Postfix daemon
/*	process terminates.
/* .IP "\fBmyhostname (see 'postconf -d' output)\fR"
/*	The internet hostname of this mail system.
/* .IP "\fBmyorigin ($myhostname)\fR"
/*	The default domain name that locally-posted mail appears to come
/*	from, and that locally posted mail is delivered to.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsoft_bounce (no)\fR"
/*	Safety net to keep mail queued that would otherwise be returned to
/*	the sender.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBenable_original_recipient (yes)\fR"
/*	Enable support for the X-Original-To message header.
/* FILES
/*	/etc/postfix/canonical*, canonical mapping table
/*	/etc/postfix/virtual*, virtual mapping table
/* SEE ALSO
/*	trivial-rewrite(8), address rewriting
/*	qmgr(8), queue manager
/*	header_checks(5), message header content inspection
/*	body_checks(5), body parts content inspection
/*	canonical(5), canonical address lookup table format
/*	virtual(5), virtual alias lookup table format
/*	postconf(5), configuration parameters
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	ADDRESS_REWRITING_README Postfix address manipulation
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
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
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
		       MAIL_SERVER_BOOL_TABLE, cleanup_bool_table,
		       MAIL_SERVER_STR_TABLE, cleanup_str_table,
		       MAIL_SERVER_TIME_TABLE, cleanup_time_table,
		       MAIL_SERVER_PRE_INIT, cleanup_pre_jail,
		       MAIL_SERVER_POST_INIT, cleanup_post_jail,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_IN_FLOW_DELAY,
		       MAIL_SERVER_UNLIMITED,
		       0);
}
