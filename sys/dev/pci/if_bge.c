/*	$NetBSD: if_bge.c,v 1.123.2.1 2007/02/27 16:53:59 yamt Exp $	*/

/*
 * Copyright (c) 2001 Wind River Systems
 * Copyright (c) 1997, 1998, 1999, 2001
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
 * $FreeBSD: if_bge.c,v 1.13 2002/04/04 06:01:31 wpaul Exp $
 */

/*
 * Broadcom BCM570x family gigabit ethernet driver for NetBSD.
 *
 * NetBSD version by:
 *
 *	Frank van der Linden <fvdl@wasabisystems.com>
 *	Jason Thorpe <thorpej@wasabisystems.com>
 *	Jonathan Stone <jonathan@dsg.stanford.edu>
 *
 * Originally written for FreeBSD by Bill Paul <wpaul@windriver.com>
 * Senior Engineer, Wind River Systems
 */

/*
 * The Broadcom BCM5700 is based on technology originally developed by
 * Alteon Networks as part of the Tigon I and Tigon II gigabit ethernet
 * MAC chips. The BCM5700, sometimes refered to as the Tigon III, has
 * two on-board MIPS R4000 CPUs and can have as much as 16MB of external
 * SSRAM. The BCM5700 supports TCP, UDP and IP checksum offload, jumbo
 * frames, highly configurable RX filtering, and 16 RX and TX queues
 * (which, along with RX filter rules, can be used for QOS applications).
 * Other features, such as TCP segmentation, may be available as part
 * of value-added firmware updates. Unlike the Tigon I and Tigon II,
 * firmware images can be stored in hardware and need not be compiled
 * into the driver.
 *
 * The BCM5700 supports the PCI v2.2 and PCI-X v1.0 standards, and will
 * function in a 32-bit/64-bit 33/66MHz bus, or a 64-bit/133MHz bus.
 *
 * The BCM5701 is a single-chip solution incorporating both the BCM5700
 * MAC and a BCM5401 10/100/1000 PHY. Unlike the BCM5700, the BCM5701
 * does not support external SSRAM.
 *
 * Broadcom also produces a variation of the BCM5700 under the "Altima"
 * brand name, which is functionally similar but lacks PCI-X support.
 *
 * Without external SSRAM, you can only have at most 4 TX rings,
 * and the use of the mini RX ring is disabled. This seems to imply
 * that these features are simply not available on the BCM5701. As a
 * result, this driver does not implement any support for the mini RX
 * ring.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_bge.c,v 1.123.2.1 2007/02/27 16:53:59 yamt Exp $");

#include "bpfilter.h"
#include "vlan.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

/* Headers for TCP  Segmentation Offload (TSO) */
#include <netinet/in_systm.h>		/* n_time for <netinet/ip.h>... */
#include <netinet/in.h>			/* ip_{src,dst}, for <netinet/ip.h> */
#include <netinet/ip.h>			/* for struct ip */
#include <netinet/tcp.h>		/* for struct tcphdr */


#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>
#include <dev/mii/brgphyreg.h>

#include <dev/pci/if_bgereg.h>

#include <uvm/uvm_extern.h>

#define ETHER_MIN_NOPAD (ETHER_MIN_LEN - ETHER_CRC_LEN) /* i.e., 60 */


/*
 * Tunable thresholds for rx-side bge interrupt mitigation.
 */

/*
 * The pairs of values below were obtained from empirical measurement
 * on bcm5700 rev B2; they ar designed to give roughly 1 receive
 * interrupt for every N packets received, where N is, approximately,
 * the second value (rx_max_bds) in each pair.  The values are chosen
 * such that moving from one pair to the succeeding pair was observed
 * to roughly halve interrupt rate under sustained input packet load.
 * The values were empirically chosen to avoid overflowing internal
 * limits on the  bcm5700: inreasing rx_ticks much beyond 600
 * results in internal wrapping and higher interrupt rates.
 * The limit of 46 frames was chosen to match NFS workloads.
 *
 * These values also work well on bcm5701, bcm5704C, and (less
 * tested) bcm5703.  On other chipsets, (including the Altima chip
 * family), the larger values may overflow internal chip limits,
 * leading to increasing interrupt rates rather than lower interrupt
 * rates.
 *
 * Applications using heavy interrupt mitigation (interrupting every
 * 32 or 46 frames) in both directions may need to increase the TCP
 * windowsize to above 131072 bytes (e.g., to 199608 bytes) to sustain
 * full link bandwidth, due to ACKs and window updates lingering
 * in the RX queue during the 30-to-40-frame interrupt-mitigation window.
 */
static const struct bge_load_rx_thresh {
	int rx_ticks;
	int rx_max_bds; }
bge_rx_threshes[] = {
	{ 32,   2 },
	{ 50,   4 },
	{ 100,  8 },
	{ 192, 16 },
	{ 416, 32 },
	{ 598, 46 }
};
#define NBGE_RX_THRESH (sizeof(bge_rx_threshes) / sizeof(bge_rx_threshes[0]))

/* XXX patchable; should be sysctl'able */
static int	bge_auto_thresh = 1;
static int	bge_rx_thresh_lvl;

static int	bge_rxthresh_nodenum;

static int	bge_probe(device_t, cfdata_t, void *);
static void	bge_attach(device_t, device_t, void *);
static void	bge_powerhook(int, void *);
static void	bge_release_resources(struct bge_softc *);
static void	bge_txeof(struct bge_softc *);
static void	bge_rxeof(struct bge_softc *);

static void	bge_tick(void *);
static void	bge_stats_update(struct bge_softc *);
static int	bge_encap(struct bge_softc *, struct mbuf *, u_int32_t *);

static int	bge_intr(void *);
static void	bge_start(struct ifnet *);
static int	bge_ioctl(struct ifnet *, u_long, caddr_t);
static int	bge_init(struct ifnet *);
static void	bge_stop(struct bge_softc *);
static void	bge_watchdog(struct ifnet *);
static void	bge_shutdown(void *);
static int	bge_ifmedia_upd(struct ifnet *);
static void	bge_ifmedia_sts(struct ifnet *, struct ifmediareq *);

static void	bge_setmulti(struct bge_softc *);

static void	bge_handle_events(struct bge_softc *);
static int	bge_alloc_jumbo_mem(struct bge_softc *);
#if 0 /* XXX */
static void	bge_free_jumbo_mem(struct bge_softc *);
#endif
static void	*bge_jalloc(struct bge_softc *);
static void	bge_jfree(struct mbuf *, caddr_t, size_t, void *);
static int	bge_newbuf_std(struct bge_softc *, int, struct mbuf *,
			       bus_dmamap_t);
static int	bge_newbuf_jumbo(struct bge_softc *, int, struct mbuf *);
static int	bge_init_rx_ring_std(struct bge_softc *);
static void	bge_free_rx_ring_std(struct bge_softc *);
static int	bge_init_rx_ring_jumbo(struct bge_softc *);
static void	bge_free_rx_ring_jumbo(struct bge_softc *);
static void	bge_free_tx_ring(struct bge_softc *);
static int	bge_init_tx_ring(struct bge_softc *);

static int	bge_chipinit(struct bge_softc *);
static int	bge_blockinit(struct bge_softc *);
static int	bge_setpowerstate(struct bge_softc *, int);

static void	bge_reset(struct bge_softc *);

#define BGE_DEBUG
#ifdef BGE_DEBUG
#define DPRINTF(x)	if (bgedebug) printf x
#define DPRINTFN(n,x)	if (bgedebug >= (n)) printf x
#define BGE_TSO_PRINTF(x)  do { if (bge_tso_debug) printf x ;} while (0)
int	bgedebug = 0;
int	bge_tso_debug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#define BGE_TSO_PRINTF(x)
#endif

#ifdef BGE_EVENT_COUNTERS
#define	BGE_EVCNT_INCR(ev)	(ev).ev_count++
#define	BGE_EVCNT_ADD(ev, val)	(ev).ev_count += (val)
#define	BGE_EVCNT_UPD(ev, val)	(ev).ev_count = (val)
#else
#define	BGE_EVCNT_INCR(ev)	/* nothing */
#define	BGE_EVCNT_ADD(ev, val)	/* nothing */
#define	BGE_EVCNT_UPD(ev, val)	/* nothing */
#endif

/* Various chip quirks. */
#define	BGE_QUIRK_LINK_STATE_BROKEN	0x00000001
#define	BGE_QUIRK_CSUM_BROKEN		0x00000002
#define	BGE_QUIRK_ONLY_PHY_1		0x00000004
#define	BGE_QUIRK_5700_SMALLDMA		0x00000008
#define	BGE_QUIRK_5700_PCIX_REG_BUG	0x00000010
#define	BGE_QUIRK_PRODUCER_BUG		0x00000020
#define	BGE_QUIRK_PCIX_DMA_ALIGN_BUG	0x00000040
#define	BGE_QUIRK_5705_CORE		0x00000080
#define	BGE_QUIRK_FEWER_MBUFS		0x00000100

/*
 * XXX: how to handle variants based on 5750 and derivatives:
 * 5750 5751, 5721, possibly 5714, 5752, and 5708?, which
 * in general behave like a 5705, except with additional quirks.
 * This driver's current handling of the 5721 is wrong;
 * how we map ASIC revision to "quirks" needs more thought.
 * (defined here until the thought is done).
 */
#define BGE_IS_5714_FAMILY(sc) \
	(BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5714_A0 || \
	 BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5780 ||	\
	 BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5714 )

#define BGE_IS_5750_OR_BEYOND(sc)  \
	(BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5750 || \
	 BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5752 || \
	 BGE_IS_5714_FAMILY(sc) )

#define BGE_IS_5705_OR_BEYOND(sc)  \
	( ((sc)->bge_quirks & BGE_QUIRK_5705_CORE) || \
	  BGE_IS_5750_OR_BEYOND(sc) )


/* following bugs are common to bcm5700 rev B, all flavours */
#define BGE_QUIRK_5700_COMMON \
	(BGE_QUIRK_5700_SMALLDMA|BGE_QUIRK_PRODUCER_BUG)

CFATTACH_DECL(bge, sizeof(struct bge_softc),
    bge_probe, bge_attach, NULL, NULL);

static u_int32_t
bge_readmem_ind(struct bge_softc *sc, int off)
{
	struct pci_attach_args	*pa = &(sc->bge_pa);
	pcireg_t val;

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MEMWIN_BASEADDR, off);
	val = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_MEMWIN_DATA);
	return val;
}

static void
bge_writemem_ind(struct bge_softc *sc, int off, int val)
{
	struct pci_attach_args	*pa = &(sc->bge_pa);

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MEMWIN_BASEADDR, off);
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MEMWIN_DATA, val);
}

#ifdef notdef
static u_int32_t
bge_readreg_ind(struct bge_softc *sc, int off)
{
	struct pci_attach_args	*pa = &(sc->bge_pa);

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_REG_BASEADDR, off);
	return(pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_REG_DATA));
}
#endif

static void
bge_writereg_ind(struct bge_softc *sc, int off, int val)
{
	struct pci_attach_args	*pa = &(sc->bge_pa);

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_REG_BASEADDR, off);
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_REG_DATA, val);
}

#ifdef notdef
static u_int8_t
bge_vpd_readbyte(struct bge_softc *sc, int addr)
{
	int i;
	u_int32_t val;
	struct pci_attach_args	*pa = &(sc->bge_pa);

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_VPD_ADDR, addr);
	for (i = 0; i < BGE_TIMEOUT * 10; i++) {
		DELAY(10);
		if (pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_VPD_ADDR) &
		    BGE_VPD_FLAG)
			break;
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: VPD read timed out\n", sc->bge_dev.dv_xname);
		return(0);
	}

	val = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_VPD_DATA);

	return((val >> ((addr % 4) * 8)) & 0xFF);
}

static void
bge_vpd_read_res(struct bge_softc *sc, struct vpd_res *res, int addr)
{
	int i;
	u_int8_t *ptr;

	ptr = (u_int8_t *)res;
	for (i = 0; i < sizeof(struct vpd_res); i++)
		ptr[i] = bge_vpd_readbyte(sc, i + addr);
}

static void
bge_vpd_read(struct bge_softc *sc)
{
	int pos = 0, i;
	struct vpd_res res;

	if (sc->bge_vpd_prodname != NULL)
		free(sc->bge_vpd_prodname, M_DEVBUF);
	if (sc->bge_vpd_readonly != NULL)
		free(sc->bge_vpd_readonly, M_DEVBUF);
	sc->bge_vpd_prodname = NULL;
	sc->bge_vpd_readonly = NULL;

	bge_vpd_read_res(sc, &res, pos);

	if (res.vr_id != VPD_RES_ID) {
		printf("%s: bad VPD resource id: expected %x got %x\n",
			sc->bge_dev.dv_xname, VPD_RES_ID, res.vr_id);
		return;
	}

	pos += sizeof(res);
	sc->bge_vpd_prodname = malloc(res.vr_len + 1, M_DEVBUF, M_NOWAIT);
	if (sc->bge_vpd_prodname == NULL)
		panic("bge_vpd_read");
	for (i = 0; i < res.vr_len; i++)
		sc->bge_vpd_prodname[i] = bge_vpd_readbyte(sc, i + pos);
	sc->bge_vpd_prodname[i] = '\0';
	pos += i;

	bge_vpd_read_res(sc, &res, pos);

	if (res.vr_id != VPD_RES_READ) {
		printf("%s: bad VPD resource id: expected %x got %x\n",
		    sc->bge_dev.dv_xname, VPD_RES_READ, res.vr_id);
		return;
	}

	pos += sizeof(res);
	sc->bge_vpd_readonly = malloc(res.vr_len, M_DEVBUF, M_NOWAIT);
	if (sc->bge_vpd_readonly == NULL)
		panic("bge_vpd_read");
	for (i = 0; i < res.vr_len + 1; i++)
		sc->bge_vpd_readonly[i] = bge_vpd_readbyte(sc, i + pos);
}
#endif

/*
 * Read a byte of data stored in the EEPROM at address 'addr.' The
 * BCM570x supports both the traditional bitbang interface and an
 * auto access interface for reading the EEPROM. We use the auto
 * access method.
 */
static u_int8_t
bge_eeprom_getbyte(struct bge_softc *sc, int addr, u_int8_t *dest)
{
	int i;
	u_int32_t byte = 0;

	/*
	 * Enable use of auto EEPROM access so we can avoid
	 * having to use the bitbang method.
	 */
	BGE_SETBIT(sc, BGE_MISC_LOCAL_CTL, BGE_MLC_AUTO_EEPROM);

	/* Reset the EEPROM, load the clock period. */
	CSR_WRITE_4(sc, BGE_EE_ADDR,
	    BGE_EEADDR_RESET|BGE_EEHALFCLK(BGE_HALFCLK_384SCL));
	DELAY(20);

	/* Issue the read EEPROM command. */
	CSR_WRITE_4(sc, BGE_EE_ADDR, BGE_EE_READCMD | addr);

	/* Wait for completion */
	for(i = 0; i < BGE_TIMEOUT * 10; i++) {
		DELAY(10);
		if (CSR_READ_4(sc, BGE_EE_ADDR) & BGE_EEADDR_DONE)
			break;
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: eeprom read timed out\n", sc->bge_dev.dv_xname);
		return(0);
	}

	/* Get result. */
	byte = CSR_READ_4(sc, BGE_EE_DATA);

	*dest = (byte >> ((addr % 4) * 8)) & 0xFF;

	return(0);
}

/*
 * Read a sequence of bytes from the EEPROM.
 */
static int
bge_read_eeprom(struct bge_softc *sc, caddr_t dest, int off, int cnt)
{
	int err = 0, i;
	u_int8_t byte = 0;

	for (i = 0; i < cnt; i++) {
		err = bge_eeprom_getbyte(sc, off + i, &byte);
		if (err)
			break;
		*(dest + i) = byte;
	}

	return(err ? 1 : 0);
}

static int
bge_miibus_readreg(device_t dev, int phy, int reg)
{
	struct bge_softc *sc = (struct bge_softc *)dev;
	u_int32_t val;
	u_int32_t saved_autopoll;
	int i;

	/*
	 * Several chips with builtin PHYs will incorrectly answer to
	 * other PHY instances than the builtin PHY at id 1.
	 */
	if (phy != 1 && (sc->bge_quirks & BGE_QUIRK_ONLY_PHY_1))
		return(0);

	/* Reading with autopolling on may trigger PCI errors */
	saved_autopoll = CSR_READ_4(sc, BGE_MI_MODE);
	if (saved_autopoll & BGE_MIMODE_AUTOPOLL) {
		CSR_WRITE_4(sc, BGE_MI_MODE,
		    saved_autopoll &~ BGE_MIMODE_AUTOPOLL);
		DELAY(40);
	}

	CSR_WRITE_4(sc, BGE_MI_COMM, BGE_MICMD_READ|BGE_MICOMM_BUSY|
	    BGE_MIPHY(phy)|BGE_MIREG(reg));

	for (i = 0; i < BGE_TIMEOUT; i++) {
		val = CSR_READ_4(sc, BGE_MI_COMM);
		if (!(val & BGE_MICOMM_BUSY))
			break;
		delay(10);
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: PHY read timed out\n", sc->bge_dev.dv_xname);
		val = 0;
		goto done;
	}

	val = CSR_READ_4(sc, BGE_MI_COMM);

done:
	if (saved_autopoll & BGE_MIMODE_AUTOPOLL) {
		CSR_WRITE_4(sc, BGE_MI_MODE, saved_autopoll);
		DELAY(40);
	}

	if (val & BGE_MICOMM_READFAIL)
		return(0);

	return(val & 0xFFFF);
}

static void
bge_miibus_writereg(device_t dev, int phy, int reg, int val)
{
	struct bge_softc *sc = (struct bge_softc *)dev;
	u_int32_t saved_autopoll;
	int i;

	/* Touching the PHY while autopolling is on may trigger PCI errors */
	saved_autopoll = CSR_READ_4(sc, BGE_MI_MODE);
	if (saved_autopoll & BGE_MIMODE_AUTOPOLL) {
		delay(40);
		CSR_WRITE_4(sc, BGE_MI_MODE,
		    saved_autopoll & (~BGE_MIMODE_AUTOPOLL));
		delay(10); /* 40 usec is supposed to be adequate */
	}

	CSR_WRITE_4(sc, BGE_MI_COMM, BGE_MICMD_WRITE|BGE_MICOMM_BUSY|
	    BGE_MIPHY(phy)|BGE_MIREG(reg)|val);

	for (i = 0; i < BGE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, BGE_MI_COMM) & BGE_MICOMM_BUSY))
			break;
		delay(10);
	}

	if (saved_autopoll & BGE_MIMODE_AUTOPOLL) {
		CSR_WRITE_4(sc, BGE_MI_MODE, saved_autopoll);
		delay(40);
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: PHY read timed out\n", sc->bge_dev.dv_xname);
	}
}

