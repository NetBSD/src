/* $NetBSD: ixv.c,v 1.125.2.14 2022/01/29 16:43:23 martin Exp $ */

/******************************************************************************

  Copyright (c) 2001-2017, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: head/sys/dev/ixgbe/if_ixv.c 331224 2018-03-19 20:55:05Z erj $*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixv.c,v 1.125.2.14 2022/01/29 16:43:23 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_net_mpsafe.h"
#endif

#include "ixgbe.h"
#include "vlan.h"

/************************************************************************
 * Driver version
 ************************************************************************/
static const char ixv_driver_version[] = "2.0.1-k";
/* XXX NetBSD: + 1.5.17 */

/************************************************************************
 * PCI Device ID Table
 *
 *   Used by probe to select devices to load on
 *   Last field stores an index into ixv_strings
 *   Last entry must be all 0s
 *
 *   { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 ************************************************************************/
static const ixgbe_vendor_info_t ixv_vendor_info_array[] =
{
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X540_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_VF, 0, 0, 0},
	/* required last entry */
	{0, 0, 0, 0, 0}
};

/************************************************************************
 * Table of branding strings
 ************************************************************************/
static const char *ixv_strings[] = {
	"Intel(R) PRO/10GbE Virtual Function Network Driver"
};

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int	ixv_probe(device_t, cfdata_t, void *);
static void	ixv_attach(device_t, device_t, void *);
static int	ixv_detach(device_t, int);
#if 0
static int	ixv_shutdown(device_t);
#endif
static int	ixv_ifflags_cb(struct ethercom *);
static int	ixv_ioctl(struct ifnet *, u_long, void *);
static int	ixv_init(struct ifnet *);
static void	ixv_init_locked(struct adapter *);
static void	ixv_ifstop(struct ifnet *, int);
static void	ixv_stop_locked(void *);
static void	ixv_init_device_features(struct adapter *);
static void	ixv_media_status(struct ifnet *, struct ifmediareq *);
static int	ixv_media_change(struct ifnet *);
static int	ixv_allocate_pci_resources(struct adapter *,
		    const struct pci_attach_args *);
static int	ixv_allocate_msix(struct adapter *,
		    const struct pci_attach_args *);
static int	ixv_configure_interrupts(struct adapter *);
static void	ixv_free_pci_resources(struct adapter *);
static void	ixv_local_timer(void *);
static void	ixv_local_timer_locked(void *);
static int	ixv_setup_interface(device_t, struct adapter *);
static int	ixv_negotiate_api(struct adapter *);

static void	ixv_initialize_transmit_units(struct adapter *);
static void	ixv_initialize_receive_units(struct adapter *);
static void	ixv_initialize_rss_mapping(struct adapter *);
static s32	ixv_check_link(struct adapter *);

static void	ixv_enable_intr(struct adapter *);
static void	ixv_disable_intr(struct adapter *);
static int	ixv_set_rxfilter(struct adapter *);
static void	ixv_update_link_status(struct adapter *);
static int	ixv_sysctl_debug(SYSCTLFN_PROTO);
static void	ixv_set_ivar(struct adapter *, u8, u8, s8);
static void	ixv_configure_ivars(struct adapter *);
static u8 *	ixv_mc_array_itr(struct ixgbe_hw *, u8 **, u32 *);
static void	ixv_eitr_write(struct adapter *, uint32_t, uint32_t);

static void	ixv_setup_vlan_tagging(struct adapter *);
static int	ixv_setup_vlan_support(struct adapter *);
static int	ixv_vlan_cb(struct ethercom *, uint16_t, bool);
static int	ixv_register_vlan(struct adapter *, u16);
static int	ixv_unregister_vlan(struct adapter *, u16);

static void	ixv_add_device_sysctls(struct adapter *);
static void	ixv_init_stats(struct adapter *);
static void	ixv_update_stats(struct adapter *);
static void	ixv_add_stats_sysctls(struct adapter *);
static void	ixv_clear_evcnt(struct adapter *);

/* Sysctl handlers */
static void	ixv_set_sysctl_value(struct adapter *, const char *,
		    const char *, int *, int);
static int	ixv_sysctl_interrupt_rate_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_next_to_check_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_next_to_refresh_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_rdh_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_rdt_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_tdt_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_tdh_handler(SYSCTLFN_PROTO);
static int	ixv_sysctl_rx_copy_len(SYSCTLFN_PROTO);

/* The MSI-X Interrupt handlers */
static int	ixv_msix_que(void *);
static int	ixv_msix_mbx(void *);

/* Deferred interrupt tasklets */
static void	ixv_handle_que(void *);
static void	ixv_handle_link(void *);

/* Workqueue handler for deferred work */
static void	ixv_handle_que_work(struct work *, void *);

const struct sysctlnode *ixv_sysctl_instance(struct adapter *);
static const ixgbe_vendor_info_t *ixv_lookup(const struct pci_attach_args *);

/************************************************************************
 * NetBSD Device Interface Entry Points
 ************************************************************************/
CFATTACH_DECL3_NEW(ixv, sizeof(struct adapter),
    ixv_probe, ixv_attach, ixv_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

#if 0
static driver_t ixv_driver = {
	"ixv", ixv_methods, sizeof(struct adapter),
};

devclass_t ixv_devclass;
DRIVER_MODULE(ixv, pci, ixv_driver, ixv_devclass, 0, 0);
MODULE_DEPEND(ixv, pci, 1, 1, 1);
MODULE_DEPEND(ixv, ether, 1, 1, 1);
#endif

/*
 * TUNEABLE PARAMETERS:
 */

/* Number of Queues - do not exceed MSI-X vectors - 1 */
static int ixv_num_queues = 0;
#define	TUNABLE_INT(__x, __y)
TUNABLE_INT("hw.ixv.num_queues", &ixv_num_queues);

/*
 * AIM: Adaptive Interrupt Moderation
 * which means that the interrupt rate
 * is varied over time based on the
 * traffic for that interrupt vector
 */
static bool ixv_enable_aim = false;
TUNABLE_INT("hw.ixv.enable_aim", &ixv_enable_aim);

static int ixv_max_interrupt_rate = (4000000 / IXGBE_LOW_LATENCY);
TUNABLE_INT("hw.ixv.max_interrupt_rate", &ixv_max_interrupt_rate);

/* How many packets rxeof tries to clean at a time */
static int ixv_rx_process_limit = 256;
TUNABLE_INT("hw.ixv.rx_process_limit", &ixv_rx_process_limit);

/* How many packets txeof tries to clean at a time */
static int ixv_tx_process_limit = 256;
TUNABLE_INT("hw.ixv.tx_process_limit", &ixv_tx_process_limit);

/* Which packet processing uses workqueue or softint */
static bool ixv_txrx_workqueue = false;

/*
 * Number of TX descriptors per ring,
 * setting higher than RX as this seems
 * the better performing choice.
 */
static int ixv_txd = PERFORM_TXD;
TUNABLE_INT("hw.ixv.txd", &ixv_txd);

/* Number of RX descriptors per ring */
static int ixv_rxd = PERFORM_RXD;
TUNABLE_INT("hw.ixv.rxd", &ixv_rxd);

/* Legacy Transmit (single queue) */
static int ixv_enable_legacy_tx = 0;
TUNABLE_INT("hw.ixv.enable_legacy_tx", &ixv_enable_legacy_tx);

#ifdef NET_MPSAFE
#define IXGBE_MPSAFE		1
#define IXGBE_CALLOUT_FLAGS	CALLOUT_MPSAFE
#define IXGBE_SOFTINT_FLAGS	SOFTINT_MPSAFE
#define IXGBE_WORKQUEUE_FLAGS	WQ_PERCPU | WQ_MPSAFE
#else
#define IXGBE_CALLOUT_FLAGS	0
#define IXGBE_SOFTINT_FLAGS	0
#define IXGBE_WORKQUEUE_FLAGS	WQ_PERCPU
#endif
#define IXGBE_WORKQUEUE_PRI PRI_SOFTNET

#if 0
static int (*ixv_start_locked)(struct ifnet *, struct tx_ring *);
static int (*ixv_ring_empty)(struct ifnet *, struct buf_ring *);
#endif

/************************************************************************
 * ixv_probe - Device identification routine
 *
 *   Determines if the driver should be loaded on
 *   adapter based on its PCI vendor/device ID.
 *
 *   return BUS_PROBE_DEFAULT on success, positive on failure
 ************************************************************************/
static int
ixv_probe(device_t dev, cfdata_t cf, void *aux)
{
#ifdef __HAVE_PCI_MSI_MSIX
	const struct pci_attach_args *pa = aux;

	return (ixv_lookup(pa) != NULL) ? 1 : 0;
#else
	return 0;
#endif
} /* ixv_probe */

static const ixgbe_vendor_info_t *
ixv_lookup(const struct pci_attach_args *pa)
{
	const ixgbe_vendor_info_t *ent;
	pcireg_t subid;

	INIT_DEBUGOUT("ixv_lookup: begin");

	if (PCI_VENDOR(pa->pa_id) != IXGBE_INTEL_VENDOR_ID)
		return NULL;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	for (ent = ixv_vendor_info_array; ent->vendor_id != 0; ent++) {
		if ((PCI_VENDOR(pa->pa_id) == ent->vendor_id) &&
		    (PCI_PRODUCT(pa->pa_id) == ent->device_id) &&
		    ((PCI_SUBSYS_VENDOR(subid) == ent->subvendor_id) ||
		     (ent->subvendor_id == 0)) &&
		    ((PCI_SUBSYS_ID(subid) == ent->subdevice_id) ||
		     (ent->subdevice_id == 0))) {
			return ent;
		}
	}

	return NULL;
}

/************************************************************************
 * ixv_attach - Device initialization routine
 *
 *   Called when the driver is being loaded.
 *   Identifies the type of hardware, allocates all resources
 *   and initializes the hardware.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static void
ixv_attach(device_t parent, device_t dev, void *aux)
{
	struct adapter *adapter;
	struct ixgbe_hw *hw;
	int		error = 0;
	pcireg_t	id, subid;
	const ixgbe_vendor_info_t *ent;
	const struct pci_attach_args *pa = aux;
	const char *apivstr;
	const char *str;
	char buf[256];

	INIT_DEBUGOUT("ixv_attach: begin");

	/*
	 * Make sure BUSMASTER is set, on a VM under
	 * KVM it may not be and will break things.
	 */
	ixgbe_pci_enable_busmaster(pa->pa_pc, pa->pa_tag);

	/* Allocate, clear, and link in our adapter structure */
	adapter = device_private(dev);
	adapter->hw.back = adapter;
	adapter->dev = dev;
	hw = &adapter->hw;

	adapter->init_locked = ixv_init_locked;
	adapter->stop_locked = ixv_stop_locked;

	adapter->osdep.pc = pa->pa_pc;
	adapter->osdep.tag = pa->pa_tag;
	if (pci_dma64_available(pa))
		adapter->osdep.dmat = pa->pa_dmat64;
	else
		adapter->osdep.dmat = pa->pa_dmat;
	adapter->osdep.attached = false;

	ent = ixv_lookup(pa);

	KASSERT(ent != NULL);

	aprint_normal(": %s, Version - %s\n",
	    ixv_strings[ent->index], ixv_driver_version);

	/* Core Lock Init */
	IXGBE_CORE_LOCK_INIT(adapter, device_xname(dev));

	/* Do base PCI setup - map BAR0 */
	if (ixv_allocate_pci_resources(adapter, pa)) {
		aprint_error_dev(dev, "ixv_allocate_pci_resources() failed!\n");
		error = ENXIO;
		goto err_out;
	}

	/* SYSCTL APIs */
	ixv_add_device_sysctls(adapter);

	/* Set up the timer callout */
	callout_init(&adapter->timer, IXGBE_CALLOUT_FLAGS);

	/* Save off the information about this board */
	id = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_ID_REG);
	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	hw->vendor_id = PCI_VENDOR(id);
	hw->device_id = PCI_PRODUCT(id);
	hw->revision_id =
	    PCI_REVISION(pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG));
	hw->subsystem_vendor_id = PCI_SUBSYS_VENDOR(subid);
	hw->subsystem_device_id = PCI_SUBSYS_ID(subid);

	/* A subset of set_mac_type */
	switch (hw->device_id) {
	case IXGBE_DEV_ID_82599_VF:
		hw->mac.type = ixgbe_mac_82599_vf;
		str = "82599 VF";
		break;
	case IXGBE_DEV_ID_X540_VF:
		hw->mac.type = ixgbe_mac_X540_vf;
		str = "X540 VF";
		break;
	case IXGBE_DEV_ID_X550_VF:
		hw->mac.type = ixgbe_mac_X550_vf;
		str = "X550 VF";
		break;
	case IXGBE_DEV_ID_X550EM_X_VF:
		hw->mac.type = ixgbe_mac_X550EM_x_vf;
		str = "X550EM X VF";
		break;
	case IXGBE_DEV_ID_X550EM_A_VF:
		hw->mac.type = ixgbe_mac_X550EM_a_vf;
		str = "X550EM A VF";
		break;
	default:
		/* Shouldn't get here since probe succeeded */
		aprint_error_dev(dev, "Unknown device ID!\n");
		error = ENXIO;
		goto err_out;
		break;
	}
	aprint_normal_dev(dev, "device %s\n", str);

	ixv_init_device_features(adapter);

	/* Initialize the shared code */
	error = ixgbe_init_ops_vf(hw);
	if (error) {
		aprint_error_dev(dev, "ixgbe_init_ops_vf() failed!\n");
		error = EIO;
		goto err_out;
	}

	/* Setup the mailbox */
	ixgbe_init_mbx_params_vf(hw);

	/* Set the right number of segments */
	KASSERT(IXGBE_82599_SCATTER_MAX >= IXGBE_SCATTER_DEFAULT);
	adapter->num_segs = IXGBE_SCATTER_DEFAULT;

	/* Reset mbox api to 1.0 */
	error = hw->mac.ops.reset_hw(hw);
	if (error == IXGBE_ERR_RESET_FAILED)
		aprint_error_dev(dev, "...reset_hw() failure: Reset Failed!\n");
	else if (error)
		aprint_error_dev(dev, "...reset_hw() failed with error %d\n",
		    error);
	if (error) {
		error = EIO;
		goto err_out;
	}

	error = hw->mac.ops.init_hw(hw);
	if (error) {
		aprint_error_dev(dev, "...init_hw() failed!\n");
		error = EIO;
		goto err_out;
	}

	/* Negotiate mailbox API version */
	error = ixv_negotiate_api(adapter);
	if (error)
		aprint_normal_dev(dev,
		    "MBX API negotiation failed during attach!\n");
	switch (hw->api_version) {
	case ixgbe_mbox_api_10:
		apivstr = "1.0";
		break;
	case ixgbe_mbox_api_20:
		apivstr = "2.0";
		break;
	case ixgbe_mbox_api_11:
		apivstr = "1.1";
		break;
	case ixgbe_mbox_api_12:
		apivstr = "1.2";
		break;
	case ixgbe_mbox_api_13:
		apivstr = "1.3";
		break;
	default:
		apivstr = "unknown";
		break;
	}
	aprint_normal_dev(dev, "Mailbox API %s\n", apivstr);

	/* If no mac address was assigned, make a random one */
	if (!ixv_check_ether_addr(hw->mac.addr)) {
		u8 addr[ETHER_ADDR_LEN];
		uint64_t rndval = cprng_strong64();

		memcpy(addr, &rndval, sizeof(addr));
		addr[0] &= 0xFE;
		addr[0] |= 0x02;
		bcopy(addr, hw->mac.addr, sizeof(addr));
	}

	/* Register for VLAN events */
	ether_set_vlan_cb(&adapter->osdep.ec, ixv_vlan_cb);

	/* Sysctls for limiting the amount of work done in the taskqueues */
	ixv_set_sysctl_value(adapter, "rx_processing_limit",
	    "max number of rx packets to process",
	    &adapter->rx_process_limit, ixv_rx_process_limit);

	ixv_set_sysctl_value(adapter, "tx_processing_limit",
	    "max number of tx packets to process",
	    &adapter->tx_process_limit, ixv_tx_process_limit);

	/* Do descriptor calc and sanity checks */
	if (((ixv_txd * sizeof(union ixgbe_adv_tx_desc)) % DBA_ALIGN) != 0 ||
	    ixv_txd < MIN_TXD || ixv_txd > MAX_TXD) {
		aprint_error_dev(dev, "TXD config issue, using default!\n");
		adapter->num_tx_desc = DEFAULT_TXD;
	} else
		adapter->num_tx_desc = ixv_txd;

	if (((ixv_rxd * sizeof(union ixgbe_adv_rx_desc)) % DBA_ALIGN) != 0 ||
	    ixv_rxd < MIN_RXD || ixv_rxd > MAX_RXD) {
		aprint_error_dev(dev, "RXD config issue, using default!\n");
		adapter->num_rx_desc = DEFAULT_RXD;
	} else
		adapter->num_rx_desc = ixv_rxd;

	/* Set default high limit of copying mbuf in rxeof */
	adapter->rx_copy_len = IXGBE_RX_COPY_LEN_MAX;

	/* Setup MSI-X */
	error = ixv_configure_interrupts(adapter);
	if (error)
		goto err_out;

	/* Allocate our TX/RX Queues */
	if (ixgbe_allocate_queues(adapter)) {
		aprint_error_dev(dev, "ixgbe_allocate_queues() failed!\n");
		error = ENOMEM;
		goto err_out;
	}

	/* hw.ix defaults init */
	adapter->enable_aim = ixv_enable_aim;

	adapter->txrx_use_workqueue = ixv_txrx_workqueue;

	error = ixv_allocate_msix(adapter, pa);
	if (error) {
		aprint_error_dev(dev, "ixv_allocate_msix() failed!\n");
		goto err_late;
	}

	/* Setup OS specific network interface */
	error = ixv_setup_interface(dev, adapter);
	if (error != 0) {
		aprint_error_dev(dev, "ixv_setup_interface() failed!\n");
		goto err_late;
	}

	/* Do the stats setup */
	ixv_init_stats(adapter);
	ixv_add_stats_sysctls(adapter);

	if (adapter->feat_en & IXGBE_FEATURE_NETMAP)
		ixgbe_netmap_attach(adapter);

	snprintb(buf, sizeof(buf), IXGBE_FEATURE_FLAGS, adapter->feat_cap);
	aprint_verbose_dev(dev, "feature cap %s\n", buf);
	snprintb(buf, sizeof(buf), IXGBE_FEATURE_FLAGS, adapter->feat_en);
	aprint_verbose_dev(dev, "feature ena %s\n", buf);

	INIT_DEBUGOUT("ixv_attach: end");
	adapter->osdep.attached = true;

	return;

