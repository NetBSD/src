/*++
/* NAME
/*	smtp 8
/* SUMMARY
/*	Postfix remote delivery via SMTP
/* SYNOPSIS
/*	\fBsmtp\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The SMTP client processes message delivery requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, a domain or host to deliver to, and recipient information.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The SMTP client updates the queue file and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery problem reports are sent
/*	to the \fBbounce\fR(8) or \fBdefer\fR(8) daemon as appropriate.
/*
/*	The SMTP client looks up a list of mail exchanger addresses for
/*	the destination host, sorts the list by preference, and connects
/*	to each listed address until it finds a server that responds.
/*
/*	When the domain or host is specified as a comma/whitespace
/*	separated list, the SMTP client repeats the above process
/*	for all destinations until it finds a server that responds.
/*
/*	Once the SMTP client has received the server greeting banner, no
/*	error will cause it to proceed to the next address on the mail
/*	exchanger list. Instead, the message is either bounced, or its
/*	delivery is deferred until later.
/* SECURITY
/* .ad
/* .fi
/*	The SMTP client is moderately security-sensitive. It talks to SMTP
/*	servers and to DNS servers on the network. The SMTP client can be
/*	run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 1651 (SMTP service extensions)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 2197 (Pipelining)
/*	RFC 2554 (AUTH command)
/*	RFC 2821 (SMTP protocol)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*	Corrupted message files are marked so that the queue manager can
/*	move them to the \fBcorrupt\fR queue for further inspection.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems, and of
/*	other trouble.
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
/* .IP \fBbest_mx_transport\fR
/*	Name of the delivery transport to use when the local machine
/*	is the most-preferred mail exchanger (by default, a mailer
/*	loop is reported, and the message is bounced).
/* .IP \fBdebug_peer_level\fR
/*	Verbose logging level increment for hosts that match a
/*	pattern in the \fBdebug_peer_list\fR parameter.
/* .IP \fBdebug_peer_list\fR
/*	List of domain or network patterns. When a remote host matches
/*	a pattern, increase the verbose logging level by the amount
/*	specified in the \fBdebug_peer_level\fR parameter.
/* .IP \fBdisable_dns_lookups\fR
/*	Disable DNS lookups. This means that mail must be forwarded
/*	via a smart relay host.
/* .IP \fBerror_notice_recipient\fR
/*	Recipient of protocol/policy/resource/software error notices.
/* .IP \fBfallback_relay\fR
/*	Hosts to hand off mail to if a message destination is not found
/*	or if a destination is unreachable.
/* .IP \fBignore_mx_lookup_error\fR
/*	When a name server fails to respond to an MX query, search for an
/*	A record instead deferring mail delivery.
/* .IP \fBinet_interfaces\fR
/*	The network interface addresses that this mail system receives
/*	mail on. When any of those addresses appears in the list of mail
/*	exchangers for a remote destination, the list is truncated to
/*	avoid mail delivery loops.
/* .IP \fBnotify_classes\fR
/*	When this parameter includes the \fBprotocol\fR class, send mail to the
/*	postmaster with transcripts of SMTP sessions with protocol errors.
/* .IP \fBsmtp_always_send_ehlo\fR
/*	Always send EHLO at the start of a connection.
/* .IP \fBsmtp_never_send_ehlo\fR
/*	Never send EHLO at the start of a connection.
/* .IP \fBsmtp_bind_address\fR
/*	Numerical source network address to bind to when making a connection.
/* .IP \fBsmtp_break_lines\fR
/*	Break lines > \fB$line_length_limit\fR into multiple shorter lines.
/*	Some SMTP servers misbehave on long lines.
/* .IP \fBsmtp_skip_4xx_greeting\fR
/*	Skip servers that greet us with a 4xx status code.
/* .IP \fBsmtp_skip_5xx_greeting\fR
/*	Skip servers that greet us with a 5xx status code.
/* .IP \fBsmtp_skip_quit_response\fR
/*	Do not wait for the server response after sending QUIT.
/* .IP \fBsmtp_pix_workaround_delay_time\fR
/*	The time to pause before sending .<CR><LF>, while working
/*	around the CISCO PIX firewall <CR><LF>.<CR><LF> bug.
/* .IP \fBsmtp_pix_workaround_threshold_time\fR
/*	The time a message must be queued before the CISCO PIX firewall
/*	<CR><LF>.<CR><LF> bug workaround is turned on.
/* .SH "Authentication controls"
/* .IP \fBsmtp_enable_sasl_auth\fR
/*	Enable per-session authentication as per RFC 2554 (SASL).
/*	By default, Postfix is built without SASL support.
/* .IP \fBsmtp_sasl_password_maps\fR
/*	Lookup tables with per-host or domain \fIname\fR:\fIpassword\fR entries.
/*	No entry for a host means no attempt to authenticate.
/* .IP \fBsmtp_sasl_security_options\fR
/*	Zero or more of the following.
/* .RS
/* .IP \fBnoplaintext\fR
/*	Disallow authentication methods that use plaintext passwords.
/* .IP \fBnoactive\fR
/*	Disallow authentication methods that are vulnerable to non-dictionary
/*	active attacks.
/* .IP \fBnodictionary\fR
/*	Disallow authentication methods that are vulnerable to passive
/*	dictionary attack.
/* .IP \fBnoanonymous\fR
/*	Disallow anonymous logins.
/* .RE
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBsmtp_destination_concurrency_limit\fR
/*	Limit the number of parallel deliveries to the same destination.
/*	The default limit is taken from the
/*	\fBdefault_destination_concurrency_limit\fR parameter.
/* .IP \fBsmtp_destination_recipient_limit\fR
/*	Limit the number of recipients per message delivery.
/*	The default limit is taken from the
/*	\fBdefault_destination_recipient_limit\fR parameter.
/* .SH "Timeout controls"
/* .ad
/* .fi
/* .PP
/*	The default time unit is seconds; an explicit time unit can
/*	be specified by appending a one-letter suffix to the value:
/*	s (seconds), m (minutes), h (hours), d (days) or w (weeks).
/* .IP \fBsmtp_connect_timeout\fR
/*	Timeout for completing a TCP connection. When no
/*	connection can be made within the deadline, the SMTP client
/*	tries the next address on the mail exchanger list.
/* .IP \fBsmtp_helo_timeout\fR
/*	Timeout for receiving the SMTP greeting banner.
/*	When the server drops the connection without sending a
/*	greeting banner, or when it sends no greeting banner within the
/*	deadline, the SMTP client tries the next address on the mail
/*	exchanger list.
/* .IP \fBsmtp_helo_timeout\fR
/*	Timeout for sending the \fBHELO\fR command, and for
/*	receiving the server response.
/* .IP \fBsmtp_mail_timeout\fR
/*	Timeout for sending the \fBMAIL FROM\fR command, and for
/*	receiving the server response.
/* .IP \fBsmtp_rcpt_timeout\fR
/*	Timeout for sending the \fBRCPT TO\fR command, and for
/*	receiving the server response.
/* .IP \fBsmtp_data_init_timeout\fR
/*	Timeout for sending the \fBDATA\fR command, and for
/*	receiving the server response.
/* .IP \fBsmtp_data_xfer_timeout\fR
/*	Timeout for sending the message content.
/* .IP \fBsmtp_data_done_timeout\fR
/*	Timeout for sending the "\fB.\fR" command, and for
/*	receiving the server response. When no response is received, a
/*	warning is logged that the mail may be delivered multiple times.
/* .IP \fBsmtp_quit_timeout\fR
/*	Timeout for sending the \fBQUIT\fR command, and for
/*	receiving the server response.
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
#include <string.h>
#include <fcntl.h>
#include <dict.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <name_mask.h>

/* Global library. */