static void
bge_miibus_statchg(device_t dev)
{
	struct bge_softc *sc = (struct bge_softc *)dev;
	struct mii_data *mii = &sc->bge_mii;

	/*
	 * Get flow control negotiation result.
	 */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->bge_flowflags) {
		sc->bge_flowflags = mii->mii_media_active & IFM_ETH_FMASK;
		mii->mii_media_active &= ~IFM_ETH_FMASK;
	}

	BGE_CLRBIT(sc, BGE_MAC_MODE, BGE_MACMODE_PORTMODE);
	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
		BGE_SETBIT(sc, BGE_MAC_MODE, BGE_PORTMODE_GMII);
	} else {
		BGE_SETBIT(sc, BGE_MAC_MODE, BGE_PORTMODE_MII);
	}

	if ((mii->mii_media_active & IFM_GMASK) == IFM_FDX) {
		BGE_CLRBIT(sc, BGE_MAC_MODE, BGE_MACMODE_HALF_DUPLEX);
	} else {
		BGE_SETBIT(sc, BGE_MAC_MODE, BGE_MACMODE_HALF_DUPLEX);
	}

	/*
	 * 802.3x flow control
	 */
	if (sc->bge_flowflags & IFM_ETH_RXPAUSE) {
		BGE_SETBIT(sc, BGE_RX_MODE, BGE_RXMODE_FLOWCTL_ENABLE);
	} else {
		BGE_CLRBIT(sc, BGE_RX_MODE, BGE_RXMODE_FLOWCTL_ENABLE);
	}
	if (sc->bge_flowflags & IFM_ETH_TXPAUSE) {
		BGE_SETBIT(sc, BGE_TX_MODE, BGE_TXMODE_FLOWCTL_ENABLE);
	} else {
		BGE_CLRBIT(sc, BGE_TX_MODE, BGE_TXMODE_FLOWCTL_ENABLE);
	}
}

/*
 * Update rx threshold levels to values in a particular slot
 * of the interrupt-mitigation table bge_rx_threshes.
 */
static void
bge_set_thresh(struct ifnet *ifp, int lvl)
{
	struct bge_softc *sc = ifp->if_softc;
	int s;

	/* For now, just save the new Rx-intr thresholds and record
	 * that a threshold update is pending.  Updating the hardware
	 * registers here (even at splhigh()) is observed to
	 * occasionaly cause glitches where Rx-interrupts are not
	 * honoured for up to 10 seconds. jonathan@NetBSD.org, 2003-04-05
	 */
	s = splnet();
	sc->bge_rx_coal_ticks = bge_rx_threshes[lvl].rx_ticks;
	sc->bge_rx_max_coal_bds = bge_rx_threshes[lvl].rx_max_bds;
	sc->bge_pending_rxintr_change = 1;
	splx(s);

	 return;
}


/*
 * Update Rx thresholds of all bge devices
 */
static void
bge_update_all_threshes(int lvl)
{
	struct ifnet *ifp;
	const char * const namebuf = "bge";
	int namelen;

	if (lvl < 0)
		lvl = 0;
	else if( lvl >= NBGE_RX_THRESH)
		lvl = NBGE_RX_THRESH - 1;

	namelen = strlen(namebuf);
	/*
	 * Now search all the interfaces for this name/number
	 */
	IFNET_FOREACH(ifp) {
		if (strncmp(ifp->if_xname, namebuf, namelen) != 0)
		      continue;
		/* We got a match: update if doing auto-threshold-tuning */
		if (bge_auto_thresh)
			bge_set_thresh(ifp, lvl);
	}
}

/*
 * Handle events that have triggered interrupts.
 */
static void
bge_handle_events(struct bge_softc *sc)
{

	return;
}

/*
 * Memory management for jumbo frames.
 */

static int
bge_alloc_jumbo_mem(struct bge_softc *sc)
{
	caddr_t			ptr, kva;
	bus_dma_segment_t	seg;
	int		i, rseg, state, error;
	struct bge_jpool_entry   *entry;

	state = error = 0;

	/* Grab a big chunk o' storage. */
	if (bus_dmamem_alloc(sc->bge_dmatag, BGE_JMEM, PAGE_SIZE, 0,
	     &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf("%s: can't alloc rx buffers\n", sc->bge_dev.dv_xname);
		return ENOBUFS;
	}

	state = 1;
	if (bus_dmamem_map(sc->bge_dmatag, &seg, rseg, BGE_JMEM, &kva,
	    BUS_DMA_NOWAIT)) {
		printf("%s: can't map DMA buffers (%d bytes)\n",
		    sc->bge_dev.dv_xname, (int)BGE_JMEM);
		error = ENOBUFS;
		goto out;
	}

	state = 2;
	if (bus_dmamap_create(sc->bge_dmatag, BGE_JMEM, 1, BGE_JMEM, 0,
	    BUS_DMA_NOWAIT, &sc->bge_cdata.bge_rx_jumbo_map)) {
		printf("%s: can't create DMA map\n", sc->bge_dev.dv_xname);
		error = ENOBUFS;
		goto out;
	}

	state = 3;
	if (bus_dmamap_load(sc->bge_dmatag, sc->bge_cdata.bge_rx_jumbo_map,
	    kva, BGE_JMEM, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: can't load DMA map\n", sc->bge_dev.dv_xname);
		error = ENOBUFS;
		goto out;
	}

	state = 4;
	sc->bge_cdata.bge_jumbo_buf = (caddr_t)kva;
	DPRINTFN(1,("bge_jumbo_buf = %p\n", sc->bge_cdata.bge_jumbo_buf));

	SLIST_INIT(&sc->bge_jfree_listhead);
	SLIST_INIT(&sc->bge_jinuse_listhead);

	/*
	 * Now divide it up into 9K pieces and save the addresses
	 * in an array.
	 */
	ptr = sc->bge_cdata.bge_jumbo_buf;
	for (i = 0; i < BGE_JSLOTS; i++) {
		sc->bge_cdata.bge_jslots[i] = ptr;
		ptr += BGE_JLEN;
		entry = malloc(sizeof(struct bge_jpool_entry),
		    M_DEVBUF, M_NOWAIT);
		if (entry == NULL) {
			printf("%s: no memory for jumbo buffer queue!\n",
			    sc->bge_dev.dv_xname);
			error = ENOBUFS;
			goto out;
		}
		entry->slot = i;
		SLIST_INSERT_HEAD(&sc->bge_jfree_listhead,
				 entry, jpool_entries);
	}
out:
	if (error != 0) {
		switch (state) {
		case 4:
			bus_dmamap_unload(sc->bge_dmatag,
			    sc->bge_cdata.bge_rx_jumbo_map);
		case 3:
			bus_dmamap_destroy(sc->bge_dmatag,
			    sc->bge_cdata.bge_rx_jumbo_map);
		case 2:
			bus_dmamem_unmap(sc->bge_dmatag, kva, BGE_JMEM);
		case 1:
			bus_dmamem_free(sc->bge_dmatag, &seg, rseg);
			break;
		default:
			break;
		}
	}

	return error;
}

/*
 * Allocate a jumbo buffer.
 */
static void *
bge_jalloc(struct bge_softc *sc)
{
	struct bge_jpool_entry   *entry;

	entry = SLIST_FIRST(&sc->bge_jfree_listhead);

	if (entry == NULL) {
		printf("%s: no free jumbo buffers\n", sc->bge_dev.dv_xname);
		return(NULL);
	}

	SLIST_REMOVE_HEAD(&sc->bge_jfree_listhead, jpool_entries);
	SLIST_INSERT_HEAD(&sc->bge_jinuse_listhead, entry, jpool_entries);
	return(sc->bge_cdata.bge_jslots[entry->slot]);
}

/*
 * Release a jumbo buffer.
 */
static void
bge_jfree(struct mbuf *m, caddr_t buf, size_t size, void *arg)
{
	struct bge_jpool_entry *entry;
	struct bge_softc *sc;
	int i, s;

	/* Extract the softc struct pointer. */
	sc = (struct bge_softc *)arg;

	if (sc == NULL)
		panic("bge_jfree: can't find softc pointer!");

	/* calculate the slot this buffer belongs to */

	i = ((caddr_t)buf
	     - (caddr_t)sc->bge_cdata.bge_jumbo_buf) / BGE_JLEN;

	if ((i < 0) || (i >= BGE_JSLOTS))
		panic("bge_jfree: asked to free buffer that we don't manage!");

	s = splvm();
	entry = SLIST_FIRST(&sc->bge_jinuse_listhead);
	if (entry == NULL)
		panic("bge_jfree: buffer not in use!");
	entry->slot = i;
	SLIST_REMOVE_HEAD(&sc->bge_jinuse_listhead, jpool_entries);
	SLIST_INSERT_HEAD(&sc->bge_jfree_listhead, entry, jpool_entries);

	if (__predict_true(m != NULL))
  		pool_cache_put(&mbpool_cache, m);
	splx(s);
}


/*
 * Intialize a standard receive ring descriptor.
 */
static int
bge_newbuf_std(struct bge_softc *sc, int i, struct mbuf *m, bus_dmamap_t dmamap)
{
	struct mbuf		*m_new = NULL;
	struct bge_rx_bd	*r;
	int			error;

	if (dmamap == NULL) {
		error = bus_dmamap_create(sc->bge_dmatag, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &dmamap);
		if (error != 0)
			return error;
	}

	sc->bge_cdata.bge_rx_std_map[i] = dmamap;

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			return(ENOBUFS);
		}

		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			m_freem(m_new);
			return(ENOBUFS);
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;

	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}
	if (!sc->bge_rx_alignment_bug)
	    m_adj(m_new, ETHER_ALIGN);
	if (bus_dmamap_load_mbuf(sc->bge_dmatag, dmamap, m_new,
	    BUS_DMA_READ|BUS_DMA_NOWAIT))
		return(ENOBUFS);
	bus_dmamap_sync(sc->bge_dmatag, dmamap, 0, dmamap->dm_mapsize, 
	    BUS_DMASYNC_PREREAD);

	sc->bge_cdata.bge_rx_std_chain[i] = m_new;
	r = &sc->bge_rdata->bge_rx_std_ring[i];
	bge_set_hostaddr(&r->bge_addr,
	    dmamap->dm_segs[0].ds_addr);
	r->bge_flags = BGE_RXBDFLAG_END;
	r->bge_len = m_new->m_len;
	r->bge_idx = i;

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offsetof(struct bge_ring_data, bge_rx_std_ring) +
		i * sizeof (struct bge_rx_bd),
	    sizeof (struct bge_rx_bd),
	    BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);

	return(0);
}

/*
 * Initialize a jumbo receive ring descriptor. This allocates
 * a jumbo buffer from the pool managed internally by the driver.
 */
static int
bge_newbuf_jumbo(struct bge_softc *sc, int i, struct mbuf *m)
{
	struct mbuf *m_new = NULL;
	struct bge_rx_bd *r;
	caddr_t buf = NULL;

	if (m == NULL) {

		/* Allocate the mbuf. */
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			return(ENOBUFS);
		}

		/* Allocate the jumbo buffer */
		buf = bge_jalloc(sc);
		if (buf == NULL) {
			m_freem(m_new);
			printf("%s: jumbo allocation failed "
			    "-- packet dropped!\n", sc->bge_dev.dv_xname);
			return(ENOBUFS);
		}

		/* Attach the buffer to the mbuf. */
		m_new->m_len = m_new->m_pkthdr.len = BGE_JUMBO_FRAMELEN;
		MEXTADD(m_new, buf, BGE_JUMBO_FRAMELEN, M_DEVBUF,
		    bge_jfree, sc);
		m_new->m_flags |= M_EXT_RW;
	} else {
		m_new = m;
		buf = m_new->m_data = m_new->m_ext.ext_buf;
		m_new->m_ext.ext_size = BGE_JUMBO_FRAMELEN;
	}
	if (!sc->bge_rx_alignment_bug)
	    m_adj(m_new, ETHER_ALIGN);
	bus_dmamap_sync(sc->bge_dmatag, sc->bge_cdata.bge_rx_jumbo_map,
	    mtod(m_new, caddr_t) - sc->bge_cdata.bge_jumbo_buf, BGE_JLEN,
	    BUS_DMASYNC_PREREAD);
	/* Set up the descriptor. */
	r = &sc->bge_rdata->bge_rx_jumbo_ring[i];
	sc->bge_cdata.bge_rx_jumbo_chain[i] = m_new;
	bge_set_hostaddr(&r->bge_addr, BGE_JUMBO_DMA_ADDR(sc, m_new));
	r->bge_flags = BGE_RXBDFLAG_END|BGE_RXBDFLAG_JUMBO_RING;
	r->bge_len = m_new->m_len;
	r->bge_idx = i;

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offsetof(struct bge_ring_data, bge_rx_jumbo_ring) +
		i * sizeof (struct bge_rx_bd),
	    sizeof (struct bge_rx_bd),
	    BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);

	return(0);
}

/*
 * The standard receive ring has 512 entries in it. At 2K per mbuf cluster,
 * that's 1MB or memory, which is a lot. For now, we fill only the first
 * 256 ring entries and hope that our CPU is fast enough to keep up with
 * the NIC.
 */
static int
bge_init_rx_ring_std(struct bge_softc *sc)
{
	int i;

	if (sc->bge_flags & BGE_RXRING_VALID)
		return 0;

	for (i = 0; i < BGE_SSLOTS; i++) {
		if (bge_newbuf_std(sc, i, NULL, 0) == ENOBUFS)
			return(ENOBUFS);
	}

	sc->bge_std = i - 1;
	CSR_WRITE_4(sc, BGE_MBX_RX_STD_PROD_LO, sc->bge_std);

	sc->bge_flags |= BGE_RXRING_VALID;

	return(0);
}

static void
bge_free_rx_ring_std(struct bge_softc *sc)
{
	int i;

	if (!(sc->bge_flags & BGE_RXRING_VALID))
		return;

	for (i = 0; i < BGE_STD_RX_RING_CNT; i++) {
		if (sc->bge_cdata.bge_rx_std_chain[i] != NULL) {
			m_freem(sc->bge_cdata.bge_rx_std_chain[i]);
			sc->bge_cdata.bge_rx_std_chain[i] = NULL;
			bus_dmamap_destroy(sc->bge_dmatag,
			    sc->bge_cdata.bge_rx_std_map[i]);
		}
		memset((char *)&sc->bge_rdata->bge_rx_std_ring[i], 0,
		    sizeof(struct bge_rx_bd));
	}

	sc->bge_flags &= ~BGE_RXRING_VALID;
}

static int
bge_init_rx_ring_jumbo(struct bge_softc *sc)
{
	int i;
	volatile struct bge_rcb *rcb;

	if (sc->bge_flags & BGE_JUMBO_RXRING_VALID)
		return 0;

	for (i = 0; i < BGE_JUMBO_RX_RING_CNT; i++) {
		if (bge_newbuf_jumbo(sc, i, NULL) == ENOBUFS)
			return(ENOBUFS);
	};

	sc->bge_jumbo = i - 1;
	sc->bge_flags |= BGE_JUMBO_RXRING_VALID;

	rcb = &sc->bge_rdata->bge_info.bge_jumbo_rx_rcb;
	rcb->bge_maxlen_flags = 0;
	CSR_WRITE_4(sc, BGE_RX_JUMBO_RCB_MAXLEN_FLAGS, rcb->bge_maxlen_flags);

	CSR_WRITE_4(sc, BGE_MBX_RX_JUMBO_PROD_LO, sc->bge_jumbo);

	return(0);
}

static void
bge_free_rx_ring_jumbo(struct bge_softc *sc)
{
	int i;

	if (!(sc->bge_flags & BGE_JUMBO_RXRING_VALID))
		return;

	for (i = 0; i < BGE_JUMBO_RX_RING_CNT; i++) {
		if (sc->bge_cdata.bge_rx_jumbo_chain[i] != NULL) {
			m_freem(sc->bge_cdata.bge_rx_jumbo_chain[i]);
			sc->bge_cdata.bge_rx_jumbo_chain[i] = NULL;
		}
		memset((char *)&sc->bge_rdata->bge_rx_jumbo_ring[i], 0,
		    sizeof(struct bge_rx_bd));
	}

	sc->bge_flags &= ~BGE_JUMBO_RXRING_VALID;
}

static void
bge_free_tx_ring(struct bge_softc *sc)
{
	int i, freed;
	struct txdmamap_pool_entry *dma;

	if (!(sc->bge_flags & BGE_TXRING_VALID))
		return;

	freed = 0;

	for (i = 0; i < BGE_TX_RING_CNT; i++) {
		if (sc->bge_cdata.bge_tx_chain[i] != NULL) {
			freed++;
			m_freem(sc->bge_cdata.bge_tx_chain[i]);
			sc->bge_cdata.bge_tx_chain[i] = NULL;
			SLIST_INSERT_HEAD(&sc->txdma_list, sc->txdma[i],
					    link);
			sc->txdma[i] = 0;
		}
		memset((char *)&sc->bge_rdata->bge_tx_ring[i], 0,
		    sizeof(struct bge_tx_bd));
	}

	while ((dma = SLIST_FIRST(&sc->txdma_list))) {
		SLIST_REMOVE_HEAD(&sc->txdma_list, link);
		bus_dmamap_destroy(sc->bge_dmatag, dma->dmamap);
		free(dma, M_DEVBUF);
	}

	sc->bge_flags &= ~BGE_TXRING_VALID;
}

static int
bge_init_tx_ring(struct bge_softc *sc)
{
	int i;
	bus_dmamap_t dmamap;
	struct txdmamap_pool_entry *dma;

	if (sc->bge_flags & BGE_TXRING_VALID)
		return 0;

	sc->bge_txcnt = 0;
	sc->bge_tx_saved_considx = 0;

	/* Initialize transmit producer index for host-memory send ring. */
	sc->bge_tx_prodidx = 0;
	CSR_WRITE_4(sc, BGE_MBX_TX_HOST_PROD0_LO, sc->bge_tx_prodidx);
	if (sc->bge_quirks & BGE_QUIRK_PRODUCER_BUG)	/* 5700 b2 errata */
		CSR_WRITE_4(sc, BGE_MBX_TX_HOST_PROD0_LO, sc->bge_tx_prodidx);

	/* NIC-memory send ring  not used; initialize to zero. */
	CSR_WRITE_4(sc, BGE_MBX_TX_NIC_PROD0_LO, 0);
	if (sc->bge_quirks & BGE_QUIRK_PRODUCER_BUG)	/* 5700 b2 errata */
		CSR_WRITE_4(sc, BGE_MBX_TX_HOST_PROD0_LO, 0);

	SLIST_INIT(&sc->txdma_list);
	for (i = 0; i < BGE_RSLOTS; i++) {
		if (bus_dmamap_create(sc->bge_dmatag, BGE_TXDMA_MAX,
		    BGE_NTXSEG, ETHER_MAX_LEN_JUMBO, 0, BUS_DMA_NOWAIT,
		    &dmamap))
			return(ENOBUFS);
		if (dmamap == NULL)
			panic("dmamap NULL in bge_init_tx_ring");
		dma = malloc(sizeof(*dma), M_DEVBUF, M_NOWAIT);
		if (dma == NULL) {
			printf("%s: can't alloc txdmamap_pool_entry\n",
			    sc->bge_dev.dv_xname);
			bus_dmamap_destroy(sc->bge_dmatag, dmamap);
			return (ENOMEM);
		}
		dma->dmamap = dmamap;
		SLIST_INSERT_HEAD(&sc->txdma_list, dma, link);
	}

	sc->bge_flags |= BGE_TXRING_VALID;

	return(0);
}

