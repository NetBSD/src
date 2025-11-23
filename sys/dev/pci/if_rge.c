/*	$NetBSD: if_rge.c,v 1.46 2025/11/23 18:43:31 pgoyette Exp $	*/
/*	$OpenBSD: if_rge.c,v 1.41 2025/11/17 08:59:22 jsg Exp $ */

/*
 * Copyright (c) 2019, 2020, 2023-2025
 *	Kevin Lo <kevlo@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: if_rge.c,v 1.46 2025/11/23 18:43:31 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_net_mpsafe.h"
#endif

#include <sys/types.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/callout.h>
#include <sys/workqueue.h>
#include <sys/module.h>

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

#ifdef __NetBSD__
#define letoh32 	htole32
#define nitems(x) 	__arraycount(x)
#endif

#include <dev/pci/if_rgereg.h>

#ifdef __NetBSD__
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

static int	rge_match(device_t, cfdata_t, void *);
static void	rge_attach(device_t, device_t, void *);
static int	rge_detach(device_t, int);
static int	rge_intr(void *);
static int	rge_encap(struct rge_softc *, struct rge_queues *,
    struct mbuf *, int);
static int	rge_ioctl(struct ifnet *, u_long, void *);
static void	rge_start(struct ifnet *);
static void	rge_watchdog(struct ifnet *);
static int	rge_init(struct ifnet *);
static void	rge_stop(struct ifnet *, int);
static int	rge_ifmedia_upd(struct ifnet *);
static void	rge_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static int	rge_allocmem(struct rge_softc *);
static int	rge_deallocmem(struct rge_softc *);
static int	rge_newbuf(struct rge_queues *, int);
static int	rge_rx_list_init(struct rge_queues *);
static void	rge_rx_list_fini(struct rge_queues *);
static void	rge_tx_list_init(struct rge_queues *);
static int	rge_rxeof(struct rge_softc *);
static int	rge_txeof(struct rge_softc *);
static int	rge_reset(struct rge_softc *);
static void	rge_iff(struct rge_softc *);
static void	rge_mac_config_mcu(struct rge_softc *, enum rge_mac_type);
static void	rge_mac_config_ext_mcu(struct rge_softc *, enum rge_mac_type);
static uint64_t	rge_mcu_get_bin_version(const uint16_t *, uint16_t);
static void	rge_mcu_set_version(struct rge_softc *, uint64_t);
static int	rge_chipinit(struct rge_softc *);
static void	rge_set_phy_power(struct rge_softc *, int);
static void	rge_ephy_config(struct rge_softc *);
static void	rge_ephy_config_mac_r25(struct rge_softc *);
static void	rge_ephy_config_mac_r25b(struct rge_softc *);
static void	rge_ephy_config_mac_r27(struct rge_softc *);
static int	rge_phy_config(struct rge_softc *);
static void	rge_phy_config_mac_r27(struct rge_softc *);
static void	rge_phy_config_mac_r26_1(struct rge_softc *);
static void	rge_phy_config_mac_r26_2(struct rge_softc *);
static void	rge_phy_config_mac_r25(struct rge_softc *);
static void	rge_phy_config_mac_r25b(struct rge_softc *);
static void	rge_phy_config_mac_r25d(struct rge_softc *);
static void	rge_phy_config_mcu(struct rge_softc *, uint16_t);
static void	rge_set_macaddr(struct rge_softc *, const uint8_t *);
static void	rge_get_macaddr(struct rge_softc *, uint8_t *);
static void	rge_hw_init(struct rge_softc *);
static void	rge_hw_reset(struct rge_softc *);
static void	rge_disable_phy_ocp_pwrsave(struct rge_softc *);
static void	rge_patch_phy_mcu(struct rge_softc *, int);
static void	rge_add_media_types(struct rge_softc *);
static void	rge_config_imtype(struct rge_softc *, int);
static void	rge_disable_aspm_clkreq(struct rge_softc *);
static void	rge_disable_hw_im(struct rge_softc *);
static void	rge_disable_sim_im(struct rge_softc *);
static void	rge_setup_sim_im(struct rge_softc *);
static void	rge_setup_intr(struct rge_softc *, int);
static void	rge_switch_mcu_ram_page(struct rge_softc *, int);
static int	rge_exit_oob(struct rge_softc *);
static void	rge_write_csi(struct rge_softc *, uint32_t, uint32_t);
static uint32_t	rge_read_csi(struct rge_softc *, uint32_t);
static void	rge_write_mac_ocp(struct rge_softc *, uint16_t, uint16_t);
static uint16_t	rge_read_mac_ocp(struct rge_softc *, uint16_t);
static void	rge_write_ephy(struct rge_softc *, uint16_t, uint16_t);
static uint16_t	rge_read_ephy(struct rge_softc *, uint16_t);
static uint16_t	rge_check_ephy_ext_add(struct rge_softc *, uint16_t);
static void	rge_r27_write_ephy(struct rge_softc *, uint16_t, uint16_t);
static void	rge_write_phy(struct rge_softc *, uint16_t, uint16_t, uint16_t);
static uint16_t	rge_read_phy(struct rge_softc *, uint16_t, uint16_t);
static void	rge_write_phy_ocp(struct rge_softc *, uint16_t, uint16_t);
static uint16_t	rge_read_phy_ocp(struct rge_softc *, uint16_t);
static int	rge_get_link_status(struct rge_softc *);
static void	rge_txstart(void *);
static void	rge_tick(void *);
static void	rge_link_state(struct rge_softc *);

static const struct {
	uint16_t reg;
	uint16_t val;
}  mac_r25_mcu[] = {
	MAC_R25_MCU
}, mac_r25b_mcu[] = {
	MAC_R25B_MCU
}, mac_r25d_mcu[] = {
	MAC_R25D_MCU};

CFATTACH_DECL_NEW(rge, sizeof(struct rge_softc), rge_match, rge_attach,
		rge_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .id = PCI_ID_CODE(PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_E3000) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8125) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8126) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_REALTEK, PCI_PRODUCT_REALTEK_RT8127) },

	PCI_COMPAT_EOL
};

static int
rge_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa =aux;

	return pci_compatible_match(pa, compat_data);
}

static void
rge_attach(device_t parent, device_t self, void *aux)
{
	struct rge_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr = NULL;
	struct ifnet *ifp;
	struct rge_queues *q;
	pcireg_t reg;
	uint32_t hwrev;
	uint8_t eaddr[ETHER_ADDR_LEN];
	int offset;
	pcireg_t command;
	const char *revstr;

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

	q = malloc(sizeof(struct rge_queues), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (q == NULL) {
		aprint_error(": unable to allocate queue memory\n");
		return;
	}
	q->q_sc = sc;
	q->q_index = 0;

	sc->sc_queues = q;
	sc->sc_nqueues = 1;

	/*
	 * Allocate interrupt.
	 */
	int counts[PCI_INTR_TYPE_SIZE] = {
 		[PCI_INTR_TYPE_INTX] = 1,
 		[PCI_INTR_TYPE_MSI] = 1,
 		[PCI_INTR_TYPE_MSIX] = 1,
 	};
	int max_type = PCI_INTR_TYPE_MSIX;

	if (pci_intr_alloc(pa, &sc->sc_intrs, counts, max_type) != 0) {
		aprint_error(": couldn't map interrupt\n");
		return;
	}
	switch (pci_intr_type(pc, sc->sc_intrs[0])) {
	case PCI_INTR_TYPE_MSIX:
	case PCI_INTR_TYPE_MSI:
		sc->rge_flags |= RGE_FLAG_MSI;
		break;
	default:
		break;
	}
	intrstr = pci_intr_string(pc, sc->sc_intrs[0],
	    intrbuf, sizeof(intrbuf));
	sc->sc_ihs[0] = pci_intr_establish_xname(pc, sc->sc_intrs[0],
	    IPL_NET, rge_intr, sc, device_xname(sc->sc_dev));

	if (sc->sc_ihs[0] == NULL) {
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
	case 0x60900000:
		sc->rge_type = MAC_R25;
		revstr = "A";
		break;
	case 0x64100000:
		sc->rge_type = MAC_R25B;
		revstr = "B";
		break;
	case 0x64900000:
		sc->rge_type = MAC_R26_1;
		revstr = "A";
		break;
	case 0x64a00000:
		sc->rge_type = MAC_R26_2;
		revstr = "B";
		break;
	case 0x68800000:
		sc->rge_type = MAC_R25D;
		revstr = "D";
		break;
	case 0x6c900000:
		sc->rge_type = MAC_R27;
		revstr = "A";
		break;
	default:
		aprint_error(": unknown version 0x%08x\n", hwrev);
		return;
	}

	aprint_normal_dev(sc->sc_dev, "HW rev. %s\n", revstr);
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

	ifp->if_capabilities = IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_IPv4_Tx |IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx|
	    IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx;

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
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, eaddr);

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
rge_detach(device_t self, int flags)
{      
	struct rge_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	pci_chipset_tag_t pc = sc->sc_pc;

	rge_stop(ifp, 1);

	pmf_device_deregister(self);

	ether_ifdetach(ifp);

	if_detach(ifp);

	ifmedia_fini(&sc->sc_media);

	if (sc->sc_ihs[0] != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[0]);
		sc->sc_ihs[0] = NULL;
	}           

	pci_intr_release(pc, sc->sc_intrs, 1);

	rge_deallocmem(sc);

	free(sc->sc_queues, M_DEVBUF);

	return 0;
}
static int
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

static inline void
rge_tx_list_sync(struct rge_softc *sc, struct rge_queues *q,
    unsigned int idx, unsigned int len, int ops)
{
	bus_dmamap_sync(sc->sc_dmat, q->q_tx.rge_tx_list_map,
	    idx * sizeof(struct rge_tx_desc), len * sizeof(struct rge_tx_desc),
	    ops);
}

