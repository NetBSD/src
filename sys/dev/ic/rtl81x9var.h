/*	$NetBSD: rtl81x9var.h,v 1.23 2006/10/20 11:30:54 tsutsui Exp $	*/

/*
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	FreeBSD Id: if_rlreg.h,v 1.9 1999/06/20 18:56:09 wpaul Exp
 */

#include "rnd.h"

#if NRND > 0
#include <sys/rnd.h>
#endif

#ifdef __NO_STRICT_ALIGNMENT
#define	RTK_ETHER_ALIGN	0
#else
#define RTK_ETHER_ALIGN	2
#endif
#define RTK_RXSTAT_LEN	4

struct rtk_type {
	u_int16_t		rtk_vid;
	u_int16_t		rtk_did;
	int			rtk_basetype;
	const char		*rtk_name;
};

struct rtk_hwrev {
	uint32_t		rtk_rev;
	int			rtk_type;
	const char		*rtk_desc;
};

struct rtk_mii_frame {
	u_int8_t		mii_stdelim;
	u_int8_t		mii_opcode;
	u_int8_t		mii_phyaddr;
	u_int8_t		mii_regaddr;
	u_int8_t		mii_turnaround;
	u_int16_t		mii_data;
};

/*
 * MII constants
 */
#define RTK_MII_STARTDELIM	0x01
#define RTK_MII_READOP		0x02
#define RTK_MII_WRITEOP		0x01
#define RTK_MII_TURNAROUND	0x02

#define RTK_8129		1
#define RTK_8139		2
#define RTK_8139CPLUS		3
#define RTK_8169		4

#define RTK_ISCPLUS(x)	((x)->rtk_type == RTK_8139CPLUS || \
			 (x)->rtk_type == RTK_8169)

#define RTK_TX_QLEN		64

/*
 * The 8139C+ and 8160 gigE chips support descriptor-based TX
 * and RX. In fact, they even support TCP large send. Descriptors
 * must be allocated in contiguous blocks that are aligned on a
 * 256-byte boundary. The rings can hold a maximum of 64 descriptors.
 */

struct rtk_list_data {
	struct rtk_txq {
		struct mbuf *txq_mbuf;
		bus_dmamap_t txq_dmamap;
		int txq_descidx;
	} rtk_txq[RTK_TX_QLEN];
	int			rtk_txq_considx;
	int			rtk_txq_prodidx;
	bus_dmamap_t		rtk_tx_list_map;
	struct rtk_desc		*rtk_tx_list;
	bus_dma_segment_t 	rtk_tx_listseg;
	int			rtk_tx_free;	/* # of free descriptors */
	int			rtk_tx_nextfree; /* next descriptor to use */
	int			rtk_tx_desc_cnt; /* # of descriptors */
	int			rtk_tx_listnseg;

	struct mbuf		*rtk_rx_mbuf[RTK_RX_DESC_CNT];
	bus_dmamap_t		rtk_rx_dmamap[RTK_RX_DESC_CNT];
	bus_dmamap_t		rtk_rx_list_map;
	struct rtk_desc		*rtk_rx_list;
	bus_dma_segment_t 	rtk_rx_listseg;
	int			rtk_rx_prodidx;
	int			rtk_rx_listnseg;
};
struct rtk_tx_desc {
	SIMPLEQ_ENTRY(rtk_tx_desc) txd_q;
	struct mbuf		*txd_mbuf;
	bus_dmamap_t		txd_dmamap;
	bus_addr_t		txd_txaddr;
	bus_addr_t		txd_txstat;
};

struct rtk_softc {
	struct device sc_dev;		/* generic device structures */
	struct ethercom		ethercom;	/* interface info */
	struct mii_data		mii;
	struct callout		rtk_tick_ch;	/* tick callout */
	bus_space_handle_t	rtk_bhandle;	/* bus space handle */
	bus_space_tag_t		rtk_btag;	/* bus space tag */
	int			rtk_type;
	bus_dma_tag_t 		sc_dmat;
	bus_dma_segment_t 	sc_dmaseg;
	int			sc_dmanseg;

	bus_dmamap_t 		recv_dmamap;
	caddr_t			rtk_rx_buf;

