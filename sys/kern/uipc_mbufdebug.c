/*	$NetBSD: uipc_mbufdebug.c,v 1.2.2.2 2018/07/28 04:38:08 pgoyette Exp $	*/

/*
 * Copyright (C) 2017 Internet Initiative Japan Inc.
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
__KERNEL_RCSID(0, "$NetBSD: uipc_mbufdebug.c,v 1.2.2.2 2018/07/28 04:38:08 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/ppp_defs.h>
#include <net/if_arp.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/if_inarp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define EXAMINE_HEX_LIMIT 128
#define EXAMINE_HEX_COL 4

/* mbuf operations without change of mbuf chain */
static int m_peek_data(const struct mbuf *, int, int, void *);
static unsigned int m_peek_len(const struct mbuf *, const char *);

/* utility */
static char *str_ethaddr(const uint8_t *);
static char *str_ipaddr(const struct in_addr *);
static char *str_ip6addr(const struct in6_addr *);
static const char *str_ipproto(const uint8_t);

/* parsers for m_examine() */
static void m_examine_ether(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_pppoe(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_ppp(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_arp(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_ip(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_icmp(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_ip6(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_icmp6(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_tcp(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_udp(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));
static void m_examine_hex(const struct mbuf *, int, const char *,
    void (*)(const char *, ...));

/* header structure for some protocol */
struct pppoehdr {
	uint8_t vertype;
	uint8_t code;
	uint16_t session;
	uint16_t plen;
} __attribute__((__packed__));

struct pppoetag {
	uint16_t tag;
	uint16_t len;
} __attribute__((__packed__));

#define PPPOE_TAG_EOL 0x0000
#define PPPOE_CODE_PADI		0x09	/* Active Discovery Initiation */
#define	PPPOE_CODE_PADO		0x07	/* Active Discovery Offer */
#define	PPPOE_CODE_PADR		0x19	/* Active Discovery Request */
#define	PPPOE_CODE_PADS		0x65	/* Active Discovery Session confirmation */
#define	PPPOE_CODE_PADT		0xA7	/* Active Discovery Terminate */

struct ppp_header {
	uint8_t address;
	uint8_t control;
	uint16_t protocol;
} __attribute__((__packed__));

#define CISCO_MULTICAST		0x8f	/* Cisco multicast address */
#define CISCO_UNICAST		0x0f	/* Cisco unicast address */
#define CISCO_KEEPALIVE		0x8035	/* Cisco keepalive protocol */

#ifndef NELEMS
#define NELEMS(elem) ((sizeof(elem))/(sizeof((elem)[0])))
#endif

static int
m_peek_data(const struct mbuf *m, int off, int len, void *vp)
{
	unsigned int count;
	char *cp = vp;

	if (off < 0 || len < 0)
		return -1;

	while (off > 0) {
		if (m == 0)
			return -1;
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == 0)
			return -1;
		count = min(m->m_len - off, len);
		memcpy(cp, mtod(m, char *) + off, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}

	return 0;
}

static unsigned int
m_peek_len(const struct mbuf *m, const char *modif)
{
	const struct mbuf *m0;
	unsigned int pktlen;
	boolean_t opt_c = FALSE;
	unsigned char ch;

	while ( modif && (ch = *(modif++)) != '\0') {
		switch (ch) {
		case 'c':
			opt_c = TRUE;
			break;
		}
	}

	if (opt_c == TRUE) {
		return m->m_len;
	}

	if ((m->m_flags & M_PKTHDR) != 0)
		return m->m_pkthdr.len;

	pktlen = 0;
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		pktlen += m0->m_len;

	return pktlen;
}

static char *
str_ethaddr(const uint8_t *ap)
{
	static char buf[3 * ETHER_ADDR_LEN];

	return ether_snprintf(buf, sizeof(buf), ap);
}

static char *
str_ipaddr(const struct in_addr *ap)
{
	static char buf[INET_ADDRSTRLEN];

	return IN_PRINT(buf, ap);
}

static char *
str_ip6addr(const struct in6_addr *ap)
{
	static char buf[INET6_ADDRSTRLEN];

	return IN6_PRINT(buf, ap);
}

static const char *
str_ipproto(const uint8_t proto)
{
	switch (proto) {
	case IPPROTO_HOPOPTS:
		return ("IPv6 Hop-by-Hop");
		break;
	case IPPROTO_TCP:
		return("TCP");
		break;
	case IPPROTO_UDP:
		return("UDP");
		break;
	case IPPROTO_ICMP:
		return("ICMP");
		break;
	case IPPROTO_IGMP:
		return("IGMP");
		break;
	case IPPROTO_ESP:
		return("ESP");
		break;
	case IPPROTO_AH:
		return("AH");
		break;
	case IPPROTO_IPV6_ICMP:
		return("ICMP6");
	default:
		return("unknown");
		break;
	}

	/* not reached */
	return NULL;
}

static void
m_examine_ether(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	struct ether_header eh;
	unsigned int pktlen;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(eh)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(eh), (void *)(&eh)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(eh);

	(*pr)("ETHER: DST = %s\n", str_ethaddr(eh.ether_dhost));
	(*pr)("ETHER: SRC = %s\n", str_ethaddr(eh.ether_shost));

	(*pr)("ETHER: TYPE = 0x%04x(", ntohs(eh.ether_type));
	switch (ntohs(eh.ether_type)) {
	case ETHERTYPE_PPPOE:
		(*pr)("PPPoE)\n");
		return m_examine_pppoe(m, off, modif, pr);
		break;
	case ETHERTYPE_ARP:
		(*pr)("ARP)\n");
		return m_examine_arp(m, off, modif, pr);
		break;
	case ETHERTYPE_IP:
		(*pr)("IPv4)\n");
		return m_examine_ip(m, off, modif, pr);
		break;
	case ETHERTYPE_IPV6:
		(*pr)("IPv6)\n");
		return m_examine_ip6(m, off, modif, pr);
		break;
	default:
		(*pr)("unknown)\n");
		break;
	}

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_pppoe(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	struct pppoehdr ph;
	struct pppoetag pt;
	unsigned int pktlen;
	uint8_t vt;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(ph)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(ph), (void *)(&ph)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(ph);

	while (off + sizeof(pt) > pktlen) {
		if (m_peek_data(m, off, sizeof(pt), (void *)(&pt)) < 0) {
			(*pr)("%s: cannot read header\n", __func__);
			return m_examine_hex(m, off, modif, pr);
		}
		off += sizeof(pt);

		if (ntohs(pt.tag) == PPPOE_TAG_EOL)
			break;
		off += ntohs(pt.len);
	}

	vt = ph.vertype;

	(*pr)("PPPoE: Version = %u\n", ((vt >> 4) & 0xff));
	(*pr)("PPPoE: Type = %u\n", (vt & 0xff));
	(*pr)("PPPoE: Code = %u(", ph.code);
	switch (ph.code) {
	case 0:
		(*pr)("DATA");
		break;
	case PPPOE_CODE_PADI:
		(*pr)("PADI");
		break;
	case PPPOE_CODE_PADO:
		(*pr)("PADO");
		break;
	case PPPOE_CODE_PADS:
		(*pr)("PADS");
		break;
	case PPPOE_CODE_PADT:
		(*pr)("PADT");
		break;
	default:
		(*pr)("unknown");
		break;
	}
	(*pr)(")\n");

	(*pr)("PPPoE: Session = 0x%04x\n", ntohs(ph.session));
	(*pr)("PPPoE: Payload Length = %u\n", ntohs(ph.plen));

	switch (ph.code) {
	case PPPOE_CODE_PADI:
	case PPPOE_CODE_PADO:
	case PPPOE_CODE_PADS:
	case PPPOE_CODE_PADT:
		(*pr)("No parser for PPPoE control frame.\n");
		return m_examine_hex(m, off, modif, pr);
		break;
	}

	if (ph.code != 0) {
		(*pr)("Unknown PPPoE code.\n");
		return m_examine_hex(m, off, modif, pr);
	}

	return m_examine_ppp(m, off, modif, pr);
}

static void
m_examine_ppp(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	struct ppp_header h;
	unsigned int pktlen;
	uint16_t protocol;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(h)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(h), (void *)(&h)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(h);

	protocol = ntohs(h.protocol);

	(*pr)("SPPP: Address = %d(", h.address);
	switch (h.address) {
	case PPP_ALLSTATIONS:
		(*pr)("ALLSTATIONS)\n");
		(*pr)("SPPP: Protocol = %d(", protocol);
		switch (protocol) {
		case PPP_LCP:
			(*pr)("LCP)\n");
			break;
		case PPP_PAP:
			(*pr)("PAP)\n");
			break;
		case PPP_CHAP:
			(*pr)("CHAP)\n");
			break;
		case PPP_IPCP:
			(*pr)("IPCP)\n");
			break;
		case PPP_IPV6CP:
			(*pr)("IPV6CP)\n");
			break;
		case PPP_IP:
			(*pr)("IP)\n");
			return m_examine_ip(m, off, modif, pr);
			break;
		case PPP_IPV6:
			(*pr)("IPv6)\n");
			return m_examine_ip6(m, off, modif, pr);
			break;
		default:
			(*pr)("unknown)\n");
			break;
		}
		break;
	case CISCO_MULTICAST:
	case CISCO_UNICAST:
		if (h.address == CISCO_MULTICAST) {
			(*pr)("MULTICAST)\n");
		}
		else {
			(*pr)("UNICAST)\n");
		}
		(*pr)("SPPP: Protocol = %d(", protocol);
		switch (protocol) {
		case CISCO_KEEPALIVE:
			(*pr)("Keepalive)\n");
			break;
		case ETHERTYPE_IP:
			(*pr)("IP)\n");
			return m_examine_ip(m, off, modif, pr);
			break;
		case ETHERTYPE_IPV6:
			(*pr)("IPv6)\n");
			return m_examine_ip6(m, off, modif, pr);
			break;
		default:
			(*pr)("unknown)\n");
			break;
		}
		break;
	default:
		(*pr)("unknown)\n", h.address);
		break;
	}

	(*pr)("No parser for address %d, protocol %d\n", h.address, protocol);
	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_arp(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct arphdr ar;
	uint16_t hrd, op;
	struct in_addr isaddr, itaddr;
	uint8_t esaddr[ETHER_ADDR_LEN], etaddr[ETHER_ADDR_LEN];

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(ar)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(ar), (void *)(&ar)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(ar);

	hrd = ntohs(ar.ar_hrd);
	(*pr)("ARP: AddressType = %u(", hrd);
	switch (hrd) {
	case ARPHRD_ETHER:
		(*pr)("ETHER)\n");
		break;
	case ARPHRD_IEEE802:
		(*pr)("IEEE802)\n");
		break;
	default:
		(*pr)("unknown)\n");
		return m_examine_hex(m, off, modif, pr);
		break;
	}
	(*pr)("ARP: Protocol Address Format = %u\n", ntohs(ar.ar_pro));
	(*pr)("ARP: Protocol Address Length = %u\n", ar.ar_pln);
	(*pr)("ARP: H/W Address Length = %u\n", ar.ar_hln);
	op = ntohs(ar.ar_op);
	(*pr)("ARP: Operation = %u(", op);
	switch (op) {
	case ARPOP_REQUEST:
		(*pr)("REQUEST)\n");
		break;
	case ARPOP_REPLY:
		(*pr)("REPLY)\n");
		break;
	case ARPOP_REVREQUEST:
		(*pr)("REVREQUEST)\n");
		break;
	case ARPOP_REVREPLY:
		(*pr)("REVREPLY)\n");
		break;
	case ARPOP_INVREQUEST:
		(*pr)("INVREQUEST)\n");
		break;
	case ARPOP_INVREPLY:
		(*pr)("INVREPLY)\n");
		break;
	}

	if (ar.ar_hln == 0 || ar.ar_pln == 0 ||
	    ar.ar_hln != sizeof(esaddr) || ar.ar_pln != sizeof(isaddr)) {
		(*pr)("Cannot parse.\n");
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(esaddr), (void *)(esaddr)) < 0) {
		(*pr)("Cannot read payload\n");
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(esaddr);
	(*pr)("ARP: Ether Src = %s\n", str_ethaddr(esaddr));

	if (m_peek_data(m, off, sizeof(isaddr), (void *)(&isaddr)) < 0) {
		(*pr)("Cannot read payload\n");
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(isaddr);
	(*pr)("ARP: IP Src = %s\n", str_ipaddr(&isaddr));

	if (m_peek_data(m, off, sizeof(etaddr), (void *)(etaddr)) < 0) {
		(*pr)("Cannot read payload\n");
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(etaddr);
	(*pr)("ARP: Ether Tgt = %s\n", str_ethaddr(etaddr));

	if (m_peek_data(m, off, sizeof(itaddr), (void *)(&itaddr)) < 0) {
		(*pr)("Cannot read payload\n");
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(itaddr);
	(*pr)("ARP: IP Tgt = %s\n", str_ipaddr(&itaddr));

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_ip(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct ip ip;
	uint16_t offset;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(ip)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(ip), (void *)(&ip)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(ip);

	(*pr)("IP: Version = %u\n", ip.ip_v);
	(*pr)("IP: Header Length = %u\n", (ip.ip_hl << 2));
	(*pr)("IP: ToS = 0x%02x\n", ip.ip_tos);
	(*pr)("IP: Packet Length = %u\n", ntohs(ip.ip_len));
	(*pr)("IP: ID = %u\n", ntohs(ip.ip_id));
	offset = ntohs(ip.ip_off);
	(*pr)("IP: Offset = %u\n", (offset & IP_OFFMASK));
	if (offset & IP_RF) {
		(*pr)("IP: Flag 0x%04x (reserved)\n", IP_RF);
	}
	if (offset & IP_EF) {
		(*pr)("IP: Flag 0x%04x (evil flag)\n", IP_EF);
	}
	if (offset & IP_DF) {
		(*pr)("IP: Flag 0x%04x (don't fragment)\n", IP_DF);
	}
	if (offset & IP_MF) {
		(*pr)("IP: Flag 0x%04x (more fragment)\n", IP_MF);
	}
	(*pr)("IP: TTL = %u\n", ip.ip_ttl);
	(*pr)("IP: protocol = %u(%s)\n", ip.ip_p, str_ipproto(ip.ip_p));
	(*pr)("IP: Src = %s\n", str_ipaddr(&ip.ip_src));
	(*pr)("IP: Dst = %s\n", str_ipaddr(&ip.ip_dst));


	switch (ip.ip_p) {
	case IPPROTO_ICMP:
		return m_examine_icmp(m, off, modif, pr);
		break;
	case IPPROTO_TCP:
		return m_examine_tcp(m, off, modif, pr);
		break;
	case IPPROTO_UDP:
		return m_examine_udp(m, off, modif, pr);
		break;
	default:
		break;
	}


	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_icmp(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct icmp icmphdr;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(icmphdr)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(icmphdr), (void *)(&icmphdr)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(icmphdr);

	(*pr)("ICMP: Type = %u(", icmphdr.icmp_type);
	switch (icmphdr.icmp_type) {
	case ICMP_ECHOREPLY:
		(*pr)("Echo Reply)\n");
		break;
	case ICMP_UNREACH:
		(*pr)("Destination Unreachable)\n");
		break;
	case ICMP_SOURCEQUENCH:
		(*pr)("Source Quench)\n");
		break;
	case ICMP_REDIRECT:
		(*pr)("Redirect)\n");
		break;
	case ICMP_TIMXCEED:
		(*pr)("Time Exceeded)\n");
		break;
	default:
		(*pr)("unknown)\n");
		break;
	}
	(*pr)("ICMP: Code = %d\n", icmphdr.icmp_code);

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_ip6(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct ip6_hdr ip6;
	struct ip6_hbh hbh;
	int hbhlen;
	uint32_t flow;
	uint8_t vfc;
	uint8_t nxt;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(ip6)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(ip6), (void *)(&ip6)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(ip6);

	vfc = ip6.ip6_vfc;
	(*pr)("IPv6: Version = %u\n", (vfc & IPV6_VERSION_MASK) >> 4);
	flow = ntohl(ip6.ip6_flow);
	(*pr)("IPv6: Flow INFO = 0x%07x\n", flow & IPV6_FLOWINFO_MASK);
	(*pr)("IPv6: Payload Length = %u\n", ip6.ip6_plen);
	nxt = ip6.ip6_nxt;
	(*pr)("IPv6: Next Header = %u(%s)\n", nxt, str_ipproto(nxt));
	(*pr)("IPv6: Hop Limit = %u\n", ip6.ip6_hlim);
	(*pr)("IPv6: Src = %s\n", str_ip6addr(&ip6.ip6_src));
	(*pr)("IPv6: Dst = %s\n", str_ip6addr(&ip6.ip6_dst));

	/* Strip Hop-by-Hop options */
	if (nxt == IPPROTO_HOPOPTS) {
		if (m_peek_data(m, off, sizeof(hbh), (void *)(&hbh)) < 0) {
			(*pr)("Cannot read option\n");
			return m_examine_hex(m, off, modif, pr);
		}
		hbhlen = (hbh.ip6h_len + 1) << 3;
		nxt = hbh.ip6h_nxt;
		off += hbhlen;

		(*pr)("IPv6: Stripped Hop-by-Hop\n");
		(*pr)("IPv6: Next Header = %u(%s)\n", nxt, str_ipproto(nxt));
	}

	switch (nxt) {
	case IPPROTO_IPV6_ICMP:
		return m_examine_icmp6(m, off, modif, pr);
		break;
	case IPPROTO_TCP:
		return m_examine_tcp(m, off, modif, pr);
		break;
	case IPPROTO_UDP:
		return m_examine_udp(m, off, modif, pr);
		break;
	default:
		break;
	}

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_icmp6(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct icmp6_hdr icmp6;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(icmp6)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(icmp6), (void *)(&icmp6)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(icmp6);

	(*pr)("ICMP6: Type = %u(", icmp6.icmp6_type);
	switch (icmp6.icmp6_type) {
	case ICMP6_DST_UNREACH:
		(*pr)("Destination Unreachable)\n");
		break;
	case ICMP6_PACKET_TOO_BIG:
		(*pr)("Packet Too Big)\n");
		break;
	case ICMP6_TIME_EXCEEDED:
		(*pr)("Time Exceeded)\n");
		break;
	case ICMP6_PARAM_PROB:
		(*pr)("Parameter Problem)\n");
		break;
	case ICMP6_ECHO_REQUEST:
		(*pr)("Echo Request)\n");
		break;
	case ICMP6_ECHO_REPLY:
		(*pr)("Echo Reply)\n");
		break;

	case MLD_LISTENER_QUERY:
		(*pr)("MLD Listener Query)\n");
		break;
	case MLD_LISTENER_REPORT:
		(*pr)("MLD Listener Report)\n");
		break;
	case MLD_LISTENER_DONE:
		(*pr)("MLD Listener Done)\n");
		break;

	case ND_ROUTER_SOLICIT:
		(*pr)("Router Solicitation)\n");
		break;
	case ND_ROUTER_ADVERT:
		(*pr)("Router Advertizement)\n");
		break;
	case ND_NEIGHBOR_SOLICIT:
		(*pr)("Neighbor Solicitation)\n");
		break;
	case ND_NEIGHBOR_ADVERT:
		(*pr)("Neighbor Advertizement)\n");
		break;
	case ND_REDIRECT:
		(*pr)("Redirect)\n");
		break;

	default:
		(*pr)("unknown)\n");
		break;
	}
	(*pr)("ICMP6: Code = %u\n", icmp6.icmp6_code);

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_tcp(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct tcphdr tcp;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(tcp)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(tcp), (void *)(&tcp)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(tcp);

	(*pr)("TCP: Src = %u\n", ntohs(tcp.th_sport));
	(*pr)("TCP: Dst = %u\n", ntohs(tcp.th_dport));
	(*pr)("TCP: Seq. = %u\n", ntohl(tcp.th_seq));
	(*pr)("TCP: Ack. = %u\n", ntohl(tcp.th_ack));
	(*pr)("TCP: Header Length = %u\n", ntohl(tcp.th_off) << 2);
	if (tcp.th_flags) {
		(*pr)("TCP: Flags 0x%02x : ", tcp.th_flags);
		if (tcp.th_flags & TH_FIN)
			(*pr)("FIN ");
		if (tcp.th_flags & TH_SYN)
			(*pr)("SYN ");
		if (tcp.th_flags & TH_RST)
			(*pr)("RST ");
		if (tcp.th_flags & TH_PUSH)
			(*pr)("PUSH ");
		if (tcp.th_flags & TH_URG)
			(*pr)("URG ");
		if (tcp.th_flags & TH_ECE)
			(*pr)("ECE ");
		if (tcp.th_flags & TH_CWR)
			(*pr)("CWR ");
		(*pr)("\n");
	}
	(*pr)("TCP: Windows Size = %u\n", ntohs(tcp.th_win));
	(*pr)("TCP: Urgent Pointer = %u\n", ntohs(tcp.th_urp));

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_udp(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	struct udphdr udp;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen < sizeof(udp)) {
		(*pr)("%s: too short mbuf chain\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}

	if (m_peek_data(m, off, sizeof(udp), (void *)(&udp)) < 0) {
		(*pr)("%s: cannot read header\n", __func__);
		return m_examine_hex(m, off, modif, pr);
	}
	off += sizeof(udp);

	(*pr)("UDP: Src = %u\n", ntohs(udp.uh_sport));
	(*pr)("UDP: Dst = %u\n", ntohs(udp.uh_dport));
	(*pr)("UDP: Length = %u\n", ntohs(udp.uh_ulen));

	return m_examine_hex(m, off, modif, pr);
}

static void
m_examine_hex(const struct mbuf *m, int off, const char *modif,
    void (*pr)(const char *, ...))
{
	unsigned int pktlen;
	int newline = 0;
	uint8_t v;

	pktlen = m_peek_len(m, modif) - off;
	if (pktlen > EXAMINE_HEX_LIMIT)
		pktlen = EXAMINE_HEX_LIMIT;

	if (pktlen == 0)
		return;

	(*pr)("offset %04d: ", off);
	while (pktlen > 0) {
		if (m_peek_data(m, off, sizeof(v), (void *)(&v)) < 0)
			break;
		pktlen --;
		off++;
		newline++;

		(*pr)("%02x", v);
		if (pktlen == 0)
			break;

		if ((newline % EXAMINE_HEX_COL) == 0) {
			(*pr)("\n");
			(*pr)("offset %04d: ", off);
		}
		else {
			(*pr)(" ");
		}
	}
	(*pr)("\n");
}

void
m_examine(const struct mbuf *m, int af, const char *modif,
    void (*pr)(const char *, ...))
{
	if (m == NULL)
		return;

	if (pr == NULL)
		return;

	switch (af) {
	case AF_UNSPEC:
		return m_examine_hex(m, 0, modif, pr);
		break;
	case AF_ETHER:
		return m_examine_ether(m, 0, modif, pr);
		break;
	case AF_ARP:
		return m_examine_arp(m, 0, modif, pr);
		break;
	case AF_INET:
		return m_examine_ip(m, 0, modif, pr);
		break;
	case AF_INET6:
		return m_examine_ip6(m, 0, modif, pr);
		break;
	default:
		(*pr)("No parser for AF %d\n", af);
		return m_examine_hex(m, 0, modif, pr);
		break;
	}

	/* not reached */
	return;
}
