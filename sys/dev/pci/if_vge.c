/* $NetBSD: if_vge.c,v 1.33.4.1 2007/03/12 05:55:22 rmind Exp $ */

/*-
 * Copyright (c) 2004
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 * FreeBSD: src/sys/dev/vge/if_vge.c,v 1.5 2005/02/07 19:39:29 glebius Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_vge.c,v 1.33.4.1 2007/03/12 05:55:22 rmind Exp $");

/*
 * VIA Networking Technologies VT612x PCI gigabit ethernet NIC driver.
 *
 * Written by Bill Paul <wpaul@windriver.com>
 * Senior Networking Software Engineer
 * Wind River Systems
 */

/*
 * The VIA Networking VT6122 is a 32bit, 33/66 MHz PCI device that
 * combines a tri-speed ethernet MAC and PHY, with the following
 * features:
 *
 *	o Jumbo frame support up to 16K
 *	o Transmit and receive flow control
 *	o IPv4 checksum offload
 *	o VLAN tag insertion and stripping
 *	o TCP large send
 *	o 64-bit multicast hash table filter
 *	o 64 entry CAM filter
 *	o 16K RX FIFO and 48K TX FIFO memory
 *	o Interrupt moderation
 *
 * The VT6122 supports up to four transmit DMA queues. The descriptors
 * in the transmit ring can address up to 7 data fragments; frames which
 * span more than 7 data buffers must be coalesced, but in general the
 * BSD TCP/IP stack rarely generates frames more than 2 or 3 fragments
 * long. The receive descriptors address only a single buffer.
 *
 * There are two peculiar design issues with the VT6122. One is that
 * receive data buffers must be aligned on a 32-bit boundary. This is
 * not a problem where the VT6122 is used as a LOM device in x86-based
 * systems, but on architectures that generate unaligned access traps, we
 * have to do some copying.
 *
 * The other issue has to do with the way 64-bit addresses are handled.
 * The DMA descriptors only allow you to specify 48 bits of addressing
 * information. The remaining 16 bits are specified using one of the
 * I/O registers. If you only have a 32-bit system, then this isn't
 * an issue, but if you have a 64-bit system and more than 4GB of
 * memory, you must have to make sure your network data buffers reside
 * in the same 48-bit 'segment.'
 *
 * Special thanks to Ryan Fu at VIA Networking for providing documentation
 * and sample NICs for testing.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <machine/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_vgereg.h>

#define VGE_JUMBO_MTU		9000

#define VGE_IFQ_MAXLEN		64

#define VGE_RING_ALIGN		256

#define VGE_NTXDESC		256
#define VGE_NTXDESC_MASK	(VGE_NTXDESC - 1)
#define VGE_NEXT_TXDESC(x)	((x + 1) & VGE_NTXDESC_MASK)
#define VGE_PREV_TXDESC(x)	((x - 1) & VGE_NTXDESC_MASK)

#define VGE_NRXDESC		256	/* Must be a multiple of 4!! */
#define VGE_NRXDESC_MASK	(VGE_NRXDESC - 1)
#define VGE_NEXT_RXDESC(x)	((x + 1) & VGE_NRXDESC_MASK)
#define VGE_PREV_RXDESC(x)	((x - 1) & VGE_NRXDESC_MASK)

#define VGE_ADDR_LO(y)		((uint64_t)(y) & 0xFFFFFFFF)
#define VGE_ADDR_HI(y)		((uint64_t)(y) >> 32)
#define VGE_BUFLEN(y)		((y) & 0x7FFF)
#define ETHER_PAD_LEN		(ETHER_MIN_LEN - ETHER_CRC_LEN)

#define VGE_POWER_MANAGEMENT	0	/* disabled for now */

/*
 * Mbuf adjust factor to force 32-bit alignment of IP header.
 * Drivers should pad ETHER_ALIGN bytes when setting up a
 * RX mbuf so the upper layers get the IP header properly aligned
 * past the 14-byte Ethernet header.
 *
 * See also comment in vge_encap().
 */
#define ETHER_ALIGN		2

#ifdef __NO_STRICT_ALIGNMENT
#define VGE_RX_BUFSIZE		MCLBYTES
#else
#define VGE_RX_PAD		sizeof(uint32_t)
#define VGE_RX_BUFSIZE		(MCLBYTES - VGE_RX_PAD)
#endif

/*
 * Control structures are DMA'd to the vge chip. We allocate them in
 * a single clump that maps to a single DMA segment to make several things
 * easier.
 */
struct vge_control_data {
	/* TX descriptors */
	struct vge_txdesc	vcd_txdescs[VGE_NTXDESC];
	/* RX descriptors */
	struct vge_rxdesc	vcd_rxdescs[VGE_NRXDESC];
	/* dummy data for TX padding */
	uint8_t			vcd_pad[ETHER_PAD_LEN];
};

#define VGE_CDOFF(x)	offsetof(struct vge_control_data, x)
#define VGE_CDTXOFF(x)	VGE_CDOFF(vcd_txdescs[(x)])
#define VGE_CDRXOFF(x)	VGE_CDOFF(vcd_rxdescs[(x)])
#define VGE_CDPADOFF()	VGE_CDOFF(vcd_pad[0])

/*
 * Software state for TX jobs.
 */
struct vge_txsoft {
	struct mbuf	*txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t	txs_dmamap;		/* our DMA map */
};

/*
 * Software state for RX jobs.
 */
struct vge_rxsoft {
	struct mbuf	*rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t	rxs_dmamap;		/* our DMA map */
};


struct vge_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_bst;		/* bus space tag */
	bus_space_handle_t	sc_bsh;		/* bus space handle */
	bus_dma_tag_t		sc_dmat;

	struct ethercom		sc_ethercom;	/* interface info */
	uint8_t			sc_eaddr[ETHER_ADDR_LEN];

	void			*sc_intrhand;
	struct mii_data		sc_mii;
	uint8_t			sc_type;
	int			sc_if_flags;
	int			sc_link;
	int			sc_camidx;
	struct callout		sc_timeout;

	bus_dmamap_t		sc_cddmamap;
#define sc_cddma		sc_cddmamap->dm_segs[0].ds_addr

	struct vge_txsoft	sc_txsoft[VGE_NTXDESC];
	struct vge_rxsoft	sc_rxsoft[VGE_NRXDESC];
	struct vge_control_data	*sc_control_data;
#define sc_txdescs		sc_control_data->vcd_txdescs
#define sc_rxdescs		sc_control_data->vcd_rxdescs

	int			sc_tx_prodidx;
	int			sc_tx_considx;
	int			sc_tx_free;

	struct mbuf		*sc_rx_mhead;
	struct mbuf		*sc_rx_mtail;
	int			sc_rx_prodidx;
	int			sc_rx_consumed;

	int			sc_suspended;	/* 0 = normal  1 = suspended */
	uint32_t		sc_saved_maps[5];	/* pci data */
	uint32_t		sc_saved_biosaddr;
	uint8_t			sc_saved_intline;
	uint8_t			sc_saved_cachelnsz;
	uint8_t			sc_saved_lattimer;
};

#define VGE_CDTXADDR(sc, x)	((sc)->sc_cddma + VGE_CDTXOFF(x))
#define VGE_CDRXADDR(sc, x)	((sc)->sc_cddma + VGE_CDRXOFF(x))
#define VGE_CDPADADDR(sc)	((sc)->sc_cddma + VGE_CDPADOFF())

#define VGE_TXDESCSYNC(sc, idx, ops)					\
	bus_dmamap_sync((sc)->sc_dmat,(sc)->sc_cddmamap,		\
	    VGE_CDTXOFF(idx),						\
	    offsetof(struct vge_txdesc, td_frag[0]),			\
	    (ops))
#define VGE_TXFRAGSYNC(sc, idx, nsegs, ops)				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    VGE_CDTXOFF(idx) +						\
	    offsetof(struct vge_txdesc, td_frag[0]),			\
	    sizeof(struct vge_txfrag) * (nsegs),			\
	    (ops))
#define VGE_RXDESCSYNC(sc, idx, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    VGE_CDRXOFF(idx),						\
	    sizeof(struct vge_rxdesc),					\
	    (ops))

/*
 * register space access macros
 */
