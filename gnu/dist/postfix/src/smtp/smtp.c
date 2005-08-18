/*	$NetBSD: smtp.c,v 1.1.1.8 2005/08/18 21:08:49 rpaulo Exp $	*/

/*++
/* NAME
/*	smtp 8
/* SUMMARY
/*	Postfix SMTP client
/* SYNOPSIS
/*	\fBsmtp\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix SMTP client processes message delivery requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, a domain or host to deliver to, and recipient information.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The SMTP client updates the queue file and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery status reports are sent
/*	to the \fBbounce\fR(8), \fBdefer\fR(8) or \fBtrace\fR(8) daemon as
/*	appropriate.
/*
/*	The SMTP client looks up a list of mail exchanger addresses for
/*	the destination host, sorts the list by preference, and connects
/*	to each listed address until it finds a server that responds.
/*
/*	When a server is not reachable, or when mail delivery fails due
/*	to a recoverable error condition, the SMTP client will try to
/*	deliver the mail to an alternate host.
/*
/*	After a successful mail transaction, a connection may be saved
/*	to the \fBscache\fR(8) connection cache server, so that it
/*	may be used by any SMTP client for a subsequent transaction.
/*
/*	By default, connection caching is enabled temporarily for
/*	destinations that have a high volume of mail in the active
/*	queue. Session caching can be enabled permanently for
/*	specific destinations.
/* SECURITY
/* .ad
/* .fi
/*	The SMTP client is moderately security-sensitive. It talks to SMTP
/*	servers and to DNS servers on the network. The SMTP client can be
/*	run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 1651 (SMTP service extensions)
/*	RFC 1652 (8bit-MIME transport)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 2045 (MIME: Format of Internet Message Bodies)
/*	RFC 2046 (MIME: Media Types)
/*	RFC 2554 (AUTH command)
/*	RFC 2821 (SMTP protocol)
/*	RFC 2920 (SMTP Pipelining)
/*	RFC 3207 (STARTTLS command)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*	Corrupted message files are marked so that the queue manager can
/*	move them to the \fBcorrupt\fR queue for further inspection.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems, and of
/*	other trouble.
/* BUGS
/*	SMTP connection caching does not work with TLS. The necessary
/*	support for TLS object passivation and re-activation does not
/*	exist without closing the session, which defeats the purpose.
/*
/*	SMTP connection caching assumes that SASL credentials are valid for
/*	all destinations that map onto the same IP address and TCP port.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as \fBsmtp\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBignore_mx_lookup_error (no)\fR"
/*	Ignore DNS MX lookups that produce no response.
/* .IP "\fBsmtp_always_send_ehlo (yes)\fR"
/*	Always send EHLO at the start of an SMTP session.
/* .IP "\fBsmtp_never_send_ehlo (no)\fR"
/*	Never send EHLO at the start of an SMTP session.
/* .IP "\fBsmtp_defer_if_no_mx_address_found (no)\fR"
/*	Defer mail delivery when no MX record resolves to an IP address.
/* .IP "\fBsmtp_line_length_limit (990)\fR"
/*	The maximal length of message header and body lines that Postfix
/*	will send via SMTP.
/* .IP "\fBsmtp_pix_workaround_delay_time (10s)\fR"
/*	How long the Postfix SMTP client pauses before sending
/*	".<CR><LF>" in order to work around the PIX firewall
/*	"<CR><LF>.<CR><LF>" bug.
/* .IP "\fBsmtp_pix_workaround_threshold_time (500s)\fR"
/*	How long a message must be queued before the PIX firewall
/*	"<CR><LF>.<CR><LF>" bug workaround is turned
/*	on.
/* .IP "\fBsmtp_quote_rfc821_envelope (yes)\fR"
/*	Quote addresses in SMTP MAIL FROM and RCPT TO commands as required
/*	by RFC 821.
/* .IP "\fBsmtp_skip_5xx_greeting (yes)\fR"
/*	Skip SMTP servers that greet with a 5XX status code (go away, do
/*	not try again later).
/* .IP "\fBsmtp_skip_quit_response (yes)\fR"
/*	Do not wait for the response to the SMTP QUIT command.
/* .PP
/*	Available in Postfix version 2.0 and earlier:
/* .IP "\fBsmtp_skip_4xx_greeting (yes)\fR"
/*	Skip SMTP servers that greet with a 4XX status code (go away, try
/*	again later).
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_discard_ehlo_keyword_address_maps (empty)\fR"
/*	Lookup tables, indexed by the remote SMTP server address, with
/*	case insensitive lists of EHLO keywords (pipelining, starttls,
/*	auth, etc.) that the SMTP client will ignore in the EHLO response
/*	from a remote SMTP server.
/* .IP "\fBsmtp_discard_ehlo_keywords (empty)\fR"
/*	A case insensitive list of EHLO keywords (pipelining, starttls,
/*	auth, etc.) that the SMTP client will ignore in the EHLO response
/*	from a remote SMTP server.
/* .IP "\fBsmtp_generic_maps (empty)\fR"
/*	Optional lookup tables that perform address rewriting in the
/*	SMTP client, typically to transform a locally valid address into
/*	a globally valid address when sending mail across the Internet.
/* MIME PROCESSING CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBdisable_mime_output_conversion (no)\fR"
/*	Disable the conversion of 8BITMIME format to 7BIT format.
/* .IP "\fBmime_boundary_length_limit (2048)\fR"
/*	The maximal length of MIME multipart boundary strings.
/* .IP "\fBmime_nesting_limit (100)\fR"
/*	The maximal recursion level that the MIME processor will handle.
/* EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtp_send_xforward_command (no)\fR"
/*	Send the non-standard XFORWARD command when the Postfix SMTP server EHLO
/*	response announces XFORWARD support.
/* SASL AUTHENTICATION CONTROLS
/* .ad
/* .fi
/* .IP "\fBsmtp_sasl_auth_enable (no)\fR"
/*	Enable SASL authentication in the Postfix SMTP client.
/* .IP "\fBsmtp_sasl_password_maps (empty)\fR"
/*	Optional SMTP client lookup tables with one username:password entry
/*	per remote hostname or domain.
/* .IP "\fBsmtp_sasl_security_options (noplaintext, noanonymous)\fR"
/*	What authentication mechanisms the Postfix SMTP client is allowed
/*	to use.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_sasl_mechanism_filter (empty)\fR"
/*	If non-empty, a Postfix SMTP client filter for the remote SMTP
/*	server's list of offered SASL mechanisms.
/* STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/*	Detailed information about STARTTLS configuration may be found
/*	in the TLS_README document.
/* .IP "\fBsmtp_use_tls (no)\fR"
/*	Opportunistic mode: use TLS when a remote SMTP server announces
/*	STARTTLS support, otherwise send the mail in the clear.
/* .IP "\fBsmtp_enforce_tls (no)\fR"
/*	Enforcement mode: require that remote SMTP servers use TLS
/*	encryption, and never send mail in the clear.
/* .IP "\fBsmtp_sasl_tls_security_options ($smtp_sasl_security_options)\fR"
/*	The SASL authentication security options that the Postfix SMTP
/*	client uses for TLS encrypted SMTP sessions.
/* .IP "\fBsmtp_starttls_timeout (300s)\fR"
/*	Time limit for Postfix SMTP client write and read operations
/*	during TLS startup and shutdown handshake procedures.
/* .IP "\fBsmtp_tls_CAfile (empty)\fR"
/*	The file with the certificate of the certification authority
/*	(CA) that issued the Postfix SMTP client certificate.
/* .IP "\fBsmtp_tls_CApath (empty)\fR"
/*	Directory with PEM format certificate authority certificates
/*	that the Postfix SMTP client uses to verify a remote SMTP server
/*	certificate.
/* .IP "\fBsmtp_tls_cert_file (empty)\fR"
/*	File with the Postfix SMTP client RSA certificate in PEM format.
/* .IP "\fBsmtp_tls_cipherlist (empty)\fR"
/*	Controls the Postfix SMTP client TLS cipher selection scheme.
/* .IP "\fBsmtp_tls_dcert_file (empty)\fR"
/*	File with the Postfix SMTP client DSA certificate in PEM format.
/* .IP "\fBsmtp_tls_dkey_file ($smtp_tls_dcert_file)\fR"
/*	File with the Postfix SMTP client DSA private key in PEM format.
/* .IP "\fBsmtp_tls_enforce_peername (yes)\fR"
/*	When TLS encryption is enforced, require that the remote SMTP
/*	server hostname matches the information in the remote SMTP server
/*	certificate.
/* .IP "\fBsmtp_tls_key_file ($smtp_tls_cert_file)\fR"
/*	File with the Postfix SMTP client RSA private key in PEM format.
/* .IP "\fBsmtp_tls_loglevel (0)\fR"
/*	Enable additional Postfix SMTP client logging of TLS activity.
/* .IP "\fBsmtp_tls_note_starttls_offer (no)\fR"
/*	Log the hostname of a remote SMTP server that offers STARTTLS,
/*	when TLS is not already enabled for that server.
/* .IP "\fBsmtp_tls_per_site (empty)\fR"
/*	Optional lookup tables with the Postfix SMTP client TLS usage
/*	policy by next-hop domain name and by remote SMTP server hostname.
/* .IP "\fBsmtp_tls_scert_verifydepth (5)\fR"
/*	The verification depth for remote SMTP server certificates.
/* .IP "\fBsmtp_tls_session_cache_database (empty)\fR"
/*	Name of the file containing the optional Postfix SMTP client
/*	TLS session cache.
/* .IP "\fBsmtp_tls_session_cache_timeout (3600s)\fR"
/*	The expiration time of Postfix SMTP client TLS session cache
/*	information.
/* .IP "\fBtls_daemon_random_bytes (32)\fR"
/*	The number of pseudo-random bytes that an \fBsmtp\fR(8) or \fBsmtpd\fR(8)
/*	process requests from the \fBtlsmgr\fR(8) server in order to seed its
/*	internal pseudo random number generator (PRNG).
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBsmtp_destination_concurrency_limit ($default_destination_concurrency_limit)\fR"
/*	The maximal number of parallel deliveries to the same destination
/*	via the smtp message delivery transport.
/* .IP "\fBsmtp_destination_recipient_limit ($default_destination_recipient_limit)\fR"
/*	The maximal number of recipients per delivery via the smtp
/*	message delivery transport.
/* .IP "\fBsmtp_connect_timeout (30s)\fR"
/*	The SMTP client time limit for completing a TCP connection, or
/*	zero (use the operating system built-in time limit).
/* .IP "\fBsmtp_helo_timeout (300s)\fR"
/*	The SMTP client time limit for sending the HELO or EHLO command,
/*	and for receiving the initial server response.
/* .IP "\fBsmtp_xforward_timeout (300s)\fR"
/*	The SMTP client time limit for sending the XFORWARD command, and
/*	for receiving the server response.
/* .IP "\fBsmtp_mail_timeout (300s)\fR"
/*	The SMTP client time limit for sending the MAIL FROM command, and
/*	for receiving the server response.
/* .IP "\fBsmtp_rcpt_timeout (300s)\fR"
/*	The SMTP client time limit for sending the SMTP RCPT TO command, and
/*	for receiving the server response.
/* .IP "\fBsmtp_data_init_timeout (120s)\fR"
/*	The SMTP client time limit for sending the SMTP DATA command, and for
/*	receiving the server response.
/* .IP "\fBsmtp_data_xfer_timeout (180s)\fR"
/*	The SMTP client time limit for sending the SMTP message content.
/* .IP "\fBsmtp_data_done_timeout (600s)\fR"
/*	The SMTP client time limit for sending the SMTP ".", and for receiving
/*	the server response.
/* .IP "\fBsmtp_quit_timeout (300s)\fR"
/*	The SMTP client time limit for sending the QUIT command, and for
/*	receiving the server response.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtp_mx_address_limit (0)\fR"
/*	The maximal number of MX (mail exchanger) IP addresses that can
/*	result from mail exchanger lookups, or zero (no limit).
/* .IP "\fBsmtp_mx_session_limit (2)\fR"
/*	The maximal number of SMTP sessions per delivery request before
/*	giving up or delivering to a fall-back relay host, or zero (no
/*	limit).
/* .IP "\fBsmtp_rset_timeout (20s)\fR"
/*	The SMTP client time limit for sending the RSET command, and
/*	for receiving the server response.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_connection_cache_destinations (empty)\fR"
/*	Permanently enable SMTP connection caching for the specified
/*	destinations.
/* .IP "\fBsmtp_connection_cache_on_demand (yes)\fR"
/*	Temporarily enable SMTP connection caching while a destination
/*	has a high volume of mail in the active queue.
/* .IP "\fBsmtp_connection_cache_reuse_limit (10)\fR"
/*	When SMTP connection caching is enabled, the number of times that
/*	an SMTP session is reused before it is closed.
/* .IP "\fBsmtp_connection_cache_time_limit (2s)\fR"
/*	When SMTP connection caching is enabled, the amount of time that
/*	an unused SMTP client socket is kept open before it is closed.
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
/* .IP "\fBerror_notice_recipient (postmaster)\fR"
/*	The recipient of postmaster notifications about mail delivery
/*	problems that are caused by policy, resource, software or protocol
/*	errors.
/* .IP "\fBnotify_classes (resource, software)\fR"
/*	The list of error classes that are reported to the postmaster.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBbest_mx_transport (empty)\fR"
/*	Where the Postfix SMTP client should deliver mail when it detects
/*	a "mail loops back to myself" error condition.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBdisable_dns_lookups (no)\fR"
/*	Disable DNS lookups in the Postfix SMTP and LMTP clients.
/* .IP "\fBfallback_relay (empty)\fR"
/*	Optional list of relay hosts for SMTP destinations that can't be
/*	found or that are unreachable.
/* .IP "\fBinet_interfaces (all)\fR"
/*	The network interface addresses that this mail system receives
/*	mail on.
/* .IP "\fBinet_protocols (ipv4)\fR"
/*	The Internet protocols Postfix will attempt to use when making
/*	or accepting connections.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process
/*	waits for the next service request before exiting.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of connection requests before a Postfix daemon
/*	process terminates.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBproxy_interfaces (empty)\fR"
/*	The network interface addresses that this mail system receives mail
/*	on by way of a proxy or network address translation unit.
/* .IP "\fBsmtp_bind_address (empty)\fR"
/*	An optional numerical network address that the SMTP client should
/*	bind to when making an IPv4 connection.
/* .IP "\fBsmtp_bind_address6 (empty)\fR"
/*	An optional numerical network address that the SMTP client should
/*	bind to when making an IPv6 connection.
/* .IP "\fBsmtp_helo_name ($myhostname)\fR"
/*	The hostname to send in the SMTP EHLO or HELO command.
/* .IP "\fBsmtp_host_lookup (dns)\fR"
/*	What mechanisms when the SMTP client uses to look up a host's IP
/*	address.
/* .IP "\fBsmtp_randomize_addresses (yes)\fR"
/*	Randomize the order of equal-preference MX host addresses.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	qmgr(8), queue manager
/*	bounce(8), delivery status reports
/*	scache(8), connection cache server
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	tlsmgr(8), TLS session and PRNG management
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	SASL_README, Postfix SASL howto
/*	TLS_README, Postfix STARTTLS howto
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
/*	Command pipelining in cooperation with:
/*	Jon Ribbens
/*	Oaktree Internet Solutions Ltd.,
/*	Internet House,
/*	Canal Basin,
/*	Coventry,
/*	CV1 4LY, United Kingdom.
/*
/*	Connection caching in cooperation with:
/*	Victor Duchovni
/*	Morgan Stanley
/*
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
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
#include <flush_clnt.h>
#include <scache.h>
#include <string_list.h>
#include <maps.h>
#include <ext_prop.h>

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
int     var_smtp_xfwd_tmout;
int     var_smtp_mail_tmout;
int     var_smtp_rcpt_tmout;
int     var_smtp_data0_tmout;
int     var_smtp_data1_tmout;
int     var_smtp_data2_tmout;
int     var_smtp_rset_tmout;
int     var_smtp_quit_tmout;
char   *var_inet_interfaces;
char   *var_notify_classes;
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
char   *var_smtp_sasl_mechs;
char   *var_smtp_bind_addr;
char   *var_smtp_bind_addr6;
bool    var_smtp_rand_addr;
int     var_smtp_pix_thresh;
int     var_smtp_pix_delay;
int     var_smtp_line_limit;
char   *var_smtp_helo_name;
char   *var_smtp_host_lookup;
bool    var_smtp_quote_821_env;
bool    var_smtp_defer_mxaddr;
bool    var_smtp_send_xforward;
int     var_smtp_mxaddr_limit;
int     var_smtp_mxsess_limit;
int     var_smtp_cache_conn;
int     var_smtp_reuse_limit;
char   *var_smtp_cache_dest;
char   *var_scache_service;
bool    var_smtp_cache_demand;
char   *var_smtp_ehlo_dis_words;
char   *var_smtp_ehlo_dis_maps;

bool    var_smtp_use_tls;
bool    var_smtp_enforce_tls;
char   *var_smtp_tls_per_site;
#ifdef USE_TLS
int     var_smtp_starttls_tmout;
char   *var_smtp_sasl_tls_opts;
bool    var_smtp_tls_enforce_peername;
int     var_smtp_tls_scert_vd;
bool    var_smtp_tls_note_starttls_offer;
#endif

char   *var_smtp_generic_maps;
char   *var_prop_extension;

 /*
  * Global variables. smtp_errno is set by the address lookup routines and by
  * the connection management routines.
  */
