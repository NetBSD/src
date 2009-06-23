/*	$NetBSD: test_dns_lookup.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

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
#include <mymalloc.h>
#include <argv.h>

/* Application-specific. */

#include "dns.h"

static void print_rr(DNS_RR *rr)
{
    MAI_HOSTADDR_STR host;

    while (rr) {
	printf("%s: ttl: %9d ", rr->rname, rr->ttl);
	switch (rr->type) {
	case T_A:
#ifdef T_AAAA
	case T_AAAA:
#endif
	    if (dns_rr_to_pa(rr, &host) == 0)
		msg_fatal("conversion error for resource record type %s: %m",
			  dns_strtype(rr->type));
	    printf("%s: %s\n", dns_strtype(rr->type), host.buf);
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
    ARGV   *types_argv;
    unsigned *types;
    char   *name;
    VSTRING *fqdn = vstring_alloc(100);
    VSTRING *why = vstring_alloc(100);
    DNS_RR *rr;
    int     i;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 3)
	msg_fatal("usage: %s types name", argv[0]);
    types_argv = argv_split(argv[1], ", \t\r\n");
    types = (int *) mymalloc(sizeof(*types) * (types_argv->argc + 1));
    for (i = 0; i < types_argv->argc; i++)
	if ((types[i] = dns_type(types_argv->argv[i])) == 0)
	    msg_fatal("invalid query type: %s", types_argv->argv[i]);
    types[i] = 0;
    argv_free(types_argv);
    name = argv[2];
    msg_verbose = 1;
    switch (dns_lookup_v(name, RES_DEFNAMES | RES_DEBUG, &rr, fqdn, why,
			 DNS_REQ_FLAG_NONE, types)) {
    default:
	msg_fatal("%s", vstring_str(why));
    case DNS_OK:
	printf("%s: fqdn: %s\n", name, vstring_str(fqdn));
	print_rr(rr);
	dns_rr_free(rr);
    }
    myfree((char *) types);
    exit(0);
}