err_late:
	ixgbe_free_queues(adapter);
err_out:
	ixv_free_pci_resources(adapter);
	IXGBE_CORE_LOCK_DESTROY(adapter);

	return;
} /* ixv_attach */

/************************************************************************
 * ixv_detach - Device removal routine
 *
 *   Called when the driver is being removed.
 *   Stops the adapter and deallocates all the resources
 *   that were allocated for driver operation.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static int
ixv_detach(device_t dev, int flags)
{
	struct adapter	*adapter = device_private(dev);
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = adapter->queues;
	struct tx_ring *txr = adapter->tx_rings;
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;

	INIT_DEBUGOUT("ixv_detach: begin");
	if (adapter->osdep.attached == false)
		return 0;

	/* Stop the interface. Callouts are stopped in it. */
	ixv_ifstop(adapter->ifp, 1);

#if NVLAN > 0
	/* Make sure VLANs are not using driver */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		;	/* nothing to do: no VLANs */
	else if ((flags & (DETACH_SHUTDOWN | DETACH_FORCE)) != 0)
		vlan_ifdetach(adapter->ifp);
	else {
		aprint_error_dev(dev, "VLANs in use, detach first\n");
		return EBUSY;
	}
#endif

	for (int i = 0; i < adapter->num_queues; i++, que++, txr++) {
		if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX))
			softint_disestablish(txr->txr_si);
		softint_disestablish(que->que_si);
	}
	if (adapter->txr_wq != NULL)
		workqueue_destroy(adapter->txr_wq);
	if (adapter->txr_wq_enqueued != NULL)
		percpu_free(adapter->txr_wq_enqueued, sizeof(u_int));
	if (adapter->que_wq != NULL)
		workqueue_destroy(adapter->que_wq);

	/* Drain the Mailbox(link) queue */
	softint_disestablish(adapter->link_si);

	ether_ifdetach(adapter->ifp);
	callout_halt(&adapter->timer, NULL);

	if (adapter->feat_en & IXGBE_FEATURE_NETMAP)
		netmap_detach(adapter->ifp);

	ixv_free_pci_resources(adapter);
#if 0 /* XXX the NetBSD port is probably missing something here */
	bus_generic_detach(dev);
#endif
	if_detach(adapter->ifp);
	if_percpuq_destroy(adapter->ipq);

	sysctl_teardown(&adapter->sysctllog);
	evcnt_detach(&adapter->efbig_tx_dma_setup);
	evcnt_detach(&adapter->mbuf_defrag_failed);
	evcnt_detach(&adapter->efbig2_tx_dma_setup);
	evcnt_detach(&adapter->einval_tx_dma_setup);
	evcnt_detach(&adapter->other_tx_dma_setup);
	evcnt_detach(&adapter->eagain_tx_dma_setup);
	evcnt_detach(&adapter->enomem_tx_dma_setup);
	evcnt_detach(&adapter->watchdog_events);
	evcnt_detach(&adapter->tso_err);
	evcnt_detach(&adapter->link_irq);

	txr = adapter->tx_rings;
	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		evcnt_detach(&adapter->queues[i].irqs);
		evcnt_detach(&adapter->queues[i].handleq);
		evcnt_detach(&adapter->queues[i].req);
		evcnt_detach(&txr->no_desc_avail);
		evcnt_detach(&txr->total_packets);
		evcnt_detach(&txr->tso_tx);
#ifndef IXGBE_LEGACY_TX
		evcnt_detach(&txr->pcq_drops);
#endif

		evcnt_detach(&rxr->rx_packets);
		evcnt_detach(&rxr->rx_bytes);
		evcnt_detach(&rxr->rx_copies);
		evcnt_detach(&rxr->no_mbuf);
		evcnt_detach(&rxr->rx_discarded);
	}
	evcnt_detach(&stats->ipcs);
	evcnt_detach(&stats->l4cs);
	evcnt_detach(&stats->ipcs_bad);
	evcnt_detach(&stats->l4cs_bad);

	/* Packet Reception Stats */
	evcnt_detach(&stats->vfgorc);
	evcnt_detach(&stats->vfgprc);
	evcnt_detach(&stats->vfmprc);

	/* Packet Transmission Stats */
	evcnt_detach(&stats->vfgotc);
	evcnt_detach(&stats->vfgptc);

	/* Mailbox Stats */
	evcnt_detach(&hw->mbx.stats.msgs_tx);
	evcnt_detach(&hw->mbx.stats.msgs_rx);
	evcnt_detach(&hw->mbx.stats.acks);
	evcnt_detach(&hw->mbx.stats.reqs);
	evcnt_detach(&hw->mbx.stats.rsts);

	ixgbe_free_queues(adapter);

	IXGBE_CORE_LOCK_DESTROY(adapter);

	return (0);
} /* ixv_detach */

/************************************************************************
 * ixv_init_locked - Init entry point
 *
 *   Used in two ways: It is used by the stack as an init entry
 *   point in network interface structure. It is also used
 *   by the driver as a hw/sw initialization routine to get
 *   to a consistent state.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static void
ixv_init_locked(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	device_t	dev = adapter->dev;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue	*que;
	int		error = 0;
	uint32_t mask;
	int i;

	INIT_DEBUGOUT("ixv_init_locked: begin");
	KASSERT(mutex_owned(&adapter->core_mtx));
	hw->adapter_stopped = FALSE;
	hw->mac.ops.stop_adapter(hw);
	callout_stop(&adapter->timer);
	for (i = 0, que = adapter->queues; i < adapter->num_queues; i++, que++)
		que->disabled_count = 0;

	adapter->max_frame_size =
	    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

	/* reprogram the RAR[0] in case user changed it. */
	hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

	/* Get the latest mac address, User can use a LAA */
	memcpy(hw->mac.addr, CLLADDR(ifp->if_sadl),
	     IXGBE_ETH_LENGTH_OF_ADDRESS);
	hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, 1);

	/* Prepare transmit descriptors and buffers */
	if (ixgbe_setup_transmit_structures(adapter)) {
		aprint_error_dev(dev, "Could not setup transmit structures\n");
		ixv_stop_locked(adapter);
		return;
	}

	/* Reset VF and renegotiate mailbox API version */
	hw->mac.ops.reset_hw(hw);
	hw->mac.ops.start_hw(hw);
	error = ixv_negotiate_api(adapter);
	if (error)
		device_printf(dev,
		    "Mailbox API negotiation failed in init_locked!\n");

	ixv_initialize_transmit_units(adapter);

	/* Setup Multicast table */
	ixv_set_rxfilter(adapter);

	/* Use fixed buffer size, even for jumbo frames */
	adapter->rx_mbuf_sz = MCLBYTES;

	/* Prepare receive descriptors and buffers */
	error = ixgbe_setup_receive_structures(adapter);
	if (error) {
		device_printf(dev,
		    "Could not setup receive structures (err = %d)\n", error);
		ixv_stop_locked(adapter);
		return;
	}

	/* Configure RX settings */
	ixv_initialize_receive_units(adapter);

	/* Set up VLAN offload and filter */
	ixv_setup_vlan_support(adapter);

	/* Set up MSI-X routing */
	ixv_configure_ivars(adapter);

	/* Set up auto-mask */
	mask = (1 << adapter->vector);
	for (i = 0, que = adapter->queues; i < adapter->num_queues; i++, que++)
		mask |= (1 << que->msix);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIAM, mask);

	/* Set moderation on the Link interrupt */
	ixv_eitr_write(adapter, adapter->vector, IXGBE_LINK_ITR);

	/* Stats init */
	ixv_init_stats(adapter);

	/* Config/Enable Link */
	hw->mac.get_link_status = TRUE;
	hw->mac.ops.check_link(hw, &adapter->link_speed, &adapter->link_up,
	    FALSE);

	/* Start watchdog */
	callout_reset(&adapter->timer, hz, ixv_local_timer, adapter);

	/* And now turn on interrupts */
	ixv_enable_intr(adapter);

	/* Update saved flags. See ixgbe_ifflags_cb() */
	adapter->if_flags = ifp->if_flags;
	adapter->ec_capenable = adapter->osdep.ec.ec_capenable;

	/* Now inform the stack we're ready */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return;
} /* ixv_init_locked */

/************************************************************************
 * ixv_enable_queue
 ************************************************************************/
static inline void
ixv_enable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = &adapter->queues[vector];
	u32		queue = 1UL << vector;
	u32		mask;

	mutex_enter(&que->dc_mtx);
	if (que->disabled_count > 0 && --que->disabled_count > 0)
		goto out;

	mask = (IXGBE_EIMS_RTX_QUEUE & queue);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, mask);
out:
	mutex_exit(&que->dc_mtx);
} /* ixv_enable_queue */

/************************************************************************
 * ixv_disable_queue
 ************************************************************************/
