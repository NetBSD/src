/*	$NetBSD: darwin_route.c,v 1.3 2004/07/21 23:43:25 manu Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_route.c,v 1.3 2004/07/21 23:43:25 manu Exp $");

#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <net/if.h>

#include <compat/darwin/darwin_socket.h>
#include <compat/darwin/darwin_route.h>

#define ALIGN(a)	(((a) + 3) & ~0x3UL)
int
darwin_ifaddrs(af, dst, sizep)
	int af;
	char *dst;
	size_t *sizep;
{
	struct darwin_if_msghdr dim;
	struct ifnet *ifp;
	int error;
	int index = 1;
	size_t size = 0;
	size_t maxsize = *sizep;

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		struct ifaddr *ifa;

		size += sizeof(struct darwin_if_msghdr);
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			size +=	sizeof(struct darwin_ifa_msghdr);
			if (ifa->ifa_addr)
				size += ALIGN(ifa->ifa_addr->sa_len);
			if (ifa->ifa_netmask)
				size += ALIGN(ifa->ifa_netmask->sa_len);
			if ((ifa->ifa_dstaddr != NULL) &&
			    (ifp->if_flags & IFF_POINTOPOINT))
				size += ALIGN(ifa->ifa_dstaddr->sa_len);
			if ((ifa->ifa_broadaddr != NULL) && 
			    (ifp->if_flags & IFF_BROADCAST))
				size += ALIGN(ifa->ifa_broadaddr->sa_len);
		}
	}

	*sizep = size;
	if (dst == NULL)
		return 0;
	if (size > maxsize)
		return ENOMEM;

	af = darwin_to_native_af[af];

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		struct ifaddr *ifa;

		dim.dim_len = sizeof(dim);
		dim.dim_vers = DARWIN_RTM_VERSION;
		dim.dim_type = DARWIN_RTM_IFINFO;
		dim.dim_addrs = DARWIN_RTA_IFP;
		dim.dim_flags = ifp->if_flags & 0xffff;
		dim.dim_index = index++;

		dim.dim_data.did_type = ifp->if_data.ifi_type; /* XXX */
		dim.dim_data.did_typelen = 0 ;/* XXX */
		dim.dim_data.did_physical = 0; /* XXX */
		dim.dim_data.did_addrlen = ifp->if_data.ifi_addrlen;
		dim.dim_data.did_hdrlen = ifp->if_data.ifi_hdrlen;
		dim.dim_data.did_recquota = 0; /* XXX */
		dim.dim_data.did_xmitquota = 0; /* XXX */
		dim.dim_data.did_mtu = ifp->if_data.ifi_mtu;
		dim.dim_data.did_metric = ifp->if_data.ifi_metric;
		dim.dim_data.did_baudrate = ifp->if_data.ifi_baudrate;
		dim.dim_data.did_ipackets = ifp->if_data.ifi_ipackets;
		dim.dim_data.did_ierrors = ifp->if_data.ifi_ierrors;
		dim.dim_data.did_opackets = ifp->if_data.ifi_opackets;
		dim.dim_data.did_oerrors = ifp->if_data.ifi_oerrors;
		dim.dim_data.did_collisions = ifp->if_data.ifi_collisions;
		dim.dim_data.did_ibytes = ifp->if_data.ifi_ibytes;
		dim.dim_data.did_obytes = ifp->if_data.ifi_obytes;
		dim.dim_data.did_imcasts = ifp->if_data.ifi_imcasts;
		dim.dim_data.did_omcasts = ifp->if_data.ifi_omcasts;
		dim.dim_data.did_iqdrops = ifp->if_data.ifi_iqdrops;
		dim.dim_data.did_noproto = ifp->if_data.ifi_noproto;
		dim.dim_data.did_lastchange.tv_sec = 
		    ifp->if_data.ifi_lastchange.tv_sec;
		dim.dim_data.did_lastchange.tv_usec = 
		    ifp->if_data.ifi_lastchange.tv_usec;
		dim.dim_data.did_default_proto = 0; /* XXX */
		dim.dim_data.did_hwassist = 0; /* XXX */
		
		dim.dim_index2 = dim.dim_index;
		dim.dim_type2 = dim.dim_data.did_type;
		dim.dim_xnamelen = strlen(ifp->if_xname);
		strlcpy(&dim.dim_xname[0], &ifp->if_xname[0], IFNAMSIZ);

