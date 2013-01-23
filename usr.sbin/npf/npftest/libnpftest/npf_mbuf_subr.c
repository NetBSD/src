/*	$NetBSD$	*/

/*
 * NPF testing - helper routines.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/kmem.h>

#include "npf_impl.h"
#include "npf_test.h"

struct mbuf *
mbuf_getwithdata(const void *data, size_t len)
{
	struct mbuf *m;
	void *dst;

	m = m_gethdr(M_WAITOK, MT_HEADER);
	assert(m != NULL);
	dst = mtod(m, void *);
	memcpy(dst, data, len);
	m->m_pkthdr.len = len;
	m->m_len = len;
	return m;
}

struct mbuf *
mbuf_construct_ether(int proto)
{
	struct mbuf *m0, *m1;
	struct ether_header *ethdr;

	m0 = m_gethdr(M_WAITOK, MT_HEADER);
	ethdr = mtod(m0, struct ether_header *);
	ethdr->ether_type = htons(ETHERTYPE_IP);
	m0->m_pkthdr.len = sizeof(struct ether_header);
	m0->m_len = sizeof(struct ether_header);

	m1 = mbuf_construct(proto);
	m0->m_next = m1;
	m1->m_next = NULL;
	return m0;
}

static int
mbuf_fill_proto(int proto, void *l4data)
{
	struct tcphdr *th;
	int size = 0;

	switch (proto) {
	case IPPROTO_TCP:
		th = l4data;
		th->th_off = sizeof(struct tcphdr) >> 2;
		size = sizeof(struct tcphdr);
		break;
	case IPPROTO_UDP:
		size = sizeof(struct udphdr);
		break;
	case IPPROTO_ICMP:
		size = offsetof(struct icmp, icmp_data);
		break;
	}
	return size;
}

struct mbuf *
mbuf_construct(int proto)
{
	struct mbuf *m;
	struct ip *iphdr;
	void *l4data;
	int size;

	m = m_gethdr(M_WAITOK, MT_HEADER);
	iphdr = mtod(m, struct ip *);

	iphdr->ip_v = IPVERSION;
	iphdr->ip_hl = sizeof(struct ip) >> 2;
	iphdr->ip_off = 0;
	iphdr->ip_ttl = 64;
	iphdr->ip_p = proto;

	size = sizeof(struct ip);
	l4data = (void *)(iphdr + 1);
	size += mbuf_fill_proto(proto, l4data);
	iphdr->ip_len = htons(size);

	m->m_pkthdr.len = size;
	m->m_len = size;
	m->m_next = NULL;
	return m;
}

struct mbuf *
mbuf_construct6(int proto)
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	void *l4data;
	int size;

	m = m_gethdr(M_WAITOK, MT_HEADER);
	ip6 = mtod(m, struct ip6_hdr *);

	ip6->ip6_vfc = IPV6_VERSION;
	ip6->ip6_nxt = proto;
	ip6->ip6_hlim = 64;

	size = sizeof(struct ip6_hdr);
	l4data = (void *)(ip6 + 1);
	size += mbuf_fill_proto(proto, l4data);
	ip6->ip6_plen = htons(size);

	m->m_pkthdr.len = size;
	m->m_len = size;
	m->m_next = NULL;
	return m;
}

void *
mbuf_return_hdrs(struct mbuf *m, bool ether, struct ip **ip)
{
	struct ip *iphdr;

	if (ether) {
		struct mbuf *mn = m->m_next;
		iphdr = mtod(mn, struct ip *);
	} else {
		iphdr = mtod(m, struct ip *);
	}
	*ip = iphdr;
	return (void *)(iphdr + 1);
}

void
mbuf_icmp_append(struct mbuf *m, struct mbuf *m_orig)
{
	struct ip *iphdr = mtod(m, struct ip *);
	const size_t hlen = iphdr->ip_hl << 2;
	struct icmp *ic = (struct icmp *)((uint8_t *)iphdr + hlen);
	const size_t addlen = m_length(m_orig);

	iphdr->ip_len = htons(ntohs(iphdr->ip_len) + addlen);
	memcpy(&ic->icmp_ip, mtod(m_orig, struct ip *), addlen);
	m->m_pkthdr.len += addlen;
	m->m_len += addlen;
	m_freem(m_orig);
}
