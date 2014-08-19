/*	$NetBSD: trivial-rewrite.c,v 1.1.1.2.12.1 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	trivial-rewrite 8
/* SUMMARY
/*	Postfix address rewriting and resolving daemon
/* SYNOPSIS
/*	\fBtrivial-rewrite\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBtrivial-rewrite\fR(8) daemon processes three types of client
/*	service requests:
/* .IP "\fBrewrite \fIcontext address\fR"
/*	Rewrite an address to standard form, according to the
/*	address rewriting context:
/* .RS
/* .IP \fBlocal\fR
/*	Append the domain names specified with \fB$myorigin\fR or
/*	\fB$mydomain\fR to incomplete addresses; do \fBswap_bangpath\fR
/*	and \fBallow_percent_hack\fR processing as described below, and
/*	strip source routed addresses (\fI@site,@site:user@domain\fR)
/*	to \fIuser@domain\fR form.
/* .IP \fBremote\fR
/*	Append the domain name specified with
/*	\fB$remote_header_rewrite_domain\fR to incomplete
/*	addresses. Otherwise the result is identical to that of
/*	the \fBlocal\fR address rewriting context. This prevents
/*      Postfix from appending the local domain to spam from poorly
/*	written remote clients.
/* .RE
/* .IP "\fBresolve \fIsender\fR \fIaddress\fR"
/*	Resolve the address to a (\fItransport\fR, \fInexthop\fR,
/*      \fIrecipient\fR, \fIflags\fR) quadruple. The meaning of
/*	the results is as follows:
/* .RS
/* .IP \fItransport\fR
/*	The delivery agent to use. This is the first field of an entry
/*	in the \fBmaster.cf\fR file.
/* .IP \fInexthop\fR
/*	The host to send to and optional delivery method information.
/* .IP \fIrecipient\fR
/*	The envelope recipient address that is passed on to \fInexthop\fR.
/* .IP \fIflags\fR
/*	The address class, whether the address requires relaying,
/*	whether the address has problems, and whether the request failed.
/* .RE
/* .IP "\fBverify \fIsender\fR \fIaddress\fR"
/*	Resolve the address for address verification purposes.
/* SERVER PROCESS MANAGEMENT
/* .ad
/* .fi
/*	The \fBtrivial-rewrite\fR(8) servers run under control by
/*	the Postfix master
/*	server.  Each server can handle multiple simultaneous connections.
/*	When all servers are busy while a client connects, the master
/*	creates a new server process, provided that the trivial-rewrite
/*	server process limit is not exceeded.
/*	Each trivial-rewrite server terminates after
/*	serving at least \fB$max_use\fR clients of after \fB$max_idle\fR
/*	seconds of idle time.
/* STANDARDS
/* .ad
/* .fi
/*	None. The command does not interact with the outside world.
/* SECURITY
/* .ad
/* .fi
/*	The \fBtrivial-rewrite\fR(8) daemon is not security sensitive.
/*	By default, this daemon does not talk to remote or local users.
/*	It can run at a fixed low privilege in a chrooted environment.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	On busy mail systems a long time may pass before a \fBmain.cf\fR
/*	change affecting \fBtrivial-rewrite\fR(8) is picked up. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBresolve_dequoted_address (yes)\fR"
/*	Resolve a recipient address safely instead of correctly, by
/*	looking inside quotes.
/* .PP
/*	Available with Postfix version 2.1 and later:
/* .IP "\fBresolve_null_domain (no)\fR"
/*	Resolve an address that ends in the "@" null domain as if the
/*	local hostname were specified, instead of rejecting the address as
/*	invalid.
/* .PP
/*	Available with Postfix version 2.3 and later:
/* .IP "\fBresolve_numeric_domain (no)\fR"
/*	Resolve "user@ipaddress" as "user@[ipaddress]", instead of
/*	rejecting the address as invalid.
/* .PP
/*	Available with Postfix version 2.5 and later:
/* .IP "\fBallow_min_user (no)\fR"
/*	Allow a sender or recipient address to have `-' as the first
/*	character.
/* ADDRESS REWRITING CONTROLS
/* .ad
/* .fi
/* .IP "\fBmyorigin ($myhostname)\fR"
/*	The domain name that locally-posted mail appears to come
/*	from, and that locally posted mail is delivered to.
/* .IP "\fBallow_percent_hack (yes)\fR"
/*	Enable the rewriting of the form "user%domain" to "user@domain".
/* .IP "\fBappend_at_myorigin (yes)\fR"
/*	With locally submitted mail, append the string "@$myorigin" to mail
/*	addresses without domain information.
/* .IP "\fBappend_dot_mydomain (yes)\fR"
/*	With locally submitted mail, append the string ".$mydomain" to
/*	addresses that have no ".domain" information.
/* .IP "\fBrecipient_delimiter (empty)\fR"
/*	The set of characters that can separate a user name from its
/*	extension (example: user+foo), or a .forward file name from its
/*	extension (example: .forward+foo).
/* .IP "\fBswap_bangpath (yes)\fR"
/*	Enable the rewriting of "site!user" into "user@site".
/* .PP
/*	Available in Postfix 2.2 and later:
/* .IP "\fBremote_header_rewrite_domain (empty)\fR"
/*	Don't rewrite message headers from remote clients at all when
/*	this parameter is empty; otherwise, rewrite message headers and
/*	append the specified domain name to incomplete addresses.
/* ROUTING CONTROLS
/* .ad
/* .fi
/*	The following is applicable to Postfix version 2.0 and later.
/*	Earlier versions do not have support for: virtual_transport,
/*	relay_transport, virtual_alias_domains, virtual_mailbox_domains
/*	or proxy_interfaces.
/* .IP "\fBlocal_transport (local:$myhostname)\fR"
/*	The default mail delivery transport and next-hop destination
/*	for final delivery to domains listed with mydestination, and for
/*	[ipaddress] destinations that match $inet_interfaces or $proxy_interfaces.
/* .IP "\fBvirtual_transport (virtual)\fR"
/*	The default mail delivery transport and next-hop destination for
/*	final delivery to domains listed with $virtual_mailbox_domains.
/* .IP "\fBrelay_transport (relay)\fR"
/*	The default mail delivery transport and next-hop destination for
/*	remote delivery to domains listed with $relay_domains.
/* .IP "\fBdefault_transport (smtp)\fR"
/*	The default mail delivery transport and next-hop destination for
/*	destinations that do not match $mydestination, $inet_interfaces,
/*	$proxy_interfaces, $virtual_alias_domains, $virtual_mailbox_domains,
/*	or $relay_domains.
/* .IP "\fBparent_domain_matches_subdomains (see 'postconf -d' output)\fR"
/*	What Postfix features match subdomains of "domain.tld" automatically,
/*	instead of requiring an explicit ".domain.tld" pattern.
/* .IP "\fBrelayhost (empty)\fR"
/*	The next-hop destination of non-local mail; overrides non-local
/*	domains in recipient addresses.
/* .IP "\fBtransport_maps (empty)\fR"
/*	Optional lookup tables with mappings from recipient address to
/*	(message delivery transport, next-hop destination).
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsender_dependent_relayhost_maps (empty)\fR"
/*	A sender-dependent override for the global relayhost parameter
/*	setting.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBempty_address_relayhost_maps_lookup_key (<>)\fR"
/*	The sender_dependent_relayhost_maps search string that will be
/*	used instead of the null sender address.
/* .PP
/*	Available in Postfix version 2.7 and later:
/* .IP "\fBempty_address_default_transport_maps_lookup_key (<>)\fR"
/*	The sender_dependent_default_transport_maps search string that
/*	will be used instead of the null sender address.
/* .IP "\fBsender_dependent_default_transport_maps (empty)\fR"
/*	A sender-dependent override for the global default_transport
/*	parameter setting.
/* ADDRESS VERIFICATION CONTROLS
/* .ad
/* .fi
/*	Postfix version 2.1 introduces sender and recipient address verification.
/*	This feature is implemented by sending probe email messages that
/*	are not actually delivered.
/*	By default, address verification probes use the same route
/*	as regular mail. To override specific aspects of message
/*	routing for address verification probes, specify one or more
/*	of the following:
/* .IP "\fBaddress_verify_local_transport ($local_transport)\fR"
/*	Overrides the local_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_virtual_transport ($virtual_transport)\fR"
/*	Overrides the virtual_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_relay_transport ($relay_transport)\fR"
/*	Overrides the relay_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_default_transport ($default_transport)\fR"
/*	Overrides the default_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_relayhost ($relayhost)\fR"
/*	Overrides the relayhost parameter setting for address verification
/*	probes.
/* .IP "\fBaddress_verify_transport_maps ($transport_maps)\fR"
/*	Overrides the transport_maps parameter setting for address verification
/*	probes.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBaddress_verify_sender_dependent_relayhost_maps ($sender_dependent_relayhost_maps)\fR"
/*	Overrides the sender_dependent_relayhost_maps parameter setting for address
/*	verification probes.
/* .PP
/*	Available in Postfix version 2.7 and later:
/* .IP "\fBaddress_verify_sender_dependent_default_transport_maps ($sender_dependent_default_transport_maps)\fR"
/*	Overrides the sender_dependent_default_transport_maps parameter
/*	setting for address verification probes.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBempty_address_recipient (MAILER-DAEMON)\fR"
/*	The recipient of mail addressed to the null address.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBrelocated_maps (empty)\fR"
/*	Optional lookup tables with new contact information for users or
/*	domains that no longer exist.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBshow_user_unknown_table_name (yes)\fR"
/*	Display the name of the recipient table in the "User unknown"
/*	responses.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBhelpful_warnings (yes)\fR"
/*	Log warnings about problematic configuration settings, and provide
/*	helpful suggestions.
/* SEE ALSO
/*	postconf(5), configuration parameters
/*	transport(5), transport table format
/*	relocated(5), format of the "user has moved" table
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	ADDRESS_CLASS_README, Postfix address classes howto
/*	ADDRESS_VERIFICATION_README, Postfix address verification
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <split_at.h>
#include <stringops.h>
#include <dict.h>
#include <events.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <resolve_local.h>
#include <mail_conf.h>
#include <resolve_clnt.h>
#include <rewrite_clnt.h>
#include <tok822.h>
#include <mail_addr.h>

/* Multi server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include <trivial-rewrite.h>
#include <transport.h>

static VSTRING *command;

 /*
  * Tunable parameters.
  */
