/*++
/* NAME
/*	smtp_addr 3
/* SUMMARY
/*	SMTP server address lookup
/* SYNOPSIS
/*	#include "smtp_addr.h"
/*
/*	DNS_RR *smtp_domain_addr(name, misc_flags, why)
/*	char	*name;
/*	int	misc_flags;
/*	VSTRING	*why;
/*
/*	DNS_RR *smtp_host_addr(name, misc_flags, why)
/*	char	*name;
/*	int	misc_flags;
/*	VSTRING	*why;
/* DESCRIPTION
/*	This module implements Internet address lookups. By default,
/*	lookups are done via the Internet domain name service (DNS).
/*	A reasonable number of CNAME indirections is permitted. When
/*	DNS lookups are disabled, host address lookup is done with
/*	gethostbyname().
/*
/*	smtp_domain_addr() looks up the network addresses for mail
/*	exchanger hosts listed for the named domain. Addresses are
/*	returned in most-preferred first order. The result is truncated
/*	so that it contains only hosts that are more preferred than the
/*	local mail server itself.
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
/*	Results from smtp_domain_addr() or smtp_host_addr() are
/*	destroyed by dns_rr_free(), including null lists.
/* DIAGNOSTICS
/*	Panics: interface violations. For example, calling smtp_domain_addr()
/*	when DNS lookups are explicitly disabled.
/*
/*	All routines either return a DNS_RR pointer, or return a null
/*	pointer and set the \fIsmtp_errno\fR global variable accordingly:
/* .IP SMTP_RETRY
/*	The request failed due to a soft error, and should be retried later.
/* .IP SMTP_FAIL
/*	The request attempt failed due to a hard error.
/* .IP SMTP_LOOP
/*	The local machine is the best mail exchanger.
/* .PP
/*	In addition, a textual description of the problem is made available
/*	via the \fIwhy\fR argument.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

 /*
  * Older systems don't have h_errno. Even modern systems don't have
  * hstrerror().
  */
#ifdef NO_HERRNO

static int h_errno = TRY_AGAIN;

#define  HSTRERROR(err) "Host not found"

#else

#define  HSTRERROR(err) (\
        err == TRY_AGAIN ? "Host not found, try again" : \
        err == HOST_NOT_FOUND ? "Host not found" : \
        err == NO_DATA ? "Host name has no address" : \
        err == NO_RECOVERY ? "Name server failure" : \
        strerror(errno) \
    )
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <stringops.h>
#include <myrand.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_addr.h"

/* smtp_print_addr - print address list */

static void smtp_print_addr(char *what, DNS_RR *addr_list)
{
    DNS_RR *addr;
    struct in_addr in_addr;

    msg_info("begin %s address list", what);
    for (addr = addr_list; addr; addr = addr->next) {
	if (addr->data_len > sizeof(addr)) {
	    msg_warn("skipping address length %d", addr->data_len);
	} else {
	    memcpy((char *) &in_addr, addr->data, sizeof(in_addr));
	    msg_info("pref %4d host %s/%s",
		     addr->pref, addr->name,
		     inet_ntoa(in_addr));
	}
    }
    msg_info("end %s address list", what);
}

/* smtp_addr_one - address lookup for one host name */

