/*	$NetBSD: smtp_addr.c,v 1.4.2.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	smtp_addr 3
/* SUMMARY
/*	SMTP server address lookup
/* SYNOPSIS
/*	#include "smtp_addr.h"
/*
/*	DNS_RR *smtp_domain_addr(name, mxrr, misc_flags, why, found_myself)
/*	char	*name;
/*	DNS_RR  **mxrr;
/*	int	misc_flags;
/*	DSN_BUF	*why;
/*	int	*found_myself;
/*
/*	DNS_RR *smtp_host_addr(name, misc_flags, why)
/*	char	*name;
/*	int	misc_flags;
/*	DSN_BUF	*why;
/*
/*	DNS_RR	*smtp_service_addr(name, service, mxrr, misc_flags, why,
/*					found_myself)
/*	const char *name;
/*	const char *service;
/*	DNS_RR  **mxrr;
/*	int	misc_flags;
/*	DSN_BUF	*why;
/*	int	*found_myself;
/* DESCRIPTION
/*	This module implements Internet address lookups. By default,
/*	lookups are done via the Internet domain name service (DNS).
/*	A reasonable number of CNAME indirections is permitted. When
/*	DNS lookups are disabled, host address lookup is done with
/*	getnameinfo() or gethostbyname().
/*
/*	smtp_domain_addr() looks up the network addresses for mail
/*	exchanger hosts listed for the named domain. Addresses are
/*	returned in most-preferred first order. The result is truncated
/*	so that it contains only hosts that are more preferred than the
/*	local mail server itself. The found_myself result parameter
/*	is updated when the local MTA is MX host for the specified
/*	destination.  If MX records were found, the rname, qname,
/*	and dnssec validation status of the MX RRset are returned
/*	via mxrr, which the caller must free with dns_rr_free().
/*	Fallback from MX to address lookups is governed by RFC 2821,
/*	and by local policy (var_ign_mx_lookup_err).
/*
/*	When no mail exchanger is listed in the DNS for \fIname\fR, the
/*	request is passed to smtp_host_addr().
/*
/*	It is an error to call smtp_domain_addr() when DNS lookups are
/*	disabled.
/*
/*	smtp_host_addr() looks up all addresses listed for the named
/*	host.  The host can be specified as a numerical Internet network
/*	address, or as a symbolic host name.
/*
/*	smtp_service_addr() looks up addresses for hosts specified
/*	in SRV records for the specified domain and service. This
/*	supports the features of smtp_domain_addr() except that
/*	the order of SRV records is determined by RFC 2782, and
/*	that address records are not sorted by IP address family
/*	preference.  Fallback from SRV to MX or address lookups is
/*	governed by local policy (var_ign_mx_lookup_err and
/*	var_allow_srv_fallback).
/*
/*	Results from smtp_domain_addr(), smtp_host_addr(), and
/*	smtp_service_addr() are destroyed by dns_rr_free(), including
/*	null lists.
/* DIAGNOSTICS
/*	Panics: interface violations. For example, calling smtp_domain_addr()
/*	when DNS lookups are explicitly disabled.
/*
/*	All routines either return a DNS_RR pointer, or return a null
/*	pointer and update the \fIwhy\fR argument accordingly.
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	SRV Support by
/*	Tomas Korbar
/*	Red Hat, Inc.
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <stringops.h>
#include <myaddrinfo.h>
#include <inet_proto.h>
#include <midna_domain.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>
#include <dsn_buf.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_addr.h"

/* smtp_print_addr - print address list */

static void smtp_print_addr(const char *what, DNS_RR *addr_list)
{
    DNS_RR *addr;
    MAI_HOSTADDR_STR hostaddr;

    msg_info("begin %s address list", what);
    for (addr = addr_list; addr; addr = addr->next) {
	if (dns_rr_to_pa(addr, &hostaddr) == 0) {
	    msg_warn("skipping record type %s: %m", dns_strtype(addr->type));
	} else {
	    msg_info("pref %4d host %s/%s",
		     addr->pref, SMTP_HNAME(addr),
		     hostaddr.buf);
	}
    }
    msg_info("end %s address list", what);
}

/* smtp_addr_one - address lookup for one host name */

