/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <net/if.h>
#include <net/route.h>		/* for RTM_ADD et all */
#include <netinet/in.h>
#ifdef BSD
#include <netinet/in_var.h>	/* for IN_IFF_TENTATIVE et all */
#endif

#include <ifaddrs.h>

/* Some systems have in-built IPv4 DAD.
 * However, we need them to do DAD at carrier up as well. */
#ifdef IN_IFF_TENTATIVE
#  ifdef __NetBSD__
#    define NOCARRIER_PRESERVE_IP
#  endif
#endif

/*
 * Systems which handle 1 address per alias.
 * Currenly this is just Solaris.
 * While Linux can do aliased addresses, it is only useful for their
 * legacy ifconfig(8) tool which cannot display >1 IPv4 address
 * (it can display many IPv6 addresses which makes the limitation odd).
 * Linux has ip(8) which is a more feature rich tool, without the above
 * restriction.
 */
#ifndef ALIAS_ADDR
#  ifdef __sun
#    define ALIAS_ADDR
#  endif
#endif

#include "config.h"
#include "dhcpcd.h"
#include "ipv4.h"
#include "ipv6.h"
#include "route.h"

#define EUI64_ADDR_LEN			8
#define INFINIBAND_ADDR_LEN		20

/* Linux 2.4 doesn't define this */
#ifndef ARPHRD_IEEE1394
#  define ARPHRD_IEEE1394		24
#endif

/* The BSD's don't define this yet */
#ifndef ARPHRD_INFINIBAND
#  define ARPHRD_INFINIBAND		32
#endif

/* Work out if we have a private address or not
 * 10/8
 * 172.16/12
 * 192.168/16
 */
#ifndef IN_PRIVATE
# define IN_PRIVATE(addr) (((addr & IN_CLASSA_NET) == 0x0a000000) ||	      \
	    ((addr & 0xfff00000)    == 0xac100000) ||			      \
	    ((addr & IN_CLASSB_NET) == 0xc0a80000))
#endif

#define RAW_EOF			1 << 0
#define RAW_PARTIALCSUM		2 << 0

#ifndef CLLADDR
#ifdef AF_LINK
#  define CLLADDR(sdl) (const void *)((sdl)->sdl_data + (sdl)->sdl_nlen)
#endif
#endif

#ifdef __sun
/* Solaris stupidly defines this for compat with BSD
 * but then ignores it. */
#undef RTF_CLONING

/* Solaris getifaddrs is very un-suitable for dhcpcd.
 * See if-sun.c for details why. */
struct ifaddrs;
int if_getifaddrs(struct ifaddrs **);
#define	getifaddrs	if_getifaddrs
#endif

int if_setflag(struct interface *ifp, short flag);
#define if_up(ifp) if_setflag((ifp), (IFF_UP | IFF_RUNNING))
bool if_valid_hwaddr(const uint8_t *, size_t);
struct if_head *if_discover(struct dhcpcd_ctx *, struct ifaddrs **,
    int, char * const *);
void if_markaddrsstale(struct if_head *);
void if_learnaddrs(struct dhcpcd_ctx *, struct if_head *, struct ifaddrs **);
void if_deletestaleaddrs(struct if_head *);
struct interface *if_find(struct if_head *, const char *);
struct interface *if_findindex(struct if_head *, unsigned int);
struct interface *if_loopback(struct dhcpcd_ctx *);
void if_sortinterfaces(struct dhcpcd_ctx *);
void if_free(struct interface *);
int if_domtu(const struct interface *, short int);
#define if_getmtu(ifp) if_domtu((ifp), 0)
#define if_setmtu(ifp, mtu) if_domtu((ifp), (mtu))
int if_carrier(struct interface *);

/*
 * Helper to decode an interface name of bge0:1 to
 * devname = bge0, drvname = bge0, ppa = 0, lun = 1.
 * If ppa or lun are invalid they are set to -1.
 */
struct if_spec {
	char ifname[IF_NAMESIZE];
	char devname[IF_NAMESIZE];
	char drvname[IF_NAMESIZE];
	int ppa;
	int lun;
};
int if_nametospec(const char *, struct if_spec *);

/* The below functions are provided by if-KERNEL.c */
int if_conf(struct interface *);
int if_init(struct interface *);
int if_getssid(struct interface *);
int if_vimaster(const struct dhcpcd_ctx *ctx, const char *);
unsigned short if_vlanid(const struct interface *);
int if_opensockets(struct dhcpcd_ctx *);
int if_opensockets_os(struct dhcpcd_ctx *);
void if_closesockets(struct dhcpcd_ctx *);
void if_closesockets_os(struct dhcpcd_ctx *);
int if_handlelink(struct dhcpcd_ctx *);

/* dhcpcd uses the same routing flags as BSD.
 * If the platform doesn't use these flags,
 * map them in the platform interace file. */
#ifndef RTM_ADD
#define RTM_ADD		0x1	/* Add Route */
#define RTM_DELETE	0x2	/* Delete Route */
#define RTM_CHANGE	0x3	/* Change Metrics or flags */
#define RTM_GET		0x4	/* Report Metrics */
#endif

/* Define SOCK_CLOEXEC and SOCK_NONBLOCK for systems that lack it.
 * xsocket() in if.c will map them to fctnl FD_CLOEXEC and O_NONBLOCK. */
#ifdef SOCK_CLOEXEC
# define HAVE_SOCK_CLOEXEC
#else
# define SOCK_CLOEXEC	0x10000000
#endif
#ifdef SOCK_NONBLOCK
# define HAVE_SOCK_NONBLOCK
#else
# define SOCK_NONBLOCK	0x20000000
#endif

int if_route(unsigned char, const struct rt *rt);
int if_initrt(struct dhcpcd_ctx *, int);

#ifdef INET
int if_address(unsigned char, const struct ipv4_addr *);
int if_addrflags(const struct interface *, const struct in_addr *,
    const char *);

#endif

#ifdef INET6
void if_setup_inet6(const struct interface *);
#ifdef IPV6_MANAGETEMPADDR
int ip6_use_tempaddr(const char *ifname);
int ip6_temp_preferred_lifetime(const char *ifname);
int ip6_temp_valid_lifetime(const char *ifname);
#else
#define ip6_use_tempaddr(a) (0)
#endif

int if_address6(unsigned char, const struct ipv6_addr *);
int if_addrflags6(const struct interface *, const struct in6_addr *,
    const char *);
int if_getlifetime6(struct ipv6_addr *);

#else
#define if_checkipv6(a, b, c) (-1)
#endif

int if_machinearch(char *, size_t);
int xsocket(int, int, int);
#endif
