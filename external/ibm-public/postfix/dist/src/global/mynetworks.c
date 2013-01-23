/*	$NetBSD: mynetworks.c,v 1.1.1.1.10.1 2013/01/23 00:05:03 yamt Exp $	*/

/*++
/* NAME
/*	mynetworks 3
/* SUMMARY
/*	generate patterns for my own interface addresses
/* SYNOPSIS
/*	#include <mynetworks.h>
/*
/*	const char *mynetworks()
/* DESCRIPTION
/*	This routine uses the address list built by own_inet_addr()
/*	to produce a list of patterns that match the corresponding
/*	networks.
/*
/*	The interface list is specified with the "inet_interfaces"
/*	configuration parameter.
/*
/*	The address to netblock conversion style is specified with
/*	the "mynetworks_style" parameter: one of "class" (match
/*	whole class A, B, C or D networks), "subnet" (match local
/*	subnets), or "host" (match local interfaces only).
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
/*	Dean C. Strik
/*	Department ICT Services
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB  Eindhoven, Netherlands
/*	E-mail: <dean@ipnet6.org>
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef IN_CLASSD_NET
#define IN_CLASSD_NET		0xf0000000
#define IN_CLASSD_NSHIFT 	28
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <inet_addr_list.h>
#include <name_mask.h>
#include <myaddrinfo.h>
#include <mask_addr.h>
#include <argv.h>
#include <inet_proto.h>

/* Global library. */

#include <own_inet_addr.h>
#include <mail_params.h>
#include <mynetworks.h>
#include <sock_addr.h>
#include <been_here.h>

/* Application-specific. */

#define MASK_STYLE_CLASS	(1 << 0)
#define MASK_STYLE_SUBNET	(1 << 1)
#define MASK_STYLE_HOST		(1 << 2)

static const NAME_MASK mask_styles[] = {
    MYNETWORKS_STYLE_CLASS, MASK_STYLE_CLASS,
    MYNETWORKS_STYLE_SUBNET, MASK_STYLE_SUBNET,
    MYNETWORKS_STYLE_HOST, MASK_STYLE_HOST,
    0,
};

/* mynetworks - return patterns that match my own networks */

