/*	$NetBSD: if_iwi.c,v 1.11 2005/07/07 00:43:01 dyoung Exp $  */

/*-
 * Copyright (c) 2004, 2005
 *      Damien Bergamini <damien.bergamini@free.fr>. All rights reserved.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_iwi.c,v 1.11 2005/07/07 00:43:01 dyoung Exp $");

/*-
 * Intel(R) PRO/Wireless 2200BG/2915ABG driver
 * http://www.intel.com/network/connectivity/products/wireless/prowireless_mobile.htm
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/endian.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#include <crypto/arc4/arc4.h>

#include <dev/pci/if_iwireg.h>
#include <dev/pci/if_iwivar.h>

static const struct ieee80211_rateset iwi_rateset_11a =
	{ 8, { 12, 18, 24, 36, 48, 72, 96, 108 } };

static const struct ieee80211_rateset iwi_rateset_11b =
	{ 4, { 2, 4, 11, 22 } };

static const struct ieee80211_rateset iwi_rateset_11g =
	{ 12, { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 } };

static int iwi_match(struct device *, struct cfdata *, void *);
static void iwi_attach(struct device *, struct device *, void *);
static int iwi_detach(struct device *, int);
static int iwi_dma_alloc(struct iwi_softc *);
static void iwi_release(struct iwi_softc *);
static int iwi_media_change(struct ifnet *);
static void iwi_media_status(struct ifnet *, struct ifmediareq *);
static u_int16_t iwi_read_prom_word(struct iwi_softc *, u_int8_t);
static int iwi_newstate(struct ieee80211com *, enum ieee80211_state, int);
static void iwi_fix_channel(struct ieee80211com *, struct mbuf *);
static void iwi_frame_intr(struct iwi_softc *, struct iwi_rx_buf *, int,
    struct iwi_frame *);
static void iwi_notification_intr(struct iwi_softc *, struct iwi_rx_buf *,
    struct iwi_notif *);
static void iwi_rx_intr(struct iwi_softc *);
static void iwi_tx_intr(struct iwi_softc *);
static int iwi_intr(void *);
static int iwi_cmd(struct iwi_softc *, u_int8_t, void *, u_int8_t, int);
static int iwi_tx_start(struct ifnet *, struct mbuf *, struct ieee80211_node *);
static void iwi_start(struct ifnet *);
static void iwi_watchdog(struct ifnet *);
static int iwi_get_table0(struct iwi_softc *, u_int32_t *);
static int iwi_get_radio(struct iwi_softc *, int *);
static int iwi_ioctl(struct ifnet *, u_long, caddr_t);
static void iwi_stop_master(struct iwi_softc *);
static int iwi_reset(struct iwi_softc *);
static int iwi_load_ucode(struct iwi_softc *, void *, int);
static int iwi_load_firmware(struct iwi_softc *, void *, int);
static int iwi_cache_firmware(struct iwi_softc *, void *);
static void iwi_free_firmware(struct iwi_softc *);
static int iwi_config(struct iwi_softc *);
static int iwi_set_chan(struct iwi_softc *, struct ieee80211_channel *);
static int iwi_scan(struct iwi_softc *);
static int iwi_auth_and_assoc(struct iwi_softc *);
static int iwi_init(struct ifnet *);
static void iwi_stop(struct ifnet *, int);
static int iwi_key_alloc(struct ieee80211com *, const struct ieee80211_key *);

static __inline u_int8_t MEM_READ_1(struct iwi_softc *sc, u_int32_t addr)
{
	CSR_WRITE_4(sc, IWI_CSR_INDIRECT_ADDR, addr);
	return CSR_READ_1(sc, IWI_CSR_INDIRECT_DATA);
}

static __inline u_int32_t MEM_READ_4(struct iwi_softc *sc, u_int32_t addr)
{
	CSR_WRITE_4(sc, IWI_CSR_INDIRECT_ADDR, addr);
	return CSR_READ_4(sc, IWI_CSR_INDIRECT_DATA);
}

#ifdef IWI_DEBUG
#define DPRINTF(x)	if (iwi_debug > 0) printf x
#define DPRINTFN(n, x)	if (iwi_debug >= (n)) printf x
int iwi_debug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

CFATTACH_DECL(iwi, sizeof (struct iwi_softc), iwi_match, iwi_attach,
    iwi_detach, NULL);

static int
iwi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_2200BG ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_2915ABG_1)
		return 1;

	return 0;
}

/* Base Address Register */
#define IWI_PCI_BAR0	0x10

