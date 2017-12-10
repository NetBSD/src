/******************************************************************************

  Copyright (c) 2001-2015, Intel Corporation 
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
/*$FreeBSD: head/sys/dev/ixgbe/if_ixv.c 302384 2016-07-07 03:39:18Z sbruno $*/
/*$NetBSD: ixv.c,v 1.56.2.2 2017/12/10 10:10:24 snj Exp $*/

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_net_mpsafe.h"
#endif

#include "ixgbe.h"
#include "vlan.h"

/*********************************************************************
 *  Driver version
 *********************************************************************/
char ixv_driver_version[] = "1.4.6-k";

/*********************************************************************
 *  PCI Device ID Table
 *
 *  Used by probe to select devices to load on
 *  Last field stores an index into ixv_strings
 *  Last entry must be all 0s
 *
 *  { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 *********************************************************************/

static ixgbe_vendor_info_t ixv_vendor_info_array[] =
{
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X540_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550_VF, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_VF, 0, 0, 0},
	/* required last entry */
	{0, 0, 0, 0, 0}
};

/*********************************************************************
 *  Table of branding strings
 *********************************************************************/

static const char    *ixv_strings[] = {
	"Intel(R) PRO/10GbE Virtual Function Network Driver"
};

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int      ixv_probe(device_t, cfdata_t, void *);
static void	ixv_attach(device_t, device_t, void *);
static int      ixv_detach(device_t, int);
#if 0
static int      ixv_shutdown(device_t);
#endif
static int      ixv_ioctl(struct ifnet *, u_long, void *);
static int	ixv_init(struct ifnet *);
static void	ixv_init_locked(struct adapter *);
static void	ixv_ifstop(struct ifnet *, int);
static void     ixv_stop(void *);
static void     ixv_media_status(struct ifnet *, struct ifmediareq *);
static int      ixv_media_change(struct ifnet *);
static void     ixv_identify_hardware(struct adapter *);
static int      ixv_allocate_pci_resources(struct adapter *,
		    const struct pci_attach_args *);
static int      ixv_allocate_msix(struct adapter *,
		    const struct pci_attach_args *);
static int	ixv_setup_msix(struct adapter *);
static void	ixv_free_pci_resources(struct adapter *);
static void     ixv_local_timer(void *);
static void     ixv_local_timer_locked(void *);
static int      ixv_setup_interface(device_t, struct adapter *);
static void     ixv_config_link(struct adapter *);

static void     ixv_initialize_transmit_units(struct adapter *);
static void     ixv_initialize_receive_units(struct adapter *);

static void     ixv_enable_intr(struct adapter *);
static void     ixv_disable_intr(struct adapter *);
static void     ixv_set_multi(struct adapter *);
static void     ixv_update_link_status(struct adapter *);
static int	ixv_sysctl_debug(SYSCTLFN_PROTO);
static void	ixv_set_ivar(struct adapter *, u8, u8, s8);
static void	ixv_configure_ivars(struct adapter *);
static u8 *	ixv_mc_array_itr(struct ixgbe_hw *, u8 **, u32 *);

static void	ixv_setup_vlan_support(struct adapter *);
#if 0
static void	ixv_register_vlan(void *, struct ifnet *, u16);
static void	ixv_unregister_vlan(void *, struct ifnet *, u16);
#endif

static void	ixv_add_device_sysctls(struct adapter *);
static void	ixv_save_stats(struct adapter *);
static void	ixv_init_stats(struct adapter *);
static void	ixv_update_stats(struct adapter *);
static void	ixv_add_stats_sysctls(struct adapter *);
static void	ixv_set_sysctl_value(struct adapter *, const char *,
		    const char *, int *, int);

/* The MSI/X Interrupt handlers */
static int	ixv_msix_que(void *);
static int	ixv_msix_mbx(void *);

/* Deferred interrupt tasklets */
static void	ixv_handle_que(void *);
static void	ixv_handle_mbx(void *);

const struct sysctlnode *ixv_sysctl_instance(struct adapter *);
static ixgbe_vendor_info_t *ixv_lookup(const struct pci_attach_args *);

#ifdef DEV_NETMAP
/*
 * This is defined in <dev/netmap/ixgbe_netmap.h>, which is included by
 * if_ix.c.
 */
extern void ixgbe_netmap_attach(struct adapter *adapter);

#include <net/netmap.h>
#include <sys/selinfo.h>
#include <dev/netmap/netmap_kern.h>
#endif /* DEV_NETMAP */

/*********************************************************************
 *  FreeBSD Device Interface Entry Points
 *********************************************************************/

CFATTACH_DECL3_NEW(ixv, sizeof(struct adapter),
    ixv_probe, ixv_attach, ixv_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

# if 0
static device_method_t ixv_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, ixv_probe),
	DEVMETHOD(device_attach, ixv_attach),
	DEVMETHOD(device_detach, ixv_detach),
	DEVMETHOD(device_shutdown, ixv_shutdown),
	DEVMETHOD_END
};
#endif

#if 0
static driver_t ixv_driver = {
	"ixv", ixv_methods, sizeof(struct adapter),
};

devclass_t ixv_devclass;
DRIVER_MODULE(ixv, pci, ixv_driver, ixv_devclass, 0, 0);
MODULE_DEPEND(ixv, pci, 1, 1, 1);
MODULE_DEPEND(ixv, ether, 1, 1, 1);
#ifdef DEV_NETMAP
MODULE_DEPEND(ix, netmap, 1, 1, 1);
#endif /* DEV_NETMAP */
/* XXX depend on 'ix' ? */
#endif

/*
** TUNEABLE PARAMETERS:
*/

/* Number of Queues - do not exceed MSIX vectors - 1 */
static int ixv_num_queues = 0;
#define	TUNABLE_INT(__x, __y)
TUNABLE_INT("hw.ixv.num_queues", &ixv_num_queues);

/*
** AIM: Adaptive Interrupt Moderation
** which means that the interrupt rate
** is varied over time based on the
** traffic for that interrupt vector
*/
static bool ixv_enable_aim = false;
TUNABLE_INT("hw.ixv.enable_aim", &ixv_enable_aim);

/* How many packets rxeof tries to clean at a time */
static int ixv_rx_process_limit = 256;
TUNABLE_INT("hw.ixv.rx_process_limit", &ixv_rx_process_limit);

/* How many packets txeof tries to clean at a time */
static int ixv_tx_process_limit = 256;
TUNABLE_INT("hw.ixv.tx_process_limit", &ixv_tx_process_limit);

/*
** Number of TX descriptors per ring,
** setting higher than RX as this seems
** the better performing choice.
*/
static int ixv_txd = DEFAULT_TXD;
TUNABLE_INT("hw.ixv.txd", &ixv_txd);

/* Number of RX descriptors per ring */
static int ixv_rxd = DEFAULT_RXD;
TUNABLE_INT("hw.ixv.rxd", &ixv_rxd);

/*
** Shadow VFTA table, this is needed because
** the real filter table gets cleared during
** a soft reset and we need to repopulate it.
*/
static u32 ixv_shadow_vfta[IXGBE_VFTA_SIZE];

#ifdef NET_MPSAFE
#define IXGBE_MPSAFE		1
#define IXGBE_CALLOUT_FLAGS	CALLOUT_MPSAFE
#define IXGBE_SOFTINFT_FLAGS	SOFTINT_MPSAFE
#else
#define IXGBE_CALLOUT_FLAGS	0
#define IXGBE_SOFTINFT_FLAGS	0
#endif

