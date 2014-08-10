/* $NetBSD: if_admsw.c,v 1.11.10.1 2014/08/10 06:54:02 tls Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device driver for Alchemy Semiconductor Au1x00 Ethernet Media
 * Access Controller.
 *
 * TODO:
 *
 *	Better Rx buffer management; we want to get new Rx buffers
 *	to the chip more quickly than we currently do.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_admsw.c,v 1.11.10.1 2014/08/10 06:54:02 tls Exp $");


#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <prop/proplib.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_obiovar.h>
#include <mips/adm5120/dev/if_admswreg.h>
#include <mips/adm5120/dev/if_admswvar.h>

static uint8_t vlan_matrix[SW_DEVS] = {
	(1 << 6) | (1 << 0),		/* CPU + port0 */
	(1 << 6) | (1 << 1),		/* CPU + port1 */
	(1 << 6) | (1 << 2),		/* CPU + port2 */
	(1 << 6) | (1 << 3),		/* CPU + port3 */
	(1 << 6) | (1 << 4),		/* CPU + port4 */
	(1 << 6) | (1 << 5),		/* CPU + port5 */
};

#ifdef ADMSW_EVENT_COUNTERS
#define	ADMSW_EVCNT_INCR(ev)	(ev)->ev_count++
#else
#define	ADMSW_EVCNT_INCR(ev)	/* nothing */
#endif

static void	admsw_start(struct ifnet *);
static void	admsw_watchdog(struct ifnet *);
static int	admsw_ioctl(struct ifnet *, u_long, void *);
static int	admsw_init(struct ifnet *);
static void	admsw_stop(struct ifnet *, int);

static void	admsw_shutdown(void *);

static void	admsw_reset(struct admsw_softc *);
static void	admsw_set_filter(struct admsw_softc *);

static int	admsw_intr(void *);
static void	admsw_txintr(struct admsw_softc *, int);
static void	admsw_rxintr(struct admsw_softc *, int);
static int	admsw_add_rxbuf(struct admsw_softc *, int, int);
#define	admsw_add_rxhbuf(sc, idx)	admsw_add_rxbuf(sc, idx, 1)
#define	admsw_add_rxlbuf(sc, idx)	admsw_add_rxbuf(sc, idx, 0)

static int	admsw_mediachange(struct ifnet *);
static void	admsw_mediastatus(struct ifnet *, struct ifmediareq *);

static int	admsw_match(device_t, cfdata_t, void *);
static void	admsw_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(admsw, sizeof(struct admsw_softc),
    admsw_match, admsw_attach, NULL, NULL);

static int
admsw_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *aa = aux;

	return strcmp(aa->oba_name, cf->cf_name) == 0;
}

#define	REG_READ(o)	bus_space_read_4(sc->sc_st, sc->sc_ioh, (o))
#define	REG_WRITE(o,v)	bus_space_write_4(sc->sc_st, sc->sc_ioh, (o),(v))