static void
iwi_attach(struct device *parent, struct device *self, void *aux)
{
	struct iwi_softc *sc = (struct iwi_softc *)self;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_if;
	struct pci_attach_args *pa = aux;
	const char *intrstr;
	char devinfo[256];
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_addr_t base;
	pci_intr_handle_t ih;
	pcireg_t data;
	u_int16_t val;
	int error, revision, i;

	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof devinfo);
	revision = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo, revision);

	/* clear device specific PCI configuration register 0x41 */
	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0x40);
	data &= ~0x0000ff00;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0x40, data);

	/* enable bus-mastering */
	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	data |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, data);

	/* map the register window */
	error = pci_mapreg_map(pa, IWI_PCI_BAR0, PCI_MAPREG_TYPE_MEM |
	    PCI_MAPREG_MEM_TYPE_32BIT, 0, &memt, &memh, &base, &sc->sc_sz);
	if (error != 0) {
		aprint_error("%s: could not map memory space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_st = memt;
	sc->sc_sh = memh;
	sc->sc_dmat = pa->pa_dmat;

	/* disable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, 0);

	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error("%s: could not map interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	intrstr = pci_intr_string(sc->sc_pct, ih);
	sc->sc_ih = pci_intr_establish(sc->sc_pct, ih, IPL_NET, iwi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: could not establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	if (iwi_reset(sc) != 0) {
		aprint_error("%s: could not reset adapter\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (iwi_dma_alloc(sc) != 0) {
		aprint_error("%s: could not allocate DMA resources\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_state = IEEE80211_S_INIT;

	/* set device capabilities */
	ic->ic_caps = IEEE80211_C_IBSS | IEEE80211_C_PMGT | IEEE80211_C_WEP |
	    IEEE80211_C_TXPMGT | IEEE80211_C_SHPREAMBLE | IEEE80211_C_MONITOR;

	/* read MAC address from EEPROM */
	val = iwi_read_prom_word(sc, IWI_EEPROM_MAC + 0);
	ic->ic_myaddr[0] = val >> 8;
	ic->ic_myaddr[1] = val & 0xff;
	val = iwi_read_prom_word(sc, IWI_EEPROM_MAC + 1);
	ic->ic_myaddr[2] = val >> 8;
	ic->ic_myaddr[3] = val & 0xff;
	val = iwi_read_prom_word(sc, IWI_EEPROM_MAC + 2);
	ic->ic_myaddr[4] = val >> 8;
	ic->ic_myaddr[5] = val & 0xff;

	aprint_normal("%s: 802.11 address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(ic->ic_myaddr));

	if (PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_PRO_WL_2200BG) {
		/* set supported .11a rates */
		ic->ic_sup_rates[IEEE80211_MODE_11A] = iwi_rateset_11a;

		/* set supported .11a channels */
		for (i = 36; i <= 64; i += 4) {
			ic->ic_channels[i].ic_freq =
			    ieee80211_ieee2mhz(i, IEEE80211_CHAN_5GHZ);
			ic->ic_channels[i].ic_flags = IEEE80211_CHAN_A;
		}
		for (i = 149; i <= 161; i += 4) {
			ic->ic_channels[i].ic_freq =
			    ieee80211_ieee2mhz(i, IEEE80211_CHAN_5GHZ);
			ic->ic_channels[i].ic_flags = IEEE80211_CHAN_A;
		}
	}

	/* set supported .11b and .11g rates */
	ic->ic_sup_rates[IEEE80211_MODE_11B] = iwi_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = iwi_rateset_11g;

	/* set supported .11b and .11g channels (1 through 14) */
	for (i = 1; i <= 14; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
		    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
	}

	/* default to authmode OPEN */
	sc->authmode = IEEE80211_AUTH_OPEN;

	/* IBSS channel undefined for now */
	ic->ic_ibss_chan = &ic->ic_channels[0];

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = iwi_init;
	ifp->if_stop = iwi_stop;
	ifp->if_ioctl = iwi_ioctl;
	ifp->if_start = iwi_start;
	ifp->if_watchdog = iwi_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ic);
	/* override state transition machine */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = iwi_newstate;
	ic->ic_crypto.cs_key_alloc = iwi_key_alloc;
	ieee80211_media_init(ic, iwi_media_change, iwi_media_status);

#if NBPFILTER > 0
	bpfattach2(ifp, DLT_IEEE802_11_RADIO,
	    sizeof (struct ieee80211_frame) + 64, &sc->sc_drvbpf);

	sc->sc_rxtap_len = sizeof sc->sc_rxtapu;
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(IWI_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof sc->sc_txtapu;
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(IWI_TX_RADIOTAP_PRESENT);
#endif
}

static int
iwi_detach(struct device* self, int flags)
{
	struct iwi_softc *sc = (struct iwi_softc *)self;
	struct ifnet *ifp = &sc->sc_if;

	iwi_stop(ifp, 1);
	iwi_free_firmware(sc);

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	ieee80211_ifdetach(&sc->sc_ic);
	if_detach(ifp);

	iwi_release(sc);

	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pct, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);

	return 0;
}

static int
iwi_dma_alloc(struct iwi_softc *sc)
{
	int i, nsegs, error;

	/*
	 * Allocate and map Tx ring
	 */
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE, 1,
	    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE, 0, BUS_DMA_NOWAIT,
	    &sc->tx_ring_map);
	if (error != 0) {
		aprint_error("%s: could not create tx ring DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE, PAGE_SIZE, 0,
	    &sc->tx_ring_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate tx ring DMA memory\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamem_map(sc->sc_dmat, &sc->tx_ring_seg, nsegs,
	    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE,
	    (caddr_t *)&sc->tx_desc, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not map tx ring DMA memory\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamap_load(sc->sc_dmat, sc->tx_ring_map, sc->tx_desc,
	    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE, NULL,
	    BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not load tx ring DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	memset(sc->tx_desc, 0, sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE);

	/*
	 * Allocate and map command ring
	 */
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE, 1,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE, 0,
	    BUS_DMA_NOWAIT, &sc->cmd_ring_map);
	if (error != 0) {
		aprint_error("%s: could not create command ring DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE, PAGE_SIZE, 0,
	    &sc->cmd_ring_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate command ring DMA memory\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamem_map(sc->sc_dmat, &sc->cmd_ring_seg, nsegs,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE,
	    (caddr_t *)&sc->cmd_desc, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not map command ring DMA memory\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	error = bus_dmamap_load(sc->sc_dmat, sc->cmd_ring_map, sc->cmd_desc,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE, NULL,
	    BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not load command ring DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	memset(sc->cmd_desc, 0,
	    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE);

	/*
	 * Allocate Tx buffers DMA maps
	 */
	for (i = 0; i < IWI_TX_RING_SIZE; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, IWI_MAX_NSEG,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &sc->tx_buf[i].map);
		if (error != 0) {
			aprint_error("%s: could not create tx buf DMA map",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
	}

	/*
	 * Allocate and map Rx buffers
	 */
	for (i = 0; i < IWI_RX_RING_SIZE; i++) {

		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES,
		    0, BUS_DMA_NOWAIT, &sc->rx_buf[i].map);
		if (error != 0) {
			aprint_error("%s: could not create rx buf DMA map",
			    sc->sc_dev.dv_xname);
			goto fail;
		}

		MGETHDR(sc->rx_buf[i].m, M_DONTWAIT, MT_DATA);
		if (sc->rx_buf[i].m == NULL) {
			aprint_error("%s: could not allocate rx mbuf\n",
			    sc->sc_dev.dv_xname);
			error = ENOMEM;
			goto fail;
		}

		MCLGET(sc->rx_buf[i].m, M_DONTWAIT);
		if (!(sc->rx_buf[i].m->m_flags & M_EXT)) {
			m_freem(sc->rx_buf[i].m);
			aprint_error("%s: could not allocate rx mbuf cluster\n",
			    sc->sc_dev.dv_xname);
			error = ENOMEM;
			goto fail;
		}

		error = bus_dmamap_load(sc->sc_dmat, sc->rx_buf[i].map,
		    mtod(sc->rx_buf[i].m, void *), MCLBYTES, NULL,
		    BUS_DMA_NOWAIT);
		if (error != 0) {
			aprint_error("%s: could not load rx buffer DMA map\n",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
	}

	return 0;

fail:	iwi_release(sc);
	return error;
}

static void
iwi_release(struct iwi_softc *sc)
{
	int i;

	if (sc->tx_ring_map != NULL) {
		if (sc->tx_desc != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->tx_ring_map);
			bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->tx_desc,
			    sizeof (struct iwi_tx_desc) * IWI_TX_RING_SIZE);
			bus_dmamem_free(sc->sc_dmat, &sc->tx_ring_seg, 1);
		}
		bus_dmamap_destroy(sc->sc_dmat, sc->tx_ring_map);
	}

	if (sc->cmd_ring_map != NULL) {
		if (sc->cmd_desc != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->cmd_ring_map);
			bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->cmd_desc,
			    sizeof (struct iwi_cmd_desc) * IWI_CMD_RING_SIZE);
			bus_dmamem_free(sc->sc_dmat, &sc->cmd_ring_seg, 1);
		}
		bus_dmamap_destroy(sc->sc_dmat, sc->cmd_ring_map);
	}

	for (i = 0; i < IWI_TX_RING_SIZE; i++) {
		if (sc->tx_buf[i].m != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->tx_buf[i].map);
			m_freem(sc->tx_buf[i].m);
		}
		bus_dmamap_destroy(sc->sc_dmat, sc->tx_buf[i].map);
	}

	for (i = 0; i < IWI_RX_RING_SIZE; i++) {
		if (sc->rx_buf[i].m != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->rx_buf[i].map);
			m_freem(sc->rx_buf[i].m);
		}
		bus_dmamap_destroy(sc->sc_dmat, sc->rx_buf[i].map);
	}
}

static int
iwi_key_alloc(struct ieee80211com *ic, const struct ieee80211_key *k)
{
	if (k >= &ic->ic_nw_keys[0] && k < &ic->ic_nw_keys[IEEE80211_WEP_NKID])
		return k - ic->ic_nw_keys;

	return IEEE80211_KEYIX_NONE;
}

static int
iwi_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return error;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
		iwi_init(ifp);

	return 0;
}

static void
iwi_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
#define N(a)	(sizeof (a) / sizeof (a[0]))
	static const struct {
		u_int32_t	val;
		int		rate;
	} rates[] = {
		{ IWI_RATE_DS1,      2 },
		{ IWI_RATE_DS2,      4 },
		{ IWI_RATE_DS5,     11 },
		{ IWI_RATE_DS11,    22 },
		{ IWI_RATE_OFDM6,   12 },
		{ IWI_RATE_OFDM9,   18 },
		{ IWI_RATE_OFDM12,  24 },
		{ IWI_RATE_OFDM18,  36 },
		{ IWI_RATE_OFDM24,  48 },
		{ IWI_RATE_OFDM36,  72 },
		{ IWI_RATE_OFDM48,  96 },
		{ IWI_RATE_OFDM54, 108 },
	};
	u_int32_t val;
	int rate, i;

	imr->ifm_status = IFM_AVALID;
	imr->ifm_active = IFM_IEEE80211;
	if (ic->ic_state == IEEE80211_S_RUN)
		imr->ifm_status |= IFM_ACTIVE;

	/* read current transmission rate from adapter */
	val = CSR_READ_4(sc, IWI_CSR_CURRENT_TX_RATE);

	/* convert rate to 802.11 rate */
	for (i = 0; i < N(rates) && rates[i].val != val; i++);
	rate = (i < N(rates)) ? rates[i].rate : 0;

	imr->ifm_active |= ieee80211_rate2media(ic, rate, ic->ic_curmode);
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		break;

	case IEEE80211_M_IBSS:
		imr->ifm_active |= IFM_IEEE80211_ADHOC;
		break;

	case IEEE80211_M_MONITOR:
		imr->ifm_active |= IFM_IEEE80211_MONITOR;
		break;

	case IEEE80211_M_AHDEMO:
	case IEEE80211_M_HOSTAP:
		/* should not get there */
		break;
	}
#undef N
}