static int
rge_encap(struct rge_softc *sc, struct rge_queues *q,struct mbuf *m, int idx)
{
	struct rge_tx_desc *d = NULL;
	struct rge_txq *txq;
	bus_dmamap_t txmap;
	uint32_t cmdsts, cflags = 0;
	int cur, error, i;

	txq = &q->q_tx.rge_txq[idx];
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

	/*
	 * Set RGE_TDEXTSTS_IPCSUM if any checksum offloading is requested.
	 * Otherwise, RGE_TDEXTSTS_TCPCSUM / RGE_TDEXTSTS_UDPCSUM does not
	 * take affect.
	 */
	if ((m->m_pkthdr.csum_flags &
	    (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0) {
		cflags |= RGE_TDEXTSTS_IPCSUM;
		if (m->m_pkthdr.csum_flags & M_CSUM_TCPv4)
			cflags |= RGE_TDEXTSTS_TCPCSUM;
		if (m->m_pkthdr.csum_flags & M_CSUM_UDPv4)
			cflags |= RGE_TDEXTSTS_UDPCSUM;
	}

	/* Set up hardware VLAN tagging. */
	if (vlan_has_tag(m))
		cflags |= bswap16(vlan_get_tag(m)) | RGE_TDEXTSTS_VTAG;

	cur = idx;
	for (i = 1; i < txmap->dm_nsegs; i++) {
		cur = RGE_NEXT_TX_DESC(cur);

		cmdsts = RGE_TDCMDSTS_OWN;
		cmdsts |= txmap->dm_segs[i].ds_len;

		if (cur == RGE_TX_LIST_CNT - 1)
			cmdsts |= RGE_TDCMDSTS_EOR;
		if (i == txmap->dm_nsegs - 1)
			cmdsts |= RGE_TDCMDSTS_EOF;

		d = &q->q_tx.rge_tx_list[cur];
		d->rge_cmdsts = htole32(cmdsts);
		d->rge_extsts = htole32(cflags);
		d->rge_addr = htole64(txmap->dm_segs[i].ds_addr);
	}

	/* Update info of TX queue and descriptors. */
	txq->txq_mbuf = m;
	txq->txq_descidx = cur;

	cmdsts = RGE_TDCMDSTS_SOF;
	cmdsts |= txmap->dm_segs[0].ds_len;

	if (idx == RGE_TX_LIST_CNT - 1)
		cmdsts |= RGE_TDCMDSTS_EOR;
	if (txmap->dm_nsegs == 1)
		cmdsts |= RGE_TDCMDSTS_EOF;

	d = &q->q_tx.rge_tx_list[idx];
	d->rge_cmdsts = htole32(cmdsts);
	d->rge_extsts = htole32(cflags);
	d->rge_addr = htole64(txmap->dm_segs[0].ds_addr);

	if (cur >= idx) {
		rge_tx_list_sync(sc, q, idx, txmap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	} else {
		rge_tx_list_sync(sc, q, idx, RGE_TX_LIST_CNT - idx,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		rge_tx_list_sync(sc, q, 0, cur + 1,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	}

	/* Transfer ownership of packet to the chip. */
	cmdsts |= RGE_TDCMDSTS_OWN;
	rge_tx_list_sync(sc, q, idx, 1, BUS_DMASYNC_POSTWRITE);
	d->rge_cmdsts = htole32(cmdsts);
	rge_tx_list_sync(sc, q, idx, 1, BUS_DMASYNC_PREWRITE);

	return (txmap->dm_nsegs);
}

static int
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

static void
rge_start(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	struct rge_queues *q = sc->sc_queues;
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
	idx = q->q_tx.rge_txq_prodidx;
	free = q->q_tx.rge_txq_considx;
	if (free <= idx)
		free += RGE_TX_LIST_CNT;
	free -= idx;

	for (;;) {
		if (free < RGE_TX_NSEGS + 2) {
			SET(ifp->if_flags, IFF_OACTIVE);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		used = rge_encap(sc, q, m, idx);
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

	q->q_tx.rge_txq_prodidx = idx;
	rge_txstart(sc);
}

static void
rge_watchdog(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;

	device_printf(sc->sc_dev, "watchdog timeout\n");
	if_statinc(ifp, if_oerrors);

	rge_init(ifp);
}

static int
rge_init(struct ifnet *ifp)
{
	struct rge_softc *sc = ifp->if_softc;
	struct rge_queues *q = sc->sc_queues;
	uint32_t rxconf, val;
	unsigned i, num_miti;
	int error = 0;

	rge_stop(ifp, 0);

	/* Set MAC address. */
	rge_set_macaddr(sc, CLLADDR(ifp->if_sadl));

	/* Initialize RX and TX descriptors lists. */
	rge_rx_list_init(q);
	rge_tx_list_init(q);

	if ((error = rge_chipinit(sc)))
		return error;

	if ((error = rge_phy_config(sc)))
		return error;

	RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);

	RGE_CLRBIT_1(sc, 0xf1, 0x80);
	rge_disable_aspm_clkreq(sc);
	RGE_WRITE_2(sc, RGE_EEE_TXIDLE_TIMER,
	    RGE_JUMBO_MTU + ETHER_HDR_LEN + 32);

	/* Load the addresses of the RX and TX lists into the chip. */
	RGE_WRITE_4(sc, RGE_RXDESC_ADDR_LO,
	    RGE_ADDR_LO(q->q_rx.rge_rx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_RXDESC_ADDR_HI,
	    RGE_ADDR_HI(q->q_rx.rge_rx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_TXDESC_ADDR_LO,
	    RGE_ADDR_LO(q->q_tx.rge_tx_list_map->dm_segs[0].ds_addr));
	RGE_WRITE_4(sc, RGE_TXDESC_ADDR_HI,
	    RGE_ADDR_HI(q->q_tx.rge_tx_list_map->dm_segs[0].ds_addr));

	/* Set the initial RX and TX configurations. */
	if (sc->rge_type == MAC_R25)
		rxconf = RGE_RXCFG_CONFIG;
	else if (sc->rge_type == MAC_R25B)
		rxconf = RGE_RXCFG_CONFIG_8125B;
	else if (sc->rge_type == MAC_R25D)
		rxconf = RGE_RXCFG_CONFIG_8125D;
	else
		rxconf = RGE_RXCFG_CONFIG_8126;
	RGE_WRITE_4(sc, RGE_RXCFG, rxconf);
	RGE_WRITE_4(sc, RGE_TXCFG, RGE_TXCFG_CONFIG);

	val = rge_read_csi(sc, 0x70c) & ~0x3f000000;
	rge_write_csi(sc, 0x70c, val | 0x27000000);

	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27) {
		/* Disable L1 timeout. */
		val = rge_read_csi(sc, 0x890) & ~0x00000001;
		rge_write_csi(sc, 0x890, val);
	} else if (sc->rge_type != MAC_R25D)
		RGE_WRITE_2(sc, 0x0382, 0x221b);

	RGE_WRITE_1(sc, RGE_RSS_CTRL, 0);
	RGE_WRITE_2(sc, RGE_RXQUEUE_CTRL, 0);
	RGE_CLRBIT_1(sc, RGE_CFG1, RGE_CFG1_SPEED_DOWN);

	rge_write_mac_ocp(sc, 0xc140, 0xffff);
	rge_write_mac_ocp(sc, 0xc142, 0xffff);

	RGE_MAC_SETBIT(sc, 0xeb58, 0x0001);

	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27) {
		RGE_CLRBIT_1(sc, 0xd8, 0x02);
		if (sc->rge_type == MAC_R27) {
			RGE_CLRBIT_1(sc, 0x20e4, 0x04);
			RGE_MAC_CLRBIT(sc, 0xe00c, 0x1000);
			RGE_MAC_CLRBIT(sc, 0xc0c2, 0x0040);
		}
	}

	val = rge_read_mac_ocp(sc, 0xe614);
	val &= (sc->rge_type == MAC_R27) ? ~0x0f00 : ~0x0700;
	if (sc->rge_type == MAC_R25 || sc->rge_type == MAC_R25D)
		rge_write_mac_ocp(sc, 0xe614, val | 0x0300);
	else if (sc->rge_type == MAC_R25B)
		rge_write_mac_ocp(sc, 0xe614, val | 0x0200);
	else if (RGE_TYPE_R26(sc))
		rge_write_mac_ocp(sc, 0xe614, val | 0x0300);
	else
		rge_write_mac_ocp(sc, 0xe614, val | 0x0f00);

	RGE_MAC_CLRBIT(sc, 0xe63e, 0x0c00);
 
	RGE_MAC_CLRBIT(sc, 0xe63e, 0x0030);

	val = rge_read_mac_ocp(sc, 0xe63e) & ~0x0030;
	rge_write_mac_ocp(sc, 0xe63e, val | 0x0020);

	RGE_MAC_CLRBIT(sc, 0xc0b4, 0x0001);
	RGE_MAC_SETBIT(sc, 0xc0b4, 0x0001);

	RGE_MAC_SETBIT(sc, 0xc0b4, 0x000c);

	val = rge_read_mac_ocp(sc, 0xeb6a) & ~0x00ff;
	rge_write_mac_ocp(sc, 0xeb6a, val | 0x0033);

	val = rge_read_mac_ocp(sc, 0xeb50) & ~0x03e0;
	rge_write_mac_ocp(sc, 0xeb50, val | 0x0040);

	RGE_MAC_CLRBIT(sc, 0xe056, 0x00f0);

	RGE_WRITE_1(sc, RGE_TDFNR, 0x10);

	RGE_MAC_CLRBIT(sc, 0xe040, 0x1000);

	val = rge_read_mac_ocp(sc, 0xea1c) & ~0x0003;
	rge_write_mac_ocp(sc, 0xea1c, val | 0x0001);

	if (sc->rge_type == MAC_R25D)
		rge_write_mac_ocp(sc, 0xe0c0, 0x4403);
	else
		rge_write_mac_ocp(sc, 0xe0c0, 0x4000);

	RGE_MAC_SETBIT(sc, 0xe052, 0x0060);
	RGE_MAC_CLRBIT(sc, 0xe052, 0x0088);

	val = rge_read_mac_ocp(sc, 0xd430) & ~0x0fff;
	rge_write_mac_ocp(sc, 0xd430, val | 0x045f);

	RGE_SETBIT_1(sc, RGE_DLLPR, RGE_DLLPR_PFM_EN | RGE_DLLPR_TX_10M_PS_EN);

	if (sc->rge_type == MAC_R25)
		RGE_SETBIT_1(sc, RGE_MCUCMD, 0x01);

	if (sc->rge_type != MAC_R25D) {
		/* Disable EEE plus. */
		RGE_MAC_CLRBIT(sc, 0xe080, 0x0002);
	}

	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27)
		RGE_MAC_CLRBIT(sc, 0xea1c, 0x0304);
	else
		RGE_MAC_CLRBIT(sc, 0xea1c, 0x0004);

	/* Clear tcam entries. */
	RGE_MAC_SETBIT(sc, 0xeb54, 0x0001);
	DELAY(1);
	RGE_MAC_CLRBIT(sc, 0xeb54, 0x0001);

	RGE_CLRBIT_2(sc, 0x1880, 0x0030);

	if (sc->rge_type == MAC_R27) {
		val = rge_read_mac_ocp(sc, 0xd40c) & ~0xe038;
		rge_write_phy_ocp(sc, 0xd40c, val | 0x8020);
	}

	/* Config interrupt type. */
	if (sc->rge_type == MAC_R27)
		RGE_CLRBIT_1(sc, RGE_INT_CFG0, RGE_INT_CFG0_AVOID_MISS_INTR);
	else if (sc->rge_type != MAC_R25)
		RGE_CLRBIT_1(sc, RGE_INT_CFG0, RGE_INT_CFG0_EN);

	/* Clear timer interrupts. */
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT1, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT2, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT3, 0);

	num_miti =
	    (sc->rge_type == MAC_R25B || RGE_TYPE_R26(sc)) ? 32 : 64;
	/* Clear interrupt moderation timer. */
	for (i = 0; i < num_miti; i++)
		RGE_WRITE_4(sc, RGE_INTMITI(i), 0);

	if (RGE_TYPE_R26(sc)) {
		RGE_CLRBIT_1(sc, RGE_INT_CFG0,
		    RGE_INT_CFG0_TIMEOUT_BYPASS | RGE_INT_CFG0_RDU_BYPASS_8126 |
		    RGE_INT_CFG0_MITIGATION_BYPASS);
		RGE_WRITE_2(sc, RGE_INT_CFG1, 0);
	}

	RGE_MAC_SETBIT(sc, 0xc0ac, 0x1f80);

	rge_write_mac_ocp(sc, 0xe098, 0xc302);

	RGE_MAC_CLRBIT(sc, 0xe032, 0x0003);
	val = rge_read_csi(sc, 0x98) & ~0x0000ff00;
	rge_write_csi(sc, 0x98, val);

	if (sc->rge_type == MAC_R25D) {
		val = rge_read_mac_ocp(sc, 0xe092) & ~0x00ff;
		rge_write_mac_ocp(sc, 0xe092, val | 0x0008);
	} else
		RGE_MAC_CLRBIT(sc, 0xe092, 0x00ff);

	if (ifp->if_capabilities & ETHERCAP_VLAN_HWTAGGING)
		RGE_SETBIT_4(sc, RGE_RXCFG, RGE_RXCFG_VLANSTRIP);

	RGE_SETBIT_2(sc, RGE_CPLUSCMD, RGE_CPLUSCMD_RXCSUM);
	RGE_READ_2(sc, RGE_CPLUSCMD);

	/* Set Maximum frame size. */
	RGE_WRITE_2(sc, RGE_RXMAXSIZE, RGE_JUMBO_FRAMELEN);

	/* Disable RXDV gate. */
	RGE_CLRBIT_1(sc, RGE_PPSW, 0x08);
	DELAY(2000);

	/* Program promiscuous mode and multicast filters. */
	rge_iff(sc);

	if (sc->rge_type == MAC_R27)
		RGE_CLRBIT_1(sc, RGE_RADMFIFO_PROTECT, 0x2001);

	rge_disable_aspm_clkreq(sc);

	RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
	DELAY(10);

	rge_ifmedia_upd(ifp);

	/* Enable transmit and receive. */
	RGE_WRITE_1(sc, RGE_CMD, RGE_CMD_TXENB | RGE_CMD_RXENB);

	/* Enable interrupts. */
	rge_setup_intr(sc, RGE_IMTYPE_SIM);

	ifp->if_flags |= IFF_RUNNING;
	CLR(ifp->if_flags, IFF_OACTIVE);

	callout_schedule(&sc->sc_timeout, 1);

	return 0;
}

/*
 * Stop the adapter and free any mbufs allocated to the RX and TX lists.
 */
static void
rge_stop(struct ifnet *ifp, int disable)
{
	struct rge_softc *sc = ifp->if_softc;
	struct rge_queues *q = sc->sc_queues;
	unsigned i;

	callout_halt(&sc->sc_timeout, NULL);

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_RUNNING;
	sc->rge_timerintr = 0;

	RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_ALLPHYS | RGE_RXCFG_INDIV |
	    RGE_RXCFG_MULTI | RGE_RXCFG_BROAD | RGE_RXCFG_RUNT |
	    RGE_RXCFG_ERRPKT);

	rge_hw_reset(sc);

	RGE_MAC_CLRBIT(sc, 0xc0ac, 0x1f80);

//	intr_barrier(sc->sc_ih);
//	ifq_barrier(&ifp->if_snd);
/*	ifq_clr_oactive(&ifp->if_snd); Sevan - OpenBSD queue API */

	if (q->q_rx.rge_head != NULL) {
		m_freem(q->q_rx.rge_head);
		q->q_rx.rge_head = NULL;
		q->q_rx.rge_tail = &q->q_rx.rge_head;
	}

	/* Free the TX list buffers. */
	for (i = 0; i < RGE_TX_LIST_CNT; i++) {
		if (q->q_tx.rge_txq[i].txq_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    q->q_tx.rge_txq[i].txq_dmamap);
			m_freem(q->q_tx.rge_txq[i].txq_mbuf);
			q->q_tx.rge_txq[i].txq_mbuf = NULL;
		}
	}

	/* Free the RX list buffers. */
	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		if (q->q_rx.rge_rxq[i].rxq_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    q->q_rx.rge_rxq[i].rxq_dmamap);
			m_freem(q->q_rx.rge_rxq[i].rxq_mbuf);
			q->q_rx.rge_rxq[i].rxq_mbuf = NULL;
		}
	}
}

/*
 * Set media options.
 */
static int
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
	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27)
		RGE_PHY_CLRBIT(sc, 0xa5ea, 0x0007);

	val = rge_read_phy_ocp(sc, 0xa5d4);
	switch (sc->rge_type) {
	case MAC_R27:
		val &= ~RGE_ADV_10000TFDX;
                /* fallthrough */
	case MAC_R26_1:
	case MAC_R26_2:
		val &= ~RGE_ADV_5000TFDX;
                /* fallthrough */
        default:
                val &= ~RGE_ADV_2500TFDX;
                break;
        }

	anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
	gig = GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		val |= RGE_ADV_2500TFDX;
		if (RGE_TYPE_R26(sc))
			val |= RGE_ADV_5000TFDX;
		else if (sc->rge_type == MAC_R27)
			val |= RGE_ADV_5000TFDX | RGE_ADV_10000TFDX;
		break;
	case IFM_10G_T:
		val |= RGE_ADV_10000TFDX;
		ifp->if_baudrate = IF_Gbps(10);
		break;
	case IFM_5000_T:
		val |= RGE_ADV_5000TFDX;
		ifp->if_baudrate = IF_Gbps(5);
		break;
	case IFM_2500_T:
		val |= RGE_ADV_2500TFDX;
		ifp->if_baudrate = IF_Mbps(2500);
		break;
	case IFM_1000_T:
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
static void
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
		    (status & (RGE_PHYSTAT_1000MBPS | RGE_PHYSTAT_2500MBPS |
		    RGE_PHYSTAT_5000MBPS | RGE_PHYSTAT_10000MBPS)))
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
		else if (status & RGE_PHYSTAT_5000MBPS)
			ifmr->ifm_active |= IFM_5000_T;
		else if (status & RGE_PHYSTAT_10000MBPS)
			ifmr->ifm_active |= IFM_10G_T;
	}
}

/*
 * Allocate memory for RX/TX rings.
 *
 * XXX There is no tear-down for this if it any part fails, so everything
 * remains allocated.
 */