#include <deliver_request.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <debug_peer.h>
#include <mail_error.h>
#include <deliver_pass.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

 /*
  * Tunable parameters. These have compiled-in defaults that can be overruled
  * by settings in the global Postfix configuration file.
  */
int     var_smtp_conn_tmout;
int     var_smtp_helo_tmout;
int     var_smtp_mail_tmout;
int     var_smtp_rcpt_tmout;
int     var_smtp_data0_tmout;
int     var_smtp_data1_tmout;
int     var_smtp_data2_tmout;
int     var_smtp_quit_tmout;
char   *var_inet_interfaces;
char   *var_notify_classes;
int     var_smtp_skip_4xx_greeting;
int     var_smtp_skip_5xx_greeting;
int     var_ign_mx_lookup_err;
int     var_skip_quit_resp;
char   *var_fallback_relay;
char   *var_bestmx_transp;
char   *var_error_rcpt;
int     var_smtp_always_ehlo;
int     var_smtp_never_ehlo;
char   *var_smtp_sasl_opts;
char   *var_smtp_sasl_passwd;
bool    var_smtp_sasl_enable;
char   *var_smtp_bind_addr;
bool    var_smtp_rand_addr;
bool    var_smtp_break_lines;
int     var_smtp_pix_thresh;
int     var_smtp_pix_delay;

 /*
  * Global variables. smtp_errno is set by the address lookup routines and by
  * the connection management routines.
  */
