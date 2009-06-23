/*	$NetBSD: qmqpd.c,v 1.1.1.1 2009/06/23 10:08:53 tron Exp $	*/

/*++
/* NAME
/*	qmqpd 8
/* SUMMARY
/*	Postfix QMQP server
/* SYNOPSIS
/*	\fBqmqpd\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix QMQP server receives one message per connection.
/*	Each message is piped through the \fBcleanup\fR(8)
/*	daemon, and is placed into the \fBincoming\fR queue as one
/*	single queue file.  The program expects to be run from the
/*	\fBmaster\fR(8) process manager.
/*
/*	The QMQP server implements one access policy: only explicitly
/*	authorized client hosts are allowed to use the service.
/* SECURITY
/* .ad
/* .fi
/*	The QMQP server is moderately security-sensitive. It talks to QMQP
/*	clients and to DNS servers on the network. The QMQP server can be
/*	run chrooted at fixed low privilege.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The QMQP protocol provides only one server reply per message
/*	delivery. It is therefore not possible to reject individual
/*	recipients.
/*
/*	The QMQP protocol requires the server to receive the entire
/*	message before replying. If a message is malformed, or if any
/*	netstring component is longer than acceptable, Postfix replies
/*	immediately and closes the connection. It is left up to the
/*	client to handle the situation.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as \fBqmqpd\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/* .IP "\fBcontent_filter (empty)\fR"
/*	The name of a mail delivery transport that filters mail after
/*	it is queued.
/* .IP "\fBreceive_override_options (empty)\fR"
/*	Enable or disable recipient validation, built-in content
/*	filtering, or address mapping.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBline_length_limit (2048)\fR"
/*	Upon input, long lines are chopped up into pieces of at most
/*	this length; upon delivery, long lines are reconstructed.
/* .IP "\fBhopcount_limit (50)\fR"
/*	The maximal number of Received:  message headers that is allowed
/*	in the primary message headers.
/* .IP "\fBmessage_size_limit (10240000)\fR"
/*	The maximal size in bytes of a message, including envelope information.
/* .IP "\fBqmqpd_timeout (300s)\fR"
/*	The time limit for sending or receiving information over the network.
/* TROUBLE SHOOTING CONTROLS
/* .ad
/* .fi
/* .IP "\fBdebug_peer_level (2)\fR"
/*	The increment in verbose logging level when a remote client or
/*	server matches a pattern in the debug_peer_list parameter.
/* .IP "\fBdebug_peer_list (empty)\fR"
/*	Optional list of remote client or server hostname or network
/*	address patterns that cause the verbose logging level to increase
/*	by the amount specified in $debug_peer_level.
/* .IP "\fBsoft_bounce (no)\fR"
/*	Safety net to keep mail queued that would otherwise be returned to
/*	the sender.
/* TARPIT CONTROLS
/* .ad
/* .fi
/* .IP "\fBqmqpd_error_delay (1s)\fR"
/*	How long the QMQP server will pause before sending a negative reply
/*	to the client.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqmqpd_authorized_clients (empty)\fR"
/*	What clients are allowed to connect to the QMQP server port.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .IP "\fBverp_delimiter_filter (-=+)\fR"
/*	The characters Postfix accepts as VERP delimiter characters on the
/*	Postfix \fBsendmail\fR(1) command line and in SMTP commands.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBqmqpd_client_port_logging (no)\fR"
/*	Enable logging of the remote QMQP client port in addition to
/*	the hostname and IP address.
/* SEE ALSO
/*	http://cr.yp.to/proto/qmqp.html, QMQP protocol
/*	cleanup(8), message canonicalization
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	QMQP_README, Postfix ezmlm-idx howto.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	The qmqpd service was introduced with Postfix version 1.1.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <netstring.h>
#include <dict.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <mail_date.h>
#include <mail_conf.h>
#include <debug_peer.h>
#include <mail_stream.h>
#include <namadr_list.h>
#include <quote_822_local.h>
#include <match_parent_style.h>
#include <lex_822.h>
#include <verp_sender.h>
#include <input_transp.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific */

#include <qmqpd.h>

 /*
  * Tunable parameters. Make sure that there is some bound on the length of a
  * netstring, so that the mail system stays in control even when a malicious
  * client sends netstrings of unreasonable length. The recipient count limit
  * is enforced by the message size limit.
  */
int     var_qmqpd_timeout;
int     var_qmqpd_err_sleep;
char   *var_filter_xport;
char   *var_qmqpd_clients;
char   *var_input_transp;
bool    var_qmqpd_client_port_log;

 /*
  * Silly little macros.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

#define DO_LOG		1
#define DONT_LOG	0

 /*
  * Access control. This service should be exposed only to explicitly
  * authorized clients. There is no default authorization.
  */
static NAMADR_LIST *qmqpd_clients;

 /*
  * Transparency: before mail is queued, do we allow address mapping,
  * automatic bcc, header/body checks?
  */
int     qmqpd_input_transp_mask;

/* qmqpd_open_file - open a queue file */

static void qmqpd_open_file(QMQPD_STATE *state)
{
    int     cleanup_flags;

    /*
     * Connect to the cleanup server. Log client name/address with queue ID.
     */
    cleanup_flags = input_transp_cleanup(CLEANUP_FLAG_MASK_EXTERNAL,
					 qmqpd_input_transp_mask);
    state->dest = mail_stream_service(MAIL_CLASS_PUBLIC, var_cleanup_service);
    if (state->dest == 0
	|| attr_print(state->dest->stream, ATTR_FLAG_NONE,
		      ATTR_TYPE_INT, MAIL_ATTR_FLAGS, cleanup_flags,
		      ATTR_TYPE_END) != 0)
	msg_fatal("unable to connect to the %s %s service",
		  MAIL_CLASS_PUBLIC, var_cleanup_service);
    state->cleanup = state->dest->stream;
    state->queue_id = mystrdup(state->dest->id);
    msg_info("%s: client=%s", state->queue_id, state->namaddr);

    /*
     * Record the time of arrival. Optionally, enable content filtering (not
     * bloody likely, but present for the sake of consistency with all other
     * Postfix points of entrance).
     */
    rec_fprintf(state->cleanup, REC_TYPE_TIME, REC_TYPE_TIME_FORMAT,
		REC_TYPE_TIME_ARG(state->arrival_time));
    if (*var_filter_xport)
	rec_fprintf(state->cleanup, REC_TYPE_FILT, "%s", var_filter_xport);
}

/* qmqpd_read_content - receive message content */

static void qmqpd_read_content(QMQPD_STATE *state)
{
    state->where = "receiving message content";
    netstring_get(state->client, state->message, var_message_limit);
}

/* qmqpd_copy_sender - copy envelope sender */

