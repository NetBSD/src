/*	$NetBSD: if_cpsw.c,v 1.6.4.5 2016/10/05 20:55:25 skrll Exp $	*/

/*
 * Copyright (c) 2013 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2012 Damjan Marion <dmarion@Freebsd.org>
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: if_cpsw.c,v 1.6.4.5 2016/10/05 20:55:25 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <arch/arm/omap/omap2_obiovar.h>
#include <arch/arm/omap/if_cpswreg.h>
#include <arch/arm/omap/sitara_cmreg.h>
#include <arch/arm/omap/sitara_cm.h>

#define ETHER_ALIGN (roundup2(ETHER_HDR_LEN, sizeof(uint32_t)) - ETHER_HDR_LEN)

#define CPSW_TXFRAGS	16

#define CPSW_CPPI_RAM_SIZE (0x2000)
#define CPSW_CPPI_RAM_TXDESCS_SIZE (CPSW_CPPI_RAM_SIZE/2)
#define CPSW_CPPI_RAM_RXDESCS_SIZE \
    (CPSW_CPPI_RAM_SIZE - CPSW_CPPI_RAM_TXDESCS_SIZE)
#define CPSW_CPPI_RAM_TXDESCS_BASE (CPSW_CPPI_RAM_OFFSET + 0x0000)
#define CPSW_CPPI_RAM_RXDESCS_BASE \
    (CPSW_CPPI_RAM_OFFSET + CPSW_CPPI_RAM_TXDESCS_SIZE)

#define CPSW_NTXDESCS (CPSW_CPPI_RAM_TXDESCS_SIZE/sizeof(struct cpsw_cpdma_bd))
#define CPSW_NRXDESCS (CPSW_CPPI_RAM_RXDESCS_SIZE/sizeof(struct cpsw_cpdma_bd))

CTASSERT(powerof2(CPSW_NTXDESCS));
CTASSERT(powerof2(CPSW_NRXDESCS));

#define CPSW_PAD_LEN (ETHER_MIN_LEN - ETHER_CRC_LEN)

#define TXDESC_NEXT(x) cpsw_txdesc_adjust((x), 1)
#define TXDESC_PREV(x) cpsw_txdesc_adjust((x), -1)

#define RXDESC_NEXT(x) cpsw_rxdesc_adjust((x), 1)
#define RXDESC_PREV(x) cpsw_rxdesc_adjust((x), -1)

struct cpsw_ring_data {
	bus_dmamap_t tx_dm[CPSW_NTXDESCS];
	struct mbuf *tx_mb[CPSW_NTXDESCS];
	bus_dmamap_t rx_dm[CPSW_NRXDESCS];
	struct mbuf *rx_mb[CPSW_NRXDESCS];
};

struct cpsw_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_size_t sc_bss;
	bus_dma_tag_t sc_bdt;
	bus_space_handle_t sc_bsh_txdescs;
	bus_space_handle_t sc_bsh_rxdescs;
	bus_addr_t sc_txdescs_pa;
	bus_addr_t sc_rxdescs_pa;
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	bool sc_phy_has_1000t;
	bool sc_attached;
	callout_t sc_tick_ch;
	void *sc_ih;
	struct cpsw_ring_data *sc_rdp;
	volatile u_int sc_txnext;
	volatile u_int sc_txhead;
	volatile u_int sc_rxhead;
	void *sc_rxthih;
	void *sc_rxih;
	void *sc_txih;
	void *sc_miscih;
	void *sc_txpad;
	bus_dmamap_t sc_txpad_dm;
#define sc_txpad_pa sc_txpad_dm->dm_segs[0].ds_addr
	uint8_t sc_enaddr[ETHER_ADDR_LEN];
	volatile bool sc_txrun;
	volatile bool sc_rxrun;
	volatile bool sc_txeoq;
	volatile bool sc_rxeoq;
};

static int cpsw_match(device_t, cfdata_t, void *);
static void cpsw_attach(device_t, device_t, void *);
static int cpsw_detach(device_t, int);

static void cpsw_start(struct ifnet *);
static int cpsw_ioctl(struct ifnet *, u_long, void *);
static void cpsw_watchdog(struct ifnet *);
static int cpsw_init(struct ifnet *);
static void cpsw_stop(struct ifnet *, int);

static int cpsw_mii_readreg(device_t, int, int);
static void cpsw_mii_writereg(device_t, int, int, int);
static void cpsw_mii_statchg(struct ifnet *);

static int cpsw_new_rxbuf(struct cpsw_softc * const, const u_int);
static void cpsw_tick(void *);

static int cpsw_rxthintr(void *);
static int cpsw_rxintr(void *);
static int cpsw_txintr(void *);
static int cpsw_miscintr(void *);

/* ALE support */
#define CPSW_MAX_ALE_ENTRIES	1024

static int cpsw_ale_update_addresses(struct cpsw_softc *, int purge);

CFATTACH_DECL_NEW(cpsw, sizeof(struct cpsw_softc),
    cpsw_match, cpsw_attach, cpsw_detach, NULL);

#undef KERNHIST
#include <sys/kernhist.h>
KERNHIST_DEFINE(cpswhist);

#ifdef KERNHIST
#define KERNHIST_CALLED_5(NAME, i, j, k, l) \
do { \
	_kernhist_call = atomic_inc_uint_nv(&_kernhist_cnt); \
	KERNHIST_LOG(NAME, "called! %x %x %x %x", i, j, k, l); \
} while (/*CONSTCOND*/ 0)
#else
#define KERNHIST_CALLED_5(NAME, i, j, k, l)
#endif

static inline u_int
cpsw_txdesc_adjust(u_int x, int y)
{
	return (((x) + y) & (CPSW_NTXDESCS - 1));
}

static inline u_int
cpsw_rxdesc_adjust(u_int x, int y)
{
	return (((x) + y) & (CPSW_NRXDESCS - 1));
}

static inline uint32_t
cpsw_read_4(struct cpsw_softc * const sc, bus_size_t const offset)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, offset);
}

static inline void
cpsw_write_4(struct cpsw_softc * const sc, bus_size_t const offset,
    uint32_t const value)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, offset, value);
}

static inline void
cpsw_set_txdesc_next(struct cpsw_softc * const sc, const u_int i, uint32_t n)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i + 0;

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, n, 0);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh_txdescs, o, n);
}

static inline void
cpsw_set_rxdesc_next(struct cpsw_softc * const sc, const u_int i, uint32_t n)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i + 0;

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, n, 0);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh_rxdescs, o, n);
}

static inline void
cpsw_get_txdesc(struct cpsw_softc * const sc, const u_int i,
    struct cpsw_cpdma_bd * const bdp)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i;
	uint32_t * const dp = bdp->word;
	const bus_size_t c = __arraycount(bdp->word);

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, bdp, 0);

	bus_space_read_region_4(sc->sc_bst, sc->sc_bsh_txdescs, o, dp, c);
	KERNHIST_LOG(cpswhist, "%08x %08x %08x %08x\n",
	    dp[0], dp[1], dp[2], dp[3]);
}