static int
iwi_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct iwi_softc *sc = ic->ic_ifp->if_softc;

	switch (nstate) {
	case IEEE80211_S_SCAN:
		iwi_scan(sc);
		break;

	case IEEE80211_S_AUTH:
		iwi_auth_and_assoc(sc);
		break;

	case IEEE80211_S_RUN:
		if (ic->ic_opmode == IEEE80211_M_IBSS)
			ieee80211_new_state(ic, IEEE80211_S_AUTH, -1);
		else if (ic->ic_opmode == IEEE80211_M_MONITOR)
			iwi_set_chan(sc, ic->ic_ibss_chan);
		break;

	case IEEE80211_S_ASSOC:
	case IEEE80211_S_INIT:
		break;
	}

	ic->ic_state = nstate;
	return 0;
}

/*
 * Read 16 bits at address 'addr' from the serial EEPROM.
 * DON'T PLAY WITH THIS CODE UNLESS YOU KNOW *EXACTLY* WHAT YOU'RE DOING!
 */
static u_int16_t
iwi_read_prom_word(struct iwi_softc *sc, u_int8_t addr)
{
	u_int32_t tmp;
	u_int16_t val;
	int n;

	/* Clock C once before the first command */
	IWI_EEPROM_CTL(sc, 0);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_C);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S);

	/* Write start bit (1) */
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_D);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_D | IWI_EEPROM_C);

	/* Write READ opcode (10) */
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_D);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_D | IWI_EEPROM_C);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_C);

	/* Write address A7-A0 */
	for (n = 7; n >= 0; n--) {
		IWI_EEPROM_CTL(sc, IWI_EEPROM_S |
		    (((addr >> n) & 1) << IWI_EEPROM_SHIFT_D));
		IWI_EEPROM_CTL(sc, IWI_EEPROM_S |
		    (((addr >> n) & 1) << IWI_EEPROM_SHIFT_D) | IWI_EEPROM_C);
	}

	IWI_EEPROM_CTL(sc, IWI_EEPROM_S);

	/* Read data Q15-Q0 */
	val = 0;
	for (n = 15; n >= 0; n--) {
		IWI_EEPROM_CTL(sc, IWI_EEPROM_S | IWI_EEPROM_C);
		IWI_EEPROM_CTL(sc, IWI_EEPROM_S);
		tmp = MEM_READ_4(sc, IWI_MEM_EEPROM_CTL);
		val |= ((tmp & IWI_EEPROM_Q) >> IWI_EEPROM_SHIFT_Q) << n;
	}

	IWI_EEPROM_CTL(sc, 0);

	/* Clear Chip Select and clock C */
	IWI_EEPROM_CTL(sc, IWI_EEPROM_S);
	IWI_EEPROM_CTL(sc, 0);
	IWI_EEPROM_CTL(sc, IWI_EEPROM_C);

	return be16toh(val);
}

/*
 * XXX: Hack to set the current channel to the value advertised in beacons or
 * probe responses. Only used during AP detection.
 */
static void
iwi_fix_channel(struct ieee80211com *ic, struct mbuf *m)
{
	struct ieee80211_frame *wh;
	u_int8_t subtype;
	u_int8_t *frm, *efrm;

	wh = mtod(m, struct ieee80211_frame *);

	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT)
		return;

	subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

	if (subtype != IEEE80211_FC0_SUBTYPE_BEACON &&
	    subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP)
		return;

	frm = (u_int8_t *)(wh + 1);
	efrm = mtod(m, u_int8_t *) + m->m_len;

	frm += 12;	/* skip tstamp, bintval and capinfo fields */
	while (frm < efrm) {
		if (*frm == IEEE80211_ELEMID_DSPARMS)
#if IEEE80211_CHAN_MAX < 255
		if (frm[2] <= IEEE80211_CHAN_MAX)
#endif
			ic->ic_bss->ni_chan = &ic->ic_channels[frm[2]];

		frm += frm[1] + 2;
	}
}

static void
iwi_frame_intr(struct iwi_softc *sc, struct iwi_rx_buf *buf, int i,
    struct iwi_frame *frame)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;
	struct ieee80211_frame_min *wh;
	struct ieee80211_node *ni;
	int error;

	DPRINTFN(5, ("RX!DATA!%u!%u!%u\n", le16toh(frame->len), frame->chan,
	    frame->rssi_dbm));

	bus_dmamap_sync(sc->sc_dmat, buf->map, sizeof (struct iwi_hdr),
	    sizeof (struct iwi_frame) + le16toh(frame->len),
	    BUS_DMASYNC_POSTREAD);

	if (le16toh(frame->len) < sizeof (struct ieee80211_frame_min) ||
	    le16toh(frame->len) > MCLBYTES) {
		aprint_error("%s: bad frame length\n", sc->sc_dev.dv_xname);
	}

	bus_dmamap_unload(sc->sc_dmat, buf->map);

	/* Finalize mbuf */
	m = buf->m;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = sizeof (struct iwi_hdr) +
	    sizeof (struct iwi_frame) + le16toh(frame->len);

	m_adj(m, sizeof (struct iwi_hdr) + sizeof (struct iwi_frame));

	wh = mtod(m, struct ieee80211_frame_min *);

#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct iwi_rx_radiotap_header *tap = &sc->sc_rxtap;

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_txtap_len, m);
	}
#endif

	if (ic->ic_state == IEEE80211_S_SCAN)
		iwi_fix_channel(ic, m);

	ni = ieee80211_find_rxnode(ic, wh);

	/* Send the frame to the upper layer */
	ieee80211_input(ic, m, ni, IWI_RSSIDBM2RAW(frame->rssi_dbm), 0);

	ieee80211_free_node(ni);

	MGETHDR(buf->m, M_DONTWAIT, MT_DATA);
	if (buf->m == NULL) {
		aprint_error("%s: could not allocate rx mbuf\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	MCLGET(buf->m, M_DONTWAIT);
	if (!(buf->m->m_flags & M_EXT)) {
		aprint_error("%s: could not allocate rx mbuf cluster\n",
		    sc->sc_dev.dv_xname);
		m_freem(buf->m);
		buf->m = NULL;
		return;
	}

	error = bus_dmamap_load(sc->sc_dmat, buf->map, mtod(buf->m, void *),
	    MCLBYTES, NULL, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not load rx buf DMA map\n",
		    sc->sc_dev.dv_xname);
		m_freem(buf->m);
		buf->m = NULL;
		return;
	}

	CSR_WRITE_4(sc, IWI_CSR_RX_BASE + i * 4, buf->map->dm_segs[0].ds_addr);
}

static void
iwi_notification_intr(struct iwi_softc *sc, struct iwi_rx_buf *buf,
    struct iwi_notif *notif)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwi_notif_scan_channel *chan;
	struct iwi_notif_scan_complete *scan;
	struct iwi_notif_authentication *auth;
	struct iwi_notif_association *assoc;

	bus_dmamap_sync(sc->sc_dmat, buf->map, sizeof (struct iwi_hdr),
	    sizeof (struct iwi_notif) + le16toh(notif->len),
	    BUS_DMASYNC_POSTREAD);

	switch (notif->type) {
	case IWI_NOTIF_TYPE_SCAN_CHANNEL:
		chan = (struct iwi_notif_scan_channel *)(notif + 1);

		DPRINTFN(2, ("Scan channel (%u)\n", chan->nchan));
		break;

	case IWI_NOTIF_TYPE_SCAN_COMPLETE:
		scan = (struct iwi_notif_scan_complete *)(notif + 1);

		DPRINTFN(2, ("Scan completed (%u, %u)\n", scan->nchan,
		    scan->status));

		/* monitor mode uses scan to set the channel ... */
		if (ic->ic_opmode != IEEE80211_M_MONITOR)
			ieee80211_end_scan(ic);
		else
			iwi_set_chan(sc, ic->ic_ibss_chan);
		break;

	case IWI_NOTIF_TYPE_AUTHENTICATION:
		auth = (struct iwi_notif_authentication *)(notif + 1);

		DPRINTFN(2, ("Authentication (%u)\n", auth->state));

		switch (auth->state) {
		case IWI_AUTHENTICATED:
			ieee80211_new_state(ic, IEEE80211_S_ASSOC, -1);
			break;

		case IWI_DEAUTHENTICATED:
			break;

		default:
			aprint_error("%s: unknown authentication state %u\n",
			    sc->sc_dev.dv_xname, auth->state);
		}
		break;

	case IWI_NOTIF_TYPE_ASSOCIATION:
		assoc = (struct iwi_notif_association *)(notif + 1);

		DPRINTFN(2, ("Association (%u, %u)\n", assoc->state,
		    assoc->status));

		switch (assoc->state) {
		case IWI_ASSOCIATED:
			ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
			break;

		case IWI_DEASSOCIATED:
			ieee80211_begin_scan(ic, 0);
			break;

		default:
			aprint_error("%s: unknown association state %u\n",
			    sc->sc_dev.dv_xname, assoc->state);
		}
		break;

	case IWI_NOTIF_TYPE_CALIBRATION:
	case IWI_NOTIF_TYPE_BEACON:
	case IWI_NOTIF_TYPE_NOISE:
		DPRINTFN(5, ("Notification (%u)\n", notif->type));
		break;

	default:
		aprint_error("%s: unknown notification type %u\n",
		    sc->sc_dev.dv_xname, notif->type);
	}
}

