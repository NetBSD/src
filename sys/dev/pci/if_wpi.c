/*  $NetBSD: if_wpi.c,v 1.9.2.2 2007/07/15 13:21:35 ad Exp $    */

/*-
 * Copyright (c) 2006, 2007
 *	Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_wpi.c,v 1.9.2.2 2007/07/15 13:21:35 ad Exp $");

/*
 * Driver for Intel PRO/Wireless 3945ABG 802.11 network adapters.
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
#include <sys/kauth.h>
#include <sys/callout.h>

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
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#include <dev/firmload.h>

#include <dev/pci/if_wpireg.h>
#include <dev/pci/if_wpivar.h>

#ifdef WPI_DEBUG
#define DPRINTF(x)	if (wpi_debug > 0) printf x
#define DPRINTFN(n, x)	if (wpi_debug >= (n)) printf x
int wpi_debug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

/*
 * Supported rates for 802.11a/b/g modes (in 500Kbps unit).
 */
static const struct ieee80211_rateset wpi_rateset_11a =
	{ 8, { 12, 18, 24, 36, 48, 72, 96, 108 } };

static const struct ieee80211_rateset wpi_rateset_11b =
	{ 4, { 2, 4, 11, 22 } };

static const struct ieee80211_rateset wpi_rateset_11g =
	{ 12, { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 } };

static int  wpi_match(struct device *, struct cfdata *, void *);
static void wpi_attach(struct device *, struct device *, void *);
static int  wpi_detach(struct device*, int);
static void wpi_power(int, void *);
static int  wpi_dma_contig_alloc(bus_dma_tag_t, struct wpi_dma_info *,
	void **, bus_size_t, bus_size_t, int);
static void wpi_dma_contig_free(struct wpi_dma_info *);
static int  wpi_alloc_shared(struct wpi_softc *);
static void wpi_free_shared(struct wpi_softc *);
static int  wpi_alloc_fwmem(struct wpi_softc *);
static void wpi_free_fwmem(struct wpi_softc *);
static struct wpi_rbuf *wpi_alloc_rbuf(struct wpi_softc *);
static void wpi_free_rbuf(struct mbuf *, void *, size_t, void *);
static int  wpi_alloc_rpool(struct wpi_softc *);
static void wpi_free_rpool(struct wpi_softc *);
static int  wpi_alloc_rx_ring(struct wpi_softc *, struct wpi_rx_ring *);
static void wpi_reset_rx_ring(struct wpi_softc *, struct wpi_rx_ring *);
static void wpi_free_rx_ring(struct wpi_softc *, struct wpi_rx_ring *);
static int  wpi_alloc_tx_ring(struct wpi_softc *, struct wpi_tx_ring *, int,
	int);
static void wpi_reset_tx_ring(struct wpi_softc *, struct wpi_tx_ring *);
static void wpi_free_tx_ring(struct wpi_softc *, struct wpi_tx_ring *);
static struct ieee80211_node * wpi_node_alloc(struct ieee80211_node_table *);
static void wpi_newassoc(struct ieee80211_node *, int);
static int  wpi_media_change(struct ifnet *);
static int  wpi_newstate(struct ieee80211com *, enum ieee80211_state, int);
static void wpi_mem_lock(struct wpi_softc *);
static void wpi_mem_unlock(struct wpi_softc *);
static uint32_t wpi_mem_read(struct wpi_softc *, uint16_t);
static void wpi_mem_write(struct wpi_softc *, uint16_t, uint32_t);
static int  wpi_read_prom_data(struct wpi_softc *, uint32_t, void *, int);
static int  wpi_load_segment(struct wpi_softc *, uint32_t, const uint8_t *,
			     int);
static int  wpi_load_firmware(struct wpi_softc *);
static void wpi_calib_timeout(void *);
static void wpi_iter_func(void *, struct ieee80211_node *);
static void wpi_power_calibration(struct wpi_softc *, int);
static void wpi_rx_intr(struct wpi_softc *, struct wpi_rx_desc *,
	struct wpi_rx_data *);
static void wpi_tx_intr(struct wpi_softc *, struct wpi_rx_desc *);
static void wpi_cmd_intr(struct wpi_softc *, struct wpi_rx_desc *);
static void wpi_notif_intr(struct wpi_softc *);
static int  wpi_intr(void *);
static void wpi_read_eeprom(struct wpi_softc *);
static void wpi_read_eeprom_channels(struct wpi_softc *, int);
static void wpi_read_eeprom_group(struct wpi_softc *, int);
static uint8_t wpi_plcp_signal(int);
static int  wpi_tx_data(struct wpi_softc *, struct mbuf *,
	struct ieee80211_node *, int);
static void wpi_start(struct ifnet *);
static void wpi_watchdog(struct ifnet *);
static int  wpi_ioctl(struct ifnet *, u_long, void *);
static int  wpi_cmd(struct wpi_softc *, int, const void *, int, int);
static int  wpi_wme_update(struct ieee80211com *);
static int  wpi_mrr_setup(struct wpi_softc *);
static void wpi_set_led(struct wpi_softc *, uint8_t, uint8_t, uint8_t);
static void wpi_enable_tsf(struct wpi_softc *, struct ieee80211_node *);
static int  wpi_set_txpower(struct wpi_softc *, 
			    struct ieee80211_channel *, int);
static int  wpi_get_power_index(struct wpi_softc *,
		struct wpi_power_group *, struct ieee80211_channel *, int);
static int  wpi_setup_beacon(struct wpi_softc *, struct ieee80211_node *);
static int  wpi_auth(struct wpi_softc *);
static int  wpi_scan(struct wpi_softc *, uint16_t);
static int  wpi_config(struct wpi_softc *);
static void wpi_stop_master(struct wpi_softc *);
static int  wpi_power_up(struct wpi_softc *);
static int  wpi_reset(struct wpi_softc *);
static void wpi_hw_config(struct wpi_softc *);
static int  wpi_init(struct ifnet *);
static void wpi_stop(struct ifnet *, int);

CFATTACH_DECL(wpi, sizeof (struct wpi_softc), wpi_match, wpi_attach,
	wpi_detach, NULL);

static int
wpi_match(struct device *parent, struct cfdata *match __unused, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_3945ABG_1 ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_3945ABG_2)
		return 1;

	return 0;
}

/* Base Address Register */
#define WPI_PCI_BAR0	0x10

static void
wpi_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct wpi_softc *sc = (struct wpi_softc *)self;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct pci_attach_args *pa = aux;
	const char *intrstr;
	char devinfo[256];
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	pci_intr_handle_t ih;
	pcireg_t data;
	int error, ac, revision, i;

	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	callout_init(&sc->amrr_ch, 0);

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
	error = pci_mapreg_map(pa, WPI_PCI_BAR0, PCI_MAPREG_TYPE_MEM |
		PCI_MAPREG_MEM_TYPE_32BIT, 0, &memt, &memh, NULL, &sc->sc_sz);
	if (error != 0) {
		aprint_error("%s: could not map memory space\n",
			sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_st = memt;
	sc->sc_sh = memh;
	sc->sc_dmat = pa->pa_dmat;

	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error("%s: could not map interrupt\n",
			sc->sc_dev.dv_xname);
		return;
	}

	intrstr = pci_intr_string(sc->sc_pct, ih);
	sc->sc_ih = pci_intr_establish(sc->sc_pct, ih, IPL_NET, wpi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: could not establish interrupt",
			sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	if (wpi_reset(sc) != 0) {
		aprint_error("%s: could not reset adapter\n",
			sc->sc_dev.dv_xname);
		return;
	}

 	/*
	 * Allocate DMA memory for firmware transfers.
	 */
	if ((error = wpi_alloc_fwmem(sc)) != 0) {
		aprint_error(": could not allocate firmware memory\n");
		return;
	}

	/*
	 * Allocate shared page and Tx/Rx rings.
	 */
	if ((error = wpi_alloc_shared(sc)) != 0) {
		aprint_error("%s: could not allocate shared area\n",
			sc->sc_dev.dv_xname);
		goto fail1;
	}

	if ((error = wpi_alloc_rpool(sc)) != 0) {
		aprint_error("%s: could not allocate Rx buffers\n",
			sc->sc_dev.dv_xname);
		goto fail2;
	}

	for (ac = 0; ac < 4; ac++) {
		error = wpi_alloc_tx_ring(sc, &sc->txq[ac], WPI_TX_RING_COUNT, ac);
		if (error != 0) {
			aprint_error("%s: could not allocate Tx ring %d\n",
					sc->sc_dev.dv_xname, ac);
			goto fail3;
		}
	}

	error = wpi_alloc_tx_ring(sc, &sc->cmdq, WPI_CMD_RING_COUNT, 4);
	if (error != 0) {
		aprint_error("%s: could not allocate command ring\n",
			sc->sc_dev.dv_xname);
		goto fail3;
	}

	error = wpi_alloc_tx_ring(sc, &sc->svcq, WPI_SVC_RING_COUNT, 5);
	if (error != 0) {
		aprint_error("%s: could not allocate service ring\n",
			sc->sc_dev.dv_xname);
		goto fail4;
	}

	if (wpi_alloc_rx_ring(sc, &sc->rxq) != 0) {
		aprint_error("%s: could not allocate Rx ring\n",
			sc->sc_dev.dv_xname);
		goto fail5;
	}

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM; /* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA; /* default to BSS mode */
	ic->ic_state = IEEE80211_S_INIT;

	/* set device capabilities */
	ic->ic_caps =
		IEEE80211_C_IBSS |       /* IBSS mode support */
		IEEE80211_C_WPA |        /* 802.11i */
		IEEE80211_C_MONITOR |    /* monitor mode supported */
		IEEE80211_C_TXPMGT |     /* tx power management */
		IEEE80211_C_SHSLOT |     /* short slot time supported */
		IEEE80211_C_SHPREAMBLE | /* short preamble supported */
		IEEE80211_C_WME;         /* 802.11e */

	/* read supported channels and MAC address from EEPROM */
	wpi_read_eeprom(sc);

	/* set supported .11a, .11b, .11g rates */
	ic->ic_sup_rates[IEEE80211_MODE_11A] = wpi_rateset_11a;
	ic->ic_sup_rates[IEEE80211_MODE_11B] = wpi_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = wpi_rateset_11g;

	ic->ic_ibss_chan = &ic->ic_channels[0];

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = wpi_init;
	ifp->if_stop = wpi_stop;
	ifp->if_ioctl = wpi_ioctl;
	ifp->if_start = wpi_start;
	ifp->if_watchdog = wpi_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ic);
	/* override default methods */
	ic->ic_node_alloc = wpi_node_alloc;
	ic->ic_newassoc = wpi_newassoc;
	ic->ic_wme.wme_update = wpi_wme_update;

	/* override state transition machine */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = wpi_newstate;
	ieee80211_media_init(ic, wpi_media_change, ieee80211_media_status);

	sc->amrr.amrr_min_success_threshold = 1;
	sc->amrr.amrr_max_success_threshold = 15;

	/* set powerhook */
	sc->powerhook = powerhook_establish(sc->sc_dev.dv_xname, wpi_power, sc);

#if NBPFILTER > 0
	bpfattach2(ifp, DLT_IEEE802_11_RADIO,
		sizeof (struct ieee80211_frame) + IEEE80211_RADIOTAP_HDRLEN,
		&sc->sc_drvbpf);

	sc->sc_rxtap_len = sizeof sc->sc_rxtapu;
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(WPI_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof sc->sc_txtapu;
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(WPI_TX_RADIOTAP_PRESENT);
#endif

	ieee80211_announce(ic);

	return;

fail5:  wpi_free_tx_ring(sc, &sc->svcq);
fail4:  wpi_free_tx_ring(sc, &sc->cmdq);
fail3:  while (--ac >= 0)
			wpi_free_tx_ring(sc, &sc->txq[ac]);
	wpi_free_rpool(sc);
fail2:	wpi_free_shared(sc);
fail1:	wpi_free_fwmem(sc);
}

static int
wpi_detach(struct device* self, int flags __unused)
{
	struct wpi_softc *sc = (struct wpi_softc *)self;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	int ac;

	wpi_stop(ifp, 1);

#if NBPFILTER > 0
	if (ifp != NULL)
		bpfdetach(ifp);
#endif
	ieee80211_ifdetach(&sc->sc_ic);
	if (ifp != NULL)
		if_detach(ifp);

	for (ac = 0; ac < 4; ac++)
		wpi_free_tx_ring(sc, &sc->txq[ac]);
	wpi_free_tx_ring(sc, &sc->cmdq);
	wpi_free_tx_ring(sc, &sc->svcq);
	wpi_free_rx_ring(sc, &sc->rxq);
	wpi_free_rpool(sc);
	wpi_free_shared(sc);

	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pct, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);

	return 0;
}