static inline void
cpsw_set_txdesc(struct cpsw_softc * const sc, const u_int i,
    struct cpsw_cpdma_bd * const bdp)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i;
	uint32_t * const dp = bdp->word;
	const bus_size_t c = __arraycount(bdp->word);

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, bdp, 0);
	KERNHIST_LOG(cpswhist, "%08x %08x %08x %08x\n",
	    dp[0], dp[1], dp[2], dp[3]);

	bus_space_write_region_4(sc->sc_bst, sc->sc_bsh_txdescs, o, dp, c);
}

static inline void
cpsw_get_rxdesc(struct cpsw_softc * const sc, const u_int i,
    struct cpsw_cpdma_bd * const bdp)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i;
	uint32_t * const dp = bdp->word;
	const bus_size_t c = __arraycount(bdp->word);

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, bdp, 0);

	bus_space_read_region_4(sc->sc_bst, sc->sc_bsh_rxdescs, o, dp, c);

	KERNHIST_LOG(cpswhist, "%08x %08x %08x %08x\n",
	    dp[0], dp[1], dp[2], dp[3]);
}

static inline void
cpsw_set_rxdesc(struct cpsw_softc * const sc, const u_int i,
    struct cpsw_cpdma_bd * const bdp)
{
	const bus_size_t o = sizeof(struct cpsw_cpdma_bd) * i;
	uint32_t * const dp = bdp->word;
	const bus_size_t c = __arraycount(bdp->word);

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, i, bdp, 0);
	KERNHIST_LOG(cpswhist, "%08x %08x %08x %08x\n",
	    dp[0], dp[1], dp[2], dp[3]);

	bus_space_write_region_4(sc->sc_bst, sc->sc_bsh_rxdescs, o, dp, c);
}

static inline bus_addr_t
cpsw_txdesc_paddr(struct cpsw_softc * const sc, u_int x)
{
	KASSERT(x < CPSW_NTXDESCS);
	return sc->sc_txdescs_pa + sizeof(struct cpsw_cpdma_bd) * x;
}

static inline bus_addr_t
cpsw_rxdesc_paddr(struct cpsw_softc * const sc, u_int x)
{
	KASSERT(x < CPSW_NRXDESCS);
	return sc->sc_rxdescs_pa + sizeof(struct cpsw_cpdma_bd) * x;
}


static int
cpsw_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const oa = aux;
#ifdef TI_AM335X
	if (oa->obio_addr == 0x4a100000 && oa->obio_size >= 0x4000)
		return 1;
#endif
	return 0;
}

static bool
cpsw_phy_has_1000t(struct cpsw_softc * const sc)
{
	struct ifmedia_entry *ifm;

	TAILQ_FOREACH(ifm, &sc->sc_mii.mii_media.ifm_list, ifm_list) {
		if (IFM_SUBTYPE(ifm->ifm_media) == IFM_1000_T)
			return true;
	}
	return false;
}

static int
cpsw_detach(device_t self, int flags)
{
	struct cpsw_softc * const sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	u_int i;

	/* Succeed now if there's no work to do. */
	if (!sc->sc_attached)
		return 0;

	sc->sc_attached = false;

	/* Stop the interface. Callouts are stopped in it. */
	cpsw_stop(ifp, 1);

	/* Destroy our callout. */
	callout_destroy(&sc->sc_tick_ch);

	/* Let go of the interrupts */
	intr_disestablish(sc->sc_rxthih);
	intr_disestablish(sc->sc_rxih);
	intr_disestablish(sc->sc_txih);
	intr_disestablish(sc->sc_miscih);

	/* Delete all media. */
	ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);

	ether_ifdetach(ifp);
	if_detach(ifp);

	/* Free the packet padding buffer */
	kmem_free(sc->sc_txpad, ETHER_MIN_LEN);
	bus_dmamap_destroy(sc->sc_bdt, sc->sc_txpad_dm);

	/* Destroy all the descriptors */
	for (i = 0; i < CPSW_NTXDESCS; i++)
		bus_dmamap_destroy(sc->sc_bdt, sc->sc_rdp->tx_dm[i]);
	for (i = 0; i < CPSW_NRXDESCS; i++)
		bus_dmamap_destroy(sc->sc_bdt, sc->sc_rdp->rx_dm[i]);
	kmem_free(sc->sc_rdp, sizeof(*sc->sc_rdp));

	/* Unmap */
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, sc->sc_bss);


	return 0;
}

