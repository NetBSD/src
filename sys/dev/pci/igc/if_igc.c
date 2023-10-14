/*	$NetBSD: if_igc.c,v 1.3.2.3 2023/10/14 06:49:37 martin Exp $	*/
/*	$OpenBSD: if_igc.c,v 1.13 2023/04/28 10:18:57 bluhm Exp $	*/
/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2016 Nicole Graziano <nicole@nextbsd.org>
 * All rights reserved.
 * Copyright (c) 2021 Rubicon Communications, LLC (Netgate)
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
__KERNEL_RCSID(0, "$NetBSD: if_igc.c,v 1.3.2.3 2023/10/14 06:49:37 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#include "opt_if_igc.h"
#if 0 /* notyet */
#include "vlan.h"
#endif
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/intr.h>
#include <sys/interrupt.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/socket.h>
#include <sys/workqueue.h>
#include <sys/xcall.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_vlanvar.h>
#include <net/rss_config.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/igc/if_igc.h>
#include <dev/pci/igc/igc_evcnt.h>
#include <dev/pci/igc/igc_hw.h>
#include <dev/mii/miivar.h>

#define IGC_WORKQUEUE_PRI	PRI_SOFTNET

#ifndef IGC_RX_INTR_PROCESS_LIMIT_DEFAULT
#define IGC_RX_INTR_PROCESS_LIMIT_DEFAULT	0
#endif
#ifndef IGC_TX_INTR_PROCESS_LIMIT_DEFAULT
#define IGC_TX_INTR_PROCESS_LIMIT_DEFAULT	0
#endif

#ifndef IGC_RX_PROCESS_LIMIT_DEFAULT
#define IGC_RX_PROCESS_LIMIT_DEFAULT		256
#endif
#ifndef IGC_TX_PROCESS_LIMIT_DEFAULT
#define IGC_TX_PROCESS_LIMIT_DEFAULT		256
#endif

#define	htolem32(p, x)	(*((uint32_t *)(p)) = htole32(x))
#define	htolem64(p, x)	(*((uint64_t *)(p)) = htole64(x))

static const struct igc_product {
	pci_vendor_id_t		igcp_vendor;
	pci_product_id_t	igcp_product;
	const char		*igcp_name;
} igc_products[] = {
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_IT,
	    "Intel(R) Ethernet Controller I225-IT(2)" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_LM,
	    "Intel(R) Ethernet Controller I226-LM" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_V,
	    "Intel(R) Ethernet Controller I226-V" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_IT,
	    "Intel(R) Ethernet Controller I226-IT" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I221_V,
	    "Intel(R) Ethernet Controller I221-V" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_BLANK_NVM,
	    "Intel(R) Ethernet Controller I226(blankNVM)" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_LM,
	    "Intel(R) Ethernet Controller I225-LM" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_V,
	    "Intel(R) Ethernet Controller I225-V" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I220_V,
	    "Intel(R) Ethernet Controller I220-V" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_I,
	    "Intel(R) Ethernet Controller I225-I" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_BLANK_NVM,
	    "Intel(R) Ethernet Controller I225(blankNVM)" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_K,
	    "Intel(R) Ethernet Controller I225-K" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_K2,
	    "Intel(R) Ethernet Controller I225-K(2)" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_K,
	    "Intel(R) Ethernet Controller I226-K" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I225_LMVP,
	    "Intel(R) Ethernet Controller I225-LMvP(2)" },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_I226_LMVP,
	    "Intel(R) Ethernet Controller I226-LMvP" },
	{ 0, 0, NULL },
};

#define	IGC_DF_CFG	0x1
#define	IGC_DF_TX	0x2
#define	IGC_DF_RX	0x4
#define	IGC_DF_MISC	0x8

#ifdef IGC_DEBUG_FLAGS
int igc_debug_flags = IGC_DEBUG_FLAGS;
#else
int igc_debug_flags = 0;
#endif

#define	DPRINTF(flag, fmt, args...)		do {			\
	if (igc_debug_flags & (IGC_DF_ ## flag))			\
		printf("%s: %d: " fmt, __func__, __LINE__, ##args);	\
    } while (0)

/*********************************************************************
 *  Function Prototypes
 *********************************************************************/
static int	igc_match(device_t, cfdata_t, void *);
static void	igc_attach(device_t, device_t, void *);
static int	igc_detach(device_t, int);

static void	igc_identify_hardware(struct igc_softc *);
static int	igc_adjust_nqueues(struct igc_softc *);
static int	igc_allocate_pci_resources(struct igc_softc *);
static int	igc_allocate_interrupts(struct igc_softc *);
static int	igc_allocate_queues(struct igc_softc *);
static void	igc_free_pci_resources(struct igc_softc *);
static void	igc_free_interrupts(struct igc_softc *);
static void	igc_free_queues(struct igc_softc *);
static void	igc_reset(struct igc_softc *);
static void	igc_init_dmac(struct igc_softc *, uint32_t);
static int	igc_setup_interrupts(struct igc_softc *);
static void	igc_attach_counters(struct igc_softc *sc);
static void	igc_detach_counters(struct igc_softc *sc);
static void	igc_update_counters(struct igc_softc *sc);
static void	igc_clear_counters(struct igc_softc *sc);
static int	igc_setup_msix(struct igc_softc *);
static int	igc_setup_msi(struct igc_softc *);
static int	igc_setup_intx(struct igc_softc *);
static int	igc_dma_malloc(struct igc_softc *, bus_size_t,
		    struct igc_dma_alloc *);
static void	igc_dma_free(struct igc_softc *, struct igc_dma_alloc *);
static void	igc_setup_interface(struct igc_softc *);

static int	igc_init(struct ifnet *);
static int	igc_init_locked(struct igc_softc *);
static void	igc_start(struct ifnet *);
static int	igc_transmit(struct ifnet *, struct mbuf *);
static void	igc_tx_common_locked(struct ifnet *, struct tx_ring *, int);
static bool	igc_txeof(struct tx_ring *, u_int);
static void	igc_intr_barrier(struct igc_softc *);
static void	igc_stop(struct ifnet *, int);
static void	igc_stop_locked(struct igc_softc *);
static int	igc_ioctl(struct ifnet *, u_long, void *);
#ifdef IF_RXR
static int	igc_rxrinfo(struct igc_softc *, struct if_rxrinfo *);
#endif
static void	igc_rxfill(struct rx_ring *);
static void	igc_rxrefill(struct rx_ring *, int);
static bool	igc_rxeof(struct rx_ring *, u_int);
static int	igc_rx_checksum(struct igc_queue *, uint64_t, uint32_t,
		    uint32_t);
static void	igc_watchdog(struct ifnet *);
static void	igc_tick(void *);
static void	igc_media_status(struct ifnet *, struct ifmediareq *);
static int	igc_media_change(struct ifnet *);
static int	igc_ifflags_cb(struct ethercom *);
static void	igc_set_filter(struct igc_softc *);
static void	igc_update_link_status(struct igc_softc *);
static int	igc_get_buf(struct rx_ring *, int, bool);
static int	igc_tx_ctx_setup(struct tx_ring *, struct mbuf *, int,
		    uint32_t *, uint32_t *);
static int	igc_tso_setup(struct tx_ring *, struct mbuf *, int,
		    uint32_t *, uint32_t *);

static void	igc_configure_queues(struct igc_softc *);
static void	igc_set_queues(struct igc_softc *, uint32_t, uint32_t, int);
static void	igc_enable_queue(struct igc_softc *, uint32_t);
static void	igc_enable_intr(struct igc_softc *);
static void	igc_disable_intr(struct igc_softc *);
static int	igc_intr_link(void *);
static int	igc_intr_queue(void *);
static int	igc_intr(void *);
static void	igc_handle_queue(void *);
static void	igc_handle_queue_work(struct work *, void *);
static void	igc_sched_handle_queue(struct igc_softc *, struct igc_queue *);
static void	igc_barrier_handle_queue(struct igc_softc *);

static int	igc_allocate_transmit_buffers(struct tx_ring *);
static int	igc_setup_transmit_structures(struct igc_softc *);
static int	igc_setup_transmit_ring(struct tx_ring *);
static void	igc_initialize_transmit_unit(struct igc_softc *);
static void	igc_free_transmit_structures(struct igc_softc *);
static void	igc_free_transmit_buffers(struct tx_ring *);
static void	igc_withdraw_transmit_packets(struct tx_ring *, bool);
static int	igc_allocate_receive_buffers(struct rx_ring *);
static int	igc_setup_receive_structures(struct igc_softc *);
static int	igc_setup_receive_ring(struct rx_ring *);
static void	igc_initialize_receive_unit(struct igc_softc *);
static void	igc_free_receive_structures(struct igc_softc *);
static void	igc_free_receive_buffers(struct rx_ring *);
static void	igc_clear_receive_status(struct rx_ring *);
static void	igc_initialize_rss_mapping(struct igc_softc *);

static void	igc_get_hw_control(struct igc_softc *);
static void	igc_release_hw_control(struct igc_softc *);
static int	igc_is_valid_ether_addr(uint8_t *);
static void	igc_print_devinfo(struct igc_softc *);

CFATTACH_DECL3_NEW(igc, sizeof(struct igc_softc),
    igc_match, igc_attach, igc_detach, NULL, NULL, NULL, 0);

static inline int
igc_txdesc_incr(struct igc_softc *sc, int id)
{

	if (++id == sc->num_tx_desc)
		id = 0;
	return id;
}

static inline int __unused
igc_txdesc_decr(struct igc_softc *sc, int id)
{

	if (--id < 0)
		id = sc->num_tx_desc - 1;
	return id;
}

static inline void
igc_txdesc_sync(struct tx_ring *txr, int id, int ops)
{

	bus_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    id * sizeof(union igc_adv_tx_desc), sizeof(union igc_adv_tx_desc),
	    ops);
}

static inline int
igc_rxdesc_incr(struct igc_softc *sc, int id)
{

	if (++id == sc->num_rx_desc)
		id = 0;
	return id;
}

static inline int
igc_rxdesc_decr(struct igc_softc *sc, int id)
{

	if (--id < 0)
		id = sc->num_rx_desc - 1;
	return id;
}

static inline void
igc_rxdesc_sync(struct rx_ring *rxr, int id, int ops)
{

	bus_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
	    id * sizeof(union igc_adv_rx_desc), sizeof(union igc_adv_rx_desc),
	    ops);
}

static const struct igc_product *
igc_lookup(const struct pci_attach_args *pa)
{
	const struct igc_product *igcp;

	for (igcp = igc_products; igcp->igcp_name != NULL; igcp++) {
		if (PCI_VENDOR(pa->pa_id) == igcp->igcp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == igcp->igcp_product)
			return igcp;
	}
	return NULL;
}

/*********************************************************************
 *  Device identification routine
 *
 *  igc_match determines if the driver should be loaded on
 *  adapter based on PCI vendor/device id of the adapter.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/
static int
igc_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (igc_lookup(pa) != NULL)
		return 1;

	return 0;
}

/*********************************************************************
 *  Device initialization routine
 *
 *  The attach entry point is called when the driver is being loaded.
 *  This routine identifies the type of hardware, allocates all resources
 *  and initializes the hardware.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/
static void
igc_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct igc_softc *sc = device_private(self);
	struct igc_hw *hw = &sc->hw;

	const struct igc_product *igcp = igc_lookup(pa);
	KASSERT(igcp != NULL);
	pci_aprint_devinfo_fancy(pa, "Ethernet controller", igcp->igcp_name, 1);

	sc->sc_dev = self;
	callout_init(&sc->sc_tick_ch, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_tick_ch, igc_tick, sc);
	sc->sc_core_stopping = false;

	sc->osdep.os_sc = sc;
	sc->osdep.os_pa = *pa;
#ifdef __aarch64__
	/*
	 * XXX PR port-arm/57643
	 * 64-bit DMA does not work at least for LX2K with 32/64GB memory.
	 * smmu(4) support may be required.
	 */
	sc->osdep.os_dmat = pa->pa_dmat;
#else
	sc->osdep.os_dmat = pci_dma64_available(pa) ?
	    pa->pa_dmat64 : pa->pa_dmat;
#endif

	/* Determine hardware and mac info */
	igc_identify_hardware(sc);

	sc->num_tx_desc = IGC_DEFAULT_TXD;
	sc->num_rx_desc = IGC_DEFAULT_RXD;

	 /* Setup PCI resources */
	if (igc_allocate_pci_resources(sc)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate PCI resources\n");
		goto err_pci;
	}

	if (igc_allocate_interrupts(sc)) {
		aprint_error_dev(sc->sc_dev, "unable to allocate interrupts\n");
		goto err_pci;
	}

	/* Allocate TX/RX queues */
	if (igc_allocate_queues(sc)) {
		aprint_error_dev(sc->sc_dev, "unable to allocate queues\n");
		goto err_alloc_intr;
	}

	/* Do shared code initialization */
	if (igc_setup_init_funcs(hw, true)) {
		aprint_error_dev(sc->sc_dev, "unable to initialize\n");
		goto err_alloc_intr;
	}

	hw->mac.autoneg = DO_AUTO_NEG;
	hw->phy.autoneg_wait_to_complete = false;
	hw->phy.autoneg_advertised = AUTONEG_ADV_DEFAULT;

	/* Copper options. */
	if (hw->phy.media_type == igc_media_type_copper)
		hw->phy.mdix = AUTO_ALL_MODES;

	/* Set the max frame size. */
	sc->hw.mac.max_frame_size = 9234;

	/* Allocate multicast array memory. */
	sc->mta = kmem_alloc(IGC_MTA_LEN, KM_SLEEP);

	/* Check SOL/IDER usage. */
	if (igc_check_reset_block(hw)) {
		aprint_error_dev(sc->sc_dev,
		    "PHY reset is blocked due to SOL/IDER session\n");
	}

	/* Disable Energy Efficient Ethernet. */
	sc->hw.dev_spec._i225.eee_disable = true;

	igc_reset_hw(hw);

	/* Make sure we have a good EEPROM before we read from it. */
	if (igc_validate_nvm_checksum(hw) < 0) {
		/*
		 * Some PCI-E parts fail the first check due to
		 * the link being in sleep state, call it again,
		 * if it fails a second time its a real issue.
		 */
		if (igc_validate_nvm_checksum(hw) < 0) {
			aprint_error_dev(sc->sc_dev,
			    "EEPROM checksum invalid\n");
			goto err_late;
		}
	}

	/* Copy the permanent MAC address out of the EEPROM. */
	if (igc_read_mac_addr(hw) < 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to read MAC address from EEPROM\n");
		goto err_late;
	}

	if (!igc_is_valid_ether_addr(hw->mac.addr)) {
		aprint_error_dev(sc->sc_dev, "invalid MAC address\n");
		goto err_late;
	}

	if (igc_setup_interrupts(sc))
		goto err_late;

	/* Attach counters. */
	igc_attach_counters(sc);

	/* Setup OS specific network interface. */
	igc_setup_interface(sc);

	igc_print_devinfo(sc);

	igc_reset(sc);
	hw->mac.get_link_status = true;
	igc_update_link_status(sc);

	/* The driver can now take control from firmware. */
	igc_get_hw_control(sc);

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(sc->hw.mac.addr));

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, &sc->sc_ec.ec_if);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

 err_late:
	igc_release_hw_control(sc);
 err_alloc_intr:
	igc_free_interrupts(sc);
 err_pci:
	igc_free_pci_resources(sc);
	kmem_free(sc->mta, IGC_MTA_LEN);
}