char   *var_transport_maps;
bool    var_swap_bangpath;
bool    var_append_dot_mydomain;
bool    var_append_at_myorigin;
bool    var_percent_hack;
char   *var_local_transport;
char   *var_virt_transport;
char   *var_relay_transport;
int     var_resolve_dequoted;
char   *var_virt_alias_maps;		/* XXX virtual_alias_domains */
char   *var_virt_mailbox_maps;		/* XXX virtual_mailbox_domains */
char   *var_virt_alias_doms;
char   *var_virt_mailbox_doms;
char   *var_relocated_maps;
char   *var_def_transport;
char   *var_snd_def_xport_maps;
char   *var_empty_addr;
int     var_show_unk_rcpt_table;
int     var_resolve_nulldom;
char   *var_remote_rwr_domain;
char   *var_snd_relay_maps;
char   *var_null_relay_maps_key;
char   *var_null_def_xport_maps_key;
int     var_resolve_num_dom;
bool    var_allow_min_user;

 /*
  * Shadow personality for address verification.
  */
char   *var_vrfy_xport_maps;
char   *var_vrfy_local_xport;
char   *var_vrfy_virt_xport;
char   *var_vrfy_relay_xport;
char   *var_vrfy_def_xport;
char   *var_vrfy_snd_def_xport_maps;
char   *var_vrfy_relayhost;
char   *var_vrfy_relay_maps;

 /*
  * Different resolver personalities depending on the kind of request.
  */