static void
cpsw_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct cpsw_softc * const sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct ethercom * const ec = &sc->sc_ec;
	struct ifnet * const ifp = &ec->ec_if;
	int error;
	u_int i;

	KERNHIST_INIT(cpswhist, 4096);

	sc->sc_dev = self;

	aprint_normal(": TI Layer 2 3-Port Switch\n");
	aprint_naive("\n");

	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, cpsw_tick, sc);

	prop_data_t eaprop = prop_dictionary_get(dict, "mac-address");
	if (eaprop == NULL) {
		/* grab mac_id0 from AM335x control module */
		uint32_t reg_lo, reg_hi;

		if (sitara_cm_reg_read_4(OMAP2SCM_MAC_ID0_LO, &reg_lo) == 0 &&
		    sitara_cm_reg_read_4(OMAP2SCM_MAC_ID0_HI, &reg_hi) == 0) {
			sc->sc_enaddr[0] = (reg_hi >>  0) & 0xff;
			sc->sc_enaddr[1] = (reg_hi >>  8) & 0xff;
			sc->sc_enaddr[2] = (reg_hi >> 16) & 0xff;
			sc->sc_enaddr[3] = (reg_hi >> 24) & 0xff;
			sc->sc_enaddr[4] = (reg_lo >>  0) & 0xff;
			sc->sc_enaddr[5] = (reg_lo >>  8) & 0xff;
		} else {
			aprint_error_dev(sc->sc_dev,
			    "using fake station address\n");
			/* 'N' happens to have the Local bit set */
			sc->sc_enaddr[0] = 'N';
			sc->sc_enaddr[1] = 'e';
			sc->sc_enaddr[2] = 't';
			sc->sc_enaddr[3] = 'B';
			sc->sc_enaddr[4] = 'S';
			sc->sc_enaddr[5] = 'D';
		}
	} else {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(eaprop),
		    ETHER_ADDR_LEN);
	}

	sc->sc_rxthih = intr_establish(oa->obio_intrbase + CPSW_INTROFF_RXTH,
	    IPL_VM, IST_LEVEL, cpsw_rxthintr, sc);
	sc->sc_rxih = intr_establish(oa->obio_intrbase + CPSW_INTROFF_RX,
	    IPL_VM, IST_LEVEL, cpsw_rxintr, sc);
	sc->sc_txih = intr_establish(oa->obio_intrbase + CPSW_INTROFF_TX,
	    IPL_VM, IST_LEVEL, cpsw_txintr, sc);
	sc->sc_miscih = intr_establish(oa->obio_intrbase + CPSW_INTROFF_MISC,
	    IPL_VM, IST_LEVEL, cpsw_miscintr, sc);

	sc->sc_bst = oa->obio_iot;
	sc->sc_bss = oa->obio_size;
	sc->sc_bdt = oa->obio_dmat;

	error = bus_space_map(sc->sc_bst, oa->obio_addr, oa->obio_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(sc->sc_dev,
			"can't map registers: %d\n", error);
		return;
	}

	sc->sc_txdescs_pa = oa->obio_addr + CPSW_CPPI_RAM_TXDESCS_BASE;
	error = bus_space_subregion(sc->sc_bst, sc->sc_bsh,
	    CPSW_CPPI_RAM_TXDESCS_BASE, CPSW_CPPI_RAM_TXDESCS_SIZE,
	    &sc->sc_bsh_txdescs);
	if (error) {
		aprint_error_dev(sc->sc_dev,
			"can't subregion tx ring SRAM: %d\n", error);
		return;
	}
	aprint_debug_dev(sc->sc_dev, "txdescs at %p\n",
	    (void *)sc->sc_bsh_txdescs);

	sc->sc_rxdescs_pa = oa->obio_addr + CPSW_CPPI_RAM_RXDESCS_BASE;
	error = bus_space_subregion(sc->sc_bst, sc->sc_bsh,
	    CPSW_CPPI_RAM_RXDESCS_BASE, CPSW_CPPI_RAM_RXDESCS_SIZE,
	    &sc->sc_bsh_rxdescs);
	if (error) {
		aprint_error_dev(sc->sc_dev,
			"can't subregion rx ring SRAM: %d\n", error);
		return;
	}
	aprint_debug_dev(sc->sc_dev, "rxdescs at %p\n",
	    (void *)sc->sc_bsh_rxdescs);

	sc->sc_rdp = kmem_alloc(sizeof(*sc->sc_rdp), KM_SLEEP);
	KASSERT(sc->sc_rdp != NULL);

	for (i = 0; i < CPSW_NTXDESCS; i++) {
		if ((error = bus_dmamap_create(sc->sc_bdt, MCLBYTES,
		    CPSW_TXFRAGS, MCLBYTES, 0, 0,
		    &sc->sc_rdp->tx_dm[i])) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create tx DMA map: %d\n", error);
		}
		sc->sc_rdp->tx_mb[i] = NULL;
	}

	for (i = 0; i < CPSW_NRXDESCS; i++) {
		if ((error = bus_dmamap_create(sc->sc_bdt, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rdp->rx_dm[i])) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create rx DMA map: %d\n", error);
		}
		sc->sc_rdp->rx_mb[i] = NULL;
	}

	sc->sc_txpad = kmem_zalloc(ETHER_MIN_LEN, KM_SLEEP);
	KASSERT(sc->sc_txpad != NULL);
	bus_dmamap_create(sc->sc_bdt, ETHER_MIN_LEN, 1, ETHER_MIN_LEN, 0,
	    BUS_DMA_WAITOK, &sc->sc_txpad_dm);
	bus_dmamap_load(sc->sc_bdt, sc->sc_txpad_dm, sc->sc_txpad,
	    ETHER_MIN_LEN, NULL, BUS_DMA_WAITOK|BUS_DMA_WRITE);
	bus_dmamap_sync(sc->sc_bdt, sc->sc_txpad_dm, 0, ETHER_MIN_LEN,
	    BUS_DMASYNC_PREWRITE);

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_capabilities = 0;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = cpsw_start;
	ifp->if_ioctl = cpsw_ioctl;
	ifp->if_init = cpsw_init;
	ifp->if_stop = cpsw_stop;
	ifp->if_watchdog = cpsw_watchdog;
	IFQ_SET_READY(&ifp->if_snd);

	cpsw_stop(ifp, 0);

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = cpsw_mii_readreg;
	sc->sc_mii.mii_writereg = cpsw_mii_writereg;
	sc->sc_mii.mii_statchg = cpsw_mii_statchg;

	sc->sc_ec.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);

	/* Initialize MDIO */
	cpsw_write_4(sc, MDIOCONTROL,
	    MDIOCTL_ENABLE | MDIOCTL_FAULTENB | MDIOCTL_CLKDIV(0xff));
	/* Clear ALE */
	cpsw_write_4(sc, CPSW_ALE_CONTROL, ALECTL_CLEAR_TABLE);

	mii_attach(self, &sc->sc_mii, 0xffffffff, MII_PHY_ANY, 0, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		aprint_error_dev(self, "no PHY found!\n");
		sc->sc_phy_has_1000t = false;
		ifmedia_add(&sc->sc_mii.mii_media,
		    IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
	} else {
		sc->sc_phy_has_1000t = cpsw_phy_has_1000t(sc);
		if (sc->sc_phy_has_1000t) {
			aprint_normal_dev(sc->sc_dev, "1000baseT PHY found. "
			    "Setting RGMII Mode\n");
			/*
			 * Select the Interface RGMII Mode in the Control
			 * Module
			 */
			sitara_cm_reg_write_4(CPSW_GMII_SEL,
			    GMIISEL_GMII2_SEL(RGMII_MODE) |
			    GMIISEL_GMII1_SEL(RGMII_MODE));
		}

		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	/* The attach is successful. */
	sc->sc_attached = true;

	return;
}

static void
cpsw_start(struct ifnet *ifp)
{
	struct cpsw_softc * const sc = ifp->if_softc;
	struct cpsw_ring_data * const rdp = sc->sc_rdp;
	struct cpsw_cpdma_bd bd;
	uint32_t * const dw = bd.word;
	struct mbuf *m;
	bus_dmamap_t dm;
	u_int eopi __diagused = ~0;
	u_int seg;
	u_int txfree;
	int txstart = -1;
	int error;
	bool pad;
	u_int mlen;

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, 0, 0, 0);

	if (__predict_false((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) !=
	    IFF_RUNNING)) {
		return;
	}

	if (sc->sc_txnext >= sc->sc_txhead)
		txfree = CPSW_NTXDESCS - 1 + sc->sc_txhead - sc->sc_txnext;
	else
		txfree = sc->sc_txhead - sc->sc_txnext - 1;

	KERNHIST_LOG(cpswhist, "start txf %x txh %x txn %x txr %x\n",
	    txfree, sc->sc_txhead, sc->sc_txnext, sc->sc_txrun);

	while (txfree > 0) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		dm = rdp->tx_dm[sc->sc_txnext];

		error = bus_dmamap_load_mbuf(sc->sc_bdt, dm, m, BUS_DMA_NOWAIT);
		if (error == EFBIG) {
			device_printf(sc->sc_dev, "won't fit\n");
			IFQ_DEQUEUE(&ifp->if_snd, m);
			m_freem(m);
			ifp->if_oerrors++;
			continue;
		} else if (error != 0) {
			device_printf(sc->sc_dev, "error\n");
			break;
		}

		if (dm->dm_nsegs + 1 >= txfree) {
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_bdt, dm);
			break;
		}

		mlen = m_length(m);
		pad = mlen < CPSW_PAD_LEN;

		KASSERT(rdp->tx_mb[sc->sc_txnext] == NULL);
		rdp->tx_mb[sc->sc_txnext] = m;
		IFQ_DEQUEUE(&ifp->if_snd, m);

		bus_dmamap_sync(sc->sc_bdt, dm, 0, dm->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		if (txstart == -1)
			txstart = sc->sc_txnext;
		eopi = sc->sc_txnext;
		for (seg = 0; seg < dm->dm_nsegs; seg++) {
			dw[0] = cpsw_txdesc_paddr(sc,
			    TXDESC_NEXT(sc->sc_txnext));
			dw[1] = dm->dm_segs[seg].ds_addr;
			dw[2] = dm->dm_segs[seg].ds_len;
			dw[3] = 0;

			if (seg == 0)
				dw[3] |= CPDMA_BD_SOP | CPDMA_BD_OWNER |
				    MAX(mlen, CPSW_PAD_LEN);

			if ((seg == dm->dm_nsegs - 1) && !pad)
				dw[3] |= CPDMA_BD_EOP;

			cpsw_set_txdesc(sc, sc->sc_txnext, &bd);
			txfree--;
			eopi = sc->sc_txnext;
			sc->sc_txnext = TXDESC_NEXT(sc->sc_txnext);
		}
		if (pad) {
			dw[0] = cpsw_txdesc_paddr(sc,
			    TXDESC_NEXT(sc->sc_txnext));
			dw[1] = sc->sc_txpad_pa;
			dw[2] = CPSW_PAD_LEN - mlen;
			dw[3] = CPDMA_BD_EOP;

			cpsw_set_txdesc(sc, sc->sc_txnext, &bd);
			txfree--;
			eopi = sc->sc_txnext;
			sc->sc_txnext = TXDESC_NEXT(sc->sc_txnext);
		}

		bpf_mtap(ifp, m);
	}

	if (txstart >= 0) {
		ifp->if_timer = 5;
		/* terminate the new chain */
		KASSERT(eopi == TXDESC_PREV(sc->sc_txnext));
		cpsw_set_txdesc_next(sc, TXDESC_PREV(sc->sc_txnext), 0);
		KERNHIST_LOG(cpswhist, "CP %x HDP %x s %x e %x\n",
		    cpsw_read_4(sc, CPSW_CPDMA_TX_CP(0)),
		    cpsw_read_4(sc, CPSW_CPDMA_TX_HDP(0)), txstart, eopi);
		/* link the new chain on */
		cpsw_set_txdesc_next(sc, TXDESC_PREV(txstart),
		    cpsw_txdesc_paddr(sc, txstart));
		if (sc->sc_txeoq) {
			/* kick the dma engine */
			sc->sc_txeoq = false;
			cpsw_write_4(sc, CPSW_CPDMA_TX_HDP(0),
			    cpsw_txdesc_paddr(sc, txstart));
		}
	}
	KERNHIST_LOG(cpswhist, "end txf %x txh %x txn %x txr %x\n",
	    txfree, sc->sc_txhead, sc->sc_txnext, sc->sc_txrun);
}