static void
admsw_init_bufs(struct admsw_softc *sc)
{
	int i;
	struct admsw_desc *desc;

	for (i = 0; i < ADMSW_NTXHDESC; i++) {
		if (sc->sc_txhsoft[i].ds_mbuf != NULL) {
			m_freem(sc->sc_txhsoft[i].ds_mbuf);
			sc->sc_txhsoft[i].ds_mbuf = NULL;
		}
		desc = &sc->sc_txhdescs[i];
		desc->data = 0;
		desc->cntl = 0;
		desc->len = MAC_BUFLEN;
		desc->status = 0;
		ADMSW_CDTXHSYNC(sc, i,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
	sc->sc_txhdescs[ADMSW_NTXHDESC - 1].data |= ADM5120_DMA_RINGEND;
	ADMSW_CDTXHSYNC(sc, ADMSW_NTXHDESC - 1,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	for (i = 0; i < ADMSW_NRXHDESC; i++) {
		if (sc->sc_rxhsoft[i].ds_mbuf == NULL) {
			if (admsw_add_rxhbuf(sc, i) != 0)
				panic("admsw_init_bufs\n");
		} else
			ADMSW_INIT_RXHDESC(sc, i);
	}

	for (i = 0; i < ADMSW_NTXLDESC; i++) {
		if (sc->sc_txlsoft[i].ds_mbuf != NULL) {
			m_freem(sc->sc_txlsoft[i].ds_mbuf);
			sc->sc_txlsoft[i].ds_mbuf = NULL;
		}
		desc = &sc->sc_txldescs[i];
		desc->data = 0;
		desc->cntl = 0;
		desc->len = MAC_BUFLEN;
		desc->status = 0;
		ADMSW_CDTXLSYNC(sc, i,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
	sc->sc_txldescs[ADMSW_NTXLDESC - 1].data |= ADM5120_DMA_RINGEND;
	ADMSW_CDTXLSYNC(sc, ADMSW_NTXLDESC - 1,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	for (i = 0; i < ADMSW_NRXLDESC; i++) {
		if (sc->sc_rxlsoft[i].ds_mbuf == NULL) {
			if (admsw_add_rxlbuf(sc, i) != 0)
				panic("admsw_init_bufs\n");
		} else
			ADMSW_INIT_RXLDESC(sc, i);
	}

	REG_WRITE(SEND_HBADDR_REG, ADMSW_CDTXHADDR(sc, 0));
	REG_WRITE(SEND_LBADDR_REG, ADMSW_CDTXLADDR(sc, 0));
	REG_WRITE(RECV_HBADDR_REG, ADMSW_CDRXHADDR(sc, 0));
	REG_WRITE(RECV_LBADDR_REG, ADMSW_CDRXLADDR(sc, 0));

	sc->sc_txfree = ADMSW_NTXLDESC;
	sc->sc_txnext = 0;
	sc->sc_txdirty = 0;
	sc->sc_rxptr = 0;
}

static void
admsw_setvlan(struct admsw_softc *sc, char matrix[6])
{
	uint32_t i;

	i = matrix[0] + (matrix[1] << 8) + (matrix[2] << 16) + (matrix[3] << 24);
	REG_WRITE(VLAN_G1_REG, i);
	i = matrix[4] + (matrix[5] << 8);
	REG_WRITE(VLAN_G2_REG, i);
}

static void
admsw_reset(struct admsw_softc *sc)
{
	uint32_t wdog1;
	int i;

	REG_WRITE(PORT_CONF0_REG,
	    REG_READ(PORT_CONF0_REG) | PORT_CONF0_DP_MASK);
	REG_WRITE(CPUP_CONF_REG,
	    REG_READ(CPUP_CONF_REG) | CPUP_CONF_DCPUP);

        /* Wait for DMA to complete.  Overkill.  In 3ms, we can
         * send at least two entire 1500-byte packets at 10 Mb/s.
	 */
	DELAY(3000);

        /* The datasheet recommends that we move all PHYs to reset
         * state prior to software reset.
	 */
	REG_WRITE(PHY_CNTL2_REG,
	    REG_READ(PHY_CNTL2_REG) & ~PHY_CNTL2_PHYR_MASK);

	/* Reset the switch. */
	REG_WRITE(ADMSW_SW_RES, 0x1);

	DELAY(100 * 1000);

	REG_WRITE(ADMSW_BOOT_DONE, ADMSW_BOOT_DONE_BO);

	/* begin old code */
	REG_WRITE(CPUP_CONF_REG,
	    CPUP_CONF_DCPUP | CPUP_CONF_CRCP | CPUP_CONF_DUNP_MASK |
	    CPUP_CONF_DMCP_MASK);

	REG_WRITE(PORT_CONF0_REG, PORT_CONF0_EMCP_MASK | PORT_CONF0_EMBP_MASK);

	REG_WRITE(PHY_CNTL2_REG,
	    REG_READ(PHY_CNTL2_REG) | PHY_CNTL2_ANE_MASK | PHY_CNTL2_PHYR_MASK |
	    PHY_CNTL2_AMDIX_MASK);

	REG_WRITE(PHY_CNTL3_REG, REG_READ(PHY_CNTL3_REG) | PHY_CNTL3_RNT);

	REG_WRITE(ADMSW_INT_MASK, INT_MASK);
	REG_WRITE(ADMSW_INT_ST, INT_MASK);

	/*
	 * While in DDB, we stop servicing interrupts, RX ring
	 * fills up and when free block counter falls behind FC
	 * threshold, the switch starts to emit 802.3x PAUSE
	 * frames.  This can upset peer switches.
	 *
	 * Stop this from happening by disabling FC and D2
	 * thresholds.
	 */
	REG_WRITE(FC_TH_REG,
	    REG_READ(FC_TH_REG) & ~(FC_TH_FCS_MASK | FC_TH_D2S_MASK));

	admsw_setvlan(sc, vlan_matrix);

	for (i = 0; i < SW_DEVS; i++) {
		REG_WRITE(MAC_WT1_REG,
		    sc->sc_enaddr[2] |
		    (sc->sc_enaddr[3]<<8) |
		    (sc->sc_enaddr[4]<<16) |
		    ((sc->sc_enaddr[5]+i)<<24));
		REG_WRITE(MAC_WT0_REG, (i<<MAC_WT0_VLANID_SHIFT) |
		    (sc->sc_enaddr[0]<<16) | (sc->sc_enaddr[1]<<24) |
		    MAC_WT0_WRITE | MAC_WT0_VLANID_EN);

		while (!(REG_READ(MAC_WT0_REG) & MAC_WT0_WRITE_DONE));
	}
	wdog1 = REG_READ(ADM5120_WDOG1);
	REG_WRITE(ADM5120_WDOG1, wdog1 & ~ADM5120_WDOG1_WDE);
}

static void
admsw_attach(device_t parent, device_t self, void *aux)
{
	uint8_t enaddr[ETHER_ADDR_LEN];
	struct admsw_softc *sc = device_private(self);
	struct obio_attach_args *aa = aux;
	struct ifnet *ifp;
	bus_dma_segment_t seg;
	int error, i, rseg;
	prop_data_t pd;

	printf(": ADM5120 Switch Engine, %d ports\n", SW_DEVS);

	sc->sc_dev = self;
	sc->sc_dmat = aa->oba_dt;
	sc->sc_st = aa->oba_st;

	pd = prop_dictionary_get(device_properties(self), "mac-address");

	if (pd == NULL) {
		enaddr[0] = 0x02;
		enaddr[1] = 0xaa;
		enaddr[2] = 0xbb;
		enaddr[3] = 0xcc;
		enaddr[4] = 0xdd;
		enaddr[5] = 0xee;
	} else
		memcpy(enaddr, prop_data_data_nocopy(pd), sizeof(enaddr));

	memcpy(sc->sc_enaddr, enaddr, sizeof(sc->sc_enaddr));

	printf("%s: base Ethernet address %s\n", device_xname(sc->sc_dev),
	    ether_sprintf(enaddr));

	/* Map the device. */
	if (bus_space_map(sc->sc_st, aa->oba_addr, 512, 0, &sc->sc_ioh) != 0) {
		printf("%s: unable to map device\n", device_xname(sc->sc_dev));
		return;
	}

	/* Hook up the interrupt handler. */
	sc->sc_ih = adm5120_intr_establish(aa->oba_irq, INTR_IRQ, admsw_intr, sc);

	if (sc->sc_ih == NULL) {
		printf("%s: unable to register interrupt handler\n",
		    device_xname(sc->sc_dev));
		return;
	}

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct admsw_control_data), PAGE_SIZE, 0, &seg, 1, &rseg,
	    0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct admsw_control_data), (void *)&sc->sc_control_data,
	    0)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}
	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct admsw_control_data), 1,
	    sizeof(struct admsw_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", device_xname(sc->sc_dev), error);
		return;
	}
	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct admsw_control_data), NULL,
	    0)) != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < ADMSW_NTXHDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    2, MCLBYTES, 0, 0,
		    &sc->sc_txhsoft[i].ds_dmamap)) != 0) {
			printf("%s: unable to create txh DMA map %d, "
			    "error = %d\n", device_xname(sc->sc_dev), i, error);
			return;
		}
		sc->sc_txhsoft[i].ds_mbuf = NULL;
	}
	for (i = 0; i < ADMSW_NTXLDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    2, MCLBYTES, 0, 0,
		    &sc->sc_txlsoft[i].ds_dmamap)) != 0) {
			printf("%s: unable to create txl DMA map %d, "
			    "error = %d\n", device_xname(sc->sc_dev), i, error);
			return;
		}
		sc->sc_txlsoft[i].ds_mbuf = NULL;
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < ADMSW_NRXHDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxhsoft[i].ds_dmamap)) != 0) {
			printf("%s: unable to create rxh DMA map %d, "
			    "error = %d\n", device_xname(sc->sc_dev), i, error);
			return;
		}
		sc->sc_rxhsoft[i].ds_mbuf = NULL;
	}
	for (i = 0; i < ADMSW_NRXLDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxlsoft[i].ds_dmamap)) != 0) {
			printf("%s: unable to create rxl DMA map %d, "
			    "error = %d\n", device_xname(sc->sc_dev), i, error);
			return;
		}
		sc->sc_rxlsoft[i].ds_mbuf = NULL;
	}

	admsw_init_bufs(sc);

	admsw_reset(sc);

	for (i = 0; i < SW_DEVS; i++) {
		ifmedia_init(&sc->sc_ifmedia[i], 0, admsw_mediachange, admsw_mediastatus);
		ifmedia_add(&sc->sc_ifmedia[i], IFM_ETHER|IFM_10_T, 0, NULL);
		ifmedia_add(&sc->sc_ifmedia[i], IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);
		ifmedia_add(&sc->sc_ifmedia[i], IFM_ETHER|IFM_100_TX, 0, NULL);
		ifmedia_add(&sc->sc_ifmedia[i], IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);
		ifmedia_add(&sc->sc_ifmedia[i], IFM_ETHER|IFM_AUTO, 0, NULL);
		ifmedia_set(&sc->sc_ifmedia[i], IFM_ETHER|IFM_AUTO);

		ifp = &sc->sc_ethercom[i].ec_if;
		strcpy(ifp->if_xname, device_xname(sc->sc_dev));
		ifp->if_xname[5] += i;
		ifp->if_softc = sc;
		ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
		ifp->if_ioctl = admsw_ioctl;
		ifp->if_start = admsw_start;
		ifp->if_watchdog = admsw_watchdog;
		ifp->if_init = admsw_init;
		ifp->if_stop = admsw_stop;
		ifp->if_capabilities |= IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx;
		IFQ_SET_MAXLEN(&ifp->if_snd, max(ADMSW_NTXLDESC, IFQ_MAXLEN));
		IFQ_SET_READY(&ifp->if_snd);

		/* Attach the interface. */
		if_attach(ifp);
		ether_ifattach(ifp, enaddr);
		enaddr[5]++;
	}