static void
wpi_power(int why, void *arg)
{
	struct wpi_softc *sc = arg;
	struct ifnet *ifp;
	pcireg_t data;
	int s;

	if (why != PWR_RESUME)
		return;

	/* clear device specific PCI configuration register 0x41 */
	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0x40);
	data &= ~0x0000ff00;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0x40, data);

	s = splnet();
	ifp = sc->sc_ic.ic_ifp;
	if (ifp->if_flags & IFF_UP) {
		ifp->if_init(ifp);
		if (ifp->if_flags & IFF_RUNNING)
			ifp->if_start(ifp);
	}
	splx(s);
}

static int
wpi_dma_contig_alloc(bus_dma_tag_t tag, struct wpi_dma_info *dma,
	void **kvap, bus_size_t size, bus_size_t alignment, int flags)
{
	int nsegs, error;

	dma->tag = tag;
	dma->size = size;

	error = bus_dmamap_create(tag, size, 1, size, 0, flags, &dma->map);
	if (error != 0)
		goto fail;

	error = bus_dmamem_alloc(tag, size, alignment, 0, &dma->seg, 1, &nsegs,
	    flags);
	if (error != 0)
		goto fail;

	error = bus_dmamem_map(tag, &dma->seg, 1, size, &dma->vaddr, flags);
	if (error != 0)
		goto fail;

	error = bus_dmamap_load(tag, dma->map, dma->vaddr, size, NULL, flags);
	if (error != 0)
		goto fail;

	memset(dma->vaddr, 0, size);

	dma->paddr = dma->map->dm_segs[0].ds_addr;
	if (kvap != NULL)
		*kvap = dma->vaddr;

	return 0;

fail:   wpi_dma_contig_free(dma);
	return error;
}

static void
wpi_dma_contig_free(struct wpi_dma_info *dma)
{
	if (dma->map != NULL) {
		if (dma->vaddr != NULL) {
			bus_dmamap_unload(dma->tag, dma->map);
			bus_dmamem_unmap(dma->tag, dma->vaddr, dma->size);
			bus_dmamem_free(dma->tag, &dma->seg, 1);
			dma->vaddr = NULL;
		}
		bus_dmamap_destroy(dma->tag, dma->map);
		dma->map = NULL;
	}
}

/*
 * Allocate a shared page between host and NIC.
 */
static int
wpi_alloc_shared(struct wpi_softc *sc)
{
	int error;
	/* must be aligned on a 4K-page boundary */
	error = wpi_dma_contig_alloc(sc->sc_dmat, &sc->shared_dma,
			(void **)&sc->shared, sizeof (struct wpi_shared), 
			WPI_BUF_ALIGN,BUS_DMA_NOWAIT);
	if (error != 0)
		aprint_error(
			"%s: could not allocate shared area DMA memory\n",
			sc->sc_dev.dv_xname);

	return error;
}

static void
wpi_free_shared(struct wpi_softc *sc)
{
	wpi_dma_contig_free(&sc->shared_dma);
}

/*
 * Allocate DMA-safe memory for firmware transfer.
 */
static int
wpi_alloc_fwmem(struct wpi_softc *sc)
{
	int error;
	/* allocate enough contiguous space to store text and data */
	error = wpi_dma_contig_alloc(sc->sc_dmat, &sc->fw_dma, NULL,
	    WPI_FW_MAIN_TEXT_MAXSZ + WPI_FW_MAIN_DATA_MAXSZ, 0,
	    BUS_DMA_NOWAIT);

	if (error != 0)
		aprint_error(
			"%s: could not allocate firmware transfer area"
			"DMA memory\n", sc->sc_dev.dv_xname);
	return error;
}

static void
wpi_free_fwmem(struct wpi_softc *sc)
{
	wpi_dma_contig_free(&sc->fw_dma);
}


static struct wpi_rbuf *
wpi_alloc_rbuf(struct wpi_softc *sc)
{
	struct wpi_rbuf *rbuf;

	rbuf = SLIST_FIRST(&sc->rxq.freelist);
	if (rbuf == NULL)
		return NULL;
	SLIST_REMOVE_HEAD(&sc->rxq.freelist, next);
	sc->rxq.nb_free_entries --;

	return rbuf;
}

/*
 * This is called automatically by the network stack when the mbuf to which our
 * Rx buffer is attached is freed.
 */
static void
wpi_free_rbuf(struct mbuf* m, void *buf, size_t size, void *arg)
{
	struct wpi_rbuf *rbuf = arg;
	struct wpi_softc *sc = rbuf->sc;
	int s;

	/* put the buffer back in the free list */

	SLIST_INSERT_HEAD(&sc->rxq.freelist, rbuf, next);
	sc->rxq.nb_free_entries ++;

	if (__predict_true(m != NULL)) {
		s = splvm();
		pool_cache_put(&mbpool_cache, m);
		splx(s);
	}
}

static int
wpi_alloc_rpool(struct wpi_softc *sc)
{
	struct wpi_rx_ring *ring = &sc->rxq;
	struct wpi_rbuf *rbuf;
	int i, error;

	/* allocate a big chunk of DMA'able memory.. */
	error = wpi_dma_contig_alloc(sc->sc_dmat, &ring->buf_dma, NULL,
	    WPI_RBUF_COUNT * WPI_RBUF_SIZE, WPI_BUF_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_normal("%s: could not allocate Rx buffers DMA memory\n",
		    sc->sc_dev.dv_xname);
	return error;
	}

	/* ..and split it into 3KB chunks */
	SLIST_INIT(&ring->freelist);
	for (i = 0; i < WPI_RBUF_COUNT; i++) {
		rbuf = &ring->rbuf[i];
		rbuf->sc = sc;	/* backpointer for callbacks */
		rbuf->vaddr = (char *)ring->buf_dma.vaddr + i * WPI_RBUF_SIZE;
		rbuf->paddr = ring->buf_dma.paddr + i * WPI_RBUF_SIZE;

		SLIST_INSERT_HEAD(&ring->freelist, rbuf, next);
	}

	ring->nb_free_entries = WPI_RBUF_COUNT;
	return 0;
}

static void
wpi_free_rpool(struct wpi_softc *sc)
{
	wpi_dma_contig_free(&sc->rxq.buf_dma);
}

static int
wpi_alloc_rx_ring(struct wpi_softc *sc, struct wpi_rx_ring *ring)
{
	struct wpi_rx_data *data;
	struct wpi_rbuf *rbuf;
	int i, error;

	ring->cur = 0;

	error = wpi_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma,
		(void **)&ring->desc,
		WPI_RX_RING_COUNT * sizeof (struct wpi_rx_desc),
		WPI_RING_DMA_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate rx ring DMA memory\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	/*
	 * Setup Rx buffers.
	 */
	for (i = 0; i < WPI_RX_RING_COUNT; i++) {
		data = &ring->data[i];

		MGETHDR(data->m, M_DONTWAIT, MT_DATA);
		if (data->m == NULL) {
			aprint_error("%s: could not allocate rx mbuf\n",
				sc->sc_dev.dv_xname);
			error = ENOMEM;
			goto fail;
		}
		if ((rbuf = wpi_alloc_rbuf(sc)) == NULL) {
			m_freem(data->m);
			data->m = NULL;
			aprint_error("%s: could not allocate rx cluster\n",
				sc->sc_dev.dv_xname);
			error = ENOMEM;
			goto fail;
		}
		/* attach Rx buffer to mbuf */
		MEXTADD(data->m, rbuf->vaddr, WPI_RBUF_SIZE, 0, wpi_free_rbuf,
		    rbuf);
		data->m->m_flags |= M_EXT_RW;

		ring->desc[i] = htole32(rbuf->paddr);
	}

	return 0;

fail:	wpi_free_rx_ring(sc, ring);
	return error;
}

static void
wpi_reset_rx_ring(struct wpi_softc *sc, struct wpi_rx_ring *ring)
{
	int ntries;

	wpi_mem_lock(sc);

	WPI_WRITE(sc, WPI_RX_CONFIG, 0);
	for (ntries = 0; ntries < 100; ntries++) {
		if (WPI_READ(sc, WPI_RX_STATUS) & WPI_RX_IDLE)
			break;
		DELAY(10);
	}
#ifdef WPI_DEBUG
	if (ntries == 100 && wpi_debug > 0)
		aprint_error("%s: timeout resetting Rx ring\n",
			sc->sc_dev.dv_xname);
#endif
	wpi_mem_unlock(sc);

	ring->cur = 0;
}

static void
wpi_free_rx_ring(struct wpi_softc *sc, struct wpi_rx_ring *ring)
{
	int i;

	wpi_dma_contig_free(&ring->desc_dma);

	for (i = 0; i < WPI_RX_RING_COUNT; i++) {
		if (ring->data[i].m != NULL)
			m_freem(ring->data[i].m);
	}
}

static int
wpi_alloc_tx_ring(struct wpi_softc *sc, struct wpi_tx_ring *ring, int count,
	int qid)
{
	struct wpi_tx_data *data;
	int i, error;

	ring->qid = qid;
	ring->count = count;
	ring->queued = 0;
	ring->cur = 0;

	error = wpi_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma,
		(void **)&ring->desc, count * sizeof (struct wpi_tx_desc),
		WPI_RING_DMA_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate tx ring DMA memory\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	/* update shared page with ring's base address */
	sc->shared->txbase[qid] = htole32(ring->desc_dma.paddr);

	error = wpi_dma_contig_alloc(sc->sc_dmat, &ring->cmd_dma,
		(void **)&ring->cmd,
		count * sizeof (struct wpi_tx_cmd), 4, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error("%s: could not allocate tx cmd DMA memory\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	ring->data = malloc(count * sizeof (struct wpi_tx_data), M_DEVBUF,
		M_NOWAIT);
	if (ring->data == NULL) {
		aprint_error("%s: could not allocate tx data slots\n",
			sc->sc_dev.dv_xname);
		goto fail;
	}

	memset(ring->data, 0, count * sizeof (struct wpi_tx_data));

	for (i = 0; i < count; i++) {
		data = &ring->data[i];

		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
			WPI_MAX_SCATTER - 1, MCLBYTES, 0, BUS_DMA_NOWAIT,
			&data->map);
		if (error != 0) {
			aprint_error("%s: could not create tx buf DMA map\n",
				sc->sc_dev.dv_xname);
			goto fail;
		}
	}

	return 0;

fail:	wpi_free_tx_ring(sc, ring);
	return error;
}

static void
wpi_reset_tx_ring(struct wpi_softc *sc, struct wpi_tx_ring *ring)
{
	struct wpi_tx_data *data;
	int i, ntries;

	wpi_mem_lock(sc);

	WPI_WRITE(sc, WPI_TX_CONFIG(ring->qid), 0);
	for (ntries = 0; ntries < 100; ntries++) {
		if (WPI_READ(sc, WPI_TX_STATUS) & WPI_TX_IDLE(ring->qid))
			break;
		DELAY(10);
	}
#ifdef WPI_DEBUG
	if (ntries == 100 && wpi_debug > 0) {
		aprint_error("%s: timeout resetting Tx ring %d\n",
			sc->sc_dev.dv_xname, ring->qid);
	}
#endif
	wpi_mem_unlock(sc);

	for (i = 0; i < ring->count; i++) {
		data = &ring->data[i];

		if (data->m != NULL) {
			bus_dmamap_unload(sc->sc_dmat, data->map);
			m_freem(data->m);
			data->m = NULL;
		}
	}

	ring->queued = 0;
	ring->cur = 0;
}

static void
wpi_free_tx_ring(struct wpi_softc *sc, struct wpi_tx_ring *ring)
{
	struct wpi_tx_data *data;
	int i;

	wpi_dma_contig_free(&ring->desc_dma);
	wpi_dma_contig_free(&ring->cmd_dma);

	if (ring->data != NULL) {
		for (i = 0; i < ring->count; i++) {
			data = &ring->data[i];

			if (data->m != NULL) {
				bus_dmamap_unload(sc->sc_dmat, data->map);
				m_freem(data->m);
			}
		}
		free(ring->data, M_DEVBUF);
	}
}

/*ARGUSED*/
static struct ieee80211_node *
wpi_node_alloc(struct ieee80211_node_table *nt __unused)
{
	struct wpi_node *wn;

	wn = malloc(sizeof (struct wpi_node), M_DEVBUF, M_NOWAIT);

	if (wn != NULL)
		memset(wn, 0, sizeof (struct wpi_node));
	return (struct ieee80211_node *)wn;
}

static void
wpi_newassoc(struct ieee80211_node *ni, int isnew)
{
	struct wpi_softc *sc = ni->ni_ic->ic_ifp->if_softc;
	int i;

	ieee80211_amrr_node_init(&sc->amrr, &((struct wpi_node *)ni)->amn);

	/* set rate to some reasonable initial value */
	for (i = ni->ni_rates.rs_nrates - 1;
	     i > 0 && (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL) > 72;
	     i--);
	ni->ni_txrate = i;
}

static int
wpi_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return error;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
		wpi_init(ifp);

	return 0;
}

