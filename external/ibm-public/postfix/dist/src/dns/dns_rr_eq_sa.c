/*	$NetBSD: dns_rr_eq_sa.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	dns_rr_eq_sa 3
/* SUMMARY
/*	compare resource record with socket address
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	int	dns_rr_eq_sa(DNS_RR *rr, struct sockaddr *sa)
/*	DNS_RR	*rr;
/*	struct sockaddr *sa;
/*
/*	int	DNS_RR_EQ_SA(DNS_RR *rr, struct sockaddr *sa)
/*	DNS_RR	*rr;
/*	struct sockaddr *sa;
/* DESCRIPTION
/*	dns_rr_eq_sa() compares a DNS resource record with a socket
/*	address.  The result is non-zero when the resource type
/*	matches the socket address family, and when the network
/*	address information is identical.
/*
/*	DNS_RR_EQ_SA() is an unsafe macro version for those who live fast.
/*
/*	Arguments:
/* .IP rr
/*	DNS resource record pointer.
/* .IP sa
/*	Binary address pointer.
/* DIAGNOSTICS
/*	Panic: unknown socket address family.
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

/* Utility library. */

#include <msg.h>
#include <sock_addr.h>

/* DNS library. */

#include <dns.h>

/* dns_rr_eq_sa - compare resource record with socket address */

int     dns_rr_eq_sa(DNS_RR *rr, struct sockaddr * sa)
{
    const char *myname = "dns_rr_eq_sa";

    if (sa->sa_family == AF_INET) {
	return (rr->type == T_A
		&& SOCK_ADDR_IN_ADDR(sa).s_addr == IN_ADDR(rr->data).s_addr);
#ifdef HAS_IPV6
    } else if (sa->sa_family == AF_INET6) {
	return (rr->type == T_AAAA
		&& memcmp((char *) &SOCK_ADDR_IN6_ADDR(sa),
			  rr->data, rr->data_len) == 0);
#endif
    } else {
	msg_panic("%s: unsupported socket address family type: %d",
		  myname, sa->sa_family);
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
    msg_fatal("usage: %s hostname address", myname);
}

int     main(int argc, char **argv)
{
    MAI_HOSTADDR_STR hostaddr;
    DNS_RR *rr;
    struct addrinfo *res0;
    struct addrinfo *res1;
    struct addrinfo *res;
    int     aierr;

    myname = argv[0];

    if (argc < 3)
	usage();

    inet_proto_init(argv[0], INET_PROTO_NAME_ALL);

    while (*++argv) {
	if (argv[1] == 0)
	    usage();

	if ((aierr = hostaddr_to_sockaddr(argv[1], (char *) 0, 0, &res1)) != 0)
	    msg_fatal("host address %s: %s", argv[1], MAI_STRERROR(aierr));
	if ((rr = dns_sa_to_rr(argv[1], 0, res1->ai_addr)) == 0)
	    msg_fatal("dns_sa_to_rr: %m");
	freeaddrinfo(res1);

	if ((aierr = hostname_to_sockaddr(argv[0], (char *) 0, 0, &res0)) != 0)
	    msg_fatal("host name %s: %s", argv[0], MAI_STRERROR(aierr));
	for (res = res0; res != 0; res = res->ai_next) {
	    SOCKADDR_TO_HOSTADDR(res->ai_addr, res->ai_addrlen,
				 &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	    vstream_printf("%s =?= %s\n", hostaddr.buf, argv[1]);
	    vstream_printf("tested by function: %s\n",
			   dns_rr_eq_sa(rr, res->ai_addr) ?
			   "yes" : "no");
	    vstream_printf("tested by macro:    %s\n",
			   DNS_RR_EQ_SA(rr, res->ai_addr) ?
			   "yes" : "no");
	}
	dns_rr_free(rr);
	freeaddrinfo(res0);
	vstream_fflush(VSTREAM_OUT);
	argv += 1;
    }
    return (0);
}

#endif
