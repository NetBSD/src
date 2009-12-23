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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#ifdef AF_LINK
#  include <net/if_dl.h>
#  include <net/if_types.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define __FAVOR_BSD /* Nasty glibc hack so we can use BSD semantics for UDP */
#include <netinet/udp.h>
#undef __FAVOR_BSD
#ifdef AF_PACKET
#  include <netpacket/packet.h>
#endif
#ifdef SIOCGIFMEDIA
#  include <net/if_media.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <ifaddrs.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "if-options.h"
#include "net.h"
#include "signals.h"

static char hwaddr_buffer[(HWADDR_LEN * 3) + 1];

int socket_afnet = -1;

int
inet_ntocidr(struct in_addr address)
{
	int cidr = 0;
	uint32_t mask = htonl(address.s_addr);

	while (mask) {
		cidr++;
		mask <<= 1;
	}
	return cidr;
}

int
inet_cidrtoaddr(int cidr, struct in_addr *addr)
{
	int ocets;

	if (cidr < 1 || cidr > 32) {
		errno = EINVAL;
		return -1;
	}
	ocets = (cidr + 7) / 8;

	addr->s_addr = 0;
	if (ocets > 0) {
		memset(&addr->s_addr, 255, (size_t)ocets - 1);
		memset((unsigned char *)&addr->s_addr + (ocets - 1),
		    (256 - (1 << (32 - cidr) % 8)), 1);
	}

	return 0;
}

uint32_t
get_netmask(uint32_t addr)
{
	uint32_t dst;

	if (addr == 0)
		return 0;

	dst = htonl(addr);
	if (IN_CLASSA(dst))
		return ntohl(IN_CLASSA_NET);
	if (IN_CLASSB(dst))
		return ntohl(IN_CLASSB_NET);
	if (IN_CLASSC(dst))
		return ntohl(IN_CLASSC_NET);

	return 0;
}

char *
hwaddr_ntoa(const unsigned char *hwaddr, size_t hwlen)
{
	char *p = hwaddr_buffer;
	size_t i;

	for (i = 0; i < hwlen && i < HWADDR_LEN; i++) {
		if (i > 0)
			*p ++= ':';
		p += snprintf(p, 3, "%.2x", hwaddr[i]);
	}

	*p ++= '\0';

	return hwaddr_buffer;
}

size_t
hwaddr_aton(unsigned char *buffer, const char *addr)
{
	char c[3];
	const char *p = addr;
	unsigned char *bp = buffer;
	size_t len = 0;

	c[2] = '\0';
	while (*p) {
		c[0] = *p++;
		c[1] = *p++;
		/* Ensure that digits are hex */
		if (isxdigit((unsigned char)c[0]) == 0 ||
		    isxdigit((unsigned char)c[1]) == 0)
		{
			errno = EINVAL;
			return 0;
		}
		/* We should have at least two entries 00:01 */
		if (len == 0 && *p == '\0') {
			errno = EINVAL;
			return 0;
		}
		/* Ensure that next data is EOL or a seperator with data */
		if (!(*p == '\0' || (*p == ':' && *(p + 1) != '\0'))) {
			errno = EINVAL;
			return 0;
		}
		if (*p)
			p++;
		if (bp)
			*bp++ = (unsigned char)strtol(c, NULL, 16);
		len++;
	}
	return len;
}

struct interface *
init_interface(const char *ifname)
{
	struct ifreq ifr;
	struct interface *iface = NULL;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(socket_afnet, SIOCGIFFLAGS, &ifr) == -1)
		goto eexit;

	iface = xzalloc(sizeof(*iface));
	strlcpy(iface->name, ifname, sizeof(iface->name));
	iface->flags = ifr.ifr_flags;
	/* We reserve the 100 range for virtual interfaces, if and when
	 * we can work them out. */
	iface->metric = 200 + if_nametoindex(iface->name);
	if (getifssid(ifname, iface->ssid) != -1) {
		iface->wireless = 1;
		iface->metric += 100;
	}

	if (ioctl(socket_afnet, SIOCGIFMTU, &ifr) == -1)
		goto eexit;
	/* Ensure that the MTU is big enough for DHCP */
	if (ifr.ifr_mtu < MTU_MIN) {
		ifr.ifr_mtu = MTU_MIN;
		strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		if (ioctl(socket_afnet, SIOCSIFMTU, &ifr) == -1)
			goto eexit;
	}

	snprintf(iface->leasefile, sizeof(iface->leasefile),
	    LEASEFILE, ifname);
	/* 0 is a valid fd, so init to -1 */
	iface->raw_fd = -1;
	iface->udp_fd = -1;
	iface->arp_fd = -1;
	goto exit;

