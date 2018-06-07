/*
 * dhcpcd: BPF arp and bootp filtering
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

#include <arpa/inet.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#ifdef __linux__
/* Special BPF snowflake. */
#include <linux/filter.h>
#define	bpf_insn		sock_filter
#else
#include <net/bpf.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "arp.h"
#include "bpf.h"
#include "dhcp.h"
#include "if.h"
#include "logerr.h"

#define	ARP_ADDRS_MAX	3

/* BPF helper macros */
#ifdef __linux__
#define	BPF_WHOLEPACKET		0x7fffffff /* work around buggy LPF filters */
#else
#define	BPF_WHOLEPACKET		~0U
#endif

/* Macros to update the BPF structure */
#define	BPF_SET_STMT(insn, c, v) {				\
	(insn)->code = (c);					\
	(insn)->jt = 0;						\
	(insn)->jf = 0;						\
	(insn)->k = (uint32_t)(v);				\
};

#define	BPF_SET_JUMP(insn, c, v, t, f) {			\
	(insn)->code = (c);					\
	(insn)->jt = (t);					\
	(insn)->jf = (f);					\
	(insn)->k = (uint32_t)(v);				\
};

size_t
bpf_frame_header_len(const struct interface *ifp)
{

	switch(ifp->family) {
	case ARPHRD_ETHER:
		return sizeof(struct ether_header);
	default:
		return 0;
	}
}

#ifndef __linux__
/* Linux is a special snowflake for opening, attaching and reading BPF.
 * See if-linux.c for the Linux specific BPF functions. */

const char *bpf_name = "Berkley Packet Filter";

int
bpf_open(struct interface *ifp, int (*filter)(struct interface *, int))
{
	struct ipv4_state *state;
	int fd = -1;
	struct ifreq ifr;
	int ibuf_len = 0;
	size_t buf_len;
	struct bpf_version pv;
#ifdef BIOCIMMEDIATE
	unsigned int flags;
#endif
#ifndef O_CLOEXEC
	int fd_opts;
#endif

#ifdef _PATH_BPF
	fd = open(_PATH_BPF, O_RDWR | O_NONBLOCK
#ifdef O_CLOEXEC
		| O_CLOEXEC
#endif
	);
#else
	char device[32];
	int n = 0;

	do {
		snprintf(device, sizeof(device), "/dev/bpf%d", n++);
		fd = open(device, O_RDWR | O_NONBLOCK
#ifdef O_CLOEXEC
				| O_CLOEXEC
#endif
		);
	} while (fd == -1 && errno == EBUSY);
#endif

	if (fd == -1)
		return -1;

#ifndef O_CLOEXEC
	if ((fd_opts = fcntl(fd, F_GETFD)) == -1 ||
	    fcntl(fd, F_SETFD, fd_opts | FD_CLOEXEC) == -1) {
		close(fd);
		return -1;
	}
#endif

	memset(&pv, 0, sizeof(pv));
	if (ioctl(fd, BIOCVERSION, &pv) == -1)
		goto eexit;
	if (pv.bv_major != BPF_MAJOR_VERSION ||
	    pv.bv_minor < BPF_MINOR_VERSION) {
		logerrx("BPF version mismatch - recompile");
		goto eexit;
	}

	if (filter(ifp, fd) != 0)
		goto eexit;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(fd, BIOCSETIF, &ifr) == -1)
		goto eexit;

	/* Get the required BPF buffer length from the kernel. */
	if (ioctl(fd, BIOCGBLEN, &ibuf_len) == -1)
		goto eexit;
	buf_len = (size_t)ibuf_len;
	state = ipv4_getstate(ifp);
	if (state == NULL)
		goto eexit;
	if (state->buffer_size != buf_len) {
		void *nb;

		if ((nb = realloc(state->buffer, buf_len)) == NULL)
			goto eexit;
		state->buffer = nb;
		state->buffer_size = buf_len;
	}

#ifdef BIOCIMMEDIATE
	flags = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &flags) == -1)
		goto eexit;
#endif

	return fd;

eexit:
	close(fd);
	return -1;
}

/* BPF requires that we read the entire buffer.
 * So we pass the buffer in the API so we can loop on >1 packet. */
