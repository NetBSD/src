/*	$NetBSD: octeon_ipd.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_ipd.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "opt_octeon.h"

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_ipdreg.h>
#include <mips/cavium/dev/octeon_ipdvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define IP_OFFSET(data, word2) \
	((uintptr_t)(data) + (uintptr_t)((word2 & PIP_WQE_WORD2_IP_OFFSET) >> PIP_WQE_WORD2_IP_OFFSET_SHIFT))

#ifdef OCTEON_ETH_DEBUG
void			octeon_ipd_intr_evcnt_attach(struct octeon_ipd_softc *);
void			octeon_ipd_intr_rml(void *);
int			octeon_ipd_intr_drop(void *);

void			octeon_ipd_dump(void);

static void		*octeon_ipd_intr_drop_ih;
struct evcnt		octeon_ipd_intr_drop_evcnt =
			    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "octeon",
			    "ipd drop intr");
EVCNT_ATTACH_STATIC(octeon_ipd_intr_drop_evcnt);

struct octeon_ipd_softc	*__octeon_ipd_softc[3/* XXX */];
#endif

/* XXX */
void
octeon_ipd_init(struct octeon_ipd_attach_args *aa,
    struct octeon_ipd_softc **rsc)
{
	struct octeon_ipd_softc *sc;
	int status;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK | M_ZERO);
	if (sc == NULL)
		panic("can't allocate memory: %s", __func__);

	sc->sc_port = aa->aa_port;
	sc->sc_regt = aa->aa_regt;
	sc->sc_first_mbuff_skip = aa->aa_first_mbuff_skip;
	sc->sc_not_first_mbuff_skip = aa->aa_not_first_mbuff_skip;

	status = bus_space_map(sc->sc_regt, IPD_BASE, IPD_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "ipd register");

	*rsc = sc;

#ifdef OCTEON_ETH_DEBUG
	octeon_ipd_int_enable(sc, 1);
	octeon_ipd_intr_evcnt_attach(sc);
	if (octeon_ipd_intr_drop_ih == NULL)
		octeon_ipd_intr_drop_ih = octeon_intr_establish(
		   ffs64(CIU_INTX_SUM0_IPD_DRP) - 1, IPL_NET,
		   octeon_ipd_intr_drop, NULL);
	__octeon_ipd_softc[sc->sc_port] = sc;
#endif /* OCTEON_ETH_DEBUG */
}

#define	_IPD_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_IPD_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

int
octeon_ipd_enable(struct octeon_ipd_softc *sc)
{
	uint64_t ctl_status;

	ctl_status = _IPD_RD8(sc, IPD_CTL_STATUS_OFFSET);
	SET(ctl_status, IPD_CTL_STATUS_IPD_EN);
	_IPD_WR8(sc, IPD_CTL_STATUS_OFFSET, ctl_status);

	return 0;
}

int
octeon_ipd_config(struct octeon_ipd_softc *sc)
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
	SET(ctl_status, IPD_CTL_STATUS_OPC_MODE_ALL);
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
octeon_ipd_offload(uint64_t word2, void *data, int *rcflags)
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

int
octeon_ipd_red(struct octeon_ipd_softc *sc, uint64_t pass_thr, uint64_t drop_thr)
{
#if defined(OCTEON_ETH_IPD_RED)
	/*
	 * no receive problem workaround.
	 * if not set IPD RED pramaters,
	 * may become unable to receive packet
	 * on media mismatch environment
	 * of self media 100-half duplex.
	 */
	uint64_t red_marks;
	uint64_t red_param;
	uint64_t red_port;

        red_marks = drop_thr << 32 /* XXX */ | pass_thr;
        _IPD_WR8(sc, IPD_QOS0_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS1_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS2_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS3_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS4_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS5_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS6_RED_MARKS_OFFSET, red_marks);
        _IPD_WR8(sc, IPD_QOS7_RED_MARKS_OFFSET, red_marks);
        red_param = 
            ((255ull << 24 /* XXX */) / (pass_thr - drop_thr)) |
            1ull << 32 /* XXX */ |
            255ull << 40 /* XXX */ |
            1ull << 48 /* XXX */;
        _IPD_WR8(sc, IPD_RED_QUE0_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE1_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE2_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE3_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE4_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE5_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE6_PARAM_OFFSET, red_param);
        _IPD_WR8(sc, IPD_RED_QUE7_PARAM_OFFSET, red_param);

        _IPD_WR8(sc, IPD_BP_PRT_RED_END_OFFSET, 0);

        red_port = 0xfffffffffull |
            10000ull << 36 /* XXX */ |
            10000ull << 50 /* XXX */;
        _IPD_WR8(sc, IPD_RED_PORT_ENABLE_OFFSET, red_port);
#endif

	return 0;
}