/*********************************************************************
 *  Device removal routine
 *
 *  The detach entry point is called when the driver is being removed.
 *  This routine stops the adapter and deallocates all the resources
 *  that were allocated for driver operation.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/
static int
igc_detach(device_t self, int flags)
{
	struct igc_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	mutex_enter(&sc->sc_core_lock);
	igc_stop_locked(sc);
	mutex_exit(&sc->sc_core_lock);

	igc_detach_counters(sc);

	igc_free_queues(sc);

	igc_phy_hw_reset(&sc->hw);
	igc_release_hw_control(sc);

	ether_ifdetach(ifp);
	if_detach(ifp);
	ifmedia_fini(&sc->media);

	igc_free_interrupts(sc);
	igc_free_pci_resources(sc);
	kmem_free(sc->mta, IGC_MTA_LEN);

	mutex_destroy(&sc->sc_core_lock);

	return 0;
}

static void
igc_identify_hardware(struct igc_softc *sc)
{
	struct igc_osdep *os = &sc->osdep;
	struct pci_attach_args *pa = &os->os_pa;

	/* Save off the information about this board. */
	sc->hw.device_id = PCI_PRODUCT(pa->pa_id);

	/* Do shared code init and setup. */
	if (igc_set_mac_type(&sc->hw)) {
		aprint_error_dev(sc->sc_dev, "unable to identify hardware\n");
		return;
	}
}

static int
igc_allocate_pci_resources(struct igc_softc *sc)
{
	struct igc_osdep *os = &sc->osdep;
	struct pci_attach_args *pa = &os->os_pa;

	/*
	 * Enable bus mastering and memory-mapped I/O for sure.
	 */
	pcireg_t csr =
	    pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);

	const pcireg_t memtype =
	    pci_mapreg_type(pa->pa_pc, pa->pa_tag, IGC_PCIREG);
	if (pci_mapreg_map(pa, IGC_PCIREG, memtype, 0, &os->os_memt,
	    &os->os_memh, &os->os_membase, &os->os_memsize)) {
		aprint_error_dev(sc->sc_dev, "unable to map registers\n");
		return ENXIO;
	}

	sc->hw.hw_addr = os->os_membase;
	sc->hw.back = os;

	return 0;
}

static int __unused
igc_adjust_nqueues(struct igc_softc *sc)
{
	struct pci_attach_args *pa = &sc->osdep.os_pa;
	int nqueues = MIN(IGC_MAX_NQUEUES, ncpu);

	const int nmsix = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (nmsix <= 1)
		nqueues = 1;
	else if (nmsix < nqueues + 1)
		nqueues = nmsix - 1;

	return nqueues;
}

static int
igc_allocate_interrupts(struct igc_softc *sc)
{
	struct pci_attach_args *pa = &sc->osdep.os_pa;
	int error;

#ifndef IGC_DISABLE_MSIX
	const int nqueues = igc_adjust_nqueues(sc);
	if (nqueues > 1) {
		sc->sc_nintrs = nqueues + 1;
		error = pci_msix_alloc_exact(pa, &sc->sc_intrs, sc->sc_nintrs);
		if (!error) {
			sc->sc_nqueues = nqueues;
			sc->sc_intr_type = PCI_INTR_TYPE_MSIX;
			return 0;
		}
	}
#endif

	/* fallback to MSI */
	sc->sc_nintrs = sc->sc_nqueues = 1;

#ifndef IGC_DISABLE_MSI
	error = pci_msi_alloc_exact(pa, &sc->sc_intrs, sc->sc_nintrs);
	if (!error) {
		sc->sc_intr_type = PCI_INTR_TYPE_MSI;
		return 0;
	}
#endif

	/* fallback to INTx */

	error = pci_intx_alloc(pa, &sc->sc_intrs);
	if (!error) {
		sc->sc_intr_type = PCI_INTR_TYPE_INTX;
		return 0;
	}

	return error;
}

static int
igc_allocate_queues(struct igc_softc *sc)
{
	device_t dev = sc->sc_dev;
	int rxconf = 0, txconf = 0;

	/* Allocate the top level queue structs. */
	sc->queues =
	    kmem_zalloc(sc->sc_nqueues * sizeof(struct igc_queue), KM_SLEEP);

	/* Allocate the TX ring. */
	sc->tx_rings =
	    kmem_zalloc(sc->sc_nqueues * sizeof(struct tx_ring), KM_SLEEP);

	/* Allocate the RX ring. */
	sc->rx_rings =
	    kmem_zalloc(sc->sc_nqueues * sizeof(struct rx_ring), KM_SLEEP);

	/* Set up the TX queues. */
	for (int iq = 0; iq < sc->sc_nqueues; iq++, txconf++) {
		struct tx_ring *txr = &sc->tx_rings[iq];
		const int tsize = roundup2(
		    sc->num_tx_desc * sizeof(union igc_adv_tx_desc),
		    IGC_DBA_ALIGN);

		txr->sc = sc;
		txr->txr_igcq = &sc->queues[iq];
		txr->me = iq;
		if (igc_dma_malloc(sc, tsize, &txr->txdma)) {
			aprint_error_dev(dev,
			    "unable to allocate TX descriptor\n");
			goto fail;
		}
		txr->tx_base = (union igc_adv_tx_desc *)txr->txdma.dma_vaddr;
		memset(txr->tx_base, 0, tsize);
	}

	/* Prepare transmit descriptors and buffers. */
	if (igc_setup_transmit_structures(sc)) {
		aprint_error_dev(dev, "unable to setup transmit structures\n");
		goto fail;
	}

	/* Set up the RX queues. */
	for (int iq = 0; iq < sc->sc_nqueues; iq++, rxconf++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];
		const int rsize = roundup2(
		    sc->num_rx_desc * sizeof(union igc_adv_rx_desc),
		    IGC_DBA_ALIGN);

		rxr->sc = sc;
		rxr->rxr_igcq = &sc->queues[iq];
		rxr->me = iq;
#ifdef OPENBSD
		timeout_set(&rxr->rx_refill, igc_rxrefill, rxr);
#endif
		if (igc_dma_malloc(sc, rsize, &rxr->rxdma)) {
			aprint_error_dev(dev,
			    "unable to allocate RX descriptor\n");
			goto fail;
		}
		rxr->rx_base = (union igc_adv_rx_desc *)rxr->rxdma.dma_vaddr;
		memset(rxr->rx_base, 0, rsize);
	}

	sc->rx_mbuf_sz = MCLBYTES;
	/* Prepare receive descriptors and buffers. */
	if (igc_setup_receive_structures(sc)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to setup receive structures\n");
		goto fail;
	}

	/* Set up the queue holding structs. */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		q->sc = sc;
		q->txr = &sc->tx_rings[iq];
		q->rxr = &sc->rx_rings[iq];
	}

	return 0;

 fail:
	for (struct rx_ring *rxr = sc->rx_rings; rxconf > 0; rxr++, rxconf--)
		igc_dma_free(sc, &rxr->rxdma);
	for (struct tx_ring *txr = sc->tx_rings; txconf > 0; txr++, txconf--)
		igc_dma_free(sc, &txr->txdma);

	kmem_free(sc->rx_rings, sc->sc_nqueues * sizeof(struct rx_ring));
	sc->rx_rings = NULL;
	kmem_free(sc->tx_rings, sc->sc_nqueues * sizeof(struct tx_ring));
	sc->tx_rings = NULL;
	kmem_free(sc->queues, sc->sc_nqueues * sizeof(struct igc_queue));
	sc->queues = NULL;

	return ENOMEM;
}

static void
igc_free_pci_resources(struct igc_softc *sc)
{
	struct igc_osdep *os = &sc->osdep;

	if (os->os_membase != 0)
		bus_space_unmap(os->os_memt, os->os_memh, os->os_memsize);
	os->os_membase = 0;
}

static void
igc_free_interrupts(struct igc_softc *sc)
{
	struct pci_attach_args *pa = &sc->osdep.os_pa;
	pci_chipset_tag_t pc = pa->pa_pc;

	for (int i = 0; i < sc->sc_nintrs; i++) {
		if (sc->sc_ihs[i] != NULL) {
			pci_intr_disestablish(pc, sc->sc_ihs[i]);
			sc->sc_ihs[i] = NULL;
		}
	}
	pci_intr_release(pc, sc->sc_intrs, sc->sc_nintrs);
}

static void
igc_free_queues(struct igc_softc *sc)
{

	igc_free_receive_structures(sc);
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		igc_dma_free(sc, &rxr->rxdma);
	}

	igc_free_transmit_structures(sc);
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct tx_ring *txr = &sc->tx_rings[iq];

		igc_dma_free(sc, &txr->txdma);
	}

	kmem_free(sc->rx_rings, sc->sc_nqueues * sizeof(struct rx_ring));
	kmem_free(sc->tx_rings, sc->sc_nqueues * sizeof(struct tx_ring));
	kmem_free(sc->queues, sc->sc_nqueues * sizeof(struct igc_queue));
}

/*********************************************************************
 *
 *  Initialize the hardware to a configuration as specified by the
 *  adapter structure.
 *
 **********************************************************************/
static void
igc_reset(struct igc_softc *sc)
{
	struct igc_hw *hw = &sc->hw;

	/* Let the firmware know the OS is in control */
	igc_get_hw_control(sc);

	/*
	 * Packet Buffer Allocation (PBA)
	 * Writing PBA sets the receive portion of the buffer
	 * the remainder is used for the transmit buffer.
	 */
	const uint32_t pba = IGC_PBA_34K;

	/*
	 * These parameters control the automatic generation (Tx) and
	 * response (Rx) to Ethernet PAUSE frames.
	 * - High water mark should allow for at least two frames to be
	 *   received after sending an XOFF.
	 * - Low water mark works best when it is very near the high water mark.
	 *   This allows the receiver to restart by sending XON when it has
	 *   drained a bit. Here we use an arbitrary value of 1500 which will
	 *   restart after one full frame is pulled from the buffer. There
	 *   could be several smaller frames in the buffer and if so they will
	 *   not trigger the XON until their total number reduces the buffer
	 *   by 1500.
	 * - The pause time is fairly large at 1000 x 512ns = 512 usec.
	 */
	const uint16_t rx_buffer_size = (pba & 0xffff) << 10;

	hw->fc.high_water = rx_buffer_size -
	    roundup2(sc->hw.mac.max_frame_size, 1024);
	/* 16-byte granularity */
	hw->fc.low_water = hw->fc.high_water - 16;

	if (sc->fc) /* locally set flow control value? */
		hw->fc.requested_mode = sc->fc;
	else
		hw->fc.requested_mode = igc_fc_full;

	hw->fc.pause_time = IGC_FC_PAUSE_TIME;

	hw->fc.send_xon = true;

	/* Issue a global reset */
	igc_reset_hw(hw);
	IGC_WRITE_REG(hw, IGC_WUC, 0);

	/* and a re-init */
	if (igc_init_hw(hw) < 0) {
		aprint_error_dev(sc->sc_dev, "unable to reset hardware\n");
		return;
	}

	/* Setup DMA Coalescing */
	igc_init_dmac(sc, pba);

	IGC_WRITE_REG(hw, IGC_VET, ETHERTYPE_VLAN);
	igc_get_phy_info(hw);
	igc_check_for_link(hw);
}

/*********************************************************************
 *
 *  Initialize the DMA Coalescing feature
 *
 **********************************************************************/
static void
igc_init_dmac(struct igc_softc *sc, uint32_t pba)
{
	struct igc_hw *hw = &sc->hw;
	const uint16_t max_frame_size = sc->hw.mac.max_frame_size;
	uint32_t reg, status;

	if (sc->dmac == 0) { /* Disabling it */
		reg = ~IGC_DMACR_DMAC_EN;	/* XXXRO */
		IGC_WRITE_REG(hw, IGC_DMACR, reg);
		DPRINTF(MISC, "DMA coalescing disabled\n");
		return;
	} else {
		device_printf(sc->sc_dev, "DMA coalescing enabled\n");
	}

	/* Set starting threshold */
	IGC_WRITE_REG(hw, IGC_DMCTXTH, 0);

	uint16_t hwm = 64 * pba - max_frame_size / 16;
	if (hwm < 64 * (pba - 6))
		hwm = 64 * (pba - 6);
	reg = IGC_READ_REG(hw, IGC_FCRTC);
	reg &= ~IGC_FCRTC_RTH_COAL_MASK;
	reg |= (hwm << IGC_FCRTC_RTH_COAL_SHIFT) & IGC_FCRTC_RTH_COAL_MASK;
	IGC_WRITE_REG(hw, IGC_FCRTC, reg);

	uint32_t dmac = pba - max_frame_size / 512;
	if (dmac < pba - 10)
		dmac = pba - 10;
	reg = IGC_READ_REG(hw, IGC_DMACR);
	reg &= ~IGC_DMACR_DMACTHR_MASK;
	reg |= (dmac << IGC_DMACR_DMACTHR_SHIFT) & IGC_DMACR_DMACTHR_MASK;

	/* transition to L0x or L1 if available..*/
	reg |= IGC_DMACR_DMAC_EN | IGC_DMACR_DMAC_LX_MASK;

	/* Check if status is 2.5Gb backplane connection
	 * before configuration of watchdog timer, which is
	 * in msec values in 12.8usec intervals
	 * watchdog timer= msec values in 32usec intervals
	 * for non 2.5Gb connection
	 */
	status = IGC_READ_REG(hw, IGC_STATUS);
	if ((status & IGC_STATUS_2P5_SKU) &&
	    !(status & IGC_STATUS_2P5_SKU_OVER))
		reg |= (sc->dmac * 5) >> 6;
	else
		reg |= sc->dmac >> 5;

	IGC_WRITE_REG(hw, IGC_DMACR, reg);

	IGC_WRITE_REG(hw, IGC_DMCRTRH, 0);

	/* Set the interval before transition */
	reg = IGC_READ_REG(hw, IGC_DMCTLX);
	reg |= IGC_DMCTLX_DCFLUSH_DIS;

	/*
	 * in 2.5Gb connection, TTLX unit is 0.4 usec
	 * which is 0x4*2 = 0xA. But delay is still 4 usec
	 */
	status = IGC_READ_REG(hw, IGC_STATUS);
	if ((status & IGC_STATUS_2P5_SKU) &&
	    !(status & IGC_STATUS_2P5_SKU_OVER))
		reg |= 0xA;
	else
		reg |= 0x4;

	IGC_WRITE_REG(hw, IGC_DMCTLX, reg);

	/* free space in tx packet buffer to wake from DMA coal */
	IGC_WRITE_REG(hw, IGC_DMCTXTH,
	    (IGC_TXPBSIZE - (2 * max_frame_size)) >> 6);

	/* make low power state decision controlled by DMA coal */
	reg = IGC_READ_REG(hw, IGC_PCIEMISC);
	reg &= ~IGC_PCIEMISC_LX_DECISION;
	IGC_WRITE_REG(hw, IGC_PCIEMISC, reg);
}

static int
igc_setup_interrupts(struct igc_softc *sc)
{
	int error;

	switch (sc->sc_intr_type) {
	case PCI_INTR_TYPE_MSIX:
		error = igc_setup_msix(sc);
		break;
	case PCI_INTR_TYPE_MSI:
		error = igc_setup_msi(sc);
		break;
	case PCI_INTR_TYPE_INTX:
		error = igc_setup_intx(sc);
		break;
	default:
		panic("%s: invalid interrupt type: %d",
		    device_xname(sc->sc_dev), sc->sc_intr_type);
	}

	return error;
}

