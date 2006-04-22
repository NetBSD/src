/*	$NetBSD: i82557.c,v 1.96.6.1 2006/04/22 11:38:55 simonb Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1995, David Greenman
 * Copyright (c) 2001 Jonathan Lemon <jlemon@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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
 *
 *	Id: if_fxp.c,v 1.113 2001/05/17 23:50:24 jlemon
 */

/*
 * Device driver for the Intel i82557 fast Ethernet controller,
 * and its successors, the i82558 and i82559.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i82557.c,v 1.96.6.1 2006/04/22 11:38:55 simonb Exp $");

#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/endian.h>

#include <uvm/uvm_extern.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/ic/i82557reg.h>
#include <dev/ic/i82557var.h>

#include <dev/microcode/i8255x/rcvbundl.h>

/*
 * NOTE!  On the Alpha, we have an alignment constraint.  The
 * card DMAs the packet immediately following the RFA.  However,
 * the first thing in the packet is a 14-byte Ethernet header.
 * This means that the packet is misaligned.  To compensate,
 * we actually offset the RFA 2 bytes into the cluster.  This
 * alignes the packet after the Ethernet header at a 32-bit
 * boundary.  HOWEVER!  This means that the RFA is misaligned!
 */
#define	RFA_ALIGNMENT_FUDGE	2

/*
 * The configuration byte map has several undefined fields which
 * must be one or must be zero.  Set up a template for these bits
 * only (assuming an i82557 chip), leaving the actual configuration
 * for fxp_init().
 *
 * See the definition of struct fxp_cb_config for the bit definitions.
 */
const u_int8_t fxp_cb_config_template[] = {
	0x0, 0x0,		/* cb_status */
	0x0, 0x0,		/* cb_command */
	0x0, 0x0, 0x0, 0x0,	/* link_addr */
	0x0,	/*  0 */
	0x0,	/*  1 */
	0x0,	/*  2 */
	0x0,	/*  3 */
	0x0,	/*  4 */
	0x0,	/*  5 */
	0x32,	/*  6 */
	0x0,	/*  7 */
	0x0,	/*  8 */
	0x0,	/*  9 */
	0x6,	/* 10 */
	0x0,	/* 11 */
	0x0,	/* 12 */
	0x0,	/* 13 */
	0xf2,	/* 14 */
	0x48,	/* 15 */
	0x0,	/* 16 */
	0x40,	/* 17 */
	0xf0,	/* 18 */
	0x0,	/* 19 */
	0x3f,	/* 20 */
	0x5,	/* 21 */
	0x0,	/* 22 */
	0x0,	/* 23 */
	0x0,	/* 24 */
	0x0,	/* 25 */
	0x0,	/* 26 */
	0x0,	/* 27 */
	0x0,	/* 28 */
	0x0,	/* 29 */
	0x0,	/* 30 */
	0x0,	/* 31 */
};

void	fxp_mii_initmedia(struct fxp_softc *);
int	fxp_mii_mediachange(struct ifnet *);
void	fxp_mii_mediastatus(struct ifnet *, struct ifmediareq *);

void	fxp_80c24_initmedia(struct fxp_softc *);
int	fxp_80c24_mediachange(struct ifnet *);
void	fxp_80c24_mediastatus(struct ifnet *, struct ifmediareq *);

void	fxp_start(struct ifnet *);
int	fxp_ioctl(struct ifnet *, u_long, caddr_t);
void	fxp_watchdog(struct ifnet *);
int	fxp_init(struct ifnet *);
void	fxp_stop(struct ifnet *, int);

void	fxp_txintr(struct fxp_softc *);
void	fxp_rxintr(struct fxp_softc *);

int	fxp_rx_hwcksum(struct mbuf *, const struct fxp_rfa *);

void	fxp_rxdrain(struct fxp_softc *);
int	fxp_add_rfabuf(struct fxp_softc *, bus_dmamap_t, int);
int	fxp_mdi_read(struct device *, int, int);
void	fxp_statchg(struct device *);
void	fxp_mdi_write(struct device *, int, int, int);
void	fxp_autosize_eeprom(struct fxp_softc*);
void	fxp_read_eeprom(struct fxp_softc *, u_int16_t *, int, int);
void	fxp_write_eeprom(struct fxp_softc *, u_int16_t *, int, int);
void	fxp_eeprom_update_cksum(struct fxp_softc *);
void	fxp_get_info(struct fxp_softc *, u_int8_t *);
void	fxp_tick(void *);
void	fxp_mc_setup(struct fxp_softc *);
void	fxp_load_ucode(struct fxp_softc *);

void	fxp_shutdown(void *);
void	fxp_power(int, void *);

int	fxp_copy_small = 0;

/*
 * Variables for interrupt mitigating microcode.
 */
int	fxp_int_delay = 1000;		/* usec */
int	fxp_bundle_max = 6;		/* packets */

struct fxp_phytype {
	int	fp_phy;		/* type of PHY, -1 for MII at the end. */
	void	(*fp_init)(struct fxp_softc *);
} fxp_phytype_table[] = {
	{ FXP_PHY_80C24,		fxp_80c24_initmedia },
	{ -1,				fxp_mii_initmedia },
};

/*
 * Set initial transmit threshold at 64 (512 bytes). This is
 * increased by 64 (512 bytes) at a time, to maximum of 192
 * (1536 bytes), if an underrun occurs.
 */
static int tx_threshold = 64;

/*
 * Wait for the previous command to be accepted (but not necessarily
 * completed).
 */
static inline void
fxp_scb_wait(struct fxp_softc *sc)
{
	int i = 10000;

	while (CSR_READ_1(sc, FXP_CSR_SCB_COMMAND) && --i)
		delay(2);
	if (i == 0)
		log(LOG_WARNING,
		    "%s: WARNING: SCB timed out!\n", sc->sc_dev.dv_xname);
}

/*
 * Submit a command to the i82557.
 */
static inline void
fxp_scb_cmd(struct fxp_softc *sc, u_int8_t cmd)
{

	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, cmd);
}

/*
 * Finish attaching an i82557 interface.  Called by bus-specific front-end.
 */