static inline void
ixv_disable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = &adapter->queues[vector];
	u32		queue = 1UL << vector;
	u32		mask;

	mutex_enter(&que->dc_mtx);
	if (que->disabled_count++ > 0)
		goto  out;

	mask = (IXGBE_EIMS_RTX_QUEUE & queue);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIMC, mask);
out:
	mutex_exit(&que->dc_mtx);
} /* ixv_disable_queue */

#if 0
static inline void
ixv_rearm_queues(struct adapter *adapter, u64 queues)
{
	u32 mask = (IXGBE_EIMS_RTX_QUEUE & queues);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEICS, mask);
} /* ixv_rearm_queues */
#endif


/************************************************************************
 * ixv_msix_que - MSI-X Queue Interrupt Service routine
 ************************************************************************/
static int
ixv_msix_que(void *arg)
{
	struct ix_queue	*que = arg;
	struct adapter	*adapter = que->adapter;
	struct tx_ring	*txr = que->txr;
	struct rx_ring	*rxr = que->rxr;
	bool		more;
	u32		newitr = 0;

	ixv_disable_queue(adapter, que->msix);
	++que->irqs.ev_count;

#ifdef __NetBSD__
	/* Don't run ixgbe_rxeof in interrupt context */
	more = true;
#else
	more = ixgbe_rxeof(que);
#endif

	IXGBE_TX_LOCK(txr);
	ixgbe_txeof(txr);
	IXGBE_TX_UNLOCK(txr);

	/* Do AIM now? */

	if (adapter->enable_aim == false)
		goto no_calc;
	/*
	 * Do Adaptive Interrupt Moderation:
	 *  - Write out last calculated setting
	 *  - Calculate based on average size over
	 *    the last interval.
	 */
	if (que->eitr_setting)
		ixv_eitr_write(adapter, que->msix, que->eitr_setting);

	que->eitr_setting = 0;

	/* Idle, do nothing */
	if ((txr->bytes == 0) && (rxr->bytes == 0))
		goto no_calc;

	if ((txr->bytes) && (txr->packets))
		newitr = txr->bytes/txr->packets;
	if ((rxr->bytes) && (rxr->packets))
		newitr = uimax(newitr, (rxr->bytes / rxr->packets));
	newitr += 24; /* account for hardware frame, crc */

	/* set an upper boundary */
	newitr = uimin(newitr, 3000);

	/* Be nice to the mid range */
	if ((newitr > 300) && (newitr < 1200))
		newitr = (newitr / 3);
	else
		newitr = (newitr / 2);

	/*
	 * When RSC is used, ITR interval must be larger than RSC_DELAY.
	 * Currently, we use 2us for RSC_DELAY. The minimum value is always
	 * greater than 2us on 100M (and 10M?(not documented)), but it's not
	 * on 1G and higher.
	 */
	if ((adapter->link_speed != IXGBE_LINK_SPEED_100_FULL)
	    && (adapter->link_speed != IXGBE_LINK_SPEED_10_FULL)) {
		if (newitr < IXGBE_MIN_RSC_EITR_10G1G)
			newitr = IXGBE_MIN_RSC_EITR_10G1G;
	}

	/* save for next interrupt */
	que->eitr_setting = newitr;

	/* Reset state */
	txr->bytes = 0;
	txr->packets = 0;
	rxr->bytes = 0;
	rxr->packets = 0;

no_calc:
	if (more)
		softint_schedule(que->que_si);
	else /* Re-enable this interrupt */
		ixv_enable_queue(adapter, que->msix);

	return 1;
} /* ixv_msix_que */

/************************************************************************
 * ixv_msix_mbx
 ************************************************************************/
static int
ixv_msix_mbx(void *arg)
{
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;

	++adapter->link_irq.ev_count;
	/* NetBSD: We use auto-clear, so it's not required to write VTEICR */

	/* Link status change */
	hw->mac.get_link_status = TRUE;
	softint_schedule(adapter->link_si);

	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, (1 << adapter->vector));

	return 1;
} /* ixv_msix_mbx */

static void
ixv_eitr_write(struct adapter *adapter, uint32_t index, uint32_t itr)
{

	/*
	 * Newer devices than 82598 have VF function, so this function is
	 * simple.
	 */
	itr |= IXGBE_EITR_CNT_WDIS;

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEITR(index), itr);
}


/************************************************************************
 * ixv_media_status - Media Ioctl callback
 *
 *   Called whenever the user queries the status of
 *   the interface using ifconfig.
 ************************************************************************/
static void
ixv_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct adapter *adapter = ifp->if_softc;

	INIT_DEBUGOUT("ixv_media_status: begin");
	IXGBE_CORE_LOCK(adapter);
	ixv_update_link_status(adapter);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (adapter->link_active != LINK_STATE_UP) {
		ifmr->ifm_active |= IFM_NONE;
		IXGBE_CORE_UNLOCK(adapter);
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;

	switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_5GB_FULL:
			ifmr->ifm_active |= IFM_5000_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_100_FULL:
			ifmr->ifm_active |= IFM_100_TX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_10_FULL:
			ifmr->ifm_active |= IFM_10_T | IFM_FDX;
			break;
	}

	ifp->if_baudrate = ifmedia_baudrate(ifmr->ifm_active);

	IXGBE_CORE_UNLOCK(adapter);
} /* ixv_media_status */

/************************************************************************
 * ixv_media_change - Media Ioctl callback
 *
 *   Called when the user changes speed/duplex using
 *   media/mediopt option with ifconfig.
 ************************************************************************/
static int
ixv_media_change(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;
	struct ifmedia *ifm = &adapter->media;

	INIT_DEBUGOUT("ixv_media_change: begin");

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		break;
	default:
		device_printf(adapter->dev, "Only auto media type\n");
		return (EINVAL);
	}

	return (0);
} /* ixv_media_change */

/************************************************************************
 * ixv_negotiate_api
 *
 *   Negotiate the Mailbox API with the PF;
 *   start with the most featured API first.
 ************************************************************************/
static int
ixv_negotiate_api(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	int		mbx_api[] = { ixgbe_mbox_api_13,
				      ixgbe_mbox_api_12,
				      ixgbe_mbox_api_11,
				      ixgbe_mbox_api_10,
				      ixgbe_mbox_api_unknown };
	int		i = 0;

	while (mbx_api[i] != ixgbe_mbox_api_unknown) {
		if (ixgbevf_negotiate_api_version(hw, mbx_api[i]) == 0)
			return (0);
		i++;
	}

	return (EINVAL);
} /* ixv_negotiate_api */


/************************************************************************
 * ixv_set_rxfilter - Multicast Update
 *
 *   Called whenever multicast address list is updated.
 ************************************************************************/
static int
ixv_set_rxfilter(struct adapter *adapter)
{
	u8	mta[IXGBE_MAX_VF_MC * IXGBE_ETH_LENGTH_OF_ADDRESS];
	struct ifnet		*ifp = adapter->ifp;
	struct ixgbe_hw		*hw = &adapter->hw;
	u8			*update_ptr;
	int			mcnt = 0;
	struct ethercom		*ec = &adapter->osdep.ec;
	struct ether_multi	*enm;
	struct ether_multistep	step;
	bool			overflow = false;
	int			error, rc = 0;

	KASSERT(mutex_owned(&adapter->core_mtx));
	IOCTL_DEBUGOUT("ixv_set_rxfilter: begin");

	/* 1: For PROMISC */
	if (ifp->if_flags & IFF_PROMISC) {
		error = hw->mac.ops.update_xcast_mode(hw,
		    IXGBEVF_XCAST_MODE_PROMISC);
		if (error == IXGBE_ERR_NOT_TRUSTED) {
			device_printf(adapter->dev,
			    "this interface is not trusted\n");
			error = EPERM;
		} else if (error == IXGBE_ERR_FEATURE_NOT_SUPPORTED) {
			device_printf(adapter->dev,
			    "the PF doesn't support promisc mode\n");
			error = EOPNOTSUPP;
		} else if (error == IXGBE_ERR_NOT_IN_PROMISC) {
			device_printf(adapter->dev,
			    "the PF may not in promisc mode\n");
			error = EINVAL;
		} else if (error) {
			device_printf(adapter->dev,
			    "failed to set promisc mode. error = %d\n",
			    error);
			error = EIO;
		} else
			return 0;
		rc = error;
	}

	/* 2: For ALLMULTI or normal */
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if ((mcnt >= IXGBE_MAX_VF_MC) ||
		    (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			ETHER_ADDR_LEN) != 0)) {
			overflow = true;
			break;
		}
		bcopy(enm->enm_addrlo,
		    &mta[mcnt * IXGBE_ETH_LENGTH_OF_ADDRESS],
		    IXGBE_ETH_LENGTH_OF_ADDRESS);
		mcnt++;
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);

	/* 3: For ALLMULTI */
	if (overflow) {
		error = hw->mac.ops.update_xcast_mode(hw,
		    IXGBEVF_XCAST_MODE_ALLMULTI);
		if (error == IXGBE_ERR_NOT_TRUSTED) {
			device_printf(adapter->dev,
			    "this interface is not trusted\n");
			error = EPERM;
		} else if (error == IXGBE_ERR_FEATURE_NOT_SUPPORTED) {
			device_printf(adapter->dev,
			    "the PF doesn't support allmulti mode\n");
			error = EOPNOTSUPP;
		} else if (error) {
			device_printf(adapter->dev,
			    "number of Ethernet multicast addresses "
			    "exceeds the limit (%d). error = %d\n",
			    IXGBE_MAX_VF_MC, error);
			error = ENOSPC;
		} else {
			ETHER_LOCK(ec);
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			return rc; /* Promisc might have failed */
		}

		if (rc == 0)
			rc = error;

		/* Continue to update the multicast table as many as we can */
	}

	/* 4: For normal operation */
	error = hw->mac.ops.update_xcast_mode(hw, IXGBEVF_XCAST_MODE_MULTI);
	if ((error == IXGBE_ERR_FEATURE_NOT_SUPPORTED) || (error == 0)) {
		/* Normal operation */
		ETHER_LOCK(ec);
		ec->ec_flags &= ~ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		error = 0;
	} else if (error) {
		device_printf(adapter->dev,
		    "failed to set Ethernet multicast address "
		    "operation to normal. error = %d\n", error);
	}

	update_ptr = mta;

	error = adapter->hw.mac.ops.update_mc_addr_list(&adapter->hw,
	    update_ptr, mcnt, ixv_mc_array_itr, TRUE);
	if (rc == 0)
		rc = error;

	return rc;
} /* ixv_set_rxfilter */

/************************************************************************
 * ixv_mc_array_itr
 *
 *   An iterator function needed by the multicast shared code.
 *   It feeds the shared code routine the addresses in the
 *   array of ixv_set_rxfilter() one by one.
 ************************************************************************/
static u8 *
ixv_mc_array_itr(struct ixgbe_hw *hw, u8 **update_ptr, u32 *vmdq)
{
	u8 *addr = *update_ptr;
	u8 *newptr;

	*vmdq = 0;

	newptr = addr + IXGBE_ETH_LENGTH_OF_ADDRESS;
	*update_ptr = newptr;

	return addr;
} /* ixv_mc_array_itr */

/************************************************************************
 * ixv_local_timer - Timer routine
 *
 *   Checks for link status, updates statistics,
 *   and runs the watchdog check.
 ************************************************************************/
