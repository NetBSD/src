/*++
/* NAME
/*	trivial-rewrite 8
/* SUMMARY
/*	Postfix address rewriting and resolving daemon
/* SYNOPSIS
/*	\fBtrivial-rewrite\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBtrivial-rewrite\fR daemon processes two types of client
/*	service requests:
/* .IP \fBrewrite\fR
/*	Rewrite an address to standard form. The \fBtrivial-rewrite\fR
/*	daemon by default appends local domain information to unqualified
/*	addresses, swaps bang paths to domain form, and strips source
/*	routing information. This process is under control of several
/*	configuration parameters (see below).
/* .IP \fBresolve\fR
/*	Resolve an address to a (\fItransport\fR, \fInexthop\fR,
/*	\fIrecipient\fR) triple. The meaning of the results is as follows:
/* .RS
/* .IP \fItransport\fR
/*	The delivery agent to use. This is the first field of an entry
/*	in the \fBmaster.cf\fR file.
/* .IP \fInexthop\fR
/*	The host to send to and optional delivery method information.
/* .IP \fIrecipient\fR
/*	The envelope recipient address that is passed on to \fInexthop\fR.
/* .RE
/* DEFAULT DELIVERY METHODS
/* .ad
/* .fi
/*	By default, Postfix uses one of the following delivery methods.
/*	This may be overruled with the optional transport(5) table.
/*	The default delivery method is selected by matching the
/*	recipient address domain against one of the following:
/* .IP \fB$mydestination\fR
/* .IP \fB$inet_interfaces\fR
/*	The transport and optional nexthop
/*	are specified with \fB$local_transport\fR.
/*	The default nexthop is the recipient domain.
/* .IP \fB$virtual_alias_domains\fR
/*	The recipient address is undeliverable (user unknown).
/*	By definition, all known addresses in a virtual alias domain
/*	are aliased to other addresses.
/* .IP \fB$virtual_mailbox_domains\fR
/*	The transport and optional nexthop are specified with
/*	\fB$virtual_transport\fR.
/*	The default nexthop is the recipient domain.
/* .IP \fB$relay_domains\fR
/*	The transport and optional nexthop are specified with
/*	\fB$relay_transport\fR. This overrides the optional nexthop
/*	information that is specified with \fB$relayhost\fR.
/*	The default nexthop is the recipient domain.
/* .IP "none of the above"
/*	The transport and optional nexthop are specified with
/*	\fB$default_transport\fR.
/*	This overrides the optional nexthop information that is specified
/*	with \fB$relayhost\fR.
/*	The default nexthop is the recipient domain.
/* SERVER PROCESS MANAGEMENT
/* .ad
/* .fi
/*	The trivial-rewrite servers run under control by the Postfix master
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
/*	The \fBtrivial-rewrite\fR daemon is not security sensitive.
/*	By default, this daemon does not talk to remote or local users.
/*	It can run at a fixed low privilege in a chrooted environment.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
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
/* .IP \fBempty_address_recipient\fR
/*	The recipient that is substituted for the null address.
/* .IP \fBinet_interfaces\fR
/*	The network interfaces that this mail system receives mail on.
/*	This information is used to determine if
/*	\fIuser\fR@[\fInet.work.addr.ess\fR] is local or remote.
/*	Mail for local users is given to the \fB$local_transport\fR.
/* .IP \fBmydestination\fR
/*	List of domains that are given to the \fB$local_transport\fR.
/* .IP \fBvirtual_alias_domains\fR
/*	List of simulated virtual domains (domains with all recipients
/*	aliased to some other local or remote domain).
/* .IP \fBvirtual_mailbox_domains\fR
/*	List of domains that are given to the \fB$virtual_transport\fR.
/* .IP \fBrelay_domains\fR
/*	List of domains that are given to the \fB$relay_transport\fR.
/* .IP \fBresolve_unquoted_address\fR
/*	When resolving an address, do not quote the address localpart as
/*	per RFC 822, so that additional \fB@\fR, \fB%\fR or \fB!\fR
/*	characters remain visible. This is technically incorrect, but
/*	allows us to stop relay attacks when forwarding mail to a Sendmail
/*	primary MX host.
/* .IP \fBrelocated_maps\fR
/*      Tables with contact information for users, hosts or domains
/*      that no longer exist. See \fBrelocated\fR(5).
/* .SH Rewriting
/* .ad
/* .fi
/* .IP \fBmyorigin\fR
/*	The domain that locally-posted mail appears to come from.
/* .IP \fBallow_percent_hack\fR
/*	Rewrite \fIuser\fR%\fIdomain\fR to \fIuser\fR@\fIdomain\fR.
/* .IP \fBappend_at_myorigin\fR
/*	Rewrite \fIuser\fR to \fIuser\fR@\fB$myorigin\fR.
/* .IP \fBappend_dot_mydomain\fR
/*	Rewrite \fIuser\fR@\fIhost\fR to \fIuser\fR@\fIhost\fR.\fB$mydomain\fR.
/* .IP \fBswap_bangpath\fR
/*	Rewrite \fIsite\fR!\fIuser\fR to \fIuser\fR@\fIsite\fR.
/* .SH Routing
/* .ad
/* .fi
/* .IP \fBlocal_transport\fR
/*	Where to deliver mail for destinations that match \fB$mydestination\fR
/*	or \fB$inet_interfaces\fR.
/*	The default transport is \fBlocal:$myhostname\fR.
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBvirtual_transport\fR
/*	Where to deliver mail for non-local domains that match
/*	\fB$virtual_mailbox_domains\fR.
/*	The default transport is \fBvirtual\fR.
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBrelay_transport\fR
/*	Where to deliver mail for non-local domains that match
/*	\fB$relay_domains\fR.
/*	The default transport is \fBrelay\fR (which normally is a clone
/*	of the \fBsmtp\fR transport).
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBdefault_transport\fR
/*	Where to deliver all other non-local mail.
/*	The default transport is \fBsmtp\fR.
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBparent_domain_matches_subdomains\fR
/*	List of Postfix features that use \fIdomain.tld\fR patterns
/*	to match \fIsub.domain.tld\fR (as opposed to
/*	requiring \fI.domain.tld\fR patterns).
/* .IP \fBrelayhost\fR
/*	The default host to send non-local mail to when no host is
/*	specified with \fB$relay_transport\fR or \fB$default_transport\fR,
/*	and when the recipient address does not match the optional the
/*	\fBtransport\fR(5) table.
/* .IP \fBtransport_maps\fR
/*	List of tables with \fIrecipient\fR or \fIdomain\fR to
/*	(\fItransport, nexthop\fR) mappings.
/* SEE ALSO
/*	master(8) process manager
/*	syslogd(8) system logging
/*	transport(5) transport table format
/*	relocated(5) format of the "user has moved" table
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

/* Global library. */

