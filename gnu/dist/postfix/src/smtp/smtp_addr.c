/*++
/* NAME
/*	smtp_addr 3
/* SUMMARY
/*	SMTP server address lookup
/* SYNOPSIS
/*	#include "smtp_addr.h"
/*
/*	DNS_RR *smtp_domain_addr(name, why, found_myself)
/*	char	*name;
/*	VSTRING	*why;
/*	int	*found_myself;
/*
/*	DNS_RR *smtp_host_addr(name, why)
/*	char	*name;
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
/*	local mail server itself. When the "best MX is local" feature
/*	is enabled, the local system is allowed to be the best mail
/*	exchanger, and the result is a null list pointer. Otherwise,
/*	mailer loops are treated as an error.
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
/* .IP SMTP_OK
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

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <stringops.h>

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
#ifdef INET6
    SOCKADDR_SIZE salen;
    struct sockaddr_storage ss;
#else
    struct sockaddr ss;
#endif
    struct sockaddr_in *sin;
#ifdef INET6
    struct sockaddr_in6 *sin6;
    char   hbuf[NI_MAXHOST];
#else
    char   hbuf[sizeof("255.255.255.255") + 1];
#endif

    msg_info("begin %s address list", what);
    for (addr = addr_list; addr; addr = addr->next) {
	if (addr->class != C_IN) {
	    msg_warn("skipping unsupported address (class=%u)", addr->class);
	    continue;
	}
	switch (addr->type) {
	case T_A:
	    if (addr->data_len != sizeof(sin->sin_addr)) {
		msg_warn("skipping invalid address (AAAA, len=%u)",
		    addr->data_len);
		continue;
	    }
	    sin = (struct sockaddr_in *)&ss;
	    memset(sin, 0, sizeof(*sin));
	    sin->sin_family = AF_INET;
#ifdef HAS_SA_LEN
	    sin->sin_len = sizeof(*sin);
#endif
	    memcpy(&sin->sin_addr, addr->data, sizeof(sin->sin_addr));
	    break;
#ifdef INET6
	case T_AAAA:
	    if (addr->data_len != sizeof(sin6->sin6_addr)) {
		msg_warn("skipping invalid address (AAAA, len=%u)",
		    addr->data_len);
		continue;
	    }
	    sin6 = (struct sockaddr_in6 *)&ss;
	    memset(sin6, 0, sizeof(*sin6));
	    sin6->sin6_family = AF_INET6;
#ifdef HAS_SA_LEN
	    sin6->sin6_len = sizeof(*sin6);
#endif
	    memcpy(&sin6->sin6_addr, addr->data, sizeof(sin6->sin6_addr));
	    break;
#endif
	default:
	    msg_warn("skipping unsupported address (type=%u)", addr->type);
	    continue;
	}

#ifdef INET6
#ifndef HAS_SA_LEN
	salen = SA_LEN((struct sockaddr *)&ss);
#else
	salen = ((struct sockaddr *)&ss)->sa_len;
#endif
	(void)getnameinfo((struct sockaddr *)&ss, salen,
	    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
#else
	(void)inet_ntop(AF_INET, &sin->sin_addr, hbuf, sizeof(hbuf));
#endif
	msg_info("pref %4d host %s/%s", addr->pref, addr->name, hbuf);
    }
    msg_info("end %s address list", what);
}

/* smtp_addr_one - address lookup for one host name */

