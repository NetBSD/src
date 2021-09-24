/*	$NetBSD: rss_config.c,v 1.3 2021/09/24 04:11:02 knakahara Exp $  */

/*
 * Copyright (c) 2018 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: rss_config.c,v 1.3 2021/09/24 04:11:02 knakahara Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>

#include <net/rss_config.h>
#include <net/toeplitz.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>

/*
 * Same as FreeBSD.
 *
 * This rss key is assumed for verification suite in many intel Gigabit and
 * 10 Gigabit Controller specifications.
 */
static uint8_t rss_default_key[RSS_KEYSIZE] = {
	0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
	0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
	0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
	0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
	0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa,
};

#ifdef NOTYET
/*
 * Same as DragonFlyBSD.
 *
 * This rss key make rss hash value symmetric, that is, the hash value
 * calculated by func("source address", "destination address") equals to
 * the hash value calculated by func("destination address", "source address").
 */
static uint8_t rss_symmetric_key[RSS_KEYSIZE] = {
	0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a,
	0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a,
	0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a,
	0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a,
	0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a, 0x6d, 0x5a,
};
#endif

/*
 * sizeof(key) must be more than or equal to RSS_KEYSIZE.
 */
void
rss_getkey(uint8_t *key)
{

	memcpy(key, rss_default_key, sizeof(rss_default_key));
}

/*
 * Calculate rss hash value from IPv4 mbuf.
 * This function should be called before ip_input().
 */
uint32_t
rss_toeplitz_hash_from_mbuf_ipv4(const struct mbuf *m, u_int flag)
{
	struct ip *ip;
	int hlen;
	uint8_t key[RSS_KEYSIZE];

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT(m->m_len >= sizeof (struct ip));

	ip = mtod(m, struct ip *);
	KASSERT(ip->ip_v == IPVERSION);

	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip))
		return 0;

	rss_getkey(key);

	switch (ip->ip_p) {
	case IPPROTO_TCP:
	{
		if ((flag & RSS_TOEPLITZ_USE_TCP_PORT) != 0) {
			if (m->m_len >= hlen + sizeof(struct tcphdr)) {
				struct tcphdr *th;

				th = (struct tcphdr *)(mtod(m, char *) + hlen);
				return toeplitz_vhash(key, sizeof(key),
				    /* ip_src and ip_dst in struct ip must be sequential */
				    &ip->ip_src, sizeof(ip->ip_src) * 2,
				    /* th_sport and th_dport in tcphdr must be sequential */
				    &th->th_sport, sizeof(th->th_sport) * 2,
				    NULL);
			} else if (m->m_pkthdr.len >= hlen + sizeof(struct tcphdr)) {
				uint16_t ports[2];

				/* ditto */
				m_copydata(__UNCONST(m), hlen + offsetof(struct tcphdr, th_sport),
				    sizeof(ports), ports);
				return toeplitz_vhash(key, sizeof(key),
				    &ip->ip_src, sizeof(ip->ip_src) * 2,
				    ports, sizeof(ports),
				    NULL);
			}
		}
		/*
		 * Treat as raw packet.
		 */
		return toeplitz_vhash(key, sizeof(key),
		    /* ditto */
		    &ip->ip_src, sizeof(ip->ip_src) * 2,
		    NULL);
	}
	case IPPROTO_UDP:
	{
		if ((flag & RSS_TOEPLITZ_USE_UDP_PORT) != 0) {
			if (m->m_len >= hlen + sizeof(struct udphdr)) {
				struct udphdr *uh;

				uh = (struct udphdr *)(mtod(m, char *) + hlen);
				return toeplitz_vhash(key, sizeof(key),
				    /* ip_src and ip_dst in struct ip must sequential */
				    &ip->ip_src, sizeof(ip->ip_src) * 2,
				    /* uh_sport and uh_dport in udphdr must be sequential */
				    &uh->uh_sport, sizeof(uh->uh_sport) * 2,
				    NULL);
			} else if (m->m_pkthdr.len >= hlen + sizeof(struct udphdr)) {
				uint16_t ports[2];

				/* ditto */
				m_copydata(__UNCONST(m), hlen + offsetof(struct udphdr, uh_sport),
				    sizeof(ports), ports);
				return toeplitz_vhash(key, sizeof(key),
				    &ip->ip_src, sizeof(ip->ip_src) * 2,
				    ports, sizeof(ports),
				    NULL);
			}
		}
		/*
		 * Treat as raw packet.
		 */
		return toeplitz_vhash(key, sizeof(key),
		    /* ditto */
		    &ip->ip_src, sizeof(ip->ip_src) * 2,
		    NULL);
	}
	/*
	 * Other protocols are treated as raw packets to apply RPS.
	 */
	default:
		return toeplitz_vhash(key, sizeof(key),
		    /* ditto */
		    &ip->ip_src, sizeof(ip->ip_src) * 2,
		    NULL);
	}
}