eexit:
	free(iface);
	iface = NULL;
exit:
	return iface;
}

void
free_interface(struct interface *iface)
{
	if (!iface)
		return;
	if (iface->state) {
		free_options(iface->state->options);
		free(iface->state->old);
		free(iface->state->new);
		free(iface->state->offer);
		free(iface->state);
	}
	free(iface->clientid);
	free(iface);
}

int
carrier_status(struct interface *iface)
{
	int ret;
	struct ifreq ifr;
#ifdef SIOCGIFMEDIA
	struct ifmediareq ifmr;
#endif
#ifdef __linux__
	char *p;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
#ifdef __linux__
	/* We can only test the real interface up */
	if ((p = strchr(ifr.ifr_name, ':')))
		*p = '\0';
#endif

	if (ioctl(socket_afnet, SIOCGIFFLAGS, &ifr) == -1)
		return -1;
	iface->flags = ifr.ifr_flags;

	ret = -1;
#ifdef SIOCGIFMEDIA
	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, iface->name, sizeof(ifmr.ifm_name));
	if (ioctl(socket_afnet, SIOCGIFMEDIA, &ifmr) != -1 &&
	    ifmr.ifm_status & IFM_AVALID)
		ret = (ifmr.ifm_status & IFM_ACTIVE) ? 1 : 0;
#endif
	if (ret == -1)
		ret = (ifr.ifr_flags & IFF_RUNNING) ? 1 : 0;
	return ret;
}

int
up_interface(struct interface *iface)
{
	struct ifreq ifr;
	int retval = -1;
#ifdef __linux__
	char *p;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
#ifdef __linux__
	/* We can only bring the real interface up */
	if ((p = strchr(ifr.ifr_name, ':')))
		*p = '\0';
#endif
	if (ioctl(socket_afnet, SIOCGIFFLAGS, &ifr) == 0) {
		if ((ifr.ifr_flags & IFF_UP))
			retval = 0;
		else {
			ifr.ifr_flags |= IFF_UP;
			if (ioctl(socket_afnet, SIOCSIFFLAGS, &ifr) == 0)
				retval = 0;
		}
		iface->flags = ifr.ifr_flags;
	}
	return retval;
}

struct interface *
discover_interfaces(int argc, char * const *argv)
{
	struct ifaddrs *ifaddrs, *ifa;
	char *p;
	int i;
	struct interface *ifp, *ifs, *ifl;
#ifdef __linux__
	char ifn[IF_NAMESIZE];
#endif
#ifdef AF_LINK
	const struct sockaddr_dl *sdl;
#elif AF_PACKET
	const struct sockaddr_ll *sll;
#endif

	if (getifaddrs(&ifaddrs) == -1)
		return NULL;

	ifs = ifl = NULL;
	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;
#elif AF_PACKET
			if (ifa->ifa_addr->sa_family != AF_PACKET)
				continue;
#endif
		}

		/* It's possible for an interface to have >1 AF_LINK.
		 * For our purposes, we use the first one. */
		for (ifp = ifs; ifp; ifp = ifp->next)
			if (strcmp(ifp->name, ifa->ifa_name) == 0)
				break;
		if (ifp)
			continue;
		if (argc > 0) {
			for (i = 0; i < argc; i++) {
#ifdef __linux__
				/* Check the real interface name */
				strlcpy(ifn, argv[i], sizeof(ifn));
				p = strchr(ifn, ':');
				if (p)
					*p = '\0';
				if (strcmp(ifn, ifa->ifa_name) == 0)
					break;
#else
				if (strcmp(argv[i], ifa->ifa_name) == 0)
					break;
#endif
			}
			if (i == argc)
				continue;
			p = argv[i];
		} else {
			/* -1 means we're discovering against a specific
			 * interface, but we still need the below rules
			 * to apply. */
			if (argc == -1 && strcmp(argv[0], ifa->ifa_name) != 0)
				continue;
			for (i = 0; i < ifdc; i++)
				if (!fnmatch(ifdv[i], ifa->ifa_name, 0))
					break;
			if (i < ifdc)
				continue;
			for (i = 0; i < ifac; i++)
				if (!fnmatch(ifav[i], ifa->ifa_name, 0))
					break;
			if (ifac && i == ifac)
				continue;
			p = ifa->ifa_name;
		}
		if ((ifp = init_interface(p)) == NULL)
			continue;

		/* Bring the interface up if not already */
		if (!(ifp->flags & IFF_UP)
#ifdef SIOCGIFMEDIA
		    && carrier_status(ifp) != -1
#endif
		   )
		{
			if (up_interface(ifp) == 0)
				options |= DHCPCD_WAITUP;
			else
				syslog(LOG_ERR, "%s: up_interface: %m", ifp->name);
		}

		/* Don't allow loopback unless explicit */
		if (ifp->flags & IFF_LOOPBACK) {
			if (argc == 0 && ifac == 0) {
				free_interface(ifp);
				continue;
			}
		} else if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			sdl = (const struct sockaddr_dl *)(void *)ifa->ifa_addr;
			switch(sdl->sdl_type) {
			case IFT_ETHER:
				ifp->family = ARPHRD_ETHER;
				break;
			case IFT_IEEE1394:
				ifp->family = ARPHRD_IEEE1394;
				break;
			}
			ifp->hwlen = sdl->sdl_alen;
