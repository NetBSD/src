/*
 * dhcpcd: BPF arp and bootp filtering
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2006-2025 Roy Marples <roy@marples.name>
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

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config.h" // IWYU pragma: keep
#ifdef USE_LIBPCAP
#include <pcap/bpf.h>
#elif defined(__linux__)
#include <linux/filter.h>
#define bpf_insn sock_filter
#else
#include <net/bpf.h>
#endif

#include "arp.h"
#include "bpf.h"
#include "common.h"
#include "dhcp.h"
#include "if.h"
#include "logerr.h"

/* BPF helper macros */
#ifdef __linux__
#define BPF_WHOLEPACKET 0x7fffffff /* work around buggy LPF filters */
#else
#define BPF_WHOLEPACKET ~0U
#endif

/* Macros to update the BPF structure */
#define BPF_SET_STMT(insn, c, v)           \
	{                                  \
		(insn)->code = (c);        \
		(insn)->jt = 0;            \
		(insn)->jf = 0;            \
		(insn)->k = (uint32_t)(v); \
	}

#define BPF_SET_JUMP(insn, c, v, t, f)     \
	{                                  \
		(insn)->code = (c);        \
		(insn)->jt = (t);          \
		(insn)->jf = (f);          \
		(insn)->k = (uint32_t)(v); \
	}

size_t
bpf_frame_header_len(const struct interface *ifp)
{
	switch (ifp->hwtype) {
	case ARPHRD_ETHER:
		return sizeof(struct ether_header);
	default:
		return 0;
	}
}

void *
bpf_frame_header_src(const struct interface *ifp, void *fh, size_t *len)
{
	uint8_t *f = fh;

	switch (ifp->hwtype) {
	case ARPHRD_ETHER:
		*len = sizeof(((struct ether_header *)0)->ether_shost);
		return f + offsetof(struct ether_header, ether_shost);
	default:
		*len = 0;
		errno = ENOTSUP;
		return NULL;
	}
}

void *
bpf_frame_header_dst(const struct interface *ifp, void *fh, size_t *len)
{
	uint8_t *f = fh;

	switch (ifp->hwtype) {
	case ARPHRD_ETHER:
		*len = sizeof(((struct ether_header *)0)->ether_dhost);
		return f + offsetof(struct ether_header, ether_dhost);
	default:
		*len = 0;
		errno = ENOTSUP;
		return NULL;
	}
}

static const uint8_t etherbcastaddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

int
bpf_frame_bcast(const struct interface *ifp, const void *frame)
{
	switch (ifp->hwtype) {
	case ARPHRD_ETHER:
		return memcmp((const char *)frame +
			offsetof(struct ether_header, ether_dhost),
		    etherbcastaddr, sizeof(etherbcastaddr));
	default:
		return -1;
	}
}

ssize_t
bpf_send(const struct bpf *bpf, uint16_t protocol, const void *data, size_t len)
{
	struct iovec iov[2];
	struct ether_header eh;

	switch (bpf->bpf_ifp->hwtype) {
	case ARPHRD_ETHER:
		memset(&eh.ether_dhost, 0xff, sizeof(eh.ether_dhost));
		memcpy(&eh.ether_shost, bpf->bpf_ifp->hwaddr,
		    sizeof(eh.ether_shost));
		eh.ether_type = htons(protocol);
		iov[0].iov_base = &eh;
		iov[0].iov_len = sizeof(eh);
		break;
	default:
		iov[0].iov_base = NULL;
		iov[0].iov_len = 0;
		break;
	}
	iov[1].iov_base = UNCONST(data);
	iov[1].iov_len = len;

	return bpf_writev(bpf, iov, __arraycount(iov));
}