#ifdef DEBUG_DARWIN
		printf("copyout dim 0x%x@%p\n", sizeof(dim), dst);
#endif
		if ((error = copyout(&dim, dst, sizeof(dim))) != 0)
			return error;
		dst += sizeof(dim);	

		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			struct darwin_ifa_msghdr diam;

			if (ifa->ifa_addr) {
				if ((af != 0) && 
				    (ifa->ifa_addr->sa_family != af))
					continue;
				if (ifa->ifa_addr->sa_family > AF_MAX)
					continue;
				if (ifa->ifa_addr->sa_family == 0);
					continue;
			}

			bzero(&diam, sizeof(diam));

			diam.diam_len = sizeof(diam);
			diam.diam_vers = DARWIN_RTM_VERSION;
			diam.diam_type = DARWIN_RTM_NEWADDR;

			if (ifa->ifa_addr) {
				diam.diam_addrs |= DARWIN_RTA_IFA;
				diam.diam_len += ALIGN(ifa->ifa_addr->sa_len);
			}
			if (ifa->ifa_netmask) {
				diam.diam_addrs |= DARWIN_RTA_NETMASK;
				diam.diam_len += 
				    ALIGN(ifa->ifa_netmask->sa_len);
			}
			if ((ifa->ifa_dstaddr != NULL) &&
			    (ifp->if_flags & IFF_POINTOPOINT)) {
				diam.diam_addrs |= DARWIN_RTA_DST;
				diam.diam_len += 
				    ALIGN(ifa->ifa_dstaddr->sa_len);
			}
			if ((ifa->ifa_broadaddr != NULL) && 
			    (ifp->if_flags & IFF_BROADCAST)) {
				diam.diam_addrs |= DARWIN_RTA_BRD;
				diam.diam_len += 
				    ALIGN(ifa->ifa_broadaddr->sa_len);
			}

			diam.diam_flags = (int)ifa->ifa_flags;
			diam.diam_index = dim.dim_index;
			diam.diam_metric = ifa->ifa_metric;
			
#ifdef DEBUG_DARWIN
			printf("copyout diam 0x%x@%p\n", sizeof(diam), dst);
#endif
			if ((error = copyout(&diam, dst, sizeof(diam))) != 0)
				return error;
			dst += sizeof(diam);	

			/* Interface netmask */
			if (diam.diam_addrs & DARWIN_RTA_NETMASK) {
				struct sockaddr_storage sa;
				size_t len = ifa->ifa_netmask->sa_len;

				native_to_darwin_sockaddr
				    (ifa->ifa_netmask, &sa);
#ifdef DEBUG_DARWIN
				printf("copyout netmask 0x%x@%p\n", len, dst);
#endif
				if ((error = copyout(&sa, dst, len)) != 0)
					return error;
				dst += ALIGN(len);
			}

			/* Interface address */
			if (diam.diam_addrs & DARWIN_RTA_IFA) {
				struct sockaddr_storage sa;
				size_t len = ifa->ifa_addr->sa_len;

				native_to_darwin_sockaddr(ifa->ifa_addr, &sa);
#ifdef DEBUG_DARWIN
				printf("copyout ifa 0x%x@%p\n", len, dst);
#endif
				if ((error = copyout(&sa, dst, len)) != 0)
					return error;
				dst += ALIGN(len);
			}
			
			/* Interface remote address */
			if (diam.diam_addrs & DARWIN_RTA_DST) {
				struct sockaddr_storage sa;
				size_t len = ifa->ifa_dstaddr->sa_len;

				native_to_darwin_sockaddr
				    (ifa->ifa_dstaddr, &sa);
#ifdef DEBUG_DARWIN
				printf("copyout dst 0x%x@%p\n", len, dst);
#endif
				if ((error = copyout(&sa, dst, len)) != 0)
					return error;
				dst += len;
			}
			
			/* Interface broadcast address */
			if (diam.diam_addrs & DARWIN_RTA_BRD) {
				struct sockaddr_storage sa;
				size_t len = ifa->ifa_broadaddr->sa_len;

				native_to_darwin_sockaddr
				    (ifa->ifa_broadaddr, &sa);
#ifdef DEBUG_DARWIN
				printf("copyout broad 0x%x@%p\n", len, dst);
#endif
				if ((error = copyout(&sa, dst, len)) != 0)
					return error;
				dst += len;
			}
		}
	}

	return 0;
}