#ifdef ADMSW_EVENT_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_txstall, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "txstall");
	evcnt_attach_dynamic(&sc->sc_ev_rxstall, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "rxstall");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "rxintr");
#if 1
	evcnt_attach_dynamic(&sc->sc_ev_rxsync, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "rxsync");
#endif
#endif

	admwdog_attach(sc);

	/* Make sure the interface is shutdown during reboot. */
	sc->sc_sdhook = shutdownhook_establish(admsw_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    device_xname(sc->sc_dev));

	/* leave interrupts and cpu port disabled */
	return;
}


/*
 * admsw_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
static void
admsw_shutdown(void *arg)
{
	struct admsw_softc *sc = arg;
	int i;

	for (i = 0; i < SW_DEVS; i++)
		admsw_stop(&sc->sc_ethercom[i].ec_if, 1);
}

/*
 * admsw_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
static void
admsw_start(struct ifnet *ifp)
{
	struct admsw_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct admsw_descsoft *ds;
	struct admsw_desc *desc;
	bus_dmamap_t dmamap;
	struct ether_header *eh;
	int error, nexttx, len, i;
	static int vlan = 0;

	/*
	 * Loop through the send queues, setting up transmit descriptors
	 * unitl we drain the queues, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		vlan++;
		if (vlan == SW_DEVS)
			vlan = 0;
		i = vlan;
		for (;;) {
			ifp = &sc->sc_ethercom[i].ec_if;
			if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) ==
			    IFF_RUNNING) {
				/* Grab a packet off the queue. */
				IFQ_POLL(&ifp->if_snd, m0);
				if (m0 != NULL)
					break;
			}
			i++;
			if (i == SW_DEVS)
				i = 0;
			if (i == vlan)
				return;
		}
		vlan = i;
		m = NULL;

		/* Get a spare descriptor. */
		if (sc->sc_txfree == 0) {
			/* No more slots left; notify upper layer. */
			ifp->if_flags |= IFF_OACTIVE;
			ADMSW_EVCNT_INCR(&sc->sc_ev_txstall);
			break;
		}
		nexttx = sc->sc_txnext;
		desc = &sc->sc_txldescs[nexttx];
		ds = &sc->sc_txlsoft[nexttx];
		dmamap = ds->ds_dmamap;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the alloted number of segments, or we
		 * were short on resources.  In this case, we'll copy
		 * and try again.
		 */
		if (m0->m_pkthdr.len < ETHER_MIN_LEN ||
		    bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				    device_xname(sc->sc_dev));
				break;
			}
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", device_xname(sc->sc_dev));
					m_freem(m);
					break;
				}
			}
			m->m_pkthdr.csum_flags = m0->m_pkthdr.csum_flags;
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, void *));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			if (m->m_pkthdr.len < ETHER_MIN_LEN) {
				if (M_TRAILINGSPACE(m) < ETHER_MIN_LEN - m->m_pkthdr.len)
					panic("admsw_start: M_TRAILINGSPACE\n");
				memset(mtod(m, uint8_t *) + m->m_pkthdr.len, 0,
				    ETHER_MIN_LEN - ETHER_CRC_LEN - m->m_pkthdr.len);
				m->m_pkthdr.len = m->m_len = ETHER_MIN_LEN;
			}
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", device_xname(sc->sc_dev), error);
				break;
			}
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		if (dmamap->dm_nsegs != 1 && dmamap->dm_nsegs != 2)
			panic("admsw_start: dm_nsegs == %d\n", dmamap->dm_nsegs);
		desc->data = dmamap->dm_segs[0].ds_addr;
		desc->len = len = dmamap->dm_segs[0].ds_len;
		if (dmamap->dm_nsegs > 1) {
			len += dmamap->dm_segs[1].ds_len;
			desc->cntl = dmamap->dm_segs[1].ds_addr | ADM5120_DMA_BUF2ENABLE;
		} else
			desc->cntl = 0;
		desc->status = (len << ADM5120_DMA_LENSHIFT) | (1 << vlan);
		eh = mtod(m0, struct ether_header *);
		if (ntohs(eh->ether_type) == ETHERTYPE_IP &&
		    m0->m_pkthdr.csum_flags & M_CSUM_IPv4)
			desc->status |= ADM5120_DMA_CSUM;
		if (nexttx == ADMSW_NTXLDESC - 1)
			desc->data |= ADM5120_DMA_RINGEND;
		desc->data |= ADM5120_DMA_OWN;

		/* Sync the descriptor. */
		ADMSW_CDTXLSYNC(sc, nexttx,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		REG_WRITE(SEND_TRIG_REG, 1);
		/* printf("send slot %d\n",nexttx); */

		/*
		 * Store a pointer to the packet so we can free it later.
		 */
		ds->ds_mbuf = m0;

		/* Advance the Tx pointer. */
		sc->sc_txfree--;
		sc->sc_txnext = ADMSW_NEXTTXL(nexttx);

		/* Pass the packet to any BPF listeners. */
		bpf_mtap(ifp, m0);

		/* Set a watchdog timer in case the chip flakes out. */
		sc->sc_ethercom[0].ec_if.if_timer = 5;
	}
}