static void
igc_attach_counters(struct igc_softc *sc)
{
#ifdef IGC_EVENT_COUNTERS

	/* Global counters */
	sc->sc_global_evcnts = kmem_zalloc(
	    IGC_GLOBAL_COUNTERS * sizeof(sc->sc_global_evcnts[0]), KM_SLEEP);

	for (int cnt = 0; cnt < IGC_GLOBAL_COUNTERS; cnt++) {
		evcnt_attach_dynamic(&sc->sc_global_evcnts[cnt],
		    igc_global_counters[cnt].type, NULL,
		    device_xname(sc->sc_dev), igc_global_counters[cnt].name);
	}

	/* Driver counters */
	sc->sc_driver_evcnts = kmem_zalloc(
	    IGC_DRIVER_COUNTERS * sizeof(sc->sc_driver_evcnts[0]), KM_SLEEP);

	for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++) {
		evcnt_attach_dynamic(&sc->sc_driver_evcnts[cnt],
		    igc_driver_counters[cnt].type, NULL,
		    device_xname(sc->sc_dev), igc_driver_counters[cnt].name);
	}

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		q->igcq_driver_counters = kmem_zalloc(
		    IGC_DRIVER_COUNTERS * sizeof(q->igcq_driver_counters[0]),
		    KM_SLEEP);
	}

	/* Queue counters */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		snprintf(q->igcq_queue_evname, sizeof(q->igcq_queue_evname),
		    "%s q%d", device_xname(sc->sc_dev), iq);

		q->igcq_queue_evcnts = kmem_zalloc(
		    IGC_QUEUE_COUNTERS * sizeof(q->igcq_queue_evcnts[0]),
		    KM_SLEEP);

		for (int cnt = 0; cnt < IGC_QUEUE_COUNTERS; cnt++) {
			evcnt_attach_dynamic(&q->igcq_queue_evcnts[cnt],
			    igc_queue_counters[cnt].type, NULL,
			    q->igcq_queue_evname, igc_queue_counters[cnt].name);
		}
	}

	/* MAC counters */
	snprintf(sc->sc_mac_evname, sizeof(sc->sc_mac_evname),
	    "%s Mac Statistics", device_xname(sc->sc_dev));

	sc->sc_mac_evcnts = kmem_zalloc(
	    IGC_MAC_COUNTERS * sizeof(sc->sc_mac_evcnts[0]), KM_SLEEP);

	for (int cnt = 0; cnt < IGC_MAC_COUNTERS; cnt++) {
		evcnt_attach_dynamic(&sc->sc_mac_evcnts[cnt], EVCNT_TYPE_MISC,
		    NULL, sc->sc_mac_evname, igc_mac_counters[cnt].name);
	}
#endif
}

static void
igc_detach_counters(struct igc_softc *sc)
{
#ifdef IGC_EVENT_COUNTERS

	/* Global counters */
	for (int cnt = 0; cnt < IGC_GLOBAL_COUNTERS; cnt++)
		evcnt_detach(&sc->sc_global_evcnts[cnt]);

	kmem_free(sc->sc_global_evcnts,
	    IGC_GLOBAL_COUNTERS * sizeof(sc->sc_global_evcnts));

	/* Driver counters */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		kmem_free(q->igcq_driver_counters,
		    IGC_DRIVER_COUNTERS * sizeof(q->igcq_driver_counters[0]));
	}

	for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++)
		evcnt_detach(&sc->sc_driver_evcnts[cnt]);

	kmem_free(sc->sc_driver_evcnts,
	    IGC_DRIVER_COUNTERS * sizeof(sc->sc_driver_evcnts));

	/* Queue counters */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		for (int cnt = 0; cnt < IGC_QUEUE_COUNTERS; cnt++)
			evcnt_detach(&q->igcq_queue_evcnts[cnt]);

		kmem_free(q->igcq_queue_evcnts,
		    IGC_QUEUE_COUNTERS * sizeof(q->igcq_queue_evcnts[0]));
	}

	/* MAC statistics */
	for (int cnt = 0; cnt < IGC_MAC_COUNTERS; cnt++)
		evcnt_detach(&sc->sc_mac_evcnts[cnt]);

	kmem_free(sc->sc_mac_evcnts,
	    IGC_MAC_COUNTERS * sizeof(sc->sc_mac_evcnts[0]));
#endif
}

/*
 * XXX
 * FreeBSD uses 4-byte-wise read for 64-bit counters, while Linux just
 * drops hi words.
 */
static inline uint64_t __unused
igc_read_mac_counter(struct igc_hw *hw, bus_size_t reg, bool is64)
{
	uint64_t val;

	val = IGC_READ_REG(hw, reg);
	if (is64)
		val += ((uint64_t)IGC_READ_REG(hw, reg + 4)) << 32;
	return val;
}

static void
igc_update_counters(struct igc_softc *sc)
{
#ifdef IGC_EVENT_COUNTERS

	/* Global counters: nop */

	/* Driver counters */
	uint64_t sum[IGC_DRIVER_COUNTERS];

	memset(sum, 0, sizeof(sum));
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++) {
			sum[cnt] += IGC_QUEUE_DRIVER_COUNTER_VAL(q, cnt);
			IGC_QUEUE_DRIVER_COUNTER_STORE(q, cnt, 0);
		}
	}

	for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++)
		IGC_DRIVER_COUNTER_ADD(sc, cnt, sum[cnt]);

	/* Queue counters: nop */

	/* Mac statistics */
	struct igc_hw *hw = &sc->hw;

	for (int cnt = 0; cnt < IGC_MAC_COUNTERS; cnt++) {
		IGC_MAC_COUNTER_ADD(sc, cnt, igc_read_mac_counter(hw,
		    igc_mac_counters[cnt].reg, igc_mac_counters[cnt].is64));
	}
#endif
}

static void
igc_clear_counters(struct igc_softc *sc)
{
#ifdef IGC_EVENT_COUNTERS

	/* Global counters */
	for (int cnt = 0; cnt < IGC_GLOBAL_COUNTERS; cnt++)
		IGC_GLOBAL_COUNTER_STORE(sc, cnt, 0);

	/* Driver counters */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++)
			IGC_QUEUE_DRIVER_COUNTER_STORE(q, cnt, 0);
	}

	for (int cnt = 0; cnt < IGC_DRIVER_COUNTERS; cnt++)
		IGC_DRIVER_COUNTER_STORE(sc, cnt, 0);

	/* Queue counters */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		for (int cnt = 0; cnt < IGC_QUEUE_COUNTERS; cnt++)
			IGC_QUEUE_COUNTER_STORE(q, cnt, 0);
	}

	/* Mac statistics */
	struct igc_hw *hw = &sc->hw;

	for (int cnt = 0; cnt < IGC_MAC_COUNTERS; cnt++) {
		(void)igc_read_mac_counter(hw, igc_mac_counters[cnt].reg,
		    igc_mac_counters[cnt].is64);
		IGC_MAC_COUNTER_STORE(sc, cnt, 0);
	}
#endif
}

static int
igc_setup_msix(struct igc_softc *sc)
{
	pci_chipset_tag_t pc = sc->osdep.os_pa.pa_pc;
	device_t dev = sc->sc_dev;
	pci_intr_handle_t *intrs;
	void **ihs;
	const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
	char xnamebuf[MAX(32, MAXCOMLEN)];
	int iq, error;

	for (iq = 0, intrs = sc->sc_intrs, ihs = sc->sc_ihs;
	    iq < sc->sc_nqueues; iq++, intrs++, ihs++) {
		struct igc_queue *q = &sc->queues[iq];

		snprintf(xnamebuf, sizeof(xnamebuf), "%s: txrx %d",
		    device_xname(dev), iq);

		intrstr = pci_intr_string(pc, *intrs, intrbuf, sizeof(intrbuf));

		pci_intr_setattr(pc, intrs, PCI_INTR_MPSAFE, true);
		*ihs = pci_intr_establish_xname(pc, *intrs, IPL_NET,
		    igc_intr_queue, q, xnamebuf);
		if (*ihs == NULL) {
			aprint_error_dev(dev,
			    "unable to establish txrx interrupt at %s\n",
			    intrstr);
			return ENOBUFS;
		}
		aprint_normal_dev(dev, "txrx interrupting at %s\n", intrstr);

		kcpuset_t *affinity;
		kcpuset_create(&affinity, true);
		kcpuset_set(affinity, iq % ncpu);
		error = interrupt_distribute(*ihs, affinity, NULL);
		if (error) {
			aprint_normal_dev(dev,
			    "%s: unable to change affinity, use default CPU\n",
			    intrstr);
		}
		kcpuset_destroy(affinity);

		q->igcq_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
		    igc_handle_queue, q);
		if (q->igcq_si == NULL) {
			aprint_error_dev(dev,
			    "%s: unable to establish softint\n", intrstr);
			return ENOBUFS;
		}

		q->msix = iq;
		q->eims = 1 << iq;
	}

	snprintf(xnamebuf, MAXCOMLEN, "%s_tx_rx", device_xname(dev));
	error = workqueue_create(&sc->sc_queue_wq, xnamebuf,
	    igc_handle_queue_work, sc, IGC_WORKQUEUE_PRI, IPL_NET,
	    WQ_PERCPU | WQ_MPSAFE);
	if (error) {
		aprint_error_dev(dev, "workqueue_create failed\n");
		return ENOBUFS;
	}
	sc->sc_txrx_workqueue = false;

	intrstr = pci_intr_string(pc, *intrs, intrbuf, sizeof(intrbuf));
	snprintf(xnamebuf, sizeof(xnamebuf), "%s: link", device_xname(dev));
	pci_intr_setattr(pc, intrs, PCI_INTR_MPSAFE, true);
	*ihs = pci_intr_establish_xname(pc, *intrs, IPL_NET,
	    igc_intr_link, sc, xnamebuf);
	if (*ihs == NULL) {
		aprint_error_dev(dev,
		    "unable to establish link interrupt at %s\n", intrstr);
		return ENOBUFS;
	}
	aprint_normal_dev(dev, "link interrupting at %s\n", intrstr);
	/* use later in igc_configure_queues() */
	sc->linkvec = iq;

	return 0;
}

static int
igc_setup_msi(struct igc_softc *sc)
{
	pci_chipset_tag_t pc = sc->osdep.os_pa.pa_pc;
	device_t dev = sc->sc_dev;
	pci_intr_handle_t *intr = sc->sc_intrs;
	void **ihs = sc->sc_ihs;
	const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
	char xnamebuf[MAX(32, MAXCOMLEN)];
	int error;

	intrstr = pci_intr_string(pc, *intr, intrbuf, sizeof(intrbuf));

	snprintf(xnamebuf, sizeof(xnamebuf), "%s: msi", device_xname(dev));
	pci_intr_setattr(pc, intr, PCI_INTR_MPSAFE, true);
	*ihs = pci_intr_establish_xname(pc, *intr, IPL_NET,
	    igc_intr, sc, xnamebuf);
	if (*ihs == NULL) {
		aprint_error_dev(dev,
		    "unable to establish interrupt at %s\n", intrstr);
		return ENOBUFS;
	}
	aprint_normal_dev(dev, "interrupting at %s\n", intrstr);

	struct igc_queue *iq = sc->queues;
	iq->igcq_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    igc_handle_queue, iq);
	if (iq->igcq_si == NULL) {
		aprint_error_dev(dev,
		    "%s: unable to establish softint\n", intrstr);
		return ENOBUFS;
	}

	snprintf(xnamebuf, MAXCOMLEN, "%s_tx_rx", device_xname(dev));
	error = workqueue_create(&sc->sc_queue_wq, xnamebuf,
	    igc_handle_queue_work, sc, IGC_WORKQUEUE_PRI, IPL_NET,
	    WQ_PERCPU | WQ_MPSAFE);
	if (error) {
		aprint_error_dev(dev, "workqueue_create failed\n");
		return ENOBUFS;
	}
	sc->sc_txrx_workqueue = false;

	sc->queues[0].msix = 0;
	sc->linkvec = 0;

	return 0;
}

static int
igc_setup_intx(struct igc_softc *sc)
{
	pci_chipset_tag_t pc = sc->osdep.os_pa.pa_pc;
	device_t dev = sc->sc_dev;
	pci_intr_handle_t *intr = sc->sc_intrs;
	void **ihs = sc->sc_ihs;
	const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
	char xnamebuf[32];

	intrstr = pci_intr_string(pc, *intr, intrbuf, sizeof(intrbuf));

	snprintf(xnamebuf, sizeof(xnamebuf), "%s:intx", device_xname(dev));
	pci_intr_setattr(pc, intr, PCI_INTR_MPSAFE, true);
	*ihs = pci_intr_establish_xname(pc, *intr, IPL_NET,
	    igc_intr, sc, xnamebuf);
	if (*ihs == NULL) {
		aprint_error_dev(dev,
		    "unable to establish interrupt at %s\n", intrstr);
		return ENOBUFS;
	}
	aprint_normal_dev(dev, "interrupting at %s\n", intrstr);

	struct igc_queue *iq = sc->queues;
	iq->igcq_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    igc_handle_queue, iq);
	if (iq->igcq_si == NULL) {
		aprint_error_dev(dev,
		    "%s: unable to establish softint\n", intrstr);
		return ENOBUFS;
	}

	/* create workqueue? */
	sc->sc_txrx_workqueue = false;

	sc->queues[0].msix = 0;
	sc->linkvec = 0;

	return 0;
}

static int
igc_dma_malloc(struct igc_softc *sc, bus_size_t size, struct igc_dma_alloc *dma)
{
	struct igc_osdep *os = &sc->osdep;

	dma->dma_tag = os->os_dmat;

	if (bus_dmamap_create(dma->dma_tag, size, 1, size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &dma->dma_map))
		return 1;
	if (bus_dmamem_alloc(dma->dma_tag, size, PAGE_SIZE, 0, &dma->dma_seg,
	    1, &dma->dma_nseg, BUS_DMA_WAITOK))
		goto destroy;
	/*
	 * XXXRO
	 *
	 * Coherent mapping for descriptors is required for now.
	 *
	 * Both TX and RX descriptors are 16-byte length, which is shorter
	 * than dcache lines on modern CPUs. Therefore, sync for a descriptor
	 * may overwrite DMA read for descriptors in the same cache line.
	 *
	 * Can't we avoid this by use cache-line-aligned descriptors at once?
	 */
	if (bus_dmamem_map(dma->dma_tag, &dma->dma_seg, dma->dma_nseg, size,
	    &dma->dma_vaddr, BUS_DMA_WAITOK | BUS_DMA_COHERENT /* XXXRO */))
		goto free;
	if (bus_dmamap_load(dma->dma_tag, dma->dma_map, dma->dma_vaddr, size,
	    NULL, BUS_DMA_WAITOK))
		goto unmap;

	dma->dma_size = size;

	return 0;
 unmap:
	bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, size);
 free:
	bus_dmamem_free(dma->dma_tag, &dma->dma_seg, dma->dma_nseg);
 destroy:
	bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
	dma->dma_map = NULL;
	dma->dma_tag = NULL;
	return 1;
}

static void
igc_dma_free(struct igc_softc *sc, struct igc_dma_alloc *dma)
{

	if (dma->dma_tag == NULL)
		return;

	if (dma->dma_map != NULL) {
		bus_dmamap_sync(dma->dma_tag, dma->dma_map, 0,
		    dma->dma_map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dma->dma_tag, dma->dma_map);
		bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, dma->dma_size);
		bus_dmamem_free(dma->dma_tag, &dma->dma_seg, dma->dma_nseg);
		bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
		dma->dma_map = NULL;
	}
}

/*********************************************************************
 *
 *  Setup networking device structure and register an interface.
 *
 **********************************************************************/