static void
ixv_local_timer(void *arg)
{
	struct adapter *adapter = arg;

	IXGBE_CORE_LOCK(adapter);
	ixv_local_timer_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

static void
ixv_local_timer_locked(void *arg)
{
	struct adapter	*adapter = arg;
	device_t	dev = adapter->dev;
	struct ix_queue	*que = adapter->queues;
	u64		queues = 0;
	u64		v0, v1, v2, v3, v4, v5, v6, v7;
	int		hung = 0;
	int		i;

	KASSERT(mutex_owned(&adapter->core_mtx));

	if (ixv_check_link(adapter)) {
		ixv_init_locked(adapter);
		return;
	}

	/* Stats Update */
	ixv_update_stats(adapter);

	/* Update some event counters */
	v0 = v1 = v2 = v3 = v4 = v5 = v6 = v7 = 0;
	que = adapter->queues;
	for (i = 0; i < adapter->num_queues; i++, que++) {
		struct tx_ring	*txr = que->txr;

		v0 += txr->q_efbig_tx_dma_setup;
		v1 += txr->q_mbuf_defrag_failed;
		v2 += txr->q_efbig2_tx_dma_setup;
		v3 += txr->q_einval_tx_dma_setup;
		v4 += txr->q_other_tx_dma_setup;
		v5 += txr->q_eagain_tx_dma_setup;
		v6 += txr->q_enomem_tx_dma_setup;
		v7 += txr->q_tso_err;
	}
	adapter->efbig_tx_dma_setup.ev_count = v0;
	adapter->mbuf_defrag_failed.ev_count = v1;
	adapter->efbig2_tx_dma_setup.ev_count = v2;
	adapter->einval_tx_dma_setup.ev_count = v3;
	adapter->other_tx_dma_setup.ev_count = v4;
	adapter->eagain_tx_dma_setup.ev_count = v5;
	adapter->enomem_tx_dma_setup.ev_count = v6;
	adapter->tso_err.ev_count = v7;

	/*
	 * Check the TX queues status
	 *	- mark hung queues so we don't schedule on them
	 *	- watchdog only if all queues show hung
	 */
	que = adapter->queues;
	for (i = 0; i < adapter->num_queues; i++, que++) {
		/* Keep track of queues with work for soft irq */
		if (que->txr->busy)
			queues |= ((u64)1 << que->me);
		/*
		 * Each time txeof runs without cleaning, but there
		 * are uncleaned descriptors it increments busy. If
		 * we get to the MAX we declare it hung.
		 */
		if (que->busy == IXGBE_QUEUE_HUNG) {
			++hung;
			/* Mark the queue as inactive */
			adapter->active_queues &= ~((u64)1 << que->me);
			continue;
		} else {
			/* Check if we've come back from hung */
			if ((adapter->active_queues & ((u64)1 << que->me)) == 0)
				adapter->active_queues |= ((u64)1 << que->me);
		}
		if (que->busy >= IXGBE_MAX_TX_BUSY) {
			device_printf(dev,
			    "Warning queue %d appears to be hung!\n", i);
			que->txr->busy = IXGBE_QUEUE_HUNG;
			++hung;
		}
	}

	/* Only truly watchdog if all queues show hung */
	if (hung == adapter->num_queues)
		goto watchdog;
#if 0
	else if (queues != 0) { /* Force an IRQ on queues with work */
		ixv_rearm_queues(adapter, queues);
	}
#endif

	callout_reset(&adapter->timer, hz, ixv_local_timer, adapter);

	return;

watchdog:
	device_printf(adapter->dev, "Watchdog timeout -- resetting\n");
	adapter->ifp->if_flags &= ~IFF_RUNNING;
	adapter->watchdog_events.ev_count++;
	ixv_init_locked(adapter);
} /* ixv_local_timer */

/************************************************************************
 * ixv_update_link_status - Update OS on link state
 *
 * Note: Only updates the OS on the cached link state.
 *	 The real check of the hardware only happens with
 *	 a link interrupt.
 ************************************************************************/
static void
ixv_update_link_status(struct adapter *adapter)
{
	struct ifnet *ifp = adapter->ifp;
	device_t     dev = adapter->dev;

	KASSERT(mutex_owned(&adapter->core_mtx));

	if (adapter->link_up) {
		if (adapter->link_active != LINK_STATE_UP) {
			if (bootverbose) {
				const char *bpsmsg;

				switch (adapter->link_speed) {
				case IXGBE_LINK_SPEED_10GB_FULL:
					bpsmsg = "10 Gbps";
					break;
				case IXGBE_LINK_SPEED_5GB_FULL:
					bpsmsg = "5 Gbps";
					break;
				case IXGBE_LINK_SPEED_2_5GB_FULL:
					bpsmsg = "2.5 Gbps";
					break;
				case IXGBE_LINK_SPEED_1GB_FULL:
					bpsmsg = "1 Gbps";
					break;
				case IXGBE_LINK_SPEED_100_FULL:
					bpsmsg = "100 Mbps";
					break;
				case IXGBE_LINK_SPEED_10_FULL:
					bpsmsg = "10 Mbps";
					break;
				default:
					bpsmsg = "unknown speed";
					break;
				}
				device_printf(dev, "Link is up %s %s \n",
				    bpsmsg, "Full Duplex");
			}
			adapter->link_active = LINK_STATE_UP;
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else {
		/*
		 * Do it when link active changes to DOWN. i.e.
		 * a) LINK_STATE_UNKNOWN -> LINK_STATE_DOWN
		 * b) LINK_STATE_UP	 -> LINK_STATE_DOWN
		 */
		if (adapter->link_active != LINK_STATE_DOWN) {
			if (bootverbose)
				device_printf(dev, "Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			adapter->link_active = LINK_STATE_DOWN;
		}
	}
} /* ixv_update_link_status */


/************************************************************************
 * ixv_stop - Stop the hardware
 *
 *   Disables all traffic on the adapter by issuing a
 *   global reset on the MAC and deallocates TX/RX buffers.
 ************************************************************************/
static void
ixv_ifstop(struct ifnet *ifp, int disable)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixv_stop_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

static void
ixv_stop_locked(void *arg)
{
	struct ifnet	*ifp;
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;

	ifp = adapter->ifp;

	KASSERT(mutex_owned(&adapter->core_mtx));

	INIT_DEBUGOUT("ixv_stop_locked: begin\n");
	ixv_disable_intr(adapter);

	/* Tell the stack that the interface is no longer active */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	hw->mac.ops.reset_hw(hw);
	adapter->hw.adapter_stopped = FALSE;
	hw->mac.ops.stop_adapter(hw);
	callout_stop(&adapter->timer);

	/* reprogram the RAR[0] in case user changed it. */
	hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

	return;
} /* ixv_stop_locked */


/************************************************************************
 * ixv_allocate_pci_resources
 ************************************************************************/
static int
ixv_allocate_pci_resources(struct adapter *adapter,
    const struct pci_attach_args *pa)
{
	pcireg_t	memtype, csr;
	device_t	dev = adapter->dev;
	bus_addr_t addr;
	int flags;

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_BAR(0));
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		adapter->osdep.mem_bus_space_tag = pa->pa_memt;
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, PCI_BAR(0),
		      memtype, &addr, &adapter->osdep.mem_size, &flags) != 0)
			goto map_err;
		if ((flags & BUS_SPACE_MAP_PREFETCHABLE) != 0) {
			aprint_normal_dev(dev, "clearing prefetchable bit\n");
			flags &= ~BUS_SPACE_MAP_PREFETCHABLE;
		}
		if (bus_space_map(adapter->osdep.mem_bus_space_tag, addr,
		     adapter->osdep.mem_size, flags,
		     &adapter->osdep.mem_bus_space_handle) != 0) {
map_err:
			adapter->osdep.mem_size = 0;
			aprint_error_dev(dev, "unable to map BAR0\n");
			return ENXIO;
		}
		/*
		 * Enable address decoding for memory range in case it's not
		 * set.
		 */
		csr = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_MEM_ENABLE;
		pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		    csr);
		break;
	default:
		aprint_error_dev(dev, "unexpected type on BAR0\n");
		return ENXIO;
	}

	/* Pick up the tuneable queues */
	adapter->num_queues = ixv_num_queues;

	return (0);
} /* ixv_allocate_pci_resources */

/************************************************************************
 * ixv_free_pci_resources
 ************************************************************************/
static void
ixv_free_pci_resources(struct adapter * adapter)
{
	struct		ix_queue *que = adapter->queues;
	int		rid;

	/*
	 *  Release all msix queue resources:
	 */
	for (int i = 0; i < adapter->num_queues; i++, que++) {
		if (que->res != NULL)
			pci_intr_disestablish(adapter->osdep.pc,
			    adapter->osdep.ihs[i]);
	}


	/* Clean the Mailbox interrupt last */
	rid = adapter->vector;

	if (adapter->osdep.ihs[rid] != NULL) {
		pci_intr_disestablish(adapter->osdep.pc,
		    adapter->osdep.ihs[rid]);
		adapter->osdep.ihs[rid] = NULL;
	}

	pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs,
	    adapter->osdep.nintrs);

	if (adapter->osdep.mem_size != 0) {
		bus_space_unmap(adapter->osdep.mem_bus_space_tag,
		    adapter->osdep.mem_bus_space_handle,
		    adapter->osdep.mem_size);
	}

	return;
} /* ixv_free_pci_resources */

/************************************************************************
 * ixv_setup_interface
 *
 *   Setup networking device structure and register an interface.
 ************************************************************************/
static int
ixv_setup_interface(device_t dev, struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ifnet   *ifp;
	int rv;

	INIT_DEBUGOUT("ixv_setup_interface: begin");

	ifp = adapter->ifp = &ec->ec_if;
	strlcpy(ifp->if_xname, device_xname(dev), IFNAMSIZ);
	ifp->if_baudrate = IF_Gbps(10);
	ifp->if_init = ixv_init;
	ifp->if_stop = ixv_ifstop;
	ifp->if_softc = adapter;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef IXGBE_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_ioctl = ixv_ioctl;
	if (adapter->feat_en & IXGBE_FEATURE_LEGACY_TX) {
#if 0
		ixv_start_locked = ixgbe_legacy_start_locked;
#endif
	} else {
		ifp->if_transmit = ixgbe_mq_start;
#if 0
		ixv_start_locked = ixgbe_mq_start_locked;
#endif
	}
	ifp->if_start = ixgbe_legacy_start;
	IFQ_SET_MAXLEN(&ifp->if_snd, adapter->num_tx_desc - 2);
	IFQ_SET_READY(&ifp->if_snd);

	rv = if_initialize(ifp);
	if (rv != 0) {
		aprint_error_dev(dev, "if_initialize failed(%d)\n", rv);
		return rv;
	}
	adapter->ipq = if_percpuq_create(&adapter->osdep.ec.ec_if);
	ether_ifattach(ifp, adapter->hw.mac.addr);
	aprint_normal_dev(dev, "Ethernet address %s\n",
	    ether_sprintf(adapter->hw.mac.addr));
	/*
	 * We use per TX queue softint, so if_deferred_start_init() isn't
	 * used.
	 */
	ether_set_ifflags_cb(ec, ixv_ifflags_cb);

	adapter->max_frame_size = ifp->if_mtu + IXGBE_MTU_HDR;

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);

	/* Set capability flags */
	ifp->if_capabilities |= IFCAP_HWCSUM
			     |	IFCAP_TSOv4
			     |	IFCAP_TSOv6;
	ifp->if_capenable = 0;

	ec->ec_capabilities |= ETHERCAP_VLAN_HWFILTER
			    |  ETHERCAP_VLAN_HWTAGGING
			    |  ETHERCAP_VLAN_HWCSUM
			    |  ETHERCAP_JUMBO_MTU
			    |  ETHERCAP_VLAN_MTU;

	/* Enable the above capabilities by default */
	ec->ec_capenable = ec->ec_capabilities;

	/* Don't enable LRO by default */
#if 0
	/* NetBSD doesn't support LRO yet */
	ifp->if_capabilities |= IFCAP_LRO;
#endif

	/*
	 * Specify the media types supported by this adapter and register
	 * callbacks to update media and link information
	 */
	ec->ec_ifmedia = &adapter->media;
	ifmedia_init(&adapter->media, IFM_IMASK, ixv_media_change,
	    ixv_media_status);
	ifmedia_add(&adapter->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);

	if_register(ifp);

	return 0;
} /* ixv_setup_interface */


/************************************************************************
 * ixv_initialize_transmit_units - Enable transmit unit.
 ************************************************************************/
static void
ixv_initialize_transmit_units(struct adapter *adapter)
{
	struct tx_ring	*txr = adapter->tx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	int i;

	for (i = 0; i < adapter->num_queues; i++, txr++) {
		u64 tdba = txr->txdma.dma_paddr;
		u32 txctrl, txdctl;
		int j = txr->me;

		/* Set WTHRESH to 8, burst writeback */
		txdctl = IXGBE_READ_REG(hw, IXGBE_VFTXDCTL(j));
		txdctl |= IXGBE_TX_WTHRESH << IXGBE_TXDCTL_WTHRESH_SHIFT;
		IXGBE_WRITE_REG(hw, IXGBE_VFTXDCTL(j), txdctl);

		/* Set the HW Tx Head and Tail indices */
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_VFTDH(j), 0);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_VFTDT(j), 0);

		/* Set Tx Tail register */
		txr->tail = IXGBE_VFTDT(j);

		txr->txr_no_space = false;

		/* Set Ring parameters */
		IXGBE_WRITE_REG(hw, IXGBE_VFTDBAL(j),
		    (tdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_VFTDBAH(j), (tdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_VFTDLEN(j),
		    adapter->num_tx_desc * sizeof(struct ixgbe_legacy_tx_desc));
		txctrl = IXGBE_READ_REG(hw, IXGBE_VFDCA_TXCTRL(j));
		txctrl &= ~IXGBE_DCA_TXCTRL_DESC_WRO_EN;
		IXGBE_WRITE_REG(hw, IXGBE_VFDCA_TXCTRL(j), txctrl);

		/* Now enable */
		txdctl = IXGBE_READ_REG(hw, IXGBE_VFTXDCTL(j));
		txdctl |= IXGBE_TXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_VFTXDCTL(j), txdctl);
	}

	return;
} /* ixv_initialize_transmit_units */


/************************************************************************
 * ixv_initialize_rss_mapping
 ************************************************************************/
static void
ixv_initialize_rss_mapping(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32		reta = 0, mrqc, rss_key[10];
	int		queue_id;
	int		i, j;
	u32		rss_hash_config;

	/* force use default RSS key. */
#ifdef __NetBSD__
	rss_getkey((uint8_t *) &rss_key);
#else
	if (adapter->feat_en & IXGBE_FEATURE_RSS) {
		/* Fetch the configured RSS key */
		rss_getkey((uint8_t *)&rss_key);
	} else {
		/* set up random bits */
		cprng_fast(&rss_key, sizeof(rss_key));
	}
#endif

	/* Now fill out hash function seeds */
	for (i = 0; i < 10; i++)
		IXGBE_WRITE_REG(hw, IXGBE_VFRSSRK(i), rss_key[i]);

	/* Set up the redirection table */
	for (i = 0, j = 0; i < 64; i++, j++) {
		if (j == adapter->num_queues)
			j = 0;

		if (adapter->feat_en & IXGBE_FEATURE_RSS) {
			/*
			 * Fetch the RSS bucket id for the given indirection
			 * entry. Cap it at the number of configured buckets
			 * (which is num_queues.)
			 */
			queue_id = rss_get_indirection_to_bucket(i);
			queue_id = queue_id % adapter->num_queues;
		} else
			queue_id = j;

		/*
		 * The low 8 bits are for hash value (n+0);
		 * The next 8 bits are for hash value (n+1), etc.
		 */
		reta >>= 8;
		reta |= ((uint32_t)queue_id) << 24;
		if ((i & 3) == 3) {
			IXGBE_WRITE_REG(hw, IXGBE_VFRETA(i >> 2), reta);
			reta = 0;
		}
	}

	/* Perform hash on these packet types */
	if (adapter->feat_en & IXGBE_FEATURE_RSS)
		rss_hash_config = rss_gethashconfig();
	else {
		/*
		 * Disable UDP - IP fragments aren't currently being handled
		 * and so we end up with a mix of 2-tuple and 4-tuple
		 * traffic.
		 */
		rss_hash_config = RSS_HASHTYPE_RSS_IPV4
				| RSS_HASHTYPE_RSS_TCP_IPV4
				| RSS_HASHTYPE_RSS_IPV6
				| RSS_HASHTYPE_RSS_TCP_IPV6;
	}

	mrqc = IXGBE_MRQC_RSSEN;
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV6_EX)
		device_printf(adapter->dev, "%s: RSS_HASHTYPE_RSS_IPV6_EX defined, but not supported\n",
		    __func__);
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV6_EX)
		device_printf(adapter->dev, "%s: RSS_HASHTYPE_RSS_TCP_IPV6_EX defined, but not supported\n",
		    __func__);
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6_EX)
		device_printf(adapter->dev, "%s: RSS_HASHTYPE_RSS_UDP_IPV6_EX defined, but not supported\n",
		    __func__);
	IXGBE_WRITE_REG(hw, IXGBE_VFMRQC, mrqc);
} /* ixv_initialize_rss_mapping */


