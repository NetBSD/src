/*++
/* NAME
/*	test_dns_lookup 1
/* SUMMARY
/*	DNS lookup test program
/* SYNOPSIS
/*	test_dns_lookup query-type domain-name
/* DESCRIPTION
/*	test_dns_lookup performs a DNS query of the specified resource
/*	type for the specified resource name.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

/* Utility library. */

#include <vstring.h>
#include <msg.h>
#include <msg_vstream.h>

/* Application-specific. */

#include "dns.h"

static void print_rr(DNS_RR *rr)
{
    struct in_addr addr;

    while (rr) {
	printf("%s: ttl: %9d ", rr->name, rr->ttl);
	switch (rr->type) {
	case T_A:
	    memcpy((char *) &addr.s_addr, rr->data, sizeof(addr.s_addr));
	    printf("%s: %s\n", dns_strtype(rr->type), inet_ntoa(addr));
	    break;
	case T_CNAME:
	case T_MB:
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
	case T_TXT:
	    printf("%s: %s\n", dns_strtype(rr->type), rr->data);
	    break;
	case T_MX:
	    printf("pref: %d %s: %s\n",
		   rr->pref, dns_strtype(rr->type), rr->data);
	    break;
	default:
	    msg_fatal("print_rr: don't know how to print type %s",
		      dns_strtype(rr->type));
	}
	rr = rr->next;
    }
}

int     main(int argc, char **argv)
{
    int     type;
    char   *name;
    VSTRING *fqdn = vstring_alloc(100);
    VSTRING *why = vstring_alloc(100);
    DNS_RR *rr;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 3)
	msg_fatal("usage: %s type name", argv[0]);
    if ((type = dns_type(argv[1])) == 0)
	msg_fatal("invalid query type: %s", argv[1]);
    name = argv[2];
    msg_verbose = 1;
    switch (dns_lookup_types(name, RES_DEFNAMES | RES_DEBUG, &rr, fqdn, why, type, 0)) {
    default:
	msg_fatal("%s", vstring_str(why));
    case DNS_OK:
	printf("%s: fqdn: %s\n", name, vstring_str(fqdn));
	print_rr(rr);
    }
    exit(0);
}
