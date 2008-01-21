/*	$NetBSD: if_gre.h,v 1.14.4.7 2008/01/21 09:47:03 yamt Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
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

#ifndef _NET_IF_GRE_H_
#define _NET_IF_GRE_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/malloc.h>
#include <sys/mallocvar.h>

#ifdef _KERNEL
struct gre_soparm {
	struct sockaddr_storage sp_src;	/* source of gre packets */
	struct sockaddr_storage sp_dst;	/* destination of gre packets */
	int		sp_type;	/* encapsulating socket type */
	int		sp_proto;	/* encapsulating protocol */
	int		sp_fd;
	int		sp_bysock;	/* encapsulation configured by passing
					 * socket, not by SIOCSLIFPHYADDR
					 */
};

enum gre_state {
	  GRE_S_IDLE = 0
	, GRE_S_IOCTL
	, GRE_S_DIE
};

#define	__cacheline_aligned	__attribute__((__aligned__(CACHE_LINE_SIZE)))

struct gre_bufq {
	volatile int	bq_prodidx;
	volatile int	bq_considx;
	size_t		bq_len __cacheline_aligned;
	size_t		bq_lenmask;
	volatile int	bq_drops;
	struct mbuf	**bq_buf;
};

MALLOC_DECLARE(M_GRE_BUFQ);

struct gre_softc {
	struct ifnet		sc_if;
	kmutex_t		sc_mtx;
	kcondvar_t		sc_condvar;
	struct gre_bufq		sc_snd;
	struct gre_soparm	sc_soparm;
	struct lwp		*sc_lwp;
	volatile enum gre_state	sc_state;
	volatile int		sc_waiters;
	volatile int		sc_upcalls;
	void			*sc_si;
	struct socket		*sc_so;

	struct evcnt		sc_recv_ev;
	struct evcnt		sc_send_ev;

	struct evcnt		sc_block_ev;
	struct evcnt		sc_error_ev;
	struct evcnt		sc_pullup_ev;
	struct evcnt		sc_unsupp_ev;
	struct evcnt		sc_oflow_ev;
};

struct gre_h {
	uint16_t flags;		/* GRE flags */
	uint16_t ptype;		/* protocol type of payload typically
				 * ethernet protocol type
				 */
/*
 *  from here on: fields are optional, presence indicated by flags
 *
	u_int_16 checksum	checksum (one-complements of GRE header
				and payload
				Present if (ck_pres | rt_pres == 1).
				Valid if (ck_pres == 1).
	u_int_16 offset		offset from start of routing filed to
				first octet of active SRE (see below).
				Present if (ck_pres | rt_pres == 1).
				Valid if (rt_pres == 1).
	u_int_32 key		inserted by encapsulator e.g. for
				authentication
				Present if (key_pres ==1 ).
	u_int_32 seq_num	Sequence number to allow for packet order
				Present if (seq_pres ==1 ).
	struct gre_sre[] routing Routing fileds (see below)
				Present if (rt_pres == 1)
 */
} __packed;

#define GRE_CP		0x8000  /* Checksum Present */
#define GRE_RP		0x4000  /* Routing Present */
#define GRE_KP		0x2000  /* Key Present */
#define GRE_SP		0x1000  /* Sequence Present */
#define GRE_SS		0x0800	/* Strict Source Route */

/*
 * gre_sre defines a Source route Entry. These are needed if packets
 * should be routed over more than one tunnel hop by hop
 */
struct gre_sre {
	u_int16_t sre_family;	/* address family */
	u_char	sre_offset;	/* offset to first octet of active entry */
	u_char	sre_length;	/* number of octets in the SRE.
				   sre_lengthl==0 -> last entry. */
	u_char	*sre_rtinfo;	/* the routing information */
};

#define	GRE_TTL	30
extern int ip_gre_ttl;
#endif /* _KERNEL */

/*
 * ioctls needed to manipulate the interface
 */

#define GRESADDRS	_IOW('i', 101, struct ifreq)
#define GRESADDRD	_IOW('i', 102, struct ifreq)
#define GREGADDRS	_IOWR('i', 103, struct ifreq)
#define GREGADDRD	_IOWR('i', 104, struct ifreq)
#define GRESPROTO	_IOW('i' , 105, struct ifreq)
#define GREGPROTO	_IOWR('i', 106, struct ifreq)
#define GRESSOCK	_IOW('i' , 107, struct ifreq)
#define GREDSOCK	_IOW('i' , 108, struct ifreq)

#endif /* !_NET_IF_GRE_H_ */