void
fxp_attach(struct fxp_softc *sc)
{
	u_int8_t enaddr[ETHER_ADDR_LEN];
	struct ifnet *ifp;
	bus_dma_segment_t seg;
	int rseg, i, error;
	struct fxp_phytype *fp;

	callout_init(&sc->sc_callout);

	/*
	 * Enable some good stuff on i82558 and later.
	 */
	if (sc->sc_rev >= FXP_REV_82558_A4) {
		/* Enable the extended TxCB. */
		sc->sc_flags |= FXPF_EXT_TXCB;
	}

        /*
	 * Enable use of extended RFDs and TCBs for 82550
	 * and later chips. Note: we need extended TXCB support
	 * too, but that's already enabled by the code above.
	 * Be careful to do this only on the right devices.
	 */
	if (sc->sc_rev == FXP_REV_82550 || sc->sc_rev == FXP_REV_82550_C) {
		sc->sc_flags |= FXPF_EXT_RFA | FXPF_IPCB;
		sc->sc_txcmd = htole16(FXP_CB_COMMAND_IPCBXMIT);
	} else {
		sc->sc_txcmd = htole16(FXP_CB_COMMAND_XMIT);
	}

	sc->sc_rfa_size =
	    (sc->sc_flags & FXPF_EXT_RFA) ? RFA_EXT_SIZE : RFA_SIZE;

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct fxp_control_data), PAGE_SIZE, 0, &seg, 1, &rseg,
	    0)) != 0) {
		aprint_error(
		    "%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct fxp_control_data), (caddr_t *)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		aprint_error("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}
	sc->sc_cdseg = seg;
	sc->sc_cdnseg = rseg;

	memset(sc->sc_control_data, 0, sizeof(struct fxp_control_data));

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct fxp_control_data), 1,
	    sizeof(struct fxp_control_data), 0, 0, &sc->sc_dmamap)) != 0) {
		aprint_error("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
	    sc->sc_control_data, sizeof(struct fxp_control_data), NULL,
	    0)) != 0) {
		aprint_error(
		    "%s: can't load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < FXP_NTXCB; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    (sc->sc_flags & FXPF_IPCB) ? FXP_IPCB_NTXSEG : FXP_NTXSEG,
		    MCLBYTES, 0, 0, &FXP_DSTX(sc, i)->txs_dmamap)) != 0) {
			aprint_error("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < FXP_NRFABUFS; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxmaps[i])) != 0) {
			aprint_error("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
	}

	/* Initialize MAC address and media structures. */
	fxp_get_info(sc, enaddr);

	aprint_normal("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	ifp = &sc->sc_ethercom.ec_if;

	/*
	 * Get info about our media interface, and initialize it.  Note
	 * the table terminates itself with a phy of -1, indicating
	 * that we're using MII.
	 */
	for (fp = fxp_phytype_table; fp->fp_phy != -1; fp++)
		if (fp->fp_phy == sc->phy_primary_device)
			break;
	(*fp->fp_init)(sc);

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = fxp_ioctl;
	ifp->if_start = fxp_start;
	ifp->if_watchdog = fxp_watchdog;
	ifp->if_init = fxp_init;
	ifp->if_stop = fxp_stop;
	IFQ_SET_READY(&ifp->if_snd);

	if (sc->sc_flags & FXPF_IPCB) {
		KASSERT(sc->sc_flags & FXPF_EXT_RFA); /* we have both or none */
		/*
		 * IFCAP_CSUM_IPv4_Tx seems to have a problem,
		 * at least, on i82550 rev.12.
		 * specifically, it doesn't calculate ipv4 checksum correctly
		 * when sending 20 byte ipv4 header + 1 or 2 byte data.
		 * FreeBSD driver has related comments.
		 */
		ifp->if_capabilities =
		    IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
		    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;
		sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;
	}

	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif

#ifdef FXP_EVENT_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_txstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txstall");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "rxintr");
	if (sc->sc_rev >= FXP_REV_82558_A4) {
		evcnt_attach_dynamic(&sc->sc_ev_txpause, EVCNT_TYPE_MISC,
		    NULL, sc->sc_dev.dv_xname, "txpause");
		evcnt_attach_dynamic(&sc->sc_ev_rxpause, EVCNT_TYPE_MISC,
		    NULL, sc->sc_dev.dv_xname, "rxpause");
	}
#endif /* FXP_EVENT_COUNTERS */

	/*
	 * Add shutdown hook so that DMA is disabled prior to reboot. Not
	 * doing do could allow DMA to corrupt kernel memory during the
	 * reboot before the driver initializes.
	 */
	sc->sc_sdhook = shutdownhook_establish(fxp_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	/*
  	 * Add suspend hook, for similar reasons..
	 */
	sc->sc_powerhook = powerhook_establish(fxp_power, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);

	/* The attach is successful. */
	sc->sc_flags |= FXPF_ATTACHED;

	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall though.
	 */
 fail_5:
	for (i = 0; i < FXP_NRFABUFS; i++) {
		if (sc->sc_rxmaps[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_rxmaps[i]);
	}
 fail_4:
	for (i = 0; i < FXP_NTXCB; i++) {
		if (FXP_DSTX(sc, i)->txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    FXP_DSTX(sc, i)->txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct fxp_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 fail_0:
	return;
}

void
fxp_mii_initmedia(struct fxp_softc *sc)
{
	int flags;

	sc->sc_flags |= FXPF_MII;

	sc->sc_mii.mii_ifp = &sc->sc_ethercom.ec_if;
	sc->sc_mii.mii_readreg = fxp_mdi_read;
	sc->sc_mii.mii_writereg = fxp_mdi_write;
	sc->sc_mii.mii_statchg = fxp_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, fxp_mii_mediachange,
	    fxp_mii_mediastatus);

	flags = MIIF_NOISOLATE;
	if (sc->sc_rev >= FXP_REV_82558_A4)
		flags |= MIIF_DOPAUSE;
	/*
	 * The i82557 wedges if all of its PHYs are isolated!
	 */
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, flags);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
}

void
fxp_80c24_initmedia(struct fxp_softc *sc)
{

	/*
	 * The Seeq 80c24 AutoDUPLEX(tm) Ethernet Interface Adapter
	 * doesn't have a programming interface of any sort.  The
	 * media is sensed automatically based on how the link partner
	 * is configured.  This is, in essence, manual configuration.
	 */
	aprint_normal("%s: Seeq 80c24 AutoDUPLEX media interface present\n",
	    sc->sc_dev.dv_xname);
	ifmedia_init(&sc->sc_mii.mii_media, 0, fxp_80c24_mediachange,
	    fxp_80c24_mediastatus);
	ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
}

/*
 * Device shutdown routine. Called at system shutdown after sync. The
 * main purpose of this routine is to shut off receiver DMA so that
 * kernel memory doesn't get clobbered during warmboot.
 */
void
fxp_shutdown(void *arg)
{
	struct fxp_softc *sc = arg;

	/*
	 * Since the system's going to halt shortly, don't bother
	 * freeing mbufs.
	 */
	fxp_stop(&sc->sc_ethercom.ec_if, 0);
}
/*
 * Power handler routine. Called when the system is transitioning
 * into/out of power save modes.  As with fxp_shutdown, the main
 * purpose of this routine is to shut off receiver DMA so it doesn't
 * clobber kernel memory at the wrong time.
 */
void
fxp_power(int why, void *arg)
{
	struct fxp_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	s = splnet();
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		fxp_stop(ifp, 0);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP)
			fxp_init(ifp);
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}

/*
 * Initialize the interface media.
 */
void
fxp_get_info(struct fxp_softc *sc, u_int8_t *enaddr)
{
	u_int16_t data, myea[ETHER_ADDR_LEN / 2];

	/*
	 * Reset to a stable state.
	 */
	CSR_WRITE_4(sc, FXP_CSR_PORT, FXP_PORT_SELECTIVE_RESET);
	DELAY(100);

	sc->sc_eeprom_size = 0;
	fxp_autosize_eeprom(sc);
	if (sc->sc_eeprom_size == 0) {
		aprint_error("%s: failed to detect EEPROM size\n",
		    sc->sc_dev.dv_xname);
		sc->sc_eeprom_size = 6; /* XXX panic here? */
	}
#ifdef DEBUG
	aprint_debug("%s: detected %d word EEPROM\n",
	    sc->sc_dev.dv_xname, 1 << sc->sc_eeprom_size);
#endif

	/*
	 * Get info about the primary PHY
	 */
	fxp_read_eeprom(sc, &data, 6, 1);
	sc->phy_primary_device =
	    (data & FXP_PHY_DEVICE_MASK) >> FXP_PHY_DEVICE_SHIFT;

	/*
	 * Read MAC address.
	 */
	fxp_read_eeprom(sc, myea, 0, 3);
	enaddr[0] = myea[0] & 0xff;
	enaddr[1] = myea[0] >> 8;
	enaddr[2] = myea[1] & 0xff;
	enaddr[3] = myea[1] >> 8;
	enaddr[4] = myea[2] & 0xff;
	enaddr[5] = myea[2] >> 8;

	/*
	 * Systems based on the ICH2/ICH2-M chip from Intel, as well
	 * as some i82559 designs, have a defect where the chip can
	 * cause a PCI protocol violation if it receives a CU_RESUME
	 * command when it is entering the IDLE state.
	 *
	 * The work-around is to disable Dynamic Standby Mode, so that
	 * the chip never deasserts #CLKRUN, and always remains in the
	 * active state.
	 *
	 * Unfortunately, the only way to disable Dynamic Standby is
	 * to frob an EEPROM setting and reboot (the EEPROM setting
	 * is only consulted when the PCI bus comes out of reset).
	 *
	 * See Intel 82801BA/82801BAM Specification Update, Errata #30.
	 */
	if (sc->sc_flags & FXPF_HAS_RESUME_BUG) {
		fxp_read_eeprom(sc, &data, 10, 1);
		if (data & 0x02) {		/* STB enable */
			aprint_error("%s: WARNING: "
			    "Disabling dynamic standby mode in EEPROM "
			    "to work around a\n",
			    sc->sc_dev.dv_xname);
			aprint_normal(
			    "%s: WARNING: hardware bug.  You must reset "
			    "the system before using this\n",
			    sc->sc_dev.dv_xname);
			aprint_normal("%s: WARNING: interface.\n",
			    sc->sc_dev.dv_xname);
			data &= ~0x02;
			fxp_write_eeprom(sc, &data, 10, 1);
			aprint_normal("%s: new EEPROM ID: 0x%04x\n",
			    sc->sc_dev.dv_xname, data);
			fxp_eeprom_update_cksum(sc);
		}
	}

	/* Receiver lock-up workaround detection. (FXPF_RECV_WORKAROUND) */
	/* Due to false positives we make it conditional on setting link1 */
	fxp_read_eeprom(sc, &data, 3, 1);
	if ((data & 0x03) != 0x03) {
		aprint_verbose("%s: May need receiver lock-up workaround\n",
		    sc->sc_dev.dv_xname);
	}
}

static void
fxp_eeprom_shiftin(struct fxp_softc *sc, int data, int len)
{
	uint16_t reg;
	int x;

	for (x = 1 << (len - 1); x != 0; x >>= 1) {
		DELAY(40);
		if (data & x)
			reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
		else
			reg = FXP_EEPROM_EECS;
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
		    reg | FXP_EEPROM_EESK);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
	}
	DELAY(40);
}

/*
 * Figure out EEPROM size.
 *
 * 559's can have either 64-word or 256-word EEPROMs, the 558
 * datasheet only talks about 64-word EEPROMs, and the 557 datasheet
 * talks about the existence of 16 to 256 word EEPROMs.
 *
 * The only known sizes are 64 and 256, where the 256 version is used
 * by CardBus cards to store CIS information.
 *
 * The address is shifted in msb-to-lsb, and after the last
 * address-bit the EEPROM is supposed to output a `dummy zero' bit,
 * after which follows the actual data. We try to detect this zero, by
 * probing the data-out bit in the EEPROM control register just after
 * having shifted in a bit. If the bit is zero, we assume we've
 * shifted enough address bits. The data-out should be tri-state,
 * before this, which should translate to a logical one.
 *
 * Other ways to do this would be to try to read a register with known
 * contents with a varying number of address bits, but no such
 * register seem to be available. The high bits of register 10 are 01
 * on the 558 and 559, but apparently not on the 557.
 *
 * The Linux driver computes a checksum on the EEPROM data, but the
 * value of this checksum is not very well documented.
 */

void
fxp_autosize_eeprom(struct fxp_softc *sc)
{
	int x;

	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
	DELAY(40);

	/* Shift in read opcode. */
	fxp_eeprom_shiftin(sc, FXP_EEPROM_OPC_READ, 3);

	/*
	 * Shift in address, wait for the dummy zero following a correct
	 * address shift.
	 */
	for (x = 1; x <= 8; x++) {
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
		    FXP_EEPROM_EECS | FXP_EEPROM_EESK);
		DELAY(40);
		if ((CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
		    FXP_EEPROM_EEDO) == 0)
			break;
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		DELAY(40);
	}
	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
	DELAY(40);
	if (x != 6 && x != 8) {
#ifdef DEBUG
		printf("%s: strange EEPROM size (%d)\n",
		    sc->sc_dev.dv_xname, 1 << x);
#endif
	} else
		sc->sc_eeprom_size = x;
}

/*
 * Read from the serial EEPROM. Basically, you manually shift in
 * the read opcode (one bit at a time) and then shift in the address,
 * and then you shift out the data (all of this one bit at a time).
 * The word size is 16 bits, so you have to provide the address for
 * every 16 bits of data.
 */
void
fxp_read_eeprom(struct fxp_softc *sc, u_int16_t *data, int offset, int words)
{
	u_int16_t reg;
	int i, x;

	for (i = 0; i < words; i++) {
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);

		/* Shift in read opcode. */
		fxp_eeprom_shiftin(sc, FXP_EEPROM_OPC_READ, 3);

		/* Shift in address. */
		fxp_eeprom_shiftin(sc, i + offset, sc->sc_eeprom_size);

		reg = FXP_EEPROM_EECS;
		data[i] = 0;

		/* Shift out data. */
		for (x = 16; x > 0; x--) {
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			DELAY(40);
			if (CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
			    FXP_EEPROM_EEDO)
				data[i] |= (1 << (x - 1));
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			DELAY(40);
		}
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(40);
	}
}