#define CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define CSR_READ_4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CSR_READ_2(sc, reg)		\
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CSR_READ_1(sc, reg)		\
	bus_space_read_1((sc)->sc_bst, (sc)->sc_bsh, (reg))

#define CSR_SETBIT_1(sc, reg, x)	\
	CSR_WRITE_1((sc), (reg), CSR_READ_1((sc), (reg)) | (x))
#define CSR_SETBIT_2(sc, reg, x)	\
	CSR_WRITE_2((sc), (reg), CSR_READ_2((sc), (reg)) | (x))
#define CSR_SETBIT_4(sc, reg, x)	\
	CSR_WRITE_4((sc), (reg), CSR_READ_4((sc), (reg)) | (x))

#define CSR_CLRBIT_1(sc, reg, x)	\
	CSR_WRITE_1((sc), (reg), CSR_READ_1((sc), (reg)) & ~(x))
#define CSR_CLRBIT_2(sc, reg, x)	\
	CSR_WRITE_2((sc), (reg), CSR_READ_2((sc), (reg)) & ~(x))
#define CSR_CLRBIT_4(sc, reg, x)	\
	CSR_WRITE_4((sc), (reg), CSR_READ_4((sc), (reg)) & ~(x))

#define VGE_TIMEOUT		10000

#define VGE_PCI_LOIO             0x10
#define VGE_PCI_LOMEM            0x14

static inline void vge_set_txaddr(struct vge_txfrag *, bus_addr_t);
static inline void vge_set_rxaddr(struct vge_rxdesc *, bus_addr_t);

static int vge_match(struct device *, struct cfdata *, void *);
static void vge_attach(struct device *, struct device *, void *);

static int vge_encap(struct vge_softc *, struct mbuf *, int);

static int vge_allocmem(struct vge_softc *);
static int vge_newbuf(struct vge_softc *, int, struct mbuf *);
#ifndef __NO_STRICT_ALIGNMENT
static inline void vge_fixup_rx(struct mbuf *);
#endif
static void vge_rxeof(struct vge_softc *);
static void vge_txeof(struct vge_softc *);
static int vge_intr(void *);
static void vge_tick(void *);
static void vge_start(struct ifnet *);
static int vge_ioctl(struct ifnet *, u_long, void *);
static int vge_init(struct ifnet *);
static void vge_stop(struct vge_softc *);
static void vge_watchdog(struct ifnet *);
#if VGE_POWER_MANAGEMENT
static int vge_suspend(struct device *);
static int vge_resume(struct device *);
#endif
static void vge_shutdown(void *);
static int vge_ifmedia_upd(struct ifnet *);
static void vge_ifmedia_sts(struct ifnet *, struct ifmediareq *);

static uint16_t vge_read_eeprom(struct vge_softc *, int);

static void vge_miipoll_start(struct vge_softc *);
static void vge_miipoll_stop(struct vge_softc *);
static int vge_miibus_readreg(struct device *, int, int);
static void vge_miibus_writereg(struct device *, int, int, int);
static void vge_miibus_statchg(struct device *);

static void vge_cam_clear(struct vge_softc *);
static int vge_cam_set(struct vge_softc *, uint8_t *);
static void vge_setmulti(struct vge_softc *);
static void vge_reset(struct vge_softc *);

CFATTACH_DECL(vge, sizeof(struct vge_softc),
    vge_match, vge_attach, NULL, NULL);

static inline void
vge_set_txaddr(struct vge_txfrag *f, bus_addr_t daddr)
{

	f->tf_addrlo = htole32((uint32_t)daddr);
	if (sizeof(bus_addr_t) == sizeof(uint64_t))
		f->tf_addrhi = htole16(((uint64_t)daddr >> 32) & 0xFFFF);
	else
		f->tf_addrhi = 0;
}

static inline void
vge_set_rxaddr(struct vge_rxdesc *rxd, bus_addr_t daddr)
{

	rxd->rd_addrlo = htole32((uint32_t)daddr);
	if (sizeof(bus_addr_t) == sizeof(uint64_t))
		rxd->rd_addrhi = htole16(((uint64_t)daddr >> 32) & 0xFFFF);
	else
		rxd->rd_addrhi = 0;
}

/*
 * Defragment mbuf chain contents to be as linear as possible.
 * Returns new mbuf chain on success, NULL on failure. Old mbuf
 * chain is always freed.
 * XXX temporary until there would be generic function doing this.
 */
#define m_defrag	vge_m_defrag
struct mbuf * vge_m_defrag(struct mbuf *, int);

struct mbuf *
vge_m_defrag(struct mbuf *mold, int flags)
{
	struct mbuf *m0, *mn, *n;
	size_t sz = mold->m_pkthdr.len;

#ifdef DIAGNOSTIC
	if ((mold->m_flags & M_PKTHDR) == 0)
		panic("m_defrag: not a mbuf chain header");
#endif

	MGETHDR(m0, flags, MT_DATA);
	if (m0 == NULL)
		return NULL;
	m0->m_pkthdr.len = mold->m_pkthdr.len;
	mn = m0;

	do {
		if (sz > MHLEN) {
			MCLGET(mn, M_DONTWAIT);
			if ((mn->m_flags & M_EXT) == 0) {
				m_freem(m0);
				return NULL;
			}
		}

		mn->m_len = MIN(sz, MCLBYTES);

		m_copydata(mold, mold->m_pkthdr.len - sz, mn->m_len,
		     mtod(mn, void *));

		sz -= mn->m_len;

		if (sz > 0) {
			/* need more mbufs */
			MGET(n, M_NOWAIT, MT_DATA);
			if (n == NULL) {
				m_freem(m0);
				return NULL;
			}

			mn->m_next = n;
			mn = n;
		}
	} while (sz > 0);

	return m0;
}

/*
 * Read a word of data stored in the EEPROM at address 'addr.'
 */
static uint16_t
vge_read_eeprom(struct vge_softc *sc, int addr)
{
	int i;
	uint16_t word = 0;

	/*
	 * Enter EEPROM embedded programming mode. In order to
	 * access the EEPROM at all, we first have to set the
	 * EELOAD bit in the CHIPCFG2 register.
	 */
	CSR_SETBIT_1(sc, VGE_CHIPCFG2, VGE_CHIPCFG2_EELOAD);
	CSR_SETBIT_1(sc, VGE_EECSR, VGE_EECSR_EMBP/*|VGE_EECSR_ECS*/);

	/* Select the address of the word we want to read */
	CSR_WRITE_1(sc, VGE_EEADDR, addr);

	/* Issue read command */
	CSR_SETBIT_1(sc, VGE_EECMD, VGE_EECMD_ERD);

	/* Wait for the done bit to be set. */
	for (i = 0; i < VGE_TIMEOUT; i++) {
		if (CSR_READ_1(sc, VGE_EECMD) & VGE_EECMD_EDONE)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: EEPROM read timed out\n",
		    sc->sc_dev.dv_xname);
		return 0;
	}

	/* Read the result */
	word = CSR_READ_2(sc, VGE_EERDDAT);

	/* Turn off EEPROM access mode. */
	CSR_CLRBIT_1(sc, VGE_EECSR, VGE_EECSR_EMBP/*|VGE_EECSR_ECS*/);
	CSR_CLRBIT_1(sc, VGE_CHIPCFG2, VGE_CHIPCFG2_EELOAD);

	return word;
}

static void
vge_miipoll_stop(struct vge_softc *sc)
{
	int i;

	CSR_WRITE_1(sc, VGE_MIICMD, 0);

	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if (CSR_READ_1(sc, VGE_MIISTS) & VGE_MIISTS_IIDL)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: failed to idle MII autopoll\n",
		    sc->sc_dev.dv_xname);
	}
}

static void
vge_miipoll_start(struct vge_softc *sc)
{
	int i;

	/* First, make sure we're idle. */

	CSR_WRITE_1(sc, VGE_MIICMD, 0);
	CSR_WRITE_1(sc, VGE_MIIADDR, VGE_MIIADDR_SWMPL);

	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if (CSR_READ_1(sc, VGE_MIISTS) & VGE_MIISTS_IIDL)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: failed to idle MII autopoll\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Now enable auto poll mode. */

	CSR_WRITE_1(sc, VGE_MIICMD, VGE_MIICMD_MAUTO);

	/* And make sure it started. */

	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if ((CSR_READ_1(sc, VGE_MIISTS) & VGE_MIISTS_IIDL) == 0)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: failed to start MII autopoll\n",
		    sc->sc_dev.dv_xname);
	}
}