static int
rge_allocmem(struct rge_softc *sc)
{
	struct rge_queues *q = sc->sc_queues;
	int error, i;

	/* Allocate DMA'able memory for the TX ring. */
	error = bus_dmamap_create(sc->sc_dmat, RGE_TX_LIST_SZ, 1,
	    RGE_TX_LIST_SZ, 0, BUS_DMA_NOWAIT, &q->q_tx.rge_tx_list_map);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't create TX list map\n");
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_TX_LIST_SZ, RGE_ALIGN, 0,
	    &q->q_tx.rge_tx_listseg, 1, &q->q_tx.rge_tx_listnseg,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc TX list\n");
		return (error);
	}

	/* Load the map for the TX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &q->q_tx.rge_tx_listseg,
	    q->q_tx.rge_tx_listnseg, RGE_TX_LIST_SZ,
	    (void **) &q->q_tx.rge_tx_list,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't map TX dma buffers\n");
		bus_dmamem_free(sc->sc_dmat, &q->q_tx.rge_tx_listseg,
		    q->q_tx.rge_tx_listnseg);
		return (error);
	}
	memset(q->q_tx.rge_tx_list, 0, RGE_TX_LIST_SZ);
	error = bus_dmamap_load(sc->sc_dmat, q->q_tx.rge_tx_list_map,
	    q->q_tx.rge_tx_list, RGE_TX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't load TX dma map\n");
		bus_dmamap_destroy(sc->sc_dmat, q->q_tx.rge_tx_list_map);
		bus_dmamem_unmap(sc->sc_dmat,
		    q->q_tx.rge_tx_list, RGE_TX_LIST_SZ);
		bus_dmamem_free(sc->sc_dmat, &q->q_tx.rge_tx_listseg,
		    q->q_tx.rge_tx_listnseg);
		return (error);
	}

	/* Create DMA maps for TX buffers. */
	for (i = 0; i < RGE_TX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, RGE_JUMBO_FRAMELEN,
		    RGE_TX_NSEGS, RGE_JUMBO_FRAMELEN, 0, 0,
		    &q->q_tx.rge_txq[i].txq_dmamap);
		if (error) {
			aprint_error_dev(sc->sc_dev, "can't create DMA map for TX\n");
			return (error);
		}
	}

	/* Allocate DMA'able memory for the RX ring. */
	error = bus_dmamap_create(sc->sc_dmat, RGE_RX_LIST_SZ, 1,
	    RGE_RX_LIST_SZ, 0, 0, &q->q_rx.rge_rx_list_map);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't create RX list map\n");
		return (error);
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RGE_RX_LIST_SZ, RGE_ALIGN, 0,
	    &q->q_rx.rge_rx_listseg, 1, &q->q_rx.rge_rx_listnseg,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't alloc RX list\n");
		return (error);
	}

	/* Load the map for the RX ring. */
	error = bus_dmamem_map(sc->sc_dmat, &q->q_rx.rge_rx_listseg,
	    q->q_rx.rge_rx_listnseg, RGE_RX_LIST_SZ,
	    (void **) &q->q_rx.rge_rx_list,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't map RX dma buffers\n");
		bus_dmamem_free(sc->sc_dmat, &q->q_rx.rge_rx_listseg,
		    q->q_rx.rge_rx_listnseg);
		return (error);
	}
	memset(q->q_rx.rge_rx_list, 0, RGE_RX_LIST_SZ);
	error = bus_dmamap_load(sc->sc_dmat, q->q_rx.rge_rx_list_map,
	    q->q_rx.rge_rx_list, RGE_RX_LIST_SZ, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't load RX dma map\n");
		bus_dmamap_destroy(sc->sc_dmat, q->q_rx.rge_rx_list_map);
		bus_dmamem_unmap(sc->sc_dmat,
		    q->q_rx.rge_rx_list, RGE_RX_LIST_SZ);
		bus_dmamem_free(sc->sc_dmat, &q->q_rx.rge_rx_listseg,
		    q->q_rx.rge_rx_listnseg);
		return (error);
	}

	/*
	 * Create DMA maps for RX buffers.  Use BUS_DMA_ALLOCNOW to avoid any
	 * potential failure in bus_dmamap_load_mbuf() in the RX path.
	 */
	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, RGE_JUMBO_FRAMELEN, 1,
		    RGE_JUMBO_FRAMELEN, 0, BUS_DMA_ALLOCNOW,
		    &q->q_rx.rge_rxq[i].rxq_dmamap);
		if (error) {
			aprint_error_dev(sc->sc_dev, "can't create DMA map for RX\n");
			return (error);
		}
	}

	return (error);
}

/*
 * release memory allocated to RX/TX rings
 */
static int
rge_deallocmem(struct rge_softc *sc)
{
	struct rge_queues *q = sc->sc_queues;
	int i;

	/* Destroy DMA maps for RX buffers */
	for (i = 0; i < RGE_RX_LIST_CNT; i++)
		bus_dmamap_destroy(sc->sc_dmat,
		    q->q_rx.rge_rxq[i].rxq_dmamap);

	/* Unload the map for the RX ring */
	bus_dmamap_unload(sc->sc_dmat, q->q_rx.rge_rx_list_map);
	bus_dmamem_unmap(sc->sc_dmat,
	    q->q_rx.rge_rx_list, RGE_RX_LIST_SZ);

	/* Deallocate DMA'able memory for the RX ring. */
	bus_dmamem_free(sc->sc_dmat, &q->q_rx.rge_rx_listseg,
	    q->q_rx.rge_rx_listnseg);
	bus_dmamap_destroy(sc->sc_dmat, q->q_rx.rge_rx_list_map);

	/* Destroy DMA maps for TX buffers. */
	for (i = 0; i < RGE_TX_LIST_CNT; i++)
		bus_dmamap_destroy(sc->sc_dmat,
		    q->q_tx.rge_txq[i].txq_dmamap);

	/* Unload the map for the TX ring */
	bus_dmamap_unload(sc->sc_dmat, q->q_tx.rge_tx_list_map);
	bus_dmamem_unmap(sc->sc_dmat,
	    q->q_tx.rge_tx_list_map, RGE_TX_LIST_SZ);

	/* Deallocate DMA'able memory for the TX ring. */
	bus_dmamem_free(sc->sc_dmat, &q->q_tx.rge_tx_listseg,
	    q->q_tx.rge_tx_listnseg);
	bus_dmamap_destroy(sc->sc_dmat, q->q_tx.rge_tx_list_map);

	return 0;
}

/*
 * Set an RX descriptor and sync it.
 */