	struct rtk_tx_desc	rtk_tx_descs[RTK_TX_LIST_CNT];
	SIMPLEQ_HEAD(, rtk_tx_desc) rtk_tx_free;
	SIMPLEQ_HEAD(, rtk_tx_desc) rtk_tx_dirty;
	struct rtk_list_data	rtk_ldata;
	struct mbuf		*rtk_head;
	struct mbuf		*rtk_tail;
	u_int32_t		rtk_rxlenmask;
	int			rtk_testmode;

	int			sc_flags;	/* misc flags */
	int			sc_txthresh;	/* Early tx threshold */
	int			sc_rev;		/* revision within rtk_type */

	void	*sc_sdhook;			/* shutdown hook */
	void	*sc_powerhook;			/* power management hook */

	/* Power management hooks. */
	int	(*sc_enable)	(struct rtk_softc *);
	void	(*sc_disable)	(struct rtk_softc *);
	void	(*sc_power)	(struct rtk_softc *, int);
#if NRND > 0
	rndsource_element_t     rnd_source;
#endif
};

#define	RTK_TX_DESC_CNT(sc)	\
	((sc)->rtk_ldata.rtk_tx_desc_cnt)
#define	RTK_TX_LIST_SZ(sc)	\
	(RTK_TX_DESC_CNT(sc) * sizeof(struct rtk_desc))
#define	RTK_TX_DESC_INC(sc, x)	\
	((x) = ((x) + 1) % RTK_TX_DESC_CNT(sc))
#define	RTK_RX_DESC_INC(sc, x)	\
	((x) = ((x) + 1) % RTK_RX_DESC_CNT)

#define	RTK_TXDESCSYNC(sc, idx, ops)					\
	bus_dmamap_sync((sc)->sc_dmat,					\
	    (sc)->rtk_ldata.rtk_tx_list_map,				\
	    sizeof(struct rtk_desc) * (idx),				\
	    sizeof(struct rtk_desc),					\
	    (ops))
#define	RTK_RXDESCSYNC(sc, idx, ops)					\
	bus_dmamap_sync((sc)->sc_dmat,					\
	    (sc)->rtk_ldata.rtk_rx_list_map,				\
	    sizeof(struct rtk_desc) * (idx),				\
	    sizeof(struct rtk_desc),					\
	    (ops))

#define RTK_ATTACHED 0x00000001 /* attach has succeeded */
#define RTK_ENABLED  0x00000002 /* chip is enabled	*/

#define RTK_IS_ENABLED(sc)	((sc)->sc_flags & RTK_ENABLED)
#define RTK_TX_THRESH(sc)	(((sc)->sc_txthresh << 16) & 0x003F0000)

#define TXTH_256	8
#define TXTH_MAX	48

/*
 * register space access macros
 */
#define CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4(sc->rtk_btag, sc->rtk_bhandle, reg, val)
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->rtk_btag, sc->rtk_bhandle, reg, val)
#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->rtk_btag, sc->rtk_bhandle, reg, val)
#define CSR_WRITE_STREAM_4(sc, reg, val)	\
	bus_space_write_stream_4(sc->rtk_btag, sc->rtk_bhandle, reg, val)


#define CSR_READ_4(sc, reg)		\
	bus_space_read_4(sc->rtk_btag, sc->rtk_bhandle, reg)
#define CSR_READ_2(sc, reg)		\
	bus_space_read_2(sc->rtk_btag, sc->rtk_bhandle, reg)
#define CSR_READ_1(sc, reg)		\
	bus_space_read_1(sc->rtk_btag, sc->rtk_bhandle, reg)

#define RTK_TIMEOUT		1000

/*
 * PCI low memory base and low I/O base register, and
 * other PCI registers.
 */

#define RTK_PCI_LOIO		0x10
#define RTK_PCI_LOMEM		0x14

#define RTK_PSTATE_MASK		0x0003
#define RTK_PSTATE_D0		0x0000
#define RTK_PSTATE_D1		0x0002
#define RTK_PSTATE_D2		0x0002
#define RTK_PSTATE_D3		0x0003
#define RTK_PME_EN		0x0010
#define RTK_PME_STATUS		0x8000

#ifdef _KERNEL
u_int16_t rtk_read_eeprom(struct rtk_softc *, int, int);
void	rtk_setmulti(struct rtk_softc *);
void	rtk_attach(struct rtk_softc *);
int	rtk_detach(struct rtk_softc *);
int	rtk_activate(struct device *, enum devact);
int	rtk_intr(void *);
#endif /* _KERNEL */