RES_CONTEXT resolve_regular = {
    VAR_LOCAL_TRANSPORT, &var_local_transport,
    VAR_VIRT_TRANSPORT, &var_virt_transport,
    VAR_RELAY_TRANSPORT, &var_relay_transport,
    VAR_DEF_TRANSPORT, &var_def_transport,
    VAR_SND_DEF_XPORT_MAPS, &var_snd_def_xport_maps, 0,
    VAR_RELAYHOST, &var_relayhost,
    VAR_SND_RELAY_MAPS, &var_snd_relay_maps, 0,
    VAR_TRANSPORT_MAPS, &var_transport_maps, 0
};

RES_CONTEXT resolve_verify = {
    VAR_VRFY_LOCAL_XPORT, &var_vrfy_local_xport,
    VAR_VRFY_VIRT_XPORT, &var_vrfy_virt_xport,
    VAR_VRFY_RELAY_XPORT, &var_vrfy_relay_xport,
    VAR_VRFY_DEF_XPORT, &var_vrfy_def_xport,
    VAR_VRFY_SND_DEF_XPORT_MAPS, &var_vrfy_snd_def_xport_maps, 0,
    VAR_VRFY_RELAYHOST, &var_vrfy_relayhost,
    VAR_VRFY_RELAY_MAPS, &var_vrfy_relay_maps, 0,
    VAR_VRFY_XPORT_MAPS, &var_vrfy_xport_maps, 0
};

 /*
  * Connection management. When file-based lookup tables change we should
  * restart at our convenience, but avoid client read errors. We restart
  * rather than reopen, because the process may be chrooted (and if it isn't
  * we still need code that handles the chrooted case anyway).
  * 
  * Three variants are implemented. Only one should be used.
  * 
  * ifdef DETACH_AND_ASK_CLIENTS_TO_RECONNECT
  * 
  * This code detaches the trivial-rewrite process from the master, stops
  * accepting new clients, and handles established clients in the background,
  * asking them to reconnect the next time they send a request. The master
  * creates a new process that accepts connections. This is reasonably safe
  * because the number of trivial-rewrite server processes is small compared
  * to the number of trivial-rewrite client processes. The few extra
  * background processes should not make a difference in Postfix's footprint.
  * However, once a daemon detaches from the master, its exit status will be
  * lost, and abnormal termination may remain undetected. Timely restart is
  * achieved by checking the table changed status every 10 seconds or so
  * before responding to a client request.
  * 
  * ifdef CHECK_TABLE_STATS_PERIODICALLY
  * 
  * This code runs every 10 seconds and terminates the process when lookup
  * tables have changed. This is subject to race conditions when established
  * clients send a request while the server exits; those clients may read EOF
  * instead of a server reply. If the experience with the oldest option
  * (below) is anything to go by, however, then this is unlikely to be a
  * problem during real deployment.
  * 
  * ifdef CHECK_TABLE_STATS_BEFORE_ACCEPT
  * 
  * This is the old code. It checks the table changed status when a new client
  * connects (i.e. before the server calls accept()), and terminates
  * immediately. This is invisible for the connecting client, but is subject
  * to race conditions when established clients send a request while the
  * server exits; those clients may read EOF instead of a server reply. This
  * has, however, not been a problem in real deployment. With the old code,
  * timely restart is achieved by setting the ipc_ttl parameter to 60
  * seconds, so that the table change status is checked several times a
  * minute.
  */
