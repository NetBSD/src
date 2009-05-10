/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
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
#include "dhcp.h"
#include "dhcpcd.h"

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

struct rt {
	struct in_addr dest;
	struct in_addr net;
	struct in_addr gate;
	const struct interface *iface;
	struct rt *next;
};

uint32_t get_netmask(uint32_t);
char *hwaddr_ntoa(const unsigned char *, size_t);
size_t hwaddr_aton(unsigned char *, const char *);

int getifssid(const char *, char *);
struct interface *init_interface(const char *);
struct interface *discover_interfaces(int, char * const *);
void free_interface(struct interface *);
int do_mtu(const char *, short int);
#define get_mtu(iface) do_mtu(iface, 0)
#define set_mtu(iface, mtu) do_mtu(iface, mtu)

int inet_ntocidr(struct in_addr);
int inet_cidrtoaddr(int, struct in_addr *);

int up_interface(const char *);
int do_address(const char *,
    struct in_addr *, struct in_addr *, struct in_addr *, int);
int if_address(const struct interface *,
    const struct in_addr *, const struct in_addr *,
    const struct in_addr *, int);
#define add_address(iface, addr, net, brd)				      \
	if_address(iface, addr, net, brd, 1)
#define del_address(iface, addr, net)					      \
	if_address(iface, addr, net, NULL, -1)
#define has_address(iface, addr, net)					      \
	do_address(iface, addr, net, NULL, 0)
#define get_address(iface, addr, net, dst)				      \
	do_address(iface, addr, net, dst, 1)

int if_route(const struct interface *, const struct in_addr *,
    const struct in_addr *, const struct in_addr *, int, int);
#define add_route(iface, dest, mask, gate, metric)			      \
	if_route(iface, dest, mask, gate, metric, 1)
#define change_route(iface, dest, mask, gate, metric)			      \
	if_route(iface, dest, mask, gate, metric, 0)
#define del_route(iface, dest, mask, gate, metric)			      \
	if_route(iface, dest, mask, gate, metric, -1)
#define del_src_route(iface, dest, mask, gate, metric)			      \
	if_route(iface, dest, mask, gate, metric, -2)
void free_routes(struct rt *);

int open_udp_socket(struct interface *);
const size_t udp_dhcp_len;
ssize_t make_udp_packet(uint8_t **, const uint8_t *, size_t,
    struct in_addr, struct in_addr);
ssize_t get_udp_data(const uint8_t **, const uint8_t *);
int valid_udp_packet(const uint8_t *, size_t, struct in_addr *);

int open_socket(struct interface *, int);
ssize_t send_packet(const struct interface *, struct in_addr, 
    const uint8_t *, ssize_t);
ssize_t send_raw_packet(const struct interface *, int,
    const void *, ssize_t);
ssize_t get_raw_packet(struct interface *, int, void *, ssize_t);

int init_socket(void);
int open_link_socket(void);
int manage_link(int);
int carrier_status(const char *);
#endif