void
octeon_ipd_sub_port_fcs(struct octeon_ipd_softc *sc, int enable)
{
	uint64_t sub_port_fcs;

	sub_port_fcs = _IPD_RD8(sc, IPD_SUB_PORT_FCS_OFFSET);
	if (enable == 0)
		CLR(sub_port_fcs, 1 << sc->sc_port);
	else
		SET(sub_port_fcs, 1 << sc->sc_port);
	_IPD_WR8(sc, IPD_SUB_PORT_FCS_OFFSET, sub_port_fcs);
}

#ifdef OCTEON_ETH_DEBUG
int			octeon_ipd_intr_rml_verbose;
struct evcnt		octeon_ipd_intr_evcnt;

static const struct octeon_evcnt_entry octeon_ipd_intr_evcnt_entries[] = {
#define	_ENTRY(name, type, parent, descr) \
	OCTEON_EVCNT_ENTRY(struct octeon_ipd_softc, name, type, parent, descr)
	_ENTRY(ipdbpsub,	MISC, NULL, "ipd backpressure subtract"),
	_ENTRY(ipdprcpar3,	MISC, NULL, "ipd parity error 127:96"),
	_ENTRY(ipdprcpar2,	MISC, NULL, "ipd parity error 95:64"),
	_ENTRY(ipdprcpar1,	MISC, NULL, "ipd parity error 63:32"),
	_ENTRY(ipdprcpar0,	MISC, NULL, "ipd parity error 31:0"),
#undef	_ENTRY
};

void
octeon_ipd_intr_evcnt_attach(struct octeon_ipd_softc *sc)
{
	OCTEON_EVCNT_ATTACH_EVCNTS(sc, octeon_ipd_intr_evcnt_entries, "ipd0");
}

void
octeon_ipd_intr_rml(void *arg)
{
	int i;

	octeon_ipd_intr_evcnt.ev_count++;
	for (i = 0; i < 3/* XXX */; i++) {
		struct octeon_ipd_softc *sc;
		uint64_t reg;

		sc = __octeon_ipd_softc[i];
		KASSERT(sc != NULL);
		reg = octeon_ipd_int_summary(sc);
		if (octeon_ipd_intr_rml_verbose)
			printf("%s: IPD_INT_SUM=0x%016" PRIx64 "\n", __func__, reg);
		if (reg & IPD_INT_SUM_BP_SUB)
			OCTEON_EVCNT_INC(sc, ipdbpsub);
		if (reg & IPD_INT_SUM_PRC_PAR3)
			OCTEON_EVCNT_INC(sc, ipdprcpar3);
		if (reg & IPD_INT_SUM_PRC_PAR2)
			OCTEON_EVCNT_INC(sc, ipdprcpar2);
		if (reg & IPD_INT_SUM_PRC_PAR1)
			OCTEON_EVCNT_INC(sc, ipdprcpar1);
		if (reg & IPD_INT_SUM_PRC_PAR0)
			OCTEON_EVCNT_INC(sc, ipdprcpar0);
	}
}

void
octeon_ipd_int_enable(struct octeon_ipd_softc *sc, int enable)
{
	uint64_t ipd_int_xxx = 0;

	SET(ipd_int_xxx,
	    IPD_INT_SUM_BP_SUB |
	    IPD_INT_SUM_PRC_PAR3 |
	    IPD_INT_SUM_PRC_PAR2 |
	    IPD_INT_SUM_PRC_PAR1 |
	    IPD_INT_SUM_PRC_PAR0);
	_IPD_WR8(sc, IPD_INT_SUM_OFFSET, ipd_int_xxx);
	_IPD_WR8(sc, IPD_INT_ENB_OFFSET, enable ? ipd_int_xxx : 0);
}

uint64_t
octeon_ipd_int_summary(struct octeon_ipd_softc *sc)
{
	uint64_t summary;

	summary = _IPD_RD8(sc, IPD_INT_SUM_OFFSET);
	_IPD_WR8(sc, IPD_INT_SUM_OFFSET, summary);
	return summary;
}

