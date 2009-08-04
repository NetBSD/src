/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <limits.h>

#include "config.h"

#ifndef DUID_LEN
#  define DUID_LEN			128 + 2
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

#define HWADDR_LEN			20

/* Work out if we have a private address or not
 * 10/8
 * 172.16/12
 * 192.168/16
 */
#ifndef IN_PRIVATE
# define IN_PRIVATE(addr) (((addr & IN_CLASSA_NET) == 0x0a000000) || \
			   ((addr & 0xfff00000)    == 0xac100000) || \
			   ((addr & IN_CLASSB_NET) == 0xc0a80000))
#endif

#define LINKLOCAL_ADDR	0xa9fe0000
#define LINKLOCAL_MASK	IN_CLASSB_NET
#define LINKLOCAL_BRDC	(LINKLOCAL_ADDR | ~LINKLOCAL_MASK)

#ifndef IN_LINKLOCAL
# define IN_LINKLOCAL(addr) ((addr & IN_CLASSB_NET) == LINKLOCAL_ADDR)
#endif

/* There is an argument that this should be converted to an STAIL using
 * queue(3). However, that isn't readily available on all libc's that
 * dhcpcd works on. The only benefit of STAILQ over this is the ability to
 * quickly loop backwards through the list - currently we reverse the list
 * and then move through it forwards. This isn't that much of a big deal
 * though as the norm is to just have one default route, and an IPV4LL route.
 * You can (and do) get more routes in the DHCP message, but not enough to
 * really warrant a change to STAIL queue for performance reasons. */
struct rt {
	struct in_addr dest;
	struct in_addr net;
	struct in_addr gate;
	struct rt *next;
};

struct interface
{
	char name[IF_NAMESIZE];
	sa_family_t family;
	unsigned char hwaddr[HWADDR_LEN];
	size_t hwlen;
	int arpable;

	int raw_fd;
	int udp_fd;
	int arp_fd;
	int link_fd;
	size_t buffer_size, buffer_len, buffer_pos;
	unsigned char *buffer;

	struct in_addr addr;
	struct in_addr net;
	struct rt *routes;

	char leasefile[PATH_MAX];
	time_t start_uptime;

	unsigned char *clientid;
};

uint32_t get_netmask(uint32_t);
char *hwaddr_ntoa(const unsigned char *, size_t);
size_t hwaddr_aton(unsigned char *, const char *);

struct interface *read_interface(const char *, int);
int do_mtu(const char *, short int);
#define get_mtu(iface) do_mtu(iface, 0)
#define set_mtu(iface, mtu) do_mtu(iface, mtu)

int inet_ntocidr(struct in_addr);
int inet_cidrtoaddr(int, struct in_addr *);

int up_interface(const char *);
int do_interface(const char *, unsigned char *, size_t *,
		 struct in_addr *, struct in_addr *, int);
int if_address(const char *, const struct in_addr *, const struct in_addr *,
	       const struct in_addr *, int);
#define add_address(ifname, addr, net, brd) \
	if_address(ifname, addr, net, brd, 1)
#define del_address(ifname, addr, net) \
	if_address(ifname, addr, net, NULL, -1)
#define has_address(ifname, addr, net) \
	do_interface(ifname, NULL, NULL, addr, net, 0)
#define get_address(ifname, addr, net) \
	do_interface(ifname, NULL, NULL, addr, net, 1)

int if_route(const struct interface *,
	     const struct in_addr *,const struct in_addr *,
	     const struct in_addr *, int, int);
#define add_route(iface, dest, mask, gate, metric) \
	if_route(iface, dest, mask, gate, metric, 1)
#define change_route(iface, dest, mask, gate, metric) \
	if_route(iface, dest, mask, gate, metric, 0)
#define del_route(iface, dest, mask, gate, metric) \
	if_route(iface, dest, mask, gate, metric, -1)
void free_routes(struct rt *);

int open_udp_socket(struct interface *);
const size_t udp_dhcp_len;
ssize_t make_udp_packet(uint8_t **, const uint8_t *, size_t,
			struct in_addr, struct in_addr);
ssize_t get_udp_data(const uint8_t **, const uint8_t *);
int valid_udp_packet(const uint8_t *, size_t);

int open_socket(struct interface *, int);
ssize_t send_packet(const struct interface *, struct in_addr, 
		    const uint8_t *, ssize_t);
ssize_t send_raw_packet(const struct interface *, int,
			const void *, ssize_t);
ssize_t get_raw_packet(struct interface *, int, void *, ssize_t);

int send_arp(const struct interface *, int, in_addr_t, in_addr_t);

int open_link_socket(struct interface *);
int link_changed(struct interface *);
int carrier_status(const char *);
#endif
