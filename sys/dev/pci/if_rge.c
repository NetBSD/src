/*	$NetBSD: if_rge.c,v 1.14.2.1 2021/04/03 22:28:46 thorpej Exp $	*/
/*	$OpenBSD: if_rge.c,v 1.9 2020/12/12 11:48:53 jan Exp $	*/

/*
 * Copyright (c) 2019, 2020 Kevin Lo <kevlo@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: if_rge.c,v 1.14.2.1 2021/04/03 22:28:46 thorpej Exp $");

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

#include <net/bpf.h>

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

static struct mbuf *
MCLGETL(struct rge_softc *sc __unused, int how,
    u_int size)
{
	struct mbuf *m;

	MGETHDR(m, how, MT_DATA);
	if (m == NULL)
		return NULL;

	MEXTMALLOC(m, size, how);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return NULL;
	}
	return m;
}

#ifdef NET_MPSAFE
#define 	RGE_MPSAFE	1
#define 	CALLOUT_FLAGS	CALLOUT_MPSAFE
#else
#define 	CALLOUT_FLAGS	0
#endif
#endif

#ifdef RGE_DEBUG
#define DPRINTF(x)	do { if (rge_debug > 0) printf x; } while (0)
int rge_debug = 0;
#else
#define DPRINTF(x)
#endif

static int		rge_match(device_t, cfdata_t, void *);
static void		rge_attach(device_t, device_t, void *);
int		rge_intr(void *);
int		rge_encap(struct rge_softc *, struct mbuf *, int);
int		rge_ioctl(struct ifnet *, u_long, void *);
void		rge_start(struct ifnet *);
void		rge_watchdog(struct ifnet *);
int		rge_init(struct ifnet *);
void		rge_stop(struct ifnet *, int);
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
void		rge_phy_config_mac_cfg2(struct rge_softc *);
void		rge_phy_config_mac_cfg3(struct rge_softc *);
void		rge_phy_config_mac_cfg4(struct rge_softc *);
void		rge_phy_config_mac_cfg5(struct rge_softc *);
void		rge_phy_config_mcu(struct rge_softc *, uint16_t);
void		rge_set_macaddr(struct rge_softc *, const uint8_t *);
void		rge_get_macaddr(struct rge_softc *, uint8_t *);
void		rge_hw_init(struct rge_softc *);
void		rge_disable_phy_ocp_pwrsave(struct rge_softc *);
void		rge_patch_phy_mcu(struct rge_softc *, int);
void		rge_add_media_types(struct rge_softc *);
void		rge_config_imtype(struct rge_softc *, int);
void		rge_disable_hw_im(struct rge_softc *);
void		rge_disable_sim_im(struct rge_softc *);
void		rge_setup_sim_im(struct rge_softc *);
void		rge_setup_intr(struct rge_softc *, int);
void		rge_exit_oob(struct rge_softc *);
void		rge_write_csi(struct rge_softc *, uint32_t, uint32_t);
uint32_t	rge_read_csi(struct rge_softc *, uint32_t);
void		rge_write_mac_ocp(struct rge_softc *, uint16_t, uint16_t);
uint16_t	rge_read_mac_ocp(struct rge_softc *, uint16_t);
void		rge_write_ephy(struct rge_softc *, uint16_t, uint16_t);
uint16_t	rge_read_ephy(struct rge_softc *, uint16_t);
void		rge_write_phy(struct rge_softc *, uint16_t, uint16_t, uint16_t);
uint16_t	rge_read_phy(struct rge_softc *, uint16_t, uint16_t);
void		rge_write_phy_ocp(struct rge_softc *, uint16_t, uint16_t);
uint16_t	rge_read_phy_ocp(struct rge_softc *, uint16_t);
int		rge_get_link_status(struct rge_softc *);
void		rge_txstart(struct work *, void *);
void		rge_tick(void *);
void		rge_link_state(struct rge_softc *);

static const struct {
	uint16_t reg;
	uint16_t val;
}  rtl8125_mac_cfg2_mcu[] = {
	RTL8125_MAC_CFG2_MCU
}, rtl8125_mac_cfg3_mcu[] = {
	RTL8125_MAC_CFG3_MCU
}, rtl8125_mac_cfg4_mcu[] = {
	RTL8125_MAC_CFG4_MCU
}, rtl8125_mac_cfg5_mcu[] = {
	RTL8125_MAC_CFG5_MCU
};

CFATTACH_DECL_NEW(rge, sizeof(struct rge_softc), rge_match, rge_attach,
		NULL, NULL); /* Sevan - detach function? */

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
			return 3;
	}

	return 0;
}

