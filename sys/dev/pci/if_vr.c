/*	$NetBSD: if_vr.c,v 1.17 1999/02/05 22:09:46 thorpej Exp $	*/

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
 *	$FreeBSD: if_vr.c,v 1.7 1999/01/10 18:51:49 wpaul Exp $
 */

/*
 * VIA Rhine fast ethernet PCI NIC driver
 *
 * Supports various network adapters based on the VIA Rhine
 * and Rhine II PCI controllers, including the D-Link DFE530TX.
 * Datasheets are available at http://www.via.com.tw.
 *
 * Written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The VIA Rhine controllers are similar in some respects to the
 * the DEC tulip chips, except less complicated. The controller
 * uses an MII bus and an external physical layer interface. The
 * receiver has a one entry perfect filter and a 64-bit hash table
 * multicast filter. Transmit and receive descriptors are similar
 * to the tulip.
 *
 * The Rhine has a serious flaw in its transmit DMA mechanism:
 * transmit buffers must be longword aligned. Unfortunately,
 * the kernel doesn't guarantee that mbufs will be filled in starting
 * at longword boundaries, so we have to do a buffer copy before
 * transmission.
 *
 * Apparently, the receive DMA mechanism also has the same flaw.  This
 * means that on systems with struct alignment requirements, incoming
 * frames must be copied to a new buffer which shifts the data forward
 * 2 bytes so that the payload is aligned on a 4-byte boundary.
 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if defined(INET)
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <vm/vm.h>		/* for vtophys */

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_vrreg.h>

#if defined(__NetBSD__) && defined(__alpha__)
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vaddr_t)(va))
#endif

#define	VR_USEIOSPACE

#define	ETHER_CRC_LEN	4	/* XXX Should be in a common header. */

/*
 * Various supported device vendors/types and their names.
 */
static struct vr_type {
	pci_vendor_id_t		vr_vid;
	pci_product_id_t	vr_did;
	const char		*vr_name;
} vr_devs[] = {
	{ PCI_VENDOR_VIATECH, PCI_PRODUCT_VIATECH_VT3043,
		"VIA VT3043 Rhine I 10/100BaseTX" },
	{ PCI_VENDOR_VIATECH, PCI_PRODUCT_VIATECH_VT86C100A,
		"VIA VT86C100A Rhine II 10/100BaseTX" },
	{ 0, 0, NULL }
};

struct vr_list_data {
	struct vr_desc		vr_rx_list[VR_RX_LIST_CNT];
	struct vr_desc		vr_tx_list[VR_TX_LIST_CNT];
};

struct vr_chain {
	struct vr_desc		*vr_ptr;
	struct mbuf		*vr_mbuf;
	struct vr_chain		*vr_nextdesc;
};

struct vr_chain_onefrag {
	struct vr_desc		*vr_ptr;
	struct mbuf		*vr_mbuf;
	struct vr_chain_onefrag	*vr_nextdesc;
};

struct vr_chain_data {
	struct vr_chain_onefrag	vr_rx_chain[VR_RX_LIST_CNT];
	struct vr_chain		vr_tx_chain[VR_TX_LIST_CNT];

	struct vr_chain_onefrag	*vr_rx_head;

	struct vr_chain		*vr_tx_head;
	struct vr_chain		*vr_tx_tail;
	struct vr_chain		*vr_tx_free;
};

struct vr_softc {
	struct device		vr_dev;		/* generic device glue */
	void			*vr_ih;		/* interrupt cookie */
	void			*vr_ats;	/* shutdown hook */
	bus_space_tag_t		vr_bst;		/* bus space tag */
	bus_space_handle_t	vr_bsh;		/* bus space handle */
	pci_chipset_tag_t	vr_pc;		/* PCI chipset info */
	struct ethercom		vr_ec;		/* Ethernet common info */
	u_int8_t 		vr_enaddr[ETHER_ADDR_LEN];
	struct mii_data		vr_mii;		/* MII/media info */
	caddr_t			vr_ldata_ptr;
	struct vr_list_data	*vr_ldata;
	struct vr_chain_data	vr_cdata;
};

/*
 * register space access macros
 */
#define	CSR_WRITE_4(sc, reg, val)	\
	bus_space_write_4(sc->vr_bst, sc->vr_bsh, reg, val)
#define	CSR_WRITE_2(sc, reg, val)	\
	bus_space_write_2(sc->vr_bst, sc->vr_bsh, reg, val)
#define	CSR_WRITE_1(sc, reg, val)	\
	bus_space_write_1(sc->vr_bst, sc->vr_bsh, reg, val)

#define	CSR_READ_4(sc, reg)		\
	bus_space_read_4(sc->vr_bst, sc->vr_bsh, reg)
#define	CSR_READ_2(sc, reg)		\
	bus_space_read_2(sc->vr_bst, sc->vr_bsh, reg)
#define	CSR_READ_1(sc, reg)		\
	bus_space_read_1(sc->vr_bst, sc->vr_bsh, reg)

#define	VR_TIMEOUT		1000

static int vr_newbuf		__P((struct vr_softc *,
						struct vr_chain_onefrag *));
static int vr_encap		__P((struct vr_softc *, struct vr_chain *,
						struct mbuf *));

static void vr_rxeof		__P((struct vr_softc *));
static void vr_rxeoc		__P((struct vr_softc *));
static void vr_txeof		__P((struct vr_softc *));
static void vr_txeoc		__P((struct vr_softc *));
static int vr_intr		__P((void *));
static void vr_start		__P((struct ifnet *));
static int vr_ioctl		__P((struct ifnet *, u_long, caddr_t));
static void vr_init		__P((void *));
static void vr_stop		__P((struct vr_softc *));
static void vr_watchdog		__P((struct ifnet *));
static void vr_tick		__P((void *));

static int vr_ifmedia_upd	__P((struct ifnet *));
static void vr_ifmedia_sts	__P((struct ifnet *, struct ifmediareq *));

static void vr_mii_sync		__P((struct vr_softc *));
static void vr_mii_send		__P((struct vr_softc *, u_int32_t, int));
static int vr_mii_readreg	__P((struct device *, int, int));
static void vr_mii_writereg	__P((struct device *, int, int, int));
static void vr_mii_statchg	__P((struct device *));