static int
cpsw_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	const int s = splnet();
	int error = 0;

	switch (cmd) {
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			error = 0;
		}
		break;
	}

	splx(s);

	return error;
}

static void
cpsw_watchdog(struct ifnet *ifp)
{
	struct cpsw_softc *sc = ifp->if_softc;

	device_printf(sc->sc_dev, "device timeout\n");

	ifp->if_oerrors++;
	cpsw_init(ifp);
	cpsw_start(ifp);
}

static int
cpsw_mii_wait(struct cpsw_softc * const sc, int reg)
{
	u_int tries;

	for (tries = 0; tries < 1000; tries++) {
		if ((cpsw_read_4(sc, reg) & __BIT(31)) == 0)
			return 0;
		delay(1);
	}
	return ETIMEDOUT;
}

static int
cpsw_mii_readreg(device_t dev, int phy, int reg)
{
	struct cpsw_softc * const sc = device_private(dev);
	uint32_t v;

	if (cpsw_mii_wait(sc, MDIOUSERACCESS0) != 0)
		return 0;

	cpsw_write_4(sc, MDIOUSERACCESS0, (1 << 31) |
	    ((reg & 0x1F) << 21) | ((phy & 0x1F) << 16));

	if (cpsw_mii_wait(sc, MDIOUSERACCESS0) != 0)
		return 0;

	v = cpsw_read_4(sc, MDIOUSERACCESS0);
	if (v & __BIT(29))
		return v & 0xffff;
	else
		return 0;
}

static void
cpsw_mii_writereg(device_t dev, int phy, int reg, int val)
{
	struct cpsw_softc * const sc = device_private(dev);
	uint32_t v;

	KASSERT((val & 0xffff0000UL) == 0);

	if (cpsw_mii_wait(sc, MDIOUSERACCESS0) != 0)
		goto out;

	cpsw_write_4(sc, MDIOUSERACCESS0, (1 << 31) | (1 << 30) |
	    ((reg & 0x1F) << 21) | ((phy & 0x1F) << 16) | val);

	if (cpsw_mii_wait(sc, MDIOUSERACCESS0) != 0)
		goto out;

	v = cpsw_read_4(sc, MDIOUSERACCESS0);
	if ((v & __BIT(29)) == 0)
out:
		device_printf(sc->sc_dev, "%s error\n", __func__);

}

static void
cpsw_mii_statchg(struct ifnet *ifp)
{
	return;
}

static int
cpsw_new_rxbuf(struct cpsw_softc * const sc, const u_int i)
{
	struct cpsw_ring_data * const rdp = sc->sc_rdp;
	const u_int h = RXDESC_PREV(i);
	struct cpsw_cpdma_bd bd;
	uint32_t * const dw = bd.word;
	struct mbuf *m;
	int error = ENOBUFS;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		goto reuse;
	}

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		goto reuse;
	}

	/* We have a new buffer, prepare it for the ring. */

	if (rdp->rx_mb[i] != NULL)
		bus_dmamap_unload(sc->sc_bdt, rdp->rx_dm[i]);

	m->m_len = m->m_pkthdr.len = MCLBYTES;

	rdp->rx_mb[i] = m;

	error = bus_dmamap_load_mbuf(sc->sc_bdt, rdp->rx_dm[i], rdp->rx_mb[i],
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		device_printf(sc->sc_dev, "can't load rx DMA map %d: %d\n",
		    i, error);
	}

	bus_dmamap_sync(sc->sc_bdt, rdp->rx_dm[i],
	    0, rdp->rx_dm[i]->dm_mapsize, BUS_DMASYNC_PREREAD);

	error = 0;

