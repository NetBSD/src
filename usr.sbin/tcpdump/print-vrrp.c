/*	$NetBSD: print-vrrp.c,v 1.1 2001/01/19 09:10:13 kleink Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein of the Security Competence Center, KPNQwest Germany GmbH.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"

#define	VRRP_MCAST_DSTADDR	0xe0000012	/* 224.0.0.18 */

/* Note: this is passed down aligned, and therefore safe. */
struct vrrphdr {
	uint8_t		vers_type;	/* protocol version, data type */
	uint8_t		vrrid;		/* virtual router ID */
	uint8_t		priority;	/* sender's priority for VR */
	uint8_t		addrcount;	/* # of addresses to follow */
	uint8_t		auth_type;	/* authentication type */
	uint8_t		adver_int;	/* Advertisement Interval */
	uint16_t	checksum;	/* Checksum */
	/* #addrcount IPv4 addresses */
	/* 8 bytes of VRRP_AUTH_SIMPLE data */
};

/* vers_type */
#define VRRP_VERSION_SHIFT	4
#define VRRP_VERSION_MASK	0x0f
#define VRRP_VERSION(vp) \
	    (((vp)->vers_type >> VRRP_VERSION_SHIFT) & VRRP_VERSION_MASK)

#define VRRP_TYPE_SHIFT		0
#define VRRP_TYPE_MASK		0x0f
#define VRRP_TYPE(vp) \
	    (((vp)->vers_type >> VRRP_TYPE_SHIFT) & VRRP_TYPE_MASK)

#define VRRP_TYPE_ADVERTISEMENT	1

/* auth_type */
#define VRRP_AUTH_NONE		0
#define VRRP_AUTH_SIMPLE	1
#define VRRP_AUTH_AH		2

#define VRRP_AUTHDATA(vp) \
	    ((unsigned char *)((vp) + 1) + \
	     (vp)->addrcount * sizeof (struct in_addr))
#define VRRP_AUTHDATA_LEN	8

static unsigned int vrrp_cksum(const struct vrrphdr *, unsigned int);

static unsigned int
vrrp_cksum(const struct vrrphdr *vp, unsigned int len)
{
	const uint16_t *sp;
	unsigned int sum;

	sum = 0;
	for (sp = (uint16_t *)vp; sp < &vp->checksum; sp++)
		sum += ntohs(*sp);
	for (sp += 1; sp < (uint16_t *)((uint8_t *)vp + len); sp++)
		sum += ntohs(*sp);

	while (sum > 0xffff)
		sum = (sum & 0xffff) + (sum >> 16);
	sum = ~sum & 0xffff;

	return (sum);
}

void
vrrp_print(const u_char *bp, u_int length, const u_char *bp2)
{
	const struct vrrphdr *vp;
	const struct ip *ip;
	const struct in_addr *iap;
	unsigned int version, type;
	int i;

	vp = (const struct vrrphdr *)bp;
	ip = (const struct ip *)bp2;
	iap = (const struct in_addr *)(vp + 1);

	if (length < sizeof (*vp) ||
	    length < sizeof (*vp) + vp->addrcount * 4 + VRRP_AUTHDATA_LEN) {
		(void)printf(" truncated-vrrp %u\n", length);
		return;
	}

	version = vp->vers_type >> 4;
	type = vp->vers_type & 0xf;

	(void)printf("VRRPv%u", version);

	if (ntohs(vp->checksum) != vrrp_cksum(vp, length))
		printf(" [bad cksum 0x%04x]", ntohs(vp->checksum));
	else if (vflag)
		printf(" [cksum ok]");

	if (ntohl(ip->ip_dst.s_addr) != VRRP_MCAST_DSTADDR)
		(void)printf(" [bad dst %s]", ipaddr_string(&ip->ip_dst));
	if (ip->ip_ttl < 255U)
		(void)printf(" [bad ttl %u]", ip->ip_ttl);

	switch (version) {
	case 2:
		switch (type) {
		case VRRP_TYPE_ADVERTISEMENT:
			printf(" adv");

			printf(" vrrid %u prio %u", vp->vrrid, vp->priority);

			switch (vp->auth_type) {
			case VRRP_AUTH_NONE:
				printf(" auth-none");
				break;
			case VRRP_AUTH_SIMPLE:
				printf(" auth-simple");
				if (vflag >= 2) {
					printf(" [authdata ");
					(void)fn_printn(VRRP_AUTHDATA(vp),
					    VRRP_AUTHDATA_LEN, NULL);
					printf("]");
				}
				break;
			case VRRP_AUTH_AH:
				printf(" auth-ah");
				break;
			default:
				printf(" auth-%u", vp->auth_type);
				break;
			}

			printf(" adv-interval %u", vp->adver_int);

			if (vp->addrcount > 0)
				printf(" assoc-addr");
			for (i = 0; i < vp->addrcount; i++) {
				printf(" %s", ipaddr_string(&iap[i]));
			}
			break;

		default:	/* type */
			printf(" type-%u", type);
			return;
		}
		break;

	default:	/* version */
		break;
	}
}