static int
vge_miibus_readreg(struct device *dev, int phy, int reg)
{
	struct vge_softc *sc;
	int i, s;
	uint16_t rval;

	sc = (void *)dev;
	rval = 0;
	if (phy != (CSR_READ_1(sc, VGE_MIICFG) & 0x1F))
		return 0;

	s = splnet();
	vge_miipoll_stop(sc);

	/* Specify the register we want to read. */
	CSR_WRITE_1(sc, VGE_MIIADDR, reg);

	/* Issue read command. */
	CSR_SETBIT_1(sc, VGE_MIICMD, VGE_MIICMD_RCMD);

	/* Wait for the read command bit to self-clear. */
	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if ((CSR_READ_1(sc, VGE_MIICMD) & VGE_MIICMD_RCMD) == 0)
			break;
	}

	if (i == VGE_TIMEOUT)
		aprint_error("%s: MII read timed out\n", sc->sc_dev.dv_xname);
	else
		rval = CSR_READ_2(sc, VGE_MIIDATA);

	vge_miipoll_start(sc);
	splx(s);

	return rval;
}

static void
vge_miibus_writereg(struct device *dev, int phy, int reg, int data)
{
	struct vge_softc *sc;
	int i, s;

	sc = (void *)dev;
	if (phy != (CSR_READ_1(sc, VGE_MIICFG) & 0x1F))
		return;

	s = splnet();
	vge_miipoll_stop(sc);

	/* Specify the register we want to write. */
	CSR_WRITE_1(sc, VGE_MIIADDR, reg);

	/* Specify the data we want to write. */
	CSR_WRITE_2(sc, VGE_MIIDATA, data);

	/* Issue write command. */
	CSR_SETBIT_1(sc, VGE_MIICMD, VGE_MIICMD_WCMD);

	/* Wait for the write command bit to self-clear. */
	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if ((CSR_READ_1(sc, VGE_MIICMD) & VGE_MIICMD_WCMD) == 0)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: MII write timed out\n", sc->sc_dev.dv_xname);
	}

	vge_miipoll_start(sc);
	splx(s);
}

static void
vge_cam_clear(struct vge_softc *sc)
{
	int i;

	/*
	 * Turn off all the mask bits. This tells the chip
	 * that none of the entries in the CAM filter are valid.
	 * desired entries will be enabled as we fill the filter in.
	 */

	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_CAMMASK);
	CSR_WRITE_1(sc, VGE_CAMADDR, VGE_CAMADDR_ENABLE);
	for (i = 0; i < 8; i++)
		CSR_WRITE_1(sc, VGE_CAM0 + i, 0);

	/* Clear the VLAN filter too. */

	CSR_WRITE_1(sc, VGE_CAMADDR, VGE_CAMADDR_ENABLE|VGE_CAMADDR_AVSEL|0);
	for (i = 0; i < 8; i++)
		CSR_WRITE_1(sc, VGE_CAM0 + i, 0);

	CSR_WRITE_1(sc, VGE_CAMADDR, 0);
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_MAR);

	sc->sc_camidx = 0;
}

static int
vge_cam_set(struct vge_softc *sc, uint8_t *addr)
{
	int i, error;

	error = 0;

	if (sc->sc_camidx == VGE_CAM_MAXADDRS)
		return ENOSPC;

	/* Select the CAM data page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_CAMDATA);

	/* Set the filter entry we want to update and enable writing. */
	CSR_WRITE_1(sc, VGE_CAMADDR, VGE_CAMADDR_ENABLE | sc->sc_camidx);

	/* Write the address to the CAM registers */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		CSR_WRITE_1(sc, VGE_CAM0 + i, addr[i]);

	/* Issue a write command. */
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_WRITE);

	/* Wake for it to clear. */
	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(1);
		if ((CSR_READ_1(sc, VGE_CAMCTL) & VGE_CAMCTL_WRITE) == 0)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: setting CAM filter failed\n",
		    sc->sc_dev.dv_xname);
		error = EIO;
		goto fail;
	}

	/* Select the CAM mask page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_CAMMASK);

	/* Set the mask bit that enables this filter. */
	CSR_SETBIT_1(sc, VGE_CAM0 + (sc->sc_camidx / 8),
	    1 << (sc->sc_camidx & 7));

	sc->sc_camidx++;

 fail:
	/* Turn off access to CAM. */
	CSR_WRITE_1(sc, VGE_CAMADDR, 0);
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_MAR);

	return error;
}

/*
 * Program the multicast filter. We use the 64-entry CAM filter
 * for perfect filtering. If there's more than 64 multicast addresses,
 * we use the hash filter instead.
 */
static void
vge_setmulti(struct vge_softc *sc)
{
	struct ifnet *ifp;
	int error;
	uint32_t h, hashes[2] = { 0, 0 };
	struct ether_multi *enm;
	struct ether_multistep step;

	error = 0;
	ifp = &sc->sc_ethercom.ec_if;

	/* First, zot all the multicast entries. */
	vge_cam_clear(sc);
	CSR_WRITE_4(sc, VGE_MAR0, 0);
	CSR_WRITE_4(sc, VGE_MAR1, 0);
	ifp->if_flags &= ~IFF_ALLMULTI;

	/*
	 * If the user wants allmulti or promisc mode, enable reception
	 * of all multicast frames.
	 */
	if (ifp->if_flags & IFF_PROMISC) {
 allmulti:
		CSR_WRITE_4(sc, VGE_MAR0, 0xFFFFFFFF);
		CSR_WRITE_4(sc, VGE_MAR1, 0xFFFFFFFF);
		ifp->if_flags |= IFF_ALLMULTI;
		return;
	}

	/* Now program new ones */
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		/*
		 * If multicast range, fall back to ALLMULTI.
		 */
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
				ETHER_ADDR_LEN) != 0)
			goto allmulti;

		error = vge_cam_set(sc, enm->enm_addrlo);
		if (error)
			break;

		ETHER_NEXT_MULTI(step, enm);
	}

	/* If there were too many addresses, use the hash filter. */
	if (error) {
		vge_cam_clear(sc);

		ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
		while (enm != NULL) {
			/*
			 * If multicast range, fall back to ALLMULTI.
			 */
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
					ETHER_ADDR_LEN) != 0)
				goto allmulti;

			h = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN) >> 26;
			hashes[h >> 5] |= 1 << (h & 0x1f);

			ETHER_NEXT_MULTI(step, enm);
		}

		CSR_WRITE_4(sc, VGE_MAR0, hashes[0]);
		CSR_WRITE_4(sc, VGE_MAR1, hashes[1]);
	}
}

