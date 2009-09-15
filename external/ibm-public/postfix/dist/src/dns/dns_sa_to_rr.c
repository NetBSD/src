/*	$NetBSD: dns_sa_to_rr.c,v 1.1.1.1.2.2 2009/09/15 06:02:36 snj Exp $	*/

/*++
/* NAME
/*	dns_sa_to_rr 3
/* SUMMARY
/*	socket address to resource record
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	DNS_RR	*dns_sa_to_rr(hostname, pref, sa)
/*	const char *hostname;
/*	unsigned pref;
/*	struct sockaddr *sa;
/* DESCRIPTION
/*	dns_sa_to_rr() converts a socket address into a DNS resource record.
/*
/*	Arguments:
/* .IP hostname
/*	The resource record host name.
/* .IP pref
/*	The resource record MX host preference, if applicable.
/* .IP sa
/*	Binary address.
/* DIAGNOSTICS
/*	The result is a null pointer in case of problems, with the
/*	errno variable set to indicate the problem type.
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

/* System libraries. */

#include <sys_defs.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>

/* DNS library. */

#include <dns.h>

/* dns_sa_to_rr - socket address to resource record */

DNS_RR *dns_sa_to_rr(const char *hostname, unsigned pref, struct sockaddr * sa)
{
#define DUMMY_TTL	0

    if (sa->sa_family == AF_INET) {
	return (dns_rr_create(hostname, hostname, T_A, C_IN, DUMMY_TTL, pref,
			      (char *) &SOCK_ADDR_IN_ADDR(sa),
			      sizeof(SOCK_ADDR_IN_ADDR(sa))));
#ifdef HAS_IPV6
    } else if (sa->sa_family == AF_INET6) {
	return (dns_rr_create(hostname, hostname, T_AAAA, C_IN, DUMMY_TTL, pref,
			      (char *) &SOCK_ADDR_IN6_ADDR(sa),
			      sizeof(SOCK_ADDR_IN6_ADDR(sa))));
#endif
    } else {
	errno = EAFNOSUPPORT;
	return (0);
    }
}

 /*
  * Stand-alone test program.
  */
#ifdef TEST
#include <vstream.h>
#include <myaddrinfo.h>
#include <inet_proto.h>

static const char *myname;

static NORETURN usage(void)
{
    msg_fatal("usage: %s hostname", myname);
}

int     main(int argc, char **argv)
{
    MAI_HOSTADDR_STR hostaddr;
    struct addrinfo *res0;
    struct addrinfo *res;
    DNS_RR *rr;
    int     aierr;

    myname = argv[0];
    if (argc < 2)
	usage();

    inet_proto_init(argv[0], INET_PROTO_NAME_ALL);

    while (*++argv) {
	if ((aierr = hostname_to_sockaddr(argv[0], (char *) 0, 0, &res0)) != 0)
	    msg_fatal("%s: %s", argv[0], MAI_STRERROR(aierr));
	for (res = res0; res != 0; res = res->ai_next) {
	    if ((rr = dns_sa_to_rr(argv[0], 0, res->ai_addr)) == 0)
		msg_fatal("dns_sa_to_rr: %m");
	    if (dns_rr_to_pa(rr, &hostaddr) == 0)
		msg_fatal("dns_rr_to_pa: %m");
	    vstream_printf("%s -> %s\n", argv[0], hostaddr.buf);
	    vstream_fflush(VSTREAM_OUT);
	    dns_rr_free(rr);
	}
	freeaddrinfo(res0);
    }
    return (0);
}

#endif
