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
#include <name_mask.h>
#ifdef INET6
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

/* Global library. */

#include <own_inet_addr.h>
#include <mail_params.h>
#include <mynetworks.h>

/* Application-specific. */

#define MASK_STYLE_CLASS	(1 << 0)
#define MASK_STYLE_SUBNET	(1 << 1)
#define MASK_STYLE_HOST		(1 << 2)

static NAME_MASK mask_styles[] = {
    MYNETWORKS_STYLE_CLASS, MASK_STYLE_CLASS,
    MYNETWORKS_STYLE_SUBNET, MASK_STYLE_SUBNET,
    MYNETWORKS_STYLE_HOST, MASK_STYLE_HOST,
    0,
};

/* mynetworks - return patterns that match my own networks */

const char *mynetworks(void)
{
    static VSTRING *result;
#ifdef INET6
    char hbuf[NI_MAXHOST];
#endif

    if (result == 0) {
	char   *myname = "mynetworks";
	INET_ADDR_LIST *my_addr_list;
	INET_ADDR_LIST *my_mask_list;
	unsigned long addr;
	unsigned long mask;
	struct in_addr net;
	int     shift;
	int     junk;
	int     i;
	int     mask_style;
#ifdef INET6
	struct sockaddr *sa;
	SOCKADDR_SIZE salen;
#endif

	mask_style = name_mask("mynetworks mask style", mask_styles,
			       var_mynetworks_style);

	result = vstring_alloc(20);
	my_addr_list = own_inet_addr_list();
	my_mask_list = own_inet_mask_list();

	for (i = 0; i < my_addr_list->used; i++) {
#ifdef INET6
	    sa = (struct sockaddr *)&my_addr_list->addrs[i];
#ifndef HAS_SA_LEN					
	    salen = SA_LEN((struct sockaddr *)&sa);
#else					
	    salen = sa->sa_len;
#endif					
	    if (sa->sa_family != AF_INET) {
	        vstring_sprintf_append(result, "XAATODOmynetworks ");
		continue;
	    }
	    addr = ntohl(((struct sockaddr_in *)sa)->sin_addr.s_addr);
	    mask = ntohl(((struct sockaddr_in *)&my_mask_list->addrs[i])->sin_addr.s_addr);
#else
	    addr = ntohl(my_addr_list->addrs[i].s_addr);
	    mask = ntohl(my_mask_list->addrs[i].s_addr);
#endif

	    switch (mask_style) {

		/*
		 * Natural mask. This is dangerous if you're customer of an
		 * ISP who gave you a small portion of their network.
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
#ifdef INET6
		    if (getnameinfo(sa, salen, hbuf, sizeof(hbuf), NULL, 0,
			    NI_NUMERICHOST))
			strncpy(hbuf, "???", sizeof(hbuf));
		    msg_fatal("%s: bad address class: %s", myname, hbuf);
#else
		    msg_fatal("%s: bad address class: %s",
			      myname, inet_ntoa(my_addr_list->addrs[i]));
#endif
		}
		break;

		/*
		 * Subnet mask. This is safe, but breaks backwards
		 * compatibility when used as default setting.
		 */
	    case MASK_STYLE_SUBNET:
		for (junk = mask, shift = BITS_PER_ADDR; junk != 0; shift--, (junk <<= 1))
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
				   inet_ntoa(net), BITS_PER_ADDR - shift);
	}
	if (msg_verbose)
	    msg_info("%s: %s", myname, vstring_str(result));
    }
    return (vstring_str(result));
}

#ifdef TEST

char   *var_inet_interfaces;
char   *var_mynetworks_style;

int     main(int argc, char **argv)
{
    if (argc != 3)
	msg_fatal("usage: %s mask_style interface_list", argv[0]);
    msg_verbose = 10;
    var_inet_interfaces = argv[2];
    var_mynetworks_style = argv[1];
    mynetworks();
}

#endif