/*
 * admsw_watchdog:	[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
static void
admsw_watchdog(struct ifnet *ifp)
{
	struct admsw_softc *sc = ifp->if_softc;
	int vlan;

#if 1
	/* Check if an interrupt was lost. */
	if (sc->sc_txfree == ADMSW_NTXLDESC) {
		printf("%s: watchdog false alarm\n", device_xname(sc->sc_dev));
		return;
	}
	if (sc->sc_ethercom[0].ec_if.if_timer != 0)
		printf("%s: watchdog timer is %d!\n", device_xname(sc->sc_dev), sc->sc_ethercom[0].ec_if.if_timer);
	admsw_txintr(sc, 0);
	if (sc->sc_txfree == ADMSW_NTXLDESC) {
		printf("%s: tx IRQ lost (queue empty)\n", device_xname(sc->sc_dev));
		return;
	}
	if (sc->sc_ethercom[0].ec_if.if_timer != 0) {
		printf("%s: tx IRQ lost (timer recharged)\n", device_xname(sc->sc_dev));
		return;
	}
#endif

	printf("%s: device timeout, txfree = %d\n", device_xname(sc->sc_dev), sc->sc_txfree);
	for (vlan = 0; vlan < SW_DEVS; vlan++)
		admsw_stop(&sc->sc_ethercom[vlan].ec_if, 0);
	for (vlan = 0; vlan < SW_DEVS; vlan++)
		(void) admsw_init(&sc->sc_ethercom[vlan].ec_if);

	/* Try to get more packets going. */
	admsw_start(ifp);
}