static void
bge_setmulti(struct bge_softc *sc)
{
	struct ethercom		*ac = &sc->ethercom;
	struct ifnet		*ifp = &ac->ec_if;
	struct ether_multi	*enm;
	struct ether_multistep  step;
	u_int32_t		hashes[4] = { 0, 0, 0, 0 };
	u_int32_t		h;
	int			i;

	if (ifp->if_flags & IFF_PROMISC)
		goto allmulti;

	/* Now program new ones. */
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}

		h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 7 least-significant bits. */
		h &= 0x7f;

		hashes[(h & 0x60) >> 5] |= 1 << (h & 0x1F);
		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	hashes[0] = hashes[1] = hashes[2] = hashes[3] = 0xffffffff;

 setit:
	for (i = 0; i < 4; i++)
		CSR_WRITE_4(sc, BGE_MAR0 + (i * 4), hashes[i]);
}

const int bge_swapbits[] = {
	0,
	BGE_MODECTL_BYTESWAP_DATA,
	BGE_MODECTL_WORDSWAP_DATA,
	BGE_MODECTL_BYTESWAP_NONFRAME,
	BGE_MODECTL_WORDSWAP_NONFRAME,

	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_WORDSWAP_DATA,
	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_BYTESWAP_NONFRAME,
	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_WORDSWAP_NONFRAME,

	BGE_MODECTL_WORDSWAP_DATA|BGE_MODECTL_BYTESWAP_NONFRAME,
	BGE_MODECTL_WORDSWAP_DATA|BGE_MODECTL_WORDSWAP_NONFRAME,

	BGE_MODECTL_BYTESWAP_NONFRAME|BGE_MODECTL_WORDSWAP_NONFRAME,

	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_WORDSWAP_DATA|
	    BGE_MODECTL_BYTESWAP_NONFRAME,
	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_WORDSWAP_DATA|
	    BGE_MODECTL_WORDSWAP_NONFRAME,
	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_BYTESWAP_NONFRAME|
	    BGE_MODECTL_WORDSWAP_NONFRAME,
	BGE_MODECTL_WORDSWAP_DATA|BGE_MODECTL_BYTESWAP_NONFRAME|
	    BGE_MODECTL_WORDSWAP_NONFRAME,

	BGE_MODECTL_BYTESWAP_DATA|BGE_MODECTL_WORDSWAP_DATA|
	    BGE_MODECTL_BYTESWAP_NONFRAME|BGE_MODECTL_WORDSWAP_NONFRAME,
};

int bge_swapindex = 0;

/*
 * Do endian, PCI and DMA initialization. Also check the on-board ROM
 * self-test results.
 */
static int
bge_chipinit(struct bge_softc *sc)
{
	u_int32_t		cachesize;
	int			i;
	u_int32_t		dma_rw_ctl;
	struct pci_attach_args	*pa = &(sc->bge_pa);


	/* Set endianness before we access any non-PCI registers. */
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MISC_CTL,
	    BGE_INIT);

	/* Set power state to D0. */
	bge_setpowerstate(sc, 0);

	/*
	 * Check the 'ROM failed' bit on the RX CPU to see if
	 * self-tests passed.
	 */
	if (CSR_READ_4(sc, BGE_RXCPU_MODE) & BGE_RXCPUMODE_ROMFAIL) {
		printf("%s: RX CPU self-diagnostics failed!\n",
		    sc->bge_dev.dv_xname);
		return(ENODEV);
	}

	/* Clear the MAC control register */
	CSR_WRITE_4(sc, BGE_MAC_MODE, 0);

	/*
	 * Clear the MAC statistics block in the NIC's
	 * internal memory.
	 */
	for (i = BGE_STATS_BLOCK;
	    i < BGE_STATS_BLOCK_END + 1; i += sizeof(u_int32_t))
		BGE_MEMWIN_WRITE(pa->pa_pc, pa->pa_tag, i, 0);

	for (i = BGE_STATUS_BLOCK;
	    i < BGE_STATUS_BLOCK_END + 1; i += sizeof(u_int32_t))
		BGE_MEMWIN_WRITE(pa->pa_pc, pa->pa_tag, i, 0);

	/* Set up the PCI DMA control register. */
	if (sc->bge_pcie) {
	  u_int32_t device_ctl;

		/* From FreeBSD */
		DPRINTFN(4, ("(%s: PCI-Express DMA setting)\n",
		    sc->bge_dev.dv_xname));
		dma_rw_ctl = (BGE_PCI_READ_CMD | BGE_PCI_WRITE_CMD |
		    (0xf << BGE_PCIDMARWCTL_RD_WAT_SHIFT) |
		    (0x2 << BGE_PCIDMARWCTL_WR_WAT_SHIFT));

		/* jonathan: alternative from Linux driver */
#define DMA_CTRL_WRITE_PCIE_H20MARK_128         0x00180000
#define DMA_CTRL_WRITE_PCIE_H20MARK_256         0x00380000

		dma_rw_ctl =   0x76000000; /* XXX XXX XXX */;
		device_ctl = pci_conf_read(pa->pa_pc, pa->pa_tag,
					   BGE_PCI_CONF_DEV_CTRL);
		aprint_debug("%s: pcie mode=0x%x\n", sc->bge_dev.dv_xname,
		    device_ctl);

		if ((device_ctl & 0x00e0) && 0) {
			/*
			 * XXX jonathan@NetBSD.org:
			 * This clause is exactly what the Broadcom-supplied
			 * Linux does; but given overall register programming
			 * by if_bge(4), this larger DMA-write watermark
			 * value causes bcm5721 chips to totally wedge.
			 */
			dma_rw_ctl |= BGE_PCIDMA_RWCTL_PCIE_WRITE_WATRMARK_256;
		} else {
			dma_rw_ctl |= BGE_PCIDMA_RWCTL_PCIE_WRITE_WATRMARK_128;
		}
	} else if (pci_conf_read(pa->pa_pc, pa->pa_tag,BGE_PCI_PCISTATE) &
	    BGE_PCISTATE_PCI_BUSMODE) {
		/* Conventional PCI bus */
	  	DPRINTFN(4, ("(%s: PCI 2.2 DMA setting)\n", sc->bge_dev.dv_xname));
		dma_rw_ctl = (BGE_PCI_READ_CMD | BGE_PCI_WRITE_CMD |
		   (0x7 << BGE_PCIDMARWCTL_RD_WAT_SHIFT) |
		   (0x7 << BGE_PCIDMARWCTL_WR_WAT_SHIFT));
		if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
			dma_rw_ctl |= 0x0F;
		}
	} else {
	  	DPRINTFN(4, ("(:%s: PCI-X DMA setting)\n", sc->bge_dev.dv_xname));
		/* PCI-X bus */
		dma_rw_ctl = BGE_PCI_READ_CMD|BGE_PCI_WRITE_CMD |
		    (0x3 << BGE_PCIDMARWCTL_RD_WAT_SHIFT) |
		    (0x3 << BGE_PCIDMARWCTL_WR_WAT_SHIFT) |
		    (0x0F);
		/*
		 * 5703 and 5704 need ONEDMA_AT_ONCE as a workaround
		 * for hardware bugs, which means we should also clear
		 * the low-order MINDMA bits.  In addition, the 5704
		 * uses a different encoding of read/write watermarks.
		 */
		if (BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5704) {
			dma_rw_ctl = BGE_PCI_READ_CMD|BGE_PCI_WRITE_CMD |
			  /* should be 0x1f0000 */
			  (0x7 << BGE_PCIDMARWCTL_RD_WAT_SHIFT) |
			  (0x3 << BGE_PCIDMARWCTL_WR_WAT_SHIFT);
			dma_rw_ctl |= BGE_PCIDMARWCTL_ONEDMA_ATONCE;
		}
		else if (BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5703) {
			dma_rw_ctl &=  0xfffffff0;
			dma_rw_ctl |= BGE_PCIDMARWCTL_ONEDMA_ATONCE;
		}
		else if (BGE_IS_5714_FAMILY(sc)) {
			dma_rw_ctl = BGE_PCI_READ_CMD|BGE_PCI_WRITE_CMD;
			dma_rw_ctl &= ~BGE_PCIDMARWCTL_ONEDMA_ATONCE; /* XXX */
			/* XXX magic values, Broadcom-supplied Linux driver */
			if (BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5780)
				dma_rw_ctl |= (1 << 20) | (1 << 18) |
				  BGE_PCIDMARWCTL_ONEDMA_ATONCE;
			else
				dma_rw_ctl |= (1<<20) | (1<<18) | (1 << 15);
		}
	}

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL, dma_rw_ctl);

	/*
	 * Set up general mode register.
	 */
	CSR_WRITE_4(sc, BGE_MODE_CTL, BGE_DMA_SWAP_OPTIONS|
		    BGE_MODECTL_MAC_ATTN_INTR|BGE_MODECTL_HOST_SEND_BDS|
		    BGE_MODECTL_TX_NO_PHDR_CSUM|BGE_MODECTL_RX_NO_PHDR_CSUM);

	/* Get cache line size. */
	cachesize = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_CACHESZ);

	/*
	 * Avoid violating PCI spec on certain chip revs.
	 */
	if (pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_CMD) &
	    PCIM_CMD_MWIEN) {
		switch(cachesize) {
		case 1:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_16BYTES);
			break;
		case 2:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_32BYTES);
			break;
		case 4:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_64BYTES);
			break;
		case 8:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_128BYTES);
			break;
		case 16:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_256BYTES);
			break;
		case 32:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_512BYTES);
			break;
		case 64:
			PCI_SETBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_DMA_RW_CTL,
				   BGE_PCI_WRITE_BNDRY_1024BYTES);
			break;
		default:
		/* Disable PCI memory write and invalidate. */
#if 0
			if (bootverbose)
				printf("%s: cache line size %d not "
				    "supported; disabling PCI MWI\n",
				    sc->bge_dev.dv_xname, cachesize);
#endif
			PCI_CLRBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_CMD,
			    PCIM_CMD_MWIEN);
			break;
		}
	}

	/*
	 * Disable memory write invalidate.  Apparently it is not supported
	 * properly by these devices.
	 */
	PCI_CLRBIT(pa->pa_pc, pa->pa_tag, BGE_PCI_CMD, PCIM_CMD_MWIEN);


#ifdef __brokenalpha__
	/*
	 * Must insure that we do not cross an 8K (bytes) boundary
	 * for DMA reads.  Our highest limit is 1K bytes.  This is a
	 * restriction on some ALPHA platforms with early revision
	 * 21174 PCI chipsets, such as the AlphaPC 164lx
	 */
	PCI_SETBIT(sc, BGE_PCI_DMA_RW_CTL, BGE_PCI_READ_BNDRY_1024, 4);
#endif

	/* Set the timer prescaler (always 66MHz) */
	CSR_WRITE_4(sc, BGE_MISC_CFG, 65 << 1/*BGE_32BITTIME_66MHZ*/);

	return(0);
}

static int
bge_blockinit(struct bge_softc *sc)
{
	volatile struct bge_rcb		*rcb;
	bus_size_t		rcb_addr;
	int			i;
	struct ifnet		*ifp = &sc->ethercom.ec_if;
	bge_hostaddr		taddr;

	/*
	 * Initialize the memory window pointer register so that
	 * we can access the first 32K of internal NIC RAM. This will
	 * allow us to set up the TX send ring RCBs and the RX return
	 * ring RCBs, plus other things which live in NIC memory.
	 */

	pci_conf_write(sc->bge_pa.pa_pc, sc->bge_pa.pa_tag,
	    BGE_PCI_MEMWIN_BASEADDR, 0);

	/* Configure mbuf memory pool */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		if (sc->bge_extram) {
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_BASEADDR,
			    BGE_EXT_SSRAM);
			if ((sc->bge_quirks & BGE_QUIRK_FEWER_MBUFS) != 0)
				CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_LEN, 0x10000);
			else
				CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_LEN, 0x18000);
		} else {
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_BASEADDR,
			    BGE_BUFFPOOL_1);
			if ((sc->bge_quirks & BGE_QUIRK_FEWER_MBUFS) != 0)
				CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_LEN, 0x10000);
			else
				CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_LEN, 0x18000);
		}

		/* Configure DMA resource pool */
		CSR_WRITE_4(sc, BGE_BMAN_DMA_DESCPOOL_BASEADDR,
		    BGE_DMA_DESCRIPTORS);
		CSR_WRITE_4(sc, BGE_BMAN_DMA_DESCPOOL_LEN, 0x2000);
	}

	/* Configure mbuf pool watermarks */
#ifdef ORIG_WPAUL_VALUES
	CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_READDMA_LOWAT, 24);
	CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_MACRX_LOWAT, 24);
	CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_HIWAT, 48);
#else
	/* new broadcom docs strongly recommend these: */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		if (ifp->if_mtu > ETHER_MAX_LEN) {
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_READDMA_LOWAT, 0x50);
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_MACRX_LOWAT, 0x20);
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_HIWAT, 0x60);
		} else {
			/* Values from Linux driver... */
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_READDMA_LOWAT, 304);
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_MACRX_LOWAT, 152);
			CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_HIWAT, 380);
		}
	} else {
		CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_READDMA_LOWAT, 0x0);
		CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_MACRX_LOWAT, 0x10);
		CSR_WRITE_4(sc, BGE_BMAN_MBUFPOOL_HIWAT, 0x60);
	}
