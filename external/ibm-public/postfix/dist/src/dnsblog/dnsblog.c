/*	$NetBSD: dnsblog.c,v 1.4 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	dnsblog 8
/* SUMMARY
/*	Postfix DNS allow/denylist logger
/* SYNOPSIS
/*	\fBdnsblog\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBdnsblog\fR(8) server implements an ad-hoc DNS
/*	allow/denylist lookup service. This may eventually be
/*	replaced by an UDP client that is built directly into the
/*	\fBpostscreen\fR(8) server.
/* PROTOCOL
/* .ad
/* .fi
/*	With each connection, the \fBdnsblog\fR(8) server receives
/*	a DNS allow/denylist domain name, an IP address, and an ID.
/*	If the IP address is listed under the DNS allow/denylist, the
/*	\fBdnsblog\fR(8) server logs the match and replies with the
/*	query arguments plus an address list with the resulting IP
/*	addresses, separated by whitespace, and the reply TTL.
/*	Otherwise it replies with the query arguments plus an empty
/*	address list and the reply TTL; the reply TTL is -1 if there
/*	is no reply, or a negative reply that contains no SOA record.
/*	Finally, the \fBdnsblog\fR(8) server closes the connection.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8)
/*	or \fBpostlogd\fR(8).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as
/*	\fBdnsblog\fR(8) processes run for only a limited amount
/*	of time. Use the command "\fBpostfix reload\fR" to speed
/*	up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBpostscreen_dnsbl_sites (empty)\fR"
/*	Optional list of DNS allow/denylist domains, filters and weight
/*	factors.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .PP
/*	Available in Postfix 3.3 and later:
/* .IP "\fBservice_name (read-only)\fR"
/*	The master.cf service name of a Postfix daemon process.
/* SEE ALSO
/*	smtpd(8), Postfix SMTP server
/*	postconf(5), configuration parameters
/*	postlogd(8), Postfix logging
/*	syslogd(8), system logging
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 2.8.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <limits.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <argv.h>
#include <myaddrinfo.h>
#include <valid_hostname.h>
#include <sock_addr.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <mail_params.h>

/* DNS library. */

#include <dns.h>

/* Server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * Tunable parameters.
  */
int     var_dnsblog_delay;

 /*
  * Static so we don't allocate and free on every request.
  */
static VSTRING *rbl_domain;
static VSTRING *addr;
static VSTRING *query;
static VSTRING *why;
static VSTRING *result;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define LEN(x)			VSTRING_LEN(x)

/* static void dnsblog_query - query DNSBL for client address */