static u_int8_t vr_calchash	__P((u_int8_t *));
static void vr_setmulti		__P((struct vr_softc *));
static void vr_reset		__P((struct vr_softc *));
static int vr_list_rx_init	__P((struct vr_softc *));
static int vr_list_tx_init	__P((struct vr_softc *));

#define	VR_SETBIT(sc, reg, x)				\
	CSR_WRITE_1(sc, reg,				\
		CSR_READ_1(sc, reg) | x)

#define	VR_CLRBIT(sc, reg, x)				\
	CSR_WRITE_1(sc, reg,				\
		CSR_READ_1(sc, reg) & ~x)

#define	VR_SETBIT16(sc, reg, x)				\
	CSR_WRITE_2(sc, reg,				\
		CSR_READ_2(sc, reg) | x)

#define	VR_CLRBIT16(sc, reg, x)				\
	CSR_WRITE_2(sc, reg,				\
		CSR_READ_2(sc, reg) & ~x)

#define	VR_SETBIT32(sc, reg, x)				\
	CSR_WRITE_4(sc, reg,				\
		CSR_READ_4(sc, reg) | x)

#define	VR_CLRBIT32(sc, reg, x)				\
	CSR_WRITE_4(sc, reg,				\
		CSR_READ_4(sc, reg) & ~x)

#define	SIO_SET(x)					\
	CSR_WRITE_1(sc, VR_MIICMD,			\
		CSR_READ_1(sc, VR_MIICMD) | x)

#define	SIO_CLR(x)					\
	CSR_WRITE_1(sc, VR_MIICMD,			\
		CSR_READ_1(sc, VR_MIICMD) & ~x)

/*
 * Sync the PHYs by setting data bit and strobing the clock 32 times.
 */
static void
vr_mii_sync(sc)
	struct vr_softc *sc;
{
	int i;

	SIO_SET(VR_MIICMD_DIR|VR_MIICMD_DATAOUT);

	for (i = 0; i < 32; i++) {
		SIO_SET(VR_MIICMD_CLK);
		DELAY(1);
		SIO_CLR(VR_MIICMD_CLK);
		DELAY(1);
	}

	return;
}

/*
 * Clock a series of bits through the MII.
 */
static void
vr_mii_send(sc, bits, cnt)
	struct vr_softc *sc;
	u_int32_t bits;
	int cnt;
{
	int i;

	SIO_CLR(VR_MIICMD_CLK);

	for (i = (0x1 << (cnt - 1)); i; i >>= 1) {
		if (bits & i) {
			SIO_SET(VR_MIICMD_DATAOUT);
		} else {
			SIO_CLR(VR_MIICMD_DATAOUT);
		}
		DELAY(1);
		SIO_CLR(VR_MIICMD_CLK);
		DELAY(1);
		SIO_SET(VR_MIICMD_CLK);
	}
}

/*
 * Read an PHY register through the MII.
 */
static int
vr_mii_readreg(self, phy, reg)
	struct device *self;
	int phy, reg;
{
	struct vr_softc *sc = (struct vr_softc *)self;
	int i, ack, val = 0;

	CSR_WRITE_1(sc, VR_MIICMD, 0);
	VR_SETBIT(sc, VR_MIICMD, VR_MIICMD_DIRECTPGM);

	/*
	 * Turn on data xmit.
	 */
	SIO_SET(VR_MIICMD_DIR);

	vr_mii_sync(sc);

	/*
	 * Send command/address info.
	 */
	vr_mii_send(sc, MII_COMMAND_START, 2);
	vr_mii_send(sc, MII_COMMAND_READ, 2);
	vr_mii_send(sc, phy, 5);
	vr_mii_send(sc, reg, 5);

	/* Idle bit */
	SIO_CLR((VR_MIICMD_CLK|VR_MIICMD_DATAOUT));
	DELAY(1);
	SIO_SET(VR_MIICMD_CLK);
	DELAY(1);

	/* Turn off xmit. */
	SIO_CLR(VR_MIICMD_DIR);

	/* Check for ack */
	SIO_CLR(VR_MIICMD_CLK);
	DELAY(1);
	SIO_SET(VR_MIICMD_CLK);
	DELAY(1);
	ack = CSR_READ_4(sc, VR_MIICMD) & VR_MIICMD_DATAIN;

	/*
	 * Now try reading data bits. If the ack failed, we still
	 * need to clock through 16 cycles to keep the PHY(s) in sync.
	 */
	if (ack) {
		for (i = 0; i < 16; i++) {
			SIO_CLR(VR_MIICMD_CLK);
			DELAY(1);
			SIO_SET(VR_MIICMD_CLK);
			DELAY(1);
		}
		goto fail;
	}

	for (i = 0x8000; i; i >>= 1) {
		SIO_CLR(VR_MIICMD_CLK);
		DELAY(1);
		if (!ack) {
			if (CSR_READ_4(sc, VR_MIICMD) & VR_MIICMD_DATAIN)
				val |= i;
			DELAY(1);
		}
		SIO_SET(VR_MIICMD_CLK);
		DELAY(1);
	}

 fail:

	SIO_CLR(VR_MIICMD_CLK);
	DELAY(1);
	SIO_SET(VR_MIICMD_CLK);
	DELAY(1);

	return (val);
}

/*
 * Write to a PHY register through the MII.
 */
static void
vr_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct vr_softc *sc = (struct vr_softc *)self;

	CSR_WRITE_1(sc, VR_MIICMD, 0);
	VR_SETBIT(sc, VR_MIICMD, VR_MIICMD_DIRECTPGM);

	/*
	 * Turn on data output.
	 */
	SIO_SET(VR_MIICMD_DIR);

	vr_mii_sync(sc);

	vr_mii_send(sc, MII_COMMAND_START, 2);
	vr_mii_send(sc, MII_COMMAND_WRITE, 2);
	vr_mii_send(sc, phy, 5);
	vr_mii_send(sc, reg, 5);
	vr_mii_send(sc, MII_COMMAND_ACK, 2);
	vr_mii_send(sc, val, 16);

	/* Idle bit. */
	SIO_SET(VR_MIICMD_CLK);
	DELAY(1);
	SIO_CLR(VR_MIICMD_CLK);
	DELAY(1);

	/*
	 * Turn off xmit.
	 */
	SIO_CLR(VR_MIICMD_DIR);
}

