/*	$NetBSD: inet_addr_local.c,v 1.1.1.1.2.2 2009/09/15 06:03:59 snj Exp $	*/

/*++
/* NAME
/*	inet_addr_local 3
/* SUMMARY
/*	determine if IP address is local
/* SYNOPSIS
/*	#include <inet_addr_local.h>
/*
/*	int	inet_addr_local(addr_list, mask_list, addr_family_list)
/*	INET_ADDR_LIST *addr_list;
/*	INET_ADDR_LIST *mask_list;
/*	unsigned *addr_family;
/* DESCRIPTION
/*	inet_addr_local() determines all active IP interface addresses
/*	of the local system. Any address found is appended to the
/*	specified address list. The result value is the number of
/*	active interfaces found.
/*
/*	The mask_list is either a null pointer, or it is a list that
/*	receives the netmasks of the interface addresses that were found.
/*
/*	The addr_family_list specifies one or more of AF_INET or AF_INET6.
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
/*
/*	Dean C. Strik
/*	Department ICT
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB  Eindhoven, Netherlands
/*	E-mail: <dean@ipnet6.org>
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
#ifdef HAS_IPV6				/* Linux only? */
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
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <mask_addr.h>
#include <hex_code.h>

 /*
  * Postfix needs its own interface address information to determine whether
  * or not it is an MX host for some destination; without this information,
  * mail would loop between MX hosts. Postfix also needs its interface
  * addresses to figure out whether or not it is final destination for
  * addresses of the form username@[ipaddress].
  * 
  * Postfix needs its own interface netmask information when no explicit
  * mynetworks setting is given in main.cf, and "mynetworks_style = subnet".
  * The mynetworks parameter controls, among others, what mail clients are
  * allowed to relay mail through Postfix.
  * 
  * Different systems have different ways to find out this information. We will
  * therefore use OS dependent methods. An overview:
  * 
  * - Use getifaddrs() when available.  This supports both IPv4/IPv6 addresses.
  * The implementation however is not present in all major operating systems.
  * 
  * - Use SIOCGLIFCONF when available. This supports both IPv4/IPv6 addresses.
  * With SIOCGLIFNETMASK we can obtain the netmask for either address family.
  * Again, this is not present in all major operating systems.
  * 
  * - On Linux, glibc's getifaddrs(3) has returned IPv4 information for some
  * time, but IPv6 information was not returned until 2.3.3. With older Linux
  * versions we get IPv4 interface information with SIOCGIFCONF, and read
  * IPv6 address/prefix information from a file in the /proc filesystem.
  * 
  * - On other systems we expect SIOCGIFCONF to return IPv6 addresses. Since
  * SIOCGIFNETMASK does not work reliably for IPv6 addresses, we always set
  * the prefix length to /128 (host), and expect the user to configure a more
  * appropriate mynetworks setting if needed.
  * 
  * XXX: Each lookup method is implemented by its own function, so we duplicate
  * some code. In this case, I think this is better than really drowning in
  * the #ifdefs...
  * 
  * -- Dean Strik (dcs)
  */

/* ial_socket - make socket for ioctl() operations */

static int ial_socket(int af)
{
    const char *myname = "inet_addr_local[socket]";
    int     sock;

    /*
     * The host may not be actually configured with IPv6. When IPv6 support
     * is not actually in the kernel, don't consider failure to create an
     * IPv6 socket as fatal. This could be tuned better though. For other
     * families, the error is fatal.
     * 
     * XXX Now that Postfix controls protocol support centrally with the
     * inet_proto(3) module, this workaround should no longer be needed.
     */
    if ((sock = socket(af, SOCK_DGRAM, 0)) < 0) {
#ifdef HAS_IPV6
	if (af == AF_INET6) {
	    if (msg_verbose)
		msg_warn("%s: socket: %m", myname);
	    return (-1);
	}
#endif
	msg_fatal("%s: socket: %m", myname);
    }
    return (sock);
}

#ifdef HAVE_GETIFADDRS

/*
 * The getifaddrs(3) function, introduced by BSD/OS, provides a
 * platform-independent way of requesting interface addresses,
 * including IPv6 addresses. The implementation however is not
 * present in all major operating systems.
 */

