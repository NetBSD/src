/*	$NetBSD: postscreen.c,v 1.1.1.4.2.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postscreen 8
/* SUMMARY
/*	Postfix zombie blocker
/* SYNOPSIS
/*	\fBpostscreen\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix \fBpostscreen\fR(8) server provides additional
/*	protection against mail server overload. One \fBpostscreen\fR(8)
/*	process handles multiple inbound SMTP connections, and decides
/*	which clients may talk to a Postfix SMTP server process.
/*	By keeping spambots away, \fBpostscreen\fR(8) leaves more
/*	SMTP server processes available for legitimate clients, and
/*	delays the onset of server overload conditions.
/*
/*	This program should not be used on SMTP ports that receive
/*	mail from end-user clients (MUAs). In a typical deployment,
/*	\fBpostscreen\fR(8) handles the MX service on TCP port 25,
/*	while MUA clients submit mail via the \fBsubmission\fR
/*	service on TCP port 587 which requires client authentication.
/*	Alternatively, a site could set up a dedicated, non-postscreen,
/*	"port 25" server that provides \fBsubmission\fR service and
/*	client authentication, but no MX service.
/*
/*	\fBpostscreen\fR(8) maintains a temporary whitelist for
/*	clients that have passed a number of tests.  When an SMTP
/*	client IP address is whitelisted, \fBpostscreen\fR(8) hands
/*	off the connection immediately to a Postfix SMTP server
/*	process. This minimizes the overhead for legitimate mail.
/*
/*	By default, \fBpostscreen\fR(8) logs statistics and hands
/*	off every connection to a Postfix SMTP server process, while
/*	excluding clients in mynetworks from all tests (primarily,
/*	to avoid problems with non-standard SMTP implementations
/*	in network appliances).  This mode is useful for non-destructive
/*	testing.
/*
/*	In a typical production setting, \fBpostscreen\fR(8) is
/*	configured to reject mail from clients that fail one or
/*	more tests. \fBpostscreen\fR(8) logs rejected mail with the
/*	client address, helo, sender and recipient information.
/*
/*	\fBpostscreen\fR(8) is not an SMTP proxy; this is intentional.
/*	The purpose is to keep spambots away from Postfix SMTP
/*	server processes, while minimizing overhead for legitimate
/*	traffic.
/* SECURITY
/* .ad
/* .fi
/*	The \fBpostscreen\fR(8) server is moderately security-sensitive.
/*	It talks to untrusted clients on the network. The process
/*	can be run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 1123 (Host requirements)
/*	RFC 1652 (8bit-MIME transport)
/*	RFC 1869 (SMTP service extensions)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 1985 (ETRN command)
/*	RFC 2034 (SMTP Enhanced Status Codes)
/*	RFC 2821 (SMTP protocol)
/*	Not: RFC 2920 (SMTP Pipelining)
/*	RFC 3207 (STARTTLS command)
/*	RFC 3461 (SMTP DSN Extension)
/*	RFC 3463 (Enhanced Status Codes)
/*	RFC 5321 (SMTP protocol, including multi-line 220 banners)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The \fBpostscreen\fR(8) built-in SMTP protocol engine
/*	currently does not announce support for AUTH, XCLIENT or
/*	XFORWARD.
/*	If you need to make these services available
/*	on port 25, then do not enable the optional "after 220
/*	server greeting" tests, and do not use DNSBLs that reject
/*	traffic from dial-up and residential networks.
/*
/*	The optional "after 220 server greeting" tests involve
/*	\fBpostscreen\fR(8)'s built-in SMTP protocol engine. When
/*	these tests succeed, \fBpostscreen\fR(8) adds the client
/*	to the temporary whitelist, but it cannot not hand off the
/*	"live" connection to a Postfix SMTP server process in the
/*	middle of a session.  Instead, \fBpostscreen\fR(8) defers
/*	attempts to deliver mail with a 4XX status, and waits for
/*	the client to disconnect.  When the client connects again,
/*	\fBpostscreen\fR(8) will allow the client to talk to a
/*	Postfix SMTP server process (provided that the whitelist
/*	status has not expired).  \fBpostscreen\fR(8) mitigates
/*	the impact of this limitation by giving the "after 220
/*	server greeting" tests a long expiration time.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to main.cf are not picked up automatically, as
/*	\fBpostscreen\fR(8) processes may run for several hours.
/*	Use the command "postfix reload" after a configuration
/*	change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/*
/*	NOTE: Some \fBpostscreen\fR(8) parameters implement
/*	stress-dependent behavior.  This is supported only when the
/*	default parameter value is stress-dependent (that is, it
/*	looks like ${stress?X}${stress:Y}, or it is the $\fIname\fR
/*	of an smtpd parameter with a stress-dependent default).
/*	Other parameters always evaluate as if the \fBstress\fR
/*	parameter value is the empty string.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBpostscreen_command_filter ($smtpd_command_filter)\fR"
/*	A mechanism to transform commands from remote SMTP clients.
/* .IP "\fBpostscreen_discard_ehlo_keyword_address_maps ($smtpd_discard_ehlo_keyword_address_maps)\fR"
/*	Lookup tables, indexed by the remote SMTP client address, with
/*	case insensitive lists of EHLO keywords (pipelining, starttls, auth,
/*	etc.) that the \fBpostscreen\fR(8) server will not send in the EHLO response
/*	to a remote SMTP client.
/* .IP "\fBpostscreen_discard_ehlo_keywords ($smtpd_discard_ehlo_keywords)\fR"
/*	A case insensitive list of EHLO keywords (pipelining, starttls,
/*	auth, etc.) that the \fBpostscreen\fR(8) server will not send in the EHLO
/*	response to a remote SMTP client.
/* TROUBLE SHOOTING CONTROLS
/* .ad
/* .fi
/* .IP "\fBpostscreen_expansion_filter (see 'postconf -d' output)\fR"
/*	List of characters that are permitted in postscreen_reject_footer
/*	attribute expansions.
/* .IP "\fBpostscreen_reject_footer ($smtpd_reject_footer)\fR"
/*	Optional information that is appended after a 4XX or 5XX
/*	\fBpostscreen\fR(8) server
/*	response.
/* .IP "\fBsoft_bounce (no)\fR"
/*	Safety net to keep mail queued that would otherwise be returned to
/*	the sender.
/* BEFORE-POSTSCREEN PROXY AGENT
/* .ad
/* .fi
/*	Available in Postfix version 2.10 and later:
/* .IP "\fBpostscreen_upstream_proxy_protocol (empty)\fR"
/*	The name of the proxy protocol used by an optional before-postscreen
/*	proxy agent.
/* .IP "\fBpostscreen_upstream_proxy_timeout (5s)\fR"
/*	The time limit for the proxy protocol specified with the
/*	postscreen_upstream_proxy_protocol parameter.
/* PERMANENT WHITE/BLACKLIST TEST
/* .ad
/* .fi
/*	This test is executed immediately after a remote SMTP client
/*	connects. If a client is permanently whitelisted, the client
/*	will be handed off immediately to a Postfix SMTP server
/*	process.
/* .IP "\fBpostscreen_access_list (permit_mynetworks)\fR"
/*	Permanent white/blacklist for remote SMTP client IP addresses.
/* .IP "\fBpostscreen_blacklist_action (ignore)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client is
/*	permanently blacklisted with the postscreen_access_list parameter.
/* MAIL EXCHANGER POLICY TESTS
/* .ad
/* .fi
/*	When \fBpostscreen\fR(8) is configured to monitor all primary
/*	and backup MX addresses, it can refuse to whitelist clients
/*	that connect to a backup MX address only. For small sites,
/*	this requires configuring primary and backup MX addresses
/*	on the same MTA. Larger sites would have to share the
/*	\fBpostscreen\fR(8) cache between primary and backup MTAs,
/*	which would introduce a common point of failure.
/* .IP "\fBpostscreen_whitelist_interfaces (static:all)\fR"
/*	A list of local \fBpostscreen\fR(8) server IP addresses where a
/*	non-whitelisted remote SMTP client can obtain \fBpostscreen\fR(8)'s temporary
/*	whitelist status.
/* BEFORE 220 GREETING TESTS
/* .ad
/* .fi
/*	These tests are executed before the remote SMTP client
/*	receives the "220 servername" greeting. If no tests remain
/*	after the successful completion of this phase, the client
/*	will be handed off immediately to a Postfix SMTP server
/*	process.
/* .IP "\fBdnsblog_service_name (dnsblog)\fR"
/*	The name of the \fBdnsblog\fR(8) service entry in master.cf.
/* .IP "\fBpostscreen_dnsbl_action (ignore)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client's combined
/*	DNSBL score is equal to or greater than a threshold (as defined
/*	with the postscreen_dnsbl_sites and postscreen_dnsbl_threshold
/*	parameters).
/* .IP "\fBpostscreen_dnsbl_reply_map (empty)\fR"
/*	A mapping from actual DNSBL domain name which includes a secret
/*	password, to the DNSBL domain name that postscreen will reply with
/*	when it rejects mail.
/* .IP "\fBpostscreen_dnsbl_sites (empty)\fR"
/*	Optional list of DNS white/blacklist domains, filters and weight
/*	factors.
/* .IP "\fBpostscreen_dnsbl_threshold (1)\fR"
/*	The inclusive lower bound for blocking a remote SMTP client, based on
/*	its combined DNSBL score as defined with the postscreen_dnsbl_sites
/*	parameter.
/* .IP "\fBpostscreen_greet_action (ignore)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client speaks
/*	before its turn within the time specified with the postscreen_greet_wait
/*	parameter.
/* .IP "\fBpostscreen_greet_banner ($smtpd_banner)\fR"
/*	The \fItext\fR in the optional "220-\fItext\fR..." server
/*	response that
/*	\fBpostscreen\fR(8) sends ahead of the real Postfix SMTP server's "220
/*	text..." response, in an attempt to confuse bad SMTP clients so
/*	that they speak before their turn (pre-greet).
/* .IP "\fBpostscreen_greet_wait (${stress?2}${stress:6}s)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will wait for an SMTP
/*	client to send a command before its turn, and for DNS blocklist
/*	lookup results to arrive (default: up to 2 seconds under stress,
/*	up to 6 seconds otherwise).
/* .IP "\fBsmtpd_service_name (smtpd)\fR"
/*	The internal service that \fBpostscreen\fR(8) hands off allowed
/*	connections to.
/* .PP
/*	Available in Postfix version 2.11 and later:
/* .IP "\fBpostscreen_dnsbl_whitelist_threshold (0)\fR"
/*	Allow a remote SMTP client to skip "before" and "after 220
/*	greeting" protocol tests, based on its combined DNSBL score as
/*	defined with the postscreen_dnsbl_sites parameter.
/* AFTER 220 GREETING TESTS
/* .ad
/* .fi
/*	These tests are executed after the remote SMTP client
/*	receives the "220 servername" greeting. If a client passes
/*	all tests during this phase, it will receive a 4XX response
/*	to all RCPT TO commands. After the client reconnects, it
/*	will be allowed to talk directly to a Postfix SMTP server
/*	process.
/* .IP "\fBpostscreen_bare_newline_action (ignore)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client sends
/*	a bare newline character, that is, a newline not preceded by carriage
/*	return.
/* .IP "\fBpostscreen_bare_newline_enable (no)\fR"
/*	Enable "bare newline" SMTP protocol tests in the \fBpostscreen\fR(8)
/*	server.
/* .IP "\fBpostscreen_disable_vrfy_command ($disable_vrfy_command)\fR"
/*	Disable the SMTP VRFY command in the \fBpostscreen\fR(8) daemon.
/* .IP "\fBpostscreen_forbidden_commands ($smtpd_forbidden_commands)\fR"
/*	List of commands that the \fBpostscreen\fR(8) server considers in
/*	violation of the SMTP protocol.
/* .IP "\fBpostscreen_helo_required ($smtpd_helo_required)\fR"
/*	Require that a remote SMTP client sends HELO or EHLO before
/*	commencing a MAIL transaction.
/* .IP "\fBpostscreen_non_smtp_command_action (drop)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client sends
/*	non-SMTP commands as specified with the postscreen_forbidden_commands
/*	parameter.
/* .IP "\fBpostscreen_non_smtp_command_enable (no)\fR"
/*	Enable "non-SMTP command" tests in the \fBpostscreen\fR(8) server.
/* .IP "\fBpostscreen_pipelining_action (enforce)\fR"
/*	The action that \fBpostscreen\fR(8) takes when a remote SMTP client
/*	sends
/*	multiple commands instead of sending one command and waiting for
/*	the server to respond.
/* .IP "\fBpostscreen_pipelining_enable (no)\fR"
/*	Enable "pipelining" SMTP protocol tests in the \fBpostscreen\fR(8)
/*	server.
/* CACHE CONTROLS
/* .ad
/* .fi
/* .IP "\fBpostscreen_cache_cleanup_interval (12h)\fR"
/*	The amount of time between \fBpostscreen\fR(8) cache cleanup runs.
/* .IP "\fBpostscreen_cache_map (btree:$data_directory/postscreen_cache)\fR"
/*	Persistent storage for the \fBpostscreen\fR(8) server decisions.
/* .IP "\fBpostscreen_cache_retention_time (7d)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will cache an expired
/*	temporary whitelist entry before it is removed.
/* .IP "\fBpostscreen_bare_newline_ttl (30d)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will use the result from
/*	a successful "bare newline" SMTP protocol test.
/* .IP "\fBpostscreen_dnsbl_ttl (1h)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will use the result from
/*	a successful DNS blocklist test.
/* .IP "\fBpostscreen_greet_ttl (1d)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will use the result from
/*	a successful PREGREET test.
/* .IP "\fBpostscreen_non_smtp_command_ttl (30d)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will use the result from
/*	a successful "non_smtp_command" SMTP protocol test.
/* .IP "\fBpostscreen_pipelining_ttl (30d)\fR"
/*	The amount of time that \fBpostscreen\fR(8) will use the result from
/*	a successful "pipelining" SMTP protocol test.
/* RESOURCE CONTROLS
/* .ad
/* .fi
/* .IP "\fBline_length_limit (2048)\fR"
/*	Upon input, long lines are chopped up into pieces of at most
/*	this length; upon delivery, long lines are reconstructed.
/* .IP "\fBpostscreen_client_connection_count_limit ($smtpd_client_connection_count_limit)\fR"
/*	How many simultaneous connections any remote SMTP client is
/*	allowed to have
/*	with the \fBpostscreen\fR(8) daemon.
/* .IP "\fBpostscreen_command_count_limit (20)\fR"
/*	The limit on the total number of commands per SMTP session for
/*	\fBpostscreen\fR(8)'s built-in SMTP protocol engine.
/* .IP "\fBpostscreen_command_time_limit (${stress?10}${stress:300}s)\fR"
/*	The time limit to read an entire command line with \fBpostscreen\fR(8)'s
/*	built-in SMTP protocol engine.
/* .IP "\fBpostscreen_post_queue_limit ($default_process_limit)\fR"
/*	The number of clients that can be waiting for service from a
/*	real Postfix SMTP server process.
/* .IP "\fBpostscreen_pre_queue_limit ($default_process_limit)\fR"
/*	The number of non-whitelisted clients that can be waiting for
/*	a decision whether they will receive service from a real Postfix
/*	SMTP server
/*	process.
/* .IP "\fBpostscreen_watchdog_timeout (10s)\fR"
/*	How much time a \fBpostscreen\fR(8) process may take to respond to
/*	a remote SMTP client command or to perform a cache operation before it
/*	is terminated by a built-in watchdog timer.
/* STARTTLS CONTROLS
/* .ad
/* .fi
/* .IP "\fBpostscreen_tls_security_level ($smtpd_tls_security_level)\fR"
/*	The SMTP TLS security level for the \fBpostscreen\fR(8) server; when
/*	a non-empty value is specified, this overrides the obsolete parameters
/*	postscreen_use_tls and postscreen_enforce_tls.
/* .IP "\fBtlsproxy_service_name (tlsproxy)\fR"
/*	The name of the \fBtlsproxy\fR(8) service entry in master.cf.
/* OBSOLETE STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/*	These parameters are supported for compatibility with
/*	\fBsmtpd\fR(8) legacy parameters.
/* .IP "\fBpostscreen_use_tls ($smtpd_use_tls)\fR"
/*	Opportunistic TLS: announce STARTTLS support to remote SMTP clients,
/*	but do not require that clients use TLS encryption.
/* .IP "\fBpostscreen_enforce_tls ($smtpd_enforce_tls)\fR"
/*	Mandatory TLS: announce STARTTLS support to remote SMTP clients, and
/*	require that clients use TLS encryption.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdelay_logging_resolution_limit (2)\fR"
/*	The maximal number of digits after the decimal point when logging
/*	sub-second delay values.
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	smtpd(8), Postfix SMTP server
/*	tlsproxy(8), Postfix TLS proxy server
/*	dnsblog(8), DNS black/whitelist logger
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or "\fBpostconf
/*	html_directory\fR" to locate this information.
/* .nf
/* .na
/*	POSTSCREEN_README, Postfix Postscreen Howto
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 2.8.
/*
/*	Many ideas in \fBpostscreen\fR(8) were explored in earlier
/*	work by Michael Tokarev, in OpenBSD spamd, and in MailChannels
/*	Traffic Control.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <myaddrinfo.h>
#include <dict_cache.h>
#include <set_eugid.h>
#include <vstream.h>
#include <name_code.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <data_redirect.h>
#include <string_list.h>

/* Master server protocols. */