static void
vr_mii_statchg(self)
	struct device *self;
{
	struct vr_softc *sc = (struct vr_softc *)self;
	int restart = 0;

	/*
	 * In order to fiddle with the 'full-duplex' bit in the netconfig
	 * register, we first have to put the transmit and/or receive logic
	 * in the idle state.
	 */
	if (CSR_READ_2(sc, VR_COMMAND) & (VR_CMD_TX_ON|VR_CMD_RX_ON)) {
		restart = 1;
		VR_CLRBIT16(sc, VR_COMMAND, (VR_CMD_TX_ON|VR_CMD_RX_ON));
	}

	if (sc->vr_mii.mii_media_active & IFM_FDX)
		VR_SETBIT16(sc, VR_COMMAND, VR_CMD_FULLDUPLEX);
	else
		VR_CLRBIT16(sc, VR_COMMAND, VR_CMD_FULLDUPLEX);

	if (restart)
		VR_SETBIT16(sc, VR_COMMAND, VR_CMD_TX_ON|VR_CMD_RX_ON);

	/* XXX Update ifp->if_baudrate */
}

/*
 * Calculate CRC of a multicast group address, return the lower 6 bits.
 */
static u_int8_t
vr_calchash(addr)
	u_int8_t *addr;
{
	u_int32_t crc, carry;
	int i, j;
	u_int8_t c;

	/* Compute CRC for the address value. */
	crc = 0xFFFFFFFF; /* initial value */

	for (i = 0; i < 6; i++) {
		c = *(addr + i);
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01);
			crc <<= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ 0x04c11db6) | carry;
		}
	}

	/* return the filter bit position */
	return ((crc >> 26) & 0x0000003F);
}

/*
 * Program the 64-bit multicast hash filter.
 */
static void
vr_setmulti(sc)
	struct vr_softc *sc;
{
	struct ifnet *ifp;
	int h = 0;
	u_int32_t hashes[2] = { 0, 0 };
	struct ether_multistep step;
	struct ether_multi *enm;
	int mcnt = 0;
	u_int8_t rxfilt;

	ifp = &sc->vr_ec.ec_if;

	rxfilt = CSR_READ_1(sc, VR_RXCFG);

	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		rxfilt |= VR_RXCFG_RX_MULTI;
		CSR_WRITE_1(sc, VR_RXCFG, rxfilt);
		CSR_WRITE_4(sc, VR_MAR0, 0xFFFFFFFF);
		CSR_WRITE_4(sc, VR_MAR1, 0xFFFFFFFF);
		return;
	}

	/* first, zot all the existing hash bits */
	CSR_WRITE_4(sc, VR_MAR0, 0);
	CSR_WRITE_4(sc, VR_MAR1, 0);

	/* now program new ones */
	ETHER_FIRST_MULTI(step, &sc->vr_ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0)
			continue;

		h = vr_calchash(enm->enm_addrlo);

		if (h < 32)
			hashes[0] |= (1 << h);
		else
			hashes[1] |= (1 << (h - 32));
		ETHER_NEXT_MULTI(step, enm);
		mcnt++;
	}

	if (mcnt)
		rxfilt |= VR_RXCFG_RX_MULTI;
	else
		rxfilt &= ~VR_RXCFG_RX_MULTI;

	CSR_WRITE_4(sc, VR_MAR0, hashes[0]);
	CSR_WRITE_4(sc, VR_MAR1, hashes[1]);
	CSR_WRITE_1(sc, VR_RXCFG, rxfilt);

	return;
}

static void
vr_reset(sc)
	struct vr_softc *sc;
{
	int i;

	VR_SETBIT16(sc, VR_COMMAND, VR_CMD_RESET);

	for (i = 0; i < VR_TIMEOUT; i++) {
		DELAY(10);
		if (!(CSR_READ_2(sc, VR_COMMAND) & VR_CMD_RESET))
			break;
	}
	if (i == VR_TIMEOUT)
		printf("%s: reset never completed!\n",
			sc->vr_dev.dv_xname);

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);

	return;
}

/*
 * Initialize the transmit descriptors.
 */
static int
vr_list_tx_init(sc)
	struct vr_softc *sc;
{
	struct vr_chain_data *cd;
	struct vr_list_data *ld;
	int i;

	cd = &sc->vr_cdata;
	ld = sc->vr_ldata;
	for (i = 0; i < VR_TX_LIST_CNT; i++) {
		cd->vr_tx_chain[i].vr_ptr = &ld->vr_tx_list[i];
		if (i == (VR_TX_LIST_CNT - 1))
			cd->vr_tx_chain[i].vr_nextdesc =
				&cd->vr_tx_chain[0];
		else
			cd->vr_tx_chain[i].vr_nextdesc =
				&cd->vr_tx_chain[i + 1];
	}

	cd->vr_tx_free = &cd->vr_tx_chain[0];
	cd->vr_tx_tail = cd->vr_tx_head = NULL;

	return (0);
}


/*
 * Initialize the RX descriptors and allocate mbufs for them. Note that
 * we arrange the descriptors in a closed ring, so that the last descriptor
 * points back to the first.
 */
static int
vr_list_rx_init(sc)
	struct vr_softc *sc;
{
	struct vr_chain_data *cd;
	struct vr_list_data *ld;
	int i;

	cd = &sc->vr_cdata;
	ld = sc->vr_ldata;

	for (i = 0; i < VR_RX_LIST_CNT; i++) {
		cd->vr_rx_chain[i].vr_ptr =
			(struct vr_desc *)&ld->vr_rx_list[i];
		if (vr_newbuf(sc, &cd->vr_rx_chain[i]) == ENOBUFS)
			return (ENOBUFS);
		if (i == (VR_RX_LIST_CNT - 1)) {
			cd->vr_rx_chain[i].vr_nextdesc =
					&cd->vr_rx_chain[0];
			ld->vr_rx_list[i].vr_next =
					vtophys(&ld->vr_rx_list[0]);
		} else {
			cd->vr_rx_chain[i].vr_nextdesc =
					&cd->vr_rx_chain[i + 1];
			ld->vr_rx_list[i].vr_next =
					vtophys(&ld->vr_rx_list[i + 1]);
		}
	}

