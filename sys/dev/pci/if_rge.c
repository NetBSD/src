/*	$NetBSD: if_rge.c,v 1.3.2.2 2020/01/17 21:47:31 ad Exp $	*/
/*	$OpenBSD: if_rge.c,v 1.2 2020/01/02 09:00:45 kevlo Exp $	*/

/*
 * Copyright (c) 2019 Kevin Lo <kevlo@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: if_rge.c,v 1.3.2.2 2020/01/17 21:47:31 ad Exp $");

/* #include "bpfilter.h" Sevan */
/* #include "vlan.h" Sevan */

#include <sys/types.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/callout.h>
#include <sys/workqueue.h>

#include <net/if.h>

#include <net/if_dl.h>
#include <net/if_ether.h>

#include <net/if_media.h>

#include <netinet/in.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_rgereg.h>

#ifdef __NetBSD__
#define letoh32 	htole32
#define nitems(x) 	__arraycount(x)
#define MBUF_LIST_INITIALIZER() 	{ NULL, NULL, 0 }
struct mbuf_list {
	struct mbuf 	*ml_head;
	struct mbuf 	*ml_tail;
	u_int 	ml_len;
};
#ifdef NET_MPSAFE
#define 	RGE_MPSAFE	1
#define 	CALLOUT_FLAGS	CALLOUT_MPSAFE
#else
#define 	CALLOUT_FLAGS	0
#endif
#endif

static int		rge_match(device_t, cfdata_t, void *);
static void		rge_attach(device_t, device_t, void *); 
int		rge_intr(void *);
int		rge_encap(struct rge_softc *, struct mbuf *, int);
int		rge_ioctl(struct ifnet *, u_long, void *);
void		rge_start(struct ifnet *);
void		rge_watchdog(struct ifnet *);
int		rge_init(struct ifnet *);
void		rge_stop(struct ifnet *);
int		rge_ifmedia_upd(struct ifnet *);
void		rge_ifmedia_sts(struct ifnet *, struct ifmediareq *);
int		rge_allocmem(struct rge_softc *);
int		rge_newbuf(struct rge_softc *, int);
void		rge_discard_rxbuf(struct rge_softc *, int);
int		rge_rx_list_init(struct rge_softc *);
void		rge_tx_list_init(struct rge_softc *);
int		rge_rxeof(struct rge_softc *);
int		rge_txeof(struct rge_softc *);
void		rge_reset(struct rge_softc *);
void		rge_iff(struct rge_softc *);
void		rge_set_phy_power(struct rge_softc *, int);
void		rge_phy_config(struct rge_softc *);
void		rge_set_macaddr(struct rge_softc *, const uint8_t *);
void		rge_get_macaddr(struct rge_softc *, uint8_t *);
void		rge_hw_init(struct rge_softc *);
void		rge_disable_phy_ocp_pwrsave(struct rge_softc *);
void		rge_patch_phy_mcu(struct rge_softc *, int);
void		rge_add_media_types(struct rge_softc *);
void		rge_config_imtype(struct rge_softc *, int);
void		rge_disable_sim_im(struct rge_softc *);
void		rge_setup_sim_im(struct rge_softc *);
void		rge_setup_intr(struct rge_softc *, int);
void		rge_exit_oob(struct rge_softc *);
void		rge_write_csi(struct rge_softc *, uint32_t, uint32_t);
uint32_t	rge_read_csi(struct rge_softc *, uint32_t);
void		rge_write_mac_ocp(struct rge_softc *, uint16_t, uint16_t);
uint16_t	rge_read_mac_ocp(struct rge_softc *, uint16_t);
void		rge_write_ephy(struct rge_softc *, uint16_t, uint16_t);
void		rge_write_phy(struct rge_softc *, uint16_t, uint16_t, uint16_t);
void		rge_write_phy_ocp(struct rge_softc *, uint16_t, uint16_t);
uint16_t	rge_read_phy_ocp(struct rge_softc *, uint16_t);
int		rge_get_link_status(struct rge_softc *);
void		rge_txstart(struct work *, void *);
void		rge_tick(void *);
void		rge_link_state(struct rge_softc *);

static const struct {
	uint16_t reg;
	uint16_t val;
}  rtl8125_def_bps[] = {
	RTL8125_DEF_BPS
}, rtl8125_mac_cfg2_ephy[] = {
	RTL8125_MAC_CFG2_EPHY
}, rtl8125_mac_cfg2_mcu[] = {
	RTL8125_MAC_CFG2_MCU
}, rtl8125_mac_cfg3_ephy[] = {
	RTL8125_MAC_CFG3_EPHY
}, rtl8125_mac_cfg3_mcu[] = {
	RTL8125_MAC_CFG3_MCU
};

CFATTACH_DECL_NEW(rge, sizeof(struct rge_softc), rge_match, rge_attach,
		NULL, NULL); /* Sevan - detach function? */

extern struct cfdriver rge_cd;

static const struct {
	pci_vendor_id_t 	vendor;
	pci_product_id_t 	product;
}rge_devices[] = {
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_E3000 },
	{ PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8125 },
};

static int
rge_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa =aux;
	int n;

	for (n =0; n < __arraycount(rge_devices); n++) {
		if (PCI_VENDOR(pa->pa_id) == rge_devices[n].vendor &&
		    PCI_PRODUCT(pa->pa_id) == rge_devices[n].product)
			return 1;
	}

	return 0;
}

void
rge_attach(device_t parent, device_t self, void *aux)
{
	struct rge_softc *sc = (struct rge_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr = NULL;
	struct ifnet *ifp;
	pcireg_t reg;
	uint32_t hwrev;
	uint8_t eaddr[ETHER_ADDR_LEN];
	int offset;

	pci_set_powerstate(pa->pa_pc, pa->pa_tag, PCI_PMCSR_STATE_D0);

	/* 
	 * Map control/status registers.
	 */
	if (pci_mapreg_map(pa, RGE_PCI_BAR2, PCI_MAPREG_TYPE_MEM |
	    PCI_MAPREG_MEM_TYPE_64BIT, 0, &sc->rge_btag, &sc->rge_bhandle,
	    NULL, &sc->rge_bsize)) {
		if (pci_mapreg_map(pa, RGE_PCI_BAR1, PCI_MAPREG_TYPE_MEM |
		    PCI_MAPREG_MEM_TYPE_32BIT, 0, &sc->rge_btag,
		    &sc->rge_bhandle, NULL, &sc->rge_bsize)) {
			if (pci_mapreg_map(pa, RGE_PCI_BAR0, PCI_MAPREG_TYPE_IO,
			    0, &sc->rge_btag, &sc->rge_bhandle, NULL,
			    &sc->rge_bsize)) {
				printf(": can't map mem or i/o space\n");
				return;
			}
		}
	}

	/* 
	 * Allocate interrupt.
	 */
	if (pci_intr_map(pa, &ih) == 0)
		sc->rge_flags |= RGE_FLAG_MSI;
	else if (pci_intr_map(pa, &ih) != 0) {
		printf(": couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih, intrbuf, sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(pc, ih, IPL_NET, rge_intr,
	    sc, sc->sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf(": %s", intrstr);

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	/* Determine hardware revision */
	hwrev = RGE_READ_4(sc, RGE_TXCFG) & RGE_TXCFG_HWREV;
	switch (hwrev) {
	case 0x60800000:
		sc->rge_type = MAC_CFG2;
		break;
	case 0x60900000:
		sc->rge_type = MAC_CFG3;
		break;
	default:
		printf(": unknown version 0x%08x\n", hwrev);
		return;
	}

	rge_config_imtype(sc, RGE_IMTYPE_SIM);

	/* 
	 * PCI Express check.
	 */
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_PCIEXPRESS,
	    &offset, NULL)) {
		/* Disable PCIe ASPM. */
		reg = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    offset + PCIE_LCSR);
		reg &= ~(PCIE_LCSR_ASPM_L0S | PCIE_LCSR_ASPM_L1 );
		pci_conf_write(pa->pa_pc, pa->pa_tag, offset + PCIE_LCSR,
		    reg);
	}

	rge_exit_oob(sc);
	rge_hw_init(sc);

	rge_get_macaddr(sc, eaddr);
	printf(", address %s\n", ether_sprintf(eaddr));

	memcpy(sc->sc_enaddr, eaddr, ETHER_ADDR_LEN);

	rge_set_phy_power(sc, 1);
	rge_phy_config(sc);

	if (rge_allocmem(sc))
		return;

	ifp = &sc->sc_ec.ec_if;
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef RGE_MPSAFE
	ifp->if_xflags = IFEF_MPSAFE;
#endif
	ifp->if_ioctl = rge_ioctl;
	ifp->if_start = rge_start;
	ifp->if_watchdog = rge_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, RGE_TX_LIST_CNT);
	ifp->if_mtu = RGE_JUMBO_MTU;

	ifp->if_capabilities = ETHERCAP_VLAN_MTU | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_IPv4_Tx |IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx|
	    IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx;