ssize_t
bpf_read(struct interface *ifp, int fd, void *data, size_t len,
    unsigned int *flags)
{
	ssize_t fl = (ssize_t)bpf_frame_header_len(ifp);
	ssize_t bytes;
	struct ipv4_state *state = IPV4_STATE(ifp);

	struct bpf_hdr packet;
	const char *payload;

	*flags &= ~BPF_EOF;
	for (;;) {
		if (state->buffer_len == 0) {
			bytes = read(fd, state->buffer, state->buffer_size);
#if defined(__sun)
			/* After 2^31 bytes, the kernel offset overflows.
			 * To work around this bug, lseek 0. */
			if (bytes == -1 && errno == EINVAL) {
				lseek(fd, 0, SEEK_SET);
				continue;
			}
#endif
			if (bytes == -1 || bytes == 0)
				return bytes;
			state->buffer_len = (size_t)bytes;
			state->buffer_pos = 0;
		}
		bytes = -1;
		memcpy(&packet, state->buffer + state->buffer_pos,
		    sizeof(packet));
		if (state->buffer_pos + packet.bh_caplen + packet.bh_hdrlen >
		    state->buffer_len)
			goto next; /* Packet beyond buffer, drop. */
		payload = state->buffer + state->buffer_pos +
		    packet.bh_hdrlen + fl;
		bytes = (ssize_t)packet.bh_caplen - fl;
		if ((size_t)bytes > len)
			bytes = (ssize_t)len;
		memcpy(data, payload, (size_t)bytes);
next:
		state->buffer_pos += BPF_WORDALIGN(packet.bh_hdrlen +
		    packet.bh_caplen);
		if (state->buffer_pos >= state->buffer_len) {
			state->buffer_len = state->buffer_pos = 0;
			*flags |= BPF_EOF;
		}
		if (bytes != -1)
			return bytes;
	}

	/* NOTREACHED */
}

int
bpf_attach(int fd, void *filter, unsigned int filter_len)
{
	struct bpf_program pf;

	/* Install the filter. */
	memset(&pf, 0, sizeof(pf));
	pf.bf_insns = filter;
	pf.bf_len = filter_len;
	return ioctl(fd, BIOCSETF, &pf);
}
#endif

#ifndef __sun
/* SunOS is special too - sending via BPF goes nowhere. */
ssize_t
bpf_send(const struct interface *ifp, int fd, uint16_t protocol,
    const void *data, size_t len)
{
	struct iovec iov[2];
	struct ether_header eh;

	switch(ifp->family) {
	case ARPHRD_ETHER:
		memset(&eh.ether_dhost, 0xff, sizeof(eh.ether_dhost));
		memcpy(&eh.ether_shost, ifp->hwaddr, sizeof(eh.ether_shost));
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
	return writev(fd, iov, 2);
}
#endif

int
bpf_close(struct interface *ifp, int fd)
{
	struct ipv4_state *state = IPV4_STATE(ifp);

	/* Rewind the buffer on closing. */
	state->buffer_len = state->buffer_pos = 0;
	return close(fd);
}

/* Normally this is needed by bootp.
 * Once that uses this again, the ARP guard here can be removed. */
#ifdef ARP
static unsigned int
bpf_cmp_hwaddr(struct bpf_insn *bpf, size_t bpf_len, size_t off,
    bool equal, uint8_t *hwaddr, size_t hwaddr_len)
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
	     hwaddr += maclen, hwaddr_len -= maclen, off += maclen)
	{
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
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
			             *hwaddr, jt, jf);
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