	cd->vr_rx_head = &cd->vr_rx_chain[0];

	return (0);
}

/*
 * Initialize an RX descriptor and attach an MBUF cluster.
 * Note: the length fields are only 11 bits wide, which means the
 * largest size we can specify is 2047. This is important because
 * MCLBYTES is 2048, so we have to subtract one otherwise we'll
 * overflow the field and make a mess.
 */
static int
vr_newbuf(sc, c)
	struct vr_softc *sc;
	struct vr_chain_onefrag *c;
{
	struct mbuf *m_new = NULL;

	MGETHDR(m_new, M_DONTWAIT, MT_DATA);
	if (m_new == NULL) {
		printf("%s: no memory for rx list -- packet dropped!\n",
			sc->vr_dev.dv_xname);
		return (ENOBUFS);
	}

	MCLGET(m_new, M_DONTWAIT);
	if (!(m_new->m_flags & M_EXT)) {
		printf("%s: no memory for rx list -- packet dropped!\n",
			sc->vr_dev.dv_xname);
		m_freem(m_new);
		return (ENOBUFS);
	}

	c->vr_mbuf = m_new;
	c->vr_ptr->vr_status = VR_RXSTAT;
	c->vr_ptr->vr_data = vtophys(mtod(m_new, caddr_t));
	c->vr_ptr->vr_ctl = VR_RXCTL | VR_RXLEN;

	return (0);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
vr_rxeof(sc)
	struct vr_softc *sc;
{
	struct ether_header *eh;
	struct mbuf *m;
	struct ifnet *ifp;
	struct vr_chain_onefrag *cur_rx;
	int total_len = 0;
	u_int32_t rxstat;

	ifp = &sc->vr_ec.ec_if;

	while (!((rxstat = sc->vr_cdata.vr_rx_head->vr_ptr->vr_status) &
							VR_RXSTAT_OWN)) {
		cur_rx = sc->vr_cdata.vr_rx_head;
		sc->vr_cdata.vr_rx_head = cur_rx->vr_nextdesc;

		/*
		 * If an error occurs, update stats, clear the
		 * status word and leave the mbuf cluster in place:
		 * it should simply get re-used next time this descriptor
		 * comes up in the ring.
		 */
		if (rxstat & VR_RXSTAT_RXERR) {
			ifp->if_ierrors++;
			printf("%s: rx error: ", sc->vr_dev.dv_xname);
			switch (rxstat & 0x000000FF) {
			case VR_RXSTAT_CRCERR:
				printf("crc error\n");
				break;
			case VR_RXSTAT_FRAMEALIGNERR:
				printf("frame alignment error\n");
				break;
			case VR_RXSTAT_FIFOOFLOW:
				printf("FIFO overflow\n");
				break;
			case VR_RXSTAT_GIANT:
				printf("received giant packet\n");
				break;
			case VR_RXSTAT_RUNT:
				printf("received runt packet\n");
				break;
			case VR_RXSTAT_BUSERR:
				printf("system bus error\n");
				break;
			case VR_RXSTAT_BUFFERR:
				printf("rx buffer error\n");
				break;
			default:
				printf("unknown rx error\n");
				break;
			}
			cur_rx->vr_ptr->vr_status = VR_RXSTAT;
			cur_rx->vr_ptr->vr_ctl = VR_RXCTL|VR_RXLEN;
			continue;
		}

		/* No errors; receive the packet. */
		total_len = VR_RXBYTES(cur_rx->vr_ptr->vr_status);

		/*
		 * XXX The VIA Rhine chip includes the CRC with every
		 * received frame, and there's no way to turn this
		 * behavior off (at least, I can't find anything in
		 * the manual that explains how to do it) so we have
		 * to trim off the CRC manually.
		 */
		total_len -= ETHER_CRC_LEN;

#ifdef __NO_STRICT_ALIGNMENT
		/*
		 * Try to conjure up a new mbuf cluster. If that
		 * fails, it means we have an out of memory condition and
		 * should leave the buffer in place and continue. This will
		 * result in a lost packet, but there's little else we
		 * can do in this situation.
		 */
		m = cur_rx->vr_mbuf;
		if (vr_newbuf(sc, cur_rx) == ENOBUFS) {
			ifp->if_ierrors++;
			cur_rx->vr_ptr->vr_status = VR_RXSTAT;
			cur_rx->vr_ptr->vr_ctl = VR_RXCTL|VR_RXLEN;
			continue;
		}
#else
		/*
		 * The Rhine's packet buffers must be 4-byte aligned.
		 * But this means that the data after the Ethernet header
		 * is misaligned.  We must allocate a new buffer and
		 * copy the data, shifted forward 2 bytes.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
 dropit:
			ifp->if_ierrors++;
			cur_rx->vr_ptr->vr_status = VR_RXSTAT;
			cur_rx->vr_ptr->vr_ctl = VR_RXCTL|VR_RXLEN;
			continue;
		}
		if (total_len > (MHLEN - 2)) {
			MCLGET(m, M_DONTWAIT);
			if (m == NULL)
				goto dropit;
		}
		m->m_data += 2;

		/*
		 * Note that we use clusters for incoming frames, so the
		 * buffer is virtually contiguous.
		 */
		memcpy(mtod(m, caddr_t), mtod(cur_rx->vr_mbuf, caddr_t),
		    total_len);

		/* Allow the recieve descriptor to continue using its mbuf. */
		cur_rx->vr_ptr->vr_status = VR_RXSTAT;
		cur_rx->vr_ptr->vr_ctl = VR_RXCTL|VR_RXLEN;
#endif /* __NO_STRICT_ALIGNMENT */

		ifp->if_ipackets++;
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = total_len;
#if NBPFILTER > 0
		/*
		 * Handle BPF listeners. Let the BPF user see the packet, but
		 * don't pass it up to the ether_input() layer unless it's
		 * a broadcast packet, multicast packet, matches our ethernet
		 * address or the interface is in promiscuous mode.
		 */
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);
			if (ifp->if_flags & IFF_PROMISC &&
				(memcmp(eh->ether_dhost, sc->vr_enaddr,
						ETHER_ADDR_LEN) &&
					(eh->ether_dhost[0] & 1) == 0)) {
				m_freem(m);
				continue;
			}
		}
#endif
		/* Remove header from mbuf and pass it on. */
		m_adj(m, sizeof (struct ether_header));
		ether_input(ifp, eh, m);
	}
}