reuse:
	/* (re-)setup the descriptor */
	dw[0] = 0;
	dw[1] = rdp->rx_dm[i]->dm_segs[0].ds_addr;
	dw[2] = MIN(0x7ff, rdp->rx_dm[i]->dm_segs[0].ds_len);
	dw[3] = CPDMA_BD_OWNER;

	cpsw_set_rxdesc(sc, i, &bd);
	/* and link onto ring */
	cpsw_set_rxdesc_next(sc, h, cpsw_rxdesc_paddr(sc, i));

	return error;
}

static int
cpsw_init(struct ifnet *ifp)
{
	struct cpsw_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;
	int i;

	cpsw_stop(ifp, 0);

	sc->sc_txnext = 0;
	sc->sc_txhead = 0;

	/* Reset wrapper */
	cpsw_write_4(sc, CPSW_WR_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_WR_SOFT_RESET) & 1);

	/* Reset SS */
	cpsw_write_4(sc, CPSW_SS_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_SS_SOFT_RESET) & 1);

	/* Clear table and enable ALE */
	cpsw_write_4(sc, CPSW_ALE_CONTROL,
	    ALECTL_ENABLE_ALE | ALECTL_CLEAR_TABLE);

	/* Reset and init Sliver port 1 and 2 */
	for (i = 0; i < CPSW_ETH_PORTS; i++) {
		uint32_t macctl;

		/* Reset */
		cpsw_write_4(sc, CPSW_SL_SOFT_RESET(i), 1);
		while(cpsw_read_4(sc, CPSW_SL_SOFT_RESET(i)) & 1);
		/* Set Slave Mapping */
		cpsw_write_4(sc, CPSW_SL_RX_PRI_MAP(i), 0x76543210);
		cpsw_write_4(sc, CPSW_PORT_P_TX_PRI_MAP(i+1), 0x33221100);
		cpsw_write_4(sc, CPSW_SL_RX_MAXLEN(i), 0x5f2);
		/* Set MAC Address */
		cpsw_write_4(sc, CPSW_PORT_P_SA_HI(i+1),
		    sc->sc_enaddr[0] | (sc->sc_enaddr[1] << 8) |
		    (sc->sc_enaddr[2] << 16) | (sc->sc_enaddr[3] << 24));
		cpsw_write_4(sc, CPSW_PORT_P_SA_LO(i+1),
		    sc->sc_enaddr[4] | (sc->sc_enaddr[5] << 8));

		/* Set MACCONTROL for ports 0,1 */
		macctl = SLMACCTL_FULLDUPLEX | SLMACCTL_GMII_EN |
		    SLMACCTL_IFCTL_A;
		if (sc->sc_phy_has_1000t)
			macctl |= SLMACCTL_GIG;
		cpsw_write_4(sc, CPSW_SL_MACCONTROL(i), macctl);

		/* Set ALE port to forwarding(3) */
		cpsw_write_4(sc, CPSW_ALE_PORTCTL(i+1), 3);
	}

	/* Set Host Port Mapping */
	cpsw_write_4(sc, CPSW_PORT_P0_CPDMA_TX_PRI_MAP, 0x76543210);
	cpsw_write_4(sc, CPSW_PORT_P0_CPDMA_RX_CH_MAP, 0);

	/* Set ALE port to forwarding(3) */
	cpsw_write_4(sc, CPSW_ALE_PORTCTL(0), 3);

	/* Initialize addrs */
	cpsw_ale_update_addresses(sc, 1);

	cpsw_write_4(sc, CPSW_SS_PTYPE, 0);
	cpsw_write_4(sc, CPSW_SS_STAT_PORT_EN, 7);

	cpsw_write_4(sc, CPSW_CPDMA_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_CPDMA_SOFT_RESET) & 1);

	for (i = 0; i < 8; i++) {
		cpsw_write_4(sc, CPSW_CPDMA_TX_HDP(i), 0);
		cpsw_write_4(sc, CPSW_CPDMA_RX_HDP(i), 0);
		cpsw_write_4(sc, CPSW_CPDMA_TX_CP(i), 0);
		cpsw_write_4(sc, CPSW_CPDMA_RX_CP(i), 0);
	}

	bus_space_set_region_4(sc->sc_bst, sc->sc_bsh_txdescs, 0, 0,
	    CPSW_CPPI_RAM_TXDESCS_SIZE/4);

	sc->sc_txhead = 0;
	sc->sc_txnext = 0;

	cpsw_write_4(sc, CPSW_CPDMA_RX_FREEBUFFER(0), 0);

	bus_space_set_region_4(sc->sc_bst, sc->sc_bsh_rxdescs, 0, 0,
	    CPSW_CPPI_RAM_RXDESCS_SIZE/4);
	/* Initialize RX Buffer Descriptors */
	cpsw_set_rxdesc_next(sc, RXDESC_PREV(0), 0);
	for (i = 0; i < CPSW_NRXDESCS; i++) {
		cpsw_new_rxbuf(sc, i);
	}
	sc->sc_rxhead = 0;

	/* turn off flow control */
	cpsw_write_4(sc, CPSW_SS_FLOW_CONTROL, 0);

	/* align layer 3 header to 32-bit */
	cpsw_write_4(sc, CPSW_CPDMA_RX_BUFFER_OFFSET, ETHER_ALIGN);

	/* Clear all interrupt Masks */
	cpsw_write_4(sc, CPSW_CPDMA_RX_INTMASK_CLEAR, 0xFFFFFFFF);
	cpsw_write_4(sc, CPSW_CPDMA_TX_INTMASK_CLEAR, 0xFFFFFFFF);

	/* Enable TX & RX DMA */
	cpsw_write_4(sc, CPSW_CPDMA_TX_CONTROL, 1);
	cpsw_write_4(sc, CPSW_CPDMA_RX_CONTROL, 1);

	/* Enable TX and RX interrupt receive for core 0 */
	cpsw_write_4(sc, CPSW_WR_C_TX_EN(0), 1);
	cpsw_write_4(sc, CPSW_WR_C_RX_EN(0), 1);
	cpsw_write_4(sc, CPSW_WR_C_MISC_EN(0), 0x1F);

	/* Enable host Error Interrupt */
	cpsw_write_4(sc, CPSW_CPDMA_DMA_INTMASK_SET, 2);

	/* Enable interrupts for TX and RX Channel 0 */
	cpsw_write_4(sc, CPSW_CPDMA_TX_INTMASK_SET, 1);
	cpsw_write_4(sc, CPSW_CPDMA_RX_INTMASK_SET, 1);

	/* Ack stalled irqs */
	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_RXTH);
	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_RX);
	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_TX);
	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_MISC);

	/* Initialize MDIO - ENABLE, PREAMBLE=0, FAULTENB, CLKDIV=0xFF */
	/* TODO Calculate MDCLK=CLK/(CLKDIV+1) */
	cpsw_write_4(sc, MDIOCONTROL,
	    MDIOCTL_ENABLE | MDIOCTL_FAULTENB | MDIOCTL_CLKDIV(0xff));

	mii_mediachg(mii);

	/* Write channel 0 RX HDP */
	cpsw_write_4(sc, CPSW_CPDMA_RX_HDP(0), cpsw_rxdesc_paddr(sc, 0));
	sc->sc_rxrun = true;
	sc->sc_rxeoq = false;

	sc->sc_txrun = true;
	sc->sc_txeoq = true;
	callout_schedule(&sc->sc_tick_ch, hz);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

