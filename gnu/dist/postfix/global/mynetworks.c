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
/*	networks. The patterns are conservative: they match whole
/*	class A, B, C or D networks. This is usually sufficient to
/*	distinguish between organizations.
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
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef IN_CLASSD_NET
#define IN_CLASSD_NET		0xf0000000
#define IN_CLASSD_NSHIFT 	28
#endif

#define BITS_PER_ADDR		32

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <inet_addr_list.h>

/* Global library. */

#include <own_inet_addr.h>
#include <mynetworks.h>

/* mynetworks - return patterns that match my own networks */

const char *mynetworks(void)
{
    static VSTRING *result;

    if (result == 0) {
	char   *myname = "mynetworks";
	INET_ADDR_LIST *my_addr_list;
	unsigned long addr;
	unsigned long mask;
	struct in_addr net;
	int     shift;
	int     i;

	result = vstring_alloc(20);
	my_addr_list = own_inet_addr_list();

	for (i = 0; i < my_addr_list->used; i++) {
	    addr = ntohl(my_addr_list->addrs[i].s_addr);
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
		msg_fatal("%s: bad address class: %s",
			  myname, inet_ntoa(my_addr_list->addrs[i]));
	    }
	    net.s_addr = htonl(addr & mask);
	    vstring_sprintf_append(result, "%s/%d ",
				   inet_ntoa(net), BITS_PER_ADDR - shift);
	}
	if (msg_verbose)
	    msg_info("%s: %s", myname, vstring_str(result));
    }
    return (vstring_str(result));
}

#ifdef TEST

char   *var_inet_interfaces;

main(int argc, char **argv)
{
    if (argc != 2)
	msg_fatal("usage: %s interface_list", argv[0]);
    msg_verbose = 10;
    var_inet_interfaces = argv[1];
    mynetworks();
}

#endif