#endif

	/* Configure DMA resource watermarks */
	CSR_WRITE_4(sc, BGE_BMAN_DMA_DESCPOOL_LOWAT, 5);
	CSR_WRITE_4(sc, BGE_BMAN_DMA_DESCPOOL_HIWAT, 10);

	/* Enable buffer manager */
	CSR_WRITE_4(sc, BGE_BMAN_MODE,
	    BGE_BMANMODE_ENABLE|BGE_BMANMODE_LOMBUF_ATTN);

	/* Poll for buffer manager start indication */
	for (i = 0; i < BGE_TIMEOUT; i++) {
		if (CSR_READ_4(sc, BGE_BMAN_MODE) & BGE_BMANMODE_ENABLE)
			break;
		DELAY(10);
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: buffer manager failed to start\n",
		    sc->bge_dev.dv_xname);
		return(ENXIO);
	}

	/* Enable flow-through queues */
	CSR_WRITE_4(sc, BGE_FTQ_RESET, 0xFFFFFFFF);
	CSR_WRITE_4(sc, BGE_FTQ_RESET, 0);

	/* Wait until queue initialization is complete */
	for (i = 0; i < BGE_TIMEOUT; i++) {
		if (CSR_READ_4(sc, BGE_FTQ_RESET) == 0)
			break;
		DELAY(10);
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: flow-through queue init failed\n",
		    sc->bge_dev.dv_xname);
		return(ENXIO);
	}

	/* Initialize the standard RX ring control block */
	rcb = &sc->bge_rdata->bge_info.bge_std_rx_rcb;
	bge_set_hostaddr(&rcb->bge_hostaddr,
	    BGE_RING_DMA_ADDR(sc, bge_rx_std_ring));
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		rcb->bge_maxlen_flags =
		    BGE_RCB_MAXLEN_FLAGS(BGE_MAX_FRAMELEN, 0);
	} else {
		rcb->bge_maxlen_flags = BGE_RCB_MAXLEN_FLAGS(512, 0);
	}
	if (sc->bge_extram)
		rcb->bge_nicaddr = BGE_EXT_STD_RX_RINGS;
	else
		rcb->bge_nicaddr = BGE_STD_RX_RINGS;
	CSR_WRITE_4(sc, BGE_RX_STD_RCB_HADDR_HI, rcb->bge_hostaddr.bge_addr_hi);
	CSR_WRITE_4(sc, BGE_RX_STD_RCB_HADDR_LO, rcb->bge_hostaddr.bge_addr_lo);
	CSR_WRITE_4(sc, BGE_RX_STD_RCB_MAXLEN_FLAGS, rcb->bge_maxlen_flags);
	CSR_WRITE_4(sc, BGE_RX_STD_RCB_NICADDR, rcb->bge_nicaddr);

	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		sc->bge_return_ring_cnt = BGE_RETURN_RING_CNT;
	} else {
		sc->bge_return_ring_cnt = BGE_RETURN_RING_CNT_5705;
	}

	/*
	 * Initialize the jumbo RX ring control block
	 * We set the 'ring disabled' bit in the flags
	 * field until we're actually ready to start
	 * using this ring (i.e. once we set the MTU
	 * high enough to require it).
	 */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		rcb = &sc->bge_rdata->bge_info.bge_jumbo_rx_rcb;
		bge_set_hostaddr(&rcb->bge_hostaddr,
		    BGE_RING_DMA_ADDR(sc, bge_rx_jumbo_ring));
		rcb->bge_maxlen_flags =
		    BGE_RCB_MAXLEN_FLAGS(BGE_MAX_FRAMELEN,
			BGE_RCB_FLAG_RING_DISABLED);
		if (sc->bge_extram)
			rcb->bge_nicaddr = BGE_EXT_JUMBO_RX_RINGS;
		else
			rcb->bge_nicaddr = BGE_JUMBO_RX_RINGS;

		CSR_WRITE_4(sc, BGE_RX_JUMBO_RCB_HADDR_HI,
		    rcb->bge_hostaddr.bge_addr_hi);
		CSR_WRITE_4(sc, BGE_RX_JUMBO_RCB_HADDR_LO,
		    rcb->bge_hostaddr.bge_addr_lo);
		CSR_WRITE_4(sc, BGE_RX_JUMBO_RCB_MAXLEN_FLAGS,
		    rcb->bge_maxlen_flags);
		CSR_WRITE_4(sc, BGE_RX_JUMBO_RCB_NICADDR, rcb->bge_nicaddr);

		/* Set up dummy disabled mini ring RCB */
		rcb = &sc->bge_rdata->bge_info.bge_mini_rx_rcb;
		rcb->bge_maxlen_flags = BGE_RCB_MAXLEN_FLAGS(0,
		    BGE_RCB_FLAG_RING_DISABLED);
		CSR_WRITE_4(sc, BGE_RX_MINI_RCB_MAXLEN_FLAGS,
		    rcb->bge_maxlen_flags);

		bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
		    offsetof(struct bge_ring_data, bge_info),
		    sizeof (struct bge_gib),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}

	/*
	 * Set the BD ring replentish thresholds. The recommended
	 * values are 1/8th the number of descriptors allocated to
	 * each ring.
	 */
	CSR_WRITE_4(sc, BGE_RBDI_STD_REPL_THRESH, BGE_STD_RX_RING_CNT/8);
	CSR_WRITE_4(sc, BGE_RBDI_JUMBO_REPL_THRESH, BGE_JUMBO_RX_RING_CNT/8);

	/*
	 * Disable all unused send rings by setting the 'ring disabled'
	 * bit in the flags field of all the TX send ring control blocks.
	 * These are located in NIC memory.
	 */
	rcb_addr = BGE_MEMWIN_START + BGE_SEND_RING_RCB;
	for (i = 0; i < BGE_TX_RINGS_EXTSSRAM_MAX; i++) {
		RCB_WRITE_4(sc, rcb_addr, bge_maxlen_flags,
		    BGE_RCB_MAXLEN_FLAGS(0,BGE_RCB_FLAG_RING_DISABLED));
		RCB_WRITE_4(sc, rcb_addr, bge_nicaddr, 0);
		rcb_addr += sizeof(struct bge_rcb);
	}

	/* Configure TX RCB 0 (we use only the first ring) */
	rcb_addr = BGE_MEMWIN_START + BGE_SEND_RING_RCB;
	bge_set_hostaddr(&taddr, BGE_RING_DMA_ADDR(sc, bge_tx_ring));
	RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_hi, taddr.bge_addr_hi);
	RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_lo, taddr.bge_addr_lo);
	RCB_WRITE_4(sc, rcb_addr, bge_nicaddr,
		    BGE_NIC_TXRING_ADDR(0, BGE_TX_RING_CNT));
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		RCB_WRITE_4(sc, rcb_addr, bge_maxlen_flags,
		    BGE_RCB_MAXLEN_FLAGS(BGE_TX_RING_CNT, 0));
	}

	/* Disable all unused RX return rings */
	rcb_addr = BGE_MEMWIN_START + BGE_RX_RETURN_RING_RCB;
	for (i = 0; i < BGE_RX_RINGS_MAX; i++) {
		RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_hi, 0);
		RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_lo, 0);
		RCB_WRITE_4(sc, rcb_addr, bge_maxlen_flags,
			    BGE_RCB_MAXLEN_FLAGS(sc->bge_return_ring_cnt,
                                     BGE_RCB_FLAG_RING_DISABLED));
		RCB_WRITE_4(sc, rcb_addr, bge_nicaddr, 0);
		CSR_WRITE_4(sc, BGE_MBX_RX_CONS0_LO +
		    (i * (sizeof(u_int64_t))), 0);
		rcb_addr += sizeof(struct bge_rcb);
	}

	/* Initialize RX ring indexes */
	CSR_WRITE_4(sc, BGE_MBX_RX_STD_PROD_LO, 0);
	CSR_WRITE_4(sc, BGE_MBX_RX_JUMBO_PROD_LO, 0);
	CSR_WRITE_4(sc, BGE_MBX_RX_MINI_PROD_LO, 0);

	/*
	 * Set up RX return ring 0
	 * Note that the NIC address for RX return rings is 0x00000000.
	 * The return rings live entirely within the host, so the
	 * nicaddr field in the RCB isn't used.
	 */
	rcb_addr = BGE_MEMWIN_START + BGE_RX_RETURN_RING_RCB;
	bge_set_hostaddr(&taddr, BGE_RING_DMA_ADDR(sc, bge_rx_return_ring));
	RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_hi, taddr.bge_addr_hi);
	RCB_WRITE_4(sc, rcb_addr, bge_hostaddr.bge_addr_lo, taddr.bge_addr_lo);
	RCB_WRITE_4(sc, rcb_addr, bge_nicaddr, 0x00000000);
	RCB_WRITE_4(sc, rcb_addr, bge_maxlen_flags,
	    BGE_RCB_MAXLEN_FLAGS(sc->bge_return_ring_cnt, 0));

	/* Set random backoff seed for TX */
	CSR_WRITE_4(sc, BGE_TX_RANDOM_BACKOFF,
	    LLADDR(ifp->if_sadl)[0] + LLADDR(ifp->if_sadl)[1] +
	    LLADDR(ifp->if_sadl)[2] + LLADDR(ifp->if_sadl)[3] +
	    LLADDR(ifp->if_sadl)[4] + LLADDR(ifp->if_sadl)[5] +
	    BGE_TX_BACKOFF_SEED_MASK);

	/* Set inter-packet gap */
	CSR_WRITE_4(sc, BGE_TX_LENGTHS, 0x2620);

	/*
	 * Specify which ring to use for packets that don't match
	 * any RX rules.
	 */
	CSR_WRITE_4(sc, BGE_RX_RULES_CFG, 0x08);

	/*
	 * Configure number of RX lists. One interrupt distribution
	 * list, sixteen active lists, one bad frames class.
	 */
	CSR_WRITE_4(sc, BGE_RXLP_CFG, 0x181);

	/* Inialize RX list placement stats mask. */
	CSR_WRITE_4(sc, BGE_RXLP_STATS_ENABLE_MASK, 0x007FFFFF);
	CSR_WRITE_4(sc, BGE_RXLP_STATS_CTL, 0x1);

	/* Disable host coalescing until we get it set up */
	CSR_WRITE_4(sc, BGE_HCC_MODE, 0x00000000);

	/* Poll to make sure it's shut down. */
	for (i = 0; i < BGE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, BGE_HCC_MODE) & BGE_HCCMODE_ENABLE))
			break;
		DELAY(10);
	}

	if (i == BGE_TIMEOUT) {
		printf("%s: host coalescing engine failed to idle\n",
		    sc->bge_dev.dv_xname);
		return(ENXIO);
	}

	/* Set up host coalescing defaults */
	CSR_WRITE_4(sc, BGE_HCC_RX_COAL_TICKS, sc->bge_rx_coal_ticks);
	CSR_WRITE_4(sc, BGE_HCC_TX_COAL_TICKS, sc->bge_tx_coal_ticks);
	CSR_WRITE_4(sc, BGE_HCC_RX_MAX_COAL_BDS, sc->bge_rx_max_coal_bds);
	CSR_WRITE_4(sc, BGE_HCC_TX_MAX_COAL_BDS, sc->bge_tx_max_coal_bds);
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		CSR_WRITE_4(sc, BGE_HCC_RX_COAL_TICKS_INT, 0);
		CSR_WRITE_4(sc, BGE_HCC_TX_COAL_TICKS_INT, 0);
	}
	CSR_WRITE_4(sc, BGE_HCC_RX_MAX_COAL_BDS_INT, 0);
	CSR_WRITE_4(sc, BGE_HCC_TX_MAX_COAL_BDS_INT, 0);

	/* Set up address of statistics block */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		bge_set_hostaddr(&taddr,
		    BGE_RING_DMA_ADDR(sc, bge_info.bge_stats));
		CSR_WRITE_4(sc, BGE_HCC_STATS_TICKS, sc->bge_stat_ticks);
		CSR_WRITE_4(sc, BGE_HCC_STATS_BASEADDR, BGE_STATS_BLOCK);
		CSR_WRITE_4(sc, BGE_HCC_STATS_ADDR_HI, taddr.bge_addr_hi);
		CSR_WRITE_4(sc, BGE_HCC_STATS_ADDR_LO, taddr.bge_addr_lo);
	}

	/* Set up address of status block */
	bge_set_hostaddr(&taddr, BGE_RING_DMA_ADDR(sc, bge_status_block));
	CSR_WRITE_4(sc, BGE_HCC_STATUSBLK_BASEADDR, BGE_STATUS_BLOCK);
	CSR_WRITE_4(sc, BGE_HCC_STATUSBLK_ADDR_HI, taddr.bge_addr_hi);
	CSR_WRITE_4(sc, BGE_HCC_STATUSBLK_ADDR_LO, taddr.bge_addr_lo);
	sc->bge_rdata->bge_status_block.bge_idx[0].bge_rx_prod_idx = 0;
	sc->bge_rdata->bge_status_block.bge_idx[0].bge_tx_cons_idx = 0;

	/* Turn on host coalescing state machine */
	CSR_WRITE_4(sc, BGE_HCC_MODE, BGE_HCCMODE_ENABLE);

	/* Turn on RX BD completion state machine and enable attentions */
	CSR_WRITE_4(sc, BGE_RBDC_MODE,
	    BGE_RBDCMODE_ENABLE|BGE_RBDCMODE_ATTN);

	/* Turn on RX list placement state machine */
	CSR_WRITE_4(sc, BGE_RXLP_MODE, BGE_RXLPMODE_ENABLE);

	/* Turn on RX list selector state machine. */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		CSR_WRITE_4(sc, BGE_RXLS_MODE, BGE_RXLSMODE_ENABLE);
	}

	/* Turn on DMA, clear stats */
	CSR_WRITE_4(sc, BGE_MAC_MODE, BGE_MACMODE_TXDMA_ENB|
	    BGE_MACMODE_RXDMA_ENB|BGE_MACMODE_RX_STATS_CLEAR|
	    BGE_MACMODE_TX_STATS_CLEAR|BGE_MACMODE_RX_STATS_ENB|
	    BGE_MACMODE_TX_STATS_ENB|BGE_MACMODE_FRMHDR_DMA_ENB|
	    (sc->bge_tbi ? BGE_PORTMODE_TBI : BGE_PORTMODE_MII));

	/* Set misc. local control, enable interrupts on attentions */
	sc->bge_local_ctrl_reg = BGE_MLC_INTR_ONATTN | BGE_MLC_AUTO_EEPROM;

#ifdef notdef
	/* Assert GPIO pins for PHY reset */
	BGE_SETBIT(sc, BGE_MISC_LOCAL_CTL, BGE_MLC_MISCIO_OUT0|
	    BGE_MLC_MISCIO_OUT1|BGE_MLC_MISCIO_OUT2);
	BGE_SETBIT(sc, BGE_MISC_LOCAL_CTL, BGE_MLC_MISCIO_OUTEN0|
	    BGE_MLC_MISCIO_OUTEN1|BGE_MLC_MISCIO_OUTEN2);
#endif

#if defined(not_quite_yet)
	/* Linux driver enables enable gpio pin #1 on 5700s */
	if (sc->bge_chipid == BGE_CHIPID_BCM5700) {
		sc->bge_local_ctrl_reg |=
		  (BGE_MLC_MISCIO_OUT1|BGE_MLC_MISCIO_OUTEN1);
	}
#endif
	CSR_WRITE_4(sc, BGE_MISC_LOCAL_CTL, sc->bge_local_ctrl_reg);

	/* Turn on DMA completion state machine */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		CSR_WRITE_4(sc, BGE_DMAC_MODE, BGE_DMACMODE_ENABLE);
	}

	/* Turn on write DMA state machine */
	CSR_WRITE_4(sc, BGE_WDMA_MODE,
	    BGE_WDMAMODE_ENABLE|BGE_WDMAMODE_ALL_ATTNS);

	/* Turn on read DMA state machine */
	{
		uint32_t dma_read_modebits;

		dma_read_modebits =
		  BGE_RDMAMODE_ENABLE | BGE_RDMAMODE_ALL_ATTNS;

		if (sc->bge_pcie && 0) {
			dma_read_modebits |= BGE_RDMA_MODE_FIFO_LONG_BURST;
		} else if ((sc->bge_quirks & BGE_QUIRK_5705_CORE)) {
			dma_read_modebits |= BGE_RDMA_MODE_FIFO_SIZE_128;
		}

		/* XXX broadcom-supplied linux driver; undocumented */
		if (BGE_IS_5750_OR_BEYOND(sc)) {
 			/*
			 * XXX: magic values.
			 * From Broadcom-supplied Linux driver;  apparently
			 * required to workaround a DMA bug affecting TSO
			 * on bcm575x/bcm5721?
			 */
			dma_read_modebits |= (1 << 27);
		}
		CSR_WRITE_4(sc, BGE_RDMA_MODE, dma_read_modebits);
	}

	/* Turn on RX data completion state machine */
	CSR_WRITE_4(sc, BGE_RDC_MODE, BGE_RDCMODE_ENABLE);

	/* Turn on RX BD initiator state machine */
	CSR_WRITE_4(sc, BGE_RBDI_MODE, BGE_RBDIMODE_ENABLE);

	/* Turn on RX data and RX BD initiator state machine */
	CSR_WRITE_4(sc, BGE_RDBDI_MODE, BGE_RDBDIMODE_ENABLE);

	/* Turn on Mbuf cluster free state machine */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		CSR_WRITE_4(sc, BGE_MBCF_MODE, BGE_MBCFMODE_ENABLE);
	}

	/* Turn on send BD completion state machine */
	CSR_WRITE_4(sc, BGE_SBDC_MODE, BGE_SBDCMODE_ENABLE);

	/* Turn on send data completion state machine */
	CSR_WRITE_4(sc, BGE_SDC_MODE, BGE_SDCMODE_ENABLE);

	/* Turn on send data initiator state machine */
	if (BGE_IS_5750_OR_BEYOND(sc)) {
		/* XXX: magic value from Linux driver */
		CSR_WRITE_4(sc, BGE_SDI_MODE, BGE_SDIMODE_ENABLE | 0x08);
	} else {
		CSR_WRITE_4(sc, BGE_SDI_MODE, BGE_SDIMODE_ENABLE);
	}

	/* Turn on send BD initiator state machine */
	CSR_WRITE_4(sc, BGE_SBDI_MODE, BGE_SBDIMODE_ENABLE);

	/* Turn on send BD selector state machine */
	CSR_WRITE_4(sc, BGE_SRS_MODE, BGE_SRSMODE_ENABLE);

	CSR_WRITE_4(sc, BGE_SDI_STATS_ENABLE_MASK, 0x007FFFFF);
	CSR_WRITE_4(sc, BGE_SDI_STATS_CTL,
	    BGE_SDISTATSCTL_ENABLE|BGE_SDISTATSCTL_FASTER);

	/* ack/clear link change events */
	CSR_WRITE_4(sc, BGE_MAC_STS, BGE_MACSTAT_SYNC_CHANGED|
	    BGE_MACSTAT_CFG_CHANGED);
	CSR_WRITE_4(sc, BGE_MI_STS, 0);

	/* Enable PHY auto polling (for MII/GMII only) */
	if (sc->bge_tbi) {
		CSR_WRITE_4(sc, BGE_MI_STS, BGE_MISTS_LINK);
 	} else {
		BGE_SETBIT(sc, BGE_MI_MODE, BGE_MIMODE_AUTOPOLL|10<<16);
		if (sc->bge_quirks & BGE_QUIRK_LINK_STATE_BROKEN)
			CSR_WRITE_4(sc, BGE_MAC_EVT_ENB,
			    BGE_EVTENB_MI_INTERRUPT);
	}

	/* Enable link state change attentions. */
	BGE_SETBIT(sc, BGE_MAC_EVT_ENB, BGE_EVTENB_LINK_CHANGED);

	return(0);
}