#ifndef CLLADDR
#  define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))
#endif
			memcpy(ifp->hwaddr, CLLADDR(sdl), ifp->hwlen);
#elif AF_PACKET
			sll = (const struct sockaddr_ll *)(void *)ifa->ifa_addr;
			ifp->family = sll->sll_hatype;
			ifp->hwlen = sll->sll_halen;
			if (ifp->hwlen != 0)
				memcpy(ifp->hwaddr, sll->sll_addr, ifp->hwlen);
#endif
		}

		/* We only work on ethernet by default */
		if (!(ifp->flags & IFF_POINTOPOINT) &&
		    ifp->family != ARPHRD_ETHER)
		{
			if (argc == 0 && ifac == 0) {
				free_interface(ifp);
				continue;
			}
			if (ifp->family != ARPHRD_IEEE1394)
				syslog(LOG_WARNING,
				    "%s: unknown hardware family", p);
		}
		if (ifl)
			ifl->next = ifp; 
		else
			ifs = ifp;
		ifl = ifp;
	}
	freeifaddrs(ifaddrs);
	return ifs;
}

int
do_address(const char *ifname,
    struct in_addr *addr, struct in_addr *net, struct in_addr *dst, int act)
{
	struct ifaddrs *ifaddrs, *ifa;
	const struct sockaddr_in *a, *n, *d;
	int retval;

	if (getifaddrs(&ifaddrs) == -1)
		return -1;

	retval = 0;
	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET ||
		    strcmp(ifa->ifa_name, ifname) != 0)
			continue;
		a = (const struct sockaddr_in *)(void *)ifa->ifa_addr;
		n = (const struct sockaddr_in *)(void *)ifa->ifa_netmask;
		if (ifa->ifa_flags & IFF_POINTOPOINT)
			d = (const struct sockaddr_in *)(void *)
			    ifa->ifa_dstaddr;
		else
			d = NULL;
		if (act == 1) {
			addr->s_addr = a->sin_addr.s_addr;
			net->s_addr = n->sin_addr.s_addr;
			if (dst && ifa->ifa_flags & IFF_POINTOPOINT)
				dst->s_addr = d->sin_addr.s_addr;
			retval = 1;
			break;
		}
		if (addr->s_addr == a->sin_addr.s_addr &&
		    (net == NULL || net->s_addr == n->sin_addr.s_addr))
		{
			retval = 1;
			break;
		}
	}
	freeifaddrs(ifaddrs);
	return retval;
}

int
do_mtu(const char *ifname, short int mtu)
{
	struct ifreq ifr;
	int r;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = mtu;
	r = ioctl(socket_afnet, mtu ? SIOCSIFMTU : SIOCGIFMTU, &ifr);
	if (r == -1)
		return -1;
	return ifr.ifr_mtu;
}

void
free_routes(struct rt *routes)
{
	struct rt *r;

	while (routes) {
		r = routes->next;
		free(routes);
		routes = r;
	}
}

int
open_udp_socket(struct interface *iface)
{
	int s;
	struct sockaddr_in sin;
	int n;
#ifdef SO_BINDTODEVICE
	struct ifreq ifr;
	char *p;
#endif

	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1)
		goto eexit;
#ifdef SO_BINDTODEVICE
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
	/* We can only bind to the real device */
	p = strchr(ifr.ifr_name, ':');
	if (p)
		*p = '\0';
	if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr,
		sizeof(ifr)) == -1)
		goto eexit;
#endif
	/* As we don't use this socket for receiving, set the
	 * receive buffer to 1 */
	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1)
		goto eexit;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DHCP_CLIENT_PORT);
	sin.sin_addr.s_addr = iface->addr.s_addr;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		goto eexit;

	iface->udp_fd = s;
	set_cloexec(s);
	return 0;

eexit:
	close(s);
	return -1;
}