int     server_flags;

 /*
  * Define exactly one of these.
  */
/* #define DETACH_AND_ASK_CLIENTS_TO_RECONNECT	/* correct and complex */
#define CHECK_TABLE_STATS_PERIODICALLY	/* quick */
/* #define CHECK_TABLE_STATS_BEFORE_ACCEPT	/* slow */

/* rewrite_service - read request and send reply */

static void rewrite_service(VSTREAM *stream, char *unused_service, char **argv)
{
    int     status = -1;

#ifdef DETACH_AND_ASK_CLIENTS_TO_RECONNECT
    static time_t last;
    time_t  now;
    const char *table;

#endif

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Client connections are long-lived. Be sure to refesh timely.
     */
#ifdef DETACH_AND_ASK_CLIENTS_TO_RECONNECT
    if (server_flags == 0 && (now = event_time()) - last > 10) {
	if ((table = dict_changed_name()) != 0) {
	    msg_info("table %s has changed -- restarting", table);
	    if (multi_server_drain() == 0)
		server_flags = 1;
	}
	last = now;
    }
#endif

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to address rewriting. All connection-management stuff is
     * handled by the common code in multi_server.c.
     */
    if (attr_scan(stream, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
		  ATTR_TYPE_STR, MAIL_ATTR_REQ, command,
		  ATTR_TYPE_END) == 1) {
	if (strcmp(vstring_str(command), REWRITE_ADDR) == 0) {
	    status = rewrite_proto(stream);
	} else if (strcmp(vstring_str(command), RESOLVE_REGULAR) == 0) {
	    status = resolve_proto(&resolve_regular, stream);
	} else if (strcmp(vstring_str(command), RESOLVE_VERIFY) == 0) {
	    status = resolve_proto(&resolve_verify, stream);
	} else {
	    msg_warn("bad command %.30s", printable(vstring_str(command), '?'));
	}
    }
    if (status < 0)
	multi_server_disconnect(stream);
}