static void
vge_reset(struct vge_softc *sc)
{
	int i;

	CSR_WRITE_1(sc, VGE_CRS1, VGE_CR1_SOFTRESET);

	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(5);
		if ((CSR_READ_1(sc, VGE_CRS1) & VGE_CR1_SOFTRESET) == 0)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: soft reset timed out", sc->sc_dev.dv_xname);
		CSR_WRITE_1(sc, VGE_CRS3, VGE_CR3_STOP_FORCE);
		DELAY(2000);
	}

	DELAY(5000);

	CSR_SETBIT_1(sc, VGE_EECSR, VGE_EECSR_RELOAD);

	for (i = 0; i < VGE_TIMEOUT; i++) {
		DELAY(5);
		if ((CSR_READ_1(sc, VGE_EECSR) & VGE_EECSR_RELOAD) == 0)
			break;
	}

	if (i == VGE_TIMEOUT) {
		aprint_error("%s: EEPROM reload timed out\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * On some machine, the first read data from EEPROM could be
	 * messed up, so read one dummy data here to avoid the mess.
	 */
	(void)vge_read_eeprom(sc, 0);

	CSR_CLRBIT_1(sc, VGE_CHIPCFG0, VGE_CHIPCFG0_PACPI);
}

/*
 * Probe for a VIA gigabit chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
vge_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VIATECH
	    && PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VIATECH_VT612X)
		return 1;

	return 0;
}

static int
vge_allocmem(struct vge_softc *sc)
{
	int error;
	int nseg;
	int i;
	bus_dma_segment_t seg;

	/*
	 * Allocate memory for control data.
	 */

	error = bus_dmamem_alloc(sc->sc_dmat, sizeof(struct vge_control_data),
	     VGE_RING_ALIGN, 0, &seg, 1, &nseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: could not allocate control data dma memory\n",
		    sc->sc_dev.dv_xname);
		goto fail_1;
	}

	/* Map the memory to kernel VA space */

	error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct vge_control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: could not map control data dma memory\n",
		    sc->sc_dev.dv_xname);
		goto fail_2;
	}
	memset(sc->sc_control_data, 0, sizeof(struct vge_control_data));

	/*
	 * Create map for control data.
	 */
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct vge_control_data), 1,
	    sizeof(struct vge_control_data), 0, BUS_DMA_NOWAIT,
	    &sc->sc_cddmamap);
	if (error) {
		aprint_error("%s: could not create control data dmamap\n",
		    sc->sc_dev.dv_xname);
		goto fail_3;
	}

	/* Load the map for the control data. */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct vge_control_data), NULL,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: could not load control data dma memory\n",
		    sc->sc_dev.dv_xname);
		goto fail_4;
	}

	/* Create DMA maps for TX buffers */

	for (i = 0; i < VGE_NTXDESC; i++) {
		error = bus_dmamap_create(sc->sc_dmat, VGE_TX_MAXLEN,
		    VGE_TX_FRAGS, VGE_TX_MAXLEN, 0, BUS_DMA_NOWAIT,
		    &sc->sc_txsoft[i].txs_dmamap);
		if (error) {
			aprint_error("%s: can't create DMA map for TX descs\n",
			    sc->sc_dev.dv_xname);
			goto fail_5;
		}
	}

	/* Create DMA maps for RX buffers */

	for (i = 0; i < VGE_NRXDESC; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_rxsoft[i].rxs_dmamap);
		if (error) {
			aprint_error("%s: can't create DMA map for RX descs\n",
			    sc->sc_dev.dv_xname);
			goto fail_6;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	return 0;

 fail_6:
	for (i = 0; i < VGE_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_5:
	for (i = 0; i < VGE_NTXDESC; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_4:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct vge_control_data));
 fail_2:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
 fail_1:
	return ENOMEM;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
vge_attach(struct device *parent, struct device *self, void *aux)
{
	uint8_t	*eaddr;
	struct vge_softc *sc = (void *)self;
	struct ifnet *ifp;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	const char *intrstr;
	pci_intr_handle_t ih;
	uint16_t val;

	aprint_normal(": VIA VT612X Gigabit Ethernet (rev. %#x)\n",
	    PCI_REVISION(pa->pa_class));

	/* Make sure bus-mastering is enabled */
        pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Map control/status registers.
	 */
	if (pci_mapreg_map(pa, VGE_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_bst, &sc->sc_bsh, NULL, NULL) != 0) {
		aprint_error("%s: couldn't map memory\n", sc->sc_dev.dv_xname);
		return;
	}

        /*
         * Map and establish our interrupt.
         */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: unable to map interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_intrhand = pci_intr_establish(pc, ih, IPL_NET, vge_intr, sc);
	if (sc->sc_intrhand == NULL) {
		aprint_error("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* Reset the adapter. */
	vge_reset(sc);

	/*
	 * Get station address from the EEPROM.
	 */
	eaddr = sc->sc_eaddr;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 0);
	eaddr[0] = val & 0xff;
	eaddr[1] = val >> 8;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 1);
	eaddr[2] = val & 0xff;
	eaddr[3] = val >> 8;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 2);
	eaddr[4] = val & 0xff;
	eaddr[5] = val >> 8;

	aprint_normal("%s: Ethernet address: %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(eaddr));

	/*
	 * Use the 32bit tag. Hardware supports 48bit physical addresses,
	 * but we don't use that for now.
	 */
	sc->sc_dmat = pa->pa_dmat;

	if (vge_allocmem(sc) != 0)
		return;

	ifp = &sc->sc_ethercom.ec_if;
	ifp->if_softc = sc;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_mtu = ETHERMTU;
	ifp->if_baudrate = IF_Gbps(1);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = vge_ioctl;
	ifp->if_start = vge_start;

	/*
	 * We can support 802.1Q VLAN-sized frames and jumbo
	 * Ethernet frames.
	 */
	sc->sc_ethercom.ec_capabilities |=
	    ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU |
	    ETHERCAP_VLAN_HWTAGGING;

	/*
	 * We can do IPv4/TCPv4/UDPv4 checksums in hardware.
	 */
	ifp->if_capabilities |=
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;

#ifdef DEVICE_POLLING
#ifdef IFCAP_POLLING
	ifp->if_capabilities |= IFCAP_POLLING;
#endif
#endif
	ifp->if_watchdog = vge_watchdog;
	ifp->if_init = vge_init;
	IFQ_SET_MAXLEN(&ifp->if_snd, max(VGE_IFQ_MAXLEN, IFQ_MAXLEN));

	/*
	 * Initialize our media structures and probe the MII.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = vge_miibus_readreg;
	sc->sc_mii.mii_writereg = vge_miibus_writereg;
	sc->sc_mii.mii_statchg = vge_miibus_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, vge_ifmedia_upd,
	    vge_ifmedia_sts);
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, MIIF_DOPAUSE);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

	callout_init(&sc->sc_timeout);
	callout_setfunc(&sc->sc_timeout, vge_tick, sc);

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	if (shutdownhook_establish(vge_shutdown, sc) == NULL) {
		aprint_error("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	}
}

static int
vge_newbuf(struct vge_softc *sc, int idx, struct mbuf *m)
{
	struct mbuf *m_new;
	struct vge_rxdesc *rxd;
	struct vge_rxsoft *rxs;
	bus_dmamap_t map;
	int i;
#ifdef DIAGNOSTIC
	uint32_t rd_sts;
#endif

	m_new = NULL;
	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL)
			return ENOBUFS;

		MCLGET(m_new, M_DONTWAIT);
		if ((m_new->m_flags & M_EXT) == 0) {
			m_freem(m_new);
			return ENOBUFS;
		}

		m = m_new;
	} else
		m->m_data = m->m_ext.ext_buf;


	/*
	 * This is part of an evil trick to deal with non-x86 platforms.
	 * The VIA chip requires RX buffers to be aligned on 32-bit
	 * boundaries, but that will hose non-x86 machines. To get around
	 * this, we leave some empty space at the start of each buffer
	 * and for non-x86 hosts, we copy the buffer back two bytes
	 * to achieve word alignment. This is slightly more efficient
	 * than allocating a new buffer, copying the contents, and
	 * discarding the old buffer.
	 */
	m->m_len = m->m_pkthdr.len = VGE_RX_BUFSIZE;
#ifndef __NO_STRICT_ALIGNMENT
	m->m_data += VGE_RX_PAD;
#endif
	rxs = &sc->sc_rxsoft[idx];
	map = rxs->rxs_dmamap;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m, BUS_DMA_NOWAIT) != 0)
		goto out;

	rxd = &sc->sc_rxdescs[idx];

#ifdef DIAGNOSTIC
	/* If this descriptor is still owned by the chip, bail. */
	VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	rd_sts = le32toh(rxd->rd_sts);
	VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
	if (rd_sts & VGE_RDSTS_OWN) {
		panic("%s: tried to map busy RX descriptor",
		    sc->sc_dev.dv_xname);
	}
#endif

	rxs->rxs_mbuf = m;
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	rxd->rd_buflen =
	    htole16(VGE_BUFLEN(map->dm_segs[0].ds_len) | VGE_RXDESC_I);
	vge_set_rxaddr(rxd, map->dm_segs[0].ds_addr);
	rxd->rd_sts = 0;
	rxd->rd_ctl = 0;
	VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Note: the manual fails to document the fact that for
	 * proper opration, the driver needs to replentish the RX
	 * DMA ring 4 descriptors at a time (rather than one at a
	 * time, like most chips). We can allocate the new buffers
	 * but we should not set the OWN bits until we're ready
	 * to hand back 4 of them in one shot.
	 */