static void
iwi_rx_intr(struct iwi_softc *sc)
{
	struct iwi_rx_buf *buf;
	struct iwi_hdr *hdr;
	u_int32_t r, i;

	r = CSR_READ_4(sc, IWI_CSR_RX_READ_INDEX);

	for (i = (sc->rx_cur + 1) % IWI_RX_RING_SIZE; i != r;
	     i = (i + 1) % IWI_RX_RING_SIZE) {

		buf = &sc->rx_buf[i];

		bus_dmamap_sync(sc->sc_dmat, buf->map, 0,
		    sizeof (struct iwi_hdr), BUS_DMASYNC_POSTREAD);

		hdr = mtod(buf->m, struct iwi_hdr *);

		switch (hdr->type) {
		case IWI_HDR_TYPE_FRAME:
			iwi_frame_intr(sc, buf, i,
			    (struct iwi_frame *)(hdr + 1));
			break;

		case IWI_HDR_TYPE_NOTIF:
			iwi_notification_intr(sc, buf,
			    (struct iwi_notif *)(hdr + 1));
			break;

		default:
			aprint_error("%s: unknown hdr type %u\n",
			    sc->sc_dev.dv_xname, hdr->type);
		}
	}

	/* Tell the firmware what we have processed */
	sc->rx_cur = (r == 0) ? IWI_RX_RING_SIZE - 1 : r - 1;
	CSR_WRITE_4(sc, IWI_CSR_RX_WRITE_INDEX, sc->rx_cur);
}

static void
iwi_tx_intr(struct iwi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct iwi_tx_buf *buf;
	u_int32_t r, i;

	r = CSR_READ_4(sc, IWI_CSR_TX1_READ_INDEX);

	for (i = (sc->tx_old + 1) % IWI_TX_RING_SIZE; i != r;
	     i = (i + 1) % IWI_TX_RING_SIZE) {

		buf = &sc->tx_buf[i];

		bus_dmamap_unload(sc->sc_dmat, buf->map);
		m_freem(buf->m);
		buf->m = NULL;
		ieee80211_free_node(buf->ni);
		buf->ni = NULL;

		sc->tx_queued--;

		/* kill watchdog timer */
		sc->sc_tx_timer = 0;
	}

	/* Remember what the firmware has processed */
	sc->tx_old = (r == 0) ? IWI_TX_RING_SIZE - 1 : r - 1;

	/* Call start() since some buffer descriptors have been released */
	ifp->if_flags &= ~IFF_OACTIVE;
	(*ifp->if_start)(ifp);
}

static int
iwi_intr(void *arg)
{
	struct iwi_softc *sc = arg;
	u_int32_t r;

	if ((r = CSR_READ_4(sc, IWI_CSR_INTR)) == 0 || r == 0xffffffff)
		return 0;

	/* Disable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, 0);

	DPRINTFN(8, ("INTR!0x%08x\n", r));

	if (r & (IWI_INTR_FATAL_ERROR | IWI_INTR_PARITY_ERROR)) {
		aprint_error("%s: fatal error\n", sc->sc_dev.dv_xname);
		iwi_stop(&sc->sc_if, 1);
	}

	if (r & IWI_INTR_FW_INITED) {
		if (!(r & (IWI_INTR_FATAL_ERROR | IWI_INTR_PARITY_ERROR)))
			wakeup(sc);
	}

	if (r & IWI_INTR_RADIO_OFF) {
		DPRINTF(("radio transmitter off\n"));
		iwi_stop(&sc->sc_if, 1);
	}

	if (r & IWI_INTR_RX_TRANSFER)
		iwi_rx_intr(sc);

	if (r & IWI_INTR_CMD_TRANSFER)
		wakeup(sc);

	if (r & IWI_INTR_TX1_TRANSFER)
		iwi_tx_intr(sc);

	/* Acknowledge interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR, r);

	/* Re-enable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, IWI_INTR_MASK);

	return 1;
}

static int
iwi_cmd(struct iwi_softc *sc, u_int8_t type, void *data, u_int8_t len,
    int async)
{
	struct iwi_cmd_desc *desc;

	DPRINTFN(2, ("TX!CMD!%u!%u\n", type, len));

	desc = &sc->cmd_desc[sc->cmd_cur];
	desc->hdr.type = IWI_HDR_TYPE_COMMAND;
	desc->hdr.flags = IWI_HDR_FLAG_IRQ;
	desc->type = type;
	desc->len = len;
	memcpy(desc->data, data, len);

	bus_dmamap_sync(sc->sc_dmat, sc->cmd_ring_map,
	    sc->cmd_cur * sizeof (struct iwi_cmd_desc),
	    sizeof (struct iwi_cmd_desc), BUS_DMASYNC_PREWRITE);

	sc->cmd_cur = (sc->cmd_cur + 1) % IWI_CMD_RING_SIZE;
	CSR_WRITE_4(sc, IWI_CSR_CMD_WRITE_INDEX, sc->cmd_cur);

	return async ? 0 : tsleep(sc, 0, "iwicmd", hz);
}

static int
iwi_tx_start(struct ifnet *ifp, struct mbuf *m0, struct ieee80211_node *ni)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_frame *wh;
	struct iwi_tx_buf *buf;
	struct iwi_tx_desc *desc;
	struct mbuf *mnew;
	int error, i;

#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct iwi_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_chan_freq = htole16(ic->ic_bss->ni_chan->ic_freq);
		tap->wt_chan_flags = htole16(ic->ic_bss->ni_chan->ic_flags);

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_txtap_len, m0);
	}
#endif

	buf = &sc->tx_buf[sc->tx_cur];
	desc = &sc->tx_desc[sc->tx_cur];

	wh = mtod(m0, struct ieee80211_frame *);

	/* trim IEEE802.11 header */
	m_adj(m0, sizeof (struct ieee80211_frame));

	error = bus_dmamap_load_mbuf(sc->sc_dmat, buf->map, m0, BUS_DMA_NOWAIT);
	if (error != 0 && error != EFBIG) {
		aprint_error("%s: could not map mbuf (error %d)\n",
		    sc->sc_dev.dv_xname, error);
		m_freem(m0);
		return error;
	}
	if (error != 0) {
		/* too many fragments, linearize */

		MGETHDR(mnew, M_DONTWAIT, MT_DATA);
		if (mnew == NULL) {
			m_freem(m0);
			return ENOMEM;
		}

		M_COPY_PKTHDR(mnew, m0);
		MCLGET(mnew, M_DONTWAIT);
		if (!(mnew->m_flags & M_EXT)) {
			m_freem(m0);
			m_freem(mnew);
			return ENOMEM;
		}

		m_copydata(m0, 0, m0->m_pkthdr.len, mtod(mnew, caddr_t));
		m_freem(m0);
		mnew->m_len = mnew->m_pkthdr.len;
		m0 = mnew;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, buf->map, m0,
		    BUS_DMA_NOWAIT);
		if (error != 0) {
			aprint_error("%s: could not map mbuf (error %d)\n",
			    sc->sc_dev.dv_xname, error);
			m_freem(m0);
			return error;
		}
	}

	buf->m = m0;
	buf->ni = ni;

	desc->hdr.type = IWI_HDR_TYPE_DATA;
	desc->hdr.flags = IWI_HDR_FLAG_IRQ;
	desc->cmd = IWI_DATA_CMD_TX;
	desc->len = htole16(m0->m_pkthdr.len);
	desc->flags = 0;
	if (ic->ic_opmode == IEEE80211_M_IBSS) {
		if (!IEEE80211_IS_MULTICAST(wh->i_addr1))
			desc->flags |= IWI_DATA_FLAG_NEED_ACK;
	} else if (!IEEE80211_IS_MULTICAST(wh->i_addr3))
		desc->flags |= IWI_DATA_FLAG_NEED_ACK;

	if (ic->ic_flags & IEEE80211_F_PRIVACY) {
		wh->i_fc[1] |= IEEE80211_FC1_WEP;
		desc->wep_txkey = ic->ic_def_txkey;
	} else
		desc->flags |= IWI_DATA_FLAG_NO_WEP;

	if (ic->ic_flags & IEEE80211_F_SHPREAMBLE)
		desc->flags |= IWI_DATA_FLAG_SHPREAMBLE;

	memcpy(&desc->wh, wh, sizeof (struct ieee80211_frame));
	desc->nseg = htole32(buf->map->dm_nsegs);
	for (i = 0; i < buf->map->dm_nsegs; i++) {
		desc->seg_addr[i] = htole32(buf->map->dm_segs[i].ds_addr);
		desc->seg_len[i]  = htole32(buf->map->dm_segs[i].ds_len);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->tx_ring_map,
	    sc->tx_cur * sizeof (struct iwi_tx_desc),
	    sizeof (struct iwi_tx_desc), BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(sc->sc_dmat, buf->map, 0, MCLBYTES,
	    BUS_DMASYNC_PREWRITE);

	DPRINTFN(5, ("TX!DATA!%u!%u\n", desc->len, desc->nseg));

	/* Inform firmware about this new packet */
	sc->tx_queued++;
	sc->tx_cur = (sc->tx_cur + 1) % IWI_TX_RING_SIZE;
	CSR_WRITE_4(sc, IWI_CSR_TX1_WRITE_INDEX, sc->tx_cur);

	return 0;
}