void
rge_attach(device_t parent, device_t self, void *aux)
{
	struct rge_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t *ihp;
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr = NULL;
	struct ifnet *ifp;
	pcireg_t reg;
	uint32_t hwrev;
	uint8_t eaddr[ETHER_ADDR_LEN];
	int offset;
	pcireg_t command;

	pci_set_powerstate(pa->pa_pc, pa->pa_tag, PCI_PMCSR_STATE_D0);

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, "Ethernet controller");

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
				aprint_error(": can't map mem or i/o space\n");
				return;
			}
		}
	}

	int counts[PCI_INTR_TYPE_SIZE] = {
 		[PCI_INTR_TYPE_INTX] = 1,
 		[PCI_INTR_TYPE_MSI] = 1,
 		[PCI_INTR_TYPE_MSIX] = 1,
 	};
	int max_type = PCI_INTR_TYPE_MSIX;
	/*
	 * Allocate interrupt.
	 */
	if (pci_intr_alloc(pa, &ihp, counts, max_type) != 0) {
		aprint_error(": couldn't map interrupt\n");
		return;
	}
	switch (pci_intr_type(pc, ihp[0])) {
	case PCI_INTR_TYPE_MSIX:
	case PCI_INTR_TYPE_MSI:
		sc->rge_flags |= RGE_FLAG_MSI;
		break;
	default:
		break;
	}
	intrstr = pci_intr_string(pc, ihp[0], intrbuf, sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(pc, ihp[0], IPL_NET, rge_intr,
	    sc, device_xname(sc->sc_dev));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, ": couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s\n", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);

	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
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
	case 0x64000000:
		sc->rge_type = MAC_CFG4;
		break;
	case 0x64100000:
		sc->rge_type = MAC_CFG5;
		break;
	default:
		aprint_error(": unknown version 0x%08x\n", hwrev);
		return;
	}

	rge_config_imtype(sc, RGE_IMTYPE_SIM);

	/*
	 * PCI Express check.
	 */
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_PCIEXPRESS,
	    &offset, NULL)) {
		/* Disable PCIe ASPM and ECPM. */
		reg = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    offset + PCIE_LCSR);
		reg &= ~(PCIE_LCSR_ASPM_L0S | PCIE_LCSR_ASPM_L1 |
		    PCIE_LCSR_ENCLKPM);
		pci_conf_write(pa->pa_pc, pa->pa_tag, offset + PCIE_LCSR,
		    reg);
	}

	rge_exit_oob(sc);
	rge_hw_init(sc);

	rge_get_macaddr(sc, eaddr);
	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(eaddr));

	memcpy(sc->sc_enaddr, eaddr, ETHER_ADDR_LEN);

	rge_set_phy_power(sc, 1);
	rge_phy_config(sc);

	if (rge_allocmem(sc))
		return;

	ifp = &sc->sc_ec.ec_if;
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef RGE_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_ioctl = rge_ioctl;
	ifp->if_stop = rge_stop;
	ifp->if_start = rge_start;
	ifp->if_init = rge_init;
	ifp->if_watchdog = rge_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, RGE_TX_LIST_CNT - 1);

#if notyet
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_IPv4_Tx |IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx|
	    IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx;
#endif

	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;

	callout_init(&sc->sc_timeout, CALLOUT_FLAGS);
	callout_setfunc(&sc->sc_timeout, rge_tick, sc);

	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	/* Initialize ifmedia structures. */
	sc->sc_ec.ec_ifmedia = &sc->sc_media;
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

	if (!(sc->rge_flags & RGE_FLAG_MSI)) {
		if ((RGE_READ_4(sc, RGE_ISR) & sc->rge_intrs) == 0)
			return (0);
	}

	status = RGE_READ_4(sc, RGE_ISR);
	if (status)
		RGE_WRITE_4(sc, RGE_ISR, status);

	if (status & RGE_ISR_PCS_TIMEOUT)
		claimed = 1;

	rx = tx = 0;
	if (status & sc->rge_intrs) {
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

#if notyet
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
#endif

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
	if (vlan_has_tag(m))
		cflags |= bswap16(vlan_get_tag(m)) | RGE_TDEXTSTS_VTAG;

	last = cur = idx;
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
	//struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/* XXX set an ifflags callback and let ether_ioctl
		 * handle all of this.
		 */
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				error = ENETRESET;
			else
				rge_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				rge_stop(ifp, 1);
		}
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
		IFQ_PURGE(&ifp->if_snd);
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

		bpf_mtap(ifp, m, BPF_D_OUT);

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
#if 0
	ifq_serialize(ifq, &sc->sc_task);
#else
	rge_txstart(&sc->sc_task, sc);
#endif
}

void
rge_watchdog(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;

	device_printf(sc->sc_dev, "watchdog timeout\n");
	if_statinc(ifp, if_oerrors);

	rge_init(ifp);
}