void
vr_rxeoc(sc)
	struct vr_softc *sc;
{

	vr_rxeof(sc);
	VR_CLRBIT16(sc, VR_COMMAND, VR_CMD_RX_ON);
	CSR_WRITE_4(sc, VR_RXADDR, vtophys(sc->vr_cdata.vr_rx_head->vr_ptr));
	VR_SETBIT16(sc, VR_COMMAND, VR_CMD_RX_ON);
	VR_SETBIT16(sc, VR_COMMAND, VR_CMD_RX_GO);
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */

static void
vr_txeof(sc)
	struct vr_softc *sc;
{
	struct vr_chain *cur_tx;
	struct ifnet *ifp;
	register struct mbuf *n;

	ifp = &sc->vr_ec.ec_if;

	/* Clear the timeout timer. */
	ifp->if_timer = 0;

	/* Sanity check. */
	if (sc->vr_cdata.vr_tx_head == NULL)
		return;

	/*
	 * Go through our tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	while (sc->vr_cdata.vr_tx_head->vr_mbuf != NULL) {
		u_int32_t txstat;

		cur_tx = sc->vr_cdata.vr_tx_head;
		txstat = cur_tx->vr_ptr->vr_status;

		if (txstat & VR_TXSTAT_OWN)
			break;

		if (txstat & VR_TXSTAT_ERRSUM) {
			ifp->if_oerrors++;
			if (txstat & VR_TXSTAT_DEFER)
				ifp->if_collisions++;
			if (txstat & VR_TXSTAT_LATECOLL)
				ifp->if_collisions++;
		}

		ifp->if_collisions +=(txstat & VR_TXSTAT_COLLCNT) >> 3;

		ifp->if_opackets++;
		MFREE(cur_tx->vr_mbuf, n);
		cur_tx->vr_mbuf = NULL;

		if (sc->vr_cdata.vr_tx_head == sc->vr_cdata.vr_tx_tail) {
			sc->vr_cdata.vr_tx_head = NULL;
			sc->vr_cdata.vr_tx_tail = NULL;
			break;
		}

		sc->vr_cdata.vr_tx_head = cur_tx->vr_nextdesc;
	}
}

/*
 * TX 'end of channel' interrupt handler.
 */
static void
vr_txeoc(sc)
	struct vr_softc *sc;
{
	struct ifnet *ifp;

	ifp = &sc->vr_ec.ec_if;

	ifp->if_timer = 0;

	if (sc->vr_cdata.vr_tx_head == NULL) {
		ifp->if_flags &= ~IFF_OACTIVE;
		sc->vr_cdata.vr_tx_tail = NULL;
	}
}

static int
vr_intr(arg)
	void *arg;
{
	struct vr_softc *sc;
	struct ifnet *ifp;
	u_int16_t status;
	int handled = 0;

	sc = arg;
	ifp = &sc->vr_ec.ec_if;

	/* Supress unwanted interrupts. */
	if ((ifp->if_flags & IFF_UP) == 0) {
		vr_stop(sc);
		return (0);
	}

	/* Disable interrupts. */
	CSR_WRITE_2(sc, VR_IMR, 0x0000);

	for (;;) {
		status = CSR_READ_2(sc, VR_ISR);
		if (status)
			CSR_WRITE_2(sc, VR_ISR, status);

		if ((status & VR_INTRS) == 0)
			break;

		handled = 1;

		if (status & VR_ISR_RX_OK)
			vr_rxeof(sc);

		if ((status & VR_ISR_RX_ERR) || (status & VR_ISR_RX_NOBUF) ||
		    (status & VR_ISR_RX_NOBUF) || (status & VR_ISR_RX_OFLOW) ||
		    (status & VR_ISR_RX_DROPPED)) {
			vr_rxeof(sc);
			vr_rxeoc(sc);
		}

		if (status & VR_ISR_TX_OK) {
			vr_txeof(sc);
			vr_txeoc(sc);
		}

		if ((status & VR_ISR_TX_UNDERRUN)||(status & VR_ISR_TX_ABRT)) {
			ifp->if_oerrors++;
			vr_txeof(sc);
			if (sc->vr_cdata.vr_tx_head != NULL) {
				VR_SETBIT16(sc, VR_COMMAND, VR_CMD_TX_ON);
				VR_SETBIT16(sc, VR_COMMAND, VR_CMD_TX_GO);
			}
		}

		if (status & VR_ISR_BUSERR) {
			vr_reset(sc);
			vr_init(sc);
		}
	}

	/* Re-enable interrupts. */
	CSR_WRITE_2(sc, VR_IMR, VR_INTRS);

	if (ifp->if_snd.ifq_head != NULL) {
		vr_start(ifp);
	}

	return (handled);
}

/*
 * Encapsulate an mbuf chain in a descriptor by coupling the mbuf data
 * pointers to the fragment pointers.
 */
static int
vr_encap(sc, c, m_head)
	struct vr_softc *sc;
	struct vr_chain *c;
	struct mbuf *m_head;
{
	int frag = 0;
	struct vr_desc *f = NULL;
	int total_len;
	struct mbuf *m;

	m = m_head;
	total_len = 0;