/* ial_getifaddrs - determine IP addresses using getifaddrs(3) */

static int ial_getifaddrs(INET_ADDR_LIST *addr_list,
			          INET_ADDR_LIST *mask_list,
			          int af)
{
    const char *myname = "inet_addr_local[getifaddrs]";
    struct ifaddrs *ifap, *ifa;
    struct sockaddr *sa, *sam;

    if (getifaddrs(&ifap) < 0)
	msg_fatal("%s: getifaddrs: %m", myname);

    /*
     * Get the address of each IP network interface. According to BIND we
     * must include interfaces that are down because the machine may still
     * receive packets for that address (yes, via some other interface).
     * Having no way to verify this claim on every machine, I will give them
     * the benefit of the doubt.
     * 
     * FIX 200501: The IPv6 patch did not report NetBSD loopback interfaces;
     * fixed by replacing IFF_RUNNING by IFF_UP.
     * 
     * FIX 200501: The IPV6 patch did not skip wild-card interface addresses
     * (tested on FreeBSD).
     */
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	if (!(ifa->ifa_flags & IFF_UP) || ifa->ifa_addr == 0)
	    continue;
	sa = ifa->ifa_addr;
	sam = ifa->ifa_netmask;
	if (af != AF_UNSPEC && sa->sa_family != af)
	    continue;
	switch (sa->sa_family) {
	case AF_INET:
	    if (SOCK_ADDR_IN_ADDR(sa).s_addr == INADDR_ANY)
		continue;
	    break;
#ifdef HAS_IPV6
	case AF_INET6:
	    if (IN6_IS_ADDR_UNSPECIFIED(&SOCK_ADDR_IN6_ADDR(sa)))
		continue;
	    break;
#endif
	default:
	    continue;
	}

	inet_addr_list_append(addr_list, sa);
	if (mask_list != 0) {

	    /*
	     * Unfortunately, sa_len/sa_family may be broken in the netmask
	     * sockaddr structure. We must fix this manually to have correct
	     * addresses.   --dcs
	     */
#ifdef HAS_SA_LEN
	    sam->sa_len = sa->sa_family == AF_INET6 ?
		sizeof(struct sockaddr_in6) :
		sizeof(struct sockaddr_in);
#endif
	    sam->sa_family = sa->sa_family;
	    inet_addr_list_append(mask_list, sam);
	}
    }
    freeifaddrs(ifap);
    return (0);
}

#endif					/* HAVE_GETIFADDRS */

#ifdef HAS_SIOCGLIF

/*
 * The SIOCLIF* ioctls are the successors of SIOCGIF* on the Solaris
 * and HP/UX operating systems. The data is stored in sockaddr_storage
 * structure. Both IPv4 and IPv6 addresses are returned though these
 * calls.
 */
#define NEXT_INTERFACE(lifr)	(lifr + 1)
#define LIFREQ_SIZE(lifr)	sizeof(lifr[0])

/* ial_siocglif - determine IP addresses using ioctl(SIOCGLIF*) */

