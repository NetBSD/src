/*++
/* NAME
/*	inet_addr_local 3
/* SUMMARY
/*	determine if IP address is local
/* SYNOPSIS
/*	#include <inet_addr_local.h>
/*
/*	int	inet_addr_local(list)
/*	INET_ADDR_LIST *list;
/* DESCRIPTION
/*	inet_addr_local() determines all active IP interface addresses
/*	of the local system. Any address found is appended to the
/*	specified address list. The result value is the number of
/*	active interfaces found.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* SEE ALSO
/*	inet_addr_list(3) address list management
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
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef USE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <inet_addr_list.h>
#include <inet_addr_local.h>

 /*
  * Support for variable-length addresses.
  */
#ifdef _SIZEOF_ADDR_IFREQ
#define NEXT_INTERFACE(ifr) ((struct ifreq *) \
	((char *) ifr + _SIZEOF_ADDR_IFREQ(*ifr)))
#else
#ifdef HAS_SA_LEN
#define NEXT_INTERFACE(ifr) ((struct ifreq *) \
	((char *) ifr + sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len))
#else
#define NEXT_INTERFACE(ifr) (ifr + 1)
#endif
#endif

/* inet_addr_local - find all IP addresses for this host */

int     inet_addr_local(INET_ADDR_LIST *addr_list)
{
    const char *myname = "inet_addr_local";
    struct ifconf ifc;
    struct ifreq *ifr;
    struct ifreq *the_end;
    int     sock;
    VSTRING *buf = vstring_alloc(1024);
    int     initial_count = addr_list->used;
    struct in_addr addr;

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    /*
     * Get the network interface list. XXX The socket API appears to have no
     * function that returns the number of network interfaces, so we have to
     * guess how much space is needed to store the result.
     * 
     * On BSD-derived systems, ioctl SIOCGIFCONF returns as much information as
     * possible, leaving it up to the application to repeat the request with
     * a larger buffer if the result caused a tight fit.
     * 
     * Other systems, such as Solaris 2.5, generate an EINVAL error when the
     * buffer is too small for the entire result. Workaround: ignore EINVAL
     * errors and repeat the request with a larger buffer. The downside is
     * that the program can run out of memory due to a non-memory problem,
     * making it more difficult than necessary to diagnose the real problem.
     */
    for (;;) {
	ifc.ifc_len = vstring_avail(buf);
	ifc.ifc_buf = vstring_str(buf);
	if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0) {
	    if (errno != EINVAL)
		msg_fatal("%s: ioctl SIOCGIFCONF: %m", myname);
	} else if (ifc.ifc_len < vstring_avail(buf) / 2)
	    break;
	VSTRING_SPACE(buf, vstring_avail(buf) * 2);
    }

    /*
     * Get the address of each IP network interface. According to BIND we
     * must include interfaces that are down because the machine may still
     * receive packets for that address (yes, via some other interface).
     * Having no way to verify this claim on every machine, I will give them
     * the benefit of the doubt.
     */
    the_end = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);
    for (ifr = ifc.ifc_req; ifr < the_end;) {
	if (ifr->ifr_addr.sa_family == AF_INET) {	/* IP interface */
	    addr = ((struct sockaddr_in *) & ifr->ifr_addr)->sin_addr;
	    if (addr.s_addr != INADDR_ANY)	/* has IP address */
		inet_addr_list_append(addr_list, &addr);
	}
	ifr = NEXT_INTERFACE(ifr);
    }
    vstring_free(buf);
    (void) close(sock);
    return (addr_list->used - initial_count);
}

#ifdef TEST

#include <vstream.h>
#include <msg_vstream.h>

int     main(int unused_argc, char **argv)
{
    INET_ADDR_LIST addr_list;
    int     i;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    inet_addr_list_init(&addr_list);
    inet_addr_local(&addr_list);

    if (addr_list.used == 0)
	msg_fatal("cannot find any active network interfaces");

    if (addr_list.used == 1)
	msg_warn("found only one active network interface");

    for (i = 0; i < addr_list.used; i++)
	vstream_printf("%s\n", inet_ntoa(addr_list.addrs[i]));
    vstream_fflush(VSTREAM_OUT);
    inet_addr_list_free(&addr_list);
}

#endif