	/*
	 * The VIA Rhine wants packet buffers to be longword
	 * aligned, but very often our mbufs aren't. Rather than
	 * waste time trying to decide when to copy and when not
	 * to copy, just do it all the time.
	 */
	if (m != NULL) {
		struct mbuf		*m_new = NULL;

		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: no memory for tx list",
				sc->vr_dev.dv_xname);
			return (1);
		}
		if (m_head->m_pkthdr.len > MHLEN) {
			MCLGET(m_new, M_DONTWAIT);
			if (!(m_new->m_flags & M_EXT)) {
				m_freem(m_new);
				printf("%s: no memory for tx list",
					sc->vr_dev.dv_xname);
				return (1);
			}
		}
		m_copydata(m_head, 0, m_head->m_pkthdr.len,
					mtod(m_new, caddr_t));
		m_new->m_pkthdr.len = m_new->m_len = m_head->m_pkthdr.len;
		m_freem(m_head);
		m_head = m_new;
		/*
		 * The Rhine chip doesn't auto-pad, so we have to make
		 * sure to pad short frames out to the minimum frame length
		 * ourselves.
		 */
		if (m_head->m_len < VR_MIN_FRAMELEN) {
			m_new->m_pkthdr.len += VR_MIN_FRAMELEN - m_new->m_len;
			m_new->m_len = m_new->m_pkthdr.len;
		}
		f = c->vr_ptr;
		f->vr_data = vtophys(mtod(m_new, caddr_t));
		f->vr_ctl = total_len = m_new->m_len;
		f->vr_ctl |= VR_TXCTL_TLINK|VR_TXCTL_FIRSTFRAG;
		f->vr_status = 0;
		frag = 1;
	}

	c->vr_mbuf = m_head;
	c->vr_ptr->vr_ctl |= VR_TXCTL_LASTFRAG|VR_TXCTL_FINT;
	c->vr_ptr->vr_next = vtophys(c->vr_nextdesc->vr_ptr);

	return (0);
}

/*
 * Main transmit routine. To avoid having to do mbuf copies, we put pointers
 * to the mbuf data regions directly in the transmit lists. We also save a
 * copy of the pointers since the transmit list fragment pointers are
 * physical addresses.
 */
static void
vr_start(ifp)
	struct ifnet *ifp;
{
	struct vr_softc *sc;
	struct mbuf *m_head = NULL;
	struct vr_chain *cur_tx = NULL, *start_tx;

	sc = ifp->if_softc;

	/*
	 * Check for an available queue slot. If there are none,
	 * punt.
	 */
	if (sc->vr_cdata.vr_tx_free->vr_mbuf != NULL) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	start_tx = sc->vr_cdata.vr_tx_free;

	while (sc->vr_cdata.vr_tx_free->vr_mbuf == NULL) {
		IF_DEQUEUE(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		/* Pick a descriptor off the free list. */
		cur_tx = sc->vr_cdata.vr_tx_free;
		sc->vr_cdata.vr_tx_free = cur_tx->vr_nextdesc;

		/* Pack the data into the descriptor. */
		vr_encap(sc, cur_tx, m_head);

		if (cur_tx != start_tx)
			VR_TXOWN(cur_tx) = VR_TXSTAT_OWN;

#if NBPFILTER > 0
		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, cur_tx->vr_mbuf);
#endif
		VR_TXOWN(cur_tx) = VR_TXSTAT_OWN;
		VR_SETBIT16(sc, VR_COMMAND, VR_CMD_TX_ON|VR_CMD_TX_GO);
	}

	/*
	 * If there are no frames queued, bail.
	 */
	if (cur_tx == NULL)
		return;

	sc->vr_cdata.vr_tx_tail = cur_tx;

	if (sc->vr_cdata.vr_tx_head == NULL)
		sc->vr_cdata.vr_tx_head = start_tx;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

/*
 * Initialize the interface.  Must be called at splnet.
 */
static void
vr_init(xsc)
	void *xsc;
{
	struct vr_softc *sc = xsc;
	struct ifnet *ifp = &sc->vr_ec.ec_if;

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
	vr_stop(sc);
	vr_reset(sc);

	VR_CLRBIT(sc, VR_RXCFG, VR_RXCFG_RX_THRESH);
	VR_SETBIT(sc, VR_RXCFG, VR_RXTHRESH_STORENFWD);

	VR_CLRBIT(sc, VR_TXCFG, VR_TXCFG_TX_THRESH);
	VR_SETBIT(sc, VR_TXCFG, VR_TXTHRESH_STORENFWD);

	/* Init circular RX list. */
	if (vr_list_rx_init(sc) == ENOBUFS) {
		printf("%s: initialization failed: no "
			"memory for rx buffers\n", sc->vr_dev.dv_xname);
		vr_stop(sc);
		return;
	}

	/*
	 * Init tx descriptors.
	 */
	vr_list_tx_init(sc);

	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC)
		VR_SETBIT(sc, VR_RXCFG, VR_RXCFG_RX_PROMISC);
	else
		VR_CLRBIT(sc, VR_RXCFG, VR_RXCFG_RX_PROMISC);

	/* Set capture broadcast bit to capture broadcast frames. */
	if (ifp->if_flags & IFF_BROADCAST)
		VR_SETBIT(sc, VR_RXCFG, VR_RXCFG_RX_BROAD);
	else
		VR_CLRBIT(sc, VR_RXCFG, VR_RXCFG_RX_BROAD);

	/*
	 * Program the multicast filter, if necessary.
	 */
	vr_setmulti(sc);

	/*
	 * Load the address of the RX list.
	 */
	CSR_WRITE_4(sc, VR_RXADDR, vtophys(sc->vr_cdata.vr_rx_head->vr_ptr));

	/* Enable receiver and transmitter. */
	CSR_WRITE_2(sc, VR_COMMAND, VR_CMD_TX_NOPOLL|VR_CMD_START|
				    VR_CMD_TX_ON|VR_CMD_RX_ON|
				    VR_CMD_RX_GO);

	/* Set current media. */
	mii_mediachg(&sc->vr_mii);

	CSR_WRITE_4(sc, VR_TXADDR, vtophys(&sc->vr_ldata->vr_tx_list[0]));

	/*
	 * Enable interrupts.
	 */
	CSR_WRITE_2(sc, VR_ISR, 0xFFFF);
	CSR_WRITE_2(sc, VR_IMR, VR_INTRS);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Start one second timer. */
	timeout(vr_tick, sc, hz);
}

/*
 * Set media options.
 */
static int
vr_ifmedia_upd(ifp)
	struct ifnet *ifp;
{
	struct vr_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->vr_mii);
	return (0);
}