static DNS_RR *smtp_addr_one(DNS_RR *addr_list, char *host, unsigned pref, VSTRING *why)
{
    char   *myname = "smtp_addr_one";
    struct in_addr inaddr;
    DNS_FIXED fixed;
    DNS_RR *addr = 0;
    DNS_RR *rr;
    struct hostent *hp;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * Interpret a numerical name as an address.
     */
    if (ISDIGIT(host[0]) && (inaddr.s_addr = inet_addr(host)) != INADDR_NONE) {
	memset((char *) &fixed, 0, sizeof(fixed));
	return (dns_rr_append(addr_list,
			      dns_rr_create(host, &fixed, pref,
					(char *) &inaddr, sizeof(inaddr))));
    }

    /*
     * Use DNS lookup, but keep the option open to use native name service.
     */
    if (smtp_host_lookup_mask & SMTP_HOST_FLAG_DNS) {
	switch (dns_lookup(host, T_A, RES_DEFNAMES, &addr, (VSTRING *) 0, why)) {
	case DNS_OK:
	    for (rr = addr; rr; rr = rr->next)
		rr->pref = pref;
	    addr_list = dns_rr_append(addr_list, addr);
	    return (addr_list);
	default:
	    smtp_errno = SMTP_ERR_RETRY;
	    return (addr_list);
	case DNS_FAIL:
	    if (smtp_errno != SMTP_ERR_RETRY)
		smtp_errno = SMTP_ERR_FAIL;
	    return (addr_list);
	case DNS_NOTFOUND:
	    if (smtp_errno != SMTP_ERR_RETRY)
		smtp_errno = SMTP_ERR_FAIL;
	    /* maybe gethostbyname() will succeed */
	    break;
	}
    }

    /*
     * Use the native name service which also looks in /etc/hosts.
     */
    if (smtp_host_lookup_mask & SMTP_HOST_FLAG_NATIVE) {
	memset((char *) &fixed, 0, sizeof(fixed));
	if ((hp = gethostbyname(host)) == 0) {
	    vstring_sprintf(why, "%s: %s", host, HSTRERROR(h_errno));
	    if (smtp_errno != SMTP_ERR_RETRY)
		smtp_errno =
		    (h_errno == TRY_AGAIN ? SMTP_ERR_RETRY : SMTP_ERR_FAIL);
	} else if (hp->h_addrtype != AF_INET) {
	    vstring_sprintf(why, "%s: host not found", host);
	    msg_warn("%s: unknown address family %d for %s",
		     myname, hp->h_addrtype, host);
	    if (smtp_errno != SMTP_ERR_RETRY)
		smtp_errno = SMTP_ERR_FAIL;
	} else {
	    while (hp->h_addr_list[0]) {
		addr_list = dns_rr_append(addr_list,
					  dns_rr_create(host, &fixed, pref,
							hp->h_addr_list[0],
							sizeof(inaddr)));
		hp->h_addr_list++;
	    }
	}
	return (addr_list);
    }

    /*
     * No further alternatives for host lookup.
     */
    return (addr_list);
}

/* smtp_addr_list - address lookup for a list of mail exchangers */

static DNS_RR *smtp_addr_list(DNS_RR *mx_names, VSTRING *why)
{
    DNS_RR *addr_list = 0;
    DNS_RR *rr;

    /*
     * As long as we are able to look up any host address, we ignore problems
     * with DNS lookups (except if we're backup MX, and all the better MX
     * hosts can't be found).
     * 
     * XXX 2821: update smtp_errno (0->FAIL upon unrecoverable lookup error,
     * any->RETRY upon temporary lookup error) so that we can correctly
     * handle the case of no resolvable MX host. Currently this is always
     * treated as a soft error. RFC 2821 wants a more precise response.
     */
    for (rr = mx_names; rr; rr = rr->next) {
	if (rr->type != T_MX)
	    msg_panic("smtp_addr_list: bad resource type: %d", rr->type);
	addr_list = smtp_addr_one(addr_list, (char *) rr->data, rr->pref, why);
    }
    return (addr_list);
}

/* smtp_find_self - spot myself in a crowd of mail exchangers */

