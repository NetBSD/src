/*++
/* NAME
/*	lmtp 8
/* SUMMARY
/*	Postfix local delivery via LMTP
/* SYNOPSIS
/*	\fBlmtp\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The LMTP client processes message delivery requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, a domain or host to deliver to, and recipient information.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The LMTP client updates the queue file and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery problem reports are sent
/*	to the \fBbounce\fR(8) or \fBdefer\fR(8) daemon as appropriate.
/*
/*	The LMTP client connects to the destination specified in the message
/*	delivery request. The destination, usually specified in the Postfix
/*	\fBtransport\fR(5) table, has the form:
/* .IP \fBunix\fR:\fIpathname\fR
/*	Connect to the local UNIX-domain server that is bound to the specified
/*	\fIpathname\fR. If the process runs chrooted, an absolute pathname
/*	is interpreted relative to the changed root directory.
/* .IP "\fBinet\fR:\fIhost\fR, \fBinet\fB:\fIhost\fR:\fIport\fR (symbolic host)"
/* .IP "\fBinet\fR:[\fIaddr\fR], \fBinet\fR:[\fIaddr\fR]:\fIport\fR (numeric host)"
/*	Connect to the specified IPV4 TCP port on the specified local or
/*	remote host. If no port is specified, connect to the port defined as
/*	\fBlmtp\fR in \fBservices\fR(4).
/*	If no such service is found, the \fBlmtp_tcp_port\fR configuration
/*	parameter (default value of 24) will be used.
/*
/*	The LMTP client does not perform MX (mail exchanger) lookups since
/*	those are defined only for mail delivery via SMTP.
/* .PP
/*	If neither \fBunix:\fR nor \fBinet:\fR are specified, \fBinet:\fR
/*	is assumed.
/* SECURITY
/* .ad
/* .fi
/*	The LMTP client is moderately security-sensitive. It talks to LMTP
/*	servers and to DNS servers on the network. The LMTP client can be
/*	run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 1651 (SMTP service extensions)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 2033 (LMTP protocol)
/*	RFC 2197 (Pipelining)
/*	RFC 2554 (AUTH command)
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
/* .IP \fBdebug_peer_level\fR
/*	Verbose logging level increment for hosts that match a
/*	pattern in the \fBdebug_peer_list\fR parameter.
/* .IP \fBdebug_peer_list\fR
/*	List of domain or network patterns. When a remote host matches
/*	a pattern, increase the verbose logging level by the amount
/*	specified in the \fBdebug_peer_level\fR parameter.
/* .IP \fBerror_notice_recipient\fR
/*	Recipient of protocol/policy/resource/software error notices.
/* .IP \fBnotify_classes\fR
/*	When this parameter includes the \fBprotocol\fR class, send mail to the
/*	postmaster with transcripts of LMTP sessions with protocol errors.
/* .IP \fBlmtp_skip_quit_response\fR
/*	Do not wait for the server response after sending QUIT.
/* .IP \fBlmtp_tcp_port\fR
/*	The TCP port to be used when connecting to a LMTP server.  Used as
/*	backup if the \fBlmtp\fR service is not found in \fBservices\fR(4).
/* .SH "Authentication controls"
/* .IP \fBlmtp_enable_sasl_auth\fR
/*	Enable per-session authentication as per RFC 2554 (SASL).
/*	By default, Postfix is built without SASL support.
/* .IP \fBlmtp_sasl_password_maps\fR
/*	Lookup tables with per-host or domain \fIname\fR:\fIpassword\fR entries.
/*	No entry for a host means no attempt to authenticate.
/* .IP \fBlmtp_sasl_security_options\fR
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
/* .IP \fBlmtp_cache_connection\fR
/*	Should we cache the connection to the LMTP server? The effectiveness
/*	of cached connections will be determined by the number of LMTP servers
/*	in use, and the concurrency limit specified for the LMTP client.
/*	Cached connections are closed under any of the following conditions:
/* .RS
/* .IP \(bu
/*	The LMTP client idle time limit is reached. This limit is specified
/*	with the Postfix \fBmax_idle\fR configuration parameter.
/* .IP \(bu
/*	A delivery request specifies a different destination than the one
/*	currently cached.
/* .IP \(bu
/*	The per-process limit on the number of delivery requests is reached.
/*	This limit is specified with the Postfix \fBmax_use\fR configuration
/*	parameter.
/* .IP \(bu
/*	Upon the onset of another delivery request, the LMTP server associated
/*	with the current session does not respond to the \fBRSET\fR command.
/* .RE
/* .IP \fItransport_\fBdestination_concurrency_limit\fR
/*	Limit the number of parallel deliveries to the same destination
/*	via this mail delivery transport. \fItransport\fR is the name
/*	of the service as specified in the \fBmaster.cf\fR file.
/*	The default limit is taken from the
/*	\fBdefault_destination_concurrency_limit\fR parameter.
/* .IP \fItransport_\fBdestination_recipient_limit\fR
/*	Limit the number of recipients per message delivery via this mail
/*	delivery transport. \fItransport\fR is the name
/*	of the service as specified in the \fBmaster.cf\fR file.
/*	The default limit is taken from the
/*	\fBdefault_destination_recipient_limit\fR parameter.
/*
/*	This parameter becomes significant if the LMTP client is used
/*	for local delivery.  Some LMTP servers can optimize delivery of
/*	the same message to multiple recipients. The default limit for
/*	local mail delivery is 1.
/*
/*	Setting this parameter to 0 will lead to an unbounded number of
/*	recipients per delivery.  However, this could be risky since it may
/*	make the machine vulnerable to running out of resources if messages
/*	are encountered with an inordinate number of recipients.  Exercise
/*	care when setting this parameter.
/* .SH "Timeout controls"
/* .ad
/* .fi
/* .PP
/*	The default time unit is seconds; an explicit time unit can
/*	be specified by appending a one-letter suffix to the value:
/*	s (seconds), m (minutes), h (hours), d (days) or w (weeks).
/* .IP \fBlmtp_connect_timeout\fR
/*	Timeout for opening a connection to the LMTP server.
/*	If no connection can be made within the deadline, the message
/*	is deferred.
/* .IP \fBlmtp_lhlo_timeout\fR
/*	Timeout for sending the \fBLHLO\fR command, and for
/*	receiving the server response.
/* .IP \fBlmtp_mail_timeout\fR
/*	Timeout for sending the \fBMAIL FROM\fR command, and for
/*	receiving the server response.
/* .IP \fBlmtp_rcpt_timeout\fR
/*	Timeout for sending the \fBRCPT TO\fR command, and for
/*	receiving the server response.
/* .IP \fBlmtp_data_init_timeout\fR
/*	Timeout for sending the \fBDATA\fR command, and for
/*	receiving the server response.
/* .IP \fBlmtp_data_xfer_timeout\fR
/*	Timeout for sending the message content.
/* .IP \fBlmtp_data_done_timeout\fR
/*	Timeout for sending the "\fB.\fR" command, and for
/*	receiving the server response. When no response is received, a
/*	warning is logged that the mail may be delivered multiple times.
/* .IP \fBlmtp_rset_timeout\fR
/*	Timeout for sending the \fBRSET\fR command, and for
/*	receiving the server response.
/* .IP \fBlmtp_quit_timeout\fR
/*	Timeout for sending the \fBQUIT\fR command, and for
/*	receiving the server response.
/* SEE ALSO
/*	bounce(8) non-delivery status reports
/*	local(8) local mail delivery
/*	master(8) process manager
/*	qmgr(8) queue manager
/*	services(4) Internet services and aliases
/*	spawn(8) auxiliary command spawner
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
/*
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dict.h>
#include <pwd.h>
#include <grp.h>

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <mymalloc.h>
#include <name_mask.h>
#include <split_at.h>

/* Global library. */