int
rge_init(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	uint32_t val;
	int i;

	rge_stop(ifp, 0);

	/* Set MAC address. */
	rge_set_macaddr(sc, CLLADDR(ifp->if_sadl));

	/* Set Maximum frame size. */
	RGE_WRITE_2(sc, RGE_RXMAXSIZE, RGE_JUMBO_FRAMELEN);

	/* Initialize RX descriptors list. */
	if (rge_rx_list_init(sc) == ENOBUFS) {
		device_printf(sc->sc_dev,
		    "init failed: no memory for RX buffers\n");
		rge_stop(ifp, 1);
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
		RGE_WRITE_4(sc, RGE_INTMITI(i), 0);

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
	if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3)
		rge_write_mac_ocp(sc, 0xe614, val | 0x0400);
	else
		rge_write_mac_ocp(sc, 0xe614, val | 0x0200);

	RGE_MAC_CLRBIT(sc, 0xe63e, 0x0c00);

	if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3) {
		val = rge_read_mac_ocp(sc, 0xe63e) & ~0x0030;
		rge_write_mac_ocp(sc, 0xe63e, val | 0x0020);
	} else
		RGE_MAC_CLRBIT(sc, 0xe63e, 0x0030);

	RGE_MAC_SETBIT(sc, 0xc0b4, 0x000c);

	val = rge_read_mac_ocp(sc, 0xeb6a) & ~0x00ff;
	rge_write_mac_ocp(sc, 0xeb6a, val | 0x0033);

	val = rge_read_mac_ocp(sc, 0xeb50) & ~0x03e0;
	rge_write_mac_ocp(sc, 0xeb50, val | 0x0040);

	val = rge_read_mac_ocp(sc, 0xe056) & ~0x00f0;
	rge_write_mac_ocp(sc, 0xe056, val | 0x0030);

	RGE_WRITE_1(sc, RGE_TDFNR, 0x10);

	RGE_SETBIT_1(sc, RGE_DLLPR, RGE_DLLPR_TX_10M_PS_EN);

	RGE_MAC_CLRBIT(sc, 0xe040, 0x1000);

	val = rge_read_mac_ocp(sc, 0xea1c) & ~0x0003;
	rge_write_mac_ocp(sc, 0xea1c, val | 0x0001);

	val = rge_read_mac_ocp(sc, 0xe0c0) & ~0x4f0f;
	rge_write_mac_ocp(sc, 0xe0c0, val | 0x4403);

	RGE_MAC_SETBIT(sc, 0xe052, 0x0068);
	RGE_MAC_CLRBIT(sc, 0xe052, 0x0080);

	val = rge_read_mac_ocp(sc, 0xc0ac) & ~0x0080;
	rge_write_mac_ocp(sc, 0xc0ac, val | 0x1f00);

	val = rge_read_mac_ocp(sc, 0xd430) & ~0x0fff;
	rge_write_mac_ocp(sc, 0xd430, val | 0x047f);

	val = rge_read_mac_ocp(sc, 0xe84c) & ~0x0040;
	if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3)
		rge_write_mac_ocp(sc, 0xe84c, 0x00c0);
	else
		rge_write_mac_ocp(sc, 0xe84c, 0x0080);

	RGE_SETBIT_1(sc, RGE_DLLPR, RGE_DLLPR_PFM_EN);

	if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3)
		RGE_SETBIT_1(sc, RGE_MCUCMD, 0x01);

	/* Disable EEE plus. */
	RGE_MAC_CLRBIT(sc, 0xe080, 0x0002);

	RGE_MAC_CLRBIT(sc, 0xea1c, 0x0004);

	RGE_MAC_SETBIT(sc, 0xeb54, 0x0001);
	DELAY(1);
	RGE_MAC_CLRBIT(sc, 0xeb54, 0x0001);

	RGE_CLRBIT_4(sc, 0x1880, 0x0030);

	rge_write_mac_ocp(sc, 0xe098, 0xc302);

	if ((sc->sc_ec.ec_capenable & ETHERCAP_VLAN_HWTAGGING) != 0)
		RGE_SETBIT_4(sc, RGE_RXCFG, RGE_RXCFG_VLANSTRIP);
	else
		RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_VLANSTRIP);

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
rge_stop(struct ifnet *ifp, int disable)
{
	struct rge_softc *sc = ifp->if_softc;
	int i;

	if (disable) {
		callout_halt(&sc->sc_timeout, NULL);
	} else 
		callout_stop(&sc->sc_timeout);

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_RUNNING;
	sc->rge_timerintr = 0;

	RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_ALLPHYS | RGE_RXCFG_INDIV |
	    RGE_RXCFG_MULTI | RGE_RXCFG_BROAD | RGE_RXCFG_RUNT |
	    RGE_RXCFG_ERRPKT);

	RGE_WRITE_4(sc, RGE_IMR, 0);

	/* Clear timer interrupts. */
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT1, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT2, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT3, 0);

	rge_reset(sc);

