/*	$NetBSD: xennet_checksum.c,v 1.8 2020/03/19 10:53:43 jdolecek Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xennet_checksum.c,v 1.8 2020/03/19 10:53:43 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_offload.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <xen/xennet_checksum.h>

static struct evcnt xn_cksum_defer = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "xennet", "csum blank");
static struct evcnt xn_cksum_undefer = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "xennet", "csum undeferred");
static struct evcnt xn_cksum_valid = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "xennet", "csum data valid");

EVCNT_ATTACH_STATIC(xn_cksum_defer);
EVCNT_ATTACH_STATIC(xn_cksum_undefer);
EVCNT_ATTACH_STATIC(xn_cksum_valid);

/* ratecheck(9) for checksum validation failures */
static const struct timeval xn_cksum_errintvl = { 600, 0 };  /* 10 min, each */

static void *
m_extract(struct mbuf *m, int off, int len)
{
	KASSERT(m->m_pkthdr.len >= off + len);
	KASSERT(m->m_len >= off + len);

	if (m->m_pkthdr.len >= off + len)
		return mtod(m, char *) + off;
	else
		return NULL;
}

/*
 * xennet_checksum_fill: fill TCP/UDP checksum, or arrange
 * for hw offload to do it
 */
int
xennet_checksum_fill(struct ifnet *ifp, struct mbuf *m, bool data_validated)
{
	const struct ether_header *eh;
	struct ip *iph;
	int ehlen;
	int iphlen;
	int iplen;
	uint16_t etype;
	uint8_t nxt;
	int error = 0;
	int sw_csum;

	KASSERT(!M_READONLY(m));
	KASSERT((m->m_flags & M_PKTHDR) != 0);

	eh = m_extract(m, 0, sizeof(*eh));
	if (eh == NULL) {
		/* Too short, packet will be dropped by upper layer */
		return EINVAL;
	}
	etype = eh->ether_type;
	if (etype == htobe16(ETHERTYPE_VLAN)) {
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else if (etype == htobe16(ETHERTYPE_IP)) {
		ehlen = ETHER_HDR_LEN;
	} else {
		static struct timeval lasttime;
		if (ratecheck(&lasttime, &xn_cksum_errintvl))
			printf("%s: unknown etype %#x passed%s\n",
			    ifp->if_xname, ntohs(etype),
			    data_validated ? "" : " no checksum");
		return EINVAL;
	}

	iph = m_extract(m, ehlen, sizeof(*iph));
	if (iph == NULL) {
		/* Too short, packet will be dropped by upper layer */
		return EINVAL;
	}
	nxt = iph->ip_p;
	iphlen = iph->ip_hl << 2;
	iplen = ntohs(iph->ip_len);
	if (ehlen + iplen > m->m_pkthdr.len) {
		/* Too short, packet will be dropped by upper layer */
		return EINVAL;
	}

	switch (nxt) {
	case IPPROTO_UDP:
		m->m_pkthdr.csum_flags = M_CSUM_UDPv4 | M_CSUM_IPv4;
		m->m_pkthdr.csum_data = offsetof(struct udphdr, uh_sum);
		m->m_pkthdr.csum_data |= iphlen << 16;
		break;
	case IPPROTO_TCP:
		m->m_pkthdr.csum_flags = M_CSUM_TCPv4 | M_CSUM_IPv4;
		m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
		m->m_pkthdr.csum_data |= iphlen << 16;
		break;
	case IPPROTO_ICMP:
	case IPPROTO_IGMP:
		m->m_pkthdr.csum_flags = M_CSUM_IPv4;
		m->m_pkthdr.csum_data = iphlen << 16;
		break;
	default:
	    {
		static struct timeval lasttime;
		if (ratecheck(&lasttime, &xn_cksum_errintvl))
			printf("%s: unknown proto %d passed%s\n",
			    ifp->if_xname, nxt,
			    data_validated ? "" : " no checksum");
		error = EINVAL;
		goto out;
	    }
	}

	if (!data_validated) {
		/*
		 * Only compute the checksum if impossible to defer.
		 */
		sw_csum = m->m_pkthdr.csum_flags & ~ifp->if_csum_flags_rx;

		/*
		 * Always initialize the sum to 0!  Some HW assisted
		 * checksumming requires this. in_undefer_cksum()
		 * also needs it to be zero.
		 */
		if (m->m_pkthdr.csum_flags & M_CSUM_IPv4)
			iph->ip_sum = 0;

		if (sw_csum & (M_CSUM_IPv4|M_CSUM_UDPv4|M_CSUM_TCPv4)) {
			in_undefer_cksum(m, ehlen,
			    sw_csum & (M_CSUM_IPv4|M_CSUM_UDPv4|M_CSUM_TCPv4));
		}

		if (m->m_pkthdr.csum_flags != 0) {
			xn_cksum_defer.ev_count++;
#ifdef M_CSUM_BLANK
			m->m_pkthdr.csum_flags |= M_CSUM_BLANK;
#endif
		} else {
			xn_cksum_undefer.ev_count++;
		}
	} else {
		xn_cksum_valid.ev_count++;
	}

    out:
	return error;
}