/*
 * Calculate rss hash value from IPv6 mbuf.
 * This function should be called before ip6_input().
 */
uint32_t
rss_toeplitz_hash_from_mbuf_ipv6(const struct mbuf *m, u_int flag)
{
	struct ip6_hdr *ip6;
	int hlen;
	uint8_t key[RSS_KEYSIZE];

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT(m->m_len >= sizeof (struct ip6_hdr));

	ip6 = mtod(m, struct ip6_hdr *);
	KASSERT((ip6->ip6_vfc & IPV6_VERSION_MASK) == IPV6_VERSION);

	hlen = sizeof(struct ip6_hdr);
	rss_getkey(key);

	switch (ip6->ip6_nxt) {
	case IPPROTO_TCP:
	{
		if ((flag & RSS_TOEPLITZ_USE_TCP_PORT) != 0) {
			if (m->m_len >= hlen + sizeof(struct tcphdr)) {
				struct tcphdr *th;

				th = (struct tcphdr *)(mtod(m, char *) + hlen);
				return toeplitz_vhash(key, sizeof(key),
				    /* ip6_src and ip6_dst in ip6_hdr must be sequential */
				    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
				    /* th_sport and th_dport in tcphdr must be sequential */
				    &th->th_sport, sizeof(th->th_sport) * 2,
				    NULL);
			} else if (m->m_pkthdr.len >= hlen + sizeof(struct tcphdr)) {
				uint16_t ports[2];

				/* ditto */
				m_copydata(__UNCONST(m), hlen + offsetof(struct tcphdr, th_sport),
				    sizeof(ports), ports);
				return toeplitz_vhash(key, sizeof(key),
				    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
				    ports, sizeof(ports),
				    NULL);
			}
		}
		/*
		 * Treat as raw packet.
		 */
		return toeplitz_vhash(key, sizeof(key),
		    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
		    NULL);
	}
	case IPPROTO_UDP:
	{
		if ((flag & RSS_TOEPLITZ_USE_UDP_PORT) != 0) {
			if (m->m_len >= hlen + sizeof(struct udphdr)) {
				struct udphdr *uh;

				uh = (struct udphdr *)(mtod(m, char *) + hlen);
				return toeplitz_vhash(key, sizeof(key),
				    /* ip6_src and ip6_dst in ip6_hdr must sequential */
				    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
				    /* uh_sport and uh_dport in udphdr must be sequential */
				    &uh->uh_sport, sizeof(uh->uh_sport) * 2,
				    NULL);
			} else if (m->m_pkthdr.len >= hlen + sizeof(struct udphdr)) {
				uint16_t ports[2];

				/* ditto */
				m_copydata(__UNCONST(m), hlen + offsetof(struct udphdr, uh_sport),
				    sizeof(ports), ports);
				return toeplitz_vhash(key, sizeof(key),
				    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
				    &ports, sizeof(ports),
				    NULL);
			}
		}
		/*
		 * Treat as raw packet.
		 */
		return toeplitz_vhash(key, sizeof(key),
		    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
		    NULL);
	}
	/*
	 * Other protocols are treated as raw packets to apply RPS.
	 */
	default:
		return toeplitz_vhash(key, sizeof(key),
		    &ip6->ip6_src, sizeof(ip6->ip6_src) * 2,
		    NULL);
	}

	return 0;
}
