/*	$NetBSD: if_iwn.c,v 1.30 2009/05/07 08:10:02 cegger Exp $	*/

/*-
 * Copyright (c) 2007
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
__KERNEL_RCSID(0, "$NetBSD: if_iwn.c,v 1.30 2009/05/07 08:10:02 cegger Exp $");


/*
 * Driver for Intel Wireless WiFi Link 4965AGN 802.11 network adapters.
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
#include <sys/mutex.h>
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
#include <net/if_media.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <net/if_ether.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/firmload.h>

#include <dev/pci/if_iwnreg.h>
#include <dev/pci/if_iwnvar.h>

#if 0
static const struct pci_matchid iwn_devices[] = {
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_PRO_WL_4965AGN_1 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_PRO_WL_4965AGN_2 }
};
#endif

/*
 * Supported rates for 802.11a/b/g modes (in 500Kbps unit).
 */
static const struct ieee80211_rateset iwn_rateset_11a =
	{ 8, { 12, 18, 24, 36, 48, 72, 96, 108 } };

static const struct ieee80211_rateset iwn_rateset_11b =
	{ 4, { 2, 4, 11, 22 } };

static const struct ieee80211_rateset iwn_rateset_11g =
	{ 12, { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 } };


#define EDCA_NUM_AC	4
static int		iwn_match(device_t , cfdata_t, void *);
static void		iwn_attach(device_t , device_t, void *);
static int		iwn_detach(device_t, int);

static void		iwn_radiotap_attach(struct iwn_softc *);
static int		iwn_dma_contig_alloc(bus_dma_tag_t, struct iwn_dma_info *,
    void **, bus_size_t, bus_size_t, int);
static void		iwn_dma_contig_free(struct iwn_dma_info *);
static int		iwn_alloc_shared(struct iwn_softc *);
static void		iwn_free_shared(struct iwn_softc *);
static int		iwn_alloc_kw(struct iwn_softc *);
static void		iwn_free_kw(struct iwn_softc *);
static int		iwn_alloc_fwmem(struct iwn_softc *);
static void		iwn_free_fwmem(struct iwn_softc *);
static struct		iwn_rbuf *iwn_alloc_rbuf(struct iwn_softc *);
static void		iwn_free_rbuf(struct mbuf *, void *, size_t, void *);
static int		iwn_alloc_rpool(struct iwn_softc *);
static void		iwn_free_rpool(struct iwn_softc *);
static int		iwn_alloc_rx_ring(struct iwn_softc *, struct iwn_rx_ring *);
static void		iwn_reset_rx_ring(struct iwn_softc *, struct iwn_rx_ring *);
static void		iwn_free_rx_ring(struct iwn_softc *, struct iwn_rx_ring *);
static int		iwn_alloc_tx_ring(struct iwn_softc *, struct iwn_tx_ring *,
    int, int);
static void		iwn_reset_tx_ring(struct iwn_softc *, struct iwn_tx_ring *);
static void		iwn_free_tx_ring(struct iwn_softc *, struct iwn_tx_ring *);
static struct		ieee80211_node *iwn_node_alloc(struct ieee80211_node_table *);
static void		iwn_newassoc(struct ieee80211_node *, int);
static int		iwn_media_change(struct ifnet *);
static int		iwn_newstate(struct ieee80211com *, enum ieee80211_state, int);
static void		iwn_mem_lock(struct iwn_softc *);
static void		iwn_mem_unlock(struct iwn_softc *);
static uint32_t iwn_mem_read(struct iwn_softc *, uint32_t);
static void		iwn_mem_write(struct iwn_softc *, uint32_t, uint32_t);
static void		iwn_mem_write_region_4(struct iwn_softc *, uint32_t,
    const uint32_t *, int);
static int		iwn_eeprom_lock(struct iwn_softc *);
static void		iwn_eeprom_unlock(struct iwn_softc *);
static int		iwn_read_prom_data(struct iwn_softc *, uint32_t, void *, int);
static int		iwn_load_microcode(struct iwn_softc *, const uint8_t *, int);
static int		iwn_load_firmware(struct iwn_softc *);
static void		iwn_calib_timeout(void *);
static void		iwn_iter_func(void *, struct ieee80211_node *);
static void		iwn_ampdu_rx_start(struct iwn_softc *, struct iwn_rx_desc *);
static void		iwn_rx_intr(struct iwn_softc *, struct iwn_rx_desc *,
    struct iwn_rx_data *);
static void		iwn_rx_statistics(struct iwn_softc *, struct iwn_rx_desc *);
static void		iwn_tx_intr(struct iwn_softc *, struct iwn_rx_desc *);
static void		iwn_cmd_intr(struct iwn_softc *, struct iwn_rx_desc *);
static void		iwn_notif_intr(struct iwn_softc *);
static int		iwn_intr(void *);
static void		iwn_read_eeprom(struct iwn_softc *);
static void		iwn_read_eeprom_channels(struct iwn_softc *, int);
static uint8_t		iwn_plcp_signal(int);
static int		iwn_tx_data(struct iwn_softc *, struct mbuf *,
    struct ieee80211_node *, int);
static void		iwn_start(struct ifnet *);
static void		iwn_watchdog(struct ifnet *);
static int		iwn_ioctl(struct ifnet *, u_long, void *);
static int		iwn_cmd(struct iwn_softc *, int, const void *, int, int);
static int		iwn_wme_update(struct ieee80211com *);
static int		iwn_setup_node_mrr(struct iwn_softc *, uint8_t, int);
static void		iwn_set_led(struct iwn_softc *, uint8_t, uint8_t, uint8_t);
static int		iwn_set_critical_temp(struct iwn_softc *);
static void		iwn_enable_tsf(struct iwn_softc *, struct ieee80211_node *);
static void		iwn_power_calibration(struct iwn_softc *, int);
static int		iwn_set_txpower(struct iwn_softc *,
    struct ieee80211_channel *, int);
static int		iwn_get_rssi(const struct iwn_rx_stat *);
static int		iwn_get_noise(const struct iwn_rx_general_stats *);
static int		iwn_get_temperature(struct iwn_softc *);
static int		iwn_init_sensitivity(struct iwn_softc *);
static void		iwn_compute_differential_gain(struct iwn_softc *,
    const struct iwn_rx_general_stats *);
static void		iwn_tune_sensitivity(struct iwn_softc *,
    const struct iwn_rx_stats *);
static int		iwn_send_sensitivity(struct iwn_softc *);
static int		iwn_setup_beacon(struct iwn_softc *, struct ieee80211_node *);
static int		iwn_auth(struct iwn_softc *);
static int		iwn_run(struct iwn_softc *);
static int		iwn_scan(struct iwn_softc *, uint16_t);
static int		iwn_config(struct iwn_softc *);
static void		iwn_post_alive(struct iwn_softc *);
static void		iwn_stop_master(struct iwn_softc *);
static int		iwn_reset(struct iwn_softc *);
static void		iwn_hw_config(struct iwn_softc *);
static int		iwn_init(struct ifnet *);
static void		iwn_stop(struct ifnet *, int);
static void		iwn_fix_channel(struct ieee80211com *, struct mbuf *);
static bool		iwn_resume(device_t PMF_FN_PROTO);
static int		iwn_add_node(struct iwn_softc *sc,
				     struct ieee80211_node *ni, bool broadcast, bool async, uint32_t htflags);



#define IWN_DEBUG

#ifdef IWN_DEBUG
#define DPRINTF(x)	do { if (iwn_debug > 0) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (iwn_debug >= (n)) printf x; } while (0)
int iwn_debug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

#ifdef IWN_DEBUG
static void		iwn_print_power_group(struct iwn_softc *, int);
#endif

CFATTACH_DECL_NEW(iwn, sizeof(struct iwn_softc), iwn_match, iwn_attach,
    iwn_detach, NULL);

static int
iwn_match(device_t parent, cfdata_t match __unused, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_4965AGN_1 ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_PRO_WL_4965AGN_2)
		return 1;

	return 0;
}

/* Base Address Register */
#define IWN_PCI_BAR0	0x10

static void
iwn_attach(device_t parent __unused, device_t self, void *aux)
{
	struct iwn_softc *sc = device_private(self);
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct pci_attach_args *pa = aux;
	const char *intrstr;
	char devinfo[256];
	pci_intr_handle_t ih;
	pcireg_t memtype, data;
	int i, error, revision;

	sc->sc_dev = self;
	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	callout_init(&sc->calib_to, 0);
	callout_setfunc(&sc->calib_to, iwn_calib_timeout, sc);

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof devinfo);
	revision = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s (rev. 0x%2x)\n", devinfo, revision);


	/* clear device specific PCI configuration register 0x41 */
	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0x40);
	data &= ~0x0000ff00;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0x40, data);

	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	data |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, data);

	/* enable bus-mastering */
	data = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	data |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, data);

	/* map the register window */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, IWN_PCI_BAR0);
	error = pci_mapreg_map(pa, IWN_PCI_BAR0, memtype, 0, &sc->sc_st,
	    &sc->sc_sh, NULL, &sc->sc_sz);
	if (error != 0) {
		aprint_error_dev(self, "could not map memory space\n");
		return;
	}

#if 0
	sc->sc_dmat = pa->pa_dmat;
#endif
	/* XXX may not be needed */
	if (bus_dmatag_subregion(pa->pa_dmat, 0, 3 << 30,
	    &(sc->sc_dmat), BUS_DMA_NOWAIT) != 0) {
		aprint_error_dev(self,
		    "WARNING: failed to restrict dma range, "
		    "falling back to parent bus dma range\n");
		sc->sc_dmat = pa->pa_dmat;
	}

	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error_dev(self, "could not map interrupt\n");
		return;
	}

	intrstr = pci_intr_string(sc->sc_pct, ih);
	sc->sc_ih = pci_intr_establish(sc->sc_pct, ih, IPL_NET, iwn_intr, sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "could not establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	if (iwn_reset(sc) != 0) {
		aprint_error_dev(self, "could not reset adapter\n");
		return;
	}

	/*
	 * Allocate DMA memory for firmware transfers.
	 */
	if ((error = iwn_alloc_fwmem(sc)) != 0) {
		aprint_error_dev(self, "could not allocate firmware memory\n");
		return;
	}

	/*
	 * Allocate a "keep warm" page.
	 */
	if ((error = iwn_alloc_kw(sc)) != 0) {
		aprint_error_dev(self, "could not allocate keep warm page\n");
		goto fail1;
	}

	/*
	 * Allocate shared area (communication area).
	 */
	if ((error = iwn_alloc_shared(sc)) != 0) {
		aprint_error_dev(self, "could not allocate shared area\n");
		goto fail2;
	}

	/*
	 * Allocate Rx buffers and Tx/Rx rings.
	 */
	if ((error = iwn_alloc_rpool(sc)) != 0) {
		aprint_error_dev(self, "could not allocate Rx buffers\n");
		goto fail3;
	}

	for (i = 0; i < IWN_NTXQUEUES; i++) {
		struct iwn_tx_ring *txq = &sc->txq[i];
		error = iwn_alloc_tx_ring(sc, txq, IWN_TX_RING_COUNT, i);
		if (error != 0) {
			aprint_error_dev(self, "could not allocate Tx ring %d\n", i);
			goto fail4;
		}
	}

	if (iwn_alloc_rx_ring(sc, &sc->rxq) != 0)  {
		aprint_error_dev(self, "could not allocate Rx ring\n");
		goto fail4;
	}


	/* Set the state of the RF kill switch */
	sc->sc_radio = (IWN_READ(sc, IWN_GPIO_CTL) & IWN_GPIO_RF_ENABLED);

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;	/* default to BSS mode */
	ic->ic_state = IEEE80211_S_INIT;

	/* set device capabilities */
	ic->ic_caps =
	    IEEE80211_C_IBSS |		/* IBSS mode support */
	    IEEE80211_C_WPA  |		/* 802.11i */
	    IEEE80211_C_MONITOR |	/* monitor mode supported */
	    IEEE80211_C_TXPMGT |	/* tx power management */
	    IEEE80211_C_SHSLOT |	/* short slot time supported */
	    IEEE80211_C_SHPREAMBLE|	/* short preamble supported */
	    IEEE80211_C_WME;		/* 802.11e */

	/* read supported channels and MAC address from EEPROM */
	iwn_read_eeprom(sc);

	/* set supported .11a, .11b and .11g rates */
	ic->ic_sup_rates[IEEE80211_MODE_11A] = iwn_rateset_11a;
	ic->ic_sup_rates[IEEE80211_MODE_11B] = iwn_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = iwn_rateset_11g;

	/* IBSS channel undefined for now */
	ic->ic_ibss_chan = &ic->ic_channels[0];

	memset(ic->ic_des_essid, 0, IEEE80211_NWID_LEN);
	ic->ic_des_esslen = 0;

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = iwn_init;
	ifp->if_stop = iwn_stop;
	ifp->if_ioctl = iwn_ioctl;
	ifp->if_start = iwn_start;
	ifp->if_watchdog = iwn_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ic);
	ic->ic_node_alloc = iwn_node_alloc;
	ic->ic_newassoc = iwn_newassoc;
	ic->ic_wme.wme_update = iwn_wme_update;

	/* override state transition machine */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = iwn_newstate;
	ieee80211_media_init(ic, iwn_media_change, ieee80211_media_status);

	sc->amrr.amrr_min_success_threshold =  1;
	sc->amrr.amrr_max_success_threshold = 15;

	if (!pmf_device_register(self, NULL, iwn_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, ifp);

	iwn_radiotap_attach(sc);

	ieee80211_announce(ic);

	return;

	/* free allocated memory if something failed during attachment */
fail4:	while (--i >= 0)
		iwn_free_tx_ring(sc, &sc->txq[i]);
	iwn_free_rpool(sc);
fail3:	iwn_free_shared(sc);
fail2:	iwn_free_kw(sc);
fail1:	iwn_free_fwmem(sc);
}

static int
iwn_detach(device_t self, int flags __unused)
{
	struct iwn_softc *sc = (struct iwn_softc *)self;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	int ac;

	iwn_stop(ifp, 1);

#if NBPFILTER > 0
	if (ifp != NULL)
		bpfdetach(ifp);
#endif
	ieee80211_ifdetach(&sc->sc_ic);
	if (ifp != NULL)
		if_detach(ifp);

	for (ac = 0; ac < IWN_NTXQUEUES; ac++)
		iwn_free_tx_ring(sc, &sc->txq[ac]);
	iwn_free_rx_ring(sc, &sc->rxq);
	iwn_free_rpool(sc);
	iwn_free_shared(sc);

	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pct, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);

	return 0;
}

/*
 * Attach the interface to 802.11 radiotap.
 */