#if NVLAN > 0
	ifp->if_capabilities |= IFCAP_VLAN_HWTAGGING;
#endif

	callout_init(&sc->sc_timeout, CALLOUT_FLAGS);
	callout_setfunc(&sc->sc_timeout, rge_tick, sc);
	rge_txstart(&sc->sc_task, sc);

	/* Initialize ifmedia structures. */
	ifmedia_init(&sc->sc_media, IFM_IMASK, rge_ifmedia_upd,
	    rge_ifmedia_sts);
	rge_add_media_types(sc);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);
	sc->sc_media.ifm_media = sc->sc_media.ifm_cur->ifm_media;

	if_attach(ifp);
	ether_ifattach(ifp, eaddr);
}

int
rge_intr(void *arg)
{
	struct rge_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t status;
	int claimed = 0, rx, tx;

	if (!(ifp->if_flags & IFF_RUNNING))
		return (0);

	/* Disable interrupts. */
	RGE_WRITE_4(sc, RGE_IMR, 0);

	status = RGE_READ_4(sc, RGE_ISR);
	if (!(sc->rge_flags & RGE_FLAG_MSI)) {
		if ((status & RGE_INTRS) == 0 || status == 0xffffffff)
			return (0);
	}
	if (status)
		RGE_WRITE_4(sc, RGE_ISR, status);

	if (status & RGE_ISR_PCS_TIMEOUT)
		claimed = 1;

	rx = tx = 0;
	if (status & RGE_INTRS) {
		if (status &
		    (sc->rge_rx_ack | RGE_ISR_RX_ERR | RGE_ISR_RX_FIFO_OFLOW)) {
			rx |= rge_rxeof(sc);
			claimed = 1;
		}

		if (status & (sc->rge_tx_ack | RGE_ISR_TX_ERR)) {
			tx |= rge_txeof(sc);
			claimed = 1;
		}

		if (status & RGE_ISR_SYSTEM_ERR) {
			KERNEL_LOCK(1, NULL);
			rge_init(ifp);
			KERNEL_UNLOCK_ONE(NULL);
			claimed = 1;
		}
	}

	if (sc->rge_timerintr) {
		if ((tx | rx) == 0) {
			/*
			 * Nothing needs to be processed, fallback
			 * to use TX/RX interrupts.
			 */
			rge_setup_intr(sc, RGE_IMTYPE_NONE);

			/*
			 * Recollect, mainly to avoid the possible
			 * race introduced by changing interrupt
			 * masks.
			 */
			rge_rxeof(sc);
			rge_txeof(sc);
		} else
			RGE_WRITE_4(sc, RGE_TIMERCNT, 1);
	} else if (tx | rx) {
		/*
		 * Assume that using simulated interrupt moderation
		 * (hardware timer based) could reduce the interrupt
		 * rate.
		 */
		rge_setup_intr(sc, RGE_IMTYPE_SIM);
	}

	RGE_WRITE_4(sc, RGE_IMR, sc->rge_intrs);

	return (claimed);
}

int
rge_encap(struct rge_softc *sc, struct mbuf *m, int idx)
{
	struct rge_tx_desc *d = NULL;
	struct rge_txq *txq;
	bus_dmamap_t txmap;
	uint32_t cmdsts, cflags = 0;
	int cur, error, i, last, nsegs;

	/*
	 * Set RGE_TDEXTSTS_IPCSUM if any checksum offloading is requested.
	 * Otherwise, RGE_TDEXTSTS_TCPCSUM / RGE_TDEXTSTS_UDPCSUM does not
	 * take affect.
	 */
	if ((m->m_pkthdr.csum_flags &
	    (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0) {
		cflags |= RGE_TDEXTSTS_IPCSUM;
		if (m->m_pkthdr.csum_flags & M_TCP_CSUM_OUT)
			cflags |= RGE_TDEXTSTS_TCPCSUM;
		if (m->m_pkthdr.csum_flags & M_UDP_CSUM_OUT)
			cflags |= RGE_TDEXTSTS_UDPCSUM;
	}

	txq = &sc->rge_ldata.rge_txq[idx];
	txmap = txq->txq_dmamap;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, txmap, m, BUS_DMA_NOWAIT);
	switch (error) {
	case 0:
		break;
	case EFBIG: /* mbuf chain is too fragmented */
		if (m_defrag(m, M_DONTWAIT) == 0 &&
		    bus_dmamap_load_mbuf(sc->sc_dmat, txmap, m,
		    BUS_DMA_NOWAIT) == 0)
			break;

		/* FALLTHROUGH */
	default:
		return (0);
	}

	bus_dmamap_sync(sc->sc_dmat, txmap, 0, txmap->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	nsegs = txmap->dm_nsegs;

	/* Set up hardware VLAN tagging. */
#if NVLAN > 0
	if (m->m_flags & M_VLANTAG)
		cflags |= swap16(m->m_pkthdr.ether_vtag | RGE_TDEXTSTS_VTAG);
#endif

	cur = idx;
	cmdsts = RGE_TDCMDSTS_SOF;

	for (i = 0; i < txmap->dm_nsegs; i++) {
		d = &sc->rge_ldata.rge_tx_list[cur];

		d->rge_extsts = htole32(cflags);
		d->rge_addrlo = htole32(RGE_ADDR_LO(txmap->dm_segs[i].ds_addr));
		d->rge_addrhi = htole32(RGE_ADDR_HI(txmap->dm_segs[i].ds_addr));

		cmdsts |= txmap->dm_segs[i].ds_len;

		if (cur == RGE_TX_LIST_CNT - 1)
			cmdsts |= RGE_TDCMDSTS_EOR;

		d->rge_cmdsts = htole32(cmdsts);

		last = cur;
		cmdsts = RGE_TDCMDSTS_OWN;
		cur = RGE_NEXT_TX_DESC(cur);
	}

	/* Set EOF on the last descriptor. */
	d->rge_cmdsts |= htole32(RGE_TDCMDSTS_EOF);

	/* Transfer ownership of packet to the chip. */
	d = &sc->rge_ldata.rge_tx_list[idx];

	d->rge_cmdsts |= htole32(RGE_TDCMDSTS_OWN);

	bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map,
	    cur * sizeof(struct rge_tx_desc), sizeof(struct rge_tx_desc),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/* Update info of TX queue and descriptors. */
	txq->txq_mbuf = m;
	txq->txq_descidx = last;

	return (nsegs);
}

int
rge_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct rge_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if (!(ifp->if_flags & IFF_RUNNING))
			rge_init(ifp);
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				error = ENETRESET;
			else
				rge_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				rge_stop(ifp);
		}
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ifp->if_mtu) {
			error = EINVAL;
			break;
		}
		ifp->if_mtu = ifr->ifr_mtu;
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			rge_iff(sc);
		error = 0;
	}

	splx(s);
	return (error);
}