/************************************************************************
 * ixv_initialize_receive_units - Setup receive registers and features.
 ************************************************************************/
static void
ixv_initialize_receive_units(struct adapter *adapter)
{
	struct	rx_ring	*rxr = adapter->rx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet	*ifp = adapter->ifp;
	u32		bufsz, psrtype;

	if (ifp->if_mtu > ETHERMTU)
		bufsz = 4096 >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;
	else
		bufsz = 2048 >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;

	psrtype = IXGBE_PSRTYPE_TCPHDR
		| IXGBE_PSRTYPE_UDPHDR
		| IXGBE_PSRTYPE_IPV4HDR
		| IXGBE_PSRTYPE_IPV6HDR
		| IXGBE_PSRTYPE_L2HDR;

	if (adapter->num_queues > 1)
		psrtype |= 1 << 29;

	IXGBE_WRITE_REG(hw, IXGBE_VFPSRTYPE, psrtype);

	/* Tell PF our max_frame size */
	if (ixgbevf_rlpml_set_vf(hw, adapter->max_frame_size) != 0) {
		device_printf(adapter->dev, "There is a problem with the PF setup.  It is likely the receive unit for this VF will not function correctly.\n");
	}

	for (int i = 0; i < adapter->num_queues; i++, rxr++) {
		u64 rdba = rxr->rxdma.dma_paddr;
		u32 reg, rxdctl;
		int j = rxr->me;

		/* Disable the queue */
		rxdctl = IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(j));
		rxdctl &= ~IXGBE_RXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(j), rxdctl);
		for (int k = 0; k < 10; k++) {
			if (IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(j)) &
			    IXGBE_RXDCTL_ENABLE)
				msec_delay(1);
			else
				break;
		}
		IXGBE_WRITE_BARRIER(hw);
		/* Setup the Base and Length of the Rx Descriptor Ring */
		IXGBE_WRITE_REG(hw, IXGBE_VFRDBAL(j),
		    (rdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_VFRDBAH(j), (rdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_VFRDLEN(j),
		    adapter->num_rx_desc * sizeof(union ixgbe_adv_rx_desc));

		/* Reset the ring indices */
		IXGBE_WRITE_REG(hw, IXGBE_VFRDH(rxr->me), 0);
		IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me), 0);

		/* Set up the SRRCTL register */
		reg = IXGBE_READ_REG(hw, IXGBE_VFSRRCTL(j));
		reg &= ~IXGBE_SRRCTL_BSIZEHDR_MASK;
		reg &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;
		reg |= bufsz;
		reg |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;
		IXGBE_WRITE_REG(hw, IXGBE_VFSRRCTL(j), reg);

		/* Capture Rx Tail index */
		rxr->tail = IXGBE_VFRDT(rxr->me);

		/* Do the queue enabling last */
		rxdctl |= IXGBE_RXDCTL_ENABLE | IXGBE_RXDCTL_VME;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(j), rxdctl);
		for (int k = 0; k < 10; k++) {
			if (IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(j)) &
			    IXGBE_RXDCTL_ENABLE)
				break;
			msec_delay(1);
		}
		IXGBE_WRITE_BARRIER(hw);

		/* Set the Tail Pointer */
#ifdef DEV_NETMAP
		/*
		 * In netmap mode, we must preserve the buffers made
		 * available to userspace before the if_init()
		 * (this is true by default on the TX side, because
		 * init makes all buffers available to userspace).
		 *
		 * netmap_reset() and the device specific routines
		 * (e.g. ixgbe_setup_receive_rings()) map these
		 * buffers at the end of the NIC ring, so here we
		 * must set the RDT (tail) register to make sure
		 * they are not overwritten.
		 *
		 * In this driver the NIC ring starts at RDH = 0,
		 * RDT points to the last slot available for reception (?),
		 * so RDT = num_rx_desc - 1 means the whole ring is available.
		 */
		if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) &&
		    (ifp->if_capenable & IFCAP_NETMAP)) {
			struct netmap_adapter *na = NA(adapter->ifp);
			struct netmap_kring *kring = na->rx_rings[i];
			int t = na->num_rx_desc - 1 - nm_kr_rxspace(kring);

			IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me), t);
		} else
#endif /* DEV_NETMAP */
			IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me),
			    adapter->num_rx_desc - 1);
	}

	if (adapter->hw.mac.type >= ixgbe_mac_X550_vf)
		ixv_initialize_rss_mapping(adapter);
} /* ixv_initialize_receive_units */

/************************************************************************
 * ixv_sysctl_tdh_handler - Transmit Descriptor Head handler function
 *
 *   Retrieves the TDH value from the hardware
 ************************************************************************/
static int
ixv_sysctl_tdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct tx_ring *txr = (struct tx_ring *)node.sysctl_data;
	uint32_t val;

	if (!txr)
		return (0);

	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_VFTDH(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_tdh_handler */

/************************************************************************
 * ixgbe_sysctl_tdt_handler - Transmit Descriptor Tail handler function
 *
 *   Retrieves the TDT value from the hardware
 ************************************************************************/
static int
ixv_sysctl_tdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct tx_ring *txr = (struct tx_ring *)node.sysctl_data;
	uint32_t val;

	if (!txr)
		return (0);

	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_VFTDT(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_tdt_handler */

/************************************************************************
 * ixv_sysctl_next_to_check_handler - Receive Descriptor next to check
 * handler function
 *
 *   Retrieves the next_to_check value
 ************************************************************************/
static int
ixv_sysctl_next_to_check_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	uint32_t val;

	if (!rxr)
		return (0);

	val = rxr->next_to_check;
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_next_to_check_handler */

/************************************************************************
 * ixv_sysctl_next_to_refresh_handler - Receive Descriptor next to refresh
 * handler function
 *
 *   Retrieves the next_to_refresh value
 ************************************************************************/
static int
ixv_sysctl_next_to_refresh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	struct adapter *adapter;
	uint32_t val;

	if (!rxr)
		return (0);

	adapter = rxr->adapter;
	if (ixgbe_fw_recovery_mode_swflag(adapter))
		return (EPERM);

	val = rxr->next_to_refresh;
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_next_to_refresh_handler */

/************************************************************************
 * ixv_sysctl_rdh_handler - Receive Descriptor Head handler function
 *
 *   Retrieves the RDH value from the hardware
 ************************************************************************/
static int
ixv_sysctl_rdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	uint32_t val;

	if (!rxr)
		return (0);

	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_VFRDH(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_rdh_handler */

/************************************************************************
 * ixv_sysctl_rdt_handler - Receive Descriptor Tail handler function
 *
 *   Retrieves the RDT value from the hardware
 ************************************************************************/
static int
ixv_sysctl_rdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	uint32_t val;

	if (!rxr)
		return (0);

	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_VFRDT(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixv_sysctl_rdt_handler */

static void
ixv_setup_vlan_tagging(struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	struct rx_ring	*rxr;
	u32		ctrl;
	int		i;
	bool		hwtagging;

	/* Enable HW tagging only if any vlan is attached */
	hwtagging = (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING)
	    && VLAN_ATTACHED(ec);

	/* Enable the queues */
	for (i = 0; i < adapter->num_queues; i++) {
		rxr = &adapter->rx_rings[i];
		ctrl = IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(rxr->me));
		if (hwtagging)
			ctrl |= IXGBE_RXDCTL_VME;
		else
			ctrl &= ~IXGBE_RXDCTL_VME;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(rxr->me), ctrl);
		/*
		 * Let Rx path know that it needs to store VLAN tag
		 * as part of extra mbuf info.
		 */
		rxr->vtag_strip = hwtagging ? TRUE : FALSE;
	}
} /* ixv_setup_vlan_tagging */

/************************************************************************
 * ixv_setup_vlan_support
 ************************************************************************/
static int
ixv_setup_vlan_support(struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		vid, vfta, retry;
	struct vlanid_list *vlanidp;
	int rv, error = 0;

	/*
	 *  This function is called from both if_init and ifflags_cb()
	 * on NetBSD.
	 */

	/*
	 * Part 1:
	 * Setup VLAN HW tagging
	 */
	ixv_setup_vlan_tagging(adapter);

	if (!VLAN_ATTACHED(ec))
		return 0;

	/*
	 * Part 2:
	 * Setup VLAN HW filter
	 */
	/* Cleanup shadow_vfta */
	for (int i = 0; i < IXGBE_VFTA_SIZE; i++)
		adapter->shadow_vfta[i] = 0;
	/* Generate shadow_vfta from ec_vids */
	ETHER_LOCK(ec);
	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list) {
		uint32_t idx;

		idx = vlanidp->vid / 32;
		KASSERT(idx < IXGBE_VFTA_SIZE);
		adapter->shadow_vfta[idx] |= (u32)1 << (vlanidp->vid % 32);
	}
	ETHER_UNLOCK(ec);

	/*
	 * A soft reset zero's out the VFTA, so
	 * we need to repopulate it now.
	 */
	for (int i = 0; i < IXGBE_VFTA_SIZE; i++) {
		if (adapter->shadow_vfta[i] == 0)
			continue;
		vfta = adapter->shadow_vfta[i];
		/*
		 * Reconstruct the vlan id's
		 * based on the bits set in each
		 * of the array ints.
		 */
		for (int j = 0; j < 32; j++) {
			retry = 0;
			if ((vfta & ((u32)1 << j)) == 0)
				continue;
			vid = (i * 32) + j;

			/* Call the shared code mailbox routine */
			while ((rv = hw->mac.ops.set_vfta(hw, vid, 0, TRUE,
			    FALSE)) != 0) {
				if (++retry > 5) {
					device_printf(adapter->dev,
					    "%s: max retry exceeded\n",
						__func__);
					break;
				}
			}
			if (rv != 0) {
				device_printf(adapter->dev,
				    "failed to set vlan %d\n", vid);
				error = EACCES;
			}
		}
	}
	return error;
} /* ixv_setup_vlan_support */

static int
ixv_vlan_cb(struct ethercom *ec, uint16_t vid, bool set)
{
	struct ifnet *ifp = &ec->ec_if;
	struct adapter *adapter = ifp->if_softc;
	int rv;

	if (set)
		rv = ixv_register_vlan(adapter, vid);
	else
		rv = ixv_unregister_vlan(adapter, vid);

	if (rv != 0)
		return rv;

	/*
	 * Control VLAN HW tagging when ec_nvlan is changed from 1 to 0
	 * or 0 to 1.
	 */
	if ((set && (ec->ec_nvlans == 1)) || (!set && (ec->ec_nvlans == 0)))
		ixv_setup_vlan_tagging(adapter);

	return rv;
}

/************************************************************************
 * ixv_register_vlan
 *
 *   Run via a vlan config EVENT, it enables us to use the
 *   HW Filter table since we can get the vlan id. This just
 *   creates the entry in the soft version of the VFTA, init
 *   will repopulate the real table.
 ************************************************************************/
static int
ixv_register_vlan(struct adapter *adapter, u16 vtag)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u16		index, bit;
	int error;

	if ((vtag == 0) || (vtag > 4095)) /* Invalid */
		return EINVAL;
	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	adapter->shadow_vfta[index] |= ((u32)1 << bit);
	error = hw->mac.ops.set_vfta(hw, vtag, 0, true, false);
	IXGBE_CORE_UNLOCK(adapter);

	if (error != 0) {
		device_printf(adapter->dev, "failed to register vlan %hu\n",
		    vtag);
		error = EACCES;
	}
	return error;
} /* ixv_register_vlan */

/************************************************************************
 * ixv_unregister_vlan
 *
 *   Run via a vlan unconfig EVENT, remove our entry
 *   in the soft vfta.
 ************************************************************************/
static int
ixv_unregister_vlan(struct adapter *adapter, u16 vtag)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u16		index, bit;
	int		error;

	if ((vtag == 0) || (vtag > 4095))  /* Invalid */
		return EINVAL;

	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	adapter->shadow_vfta[index] &= ~((u32)1 << bit);
	error = hw->mac.ops.set_vfta(hw, vtag, 0, false, false);
	IXGBE_CORE_UNLOCK(adapter);

	if (error != 0) {
		device_printf(adapter->dev, "failed to unregister vlan %hu\n",
		    vtag);
		error = EIO;
	}
	return error;
} /* ixv_unregister_vlan */

/************************************************************************
 * ixv_enable_intr
 ************************************************************************/
static void
ixv_enable_intr(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = adapter->queues;
	u32		mask;
	int i;

	/* For VTEIAC */
	mask = (1 << adapter->vector);
	for (i = 0; i < adapter->num_queues; i++, que++)
		mask |= (1 << que->msix);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIAC, mask);

	/* For VTEIMS */
	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, (1 << adapter->vector));
	que = adapter->queues;
	for (i = 0; i < adapter->num_queues; i++, que++)
		ixv_enable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(hw);
} /* ixv_enable_intr */