/* pre_accept - see if tables have changed */

#ifdef CHECK_TABLE_STATS_BEFORE_ACCEPT

static void pre_accept(char *unused_name, char **unused_argv)
{
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
	exit(0);
    }
}

#endif

static void check_table_stats(int unused_event, char *unused_context)
{
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
	exit(0);
    }
    event_request_timer(check_table_stats, (char *) 0, 10);
}

/* pre_jail_init - initialize before entering chroot jail */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    command = vstring_alloc(100);
    rewrite_init();
    resolve_init();
    if (*RES_PARAM_VALUE(resolve_regular.transport_maps))
	resolve_regular.transport_info =
	    transport_pre_init(resolve_regular.transport_maps_name,
			   RES_PARAM_VALUE(resolve_regular.transport_maps));
    if (*RES_PARAM_VALUE(resolve_verify.transport_maps))
	resolve_verify.transport_info =
	    transport_pre_init(resolve_verify.transport_maps_name,
			    RES_PARAM_VALUE(resolve_verify.transport_maps));
    if (*RES_PARAM_VALUE(resolve_regular.snd_relay_maps))
	resolve_regular.snd_relay_info =
	    maps_create(resolve_regular.snd_relay_maps_name,
			RES_PARAM_VALUE(resolve_regular.snd_relay_maps),
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
			| DICT_FLAG_NO_REGSUB);
    if (*RES_PARAM_VALUE(resolve_verify.snd_relay_maps))
	resolve_verify.snd_relay_info =
	    maps_create(resolve_verify.snd_relay_maps_name,
			RES_PARAM_VALUE(resolve_verify.snd_relay_maps),
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
			| DICT_FLAG_NO_REGSUB);
    if (*RES_PARAM_VALUE(resolve_regular.snd_def_xp_maps))
	resolve_regular.snd_def_xp_info =
	    maps_create(resolve_regular.snd_def_xp_maps_name,
			RES_PARAM_VALUE(resolve_regular.snd_def_xp_maps),
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
			| DICT_FLAG_NO_REGSUB);
    if (*RES_PARAM_VALUE(resolve_verify.snd_def_xp_maps))
	resolve_verify.snd_def_xp_info =
	    maps_create(resolve_verify.snd_def_xp_maps_name,
			RES_PARAM_VALUE(resolve_verify.snd_def_xp_maps),
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
			| DICT_FLAG_NO_REGSUB);
}

/* post_jail_init - initialize after entering chroot jail */