static void
rge_load_rxbuf(struct rge_queues *q, int idx)
{
	struct rge_softc *sc = q->q_sc;
	struct rge_rx_desc *r = &q->q_rx.rge_rx_list[idx];
	struct rge_rxq *rxq = &q->q_rx.rge_rxq[idx];
	bus_dmamap_t rxmap = rxq->rxq_dmamap;
	uint32_t cmdsts;

	cmdsts = rxmap->dm_segs[0].ds_len | RGE_RDCMDSTS_OWN;
	if (idx == RGE_RX_LIST_CNT - 1)
		cmdsts |= RGE_RDCMDSTS_EOR;

	r->hi_qword0.rge_addr = htole64(rxmap->dm_segs[0].ds_addr);
	r->hi_qword1.rx_qword4.rge_extsts = 0;
	r->hi_qword1.rx_qword4.rge_cmdsts = htole32(cmdsts);

	bus_dmamap_sync(sc->sc_dmat, q->q_rx.rge_rx_list_map,
	    idx * sizeof(struct rge_rx_desc), sizeof(struct rge_rx_desc),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

/*
 * Initialize the RX descriptor and attach an mbuf cluster.
 */
static int
rge_newbuf(struct rge_queues *q, int idx)
{
	struct rge_softc *sc = q->q_sc;
	struct mbuf *m;
	struct rge_rxq *rxq;
	bus_dmamap_t rxmap;
	int error __diagused;

	m = MCLGETL(NULL, M_DONTWAIT, RGE_JUMBO_FRAMELEN);
	if (m == NULL)
		return (ENOBUFS);
	MCLAIM(m, &sc->sc_ec.ec_rx_mowner);

	m->m_len = m->m_pkthdr.len = RGE_JUMBO_FRAMELEN;

	rxq = &q->q_rx.rge_rxq[idx];
	rxmap = rxq->rxq_dmamap;

	if (rxq->rxq_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxq->rxq_dmamap);

	/* This map was created with BUS_DMA_ALLOCNOW so should never fail. */
	error = bus_dmamap_load_mbuf(sc->sc_dmat, rxmap, m, BUS_DMA_NOWAIT);
	KASSERTMSG(error == 0, "error=%d", error);

	bus_dmamap_sync(sc->sc_dmat, rxmap, 0, rxmap->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	/* Map the segments into RX descriptors. */

	rxq->rxq_mbuf = m;
	rge_load_rxbuf(q, idx);

	return 0;
}

static int
rge_rx_list_init(struct rge_queues *q)
{
	unsigned i;

	memset(q->q_rx.rge_rx_list, 0, RGE_RX_LIST_SZ);

	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		q->q_rx.rge_rxq[i].rxq_mbuf = NULL;
		if (rge_newbuf(q, i) != 0) {
			rge_rx_list_fini(q);
			return (ENOBUFS);
		}
	}

	q->q_rx.rge_rxq_prodidx = q->q_rx.rge_rxq_considx = 0;
	q->q_rx.rge_head = NULL;
	q->q_rx.rge_tail = &q->q_rx.rge_head;;

	return (0);
}

static void
rge_rx_list_fini(struct rge_queues *q)
{
	struct rge_softc *sc = q->q_sc;
	unsigned i;

	/* Free the RX list buffers. */
	for (i = 0; i < RGE_RX_LIST_CNT; i++) {
		if (q->q_rx.rge_rxq[i].rxq_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    q->q_rx.rge_rxq[i].rxq_dmamap);
			m_freem(q->q_rx.rge_rxq[i].rxq_mbuf);
			q->q_rx.rge_rxq[i].rxq_mbuf = NULL;
		}
	}
}

static void
rge_tx_list_init(struct rge_queues *q)
{
	struct rge_softc *sc = q->q_sc;
	unsigned i;

	memset(q->q_tx.rge_tx_list, 0, RGE_TX_LIST_SZ);

	for (i = 0; i < RGE_TX_LIST_CNT; i++)
		q->q_tx.rge_txq[i].txq_mbuf = NULL;

	bus_dmamap_sync(sc->sc_dmat, q->q_tx.rge_tx_list_map, 0,
	    q->q_tx.rge_tx_list_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	q->q_tx.rge_txq_prodidx = q->q_tx.rge_txq_considx = 0;
}

static int
rge_rxeof(struct rge_softc *sc)
{
	struct mbuf *m;
	struct rge_queues *q = sc->sc_queues;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rge_rx_desc *cur_rx;
	struct rge_rxq *rxq;
	uint32_t rxstat, extsts;
	int i, total_len, rx = 0;

	for (i = q->q_rx.rge_rxq_considx; ; i = RGE_NEXT_RX_DESC(i)) {
		/* Invalidate the descriptor memory. */
		bus_dmamap_sync(sc->sc_dmat, q->q_rx.rge_rx_list_map,
		    i * sizeof(struct rge_rx_desc), sizeof(struct rge_rx_desc),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur_rx = &q->q_rx.rge_rx_list[i];

		if (RGE_OWN(cur_rx))
			break;

		rxstat = letoh32(cur_rx->hi_qword1.rx_qword4.rge_cmdsts);
		extsts = letoh32(cur_rx->hi_qword1.rx_qword4.rge_extsts);

		total_len = RGE_RXBYTES(cur_rx);
		rxq = &q->q_rx.rge_rxq[i];
		m = rxq->rxq_mbuf;
		rx = 1;

		/* Invalidate the RX mbuf. */
		bus_dmamap_sync(sc->sc_dmat, rxq->rxq_dmamap, 0,
		    rxq->rxq_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		if ((rxstat & (RGE_RDCMDSTS_SOF | RGE_RDCMDSTS_EOF)) !=
		    (RGE_RDCMDSTS_SOF | RGE_RDCMDSTS_EOF)) {
			if_statinc(ifp, if_ierrors);
			rge_load_rxbuf(q, i);
			continue;
		}

		if (rxstat & RGE_RDCMDSTS_RXERRSUM) {
			if_statinc(ifp, if_ierrors);
			/*
			 * If this is part of a multi-fragment packet,
			 * discard all the pieces.
			 */
			if (q->q_rx.rge_head != NULL) {
				m_freem(q->q_rx.rge_head);
				q->q_rx.rge_head = NULL;
				q->q_rx.rge_tail = &q->q_rx.rge_head;
			}
			rge_load_rxbuf(q, i);
			continue;
		}

		/*
		 * If allocating a replacement mbuf fails,
		 * reload the current one.
		 */
		if (rge_newbuf(q, i) != 0) {
			if_statinc(ifp, if_iqdrops);
			if (q->q_rx.rge_head != NULL) {
				m_freem(q->q_rx.rge_head);
				q->q_rx.rge_head = NULL;
				q->q_rx.rge_tail = &q->q_rx.rge_head;
			}
			rge_load_rxbuf(q, i);
			continue;
		}

		m_set_rcvif(m, ifp);
		if (q->q_rx.rge_head != NULL) {
			m->m_len = total_len;
			/*
			 * Special case: if there's 4 bytes or less
			 * in this buffer, the mbuf can be discarded:
			 * the last 4 bytes is the CRC, which we don't
			 * care about anyway.
			 */
			if (m->m_len <= ETHER_CRC_LEN) {
				(*q->q_rx.rge_tail)->m_len -=
				    (ETHER_CRC_LEN - m->m_len);
				m_freem(m);
			} else {
				m->m_len -= ETHER_CRC_LEN;
				m->m_flags &= ~M_PKTHDR;
				(*q->q_rx.rge_tail)->m_next = m;
			}
			m = q->q_rx.rge_head;
			q->q_rx.rge_head = NULL;
			q->q_rx.rge_tail = &q->q_rx.rge_head;
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

		/* Check IP header checksum. */
		if ((extsts & RGE_RDEXTSTS_IPCSUMERR) &&
		    (extsts & RGE_RDEXTSTS_IPV4))
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;

		/* Check TCP/UDP checksum. */

		if ((extsts & (RGE_RDEXTSTS_IPV4 | RGE_RDEXTSTS_IPV6)) &&
		    (((extsts & RGE_RDEXTSTS_TCPPKT) &&
		    (extsts & RGE_RDEXTSTS_TCPCSUMERR)) ||
		    ((extsts & RGE_RDEXTSTS_UDPPKT) &&
		    (extsts & RGE_RDEXTSTS_UDPCSUMERR))))
			m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;

		if (extsts & RGE_RDEXTSTS_VTAG) {
			vlan_set_tag(m,
			    bswap16(extsts & RGE_RDEXTSTS_VLAN_MASK));
		}

		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

	q->q_rx.rge_rxq_considx = i;

	return (rx);
}

static int
rge_txeof(struct rge_softc *sc)
{
	struct rge_queues *q = sc->sc_queues;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rge_txq *txq;
	uint32_t txstat;
	int cons, idx, prod;
	int free = 0;

	prod = q->q_tx.rge_txq_prodidx;
	cons = q->q_tx.rge_txq_considx;

	while (prod != cons) {
		txq = &q->q_tx.rge_txq[cons];
		idx = txq->txq_descidx;

		bus_dmamap_sync(sc->sc_dmat, q->q_tx.rge_tx_list_map,
		    idx * sizeof(struct rge_tx_desc),
		    sizeof(struct rge_tx_desc),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		txstat = letoh32(q->q_tx.rge_tx_list[idx].rge_cmdsts);

		if (txstat & RGE_TDCMDSTS_OWN) {
			free = 2;
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, txq->txq_dmamap, 0,
		    txq->txq_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txq->txq_dmamap);
		m_freem(txq->txq_mbuf);
		txq->txq_mbuf = NULL;

		net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
		if (txstat & (RGE_TDCMDSTS_EXCESSCOLL | RGE_TDCMDSTS_COLL))
			if_statinc_ref(ifp, nsr, if_collisions);
		if (txstat & RGE_TDCMDSTS_TXERR)
			if_statinc_ref(ifp, nsr, if_oerrors);
		else
			if_statinc_ref(ifp, nsr, if_opackets);
		IF_STAT_PUTREF(ifp);

		bus_dmamap_sync(sc->sc_dmat, q->q_tx.rge_tx_list_map,
		    idx * sizeof(struct rge_tx_desc),
		    sizeof(struct rge_tx_desc),
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		cons = RGE_NEXT_TX_DESC(idx);
		free = 1;
	}

	if (free == 0)
		return (0);

	q->q_tx.rge_txq_considx = cons;

	if (free == 2)
		rge_txstart(sc);

	CLR(ifp->if_flags, IFF_OACTIVE);
	ifp->if_timer = 0;
	if_schedule_deferred_start(ifp);

	return (1);
}

static int
rge_reset(struct rge_softc *sc)
{
	int i;

	RGE_CLRBIT_4(sc, RGE_RXCFG, RGE_RXCFG_ALLPHYS | RGE_RXCFG_INDIV |
	    RGE_RXCFG_MULTI | RGE_RXCFG_BROAD | RGE_RXCFG_RUNT |
	    RGE_RXCFG_ERRPKT);

	/* Enable RXDV gate. */
	RGE_SETBIT_1(sc, RGE_PPSW, 0x08);

	RGE_SETBIT_1(sc, RGE_CMD, RGE_CMD_STOPREQ);
	DELAY(200);

	for (i = 0; i < 3000; i++) {
		DELAY(50);
		if ((RGE_READ_1(sc, RGE_MCUCMD) & (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY)) == (RGE_MCUCMD_RXFIFO_EMPTY |
		    RGE_MCUCMD_TXFIFO_EMPTY))
			break;
	}
	if (sc->rge_type != MAC_R25) {
		for (i = 0; i < 3000; i++) {
			DELAY(50);
			if ((RGE_READ_2(sc, RGE_IM) & 0x0103) == 0x0103)
				break;
		}
	}

	RGE_WRITE_1(sc, RGE_CMD,
	    RGE_READ_1(sc, RGE_CMD) & (RGE_CMD_TXENB | RGE_CMD_RXENB));

	/* Soft reset. */
	RGE_WRITE_1(sc, RGE_CMD, RGE_CMD_RESET);

	for (i = 0; i < RGE_TIMEOUT; i++) {
		DELAY(100);
		if (!(RGE_READ_1(sc, RGE_CMD) & RGE_CMD_RESET))
			break;
	}
	if (i == RGE_TIMEOUT) {
		device_printf(sc->sc_dev, "reset never completed!\n");
		return ETIMEDOUT;
	}

	return 0;
}

static void
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
				hashes[0] |= (1U << h);
			else
				hashes[1] |= (1U << (h - 32));

			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
	}

	RGE_WRITE_4(sc, RGE_RXCFG, rxfilt);
	RGE_WRITE_4(sc, RGE_MAR0, bswap32(hashes[1]));
	RGE_WRITE_4(sc, RGE_MAR4, bswap32(hashes[0]));
}

static int
rge_chipinit(struct rge_softc *sc)
{
	int error;

	if ((error = rge_exit_oob(sc)) != 0)
		return error;
	rge_set_phy_power(sc, 1);
	rge_hw_init(sc);
	rge_hw_reset(sc);

	return 0;
}

static void
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

static void
rge_mac_config_mcu(struct rge_softc *sc, enum rge_mac_type type)
{
	uint16_t reg;
	int i, npages;

	if (type == MAC_R25) {
		for (npages = 0; npages < 3; npages++) {
			rge_switch_mcu_ram_page(sc, npages);
			for (i = 0; i < nitems(rtl8125_mac_bps); i++) {
				if (npages == 0)
					rge_write_mac_ocp(sc,
					    rtl8125_mac_bps[i].reg,
					    rtl8125_mac_bps[i].val);
				else if (npages == 1)
					rge_write_mac_ocp(sc,
					    rtl8125_mac_bps[i].reg, 0);
				else {
					if (rtl8125_mac_bps[i].reg < 0xf9f8)
						rge_write_mac_ocp(sc,
						    rtl8125_mac_bps[i].reg, 0);
				}
			}
			if (npages == 2) {
				rge_write_mac_ocp(sc, 0xf9f8, 0x6486);
				rge_write_mac_ocp(sc, 0xf9fa, 0x0b15);
				rge_write_mac_ocp(sc, 0xf9fc, 0x090e);
				rge_write_mac_ocp(sc, 0xf9fe, 0x1139);
			}
		}
		rge_write_mac_ocp(sc, 0xfc26, 0x8000);
		rge_write_mac_ocp(sc, 0xfc2a, 0x0540);
		rge_write_mac_ocp(sc, 0xfc2e, 0x0a06);
		rge_write_mac_ocp(sc, 0xfc30, 0x0eb8);
		rge_write_mac_ocp(sc, 0xfc32, 0x3a5c);
		rge_write_mac_ocp(sc, 0xfc34, 0x10a8);
		rge_write_mac_ocp(sc, 0xfc40, 0x0d54);
		rge_write_mac_ocp(sc, 0xfc42, 0x0e24);
		rge_write_mac_ocp(sc, 0xfc48, 0x307a);
	} else if (type == MAC_R25B) {
		rge_switch_mcu_ram_page(sc, 0);
		for (i = 0; i < nitems(rtl8125b_mac_bps); i++) {
			rge_write_mac_ocp(sc, rtl8125b_mac_bps[i].reg,
			    rtl8125b_mac_bps[i].val);
		}
	} else if (type == MAC_R25D) {
		for (npages = 0; npages < 3; npages++) {
			rge_switch_mcu_ram_page(sc, npages);

			rge_write_mac_ocp(sc, 0xf800,
			    (npages == 0) ? 0xe002 : 0);
			rge_write_mac_ocp(sc, 0xf802,
			    (npages == 0) ? 0xe006 : 0);
			rge_write_mac_ocp(sc, 0xf804,
			    (npages == 0) ? 0x4166 : 0);
			rge_write_mac_ocp(sc, 0xf806,
			    (npages == 0) ? 0x9cf6 : 0);
			rge_write_mac_ocp(sc, 0xf808,
			    (npages == 0) ? 0xc002 : 0);
			rge_write_mac_ocp(sc, 0xf80a,
			    (npages == 0) ? 0xb800 : 0);
			rge_write_mac_ocp(sc, 0xf80c,
			    (npages == 0) ? 0x14a4 : 0);
			rge_write_mac_ocp(sc, 0xf80e,
			    (npages == 0) ? 0xc102 : 0);
			rge_write_mac_ocp(sc, 0xf810,
			    (npages == 0) ? 0xb900 : 0);

			for (reg = 0xf812; reg <= 0xf9f6; reg += 2)
				rge_write_mac_ocp(sc, reg, 0);

			rge_write_mac_ocp(sc, 0xf9f8,
			    (npages == 2) ? 0x6938 : 0);
			rge_write_mac_ocp(sc, 0xf9fa,
			    (npages == 2) ? 0x0a18 : 0);
			rge_write_mac_ocp(sc, 0xf9fc,
			    (npages == 2) ? 0x0217 : 0);
			rge_write_mac_ocp(sc, 0xf9fe,
			    (npages == 2) ? 0x0d2a : 0);
		}
		rge_write_mac_ocp(sc, 0xfc26, 0x8000);
		rge_write_mac_ocp(sc, 0xfc28, 0x14a2);
		rge_write_mac_ocp(sc, 0xfc48, 0x0001);
	}
}

static void
rge_mac_config_ext_mcu(struct rge_softc *sc, enum rge_mac_type type)
{
	const struct rge_mac_bps *bps;
	uint64_t mcodever = 0;
	int i;

	/* Read microcode version. */
	rge_switch_mcu_ram_page(sc, 2);
	sc->rge_mcodever = 0;
	for (i = 0; i < 8; i += 2) {
		sc->rge_mcodever <<= 16;
		sc->rge_mcodever |= rge_read_mac_ocp(sc, 0xf9f8 + i);
	}
	rge_switch_mcu_ram_page(sc, 0);

	if (type == MAC_R26_1) {
		bps = &rtl8126_1_mac_bps;
		mcodever =
		    rge_mcu_get_bin_version(rtl8126_1_mac_bps_vals, bps->count);
		if (sc->rge_mcodever != mcodever) {
			/* Switch to page 0. */
			rge_switch_mcu_ram_page(sc, 0);
			for (i = 0; i < bps->count; i++)
				rge_write_mac_ocp(sc, bps->regs[i],
				    bps->vals[i]);
		}
		rge_write_mac_ocp(sc, 0xfc26, 0x8000);
		rge_write_mac_ocp(sc, 0xfc2c, 0x2360);
		rge_write_mac_ocp(sc, 0xfc2E, 0x14a4);
		rge_write_mac_ocp(sc, 0xfc30, 0x415e);
		rge_write_mac_ocp(sc, 0xfc32, 0x41e4);
		rge_write_mac_ocp(sc, 0xfc34, 0x4280);
		rge_write_mac_ocp(sc, 0xfc36, 0x234a);
		rge_write_mac_ocp(sc, 0xfc48, 0x00fc);
	} else if (type == MAC_R26_2) {
		bps = &rtl8126_2_mac_bps;
		mcodever =
		    rge_mcu_get_bin_version(rtl8126_2_mac_bps_vals, bps->count);
		if (sc->rge_mcodever != mcodever) {
			/* Switch to page 0. */
			rge_switch_mcu_ram_page(sc, 0);
			for (i = 0; i < 256; i++)
				rge_write_mac_ocp(sc, bps->regs[i],
				    bps->vals[i]);
		    	/* Switch to page 1. */
			rge_switch_mcu_ram_page(sc, 1);
			for (; i < bps->count; i++)
				rge_write_mac_ocp(sc, bps->regs[i],
				    bps->vals[i]);
		}
		rge_write_mac_ocp(sc, 0xfc26, 0x8000);
		rge_write_mac_ocp(sc, 0xfc2c, 0x14a4);
		rge_write_mac_ocp(sc, 0xfc2e, 0x4176);
		rge_write_mac_ocp(sc, 0xfc30, 0x41fc);
		rge_write_mac_ocp(sc, 0xfc32, 0x4298);
		rge_write_mac_ocp(sc, 0xfc3a, 0x234a);
		rge_write_mac_ocp(sc, 0xfc48, 0x023c);
	} else if (type == MAC_R27) {
		bps = &rtl8127_mac_bps;
		mcodever =
		    rge_mcu_get_bin_version(rtl8127_mac_bps_vals, bps->count);
		if (sc->rge_mcodever != mcodever) {
			/* Switch to page 0. */
			rge_switch_mcu_ram_page(sc, 0);
			for (i = 0; i < 256; i++)
				rge_write_mac_ocp(sc, bps->regs[i],
				    bps->vals[i]);
		    	/* Switch to page 1. */
			rge_switch_mcu_ram_page(sc, 1);
			for (; i < bps->count; i++)
				rge_write_mac_ocp(sc, bps->regs[i],
				    bps->vals[i]);
		}
		rge_write_mac_ocp(sc, 0xfc26, 0x8000);
		rge_write_mac_ocp(sc, 0xfc28, 0x1520);
		rge_write_mac_ocp(sc, 0xfc2a, 0x41e0);
		rge_write_mac_ocp(sc, 0xfc2c, 0x508c);
		rge_write_mac_ocp(sc, 0xfc2e, 0x50f6);
		rge_write_mac_ocp(sc, 0xfc30, 0x34fa);
		rge_write_mac_ocp(sc, 0xfc32, 0x0166);
		rge_write_mac_ocp(sc, 0xfc34, 0x1a6a);
		rge_write_mac_ocp(sc, 0xfc36, 0x1a2c);
		rge_write_mac_ocp(sc, 0xfc48, 0x00ff);
	}

	/* Write microcode version. */
	rge_mcu_set_version(sc, mcodever);
};

static uint64_t
rge_mcu_get_bin_version(const uint16_t *mac_bps, uint16_t entries)
{
	uint64_t binver = 0;
	int i;

	for (i = 0; i < 4; i++) {
		binver <<= 16;
		binver |= mac_bps[entries - 4 + i];
	}

	return binver;
}

static void
rge_mcu_set_version(struct rge_softc *sc, uint64_t mcodever)
{
	int i;

	/* Switch to page 2. */
	rge_switch_mcu_ram_page(sc, 2);

	for (i = 0; i < 8; i += 2) {
		rge_write_mac_ocp(sc, 0xf9f8 + 6 - i, (uint16_t)mcodever);
		mcodever >>= 16;
	}

	/* Switch back to page 0. */
	rge_switch_mcu_ram_page(sc, 0);
}

static void
rge_ephy_config(struct rge_softc *sc)
{
	switch (sc->rge_type) {
	case MAC_R25:
		rge_ephy_config_mac_r25(sc);
		break;
	case MAC_R25B:
		rge_ephy_config_mac_r25b(sc);
		break;
	case MAC_R27:
		rge_ephy_config_mac_r27(sc);
		break;
	default:
		break;	/* Nothing to do. */
	}
}

void
rge_ephy_config_mac_r25(struct rge_softc *sc)
{
	uint16_t val;
	int i;

	for (i = 0; i < nitems(mac_r25_ephy); i++)
		rge_write_ephy(sc, mac_r25_ephy[i].reg, mac_r25_ephy[i].val);

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
}

void
rge_ephy_config_mac_r25b(struct rge_softc *sc)
{
	int i;

	for (i = 0; i < nitems(mac_r25b_ephy); i++)
		rge_write_ephy(sc, mac_r25b_ephy[i].reg, mac_r25b_ephy[i].val);
}

void
rge_ephy_config_mac_r27(struct rge_softc *sc)
{
	int i;

	for (i = 0; i < nitems(mac_r27_ephy); i++)
		rge_r27_write_ephy(sc, mac_r27_ephy[i].reg,
		    mac_r27_ephy[i].val);

	/* Clear extended address. */
	rge_write_ephy(sc, RGE_EPHYAR_EXT_ADDR, 0);
}

static int
rge_phy_config(struct rge_softc *sc)
{
	uint16_t val = 0;
	int i;

	rge_ephy_config(sc);

	/* PHY reset. */
	rge_write_phy(sc, 0, MII_ANAR,
	    rge_read_phy(sc, 0, MII_ANAR) &
	    ~(ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10));
	rge_write_phy(sc, 0, MII_100T2CR,
	    rge_read_phy(sc, 0, MII_100T2CR) &
	    ~(GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX));
	switch (sc->rge_type) {
	case MAC_R27:
		val |= RGE_ADV_10000TFDX;
		/* fallthrough */
	case MAC_R26_1:
	case MAC_R26_2:
		val |= RGE_ADV_5000TFDX;
		/* fallthrough */
	default:
		val |= RGE_ADV_2500TFDX;
		break;
	}

	RGE_PHY_CLRBIT(sc, 0xa5d4, val);
	rge_write_phy(sc, 0, MII_BMCR, BMCR_RESET | BMCR_AUTOEN |
	    BMCR_STARTNEG);
	for (i = 0; i < 2500; i++) {
		if (!(rge_read_phy(sc, 0, MII_BMCR) & BMCR_RESET))
			break;
		DELAY(1000);
	}
	if (i == 2500) {
		device_printf(sc->sc_dev, "PHY reset failed\n");
		return (ETIMEDOUT);
	}

	/* Read ram code version. */
	rge_write_phy_ocp(sc, 0xa436, 0x801e);
	sc->rge_rcodever = rge_read_phy_ocp(sc, 0xa438);

	switch (sc->rge_type) {
	case MAC_R25:
		rge_phy_config_mac_r25(sc);
		break;
	case MAC_R25B:
		rge_phy_config_mac_r25b(sc);
		break;
	case MAC_R25D:
		rge_phy_config_mac_r25d(sc);
		break;
	case MAC_R26_1:
		rge_phy_config_mac_r26_1(sc);
		break;
	case MAC_R26_2:
		rge_phy_config_mac_r26_2(sc);
		break;
	case MAC_R27:
		rge_phy_config_mac_r27(sc);
		break;
	default:
		break;	/* Can't happen. */
	}
	RGE_PHY_CLRBIT(sc, 0xa5b4, 0x8000);

	/* Disable EEE. */
	RGE_MAC_CLRBIT(sc, 0xe040, 0x0003);
	if (sc->rge_type == MAC_R25) {
		RGE_MAC_CLRBIT(sc, 0xeb62, 0x0006);
		RGE_PHY_CLRBIT(sc, 0xa432, 0x0010);
	} else if (sc->rge_type == MAC_R25B || sc->rge_type == MAC_R25D)
		RGE_PHY_SETBIT(sc, 0xa432, 0x0010);

	RGE_PHY_CLRBIT(sc, 0xa5d0, (sc->rge_type == MAC_R27) ? 0x000e : 0x0006);
	RGE_PHY_CLRBIT(sc, 0xa6d4, 0x0001);
	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27)
		RGE_PHY_CLRBIT(sc, 0xa6d4, 0x0002);
	RGE_PHY_CLRBIT(sc, 0xa6d8, 0x0010);
	RGE_PHY_CLRBIT(sc, 0xa428, 0x0080);
	RGE_PHY_CLRBIT(sc, 0xa4a2, 0x0200);

	/* Disable advanced EEE. */
	RGE_MAC_CLRBIT(sc, 0xe052, 0x0001);
	RGE_PHY_CLRBIT(sc, 0xa442, 0x3000);
	RGE_PHY_CLRBIT(sc, 0xa430, 0x8000);

	return (0);
}

static 
void
rge_phy_config_mac_r27(struct rge_softc *sc)
{
	uint16_t val;
	int i;
	static const uint16_t mac_cfg_value[] =
	    { 0x815a, 0x0150, 0x81f4, 0x0150, 0x828e, 0x0150, 0x81b1, 0x0000,
	      0x824b, 0x0000, 0x82e5, 0x0000 };

	static const uint16_t mac_cfg2_value[] =
	    { 0x88d7, 0x01a0, 0x88d9, 0x01a0, 0x8ffa, 0x002a, 0x8fee, 0xffdf,
	      0x8ff0, 0xffff, 0x8ff2, 0x0a4a, 0x8ff4, 0xaa5a, 0x8ff6, 0x0a4a,
	      0x8ff8, 0xaa5a };

	static const uint16_t mac_cfg_a438_value[] =
	    { 0x003b, 0x0086, 0x00b7, 0x00db, 0x00fe, 0x00fe, 0x00fe, 0x00fe,
	      0x00c3, 0x0078, 0x0047, 0x0023 };

	rge_phy_config_mcu(sc, RGE_MAC_R27_RCODE_VER);

	rge_write_phy_ocp(sc, 0xa4d2, 0x0000);
	rge_read_phy_ocp(sc, 0xa4d4);

	RGE_PHY_CLRBIT(sc, 0xa442, 0x0800);
	rge_write_phy_ocp(sc, 0xa436, 0x8415);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x9300);
	rge_write_phy_ocp(sc, 0xa436, 0x81a3);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0f00);
	rge_write_phy_ocp(sc, 0xa436, 0x81ae);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0f00);
	rge_write_phy_ocp(sc, 0xa436, 0x81b9);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xb900);
	rge_write_phy_ocp(sc, 0xb87c, 0x83b0);
	RGE_PHY_CLRBIT(sc,0xb87e, 0x0e00);
	rge_write_phy_ocp(sc, 0xb87c, 0x83c5);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0x0e00);
	rge_write_phy_ocp(sc, 0xb87c, 0x83da);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0x0e00);
	rge_write_phy_ocp(sc, 0xb87c, 0x83ef);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0x0e00);
	val = rge_read_phy_ocp(sc, 0xbf38) & ~0x01f0;
	rge_write_phy_ocp(sc, 0xbf38, val | 0x0160);
	val = rge_read_phy_ocp(sc, 0xbf3a) & ~0x001f;
	rge_write_phy_ocp(sc, 0xbf3a, val | 0x0014);
	RGE_PHY_CLRBIT(sc, 0xbf28, 0x6000);
	RGE_PHY_CLRBIT(sc, 0xbf2c, 0xc000);
	val = rge_read_phy_ocp(sc, 0xbf28) & ~0x1fff;
	rge_write_phy_ocp(sc, 0xbf28, val | 0x0187);
	val = rge_read_phy_ocp(sc, 0xbf2a) & ~0x003f;
	rge_write_phy_ocp(sc, 0xbf2a, val | 0x0003);
	rge_write_phy_ocp(sc, 0xa436, 0x8173);
	rge_write_phy_ocp(sc, 0xa438, 0x8620);
	rge_write_phy_ocp(sc, 0xa436, 0x8175);
	rge_write_phy_ocp(sc, 0xa438, 0x8671);
	rge_write_phy_ocp(sc, 0xa436, 0x817c);
	RGE_PHY_SETBIT(sc, 0xa438, 0x2000);
	rge_write_phy_ocp(sc, 0xa436, 0x8187);
	RGE_PHY_SETBIT(sc, 0xa438, 0x2000);
	rge_write_phy_ocp(sc, 0xA436, 0x8192);
	RGE_PHY_SETBIT(sc, 0xA438, 0x2000);
	rge_write_phy_ocp(sc, 0xA436, 0x819D);
	RGE_PHY_SETBIT(sc, 0xA438, 0x2000);
	rge_write_phy_ocp(sc, 0xA436, 0x81A8);
	RGE_PHY_CLRBIT(sc, 0xA438, 0x2000);
	rge_write_phy_ocp(sc, 0xA436, 0x81B3);
	RGE_PHY_CLRBIT(sc, 0xA438, 0x2000);
	rge_write_phy_ocp(sc, 0xA436, 0x81BE);
	RGE_PHY_SETBIT(sc, 0xA438, 0x2000);
	rge_write_phy_ocp(sc, 0xa436, 0x817d);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xa600);
	rge_write_phy_ocp(sc, 0xa436, 0x8188);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xa600);
	rge_write_phy_ocp(sc, 0xa436, 0x8193);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xa600);
	rge_write_phy_ocp(sc, 0xa436, 0x819e);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xa600);
	rge_write_phy_ocp(sc, 0xa436, 0x81a9);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1400);
	rge_write_phy_ocp(sc, 0xa436, 0x81b4);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1400);
	rge_write_phy_ocp(sc, 0xa436, 0x81bf);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xa600);
	RGE_PHY_CLRBIT(sc, 0xaeaa, 0x0028);
	rge_write_phy_ocp(sc, 0xb87c, 0x84f0);
	rge_write_phy_ocp(sc, 0xb87e, 0x201c);
	rge_write_phy_ocp(sc, 0xb87c, 0x84f2);
	rge_write_phy_ocp(sc, 0xb87e, 0x3117);
	rge_write_phy_ocp(sc, 0xaec6, 0x0000);
	rge_write_phy_ocp(sc, 0xae20, 0xffff);
	rge_write_phy_ocp(sc, 0xaece, 0xffff);
	rge_write_phy_ocp(sc, 0xaed2, 0xffff);
	rge_write_phy_ocp(sc, 0xaec8, 0x0000);
	RGE_PHY_CLRBIT(sc, 0xaed0, 0x0001);
	rge_write_phy_ocp(sc, 0xadb8, 0x0150);
	rge_write_phy_ocp(sc, 0xb87c, 0x8197);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8231);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5000);
	rge_write_phy_ocp(sc, 0xb87c, 0x82cb);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5000);
	rge_write_phy_ocp(sc, 0xb87c, 0x82cd);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5700);
	rge_write_phy_ocp(sc, 0xb87c, 0x8233);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5700);
	rge_write_phy_ocp(sc, 0xb87c, 0x8199);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5700);
	for (i = 0; i < nitems(mac_cfg_value); i+=2) {
		rge_write_phy_ocp(sc, 0xb87c, mac_cfg_value[i]);
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg_value[i + 1]);
	}
	rge_write_phy_ocp(sc, 0xb87c, 0x84f7);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x2800);
	RGE_PHY_SETBIT(sc, 0xaec2, 0x1000);
	rge_write_phy_ocp(sc, 0xb87c, 0x81b3);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xad00);
	rge_write_phy_ocp(sc, 0xb87c, 0x824d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xad00);
	rge_write_phy_ocp(sc, 0xb87c, 0x82e7);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xad00);
	val = rge_read_phy_ocp(sc, 0xae4e) & ~0x000f;
	rge_write_phy_ocp(sc, 0xae4e, val | 0x0001);
	rge_write_phy_ocp(sc, 0xb87c, 0x82ce);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xf000;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x4000);
	rge_write_phy_ocp(sc, 0xb87c, 0x84ac);
	rge_write_phy_ocp(sc, 0xb87e, 0x0000);
	rge_write_phy_ocp(sc, 0xb87c, 0x84ae);
	rge_write_phy_ocp(sc, 0xb87e, 0x0000);
	rge_write_phy_ocp(sc, 0xb87c, 0x84b0);
	rge_write_phy_ocp(sc, 0xb87e, 0xf818);
	rge_write_phy_ocp(sc, 0xb87c, 0x84b2);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x6000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffc);
	rge_write_phy_ocp(sc, 0xb87e, 0x6008);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffe);
	rge_write_phy_ocp(sc, 0xb87e, 0xf450);
	rge_write_phy_ocp(sc, 0xb87c, 0x8015);
	RGE_PHY_SETBIT(sc, 0xb87e, 0x0200);
	rge_write_phy_ocp(sc, 0xb87c, 0x8016);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0x0800);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe6);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0800);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe4);
	rge_write_phy_ocp(sc, 0xb87e, 0x2114);
	rge_write_phy_ocp(sc, 0xb87c, 0x8647);
	rge_write_phy_ocp(sc, 0xb87e, 0xa7B1);
	rge_write_phy_ocp(sc, 0xb87c, 0x8649);
	rge_write_phy_ocp(sc, 0xb87e, 0xbbca);
	rge_write_phy_ocp(sc, 0xb87c, 0x864b);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xdc00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8154);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xc000;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x4000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8158);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0xc000);
	rge_write_phy_ocp(sc, 0xb87c, 0x826c);
	rge_write_phy_ocp(sc, 0xb87e, 0xffff);
	rge_write_phy_ocp(sc, 0xb87c, 0x826e);
	rge_write_phy_ocp(sc, 0xb87e, 0xffff);
	rge_write_phy_ocp(sc, 0xb87c, 0x8872);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0e00);
	rge_write_phy_ocp(sc, 0xa436, 0x8012);
	RGE_PHY_SETBIT(sc, 0xa438, 0x0800);
	rge_write_phy_ocp(sc, 0xa436, 0x8012);
	RGE_PHY_SETBIT(sc, 0xa438, 0x4000);
	RGE_PHY_SETBIT(sc, 0xb576, 0x0001);
	rge_write_phy_ocp(sc, 0xa436, 0x834a);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0700);
	rge_write_phy_ocp(sc, 0xb87c, 0x8217);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0x3f00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x2a00);
	rge_write_phy_ocp(sc, 0xa436, 0x81b1);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0b00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fed);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x4e00);
	rge_write_phy_ocp(sc, 0xb87c, 0x88ac);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x2300);
	RGE_PHY_SETBIT(sc, 0xbf0c, 0x3800);
	rge_write_phy_ocp(sc, 0xb87c, 0x88de);
	RGE_PHY_CLRBIT(sc, 0xb87e, 0xFF00);
	rge_write_phy_ocp(sc, 0xb87c, 0x80B4);
	rge_write_phy_ocp(sc, 0xb87e, 0x5195);
	rge_write_phy_ocp(sc, 0xa436, 0x8370);
	rge_write_phy_ocp(sc, 0xa438, 0x8671);
	rge_write_phy_ocp(sc, 0xa436, 0x8372);
	rge_write_phy_ocp(sc, 0xa438, 0x86c8);
	rge_write_phy_ocp(sc, 0xa436, 0x8401);
	rge_write_phy_ocp(sc, 0xa438, 0x86c8);
	rge_write_phy_ocp(sc, 0xa436, 0x8403);
	rge_write_phy_ocp(sc, 0xa438, 0x86da);
	rge_write_phy_ocp(sc, 0xa436, 0x8406);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x8408);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x840a);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x840c);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x840e);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x8410);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x8412);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x8414);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x8416);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x1800;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x82bd);
	rge_write_phy_ocp(sc, 0xa438, 0x1f40);
	val = rge_read_phy_ocp(sc, 0xbfb4) & ~0x07ff;
	rge_write_phy_ocp(sc, 0xbfb4, val | 0x0328);
	rge_write_phy_ocp(sc, 0xbfb6, 0x3e14);
	rge_write_phy_ocp(sc, 0xa436, 0x81c4);
	for (i = 0; i < nitems(mac_cfg_a438_value); i++)
		rge_write_phy_ocp(sc, 0xa438, mac_cfg_a438_value[i]);
	for (i = 0; i < nitems(mac_cfg2_value); i+=2) {
		rge_write_phy_ocp(sc, 0xb87c, mac_cfg2_value[i]);
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg2_value[i + 1]);
	}
	rge_write_phy_ocp(sc, 0xb87c, 0x88d5);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0200);
	rge_write_phy_ocp(sc, 0xa436, 0x84bb);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0a00);
	rge_write_phy_ocp(sc, 0xa436, 0x84c0);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1600);
	RGE_PHY_SETBIT(sc, 0xa430, 0x0003);
}