static void qmqpd_copy_sender(QMQPD_STATE *state)
{
    char   *end_prefix;
    char   *end_origin;
    int     verp_requested;
    static char verp_delims[] = "-=";

    /*
     * If the sender address looks like prefix@origin-@[], then request
     * variable envelope return path delivery, with an envelope sender
     * address of prefi@origin, and with VERP delimiters of x and =. This
     * way, the recipients will see envelope sender addresses that look like:
     * prefixuser=domain@origin.
     */
    state->where = "receiving sender address";
    netstring_get(state->client, state->buf, var_line_limit);
    VSTRING_TERMINATE(state->buf);
    verp_requested =
	((end_origin = vstring_end(state->buf) - 4) > STR(state->buf)
	 && strcmp(end_origin, "-@[]") == 0
	 && (end_prefix = strchr(STR(state->buf), '@')) != 0	/* XXX */
	 && --end_prefix < end_origin - 2	/* non-null origin */
	 && end_prefix > STR(state->buf));	/* non-null prefix */
    if (verp_requested) {
	verp_delims[0] = end_prefix[0];
	if (verp_delims_verify(verp_delims) != 0) {
	    state->err |= CLEANUP_STAT_CONT;	/* XXX */
	    vstring_sprintf(state->why_rejected, "Invalid VERP delimiters: \"%s\". Need two characters from \"%s\"",
			    verp_delims, var_verp_filter);
	}
	memmove(end_prefix, end_prefix + 1, end_origin - end_prefix - 1);
	vstring_truncate(state->buf, end_origin - STR(state->buf) - 1);
    }
    if (state->err == CLEANUP_STAT_OK
	&& REC_PUT_BUF(state->cleanup, REC_TYPE_FROM, state->buf) < 0)
	state->err = CLEANUP_STAT_WRITE;
    if (verp_requested)
	if (state->err == CLEANUP_STAT_OK
	    && rec_put(state->cleanup, REC_TYPE_VERP, verp_delims, 2) < 0)
	    state->err = CLEANUP_STAT_WRITE;
    state->sender = mystrndup(STR(state->buf), LEN(state->buf));
}

/* qmqpd_write_attributes - save session attributes */

static void qmqpd_write_attributes(QMQPD_STATE *state)
{

    /*
     * Logging attributes, also used for XFORWARD.
     */
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_LOG_CLIENT_NAME, state->name);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_LOG_CLIENT_ADDR, state->rfc_addr);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_LOG_CLIENT_PORT, state->port);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_LOG_ORIGIN, state->namaddr);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_LOG_PROTO_NAME, state->protocol);

    /*
     * For consistency with the smtpd Milter client, we need to provide the
     * real client attributes to the cleanup Milter client. This does not
     * matter much with qmqpd which speaks to trusted clients only, but we
     * want to be sure that the cleanup input protocol is ready when a new
     * type of network daemon is added to receive mail from the Internet.
     * 
     * See also the comments in smtpd.c.
     */
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_ACT_CLIENT_NAME, state->name);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_ACT_CLIENT_ADDR, state->addr);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_ACT_CLIENT_PORT, state->port);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%u",
		MAIL_ATTR_ACT_CLIENT_AF, state->addr_family);
    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_ACT_PROTO_NAME, state->protocol);

    /* XXX What about the address rewriting context? */
}

/* qmqpd_copy_recipients - copy message recipients */

static void qmqpd_copy_recipients(QMQPD_STATE *state)
{
    int     ch;

    /*
     * Remember the first recipient. We are done when we read the over-all
     * netstring terminator.
     * 
     * XXX This approach violates abstractions, but it is a heck of a lot more
     * convenient than counting the over-all byte count down to zero, like
     * qmail does.
     */
    state->where = "receiving recipient address";
    while ((ch = VSTREAM_GETC(state->client)) != ',') {
	vstream_ungetc(state->client, ch);
	netstring_get(state->client, state->buf, var_line_limit);
	if (state->err == CLEANUP_STAT_OK
	    && REC_PUT_BUF(state->cleanup, REC_TYPE_RCPT, state->buf) < 0)
	    state->err = CLEANUP_STAT_WRITE;
	state->rcpt_count++;
	if (state->recipient == 0)
	    state->recipient = mystrndup(STR(state->buf), LEN(state->buf));
    }
}

/* qmqpd_next_line - get line from buffer, return last char, newline, or -1 */

static int qmqpd_next_line(VSTRING *message, char **start, int *len,
			           char **next)
{
    char   *beyond = STR(message) + LEN(message);
    char   *enough = *next + var_line_limit;
    char   *cp;

    /*
     * Stop at newline or at some limit. Don't look beyond the end of the
     * buffer.
     */
#define UCHARPTR(x) ((unsigned char *) (x))

    for (cp = *start = *next; /* void */ ; cp++) {
	if (cp >= beyond)
	    return ((*len = (*next = cp) - *start) > 0 ? UCHARPTR(cp)[-1] : -1);
	if (*cp == '\n')
	    return ((*len = cp - *start), (*next = cp + 1), '\n');
	if (cp >= enough)
	    return ((*len = cp - *start), (*next = cp), UCHARPTR(cp)[-1]);
    }
}