#define VGE_RXCHUNK 4
	sc->sc_rx_consumed++;
	if (sc->sc_rx_consumed == VGE_RXCHUNK) {
		for (i = idx; i != idx - VGE_RXCHUNK; i--) {
			KASSERT(i >= 0);
			sc->sc_rxdescs[i].rd_sts |= htole32(VGE_RDSTS_OWN);
			VGE_RXDESCSYNC(sc, i,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		}
		sc->sc_rx_consumed = 0;
	}

	return 0;
 out:
	if (m_new != NULL)
		m_freem(m_new);
	return ENOMEM;
}

#ifndef __NO_STRICT_ALIGNMENT
static inline void
vge_fixup_rx(struct mbuf *m)
{
	int i;
	uint16_t *src, *dst;

	src = mtod(m, uint16_t *);
	dst = src - 1;

	for (i = 0; i < (m->m_len / sizeof(uint16_t) + 1); i++)
		*dst++ = *src++;

	m->m_data -= ETHER_ALIGN;
}
#endif

/*
 * RX handler. We support the reception of jumbo frames that have
 * been fragmented across multiple 2K mbuf cluster buffers.
 */
static void
vge_rxeof(struct vge_softc *sc)
{
	struct mbuf *m;
	struct ifnet *ifp;
	int idx, total_len, lim;
	struct vge_rxdesc *cur_rxd;
	struct vge_rxsoft *rxs;
	uint32_t rxstat, rxctl;

	ifp = &sc->sc_ethercom.ec_if;
	lim = 0;

	/* Invalidate the descriptor memory */

	for (idx = sc->sc_rx_prodidx;; idx = VGE_NEXT_RXDESC(idx)) {
		cur_rxd = &sc->sc_rxdescs[idx];

		VGE_RXDESCSYNC(sc, idx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		rxstat = le32toh(cur_rxd->rd_sts);
		if ((rxstat & VGE_RDSTS_OWN) != 0) {
			VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
			break;
		}

		rxctl = le32toh(cur_rxd->rd_ctl);
		rxs = &sc->sc_rxsoft[idx];
		m = rxs->rxs_mbuf;
		total_len = (rxstat & VGE_RDSTS_BUFSIZ) >> 16;

		/* Invalidate the RX mbuf and unload its map */

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap,
		    0, rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

		/*
		 * If the 'start of frame' bit is set, this indicates
		 * either the first fragment in a multi-fragment receive,
		 * or an intermediate fragment. Either way, we want to
		 * accumulate the buffers.
		 */
		if (rxstat & VGE_RXPKT_SOF) {
			m->m_len = VGE_RX_BUFSIZE;
			if (sc->sc_rx_mhead == NULL)
				sc->sc_rx_mhead = sc->sc_rx_mtail = m;
			else {
				m->m_flags &= ~M_PKTHDR;
				sc->sc_rx_mtail->m_next = m;
				sc->sc_rx_mtail = m;
			}
			vge_newbuf(sc, idx, NULL);
			continue;
		}

		/*
		 * Bad/error frames will have the RXOK bit cleared.
		 * However, there's one error case we want to allow:
		 * if a VLAN tagged frame arrives and the chip can't
		 * match it against the CAM filter, it considers this
		 * a 'VLAN CAM filter miss' and clears the 'RXOK' bit.
		 * We don't want to drop the frame though: our VLAN
		 * filtering is done in software.
		 */
		if ((rxstat & VGE_RDSTS_RXOK) == 0 &&
		    (rxstat & VGE_RDSTS_VIDM) == 0 &&
		    (rxstat & VGE_RDSTS_CSUMERR) == 0) {
			ifp->if_ierrors++;
			/*
			 * If this is part of a multi-fragment packet,
			 * discard all the pieces.
			 */
			if (sc->sc_rx_mhead != NULL) {
				m_freem(sc->sc_rx_mhead);
				sc->sc_rx_mhead = sc->sc_rx_mtail = NULL;
			}
			vge_newbuf(sc, idx, m);
			continue;
		}

		/*
		 * If allocating a replacement mbuf fails,
		 * reload the current one.
		 */

		if (vge_newbuf(sc, idx, NULL)) {
			ifp->if_ierrors++;
			if (sc->sc_rx_mhead != NULL) {
				m_freem(sc->sc_rx_mhead);
				sc->sc_rx_mhead = sc->sc_rx_mtail = NULL;
			}
			vge_newbuf(sc, idx, m);
			continue;
		}

		if (sc->sc_rx_mhead != NULL) {
			m->m_len = total_len % VGE_RX_BUFSIZE;
			/*
			 * Special case: if there's 4 bytes or less
			 * in this buffer, the mbuf can be discarded:
			 * the last 4 bytes is the CRC, which we don't
			 * care about anyway.
			 */
			if (m->m_len <= ETHER_CRC_LEN) {
				sc->sc_rx_mtail->m_len -=
				    (ETHER_CRC_LEN - m->m_len);
				m_freem(m);
			} else {
				m->m_len -= ETHER_CRC_LEN;
				m->m_flags &= ~M_PKTHDR;
				sc->sc_rx_mtail->m_next = m;
			}
			m = sc->sc_rx_mhead;
			sc->sc_rx_mhead = sc->sc_rx_mtail = NULL;
			m->m_pkthdr.len = total_len - ETHER_CRC_LEN;
		} else
			m->m_pkthdr.len = m->m_len = total_len - ETHER_CRC_LEN;

#ifndef __NO_STRICT_ALIGNMENT
		vge_fixup_rx(m);
#endif
		ifp->if_ipackets++;
		m->m_pkthdr.rcvif = ifp;

		/* Do RX checksumming if enabled */
		if (ifp->if_csum_flags_rx & M_CSUM_IPv4) {

			/* Check IP header checksum */
			if (rxctl & VGE_RDCTL_IPPKT)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			if ((rxctl & VGE_RDCTL_IPCSUMOK) == 0)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
		}

		if (ifp->if_csum_flags_rx & M_CSUM_TCPv4) {
			/* Check UDP checksum */
			if (rxctl & VGE_RDCTL_TCPPKT)
				m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;

			if ((rxctl & VGE_RDCTL_PROTOCSUMOK) == 0)
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}

		if (ifp->if_csum_flags_rx & M_CSUM_UDPv4) {
			/* Check UDP checksum */
			if (rxctl & VGE_RDCTL_UDPPKT)
				m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;

			if ((rxctl & VGE_RDCTL_PROTOCSUMOK) == 0)
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}

		if (rxstat & VGE_RDSTS_VTAG) {
			/*
			 * We use bswap16() here because:
			 * On LE machines, tag is stored in BE as stream data.
			 * On BE machines, tag is stored in BE as stream data
			 *  but it was already swapped by le32toh() above.
			 */
			VLAN_INPUT_TAG(ifp, m,
			    bswap16(rxctl & VGE_RDCTL_VLANID), continue);
		}

#if NBPFILTER > 0
		/*
		 * Handle BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		(*ifp->if_input)(ifp, m);

		lim++;
		if (lim == VGE_NRXDESC)
			break;
	}

	sc->sc_rx_prodidx = idx;
	CSR_WRITE_2(sc, VGE_RXDESC_RESIDUECNT, lim);
}

static void
vge_txeof(struct vge_softc *sc)
{
	struct ifnet *ifp;
	struct vge_txsoft *txs;
	uint32_t txstat;
	int idx;

	ifp = &sc->sc_ethercom.ec_if;

	for (idx = sc->sc_tx_considx;
	    sc->sc_tx_free < VGE_NTXDESC;
	    idx = VGE_NEXT_TXDESC(idx), sc->sc_tx_free++) {
		VGE_TXDESCSYNC(sc, idx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		txstat = le32toh(sc->sc_txdescs[idx].td_sts);
		VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
		if (txstat & VGE_TDSTS_OWN) {
			break;
		}

		txs = &sc->sc_txsoft[idx];
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap, 0,
		    txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		if (txstat & (VGE_TDSTS_EXCESSCOLL|VGE_TDSTS_COLL))
			ifp->if_collisions++;
		if (txstat & VGE_TDSTS_TXERR)
			ifp->if_oerrors++;
		else
			ifp->if_opackets++;
	}

	sc->sc_tx_considx = idx;

	if (sc->sc_tx_free > 0) {
		ifp->if_flags &= ~IFF_OACTIVE;
	}

	/*
	 * If not all descriptors have been released reaped yet,
	 * reload the timer so that we will eventually get another
	 * interrupt that will cause us to re-enter this routine.
	 * This is done in case the transmitter has gone idle.
	 */
	if (sc->sc_tx_free < VGE_NTXDESC)
		CSR_WRITE_1(sc, VGE_CRS1, VGE_CR1_TIMER0_ENABLE);
	else
		ifp->if_timer = 0;
}

static void
vge_tick(void *xsc)
{
	struct vge_softc *sc;
	struct ifnet *ifp;
	struct mii_data *mii;
	int s;

	sc = xsc;
	ifp = &sc->sc_ethercom.ec_if;
	mii = &sc->sc_mii;

	s = splnet();

	callout_schedule(&sc->sc_timeout, hz);

	mii_tick(mii);
	if (sc->sc_link) {
		if ((mii->mii_media_status & IFM_ACTIVE) == 0)
			sc->sc_link = 0;
	} else {
		if (mii->mii_media_status & IFM_ACTIVE &&
		    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
			sc->sc_link = 1;
			if (!IFQ_IS_EMPTY(&ifp->if_snd))
				vge_start(ifp);
		}
	}

	splx(s);
}

static int
vge_intr(void *arg)
{
	struct vge_softc *sc;
	struct ifnet *ifp;
	uint32_t status;
	int claim;

	sc = arg;
	claim = 0;
	if (sc->sc_suspended) {
		return claim;
	}

	ifp = &sc->sc_ethercom.ec_if;

	if ((ifp->if_flags & IFF_UP) == 0) {
		return claim;
	}

	/* Disable interrupts */
	CSR_WRITE_1(sc, VGE_CRC3, VGE_CR3_INT_GMSK);

	for (;;) {

		status = CSR_READ_4(sc, VGE_ISR);
		/* If the card has gone away the read returns 0xffff. */
		if (status == 0xFFFFFFFF)
			break;

		if (status) {
			claim = 1;
			CSR_WRITE_4(sc, VGE_ISR, status);
		}

		if ((status & VGE_INTRS) == 0)
			break;

		if (status & (VGE_ISR_RXOK|VGE_ISR_RXOK_HIPRIO))
			vge_rxeof(sc);

		if (status & (VGE_ISR_RXOFLOW|VGE_ISR_RXNODESC)) {
			vge_rxeof(sc);
			CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_RUN);
			CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_WAK);
		}

		if (status & (VGE_ISR_TXOK0|VGE_ISR_TIMER0))
			vge_txeof(sc);

		if (status & (VGE_ISR_TXDMA_STALL|VGE_ISR_RXDMA_STALL))
			vge_init(ifp);

		if (status & VGE_ISR_LINKSTS)
			vge_tick(sc);
	}

	/* Re-enable interrupts */
	CSR_WRITE_1(sc, VGE_CRS3, VGE_CR3_INT_GMSK);

	if (claim && !IFQ_IS_EMPTY(&ifp->if_snd))
		vge_start(ifp);

	return claim;
}