static void
iwn_radiotap_attach(struct iwn_softc *sc)
{
	struct ifnet *ifp = sc->sc_ic.ic_ifp;

#if NBPFILTER > 0
	bpfattach2(ifp, DLT_IEEE802_11_RADIO,
	    sizeof (struct ieee80211_frame) + IEEE80211_RADIOTAP_HDRLEN,
	    &sc->sc_drvbpf);

	sc->sc_rxtap_len = sizeof sc->sc_rxtapu;
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(IWN_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof sc->sc_txtapu;
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(IWN_TX_RADIOTAP_PRESENT);
#endif
}


/*
 * Build a beacon frame that the firmware will broadcast periodically in
 * IBSS or HostAP modes.
 */
static int
iwn_setup_beacon(struct iwn_softc *sc, struct ieee80211_node *ni)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_tx_ring *ring = &sc->txq[4];
	struct iwn_tx_desc *desc;
	struct iwn_tx_data *data;
	struct iwn_tx_cmd *cmd;
	struct iwn_cmd_beacon *bcn;
	struct ieee80211_beacon_offsets bo;
	struct mbuf *m0;
	bus_addr_t paddr;
	int error;

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	m0 = ieee80211_beacon_alloc(ic, ni, &bo);
	if (m0 == NULL) {
		aprint_error_dev(sc->sc_dev, "could not allocate beacon frame\n");
		return ENOMEM;
	}

	cmd = &ring->cmd[ring->cur];
	cmd->code = IWN_CMD_SET_BEACON;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	bcn = (struct iwn_cmd_beacon *)cmd->data;
	memset(bcn, 0, sizeof (struct iwn_cmd_beacon));
	bcn->id = IWN_ID_BROADCAST;
	bcn->lifetime = htole32(IWN_LIFETIME_INFINITE);
	bcn->len = htole16(m0->m_pkthdr.len);
	bcn->rate = (ic->ic_curmode == IEEE80211_MODE_11A) ?
	    iwn_plcp_signal(12) : iwn_plcp_signal(2);
	bcn->flags2 = 0x2; /* RATE_MCS_CCK_MSK */

	bcn->flags = htole32(IWN_TX_AUTO_SEQ | IWN_TX_INSERT_TSTAMP
			     | IWN_TX_USE_NODE_RATE);

	/* save and trim IEEE802.11 header */
	m_copydata(m0, 0, sizeof (struct ieee80211_frame), (void *)&bcn->wh);
	m_adj(m0, sizeof (struct ieee80211_frame));

	/* assume beacon frame is contiguous */
	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m0,
	    BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "could not map beacon\n");
		m_freem(m0);
		return error;
	}

	data->m = m0;

	/* first scatter/gather segment is used by the beacon command */
	paddr = ring->cmd_dma.paddr + ring->cur * sizeof (struct iwn_tx_cmd);

	IWN_SET_DESC_NSEGS(desc, 2);
	IWN_SET_DESC_SEG(desc, 0, paddr , 4 + sizeof(struct iwn_cmd_beacon));
	IWN_SET_DESC_SEG(desc, 1,  data->map->dm_segs[0].ds_addr,
	    data->map->dm_segs[1].ds_len);

	bus_dmamap_sync(sc->sc_dmat, data->map, 0,
	    data->map->dm_mapsize /* calc? */, BUS_DMASYNC_PREWRITE);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % IWN_TX_RING_COUNT;
	IWN_WRITE(sc, IWN_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;
}

static int
iwn_dma_contig_alloc(bus_dma_tag_t tag, struct iwn_dma_info *dma, void **kvap,
    bus_size_t size, bus_size_t alignment, int flags)
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

fail:	iwn_dma_contig_free(dma);
	return error;
}

static void
iwn_dma_contig_free(struct iwn_dma_info *dma)
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

static int
iwn_alloc_shared(struct iwn_softc *sc)
{
	int error;
	void *p;
	/* must be aligned on a 1KB boundary */
	error = iwn_dma_contig_alloc(sc->sc_dmat, &sc->shared_dma,
	    &p, sizeof (struct iwn_shared), 1024,BUS_DMA_NOWAIT);
	sc->shared = p;
	if (error != 0)
		aprint_error_dev(sc->sc_dev,
		    "could not allocate shared area DMA memory\n");

	return error;

}

static void
iwn_free_shared(struct iwn_softc *sc)
{
	iwn_dma_contig_free(&sc->shared_dma);
}

static int
iwn_alloc_kw(struct iwn_softc *sc)
{
	/* must be aligned on a 16-byte boundary */
	return iwn_dma_contig_alloc(sc->sc_dmat, &sc->kw_dma, NULL,
	    PAGE_SIZE, PAGE_SIZE, BUS_DMA_NOWAIT);
}

static void
iwn_free_kw(struct iwn_softc *sc)
{
	iwn_dma_contig_free(&sc->kw_dma);
}

static int
iwn_alloc_fwmem(struct iwn_softc *sc)
{
	int error;
	/* allocate enough contiguous space to store text and data */
	error = iwn_dma_contig_alloc(sc->sc_dmat, &sc->fw_dma, NULL,
	    IWN_FW_MAIN_TEXT_MAXSZ + IWN_FW_MAIN_DATA_MAXSZ, 16,
	    BUS_DMA_NOWAIT);

	if (error != 0){
		aprint_error_dev(sc->sc_dev,
		    "could not allocate firmware transfer area DMA memory\n" );

	}
	return error;
}

static void
iwn_free_fwmem(struct iwn_softc *sc)
{
	iwn_dma_contig_free(&sc->fw_dma);
}

static struct iwn_rbuf *
iwn_alloc_rbuf(struct iwn_softc *sc)
{
	struct iwn_rbuf *rbuf;

	mutex_enter(&sc->rxq.freelist_mtx);
	rbuf = SLIST_FIRST(&sc->rxq.freelist);
	if (rbuf != NULL) {
		SLIST_REMOVE_HEAD(&sc->rxq.freelist, next);
		sc->rxq.nb_free_entries --;
	}
	mutex_exit(&sc->rxq.freelist_mtx);

	return rbuf;
}

/*
 * This is called automatically by the network stack when the mbuf to which
 * our Rx buffer is attached is freed.
 */
static void
iwn_free_rbuf(struct mbuf* m, void *buf,  size_t size, void *arg)
{
	struct iwn_rbuf *rbuf = arg;
	struct iwn_softc *sc = rbuf->sc;

	/* put the buffer back in the free list */
	mutex_enter(&sc->rxq.freelist_mtx);
	SLIST_INSERT_HEAD(&sc->rxq.freelist, rbuf, next);
	mutex_exit(&sc->rxq.freelist_mtx);
	sc->rxq.nb_free_entries ++;

	if (__predict_true(m != NULL))
		pool_cache_put(mb_cache, m);
}


static int
iwn_alloc_rpool(struct iwn_softc *sc)
{
	struct iwn_rx_ring *ring = &sc->rxq;
	struct iwn_rbuf *rbuf;
	int i, error;

	mutex_init(&ring->freelist_mtx, MUTEX_DEFAULT, IPL_NET);

	/* allocate a big chunk of DMA'able memory.. */
	error = iwn_dma_contig_alloc(sc->sc_dmat, &ring->buf_dma, NULL,
	    IWN_RBUF_COUNT * IWN_RBUF_SIZE, IWN_BUF_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate Rx buffers DMA memory\n");
		return error;
	}

	/* ..and split it into chunks of "rbufsz" bytes */
	SLIST_INIT(&ring->freelist);
	for (i = 0; i < IWN_RBUF_COUNT; i++) {
		rbuf = &ring->rbuf[i];

		rbuf->sc = sc;	/* backpointer for callbacks */
		rbuf->vaddr = (char *)ring->buf_dma.vaddr + i * IWN_RBUF_SIZE;
		rbuf->paddr = ring->buf_dma.paddr + i * IWN_RBUF_SIZE;

		SLIST_INSERT_HEAD(&ring->freelist, rbuf, next);
	}
	ring->nb_free_entries = IWN_RBUF_COUNT;
	return 0;
}

static void
iwn_free_rpool(struct iwn_softc *sc)
{
	iwn_dma_contig_free(&sc->rxq.buf_dma);
}

static int
iwn_alloc_rx_ring(struct iwn_softc *sc, struct iwn_rx_ring *ring)
{
	struct iwn_rx_data *data;
	struct iwn_rbuf *rbuf;
	int i, error;
	void *p;

	ring->cur = 0;

	error = iwn_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma,
	    &p, IWN_RX_RING_COUNT * sizeof (struct iwn_rx_desc),
	    IWN_RING_DMA_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate rx ring DMA memory\n");
		goto fail;
	}
	ring->desc = p;

	/*
	 * Setup Rx buffers.
	 */
	for (i = 0; i < IWN_RX_RING_COUNT; i++) {
		data = &ring->data[i];

		MGETHDR(data->m, M_DONTWAIT, MT_DATA);
		if (data->m == NULL) {
			aprint_error_dev(sc->sc_dev, "could not allocate rx mbuf\n");
			error = ENOMEM;
			goto fail;
		}
		if ((rbuf = iwn_alloc_rbuf(sc)) == NULL) {
			m_freem(data->m);
			data->m = NULL;
			aprint_error_dev(sc->sc_dev, "could not allocate rx buffer\n");
			error = ENOMEM;
			goto fail;
		}
		/* attach Rx buffer to mbuf */
		MEXTADD(data->m, rbuf->vaddr, IWN_RBUF_SIZE, 0, iwn_free_rbuf,
		    rbuf);

		data->m->m_flags |= M_EXT_RW;
		/* Rx buffers are aligned on a 256-byte boundary */
		ring->desc[i] = htole32(rbuf->paddr >> 8);
	}

	return 0;

fail:	iwn_free_rx_ring(sc, ring);
	return error;
}

static void
iwn_reset_rx_ring(struct iwn_softc *sc, struct iwn_rx_ring *ring)
{
	int ntries;

	iwn_mem_lock(sc);

	IWN_WRITE(sc, IWN_RX_CONFIG, 0);
	for (ntries = 0; ntries < 100; ntries++) {
		if (IWN_READ(sc, IWN_RX_STATUS) & IWN_RX_IDLE)
			break;
		DELAY(10);
	}
#ifdef IWN_DEBUG
	if (ntries == 100 && iwn_debug > 0)
		aprint_error_dev(sc->sc_dev, "timeout resetting Rx ring\n");
#endif
	iwn_mem_unlock(sc);

	ring->cur = 0;
}

static void
iwn_free_rx_ring(struct iwn_softc *sc, struct iwn_rx_ring *ring)
{
	int i;

	iwn_dma_contig_free(&ring->desc_dma);

	for (i = 0; i < IWN_RX_RING_COUNT; i++) {
		if (ring->data[i].m != NULL)
			m_freem(ring->data[i].m);
	}
}

static int
iwn_alloc_tx_ring(struct iwn_softc *sc, struct iwn_tx_ring *ring, int count,
    int qid)
{
	struct iwn_tx_data *data;
	int i, error;
	void *p;

	ring->qid = qid;
	ring->count = count;
	ring->queued = 0;
	ring->cur = 0;

	error = iwn_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma,
	    &p, count * sizeof (struct iwn_tx_desc),
	    IWN_RING_DMA_ALIGN, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not allocate tx ring DMA memory\n");
		goto fail;
	}
	ring->desc = p;

	error = iwn_dma_contig_alloc(sc->sc_dmat, &ring->cmd_dma,
	    &p, count * sizeof (struct iwn_tx_cmd), 4, BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not allocate tx cmd DMA memory\n");
		goto fail;
	}
	ring->cmd = p;

	ring->data = malloc(count * sizeof (struct iwn_tx_data), M_DEVBUF, M_NOWAIT);

	if (ring->data == NULL) {
		aprint_error_dev(sc->sc_dev,"could not allocate tx data slots\n");
		goto fail;
	}

	memset(ring->data, 0, count * sizeof (struct iwn_tx_data));

	for (i = 0; i < count; i++) {
		data = &ring->data[i];

		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    IWN_MAX_SCATTER - 1, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &data->map);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev, "could not create tx buf DMA map\n");
			goto fail;
		}
	}

	return 0;

fail:	iwn_free_tx_ring(sc, ring);
	return error;
}

static void
iwn_reset_tx_ring(struct iwn_softc *sc, struct iwn_tx_ring *ring)
{
	struct iwn_tx_data *data;
	uint32_t tmp;
	int i, ntries;

	iwn_mem_lock(sc);

	IWN_WRITE(sc, IWN_TX_CONFIG(ring->qid), 0);
	for (ntries = 0; ntries < 100; ntries++) {
		tmp = IWN_READ(sc, IWN_TX_STATUS);
		if ((tmp & IWN_TX_IDLE(ring->qid)) == IWN_TX_IDLE(ring->qid))
			break;
		DELAY(10);
	}
#ifdef IWN_DEBUG
	if (ntries == 100 && iwn_debug > 1) {
		aprint_error_dev(sc->sc_dev, "timeout resetting Tx ring %d\n", ring->qid);
	}
#endif
	iwn_mem_unlock(sc);

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
iwn_free_tx_ring(struct iwn_softc *sc, struct iwn_tx_ring *ring)
{
	struct iwn_tx_data *data;
	int i;

	iwn_dma_contig_free(&ring->desc_dma);
	iwn_dma_contig_free(&ring->cmd_dma);

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
struct ieee80211_node *
iwn_node_alloc(struct ieee80211_node_table *nt __unused)
{
	struct iwn_node *wn;

	wn = malloc(sizeof (struct iwn_node), M_80211_NODE, M_NOWAIT | M_ZERO);

	return (struct ieee80211_node *)wn;
}

static void
iwn_newassoc(struct ieee80211_node *ni, int isnew)
{
	struct iwn_softc *sc = ni->ni_ic->ic_ifp->if_softc;
	int i;

	ieee80211_amrr_node_init(&sc->amrr, &((struct iwn_node *)ni)->amn);

	/* set rate to some reasonable initial value */
	for (i = ni->ni_rates.rs_nrates - 1;
	     i > 0 && (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL) > 72;
	     i--);
	ni->ni_txrate = i;
}

static int
iwn_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return error;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
		iwn_init(ifp);

	return 0;
}

static int
iwn_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct iwn_softc *sc = ifp->if_softc;
	int error;

	callout_stop(&sc->calib_to);

	DPRINTF(("iwn_newstate: nstate = %d, ic->ic_state = %d\n", nstate,
		ic->ic_state));

	switch (nstate) {

	case IEEE80211_S_SCAN:

		if (sc->is_scanning)
			break;

		sc->is_scanning = true;
		ieee80211_node_table_reset(&ic->ic_scan);
		ic->ic_flags |= IEEE80211_F_SCAN | IEEE80211_F_ASCAN;

		/* make the link LED blink while we're scanning */
		iwn_set_led(sc, IWN_LED_LINK, 20, 2);

		if ((error = iwn_scan(sc, IEEE80211_CHAN_G)) != 0) {
			aprint_error_dev(sc->sc_dev, "could not initiate scan\n");
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
		/* cancel any active scan - it apparently breaks auth */
		/*(void)iwn_cmd(sc, IWN_CMD_SCAN_ABORT, NULL, 0, 1);*/

		if ((error = iwn_auth(sc)) != 0) {
			aprint_error_dev(sc->sc_dev,
					 "could not move to auth state\n");
			return error;
		}
		break;

	case IEEE80211_S_RUN:
		if ((error = iwn_run(sc)) != 0) {
			aprint_error_dev(sc->sc_dev,
					 "could not move to run state\n");
			return error;
		}
		break;

	case IEEE80211_S_INIT:
		sc->is_scanning = false;
		break;
	}

	return sc->sc_newstate(ic, nstate, arg);
}

/*
 * Grab exclusive access to NIC memory.
 */
static void
iwn_mem_lock(struct iwn_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = IWN_READ(sc, IWN_GPIO_CTL);
	IWN_WRITE(sc, IWN_GPIO_CTL, tmp | IWN_GPIO_MAC);

	/* spin until we actually get the lock */
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((IWN_READ(sc, IWN_GPIO_CTL) &
			(IWN_GPIO_CLOCK | IWN_GPIO_SLEEP)) == IWN_GPIO_CLOCK)
			break;
		DELAY(10);
	}
	if (ntries == 1000)
		aprint_error_dev(sc->sc_dev, "could not lock memory\n");
}

/*
 * Release lock on NIC memory.
 */
static void
iwn_mem_unlock(struct iwn_softc *sc)
{
	uint32_t tmp = IWN_READ(sc, IWN_GPIO_CTL);
	IWN_WRITE(sc, IWN_GPIO_CTL, tmp & ~IWN_GPIO_MAC);
}

static uint32_t
iwn_mem_read(struct iwn_softc *sc, uint32_t addr)
{
	IWN_WRITE(sc, IWN_READ_MEM_ADDR, IWN_MEM_4 | addr);
	return IWN_READ(sc, IWN_READ_MEM_DATA);
}

static void
iwn_mem_write(struct iwn_softc *sc, uint32_t addr, uint32_t data)
{
	IWN_WRITE(sc, IWN_WRITE_MEM_ADDR, IWN_MEM_4 | addr);
	IWN_WRITE(sc, IWN_WRITE_MEM_DATA, data);
}