//	intr_barrier(sc->sc_ih);
//	ifq_barrier(&ifp->if_snd);
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
		anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig = GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		val |= RGE_ADV_2500TFDX;
		break;
	case IFM_2500_T:
		anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig = GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		val |= RGE_ADV_2500TFDX;
		ifp->if_baudrate = IF_Mbps(2500);
		break;
	case IFM_1000_T:
		anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		gig = GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		ifp->if_baudrate = IF_Gbps(1);
		break;
	case IFM_100_TX:
		gig = rge_read_phy(sc, 0, MII_100T2CR) &
		    ~(GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX);
		anar = ((ifm->ifm_media & IFM_GMASK) == IFM_FDX) ?
		    ANAR_TX | ANAR_TX_FD | ANAR_10_FD | ANAR_10 :
		    ANAR_TX | ANAR_10_FD | ANAR_10;
		ifp->if_baudrate = IF_Mbps(100);
		break;
	case IFM_10_T:
		gig = rge_read_phy(sc, 0, MII_100T2CR) &
		    ~(GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX);
		anar = ((ifm->ifm_media & IFM_GMASK) == IFM_FDX) ?
		    ANAR_10_FD | ANAR_10 : ANAR_10;
		ifp->if_baudrate = IF_Mbps(10);
		break;
	default:
		device_printf(sc->sc_dev,
		    "unsupported media type\n");
		return (EINVAL);
	}

	rge_write_phy(sc, 0, MII_ANAR, anar | ANAR_PAUSE_ASYM | ANAR_FC);
	rge_write_phy(sc, 0, MII_100T2CR, gig);
	rge_write_phy_ocp(sc, 0xa5d4, val);
	rge_write_phy(sc, 0, MII_BMCR, BMCR_RESET | BMCR_AUTOEN |
	    BMCR_STARTNEG);

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
		aprint_error_dev(sc->sc_dev, "can't create TX list map\n");
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_TX_LIST_SZ, RGE_ALIGN, 0,
	    &sc->rge_ldata.rge_tx_listseg, 1, &sc->rge_ldata.rge_tx_listnseg,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc TX list\n");
		return (error);
	}

	/* Load the map for the TX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &sc->rge_ldata.rge_tx_listseg,
	    sc->rge_ldata.rge_tx_listnseg, RGE_TX_LIST_SZ,
	    (void **) &sc->rge_ldata.rge_tx_list,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't map TX dma buffers\n");
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_tx_listseg,
		    sc->rge_ldata.rge_tx_listnseg);
		return (error);
	}
	memset(sc->rge_ldata.rge_tx_list, 0, RGE_TX_LIST_SZ);
	error = bus_dmamap_load(sc->sc_dmat, sc->rge_ldata.rge_tx_list_map,
	    sc->rge_ldata.rge_tx_list, RGE_TX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't load TX dma map\n");
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
			aprint_error_dev(sc->sc_dev, "can't create DMA map for TX\n");
			return (error);
		}
	}

	/* Allocate DMA'able memory for the RX ring. */
	error = bus_dmamap_create(sc->sc_dmat, RGE_RX_LIST_SZ, 1,
	    RGE_RX_LIST_SZ, 0, 0, &sc->rge_ldata.rge_rx_list_map);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't create RX list map\n");
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_RX_LIST_SZ, RGE_ALIGN, 0,
	    &sc->rge_ldata.rge_rx_listseg, 1, &sc->rge_ldata.rge_rx_listnseg,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc RX list\n");
		return (error);
	}

	/* Load the map for the RX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &sc->rge_ldata.rge_rx_listseg,
	    sc->rge_ldata.rge_rx_listnseg, RGE_RX_LIST_SZ,
	    (void **) &sc->rge_ldata.rge_rx_list,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't map RX dma buffers\n");
		bus_dmamem_free(sc->sc_dmat, &sc->rge_ldata.rge_rx_listseg,
		    sc->rge_ldata.rge_rx_listnseg);
		return (error);
	}
	memset(sc->rge_ldata.rge_rx_list, 0, RGE_RX_LIST_SZ);
	error = bus_dmamap_load(sc->sc_dmat, sc->rge_ldata.rge_rx_list_map,
	    sc->rge_ldata.rge_rx_list, RGE_RX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't load RX dma map\n");
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
			aprint_error_dev(sc->sc_dev, "can't create DMA map for RX\n");
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

	m = MCLGETL(NULL, M_DONTWAIT, RGE_JUMBO_FRAMELEN);
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
		device_printf(sc->sc_dev, "tried to map busy RX descriptor\n");
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

	sc->rge_ldata.rge_rxq_prodidx = sc->rge_ldata.rge_rxq_considx = 0;
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
	struct mbuf *m;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rge_rx_desc *cur_rx;
	struct rge_rxq *rxq;
	uint32_t rxstat, extsts;
	int i, total_len, rx = 0;

	for (i = sc->rge_ldata.rge_rxq_considx; ; i = RGE_NEXT_RX_DESC(i)) {
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
		rxq->rxq_mbuf = NULL;
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
			if_statinc(ifp, if_ierrors);
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

		m_set_rcvif(m, ifp);
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
	#if 0
			m->m_pkthdr.len = m->m_len =
			    (total_len - ETHER_CRC_LEN);
	#else
		{
			m->m_pkthdr.len = m->m_len = total_len;
			m->m_flags |= M_HASFCS;
		}
	#endif

#if notyet
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
#endif

		if (extsts & RGE_RDEXTSTS_VTAG) {
			vlan_set_tag(m,
			    bswap16(extsts & RGE_RDEXTSTS_VLAN_MASK));
		}

		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

	sc->rge_ldata.rge_rxq_considx = i;

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
			if_statinc(ifp, if_collisions);
		if (txstat & RGE_TDCMDSTS_TXERR)
			if_statinc(ifp, if_oerrors);

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

#if 0
	if (ifq_is_oactive(&ifp->if_snd))
		ifq_restart(&ifp->if_snd);
	else if (free == 2)
		ifq_serialize(&ifp->if_snd, &sc->sc_task);
	else
		ifp->if_timer = 0;
#else
#if 0
	if (!IF_IS_EMPTY(&ifp->if_snd))
		rge_start(ifp);
	else
	if (free == 2)
		if (0) { rge_txstart(&sc->sc_task, sc); }
	else
#endif
		ifp->if_timer = 0;
#endif

	return (1);
}

void
rge_reset(struct rge_softc *sc)
{
	int i;

	/* Enable RXDV gate. */
	RGE_SETBIT_1(sc, RGE_PPSW, 0x08);
	DELAY(2000);

	for (i = 0; i < 3000; i++) {
		DELAY(50);
		if ((RGE_READ_1(sc, RGE_MCUCMD) & (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY)) == (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY))
			break;
	}
	if (sc->rge_type == MAC_CFG4 || sc->rge_type == MAC_CFG5) {
		for (i = 0; i < 3000; i++) {
			DELAY(50);
			if ((RGE_READ_2(sc, RGE_IM) & 0x0103) == 0x0103)
				break;
		}
	}

	DELAY(2000);

	/* Soft reset. */
	RGE_WRITE_1(sc, RGE_CMD, RGE_CMD_RESET);

	for (i = 0; i < RGE_TIMEOUT; i++) {
		DELAY(100);
		if (!(RGE_READ_1(sc, RGE_CMD) & RGE_CMD_RESET))
			break;
	}
	if (i == RGE_TIMEOUT)
		device_printf(sc->sc_dev, "reset never completed!\n");
}

void
rge_iff(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ethercom *ec = &sc->sc_ec;
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

	if (ifp->if_flags & IFF_PROMISC) {
 allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		rxfilt |= RGE_RXCFG_MULTI;
		if (ifp->if_flags & IFF_PROMISC)
			rxfilt |= RGE_RXCFG_ALLPHYS;
		hashes[0] = hashes[1] = 0xffffffff;
	} else {
		rxfilt |= RGE_RXCFG_MULTI;
		/* Program new filter. */
		memset(hashes, 0, sizeof(hashes));

		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN) != 0) {
			    	ETHER_UNLOCK(ec);
				goto allmulti;
			}
			h = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN) >> 26;

			if (h < 32)
				hashes[0] |= (1 << h);
			else
				hashes[1] |= (1 << (h - 32));

			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
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
			if ((rge_read_phy_ocp(sc, 0xa420) & 0x0007) == 3)
				break;
			DELAY(1000);
		}
	} else {
		rge_write_phy(sc, 0, MII_BMCR, BMCR_AUTOEN | BMCR_PDOWN);
		RGE_CLRBIT_1(sc, RGE_PMCH, 0x80);
		RGE_CLRBIT_1(sc, RGE_PPSW, 0x40);
	}
}