static int
vge_encap(struct vge_softc *sc, struct mbuf *m_head, int idx)
{
	struct vge_txsoft *txs;
	struct vge_txdesc *txd;
	struct vge_txfrag *f;
	struct mbuf *m_new;
	bus_dmamap_t map;
	int m_csumflags, seg, error, flags;
	struct m_tag *mtag;
	size_t sz;
	uint32_t td_sts, td_ctl;

	KASSERT(sc->sc_tx_free > 0);

	txd = &sc->sc_txdescs[idx];

#ifdef DIAGNOSTIC
	/* If this descriptor is still owned by the chip, bail. */
	VGE_TXDESCSYNC(sc, idx, 
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	td_sts = le32toh(txd->td_sts);
	VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
	if (td_sts & VGE_TDSTS_OWN) {
		return ENOBUFS;
	}
#endif

	/*
	 * Preserve m_pkthdr.csum_flags here since m_head might be
	 * updated by m_defrag()
	 */
	m_csumflags = m_head->m_pkthdr.csum_flags;

	txs = &sc->sc_txsoft[idx];
	map = txs->txs_dmamap;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m_head, BUS_DMA_NOWAIT);

	/* If too many segments to map, coalesce */
	if (error == EFBIG ||
	    (m_head->m_pkthdr.len < ETHER_PAD_LEN &&
	     map->dm_nsegs == VGE_TX_FRAGS)) {
		m_new = m_defrag(m_head, M_DONTWAIT);
		if (m_new == NULL)
			return EFBIG;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, map,
		    m_new, BUS_DMA_NOWAIT);
		if (error) {
			m_freem(m_new);
			return error;
		}

		m_head = m_new;
	} else if (error)
		return error;

	txs->txs_mbuf = m_head;

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	for (seg = 0, f = &txd->td_frag[0]; seg < map->dm_nsegs; seg++, f++) {
		f->tf_buflen = htole16(VGE_BUFLEN(map->dm_segs[seg].ds_len));
		vge_set_txaddr(f, map->dm_segs[seg].ds_addr);
	}

	/* Argh. This chip does not autopad short frames */
	sz = m_head->m_pkthdr.len;
	if (sz < ETHER_PAD_LEN) {
		f->tf_buflen = htole16(VGE_BUFLEN(ETHER_PAD_LEN - sz));
		vge_set_txaddr(f, VGE_CDPADADDR(sc));
		sz = ETHER_PAD_LEN;
		seg++;
	}
	VGE_TXFRAGSYNC(sc, idx, seg, BUS_DMASYNC_PREWRITE);

	/*
	 * When telling the chip how many segments there are, we
	 * must use nsegs + 1 instead of just nsegs. Darned if I
	 * know why.
	 */
	seg++;

	flags = 0;
	if (m_csumflags & M_CSUM_IPv4)
		flags |= VGE_TDCTL_IPCSUM;
	if (m_csumflags & M_CSUM_TCPv4)
		flags |= VGE_TDCTL_TCPCSUM;
	if (m_csumflags & M_CSUM_UDPv4)
		flags |= VGE_TDCTL_UDPCSUM;
	td_sts = sz << 16;
	td_ctl = flags | (seg << 28) | VGE_TD_LS_NORM;

	if (sz > ETHERMTU + ETHER_HDR_LEN)
		td_ctl |= VGE_TDCTL_JUMBO;

	/*
	 * Set up hardware VLAN tagging.
	 */
	mtag = VLAN_OUTPUT_TAG(&sc->sc_ethercom, m_head);
	if (mtag != NULL) {
		/* 
		 * No need htons() here since vge(4) chip assumes
		 * that tags are written in little endian and
		 * we already use htole32() here.
		 */
		td_ctl |= VLAN_TAG_VALUE(mtag) | VGE_TDCTL_VTAG;
	}
	txd->td_ctl = htole32(td_ctl);
	txd->td_sts = htole32(td_sts);
	VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	txd->td_sts = htole32(VGE_TDSTS_OWN | td_sts);
	VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	sc->sc_tx_free--;

	return 0;
}

/*
 * Main transmit routine.
 */

