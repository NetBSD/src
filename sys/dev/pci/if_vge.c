/* $NetBSD: if_vge.c,v 1.15 2006/10/14 19:53:51 tsutsui Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: if_vge.c,v 1.15 2006/10/14 19:53:51 tsutsui Exp $");

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
#include <dev/pci/if_vgevar.h>

static int vge_probe(struct device *, struct cfdata *, void *);
static void vge_attach(struct device *, struct device *, void *);

static int vge_encap(struct vge_softc *, struct mbuf *, int);

static int vge_allocmem(struct vge_softc *);
static int vge_newbuf(struct vge_softc *, int, struct mbuf *);
static int vge_rx_list_init(struct vge_softc *);
static int vge_tx_list_init(struct vge_softc *);
#ifndef __NO_STRICT_ALIGNMENT
static inline void vge_fixup_rx(struct mbuf *);
#endif
static void vge_rxeof(struct vge_softc *);
static void vge_txeof(struct vge_softc *);
static int vge_intr(void *);
static void vge_tick(void *);
static void vge_start(struct ifnet *);
static int vge_ioctl(struct ifnet *, u_long, caddr_t);
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

#define VGE_PCI_LOIO             0x10
#define VGE_PCI_LOMEM            0x14

CFATTACH_DECL(vge, sizeof(struct vge_softc),
    vge_probe, vge_attach, NULL, NULL);

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
		     mtod(mn, caddr_t));

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
		printf("%s: EEPROM read timed out\n", sc->sc_dev.dv_xname);
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
		printf("%s: failed to idle MII autopoll\n",
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
		printf("%s: failed to idle MII autopoll\n",
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
		printf("%s: failed to start MII autopoll\n",
		    sc->sc_dev.dv_xname);
	}
}

static int
vge_miibus_readreg(struct device *dev, int phy, int reg)
{
	struct vge_softc *sc;
	int i;
	uint16_t rval;

	sc = (void *)dev;
	rval = 0;
	if (phy != (CSR_READ_1(sc, VGE_MIICFG) & 0x1F))
		return 0;

	VGE_LOCK(sc);
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
		printf("%s: MII read timed out\n", sc->sc_dev.dv_xname);
	else
		rval = CSR_READ_2(sc, VGE_MIIDATA);

	vge_miipoll_start(sc);
	VGE_UNLOCK(sc);

	return rval;
}

static void
vge_miibus_writereg(struct device *dev, int phy, int reg, int data)
{
	struct vge_softc *sc;
	int i;

	sc = (void *)dev;
	if (phy != (CSR_READ_1(sc, VGE_MIICFG) & 0x1F))
		return;

	VGE_LOCK(sc);
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
		printf("%s: MII write timed out\n", sc->sc_dev.dv_xname);
	}

	vge_miipoll_start(sc);
	VGE_UNLOCK(sc);
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

	sc->vge_camidx = 0;
}

static int
vge_cam_set(struct vge_softc *sc, uint8_t *addr)
{
	int i, error;

	error = 0;

	if (sc->vge_camidx == VGE_CAM_MAXADDRS)
		return ENOSPC;

	/* Select the CAM data page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_CAMDATA);

	/* Set the filter entry we want to update and enable writing. */
	CSR_WRITE_1(sc, VGE_CAMADDR, VGE_CAMADDR_ENABLE|sc->vge_camidx);

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
		printf("%s: setting CAM filter failed\n", sc->sc_dev.dv_xname);
		error = EIO;
		goto fail;
	}

	/* Select the CAM mask page. */
	CSR_CLRBIT_1(sc, VGE_CAMCTL, VGE_CAMCTL_PAGESEL);
	CSR_SETBIT_1(sc, VGE_CAMCTL, VGE_PAGESEL_CAMMASK);

	/* Set the mask bit that enables this filter. */
	CSR_SETBIT_1(sc, VGE_CAM0 + (sc->vge_camidx / 8),
	    1 << (sc->vge_camidx & 7));

	sc->vge_camidx++;

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
 * we use the hash filter insted.
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
		printf("%s: soft reset timed out", sc->sc_dev.dv_xname);
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
		printf("%s: EEPROM reload timed out\n", sc->sc_dev.dv_xname);
		return;
	}

	CSR_CLRBIT_1(sc, VGE_CHIPCFG0, VGE_CHIPCFG0_PACPI);
}