/*
 * Write data to the serial EEPROM.
 */
void
fxp_write_eeprom(struct fxp_softc *sc, u_int16_t *data, int offset, int words)
{
	int i, j;

	for (i = 0; i < words; i++) {
		/* Erase/write enable. */
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		fxp_eeprom_shiftin(sc, FXP_EEPROM_OPC_ERASE, 3);
		fxp_eeprom_shiftin(sc, 0x3 << (sc->sc_eeprom_size - 2),
		    sc->sc_eeprom_size);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(4);

		/* Shift in write opcode, address, data. */
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		fxp_eeprom_shiftin(sc, FXP_EEPROM_OPC_WRITE, 3);
		fxp_eeprom_shiftin(sc, offset, sc->sc_eeprom_size);
		fxp_eeprom_shiftin(sc, data[i], 16);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(4);

		/* Wait for the EEPROM to finish up. */
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		DELAY(4);
		for (j = 0; j < 1000; j++) {
			if (CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
			    FXP_EEPROM_EEDO)
				break;
			DELAY(50);
		}
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(4);

		/* Erase/write disable. */
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		fxp_eeprom_shiftin(sc, FXP_EEPROM_OPC_ERASE, 3);
		fxp_eeprom_shiftin(sc, 0, sc->sc_eeprom_size);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(4);
	}
}

/*
 * Update the checksum of the EEPROM.
 */
void
fxp_eeprom_update_cksum(struct fxp_softc *sc)
{
	int i;
	uint16_t data, cksum;

	cksum = 0;
	for (i = 0; i < (1 << sc->sc_eeprom_size) - 1; i++) {
		fxp_read_eeprom(sc, &data, i, 1);
		cksum += data;
	}
	i = (1 << sc->sc_eeprom_size) - 1;
	cksum = 0xbaba - cksum;
	fxp_read_eeprom(sc, &data, i, 1);
	fxp_write_eeprom(sc, &cksum, i, 1);
	log(LOG_INFO, "%s: EEPROM checksum @ 0x%x: 0x%04x -> 0x%04x\n",
	    sc->sc_dev.dv_xname, i, data, cksum);
}

/*
 * Start packet transmission on the interface.
 */