#ifdef ARP
#define BPF_CMP_HWADDR_LEN ((((HWADDR_LEN / 4) + 2) * 2) + 1)
static unsigned int
bpf_cmp_hwaddr(struct bpf_insn *bpf, size_t bpf_len, size_t off, bool equal,
    const uint8_t *hwaddr, size_t hwaddr_len)
{
	struct bpf_insn *bp;
	size_t maclen, nlft, njmps;
	uint32_t mac32;
	uint16_t mac16;
	uint8_t jt, jf;

	/* Calc the number of jumps */
	if ((hwaddr_len / 4) >= 128) {
		errno = EINVAL;
		return 0;
	}
	njmps = (hwaddr_len / 4) * 2; /* 2 instructions per check */
	/* We jump after the 1st check. */
	if (njmps)
		njmps -= 2;
	nlft = hwaddr_len % 4;
	if (nlft) {
		njmps += (nlft / 2) * 2;
		nlft = nlft % 2;
		if (nlft)
			njmps += 2;
	}

	/* Skip to positive finish. */
	njmps++;
	if (equal) {
		jt = (uint8_t)njmps;
		jf = 0;
	} else {
		jt = 0;
		jf = (uint8_t)njmps;
	}

	bp = bpf;
	for (; hwaddr_len > 0;
	    hwaddr += maclen, hwaddr_len -= maclen, off += maclen) {
		if (bpf_len < 3) {
			errno = ENOBUFS;
			return 0;
		}
		bpf_len -= 3;

		if (hwaddr_len >= 4) {
			maclen = sizeof(mac32);
			memcpy(&mac32, hwaddr, maclen);
			BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND, off);
			bp++;
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
			    htonl(mac32), jt, jf);
		} else if (hwaddr_len >= 2) {
			maclen = sizeof(mac16);
			memcpy(&mac16, hwaddr, maclen);
			BPF_SET_STMT(bp, BPF_LD + BPF_H + BPF_IND, off);
			bp++;
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
			    htons(mac16), jt, jf);
		} else {
			maclen = sizeof(*hwaddr);
			BPF_SET_STMT(bp, BPF_LD + BPF_B + BPF_IND, off);
			bp++;
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K, *hwaddr, jt,
			    jf);
		}
		if (jt)
			jt = (uint8_t)(jt - 2);
		if (jf)
			jf = (uint8_t)(jf - 2);
		bp++;
	}

	/* Last step is always return failure.
	 * Next step is a positive finish. */
	BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
	bp++;

	return (unsigned int)(bp - bpf);
}
#endif