int     smtp_errno;

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(DELIVER_REQUEST *request)
{
    VSTRING *why;
    SMTP_STATE *state;
    int     result;

    if (msg_verbose)
	msg_info("deliver_message: from %s", request->sender);

    /*
     * Sanity checks. The smtp server is unprivileged and chrooted, so we can
     * afford to distribute the data censoring code, instead of having it all
     * in one place.
     */
    if (request->nexthop[0] == 0)
	msg_fatal("empty nexthop hostname");
    if (request->rcpt_list.len <= 0)
	msg_fatal("recipient count: %d", request->rcpt_list.len);

    /*
     * Initialize. Bundle all information about the delivery request, so that
     * we can produce understandable diagnostics when something goes wrong
     * many levels below. The alternative would be to make everything global.
     */
    why = vstring_alloc(100);
    state = smtp_state_alloc();
    state->request = request;
    state->src = request->fp;

    /*
     * Establish an SMTP session and deliver this message to all requested
     * recipients. At the end, notify the postmaster of any protocol errors.
     * Optionally deliver mail locally when this machine is the best mail
     * exchanger.
     */
    if ((state->session = smtp_connect(request->nexthop, why)) == 0) {
	if (smtp_errno == SMTP_OK) {
	    if (*var_bestmx_transp == 0)
		msg_panic("smtp_errno botch");
	    state->status = deliver_pass_all(MAIL_CLASS_PRIVATE,
					     var_bestmx_transp,
					     request);
	} else
	    smtp_site_fail(state, smtp_errno == SMTP_RETRY ? 450 : 550,
			   "%s", vstring_str(why));
    } else {
	debug_peer_check(state->session->host, state->session->addr);
	if (smtp_helo(state) == 0)
	    smtp_xfer(state);
	if (state->history != 0
	    && (state->error_mask & name_mask(VAR_NOTIFY_CLASSES,
				     mail_error_masks, var_notify_classes)))
	    smtp_chat_notify(state);
	smtp_session_free(state->session);
	debug_peer_restore();
    }

    /*
     * Clean up.
     */
    vstring_free(why);
    smtp_chat_reset(state);
    result = state->status;
    smtp_state_free(state);

    return (result);
}

/* smtp_service - perform service for client */

static void smtp_service(VSTREAM *client_stream, char *unused_service, char **argv)
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
     * dedicated to remote SMTP delivery service. What we see below is a
     * little protocol to (1) tell the queue manager that we are ready, (2)
     * read a request from the queue manager, and (3) report the completion
     * status of that request. All connection-management stuff is handled by
     * the common code in single_server.c.
     */
    if ((request = deliver_request_read(client_stream)) != 0) {
	status = deliver_message(request);
	deliver_request_done(client_stream, request, status);
    }
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{
    debug_peer_init();

    if (var_smtp_sasl_enable)
#ifdef USE_SASL_AUTH
	smtp_sasl_initialize();
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_SMTP_SASL_ENABLE);
#endif
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* pre_exit - pre-exit cleanup */