void
rge_start(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int free, idx, used;
	int queued = 0;

#define LINK_STATE_IS_UP(_s)    \
	((_s) >= LINK_STATE_UP || (_s) == LINK_STATE_UNKNOWN)

	if (!LINK_STATE_IS_UP(ifp->if_link_state)) {
		ifq_purge(ifq);
		return;
	}

	/* Calculate free space. */
	idx = sc->rge_ldata.rge_txq_prodidx;
	free = sc->rge_ldata.rge_txq_considx;
	if (free <= idx)
		free += RGE_TX_LIST_CNT;
	free -= idx;

	for (;;) {
		if (RGE_TX_NSEGS >= free + 2) {
			SET(ifp->if_flags, IFF_OACTIVE);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		used = rge_encap(sc, m, idx);
		if (used == 0) {
			m_freem(m);
			continue;
		}

		KASSERT(used <= free);
		free -= used;

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap_ether(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

		idx += used;
		if (idx >= RGE_TX_LIST_CNT)
			idx -= RGE_TX_LIST_CNT;

		queued++;
	}

	if (queued == 0)
		return;

	/* Set a timeout in case the chip goes out to lunch. */
	ifp->if_timer = 5;

	sc->rge_ldata.rge_txq_prodidx = idx;
	ifq_serialize(ifq, &sc->sc_task);
}

void
rge_watchdog(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;

	printf("%s: watchdog timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;

	rge_init(ifp);
}

int
rge_init(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	uint32_t val;
	uint16_t max_frame_size;
	int i;

	rge_stop(ifp);

	/* Set MAC address. */
	rge_set_macaddr(sc, sc->sc_enaddr);

	/* Set Maximum frame size but don't let MTU be lass than ETHER_MTU. */
	if (ifp->if_mtu < ETHERMTU)
		max_frame_size = ETHERMTU;
	else
		max_frame_size = ifp->if_mtu;

	max_frame_size += ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN +
	    ETHER_CRC_LEN + 1;

	if (max_frame_size > RGE_JUMBO_FRAMELEN)
		max_frame_size -= 1; 

	RGE_WRITE_2(sc, RGE_RXMAXSIZE, max_frame_size);

	/* Initialize RX descriptors list. */
	if (rge_rx_list_init(sc) == ENOBUFS) {
		printf("%s: init failed: no memory for RX buffers\n",
		    sc->sc_dev.dv_xname);
		rge_stop(ifp);
		return (ENOBUFS);
	}

	/* Initialize TX descriptors. */
	rge_tx_list_init(sc);

	/* Load the addresses of the RX and TX lists into the chip. */
	RGE_WRITE_4(sc, RGE_RXDESC_ADDR_LO,
	    RGE_ADDR_LO(sc->rge_ldata.rge_rx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_RXDESC_ADDR_HI,
	    RGE_ADDR_HI(sc->rge_ldata.rge_rx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_TXDESC_ADDR_LO,
	    RGE_ADDR_LO(sc->rge_ldata.rge_tx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_TXDESC_ADDR_HI,
	    RGE_ADDR_HI(sc->rge_ldata.rge_tx_list_map->dm_segs[0].ds_addr));

	RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);

	RGE_CLRBIT_1(sc, 0xf1, 0x80);
	RGE_CLRBIT_1(sc, RGE_CFG2, RGE_CFG2_CLKREQ_EN);
	RGE_CLRBIT_1(sc, RGE_CFG5, RGE_CFG5_PME_STS);
	RGE_CLRBIT_1(sc, RGE_CFG3, RGE_CFG3_RDY_TO_L23);

	/* Clear interrupt moderation timer. */
	for (i = 0; i < 64; i++)
		RGE_WRITE_4(sc, RGE_IM(i), 0);

	/* Set the initial RX and TX configurations. */
	RGE_WRITE_4(sc, RGE_RXCFG, RGE_RXCFG_CONFIG);
	RGE_WRITE_4(sc, RGE_TXCFG, RGE_TXCFG_CONFIG);

	val = rge_read_csi(sc, 0x70c) & ~0xff000000;
	rge_write_csi(sc, 0x70c, val | 0x27000000);

	/* Enable hardware optimization function. */
	val = pci_conf_read(sc->sc_pc, sc->sc_tag, 0x78) & ~0x00007000;
	pci_conf_write(sc->sc_pc, sc->sc_tag, 0x78, val | 0x00005000);

	RGE_WRITE_2(sc, 0x0382, 0x221b);
	RGE_WRITE_1(sc, 0x4500, 0);
	RGE_WRITE_2(sc, 0x4800, 0);
	RGE_CLRBIT_1(sc, RGE_CFG1, RGE_CFG1_SPEED_DOWN);

	rge_write_mac_ocp(sc, 0xc140, 0xffff);
	rge_write_mac_ocp(sc, 0xc142, 0xffff);

	val = rge_read_mac_ocp(sc, 0xd3e2) & ~0x0fff;
	rge_write_mac_ocp(sc, 0xd3e2, val | 0x03a9);

	RGE_MAC_CLRBIT(sc, 0xd3e4, 0x00ff);
	RGE_MAC_SETBIT(sc, 0xe860, 0x0080);
	RGE_MAC_SETBIT(sc, 0xeb58, 0x0001);

	val = rge_read_mac_ocp(sc, 0xe614) & ~0x0700;
	rge_write_mac_ocp(sc, 0xe614, val | 0x0400);

	RGE_MAC_CLRBIT(sc, 0xe63e, 0x0c00);

	val = rge_read_mac_ocp(sc, 0xe63e) & ~0x0030;
	rge_write_mac_ocp(sc, 0xe63e, val | 0x0020);

	RGE_MAC_SETBIT(sc, 0xc0b4, 0x000c);

	val = rge_read_mac_ocp(sc, 0xeb6a) & ~0x007f;
	rge_write_mac_ocp(sc, 0xeb6a, val | 0x0033);

	val = rge_read_mac_ocp(sc, 0xeb50) & ~0x03e0;
	rge_write_mac_ocp(sc, 0xeb50, val | 0x0040);

	val = rge_read_mac_ocp(sc, 0xe056) & ~0x00f0;
	rge_write_mac_ocp(sc, 0xe056, val | 0x0030);

	RGE_WRITE_1(sc, RGE_TDFNR, 0x10);

	RGE_MAC_CLRBIT(sc, 0xe040, 0x1000);

	val = rge_read_mac_ocp(sc, 0xe0c0) & ~0x4f0f;
	rge_write_mac_ocp(sc, 0xe0c0, val | 0x4403);

	RGE_MAC_SETBIT(sc, 0xe052, 0x0068);
	RGE_MAC_CLRBIT(sc, 0xe052, 0x0080);

	val = rge_read_mac_ocp(sc, 0xc0ac) & ~0x0080;
	rge_write_mac_ocp(sc, 0xc0ac, val | 0x1f00);

	val = rge_read_mac_ocp(sc, 0xd430) & ~0x0fff;
	rge_write_mac_ocp(sc, 0xd430, val | 0x047f);

	RGE_MAC_SETBIT(sc, 0xe84c, 0x00c0); 

	/* Disable EEE plus. */
	RGE_MAC_CLRBIT(sc, 0xe080, 0x0002);

	RGE_MAC_CLRBIT(sc, 0xea1c, 0x0004);

	RGE_MAC_SETBIT(sc, 0xeb54, 0x0001);
	DELAY(1);
	RGE_MAC_CLRBIT(sc, 0xeb54, 0x0001);

	RGE_CLRBIT_4(sc, 0x1880, 0x0030);

	rge_write_mac_ocp(sc, 0xe098, 0xc302);

	if (ifp->if_capabilities & ETHERCAP_VLAN_HWTAGGING)
		RGE_SETBIT_4(sc, RGE_RXCFG, RGE_RXCFG_VLANSTRIP);

	RGE_SETBIT_2(sc, RGE_CPLUSCMD, RGE_CPLUSCMD_RXCSUM);

	for (i = 0; i < 10; i++) {
		if (!(rge_read_mac_ocp(sc, 0xe00e) & 0x2000))
			break;
		DELAY(1000);
	}

	/* Disable RXDV gate. */
	RGE_CLRBIT_1(sc, RGE_PPSW, 0x08);
	DELAY(2000);

	rge_ifmedia_upd(ifp);

	/* Enable transmit and receive. */
	RGE_WRITE_1(sc, RGE_CMD, RGE_CMD_TXENB | RGE_CMD_RXENB);

	/* Program promiscuous mode and multicast filters. */
	rge_iff(sc);

	RGE_CLRBIT_1(sc, RGE_CFG2, RGE_CFG2_CLKREQ_EN);
	RGE_CLRBIT_1(sc, RGE_CFG5, RGE_CFG5_PME_STS);

	RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);

	/* Enable interrupts. */
	rge_setup_intr(sc, RGE_IMTYPE_SIM);

	ifp->if_flags |= IFF_RUNNING;
	CLR(ifp->if_flags, IFF_OACTIVE);

	callout_schedule(&sc->sc_timeout, 1);

	return (0);
}

/*
 * Stop the adapter and free any mbufs allocated to the RX and TX lists.
 */
void
rge_stop(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	int i;

	timeout_del(&sc->sc_timeout);

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_RUNNING;
	sc->rge_timerintr = 0;

	RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_ALLPHYS | RGE_RXCFG_INDIV |
	    RGE_RXCFG_MULTI | RGE_RXCFG_BROAD | RGE_RXCFG_RUNT |
	    RGE_RXCFG_ERRPKT);

	RGE_WRITE_4(sc, RGE_IMR, 0);
	RGE_WRITE_4(sc, RGE_ISR, 0xffffffff);

	rge_reset(sc);

	intr_barrier(sc->sc_ih);
	ifq_barrier(&ifp->if_snd);
/*	ifq_clr_oactive(&ifp->if_snd); Sevan - OpenBSD queue API */

	if (sc->rge_head != NULL) {
		m_freem(sc->rge_head);
		sc->rge_head = sc->rge_tail = NULL;
	}

	/* Free the TX list buffers. */
	for (i = 0; i < RGE_TX_LIST_CNT; i++) {
		if (sc->rge_ldata.rge_txq[i].txq_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    sc->rge_ldata.rge_txq[i].txq_dmamap);
			m_freem(sc->rge_ldata.rge_txq[i].txq_mbuf);
			sc->rge_ldata.rge_txq[i].txq_mbuf = NULL;
		}
	}

	/* Free the RX list buffers. */
	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		if (sc->rge_ldata.rge_rxq[i].rxq_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    sc->rge_ldata.rge_rxq[i].rxq_dmamap);
			m_freem(sc->rge_ldata.rge_rxq[i].rxq_mbuf);
			sc->rge_ldata.rge_rxq[i].rxq_mbuf = NULL;
		}
	}
}

/*
 * Set media options.
 */