static void
cpsw_stop(struct ifnet *ifp, int disable)
{
	struct cpsw_softc * const sc = ifp->if_softc;
	struct cpsw_ring_data * const rdp = sc->sc_rdp;
	u_int i;

	aprint_debug_dev(sc->sc_dev, "%s: ifp %p disable %d\n", __func__,
	    ifp, disable);

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);

	cpsw_write_4(sc, CPSW_CPDMA_TX_INTMASK_CLEAR, 1);
	cpsw_write_4(sc, CPSW_CPDMA_RX_INTMASK_CLEAR, 1);
	cpsw_write_4(sc, CPSW_WR_C_TX_EN(0), 0x0);
	cpsw_write_4(sc, CPSW_WR_C_RX_EN(0), 0x0);
	cpsw_write_4(sc, CPSW_WR_C_MISC_EN(0), 0x0);

	cpsw_write_4(sc, CPSW_CPDMA_TX_TEARDOWN, 0);
	cpsw_write_4(sc, CPSW_CPDMA_RX_TEARDOWN, 0);
	i = 0;
	while ((sc->sc_txrun || sc->sc_rxrun) && i < 10000) {
		delay(10);
		if ((sc->sc_txrun == true) && cpsw_txintr(sc) == 0)
			sc->sc_txrun = false;
		if ((sc->sc_rxrun == true) && cpsw_rxintr(sc) == 0)
			sc->sc_rxrun = false;
		i++;
	}
	//printf("%s toredown complete in %u\n", __func__, i);

	/* Reset wrapper */
	cpsw_write_4(sc, CPSW_WR_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_WR_SOFT_RESET) & 1);

	/* Reset SS */
	cpsw_write_4(sc, CPSW_SS_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_SS_SOFT_RESET) & 1);

	for (i = 0; i < CPSW_ETH_PORTS; i++) {
		cpsw_write_4(sc, CPSW_SL_SOFT_RESET(i), 1);
		while(cpsw_read_4(sc, CPSW_SL_SOFT_RESET(i)) & 1);
	}

	/* Reset CPDMA */
	cpsw_write_4(sc, CPSW_CPDMA_SOFT_RESET, 1);
	while(cpsw_read_4(sc, CPSW_CPDMA_SOFT_RESET) & 1);

	/* Release any queued transmit buffers. */
	for (i = 0; i < CPSW_NTXDESCS; i++) {
		bus_dmamap_unload(sc->sc_bdt, rdp->tx_dm[i]);
		m_freem(rdp->tx_mb[i]);
		rdp->tx_mb[i] = NULL;
	}

	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
	ifp->if_timer = 0;

	if (!disable)
		return;

	for (i = 0; i < CPSW_NRXDESCS; i++) {
		bus_dmamap_unload(sc->sc_bdt, rdp->rx_dm[i]);
		m_freem(rdp->rx_mb[i]);
		rdp->rx_mb[i] = NULL;
	}
}

static void
cpsw_tick(void *arg)
{
	struct cpsw_softc * const sc = arg;
	struct mii_data * const mii = &sc->sc_mii;
	const int s = splnet();

	mii_tick(mii);

	splx(s);

	callout_schedule(&sc->sc_tick_ch, hz);
}

static int
cpsw_rxthintr(void *arg)
{
	struct cpsw_softc * const sc = arg;

	/* this won't deassert the interrupt though */
	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_RXTH);

	return 1;
}

static int
cpsw_rxintr(void *arg)
{
	struct cpsw_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct cpsw_ring_data * const rdp = sc->sc_rdp;
	struct cpsw_cpdma_bd bd;
	const uint32_t * const dw = bd.word;
	bus_dmamap_t dm;
	struct mbuf *m;
	u_int i;
	u_int len, off;

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, 0, 0, 0);

	for (;;) {
		KASSERT(sc->sc_rxhead < CPSW_NRXDESCS);

		i = sc->sc_rxhead;
		KERNHIST_LOG(cpswhist, "rxhead %x CP %x\n", i,
		    cpsw_read_4(sc, CPSW_CPDMA_RX_CP(0)), 0, 0);
		dm = rdp->rx_dm[i];
		m = rdp->rx_mb[i];

		KASSERT(dm != NULL);
		KASSERT(m != NULL);

		cpsw_get_rxdesc(sc, i, &bd);

		if (ISSET(dw[3], CPDMA_BD_OWNER))
			break;

		if (ISSET(dw[3], CPDMA_BD_TDOWNCMPLT)) {
			sc->sc_rxrun = false;
			return 1;
		}

		if ((dw[3] & (CPDMA_BD_SOP|CPDMA_BD_EOP)) !=
		    (CPDMA_BD_SOP|CPDMA_BD_EOP)) {
			//Debugger();
		}

		bus_dmamap_sync(sc->sc_bdt, dm, 0, dm->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);

		if (cpsw_new_rxbuf(sc, i) != 0) {
			/* drop current packet, reuse buffer for new */
			ifp->if_ierrors++;
			goto next;
		}

		off = __SHIFTOUT(dw[2], (uint32_t)__BITS(26, 16));
		len = __SHIFTOUT(dw[3], (uint32_t)__BITS(10,  0));

		if (ISSET(dw[3], CPDMA_BD_PASSCRC))
			len -= ETHER_CRC_LEN;

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = len;
		m->m_data += off;

		ifp->if_ipackets++;

		bpf_mtap(ifp, m);

		if_percpuq_enqueue(ifp->if_percpuq, m);

next:
		sc->sc_rxhead = RXDESC_NEXT(sc->sc_rxhead);
		if (ISSET(dw[3], CPDMA_BD_EOQ)) {
			sc->sc_rxeoq = true;
			break;
		} else {
			sc->sc_rxeoq = false;
		}
		cpsw_write_4(sc, CPSW_CPDMA_RX_CP(0),
		    cpsw_rxdesc_paddr(sc, i));
	}

	if (sc->sc_rxeoq) {
		device_printf(sc->sc_dev, "rxeoq\n");
		//Debugger();
	}

	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_RX);

	return 1;
}

