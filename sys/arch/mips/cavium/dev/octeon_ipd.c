/*	$NetBSD: octeon_ipd.c,v 1.8 2021/01/04 17:22:59 thorpej Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_ipd.c,v 1.8 2021/01/04 17:22:59 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_fpareg.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_ipdreg.h>
#include <mips/cavium/dev/octeon_ipdvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define IP_OFFSET(data, word2) \
	((uintptr_t)(data) + (uintptr_t)__SHIFTOUT(word2, PIP_WQE_WORD2_IP_OFFSET))

/* XXX */
void
octipd_init(struct octipd_attach_args *aa, struct octipd_softc **rsc)
{
	struct octipd_softc *sc;
	int status;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_port = aa->aa_port;
	sc->sc_regt = aa->aa_regt;
	sc->sc_first_mbuff_skip = aa->aa_first_mbuff_skip;
	sc->sc_not_first_mbuff_skip = aa->aa_not_first_mbuff_skip;

	status = bus_space_map(sc->sc_regt, IPD_BASE, IPD_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "ipd register");

	*rsc = sc;
}

#define	_IPD_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_IPD_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

int
octipd_enable(struct octipd_softc *sc)
{
	uint64_t ctl_status;

	ctl_status = _IPD_RD8(sc, IPD_CTL_STATUS_OFFSET);
	SET(ctl_status, IPD_CTL_STATUS_IPD_EN);
	_IPD_WR8(sc, IPD_CTL_STATUS_OFFSET, ctl_status);

	return 0;
}

int
octipd_config(struct octipd_softc *sc)
{
	uint64_t first_mbuff_skip;
	uint64_t not_first_mbuff_skip;
	uint64_t packet_mbuff_size;
	uint64_t first_next_ptr_back;
	uint64_t second_next_ptr_back;
	uint64_t sqe_fpa_queue;
	uint64_t ctl_status;

	/* XXX XXX XXX */
	first_mbuff_skip = 0;
	SET(first_mbuff_skip, (sc->sc_first_mbuff_skip / 8) & IPD_1ST_MBUFF_SKIP_SZ);
	_IPD_WR8(sc, IPD_1ST_MBUFF_SKIP_OFFSET, first_mbuff_skip);
	/* XXX XXX XXX */

	/* XXX XXX XXX */
	not_first_mbuff_skip = 0;
	SET(not_first_mbuff_skip, (sc->sc_not_first_mbuff_skip / 8) &
	    IPD_NOT_1ST_MBUFF_SKIP_SZ);
	_IPD_WR8(sc, IPD_NOT_1ST_MBUFF_SKIP_OFFSET, not_first_mbuff_skip);
	/* XXX XXX XXX */

	packet_mbuff_size = 0;
	SET(packet_mbuff_size, (FPA_RECV_PKT_POOL_SIZE / 8) &
	    IPD_PACKET_MBUFF_SIZE_MB_SIZE);
	_IPD_WR8(sc, IPD_PACKET_MBUFF_SIZE_OFFSET, packet_mbuff_size);

	first_next_ptr_back = 0;
	SET(first_next_ptr_back, (sc->sc_first_mbuff_skip / 128) & IPD_1ST_NEXT_PTR_BACK_BACK);
	_IPD_WR8(sc, IPD_1ST_NEXT_PTR_BACK_OFFSET, first_next_ptr_back);

	second_next_ptr_back = 0;
	SET(second_next_ptr_back, (sc->sc_not_first_mbuff_skip / 128) &
	    IPD_2ND_NEXT_PTR_BACK_BACK);
	_IPD_WR8(sc, IPD_2ND_NEXT_PTR_BACK_OFFSET, second_next_ptr_back);

	sqe_fpa_queue = 0;
	SET(sqe_fpa_queue, FPA_WQE_POOL & IPD_WQE_FPA_QUEUE_WQE_QUE);
	_IPD_WR8(sc, IPD_WQE_FPA_QUEUE_OFFSET, sqe_fpa_queue);

	ctl_status = _IPD_RD8(sc, IPD_CTL_STATUS_OFFSET);
	CLR(ctl_status, IPD_CTL_STATUS_OPC_MODE);
	SET(ctl_status,
	    __SHIFTIN(IPD_CTL_STATUS_OPC_MODE_ALL, IPD_CTL_STATUS_OPC_MODE));
	SET(ctl_status, IPD_CTL_STATUS_PBP_EN);

	/*
	* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 
	*          from SDK                 
	* SET(ctl_status, IPD_CTL_STATUS_LEN_M8);
        * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 
	*/

	_IPD_WR8(sc, IPD_CTL_STATUS_OFFSET, ctl_status);

	return 0;
}

/*
 * octeon work queue entry offload
 * L3 error & L4 error
 */
void
octipd_offload(uint64_t word2, void *data, int *rcflags)
{
	int cflags;

	if (ISSET(word2, PIP_WQE_WORD2_IP_NI))
		return;

	cflags = 0;

	if (!ISSET(word2, PIP_WQE_WORD2_IP_V6))
		SET(cflags, M_CSUM_IPv4);

	if (ISSET(word2, PIP_WQE_WORD2_IP_TU)) {
		SET(cflags,
		    !ISSET(word2, PIP_WQE_WORD2_IP_V6) ?
		    (M_CSUM_TCPv4 | M_CSUM_UDPv4) : 
		    (M_CSUM_TCPv6 | M_CSUM_UDPv6));
	}

	/* check L3 (IP) error */
	if (ISSET(word2, PIP_WQE_WORD2_IP_IE)) {
		struct ip *ip;

		switch (word2 & PIP_WQE_WORD2_IP_OPECODE) {
		case IPD_WQE_L3_V4_CSUM_ERR:
			/* CN31XX Pass 1.1 Errata */
			ip = (struct ip *)(IP_OFFSET(data, word2));
			if (ip->ip_hl == 5)
				SET(cflags, M_CSUM_IPv4_BAD);
			break;
		default:
			break;
		}
	}

	/* check L4 (UDP / TCP) error */
	if (ISSET(word2, PIP_WQE_WORD2_IP_LE)) {
		switch (word2 & PIP_WQE_WORD2_IP_OPECODE) {
		case IPD_WQE_L4_CSUM_ERR:
			SET(cflags, M_CSUM_TCP_UDP_BAD);
			break;
		default:
			break;
		}
	}

	*rcflags = cflags;
}

void
octipd_sub_port_fcs(struct octipd_softc *sc, int enable)
{
	uint64_t sub_port_fcs;

	sub_port_fcs = _IPD_RD8(sc, IPD_SUB_PORT_FCS_OFFSET);
	if (enable == 0)
		CLR(sub_port_fcs, __BIT(sc->sc_port));
	else
		SET(sub_port_fcs, __BIT(sc->sc_port));
	_IPD_WR8(sc, IPD_SUB_PORT_FCS_OFFSET, sub_port_fcs);
}