/*********************************************************************
 *  Device identification routine
 *
 *  ixv_probe determines if the driver should be loaded on
 *  adapter based on PCI vendor/device id of the adapter.
 *
 *  return 1 on success, 0 on failure
 *********************************************************************/

static int
ixv_probe(device_t dev, cfdata_t cf, void *aux)
{
#ifdef __HAVE_PCI_MSI_MSIX
	const struct pci_attach_args *pa = aux;

	return (ixv_lookup(pa) != NULL) ? 1 : 0;
#else
	return 0;
#endif
}

static ixgbe_vendor_info_t *
ixv_lookup(const struct pci_attach_args *pa)
{
	pcireg_t subid;
	ixgbe_vendor_info_t *ent;

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
ixv_attach(device_t parent, device_t dev, void *aux)
{
	struct adapter *adapter;
	struct ixgbe_hw *hw;
	int             error = 0;
	ixgbe_vendor_info_t *ent;
	const struct pci_attach_args *pa = aux;

	INIT_DEBUGOUT("ixv_attach: begin");

	/* Allocate, clear, and link in our adapter structure */
	adapter = device_private(dev);
	adapter->dev = dev;
	hw = &adapter->hw;

#ifdef DEV_NETMAP
	adapter->init_locked = ixv_init_locked;
	adapter->stop_locked = ixv_stop;
#endif

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

	/* Core Lock Init*/
	IXGBE_CORE_LOCK_INIT(adapter, device_xname(dev));

	/* Set up the timer callout */
	callout_init(&adapter->timer, IXGBE_CALLOUT_FLAGS);

	/* Determine hardware revision */
	ixv_identify_hardware(adapter);

	/* Do base PCI setup - map BAR0 */
	if (ixv_allocate_pci_resources(adapter, pa)) {
		aprint_error_dev(dev, "ixv_allocate_pci_resources() failed!\n");
		error = ENXIO;
		goto err_out;
	}

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

	/* Allocate our TX/RX Queues */
	if (ixgbe_allocate_queues(adapter)) {
		aprint_error_dev(dev, "ixgbe_allocate_queues() failed!\n");
		error = ENOMEM;
		goto err_out;
	}

	/*
	** Initialize the shared code: its
	** at this point the mac type is set.
	*/
	error = ixgbe_init_shared_code(hw);
	if (error) {
		aprint_error_dev(dev, "ixgbe_init_shared_code() failed!\n");
		error = EIO;
		goto err_late;
	}

	/* Setup the mailbox */
	ixgbe_init_mbx_params_vf(hw);

	/* Reset mbox api to 1.0 */
	error = ixgbe_reset_hw(hw);
	if (error == IXGBE_ERR_RESET_FAILED)
		aprint_error_dev(dev, "ixgbe_reset_hw() failure: Reset Failed!\n");
	else if (error)
		aprint_error_dev(dev, "ixgbe_reset_hw() failed with error %d\n", error);
	if (error) {
		error = EIO;
		goto err_late;
	}

	/* Negotiate mailbox API version */
	error = ixgbevf_negotiate_api_version(hw, ixgbe_mbox_api_11);
	if (error)
		aprint_debug_dev(dev,
		    "MBX API 1.1 negotiation failed! Error %d\n", error);

	error = ixgbe_init_hw(hw);
	if (error) {
		aprint_error_dev(dev, "ixgbe_init_hw() failed!\n");
		error = EIO;
		goto err_late;
	}
	
	error = ixv_allocate_msix(adapter, pa); 
	if (error) {
		device_printf(dev, "ixv_allocate_msix() failed!\n");
		goto err_late;
	}

	/* If no mac address was assigned, make a random one */
	if (!ixv_check_ether_addr(hw->mac.addr)) {
		u8 addr[ETHER_ADDR_LEN];
		uint64_t rndval = cprng_strong64();

		memcpy(addr, &rndval, sizeof(addr));
		addr[0] &= 0xFE;
		addr[0] |= 0x02;
		bcopy(addr, hw->mac.addr, sizeof(addr));
	}

	/* hw.ix defaults init */
	adapter->enable_aim = ixv_enable_aim;

	/* Setup OS specific network interface */
	error = ixv_setup_interface(dev, adapter);
	if (error != 0) {
		aprint_error_dev(dev, "ixv_setup_interface() failed!\n");
		goto err_late;
	}

	/* Do the stats setup */
	ixv_save_stats(adapter);
	ixv_init_stats(adapter);

	/* Register for VLAN events */
#if 0 /* XXX delete after write? */
	adapter->vlan_attach = EVENTHANDLER_REGISTER(vlan_config,
	    ixv_register_vlan, adapter, EVENTHANDLER_PRI_FIRST);
	adapter->vlan_detach = EVENTHANDLER_REGISTER(vlan_unconfig,
	    ixv_unregister_vlan, adapter, EVENTHANDLER_PRI_FIRST);
#endif

	/* Add sysctls */
	ixv_add_device_sysctls(adapter);
	ixv_add_stats_sysctls(adapter);

#ifdef DEV_NETMAP
	ixgbe_netmap_attach(adapter);
#endif /* DEV_NETMAP */
	INIT_DEBUGOUT("ixv_attach: end");
	adapter->osdep.attached = true;
	return;

err_late:
	ixgbe_free_transmit_structures(adapter);
	ixgbe_free_receive_structures(adapter);
err_out:
	ixv_free_pci_resources(adapter);
	return;

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
ixv_detach(device_t dev, int flags)
{
	struct adapter *adapter = device_private(dev);
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
	/* Make sure VLANS are not using driver */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		;	/* nothing to do: no VLANs */ 
	else if ((flags & (DETACH_SHUTDOWN|DETACH_FORCE)) != 0)
		vlan_ifdetach(adapter->ifp);
	else {
		aprint_error_dev(dev, "VLANs in use, detach first\n");
		return EBUSY;
	}
#endif

	IXGBE_CORE_LOCK(adapter);
	ixv_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	for (int i = 0; i < adapter->num_queues; i++, que++, txr++) {
#ifndef IXGBE_LEGACY_TX
		softint_disestablish(txr->txr_si);
#endif
		softint_disestablish(que->que_si);
	}

	/* Drain the Mailbox(link) queue */
	softint_disestablish(adapter->link_si);

	/* Unregister VLAN events */
#if 0 /* XXX msaitoh delete after write? */
	if (adapter->vlan_attach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_config, adapter->vlan_attach);
	if (adapter->vlan_detach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_unconfig, adapter->vlan_detach);
#endif

	ether_ifdetach(adapter->ifp);
	callout_halt(&adapter->timer, NULL);
#ifdef DEV_NETMAP
	netmap_detach(adapter->ifp);
#endif /* DEV_NETMAP */
	ixv_free_pci_resources(adapter);
#if 0 /* XXX the NetBSD port is probably missing something here */
	bus_generic_detach(dev);
#endif
	if_detach(adapter->ifp);
	if_percpuq_destroy(adapter->ipq);

	sysctl_teardown(&adapter->sysctllog);
	evcnt_detach(&adapter->handleq);
	evcnt_detach(&adapter->req);
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
		evcnt_detach(&txr->no_desc_avail);
		evcnt_detach(&txr->total_packets);
		evcnt_detach(&txr->tso_tx);
#ifndef IXGBE_LEGACY_TX
		evcnt_detach(&txr->pcq_drops);
#endif

		evcnt_detach(&rxr->rx_packets);
		evcnt_detach(&rxr->rx_bytes);
		evcnt_detach(&rxr->rx_copies);
		evcnt_detach(&rxr->no_jmbuf);
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

	ixgbe_free_transmit_structures(adapter);
	ixgbe_free_receive_structures(adapter);

	IXGBE_CORE_LOCK_DESTROY(adapter);
	return (0);
}

/*********************************************************************
 *
 *  Shutdown entry point
 *
 **********************************************************************/
#if 0 /* XXX NetBSD ought to register something like this through pmf(9) */
static int
ixv_shutdown(device_t dev)
{
	struct adapter *adapter = device_private(dev);
	IXGBE_CORE_LOCK(adapter);
	ixv_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);
	return (0);
}
#endif

static int
ixv_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct adapter *adapter = ifp->if_softc;
	int change = ifp->if_flags ^ adapter->if_flags, rc = 0;

	IXGBE_CORE_LOCK(adapter);

	if (change != 0)
		adapter->if_flags = ifp->if_flags;

	if ((change & ~(IFF_CANTCHANGE|IFF_DEBUG)) != 0)
		rc = ENETRESET;

	IXGBE_CORE_UNLOCK(adapter);

	return rc;
}