static void
vge_start(struct ifnet *ifp)
{
	struct vge_softc *sc;
	struct vge_txsoft *txs;
	struct mbuf *m_head;
	int idx, pidx, ofree, error;

	sc = ifp->if_softc;

	if (!sc->sc_link ||
	    (ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING) {
		return;
	}

	m_head = NULL;
	idx = sc->sc_tx_prodidx;
	pidx = VGE_PREV_TXDESC(idx);
	ofree = sc->sc_tx_free;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		/* Grab a packet off the queue. */
		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		if (sc->sc_tx_free == 0) {
			/*
			 * All slots used, stop for now.
			 */
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		txs = &sc->sc_txsoft[idx];
		KASSERT(txs->txs_mbuf == NULL);

		if ((error = vge_encap(sc, m_head, idx))) {
			if (error == EFBIG) {
				aprint_error("%s: Tx packet consumes too many "
				    "DMA segments, dropping...\n",
				    sc->sc_dev.dv_xname);
				IFQ_DEQUEUE(&ifp->if_snd, m_head);
				m_freem(m_head);
				continue;
			}

			/*
			 * Short on resources, just stop for now.
			 */
			if (error == ENOBUFS)
				ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m_head);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		sc->sc_txdescs[pidx].td_frag[0].tf_buflen |=
		    htole16(VGE_TXDESC_Q);
		VGE_TXFRAGSYNC(sc, pidx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		if (txs->txs_mbuf != m_head) {
			m_freem(m_head);
			m_head = txs->txs_mbuf;
		}

		pidx = idx;
		idx = VGE_NEXT_TXDESC(idx);

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m_head);
#endif
	}

	if (sc->sc_tx_free < ofree) {
		/* TX packet queued */

		sc->sc_tx_prodidx = idx;

		/* Issue a transmit command. */
		CSR_WRITE_2(sc, VGE_TXQCSRS, VGE_TXQCSR_WAK0);

		/*
		 * Use the countdown timer for interrupt moderation.
		 * 'TX done' interrupts are disabled. Instead, we reset the
		 * countdown timer, which will begin counting until it hits
		 * the value in the SSTIMER register, and then trigger an
		 * interrupt. Each time we set the TIMER0_ENABLE bit, the
		 * the timer count is reloaded. Only when the transmitter
		 * is idle will the timer hit 0 and an interrupt fire.
		 */
		CSR_WRITE_1(sc, VGE_CRS1, VGE_CR1_TIMER0_ENABLE);

		/*
		 * Set a timeout in case the chip goes out to lunch.
		 */
		ifp->if_timer = 5;
	}
}

static int
vge_init(struct ifnet *ifp)
{
	struct vge_softc *sc;
	int i;

	sc = ifp->if_softc;

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
	vge_stop(sc);
	vge_reset(sc);

	/* Initialize the RX descriptors and mbufs. */
	memset(sc->sc_rxdescs, 0, sizeof(sc->sc_rxdescs));
	for (i = 0; i < VGE_NRXDESC; i++) {
		if (vge_newbuf(sc, i, NULL) == ENOBUFS) {
			aprint_error("%s: unable to allocate or map "
			    "rx buffer\n", sc->sc_dev.dv_xname);
			return 1; /* XXX */
		}
	}
	sc->sc_rx_prodidx = 0;
	sc->sc_rx_consumed = 0;
	sc->sc_rx_mhead = sc->sc_rx_mtail = NULL;

	/* Initialize the  TX descriptors and mbufs. */
	memset(sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	bus_dmamap_sync(sc->sc_dmat, sc->sc_cddmamap,
	    VGE_CDTXOFF(0), sizeof(sc->sc_txdescs),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	for (i = 0; i < VGE_NTXDESC; i++)
		sc->sc_txsoft[i].txs_mbuf = NULL;

	sc->sc_tx_prodidx = 0;
	sc->sc_tx_considx = 0;
	sc->sc_tx_free = VGE_NTXDESC;

	/* Set our station address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		CSR_WRITE_1(sc, VGE_PAR0 + i, sc->sc_eaddr[i]);

	/*
	 * Set receive FIFO threshold. Also allow transmission and
	 * reception of VLAN tagged frames.
	 */
	CSR_CLRBIT_1(sc, VGE_RXCFG, VGE_RXCFG_FIFO_THR|VGE_RXCFG_VTAGOPT);
	CSR_SETBIT_1(sc, VGE_RXCFG, VGE_RXFIFOTHR_128BYTES|VGE_VTAG_OPT2);

	/* Set DMA burst length */
	CSR_CLRBIT_1(sc, VGE_DMACFG0, VGE_DMACFG0_BURSTLEN);
	CSR_SETBIT_1(sc, VGE_DMACFG0, VGE_DMABURST_128);

	CSR_SETBIT_1(sc, VGE_TXCFG, VGE_TXCFG_ARB_PRIO|VGE_TXCFG_NONBLK);

	/* Set collision backoff algorithm */
	CSR_CLRBIT_1(sc, VGE_CHIPCFG1, VGE_CHIPCFG1_CRANDOM|
	    VGE_CHIPCFG1_CAP|VGE_CHIPCFG1_MBA|VGE_CHIPCFG1_BAKOPT);
	CSR_SETBIT_1(sc, VGE_CHIPCFG1, VGE_CHIPCFG1_OFSET);

	/* Disable LPSEL field in priority resolution */
	CSR_SETBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_LPSEL_DIS);

	/*
	 * Load the addresses of the DMA queues into the chip.
	 * Note that we only use one transmit queue.
	 */

	CSR_WRITE_4(sc, VGE_TXDESC_ADDR_LO0, VGE_ADDR_LO(VGE_CDTXADDR(sc, 0)));
	CSR_WRITE_2(sc, VGE_TXDESCNUM, VGE_NTXDESC - 1);

	CSR_WRITE_4(sc, VGE_RXDESC_ADDR_LO, VGE_ADDR_LO(VGE_CDRXADDR(sc, 0)));
	CSR_WRITE_2(sc, VGE_RXDESCNUM, VGE_NRXDESC - 1);
	CSR_WRITE_2(sc, VGE_RXDESC_RESIDUECNT, VGE_NRXDESC);

	/* Enable and wake up the RX descriptor queue */
	CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_RUN);
	CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_WAK);

	/* Enable the TX descriptor queue */
	CSR_WRITE_2(sc, VGE_TXQCSRS, VGE_TXQCSR_RUN0);

	/* Set up the receive filter -- allow large frames for VLANs. */
	CSR_WRITE_1(sc, VGE_RXCTL, VGE_RXCTL_RX_UCAST|VGE_RXCTL_RX_GIANT);

	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC) {
		CSR_SETBIT_1(sc, VGE_RXCTL, VGE_RXCTL_RX_PROMISC);
	}

	/* Set capture broadcast bit to capture broadcast frames. */
	if (ifp->if_flags & IFF_BROADCAST) {
		CSR_SETBIT_1(sc, VGE_RXCTL, VGE_RXCTL_RX_BCAST);
	}

	/* Set multicast bit to capture multicast frames. */
	if (ifp->if_flags & IFF_MULTICAST) {
		CSR_SETBIT_1(sc, VGE_RXCTL, VGE_RXCTL_RX_MCAST);
	}

	/* Init the cam filter. */
	vge_cam_clear(sc);

	/* Init the multicast filter. */
	vge_setmulti(sc);

	/* Enable flow control */

	CSR_WRITE_1(sc, VGE_CRS2, 0x8B);

	/* Enable jumbo frame reception (if desired) */

	/* Start the MAC. */
	CSR_WRITE_1(sc, VGE_CRC0, VGE_CR0_STOP);
	CSR_WRITE_1(sc, VGE_CRS1, VGE_CR1_NOPOLL);
	CSR_WRITE_1(sc, VGE_CRS0,
	    VGE_CR0_TX_ENABLE|VGE_CR0_RX_ENABLE|VGE_CR0_START);

	/*
	 * Configure one-shot timer for microsecond
	 * resulution and load it for 500 usecs.
	 */
	CSR_SETBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_TIMER0_RES);
	CSR_WRITE_2(sc, VGE_SSTIMER, 400);

	/*
	 * Configure interrupt moderation for receive. Enable
	 * the holdoff counter and load it, and set the RX
	 * suppression count to the number of descriptors we
	 * want to allow before triggering an interrupt.
	 * The holdoff timer is in units of 20 usecs.
	 */

#ifdef notyet
	CSR_WRITE_1(sc, VGE_INTCTL1, VGE_INTCTL_TXINTSUP_DISABLE);
	/* Select the interrupt holdoff timer page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_INTHLDOFF);
	CSR_WRITE_1(sc, VGE_INTHOLDOFF, 10); /* ~200 usecs */

	/* Enable use of the holdoff timer. */
	CSR_WRITE_1(sc, VGE_CRS3, VGE_CR3_INT_HOLDOFF);
	CSR_WRITE_1(sc, VGE_INTCTL1, VGE_INTCTL_SC_RELOAD);

	/* Select the RX suppression threshold page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_RXSUPPTHR);
	CSR_WRITE_1(sc, VGE_RXSUPPTHR, 64); /* interrupt after 64 packets */

	/* Restore the page select bits. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_MAR);
#endif

#ifdef DEVICE_POLLING
	/*
	 * Disable interrupts if we are polling.
	 */
	if (ifp->if_flags & IFF_POLLING) {
		CSR_WRITE_4(sc, VGE_IMR, 0);
		CSR_WRITE_1(sc, VGE_CRC3, VGE_CR3_INT_GMSK);
	} else	/* otherwise ... */