ssize_t
send_packet(const struct interface *iface, struct in_addr to,
    const uint8_t *data, ssize_t len)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = to.s_addr;
	sin.sin_port = htons(DHCP_SERVER_PORT);
	return sendto(iface->udp_fd, data, len, 0,
	    (struct sockaddr *)&sin, sizeof(sin));
}

struct udp_dhcp_packet
{
	struct ip ip;
	struct udphdr udp;
	struct dhcp_message dhcp;
};
const size_t udp_dhcp_len = sizeof(struct udp_dhcp_packet);

static uint16_t
checksum(const void *data, uint16_t len)
{
	const uint8_t *addr = data;
	uint32_t sum = 0;

	while (len > 1) {
		sum += addr[0] * 256 + addr[1];
		addr += 2;
		len -= 2;
	}

	if (len == 1)
		sum += *addr * 256;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	sum = htons(sum);

	return ~sum;
}

ssize_t
make_udp_packet(uint8_t **packet, const uint8_t *data, size_t length,
    struct in_addr source, struct in_addr dest)
{
	struct udp_dhcp_packet *udpp;
	struct ip *ip;
	struct udphdr *udp;

	udpp = xzalloc(sizeof(*udpp));
	ip = &udpp->ip;
	udp = &udpp->udp;

	/* OK, this is important :)
	 * We copy the data to our packet and then create a small part of the
	 * ip structure and an invalid ip_len (basically udp length).
	 * We then fill the udp structure and put the checksum
	 * of the whole packet into the udp checksum.
	 * Finally we complete the ip structure and ip checksum.
	 * If we don't do the ordering like so then the udp checksum will be
	 * broken, so find another way of doing it! */

	memcpy(&udpp->dhcp, data, length);

	ip->ip_p = IPPROTO_UDP;
	ip->ip_src.s_addr = source.s_addr;
	if (dest.s_addr == 0)
		ip->ip_dst.s_addr = INADDR_BROADCAST;
	else
		ip->ip_dst.s_addr = dest.s_addr;

	udp->uh_sport = htons(DHCP_CLIENT_PORT);
	udp->uh_dport = htons(DHCP_SERVER_PORT);
	udp->uh_ulen = htons(sizeof(*udp) + length);
	ip->ip_len = udp->uh_ulen;
	udp->uh_sum = checksum(udpp, sizeof(*udpp));

	ip->ip_v = IPVERSION;
	ip->ip_hl = 5;
	ip->ip_id = 0;
	ip->ip_tos = IPTOS_LOWDELAY;
	ip->ip_len = htons (sizeof(*ip) + sizeof(*udp) + length);
	ip->ip_id = 0;
	ip->ip_off = htons(IP_DF); /* Don't fragment */
	ip->ip_ttl = IPDEFTTL;

	ip->ip_sum = checksum(ip, sizeof(*ip));

	*packet = (uint8_t *)udpp;
	return sizeof(*ip) + sizeof(*udp) + length;
}

ssize_t
get_udp_data(const uint8_t **data, const uint8_t *udp)
{
	struct udp_dhcp_packet packet;

	memcpy(&packet, udp, sizeof(packet));
	*data = udp + offsetof(struct udp_dhcp_packet, dhcp);
	return ntohs(packet.ip.ip_len) -
	    sizeof(packet.ip) -
	    sizeof(packet.udp);
}

int
valid_udp_packet(const uint8_t *data, size_t data_len, struct in_addr *from)
{
	struct udp_dhcp_packet packet;
	uint16_t bytes, udpsum;

	if (data_len < sizeof(packet.ip)) {
		if (from)
			from->s_addr = INADDR_ANY;
		errno = EINVAL;
		return -1;
	}
	memcpy(&packet, data, MIN(data_len, sizeof(packet)));
	if (from)
		from->s_addr = packet.ip.ip_src.s_addr;
	if (data_len > sizeof(packet)) {
		errno = EINVAL;
		return -1;
	}
	if (checksum(&packet.ip, sizeof(packet.ip)) != 0) {
		errno = EINVAL;
		return -1;
	}

	bytes = ntohs(packet.ip.ip_len);
	if (data_len < bytes) {
		errno = EINVAL;
		return -1;
	}
	udpsum = packet.udp.uh_sum;
	packet.udp.uh_sum = 0;
	packet.ip.ip_hl = 0;
	packet.ip.ip_v = 0;
	packet.ip.ip_tos = 0;
	packet.ip.ip_len = packet.udp.uh_ulen;
	packet.ip.ip_id = 0;
	packet.ip.ip_off = 0;
	packet.ip.ip_ttl = 0;
	packet.ip.ip_sum = 0;
	if (udpsum && checksum(&packet, bytes) != udpsum) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}