static void pre_exit(void)
{
#ifdef USE_SASL_AUTH
    if (var_smtp_sasl_enable)
	sasl_done();
#endif
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_FALLBACK_RELAY, DEF_FALLBACK_RELAY, &var_fallback_relay, 0, 0,
	VAR_BESTMX_TRANSP, DEF_BESTMX_TRANSP, &var_bestmx_transp, 0, 0,
	VAR_ERROR_RCPT, DEF_ERROR_RCPT, &var_error_rcpt, 1, 0,
	VAR_SMTP_SASL_PASSWD, DEF_SMTP_SASL_PASSWD, &var_smtp_sasl_passwd, 0, 0,
	VAR_SMTP_SASL_OPTS, DEF_SMTP_SASL_OPTS, &var_smtp_sasl_opts, 0, 0,
	VAR_SMTP_BIND_ADDR, DEF_SMTP_BIND_ADDR, &var_smtp_bind_addr, 0, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_SMTP_CONN_TMOUT, DEF_SMTP_CONN_TMOUT, &var_smtp_conn_tmout, 0, 0,
	VAR_SMTP_HELO_TMOUT, DEF_SMTP_HELO_TMOUT, &var_smtp_helo_tmout, 1, 0,
	VAR_SMTP_MAIL_TMOUT, DEF_SMTP_MAIL_TMOUT, &var_smtp_mail_tmout, 1, 0,
	VAR_SMTP_RCPT_TMOUT, DEF_SMTP_RCPT_TMOUT, &var_smtp_rcpt_tmout, 1, 0,
	VAR_SMTP_DATA0_TMOUT, DEF_SMTP_DATA0_TMOUT, &var_smtp_data0_tmout, 1, 0,
	VAR_SMTP_DATA1_TMOUT, DEF_SMTP_DATA1_TMOUT, &var_smtp_data1_tmout, 1, 0,
	VAR_SMTP_DATA2_TMOUT, DEF_SMTP_DATA2_TMOUT, &var_smtp_data2_tmout, 1, 0,
	VAR_SMTP_QUIT_TMOUT, DEF_SMTP_QUIT_TMOUT, &var_smtp_quit_tmout, 1, 0,
	VAR_SMTP_PIX_THRESH, DEF_SMTP_PIX_THRESH, &var_smtp_pix_thresh, 0, 0,
	VAR_SMTP_PIX_DELAY, DEF_SMTP_PIX_DELAY, &var_smtp_pix_delay, 1, 0,
	0,
    };
    static CONFIG_INT_TABLE int_table[] = {
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SMTP_SKIP_4XX, DEF_SMTP_SKIP_4XX, &var_smtp_skip_4xx_greeting,
	VAR_SMTP_SKIP_5XX, DEF_SMTP_SKIP_5XX, &var_smtp_skip_5xx_greeting,
	VAR_IGN_MX_LOOKUP_ERR, DEF_IGN_MX_LOOKUP_ERR, &var_ign_mx_lookup_err,
	VAR_SKIP_QUIT_RESP, DEF_SKIP_QUIT_RESP, &var_skip_quit_resp,
	VAR_SMTP_ALWAYS_EHLO, DEF_SMTP_ALWAYS_EHLO, &var_smtp_always_ehlo,
	VAR_SMTP_NEVER_EHLO, DEF_SMTP_NEVER_EHLO, &var_smtp_never_ehlo,
	VAR_SMTP_SASL_ENABLE, DEF_SMTP_SASL_ENABLE, &var_smtp_sasl_enable,
	VAR_SMTP_RAND_ADDR, DEF_SMTP_RAND_ADDR, &var_smtp_rand_addr,
	VAR_SMTP_BREAK_LINES, DEF_SMTP_BREAK_LINES, &var_smtp_break_lines,
	0,
    };

    single_server_main(argc, argv, smtp_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_EXIT, pre_exit,
		       0);
}