static const struct bge_revision {
	uint32_t		br_chipid;
	uint32_t		br_quirks;
	const char		*br_name;
} bge_revisions[] = {
	{ BGE_CHIPID_BCM5700_A0,
	  BGE_QUIRK_LINK_STATE_BROKEN,
	  "BCM5700 A0" },

	{ BGE_CHIPID_BCM5700_A1,
	  BGE_QUIRK_LINK_STATE_BROKEN,
	  "BCM5700 A1" },

	{ BGE_CHIPID_BCM5700_B0,
	  BGE_QUIRK_LINK_STATE_BROKEN|BGE_QUIRK_CSUM_BROKEN|BGE_QUIRK_5700_COMMON,
	  "BCM5700 B0" },

	{ BGE_CHIPID_BCM5700_B1,
	  BGE_QUIRK_LINK_STATE_BROKEN|BGE_QUIRK_5700_COMMON,
	  "BCM5700 B1" },

	{ BGE_CHIPID_BCM5700_B2,
	  BGE_QUIRK_LINK_STATE_BROKEN|BGE_QUIRK_5700_COMMON,
	  "BCM5700 B2" },

	{ BGE_CHIPID_BCM5700_B3,
	  BGE_QUIRK_LINK_STATE_BROKEN|BGE_QUIRK_5700_COMMON,
	  "BCM5700 B3" },

	/* This is treated like a BCM5700 Bx */
	{ BGE_CHIPID_BCM5700_ALTIMA,
	  BGE_QUIRK_LINK_STATE_BROKEN|BGE_QUIRK_5700_COMMON,
	  "BCM5700 Altima" },

	{ BGE_CHIPID_BCM5700_C0,
	  0,
	  "BCM5700 C0" },

	{ BGE_CHIPID_BCM5701_A0,
	  0, /*XXX really, just not known */
	  "BCM5701 A0" },

	{ BGE_CHIPID_BCM5701_B0,
	  BGE_QUIRK_PCIX_DMA_ALIGN_BUG,
	  "BCM5701 B0" },

	{ BGE_CHIPID_BCM5701_B2,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_PCIX_DMA_ALIGN_BUG,
	  "BCM5701 B2" },

	{ BGE_CHIPID_BCM5701_B5,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_PCIX_DMA_ALIGN_BUG,
	  "BCM5701 B5" },

	{ BGE_CHIPID_BCM5703_A0,
	  0,
	  "BCM5703 A0" },

	{ BGE_CHIPID_BCM5703_A1,
	  0,
	  "BCM5703 A1" },

	{ BGE_CHIPID_BCM5703_A2,
	  BGE_QUIRK_ONLY_PHY_1,
	  "BCM5703 A2" },

	{ BGE_CHIPID_BCM5703_A3,
	  BGE_QUIRK_ONLY_PHY_1,
	  "BCM5703 A3" },

	{ BGE_CHIPID_BCM5703_B0,
	  BGE_QUIRK_ONLY_PHY_1,
	  "BCM5703 B0" },

	{ BGE_CHIPID_BCM5704_A0,
  	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_FEWER_MBUFS,
	  "BCM5704 A0" },

	{ BGE_CHIPID_BCM5704_A1,
  	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_FEWER_MBUFS,
	  "BCM5704 A1" },

	{ BGE_CHIPID_BCM5704_A2,
  	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_FEWER_MBUFS,
	  "BCM5704 A2" },

	{ BGE_CHIPID_BCM5704_A3,
  	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_FEWER_MBUFS,
	  "BCM5704 A3" },

	{ BGE_CHIPID_BCM5705_A0,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5705 A0" },

	{ BGE_CHIPID_BCM5705_A1,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5705 A1" },

	{ BGE_CHIPID_BCM5705_A2,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5705 A2" },

	{ BGE_CHIPID_BCM5705_A3,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5705 A3" },

	{ BGE_CHIPID_BCM5750_A0,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5750 A0" },

	{ BGE_CHIPID_BCM5750_A1,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5750 A1" },

	{ BGE_CHIPID_BCM5751_A1,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5751 A1" },

	{ BGE_CHIPID_BCM5752_A0,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5752 A0" },

	{ BGE_CHIPID_BCM5752_A1,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5752 A1" },

	{ BGE_CHIPID_BCM5752_A2,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "BCM5752 A2" },

	{ 0, 0, NULL }
};

/*
 * Some defaults for major revisions, so that newer steppings
 * that we don't know about have a shot at working.
 */
static const struct bge_revision bge_majorrevs[] = {
	{ BGE_ASICREV_BCM5700,
	  BGE_QUIRK_LINK_STATE_BROKEN,
	  "unknown BCM5700" },

	{ BGE_ASICREV_BCM5701,
	  BGE_QUIRK_PCIX_DMA_ALIGN_BUG,
	  "unknown BCM5701" },

	{ BGE_ASICREV_BCM5703,
	  0,
	  "unknown BCM5703" },

	{ BGE_ASICREV_BCM5704,
	  BGE_QUIRK_ONLY_PHY_1,
	  "unknown BCM5704" },

	{ BGE_ASICREV_BCM5705,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM5705" },

	{ BGE_ASICREV_BCM5750,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM575x family" },

	{ BGE_ASICREV_BCM5714_A0,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM5714" },

	{ BGE_ASICREV_BCM5714,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM5714" },

	{ BGE_ASICREV_BCM5752,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM5752 family" },


	{ BGE_ASICREV_BCM5780,
	  BGE_QUIRK_ONLY_PHY_1|BGE_QUIRK_5705_CORE,
	  "unknown BCM5780" },

	{ 0,
	  0,
	  NULL }
};


static const struct bge_revision *
bge_lookup_rev(uint32_t chipid)
{
	const struct bge_revision *br;

	for (br = bge_revisions; br->br_name != NULL; br++) {
		if (br->br_chipid == chipid)
			return (br);
	}

	for (br = bge_majorrevs; br->br_name != NULL; br++) {
		if (br->br_chipid == BGE_ASICREV(chipid))
			return (br);
	}

	return (NULL);
}

static const struct bge_product {
	pci_vendor_id_t		bp_vendor;
	pci_product_id_t	bp_product;
	const char		*bp_name;
} bge_products[] = {
	/*
	 * The BCM5700 documentation seems to indicate that the hardware
	 * still has the Alteon vendor ID burned into it, though it
	 * should always be overridden by the value in the EEPROM.  We'll
	 * check for it anyway.
	 */
	{ PCI_VENDOR_ALTEON,
	  PCI_PRODUCT_ALTEON_BCM5700,
	  "Broadcom BCM5700 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_ALTEON,
	  PCI_PRODUCT_ALTEON_BCM5701,
	  "Broadcom BCM5701 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_ALTIMA,
	  PCI_PRODUCT_ALTIMA_AC1000,
	  "Altima AC1000 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_ALTIMA,
	  PCI_PRODUCT_ALTIMA_AC1001,
	  "Altima AC1001 Gigabit Ethernet",
	   },
	{ PCI_VENDOR_ALTIMA,
	  PCI_PRODUCT_ALTIMA_AC9100,
	  "Altima AC9100 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5700,
	  "Broadcom BCM5700 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5701,
	  "Broadcom BCM5701 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5702,
	  "Broadcom BCM5702 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5702X,
	  "Broadcom BCM5702X Gigabit Ethernet" },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5703,
	  "Broadcom BCM5703 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5703X,
	  "Broadcom BCM5703X Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5703_ALT,
	  "Broadcom BCM5703 Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5704C,
	  "Broadcom BCM5704C Dual Gigabit Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5704S,
	  "Broadcom BCM5704S Dual Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5705,
	  "Broadcom BCM5705 Gigabit Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5705K,
	  "Broadcom BCM5705K Gigabit Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5705M,
	  "Broadcom BCM5705M Gigabit Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5705M_ALT,
	  "Broadcom BCM5705M Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5714,
	  "Broadcom BCM5714/5715 Gigabit Ethernet",
	  },
	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5789,
	  "Broadcom BCM5789 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5721,
	  "Broadcom BCM5721 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5750,
	  "Broadcom BCM5750 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5750M,
	  "Broadcom BCM5750M Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5751,
	  "Broadcom BCM5751 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5751M,
	  "Broadcom BCM5751M Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5752,
	  "Broadcom BCM5752 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5752M,
	  "Broadcom BCM5752M Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5780,
	  "Broadcom BCM5780 Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5780S,
	  "Broadcom BCM5780S Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5782,
	  "Broadcom BCM5782 Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5788,
	  "Broadcom BCM5788 Gigabit Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5789,
	  "Broadcom BCM5789 Gigabit Ethernet",
	  },

   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5901,
	  "Broadcom BCM5901 Fast Ethernet",
	  },
   	{ PCI_VENDOR_BROADCOM,
	  PCI_PRODUCT_BROADCOM_BCM5901A2,
	  "Broadcom BCM5901A2 Fast Ethernet",
	  },

	{ PCI_VENDOR_SCHNEIDERKOCH,
	  PCI_PRODUCT_SCHNEIDERKOCH_SK_9DX1,
	  "SysKonnect SK-9Dx1 Gigabit Ethernet",
	  },

	{ PCI_VENDOR_3COM,
	  PCI_PRODUCT_3COM_3C996,
	  "3Com 3c996 Gigabit Ethernet",
	  },

	{ 0,
	  0,
	  NULL },
};

static const struct bge_product *
bge_lookup(const struct pci_attach_args *pa)
{
	const struct bge_product *bp;

	for (bp = bge_products; bp->bp_name != NULL; bp++) {
		if (PCI_VENDOR(pa->pa_id) == bp->bp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == bp->bp_product)
			return (bp);
	}

	return (NULL);
}

static int
bge_setpowerstate(struct bge_softc *sc, int powerlevel)
{
#ifdef NOTYET
	u_int32_t pm_ctl = 0;

	/* XXX FIXME: make sure indirect accesses enabled? */
	pm_ctl = pci_conf_read(sc->bge_dev, BGE_PCI_MISC_CTL, 4);
	pm_ctl |= BGE_PCIMISCCTL_INDIRECT_ACCESS;
	pci_write_config(sc->bge_dev, BGE_PCI_MISC_CTL, pm_ctl, 4);

	/* clear the PME_assert bit and power state bits, enable PME */
	pm_ctl = pci_conf_read(sc->bge_dev, BGE_PCI_PWRMGMT_CMD, 2);
	pm_ctl &= ~PCIM_PSTAT_DMASK;
	pm_ctl |= (1 << 8);

	if (powerlevel == 0) {
		pm_ctl |= PCIM_PSTAT_D0;
		pci_write_config(sc->bge_dev, BGE_PCI_PWRMGMT_CMD,
		    pm_ctl, 2);
		DELAY(10000);
		CSR_WRITE_4(sc, BGE_MISC_LOCAL_CTL, sc->bge_local_ctrl_reg);
		DELAY(10000);

#ifdef NOTYET
		/* XXX FIXME: write 0x02 to phy aux_Ctrl reg */
		bge_miibus_writereg(sc->bge_dev, 1, 0x18, 0x02);
#endif
		DELAY(40); DELAY(40); DELAY(40);
		DELAY(10000);	/* above not quite adequate on 5700 */
		return 0;
	}


	/*
	 * Entering ACPI power states D1-D3 is achieved by wiggling
	 * GMII gpio pins. Example code assumes all hardware vendors
	 * followed Broadom's sample pcb layout. Until we verify that
	 * for all supported OEM cards, states D1-D3 are  unsupported.
	 */
	printf("%s: power state %d unimplemented; check GPIO pins\n",
	       sc->bge_dev.dv_xname, powerlevel);
#endif
	return EOPNOTSUPP;
}


/*
 * Probe for a Broadcom chip. Check the PCI vendor and device IDs
 * against our list and return its name if we find a match. Note
 * that since the Broadcom controller contains VPD support, we
 * can get the device name string from the controller itself instead
 * of the compiled-in string. This is a little slow, but it guarantees
 * we'll always announce the right product name.
 */
static int
bge_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (bge_lookup(pa) != NULL)
		return (1);

	return (0);
}

static void
bge_attach(device_t parent, device_t self, void *aux)
{
	struct bge_softc	*sc = (struct bge_softc *)self;
	struct pci_attach_args	*pa = aux;
	const struct bge_product *bp;
	const struct bge_revision *br;
	pci_chipset_tag_t	pc = pa->pa_pc;
	pci_intr_handle_t	ih;
	const char		*intrstr = NULL;
	bus_dma_segment_t	seg;
	int			rseg;
	u_int32_t		hwcfg = 0;
	u_int32_t		mac_addr = 0;
	u_int32_t		command;
	struct ifnet		*ifp;
	caddr_t			kva;
	u_char			eaddr[ETHER_ADDR_LEN];
	pcireg_t		memtype;
	bus_addr_t		memaddr;
	bus_size_t		memsize;
	u_int32_t		pm_ctl;

	bp = bge_lookup(pa);
	KASSERT(bp != NULL);

	sc->bge_pa = *pa;

	aprint_naive(": Ethernet controller\n");
	aprint_normal(": %s\n", bp->bp_name);

	/*
	 * Map control/status registers.
	 */
	DPRINTFN(5, ("Map control/status regs\n"));
	command = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);
	command = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	if (!(command & PCI_COMMAND_MEM_ENABLE)) {
		aprint_error("%s: failed to enable memory mapping!\n",
		    sc->bge_dev.dv_xname);
		return;
	}

	DPRINTFN(5, ("pci_mem_find\n"));
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, BGE_PCI_BAR0);
 	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		if (pci_mapreg_map(pa, BGE_PCI_BAR0,
		    memtype, 0, &sc->bge_btag, &sc->bge_bhandle,
		    &memaddr, &memsize) == 0)
			break;
	default:
		aprint_error("%s: can't find mem space\n",
		    sc->bge_dev.dv_xname);
		return;
	}

	DPRINTFN(5, ("pci_intr_map\n"));
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: couldn't map interrupt\n",
		    sc->bge_dev.dv_xname);
		return;
	}

	DPRINTFN(5, ("pci_intr_string\n"));
	intrstr = pci_intr_string(pc, ih);

	DPRINTFN(5, ("pci_intr_establish\n"));
	sc->bge_intrhand = pci_intr_establish(pc, ih, IPL_NET, bge_intr, sc);

	if (sc->bge_intrhand == NULL) {
		aprint_error("%s: couldn't establish interrupt",
		    sc->bge_dev.dv_xname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n",
	    sc->bge_dev.dv_xname, intrstr);

	/*
	 * Kludge for 5700 Bx bug: a hardware bug (PCIX byte enable?)
	 * can clobber the chip's PCI config-space power control registers,
	 * leaving the card in D3 powersave state.
	 * We do not have memory-mapped registers in this state,
	 * so force device into D0 state before starting initialization.
	 */
	pm_ctl = pci_conf_read(pc, pa->pa_tag, BGE_PCI_PWRMGMT_CMD);
	pm_ctl &= ~(PCI_PWR_D0|PCI_PWR_D1|PCI_PWR_D2|PCI_PWR_D3);
	pm_ctl |= (1 << 8) | PCI_PWR_D0 ; /* D0 state */
	pci_conf_write(pc, pa->pa_tag, BGE_PCI_PWRMGMT_CMD, pm_ctl);
	DELAY(1000);	/* 27 usec is allegedly sufficent */

	/*
	 * Save ASIC rev.  Look up any quirks associated with this
	 * ASIC.
	 */
	sc->bge_chipid =
	    pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_MISC_CTL) &
	    BGE_PCIMISCCTL_ASICREV;

	/*
	 * Detect PCI-Express devices
	 * XXX: guessed from Linux/FreeBSD; no documentation
	 */
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_PCIEXPRESS,
	        NULL, NULL) != 0)
		sc->bge_pcie = 1;
	else
		sc->bge_pcie = 0;

	/* Try to reset the chip. */
	DPRINTFN(5, ("bge_reset\n"));
	bge_reset(sc);

	if (bge_chipinit(sc)) {
		aprint_error("%s: chip initialization failed\n",
		    sc->bge_dev.dv_xname);
		bge_release_resources(sc);
		return;
	}

	/*
	 * Get station address from the EEPROM.
	 */
	mac_addr = bge_readmem_ind(sc, 0x0c14);
	if ((mac_addr >> 16) == 0x484b) {
		eaddr[0] = (u_char)(mac_addr >> 8);
		eaddr[1] = (u_char)(mac_addr >> 0);
		mac_addr = bge_readmem_ind(sc, 0x0c18);
		eaddr[2] = (u_char)(mac_addr >> 24);
		eaddr[3] = (u_char)(mac_addr >> 16);
		eaddr[4] = (u_char)(mac_addr >> 8);
		eaddr[5] = (u_char)(mac_addr >> 0);
	} else if (bge_read_eeprom(sc, (caddr_t)eaddr,
	    BGE_EE_MAC_OFFSET + 2, ETHER_ADDR_LEN)) {
		aprint_error("%s: failed to read station address\n",
		    sc->bge_dev.dv_xname);
		bge_release_resources(sc);
		return;
	}

	br = bge_lookup_rev(sc->bge_chipid);
	aprint_normal("%s: ", sc->bge_dev.dv_xname);

	if (br == NULL) {
		aprint_normal("unknown ASIC (0x%04x)", sc->bge_chipid >> 16);
		sc->bge_quirks = 0;
	} else {
		aprint_normal("ASIC %s (0x%04x)",
		    br->br_name, sc->bge_chipid >> 16);
		sc->bge_quirks |= br->br_quirks;
	}
	aprint_normal(", Ethernet address %s\n", ether_sprintf(eaddr));

	/* Allocate the general information block and ring buffers. */
	if (pci_dma64_available(pa))
		sc->bge_dmatag = pa->pa_dmat64;
	else
		sc->bge_dmatag = pa->pa_dmat;
	DPRINTFN(5, ("bus_dmamem_alloc\n"));
	if (bus_dmamem_alloc(sc->bge_dmatag, sizeof(struct bge_ring_data),
			     PAGE_SIZE, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		aprint_error("%s: can't alloc rx buffers\n",
		    sc->bge_dev.dv_xname);
		return;
	}
	DPRINTFN(5, ("bus_dmamem_map\n"));
	if (bus_dmamem_map(sc->bge_dmatag, &seg, rseg,
			   sizeof(struct bge_ring_data), &kva,
			   BUS_DMA_NOWAIT)) {
		aprint_error("%s: can't map DMA buffers (%d bytes)\n",
		    sc->bge_dev.dv_xname, (int)sizeof(struct bge_ring_data));
		bus_dmamem_free(sc->bge_dmatag, &seg, rseg);
		return;
	}
	DPRINTFN(5, ("bus_dmamem_create\n"));
	if (bus_dmamap_create(sc->bge_dmatag, sizeof(struct bge_ring_data), 1,
	    sizeof(struct bge_ring_data), 0,
	    BUS_DMA_NOWAIT, &sc->bge_ring_map)) {
		aprint_error("%s: can't create DMA map\n",
		    sc->bge_dev.dv_xname);
		bus_dmamem_unmap(sc->bge_dmatag, kva,
				 sizeof(struct bge_ring_data));
		bus_dmamem_free(sc->bge_dmatag, &seg, rseg);
		return;
	}
	DPRINTFN(5, ("bus_dmamem_load\n"));
	if (bus_dmamap_load(sc->bge_dmatag, sc->bge_ring_map, kva,
			    sizeof(struct bge_ring_data), NULL,
			    BUS_DMA_NOWAIT)) {
		bus_dmamap_destroy(sc->bge_dmatag, sc->bge_ring_map);
		bus_dmamem_unmap(sc->bge_dmatag, kva,
				 sizeof(struct bge_ring_data));
		bus_dmamem_free(sc->bge_dmatag, &seg, rseg);
		return;
	}

	DPRINTFN(5, ("bzero\n"));
	sc->bge_rdata = (struct bge_ring_data *)kva;

	memset(sc->bge_rdata, 0, sizeof(struct bge_ring_data));

	/* Try to allocate memory for jumbo buffers. */
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		if (bge_alloc_jumbo_mem(sc)) {
			aprint_error("%s: jumbo buffer allocation failed\n",
			    sc->bge_dev.dv_xname);
		} else
			sc->ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;
	}

	/* Set default tuneable values. */
	sc->bge_stat_ticks = BGE_TICKS_PER_SEC;
	sc->bge_rx_coal_ticks = 150;
	sc->bge_rx_max_coal_bds = 64;
#ifdef ORIG_WPAUL_VALUES
	sc->bge_tx_coal_ticks = 150;
	sc->bge_tx_max_coal_bds = 128;
#else
	sc->bge_tx_coal_ticks = 300;
	sc->bge_tx_max_coal_bds = 400;
#endif
	if (sc->bge_quirks & BGE_QUIRK_5705_CORE) {
		sc->bge_tx_coal_ticks = (12 * 5);
		sc->bge_rx_max_coal_bds = (12 * 5);
			aprint_verbose("%s: setting short Tx thresholds\n",
			    sc->bge_dev.dv_xname);
	}

	/* Set up ifnet structure */
	ifp = &sc->ethercom.ec_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = bge_ioctl;
	ifp->if_start = bge_start;
	ifp->if_init = bge_init;
	ifp->if_watchdog = bge_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, max(BGE_TX_RING_CNT - 1, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);
	DPRINTFN(5, ("strcpy if_xname\n"));
	strcpy(ifp->if_xname, sc->bge_dev.dv_xname);

	if ((sc->bge_quirks & BGE_QUIRK_CSUM_BROKEN) == 0)
		sc->ethercom.ec_if.if_capabilities |=
		    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
		    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;
	sc->ethercom.ec_capabilities |=
	    ETHERCAP_VLAN_HWTAGGING | ETHERCAP_VLAN_MTU;

	if (sc->bge_pcie)
		sc->ethercom.ec_if.if_capabilities |= IFCAP_TSOv4;

	/*
	 * Do MII setup.
	 */
	DPRINTFN(5, ("mii setup\n"));
	sc->bge_mii.mii_ifp = ifp;
	sc->bge_mii.mii_readreg = bge_miibus_readreg;
	sc->bge_mii.mii_writereg = bge_miibus_writereg;
	sc->bge_mii.mii_statchg = bge_miibus_statchg;

	/*
	 * Figure out what sort of media we have by checking the
	 * hardware config word in the first 32k of NIC internal memory,
	 * or fall back to the config word in the EEPROM. Note: on some BCM5700
	 * cards, this value appears to be unset. If that's the
	 * case, we have to rely on identifying the NIC by its PCI
	 * subsystem ID, as we do below for the SysKonnect SK-9D41.
	 */
	if (bge_readmem_ind(sc, BGE_SOFTWARE_GENCOMM_SIG) == BGE_MAGIC_NUMBER) {
		hwcfg = bge_readmem_ind(sc, BGE_SOFTWARE_GENCOMM_NICCFG);
	} else {
		bge_read_eeprom(sc, (caddr_t)&hwcfg,
		    BGE_EE_HWCFG_OFFSET, sizeof(hwcfg));
		hwcfg = be32toh(hwcfg);
	}
	if ((hwcfg & BGE_HWCFG_MEDIA) == BGE_MEDIA_FIBER)
		sc->bge_tbi = 1;

	/* The SysKonnect SK-9D41 is a 1000baseSX card. */
	if ((pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_SUBSYS) >> 16) ==
	    SK_SUBSYSID_9D41)
		sc->bge_tbi = 1;

	if (sc->bge_tbi) {
		ifmedia_init(&sc->bge_ifmedia, IFM_IMASK, bge_ifmedia_upd,
		    bge_ifmedia_sts);
		ifmedia_add(&sc->bge_ifmedia, IFM_ETHER|IFM_1000_SX, 0, NULL);
		ifmedia_add(&sc->bge_ifmedia, IFM_ETHER|IFM_1000_SX|IFM_FDX,
			    0, NULL);
		ifmedia_add(&sc->bge_ifmedia, IFM_ETHER|IFM_AUTO, 0, NULL);
		ifmedia_set(&sc->bge_ifmedia, IFM_ETHER|IFM_AUTO);
	} else {
		/*
		 * Do transceiver setup.
		 */
		ifmedia_init(&sc->bge_mii.mii_media, 0, bge_ifmedia_upd,
			     bge_ifmedia_sts);
		mii_attach(&sc->bge_dev, &sc->bge_mii, 0xffffffff,
			   MII_PHY_ANY, MII_OFFSET_ANY,
			   MIIF_FORCEANEG|MIIF_DOPAUSE);

		if (LIST_FIRST(&sc->bge_mii.mii_phys) == NULL) {
			printf("%s: no PHY found!\n", sc->bge_dev.dv_xname);
			ifmedia_add(&sc->bge_mii.mii_media,
				    IFM_ETHER|IFM_MANUAL, 0, NULL);
			ifmedia_set(&sc->bge_mii.mii_media,
				    IFM_ETHER|IFM_MANUAL);
		} else
			ifmedia_set(&sc->bge_mii.mii_media,
				    IFM_ETHER|IFM_AUTO);
	}

	/*
	 * When using the BCM5701 in PCI-X mode, data corruption has
	 * been observed in the first few bytes of some received packets.
	 * Aligning the packet buffer in memory eliminates the corruption.
	 * Unfortunately, this misaligns the packet payloads.  On platforms
	 * which do not support unaligned accesses, we will realign the
	 * payloads by copying the received packets.
	 */
	if (sc->bge_quirks & BGE_QUIRK_PCIX_DMA_ALIGN_BUG) {
		/* If in PCI-X mode, work around the alignment bug. */
		if ((pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_PCISTATE) &
                    (BGE_PCISTATE_PCI_BUSMODE | BGE_PCISTATE_PCI_BUSSPEED)) ==
                         BGE_PCISTATE_PCI_BUSSPEED)
		sc->bge_rx_alignment_bug = 1;
        }

	/*
	 * Call MI attach routine.
	 */
	DPRINTFN(5, ("if_attach\n"));
	if_attach(ifp);
	DPRINTFN(5, ("ether_ifattach\n"));
	ether_ifattach(ifp, eaddr);
#ifdef BGE_EVENT_COUNTERS
	/*
	 * Attach event counters.
	 */
	evcnt_attach_dynamic(&sc->bge_ev_intr, EVCNT_TYPE_INTR,
	    NULL, sc->bge_dev.dv_xname, "intr");
	evcnt_attach_dynamic(&sc->bge_ev_tx_xoff, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "tx_xoff");
	evcnt_attach_dynamic(&sc->bge_ev_tx_xon, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "tx_xon");
	evcnt_attach_dynamic(&sc->bge_ev_rx_xoff, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "rx_xoff");
	evcnt_attach_dynamic(&sc->bge_ev_rx_xon, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "rx_xon");
	evcnt_attach_dynamic(&sc->bge_ev_rx_macctl, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "rx_macctl");
	evcnt_attach_dynamic(&sc->bge_ev_xoffentered, EVCNT_TYPE_MISC,
	    NULL, sc->bge_dev.dv_xname, "xoffentered");
#endif /* BGE_EVENT_COUNTERS */
	DPRINTFN(5, ("callout_init\n"));
	callout_init(&sc->bge_timeout);

	sc->bge_powerhook = powerhook_establish(sc->bge_dev.dv_xname,
	    bge_powerhook, sc);
	if (sc->bge_powerhook == NULL)
		printf("%s: WARNING: unable to establish PCI power hook\n",
		    sc->bge_dev.dv_xname);
}

static void
bge_release_resources(struct bge_softc *sc)
{
	if (sc->bge_vpd_prodname != NULL)
		free(sc->bge_vpd_prodname, M_DEVBUF);

	if (sc->bge_vpd_readonly != NULL)
		free(sc->bge_vpd_readonly, M_DEVBUF);
}

static void
bge_reset(struct bge_softc *sc)
{
	struct pci_attach_args *pa = &sc->bge_pa;
	u_int32_t cachesize, command, pcistate, new_pcistate;
	int i, val;

	/* Save some important PCI state. */
	cachesize = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_CACHESZ);
	command = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_CMD);
	pcistate = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_PCISTATE);

	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MISC_CTL,
	    BGE_PCIMISCCTL_INDIRECT_ACCESS|BGE_PCIMISCCTL_MASK_PCI_INTR|
	    BGE_HIF_SWAP_OPTIONS|BGE_PCIMISCCTL_PCISTATE_RW);

	/*
	 * Disable the firmware fastboot feature on 5752 ASIC
	 * to avoid firmware timeout.
	 */
	if (BGE_ASICREV(sc->bge_chipid) == BGE_ASICREV_BCM5752)
		CSR_WRITE_4(sc, BGE_FASTBOOT_PC, 0);

	val = BGE_MISCCFG_RESET_CORE_CLOCKS | (65<<1);
	/*
	 * XXX: from FreeBSD/Linux; no documentation
	 */
	if (sc->bge_pcie) {
		if (CSR_READ_4(sc, BGE_PCIE_CTL1) == 0x60)
			CSR_WRITE_4(sc, BGE_PCIE_CTL1, 0x20);
		if (sc->bge_chipid != BGE_CHIPID_BCM5750_A0) {
			/* No idea what that actually means */
			CSR_WRITE_4(sc, BGE_MISC_CFG, 1 << 29);
			val |= (1<<29);
		}
	}

	/* Issue global reset */
	bge_writereg_ind(sc, BGE_MISC_CFG, val);

	DELAY(1000);

	/*
	 * XXX: from FreeBSD/Linux; no documentation
	 */
	if (sc->bge_pcie) {
		if (sc->bge_chipid == BGE_CHIPID_BCM5750_A0) {
			pcireg_t reg;

			DELAY(500000);
			/* XXX: Magic Numbers */
			reg = pci_conf_read(pa->pa_pc, pa->pa_tag, BGE_PCI_UNKNOWN0);
			pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_UNKNOWN0,
			    reg | (1 << 15));
		}
		/*
		 * XXX: Magic Numbers.
		 * Sets maximal PCI-e payload and clears any PCI-e errors.
		 * Should be replaced with references to PCI config-space
		 * capability block for PCI-Express.
		 */
		pci_conf_write(pa->pa_pc, pa->pa_tag,
		    BGE_PCI_CONF_DEV_CTRL, 0xf5000);

	}

	/* Reset some of the PCI state that got zapped by reset */
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_MISC_CTL,
	    BGE_PCIMISCCTL_INDIRECT_ACCESS|BGE_PCIMISCCTL_MASK_PCI_INTR|
	    BGE_HIF_SWAP_OPTIONS|BGE_PCIMISCCTL_PCISTATE_RW);
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_CMD, command);
	pci_conf_write(pa->pa_pc, pa->pa_tag, BGE_PCI_CACHESZ, cachesize);
	bge_writereg_ind(sc, BGE_MISC_CFG, (65 << 1));

	/* Enable memory arbiter. */
	{
		uint32_t marbmode = 0;
		if (BGE_IS_5714_FAMILY(sc)) {
			marbmode = CSR_READ_4(sc, BGE_MARB_MODE);
		}
 		CSR_WRITE_4(sc, BGE_MARB_MODE, BGE_MARBMODE_ENABLE | marbmode);
	}

	/*
	 * Prevent PXE restart: write a magic number to the
	 * general communications memory at 0xB50.
	 */
	bge_writemem_ind(sc, BGE_SOFTWARE_GENCOMM, BGE_MAGIC_NUMBER);

	/*
	 * Poll the value location we just wrote until
	 * we see the 1's complement of the magic number.
	 * This indicates that the firmware initialization
	 * is complete.
	 */
	for (i = 0; i < BGE_TIMEOUT; i++) {
		val = bge_readmem_ind(sc, BGE_SOFTWARE_GENCOMM);
		if (val == ~BGE_MAGIC_NUMBER)
			break;
		DELAY(1000);
	}

	if (i >= BGE_TIMEOUT) {
		printf("%s: firmware handshake timed out, val = %x\n",
		    sc->bge_dev.dv_xname, val);
		/*
		 * XXX: occasionally fired on bcm5721, but without
		 * apparent harm.  For now, keep going if we timeout
		 * against PCI-E devices.
		 */
		 if (!sc->bge_pcie)
		  return;
	}

	/*
	 * XXX Wait for the value of the PCISTATE register to
	 * return to its original pre-reset state. This is a
	 * fairly good indicator of reset completion. If we don't
	 * wait for the reset to fully complete, trying to read
	 * from the device's non-PCI registers may yield garbage
	 * results.
	 */
	for (i = 0; i < BGE_TIMEOUT; i++) {
		new_pcistate = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    BGE_PCI_PCISTATE);
		if ((new_pcistate & ~BGE_PCISTATE_RESERVED) ==
		    (pcistate & ~BGE_PCISTATE_RESERVED))
			break;
		DELAY(10);
	}
	if ((new_pcistate & ~BGE_PCISTATE_RESERVED) !=
	    (pcistate & ~BGE_PCISTATE_RESERVED)) {
		printf("%s: pcistate failed to revert\n",
		    sc->bge_dev.dv_xname);
	}

	/* XXX: from FreeBSD/Linux; no documentation */
	if (sc->bge_pcie && sc->bge_chipid != BGE_CHIPID_BCM5750_A0)
		CSR_WRITE_4(sc, BGE_PCIE_CTL0, CSR_READ_4(sc, BGE_PCIE_CTL0) | (1<<25));

	/* Enable memory arbiter. */
	/* XXX why do this twice? */
	{
		uint32_t marbmode = 0;
		if (BGE_IS_5714_FAMILY(sc)) {
			marbmode = CSR_READ_4(sc, BGE_MARB_MODE);
		}
 		CSR_WRITE_4(sc, BGE_MARB_MODE, BGE_MARBMODE_ENABLE | marbmode);
	}

	/* Fix up byte swapping */
	CSR_WRITE_4(sc, BGE_MODE_CTL, BGE_DMA_SWAP_OPTIONS);

	CSR_WRITE_4(sc, BGE_MAC_MODE, 0);

	DELAY(10000);
}