static int ial_siocglif(INET_ADDR_LIST *addr_list,
			        INET_ADDR_LIST *mask_list,
			        int af)
{
    const char *myname = "inet_addr_local[siocglif]";
    struct lifconf lifc;
    struct lifreq *lifr;
    struct lifreq *lifr_mask;
    struct lifreq *the_end;
    struct sockaddr *sa;
    int     sock;
    VSTRING *buf;

    /*
     * See also comments in ial_siocgif()
     */
    if (af != AF_INET && af != AF_INET6)
	msg_fatal("%s: address family was %d, must be AF_INET (%d) or "
		  "AF_INET6 (%d)", myname, af, AF_INET, AF_INET6);
    sock = ial_socket(af);
    if (sock < 0)
	return (0);
    buf = vstring_alloc(1024);
    for (;;) {
	memset(&lifc, 0, sizeof(lifc));
	lifc.lifc_family = AF_UNSPEC;		/* XXX Why??? */
	lifc.lifc_len = vstring_avail(buf);
	lifc.lifc_buf = vstring_str(buf);
	if (ioctl(sock, SIOCGLIFCONF, (char *) &lifc) < 0) {
	    if (errno != EINVAL)
		msg_fatal("%s: ioctl SIOCGLIFCONF: %m", myname);
	} else if (lifc.lifc_len < vstring_avail(buf) / 2)
	    break;
	VSTRING_SPACE(buf, vstring_avail(buf) * 2);
    }

    the_end = (struct lifreq *) (lifc.lifc_buf + lifc.lifc_len);
    for (lifr = lifc.lifc_req; lifr < the_end;) {
	sa = (struct sockaddr *) & lifr->lifr_addr;
	if (sa->sa_family != af) {
	    lifr = NEXT_INTERFACE(lifr);
	    continue;
	}
	if (af == AF_INET) {
	    if (SOCK_ADDR_IN_ADDR(sa).s_addr == INADDR_ANY) {
		lifr = NEXT_INTERFACE(lifr);
		continue;
	    }
#ifdef HAS_IPV6
	} else if (af == AF_INET6) {
	    if (IN6_IS_ADDR_UNSPECIFIED(&SOCK_ADDR_IN6_ADDR(sa))) {
		lifr = NEXT_INTERFACE(lifr);
		continue;
	    }
	}
#endif
	inet_addr_list_append(addr_list, sa);
	if (mask_list) {
	    lifr_mask = (struct lifreq *) mymalloc(sizeof(struct lifreq));
	    memcpy((char *) lifr_mask, (char *) lifr, sizeof(struct lifreq));
	    if (ioctl(sock, SIOCGLIFNETMASK, lifr_mask) < 0)
		msg_fatal("%s: ioctl(SIOCGLIFNETMASK): %m", myname);
	    /* XXX: Check whether sa_len/family are honoured --dcs */
	    inet_addr_list_append(mask_list,
				(struct sockaddr *) & lifr_mask->lifr_addr);
	    myfree((char *) lifr_mask);
	}
	lifr = NEXT_INTERFACE(lifr);
    }
    vstring_free(buf);
    (void) close(sock);
    return (0);
}

#else					/* HAVE_SIOCGLIF */

/*
 * The classic SIOCGIF* ioctls. Modern BSD operating systems will
 * also return IPv6 addresses through these structure. Note however
 * that recent versions of these operating systems have getifaddrs.
 */
#if defined(_SIZEOF_ADDR_IFREQ)
#define NEXT_INTERFACE(ifr)	((struct ifreq *) \
	((char *) ifr + _SIZEOF_ADDR_IFREQ(*ifr)))
#define IFREQ_SIZE(ifr)	_SIZEOF_ADDR_IFREQ(*ifr)
#elif defined(HAS_SA_LEN)
#define NEXT_INTERFACE(ifr)	((struct ifreq *) \
	((char *) ifr + sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len))
#define IFREQ_SIZE(ifr)	(sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len)
#else
#define NEXT_INTERFACE(ifr)	(ifr + 1)
#define IFREQ_SIZE(ifr)	sizeof(ifr[0])
#endif

/* ial_siocgif - determine IP addresses using ioctl(SIOCGIF*) */

