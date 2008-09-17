/*++
/* NAME
/*	dns_rr_to_sa 3
/* SUMMARY
/*	resource record to socket address
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	int	dns_rr_to_sa(rr, port, sa, sa_length)
/*	DNS_RR	*rr;
/*	unsigned port;
/*	struct sockaddr *sa;
/*	SOCKADDR_SIZE *sa_length;
/* DESCRIPTION
/*	dns_rr_to_sa() converts the address in a DNS resource record into
/*	a socket address of the corresponding type.
/*
/*	Arguments:
/* .IP rr
/*	DNS resource record pointer.
/* .IP port
/*	TCP or UDP port, network byte order.
/* .IP sa
/*	Socket address pointer.
/* .IP sa_length
/*	On input, the available socket address storage space.
/*	On output, the amount of space actually used.
/* DIAGNOSTICS
/*	The result is non-zero in case of problems, with the
/*	error type returned via the errno variable.
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

/* dns_rr_to_sa - resource record to socket address */

int     dns_rr_to_sa(DNS_RR *rr, unsigned port, struct sockaddr * sa,
		             SOCKADDR_SIZE *sa_length)
{
    SOCKADDR_SIZE sock_addr_len;

    if (rr->type == T_A) {
	if (rr->data_len != sizeof(SOCK_ADDR_IN_ADDR(sa))) {
	    errno = EINVAL;
	    return (-1);
	} else if ((sock_addr_len = sizeof(*SOCK_ADDR_IN_PTR(sa))) > *sa_length) {
	    errno = ENOSPC;
	    return (-1);
	} else {
	    memset((char *) SOCK_ADDR_IN_PTR(sa), 0, sock_addr_len);
	    SOCK_ADDR_IN_FAMILY(sa) = AF_INET;
	    SOCK_ADDR_IN_PORT(sa) = port;
	    SOCK_ADDR_IN_ADDR(sa) = IN_ADDR(rr->data);
#ifdef HAS_SA_LEN
	    sa->sa_len = sock_addr_len;
#endif
	    *sa_length = sock_addr_len;
	    return (0);
	}
#ifdef HAS_IPV6
    } else if (rr->type == T_AAAA) {
	if (rr->data_len != sizeof(SOCK_ADDR_IN6_ADDR(sa))) {
	    errno = EINVAL;
	    return (-1);
	} else if ((sock_addr_len = sizeof(*SOCK_ADDR_IN6_PTR(sa))) > *sa_length) {
	    errno = ENOSPC;
	    return (-1);
	} else {
	    memset((char *) SOCK_ADDR_IN6_PTR(sa), 0, sock_addr_len);
	    SOCK_ADDR_IN6_FAMILY(sa) = AF_INET6;
	    SOCK_ADDR_IN6_PORT(sa) = port;
	    SOCK_ADDR_IN6_ADDR(sa) = IN6_ADDR(rr->data);
#ifdef HAS_SA_LEN
	    sa->sa_len = sock_addr_len;
#endif
	    *sa_length = sock_addr_len;
	    return (0);
	}
#endif
    } else {
	errno = EAFNOSUPPORT;
	return (-1);
    }
}

 /*
  * Stand-alone test program.
  */
#ifdef TEST
#include <stdlib.h>

#include <stringops.h>
#include <vstream.h>
#include <myaddrinfo.h>

static const char *myname;

static NORETURN usage(void)
{
    msg_fatal("usage: %s dnsaddrtype hostname portnumber", myname);
}

int     main(int argc, char **argv)
{
    DNS_RR *rr;
    MAI_HOSTADDR_STR hostaddr;
    MAI_SERVPORT_STR portnum;
    struct sockaddr_storage ss;
    struct sockaddr *sa = (struct sockaddr *) & ss;
    SOCKADDR_SIZE sa_length = sizeof(ss);
    VSTRING *why;
    int     type;
    int     port;

    myname = argv[0];
    if (argc < 4)
	usage();
    why = vstring_alloc(1);

    while (*++argv) {
	if (argv[1] == 0 || argv[2] == 0)
	    usage();
	if ((type = dns_type(argv[0])) == 0)
	    usage();
	if (!alldig(argv[2]) || (port = atoi(argv[2])) > 65535)
	    usage();
	if (dns_lookup(argv[1], type, 0, &rr, (VSTRING *) 0, why) != DNS_OK)
	    msg_fatal("%s: %s", argv[1], vstring_str(why));
	sa_length = sizeof(ss);
	if (dns_rr_to_sa(rr, htons(port), sa, &sa_length) != 0)
	    msg_fatal("dns_rr_to_sa: %m");
	SOCKADDR_TO_HOSTADDR(sa, sa_length, &hostaddr, &portnum, 0);
	vstream_printf("%s %s -> %s %s\n",
		       argv[1], argv[2], hostaddr.buf, portnum.buf);
	vstream_fflush(VSTREAM_OUT);
	argv += 2;
	dns_rr_free(rr);
    }
    vstring_free(why);
    return (0);
}

#endif