static void
rge_phy_config_mac_r26_1(struct rge_softc *sc)
{
	uint16_t val;
	int i;
	static const uint16_t mac_cfg2_a438_value[] =
	    { 0x0044, 0x00a8, 0x00d6, 0x00ec, 0x00f6, 0x00fc, 0x00fe,
	      0x00fe, 0x00bc, 0x0058, 0x002a, 0x003f, 0x3f02, 0x023c,
	      0x3b0a, 0x1c00, 0x0000, 0x0000, 0x0000, 0x0000 };

	static const uint16_t mac_cfg2_b87e_value[] =
	    { 0x03ed, 0x03ff, 0x0009, 0x03fe, 0x000b, 0x0021, 0x03f7,
	      0x03b8, 0x03e0, 0x0049, 0x0049, 0x03e0, 0x03b8, 0x03f7,
	      0x0021, 0x000b, 0x03fe, 0x0009, 0x03ff, 0x03ed, 0x000e,
	      0x03fe, 0x03ed, 0x0006, 0x001a, 0x03f1, 0x03d8, 0x0023,
	      0x0054, 0x0322, 0x00dd, 0x03ab, 0x03dc, 0x0027, 0x000e,
	      0x03e5, 0x03f9, 0x0012, 0x0001, 0x03f1 };

	rge_phy_config_mcu(sc, RGE_MAC_R26_1_RCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	rge_write_phy_ocp(sc, 0xa436, 0x80bf);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xed00);
	rge_write_phy_ocp(sc, 0xa436, 0x80cd);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1000);
	rge_write_phy_ocp(sc, 0xa436, 0x80d1);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xc800);
	rge_write_phy_ocp(sc, 0xa436, 0x80d4);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xc800);
	rge_write_phy_ocp(sc, 0xa436, 0x80e1);
	rge_write_phy_ocp(sc, 0xa438, 0x10cc);
	rge_write_phy_ocp(sc, 0xa436, 0x80e5);
	rge_write_phy_ocp(sc, 0xa438, 0x4f0c);
	rge_write_phy_ocp(sc, 0xa436, 0x8387);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x4700);
	val = rge_read_phy_ocp(sc, 0xa80c) & ~0x00c0;
	rge_write_phy_ocp(sc, 0xa80c, val | 0x0080);
	RGE_PHY_CLRBIT(sc, 0xac90, 0x0010);
	RGE_PHY_CLRBIT(sc, 0xad2c, 0x8000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8321);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1100);
	RGE_PHY_SETBIT(sc, 0xacf8, 0x000c);
	rge_write_phy_ocp(sc, 0xa436, 0x8183);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5900);
	RGE_PHY_SETBIT(sc, 0xad94, 0x0020);
	RGE_PHY_CLRBIT(sc, 0xa654, 0x0800);
	RGE_PHY_SETBIT(sc, 0xb648, 0x4000);
	rge_write_phy_ocp(sc, 0xb87c, 0x839e);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x2f00);
	rge_write_phy_ocp(sc, 0xb87c, 0x83f2);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0800);
	RGE_PHY_SETBIT(sc, 0xada0, 0x0002);
	rge_write_phy_ocp(sc, 0xb87c, 0x80f3);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x9900);
	rge_write_phy_ocp(sc, 0xb87c, 0x8126);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xc100);
	rge_write_phy_ocp(sc, 0xb87c, 0x893a);
	rge_write_phy_ocp(sc, 0xb87e, 0x8080);
	rge_write_phy_ocp(sc, 0xb87c, 0x8647);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xe600);
	rge_write_phy_ocp(sc, 0xb87c, 0x862c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1200);
	rge_write_phy_ocp(sc, 0xb87c, 0x864a);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xe600);
	rge_write_phy_ocp(sc, 0xb87c, 0x80a0);
	rge_write_phy_ocp(sc, 0xb87e, 0xbcbc);
	rge_write_phy_ocp(sc, 0xb87c, 0x805e);
	rge_write_phy_ocp(sc, 0xb87e, 0xbcbc);
	rge_write_phy_ocp(sc, 0xb87c, 0x8056);
	rge_write_phy_ocp(sc, 0xb87e, 0x3077);
	rge_write_phy_ocp(sc, 0xb87c, 0x8058);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5a00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8098);
	rge_write_phy_ocp(sc, 0xb87e, 0x3077);
	rge_write_phy_ocp(sc, 0xb87c, 0x809a);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5a00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8052);
	rge_write_phy_ocp(sc, 0xb87e, 0x3733);
	rge_write_phy_ocp(sc, 0xb87c, 0x8094);
	rge_write_phy_ocp(sc, 0xb87e, 0x3733);
	rge_write_phy_ocp(sc, 0xb87c, 0x807f);
	rge_write_phy_ocp(sc, 0xb87e, 0x7c75);
	rge_write_phy_ocp(sc, 0xb87c, 0x803d);
	rge_write_phy_ocp(sc, 0xb87e, 0x7c75);
	rge_write_phy_ocp(sc, 0xb87c, 0x8036);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8078);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8031);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3300);
	rge_write_phy_ocp(sc, 0xb87c, 0x8073);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3300);
	val = rge_read_phy_ocp(sc, 0xae06) & ~0xfc00;
	rge_write_phy_ocp(sc, 0xae06, val | 0x7c00);
	rge_write_phy_ocp(sc, 0xb87c, 0x89D1);
	rge_write_phy_ocp(sc, 0xb87e, 0x0004);
	rge_write_phy_ocp(sc, 0xa436, 0x8fbd);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0a00);
	rge_write_phy_ocp(sc, 0xa436, 0x8fbe);
	rge_write_phy_ocp(sc, 0xa438, 0x0d09);
	rge_write_phy_ocp(sc, 0xb87c, 0x89cd);
	rge_write_phy_ocp(sc, 0xb87e, 0x0f0f);
	rge_write_phy_ocp(sc, 0xb87c, 0x89cf);
	rge_write_phy_ocp(sc, 0xb87e, 0x0f0f);
	rge_write_phy_ocp(sc, 0xb87c, 0x83a4);
	rge_write_phy_ocp(sc, 0xb87e, 0x6600);
	rge_write_phy_ocp(sc, 0xb87c, 0x83a6);
	rge_write_phy_ocp(sc, 0xb87e, 0x6601);
	rge_write_phy_ocp(sc, 0xb87c, 0x83c0);
	rge_write_phy_ocp(sc, 0xb87e, 0x6600);
	rge_write_phy_ocp(sc, 0xb87c, 0x83c2);
	rge_write_phy_ocp(sc, 0xb87e, 0x6601);
	rge_write_phy_ocp(sc, 0xb87c, 0x8414);
	rge_write_phy_ocp(sc, 0xb87e, 0x6600);
	rge_write_phy_ocp(sc, 0xb87c, 0x8416);
	rge_write_phy_ocp(sc, 0xb87e, 0x6601);
	rge_write_phy_ocp(sc, 0xb87c, 0x83f8);
	rge_write_phy_ocp(sc, 0xb87e, 0x6600);
	rge_write_phy_ocp(sc, 0xb87c, 0x83fa);
	rge_write_phy_ocp(sc, 0xb87e, 0x6601);

	rge_patch_phy_mcu(sc, 1);
	val = rge_read_phy_ocp(sc, 0xbd96) & ~0x1f00;
	rge_write_phy_ocp(sc, 0xbd96, val | 0x1000);
	val = rge_read_phy_ocp(sc, 0xbf1c) & ~0x0007;
	rge_write_phy_ocp(sc, 0xbf1c, val | 0x0007);
	RGE_PHY_CLRBIT(sc, 0xbfbe, 0x8000);
	val = rge_read_phy_ocp(sc, 0xbf40) & ~0x0380;
	rge_write_phy_ocp(sc, 0xbf40, val | 0x0280);
	val = rge_read_phy_ocp(sc, 0xbf90) & ~0x0080;
	rge_write_phy_ocp(sc, 0xbf90, val | 0x0060);
	val = rge_read_phy_ocp(sc, 0xbf90) & ~0x0010;
	rge_write_phy_ocp(sc, 0xbf90, val | 0x000c);
	rge_patch_phy_mcu(sc, 0);

	rge_write_phy_ocp(sc, 0xa436, 0x843b);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x2000);
	rge_write_phy_ocp(sc, 0xa436, 0x843d);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x2000);
	RGE_PHY_CLRBIT(sc, 0xb516, 0x007f);
	RGE_PHY_CLRBIT(sc, 0xbf80, 0x0030);

	rge_write_phy_ocp(sc, 0xa436, 0x8188);
	for (i = 0; i < 11; i++)
		rge_write_phy_ocp(sc, 0xa438, mac_cfg2_a438_value[i]);

	rge_write_phy_ocp(sc, 0xb87c, 0x8015);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0800);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffd);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fff);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x7f00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffb);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe9);
	rge_write_phy_ocp(sc, 0xb87e, 0x0002);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fef);
	rge_write_phy_ocp(sc, 0xb87e, 0x00a5);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ff1);
	rge_write_phy_ocp(sc, 0xb87e, 0x0106);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe1);
	rge_write_phy_ocp(sc, 0xb87e, 0x0102);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe3);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0400);
	RGE_PHY_SETBIT(sc, 0xa654, 0x0800);
	RGE_PHY_CLRBIT(sc, 0xa654, 0x0003);
	rge_write_phy_ocp(sc, 0xac3a, 0x5851);
	val = rge_read_phy_ocp(sc, 0xac3c) & ~0xd000;
	rge_write_phy_ocp(sc, 0xac3c, val | 0x2000);
	val = rge_read_phy_ocp(sc, 0xac42) & ~0x0200;
	rge_write_phy_ocp(sc, 0xac42, val | 0x01c0);
	RGE_PHY_CLRBIT(sc, 0xac3e, 0xe000);
	RGE_PHY_CLRBIT(sc, 0xac42, 0x0038);
	val = rge_read_phy_ocp(sc, 0xac42) & ~0x0002;
	rge_write_phy_ocp(sc, 0xac42, val | 0x0005);
	rge_write_phy_ocp(sc, 0xac1a, 0x00db);
	rge_write_phy_ocp(sc, 0xade4, 0x01b5);
	RGE_PHY_CLRBIT(sc, 0xad9c, 0x0c00);
	rge_write_phy_ocp(sc, 0xb87c, 0x814b);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1100);
	rge_write_phy_ocp(sc, 0xb87c, 0x814d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1100);
	rge_write_phy_ocp(sc, 0xb87c, 0x814f);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0b00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8142);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8144);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8150);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8118);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0700);
	rge_write_phy_ocp(sc, 0xb87c, 0x811a);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0700);
	rge_write_phy_ocp(sc, 0xb87c, 0x811c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0500);
	rge_write_phy_ocp(sc, 0xb87c, 0x810f);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8111);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x811d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	RGE_PHY_SETBIT(sc, 0xac36, 0x1000);
	RGE_PHY_CLRBIT(sc, 0xad1c, 0x0100);
	val = rge_read_phy_ocp(sc, 0xade8) & ~0xffc0;
	rge_write_phy_ocp(sc, 0xade8, val | 0x1400);
	rge_write_phy_ocp(sc, 0xb87c, 0x864b);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x9d00);

	rge_write_phy_ocp(sc, 0xa436, 0x8f97);
	for (; i < nitems(mac_cfg2_a438_value); i++)
		rge_write_phy_ocp(sc, 0xa438, mac_cfg2_a438_value[i]);

	RGE_PHY_SETBIT(sc, 0xad9c, 0x0020);
	rge_write_phy_ocp(sc, 0xb87c, 0x8122);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0c00);

	rge_write_phy_ocp(sc, 0xb87c, 0x82c8);
	for (i = 0; i < 20; i++)
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg2_b87e_value[i]);

	rge_write_phy_ocp(sc, 0xb87c, 0x80ef);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0c00);

	rge_write_phy_ocp(sc, 0xb87c, 0x82a0);
	for (; i < nitems(mac_cfg2_b87e_value); i++)
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg2_b87e_value[i]);

	rge_write_phy_ocp(sc, 0xa436, 0x8018);
	RGE_PHY_SETBIT(sc, 0xa438, 0x2000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe4);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0);
	val = rge_read_phy_ocp(sc, 0xb54c) & ~0xffc0;
	rge_write_phy_ocp(sc, 0xb54c, val | 0x3700);
}