void
rge_phy_config(struct rge_softc *sc)
{
	/* Read microcode version. */
	rge_write_phy_ocp(sc, 0xa436, 0x801e);
	sc->rge_mcodever = rge_read_phy_ocp(sc, 0xa438);

	switch (sc->rge_type) {
	case MAC_CFG2:
		rge_phy_config_mac_cfg2(sc);
		break;
	case MAC_CFG3:
		rge_phy_config_mac_cfg3(sc);
		break;
	case MAC_CFG4:
		rge_phy_config_mac_cfg4(sc);
		break;
	case MAC_CFG5:
		rge_phy_config_mac_cfg5(sc);
		break;
	default:
		break;	/* Can't happen. */
	}

	rge_write_phy(sc, 0x0a5b, 0x12,
	    rge_read_phy(sc, 0x0a5b, 0x12) & ~0x8000);

	/* Disable EEE. */
	RGE_MAC_CLRBIT(sc, 0xe040, 0x0003);
	if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3) {
		RGE_MAC_CLRBIT(sc, 0xeb62, 0x0006);
		RGE_PHY_CLRBIT(sc, 0xa432, 0x0010);
	}
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
rge_phy_config_mac_cfg2(struct rge_softc *sc)
{
	uint16_t val;
	int i;

	for (i = 0; i < nitems(rtl8125_mac_cfg2_ephy); i++)
		rge_write_ephy(sc, rtl8125_mac_cfg2_ephy[i].reg,
		    rtl8125_mac_cfg2_ephy[i].val);

	rge_phy_config_mcu(sc, RGE_MAC_CFG2_MCODE_VER);

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
}

void
rge_phy_config_mac_cfg3(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint16_t val;
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

	for (i = 0; i < nitems(rtl8125_mac_cfg3_ephy); i++)
		rge_write_ephy(sc, rtl8125_mac_cfg3_ephy[i].reg,
		    rtl8125_mac_cfg3_ephy[i].val);

	val = rge_read_ephy(sc, 0x002a) & ~0x7000;
	rge_write_ephy(sc, 0x002a, val | 0x3000);
	RGE_EPHY_CLRBIT(sc, 0x0019, 0x0040);
	RGE_EPHY_SETBIT(sc, 0x001b, 0x0e00);
	RGE_EPHY_CLRBIT(sc, 0x001b, 0x7000);
	rge_write_ephy(sc, 0x0002, 0x6042);
	rge_write_ephy(sc, 0x0006, 0x0014);
	val = rge_read_ephy(sc, 0x006a) & ~0x7000;
	rge_write_ephy(sc, 0x006a, val | 0x3000);
	RGE_EPHY_CLRBIT(sc, 0x0059, 0x0040);
	RGE_EPHY_SETBIT(sc, 0x005b, 0x0e00);
	RGE_EPHY_CLRBIT(sc, 0x005b, 0x7000);
	rge_write_ephy(sc, 0x0042, 0x6042);
	rge_write_ephy(sc, 0x0046, 0x0014);

	rge_phy_config_mcu(sc, RGE_MAC_CFG3_MCODE_VER);

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
	rge_write_phy_ocp(sc, 0xb87c, 0x8157);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0500);
	rge_write_phy_ocp(sc, 0xb87c, 0x8159);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0700);
	RGE_WRITE_2(sc, RGE_EEE_TXIDLE_TIMER, ifp->if_mtu + ETHER_HDR_LEN +
	    32);
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
		rge_write_phy_ocp(sc, 0xb890, mac_cfg3_b88e_value[i + 1]);
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