void
fxp_start(struct ifnet *ifp)
{
	struct fxp_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct fxp_txdesc *txd;
	struct fxp_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, lasttx, nexttx, opending, seg;

	/*
	 * If we want a re-init, bail out now.
	 */
	if (sc->sc_flags & FXPF_WANTINIT) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous txpending and the current lasttx.
	 */
	opending = sc->sc_txpending;
	lasttx = sc->sc_txlast;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		struct fxp_tbd *tbdp;
		int csum_flags;

		/*
		 * Grab a packet off the queue.
		 */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		m = NULL;

		if (sc->sc_txpending == FXP_NTXCB) {
			FXP_EVCNT_INCR(&sc->sc_ev_txstall);
			break;
		}

		/*
		 * Get the next available transmit descriptor.
		 */
		nexttx = FXP_NEXTTX(sc->sc_txlast);
		txd = FXP_CDTX(sc, nexttx);
		txs = FXP_DSTX(sc, nexttx);
		dmamap = txs->txs_dmamap;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the allotted number of frags, or we were
		 * short on resources.  In this case, we'll copy and try
		 * again.
		 */
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				log(LOG_ERR, "%s: unable to allocate Tx mbuf\n",
				    sc->sc_dev.dv_xname);
				break;
			}
			MCLAIM(m, &sc->sc_ethercom.ec_tx_mowner);
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					log(LOG_ERR,
					    "%s: unable to allocate Tx "
					    "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m);
					break;
				}
			}
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, caddr_t));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				log(LOG_ERR, "%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				break;
			}
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		csum_flags = m0->m_pkthdr.csum_flags;
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}

		/* Initialize the fraglist. */
		tbdp = txd->txd_tbd;
		if (sc->sc_flags & FXPF_IPCB)
			tbdp++;
		for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
			tbdp[seg].tb_addr =
			    htole32(dmamap->dm_segs[seg].ds_addr);
			tbdp[seg].tb_size =
			    htole32(dmamap->dm_segs[seg].ds_len);
		}

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Store a pointer to the packet so we can free it later.
		 */
		txs->txs_mbuf = m0;

		/*
		 * Initialize the transmit descriptor.
		 */
		/* BIG_ENDIAN: no need to swap to store 0 */
		txd->txd_txcb.cb_status = 0;
		txd->txd_txcb.cb_command =
		    sc->sc_txcmd | htole16(FXP_CB_COMMAND_SF);
		txd->txd_txcb.tx_threshold = tx_threshold;
		txd->txd_txcb.tbd_number = dmamap->dm_nsegs;

		KASSERT((csum_flags & (M_CSUM_TCPv6 | M_CSUM_UDPv6)) == 0);
		if (sc->sc_flags & FXPF_IPCB) {
			struct m_tag *vtag;
			struct fxp_ipcb *ipcb;
			/*
			 * Deal with TCP/IP checksum offload. Note that
			 * in order for TCP checksum offload to work,
			 * the pseudo header checksum must have already
			 * been computed and stored in the checksum field
			 * in the TCP header. The stack should have
			 * already done this for us.
			 */
			ipcb = &txd->txd_u.txdu_ipcb;
			memset(ipcb, 0, sizeof(*ipcb));
			/*
			 * always do hardware parsing.
			 */
			ipcb->ipcb_ip_activation_high =
			    FXP_IPCB_HARDWAREPARSING_ENABLE;
			/*
			 * ip checksum offloading.
			 */
			if (csum_flags & M_CSUM_IPv4) {
				ipcb->ipcb_ip_schedule |=
				    FXP_IPCB_IP_CHECKSUM_ENABLE;
			}
			/*
			 * TCP/UDP checksum offloading.
			 */
			if (csum_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
				ipcb->ipcb_ip_schedule |=
				    FXP_IPCB_TCPUDP_CHECKSUM_ENABLE;
			}

			/*
			 * request VLAN tag insertion if needed.
			 */
			vtag = VLAN_OUTPUT_TAG(&sc->sc_ethercom, m0);
			if (vtag) {
				ipcb->ipcb_vlan_id =
				    htobe16(*(u_int *)(vtag + 1));
				ipcb->ipcb_ip_activation_high |=
				    FXP_IPCB_INSERTVLAN_ENABLE;
			}
		} else {
			KASSERT((csum_flags &
			    (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) == 0);
		}

		FXP_CDTXSYNC(sc, nexttx,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Advance the tx pointer. */
		sc->sc_txpending++;
		sc->sc_txlast = nexttx;

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif
	}

	if (sc->sc_txpending == FXP_NTXCB) {
		/* No more slots; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txpending != opending) {
		/*
		 * We enqueued packets.  If the transmitter was idle,
		 * reset the txdirty pointer.
		 */
		if (opending == 0)
			sc->sc_txdirty = FXP_NEXTTX(lasttx);

		/*
		 * Cause the chip to interrupt and suspend command
		 * processing once the last packet we've enqueued
		 * has been transmitted.
		 */
		FXP_CDTX(sc, sc->sc_txlast)->txd_txcb.cb_command |=
		    htole16(FXP_CB_COMMAND_I | FXP_CB_COMMAND_S);
		FXP_CDTXSYNC(sc, sc->sc_txlast,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * The entire packet chain is set up.  Clear the suspend bit
		 * on the command prior to the first packet we set up.
		 */
		FXP_CDTXSYNC(sc, lasttx,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		FXP_CDTX(sc, lasttx)->txd_txcb.cb_command &=
		    htole16(~FXP_CB_COMMAND_S);
		FXP_CDTXSYNC(sc, lasttx,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Issue a Resume command in case the chip was suspended.
		 */
		fxp_scb_wait(sc);
		fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_RESUME);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

/*
 * Process interface interrupts.
 */
int
fxp_intr(void *arg)
{
	struct fxp_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_dmamap_t rxmap;
	int claimed = 0;
	u_int8_t statack;

	if (!device_is_active(&sc->sc_dev) || sc->sc_enabled == 0)
		return (0);
	/*
	 * If the interface isn't running, don't try to
	 * service the interrupt.. just ack it and bail.
	 */
	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		statack = CSR_READ_1(sc, FXP_CSR_SCB_STATACK);
		if (statack) {
			claimed = 1;
			CSR_WRITE_1(sc, FXP_CSR_SCB_STATACK, statack);
		}
		return (claimed);
	}

	while ((statack = CSR_READ_1(sc, FXP_CSR_SCB_STATACK)) != 0) {
		claimed = 1;

		/*
		 * First ACK all the interrupts in this pass.
		 */
		CSR_WRITE_1(sc, FXP_CSR_SCB_STATACK, statack);

		/*
		 * Process receiver interrupts. If a no-resource (RNR)
		 * condition exists, get whatever packets we can and
		 * re-start the receiver.
		 */
		if (statack & (FXP_SCB_STATACK_FR | FXP_SCB_STATACK_RNR)) {
			FXP_EVCNT_INCR(&sc->sc_ev_rxintr);
			fxp_rxintr(sc);
		}

		if (statack & FXP_SCB_STATACK_RNR) {
			fxp_scb_wait(sc);
			fxp_scb_cmd(sc, FXP_SCB_COMMAND_RU_ABORT);
			rxmap = M_GETCTX(sc->sc_rxq.ifq_head, bus_dmamap_t);
			fxp_scb_wait(sc);
			CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
			    rxmap->dm_segs[0].ds_addr +
			    RFA_ALIGNMENT_FUDGE);
			fxp_scb_cmd(sc, FXP_SCB_COMMAND_RU_START);
		}

		/*
		 * Free any finished transmit mbuf chains.
		 */
		if (statack & (FXP_SCB_STATACK_CXTNO|FXP_SCB_STATACK_CNA)) {
			FXP_EVCNT_INCR(&sc->sc_ev_txintr);
			fxp_txintr(sc);

			/*
			 * Try to get more packets going.
			 */
			fxp_start(ifp);

			if (sc->sc_txpending == 0) {
				/*
				 * If we want a re-init, do that now.
				 */
				if (sc->sc_flags & FXPF_WANTINIT)
					(void) fxp_init(ifp);
			}
		}
	}

#if NRND > 0
	if (claimed)
		rnd_add_uint32(&sc->rnd_source, statack);
#endif
	return (claimed);
}

/*
 * Handle transmit completion interrupts.
 */
void
fxp_txintr(struct fxp_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct fxp_txdesc *txd;
	struct fxp_txsoft *txs;
	int i;
	u_int16_t txstat;

	ifp->if_flags &= ~IFF_OACTIVE;
	for (i = sc->sc_txdirty; sc->sc_txpending != 0;
	    i = FXP_NEXTTX(i), sc->sc_txpending--) {
		txd = FXP_CDTX(sc, i);
		txs = FXP_DSTX(sc, i);

		FXP_CDTXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		txstat = le16toh(txd->txd_txcb.cb_status);

		if ((txstat & FXP_CB_STATUS_C) == 0)
			break;

		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}

	/* Update the dirty transmit buffer pointer. */
	sc->sc_txdirty = i;

	/*
	 * Cancel the watchdog timer if there are no pending
	 * transmissions.
	 */
	if (sc->sc_txpending == 0)
		ifp->if_timer = 0;
}

/*
 * fxp_rx_hwcksum: check status of H/W offloading for received packets.
 */

int
fxp_rx_hwcksum(struct mbuf *m, const struct fxp_rfa *rfa)
{
	u_int16_t rxparsestat;
	u_int16_t csum_stat;
	u_int32_t csum_data;
	int csum_flags;

	/*
	 * check VLAN tag stripping.
	 */

	if (rfa->rfa_status & htole16(FXP_RFA_STATUS_VLAN)) {
		struct m_tag *vtag;

		vtag = m_tag_get(PACKET_TAG_VLAN, sizeof(u_int), M_NOWAIT);
		if (vtag == NULL)
			return ENOMEM;
		*(u_int *)(vtag + 1) = be16toh(rfa->vlan_id);
		m_tag_prepend(m, vtag);
	}

	/*
	 * check H/W Checksumming.
	 */

	csum_stat = le16toh(rfa->cksum_stat);
	rxparsestat = le16toh(rfa->rx_parse_stat);
	if (!(rfa->rfa_status & htole16(FXP_RFA_STATUS_PARSE)))
		return 0;

	csum_flags = 0;
	csum_data = 0;

	if (csum_stat & FXP_RFDX_CS_IP_CSUM_BIT_VALID) {
		csum_flags = M_CSUM_IPv4;
		if (!(csum_stat & FXP_RFDX_CS_IP_CSUM_VALID))
			csum_flags |= M_CSUM_IPv4_BAD;
	}

	if (csum_stat & FXP_RFDX_CS_TCPUDP_CSUM_BIT_VALID) {
		csum_flags |= (M_CSUM_TCPv4|M_CSUM_UDPv4); /* XXX */
		if (!(csum_stat & FXP_RFDX_CS_TCPUDP_CSUM_VALID))
			csum_flags |= M_CSUM_TCP_UDP_BAD;
	}

	m->m_pkthdr.csum_flags = csum_flags;
	m->m_pkthdr.csum_data = csum_data;

	return 0;
}

/*
 * Handle receive interrupts.
 */
void
fxp_rxintr(struct fxp_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m, *m0;
	bus_dmamap_t rxmap;
	struct fxp_rfa *rfa;
	u_int16_t len, rxstat;

	for (;;) {
		m = sc->sc_rxq.ifq_head;
		rfa = FXP_MTORFA(m);
		rxmap = M_GETCTX(m, bus_dmamap_t);

		FXP_RFASYNC(sc, m,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = le16toh(rfa->rfa_status);

		if ((rxstat & FXP_RFA_STATUS_C) == 0) {
			/*
			 * We have processed all of the
			 * receive buffers.
			 */
			FXP_RFASYNC(sc, m, BUS_DMASYNC_PREREAD);
			return;
		}

		IF_DEQUEUE(&sc->sc_rxq, m);

		FXP_RXBUFSYNC(sc, m, BUS_DMASYNC_POSTREAD);

		len = le16toh(rfa->actual_size) &
		    (m->m_ext.ext_size - 1);

		if (len < sizeof(struct ether_header)) {
			/*
			 * Runt packet; drop it now.
			 */
			FXP_INIT_RFABUF(sc, m);
			continue;
		}

		/*
		 * If support for 802.1Q VLAN sized frames is
		 * enabled, we need to do some additional error
		 * checking (as we are saving bad frames, in
		 * order to receive the larger ones).
		 */
		if ((ec->ec_capenable & ETHERCAP_VLAN_MTU) != 0 &&
		    (rxstat & (FXP_RFA_STATUS_OVERRUN|
			       FXP_RFA_STATUS_RNR|
			       FXP_RFA_STATUS_ALIGN|
			       FXP_RFA_STATUS_CRC)) != 0) {
			FXP_INIT_RFABUF(sc, m);
			continue;
		}

		/* Do checksum checking. */
		m->m_pkthdr.csum_flags = 0;
		if (sc->sc_flags & FXPF_EXT_RFA)
			if (fxp_rx_hwcksum(m, rfa))
				goto dropit;

		/*
		 * If the packet is small enough to fit in a
		 * single header mbuf, allocate one and copy
		 * the data into it.  This greatly reduces
		 * memory consumption when we receive lots
		 * of small packets.
		 *
		 * Otherwise, we add a new buffer to the receive
		 * chain.  If this fails, we drop the packet and
		 * recycle the old buffer.
		 */
		if (fxp_copy_small != 0 && len <= MHLEN) {
			MGETHDR(m0, M_DONTWAIT, MT_DATA);
			if (m0 == NULL)
				goto dropit;
			MCLAIM(m0, &sc->sc_ethercom.ec_rx_mowner);
			memcpy(mtod(m0, caddr_t),
			    mtod(m, caddr_t), len);
			m0->m_pkthdr.csum_flags = m->m_pkthdr.csum_flags;
			m0->m_pkthdr.csum_data = m->m_pkthdr.csum_data;
			FXP_INIT_RFABUF(sc, m);
			m = m0;
		} else {
			if (fxp_add_rfabuf(sc, rxmap, 1) != 0) {
 dropit:
				ifp->if_ierrors++;
				FXP_INIT_RFABUF(sc, m);
				continue;
			}
		}

		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass it up the stack it its for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}
}

/*
 * Update packet in/out/collision statistics. The i82557 doesn't
 * allow you to access these counters without doing a fairly
 * expensive DMA to get _all_ of the statistics it maintains, so
 * we do this operation here only once per second. The statistics
 * counters in the kernel are updated from the previous dump-stats
 * DMA and then a new dump-stats DMA is started. The on-chip
 * counters are zeroed when the DMA completes. If we can't start
 * the DMA immediately, we don't wait - we just prepare to read
 * them again next time.
 */
void
fxp_tick(void *arg)
{
	struct fxp_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct fxp_stats *sp = &sc->sc_control_data->fcd_stats;
	int s;

	if (!device_is_active(&sc->sc_dev))
		return;

	s = splnet();

	FXP_CDSTATSSYNC(sc, BUS_DMASYNC_POSTREAD);

	ifp->if_opackets += le32toh(sp->tx_good);
	ifp->if_collisions += le32toh(sp->tx_total_collisions);
	if (sp->rx_good) {
		ifp->if_ipackets += le32toh(sp->rx_good);
		sc->sc_rxidle = 0;
	} else if (sc->sc_flags & FXPF_RECV_WORKAROUND) {
		sc->sc_rxidle++;
	}
	ifp->if_ierrors +=
	    le32toh(sp->rx_crc_errors) +
	    le32toh(sp->rx_alignment_errors) +
	    le32toh(sp->rx_rnr_errors) +
	    le32toh(sp->rx_overrun_errors);
	/*
	 * If any transmit underruns occurred, bump up the transmit
	 * threshold by another 512 bytes (64 * 8).
	 */
	if (sp->tx_underruns) {
		ifp->if_oerrors += le32toh(sp->tx_underruns);
		if (tx_threshold < 192)
			tx_threshold += 64;
	}
#ifdef FXP_EVENT_COUNTERS
	if (sc->sc_rev >= FXP_REV_82558_A4) {
		sc->sc_ev_txpause.ev_count += sp->tx_pauseframes;
		sc->sc_ev_rxpause.ev_count += sp->rx_pauseframes;
	}
#endif

	/*
	 * If we haven't received any packets in FXP_MAX_RX_IDLE seconds,
	 * then assume the receiver has locked up and attempt to clear
	 * the condition by reprogramming the multicast filter (actually,
	 * resetting the interface). This is a work-around for a bug in
	 * the 82557 where the receiver locks up if it gets certain types
	 * of garbage in the synchronization bits prior to the packet header.
	 * This bug is supposed to only occur in 10Mbps mode, but has been
	 * seen to occur in 100Mbps mode as well (perhaps due to a 10/100
	 * speed transition).
	 */
	if (sc->sc_rxidle > FXP_MAX_RX_IDLE) {
		(void) fxp_init(ifp);
		splx(s);
		return;
	}
	/*
	 * If there is no pending command, start another stats
	 * dump. Otherwise punt for now.
	 */
	if (CSR_READ_1(sc, FXP_CSR_SCB_COMMAND) == 0) {
		/*
		 * Start another stats dump.
		 */
		FXP_CDSTATSSYNC(sc, BUS_DMASYNC_PREREAD);
		fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_DUMPRESET);
	} else {
		/*
		 * A previous command is still waiting to be accepted.
		 * Just zero our copy of the stats and wait for the
		 * next timer event to update them.
		 */
		/* BIG_ENDIAN: no swap required to store 0 */
		sp->tx_good = 0;
		sp->tx_underruns = 0;
		sp->tx_total_collisions = 0;

		sp->rx_good = 0;
		sp->rx_crc_errors = 0;
		sp->rx_alignment_errors = 0;
		sp->rx_rnr_errors = 0;
		sp->rx_overrun_errors = 0;
		if (sc->sc_rev >= FXP_REV_82558_A4) {
			sp->tx_pauseframes = 0;
			sp->rx_pauseframes = 0;
		}
	}

	if (sc->sc_flags & FXPF_MII) {
		/* Tick the MII clock. */
		mii_tick(&sc->sc_mii);
	}

	splx(s);

	/*
	 * Schedule another timeout one second from now.
	 */
	callout_reset(&sc->sc_callout, hz, fxp_tick, sc);
}

/*
 * Drain the receive queue.
 */
void
fxp_rxdrain(struct fxp_softc *sc)
{
	bus_dmamap_t rxmap;
	struct mbuf *m;

	for (;;) {
		IF_DEQUEUE(&sc->sc_rxq, m);
		if (m == NULL)
			break;
		rxmap = M_GETCTX(m, bus_dmamap_t);
		bus_dmamap_unload(sc->sc_dmat, rxmap);
		FXP_RXMAP_PUT(sc, rxmap);
		m_freem(m);
	}
}

/*
 * Stop the interface. Cancels the statistics updater and resets
 * the interface.
 */
void
fxp_stop(struct ifnet *ifp, int disable)
{
	struct fxp_softc *sc = ifp->if_softc;
	struct fxp_txsoft *txs;
	int i;

	/*
	 * Turn down interface (done early to avoid bad interactions
	 * between panics, shutdown hooks, and the watchdog timer)
	 */
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	/*
	 * Cancel stats updater.
	 */
	callout_stop(&sc->sc_callout);
	if (sc->sc_flags & FXPF_MII) {
		/* Down the MII. */
		mii_down(&sc->sc_mii);
	}

	/*
	 * Issue software reset.  This unloads any microcode that
	 * might already be loaded.
	 */
	sc->sc_flags &= ~FXPF_UCODE_LOADED;
	CSR_WRITE_4(sc, FXP_CSR_PORT, FXP_PORT_SOFTWARE_RESET);
	DELAY(50);

	/*
	 * Release any xmit buffers.
	 */
	for (i = 0; i < FXP_NTXCB; i++) {
		txs = FXP_DSTX(sc, i);
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}
	sc->sc_txpending = 0;

	if (disable) {
		fxp_rxdrain(sc);
		fxp_disable(sc);
	}

}

/*
 * Watchdog/transmission transmit timeout handler. Called when a
 * transmission is started on the interface, but no interrupt is
 * received before the timeout. This usually indicates that the
 * card has wedged for some reason.
 */
void
fxp_watchdog(struct ifnet *ifp)
{
	struct fxp_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;

	(void) fxp_init(ifp);
}

/*
 * Initialize the interface.  Must be called at splnet().
 */
int
fxp_init(struct ifnet *ifp)
{
	struct fxp_softc *sc = ifp->if_softc;
	struct fxp_cb_config *cbp;
	struct fxp_cb_ias *cb_ias;
	struct fxp_txdesc *txd;
	bus_dmamap_t rxmap;
	int i, prm, save_bf, lrxen, vlan_drop, allm, error = 0;

	if ((error = fxp_enable(sc)) != 0)
		goto out;

	/*
	 * Cancel any pending I/O
	 */
	fxp_stop(ifp, 0);

	/*
	 * XXX just setting sc_flags to 0 here clears any FXPF_MII
	 * flag, and this prevents the MII from detaching resulting in
	 * a panic. The flags field should perhaps be split in runtime
	 * flags and more static information. For now, just clear the
	 * only other flag set.
	 */

	sc->sc_flags &= ~FXPF_WANTINIT;

	/*
	 * Initialize base of CBL and RFA memory. Loading with zero
	 * sets it up for regular linear addressing.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, 0);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_BASE);

	fxp_scb_wait(sc);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_RU_BASE);

	/*
	 * Initialize the multicast filter.  Do this now, since we might
	 * have to setup the config block differently.
	 */
	fxp_mc_setup(sc);

	prm = (ifp->if_flags & IFF_PROMISC) ? 1 : 0;
	allm = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;

	/*
	 * In order to support receiving 802.1Q VLAN frames, we have to
	 * enable "save bad frames", since they are 4 bytes larger than
	 * the normal Ethernet maximum frame length.  On i82558 and later,
	 * we have a better mechanism for this.
	 */
	save_bf = 0;
	lrxen = 0;
	vlan_drop = 0;
	if (sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_MTU) {
		if (sc->sc_rev < FXP_REV_82558_A4)
			save_bf = 1;
		else
			lrxen = 1;
		if (sc->sc_rev >= FXP_REV_82550)
			vlan_drop = 1;
	}

	/*
	 * Initialize base of dump-stats buffer.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    sc->sc_cddma + FXP_CDSTATSOFF);
	FXP_CDSTATSSYNC(sc, BUS_DMASYNC_PREREAD);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_DUMP_ADR);

	cbp = &sc->sc_control_data->fcd_configcb;
	memset(cbp, 0, sizeof(struct fxp_cb_config));

	/*
	 * Load microcode for this controller.
	 */
	fxp_load_ucode(sc);

	if ((sc->sc_ethercom.ec_if.if_flags & IFF_LINK1))
		sc->sc_flags |= FXPF_RECV_WORKAROUND;
	else
		sc->sc_flags &= ~FXPF_RECV_WORKAROUND;

	/*
	 * This copy is kind of disgusting, but there are a bunch of must be
	 * zero and must be one bits in this structure and this is the easiest
	 * way to initialize them all to proper values.
	 */
	memcpy(cbp, fxp_cb_config_template, sizeof(fxp_cb_config_template));

	/* BIG_ENDIAN: no need to swap to store 0 */
	cbp->cb_status =	0;
	cbp->cb_command =	htole16(FXP_CB_COMMAND_CONFIG |
				    FXP_CB_COMMAND_EL);
	/* BIG_ENDIAN: no need to swap to store 0xffffffff */
	cbp->link_addr =	0xffffffff; /* (no) next command */
					/* bytes in config block */
	cbp->byte_count =	(sc->sc_flags & FXPF_EXT_RFA) ?
				FXP_EXT_CONFIG_LEN : FXP_CONFIG_LEN;
	cbp->rx_fifo_limit =	8;	/* rx fifo threshold (32 bytes) */
	cbp->tx_fifo_limit =	0;	/* tx fifo threshold (0 bytes) */
	cbp->adaptive_ifs =	0;	/* (no) adaptive interframe spacing */
	cbp->mwi_enable =	(sc->sc_flags & FXPF_MWI) ? 1 : 0;
	cbp->type_enable =	0;	/* actually reserved */
	cbp->read_align_en =	(sc->sc_flags & FXPF_READ_ALIGN) ? 1 : 0;
	cbp->end_wr_on_cl =	(sc->sc_flags & FXPF_WRITE_ALIGN) ? 1 : 0;
	cbp->rx_dma_bytecount =	0;	/* (no) rx DMA max */
	cbp->tx_dma_bytecount =	0;	/* (no) tx DMA max */
	cbp->dma_mbce =		0;	/* (disable) dma max counters */
	cbp->late_scb =		0;	/* (don't) defer SCB update */
	cbp->tno_int_or_tco_en =0;	/* (disable) tx not okay interrupt */
	cbp->ci_int =		1;	/* interrupt on CU idle */
	cbp->ext_txcb_dis =	(sc->sc_flags & FXPF_EXT_TXCB) ? 0 : 1;
	cbp->ext_stats_dis =	1;	/* disable extended counters */
	cbp->keep_overrun_rx =	0;	/* don't pass overrun frames to host */
	cbp->save_bf =		save_bf;/* save bad frames */
	cbp->disc_short_rx =	!prm;	/* discard short packets */
	cbp->underrun_retry =	1;	/* retry mode (1) on DMA underrun */
	cbp->ext_rfa =		(sc->sc_flags & FXPF_EXT_RFA) ? 1 : 0;
	cbp->two_frames =	0;	/* do not limit FIFO to 2 frames */
	cbp->dyn_tbd =		0;	/* (no) dynamic TBD mode */
					/* interface mode */
	cbp->mediatype =	(sc->sc_flags & FXPF_MII) ? 1 : 0;
	cbp->csma_dis =		0;	/* (don't) disable link */
	cbp->tcp_udp_cksum =	0;	/* (don't) enable checksum */
	cbp->vlan_tco =		0;	/* (don't) enable vlan wakeup */
	cbp->link_wake_en =	0;	/* (don't) assert PME# on link change */
	cbp->arp_wake_en =	0;	/* (don't) assert PME# on arp */
	cbp->mc_wake_en =	0;	/* (don't) assert PME# on mcmatch */
	cbp->nsai =		1;	/* (don't) disable source addr insert */
	cbp->preamble_length =	2;	/* (7 byte) preamble */
	cbp->loopback =		0;	/* (don't) loopback */
	cbp->linear_priority =	0;	/* (normal CSMA/CD operation) */
	cbp->linear_pri_mode =	0;	/* (wait after xmit only) */
	cbp->interfrm_spacing =	6;	/* (96 bits of) interframe spacing */
	cbp->promiscuous =	prm;	/* promiscuous mode */
	cbp->bcast_disable =	0;	/* (don't) disable broadcasts */
	cbp->wait_after_win =	0;	/* (don't) enable modified backoff alg*/
	cbp->ignore_ul =	0;	/* consider U/L bit in IA matching */
	cbp->crc16_en =		0;	/* (don't) enable crc-16 algorithm */
	cbp->crscdt =		(sc->sc_flags & FXPF_MII) ? 0 : 1;
	cbp->stripping =	!prm;	/* truncate rx packet to byte count */
	cbp->padding =		1;	/* (do) pad short tx packets */
	cbp->rcv_crc_xfer =	0;	/* (don't) xfer CRC to host */
	cbp->long_rx_en =	lrxen;	/* long packet receive enable */
	cbp->ia_wake_en =	0;	/* (don't) wake up on address match */
	cbp->magic_pkt_dis =	0;	/* (don't) disable magic packet */
					/* must set wake_en in PMCSR also */
	cbp->force_fdx =	0;	/* (don't) force full duplex */
	cbp->fdx_pin_en =	1;	/* (enable) FDX# pin */
	cbp->multi_ia =		0;	/* (don't) accept multiple IAs */
	cbp->mc_all =		allm;	/* accept all multicasts */
	cbp->ext_rx_mode =	(sc->sc_flags & FXPF_EXT_RFA) ? 1 : 0;
	cbp->vlan_drop_en =	vlan_drop;

	if (sc->sc_rev < FXP_REV_82558_A4) {
		/*
		 * The i82557 has no hardware flow control, the values
		 * here are the defaults for the chip.
		 */
		cbp->fc_delay_lsb =	0;
		cbp->fc_delay_msb =	0x40;
		cbp->pri_fc_thresh =	3;
		cbp->tx_fc_dis =	0;
		cbp->rx_fc_restop =	0;
		cbp->rx_fc_restart =	0;
		cbp->fc_filter =	0;
		cbp->pri_fc_loc =	1;
	} else {
		cbp->fc_delay_lsb =	0x1f;
		cbp->fc_delay_msb =	0x01;
		cbp->pri_fc_thresh =	3;
		cbp->tx_fc_dis =	0;	/* enable transmit FC */
		cbp->rx_fc_restop =	1;	/* enable FC restop frames */
		cbp->rx_fc_restart =	1;	/* enable FC restart frames */
		cbp->fc_filter =	!prm;	/* drop FC frames to host */
		cbp->pri_fc_loc =	1;	/* FC pri location (byte31) */
		cbp->ext_stats_dis =	0;	/* enable extended stats */
	}

	FXP_CDCONFIGSYNC(sc, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Start the config command/DMA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, sc->sc_cddma + FXP_CDCONFIGOFF);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_START);
	/* ...and wait for it to complete. */
	i = 1000;
	do {
		FXP_CDCONFIGSYNC(sc,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DELAY(1);
	} while ((le16toh(cbp->cb_status) & FXP_CB_STATUS_C) == 0 && --i);
	if (i == 0) {
		log(LOG_WARNING, "%s: line %d: dmasync timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return (ETIMEDOUT);
	}

	/*
	 * Initialize the station address.
	 */
	cb_ias = &sc->sc_control_data->fcd_iascb;
	/* BIG_ENDIAN: no need to swap to store 0 */
	cb_ias->cb_status = 0;
	cb_ias->cb_command = htole16(FXP_CB_COMMAND_IAS | FXP_CB_COMMAND_EL);
	/* BIG_ENDIAN: no need to swap to store 0xffffffff */
	cb_ias->link_addr = 0xffffffff;
	memcpy(cb_ias->macaddr, LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);

	FXP_CDIASSYNC(sc, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Start the IAS (Individual Address Setup) command/DMA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, sc->sc_cddma + FXP_CDIASOFF);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_START);
	/* ...and wait for it to complete. */
	i = 1000;
	do {
		FXP_CDIASSYNC(sc,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DELAY(1);
	} while ((le16toh(cb_ias->cb_status) & FXP_CB_STATUS_C) == 0 && --i);
	if (i == 0) {
		log(LOG_WARNING, "%s: line %d: dmasync timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return (ETIMEDOUT);
	}

	/*
	 * Initialize the transmit descriptor ring.  txlast is initialized
	 * to the end of the list so that it will wrap around to the first
	 * descriptor when the first packet is transmitted.
	 */
	for (i = 0; i < FXP_NTXCB; i++) {
		txd = FXP_CDTX(sc, i);
		memset(txd, 0, sizeof(*txd));
		txd->txd_txcb.cb_command =
		    htole16(FXP_CB_COMMAND_NOP | FXP_CB_COMMAND_S);
		txd->txd_txcb.link_addr =
		    htole32(FXP_CDTXADDR(sc, FXP_NEXTTX(i)));
		if (sc->sc_flags & FXPF_EXT_TXCB)
			txd->txd_txcb.tbd_array_addr =
			    htole32(FXP_CDTBDADDR(sc, i) +
				    (2 * sizeof(struct fxp_tbd)));
		else
			txd->txd_txcb.tbd_array_addr =
			    htole32(FXP_CDTBDADDR(sc, i));
		FXP_CDTXSYNC(sc, i, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
	sc->sc_txpending = 0;
	sc->sc_txdirty = 0;
	sc->sc_txlast = FXP_NTXCB - 1;

	/*
	 * Initialize the receive buffer list.
	 */
	sc->sc_rxq.ifq_maxlen = FXP_NRFABUFS;
	while (sc->sc_rxq.ifq_len < FXP_NRFABUFS) {
		rxmap = FXP_RXMAP_GET(sc);
		if ((error = fxp_add_rfabuf(sc, rxmap, 0)) != 0) {
			log(LOG_ERR, "%s: unable to allocate or map rx "
			    "buffer %d, error = %d\n",
			    sc->sc_dev.dv_xname,
			    sc->sc_rxq.ifq_len, error);
			/*
			 * XXX Should attempt to run with fewer receive
			 * XXX buffers instead of just failing.
			 */
			FXP_RXMAP_PUT(sc, rxmap);
			fxp_rxdrain(sc);
			goto out;
		}
	}
	sc->sc_rxidle = 0;

	/*
	 * Give the transmit ring to the chip.  We do this by pointing
	 * the chip at the last descriptor (which is a NOP|SUSPEND), and
	 * issuing a start command.  It will execute the NOP and then
	 * suspend, pointing at the first descriptor.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, FXP_CDTXADDR(sc, sc->sc_txlast));
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_START);

	/*
	 * Initialize receiver buffer area - RFA.
	 */
	rxmap = M_GETCTX(sc->sc_rxq.ifq_head, bus_dmamap_t);
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    rxmap->dm_segs[0].ds_addr + RFA_ALIGNMENT_FUDGE);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_RU_START);

	if (sc->sc_flags & FXPF_MII) {
		/*
		 * Set current media.
		 */
		mii_mediachg(&sc->sc_mii);
	}

	/*
	 * ...all done!
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * Start the one second timer.
	 */
	callout_reset(&sc->sc_callout, hz, fxp_tick, sc);

	/*
	 * Attempt to start output on the interface.
	 */
	fxp_start(ifp);

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		log(LOG_ERR, "%s: interface not running\n",
		    sc->sc_dev.dv_xname);
	}
	return (error);
}

/*
 * Change media according to request.
 */
int
fxp_mii_mediachange(struct ifnet *ifp)
{
	struct fxp_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return (0);
}

/*
 * Notify the world which media we're using.
 */
void
fxp_mii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct fxp_softc *sc = ifp->if_softc;

	if (sc->sc_enabled == 0) {
		ifmr->ifm_active = IFM_ETHER | IFM_NONE;
		ifmr->ifm_status = 0;
		return;
	}

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;

	/*
	 * XXX Flow control is always turned on if the chip supports
	 * XXX it; we can't easily control it dynamically, since it
	 * XXX requires sending a setup packet.
	 */
	if (sc->sc_rev >= FXP_REV_82558_A4)
		ifmr->ifm_active |= IFM_FLOW|IFM_ETH_TXPAUSE|IFM_ETH_RXPAUSE;
}

int
fxp_80c24_mediachange(struct ifnet *ifp)
{

	/* Nothing to do here. */
	return (0);
}

void
fxp_80c24_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct fxp_softc *sc = ifp->if_softc;

	/*
	 * Media is currently-selected media.  We cannot determine
	 * the link status.
	 */
	ifmr->ifm_status = 0;
	ifmr->ifm_active = sc->sc_mii.mii_media.ifm_cur->ifm_media;
}

/*
 * Add a buffer to the end of the RFA buffer list.
 * Return 0 if successful, error code on failure.
 *
 * The RFA struct is stuck at the beginning of mbuf cluster and the
 * data pointer is fixed up to point just past it.
 */
int
fxp_add_rfabuf(struct fxp_softc *sc, bus_dmamap_t rxmap, int unload)
{
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (unload)
		bus_dmamap_unload(sc->sc_dmat, rxmap);

	M_SETCTX(m, rxmap);

	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, rxmap, m,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		/* XXX XXX XXX */
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, sc->sc_rxq.ifq_len, error);
		panic("fxp_add_rfabuf");
	}

	FXP_INIT_RFABUF(sc, m);

	return (0);
}