static void
iwi_start(struct ifnet *ifp)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct mbuf *m0;
	struct ieee80211_node *ni;

	if (ic->ic_state != IEEE80211_S_RUN)
		return;

	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		if (sc->tx_queued >= IWI_TX_RING_SIZE - 4) {
			IF_PREPEND(&ifp->if_snd, m0);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

#if NBPFILTER > 0
		if (ifp->if_bpf != NULL)
			bpf_mtap(ifp->if_bpf, m0);
#endif

		if ((ni = ieee80211_find_txnode(ic,
		    mtod(m0, struct ether_header *)->ether_dhost)) == NULL) {
			m_freem(m0);
			continue;
		}
		m0 = ieee80211_encap(ic, m0, ni);
		if (m0 == NULL)
			continue;

#if NBPFILTER > 0
		if (ic->ic_rawbpf != NULL)
			bpf_mtap(ic->ic_rawbpf, m0);
#endif

		if (iwi_tx_start(ifp, m0, ni) != 0) {
			if (ni != NULL)
				ieee80211_free_node(ni);
			break;
		}

		/* start watchdog timer */
		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

static void
iwi_watchdog(struct ifnet *ifp)
{
	struct iwi_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			aprint_error("%s: device timeout\n",
			    sc->sc_dev.dv_xname);
			iwi_stop(ifp, 1);
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(&sc->sc_ic);
}

static int
iwi_get_table0(struct iwi_softc *sc, u_int32_t *tbl)
{
	u_int32_t size, buf[128];

	if (!(sc->flags & IWI_FLAG_FW_INITED)) {
		memset(buf, 0, sizeof buf);
		return copyout(buf, tbl, sizeof buf);
	}

	size = min(CSR_READ_4(sc, IWI_CSR_TABLE0_SIZE), 128 - 1);
	CSR_READ_REGION_4(sc, IWI_CSR_TABLE0_BASE, &buf[1], size);

	return copyout(buf, tbl, sizeof buf);
}

static int
iwi_get_radio(struct iwi_softc *sc, int *ret)
{
	int val;

	val = (CSR_READ_4(sc, IWI_CSR_IO) & IWI_IO_RADIO_ENABLED) ? 1 : 0;
	return copyout(&val, ret, sizeof val);
}

static int
iwi_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ifreq *ifr;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING))
				iwi_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				iwi_stop(ifp, 1);
		}
		break;

	case SIOCGTABLE0:
		ifr = (struct ifreq *)data;
		error = iwi_get_table0(sc, (u_int32_t *)ifr->ifr_data);
		break;

	case SIOCGRADIO:
		ifr = (struct ifreq *)data;
		error = iwi_get_radio(sc, (int *)ifr->ifr_data);
		break;

	case SIOCSLOADFW:
		/* only super-user can do that! */
		if ((error = suser(curproc->p_ucred, &curproc->p_acflag)) != 0)
			break;

		ifr = (struct ifreq *)data;
		error = iwi_cache_firmware(sc, ifr->ifr_data);
		break;

	case SIOCSKILLFW:
		/* only super-user can do that! */
		if ((error = suser(curproc->p_ucred, &curproc->p_acflag)) != 0)
			break;

		iwi_stop(ifp, 1);
		iwi_free_firmware(sc);
		break;

	case SIOCG80211AUTH:
		((struct ieee80211_auth *)data)->i_authtype = sc->authmode;
		break;

	case SIOCS80211AUTH:
		/* only super-user can do that! */
		if ((error = suser(curproc->p_ucred, &curproc->p_acflag)) != 0)
			break;

		sc->authmode = ((struct ieee80211_auth *)data)->i_authtype;
		break;

	default:
		error = ieee80211_ioctl(&sc->sc_ic, cmd, data);
	}

	if (error == ENETRESET && cmd != SIOCADDMULTI) {
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING))
			iwi_init(ifp);
		error = 0;
	}

	splx(s);
	return error;
}

static void
iwi_stop_master(struct iwi_softc *sc)
{
	int ntries;

	/* Disable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, 0);

	CSR_WRITE_4(sc, IWI_CSR_RST, IWI_RST_STOP_MASTER);
	for (ntries = 0; ntries < 5; ntries++) {
		if (CSR_READ_4(sc, IWI_CSR_RST) & IWI_RST_MASTER_DISABLED)
			break;
		DELAY(10);
	}
	if (ntries == 5)
		aprint_error("%s: timeout waiting for master\n",
		    sc->sc_dev.dv_xname);

	CSR_WRITE_4(sc, IWI_CSR_RST, CSR_READ_4(sc, IWI_CSR_RST) |
	    IWI_RST_PRINCETON_RESET);

	sc->flags &= ~IWI_FLAG_FW_INITED;
}

static int
iwi_reset(struct iwi_softc *sc)
{
	int i, ntries;

	iwi_stop_master(sc);

	/* Move adapter to D0 state */
	CSR_WRITE_4(sc, IWI_CSR_CTL, CSR_READ_4(sc, IWI_CSR_CTL) |
	    IWI_CTL_INIT);

	/* Initialize Phase-Locked Level  (PLL) */
	CSR_WRITE_4(sc, IWI_CSR_READ_INT, IWI_READ_INT_INIT_HOST);

	/* Wait for clock stabilization */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (CSR_READ_4(sc, IWI_CSR_CTL) & IWI_CTL_CLOCK_READY)
			break;
		DELAY(200);
	}
	if (ntries == 1000)
		return EIO;

	CSR_WRITE_4(sc, IWI_CSR_RST, CSR_READ_4(sc, IWI_CSR_RST) |
	    IWI_RST_SW_RESET);

	DELAY(10);

	CSR_WRITE_4(sc, IWI_CSR_CTL, CSR_READ_4(sc, IWI_CSR_CTL) |
	    IWI_CTL_INIT);

	/* Clear NIC memory */
	CSR_WRITE_4(sc, IWI_CSR_AUTOINC_ADDR, 0);
	for (i = 0; i < 0xc000; i++)
		CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, 0);

	return 0;
}