#endif /* DEVICE_POLLING */
	{
	/*
	 * Enable interrupts.
	 */
		CSR_WRITE_4(sc, VGE_IMR, VGE_INTRS);
		CSR_WRITE_4(sc, VGE_ISR, 0);
		CSR_WRITE_1(sc, VGE_CRS3, VGE_CR3_INT_GMSK);
	}

	mii_mediachg(&sc->sc_mii);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	sc->sc_if_flags = 0;
	sc->sc_link = 0;

	callout_schedule(&sc->sc_timeout, hz);

	return 0;
}

/*
 * Set media options.
 */
static int
vge_ifmedia_upd(struct ifnet *ifp)
{
	struct vge_softc *sc;

	sc = ifp->if_softc;
	mii_mediachg(&sc->sc_mii);

	return 0;
}

/*
 * Report current media status.
 */
static void
vge_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct vge_softc *sc;
	struct mii_data *mii;

	sc = ifp->if_softc;
	mii = &sc->sc_mii;

	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

static void
vge_miibus_statchg(struct device *self)
{
	struct vge_softc *sc;
	struct mii_data *mii;
	struct ifmedia_entry *ife;

	sc = (void *)self;
	mii = &sc->sc_mii;
	ife = mii->mii_media.ifm_cur;
	/*
	 * If the user manually selects a media mode, we need to turn
	 * on the forced MAC mode bit in the DIAGCTL register. If the
	 * user happens to choose a full duplex mode, we also need to
	 * set the 'force full duplex' bit. This applies only to
	 * 10Mbps and 100Mbps speeds. In autoselect mode, forced MAC
	 * mode is disabled, and in 1000baseT mode, full duplex is
	 * always implied, so we turn on the forced mode bit but leave
	 * the FDX bit cleared.
	 */

	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		CSR_CLRBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_MACFORCE);
		CSR_CLRBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_FDXFORCE);
		break;
	case IFM_1000_T:
		CSR_SETBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_MACFORCE);
		CSR_CLRBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_FDXFORCE);
		break;
	case IFM_100_TX:
	case IFM_10_T:
		CSR_SETBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_MACFORCE);
		if ((ife->ifm_media & IFM_GMASK) == IFM_FDX) {
			CSR_SETBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_FDXFORCE);
		} else {
			CSR_CLRBIT_1(sc, VGE_DIAGCTL, VGE_DIAGCTL_FDXFORCE);
		}
		break;
	default:
		aprint_error("%s: unknown media type: %x\n",
		    sc->sc_dev.dv_xname,
		    IFM_SUBTYPE(ife->ifm_media));
		break;
	}
}

static int
vge_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct vge_softc *sc;
	struct ifreq *ifr;
	struct mii_data *mii;
	int s, error;

	sc = ifp->if_softc;
	ifr = (struct ifreq *)data;
	error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > VGE_JUMBO_MTU)
			error = EINVAL;
		ifp->if_mtu = ifr->ifr_mtu;
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING &&
			    ifp->if_flags & IFF_PROMISC &&
			    (sc->sc_if_flags & IFF_PROMISC) == 0) {
				CSR_SETBIT_1(sc, VGE_RXCTL,
				    VGE_RXCTL_RX_PROMISC);
				vge_setmulti(sc);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    (ifp->if_flags & IFF_PROMISC) == 0 &&
			    sc->sc_if_flags & IFF_PROMISC) {
				CSR_CLRBIT_1(sc, VGE_RXCTL,
				    VGE_RXCTL_RX_PROMISC);
				vge_setmulti(sc);
                        } else
				vge_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				vge_stop(sc);
		}
		sc->sc_if_flags = ifp->if_flags;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING)
				vge_setmulti(sc);
			error = 0;
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		mii = &sc->sc_mii;
		error = ifmedia_ioctl(ifp, ifr, &mii->mii_media, command);
		break;
	default:
		error = ether_ioctl(ifp, command, data);
		break;
	}

	splx(s);
	return error;
}

static void
vge_watchdog(struct ifnet *ifp)
{
	struct vge_softc *sc;
	int s;

	sc = ifp->if_softc;
	s = splnet();
	aprint_error("%s: watchdog timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;

	vge_txeof(sc);
	vge_rxeof(sc);

	vge_init(ifp);

	splx(s);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
vge_stop(struct vge_softc *sc)
{
	struct ifnet *ifp;
	struct vge_txsoft *txs;
	struct vge_rxsoft *rxs;
	int i, s;

	ifp = &sc->sc_ethercom.ec_if;

	s = splnet();
	ifp->if_timer = 0;

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
#ifdef DEVICE_POLLING
	ether_poll_deregister(ifp);
#endif /* DEVICE_POLLING */

	CSR_WRITE_1(sc, VGE_CRC3, VGE_CR3_INT_GMSK);
	CSR_WRITE_1(sc, VGE_CRS0, VGE_CR0_STOP);
	CSR_WRITE_4(sc, VGE_ISR, 0xFFFFFFFF);
	CSR_WRITE_2(sc, VGE_TXQCSRC, 0xFFFF);
	CSR_WRITE_1(sc, VGE_RXQCSRC, 0xFF);
	CSR_WRITE_4(sc, VGE_RXDESC_ADDR_LO, 0);

	if (sc->sc_rx_mhead != NULL) {
		m_freem(sc->sc_rx_mhead);
		sc->sc_rx_mhead = sc->sc_rx_mtail = NULL;
	}

	/* Free the TX list buffers. */

	for (i = 0; i < VGE_NTXDESC; i++) {
		txs = &sc->sc_txsoft[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}

	/* Free the RX list buffers. */

	for (i = 0; i < VGE_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}

	splx(s);
}

#if VGE_POWER_MANAGEMENT
/*
 * Device suspend routine.  Stop the interface and save some PCI
 * settings in case the BIOS doesn't restore them properly on
 * resume.
 */
static int
vge_suspend(struct device *dev)
{
	struct vge_softc *sc;
	int i;

	sc = device_get_softc(dev);

	vge_stop(sc);

        for (i = 0; i < 5; i++)
		sc->sc_saved_maps[i] =
		    pci_read_config(dev, PCIR_MAPS + i * 4, 4);
	sc->sc_saved_biosaddr = pci_read_config(dev, PCIR_BIOS, 4);
	sc->sc_saved_intline = pci_read_config(dev, PCIR_INTLINE, 1);
	sc->sc_saved_cachelnsz = pci_read_config(dev, PCIR_CACHELNSZ, 1);
	sc->sc_saved_lattimer = pci_read_config(dev, PCIR_LATTIMER, 1);

	sc->suspended = 1;

	return 0;
}

/*
 * Device resume routine.  Restore some PCI settings in case the BIOS
 * doesn't, re-enable busmastering, and restart the interface if
 * appropriate.
 */
static int
vge_resume(struct device *dev)
{
	struct vge_softc *sc;
	struct ifnet *ifp;
	int i;

	sc = (void *)dev;
	ifp = &sc->sc_ethercom.ec_if;

        /* better way to do this? */
	for (i = 0; i < 5; i++)
		pci_write_config(dev, PCIR_MAPS + i * 4,
		    sc->sc_saved_maps[i], 4);
	pci_write_config(dev, PCIR_BIOS, sc->sc_saved_biosaddr, 4);
	pci_write_config(dev, PCIR_INTLINE, sc->sc_saved_intline, 1);
	pci_write_config(dev, PCIR_CACHELNSZ, sc->sc_saved_cachelnsz, 1);
	pci_write_config(dev, PCIR_LATTIMER, sc->sc_saved_lattimer, 1);

	/* reenable busmastering */
	pci_enable_busmaster(dev);
	pci_enable_io(dev, SYS_RES_MEMORY);

	/* reinitialize interface if necessary */
	if (ifp->if_flags & IFF_UP)
		vge_init(sc);

	sc->suspended = 0;

	return 0;
}
#endif

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static void
vge_shutdown(void *arg)
{
	struct vge_softc *sc;

	sc = arg;
	vge_stop(sc);
}