#ifdef ARP
static const struct bpf_insn bpf_arp_ether[] = {
	/* Check this is an ARP packet. */
	BPF_STMT(BPF_LD + BPF_H + BPF_ABS,
	    offsetof(struct ether_header, ether_type)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_ARP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Load frame header length into X */
	BPF_STMT(BPF_LDX + BPF_W + BPF_IMM, sizeof(struct ether_header)),

	/* Make sure the hardware type matches. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_hrd)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPHRD_ETHER, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure the hardware length matches. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct arphdr, ar_hln)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K,
	    sizeof(((struct ether_header *)0)->ether_shost), 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define BPF_ARP_ETHER_LEN __arraycount(bpf_arp_ether)

static const struct bpf_insn bpf_arp_filter[] = {
	/* Make sure this is for IP. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_pro)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_IP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
	/* Make sure this is an ARP REQUEST. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_op)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPOP_REQUEST, 2, 0),
	/* or ARP REPLY. */
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPOP_REPLY, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
	/* Make sure the protocol length matches. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct arphdr, ar_pln)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, sizeof(in_addr_t), 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define BPF_ARP_FILTER_LEN __arraycount(bpf_arp_filter)

/* One address is two checks of two statements. */
#define BPF_NADDRS	  1
#define BPF_ARP_ADDRS_LEN 5 + ((BPF_NADDRS * 2) * 2)

#define BPF_ARP_LEN                                                   \
	BPF_ARP_ETHER_LEN + BPF_ARP_FILTER_LEN + BPF_CMP_HWADDR_LEN + \
	    BPF_ARP_ADDRS_LEN

static int
bpf_arp_rw(const struct bpf *bpf, const struct in_addr *ia, bool recv)
{
	const struct interface *ifp = bpf->bpf_ifp;
	struct bpf_insn buf[BPF_ARP_LEN + 1];
	struct bpf_insn *bp;
	uint16_t arp_len;
	unsigned int len;

	bp = buf;
	/* Check frame header. */
	switch (ifp->hwtype) {
	case ARPHRD_ETHER:
		memcpy(bp, bpf_arp_ether, sizeof(bpf_arp_ether));
		bp += BPF_ARP_ETHER_LEN;
		arp_len = sizeof(struct ether_header) + sizeof(struct arphdr) +
		    ((sizeof(((struct ether_header *)0)->ether_shost) +
			 sizeof(uint32_t)) *
			2);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* Copy in the main filter. */
	memcpy(bp, bpf_arp_filter, sizeof(bpf_arp_filter));
	bp += BPF_ARP_FILTER_LEN;

	/* Ensure it's not from us. */
	bp += bpf_cmp_hwaddr(bp, BPF_CMP_HWADDR_LEN, sizeof(struct arphdr),
	    !recv, ifp->hwaddr, ifp->hwlen);

	/* Match sender protocol address */
	BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
	    sizeof(struct arphdr) + ifp->hwlen);
	bp++;
	BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K, htonl(ia->s_addr), 0, 1);
	bp++;
	BPF_SET_STMT(bp, BPF_RET + BPF_K, arp_len);
	bp++;

	/* If we didn't match sender, then we're only interested in
	 * ARP probes to us, so check the null host sender. */
	BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K, INADDR_ANY, 1, 0);
	bp++;
	BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
	bp++;

	/* Match target protocol address */
	BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
	    (sizeof(struct arphdr) + (size_t)(ifp->hwlen * 2) +
		sizeof(in_addr_t)));
	bp++;
	BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K, htonl(ia->s_addr), 0, 1);
	bp++;
	BPF_SET_STMT(bp, BPF_RET + BPF_K, arp_len);
	bp++;

	/* No match, drop it */
	BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
	bp++;

	len = (unsigned int)(bp - buf);
	if (recv)
		return bpf_setfilter(bpf, buf, len);
	return bpf_setwfilter(bpf, buf, len);
}