static void
iwn_mem_write_region_4(struct iwn_softc *sc, uint32_t addr,
    const uint32_t *data, int wlen)
{
	for (; wlen > 0; wlen--, data++, addr += 4)
		iwn_mem_write(sc, addr, *data);
}

static int
iwn_eeprom_lock(struct iwn_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = IWN_READ(sc, IWN_HWCONFIG);
	IWN_WRITE(sc, IWN_HWCONFIG, tmp | IWN_HW_EEPROM_LOCKED);

	/* spin until we actually get the lock */
	for (ntries = 0; ntries < 100; ntries++) {
		if (IWN_READ(sc, IWN_HWCONFIG) & IWN_HW_EEPROM_LOCKED)
			return 0;
		DELAY(10);
	}
	return ETIMEDOUT;
}

static void
iwn_eeprom_unlock(struct iwn_softc *sc)
{
	uint32_t tmp = IWN_READ(sc, IWN_HWCONFIG);
	IWN_WRITE(sc, IWN_HWCONFIG, tmp & ~IWN_HW_EEPROM_LOCKED);
}

/*
 * Read `len' bytes from the EEPROM. We access the EEPROM through the MAC
 * instead of using the traditional bit-bang method.
 */
static int
iwn_read_prom_data(struct iwn_softc *sc, uint32_t addr, void *data, int len)
{
	uint8_t *out = data;
	uint32_t val;
	int ntries;

	iwn_mem_lock(sc);
	for (; len > 0; len -= 2, addr++) {
		IWN_WRITE(sc, IWN_EEPROM_CTL, addr << 2);
		IWN_WRITE(sc, IWN_EEPROM_CTL,
		    IWN_READ(sc, IWN_EEPROM_CTL) & ~IWN_EEPROM_CMD);

		for (ntries = 0; ntries < 10; ntries++) {
			if ((val = IWN_READ(sc, IWN_EEPROM_CTL)) &
			    IWN_EEPROM_READY)
				break;
			DELAY(5);
		}
		if (ntries == 10) {
			aprint_error_dev(sc->sc_dev, "could not read EEPROM\n");
			return ETIMEDOUT;
		}
		*out++ = val >> 16;
		if (len > 1)
			*out++ = val >> 24;
	}
	iwn_mem_unlock(sc);

	return 0;
}

/*
 * The firmware boot code is small and is intended to be copied directly into
 * the NIC internal memory.
 */
static int
iwn_load_microcode(struct iwn_softc *sc, const uint8_t *ucode, int size)
{
	int ntries;

	size /= sizeof (uint32_t);

	iwn_mem_lock(sc);

	/* copy microcode image into NIC memory */
	iwn_mem_write_region_4(sc, IWN_MEM_UCODE_BASE,
	    (const uint32_t *)ucode, size);

	iwn_mem_write(sc, IWN_MEM_UCODE_SRC, 0);
	iwn_mem_write(sc, IWN_MEM_UCODE_DST, IWN_FW_TEXT);
	iwn_mem_write(sc, IWN_MEM_UCODE_SIZE, size);

	/* run microcode */
	iwn_mem_write(sc, IWN_MEM_UCODE_CTL, IWN_UC_RUN);

	/* wait for transfer to complete */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (!(iwn_mem_read(sc, IWN_MEM_UCODE_CTL) & IWN_UC_RUN))
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		iwn_mem_unlock(sc);
		aprint_error_dev(sc->sc_dev, "could not load boot firmware\n");
		return ETIMEDOUT;
	}
	iwn_mem_write(sc, IWN_MEM_UCODE_CTL, IWN_UC_ENABLE);

	iwn_mem_unlock(sc);

	return 0;
}

static int
iwn_load_firmware(struct iwn_softc *sc)
{
	struct iwn_dma_info *dma = &sc->fw_dma;
	struct iwn_firmware_hdr hdr;
	const uint8_t *init_text, *init_data, *main_text, *main_data;
	const uint8_t *boot_text;
	uint32_t init_textsz, init_datasz, main_textsz, main_datasz;
	uint32_t boot_textsz;
	firmware_handle_t fw;
	u_char *dfw;
	size_t size;
	int error;

	/* load firmware image from disk */
	if ((error = firmware_open("if_iwn","iwlwifi-4965-1.ucode", &fw)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not read firmware file\n");
		goto fail1;
	}

	size = firmware_get_size(fw);

	/* extract firmware header information */
	if (size < sizeof (struct iwn_firmware_hdr)) {
		aprint_error_dev(sc->sc_dev, "truncated firmware header: %zu bytes\n", size);

		error = EINVAL;
		goto fail2;
	}


	if ((error = firmware_read(fw, 0, &hdr,
		    sizeof (struct iwn_firmware_hdr))) != 0) {
		aprint_error_dev(sc->sc_dev, "can't get firmware header\n");
		goto fail2;
	}

	main_textsz = le32toh(hdr.main_textsz);
	main_datasz = le32toh(hdr.main_datasz);
	init_textsz = le32toh(hdr.init_textsz);
	init_datasz = le32toh(hdr.init_datasz);
	boot_textsz = le32toh(hdr.boot_textsz);

	/* sanity-check firmware segments sizes */
	if (main_textsz > IWN_FW_MAIN_TEXT_MAXSZ ||
	    main_datasz > IWN_FW_MAIN_DATA_MAXSZ ||
	    init_textsz > IWN_FW_INIT_TEXT_MAXSZ ||
	    init_datasz > IWN_FW_INIT_DATA_MAXSZ ||
	    boot_textsz > IWN_FW_BOOT_TEXT_MAXSZ ||
	    (boot_textsz & 3) != 0) {
		aprint_error_dev(sc->sc_dev, "invalid firmware header\n");
		error = EINVAL;
		goto fail2;
	}

	/* check that all firmware segments are present */
	if (size < sizeof (struct iwn_firmware_hdr) + main_textsz +
	    main_datasz + init_textsz + init_datasz + boot_textsz) {
		aprint_error_dev(sc->sc_dev, "firmware file too short: %zu bytes\n", size);
		error = EINVAL;
		goto fail2;
	}

	dfw = firmware_malloc(size);
	if (dfw == NULL) {
		aprint_error_dev(sc->sc_dev, "not enough memory to stock firmware\n");
		error = ENOMEM;
		goto fail2;
	}

	if ((error = firmware_read(fw, 0, dfw, size)) != 0) {
		aprint_error_dev(sc->sc_dev, "can't get firmware\n");
		goto fail2;
	}

	/* get pointers to firmware segments */
	main_text = dfw + sizeof (struct iwn_firmware_hdr);
	main_data = main_text + main_textsz;
	init_text = main_data + main_datasz;
	init_data = init_text + init_textsz;
	boot_text = init_data + init_datasz;

	/* copy initialization images into pre-allocated DMA-safe memory */
	memcpy(dma->vaddr, init_data, init_datasz);
	memcpy((char *)dma->vaddr + IWN_FW_INIT_DATA_MAXSZ, init_text, init_textsz);

	/* tell adapter where to find initialization images */
	iwn_mem_lock(sc);
	iwn_mem_write(sc, IWN_MEM_DATA_BASE, dma->paddr >> 4);
	iwn_mem_write(sc, IWN_MEM_DATA_SIZE, init_datasz);
	iwn_mem_write(sc, IWN_MEM_TEXT_BASE,
	    (dma->paddr + IWN_FW_INIT_DATA_MAXSZ) >> 4);
	iwn_mem_write(sc, IWN_MEM_TEXT_SIZE, init_textsz);
	iwn_mem_unlock(sc);

	/* load firmware boot code */
	if ((error = iwn_load_microcode(sc, boot_text, boot_textsz)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not load boot firmware\n");
		goto fail3;
	}

	/* now press "execute" ;-) */
	IWN_WRITE(sc, IWN_RESET, 0);

	/* ..and wait at most one second for adapter to initialize */
	if ((error = tsleep(sc, PCATCH, "iwninit", hz)) != 0) {
		/* this isn't what was supposed to happen.. */
		aprint_error_dev(sc->sc_dev, "timeout waiting for adapter to initialize\n");
	}

	/* copy runtime images into pre-allocated DMA-safe memory */
	memcpy((char *)dma->vaddr, main_data, main_datasz);
	memcpy((char *)dma->vaddr + IWN_FW_MAIN_DATA_MAXSZ, main_text, main_textsz);

	/* tell adapter where to find runtime images */
	iwn_mem_lock(sc);
	iwn_mem_write(sc, IWN_MEM_DATA_BASE, dma->paddr >> 4);
	iwn_mem_write(sc, IWN_MEM_DATA_SIZE, main_datasz);
	iwn_mem_write(sc, IWN_MEM_TEXT_BASE,
	    (dma->paddr + IWN_FW_MAIN_DATA_MAXSZ) >> 4);
	iwn_mem_write(sc, IWN_MEM_TEXT_SIZE, IWN_FW_UPDATED | main_textsz);
	iwn_mem_unlock(sc);

	/* wait at most one second for second alive notification */
	if ((error = tsleep(sc, PCATCH, "iwninit", hz)) != 0) {
		/* this isn't what was supposed to happen.. */
		aprint_error_dev(sc->sc_dev, "timeout waiting for adapter to initialize\n");
	}

fail3: firmware_free(dfw,size);
fail2:	firmware_close(fw);
fail1:	return error;
}

static void
iwn_calib_timeout(void *arg)
{
	struct iwn_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	/* automatic rate control triggered every 500ms */
	if (ic->ic_fixed_rate == -1) {
		s = splnet();
		if (ic->ic_opmode == IEEE80211_M_STA)
			iwn_iter_func(sc, ic->ic_bss);
		else
			ieee80211_iterate_nodes(&ic->ic_sta, iwn_iter_func, sc);
		splx(s);
	}

	/* automatic calibration every 60s */
	if (++sc->calib_cnt >= 120) {
		DPRINTF(("sending request for statistics\n"));
		(void)iwn_cmd(sc, IWN_CMD_GET_STATISTICS, NULL, 0, 1);
		sc->calib_cnt = 0;
	}

	callout_schedule(&sc->calib_to, hz/2);

}

static void
iwn_iter_func(void *arg, struct ieee80211_node *ni)
{
	struct iwn_softc *sc = arg;
	struct iwn_node *wn = (struct iwn_node *)ni;

	ieee80211_amrr_choose(&sc->amrr, ni, &wn->amn);
}

static void
iwn_ampdu_rx_start(struct iwn_softc *sc, struct iwn_rx_desc *desc)
{
	struct iwn_rx_stat *stat;

	DPRINTFN(2, ("received AMPDU stats\n"));
	/* save Rx statistics, they will be used on IWN_AMPDU_RX_DONE */
	stat = (struct iwn_rx_stat *)(desc + 1);
	memcpy(&sc->last_rx_stat, stat, sizeof (*stat));
	sc->last_rx_valid = 1;
}

void
iwn_rx_intr(struct iwn_softc *sc, struct iwn_rx_desc *desc,
    struct iwn_rx_data *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct iwn_rx_ring *ring = &sc->rxq;
	struct iwn_rbuf *rbuf;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct mbuf *m, *mnew;
	struct iwn_rx_stat *stat;
	char *head;
	uint32_t *tail;
	int len, rssi;

	if (desc->type == IWN_AMPDU_RX_DONE) {
		/* check for prior AMPDU_RX_START */
		if (!sc->last_rx_valid) {
			DPRINTF(("missing AMPDU_RX_START\n"));
			ifp->if_ierrors++;
			return;
		}
		sc->last_rx_valid = 0;
		stat = &sc->last_rx_stat;
	} else
		stat = (struct iwn_rx_stat *)(desc + 1);

	if (stat->cfg_phy_len > IWN_STAT_MAXLEN) {
		aprint_error_dev(sc->sc_dev, "invalid rx statistic header\n");
		ifp->if_ierrors++;
		return;
	}

	if (desc->type == IWN_AMPDU_RX_DONE) {
		struct iwn_rx_ampdu *ampdu =
		    (struct iwn_rx_ampdu *)(desc + 1);
		head = (char *)(ampdu + 1);
		len = le16toh(ampdu->len);
	} else {
		head = (char *)(stat + 1) + stat->cfg_phy_len;
		len = le16toh(stat->len);
	}

	DPRINTF(("rx packet len %d\n", len));
	/* discard Rx frames with bad CRC early */
	tail = (uint32_t *)(head + len);
	if ((le32toh(*tail) & IWN_RX_NOERROR) != IWN_RX_NOERROR) {
		DPRINTFN(2, ("rx flags error %x\n", le32toh(*tail)));
		ifp->if_ierrors++;
		return;
	}
	/* XXX for ieee80211_find_rxnode() */
	if (len < sizeof (struct ieee80211_frame)) {
		DPRINTF(("frame too short: %d\n", len));
		ic->ic_stats.is_rx_tooshort++;
		ifp->if_ierrors++;
		return;
	}

	m = data->m;

	/* finalize mbuf */
	m->m_pkthdr.rcvif = ifp;
	m->m_data = head;
	m->m_pkthdr.len = m->m_len = len;

	/*
	 * See comment in if_wpi.c:wpi_rx_intr() about locking
	 * nb_free_entries here.  In short:  it's not required.
	 */
	if (sc->rxq.nb_free_entries > 0) {
		MGETHDR(mnew, M_DONTWAIT, MT_DATA);
		if (mnew == NULL) {
			ic->ic_stats.is_rx_nobuf++;
			ifp->if_ierrors++;
			return;
		}

		rbuf = iwn_alloc_rbuf(sc);

		/* attach Rx buffer to mbuf */
		MEXTADD(mnew, rbuf->vaddr, IWN_RBUF_SIZE, 0, iwn_free_rbuf,
		    rbuf);
		mnew->m_flags |= M_EXT_RW;

		data->m = mnew;

		/* update Rx descriptor */
		ring->desc[ring->cur] = htole32(rbuf->paddr >> 8);
	} else {
		/* no free rbufs, copy frame */
		m = m_dup(m, 0, M_COPYALL, M_DONTWAIT);
		if (m == NULL) {
			/* no free mbufs either, drop frame */
			ic->ic_stats.is_rx_nobuf++;
			ifp->if_ierrors++;
			return;
		}
	}

	rssi = iwn_get_rssi(stat);

	if (ic->ic_state == IEEE80211_S_SCAN)
		iwn_fix_channel(ic, m);

#if NBPFILTER > 0
	if (sc->sc_drvbpf != NULL) {
		struct iwn_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		tap->wr_chan_freq =
		    htole16(ic->ic_channels[stat->chan].ic_freq);
		tap->wr_chan_flags =
		    htole16(ic->ic_channels[stat->chan].ic_flags);
		tap->wr_dbm_antsignal = (int8_t)rssi;
		tap->wr_dbm_antnoise = (int8_t)sc->noise;
		tap->wr_tsft = stat->tstamp;
		switch (stat->rate) {
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

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_rxtap_len, m);
	}
#endif

	/* grab a reference to the source node */
	wh = mtod(m, struct ieee80211_frame *);
	ni = ieee80211_find_rxnode(ic,(struct ieee80211_frame_min *)wh);

	/* send the frame to the 802.11 layer */
	ieee80211_input(ic, m, ni, rssi, 0);

	/* node is no longer needed */
	ieee80211_free_node(ni);
}


/*
 * XXX: Hack to set the current channel to the value advertised in beacons or
 * probe responses. Only used during AP detection.
 * XXX: Duplicated from if_iwi.c
 */
static void
iwn_fix_channel(struct ieee80211com *ic, struct mbuf *m)
{
	struct ieee80211_frame *wh;
	uint8_t subtype;
	uint8_t *frm, *efrm;

	wh = mtod(m, struct ieee80211_frame *);

	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT)
		return;

	subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

	if (subtype != IEEE80211_FC0_SUBTYPE_BEACON &&
	    subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP)
		return;

	frm = (uint8_t *)(wh + 1);
	efrm = mtod(m, uint8_t *) + m->m_len;

	frm += 12;	/* skip tstamp, bintval and capinfo fields */
	while (frm < efrm) {
		if (*frm == IEEE80211_ELEMID_DSPARMS)
#if IEEE80211_CHAN_MAX < 255
			if (frm[2] <= IEEE80211_CHAN_MAX)
#endif
				ic->ic_curchan = &ic->ic_channels[frm[2]];

		frm += frm[1] + 2;
	}
}

static void
iwn_rx_statistics(struct iwn_softc *sc, struct iwn_rx_desc *desc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_calib_state *calib = &sc->calib;
	struct iwn_stats *stats = (struct iwn_stats *)(desc + 1);

	/* ignore beacon statistics received during a scan */
	if (ic->ic_state != IEEE80211_S_RUN)
		return;

	DPRINTFN(3, ("received statistics (cmd=%d)\n", desc->type));
	sc->calib_cnt = 0;	/* reset timeout */

	/* test if temperature has changed */
	if (stats->general.temp != sc->rawtemp) {
		int temp;

		sc->rawtemp = stats->general.temp;
		temp = iwn_get_temperature(sc);
		DPRINTFN(2, ("temperature=%d\n", temp));

		/* update Tx power if need be */
		iwn_power_calibration(sc, temp);
	}

	if (desc->type != IWN_BEACON_STATISTICS)
		return; /* reply to a statistics request */

	sc->noise = iwn_get_noise(&stats->rx.general);
	DPRINTFN(3, ("noise=%d\n", sc->noise));

	/* test that RSSI and noise are present in stats report */
	if (le32toh(stats->rx.general.flags) != 1) {
		DPRINTF(("received statistics without RSSI\n"));
		return;
	}

	if (calib->state == IWN_CALIB_STATE_ASSOC)
		iwn_compute_differential_gain(sc, &stats->rx.general);
	else if (calib->state == IWN_CALIB_STATE_RUN)
		iwn_tune_sensitivity(sc, &stats->rx);
}

static void
iwn_tx_intr(struct iwn_softc *sc, struct iwn_rx_desc *desc)
{
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	struct iwn_tx_ring *ring = &sc->txq[desc->qid & 0xf];
	struct iwn_tx_data *txdata = &ring->data[desc->idx];
	struct iwn_tx_stat *stat = (struct iwn_tx_stat *)(desc + 1);
	struct iwn_node *wn = (struct iwn_node *)txdata->ni;
	uint32_t status;

	DPRINTFN(4, ("tx done: qid=%d idx=%d retries=%d nkill=%d rate=%x "
		"duration=%d status=%x\n", desc->qid, desc->idx, stat->ntries,
		stat->nkill, stat->rate, le16toh(stat->duration),
		le32toh(stat->status)));

	/*
	 * Update rate control statistics for the node.
	 */
	wn->amn.amn_txcnt++;
	if (stat->ntries > 0) {
		DPRINTFN(3, ("tx intr ntries %d\n", stat->ntries));
		wn->amn.amn_retrycnt++;
	}

	status = le32toh(stat->status) & 0xff;
	if (status != 1 && status != 2)
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
	iwn_start(ifp);
}

static void
iwn_cmd_intr(struct iwn_softc *sc, struct iwn_rx_desc *desc)
{
	struct iwn_tx_ring *ring = &sc->txq[4];
	struct iwn_tx_data *data;

	if ((desc->qid & 0xf) != 4)
		return; /* not a command ack */

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
iwn_microcode_ready(struct iwn_softc *sc, struct iwn_ucode_info *uc)
{

	/* the microcontroller is ready */
	DPRINTF(("microcode alive notification version=%d.%d "
		 "subtype=%x alive=%x\n", uc->major, uc->minor,
		 uc->subtype, le32toh(uc->valid)));

	if (le32toh(uc->valid) != 1) {
		aprint_error_dev(sc->sc_dev, "microcontroller initialization "
				 "failed\n");
		return;
	}
	if (uc->subtype == IWN_UCODE_INIT) {
		/* save microcontroller's report */
		memcpy(&sc->ucode_info, uc, sizeof (*uc));
	}
}


static void
iwn_notif_intr(struct iwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct iwn_rx_data *data;
	struct iwn_rx_desc *desc;
	uint16_t hw;

	hw = le16toh(sc->shared->closed_count);

	/*
	 * If the radio is disabled then down the interface and stop
	 * processing - scan the queue for a microcode load command
	 * result.  It is the only thing that we can do with the radio
	 * off.
	 */
	if (!sc->sc_radio) {
		while (sc->rxq.cur != hw) {
			data = &sc->rxq.data[sc->rxq.cur];
			desc = (void *)data->m->m_ext.ext_buf;
			if (desc->type == IWN_UC_READY) {
				iwn_microcode_ready(sc,
				    (struct iwn_ucode_info *)(desc + 1));
			} else if (desc->type == IWN_STATE_CHANGED) {
				uint32_t *status = (uint32_t *)(desc + 1);

				/* enabled/disabled notification */
				DPRINTF(("state changed to %x\n",
					 le32toh(*status)));

				sc->sc_radio = !(le32toh(*status) & 1);
			}

			sc->rxq.cur = (sc->rxq.cur + 1) % IWN_RX_RING_COUNT;
		}

		if (!sc->sc_radio) {
			ifp->if_flags &= ~IFF_UP;
			iwn_stop(ifp, 1);
		}

		return;
	}

	while (sc->rxq.cur != hw) {
		data = &sc->rxq.data[sc->rxq.cur];
		desc = (void *)data->m->m_ext.ext_buf;

		DPRINTFN(4,("rx notification qid=%x idx=%d flags=%x type=%d "
			"len=%d\n", desc->qid, desc->idx, desc->flags, desc->type,
			le32toh(desc->len)));

		if (!(desc->qid & 0x80))	/* reply to a command */
			iwn_cmd_intr(sc, desc);

		switch (desc->type) {
		case IWN_RX_DONE:
		case IWN_AMPDU_RX_DONE:
			iwn_rx_intr(sc, desc, data);
			break;

		case IWN_AMPDU_RX_START:
			iwn_ampdu_rx_start(sc, desc);
			break;

		case IWN_TX_DONE:
			/* a 802.11 frame has been transmitted */
			iwn_tx_intr(sc, desc);
			break;

		case IWN_RX_STATISTICS:
		case IWN_BEACON_STATISTICS:
			iwn_rx_statistics(sc, desc);
			break;

		case IWN_BEACON_MISSED:
		{
			struct iwn_beacon_missed *miss =
			    (struct iwn_beacon_missed *)(desc + 1);
			/*
			 * If more than 5 consecutive beacons are missed,
			 * reinitialize the sensitivity state machine.
			 */
			DPRINTFN(2, ("beacons missed %d/%d\n",
				le32toh(miss->consecutive), le32toh(miss->total)));
			if (ic->ic_state == IEEE80211_S_RUN &&
			    le32toh(miss->consecutive) > 5)
				(void)iwn_init_sensitivity(sc);
			break;
		}

		case IWN_UC_READY:
		{
			iwn_microcode_ready(sc,
			    (struct iwn_ucode_info *)(desc + 1));
			break;
		}
		case IWN_STATE_CHANGED:
		{
			uint32_t *status = (uint32_t *)(desc + 1);

			/* enabled/disabled notification */
			DPRINTF(("state changed to %x\n", le32toh(*status)));

			sc->sc_radio = !(le32toh(*status) & 1);
			if (le32toh(*status) & 1) {
				/* the radio button has to be pushed */
				aprint_error_dev(sc->sc_dev, "Radio transmitter is off\n");
				/* turn the interface down */
				ifp->if_flags &= ~IFF_UP;
				iwn_stop(ifp, 1);
				return; /* no further processing */
			}
			break;
		}
		case IWN_START_SCAN:
		{
			struct iwn_start_scan *scan =
			    (struct iwn_start_scan *)(desc + 1);

			DPRINTFN(2, ("scanning channel %d status %x\n",
				scan->chan, le32toh(scan->status)));

			/* fix current channel */
			ic->ic_bss->ni_chan = &ic->ic_channels[scan->chan];
			break;
		}
		case IWN_STOP_SCAN:
		{
			struct iwn_stop_scan *scan =
			    (struct iwn_stop_scan *)(desc + 1);

			DPRINTF(("scan finished nchan=%d status=%d chan=%d\n",
				scan->nchan, scan->status, scan->chan));

			if (scan->status == 1 && scan->chan <= 14) {
				/*
				 * We just finished scanning 802.11g channels,
				 * start scanning 802.11a ones.
				 */
				if (iwn_scan(sc, IEEE80211_CHAN_A) == 0)
					break;
			}
			sc->is_scanning = false;
			ieee80211_end_scan(ic);
			break;
		}
		}

		sc->rxq.cur = (sc->rxq.cur + 1) % IWN_RX_RING_COUNT;
	}

	/* tell the firmware what we have processed */
	hw = (hw == 0) ? IWN_RX_RING_COUNT - 1 : hw - 1;
	IWN_WRITE(sc, IWN_RX_WIDX, hw & ~7);
}

static int
iwn_intr(void *arg)
{
	struct iwn_softc *sc = arg;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	uint32_t r1, r2;

	/* disable interrupts */
	IWN_WRITE(sc, IWN_MASK, 0);

	r1 = IWN_READ(sc, IWN_INTR);
	r2 = IWN_READ(sc, IWN_INTR_STATUS);

	if (r1 == 0 && r2 == 0) {
		if (ifp->if_flags & IFF_UP)
			IWN_WRITE(sc, IWN_MASK, IWN_INTR_MASK);
		return 0;	/* not for us */
	}

	if (r1 == 0xffffffff)
		return 0;	/* hardware gone */

	/* ack interrupts */
	IWN_WRITE(sc, IWN_INTR, r1);
	IWN_WRITE(sc, IWN_INTR_STATUS, r2);

	DPRINTFN(5, ("interrupt reg1=%x reg2=%x\n", r1, r2));

	if (r1 & IWN_RF_TOGGLED) {
		uint32_t tmp = IWN_READ(sc, IWN_GPIO_CTL);
		aprint_error_dev(sc->sc_dev, "RF switch: radio %s\n",
		    (tmp & IWN_GPIO_RF_ENABLED) ? "enabled" : "disabled");
		sc->sc_radio = (tmp & IWN_GPIO_RF_ENABLED);
	}
	if (r1 & IWN_CT_REACHED) {
		aprint_error_dev(sc->sc_dev, "critical temperature reached!\n");
	}
	if (r1 & (IWN_SW_ERROR | IWN_HW_ERROR)) {
		aprint_error_dev(sc->sc_dev, "fatal firmware error\n");
		sc->sc_ic.ic_ifp->if_flags &= ~IFF_UP;
		iwn_stop(sc->sc_ic.ic_ifp, 1);
		return 1;
	}

	if ((r1 & (IWN_RX_INTR | IWN_SW_RX_INTR)) ||
	    (r2 & IWN_RX_STATUS_INTR))
		iwn_notif_intr(sc);

	if (r1 & IWN_ALIVE_INTR)
		wakeup(sc);

	/* re-enable interrupts */
	if (ifp->if_flags & IFF_UP)
		IWN_WRITE(sc, IWN_MASK, IWN_INTR_MASK);

	return 1;
}

static uint8_t
iwn_plcp_signal(int rate)
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
	case 120:	return 0x3;
	}
	/* unknown rate (should not get there) */
	return 0;
}