static int
iwi_load_ucode(struct iwi_softc *sc, void *uc, int size)
{
	u_int16_t *w;
	int ntries, i;

	CSR_WRITE_4(sc, IWI_CSR_RST, CSR_READ_4(sc, IWI_CSR_RST) |
	    IWI_RST_STOP_MASTER);
	for (ntries = 0; ntries < 5; ntries++) {
		if (CSR_READ_4(sc, IWI_CSR_RST) & IWI_RST_MASTER_DISABLED)
			break;
		DELAY(10);
	}
	if (ntries == 5) {
		aprint_error("%s: timeout waiting for master\n",
		    sc->sc_dev.dv_xname);
		return EIO;
	}

	MEM_WRITE_4(sc, 0x3000e0, 0x80000000);
	DELAY(5000);
	CSR_WRITE_4(sc, IWI_CSR_RST, CSR_READ_4(sc, IWI_CSR_RST) &
	    ~IWI_RST_PRINCETON_RESET);
	DELAY(5000);
	MEM_WRITE_4(sc, 0x3000e0, 0);
	DELAY(1000);
	MEM_WRITE_4(sc, 0x300004, 1);
	DELAY(1000);
	MEM_WRITE_4(sc, 0x300004, 0);
	DELAY(1000);
	MEM_WRITE_1(sc, 0x200000, 0x00);
	MEM_WRITE_1(sc, 0x200000, 0x40);

	/* Adapter is buggy, we must set the address for each word */
	for (w = uc; size > 0; w++, size -= 2)
		MEM_WRITE_2(sc, 0x200010, *w);

	MEM_WRITE_1(sc, 0x200000, 0x00);
	MEM_WRITE_1(sc, 0x200000, 0x80);

	/* Wait until we get a response in the uc queue */
	for (ntries = 0; ntries < 100; ntries++) {
		if (MEM_READ_1(sc, 0x200000) & 1)
			break;
		DELAY(100);
	}
	if (ntries == 100) {
		aprint_error("%s: timeout waiting for ucode to initialize\n",
		    sc->sc_dev.dv_xname);
		return EIO;
	}

	/* Empty the uc queue or the firmware will not initialize properly */
	for (i = 0; i < 7; i++)
		MEM_READ_4(sc, 0x200004);

	MEM_WRITE_1(sc, 0x200000, 0x00);

	return 0;
}

/* macro to handle unaligned little endian data in firmware image */
#define GETLE32(p) ((p)[0] | (p)[1] << 8 | (p)[2] << 16 | (p)[3] << 24)
static int
iwi_load_firmware(struct iwi_softc *sc, void *fw, int size)
{
	bus_dmamap_t map;
	bus_dma_segment_t seg;
	caddr_t virtaddr;
	u_char *p, *end;
	u_int32_t sentinel, ctl, src, dst, sum, len, mlen;
	int ntries, nsegs, error;

	/* Allocate DMA memory for storing firmware image */
	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT, &map);
	if (error != 0) {
		aprint_error("%s: could not create firmware DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail1;
	}

	/*
	 * We cannot map fw directly because of some hardware constraints on
	 * the mapping address.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &seg, 1,
	    &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate firmware DMA memory\n",
		    sc->sc_dev.dv_xname);
		goto fail2;
	}

	error = bus_dmamem_map(sc->sc_dmat, &seg, nsegs, size, &virtaddr,
	    BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not load firmware DMA map\n",
		    sc->sc_dev.dv_xname);
		goto fail3;
	}

	error = bus_dmamap_load(sc->sc_dmat, map, virtaddr, size, NULL,
	    BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not load fw dma map\n",
		    sc->sc_dev.dv_xname);
		goto fail4;
	}

	/* Copy firmware image to DMA memory */
	memcpy(virtaddr, fw, size);

	/* Make sure the adapter will get up-to-date values */
	bus_dmamap_sync(sc->sc_dmat, map, 0, size, BUS_DMASYNC_PREWRITE);

	/* Tell the adapter where the command blocks are stored */
	MEM_WRITE_4(sc, 0x3000a0, 0x27000);

	/*
	 * Store command blocks into adapter's internal memory using register
	 * indirections. The adapter will read the firmware image through DMA
	 * using information stored in command blocks.
	 */
	src = map->dm_segs[0].ds_addr;
	p = virtaddr;
	end = p + size;
	CSR_WRITE_4(sc, IWI_CSR_AUTOINC_ADDR, 0x27000);

	while (p < end) {
		dst = GETLE32(p); p += 4; src += 4;
		len = GETLE32(p); p += 4; src += 4;
		p += len;

		while (len > 0) {
			mlen = min(len, IWI_CB_MAXDATALEN);

			ctl = IWI_CB_DEFAULT_CTL | mlen;
			sum = ctl ^ src ^ dst;

			/* Write a command block */
			CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, ctl);
			CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, src);
			CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, dst);
			CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, sum);

			src += mlen;
			dst += mlen;
			len -= mlen;
		}
	}

	/* Write a fictive final command block (sentinel) */
	sentinel = CSR_READ_4(sc, IWI_CSR_AUTOINC_ADDR);
	CSR_WRITE_4(sc, IWI_CSR_AUTOINC_DATA, 0);

	CSR_WRITE_4(sc, IWI_CSR_RST, CSR_READ_4(sc, IWI_CSR_RST) &
	    ~(IWI_RST_MASTER_DISABLED | IWI_RST_STOP_MASTER));

	/* Tell the adapter to start processing command blocks */
	MEM_WRITE_4(sc, 0x3000a4, 0x540100);

	/* Wait until the adapter has processed all command blocks */
	for (ntries = 0; ntries < 400; ntries++) {
		if (MEM_READ_4(sc, 0x3000d0) >= sentinel)
			break;
		DELAY(100);
	}
	if (ntries == 400) {
		aprint_error("%s: timeout processing cb\n",
		    sc->sc_dev.dv_xname);
		error = EIO;
		goto fail5;
	}

	/* We're done with command blocks processing */
	MEM_WRITE_4(sc, 0x3000a4, 0x540c00);

	/* Allow interrupts so we know when the firmware is inited */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, IWI_INTR_MASK);

	/* Tell the adapter to initialize the firmware */
	CSR_WRITE_4(sc, IWI_CSR_RST, 0);
	CSR_WRITE_4(sc, IWI_CSR_CTL, CSR_READ_4(sc, IWI_CSR_CTL) |
	    IWI_CTL_ALLOW_STANDBY);

	/* Wait at most one second for firmware initialization to complete */
	if ((error = tsleep(sc, 0, "iwiinit", hz)) != 0) {
		aprint_error("%s: timeout waiting for firmware initialization "
		    "to complete\n", sc->sc_dev.dv_xname);
		goto fail5;
	}

fail5:	bus_dmamap_sync(sc->sc_dmat, map, 0, size, BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, map);
fail4:	bus_dmamem_unmap(sc->sc_dmat, virtaddr, size);
fail3:	bus_dmamem_free(sc->sc_dmat, &seg, 1);
fail2:	bus_dmamap_destroy(sc->sc_dmat, map);

fail1:	return error;
}

/*
 * Store firmware into kernel memory so we can download it when we need to,
 * e.g when the adapter wakes up from suspend mode.
 */