#include <mail_server.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Configuration parameters.
  */
char   *var_smtpd_service;
char   *var_smtpd_banner;
bool    var_disable_vrfy_cmd;
bool    var_helo_required;

char   *var_smtpd_cmd_filter;
char   *var_psc_cmd_filter;

char   *var_smtpd_forbid_cmds;
char   *var_psc_forbid_cmds;

char   *var_smtpd_ehlo_dis_words;
char   *var_smtpd_ehlo_dis_maps;
char   *var_psc_ehlo_dis_words;
char   *var_psc_ehlo_dis_maps;

char   *var_smtpd_tls_level;
bool    var_smtpd_use_tls;
bool    var_smtpd_enforce_tls;
char   *var_psc_tls_level;
bool    var_psc_use_tls;
bool    var_psc_enforce_tls;

bool    var_psc_disable_vrfy;
bool    var_psc_helo_required;

char   *var_psc_cache_map;
int     var_psc_cache_scan;
int     var_psc_cache_ret;
int     var_psc_post_queue_limit;
int     var_psc_pre_queue_limit;
int     var_psc_watchdog;

char   *var_psc_acl;
char   *var_psc_blist_action;

char   *var_psc_greet_ttl;
int     var_psc_greet_wait;

char   *var_psc_pregr_banner;
char   *var_psc_pregr_action;
int     var_psc_pregr_ttl;