static const struct bpf_insn bpf_arp_ether [] = {
	/* Ensure packet is at least correct size. */
	BPF_STMT(BPF_LD + BPF_W + BPF_LEN, 0),
	BPF_JUMP(BPF_JMP + BPF_JGE + BPF_K, sizeof(struct ether_arp), 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Check this is an ARP packet. */
	BPF_STMT(BPF_LD + BPF_H + BPF_ABS,
	         offsetof(struct ether_header, ether_type)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_ARP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Load frame header length into X */
	BPF_STMT(BPF_LDX + BPF_W + BPF_IMM, sizeof(struct ether_header)),

	/* Make sure the hardware family matches. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_hrd)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPHRD_ETHER, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure the hardware length matches. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct arphdr, ar_hln)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K,
	         sizeof(((struct ether_arp *)0)->arp_sha), 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define bpf_arp_ether_len	__arraycount(bpf_arp_ether)

static const struct bpf_insn bpf_arp_filter [] = {
	/* Make sure this is for IP. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_pro)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_IP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
	/* Make sure this is an ARP REQUEST. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct arphdr, ar_op)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPOP_REQUEST, 2, 0),
	/* or ARP REPLY. */
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARPOP_REPLY, 1, 1),
	BPF_STMT(BPF_RET + BPF_K, 0),
	/* Make sure the protocol length matches. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct arphdr, ar_pln)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, sizeof(in_addr_t), 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};
#define bpf_arp_filter_len	__arraycount(bpf_arp_filter)
#define bpf_arp_extra		((((ARP_ADDRS_MAX + 1) * 2) * 2) + 2)
#define bpf_arp_hw		((((HWADDR_LEN / 4) + 2) * 2) + 1)

int
bpf_arp(struct interface *ifp, int fd)
{
	struct bpf_insn bpf[3+ bpf_arp_filter_len + bpf_arp_hw + bpf_arp_extra];
	struct bpf_insn *bp;
	struct iarp_state *state;
	uint16_t arp_len;

	if (fd == -1)
		return 0;

	bp = bpf;
	/* Check frame header. */
	switch(ifp->family) {
	case ARPHRD_ETHER:
		memcpy(bp, bpf_arp_ether, sizeof(bpf_arp_ether));
		bp += bpf_arp_ether_len;
		arp_len = sizeof(struct ether_header)+sizeof(struct ether_arp);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* Copy in the main filter. */
	memcpy(bp, bpf_arp_filter, sizeof(bpf_arp_filter));
	bp += bpf_arp_filter_len;

	/* Ensure it's not from us. */
	bp += bpf_cmp_hwaddr(bp, bpf_arp_hw, sizeof(struct arphdr),
	                     false, ifp->hwaddr, ifp->hwlen);

	state = ARP_STATE(ifp);
	if (TAILQ_FIRST(&state->arp_states)) {
		struct arp_state *astate;
		size_t naddrs;

		/* Match sender protocol address */
		BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
		             sizeof(struct arphdr) + ifp->hwlen);
		bp++;
		naddrs = 0;
		TAILQ_FOREACH(astate, &state->arp_states, next) {
			if (++naddrs > ARP_ADDRS_MAX) {
				errno = ENOBUFS;
				logerr(__func__);
				break;
			}
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
			             htonl(astate->addr.s_addr), 0, 1);
			bp++;
			BPF_SET_STMT(bp, BPF_RET + BPF_K, arp_len);
			bp++;
		}

		/* If we didn't match sender, then we're only interested in
		 * ARP probes to us, so check the null host sender. */
		BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K, INADDR_ANY, 1, 0);
		bp++;
		BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
		bp++;

		/* Match target protocol address */
		BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
		             (sizeof(struct arphdr)
			     + (size_t)(ifp->hwlen * 2) + sizeof(in_addr_t)));
		bp++;
		naddrs = 0;
		TAILQ_FOREACH(astate, &state->arp_states, next) {
			if (++naddrs > ARP_ADDRS_MAX) {
				/* Already logged error above. */
				break;
			}
			BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
			             htonl(astate->addr.s_addr), 0, 1);
			bp++;
			BPF_SET_STMT(bp, BPF_RET + BPF_K, arp_len);
			bp++;
		}

		/* Return nothing, no protocol address match. */
		BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
		bp++;
	}

	return bpf_attach(fd, bpf, (unsigned int)(bp - bpf));
}
#endif

static const struct bpf_insn bpf_bootp_ether[] = {
	/* Make sure this is an IP packet. */
	BPF_STMT(BPF_LD + BPF_H + BPF_ABS,
	         offsetof(struct ether_header, ether_type)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_IP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Load frame header length into X. */
	BPF_STMT(BPF_LDX + BPF_W + BPF_IMM, sizeof(struct ether_header)),
	/* Copy to M0. */
	BPF_STMT(BPF_STX, 0),
};
#define BPF_BOOTP_ETHER_LEN	__arraycount(bpf_bootp_ether)

static const struct bpf_insn bpf_bootp_filter[] = {
	/* Make sure it's an optionless IPv4 packet. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, 0),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, 0x45, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure it's a UDP packet. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct ip, ip_p)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, IPPROTO_UDP, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Make sure this isn't a fragment. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct ip, ip_off)),
	BPF_JUMP(BPF_JMP + BPF_JSET + BPF_K, 0x1fff, 0, 1),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Store IP location in M1. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct ip, ip_len)),
	BPF_STMT(BPF_ST, 1),

	/* Store IP length in M2. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct ip, ip_len)),
	BPF_STMT(BPF_ST, 2),

	/* Advance to the UDP header. */
	BPF_STMT(BPF_MISC + BPF_TXA, 0),
	BPF_STMT(BPF_ALU + BPF_ADD + BPF_K, sizeof(struct ip)),
	BPF_STMT(BPF_MISC + BPF_TAX, 0),

	/* Store X in M3. */
	BPF_STMT(BPF_STX, 3),

	/* Make sure it's from and to the right port. */
	BPF_STMT(BPF_LD + BPF_W + BPF_IND, 0),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, (BOOTPS << 16) + BOOTPC, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Store UDP length in X. */
	BPF_STMT(BPF_LD + BPF_H + BPF_IND, offsetof(struct udphdr, uh_ulen)),
	BPF_STMT(BPF_MISC + BPF_TAX, 0),
	/* Copy IP length in M2 to A. */
	BPF_STMT(BPF_LD + BPF_MEM, 2),
	/* Ensure IP length - IP header size == UDP length. */
	BPF_STMT(BPF_ALU + BPF_SUB + BPF_K, sizeof(struct ip)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_X, 0, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),

	/* Advance to the BOOTP packet (UDP X is in M3). */
	BPF_STMT(BPF_LD + BPF_MEM, 3),
	BPF_STMT(BPF_ALU + BPF_ADD + BPF_K, sizeof(struct udphdr)),
	BPF_STMT(BPF_MISC + BPF_TAX, 0),

	/* Make sure it's BOOTREPLY. */
	BPF_STMT(BPF_LD + BPF_B + BPF_IND, offsetof(struct bootp, op)),
	BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, BOOTREPLY, 1, 0),
	BPF_STMT(BPF_RET + BPF_K, 0),
};