int
fxp_mdi_read(struct device *self, int phy, int reg)
{
	struct fxp_softc *sc = (struct fxp_softc *)self;
	int count = 10000;
	int value;

	CSR_WRITE_4(sc, FXP_CSR_MDICONTROL,
	    (FXP_MDI_READ << 26) | (reg << 16) | (phy << 21));

	while (((value = CSR_READ_4(sc, FXP_CSR_MDICONTROL)) &
	    0x10000000) == 0 && count--)
		DELAY(10);

	if (count <= 0)
		log(LOG_WARNING,
		    "%s: fxp_mdi_read: timed out\n", sc->sc_dev.dv_xname);

	return (value & 0xffff);
}

void
fxp_statchg(struct device *self)
{

	/* Nothing to do. */
}

void
fxp_mdi_write(struct device *self, int phy, int reg, int value)
{
	struct fxp_softc *sc = (struct fxp_softc *)self;
	int count = 10000;

	CSR_WRITE_4(sc, FXP_CSR_MDICONTROL,
	    (FXP_MDI_WRITE << 26) | (reg << 16) | (phy << 21) |
	    (value & 0xffff));

	while ((CSR_READ_4(sc, FXP_CSR_MDICONTROL) & 0x10000000) == 0 &&
	    count--)
		DELAY(10);

	if (count <= 0)
		log(LOG_WARNING,
		    "%s: fxp_mdi_write: timed out\n", sc->sc_dev.dv_xname);
}