static int
wpi_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct wpi_softc *sc = ifp->if_softc;
	struct ieee80211_node *ni;
	int error;

	callout_stop(&sc->calib_to);

	switch (nstate) {
	case IEEE80211_S_SCAN:
		ieee80211_node_table_reset(&ic->ic_scan);
		ic->ic_flags |= IEEE80211_F_SCAN | IEEE80211_F_ASCAN;

		/* make the link LED blink while we're scanning */
		wpi_set_led(sc, WPI_LED_LINK, 20, 2);

		if ((error = wpi_scan(sc, IEEE80211_CHAN_G)) != 0) {
			aprint_error("%s: could not initiate scan\n",
				sc->sc_dev.dv_xname);
			ic->ic_flags &= ~(IEEE80211_F_SCAN | IEEE80211_F_ASCAN);
			return error;
		}

		ic->ic_state = nstate;
		return 0;

	case IEEE80211_S_ASSOC:
		if (ic->ic_state != IEEE80211_S_RUN)
			break;
		/* FALLTHROUGH */
	case IEEE80211_S_AUTH:
		sc->config.associd = 0;
		sc->config.filter &= ~htole32(WPI_FILTER_BSS);
		if ((error = wpi_auth(sc)) != 0) {
			aprint_error("%s: could not send authentication request\n",
				sc->sc_dev.dv_xname);
			return error;
		}
		break;

	case IEEE80211_S_RUN:
		if (ic->ic_opmode == IEEE80211_M_MONITOR) {
			/* link LED blinks while monitoring */
			wpi_set_led(sc, WPI_LED_LINK, 5, 5);
			break;
		}
		
		ni = ic->ic_bss;

		if (ic->ic_opmode != IEEE80211_M_STA) {
			(void) wpi_auth(sc);    /* XXX */
			wpi_setup_beacon(sc, ni);
		}

		wpi_enable_tsf(sc, ni);

		/* update adapter's configuration */
		sc->config.associd = htole16(ni->ni_associd & ~0xc000);
		/* short preamble/slot time are negotiated when associating */
		sc->config.flags &= ~htole32(WPI_CONFIG_SHPREAMBLE |
			WPI_CONFIG_SHSLOT);
		if (ic->ic_flags & IEEE80211_F_SHSLOT)
			sc->config.flags |= htole32(WPI_CONFIG_SHSLOT);
		if (ic->ic_flags & IEEE80211_F_SHPREAMBLE)
			sc->config.flags |= htole32(WPI_CONFIG_SHPREAMBLE);
		sc->config.filter |= htole32(WPI_FILTER_BSS);
		if (ic->ic_opmode != IEEE80211_M_STA)
			sc->config.filter |= htole32(WPI_FILTER_BEACON);

/* XXX put somewhere HC_QOS_SUPPORT_ASSOC + HC_IBSS_START */

		DPRINTF(("config chan %d flags %x\n", sc->config.chan,
			sc->config.flags));
		error = wpi_cmd(sc, WPI_CMD_CONFIGURE, &sc->config,
			sizeof (struct wpi_config), 1);
		if (error != 0) {
			aprint_error("%s: could not update configuration\n",
				sc->sc_dev.dv_xname);
			return error;
		}

		/* configuration has changed, set Tx power accordingly */
		if ((error = wpi_set_txpower(sc, ni->ni_chan, 1)) != 0) {
			aprint_error("%s: could not set Tx power\n",
			    sc->sc_dev.dv_xname);
			return error;
		}

		if (ic->ic_opmode == IEEE80211_M_STA) {
			/* fake a join to init the tx rate */
			wpi_newassoc(ni, 1);
		}

		/* start periodic calibration timer */
		sc->calib_cnt = 0;
		callout_reset(&sc->calib_to, hz/2, wpi_calib_timeout, sc);

		/* link LED always on while associated */
		wpi_set_led(sc, WPI_LED_LINK, 0, 1);
		break;

	case IEEE80211_S_INIT:
		break;
	}

	return sc->sc_newstate(ic, nstate, arg);
}

/*
 * Grab exclusive access to NIC memory.
 */
static void
wpi_mem_lock(struct wpi_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = WPI_READ(sc, WPI_GPIO_CTL);
	WPI_WRITE(sc, WPI_GPIO_CTL, tmp | WPI_GPIO_MAC);

	/* spin until we actually get the lock */
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((WPI_READ(sc, WPI_GPIO_CTL) &
			(WPI_GPIO_CLOCK | WPI_GPIO_SLEEP)) == WPI_GPIO_CLOCK)
			break;
		DELAY(10);
	}
	if (ntries == 1000)
		aprint_error("%s: could not lock memory\n", sc->sc_dev.dv_xname);
}

/*
 * Release lock on NIC memory.
 */
static void
wpi_mem_unlock(struct wpi_softc *sc)
{
	uint32_t tmp = WPI_READ(sc, WPI_GPIO_CTL);
	WPI_WRITE(sc, WPI_GPIO_CTL, tmp & ~WPI_GPIO_MAC);
}

static uint32_t
wpi_mem_read(struct wpi_softc *sc, uint16_t addr)
{
	WPI_WRITE(sc, WPI_READ_MEM_ADDR, WPI_MEM_4 | addr);
	return WPI_READ(sc, WPI_READ_MEM_DATA);
}

static void
wpi_mem_write(struct wpi_softc *sc, uint16_t addr, uint32_t data)
{
	WPI_WRITE(sc, WPI_WRITE_MEM_ADDR, WPI_MEM_4 | addr);
	WPI_WRITE(sc, WPI_WRITE_MEM_DATA, data);
}

/*
 * Read `len' bytes from the EEPROM.  We access the EEPROM through the MAC
 * instead of using the traditional bit-bang method.
 */
static int
wpi_read_prom_data(struct wpi_softc *sc, uint32_t addr, void *data, int len)
{
	uint8_t *out = data;
	uint32_t val;
	int ntries;

	wpi_mem_lock(sc);
	for (; len > 0; len -= 2, addr++) {
		WPI_WRITE(sc, WPI_EEPROM_CTL, addr << 2);

		for (ntries = 0; ntries < 10; ntries++) {
			if ((val = WPI_READ(sc, WPI_EEPROM_CTL)) &
			    WPI_EEPROM_READY)
				break;
			DELAY(5);
		}
		if (ntries == 10) {
			aprint_error("%s: could not read EEPROM\n",
			    sc->sc_dev.dv_xname);
			return ETIMEDOUT;
		}
		*out++ = val >> 16;
		if (len > 1)
			*out++ = val >> 24;
	}
	wpi_mem_unlock(sc);

	return 0;
}

static int
wpi_load_segment(struct wpi_softc *sc, uint32_t target, const uint8_t *data,
		 int len)
{
	struct wpi_dma_info *dma = &sc->fw_dma;
	struct wpi_tx_desc desc;
	int ntries, error = 0;

	DPRINTFN(2, ("loading firmware segment target=%x len=%d\n", target,
	    len));

	/* copy data to pre-allocated DMA-safe memory */
	memcpy(dma->vaddr, data, len);
	bus_dmamap_sync(dma->tag, dma->map, 0, len, BUS_DMASYNC_PREWRITE);

	/* setup Tx descriptor */
	memset(&desc, 0, sizeof desc);
	desc.flags = htole32(WPI_PAD32(len) << 28 | 1 << 24);
	desc.segs[0].addr = htole32(dma->paddr);
	desc.segs[0].len  = htole32(len);

	/* tell adapter where to copy data in its internal memory */
	WPI_WRITE(sc, WPI_FW_TARGET, target);

	WPI_WRITE(sc, WPI_TX_CONFIG(6), 0);

	/* copy Tx descriptor into NIC memory */
	WPI_WRITE_REGION_4(sc, WPI_TX_DESC(6), (uint32_t *)&desc,
	    sizeof desc / sizeof (uint32_t));

	WPI_WRITE(sc, WPI_TX_CREDIT(6), 0xfffff);
	WPI_WRITE(sc, WPI_TX_STATE(6), 0x4001);
	WPI_WRITE(sc, WPI_TX_CONFIG(6), 0x80000001);

	/* wait while the adapter transfers the block */
	for (ntries = 0; ntries < 100; ntries++) {
		if (WPI_READ(sc, WPI_TX_STATUS) & WPI_TX_IDLE(6))
			break;
		DELAY(1000);
	}
	if (ntries == 100) {
		aprint_error("%s: timeout transferring firmware segment\n",
		    sc->sc_dev.dv_xname);
		error = ETIMEDOUT;
	}

	WPI_WRITE(sc, WPI_TX_CREDIT(6), 0);

	return error;
}

static int
wpi_load_firmware(struct wpi_softc *sc)
{
	struct wpi_dma_info *dma = &sc->fw_dma;
	struct wpi_firmware_hdr hdr;
	const uint8_t *boot_text, *boot_data, *main_text, *main_data;
	uint32_t main_textsz, main_datasz, boot_textsz, boot_datasz;
	firmware_handle_t fw;
	u_char *dfw;
	size_t size;
	uint32_t tmp;
	int error;

	/* load firmware image from disk */
	if ((error = firmware_open("if_wpi","iwlwifi-3945.ucode", &fw) != 0)) {
		aprint_error("%s: could not read firmware file\n",
		    sc->sc_dev.dv_xname);
		goto fail1;
	}
	
	size = firmware_get_size(fw);

	/* extract firmware header information */
	if (size < sizeof (struct wpi_firmware_hdr)) {
		aprint_error("%s: truncated firmware header: %zu bytes\n",
		    sc->sc_dev.dv_xname, size);
		error = EINVAL;
		goto fail2;
	}

	if ((error = firmware_read(fw, 0, &hdr,
		sizeof (struct wpi_firmware_hdr))) != 0) {
		aprint_error("%s: can't get firmware header\n",
			sc->sc_dev.dv_xname);
		goto fail2;
	}

	main_textsz = le32toh(hdr.main_textsz);
	main_datasz = le32toh(hdr.main_datasz);
	boot_textsz = le32toh(hdr.boot_textsz);
	boot_datasz = le32toh(hdr.boot_datasz);

	/* sanity-check firmware header */
	if (main_textsz > WPI_FW_MAIN_TEXT_MAXSZ) {
		aprint_error("%s: main .text segment too large: %u bytes\n",
		    sc->sc_dev.dv_xname, main_textsz);
		error = EINVAL;
		goto fail2;
	}
	if (main_datasz > WPI_FW_MAIN_DATA_MAXSZ) {
		aprint_error("%s: main .data segment too large: %u bytes\n",
		    sc->sc_dev.dv_xname, main_datasz);
		error = EINVAL;
		goto fail2;
	}
	if (boot_textsz > WPI_FW_BOOT_TEXT_MAXSZ) {
		aprint_error("%s: boot .text segment too large: %u bytes\n",
		    sc->sc_dev.dv_xname, boot_textsz);
		error = EINVAL;
		goto fail2;
	}
	if (boot_datasz > WPI_FW_BOOT_DATA_MAXSZ) {
		aprint_error("%s: boot .data segment too large: %u bytes\n",
		    sc->sc_dev.dv_xname, boot_datasz);
		error = EINVAL;
		goto fail2;
	}

	/* check that all firmware segments are present */
	if (size < sizeof (struct wpi_firmware_hdr) + main_textsz +
	    main_datasz + boot_textsz + boot_datasz) {
		aprint_error("%s: firmware file too short: %zu bytes\n",
		    sc->sc_dev.dv_xname, size);
		error = EINVAL;
		goto fail2;
	}

	dfw = firmware_malloc(size);
	if (dfw == NULL) {
		aprint_error("%s: not enough memory to stock firmware\n",
			sc->sc_dev.dv_xname);
		error = ENOMEM;
		goto fail2;
	}

	if ((error = firmware_read(fw, 0, dfw, size)) != 0) {
		aprint_error("%s: can't get firmware\n",
			sc->sc_dev.dv_xname);
		goto fail2;
	}

	/* get pointers to firmware segments */
	main_text = dfw + sizeof (struct wpi_firmware_hdr);
	main_data = main_text + main_textsz;
	boot_text = main_data + main_datasz;
	boot_data = boot_text + boot_textsz;

	/* load firmware boot .data segment into NIC */
	error = wpi_load_segment(sc, WPI_FW_DATA, boot_data, boot_datasz);
	if (error != 0) {
		aprint_error("%s: could not load firmware boot .data segment\n",
		    sc->sc_dev.dv_xname);
		goto fail3;
	}

	/* load firmware boot .text segment into NIC */
	error = wpi_load_segment(sc, WPI_FW_TEXT, boot_text, boot_textsz);
	if (error != 0) {
		aprint_error("%s: could not load firmware boot .text segment\n",
		    sc->sc_dev.dv_xname);
		goto fail3;
	}

	/* copy firmware runtime into pre-allocated DMA-safe memory */
	memcpy(dma->vaddr, main_text, main_textsz);
	memcpy((uint8_t*)dma->vaddr + main_textsz, main_data, main_datasz);
	bus_dmamap_sync(dma->tag, dma->map, 0, main_textsz + main_datasz,
	    BUS_DMASYNC_PREWRITE);

	/* tell adapter where to find firmware runtime */
	wpi_mem_lock(sc);
	wpi_mem_write(sc, WPI_MEM_MAIN_TEXT_BASE, dma->paddr);
	wpi_mem_write(sc, WPI_MEM_MAIN_TEXT_SIZE, main_textsz);
	wpi_mem_write(sc, WPI_MEM_MAIN_DATA_BASE, dma->paddr + main_textsz);
	wpi_mem_write(sc, WPI_MEM_MAIN_DATA_SIZE, main_datasz);
	wpi_mem_unlock(sc);

	/* now press "execute" ;-) */
	tmp = WPI_READ(sc, WPI_RESET);
	tmp &= ~(WPI_MASTER_DISABLED | WPI_STOP_MASTER | WPI_NEVO_RESET);
	WPI_WRITE(sc, WPI_RESET, tmp);

	/* ..and wait at most one second for adapter to initialize */
	if ((error = tsleep(sc, PCATCH, "wpiinit", hz)) != 0) {
		/* this isn't what was supposed to happen.. */
		aprint_error("%s: timeout waiting for adapter to initialize\n",
		    sc->sc_dev.dv_xname);
	}

fail3: 	firmware_free(dfw,size);
fail2:	firmware_close(fw);
fail1:	return error;
}

