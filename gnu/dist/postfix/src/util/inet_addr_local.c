/*++
/* NAME
/*	inet_addr_local 3
/* SUMMARY
/*	determine if IP address is local
/* SYNOPSIS
/*	#include <inet_addr_local.h>
/*
/*	int	inet_addr_local(addr_list, mask_list)
/*	INET_ADDR_LIST *addr_list;
/*	INET_ADDR_LIST *mask_list;
/* DESCRIPTION
/*	inet_addr_local() determines all active IP interface addresses
/*	of the local system. Any address found is appended to the
/*	specified address list. The result value is the number of
/*	active interfaces found.
/*
/*	The mask_list is either a null pointer, or it is a list that
/*	receives the netmasks of the interface addresses that were found.
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
#include <string.h>
#if defined(INET6) && (defined (LINUX) || defined (LINUX2))
#include <netdb.h>
#include <stdio.h>
#endif
#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

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
#define IFREQ_SIZE(ifr)	_SIZEOF_ADDR_IFREQ(*ifr)
#else
#ifdef HAS_SA_LEN
#define NEXT_INTERFACE(ifr) ((struct ifreq *) \
	((char *) ifr + sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len))
#define IFREQ_SIZE(ifr)	(sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len)
#else
#define NEXT_INTERFACE(ifr) (ifr + 1)
#define IFREQ_SIZE(ifr)	sizeof(ifr[0])
#endif
#endif

/* inet_addr_local - find all IP addresses for this host */

int     inet_addr_local(INET_ADDR_LIST *addr_list, INET_ADDR_LIST *mask_list)
{
#ifdef HAVE_GETIFADDRS
    char *myname = "inet_addr_local";
    struct ifaddrs *ifap, *ifa;
    int initial_count = addr_list->used;
    struct sockaddr *sa;
#ifdef INET6
#ifdef __KAME__
    struct sockaddr_in6 addr6;
#endif
#else
    void *addr;
#endif

    if (getifaddrs(&ifap) < 0)
	msg_fatal("%s: getifaddrs: %m", myname);

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	sa = ifa->ifa_addr;
	switch (ifa->ifa_addr->sa_family) {
	case AF_INET:
#ifndef INET6
	    addr = (void *)&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
#endif
	    break;
#ifdef INET6
	case AF_INET6:
#ifdef __KAME__
	    memcpy(&addr6, ifa->ifa_addr, ifa->ifa_addr->sa_len);
	    /* decode scoped address notation */
	    if ((IN6_IS_ADDR_LINKLOCAL(&addr6.sin6_addr) ||
	         IN6_IS_ADDR_SITELOCAL(&addr6.sin6_addr)) &&
		addr6.sin6_scope_id == 0) {
		addr6.sin6_scope_id = ntohs(addr6.sin6_addr.s6_addr[3] |
		    (unsigned int)addr6.sin6_addr.s6_addr[2] << 8);
		addr6.sin6_addr.s6_addr[2] = addr6.sin6_addr.s6_addr[3] = 0;
		sa = (struct sockaddr *)&addr6;
	    }
#endif
	    break;
#endif
	default:
	    continue;
	}

#ifdef INET6
	inet_addr_list_append(addr_list, sa);
#else
	inet_addr_list_append(addr_list, (struct in_addr *)addr);
#endif
    }

    freeifaddrs(ifap);
    return (addr_list->used - initial_count);
#else
    char   *myname = "inet_addr_local";
    struct ifconf ifc;
    struct ifreq *ifr;
    struct ifreq *the_end;
    int     sock;
    VSTRING *buf;
    int     initial_count = addr_list->used;
    struct in_addr addr;
    struct ifreq *ifr_mask;
    int af = AF_INET;
#ifdef INET6
#if defined (LINUX) || defined (LINUX2)
#define _PATH_PROCNET_IFINET6   "/proc/net/if_inet6"
    FILE *f;
    char addr6p[8][5], addr6res[40], devname[20];
    int plen, scope, dad_status, if_idx, gaierror;
    struct addrinfo hints, *res, *res0;
#endif
    struct sockaddr_in6 addr6;