#include <deliver_request.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <debug_peer.h>
#include <mail_error.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "lmtp.h"
#include "lmtp_sasl.h"

 /*
  * Tunable parameters. These have compiled-in defaults that can be overruled
  * by settings in the global Postfix configuration file.
  */
int     var_lmtp_tcp_port;
int     var_lmtp_conn_tmout;
int     var_lmtp_rset_tmout;
int     var_lmtp_lhlo_tmout;
int     var_lmtp_mail_tmout;
int     var_lmtp_rcpt_tmout;
int     var_lmtp_data0_tmout;
int     var_lmtp_data1_tmout;
int     var_lmtp_data2_tmout;
int     var_lmtp_quit_tmout;
char   *var_debug_peer_list;
int     var_debug_peer_level;
int     var_lmtp_cache_conn;
int     var_lmtp_skip_quit_resp;
char   *var_notify_classes;
char   *var_error_rcpt;
char   *var_lmtp_sasl_opts;
char   *var_lmtp_sasl_passwd;
bool    var_lmtp_sasl_enable;

 /*
  * Global variables.
  * 
  * lmtp_errno is set by the address lookup routines and by the connection
  * management routines.
  * 
  * state is global for the connection caching to work.
  */
int     lmtp_errno;
static LMTP_STATE *state = 0;

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(DELIVER_REQUEST *request, char **unused_argv)
{
    char   *myname = "deliver_message";
    VSTRING *why;
    int     result;

    if (msg_verbose)
	msg_info("%s: from %s", myname, request->sender);

    /*
     * Sanity checks.
     */
    if (request->rcpt_list.len <= 0)
	msg_fatal("%s: recipient count: %d", myname, request->rcpt_list.len);

    /*
     * Initialize. Bundle all information about the delivery request, so that
     * we can produce understandable diagnostics when something goes wrong
     * many levels below. The alternative would be to make everything global.
     * 
     * Note: `state' was made global (to this file) so that we can cache
     * connections and so that we can close a cached connection via the
     * MAIL_SERVER_EXIT function (cleanup). The alloc for `state' is
     * performed in the MAIL_SERVER_PRE_INIT function (pre_init).
     * 
     */
    why = vstring_alloc(100);
    state->request = request;
    state->src = request->fp;

    /*
     * See if we can reuse an existing connection.
     */
    if (state->session != 0) {

	/*
	 * Disconnect if we're going to a different destination. Discard
	 * transcript and status information for sending QUIT.
	 */
	if (strcasecmp(state->session->dest, request->nexthop) != 0) {
	    lmtp_quit(state);
	    lmtp_chat_reset(state);
	    state->session = lmtp_session_free(state->session);
	}

	/*
	 * Disconnect if RSET can't be sent over an existing connection.
	 * Discard transcript and status information for sending RSET.
	 */
	else if (lmtp_rset(state) != 0) {
	    lmtp_chat_reset(state);
	    state->session = lmtp_session_free(state->session);
	}

	/*
	 * Ready to go with another load. The reuse counter is logged for
	 * statistical analysis purposes. Given the Postfix architecture,
	 * connection cacheing makes sense only for dedicated transports.
	 * Logging the reuse count can help to convince people.
	 */
	else {
	    ++state->reuse;
	    if (msg_verbose)
		msg_info("%s: reusing (count %d) session with: %s",
			 myname, state->reuse, state->session->host);
	}
    }

    /*
     * See if we need to establish an LMTP connection.
     */
    if (state->session == 0) {

	/*
	 * Bounce or defer the recipients if no connection can be made.
	 */
	if ((state->session = lmtp_connect(request->nexthop, why)) == 0) {
	    lmtp_site_fail(state, lmtp_errno == LMTP_RETRY ? 450 : 550,
			   "%s", vstring_str(why));
	}

	/*
	 * Bounce or defer the recipients if the LMTP handshake fails.
	 */
	else if (lmtp_lhlo(state) != 0) {
	    state->session = lmtp_session_free(state->session);
	}

	/*
	 * Congratulations. We just established a new LMTP connection.
	 */
	else
	    state->reuse = 0;
    }

    /*
     * If a session exists, deliver this message to all requested recipients.
     */
    if (state->session != 0)
	lmtp_xfer(state);

    /*
     * Optionally, notify the postmaster of problems with establishing a
     * session or with delivering mail.
     */
    if (state->history != 0
     && (state->error_mask & name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				       var_notify_classes)))
	lmtp_chat_notify(state);

    /*
     * Disconnect if we're not cacheing connections. The pipelined protocol
     * state machine knows if it should have sent a QUIT command.
     */
    if (state->session != 0 && !var_lmtp_cache_conn)
	state->session = lmtp_session_free(state->session);

    /*
     * Clean up.
     */
    vstring_free(why);
    result = state->status;
    lmtp_chat_reset(state);

    return (result);
}