static void
igc_setup_interface(struct igc_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), sizeof(ifp->if_xname));
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = igc_ioctl;
	ifp->if_start = igc_start;
	if (sc->sc_nqueues > 1)
		ifp->if_transmit = igc_transmit;
	ifp->if_watchdog = igc_watchdog;
	ifp->if_init = igc_init;
	ifp->if_stop = igc_stop;

#if 0 /* notyet */
	ifp->if_capabilities = IFCAP_TSOv4 | IFCAP_TSOv6;
#endif

	ifp->if_capabilities |=
	    IFCAP_CSUM_IPv4_Tx  | IFCAP_CSUM_IPv4_Rx  |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_TCPv6_Rx |
	    IFCAP_CSUM_UDPv6_Tx | IFCAP_CSUM_UDPv6_Rx;

	ifp->if_capenable = 0;

	sc->sc_ec.ec_capabilities |=
	    ETHERCAP_JUMBO_MTU | ETHERCAP_VLAN_MTU;

	IFQ_SET_MAXLEN(&ifp->if_snd, sc->num_tx_desc - 1);
	IFQ_SET_READY(&ifp->if_snd);

#if NVLAN > 0
	sc->sc_ec.ec_capabilities |=  ETHERCAP_VLAN_HWTAGGING;
#endif

	mutex_init(&sc->sc_core_lock, MUTEX_DEFAULT, IPL_NET);

	/* Initialize ifmedia structures. */
	sc->sc_ec.ec_ifmedia = &sc->media;
	ifmedia_init_with_lock(&sc->media, IFM_IMASK, igc_media_change,
	    igc_media_status, &sc->sc_core_lock);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_10_T, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_10_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_100_TX, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_100_TX | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_1000_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_2500_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->media, IFM_ETHER | IFM_AUTO);

	sc->sc_rx_intr_process_limit = IGC_RX_INTR_PROCESS_LIMIT_DEFAULT;
	sc->sc_tx_intr_process_limit = IGC_TX_INTR_PROCESS_LIMIT_DEFAULT;
	sc->sc_rx_process_limit = IGC_RX_PROCESS_LIMIT_DEFAULT;
	sc->sc_tx_process_limit = IGC_TX_PROCESS_LIMIT_DEFAULT;

	if_initialize(ifp);
	sc->sc_ipq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->hw.mac.addr);
	ether_set_ifflags_cb(&sc->sc_ec, igc_ifflags_cb);
	if_register(ifp);
}

static int
igc_init(struct ifnet *ifp)
{
	struct igc_softc *sc = ifp->if_softc;
	int error;

	mutex_enter(&sc->sc_core_lock);
	error = igc_init_locked(sc);
	mutex_exit(&sc->sc_core_lock);

	return error;
}

static int
igc_init_locked(struct igc_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ifnet *ifp = &ec->ec_if;

	DPRINTF(CFG, "called\n");

	KASSERT(mutex_owned(&sc->sc_core_lock));

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		igc_stop_locked(sc);

	/* Put the address into the receive address array. */
	igc_rar_set(&sc->hw, sc->hw.mac.addr, 0);

	/* Initialize the hardware. */
	igc_reset(sc);
	igc_update_link_status(sc);

	/* Setup VLAN support, basic and offload if available. */
	IGC_WRITE_REG(&sc->hw, IGC_VET, ETHERTYPE_VLAN);

	igc_initialize_transmit_unit(sc);
	igc_initialize_receive_unit(sc);

	if (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING) {
		uint32_t ctrl = IGC_READ_REG(&sc->hw, IGC_CTRL);
		ctrl |= IGC_CTRL_VME;
		IGC_WRITE_REG(&sc->hw, IGC_CTRL, ctrl);
	}

	/* Setup multicast table. */
	igc_set_filter(sc);

	igc_clear_hw_cntrs_base_generic(&sc->hw);

	if (sc->sc_intr_type == PCI_INTR_TYPE_MSIX)
		igc_configure_queues(sc);

	/* This clears any pending interrupts */
	IGC_READ_REG(&sc->hw, IGC_ICR);
	IGC_WRITE_REG(&sc->hw, IGC_ICS, IGC_ICS_LSC);

	/* The driver can now take control from firmware. */
	igc_get_hw_control(sc);

	/* Set Energy Efficient Ethernet. */
	igc_set_eee_i225(&sc->hw, true, true, true);

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		mutex_enter(&rxr->rxr_lock);
		igc_rxfill(rxr);
		mutex_exit(&rxr->rxr_lock);
	}

	sc->sc_core_stopping = false;

	ifp->if_flags |= IFF_RUNNING;

	/* Save last flags for the callback */
	sc->sc_if_flags = ifp->if_flags;

	callout_schedule(&sc->sc_tick_ch, hz);

	igc_enable_intr(sc);

	return 0;
}

static inline int
igc_load_mbuf(struct igc_queue *q, bus_dma_tag_t dmat, bus_dmamap_t map,
    struct mbuf *m)
{
	int error;

	error = bus_dmamap_load_mbuf(dmat, map, m,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);

	if (__predict_false(error == EFBIG)) {
		IGC_DRIVER_EVENT(q, txdma_efbig, 1);
		m = m_defrag(m, M_NOWAIT);
		if (__predict_false(m == NULL)) {
			IGC_DRIVER_EVENT(q, txdma_defrag, 1);
			return ENOBUFS;
		}
		error = bus_dmamap_load_mbuf(dmat, map, m,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	}

	switch (error) {
	case 0:
		break;
	case ENOMEM:
		IGC_DRIVER_EVENT(q, txdma_enomem, 1);
		break;
	case EINVAL:
		IGC_DRIVER_EVENT(q, txdma_einval, 1);
		break;
	case EAGAIN:
		IGC_DRIVER_EVENT(q, txdma_eagain, 1);
		break;
	default:
		IGC_DRIVER_EVENT(q, txdma_other, 1);
		break;
	}

	return error;
}

#define IGC_TX_START	1
#define IGC_TX_TRANSMIT	2

static void
igc_start(struct ifnet *ifp)
{
	struct igc_softc *sc = ifp->if_softc;

	if (__predict_false(!sc->link_active)) {
		IFQ_PURGE(&ifp->if_snd);
		return;
	}

	struct tx_ring *txr = &sc->tx_rings[0]; /* queue 0 */
	mutex_enter(&txr->txr_lock);
	igc_tx_common_locked(ifp, txr, IGC_TX_START);
	mutex_exit(&txr->txr_lock);
}

static inline u_int
igc_select_txqueue(struct igc_softc *sc, struct mbuf *m __unused)
{
	const u_int cpuid = cpu_index(curcpu());

	return cpuid % sc->sc_nqueues;
}

static int
igc_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct igc_softc *sc = ifp->if_softc;
	const u_int qid = igc_select_txqueue(sc, m);
	struct tx_ring *txr = &sc->tx_rings[qid];
	struct igc_queue *q = txr->txr_igcq;

	if (__predict_false(!pcq_put(txr->txr_interq, m))) {
		IGC_QUEUE_EVENT(q, tx_pcq_drop, 1);
		m_freem(m);
		return ENOBUFS;
	}

	mutex_enter(&txr->txr_lock);
	igc_tx_common_locked(ifp, txr, IGC_TX_TRANSMIT);
	mutex_exit(&txr->txr_lock);

	return 0;
}

static void
igc_tx_common_locked(struct ifnet *ifp, struct tx_ring *txr, int caller)
{
	struct igc_softc *sc = ifp->if_softc;
	struct igc_queue *q = txr->txr_igcq;
	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	int prod, free, last = -1;
	bool post = false;

	prod = txr->next_avail_desc;
	free = txr->next_to_clean;
	if (free <= prod)
		free += sc->num_tx_desc;
	free -= prod;

	DPRINTF(TX, "%s: begin: msix %d prod %d n2c %d free %d\n",
	    caller == IGC_TX_TRANSMIT ? "transmit" : "start",
	    txr->me, prod, txr->next_to_clean, free);

	for (;;) {
		struct mbuf *m;

		if (__predict_false(free <= IGC_MAX_SCATTER)) {
			IGC_QUEUE_EVENT(q, tx_no_desc, 1);
			break;
		}

		if (caller == IGC_TX_TRANSMIT)
			m = pcq_get(txr->txr_interq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);
		if (__predict_false(m == NULL))
			break;

		struct igc_tx_buf *txbuf = &txr->tx_buffers[prod];
		bus_dmamap_t map = txbuf->map;

		if (__predict_false(
		    igc_load_mbuf(q, txr->txdma.dma_tag, map, m))) {
			if (caller == IGC_TX_TRANSMIT)
				IGC_QUEUE_EVENT(q, tx_pcq_drop, 1);
			m_freem(m);
			if_statinc_ref(nsr, if_oerrors);
			continue;
		}

		bus_dmamap_sync(txr->txdma.dma_tag, map, 0,
		    map->dm_mapsize, BUS_DMASYNC_PREWRITE);

		uint32_t ctx_cmd_type_len = 0, olinfo_status = 0;
		if (igc_tx_ctx_setup(txr, m, prod, &ctx_cmd_type_len,
		    &olinfo_status)) {
			IGC_QUEUE_EVENT(q, tx_ctx, 1);
			/* Consume the first descriptor */
			prod = igc_txdesc_incr(sc, prod);
			free--;
		}
		for (int i = 0; i < map->dm_nsegs; i++) {
			union igc_adv_tx_desc *txdesc = &txr->tx_base[prod];

			uint32_t cmd_type_len = ctx_cmd_type_len |
			    IGC_ADVTXD_DCMD_IFCS | IGC_ADVTXD_DTYP_DATA |
			    IGC_ADVTXD_DCMD_DEXT | map->dm_segs[i].ds_len;
			if (i == map->dm_nsegs - 1) {
				cmd_type_len |=
				    IGC_ADVTXD_DCMD_EOP | IGC_ADVTXD_DCMD_RS;
			}

			igc_txdesc_sync(txr, prod,
			    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
			htolem64(&txdesc->read.buffer_addr,
			    map->dm_segs[i].ds_addr);
			htolem32(&txdesc->read.cmd_type_len, cmd_type_len);
			htolem32(&txdesc->read.olinfo_status, olinfo_status);
			igc_txdesc_sync(txr, prod,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

			last = prod;
			prod = igc_txdesc_incr(sc, prod);
		}

		txbuf->m_head = m;
		txbuf->eop_index = last;

		bpf_mtap(ifp, m, BPF_D_OUT);

		if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
		if (m->m_flags & M_MCAST)
			if_statinc_ref(nsr, if_omcasts);
		IGC_QUEUE_EVENT(q, tx_packets, 1);
		IGC_QUEUE_EVENT(q, tx_bytes, m->m_pkthdr.len);

		free -= map->dm_nsegs;
		post = true;
	}

	if (post) {
		txr->next_avail_desc = prod;
		IGC_WRITE_REG(&sc->hw, IGC_TDT(txr->me), prod);
	}

	DPRINTF(TX, "%s: done : msix %d prod %d n2c %d free %d\n",
	    caller == IGC_TX_TRANSMIT ? "transmit" : "start",
	    txr->me, prod, txr->next_to_clean, free);

	IF_STAT_PUTREF(ifp);
}

static bool
igc_txeof(struct tx_ring *txr, u_int limit)
{
	struct igc_softc *sc = txr->sc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int cons, prod;
	bool more = false;

	prod = txr->next_avail_desc;
	cons = txr->next_to_clean;

	if (cons == prod) {
		DPRINTF(TX, "false: msix %d cons %d prod %d\n",
		    txr->me, cons, prod);
		return false;
	}

	do {
		struct igc_tx_buf *txbuf = &txr->tx_buffers[cons];
		const int last = txbuf->eop_index;

		membar_consumer();	/* XXXRO necessary? */

		KASSERT(last != -1);
		union igc_adv_tx_desc *txdesc = &txr->tx_base[last];
		igc_txdesc_sync(txr, last, BUS_DMASYNC_POSTREAD);
		const uint32_t status = le32toh(txdesc->wb.status);
		igc_txdesc_sync(txr, last, BUS_DMASYNC_PREREAD);

		if (!(status & IGC_TXD_STAT_DD))
			break;

		if (limit-- == 0) {
			more = true;
			DPRINTF(TX, "pending TX "
			    "msix %d cons %d last %d prod %d "
			    "status 0x%08x\n",
			    txr->me, cons, last, prod, status);
			break;
		}

		DPRINTF(TX, "handled TX "
		    "msix %d cons %d last %d prod %d "
		    "status 0x%08x\n",
		    txr->me, cons, last, prod, status);

		if_statinc(ifp, if_opackets);

		bus_dmamap_t map = txbuf->map;
		bus_dmamap_sync(txr->txdma.dma_tag, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(txr->txdma.dma_tag, map);
		m_freem(txbuf->m_head);

		txbuf->m_head = NULL;
		txbuf->eop_index = -1;

		cons = igc_txdesc_incr(sc, last);
	} while (cons != prod);

	txr->next_to_clean = cons;

	return more;
}

static void
igc_intr_barrier(struct igc_softc *sc __unused)
{

	xc_barrier(0);
}

static void
igc_stop(struct ifnet *ifp, int disable)
{
	struct igc_softc *sc = ifp->if_softc;

	mutex_enter(&sc->sc_core_lock);
	igc_stop_locked(sc);
	mutex_exit(&sc->sc_core_lock);
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC.
 *
 **********************************************************************/
static void
igc_stop_locked(struct igc_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	DPRINTF(CFG, "called\n");

	KASSERT(mutex_owned(&sc->sc_core_lock));

	/*
	 * If stopping processing has already started, do nothing.
	 */
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	/* Tell the stack that the interface is no longer active. */
	ifp->if_flags &= ~IFF_RUNNING;

	/*
	 * igc_handle_queue() can enable interrupts, so wait for completion of
	 * last igc_handle_queue() after unset IFF_RUNNING.
	 */
	mutex_exit(&sc->sc_core_lock);
	igc_barrier_handle_queue(sc);
	mutex_enter(&sc->sc_core_lock);

	sc->sc_core_stopping = true;

	igc_disable_intr(sc);

	callout_halt(&sc->sc_tick_ch, &sc->sc_core_lock);

	igc_reset_hw(&sc->hw);
	IGC_WRITE_REG(&sc->hw, IGC_WUC, 0);

	/*
	 * Wait for completion of interrupt handlers.
	 */
	mutex_exit(&sc->sc_core_lock);
	igc_intr_barrier(sc);
	mutex_enter(&sc->sc_core_lock);

	igc_update_link_status(sc);

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct tx_ring *txr = &sc->tx_rings[iq];

		igc_withdraw_transmit_packets(txr, false);
	}

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		igc_clear_receive_status(rxr);
	}

	/* Save last flags for the callback */
	sc->sc_if_flags = ifp->if_flags;
}

/*********************************************************************
 *  Ioctl entry point
 *
 *  igc_ioctl is called when the user wants to configure the
 *  interface.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/
static int
igc_ioctl(struct ifnet * ifp, u_long cmd, void *data)
{
	struct igc_softc *sc __unused = ifp->if_softc;
	int s;
	int error;

	DPRINTF(CFG, "cmd 0x%016lx\n", cmd);

	switch (cmd) {
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;
	default:
		KASSERT(IFNET_LOCKED(ifp));
	}

	if (cmd == SIOCZIFDATA) {
		mutex_enter(&sc->sc_core_lock);
		igc_clear_counters(sc);
		mutex_exit(&sc->sc_core_lock);
	}

	switch (cmd) {
#ifdef IF_RXR
	case SIOCGIFRXR:
		s = splnet();
		error = igc_rxrinfo(sc, (struct if_rxrinfo *)ifr->ifr_data);
		splx(s);
		break;
#endif
	default:
		s = splnet();
		error = ether_ioctl(ifp, cmd, data);
		splx(s);
		break;
	}

	if (error != ENETRESET)
		return error;

	error = 0;

	if (cmd == SIOCSIFCAP)
		error = if_init(ifp);
	else if ((cmd == SIOCADDMULTI) || (cmd == SIOCDELMULTI)) {
		mutex_enter(&sc->sc_core_lock);
		if (sc->sc_if_flags & IFF_RUNNING) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			igc_disable_intr(sc);
			igc_set_filter(sc);
			igc_enable_intr(sc);
		}
		mutex_exit(&sc->sc_core_lock);
	}

	return error;
}

#ifdef IF_RXR
static int
igc_rxrinfo(struct igc_softc *sc, struct if_rxrinfo *ifri)
{
	struct if_rxring_info *ifr, ifr1;
	int error;

	if (sc->sc_nqueues > 1) {
		ifr = kmem_zalloc(sc->sc_nqueues * sizeof(*ifr), KM_SLEEP);
	} else {
		ifr = &ifr1;
		memset(ifr, 0, sizeof(*ifr));
	}

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		ifr[iq].ifr_size = MCLBYTES;
		snprintf(ifr[iq].ifr_name, sizeof(ifr[iq].ifr_name), "%d", iq);
		ifr[iq].ifr_info = rxr->rx_ring;
	}

	error = if_rxr_info_ioctl(ifri, sc->sc_nqueues, ifr);
	if (sc->sc_nqueues > 1)
		kmem_free(ifr, sc->sc_nqueues * sizeof(*ifr));

	return error;
}
#endif

static void
igc_rxfill(struct rx_ring *rxr)
{
	struct igc_softc *sc = rxr->sc;
	int id;

	for (id = 0; id < sc->num_rx_desc; id++) {
		if (igc_get_buf(rxr, id, false)) {
			panic("%s: msix=%d i=%d\n", __func__, rxr->me, id);
		}
	}

	id = sc->num_rx_desc - 1;
	rxr->last_desc_filled = id;
	IGC_WRITE_REG(&sc->hw, IGC_RDT(rxr->me), id);
	rxr->next_to_check = 0;
}

static void
igc_rxrefill(struct rx_ring *rxr, int end)
{
	struct igc_softc *sc = rxr->sc;
	int id;

	for (id = rxr->next_to_check; id != end; id = igc_rxdesc_incr(sc, id)) {
		if (igc_get_buf(rxr, id, true)) {
			/* XXXRO */
			panic("%s: msix=%d id=%d\n", __func__, rxr->me, id);
		}
	}

	id = igc_rxdesc_decr(sc, id);
	DPRINTF(RX, "%s RDT %d id %d\n",
	    rxr->last_desc_filled == id ? "same" : "diff",
	    rxr->last_desc_filled, id);
	rxr->last_desc_filled = id;
	IGC_WRITE_REG(&sc->hw, IGC_RDT(rxr->me), id);
}

