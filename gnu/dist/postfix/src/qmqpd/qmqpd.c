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
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalways_bcc\fR
/*	Address to send a copy of each message that enters the system.
/* .IP \fBdebug_peer_level\fR
/*	Increment in verbose logging level when a remote host matches a
/*	pattern in the \fBdebug_peer_list\fR parameter.
/* .IP \fBdebug_peer_list\fR
/*	List of domain or network patterns. When a remote host matches
/*	a pattern, increase the verbose logging level by the amount
/*	specified in the \fBdebug_peer_level\fR parameter.
/* .IP \fBhopcount_limit\fR
/*	Limit the number of \fBReceived:\fR message headers.
/* .IP \fBqmqpd_authorized_clients\fR
/*      A list of domain or network patterns that specifies what
/*	clients are allowed to use the service.
/* .IP \fBqmqpd_timeout\fR
/*	Limit the time to send a server response and to receive a client
/*	request.
/* .IP \fBsoft_bounce\fR
/*	Change hard (D) reject responses into soft (Z) reject responses.
/*	This can be useful for testing purposes.
/* .SH "Content inspection controls"
/* .IP \fBcontent_filter\fR
/*	The name of a mail delivery transport that filters mail and that
/*	either bounces mail or re-injects the result back into Postfix.
/*	This parameter uses the same syntax as the right-hand side of
/*	a Postfix transport table.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBline_length_limit\fR
/*	Limit the amount of memory in bytes used for the handling of
/*	partial input lines, and the length of sender and recipient
/*	addresses that are received from client.
/* .IP \fBmessage_size_limit\fR
/*	Limit the total size in bytes of a message, including on-disk
/*	storage for sender and recipient address information.
/* .SH Tarpitting
/* .ad
/* .fi
/* .IP \fBqmqpd_error_sleep_time\fR
/*	Time to wait in seconds before informing the client of
/*	a problem. This slows down run-away errors.
/* SEE ALSO
/*	http://cr.yp.to/proto/qmqp.html, QMQP protocol
/*	cleanup(8) message canonicalization
/*	master(8) process manager
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <netstring.h>
#include <dict.h>

/* Global library. */

#include <mail_params.h>
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
char   *var_always_bcc;
char   *var_filter_xport;
char   *var_qmqpd_clients;

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

/* qmqpd_open_file - open a queue file */

static void qmqpd_open_file(QMQPD_STATE *state)
{

    /*
     * Connect to the cleanup server. Log client name/address with queue ID.
     */
    state->dest = mail_stream_service(MAIL_CLASS_PUBLIC, MAIL_SERVICE_CLEANUP);
    if (state->dest == 0
	|| attr_print(state->dest->stream, ATTR_FLAG_NONE,
		      ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, CLEANUP_FLAG_FILTER,
		      ATTR_TYPE_END) != 0)
	msg_fatal("unable to connect to the %s %s service",
		  MAIL_CLASS_PUBLIC, MAIL_SERVICE_CLEANUP);
    state->cleanup = state->dest->stream;
    state->queue_id = mystrdup(state->dest->id);
    msg_info("%s: client=%s", state->queue_id, state->namaddr);

    /*
     * Record the time of arrival. Optionally, enable content filtering (not
     * bloody likely, but present for the sake of consistency with all other
     * Postfix points of entrance).
     */
    rec_fprintf(state->cleanup, REC_TYPE_TIME, "%ld", (long) state->time);
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

    /*
     * If the sender address looks like prefix-@origin-@[], then request
     * variable envelope return path delivery, with an envelope sender
     * address of prefix@origin, and with VERP delimiters of - and =. This
     * way, the recipients will see envelope sender addresses that look like:
     * prefix-user=domain@origin.
     */
    state->where = "receiving sender address";
    netstring_get(state->client, state->buf, var_line_limit);
    VSTRING_TERMINATE(state->buf);
    verp_requested = ((end_prefix = strstr(STR(state->buf), "-@")) != 0
		      && (end_origin = strstr(end_prefix + 2, "-@")) != 0
		      && strncmp(end_origin + 2, "[]", 2) == 0
		      && vstring_end(state->buf) == end_origin + 4);
    if (verp_requested) {
	memcpy(end_prefix, end_prefix + 1, end_origin - end_prefix - 1);
	vstring_truncate(state->buf, end_origin - STR(state->buf) - 1);
    }
    if (state->err == CLEANUP_STAT_OK
	&& REC_PUT_BUF(state->cleanup, REC_TYPE_FROM, state->buf) < 0)
	state->err = CLEANUP_STAT_WRITE;
    if (verp_requested)
	if (state->err == CLEANUP_STAT_OK
	    && rec_put(state->cleanup, REC_TYPE_VERP, "-=", 2) < 0)
	    state->err = CLEANUP_STAT_WRITE;
    state->sender = mystrndup(STR(state->buf), LEN(state->buf));
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

    /*
     * Append the optional recipient who is copied on all mail.
     */
    if (*var_always_bcc)
	rec_fputs(state->cleanup, REC_TYPE_RCPT, var_always_bcc);
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
		state->name, state->name, state->addr);
    if (state->rcpt_count == 1 && state->recipient) {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s id %s",
		    var_myhostname, var_mail_name,
		    state->protocol, state->queue_id);
	quote_822_local(state->buf, state->recipient);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		 "\tfor <%s>; %s", STR(state->buf), mail_date(state->time));
    } else {
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tby %s (%s) with %s",
		    var_myhostname, var_mail_name, state->protocol);
	rec_fprintf(state->cleanup, REC_TYPE_NORM,
		    "\tid %s; %s", state->queue_id, mail_date(state->time));
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
			    "Mailbox-Line: %*s", len, start);
		continue;
	    }
	    first = 0;
	    if (len > 0 && ISSPACE(start[0]))
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
	qmqpd_reply(state, DO_LOG, QMQPD_STAT_HARD,
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
     * Read and write the envelope recipients, including the optional big
     * brother recipient.
     */
    qmqpd_copy_recipients(state);

    /*
     * Start the message content segment, prepend our own Received: header,
     * and write the message content.
     */
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
     * See if we want to talk to this client at all. In all cases, log the
     * connection event.
     */
    if (namadr_list_match(qmqpd_clients, state->name, state->addr) == 0) {
	msg_info("refused connect from %s", state->namaddr);
	qmqpd_reply(state, DONT_LOG, QMQPD_STAT_HARD,
		    "Error: %s is not authorized to use this service",
		    state->namaddr);
    }

    /*
     * Provide the QMQP service.
     */
    else {
	msg_info("connect from %s", state->namaddr);
	qmqpd_proto(state);
	msg_info("disconnect from %s", state->namaddr);
    }

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
    if (dict_changed()) {
	msg_info("lookup table has changed -- exiting");
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

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_QMTPD_TMOUT, DEF_QMTPD_TMOUT, &var_qmqpd_timeout, 1, 0,
	VAR_QMTPD_ERR_SLEEP, DEF_QMTPD_ERR_SLEEP, &var_qmqpd_err_sleep, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
	VAR_FILTER_XPORT, DEF_FILTER_XPORT, &var_filter_xport, 0, 0,
	VAR_QMQPD_CLIENTS, DEF_QMQPD_CLIENTS, &var_qmqpd_clients, 0, 0,
	0,
    };

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, qmqpd_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