static void
rge_phy_config_mac_r26_2(struct rge_softc *sc)
{
	uint16_t val;
	int i;
	static const uint16_t mac_cfg3_b87e_value[] =
	    { 0x03ed, 0x03ff, 0x0009, 0x03fe, 0x000b, 0x0021, 0x03f7,
	      0x03b8, 0x03e0, 0x0049, 0x0049, 0x03e0, 0x03b8, 0x03f7,
	      0x0021, 0x000b, 0x03fe, 0x0009, 0x03ff, 0x03ed, 0x82a0,
	      0x000e, 0x03fe, 0x03ed, 0x0006, 0x001a, 0x03f1, 0x03d8,
	      0x0023, 0x0054, 0x0322, 0x00dd, 0x03ab, 0x03dc, 0x0027,
	      0x000e, 0x03e5, 0x03f9, 0x0012, 0x0001, 0x03f1 };

	rge_phy_config_mcu(sc, RGE_MAC_R26_2_RCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	rge_write_phy_ocp(sc, 0xa436, 0x8183);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5900);
	RGE_PHY_SETBIT(sc, 0xa654, 0x0800);
	RGE_PHY_SETBIT(sc, 0xb648, 0x4000);
	RGE_PHY_SETBIT(sc, 0xad2c, 0x8000);
	RGE_PHY_SETBIT(sc, 0xad94, 0x0020);
	RGE_PHY_SETBIT(sc, 0xada0, 0x0002);
	val = rge_read_phy_ocp(sc, 0xae06) & ~0xfc00;
	rge_write_phy_ocp(sc, 0xae06, val | 0x7c00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8647);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0xe600);
	rge_write_phy_ocp(sc, 0xb87c, 0x8036);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3000);
	rge_write_phy_ocp(sc, 0xb87c, 0x8078);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3000);
	rge_write_phy_ocp(sc, 0xb87c, 0x89e9);
	RGE_PHY_SETBIT(sc, 0xb87e, 0xff00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffd);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8ffe);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0200);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fff);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0400);
	rge_write_phy_ocp(sc, 0xa436, 0x8018);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x7700);
	rge_write_phy_ocp(sc, 0xa436, 0x8f9c);
	rge_write_phy_ocp(sc, 0xa438, 0x0005);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	rge_write_phy_ocp(sc, 0xa438, 0x00ed);
	rge_write_phy_ocp(sc, 0xa438, 0x0502);
	rge_write_phy_ocp(sc, 0xa438, 0x0b00);
	rge_write_phy_ocp(sc, 0xa438, 0xd401);
	rge_write_phy_ocp(sc, 0xa436, 0x8fa8);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x2900);
	rge_write_phy_ocp(sc, 0xb87c, 0x814b);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1100);
	rge_write_phy_ocp(sc, 0xb87c, 0x814d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1100);
	rge_write_phy_ocp(sc, 0xb87c, 0x814f);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0b00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8142);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8144);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8150);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8118);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0700);
	rge_write_phy_ocp(sc, 0xb87c, 0x811a);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0700);
	rge_write_phy_ocp(sc, 0xb87c, 0x811c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0500);
	rge_write_phy_ocp(sc, 0xb87c, 0x810f);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8111);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x811d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	RGE_PHY_SETBIT(sc, 0xad1c, 0x0100);
	val = rge_read_phy_ocp(sc, 0xade8) & ~0xffc0;
	rge_write_phy_ocp(sc, 0xade8, val | 0x1400);
	rge_write_phy_ocp(sc, 0xb87c, 0x864b);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x9d00);
	rge_write_phy_ocp(sc, 0xb87c, 0x862c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x1200);
	rge_write_phy_ocp(sc, 0xa436, 0x8566);
	rge_write_phy_ocp(sc, 0xa438, 0x003f);
	rge_write_phy_ocp(sc, 0xa438, 0x3f02);
	rge_write_phy_ocp(sc, 0xa438, 0x023c);
	rge_write_phy_ocp(sc, 0xa438, 0x3b0a);
	rge_write_phy_ocp(sc, 0xa438, 0x1c00);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	RGE_PHY_SETBIT(sc, 0xad9c, 0x0020);
	rge_write_phy_ocp(sc, 0xb87c, 0x8122);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0c00);
	rge_write_phy_ocp(sc, 0xb87c, 0x82c8);
	for (i = 0; i < 20; i++)
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg3_b87e_value[i]);
	rge_write_phy_ocp(sc, 0xb87c, 0x80ef);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0c00);
	for (; i < nitems(mac_cfg3_b87e_value); i++)
		rge_write_phy_ocp(sc, 0xb87e, mac_cfg3_b87e_value[i]);
	RGE_PHY_SETBIT(sc, 0xa430, 0x0003);
	val = rge_read_phy_ocp(sc, 0xb54c) & ~0xffc0;
	rge_write_phy_ocp(sc, 0xb54c, val | 0x3700);
	RGE_PHY_SETBIT(sc, 0xb648, 0x0040);
	rge_write_phy_ocp(sc, 0xb87c, 0x8082);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5d00);
	rge_write_phy_ocp(sc, 0xb87c, 0x807c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5000);
	rge_write_phy_ocp(sc, 0xb87c, 0x809d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x5000);
}

