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
/*	The host to send to. For local delivery this is an empty string.
/* .IP \fIrecipient\fR
/*	The envelope recipient address that is passed on to \fInexthop\fR.
/* .PP
/*	The \fBtrivial-rewrite\fR daemon by default only distinguishes
/*	between local and non-local mail. For finer control over mail
/*	routing, use the optional \fBtransport\fR(5) lookup table.
/* .RE
/* .PP
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
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
/* .IP \fBinet_interfaces\fR
/*	The network interfaces that this mail system receives mail on.
/*	This information is used to determine if
/*	\fIuser\fR@[\fInet.work.addr.ess\fR] is local or remote.
/* .IP \fBmydestination\fR
/*	List of domains that this machine considers local.
/* .IP \fBmyorigin\fR
/*	The domain that locally-posted mail appears to come from.
/* .SH Rewriting
/* .ad
/* .fi
/* .IP \fBallow_percent_hack\fR
/*	Rewrite \fIuser\fR%\fIdomain\fR to \fIuser\fR@\fIdomain\fR.
/* .IP \fBappend_at_myorigin\fR
/*	Rewrite \fIuser\fR to \fIuser\fR@$\fBmyorigin\fR.
/* .IP \fBappend_dot_mydomain\fR
/*	Rewrite \fIuser\fR@\fIhost\fR to \fIuser\fR@\fIhost\fR.$\fBmydomain\fR.
/* .IP \fBswap_bangpath\fR
/*	Rewrite \fIsite\fR!\fIuser\fR to \fIuser\fR@\fIsite\fR.
/* .SH Routing
/* .ad
/* .fi
/* .IP \fBlocal_transport\fR
/*	Where to deliver mail for destinations that match $\fBmydestination\fR
/*	or $\fBinet_interfaces\fR. 
/*	The default transport is \fBlocal\fR.
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBdefault_transport\fR
/*	Where to deliver non-local mail when no information is explicitly
/*	given in the \fBtransport\fR(5) table.
/*	The default transport is \fBsmtp\fR.
/* .sp
/*	Syntax is \fItransport\fR:\fInexthop\fR; see \fBtransport\fR(5)
/*	for details. The :\fInexthop\fR part is optional.
/* .IP \fBrelayhost\fR
/*	The default host to send non-local mail to when no entry is matched
/*	in the \fBtransport\fR(5) table.
/* .sp
/*	When no \fBrelayhost\fR is specified, mail is routed directly
/*	to the destination's mail exchanger.
/* .IP \fBtransport_maps\fR
/*	List of tables with \fIdomain\fR to (\fItransport, nexthop\fR)
/*	mappings.
/* SEE ALSO
/*	master(8) process manager
/*	syslogd(8) system logging
/*	transport(5) transport table format
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
    if (mail_scan(stream, "%s", command) == 1) {
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

/* main - pass control to the multi-threaded skeleton code */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_TRANSPORT_MAPS, DEF_TRANSPORT_MAPS, &var_transport_maps, 0, 0,
	VAR_LOCAL_TRANSPORT, DEF_LOCAL_TRANSPORT, &var_local_transport, 0, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SWAP_BANGPATH, DEF_SWAP_BANGPATH, &var_swap_bangpath,
	VAR_APP_DOT_MYDOMAIN, DEF_APP_DOT_MYDOMAIN, &var_append_dot_mydomain,
	VAR_APP_AT_MYORIGIN, DEF_APP_AT_MYORIGIN, &var_append_at_myorigin,
	VAR_PERCENT_HACK, DEF_PERCENT_HACK, &var_percent_hack,
	0,
    };

    multi_server_main(argc, argv, rewrite_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_BOOL_TABLE, bool_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_PRE_ACCEPT, pre_accept,
		      0);
}