int
bpf_filter_arp(const struct bpf *bpf, const struct in_addr *ia)
{
	if (bpf_arp_rw(bpf, ia, true) == -1)
		return -1;
	if (bpf_arp_rw(bpf, ia, false) == -1 && errno != ENOSYS)
		return -1;
	if (bpf_lockfilter(bpf) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}
#endif

#ifdef ARPHRD_NONE
static const struct bpf_insn bpf_bootp_none[] = {};
#define BPF_BOOTP_NONE_LEN __arraycount(bpf_bootp_none)
#endif

static const struct bpf_insn bpf_bootp_ether[] = {
	/* Make sure this is an IP packet. */
	BPF_STMT(BPF_LD + BPF_H + BPF_ABS,
	    offsetof(struct ether_header, ether_type)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_IP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Advance to the IP header. */
	BPF_STMT(BPF_LDX + BPF_K, sizeof(struct ether_header)),
};
#define BPF_BOOTP_ETHER_LEN __arraycount(bpf_bootp_ether)

static const struct bpf_insn bpf_bootp_base[] = {
	/* Make sure it's an IPv4 packet. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, 0),
	BPF_STMT(BPF_ALU + BPF_AND + BPF_K, 0xf0),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, 0x40, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure it's a UDP packet. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct ip, ip_p)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, IPPROTO_UDP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure this isn't a fragment. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct ip, ip_off)),
	BPF_JUMP(BPF_JMP + BPF_JSET + BPF_K, 0x1fff, 0, 1),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Advance to the UDP header. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, 0),
	BPF_STMT(BPF_ALU + BPF_AND + BPF_K, 0x0f),
	BPF_STMT(BPF_ALU + BPF_MUL + BPF_K, 4),
	BPF_STMT(BPF_ALU + BPF_ADD + BPF_X, 0),
	BPF_STMT(BPF_MISC + BPF_TAX, 0),
};
#define BPF_BOOTP_BASE_LEN __arraycount(bpf_bootp_base)

static const struct bpf_insn bpf_bootp_read[] = {
	/* Make sure it's to the right port.
	 * RFC2131 makes no mention of enforcing a source port. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct udphdr, uh_dport)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, BOOTPC, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define BPF_BOOTP_READ_LEN __arraycount(bpf_bootp_read)

static const struct bpf_insn bpf_bootp_write[] = {
	/* Make sure it's from and to the right port.
	 * RFC2131 makes no mention of encforcing a source port,
	 * but dhcpcd does enforce it for sending. */
	BPF_STMT(BPF_LD + BPF_W + BPF_IND, 0),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, (BOOTPC << 16) + BOOTPS, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define BPF_BOOTP_WRITE_LEN  __arraycount(bpf_bootp_write)

#define BPF_BOOTP_CHADDR_LEN ((BOOTP_CHADDR_LEN / 4) * 3)
#define BPF_BOOTP_XID_LEN    4 /* BOUND check is 4 instructions */

#define BPF_BOOTP_LEN                                                   \
	BPF_BOOTP_ETHER_LEN + BPF_BOOTP_BASE_LEN + BPF_BOOTP_READ_LEN + \
	    BPF_BOOTP_XID_LEN + BPF_BOOTP_CHADDR_LEN + 4

static int
bpf_bootp_rw(const struct bpf *bpf, bool read)
{
	struct bpf_insn buf[BPF_BOOTP_LEN + 1];
	struct bpf_insn *bp;

	bp = buf;
	/* Check frame header. */
	switch (bpf->bpf_ifp->hwtype) {
#ifdef ARPHRD_NONE
	case ARPHRD_NONE:
		memcpy(bp, bpf_bootp_none, sizeof(bpf_bootp_none));
		bp += BPF_BOOTP_NONE_LEN;
		break;
#endif
	case ARPHRD_ETHER:
		memcpy(bp, bpf_bootp_ether, sizeof(bpf_bootp_ether));
		bp += BPF_BOOTP_ETHER_LEN;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* Copy in the main filter. */
	memcpy(bp, bpf_bootp_base, sizeof(bpf_bootp_base));
	bp += BPF_BOOTP_BASE_LEN;

	if (!read) {
		memcpy(bp, bpf_bootp_write, sizeof(bpf_bootp_write));
		bp += BPF_BOOTP_WRITE_LEN;

		/* All passed, return the packet. */
		BPF_SET_STMT(bp, BPF_RET + BPF_K, BPF_WHOLEPACKET);
		bp++;

		return bpf_setwfilter(bpf, buf, (unsigned int)(bp - buf));
	}

	memcpy(bp, bpf_bootp_read, sizeof(bpf_bootp_read));
	bp += BPF_BOOTP_READ_LEN;

	/* All passed, return the packet. */
	BPF_SET_STMT(bp, BPF_RET + BPF_K, BPF_WHOLEPACKET);
	bp++;

	return bpf_setfilter(bpf, buf, (unsigned int)(bp - buf));
}

int
bpf_filter_bootp(const struct bpf *bpf, __unused const struct in_addr *ia)
{
	if (bpf_bootp_rw(bpf, true) == -1)
		return -1;
	if (bpf_bootp_rw(bpf, false) == -1 && errno != ENOSYS)
		return -1;
	if (bpf_lockfilter(bpf) == -1 && errno != ENOSYS)
		return -1;
	return 0;
}