/* determine if a given rate is CCK or OFDM */
#define IWN_RATE_IS_OFDM(rate) ((rate) >= 12 && (rate) != 22)

static int
iwn_tx_data(struct iwn_softc *sc, struct mbuf *m0, struct ieee80211_node *ni,
    int ac)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_tx_ring *ring = &sc->txq[ac];
	struct iwn_tx_desc *desc;
	struct iwn_tx_data *data;
	struct iwn_tx_cmd *cmd;
	struct iwn_cmd_data *tx;
	struct ieee80211_frame *wh;
	struct ieee80211_key *k;
	const struct chanAccParams *cap;
	struct mbuf *mnew;
	bus_addr_t paddr;
	uint32_t flags;
	uint8_t type;
	int i, error, pad, rate, hdrlen, noack = 0;

	DPRINTFN(5, ("iwn_tx_data entry\n"));

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	wh = mtod(m0, struct ieee80211_frame *);
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;

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
	if (type == IEEE80211_FC0_TYPE_MGT) {
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
		struct iwn_tx_radiotap_header *tap = &sc->sc_txtap;

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
	cmd->code = IWN_CMD_TX_DATA;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	tx = (struct iwn_cmd_data *)cmd->data;

	flags = IWN_TX_AUTO_SEQ;
	if (!noack && !IEEE80211_IS_MULTICAST(wh->i_addr1)){
		flags |= IWN_TX_NEED_ACK;
	}else if (m0->m_pkthdr.len + IEEE80211_CRC_LEN > ic->ic_rtsthreshold)
		flags |= (IWN_TX_NEED_RTS | IWN_TX_FULL_TXOP);

	if (IEEE80211_IS_MULTICAST(wh->i_addr1)
	    || (type != IEEE80211_FC0_TYPE_DATA))
		tx->id = IWN_ID_BROADCAST;
	else
		tx->id = IWN_ID_BSS;

	DPRINTFN(5, ("addr1: %x:%x:%x:%x:%x:%x, id = 0x%x\n",
		     wh->i_addr1[0], wh->i_addr1[1], wh->i_addr1[2],
		     wh->i_addr1[3], wh->i_addr1[4], wh->i_addr1[5], tx->id));

	if (type == IEEE80211_FC0_TYPE_MGT) {
		uint8_t subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

		/* tell h/w to set timestamp in probe responses */
		if ((subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
		    (subtype == IEEE80211_FC0_SUBTYPE_PROBE_REQ))
			flags |= IWN_TX_INSERT_TSTAMP;

		if (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ ||
		    subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ ||
		    subtype == IEEE80211_FC0_SUBTYPE_AUTH ||
		    subtype == IEEE80211_FC0_SUBTYPE_DEAUTH) {
			flags &= ~IWN_TX_NEED_RTS;
			flags |= IWN_TX_NEED_CTS;
			tx->timeout = htole16(3);
		} else
			tx->timeout = htole16(2);
	} else
		tx->timeout = htole16(0);

	if (hdrlen & 3) {
		/* first segment's length must be a multiple of 4 */
		flags |= IWN_TX_NEED_PADDING;
		pad = 4 - (hdrlen & 3);
	} else
		pad = 0;

	if (type == IEEE80211_FC0_TYPE_CTL) {
		uint8_t subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

		/* tell h/w to set timestamp in probe responses */
		if (subtype == 0x0080) /* linux says this is "back request" */
			/* linux says (1 << 6) is IMM_BA_RSP_MASK */
			flags |= (IWN_TX_NEED_ACK | (1 << 6));
	}


	tx->flags = htole32(flags);
	tx->len = htole16(m0->m_pkthdr.len);
	tx->rate = iwn_plcp_signal(rate);
	tx->rts_ntries = 60;
	tx->data_ntries = 15;
	tx->lifetime = htole32(IWN_LIFETIME_INFINITE);

	/* XXX alternate between Ant A and Ant B ? */
	tx->rflags = IWN_RFLAG_ANT_B;
	if (tx->id == IWN_ID_BROADCAST) {
		tx->ridx = IWN_MAX_TX_RETRIES - 1;
		if (!IWN_RATE_IS_OFDM(rate))
			tx->rflags |= IWN_RFLAG_CCK;
	} else {
		tx->ridx = 0;
		/* tell adapter to ignore rflags */
		tx->flags |= htole32(IWN_TX_USE_NODE_RATE);
	}

	/* copy and trim IEEE802.11 header */
	memcpy(((uint8_t *)tx) + sizeof(*tx), wh, hdrlen);
	m_adj(m0, hdrlen);

	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m0,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error != 0 && error != EFBIG) {
		aprint_error_dev(sc->sc_dev, "could not map mbuf (error %d)\n", error);
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
			aprint_error_dev(sc->sc_dev, "could not map mbuf (error %d)\n", error);
			m_freem(m0);
			return error;
		}
	}

	data->m = m0;
	data->ni = ni;

	DPRINTFN(4, ("sending data: qid=%d idx=%d len=%d nsegs=%d\n",
		ring->qid, ring->cur, m0->m_pkthdr.len, data->map->dm_nsegs));

	paddr = ring->cmd_dma.paddr + ring->cur * sizeof (struct iwn_tx_cmd);
	tx->loaddr = htole32(paddr + 4 +
	    offsetof(struct iwn_cmd_data, ntries));
	tx->hiaddr = 0; /* limit to 32-bit physical addresses */

	/* first scatter/gather segment is used by the tx data command */
	IWN_SET_DESC_NSEGS(desc, 1 + data->map->dm_nsegs);
	IWN_SET_DESC_SEG(desc, 0, paddr, 4 + sizeof (*tx) + hdrlen + pad);
	for (i = 1; i <= data->map->dm_nsegs; i++) {
		IWN_SET_DESC_SEG(desc, i, data->map->dm_segs[i - 1].ds_addr,
		    data->map->dm_segs[i - 1].ds_len);
	}
	sc->shared->len[ring->qid][ring->cur] =
	    htole16(hdrlen + m0->m_pkthdr.len + 8);
	if (ring->cur < IWN_TX_WINDOW) {
		sc->shared->len[ring->qid][ring->cur + IWN_TX_RING_COUNT] =
		    htole16(hdrlen + m0->m_pkthdr.len + 8);
	}

	ring->queued++;

	bus_dmamap_sync(sc->sc_dmat, data->map, 0,
	    data->map->dm_mapsize /* calc? */, BUS_DMASYNC_PREWRITE);

	/* kick ring */
	ring->cur = (ring->cur + 1) % IWN_TX_RING_COUNT;
	IWN_WRITE(sc, IWN_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;
}