static void
wpi_calib_timeout(void *arg)
{
	struct wpi_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	int temp, s;

	/* automatic rate control triggered every 500ms */
	if (ic->ic_fixed_rate == -1) {
		s = splnet();
		if (ic->ic_opmode == IEEE80211_M_STA)
			wpi_iter_func(sc, ic->ic_bss);
		else
                	ieee80211_iterate_nodes(&ic->ic_sta, wpi_iter_func, sc);
		splx(s);
	}

	/* update sensor data */
	temp = (int)WPI_READ(sc, WPI_TEMPERATURE);

	/* automatic power calibration every 60s */
	if (++sc->calib_cnt >= 120) {
		wpi_power_calibration(sc, temp);
		sc->calib_cnt = 0;
	}

	callout_reset(&sc->calib_to, hz/2, wpi_calib_timeout, sc);
}

static void
wpi_iter_func(void *arg, struct ieee80211_node *ni)
{
	struct wpi_softc *sc = arg;
	struct wpi_node *wn = (struct wpi_node *)ni;

	ieee80211_amrr_choose(&sc->amrr, ni, &wn->amn);
}

/*
 * This function is called periodically (every 60 seconds) to adjust output
 * power to temperature changes.
 */
void
wpi_power_calibration(struct wpi_softc *sc, int temp)
{
	/* sanity-check read value */
	if (temp < -260 || temp > 25) {
		/* this can't be correct, ignore */
		DPRINTF(("out-of-range temperature reported: %d\n", temp));
		return;
	}

	DPRINTF(("temperature %d->%d\n", sc->temp, temp));

	/* adjust Tx power if need be */
	if (abs(temp - sc->temp) <= 6)
		return;

	sc->temp = temp;

	if (wpi_set_txpower(sc, sc->sc_ic.ic_bss->ni_chan, 1) != 0) {
		/* just warn, too bad for the automatic calibration... */
		aprint_error("%s: could not adjust Tx power\n",
		    sc->sc_dev.dv_xname);
	}
}

static void
wpi_rx_intr(struct wpi_softc *sc, struct wpi_rx_desc *desc,
	struct wpi_rx_data *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct wpi_rx_ring *ring = &sc->rxq;
	struct wpi_rx_stat *stat;
	struct wpi_rx_head *head;
	struct wpi_rx_tail *tail;
	struct wpi_rbuf *rbuf;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct mbuf *m, *mnew;

	stat = (struct wpi_rx_stat *)(desc + 1);

	if (stat->len > WPI_STAT_MAXLEN) {
		aprint_error("%s: invalid rx statistic header\n",
			sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
		return;
	}

	head = (struct wpi_rx_head *)((char *)(stat + 1) + stat->len);
	tail = (struct wpi_rx_tail *)((char *)(head + 1) + le16toh(head->len));

	DPRINTFN(4, ("rx intr: idx=%d len=%d stat len=%d rssi=%d rate=%x "
		"chan=%d tstamp=%" PRId64 "\n", ring->cur, le32toh(desc->len),
		le16toh(head->len), (int8_t)stat->rssi, head->rate, head->chan,
		le64toh(tail->tstamp)));

	/*
	 * Discard Rx frames with bad CRC early (XXX we may want to pass them
	 * to radiotap in monitor mode).
	 */
	if ((le32toh(tail->flags) & WPI_RX_NOERROR) != WPI_RX_NOERROR) {
		DPRINTF(("rx tail flags error %x\n", le32toh(tail->flags)));
		ifp->if_ierrors++;
		return;
	}



	/* 
	 * If the number of free entry is too low
	 * just dup the data->m socket and reuse the same rbuf entry
	 */
	if (sc->rxq.nb_free_entries <= WPI_RBUF_LOW_LIMIT) {
		
		/* Set length before calling m_dup */
		data->m->m_pkthdr.len = data->m->m_len = le16toh(head->len);
		
		m = m_dup(data->m,0,M_COPYALL,M_DONTWAIT);
	} else {

		MGETHDR(mnew, M_DONTWAIT, MT_DATA);
		if (mnew == NULL) {
			ifp->if_ierrors++;
			return;
		}

		rbuf = wpi_alloc_rbuf(sc);
		KASSERT(rbuf != NULL);

 		/* attach Rx buffer to mbuf */
		MEXTADD(mnew, rbuf->vaddr, WPI_RBUF_SIZE, 0, wpi_free_rbuf, 
		 	rbuf);
		mnew->m_flags |= M_EXT_RW;

		m = data->m;
		data->m = mnew;

		/* update Rx descriptor */
		ring->desc[ring->cur] = htole32(rbuf->paddr);
	}

	/* finalize mbuf */
	m->m_pkthdr.rcvif = ifp;
	m->m_data = (void *)(head + 1);
	m->m_pkthdr.len = m->m_len = le16toh(head->len);

#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct wpi_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		tap->wr_chan_freq =
			htole16(ic->ic_channels[head->chan].ic_freq);
		tap->wr_chan_flags =
			htole16(ic->ic_channels[head->chan].ic_flags);
		tap->wr_dbm_antsignal = (int8_t)(stat->rssi - WPI_RSSI_OFFSET);
		tap->wr_dbm_antnoise = (int8_t)le16toh(stat->noise);
		tap->wr_tsft = tail->tstamp;
		tap->wr_antenna = (le16toh(head->flags) >> 4) & 0xf;
		switch (head->rate) {
		/* CCK rates */
		case  10: tap->wr_rate =   2; break;
		case  20: tap->wr_rate =   4; break;
		case  55: tap->wr_rate =  11; break;
		case 110: tap->wr_rate =  22; break;
		/* OFDM rates */
		case 0xd: tap->wr_rate =  12; break;
		case 0xf: tap->wr_rate =  18; break;
		case 0x5: tap->wr_rate =  24; break;
		case 0x7: tap->wr_rate =  36; break;
		case 0x9: tap->wr_rate =  48; break;
		case 0xb: tap->wr_rate =  72; break;
		case 0x1: tap->wr_rate =  96; break;
		case 0x3: tap->wr_rate = 108; break;
		/* unknown rate: should not happen */
		default:  tap->wr_rate =   0;
		}
		if (le16toh(head->flags) & 0x4)
			tap->wr_flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_rxtap_len, m);
	}
#endif

	/* grab a reference to the source node */
	wh = mtod(m, struct ieee80211_frame *);
	ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)wh);

	/* send the frame to the 802.11 layer */
	ieee80211_input(ic, m, ni, stat->rssi, 0);

	/* release node reference */
	ieee80211_free_node(ni);
}

static void
wpi_tx_intr(struct wpi_softc *sc, struct wpi_rx_desc *desc)
{
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	struct wpi_tx_ring *ring = &sc->txq[desc->qid & 0x3];
	struct wpi_tx_data *txdata = &ring->data[desc->idx];
	struct wpi_tx_stat *stat = (struct wpi_tx_stat *)(desc + 1);
	struct wpi_node *wn = (struct wpi_node *)txdata->ni;

	DPRINTFN(4, ("tx done: qid=%d idx=%d retries=%d nkill=%d rate=%x "
		"duration=%d status=%x\n", desc->qid, desc->idx, stat->ntries,
		stat->nkill, stat->rate, le32toh(stat->duration),
		le32toh(stat->status)));

	/*
	 * Update rate control statistics for the node.
	 * XXX we should not count mgmt frames since they're always sent at
	 * the lowest available bit-rate.
	 */
	wn->amn.amn_txcnt++;
	if (stat->ntries > 0) {
		DPRINTFN(3, ("tx intr ntries %d\n", stat->ntries));
		wn->amn.amn_retrycnt++;
	}

	if ((le32toh(stat->status) & 0xff) != 1)
		ifp->if_oerrors++;
	else
		ifp->if_opackets++;

	bus_dmamap_unload(sc->sc_dmat, txdata->map);
	m_freem(txdata->m);
	txdata->m = NULL;
	ieee80211_free_node(txdata->ni);
	txdata->ni = NULL;

	ring->queued--;

	sc->sc_tx_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;
	wpi_start(ifp);
}

static void
wpi_cmd_intr(struct wpi_softc *sc, struct wpi_rx_desc *desc)
{
	struct wpi_tx_ring *ring = &sc->cmdq;
	struct wpi_tx_data *data;

	if ((desc->qid & 7) != 4)
		return;	/* not a command ack */

	data = &ring->data[desc->idx];

	/* if the command was mapped in a mbuf, free it */
	if (data->m != NULL) {
		bus_dmamap_unload(sc->sc_dmat, data->map);
		m_freem(data->m);
		data->m = NULL;
	}

	wakeup(&ring->cmd[desc->idx]);
}

static void
wpi_notif_intr(struct wpi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp =  ic->ic_ifp;
	struct wpi_rx_desc *desc;
	struct wpi_rx_data *data;
	uint32_t hw;

	hw = le32toh(sc->shared->next);
	while (sc->rxq.cur != hw) {
		data = &sc->rxq.data[sc->rxq.cur];

		desc = mtod(data->m, struct wpi_rx_desc *);

		DPRINTFN(4, ("rx notification qid=%x idx=%d flags=%x type=%d "
			"len=%d\n", desc->qid, desc->idx, desc->flags,
			desc->type, le32toh(desc->len)));

		if (!(desc->qid & 0x80))	/* reply to a command */
			wpi_cmd_intr(sc, desc);

		switch (desc->type) {
		case WPI_RX_DONE:
			/* a 802.11 frame was received */
			wpi_rx_intr(sc, desc, data);
			break;

		case WPI_TX_DONE:
			/* a 802.11 frame has been transmitted */
			wpi_tx_intr(sc, desc);
			break;

		case WPI_UC_READY:
		{
			struct wpi_ucode_info *uc =
				(struct wpi_ucode_info *)(desc + 1);

			/* the microcontroller is ready */
			DPRINTF(("microcode alive notification version %x "
				"alive %x\n", le32toh(uc->version),
				le32toh(uc->valid)));

			if (le32toh(uc->valid) != 1) {
				aprint_error("%s: microcontroller "
					"initialization failed\n",
					sc->sc_dev.dv_xname);
			}
			break;
		}
		case WPI_STATE_CHANGED:
		{
			uint32_t *status = (uint32_t *)(desc + 1);

			/* enabled/disabled notification */
			DPRINTF(("state changed to %x\n", le32toh(*status)));

			if (le32toh(*status) & 1) {
				/* the radio button has to be pushed */
				aprint_error("%s: Radio transmitter is off\n",
					sc->sc_dev.dv_xname);
				/* turn the interface down */
				ifp->if_flags &= ~IFF_UP;
				wpi_stop(ifp, 1);
				return;	/* no further processing */
			}
			break;
		}
		case WPI_START_SCAN:
		{
			struct wpi_start_scan *scan =
				(struct wpi_start_scan *)(desc + 1);

			DPRINTFN(2, ("scanning channel %d status %x\n",
				scan->chan, le32toh(scan->status)));

			/* fix current channel */
			ic->ic_bss->ni_chan = &ic->ic_channels[scan->chan];
			break;
		}
		case WPI_STOP_SCAN:
		{
			struct wpi_stop_scan *scan =
				(struct wpi_stop_scan *)(desc + 1);

			DPRINTF(("scan finished nchan=%d status=%d chan=%d\n",
				scan->nchan, scan->status, scan->chan));

			if (scan->status == 1 && scan->chan <= 14) {
				/*
				 * We just finished scanning 802.11g channels,
				 * start scanning 802.11a ones.
				 */
				if (wpi_scan(sc, IEEE80211_CHAN_A) == 0)
					break;
			}
			ieee80211_end_scan(ic);
			break;
		}
		}

		sc->rxq.cur = (sc->rxq.cur + 1) % WPI_RX_RING_COUNT;
	}

	/* tell the firmware what we have processed */
	hw = (hw == 0) ? WPI_RX_RING_COUNT - 1 : hw - 1;
	WPI_WRITE(sc, WPI_RX_WIDX, hw & ~7);
}