other_socket_type:
#endif
    buf = vstring_alloc(1024);

    if ((sock = socket(af, SOCK_DGRAM, 0)) < 0) {
#ifdef INET6
	if (af == AF_INET6)
	{
	    if (msg_verbose)
		    msg_warn("%s: socket: %m", myname);
	    goto end;
	}
	else
#endif
	msg_fatal("%s: socket: %m", myname);
    }

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
	if ((ifr->ifr_addr.sa_family == AF_INET) &&
			(ifr->ifr_addr.sa_family == af)) { /* IP interface */
	    addr = ((struct sockaddr_in *) & ifr->ifr_addr)->sin_addr;
	    if (addr.s_addr != INADDR_ANY) {	/* has IP address */
#ifdef INET6
		inet_addr_list_append(addr_list, &ifr->ifr_addr);
#else
		inet_addr_list_append(addr_list, &addr);
#endif
		if (mask_list) {
		    ifr_mask = (struct ifreq *) mymalloc(IFREQ_SIZE(ifr));
		    memcpy((char *) ifr_mask, (char *) ifr, IFREQ_SIZE(ifr));
		    if (ioctl(sock, SIOCGIFNETMASK, ifr_mask) < 0)
			msg_fatal("%s: ioctl SIOCGIFNETMASK: %m", myname);
		    addr = ((struct sockaddr_in *) & ifr_mask->ifr_addr)->sin_addr;
		    inet_addr_list_append(mask_list, &addr);
		    myfree((char *) ifr_mask);
		}
	    }
	}
#ifdef INET6
	else if ((ifr->ifr_addr.sa_family == AF_INET6) &&
			(ifr->ifr_addr.sa_family == af)) {  /* IPv6 interface */
	    addr6 = *((struct sockaddr_in6 *) & ifr->ifr_addr);
#ifdef __KAME__
	    /* decode scoped address notation */
	    if ((IN6_IS_ADDR_LINKLOCAL(&addr6.sin6_addr) ||
	         IN6_IS_ADDR_SITELOCAL(&addr6.sin6_addr)) &&
		addr6.sin6_scope_id == 0) {
		addr6.sin6_scope_id = ntohs(addr6.sin6_addr.s6_addr[3] |
		    (unsigned int)addr6.sin6_addr.s6_addr[2] << 8);
		addr6.sin6_addr.s6_addr[2] = addr6.sin6_addr.s6_addr[3] = 0;
	    }
#endif
	    if (!(IN6_IS_ADDR_UNSPECIFIED(&addr6.sin6_addr)))
	        inet_addr_list_append(addr_list, (struct sockaddr *)&addr6);
	}
#endif
	ifr = NEXT_INTERFACE(ifr);
    }
    vstring_free(buf);
    (void) close(sock);
#ifdef INET6
end:
    if (af != AF_INET6) {
	    af = AF_INET6;
	    goto other_socket_type;
    }
#if defined (LINUX) || defined (LINUX2)
    if ((f = fopen(_PATH_PROCNET_IFINET6, "r")) != NULL) {
         while (fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
	       addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
	       addr6p[5], addr6p[6], addr6p[7],
	       &if_idx, &plen, &scope, &dad_status, devname) != EOF) {
		 sprintf(addr6res, "%s:%s:%s:%s:%s:%s:%s:%s",
				 addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				 addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
		 addr6res[sizeof(addr6res) - 1] = 0;
		 memset(&hints, 0, sizeof(hints));
		 hints.ai_flags = AI_NUMERICHOST;
		 hints.ai_family = AF_UNSPEC;
		 hints.ai_socktype = SOCK_DGRAM;
		 gaierror = getaddrinfo(addr6res, NULL, &hints, &res0);
		 if (!gaierror) {
			 for (res = res0; res; res = res->ai_next) {
			       inet_addr_list_append(addr_list, res->ai_addr);
			 }
			 freeaddrinfo(res0);
		 }
	 }
    }
#endif /* linux */
#endif
    return (addr_list->used - initial_count);
#endif
}

#ifdef TEST

#include <vstream.h>
#include <msg_vstream.h>

int     main(int unused_argc, char **argv)
{
    INET_ADDR_LIST addr_list;
    INET_ADDR_LIST mask_list;
    int     i;
    char abuf[NI_MAXHOST], mbuf[NI_MAXHOST];
    struct sockaddr *sa;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    inet_addr_list_init(&addr_list);
    inet_addr_list_init(&mask_list);
    inet_addr_local(&addr_list, &mask_list);

    if (addr_list.used == 0)
	msg_fatal("cannot find any active network interfaces");

    if (addr_list.used == 1)
	msg_warn("found only one active network interface");

    for (i = 0; i < addr_list.used; i++) {
	sa = (struct sockaddr *)&addr_list.addrs[i];
	if (getnameinfo(sa, sa->sa_len, abuf, sizeof(abuf), NULL, 0,
		NI_NUMERICHOST)) {
	    strncpy(buf, "???", sizeof(abuf));
	}
	sa = (struct sockaddr *)&mask_list.addrs[i];
	if (getnameinfo(sa, sa->sa_len, mbuf, sizeof(mbuf), NULL, 0,
		NI_NUMERICHOST)) {
	    strncpy(buf, "???", sizeof(buf));
	}
	vstream_printf("%s/%s\n", abuf, mbuf);
    }
    vstream_fflush(VSTREAM_OUT);
    inet_addr_list_free(&addr_list);
    inet_addr_list_free(&mask_list);
}

#endif