static int
cpsw_txintr(void *arg)
{
	struct cpsw_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct cpsw_ring_data * const rdp = sc->sc_rdp;
	struct cpsw_cpdma_bd bd;
	const uint32_t * const dw = bd.word;
	bool handled = false;
	uint32_t tx0_cp;
	u_int cpi;

	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED_5(cpswhist, sc, 0, 0, 0);

	KASSERT(sc->sc_txrun);

	KERNHIST_LOG(cpswhist, "before txnext %x txhead %x txrun %x\n",
	    sc->sc_txnext, sc->sc_txhead, sc->sc_txrun, 0);

	tx0_cp = cpsw_read_4(sc, CPSW_CPDMA_TX_CP(0));

	if (tx0_cp == 0xfffffffc) {
		/* Teardown, ack it */
		cpsw_write_4(sc, CPSW_CPDMA_TX_CP(0), 0xfffffffc);
		cpsw_write_4(sc, CPSW_CPDMA_TX_HDP(0), 0);
		sc->sc_txrun = false;
		return 0;
	}

	for (;;) {
		tx0_cp = cpsw_read_4(sc, CPSW_CPDMA_TX_CP(0));
		cpi = (tx0_cp - sc->sc_txdescs_pa) / sizeof(struct cpsw_cpdma_bd);
		KASSERT(sc->sc_txhead < CPSW_NTXDESCS);

		KERNHIST_LOG(cpswhist, "txnext %x txhead %x txrun %x cpi %x\n",
		    sc->sc_txnext, sc->sc_txhead, sc->sc_txrun, cpi);

		cpsw_get_txdesc(sc, sc->sc_txhead, &bd);

		if (dw[2] == 0) {
			//Debugger();
		}

		if (ISSET(dw[3], CPDMA_BD_SOP) == 0)
			goto next;

		if (ISSET(dw[3], CPDMA_BD_OWNER)) {
			printf("pwned %x %x %x\n", cpi, sc->sc_txhead,
			    sc->sc_txnext);
			break;
		}

		if (ISSET(dw[3], CPDMA_BD_TDOWNCMPLT)) {
			sc->sc_txrun = false;
			return 1;
		}

		bus_dmamap_sync(sc->sc_bdt, rdp->tx_dm[sc->sc_txhead],
		    0, rdp->tx_dm[sc->sc_txhead]->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_bdt, rdp->tx_dm[sc->sc_txhead]);

		m_freem(rdp->tx_mb[sc->sc_txhead]);
		rdp->tx_mb[sc->sc_txhead] = NULL;

		ifp->if_opackets++;

		handled = true;

		ifp->if_flags &= ~IFF_OACTIVE;

next:
		if (ISSET(dw[3], CPDMA_BD_EOP) && ISSET(dw[3], CPDMA_BD_EOQ)) {
			sc->sc_txeoq = true;
		}
		if (sc->sc_txhead == cpi) {
			cpsw_write_4(sc, CPSW_CPDMA_TX_CP(0),
			    cpsw_txdesc_paddr(sc, cpi));
			sc->sc_txhead = TXDESC_NEXT(sc->sc_txhead);
			break;
		}
		sc->sc_txhead = TXDESC_NEXT(sc->sc_txhead);
		if (ISSET(dw[3], CPDMA_BD_EOP) && ISSET(dw[3], CPDMA_BD_EOQ)) {
			sc->sc_txeoq = true;
			break;
		}
	}

	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_TX);

	if ((sc->sc_txnext != sc->sc_txhead) && sc->sc_txeoq) {
		if (cpsw_read_4(sc, CPSW_CPDMA_TX_HDP(0)) == 0) {
			sc->sc_txeoq = false;
			cpsw_write_4(sc, CPSW_CPDMA_TX_HDP(0),
			    cpsw_txdesc_paddr(sc, sc->sc_txhead));
		}
	}

	KERNHIST_LOG(cpswhist, "after txnext %x txhead %x txrun %x\n",
	    sc->sc_txnext, sc->sc_txhead, sc->sc_txrun, 0);
	KERNHIST_LOG(cpswhist, "CP %x HDP %x\n",
	    cpsw_read_4(sc, CPSW_CPDMA_TX_CP(0)),
	    cpsw_read_4(sc, CPSW_CPDMA_TX_HDP(0)), 0, 0);

	if (handled && sc->sc_txnext == sc->sc_txhead)
		ifp->if_timer = 0;

	if (handled)
		cpsw_start(ifp);

	return handled;
}

static int
cpsw_miscintr(void *arg)
{
	struct cpsw_softc * const sc = arg;
	uint32_t miscstat;
	uint32_t dmastat;
	uint32_t stat;

	miscstat = cpsw_read_4(sc, CPSW_WR_C_MISC_STAT(0));
	device_printf(sc->sc_dev, "%s %x FIRE\n", __func__, miscstat);

#define CPSW_MISC_HOST_PEND __BIT32(2)
#define CPSW_MISC_STAT_PEND __BIT32(3)

	if (ISSET(miscstat, CPSW_MISC_HOST_PEND)) {
		/* Host Error */
		dmastat = cpsw_read_4(sc, CPSW_CPDMA_DMA_INTSTAT_MASKED);
		printf("CPSW_CPDMA_DMA_INTSTAT_MASKED %x\n", dmastat);

		printf("rxhead %02x\n", sc->sc_rxhead);

		stat = cpsw_read_4(sc, CPSW_CPDMA_DMASTATUS);
		printf("CPSW_CPDMA_DMASTATUS %x\n", stat);
		stat = cpsw_read_4(sc, CPSW_CPDMA_TX_HDP(0));
		printf("CPSW_CPDMA_TX0_HDP %x\n", stat);
		stat = cpsw_read_4(sc, CPSW_CPDMA_TX_CP(0));
		printf("CPSW_CPDMA_TX0_CP %x\n", stat);
		stat = cpsw_read_4(sc, CPSW_CPDMA_RX_HDP(0));
		printf("CPSW_CPDMA_RX0_HDP %x\n", stat);
		stat = cpsw_read_4(sc, CPSW_CPDMA_RX_CP(0));
		printf("CPSW_CPDMA_RX0_CP %x\n", stat);

		//Debugger();

		cpsw_write_4(sc, CPSW_CPDMA_DMA_INTMASK_CLEAR, dmastat);
		dmastat = cpsw_read_4(sc, CPSW_CPDMA_DMA_INTSTAT_MASKED);
		printf("CPSW_CPDMA_DMA_INTSTAT_MASKED %x\n", dmastat);
	}

	cpsw_write_4(sc, CPSW_CPDMA_CPDMA_EOI_VECTOR, CPSW_INTROFF_MISC);

	return 1;
}

/*
 *
 * ALE support routines.
 *
 */

static void
cpsw_ale_entry_init(uint32_t *ale_entry)
{
	ale_entry[0] = ale_entry[1] = ale_entry[2] = 0;
}

static void
cpsw_ale_entry_set_mac(uint32_t *ale_entry, const uint8_t *mac)
{
	ale_entry[0] = mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];
	ale_entry[1] = mac[0] << 8 | mac[1];
}

static void
cpsw_ale_entry_set_bcast_mac(uint32_t *ale_entry)
{
	ale_entry[0] = 0xffffffff;
	ale_entry[1] = 0x0000ffff;
}