static DNS_RR *smtp_addr_one(DNS_RR *addr_list, const char *host, int res_opt,
			             unsigned pref, unsigned port,
			             DSN_BUF *why)
{
    const char *myname = "smtp_addr_one";
    DNS_RR *addr = 0;
    DNS_RR *rr;
    int     aierr;
    struct addrinfo *res0;
    struct addrinfo *res;
    const INET_PROTO_INFO *proto_info = inet_proto_info();
    unsigned char *proto_family_list = proto_info->sa_family_list;
    int     found;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * Interpret a numerical name as an address.
     */
    if (hostaddr_to_sockaddr(host, (char *) 0, 0, &res0) == 0) {
	if (strchr((char *) proto_family_list, res0->ai_family) != 0) {
	    if ((addr = dns_sa_to_rr(host, pref, res0->ai_addr)) == 0)
		msg_fatal("host %s: conversion error for address family "
			  "%d: %m", host, res0->ai_addr->sa_family);
	    addr_list = dns_rr_append(addr_list, addr);
	    addr->pref = pref;
	    addr->port = port;
	    if (msg_verbose)
		msg_info("%s: using numerical host %s", myname, host);
	    freeaddrinfo(res0);
	    return (addr_list);
	}
	freeaddrinfo(res0);
    }

    /*
     * Use DNS lookup, but keep the option open to use native name service.
     * 
     * XXX A soft error dominates past and future hard errors. Therefore we
     * should not clobber a soft error text and status code.
     */
    if (smtp_host_lookup_mask & SMTP_HOST_FLAG_DNS) {
	res_opt |= smtp_dns_res_opt;
	switch (dns_lookup_v(host, res_opt, &addr, (VSTRING *) 0,
			     why->reason, DNS_REQ_FLAG_NONE,
			     proto_info->dns_atype_list)) {
	case DNS_OK:
	    for (rr = addr; rr; rr = rr->next) {
		rr->pref = pref;
		rr->port = port;
	    }
	    addr_list = dns_rr_append(addr_list, addr);
	    return (addr_list);
	default:
	    dsb_status(why, "4.4.3");
	    return (addr_list);
	case DNS_FAIL:
	    dsb_status(why, SMTP_HAS_SOFT_DSN(why) ? "4.4.3" : "5.4.3");
	    return (addr_list);
	case DNS_INVAL:
	    dsb_status(why, SMTP_HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4");
	    return (addr_list);
	case DNS_POLICY:
	    dsb_status(why, "4.7.0");
	    return (addr_list);
	case DNS_NOTFOUND:
	    dsb_status(why, SMTP_HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4");
	    /* maybe native naming service will succeed */
	    break;
	}
    }

    /*
     * Use the native name service which also looks in /etc/hosts.
     * 
     * XXX A soft error dominates past and future hard errors. Therefore we
     * should not clobber a soft error text and status code.
     */
#define RETRY_AI_ERROR(e) \
        ((e) == EAI_AGAIN || (e) == EAI_MEMORY || (e) == EAI_SYSTEM)
#ifdef EAI_NODATA
#define DSN_NOHOST(e) \
	((e) == EAI_AGAIN || (e) == EAI_NODATA || (e) == EAI_NONAME)
#else
#define DSN_NOHOST(e) \
	((e) == EAI_AGAIN || (e) == EAI_NONAME)
#endif

    if (smtp_host_lookup_mask & SMTP_HOST_FLAG_NATIVE) {
	if ((aierr = hostname_to_sockaddr(host, (char *) 0, 0, &res0)) != 0) {
	    dsb_simple(why, (SMTP_HAS_SOFT_DSN(why) || RETRY_AI_ERROR(aierr)) ?
		       (DSN_NOHOST(aierr) ? "4.4.4" : "4.3.0") :
		       (DSN_NOHOST(aierr) ? "5.4.4" : "5.3.0"),
		       "unable to look up host %s: %s",
		       host, MAI_STRERROR(aierr));
	} else {
	    for (found = 0, res = res0; res != 0; res = res->ai_next) {
		if (strchr((char *) proto_family_list, res->ai_family) == 0) {
		    msg_info("skipping address family %d for host %s",
			     res->ai_family, host);
		    continue;
		}
		found++;
		if ((addr = dns_sa_to_rr(host, pref, res->ai_addr)) == 0)
		    msg_fatal("host %s: conversion error for address family "
			      "%d: %m", host, res0->ai_addr->sa_family);
		addr_list = dns_rr_append(addr_list, addr);
		if (msg_verbose) {
		    MAI_HOSTADDR_STR hostaddr_str;

		    SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
				  &hostaddr_str, (MAI_SERVPORT_STR *) 0, 0);
		    msg_info("%s: native lookup result: %s",
			     myname, hostaddr_str.buf);
		}
	    }
	    freeaddrinfo(res0);
	    if (found == 0) {
		dsb_simple(why, SMTP_HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4",
			   "%s: host not found", host);
	    }
	    return (addr_list);
	}
    }

    /*
     * No further alternatives for host lookup.
     */
    return (addr_list);
}

/* smtp_addr_list - address lookup for a list of mail exchangers */

static DNS_RR *smtp_addr_list(DNS_RR *mx_names, DSN_BUF *why)
{
    DNS_RR *addr_list = 0;
    DNS_RR *rr;
    int     res_opt = 0;

    if (mx_names->dnssec_valid)
	res_opt = RES_USE_DNSSEC;
#ifdef USE_TLS
    else if (smtp_tls_insecure_mx_policy > TLS_LEV_MAY)
	res_opt = RES_USE_DNSSEC;
#endif

    /*
     * As long as we are able to look up any host address, we ignore problems
     * with DNS lookups (except if we're backup MX, and all the better MX
     * hosts can't be found).
     * 
     * XXX 2821: update the error status (0->FAIL upon unrecoverable lookup
     * error, any->RETRY upon temporary lookup error) so that we can
     * correctly handle the case of no resolvable MX host. Currently this is
     * always treated as a soft error. RFC 2821 wants a more precise
     * response.
     * 
     * XXX dns_lookup() enables RES_DEFNAMES. This is wrong for names found in
     * MX records - we should not append the local domain to dot-less names.
     * 
     * XXX However, this is not the only problem. If we use the native name
     * service for host lookup, then it will usually enable RES_DNSRCH which
     * appends local domain information to all lookups. In particular,
     * getaddrinfo() may invoke a resolver that runs in a different process
     * (NIS server, nscd), so we can't even reliably turn this off by
     * tweaking the in-process resolver flags.
     */
    for (rr = mx_names; rr; rr = rr->next) {
	if (rr->type != T_MX && rr->type != T_SRV)
	    msg_panic("smtp_addr_list: bad resource type: %d", rr->type);
	addr_list = smtp_addr_one(addr_list, (char *) rr->data, res_opt,
				  rr->pref, rr->port, why);
    }
    return (addr_list);
}

/* smtp_find_self - spot myself in a crowd of mail exchangers */

static DNS_RR *smtp_find_self(DNS_RR *addr_list)
{
    const char *myname = "smtp_find_self";
    INET_ADDR_LIST *self;
    INET_ADDR_LIST *proxy;
    DNS_RR *addr;
    int     i;

    self = own_inet_addr_list();
    proxy = proxy_inet_addr_list();

    for (addr = addr_list; addr; addr = addr->next) {

	/*
	 * Find out if this mail system is listening on this address.
	 */
	for (i = 0; i < self->used; i++)
	    if (DNS_RR_EQ_SA(addr, (struct sockaddr *) (self->addrs + i))) {
		if (msg_verbose)
		    msg_info("%s: found self at pref %d", myname, addr->pref);
		return (addr);
	    }

	/*
	 * Find out if this mail system has a proxy listening on this
	 * address.
	 */
	for (i = 0; i < proxy->used; i++)
	    if (DNS_RR_EQ_SA(addr, (struct sockaddr *) (proxy->addrs + i))) {
		if (msg_verbose)
		    msg_info("%s: found proxy at pref %d", myname, addr->pref);
		return (addr);
	    }
    }

    /*
     * Didn't find myself, or my proxy.
     */
    if (msg_verbose)
	msg_info("%s: not found", myname);
    return (0);
}

/* smtp_truncate_self - truncate address list at self and equivalents */

static DNS_RR *smtp_truncate_self(DNS_RR *addr_list, unsigned pref)
{
    DNS_RR *addr;
    DNS_RR *last;

    for (last = 0, addr = addr_list; addr; last = addr, addr = addr->next) {
	if (pref == addr->pref) {
	    if (msg_verbose)
		smtp_print_addr("truncated", addr);
	    dns_rr_free(addr);
	    if (last == 0) {
		addr_list = 0;
	    } else {
		last->next = 0;
	    }
	    break;
	}
    }
    return (addr_list);
}

/* smtp_balance_inet_proto - balance IPv4/6 protocols within address limit */

static DNS_RR *smtp_balance_inet_proto(DNS_RR *addr_list, int misc_flags,
				               int addr_limit)
{
    const char myname[] = "smtp_balance_inet_proto";
    DNS_RR *rr;
    DNS_RR *stripped_list;
    DNS_RR *next;
    int     v6_count;
    int     v4_count;
    int     v6_target, v4_target;
    int    *p;

    /*
     * Precondition: the input is sorted by MX preference (not necessarily IP
     * address family preference), and addresses with the same or worse
     * preference than 'myself' have been eliminated. Postcondition: the
     * relative list order is unchanged, but some elements are removed.
     */

    /*
     * Count the number of IPv6 and IPv4 addresses.
     */
    for (v4_count = v6_count = 0, rr = addr_list; rr != 0; rr = rr->next) {
	if (rr->type == T_A) {
	    v4_count++;
	} else if (rr->type == T_AAAA) {
	    v6_count++;
	} else {
	    msg_panic("%s: unexpected record type: %s",
		      myname, dns_strtype(rr->type));
	}
    }

    /*
     * Ensure that one address type will not out-crowd the other, while
     * enforcing the address count limit. This works around a current problem
     * where some destination announces primarily IPv6 MX addresses, the
     * smtp_address_limit eliminates most or all IPv4 addresses, and the
     * destination is not reachable over IPv6.
     * 
     * Maybe: do all smtp_mx_address_limit enforcement here, and remove
     * pre-existing enforcement elsewhere. That would obsolete the
     * smtp_balance_inet_protocols configuration parameter.
     */
    if (v4_count > 0 && v6_count > 0 && v4_count + v6_count > addr_limit) {

	/*-
         * Decide how many IPv6 and IPv4 addresses to keep. The code below
         * has three branches, corresponding to the regions R1, R2 and R3
         * in the figure.
         *
         *  L = addr_limit
         *  X = excluded by condition (v4_count + v6_count > addr_limit)
         *
         * v4_count
         *     ^
         *     |
         *  L  \  R1
         *     |X\     |
         *     |XXX\   |
         *     |XXXXX\ | R2
         * L/2 +-------\-------
         *     |XXXXXXX|X\
         *     |XXXXXXX|XXX\  R3
         *     |XXXXXXX|XXXXX\
         *   0 +-------+-------\--> v6_count
         *     0      L/2      L
         */
	if (v6_count <= addr_limit / 2) {	/* Region R1 */
	    v6_target = v6_count;
	    v4_target = addr_limit - v6_target;
	} else if (v4_count <= addr_limit / 2) {/* Region R3 */
	    v4_target = v4_count;
	    v6_target = addr_limit - v4_target;
	} else {				/* Region R2 */
	    /* v4_count > addr_limit / 2 && v6_count > addr_limit / 2 */
	    v4_target = (addr_limit + (addr_list->type == T_A)) / 2;
	    v6_target = addr_limit - v4_target;
	}
	if (msg_verbose)
	    msg_info("v6_target=%d, v4_target=%d", v6_target, v4_target);

	/* Enforce the address count targets. */
	stripped_list = 0;
	for (rr = addr_list; rr != 0; rr = next) {
	    next = rr->next;
	    rr->next = 0;
	    if (rr->type == T_A) {
		p = &v4_target;
	    } else if (rr->type == T_AAAA) {
		p = &v6_target;
	    } else {
		msg_panic("%s: unexpected record type: %s",
			  myname, dns_strtype(rr->type));
	    }
	    if (*p > 0) {
		stripped_list = dns_rr_append(stripped_list, rr);
		*p -= 1;
	    } else {
		dns_rr_free(rr);
	    }
	}
	if (v4_target > 0 || v6_target > 0)
	    msg_panic("%s: bad target count: v4_target=%d, v6_target=%d",
		      myname, v4_target, v6_target);
	if (msg_verbose)
	    smtp_print_addr("smtp_balance_inet_proto result", stripped_list);
	return (stripped_list);
    } else {
	return (addr_list);
    }
}

/* smtp_domain_addr - mail exchanger address lookup */

DNS_RR *smtp_domain_addr(const char *name, DNS_RR **mxrr, int misc_flags,
			         DSN_BUF *why, int *found_myself)
{
    DNS_RR *mx_names;
    DNS_RR *addr_list = 0;
    DNS_RR *self = 0;
    unsigned best_pref;
    unsigned best_found;
    int     r = 0;			/* Resolver flags */
    const char *aname;

    dsb_reset(why);				/* Paranoia */

    /*
     * Preferences from DNS use 0..32767, fall-backs use 32768+.
     */
#define IMPOSSIBLE_PREFERENCE	(~0)

    /*
     * Sanity check.
     */
    if (smtp_dns_support == SMTP_DNS_DISABLED)
	msg_panic("smtp_domain_addr: DNS lookup is disabled");
    if (smtp_dns_support == SMTP_DNS_DNSSEC)
	r |= RES_USE_DNSSEC;

    /*
     * IDNA support.
     */
#ifndef NO_EAI
    if (!allascii(name) && (aname = midna_domain_to_ascii(name)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", name, aname);
    } else
#endif
	aname = name;

    /*
     * Look up the mail exchanger hosts listed for this name. Sort the
     * results by preference. Look up the corresponding host addresses, and
     * truncate the list so that it contains only hosts that are more
     * preferred than myself. When no MX resource records exist, look up the
     * addresses listed for this name.
     * 
     * According to RFC 974: "It is possible that the list of MXs in the
     * response to the query will be empty.  This is a special case.  If the
     * list is empty, mailers should treat it as if it contained one RR, an
     * MX RR with a preference value of 0, and a host name of REMOTE.  (I.e.,
     * REMOTE is its only MX).  In addition, the mailer should do no further
     * processing on the list, but should attempt to deliver the message to
     * REMOTE."
     * 
     * Normally it is OK if an MX host cannot be found in the DNS; we'll just
     * use a backup one, and silently ignore the better MX host. However, if
     * the best backup that we can find in the DNS is the local machine, then
     * we must remember that the local machine is not the primary MX host, or
     * else we will claim that mail loops back.
     * 
     * XXX Optionally do A lookups even when the MX lookup didn't complete.
     * Unfortunately with some DNS servers this is not a transient problem.
     * 
     * XXX Ideally we would perform A lookups only as far as needed. But as long
     * as we're looking up all the hosts, it would be better to look up the
     * least preferred host first, so that DNS lookup error messages make
     * more sense.
     * 
     * XXX 2821: RFC 2821 says that the sender must shuffle equal-preference MX
     * hosts, whereas multiple A records per hostname must be used in the
     * order as received. They make the bogus assumption that a hostname with
     * multiple A records corresponds to one machine with multiple network
     * interfaces.
     * 
     * XXX 2821: Postfix recognizes the local machine by looking for its own IP
     * address in the list of mail exchangers. RFC 2821 says one has to look
     * at the mail exchanger hostname as well, making the bogus assumption
     * that an IP address is listed only under one hostname. However, looking
     * at hostnames provides a partial solution for MX hosts behind a NAT
     * gateway.
     */
    switch (dns_lookup(aname, T_MX, r, &mx_names, (VSTRING *) 0, why->reason)) {
    default:
	dsb_status(why, "4.4.3");
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(aname, misc_flags, why);
	break;
    case DNS_INVAL:
	dsb_status(why, "5.4.4");
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(aname, misc_flags, why);
	break;
    case DNS_NULLMX:
	dsb_status(why, "5.1.0");
	break;
    case DNS_POLICY:
	dsb_status(why, "4.7.0");
	break;
    case DNS_FAIL:
	dsb_status(why, "5.4.3");
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(aname, misc_flags, why);
	break;
    case DNS_OK:
	mx_names = dns_rr_sort(mx_names, dns_rr_compare_pref_any);
	best_pref = (mx_names ? mx_names->pref : IMPOSSIBLE_PREFERENCE);
	addr_list = smtp_addr_list(mx_names, why);
	if (mxrr)
	    *mxrr = dns_rr_copy(mx_names);	/* copies one record! */
	dns_rr_free(mx_names);
	if (addr_list == 0) {
	    /* Text does not change. */
	    if (var_smtp_defer_mxaddr) {
		/* Don't clobber the null terminator. */
		if (SMTP_HAS_HARD_DSN(why))
		    SMTP_SET_SOFT_DSN(why);	/* XXX */
		/* Require some error status. */
		else if (!SMTP_HAS_SOFT_DSN(why))
		    msg_panic("smtp_domain_addr: bad status");
	    }
	    msg_warn("no MX host for %s has a valid address record", name);
	    break;
	}
	best_found = (addr_list ? addr_list->pref : IMPOSSIBLE_PREFERENCE);
	if (msg_verbose)
	    smtp_print_addr(name, addr_list);
	if ((misc_flags & SMTP_MISC_FLAG_LOOP_DETECT)
	    && (self = smtp_find_self(addr_list)) != 0) {
	    addr_list = smtp_truncate_self(addr_list, self->pref);
	    if (addr_list == 0) {
		if (best_pref != best_found) {
		    dsb_simple(why, "4.4.4",
			       "unable to find primary relay for %s", name);
		} else {
		    dsb_simple(why, "5.4.6", "mail for %s loops back to myself",
			       name);
		}
	    }
	}
#define SMTP_COMPARE_ADDR(flags) \
	(((flags) & SMTP_MISC_FLAG_PREF_IPV6) ? dns_rr_compare_pref_ipv6 : \
	 ((flags) & SMTP_MISC_FLAG_PREF_IPV4) ? dns_rr_compare_pref_ipv4 : \
	 dns_rr_compare_pref_any)

	if (addr_list && addr_list->next) {
	    if (var_smtp_rand_addr)
		addr_list = dns_rr_shuffle(addr_list);
	    addr_list = dns_rr_sort(addr_list, SMTP_COMPARE_ADDR(misc_flags));
	    if (var_smtp_mxaddr_limit > 0 && var_smtp_balance_inet_proto)
		addr_list = smtp_balance_inet_proto(addr_list, misc_flags,
						    var_smtp_mxaddr_limit);
	}
	break;
    case DNS_NOTFOUND:
	addr_list = smtp_host_addr(aname, misc_flags, why);
	break;
    }

    /*
     * Clean up.
     */
    *found_myself |= (self != 0);
    return (addr_list);
}

/* smtp_host_addr - direct host lookup */

DNS_RR *smtp_host_addr(const char *host, int misc_flags, DSN_BUF *why)
{
    DNS_RR *addr_list;
    int     res_opt = 0;
    const char *ahost;

    dsb_reset(why);				/* Paranoia */

    if (smtp_dns_support == SMTP_DNS_DNSSEC)
	res_opt |= RES_USE_DNSSEC;

    /*
     * IDNA support.
     */
#ifndef NO_EAI
    if (!allascii(host) && (ahost = midna_domain_to_ascii(host)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", host, ahost);
    } else
#endif
	ahost = host;

    /*
     * If the host is specified by numerical address, just convert the
     * address to internal form. Otherwise, the host is specified by name.
     */
#define PREF0	0
    addr_list = smtp_addr_one((DNS_RR *) 0, ahost, res_opt, PREF0, 0, why);
    if (addr_list
	&& (misc_flags & SMTP_MISC_FLAG_LOOP_DETECT)
	&& smtp_find_self(addr_list) != 0) {
	dns_rr_free(addr_list);
	dsb_simple(why, "5.4.6", "mail for %s loops back to myself", host);
	return (0);
    }
    if (addr_list && addr_list->next) {
	if (var_smtp_rand_addr)
	    addr_list = dns_rr_shuffle(addr_list);
	/* The following changes the order of equal-preference hosts. */
	if (inet_proto_info()->ai_family_list[1] != 0)
	    addr_list = dns_rr_sort(addr_list, SMTP_COMPARE_ADDR(misc_flags));
	if (var_smtp_mxaddr_limit > 0 && var_smtp_balance_inet_proto)
	    addr_list = smtp_balance_inet_proto(addr_list, misc_flags,
						var_smtp_mxaddr_limit);
    }
    if (msg_verbose)
	smtp_print_addr(host, addr_list);
    return (addr_list);
}

/* smtp_service_addr - service address lookup */

DNS_RR *smtp_service_addr(const char *name, const char *service, DNS_RR **mxrr,
			          int misc_flags, DSN_BUF *why,
			          int *found_myself)
{
    static VSTRING *srv_qname = 0;
    const char *str_srv_qname;
    DNS_RR *srv_names = 0;
    DNS_RR *addr_list = 0;
    DNS_RR *self = 0;
    unsigned best_pref;
    unsigned best_found;
    int     r = 0;
    const char *aname;
    int     allow_non_srv_fallback = var_allow_srv_fallback;

    dsb_reset(why);

    /*
     * Sanity check.
     */
    if (smtp_dns_support == SMTP_DNS_DISABLED)
	msg_panic("smtp_service_addr: DNS lookup is disabled");

    if (smtp_dns_support == SMTP_DNS_DNSSEC) {
	r |= RES_USE_DNSSEC;
    }
    if (srv_qname == 0)
	srv_qname = vstring_alloc(100);
    vstring_sprintf(srv_qname, "_%s._tcp.%s", service, name);
    str_srv_qname = STR(srv_qname);

    /*
     * IDNA support.
     */
#ifndef NO_EAI
    if (!allascii(str_srv_qname)
	&& (aname = midna_domain_to_ascii(str_srv_qname)) != 0) {
	if (msg_verbose)
	    msg_info("%s asciified to %s", str_srv_qname, aname);
    } else
#endif
	aname = str_srv_qname;

    switch (dns_lookup(aname, T_SRV, r, &srv_names, (VSTRING *) 0,
		       why->reason)) {
    default:
	dsb_status(why, "4.4.3");
	allow_non_srv_fallback |= var_ign_srv_lookup_err;
	break;
    case DNS_INVAL:
	dsb_status(why, "5.4.4");
	allow_non_srv_fallback |= var_ign_srv_lookup_err;
	break;
    case DNS_POLICY:
	dsb_status(why, "4.7.0");
	break;
    case DNS_FAIL:
	dsb_status(why, "5.4.3");
	allow_non_srv_fallback |= var_ign_srv_lookup_err;
	break;
    case DNS_NULLSRV:
	dsb_status(why, "5.1.0");
	break;
    case DNS_OK:
	/* Shuffle then sort the SRV rr records by priority and weight. */
	srv_names = dns_srv_rr_sort(srv_names);
	best_pref = (srv_names ? srv_names->pref : IMPOSSIBLE_PREFERENCE);
	addr_list = smtp_addr_list(srv_names, why);
	if (mxrr)
	    *mxrr = dns_rr_copy(srv_names);	/* copies one record! */
	dns_rr_free(srv_names);
	if (addr_list == 0) {
	    msg_warn("no SRV host for %s has a valid address record",
		     str_srv_qname);
	    break;
	}
	/* Optional loop prevention, similar to smtp_domain_addr(). */
	best_found = (addr_list ? addr_list->pref : IMPOSSIBLE_PREFERENCE);
	if (msg_verbose)
	    smtp_print_addr(aname, addr_list);
	if ((misc_flags & SMTP_MISC_FLAG_LOOP_DETECT)
	    && (self = smtp_find_self(addr_list)) != 0) {
	    addr_list = smtp_truncate_self(addr_list, self->pref);
	    if (addr_list == 0) {
		if (best_pref != best_found) {
		    dsb_simple(why, "4.4.4",
			       "unable to find primary relay for %s",
			       str_srv_qname);
		} else {
		    dsb_simple(why, "5.4.6", "mail for %s loops back to myself",
			       str_srv_qname);
		}
	    }
	}
	/* TODO: sort by priority, weight, and address family preference. */

	/* Optional address family balancing, as in smtp_domain_addr(). */
	if (addr_list && addr_list->next) {
	    if (var_smtp_mxaddr_limit > 0 && var_smtp_balance_inet_proto)
		addr_list = smtp_balance_inet_proto(addr_list, misc_flags,
						    var_smtp_mxaddr_limit);
	}
	break;
    case DNS_NOTFOUND:
	dsb_status(why, "5.4.4");
	break;
    }

    /*
     * If permitted, fall back to non-SRV record lookups.
     */
    if (addr_list == 0 && allow_non_srv_fallback) {
	msg_info("skipping SRV lookup for %s: %s",
		 str_srv_qname, STR(why->reason));
	if (misc_flags & SMTP_MISC_FLAG_FALLBACK_SRV_TO_MX)
	    addr_list = smtp_domain_addr(name, mxrr, misc_flags, why,
					 found_myself);
	else
	    addr_list = smtp_host_addr(name, misc_flags, why);
    }

    /*
     * Only if we're not falling back.
     */ 
    else {
	*found_myself |= (self != 0);
    }
    return (addr_list);
}