/*********************************************************************
 *  Ioctl entry point
 *
 *  ixv_ioctl is called when the user wants to configure the
 *  interface.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/

static int
ixv_ioctl(struct ifnet * ifp, u_long command, void *data)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ifcapreq *ifcr = data;
	struct ifreq	*ifr = (struct ifreq *) data;
	int             error = 0;
	int l4csum_en;
	const int l4csum = IFCAP_CSUM_TCPv4_Rx|IFCAP_CSUM_UDPv4_Rx|
	     IFCAP_CSUM_TCPv6_Rx|IFCAP_CSUM_UDPv6_Rx;

	switch (command) {
	case SIOCSIFFLAGS:
		IOCTL_DEBUGOUT("ioctl: SIOCSIFFLAGS (Set Interface Flags)");
		break;
	case SIOCADDMULTI:
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
	default:
		IOCTL_DEBUGOUT1("ioctl: UNKNOWN (0x%X)", (int)command);
		break;
	}

	switch (command) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		return ifmedia_ioctl(ifp, ifr, &adapter->media, command);
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
			ixv_set_multi(adapter);
			ixv_enable_intr(adapter);
			IXGBE_CORE_UNLOCK(adapter);
		}
		return 0;
	}
}

/*********************************************************************
 *  Init entry point
 *
 *  This routine is used in two ways. It is used by the stack as
 *  init entry point in network interface structure. It is also used
 *  by the driver as a hw/sw initialization routine to get to a
 *  consistent state.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/
#define IXGBE_MHADD_MFS_SHIFT 16

static void
ixv_init_locked(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	device_t 	dev = adapter->dev;
	struct ixgbe_hw *hw = &adapter->hw;
	int error = 0;

	INIT_DEBUGOUT("ixv_init_locked: begin");
	KASSERT(mutex_owned(&adapter->core_mtx));
	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
        callout_stop(&adapter->timer);

        /* reprogram the RAR[0] in case user changed it. */
        ixgbe_set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

	/* Get the latest mac address, User can use a LAA */
	memcpy(hw->mac.addr, CLLADDR(adapter->ifp->if_sadl),
	     IXGBE_ETH_LENGTH_OF_ADDRESS);
        ixgbe_set_rar(hw, 0, hw->mac.addr, 0, 1);
	hw->addr_ctrl.rar_used_count = 1;

	/* Prepare transmit descriptors and buffers */
	if (ixgbe_setup_transmit_structures(adapter)) {
		aprint_error_dev(dev, "Could not setup transmit structures\n");
		ixv_stop(adapter);
		return;
	}

	/* Reset VF and renegotiate mailbox API version */
	ixgbe_reset_hw(hw);
	error = ixgbevf_negotiate_api_version(hw, ixgbe_mbox_api_11);
	if (error)
		device_printf(dev, "MBX API 1.1 negotiation failed! Error %d\n", error);

	ixv_initialize_transmit_units(adapter);

	/* Setup Multicast table */
	ixv_set_multi(adapter);

	/*
	** Determine the correct mbuf pool
	** for doing jumbo/headersplit
	*/
	if (ifp->if_mtu > ETHERMTU)
		adapter->rx_mbuf_sz = MJUMPAGESIZE;
	else
		adapter->rx_mbuf_sz = MCLBYTES;

	/* Prepare receive descriptors and buffers */
	if (ixgbe_setup_receive_structures(adapter)) {
		device_printf(dev, "Could not setup receive structures\n");
		ixv_stop(adapter);
		return;
	}

	/* Configure RX settings */
	ixv_initialize_receive_units(adapter);

#if 0 /* XXX isn't it required? -- msaitoh  */
	/* Set the various hardware offload abilities */
	ifp->if_hwassist = 0;
	if (ifp->if_capenable & IFCAP_TSO4)
		ifp->if_hwassist |= CSUM_TSO;
	if (ifp->if_capenable & IFCAP_TXCSUM) {
		ifp->if_hwassist |= (CSUM_TCP | CSUM_UDP);
#if __FreeBSD_version >= 800000
		ifp->if_hwassist |= CSUM_SCTP;
#endif
	}
#endif
	
	/* Set up VLAN offload and filter */
	ixv_setup_vlan_support(adapter);

	/* Set up MSI/X routing */
	ixv_configure_ivars(adapter);

	/* Set up auto-mask */
	IXGBE_WRITE_REG(hw, IXGBE_VTEIAM, IXGBE_EICS_RTX_QUEUE);

        /* Set moderation on the Link interrupt */
        IXGBE_WRITE_REG(hw, IXGBE_VTEITR(adapter->vector), IXGBE_LINK_ITR);

	/* Stats init */
	ixv_init_stats(adapter);

	/* Config/Enable Link */
	hw->mac.get_link_status = TRUE;
	ixv_config_link(adapter);

	/* Start watchdog */
	callout_reset(&adapter->timer, hz, ixv_local_timer, adapter);

	/* And now turn on interrupts */
	ixv_enable_intr(adapter);

	/* Now inform the stack we're ready */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return;
}

static int
ixv_init(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixv_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
	return 0;
}


/*
**
** MSIX Interrupt Handlers and Tasklets
**
*/

static inline void
ixv_enable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32	queue = 1 << vector;
	u32	mask;

	mask = (IXGBE_EIMS_RTX_QUEUE & queue);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, mask);
}

static inline void
ixv_disable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u64	queue = (u64)(1 << vector);
	u32	mask;

	mask = (IXGBE_EIMS_RTX_QUEUE & queue);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIMC, mask);
}

static inline void
ixv_rearm_queues(struct adapter *adapter, u64 queues)
{
	u32 mask = (IXGBE_EIMS_RTX_QUEUE & queues);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEICS, mask);
}