/* qmqpd_write_content - write the message content segment */

static void qmqpd_write_content(QMQPD_STATE *state)
{
    char   *start;
    char   *next;
    int     len;
    int     rec_type;
    int     first = 1;
    int     ch;

    /*
     * Start the message content segment. Prepend our own Received: header to
     * the message content. List the recipient only when a message has one
     * recipient. Otherwise, don't list the recipient to avoid revealing Bcc:
     * recipients that are supposed to be invisible.
     */
    rec_fputs(state->cleanup, REC_TYPE_MESG, "");
    rec_fprintf(state->cleanup, REC_TYPE_NORM, "Received: from %s (%s [%s])",
		state->name, state->name, state->rfc_addr);
    if (state->rcpt_count == 1 && state->recipient) {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s id %s",
		    var_myhostname, var_mail_name,
		    state->protocol, state->queue_id);
	quote_822_local(state->buf, state->recipient);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tfor <%s>; %s", STR(state->buf),
		    mail_date(state->arrival_time.tv_sec));
    } else {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s",
		    var_myhostname, var_mail_name, state->protocol);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tid %s; %s", state->queue_id,
		    mail_date(state->arrival_time.tv_sec));
    }
#ifdef RECEIVED_ENVELOPE_FROM
    quote_822_local(state->buf, state->sender);
    rec_fprintf(state->cleanup, REC_TYPE_NORM,
		"\t(envelope-from <%s>)", STR(state->buf));
#endif

    /*
     * Write the message content.
     * 
     * XXX Force an empty record when the queue file content begins with
     * whitespace, so that it won't be considered as being part of our own
     * Received: header. What an ugly Kluge.
     * 
     * XXX Deal with UNIX-style From_ lines at the start of message content just
     * in case.
     */
    for (next = STR(state->message); /* void */ ; /* void */ ) {
	if ((ch = qmqpd_next_line(state->message, &start, &len, &next)) < 0)
	    break;
	if (ch == '\n')
	    rec_type = REC_TYPE_NORM;
	else
	    rec_type = REC_TYPE_CONT;
	if (first) {
	    if (strncmp(start + strspn(start, ">"), "From ", 5) == 0) {
		rec_fprintf(state->cleanup, rec_type,
			    "X-Mailbox-Line: %*s", len, start);
		continue;
	    }
	    first = 0;
	    if (len > 0 && IS_SPACE_TAB(start[0]))
		rec_put(state->cleanup, REC_TYPE_NORM, "", 0);
	}
	if (rec_put(state->cleanup, rec_type, start, len) < 0) {
	    state->err = CLEANUP_STAT_WRITE;
	    return;
	}
    }
}

/* qmqpd_close_file - close queue file */

static void qmqpd_close_file(QMQPD_STATE *state)
{

    /*
     * Send the end-of-segment markers.
     */
    if (state->err == CLEANUP_STAT_OK)
	if (rec_fputs(state->cleanup, REC_TYPE_XTRA, "") < 0
	    || rec_fputs(state->cleanup, REC_TYPE_END, "") < 0
	    || vstream_fflush(state->cleanup))
	    state->err = CLEANUP_STAT_WRITE;

    /*
     * Finish the queue file or finish the cleanup conversation.
     */
    if (state->err == 0)
	state->err = mail_stream_finish(state->dest, state->why_rejected);
    else
	mail_stream_cleanup(state->dest);
    state->dest = 0;
}

/* qmqpd_reply - send status to client and optionally log message */