/************************************************************************
 * ixv_disable_intr
 ************************************************************************/
static void
ixv_disable_intr(struct adapter *adapter)
{
	struct ix_queue	*que = adapter->queues;

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEIAC, 0);

	/* disable interrupts other than queues */
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEIMC, adapter->vector);

	for (int i = 0; i < adapter->num_queues; i++, que++)
		ixv_disable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(&adapter->hw);
} /* ixv_disable_intr */

/************************************************************************
 * ixv_set_ivar
 *
 *   Setup the correct IVAR register for a particular MSI-X interrupt
 *    - entry is the register array entry
 *    - vector is the MSI-X vector for this queue
 *    - type is RX/TX/MISC
 ************************************************************************/
static void
ixv_set_ivar(struct adapter *adapter, u8 entry, u8 vector, s8 type)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32		ivar, index;

	vector |= IXGBE_IVAR_ALLOC_VAL;

	if (type == -1) { /* MISC IVAR */
		ivar = IXGBE_READ_REG(hw, IXGBE_VTIVAR_MISC);
		ivar &= ~0xFF;
		ivar |= vector;
		IXGBE_WRITE_REG(hw, IXGBE_VTIVAR_MISC, ivar);
	} else {	  /* RX/TX IVARS */
		index = (16 * (entry & 1)) + (8 * type);
		ivar = IXGBE_READ_REG(hw, IXGBE_VTIVAR(entry >> 1));
		ivar &= ~(0xffUL << index);
		ivar |= ((u32)vector << index);
		IXGBE_WRITE_REG(hw, IXGBE_VTIVAR(entry >> 1), ivar);
	}
} /* ixv_set_ivar */

/************************************************************************
 * ixv_configure_ivars
 ************************************************************************/
static void
ixv_configure_ivars(struct adapter *adapter)
{
	struct ix_queue *que = adapter->queues;

	/* XXX We should sync EITR value calculation with ixgbe.c? */

	for (int i = 0; i < adapter->num_queues; i++, que++) {
		/* First the RX queue entry */
		ixv_set_ivar(adapter, i, que->msix, 0);
		/* ... and the TX */
		ixv_set_ivar(adapter, i, que->msix, 1);
		/* Set an initial value in EITR */
		ixv_eitr_write(adapter, que->msix, IXGBE_EITR_DEFAULT);
	}

	/* For the mailbox interrupt */
	ixv_set_ivar(adapter, 1, adapter->vector, -1);
} /* ixv_configure_ivars */


/************************************************************************
 * ixv_init_stats
 *
 *   The VF stats registers never have a truly virgin
 *   starting point, so this routine save initial vaules to
 *   last_<REGNAME>.
 ************************************************************************/
static void
ixv_init_stats(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;

	adapter->stats.vf.last_vfgprc = IXGBE_READ_REG(hw, IXGBE_VFGPRC);
	adapter->stats.vf.last_vfgorc = IXGBE_READ_REG(hw, IXGBE_VFGORC_LSB);
	adapter->stats.vf.last_vfgorc |=
	    (((u64)(IXGBE_READ_REG(hw, IXGBE_VFGORC_MSB))) << 32);

	adapter->stats.vf.last_vfgptc = IXGBE_READ_REG(hw, IXGBE_VFGPTC);
	adapter->stats.vf.last_vfgotc = IXGBE_READ_REG(hw, IXGBE_VFGOTC_LSB);
	adapter->stats.vf.last_vfgotc |=
	    (((u64)(IXGBE_READ_REG(hw, IXGBE_VFGOTC_MSB))) << 32);

	adapter->stats.vf.last_vfmprc = IXGBE_READ_REG(hw, IXGBE_VFMPRC);
} /* ixv_init_stats */

#define UPDATE_STAT_32(reg, last, count)		\
{							\
	u32 current = IXGBE_READ_REG(hw, (reg));	\
	count.ev_count += current - last;		\
	(last) = current;				\
}

#define UPDATE_STAT_36(lsb, msb, last, count)		   	\
{						   		\
	u64 cur_lsb = IXGBE_READ_REG(hw, (lsb));		\
	u64 cur_msb = IXGBE_READ_REG(hw, (msb));		\
	u64 current = ((cur_msb << 32) | cur_lsb);		\
	if (current < (last))					\
		count.ev_count += current + __BIT(36) - (last);	\
	else							\
		count.ev_count += current - (last);		\
	(last) = current;					\
}

/************************************************************************
 * ixv_update_stats - Update the board statistics counters.
 ************************************************************************/
void
ixv_update_stats(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;

	UPDATE_STAT_32(IXGBE_VFGPRC, stats->last_vfgprc, stats->vfgprc);
	UPDATE_STAT_32(IXGBE_VFGPTC, stats->last_vfgptc, stats->vfgptc);
	UPDATE_STAT_36(IXGBE_VFGORC_LSB, IXGBE_VFGORC_MSB, stats->last_vfgorc,
	    stats->vfgorc);
	UPDATE_STAT_36(IXGBE_VFGOTC_LSB, IXGBE_VFGOTC_MSB, stats->last_vfgotc,
	    stats->vfgotc);
	UPDATE_STAT_32(IXGBE_VFMPRC, stats->last_vfmprc, stats->vfmprc);

	/* VF doesn't count errors by hardware */

} /* ixv_update_stats */

/************************************************************************
 * ixv_sysctl_interrupt_rate_handler
 ************************************************************************/
static int
ixv_sysctl_interrupt_rate_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ix_queue *que = (struct ix_queue *)node.sysctl_data;
	struct adapter	*adapter = que->adapter;
	uint32_t reg, usec, rate;
	int error;

	if (que == NULL)
		return 0;
	reg = IXGBE_READ_REG(&que->adapter->hw, IXGBE_VTEITR(que->msix));
	usec = ((reg & 0x0FF8) >> 3);
	if (usec > 0)
		rate = 500000 / usec;
	else
		rate = 0;
	node.sysctl_data = &rate;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	reg &= ~0xfff; /* default, no limitation */
	if (rate > 0 && rate < 500000) {
		if (rate < 1000)
			rate = 1000;
		reg |= ((4000000 / rate) & 0xff8);
		/*
		 * When RSC is used, ITR interval must be larger than
		 * RSC_DELAY. Currently, we use 2us for RSC_DELAY.
		 * The minimum value is always greater than 2us on 100M
		 * (and 10M?(not documented)), but it's not on 1G and higher.
		 */
		if ((adapter->link_speed != IXGBE_LINK_SPEED_100_FULL)
		    && (adapter->link_speed != IXGBE_LINK_SPEED_10_FULL)) {
			if ((adapter->num_queues > 1)
			    && (reg < IXGBE_MIN_RSC_EITR_10G1G))
				return EINVAL;
		}
		ixv_max_interrupt_rate = rate;
	} else
		ixv_max_interrupt_rate = 0;
	ixv_eitr_write(adapter, que->msix, reg);

	return (0);
} /* ixv_sysctl_interrupt_rate_handler */