/*
 * admsw_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
static int
admsw_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct admsw_softc *sc = ifp->if_softc;
	struct ifdrv *ifd;
	int s, error, port;

	s = splnet();

	switch (cmd) {
	case SIOCSIFCAP:
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET)
			error = 0;
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		port = (struct ethercom *)ifp - sc->sc_ethercom;	/* XXX */
		if (port >= SW_DEVS)
			error = EOPNOTSUPP;
		else
			error = ifmedia_ioctl(ifp, (struct ifreq *)data,
			    &sc->sc_ifmedia[port], cmd);
		break;

	case SIOCGDRVSPEC:
	case SIOCSDRVSPEC:
		ifd = (struct ifdrv *) data;
		if (ifd->ifd_cmd != 0 || ifd->ifd_len != sizeof(vlan_matrix)) {
			error = EINVAL;
			break;
		}
		if (cmd == SIOCGDRVSPEC) {
			error = copyout(vlan_matrix, ifd->ifd_data,
			    sizeof(vlan_matrix));
		} else {
			error = copyin(ifd->ifd_data, vlan_matrix,
			    sizeof(vlan_matrix));
			admsw_setvlan(sc, vlan_matrix);
		}
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			admsw_set_filter(sc);
			error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	admsw_start(ifp);

	splx(s);
	return (error);
}


/*
 * admsw_intr:
 *
 *	Interrupt service routine.
 */
static int
admsw_intr(void *arg)
{
	struct admsw_softc *sc = arg;
	uint32_t pending;
	char buf[64];

	pending = REG_READ(ADMSW_INT_ST);

	if ((pending & ~(ADMSW_INTR_RHD|ADMSW_INTR_RLD|ADMSW_INTR_SHD|ADMSW_INTR_SLD|ADMSW_INTR_W1TE|ADMSW_INTR_W0TE)) != 0) {
		snprintb(buf, sizeof(buf), ADMSW_INT_FMT, pending);
		printf("%s: pending=%s\n", __func__, buf);
	}
	REG_WRITE(ADMSW_INT_ST, pending);

	if (sc->ndevs == 0)
		return (0);

	if ((pending & ADMSW_INTR_RHD) != 0)
		admsw_rxintr(sc, 1);

	if ((pending & ADMSW_INTR_RLD) != 0)
		admsw_rxintr(sc, 0);

	if ((pending & ADMSW_INTR_SHD) != 0)
		admsw_txintr(sc, 1);

	if ((pending & ADMSW_INTR_SLD) != 0)
		admsw_txintr(sc, 0);

	return (1);
}