static void
rge_phy_config_mac_r25(struct rge_softc *sc)
{
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

	rge_phy_config_mcu(sc, RGE_MAC_R25_RCODE_VER);

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
	RGE_PHY_SETBIT(sc, 0xa424, 0x0008);
}

static void
rge_phy_config_mac_r25b(struct rge_softc *sc)
{
	uint16_t val;
	int i;

	rge_phy_config_mcu(sc, RGE_MAC_R25B_RCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);
	val = rge_read_phy_ocp(sc, 0xac46) & ~0x00f0;
	rge_write_phy_ocp(sc, 0xac46, val | 0x0090);
	val = rge_read_phy_ocp(sc, 0xad30) & ~0x0003;
	rge_write_phy_ocp(sc, 0xad30, val | 0x0001);
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
	rge_write_phy_ocp(sc, 0xa436, 0x8170);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x2700;
	rge_write_phy_ocp(sc, 0xa438, val | 0xd800);
	RGE_PHY_SETBIT(sc, 0xa424, 0x0008);
}

static void
rge_phy_config_mac_r25d(struct rge_softc *sc)
{
	uint16_t val;
	int i;

	rge_phy_config_mcu(sc, RGE_MAC_R25D_RCODE_VER);

	RGE_PHY_SETBIT(sc, 0xa442, 0x0800);

	rge_patch_phy_mcu(sc, 1);
	RGE_PHY_SETBIT(sc, 0xbf96, 0x8000);
	val = rge_read_phy_ocp(sc, 0xbf94) & ~0x0007;
	rge_write_phy_ocp(sc, 0xbf94, val | 0x0005);
	val = rge_read_phy_ocp(sc, 0xbf8e) & ~0x3c00;
	rge_write_phy_ocp(sc, 0xbf8e, val | 0x2800);
	val = rge_read_phy_ocp(sc, 0xbcd8) & ~0xc000;
	rge_write_phy_ocp(sc, 0xbcd8, val | 0x4000);
	RGE_PHY_SETBIT(sc, 0xbcd8, 0xc000);
	val = rge_read_phy_ocp(sc, 0xbcd8) & ~0xc000;
	rge_write_phy_ocp(sc, 0xbcd8, val | 0x4000);
	val = rge_read_phy_ocp(sc, 0xbc80) & ~0x001f;
	rge_write_phy_ocp(sc, 0xbc80, val | 0x0004);
	RGE_PHY_SETBIT(sc, 0xbc82, 0xe000);
	RGE_PHY_SETBIT(sc, 0xbc82, 0x1c00);
	val = rge_read_phy_ocp(sc, 0xbc80) & ~0x001f;
	rge_write_phy_ocp(sc, 0xbc80, val | 0x0005);
	val = rge_read_phy_ocp(sc, 0xbc82) & ~0x00e0;
	rge_write_phy_ocp(sc, 0xbc82, val | 0x0040);
	RGE_PHY_SETBIT(sc, 0xbc82, 0x001c);
	RGE_PHY_CLRBIT(sc, 0xbcd8, 0xc000);
	val = rge_read_phy_ocp(sc, 0xbcd8) & ~0xc000;
	rge_write_phy_ocp(sc, 0xbcd8, val | 0x8000);
	RGE_PHY_CLRBIT(sc, 0xbcd8, 0xc000);
	RGE_PHY_CLRBIT(sc, 0xbd70, 0x0100);
	RGE_PHY_SETBIT(sc, 0xa466, 0x0002);
	rge_write_phy_ocp(sc, 0xa436, 0x836a);
	RGE_PHY_CLRBIT(sc, 0xa438, 0xff00);
	rge_patch_phy_mcu(sc, 0);

	rge_write_phy_ocp(sc, 0xb87c, 0x832c);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0500);
	val = rge_read_phy_ocp(sc, 0xb106) & ~0x0700;
	rge_write_phy_ocp(sc, 0xb106, val | 0x0100);
	val = rge_read_phy_ocp(sc, 0xb206) & ~0x0700;
	rge_write_phy_ocp(sc, 0xb206, val | 0x0200);
	val = rge_read_phy_ocp(sc, 0xb306) & ~0x0700;
	rge_write_phy_ocp(sc, 0xb306, val | 0x0300);
	rge_write_phy_ocp(sc, 0xb87c, 0x80cb);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0300);
	rge_write_phy_ocp(sc, 0xbcf4, 0x0000);
	rge_write_phy_ocp(sc, 0xbcf6, 0x0000);
	rge_write_phy_ocp(sc, 0xbc12, 0x0000);
	rge_write_phy_ocp(sc, 0xb87c, 0x844d);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0200);

	rge_write_phy_ocp(sc, 0xb87c, 0x8feb);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0100);
	rge_write_phy_ocp(sc, 0xb87c, 0x8fe9);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x0600);

	val = rge_read_phy_ocp(sc, 0xac7e) & ~0x01fc;
	rge_write_phy_ocp(sc, 0xac7e, val | 0x00B4);
	rge_write_phy_ocp(sc, 0xb87c, 0x8105);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x7a00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8117);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3a00);
	rge_write_phy_ocp(sc, 0xb87c, 0x8103);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x7400);
	rge_write_phy_ocp(sc, 0xb87c, 0x8115);
	val = rge_read_phy_ocp(sc, 0xb87e) & ~0xff00;
	rge_write_phy_ocp(sc, 0xb87e, val | 0x3400);
	RGE_PHY_CLRBIT(sc, 0xad40, 0x0030);
	val = rge_read_phy_ocp(sc, 0xad66) & ~0x000f;
	rge_write_phy_ocp(sc, 0xad66, val | 0x0007);
	val = rge_read_phy_ocp(sc, 0xad68) & ~0xf000;
	rge_write_phy_ocp(sc, 0xad68, val | 0x8000);
	val = rge_read_phy_ocp(sc, 0xad68) & ~0x0f00;
	rge_write_phy_ocp(sc, 0xad68, val | 0x0500);
	val = rge_read_phy_ocp(sc, 0xad68) & ~0x000f;
	rge_write_phy_ocp(sc, 0xad68, val | 0x0002);
	val = rge_read_phy_ocp(sc, 0xad6a) & ~0xf000;
	rge_write_phy_ocp(sc, 0xad6a, val | 0x7000);
	rge_write_phy_ocp(sc, 0xac50, 0x01e8);
	rge_write_phy_ocp(sc, 0xa436, 0x81fa);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5400);
	val = rge_read_phy_ocp(sc, 0xa864) & ~0x00f0;
	rge_write_phy_ocp(sc, 0xa864, val | 0x00c0);
	val = rge_read_phy_ocp(sc, 0xa42c) & ~0x00ff;
	rge_write_phy_ocp(sc, 0xa42c, val | 0x0002);
	rge_write_phy_ocp(sc, 0xa436, 0x80e1);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0f00);
	rge_write_phy_ocp(sc, 0xa436, 0x80de);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xf000;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0700);
	RGE_PHY_SETBIT(sc, 0xa846, 0x0080);
	rge_write_phy_ocp(sc, 0xa436, 0x80ba);
	rge_write_phy_ocp(sc, 0xa438, 0x8a04);
	rge_write_phy_ocp(sc, 0xa436, 0x80bd);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xca00);
	rge_write_phy_ocp(sc, 0xa436, 0x80b7);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xb300);
	rge_write_phy_ocp(sc, 0xa436, 0x80ce);
	rge_write_phy_ocp(sc, 0xa438, 0x8a04);
	rge_write_phy_ocp(sc, 0xa436, 0x80d1);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xca00);
	rge_write_phy_ocp(sc, 0xa436, 0x80cb);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0xbb00);
	rge_write_phy_ocp(sc, 0xa436, 0x80a6);
	rge_write_phy_ocp(sc, 0xa438, 0x4909);
	rge_write_phy_ocp(sc, 0xa436, 0x80a8);
	rge_write_phy_ocp(sc, 0xa438, 0x05b8);
	rge_write_phy_ocp(sc, 0xa436, 0x8200);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x5800);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff1);
	rge_write_phy_ocp(sc, 0xa438, 0x7078);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff3);
	rge_write_phy_ocp(sc, 0xa438, 0x5d78);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff5);
	rge_write_phy_ocp(sc, 0xa438, 0x7862);
	rge_write_phy_ocp(sc, 0xa436, 0x8ff7);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1400);

	rge_write_phy_ocp(sc, 0xa436, 0x814c);
	rge_write_phy_ocp(sc, 0xa438, 0x8455);
	rge_write_phy_ocp(sc, 0xa436, 0x814e);
	rge_write_phy_ocp(sc, 0xa438, 0x84a6);
	rge_write_phy_ocp(sc, 0xa436, 0x8163);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0600);
	rge_write_phy_ocp(sc, 0xa436, 0x816a);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0500);
	rge_write_phy_ocp(sc, 0xa436, 0x8171);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1f00);

	val = rge_read_phy_ocp(sc, 0xbc3a) & ~0x000f;
	rge_write_phy_ocp(sc, 0xbc3a, val | 0x0006);
	for (i = 0; i < 10; i++) {
		rge_write_phy_ocp(sc, 0xa436, 0x8064 + i * 3);
		RGE_PHY_CLRBIT(sc, 0xa438, 0x0700);
	}
	val = rge_read_phy_ocp(sc, 0xbfa0) & ~0xff70;
	rge_write_phy_ocp(sc, 0xbfa0, val | 0x5500);
	rge_write_phy_ocp(sc, 0xbfa2, 0x9d00);
	rge_write_phy_ocp(sc, 0xa436, 0x8165);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0x0700;
	rge_write_phy_ocp(sc, 0xa438, val | 0x0200);

	rge_write_phy_ocp(sc, 0xa436, 0x8019);
	RGE_PHY_SETBIT(sc, 0xa438, 0x0100);
	rge_write_phy_ocp(sc, 0xa436, 0x8fe3);
	rge_write_phy_ocp(sc, 0xa438, 0x0005);
	rge_write_phy_ocp(sc, 0xa438, 0x0000);
	rge_write_phy_ocp(sc, 0xa438, 0x00ed);
	rge_write_phy_ocp(sc, 0xa438, 0x0502);
	rge_write_phy_ocp(sc, 0xa438, 0x0b00);
	rge_write_phy_ocp(sc, 0xa438, 0xd401);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x2900);

	rge_write_phy_ocp(sc, 0xa436, 0x8018);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1700);

	rge_write_phy_ocp(sc, 0xa436, 0x815b);
	val = rge_read_phy_ocp(sc, 0xa438) & ~0xff00;
	rge_write_phy_ocp(sc, 0xa438, val | 0x1700);

	RGE_PHY_CLRBIT(sc, 0xa4e0, 0x8000);
	RGE_PHY_CLRBIT(sc, 0xa5d4, 0x0020);
	RGE_PHY_CLRBIT(sc, 0xa654, 0x0800);
	RGE_PHY_SETBIT(sc, 0xa430, 0x1001);
	RGE_PHY_SETBIT(sc, 0xa442, 0x0080);
}