void
rge_phy_config_mac_cfg4(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint16_t val;
	int i;
	static const uint16_t mac_cfg4_b87c_value[] =
	    { 0x8013, 0x0700, 0x8fb9, 0x2801, 0x8fba, 0x0100, 0x8fbc, 0x1900, 
	      0x8fbe, 0xe100, 0x8fc0, 0x0800, 0x8fc2, 0xe500, 0x8fc4, 0x0f00, 
	      0x8fc6, 0xf100, 0x8fc8, 0x0400, 0x8fca, 0xf300, 0x8fcc, 0xfd00, 
	      0x8fce, 0xff00, 0x8fd0, 0xfb00, 0x8fd2, 0x0100, 0x8fd4, 0xf400, 
	      0x8fd6, 0xff00, 0x8fd8, 0xf600, 0x813d, 0x390e, 0x814f, 0x790e, 
	      0x80b0, 0x0f31 };

	for (i = 0; i < nitems(rtl8125_mac_cfg4_ephy); i++)
		rge_write_ephy(sc, rtl8125_mac_cfg4_ephy[i].reg,
		    rtl8125_mac_cfg4_ephy[i].val);

	rge_write_phy_ocp(sc, 0xbf86, 0x9000);
	RGE_PHY_SETBIT(sc, 0xc402, 0x0400);
	RGE_PHY_CLRBIT(sc, 0xc402, 0x0400);
	rge_write_phy_ocp(sc, 0xbd86, 0x1010);
	rge_write_phy_ocp(sc, 0xbd88, 0x1010);
	val = rge_read_phy_ocp(sc, 0xbd4e) & ~0x0c00;
	rge_write_phy_ocp(sc, 0xbd4e, val | 0x0800);
	val = rge_read_phy_ocp(sc, 0xbf46) & ~0x0f00;
	rge_write_phy_ocp(sc, 0xbf46, val | 0x0700);

	rge_phy_config_mcu(sc, RGE_MAC_CFG4_MCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	RGE_PHY_SETBIT(sc, 0xbc08, 0x000c);
	rge_write_phy_ocp(sc, 0xa436, 0x8fff);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0400);
	for (i = 0; i < 6; i++) {
		rge_write_phy_ocp(sc, 0xb87c, 0x8560 + i * 2);
		if (i < 3)
			rge_write_phy_ocp(sc, 0xb87e, 0x19cc);
		else
			rge_write_phy_ocp(sc, 0xb87e, 0x147d);
	}
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffe);
	rge_write_phy_ocp(sc, 0xb87e, 0x0907);
	val = rge_read_phy_ocp(sc, 0xacda) & ~0xff00;
	rge_write_phy_ocp(sc, 0xacda, val | 0xff00);
	val = rge_read_phy_ocp(sc, 0xacde) & ~0xf000;
	rge_write_phy_ocp(sc, 0xacde, val | 0xf000);
	rge_write_phy_ocp(sc, 0xb87c, 0x80d6);
	rge_write_phy_ocp(sc, 0xb87e, 0x2801);
	rge_write_phy_ocp(sc, 0xb87c, 0x80F2);
	rge_write_phy_ocp(sc, 0xb87e, 0x2801);
	rge_write_phy_ocp(sc, 0xb87c, 0x80f4);
	rge_write_phy_ocp(sc, 0xb87e, 0x6077);
	rge_write_phy_ocp(sc, 0xb506, 0x01e7);
	rge_write_phy_ocp(sc, 0xac8c, 0x0ffc);
	rge_write_phy_ocp(sc, 0xac46, 0xb7b4);
	rge_write_phy_ocp(sc, 0xac50, 0x0fbc);
	rge_write_phy_ocp(sc, 0xac3c, 0x9240);
	rge_write_phy_ocp(sc, 0xac4E, 0x0db4);
	rge_write_phy_ocp(sc, 0xacc6, 0x0707);
	rge_write_phy_ocp(sc, 0xacc8, 0xa0d3);
	rge_write_phy_ocp(sc, 0xad08, 0x0007);
	for (i = 0; i < nitems(mac_cfg4_b87c_value); i += 2) {
		rge_write_phy_ocp(sc, 0xb87c, mac_cfg4_b87c_value[i]);
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg4_b87c_value[i + 1]);
	}
	RGE_PHY_SETBIT(sc, 0xbf4c, 0x0002);
	RGE_PHY_SETBIT(sc, 0xbcca, 0x0300);
	rge_write_phy_ocp(sc, 0xb87c, 0x8141);
	rge_write_phy_ocp(sc, 0xb87e, 0x320e);
	rge_write_phy_ocp(sc, 0xb87c, 0x8153);
	rge_write_phy_ocp(sc, 0xb87e, 0x720e);
	RGE_PHY_CLRBIT(sc, 0xa432, 0x0040);
	rge_write_phy_ocp(sc, 0xb87c, 0x8529);
	rge_write_phy_ocp(sc, 0xb87e, 0x050e);
	RGE_WRITE_2(sc, RGE_EEE_TXIDLE_TIMER, ifp->if_mtu + ETHER_HDR_LEN +
	    32);
	rge_write_phy_ocp(sc, 0xa436, 0x816c);
	rge_write_phy_ocp(sc, 0xa438, 0xc4a0);
	rge_write_phy_ocp(sc, 0xa436, 0x8170);
	rge_write_phy_ocp(sc, 0xa438, 0xc4a0);
	rge_write_phy_ocp(sc, 0xa436, 0x8174);
	rge_write_phy_ocp(sc, 0xa438, 0x04a0);
	rge_write_phy_ocp(sc, 0xa436, 0x8178);
	rge_write_phy_ocp(sc, 0xa438, 0x04a0);
	rge_write_phy_ocp(sc, 0xa436, 0x817c);
	rge_write_phy_ocp(sc, 0xa438, 0x0719);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff4);
	rge_write_phy_ocp(sc, 0xa438, 0x0400);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff1);
	rge_write_phy_ocp(sc, 0xa438, 0x0404);
	rge_write_phy_ocp(sc, 0xbf4a, 0x001b);
	for (i = 0; i < 6; i++) {
		rge_write_phy_ocp(sc, 0xb87c, 0x8033 + i * 4);
		if (i == 2)
			rge_write_phy_ocp(sc, 0xb87e, 0xfc32);
		else
			rge_write_phy_ocp(sc, 0xb87e, 0x7c13);
	}
	rge_write_phy_ocp(sc, 0xb87c, 0x8145);
	rge_write_phy_ocp(sc, 0xb87e, 0x370e);
	rge_write_phy_ocp(sc, 0xb87c, 0x8157);
	rge_write_phy_ocp(sc, 0xb87e, 0x770e);
	rge_write_phy_ocp(sc, 0xb87c, 0x8169);
	rge_write_phy_ocp(sc, 0xb87e, 0x0d0a);
	rge_write_phy_ocp(sc, 0xb87c, 0x817b);
	rge_write_phy_ocp(sc, 0xb87e, 0x1d0a);
	rge_write_phy_ocp(sc, 0xa436, 0x8217);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5000);
	rge_write_phy_ocp(sc, 0xa436, 0x821a);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5000);
	rge_write_phy_ocp(sc, 0xa436, 0x80da);
	rge_write_phy_ocp(sc, 0xa438, 0x0403);
	rge_write_phy_ocp(sc, 0xa436, 0x80dc);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x80b3);
	rge_write_phy_ocp(sc, 0xa438, 0x0384);
	rge_write_phy_ocp(sc, 0xa436, 0x80b7);
	rge_write_phy_ocp(sc, 0xa438, 0x2007);
	rge_write_phy_ocp(sc, 0xa436, 0x80ba);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x6c00);
	rge_write_phy_ocp(sc, 0xa436, 0x80b5);
	rge_write_phy_ocp(sc, 0xa438, 0xf009);
	rge_write_phy_ocp(sc, 0xa436, 0x80bd);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x9f00);
	rge_write_phy_ocp(sc, 0xa436, 0x80c7);
	rge_write_phy_ocp(sc, 0xa438, 0xf083);
	rge_write_phy_ocp(sc, 0xa436, 0x80dd);
	rge_write_phy_ocp(sc, 0xa438, 0x03f0);
	rge_write_phy_ocp(sc, 0xa436, 0x80df);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x80cb);
	rge_write_phy_ocp(sc, 0xa438, 0x2007);
	rge_write_phy_ocp(sc, 0xa436, 0x80ce);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x6c00);
	rge_write_phy_ocp(sc, 0xa436, 0x80c9);
	rge_write_phy_ocp(sc, 0xa438, 0x8009);
	rge_write_phy_ocp(sc, 0xa436, 0x80d1);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x8000);
	rge_write_phy_ocp(sc, 0xa436, 0x80a3);
	rge_write_phy_ocp(sc, 0xa438, 0x200a);
	rge_write_phy_ocp(sc, 0xa436, 0x80a5);
	rge_write_phy_ocp(sc, 0xa438, 0xf0ad);
	rge_write_phy_ocp(sc, 0xa436, 0x809f);
	rge_write_phy_ocp(sc, 0xa438, 0x6073);
	rge_write_phy_ocp(sc, 0xa436, 0x80a1);
	rge_write_phy_ocp(sc, 0xa438, 0x000b);
	rge_write_phy_ocp(sc, 0xa436, 0x80a9);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xc000);
	rge_patch_phy_mcu(sc, 1);
	RGE_PHY_CLRBIT(sc, 0xb896, 0x0001); 
	RGE_PHY_CLRBIT(sc, 0xb892, 0xff00); 
	rge_write_phy_ocp(sc, 0xb88e, 0xc23e);
	rge_write_phy_ocp(sc, 0xb890, 0x0000);
	rge_write_phy_ocp(sc, 0xb88e, 0xc240);
	rge_write_phy_ocp(sc, 0xb890, 0x0103);
	rge_write_phy_ocp(sc, 0xb88e, 0xc242);
	rge_write_phy_ocp(sc, 0xb890, 0x0507);
	rge_write_phy_ocp(sc, 0xb88e, 0xc244);
	rge_write_phy_ocp(sc, 0xb890, 0x090b);
	rge_write_phy_ocp(sc, 0xb88e, 0xc246);
	rge_write_phy_ocp(sc, 0xb890, 0x0c0e);
	rge_write_phy_ocp(sc, 0xb88e, 0xc248);
	rge_write_phy_ocp(sc, 0xb890, 0x1012);
	rge_write_phy_ocp(sc, 0xb88e, 0xc24a);
	rge_write_phy_ocp(sc, 0xb890, 0x1416);
	RGE_PHY_SETBIT(sc, 0xb896, 0x0001); 
	rge_patch_phy_mcu(sc, 0);
	RGE_PHY_SETBIT(sc, 0xa86a, 0x0001); 
	RGE_PHY_SETBIT(sc, 0xa6f0, 0x0001); 
	rge_write_phy_ocp(sc, 0xbfa0, 0xd70d);
	rge_write_phy_ocp(sc, 0xbfa2, 0x4100);
	rge_write_phy_ocp(sc, 0xbfa4, 0xe868);
	rge_write_phy_ocp(sc, 0xbfa6, 0xdc59);
	rge_write_phy_ocp(sc, 0xb54c, 0x3c18);
	RGE_PHY_CLRBIT(sc, 0xbfa4, 0x0020);
	rge_write_phy_ocp(sc, 0xa436, 0x817d);
	RGE_PHY_SETBIT(sc, 0xa438, 0x1000); 
}