int
octeon_ipd_intr_drop(void *arg)
{
	octeon_write_csr(CIU_INT0_SUM0, CIU_INTX_SUM0_IPD_DRP);
	octeon_ipd_intr_drop_evcnt.ev_count++;
	return (1);
}

#define	_ENTRY(x)	{ #x, x##_BITS, x##_OFFSET }

struct octeon_ipd_dump_reg {
	const char *name;
	const char *format;
	size_t	offset;
};

static const struct octeon_ipd_dump_reg octeon_ipd_dump_regs[] = {
	_ENTRY(IPD_1ST_MBUFF_SKIP),
	_ENTRY(IPD_NOT_1ST_MBUFF_SKIP),
	_ENTRY(IPD_PACKET_MBUFF_SIZE),
	_ENTRY(IPD_CTL_STATUS),
	_ENTRY(IPD_WQE_FPA_QUEUE),
	_ENTRY(IPD_PORT0_BP_PAGE_CNT),
	_ENTRY(IPD_PORT1_BP_PAGE_CNT),
	_ENTRY(IPD_PORT2_BP_PAGE_CNT),
	_ENTRY(IPD_PORT32_BP_PAGE_CNT),
	_ENTRY(IPD_SUB_PORT_BP_PAGE_CNT),
	_ENTRY(IPD_1ST_NEXT_PTR_BACK),
	_ENTRY(IPD_2ND_NEXT_PTR_BACK),
	_ENTRY(IPD_INT_ENB),
	_ENTRY(IPD_INT_SUM),
	_ENTRY(IPD_SUB_PORT_FCS),
	_ENTRY(IPD_QOS0_RED_MARKS),
	_ENTRY(IPD_QOS1_RED_MARKS),
	_ENTRY(IPD_QOS2_RED_MARKS),
	_ENTRY(IPD_QOS3_RED_MARKS),
	_ENTRY(IPD_QOS4_RED_MARKS),
	_ENTRY(IPD_QOS5_RED_MARKS),
	_ENTRY(IPD_QOS6_RED_MARKS),
	_ENTRY(IPD_QOS7_RED_MARKS),
	_ENTRY(IPD_PORT_BP_COUNTERS_PAIR0),
	_ENTRY(IPD_PORT_BP_COUNTERS_PAIR1),
	_ENTRY(IPD_PORT_BP_COUNTERS_PAIR2),
	_ENTRY(IPD_PORT_BP_COUNTERS_PAIR32),
	_ENTRY(IPD_RED_PORT_ENABLE),
	_ENTRY(IPD_RED_QUE0_PARAM),
	_ENTRY(IPD_RED_QUE1_PARAM),
	_ENTRY(IPD_RED_QUE2_PARAM),
	_ENTRY(IPD_RED_QUE3_PARAM),
	_ENTRY(IPD_RED_QUE4_PARAM),
	_ENTRY(IPD_RED_QUE5_PARAM),
	_ENTRY(IPD_RED_QUE6_PARAM),
	_ENTRY(IPD_RED_QUE7_PARAM),
	_ENTRY(IPD_PTR_COUNT),
	_ENTRY(IPD_BP_PRT_RED_END),
	_ENTRY(IPD_QUE0_FREE_PAGE_CNT),
	_ENTRY(IPD_CLK_COUNT),
	_ENTRY(IPD_PWP_PTR_FIFO_CTL),
	_ENTRY(IPD_PRC_HOLD_PTR_FIFO_CTL),
	_ENTRY(IPD_PRC_PORT_PTR_FIFO_CTL),
	_ENTRY(IPD_PKT_PTR_VALID),
	_ENTRY(IPD_WQE_PTR_VALID),
	_ENTRY(IPD_BIST_STATUS),
};

void
octeon_ipd_dump(void)
{
	struct octeon_ipd_softc *sc;
	const struct octeon_ipd_dump_reg *reg;
	uint64_t tmp;
	char buf[512];
	int i;

	sc = __octeon_ipd_softc[0];
	for (i = 0; i < (int)__arraycount(octeon_ipd_dump_regs); i++) {
		reg = &octeon_ipd_dump_regs[i];
		tmp = _IPD_RD8(sc, reg->offset);
		if (reg->format == NULL) {
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		} else {
			snprintb(buf, sizeof(buf), reg->format, tmp);
		}
		printf("%-32s: %s\n", reg->name, buf);
	}
}
#endif /* OCTEON_ETH_DEBUG */