#define BPF_BOOTP_FILTER_LEN	__arraycount(bpf_bootp_filter)
#define BPF_BOOTP_CHADDR_LEN	((BOOTP_CHADDR_LEN / 4) * 3)
#define	BPF_BOOTP_XID_LEN	4 /* BOUND check is 4 instructions */

#define BPF_BOOTP_LEN		BPF_BOOTP_ETHER_LEN + BPF_BOOTP_FILTER_LEN \
				+ BPF_BOOTP_XID_LEN + BPF_BOOTP_CHADDR_LEN + 4

int
bpf_bootp(struct interface *ifp, int fd)
{
#if 0
	const struct dhcp_state *state = D_CSTATE(ifp);
#endif
	struct bpf_insn bpf[BPF_BOOTP_LEN];
	struct bpf_insn *bp;

	if (fd == -1)
		return 0;

	bp = bpf;
	/* Check frame header. */
	switch(ifp->family) {
	case ARPHRD_ETHER:
		memcpy(bp, bpf_bootp_ether, sizeof(bpf_bootp_ether));
		bp += BPF_BOOTP_ETHER_LEN;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* Copy in the main filter. */
	memcpy(bp, bpf_bootp_filter, sizeof(bpf_bootp_filter));
	bp += BPF_BOOTP_FILTER_LEN;

	/* These checks won't work when same IP exists on other interfaces. */
#if 0
	if (ifp->hwlen <= sizeof(((struct bootp *)0)->chaddr))
		bp += bpf_cmp_hwaddr(bp, BPF_BOOTP_CHADDR_LEN,
		                     offsetof(struct bootp, chaddr),
				     true, ifp->hwaddr, ifp->hwlen);

	/* Make sure the BOOTP packet is for us. */
	if (state->state == DHS_BOUND) {
		/* If bound, we only expect FORCERENEW messages
		 * and they need to be unicast to us.
		 * Move back to the IP header in M0 and check dst. */
		BPF_SET_STMT(bp, BPF_LDX + BPF_W + BPF_MEM, 0);
		bp++;
		BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
		             offsetof(struct ip, ip_dst));
		bp++;
		BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
		             htonl(state->lease.addr.s_addr), 1, 0);
		bp++;
		BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
		bp++;
	} else {
		/* As we're not bound, we need to check xid to ensure
		 * it's a reply to our transaction. */
		BPF_SET_STMT(bp, BPF_LD + BPF_W + BPF_IND,
		             offsetof(struct bootp, xid));
		bp++;
		BPF_SET_JUMP(bp, BPF_JMP + BPF_JEQ + BPF_K,
		             state->xid, 1, 0);
		bp++;
		BPF_SET_STMT(bp, BPF_RET + BPF_K, 0);
		bp++;
	}
#endif

	/* All passed, return the packet
	 * (Frame length in M0, IP length in M2). */
	BPF_SET_STMT(bp, BPF_LD + BPF_MEM, 0);
	bp++;
	BPF_SET_STMT(bp, BPF_LDX + BPF_MEM, 2);
	bp++;
	BPF_SET_STMT(bp, BPF_ALU + BPF_ADD + BPF_X, 0);
	bp++;
	BPF_SET_STMT(bp, BPF_RET + BPF_A, 0);
	bp++;

	return bpf_attach(fd, bpf, (unsigned int)(bp - bpf));
}