static void
iwn_start(struct ifnet *ifp)
{
	struct iwn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ether_header *eh;
	struct mbuf *m0;
	int ac;

	DPRINTFN(5, ("iwn_start enter\n"));

	/*
	 * net80211 may still try to send management frames even if the
	 * IFF_RUNNING flag is not set... Also, don't bother if the radio
	 * is not enabled.
	 */
	if (((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING) ||
	    !sc->sc_radio)
		return;

	for (;;) {
		IF_DEQUEUE(&ic->ic_mgtq, m0);
		if (m0 != NULL) {
			/* management frames go into ring 0 */


			ni = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
			m0->m_pkthdr.rcvif = NULL;

			/* management goes into ring 0 */
			if (sc->txq[0].queued > sc->txq[0].count - 8) {
				ifp->if_oerrors++;
				continue;
			}

#if NBPFILTER > 0
			if (ic->ic_rawbpf != NULL)
				bpf_mtap(ic->ic_rawbpf, m0);
#endif
			if (iwn_tx_data(sc, m0, ni, 0) != 0) {
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
			    (m0 = m_pullup(m0, sizeof (*eh))) == NULL) {
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
			if (iwn_tx_data(sc, m0, ni, ac) != 0) {
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
iwn_watchdog(struct ifnet *ifp)
{
	struct iwn_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			aprint_error_dev(sc->sc_dev, "device timeout\n");
			ifp->if_flags &= ~IFF_UP;
			iwn_stop(ifp, 1);
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(&sc->sc_ic);
}

static int
iwn_ioctl(struct ifnet *ifp, u_long cmd, void * data)
{

#define IS_RUNNING(ifp)							\
	((ifp->if_flags & IFF_UP) && (ifp->if_flags & IFF_RUNNING))

	struct iwn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		if (ifp->if_flags & IFF_UP) {
			/*
			 * resync the radio state just in case we missed
			 * and event.
			 */
			sc->sc_radio =
			    (IWN_READ(sc, IWN_GPIO_CTL) & IWN_GPIO_RF_ENABLED);

			if (!sc->sc_radio) {
				ifp->if_flags &= ~IFF_UP;
				error = EBUSY; /* XXX not really but same as elsewhere in driver */
				if (ifp->if_flags & IFF_RUNNING)
					iwn_stop(ifp, 1);
			} else if (!(ifp->if_flags & IFF_RUNNING))
				iwn_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				iwn_stop(ifp, 1);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* XXX no h/w multicast filter? --dyoung */
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
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
			iwn_init(ifp);
		error = 0;
	}

	splx(s);
	return error;

#undef IS_RUNNING
}

static void
iwn_read_eeprom(struct iwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	char domain[4];
	uint16_t val;
	int i, error;

	if ((error = iwn_eeprom_lock(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not lock EEPROM (error=%d)\n", error);
		return;
	}
	/* read and print regulatory domain */
	iwn_read_prom_data(sc, IWN_EEPROM_DOMAIN, domain, 4);
	aprint_error_dev(sc->sc_dev, "%.4s", domain);

	/* read and print MAC address */
	iwn_read_prom_data(sc, IWN_EEPROM_MAC, ic->ic_myaddr, 6);
	aprint_error(", address %s\n", ether_sprintf(ic->ic_myaddr));

	/* read the list of authorized channels */
	for (i = 0; i < IWN_CHAN_BANDS_COUNT; i++)
		iwn_read_eeprom_channels(sc, i);

	/* read maximum allowed Tx power for 2GHz and 5GHz bands */
	iwn_read_prom_data(sc, IWN_EEPROM_MAXPOW, &val, 2);
	sc->maxpwr2GHz = val & 0xff;
	sc->maxpwr5GHz = val >> 8;
	/* check that EEPROM values are correct */
	if (sc->maxpwr5GHz < 20 || sc->maxpwr5GHz > 50)
		sc->maxpwr5GHz = 38;
	if (sc->maxpwr2GHz < 20 || sc->maxpwr2GHz > 50)
		sc->maxpwr2GHz = 38;
	DPRINTF(("maxpwr 2GHz=%d 5GHz=%d\n", sc->maxpwr2GHz, sc->maxpwr5GHz));

	/* read voltage at which samples were taken */
	iwn_read_prom_data(sc, IWN_EEPROM_VOLTAGE, &val, 2);
	sc->eeprom_voltage = (int16_t)le16toh(val);
	DPRINTF(("voltage=%d (in 0.3V)\n", sc->eeprom_voltage));

	/* read power groups */
	iwn_read_prom_data(sc, IWN_EEPROM_BANDS, sc->bands, sizeof sc->bands);
#ifdef IWN_DEBUG
	if (iwn_debug > 0) {
		for (i = 0; i < IWN_NBANDS; i++)
			iwn_print_power_group(sc, i);
	}
#endif
	iwn_eeprom_unlock(sc);
}

static void
iwn_read_eeprom_channels(struct iwn_softc *sc, int n)
{
	struct ieee80211com *ic = &sc->sc_ic;
	const struct iwn_chan_band *band = &iwn_bands[n];
	struct iwn_eeprom_chan channels[IWN_MAX_CHAN_PER_BAND];
	int chan, i;

	iwn_read_prom_data(sc, band->addr, channels,
	    band->nchan * sizeof (struct iwn_eeprom_chan));

	for (i = 0; i < band->nchan; i++) {
		if (!(channels[i].flags & IWN_EEPROM_CHAN_VALID))
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
			 * Some adapters support channels 7, 8, 11 and 12
			 * both in the 2GHz *and* 5GHz bands.
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
		if (!(channels[i].flags & IWN_EEPROM_CHAN_ACTIVE)) {
			ic->ic_channels[chan].ic_flags |=
			    IEEE80211_CHAN_PASSIVE;
		}

		/* save maximum allowed power for this channel */
		sc->maxpwr[chan] = channels[i].maxpwr;

		DPRINTF(("adding chan %d flags=0x%x maxpwr=%d\n",
			chan, channels[i].flags, sc->maxpwr[chan]));
	}
}

#ifdef IWN_DEBUG
static void
iwn_print_power_group(struct iwn_softc *sc, int i)
{
	struct iwn_eeprom_band *band = &sc->bands[i];
	struct iwn_eeprom_chan_samples *chans = band->chans;
	int j, c;

	DPRINTF(("===band %d===\n", i));
	DPRINTF(("chan lo=%d, chan hi=%d\n", band->lo, band->hi));
	DPRINTF(("chan1 num=%d\n", chans[0].num));
	for (c = 0; c < IWN_NTXCHAINS; c++) {
		for (j = 0; j < IWN_NSAMPLES; j++) {
			DPRINTF(("chain %d, sample %d: temp=%d gain=%d "
				"power=%d pa_det=%d\n", c, j,
				chans[0].samples[c][j].temp,
				chans[0].samples[c][j].gain,
				chans[0].samples[c][j].power,
				chans[0].samples[c][j].pa_det));
		}
	}
	DPRINTF(("chan2 num=%d\n", chans[1].num));
	for (c = 0; c < IWN_NTXCHAINS; c++) {
		for (j = 0; j < IWN_NSAMPLES; j++) {
			DPRINTF(("chain %d, sample %d: temp=%d gain=%d "
				"power=%d pa_det=%d\n", c, j,
				chans[1].samples[c][j].temp,
				chans[1].samples[c][j].gain,
				chans[1].samples[c][j].power,
				chans[1].samples[c][j].pa_det));
		}
	}
}
#endif

/*
 * Send a command to the firmware.
 */
static int
iwn_cmd(struct iwn_softc *sc, int code, const void *buf, int size, int async)
{
	struct iwn_tx_ring *ring = &sc->txq[4];
	struct iwn_tx_desc *desc;
	struct iwn_tx_cmd *cmd;
	bus_addr_t paddr;

	KASSERT(size <= sizeof cmd->data);

	desc = &ring->desc[ring->cur];
	cmd = &ring->cmd[ring->cur];

	cmd->code = code;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;
	memcpy(cmd->data, buf, size);

	paddr = ring->cmd_dma.paddr + ring->cur * sizeof (struct iwn_tx_cmd);

	IWN_SET_DESC_NSEGS(desc, 1);
	IWN_SET_DESC_SEG(desc, 0, paddr, 4 + size);
	sc->shared->len[ring->qid][ring->cur] = htole16(8);
	if (ring->cur < IWN_TX_WINDOW) {
		sc->shared->len[ring->qid][ring->cur + IWN_TX_RING_COUNT] =
		    htole16(8);
	}

	bus_dmamap_sync(sc->sc_dmat, ring->cmd_dma.map, 0,
	    4 + size, BUS_DMASYNC_PREWRITE);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % IWN_TX_RING_COUNT;
	IWN_WRITE(sc, IWN_TX_WIDX, ring->qid << 8 | ring->cur);

	return async ? 0 : tsleep(cmd, PCATCH, "iwncmd", hz);
}

/*
 * Configure hardware multi-rate retries for one node.
 */
static int
iwn_setup_node_mrr(struct iwn_softc *sc, uint8_t id, int async)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_cmd_mrr mrr;
	int i, ridx;

	memset(&mrr, 0, sizeof mrr);
	mrr.id = id;
	mrr.ssmask = 2;
	mrr.dsmask = 3;
	mrr.ampdu_disable = 3;
	mrr.ampdu_limit = htole16(4000);

	if (id == IWN_ID_BSS)
		ridx = IWN_OFDM54;
	else if (ic->ic_curmode == IEEE80211_MODE_11A)
		ridx = IWN_OFDM6;
	else
		ridx = IWN_CCK1;
	for (i = 0; i < IWN_MAX_TX_RETRIES; i++) {
		mrr.table[i].rate = iwn_ridx_to_plcp[ridx];
		mrr.table[i].rflags = IWN_RFLAG_ANT_B;
		if (ridx <= IWN_CCK11)
			mrr.table[i].rflags |= IWN_RFLAG_CCK;
		ridx = iwn_prev_ridx[ridx];
	}
	return iwn_cmd(sc, IWN_CMD_NODE_MRR_SETUP, &mrr, sizeof mrr, async);
}

static int
iwn_wme_update(struct ieee80211com *ic)
{
#define IWN_EXP2(v)	htole16((1 << (v)) - 1)
#define IWN_USEC(v)	htole16(IEEE80211_TXOP_TO_US(v))
	struct iwn_softc *sc = ic->ic_ifp->if_softc;
	const struct wmeParams *wmep;
	struct iwn_wme_setup wme;
	int ac;

	/* don't override default WME values if WME is not actually enabled */
	if (!(ic->ic_flags & IEEE80211_F_WME))
		return 0;

	wme.flags = 0;
	for (ac = 0; ac < WME_NUM_AC; ac++) {
		wmep = &ic->ic_wme.wme_chanParams.cap_wmeParams[ac];
		wme.ac[ac].aifsn = wmep->wmep_aifsn;
		wme.ac[ac].cwmin = IWN_EXP2(wmep->wmep_logcwmin);
		wme.ac[ac].cwmax = IWN_EXP2(wmep->wmep_logcwmax);
		wme.ac[ac].txop	 = IWN_USEC(wmep->wmep_txopLimit);

		DPRINTF(("setting WME for queue %d aifsn=%d cwmin=%d cwmax=%d "
			"txop=%d\n", ac, wme.ac[ac].aifsn, wme.ac[ac].cwmin,
			wme.ac[ac].cwmax, wme.ac[ac].txop));
	}

	return iwn_cmd(sc, IWN_CMD_SET_WME, &wme, sizeof wme, 1);
#undef IWN_USEC
#undef IWN_EXP2
}



static void
iwn_set_led(struct iwn_softc *sc, uint8_t which, uint8_t off, uint8_t on)
{
	struct iwn_cmd_led led;

	led.which = which;
	led.unit = htole32(100000);	/* on/off in unit of 100ms */
	led.off = off;
	led.on = on;

	(void)iwn_cmd(sc, IWN_CMD_SET_LED, &led, sizeof led, 1);
}

/*
 * Set the critical temperature at which the firmware will automatically stop
 * the radio transmitter.
 */
static int
iwn_set_critical_temp(struct iwn_softc *sc)
{
	struct iwn_ucode_info *uc = &sc->ucode_info;
	struct iwn_critical_temp crit;
	uint32_t r1, r2, r3, temp;

	IWN_WRITE(sc, IWN_UCODE_CLR, IWN_CTEMP_STOP_RF);

	r1 = le32toh(uc->temp[0].chan20MHz);
	r2 = le32toh(uc->temp[1].chan20MHz);
	r3 = le32toh(uc->temp[2].chan20MHz);
	/* inverse function of iwn_get_temperature() */

	temp = r2 + ((IWN_CTOK(110) * (r3 - r1)) / 259);

	memset(&crit, 0, sizeof crit);
	crit.tempR = htole32(temp);
	DPRINTF(("setting critical temperature to %u\n", temp));
	return iwn_cmd(sc, IWN_CMD_SET_CRITICAL_TEMP, &crit, sizeof crit, 0);
}

static void
iwn_enable_tsf(struct iwn_softc *sc, struct ieee80211_node *ni)
{
	struct iwn_cmd_tsf tsf;
	uint64_t val, mod;

	memset(&tsf, 0, sizeof tsf);
	memcpy(&tsf.tstamp, ni->ni_tstamp.data, 8);
	tsf.bintval = htole16(ni->ni_intval);
	tsf.lintval = htole16(10);

	/* compute remaining time until next beacon */
	val = (uint64_t)ni->ni_intval * 1024;	/* msecs -> usecs */
	mod = le64toh(tsf.tstamp) % val;
	tsf.binitval = htole32((uint32_t)(val - mod));

	DPRINTF(("TSF bintval=%u tstamp=%" PRIu64 ", init=%" PRIu64 "\n",
	    ni->ni_intval, le64toh(tsf.tstamp), val - mod));

	if (iwn_cmd(sc, IWN_CMD_TSF, &tsf, sizeof tsf, 1) != 0)
		aprint_error_dev(sc->sc_dev, "could not enable TSF\n");
}

static void
iwn_power_calibration(struct iwn_softc *sc, int temp)
{
	struct ieee80211com *ic = &sc->sc_ic;

	DPRINTF(("temperature %d->%d\n", sc->temp, temp));

	/* adjust Tx power if need be (delta >= 3�C) */
	if (abs(temp - sc->temp) < 3)
		return;

	sc->temp = temp;

	DPRINTF(("setting Tx power for channel %d\n",
		ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan)));
	if (iwn_set_txpower(sc, ic->ic_bss->ni_chan, 1) != 0) {
		/* just warn, too bad for the automatic calibration... */
		aprint_error_dev(sc->sc_dev, "could not adjust Tx power\n");
	}
}

/*
 * Set Tx power for a given channel (each rate has its own power settings).
 * This function takes into account the regulatory information from EEPROM,
 * the current temperature and the current voltage.
 */
static int
iwn_set_txpower(struct iwn_softc *sc, struct ieee80211_channel *ch, int async)
{
/* fixed-point arithmetic division using a n-bit fractional part */
#define fdivround(a, b, n)						\
	((((1 << n) * (a)) / (b) + (1 << n) / 2) / (1 << n))
/* linear interpolation */
#define interpolate(x, x1, y1, x2, y2, n)				\
	((y1) + fdivround(((int)(x) - (x1)) * ((y2) - (y1)), (x2) - (x1), n))

	static const int tdiv[IWN_NATTEN_GROUPS] = { 9, 8, 8, 8, 6 };
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_ucode_info *uc = &sc->ucode_info;
	struct iwn_cmd_txpower cmd;
	struct iwn_eeprom_chan_samples *chans;
	const uint8_t *rf_gain, *dsp_gain;
	int32_t vdiff, tdiff;
	int i, c, grp, maxpwr;
	u_int chan;

	/* get channel number */
	chan = ieee80211_chan2ieee(ic, ch);

	memset(&cmd, 0, sizeof cmd);
	cmd.band = IEEE80211_IS_CHAN_5GHZ(ch) ? 0 : 1;
	cmd.chan = chan;

	if (IEEE80211_IS_CHAN_5GHZ(ch)) {
		maxpwr	 = sc->maxpwr5GHz;
		rf_gain	 = iwn_rf_gain_5ghz;
		dsp_gain = iwn_dsp_gain_5ghz;
	} else {
		maxpwr	 = sc->maxpwr2GHz;
		rf_gain	 = iwn_rf_gain_2ghz;
		dsp_gain = iwn_dsp_gain_2ghz;
	}

	/* compute voltage compensation */
	vdiff = ((int32_t)le32toh(uc->volt) - sc->eeprom_voltage) / 7;
	if (vdiff > 0)
		vdiff *= 2;
	if (abs(vdiff) > 2)
		vdiff = 0;
	DPRINTF(("voltage compensation=%d (UCODE=%d, EEPROM=%d)\n",
		vdiff, le32toh(uc->volt), sc->eeprom_voltage));

	/* get channel's attenuation group */
	if (chan <= 20)		/* 1-20 */
		grp = 4;
	else if (chan <= 43)	/* 34-43 */
		grp = 0;
	else if (chan <= 70)	/* 44-70 */
		grp = 1;
	else if (chan <= 124)	/* 71-124 */
		grp = 2;
	else			/* 125-200 */
		grp = 3;
	DPRINTF(("chan %d, attenuation group=%d\n", chan, grp));

	/* get channel's sub-band */
	for (i = 0; i < IWN_NBANDS; i++)
		if (sc->bands[i].lo != 0 &&
		    sc->bands[i].lo <= chan && chan <= sc->bands[i].hi)
			break;
	chans = sc->bands[i].chans;
	DPRINTF(("chan %d sub-band=%d\n", chan, i));

	for (c = 0; c < IWN_NTXCHAINS; c++) {
		uint8_t power, gain, temp;
		int maxchpwr, pwr, ridx, idx;

		power = interpolate(chan,
		    chans[0].num, chans[0].samples[c][1].power,
		    chans[1].num, chans[1].samples[c][1].power, 1);
		gain  = interpolate(chan,
		    chans[0].num, chans[0].samples[c][1].gain,
		    chans[1].num, chans[1].samples[c][1].gain, 1);
		temp  = interpolate(chan,
		    chans[0].num, chans[0].samples[c][1].temp,
		    chans[1].num, chans[1].samples[c][1].temp, 1);
		DPRINTF(("Tx chain %d: power=%d gain=%d temp=%d\n",
			c, power, gain, temp));

		/* compute temperature compensation */
		tdiff = ((sc->temp - temp) * 2) / tdiv[grp];
		DPRINTF(("temperature compensation=%d (current=%d, "
			"EEPROM=%d)\n", tdiff, sc->temp, temp));

		for (ridx = 0; ridx <= IWN_RIDX_MAX; ridx++) {
			maxchpwr = sc->maxpwr[chan] * 2;
			if ((ridx / 8) & 1) {
				/* MIMO: decrease Tx power (-3dB) */
				maxchpwr -= 6;
			}

			pwr = maxpwr - 10;

			/* decrease power for highest OFDM rates */
			if ((ridx % 8) == 5)		/* 48Mbit/s */
				pwr -= 5;
			else if ((ridx % 8) == 6)	/* 54Mbit/s */
				pwr -= 7;
			else if ((ridx % 8) == 7)	/* 60Mbit/s */
				pwr -= 10;

			if (pwr > maxchpwr)
				pwr = maxchpwr;

			idx = gain - (pwr - power) - tdiff - vdiff;
			if ((ridx / 8) & 1)	/* MIMO */
				idx += (int32_t)le32toh(uc->atten[grp][c]);

			if (cmd.band == 0)
				idx += 9;	/* 5GHz */
			if (ridx == IWN_RIDX_MAX)
				idx += 5;	/* CCK */

			/* make sure idx stays in a valid range */
			if (idx < 0)
				idx = 0;
			else if (idx > IWN_MAX_PWR_INDEX)
				idx = IWN_MAX_PWR_INDEX;

			DPRINTF(("Tx chain %d, rate idx %d: power=%d\n",
				c, ridx, idx));
			cmd.power[ridx].rf_gain[c] = rf_gain[idx];
			cmd.power[ridx].dsp_gain[c] = dsp_gain[idx];
		}
	}

	DPRINTF(("setting tx power for chan %d\n", chan));
	return iwn_cmd(sc, IWN_CMD_TXPOWER, &cmd, sizeof cmd, async);

#undef interpolate
#undef fdivround
}

/*
 * Get the best (maximum) RSSI among Rx antennas (in dBm).
 */
static int
iwn_get_rssi(const struct iwn_rx_stat *stat)
{
	uint8_t mask, agc;
	int rssi;

	mask = (le16toh(stat->antenna) >> 4) & 0x7;
	agc  = (le16toh(stat->agc) >> 7) & 0x7f;

	rssi = 0;
	if (mask & (1 << 0))	/* Ant A */
		rssi = max(rssi, stat->rssi[0]);
	if (mask & (1 << 1))	/* Ant B */
		rssi = max(rssi, stat->rssi[2]);
	if (mask & (1 << 2))	/* Ant C */
		rssi = max(rssi, stat->rssi[4]);

	return rssi - agc - IWN_RSSI_TO_DBM;
}

/*
 * Get the average noise among Rx antennas (in dBm).
 */
static int
iwn_get_noise(const struct iwn_rx_general_stats *stats)
{
	int i, total, nbant, noise;

	total = nbant = 0;
	for (i = 0; i < 3; i++) {
		if ((noise = le32toh(stats->noise[i]) & 0xff) == 0)
			continue;
		total += noise;
		nbant++;
	}
	/* there should be at least one antenna but check anyway */
	return (nbant == 0) ? -127 : (total / nbant) - 107;
}

/*
 * Read temperature (in degC) from the on-board thermal sensor.
 */
static int
iwn_get_temperature(struct iwn_softc *sc)
{
	struct iwn_ucode_info *uc = &sc->ucode_info;
	int32_t r1, r2, r3, r4, temp;

	r1 = le32toh(uc->temp[0].chan20MHz);
	r2 = le32toh(uc->temp[1].chan20MHz);
	r3 = le32toh(uc->temp[2].chan20MHz);
	r4 = le32toh(sc->rawtemp);

	if (r1 == r3)	/* prevents division by 0 (should not happen) */
		return 0;

	/* sign-extend 23-bit R4 value to 32-bit */
	r4 = (r4 << 8) >> 8;
	/* compute temperature */
	temp = (259 * (r4 - r2)) / (r3 - r1);
	temp = (temp * 97) / 100 + 8;

	DPRINTF(("temperature %dK/%dC\n", temp, IWN_KTOC(temp)));
	return IWN_KTOC(temp);
}

/*
 * Initialize sensitivity calibration state machine.
 */
static int
iwn_init_sensitivity(struct iwn_softc *sc)
{
	struct iwn_calib_state *calib = &sc->calib;
	struct iwn_phy_calib_cmd cmd;
	int error;

	/* reset calibration state */
	memset(calib, 0, sizeof (*calib));
	calib->state = IWN_CALIB_STATE_INIT;
	calib->cck_state = IWN_CCK_STATE_HIFA;
	/* initial values taken from the reference driver */
	calib->corr_ofdm_x1	= 105;
	calib->corr_ofdm_mrc_x1 = 220;
	calib->corr_ofdm_x4	=  90;
	calib->corr_ofdm_mrc_x4 = 170;
	calib->corr_cck_x4	= 125;
	calib->corr_cck_mrc_x4	= 200;
	calib->energy_cck	= 100;

	/* write initial sensitivity values */
	if ((error = iwn_send_sensitivity(sc)) != 0)
		return error;

	memset(&cmd, 0, sizeof cmd);
	cmd.code = IWN_SET_DIFF_GAIN;
	/* differential gains initially set to 0 for all 3 antennas */
	DPRINTF(("setting differential gains\n"));
	return iwn_cmd(sc, IWN_PHY_CALIB, &cmd, sizeof cmd, 1);
}

/*
 * Collect noise and RSSI statistics for the first 20 beacons received
 * after association and use them to determine connected antennas and
 * set differential gains.
 */
static void
iwn_compute_differential_gain(struct iwn_softc *sc,
    const struct iwn_rx_general_stats *stats)
{
	struct iwn_calib_state *calib = &sc->calib;
	struct iwn_phy_calib_cmd cmd;
	int i, val;

	/* accumulate RSSI and noise for all 3 antennas */
	for (i = 0; i < 3; i++) {
		calib->rssi[i] += le32toh(stats->rssi[i]) & 0xff;
		calib->noise[i] += le32toh(stats->noise[i]) & 0xff;
	}

	/* we update differential gain only once after 20 beacons */
	if (++calib->nbeacons < 20)
		return;

	/* determine antenna with highest average RSSI */
	val = max(calib->rssi[0], calib->rssi[1]);
	val = max(calib->rssi[2], val);

	/* determine which antennas are connected */
	sc->antmsk = 0;
	for (i = 0; i < 3; i++)
		if (val - calib->rssi[i] <= 15 * 20)
			sc->antmsk |= 1 << i;
	/* if neither Ant A and Ant B are connected.. */
	if ((sc->antmsk & (1 << 0 | 1 << 1)) == 0)
		sc->antmsk |= 1 << 1;	/* ..mark Ant B as connected! */

	/* get minimal noise among connected antennas */
	val = INT_MAX;	/* ok, there's at least one */
	for (i = 0; i < 3; i++)
		if (sc->antmsk & (1 << i))
			val = min(calib->noise[i], val);

	memset(&cmd, 0, sizeof cmd);
	cmd.code = IWN_SET_DIFF_GAIN;
	/* set differential gains for connected antennas */
	for (i = 0; i < 3; i++) {
		if (sc->antmsk & (1 << i)) {
			cmd.gain[i] = (calib->noise[i] - val) / 30;
			/* limit differential gain to 3 */
			cmd.gain[i] = min(cmd.gain[i], 3);
			cmd.gain[i] |= IWN_GAIN_SET;
		}
	}
	DPRINTF(("setting differential gains Ant A/B/C: %x/%x/%x (%x)\n",
		cmd.gain[0], cmd.gain[1], cmd.gain[2], sc->antmsk));
	if (iwn_cmd(sc, IWN_PHY_CALIB, &cmd, sizeof cmd, 1) == 0)
		calib->state = IWN_CALIB_STATE_RUN;
}

/*
 * Tune RF Rx sensitivity based on the number of false alarms detected
 * during the last beacon period.
 */
static void
iwn_tune_sensitivity(struct iwn_softc *sc, const struct iwn_rx_stats *stats)
{
#define inc_clip(val, inc, max)						\
	if ((val) < (max)) {						\
		if ((val) < (max) - (inc))				\
			(val) += (inc);					\
		else							\
			(val) = (max);					\
		needs_update = 1;					\
	}
#define dec_clip(val, dec, min)						\
	if ((val) > (min)) {						\
		if ((val) > (min) + (dec))				\
			(val) -= (dec);					\
		else							\
			(val) = (min);					\
		needs_update = 1;					\
	}

	struct iwn_calib_state *calib = &sc->calib;
	uint32_t val, rxena, fa;
	uint32_t energy[3], energy_min;
	uint8_t noise[3], noise_ref;
	int i, needs_update = 0;

	/* check that we've been enabled long enough */
	if ((rxena = le32toh(stats->general.load)) == 0)
		return;

	/* compute number of false alarms since last call for OFDM */
	fa  = le32toh(stats->ofdm.bad_plcp) - calib->bad_plcp_ofdm;
	fa += le32toh(stats->ofdm.fa) - calib->fa_ofdm;
	fa *= 200 * 1024;	/* 200TU */

	/* save counters values for next call */
	calib->bad_plcp_ofdm = le32toh(stats->ofdm.bad_plcp);
	calib->fa_ofdm = le32toh(stats->ofdm.fa);

	if (fa > 50 * rxena) {
		/* high false alarm count, decrease sensitivity */
		DPRINTFN(2, ("OFDM high false alarm count: %u\n", fa));
		inc_clip(calib->corr_ofdm_x1,	  1, 140);
		inc_clip(calib->corr_ofdm_mrc_x1, 1, 270);
		inc_clip(calib->corr_ofdm_x4,	  1, 120);
		inc_clip(calib->corr_ofdm_mrc_x4, 1, 210);

	} else if (fa < 5 * rxena) {
		/* low false alarm count, increase sensitivity */
		DPRINTFN(2, ("OFDM low false alarm count: %u\n", fa));
		dec_clip(calib->corr_ofdm_x1,	  1, 105);
		dec_clip(calib->corr_ofdm_mrc_x1, 1, 220);
		dec_clip(calib->corr_ofdm_x4,	  1,  85);
		dec_clip(calib->corr_ofdm_mrc_x4, 1, 170);
	}

	/* compute maximum noise among 3 antennas */
	for (i = 0; i < 3; i++)
		noise[i] = (le32toh(stats->general.noise[i]) >> 8) & 0xff;
	val = max(noise[0], noise[1]);
	val = max(noise[2], val);
	/* insert it into our samples table */
	calib->noise_samples[calib->cur_noise_sample] = val;
	calib->cur_noise_sample = (calib->cur_noise_sample + 1) % 20;

	/* compute maximum noise among last 20 samples */
	noise_ref = calib->noise_samples[0];
	for (i = 1; i < 20; i++)
		noise_ref = max(noise_ref, calib->noise_samples[i]);

	/* compute maximum energy among 3 antennas */
	for (i = 0; i < 3; i++)
		energy[i] = le32toh(stats->general.energy[i]);
	val = min(energy[0], energy[1]);
	val = min(energy[2], val);
	/* insert it into our samples table */
	calib->energy_samples[calib->cur_energy_sample] = val;
	calib->cur_energy_sample = (calib->cur_energy_sample + 1) % 10;

	/* compute minimum energy among last 10 samples */
	energy_min = calib->energy_samples[0];
	for (i = 1; i < 10; i++)
		energy_min = max(energy_min, calib->energy_samples[i]);
	energy_min += 6;

	/* compute number of false alarms since last call for CCK */
	fa  = le32toh(stats->cck.bad_plcp) - calib->bad_plcp_cck;
	fa += le32toh(stats->cck.fa) - calib->fa_cck;
	fa *= 200 * 1024;	/* 200TU */

	/* save counters values for next call */
	calib->bad_plcp_cck = le32toh(stats->cck.bad_plcp);
	calib->fa_cck = le32toh(stats->cck.fa);

	if (fa > 50 * rxena) {
		/* high false alarm count, decrease sensitivity */
		DPRINTFN(2, ("CCK high false alarm count: %u\n", fa));
		calib->cck_state = IWN_CCK_STATE_HIFA;
		calib->low_fa = 0;

		if (calib->corr_cck_x4 > 160) {
			calib->noise_ref = noise_ref;
			if (calib->energy_cck > 2)
				dec_clip(calib->energy_cck, 2, energy_min);
		}
		if (calib->corr_cck_x4 < 160) {
			calib->corr_cck_x4 = 161;
			needs_update = 1;
		} else
			inc_clip(calib->corr_cck_x4, 3, 200);

		inc_clip(calib->corr_cck_mrc_x4, 3, 400);

	} else if (fa < 5 * rxena) {
		/* low false alarm count, increase sensitivity */
		DPRINTFN(2, ("CCK low false alarm count: %u\n", fa));
		calib->cck_state = IWN_CCK_STATE_LOFA;
		calib->low_fa++;

		if (calib->cck_state != 0 &&
		    ((calib->noise_ref - noise_ref) > 2 ||
			calib->low_fa > 100)) {
			inc_clip(calib->energy_cck,	 2,  97);
			dec_clip(calib->corr_cck_x4,	 3, 125);
			dec_clip(calib->corr_cck_mrc_x4, 3, 200);
		}
	} else {
		/* not worth to increase or decrease sensitivity */
		DPRINTFN(2, ("CCK normal false alarm count: %u\n", fa));
		calib->low_fa = 0;
		calib->noise_ref = noise_ref;

		if (calib->cck_state == IWN_CCK_STATE_HIFA) {
			/* previous interval had many false alarms */
			dec_clip(calib->energy_cck, 8, energy_min);
		}
		calib->cck_state = IWN_CCK_STATE_INIT;
	}

	if (needs_update)
		(void)iwn_send_sensitivity(sc);
#undef dec_clip
#undef inc_clip
}

static int
iwn_send_sensitivity(struct iwn_softc *sc)
{
	struct iwn_calib_state *calib = &sc->calib;
	struct iwn_sensitivity_cmd cmd;

	memset(&cmd, 0, sizeof cmd);
	cmd.which = IWN_SENSITIVITY_WORKTBL;
	/* OFDM modulation */
	cmd.corr_ofdm_x1     = le16toh(calib->corr_ofdm_x1);
	cmd.corr_ofdm_mrc_x1 = le16toh(calib->corr_ofdm_mrc_x1);
	cmd.corr_ofdm_x4     = le16toh(calib->corr_ofdm_x4);
	cmd.corr_ofdm_mrc_x4 = le16toh(calib->corr_ofdm_mrc_x4);
	cmd.energy_ofdm	     = le16toh(100);
	cmd.energy_ofdm_th   = le16toh(62);
	/* CCK modulation */
	cmd.corr_cck_x4	     = le16toh(calib->corr_cck_x4);
	cmd.corr_cck_mrc_x4  = le16toh(calib->corr_cck_mrc_x4);
	cmd.energy_cck	     = le16toh(calib->energy_cck);
	/* Barker modulation: use default values */
	cmd.corr_barker	     = le16toh(190);
	cmd.corr_barker_mrc  = le16toh(390);

	DPRINTFN(2, ("setting sensitivity\n"));
	return iwn_cmd(sc, IWN_SENSITIVITY, &cmd, sizeof cmd, 1);
}

static int
iwn_add_node(struct iwn_softc *sc, struct ieee80211_node *ni, bool broadcast,
	     bool async, uint32_t htflags)
{
	struct iwn_node_info node;
	int error;

	error = 0;

	memset(&node, 0, sizeof node);
	if (broadcast == true) {
		IEEE80211_ADDR_COPY(node.macaddr, etherbroadcastaddr);
		node.id = IWN_ID_BROADCAST;
		DPRINTF(("adding broadcast node\n"));
	} else {
		IEEE80211_ADDR_COPY(node.macaddr, ni->ni_macaddr);
		node.id = IWN_ID_BSS;
		node.htflags = htole32(htflags);
		DPRINTF(("adding BSS node\n"));
	}

	error = iwn_cmd(sc, IWN_CMD_ADD_NODE, &node, sizeof node, async);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not add %s node\n",
				 (broadcast == 1)? "broadcast" : "BSS");
		return error;
	}
	DPRINTF(("setting MRR for node %d\n", node.id));
	if ((error = iwn_setup_node_mrr(sc, node.id, async)) != 0) {
		aprint_error_dev(sc->sc_dev,
				 "could not setup MRR for %s node\n",
				 (broadcast == 1)? "broadcast" : "BSS");
		return error;
	}

	return error;
}

static int
iwn_auth(struct iwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	int error;

	sc->calib.state = IWN_CALIB_STATE_INIT;

	/* update adapter's configuration */
	sc->config.associd = 0;
	IEEE80211_ADDR_COPY(sc->config.bssid, ni->ni_bssid);
	sc->config.chan = htole16(ieee80211_chan2ieee(ic, ni->ni_chan));
	sc->config.flags = htole32(IWN_CONFIG_TSF);
	if (IEEE80211_IS_CHAN_2GHZ(ni->ni_chan)) {
		sc->config.flags |= htole32(IWN_CONFIG_AUTO |
		    IWN_CONFIG_24GHZ);
	}
	if (IEEE80211_IS_CHAN_A(ni->ni_chan)) {
		sc->config.cck_mask  = 0;
		sc->config.ofdm_mask = 0x15;
	} else if (IEEE80211_IS_CHAN_B(ni->ni_chan)) {
		sc->config.cck_mask  = 0x03;
		sc->config.ofdm_mask = 0;
	} else {
		/* assume 802.11b/g */
		sc->config.cck_mask  = 0xf;
		sc->config.ofdm_mask = 0x15;
	}

	if (ic->ic_flags & IEEE80211_F_SHSLOT)
		sc->config.flags |= htole32(IWN_CONFIG_SHSLOT);
	if (ic->ic_flags & IEEE80211_F_SHPREAMBLE)
		sc->config.flags |= htole32(IWN_CONFIG_SHPREAMBLE);
	sc->config.filter &= ~htole32(IWN_FILTER_BSS);

	DPRINTF(("config chan %d flags %x cck %x ofdm %x\n", sc->config.chan,
		sc->config.flags, sc->config.cck_mask, sc->config.ofdm_mask));
	error = iwn_cmd(sc, IWN_CMD_CONFIGURE, &sc->config,
	    sizeof (struct iwn_config), 1);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not configure\n");
		return error;
	}

	/* configuration has changed, set Tx power accordingly */
	if ((error = iwn_set_txpower(sc, ni->ni_chan, 1)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set Tx power\n");
		return error;
	}

	/*
	 * Reconfiguring clears the adapter's nodes table so we must
	 * add the broadcast node again.
	 */
	if ((error = iwn_add_node(sc, ni, true, true, 0)) != 0)
		return error;

	/* add BSS node */
	if ((error = iwn_add_node(sc, ni, false, true, 0)) != 0)
		return error;

	if (ic->ic_opmode == IEEE80211_M_STA) {
		/* fake a join to init the tx rate */
		iwn_newassoc(ni, 1);
	}

	if ((error = iwn_init_sensitivity(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set sensitivity\n");
		return error;
	}


	return 0;
}

/*
 * Configure the adapter for associated state.
 */
static int
iwn_run(struct iwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	int error;

	if (ic->ic_opmode == IEEE80211_M_MONITOR) {
		/* link LED blinks while monitoring */
		iwn_set_led(sc, IWN_LED_LINK, 5, 5);
		return 0;
	}

	iwn_enable_tsf(sc, ni);

	/* update adapter's configuration */
	sc->config.associd = htole16(ni->ni_associd & ~0xc000);
	/* short preamble/slot time are negotiated when associating */
	sc->config.flags &= ~htole32(IWN_CONFIG_SHPREAMBLE |
	    IWN_CONFIG_SHSLOT);
	if (ic->ic_flags & IEEE80211_F_SHSLOT)
		sc->config.flags |= htole32(IWN_CONFIG_SHSLOT);
	if (ic->ic_flags & IEEE80211_F_SHPREAMBLE)
		sc->config.flags |= htole32(IWN_CONFIG_SHPREAMBLE);
	sc->config.filter |= htole32(IWN_FILTER_BSS);

	DPRINTF(("config chan %d flags %x\n", sc->config.chan,
		sc->config.flags));
	error = iwn_cmd(sc, IWN_CMD_CONFIGURE, &sc->config,
	    sizeof (struct iwn_config), 1);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
			"could not update configuration\n");
		return error;
	}

	/* configuration has changed, set Tx power accordingly */
	if ((error = iwn_set_txpower(sc, ni->ni_chan, 1)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set Tx power\n");
		return error;
	}

	/* add BSS node */
	iwn_add_node(sc, ni, false, true,
		     (3 << IWN_AMDPU_SIZE_FACTOR_SHIFT |
		      5 << IWN_AMDPU_DENSITY_SHIFT));

	if (ic->ic_opmode == IEEE80211_M_STA) {
		/* fake a join to init the tx rate */
		iwn_newassoc(ni, 1);
	}

	if ((error = iwn_init_sensitivity(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set sensitivity\n");
		return error;
	}

	/* start periodic calibration timer */
	sc->calib.state = IWN_CALIB_STATE_ASSOC;
	sc->calib_cnt = 0;
	callout_schedule(&sc->calib_to, hz / 2);

	if (0 == 1) { /* XXX don't do the beacon - we get a firmware error
			 XXX when we try. Something is wrong with the
			 XXX setup of the frame. Just don't ever call
			 XXX the function but reference it to keep gcc happy
		      */
		/* now we are associated set up the beacon frame */
		if ((error = iwn_setup_beacon(sc, ni))) {
			aprint_error_dev(sc->sc_dev,
					 "could not setup beacon frame\n");
			return error;
		}
	}


	/* link LED always on while associated */
	iwn_set_led(sc, IWN_LED_LINK, 0, 1);

	return 0;
}

/*
 * Send a scan request to the firmware. Since this command is huge, we map it
 * into a mbuf instead of using the pre-allocated set of commands. this function
 * implemented as iwl4965_bg_request_scan in the linux driver.
 */
static int
iwn_scan(struct iwn_softc *sc, uint16_t flags)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwn_tx_ring *ring = &sc->txq[4];
	struct iwn_tx_desc *desc;
	struct iwn_tx_data *data;
	struct iwn_tx_cmd *cmd;
	struct iwn_cmd_data *tx;
	struct iwn_scan_hdr *hdr;
	struct iwn_scan_chan *chan;
	struct ieee80211_frame *wh;
	struct ieee80211_rateset *rs;
	struct ieee80211_channel *c;
	enum ieee80211_phymode mode;
	uint8_t *frm;
	int pktlen, error, nrates;

	desc = &ring->desc[ring->cur];
	data = &ring->data[ring->cur];

	/*
	 * allocate an mbuf and initialize it so that it contains a packet
	 * header. M_DONTWAIT can fail and MT_DATA means it is dynamically
	 * allocated.
	 */
	MGETHDR(data->m, M_DONTWAIT, MT_DATA);
	if (data->m == NULL) {
		aprint_error_dev(sc->sc_dev, "could not allocate mbuf for scan command\n");
		return ENOMEM;
	}

	/*
	 * allocates and adds an mbuf cluster to a normal mbuf m. the how
	 * is M_DONTWAIT and the flag M_EXT is set upon success.
	 */
	MCLGET(data->m, M_DONTWAIT);
	if (!(data->m->m_flags & M_EXT)) {
		m_freem(data->m);
		data->m = NULL;
		aprint_error_dev(sc->sc_dev, "could not allocate mbuf for scan command\n");
		return ENOMEM;
	}

	/*
	 * returns a pointer to the data contained in the specified mbuf.
	 * in this case it is our iwn_tx_cmd. we initialize the basic
	 * members of the command here with exception to data[136].
	 */
	cmd = mtod(data->m, struct iwn_tx_cmd *);
	cmd->code = IWN_CMD_SCAN;
	cmd->flags = 0;
	cmd->qid = ring->qid;
	cmd->idx = ring->cur;

	hdr = (struct iwn_scan_hdr *)cmd->data;
	memset(hdr, 0, sizeof (struct iwn_scan_hdr));
	/*
	 * Move to the next channel if no packets are received within 5 msecs
	 * after sending the probe request (this helps to reduce the duration
	 * of active scans).
	 */
	hdr->quiet = htole16(5);	/* timeout in milliseconds */
	hdr->plcp_threshold = htole16(1);	/* min # of packets */

	/* select Ant B and Ant C for scanning */
	hdr->rxchain = htole16(0x3e1 | 7 << IWN_RXCHAIN_ANTMSK_SHIFT);

	tx = &(hdr->tx_cmd);
	/*
	 * linux
	 * flags = IWN_TX_AUTO_SEQ
	 * 	   0x200 is rate selection?
	 * id = ???
	 * lifetime = IWN_LIFETIME_INFINITE
	 *
	 */
	tx->flags = htole32(IWN_TX_AUTO_SEQ | 0x200); // XXX
	tx->id = IWN_ID_BROADCAST;
	tx->lifetime = htole32(IWN_LIFETIME_INFINITE);
	tx->rflags = IWN_RFLAG_ANT_B;

	if (flags & IEEE80211_CHAN_A) {
		hdr->crc_threshold = htole16(1);
		/* send probe requests at 6Mbps */
		tx->rate = iwn_ridx_to_plcp[IWN_OFDM6];
	} else {
		hdr->flags = htole32(IWN_CONFIG_24GHZ | IWN_CONFIG_AUTO);
		/* send probe requests at 1Mbps */
		tx->rate = iwn_ridx_to_plcp[IWN_CCK1];
		tx->rflags |= IWN_RFLAG_CCK;
	}

	hdr->scan_essid[0].id  = IEEE80211_ELEMID_SSID;
	hdr->scan_essid[0].len = ic->ic_des_esslen;
	memcpy(hdr->scan_essid[0].data, ic->ic_des_essid, ic->ic_des_esslen);

	/*
	 * Build a probe request frame.	 Most of the following code is a
	 * copy & paste of what is done in net80211.
	 */
	wh = &(hdr->wh);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT |
	    IEEE80211_FC0_SUBTYPE_PROBE_REQ;
	wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
	IEEE80211_ADDR_COPY(wh->i_addr1, etherbroadcastaddr);
	IEEE80211_ADDR_COPY(wh->i_addr2, ic->ic_myaddr);
	IEEE80211_ADDR_COPY(wh->i_addr3, etherbroadcastaddr);
	*(u_int16_t *)&wh->i_dur[0] = 0;	/* filled by h/w */
	*(u_int16_t *)&wh->i_seq[0] = 0;	/* filled by h/w */

	frm = &(hdr->data[0]);

	/* add empty SSID IE */
	*frm++ = IEEE80211_ELEMID_SSID;
	*frm++ = ic->ic_des_esslen;
	memcpy(frm, ic->ic_des_essid, ic->ic_des_esslen);
	frm += ic->ic_des_esslen;

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
	tx->len = htole16(frm - (uint8_t *)wh);

	chan = (struct iwn_scan_chan *)frm;
	for (c	= &ic->ic_channels[1];
	     c <= &ic->ic_channels[IEEE80211_CHAN_MAX]; c++) {
		if ((c->ic_flags & flags) != flags)
			continue;

		chan->chan = ieee80211_chan2ieee(ic, c);
		chan->flags = 0;
		if (!(c->ic_flags & IEEE80211_CHAN_PASSIVE)) {
			chan->flags |= IWN_CHAN_ACTIVE;
			if (ic->ic_des_esslen != 0)
				chan->flags |= IWN_CHAN_DIRECT;
		}
		chan->dsp_gain = 0x6e;
		if (IEEE80211_IS_CHAN_5GHZ(c)) {
			chan->rf_gain = 0x3b;
			chan->active  = htole16(10);
			chan->passive = htole16(110);
		} else {
			chan->rf_gain = 0x28;
			chan->active  = htole16(20);
			chan->passive = htole16(120);
		}
		hdr->nchan++;
		chan++;

		frm += sizeof (struct iwn_scan_chan);
	}

	hdr->len = htole16(frm - (uint8_t *)hdr);
	pktlen = frm - (uint8_t *)cmd;

	error = bus_dmamap_load(sc->sc_dmat, data->map, cmd, pktlen, NULL,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "could not map scan command\n");
		m_freem(data->m);
		data->m = NULL;
		return error;
	}

	IWN_SET_DESC_NSEGS(desc, 1);
	IWN_SET_DESC_SEG(desc, 0, data->map->dm_segs[0].ds_addr,
	    data->map->dm_segs[0].ds_len);
	sc->shared->len[ring->qid][ring->cur] = htole16(8);
	if (ring->cur < IWN_TX_WINDOW) {
		sc->shared->len[ring->qid][ring->cur + IWN_TX_RING_COUNT] =
		    htole16(8);
	}

	bus_dmamap_sync(sc->sc_dmat, data->map, 0,
	    data->map->dm_segs[0].ds_len, BUS_DMASYNC_PREWRITE);

	/* kick cmd ring */
	ring->cur = (ring->cur + 1) % IWN_TX_RING_COUNT;
	IWN_WRITE(sc, IWN_TX_WIDX, ring->qid << 8 | ring->cur);

	return 0;	/* will be notified async. of failure/success */
}

static int
iwn_config(struct iwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct iwn_power power;
	struct iwn_bluetooth bluetooth;
	int error;

	/* set power mode */
	memset(&power, 0, sizeof power);
	power.flags = htole16(IWN_POWER_CAM | 0x8);
	DPRINTF(("setting power mode\n"));
	error = iwn_cmd(sc, IWN_CMD_SET_POWER_MODE, &power, sizeof power, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not set power mode\n");
		return error;
	}

	/* configure bluetooth coexistence */
	memset(&bluetooth, 0, sizeof bluetooth);
	bluetooth.flags = 3;
	bluetooth.lead = 0xaa;
	bluetooth.kill = 1;
	DPRINTF(("configuring bluetooth coexistence\n"));
	error = iwn_cmd(sc, IWN_CMD_BLUETOOTH, &bluetooth, sizeof bluetooth,
	    0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not configure bluetooth coexistence\n");
		return error;
	}

	/* configure adapter */
	memset(&sc->config, 0, sizeof (struct iwn_config));
	IEEE80211_ADDR_COPY(ic->ic_myaddr, CLLADDR(ifp->if_sadl));
	IEEE80211_ADDR_COPY(sc->config.myaddr, ic->ic_myaddr);
	IEEE80211_ADDR_COPY(sc->config.wlap, ic->ic_myaddr);
	/* set default channel */
	sc->config.chan = htole16(ieee80211_chan2ieee(ic, ic->ic_ibss_chan));
	sc->config.flags = htole32(IWN_CONFIG_TSF);
	if (IEEE80211_IS_CHAN_2GHZ(ic->ic_ibss_chan)) {
		sc->config.flags |= htole32(IWN_CONFIG_AUTO |
		    IWN_CONFIG_24GHZ);
	}
	sc->config.filter = 0;
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		sc->config.mode = IWN_MODE_STA;
		sc->config.filter |= htole32(IWN_FILTER_MULTICAST);
		break;
	case IEEE80211_M_IBSS:
	case IEEE80211_M_AHDEMO:
		sc->config.mode = IWN_MODE_IBSS;
		break;
	case IEEE80211_M_HOSTAP:
		sc->config.mode = IWN_MODE_HOSTAP;
		break;
	case IEEE80211_M_MONITOR:
		sc->config.mode = IWN_MODE_MONITOR;
		sc->config.filter |= htole32(IWN_FILTER_MULTICAST |
		    IWN_FILTER_CTL | IWN_FILTER_PROMISC);
		break;
	}
	sc->config.cck_mask  = 0x0f;	/* not yet negotiated */
	sc->config.ofdm_mask = 0xff;	/* not yet negotiated */
	sc->config.ht_single_mask = 0xff;
	sc->config.ht_dual_mask = 0xff;
	sc->config.rxchain = htole16(0x2800 | 7 << IWN_RXCHAIN_ANTMSK_SHIFT);
	DPRINTF(("setting configuration\n"));
	error = iwn_cmd(sc, IWN_CMD_CONFIGURE, &sc->config,
	    sizeof (struct iwn_config), 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "configure command failed\n");
		return error;
	}

	/* configuration has changed, set Tx power accordingly */
	if ((error = iwn_set_txpower(sc, ic->ic_ibss_chan, 0)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set Tx power\n");
		return error;
	}

	/* add broadcast node */
	if ((error = iwn_add_node(sc, NULL, true, false, 0)) != 0)
		return error;

	if ((error = iwn_set_critical_temp(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not set critical temperature\n");
		return error;
	}

	return 0;
}

/*
 * Do post-alive initialization of the NIC (after firmware upload).
 */
static void
iwn_post_alive(struct iwn_softc *sc)
{
	uint32_t base;
	uint16_t offset;
	int qid;

	iwn_mem_lock(sc);

	/* clear SRAM */
	base = iwn_mem_read(sc, IWN_SRAM_BASE);
	for (offset = 0x380; offset < 0x520; offset += 4) {
		IWN_WRITE(sc, IWN_MEM_WADDR, base + offset);
		IWN_WRITE(sc, IWN_MEM_WDATA, 0);
	}

	/* shared area is aligned on a 1K boundary */
	iwn_mem_write(sc, IWN_SRAM_BASE, sc->shared_dma.paddr >> 10);
	iwn_mem_write(sc, IWN_SELECT_QCHAIN, 0);

	for (qid = 0; qid < IWN_NTXQUEUES; qid++) {
		iwn_mem_write(sc, IWN_QUEUE_RIDX(qid), 0);
		IWN_WRITE(sc, IWN_TX_WIDX, qid << 8 | 0);

		/* set sched. window size */
		IWN_WRITE(sc, IWN_MEM_WADDR, base + IWN_QUEUE_OFFSET(qid));
		IWN_WRITE(sc, IWN_MEM_WDATA, 64);
		/* set sched. frame limit */
		IWN_WRITE(sc, IWN_MEM_WADDR, base + IWN_QUEUE_OFFSET(qid) + 4);
		IWN_WRITE(sc, IWN_MEM_WDATA, 64 << 16);
	}

	/* enable interrupts for all 16 queues */
	iwn_mem_write(sc, IWN_QUEUE_INTR_MASK, 0xffff);

	/* identify active Tx rings (0-7) */
	iwn_mem_write(sc, IWN_TX_ACTIVE, 0xff);

	/* mark Tx rings (4 EDCA + cmd + 2 HCCA) as active */
	for (qid = 0; qid < 7; qid++) {
		iwn_mem_write(sc, IWN_TXQ_STATUS(qid),
		    IWN_TXQ_STATUS_ACTIVE | qid << 1);
	}

	iwn_mem_unlock(sc);
}

static void
iwn_stop_master(struct iwn_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = IWN_READ(sc, IWN_RESET);
	IWN_WRITE(sc, IWN_RESET, tmp | IWN_STOP_MASTER);

	tmp = IWN_READ(sc, IWN_GPIO_CTL);
	if ((tmp & IWN_GPIO_PWR_STATUS) == IWN_GPIO_PWR_SLEEP)
		return; /* already asleep */

	for (ntries = 0; ntries < 100; ntries++) {
		if (IWN_READ(sc, IWN_RESET) & IWN_MASTER_DISABLED)
			break;
		DELAY(10);
	}
	if (ntries == 100) {
		aprint_error_dev(sc->sc_dev, "timeout waiting for master\n");
	}
}

static int
iwn_reset(struct iwn_softc *sc)
{
	uint32_t tmp;
	int ntries;

	/* clear any pending interrupts */
	IWN_WRITE(sc, IWN_INTR, 0xffffffff);

	tmp = IWN_READ(sc, IWN_CHICKEN);
	IWN_WRITE(sc, IWN_CHICKEN, tmp | IWN_CHICKEN_DISLOS);

	tmp = IWN_READ(sc, IWN_GPIO_CTL);
	IWN_WRITE(sc, IWN_GPIO_CTL, tmp | IWN_GPIO_INIT);

	/* wait for clock stabilization */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (IWN_READ(sc, IWN_GPIO_CTL) & IWN_GPIO_CLOCK)
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		aprint_error_dev(sc->sc_dev, "timeout waiting for clock stabilization\n");
		return ETIMEDOUT;
	}
	return 0;
}

static void
iwn_hw_config(struct iwn_softc *sc)
{
	uint32_t tmp, hw;

	/* enable interrupts mitigation */
	IWN_WRITE(sc, IWN_INTR_MIT, 512 / 32);

	/* voodoo from the reference driver */
	tmp = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_CLASS_REG);
	tmp = PCI_REVISION(tmp);
	if ((tmp & 0x80) && (tmp & 0x7f) < 8) {
		/* enable "no snoop" field */
		tmp = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0xe8);
		tmp &= ~IWN_DIS_NOSNOOP;
		pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0xe8, tmp);
	}

	/* disable L1 entry to work around a hardware bug */
	tmp = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0xf0);
	tmp &= ~IWN_ENA_L1;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0xf0, tmp);

	hw = IWN_READ(sc, IWN_HWCONFIG);
	IWN_WRITE(sc, IWN_HWCONFIG, hw | 0x310);

	iwn_mem_lock(sc);
	tmp = iwn_mem_read(sc, IWN_MEM_POWER);
	iwn_mem_write(sc, IWN_MEM_POWER, tmp | IWN_POWER_RESET);
	DELAY(5);
	tmp = iwn_mem_read(sc, IWN_MEM_POWER);
	iwn_mem_write(sc, IWN_MEM_POWER, tmp & ~IWN_POWER_RESET);
	iwn_mem_unlock(sc);
}

