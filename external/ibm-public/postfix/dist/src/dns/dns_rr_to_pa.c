/*	$NetBSD: dns_rr_to_pa.c,v 1.1.1.1.2.2 2009/09/15 06:02:36 snj Exp $	*/

/*++
/* NAME
/*	dns_rr_to_pa 3
/* SUMMARY
/*	resource record to printable address
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	const char *dns_rr_to_pa(rr, hostaddr)
/*	DNS_RR	*rr;
/*	MAI_HOSTADDR_STR *hostaddr;
/* DESCRIPTION
/*	dns_rr_to_pa() converts the address in a DNS resource record
/*	into printable form and returns a pointer to the result.
/*
/*	Arguments:
/* .IP rr
/*	The DNS resource record.
/* .IP hostaddr
/*	Storage for the printable address.
/* DIAGNOSTICS
/*	The result is null in case of problems, with errno set
/*	to indicate the nature of the problem.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>

/* DNS library. */

#include <dns.h>

/* dns_rr_to_pa - resource record to printable address */

const char *dns_rr_to_pa(DNS_RR *rr, MAI_HOSTADDR_STR *hostaddr)
{
    if (rr->type == T_A) {
	return (inet_ntop(AF_INET, rr->data, hostaddr->buf,
			  sizeof(hostaddr->buf)));
#ifdef HAS_IPV6
    } else if (rr->type == T_AAAA) {
	return (inet_ntop(AF_INET6, rr->data, hostaddr->buf,
			  sizeof(hostaddr->buf)));
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

static const char *myname;

static NORETURN usage(void)
{
    msg_fatal("usage: %s dnsaddrtype hostname", myname);
}

int     main(int argc, char **argv)
{
    DNS_RR *rr;
    MAI_HOSTADDR_STR hostaddr;
    VSTRING *why;
    int     type;

    myname = argv[0];
    if (argc < 3)
	usage();
    why = vstring_alloc(1);

    while (*++argv) {
	if (argv[1] == 0)
	    usage();
	if ((type = dns_type(argv[0])) == 0)
	    usage();
	if (dns_lookup(argv[1], type, 0, &rr, (VSTRING *) 0, why) != DNS_OK)
	    msg_fatal("%s: %s", argv[1], vstring_str(why));
	if (dns_rr_to_pa(rr, &hostaddr) == 0)
	    msg_fatal("dns_rr_to_sa: %m");
	vstream_printf("%s -> %s\n", argv[1], hostaddr.buf);
	vstream_fflush(VSTREAM_OUT);
	argv += 1;
	dns_rr_free(rr);
    }
    vstring_free(why);
    return (0);
}

#endif