/*
 * admsw_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
static void
admsw_txintr(struct admsw_softc *sc, int prio)
{
	struct ifnet *ifp;
	struct admsw_desc *desc;
	struct admsw_descsoft *ds;
	int i, vlan;
	int gotone = 0;

	/* printf("txintr: txdirty: %d, txfree: %d\n",sc->sc_txdirty, sc->sc_txfree); */
	for (i = sc->sc_txdirty; sc->sc_txfree != ADMSW_NTXLDESC;
	    i = ADMSW_NEXTTXL(i)) {

		ADMSW_CDTXLSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		desc = &sc->sc_txldescs[i];
		ds = &sc->sc_txlsoft[i];
		if (desc->data & ADM5120_DMA_OWN) {
			ADMSW_CDTXLSYNC(sc, i,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, ds->ds_dmamap,
		    0, ds->ds_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ds->ds_dmamap);
		m_freem(ds->ds_mbuf);
		ds->ds_mbuf = NULL;

		vlan = ffs(desc->status & 0x3f) - 1;
		if (vlan < 0 || vlan >= SW_DEVS)
			panic("admsw_txintr: bad vlan\n");
		ifp = &sc->sc_ethercom[vlan].ec_if;
		gotone = 1;
		/* printf("clear tx slot %d\n",i); */

		ifp->if_opackets++;

		sc->sc_txfree++;
	}

	if (gotone) {
		sc->sc_txdirty = i;
#ifdef ADMSW_EVENT_COUNTERS
		ADMSW_EVCNT_INCR(&sc->sc_ev_txintr);
#endif
		for (vlan = 0; vlan < SW_DEVS; vlan++)
			sc->sc_ethercom[vlan].ec_if.if_flags &= ~IFF_OACTIVE;

		ifp = &sc->sc_ethercom[0].ec_if;

		/* Try to queue more packets. */
		admsw_start(ifp);

		/*
		 * If there are no more pending transmissions,
		 * cancel the watchdog timer.
		 */
		if (sc->sc_txfree == ADMSW_NTXLDESC)
			ifp->if_timer = 0;

	}

	/* printf("txintr end: txdirty: %d, txfree: %d\n",sc->sc_txdirty, sc->sc_txfree); */
}

/*
 * admsw_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
static void
admsw_rxintr(struct admsw_softc *sc, int high)
{
	struct ifnet *ifp;
	struct admsw_descsoft *ds;
	struct mbuf *m;
	uint32_t stat;
	int i, len, port, vlan;

	/* printf("rxintr\n"); */
	if (high)
		panic("admsw_rxintr: high priority packet\n");

#ifdef ADMSW_EVENT_COUNTERS
	int pkts = 0;
#endif