const struct sysctlnode *
ixv_sysctl_instance(struct adapter *adapter)
{
	const char *dvname;
	struct sysctllog **log;
	int rc;
	const struct sysctlnode *rnode;

	log = &adapter->sysctllog;
	dvname = device_xname(adapter->dev);

	if ((rc = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, dvname,
	    SYSCTL_DESCR("ixv information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return rnode;
err:
	device_printf(adapter->dev,
	    "%s: sysctl_createv failed, rc = %d\n", __func__, rc);
	return NULL;
}

static void
ixv_add_device_sysctls(struct adapter *adapter)
{
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;
	device_t dev;

	dev = adapter->dev;
	log = &adapter->sysctllog;

	if ((rnode = ixv_sysctl_instance(adapter)) == NULL) {
		aprint_error_dev(dev, "could not create sysctl root\n");
		return;
	}

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Debug Info"),
	    ixv_sysctl_debug, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "rx_copy_len", SYSCTL_DESCR("RX Copy Length"),
	    ixv_sysctl_rx_copy_len, 0,
	    (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL,
	    "enable_aim", SYSCTL_DESCR("Interrupt Moderation"),
	    NULL, 0, &adapter->enable_aim, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL,
	    "txrx_workqueue", SYSCTL_DESCR("Use workqueue for packet processing"),
		NULL, 0, &adapter->txrx_use_workqueue, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
}

/************************************************************************
 * ixv_add_stats_sysctls - Add statistic sysctls for the VF.
 ************************************************************************/
static void
ixv_add_stats_sysctls(struct adapter *adapter)
{
	device_t		dev = adapter->dev;
	struct tx_ring		*txr = adapter->tx_rings;
	struct rx_ring		*rxr = adapter->rx_rings;
	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;
	struct ixgbe_hw *hw = &adapter->hw;
	const struct sysctlnode *rnode, *cnode;
	struct sysctllog **log = &adapter->sysctllog;
	const char *xname = device_xname(dev);

	/* Driver Statistics */
	evcnt_attach_dynamic(&adapter->efbig_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma soft fail EFBIG");
	evcnt_attach_dynamic(&adapter->mbuf_defrag_failed, EVCNT_TYPE_MISC,
	    NULL, xname, "m_defrag() failed");
	evcnt_attach_dynamic(&adapter->efbig2_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma hard fail EFBIG");
	evcnt_attach_dynamic(&adapter->einval_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma hard fail EINVAL");
	evcnt_attach_dynamic(&adapter->other_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma hard fail other");
	evcnt_attach_dynamic(&adapter->eagain_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma soft fail EAGAIN");
	evcnt_attach_dynamic(&adapter->enomem_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, xname, "Driver tx dma soft fail ENOMEM");
	evcnt_attach_dynamic(&adapter->watchdog_events, EVCNT_TYPE_MISC,
	    NULL, xname, "Watchdog timeouts");
	evcnt_attach_dynamic(&adapter->tso_err, EVCNT_TYPE_MISC,
	    NULL, xname, "TSO errors");
	evcnt_attach_dynamic(&adapter->link_irq, EVCNT_TYPE_INTR,
	    NULL, xname, "Link MSI-X IRQ Handled");

	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		snprintf(adapter->queues[i].evnamebuf,
		    sizeof(adapter->queues[i].evnamebuf), "%s q%d",
		    xname, i);
		snprintf(adapter->queues[i].namebuf,
		    sizeof(adapter->queues[i].namebuf), "q%d", i);

		if ((rnode = ixv_sysctl_instance(adapter)) == NULL) {
			aprint_error_dev(dev, "could not create sysctl root\n");
			break;
		}

		if (sysctl_createv(log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE,
		    adapter->queues[i].namebuf, SYSCTL_DESCR("Queue Name"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "interrupt_rate", SYSCTL_DESCR("Interrupt Rate"),
		    ixv_sysctl_interrupt_rate_handler, 0,
		    (void *)&adapter->queues[i], 0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "txd_head", SYSCTL_DESCR("Transmit Descriptor Head"),
		    ixv_sysctl_tdh_handler, 0, (void *)txr,
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "txd_tail", SYSCTL_DESCR("Transmit Descriptor Tail"),
		    ixv_sysctl_tdt_handler, 0, (void *)txr,
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;

		evcnt_attach_dynamic(&adapter->queues[i].irqs, EVCNT_TYPE_INTR,
		    NULL, adapter->queues[i].evnamebuf, "IRQs on queue");
		evcnt_attach_dynamic(&adapter->queues[i].handleq,
		    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
		    "Handled queue in softint");
		evcnt_attach_dynamic(&adapter->queues[i].req, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Requeued in softint");
		evcnt_attach_dynamic(&txr->tso_tx, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "TSO");
		evcnt_attach_dynamic(&txr->no_desc_avail, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "TX Queue No Descriptor Available");
		evcnt_attach_dynamic(&txr->total_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "Queue Packets Transmitted");
#ifndef IXGBE_LEGACY_TX
		evcnt_attach_dynamic(&txr->pcq_drops, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "Packets dropped in pcq");
#endif

#ifdef LRO
		struct lro_ctrl *lro = &rxr->lro;
#endif /* LRO */

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY,
		    CTLTYPE_INT,
		    "rxd_nxck", SYSCTL_DESCR("Receive Descriptor next to check"),
			ixv_sysctl_next_to_check_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT, "rxd_nxrf",
		    SYSCTL_DESCR("Receive Descriptor next to refresh"),
		    ixv_sysctl_next_to_refresh_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY,
		    CTLTYPE_INT,
		    "rxd_head", SYSCTL_DESCR("Receive Descriptor Head"),
		    ixv_sysctl_rdh_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY,
		    CTLTYPE_INT,
		    "rxd_tail", SYSCTL_DESCR("Receive Descriptor Tail"),
		    ixv_sysctl_rdt_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;

		evcnt_attach_dynamic(&rxr->rx_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Packets Received");
		evcnt_attach_dynamic(&rxr->rx_bytes, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Bytes Received");
		evcnt_attach_dynamic(&rxr->rx_copies, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Copied RX Frames");
		evcnt_attach_dynamic(&rxr->no_mbuf, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx no mbuf");
		evcnt_attach_dynamic(&rxr->rx_discarded, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx discarded");
#ifdef LRO
		SYSCTL_ADD_INT(ctx, queue_list, OID_AUTO, "lro_queued",
				CTLFLAG_RD, &lro->lro_queued, 0,
				"LRO Queued");
		SYSCTL_ADD_INT(ctx, queue_list, OID_AUTO, "lro_flushed",
				CTLFLAG_RD, &lro->lro_flushed, 0,
				"LRO Flushed");
#endif /* LRO */
	}

	/* MAC stats get their own sub node */

	snprintf(stats->namebuf,
	    sizeof(stats->namebuf), "%s MAC Statistics", xname);

	evcnt_attach_dynamic(&stats->ipcs, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - IP");
	evcnt_attach_dynamic(&stats->l4cs, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - L4");
	evcnt_attach_dynamic(&stats->ipcs_bad, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - IP bad");
	evcnt_attach_dynamic(&stats->l4cs_bad, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - L4 bad");

	/* Packet Reception Stats */
	evcnt_attach_dynamic(&stats->vfgprc, EVCNT_TYPE_MISC, NULL,
	    xname, "Good Packets Received");
	evcnt_attach_dynamic(&stats->vfgorc, EVCNT_TYPE_MISC, NULL,
	    xname, "Good Octets Received");
	evcnt_attach_dynamic(&stats->vfmprc, EVCNT_TYPE_MISC, NULL,
	    xname, "Multicast Packets Received");
	evcnt_attach_dynamic(&stats->vfgptc, EVCNT_TYPE_MISC, NULL,
	    xname, "Good Packets Transmitted");
	evcnt_attach_dynamic(&stats->vfgotc, EVCNT_TYPE_MISC, NULL,
	    xname, "Good Octets Transmitted");

	/* Mailbox Stats */
	evcnt_attach_dynamic(&hw->mbx.stats.msgs_tx, EVCNT_TYPE_MISC, NULL,
	    xname, "message TXs");
	evcnt_attach_dynamic(&hw->mbx.stats.msgs_rx, EVCNT_TYPE_MISC, NULL,
	    xname, "message RXs");
	evcnt_attach_dynamic(&hw->mbx.stats.acks, EVCNT_TYPE_MISC, NULL,
	    xname, "ACKs");
	evcnt_attach_dynamic(&hw->mbx.stats.reqs, EVCNT_TYPE_MISC, NULL,
	    xname, "REQs");
	evcnt_attach_dynamic(&hw->mbx.stats.rsts, EVCNT_TYPE_MISC, NULL,
	    xname, "RSTs");

} /* ixv_add_stats_sysctls */

static void
ixv_clear_evcnt(struct adapter *adapter)
{
	struct tx_ring		*txr = adapter->tx_rings;
	struct rx_ring		*rxr = adapter->rx_rings;
	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;
	struct ixgbe_hw *hw = &adapter->hw;
	int i;

	/* Driver Statistics */
	adapter->efbig_tx_dma_setup.ev_count = 0;
	adapter->mbuf_defrag_failed.ev_count = 0;
	adapter->efbig2_tx_dma_setup.ev_count = 0;
	adapter->einval_tx_dma_setup.ev_count = 0;
	adapter->other_tx_dma_setup.ev_count = 0;
	adapter->eagain_tx_dma_setup.ev_count = 0;
	adapter->enomem_tx_dma_setup.ev_count = 0;
	adapter->watchdog_events.ev_count = 0;
	adapter->tso_err.ev_count = 0;
	adapter->link_irq.ev_count = 0;

	for (i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		adapter->queues[i].irqs.ev_count = 0;
		adapter->queues[i].handleq.ev_count = 0;
		adapter->queues[i].req.ev_count = 0;
		txr->tso_tx.ev_count = 0;
		txr->no_desc_avail.ev_count = 0;
		txr->total_packets.ev_count = 0;
#ifndef IXGBE_LEGACY_TX
		txr->pcq_drops.ev_count = 0;
#endif
		txr->q_efbig_tx_dma_setup = 0;
		txr->q_mbuf_defrag_failed = 0;
		txr->q_efbig2_tx_dma_setup = 0;
		txr->q_einval_tx_dma_setup = 0;
		txr->q_other_tx_dma_setup = 0;
		txr->q_eagain_tx_dma_setup = 0;
		txr->q_enomem_tx_dma_setup = 0;
		txr->q_tso_err = 0;

		rxr->rx_packets.ev_count = 0;
		rxr->rx_bytes.ev_count = 0;
		rxr->rx_copies.ev_count = 0;
		rxr->no_mbuf.ev_count = 0;
		rxr->rx_discarded.ev_count = 0;
	}

	/* MAC stats get their own sub node */

	stats->ipcs.ev_count = 0;
	stats->l4cs.ev_count = 0;
	stats->ipcs_bad.ev_count = 0;
	stats->l4cs_bad.ev_count = 0;

	/*
	 * Packet Reception Stats.
	 * Call ixv_init_stats() to save last VF counters' values.
	 */
	ixv_init_stats(adapter);
	stats->vfgprc.ev_count = 0;
	stats->vfgorc.ev_count = 0;
	stats->vfmprc.ev_count = 0;
	stats->vfgptc.ev_count = 0;
	stats->vfgotc.ev_count = 0;

	/* Mailbox Stats */
	hw->mbx.stats.msgs_tx.ev_count = 0;
	hw->mbx.stats.msgs_rx.ev_count = 0;
	hw->mbx.stats.acks.ev_count = 0;
	hw->mbx.stats.reqs.ev_count = 0;
	hw->mbx.stats.rsts.ev_count = 0;

} /* ixv_clear_evcnt */

/************************************************************************
 * ixv_set_sysctl_value
 ************************************************************************/
static void
ixv_set_sysctl_value(struct adapter *adapter, const char *name,
	const char *description, int *limit, int value)
{
	device_t dev =	adapter->dev;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;

	log = &adapter->sysctllog;
	if ((rnode = ixv_sysctl_instance(adapter)) == NULL) {
		aprint_error_dev(dev, "could not create sysctl root\n");
		return;
	}
	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    name, SYSCTL_DESCR(description),
	    NULL, 0, limit, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
	*limit = value;
} /* ixv_set_sysctl_value */

/************************************************************************
 * ixv_print_debug_info
 *
 *   Called only when em_display_debug_stats is enabled.
 *   Provides a way to take a look at important statistics
 *   maintained by the driver and hardware.
 ************************************************************************/
static void
ixv_print_debug_info(struct adapter *adapter)
{
	device_t	dev = adapter->dev;
	struct ix_queue *que = adapter->queues;
	struct rx_ring	*rxr;
	struct tx_ring	*txr;
#ifdef LRO
	struct lro_ctrl *lro;
#endif /* LRO */

	for (int i = 0; i < adapter->num_queues; i++, que++) {
		txr = que->txr;
		rxr = que->rxr;
#ifdef LRO
		lro = &rxr->lro;
#endif /* LRO */
		device_printf(dev, "QUE(%d) IRQs Handled: %lu\n",
		    que->msix, (long)que->irqs.ev_count);
		device_printf(dev, "RX(%d) Packets Received: %lld\n",
		    rxr->me, (long long)rxr->rx_packets.ev_count);
		device_printf(dev, "RX(%d) Bytes Received: %lu\n",
		    rxr->me, (long)rxr->rx_bytes.ev_count);
#ifdef LRO
		device_printf(dev, "RX(%d) LRO Queued= %ju\n",
		    rxr->me, (uintmax_t)lro->lro_queued);
		device_printf(dev, "RX(%d) LRO Flushed= %ju\n",
		    rxr->me, (uintmax_t)lro->lro_flushed);
#endif /* LRO */
		device_printf(dev, "TX(%d) Packets Sent: %lu\n",
		    txr->me, (long)txr->total_packets.ev_count);
		device_printf(dev, "TX(%d) NO Desc Avail: %lu\n",
		    txr->me, (long)txr->no_desc_avail.ev_count);
	}

	device_printf(dev, "MBX IRQ Handled: %lu\n",
	    (long)adapter->link_irq.ev_count);
} /* ixv_print_debug_info */

/************************************************************************
 * ixv_sysctl_debug
 ************************************************************************/
static int
ixv_sysctl_debug(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	int	       error, result;

	node.sysctl_data = &result;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	if (result == 1)
		ixv_print_debug_info(adapter);

	return 0;
} /* ixv_sysctl_debug */

/************************************************************************
 * ixv_sysctl_rx_copy_len
 ************************************************************************/
static int
ixv_sysctl_rx_copy_len(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	int error;
	int result = adapter->rx_copy_len;

	node.sysctl_data = &result;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	if ((result < 0) || (result > IXGBE_RX_COPY_LEN_MAX))
		return EINVAL;

	adapter->rx_copy_len = result;

	return 0;
} /* ixgbe_sysctl_rx_copy_len */

/************************************************************************
 * ixv_init_device_features
 ************************************************************************/
static void
ixv_init_device_features(struct adapter *adapter)
{
	adapter->feat_cap = IXGBE_FEATURE_NETMAP
			  | IXGBE_FEATURE_VF
			  | IXGBE_FEATURE_RSS
			  | IXGBE_FEATURE_LEGACY_TX;

	/* A tad short on feature flags for VFs, atm. */
	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82599_vf:
		break;
	case ixgbe_mac_X540_vf:
		break;
	case ixgbe_mac_X550_vf:
	case ixgbe_mac_X550EM_x_vf:
	case ixgbe_mac_X550EM_a_vf:
		adapter->feat_cap |= IXGBE_FEATURE_NEEDS_CTXD;
		break;
	default:
		break;
	}

	/* Enabled by default... */
	/* Is a virtual function (VF) */
	if (adapter->feat_cap & IXGBE_FEATURE_VF)
		adapter->feat_en |= IXGBE_FEATURE_VF;
	/* Netmap */
	if (adapter->feat_cap & IXGBE_FEATURE_NETMAP)
		adapter->feat_en |= IXGBE_FEATURE_NETMAP;
	/* Receive-Side Scaling (RSS) */
	if (adapter->feat_cap & IXGBE_FEATURE_RSS)
		adapter->feat_en |= IXGBE_FEATURE_RSS;
	/* Needs advanced context descriptor regardless of offloads req'd */
	if (adapter->feat_cap & IXGBE_FEATURE_NEEDS_CTXD)
		adapter->feat_en |= IXGBE_FEATURE_NEEDS_CTXD;

	/* Enabled via sysctl... */
	/* Legacy (single queue) transmit */
	if ((adapter->feat_cap & IXGBE_FEATURE_LEGACY_TX) &&
	    ixv_enable_legacy_tx)
		adapter->feat_en |= IXGBE_FEATURE_LEGACY_TX;
} /* ixv_init_device_features */

/************************************************************************
 * ixv_shutdown - Shutdown entry point
 ************************************************************************/
#if 0 /* XXX NetBSD ought to register something like this through pmf(9) */
static int
ixv_shutdown(device_t dev)
{
	struct adapter *adapter = device_private(dev);
	IXGBE_CORE_LOCK(adapter);
	ixv_stop_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	return (0);
} /* ixv_shutdown */
#endif

static int
ixv_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct adapter *adapter = ifp->if_softc;
	u_short saved_flags;
	u_short change;
	int rv = 0;

	IXGBE_CORE_LOCK(adapter);

	saved_flags = adapter->if_flags;
	change = ifp->if_flags ^ adapter->if_flags;
	if (change != 0)
		adapter->if_flags = ifp->if_flags;

	if ((change & ~(IFF_CANTCHANGE | IFF_DEBUG)) != 0) {
		rv = ENETRESET;
		goto out;
	} else if ((change & IFF_PROMISC) != 0) {
		rv = ixv_set_rxfilter(adapter);
		if (rv != 0) {
			/* Restore previous */
			adapter->if_flags = saved_flags;
			goto out;
		}
	}

	/* Check for ec_capenable. */
	change = ec->ec_capenable ^ adapter->ec_capenable;
	adapter->ec_capenable = ec->ec_capenable;
	if ((change & ~(ETHERCAP_VLAN_MTU | ETHERCAP_VLAN_HWTAGGING
	    | ETHERCAP_VLAN_HWFILTER)) != 0) {
		rv = ENETRESET;
		goto out;
	}

	/*
	 * Special handling is not required for ETHERCAP_VLAN_MTU.
	 * PF's MAXFRS(MHADD) does not include the 4bytes of the VLAN header.
	 */

	/* Set up VLAN support and filter */
	if ((change & (ETHERCAP_VLAN_HWTAGGING | ETHERCAP_VLAN_HWFILTER)) != 0)
		rv = ixv_setup_vlan_support(adapter);

out:
	IXGBE_CORE_UNLOCK(adapter);

	return rv;
}


/************************************************************************
 * ixv_ioctl - Ioctl entry point
 *
 *   Called when the user wants to configure the interface.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static int
ixv_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ifcapreq *ifcr = data;
	int		error;
	int l4csum_en;
	const int l4csum = IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	     IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;

	switch (command) {
	case SIOCSIFFLAGS:
		IOCTL_DEBUGOUT("ioctl: SIOCSIFFLAGS (Set Interface Flags)");
		break;
	case SIOCADDMULTI: {
		struct ether_multi *enm;
		struct ether_multistep step;
		struct ethercom *ec = &adapter->osdep.ec;
		bool overflow = false;
		int mcnt = 0;

		/*
		 * Check the number of multicast address. If it exceeds,
		 * return ENOSPC.
		 * Update this code when we support API 1.3.
		 */
		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			mcnt++;

			/*
			 * This code is before adding, so one room is required
			 * at least.
			 */
			if (mcnt > (IXGBE_MAX_VF_MC - 1)) {
				overflow = true;
				break;
			}
			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
		error = 0;
		if (overflow && ((ec->ec_flags & ETHER_F_ALLMULTI) == 0)) {
			error = hw->mac.ops.update_xcast_mode(hw,
			    IXGBEVF_XCAST_MODE_ALLMULTI);
			if (error == IXGBE_ERR_NOT_TRUSTED) {
				device_printf(adapter->dev,
				    "this interface is not trusted\n");
				error = EPERM;
			} else if (error == IXGBE_ERR_FEATURE_NOT_SUPPORTED) {
				device_printf(adapter->dev,
				    "the PF doesn't support allmulti mode\n");
				error = EOPNOTSUPP;
			} else if (error) {
				device_printf(adapter->dev,
				    "number of Ethernet multicast addresses "
				    "exceeds the limit (%d). error = %d\n",
				    IXGBE_MAX_VF_MC, error);
				error = ENOSPC;
			} else
				ec->ec_flags |= ETHER_F_ALLMULTI;
		}
		if (error)
			return error;
	}
		/*FALLTHROUGH*/
	case SIOCDELMULTI:
		IOCTL_DEBUGOUT("ioctl: SIOC(ADD|DEL)MULTI");
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		IOCTL_DEBUGOUT("ioctl: SIOCxIFMEDIA (Get/Set Interface Media)");
		break;
	case SIOCSIFCAP:
		IOCTL_DEBUGOUT("ioctl: SIOCSIFCAP (Set Capabilities)");
		break;
	case SIOCSIFMTU:
		IOCTL_DEBUGOUT("ioctl: SIOCSIFMTU (Set Interface MTU)");
		break;
	case SIOCZIFDATA:
		IOCTL_DEBUGOUT("ioctl: SIOCZIFDATA (Zero counter)");
		ixv_update_stats(adapter);
		ixv_clear_evcnt(adapter);
		break;
	default:
		IOCTL_DEBUGOUT1("ioctl: UNKNOWN (0x%X)", (int)command);
		break;
	}

	switch (command) {
	case SIOCSIFCAP:
		/* Layer-4 Rx checksum offload has to be turned on and
		 * off as a unit.
		 */
		l4csum_en = ifcr->ifcr_capenable & l4csum;
		if (l4csum_en != l4csum && l4csum_en != 0)
			return EINVAL;
		/*FALLTHROUGH*/
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFFLAGS:
	case SIOCSIFMTU:
	default:
		if ((error = ether_ioctl(ifp, command, data)) != ENETRESET)
			return error;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			;
		else if (command == SIOCSIFCAP || command == SIOCSIFMTU) {
			IXGBE_CORE_LOCK(adapter);
			ixv_init_locked(adapter);
			IXGBE_CORE_UNLOCK(adapter);
		} else if (command == SIOCADDMULTI || command == SIOCDELMULTI) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			IXGBE_CORE_LOCK(adapter);
			ixv_disable_intr(adapter);
			ixv_set_rxfilter(adapter);
			ixv_enable_intr(adapter);
			IXGBE_CORE_UNLOCK(adapter);
		}
		return 0;
	}
} /* ixv_ioctl */

/************************************************************************
 * ixv_init
 ************************************************************************/
static int
ixv_init(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixv_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	return 0;
} /* ixv_init */

/************************************************************************
 * ixv_handle_que
 ************************************************************************/
static void
ixv_handle_que(void *context)
{
	struct ix_queue *que = context;
	struct adapter	*adapter = que->adapter;
	struct tx_ring	*txr = que->txr;
	struct ifnet	*ifp = adapter->ifp;
	bool		more;

	que->handleq.ev_count++;

	if (ifp->if_flags & IFF_RUNNING) {
		more = ixgbe_rxeof(que);
		IXGBE_TX_LOCK(txr);
		more |= ixgbe_txeof(txr);
		if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX))
			if (!ixgbe_mq_ring_empty(ifp, txr->txr_interq))
				ixgbe_mq_start_locked(ifp, txr);
		/* Only for queue 0 */
		/* NetBSD still needs this for CBQ */
		if ((&adapter->queues[0] == que)
		    && (!ixgbe_legacy_ring_empty(ifp, NULL)))
			ixgbe_legacy_start_locked(ifp, txr);
		IXGBE_TX_UNLOCK(txr);
		if (more) {
			que->req.ev_count++;
			if (adapter->txrx_use_workqueue) {
				/*
				 * "enqueued flag" is not required here
				 * the same as ixg(4). See ixgbe_msix_que().
				 */
				workqueue_enqueue(adapter->que_wq,
				    &que->wq_cookie, curcpu());
			} else
				  softint_schedule(que->que_si);
			return;
		}
	}

	/* Re-enable this interrupt */
	ixv_enable_queue(adapter, que->msix);

	return;
} /* ixv_handle_que */

/************************************************************************
 * ixv_handle_que_work
 ************************************************************************/
static void
ixv_handle_que_work(struct work *wk, void *context)
{
	struct ix_queue *que = container_of(wk, struct ix_queue, wq_cookie);

	/*
	 * "enqueued flag" is not required here the same as ixg(4).
	 * See ixgbe_msix_que().
	 */
	ixv_handle_que(que);
}

/************************************************************************
 * ixv_allocate_msix - Setup MSI-X Interrupt resources and handlers
 ************************************************************************/
static int
ixv_allocate_msix(struct adapter *adapter, const struct pci_attach_args *pa)
{
	device_t	dev = adapter->dev;
	struct ix_queue *que = adapter->queues;
	struct		tx_ring *txr = adapter->tx_rings;
	int		error, msix_ctrl, rid, vector = 0;
	pci_chipset_tag_t pc;
	pcitag_t	tag;
	char		intrbuf[PCI_INTRSTR_LEN];
	char		wqname[MAXCOMLEN];
	char		intr_xname[32];
	const char	*intrstr = NULL;
	kcpuset_t	*affinity;
	int		cpu_id = 0;

	pc = adapter->osdep.pc;
	tag = adapter->osdep.tag;

	adapter->osdep.nintrs = adapter->num_queues + 1;
	if (pci_msix_alloc_exact(pa, &adapter->osdep.intrs,
	    adapter->osdep.nintrs) != 0) {
		aprint_error_dev(dev,
		    "failed to allocate MSI-X interrupt\n");
		return (ENXIO);
	}

	kcpuset_create(&affinity, false);
	for (int i = 0; i < adapter->num_queues; i++, vector++, que++, txr++) {
		snprintf(intr_xname, sizeof(intr_xname), "%s TXRX%d",
		    device_xname(dev), i);
		intrstr = pci_intr_string(pc, adapter->osdep.intrs[i], intrbuf,
		    sizeof(intrbuf));
#ifdef IXGBE_MPSAFE
		pci_intr_setattr(pc, &adapter->osdep.intrs[i], PCI_INTR_MPSAFE,
		    true);
#endif
		/* Set the handler function */
		que->res = adapter->osdep.ihs[i] = pci_intr_establish_xname(pc,
		    adapter->osdep.intrs[i], IPL_NET, ixv_msix_que, que,
		    intr_xname);
		if (que->res == NULL) {
			pci_intr_release(pc, adapter->osdep.intrs,
			    adapter->osdep.nintrs);
			aprint_error_dev(dev,
			    "Failed to register QUE handler\n");
			kcpuset_destroy(affinity);
			return (ENXIO);
		}
		que->msix = vector;
		adapter->active_queues |= (u64)(1 << que->msix);

		cpu_id = i;
		/* Round-robin affinity */
		kcpuset_zero(affinity);
		kcpuset_set(affinity, cpu_id % ncpu);
		error = interrupt_distribute(adapter->osdep.ihs[i], affinity,
		    NULL);
		aprint_normal_dev(dev, "for TX/RX, interrupting at %s",
		    intrstr);
		if (error == 0)
			aprint_normal(", bound queue %d to cpu %d\n",
			    i, cpu_id % ncpu);
		else
			aprint_normal("\n");

#ifndef IXGBE_LEGACY_TX
		txr->txr_si
		    = softint_establish(SOFTINT_NET | IXGBE_SOFTINT_FLAGS,
			ixgbe_deferred_mq_start, txr);
#endif
		que->que_si
		    = softint_establish(SOFTINT_NET | IXGBE_SOFTINT_FLAGS,
			ixv_handle_que, que);
		if (que->que_si == NULL) {
			aprint_error_dev(dev,
			    "could not establish software interrupt\n");
		}
	}
	snprintf(wqname, sizeof(wqname), "%sdeferTx", device_xname(dev));
	error = workqueue_create(&adapter->txr_wq, wqname,
	    ixgbe_deferred_mq_start_work, adapter, IXGBE_WORKQUEUE_PRI, IPL_NET,
	    IXGBE_WORKQUEUE_FLAGS);
	if (error) {
		aprint_error_dev(dev, "couldn't create workqueue for deferred Tx\n");
	}
	adapter->txr_wq_enqueued = percpu_alloc(sizeof(u_int));

	snprintf(wqname, sizeof(wqname), "%sTxRx", device_xname(dev));
	error = workqueue_create(&adapter->que_wq, wqname,
	    ixv_handle_que_work, adapter, IXGBE_WORKQUEUE_PRI, IPL_NET,
	    IXGBE_WORKQUEUE_FLAGS);
	if (error) {
		aprint_error_dev(dev,
		    "couldn't create workqueue for Tx/Rx\n");
	}

	/* and Mailbox */
	cpu_id++;
	snprintf(intr_xname, sizeof(intr_xname), "%s link", device_xname(dev));
	adapter->vector = vector;
	intrstr = pci_intr_string(pc, adapter->osdep.intrs[vector], intrbuf,
	    sizeof(intrbuf));
#ifdef IXGBE_MPSAFE
	pci_intr_setattr(pc, &adapter->osdep.intrs[vector], PCI_INTR_MPSAFE,
	    true);
#endif
	/* Set the mbx handler function */
	adapter->osdep.ihs[vector] = pci_intr_establish_xname(pc,
	    adapter->osdep.intrs[vector], IPL_NET, ixv_msix_mbx, adapter,
	    intr_xname);
	if (adapter->osdep.ihs[vector] == NULL) {
		aprint_error_dev(dev, "Failed to register LINK handler\n");
		kcpuset_destroy(affinity);
		return (ENXIO);
	}
	/* Round-robin affinity */
	kcpuset_zero(affinity);
	kcpuset_set(affinity, cpu_id % ncpu);
	error = interrupt_distribute(adapter->osdep.ihs[vector], affinity,
	    NULL);

	aprint_normal_dev(dev,
	    "for link, interrupting at %s", intrstr);
	if (error == 0)
		aprint_normal(", affinity to cpu %d\n", cpu_id % ncpu);
	else
		aprint_normal("\n");

	/* Tasklets for Mailbox */
	adapter->link_si = softint_establish(SOFTINT_NET |IXGBE_SOFTINT_FLAGS,
	    ixv_handle_link, adapter);
	/*
	 * Due to a broken design QEMU will fail to properly
	 * enable the guest for MSI-X unless the vectors in
	 * the table are all set up, so we must rewrite the
	 * ENABLE in the MSI-X control register again at this
	 * point to cause it to successfully initialize us.
	 */
	if (adapter->hw.mac.type == ixgbe_mac_82599_vf) {
		pci_get_capability(pc, tag, PCI_CAP_MSIX, &rid, NULL);
		rid += PCI_MSIX_CTL;
		msix_ctrl = pci_conf_read(pc, tag, rid);
		msix_ctrl |= PCI_MSIX_CTL_ENABLE;
		pci_conf_write(pc, tag, rid, msix_ctrl);
	}

	kcpuset_destroy(affinity);
	return (0);
} /* ixv_allocate_msix */

/************************************************************************
 * ixv_configure_interrupts - Setup MSI-X resources
 *
 *   Note: The VF device MUST use MSI-X, there is no fallback.
 ************************************************************************/
static int
ixv_configure_interrupts(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	int want, queues, msgs;

	/* Must have at least 2 MSI-X vectors */
	msgs = pci_msix_count(adapter->osdep.pc, adapter->osdep.tag);
	if (msgs < 2) {
		aprint_error_dev(dev, "MSIX config error\n");
		return (ENXIO);
	}
	msgs = MIN(msgs, IXG_MAX_NINTR);

	/* Figure out a reasonable auto config value */
	queues = (ncpu > (msgs - 1)) ? (msgs - 1) : ncpu;

	if (ixv_num_queues != 0)
		queues = ixv_num_queues;
	else if ((ixv_num_queues == 0) && (queues > IXGBE_VF_MAX_TX_QUEUES))
		queues = IXGBE_VF_MAX_TX_QUEUES;

	/*
	 * Want vectors for the queues,
	 * plus an additional for mailbox.
	 */
	want = queues + 1;
	if (msgs >= want)
		msgs = want;
	else {
		aprint_error_dev(dev,
		    "MSI-X Configuration Problem, "
		    "%d vectors but %d queues wanted!\n",
		    msgs, want);
		return -1;
	}

	adapter->msix_mem = (void *)1; /* XXX */
	aprint_normal_dev(dev,
	    "Using MSI-X interrupts with %d vectors\n", msgs);
	adapter->num_queues = queues;

	return (0);
} /* ixv_configure_interrupts */


/************************************************************************
 * ixv_handle_link - Tasklet handler for MSI-X MBX interrupts
 *
 *   Done outside of interrupt context since the driver might sleep
 ************************************************************************/
static void
ixv_handle_link(void *context)
{
	struct adapter *adapter = context;

	IXGBE_CORE_LOCK(adapter);

	adapter->hw.mac.ops.check_link(&adapter->hw, &adapter->link_speed,
	    &adapter->link_up, FALSE);
	ixv_update_link_status(adapter);

	IXGBE_CORE_UNLOCK(adapter);
} /* ixv_handle_link */

/************************************************************************
 * ixv_check_link - Used in the local timer to poll for link changes
 ************************************************************************/
static s32
ixv_check_link(struct adapter *adapter)
{
	s32 error;

	KASSERT(mutex_owned(&adapter->core_mtx));

	adapter->hw.mac.get_link_status = TRUE;

	error = adapter->hw.mac.ops.check_link(&adapter->hw,
	    &adapter->link_speed, &adapter->link_up, FALSE);
	ixv_update_link_status(adapter);

	return error;
} /* ixv_check_link */