int     smtp_errno;
int     smtp_host_lookup_mask;
STRING_LIST *smtp_cache_dest;
SCACHE *smtp_scache;
MAPS   *smtp_ehlo_dis_maps;
MAPS   *smtp_generic_maps;
int     smtp_ext_prop_mask;

#ifdef USE_TLS

 /*
  * OpenSSL client state.
  */
SSL_CTX *smtp_tls_ctx;

#endif

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(const char *service, DELIVER_REQUEST *request)
{
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
    state = smtp_state_alloc();
    state->request = request;
    state->src = request->fp;
    state->service = service;
    SMTP_RCPT_INIT(state);

    /*
     * Establish an SMTP session and deliver this message to all requested
     * recipients. At the end, notify the postmaster of any protocol errors.
     * Optionally deliver mail locally when this machine is the best mail
     * exchanger.
     */
    result = smtp_connect(state);

    /*
     * Clean up.
     */
    smtp_state_free(state);

    return (result);
}

/* smtp_service - perform service for client */

static void smtp_service(VSTREAM *client_stream, char *service, char **argv)
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
	status = deliver_message(service, request);
	deliver_request_done(client_stream, request, status);
    }
}

/* post_init - post-jail initialization */

static void post_init(char *unused_name, char **unused_argv)
{
    static NAME_MASK lookup_masks[] = {
	SMTP_HOST_LOOKUP_DNS, SMTP_HOST_FLAG_DNS,
	SMTP_HOST_LOOKUP_NATIVE, SMTP_HOST_FLAG_NATIVE,
	0,
    };

    /*
     * Select hostname lookup mechanisms.
     */
    if (var_disable_dns)
	smtp_host_lookup_mask = SMTP_HOST_FLAG_NATIVE;
    else
	smtp_host_lookup_mask = name_mask(VAR_SMTP_HOST_LOOKUP, lookup_masks,
					  var_smtp_host_lookup);
    if (msg_verbose)
	msg_info("host name lookup methods: %s",
		 str_name_mask(VAR_SMTP_HOST_LOOKUP, lookup_masks,
			       smtp_host_lookup_mask));

    /*
     * Session cache instance.
     */
    if (*var_smtp_cache_dest || var_smtp_cache_demand)
#if 0
	smtp_scache = scache_multi_create();
#else
	smtp_scache = scache_clnt_create(var_scache_service,
					 var_ipc_idle_limit,
					 var_ipc_ttl_limit);
#endif
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{

    /*
     * Turn on per-peer debugging.
     */
    debug_peer_init();

    /*
     * SASL initialization.
     */
    if (var_smtp_sasl_enable)
#ifdef USE_SASL_AUTH
	smtp_sasl_initialize();
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_SMTP_SASL_ENABLE);
#endif

    /*
     * Initialize the TLS data before entering the chroot jail
     */
    if (var_smtp_use_tls || var_smtp_enforce_tls || var_smtp_tls_per_site[0]) {
#ifdef USE_TLS
	smtp_tls_ctx = tls_client_init(var_smtp_tls_scert_vd);
	smtp_tls_list_init();
#else
	msg_warn("TLS has been selected, but TLS support is not compiled in");
#endif
    }

    /*
     * Flush client.
     */
    flush_init();

    /*
     * Session cache domain list.
     */
    if (*var_smtp_cache_dest)
	smtp_cache_dest = string_list_init(MATCH_FLAG_NONE, var_smtp_cache_dest);

    /*
     * EHLO keyword filter.
     */
    if (*var_smtp_ehlo_dis_maps)
	smtp_ehlo_dis_maps = maps_create(VAR_SMTPD_EHLO_DIS_MAPS,
					 var_smtp_ehlo_dis_maps,
					 DICT_FLAG_LOCK);

    /*
     * Generic maps.
     */
    if (*var_prop_extension)
	smtp_ext_prop_mask =
	    ext_prop_mask(VAR_PROP_EXTENSION, var_prop_extension);
    if (*var_smtp_generic_maps)
	smtp_generic_maps =
	    maps_create(VAR_SMTP_GENERIC_MAPS, var_smtp_generic_maps,
			DICT_FLAG_LOCK);
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
#ifdef USE_TLS
	VAR_SMTP_SASL_TLS_OPTS, DEF_SMTP_SASL_TLS_OPTS, &var_smtp_sasl_tls_opts, 0, 0,
#endif
	VAR_SMTP_SASL_MECHS, DEF_SMTP_SASL_MECHS, &var_smtp_sasl_mechs, 0, 0,
	VAR_SMTP_BIND_ADDR, DEF_SMTP_BIND_ADDR, &var_smtp_bind_addr, 0, 0,
	VAR_SMTP_BIND_ADDR6, DEF_SMTP_BIND_ADDR6, &var_smtp_bind_addr6, 0, 0,
	VAR_SMTP_HELO_NAME, DEF_SMTP_HELO_NAME, &var_smtp_helo_name, 1, 0,
	VAR_SMTP_HOST_LOOKUP, DEF_SMTP_HOST_LOOKUP, &var_smtp_host_lookup, 1, 0,
	VAR_SMTP_CACHE_DEST, DEF_SMTP_CACHE_DEST, &var_smtp_cache_dest, 0, 0,
	VAR_SCACHE_SERVICE, DEF_SCACHE_SERVICE, &var_scache_service, 1, 0,
	VAR_SMTP_EHLO_DIS_WORDS, DEF_SMTP_EHLO_DIS_WORDS, &var_smtp_ehlo_dis_words, 0, 0,
	VAR_SMTP_EHLO_DIS_MAPS, DEF_SMTP_EHLO_DIS_MAPS, &var_smtp_ehlo_dis_maps, 0, 0,
	VAR_SMTP_TLS_PER_SITE, DEF_SMTP_TLS_PER_SITE, &var_smtp_tls_per_site, 0, 0,
	VAR_PROP_EXTENSION, DEF_PROP_EXTENSION, &var_prop_extension, 0, 0,
	VAR_SMTP_GENERIC_MAPS, DEF_SMTP_GENERIC_MAPS, &var_smtp_generic_maps, 0, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_SMTP_CONN_TMOUT, DEF_SMTP_CONN_TMOUT, &var_smtp_conn_tmout, 0, 0,
	VAR_SMTP_HELO_TMOUT, DEF_SMTP_HELO_TMOUT, &var_smtp_helo_tmout, 1, 0,
	VAR_SMTP_XFWD_TMOUT, DEF_SMTP_XFWD_TMOUT, &var_smtp_xfwd_tmout, 1, 0,
	VAR_SMTP_MAIL_TMOUT, DEF_SMTP_MAIL_TMOUT, &var_smtp_mail_tmout, 1, 0,
	VAR_SMTP_RCPT_TMOUT, DEF_SMTP_RCPT_TMOUT, &var_smtp_rcpt_tmout, 1, 0,
	VAR_SMTP_DATA0_TMOUT, DEF_SMTP_DATA0_TMOUT, &var_smtp_data0_tmout, 1, 0,
	VAR_SMTP_DATA1_TMOUT, DEF_SMTP_DATA1_TMOUT, &var_smtp_data1_tmout, 1, 0,
	VAR_SMTP_DATA2_TMOUT, DEF_SMTP_DATA2_TMOUT, &var_smtp_data2_tmout, 1, 0,
	VAR_SMTP_RSET_TMOUT, DEF_SMTP_RSET_TMOUT, &var_smtp_rset_tmout, 1, 0,
	VAR_SMTP_QUIT_TMOUT, DEF_SMTP_QUIT_TMOUT, &var_smtp_quit_tmout, 1, 0,
	VAR_SMTP_PIX_THRESH, DEF_SMTP_PIX_THRESH, &var_smtp_pix_thresh, 0, 0,
	VAR_SMTP_PIX_DELAY, DEF_SMTP_PIX_DELAY, &var_smtp_pix_delay, 1, 0,
	VAR_SMTP_CACHE_CONN, DEF_SMTP_CACHE_CONN, &var_smtp_cache_conn, 1, 0,
#ifdef USE_TLS
	VAR_SMTP_STARTTLS_TMOUT, DEF_SMTP_STARTTLS_TMOUT, &var_smtp_starttls_tmout, 1, 0,
#endif
	0,
    };
    static CONFIG_INT_TABLE int_table[] = {
	VAR_SMTP_LINE_LIMIT, DEF_SMTP_LINE_LIMIT, &var_smtp_line_limit, 0, 0,
	VAR_SMTP_MXADDR_LIMIT, DEF_SMTP_MXADDR_LIMIT, &var_smtp_mxaddr_limit, 0, 0,
	VAR_SMTP_MXSESS_LIMIT, DEF_SMTP_MXSESS_LIMIT, &var_smtp_mxsess_limit, 0, 0,
	VAR_SMTP_REUSE_LIMIT, DEF_SMTP_REUSE_LIMIT, &var_smtp_reuse_limit, 1, 0,
#ifdef USE_TLS
	VAR_SMTP_TLS_SCERT_VD, DEF_SMTP_TLS_SCERT_VD, &var_smtp_tls_scert_vd, 0, 0,
#endif
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SMTP_SKIP_5XX, DEF_SMTP_SKIP_5XX, &var_smtp_skip_5xx_greeting,
	VAR_IGN_MX_LOOKUP_ERR, DEF_IGN_MX_LOOKUP_ERR, &var_ign_mx_lookup_err,
	VAR_SKIP_QUIT_RESP, DEF_SKIP_QUIT_RESP, &var_skip_quit_resp,
	VAR_SMTP_ALWAYS_EHLO, DEF_SMTP_ALWAYS_EHLO, &var_smtp_always_ehlo,
	VAR_SMTP_NEVER_EHLO, DEF_SMTP_NEVER_EHLO, &var_smtp_never_ehlo,
	VAR_SMTP_SASL_ENABLE, DEF_SMTP_SASL_ENABLE, &var_smtp_sasl_enable,
	VAR_SMTP_RAND_ADDR, DEF_SMTP_RAND_ADDR, &var_smtp_rand_addr,
	VAR_SMTP_QUOTE_821_ENV, DEF_SMTP_QUOTE_821_ENV, &var_smtp_quote_821_env,
	VAR_SMTP_DEFER_MXADDR, DEF_SMTP_DEFER_MXADDR, &var_smtp_defer_mxaddr,
	VAR_SMTP_SEND_XFORWARD, DEF_SMTP_SEND_XFORWARD, &var_smtp_send_xforward,
	VAR_SMTP_CACHE_DEMAND, DEF_SMTP_CACHE_DEMAND, &var_smtp_cache_demand,
	VAR_SMTP_USE_TLS, DEF_SMTP_USE_TLS, &var_smtp_use_tls,
	VAR_SMTP_ENFORCE_TLS, DEF_SMTP_ENFORCE_TLS, &var_smtp_enforce_tls,
#ifdef USE_TLS
	VAR_SMTP_TLS_ENFORCE_PN, DEF_SMTP_TLS_ENFORCE_PN, &var_smtp_tls_enforce_peername,
	VAR_SMTP_TLS_NOTEOFFER, DEF_SMTP_TLS_NOTEOFFER, &var_smtp_tls_note_starttls_offer,
#endif

	0,
    };

    single_server_main(argc, argv, smtp_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       MAIL_SERVER_POST_INIT, post_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_EXIT, pre_exit,
		       0);
}