static void qmqpd_reply(QMQPD_STATE *state, int log_message,
			        int status_code, const char *fmt,...)
{
    va_list ap;

    /*
     * Optionally change hard errors into retryable ones. Send the reply and
     * optionally log it. Always insert a delay before reporting a problem.
     * This slows down software run-away conditions.
     */
    if (status_code == QMQPD_STAT_HARD && var_soft_bounce)
	status_code = QMQPD_STAT_RETRY;
    VSTRING_RESET(state->buf);
    VSTRING_ADDCH(state->buf, status_code);
    va_start(ap, fmt);
    vstring_vsprintf_append(state->buf, fmt, ap);
    va_end(ap);
    NETSTRING_PUT_BUF(state->client, state->buf);
    if (log_message)
	(status_code == QMQPD_STAT_OK ? msg_info : msg_warn) ("%s: %s: %s",
		      state->queue_id, state->namaddr, STR(state->buf) + 1);
    if (status_code != QMQPD_STAT_OK)
	sleep(var_qmqpd_err_sleep);
    netstring_fflush(state->client);
}

/* qmqpd_send_status - send mail transaction completion status */

static void qmqpd_send_status(QMQPD_STATE *state)
{

    /*
     * One message may suffer from multiple errors, so complain only about
     * the most severe error.
     * 
     * See also: smtpd.c
     */
    state->where = "sending completion status";

    if (state->err == CLEANUP_STAT_OK) {
	qmqpd_reply(state, DONT_LOG, QMQPD_STAT_OK,
		    "Ok: queued as %s", state->queue_id);
    } else if ((state->err & CLEANUP_STAT_DEFER) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_RETRY,
		    "Error: %s", STR(state->why_rejected));
    } else if ((state->err & CLEANUP_STAT_BAD) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_RETRY,
		    "Error: internal error %d", state->err);
    } else if ((state->err & CLEANUP_STAT_SIZE) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_HARD,
		    "Error: message too large");
    } else if ((state->err & CLEANUP_STAT_HOPS) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_HARD,
		    "Error: too many hops");
    } else if ((state->err & CLEANUP_STAT_CONT) != 0) {
	qmqpd_reply(state, DO_LOG, STR(state->why_rejected)[0] == '4' ?
		    QMQPD_STAT_RETRY : QMQPD_STAT_HARD,
		    "Error: %s", STR(state->why_rejected));
    } else if ((state->err & CLEANUP_STAT_WRITE) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_RETRY,
		    "Error: queue file write error");
    } else if ((state->err & CLEANUP_STAT_RCPT) != 0) {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_HARD,
		    "Error: no recipients specified");
    } else {
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_RETRY,
		    "Error: internal error %d", state->err);
    }
}

/* qmqpd_receive - receive QMQP message+sender+recipients */

static void qmqpd_receive(QMQPD_STATE *state)
{

    /*
     * Open a queue file. This must be first so that we can simplify the
     * error logging and always include the queue ID information.
     */
    qmqpd_open_file(state);

    /*
     * Read and ignore the over-all netstring length indicator.
     */
    state->where = "receiving QMQP packet header";
    (void) netstring_get_length(state->client);

    /*
     * XXX Read the message content into memory, because Postfix expects to
     * store the sender before storing the message content. Fixing that
     * requires changes to pickup, cleanup, qmgr, and perhaps elsewhere, so
     * that will have to happen later when I have more time. However, QMQP is
     * used for mailing list distribution, so the bulk of the volume is
     * expected to be not message content but recipients, and recipients are
     * not accumulated in memory.
     */
    qmqpd_read_content(state);

    /*
     * Read and write the envelope sender.
     */
    qmqpd_copy_sender(state);

    /*
     * Record some session attributes.
     */
    qmqpd_write_attributes(state);

    /*
     * Read and write the envelope recipients, including the optional big
     * brother recipient.
     */
    qmqpd_copy_recipients(state);

    /*
     * Start the message content segment, prepend our own Received: header,
     * and write the message content.
     */
    if (state->err == 0)
	qmqpd_write_content(state);

    /*
     * Close the queue file.
     */
    qmqpd_close_file(state);

    /*
     * Report the completion status to the client.
     */
    qmqpd_send_status(state);
}