static int ial_siocgif(INET_ADDR_LIST *addr_list,
		               INET_ADDR_LIST *mask_list,
		               int af)
{
    const char *myname = "inet_addr_local[siocgif]";
    struct in_addr addr;
    struct ifconf ifc;
    struct ifreq *ifr;
    struct ifreq *ifr_mask;
    struct ifreq *the_end;
    int     sock;
    VSTRING *buf;

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
    sock = ial_socket(af);
    if (sock < 0)
	return (0);
    buf = vstring_alloc(1024);
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

    the_end = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);
    for (ifr = ifc.ifc_req; ifr < the_end;) {
	if (ifr->ifr_addr.sa_family != af) {
	    ifr = NEXT_INTERFACE(ifr);
	    continue;
	}
	if (af == AF_INET) {
	    addr = ((struct sockaddr_in *) & ifr->ifr_addr)->sin_addr;
	    if (addr.s_addr != INADDR_ANY) {
		inet_addr_list_append(addr_list, &ifr->ifr_addr);
		if (mask_list) {
		    ifr_mask = (struct ifreq *) mymalloc(IFREQ_SIZE(ifr));
		    memcpy((char *) ifr_mask, (char *) ifr, IFREQ_SIZE(ifr));
		    if (ioctl(sock, SIOCGIFNETMASK, ifr_mask) < 0)
			msg_fatal("%s: ioctl SIOCGIFNETMASK: %m", myname);

		    /*
		     * Note that this SIOCGIFNETMASK has truly screwed up the
		     * contents of sa_len/sa_family. We must fix this
		     * manually to have correct addresses.   --dcs
		     */
		    ifr_mask->ifr_addr.sa_family = af;
#ifdef HAS_SA_LEN
		    ifr_mask->ifr_addr.sa_len = sizeof(struct sockaddr_in);
#endif
		    inet_addr_list_append(mask_list, &ifr_mask->ifr_addr);
		    myfree((char *) ifr_mask);
		}
	    }
	}
#ifdef HAS_IPV6
	else if (af == AF_INET6) {
	    struct sockaddr *sa;

	    sa = SOCK_ADDR_PTR(&ifr->ifr_addr);
	    if (!(IN6_IS_ADDR_UNSPECIFIED(&SOCK_ADDR_IN6_ADDR(sa)))) {
		inet_addr_list_append(addr_list, sa);
		if (mask_list) {
		    /* XXX Assume /128 for everything */
		    struct sockaddr_in6 mask6;

		    mask6 = *SOCK_ADDR_IN6_PTR(sa);
		    memset((char *) &mask6.sin6_addr, ~0,
			   sizeof(mask6.sin6_addr));
		    inet_addr_list_append(mask_list, SOCK_ADDR_PTR(&mask6));
		}
	    }
	}
#endif
	ifr = NEXT_INTERFACE(ifr);
    }
    vstring_free(buf);
    (void) close(sock);
    return (0);
}

#endif					/* HAVE_SIOCGLIF */

#ifdef HAS_PROCNET_IFINET6

/*
 * Older Linux versions lack proper calls to retrieve IPv6 interface
 * addresses. Instead, the addresses can be read from a file in the
 * /proc tree. The most important issue with this approach however
 * is that the /proc tree may not always be available, for example
 * in a chrooted environment or in "hardened" (sic) installations.
 */

/* ial_procnet_ifinet6 - determine IPv6 addresses using /proc/net/if_inet6 */

static int ial_procnet_ifinet6(INET_ADDR_LIST *addr_list,
			               INET_ADDR_LIST *mask_list)
{
    const char *myname = "inet_addr_local[procnet_ifinet6]";
    FILE   *fp;
    char    buf[BUFSIZ];
    unsigned plen;
    VSTRING *addrbuf;
    struct sockaddr_in6 addr;
    struct sockaddr_in6 mask;

    /*
     * Example: 00000000000000000000000000000001 01 80 10 80 lo
     * 
     * Fields: address, interface index, prefix length, scope value
     * (net/ipv6.h), interface flags (linux/rtnetlink.h), device name.
     * 
     * FIX 200501 The IPv6 patch used fscanf(), which will hang on unexpected
     * input. Use fgets() + sscanf() instead.
     */
    if ((fp = fopen(_PATH_PROCNET_IFINET6, "r")) != 0) {
	addrbuf = vstring_alloc(MAI_V6ADDR_BYTES + 1);
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
#ifdef HAS_SA_LEN
	addr.sin6_len = sizeof(addr);
#endif
	mask = addr;
	while (fgets(buf, sizeof(buf), fp) != 0) {
	    /* 200501 hex_decode() is light-weight compared to getaddrinfo(). */
	    if (hex_decode(addrbuf, buf, MAI_V6ADDR_BYTES * 2) == 0
		|| sscanf(buf + MAI_V6ADDR_BYTES * 2, " %*x %x", &plen) != 1
		|| plen > MAI_V6ADDR_BITS) {
		msg_warn("unexpected data in %s - skipping IPv6 configuration",
			 _PATH_PROCNET_IFINET6);
		break;
	    }
	    /* vstring_str(addrbuf) has worst-case alignment. */
	    addr.sin6_addr = *(struct in6_addr *) vstring_str(addrbuf);
	    inet_addr_list_append(addr_list, SOCK_ADDR_PTR(&addr));

	    memset((char *) &mask.sin6_addr, ~0, sizeof(mask.sin6_addr));
	    mask_addr((unsigned char *) &mask.sin6_addr,
		      sizeof(mask.sin6_addr), plen);
	    inet_addr_list_append(mask_list, SOCK_ADDR_PTR(&mask));
	}
	vstring_free(addrbuf);
	fclose(fp);				/* FIX 200501 */
    } else {
	msg_warn("can't open %s (%m) - skipping IPv6 configuration",
		 _PATH_PROCNET_IFINET6);
    }
    return (0);
}