static void
ixv_handle_que(void *context)
{
	struct ix_queue *que = context;
	struct adapter  *adapter = que->adapter;
	struct tx_ring	*txr = que->txr;
	struct ifnet    *ifp = adapter->ifp;
	bool		more;

	adapter->handleq.ev_count++;

	if (ifp->if_flags & IFF_RUNNING) {
		more = ixgbe_rxeof(que);
		IXGBE_TX_LOCK(txr);
		ixgbe_txeof(txr);
#ifndef IXGBE_LEGACY_TX
		if (pcq_peek(txr->txr_interq) != NULL)
			ixgbe_mq_start_locked(ifp, txr);
#endif
		/* Only for queue 0 */
		if ((&adapter->queues[0] == que)
		    && (!IFQ_IS_EMPTY(&ifp->if_snd)))
			ixgbe_start_locked(txr, ifp);
		IXGBE_TX_UNLOCK(txr);
		if (more) {
			adapter->req.ev_count++;
			softint_schedule(que->que_si);
			return;
		}
	}

	/* Reenable this interrupt */
	ixv_enable_queue(adapter, que->msix);
	return;
}

/*********************************************************************
 *
 *  MSI Queue Interrupt Service routine
 *
 **********************************************************************/
int
ixv_msix_que(void *arg)
{
	struct ix_queue	*que = arg;
	struct adapter  *adapter = que->adapter;
#ifdef IXGBE_LEGACY_TX
	struct ifnet    *ifp = adapter->ifp;
#endif
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
	** Do Adaptive Interrupt Moderation:
        **  - Write out last calculated setting
	**  - Calculate based on average size over
	**    the last interval.
	*/
        if (que->eitr_setting)
                IXGBE_WRITE_REG(&adapter->hw,
                    IXGBE_VTEITR(que->msix),
		    que->eitr_setting);
 
        que->eitr_setting = 0;

        /* Idle, do nothing */
        if ((txr->bytes == 0) && (rxr->bytes == 0))
                goto no_calc;
                                
	if ((txr->bytes) && (txr->packets))
               	newitr = txr->bytes/txr->packets;
	if ((rxr->bytes) && (rxr->packets))
		newitr = max(newitr,
		    (rxr->bytes / rxr->packets));
	newitr += 24; /* account for hardware frame, crc */

	/* set an upper boundary */
	newitr = min(newitr, 3000);

	/* Be nice to the mid range */
	if ((newitr > 300) && (newitr < 1200))
		newitr = (newitr / 3);
	else
		newitr = (newitr / 2);

	newitr |= newitr << 16;
                 
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
	else /* Reenable this interrupt */
		ixv_enable_queue(adapter, que->msix);
	return 1;
}

static int
ixv_msix_mbx(void *arg)
{
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		reg;

	++adapter->link_irq.ev_count;

	/* First get the cause */
	reg = IXGBE_READ_REG(hw, IXGBE_VTEICS);
	/* Clear interrupt with write */
	IXGBE_WRITE_REG(hw, IXGBE_VTEICR, reg);

	/* Link status change */
	if (reg & IXGBE_EICR_LSC)
		softint_schedule(adapter->link_si);

	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, IXGBE_EIMS_OTHER);
	return 1;
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
ixv_media_status(struct ifnet * ifp, struct ifmediareq * ifmr)
{
	struct adapter *adapter = ifp->if_softc;

	INIT_DEBUGOUT("ixv_media_status: begin");
	IXGBE_CORE_LOCK(adapter);
	ixv_update_link_status(adapter);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (!adapter->link_active) {
		ifmr->ifm_active |= IFM_NONE;
		IXGBE_CORE_UNLOCK(adapter);
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;

	switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_100_FULL:
			ifmr->ifm_active |= IFM_100_TX | IFM_FDX;
			break;
	}

	IXGBE_CORE_UNLOCK(adapter);

	return;
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
ixv_media_change(struct ifnet * ifp)
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
}


/*********************************************************************
 *  Multicast Update
 *
 *  This routine is called whenever multicast address list is updated.
 *
 **********************************************************************/
#define IXGBE_RAR_ENTRIES 16

static void
ixv_set_multi(struct adapter *adapter)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	u8	mta[MAX_NUM_MULTICAST_ADDRESSES * IXGBE_ETH_LENGTH_OF_ADDRESS];
	u8	*update_ptr;
	int	mcnt = 0;
	struct ethercom *ec = &adapter->osdep.ec;

	IOCTL_DEBUGOUT("ixv_set_multi: begin");

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		bcopy(enm->enm_addrlo,
		    &mta[mcnt * IXGBE_ETH_LENGTH_OF_ADDRESS],
		    IXGBE_ETH_LENGTH_OF_ADDRESS);
		mcnt++;
		/* XXX This might be required --msaitoh */
		if (mcnt >= MAX_NUM_MULTICAST_ADDRESSES)
			break;
		ETHER_NEXT_MULTI(step, enm);
	}

	update_ptr = mta;

	ixgbe_update_mc_addr_list(&adapter->hw,
	    update_ptr, mcnt, ixv_mc_array_itr, TRUE);

	return;
}

/*
 * This is an iterator function now needed by the multicast
 * shared code. It simply feeds the shared code routine the
 * addresses in the array of ixv_set_multi() one by one.
 */
static u8 *
ixv_mc_array_itr(struct ixgbe_hw *hw, u8 **update_ptr, u32 *vmdq)
{
	u8 *addr = *update_ptr;
	u8 *newptr;
	*vmdq = 0;

	newptr = addr + IXGBE_ETH_LENGTH_OF_ADDRESS;
	*update_ptr = newptr;
	return addr;
}

/*********************************************************************
 *  Timer routine
 *
 *  This routine checks for link status,updates statistics,
 *  and runs the watchdog check.
 *
 **********************************************************************/

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
	int		hung = 0;

	KASSERT(mutex_owned(&adapter->core_mtx));

	ixv_update_link_status(adapter);

	/* Stats Update */
	ixv_update_stats(adapter);

	/*
	** Check the TX queues status
	**      - mark hung queues so we don't schedule on them
	**      - watchdog only if all queues show hung
	*/
	for (int i = 0; i < adapter->num_queues; i++, que++) {
		/* Keep track of queues with work for soft irq */
		if (que->txr->busy)
			queues |= ((u64)1 << que->me);
		/*
		** Each time txeof runs without cleaning, but there
		** are uncleaned descriptors it increments busy. If
		** we get to the MAX we declare it hung.
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
			device_printf(dev,"Warning queue %d "
			    "appears to be hung!\n", i);
			que->txr->busy = IXGBE_QUEUE_HUNG;
			++hung;
		}

	}

	/* Only truly watchdog if all queues show hung */
	if (hung == adapter->num_queues)
		goto watchdog;
	else if (queues != 0) { /* Force an IRQ on queues with work */
		ixv_rearm_queues(adapter, queues);
	}

	callout_reset(&adapter->timer, hz, ixv_local_timer, adapter);
	return;

watchdog:
	device_printf(adapter->dev, "Watchdog timeout -- resetting\n");
	adapter->ifp->if_flags &= ~IFF_RUNNING;
	adapter->watchdog_events.ev_count++;
	ixv_init_locked(adapter);
}

