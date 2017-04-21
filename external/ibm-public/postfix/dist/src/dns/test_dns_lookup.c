/*	$NetBSD: test_dns_lookup.c,v 1.1.1.3.10.1 2017/04/21 16:52:47 bouyer Exp $	*/

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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "dns.h"

static void print_rr(VSTRING *buf, DNS_RR *rr)
{
    while (rr) {
	vstream_printf("ad: %u, rr: %s\n",
		       rr->dnssec_valid, dns_strrecord(buf, rr));
	rr = rr->next;
    }
}

static NORETURN usage(char **argv)
{
    msg_fatal("usage: %s [-npv] [-f filter] types name", argv[0]);
}

int     main(int argc, char **argv)
{
    ARGV   *types_argv;
    unsigned *types;
    char   *name;
    VSTRING *fqdn = vstring_alloc(100);
    VSTRING *why = vstring_alloc(100);
    VSTRING *buf;
    int     rcode;
    DNS_RR *rr;
    int     i;
    int     ch;
    int     lflags = DNS_REQ_FLAG_NONE;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "f:npv")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	case 'f':
	    dns_rr_filter_compile("DNS reply filter", optarg);
	    break;
	case 'n':
	    lflags |= DNS_REQ_FLAG_NCACHE_TTL;
	    break;
	case 'p':
	    var_dns_ncache_ttl_fix = 1;
	    break;
	default:
	    usage(argv);
	}
    }
    if (argc != optind + 2)
	usage(argv);
    types_argv = argv_split(argv[optind], CHARS_COMMA_SP);
    types = (unsigned *) mymalloc(sizeof(*types) * (types_argv->argc + 1));
    for (i = 0; i < types_argv->argc; i++)
	if ((types[i] = dns_type(types_argv->argv[i])) == 0)
	    msg_fatal("invalid query type: %s", types_argv->argv[i]);
    types[i] = 0;
    argv_free(types_argv);
    name = argv[optind + 1];
    msg_verbose = 1;
    switch (dns_lookup_rv(name, RES_USE_DNSSEC, &rr, fqdn, why,
			  &rcode, lflags, types)) {
    default:
	msg_warn("%s (rcode=%d)", vstring_str(why), rcode);
    case DNS_OK:
	if (rr) {
	    vstream_printf("%s: fqdn: %s\n", name, vstring_str(fqdn));
	    buf = vstring_alloc(100);
	    print_rr(buf, rr);
	    dns_rr_free(rr);
	    vstring_free(buf);
	    vstream_fflush(VSTREAM_OUT);
	}
    }
    myfree((void *) types);
    exit(0);
}