static int
iwn_init(struct ifnet *ifp)
{
	struct iwn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t tmp;
	int error, qid;

	iwn_stop(ifp, 1);
	if ((error = iwn_reset(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not reset adapter\n");
		goto fail1;
	}

	iwn_mem_lock(sc);
	iwn_mem_read(sc, IWN_CLOCK_CTL);
	iwn_mem_write(sc, IWN_CLOCK_CTL, 0xa00);
	iwn_mem_read(sc, IWN_CLOCK_CTL);
	iwn_mem_unlock(sc);

	DELAY(20);

	iwn_mem_lock(sc);
	tmp = iwn_mem_read(sc, IWN_MEM_PCIDEV);
	iwn_mem_write(sc, IWN_MEM_PCIDEV, tmp | 0x800);
	iwn_mem_unlock(sc);

	iwn_mem_lock(sc);
	tmp = iwn_mem_read(sc, IWN_MEM_POWER);
	iwn_mem_write(sc, IWN_MEM_POWER, tmp & ~0x03000000);
	iwn_mem_unlock(sc);

	iwn_hw_config(sc);

	/* init Rx ring */
	iwn_mem_lock(sc);
	IWN_WRITE(sc, IWN_RX_CONFIG, 0);
	IWN_WRITE(sc, IWN_RX_WIDX, 0);
	/* Rx ring is aligned on a 256-byte boundary */
	IWN_WRITE(sc, IWN_RX_BASE, sc->rxq.desc_dma.paddr >> 8);
	/* shared area is aligned on a 16-byte boundary */
	IWN_WRITE(sc, IWN_RW_WIDX_PTR, (sc->shared_dma.paddr +
		offsetof(struct iwn_shared, closed_count)) >> 4);
	IWN_WRITE(sc, IWN_RX_CONFIG, 0x80601000);
	iwn_mem_unlock(sc);

	IWN_WRITE(sc, IWN_RX_WIDX, (IWN_RX_RING_COUNT - 1) & ~7);

	iwn_mem_lock(sc);
	iwn_mem_write(sc, IWN_TX_ACTIVE, 0);

	/* set physical address of "keep warm" page */
	IWN_WRITE(sc, IWN_KW_BASE, sc->kw_dma.paddr >> 4);

	/* init Tx rings */
	for (qid = 0; qid < IWN_NTXQUEUES; qid++) {
		struct iwn_tx_ring *txq = &sc->txq[qid];
		IWN_WRITE(sc, IWN_TX_BASE(qid), txq->desc_dma.paddr >> 8);
		IWN_WRITE(sc, IWN_TX_CONFIG(qid), 0x80000008);
	}
	iwn_mem_unlock(sc);

	/* clear "radio off" and "disable command" bits (reversed logic) */
	IWN_WRITE(sc, IWN_UCODE_CLR, IWN_RADIO_OFF);
	IWN_WRITE(sc, IWN_UCODE_CLR, IWN_DISABLE_CMD);

	/* clear any pending interrupts */
	IWN_WRITE(sc, IWN_INTR, 0xffffffff);
	/* enable interrupts */
	IWN_WRITE(sc, IWN_MASK, IWN_INTR_MASK);

	/* not sure why/if this is necessary... */
	IWN_WRITE(sc, IWN_UCODE_CLR, IWN_RADIO_OFF);
	IWN_WRITE(sc, IWN_UCODE_CLR, IWN_RADIO_OFF);

	/* check that the radio is not disabled by RF switch */
	if (!(IWN_READ(sc, IWN_GPIO_CTL) & IWN_GPIO_RF_ENABLED)) {
		aprint_error_dev(sc->sc_dev, "radio is disabled by hardware switch\n");
		sc->sc_radio = false;
		error = EBUSY;	/* XXX ;-) */
		goto fail1;
	}

	sc->sc_radio = true;

	if ((error = iwn_load_firmware(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not load firmware\n");
		goto fail1;
	}

	/* firmware has notified us that it is alive.. */
	iwn_post_alive(sc);	/* ..do post alive initialization */

	sc->rawtemp = sc->ucode_info.temp[3].chan20MHz;
	sc->temp = iwn_get_temperature(sc);
	DPRINTF(("temperature=%d\n", sc->temp));

	if ((error = iwn_config(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "could not configure device\n");
		goto fail1;
	}

	DPRINTF(("iwn_config end\n"));

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	if (ic->ic_opmode != IEEE80211_M_MONITOR) {
		if (ic->ic_roaming != IEEE80211_ROAMING_MANUAL)
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
	}
	else
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);

	DPRINTF(("iwn_init ok\n"));
	return 0;

fail1:
	DPRINTF(("iwn_init error\n"));
	iwn_stop(ifp, 1);
	return error;
}

static void
iwn_stop(struct ifnet *ifp, int disable)
{
	struct iwn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t tmp;
	int i;

	ifp->if_timer = sc->sc_tx_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	IWN_WRITE(sc, IWN_RESET, IWN_NEVO_RESET);

	/* disable interrupts */
	IWN_WRITE(sc, IWN_MASK, 0);
	IWN_WRITE(sc, IWN_INTR, 0xffffffff);
	IWN_WRITE(sc, IWN_INTR_STATUS, 0xffffffff);

	/* make sure we no longer hold the memory lock */
	iwn_mem_unlock(sc);

	/* reset all Tx rings */
	for (i = 0; i < IWN_NTXQUEUES; i++)
		iwn_reset_tx_ring(sc, &sc->txq[i]);

	/* reset Rx ring */
	iwn_reset_rx_ring(sc, &sc->rxq);

	iwn_mem_lock(sc);
	iwn_mem_write(sc, IWN_MEM_CLOCK2, 0x200);
	iwn_mem_unlock(sc);

	DELAY(5);

	iwn_stop_master(sc);
	tmp = IWN_READ(sc, IWN_RESET);
	IWN_WRITE(sc, IWN_RESET, tmp | IWN_SW_RESET);
}

static bool
iwn_resume(device_t dv PMF_FN_ARGS)
{
	struct iwn_softc *sc = device_private(dv);

	(void)iwn_reset(sc);

	return true;
}