/* lmtp_service - perform service for client */

static void lmtp_service(VSTREAM *client_stream, char *unused_service, char **argv)
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
     * dedicated to remote LMTP delivery service. What we see below is a
     * little protocol to (1) tell the queue manager that we are ready, (2)
     * read a request from the queue manager, and (3) report the completion
     * status of that request. All connection-management stuff is handled by
     * the common code in single_server.c.
     */
    if ((request = deliver_request_read(client_stream)) != 0) {
	status = deliver_message(request, argv);
	deliver_request_done(client_stream, request, status);
    }
}

/* post_init - post-jail initialization */

static void post_init(char *unused_name, char **unused_argv)
{
    state = lmtp_state_alloc();
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{
    debug_peer_init();
    if (var_lmtp_sasl_enable)
#ifdef USE_SASL_AUTH
	lmtp_sasl_initialize();
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_LMTP_SASL_ENABLE);
#endif
}

/* cleanup - close any open connections, etc. */

static void cleanup(void)
{
    if (state->session != 0) {
	lmtp_quit(state);
	lmtp_chat_reset(state);
	state->session = lmtp_session_free(state->session);
	if (msg_verbose)
	    msg_info("cleanup: just closed down session");
    }
    lmtp_state_free(state);
#ifdef USE_SASL_AUTH
    if (var_lmtp_sasl_enable)
	sasl_done();
#endif
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	cleanup();
	exit(0);
    }
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_DEBUG_PEER_LIST, DEF_DEBUG_PEER_LIST, &var_debug_peer_list, 0, 0,
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_ERROR_RCPT, DEF_ERROR_RCPT, &var_error_rcpt, 1, 0,
	VAR_LMTP_SASL_PASSWD, DEF_LMTP_SASL_PASSWD, &var_lmtp_sasl_passwd, 0, 0,
	VAR_LMTP_SASL_OPTS, DEF_LMTP_SASL_OPTS, &var_lmtp_sasl_opts, 0, 0,
	0,
    };
    static CONFIG_INT_TABLE int_table[] = {
	VAR_LMTP_TCP_PORT, DEF_LMTP_TCP_PORT, &var_lmtp_tcp_port, 0, 0,
	VAR_DEBUG_PEER_LEVEL, DEF_DEBUG_PEER_LEVEL, &var_debug_peer_level, 1, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_LMTP_CONN_TMOUT, DEF_LMTP_CONN_TMOUT, &var_lmtp_conn_tmout, 0, 0,
	VAR_LMTP_RSET_TMOUT, DEF_LMTP_RSET_TMOUT, &var_lmtp_rset_tmout, 1, 0,
	VAR_LMTP_LHLO_TMOUT, DEF_LMTP_LHLO_TMOUT, &var_lmtp_lhlo_tmout, 1, 0,
	VAR_LMTP_MAIL_TMOUT, DEF_LMTP_MAIL_TMOUT, &var_lmtp_mail_tmout, 1, 0,
	VAR_LMTP_RCPT_TMOUT, DEF_LMTP_RCPT_TMOUT, &var_lmtp_rcpt_tmout, 1, 0,
	VAR_LMTP_DATA0_TMOUT, DEF_LMTP_DATA0_TMOUT, &var_lmtp_data0_tmout, 1, 0,
	VAR_LMTP_DATA1_TMOUT, DEF_LMTP_DATA1_TMOUT, &var_lmtp_data1_tmout, 1, 0,
	VAR_LMTP_DATA2_TMOUT, DEF_LMTP_DATA2_TMOUT, &var_lmtp_data2_tmout, 1, 0,
	VAR_LMTP_QUIT_TMOUT, DEF_LMTP_QUIT_TMOUT, &var_lmtp_quit_tmout, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_LMTP_CACHE_CONN, DEF_LMTP_CACHE_CONN, &var_lmtp_cache_conn,
	VAR_LMTP_SKIP_QUIT_RESP, DEF_LMTP_SKIP_QUIT_RESP, &var_lmtp_skip_quit_resp,
	VAR_LMTP_SASL_ENABLE, DEF_LMTP_SASL_ENABLE, &var_lmtp_sasl_enable,
	0,
    };

    single_server_main(argc, argv, lmtp_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       MAIL_SERVER_POST_INIT, post_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_EXIT, cleanup,
		       0);
}