/*
** Note: this routine updates the OS on the link state
**	the real check of the hardware only happens with
**	a link interrupt.
*/
static void
ixv_update_link_status(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	device_t dev = adapter->dev;

	if (adapter->link_up){ 
		if (adapter->link_active == FALSE) {
			if (bootverbose) {
				const char *bpsmsg;

				switch (adapter->link_speed) {
				case IXGBE_LINK_SPEED_10GB_FULL:
					bpsmsg = "10 Gbps";
					break;
				case IXGBE_LINK_SPEED_1GB_FULL:
					bpsmsg = "1 Gbps";
					break;
				case IXGBE_LINK_SPEED_100_FULL:
					bpsmsg = "100 Mbps";
					break;
				default:
					bpsmsg = "unknown speed";
					break;
				}
				device_printf(dev,"Link is up %s %s \n",
				    bpsmsg, "Full Duplex");
			}
			adapter->link_active = TRUE;
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else { /* Link down */
		if (adapter->link_active == TRUE) {
			if (bootverbose)
				device_printf(dev,"Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			adapter->link_active = FALSE;
		}
	}

	return;
}


static void
ixv_ifstop(struct ifnet *ifp, int disable)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixv_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC and deallocates TX/RX buffers.
 *
 **********************************************************************/

static void
ixv_stop(void *arg)
{
	struct ifnet   *ifp;
	struct adapter *adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	ifp = adapter->ifp;

	KASSERT(mutex_owned(&adapter->core_mtx));

	INIT_DEBUGOUT("ixv_stop: begin\n");
	ixv_disable_intr(adapter);

	/* Tell the stack that the interface is no longer active */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ixgbe_reset_hw(hw);
	adapter->hw.adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
	callout_stop(&adapter->timer);

	/* reprogram the RAR[0] in case user changed it. */
	ixgbe_set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

	return;
}


/*********************************************************************
 *
 *  Determine hardware revision.
 *
 **********************************************************************/
static void
ixv_identify_hardware(struct adapter *adapter)
{
	pcitag_t tag;
	pci_chipset_tag_t pc;
	pcireg_t subid, id;
	struct ixgbe_hw *hw = &adapter->hw;

	pc = adapter->osdep.pc;
	tag = adapter->osdep.tag;

	/*
	** Make sure BUSMASTER is set, on a VM under
	** KVM it may not be and will break things.
	*/
	ixgbe_pci_enable_busmaster(pc, tag);

	id = pci_conf_read(pc, tag, PCI_ID_REG);
	subid = pci_conf_read(pc, tag, PCI_SUBSYS_ID_REG);

	/* Save off the information about this board */
	hw->vendor_id = PCI_VENDOR(id);
	hw->device_id = PCI_PRODUCT(id);
	hw->revision_id = PCI_REVISION(pci_conf_read(pc, tag, PCI_CLASS_REG));
	hw->subsystem_vendor_id = PCI_SUBSYS_VENDOR(subid);
	hw->subsystem_device_id = PCI_SUBSYS_ID(subid);

	/* We need this to determine device-specific things */
	ixgbe_set_mac_type(hw);

	/* Set the right number of segments */
	adapter->num_segs = IXGBE_82599_SCATTER;

	return;
}

/*********************************************************************
 *
 *  Setup MSIX Interrupt resources and handlers 
 *
 **********************************************************************/
static int
ixv_allocate_msix(struct adapter *adapter, const struct pci_attach_args *pa)
{
	device_t	dev = adapter->dev;
	struct ix_queue *que = adapter->queues;
	struct		tx_ring *txr = adapter->tx_rings;
	int 		error, rid, vector = 0;
	pci_chipset_tag_t pc;
	pcitag_t	tag;
	char		intrbuf[PCI_INTRSTR_LEN];
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
		    = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixgbe_deferred_mq_start, txr);
#endif
		que->que_si
		    = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixv_handle_que, que);
		if (que->que_si == NULL) {
			aprint_error_dev(dev,
			    "could not establish software interrupt\n"); 
		}
	}

	/* and Mailbox */
	cpu_id++;
	snprintf(intr_xname, sizeof(intr_xname), "%s link", device_xname(dev));
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
		adapter->res = NULL;
		aprint_error_dev(dev, "Failed to register LINK handler\n");
		kcpuset_destroy(affinity);
		return (ENXIO);
	}
	/* Round-robin affinity */
	kcpuset_zero(affinity);
	kcpuset_set(affinity, cpu_id % ncpu);
	error = interrupt_distribute(adapter->osdep.ihs[vector], affinity,NULL);

	aprint_normal_dev(dev,
	    "for link, interrupting at %s", intrstr);
	if (error == 0)
		aprint_normal(", affinity to cpu %d\n", cpu_id % ncpu);
	else
		aprint_normal("\n");

	adapter->vector = vector;
	/* Tasklets for Mailbox */
	adapter->link_si = softint_establish(SOFTINT_NET |IXGBE_SOFTINFT_FLAGS,
	    ixv_handle_mbx, adapter);
	/*
	** Due to a broken design QEMU will fail to properly
	** enable the guest for MSIX unless the vectors in
	** the table are all set up, so we must rewrite the
	** ENABLE in the MSIX control register again at this
	** point to cause it to successfully initialize us.
	*/
	if (adapter->hw.mac.type == ixgbe_mac_82599_vf) {
		int msix_ctrl;
		pci_get_capability(pc, tag, PCI_CAP_MSIX, &rid, NULL);
		rid += PCI_MSIX_CTL;
		msix_ctrl = pci_conf_read(pc, tag, rid);
		msix_ctrl |= PCI_MSIX_CTL_ENABLE;
		pci_conf_write(pc, tag, rid, msix_ctrl);
	}

	kcpuset_destroy(affinity);
	return (0);
}

/*
 * Setup MSIX resources, note that the VF
 * device MUST use MSIX, there is no fallback.
 */
static int
ixv_setup_msix(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	int want, queues, msgs;

	/* Must have at least 2 MSIX vectors */
	msgs = pci_msix_count(adapter->osdep.pc, adapter->osdep.tag);
	if (msgs < 2) {
		aprint_error_dev(dev,"MSIX config error\n");
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
	** Want vectors for the queues,
	** plus an additional for mailbox.
	*/
	want = queues + 1;
	if (msgs >= want)
		msgs = want;
	else {
               	aprint_error_dev(dev,
		    "MSIX Configuration Problem, "
		    "%d vectors but %d queues wanted!\n",
		    msgs, want);
		return -1;
	}

	adapter->msix_mem = (void *)1; /* XXX */
	aprint_normal_dev(dev,
	    "Using MSIX interrupts with %d vectors\n", msgs);
	adapter->num_queues = queues;
	return (msgs);
}


static int
ixv_allocate_pci_resources(struct adapter *adapter,
    const struct pci_attach_args *pa)
{
	pcireg_t	memtype;
	device_t        dev = adapter->dev;
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
		break;
	default:
		aprint_error_dev(dev, "unexpected type on BAR0\n");
		return ENXIO;
	}
	adapter->hw.back = adapter;

	/* Pick up the tuneable queues */
	adapter->num_queues = ixv_num_queues;

	/*
	** Now setup MSI/X, should
	** return us the number of
	** configured vectors.
	*/
	adapter->msix = ixv_setup_msix(adapter);
	if (adapter->msix == ENXIO)
		return (ENXIO);
	else
		return (0);
}

static void
ixv_free_pci_resources(struct adapter * adapter)
{
	struct 		ix_queue *que = adapter->queues;
	int		rid;

	/*
	**  Release all msix queue resources:
	*/
	for (int i = 0; i < adapter->num_queues; i++, que++) {
		if (que->res != NULL)
			pci_intr_disestablish(adapter->osdep.pc,
			    adapter->osdep.ihs[i]);
	}


	/* Clean the Link interrupt last */
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
}

/*********************************************************************
 *
 *  Setup networking device structure and register an interface.
 *
 **********************************************************************/
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
	ifp->if_extflags = IFEF_START_MPSAFE;
#endif
	ifp->if_ioctl = ixv_ioctl;
#ifndef IXGBE_LEGACY_TX
	ifp->if_transmit = ixgbe_mq_start;