/*********************************************************************
 *
 *  This routine executes in interrupt context. It replenishes
 *  the mbufs in the descriptor and sends data which has been
 *  dma'ed into host memory to upper layer.
 *
 *********************************************************************/
static bool
igc_rxeof(struct rx_ring *rxr, u_int limit)
{
	struct igc_softc *sc = rxr->sc;
	struct igc_queue *q = rxr->rxr_igcq;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int id;
	bool more = false;

	id = rxr->next_to_check;
	for (;;) {
		union igc_adv_rx_desc *rxdesc = &rxr->rx_base[id];
		struct igc_rx_buf *rxbuf, *nxbuf;
		struct mbuf *mp, *m;

		igc_rxdesc_sync(rxr, id,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		const uint32_t staterr = le32toh(rxdesc->wb.upper.status_error);

		if (!ISSET(staterr, IGC_RXD_STAT_DD)) {
			igc_rxdesc_sync(rxr, id,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			break;
		}

		if (limit-- == 0) {
			igc_rxdesc_sync(rxr, id,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			DPRINTF(RX, "more=true\n");
			more = true;
			break;
		}

		/* Zero out the receive descriptors status. */
		rxdesc->wb.upper.status_error = 0;

		/* Pull the mbuf off the ring. */
		rxbuf = &rxr->rx_buffers[id];
		bus_dmamap_t map = rxbuf->map;
		bus_dmamap_sync(rxr->rxdma.dma_tag, map,
		    0, map->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(rxr->rxdma.dma_tag, map);

		mp = rxbuf->buf;
		rxbuf->buf = NULL;

		const bool eop = staterr & IGC_RXD_STAT_EOP;
		const uint16_t len = le16toh(rxdesc->wb.upper.length);

		const uint16_t vtag = le16toh(rxdesc->wb.upper.vlan);

		const uint32_t ptype = le32toh(rxdesc->wb.lower.lo_dword.data) &
		    IGC_PKTTYPE_MASK;

		const uint32_t hash __unused =
		    le32toh(rxdesc->wb.lower.hi_dword.rss);
		const uint16_t hashtype __unused =
		    le16toh(rxdesc->wb.lower.lo_dword.hs_rss.pkt_info) &
		    IGC_RXDADV_RSSTYPE_MASK;

		igc_rxdesc_sync(rxr, id,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		if (__predict_false(staterr & IGC_RXDEXT_STATERR_RXE)) {
			if (rxbuf->fmp) {
				m_freem(rxbuf->fmp);
				rxbuf->fmp = NULL;
			}

			m_freem(mp);
			m = NULL;

			if_statinc(ifp, if_ierrors);
			IGC_QUEUE_EVENT(q, rx_discard, 1);

			DPRINTF(RX, "ierrors++\n");

			goto next_desc;
		}

		if (__predict_false(mp == NULL)) {
			panic("%s: igc_rxeof: NULL mbuf in slot %d "
			    "(filled %d)", device_xname(sc->sc_dev),
			    id, rxr->last_desc_filled);
		}

		if (!eop) {
			/*
			 * Figure out the next descriptor of this frame.
			 */
			int nextp = igc_rxdesc_incr(sc, id);

			nxbuf = &rxr->rx_buffers[nextp];
			/*
			 * TODO prefetch(nxbuf);
			 */
		}

		mp->m_len = len;

		m = rxbuf->fmp;
		rxbuf->fmp = NULL;

		if (m != NULL) {
			m->m_pkthdr.len += mp->m_len;
		} else {
			m = mp;
			m->m_pkthdr.len = mp->m_len;
#if NVLAN > 0
			if (staterr & IGC_RXD_STAT_VP)
				vlan_set_tag(m, vtag);
#endif
		}

		/* Pass the head pointer on */
		if (!eop) {
			nxbuf->fmp = m;
			m = NULL;
			mp->m_next = nxbuf->buf;
		} else {
			m_set_rcvif(m, ifp);

			m->m_pkthdr.csum_flags = igc_rx_checksum(q,
			    ifp->if_capenable, staterr, ptype);

#ifdef notyet
			if (hashtype != IGC_RXDADV_RSSTYPE_NONE) {
				m->m_pkthdr.ph_flowid = hash;
				SET(m->m_pkthdr.csum_flags, M_FLOWID);
			}
			ml_enqueue(&ml, m);
#endif

			if_percpuq_enqueue(sc->sc_ipq, m);

			if_statinc(ifp, if_ipackets);
			IGC_QUEUE_EVENT(q, rx_packets, 1);
			IGC_QUEUE_EVENT(q, rx_bytes, m->m_pkthdr.len);
		}
 next_desc:
		/* Advance our pointers to the next descriptor. */
		id = igc_rxdesc_incr(sc, id);
	}

	DPRINTF(RX, "fill queue[%d]\n", rxr->me);
	igc_rxrefill(rxr, id);

	DPRINTF(RX, "%s n2c %d id %d\n",
	    rxr->next_to_check == id ? "same" : "diff",
	    rxr->next_to_check, id);
	rxr->next_to_check = id;

#ifdef OPENBSD
	if (!(staterr & IGC_RXD_STAT_DD))
		return 0;
#endif

	return more;
}

/*********************************************************************
 *
 *  Verify that the hardware indicated that the checksum is valid.
 *  Inform the stack about the status of checksum so that stack
 *  doesn't spend time verifying the checksum.
 *
 *********************************************************************/
static int
igc_rx_checksum(struct igc_queue *q, uint64_t capenable, uint32_t staterr,
    uint32_t ptype)
{
	const uint16_t status = (uint16_t)staterr;
	const uint8_t errors = (uint8_t)(staterr >> 24);
	int flags = 0;

	if ((status & IGC_RXD_STAT_IPCS) != 0 &&
	    (capenable & IFCAP_CSUM_IPv4_Rx) != 0) {
		IGC_DRIVER_EVENT(q, rx_ipcs, 1);
		flags |= M_CSUM_IPv4;
		if (__predict_false((errors & IGC_RXD_ERR_IPE) != 0)) {
			IGC_DRIVER_EVENT(q, rx_ipcs_bad, 1);
			flags |= M_CSUM_IPv4_BAD;
		}
	}

	if ((status & IGC_RXD_STAT_TCPCS) != 0) {
		IGC_DRIVER_EVENT(q, rx_tcpcs, 1);
		if ((capenable & IFCAP_CSUM_TCPv4_Rx) != 0)
			flags |= M_CSUM_TCPv4;
		if ((capenable & IFCAP_CSUM_TCPv6_Rx) != 0)
			flags |= M_CSUM_TCPv6;
	}

	if ((status & IGC_RXD_STAT_UDPCS) != 0) {
		IGC_DRIVER_EVENT(q, rx_udpcs, 1);
		if ((capenable & IFCAP_CSUM_UDPv4_Rx) != 0)
			flags |= M_CSUM_UDPv4;
		if ((capenable & IFCAP_CSUM_UDPv6_Rx) != 0)
			flags |= M_CSUM_UDPv6;
	}

	if (__predict_false((errors & IGC_RXD_ERR_TCPE) != 0)) {
		IGC_DRIVER_EVENT(q, rx_l4cs_bad, 1);
		if ((flags & ~M_CSUM_IPv4) != 0)
			flags |= M_CSUM_TCP_UDP_BAD;
	}

	return flags;
}

static void
igc_watchdog(struct ifnet * ifp)
{
}

static void
igc_tick(void *arg)
{
	struct igc_softc *sc = arg;

	mutex_enter(&sc->sc_core_lock);

	if (__predict_false(sc->sc_core_stopping)) {
		mutex_exit(&sc->sc_core_lock);
		return;
	}

	/* XXX watchdog */
	if (0) {
		IGC_GLOBAL_EVENT(sc, watchdog, 1);
	}

	igc_update_counters(sc);

	mutex_exit(&sc->sc_core_lock);

	callout_schedule(&sc->sc_tick_ch, hz);
}

/*********************************************************************
 *
 *  Media Ioctl callback
 *
 *  This routine is called whenever the user queries the status of
 *  the interface using ifconfig.
 *
 **********************************************************************/
static void
igc_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct igc_softc *sc = ifp->if_softc;
	struct igc_hw *hw = &sc->hw;

	igc_update_link_status(sc);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (!sc->link_active) {
		ifmr->ifm_active |= IFM_NONE;
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;

	switch (sc->link_speed) {
	case 10:
		ifmr->ifm_active |= IFM_10_T;
		break;
	case 100:
		ifmr->ifm_active |= IFM_100_TX;
		break;
	case 1000:
		ifmr->ifm_active |= IFM_1000_T;
		break;
	case 2500:
		ifmr->ifm_active |= IFM_2500_T;
		break;
	}

	if (sc->link_duplex == FULL_DUPLEX)
		ifmr->ifm_active |= IFM_FDX;
	else
		ifmr->ifm_active |= IFM_HDX;

	switch (hw->fc.current_mode) {
	case igc_fc_tx_pause:
		ifmr->ifm_active |= IFM_FLOW | IFM_ETH_TXPAUSE;
		break;
	case igc_fc_rx_pause:
		ifmr->ifm_active |= IFM_FLOW | IFM_ETH_RXPAUSE;
		break;
	case igc_fc_full:
		ifmr->ifm_active |= IFM_FLOW |
		    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
		break;
	case igc_fc_none:
	default:
		break;
	}
}

/*********************************************************************
 *
 *  Media Ioctl callback
 *
 *  This routine is called when the user changes speed/duplex using
 *  media/mediopt option with ifconfig.
 *
 **********************************************************************/
static int
igc_media_change(struct ifnet *ifp)
{
	struct igc_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return EINVAL;

	sc->hw.mac.autoneg = DO_AUTO_NEG;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		sc->hw.phy.autoneg_advertised = AUTONEG_ADV_DEFAULT;
		break;
	case IFM_2500_T:
		sc->hw.phy.autoneg_advertised = ADVERTISE_2500_FULL;
		break;
	case IFM_1000_T:
		sc->hw.phy.autoneg_advertised = ADVERTISE_1000_FULL;
		break;
	case IFM_100_TX:
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			sc->hw.phy.autoneg_advertised = ADVERTISE_100_FULL;
		else
			sc->hw.phy.autoneg_advertised = ADVERTISE_100_HALF;
		break;
	case IFM_10_T:
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			sc->hw.phy.autoneg_advertised = ADVERTISE_10_FULL;
		else
			sc->hw.phy.autoneg_advertised = ADVERTISE_10_HALF;
		break;
	default:
		return EINVAL;
	}

	igc_init_locked(sc);

	return 0;
}

static int
igc_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct igc_softc *sc = ifp->if_softc;
	int rc = 0;
	u_short iffchange;
	bool needreset = false;

	DPRINTF(CFG, "called\n");

	KASSERT(IFNET_LOCKED(ifp));

	mutex_enter(&sc->sc_core_lock);

	/*
	 * Check for if_flags.
	 * Main usage is to prevent linkdown when opening bpf.
	 */
	iffchange = ifp->if_flags ^ sc->sc_if_flags;
	sc->sc_if_flags = ifp->if_flags;
	if ((iffchange & ~(IFF_CANTCHANGE | IFF_DEBUG)) != 0) {
		needreset = true;
		goto ec;
	}

	/* iff related updates */
	if ((iffchange & IFF_PROMISC) != 0)
		igc_set_filter(sc);

#ifdef notyet
	igc_set_vlan(sc);
#endif

ec:
#ifdef notyet
	/* Check for ec_capenable. */
	ecchange = ec->ec_capenable ^ sc->sc_ec_capenable;
	sc->sc_ec_capenable = ec->ec_capenable;
	if ((ecchange & ~ETHERCAP_SOMETHING) != 0) {
		needreset = true;
		goto out;
	}
#endif
	if (needreset)
		rc = ENETRESET;

	mutex_exit(&sc->sc_core_lock);

	return rc;
}

static void
igc_set_filter(struct igc_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	uint32_t rctl;

	rctl = IGC_READ_REG(&sc->hw, IGC_RCTL);
	rctl &= ~(IGC_RCTL_BAM |IGC_RCTL_UPE | IGC_RCTL_MPE);

	if ((sc->sc_if_flags & IFF_BROADCAST) != 0)
		rctl |= IGC_RCTL_BAM;
	if ((sc->sc_if_flags & IFF_PROMISC) != 0) {
		DPRINTF(CFG, "promisc\n");
		rctl |= IGC_RCTL_UPE;
		ETHER_LOCK(ec);
 allmulti:
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		rctl |= IGC_RCTL_MPE;
	} else {
		struct ether_multistep step;
		struct ether_multi *enm;
		int mcnt = 0;

		memset(sc->mta, 0, IGC_MTA_LEN);

		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (((memcmp(enm->enm_addrlo, enm->enm_addrhi,
					ETHER_ADDR_LEN)) != 0) ||
			    (mcnt >= MAX_NUM_MULTICAST_ADDRESSES)) {
				/*
				 * We must listen to a range of multicast
				 * addresses. For now, just accept all
				 * multicasts, rather than trying to set only
				 * those filter bits needed to match the range.
				 * (At this time, the only use of address
				 * ranges is for IP multicast routing, for
				 * which the range is big enough to require all
				 * bits set.)
				 */
				goto allmulti;
			}
			DPRINTF(CFG, "%d: %s\n", mcnt,
			    ether_sprintf(enm->enm_addrlo));
			memcpy(&sc->mta[mcnt * ETHER_ADDR_LEN],
			    enm->enm_addrlo, ETHER_ADDR_LEN);

			mcnt++;
			ETHER_NEXT_MULTI(step, enm);
		}
		ec->ec_flags &= ~ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);

		DPRINTF(CFG, "hw filter\n");
		igc_update_mc_addr_list(&sc->hw, sc->mta, mcnt);
	}

	IGC_WRITE_REG(&sc->hw, IGC_RCTL, rctl);
}

static void
igc_update_link_status(struct igc_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct igc_hw *hw = &sc->hw;

	if (IGC_READ_REG(&sc->hw, IGC_STATUS) & IGC_STATUS_LU) {
		if (sc->link_active == 0) {
			igc_get_speed_and_duplex(hw, &sc->link_speed,
			    &sc->link_duplex);
			sc->link_active = 1;
			ifp->if_baudrate = IF_Mbps(sc->link_speed);
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else {
		if (sc->link_active == 1) {
			ifp->if_baudrate = sc->link_speed = 0;
			sc->link_duplex = 0;
			sc->link_active = 0;
			if_link_state_change(ifp, LINK_STATE_DOWN);
		}
	}
}

/*********************************************************************
 *
 *  Get a buffer from system mbuf buffer pool.
 *
 **********************************************************************/
static int
igc_get_buf(struct rx_ring *rxr, int id, bool strict)
{
	struct igc_softc *sc = rxr->sc;
	struct igc_queue *q = rxr->rxr_igcq;
	struct igc_rx_buf *rxbuf = &rxr->rx_buffers[id];
	bus_dmamap_t map = rxbuf->map;
	struct mbuf *m;
	int error;

	if (__predict_false(rxbuf->buf)) {
		if (strict) {
			DPRINTF(RX, "slot %d already has an mbuf\n", id);
			return EINVAL;
		}
		return 0;
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (__predict_false(m == NULL)) {
 enobuf:
		IGC_QUEUE_EVENT(q, rx_no_mbuf, 1);
		return ENOBUFS;
	}

	MCLGET(m, M_DONTWAIT);
	if (__predict_false(!(m->m_flags & M_EXT))) {
		m_freem(m);
		goto enobuf;
	}

	m->m_len = m->m_pkthdr.len = sc->rx_mbuf_sz;

	error = bus_dmamap_load_mbuf(rxr->rxdma.dma_tag, map, m,
	    BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		m_freem(m);
		return error;
	}

	bus_dmamap_sync(rxr->rxdma.dma_tag, map, 0,
	    map->dm_mapsize, BUS_DMASYNC_PREREAD);
	rxbuf->buf = m;

	union igc_adv_rx_desc *rxdesc = &rxr->rx_base[id];
	igc_rxdesc_sync(rxr, id, BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
	rxdesc->read.pkt_addr = htole64(map->dm_segs[0].ds_addr);
	igc_rxdesc_sync(rxr, id, BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	return 0;
}

static void
igc_configure_queues(struct igc_softc *sc)
{
	struct igc_hw *hw = &sc->hw;
	uint32_t ivar;

	/* First turn on RSS capability */
	IGC_WRITE_REG(hw, IGC_GPIE, IGC_GPIE_MSIX_MODE | IGC_GPIE_EIAME |
	    IGC_GPIE_PBA | IGC_GPIE_NSICR);

	/* Set the starting interrupt rate */
	uint32_t newitr = (4000000 / MAX_INTS_PER_SEC) & 0x7FFC;
	newitr |= IGC_EITR_CNT_IGNR;

	/* Turn on MSI-X */
	uint32_t newmask = 0;
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct igc_queue *q = &sc->queues[iq];

		/* RX entries */
		igc_set_queues(sc, iq, q->msix, 0);
		/* TX entries */
		igc_set_queues(sc, iq, q->msix, 1);
		newmask |= q->eims;
		IGC_WRITE_REG(hw, IGC_EITR(q->msix), newitr);
	}
	sc->msix_queuesmask = newmask;

#if 1
	ivar = IGC_READ_REG_ARRAY(hw, IGC_IVAR0, 0);
	DPRINTF(CFG, "ivar(0)=0x%x\n", ivar);
	ivar = IGC_READ_REG_ARRAY(hw, IGC_IVAR0, 1);
	DPRINTF(CFG, "ivar(1)=0x%x\n", ivar);
#endif

	/* And for the link interrupt */
	ivar = (sc->linkvec | IGC_IVAR_VALID) << 8;
	sc->msix_linkmask = 1 << sc->linkvec;
	IGC_WRITE_REG(hw, IGC_IVAR_MISC, ivar);
}

static void
igc_set_queues(struct igc_softc *sc, uint32_t entry, uint32_t vector, int type)
{
	struct igc_hw *hw = &sc->hw;
	const uint32_t index = entry >> 1;
	uint32_t ivar = IGC_READ_REG_ARRAY(hw, IGC_IVAR0, index);

	if (type) {
		if (entry & 1) {
			ivar &= 0x00FFFFFF;
			ivar |= (vector | IGC_IVAR_VALID) << 24;
		} else {
			ivar &= 0xFFFF00FF;
			ivar |= (vector | IGC_IVAR_VALID) << 8;
		}
	} else {
		if (entry & 1) {
			ivar &= 0xFF00FFFF;
			ivar |= (vector | IGC_IVAR_VALID) << 16;
		} else {
			ivar &= 0xFFFFFF00;
			ivar |= vector | IGC_IVAR_VALID;
		}
	}
	IGC_WRITE_REG_ARRAY(hw, IGC_IVAR0, index, ivar);
}

static void
igc_enable_queue(struct igc_softc *sc, uint32_t eims)
{
	IGC_WRITE_REG(&sc->hw, IGC_EIMS, eims);
}

static void
igc_enable_intr(struct igc_softc *sc)
{
	struct igc_hw *hw = &sc->hw;

	if (sc->sc_intr_type == PCI_INTR_TYPE_MSIX) {
		const uint32_t mask = sc->msix_queuesmask | sc->msix_linkmask;

		IGC_WRITE_REG(hw, IGC_EIAC, mask);
		IGC_WRITE_REG(hw, IGC_EIAM, mask);
		IGC_WRITE_REG(hw, IGC_EIMS, mask);
		IGC_WRITE_REG(hw, IGC_IMS, IGC_IMS_LSC);
	} else {
		IGC_WRITE_REG(hw, IGC_IMS, IMS_ENABLE_MASK);
	}
	IGC_WRITE_FLUSH(hw);
}

static void
igc_disable_intr(struct igc_softc *sc)
{
	struct igc_hw *hw = &sc->hw;

	if (sc->sc_intr_type == PCI_INTR_TYPE_MSIX) {
		IGC_WRITE_REG(hw, IGC_EIMC, 0xffffffff);
		IGC_WRITE_REG(hw, IGC_EIAC, 0);
	}
	IGC_WRITE_REG(hw, IGC_IMC, 0xffffffff);
	IGC_WRITE_FLUSH(hw);
}

static int
igc_intr_link(void *arg)
{
	struct igc_softc *sc = (struct igc_softc *)arg;
	const uint32_t reg_icr = IGC_READ_REG(&sc->hw, IGC_ICR);

	IGC_GLOBAL_EVENT(sc, link, 1);

	if (reg_icr & IGC_ICR_LSC) {
		mutex_enter(&sc->sc_core_lock);
		sc->hw.mac.get_link_status = true;
		igc_update_link_status(sc);
		mutex_exit(&sc->sc_core_lock);
	}

	IGC_WRITE_REG(&sc->hw, IGC_IMS, IGC_IMS_LSC);
	IGC_WRITE_REG(&sc->hw, IGC_EIMS, sc->msix_linkmask);

	return 1;
}

static int
igc_intr_queue(void *arg)
{
	struct igc_queue *iq = arg;
	struct igc_softc *sc = iq->sc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct rx_ring *rxr = iq->rxr;
	struct tx_ring *txr = iq->txr;
	const u_int txlimit = sc->sc_tx_intr_process_limit,
		    rxlimit = sc->sc_rx_intr_process_limit;
	bool txmore, rxmore;

	IGC_QUEUE_EVENT(iq, irqs, 1);

	if (__predict_false(!ISSET(ifp->if_flags, IFF_RUNNING)))
		return 0;

	mutex_enter(&txr->txr_lock);
	txmore = igc_txeof(txr, txlimit);
	mutex_exit(&txr->txr_lock);
	mutex_enter(&rxr->rxr_lock);
	rxmore = igc_rxeof(rxr, rxlimit);
	mutex_exit(&rxr->rxr_lock);

	if (txmore || rxmore) {
		IGC_QUEUE_EVENT(iq, req, 1);
		igc_sched_handle_queue(sc, iq);
	} else {
		igc_enable_queue(sc, iq->eims);
	}

	return 1;
}

static int
igc_intr(void *arg)
{
	struct igc_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct igc_queue *iq = &sc->queues[0];
	struct rx_ring *rxr = iq->rxr;
	struct tx_ring *txr = iq->txr;
	const u_int txlimit = sc->sc_tx_intr_process_limit,
		    rxlimit = sc->sc_rx_intr_process_limit;
	bool txmore, rxmore;

	if (__predict_false(!ISSET(ifp->if_flags, IFF_RUNNING)))
		return 0;

	const uint32_t reg_icr = IGC_READ_REG(&sc->hw, IGC_ICR);
	DPRINTF(MISC, "reg_icr=0x%x\n", reg_icr);

	/* Definitely not our interrupt. */
	if (reg_icr == 0x0) {
		DPRINTF(MISC, "not for me");
		return 0;
	}

	IGC_QUEUE_EVENT(iq, irqs, 1);

	/* Hot eject? */
	if (__predict_false(reg_icr == 0xffffffff)) {
		DPRINTF(MISC, "hot eject\n");
		return 0;
	}

	if (__predict_false(!(reg_icr & IGC_ICR_INT_ASSERTED))) {
		DPRINTF(MISC, "not set IGC_ICR_INT_ASSERTED");
		return 0;
	}

	/*
	 * Only MSI-X interrupts have one-shot behavior by taking advantage
	 * of the EIAC register.  Thus, explicitly disable interrupts.  This
	 * also works around the MSI message reordering errata on certain
	 * systems.
	 */
	igc_disable_intr(sc);

	mutex_enter(&txr->txr_lock);
	txmore = igc_txeof(txr, txlimit);
	mutex_exit(&txr->txr_lock);
	mutex_enter(&rxr->rxr_lock);
	rxmore = igc_rxeof(rxr, rxlimit);
	mutex_exit(&rxr->rxr_lock);

	/* Link status change */
	// XXXX FreeBSD checks IGC_ICR_RXSEQ
	if (__predict_false(reg_icr & IGC_ICR_LSC)) {
		IGC_GLOBAL_EVENT(sc, link, 1);
		mutex_enter(&sc->sc_core_lock);
		sc->hw.mac.get_link_status = true;
		igc_update_link_status(sc);
		mutex_exit(&sc->sc_core_lock);
	}

	if (txmore || rxmore) {
		IGC_QUEUE_EVENT(iq, req, 1);
		igc_sched_handle_queue(sc, iq);
	} else {
		igc_enable_intr(sc);
	}

	return 1;
}

static void
igc_handle_queue(void *arg)
{
	struct igc_queue *iq = arg;
	struct igc_softc *sc = iq->sc;
	struct tx_ring *txr = iq->txr;
	struct rx_ring *rxr = iq->rxr;
	const u_int txlimit = sc->sc_tx_process_limit,
		    rxlimit = sc->sc_rx_process_limit;
	bool txmore, rxmore;

	IGC_QUEUE_EVENT(iq, handleq, 1);

	mutex_enter(&txr->txr_lock);
	txmore = igc_txeof(txr, txlimit);
	/* for ALTQ, dequeue from if_snd */
	if (txr->me == 0) {
		struct ifnet *ifp = &sc->sc_ec.ec_if;

		igc_tx_common_locked(ifp, txr, IGC_TX_START);
	}
	mutex_exit(&txr->txr_lock);

	mutex_enter(&rxr->rxr_lock);
	rxmore = igc_rxeof(rxr, rxlimit);
	mutex_exit(&rxr->rxr_lock);

	if (txmore || rxmore) {
		igc_sched_handle_queue(sc, iq);
	} else {
		if (sc->sc_intr_type == PCI_INTR_TYPE_MSIX)
			igc_enable_queue(sc, iq->eims);
		else
			igc_enable_intr(sc);
	}
}

static void
igc_handle_queue_work(struct work *wk, void *context)
{
	struct igc_queue *iq =
	    container_of(wk, struct igc_queue, igcq_wq_cookie);

	igc_handle_queue(iq);
}

static void
igc_sched_handle_queue(struct igc_softc *sc, struct igc_queue *iq)
{

	if (iq->igcq_workqueue) {
		/* XXXRO notyet */
		workqueue_enqueue(sc->sc_queue_wq, &iq->igcq_wq_cookie,
		    curcpu());
	} else {
		softint_schedule(iq->igcq_si);
	}
}

static void
igc_barrier_handle_queue(struct igc_softc *sc)
{

	if (sc->sc_txrx_workqueue) {
		for (int iq = 0; iq < sc->sc_nqueues; iq++) {
			struct igc_queue *q = &sc->queues[iq];

			workqueue_wait(sc->sc_queue_wq, &q->igcq_wq_cookie);
		}
	} else {
		xc_barrier(0);
	}
}

/*********************************************************************
 *
 *  Allocate memory for tx_buffer structures. The tx_buffer stores all
 *  the information needed to transmit a packet on the wire.
 *
 **********************************************************************/
static int
igc_allocate_transmit_buffers(struct tx_ring *txr)
{
	struct igc_softc *sc = txr->sc;
	int error;

	txr->tx_buffers =
	    kmem_zalloc(sc->num_tx_desc * sizeof(struct igc_tx_buf), KM_SLEEP);
	txr->txtag = txr->txdma.dma_tag;

	/* Create the descriptor buffer dma maps. */
	for (int id = 0; id < sc->num_tx_desc; id++) {
		struct igc_tx_buf *txbuf = &txr->tx_buffers[id];

		error = bus_dmamap_create(txr->txdma.dma_tag,
		    round_page(IGC_TSO_SIZE + sizeof(struct ether_vlan_header)),
		    IGC_MAX_SCATTER, PAGE_SIZE, 0, BUS_DMA_NOWAIT, &txbuf->map);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create TX DMA map\n");
			goto fail;
		}

		txbuf->eop_index = -1;
	}

	return 0;
 fail:
	return error;
}


/*********************************************************************
 *
 *  Allocate and initialize transmit structures.
 *
 **********************************************************************/
static int
igc_setup_transmit_structures(struct igc_softc *sc)
{

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct tx_ring *txr = &sc->tx_rings[iq];

		if (igc_setup_transmit_ring(txr))
			goto fail;
	}

	return 0;
 fail:
	igc_free_transmit_structures(sc);
	return ENOBUFS;
}

/*********************************************************************
 *
 *  Initialize a transmit ring.
 *
 **********************************************************************/
static int
igc_setup_transmit_ring(struct tx_ring *txr)
{
	struct igc_softc *sc = txr->sc;

	/* Now allocate transmit buffers for the ring. */
	if (igc_allocate_transmit_buffers(txr))
		return ENOMEM;

	/* Clear the old ring contents */
	memset(txr->tx_base, 0,
	    sizeof(union igc_adv_tx_desc) * sc->num_tx_desc);

	/* Reset indices. */
	txr->next_avail_desc = 0;
	txr->next_to_clean = 0;

	bus_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map, 0,
	    txr->txdma.dma_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	txr->txr_interq = pcq_create(sc->num_tx_desc, KM_SLEEP);

	mutex_init(&txr->txr_lock, MUTEX_DEFAULT, IPL_NET);

	return 0;
}

/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
static void
igc_initialize_transmit_unit(struct igc_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct igc_hw *hw = &sc->hw;

	/* Setup the Base and Length of the TX descriptor ring. */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct tx_ring *txr = &sc->tx_rings[iq];
		const uint64_t bus_addr =
		    txr->txdma.dma_map->dm_segs[0].ds_addr;

		/* Base and len of TX ring */
		IGC_WRITE_REG(hw, IGC_TDLEN(iq),
		    sc->num_tx_desc * sizeof(union igc_adv_tx_desc));
		IGC_WRITE_REG(hw, IGC_TDBAH(iq), (uint32_t)(bus_addr >> 32));
		IGC_WRITE_REG(hw, IGC_TDBAL(iq), (uint32_t)bus_addr);

		/* Init the HEAD/TAIL indices */
		IGC_WRITE_REG(hw, IGC_TDT(iq), 0 /* XXX txr->next_avail_desc */);
		IGC_WRITE_REG(hw, IGC_TDH(iq), 0);

		txr->watchdog_timer = 0;

		uint32_t txdctl = 0;	/* Clear txdctl */
		txdctl |= 0x1f;		/* PTHRESH */
		txdctl |= 1 << 8;	/* HTHRESH */
		txdctl |= 1 << 16;	/* WTHRESH */
		txdctl |= 1 << 22;	/* Reserved bit 22 must always be 1 */
		txdctl |= IGC_TXDCTL_GRAN;
		txdctl |= 1 << 25;	/* LWTHRESH */

		IGC_WRITE_REG(hw, IGC_TXDCTL(iq), txdctl);
	}
	ifp->if_timer = 0;

	/* Program the Transmit Control Register */
	uint32_t tctl = IGC_READ_REG(&sc->hw, IGC_TCTL);
	tctl &= ~IGC_TCTL_CT;
	tctl |= (IGC_TCTL_PSP | IGC_TCTL_RTLC | IGC_TCTL_EN |
	    (IGC_COLLISION_THRESHOLD << IGC_CT_SHIFT));

	/* This write will effectively turn on the transmit unit. */
	IGC_WRITE_REG(&sc->hw, IGC_TCTL, tctl);
}

/*********************************************************************
 *
 *  Free all transmit rings.
 *
 **********************************************************************/
static void
igc_free_transmit_structures(struct igc_softc *sc)
{

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct tx_ring *txr = &sc->tx_rings[iq];

		igc_free_transmit_buffers(txr);
	}
}

/*********************************************************************
 *
 *  Free transmit ring related data structures.
 *
 **********************************************************************/
static void
igc_free_transmit_buffers(struct tx_ring *txr)
{
	struct igc_softc *sc = txr->sc;

	if (txr->tx_buffers == NULL)
		return;

	igc_withdraw_transmit_packets(txr, true);

	kmem_free(txr->tx_buffers,
	    sc->num_tx_desc * sizeof(struct igc_tx_buf));
	txr->tx_buffers = NULL;
	txr->txtag = NULL;

	pcq_destroy(txr->txr_interq);
	mutex_destroy(&txr->txr_lock);
}

/*********************************************************************
 *
 *  Withdraw transmit packets.
 *
 **********************************************************************/
static void
igc_withdraw_transmit_packets(struct tx_ring *txr, bool destroy)
{
	struct igc_softc *sc = txr->sc;
	struct igc_queue *q = txr->txr_igcq;

	mutex_enter(&txr->txr_lock);

	for (int id = 0; id < sc->num_tx_desc; id++) {
		union igc_adv_tx_desc *txdesc = &txr->tx_base[id];

		igc_txdesc_sync(txr, id,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		txdesc->read.buffer_addr = 0;
		txdesc->read.cmd_type_len = 0;
		txdesc->read.olinfo_status = 0;
		igc_txdesc_sync(txr, id,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		struct igc_tx_buf *txbuf = &txr->tx_buffers[id];
		bus_dmamap_t map = txbuf->map;

		if (map != NULL && map->dm_nsegs > 0) {
			bus_dmamap_sync(txr->txdma.dma_tag, map,
			    0, map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(txr->txdma.dma_tag, map);
		}
		if (txbuf->m_head != NULL) {
			m_freem(txbuf->m_head);
			txbuf->m_head = NULL;
		}
		if (map != NULL && destroy) {
			bus_dmamap_destroy(txr->txdma.dma_tag, map);
			txbuf->map = NULL;
		}
		txbuf->eop_index = -1;

		txr->next_avail_desc = 0;
		txr->next_to_clean = 0;
	}

	struct mbuf *m;
	while ((m = pcq_get(txr->txr_interq)) != NULL) {
		IGC_QUEUE_EVENT(q, tx_pcq_drop, 1);
		m_freem(m);
	}

	mutex_exit(&txr->txr_lock);
}


/*********************************************************************
 *
 *  Advanced Context Descriptor setup for VLAN, CSUM or TSO
 *
 **********************************************************************/

static int
igc_tx_ctx_setup(struct tx_ring *txr, struct mbuf *mp, int prod,
    uint32_t *cmd_type_len, uint32_t *olinfo_status)
{
	struct ether_vlan_header *evl;
	uint32_t type_tucmd_mlhl = 0;
	uint32_t vlan_macip_lens = 0;
	uint32_t ehlen, iphlen;
	uint16_t ehtype;
	int off = 0;

	const int csum_flags = mp->m_pkthdr.csum_flags;

	/* First check if TSO is to be used */
	if ((csum_flags & (M_CSUM_TSOv4 | M_CSUM_TSOv6)) != 0) {
		return igc_tso_setup(txr, mp, prod, cmd_type_len,
		    olinfo_status);
	}

	const bool v4 = (csum_flags &
	    (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) != 0;
	const bool v6 = (csum_flags & (M_CSUM_UDPv6 | M_CSUM_TCPv6)) != 0;

	/* Indicate the whole packet as payload when not doing TSO */
	*olinfo_status |= mp->m_pkthdr.len << IGC_ADVTXD_PAYLEN_SHIFT;

	/*
	 * In advanced descriptors the vlan tag must
	 * be placed into the context descriptor. Hence
	 * we need to make one even if not doing offloads.
	 */
#if NVLAN > 0
	if (vlan_has_tag(mp)) {
		vlan_macip_lens |= (uint32_t)vlan_get_tag(mp)
		    << IGC_ADVTXD_VLAN_SHIFT;
		off = 1;
	} else
#endif
	if (!v4 && !v6)
		return 0;

	KASSERT(mp->m_len >= sizeof(struct ether_header));
	evl = mtod(mp, struct ether_vlan_header *);
	if (evl->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		KASSERT(mp->m_len >= sizeof(struct ether_vlan_header));
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		ehtype = evl->evl_proto;
	} else {
		ehlen = ETHER_HDR_LEN;
		ehtype = evl->evl_encap_proto;
	}

	vlan_macip_lens |= ehlen << IGC_ADVTXD_MACLEN_SHIFT;

#ifdef IGC_DEBUG
	/*
	 * For checksum offloading, L3 headers are not mandatory.
	 * We use these only for consistency checks.
	 */
	struct ip *ip;
	struct ip6_hdr *ip6;
	uint8_t ipproto;
	char *l3d;

	if (mp->m_len == ehlen && mp->m_next != NULL)
		l3d = mtod(mp->m_next, char *);
	else
		l3d = mtod(mp, char *) + ehlen;
#endif

	switch (ntohs(ehtype)) {
	case ETHERTYPE_IP:
		iphlen = M_CSUM_DATA_IPv4_IPHL(mp->m_pkthdr.csum_data);
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_IPV4;

		if ((csum_flags & M_CSUM_IPv4) != 0) {
			*olinfo_status |= IGC_TXD_POPTS_IXSM << 8;
			off = 1;
		}
#ifdef IGC_DEBUG
		KASSERT(!v6);
		ip = (void *)l3d;
		ipproto = ip->ip_p;
		KASSERT(iphlen == ip->ip_hl << 2);
		KASSERT((mp->m_pkthdr.csum_flags & M_CSUM_IPv4) == 0 ||
		    ip->ip_sum == 0);
#endif
		break;
	case ETHERTYPE_IPV6:
		iphlen = M_CSUM_DATA_IPv6_IPHL(mp->m_pkthdr.csum_data);
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_IPV6;
#ifdef IGC_DEBUG
		KASSERT(!v4);
		ip6 = (void *)l3d;
		ipproto = ip6->ip6_nxt;	/* XXX */
		KASSERT(iphlen == sizeof(struct ip6_hdr));
#endif
		break;
	default:
		/*
		 * Unknown L3 protocol. Clear L3 header length and proceed for
		 * LAN as done by Linux driver.
		 */
		iphlen = 0;
#ifdef IGC_DEBUG
		KASSERT(!v4 && !v6);
		ipproto = 0;
#endif
		break;
	}

	vlan_macip_lens |= iphlen;

	const bool tcp = (csum_flags & (M_CSUM_TCPv4 | M_CSUM_TCPv6)) != 0;
	const bool udp = (csum_flags & (M_CSUM_UDPv4 | M_CSUM_UDPv6)) != 0;

	if (tcp) {
#ifdef IGC_DEBUG
		KASSERTMSG(ipproto == IPPROTO_TCP, "ipproto = %d", ipproto);
#endif
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_L4T_TCP;
		*olinfo_status |= IGC_TXD_POPTS_TXSM << 8;
		off = 1;
	} else if (udp) {
#ifdef IGC_DEBUG
		KASSERTMSG(ipproto == IPPROTO_UDP, "ipproto = %d", ipproto);
#endif
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_L4T_UDP;
		*olinfo_status |= IGC_TXD_POPTS_TXSM << 8;
		off = 1;
	}

	if (off == 0)
		return 0;

	type_tucmd_mlhl |= IGC_ADVTXD_DCMD_DEXT | IGC_ADVTXD_DTYP_CTXT;

	/* Now ready a context descriptor */
	struct igc_adv_tx_context_desc *txdesc =
	    (struct igc_adv_tx_context_desc *)&txr->tx_base[prod];

	/* Now copy bits into descriptor */
	igc_txdesc_sync(txr, prod,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	htolem32(&txdesc->vlan_macip_lens, vlan_macip_lens);
	htolem32(&txdesc->type_tucmd_mlhl, type_tucmd_mlhl);
	htolem32(&txdesc->seqnum_seed, 0);
	htolem32(&txdesc->mss_l4len_idx, 0);
	igc_txdesc_sync(txr, prod,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 1;
}

/*********************************************************************
 *
 *  Advanced Context Descriptor setup for TSO
 *
 *  XXX XXXRO
 *	Not working. Some packets are sent with correct csums, but
 *	others aren't. th->th_sum may be adjusted.
 *
 **********************************************************************/

static int
igc_tso_setup(struct tx_ring *txr, struct mbuf *mp, int prod,
    uint32_t *cmd_type_len, uint32_t *olinfo_status)
{
#if 1 /* notyet */
	return 0;
#else
	struct ether_vlan_header *evl;
	struct ip *ip;
	struct ip6_hdr *ip6;
	struct tcphdr *th;
	uint32_t type_tucmd_mlhl = 0;
	uint32_t vlan_macip_lens = 0;
	uint32_t mss_l4len_idx = 0;
	uint32_t ehlen, iphlen, tcphlen, paylen;
	uint16_t ehtype;

	/*
	 * In advanced descriptors the vlan tag must
	 * be placed into the context descriptor. Hence
	 * we need to make one even if not doing offloads.
	 */
#if NVLAN > 0
	if (vlan_has_tag(mp)) {
		vlan_macip_lens |= (uint32_t)vlan_get_tag(mp)
		    << IGC_ADVTXD_VLAN_SHIFT;
	}
#endif

	KASSERT(mp->m_len >= sizeof(struct ether_header));
	evl = mtod(mp, struct ether_vlan_header *);
	if (evl->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		KASSERT(mp->m_len >= sizeof(struct ether_vlan_header));
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		ehtype = evl->evl_proto;
	} else {
		ehlen = ETHER_HDR_LEN;
		ehtype = evl->evl_encap_proto;
	}

	vlan_macip_lens |= ehlen << IGC_ADVTXD_MACLEN_SHIFT;

	switch (ntohs(ehtype)) {
	case ETHERTYPE_IP:
		iphlen = M_CSUM_DATA_IPv4_IPHL(mp->m_pkthdr.csum_data);
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_IPV4;
		*olinfo_status |= IGC_TXD_POPTS_IXSM << 8;

		KASSERT(mp->m_len >= ehlen + sizeof(*ip));
		ip = (void *)(mtod(mp, char *) + ehlen);
		ip->ip_len = 0;
		KASSERT(iphlen == ip->ip_hl << 2);
		KASSERT(ip->ip_sum == 0);
		KASSERT(ip->ip_p == IPPROTO_TCP);

		KASSERT(mp->m_len >= ehlen + iphlen + sizeof(*th));
		th = (void *)((char *)ip + iphlen);
		th->th_sum = in_cksum_phdr(ip->ip_src.s_addr, ip->ip_dst.s_addr,
		    htons(IPPROTO_TCP));
		break;
	case ETHERTYPE_IPV6:
		iphlen = M_CSUM_DATA_IPv6_IPHL(mp->m_pkthdr.csum_data);
		type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_IPV6;

		KASSERT(mp->m_len >= ehlen + sizeof(*ip6));
		ip6 = (void *)(mtod(mp, char *) + ehlen);
		ip6->ip6_plen = 0;
		KASSERT(iphlen == sizeof(struct ip6_hdr));
		KASSERT(ip6->ip6_nxt == IPPROTO_TCP);

		KASSERT(mp->m_len >= ehlen + iphlen + sizeof(*th));
		th = (void *)((char *)ip6 + iphlen);
		tcphlen = th->th_off << 2;
		paylen = mp->m_pkthdr.len - ehlen - iphlen - tcphlen;
		th->th_sum = in6_cksum_phdr(&ip6->ip6_src, &ip6->ip6_dst, 0,
		    htonl(IPPROTO_TCP));
		break;
	default:
		panic("%s", __func__);
	}

	tcphlen = th->th_off << 2;
	paylen = mp->m_pkthdr.len - ehlen - iphlen - tcphlen;

	vlan_macip_lens |= iphlen;

	type_tucmd_mlhl |= IGC_ADVTXD_DCMD_DEXT | IGC_ADVTXD_DTYP_CTXT;
	type_tucmd_mlhl |= IGC_ADVTXD_TUCMD_L4T_TCP;

	mss_l4len_idx |= mp->m_pkthdr.segsz << IGC_ADVTXD_MSS_SHIFT;
	mss_l4len_idx |= tcphlen << IGC_ADVTXD_L4LEN_SHIFT;

	/* Now ready a context descriptor */
	struct igc_adv_tx_context_desc *txdesc =
	    (struct igc_adv_tx_context_desc *)&txr->tx_base[prod];

	/* Now copy bits into descriptor */
	igc_txdesc_sync(txr, prod,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	htolem32(&txdesc->vlan_macip_lens, vlan_macip_lens);
	htolem32(&txdesc->type_tucmd_mlhl, type_tucmd_mlhl);
	htolem32(&txdesc->seqnum_seed, 0);
	htolem32(&txdesc->mss_l4len_idx, mss_l4len_idx);
	igc_txdesc_sync(txr, prod,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	*cmd_type_len |= IGC_ADVTXD_DCMD_TSE;
	*olinfo_status |= IGC_TXD_POPTS_TXSM << 8;
	*olinfo_status |= paylen << IGC_ADVTXD_PAYLEN_SHIFT;

	return 1;
#endif /* notyet */
}

/*********************************************************************
 *
 *  Allocate memory for rx_buffer structures. Since we use one
 *  rx_buffer per received packet, the maximum number of rx_buffer's
 *  that we'll need is equal to the number of receive descriptors
 *  that we've allocated.
 *
 **********************************************************************/
static int
igc_allocate_receive_buffers(struct rx_ring *rxr)
{
	struct igc_softc *sc = rxr->sc;
	int error;

	rxr->rx_buffers =
	    kmem_zalloc(sc->num_rx_desc * sizeof(struct igc_rx_buf), KM_SLEEP);

	for (int id = 0; id < sc->num_rx_desc; id++) {
		struct igc_rx_buf *rxbuf = &rxr->rx_buffers[id];

		error = bus_dmamap_create(rxr->rxdma.dma_tag, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_WAITOK, &rxbuf->map);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create RX DMA map\n");
			goto fail;
		}
	}
	bus_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map, 0,
	    rxr->rxdma.dma_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 0;
 fail:
	return error;
}

/*********************************************************************
 *
 *  Allocate and initialize receive structures.
 *
 **********************************************************************/
static int
igc_setup_receive_structures(struct igc_softc *sc)
{

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		if (igc_setup_receive_ring(rxr))
			goto fail;
	}

	return 0;
 fail:
	igc_free_receive_structures(sc);
	return ENOBUFS;
}

/*********************************************************************
 *
 *  Initialize a receive ring and its buffers.
 *
 **********************************************************************/
static int
igc_setup_receive_ring(struct rx_ring *rxr)
{
	struct igc_softc *sc = rxr->sc;
	const int rsize = roundup2(
	    sc->num_rx_desc * sizeof(union igc_adv_rx_desc), IGC_DBA_ALIGN);

	/* Clear the ring contents. */
	memset(rxr->rx_base, 0, rsize);

	if (igc_allocate_receive_buffers(rxr))
		return ENOMEM;

	/* Setup our descriptor indices. */
	rxr->next_to_check = 0;
	rxr->last_desc_filled = 0;

	mutex_init(&rxr->rxr_lock, MUTEX_DEFAULT, IPL_NET);

	return 0;
}

/*********************************************************************
 *
 *  Enable receive unit.
 *
 **********************************************************************/
static void
igc_initialize_receive_unit(struct igc_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct igc_hw *hw = &sc->hw;
	uint32_t rctl, rxcsum, srrctl;

	DPRINTF(RX, "called\n");

	/*
	 * Make sure receives are disabled while setting
	 * up the descriptor ring.
	 */
	rctl = IGC_READ_REG(hw, IGC_RCTL);
	IGC_WRITE_REG(hw, IGC_RCTL, rctl & ~IGC_RCTL_EN);

	/* Setup the Receive Control Register */
	rctl &= ~(3 << IGC_RCTL_MO_SHIFT);
	rctl |= IGC_RCTL_EN | IGC_RCTL_BAM | IGC_RCTL_LBM_NO |
	    IGC_RCTL_RDMTS_HALF | (hw->mac.mc_filter_type << IGC_RCTL_MO_SHIFT);

#if 1
	/* Do not store bad packets */
	rctl &= ~IGC_RCTL_SBP;
#else
	/* for debug */
	rctl |= IGC_RCTL_SBP;
#endif

	/* Enable Long Packet receive */
	if (sc->hw.mac.max_frame_size > ETHER_MAX_LEN)
		rctl |= IGC_RCTL_LPE;
	else
		rctl &= ~IGC_RCTL_LPE;

	/* Strip the CRC */
	rctl |= IGC_RCTL_SECRC;

	/*
	 * Set the interrupt throttling rate. Value is calculated
	 * as DEFAULT_ITR = 1/(MAX_INTS_PER_SEC * 256ns)
	 *
	 * XXX Sync with Linux, especially for jumbo MTU or TSO.
	 * XXX Shouldn't be here?
	 */
	IGC_WRITE_REG(hw, IGC_ITR, DEFAULT_ITR);

	rxcsum = IGC_READ_REG(hw, IGC_RXCSUM);
	rxcsum &= ~(IGC_RXCSUM_IPOFL | IGC_RXCSUM_TUOFL | IGC_RXCSUM_PCSD);
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx)
		rxcsum |= IGC_RXCSUM_IPOFL;
	if (ifp->if_capenable & (IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
				 IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx))
		rxcsum |= IGC_RXCSUM_TUOFL;
	if (sc->sc_nqueues > 1)
		rxcsum |= IGC_RXCSUM_PCSD;
	IGC_WRITE_REG(hw, IGC_RXCSUM, rxcsum);

	if (sc->sc_nqueues > 1)
		igc_initialize_rss_mapping(sc);

	srrctl = 0;
#if 0
	srrctl |= 4096 >> IGC_SRRCTL_BSIZEPKT_SHIFT;
	rctl |= IGC_RCTL_SZ_4096 | IGC_RCTL_BSEX;
#else
	srrctl |= 2048 >> IGC_SRRCTL_BSIZEPKT_SHIFT;
	rctl |= IGC_RCTL_SZ_2048;
#endif

	/*
	 * If TX flow control is disabled and there's > 1 queue defined,
	 * enable DROP.
	 *
	 * This drops frames rather than hanging the RX MAC for all queues.
	 */
	if (sc->sc_nqueues > 1 &&
	    (sc->fc == igc_fc_none || sc->fc == igc_fc_rx_pause))
		srrctl |= IGC_SRRCTL_DROP_EN;

	/* Setup the Base and Length of the RX descriptor rings. */
	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];
		const uint64_t bus_addr =
		    rxr->rxdma.dma_map->dm_segs[0].ds_addr;

		IGC_WRITE_REG(hw, IGC_RXDCTL(iq), 0);

		srrctl |= IGC_SRRCTL_DESCTYPE_ADV_ONEBUF;

		IGC_WRITE_REG(hw, IGC_RDLEN(iq),
		    sc->num_rx_desc * sizeof(union igc_adv_rx_desc));
		IGC_WRITE_REG(hw, IGC_RDBAH(iq), (uint32_t)(bus_addr >> 32));
		IGC_WRITE_REG(hw, IGC_RDBAL(iq), (uint32_t)bus_addr);
		IGC_WRITE_REG(hw, IGC_SRRCTL(iq), srrctl);

		/* Setup the Head and Tail Descriptor Pointers */
		IGC_WRITE_REG(hw, IGC_RDH(iq), 0);
		IGC_WRITE_REG(hw, IGC_RDT(iq), 0 /* XXX rxr->last_desc_filled */);

		/* Enable this Queue */
		uint32_t rxdctl = IGC_READ_REG(hw, IGC_RXDCTL(iq));
		rxdctl |= IGC_RXDCTL_QUEUE_ENABLE;
		rxdctl &= 0xFFF00000;
		rxdctl |= IGC_RX_PTHRESH;
		rxdctl |= IGC_RX_HTHRESH << 8;
		rxdctl |= IGC_RX_WTHRESH << 16;
		IGC_WRITE_REG(hw, IGC_RXDCTL(iq), rxdctl);
	}

	/* Make sure VLAN Filters are off */
	rctl &= ~IGC_RCTL_VFE;

	/* Write out the settings */
	IGC_WRITE_REG(hw, IGC_RCTL, rctl);
}

/*********************************************************************
 *
 *  Free all receive rings.
 *
 **********************************************************************/
static void
igc_free_receive_structures(struct igc_softc *sc)
{

	for (int iq = 0; iq < sc->sc_nqueues; iq++) {
		struct rx_ring *rxr = &sc->rx_rings[iq];

		igc_free_receive_buffers(rxr);
	}
}

/*********************************************************************
 *
 *  Free receive ring data structures
 *
 **********************************************************************/
static void
igc_free_receive_buffers(struct rx_ring *rxr)
{
	struct igc_softc *sc = rxr->sc;

	if (rxr->rx_buffers != NULL) {
		for (int id = 0; id < sc->num_rx_desc; id++) {
			struct igc_rx_buf *rxbuf = &rxr->rx_buffers[id];
			bus_dmamap_t map = rxbuf->map;

			if (rxbuf->buf != NULL) {
				bus_dmamap_sync(rxr->rxdma.dma_tag, map,
				    0, map->dm_mapsize, BUS_DMASYNC_POSTREAD);
				bus_dmamap_unload(rxr->rxdma.dma_tag, map);
				m_freem(rxbuf->buf);
				rxbuf->buf = NULL;
			}
			bus_dmamap_destroy(rxr->rxdma.dma_tag, map);
			rxbuf->map = NULL;
		}
		kmem_free(rxr->rx_buffers,
		    sc->num_rx_desc * sizeof(struct igc_rx_buf));
		rxr->rx_buffers = NULL;
	}

	mutex_destroy(&rxr->rxr_lock);
}

/*********************************************************************
 *
 * Clear status registers in all RX descriptors.
 *
 **********************************************************************/
static void
igc_clear_receive_status(struct rx_ring *rxr)
{
	struct igc_softc *sc = rxr->sc;

	mutex_enter(&rxr->rxr_lock);

	for (int id = 0; id < sc->num_rx_desc; id++) {
		union igc_adv_rx_desc *rxdesc = &rxr->rx_base[id];

		igc_rxdesc_sync(rxr, id,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		rxdesc->wb.upper.status_error = 0;
		igc_rxdesc_sync(rxr, id,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	}

	mutex_exit(&rxr->rxr_lock);
}

/*
 * Initialise the RSS mapping for NICs that support multiple transmit/
 * receive rings.
 */
static void
igc_initialize_rss_mapping(struct igc_softc *sc)
{
	struct igc_hw *hw = &sc->hw;

	/*
	 * The redirection table controls which destination
	 * queue each bucket redirects traffic to.
	 * Each DWORD represents four queues, with the LSB
	 * being the first queue in the DWORD.
	 *
	 * This just allocates buckets to queues using round-robin
	 * allocation.
	 *
	 * NOTE: It Just Happens to line up with the default
	 * RSS allocation method.
	 */

	/* Warning FM follows */
	uint32_t reta = 0;
	for (int i = 0; i < 128; i++) {
		const int shift = 0; /* XXXRO */
		int queue_id = i % sc->sc_nqueues;
		/* Adjust if required */
		queue_id <<= shift;

		/*
		 * The low 8 bits are for hash value (n+0);
		 * The next 8 bits are for hash value (n+1), etc.
		 */
		reta >>= 8;
		reta |= ((uint32_t)queue_id) << 24;
		if ((i & 3) == 3) {
			IGC_WRITE_REG(hw, IGC_RETA(i >> 2), reta);
			reta = 0;
		}
	}

	/*
	 * MRQC: Multiple Receive Queues Command
	 * Set queuing to RSS control, number depends on the device.
	 */

	/* Set up random bits */
	uint32_t rss_key[RSS_KEYSIZE / sizeof(uint32_t)];
	rss_getkey((uint8_t *)rss_key);

	/* Now fill our hash function seeds */
	for (int i = 0; i < __arraycount(rss_key); i++)
		IGC_WRITE_REG_ARRAY(hw, IGC_RSSRK(0), i, rss_key[i]);

	/*
	 * Configure the RSS fields to hash upon.
	 */
	uint32_t mrqc = IGC_MRQC_ENABLE_RSS_4Q;
	mrqc |= IGC_MRQC_RSS_FIELD_IPV4 | IGC_MRQC_RSS_FIELD_IPV4_TCP;
	mrqc |= IGC_MRQC_RSS_FIELD_IPV6 | IGC_MRQC_RSS_FIELD_IPV6_TCP;
	mrqc |= IGC_MRQC_RSS_FIELD_IPV6_TCP_EX;

	IGC_WRITE_REG(hw, IGC_MRQC, mrqc);
}

/*
 * igc_get_hw_control sets the {CTRL_EXT|FWSM}:DRV_LOAD bit.
 * For ASF and Pass Through versions of f/w this means
 * that the driver is loaded. For AMT version type f/w
 * this means that the network i/f is open.
 */
static void
igc_get_hw_control(struct igc_softc *sc)
{
	const uint32_t ctrl_ext = IGC_READ_REG(&sc->hw, IGC_CTRL_EXT);

	IGC_WRITE_REG(&sc->hw, IGC_CTRL_EXT, ctrl_ext | IGC_CTRL_EXT_DRV_LOAD);
}

/*
 * igc_release_hw_control resets {CTRL_EXT|FWSM}:DRV_LOAD bit.
 * For ASF and Pass Through versions of f/w this means that
 * the driver is no longer loaded. For AMT versions of the
 * f/w this means that the network i/f is closed.
 */
static void
igc_release_hw_control(struct igc_softc *sc)
{
	const uint32_t ctrl_ext = IGC_READ_REG(&sc->hw, IGC_CTRL_EXT);

	IGC_WRITE_REG(&sc->hw, IGC_CTRL_EXT, ctrl_ext & ~IGC_CTRL_EXT_DRV_LOAD);
}

static int
igc_is_valid_ether_addr(uint8_t *addr)
{
	const char zero_addr[6] = { 0, 0, 0, 0, 0, 0 };

	if ((addr[0] & 1) || !bcmp(addr, zero_addr, ETHER_ADDR_LEN))
		return 0;

	return 1;
}

static void
igc_print_devinfo(struct igc_softc *sc)
{
	device_t dev = sc->sc_dev;
	struct igc_hw *hw = &sc->hw;
	struct igc_phy_info *phy = &hw->phy;
	u_int oui, model, rev;
	uint16_t id1, id2, nvm_ver, phy_ver;
	char descr[MII_MAX_DESCR_LEN];

	/* Print PHY Info */
	id1 = phy->id >> 16;
	/* The revision field in phy->id is cleard and it's in phy->revision */
	id2 = (phy->id & 0xfff0) | phy->revision;
	oui = MII_OUI(id1, id2);
	model = MII_MODEL(id2);
	rev = MII_REV(id2);
	mii_get_descr(descr, sizeof(descr), oui, model);
	if (descr[0])
		aprint_normal_dev(dev, "PHY: %s, rev. %d\n",
		    descr, rev);
	else
		aprint_normal_dev(dev,
		    "PHY OUI 0x%06x, model 0x%04x, rev. %d\n",
		    oui, model, rev);

	/* Get NVM version */
	hw->nvm.ops.read(hw, NVM_VERSION, 1, &nvm_ver);

	/* Get PHY FW version */
	phy->ops.read_reg(hw, 0x1e, &phy_ver);

	aprint_normal_dev(dev, "ROM image version %x.%02x",
	    (nvm_ver & NVM_VERSION_MAJOR) >> NVM_VERSION_MAJOR_SHIFT,
	    (nvm_ver & NVM_VERSION_MINOR));
	aprint_debug("(0x%04hx)", nvm_ver);

	aprint_normal(", PHY FW version 0x%04hx\n", phy_ver);
}