const char *mynetworks(void)
{
    static VSTRING *result;

    if (result == 0) {
	const char *myname = "mynetworks";
	INET_ADDR_LIST *my_addr_list;
	INET_ADDR_LIST *my_mask_list;
	unsigned shift;
	unsigned junk;
	int     i;
	unsigned mask_style;
	struct sockaddr_storage *sa;
	struct sockaddr_storage *ma;
	int     net_mask_count = 0;
	ARGV   *argv;
	BH_TABLE *dup_filter;
	char  **cpp;

	/*
	 * Avoid run-time errors when all network protocols are disabled. We
	 * can't look up interface information, and we can't convert explicit
	 * names or addresses.
	 */
	if (inet_proto_info()->ai_family_list[0] == 0) {
	    if (msg_verbose)
		msg_info("skipping %s setting - "
			 "all network protocols are disabled",
			 VAR_MYNETWORKS);
	    result = vstring_alloc(1);
	    return (vstring_str(result));
	}
	mask_style = name_mask("mynetworks mask style", mask_styles,
			       var_mynetworks_style);

	/*
	 * XXX Workaround: name_mask() needs a flags argument so that we can
	 * require exactly one value, or we need to provide an API that is
	 * dedicated for single-valued flags.
	 */
	for (i = 0, junk = mask_style; junk != 0; junk >>= 1U)
	    i += (junk & 1);
	if (i != 1)
	    msg_fatal("bad %s value: %s; specify exactly one value",
		      VAR_MYNETWORKS_STYLE, var_mynetworks_style);

	result = vstring_alloc(20);
	my_addr_list = own_inet_addr_list();
	my_mask_list = own_inet_mask_list();

	for (sa = my_addr_list->addrs, ma = my_mask_list->addrs;
	     sa < my_addr_list->addrs + my_addr_list->used;
	     sa++, ma++) {
	    unsigned long addr;
	    unsigned long mask;
	    struct in_addr net;

	    if (SOCK_ADDR_FAMILY(sa) == AF_INET) {
		addr = ntohl(SOCK_ADDR_IN_ADDR(sa).s_addr);
		mask = ntohl(SOCK_ADDR_IN_ADDR(ma).s_addr);

		switch (mask_style) {

		    /*
		     * Natural mask. This is dangerous if you're customer of
		     * an ISP who gave you a small portion of their network.
		     */
		case MASK_STYLE_CLASS:
		    if (IN_CLASSA(addr)) {
			mask = IN_CLASSA_NET;
			shift = IN_CLASSA_NSHIFT;
		    } else if (IN_CLASSB(addr)) {
			mask = IN_CLASSB_NET;
			shift = IN_CLASSB_NSHIFT;
		    } else if (IN_CLASSC(addr)) {
			mask = IN_CLASSC_NET;
			shift = IN_CLASSC_NSHIFT;
		    } else if (IN_CLASSD(addr)) {
			mask = IN_CLASSD_NET;
			shift = IN_CLASSD_NSHIFT;
		    } else {
			msg_fatal("%s: unknown address class: %s",
				  myname, inet_ntoa(SOCK_ADDR_IN_ADDR(sa)));
		    }
		    break;

		    /*
		     * Subnet mask. This is less unsafe, but still bad if
		     * you're connected to a large subnet.
		     */
		case MASK_STYLE_SUBNET:
		    for (junk = mask, shift = MAI_V4ADDR_BITS; junk != 0;
			 shift--, junk <<= 1)
			 /* void */ ;
		    break;

		    /*
		     * Host only. Do not relay authorize other hosts.
		     */
		case MASK_STYLE_HOST:
		    mask = ~0;
		    shift = 0;
		    break;

		default:
		    msg_panic("unknown mynetworks mask style: %s",
			      var_mynetworks_style);
		}
		net.s_addr = htonl(addr & mask);
		vstring_sprintf_append(result, "%s/%d ",
				   inet_ntoa(net), MAI_V4ADDR_BITS - shift);
		net_mask_count++;
		continue;
	    }
#ifdef HAS_IPV6
	    else if (SOCK_ADDR_FAMILY(sa) == AF_INET6) {
		MAI_HOSTADDR_STR hostaddr;
		unsigned char *ac;
		unsigned char *end;
		unsigned char ch;
		struct sockaddr_in6 net6;

		switch (mask_style) {

		    /*
		     * There are no classes for IPv6. We default to subnets
		     * instead.
		     */
		case MASK_STYLE_CLASS:

		    /* FALLTHROUGH */

		    /*
		     * Subnet mask.
		     */
		case MASK_STYLE_SUBNET:
		    ac = (unsigned char *) &SOCK_ADDR_IN6_ADDR(ma);
		    end = ac + sizeof(SOCK_ADDR_IN6_ADDR(ma));
		    shift = MAI_V6ADDR_BITS;
		    while (ac < end) {
			if ((ch = *ac++) == (unsigned char) -1) {
			    shift -= CHAR_BIT;
			    continue;
			} else {
			    while (ch != 0)
				shift--, ch <<= 1;
			    break;
			}
		    }
		    break;

		    /*
		     * Host only. Do not relay authorize other hosts.
		     */
		case MASK_STYLE_HOST:
		    shift = 0;
		    break;

		default:
		    msg_panic("unknown mynetworks mask style: %s",
			      var_mynetworks_style);
		}
		/* FIX 200501: IPv6 patch did not clear host bits. */
		net6 = *SOCK_ADDR_IN6_PTR(sa);
		mask_addr((unsigned char *) &net6.sin6_addr,
			  sizeof(net6.sin6_addr),
			  MAI_V6ADDR_BITS - shift);
		SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(&net6), SOCK_ADDR_LEN(&net6),
				     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
		vstring_sprintf_append(result, "[%s]/%d ",
				     hostaddr.buf, MAI_V6ADDR_BITS - shift);
		net_mask_count++;
		continue;
	    }
#endif
	    else {
		msg_warn("%s: skipping unknown address family %d",
			 myname, SOCK_ADDR_FAMILY(sa));
		continue;
	    }
	}

	/*
	 * FIX 200501 IPv6 patch produced repeated results. Some systems
	 * report the same interface multiple times, notably multi-homed
	 * systems with IPv6 link-local or site-local addresses. A
	 * straight-forward sort+uniq produces ugly results, though. Instead
	 * we preserve the original order and use a duplicate filter to
	 * suppress repeated information.
	 */
	if (net_mask_count > 1) {
	    argv = argv_split(vstring_str(result), " ");
	    VSTRING_RESET(result);
	    dup_filter = been_here_init(net_mask_count, BH_FLAG_NONE);
	    for (cpp = argv->argv; cpp < argv->argv + argv->argc; cpp++)
		if (!been_here_fixed(dup_filter, *cpp))
		    vstring_sprintf_append(result, "%s ", *cpp);
	    argv_free(argv);
	    been_here_free(dup_filter);
	}
	if (msg_verbose)
	    msg_info("%s: %s", myname, vstring_str(result));
    }
    return (vstring_str(result));
}

#ifdef TEST
#include <inet_proto.h>

char   *var_inet_interfaces;
char   *var_mynetworks_style;

int     main(int argc, char **argv)
{
    INET_PROTO_INFO *proto_info;

    if (argc != 4)
	msg_fatal("usage: %s protocols mask_style interface_list (e.g. \"all subnet all\")",
		  argv[0]);
    msg_verbose = 10;
    proto_info = inet_proto_init(argv[0], argv[1]);
    var_mynetworks_style = argv[2];
    var_inet_interfaces = argv[3];
    mynetworks();
    return (0);
}

#endif