int
rge_ifmedia_upd(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;
	int anar, gig, val;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	/* Disable Gigabit Lite. */
	RGE_PHY_CLRBIT(sc, 0xa428, 0x0200);
	RGE_PHY_CLRBIT(sc, 0xa5ea, 0x0001);

	val = rge_read_phy_ocp(sc, 0xa5d4);
	val &= ~RGE_ADV_2500TFDX;

	anar = gig = 0;
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		anar |= ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig |= GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		val |= RGE_ADV_2500TFDX;
		break;
	case IFM_2500_T:
		anar |= ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig |= GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		val |= RGE_ADV_2500TFDX;
		ifp->if_baudrate = IF_Mbps(2500);
		break;
	case IFM_1000_T:
		anar |= ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig |= GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		ifp->if_baudrate = IF_Gbps(1);
		break;
	case IFM_100_TX:
		anar |= ANAR_TX | ANAR_TX_FD;
		ifp->if_baudrate = IF_Mbps(100);
		break;
	case IFM_10_T:
		anar |= ANAR_10 | ANAR_10_FD;
		ifp->if_baudrate = IF_Mbps(10);
		break;
	default:
		printf("%s: unsupported media type\n", sc->sc_dev.dv_xname);
		return (EINVAL);
	}

	rge_write_phy(sc, 0, MII_ANAR, anar | ANAR_PAUSE_ASYM | ANAR_FC);
	rge_write_phy(sc, 0, MII_100T2CR, gig);
	rge_write_phy_ocp(sc, 0xa5d4, val);
	rge_write_phy(sc, 0, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);

	return (0);
}

/*
 * Report current media status.
 */
void
rge_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct rge_softc *sc = ifp->if_softc;
	uint16_t status = 0;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (rge_get_link_status(sc)) {
		ifmr->ifm_status |= IFM_ACTIVE;

		status = RGE_READ_2(sc, RGE_PHYSTAT);
		if ((status & RGE_PHYSTAT_FDX) ||
		    (status & RGE_PHYSTAT_2500MBPS))
			ifmr->ifm_active |= IFM_FDX;
		else
			ifmr->ifm_active |= IFM_HDX;

		if (status & RGE_PHYSTAT_10MBPS)
			ifmr->ifm_active |= IFM_10_T;
		else if (status & RGE_PHYSTAT_100MBPS)
			ifmr->ifm_active |= IFM_100_TX;
		else if (status & RGE_PHYSTAT_1000MBPS)
			ifmr->ifm_active |= IFM_1000_T;
		else if (status & RGE_PHYSTAT_2500MBPS)
			ifmr->ifm_active |= IFM_2500_T;
	}
}

/* 
 * Allocate memory for RX/TX rings.
 */
int
rge_allocmem(struct rge_softc *sc)
{
	int error, i;

	/* Allocate DMA'able memory for the TX ring. */
	error = bus_dmamap_create(sc->sc_dmat, RGE_TX_LIST_SZ, 1,
	    RGE_TX_LIST_SZ, 0, BUS_DMA_NOWAIT, &sc->rge_ldata.rge_tx_list_map);
	if (error) {
		printf("%s: can't create TX list map\n", sc->sc_dev.dv_xname);
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_TX_LIST_SZ, RGE_ALIGN, 0,
	    &sc->rge_ldata.rge_tx_listseg, 1, &sc->rge_ldata.rge_tx_listnseg,
	    BUS_DMA_NOWAIT); /* XXX OpenBSD adds BUS_DMA_ZERO */
	if (error) {
		printf("%s: can't alloc TX list\n", sc->sc_dev.dv_xname);
		return (error);
	}

	/* Load the map for the TX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &sc->rge_ldata.rge_tx_listseg,
	    sc->rge_ldata.rge_tx_listnseg, RGE_TX_LIST_SZ,
	    &sc->rge_ldata.rge_tx_list,
	    BUS_DMA_NOWAIT); /* XXX OpenBSD adds BUS_DMA_COHERENT */
	if (error) {
		printf("%s: can't map TX dma buffers\n", sc->sc_dev.dv_xname);
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_tx_listseg,
		    sc->rge_ldata.rge_tx_listnseg);
		return (error);
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map,
	    sc->rge_ldata.rge_tx_list, RGE_TX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load TX dma map\n", sc->sc_dev.dv_xname);
		bus_dmamap_destroy(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map);
		bus_dmamem_unmap(sc->sc_dmat,
		    sc->rge_ldata.rge_tx_list, RGE_TX_LIST_SZ);
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_tx_listseg,
		    sc->rge_ldata.rge_tx_listnseg);
		return (error);
	}

	/* Create DMA maps for TX buffers. */
	for (i = 0; i < RGE_TX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, RGE_JUMBO_FRAMELEN,
		    RGE_TX_NSEGS, RGE_JUMBO_FRAMELEN, 0, 0,
		    &sc->rge_ldata.rge_txq[i].txq_dmamap);
		if (error) {
			printf("%s: can't create DMA map for TX\n",
			    sc->sc_dev.dv_xname);
			return (error);
		}
	}

	/* Allocate DMA'able memory for the RX ring. */
	error = bus_dmamap_create(sc->sc_dmat, RGE_RX_LIST_SZ, 1,
	    RGE_RX_LIST_SZ, 0, 0, &sc->rge_ldata.rge_rx_list_map);
	if (error) {
		printf("%s: can't create RX list map\n", sc->sc_dev.dv_xname);
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_RX_LIST_SZ, RGE_ALIGN, 0,
	    &sc->rge_ldata.rge_rx_listseg, 1, &sc->rge_ldata.rge_rx_listnseg,
	    BUS_DMA_NOWAIT);  /* XXX OpenBSD adds BUS_DMA_ZERO */
	if (error) {
		printf("%s: can't alloc RX list\n", sc->sc_dev.dv_xname);
		return (error);
	}

	/* Load the map for the RX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &sc->rge_ldata.rge_rx_listseg,
	    sc->rge_ldata.rge_rx_listnseg, RGE_RX_LIST_SZ,
	    &sc->rge_ldata.rge_rx_list,
	    BUS_DMA_NOWAIT);  /* XXX OpenBSD adds BUS_DMA_COHERENT */
	if (error) {
		printf("%s: can't map RX dma buffers\n", sc->sc_dev.dv_xname);
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_rx_listseg,
		    sc->rge_ldata.rge_rx_listnseg);
		return (error);
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map,
	    sc->rge_ldata.rge_rx_list, RGE_RX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load RX dma map\n", sc->sc_dev.dv_xname);
		bus_dmamap_destroy(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map);
		bus_dmamem_unmap(sc->sc_dmat,
		    sc->rge_ldata.rge_rx_list, RGE_RX_LIST_SZ);
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_rx_listseg,
		    sc->rge_ldata.rge_rx_listnseg);
		return (error);
	}

	/* Create DMA maps for RX buffers. */
	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, RGE_JUMBO_FRAMELEN, 1,
		    RGE_JUMBO_FRAMELEN, 0, 0,
		    &sc->rge_ldata.rge_rxq[i].rxq_dmamap);
		if (error) {
			printf("%s: can't create DMA map for RX\n",
			    sc->sc_dev.dv_xname);
			return (error);
		}
	}

	return (error);
}

/*
 * Initialize the RX descriptor and attach an mbuf cluster.
 */
