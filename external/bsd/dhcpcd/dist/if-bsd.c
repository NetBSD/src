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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include <arpa/inet.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "net.h"

/* Darwin doesn't define this for some very odd reason */
#ifndef SA_SIZE
# define SA_SIZE(sa)						\
	(  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?	\
	   sizeof(long)		:				\
	   1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(long) - 1) ) )
#endif

int
if_address(const char *ifname, const struct in_addr *address,
	   const struct in_addr *netmask, const struct in_addr *broadcast,
	   int action)
{
	int s;
	int retval;
	struct ifaliasreq ifa;
	union {
		struct sockaddr *sa;
		struct sockaddr_in *sin;
	} _s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;

	memset(&ifa, 0, sizeof(ifa));
	strlcpy(ifa.ifra_name, ifname, sizeof(ifa.ifra_name));

#define ADDADDR(_var, _addr) \
	_s.sa = &_var; \
	_s.sin->sin_family = AF_INET; \
	_s.sin->sin_len = sizeof(*_s.sin); \
	memcpy(&_s.sin->sin_addr, _addr, sizeof(_s.sin->sin_addr));

	ADDADDR(ifa.ifra_addr, address);
	ADDADDR(ifa.ifra_mask, netmask);
	if (action >= 0) {
		ADDADDR(ifa.ifra_broadaddr, broadcast);
	}
#undef ADDADDR

	if (action < 0)
		retval = ioctl(s, SIOCDIFADDR, &ifa);
	else
		retval = ioctl(s, SIOCAIFADDR, &ifa);
	close(s);
	return retval;
}

int
if_route(const struct interface *iface, const struct in_addr *dest,
	 const struct in_addr *net, const struct in_addr *gate,
	 _unused int metric, int action)
{
	int s;
	union sockunion {
		struct sockaddr sa;
		struct sockaddr_in sin;
#ifdef INET6
		struct sockaddr_in6 sin6;
#endif
		struct sockaddr_dl sdl;
		struct sockaddr_storage ss;
	} su;
	struct rtm 
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(su) * 4];
	} rtm;
	char *bp = rtm.buffer, *p;
	size_t l;
	int retval = 0;

#define ADDSU(_su) { \
	l = SA_SIZE(&(_su.sa)); \
	memcpy(bp, &(_su), l); \
	bp += l; \
}
#define ADDADDR(_addr) { \
	memset (&su, 0, sizeof(su)); \
	su.sin.sin_family = AF_INET; \
	su.sin.sin_len = sizeof(su.sin); \
	memcpy (&su.sin.sin_addr, _addr, sizeof(su.sin.sin_addr)); \
	ADDSU(su); \
}

	if ((s = socket(PF_ROUTE, SOCK_RAW, 0)) == -1)
		return -1;

	memset(&rtm, 0, sizeof(rtm));
	rtm.hdr.rtm_version = RTM_VERSION;
	rtm.hdr.rtm_seq = 1;
	if (action == 0)
		rtm.hdr.rtm_type = RTM_CHANGE;
	else if (action > 0)
		rtm.hdr.rtm_type = RTM_ADD;
	else
		rtm.hdr.rtm_type = RTM_DELETE;
	rtm.hdr.rtm_flags = RTF_UP;
	/* None interface subnet routes are static. */
	if (gate->s_addr != INADDR_ANY ||
	    net->s_addr != iface->net.s_addr ||
	    dest->s_addr != (iface->addr.s_addr & iface->net.s_addr))
		rtm.hdr.rtm_flags |= RTF_STATIC;
	rtm.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY;
	if (dest->s_addr == gate->s_addr && net->s_addr == INADDR_BROADCAST)
		rtm.hdr.rtm_flags |= RTF_HOST;
	else {
		rtm.hdr.rtm_addrs |= RTA_NETMASK;
		if (rtm.hdr.rtm_flags & RTF_STATIC)
			rtm.hdr.rtm_flags |= RTF_GATEWAY;
		if (action >= 0)
			rtm.hdr.rtm_addrs |= RTA_IFA;
	}

	ADDADDR(dest);
	if (rtm.hdr.rtm_flags & RTF_HOST ||
	    !(rtm.hdr.rtm_flags & RTF_STATIC))
	{
		/* Make us a link layer socket for the host gateway */
		memset(&su, 0, sizeof(su));
		su.sdl.sdl_len = sizeof(struct sockaddr_dl);
		link_addr(iface->name, &su.sdl);
		ADDSU(su);
	} else
		ADDADDR(gate);

	if (rtm.hdr.rtm_addrs & RTA_NETMASK) {
		/* Ensure that netmask is set correctly */
		memset(&su, 0, sizeof(su));
		su.sin.sin_family = AF_INET;
		su.sin.sin_len = sizeof(su.sin);
		memcpy(&su.sin.sin_addr, &net->s_addr, sizeof(su.sin.sin_addr));
		p = su.sa.sa_len + (char *)&su;
		for (su.sa.sa_len = 0; p > (char *)&su;)
			if (*--p != 0) {
				su.sa.sa_len = 1 + p - (char *)&su;
				break;
			}
		ADDSU(su);
	}

	if (rtm.hdr.rtm_addrs & RTA_IFA)
		ADDADDR(&iface->addr);

	rtm.hdr.rtm_msglen = l = bp - (char *)&rtm;
	if (write(s, &rtm, l) == -1)
		retval = -1;
	close(s);
	return retval;
}

int
open_link_socket(struct interface *iface)
{
	int fd;

	fd = socket(PF_ROUTE, SOCK_RAW, 0);
	if (fd == -1)
		return -1;
	set_cloexec(fd);
	if (iface->link_fd != -1)
		close(iface->link_fd);
	iface->link_fd = fd;
	return 0;
}

#define BUFFER_LEN	2048
int
link_changed(struct interface *iface)
{
	char buffer[2048], *p;
	ssize_t bytes;
	struct rt_msghdr *rtm;
	struct if_msghdr *ifm;
	int i;

	if ((i = if_nametoindex(iface->name)) == -1)
		return -1;
	for (;;) {
		bytes = recv(iface->link_fd, buffer, BUFFER_LEN, MSG_DONTWAIT);
		if (bytes == -1) {
			if (errno == EAGAIN)
				return 0;
			if (errno == EINTR)
				continue;
			return -1;
		}
		for (p = buffer; bytes > 0;
		     bytes -= ((struct rt_msghdr *)p)->rtm_msglen,
		     p += ((struct rt_msghdr *)p)->rtm_msglen)
		{
			rtm = (struct rt_msghdr *)p;
			if (rtm->rtm_type != RTM_IFINFO)
				continue;
			ifm = (struct if_msghdr *)p;
			if (ifm->ifm_index != i)
				continue;

			/* Link changed */
			return 1;
		}
	}
	return 0;
}