void
rge_phy_config_mac_cfg5(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint16_t val;
	int i;

	for (i = 0; i < nitems(rtl8125_mac_cfg5_ephy); i++)
		rge_write_ephy(sc, rtl8125_mac_cfg5_ephy[i].reg,
		    rtl8125_mac_cfg5_ephy[i].val);

	val = rge_read_ephy(sc, 0x0022) & ~0x0030;
	rge_write_ephy(sc, 0x0022, val | 0x0020);
	val = rge_read_ephy(sc, 0x0062) & ~0x0030;
	rge_write_ephy(sc, 0x0062, val | 0x0020);

	rge_phy_config_mcu(sc, RGE_MAC_CFG5_MCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	val = rge_read_phy_ocp(sc, 0xac46) & ~0x00f0;
	rge_write_phy_ocp(sc, 0xac46, val | 0x0090);
	val = rge_read_phy_ocp(sc, 0xad30) & ~0x0003;
	rge_write_phy_ocp(sc, 0xad30, val | 0x0001);
	RGE_WRITE_2(sc, RGE_EEE_TXIDLE_TIMER, ifp->if_mtu + ETHER_HDR_LEN +
	    32);
	rge_write_phy_ocp(sc, 0xb87c, 0x80f5);
	rge_write_phy_ocp(sc, 0xb87e, 0x760e);
	rge_write_phy_ocp(sc, 0xb87c, 0x8107);
	rge_write_phy_ocp(sc, 0xb87e, 0x360e);
	rge_write_phy_ocp(sc, 0xb87c, 0x8551);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0800);
	val = rge_read_phy_ocp(sc, 0xbf00) & ~0xe000;
	rge_write_phy_ocp(sc, 0xbf00, val | 0xa000);
	val = rge_read_phy_ocp(sc, 0xbf46) & ~0x0f00;
	rge_write_phy_ocp(sc, 0xbf46, val | 0x0300);
	for (i = 0; i < 10; i++) {
		rge_write_phy_ocp(sc, 0xa436, 0x8044 + i * 6);
		rge_write_phy_ocp(sc, 0xa438, 0x2417);
	}
	RGE_PHY_SETBIT(sc, 0xa4ca, 0x0040);
	val = rge_read_phy_ocp(sc, 0xbf84) & ~0xe000;
	rge_write_phy_ocp(sc, 0xbf84, val | 0xa000);
}