#include <mail_params.h>
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
char   *var_empty_addr;
int     var_show_unk_rcpt_table;

/* rewrite_service - read request and send reply */

static void rewrite_service(VSTREAM *stream, char *unused_service, char **argv)
{
    int     status = -1;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

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
	} else if (strcmp(vstring_str(command), RESOLVE_ADDR) == 0) {
	    status = resolve_proto(stream);
	} else {
	    msg_warn("bad command %.30s", printable(vstring_str(command), '?'));
	}
    }
    if (status < 0)
	multi_server_disconnect(stream);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* pre_jail_init - initialize before entering chroot jail */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    command = vstring_alloc(100);
    rewrite_init();
    resolve_init();
    transport_init();
}

static void post_jail_init(char *unused_name, char **unused_argv)
{
    transport_wildcard_init();
}

/* main - pass control to the multi-threaded skeleton code */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_TRANSPORT_MAPS, DEF_TRANSPORT_MAPS, &var_transport_maps, 0, 0,
	VAR_LOCAL_TRANSPORT, DEF_LOCAL_TRANSPORT, &var_local_transport, 1, 0,
	VAR_VIRT_TRANSPORT, DEF_VIRT_TRANSPORT, &var_virt_transport, 1, 0,
	VAR_RELAY_TRANSPORT, DEF_RELAY_TRANSPORT, &var_relay_transport, 1, 0,
	VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps, 0, 0,
	VAR_VIRT_ALIAS_DOMS, DEF_VIRT_ALIAS_DOMS, &var_virt_alias_doms, 0, 0,
	VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps, 0, 0,
	VAR_VIRT_MAILBOX_DOMS, DEF_VIRT_MAILBOX_DOMS, &var_virt_mailbox_doms, 0, 0,
	VAR_DEF_TRANSPORT, DEF_DEF_TRANSPORT, &var_def_transport, 1, 0,
	VAR_VIRT_TRANSPORT, DEF_VIRT_TRANSPORT, &var_virt_transport, 1, 0,
	VAR_RELAY_TRANSPORT, DEF_RELAY_TRANSPORT, &var_relay_transport, 1, 0,
	VAR_RELOCATED_MAPS, DEF_RELOCATED_MAPS, &var_relocated_maps, 0, 0,
	VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SWAP_BANGPATH, DEF_SWAP_BANGPATH, &var_swap_bangpath,
	VAR_APP_DOT_MYDOMAIN, DEF_APP_DOT_MYDOMAIN, &var_append_dot_mydomain,
	VAR_APP_AT_MYORIGIN, DEF_APP_AT_MYORIGIN, &var_append_at_myorigin,
	VAR_PERCENT_HACK, DEF_PERCENT_HACK, &var_percent_hack,
	VAR_RESOLVE_DEQUOTED, DEF_RESOLVE_DEQUOTED, &var_resolve_dequoted,
	VAR_SHOW_UNK_RCPT_TABLE, DEF_SHOW_UNK_RCPT_TABLE, &var_show_unk_rcpt_table,
	0,
    };

    multi_server_main(argc, argv, rewrite_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_BOOL_TABLE, bool_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_PRE_ACCEPT, pre_accept,
		      0);
}
