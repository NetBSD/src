/*	$NetBSD: dns_strrecord.c,v 1.2.26.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	dns_strrecord 3
/* SUMMARY
/*	name service resource record printable forms
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	char	*dns_strrecord(buf, record)
/*	VSTRING	*buf;
/*	DNS_RR	*record;
/* DESCRIPTION
/*	dns_strrecord() formats a DNS resource record as "name ttl
/*	class type preference value", where the class field is
/*	always "IN", the preference field exists only for MX records,
/*	and all names end in ".". The result value is the payload
/*	of the buffer argument.
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>			/* memcpy */

/* Utility library. */

#include <vstring.h>
#include <msg.h>

/* DNS library. */

#include <dns.h>

/* dns_strrecord - format resource record as generic string */

char   *dns_strrecord(VSTRING *buf, DNS_RR *rr)
{
    const char myname[] = "dns_strrecord";
    MAI_HOSTADDR_STR host;
    UINT32_TYPE soa_buf[5];

    vstring_sprintf(buf, "%s. %u IN %s ",
		    rr->rname, rr->ttl, dns_strtype(rr->type));
    switch (rr->type) {
    case T_A:
#ifdef T_AAAA
    case T_AAAA:
#endif
	if (dns_rr_to_pa(rr, &host) == 0)
	    msg_fatal("%s: conversion error for resource record type %s: %m",
		      myname, dns_strtype(rr->type));
	vstring_sprintf_append(buf, "%s", host.buf);
	break;
    case T_CNAME:
    case T_DNAME:
    case T_MB:
    case T_MG:
    case T_MR:
    case T_NS:
    case T_PTR:
	vstring_sprintf_append(buf, "%s.", rr->data);
	break;
    case T_TXT:
	vstring_sprintf_append(buf, "%s", rr->data);
	break;
    case T_MX:
	vstring_sprintf_append(buf, "%u %s.", rr->pref, rr->data);
	break;
    case T_SRV:
	vstring_sprintf_append(buf, "%u %u %u %s.", rr->pref, rr->weight,
			       rr->port, rr->data);
	break;
    case T_TLSA:
	if (rr->data_len >= 3) {
	    uint8_t *ip = (uint8_t *) rr->data;
	    uint8_t usage = *ip++;
	    uint8_t selector = *ip++;
	    uint8_t mtype = *ip++;
	    unsigned i;

	    /* /\.example\. \d+ IN TLSA \d+ \d+ \d+ [\da-f]*$/ IGNORE */
	    vstring_sprintf_append(buf, "%d %d %d ", usage, selector, mtype);
	    for (i = 3; i < rr->data_len; ++i)
		vstring_sprintf_append(buf, "%02x", *ip++);
	} else {
	    vstring_sprintf_append(buf, "[truncated record]");
	}

	/*
	 * We use the SOA record TTL to determine the negative reply TTL. We
	 * save the time fields in the SOA record for debugging, but for now
	 * we don't bother saving the source host and mailbox information, as
	 * that would require changes to the DNS_RR structure. See also code
	 * in dns_get_rr().
	 */
    case T_SOA:
	memcpy(soa_buf, rr->data, sizeof(soa_buf));
	vstring_sprintf_append(buf, "- - %u %u %u %u %u",
			       soa_buf[0], soa_buf[1], soa_buf[2],
			       soa_buf[3], soa_buf[4]);
	break;
    default:
	msg_fatal("%s: don't know how to print type %s",
		  myname, dns_strtype(rr->type));
    }
    return (vstring_str(buf));
}
