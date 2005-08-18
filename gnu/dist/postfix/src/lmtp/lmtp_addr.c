/*	$NetBSD: lmtp_addr.c,v 1.1.1.3 2005/08/18 21:07:17 rpaulo Exp $	*/

/*++
/* NAME
/*	lmtp_addr 3
/* SUMMARY
/*	LMTP server address lookup
/* SYNOPSIS
/*	#include "lmtp_addr.h"
/*
/*	DNS_RR *lmtp_host_addr(name, why)
/*	char	*name;
/*	VSTRING	*why;
/* DESCRIPTION
/*	This module implements Internet address lookups. By default,
/*	lookups are done via the Internet domain name service (DNS).
/*	A reasonable number of CNAME indirections is permitted.
/*
/*	lmtp_host_addr() looks up all addresses listed for the named
/*	host.  The host can be specified as a numerical Internet network
/*	address, or as a symbolic host name.
/*
/*	Fortunately, we don't have to worry about MX records because
/*	those are for SMTP servers, not LMTP servers.
/*
/*	Results from lmtp_host_addr() are destroyed by dns_rr_free(),
/*	including null lists.
/* DIAGNOSTICS
/*	This routine either returns a DNS_RR pointer, or return a null
/*	pointer and sets the \fIlmtp_errno\fR global variable accordingly:
/* .IP LMTP_RETRY
/*	The request failed due to a soft error, and should be retried later.
/* .IP LMTP_FAIL
/*	The request attempt failed due to a hard error.
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
/*
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <stringops.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include "lmtp.h"
#include "lmtp_addr.h"

/* lmtp_print_addr - print address list */

static void lmtp_print_addr(char *what, DNS_RR *addr_list)
{
    DNS_RR *addr;
    MAI_HOSTADDR_STR hostaddr;

    msg_info("begin %s address list", what);
    for (addr = addr_list; addr; addr = addr->next) {
	if (dns_rr_to_pa(addr, &hostaddr) == 0) {
	    msg_warn("skipping record type %s: %m", dns_strtype(addr->type));
	} else {
	    msg_info("pref %4d host %s/%s",
		     addr->pref, addr->name,
		     hostaddr.buf);
	}
    }
    msg_info("end %s address list", what);
}

/* lmtp_addr_one - address lookup for one host name */

static DNS_RR *lmtp_addr_one(DNS_RR *addr_list, char *host, unsigned pref, VSTRING *why)
{
    char   *myname = "lmtp_addr_one";
    DNS_RR *addr = 0;
    DNS_RR *rr;
    int     aierr;
    struct addrinfo *res0;
    struct addrinfo *res;
    INET_PROTO_INFO *proto_info = inet_proto_info();
    int     found;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * Interpret a numerical name as an address.
     */
    if (hostaddr_to_sockaddr(host, (char *) 0, 0, &res0) == 0
	&& strchr((char *) proto_info->sa_family_list, res0->ai_family) != 0) {
	if ((addr = dns_sa_to_rr(host, pref, res0->ai_addr)) == 0)
	    msg_fatal("host %s: conversion error for address family %d: %m",
		    host, ((struct sockaddr *) (res0->ai_addr))->sa_family);
	addr_list = dns_rr_append(addr_list, addr);
	freeaddrinfo(res0);
	return (addr_list);
    }

    /*
     * Use native name service when DNS is disabled.
     */
#define RETRY_AI_ERROR(e) \
	((e) == EAI_AGAIN || (e) == EAI_MEMORY || (e) == EAI_SYSTEM)

    if (var_disable_dns) {
	if ((aierr = hostname_to_sockaddr(host, (char *) 0, 0, &res0)) != 0) {
	    vstring_sprintf(why, "%s: %s", host, MAI_STRERROR(aierr));
	    lmtp_errno = (RETRY_AI_ERROR(aierr) ? LMTP_RETRY : LMTP_FAIL);
	} else {
	    for (found = 0, res = res0; res != 0; res = res->ai_next) {
		if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
		    msg_info("skipping address family %d for host %s",
			     res->ai_family, host);
		    continue;
		}
		found++;
		if ((addr = dns_sa_to_rr(host, pref, res->ai_addr)) == 0)
		    msg_fatal("host %s: conversion error for address family %d: %m",
		    host, ((struct sockaddr *) (res0->ai_addr))->sa_family);
		addr_list = dns_rr_append(addr_list, addr);
	    }
	    freeaddrinfo(res0);
	    if (found == 0) {
		vstring_sprintf(why, "%s: host not found", host);
		lmtp_errno = LMTP_FAIL;
	    }
	    return (addr_list);
	}
    }

    /*
     * Append the addresses for this host to the address list.
     */
    switch (dns_lookup_v(host, RES_DEFNAMES, &addr, (VSTRING *) 0, why,
			 DNS_REQ_FLAG_ALL, proto_info->dns_atype_list)) {
    case DNS_OK:
	for (rr = addr; rr; rr = rr->next)
	    rr->pref = pref;
	addr_list = dns_rr_append(addr_list, addr);
	break;
    default:
	lmtp_errno = LMTP_RETRY;
	break;
    case DNS_NOTFOUND:
    case DNS_FAIL:
	lmtp_errno = LMTP_FAIL;
	break;
    }
    return (addr_list);
}

/* lmtp_host_addr - direct host lookup */

DNS_RR *lmtp_host_addr(char *host, VSTRING *why)
{
    DNS_RR *addr_list;

    /*
     * If the host is specified by numerical address, just convert the
     * address to internal form. Otherwise, the host is specified by name.
     */
#define PREF0	0
    addr_list = lmtp_addr_one((DNS_RR *) 0, host, PREF0, why);
    if (msg_verbose)
	lmtp_print_addr(host, addr_list);
    return (addr_list);
}