int
rge_newbuf(struct rge_softc *sc, int idx)
{
	struct mbuf *m;
	struct rge_rx_desc *r;
	struct rge_rxq *rxq;
	bus_dmamap_t rxmap;

	m = MCLGETI(NULL, M_DONTWAIT, NULL, RGE_JUMBO_FRAMELEN);
	if (m == NULL)
		return (ENOBUFS);

	m->m_len = m->m_pkthdr.len = RGE_JUMBO_FRAMELEN;

	rxq = &sc->rge_ldata.rge_rxq[idx];
	rxmap = rxq->rxq_dmamap;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, rxmap, m, BUS_DMA_NOWAIT))
		goto out;

	bus_dmamap_sync(sc->sc_dmat, rxmap, 0, rxmap->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	/* Map the segments into RX descriptors. */
	r = &sc->rge_ldata.rge_rx_list[idx];

	if (RGE_OWN(r)) {
		printf("%s: tried to map busy RX descriptor\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}

	rxq->rxq_mbuf = m;

	r->rge_extsts = 0;
	r->rge_addrlo = htole32(RGE_ADDR_LO(rxmap->dm_segs[0].ds_addr));
	r->rge_addrhi = htole32(RGE_ADDR_HI(rxmap->dm_segs[0].ds_addr));

	r->rge_cmdsts = htole32(rxmap->dm_segs[0].ds_len);
	if (idx == RGE_RX_LIST_CNT - 1)
		r->rge_cmdsts |= htole32(RGE_RDCMDSTS_EOR);

	r->rge_cmdsts |= htole32(RGE_RDCMDSTS_OWN);

	bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map,
	    idx * sizeof(struct rge_rx_desc), sizeof(struct rge_rx_desc),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return (0);
out:
	if (m != NULL)
		m_freem(m);
	return (ENOMEM);
}

void
rge_discard_rxbuf(struct rge_softc *sc, int idx)
{
	struct rge_rx_desc *r;

	r = &sc->rge_ldata.rge_rx_list[idx];

	r->rge_cmdsts = htole32(RGE_JUMBO_FRAMELEN);
	r->rge_extsts = 0;
	if (idx == RGE_RX_LIST_CNT - 1)
		r->rge_cmdsts |= htole32(RGE_RDCMDSTS_EOR);
	r->rge_cmdsts |= htole32(RGE_RDCMDSTS_OWN);

	bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map,
	    idx * sizeof(struct rge_rx_desc), sizeof(struct rge_rx_desc),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

int
rge_rx_list_init(struct rge_softc *sc)
{
	int i;

	memset(sc->rge_ldata.rge_rx_list, 0, RGE_RX_LIST_SZ);

	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		sc->rge_ldata.rge_rxq[i].rxq_mbuf = NULL;
		if (rge_newbuf(sc, i) == ENOBUFS)
			return (ENOBUFS);
	}

	sc->rge_ldata.rge_rxq_prodidx = 0;
	sc->rge_head = sc->rge_tail = NULL;

	return (0);
}

void
rge_tx_list_init(struct rge_softc *sc)
{
	int i;

	memset(sc->rge_ldata.rge_tx_list, 0, RGE_TX_LIST_SZ);

	for (i = 0; i < RGE_TX_LIST_CNT; i++)
		sc->rge_ldata.rge_txq[i].txq_mbuf = NULL;

	bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map, 0,
	    sc->rge_ldata.rge_tx_list_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	sc->rge_ldata.rge_txq_prodidx = sc->rge_ldata.rge_txq_considx = 0;
}

int
rge_rxeof(struct rge_softc *sc)
{
	struct mbuf_list ml = MBUF_LIST_INITIALIZER();
	struct mbuf *m;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rge_rx_desc *cur_rx;
	struct rge_rxq *rxq;
	uint32_t rxstat, extsts;
	int i, total_len, rx = 0;

	for (i = sc->rge_ldata.rge_rxq_prodidx; ; i = RGE_NEXT_RX_DESC(i)) {
		/* Invalidate the descriptor memory. */
		bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map,
		    i * sizeof(struct rge_rx_desc), sizeof(struct rge_rx_desc),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur_rx = &sc->rge_ldata.rge_rx_list[i];

		if (RGE_OWN(cur_rx))
			break;

		rxstat = letoh32(cur_rx->rge_cmdsts);
		extsts = letoh32(cur_rx->rge_extsts);
		
		total_len = RGE_RXBYTES(cur_rx);
		rxq = &sc->rge_ldata.rge_rxq[i];
		m = rxq->rxq_mbuf;
		rx = 1;

		/* Invalidate the RX mbuf and unload its map. */
		bus_dmamap_sync(sc->sc_dmat, rxq->rxq_dmamap, 0,
		    rxq->rxq_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rxq->rxq_dmamap);

		if ((rxstat & (RGE_RDCMDSTS_SOF | RGE_RDCMDSTS_EOF)) !=
		    (RGE_RDCMDSTS_SOF | RGE_RDCMDSTS_EOF)) {
			rge_discard_rxbuf(sc, i);
			continue;
		}

		if (rxstat & RGE_RDCMDSTS_RXERRSUM) {
			ifp->if_ierrors++;
			/*
			 * If this is part of a multi-fragment packet,
			 * discard all the pieces.
			 */
			 if (sc->rge_head != NULL) {
				m_freem(sc->rge_head);
				sc->rge_head = sc->rge_tail = NULL;
			}
			rge_discard_rxbuf(sc, i);
			continue;
		}

		/*
		 * If allocating a replacement mbuf fails,
		 * reload the current one.
		 */

		if (rge_newbuf(sc, i) == ENOBUFS) {
			if (sc->rge_head != NULL) {
				m_freem(sc->rge_head);
				sc->rge_head = sc->rge_tail = NULL;
			}
			rge_discard_rxbuf(sc, i);
			continue;
		}

		if (sc->rge_head != NULL) {
			m->m_len = total_len;
			/*
			 * Special case: if there's 4 bytes or less
			 * in this buffer, the mbuf can be discarded:
			 * the last 4 bytes is the CRC, which we don't
			 * care about anyway.
			 */
			if (m->m_len <= ETHER_CRC_LEN) {
				sc->rge_tail->m_len -=
				    (ETHER_CRC_LEN - m->m_len);
				m_freem(m);
			} else {
				m->m_len -= ETHER_CRC_LEN;
				m->m_flags &= ~M_PKTHDR;
				sc->rge_tail->m_next = m;
			}
			m = sc->rge_head;
			sc->rge_head = sc->rge_tail = NULL;
			m->m_pkthdr.len = total_len - ETHER_CRC_LEN;
		} else
			m->m_pkthdr.len = m->m_len =
			    (total_len - ETHER_CRC_LEN);

		/* Check IP header checksum. */
		if (!(rxstat & RGE_RDCMDSTS_IPCSUMERR) &&
		    (extsts & RGE_RDEXTSTS_IPV4))
			m->m_pkthdr.csum_flags |= M_IPV4_CSUM_IN_OK;

		/* Check TCP/UDP checksum. */
		if ((extsts & (RGE_RDEXTSTS_IPV4 | RGE_RDEXTSTS_IPV6)) &&
		    (((rxstat & RGE_RDCMDSTS_TCPPKT) &&
		    !(rxstat & RGE_RDCMDSTS_TCPCSUMERR)) ||
		    ((rxstat & RGE_RDCMDSTS_UDPPKT) &&
		    !(rxstat & RGE_RDCMDSTS_UDPCSUMERR))))
			m->m_pkthdr.csum_flags |= M_TCP_CSUM_IN_OK |
			    M_UDP_CSUM_IN_OK;

#if NVLAN > 0
		if (extsts & RGE_RDEXTSTS_VTAG) {
			m->m_pkthdr.ether_vtag =
			    ntohs(extsts & RGE_RDEXTSTS_VLAN_MASK);
			m->m_flags |= M_VLANTAG;
		}
#endif

		ml_enqueue(&ml, m);
	}

	sc->rge_ldata.rge_rxq_prodidx = i;

	if_input(ifp, &ml);

	return (rx);
}

int
rge_txeof(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rge_txq *txq;
	uint32_t txstat;
	int cons, idx, prod;
	int free = 0;

	prod = sc->rge_ldata.rge_txq_prodidx;
	cons = sc->rge_ldata.rge_txq_considx;

	while (prod != cons) {
		txq = &sc->rge_ldata.rge_txq[cons];
		idx = txq->txq_descidx;

		bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map,
		    idx * sizeof(struct rge_tx_desc),
		    sizeof(struct rge_tx_desc),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		txstat = letoh32(sc->rge_ldata.rge_tx_list[idx].rge_cmdsts);

		if (txstat & RGE_TDCMDSTS_OWN) {
			free = 2;
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, txq->txq_dmamap, 0, 
		    txq->txq_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txq->txq_dmamap);
		m_freem(txq->txq_mbuf);
		txq->txq_mbuf = NULL;

		if (txstat & (RGE_TDCMDSTS_EXCESSCOLL | RGE_TDCMDSTS_COLL))
			ifp->if_collisions++;
		if (txstat & RGE_TDCMDSTS_TXERR)
			ifp->if_oerrors++;

		bus_dmamap_sync(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map,
		    idx * sizeof(struct rge_tx_desc),
		    sizeof(struct rge_tx_desc),
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		cons = RGE_NEXT_TX_DESC(idx);
		free = 1;
	}

	if (free == 0)
		return (0);

	sc->rge_ldata.rge_txq_considx = cons;

	if (ifq_is_oactive(&ifp->if_snd))
		ifq_restart(&ifp->if_snd);
	else if (free == 2)
		ifq_serialize(&ifp->if_snd, &sc->sc_task);
	else
		ifp->if_timer = 0;

	return (1);
}

void
rge_reset(struct rge_softc *sc)
{
	int i;

	/* Enable RXDV gate. */
	RGE_SETBIT_1(sc, RGE_PPSW, 0x08);
	DELAY(2000);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		if ((RGE_READ_1(sc, RGE_MCUCMD) & (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY)) == (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY))
			break;
	}

	/* Soft reset. */
	RGE_WRITE_1(sc, RGE_CMD, RGE_CMD_RESET);

	for (i = 0; i < RGE_TIMEOUT; i++) {
		DELAY(100);
		if (!(RGE_READ_1(sc, RGE_CMD) & RGE_CMD_RESET))
			break;
	}
	if (i == RGE_TIMEOUT)
		printf("%s: reset never completed!\n", sc->sc_dev.dv_xname);
}

void
rge_iff(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ethercom *ac = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t hashes[2];
	uint32_t rxfilt;
	int h = 0;

	rxfilt = RGE_READ_4(sc, RGE_RXCFG);
	rxfilt &= ~(RGE_RXCFG_ALLPHYS | RGE_RXCFG_MULTI);
	ifp->if_flags &= ~IFF_ALLMULTI;

	/*
	 * Always accept frames destined to our station address.
	 * Always accept broadcast frames.
	 */
	rxfilt |= RGE_RXCFG_INDIV | RGE_RXCFG_BROAD;

	if (ifp->if_flags & IFF_PROMISC || ac->ac_multirangecnt > 0) {
		ifp->if_flags |= IFF_ALLMULTI;
		rxfilt |= RGE_RXCFG_MULTI;
		if (ifp->if_flags & IFF_PROMISC)
			rxfilt |= RGE_RXCFG_ALLPHYS;
		hashes[0] = hashes[1] = 0xffffffff;
	} else {
		rxfilt |= RGE_RXCFG_MULTI;
		/* Program new filter. */
		memset(hashes, 0, sizeof(hashes));

		ETHER_FIRST_MULTI(step, ac, enm);
		while (enm != NULL) {
			h = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN) >> 26;

			if (h < 32)
				hashes[0] |= (1 << h);
			else
				hashes[1] |= (1 << (h - 32));

			ETHER_NEXT_MULTI(step, enm);
		}
	}

	RGE_WRITE_4(sc, RGE_RXCFG, rxfilt);
	RGE_WRITE_4(sc, RGE_MAR0, bswap32(hashes[1]));
	RGE_WRITE_4(sc, RGE_MAR4, bswap32(hashes[0]));
}

void
rge_set_phy_power(struct rge_softc *sc, int on)
{
	int i;

	if (on) {
		RGE_SETBIT_1(sc, RGE_PMCH, 0xc0);

		rge_write_phy(sc, 0, MII_BMCR, BMCR_AUTOEN);

		for (i = 0; i < RGE_TIMEOUT; i++) {
			if ((rge_read_phy_ocp(sc, 0xa420) & 0x0080) == 3)
				break;
			DELAY(1000);
		}
	} else
		rge_write_phy(sc, 0, MII_BMCR, BMCR_AUTOEN | BMCR_PDOWN);
}

void
rge_phy_config(struct rge_softc *sc)
{
	uint16_t mcode_ver, val;
	int i;
	static const uint16_t mac_cfg3_a438_value[] =
	    { 0x0043, 0x00a7, 0x00d6, 0x00ec, 0x00f6, 0x00fb, 0x00fd, 0x00ff,
	      0x00bb, 0x0058, 0x0029, 0x0013, 0x0009, 0x0004, 0x0002 };

	static const uint16_t mac_cfg3_b88e_value[] =
	    { 0xc091, 0x6e12, 0xc092, 0x1214, 0xc094, 0x1516, 0xc096, 0x171b, 
	      0xc098, 0x1b1c, 0xc09a, 0x1f1f, 0xc09c, 0x2021, 0xc09e, 0x2224,
	      0xc0a0, 0x2424, 0xc0a2, 0x2424, 0xc0a4, 0x2424, 0xc018, 0x0af2,
	      0xc01a, 0x0d4a, 0xc01c, 0x0f26, 0xc01e, 0x118d, 0xc020, 0x14f3,
	      0xc022, 0x175a, 0xc024, 0x19c0, 0xc026, 0x1c26, 0xc089, 0x6050,
	      0xc08a, 0x5f6e, 0xc08c, 0x6e6e, 0xc08e, 0x6e6e, 0xc090, 0x6e12 };

	/* Read microcode version. */
	rge_write_phy_ocp(sc, 0xa436, 0x801e);
	mcode_ver = rge_read_phy_ocp(sc, 0xa438);

	if (sc->rge_type == MAC_CFG2) {
		for (i = 0; i < nitems(rtl8125_mac_cfg2_ephy); i++) {
			rge_write_ephy(sc, rtl8125_mac_cfg2_ephy[i].reg,
			    rtl8125_mac_cfg2_ephy[i].val);
		}

		if (mcode_ver != RGE_MAC_CFG2_MCODE_VER) {
			/* Disable PHY config. */
			RGE_CLRBIT_1(sc, 0xf2, 0x20);
			DELAY(1000);

			rge_patch_phy_mcu(sc, 1);

			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0x8600);
			rge_write_phy_ocp(sc, 0xa436, 0xb82e);
			rge_write_phy_ocp(sc, 0xa438, 0x0001);

			RGE_PHY_SETBIT(sc, 0xb820, 0x0080);
			for (i = 0; i < nitems(rtl8125_mac_cfg2_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg2_mcu[i].reg,
				    rtl8125_mac_cfg2_mcu[i].val);
			}
			RGE_PHY_CLRBIT(sc, 0xb820, 0x0080);

			rge_write_phy_ocp(sc, 0xa436, 0);
			rge_write_phy_ocp(sc, 0xa438, 0);
			RGE_PHY_CLRBIT(sc, 0xb82e, 0x0001);
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0);

			rge_patch_phy_mcu(sc, 0);

			/* Enable PHY config. */
			RGE_SETBIT_1(sc, 0xf2, 0x20);

			/* Write microcode version. */
			rge_write_phy_ocp(sc, 0xa436, 0x801e);
			rge_write_phy_ocp(sc, 0xa438, RGE_MAC_CFG2_MCODE_VER);
		}
		
		val = rge_read_phy_ocp(sc, 0xad40) & ~0x03ff;
		rge_write_phy_ocp(sc, 0xad40, val | 0x0084);
		RGE_PHY_SETBIT(sc, 0xad4e, 0x0010);
		val = rge_read_phy_ocp(sc, 0xad16) & ~0x03ff;
		rge_write_phy_ocp(sc, 0xad16, val | 0x0006);
		val = rge_read_phy_ocp(sc, 0xad32) & ~0x03ff;
		rge_write_phy_ocp(sc, 0xad32, val | 0x0006);
		RGE_PHY_CLRBIT(sc, 0xac08, 0x1100);
		val = rge_read_phy_ocp(sc, 0xac8a) & ~0xf000;
		rge_write_phy_ocp(sc, 0xac8a, val | 0x7000);
		RGE_PHY_SETBIT(sc, 0xad18, 0x0400);
		RGE_PHY_SETBIT(sc, 0xad1a, 0x03ff);
		RGE_PHY_SETBIT(sc, 0xad1c, 0x03ff);

		rge_write_phy_ocp(sc, 0xa436, 0x80ea);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0xc400);
		rge_write_phy_ocp(sc, 0xa436, 0x80eb);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0x0700;
		rge_write_phy_ocp(sc, 0xa438, val | 0x0300);
		rge_write_phy_ocp(sc, 0xa436, 0x80f8);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x1c00);
		rge_write_phy_ocp(sc, 0xa436, 0x80f1);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x3000);
		rge_write_phy_ocp(sc, 0xa436, 0x80fe);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0xa500);
		rge_write_phy_ocp(sc, 0xa436, 0x8102);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x5000);
		rge_write_phy_ocp(sc, 0xa436, 0x8105);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x3300);
		rge_write_phy_ocp(sc, 0xa436, 0x8100);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x7000);
		rge_write_phy_ocp(sc, 0xa436, 0x8104);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0xf000);
		rge_write_phy_ocp(sc, 0xa436, 0x8106);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0x6500);
		rge_write_phy_ocp(sc, 0xa436, 0x80dc);
		val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
		rge_write_phy_ocp(sc, 0xa438, val | 0xed00);
		rge_write_phy_ocp(sc, 0xa436, 0x80df);
		RGE_PHY_SETBIT(sc, 0xa438, 0x0100);
		rge_write_phy_ocp(sc, 0xa436, 0x80e1);
		RGE_PHY_CLRBIT(sc, 0xa438, 0x0100);
		val = rge_read_phy_ocp(sc, 0xbf06) & ~0x003f;
		rge_write_phy_ocp(sc, 0xbf06, val | 0x0038);
		rge_write_phy_ocp(sc, 0xa436, 0x819f);
		rge_write_phy_ocp(sc, 0xa438, 0xd0b6);
		rge_write_phy_ocp(sc, 0xbc34, 0x5555);
		val = rge_read_phy_ocp(sc, 0xbf0a) & ~0x0e00;
		rge_write_phy_ocp(sc, 0xbf0a, val | 0x0a00);
		RGE_PHY_CLRBIT(sc, 0xa5c0, 0x0400);
		RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	} else {
		for (i = 0; i < nitems(rtl8125_mac_cfg3_ephy); i++)
			rge_write_ephy(sc, rtl8125_mac_cfg3_ephy[i].reg,
			    rtl8125_mac_cfg3_ephy[i].val);

		if (mcode_ver != RGE_MAC_CFG3_MCODE_VER) {
			/* Disable PHY config. */
			RGE_CLRBIT_1(sc, 0xf2, 0x20);
			DELAY(1000);

			rge_patch_phy_mcu(sc, 1);

			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0x8601);
			rge_write_phy_ocp(sc, 0xa436, 0xb82e);
			rge_write_phy_ocp(sc, 0xa438, 0x0001);

			RGE_PHY_SETBIT(sc, 0xb820, 0x0080);
			for (i = 0; i < nitems(rtl8125_mac_cfg3_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg3_mcu[i].reg,
				    rtl8125_mac_cfg3_mcu[i].val);
			}
			RGE_PHY_CLRBIT(sc, 0xb820, 0x0080);

			rge_write_phy_ocp(sc, 0xa436, 0);
			rge_write_phy_ocp(sc, 0xa438, 0);
			RGE_PHY_CLRBIT(sc, 0xb82e, 0x0001);
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0);

			rge_patch_phy_mcu(sc, 0);

			/* Enable PHY config. */
			RGE_SETBIT_1(sc, 0xf2, 0x20);

			/* Write microcode version. */
			rge_write_phy_ocp(sc, 0xa436, 0x801e);
			rge_write_phy_ocp(sc, 0xa438, RGE_MAC_CFG3_MCODE_VER);
		}

		RGE_PHY_SETBIT(sc, 0xad4e, 0x0010);
		val = rge_read_phy_ocp(sc, 0xad16) & ~0x03ff;
		rge_write_phy_ocp(sc, 0xad16, val | 0x03ff);
		val = rge_read_phy_ocp(sc, 0xad32) & ~0x003f;
		rge_write_phy_ocp(sc, 0xad32, val | 0x0006);
		RGE_PHY_CLRBIT(sc, 0xac08, 0x1000);
		RGE_PHY_CLRBIT(sc, 0xac08, 0x0100);
		val = rge_read_phy_ocp(sc, 0xacc0) & ~0x0003;
		rge_write_phy_ocp(sc, 0xacc0, val | 0x0002);
		val = rge_read_phy_ocp(sc, 0xad40) & ~0x00e0;
		rge_write_phy_ocp(sc, 0xad40, val | 0x0040);
		val = rge_read_phy_ocp(sc, 0xad40) & ~0x0007;
		rge_write_phy_ocp(sc, 0xad40, val | 0x0004);
		RGE_PHY_CLRBIT(sc, 0xac14, 0x0080);
		RGE_PHY_CLRBIT(sc, 0xac80, 0x0300);
		val = rge_read_phy_ocp(sc, 0xac5e) & ~0x0007;
		rge_write_phy_ocp(sc, 0xac5e, val | 0x0002);
		rge_write_phy_ocp(sc, 0xad4c, 0x00a8);
		rge_write_phy_ocp(sc, 0xac5c, 0x01ff);
		val = rge_read_phy_ocp(sc, 0xac8a) & ~0x00f0;
		rge_write_phy_ocp(sc, 0xac8a, val | 0x0030);
		rge_write_phy_ocp(sc, 0xb87c, 0x80a2);
		rge_write_phy_ocp(sc, 0xb87e, 0x0153);
		rge_write_phy_ocp(sc, 0xb87c, 0x809c);
		rge_write_phy_ocp(sc, 0xb87e, 0x0153);

		rge_write_phy_ocp(sc, 0xa436, 0x81b3);
		for (i = 0; i < nitems(mac_cfg3_a438_value); i++)
			rge_write_phy_ocp(sc, 0xa438, mac_cfg3_a438_value[i]);
		for (i = 0; i < 26; i++)
			rge_write_phy_ocp(sc, 0xa438, 0);
		rge_write_phy_ocp(sc, 0xa436, 0x8257);
		rge_write_phy_ocp(sc, 0xa438, 0x020f);
		rge_write_phy_ocp(sc, 0xa436, 0x80ea);
		rge_write_phy_ocp(sc, 0xa438, 0x7843);

		rge_patch_phy_mcu(sc, 1);
		RGE_PHY_CLRBIT(sc, 0xb896, 0x0001);
		RGE_PHY_CLRBIT(sc, 0xb892, 0xff00);
		for (i = 0; i < nitems(mac_cfg3_b88e_value); i += 2) {
			rge_write_phy_ocp(sc, 0xb88e, mac_cfg3_b88e_value[i]);
			rge_write_phy_ocp(sc, 0xb890,
			    mac_cfg3_b88e_value[i + 1]);
		}
		RGE_PHY_SETBIT(sc, 0xb896, 0x0001);
		rge_patch_phy_mcu(sc, 0);

		RGE_PHY_SETBIT(sc, 0xd068, 0x2000);
		rge_write_phy_ocp(sc, 0xa436, 0x81a2);
		RGE_PHY_SETBIT(sc, 0xa438, 0x0100);
		val = rge_read_phy_ocp(sc, 0xb54c) & ~0xff00;
		rge_write_phy_ocp(sc, 0xb54c, val | 0xdb00);
		RGE_PHY_CLRBIT(sc, 0xa454, 0x0001);
		RGE_PHY_SETBIT(sc, 0xa5d4, 0x0020);
		RGE_PHY_CLRBIT(sc, 0xad4e, 0x0010);
		RGE_PHY_CLRBIT(sc, 0xa86a, 0x0001);
		RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	}

	/* Disable EEE. */
	RGE_MAC_CLRBIT(sc, 0xe040, 0x0003);
	RGE_MAC_CLRBIT(sc, 0xeb62, 0x0006);
	RGE_PHY_CLRBIT(sc, 0xa432, 0x0010);
	RGE_PHY_CLRBIT(sc, 0xa5d0, 0x0006);
	RGE_PHY_CLRBIT(sc, 0xa6d4, 0x0001);
	RGE_PHY_CLRBIT(sc, 0xa6d8, 0x0010);
	RGE_PHY_CLRBIT(sc, 0xa428, 0x0080);
	RGE_PHY_CLRBIT(sc, 0xa4a2, 0x0200);

	rge_patch_phy_mcu(sc, 1);
	RGE_MAC_CLRBIT(sc, 0xe052, 0x0001);
	RGE_PHY_CLRBIT(sc, 0xa442, 0x3000);
	RGE_PHY_CLRBIT(sc, 0xa430, 0x8000);
	rge_patch_phy_mcu(sc, 0);
}