/*
 * Frame reception handling. This is called if there's a frame
 * on the receive return list.
 *
 * Note: we have to be able to handle two possibilities here:
 * 1) the frame is from the jumbo recieve ring
 * 2) the frame is from the standard receive ring
 */

static void
bge_rxeof(struct bge_softc *sc)
{
	struct ifnet *ifp;
	int stdcnt = 0, jumbocnt = 0;
	bus_dmamap_t dmamap;
	bus_addr_t offset, toff;
	bus_size_t tlen;
	int tosync;

	ifp = &sc->ethercom.ec_if;

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offsetof(struct bge_ring_data, bge_status_block),
	    sizeof (struct bge_status_block),
	    BUS_DMASYNC_POSTREAD);

	offset = offsetof(struct bge_ring_data, bge_rx_return_ring);
	tosync = sc->bge_rdata->bge_status_block.bge_idx[0].bge_rx_prod_idx -
	    sc->bge_rx_saved_considx;

	toff = offset + (sc->bge_rx_saved_considx * sizeof (struct bge_rx_bd));

	if (tosync < 0) {
		tlen = (sc->bge_return_ring_cnt - sc->bge_rx_saved_considx) *
		    sizeof (struct bge_rx_bd);
		bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
		    toff, tlen, BUS_DMASYNC_POSTREAD);
		tosync = -tosync;
	}

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offset, tosync * sizeof (struct bge_rx_bd),
	    BUS_DMASYNC_POSTREAD);

	while(sc->bge_rx_saved_considx !=
	    sc->bge_rdata->bge_status_block.bge_idx[0].bge_rx_prod_idx) {
		struct bge_rx_bd	*cur_rx;
		u_int32_t		rxidx;
		struct mbuf		*m = NULL;

		cur_rx = &sc->bge_rdata->
			bge_rx_return_ring[sc->bge_rx_saved_considx];

		rxidx = cur_rx->bge_idx;
		BGE_INC(sc->bge_rx_saved_considx, sc->bge_return_ring_cnt);

		if (cur_rx->bge_flags & BGE_RXBDFLAG_JUMBO_RING) {
			BGE_INC(sc->bge_jumbo, BGE_JUMBO_RX_RING_CNT);
			m = sc->bge_cdata.bge_rx_jumbo_chain[rxidx];
			sc->bge_cdata.bge_rx_jumbo_chain[rxidx] = NULL;
			jumbocnt++;
			bus_dmamap_sync(sc->bge_dmatag,
			    sc->bge_cdata.bge_rx_jumbo_map,
			    mtod(m, caddr_t) - sc->bge_cdata.bge_jumbo_buf,
			    BGE_JLEN, BUS_DMASYNC_POSTREAD);
			if (cur_rx->bge_flags & BGE_RXBDFLAG_ERROR) {
				ifp->if_ierrors++;
				bge_newbuf_jumbo(sc, sc->bge_jumbo, m);
				continue;
			}
			if (bge_newbuf_jumbo(sc, sc->bge_jumbo,
					     NULL)== ENOBUFS) {
				ifp->if_ierrors++;
				bge_newbuf_jumbo(sc, sc->bge_jumbo, m);
				continue;
			}
		} else {
			BGE_INC(sc->bge_std, BGE_STD_RX_RING_CNT);
			m = sc->bge_cdata.bge_rx_std_chain[rxidx];

			sc->bge_cdata.bge_rx_std_chain[rxidx] = NULL;
			stdcnt++;
			dmamap = sc->bge_cdata.bge_rx_std_map[rxidx];
			sc->bge_cdata.bge_rx_std_map[rxidx] = 0;
			bus_dmamap_sync(sc->bge_dmatag, dmamap, 0,
			    dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(sc->bge_dmatag, dmamap);
			if (cur_rx->bge_flags & BGE_RXBDFLAG_ERROR) {
				ifp->if_ierrors++;
				bge_newbuf_std(sc, sc->bge_std, m, dmamap);
				continue;
			}
			if (bge_newbuf_std(sc, sc->bge_std,
			    NULL, dmamap) == ENOBUFS) {
				ifp->if_ierrors++;
				bge_newbuf_std(sc, sc->bge_std, m, dmamap);
				continue;
			}
		}

		ifp->if_ipackets++;
#ifndef __NO_STRICT_ALIGNMENT
                /*
                 * XXX: if the 5701 PCIX-Rx-DMA workaround is in effect,
                 * the Rx buffer has the layer-2 header unaligned.
                 * If our CPU requires alignment, re-align by copying.
                 */
		if (sc->bge_rx_alignment_bug) {
			memmove(mtod(m, caddr_t) + ETHER_ALIGN, m->m_data,
                                cur_rx->bge_len);
			m->m_data += ETHER_ALIGN;
		}
#endif

		m->m_pkthdr.len = m->m_len = cur_rx->bge_len - ETHER_CRC_LEN;
		m->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
		/*
		 * Handle BPF listeners. Let the BPF user see the packet.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		m->m_pkthdr.csum_flags = M_CSUM_IPv4;

		if ((cur_rx->bge_ip_csum ^ 0xffff) != 0)
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
		/*
		 * Rx transport checksum-offload may also
		 * have bugs with packets which, when transmitted,
		 * were `runts' requiring padding.
		 */
		if (cur_rx->bge_flags & BGE_RXBDFLAG_TCP_UDP_CSUM &&
		    (/* (sc->_bge_quirks & BGE_QUIRK_SHORT_CKSUM_BUG) == 0 ||*/
		     m->m_pkthdr.len >= ETHER_MIN_NOPAD)) {
			m->m_pkthdr.csum_data =
			    cur_rx->bge_tcp_udp_csum;
			m->m_pkthdr.csum_flags |=
			    (M_CSUM_TCPv4|M_CSUM_UDPv4|
			     M_CSUM_DATA|M_CSUM_NO_PSEUDOHDR);
		}

		/*
		 * If we received a packet with a vlan tag, pass it
		 * to vlan_input() instead of ether_input().
		 */
		if (cur_rx->bge_flags & BGE_RXBDFLAG_VLAN_TAG)
			VLAN_INPUT_TAG(ifp, m, cur_rx->bge_vlan_tag, continue);

		(*ifp->if_input)(ifp, m);
	}

	CSR_WRITE_4(sc, BGE_MBX_RX_CONS0_LO, sc->bge_rx_saved_considx);
	if (stdcnt)
		CSR_WRITE_4(sc, BGE_MBX_RX_STD_PROD_LO, sc->bge_std);
	if (jumbocnt)
		CSR_WRITE_4(sc, BGE_MBX_RX_JUMBO_PROD_LO, sc->bge_jumbo);
}

static void
bge_txeof(struct bge_softc *sc)
{
	struct bge_tx_bd *cur_tx = NULL;
	struct ifnet *ifp;
	struct txdmamap_pool_entry *dma;
	bus_addr_t offset, toff;
	bus_size_t tlen;
	int tosync;
	struct mbuf *m;

	ifp = &sc->ethercom.ec_if;

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offsetof(struct bge_ring_data, bge_status_block),
	    sizeof (struct bge_status_block),
	    BUS_DMASYNC_POSTREAD);

	offset = offsetof(struct bge_ring_data, bge_tx_ring);
	tosync = sc->bge_rdata->bge_status_block.bge_idx[0].bge_tx_cons_idx -
	    sc->bge_tx_saved_considx;

	toff = offset + (sc->bge_tx_saved_considx * sizeof (struct bge_tx_bd));

	if (tosync < 0) {
		tlen = (BGE_TX_RING_CNT - sc->bge_tx_saved_considx) *
		    sizeof (struct bge_tx_bd);
		bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
		    toff, tlen, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		tosync = -tosync;
	}

	bus_dmamap_sync(sc->bge_dmatag, sc->bge_ring_map,
	    offset, tosync * sizeof (struct bge_tx_bd),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	/*
	 * Go through our tx ring and free mbufs for those
	 * frames that have been sent.
	 */
	while (sc->bge_tx_saved_considx !=
	    sc->bge_rdata->bge_status_block.bge_idx[0].bge_tx_cons_idx) {
		u_int32_t		idx = 0;

		idx = sc->bge_tx_saved_considx;
		cur_tx = &sc->bge_rdata->bge_tx_ring[idx];
		if (cur_tx->bge_flags & BGE_TXBDFLAG_END)
			ifp->if_opackets++;
		m = sc->bge_cdata.bge_tx_chain[idx];
		if (m != NULL) {
			sc->bge_cdata.bge_tx_chain[idx] = NULL;
			dma = sc->txdma[idx];
			bus_dmamap_sync(sc->bge_dmatag, dma->dmamap, 0,
			    dma->dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->bge_dmatag, dma->dmamap);
			SLIST_INSERT_HEAD(&sc->txdma_list, dma, link);
			sc->txdma[idx] = NULL;

			m_freem(m);
		}
		sc->bge_txcnt--;
		BGE_INC(sc->bge_tx_saved_considx, BGE_TX_RING_CNT);
		ifp->if_timer = 0;
	}

	if (cur_tx != NULL)
		ifp->if_flags &= ~IFF_OACTIVE;
}