char   *var_psc_dnsbl_sites;
char   *var_psc_dnsbl_reply;
int     var_psc_dnsbl_thresh;
int     var_psc_dnsbl_wthresh;
char   *var_psc_dnsbl_action;
int     var_psc_dnsbl_ttl;

bool    var_psc_pipel_enable;
char   *var_psc_pipel_action;
int     var_psc_pipel_ttl;

bool    var_psc_nsmtp_enable;
char   *var_psc_nsmtp_action;
int     var_psc_nsmtp_ttl;

bool    var_psc_barlf_enable;
char   *var_psc_barlf_action;
int     var_psc_barlf_ttl;

int     var_psc_cmd_count;
char   *var_psc_cmd_time;

char   *var_dnsblog_service;
char   *var_tlsproxy_service;

char   *var_smtpd_rej_footer;
char   *var_psc_rej_footer;

int     var_smtpd_cconn_limit;
int     var_psc_cconn_limit;

char   *var_smtpd_exp_filter;
char   *var_psc_exp_filter;

char   *var_psc_wlist_if;
char   *var_psc_uproxy_proto;
int     var_psc_uproxy_tmout;

 /*
  * Global variables.
  */
int     psc_check_queue_length;		/* connections being checked */
int     psc_post_queue_length;		/* being sent to real SMTPD */
DICT_CACHE *psc_cache_map;		/* cache table handle */
VSTRING *psc_temp;			/* scratchpad */
char   *psc_smtpd_service_name;		/* path to real SMTPD */
int     psc_pregr_action;		/* PSC_ACT_DROP/ENFORCE/etc */
int     psc_dnsbl_action;		/* PSC_ACT_DROP/ENFORCE/etc */
int     psc_pipel_action;		/* PSC_ACT_DROP/ENFORCE/etc */
int     psc_nsmtp_action;		/* PSC_ACT_DROP/ENFORCE/etc */
int     psc_barlf_action;		/* PSC_ACT_DROP/ENFORCE/etc */
int     psc_min_ttl;			/* Update with new tests! */
int     psc_max_ttl;			/* Update with new tests! */
STRING_LIST *psc_forbid_cmds;		/* CONNECT GET POST */
int     psc_stress_greet_wait;		/* stressed greet wait */
int     psc_normal_greet_wait;		/* stressed greet wait */
int     psc_stress_cmd_time_limit;	/* stressed command limit */
int     psc_normal_cmd_time_limit;	/* normal command time limit */
int     psc_stress;			/* stress level */
int     psc_lowat_check_queue_length;	/* stress low-water mark */
int     psc_hiwat_check_queue_length;	/* stress high-water mark */
DICT   *psc_dnsbl_reply;		/* DNSBL name mapper */
HTABLE *psc_client_concurrency;		/* per-client concurrency */

 /*
  * Local variables and functions.
  */
