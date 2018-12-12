/*	$NetBSD: ether_sw_offload.h,v 1.1 2018/12/12 01:40:20 rin Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rin Okuyama.
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

#ifndef _NET_ETHER_SW_OFFLOAD_H_
#define _NET_ETHER_SW_OFFLOAD_H_

#ifdef _KERNEL
#include <sys/mbuf.h>
#include <net/if.h>

/*
 * Whether given TX offload options are supported by the interface.
 * We need to check TSO bits first; M_CSUM_IPv4 is turned on even if
 * TSOv4 is enabled (and some drivers depend on this behavior).
 * Therefore we cannot test
 *	m->m_pkthdr.csum_flags & ~ifp->if_csum_flags_tx
 * for interfaces that do not support hw IPv4 checksum.
 */
#define TX_OFFLOAD_SUPPORTED(if_flags, mbuf_flags)			\
    ((((mbuf_flags) & (M_CSUM_TSOv4 | M_CSUM_TSOv6)) & (if_flags)) ||	\
     !((mbuf_flags) & ~(if_flags)))

struct mbuf *ether_sw_offload_tx(struct ifnet *, struct mbuf *);
struct mbuf *ether_sw_offload_rx(struct ifnet *, struct mbuf *);
#endif /* _KERNEL */

#endif /* !_NET_ETHER_SW_OFFLOAD_H_ */