void
rge_set_macaddr(struct rge_softc *sc, const uint8_t *addr)
{
	RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
	RGE_WRITE_4(sc, RGE_MAC0,
	    addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0]);
	RGE_WRITE_4(sc, RGE_MAC4,
	    addr[5] <<  8 | addr[4]);
	RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
}

void
rge_get_macaddr(struct rge_softc *sc, uint8_t *addr)
{
	*(uint32_t *)&addr[0] = RGE_READ_4(sc, RGE_ADDR0);
	*(uint16_t *)&addr[4] = RGE_READ_2(sc, RGE_ADDR1);
}

void
rge_hw_init(struct rge_softc *sc)
{
	int i;

	RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
	RGE_CLRBIT_1(sc, RGE_CFG5, RGE_CFG5_PME_STS);
	RGE_CLRBIT_1(sc, RGE_CFG2, RGE_CFG2_CLKREQ_EN);
	RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
	RGE_CLRBIT_1(sc, 0xf1, 0x80);

	/* Disable UPS. */
	RGE_MAC_CLRBIT(sc, 0xd40a, 0x0010);

	/* Configure MAC MCU. */
	rge_write_mac_ocp(sc, 0xfc38, 0);

	for (i = 0xfc28; i < 0xfc38; i += 2)
		rge_write_mac_ocp(sc, i, 0);

	DELAY(3000);
	rge_write_mac_ocp(sc, 0xfc26, 0);

	if (sc->rge_type == MAC_CFG3) {
		for (i = 0; i < nitems(rtl8125_def_bps); i++)
			rge_write_mac_ocp(sc, rtl8125_def_bps[i].reg,
			    rtl8125_def_bps[i].val);
	}

	/* Disable PHY power saving. */
	rge_disable_phy_ocp_pwrsave(sc);

	/* Set PCIe uncorrectable error status. */
	rge_write_csi(sc, 0x108,
	    rge_read_csi(sc, 0x108) | 0x00100000);
}