#if 1
	ADMSW_CDRXLSYNC(sc, sc->sc_rxptr, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	if ((sc->sc_rxldescs[sc->sc_rxptr].data & ADM5120_DMA_OWN) == 0)
		ADMSW_CDRXLSYNC(sc, sc->sc_rxptr, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	else {
		i = sc->sc_rxptr;
		do {
			ADMSW_CDRXLSYNC(sc, i, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			i = ADMSW_NEXTRXL(i);
			/* the ring is empty, just return. */
			if (i == sc->sc_rxptr)
				return;
			ADMSW_CDRXLSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		} while (sc->sc_rxldescs[i].data & ADM5120_DMA_OWN);
		ADMSW_CDRXLSYNC(sc, i, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		ADMSW_CDRXLSYNC(sc, sc->sc_rxptr, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		if ((sc->sc_rxldescs[sc->sc_rxptr].data & ADM5120_DMA_OWN) == 0)
			ADMSW_CDRXLSYNC(sc, sc->sc_rxptr, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		else {
			ADMSW_CDRXLSYNC(sc, sc->sc_rxptr, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			/* We've fallen behind the chip: catch it. */
			printf("%s: RX ring resync, base=%x, work=%x, %d -> %d\n",
			    device_xname(sc->sc_dev), REG_READ(RECV_LBADDR_REG),
			    REG_READ(RECV_LWADDR_REG), sc->sc_rxptr, i);
			sc->sc_rxptr = i;
			ADMSW_EVCNT_INCR(&sc->sc_ev_rxsync);
		}
	}
#endif
	for (i = sc->sc_rxptr;; i = ADMSW_NEXTRXL(i)) {
		ds = &sc->sc_rxlsoft[i];

		ADMSW_CDRXLSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		if (sc->sc_rxldescs[i].data & ADM5120_DMA_OWN) {
			ADMSW_CDRXLSYNC(sc, i, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			break;
		}

		/* printf("process slot %d\n",i); */

#ifdef ADMSW_EVENT_COUNTERS
		pkts++;
#endif

		bus_dmamap_sync(sc->sc_dmat, ds->ds_dmamap, 0,
		    ds->ds_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		stat = sc->sc_rxldescs[i].status;
		len = (stat & ADM5120_DMA_LEN) >> ADM5120_DMA_LENSHIFT;
		len -= ETHER_CRC_LEN;
		port = (stat & ADM5120_DMA_PORTID) >> ADM5120_DMA_PORTSHIFT;
		for (vlan = 0; vlan < SW_DEVS; vlan++)
			if ((1 << port) & vlan_matrix[vlan])
				break;
		if (vlan == SW_DEVS)
			vlan = 0;
		ifp = &sc->sc_ethercom[vlan].ec_if;

		m = ds->ds_mbuf;
		if (admsw_add_rxlbuf(sc, i) != 0) {
			ifp->if_ierrors++;
			ADMSW_INIT_RXLDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, ds->ds_dmamap, 0,
			    ds->ds_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}

		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;
		if ((stat & ADM5120_DMA_TYPE) == ADM5120_DMA_TYPE_IP) {
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			if (stat & ADM5120_DMA_CSUMFAIL)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
		}
		/* Pass this up to any BPF listeners. */
		bpf_mtap(ifp, m);

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
		ifp->if_ipackets++;
	}
#ifdef ADMSW_EVENT_COUNTERS
	if (pkts)
		ADMSW_EVCNT_INCR(&sc->sc_ev_rxintr);

	if (pkts == ADMSW_NRXLDESC)
		ADMSW_EVCNT_INCR(&sc->sc_ev_rxstall);
#endif

	/* Update the receive pointer. */
	sc->sc_rxptr = i;
}

/*
 * admsw_init:		[ifnet interface function]
 *
 *	Initialize the interface.  Must be called at splnet().
 */
static int
admsw_init(struct ifnet *ifp)
{
	struct admsw_softc *sc = ifp->if_softc;

	/* printf("admsw_init called\n"); */

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		if (sc->ndevs == 0) {
			admsw_init_bufs(sc);
			admsw_reset(sc);
			REG_WRITE(CPUP_CONF_REG,
			    CPUP_CONF_CRCP | CPUP_CONF_DUNP_MASK |
			    CPUP_CONF_DMCP_MASK);
			/* clear all pending interrupts */
			REG_WRITE(ADMSW_INT_ST, INT_MASK);

			/* enable needed interrupts */
			REG_WRITE(ADMSW_INT_MASK, REG_READ(ADMSW_INT_MASK) &
			    ~(ADMSW_INTR_SHD | ADMSW_INTR_SLD | ADMSW_INTR_RHD |
			    ADMSW_INTR_RLD | ADMSW_INTR_HDF | ADMSW_INTR_LDF));
		}
		sc->ndevs++;
	}

	/* Set the receive filter. */
	admsw_set_filter(sc);

	/* mark iface as running */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

/*
 * admsw_stop:		[ifnet interface function]
 *
 *	Stop transmission on the interface.
 */
static void
admsw_stop(struct ifnet *ifp, int disable)
{
	struct admsw_softc *sc = ifp->if_softc;

	/* printf("admsw_stop: %d\n",disable); */

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (--sc->ndevs == 0) {
		/* printf("debug: de-initializing hardware\n"); */

		/* disable cpu port */
		REG_WRITE(CPUP_CONF_REG,
				CPUP_CONF_DCPUP | CPUP_CONF_CRCP |
				CPUP_CONF_DUNP_MASK | CPUP_CONF_DMCP_MASK);

		/* XXX We should disable, then clear? --dyoung */
		/* clear all pending interrupts */
		REG_WRITE(ADMSW_INT_ST, INT_MASK);

		/* disable interrupts */
		REG_WRITE(ADMSW_INT_MASK, INT_MASK);
	}

	/* Mark the interface as down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	return;
}

/*
 * admsw_set_filter:
 *
 *	Set up the receive filter.
 */
static void
admsw_set_filter(struct admsw_softc *sc)
{
	int i;
	uint32_t allmc, anymc, conf, promisc;
	struct ether_multi *enm;
	struct ethercom *ec;
	struct ifnet *ifp;
	struct ether_multistep step;

	/* Find which ports should be operated in promisc mode. */
	allmc = anymc = promisc = 0;
	for (i = 0; i < SW_DEVS; i++) {
		ec = &sc->sc_ethercom[i];
		ifp = &ec->ec_if;
		if (ifp->if_flags & IFF_PROMISC)
			promisc |= vlan_matrix[i];

		ifp->if_flags &= ~IFF_ALLMULTI;

		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN) != 0) {
				printf("%s: punting on mcast range\n",
				    __func__);
				ifp->if_flags |= IFF_ALLMULTI;
				allmc |= vlan_matrix[i];
				break;
			}

			anymc |= vlan_matrix[i];

#if 0
			/* XXX extract subroutine --dyoung */
			REG_WRITE(MAC_WT1_REG,
			    enm->enm_addrlo[2] |
			    (enm->enm_addrlo[3] << 8) |
			    (enm->enm_addrlo[4] << 16) |
			    (enm->enm_addrlo[5] << 24));
			REG_WRITE(MAC_WT0_REG,
			    (i << MAC_WT0_VLANID_SHIFT) |
			    (enm->enm_addrlo[0] << 16) |
			    (enm->enm_addrlo[1] << 24) |
			    MAC_WT0_WRITE | MAC_WT0_VLANID_EN);
			/* timeout? */
			while (!(REG_READ(MAC_WT0_REG) & MAC_WT0_WRITE_DONE));
#endif

			/* load h/w with mcast address, port = CPU */
			ETHER_NEXT_MULTI(step, enm);
		}
	}

	conf = REG_READ(CPUP_CONF_REG);
        /* 1 Disable forwarding of unknown & multicast packets to
         *   CPU on all ports.
         * 2 Enable forwarding of unknown & multicast packets to
         *   CPU on ports where IFF_PROMISC or IFF_ALLMULTI is set.
	 */
	conf |= CPUP_CONF_DUNP_MASK | CPUP_CONF_DMCP_MASK;
	/* Enable forwarding of unknown packets to CPU on selected ports. */
	conf ^= ((promisc << CPUP_CONF_DUNP_SHIFT) & CPUP_CONF_DUNP_MASK);
	conf ^= ((allmc << CPUP_CONF_DMCP_SHIFT) & CPUP_CONF_DMCP_MASK);
	conf ^= ((anymc << CPUP_CONF_DMCP_SHIFT) & CPUP_CONF_DMCP_MASK);
	REG_WRITE(CPUP_CONF_REG, conf);
}

/*
 * admsw_add_rxbuf:
 *
 *	Add a receive buffer to the indicated descriptor.
 */
int
admsw_add_rxbuf(struct admsw_softc *sc, int idx, int high)
{
	struct admsw_descsoft *ds;
	struct mbuf *m;
	int error;

	if (high)
		ds = &sc->sc_rxhsoft[idx];
	else
		ds = &sc->sc_rxlsoft[idx];

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (ds->ds_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, ds->ds_dmamap);

	ds->ds_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, ds->ds_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL,
	    BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    device_xname(sc->sc_dev), idx, error);
		panic("admsw_add_rxbuf");	/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, ds->ds_dmamap, 0,
	    ds->ds_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	if (high)
		ADMSW_INIT_RXHDESC(sc, idx);
	else
		ADMSW_INIT_RXLDESC(sc, idx);

	return (0);
}

int
admsw_mediachange(struct ifnet *ifp)
{
	struct admsw_softc *sc = ifp->if_softc;
	int port = (struct ethercom *)ifp - sc->sc_ethercom;	/* XXX */
	struct ifmedia *ifm = &sc->sc_ifmedia[port];
	int old, new, val;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	if (IFM_SUBTYPE(ifm->ifm_media) == IFM_AUTO) {
		val = PHY_CNTL2_AUTONEG|PHY_CNTL2_100M|PHY_CNTL2_FDX;
	} else if (IFM_SUBTYPE(ifm->ifm_media) == IFM_100_TX) {
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			val = PHY_CNTL2_100M|PHY_CNTL2_FDX;
		else
			val = PHY_CNTL2_100M;
	} else if (IFM_SUBTYPE(ifm->ifm_media) == IFM_10_T) {
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			val = PHY_CNTL2_FDX;
		else
			val = 0;
	} else
		return (EINVAL);

	old = REG_READ(PHY_CNTL2_REG);
	new = old & ~((PHY_CNTL2_AUTONEG|PHY_CNTL2_100M|PHY_CNTL2_FDX) << port);
	new |= (val << port);

	if (new != old)
		REG_WRITE(PHY_CNTL2_REG, new);

	return (0);
}

void
admsw_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct admsw_softc *sc = ifp->if_softc;
	int port = (struct ethercom *)ifp - sc->sc_ethercom;	/* XXX */
	int status;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	status = REG_READ(PHY_ST_REG) >> port;

	if ((status & PHY_ST_LINKUP) == 0) {
		ifmr->ifm_active |= IFM_NONE;
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;
	ifmr->ifm_active |= (status & PHY_ST_100M) ? IFM_100_TX : IFM_10_T;
	if (status & PHY_ST_FDX)
		ifmr->ifm_active |= IFM_FDX;
	else
		ifmr->ifm_active |= IFM_HDX;
}