static void post_jail_init(char *unused_name, char **unused_argv)
{
    if (resolve_regular.transport_info)
	transport_post_init(resolve_regular.transport_info);
    if (resolve_verify.transport_info)
	transport_post_init(resolve_verify.transport_info);
    check_table_stats(0, (char *) 0);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton code */

int     main(int argc, char **argv)
{
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TRANSPORT_MAPS, DEF_TRANSPORT_MAPS, &var_transport_maps, 0, 0,
	VAR_LOCAL_TRANSPORT, DEF_LOCAL_TRANSPORT, &var_local_transport, 1, 0,
	VAR_VIRT_TRANSPORT, DEF_VIRT_TRANSPORT, &var_virt_transport, 1, 0,
	VAR_RELAY_TRANSPORT, DEF_RELAY_TRANSPORT, &var_relay_transport, 1, 0,
	VAR_DEF_TRANSPORT, DEF_DEF_TRANSPORT, &var_def_transport, 1, 0,
	VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps, 0, 0,
	VAR_VIRT_ALIAS_DOMS, DEF_VIRT_ALIAS_DOMS, &var_virt_alias_doms, 0, 0,
	VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps, 0, 0,
	VAR_VIRT_MAILBOX_DOMS, DEF_VIRT_MAILBOX_DOMS, &var_virt_mailbox_doms, 0, 0,
	VAR_RELOCATED_MAPS, DEF_RELOCATED_MAPS, &var_relocated_maps, 0, 0,
	VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
	VAR_VRFY_XPORT_MAPS, DEF_VRFY_XPORT_MAPS, &var_vrfy_xport_maps, 0, 0,
	VAR_VRFY_LOCAL_XPORT, DEF_VRFY_LOCAL_XPORT, &var_vrfy_local_xport, 1, 0,
	VAR_VRFY_VIRT_XPORT, DEF_VRFY_VIRT_XPORT, &var_vrfy_virt_xport, 1, 0,
	VAR_VRFY_RELAY_XPORT, DEF_VRFY_RELAY_XPORT, &var_vrfy_relay_xport, 1, 0,
	VAR_VRFY_DEF_XPORT, DEF_VRFY_DEF_XPORT, &var_vrfy_def_xport, 1, 0,
	VAR_VRFY_RELAYHOST, DEF_VRFY_RELAYHOST, &var_vrfy_relayhost, 0, 0,
	VAR_REM_RWR_DOMAIN, DEF_REM_RWR_DOMAIN, &var_remote_rwr_domain, 0, 0,
	VAR_SND_RELAY_MAPS, DEF_SND_RELAY_MAPS, &var_snd_relay_maps, 0, 0,
	VAR_NULL_RELAY_MAPS_KEY, DEF_NULL_RELAY_MAPS_KEY, &var_null_relay_maps_key, 1, 0,
	VAR_VRFY_RELAY_MAPS, DEF_VRFY_RELAY_MAPS, &var_vrfy_relay_maps, 0, 0,
	VAR_SND_DEF_XPORT_MAPS, DEF_SND_DEF_XPORT_MAPS, &var_snd_def_xport_maps, 0, 0,
	VAR_NULL_DEF_XPORT_MAPS_KEY, DEF_NULL_DEF_XPORT_MAPS_KEY, &var_null_def_xport_maps_key, 1, 0,
	VAR_VRFY_SND_DEF_XPORT_MAPS, DEF_VRFY_SND_DEF_XPORT_MAPS, &var_vrfy_snd_def_xport_maps, 0, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SWAP_BANGPATH, DEF_SWAP_BANGPATH, &var_swap_bangpath,
	VAR_APP_DOT_MYDOMAIN, DEF_APP_DOT_MYDOMAIN, &var_append_dot_mydomain,
	VAR_APP_AT_MYORIGIN, DEF_APP_AT_MYORIGIN, &var_append_at_myorigin,
	VAR_PERCENT_HACK, DEF_PERCENT_HACK, &var_percent_hack,
	VAR_RESOLVE_DEQUOTED, DEF_RESOLVE_DEQUOTED, &var_resolve_dequoted,
	VAR_SHOW_UNK_RCPT_TABLE, DEF_SHOW_UNK_RCPT_TABLE, &var_show_unk_rcpt_table,
	VAR_RESOLVE_NULLDOM, DEF_RESOLVE_NULLDOM, &var_resolve_nulldom,
	VAR_RESOLVE_NUM_DOM, DEF_RESOLVE_NUM_DOM, &var_resolve_num_dom,
	VAR_ALLOW_MIN_USER, DEF_ALLOW_MIN_USER, &var_allow_min_user,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    multi_server_main(argc, argv, rewrite_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_BOOL_TABLE, bool_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_POST_INIT, post_jail_init,
#ifdef CHECK_TABLE_STATS_BEFORE_ACCEPT
		      MAIL_SERVER_PRE_ACCEPT, pre_accept,
#endif
		      0);
}
