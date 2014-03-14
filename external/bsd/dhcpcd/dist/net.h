/* $NetBSD: net.h,v 1.1.1.16 2014/03/14 11:27:41 roy Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include "config.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "ipv6.h"

/* Some systems have route metrics */
#ifndef HAVE_ROUTE_METRIC
# if defined(__linux__) || defined(SIOCGIFPRIORITY)
#  define HAVE_ROUTE_METRIC 1
# endif
# ifndef HAVE_ROUTE_METRIC
#  define HAVE_ROUTE_METRIC 0
# endif
#endif

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

#define LINKLOCAL_ADDR	0xa9fe0000
#define LINKLOCAL_MASK	IN_CLASSB_NET
#define LINKLOCAL_BRDC	(LINKLOCAL_ADDR | ~LINKLOCAL_MASK)

#ifndef IN_LINKLOCAL
# define IN_LINKLOCAL(addr) ((addr & IN_CLASSB_NET) == LINKLOCAL_ADDR)
#endif

char *hwaddr_ntoa(const unsigned char *, size_t, char *, size_t);
size_t hwaddr_aton(unsigned char *, const char *);

int getifssid(const char *, char *);
int if_vimaster(const char *);
struct if_head *discover_interfaces(struct dhcpcd_ctx *, int, char * const *);
void free_interface(struct interface *);
int do_mtu(const char *, short int);
#define get_mtu(iface) do_mtu(iface, 0)
#define set_mtu(iface, mtu) do_mtu(iface, mtu)

int if_conf(struct interface *);
int if_init(struct interface *);

int open_link_socket(void);
int manage_link(struct dhcpcd_ctx *);
int carrier_status(struct interface *);
#endif