static ARGV *psc_acl;			/* permanent white/backlist */
static int psc_blist_action;		/* PSC_ACT_DROP/ENFORCE/etc */
static ADDR_MATCH_LIST *psc_wlist_if;	/* whitelist interfaces */

static void psc_endpt_lookup_done(int, VSTREAM *,
			             MAI_HOSTADDR_STR *, MAI_SERVPORT_STR *,
			            MAI_HOSTADDR_STR *, MAI_SERVPORT_STR *);

/* psc_dump - dump some statistics before exit */

static void psc_dump(void)
{

    /*
     * Dump preliminary cache cleanup statistics when the process commits
     * suicide while a cache cleanup run is in progress. We can't currently
     * distinguish between "postfix reload" (we should restart) or "maximal
     * idle time reached" (we could finish the cache cleanup first).
     */
    if (psc_cache_map) {
	dict_cache_close(psc_cache_map);
	psc_cache_map = 0;
    }
}

/* psc_drain - delayed exit after "postfix reload" */

static void psc_drain(char *unused_service, char **unused_argv)
{
    int     count;

    /*
     * After "postfix reload", complete work-in-progress in the background,
     * instead of dropping already-accepted connections on the floor.
     * 
     * Unfortunately we must close all writable tables, so we can't store or
     * look up reputation information. The reason is that we don't have any
     * multi-writer safety guarantees. We also can't use the single-writer
     * proxywrite service, because its latency guarantees are too weak.
     * 
     * All error retry counts shall be limited. Instead of blocking here, we
     * could retry failed fork() operations in the event call-back routines,
     * but we don't need perfection. The host system is severely overloaded
     * and service levels are already way down.
     * 
     * XXX Some Berkeley DB versions break with close-after-fork. Every new
     * version is an improvement over its predecessor.
     */
    if (psc_cache_map != 0 /* XXX && psc_cache_map requires locking */) {
	dict_cache_close(psc_cache_map);
	psc_cache_map = 0;
    }
    for (count = 0; /* see below */ ; count++) {
	if (count >= 5) {
	    msg_fatal("fork: %m");
	} else if (event_server_drain() != 0) {
	    msg_warn("fork: %m");
	    sleep(1);
	    continue;
	} else {
	    return;
	}
    }
}

/* psc_service - handle new client connection */

static void psc_service(VSTREAM *smtp_client_stream,
			        char *unused_service,
			        char **unused_argv)
{

    /*
     * For sanity, require that at least one of INET or INET6 is enabled.
     * Otherwise, we can't look up interface information, and we can't
     * convert names or addresses.
     */
    if (inet_proto_info()->ai_family_list[0] == 0)
	msg_fatal("all network protocols are disabled (%s = %s)",
		  VAR_INET_PROTOCOLS, var_inet_protocols);

    /*
     * This program handles all incoming connections, so it must not block.
     * We use event-driven code for all operations that introduce latency.
     * 
     * Note: instead of using VSTREAM-level timeouts, we enforce limits on the
     * total amount of time to receive a complete SMTP command line.
     */
    non_blocking(vstream_fileno(smtp_client_stream), NON_BLOCKING);

    /*
     * Look up the remote SMTP client address and port.
     */
    psc_endpt_lookup(smtp_client_stream, psc_endpt_lookup_done);
}

/* psc_endpt_lookup_done - endpoint lookup completed */