#endif
	ifp->if_start = ixgbe_start;
	IFQ_SET_MAXLEN(&ifp->if_snd, adapter->num_tx_desc - 2);
	IFQ_SET_READY(&ifp->if_snd);

	rv = if_initialize(ifp);
	if (rv != 0) {
		aprint_error_dev(dev, "if_initialize failed(%d)\n", rv);
		return rv;
	}
	adapter->ipq = if_percpuq_create(&adapter->osdep.ec.ec_if);
	ether_ifattach(ifp, adapter->hw.mac.addr);
	/*
	 * We use per TX queue softint, so if_deferred_start_init() isn't
	 * used.
	 */
	if_register(ifp);
	ether_set_ifflags_cb(ec, ixv_ifflags_cb);

	adapter->max_frame_size =
	    ifp->if_mtu + IXGBE_MTU_HDR_VLAN;

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);

	ifp->if_capabilities |= IFCAP_HWCSUM | IFCAP_TSOv4;
	ifp->if_capenable = 0;

	ec->ec_capabilities |= ETHERCAP_VLAN_HWCSUM;
	ec->ec_capabilities |= ETHERCAP_JUMBO_MTU;
	ec->ec_capabilities |= ETHERCAP_VLAN_HWTAGGING
	    		| ETHERCAP_VLAN_MTU;
	ec->ec_capenable = ec->ec_capabilities;

	/* Don't enable LRO by default */
	ifp->if_capabilities |= IFCAP_LRO;
#if 0
	ifp->if_capenable = ifp->if_capabilities;
#endif

	/*
	** Dont turn this on by default, if vlans are
	** created on another pseudo device (eg. lagg)
	** then vlan events are not passed thru, breaking
	** operation, but with HW FILTER off it works. If
	** using vlans directly on the em driver you can
	** enable this and get full hardware tag filtering.
	*/
	ec->ec_capabilities |= ETHERCAP_VLAN_HWFILTER;

	/*
	 * Specify the media types supported by this adapter and register
	 * callbacks to update media and link information
	 */
	ifmedia_init(&adapter->media, IFM_IMASK, ixv_media_change,
		     ixv_media_status);
	ifmedia_add(&adapter->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);

	return 0;
}
	
static void
ixv_config_link(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;

	if (hw->mac.ops.check_link)
		hw->mac.ops.check_link(hw, &adapter->link_speed,
		    &adapter->link_up, FALSE);
}


/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
static void
ixv_initialize_transmit_units(struct adapter *adapter)
{
	struct tx_ring	*txr = adapter->tx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;


	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		u64	tdba = txr->txdma.dma_paddr;
		u32	txctrl, txdctl;

		/* Set WTHRESH to 8, burst writeback */
		txdctl = IXGBE_READ_REG(hw, IXGBE_VFTXDCTL(i));
		txdctl |= (8 << 16);
		IXGBE_WRITE_REG(hw, IXGBE_VFTXDCTL(i), txdctl);

		/* Set the HW Tx Head and Tail indices */
	    	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VFTDH(i), 0);
	    	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VFTDT(i), 0);

		/* Set Tx Tail register */
		txr->tail = IXGBE_VFTDT(i);

		/* Set Ring parameters */
		IXGBE_WRITE_REG(hw, IXGBE_VFTDBAL(i),
		       (tdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_VFTDBAH(i), (tdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_VFTDLEN(i),
		    adapter->num_tx_desc *
		    sizeof(struct ixgbe_legacy_tx_desc));
		txctrl = IXGBE_READ_REG(hw, IXGBE_VFDCA_TXCTRL(i));
		txctrl &= ~IXGBE_DCA_TXCTRL_DESC_WRO_EN;
		IXGBE_WRITE_REG(hw, IXGBE_VFDCA_TXCTRL(i), txctrl);

		/* Now enable */
		txdctl = IXGBE_READ_REG(hw, IXGBE_VFTXDCTL(i));
		txdctl |= IXGBE_TXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_VFTXDCTL(i), txdctl);
	}

	return;
}


/*********************************************************************
 *
 *  Setup receive registers and features.
 *
 **********************************************************************/
#define IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT 2

static void
ixv_initialize_receive_units(struct adapter *adapter)
{
	struct	rx_ring	*rxr = adapter->rx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet	*ifp = adapter->ifp;
	u32		bufsz, rxcsum, psrtype;

	if (ifp->if_mtu > ETHERMTU)
		bufsz = 4096 >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;
	else
		bufsz = 2048 >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;

	psrtype = IXGBE_PSRTYPE_TCPHDR | IXGBE_PSRTYPE_UDPHDR |
	    IXGBE_PSRTYPE_IPV4HDR | IXGBE_PSRTYPE_IPV6HDR |
	    IXGBE_PSRTYPE_L2HDR;

	IXGBE_WRITE_REG(hw, IXGBE_VFPSRTYPE, psrtype);

	/* Tell PF our max_frame size */
	ixgbevf_rlpml_set_vf(hw, adapter->max_frame_size);

	for (int i = 0; i < adapter->num_queues; i++, rxr++) {
		u64 rdba = rxr->rxdma.dma_paddr;
		u32 reg, rxdctl;

		/* Disable the queue */
		rxdctl = IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(i));
		rxdctl &= ~IXGBE_RXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(i), rxdctl);
		for (int j = 0; j < 10; j++) {
			if (IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(i)) &
			    IXGBE_RXDCTL_ENABLE)
				msec_delay(1);
			else
				break;
		}
		wmb();
		/* Setup the Base and Length of the Rx Descriptor Ring */
		IXGBE_WRITE_REG(hw, IXGBE_VFRDBAL(i),
		    (rdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_VFRDBAH(i),
		    (rdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_VFRDLEN(i),
		    adapter->num_rx_desc * sizeof(union ixgbe_adv_rx_desc));

		/* Reset the ring indices */
		IXGBE_WRITE_REG(hw, IXGBE_VFRDH(rxr->me), 0);
		IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me), 0);

		/* Set up the SRRCTL register */
		reg = IXGBE_READ_REG(hw, IXGBE_VFSRRCTL(i));
		reg &= ~IXGBE_SRRCTL_BSIZEHDR_MASK;
		reg &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;
		reg |= bufsz;
		reg |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;
		IXGBE_WRITE_REG(hw, IXGBE_VFSRRCTL(i), reg);

		/* Capture Rx Tail index */
		rxr->tail = IXGBE_VFRDT(rxr->me);

		/* Do the queue enabling last */
		rxdctl |= IXGBE_RXDCTL_ENABLE | IXGBE_RXDCTL_VME;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(i), rxdctl);
		for (int k = 0; k < 10; k++) {
			if (IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(i)) &
			    IXGBE_RXDCTL_ENABLE)
				break;
			else
				msec_delay(1);
		}
		wmb();

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
		if (ifp->if_capenable & IFCAP_NETMAP) {
			struct netmap_adapter *na = NA(adapter->ifp);
			struct netmap_kring *kring = &na->rx_rings[i];
			int t = na->num_rx_desc - 1 - nm_kr_rxspace(kring);

			IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me), t);
		} else
#endif /* DEV_NETMAP */
			IXGBE_WRITE_REG(hw, IXGBE_VFRDT(rxr->me),
			    adapter->num_rx_desc - 1);
	}

	rxcsum = IXGBE_READ_REG(hw, IXGBE_RXCSUM);

	if (ifp->if_capenable & IFCAP_RXCSUM)
		rxcsum |= IXGBE_RXCSUM_PCSD;

	if (!(rxcsum & IXGBE_RXCSUM_PCSD))
		rxcsum |= IXGBE_RXCSUM_IPPCSE;

	IXGBE_WRITE_REG(hw, IXGBE_RXCSUM, rxcsum);

	return;
}