void
rge_phy_config_mcu(struct rge_softc *sc, uint16_t mcode_version)
{
	if (sc->rge_mcodever != mcode_version) {
		int i;

		rge_patch_phy_mcu(sc, 1);

		if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3) {
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			if (sc->rge_type == MAC_CFG2)
				rge_write_phy_ocp(sc, 0xa438, 0x8600);
			else
				rge_write_phy_ocp(sc, 0xa438, 0x8601);
			rge_write_phy_ocp(sc, 0xa436, 0xb82e);
			rge_write_phy_ocp(sc, 0xa438, 0x0001);

			RGE_PHY_SETBIT(sc, 0xb820, 0x0080);
		}

		if (sc->rge_type == MAC_CFG2) {
			for (i = 0; i < nitems(rtl8125_mac_cfg2_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg2_mcu[i].reg,
				    rtl8125_mac_cfg2_mcu[i].val);
			}
		} else if (sc->rge_type == MAC_CFG3) {
			for (i = 0; i < nitems(rtl8125_mac_cfg3_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg3_mcu[i].reg,
				    rtl8125_mac_cfg3_mcu[i].val);
			}
		} else if (sc->rge_type == MAC_CFG4) {
			for (i = 0; i < nitems(rtl8125_mac_cfg4_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg4_mcu[i].reg,
				    rtl8125_mac_cfg4_mcu[i].val);
			}
		} else if (sc->rge_type == MAC_CFG5) {
			for (i = 0; i < nitems(rtl8125_mac_cfg5_mcu); i++) {
				rge_write_phy_ocp(sc,
				    rtl8125_mac_cfg5_mcu[i].reg,
				    rtl8125_mac_cfg5_mcu[i].val);
			}
		}

		if (sc->rge_type == MAC_CFG2 || sc->rge_type == MAC_CFG3) {
			RGE_PHY_CLRBIT(sc, 0xb820, 0x0080);

			rge_write_phy_ocp(sc, 0xa436, 0);
			rge_write_phy_ocp(sc, 0xa438, 0);
			RGE_PHY_CLRBIT(sc, 0xb82e, 0x0001);
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0);
		}

		rge_patch_phy_mcu(sc, 0);

		/* Write microcode version. */
		rge_write_phy_ocp(sc, 0xa436, 0x801e);
		rge_write_phy_ocp(sc, 0xa438, mcode_version);
	}
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
		for (i = 0; i < nitems(rtl8125_mac_bps); i++) {
			rge_write_mac_ocp(sc, rtl8125_mac_bps[i].reg,
			    rtl8125_mac_bps[i].val);
		}
	} else if (sc->rge_type == MAC_CFG5) {
		for (i = 0; i < nitems(rtl8125b_mac_bps); i++) {
			rge_write_mac_ocp(sc, rtl8125b_mac_bps[i].reg,
			    rtl8125b_mac_bps[i].val);
		}
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
	int i;

	if (set)
		RGE_PHY_SETBIT(sc, 0xb820, 0x0010);
	else
		RGE_PHY_CLRBIT(sc, 0xb820, 0x0010);

	for (i = 0; i < 1000; i++) {
		if ((rge_read_phy_ocp(sc, 0xb800) & 0x0040) == 0x0040)
			break;
		DELAY(100);
	}
	if (i == 1000) {
		DPRINTF(("timeout waiting to patch phy mcu\n"));
		return;
	}
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
		panic("%s: unknown imtype %d", device_xname(sc->sc_dev), imtype);
	}
}

void
rge_disable_hw_im(struct rge_softc *sc)
{
	RGE_WRITE_2(sc, RGE_IM, 0);
}

void
rge_disable_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0);
	sc->rge_timerintr = 0;
}

void
rge_setup_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0x2600);
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
		rge_disable_hw_im(sc);
		break;
	case RGE_IMTYPE_SIM:
		rge_disable_hw_im(sc);
		rge_setup_sim_im(sc);
		break;
	default:
		panic("%s: unknown imtype %d", device_xname(sc->sc_dev), imtype);
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
	rge_write_mac_ocp(sc, 0xc0a6, 0x01b5);
	rge_write_mac_ocp(sc, 0xc01e, 0x5555);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		if (RGE_READ_2(sc, RGE_TWICMD) & 0x0200)
			break;
	}

	if (rge_read_mac_ocp(sc, 0xd42c) & 0x0100) {
		printf("%s: rge_exit_oob(): rtl8125_is_ups_resume!!\n",
		    device_xname(sc->sc_dev));
		for (i = 0; i < RGE_TIMEOUT; i++) {
			if ((rge_read_phy_ocp(sc, 0xa420) & 0x0007) == 2)
				break;
			DELAY(1000);
		}
		RGE_MAC_CLRBIT(sc, 0xd408, 0x0100);
		if (sc->rge_type == MAC_CFG4 || sc->rge_type == MAC_CFG5)
			RGE_PHY_CLRBIT(sc, 0xa466, 0x0001);
		RGE_PHY_CLRBIT(sc, 0xa468, 0x000a);
	}
}

void
rge_write_csi(struct rge_softc *sc, uint32_t reg, uint32_t val)
{
	int i;

	RGE_WRITE_4(sc, RGE_CSIDR, val);
	RGE_WRITE_4(sc, RGE_CSIAR, (reg & RGE_CSIAR_ADDR_MASK) |
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

	RGE_WRITE_4(sc, RGE_CSIAR, (reg & RGE_CSIAR_ADDR_MASK) |
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

uint16_t
rge_read_ephy(struct rge_softc *sc, uint16_t reg)
{
	uint32_t val;
	int i;

	val = (reg & RGE_EPHYAR_ADDR_MASK) << RGE_EPHYAR_ADDR_SHIFT;
	RGE_WRITE_4(sc, RGE_EPHYAR, val);

	for (i = 0; i < 10; i++) {
		DELAY(100);
		val = RGE_READ_4(sc, RGE_EPHYAR);
		if (val & RGE_EPHYAR_BUSY)
			break;
	}

	DELAY(20);

	return (val & RGE_EPHYAR_DATA_MASK);
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

uint16_t
rge_read_phy(struct rge_softc *sc, uint16_t addr, uint16_t reg)
{
	uint16_t off, phyaddr;

	phyaddr = addr ? addr : RGE_PHYBASE + (reg / 8);
	phyaddr <<= 4;

	off = addr ? reg : 0x10 + (reg % 8);

	phyaddr += (off - 16) << 1;

	return (rge_read_phy_ocp(sc, phyaddr));
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

	callout_schedule(&sc->sc_timeout, hz);
}

void
rge_link_state(struct rge_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int link = LINK_STATE_DOWN;

	if (rge_get_link_status(sc))
		link = LINK_STATE_UP;

	if (ifp->if_link_state != link) { /* XXX not safe to access */
		if_link_state_change(ifp, link);
	}
}