static void psc_endpt_lookup_done(int endpt_status,
				          VSTREAM *smtp_client_stream,
				          MAI_HOSTADDR_STR *smtp_client_addr,
				          MAI_SERVPORT_STR *smtp_client_port,
				          MAI_HOSTADDR_STR *smtp_server_addr,
				          MAI_SERVPORT_STR *smtp_server_port)
{
    const char *myname = "psc_endpt_lookup_done";
    PSC_STATE *state;
    const char *stamp_str;
    int     saved_flags;

    /*
     * Best effort - if this non-blocking write(2) fails, so be it.
     */
    if (endpt_status < 0) {
	(void) write(vstream_fileno(smtp_client_stream),
		     "421 4.3.2 No system resources\r\n",
		     sizeof("421 4.3.2 No system resources\r\n") - 1);
	event_server_disconnect(smtp_client_stream);
	return;
    }
    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d connect from [%s]:%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 smtp_client_addr->buf, smtp_client_port->buf);

    msg_info("CONNECT from [%s]:%s to [%s]:%s",
	     smtp_client_addr->buf, smtp_client_port->buf,
	     smtp_server_addr->buf, smtp_server_port->buf);

    /*
     * Bundle up all the loose session pieces. This zeroes all flags and time
     * stamps.
     */
    state = psc_new_session_state(smtp_client_stream, smtp_client_addr->buf,
				  smtp_client_port->buf,
				  smtp_server_addr->buf,
				  smtp_server_port->buf);

    /*
     * Reply with 421 when the client has too many open connections.
     */
    if (var_psc_cconn_limit > 0
	&& state->client_concurrency > var_psc_cconn_limit) {
	msg_info("NOQUEUE: reject: CONNECT from [%s]:%s: too many connections",
		 state->smtp_client_addr, state->smtp_client_port);
	PSC_DROP_SESSION_STATE(state,
			       "421 4.7.0 Error: too many connections\r\n");
	return;
    }

    /*
     * Reply with 421 when we can't forward more connections.
     */
    if (var_psc_post_queue_limit > 0
	&& psc_post_queue_length >= var_psc_post_queue_limit) {
	msg_info("NOQUEUE: reject: CONNECT from [%s]:%s: all server ports busy",
		 state->smtp_client_addr, state->smtp_client_port);
	PSC_DROP_SESSION_STATE(state,
			       "421 4.3.2 All server ports are busy\r\n");
	return;
    }

    /*
     * The permanent white/blacklist has highest precedence.
     */
    if (psc_acl != 0) {
	switch (psc_acl_eval(state, psc_acl, VAR_PSC_ACL)) {

	    /*
	     * Permanently blacklisted.
	     */
	case PSC_ACL_ACT_BLACKLIST:
	    msg_info("BLACKLISTED [%s]:%s", PSC_CLIENT_ADDR_PORT(state));
	    PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_BLIST_FAIL);
	    switch (psc_blist_action) {
	    case PSC_ACT_DROP:
		PSC_DROP_SESSION_STATE(state,
			     "521 5.3.2 Service currently unavailable\r\n");
		return;
	    case PSC_ACT_ENFORCE:
		PSC_ENFORCE_SESSION_STATE(state,
			     "550 5.3.2 Service currently unavailable\r\n");
		break;
	    case PSC_ACT_IGNORE:
		PSC_UNFAIL_SESSION_STATE(state, PSC_STATE_FLAG_BLIST_FAIL);

		/*
		 * Not: PSC_PASS_SESSION_STATE. Repeat this test the next
		 * time.
		 */
		break;
	    default:
		msg_panic("%s: unknown blacklist action value %d",
			  myname, psc_blist_action);
	    }
	    break;

	    /*
	     * Permanently whitelisted.
	     */
	case PSC_ACL_ACT_WHITELIST:
	    msg_info("WHITELISTED [%s]:%s", PSC_CLIENT_ADDR_PORT(state));
	    psc_conclude(state);
	    return;

	    /*
	     * Other: dunno (don't know) or error.
	     */
	default:
	    break;
	}
    }

    /*
     * The temporary whitelist (i.e. the postscreen cache) has the lowest
     * precedence. This cache contains information about the results of prior
     * tests. Whitelist the client when all enabled test results are still
     * valid.
     */
    if ((state->flags & PSC_STATE_MASK_ANY_FAIL) == 0
	&& psc_cache_map != 0
	&& (stamp_str = psc_cache_lookup(psc_cache_map, state->smtp_client_addr)) != 0) {
	saved_flags = state->flags;
	psc_parse_tests(state, stamp_str, event_time());
	state->flags |= saved_flags;
	if (msg_verbose)
	    msg_info("%s: cached + recent flags: %s",
		     myname, psc_print_state_flags(state->flags, myname));
	if ((state->flags & PSC_STATE_MASK_ANY_TODO_FAIL) == 0) {
	    msg_info("PASS OLD [%s]:%s", PSC_CLIENT_ADDR_PORT(state));
	    psc_conclude(state);
	    return;
	}
    } else {
	saved_flags = state->flags;
	psc_new_tests(state);
	state->flags |= saved_flags;
	if (msg_verbose)
	    msg_info("%s: new + recent flags: %s",
		     myname, psc_print_state_flags(state->flags, myname));
    }

    /*
     * Don't whitelist clients that connect to backup MX addresses. Fail
     * "closed" on error.
     */
    if (addr_match_list_match(psc_wlist_if, smtp_server_addr->buf) == 0) {
	state->flags |= (PSC_STATE_FLAG_WLIST_FAIL | PSC_STATE_FLAG_NOFORWARD);
	msg_info("WHITELIST VETO [%s]:%s", PSC_CLIENT_ADDR_PORT(state));
    }

    /*
     * Reply with 421 when we can't analyze more connections. That also means
     * no deep protocol tests when the noforward flag is raised.
     */
    if (var_psc_pre_queue_limit > 0
	&& psc_check_queue_length - psc_post_queue_length
	>= var_psc_pre_queue_limit) {
	msg_info("reject: connect from [%s]:%s: all screening ports busy",
		 state->smtp_client_addr, state->smtp_client_port);
	PSC_DROP_SESSION_STATE(state,
			       "421 4.3.2 All screening ports are busy\r\n");
	return;
    }

    /*
     * If the client has no up-to-date results for some tests, do those tests
     * first. Otherwise, skip the tests and hand off the connection.
     */
    if (state->flags & PSC_STATE_MASK_EARLY_TODO)
	psc_early_tests(state);
    else if (state->flags & (PSC_STATE_MASK_SMTPD_TODO | PSC_STATE_FLAG_NOFORWARD))
	psc_smtpd_tests(state);
    else
	psc_conclude(state);
}