static void
ixv_setup_vlan_support(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32		ctrl, vid, vfta, retry;
	struct rx_ring	*rxr;

	/*
	** We get here thru init_locked, meaning
	** a soft reset, this has already cleared
	** the VFTA and other state, so if there
	** have been no vlan's registered do nothing.
	*/
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		return;

	/* Enable the queues */
	for (int i = 0; i < adapter->num_queues; i++) {
		ctrl = IXGBE_READ_REG(hw, IXGBE_VFRXDCTL(i));
		ctrl |= IXGBE_RXDCTL_VME;
		IXGBE_WRITE_REG(hw, IXGBE_VFRXDCTL(i), ctrl);
		/*
		 * Let Rx path know that it needs to store VLAN tag
		 * as part of extra mbuf info.
		 */
		rxr = &adapter->rx_rings[i];
		rxr->vtag_strip = TRUE;
	}

	/*
	** A soft reset zero's out the VFTA, so
	** we need to repopulate it now.
	*/
	for (int i = 0; i < IXGBE_VFTA_SIZE; i++) {
		if (ixv_shadow_vfta[i] == 0)
			continue;
		vfta = ixv_shadow_vfta[i];
		/*
		** Reconstruct the vlan id's
		** based on the bits set in each
		** of the array ints.
		*/
		for (int j = 0; j < 32; j++) {
			retry = 0;
			if ((vfta & (1 << j)) == 0)
				continue;
			vid = (i * 32) + j;
			/* Call the shared code mailbox routine */
			while (ixgbe_set_vfta(hw, vid, 0, TRUE)) {
				if (++retry > 5)
					break;
			}
		}
	}
}

#if 0	/* XXX Badly need to overhaul vlan(4) on NetBSD. */
/*
** This routine is run via an vlan config EVENT,
** it enables us to use the HW Filter table since
** we can get the vlan id. This just creates the
** entry in the soft version of the VFTA, init will
** repopulate the real table.
*/
static void
ixv_register_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct adapter	*adapter = ifp->if_softc;
	u16		index, bit;

	if (ifp->if_softc != arg) /* Not our event */
		return;

	if ((vtag == 0) || (vtag > 4095)) /* Invalid */
		return;

	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	ixv_shadow_vfta[index] |= (1 << bit);
	/* Re-init to load the changes */
	ixv_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/*
** This routine is run via an vlan
** unconfig EVENT, remove our entry
** in the soft vfta.
*/
static void
ixv_unregister_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct adapter	*adapter = ifp->if_softc;
	u16		index, bit;

	if (ifp->if_softc !=  arg)
		return;

	if ((vtag == 0) || (vtag > 4095))	/* Invalid */
		return;

	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	ixv_shadow_vfta[index] &= ~(1 << bit);
	/* Re-init to load the changes */
	ixv_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}
#endif

static void
ixv_enable_intr(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = adapter->queues;
	u32 mask = (IXGBE_EIMS_ENABLE_MASK & ~IXGBE_EIMS_RTX_QUEUE);


	IXGBE_WRITE_REG(hw, IXGBE_VTEIMS, mask);

	mask = IXGBE_EIMS_ENABLE_MASK;
	mask &= ~(IXGBE_EIMS_OTHER | IXGBE_EIMS_LSC);
	IXGBE_WRITE_REG(hw, IXGBE_VTEIAC, mask);

        for (int i = 0; i < adapter->num_queues; i++, que++)
		ixv_enable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(hw);

	return;
}

static void
ixv_disable_intr(struct adapter *adapter)
{
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEIAC, 0);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VTEIMC, ~0);
	IXGBE_WRITE_FLUSH(&adapter->hw);
	return;
}

/*
** Setup the correct IVAR register for a particular MSIX interrupt
**  - entry is the register array entry
**  - vector is the MSIX vector for this queue
**  - type is RX/TX/MISC
*/
static void
ixv_set_ivar(struct adapter *adapter, u8 entry, u8 vector, s8 type)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 ivar, index;

	vector |= IXGBE_IVAR_ALLOC_VAL;

	if (type == -1) { /* MISC IVAR */
		ivar = IXGBE_READ_REG(hw, IXGBE_VTIVAR_MISC);
		ivar &= ~0xFF;
		ivar |= vector;
		IXGBE_WRITE_REG(hw, IXGBE_VTIVAR_MISC, ivar);
	} else {	/* RX/TX IVARS */
		index = (16 * (entry & 1)) + (8 * type);
		ivar = IXGBE_READ_REG(hw, IXGBE_VTIVAR(entry >> 1));
		ivar &= ~(0xFF << index);
		ivar |= (vector << index);
		IXGBE_WRITE_REG(hw, IXGBE_VTIVAR(entry >> 1), ivar);
	}
}

static void
ixv_configure_ivars(struct adapter *adapter)
{
	struct  ix_queue *que = adapter->queues;

        for (int i = 0; i < adapter->num_queues; i++, que++) {
		/* First the RX queue entry */
                ixv_set_ivar(adapter, i, que->msix, 0);
		/* ... and the TX */
		ixv_set_ivar(adapter, i, que->msix, 1);
		/* Set an initial value in EITR */
                IXGBE_WRITE_REG(&adapter->hw,
                    IXGBE_VTEITR(que->msix), IXV_EITR_DEFAULT);
	}

	/* For the mailbox interrupt */
        ixv_set_ivar(adapter, 1, adapter->vector, -1);
}


/*
** Tasklet handler for MSIX MBX interrupts
**  - do outside interrupt since it might sleep
*/
static void
ixv_handle_mbx(void *context)
{
	struct adapter  *adapter = context;

	ixgbe_check_link(&adapter->hw,
	    &adapter->link_speed, &adapter->link_up, 0);
	ixv_update_link_status(adapter);
}

/*
** The VF stats registers never have a truly virgin
** starting point, so this routine tries to make an
** artificial one, marking ground zero on attach as
** it were.
*/
static void
ixv_save_stats(struct adapter *adapter)
{
	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;

	if (stats->vfgprc.ev_count || stats->vfgptc.ev_count) {
		stats->saved_reset_vfgprc +=
		    stats->vfgprc.ev_count - stats->base_vfgprc;
		stats->saved_reset_vfgptc +=
		    stats->vfgptc.ev_count - stats->base_vfgptc;
		stats->saved_reset_vfgorc +=
		    stats->vfgorc.ev_count - stats->base_vfgorc;
		stats->saved_reset_vfgotc +=
		    stats->vfgotc.ev_count - stats->base_vfgotc;
		stats->saved_reset_vfmprc +=
		    stats->vfmprc.ev_count - stats->base_vfmprc;
	}
}
 
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

	adapter->stats.vf.base_vfgprc = adapter->stats.vf.last_vfgprc;
	adapter->stats.vf.base_vfgorc = adapter->stats.vf.last_vfgorc;
	adapter->stats.vf.base_vfgptc = adapter->stats.vf.last_vfgptc;
	adapter->stats.vf.base_vfgotc = adapter->stats.vf.last_vfgotc;
	adapter->stats.vf.base_vfmprc = adapter->stats.vf.last_vfmprc;
}