static int
wpi_intr(void *arg)
{
	struct wpi_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	uint32_t r;

	r = WPI_READ(sc, WPI_INTR);
	if (r == 0 || r == 0xffffffff)
		return 0;	/* not for us */

	DPRINTFN(5, ("interrupt reg %x\n", r));

	/* disable interrupts */
	WPI_WRITE(sc, WPI_MASK, 0);
	/* ack interrupts */
	WPI_WRITE(sc, WPI_INTR, r);

	if (r & (WPI_SW_ERROR | WPI_HW_ERROR)) {
		aprint_error("%s: fatal firmware error\n", sc->sc_dev.dv_xname);
		sc->sc_ic.ic_ifp->if_flags &= ~IFF_UP;
		wpi_stop(sc->sc_ic.ic_ifp, 1);
		return 1;
	}

	if (r & WPI_RX_INTR)
		wpi_notif_intr(sc);

	if (r & WPI_ALIVE_INTR)	/* firmware initialized */
		wakeup(sc);

	/* re-enable interrupts */
	if (ifp->if_flags & IFF_UP)
		WPI_WRITE(sc, WPI_MASK, WPI_INTR_MASK);

	return 1;
}

static uint8_t
wpi_plcp_signal(int rate)
{
	switch (rate) {
	/* CCK rates (returned values are device-dependent) */
	case 2:		return 10;
	case 4:		return 20;
	case 11:	return 55;
	case 22:	return 110;

	/* OFDM rates (cf IEEE Std 802.11a-1999, pp. 14 Table 80) */
	/* R1-R4, (u)ral is R4-R1 */
	case 12:	return 0xd;
	case 18:	return 0xf;
	case 24:	return 0x5;
	case 36:	return 0x7;
	case 48:	return 0x9;
	case 72:	return 0xb;
	case 96:	return 0x1;
	case 108:	return 0x3;

	/* unsupported rates (should not get there) */
	default:	return 0;
	}
}

/* quickly determine if a given rate is CCK or OFDM */
#define WPI_RATE_IS_OFDM(rate) ((rate) >= 12 && (rate) != 22)

static int
wpi_tx_data(struct wpi_softc *sc, struct mbuf *m0, struct ieee80211_node *ni,
	int ac)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_tx_ring *ring = &sc->txq[ac];
	struct wpi_tx_desc *desc;
	struct wpi_tx_data *data;
	struct wpi_tx_cmd *cmd;
	struct wpi_cmd_data *tx;
	struct ieee80211_frame *wh;
	struct ieee80211_key *k;
	const struct chanAccParams *cap;
	struct mbuf *mnew;
	int i, error, rate, hdrlen, noack = 0;

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	wh = mtod(m0, struct ieee80211_frame *);

	if (IEEE80211_QOS_HAS_SEQ(wh)) {
		hdrlen = sizeof (struct ieee80211_qosframe);
		cap = &ic->ic_wme.wme_chanParams;
		noack = cap->cap_wmeParams[ac].wmep_noackPolicy;
	} else
		hdrlen = sizeof (struct ieee80211_frame);

	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		k = ieee80211_crypto_encap(ic, ni, m0);
		if (k == NULL) {
			m_freem(m0);
			return ENOBUFS;
		}

		/* packet header may have moved, reset our local pointer */
		wh = mtod(m0, struct ieee80211_frame *);
	}

	/* pickup a rate */
	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) ==
		IEEE80211_FC0_TYPE_MGT) {
		/* mgmt frames are sent at the lowest available bit-rate */
		rate = ni->ni_rates.rs_rates[0];
	} else {
		if (ic->ic_fixed_rate != -1) {
			rate = ic->ic_sup_rates[ic->ic_curmode].
				rs_rates[ic->ic_fixed_rate];
		} else
			rate = ni->ni_rates.rs_rates[ni->ni_txrate];
	}
	rate &= IEEE80211_RATE_VAL;


#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct wpi_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_chan_freq = htole16(ni->ni_chan->ic_freq);
		tap->wt_chan_flags = htole16(ni->ni_chan->ic_flags);
		tap->wt_rate = rate;
		tap->wt_hwqueue = ac;
		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
			tap->wt_flags |= IEEE80211_RADIOTAP_F_WEP;

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_txtap_len, m0);
	}
#endif

	cmd = &ring->cmd[ring->cur];
	cmd->code = WPI_CMD_TX_DATA;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	tx = (struct wpi_cmd_data *)cmd->data;
	tx->flags = 0;

	if (!noack && !IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		tx->flags |= htole32(WPI_TX_NEED_ACK);
	} else if (m0->m_pkthdr.len + IEEE80211_CRC_LEN > ic->ic_rtsthreshold)
		tx->flags |= htole32(WPI_TX_NEED_RTS | WPI_TX_FULL_TXOP);

	tx->flags |= htole32(WPI_TX_AUTO_SEQ);

	/* retrieve destination node's id */
	tx->id = IEEE80211_IS_MULTICAST(wh->i_addr1) ? WPI_ID_BROADCAST :
		WPI_ID_BSS;

	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) ==
		IEEE80211_FC0_TYPE_MGT) {
		/* tell h/w to set timestamp in probe responses */
		if ((wh->i_fc[0] &
		    (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)) ==
		    (IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_PROBE_RESP))
			tx->flags |= htole32(WPI_TX_INSERT_TSTAMP);

		if (((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) ==
			 IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
			((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) ==
			 IEEE80211_FC0_SUBTYPE_REASSOC_REQ))
			tx->timeout = htole16(3);
		else
			tx->timeout = htole16(2);
	} else
		tx->timeout = htole16(0);

	tx->rate = wpi_plcp_signal(rate);

	/* be very persistant at sending frames out */
	tx->rts_ntries = 7;
	tx->data_ntries = 15;

	tx->ofdm_mask = 0xff;
	tx->cck_mask = 0xf;
	tx->lifetime = htole32(WPI_LIFETIME_INFINITE);

	tx->len = htole16(m0->m_pkthdr.len);

	/* save and trim IEEE802.11 header */
	m_copydata(m0, 0, hdrlen, (void *)&tx->wh);
	m_adj(m0, hdrlen);

	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m0,
		BUS_DMA_WRITE | BUS_DMA_NOWAIT);
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
		if (m0->m_pkthdr.len > MHLEN) {
			MCLGET(mnew, M_DONTWAIT);
			if (!(mnew->m_flags & M_EXT)) {
				m_freem(m0);
				m_freem(mnew);
				return ENOMEM;
			}
		}

		m_copydata(m0, 0, m0->m_pkthdr.len, mtod(mnew, void *));
		m_freem(m0);
		mnew->m_len = mnew->m_pkthdr.len;
		m0 = mnew;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m0,
			BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (error != 0) {
			aprint_error("%s: could not map mbuf (error %d)\n",
				sc->sc_dev.dv_xname, error);
			m_freem(m0);
			return error;
		}
	}

	data->m = m0;
	data->ni = ni;

	DPRINTFN(4, ("sending data: qid=%d idx=%d len=%d nsegs=%d\n",
		ring->qid, ring->cur, m0->m_pkthdr.len, data->map->dm_nsegs));

	/* first scatter/gather segment is used by the tx data command */
	desc->flags = htole32(WPI_PAD32(m0->m_pkthdr.len) << 28 |
		(1 + data->map->dm_nsegs) << 24);
	desc->segs[0].addr = htole32(ring->cmd_dma.paddr +
		ring->cur * sizeof (struct wpi_tx_cmd));
	/*XXX The next line might be wrong. I don't use hdrlen*/
	desc->segs[0].len  = htole32(4 + sizeof (struct wpi_cmd_data));

	for (i = 1; i <= data->map->dm_nsegs; i++) {
		desc->segs[i].addr =
			htole32(data->map->dm_segs[i - 1].ds_addr);
		desc->segs[i].len  =
			htole32(data->map->dm_segs[i - 1].ds_len);
	}

	ring->queued++;

	/* kick ring */
	ring->cur = (ring->cur + 1) % WPI_TX_RING_COUNT;
	WPI_WRITE(sc, WPI_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;
}

static void
wpi_start(struct ifnet *ifp)
{
	struct wpi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ether_header *eh;
	struct mbuf *m0;
	int ac;

	/*
	 * net80211 may still try to send management frames even if the
	 * IFF_RUNNING flag is not set...
	 */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IF_POLL(&ic->ic_mgtq, m0);
		if (m0 != NULL) {
			IF_DEQUEUE(&ic->ic_mgtq, m0);

			ni = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
			m0->m_pkthdr.rcvif = NULL;

			/* management frames go into ring 0 */
			if (sc->txq[0].queued > sc->txq[0].count - 8) {
				ifp->if_oerrors++;
				continue;
			}
#if NBPFILTER > 0
			if (ic->ic_rawbpf != NULL)
				bpf_mtap(ic->ic_rawbpf, m0);
#endif
			if (wpi_tx_data(sc, m0, ni, 0) != 0) {
				ifp->if_oerrors++;
				break;
			}
		} else {
			if (ic->ic_state != IEEE80211_S_RUN)
				break;
			IFQ_POLL(&ifp->if_snd, m0);
			if (m0 == NULL)
				break;

			if (m0->m_len < sizeof (*eh) &&
			    (m0 = m_pullup(m0, sizeof (*eh))) != NULL) {
				ifp->if_oerrors++;
				continue;
			}
			eh = mtod(m0, struct ether_header *);
			ni = ieee80211_find_txnode(ic, eh->ether_dhost);
			if (ni == NULL) {
				m_freem(m0);
				ifp->if_oerrors++;
				continue;
			}

			/* classify mbuf so we can find which tx ring to use */
			if (ieee80211_classify(ic, m0, ni) != 0) {
				m_freem(m0);
				ieee80211_free_node(ni);
				ifp->if_oerrors++;
				continue;
			}

			/* no QoS encapsulation for EAPOL frames */
			ac = (eh->ether_type != htons(ETHERTYPE_PAE)) ?
			    M_WME_GETAC(m0) : WME_AC_BE;

			if (sc->txq[ac].queued > sc->txq[ac].count - 8) {
				/* there is no place left in this ring */
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IFQ_DEQUEUE(&ifp->if_snd, m0);
#if NBPFILTER > 0
			if (ifp->if_bpf != NULL)
				bpf_mtap(ifp->if_bpf, m0);
#endif
			m0 = ieee80211_encap(ic, m0, ni);
			if (m0 == NULL) {
				ieee80211_free_node(ni);
				ifp->if_oerrors++;
				continue;
			}
#if NBPFILTER > 0
			if (ic->ic_rawbpf != NULL)
				bpf_mtap(ic->ic_rawbpf, m0);
#endif
			if (wpi_tx_data(sc, m0, ni, ac) != 0) {
				ieee80211_free_node(ni);
				ifp->if_oerrors++;
				break;
			}
		}

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

static void
wpi_watchdog(struct ifnet *ifp)
{
	struct wpi_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			aprint_error("%s: device timeout\n",
				sc->sc_dev.dv_xname);
			ifp->if_oerrors++;
			ifp->if_flags &= ~IFF_UP;
			wpi_stop(ifp, 1);
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(&sc->sc_ic);
}

static int
wpi_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
#define IS_RUNNING(ifp) \
	((ifp->if_flags & IFF_UP) && (ifp->if_flags & IFF_RUNNING))

	struct wpi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING))
				wpi_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				wpi_stop(ifp, 1);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
			ether_addmulti(ifr, &sc->sc_ec) :
			ether_delmulti(ifr, &sc->sc_ec);
		if (error == ENETRESET) {
			/* setup multicast filter, etc */
			error = 0;
		}
		break;

	default:
		error = ieee80211_ioctl(ic, cmd, data);
	}

	if (error == ENETRESET) {
		if (IS_RUNNING(ifp) &&
			(ic->ic_roaming != IEEE80211_ROAMING_MANUAL))
			wpi_init(ifp);
		error = 0;
	}

	splx(s);
	return error;

#undef IS_RUNNING
}

/*
 * Extract various information from EEPROM.
 */
