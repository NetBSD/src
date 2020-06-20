/*
 * NPF testing - helper routines.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/kmem.h>
#endif

#include "npf_impl.h"
#include "npf_test.h"


#if defined(_NPF_STANDALONE)
struct mbuf *
npfkern_m_get(npf_t *npf __unused, unsigned flags, size_t space)
{
	unsigned mlen = offsetof(struct mbuf, m_data0[space]);
	struct mbuf *m;

	m = calloc(1, mlen);
	if (m) {
		m->m_type = 1;
		m->m_flags = flags;
		m->m_data = m->m_data0;
	}
	return m;
}
#else
struct mbuf *
npfkern_m_get(npf_t *npf __unused, unsigned flags, size_t space)
{
	return m_get(flags, space);
}
#endif

static void *
npfkern_m_getdata(const struct mbuf *m)
{
	return m->m_data;
}

static struct mbuf *
npfkern_m_next(struct mbuf *m)
{
	return m->m_next;
}

static size_t
npfkern_m_buflen(const struct mbuf *m)
{
	return m->m_len;
}

size_t
npfkern_m_length(const struct mbuf *m)
{
	const struct mbuf *m0;
	unsigned pktlen = 0;

	if ((m->m_flags & M_PKTHDR) != 0)
		return m->m_pkthdr.len;
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		pktlen += m0->m_len;
	return pktlen;
}

void
npfkern_m_freem(struct mbuf *m)
{
#ifdef _NPF_STANDALONE
	struct mbuf *n;

	do {
		n = m->m_next;
		m->m_type = MT_FREE;
		free(m);
		m = n;
	} while (m);
#else
	m_freem(m);
#endif
}

static bool
npfkern_m_ensure_contig(struct mbuf **m0, size_t len)
{
	struct mbuf *m1;
	unsigned tlen;
	char *dptr;

	tlen = npfkern_m_length(*m0);
	if ((m1 = npfkern_m_get(NULL, M_PKTHDR, tlen)) == NULL) {
		return false;
	}
	m1->m_pkthdr.len = m1->m_len = tlen;
	dptr = m1->m_data;
	for (struct mbuf *m = *m0; m != NULL; m = m->m_next) {
		memcpy(dptr, m->m_data, m->m_len);
		dptr += m->m_len;
	}
	npfkern_m_freem(*m0);
	*m0 = m1;
	(void)len;
	return true;
}


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

void *
mbuf_return_hdrs6(struct mbuf *m, struct ip6_hdr **ip6)
{
	struct ip6_hdr *ip6hdr = mtod(m, struct ip6_hdr *);

	*ip6 = ip6hdr;
	return (void *)(ip6hdr + 1);
}

void
mbuf_icmp_append(struct mbuf *m, struct mbuf *m_orig)
{
	struct ip *iphdr = mtod(m, struct ip *);
	const size_t hlen = iphdr->ip_hl << 2;
	void *p = (uint8_t *)iphdr + hlen;
	struct icmp *ic = (struct icmp *)p;
	const size_t addlen = m_length(m_orig);

	iphdr->ip_len = htons(ntohs(iphdr->ip_len) + addlen);
	memcpy(&ic->icmp_ip, mtod(m_orig, struct ip *), addlen);
	m->m_pkthdr.len += addlen;
	m->m_len += addlen;
	m_freem(m_orig);
}

struct mbuf *
mbuf_get_pkt(int af, int proto, const char *src, const char *dst,
    int sport, int dport)
{
	struct mbuf *m;
	struct ip *ip;
	struct ip6_hdr *ip6;
	struct tcphdr *th;
	struct udphdr *uh;
	void *p, *ipsrc, *ipdst;

	switch (af) {
	case AF_INET6:
		m = mbuf_construct6(proto);
		p = mbuf_return_hdrs6(m, &ip6);
		ipsrc = &ip6->ip6_src;
		ipdst = &ip6->ip6_dst;
		break;
	case AF_INET:
	default:
		m = mbuf_construct(proto);
		p = mbuf_return_hdrs(m, false, &ip);
		ipsrc = &ip->ip_src.s_addr;
		ipdst = &ip->ip_dst.s_addr;
	}

	npf_inet_pton(af, src, ipsrc);
	npf_inet_pton(af, dst, ipdst);

	switch (proto) {
	case IPPROTO_TCP:
		th = p;
		th->th_sport = htons(sport);
		th->th_dport = htons(dport);
		break;
	case IPPROTO_UDP:
		uh = p;
		uh->uh_sport = htons(sport);
		uh->uh_dport = htons(dport);
		break;
	default:
		KASSERT(false);
	}
	return m;
}

npf_cache_t *
get_cached_pkt(struct mbuf *m, const char *ifname)
{
	ifnet_t *ifp = npf_test_getif(ifname ? ifname : IFNAME_DUMMY);
	npf_cache_t *npc = kmem_zalloc(sizeof(npf_cache_t), KM_SLEEP);
	nbuf_t *nbuf = kmem_zalloc(sizeof(nbuf_t), KM_SLEEP);
	int ret;

	npc->npc_info = 0;
	npc->npc_ctx = npf_getkernctx();

	nbuf_init(npc->npc_ctx, nbuf, m, ifp);
	npc->npc_nbuf = nbuf;
	ret = npf_cache_all(npc);
	assert(ret); (void)ret;

	return npc;
}

void
put_cached_pkt(npf_cache_t *npc)
{
	struct mbuf *m = nbuf_head_mbuf(npc->npc_nbuf);
	kmem_free(npc->npc_nbuf, sizeof(nbuf_t));
	kmem_free(npc, sizeof(npf_cache_t));
	m_freem(m);
}

const npf_mbufops_t npftest_mbufops = {
	.alloc			= npfkern_m_get,
	.free			= npfkern_m_freem,
	.getdata		= npfkern_m_getdata,
	.getnext		= npfkern_m_next,
	.getlen			= npfkern_m_buflen,
	.getchainlen		= npfkern_m_length,
	.ensure_contig		= npfkern_m_ensure_contig,
	.ensure_writable	= NULL,
};