/*
 * Report current media status.
 */
static void
vr_ifmedia_sts(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct vr_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->vr_mii);
	ifmr->ifm_status = sc->vr_mii.mii_media_status;
	ifmr->ifm_active = sc->vr_mii.mii_media_active;
}

static int
vr_ioctl(ifp, command, data)
	struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct vr_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			vr_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif /* INET */
		default:
			vr_init(sc);
			break;
		}
		break;

	case SIOCGIFADDR:
		bcopy((caddr_t) sc->vr_enaddr,
			(caddr_t) ((struct sockaddr *)&ifr->ifr_data)->sa_data,
			ETHER_ADDR_LEN);
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			vr_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				vr_stop(sc);
		}
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (command == SIOCADDMULTI)
			error = ether_addmulti(ifr, &sc->vr_ec);
		else
			error = ether_delmulti(ifr, &sc->vr_ec);

		if (error == ENETRESET) {
			vr_setmulti(sc);
			error = 0;
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->vr_mii.mii_media, command);
		break;
	default:
		error = EINVAL;
		break;
	}

	splx(s);

	return (error);
}

static void
vr_watchdog(ifp)
	struct ifnet *ifp;
{
	struct vr_softc *sc;

	sc = ifp->if_softc;

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", sc->vr_dev.dv_xname);

	vr_stop(sc);
	vr_reset(sc);
	vr_init(sc);

	if (ifp->if_snd.ifq_head != NULL)
		vr_start(ifp);

	return;
}

/*
 * One second timer, used to tick MII.
 */
static void
vr_tick(arg)
	void *arg;
{
	struct vr_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->vr_mii);
	splx(s);

	timeout(vr_tick, sc, hz);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
vr_stop(sc)
	struct vr_softc *sc;
{
	struct ifnet *ifp;
	int i;

	/* Cancel one second timer. */
	untimeout(vr_tick, sc);

	ifp = &sc->vr_ec.ec_if;
	ifp->if_timer = 0;

	VR_SETBIT16(sc, VR_COMMAND, VR_CMD_STOP);
	VR_CLRBIT16(sc, VR_COMMAND, (VR_CMD_RX_ON|VR_CMD_TX_ON));
	CSR_WRITE_2(sc, VR_IMR, 0x0000);
	CSR_WRITE_4(sc, VR_TXADDR, 0x00000000);
	CSR_WRITE_4(sc, VR_RXADDR, 0x00000000);

	/*
	 * Free data in the RX lists.
	 */
	for (i = 0; i < VR_RX_LIST_CNT; i++) {
		if (sc->vr_cdata.vr_rx_chain[i].vr_mbuf != NULL) {
			m_freem(sc->vr_cdata.vr_rx_chain[i].vr_mbuf);
			sc->vr_cdata.vr_rx_chain[i].vr_mbuf = NULL;
		}
	}
	bzero((char *)&sc->vr_ldata->vr_rx_list,
		sizeof (sc->vr_ldata->vr_rx_list));

	/*
	 * Free the TX list buffers.
	 */
	for (i = 0; i < VR_TX_LIST_CNT; i++) {
		if (sc->vr_cdata.vr_tx_chain[i].vr_mbuf != NULL) {
			m_freem(sc->vr_cdata.vr_tx_chain[i].vr_mbuf);
			sc->vr_cdata.vr_tx_chain[i].vr_mbuf = NULL;
		}
	}

	bzero((char *)&sc->vr_ldata->vr_tx_list,
		sizeof (sc->vr_ldata->vr_tx_list));

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static struct vr_type *vr_lookup __P((struct pci_attach_args *));
static int vr_probe __P((struct device *, struct cfdata *, void *));
static void vr_attach __P((struct device *, struct device *, void *));
static void vr_shutdown __P((void *));

struct cfattach vr_ca = {
	sizeof (struct vr_softc), vr_probe, vr_attach
};

static struct vr_type *
vr_lookup(pa)
	struct pci_attach_args *pa;
{
	struct vr_type *vrt;

	for (vrt = vr_devs; vrt->vr_name != NULL; vrt++) {
		if (PCI_VENDOR(pa->pa_id) == vrt->vr_vid &&
		    PCI_PRODUCT(pa->pa_id) == vrt->vr_did)
			return (vrt);
	}
	return (NULL);
}

static int
vr_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (vr_lookup(pa) != NULL)
		return (1);