static int
iwi_cache_firmware(struct iwi_softc *sc, void *data)
{
	struct iwi_firmware *kfw = &sc->fw;
	struct iwi_firmware ufw;
	int error;

	iwi_free_firmware(sc);

	if ((error = copyin(data, &ufw, sizeof ufw)) != 0)
		goto fail1;

	kfw->boot_size  = ufw.boot_size;
	kfw->ucode_size = ufw.ucode_size;
	kfw->main_size  = ufw.main_size;

	kfw->boot = malloc(kfw->boot_size, M_DEVBUF, M_NOWAIT);
	if (kfw->boot == NULL) {
		error = ENOMEM;
		goto fail1;
	}

	kfw->ucode = malloc(kfw->ucode_size, M_DEVBUF, M_NOWAIT);
	if (kfw->ucode == NULL) {
		error = ENOMEM;
		goto fail2;
	}

	kfw->main = malloc(kfw->main_size, M_DEVBUF, M_NOWAIT);
	if (kfw->main == NULL) {
		error = ENOMEM;
		goto fail3;
	}

	if ((error = copyin(ufw.boot, kfw->boot, kfw->boot_size)) != 0)
		goto fail4;

	if ((error = copyin(ufw.ucode, kfw->ucode, kfw->ucode_size)) != 0)
		goto fail4;

	if ((error = copyin(ufw.main, kfw->main, kfw->main_size)) != 0)
		goto fail4;

	DPRINTF(("Firmware cached: boot %u, ucode %u, main %u\n",
	    kfw->boot_size, kfw->ucode_size, kfw->main_size));

	sc->flags |= IWI_FLAG_FW_CACHED;

	return 0;

fail4:	free(kfw->boot, M_DEVBUF);
fail3:	free(kfw->ucode, M_DEVBUF);
fail2:	free(kfw->main, M_DEVBUF);
fail1:
	return error;
}

static void
iwi_free_firmware(struct iwi_softc *sc)
{
	if (!(sc->flags & IWI_FLAG_FW_CACHED))
		return;

	free(sc->fw.boot, M_DEVBUF);
	free(sc->fw.ucode, M_DEVBUF);
	free(sc->fw.main, M_DEVBUF);

	sc->flags &= ~IWI_FLAG_FW_CACHED;
}

static int
iwi_config(struct iwi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_if;
	struct iwi_configuration config;
	struct iwi_rateset rs;
	struct iwi_txpower power;
	struct ieee80211_key *k;
	struct iwi_wep_key wepkey;
	u_int32_t data;
	int error, i;

	IEEE80211_ADDR_COPY(ic->ic_myaddr, LLADDR(ifp->if_sadl));
	DPRINTF(("Setting MAC address to %s\n", ether_sprintf(ic->ic_myaddr)));
	error = iwi_cmd(sc, IWI_CMD_SET_MAC_ADDRESS, ic->ic_myaddr,
	    IEEE80211_ADDR_LEN, 0);
	if (error != 0)
		return error;

	memset(&config, 0, sizeof config);
	config.bluetooth_coexistence = 1;
	config.multicast_enabled = 1;
	config.noise_reported = 1;
	DPRINTF(("Configuring adapter\n"));
	error = iwi_cmd(sc, IWI_CMD_SET_CONFIGURATION, &config, sizeof config,
	    0);
	if (error != 0)
		return error;

	data = htole32(IWI_POWER_MODE_CAM);
	DPRINTF(("Setting power mode to %u\n", le32toh(data)));
	error = iwi_cmd(sc, IWI_CMD_SET_POWER_MODE, &data, sizeof data, 0);
	if (error != 0)
		return error;

	data = htole32(ic->ic_rtsthreshold);
	DPRINTF(("Setting RTS threshold to %u\n", le32toh(data)));
	error = iwi_cmd(sc, IWI_CMD_SET_RTS_THRESHOLD, &data, sizeof data, 0);
	if (error != 0)
		return error;

	if (ic->ic_opmode == IEEE80211_M_IBSS) {
		power.mode = IWI_MODE_11B;
		power.nchan = 11;
		for (i = 0; i < 11; i++) {
			power.chan[i].chan = i + 1;
			power.chan[i].power = IWI_TXPOWER_MAX;
		}
		DPRINTF(("Setting .11b channels tx power\n"));
		error = iwi_cmd(sc, IWI_CMD_SET_TX_POWER, &power, sizeof power,
		    0);
		if (error != 0)
			return error;

		power.mode = IWI_MODE_11G;
		DPRINTF(("Setting .11g channels tx power\n"));
		error = iwi_cmd(sc, IWI_CMD_SET_TX_POWER, &power, sizeof power,
		    0);
		if (error != 0)
			return error;
	}

	rs.mode = IWI_MODE_11G;
	rs.type = IWI_RATESET_TYPE_SUPPORTED;
	rs.nrates = ic->ic_sup_rates[IEEE80211_MODE_11G].rs_nrates;
	memcpy(rs.rates, ic->ic_sup_rates[IEEE80211_MODE_11G].rs_rates,
	    rs.nrates);
	DPRINTF(("Setting .11bg supported rates (%u)\n", rs.nrates));
	error = iwi_cmd(sc, IWI_CMD_SET_RATES, &rs, sizeof rs, 0);
	if (error != 0)
		return error;

	rs.mode = IWI_MODE_11A;
	rs.type = IWI_RATESET_TYPE_SUPPORTED;
	rs.nrates = ic->ic_sup_rates[IEEE80211_MODE_11A].rs_nrates;
	memcpy(rs.rates, ic->ic_sup_rates[IEEE80211_MODE_11A].rs_rates,
	    rs.nrates);
	DPRINTF(("Setting .11a supported rates (%u)\n", rs.nrates));
	error = iwi_cmd(sc, IWI_CMD_SET_RATES, &rs, sizeof rs, 0);
	if (error != 0)
		return error;

	data = htole32(arc4random());
	DPRINTF(("Setting initialization vector to %u\n", le32toh(data)));
	error = iwi_cmd(sc, IWI_CMD_SET_IV, &data, sizeof data, 0);
	if (error != 0)
		return error;

	if (ic->ic_flags & IEEE80211_F_PRIVACY) {
		k = ic->ic_nw_keys;
		for (i = 0; i < IEEE80211_WEP_NKID; i++, k++) {
			wepkey.cmd = IWI_WEP_KEY_CMD_SETKEY;
			wepkey.idx = i;
			wepkey.len = k->wk_keylen;
			memset(wepkey.key, 0, sizeof wepkey.key);
			memcpy(wepkey.key, k->wk_key, k->wk_keylen);
			DPRINTF(("Setting wep key index %u len %u\n",
			    wepkey.idx, wepkey.len));
			error = iwi_cmd(sc, IWI_CMD_SET_WEP_KEY, &wepkey,
			    sizeof wepkey, 0);
			if (error != 0)
				return error;
		}
	}

	/* Enable adapter */
	DPRINTF(("Enabling adapter\n"));
	return iwi_cmd(sc, IWI_CMD_ENABLE, NULL, 0, 0);
}

static int
iwi_set_chan(struct iwi_softc *sc, struct ieee80211_channel *chan)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwi_scan scan;

	bzero(&scan, sizeof scan);
	scan.type = IWI_SCAN_TYPE_PASSIVE;
	scan.intval = htole16(2000);
	scan.channels[0] = 1 | (IEEE80211_IS_CHAN_5GHZ(chan) ? IWI_CHAN_5GHZ :
		IWI_CHAN_2GHZ);
	scan.channels[1] = ieee80211_chan2ieee(ic, chan);

	DPRINTF(("Setting channel to %u\n", ieee80211_chan2ieee(ic, chan)));
	return iwi_cmd(sc, IWI_CMD_SCAN, &scan, sizeof scan, 1);
}


static int
iwi_scan(struct iwi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwi_scan scan;
	u_int8_t *p;
	int i, count;

	memset(&scan, 0, sizeof scan);
	scan.type = IWI_SCAN_TYPE_BROADCAST;
	scan.intval = htole16(40);

	p = scan.channels;
	count = 0;
	for (i = 0; i <= IEEE80211_CHAN_MAX; i++) {
		if (IEEE80211_IS_CHAN_5GHZ(&ic->ic_channels[i]) &&
		    isset(ic->ic_chan_active, i)) {
			*++p = i;
			count++;
		}
	}
	*(p - count) = IWI_CHAN_5GHZ | count;

	count = 0;
	for (i = 0; i <= IEEE80211_CHAN_MAX; i++) {
		if (IEEE80211_IS_CHAN_2GHZ(&ic->ic_channels[i]) &&
		    isset(ic->ic_chan_active, i)) {
			*++p = i;
			count++;
		}
	}
	*(p - count) = IWI_CHAN_2GHZ | count;

	DPRINTF(("Start scanning\n"));
	return iwi_cmd(sc, IWI_CMD_SCAN, &scan, sizeof scan, 1);
}