void
rge_disable_phy_ocp_pwrsave(struct rge_softc *sc)
{
	if (rge_read_phy_ocp(sc, 0xc416) != 0x0500) {
		rge_patch_phy_mcu(sc, 1);
		rge_write_phy_ocp(sc, 0xc416, 0);
		rge_write_phy_ocp(sc, 0xc416, 0x0500);
		rge_patch_phy_mcu(sc, 0);
	}
}

void
rge_patch_phy_mcu(struct rge_softc *sc, int set)
{
	uint16_t val;
	int i;

	if (set)
		RGE_PHY_SETBIT(sc, 0xb820, 0x0010);
	else
		RGE_PHY_CLRBIT(sc, 0xb820, 0x0010);

	for (i = 0; i < 1000; i++) {
		val = rge_read_phy_ocp(sc, 0xb800) & 0x0040;
		DELAY(100);
		if (val == 0x0040)
			break;
	}
	if (i == 1000)
		printf("%s: timeout waiting to patch phy mcu\n",
		    sc->sc_dev.dv_xname);
}

void
rge_add_media_types(struct rge_softc *sc)
{
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10_T, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_100_TX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_100_TX | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_1000_T, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_1000_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_2500_T, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_2500_T | IFM_FDX, 0, NULL);
}

void
rge_config_imtype(struct rge_softc *sc, int imtype)
{
	switch (imtype) {
	case RGE_IMTYPE_NONE:
		sc->rge_intrs = RGE_INTRS;
		sc->rge_rx_ack = RGE_ISR_RX_OK | RGE_ISR_RX_DESC_UNAVAIL |
		    RGE_ISR_RX_FIFO_OFLOW;
		sc->rge_tx_ack = RGE_ISR_TX_OK;
		break;
	case RGE_IMTYPE_SIM:
		sc->rge_intrs = RGE_INTRS_TIMER;
		sc->rge_rx_ack = RGE_ISR_PCS_TIMEOUT;
		sc->rge_tx_ack = RGE_ISR_PCS_TIMEOUT;
		break;
	default:
		panic("%s: unknown imtype %d", sc->sc_dev.dv_xname, imtype);
	}
}