/*
 * Probe for a VIA gigabit chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
vge_probe(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
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
	 * Allocate map for TX descriptor list.
	 */
	error = bus_dmamap_create(sc->vge_dmat,
	    VGE_TX_LIST_SZ, 1, VGE_TX_LIST_SZ, 0, BUS_DMA_NOWAIT,
	    &sc->vge_ldata.vge_tx_list_map);
	if (error) {
		printf("%s: could not allocate TX dma list map\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/*
	 * Allocate memory for TX descriptor list.
	 */

	error = bus_dmamem_alloc(sc->vge_dmat, VGE_TX_LIST_SZ, VGE_RING_ALIGN,
	    0, &seg, 1, &nseg, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: could not allocate TX ring dma memory\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/* Map the memory to kernel VA space */

	error = bus_dmamem_map(sc->vge_dmat, &seg, nseg, VGE_TX_LIST_SZ,
	     (caddr_t *)&sc->vge_ldata.vge_tx_list, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: could not map TX ring dma memory\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/* Load the map for the TX ring. */
	error = bus_dmamap_load(sc->vge_dmat, sc->vge_ldata.vge_tx_list_map,
	    sc->vge_ldata.vge_tx_list, VGE_TX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: could not load TX ring dma memory\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/* Create DMA maps for TX buffers */

	for (i = 0; i < VGE_TX_DESC_CNT; i++) {
		error = bus_dmamap_create(sc->vge_dmat, VGE_TX_MAXLEN,
		    VGE_TX_FRAGS, VGE_TX_MAXLEN, 0,
		    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
		    &sc->vge_ldata.vge_tx_dmamap[i]);
		if (error) {
			printf("%s: can't create DMA map for TX\n",
			    sc->sc_dev.dv_xname);
			return ENOMEM;
		}
	}

	/*
	 * Allocate map for RX descriptor list.
	 */
	error = bus_dmamap_create(sc->vge_dmat,
	    VGE_RX_LIST_SZ, 1, VGE_RX_LIST_SZ, 0, BUS_DMA_NOWAIT,
	    &sc->vge_ldata.vge_rx_list_map);
	if (error) {
		printf("%s: could not allocate RX dma list map\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/* Allocate DMA'able memory for the RX ring */

	error = bus_dmamem_alloc(sc->vge_dmat, VGE_RX_LIST_SZ, VGE_RING_ALIGN,
	    0, &seg, 1, &nseg, BUS_DMA_NOWAIT);
	if (error)
		return ENOMEM;

	/* Map the memory to kernel VA space */

	error = bus_dmamem_map(sc->vge_dmat, &seg, nseg, VGE_RX_LIST_SZ,
	     (caddr_t *)&sc->vge_ldata.vge_rx_list, BUS_DMA_NOWAIT);
	if (error)
		return ENOMEM;

	/* Load the map for the RX ring. */
	error = bus_dmamap_load(sc->vge_dmat, sc->vge_ldata.vge_rx_list_map,
	    sc->vge_ldata.vge_rx_list, VGE_RX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: could not load RX ring dma memory\n",
		    sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	/* Create DMA maps for RX buffers */

	for (i = 0; i < VGE_RX_DESC_CNT; i++) {
		error = bus_dmamap_create(sc->vge_dmat, MCLBYTES,
		    1, MCLBYTES, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
		    &sc->vge_ldata.vge_rx_dmamap[i]);
		if (error) {
			printf("%s: can't create DMA map for RX\n",
			     sc->sc_dev.dv_xname);
			return ENOMEM;
		}
	}

	return 0;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
vge_attach(struct device *parent __unused, struct device *self, void *aux)
{
	uint8_t	*eaddr;
	struct vge_softc *sc = (struct vge_softc *)self;
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
	    &sc->vge_btag, &sc->vge_bhandle, NULL, NULL) != 0) {
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
	sc->vge_intrhand = pci_intr_establish(pc, ih, IPL_NET, vge_intr, sc);
	if (sc->vge_intrhand == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* Reset the adapter. */
	vge_reset(sc);

	/*
	 * Get station address from the EEPROM.
	 */
	eaddr = sc->vge_eaddr;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 0);
	eaddr[0] = val & 0xff;
	eaddr[1] = val >> 8;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 1);
	eaddr[2] = val & 0xff;
	eaddr[3] = val >> 8;
	val = vge_read_eeprom(sc, VGE_EE_EADDR + 2);
	eaddr[4] = val & 0xff;
	eaddr[5] = val >> 8;

	printf("%s: Ethernet address: %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(eaddr));

	/*
	 * Use the 32bit tag. Hardware supports 48bit physical addresses,
	 * but we don't use that for now.
	 */
	sc->vge_dmat = pa->pa_dmat;

	if (vge_allocmem(sc))
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

	callout_init(&sc->vge_timeout);
	callout_setfunc(&sc->vge_timeout, vge_tick, sc);

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	if (shutdownhook_establish(vge_shutdown, sc) == NULL) {
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	}
}

static int
vge_newbuf(struct vge_softc *sc, int idx, struct mbuf *m)
{
	struct vge_rx_desc *d;
	struct mbuf *m_new;
	bus_dmamap_t map;
	int i;

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


#ifndef __NO_STRICT_ALIGNMENT
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
	m->m_len = m->m_pkthdr.len = MCLBYTES - VGE_ETHER_ALIGN;
	m_adj(m, VGE_ETHER_ALIGN);
#else
	m->m_len = m->m_pkthdr.len = MCLBYTES;
#endif
	map = sc->vge_ldata.vge_rx_dmamap[idx];

	if (bus_dmamap_load_mbuf(sc->vge_dmat, map, m, BUS_DMA_NOWAIT) != 0)
		goto out;

	d = &sc->vge_ldata.vge_rx_list[idx];

	/* If this descriptor is still owned by the chip, bail. */

	VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	if (le32toh(d->vge_sts) & VGE_RDSTS_OWN) {
		printf("%s: tried to map busy descriptor\n",
		    sc->sc_dev.dv_xname);
		VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
		goto out;
	}

	d->vge_buflen =
	    htole16(VGE_BUFLEN(map->dm_segs[0].ds_len) | VGE_RXDESC_I);
	d->vge_addrlo = htole32(VGE_ADDR_LO(map->dm_segs[0].ds_addr));
	d->vge_addrhi = htole16(VGE_ADDR_HI(map->dm_segs[0].ds_addr) & 0xFFFF);
	d->vge_sts = 0;
	d->vge_ctl = 0;
	VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(sc->vge_dmat,
	    sc->vge_ldata.vge_rx_dmamap[idx],
	    0, sc->vge_ldata.vge_rx_dmamap[idx]->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	/*
	 * Note: the manual fails to document the fact that for
	 * proper opration, the driver needs to replentish the RX
	 * DMA ring 4 descriptors at a time (rather than one at a
	 * time, like most chips). We can allocate the new buffers
	 * but we should not set the OWN bits until we're ready
	 * to hand back 4 of them in one shot.
	 */

#define VGE_RXCHUNK 4
	sc->vge_rx_consumed++;
	if (sc->vge_rx_consumed == VGE_RXCHUNK) {
		for (i = idx; i != idx - sc->vge_rx_consumed; i--) {
			sc->vge_ldata.vge_rx_list[i].vge_sts |=
			    htole32(VGE_RDSTS_OWN);
			VGE_RXDESCSYNC(sc, i,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		}
		sc->vge_rx_consumed = 0;
	}

	sc->vge_ldata.vge_rx_mbuf[idx] = m;

	return 0;
 out:
	if (m_new != NULL)
		m_freem(m_new);
	return ENOMEM;
}

static int
vge_tx_list_init(struct vge_softc *sc)
{

	memset((char *)sc->vge_ldata.vge_tx_list, 0, VGE_TX_LIST_SZ);
	bus_dmamap_sync(sc->vge_dmat, sc->vge_ldata.vge_tx_list_map,
	    0, sc->vge_ldata.vge_tx_list_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	memset((char *)&sc->vge_ldata.vge_tx_mbuf, 0,
	    (VGE_TX_DESC_CNT * sizeof(struct mbuf *)));

	sc->vge_ldata.vge_tx_prodidx = 0;
	sc->vge_ldata.vge_tx_considx = 0;
	sc->vge_ldata.vge_tx_free = VGE_TX_DESC_CNT;

	return 0;
}

static int
vge_rx_list_init(struct vge_softc *sc)
{
	int i;

	memset((char *)sc->vge_ldata.vge_rx_list, 0, VGE_RX_LIST_SZ);
	memset((char *)&sc->vge_ldata.vge_rx_mbuf, 0,
	    (VGE_RX_DESC_CNT * sizeof(struct mbuf *)));

	sc->vge_rx_consumed = 0;

	for (i = 0; i < VGE_RX_DESC_CNT; i++) {
		if (vge_newbuf(sc, i, NULL) == ENOBUFS)
			return (ENOBUFS);
	}

	sc->vge_ldata.vge_rx_prodidx = 0;
	sc->vge_rx_consumed = 0;
	sc->vge_head = sc->vge_tail = NULL;

	return 0;
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
	struct vge_rx_desc *cur_rx;
	uint32_t rxstat, rxctl;

	VGE_LOCK_ASSERT(sc);
	ifp = &sc->sc_ethercom.ec_if;
	idx = sc->vge_ldata.vge_rx_prodidx;
	lim = 0;

	/* Invalidate the descriptor memory */

	for (;;) {
		cur_rx = &sc->vge_ldata.vge_rx_list[idx];

		VGE_RXDESCSYNC(sc, idx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		rxstat = le32toh(cur_rx->vge_sts);
		if ((rxstat & VGE_RDSTS_OWN) != 0) {
			VGE_RXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
			break;
		}

#ifdef DEVICE_POLLING
		if (ifp->if_flags & IFF_POLLING) {
			if (sc->rxcycles <= 0)
				break;
			sc->rxcycles--;
		}
#endif /* DEVICE_POLLING */

		m = sc->vge_ldata.vge_rx_mbuf[idx];
		total_len = (rxstat & VGE_RDSTS_BUFSIZ) >> 16;
		rxctl = le32toh(cur_rx->vge_ctl);

		/* Invalidate the RX mbuf and unload its map */

		bus_dmamap_sync(sc->vge_dmat,
		    sc->vge_ldata.vge_rx_dmamap[idx],
		    0, sc->vge_ldata.vge_rx_dmamap[idx]->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->vge_dmat,
		    sc->vge_ldata.vge_rx_dmamap[idx]);

		/*
		 * If the 'start of frame' bit is set, this indicates
		 * either the first fragment in a multi-fragment receive,
		 * or an intermediate fragment. Either way, we want to
		 * accumulate the buffers.
		 */
		if (rxstat & VGE_RXPKT_SOF) {
			m->m_len = MCLBYTES - VGE_ETHER_ALIGN;
			if (sc->vge_head == NULL)
				sc->vge_head = sc->vge_tail = m;
			else {
				m->m_flags &= ~M_PKTHDR;
				sc->vge_tail->m_next = m;
				sc->vge_tail = m;
			}
			vge_newbuf(sc, idx, NULL);
			VGE_RX_DESC_INC(idx);
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
		if (!(rxstat & VGE_RDSTS_RXOK) && !(rxstat & VGE_RDSTS_VIDM)
		    && !(rxstat & VGE_RDSTS_CSUMERR)) {
			ifp->if_ierrors++;
			/*
			 * If this is part of a multi-fragment packet,
			 * discard all the pieces.
			 */
			if (sc->vge_head != NULL) {
				m_freem(sc->vge_head);
				sc->vge_head = sc->vge_tail = NULL;
			}
			vge_newbuf(sc, idx, m);
			VGE_RX_DESC_INC(idx);
			continue;
		}

		/*
		 * If allocating a replacement mbuf fails,
		 * reload the current one.
		 */

		if (vge_newbuf(sc, idx, NULL)) {
			ifp->if_ierrors++;
			if (sc->vge_head != NULL) {
				m_freem(sc->vge_head);
				sc->vge_head = sc->vge_tail = NULL;
			}
			vge_newbuf(sc, idx, m);
			VGE_RX_DESC_INC(idx);
			continue;
		}

		VGE_RX_DESC_INC(idx);

		if (sc->vge_head != NULL) {
			m->m_len = total_len % (MCLBYTES - VGE_ETHER_ALIGN);
			/*
			 * Special case: if there's 4 bytes or less
			 * in this buffer, the mbuf can be discarded:
			 * the last 4 bytes is the CRC, which we don't
			 * care about anyway.
			 */
			if (m->m_len <= ETHER_CRC_LEN) {
				sc->vge_tail->m_len -=
				    (ETHER_CRC_LEN - m->m_len);
				m_freem(m);
			} else {
				m->m_len -= ETHER_CRC_LEN;
				m->m_flags &= ~M_PKTHDR;
				sc->vge_tail->m_next = m;
			}
			m = sc->vge_head;
			sc->vge_head = sc->vge_tail = NULL;
			m->m_pkthdr.len = total_len - ETHER_CRC_LEN;
		} else
			m->m_pkthdr.len = m->m_len =
			    (total_len - ETHER_CRC_LEN);

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

		if (rxstat & VGE_RDSTS_VTAG)
			VLAN_INPUT_TAG(ifp, m,
			    ntohs((rxctl & VGE_RDCTL_VLANID)), continue);

#if NBPFILTER > 0
		/*
		 * Handle BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		VGE_UNLOCK(sc);
		(*ifp->if_input)(ifp, m);
		VGE_LOCK(sc);

		lim++;
		if (lim == VGE_RX_DESC_CNT)
			break;

	}

	sc->vge_ldata.vge_rx_prodidx = idx;
	CSR_WRITE_2(sc, VGE_RXDESC_RESIDUECNT, lim);
}

static void
vge_txeof(struct vge_softc *sc)
{
	struct ifnet *ifp;
	uint32_t txstat;
	int idx;

	ifp = &sc->sc_ethercom.ec_if;
	idx = sc->vge_ldata.vge_tx_considx;

	while (idx != sc->vge_ldata.vge_tx_prodidx) {
		VGE_TXDESCSYNC(sc, idx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		txstat = le32toh(sc->vge_ldata.vge_tx_list[idx].vge_sts);
		if (txstat & VGE_TDSTS_OWN) {
			VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
			break;
		}

		m_freem(sc->vge_ldata.vge_tx_mbuf[idx]);
		sc->vge_ldata.vge_tx_mbuf[idx] = NULL;
		bus_dmamap_unload(sc->vge_dmat,
		    sc->vge_ldata.vge_tx_dmamap[idx]);
		if (txstat & (VGE_TDSTS_EXCESSCOLL|VGE_TDSTS_COLL))
			ifp->if_collisions++;
		if (txstat & VGE_TDSTS_TXERR)
			ifp->if_oerrors++;
		else
			ifp->if_opackets++;

		sc->vge_ldata.vge_tx_free++;
		VGE_TX_DESC_INC(idx);
	}

	/* No changes made to the TX ring, so no flush needed */

	if (idx != sc->vge_ldata.vge_tx_considx) {
		sc->vge_ldata.vge_tx_considx = idx;
		ifp->if_flags &= ~IFF_OACTIVE;
		ifp->if_timer = 0;
	}

	/*
	 * If not all descriptors have been released reaped yet,
	 * reload the timer so that we will eventually get another
	 * interrupt that will cause us to re-enter this routine.
	 * This is done in case the transmitter has gone idle.
	 */
	if (sc->vge_ldata.vge_tx_free != VGE_TX_DESC_CNT) {
		CSR_WRITE_1(sc, VGE_CRS1, VGE_CR1_TIMER0_ENABLE);
	}
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

	VGE_LOCK(sc);

	callout_schedule(&sc->vge_timeout, hz);

	mii_tick(mii);
	if (sc->vge_link) {
		if (!(mii->mii_media_status & IFM_ACTIVE))
			sc->vge_link = 0;
	} else {
		if (mii->mii_media_status & IFM_ACTIVE &&
		    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
			sc->vge_link = 1;
			if (!IFQ_IS_EMPTY(&ifp->if_snd))
				vge_start(ifp);
		}
	}

	VGE_UNLOCK(sc);

	splx(s);
}

#ifdef DEVICE_POLLING
static void
vge_poll(struct ifnet *ifp, enum poll_cmd cmd, int count)
{
	struct vge_softc *sc = ifp->if_softc;

	VGE_LOCK(sc);
#ifdef IFCAP_POLLING
	if (!(ifp->if_capenable & IFCAP_POLLING)) {
		ether_poll_deregister(ifp);
		cmd = POLL_DEREGISTER;
	}
#endif
	if (cmd == POLL_DEREGISTER) { /* final call, enable interrupts */
		CSR_WRITE_4(sc, VGE_IMR, VGE_INTRS);
		CSR_WRITE_4(sc, VGE_ISR, 0xFFFFFFFF);
		CSR_WRITE_1(sc, VGE_CRS3, VGE_CR3_INT_GMSK);
		goto done;
	}

	sc->rxcycles = count;
	vge_rxeof(sc);
	vge_txeof(sc);

#if __FreeBSD_version < 502114
	if (ifp->if_snd.ifq_head != NULL)
#else
	if (!IFQ_DRV_IS_EMPTY(&ifp->if_snd))
#endif
		taskqueue_enqueue(taskqueue_swi, &sc->vge_txtask);

	if (cmd == POLL_AND_CHECK_STATUS) { /* also check status register */
		uint32_t status;
		status = CSR_READ_4(sc, VGE_ISR);
		if (status == 0xFFFFFFFF)
			goto done;
		if (status)
			CSR_WRITE_4(sc, VGE_ISR, status);

		/*
		 * XXX check behaviour on receiver stalls.
		 */

		if (status & VGE_ISR_TXDMA_STALL ||
		    status & VGE_ISR_RXDMA_STALL)
			vge_init(sc);

		if (status & (VGE_ISR_RXOFLOW|VGE_ISR_RXNODESC)) {
			vge_rxeof(sc);
			ifp->if_ierrors++;
			CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_RUN);
			CSR_WRITE_1(sc, VGE_RXQCSRS, VGE_RXQCSR_WAK);
		}
	}
 done:
	VGE_UNLOCK(sc);
}
#endif /* DEVICE_POLLING */

static int
vge_intr(void *arg)
{
	struct vge_softc *sc;
	struct ifnet *ifp;
	uint32_t status;
	int claim;

	sc = arg;
	claim = 0;
	if (sc->suspended) {
		return claim;
	}

	ifp = &sc->sc_ethercom.ec_if;

	VGE_LOCK(sc);

	if (!(ifp->if_flags & IFF_UP)) {
		VGE_UNLOCK(sc);
		return claim;
	}

#ifdef DEVICE_POLLING
	if  (ifp->if_flags & IFF_POLLING)
		goto done;
	if (
#ifdef IFCAP_POLLING
	    (ifp->if_capenable & IFCAP_POLLING) &&
#endif
	    ether_poll_register(vge_poll, ifp)) { /* ok, disable interrupts */
		CSR_WRITE_4(sc, VGE_IMR, 0);
		CSR_WRITE_1(sc, VGE_CRC3, VGE_CR3_INT_GMSK);
		vge_poll(ifp, 0, 1);
		goto done;
	}

#endif /* DEVICE_POLLING */

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

#ifdef DEVICE_POLLING
 done:
#endif
	VGE_UNLOCK(sc);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		vge_start(ifp);

	return claim;
}

static int
vge_encap(struct vge_softc *sc, struct mbuf *m_head, int idx)
{
	struct vge_tx_desc *d;
	struct vge_tx_frag *f;
	struct mbuf *m_new;
	bus_dmamap_t map;
	int seg, error, flags;
	struct m_tag *mtag;
	size_t sz;

	d = &sc->vge_ldata.vge_tx_list[idx];

	/* If this descriptor is still owned by the chip, bail. */
	if (sc->vge_ldata.vge_tx_free <= 2) {
		VGE_TXDESCSYNC(sc, idx, 
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		if (le32toh(d->vge_sts) & VGE_TDSTS_OWN) {
			VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD);
			return ENOBUFS;
		}
	}

	map = sc->vge_ldata.vge_tx_dmamap[idx];
	error = bus_dmamap_load_mbuf(sc->vge_dmat, map, m_head, BUS_DMA_NOWAIT);

	/* If too many segments to map, coalesce */
	if (error == EFBIG) {
		m_new = m_defrag(m_head, M_DONTWAIT);
		if (m_new == NULL)
			return (error);

		error = bus_dmamap_load_mbuf(sc->vge_dmat, map,
		    m_new, BUS_DMA_NOWAIT);
		if (error) {
			m_freem(m_new);
			return error;
		}

		m_head = m_new;
	} else if (error)
		return error;

	for (seg = 0, f = &d->vge_frag[0]; seg < map->dm_nsegs; seg++, f++) {
		f->vge_buflen = htole16(VGE_BUFLEN(map->dm_segs[seg].ds_len));
		f->vge_addrlo = htole32(VGE_ADDR_LO(map->dm_segs[seg].ds_addr));
		f->vge_addrhi = htole16(VGE_ADDR_HI(map->dm_segs[seg].ds_addr));
	}

	/* Argh. This chip does not autopad short frames */

	sz = m_head->m_pkthdr.len;
	if (m_head->m_pkthdr.len < VGE_MIN_FRAMELEN) {
		f->vge_buflen = htole16(VGE_BUFLEN(VGE_MIN_FRAMELEN - sz));
		f->vge_addrlo = htole32(VGE_ADDR_LO(map->dm_segs[0].ds_addr));
		f->vge_addrhi =
		    htole16(VGE_ADDR_HI(map->dm_segs[0].ds_addr) & 0xFFFF);
		sz = VGE_MIN_FRAMELEN;
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
	if (m_head->m_pkthdr.csum_flags & M_CSUM_IPv4)
		flags |= VGE_TDCTL_IPCSUM;
	if (m_head->m_pkthdr.csum_flags & M_CSUM_TCPv4)
		flags |= VGE_TDCTL_TCPCSUM;
	if (m_head->m_pkthdr.csum_flags & M_CSUM_UDPv4)
		flags |= VGE_TDCTL_UDPCSUM;
	d->vge_sts = htole32(sz << 16);
	d->vge_ctl = htole32(flags | (seg << 28) | VGE_TD_LS_NORM);

	if (sz > ETHERMTU + ETHER_HDR_LEN)
		d->vge_ctl |= htole32(VGE_TDCTL_JUMBO);

	bus_dmamap_sync(sc->vge_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	sc->vge_ldata.vge_tx_mbuf[idx] = m_head;
	sc->vge_ldata.vge_tx_free--;

	/*
	 * Set up hardware VLAN tagging.
	 */

	mtag = VLAN_OUTPUT_TAG(&sc->sc_ethercom, m_head);
	if (mtag != NULL)
		d->vge_ctl |=
		    htole32(htons(VLAN_TAG_VALUE(mtag)) | VGE_TDCTL_VTAG);

	d->vge_sts |= htole32(VGE_TDSTS_OWN);

	VGE_TXDESCSYNC(sc, idx, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	return 0;
}

/*
 * Main transmit routine.
 */

static void
vge_start(struct ifnet *ifp)
{
	struct vge_softc *sc;
	struct mbuf *m_head;
	int idx, pidx, error;

	sc = ifp->if_softc;
	VGE_LOCK(sc);

	if (!sc->vge_link ||
	    (ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING) {
		VGE_UNLOCK(sc);
		return;
	}

	m_head = NULL;
	idx = sc->vge_ldata.vge_tx_prodidx;

	pidx = idx - 1;
	if (pidx < 0)
		pidx = VGE_TX_DESC_CNT - 1;

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

		if (sc->vge_ldata.vge_tx_mbuf[idx] != NULL) {
			/*
			 * Slot already used, stop for now.
			 */
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		if ((error = vge_encap(sc, m_head, idx))) {
			if (error == EFBIG) {
				printf("%s: Tx packet consumes too many "
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

		sc->vge_ldata.vge_tx_list[pidx].vge_frag[0].vge_buflen |=
		    htole16(VGE_TXDESC_Q);
		VGE_TXDESCSYNC(sc, pidx,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		if (sc->vge_ldata.vge_tx_mbuf[idx] != m_head) {
			m_freem(m_head);
			m_head = sc->vge_ldata.vge_tx_mbuf[idx];
		}

		pidx = idx;
		VGE_TX_DESC_INC(idx);

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m_head);
#endif
	}

	if (idx == sc->vge_ldata.vge_tx_prodidx) {
		VGE_UNLOCK(sc);
		return;
	}

	/* Issue a transmit command. */
	CSR_WRITE_2(sc, VGE_TXQCSRS, VGE_TXQCSR_WAK0);

	sc->vge_ldata.vge_tx_prodidx = idx;

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

	VGE_UNLOCK(sc);

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

static int
vge_init(struct ifnet *ifp)
{
	struct vge_softc *sc;
	int i;

	sc = ifp->if_softc;

	VGE_LOCK(sc);

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
	vge_stop(sc);
	vge_reset(sc);

	/*
	 * Initialize the RX and TX descriptors and mbufs.
	 */

	vge_rx_list_init(sc);
	vge_tx_list_init(sc);

	/* Set our station address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		CSR_WRITE_1(sc, VGE_PAR0 + i, sc->vge_eaddr[i]);

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

	CSR_WRITE_4(sc, VGE_TXDESC_ADDR_LO0,
	    VGE_ADDR_LO(sc->vge_ldata.vge_tx_list_map->dm_segs[0].ds_addr));
	CSR_WRITE_2(sc, VGE_TXDESCNUM, VGE_TX_DESC_CNT - 1);

	CSR_WRITE_4(sc, VGE_RXDESC_ADDR_LO,
	    VGE_ADDR_LO(sc->vge_ldata.vge_rx_list_map->dm_segs[0].ds_addr));
	CSR_WRITE_2(sc, VGE_RXDESCNUM, VGE_RX_DESC_CNT - 1);
	CSR_WRITE_2(sc, VGE_RXDESC_RESIDUECNT, VGE_RX_DESC_CNT);

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

	sc->vge_if_flags = 0;
	sc->vge_link = 0;

	VGE_UNLOCK(sc);

	callout_schedule(&sc->vge_timeout, hz);

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
		printf("%s: unknown media type: %x\n",
		    sc->sc_dev.dv_xname,
		    IFM_SUBTYPE(ife->ifm_media));
		break;
	}
}

static int
vge_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
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
			    !(sc->vge_if_flags & IFF_PROMISC)) {
				CSR_SETBIT_1(sc, VGE_RXCTL,
				    VGE_RXCTL_RX_PROMISC);
				vge_setmulti(sc);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC) &&
			    sc->vge_if_flags & IFF_PROMISC) {
				CSR_CLRBIT_1(sc, VGE_RXCTL,
				    VGE_RXCTL_RX_PROMISC);
				vge_setmulti(sc);
                        } else
				vge_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				vge_stop(sc);
		}
		sc->vge_if_flags = ifp->if_flags;
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

	sc = ifp->if_softc;
	VGE_LOCK(sc);
	printf("%s: watchdog timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;

	vge_txeof(sc);
	vge_rxeof(sc);

	vge_init(ifp);

	VGE_UNLOCK(sc);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
vge_stop(struct vge_softc *sc)
{
	int i;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	VGE_LOCK(sc);
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

	if (sc->vge_head != NULL) {
		m_freem(sc->vge_head);
		sc->vge_head = sc->vge_tail = NULL;
	}

	/* Free the TX list buffers. */

	for (i = 0; i < VGE_TX_DESC_CNT; i++) {
		if (sc->vge_ldata.vge_tx_mbuf[i] != NULL) {
			bus_dmamap_unload(sc->vge_dmat,
			    sc->vge_ldata.vge_tx_dmamap[i]);
			m_freem(sc->vge_ldata.vge_tx_mbuf[i]);
			sc->vge_ldata.vge_tx_mbuf[i] = NULL;
		}
	}

	/* Free the RX list buffers. */

	for (i = 0; i < VGE_RX_DESC_CNT; i++) {
		if (sc->vge_ldata.vge_rx_mbuf[i] != NULL) {
			bus_dmamap_unload(sc->vge_dmat,
			    sc->vge_ldata.vge_rx_dmamap[i]);
			m_freem(sc->vge_ldata.vge_rx_mbuf[i]);
			sc->vge_ldata.vge_rx_mbuf[i] = NULL;
		}
	}

	VGE_UNLOCK(sc);
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
		sc->saved_maps[i] = pci_read_config(dev, PCIR_MAPS + i * 4, 4);
	sc->saved_biosaddr = pci_read_config(dev, PCIR_BIOS, 4);
	sc->saved_intline = pci_read_config(dev, PCIR_INTLINE, 1);
	sc->saved_cachelnsz = pci_read_config(dev, PCIR_CACHELNSZ, 1);
	sc->saved_lattimer = pci_read_config(dev, PCIR_LATTIMER, 1);

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
		pci_write_config(dev, PCIR_MAPS + i * 4, sc->saved_maps[i], 4);
	pci_write_config(dev, PCIR_BIOS, sc->saved_biosaddr, 4);
	pci_write_config(dev, PCIR_INTLINE, sc->saved_intline, 1);
	pci_write_config(dev, PCIR_CACHELNSZ, sc->saved_cachelnsz, 1);
	pci_write_config(dev, PCIR_LATTIMER, sc->saved_lattimer, 1);

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