static void
rge_phy_config_mcu(struct rge_softc *sc, uint16_t rcodever)
{
	if (sc->rge_rcodever != rcodever) {
		int i;

		rge_patch_phy_mcu(sc, 1);

		if (sc->rge_type == MAC_R25) {
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0x8601);
			rge_write_phy_ocp(sc, 0xa436, 0xb82e);
			rge_write_phy_ocp(sc, 0xa438, 0x0001);

			RGE_PHY_SETBIT(sc, 0xb820, 0x0080);

			for (i = 0; i < nitems(mac_r25_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r25_mcu[i].reg, mac_r25_mcu[i].val);

			RGE_PHY_CLRBIT(sc, 0xb820, 0x0080);

			rge_write_phy_ocp(sc, 0xa436, 0);
			rge_write_phy_ocp(sc, 0xa438, 0);
			RGE_PHY_CLRBIT(sc, 0xb82e, 0x0001);
			rge_write_phy_ocp(sc, 0xa436, 0x8024);
			rge_write_phy_ocp(sc, 0xa438, 0);
		} else if (sc->rge_type == MAC_R25B) {
			for (i = 0; i < nitems(mac_r25b_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r25b_mcu[i].reg, mac_r25b_mcu[i].val);
		} else if (sc->rge_type == MAC_R25D) {
			for (i = 0; i < 2403; i++)
				rge_write_phy_ocp(sc,
				    mac_r25d_mcu[i].reg, mac_r25d_mcu[i].val);
			rge_patch_phy_mcu(sc, 0);

			rge_patch_phy_mcu(sc, 1);
			for (; i < 2528; i++)
				rge_write_phy_ocp(sc,
				    mac_r25d_mcu[i].reg, mac_r25d_mcu[i].val);
			rge_patch_phy_mcu(sc, 0);

			rge_patch_phy_mcu(sc, 1);
			for (; i < nitems(mac_r25d_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r25d_mcu[i].reg, mac_r25d_mcu[i].val);
		} else if (sc->rge_type == MAC_R26_1) {
			for (i = 0; i < 6989; i++)
				rge_write_phy_ocp(sc,
				    mac_r26_1_mcu[i].reg, mac_r26_1_mcu[i].val);
			rge_patch_phy_mcu(sc, 0);

			rge_patch_phy_mcu(sc, 1);
			for (; i < nitems(mac_r26_1_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r26_1_mcu[i].reg, mac_r26_1_mcu[i].val);
		} else if (sc->rge_type == MAC_R26_2) {
			for (i = 0; i < nitems(mac_r26_2_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r26_2_mcu[i].reg, mac_r26_2_mcu[i].val);
		} else if (sc->rge_type == MAC_R27) {
			for (i = 0; i < 1887; i++)
				rge_write_phy_ocp(sc,
				    mac_r27_mcu[i].reg, mac_r27_mcu[i].val);
			rge_patch_phy_mcu(sc, 0);

			rge_patch_phy_mcu(sc, 1);
			for (; i < nitems(mac_r27_mcu); i++)
				rge_write_phy_ocp(sc,
				    mac_r27_mcu[i].reg, mac_r27_mcu[i].val);
		}

		rge_patch_phy_mcu(sc, 0);

		/* Write ram code version. */
		rge_write_phy_ocp(sc, 0xa436, 0x801e);
		rge_write_phy_ocp(sc, 0xa438, rcodever);
	}
}

static void
rge_set_macaddr(struct rge_softc *sc, const uint8_t *addr)
{
	RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
	RGE_WRITE_4(sc, RGE_MAC0,
	    (uint32_t)addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0]);
	RGE_WRITE_4(sc, RGE_MAC4,
	    addr[5] <<  8 | addr[4]);
	RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
}

static void
rge_get_macaddr(struct rge_softc *sc, uint8_t *addr)
{
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		addr[i] = RGE_READ_1(sc, RGE_ADDR0 + i);
}

static void
rge_hw_init(struct rge_softc *sc)
{
	uint16_t reg;

	rge_disable_aspm_clkreq(sc);
	RGE_CLRBIT_1(sc, 0xf1, 0x80);

	/* Disable UPS. */
	RGE_MAC_CLRBIT(sc, 0xd40a, 0x0010);

	/* Disable MAC MCU. */
	rge_disable_aspm_clkreq(sc);
	rge_write_mac_ocp(sc, 0xfc48, 0);
	for (reg = 0xfc28; reg < 0xfc48; reg += 2)
		rge_write_mac_ocp(sc, reg, 0);
	DELAY(3000);
	rge_write_mac_ocp(sc, 0xfc26, 0);

	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27)
		rge_mac_config_ext_mcu(sc, sc->rge_type);
	else
		rge_mac_config_mcu(sc, sc->rge_type);

	/* Disable PHY power saving. */
	if (sc->rge_type == MAC_R25)
		rge_disable_phy_ocp_pwrsave(sc);

	/* Set PCIe uncorrectable error status. */
	rge_write_csi(sc, 0x108,
	    rge_read_csi(sc, 0x108) | 0x00100000);
}

void
rge_hw_reset(struct rge_softc *sc)
{
	/* Disable interrupts */
	RGE_WRITE_4(sc, RGE_IMR, 0);
	RGE_WRITE_4(sc, RGE_ISR, RGE_READ_4(sc, RGE_ISR));

	/* Clear timer interrupts. */
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT1, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT2, 0);
	RGE_WRITE_4(sc, RGE_TIMERINT3, 0);

	rge_reset(sc);
}

static void
rge_disable_phy_ocp_pwrsave(struct rge_softc *sc)
{
	if (rge_read_phy_ocp(sc, 0xc416) != 0x0500) {
		rge_patch_phy_mcu(sc, 1);
		rge_write_phy_ocp(sc, 0xc416, 0);
		rge_write_phy_ocp(sc, 0xc416, 0x0500);
		rge_patch_phy_mcu(sc, 0);
	}
}

static void
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

static void
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

	if (RGE_TYPE_R26(sc)) {
		ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_5000_T, 0, NULL);
		ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_5000_T | IFM_FDX,
		    0, NULL);
	} else if (sc->rge_type == MAC_R27) {
		ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10G_T, 0, NULL);
		ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10G_T | IFM_FDX,
		    0, NULL);
	}
}

static void
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

static void
rge_disable_aspm_clkreq(struct rge_softc *sc)
{
	int unlock = 1;

	if ((RGE_READ_1(sc, RGE_EECMD) & RGE_EECMD_WRITECFG) ==
	    RGE_EECMD_WRITECFG)
		unlock = 0;

	if (unlock)
		RGE_SETBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);

	if (RGE_TYPE_R26(sc) || sc->rge_type == MAC_R27)
		RGE_CLRBIT_1(sc, RGE_INT_CFG0, 0x08);
	else
		RGE_CLRBIT_1(sc, RGE_CFG2, RGE_CFG2_CLKREQ_EN);
	RGE_CLRBIT_1(sc, RGE_CFG5, RGE_CFG5_PME_STS);

	if (unlock)
		RGE_CLRBIT_1(sc, RGE_EECMD, RGE_EECMD_WRITECFG);
}

static void
rge_disable_hw_im(struct rge_softc *sc)
{
	RGE_WRITE_2(sc, RGE_IM, 0);
}

static void
rge_disable_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0);
	sc->rge_timerintr = 0;
}

static void
rge_setup_sim_im(struct rge_softc *sc)
{
	RGE_WRITE_4(sc, RGE_TIMERINT0, 0x2600);
	RGE_WRITE_4(sc, RGE_TIMERCNT, 1);
	sc->rge_timerintr = 1;
}

static void
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

static void
rge_switch_mcu_ram_page(struct rge_softc *sc, int page)
{
	uint16_t val;

	val = rge_read_mac_ocp(sc, 0xe446) & ~0x0003;
	val |= page;
	rge_write_mac_ocp(sc, 0xe446, val);
}

static int
rge_exit_oob(struct rge_softc *sc)
{
	int error, i;

	/* Disable RealWoW. */
	rge_write_mac_ocp(sc, 0xc0bc, 0x00ff);

	if ((error = rge_reset(sc)) != 0)
		return error;

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
		for (i = 0; i < RGE_TIMEOUT; i++) {
			if ((rge_read_phy_ocp(sc, 0xa420) & 0x0007) == 2)
				break;
			DELAY(1000);
		}
		RGE_MAC_CLRBIT(sc, 0xd42c, 0x0100);
		if (sc->rge_type != MAC_R25)
			RGE_PHY_CLRBIT(sc, 0xa466, 0x0001);
		RGE_PHY_CLRBIT(sc, 0xa468, 0x000a);
	}

	return 0;
}

static void
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

static uint32_t
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

static void
rge_write_mac_ocp(struct rge_softc *sc, uint16_t reg, uint16_t val)
{
	uint32_t tmp;

	tmp = (reg >> 1) << RGE_MACOCP_ADDR_SHIFT;
	tmp += val;
	tmp |= RGE_MACOCP_BUSY;
	RGE_WRITE_4(sc, RGE_MACOCP, tmp);
}

static uint16_t
rge_read_mac_ocp(struct rge_softc *sc, uint16_t reg)
{
	uint32_t val;

	val = (reg >> 1) << RGE_MACOCP_ADDR_SHIFT;
	RGE_WRITE_4(sc, RGE_MACOCP, val);

	return (RGE_READ_4(sc, RGE_MACOCP) & RGE_MACOCP_DATA_MASK);
}

static void
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

static uint16_t
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

static uint16_t
rge_check_ephy_ext_add(struct rge_softc *sc, uint16_t reg)
{
	uint16_t val;

	val = (reg >> 12);
	rge_write_ephy(sc, RGE_EPHYAR_EXT_ADDR, val);

	return reg & 0x0fff;
}

static void
rge_r27_write_ephy(struct rge_softc *sc, uint16_t reg, uint16_t val)
{
	rge_write_ephy(sc, rge_check_ephy_ext_add(sc, reg), val);
}

static void
rge_write_phy(struct rge_softc *sc, uint16_t addr, uint16_t reg, uint16_t val)
{
	uint16_t off, phyaddr;

	phyaddr = addr ? addr : RGE_PHYBASE + (reg / 8);
	phyaddr <<= 4;

	off = addr ? reg : 0x10 + (reg % 8);

	phyaddr += (off - 16) << 1;

	rge_write_phy_ocp(sc, phyaddr, val);
}

static uint16_t
rge_read_phy(struct rge_softc *sc, uint16_t addr, uint16_t reg)
{
	uint16_t off, phyaddr;

	phyaddr = addr ? addr : RGE_PHYBASE + (reg / 8);
	phyaddr <<= 4;

	off = addr ? reg : 0x10 + (reg % 8);

	phyaddr += (off - 16) << 1;

	return (rge_read_phy_ocp(sc, phyaddr));
}

static void
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

static uint16_t
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

static int
rge_get_link_status(struct rge_softc *sc)
{
	return ((RGE_READ_2(sc, RGE_PHYSTAT) & RGE_PHYSTAT_LINK) ? 1 : 0);
}

static void
rge_txstart(void *arg)
{
	struct rge_softc *sc = arg;

	RGE_WRITE_2(sc, RGE_TXSTART, RGE_TXSTART_START);
}

static void
rge_tick(void *arg)
{
	struct rge_softc *sc = arg;
	int s;

	s = splnet();
	rge_link_state(sc);
	splx(s);

	callout_schedule(&sc->sc_timeout, hz);
}

static void
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

/* Module interface */

MODULE(MODULE_CLASS_DRIVER, if_rge, "pci");
 
#ifdef _MODULE
#include "ioconf.c" 
#endif 
 
static int
if_rge_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;
 
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_rge,
		    cfattach_ioconf_rge, cfdata_ioconf_rge);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_rge,
		    cfattach_ioconf_rge, cfdata_ioconf_rge);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