static DNS_RR *smtp_addr_one(DNS_RR *addr_list, char *host, unsigned pref, VSTRING *why)
{
    char   *myname = "smtp_addr_one";
#ifndef INET6
    struct in_addr inaddr;
    DNS_RR *addr = 0;
    DNS_RR *rr;
    struct hostent *hp;
#else
    struct addrinfo hints, *res0, *res;
    int error = -1;
    char *addr;
    size_t addrlen;
#endif
    DNS_FIXED fixed;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

#ifndef INET6
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
     * Use gethostbyname() when DNS is disabled.
     */
    if (var_disable_dns) {
	memset((char *) &fixed, 0, sizeof(fixed));
	if ((hp = gethostbyname(host)) == 0) {
	    vstring_sprintf(why, "%s: host not found", host);
	    smtp_errno = SMTP_FAIL;
	} else if (hp->h_addrtype != AF_INET) {
	    vstring_sprintf(why, "%s: host not found", host);
	    msg_warn("%s: unknown address family %d for %s",
		     myname, hp->h_addrtype, host);
	    smtp_errno = SMTP_FAIL;
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
     * Append the addresses for this host to the address list.
     */
    switch (dns_lookup(host, T_A, RES_DEFNAMES, &addr, (VSTRING *) 0, why)) {
    case DNS_OK:
	for (rr = addr; rr; rr = rr->next)
	    rr->pref = pref;
	addr_list = dns_rr_append(addr_list, addr);
	break;
    default:
	smtp_errno = SMTP_RETRY;
	break;
    case DNS_NOTFOUND:
    case DNS_FAIL:
	smtp_errno = SMTP_FAIL;
	break;
    }
#else
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(host, NULL, &hints, &res0);
    if (error) {
	switch (error) {
	case EAI_AGAIN:
	    smtp_errno = SMTP_RETRY;
	    break;
	default:
	    vstring_sprintf(why, "[%s]: %s",host,gai_strerror(error));
	    smtp_errno = SMTP_FAIL;
	    break;
	}
	return (addr_list);
    }
    for (res = res0; res; res = res->ai_next) {
	memset((char *) &fixed, 0, sizeof(fixed));
	switch(res->ai_family) {
	case AF_INET6:
	    /* XXX not scope friendly */
	    fixed.type = T_AAAA;
	    addr = (char *)&((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
	    addrlen = sizeof(struct in6_addr);
	    break;
	case AF_INET:
	    fixed.type = T_A;
	    addr = (char *)&((struct sockaddr_in *)res->ai_addr)->sin_addr;
	    addrlen = sizeof(struct in_addr);
	    break;
	default:
	    msg_warn("%s: unknown address family %d for %s",
	        myname, res->ai_family, host);
	    continue;
	}
	addr_list = dns_rr_append(addr_list,
	    dns_rr_create(host, &fixed, pref, addr, addrlen));
    }
    if (res0)
	freeaddrinfo(res0);
#endif
    return (addr_list);
}

/* smtp_addr_list - address lookup for a list of mail exchangers */

static DNS_RR *smtp_addr_list(DNS_RR *mx_names, VSTRING *why)
{
    DNS_RR *addr_list = 0;
    DNS_RR *rr;

    /*
     * As long as we are able to look up any host address, we ignore problems
     * with DNS lookups.
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
#ifdef INET6
    struct sockaddr *sa;
#endif

    /*
     * Find the first address that lists any address that this mail system is
     * supposed to be listening on.
     */
#define INADDRP(x) ((struct in_addr *) (x))

    self = own_inet_addr_list();
    for (addr = addr_list; addr; addr = addr->next) {
	for (i = 0; i < self->used; i++) {
#ifdef INET6
	    sa = (struct sockaddr *)&self->addrs[i];
	    switch(addr->type) {
	    case T_AAAA:
		/* XXX scope */
		if (sa->sa_family != AF_INET6)
		    break;
		if (memcmp(&((struct sockaddr_in6 *)sa)->sin6_addr,
			addr->data, sizeof(struct in6_addr)) == 0) {
		    return(addr);
		}
		break;
	    case T_A:
		if (sa->sa_family != AF_INET)
		    break;
		if (memcmp(&((struct sockaddr_in *)sa)->sin_addr,
			addr->data, sizeof(struct in_addr)) == 0) {
		    return(addr);
		}
		break;
	    }
#else
	    if (INADDRP(addr->data)->s_addr == self->addrs[i].s_addr) {
		if (msg_verbose)
		    msg_info("%s: found at pref %d", myname, addr->pref);
		return (addr);
	    }
#endif
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

/* smtp_compare_mx - compare resource records by preference */

static int smtp_compare_mx(DNS_RR *a, DNS_RR *b)
{
    return (a->pref - b->pref);
}

/* smtp_domain_addr - mail exchanger address lookup */

DNS_RR *smtp_domain_addr(char *name, VSTRING *why, int *found_myself)
{
    DNS_RR *mx_names;
    DNS_RR *addr_list = 0;
    DNS_RR *self = 0;
    unsigned best_pref;
    unsigned best_found;

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
     */
    switch (dns_lookup(name, T_MX, 0, &mx_names, (VSTRING *) 0, why)) {
    default:
	smtp_errno = SMTP_RETRY;
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(name, why);
	break;
    case DNS_FAIL:
	smtp_errno = SMTP_FAIL;
	if (var_ign_mx_lookup_err)
	    addr_list = smtp_host_addr(name, why);
	break;
    case DNS_OK:
	mx_names = dns_rr_sort(mx_names, smtp_compare_mx);
	best_pref = (mx_names ? mx_names->pref : IMPOSSIBLE_PREFERENCE);
	addr_list = smtp_addr_list(mx_names, why);
	dns_rr_free(mx_names);
	if (addr_list == 0) {
	    smtp_errno = SMTP_RETRY;
	    msg_warn("no MX host for %s has a valid A record", name);
	    break;
	}
	best_found = (addr_list ? addr_list->pref : IMPOSSIBLE_PREFERENCE);
	if (msg_verbose)
	    smtp_print_addr(name, addr_list);
	if ((self = smtp_find_self(addr_list)) != 0) {
	    addr_list = smtp_truncate_self(addr_list, self->pref);
	    if (addr_list == 0) {
		if (best_pref != best_found) {
		    vstring_sprintf(why, "unable to find primary relay for %s",
				    name);
		    smtp_errno = SMTP_RETRY;
		} else if (*var_bestmx_transp != 0) {	/* we're best MX */
		    smtp_errno = SMTP_OK;
		} else {
		    vstring_sprintf(why, "mail for %s loops back to myself",
				    name);
		    smtp_errno = SMTP_FAIL;
		}
	    }
	}
	break;
    case DNS_NOTFOUND:
	addr_list = smtp_host_addr(name, why);
	break;
    }

    /*
     * Clean up.
     */
    *found_myself = (self != 0);
    return (addr_list);
}

/* smtp_host_addr - direct host lookup */

DNS_RR *smtp_host_addr(char *host, VSTRING *why)
{
    DNS_RR *addr_list;

    /*
     * If the host is specified by numerical address, just convert the
     * address to internal form. Otherwise, the host is specified by name.
     */
#define PREF0	0
    addr_list = smtp_addr_one((DNS_RR *) 0, host, PREF0, why);
    if (msg_verbose)
	smtp_print_addr(host, addr_list);
    return (addr_list);
}