static int
iwi_auth_and_assoc(struct iwi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct iwi_configuration config;
	struct iwi_associate assoc;
	struct iwi_rateset rs;
	u_int32_t data;
	int error;

	if (IEEE80211_IS_CHAN_2GHZ(ni->ni_chan)) {
		/* enable b/g autodection */
		memset(&config, 0, sizeof config);
		config.bluetooth_coexistence = 1;
		config.multicast_enabled = 1;
		config.bg_autodetection = 1;
		config.noise_reported = 1;
		DPRINTF(("Configuring adapter\n"));
		error = iwi_cmd(sc, IWI_CMD_SET_CONFIGURATION, &config,
		    sizeof config, 1);
		if (error != 0)
			return error;
	}

#ifdef IWI_DEBUG
	if (iwi_debug > 0) {
		printf("Setting ESSID to ");
		ieee80211_print_essid(ni->ni_essid, ni->ni_esslen);
		printf("\n");
	}
#endif
	error = iwi_cmd(sc, IWI_CMD_SET_ESSID, ni->ni_essid, ni->ni_esslen, 1);
	if (error != 0)
		return error;

	/* the rate set has already been "negociated" */
	rs.mode = IEEE80211_IS_CHAN_5GHZ(ni->ni_chan) ? IWI_MODE_11A :
	    IWI_MODE_11G;
	rs.type = IWI_RATESET_TYPE_NEGOCIATED;
	rs.nrates = ni->ni_rates.rs_nrates;
	memcpy(rs.rates, ni->ni_rates.rs_rates, rs.nrates);
	DPRINTF(("Setting negociated rates (%u)\n", rs.nrates));
	error = iwi_cmd(sc, IWI_CMD_SET_RATES, &rs, sizeof rs, 1);
	if (error != 0)
		return error;

	data = htole32(ni->ni_rssi);
	DPRINTF(("Setting sensitivity to %d\n", (int8_t)ni->ni_rssi));
	error = iwi_cmd(sc, IWI_CMD_SET_SENSITIVITY, &data, sizeof data, 1);
	if (error != 0)
		return error;

	memset(&assoc, 0, sizeof assoc);
	assoc.mode = IEEE80211_IS_CHAN_5GHZ(ni->ni_chan) ? IWI_MODE_11A :
	    IWI_MODE_11G;
	assoc.chan = ieee80211_chan2ieee(ic, ni->ni_chan);
	if (sc->authmode == IEEE80211_AUTH_SHARED)
		assoc.auth = (ic->ic_def_txkey << 4) | IWI_AUTH_SHARED;
	memcpy(assoc.tstamp, ni->ni_tstamp.data, 8);
	assoc.capinfo = htole16(ni->ni_capinfo);
	assoc.lintval = htole16(ic->ic_lintval);
	assoc.intval = htole16(ni->ni_intval);
	IEEE80211_ADDR_COPY(assoc.bssid, ni->ni_bssid);
	IEEE80211_ADDR_COPY(assoc.dst, ni->ni_bssid);
	DPRINTF(("Trying to associate to %s channel %u auth %u\n",
	    ether_sprintf(assoc.bssid), assoc.chan, assoc.auth));
	return iwi_cmd(sc, IWI_CMD_ASSOCIATE, &assoc, sizeof assoc, 1);
}

static int
iwi_init(struct ifnet *ifp)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwi_firmware *fw = &sc->fw;
	int i, error;

	/* exit immediately if firmware has not been ioctl'd */
	if (!(sc->flags & IWI_FLAG_FW_CACHED)) {
		ifp->if_flags &= ~IFF_UP;
		return EIO;
	}

	if ((error = iwi_reset(sc)) != 0) {
		aprint_error("%s: could not reset adapter\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	if ((error = iwi_load_firmware(sc, fw->boot, fw->boot_size)) != 0) {
		aprint_error("%s: could not load boot firmware\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	if ((error = iwi_load_ucode(sc, fw->ucode, fw->ucode_size)) != 0) {
		aprint_error("%s: could not load microcode\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	iwi_stop_master(sc);

	sc->tx_cur = 0;
	sc->tx_queued = 0;
	sc->tx_old = IWI_TX_RING_SIZE - 1;
	sc->cmd_cur = 0;
	sc->rx_cur = IWI_RX_RING_SIZE - 1;

	CSR_WRITE_4(sc, IWI_CSR_CMD_BASE, sc->cmd_ring_map->dm_segs[0].ds_addr);
	CSR_WRITE_4(sc, IWI_CSR_CMD_SIZE, IWI_CMD_RING_SIZE);
	CSR_WRITE_4(sc, IWI_CSR_CMD_READ_INDEX, 0);
	CSR_WRITE_4(sc, IWI_CSR_CMD_WRITE_INDEX, sc->cmd_cur);

	CSR_WRITE_4(sc, IWI_CSR_TX1_BASE, sc->tx_ring_map->dm_segs[0].ds_addr);
	CSR_WRITE_4(sc, IWI_CSR_TX1_SIZE, IWI_TX_RING_SIZE);
	CSR_WRITE_4(sc, IWI_CSR_TX1_READ_INDEX, 0);
	CSR_WRITE_4(sc, IWI_CSR_TX1_WRITE_INDEX, sc->tx_cur);

	CSR_WRITE_4(sc, IWI_CSR_TX2_BASE, sc->tx_ring_map->dm_segs[0].ds_addr);
	CSR_WRITE_4(sc, IWI_CSR_TX2_SIZE, IWI_TX_RING_SIZE);
	CSR_WRITE_4(sc, IWI_CSR_TX2_READ_INDEX, 0);
	CSR_WRITE_4(sc, IWI_CSR_TX2_WRITE_INDEX, 0);

	CSR_WRITE_4(sc, IWI_CSR_TX3_BASE, sc->tx_ring_map->dm_segs[0].ds_addr);
	CSR_WRITE_4(sc, IWI_CSR_TX3_SIZE, IWI_TX_RING_SIZE);
	CSR_WRITE_4(sc, IWI_CSR_TX3_READ_INDEX, 0);
	CSR_WRITE_4(sc, IWI_CSR_TX3_WRITE_INDEX, 0);

	CSR_WRITE_4(sc, IWI_CSR_TX4_BASE, sc->tx_ring_map->dm_segs[0].ds_addr);
	CSR_WRITE_4(sc, IWI_CSR_TX4_SIZE, IWI_TX_RING_SIZE);
	CSR_WRITE_4(sc, IWI_CSR_TX4_READ_INDEX, 0);
	CSR_WRITE_4(sc, IWI_CSR_TX4_WRITE_INDEX, 0);

	for (i = 0; i < IWI_RX_RING_SIZE; i++)
		CSR_WRITE_4(sc, IWI_CSR_RX_BASE + i * 4,
		    sc->rx_buf[i].map->dm_segs[0].ds_addr);

	/*
	 * Kick Rx
	 */
	CSR_WRITE_4(sc, IWI_CSR_RX_WRITE_INDEX, sc->rx_cur);
	CSR_WRITE_4(sc, IWI_CSR_RX_READ_INDEX, 0);

	if ((error = iwi_load_firmware(sc, fw->main, fw->main_size)) != 0) {
		aprint_error("%s: could not load main firmware\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	sc->flags |= IWI_FLAG_FW_INITED;

	if ((error = iwi_config(sc)) != 0) {
		aprint_error("%s: device configuration failed\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}

	if (ic->ic_opmode != IEEE80211_M_MONITOR)
		ieee80211_begin_scan(ic, 1);
	else
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	return 0;

fail:	iwi_stop(ifp, 0);

	return error;
}

static void
iwi_stop(struct ifnet *ifp, int disable)
{
	struct iwi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwi_tx_buf *buf;
	int i;

	iwi_stop_master(sc);
	CSR_WRITE_4(sc, IWI_CSR_RST, IWI_RST_SW_RESET);

	/*
	 * Release Tx buffers
	 */
	for (i = 0; i < IWI_TX_RING_SIZE; i++) {
		buf = &sc->tx_buf[i];

		if (buf->m != NULL) {
			bus_dmamap_unload(sc->sc_dmat, buf->map);
			m_freem(buf->m);
			buf->m = NULL;

			if (buf->ni != NULL) {
				ieee80211_free_node(buf->ni);
				buf->ni = NULL;
			}
		}
	}

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
}