/* qmqpd_proto - speak the QMQP "protocol" */

static void qmqpd_proto(QMQPD_STATE *state)
{
    int     status;

    netstring_setup(state->client, var_qmqpd_timeout);

    switch (status = vstream_setjmp(state->client)) {

    default:
	msg_panic("qmqpd_proto: unknown status %d", status);

    case NETSTRING_ERR_EOF:
	state->reason = "lost connection";
	break;

    case NETSTRING_ERR_TIME:
	state->reason = "read/write timeout";
	break;

    case NETSTRING_ERR_FORMAT:
	state->reason = "netstring format error";
	if (vstream_setjmp(state->client) == 0)
	    if (state->reason && state->where)
		qmqpd_reply(state, DONT_LOG, QMQPD_STAT_HARD, "%s while %s",
			    state->reason, state->where);
	break;

    case NETSTRING_ERR_SIZE:
	state->reason = "netstring length exceeds storage limit";
	if (vstream_setjmp(state->client) == 0)
	    if (state->reason && state->where)
		qmqpd_reply(state, DONT_LOG, QMQPD_STAT_HARD, "%s while %s",
			    state->reason, state->where);
	break;

    case 0:

	/*
	 * See if we want to talk to this client at all.
	 */
	if (namadr_list_match(qmqpd_clients, state->name, state->addr) == 0) {
	    qmqpd_reply(state, DONT_LOG, QMQPD_STAT_HARD,
			"Error: %s is not authorized to use this service",
			state->namaddr);
	} else
	    qmqpd_receive(state);
	break;
    }

    /*
     * Log abnormal session termination. Indicate the last recognized state
     * before things went wrong.
     */
    if (state->reason && state->where)
	msg_info("%s: %s: %s while %s",
	      state->queue_id, state->namaddr, state->reason, state->where);
}

/* qmqpd_service - service one client */

static void qmqpd_service(VSTREAM *stream, char *unused_service, char **argv)
{
    QMQPD_STATE *state;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs when a client has connected to our network port.
     * Look up and sanitize the peer name and initialize some connection-
     * specific state.
     */
    state = qmqpd_state_alloc(stream);

    /*
     * See if we need to turn on verbose logging for this client.
     */
    debug_peer_check(state->name, state->addr);

    /*
     * Provide the QMQP service.
     */
    msg_info("connect from %s", state->namaddr);
    qmqpd_proto(state);
    msg_info("disconnect from %s", state->namaddr);

    /*
     * After the client has gone away, clean up whatever we have set up at
     * connection time.
     */
    debug_peer_restore();
    qmqpd_state_free(state);
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

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    debug_peer_init();
    qmqpd_clients =
	namadr_list_init(match_parent_style(VAR_QMQPD_CLIENTS),
			 var_qmqpd_clients);
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Initialize the receive transparency options: do we want unknown
     * recipient checks, do we want address mapping.
     */
    qmqpd_input_transp_mask =
    input_transp_mask(VAR_INPUT_TRANSP, var_input_transp);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_QMTPD_TMOUT, DEF_QMTPD_TMOUT, &var_qmqpd_timeout, 1, 0,
	VAR_QMTPD_ERR_SLEEP, DEF_QMTPD_ERR_SLEEP, &var_qmqpd_err_sleep, 0, 0,
	0,
    };
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_FILTER_XPORT, DEF_FILTER_XPORT, &var_filter_xport, 0, 0,
	VAR_QMQPD_CLIENTS, DEF_QMQPD_CLIENTS, &var_qmqpd_clients, 0, 0,
	VAR_INPUT_TRANSP, DEF_INPUT_TRANSP, &var_input_transp, 0, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_QMQPD_CLIENT_PORT_LOG, DEF_QMQPD_CLIENT_PORT_LOG, &var_qmqpd_client_port_log,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, qmqpd_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       0);
}