int
fxp_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct fxp_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				/*
				 * Multicast list has changed; set the
				 * hardware filter accordingly.
				 */
				if (sc->sc_txpending) {
					sc->sc_flags |= FXPF_WANTINIT;
					error = 0;
				} else
					error = fxp_init(ifp);
			} else
				error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	if (sc->sc_enabled)
		fxp_start(ifp);

	splx(s);
	return (error);
}

/*
 * Program the multicast filter.
 *
 * This function must be called at splnet().
 */
void
fxp_mc_setup(struct fxp_softc *sc)
{
	struct fxp_cb_mcs *mcsp = &sc->sc_control_data->fcd_mcscb;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	int count, nmcasts;

#ifdef DIAGNOSTIC
	if (sc->sc_txpending)
		panic("fxp_mc_setup: pending transmissions");
#endif

	ifp->if_flags &= ~IFF_ALLMULTI;

	/*
	 * Initialize multicast setup descriptor.
	 */
	nmcasts = 0;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		/*
		 * Check for too many multicast addresses or if we're
		 * listening to a range.  Either way, we simply have
		 * to accept all multicasts.
		 */
		if (nmcasts >= MAXMCADDR ||
		    memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			/*
			 * Callers of this function must do the
			 * right thing with this.  If we're called
			 * from outside fxp_init(), the caller must
			 * detect if the state if IFF_ALLMULTI changes.
			 * If it does, the caller must then call
			 * fxp_init(), since allmulti is handled by
			 * the config block.
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			return;
		}
		memcpy(&mcsp->mc_addr[nmcasts][0], enm->enm_addrlo,
		    ETHER_ADDR_LEN);
		nmcasts++;
		ETHER_NEXT_MULTI(step, enm);
	}

	/* BIG_ENDIAN: no need to swap to store 0 */
	mcsp->cb_status = 0;
	mcsp->cb_command = htole16(FXP_CB_COMMAND_MCAS | FXP_CB_COMMAND_EL);
	mcsp->link_addr = htole32(FXP_CDTXADDR(sc, FXP_NEXTTX(sc->sc_txlast)));
	mcsp->mc_cnt = htole16(nmcasts * ETHER_ADDR_LEN);

	FXP_CDMCSSYNC(sc, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Wait until the command unit is not active.  This should never
	 * happen since nothing is queued, but make sure anyway.
	 */
	count = 100;
	while ((CSR_READ_1(sc, FXP_CSR_SCB_RUSCUS) >> 6) ==
	    FXP_SCB_CUS_ACTIVE && --count)
		DELAY(1);
	if (count == 0) {
		log(LOG_WARNING, "%s: line %d: command queue timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return;
	}

	/*
	 * Start the multicast setup command/DMA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, sc->sc_cddma + FXP_CDMCSOFF);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_START);

	/* ...and wait for it to complete. */
	count = 1000;
	do {
		FXP_CDMCSSYNC(sc,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DELAY(1);
	} while ((le16toh(mcsp->cb_status) & FXP_CB_STATUS_C) == 0 && --count);
	if (count == 0) {
		log(LOG_WARNING, "%s: line %d: dmasync timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return;
	}
}

static const uint32_t fxp_ucode_d101a[] = D101_A_RCVBUNDLE_UCODE;
static const uint32_t fxp_ucode_d101b0[] = D101_B0_RCVBUNDLE_UCODE;
static const uint32_t fxp_ucode_d101ma[] = D101M_B_RCVBUNDLE_UCODE;
static const uint32_t fxp_ucode_d101s[] = D101S_RCVBUNDLE_UCODE;
static const uint32_t fxp_ucode_d102[] = D102_B_RCVBUNDLE_UCODE;
static const uint32_t fxp_ucode_d102c[] = D102_C_RCVBUNDLE_UCODE;

#define	UCODE(x)	x, sizeof(x)/sizeof(uint32_t)

static const struct ucode {
	int32_t		revision;
	const uint32_t	*ucode;
	size_t		length;
	uint16_t	int_delay_offset;
	uint16_t	bundle_max_offset;
} ucode_table[] = {
	{ FXP_REV_82558_A4, UCODE(fxp_ucode_d101a),
	  D101_CPUSAVER_DWORD, 0 },

	{ FXP_REV_82558_B0, UCODE(fxp_ucode_d101b0),
	  D101_CPUSAVER_DWORD, 0 },

	{ FXP_REV_82559_A0, UCODE(fxp_ucode_d101ma),
	  D101M_CPUSAVER_DWORD, D101M_CPUSAVER_BUNDLE_MAX_DWORD },

	{ FXP_REV_82559S_A, UCODE(fxp_ucode_d101s),
	  D101S_CPUSAVER_DWORD, D101S_CPUSAVER_BUNDLE_MAX_DWORD },

	{ FXP_REV_82550, UCODE(fxp_ucode_d102),
	  D102_B_CPUSAVER_DWORD, D102_B_CPUSAVER_BUNDLE_MAX_DWORD },

	{ FXP_REV_82550_C, UCODE(fxp_ucode_d102c),
	  D102_C_CPUSAVER_DWORD, D102_C_CPUSAVER_BUNDLE_MAX_DWORD },

	{ 0, NULL, 0, 0, 0 }
};

void
fxp_load_ucode(struct fxp_softc *sc)
{
	const struct ucode *uc;
	struct fxp_cb_ucode *cbp = &sc->sc_control_data->fcd_ucode;
	int count, i;

	if (sc->sc_flags & FXPF_UCODE_LOADED)
		return;

	/*
	 * Only load the uCode if the user has requested that
	 * we do so.
	 */
	if ((sc->sc_ethercom.ec_if.if_flags & IFF_LINK0) == 0) {
		sc->sc_int_delay = 0;
		sc->sc_bundle_max = 0;
		return;
	}

	for (uc = ucode_table; uc->ucode != NULL; uc++) {
		if (sc->sc_rev == uc->revision)
			break;
	}
	if (uc->ucode == NULL)
		return;

	/* BIG ENDIAN: no need to swap to store 0 */
	cbp->cb_status = 0;
	cbp->cb_command = htole16(FXP_CB_COMMAND_UCODE | FXP_CB_COMMAND_EL);
	cbp->link_addr = 0xffffffff;		/* (no) next command */
	for (i = 0; i < uc->length; i++)
		cbp->ucode[i] = htole32(uc->ucode[i]);

	if (uc->int_delay_offset)
		*(volatile uint16_t *) &cbp->ucode[uc->int_delay_offset] =
		    htole16(fxp_int_delay + (fxp_int_delay / 2));

	if (uc->bundle_max_offset)
		*(volatile uint16_t *) &cbp->ucode[uc->bundle_max_offset] =
		    htole16(fxp_bundle_max);

	FXP_CDUCODESYNC(sc, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Download the uCode to the chip.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, sc->sc_cddma + FXP_CDUCODEOFF);
	fxp_scb_cmd(sc, FXP_SCB_COMMAND_CU_START);

	/* ...and wait for it to complete. */
	count = 10000;
	do {
		FXP_CDUCODESYNC(sc,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		DELAY(2);
	} while ((le16toh(cbp->cb_status) & FXP_CB_STATUS_C) == 0 && --count);
	if (count == 0) {
		sc->sc_int_delay = 0;
		sc->sc_bundle_max = 0;
		log(LOG_WARNING, "%s: timeout loading microcode\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (sc->sc_int_delay != fxp_int_delay ||
	    sc->sc_bundle_max != fxp_bundle_max) {
		sc->sc_int_delay = fxp_int_delay;
		sc->sc_bundle_max = fxp_bundle_max;
		log(LOG_INFO, "%s: Microcode loaded: int delay: %d usec, "
		    "max bundle: %d\n", sc->sc_dev.dv_xname,
		    sc->sc_int_delay,
		    uc->bundle_max_offset == 0 ? 0 : sc->sc_bundle_max);
	}

	sc->sc_flags |= FXPF_UCODE_LOADED;
}

int
fxp_enable(struct fxp_softc *sc)
{

	if (sc->sc_enabled == 0 && sc->sc_enable != NULL) {
		if ((*sc->sc_enable)(sc) != 0) {
			log(LOG_ERR, "%s: device enable failed\n",
			    sc->sc_dev.dv_xname);
			return (EIO);
		}
	}

	sc->sc_enabled = 1;
	return (0);
}

void
fxp_disable(struct fxp_softc *sc)
{

	if (sc->sc_enabled != 0 && sc->sc_disable != NULL) {
		(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}
}

/*
 * fxp_activate:
 *
 *	Handle device activation/deactivation requests.
 */
int
fxp_activate(struct device *self, enum devact act)
{
	struct fxp_softc *sc = (void *) self;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_flags & FXPF_MII)
			mii_activate(&sc->sc_mii, act, MII_PHY_ANY,
			    MII_OFFSET_ANY);
		if_deactivate(&sc->sc_ethercom.ec_if);
		break;
	}
	splx(s);

	return (error);
}

/*
 * fxp_detach:
 *
 *	Detach an i82557 interface.
 */
int
fxp_detach(struct fxp_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i;

	/* Succeed now if there's no work to do. */
	if ((sc->sc_flags & FXPF_ATTACHED) == 0)
		return (0);

	/* Unhook our tick handler. */
	callout_stop(&sc->sc_callout);

	if (sc->sc_flags & FXPF_MII) {
		/* Detach all PHYs */
		mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	}

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);

#if NRND > 0
	rnd_detach_source(&sc->rnd_source);
#endif
	ether_ifdetach(ifp);
	if_detach(ifp);

	for (i = 0; i < FXP_NRFABUFS; i++) {
		bus_dmamap_unload(sc->sc_dmat, sc->sc_rxmaps[i]);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_rxmaps[i]);
	}

	for (i = 0; i < FXP_NTXCB; i++) {
		bus_dmamap_unload(sc->sc_dmat, FXP_DSTX(sc, i)->txs_dmamap);
		bus_dmamap_destroy(sc->sc_dmat, FXP_DSTX(sc, i)->txs_dmamap);
	}

	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct fxp_control_data));
	bus_dmamem_free(sc->sc_dmat, &sc->sc_cdseg, sc->sc_cdnseg);

	shutdownhook_disestablish(sc->sc_sdhook);
	powerhook_disestablish(sc->sc_powerhook);

	return (0);
}