static void
wpi_read_eeprom(struct wpi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	char domain[4];
	int i;

	wpi_read_prom_data(sc, WPI_EEPROM_CAPABILITIES, &sc->cap, 1);
	wpi_read_prom_data(sc, WPI_EEPROM_REVISION, &sc->rev, 2);
	wpi_read_prom_data(sc, WPI_EEPROM_TYPE, &sc->type, 1);

	DPRINTF(("cap=%x rev=%x type=%x\n", sc->cap, le16toh(sc->rev),
	    sc->type));

	/* read and print regulatory domain */
	wpi_read_prom_data(sc, WPI_EEPROM_DOMAIN, domain, 4);
	aprint_normal(", %.4s", domain);

	/* read and print MAC address */
	wpi_read_prom_data(sc, WPI_EEPROM_MAC, ic->ic_myaddr, 6);
	aprint_normal(", address %s\n", ether_sprintf(ic->ic_myaddr));

	/* read the list of authorized channels */
	for (i = 0; i < WPI_CHAN_BANDS_COUNT; i++)
		wpi_read_eeprom_channels(sc, i);

	/* read the list of power groups */
	for (i = 0; i < WPI_POWER_GROUPS_COUNT; i++)
		wpi_read_eeprom_group(sc, i);
}

static void
wpi_read_eeprom_channels(struct wpi_softc *sc, int n)
{
	struct ieee80211com *ic = &sc->sc_ic;
	const struct wpi_chan_band *band = &wpi_bands[n];
	struct wpi_eeprom_chan channels[WPI_MAX_CHAN_PER_BAND];
	int chan, i;

	wpi_read_prom_data(sc, band->addr, channels,
	    band->nchan * sizeof (struct wpi_eeprom_chan));

	for (i = 0; i < band->nchan; i++) {
		if (!(channels[i].flags & WPI_EEPROM_CHAN_VALID))
			continue;

		chan = band->chan[i];

		if (n == 0) {	/* 2GHz band */
			ic->ic_channels[chan].ic_freq =
			    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_2GHZ);
			ic->ic_channels[chan].ic_flags =
			    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
			    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;

		} else {	/* 5GHz band */
			/*
			 * Some 3945abg adapters support channels 7, 8, 11
			 * and 12 in the 2GHz *and* 5GHz bands.
			 * Because of limitations in our net80211(9) stack,
			 * we can't support these channels in 5GHz band.
			 */
			if (chan <= 14)
				continue;

			ic->ic_channels[chan].ic_freq =
			    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_5GHZ);
			ic->ic_channels[chan].ic_flags = IEEE80211_CHAN_A;
		}

		/* is active scan allowed on this channel? */
		if (!(channels[i].flags & WPI_EEPROM_CHAN_ACTIVE)) {
			ic->ic_channels[chan].ic_flags |=
			    IEEE80211_CHAN_PASSIVE;
		}

		/* save maximum allowed power for this channel */
		sc->maxpwr[chan] = channels[i].maxpwr;

		DPRINTF(("adding chan %d flags=0x%x maxpwr=%d\n",
		    chan, channels[i].flags, sc->maxpwr[chan]));
	}
}

static void
wpi_read_eeprom_group(struct wpi_softc *sc, int n)
{
	struct wpi_power_group *group = &sc->groups[n];
	struct wpi_eeprom_group rgroup;
	int i;

	wpi_read_prom_data(sc, WPI_EEPROM_POWER_GRP + n * 32, &rgroup,
	    sizeof rgroup);

	/* save power group information */
	group->chan   = rgroup.chan;
	group->maxpwr = rgroup.maxpwr;
	/* temperature at which the samples were taken */
	group->temp   = (int16_t)le16toh(rgroup.temp);

	DPRINTF(("power group %d: chan=%d maxpwr=%d temp=%d\n", n,
	    group->chan, group->maxpwr, group->temp));

	for (i = 0; i < WPI_SAMPLES_COUNT; i++) {
		group->samples[i].index = rgroup.samples[i].index;
		group->samples[i].power = rgroup.samples[i].power;

		DPRINTF(("\tsample %d: index=%d power=%d\n", i,
		    group->samples[i].index, group->samples[i].power));
	}
}

/*
 * Send a command to the firmware.
 */
static int
wpi_cmd(struct wpi_softc *sc, int code, const void *buf, int size, int async)
{
	struct wpi_tx_ring *ring = &sc->cmdq;
	struct wpi_tx_desc *desc;
	struct wpi_tx_cmd *cmd;

	KASSERT(size <= sizeof cmd->data);

	desc = &ring->desc[ring->cur];
	cmd = &ring->cmd[ring->cur];

	cmd->code = code;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;
	memcpy(cmd->data, buf, size);

	desc->flags = htole32(WPI_PAD32(size) << 28 | 1 << 24);
	desc->segs[0].addr = htole32(ring->cmd_dma.paddr +
		ring->cur * sizeof (struct wpi_tx_cmd));
	desc->segs[0].len  = htole32(4 + size);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % WPI_CMD_RING_COUNT;
	WPI_WRITE(sc, WPI_TX_WIDX, ring->qid << 8 | ring->cur);

	return async ? 0 : tsleep(cmd, PCATCH, "wpicmd", hz);
}

static int
wpi_wme_update(struct ieee80211com *ic)
{
#define WPI_EXP2(v)	htole16((1 << (v)) - 1)
#define WPI_USEC(v)	htole16(IEEE80211_TXOP_TO_US(v))
	struct wpi_softc *sc = ic->ic_ifp->if_softc;
	const struct wmeParams *wmep;
	struct wpi_wme_setup wme;
	int ac;

	/* don't override default WME values if WME is not actually enabled */
	if (!(ic->ic_flags & IEEE80211_F_WME))
		return 0;

	wme.flags = 0;
	for (ac = 0; ac < WME_NUM_AC; ac++) {
		wmep = &ic->ic_wme.wme_chanParams.cap_wmeParams[ac];
		wme.ac[ac].aifsn = wmep->wmep_aifsn;
		wme.ac[ac].cwmin = WPI_EXP2(wmep->wmep_logcwmin);
		wme.ac[ac].cwmax = WPI_EXP2(wmep->wmep_logcwmax);
		wme.ac[ac].txop  = WPI_USEC(wmep->wmep_txopLimit);

		DPRINTF(("setting WME for queue %d aifsn=%d cwmin=%d cwmax=%d "
		    "txop=%d\n", ac, wme.ac[ac].aifsn, wme.ac[ac].cwmin,
		    wme.ac[ac].cwmax, wme.ac[ac].txop));
	}

	return wpi_cmd(sc, WPI_CMD_SET_WME, &wme, sizeof wme, 1);
#undef WPI_USEC
#undef WPI_EXP2
}

/*
 * Configure h/w multi-rate retries.
 */