#define UPDATE_STAT_32(reg, last, count)		\
{							\
	u32 current = IXGBE_READ_REG(hw, reg);		\
	if (current < last)				\
		count.ev_count += 0x100000000LL;	\
	last = current;					\
	count.ev_count &= 0xFFFFFFFF00000000LL;		\
	count.ev_count |= current;			\
}

#define UPDATE_STAT_36(lsb, msb, last, count) 		\
{							\
	u64 cur_lsb = IXGBE_READ_REG(hw, lsb);		\
	u64 cur_msb = IXGBE_READ_REG(hw, msb);		\
	u64 current = ((cur_msb << 32) | cur_lsb);	\
	if (current < last)				\
		count.ev_count += 0x1000000000LL;	\
	last = current;					\
	count.ev_count &= 0xFFFFFFF000000000LL;		\
	count.ev_count |= current;			\
}

/*
** ixv_update_stats - Update the board statistics counters.
*/
void
ixv_update_stats(struct adapter *adapter)
{
        struct ixgbe_hw *hw = &adapter->hw;

        UPDATE_STAT_32(IXGBE_VFGPRC, adapter->stats.vf.last_vfgprc,
	    adapter->stats.vf.vfgprc);
        UPDATE_STAT_32(IXGBE_VFGPTC, adapter->stats.vf.last_vfgptc,
	    adapter->stats.vf.vfgptc);
        UPDATE_STAT_36(IXGBE_VFGORC_LSB, IXGBE_VFGORC_MSB,
	    adapter->stats.vf.last_vfgorc, adapter->stats.vf.vfgorc);
        UPDATE_STAT_36(IXGBE_VFGOTC_LSB, IXGBE_VFGOTC_MSB,
	    adapter->stats.vf.last_vfgotc, adapter->stats.vf.vfgotc);
        UPDATE_STAT_32(IXGBE_VFMPRC, adapter->stats.vf.last_vfmprc,
	    adapter->stats.vf.vfmprc);
}

/**********************************************************************
 *
 *  This routine is called only when em_display_debug_stats is enabled.
 *  This routine provides a way to take a look at important statistics
 *  maintained by the driver and hardware.
 *
 **********************************************************************/
static void
ixv_print_debug_info(struct adapter *adapter)
{
        device_t dev = adapter->dev;
        struct ixgbe_hw         *hw = &adapter->hw;
        struct ix_queue         *que = adapter->queues;
        struct rx_ring          *rxr;
        struct tx_ring          *txr;
#ifdef LRO
        struct lro_ctrl         *lro;
#endif /* LRO */

        device_printf(dev,"Error Byte Count = %u \n",
            IXGBE_READ_REG(hw, IXGBE_ERRBC));

        for (int i = 0; i < adapter->num_queues; i++, que++) {
                txr = que->txr;
                rxr = que->rxr;
#ifdef LRO
                lro = &rxr->lro;
#endif /* LRO */
                device_printf(dev,"QUE(%d) IRQs Handled: %lu\n",
                    que->msix, (long)que->irqs.ev_count);
                device_printf(dev,"RX(%d) Packets Received: %lld\n",
                    rxr->me, (long long)rxr->rx_packets.ev_count);
                device_printf(dev,"RX(%d) Bytes Received: %lu\n",
                    rxr->me, (long)rxr->rx_bytes.ev_count);
#ifdef LRO
                device_printf(dev,"RX(%d) LRO Queued= %lld\n",
                    rxr->me, (long long)lro->lro_queued);
                device_printf(dev,"RX(%d) LRO Flushed= %lld\n",
                    rxr->me, (long long)lro->lro_flushed);
#endif /* LRO */
                device_printf(dev,"TX(%d) Packets Sent: %lu\n",
                    txr->me, (long)txr->total_packets.ev_count);
                device_printf(dev,"TX(%d) NO Desc Avail: %lu\n",
                    txr->me, (long)txr->no_desc_avail.ev_count);
        }

        device_printf(dev,"MBX IRQ Handled: %lu\n",
            (long)adapter->link_irq.ev_count);
        return;
}

static int
ixv_sysctl_debug(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, result;
	struct adapter *adapter;

	node = *rnode;
	adapter = (struct adapter *)node.sysctl_data;
	node.sysctl_data = &result;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error)
		return error;

	if (result == 1)
		ixv_print_debug_info(adapter);

	return 0;
}

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
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
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
	    CTLFLAG_READWRITE, CTLTYPE_BOOL,
	    "enable_aim", SYSCTL_DESCR("Interrupt Moderation"),
	    NULL, 0, &adapter->enable_aim, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
}

/*
 * Add statistic sysctls for the VF.
 */
static void
ixv_add_stats_sysctls(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	const struct sysctlnode *rnode;
	struct sysctllog **log = &adapter->sysctllog;
	struct ix_queue *que = &adapter->queues[0];
	struct tx_ring *txr = que->txr;
	struct rx_ring *rxr = que->rxr;

	struct ixgbevf_hw_stats *stats = &adapter->stats.vf;
	const char *xname = device_xname(dev);

	/* Driver Statistics */
	evcnt_attach_dynamic(&adapter->handleq, EVCNT_TYPE_MISC,
	    NULL, xname, "Handled queue in softint");
	evcnt_attach_dynamic(&adapter->req, EVCNT_TYPE_MISC,
	    NULL, xname, "Requeued in softint");
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
	    NULL, xname, "Link MSIX IRQ Handled");

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

#if 0 /* not yet */
		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "interrupt_rate", SYSCTL_DESCR("Interrupt Rate"),
		    ixgbe_sysctl_interrupt_rate_handler, 0,
		    (void *)&adapter->queues[i], 0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_QUAD,
		    "irqs", SYSCTL_DESCR("irqs on this queue"),
			NULL, 0, &(adapter->queues[i].irqs),
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "txd_head", SYSCTL_DESCR("Transmit Descriptor Head"),
		    ixgbe_sysctl_tdh_handler, 0, (void *)txr,
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "txd_tail", SYSCTL_DESCR("Transmit Descriptor Tail"),
		    ixgbe_sysctl_tdt_handler, 0, (void *)txr,
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;
#endif
		evcnt_attach_dynamic(&adapter->queues[i].irqs, EVCNT_TYPE_INTR,
		    NULL, adapter->queues[i].evnamebuf, "IRQs on queue");
		evcnt_attach_dynamic(&txr->tso_tx, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "TSO");
		evcnt_attach_dynamic(&txr->no_desc_avail, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "Queue No Descriptor Available");
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

#if 0 /* not yet */
		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY,
		    CTLTYPE_INT,
		    "rxd_head", SYSCTL_DESCR("Receive Descriptor Head"),
		    ixgbe_sysctl_rdh_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY,
		    CTLTYPE_INT,
		    "rxd_tail", SYSCTL_DESCR("Receive Descriptor Tail"),
		    ixgbe_sysctl_rdt_handler, 0, (void *)rxr, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			break;
#endif

		evcnt_attach_dynamic(&rxr->rx_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Packets Received");
		evcnt_attach_dynamic(&rxr->rx_bytes, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Bytes Received");
		evcnt_attach_dynamic(&rxr->rx_copies, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Copied RX Frames");
		evcnt_attach_dynamic(&rxr->no_jmbuf, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx no jumbo mbuf");
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

	/* MAC stats get the own sub node */

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
}

static void
ixv_set_sysctl_value(struct adapter *adapter, const char *name,
	const char *description, int *limit, int value)
{
	device_t dev =  adapter->dev;
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
}
