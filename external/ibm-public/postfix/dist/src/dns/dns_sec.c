/*	$NetBSD: dns_sec.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	dns_sec 3
/* SUMMARY
/*	DNSSEC validation availability
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	DNS_SEC_STATS_SET(
/*	int	flags)
/*
/*	DNS_SEC_STATS_TEST(
/*	int	flags)
/*
/*	void	dns_sec_probe(
/*	int	rflags)
/* DESCRIPTION
/*	This module maintains information about the availability of
/*	DNSSEC validation, in global flags that summarize
/*	process-lifetime history.
/* .IP DNS_SEC_FLAG_AVAILABLE
/*	The process has received at least one DNSSEC validated
/*	response to a query that requested DNSSEC validation.
/* .IP DNS_SEC_FLAG_DONT_PROBE
/*	The process has sent a DNSSEC probe (see below), or DNSSEC
/*	probing is disabled by configuration.
/* .PP
/*	DNS_SEC_STATS_SET() sets one or more DNS_SEC_FLAG_* flags,
/*	and DNS_SEC_STATS_TEST() returns non-zero if any of the
/*	specified flags is set.
/*
/*	dns_sec_probe() generates a query to the target specified
/*	with the \fBdnssec_probe\fR configuration parameter. It
/*	sets the DNS_SEC_FLAG_DONT_PROBE flag, and it calls
/*	dns_lookup() which sets DNS_SEC_FLAG_AVAILABLE if it receives
/*	a DNSSEC validated response. Preconditions:
/* .IP \(bu
/*	The rflags argument must request DNSSEC validation (in the
/*	same manner as dns_lookup() rflags argument).
/* .IP \(bu
/*	The DNS_SEC_FLAG_AVAILABLE and DNS_SEC_FLAG_DONT_PROBE
/*	flags must be false.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * DNS library.
  */
#include <dns.h>

int     dns_sec_stats;

/* dns_sec_probe - send a probe to establish DNSSEC viability */

void    dns_sec_probe(int rflags)
{
    const char myname[] = "dns_sec_probe";
    char   *saved_dnssec_probe;
    char   *qname;
    int     qtype;
    DNS_RR *rrlist = 0;
    int     dns_status;
    VSTRING *why;

    /*
     * Sanity checks.
     */
    if (!DNS_WANT_DNSSEC_VALIDATION(rflags))
	msg_panic("%s: DNSSEC is not requested", myname);
    if (DNS_SEC_STATS_TEST(DNS_SEC_FLAG_DONT_PROBE))
	msg_panic("%s: DNSSEC probe was already sent, or probing is disabled",
		  myname);
    if (DNS_SEC_STATS_TEST(DNS_SEC_FLAG_AVAILABLE))
	msg_panic("%s: already have validated DNS response", myname);

    /*
     * Don't recurse.
     */
    DNS_SEC_STATS_SET(DNS_SEC_FLAG_DONT_PROBE);

    /*
     * Don't probe.
     */
    if (*var_dnssec_probe == 0)
	return;

    /*
     * Parse the probe spec. Format is type:resource.
     */
    saved_dnssec_probe = mystrdup(var_dnssec_probe);
    if ((qname = split_at(saved_dnssec_probe, ':')) == 0 || *qname == 0
	|| (qtype = dns_type(saved_dnssec_probe)) == 0)
	msg_fatal("malformed %s value: %s format is qtype:qname",
		  VAR_DNSSEC_PROBE, var_dnssec_probe);

    why = vstring_alloc(100);
    dns_status = dns_lookup(qname, qtype, rflags, &rrlist, (VSTRING *) 0, why);
    if (!DNS_SEC_STATS_TEST(DNS_SEC_FLAG_AVAILABLE))
	msg_warn("DNSSEC validation may be unavailable");
    else if (msg_verbose)
	msg_info(VAR_DNSSEC_PROBE
		 " '%s' received a response that is DNSSEC validated",
		 var_dnssec_probe);
    switch (dns_status) {
    default:
	if (!DNS_SEC_STATS_TEST(DNS_SEC_FLAG_AVAILABLE))
	    msg_warn("reason: " VAR_DNSSEC_PROBE
		     " '%s' received a response that is not DNSSEC validated",
		     var_dnssec_probe);
	if (rrlist)
	    dns_rr_free(rrlist);
	break;
    case DNS_RETRY:
    case DNS_FAIL:
	msg_warn("reason: " VAR_DNSSEC_PROBE " '%s' received no response: %s",
		 var_dnssec_probe, vstring_str(why));
	break;
    }
    myfree(saved_dnssec_probe);
    vstring_free(why);
}