static int
wpi_mrr_setup(struct wpi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_mrr_setup mrr;
	int i, error;

	/* CCK rates (not used with 802.11a) */
	for (i = WPI_CCK1; i <= WPI_CCK11; i++) {
		mrr.rates[i].flags = 0;
		mrr.rates[i].plcp = wpi_ridx_to_plcp[i];
		/* fallback to the immediate lower CCK rate (if any) */
		mrr.rates[i].next = (i == WPI_CCK1) ? WPI_CCK1 : i - 1;
		/* try one time at this rate before falling back to "next" */
		mrr.rates[i].ntries = 1;
	}

	/* OFDM rates (not used with 802.11b) */
	for (i = WPI_OFDM6; i <= WPI_OFDM54; i++) {
		mrr.rates[i].flags = 0;
		mrr.rates[i].plcp = wpi_ridx_to_plcp[i];
		/* fallback to the immediate lower rate (if any) */
		/* we allow fallback from OFDM/6 to CCK/2 in 11b/g mode */
		mrr.rates[i].next = (i == WPI_OFDM6) ?
		    ((ic->ic_curmode == IEEE80211_MODE_11A) ?
			WPI_OFDM6 : WPI_CCK2) :
		    i - 1;
		/* try one time at this rate before falling back to "next" */
		mrr.rates[i].ntries = 1;
	}

	/* setup MRR for control frames */
	mrr.which = htole32(WPI_MRR_CTL);
	error = wpi_cmd(sc, WPI_CMD_MRR_SETUP, &mrr, sizeof mrr, 0);
	if (error != 0) {
		aprint_error("%s: could not setup MRR for control frames\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	/* setup MRR for data frames */
	mrr.which = htole32(WPI_MRR_DATA);
	error = wpi_cmd(sc, WPI_CMD_MRR_SETUP, &mrr, sizeof mrr, 0);
	if (error != 0) {
		aprint_error("%s: could not setup MRR for data frames\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	return 0;
}

static void
wpi_set_led(struct wpi_softc *sc, uint8_t which, uint8_t off, uint8_t on)
{
	struct wpi_cmd_led led;

	led.which = which;
	led.unit = htole32(100000);	/* on/off in unit of 100ms */
	led.off = off;
	led.on = on;

	(void)wpi_cmd(sc, WPI_CMD_SET_LED, &led, sizeof led, 1);
}

static void
wpi_enable_tsf(struct wpi_softc *sc, struct ieee80211_node *ni)
{
	struct wpi_cmd_tsf tsf;
	uint64_t val, mod;

	memset(&tsf, 0, sizeof tsf);
	memcpy(&tsf.tstamp, ni->ni_tstamp.data, 8);
	tsf.bintval = htole16(ni->ni_intval);
	tsf.lintval = htole16(10);

	/* compute remaining time until next beacon */
	val = (uint64_t)ni->ni_intval  * 1024;	/* msecs -> usecs */
	mod = le64toh(tsf.tstamp) % val;
	tsf.binitval = htole32((uint32_t)(val - mod));

	DPRINTF(("TSF bintval=%u tstamp=%" PRId64 ", init=%u\n",
	    ni->ni_intval, le64toh(tsf.tstamp), (uint32_t)(val - mod)));

	if (wpi_cmd(sc, WPI_CMD_TSF, &tsf, sizeof tsf, 1) != 0)
		aprint_error("%s: could not enable TSF\n", sc->sc_dev.dv_xname);
}

/*
 * Update Tx power to match what is defined for channel `c'.
 */
static int
wpi_set_txpower(struct wpi_softc *sc, struct ieee80211_channel *c, int async)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_power_group *group;
	struct wpi_cmd_txpower txpower;
	u_int chan;
	int i;

	/* get channel number */
	chan = ieee80211_chan2ieee(ic, c);

	/* find the power group to which this channel belongs */
	if (IEEE80211_IS_CHAN_5GHZ(c)) {
		for (group = &sc->groups[1]; group < &sc->groups[4]; group++)
			if (chan <= group->chan)
				break;
	} else
		group = &sc->groups[0];

	memset(&txpower, 0, sizeof txpower);
	txpower.band = IEEE80211_IS_CHAN_5GHZ(c) ? 0 : 1;
	txpower.chan = htole16(chan);

	/* set Tx power for all OFDM and CCK rates */
	for (i = 0; i <= 11 ; i++) {
		/* retrieve Tx power for this channel/rate combination */
		int idx = wpi_get_power_index(sc, group, c,
		    wpi_ridx_to_rate[i]);

		txpower.rates[i].plcp = wpi_ridx_to_plcp[i];

		if (IEEE80211_IS_CHAN_5GHZ(c)) {
			txpower.rates[i].rf_gain = wpi_rf_gain_5ghz[idx];
			txpower.rates[i].dsp_gain = wpi_dsp_gain_5ghz[idx];
		} else {
			txpower.rates[i].rf_gain = wpi_rf_gain_2ghz[idx];
			txpower.rates[i].dsp_gain = wpi_dsp_gain_2ghz[idx];
		}
		DPRINTF(("chan %d/rate %d: power index %d\n", chan,
		    wpi_ridx_to_rate[i], idx));
	}

	return wpi_cmd(sc, WPI_CMD_TXPOWER, &txpower, sizeof txpower, async);
}

/*
 * Determine Tx power index for a given channel/rate combination.
 * This takes into account the regulatory information from EEPROM and the
 * current temperature.
 */
static int
wpi_get_power_index(struct wpi_softc *sc, struct wpi_power_group *group,
    struct ieee80211_channel *c, int rate)
{
/* fixed-point arithmetic division using a n-bit fractional part */
#define fdivround(a, b, n)	\
	((((1 << n) * (a)) / (b) + (1 << n) / 2) / (1 << n))

/* linear interpolation */
#define interpolate(x, x1, y1, x2, y2, n)	\
	((y1) + fdivround(((x) - (x1)) * ((y2) - (y1)), (x2) - (x1), n))

	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_power_sample *sample;
	int pwr, idx;
	u_int chan;

	/* get channel number */
	chan = ieee80211_chan2ieee(ic, c);

	/* default power is group's maximum power - 3dB */
	pwr = group->maxpwr / 2;

	/* decrease power for highest OFDM rates to reduce distortion */
	switch (rate) {
	case 72:	/* 36Mb/s */
		pwr -= IEEE80211_IS_CHAN_2GHZ(c) ? 0 :  5;
		break;
	case 96:	/* 48Mb/s */
		pwr -= IEEE80211_IS_CHAN_2GHZ(c) ? 7 : 10;
		break;
	case 108:	/* 54Mb/s */
		pwr -= IEEE80211_IS_CHAN_2GHZ(c) ? 9 : 12;
		break;
	}

	/* never exceed channel's maximum allowed Tx power */
	pwr = min(pwr, sc->maxpwr[chan]);

	/* retrieve power index into gain tables from samples */
	for (sample = group->samples; sample < &group->samples[3]; sample++)
		if (pwr > sample[1].power)
			break;
	/* fixed-point linear interpolation using a 19-bit fractional part */
	idx = interpolate(pwr, sample[0].power, sample[0].index,
	    sample[1].power, sample[1].index, 19);

	/*
	 * Adjust power index based on current temperature:
	 * - if cooler than factory-calibrated: decrease output power
	 * - if warmer than factory-calibrated: increase output power
	 */
	idx -= (sc->temp - group->temp) * 11 / 100;

	/* decrease power for CCK rates (-5dB) */
	if (!WPI_RATE_IS_OFDM(rate))
		idx += 10;

	/* keep power index in a valid range */
	if (idx < 0)
		return 0;
	if (idx > WPI_MAX_PWR_INDEX)
		return WPI_MAX_PWR_INDEX;
	return idx;

#undef interpolate
#undef fdivround
}

/*
 * Build a beacon frame that the firmware will broadcast periodically in
 * IBSS or HostAP modes.
 */
static int
wpi_setup_beacon(struct wpi_softc *sc, struct ieee80211_node *ni)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_tx_ring *ring = &sc->cmdq;
	struct wpi_tx_desc *desc;
	struct wpi_tx_data *data;
	struct wpi_tx_cmd *cmd;
	struct wpi_cmd_beacon *bcn;
	struct ieee80211_beacon_offsets bo;
	struct mbuf *m0;
	int error;

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	m0 = ieee80211_beacon_alloc(ic, ni, &bo);
	if (m0 == NULL) {
		aprint_error("%s: could not allocate beacon frame\n",
			sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	cmd = &ring->cmd[ring->cur];
	cmd->code = WPI_CMD_SET_BEACON;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	bcn = (struct wpi_cmd_beacon *)cmd->data;
	memset(bcn, 0, sizeof (struct wpi_cmd_beacon));
	bcn->id = WPI_ID_BROADCAST;
	bcn->ofdm_mask = 0xff;
	bcn->cck_mask = 0x0f;
	bcn->lifetime = htole32(WPI_LIFETIME_INFINITE);
	bcn->len = htole16(m0->m_pkthdr.len);
	bcn->rate = (ic->ic_curmode == IEEE80211_MODE_11A) ?
		wpi_plcp_signal(12) : wpi_plcp_signal(2);
	bcn->flags = htole32(WPI_TX_AUTO_SEQ | WPI_TX_INSERT_TSTAMP);

	/* save and trim IEEE802.11 header */
	m_copydata(m0, 0, sizeof (struct ieee80211_frame), (void *)&bcn->wh);
	m_adj(m0, sizeof (struct ieee80211_frame));

	/* assume beacon frame is contiguous */
	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m0,
		BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: could not map beacon\n", sc->sc_dev.dv_xname);
		m_freem(m0);
		return error;
	}

	data->m = m0;

	/* first scatter/gather segment is used by the beacon command */
	desc->flags = htole32(WPI_PAD32(m0->m_pkthdr.len) << 28 | 2 << 24);
	desc->segs[0].addr = htole32(ring->cmd_dma.paddr +
		ring->cur * sizeof (struct wpi_tx_cmd));
	desc->segs[0].len  = htole32(4 + sizeof (struct wpi_cmd_beacon));
	desc->segs[1].addr = htole32(data->map->dm_segs[0].ds_addr);
	desc->segs[1].len  = htole32(data->map->dm_segs[0].ds_len);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % WPI_CMD_RING_COUNT;
	WPI_WRITE(sc, WPI_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;
}

static int
wpi_auth(struct wpi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct wpi_node_info node;
	int error;

	/* update adapter's configuration */
	IEEE80211_ADDR_COPY(sc->config.bssid, ni->ni_bssid);
	sc->config.chan = ieee80211_chan2ieee(ic, ni->ni_chan);
	sc->config.flags = htole32(WPI_CONFIG_TSF);
	if (IEEE80211_IS_CHAN_2GHZ(ni->ni_chan)) {
		sc->config.flags |= htole32(WPI_CONFIG_AUTO |
		    WPI_CONFIG_24GHZ);
	}
	switch (ic->ic_curmode) {
	case IEEE80211_MODE_11A:
		sc->config.cck_mask  = 0;
		sc->config.ofdm_mask = 0x15;
		break;
	case IEEE80211_MODE_11B:
		sc->config.cck_mask  = 0x03;
		sc->config.ofdm_mask = 0;
		break;
	default:	/* assume 802.11b/g */
		sc->config.cck_mask  = 0x0f;
		sc->config.ofdm_mask = 0x15;
	}

	DPRINTF(("config chan %d flags %x cck %x ofdm %x\n", sc->config.chan,
		sc->config.flags, sc->config.cck_mask, sc->config.ofdm_mask));
	error = wpi_cmd(sc, WPI_CMD_CONFIGURE, &sc->config,
		sizeof (struct wpi_config), 1);
	if (error != 0) {
		aprint_error("%s: could not configure\n", sc->sc_dev.dv_xname);
		return error;
	}

	/* configuration has changed, set Tx power accordingly */
	if ((error = wpi_set_txpower(sc, ni->ni_chan, 1)) != 0) {
		aprint_error("%s: could not set Tx power\n", sc->sc_dev.dv_xname);
		return error;
	}

	/* add default node */
	memset(&node, 0, sizeof node);
	IEEE80211_ADDR_COPY(node.bssid, ni->ni_bssid);
	node.id = WPI_ID_BSS;
	node.rate = (ic->ic_curmode == IEEE80211_MODE_11A) ?
	    wpi_plcp_signal(12) : wpi_plcp_signal(2);
	node.action = htole32(WPI_ACTION_SET_RATE);
	node.antenna = WPI_ANTENNA_BOTH;
	error = wpi_cmd(sc, WPI_CMD_ADD_NODE, &node, sizeof node, 1);
	if (error != 0) {
		aprint_error("%s: could not add BSS node\n", sc->sc_dev.dv_xname);
		return error;
	}

	return 0;
}

/*
 * Send a scan request to the firmware.  Since this command is huge, we map it
 * into a mbuf instead of using the pre-allocated set of commands.
 */
static int
wpi_scan(struct wpi_softc *sc, uint16_t flags)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct wpi_tx_ring *ring = &sc->cmdq;
	struct wpi_tx_desc *desc;
	struct wpi_tx_data *data;
	struct wpi_tx_cmd *cmd;
	struct wpi_scan_hdr *hdr;
	struct wpi_scan_chan *chan;
	struct ieee80211_frame *wh;
	struct ieee80211_rateset *rs;
	struct ieee80211_channel *c;
	enum ieee80211_phymode mode;
	uint8_t *frm;
	int nrates, pktlen, error;

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	MGETHDR(data->m, M_DONTWAIT, MT_DATA);
	if (data->m == NULL) {
		aprint_error("%s: could not allocate mbuf for scan command\n",
			sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	MCLGET(data->m, M_DONTWAIT);
	if (!(data->m->m_flags & M_EXT)) {
		m_freem(data->m);
		data->m = NULL;
		aprint_error("%s: could not allocate mbuf for scan command\n",
			sc->sc_dev.dv_xname);
		return ENOMEM;
	}

	cmd = mtod(data->m, struct wpi_tx_cmd *);
	cmd->code = WPI_CMD_SCAN;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	hdr = (struct wpi_scan_hdr *)cmd->data;
	memset(hdr, 0, sizeof (struct wpi_scan_hdr));
	hdr->txflags = htole32(WPI_TX_AUTO_SEQ);
	hdr->id = WPI_ID_BROADCAST;
	hdr->lifetime = htole32(WPI_LIFETIME_INFINITE);

	/*
	 * Move to the next channel if no packets are received within 5 msecs
	 * after sending the probe request (this helps to reduce the duration
	 * of active scans).
	 */
	hdr->quiet = htole16(5);        /* timeout in milliseconds */
	hdr->plcp_threshold = htole16(1);	/* min # of packets */

	if (flags & IEEE80211_CHAN_A) {
		hdr->crc_threshold = htole16(1);
		/* send probe requests at 6Mbps */
		hdr->rate = wpi_plcp_signal(12);
	} else {
		hdr->flags = htole32(WPI_CONFIG_24GHZ | WPI_CONFIG_AUTO);
		/* send probe requests at 1Mbps */
		hdr->rate = wpi_plcp_signal(2);
	}

	/* for directed scans, firmware inserts the essid IE itself */
	hdr->essid[0].id  = IEEE80211_ELEMID_SSID;
	hdr->essid[0].len = ic->ic_des_esslen;
	memcpy(hdr->essid[0].data, ic->ic_des_essid, ic->ic_des_esslen);

	/*
	 * Build a probe request frame.  Most of the following code is a
	 * copy & paste of what is done in net80211.
	 */
	wh = (struct ieee80211_frame *)(hdr + 1);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT |
		IEEE80211_FC0_SUBTYPE_PROBE_REQ;
	wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
	IEEE80211_ADDR_COPY(wh->i_addr1, etherbroadcastaddr);
	IEEE80211_ADDR_COPY(wh->i_addr2, ic->ic_myaddr);
	IEEE80211_ADDR_COPY(wh->i_addr3, etherbroadcastaddr);
	*(u_int16_t *)&wh->i_dur[0] = 0;	/* filled by h/w */
	*(u_int16_t *)&wh->i_seq[0] = 0;	/* filled by h/w */

	frm = (uint8_t *)(wh + 1);

#ifdef old_code
	/* add essid IE */
	*frm++ = IEEE80211_ELEMID_SSID;
	*frm++ = ic->ic_des_esslen;
	memcpy(frm, ic->ic_des_essid, ic->ic_des_esslen);
	frm += ic->ic_des_esslen;
#else
	/* add empty essid IE (firmware generates it for directed scans) */
	*frm++ = IEEE80211_ELEMID_SSID;
	*frm++ = 0;
#endif

	mode = ieee80211_chan2mode(ic, ic->ic_ibss_chan);
	rs = &ic->ic_sup_rates[mode];

	/* add supported rates IE */
	*frm++ = IEEE80211_ELEMID_RATES;
	nrates = rs->rs_nrates;
	if (nrates > IEEE80211_RATE_SIZE)
		nrates = IEEE80211_RATE_SIZE;
	*frm++ = nrates;
	memcpy(frm, rs->rs_rates, nrates);
	frm += nrates;

	/* add supported xrates IE */
	if (rs->rs_nrates > IEEE80211_RATE_SIZE) {
		nrates = rs->rs_nrates - IEEE80211_RATE_SIZE;
		*frm++ = IEEE80211_ELEMID_XRATES;
		*frm++ = nrates;
		memcpy(frm, rs->rs_rates + IEEE80211_RATE_SIZE, nrates);
		frm += nrates;
	}

	/* setup length of probe request */
	hdr->paylen = htole16(frm - (uint8_t *)wh);

	chan = (struct wpi_scan_chan *)frm;
	for (c  = &ic->ic_channels[1];
	     c <= &ic->ic_channels[IEEE80211_CHAN_MAX]; c++) {
		if ((c->ic_flags & flags) != flags)
			continue;

		chan->chan = ieee80211_chan2ieee(ic, c);
		chan->flags = 0;
		if (!(c->ic_flags & IEEE80211_CHAN_PASSIVE)) {
			chan->flags |= WPI_CHAN_ACTIVE;
			if (ic->ic_des_esslen != 0)
				chan->flags |= WPI_CHAN_DIRECT;
		}
		chan->dsp_gain = 0x6e;
		if (IEEE80211_IS_CHAN_5GHZ(c)) {
			chan->rf_gain = 0x3b;
			chan->active = htole16(10);
			chan->passive = htole16(110);
		} else {
			chan->rf_gain = 0x28;
			chan->active = htole16(20);
			chan->passive = htole16(120);
		}
		hdr->nchan++;
		chan++;

		frm += sizeof (struct wpi_scan_chan);
	}
	hdr->len = htole16(frm - (uint8_t *)hdr);
	pktlen = frm - (uint8_t *)cmd;

	error = bus_dmamap_load(sc->sc_dmat, data->map, cmd, pktlen,
		NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error("%s: could not map scan command\n",
			sc->sc_dev.dv_xname);
		m_freem(data->m);
		data->m = NULL;
		return error;
	}

	desc->flags = htole32(WPI_PAD32(pktlen) << 28 | 1 << 24);
	desc->segs[0].addr = htole32(data->map->dm_segs[0].ds_addr);
	desc->segs[0].len  = htole32(data->map->dm_segs[0].ds_len);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % WPI_CMD_RING_COUNT;
	WPI_WRITE(sc, WPI_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;	/* will be notified async. of failure/success */
}

static int
wpi_config(struct wpi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct wpi_power power;
	struct wpi_bluetooth bluetooth;
	struct wpi_node_info node;
	int error;

	memset(&power, 0, sizeof power);
	power.flags = htole32(WPI_POWER_CAM | 0x8);
	error = wpi_cmd(sc, WPI_CMD_SET_POWER_MODE, &power, sizeof power, 0);
	if (error != 0) {
		aprint_error("%s: could not set power mode\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	/* configure bluetooth coexistence */
	memset(&bluetooth, 0, sizeof bluetooth);
	bluetooth.flags = 3;
	bluetooth.lead = 0xaa;
	bluetooth.kill = 1;
	error = wpi_cmd(sc, WPI_CMD_BLUETOOTH, &bluetooth, sizeof bluetooth,
		0);
	if (error != 0) {
		aprint_error(
			"%s: could not configure bluetooth coexistence\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	/* configure adapter */
	memset(&sc->config, 0, sizeof (struct wpi_config));
	IEEE80211_ADDR_COPY(ic->ic_myaddr, LLADDR(ifp->if_sadl));
	IEEE80211_ADDR_COPY(sc->config.myaddr, ic->ic_myaddr);
	/*set default channel*/
	sc->config.chan = ieee80211_chan2ieee(ic, ic->ic_ibss_chan);
	sc->config.flags = htole32(WPI_CONFIG_TSF);
	if (IEEE80211_IS_CHAN_2GHZ(ic->ic_ibss_chan)) {
		sc->config.flags |= htole32(WPI_CONFIG_AUTO |
		    WPI_CONFIG_24GHZ);
	}
	sc->config.filter = 0;
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		sc->config.mode = WPI_MODE_STA;
		sc->config.filter |= htole32(WPI_FILTER_MULTICAST);
		break;
	case IEEE80211_M_IBSS:
	case IEEE80211_M_AHDEMO:
		sc->config.mode = WPI_MODE_IBSS;
		break;
	case IEEE80211_M_HOSTAP:
		sc->config.mode = WPI_MODE_HOSTAP;
		break;
	case IEEE80211_M_MONITOR:
		sc->config.mode = WPI_MODE_MONITOR;
		sc->config.filter |= htole32(WPI_FILTER_MULTICAST |
			WPI_FILTER_CTL | WPI_FILTER_PROMISC);
		break;
	}
	sc->config.cck_mask  = 0x0f;	/* not yet negotiated */
	sc->config.ofdm_mask = 0xff;	/* not yet negotiated */
	error = wpi_cmd(sc, WPI_CMD_CONFIGURE, &sc->config,
		sizeof (struct wpi_config), 0);
	if (error != 0) {
		aprint_error("%s: configure command failed\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	/* configuration has changed, set Tx power accordingly */
	if ((error = wpi_set_txpower(sc, ic->ic_ibss_chan, 0)) != 0) {
		aprint_error("%s: could not set Tx power\n", sc->sc_dev.dv_xname);
		return error;
	}

	/* add broadcast node */
	memset(&node, 0, sizeof node);
	IEEE80211_ADDR_COPY(node.bssid, etherbroadcastaddr);
	node.id = WPI_ID_BROADCAST;
	node.rate = wpi_plcp_signal(2);
	node.action = htole32(WPI_ACTION_SET_RATE);
	node.antenna = WPI_ANTENNA_BOTH;	
	error = wpi_cmd(sc, WPI_CMD_ADD_NODE, &node, sizeof node, 0);
	if (error != 0) {
		aprint_error("%s: could not add broadcast node\n",
			sc->sc_dev.dv_xname);
		return error;
	}

	if ((error = wpi_mrr_setup(sc)) != 0) {
		aprint_error("%s: could not setup MRR\n", sc->sc_dev.dv_xname);
		return error;
	}

	return 0;
}

static void
wpi_stop_master(struct wpi_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = WPI_READ(sc, WPI_RESET);
	WPI_WRITE(sc, WPI_RESET, tmp | WPI_STOP_MASTER);

	tmp = WPI_READ(sc, WPI_GPIO_CTL);
	if ((tmp & WPI_GPIO_PWR_STATUS) == WPI_GPIO_PWR_SLEEP)
		return;	/* already asleep */

	for (ntries = 0; ntries < 100; ntries++) {
		if (WPI_READ(sc, WPI_RESET) & WPI_MASTER_DISABLED)
			break;
		DELAY(10);
	}
	if (ntries == 100) {
		aprint_error("%s: timeout waiting for master\n",
			sc->sc_dev.dv_xname);
	}
}

static int
wpi_power_up(struct wpi_softc *sc)
{
	uint32_t tmp;
	int ntries;

	wpi_mem_lock(sc);
	tmp = wpi_mem_read(sc, WPI_MEM_POWER);
	wpi_mem_write(sc, WPI_MEM_POWER, tmp & ~0x03000000);
	wpi_mem_unlock(sc);

	for (ntries = 0; ntries < 5000; ntries++) {
		if (WPI_READ(sc, WPI_GPIO_STATUS) & WPI_POWERED)
			break;
		DELAY(10);
	}
	if (ntries == 5000) {
		aprint_error("%s: timeout waiting for NIC to power up\n",
			sc->sc_dev.dv_xname);
		return ETIMEDOUT;
	}
	return 0;
}

static int
wpi_reset(struct wpi_softc *sc)
{
	uint32_t tmp;
	int ntries;

	/* clear any pending interrupts */
	WPI_WRITE(sc, WPI_INTR, 0xffffffff);

	tmp = WPI_READ(sc, WPI_PLL_CTL);
	WPI_WRITE(sc, WPI_PLL_CTL, tmp | WPI_PLL_INIT);

	tmp = WPI_READ(sc, WPI_CHICKEN);
	WPI_WRITE(sc, WPI_CHICKEN, tmp | WPI_CHICKEN_RXNOLOS);

	tmp = WPI_READ(sc, WPI_GPIO_CTL);
	WPI_WRITE(sc, WPI_GPIO_CTL, tmp | WPI_GPIO_INIT);

	/* wait for clock stabilization */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (WPI_READ(sc, WPI_GPIO_CTL) & WPI_GPIO_CLOCK)
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		aprint_error("%s: timeout waiting for clock stabilization\n",
			sc->sc_dev.dv_xname);
		return ETIMEDOUT;
	}

	/* initialize EEPROM */
	tmp = WPI_READ(sc, WPI_EEPROM_STATUS);
	if ((tmp & WPI_EEPROM_VERSION) == 0) {
		aprint_error("%s: EEPROM not found\n", sc->sc_dev.dv_xname);
		return EIO;
	}
	WPI_WRITE(sc, WPI_EEPROM_STATUS, tmp & ~WPI_EEPROM_LOCKED);

	return 0;
}

static void
wpi_hw_config(struct wpi_softc *sc)
{
	uint32_t rev, hw;

	/* voodoo from the reference driver */
	hw = WPI_READ(sc, WPI_HWCONFIG);

	rev = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_CLASS_REG);
	rev = PCI_REVISION(rev);
	if ((rev & 0xc0) == 0x40)
		hw |= WPI_HW_ALM_MB;
	else if (!(rev & 0x80))
		hw |= WPI_HW_ALM_MM;

	if (sc->cap == 0x80)
		hw |= WPI_HW_SKU_MRC;

	hw &= ~WPI_HW_REV_D;
	if ((le16toh(sc->rev) & 0xf0) == 0xd0)	
		hw |= WPI_HW_REV_D;

	if (sc->type > 1)
		hw |= WPI_HW_TYPE_B;

	DPRINTF(("setting h/w config %x\n", hw));
	WPI_WRITE(sc, WPI_HWCONFIG, hw);
}

static int
wpi_init(struct ifnet *ifp)
{
	struct wpi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t tmp;
	int qid, ntries, error;

	(void)wpi_reset(sc);

	wpi_mem_lock(sc);
	wpi_mem_write(sc, WPI_MEM_CLOCK1, 0xa00);
	DELAY(20);
	tmp = wpi_mem_read(sc, WPI_MEM_PCIDEV);
	wpi_mem_write(sc, WPI_MEM_PCIDEV, tmp | 0x800);
	wpi_mem_unlock(sc);

	(void)wpi_power_up(sc);
	wpi_hw_config(sc);

	/* init Rx ring */
	wpi_mem_lock(sc);
	WPI_WRITE(sc, WPI_RX_BASE, sc->rxq.desc_dma.paddr);
	WPI_WRITE(sc, WPI_RX_RIDX_PTR, sc->shared_dma.paddr +
	    offsetof(struct wpi_shared, next));
	WPI_WRITE(sc, WPI_RX_WIDX, (WPI_RX_RING_COUNT - 1) & ~7);
	WPI_WRITE(sc, WPI_RX_CONFIG, 0xa9601010);
	wpi_mem_unlock(sc);

	/* init Tx rings */
	wpi_mem_lock(sc);
	wpi_mem_write(sc, WPI_MEM_MODE, 2); /* bypass mode */
	wpi_mem_write(sc, WPI_MEM_RA, 1);   /* enable RA0 */
	wpi_mem_write(sc, WPI_MEM_TXCFG, 0x3f); /* enable all 6 Tx rings */
	wpi_mem_write(sc, WPI_MEM_BYPASS1, 0x10000);
	wpi_mem_write(sc, WPI_MEM_BYPASS2, 0x30002);
	wpi_mem_write(sc, WPI_MEM_MAGIC4, 4);
	wpi_mem_write(sc, WPI_MEM_MAGIC5, 5);

	WPI_WRITE(sc, WPI_TX_BASE_PTR, sc->shared_dma.paddr);
	WPI_WRITE(sc, WPI_MSG_CONFIG, 0xffff05a5);

	for (qid = 0; qid < 6; qid++) {
		WPI_WRITE(sc, WPI_TX_CTL(qid), 0);
		WPI_WRITE(sc, WPI_TX_BASE(qid), 0);
		WPI_WRITE(sc, WPI_TX_CONFIG(qid), 0x80200008);
	}
	wpi_mem_unlock(sc);

	/* clear "radio off" and "disable command" bits (reversed logic) */
	WPI_WRITE(sc, WPI_UCODE_CLR, WPI_RADIO_OFF);
	WPI_WRITE(sc, WPI_UCODE_CLR, WPI_DISABLE_CMD);

	/* clear any pending interrupts */
	WPI_WRITE(sc, WPI_INTR, 0xffffffff);
	/* enable interrupts */
	WPI_WRITE(sc, WPI_MASK, WPI_INTR_MASK);

	/* not sure why/if this is necessary... */
	WPI_WRITE(sc, WPI_UCODE_CLR, WPI_RADIO_OFF);
	WPI_WRITE(sc, WPI_UCODE_CLR, WPI_RADIO_OFF);

	if ((error = wpi_load_firmware(sc)) != 0) {
		aprint_error("%s: could not load firmware\n", sc->sc_dev.dv_xname);
		goto fail1;
	}

	/* wait for thermal sensors to calibrate */
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((sc->temp = (int)WPI_READ(sc, WPI_TEMPERATURE)) != 0)	
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		aprint_error("%s: timeout waiting for thermal sensors calibration\n",
			sc->sc_dev.dv_xname);
		error = ETIMEDOUT;
		goto fail1;
	}

	DPRINTF(("temperature %d\n", sc->temp));

	if ((error = wpi_config(sc)) != 0) {
		aprint_error("%s: could not configure device\n",
			sc->sc_dev.dv_xname);
		goto fail1;
	}

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	if (ic->ic_opmode != IEEE80211_M_MONITOR) {
		if (ic->ic_roaming != IEEE80211_ROAMING_MANUAL)
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
	}
	else
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);

	return 0;

fail1:	wpi_stop(ifp, 1);
	return error;
}


static void
wpi_stop(struct ifnet *ifp, int disable)
{
	struct wpi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t tmp;
	int ac;

	ifp->if_timer = sc->sc_tx_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	/* disable interrupts */
	WPI_WRITE(sc, WPI_MASK, 0);
	WPI_WRITE(sc, WPI_INTR, WPI_INTR_MASK);
	WPI_WRITE(sc, WPI_INTR_STATUS, 0xff);
	WPI_WRITE(sc, WPI_INTR_STATUS, 0x00070000);

	wpi_mem_lock(sc);
	wpi_mem_write(sc, WPI_MEM_MODE, 0);
	wpi_mem_unlock(sc);

	/* reset all Tx rings */
	for (ac = 0; ac < 4; ac++)
		wpi_reset_tx_ring(sc, &sc->txq[ac]);
	wpi_reset_tx_ring(sc, &sc->cmdq);
	wpi_reset_tx_ring(sc, &sc->svcq);

	/* reset Rx ring */
	wpi_reset_rx_ring(sc, &sc->rxq);
	
	wpi_mem_lock(sc);
	wpi_mem_write(sc, WPI_MEM_CLOCK2, 0x200);
	wpi_mem_unlock(sc);

	DELAY(5);

	wpi_stop_master(sc);

	tmp = WPI_READ(sc, WPI_RESET);
	WPI_WRITE(sc, WPI_RESET, tmp | WPI_SW_RESET);
}