static void
cpsw_ale_entry_set(uint32_t *ale_entry, ale_entry_field_t field, uint32_t val)
{
	/* Entry type[61:60] is addr entry(1), Mcast fwd state[63:62] is fw(3)*/
	switch (field) {
	case ALE_ENTRY_TYPE:
		/* [61:60] */
		ale_entry[1] |= (val & 0x3) << 28;
		break;
	case ALE_MCAST_FWD_STATE:
		/* [63:62] */
		ale_entry[1] |= (val & 0x3) << 30;
		break;
	case ALE_PORT_MASK:
		/* [68:66] */
		ale_entry[2] |= (val & 0x7) << 2;
		break;
	case ALE_PORT_NUMBER:
		/* [67:66] */
		ale_entry[2] |= (val & 0x3) << 2;
		break;
	default:
		panic("Invalid ALE entry field: %d\n", field);
	}

	return;
}

static bool
cpsw_ale_entry_mac_match(const uint32_t *ale_entry, const uint8_t *mac)
{
	return (((ale_entry[1] >> 8) & 0xff) == mac[0]) &&
	    (((ale_entry[1] >> 0) & 0xff) == mac[1]) &&
	    (((ale_entry[0] >>24) & 0xff) == mac[2]) &&
	    (((ale_entry[0] >>16) & 0xff) == mac[3]) &&
	    (((ale_entry[0] >> 8) & 0xff) == mac[4]) &&
	    (((ale_entry[0] >> 0) & 0xff) == mac[5]);
}

static void
cpsw_ale_set_outgoing_mac(struct cpsw_softc *sc, int port, const uint8_t *mac)
{
	cpsw_write_4(sc, CPSW_PORT_P_SA_HI(port),
	    mac[3] << 24 | mac[2] << 16 | mac[1] << 8 | mac[0]);
	cpsw_write_4(sc, CPSW_PORT_P_SA_LO(port),
	    mac[5] << 8 | mac[4]);
}

static void
cpsw_ale_read_entry(struct cpsw_softc *sc, uint16_t idx, uint32_t *ale_entry)
{
	cpsw_write_4(sc, CPSW_ALE_TBLCTL, idx & 1023);
	ale_entry[0] = cpsw_read_4(sc, CPSW_ALE_TBLW0);
	ale_entry[1] = cpsw_read_4(sc, CPSW_ALE_TBLW1);
	ale_entry[2] = cpsw_read_4(sc, CPSW_ALE_TBLW2);
}

static void
cpsw_ale_write_entry(struct cpsw_softc *sc, uint16_t idx,
    const uint32_t *ale_entry)
{
	cpsw_write_4(sc, CPSW_ALE_TBLW0, ale_entry[0]);
	cpsw_write_4(sc, CPSW_ALE_TBLW1, ale_entry[1]);
	cpsw_write_4(sc, CPSW_ALE_TBLW2, ale_entry[2]);
	cpsw_write_4(sc, CPSW_ALE_TBLCTL, 1 << 31 | (idx & 1023));
}

static int
cpsw_ale_remove_all_mc_entries(struct cpsw_softc *sc)
{
	int i;
	uint32_t ale_entry[3];

	/* First two entries are link address and broadcast. */
	for (i = 2; i < CPSW_MAX_ALE_ENTRIES; i++) {
		cpsw_ale_read_entry(sc, i, ale_entry);
		if (((ale_entry[1] >> 28) & 3) == 1 && /* Address entry */
		    ((ale_entry[1] >> 8) & 1) == 1) { /* MCast link addr */
			ale_entry[0] = ale_entry[1] = ale_entry[2] = 0;
			cpsw_ale_write_entry(sc, i, ale_entry);
		}
	}
	return CPSW_MAX_ALE_ENTRIES;
}

static int
cpsw_ale_mc_entry_set(struct cpsw_softc *sc, uint8_t portmask, uint8_t *mac)
{
	int free_index = -1, matching_index = -1, i;
	uint32_t ale_entry[3];

	/* Find a matching entry or a free entry. */
	for (i = 0; i < CPSW_MAX_ALE_ENTRIES; i++) {
		cpsw_ale_read_entry(sc, i, ale_entry);

		/* Entry Type[61:60] is 0 for free entry */
		if (free_index < 0 && ((ale_entry[1] >> 28) & 3) == 0) {
			free_index = i;
		}

		if (cpsw_ale_entry_mac_match(ale_entry, mac)) {
			matching_index = i;
			break;
		}
	}

	if (matching_index < 0) {
		if (free_index < 0)
			return ENOMEM;
		i = free_index;
	}

	cpsw_ale_entry_init(ale_entry);

	cpsw_ale_entry_set_mac(ale_entry, mac);
	cpsw_ale_entry_set(ale_entry, ALE_ENTRY_TYPE, ALE_TYPE_ADDRESS);
	cpsw_ale_entry_set(ale_entry, ALE_MCAST_FWD_STATE, ALE_FWSTATE_FWONLY);
	cpsw_ale_entry_set(ale_entry, ALE_PORT_MASK, portmask);

	cpsw_ale_write_entry(sc, i, ale_entry);

	return 0;
}

static int
cpsw_ale_update_addresses(struct cpsw_softc *sc, int purge)
{
	uint8_t *mac = sc->sc_enaddr;
	uint32_t ale_entry[3];
	int i;
	struct ethercom * const ec = &sc->sc_ec;
	struct ether_multi *ifma;

	cpsw_ale_entry_init(ale_entry);
	/* Route incoming packets for our MAC address to Port 0 (host). */
	/* For simplicity, keep this entry at table index 0 in the ALE. */
	cpsw_ale_entry_set_mac(ale_entry, mac);
	cpsw_ale_entry_set(ale_entry, ALE_ENTRY_TYPE, ALE_TYPE_ADDRESS);
	cpsw_ale_entry_set(ale_entry, ALE_PORT_NUMBER, 0);
	cpsw_ale_write_entry(sc, 0, ale_entry);

	/* Set outgoing MAC Address for Ports 1 and 2. */
	for (i = CPSW_CPPI_PORTS; i < (CPSW_ETH_PORTS + CPSW_CPPI_PORTS); ++i)
		cpsw_ale_set_outgoing_mac(sc, i, mac);

	/* Keep the broadcast address at table entry 1. */
	cpsw_ale_entry_init(ale_entry);
	cpsw_ale_entry_set_bcast_mac(ale_entry);
	cpsw_ale_entry_set(ale_entry, ALE_ENTRY_TYPE, ALE_TYPE_ADDRESS);
	cpsw_ale_entry_set(ale_entry, ALE_MCAST_FWD_STATE, ALE_FWSTATE_FWONLY);
	cpsw_ale_entry_set(ale_entry, ALE_PORT_MASK, ALE_PORT_MASK_ALL);
	cpsw_ale_write_entry(sc, 1, ale_entry);

	/* SIOCDELMULTI doesn't specify the particular address
	   being removed, so we have to remove all and rebuild. */
	if (purge)
		cpsw_ale_remove_all_mc_entries(sc);

	/* Set other multicast addrs desired. */
	LIST_FOREACH(ifma, &ec->ec_multiaddrs, enm_list) {
		cpsw_ale_mc_entry_set(sc, ALE_PORT_MASK_ALL, ifma->enm_addrlo);
	}

	return 0;
}