static int
bge_intr(void *xsc)
{
	struct bge_softc *sc;
	struct ifnet *ifp;

	sc = xsc;
	ifp = &sc->ethercom.ec_if;

#ifdef notdef
	/* Avoid this for now -- checking this register is expensive. */
	/* Make sure this is really our interrupt. */
	if (!(CSR_READ_4(sc, BGE_MISC_LOCAL_CTL) & BGE_MLC_INTR_STATE))
		return (0);
#endif
	/* Ack interrupt and stop others from occuring. */
	CSR_WRITE_4(sc, BGE_MBX_IRQ0_LO, 1);

	BGE_EVCNT_INCR(sc->bge_ev_intr);

	/*
	 * Process link state changes.
	 * Grrr. The link status word in the status block does
	 * not work correctly on the BCM5700 rev AX and BX chips,
	 * according to all available information. Hence, we have
	 * to enable MII interrupts in order to properly obtain
	 * async link changes. Unfortunately, this also means that
	 * we have to read the MAC status register to detect link
	 * changes, thereby adding an additional register access to
	 * the interrupt handler.
	 */

	if (sc->bge_quirks & BGE_QUIRK_LINK_STATE_BROKEN) {
		u_int32_t		status;

		status = CSR_READ_4(sc, BGE_MAC_STS);
		if (status & BGE_MACSTAT_MI_INTERRUPT) {
			sc->bge_link = 0;
			callout_stop(&sc->bge_timeout);
			bge_tick(sc);
			/* Clear the interrupt */
			CSR_WRITE_4(sc, BGE_MAC_EVT_ENB,
			    BGE_EVTENB_MI_INTERRUPT);
			bge_miibus_readreg(&sc->bge_dev, 1, BRGPHY_MII_ISR);
			bge_miibus_writereg(&sc->bge_dev, 1, BRGPHY_MII_IMR,
			    BRGPHY_INTRS);
		}
	} else {
		if (sc->bge_rdata->bge_status_block.bge_status &
		    BGE_STATFLAG_LINKSTATE_CHANGED) {
			sc->bge_link = 0;
			callout_stop(&sc->bge_timeout);
			bge_tick(sc);
			/* Clear the interrupt */
			CSR_WRITE_4(sc, BGE_MAC_STS, BGE_MACSTAT_SYNC_CHANGED|
			    BGE_MACSTAT_CFG_CHANGED|BGE_MACSTAT_MI_COMPLETE|
			    BGE_MACSTAT_LINK_CHANGED);
		}
	}

	if (ifp->if_flags & IFF_RUNNING) {
		/* Check RX return ring producer/consumer */
		bge_rxeof(sc);

		/* Check TX ring producer/consumer */
		bge_txeof(sc);
	}

	if (sc->bge_pending_rxintr_change) {
		uint32_t rx_ticks = sc->bge_rx_coal_ticks;
		uint32_t rx_bds = sc->bge_rx_max_coal_bds;
		uint32_t junk;

		CSR_WRITE_4(sc, BGE_HCC_RX_COAL_TICKS, rx_ticks);
		DELAY(10);
		junk = CSR_READ_4(sc, BGE_HCC_RX_COAL_TICKS);

		CSR_WRITE_4(sc, BGE_HCC_RX_MAX_COAL_BDS, rx_bds);
		DELAY(10);
		junk = CSR_READ_4(sc, BGE_HCC_RX_MAX_COAL_BDS);

		sc->bge_pending_rxintr_change = 0;
	}
	bge_handle_events(sc);

	/* Re-enable interrupts. */
	CSR_WRITE_4(sc, BGE_MBX_IRQ0_LO, 0);

	if (ifp->if_flags & IFF_RUNNING && !IFQ_IS_EMPTY(&ifp->if_snd))
		bge_start(ifp);

	return (1);
}

static void
bge_tick(void *xsc)
{
	struct bge_softc *sc = xsc;
	struct mii_data *mii = &sc->bge_mii;
	struct ifmedia *ifm = NULL;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	int s;

	s = splnet();

	bge_stats_update(sc);
	callout_reset(&sc->bge_timeout, hz, bge_tick, sc);
	if (sc->bge_link) {
		splx(s);
		return;
	}

	if (sc->bge_tbi) {
		ifm = &sc->bge_ifmedia;
		if (CSR_READ_4(sc, BGE_MAC_STS) &
		    BGE_MACSTAT_TBI_PCS_SYNCHED) {
			sc->bge_link++;
			CSR_WRITE_4(sc, BGE_MAC_STS, 0xFFFFFFFF);
			if (!IFQ_IS_EMPTY(&ifp->if_snd))
				bge_start(ifp);
		}
		splx(s);
		return;
	}

	mii_tick(mii);

	if (!sc->bge_link && mii->mii_media_status & IFM_ACTIVE &&
	    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
		sc->bge_link++;
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			bge_start(ifp);
	}

	splx(s);
}

static void
bge_stats_update(struct bge_softc *sc)
{
	struct ifnet *ifp = &sc->ethercom.ec_if;
	bus_size_t stats = BGE_MEMWIN_START + BGE_STATS_BLOCK;
	bus_size_t rstats = BGE_RX_STATS;

#define READ_RSTAT(sc, stats, stat) \
	  CSR_READ_4(sc, stats + offsetof(struct bge_mac_stats_regs, stat))

	if (sc->bge_quirks & BGE_QUIRK_5705_CORE) {
		ifp->if_collisions +=
		    READ_RSTAT(sc, rstats, dot3StatsSingleCollisionFrames) +
		    READ_RSTAT(sc, rstats, dot3StatsMultipleCollisionFrames) +
		    READ_RSTAT(sc, rstats, dot3StatsExcessiveCollisions) +
		    READ_RSTAT(sc, rstats, dot3StatsLateCollisions);

		BGE_EVCNT_ADD(sc->bge_ev_tx_xoff,
			      READ_RSTAT(sc, rstats, outXoffSent));
		BGE_EVCNT_ADD(sc->bge_ev_tx_xon,
			      READ_RSTAT(sc, rstats, outXonSent));
		BGE_EVCNT_ADD(sc->bge_ev_rx_xoff,
			      READ_RSTAT(sc, rstats, xoffPauseFramesReceived));
		BGE_EVCNT_ADD(sc->bge_ev_rx_xon,
			      READ_RSTAT(sc, rstats, xonPauseFramesReceived));
		BGE_EVCNT_ADD(sc->bge_ev_rx_macctl,
			      READ_RSTAT(sc, rstats, macControlFramesReceived));
		BGE_EVCNT_ADD(sc->bge_ev_xoffentered,
			      READ_RSTAT(sc, rstats, xoffStateEntered));
		return;
	}

#undef READ_RSTAT
#define READ_STAT(sc, stats, stat) \
	  CSR_READ_4(sc, stats + offsetof(struct bge_stats, stat))

	ifp->if_collisions +=
	  (READ_STAT(sc, stats, dot3StatsSingleCollisionFrames.bge_addr_lo) +
	   READ_STAT(sc, stats, dot3StatsMultipleCollisionFrames.bge_addr_lo) +
	   READ_STAT(sc, stats, dot3StatsExcessiveCollisions.bge_addr_lo) +
	   READ_STAT(sc, stats, dot3StatsLateCollisions.bge_addr_lo)) -
	  ifp->if_collisions;

	BGE_EVCNT_UPD(sc->bge_ev_tx_xoff,
		      READ_STAT(sc, stats, outXoffSent.bge_addr_lo));
	BGE_EVCNT_UPD(sc->bge_ev_tx_xon,
		      READ_STAT(sc, stats, outXonSent.bge_addr_lo));
	BGE_EVCNT_UPD(sc->bge_ev_rx_xoff,
		      READ_STAT(sc, stats,
		      		xoffPauseFramesReceived.bge_addr_lo));
	BGE_EVCNT_UPD(sc->bge_ev_rx_xon,
		      READ_STAT(sc, stats, xonPauseFramesReceived.bge_addr_lo));
	BGE_EVCNT_UPD(sc->bge_ev_rx_macctl,
		      READ_STAT(sc, stats,
		      		macControlFramesReceived.bge_addr_lo));
	BGE_EVCNT_UPD(sc->bge_ev_xoffentered,
		      READ_STAT(sc, stats, xoffStateEntered.bge_addr_lo));

#undef READ_STAT

#ifdef notdef
	ifp->if_collisions +=
	   (sc->bge_rdata->bge_info.bge_stats.dot3StatsSingleCollisionFrames +
	   sc->bge_rdata->bge_info.bge_stats.dot3StatsMultipleCollisionFrames +
	   sc->bge_rdata->bge_info.bge_stats.dot3StatsExcessiveCollisions +
	   sc->bge_rdata->bge_info.bge_stats.dot3StatsLateCollisions) -
	   ifp->if_collisions;
#endif
}

/*
 * Pad outbound frame to ETHER_MIN_NOPAD for an unusual reason.
 * The bge hardware will pad out Tx runts to ETHER_MIN_NOPAD,
 * but when such padded frames employ the  bge IP/TCP checksum offload,
 * the hardware checksum assist gives incorrect results (possibly
 * from incorporating its own padding into the UDP/TCP checksum; who knows).
 * If we pad such runts with zeros, the onboard checksum comes out correct.
 */
static inline int
bge_cksum_pad(struct mbuf *pkt)
{
	struct mbuf *last = NULL;
	int padlen;

	padlen = ETHER_MIN_NOPAD - pkt->m_pkthdr.len;

	/* if there's only the packet-header and we can pad there, use it. */
	if (pkt->m_pkthdr.len == pkt->m_len &&
	    M_TRAILINGSPACE(pkt) >= padlen) {
		last = pkt;
	} else {
		/*
		 * Walk packet chain to find last mbuf. We will either
		 * pad there, or append a new mbuf and pad it
		 * (thus perhaps avoiding the bcm5700 dma-min bug).
		 */
		for (last = pkt; last->m_next != NULL; last = last->m_next) {
	      	       continue; /* do nothing */
		}

		/* `last' now points to last in chain. */
		if (M_TRAILINGSPACE(last) < padlen) {
			/* Allocate new empty mbuf, pad it. Compact later. */
			struct mbuf *n;
			MGET(n, M_DONTWAIT, MT_DATA);
			n->m_len = 0;
			last->m_next = n;
			last = n;
		}
	}

	KDASSERT(!M_READONLY(last));
	KDASSERT(M_TRAILINGSPACE(last) >= padlen);

	/* Now zero the pad area, to avoid the bge cksum-assist bug */
	memset(mtod(last, caddr_t) + last->m_len, 0, padlen);
	last->m_len += padlen;
	pkt->m_pkthdr.len += padlen;
	return 0;
}

/*
 * Compact outbound packets to avoid bug with DMA segments less than 8 bytes.
 */
static inline int
bge_compact_dma_runt(struct mbuf *pkt)
{
	struct mbuf	*m, *prev;
	int 		totlen, prevlen;

	prev = NULL;
	totlen = 0;
	prevlen = -1;

	for (m = pkt; m != NULL; prev = m,m = m->m_next) {
		int mlen = m->m_len;
		int shortfall = 8 - mlen ;

		totlen += mlen;
		if (mlen == 0) {
			continue;
		}
		if (mlen >= 8)
			continue;

		/* If we get here, mbuf data is too small for DMA engine.
		 * Try to fix by shuffling data to prev or next in chain.
		 * If that fails, do a compacting deep-copy of the whole chain.
		 */

		/* Internal frag. If fits in prev, copy it there. */
		if (prev && M_TRAILINGSPACE(prev) >= m->m_len) {
		  	memcpy(prev->m_data + prev->m_len, m->m_data, mlen);
			prev->m_len += mlen;
			m->m_len = 0;
			/* XXX stitch chain */
			prev->m_next = m_free(m);
			m = prev;
			continue;
		}
		else if (m->m_next != NULL &&
			     M_TRAILINGSPACE(m) >= shortfall &&
			     m->m_next->m_len >= (8 + shortfall)) {
		    /* m is writable and have enough data in next, pull up. */

		  	memcpy(m->m_data + m->m_len, m->m_next->m_data,
			    shortfall);
			m->m_len += shortfall;
			m->m_next->m_len -= shortfall;
			m->m_next->m_data += shortfall;
		}
		else if (m->m_next == NULL || 1) {
		  	/* Got a runt at the very end of the packet.
			 * borrow data from the tail of the preceding mbuf and
			 * update its length in-place. (The original data is still
			 * valid, so we can do this even if prev is not writable.)
			 */

			/* if we'd make prev a runt, just move all of its data. */
			KASSERT(prev != NULL /*, ("runt but null PREV")*/);
			KASSERT(prev->m_len >= 8 /*, ("runt prev")*/);

			if ((prev->m_len - shortfall) < 8)
				shortfall = prev->m_len;

#ifdef notyet	/* just do the safe slow thing for now */
			if (!M_READONLY(m)) {
				if (M_LEADINGSPACE(m) < shorfall) {
					void *m_dat;
					m_dat = (m->m_flags & M_PKTHDR) ?
					  m->m_pktdat : m->dat;
					memmove(m_dat, mtod(m, void*), m->m_len);
					m->m_data = m_dat;
				    }
			} else
#endif	/* just do the safe slow thing */
			{
				struct mbuf * n = NULL;
				int newprevlen = prev->m_len - shortfall;

				MGET(n, M_NOWAIT, MT_DATA);
				if (n == NULL)
				   return ENOBUFS;
				KASSERT(m->m_len + shortfall < MLEN
					/*,
					  ("runt %d +prev %d too big\n", m->m_len, shortfall)*/);

				/* first copy the data we're stealing from prev */
				memcpy(n->m_data, prev->m_data + newprevlen,
				    shortfall);

				/* update prev->m_len accordingly */
				prev->m_len -= shortfall;

				/* copy data from runt m */
				memcpy(n->m_data + shortfall, m->m_data,
				    m->m_len);

				/* n holds what we stole from prev, plus m */
				n->m_len = shortfall + m->m_len;

				/* stitch n into chain and free m */
				n->m_next = m->m_next;
				prev->m_next = n;
				/* KASSERT(m->m_next == NULL); */
				m->m_next = NULL;
				m_free(m);
				m = n;	/* for continuing loop */
			}
		}
		prevlen = m->m_len;
	}
	return 0;
}

/*
 * Encapsulate an mbuf chain in the tx ring  by coupling the mbuf data
 * pointers to descriptors.
 */
static int
bge_encap(struct bge_softc *sc, struct mbuf *m_head, u_int32_t *txidx)
{
	struct bge_tx_bd	*f = NULL;
	u_int32_t		frag, cur;
	u_int16_t		csum_flags = 0;
	u_int16_t		txbd_tso_flags = 0;
	struct txdmamap_pool_entry *dma;
	bus_dmamap_t dmamap;
	int			i = 0;
	struct m_tag		*mtag;
	int			use_tso, maxsegsize, error;

	cur = frag = *txidx;

	if (m_head->m_pkthdr.csum_flags) {
		if (m_head->m_pkthdr.csum_flags & M_CSUM_IPv4)
			csum_flags |= BGE_TXBDFLAG_IP_CSUM;
		if (m_head->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4))
			csum_flags |= BGE_TXBDFLAG_TCP_UDP_CSUM;
	}

	/*
	 * If we were asked to do an outboard checksum, and the NIC
	 * has the bug where it sometimes adds in the Ethernet padding,
	 * explicitly pad with zeros so the cksum will be correct either way.
	 * (For now, do this for all chip versions, until newer
	 * are confirmed to not require the workaround.)
	 */
	if ((csum_flags & BGE_TXBDFLAG_TCP_UDP_CSUM) == 0 ||
#ifdef notyet
	    (sc->bge_quirks & BGE_QUIRK_SHORT_CKSUM_BUG) == 0 ||
#endif
	    m_head->m_pkthdr.len >= ETHER_MIN_NOPAD)
		goto check_dma_bug;

	if (bge_cksum_pad(m_head) != 0) {
	    return ENOBUFS;
	}

check_dma_bug:
	if (!(sc->bge_quirks & BGE_QUIRK_5700_SMALLDMA))
		goto doit;
	/*
	 * bcm5700 Revision B silicon cannot handle DMA descriptors with
	 * less than eight bytes.  If we encounter a teeny mbuf
	 * at the end of a chain, we can pad.  Otherwise, copy.
	 */
	if (bge_compact_dma_runt(m_head) != 0)
		return ENOBUFS;