void
rge_disable_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT, 0);
	sc->rge_timerintr = 0;
}

void
rge_setup_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT, 0x2600);
	RGE_WRITE_4(sc, RGE_TIMERCNT, 1);
	sc->rge_timerintr = 1;
}

void
rge_setup_intr(struct rge_softc *sc, int imtype)
{
	rge_config_imtype(sc, imtype);

	/* Enable interrupts. */
	RGE_WRITE_4(sc, RGE_IMR, sc->rge_intrs);

	switch (imtype) {
	case RGE_IMTYPE_NONE:
		rge_disable_sim_im(sc);
		break;
	case RGE_IMTYPE_SIM:
		rge_setup_sim_im(sc);
		break;
	default:
		panic("%s: unknown imtype %d", sc->sc_dev.dv_xname, imtype);
	}
}

void
rge_exit_oob(struct rge_softc *sc)
{
	int i;

	RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_ALLPHYS | RGE_RXCFG_INDIV |
	    RGE_RXCFG_MULTI | RGE_RXCFG_BROAD | RGE_RXCFG_RUNT |
	    RGE_RXCFG_ERRPKT);

	/* Disable RealWoW. */
	rge_write_mac_ocp(sc, 0xc0bc, 0x00ff);

	rge_reset(sc);

	/* Disable OOB. */
	RGE_CLRBIT_1(sc, RGE_MCUCMD, RGE_MCUCMD_IS_OOB);

	RGE_MAC_CLRBIT(sc, 0xe8de, 0x4000);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		if (RGE_READ_2(sc, RGE_TWICMD) & 0x0200)
			break;
	}

	rge_write_mac_ocp(sc, 0xc0aa, 0x07d0);
	rge_write_mac_ocp(sc, 0xc0a6, 0x0150);
	rge_write_mac_ocp(sc, 0xc01e, 0x5555);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		if (RGE_READ_2(sc, RGE_TWICMD) & 0x0200)
			break;
	}

	if (rge_read_mac_ocp(sc, 0xd42c) & 0x0100) {
		for (i = 0; i < RGE_TIMEOUT; i++) {
			if ((rge_read_phy_ocp(sc, 0xa420) & 0x0080) == 2)
				break;
			DELAY(1000);
		}
		RGE_MAC_CLRBIT(sc, 0xd408, 0x0100);
		RGE_PHY_CLRBIT(sc, 0xa468, 0x000a);
	}
}

void
rge_write_csi(struct rge_softc *sc, uint32_t reg, uint32_t val)
{
	int i;

	RGE_WRITE_4(sc, RGE_CSIDR, val);
	RGE_WRITE_4(sc, RGE_CSIAR, (1 << 16) | (reg & RGE_CSIAR_ADDR_MASK) |
	    (RGE_CSIAR_BYTE_EN << RGE_CSIAR_BYTE_EN_SHIFT) | RGE_CSIAR_BUSY);

	for (i = 0; i < 10; i++) {
		 DELAY(100);
		 if (!(RGE_READ_4(sc, RGE_CSIAR) & RGE_CSIAR_BUSY))
			break;
	}

	DELAY(20);
}

uint32_t
rge_read_csi(struct rge_softc *sc, uint32_t reg)
{
	int i;

	RGE_WRITE_4(sc, RGE_CSIAR, (1 << 16) | (reg & RGE_CSIAR_ADDR_MASK) |
	    (RGE_CSIAR_BYTE_EN << RGE_CSIAR_BYTE_EN_SHIFT));

	for (i = 0; i < 10; i++) {
		 DELAY(100);
		 if (RGE_READ_4(sc, RGE_CSIAR) & RGE_CSIAR_BUSY)
			break;
	}

	DELAY(20);

	return (RGE_READ_4(sc, RGE_CSIDR));
}

void
rge_write_mac_ocp(struct rge_softc *sc, uint16_t reg, uint16_t val)
{
	uint32_t tmp;

	tmp = (reg >> 1) << RGE_MACOCP_ADDR_SHIFT;
	tmp += val;
	tmp |= RGE_MACOCP_BUSY;
	RGE_WRITE_4(sc, RGE_MACOCP, tmp);
}

uint16_t
rge_read_mac_ocp(struct rge_softc *sc, uint16_t reg)
{
	uint32_t val;

	val = (reg >> 1) << RGE_MACOCP_ADDR_SHIFT;
	RGE_WRITE_4(sc, RGE_MACOCP, val);

	return (RGE_READ_4(sc, RGE_MACOCP) & RGE_MACOCP_DATA_MASK);
}

void
rge_write_ephy(struct rge_softc *sc, uint16_t reg, uint16_t val)
{
	uint32_t tmp;
	int i;

	tmp = (reg & RGE_EPHYAR_ADDR_MASK) << RGE_EPHYAR_ADDR_SHIFT;
	tmp |= RGE_EPHYAR_BUSY | (val & RGE_EPHYAR_DATA_MASK);
	RGE_WRITE_4(sc, RGE_EPHYAR, tmp);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		if (!(RGE_READ_4(sc, RGE_EPHYAR) & RGE_EPHYAR_BUSY))
			break;
	}

	DELAY(20);
}

void
rge_write_phy(struct rge_softc *sc, uint16_t addr, uint16_t reg, uint16_t val)
{
	uint16_t off, phyaddr;

	phyaddr = addr ? addr : RGE_PHYBASE + (reg / 8);
	phyaddr <<= 4;

	off = addr ? reg : 0x10 + (reg % 8);

	phyaddr += (off - 16) << 1;

	rge_write_phy_ocp(sc, phyaddr, val);
}

void
rge_write_phy_ocp(struct rge_softc *sc, uint16_t reg, uint16_t val)
{
	uint32_t tmp;
	int i;

	tmp = (reg >> 1) << RGE_PHYOCP_ADDR_SHIFT;
	tmp |= RGE_PHYOCP_BUSY | val;
	RGE_WRITE_4(sc, RGE_PHYOCP, tmp);

	for (i = 0; i < RGE_TIMEOUT; i++) {
		DELAY(1);
		if (!(RGE_READ_4(sc, RGE_PHYOCP) & RGE_PHYOCP_BUSY))
			break;
	}
}

uint16_t
rge_read_phy_ocp(struct rge_softc *sc, uint16_t reg)
{
	uint32_t val;
	int i;

	val = (reg >> 1) << RGE_PHYOCP_ADDR_SHIFT;
	RGE_WRITE_4(sc, RGE_PHYOCP, val);

	for (i = 0; i < RGE_TIMEOUT; i++) {
		DELAY(1);
		val = RGE_READ_4(sc, RGE_PHYOCP);
		if (val & RGE_PHYOCP_BUSY)
			break;
	}

	return (val & RGE_PHYOCP_DATA_MASK);
}

int
rge_get_link_status(struct rge_softc *sc)
{
	return ((RGE_READ_2(sc, RGE_PHYSTAT) & RGE_PHYSTAT_LINK) ? 1 : 0);
}

void
rge_txstart(struct work *wk, void *arg)
{
	struct rge_softc *sc = arg;

	RGE_WRITE_2(sc, RGE_TXSTART, RGE_TXSTART_START);
}

void
rge_tick(void *arg)
{
	struct rge_softc *sc = arg;
	int s;

	s = splnet();
	rge_link_state(sc);
	splx(s);

	timeout_add_sec(&sc->sc_timeout, 1);
}

void
rge_link_state(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int link = LINK_STATE_DOWN;

	if (rge_get_link_status(sc))
		link = LINK_STATE_UP;

	if (ifp->if_link_state != link) {
		ifp->if_link_state = link;
		if_link_state_change(ifp, LINK_STATE_DOWN);
	}
}