	return (0);
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static void
vr_shutdown(arg)
	void *arg;
{
	struct vr_softc *sc = (struct vr_softc *)arg;

	vr_stop(sc);
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
vr_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vr_softc *sc = (struct vr_softc *) self;
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;
	struct vr_type *vrt;
	int i;
	u_int32_t command;
	struct ifnet *ifp;
	unsigned int round;
	caddr_t roundptr;
	u_char eaddr[ETHER_ADDR_LEN];

#define	PCI_CONF_WRITE(r, v)	pci_conf_write(pa->pa_pc, pa->pa_tag, (r), (v))
#define	PCI_CONF_READ(r)	pci_conf_read(pa->pa_pc, pa->pa_tag, (r))

	vrt = vr_lookup(pa);
	if (vrt == NULL) {
		printf("\n");
		panic("vr_attach: impossible");
	}

	printf(": %s Ethernet\n", vrt->vr_name);

	/*
	 * Handle power management nonsense.
	 */

	command = PCI_CONF_READ(VR_PCI_CAPID) & 0x000000FF;
	if (command == 0x01) {
		command = PCI_CONF_READ(VR_PCI_PWRMGMTCTRL);
		if (command & VR_PSTATE_MASK) {
			u_int32_t iobase, membase, irq;

			/* Save important PCI config data. */
			iobase = PCI_CONF_READ(VR_PCI_LOIO);
			membase = PCI_CONF_READ(VR_PCI_LOMEM);
			irq = PCI_CONF_READ(VR_PCI_INTLINE);

			/* Reset the power state. */
			printf("%s: chip is in D%d power mode "
				"-- setting to D0\n",
				sc->vr_dev.dv_xname, command & VR_PSTATE_MASK);
			command &= 0xFFFFFFFC;
			PCI_CONF_WRITE(VR_PCI_PWRMGMTCTRL, command);

			/* Restore PCI config data. */
			PCI_CONF_WRITE(VR_PCI_LOIO, iobase);
			PCI_CONF_WRITE(VR_PCI_LOMEM, membase);
			PCI_CONF_WRITE(VR_PCI_INTLINE, irq);
		}
	}

	/*
	 * Map control/status registers.
	 */
	command = PCI_CONF_READ(PCI_COMMAND_STATUS_REG);
	command |= (PCI_COMMAND_IO_ENABLE |
		    PCI_COMMAND_MEM_ENABLE |
		    PCI_COMMAND_MASTER_ENABLE);
	PCI_CONF_WRITE(PCI_COMMAND_STATUS_REG, command);
	command = PCI_CONF_READ(PCI_COMMAND_STATUS_REG);

	{
		bus_space_tag_t iot, memt;
		bus_space_handle_t ioh, memh;
		int ioh_valid, memh_valid;
		pci_intr_handle_t intrhandle;
		const char *intrstr;

		ioh_valid = (pci_mapreg_map(pa, VR_PCI_LOIO,
			PCI_MAPREG_TYPE_IO, 0,
			&iot, &ioh, NULL, NULL) == 0);
		memh_valid = (pci_mapreg_map(pa, VR_PCI_LOMEM,
			PCI_MAPREG_TYPE_MEM |
			PCI_MAPREG_MEM_TYPE_32BIT,
			0, &memt, &memh, NULL, NULL) == 0);
#if defined(VR_USEIOSPACE)
		if (ioh_valid) {
			sc->vr_bst = iot;
			sc->vr_bsh = ioh;
		} else if (memh_valid) {
			sc->vr_bst = memt;
			sc->vr_bsh = memh;
		}
#else
		if (memh_valid) {
			sc->vr_bst = memt;
			sc->vr_bsh = memh;
		} else if (ioh_valid) {
			sc->vr_bst = iot;
			sc->vr_bsh = ioh;
		}
#endif
		else {
			printf(": unable to map device registers\n");
			return;
		}

		/* Allocate interrupt */
		if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
				pa->pa_intrline, &intrhandle)) {
			printf("%s: couldn't map interrupt\n",
				sc->vr_dev.dv_xname);
			return;
		}
		intrstr = pci_intr_string(pa->pa_pc, intrhandle);
		sc->vr_ih = pci_intr_establish(pa->pa_pc, intrhandle, IPL_NET,
						vr_intr, sc);
		if (sc->vr_ih == NULL) {
			printf("%s: couldn't establish interrupt",
				sc->vr_dev.dv_xname);
			if (intrstr != NULL)
				printf(" at %s", intrstr);
			printf("\n");
		}
		printf("%s: interrupting at %s\n",
			sc->vr_dev.dv_xname, intrstr);
	}
	sc->vr_ats = shutdownhook_establish(vr_shutdown, sc);
	if (sc->vr_ats == NULL)
		printf("%s: warning: couldn't establish shutdown hook\n",
			sc->vr_dev.dv_xname);

	/* Reset the adapter. */
	vr_reset(sc);

	/*
	 * Get station address. The way the Rhine chips work,
	 * you're not allowed to directly access the EEPROM once
	 * they've been programmed a special way. Consequently,
	 * we need to read the node address from the PAR0 and PAR1
	 * registers.
	 */
	VR_SETBIT(sc, VR_EECSR, VR_EECSR_LOAD);
	DELAY(200);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		eaddr[i] = CSR_READ_1(sc, VR_PAR0 + i);

	/*
	 * A Rhine chip was detected. Inform the world.
	 */
	printf("%s: Ethernet address: %s\n",
		sc->vr_dev.dv_xname, ether_sprintf(eaddr));

	bcopy(eaddr, sc->vr_enaddr, ETHER_ADDR_LEN);

	sc->vr_ldata_ptr = malloc(sizeof (struct vr_list_data) + 8,
				M_DEVBUF, M_NOWAIT);
	if (sc->vr_ldata_ptr == NULL) {
		free(sc, M_DEVBUF);
		printf("%s: no memory for list buffers!\n",
			sc->vr_dev.dv_xname);
		return;
	}

	sc->vr_ldata = (struct vr_list_data *)sc->vr_ldata_ptr;
	round = (unsigned long)sc->vr_ldata_ptr & 0xF;
	roundptr = sc->vr_ldata_ptr;
	for (i = 0; i < 8; i++) {
		if (round % 8) {
			round++;
			roundptr++;
		} else
			break;
	}
	sc->vr_ldata = (struct vr_list_data *)roundptr;
	bzero(sc->vr_ldata, sizeof (struct vr_list_data));

	ifp = &sc->vr_ec.ec_if;
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = vr_ioctl;
	ifp->if_output = ether_output;
	ifp->if_start = vr_start;
	ifp->if_watchdog = vr_watchdog;
	ifp->if_baudrate = 10000000;
	bcopy(sc->vr_dev.dv_xname, ifp->if_xname, IFNAMSIZ);

	/*
	 * Initialize MII/media info.
	 */
	sc->vr_mii.mii_ifp = ifp;
	sc->vr_mii.mii_readreg = vr_mii_readreg;
	sc->vr_mii.mii_writereg = vr_mii_writereg;
	sc->vr_mii.mii_statchg = vr_mii_statchg;
	ifmedia_init(&sc->vr_mii.mii_media, 0, vr_ifmedia_upd, vr_ifmedia_sts);
	mii_phy_probe(&sc->vr_dev, &sc->vr_mii, 0xffffffff);
	if (LIST_FIRST(&sc->vr_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->vr_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->vr_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->vr_mii.mii_media, IFM_ETHER|IFM_AUTO);

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, sc->vr_enaddr);

#if NBPFILTER > 0
	bpfattach(&sc->vr_ec.ec_if.if_bpf,
		ifp, DLT_EN10MB, sizeof (struct ether_header));
#endif

	sc->vr_ats = shutdownhook_establish(vr_shutdown, sc);
	if (sc->vr_ats == NULL)
		printf("%s: warning: couldn't establish shutdown hook\n",
			sc->vr_dev.dv_xname);
}