#endif					/* HAS_PROCNET_IFINET6 */

/* inet_addr_local - find all IP addresses for this host */

int     inet_addr_local(INET_ADDR_LIST *addr_list, INET_ADDR_LIST *mask_list,
			        unsigned *addr_family_list)
{
    const char *myname = "inet_addr_local";
    int     initial_count = addr_list->used;
    unsigned family;
    int     count;

    while ((family = *addr_family_list++) != 0) {

	/*
	 * IP Version 4
	 */
	if (family == AF_INET) {
	    count = addr_list->used;
#if defined(HAVE_GETIFADDRS)
	    ial_getifaddrs(addr_list, mask_list, AF_INET);
#elif defined (HAS_SIOCGLIF)
	    ial_siocglif(addr_list, mask_list, AF_INET);
#else
	    ial_siocgif(addr_list, mask_list, AF_INET);
#endif
	    if (msg_verbose)
		msg_info("%s: configured %d IPv4 addresses",
			 myname, addr_list->used - count);
	}

	/*
	 * IP Version 6
	 */
#ifdef HAS_IPV6
	else if (family == AF_INET6) {
	    count = addr_list->used;
#if defined(HAVE_GETIFADDRS)
	    ial_getifaddrs(addr_list, mask_list, AF_INET6);
#elif defined(HAS_PROCNET_IFINET6)
	    ial_procnet_ifinet6(addr_list, mask_list);
#elif defined(HAS_SIOCGLIF)
	    ial_siocglif(addr_list, mask_list, AF_INET6);
#else
	    ial_siocgif(addr_list, mask_list, AF_INET6);
#endif
	    if (msg_verbose)
		msg_info("%s: configured %d IPv6 addresses", myname,
			 addr_list->used - count);
	}
#endif

	/*
	 * Something's not right.
	 */
	else
	    msg_panic("%s: unknown address family %d", myname, family);
    }
    return (addr_list->used - initial_count);
}

#ifdef TEST

#include <string.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <inet_proto.h>

int     main(int unused_argc, char **argv)
{
    INET_ADDR_LIST addr_list;
    INET_ADDR_LIST mask_list;
    MAI_HOSTADDR_STR hostaddr;
    MAI_HOSTADDR_STR hostmask;
    struct sockaddr *sa;
    int     i;
    INET_PROTO_INFO *proto_info;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_verbose = 1;

    proto_info = inet_proto_init(argv[0], INET_PROTO_NAME_ALL);
    inet_addr_list_init(&addr_list);
    inet_addr_list_init(&mask_list);
    inet_addr_local(&addr_list, &mask_list, proto_info->ai_family_list);

    if (addr_list.used == 0)
	msg_fatal("cannot find any active network interfaces");

    if (addr_list.used == 1)
	msg_warn("found only one active network interface");

    for (i = 0; i < addr_list.used; i++) {
	sa = SOCK_ADDR_PTR(addr_list.addrs + i);
	SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(sa), SOCK_ADDR_LEN(sa),
			     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	sa = SOCK_ADDR_PTR(mask_list.addrs + i);
	SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(sa), SOCK_ADDR_LEN(sa),
			     &hostmask, (MAI_SERVPORT_STR *) 0, 0);
	vstream_printf("%s/%s\n", hostaddr.buf, hostmask.buf);
	vstream_fflush(VSTREAM_OUT);
    }
    inet_addr_list_free(&addr_list);
    inet_addr_list_free(&mask_list);
    return (0);
}

#endif