doit:
	dma = SLIST_FIRST(&sc->txdma_list);
	if (dma == NULL)
		return ENOBUFS;
	dmamap = dma->dmamap;

	/*
	 * Set up any necessary TSO state before we start packing...
	 */
	use_tso = (m_head->m_pkthdr.csum_flags & M_CSUM_TSOv4) != 0;
	if (!use_tso) {
		maxsegsize = 0;
	} else {	/* TSO setup */
		unsigned  mss;
		struct ether_header *eh;
		unsigned ip_tcp_hlen, iptcp_opt_words, tcp_seg_flags, offset;
		struct mbuf * m0 = m_head;
		struct ip *ip;
		struct tcphdr *th;
		int iphl, hlen;

		/*
		 * XXX It would be nice if the mbuf pkthdr had offset
		 * fields for the protocol headers.
		 */

		eh = mtod(m0, struct ether_header *);
		switch (htons(eh->ether_type)) {
		case ETHERTYPE_IP:
			offset = ETHER_HDR_LEN;
			break;

		case ETHERTYPE_VLAN:
			offset = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
			break;

		default:
			/*
			 * Don't support this protocol or encapsulation.
			 */
			return (ENOBUFS);
		}

		/*
		 * TCP/IP headers are in the first mbuf; we can do
		 * this the easy way.
		 */
		iphl = M_CSUM_DATA_IPv4_IPHL(m0->m_pkthdr.csum_data);
		hlen = iphl + offset;
		if (__predict_false(m0->m_len <
				    (hlen + sizeof(struct tcphdr)))) {

			  printf("TSO: hard case m0->m_len == %d <"
				 " ip/tcp hlen %zd, not handled yet\n",
				 m0->m_len, hlen+ sizeof(struct tcphdr));
#ifdef NOTYET
			/*
			 * XXX jonathan@NetBSD.org: untested.
			 * how to force  this branch to be taken?
			 */
			BGE_EVCNT_INCR(&sc->sc_ev_txtsopain);

			m_copydata(m0, offset, sizeof(ip), &ip);
			m_copydata(m0, hlen, sizeof(th), &th);

			ip.ip_len = 0;

			m_copyback(m0, hlen + offsetof(struct ip, ip_len),
			    sizeof(ip.ip_len), &ip.ip_len);

			th.th_sum = in_cksum_phdr(ip.ip_src.s_addr,
			    ip.ip_dst.s_addr, htons(IPPROTO_TCP));

			m_copyback(m0, hlen + offsetof(struct tcphdr, th_sum),
			    sizeof(th.th_sum), &th.th_sum);

			hlen += th.th_off << 2;
			iptcp_opt_words	= hlen;
#else
			/*
			 * if_wm "hard" case not yet supported, can we not
			 * mandate it out of existence?
			 */
			(void) ip; (void)th; (void) ip_tcp_hlen;

			return ENOBUFS;
#endif
		} else {
			ip = (struct ip *) (mtod(m0, caddr_t) + offset);
			th = (struct tcphdr *) (mtod(m0, caddr_t) + hlen);
			ip_tcp_hlen = iphl +  (th->th_off << 2);

			/* Total IP/TCP options, in 32-bit words */
			iptcp_opt_words = (ip_tcp_hlen
					   - sizeof(struct tcphdr)
					   - sizeof(struct ip)) >> 2;
		}
		if (BGE_IS_5750_OR_BEYOND(sc)) {
			th->th_sum = 0;
			csum_flags &= ~(BGE_TXBDFLAG_TCP_UDP_CSUM);
		} else {
			/*
			 * XXX jonathan@NetBSD.org: 5705 untested.
			 * Requires TSO firmware patch for 5701/5703/5704.
			 */
			th->th_sum = in_cksum_phdr(ip->ip_src.s_addr,
			    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
		}

		mss = m_head->m_pkthdr.segsz;
		txbd_tso_flags |=
		    BGE_TXBDFLAG_CPU_PRE_DMA |
		    BGE_TXBDFLAG_CPU_POST_DMA;

		/*
		 * Our NIC TSO-assist assumes TSO has standard, optionless
		 * IPv4 and TCP headers, which total 40 bytes. By default,
		 * the NIC copies 40 bytes of IP/TCP header from the
		 * supplied header into the IP/TCP header portion of
		 * each post-TSO-segment. If the supplied packet has IP or
		 * TCP options, we need to tell the NIC to copy those extra
		 * bytes into each  post-TSO header, in addition to the normal
		 * 40-byte IP/TCP header (and to leave space accordingly).
		 * Unfortunately, the driver encoding of option length
		 * varies across different ASIC families.
		 */
		tcp_seg_flags = 0;
		if (iptcp_opt_words) {
			if ( BGE_IS_5705_OR_BEYOND(sc)) {
				tcp_seg_flags =
					iptcp_opt_words << 11;
			} else {
				txbd_tso_flags |=
					iptcp_opt_words << 12;
			}
		}
		maxsegsize = mss | tcp_seg_flags;
		ip->ip_len = htons(mss + ip_tcp_hlen);

	}	/* TSO setup */

	/*
	 * Start packing the mbufs in this chain into
	 * the fragment pointers. Stop when we run out
	 * of fragments or hit the end of the mbuf chain.
	 */
	error = bus_dmamap_load_mbuf(sc->bge_dmatag, dmamap, m_head,
	    BUS_DMA_NOWAIT);
	if (error) {
		return(ENOBUFS);
	}
	/*
	 * Sanity check: avoid coming within 16 descriptors
	 * of the end of the ring.
	 */
	if (dmamap->dm_nsegs > (BGE_TX_RING_CNT - sc->bge_txcnt - 16)) {
		BGE_TSO_PRINTF(("%s: "
		    " dmamap_load_mbuf too close to ring wrap\n",
		    sc->bge_dev.dv_xname));
		goto fail_unload;
	}

	mtag = sc->ethercom.ec_nvlans ?
	    m_tag_find(m_head, PACKET_TAG_VLAN, NULL) : NULL;


	/* Iterate over dmap-map fragments. */
	for (i = 0; i < dmamap->dm_nsegs; i++) {
		f = &sc->bge_rdata->bge_tx_ring[frag];
		if (sc->bge_cdata.bge_tx_chain[frag] != NULL)
			break;

		bge_set_hostaddr(&f->bge_addr, dmamap->dm_segs[i].ds_addr);
		f->bge_len = dmamap->dm_segs[i].ds_len;

		/*
		 * For 5751 and follow-ons, for TSO we must turn
		 * off checksum-assist flag in the tx-descr, and
		 * supply the ASIC-revision-specific encoding
		 * of TSO flags and segsize.
		 */
		if (use_tso) {
			if (BGE_IS_5750_OR_BEYOND(sc) || i == 0) {
				f->bge_rsvd = maxsegsize;
				f->bge_flags = csum_flags | txbd_tso_flags;
			} else {
				f->bge_rsvd = 0;
				f->bge_flags =
				  (csum_flags | txbd_tso_flags) & 0x0fff;
			}
		} else {
			f->bge_rsvd = 0;
			f->bge_flags = csum_flags;
		}

		if (mtag != NULL) {
			f->bge_flags |= BGE_TXBDFLAG_VLAN_TAG;
			f->bge_vlan_tag = VLAN_TAG_VALUE(mtag);
		} else {
			f->bge_vlan_tag = 0;
		}
		cur = frag;
		BGE_INC(frag, BGE_TX_RING_CNT);
	}

	if (i < dmamap->dm_nsegs) {
		BGE_TSO_PRINTF(("%s: reached %d < dm_nsegs %d\n",
		    sc->bge_dev.dv_xname, i, dmamap->dm_nsegs));
		goto fail_unload;
	}

	bus_dmamap_sync(sc->bge_dmatag, dmamap, 0, dmamap->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	if (frag == sc->bge_tx_saved_considx) {
		BGE_TSO_PRINTF(("%s: frag %d = wrapped id %d?\n",
		    sc->bge_dev.dv_xname, frag, sc->bge_tx_saved_considx));

		goto fail_unload;
	}

	sc->bge_rdata->bge_tx_ring[cur].bge_flags |= BGE_TXBDFLAG_END;
	sc->bge_cdata.bge_tx_chain[cur] = m_head;
	SLIST_REMOVE_HEAD(&sc->txdma_list, link);
	sc->txdma[cur] = dma;
	sc->bge_txcnt += dmamap->dm_nsegs;

	*txidx = frag;

	return(0);

 fail_unload:
	bus_dmamap_unload(sc->bge_dmatag, dmamap);

	return ENOBUFS;
}

/*
 * Main transmit routine. To avoid having to do mbuf copies, we put pointers
 * to the mbuf data regions directly in the transmit descriptors.
 */
static void
bge_start(struct ifnet *ifp)
{
	struct bge_softc *sc;
	struct mbuf *m_head = NULL;
	u_int32_t prodidx;
	int pkts = 0;

	sc = ifp->if_softc;

	if (!sc->bge_link && ifp->if_snd.ifq_len < 10)
		return;

	prodidx = sc->bge_tx_prodidx;

	while(sc->bge_cdata.bge_tx_chain[prodidx] == NULL) {
		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

#if 0
		/*
		 * XXX
		 * safety overkill.  If this is a fragmented packet chain
		 * with delayed TCP/UDP checksums, then only encapsulate
		 * it if we have enough descriptors to handle the entire
		 * chain at once.
		 * (paranoia -- may not actually be needed)
		 */
		if (m_head->m_flags & M_FIRSTFRAG &&
		    m_head->m_pkthdr.csum_flags & (CSUM_DELAY_DATA)) {
			if ((BGE_TX_RING_CNT - sc->bge_txcnt) <
			    M_CSUM_DATA_IPv4_OFFSET(m_head->m_pkthdr.csum_data) + 16) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
		}
#endif

		/*
		 * Pack the data into the transmit ring. If we
		 * don't have room, set the OACTIVE flag and wait
		 * for the NIC to drain the ring.
		 */
		if (bge_encap(sc, m_head, &prodidx)) {
			printf("bge: failed on len %d?\n", m_head->m_pkthdr.len);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/* now we are committed to transmit the packet */
		IFQ_DEQUEUE(&ifp->if_snd, m_head);
		pkts++;

#if NBPFILTER > 0
		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m_head);
#endif
	}
	if (pkts == 0)
		return;

	/* Transmit */
	CSR_WRITE_4(sc, BGE_MBX_TX_HOST_PROD0_LO, prodidx);
	if (sc->bge_quirks & BGE_QUIRK_PRODUCER_BUG)	/* 5700 b2 errata */
		CSR_WRITE_4(sc, BGE_MBX_TX_HOST_PROD0_LO, prodidx);

	sc->bge_tx_prodidx = prodidx;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

static int
bge_init(struct ifnet *ifp)
{
	struct bge_softc *sc = ifp->if_softc;
	u_int16_t *m;
	int s, error;

	s = splnet();

	ifp = &sc->ethercom.ec_if;

	/* Cancel pending I/O and flush buffers. */
	bge_stop(sc);
	bge_reset(sc);
	bge_chipinit(sc);

	/*
	 * Init the various state machines, ring
	 * control blocks and firmware.
	 */
	error = bge_blockinit(sc);
	if (error != 0) {
		printf("%s: initialization error %d\n", sc->bge_dev.dv_xname,
		    error);
		splx(s);
		return error;
	}

	ifp = &sc->ethercom.ec_if;

	/* Specify MTU. */
	CSR_WRITE_4(sc, BGE_RX_MTU, ifp->if_mtu +
	    ETHER_HDR_LEN + ETHER_CRC_LEN + ETHER_VLAN_ENCAP_LEN);

	/* Load our MAC address. */
	m = (u_int16_t *)&(LLADDR(ifp->if_sadl)[0]);
	CSR_WRITE_4(sc, BGE_MAC_ADDR1_LO, htons(m[0]));
	CSR_WRITE_4(sc, BGE_MAC_ADDR1_HI, (htons(m[1]) << 16) | htons(m[2]));

	/* Enable or disable promiscuous mode as needed. */
	if (ifp->if_flags & IFF_PROMISC) {
		BGE_SETBIT(sc, BGE_RX_MODE, BGE_RXMODE_RX_PROMISC);
	} else {
		BGE_CLRBIT(sc, BGE_RX_MODE, BGE_RXMODE_RX_PROMISC);
	}

	/* Program multicast filter. */
	bge_setmulti(sc);

	/* Init RX ring. */
	bge_init_rx_ring_std(sc);

	/* Init jumbo RX ring. */
	if (ifp->if_mtu > (ETHERMTU + ETHER_HDR_LEN + ETHER_CRC_LEN))
		bge_init_rx_ring_jumbo(sc);

	/* Init our RX return ring index */
	sc->bge_rx_saved_considx = 0;

	/* Init TX ring. */
	bge_init_tx_ring(sc);

	/* Turn on transmitter */
	BGE_SETBIT(sc, BGE_TX_MODE, BGE_TXMODE_ENABLE);

	/* Turn on receiver */
	BGE_SETBIT(sc, BGE_RX_MODE, BGE_RXMODE_ENABLE);

	CSR_WRITE_4(sc, BGE_MAX_RX_FRAME_LOWAT, 2);

	/* Tell firmware we're alive. */
	BGE_SETBIT(sc, BGE_MODE_CTL, BGE_MODECTL_STACKUP);

	/* Enable host interrupts. */
	BGE_SETBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_CLEAR_INTA);
	BGE_CLRBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_MASK_PCI_INTR);
	CSR_WRITE_4(sc, BGE_MBX_IRQ0_LO, 0);

	bge_ifmedia_upd(ifp);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	callout_reset(&sc->bge_timeout, hz, bge_tick, sc);

	return 0;
}

/*
 * Set media options.
 */
static int
bge_ifmedia_upd(struct ifnet *ifp)
{
	struct bge_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->bge_mii;
	struct ifmedia *ifm = &sc->bge_ifmedia;

	/* If this is a 1000baseX NIC, enable the TBI port. */
	if (sc->bge_tbi) {
		if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
			return(EINVAL);
		switch(IFM_SUBTYPE(ifm->ifm_media)) {
		case IFM_AUTO:
			break;
		case IFM_1000_SX:
			if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX) {
				BGE_CLRBIT(sc, BGE_MAC_MODE,
				    BGE_MACMODE_HALF_DUPLEX);
			} else {
				BGE_SETBIT(sc, BGE_MAC_MODE,
				    BGE_MACMODE_HALF_DUPLEX);
			}
			break;
		default:
			return(EINVAL);
		}
		/* XXX 802.3x flow control for 1000BASE-SX */
		return(0);
	}

	sc->bge_link = 0;
	mii_mediachg(mii);

	return(0);
}

/*
 * Report current media status.
 */
static void
bge_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct bge_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->bge_mii;

	if (sc->bge_tbi) {
		ifmr->ifm_status = IFM_AVALID;
		ifmr->ifm_active = IFM_ETHER;
		if (CSR_READ_4(sc, BGE_MAC_STS) &
		    BGE_MACSTAT_TBI_PCS_SYNCHED)
			ifmr->ifm_status |= IFM_ACTIVE;
		ifmr->ifm_active |= IFM_1000_SX;
		if (CSR_READ_4(sc, BGE_MAC_MODE) & BGE_MACMODE_HALF_DUPLEX)
			ifmr->ifm_active |= IFM_HDX;
		else
			ifmr->ifm_active |= IFM_FDX;
		return;
	}

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = (mii->mii_media_active & ~IFM_ETH_FMASK) |
	    sc->bge_flowflags;
}

static int
bge_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct bge_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error = 0;
	struct mii_data *mii;

	s = splnet();

	switch(command) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			/*
			 * If only the state of the PROMISC flag changed,
			 * then just use the 'set promisc mode' command
			 * instead of reinitializing the entire NIC. Doing
			 * a full re-init means reloading the firmware and
			 * waiting for it to start up, which may take a
			 * second or two.
			 */
			if (ifp->if_flags & IFF_RUNNING &&
			    ifp->if_flags & IFF_PROMISC &&
			    !(sc->bge_if_flags & IFF_PROMISC)) {
				BGE_SETBIT(sc, BGE_RX_MODE,
				    BGE_RXMODE_RX_PROMISC);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC) &&
			    sc->bge_if_flags & IFF_PROMISC) {
				BGE_CLRBIT(sc, BGE_RX_MODE,
				    BGE_RXMODE_RX_PROMISC);
			} else if (!(sc->bge_if_flags & IFF_UP))
				bge_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING) {
				bge_stop(sc);
			}
		}
		sc->bge_if_flags = ifp->if_flags;
		error = 0;
		break;
	case SIOCSIFMEDIA:
		/* XXX Flow control is not supported for 1000BASE-SX */
		if (sc->bge_tbi) {
			ifr->ifr_media &= ~IFM_ETH_FMASK;
			sc->bge_flowflags = 0;
		}

		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0) {
		    	ifr->ifr_media &= ~IFM_ETH_FMASK;
		}
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				/* We an do both TXPAUSE and RXPAUSE. */
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
			sc->bge_flowflags = ifr->ifr_media & IFM_ETH_FMASK;
		}
		/* FALLTHROUGH */
	case SIOCGIFMEDIA:
		if (sc->bge_tbi) {
			error = ifmedia_ioctl(ifp, ifr, &sc->bge_ifmedia,
			    command);
		} else {
			mii = &sc->bge_mii;
			error = ifmedia_ioctl(ifp, ifr, &mii->mii_media,
			    command);
		}
		break;
	default:
		error = ether_ioctl(ifp, command, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING)
				bge_setmulti(sc);
			error = 0;
		}
		break;
	}

	splx(s);

	return(error);
}

static void
bge_watchdog(struct ifnet *ifp)
{
	struct bge_softc *sc;

	sc = ifp->if_softc;

	printf("%s: watchdog timeout -- resetting\n", sc->bge_dev.dv_xname);

	ifp->if_flags &= ~IFF_RUNNING;
	bge_init(ifp);

	ifp->if_oerrors++;
}

static void
bge_stop_block(struct bge_softc *sc, bus_addr_t reg, uint32_t bit)
{
	int i;

	BGE_CLRBIT(sc, reg, bit);

	for (i = 0; i < BGE_TIMEOUT; i++) {
		if ((CSR_READ_4(sc, reg) & bit) == 0)
			return;
		delay(100);
		if (sc->bge_pcie)
		  DELAY(1000);
	}

	printf("%s: block failed to stop: reg 0x%lx, bit 0x%08x\n",
	    sc->bge_dev.dv_xname, (u_long) reg, bit);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
bge_stop(struct bge_softc *sc)
{
	struct ifnet *ifp = &sc->ethercom.ec_if;

	callout_stop(&sc->bge_timeout);

	/*
	 * Disable all of the receiver blocks
	 */
	bge_stop_block(sc, BGE_RX_MODE, BGE_RXMODE_ENABLE);
	bge_stop_block(sc, BGE_RBDI_MODE, BGE_RBDIMODE_ENABLE);
	bge_stop_block(sc, BGE_RXLP_MODE, BGE_RXLPMODE_ENABLE);
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		bge_stop_block(sc, BGE_RXLS_MODE, BGE_RXLSMODE_ENABLE);
	}
	bge_stop_block(sc, BGE_RDBDI_MODE, BGE_RBDIMODE_ENABLE);
	bge_stop_block(sc, BGE_RDC_MODE, BGE_RDCMODE_ENABLE);
	bge_stop_block(sc, BGE_RBDC_MODE, BGE_RBDCMODE_ENABLE);

	/*
	 * Disable all of the transmit blocks
	 */
	bge_stop_block(sc, BGE_SRS_MODE, BGE_SRSMODE_ENABLE);
	bge_stop_block(sc, BGE_SBDI_MODE, BGE_SBDIMODE_ENABLE);
	bge_stop_block(sc, BGE_SDI_MODE, BGE_SDIMODE_ENABLE);
	bge_stop_block(sc, BGE_RDMA_MODE, BGE_RDMAMODE_ENABLE);
	bge_stop_block(sc, BGE_SDC_MODE, BGE_SDCMODE_ENABLE);
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		bge_stop_block(sc, BGE_DMAC_MODE, BGE_DMACMODE_ENABLE);
	}
	bge_stop_block(sc, BGE_SBDC_MODE, BGE_SBDCMODE_ENABLE);

	/*
	 * Shut down all of the memory managers and related
	 * state machines.
	 */
	bge_stop_block(sc, BGE_HCC_MODE, BGE_HCCMODE_ENABLE);
	bge_stop_block(sc, BGE_WDMA_MODE, BGE_WDMAMODE_ENABLE);
	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		bge_stop_block(sc, BGE_MBCF_MODE, BGE_MBCFMODE_ENABLE);
	}

	CSR_WRITE_4(sc, BGE_FTQ_RESET, 0xFFFFFFFF);
	CSR_WRITE_4(sc, BGE_FTQ_RESET, 0);

	if ((sc->bge_quirks & BGE_QUIRK_5705_CORE) == 0) {
		bge_stop_block(sc, BGE_BMAN_MODE, BGE_BMANMODE_ENABLE);
		bge_stop_block(sc, BGE_MARB_MODE, BGE_MARBMODE_ENABLE);
	}

	/* Disable host interrupts. */
	BGE_SETBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_MASK_PCI_INTR);
	CSR_WRITE_4(sc, BGE_MBX_IRQ0_LO, 1);

	/*
	 * Tell firmware we're shutting down.
	 */
	BGE_CLRBIT(sc, BGE_MODE_CTL, BGE_MODECTL_STACKUP);

	/* Free the RX lists. */
	bge_free_rx_ring_std(sc);

	/* Free jumbo RX list. */
	bge_free_rx_ring_jumbo(sc);

	/* Free TX buffers. */
	bge_free_tx_ring(sc);

	/*
	 * Isolate/power down the PHY.
	 */
	if (!sc->bge_tbi)
		mii_down(&sc->bge_mii);

	sc->bge_link = 0;

	sc->bge_tx_saved_considx = BGE_TXCONS_UNSET;

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static void
bge_shutdown(void *xsc)
{
	struct bge_softc *sc = (struct bge_softc *)xsc;

	bge_stop(sc);
	bge_reset(sc);
}


static int
sysctl_bge_verify(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int*)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

#if 0
	DPRINTF2(("%s: t = %d, nodenum = %d, rnodenum = %d\n", __func__, t,
	    node.sysctl_num, rnode->sysctl_num));
#endif

	if (node.sysctl_num == bge_rxthresh_nodenum) {
		if (t < 0 || t >= NBGE_RX_THRESH)
			return (EINVAL);
		bge_update_all_threshes(t);
	} else
		return (EINVAL);

	*(int*)rnode->sysctl_data = t;

	return (0);
}

/*
 * Set up sysctl(3) MIB, hw.bge.*.
 *
 * TBD condition SYSCTL_PERMANENT on being an LKM or not
 */
SYSCTL_SETUP(sysctl_bge, "sysctl bge subtree setup")
{
	int rc, bge_root_num;
	const struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0) {
		goto err;
	}

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "bge",
	    SYSCTL_DESCR("BGE interface controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	bge_root_num = node->sysctl_num;

	/* BGE Rx interrupt mitigation level */
	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "rx_lvl",
	    SYSCTL_DESCR("BGE receive interrupt mitigation level"),
	    sysctl_bge_verify, 0,
	    &bge_rx_thresh_lvl,
	    0, CTL_HW, bge_root_num, CTL_CREATE,
	    CTL_EOL)) != 0) {
		goto err;
	}

	bge_rxthresh_nodenum = node->sysctl_num;

	return;

err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

static void
bge_powerhook(int why, void *hdl)
{
	struct bge_softc *sc = (struct bge_softc *)hdl;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	struct pci_attach_args *pa = &(sc->bge_pa);
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;

	switch (why) {
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
		bge_shutdown(sc);
		break;
	case PWR_SOFTRESUME:
		if (ifp->if_flags & IFF_UP) {
			ifp->if_flags &= ~IFF_RUNNING;
			bge_init(ifp);
		}
		break;
	case PWR_SUSPEND:
	case PWR_STANDBY:
		pci_conf_capture(pc, tag, &sc->bge_pciconf);
		break;
	case PWR_RESUME:
		pci_conf_restore(pc, tag, &sc->bge_pciconf);
		break;
	}

	return;
}