static DNS_RR *smtp_find_self(DNS_RR *addr_list)
{
    char   *myname = "smtp_find_self";
    INET_ADDR_LIST *self;
    DNS_RR *addr;
    int     i;

    /*
     * Find the first address that lists any address that this mail system is
     * supposed to be listening on.
     */
#define INADDRP(x) ((struct in_addr *) (x))

    self = own_inet_addr_list();
    for (addr = addr_list; addr; addr = addr->next) {
	for (i = 0; i < self->used; i++)
	    if (INADDRP(addr->data)->s_addr == self->addrs[i].s_addr) {
		if (msg_verbose)
		    msg_info("%s: found at pref %d", myname, addr->pref);
		return (addr);
	    }
    }

    /*
     * Find out if this mail system has a proxy listening on this address.
     */
    self = proxy_inet_addr_list();
    for (addr = addr_list; addr; addr = addr->next) {
	for (i = 0; i < self->used; i++)
	    if (INADDRP(addr->data)->s_addr == self->addrs[i].s_addr) {
		if (msg_verbose)
		    msg_info("%s: found at pref %d", myname, addr->pref);
		return (addr);
	    }
    }

    /*
     * Didn't find myself.
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

/* smtp_compare_pref - compare resource records by preference */

static int smtp_compare_pref(DNS_RR *a, DNS_RR *b)
{
    return (a->pref - b->pref);
}

/* smtp_domain_addr - mail exchanger address lookup */

DNS_RR *smtp_domain_addr(char *name, int misc_flags, VSTRING *why)
{
    DNS_RR *mx_names;
    DNS_RR *addr_list = 0;
    DNS_RR *self = 0;
    unsigned best_pref;
    unsigned best_found;

    smtp_errno = SMTP_ERR_NONE;			/* Paranoia */

    /*
     * Preferences from DNS use 0..32767, fall-backs use 32768+.
     */
#define IMPOSSIBLE_PREFERENCE	(~0)

    /*
     * Sanity check.
     */
    if (var_disable_dns)
	msg_panic("smtp_domain_addr: DNS lookup is disabled");

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
    switch (dns_lookup(name, T_MX, 0, &mx_names, (VSTRING *) 0, why)) {
    default:
	smtp_errno = SMTP_ERR_RETRY;
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(name, misc_flags, why);
	break;
    case DNS_FAIL:
	smtp_errno = SMTP_ERR_FAIL;
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(name, misc_flags, why);
	break;
    case DNS_OK:
	mx_names = dns_rr_sort(mx_names, smtp_compare_pref);
	best_pref = (mx_names ? mx_names->pref : IMPOSSIBLE_PREFERENCE);
	addr_list = smtp_addr_list(mx_names, why);
	dns_rr_free(mx_names);
	if (addr_list == 0) {
	    if (var_smtp_defer_mxaddr)
		smtp_errno = SMTP_ERR_RETRY;
	    msg_warn("no MX host for %s has a valid A record", name);
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
		    vstring_sprintf(why, "unable to find primary relay for %s",
				    name);
		    smtp_errno = SMTP_ERR_RETRY;
		} else {
		    vstring_sprintf(why, "mail for %s loops back to myself",
				    name);
		    smtp_errno = SMTP_ERR_LOOP;
		}
	    }
	}
	if (addr_list && addr_list->next && var_smtp_rand_addr) {
	    addr_list = dns_rr_shuffle(addr_list);
	    addr_list = dns_rr_sort(addr_list, smtp_compare_pref);
	}
	break;
    case DNS_NOTFOUND:
	addr_list = smtp_host_addr(name, misc_flags, why);
	break;
    }

    /*
     * Clean up.
     */
    return (addr_list);
}

/* smtp_host_addr - direct host lookup */

DNS_RR *smtp_host_addr(char *host, int misc_flags, VSTRING *why)
{
    DNS_RR *addr_list;

    smtp_errno = SMTP_ERR_NONE;			/* Paranoia */

    /*
     * If the host is specified by numerical address, just convert the
     * address to internal form. Otherwise, the host is specified by name.
     */
#define PREF0	0
    addr_list = smtp_addr_one((DNS_RR *) 0, host, PREF0, why);
    if (addr_list
	&& (misc_flags & SMTP_MISC_FLAG_LOOP_DETECT)
	&& smtp_find_self(addr_list) != 0) {
	dns_rr_free(addr_list);
	vstring_sprintf(why, "mail for %s loops back to myself", host);
	smtp_errno = SMTP_ERR_LOOP;
	return (0);
    }
    if (addr_list && addr_list->next && var_smtp_rand_addr)
	addr_list = dns_rr_shuffle(addr_list);
    if (msg_verbose)
	smtp_print_addr(host, addr_list);
    return (addr_list);
}