static VSTRING *dnsblog_query(VSTRING *result, int *result_ttl,
			              const char *dnsbl_domain,
			              const char *addr)
{
    const char *myname = "dnsblog_query";
    ARGV   *octets;
    int     i;
    struct addrinfo *res;
    unsigned char *ipv6_addr;
    int     dns_status;
    DNS_RR *addr_list;
    DNS_RR *rr;
    MAI_HOSTADDR_STR hostaddr;

    if (msg_verbose)
	msg_info("%s: addr %s dnsbl_domain %s",
		 myname, addr, dnsbl_domain);

    VSTRING_RESET(query);

    /*
     * Reverse the client IPV6 address, represented as 32 hexadecimal
     * nibbles. We use the binary address to avoid tricky code. Asking for an
     * AAAA record makes no sense here. Just like with IPv4 we use the lookup
     * result as a bit mask, not as an IP address.
     */
#ifdef HAS_IPV6
    if (valid_ipv6_hostaddr(addr, DONT_GRIPE)) {
	if (hostaddr_to_sockaddr(addr, (char *) 0, 0, &res) != 0
	    || res->ai_family != PF_INET6)
	    msg_fatal("%s: unable to convert address %s", myname, addr);
	ipv6_addr = (unsigned char *) &SOCK_ADDR_IN6_ADDR(res->ai_addr);
	for (i = sizeof(SOCK_ADDR_IN6_ADDR(res->ai_addr)) - 1; i >= 0; i--)
	    vstring_sprintf_append(query, "%x.%x.",
				   ipv6_addr[i] & 0xf, ipv6_addr[i] >> 4);
	freeaddrinfo(res);
    } else
#endif

	/*
	 * Reverse the client IPV4 address, represented as four decimal octet
	 * values. We use the textual address for convenience.
	 */
    {
	octets = argv_split(addr, ".");
	for (i = octets->argc - 1; i >= 0; i--) {
	    vstring_strcat(query, octets->argv[i]);
	    vstring_strcat(query, ".");
	}
	argv_free(octets);
    }

    /*
     * Tack on the RBL domain name and query the DNS for an A record.
     */
    vstring_strcat(query, dnsbl_domain);
    dns_status = dns_lookup_x(STR(query), T_A, 0, &addr_list, (VSTRING *) 0,
			      why, (int *) 0, DNS_REQ_FLAG_NCACHE_TTL);

    /*
     * We return the lowest TTL in the response from the A record(s) if
     * found, or from the SOA record(s) if available. If the reply specifies
     * no TTL, or if the query fails, we return a TTL of -1.
     */
    VSTRING_RESET(result);
    *result_ttl = -1;
    if (dns_status == DNS_OK) {
	for (rr = addr_list; rr != 0; rr = rr->next) {
	    if (dns_rr_to_pa(rr, &hostaddr) == 0) {
		msg_warn("%s: skipping reply record type %s for query %s: %m",
			 myname, dns_strtype(rr->type), STR(query));
	    } else {
		msg_info("addr %s listed by domain %s as %s",
			 addr, dnsbl_domain, hostaddr.buf);
		if (LEN(result) > 0)
		    vstring_strcat(result, " ");
		vstring_strcat(result, hostaddr.buf);
		/* Grab the positive reply TTL. */
		if (*result_ttl < 0 || *result_ttl > rr->ttl)
		    *result_ttl = rr->ttl;
	    }
	}
	dns_rr_free(addr_list);
    } else if (dns_status == DNS_NOTFOUND) {
	if (msg_verbose)
	    msg_info("%s: addr %s not listed by domain %s",
		     myname, addr, dnsbl_domain);
	/* Grab the negative reply TTL. */
	for (rr = addr_list; rr != 0; rr = rr->next) {
	    if (rr->type == T_SOA && (*result_ttl < 0 || *result_ttl > rr->ttl))
		*result_ttl = rr->ttl;
	}
	dns_rr_free(addr_list);
    } else {
	msg_warn("%s: lookup error for DNS query %s: %s",
		 myname, STR(query), STR(why));
    }
    VSTRING_TERMINATE(result);
    return (result);
}

/* dnsblog_service - perform service for client */

static void dnsblog_service(VSTREAM *client_stream, char *unused_service,
			            char **argv)
{
    int     request_id;
    int     result_ttl;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the dnsblog service. All connection-management stuff is handled by
     * the common code in single_server.c.
     */
    if (attr_scan(client_stream,
		  ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		  RECV_ATTR_STR(MAIL_ATTR_RBL_DOMAIN, rbl_domain),
		  RECV_ATTR_STR(MAIL_ATTR_ACT_CLIENT_ADDR, addr),
		  RECV_ATTR_INT(MAIL_ATTR_LABEL, &request_id),
		  ATTR_TYPE_END) == 3) {
	(void) dnsblog_query(result, &result_ttl, STR(rbl_domain), STR(addr));
	if (var_dnsblog_delay > 0)
	    sleep(var_dnsblog_delay);
	attr_print(client_stream, ATTR_FLAG_NONE,
		   SEND_ATTR_STR(MAIL_ATTR_RBL_DOMAIN, STR(rbl_domain)),
		   SEND_ATTR_STR(MAIL_ATTR_ACT_CLIENT_ADDR, STR(addr)),
		   SEND_ATTR_INT(MAIL_ATTR_LABEL, request_id),
		   SEND_ATTR_STR(MAIL_ATTR_RBL_ADDR, STR(result)),
		   SEND_ATTR_INT(MAIL_ATTR_TTL, result_ttl),
		   ATTR_TYPE_END);
	vstream_fflush(client_stream);
    }
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{
    rbl_domain = vstring_alloc(100);
    addr = vstring_alloc(100);
    query = vstring_alloc(100);
    why = vstring_alloc(100);
    result = vstring_alloc(100);
    var_use_limit = 0;
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_DNSBLOG_DELAY, DEF_DNSBLOG_DELAY, &var_dnsblog_delay, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    single_server_main(argc, argv, dnsblog_service,
		       CA_MAIL_SERVER_TIME_TABLE(time_table),
		       CA_MAIL_SERVER_POST_INIT(post_jail_init),
		       CA_MAIL_SERVER_UNLIMITED,
		       CA_MAIL_SERVER_RETIRE_ME,
		       0);
}