/* psc_cache_validator - validate one cache entry */

static int psc_cache_validator(const char *client_addr,
			               const char *stamp_str,
			               char *unused_context)
{
    PSC_STATE dummy;

    /*
     * This function is called by the cache cleanup pseudo thread.
     * 
     * When an entry is removed from the cache, the client will be reported as
     * "NEW" in the next session where it passes all tests again. To avoid
     * silly logging we remove the cache entry only after all tests have
     * expired longer ago than the cache retention time.
     */
    psc_parse_tests(&dummy, stamp_str, event_time() - var_psc_cache_ret);
    return ((dummy.flags & PSC_STATE_MASK_ANY_TODO) == 0);
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    VSTRING *redirect;

    /*
     * Open read-only maps before dropping privilege, for consistency with
     * other Postfix daemons.
     */
    psc_acl_pre_jail_init(var_mynetworks, VAR_PSC_ACL);
    if (*var_psc_acl)
	psc_acl = psc_acl_parse(var_psc_acl, VAR_PSC_ACL);
    /* Ignore smtpd_forbid_cmds lookup errors. Non-critical feature. */
    if (*var_psc_forbid_cmds)
	psc_forbid_cmds = string_list_init(MATCH_FLAG_RETURN,
					   var_psc_forbid_cmds);
    if (*var_psc_dnsbl_reply)
	psc_dnsbl_reply = dict_open(var_psc_dnsbl_reply, O_RDONLY,
				    DICT_FLAG_DUP_WARN);

    /*
     * Never, ever, get killed by a master signal, as that would corrupt the
     * database when we're in the middle of an update.
     */
    if (setsid() < 0)
	msg_warn("setsid: %m");

    /*
     * Security: don't create root-owned files that contain untrusted data.
     * And don't create Postfix-owned files in root-owned directories,
     * either. We want a correct relationship between (file or directory)
     * ownership and (file or directory) content. To open files before going
     * to jail, temporarily drop root privileges.
     */
    SAVE_AND_SET_EUGID(var_owner_uid, var_owner_gid);
    redirect = vstring_alloc(100);

    /*
     * Keep state in persistent external map. As a safety measure we sync the
     * database on each update. This hurts on LINUX file systems that sync
     * all dirty disk blocks whenever any application invokes fsync().
     * 
     * Start the cache maintenance pseudo thread after dropping privileges.
     */
#define PSC_DICT_OPEN_FLAGS (DICT_FLAG_DUP_REPLACE | DICT_FLAG_SYNC_UPDATE | \
	    DICT_FLAG_OPEN_LOCK)

    if (*var_psc_cache_map)
	psc_cache_map =
	    dict_cache_open(data_redirect_map(redirect, var_psc_cache_map),
			    O_CREAT | O_RDWR, PSC_DICT_OPEN_FLAGS);

    /*
     * Clean up and restore privilege.
     */
    vstring_free(redirect);
    RESTORE_SAVED_EUGID();

    /*
     * Initialize the dummy SMTP engine.
     */
    psc_smtpd_pre_jail_init();
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    static time_t last_event_time;
    time_t  new_event_time;
    const char *name;

    /*
     * If some table has changed then stop accepting new connections. Don't
     * check the tables more than once a second.
     */
    new_event_time = event_time();
    if (new_event_time >= last_event_time + 1
	&& (name = dict_changed_name()) != 0) {
	msg_info("table %s has changed - finishing in the background", name);
	event_server_drain();
    } else {
	last_event_time = new_event_time;
    }
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{
    const NAME_CODE actions[] = {
	PSC_NAME_ACT_DROP, PSC_ACT_DROP,
	PSC_NAME_ACT_ENFORCE, PSC_ACT_ENFORCE,
	PSC_NAME_ACT_IGNORE, PSC_ACT_IGNORE,
	PSC_NAME_ACT_CONT, PSC_ACT_IGNORE,	/* compatibility */
	0, -1,
    };
    int     cache_flags;
    const char *tmp;

    /*
     * This routine runs after the skeleton code has entered the chroot jail.
     * Prevent automatic process suicide after a limited number of client
     * requests. It is OK to terminate after a limited amount of idle time.
     */
    var_use_limit = 0;

    /*
     * Workaround for parameters whose values may contain "$", and that have
     * a default of "$parametername". Not sure if it would be a good idea to
     * always to this in the mail_conf_raw(3) module.
     */
    if (*var_psc_rej_footer == '$'
	&& mail_conf_lookup(var_psc_rej_footer + 1)) {
	tmp = mail_conf_eval_once(var_psc_rej_footer);
	myfree(var_psc_rej_footer);
	var_psc_rej_footer = mystrdup(tmp);
    }
    if (*var_psc_exp_filter == '$'
	&& mail_conf_lookup(var_psc_exp_filter + 1)) {
	tmp = mail_conf_eval_once(var_psc_exp_filter);
	myfree(var_psc_exp_filter);
	var_psc_exp_filter = mystrdup(tmp);
    }

    /*
     * Other one-time initialization.
     */
    psc_temp = vstring_alloc(10);
    vstring_sprintf(psc_temp, "%s/%s", MAIL_CLASS_PRIVATE, var_smtpd_service);
    psc_smtpd_service_name = mystrdup(STR(psc_temp));
    psc_dnsbl_init();
    psc_early_init();
    psc_smtpd_init();

    if ((psc_blist_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_blist_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_BLIST_ACTION,
		  var_psc_blist_action);
    if ((psc_dnsbl_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_dnsbl_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_DNSBL_ACTION,
		  var_psc_dnsbl_action);
    if ((psc_pregr_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_pregr_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_PREGR_ACTION,
		  var_psc_pregr_action);
    if ((psc_pipel_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_pipel_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_PIPEL_ACTION,
		  var_psc_pipel_action);
    if ((psc_nsmtp_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_nsmtp_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_NSMTP_ACTION,
		  var_psc_nsmtp_action);
    if ((psc_barlf_action = name_code(actions, NAME_CODE_FLAG_NONE,
				      var_psc_barlf_action)) < 0)
	msg_fatal("bad %s value: %s", VAR_PSC_BARLF_ACTION,
		  var_psc_barlf_action);
    /* Fail "closed" on error. */
    psc_wlist_if = addr_match_list_init(MATCH_FLAG_RETURN, var_psc_wlist_if);

    /*
     * Start the cache maintenance pseudo thread last. Early cleanup makes
     * verbose logging more informative (we get positive confirmation that
     * the cleanup thread runs).
     */
    cache_flags = DICT_CACHE_FLAG_STATISTICS;
    if (msg_verbose > 1)
	cache_flags |= DICT_CACHE_FLAG_VERBOSE;
    if (psc_cache_map != 0 && var_psc_cache_scan > 0)
	dict_cache_control(psc_cache_map,
			   DICT_CACHE_CTL_FLAGS, cache_flags,
			   DICT_CACHE_CTL_INTERVAL, var_psc_cache_scan,
			   DICT_CACHE_CTL_VALIDATOR, psc_cache_validator,
			   DICT_CACHE_CTL_CONTEXT, (char *) 0,
			   DICT_CACHE_CTL_END);

    /*
     * Pre-compute the minimal and maximal TTL.
     */
    psc_min_ttl =
	PSC_MIN(PSC_MIN(var_psc_pregr_ttl, var_psc_dnsbl_ttl),
		PSC_MIN(PSC_MIN(var_psc_pipel_ttl, var_psc_nsmtp_ttl),
			var_psc_barlf_ttl));
    psc_max_ttl =
	PSC_MAX(PSC_MAX(var_psc_pregr_ttl, var_psc_dnsbl_ttl),
		PSC_MAX(PSC_MAX(var_psc_pipel_ttl, var_psc_nsmtp_ttl),
			var_psc_barlf_ttl));

    /*
     * Pre-compute the stress and normal command time limits.
     */
    mail_conf_update(VAR_STRESS, "yes");
    psc_stress_cmd_time_limit =
	get_mail_conf_time(VAR_PSC_CMD_TIME, DEF_PSC_CMD_TIME, 1, 0);
    psc_stress_greet_wait =
	get_mail_conf_time(VAR_PSC_GREET_WAIT, DEF_PSC_GREET_WAIT, 1, 0);

    mail_conf_update(VAR_STRESS, "");
    psc_normal_cmd_time_limit =
	get_mail_conf_time(VAR_PSC_CMD_TIME, DEF_PSC_CMD_TIME, 1, 0);
    psc_normal_greet_wait =
	get_mail_conf_time(VAR_PSC_GREET_WAIT, DEF_PSC_GREET_WAIT, 1, 0);

    psc_lowat_check_queue_length = .7 * var_psc_pre_queue_limit;
    psc_hiwat_check_queue_length = .9 * var_psc_pre_queue_limit;
    if (msg_verbose)
	msg_info(VAR_PSC_CMD_TIME ": stress=%d normal=%d lowat=%d hiwat=%d",
		 psc_stress_cmd_time_limit, psc_normal_cmd_time_limit,
		 psc_lowat_check_queue_length, psc_hiwat_check_queue_length);

    if (psc_lowat_check_queue_length == 0)
	msg_panic("compiler error: 0.7 * %d = %d", var_psc_pre_queue_limit,
		  psc_lowat_check_queue_length);
    if (psc_hiwat_check_queue_length == 0)
	msg_panic("compiler error: 0.9 * %d = %d", var_psc_pre_queue_limit,
		  psc_hiwat_check_queue_length);

    /*
     * Per-client concurrency.
     */
    psc_client_concurrency = htable_create(var_psc_pre_queue_limit);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{

    /*
     * List smtpd(8) parameters before any postscreen(8) parameters that have
     * defaults dependencies on them.
     */
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_SMTPD_SERVICE, DEF_SMTPD_SERVICE, &var_smtpd_service, 1, 0,
	VAR_SMTPD_BANNER, DEF_SMTPD_BANNER, &var_smtpd_banner, 1, 0,
	VAR_SMTPD_FORBID_CMDS, DEF_SMTPD_FORBID_CMDS, &var_smtpd_forbid_cmds, 0, 0,
	VAR_SMTPD_EHLO_DIS_WORDS, DEF_SMTPD_EHLO_DIS_WORDS, &var_smtpd_ehlo_dis_words, 0, 0,
	VAR_SMTPD_EHLO_DIS_MAPS, DEF_SMTPD_EHLO_DIS_MAPS, &var_smtpd_ehlo_dis_maps, 0, 0,
	VAR_SMTPD_TLS_LEVEL, DEF_SMTPD_TLS_LEVEL, &var_smtpd_tls_level, 0, 0,
	VAR_SMTPD_CMD_FILTER, DEF_SMTPD_CMD_FILTER, &var_smtpd_cmd_filter, 0, 0,
	VAR_PSC_CACHE_MAP, DEF_PSC_CACHE_MAP, &var_psc_cache_map, 0, 0,
	VAR_PSC_PREGR_BANNER, DEF_PSC_PREGR_BANNER, &var_psc_pregr_banner, 0, 0,
	VAR_PSC_PREGR_ACTION, DEF_PSC_PREGR_ACTION, &var_psc_pregr_action, 1, 0,
	VAR_PSC_DNSBL_SITES, DEF_PSC_DNSBL_SITES, &var_psc_dnsbl_sites, 0, 0,
	VAR_PSC_DNSBL_ACTION, DEF_PSC_DNSBL_ACTION, &var_psc_dnsbl_action, 1, 0,
	VAR_PSC_PIPEL_ACTION, DEF_PSC_PIPEL_ACTION, &var_psc_pipel_action, 1, 0,
	VAR_PSC_NSMTP_ACTION, DEF_PSC_NSMTP_ACTION, &var_psc_nsmtp_action, 1, 0,
	VAR_PSC_BARLF_ACTION, DEF_PSC_BARLF_ACTION, &var_psc_barlf_action, 1, 0,
	VAR_PSC_ACL, DEF_PSC_ACL, &var_psc_acl, 0, 0,
	VAR_PSC_BLIST_ACTION, DEF_PSC_BLIST_ACTION, &var_psc_blist_action, 1, 0,
	VAR_PSC_FORBID_CMDS, DEF_PSC_FORBID_CMDS, &var_psc_forbid_cmds, 0, 0,
	VAR_PSC_EHLO_DIS_WORDS, DEF_PSC_EHLO_DIS_WORDS, &var_psc_ehlo_dis_words, 0, 0,
	VAR_PSC_EHLO_DIS_MAPS, DEF_PSC_EHLO_DIS_MAPS, &var_psc_ehlo_dis_maps, 0, 0,
	VAR_PSC_DNSBL_REPLY, DEF_PSC_DNSBL_REPLY, &var_psc_dnsbl_reply, 0, 0,
	VAR_PSC_TLS_LEVEL, DEF_PSC_TLS_LEVEL, &var_psc_tls_level, 0, 0,
	VAR_PSC_CMD_FILTER, DEF_PSC_CMD_FILTER, &var_psc_cmd_filter, 0, 0,
	VAR_DNSBLOG_SERVICE, DEF_DNSBLOG_SERVICE, &var_dnsblog_service, 1, 0,
	VAR_TLSPROXY_SERVICE, DEF_TLSPROXY_SERVICE, &var_tlsproxy_service, 1, 0,
	VAR_PSC_WLIST_IF, DEF_PSC_WLIST_IF, &var_psc_wlist_if, 0, 0,
	VAR_PSC_UPROXY_PROTO, DEF_PSC_UPROXY_PROTO, &var_psc_uproxy_proto, 0, 0,
	0,
    };
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_PSC_DNSBL_THRESH, DEF_PSC_DNSBL_THRESH, &var_psc_dnsbl_thresh, 0, 0,
	VAR_PSC_DNSBL_WTHRESH, DEF_PSC_DNSBL_WTHRESH, &var_psc_dnsbl_wthresh, 0, 0,
	VAR_PSC_CMD_COUNT, DEF_PSC_CMD_COUNT, &var_psc_cmd_count, 1, 0,
	VAR_SMTPD_CCONN_LIMIT, DEF_SMTPD_CCONN_LIMIT, &var_smtpd_cconn_limit, 0, 0,
	0,
    };
    static const CONFIG_NINT_TABLE nint_table[] = {
	VAR_PSC_POST_QLIMIT, DEF_PSC_POST_QLIMIT, &var_psc_post_queue_limit, 5, 0,
	VAR_PSC_PRE_QLIMIT, DEF_PSC_PRE_QLIMIT, &var_psc_pre_queue_limit, 10, 0,
	VAR_PSC_CCONN_LIMIT, DEF_PSC_CCONN_LIMIT, &var_psc_cconn_limit, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_PSC_GREET_WAIT, DEF_PSC_GREET_WAIT, &var_psc_greet_wait, 1, 0,
	VAR_PSC_PREGR_TTL, DEF_PSC_PREGR_TTL, &var_psc_pregr_ttl, 1, 0,
	VAR_PSC_DNSBL_TTL, DEF_PSC_DNSBL_TTL, &var_psc_dnsbl_ttl, 1, 0,
	VAR_PSC_PIPEL_TTL, DEF_PSC_PIPEL_TTL, &var_psc_pipel_ttl, 1, 0,
	VAR_PSC_NSMTP_TTL, DEF_PSC_NSMTP_TTL, &var_psc_nsmtp_ttl, 1, 0,
	VAR_PSC_BARLF_TTL, DEF_PSC_BARLF_TTL, &var_psc_barlf_ttl, 1, 0,
	VAR_PSC_CACHE_RET, DEF_PSC_CACHE_RET, &var_psc_cache_ret, 1, 0,
	VAR_PSC_CACHE_SCAN, DEF_PSC_CACHE_SCAN, &var_psc_cache_scan, 0, 0,
	VAR_PSC_WATCHDOG, DEF_PSC_WATCHDOG, &var_psc_watchdog, 10, 0,
	VAR_PSC_UPROXY_TMOUT, DEF_PSC_UPROXY_TMOUT, &var_psc_uproxy_tmout, 1, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_HELO_REQUIRED, DEF_HELO_REQUIRED, &var_helo_required,
	VAR_DISABLE_VRFY_CMD, DEF_DISABLE_VRFY_CMD, &var_disable_vrfy_cmd,
	VAR_SMTPD_USE_TLS, DEF_SMTPD_USE_TLS, &var_smtpd_use_tls,
	VAR_SMTPD_ENFORCE_TLS, DEF_SMTPD_ENFORCE_TLS, &var_smtpd_enforce_tls,
	VAR_PSC_PIPEL_ENABLE, DEF_PSC_PIPEL_ENABLE, &var_psc_pipel_enable,
	VAR_PSC_NSMTP_ENABLE, DEF_PSC_NSMTP_ENABLE, &var_psc_nsmtp_enable,
	VAR_PSC_BARLF_ENABLE, DEF_PSC_BARLF_ENABLE, &var_psc_barlf_enable,
	0,
    };
    static const CONFIG_RAW_TABLE raw_table[] = {
	VAR_PSC_CMD_TIME, DEF_PSC_CMD_TIME, &var_psc_cmd_time, 1, 0,
	VAR_SMTPD_REJ_FOOTER, DEF_SMTPD_REJ_FOOTER, &var_smtpd_rej_footer, 0, 0,
	VAR_PSC_REJ_FOOTER, DEF_PSC_REJ_FOOTER, &var_psc_rej_footer, 0, 0,
	VAR_SMTPD_EXP_FILTER, DEF_SMTPD_EXP_FILTER, &var_smtpd_exp_filter, 1, 0,
	VAR_PSC_EXP_FILTER, DEF_PSC_EXP_FILTER, &var_psc_exp_filter, 1, 0,
	0,
    };
    static const CONFIG_NBOOL_TABLE nbool_table[] = {
	VAR_PSC_HELO_REQUIRED, DEF_PSC_HELO_REQUIRED, &var_psc_helo_required,
	VAR_PSC_DISABLE_VRFY, DEF_PSC_DISABLE_VRFY, &var_psc_disable_vrfy,
	VAR_PSC_USE_TLS, DEF_PSC_USE_TLS, &var_psc_use_tls,
	VAR_PSC_ENFORCE_TLS, DEF_PSC_ENFORCE_TLS, &var_psc_enforce_tls,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    event_server_main(argc, argv, psc_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_INT_TABLE, int_table,
		      MAIL_SERVER_NINT_TABLE, nint_table,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_BOOL_TABLE, bool_table,
		      MAIL_SERVER_RAW_TABLE, raw_table,
		      MAIL_SERVER_NBOOL_TABLE, nbool_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_PRE_ACCEPT, pre_accept,
		      MAIL_SERVER_SOLITARY,
		      MAIL_SERVER_SLOW_EXIT, psc_drain,
		      MAIL_SERVER_EXIT, psc_dump,
		      MAIL_SERVER_WATCHDOG, &var_psc_watchdog,
		      0);
}
