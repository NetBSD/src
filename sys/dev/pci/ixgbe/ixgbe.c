/******************************************************************************

  Copyright (c) 2001-2011, Intel Corporation 
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
/*
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*$FreeBSD: src/sys/dev/ixgbe/ixgbe.c,v 1.51 2011/04/25 23:34:21 jfv Exp $*/
/*$NetBSD: ixgbe.c,v 1.9.2.1 2014/08/10 06:54:57 tls Exp $*/

#include "opt_inet.h"

#include "ixgbe.h"

/*********************************************************************
 *  Set this to one to display debug statistics
 *********************************************************************/
int             ixgbe_display_debug_stats = 0;

/*********************************************************************
 *  Driver version
 *********************************************************************/
char ixgbe_driver_version[] = "2.3.10";

/*********************************************************************
 *  PCI Device ID Table
 *
 *  Used by probe to select devices to load on
 *  Last field stores an index into ixgbe_strings
 *  Last entry must be all 0s
 *
 *  { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 *********************************************************************/

static ixgbe_vendor_info_t ixgbe_vendor_info_array[] =
{
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598AF_DUAL_PORT, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598AF_SINGLE_PORT, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598EB_CX4, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598AT, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598AT2, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598_DA_DUAL_PORT, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598_CX4_DUAL_PORT, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598EB_XF_LR, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598_SR_DUAL_PORT_EM, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82598EB_SFP_LOM, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_KX4, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_KX4_MEZZ, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_XAUI_LOM, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_CX4, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_T3_LOM, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_COMBO_BACKPLANE, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_BACKPLANE_FCOE, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP_FCOE, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP_DELL, 0, 0, 0},
	/* required last entry */
	{0, 0, 0, 0, 0}
};

/*********************************************************************
 *  Table of branding strings
 *********************************************************************/

static const char    *ixgbe_strings[] = {
	"Intel(R) PRO/10GbE PCI-Express Network Driver"
};

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int      ixgbe_probe(device_t, cfdata_t, void *);
static void     ixgbe_attach(device_t, device_t, void *);
static int      ixgbe_detach(device_t, int);
#if 0
static int      ixgbe_shutdown(device_t);
#endif
static void     ixgbe_start(struct ifnet *);
static void     ixgbe_start_locked(struct tx_ring *, struct ifnet *);
#if __FreeBSD_version >= 800000
static int	ixgbe_mq_start(struct ifnet *, struct mbuf *);
static int	ixgbe_mq_start_locked(struct ifnet *,
                    struct tx_ring *, struct mbuf *);
static void	ixgbe_qflush(struct ifnet *);
#endif
static int      ixgbe_ioctl(struct ifnet *, u_long, void *);
static void	ixgbe_ifstop(struct ifnet *, int);
static int	ixgbe_init(struct ifnet *);
static void	ixgbe_init_locked(struct adapter *);
static void     ixgbe_stop(void *);
static void     ixgbe_media_status(struct ifnet *, struct ifmediareq *);
static int      ixgbe_media_change(struct ifnet *);
static void     ixgbe_identify_hardware(struct adapter *);
static int      ixgbe_allocate_pci_resources(struct adapter *,
		    const struct pci_attach_args *);
static int      ixgbe_allocate_msix(struct adapter *,
		    const struct pci_attach_args *);
static int      ixgbe_allocate_legacy(struct adapter *,
		    const struct pci_attach_args *);
static int	ixgbe_allocate_queues(struct adapter *);
static int	ixgbe_setup_msix(struct adapter *);
static void	ixgbe_free_pci_resources(struct adapter *);
static void	ixgbe_local_timer(void *);
static int	ixgbe_setup_interface(device_t, struct adapter *);
static void	ixgbe_config_link(struct adapter *);

static int      ixgbe_allocate_transmit_buffers(struct tx_ring *);
static int	ixgbe_setup_transmit_structures(struct adapter *);
static void	ixgbe_setup_transmit_ring(struct tx_ring *);
static void     ixgbe_initialize_transmit_units(struct adapter *);
static void     ixgbe_free_transmit_structures(struct adapter *);
static void     ixgbe_free_transmit_buffers(struct tx_ring *);

static int      ixgbe_allocate_receive_buffers(struct rx_ring *);
static int      ixgbe_setup_receive_structures(struct adapter *);
static int	ixgbe_setup_receive_ring(struct rx_ring *);
static void     ixgbe_initialize_receive_units(struct adapter *);
static void     ixgbe_free_receive_structures(struct adapter *);
static void     ixgbe_free_receive_buffers(struct rx_ring *);
static void	ixgbe_setup_hw_rsc(struct rx_ring *);

static void     ixgbe_enable_intr(struct adapter *);
static void     ixgbe_disable_intr(struct adapter *);
static void     ixgbe_update_stats_counters(struct adapter *);
static bool	ixgbe_txeof(struct tx_ring *);
static bool	ixgbe_rxeof(struct ix_queue *, int);
static void	ixgbe_rx_checksum(u32, struct mbuf *, u32,
		    struct ixgbe_hw_stats *);
static void     ixgbe_set_promisc(struct adapter *);
static void     ixgbe_set_multi(struct adapter *);
static void     ixgbe_update_link_status(struct adapter *);
static void	ixgbe_refresh_mbufs(struct rx_ring *, int);
static int      ixgbe_xmit(struct tx_ring *, struct mbuf *);
static int	ixgbe_set_flowcntl(SYSCTLFN_PROTO);
static int	ixgbe_set_advertise(SYSCTLFN_PROTO);
static int	ixgbe_dma_malloc(struct adapter *, bus_size_t,
		    struct ixgbe_dma_alloc *, int);
static void     ixgbe_dma_free(struct adapter *, struct ixgbe_dma_alloc *);
static void	ixgbe_add_rx_process_limit(struct adapter *, const char *,
		    const char *, int *, int);
static u32	ixgbe_tx_ctx_setup(struct tx_ring *, struct mbuf *);
static bool	ixgbe_tso_setup(struct tx_ring *, struct mbuf *, u32 *);
static void	ixgbe_set_ivar(struct adapter *, u8, u8, s8);
static void	ixgbe_configure_ivars(struct adapter *);
static u8 *	ixgbe_mc_array_itr(struct ixgbe_hw *, u8 **, u32 *);

static void	ixgbe_setup_vlan_hw_support(struct adapter *);
#if 0
static void	ixgbe_register_vlan(void *, struct ifnet *, u16);
static void	ixgbe_unregister_vlan(void *, struct ifnet *, u16);
#endif

static void     ixgbe_add_hw_stats(struct adapter *adapter);

static __inline void ixgbe_rx_discard(struct rx_ring *, int);
static __inline void ixgbe_rx_input(struct rx_ring *, struct ifnet *,
		    struct mbuf *, u32);

/* Support for pluggable optic modules */
static bool	ixgbe_sfp_probe(struct adapter *);
static void	ixgbe_setup_optics(struct adapter *);

/* Legacy (single vector interrupt handler */
static int	ixgbe_legacy_irq(void *);

#if defined(NETBSD_MSI_OR_MSIX)
/* The MSI/X Interrupt handlers */
static void	ixgbe_msix_que(void *);
static void	ixgbe_msix_link(void *);
#endif

/* Software interrupts for deferred work */
static void	ixgbe_handle_que(void *);
static void	ixgbe_handle_link(void *);
static void	ixgbe_handle_msf(void *);
static void	ixgbe_handle_mod(void *);

const struct sysctlnode *ixgbe_sysctl_instance(struct adapter *);
static ixgbe_vendor_info_t *ixgbe_lookup(const struct pci_attach_args *);

#ifdef IXGBE_FDIR
static void	ixgbe_atr(struct tx_ring *, struct mbuf *);
static void	ixgbe_reinit_fdir(void *, int);
#endif

/*********************************************************************
 *  FreeBSD Device Interface Entry Points
 *********************************************************************/

CFATTACH_DECL3_NEW(ixg, sizeof(struct adapter),
    ixgbe_probe, ixgbe_attach, ixgbe_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

#if 0
devclass_t ixgbe_devclass;
DRIVER_MODULE(ixgbe, pci, ixgbe_driver, ixgbe_devclass, 0, 0);

MODULE_DEPEND(ixgbe, pci, 1, 1, 1);
MODULE_DEPEND(ixgbe, ether, 1, 1, 1);
#endif

/*
** TUNEABLE PARAMETERS:
*/

/*
** AIM: Adaptive Interrupt Moderation
** which means that the interrupt rate
** is varied over time based on the
** traffic for that interrupt vector
*/
static int ixgbe_enable_aim = TRUE;
#define TUNABLE_INT(__x, __y)
TUNABLE_INT("hw.ixgbe.enable_aim", &ixgbe_enable_aim);

static int ixgbe_max_interrupt_rate = (8000000 / IXGBE_LOW_LATENCY);
TUNABLE_INT("hw.ixgbe.max_interrupt_rate", &ixgbe_max_interrupt_rate);

/* How many packets rxeof tries to clean at a time */
static int ixgbe_rx_process_limit = 256;
TUNABLE_INT("hw.ixgbe.rx_process_limit", &ixgbe_rx_process_limit);

/* Flow control setting, default to full */
static int ixgbe_flow_control = ixgbe_fc_full;
TUNABLE_INT("hw.ixgbe.flow_control", &ixgbe_flow_control);

/*
** Smart speed setting, default to on
** this only works as a compile option
** right now as its during attach, set
** this to 'ixgbe_smart_speed_off' to
** disable.
*/
static int ixgbe_smart_speed = ixgbe_smart_speed_on;

/*
 * MSIX should be the default for best performance,
 * but this allows it to be forced off for testing.
 */
static int ixgbe_enable_msix = 1;
TUNABLE_INT("hw.ixgbe.enable_msix", &ixgbe_enable_msix);

/*
 * Header split: this causes the hardware to DMA
 * the header into a separate mbuf from the payload,
 * it can be a performance win in some workloads, but
 * in others it actually hurts, its off by default. 
 */
static bool ixgbe_header_split = FALSE;
TUNABLE_INT("hw.ixgbe.hdr_split", &ixgbe_header_split);

#if defined(NETBSD_MSI_OR_MSIX)
/*
 * Number of Queues, can be set to 0,
 * it then autoconfigures based on the
 * number of cpus with a max of 8. This
 * can be overriden manually here.
 */
static int ixgbe_num_queues = 0;
TUNABLE_INT("hw.ixgbe.num_queues", &ixgbe_num_queues);
#endif

/*
** Number of TX descriptors per ring,
** setting higher than RX as this seems
** the better performing choice.
*/
static int ixgbe_txd = PERFORM_TXD;
TUNABLE_INT("hw.ixgbe.txd", &ixgbe_txd);

/* Number of RX descriptors per ring */
static int ixgbe_rxd = PERFORM_RXD;
TUNABLE_INT("hw.ixgbe.rxd", &ixgbe_rxd);

/* Keep running tab on them for sanity check */
static int ixgbe_total_ports;

#ifdef IXGBE_FDIR
/*
** For Flow Director: this is the
** number of TX packets we sample
** for the filter pool, this means
** every 20th packet will be probed.
**
** This feature can be disabled by 
** setting this to 0.
*/
static int atr_sample_rate = 20;
/* 
** Flow Director actually 'steals'
** part of the packet buffer as its
** filter pool, this variable controls
** how much it uses:
**  0 = 64K, 1 = 128K, 2 = 256K
*/
static int fdir_pballoc = 1;
#endif

/*********************************************************************
 *  Device identification routine
 *
 *  ixgbe_probe determines if the driver should be loaded on
 *  adapter based on PCI vendor/device id of the adapter.
 *
 *  return 1 on success, 0 on failure
 *********************************************************************/

static int
ixgbe_probe(device_t dev, cfdata_t cf, void *aux)
{
	const struct pci_attach_args *pa = aux;

	return (ixgbe_lookup(pa) != NULL) ? 1 : 0;
}

static ixgbe_vendor_info_t *
ixgbe_lookup(const struct pci_attach_args *pa)
{
	pcireg_t subid;
	ixgbe_vendor_info_t *ent;

	INIT_DEBUGOUT("ixgbe_probe: begin");

	if (PCI_VENDOR(pa->pa_id) != IXGBE_INTEL_VENDOR_ID)
		return NULL;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	for (ent = ixgbe_vendor_info_array; ent->vendor_id != 0; ent++) {
		if (PCI_VENDOR(pa->pa_id) == ent->vendor_id &&
		    PCI_PRODUCT(pa->pa_id) == ent->device_id &&

		    (PCI_SUBSYS_VENDOR(subid) == ent->subvendor_id ||
		     ent->subvendor_id == 0) &&

		    (PCI_SUBSYS_ID(subid) == ent->subdevice_id ||
		     ent->subdevice_id == 0)) {
			++ixgbe_total_ports;
			return ent;
		}
	}
	return NULL;
}


static void
ixgbe_sysctl_attach(struct adapter *adapter)
{
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;
	device_t dev;

	dev = adapter->dev;
	log = &adapter->sysctllog;

	if ((rnode = ixgbe_sysctl_instance(adapter)) == NULL) {
		aprint_error_dev(dev, "could not create sysctl root\n");
		return;
	}

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_INT,
	    "num_rx_desc", SYSCTL_DESCR("Number of rx descriptors"),
	    NULL, 0, &adapter->num_rx_desc, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_INT,
	    "num_queues", SYSCTL_DESCR("Number of queues"),
	    NULL, 0, &adapter->num_queues, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "flow_control", SYSCTL_DESCR("Flow Control"),
	    ixgbe_set_flowcntl, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "advertise_gig", SYSCTL_DESCR("1G Link"),
	    ixgbe_set_advertise, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	/* XXX This is an *instance* sysctl controlling a *global* variable.
	 * XXX It's that way in the FreeBSD driver that this derives from.
	 */
	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "enable_aim", SYSCTL_DESCR("Interrupt Moderation"),
	    NULL, 0, &ixgbe_enable_aim, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
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
ixgbe_attach(device_t parent, device_t dev, void *aux)
{
	struct adapter *adapter;
	struct ixgbe_hw *hw;
	int             error = 0;
	u16		csum;
	u32		ctrl_ext;
	ixgbe_vendor_info_t *ent;
	const struct pci_attach_args *pa = aux;

	INIT_DEBUGOUT("ixgbe_attach: begin");

	/* Allocate, clear, and link in our adapter structure */
	adapter = device_private(dev);
	adapter->dev = adapter->osdep.dev = dev;
	hw = &adapter->hw;
	adapter->osdep.pc = pa->pa_pc;
	adapter->osdep.tag = pa->pa_tag;
	adapter->osdep.dmat = pa->pa_dmat;

	ent = ixgbe_lookup(pa);

	KASSERT(ent != NULL);

	aprint_normal(": %s, Version - %s\n",
	    ixgbe_strings[ent->index], ixgbe_driver_version);

	/* Core Lock Init*/
	IXGBE_CORE_LOCK_INIT(adapter, device_xname(dev));

	/* SYSCTL APIs */

	ixgbe_sysctl_attach(adapter);

	/* Set up the timer callout */
	callout_init(&adapter->timer, 0);

	/* Determine hardware revision */
	ixgbe_identify_hardware(adapter);

	/* Do base PCI setup - map BAR0 */
	if (ixgbe_allocate_pci_resources(adapter, pa)) {
		aprint_error_dev(dev, "Allocation of PCI resources failed\n");
		error = ENXIO;
		goto err_out;
	}

	/* Do descriptor calc and sanity checks */
	if (((ixgbe_txd * sizeof(union ixgbe_adv_tx_desc)) % DBA_ALIGN) != 0 ||
	    ixgbe_txd < MIN_TXD || ixgbe_txd > MAX_TXD) {
		aprint_error_dev(dev, "TXD config issue, using default!\n");
		adapter->num_tx_desc = DEFAULT_TXD;
	} else
		adapter->num_tx_desc = ixgbe_txd;

	/*
	** With many RX rings it is easy to exceed the
	** system mbuf allocation. Tuning nmbclusters
	** can alleviate this.
	*/
	if (nmbclusters > 0 ) {
		int s;
		s = (ixgbe_rxd * adapter->num_queues) * ixgbe_total_ports;
		if (s > nmbclusters) {
			aprint_error_dev(dev, "RX Descriptors exceed "
			    "system mbuf max, using default instead!\n");
			ixgbe_rxd = DEFAULT_RXD;
		}
	}

	if (((ixgbe_rxd * sizeof(union ixgbe_adv_rx_desc)) % DBA_ALIGN) != 0 ||
	    ixgbe_rxd < MIN_TXD || ixgbe_rxd > MAX_TXD) {
		aprint_error_dev(dev, "RXD config issue, using default!\n");
		adapter->num_rx_desc = DEFAULT_RXD;
	} else
		adapter->num_rx_desc = ixgbe_rxd;

	/* Allocate our TX/RX Queues */
	if (ixgbe_allocate_queues(adapter)) {
		error = ENOMEM;
		goto err_out;
	}

	/* Allocate multicast array memory. */
	adapter->mta = malloc(sizeof(u8) * IXGBE_ETH_LENGTH_OF_ADDRESS *
	    MAX_NUM_MULTICAST_ADDRESSES, M_DEVBUF, M_NOWAIT);
	if (adapter->mta == NULL) {
		aprint_error_dev(dev, "Cannot allocate multicast setup array\n");
		error = ENOMEM;
		goto err_late;
	}

	/* Initialize the shared code */
	error = ixgbe_init_shared_code(hw);
	if (error == IXGBE_ERR_SFP_NOT_PRESENT) {
		/*
		** No optics in this port, set up
		** so the timer routine will probe 
		** for later insertion.
		*/
		adapter->sfp_probe = TRUE;
		error = 0;
	} else if (error == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		aprint_error_dev(dev,"Unsupported SFP+ module detected!\n");
		error = EIO;
		goto err_late;
	} else if (error) {
		aprint_error_dev(dev,"Unable to initialize the shared code\n");
		error = EIO;
		goto err_late;
	}

	/* Make sure we have a good EEPROM before we read from it */
	if (ixgbe_validate_eeprom_checksum(&adapter->hw, &csum) < 0) {
		aprint_error_dev(dev,"The EEPROM Checksum Is Not Valid\n");
		error = EIO;
		goto err_late;
	}

	/* Get Hardware Flow Control setting */
	hw->fc.requested_mode = ixgbe_fc_full;
	hw->fc.pause_time = IXGBE_FC_PAUSE;
	hw->fc.low_water = IXGBE_FC_LO;
	hw->fc.high_water = IXGBE_FC_HI;
	hw->fc.send_xon = TRUE;

	error = ixgbe_init_hw(hw);
	if (error == IXGBE_ERR_EEPROM_VERSION) {
		aprint_error_dev(dev, "This device is a pre-production adapter/"
		    "LOM.  Please be aware there may be issues associated "
		    "with your hardware.\n If you are experiencing problems "
		    "please contact your Intel or hardware representative "
		    "who provided you with this hardware.\n");
	} else if (error == IXGBE_ERR_SFP_NOT_SUPPORTED)
		aprint_error_dev(dev,"Unsupported SFP+ Module\n");

	if (error) {
		error = EIO;
		aprint_error_dev(dev,"Hardware Initialization Failure\n");
		goto err_late;
	}

	/* Detect and set physical type */
	ixgbe_setup_optics(adapter);

	if ((adapter->msix > 1) && (ixgbe_enable_msix))
		error = ixgbe_allocate_msix(adapter, pa); 
	else
		error = ixgbe_allocate_legacy(adapter, pa); 
	if (error) 
		goto err_late;

	/* Setup OS specific network interface */
	if (ixgbe_setup_interface(dev, adapter) != 0)
		goto err_late;

	/* Sysctl for limiting the amount of work done in software interrupts */
	ixgbe_add_rx_process_limit(adapter, "rx_processing_limit",
	    "max number of rx packets to process", &adapter->rx_process_limit,
	    ixgbe_rx_process_limit);

	/* Initialize statistics */
	ixgbe_update_stats_counters(adapter);

        /* Print PCIE bus type/speed/width info */
	ixgbe_get_bus_info(hw);
	aprint_normal_dev(dev,"PCI Express Bus: Speed %s %s\n",
	    ((hw->bus.speed == ixgbe_bus_speed_5000) ? "5.0Gb/s":
	    (hw->bus.speed == ixgbe_bus_speed_2500) ? "2.5Gb/s":"Unknown"),
	    (hw->bus.width == ixgbe_bus_width_pcie_x8) ? "Width x8" :
	    (hw->bus.width == ixgbe_bus_width_pcie_x4) ? "Width x4" :
	    (hw->bus.width == ixgbe_bus_width_pcie_x1) ? "Width x1" :
	    ("Unknown"));

	if ((hw->bus.width <= ixgbe_bus_width_pcie_x4) &&
	    (hw->bus.speed == ixgbe_bus_speed_2500)) {
		aprint_error_dev(dev, "PCI-Express bandwidth available"
		    " for this card\n     is not sufficient for"
		    " optimal performance.\n");
		aprint_error_dev(dev, "For optimal performance a x8 "
		    "PCIE, or x4 PCIE 2 slot is required.\n");
        }

	/* let hardware know driver is loaded */
	ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
	ctrl_ext |= IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);

	ixgbe_add_hw_stats(adapter);

	INIT_DEBUGOUT("ixgbe_attach: end");
	return;
err_late:
	ixgbe_free_transmit_structures(adapter);
	ixgbe_free_receive_structures(adapter);
err_out:
	if (adapter->ifp != NULL)
		if_free(adapter->ifp);
	ixgbe_free_pci_resources(adapter);
	if (adapter->mta != NULL)
		free(adapter->mta, M_DEVBUF);
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
ixgbe_detach(device_t dev, int flags)
{
	struct adapter *adapter = device_private(dev);
	struct tx_ring *txr = adapter->tx_rings;
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbe_hw_stats *stats = &adapter->stats;
	struct ix_queue *que = adapter->queues;
	u32	ctrl_ext;

	INIT_DEBUGOUT("ixgbe_detach: begin");

	/* Make sure VLANs are not using driver */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		;	/* nothing to do: no VLANs */ 
	else if ((flags & (DETACH_SHUTDOWN|DETACH_FORCE)) != 0)
		vlan_ifdetach(adapter->ifp);
	else {
		aprint_error_dev(dev, "VLANs in use\n");
		return EBUSY;
	}

	IXGBE_CORE_LOCK(adapter);
	ixgbe_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	for (int i = 0; i < adapter->num_queues; i++, que++) {
		softint_disestablish(que->que_si);
	}

	/* Drain the Link queue */
	softint_disestablish(adapter->link_si);
	softint_disestablish(adapter->mod_si);
	softint_disestablish(adapter->msf_si);
#ifdef IXGBE_FDIR
	softint_disestablish(adapter->fdir_si);
#endif

	/* let hardware know driver is unloading */
	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	ctrl_ext &= ~IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT, ctrl_ext);

	ether_ifdetach(adapter->ifp);
	callout_halt(&adapter->timer, NULL);
	ixgbe_free_pci_resources(adapter);
#if 0	/* XXX the NetBSD port is probably missing something here */
	bus_generic_detach(dev);
#endif
	if_detach(adapter->ifp);

	sysctl_teardown(&adapter->sysctllog);
	evcnt_detach(&adapter->handleq);
	evcnt_detach(&adapter->req);
	evcnt_detach(&adapter->morerx);
	evcnt_detach(&adapter->moretx);
	evcnt_detach(&adapter->txloops);
	evcnt_detach(&adapter->efbig_tx_dma_setup);
	evcnt_detach(&adapter->m_defrag_failed);
	evcnt_detach(&adapter->efbig2_tx_dma_setup);
	evcnt_detach(&adapter->einval_tx_dma_setup);
	evcnt_detach(&adapter->other_tx_dma_setup);
	evcnt_detach(&adapter->eagain_tx_dma_setup);
	evcnt_detach(&adapter->enomem_tx_dma_setup);
	evcnt_detach(&adapter->watchdog_events);
	evcnt_detach(&adapter->tso_err);
	evcnt_detach(&adapter->tso_tx);
	evcnt_detach(&adapter->link_irq);
	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		evcnt_detach(&txr->no_desc_avail);
		evcnt_detach(&txr->total_packets);

		if (i < __arraycount(adapter->stats.mpc)) {
			evcnt_detach(&adapter->stats.mpc[i]);
		}
		if (i < __arraycount(adapter->stats.pxontxc)) {
			evcnt_detach(&adapter->stats.pxontxc[i]);
			evcnt_detach(&adapter->stats.pxonrxc[i]);
			evcnt_detach(&adapter->stats.pxofftxc[i]);
			evcnt_detach(&adapter->stats.pxoffrxc[i]);
			evcnt_detach(&adapter->stats.pxon2offc[i]);
		}
		if (i < __arraycount(adapter->stats.qprc)) {
			evcnt_detach(&adapter->stats.qprc[i]);
			evcnt_detach(&adapter->stats.qptc[i]);
			evcnt_detach(&adapter->stats.qbrc[i]);
			evcnt_detach(&adapter->stats.qbtc[i]);
			evcnt_detach(&adapter->stats.qprdc[i]);
		}

		evcnt_detach(&rxr->rx_packets);
		evcnt_detach(&rxr->rx_bytes);
		evcnt_detach(&rxr->no_jmbuf);
		evcnt_detach(&rxr->rx_discarded);
		evcnt_detach(&rxr->rx_split_packets);
		evcnt_detach(&rxr->rx_irq);
	}
	evcnt_detach(&stats->ipcs);
	evcnt_detach(&stats->l4cs);
	evcnt_detach(&stats->ipcs_bad);
	evcnt_detach(&stats->l4cs_bad);
	evcnt_detach(&stats->intzero);
	evcnt_detach(&stats->legint);
	evcnt_detach(&stats->crcerrs);
	evcnt_detach(&stats->illerrc);
	evcnt_detach(&stats->errbc);
	evcnt_detach(&stats->mspdc);
	evcnt_detach(&stats->mlfc);
	evcnt_detach(&stats->mrfc);
	evcnt_detach(&stats->rlec);
	evcnt_detach(&stats->lxontxc);
	evcnt_detach(&stats->lxonrxc);
	evcnt_detach(&stats->lxofftxc);
	evcnt_detach(&stats->lxoffrxc);

	/* Packet Reception Stats */
	evcnt_detach(&stats->tor);
	evcnt_detach(&stats->gorc);
	evcnt_detach(&stats->tpr);
	evcnt_detach(&stats->gprc);
	evcnt_detach(&stats->mprc);
	evcnt_detach(&stats->bprc);
	evcnt_detach(&stats->prc64);
	evcnt_detach(&stats->prc127);
	evcnt_detach(&stats->prc255);
	evcnt_detach(&stats->prc511);
	evcnt_detach(&stats->prc1023);
	evcnt_detach(&stats->prc1522);
	evcnt_detach(&stats->ruc);
	evcnt_detach(&stats->rfc);
	evcnt_detach(&stats->roc);
	evcnt_detach(&stats->rjc);
	evcnt_detach(&stats->mngprc);
	evcnt_detach(&stats->xec);

	/* Packet Transmission Stats */
	evcnt_detach(&stats->gotc);
	evcnt_detach(&stats->tpt);
	evcnt_detach(&stats->gptc);
	evcnt_detach(&stats->bptc);
	evcnt_detach(&stats->mptc);
	evcnt_detach(&stats->mngptc);
	evcnt_detach(&stats->ptc64);
	evcnt_detach(&stats->ptc127);
	evcnt_detach(&stats->ptc255);
	evcnt_detach(&stats->ptc511);
	evcnt_detach(&stats->ptc1023);
	evcnt_detach(&stats->ptc1522);

	/* FC Stats */
	evcnt_detach(&stats->fccrc);
	evcnt_detach(&stats->fclast);
	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
		evcnt_detach(&stats->fcoerpdc);
		evcnt_detach(&stats->fcoeprc);
		evcnt_detach(&stats->fcoeptc);
		evcnt_detach(&stats->fcoedwrc);
		evcnt_detach(&stats->fcoedwtc);
	}

	ixgbe_free_transmit_structures(adapter);
	ixgbe_free_receive_structures(adapter);
	free(adapter->mta, M_DEVBUF);

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
ixgbe_shutdown(device_t dev)
{
	struct adapter *adapter = device_private(dev);
	IXGBE_CORE_LOCK(adapter);
	ixgbe_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);
	return (0);
}
#endif


/*********************************************************************
 *  Transmit entry point
 *
 *  ixgbe_start is called by the stack to initiate a transmit.
 *  The driver will remain in this routine as long as there are
 *  packets to transmit and transmit resources are available.
 *  In case resources are not available stack is notified and
 *  the packet is requeued.
 **********************************************************************/

static void
ixgbe_start_locked(struct tx_ring *txr, struct ifnet * ifp)
{
	int rc;
	struct mbuf    *m_head;
	struct adapter *adapter = txr->adapter;

	IXGBE_TX_LOCK_ASSERT(txr);

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) !=
	    IFF_RUNNING)
		return;
	if (!adapter->link_active)
		return;

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {

		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		if ((rc = ixgbe_xmit(txr, m_head)) == EAGAIN) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m_head);
		if (rc == EFBIG) {
			struct mbuf *mtmp;

			if ((mtmp = m_defrag(m_head, M_DONTWAIT)) != NULL) {
				m_head = mtmp;
				rc = ixgbe_xmit(txr, m_head);
				if (rc != 0)
					adapter->efbig2_tx_dma_setup.ev_count++;
			} else
				adapter->m_defrag_failed.ev_count++;
		}
		if (rc != 0) {
			m_freem(m_head);
			continue;
		}

		/* Send a copy of the frame to the BPF listener */
		bpf_mtap(ifp, m_head);

		/* Set watchdog on */
		getmicrotime(&txr->watchdog_time);
		txr->queue_status = IXGBE_QUEUE_WORKING;

	}
	return;
}

/*
 * Legacy TX start - called by the stack, this
 * always uses the first tx ring, and should
 * not be used with multiqueue tx enabled.
 */
static void
ixgbe_start(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;
	struct tx_ring	*txr = adapter->tx_rings;

	if (ifp->if_flags & IFF_RUNNING) {
		IXGBE_TX_LOCK(txr);
		ixgbe_start_locked(txr, ifp);
		IXGBE_TX_UNLOCK(txr);
	}
	return;
}

#if __FreeBSD_version >= 800000
/*
** Multiqueue Transmit driver
**
*/
static int
ixgbe_mq_start(struct ifnet *ifp, struct mbuf *m)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ix_queue	*que;
	struct tx_ring	*txr;
	int 		i = 0, err = 0;

	/* Which queue to use */
	if ((m->m_flags & M_FLOWID) != 0)
		i = m->m_pkthdr.flowid % adapter->num_queues;

	txr = &adapter->tx_rings[i];
	que = &adapter->queues[i];

	if (IXGBE_TX_TRYLOCK(txr)) {
		err = ixgbe_mq_start_locked(ifp, txr, m);
		IXGBE_TX_UNLOCK(txr);
	} else {
		err = drbr_enqueue(ifp, txr->br, m);
		softint_schedule(que->que_si);
	}

	return (err);
}

static int
ixgbe_mq_start_locked(struct ifnet *ifp, struct tx_ring *txr, struct mbuf *m)
{
	struct adapter  *adapter = txr->adapter;
        struct mbuf     *next;
        int             enqueued, err = 0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) !=
	    IFF_RUNNING || adapter->link_active == 0) {
		if (m != NULL)
			err = drbr_enqueue(ifp, txr->br, m);
		return (err);
	}

	enqueued = 0;
	if (m == NULL) {
		next = drbr_dequeue(ifp, txr->br);
	} else if (drbr_needs_enqueue(ifp, txr->br)) {
		if ((err = drbr_enqueue(ifp, txr->br, m)) != 0)
			return (err);
		next = drbr_dequeue(ifp, txr->br);
	} else
		next = m;

	/* Process the queue */
	while (next != NULL) {
		if ((err = ixgbe_xmit(txr, &next)) != 0) {
			if (next != NULL)
				err = drbr_enqueue(ifp, txr->br, next);
			break;
		}
		enqueued++;
		drbr_stats_update(ifp, next->m_pkthdr.len, next->m_flags);
		/* Send a copy of the frame to the BPF listener */
		bpf_mtap(ifp, next);
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			break;
		if (txr->tx_avail < IXGBE_TX_OP_THRESHOLD)
			ixgbe_txeof(txr);
		if (txr->tx_avail < IXGBE_TX_OP_THRESHOLD) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		next = drbr_dequeue(ifp, txr->br);
	}

	if (enqueued > 0) {
		/* Set watchdog on */
		txr->queue_status = IXGBE_QUEUE_WORKING;
		getmicrotime(&txr->watchdog_time);
	}

	return (err);
}

/*
** Flush all ring buffers
*/
static void
ixgbe_qflush(struct ifnet *ifp)
{
	struct adapter	*adapter = ifp->if_softc;
	struct tx_ring	*txr = adapter->tx_rings;
	struct mbuf	*m;

	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		IXGBE_TX_LOCK(txr);
		while ((m = buf_ring_dequeue_sc(txr->br)) != NULL)
			m_freem(m);
		IXGBE_TX_UNLOCK(txr);
	}
	if_qflush(ifp);
}
#endif /* __FreeBSD_version >= 800000 */

static int
ixgbe_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct adapter *adapter = ifp->if_softc;
	int change = ifp->if_flags ^ adapter->if_flags, rc = 0;

	IXGBE_CORE_LOCK(adapter);

	if (change != 0)
		adapter->if_flags = ifp->if_flags;

	if ((change & ~(IFF_CANTCHANGE|IFF_DEBUG)) != 0)
		rc = ENETRESET;
	else if ((change & (IFF_PROMISC | IFF_ALLMULTI)) != 0)
		ixgbe_set_promisc(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return rc;
}

/*********************************************************************
 *  Ioctl entry point
 *
 *  ixgbe_ioctl is called when the user wants to configure the
 *  interface.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/

static int
ixgbe_ioctl(struct ifnet * ifp, u_long command, void *data)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ifcapreq *ifcr = data;
	struct ifreq	*ifr = data;
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
		IOCTL_DEBUGOUT1("ioctl: UNKNOWN (0x%X)\n", (int)command);
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
			ixgbe_init_locked(adapter);
			IXGBE_CORE_UNLOCK(adapter);
		} else if (command == SIOCADDMULTI || command == SIOCDELMULTI) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			IXGBE_CORE_LOCK(adapter);
			ixgbe_disable_intr(adapter);
			ixgbe_set_multi(adapter);
			ixgbe_enable_intr(adapter);
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
ixgbe_init_locked(struct adapter *adapter)
{
	struct ifnet   *ifp = adapter->ifp;
	device_t 	dev = adapter->dev;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		k, txdctl, mhadd, gpie;
	u32		rxdctl, rxctrl;

	/* XXX check IFF_UP and IFF_RUNNING, power-saving state! */

	KASSERT(mutex_owned(&adapter->core_mtx));
	INIT_DEBUGOUT("ixgbe_init: begin");
	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
        callout_stop(&adapter->timer);

	/* XXX I moved this here from the SIOCSIFMTU case in ixgbe_ioctl(). */
	adapter->max_frame_size =
		ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

        /* reprogram the RAR[0] in case user changed it. */
        ixgbe_set_rar(hw, 0, adapter->hw.mac.addr, 0, IXGBE_RAH_AV);

	/* Get the latest mac address, User can use a LAA */
	memcpy(hw->mac.addr, CLLADDR(adapter->ifp->if_sadl),
	    IXGBE_ETH_LENGTH_OF_ADDRESS);
	ixgbe_set_rar(hw, 0, hw->mac.addr, 0, 1);
	hw->addr_ctrl.rar_used_count = 1;

	/* Prepare transmit descriptors and buffers */
	if (ixgbe_setup_transmit_structures(adapter)) {
		device_printf(dev,"Could not setup transmit structures\n");
		ixgbe_stop(adapter);
		return;
	}

	ixgbe_init_hw(hw);
	ixgbe_initialize_transmit_units(adapter);

	/* Setup Multicast table */
	ixgbe_set_multi(adapter);

	/*
	** Determine the correct mbuf pool
	** for doing jumbo/headersplit
	*/
	if (adapter->max_frame_size <= 2048)
		adapter->rx_mbuf_sz = MCLBYTES;
	else if (adapter->max_frame_size <= 4096)
		adapter->rx_mbuf_sz = MJUMPAGESIZE;
	else if (adapter->max_frame_size <= 9216)
		adapter->rx_mbuf_sz = MJUM9BYTES;
	else
		adapter->rx_mbuf_sz = MJUM16BYTES;

	/* Prepare receive descriptors and buffers */
	if (ixgbe_setup_receive_structures(adapter)) {
		device_printf(dev,"Could not setup receive structures\n");
		ixgbe_stop(adapter);
		return;
	}

	/* Configure RX settings */
	ixgbe_initialize_receive_units(adapter);

	gpie = IXGBE_READ_REG(&adapter->hw, IXGBE_GPIE);

	/* Enable Fan Failure Interrupt */
	gpie |= IXGBE_SDP1_GPIEN;

	/* Add for Thermal detection */
	if (hw->mac.type == ixgbe_mac_82599EB)
		gpie |= IXGBE_SDP2_GPIEN;

	if (adapter->msix > 1) {
		/* Enable Enhanced MSIX mode */
		gpie |= IXGBE_GPIE_MSIX_MODE;
		gpie |= IXGBE_GPIE_EIAME | IXGBE_GPIE_PBA_SUPPORT |
		    IXGBE_GPIE_OCD;
	}
	IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);

	/* Set MTU size */
	if (ifp->if_mtu > ETHERMTU) {
		mhadd = IXGBE_READ_REG(hw, IXGBE_MHADD);
		mhadd &= ~IXGBE_MHADD_MFS_MASK;
		mhadd |= adapter->max_frame_size << IXGBE_MHADD_MFS_SHIFT;
		IXGBE_WRITE_REG(hw, IXGBE_MHADD, mhadd);
	}
	
	/* Now enable all the queues */

	for (int i = 0; i < adapter->num_queues; i++) {
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(i));
		txdctl |= IXGBE_TXDCTL_ENABLE;
		/* Set WTHRESH to 8, burst writeback */
		txdctl |= (8 << 16);
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(i), txdctl);
	}

	for (int i = 0; i < adapter->num_queues; i++) {
		rxdctl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(i));
		if (hw->mac.type == ixgbe_mac_82598EB) {
			/*
			** PTHRESH = 21
			** HTHRESH = 4
			** WTHRESH = 8
			*/
			rxdctl &= ~0x3FFFFF;
			rxdctl |= 0x080420;
		}
		rxdctl |= IXGBE_RXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(i), rxdctl);
		/* XXX I don't trust this loop, and I don't trust the
		 * XXX memory barrier.  What is this meant to do? --dyoung
		 */
		for (k = 0; k < 10; k++) {
			if (IXGBE_READ_REG(hw, IXGBE_RXDCTL(i)) &
			    IXGBE_RXDCTL_ENABLE)
				break;
			else
				msec_delay(1);
		}
		wmb();
		IXGBE_WRITE_REG(hw, IXGBE_RDT(i), adapter->num_rx_desc - 1);
	}

	/* Set up VLAN support and filter */
	ixgbe_setup_vlan_hw_support(adapter);

	/* Enable Receive engine */
	rxctrl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	if (hw->mac.type == ixgbe_mac_82598EB)
		rxctrl |= IXGBE_RXCTRL_DMBYPS;
	rxctrl |= IXGBE_RXCTRL_RXEN;
	ixgbe_enable_rx_dma(hw, rxctrl);

	callout_reset(&adapter->timer, hz, ixgbe_local_timer, adapter);

	/* Set up MSI/X routing */
	if (ixgbe_enable_msix)  {
		ixgbe_configure_ivars(adapter);
		/* Set up auto-mask */
		if (hw->mac.type == ixgbe_mac_82598EB)
			IXGBE_WRITE_REG(hw, IXGBE_EIAM, IXGBE_EICS_RTX_QUEUE);
		else {
			IXGBE_WRITE_REG(hw, IXGBE_EIAM_EX(0), 0xFFFFFFFF);
			IXGBE_WRITE_REG(hw, IXGBE_EIAM_EX(1), 0xFFFFFFFF);
		}
	} else {  /* Simple settings for Legacy/MSI */
                ixgbe_set_ivar(adapter, 0, 0, 0);
                ixgbe_set_ivar(adapter, 0, 0, 1);
		IXGBE_WRITE_REG(hw, IXGBE_EIAM, IXGBE_EICS_RTX_QUEUE);
	}

#ifdef IXGBE_FDIR
	/* Init Flow director */
	if (hw->mac.type != ixgbe_mac_82598EB)
		ixgbe_init_fdir_signature_82599(&adapter->hw, fdir_pballoc);
#endif

	/*
	** Check on any SFP devices that
	** need to be kick-started
	*/
	if (hw->phy.type == ixgbe_phy_none) {
		int err = hw->phy.ops.identify(hw);
		if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
                	device_printf(dev,
			    "Unsupported SFP+ module type was detected.\n");
			return;
        	}
	}

	/* Set moderation on the Link interrupt */
	IXGBE_WRITE_REG(hw, IXGBE_EITR(adapter->linkvec), IXGBE_LINK_ITR);

	/* Config/Enable Link */
	ixgbe_config_link(adapter);

	/* And now turn on interrupts */
	ixgbe_enable_intr(adapter);

	/* Now inform the stack we're ready */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return;
}

static int
ixgbe_init(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
	return 0;	/* XXX ixgbe_init_locked cannot fail?  really? */
}


/*
**
** MSIX Interrupt Handlers and Tasklets
**
*/

static inline void
ixgbe_enable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u64	queue = (u64)(1ULL << vector);
	u32	mask;

	if (hw->mac.type == ixgbe_mac_82598EB) {
                mask = (IXGBE_EIMS_RTX_QUEUE & queue);
                IXGBE_WRITE_REG(hw, IXGBE_EIMS, mask);
	} else {
                mask = (queue & 0xFFFFFFFF);
                if (mask)
                        IXGBE_WRITE_REG(hw, IXGBE_EIMS_EX(0), mask);
                mask = (queue >> 32);
                if (mask)
                        IXGBE_WRITE_REG(hw, IXGBE_EIMS_EX(1), mask);
	}
}

__unused static inline void
ixgbe_disable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u64	queue = (u64)(1ULL << vector);
	u32	mask;

	if (hw->mac.type == ixgbe_mac_82598EB) {
                mask = (IXGBE_EIMS_RTX_QUEUE & queue);
                IXGBE_WRITE_REG(hw, IXGBE_EIMC, mask);
	} else {
                mask = (queue & 0xFFFFFFFF);
                if (mask)
                        IXGBE_WRITE_REG(hw, IXGBE_EIMC_EX(0), mask);
                mask = (queue >> 32);
                if (mask)
                        IXGBE_WRITE_REG(hw, IXGBE_EIMC_EX(1), mask);
	}
}

static inline void
ixgbe_rearm_queues(struct adapter *adapter, u64 queues)
{
	u32 mask;

	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		mask = (IXGBE_EIMS_RTX_QUEUE & queues);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS, mask);
	} else {
		mask = (queues & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(0), mask);
		mask = (queues >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(1), mask);
	}
}


static void
ixgbe_handle_que(void *context)
{
	struct ix_queue *que = context;
	struct adapter  *adapter = que->adapter;
	struct tx_ring  *txr = que->txr;
	struct ifnet    *ifp = adapter->ifp;
	bool		more;

	adapter->handleq.ev_count++;

	if (ifp->if_flags & IFF_RUNNING) {
		more = ixgbe_rxeof(que, adapter->rx_process_limit);
		IXGBE_TX_LOCK(txr);
		ixgbe_txeof(txr);
#if __FreeBSD_version >= 800000
		if (!drbr_empty(ifp, txr->br))
			ixgbe_mq_start_locked(ifp, txr, NULL);
#else
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			ixgbe_start_locked(txr, ifp);
#endif
		IXGBE_TX_UNLOCK(txr);
		if (more) {
			adapter->req.ev_count++;
			softint_schedule(que->que_si);
			return;
		}
	}

	/* Reenable this interrupt */
	ixgbe_enable_queue(adapter, que->msix);

	return;
}


/*********************************************************************
 *
 *  Legacy Interrupt Service routine
 *
 **********************************************************************/

static int
ixgbe_legacy_irq(void *arg)
{
	struct ix_queue *que = arg;
	struct adapter	*adapter = que->adapter;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct 		tx_ring *txr = adapter->tx_rings;
	bool		more_tx, more_rx;
	u32       	reg_eicr, loop = MAX_LOOP;


	reg_eicr = IXGBE_READ_REG(hw, IXGBE_EICR);

	adapter->stats.legint.ev_count++;
	++que->irqs;
	if (reg_eicr == 0) {
		adapter->stats.intzero.ev_count++;
		ixgbe_enable_intr(adapter);
		return 0;
	}

	more_rx = ixgbe_rxeof(que, adapter->rx_process_limit);

	IXGBE_TX_LOCK(txr);
	do {
		adapter->txloops.ev_count++;
		more_tx = ixgbe_txeof(txr);
	} while (loop-- && more_tx);
	IXGBE_TX_UNLOCK(txr);

	if (more_rx || more_tx) {
		if (more_rx)
			adapter->morerx.ev_count++;
		if (more_tx)
			adapter->moretx.ev_count++;
		softint_schedule(que->que_si);
	}

	/* Check for fan failure */
	if ((hw->phy.media_type == ixgbe_media_type_copper) &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP1)) {
                device_printf(adapter->dev, "\nCRITICAL: FAN FAILURE!! "
		    "REPLACE IMMEDIATELY!!\n");
		IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EICR_GPI_SDP1);
	}

	/* Link status change */
	if (reg_eicr & IXGBE_EICR_LSC)
		softint_schedule(adapter->link_si);

	ixgbe_enable_intr(adapter);
	return 1;
}


#if defined(NETBSD_MSI_OR_MSIX)
/*********************************************************************
 *
 *  MSI Queue Interrupt Service routine
 *
 **********************************************************************/
void
ixgbe_msix_que(void *arg)
{
	struct ix_queue	*que = arg;
	struct adapter  *adapter = que->adapter;
	struct tx_ring	*txr = que->txr;
	struct rx_ring	*rxr = que->rxr;
	bool		more_tx, more_rx;
	u32		newitr = 0;

	++que->irqs;

	more_rx = ixgbe_rxeof(que, adapter->rx_process_limit);

	IXGBE_TX_LOCK(txr);
	more_tx = ixgbe_txeof(txr);
	IXGBE_TX_UNLOCK(txr);

	/* Do AIM now? */

	if (ixgbe_enable_aim == FALSE)
		goto no_calc;
	/*
	** Do Adaptive Interrupt Moderation:
        **  - Write out last calculated setting
	**  - Calculate based on average size over
	**    the last interval.
	*/
        if (que->eitr_setting)
                IXGBE_WRITE_REG(&adapter->hw,
                    IXGBE_EITR(que->msix), que->eitr_setting);
 
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

        if (adapter->hw.mac.type == ixgbe_mac_82598EB)
                newitr |= newitr << 16;
        else
                newitr |= IXGBE_EITR_CNT_WDIS;
                 
        /* save for next interrupt */
        que->eitr_setting = newitr;

        /* Reset state */
        txr->bytes = 0;
        txr->packets = 0;
        rxr->bytes = 0;
        rxr->packets = 0;

no_calc:
	if (more_tx || more_rx)
		softint_schedule(que->que_si);
	else /* Reenable this interrupt */
		ixgbe_enable_queue(adapter, que->msix);
	return;
}


static void
ixgbe_msix_link(void *arg)
{
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		reg_eicr;

	++adapter->link_irq.ev_count;

	/* First get the cause */
	reg_eicr = IXGBE_READ_REG(hw, IXGBE_EICS);
	/* Clear interrupt with write */
	IXGBE_WRITE_REG(hw, IXGBE_EICR, reg_eicr);

	/* Link status change */
	if (reg_eicr & IXGBE_EICR_LSC)
		softint_schedule(adapter->link_si);

	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
#ifdef IXGBE_FDIR
		if (reg_eicr & IXGBE_EICR_FLOW_DIR) {
			/* This is probably overkill :) */
			if (!atomic_cmpset_int(&adapter->fdir_reinit, 0, 1))
				return;
                	/* Clear the interrupt */
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_FLOW_DIR);
			/* Turn off the interface */
			adapter->ifp->if_flags &= ~IFF_RUNNING;
			softint_schedule(adapter->fdir_si);
		} else
#endif
		if (reg_eicr & IXGBE_EICR_ECC) {
                	device_printf(adapter->dev, "\nCRITICAL: ECC ERROR!! "
			    "Please Reboot!!\n");
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_ECC);
		} else

		if (reg_eicr & IXGBE_EICR_GPI_SDP1) {
                	/* Clear the interrupt */
                	IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1);
			softint_schedule(adapter->msf_si);
        	} else if (reg_eicr & IXGBE_EICR_GPI_SDP2) {
                	/* Clear the interrupt */
                	IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP2);
			softint_schedule(adapter->mod_si);
		}
        } 

	/* Check for fan failure */
	if ((hw->device_id == IXGBE_DEV_ID_82598AT) &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP1)) {
                device_printf(adapter->dev, "\nCRITICAL: FAN FAILURE!! "
		    "REPLACE IMMEDIATELY!!\n");
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1);
	}

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS, IXGBE_EIMS_OTHER);
	return;
}
#endif

/*********************************************************************
 *
 *  Media Ioctl callback
 *
 *  This routine is called whenever the user queries the status of
 *  the interface using ifconfig.
 *
 **********************************************************************/
static void
ixgbe_media_status(struct ifnet * ifp, struct ifmediareq * ifmr)
{
	struct adapter *adapter = ifp->if_softc;

	INIT_DEBUGOUT("ixgbe_media_status: begin");
	IXGBE_CORE_LOCK(adapter);
	ixgbe_update_link_status(adapter);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (!adapter->link_active) {
		IXGBE_CORE_UNLOCK(adapter);
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;

	switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_T | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= adapter->optics | IFM_FDX;
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
ixgbe_media_change(struct ifnet * ifp)
{
	struct adapter *adapter = ifp->if_softc;
	struct ifmedia *ifm = &adapter->media;

	INIT_DEBUGOUT("ixgbe_media_change: begin");

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

        switch (IFM_SUBTYPE(ifm->ifm_media)) {
        case IFM_AUTO:
                adapter->hw.phy.autoneg_advertised =
		    IXGBE_LINK_SPEED_1GB_FULL | IXGBE_LINK_SPEED_10GB_FULL;
                break;
        default:
                device_printf(adapter->dev, "Only auto media type\n");
		return (EINVAL);
        }

	return (0);
}

/*********************************************************************
 *
 *  This routine maps the mbufs to tx descriptors, allowing the
 *  TX engine to transmit the packets. 
 *  	- return 0 on success, positive on failure
 *
 **********************************************************************/

static int
ixgbe_xmit(struct tx_ring *txr, struct mbuf *m_head)
{
	struct m_tag *mtag;
	struct adapter  *adapter = txr->adapter;
	struct ethercom *ec = &adapter->osdep.ec;
	u32		olinfo_status = 0, cmd_type_len;
	u32		paylen = 0;
	int             i, j, error;
	int		first, last = 0;
	bus_dmamap_t	map;
	struct ixgbe_tx_buf *txbuf;
	union ixgbe_adv_tx_desc *txd = NULL;

	/* Basic descriptor defines */
        cmd_type_len = (IXGBE_ADVTXD_DTYP_DATA |
	    IXGBE_ADVTXD_DCMD_IFCS | IXGBE_ADVTXD_DCMD_DEXT);

	if ((mtag = VLAN_OUTPUT_TAG(ec, m_head)) != NULL)
        	cmd_type_len |= IXGBE_ADVTXD_DCMD_VLE;

        /*
         * Important to capture the first descriptor
         * used because it will contain the index of
         * the one we tell the hardware to report back
         */
        first = txr->next_avail_desc;
	txbuf = &txr->tx_buffers[first];
	map = txbuf->map;

	/*
	 * Map the packet for DMA.
	 */
	error = bus_dmamap_load_mbuf(txr->txtag->dt_dmat, map,
	    m_head, BUS_DMA_NOWAIT);

	switch (error) {
	case EAGAIN:
		adapter->eagain_tx_dma_setup.ev_count++;
		return EAGAIN;
	case ENOMEM:
		adapter->enomem_tx_dma_setup.ev_count++;
		return EAGAIN;
	case EFBIG:
		adapter->efbig_tx_dma_setup.ev_count++;
		return error;
	case EINVAL:
		adapter->einval_tx_dma_setup.ev_count++;
		return error;
	default:
		adapter->other_tx_dma_setup.ev_count++;
		return error;
	case 0:
		break;
	}

	/* Make certain there are enough descriptors */
	if (map->dm_nsegs > txr->tx_avail - 2) {
		txr->no_desc_avail.ev_count++;
		ixgbe_dmamap_unload(txr->txtag, txbuf->map);
		return EAGAIN;
	}

	/*
	** Set up the appropriate offload context
	** this becomes the first descriptor of 
	** a packet.
	*/
	if (m_head->m_pkthdr.csum_flags & (M_CSUM_TSOv4|M_CSUM_TSOv6)) {
		if (ixgbe_tso_setup(txr, m_head, &paylen)) {
			cmd_type_len |= IXGBE_ADVTXD_DCMD_TSE;
			olinfo_status |= IXGBE_TXD_POPTS_IXSM << 8;
			olinfo_status |= IXGBE_TXD_POPTS_TXSM << 8;
			olinfo_status |= paylen << IXGBE_ADVTXD_PAYLEN_SHIFT;
			++adapter->tso_tx.ev_count;
		} else {
			++adapter->tso_err.ev_count;
			/* XXX unload DMA map! --dyoung */
			return ENXIO;
		}
	} else
		olinfo_status |= ixgbe_tx_ctx_setup(txr, m_head);

#ifdef IXGBE_IEEE1588
        /* This is changing soon to an mtag detection */
        if (we detect this mbuf has a TSTAMP mtag)
                cmd_type_len |= IXGBE_ADVTXD_MAC_TSTAMP;
#endif

#ifdef IXGBE_FDIR
	/* Do the flow director magic */
	if ((txr->atr_sample) && (!adapter->fdir_reinit)) {
		++txr->atr_count;
		if (txr->atr_count >= atr_sample_rate) {
			ixgbe_atr(txr, m_head);
			txr->atr_count = 0;
		}
	}
#endif
        /* Record payload length */
	if (paylen == 0)
        	olinfo_status |= m_head->m_pkthdr.len <<
		    IXGBE_ADVTXD_PAYLEN_SHIFT;

	i = txr->next_avail_desc;
	for (j = 0; j < map->dm_nsegs; j++) {
		bus_size_t seglen;
		bus_addr_t segaddr;

		txbuf = &txr->tx_buffers[i];
		txd = &txr->tx_base[i];
		seglen = map->dm_segs[j].ds_len;
		segaddr = htole64(map->dm_segs[j].ds_addr);

		txd->read.buffer_addr = segaddr;
		txd->read.cmd_type_len = htole32(txr->txd_cmd |
		    cmd_type_len |seglen);
		txd->read.olinfo_status = htole32(olinfo_status);
		last = i; /* descriptor that will get completion IRQ */

		if (++i == adapter->num_tx_desc)
			i = 0;

		txbuf->m_head = NULL;
		txbuf->eop_index = -1;
	}

	txd->read.cmd_type_len |=
	    htole32(IXGBE_TXD_CMD_EOP | IXGBE_TXD_CMD_RS);
	txr->tx_avail -= map->dm_nsegs;
	txr->next_avail_desc = i;

	txbuf->m_head = m_head;
	/* We exchange the maps instead of copying because otherwise
	 * we end up with many pointers to the same map and we free
	 * one map twice in ixgbe_free_transmit_structures().  Who
	 * knows what other problems this caused.  --dyoung
	 */
	txr->tx_buffers[first].map = txbuf->map;
	txbuf->map = map;
	bus_dmamap_sync(txr->txtag->dt_dmat, map, 0, m_head->m_pkthdr.len,
	    BUS_DMASYNC_PREWRITE);

        /* Set the index of the descriptor that will be marked done */
        txbuf = &txr->tx_buffers[first];
	txbuf->eop_index = last;

        ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	/*
	 * Advance the Transmit Descriptor Tail (Tdt), this tells the
	 * hardware that this frame is available to transmit.
	 */
	++txr->total_packets.ev_count;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_TDT(txr->me), i);

	return 0;
}

static void
ixgbe_set_promisc(struct adapter *adapter)
{
	u_int32_t       reg_rctl;
	struct ifnet   *ifp = adapter->ifp;

	reg_rctl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	reg_rctl &= (~IXGBE_FCTRL_UPE);
	reg_rctl &= (~IXGBE_FCTRL_MPE);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, reg_rctl);

	if (ifp->if_flags & IFF_PROMISC) {
		reg_rctl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, reg_rctl);
	} else if (ifp->if_flags & IFF_ALLMULTI) {
		reg_rctl |= IXGBE_FCTRL_MPE;
		reg_rctl &= ~IXGBE_FCTRL_UPE;
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, reg_rctl);
	}
	return;
}


/*********************************************************************
 *  Multicast Update
 *
 *  This routine is called whenever multicast address list is updated.
 *
 **********************************************************************/
#define IXGBE_RAR_ENTRIES 16

static void
ixgbe_set_multi(struct adapter *adapter)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	u32	fctrl;
	u8	*mta;
	u8	*update_ptr;
	int	mcnt = 0;
	struct ethercom *ec = &adapter->osdep.ec;
	struct ifnet   *ifp = adapter->ifp;

	IOCTL_DEBUGOUT("ixgbe_set_multi: begin");

	mta = adapter->mta;
	bzero(mta, sizeof(u8) * IXGBE_ETH_LENGTH_OF_ADDRESS *
	    MAX_NUM_MULTICAST_ADDRESSES);

	fctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	fctrl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
	if (ifp->if_flags & IFF_PROMISC)
		fctrl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
	else if (ifp->if_flags & IFF_ALLMULTI) {
		fctrl |= IXGBE_FCTRL_MPE;
		fctrl &= ~IXGBE_FCTRL_UPE;
	} else
		fctrl &= ~(IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
	
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, fctrl);

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		           ETHER_ADDR_LEN) != 0) {
			fctrl |= IXGBE_FCTRL_MPE;
			IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, fctrl);
			break;
		}
		bcopy(enm->enm_addrlo,
		    &mta[mcnt * IXGBE_ETH_LENGTH_OF_ADDRESS],
		    IXGBE_ETH_LENGTH_OF_ADDRESS);
		mcnt++;
		ETHER_NEXT_MULTI(step, enm);
	}

	update_ptr = mta;
	ixgbe_update_mc_addr_list(&adapter->hw,
	    update_ptr, mcnt, ixgbe_mc_array_itr);

	return;
}

/*
 * This is an iterator function now needed by the multicast
 * shared code. It simply feeds the shared code routine the
 * addresses in the array of ixgbe_set_multi() one by one.
 */
static u8 *
ixgbe_mc_array_itr(struct ixgbe_hw *hw, u8 **update_ptr, u32 *vmdq)
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
ixgbe_local_timer1(void *arg)
{
	struct adapter *adapter = arg;
	device_t	dev = adapter->dev;
	struct tx_ring *txr = adapter->tx_rings;

	KASSERT(mutex_owned(&adapter->core_mtx));

	/* Check for pluggable optics */
	if (adapter->sfp_probe)
		if (!ixgbe_sfp_probe(adapter))
			goto out; /* Nothing to do */

	ixgbe_update_link_status(adapter);
	ixgbe_update_stats_counters(adapter);

	/*
	 * If the interface has been paused
	 * then don't do the watchdog check
	 */
	if (IXGBE_READ_REG(&adapter->hw, IXGBE_TFCS) & IXGBE_TFCS_TXOFF)
		goto out;

	/*
	** Check status on the TX queues for a hang
	*/
        for (int i = 0; i < adapter->num_queues; i++, txr++)
		if (txr->queue_status == IXGBE_QUEUE_HUNG)
			goto hung;

out:
	ixgbe_rearm_queues(adapter, adapter->que_mask);
	callout_reset(&adapter->timer, hz, ixgbe_local_timer, adapter);
	return;

hung:
	device_printf(adapter->dev, "Watchdog timeout -- resetting\n");
	device_printf(dev,"Queue(%d) tdh = %d, hw tdt = %d\n", txr->me,
	    IXGBE_READ_REG(&adapter->hw, IXGBE_TDH(txr->me)),
	    IXGBE_READ_REG(&adapter->hw, IXGBE_TDT(txr->me)));
	device_printf(dev,"TX(%d) desc avail = %d,"
	    "Next TX to Clean = %d\n",
	    txr->me, txr->tx_avail, txr->next_to_clean);
	adapter->ifp->if_flags &= ~IFF_RUNNING;
	adapter->watchdog_events.ev_count++;
	ixgbe_init_locked(adapter);
}

static void
ixgbe_local_timer(void *arg)
{
	struct adapter *adapter = arg;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_local_timer1(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/*
** Note: this routine updates the OS on the link state
**	the real check of the hardware only happens with
**	a link interrupt.
*/
static void
ixgbe_update_link_status(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	struct tx_ring *txr = adapter->tx_rings;
	device_t dev = adapter->dev;


	if (adapter->link_up){ 
		if (adapter->link_active == FALSE) {
			if (bootverbose)
				device_printf(dev,"Link is up %d Gbps %s \n",
				    ((adapter->link_speed == 128)? 10:1),
				    "Full Duplex");
			adapter->link_active = TRUE;
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else { /* Link down */
		if (adapter->link_active == TRUE) {
			if (bootverbose)
				device_printf(dev,"Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			adapter->link_active = FALSE;
			for (int i = 0; i < adapter->num_queues;
			    i++, txr++)
				txr->queue_status = IXGBE_QUEUE_IDLE;
		}
	}

	return;
}


static void
ixgbe_ifstop(struct ifnet *ifp, int disable)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC and deallocates TX/RX buffers.
 *
 **********************************************************************/

static void
ixgbe_stop(void *arg)
{
	struct ifnet   *ifp;
	struct adapter *adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	ifp = adapter->ifp;

	KASSERT(mutex_owned(&adapter->core_mtx));

	INIT_DEBUGOUT("ixgbe_stop: begin\n");
	ixgbe_disable_intr(adapter);

	/* Tell the stack that the interface is no longer active */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ixgbe_reset_hw(hw);
	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
	/* Turn off the laser */
	if (hw->phy.multispeed_fiber)
		ixgbe_disable_tx_laser(hw);
	callout_stop(&adapter->timer);

	/* reprogram the RAR[0] in case user changed it. */
	ixgbe_set_rar(&adapter->hw, 0, adapter->hw.mac.addr, 0, IXGBE_RAH_AV);

	return;
}


/*********************************************************************
 *
 *  Determine hardware revision.
 *
 **********************************************************************/
static void
ixgbe_identify_hardware(struct adapter *adapter)
{
	pcitag_t tag;
	pci_chipset_tag_t pc;
	pcireg_t subid, id;
	struct ixgbe_hw *hw = &adapter->hw;

	pc = adapter->osdep.pc;
	tag = adapter->osdep.tag;

	id = pci_conf_read(pc, tag, PCI_ID_REG);
	subid = pci_conf_read(pc, tag, PCI_SUBSYS_ID_REG);

	/* Save off the information about this board */
	hw->vendor_id = PCI_VENDOR(id);
	hw->device_id = PCI_PRODUCT(id);
	hw->revision_id =
	    PCI_REVISION(pci_conf_read(pc, tag, PCI_CLASS_REG));
	hw->subsystem_vendor_id = PCI_SUBSYS_VENDOR(subid);
	hw->subsystem_device_id = PCI_SUBSYS_ID(subid);

	/* We need this here to set the num_segs below */
	ixgbe_set_mac_type(hw);

	/* Pick up the 82599 and VF settings */
	if (hw->mac.type != ixgbe_mac_82598EB) {
		hw->phy.smart_speed = ixgbe_smart_speed;
		adapter->num_segs = IXGBE_82599_SCATTER;
	} else
		adapter->num_segs = IXGBE_82598_SCATTER;

	return;
}

/*********************************************************************
 *
 *  Determine optic type
 *
 **********************************************************************/
static void
ixgbe_setup_optics(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	int		layer;
	
	layer = ixgbe_get_supported_physical_layer(hw);
	switch (layer) {
		case IXGBE_PHYSICAL_LAYER_10GBASE_T:
			adapter->optics = IFM_10G_T;
			break;
		case IXGBE_PHYSICAL_LAYER_1000BASE_T:
			adapter->optics = IFM_1000_T;
			break;
		case IXGBE_PHYSICAL_LAYER_10GBASE_LR:
		case IXGBE_PHYSICAL_LAYER_10GBASE_LRM:
			adapter->optics = IFM_10G_LR;
			break;
		case IXGBE_PHYSICAL_LAYER_10GBASE_SR:
			adapter->optics = IFM_10G_SR;
			break;
		case IXGBE_PHYSICAL_LAYER_10GBASE_KX4:
		case IXGBE_PHYSICAL_LAYER_10GBASE_CX4:
			adapter->optics = IFM_10G_CX4;
			break;
		case IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU:
			adapter->optics = IFM_10G_TWINAX;
			break;
		case IXGBE_PHYSICAL_LAYER_1000BASE_KX:
		case IXGBE_PHYSICAL_LAYER_10GBASE_KR:
		case IXGBE_PHYSICAL_LAYER_10GBASE_XAUI:
		case IXGBE_PHYSICAL_LAYER_UNKNOWN:
		default:
			adapter->optics = IFM_ETHER | IFM_AUTO;
			break;
	}
	return;
}

/*********************************************************************
 *
 *  Setup the Legacy or MSI Interrupt handler
 *
 **********************************************************************/
static int
ixgbe_allocate_legacy(struct adapter *adapter, const struct pci_attach_args *pa)
{
	device_t dev = adapter->dev;
	struct		ix_queue *que = adapter->queues;
	char intrbuf[PCI_INTRSTR_LEN];
#if 0
	int rid = 0;

	/* MSI RID at 1 */
	if (adapter->msix == 1)
		rid = 1;
#endif
 
	/* We allocate a single interrupt resource */
 	if (pci_intr_map(pa, &adapter->osdep.ih) != 0) {
		aprint_error_dev(dev, "unable to map interrupt\n");
		return ENXIO;
	} else {
		aprint_normal_dev(dev, "interrupting at %s\n",
		    pci_intr_string(adapter->osdep.pc, adapter->osdep.ih,
			intrbuf, sizeof(intrbuf)));
	}

	/*
	 * Try allocating a fast interrupt and the associated deferred
	 * processing contexts.
	 */
	que->que_si = softint_establish(SOFTINT_NET, ixgbe_handle_que, que);

	/* Tasklets for Link, SFP and Multispeed Fiber */
	adapter->link_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_link, adapter);
	adapter->mod_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_mod, adapter);
	adapter->msf_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_msf, adapter);

#ifdef IXGBE_FDIR
	adapter->fdir_si =
	    softint_establish(SOFTINT_NET, ixgbe_reinit_fdir, adapter);
#endif
	if (que->que_si == NULL ||
	    adapter->link_si == NULL ||
	    adapter->mod_si == NULL ||
#ifdef IXGBE_FDIR
	    adapter->fdir_si == NULL ||
#endif
	    adapter->msf_si == NULL) {
		aprint_error_dev(dev,
		    "could not establish software interrupts\n"); 
		return ENXIO;
	}

	adapter->osdep.intr = pci_intr_establish(adapter->osdep.pc,
	    adapter->osdep.ih, IPL_NET, ixgbe_legacy_irq, que);
	if (adapter->osdep.intr == NULL) {
		aprint_error_dev(dev, "failed to register interrupt handler\n");
		softint_disestablish(que->que_si);
		softint_disestablish(adapter->link_si);
		softint_disestablish(adapter->mod_si);
		softint_disestablish(adapter->msf_si);
#ifdef IXGBE_FDIR
		softint_disestablish(adapter->fdir_si);
#endif
		return ENXIO;
	}
	/* For simplicity in the handlers */
	adapter->que_mask = IXGBE_EIMS_ENABLE_MASK;

	return (0);
}


/*********************************************************************
 *
 *  Setup MSIX Interrupt resources and handlers 
 *
 **********************************************************************/
static int
ixgbe_allocate_msix(struct adapter *adapter, const struct pci_attach_args *pa)
{
#if !defined(NETBSD_MSI_OR_MSIX)
	return 0;
#else
	device_t        dev = adapter->dev;
	struct 		ix_queue *que = adapter->queues;
	int 		error, rid, vector = 0;

	for (int i = 0; i < adapter->num_queues; i++, vector++, que++) {
		rid = vector + 1;
		que->res = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
		    RF_SHAREABLE | RF_ACTIVE);
		if (que->res == NULL) {
			aprint_error_dev(dev,"Unable to allocate"
		    	    " bus resource: que interrupt [%d]\n", vector);
			return (ENXIO);
		}
		/* Set the handler function */
		error = bus_setup_intr(dev, que->res,
		    INTR_TYPE_NET | INTR_MPSAFE, NULL,
		    ixgbe_msix_que, que, &que->tag);
		if (error) {
			que->res = NULL;
			aprint_error_dev(dev,
			    "Failed to register QUE handler\n");
			return error;
		}
#if __FreeBSD_version >= 800504
		bus_describe_intr(dev, que->res, que->tag, "que %d", i);
#endif
		que->msix = vector;
        	adapter->que_mask |= (u64)(1 << que->msix);
		/*
		** Bind the msix vector, and thus the
		** ring to the corresponding cpu.
		*/
		if (adapter->num_queues > 1)
			bus_bind_intr(dev, que->res, i);

		que->que_si = softint_establish(ixgbe_handle_que, que);
		if (que->que_si == NULL) {
			aprint_error_dev(dev,
			    "could not establish software interrupt\n"); 
		}
	}

	/* and Link */
	rid = vector + 1;
	adapter->res = bus_alloc_resource_any(dev,
    	    SYS_RES_IRQ, &rid, RF_SHAREABLE | RF_ACTIVE);
	if (!adapter->res) {
		aprint_error_dev(dev,"Unable to allocate bus resource: "
		    "Link interrupt [%d]\n", rid);
		return (ENXIO);
	}
	/* Set the link handler function */
	error = bus_setup_intr(dev, adapter->res,
	    INTR_TYPE_NET | INTR_MPSAFE, NULL,
	    ixgbe_msix_link, adapter, &adapter->tag);
	if (error) {
		adapter->res = NULL;
		aprint_error_dev(dev, "Failed to register LINK handler\n");
		return (error);
	}
#if __FreeBSD_version >= 800504
	bus_describe_intr(dev, adapter->res, adapter->tag, "link");
#endif
	adapter->linkvec = vector;
	/* Tasklets for Link, SFP and Multispeed Fiber */
	adapter->link_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_link, adapter);
	adapter->mod_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_mod, adapter);
	adapter->msf_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_msf, adapter);
#ifdef IXGBE_FDIR
	adapter->fdir_si =
	    softint_establish(SOFTINT_NET, ixgbe_reinit_fdir, adapter);
#endif

	return (0);
#endif
}

/*
 * Setup Either MSI/X or MSI
 */
static int
ixgbe_setup_msix(struct adapter *adapter)
{
#if !defined(NETBSD_MSI_OR_MSIX)
	return 0;
#else
	device_t dev = adapter->dev;
	int rid, want, queues, msgs;

	/* Override by tuneable */
	if (ixgbe_enable_msix == 0)
		goto msi;

	/* First try MSI/X */
	rid = PCI_BAR(MSIX_82598_BAR);
	adapter->msix_mem = bus_alloc_resource_any(dev,
	    SYS_RES_MEMORY, &rid, RF_ACTIVE);
       	if (!adapter->msix_mem) {
		rid += 4;	/* 82599 maps in higher BAR */
		adapter->msix_mem = bus_alloc_resource_any(dev,
		    SYS_RES_MEMORY, &rid, RF_ACTIVE);
	}
       	if (!adapter->msix_mem) {
		/* May not be enabled */
		device_printf(adapter->dev,
		    "Unable to map MSIX table \n");
		goto msi;
	}

	msgs = pci_msix_count(dev); 
	if (msgs == 0) { /* system has msix disabled */
		bus_release_resource(dev, SYS_RES_MEMORY,
		    rid, adapter->msix_mem);
		adapter->msix_mem = NULL;
		goto msi;
	}

	/* Figure out a reasonable auto config value */
	queues = (mp_ncpus > (msgs-1)) ? (msgs-1) : mp_ncpus;

	if (ixgbe_num_queues != 0)
		queues = ixgbe_num_queues;
	/* Set max queues to 8 when autoconfiguring */
	else if ((ixgbe_num_queues == 0) && (queues > 8))
		queues = 8;

	/*
	** Want one vector (RX/TX pair) per queue
	** plus an additional for Link.
	*/
	want = queues + 1;
	if (msgs >= want)
		msgs = want;
	else {
               	device_printf(adapter->dev,
		    "MSIX Configuration Problem, "
		    "%d vectors but %d queues wanted!\n",
		    msgs, want);
		return (0); /* Will go to Legacy setup */
	}
	if ((msgs) && pci_alloc_msix(dev, &msgs) == 0) {
               	device_printf(adapter->dev,
		    "Using MSIX interrupts with %d vectors\n", msgs);
		adapter->num_queues = queues;
		return (msgs);
	}
msi:
       	msgs = pci_msi_count(dev);
       	if (msgs == 1 && pci_alloc_msi(dev, &msgs) == 0)
               	device_printf(adapter->dev,"Using MSI interrupt\n");
	return (msgs);
#endif
}


static int
ixgbe_allocate_pci_resources(struct adapter *adapter, const struct pci_attach_args *pa)
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

	/* Legacy defaults */
	adapter->num_queues = 1;
	adapter->hw.back = &adapter->osdep;

	/*
	** Now setup MSI or MSI/X, should
	** return us the number of supported
	** vectors. (Will be 1 for MSI)
	*/
	adapter->msix = ixgbe_setup_msix(adapter);
	return (0);
}

static void
ixgbe_free_pci_resources(struct adapter * adapter)
{
#if defined(NETBSD_MSI_OR_MSIX)
	struct 		ix_queue *que = adapter->queues;
	device_t	dev = adapter->dev;
#endif
	int		rid;

#if defined(NETBSD_MSI_OR_MSIX)
	int		 memrid;
	if (adapter->hw.mac.type == ixgbe_mac_82598EB)
		memrid = PCI_BAR(MSIX_82598_BAR);
	else
		memrid = PCI_BAR(MSIX_82599_BAR);

	/*
	** There is a slight possibility of a failure mode
	** in attach that will result in entering this function
	** before interrupt resources have been initialized, and
	** in that case we do not want to execute the loops below
	** We can detect this reliably by the state of the adapter
	** res pointer.
	*/
	if (adapter->res == NULL)
		goto mem;

	/*
	**  Release all msix queue resources:
	*/
	for (int i = 0; i < adapter->num_queues; i++, que++) {
		rid = que->msix + 1;
		if (que->tag != NULL) {
			bus_teardown_intr(dev, que->res, que->tag);
			que->tag = NULL;
		}
		if (que->res != NULL)
			bus_release_resource(dev, SYS_RES_IRQ, rid, que->res);
	}
#endif

	/* Clean the Legacy or Link interrupt last */
	if (adapter->linkvec) /* we are doing MSIX */
		rid = adapter->linkvec + 1;
	else
		(adapter->msix != 0) ? (rid = 1):(rid = 0);

	pci_intr_disestablish(adapter->osdep.pc, adapter->osdep.intr);
	adapter->osdep.intr = NULL;

#if defined(NETBSD_MSI_OR_MSIX)
mem:
	if (adapter->msix)
		pci_release_msi(dev);

	if (adapter->msix_mem != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY,
		    memrid, adapter->msix_mem);
#endif

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
ixgbe_setup_interface(device_t dev, struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ifnet   *ifp;

	INIT_DEBUGOUT("ixgbe_setup_interface: begin");

	ifp = adapter->ifp = &ec->ec_if;
	strlcpy(ifp->if_xname, device_xname(dev), IFNAMSIZ);
	ifp->if_mtu = ETHERMTU;
	ifp->if_baudrate = 1000000000;
	ifp->if_init = ixgbe_init;
	ifp->if_stop = ixgbe_ifstop;
	ifp->if_softc = adapter;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = ixgbe_ioctl;
	ifp->if_start = ixgbe_start;
#if __FreeBSD_version >= 800000
	ifp->if_transmit = ixgbe_mq_start;
	ifp->if_qflush = ixgbe_qflush;
#endif
	ifp->if_snd.ifq_maxlen = adapter->num_tx_desc - 2;

	if_attach(ifp);
	ether_ifattach(ifp, adapter->hw.mac.addr);
	ether_set_ifflags_cb(ec, ixgbe_ifflags_cb);

	adapter->max_frame_size =
	    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);

	ifp->if_capabilities |= IFCAP_HWCSUM | IFCAP_TSOv4;
	ifp->if_capenable = 0;

	ec->ec_capabilities |= ETHERCAP_VLAN_HWCSUM;
	ec->ec_capabilities |= ETHERCAP_VLAN_HWTAGGING | ETHERCAP_VLAN_MTU;
	ec->ec_capabilities |= ETHERCAP_JUMBO_MTU;
	ec->ec_capenable = ec->ec_capabilities;

	/* Don't enable LRO by default */
	ifp->if_capabilities |= IFCAP_LRO;

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
	ifmedia_init(&adapter->media, IFM_IMASK, ixgbe_media_change,
		     ixgbe_media_status);
	ifmedia_add(&adapter->media, IFM_ETHER | adapter->optics, 0, NULL);
	ifmedia_set(&adapter->media, IFM_ETHER | adapter->optics);
	if (hw->device_id == IXGBE_DEV_ID_82598AT) {
		ifmedia_add(&adapter->media,
		    IFM_ETHER | IFM_1000_T | IFM_FDX, 0, NULL);
		ifmedia_add(&adapter->media,
		    IFM_ETHER | IFM_1000_T, 0, NULL);
	}
	ifmedia_add(&adapter->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);

	return (0);
}

static void
ixgbe_config_link(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32	autoneg, err = 0;
	bool	sfp, negotiate;

	sfp = ixgbe_is_sfp(hw);

	if (sfp) { 
		if (hw->phy.multispeed_fiber) {
			hw->mac.ops.setup_sfp(hw);
			ixgbe_enable_tx_laser(hw);
			softint_schedule(adapter->msf_si);
		} else {
			softint_schedule(adapter->mod_si);
		}
	} else {
		if (hw->mac.ops.check_link)
			err = ixgbe_check_link(hw, &autoneg,
			    &adapter->link_up, FALSE);
		if (err)
			goto out;
		autoneg = hw->phy.autoneg_advertised;
		if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
                	err  = hw->mac.ops.get_link_capabilities(hw,
			    &autoneg, &negotiate);
		else
			negotiate = 0;
		if (err)
			goto out;
		if (hw->mac.ops.setup_link)
                	err = hw->mac.ops.setup_link(hw, autoneg,
			    negotiate, adapter->link_up);
	}
out:
	return;
}

/********************************************************************
 * Manage DMA'able memory.
 *******************************************************************/

static int
ixgbe_dma_malloc(struct adapter *adapter, const bus_size_t size,
		struct ixgbe_dma_alloc *dma, const int mapflags)
{
	device_t dev = adapter->dev;
	int             r, rsegs;

	r = ixgbe_dma_tag_create(adapter->osdep.dmat,	/* parent */
			       DBA_ALIGN, 0,	/* alignment, bounds */
			       size,	/* maxsize */
			       1,	/* nsegments */
			       size,	/* maxsegsize */
			       BUS_DMA_ALLOCNOW,	/* flags */
			       &dma->dma_tag);
	if (r != 0) {
		aprint_error_dev(dev,
		    "%s: ixgbe_dma_tag_create failed; error %d\n", __func__, r);
		goto fail_0;
	}

	r = bus_dmamem_alloc(dma->dma_tag->dt_dmat,
		size,
		dma->dma_tag->dt_alignment,
		dma->dma_tag->dt_boundary,
		&dma->dma_seg, 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(dev,
		    "%s: bus_dmamem_alloc failed; error %d\n", __func__, r);
		goto fail_1;
	}

	r = bus_dmamem_map(dma->dma_tag->dt_dmat, &dma->dma_seg, rsegs,
	    size, &dma->dma_vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamem_map failed; error %d\n",
		    __func__, r);
		goto fail_2;
	}

	r = ixgbe_dmamap_create(dma->dma_tag, 0, &dma->dma_map);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamem_map failed; error %d\n",
		    __func__, r);
		goto fail_3;
	}

	r = bus_dmamap_load(dma->dma_tag->dt_dmat, dma->dma_map, dma->dma_vaddr,
			    size,
			    NULL,
			    mapflags | BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamap_load failed; error %d\n",
		    __func__, r);
		goto fail_4;
	}
	dma->dma_paddr = dma->dma_map->dm_segs[0].ds_addr;
	dma->dma_size = size;
	return 0;
fail_4:
	ixgbe_dmamap_destroy(dma->dma_tag, dma->dma_map);
fail_3:
	bus_dmamem_unmap(dma->dma_tag->dt_dmat, dma->dma_vaddr, size);
fail_2:
	bus_dmamem_free(dma->dma_tag->dt_dmat, &dma->dma_seg, rsegs);
fail_1:
	ixgbe_dma_tag_destroy(dma->dma_tag);
fail_0:
	return r;
}

static void
ixgbe_dma_free(struct adapter *adapter, struct ixgbe_dma_alloc *dma)
{
	bus_dmamap_sync(dma->dma_tag->dt_dmat, dma->dma_map, 0, dma->dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	ixgbe_dmamap_unload(dma->dma_tag, dma->dma_map);
	bus_dmamem_free(dma->dma_tag->dt_dmat, &dma->dma_seg, 1);
	ixgbe_dma_tag_destroy(dma->dma_tag);
}


/*********************************************************************
 *
 *  Allocate memory for the transmit and receive rings, and then
 *  the descriptors associated with each, called only once at attach.
 *
 **********************************************************************/
static int
ixgbe_allocate_queues(struct adapter *adapter)
{
	device_t	dev = adapter->dev;
	struct ix_queue	*que;
	struct tx_ring	*txr;
	struct rx_ring	*rxr;
	int rsize, tsize, error = IXGBE_SUCCESS;
	int txconf = 0, rxconf = 0;

        /* First allocate the top level queue structs */
        if (!(adapter->queues =
            (struct ix_queue *) malloc(sizeof(struct ix_queue) *
            adapter->num_queues, M_DEVBUF, M_NOWAIT | M_ZERO))) {
                aprint_error_dev(dev, "Unable to allocate queue memory\n");
                error = ENOMEM;
                goto fail;
        }

	/* First allocate the TX ring struct memory */
	if (!(adapter->tx_rings =
	    (struct tx_ring *) malloc(sizeof(struct tx_ring) *
	    adapter->num_queues, M_DEVBUF, M_NOWAIT | M_ZERO))) {
		aprint_error_dev(dev, "Unable to allocate TX ring memory\n");
		error = ENOMEM;
		goto tx_fail;
	}

	/* Next allocate the RX */
	if (!(adapter->rx_rings =
	    (struct rx_ring *) malloc(sizeof(struct rx_ring) *
	    adapter->num_queues, M_DEVBUF, M_NOWAIT | M_ZERO))) {
		aprint_error_dev(dev, "Unable to allocate RX ring memory\n");
		error = ENOMEM;
		goto rx_fail;
	}

	/* For the ring itself */
	tsize = roundup2(adapter->num_tx_desc *
	    sizeof(union ixgbe_adv_tx_desc), DBA_ALIGN);

	/*
	 * Now set up the TX queues, txconf is needed to handle the
	 * possibility that things fail midcourse and we need to
	 * undo memory gracefully
	 */ 
	for (int i = 0; i < adapter->num_queues; i++, txconf++) {
		/* Set up some basics */
		txr = &adapter->tx_rings[i];
		txr->adapter = adapter;
		txr->me = i;

		/* Initialize the TX side lock */
		snprintf(txr->mtx_name, sizeof(txr->mtx_name), "%s:tx(%d)",
		    device_xname(dev), txr->me);
		mutex_init(&txr->tx_mtx, MUTEX_DEFAULT, IPL_NET);

		if (ixgbe_dma_malloc(adapter, tsize,
			&txr->txdma, BUS_DMA_NOWAIT)) {
			aprint_error_dev(dev,
			    "Unable to allocate TX Descriptor memory\n");
			error = ENOMEM;
			goto err_tx_desc;
		}
		txr->tx_base = (union ixgbe_adv_tx_desc *)txr->txdma.dma_vaddr;
		bzero((void *)txr->tx_base, tsize);

        	/* Now allocate transmit buffers for the ring */
        	if (ixgbe_allocate_transmit_buffers(txr)) {
			aprint_error_dev(dev,
			    "Critical Failure setting up transmit buffers\n");
			error = ENOMEM;
			goto err_tx_desc;
        	}
#if __FreeBSD_version >= 800000
		/* Allocate a buf ring */
		txr->br = buf_ring_alloc(IXGBE_BR_SIZE, M_DEVBUF,
		    M_WAITOK, &txr->tx_mtx);
		if (txr->br == NULL) {
			aprint_error_dev(dev,
			    "Critical Failure setting up buf ring\n");
			error = ENOMEM;
			goto err_tx_desc;
        	}
#endif
	}

	/*
	 * Next the RX queues...
	 */ 
	rsize = roundup2(adapter->num_rx_desc *
	    sizeof(union ixgbe_adv_rx_desc), DBA_ALIGN);
	for (int i = 0; i < adapter->num_queues; i++, rxconf++) {
		rxr = &adapter->rx_rings[i];
		/* Set up some basics */
		rxr->adapter = adapter;
		rxr->me = i;

		/* Initialize the RX side lock */
		snprintf(rxr->mtx_name, sizeof(rxr->mtx_name), "%s:rx(%d)",
		    device_xname(dev), rxr->me);
		mutex_init(&rxr->rx_mtx, MUTEX_DEFAULT, IPL_NET);

		if (ixgbe_dma_malloc(adapter, rsize,
			&rxr->rxdma, BUS_DMA_NOWAIT)) {
			aprint_error_dev(dev,
			    "Unable to allocate RxDescriptor memory\n");
			error = ENOMEM;
			goto err_rx_desc;
		}
		rxr->rx_base = (union ixgbe_adv_rx_desc *)rxr->rxdma.dma_vaddr;
		bzero((void *)rxr->rx_base, rsize);

        	/* Allocate receive buffers for the ring*/
		if (ixgbe_allocate_receive_buffers(rxr)) {
			aprint_error_dev(dev,
			    "Critical Failure setting up receive buffers\n");
			error = ENOMEM;
			goto err_rx_desc;
		}
	}

	/*
	** Finally set up the queue holding structs
	*/
	for (int i = 0; i < adapter->num_queues; i++) {
		que = &adapter->queues[i];
		que->adapter = adapter;
		que->txr = &adapter->tx_rings[i];
		que->rxr = &adapter->rx_rings[i];
	}

	return (0);

err_rx_desc:
	for (rxr = adapter->rx_rings; rxconf > 0; rxr++, rxconf--)
		ixgbe_dma_free(adapter, &rxr->rxdma);
err_tx_desc:
	for (txr = adapter->tx_rings; txconf > 0; txr++, txconf--)
		ixgbe_dma_free(adapter, &txr->txdma);
	free(adapter->rx_rings, M_DEVBUF);
rx_fail:
	free(adapter->tx_rings, M_DEVBUF);
tx_fail:
	free(adapter->queues, M_DEVBUF);
fail:
	return (error);
}

/*********************************************************************
 *
 *  Allocate memory for tx_buffer structures. The tx_buffer stores all
 *  the information needed to transmit a packet on the wire. This is
 *  called only once at attach, setup is done every reset.
 *
 **********************************************************************/
static int
ixgbe_allocate_transmit_buffers(struct tx_ring *txr)
{
	struct adapter *adapter = txr->adapter;
	device_t dev = adapter->dev;
	struct ixgbe_tx_buf *txbuf;
	int error, i;

	/*
	 * Setup DMA descriptor areas.
	 */
	if ((error = ixgbe_dma_tag_create(adapter->osdep.dmat,	/* parent */
			       1, 0,		/* alignment, bounds */
			       IXGBE_TSO_SIZE,		/* maxsize */
			       adapter->num_segs,	/* nsegments */
			       PAGE_SIZE,		/* maxsegsize */
			       0,			/* flags */
			       &txr->txtag))) {
		aprint_error_dev(dev,"Unable to allocate TX DMA tag\n");
		goto fail;
	}

	if (!(txr->tx_buffers =
	    (struct ixgbe_tx_buf *) malloc(sizeof(struct ixgbe_tx_buf) *
	    adapter->num_tx_desc, M_DEVBUF, M_NOWAIT | M_ZERO))) {
		aprint_error_dev(dev, "Unable to allocate tx_buffer memory\n");
		error = ENOMEM;
		goto fail;
	}

        /* Create the descriptor buffer dma maps */
	txbuf = txr->tx_buffers;
	for (i = 0; i < adapter->num_tx_desc; i++, txbuf++) {
		error = ixgbe_dmamap_create(txr->txtag, 0, &txbuf->map);
		if (error != 0) {
			aprint_error_dev(dev, "Unable to create TX DMA map\n");
			goto fail;
		}
	}

	return 0;
fail:
	/* We free all, it handles case where we are in the middle */
	ixgbe_free_transmit_structures(adapter);
	return (error);
}

/*********************************************************************
 *
 *  Initialize a transmit ring.
 *
 **********************************************************************/
static void
ixgbe_setup_transmit_ring(struct tx_ring *txr)
{
	struct adapter *adapter = txr->adapter;
	struct ixgbe_tx_buf *txbuf;
	int i;

	/* Clear the old ring contents */
	IXGBE_TX_LOCK(txr);
	bzero((void *)txr->tx_base,
	      (sizeof(union ixgbe_adv_tx_desc)) * adapter->num_tx_desc);
	/* Reset indices */
	txr->next_avail_desc = 0;
	txr->next_to_clean = 0;

	/* Free any existing tx buffers. */
        txbuf = txr->tx_buffers;
	for (i = 0; i < adapter->num_tx_desc; i++, txbuf++) {
		if (txbuf->m_head != NULL) {
			bus_dmamap_sync(txr->txtag->dt_dmat, txbuf->map,
			    0, txbuf->m_head->m_pkthdr.len,
			    BUS_DMASYNC_POSTWRITE);
			ixgbe_dmamap_unload(txr->txtag, txbuf->map);
			m_freem(txbuf->m_head);
			txbuf->m_head = NULL;
		}
		/* Clear the EOP index */
		txbuf->eop_index = -1;
        }

#ifdef IXGBE_FDIR
	/* Set the rate at which we sample packets */
	if (adapter->hw.mac.type != ixgbe_mac_82598EB)
		txr->atr_sample = atr_sample_rate;
#endif

	/* Set number of descriptors available */
	txr->tx_avail = adapter->num_tx_desc;

	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	IXGBE_TX_UNLOCK(txr);
}

/*********************************************************************
 *
 *  Initialize all transmit rings.
 *
 **********************************************************************/
static int
ixgbe_setup_transmit_structures(struct adapter *adapter)
{
	struct tx_ring *txr = adapter->tx_rings;

	for (int i = 0; i < adapter->num_queues; i++, txr++)
		ixgbe_setup_transmit_ring(txr);

	return (0);
}

/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
static void
ixgbe_initialize_transmit_units(struct adapter *adapter)
{
	struct tx_ring	*txr = adapter->tx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;

	/* Setup the Base and Length of the Tx Descriptor Ring */

	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		u64	tdba = txr->txdma.dma_paddr;
		u32	txctrl;

		IXGBE_WRITE_REG(hw, IXGBE_TDBAL(i),
		       (tdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_TDBAH(i), (tdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_TDLEN(i),
		    adapter->num_tx_desc * sizeof(struct ixgbe_legacy_tx_desc));

		/* Setup the HW Tx Head and Tail descriptor pointers */
		IXGBE_WRITE_REG(hw, IXGBE_TDH(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_TDT(i), 0);

		/* Setup Transmit Descriptor Cmd Settings */
		txr->txd_cmd = IXGBE_TXD_CMD_IFCS;
		txr->queue_status = IXGBE_QUEUE_IDLE;

		/* Disable Head Writeback */
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL(i));
			break;
		case ixgbe_mac_82599EB:
		default:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(i));
			break;
                }
		txctrl &= ~IXGBE_DCA_TXCTRL_TX_WB_RO_EN;
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL(i), txctrl);
			break;
		case ixgbe_mac_82599EB:
		default:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(i), txctrl);
			break;
		}

	}

	if (hw->mac.type != ixgbe_mac_82598EB) {
		u32 dmatxctl, rttdcs;
		dmatxctl = IXGBE_READ_REG(hw, IXGBE_DMATXCTL);
		dmatxctl |= IXGBE_DMATXCTL_TE;
		IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL, dmatxctl);
		/* Disable arbiter to set MTQC */
		rttdcs = IXGBE_READ_REG(hw, IXGBE_RTTDCS);
		rttdcs |= IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
		IXGBE_WRITE_REG(hw, IXGBE_MTQC, IXGBE_MTQC_64Q_1PB);
		rttdcs &= ~IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
	}

	return;
}

/*********************************************************************
 *
 *  Free all transmit rings.
 *
 **********************************************************************/
static void
ixgbe_free_transmit_structures(struct adapter *adapter)
{
	struct tx_ring *txr = adapter->tx_rings;

	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		IXGBE_TX_LOCK(txr);
		ixgbe_free_transmit_buffers(txr);
		ixgbe_dma_free(adapter, &txr->txdma);
		IXGBE_TX_UNLOCK(txr);
		IXGBE_TX_LOCK_DESTROY(txr);
	}
	free(adapter->tx_rings, M_DEVBUF);
}

/*********************************************************************
 *
 *  Free transmit ring related data structures.
 *
 **********************************************************************/
static void
ixgbe_free_transmit_buffers(struct tx_ring *txr)
{
	struct adapter *adapter = txr->adapter;
	struct ixgbe_tx_buf *tx_buffer;
	int             i;

	INIT_DEBUGOUT("free_transmit_ring: begin");

	if (txr->tx_buffers == NULL)
		return;

	tx_buffer = txr->tx_buffers;
	for (i = 0; i < adapter->num_tx_desc; i++, tx_buffer++) {
		if (tx_buffer->m_head != NULL) {
			bus_dmamap_sync(txr->txtag->dt_dmat, tx_buffer->map,
			    0, tx_buffer->m_head->m_pkthdr.len,
			    BUS_DMASYNC_POSTWRITE);
			ixgbe_dmamap_unload(txr->txtag, tx_buffer->map);
			m_freem(tx_buffer->m_head);
			tx_buffer->m_head = NULL;
			if (tx_buffer->map != NULL) {
				ixgbe_dmamap_destroy(txr->txtag,
				    tx_buffer->map);
				tx_buffer->map = NULL;
			}
		} else if (tx_buffer->map != NULL) {
			ixgbe_dmamap_unload(txr->txtag, tx_buffer->map);
			ixgbe_dmamap_destroy(txr->txtag, tx_buffer->map);
			tx_buffer->map = NULL;
		}
	}
#if __FreeBSD_version >= 800000
	if (txr->br != NULL)
		buf_ring_free(txr->br, M_DEVBUF);
#endif
	if (txr->tx_buffers != NULL) {
		free(txr->tx_buffers, M_DEVBUF);
		txr->tx_buffers = NULL;
	}
	if (txr->txtag != NULL) {
		ixgbe_dma_tag_destroy(txr->txtag);
		txr->txtag = NULL;
	}
	return;
}

/*********************************************************************
 *
 *  Advanced Context Descriptor setup for VLAN or L4 CSUM
 *
 **********************************************************************/

static u32
ixgbe_tx_ctx_setup(struct tx_ring *txr, struct mbuf *mp)
{
	struct m_tag *mtag;
	struct adapter *adapter = txr->adapter;
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_adv_tx_context_desc *TXD;
	struct ixgbe_tx_buf        *tx_buffer;
	u32 olinfo = 0, vlan_macip_lens = 0, type_tucmd_mlhl = 0;
	struct ether_vlan_header *eh;
	struct ip ip;
	struct ip6_hdr ip6;
	int  ehdrlen, ip_hlen = 0;
	u16	etype;
	u8	ipproto __diagused = 0;
	bool	offload;
	int ctxd = txr->next_avail_desc;
	u16 vtag = 0;

	offload = ((mp->m_pkthdr.csum_flags & M_CSUM_OFFLOAD) != 0);

	tx_buffer = &txr->tx_buffers[ctxd];
	TXD = (struct ixgbe_adv_tx_context_desc *) &txr->tx_base[ctxd];

	/*
	** In advanced descriptors the vlan tag must 
	** be placed into the descriptor itself.
	*/
	if ((mtag = VLAN_OUTPUT_TAG(ec, mp)) != NULL) {
		vtag = htole16(VLAN_TAG_VALUE(mtag) & 0xffff);
		vlan_macip_lens |= (vtag << IXGBE_ADVTXD_VLAN_SHIFT);
	} else if (!offload)
		return 0;

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present,
	 * helpful for QinQ too.
	 */
	KASSERT(mp->m_len >= offsetof(struct ether_vlan_header, evl_tag));
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		KASSERT(mp->m_len >= sizeof(struct ether_vlan_header));
		etype = ntohs(eh->evl_proto);
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else {
		etype = ntohs(eh->evl_encap_proto);
		ehdrlen = ETHER_HDR_LEN;
	}

	/* Set the ether header length */
	vlan_macip_lens |= ehdrlen << IXGBE_ADVTXD_MACLEN_SHIFT;

	switch (etype) {
	case ETHERTYPE_IP:
		m_copydata(mp, ehdrlen, sizeof(ip), &ip);
		ip_hlen = ip.ip_hl << 2;
		ipproto = ip.ip_p;
#if 0
		ip.ip_sum = 0;
		m_copyback(mp, ehdrlen, sizeof(ip), &ip);
#else
		KASSERT((mp->m_pkthdr.csum_flags & M_CSUM_IPv4) == 0 ||
		    ip.ip_sum == 0);
#endif
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
		break;
	case ETHERTYPE_IPV6:
		m_copydata(mp, ehdrlen, sizeof(ip6), &ip6);
		ip_hlen = sizeof(ip6);
		ipproto = ip6.ip6_nxt;
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV6;
		break;
	default:
		break;
	}

	if ((mp->m_pkthdr.csum_flags & M_CSUM_IPv4) != 0)
		olinfo |= IXGBE_TXD_POPTS_IXSM << 8;

	vlan_macip_lens |= ip_hlen;
	type_tucmd_mlhl |= IXGBE_ADVTXD_DCMD_DEXT | IXGBE_ADVTXD_DTYP_CTXT;

	if (mp->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_TCPv6)) {
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_TCP;
		olinfo |= IXGBE_TXD_POPTS_TXSM << 8;
		KASSERT(ipproto == IPPROTO_TCP);
	} else if (mp->m_pkthdr.csum_flags & (M_CSUM_UDPv4|M_CSUM_UDPv6)) {
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_UDP;
		olinfo |= IXGBE_TXD_POPTS_TXSM << 8;
		KASSERT(ipproto == IPPROTO_UDP);
	}

	/* Now copy bits into descriptor */
	TXD->vlan_macip_lens |= htole32(vlan_macip_lens);
	TXD->type_tucmd_mlhl |= htole32(type_tucmd_mlhl);
	TXD->seqnum_seed = htole32(0);
	TXD->mss_l4len_idx = htole32(0);

	tx_buffer->m_head = NULL;
	tx_buffer->eop_index = -1;

	/* We've consumed the first desc, adjust counters */
	if (++ctxd == adapter->num_tx_desc)
		ctxd = 0;
	txr->next_avail_desc = ctxd;
	--txr->tx_avail;

        return olinfo;
}

/**********************************************************************
 *
 *  Setup work for hardware segmentation offload (TSO) on
 *  adapters using advanced tx descriptors
 *
 **********************************************************************/
static bool
ixgbe_tso_setup(struct tx_ring *txr, struct mbuf *mp, u32 *paylen)
{
	struct m_tag *mtag;
	struct adapter *adapter = txr->adapter;
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_adv_tx_context_desc *TXD;
	struct ixgbe_tx_buf        *tx_buffer;
	u32 vlan_macip_lens = 0, type_tucmd_mlhl = 0;
	u32 mss_l4len_idx = 0;
	u16 vtag = 0;
	int ctxd, ehdrlen,  hdrlen, ip_hlen, tcp_hlen;
	struct ether_vlan_header *eh;
	struct ip *ip;
	struct tcphdr *th;


	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) 
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	else
		ehdrlen = ETHER_HDR_LEN;

        /* Ensure we have at least the IP+TCP header in the first mbuf. */
        if (mp->m_len < ehdrlen + sizeof(struct ip) + sizeof(struct tcphdr))
		return FALSE;

	ctxd = txr->next_avail_desc;
	tx_buffer = &txr->tx_buffers[ctxd];
	TXD = (struct ixgbe_adv_tx_context_desc *) &txr->tx_base[ctxd];

	ip = (struct ip *)(mp->m_data + ehdrlen);
	if (ip->ip_p != IPPROTO_TCP)
		return FALSE;   /* 0 */
	ip->ip_sum = 0;
	ip_hlen = ip->ip_hl << 2;
	th = (struct tcphdr *)((char *)ip + ip_hlen);
	/* XXX Educated guess: FreeBSD's in_pseudo == NetBSD's in_cksum_phdr */
	th->th_sum = in_cksum_phdr(ip->ip_src.s_addr,
	    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
	tcp_hlen = th->th_off << 2;
	hdrlen = ehdrlen + ip_hlen + tcp_hlen;

	/* This is used in the transmit desc in encap */
	*paylen = mp->m_pkthdr.len - hdrlen;

	/* VLAN MACLEN IPLEN */
	if ((mtag = VLAN_OUTPUT_TAG(ec, mp)) != NULL) {
		vtag = htole16(VLAN_TAG_VALUE(mtag) & 0xffff);
                vlan_macip_lens |= (vtag << IXGBE_ADVTXD_VLAN_SHIFT);
	}

	vlan_macip_lens |= ehdrlen << IXGBE_ADVTXD_MACLEN_SHIFT;
	vlan_macip_lens |= ip_hlen;
	TXD->vlan_macip_lens |= htole32(vlan_macip_lens);

	/* ADV DTYPE TUCMD */
	type_tucmd_mlhl |= IXGBE_ADVTXD_DCMD_DEXT | IXGBE_ADVTXD_DTYP_CTXT;
	type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_TCP;
	type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
	TXD->type_tucmd_mlhl |= htole32(type_tucmd_mlhl);


	/* MSS L4LEN IDX */
	mss_l4len_idx |= (mp->m_pkthdr.segsz << IXGBE_ADVTXD_MSS_SHIFT);
	mss_l4len_idx |= (tcp_hlen << IXGBE_ADVTXD_L4LEN_SHIFT);
	TXD->mss_l4len_idx = htole32(mss_l4len_idx);

	TXD->seqnum_seed = htole32(0);
	tx_buffer->m_head = NULL;
	tx_buffer->eop_index = -1;

	if (++ctxd == adapter->num_tx_desc)
		ctxd = 0;

	txr->tx_avail--;
	txr->next_avail_desc = ctxd;
	return TRUE;
}

#ifdef IXGBE_FDIR
/*
** This routine parses packet headers so that Flow
** Director can make a hashed filter table entry 
** allowing traffic flows to be identified and kept
** on the same cpu.  This would be a performance
** hit, but we only do it at IXGBE_FDIR_RATE of
** packets.
*/
static void
ixgbe_atr(struct tx_ring *txr, struct mbuf *mp)
{
	struct adapter			*adapter = txr->adapter;
	struct ix_queue			*que;
	struct ip			*ip;
	struct tcphdr			*th;
	struct udphdr			*uh;
	struct ether_vlan_header	*eh;
	union ixgbe_atr_hash_dword	input = {.dword = 0}; 
	union ixgbe_atr_hash_dword	common = {.dword = 0}; 
	int  				ehdrlen, ip_hlen;
	u16				etype;

	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		etype = eh->evl_proto;
	} else {
		ehdrlen = ETHER_HDR_LEN;
		etype = eh->evl_encap_proto;
	}

	/* Only handling IPv4 */
	if (etype != htons(ETHERTYPE_IP))
		return;

	ip = (struct ip *)(mp->m_data + ehdrlen);
	ip_hlen = ip->ip_hl << 2;

	/* check if we're UDP or TCP */
	switch (ip->ip_p) {
	case IPPROTO_TCP:
		th = (struct tcphdr *)((char *)ip + ip_hlen);
		/* src and dst are inverted */
		common.port.dst ^= th->th_sport;
		common.port.src ^= th->th_dport;
		input.formatted.flow_type ^= IXGBE_ATR_FLOW_TYPE_TCPV4;
		break;
	case IPPROTO_UDP:
		uh = (struct udphdr *)((char *)ip + ip_hlen);
		/* src and dst are inverted */
		common.port.dst ^= uh->uh_sport;
		common.port.src ^= uh->uh_dport;
		input.formatted.flow_type ^= IXGBE_ATR_FLOW_TYPE_UDPV4;
		break;
	default:
		return;
	}

	input.formatted.vlan_id = htobe16(mp->m_pkthdr.ether_vtag);
	if (mp->m_pkthdr.ether_vtag)
		common.flex_bytes ^= htons(ETHERTYPE_VLAN);
	else
		common.flex_bytes ^= etype;
	common.ip ^= ip->ip_src.s_addr ^ ip->ip_dst.s_addr;

	que = &adapter->queues[txr->me];
	/*
	** This assumes the Rx queue and Tx
	** queue are bound to the same CPU
	*/
	ixgbe_fdir_add_signature_filter_82599(&adapter->hw,
	    input, common, que->msix);
}
#endif /* IXGBE_FDIR */

/**********************************************************************
 *
 *  Examine each tx_buffer in the used queue. If the hardware is done
 *  processing the packet then free associated resources. The
 *  tx_buffer is put back on the free queue.
 *
 **********************************************************************/
static bool
ixgbe_txeof(struct tx_ring *txr)
{
	struct adapter	*adapter = txr->adapter;
	struct ifnet	*ifp = adapter->ifp;
	u32	first, last, done, processed;
	struct ixgbe_tx_buf *tx_buffer;
	struct ixgbe_legacy_tx_desc *tx_desc, *eop_desc;
	struct timeval now, elapsed;

	KASSERT(mutex_owned(&txr->tx_mtx));

	if (txr->tx_avail == adapter->num_tx_desc) {
		txr->queue_status = IXGBE_QUEUE_IDLE;
		return false;
	}

	processed = 0;
	first = txr->next_to_clean;
	tx_buffer = &txr->tx_buffers[first];
	/* For cleanup we just use legacy struct */
	tx_desc = (struct ixgbe_legacy_tx_desc *)&txr->tx_base[first];
	last = tx_buffer->eop_index;
	if (last == -1)
		return false;
	eop_desc = (struct ixgbe_legacy_tx_desc *)&txr->tx_base[last];

	/*
	** Get the index of the first descriptor
	** BEYOND the EOP and call that 'done'.
	** I do this so the comparison in the
	** inner while loop below can be simple
	*/
	if (++last == adapter->num_tx_desc) last = 0;
	done = last;

        ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_POSTREAD);
	/*
	** Only the EOP descriptor of a packet now has the DD
	** bit set, this is what we look for...
	*/
	while (eop_desc->upper.fields.status & IXGBE_TXD_STAT_DD) {
		/* We clean the range of the packet */
		while (first != done) {
			tx_desc->upper.data = 0;
			tx_desc->lower.data = 0;
			tx_desc->buffer_addr = 0;
			++txr->tx_avail;
			++processed;

			if (tx_buffer->m_head) {
				txr->bytes +=
				    tx_buffer->m_head->m_pkthdr.len;
				bus_dmamap_sync(txr->txtag->dt_dmat,
				    tx_buffer->map,
				    0, tx_buffer->m_head->m_pkthdr.len,
				    BUS_DMASYNC_POSTWRITE);
				ixgbe_dmamap_unload(txr->txtag, tx_buffer->map);
				m_freem(tx_buffer->m_head);
				tx_buffer->m_head = NULL;
			}
			tx_buffer->eop_index = -1;
			getmicrotime(&txr->watchdog_time);

			if (++first == adapter->num_tx_desc)
				first = 0;

			tx_buffer = &txr->tx_buffers[first];
			tx_desc =
			    (struct ixgbe_legacy_tx_desc *)&txr->tx_base[first];
		}
		++txr->packets;
		++ifp->if_opackets;
		/* See if there is more work now */
		last = tx_buffer->eop_index;
		if (last != -1) {
			eop_desc =
			    (struct ixgbe_legacy_tx_desc *)&txr->tx_base[last];
			/* Get next done point */
			if (++last == adapter->num_tx_desc) last = 0;
			done = last;
		} else
			break;
	}
	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	txr->next_to_clean = first;

	/*
	** Watchdog calculation, we know there's
	** work outstanding or the first return
	** would have been taken, so none processed
	** for too long indicates a hang.
	*/
	getmicrotime(&now);
	timersub(&now, &txr->watchdog_time, &elapsed);
	if (!processed && tvtohz(&elapsed) > IXGBE_WATCHDOG)
		txr->queue_status = IXGBE_QUEUE_HUNG;

	/*
	 * If we have enough room, clear IFF_OACTIVE to tell the stack that
	 * it is OK to send packets. If there are no pending descriptors,
	 * clear the timeout. Otherwise, if some descriptors have been freed,
	 * restart the timeout.
	 */
	if (txr->tx_avail > IXGBE_TX_CLEANUP_THRESHOLD) {
		ifp->if_flags &= ~IFF_OACTIVE;
		if (txr->tx_avail == adapter->num_tx_desc) {
			txr->queue_status = IXGBE_QUEUE_IDLE;
			return false;
		}
	}

	return true;
}

/*********************************************************************
 *
 *  Refresh mbuf buffers for RX descriptor rings
 *   - now keeps its own state so discards due to resource
 *     exhaustion are unnecessary, if an mbuf cannot be obtained
 *     it just returns, keeping its placeholder, thus it can simply
 *     be recalled to try again.
 *
 **********************************************************************/
static void
ixgbe_refresh_mbufs(struct rx_ring *rxr, int limit)
{
	struct adapter		*adapter = rxr->adapter;
	struct ixgbe_rx_buf	*rxbuf;
	struct mbuf		*mh, *mp;
	int			i, j, error;
	bool			refreshed = false;

	i = j = rxr->next_to_refresh;
	/* Control the loop with one beyond */
	if (++j == adapter->num_rx_desc)
		j = 0;

	while (j != limit) {
		rxbuf = &rxr->rx_buffers[i];
		if (rxr->hdr_split == FALSE)
			goto no_split;

		if (rxbuf->m_head == NULL) {
			mh = m_gethdr(M_DONTWAIT, MT_DATA);
			if (mh == NULL)
				goto update;
		} else
			mh = rxbuf->m_head;

		mh->m_pkthdr.len = mh->m_len = MHLEN;
		mh->m_len = MHLEN;
		mh->m_flags |= M_PKTHDR;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf(rxr->htag->dt_dmat,
		    rxbuf->hmap, mh, BUS_DMA_NOWAIT);
		if (error != 0) {
			printf("Refresh mbufs: hdr dmamap load"
			    " failure - %d\n", error);
			m_free(mh);
			rxbuf->m_head = NULL;
			goto update;
		}
		rxbuf->m_head = mh;
		ixgbe_dmamap_sync(rxr->htag, rxbuf->hmap, BUS_DMASYNC_PREREAD);
		rxr->rx_base[i].read.hdr_addr =
		    htole64(rxbuf->hmap->dm_segs[0].ds_addr);

no_split:
		if (rxbuf->m_pack == NULL) {
			mp = ixgbe_getjcl(&adapter->jcl_head, M_DONTWAIT,
			    MT_DATA, M_PKTHDR, adapter->rx_mbuf_sz);
			if (mp == NULL) {
				rxr->no_jmbuf.ev_count++;
				goto update;
			}
		} else
			mp = rxbuf->m_pack;

		mp->m_pkthdr.len = mp->m_len = adapter->rx_mbuf_sz;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf(rxr->ptag->dt_dmat,
		    rxbuf->pmap, mp, BUS_DMA_NOWAIT);
		if (error != 0) {
			printf("Refresh mbufs: payload dmamap load"
			    " failure - %d\n", error);
			m_free(mp);
			rxbuf->m_pack = NULL;
			goto update;
		}
		rxbuf->m_pack = mp;
		bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
		    0, mp->m_pkthdr.len, BUS_DMASYNC_PREREAD);
		rxr->rx_base[i].read.pkt_addr =
		    htole64(rxbuf->pmap->dm_segs[0].ds_addr);

		refreshed = true;
		/* Next is precalculated */
		i = j;
		rxr->next_to_refresh = i;
		if (++j == adapter->num_rx_desc)
			j = 0;
	}
update:
	if (refreshed) /* Update hardware tail index */
		IXGBE_WRITE_REG(&adapter->hw,
		    IXGBE_RDT(rxr->me), rxr->next_to_refresh);
	return;
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
ixgbe_allocate_receive_buffers(struct rx_ring *rxr)
{
	struct	adapter 	*adapter = rxr->adapter;
	device_t 		dev = adapter->dev;
	struct ixgbe_rx_buf 	*rxbuf;
	int             	i, bsize, error;

	bsize = sizeof(struct ixgbe_rx_buf) * adapter->num_rx_desc;
	if (!(rxr->rx_buffers =
	    (struct ixgbe_rx_buf *) malloc(bsize,
	    M_DEVBUF, M_NOWAIT | M_ZERO))) {
		aprint_error_dev(dev, "Unable to allocate rx_buffer memory\n");
		error = ENOMEM;
		goto fail;
	}

	if ((error = ixgbe_dma_tag_create(adapter->osdep.dmat,	/* parent */
				   1, 0,	/* alignment, bounds */
				   MSIZE,		/* maxsize */
				   1,			/* nsegments */
				   MSIZE,		/* maxsegsize */
				   0,			/* flags */
				   &rxr->htag))) {
		aprint_error_dev(dev, "Unable to create RX DMA tag\n");
		goto fail;
	}

	if ((error = ixgbe_dma_tag_create(adapter->osdep.dmat,	/* parent */
				   1, 0,	/* alignment, bounds */
				   MJUM16BYTES,		/* maxsize */
				   1,			/* nsegments */
				   MJUM16BYTES,		/* maxsegsize */
				   0,			/* flags */
				   &rxr->ptag))) {
		aprint_error_dev(dev, "Unable to create RX DMA tag\n");
		goto fail;
	}

	for (i = 0; i < adapter->num_rx_desc; i++, rxbuf++) {
		rxbuf = &rxr->rx_buffers[i];
		error = ixgbe_dmamap_create(rxr->htag,
		    BUS_DMA_NOWAIT, &rxbuf->hmap);
		if (error) {
			aprint_error_dev(dev, "Unable to create RX head map\n");
			goto fail;
		}
		error = ixgbe_dmamap_create(rxr->ptag,
		    BUS_DMA_NOWAIT, &rxbuf->pmap);
		if (error) {
			aprint_error_dev(dev, "Unable to create RX pkt map\n");
			goto fail;
		}
	}

	return (0);

fail:
	/* Frees all, but can handle partial completion */
	ixgbe_free_receive_structures(adapter);
	return (error);
}

/*
** Used to detect a descriptor that has
** been merged by Hardware RSC.
*/
static inline u32
ixgbe_rsc_count(union ixgbe_adv_rx_desc *rx)
{
	return (le32toh(rx->wb.lower.lo_dword.data) &
	    IXGBE_RXDADV_RSCCNT_MASK) >> IXGBE_RXDADV_RSCCNT_SHIFT;
}

/*********************************************************************
 *
 *  Initialize Hardware RSC (LRO) feature on 82599
 *  for an RX ring, this is toggled by the LRO capability
 *  even though it is transparent to the stack.
 *
 **********************************************************************/
static void
ixgbe_setup_hw_rsc(struct rx_ring *rxr)
{
	struct	adapter 	*adapter = rxr->adapter;
	struct	ixgbe_hw	*hw = &adapter->hw;
	u32			rscctrl, rdrxctl;

	rdrxctl = IXGBE_READ_REG(hw, IXGBE_RDRXCTL);
	rdrxctl &= ~IXGBE_RDRXCTL_RSCFRSTSIZE;
	rdrxctl |= IXGBE_RDRXCTL_CRCSTRIP;
	rdrxctl |= IXGBE_RDRXCTL_RSCACKC;
	IXGBE_WRITE_REG(hw, IXGBE_RDRXCTL, rdrxctl);

	rscctrl = IXGBE_READ_REG(hw, IXGBE_RSCCTL(rxr->me));
	rscctrl |= IXGBE_RSCCTL_RSCEN;
	/*
	** Limit the total number of descriptors that
	** can be combined, so it does not exceed 64K
	*/
	if (adapter->rx_mbuf_sz == MCLBYTES)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_16;
	else if (adapter->rx_mbuf_sz == MJUMPAGESIZE)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_8;
	else if (adapter->rx_mbuf_sz == MJUM9BYTES)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_4;
	else  /* Using 16K cluster */
		rscctrl |= IXGBE_RSCCTL_MAXDESC_1;

	IXGBE_WRITE_REG(hw, IXGBE_RSCCTL(rxr->me), rscctrl);

	/* Enable TCP header recognition */
	IXGBE_WRITE_REG(hw, IXGBE_PSRTYPE(0),
	    (IXGBE_READ_REG(hw, IXGBE_PSRTYPE(0)) |
	    IXGBE_PSRTYPE_TCPHDR));

	/* Disable RSC for ACK packets */
	IXGBE_WRITE_REG(hw, IXGBE_RSCDBU,
	    (IXGBE_RSCDBU_RSCACKDIS | IXGBE_READ_REG(hw, IXGBE_RSCDBU)));

	rxr->hw_rsc = TRUE;
}


static void     
ixgbe_free_receive_ring(struct rx_ring *rxr)
{ 
	struct  adapter         *adapter;
	struct ixgbe_rx_buf       *rxbuf;
	int i;

	adapter = rxr->adapter;
	for (i = 0; i < adapter->num_rx_desc; i++) {
		rxbuf = &rxr->rx_buffers[i];
		if (rxbuf->m_head != NULL) {
			ixgbe_dmamap_sync(rxr->htag, rxbuf->hmap,
			    BUS_DMASYNC_POSTREAD);
			ixgbe_dmamap_unload(rxr->htag, rxbuf->hmap);
			rxbuf->m_head->m_flags |= M_PKTHDR;
			m_freem(rxbuf->m_head);
		}
		if (rxbuf->m_pack != NULL) {
			bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
			    0, rxbuf->m_pack->m_pkthdr.len,
			    BUS_DMASYNC_POSTREAD);
			ixgbe_dmamap_unload(rxr->ptag, rxbuf->pmap);
			rxbuf->m_pack->m_flags |= M_PKTHDR;
			m_freem(rxbuf->m_pack);
		}
		rxbuf->m_head = NULL;
		rxbuf->m_pack = NULL;
	}
}


/*********************************************************************
 *
 *  Initialize a receive ring and its buffers.
 *
 **********************************************************************/
static int
ixgbe_setup_receive_ring(struct rx_ring *rxr)
{
	struct	adapter 	*adapter;
	struct ifnet		*ifp;
	struct ixgbe_rx_buf	*rxbuf;
#ifdef LRO
	struct lro_ctrl		*lro = &rxr->lro;
#endif /* LRO */
	int			rsize, error = 0;

	adapter = rxr->adapter;
	ifp = adapter->ifp;

	/* Clear the ring contents */
	IXGBE_RX_LOCK(rxr);
	rsize = roundup2(adapter->num_rx_desc *
	    sizeof(union ixgbe_adv_rx_desc), DBA_ALIGN);
	bzero((void *)rxr->rx_base, rsize);

	/* Free current RX buffer structs and their mbufs */
	ixgbe_free_receive_ring(rxr);

	/* Now reinitialize our supply of jumbo mbufs.  The number
	 * or size of jumbo mbufs may have changed.
	 */
	ixgbe_jcl_reinit(&adapter->jcl_head, rxr->ptag->dt_dmat,
	    2 * adapter->num_rx_desc, adapter->rx_mbuf_sz);

	/* Configure header split? */
	if (ixgbe_header_split)
		rxr->hdr_split = TRUE;

	/* Now replenish the mbufs */
	for (int j = 0; j != adapter->num_rx_desc; ++j) {
		struct mbuf	*mh, *mp;

		rxbuf = &rxr->rx_buffers[j];
		/*
		** Don't allocate mbufs if not
		** doing header split, its wasteful
		*/ 
		if (rxr->hdr_split == FALSE)
			goto skip_head;

		/* First the header */
		rxbuf->m_head = m_gethdr(M_DONTWAIT, MT_DATA);
		if (rxbuf->m_head == NULL) {
			error = ENOBUFS;
			goto fail;
		}
		m_adj(rxbuf->m_head, ETHER_ALIGN);
		mh = rxbuf->m_head;
		mh->m_len = mh->m_pkthdr.len = MHLEN;
		mh->m_flags |= M_PKTHDR;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf(rxr->htag->dt_dmat,
		    rxbuf->hmap, rxbuf->m_head, BUS_DMA_NOWAIT);
		if (error != 0) /* Nothing elegant to do here */
			goto fail;
		bus_dmamap_sync(rxr->htag->dt_dmat, rxbuf->hmap,
		    0, mh->m_pkthdr.len, BUS_DMASYNC_PREREAD);
		/* Update descriptor */
		rxr->rx_base[j].read.hdr_addr =
		    htole64(rxbuf->hmap->dm_segs[0].ds_addr);

skip_head:
		/* Now the payload cluster */
		rxbuf->m_pack = ixgbe_getjcl(&adapter->jcl_head, M_DONTWAIT,
		    MT_DATA, M_PKTHDR, adapter->rx_mbuf_sz);
		if (rxbuf->m_pack == NULL) {
			error = ENOBUFS;
                        goto fail;
		}
		mp = rxbuf->m_pack;
		mp->m_pkthdr.len = mp->m_len = adapter->rx_mbuf_sz;
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf(rxr->ptag->dt_dmat,
		    rxbuf->pmap, mp, BUS_DMA_NOWAIT);
		if (error != 0)
                        goto fail;
		bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
		    0, adapter->rx_mbuf_sz, BUS_DMASYNC_PREREAD);
		/* Update descriptor */
		rxr->rx_base[j].read.pkt_addr =
		    htole64(rxbuf->pmap->dm_segs[0].ds_addr);
	}


	/* Setup our descriptor indices */
	rxr->next_to_check = 0;
	rxr->next_to_refresh = 0;
	rxr->lro_enabled = FALSE;
	rxr->rx_split_packets.ev_count = 0;
	rxr->rx_bytes.ev_count = 0;
	rxr->discard = FALSE;

	ixgbe_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	** Now set up the LRO interface:
	** 82598 uses software LRO, the
	** 82599 uses a hardware assist.
	*/
	if ((adapter->hw.mac.type != ixgbe_mac_82598EB) &&
	    (ifp->if_capenable & IFCAP_RXCSUM) &&
	    (ifp->if_capenable & IFCAP_LRO))
		ixgbe_setup_hw_rsc(rxr);
#ifdef LRO
	else if (ifp->if_capenable & IFCAP_LRO) {
		device_t dev = adapter->dev;
		int err = tcp_lro_init(lro);
		if (err) {
			device_printf(dev, "LRO Initialization failed!\n");
			goto fail;
		}
		INIT_DEBUGOUT("RX Soft LRO Initialized\n");
		rxr->lro_enabled = TRUE;
		lro->ifp = adapter->ifp;
	}
#endif /* LRO */

	IXGBE_RX_UNLOCK(rxr);
	return (0);

fail:
	ixgbe_free_receive_ring(rxr);
	IXGBE_RX_UNLOCK(rxr);
	return (error);
}

/*********************************************************************
 *
 *  Initialize all receive rings.
 *
 **********************************************************************/
static int
ixgbe_setup_receive_structures(struct adapter *adapter)
{
	struct rx_ring *rxr = adapter->rx_rings;
	int j;

	for (j = 0; j < adapter->num_queues; j++, rxr++)
		if (ixgbe_setup_receive_ring(rxr))
			goto fail;

	return (0);
fail:
	/*
	 * Free RX buffers allocated so far, we will only handle
	 * the rings that completed, the failing case will have
	 * cleaned up for itself. 'j' failed, so its the terminus.
	 */
	for (int i = 0; i < j; ++i) {
		rxr = &adapter->rx_rings[i];
		ixgbe_free_receive_ring(rxr);
	}

	return (ENOBUFS);
}

/*********************************************************************
 *
 *  Setup receive registers and features.
 *
 **********************************************************************/
#define IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT 2

static void
ixgbe_initialize_receive_units(struct adapter *adapter)
{
	int i;
	struct	rx_ring	*rxr = adapter->rx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet   *ifp = adapter->ifp;
	u32		bufsz, rxctrl, fctrl, srrctl, rxcsum;
	u32		reta, mrqc = 0, hlreg, r[10];


	/*
	 * Make sure receives are disabled while
	 * setting up the descriptor ring
	 */
	rxctrl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL,
	    rxctrl & ~IXGBE_RXCTRL_RXEN);

	/* Enable broadcasts */
	fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
	fctrl |= IXGBE_FCTRL_BAM;
	fctrl |= IXGBE_FCTRL_DPF;
	fctrl |= IXGBE_FCTRL_PMCF;
	IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);

	/* Set for Jumbo Frames? */
	hlreg = IXGBE_READ_REG(hw, IXGBE_HLREG0);
	if (ifp->if_mtu > ETHERMTU)
		hlreg |= IXGBE_HLREG0_JUMBOEN;
	else
		hlreg &= ~IXGBE_HLREG0_JUMBOEN;
	IXGBE_WRITE_REG(hw, IXGBE_HLREG0, hlreg);

	bufsz = adapter->rx_mbuf_sz  >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;

	for (i = 0; i < adapter->num_queues; i++, rxr++) {
		u64 rdba = rxr->rxdma.dma_paddr;

		/* Setup the Base and Length of the Rx Descriptor Ring */
		IXGBE_WRITE_REG(hw, IXGBE_RDBAL(i),
			       (rdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_RDBAH(i), (rdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_RDLEN(i),
		    adapter->num_rx_desc * sizeof(union ixgbe_adv_rx_desc));

		/* Set up the SRRCTL register */
		srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(i));
		srrctl &= ~IXGBE_SRRCTL_BSIZEHDR_MASK;
		srrctl &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;
		srrctl |= bufsz;
		if (rxr->hdr_split) {
			/* Use a standard mbuf for the header */
			srrctl |= ((IXGBE_RX_HDR <<
			    IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT)
			    & IXGBE_SRRCTL_BSIZEHDR_MASK);
			srrctl |= IXGBE_SRRCTL_DESCTYPE_HDR_SPLIT_ALWAYS;
		} else
			srrctl |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;
		IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(i), srrctl);

		/* Setup the HW Rx Head and Tail Descriptor Pointers */
		IXGBE_WRITE_REG(hw, IXGBE_RDH(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RDT(i), 0);
	}

	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
		u32 psrtype = IXGBE_PSRTYPE_TCPHDR |
			      IXGBE_PSRTYPE_UDPHDR |
			      IXGBE_PSRTYPE_IPV4HDR |
			      IXGBE_PSRTYPE_IPV6HDR;
		IXGBE_WRITE_REG(hw, IXGBE_PSRTYPE(0), psrtype);
	}

	rxcsum = IXGBE_READ_REG(hw, IXGBE_RXCSUM);

	/* Setup RSS */
	if (adapter->num_queues > 1) {
		int j;
		reta = 0;

		/* set up random bits */
		cprng_fast(&r, sizeof(r));

		/* Set up the redirection table */
		for (i = 0, j = 0; i < 128; i++, j++) {
			if (j == adapter->num_queues) j = 0;
			reta = (reta << 8) | (j * 0x11);
			if ((i & 3) == 3)
				IXGBE_WRITE_REG(hw, IXGBE_RETA(i >> 2), reta);
		}

		/* Now fill our hash function seeds */
		for (i = 0; i < 10; i++)
			IXGBE_WRITE_REG(hw, IXGBE_RSSRK(i), r[i]);

		/* Perform hash on these packet types */
		mrqc = IXGBE_MRQC_RSSEN
		     | IXGBE_MRQC_RSS_FIELD_IPV4
		     | IXGBE_MRQC_RSS_FIELD_IPV4_TCP
		     | IXGBE_MRQC_RSS_FIELD_IPV4_UDP
		     | IXGBE_MRQC_RSS_FIELD_IPV6_EX_TCP
		     | IXGBE_MRQC_RSS_FIELD_IPV6_EX
		     | IXGBE_MRQC_RSS_FIELD_IPV6
		     | IXGBE_MRQC_RSS_FIELD_IPV6_TCP
		     | IXGBE_MRQC_RSS_FIELD_IPV6_UDP
		     | IXGBE_MRQC_RSS_FIELD_IPV6_EX_UDP;
		IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);

		/* RSS and RX IPP Checksum are mutually exclusive */
		rxcsum |= IXGBE_RXCSUM_PCSD;
	}

	if (ifp->if_capenable & IFCAP_RXCSUM)
		rxcsum |= IXGBE_RXCSUM_PCSD;

	if (!(rxcsum & IXGBE_RXCSUM_PCSD))
		rxcsum |= IXGBE_RXCSUM_IPPCSE;

	IXGBE_WRITE_REG(hw, IXGBE_RXCSUM, rxcsum);

	return;
}

/*********************************************************************
 *
 *  Free all receive rings.
 *
 **********************************************************************/
static void
ixgbe_free_receive_structures(struct adapter *adapter)
{
	struct rx_ring *rxr = adapter->rx_rings;

	for (int i = 0; i < adapter->num_queues; i++, rxr++) {
#ifdef LRO
		struct lro_ctrl		*lro = &rxr->lro;
#endif /* LRO */
		ixgbe_free_receive_buffers(rxr);
#ifdef LRO
		/* Free LRO memory */
		tcp_lro_free(lro);
#endif /* LRO */
		/* Free the ring memory as well */
		ixgbe_dma_free(adapter, &rxr->rxdma);
	}

	free(adapter->rx_rings, M_DEVBUF);
}


/*********************************************************************
 *
 *  Free receive ring data structures
 *
 **********************************************************************/
static void
ixgbe_free_receive_buffers(struct rx_ring *rxr)
{
	struct adapter		*adapter = rxr->adapter;
	struct ixgbe_rx_buf	*rxbuf;

	INIT_DEBUGOUT("free_receive_structures: begin");

	/* Cleanup any existing buffers */
	if (rxr->rx_buffers != NULL) {
		for (int i = 0; i < adapter->num_rx_desc; i++) {
			rxbuf = &rxr->rx_buffers[i];
			if (rxbuf->m_head != NULL) {
				ixgbe_dmamap_sync(rxr->htag, rxbuf->hmap,
				    BUS_DMASYNC_POSTREAD);
				ixgbe_dmamap_unload(rxr->htag, rxbuf->hmap);
				rxbuf->m_head->m_flags |= M_PKTHDR;
				m_freem(rxbuf->m_head);
			}
			if (rxbuf->m_pack != NULL) {
				bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
				    0, rxbuf->m_pack->m_pkthdr.len,
				    BUS_DMASYNC_POSTREAD);
				ixgbe_dmamap_unload(rxr->ptag, rxbuf->pmap);
				rxbuf->m_pack->m_flags |= M_PKTHDR;
				m_freem(rxbuf->m_pack);
			}
			rxbuf->m_head = NULL;
			rxbuf->m_pack = NULL;
			if (rxbuf->hmap != NULL) {
				ixgbe_dmamap_destroy(rxr->htag, rxbuf->hmap);
				rxbuf->hmap = NULL;
			}
			if (rxbuf->pmap != NULL) {
				ixgbe_dmamap_destroy(rxr->ptag, rxbuf->pmap);
				rxbuf->pmap = NULL;
			}
		}
		if (rxr->rx_buffers != NULL) {
			free(rxr->rx_buffers, M_DEVBUF);
			rxr->rx_buffers = NULL;
		}
	}

	if (rxr->htag != NULL) {
		ixgbe_dma_tag_destroy(rxr->htag);
		rxr->htag = NULL;
	}
	if (rxr->ptag != NULL) {
		ixgbe_dma_tag_destroy(rxr->ptag);
		rxr->ptag = NULL;
	}

	return;
}

static __inline void
ixgbe_rx_input(struct rx_ring *rxr, struct ifnet *ifp, struct mbuf *m, u32 ptype)
{
	int s;

#ifdef LRO
	struct adapter	*adapter = ifp->if_softc;
	struct ethercom *ec = &adapter->osdep.ec;

        /*
         * ATM LRO is only for IPv4/TCP packets and TCP checksum of the packet
         * should be computed by hardware. Also it should not have VLAN tag in
         * ethernet header.
         */
        if (rxr->lro_enabled &&
            (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING) != 0 &&
            (ptype & IXGBE_RXDADV_PKTTYPE_ETQF) == 0 &&
            (ptype & (IXGBE_RXDADV_PKTTYPE_IPV4 | IXGBE_RXDADV_PKTTYPE_TCP)) ==
            (IXGBE_RXDADV_PKTTYPE_IPV4 | IXGBE_RXDADV_PKTTYPE_TCP) &&
            (m->m_pkthdr.csum_flags & (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) ==
            (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) {
                /*
                 * Send to the stack if:
                 **  - LRO not enabled, or
                 **  - no LRO resources, or
                 **  - lro enqueue fails
                 */
                if (rxr->lro.lro_cnt != 0)
                        if (tcp_lro_rx(&rxr->lro, m, 0) == 0)
                                return;
        }
#endif /* LRO */

	IXGBE_RX_UNLOCK(rxr);

	s = splnet();
	/* Pass this up to any BPF listeners. */
	bpf_mtap(ifp, m);
	(*ifp->if_input)(ifp, m);
	splx(s);

	IXGBE_RX_LOCK(rxr);
}

static __inline void
ixgbe_rx_discard(struct rx_ring *rxr, int i)
{
	struct ixgbe_rx_buf	*rbuf;

	rbuf = &rxr->rx_buffers[i];

        if (rbuf->fmp != NULL) {/* Partial chain ? */
		rbuf->fmp->m_flags |= M_PKTHDR;
                m_freem(rbuf->fmp);
                rbuf->fmp = NULL;
	}

	/*
	** With advanced descriptors the writeback
	** clobbers the buffer addrs, so its easier
	** to just free the existing mbufs and take
	** the normal refresh path to get new buffers
	** and mapping.
	*/
	if (rbuf->m_head) {
		m_free(rbuf->m_head);
		rbuf->m_head = NULL;
	}
 
	if (rbuf->m_pack) {
		m_free(rbuf->m_pack);
		rbuf->m_pack = NULL;
	}

	return;
}


/*********************************************************************
 *
 *  This routine executes in interrupt context. It replenishes
 *  the mbufs in the descriptor and sends data which has been
 *  dma'ed into host memory to upper layer.
 *
 *  We loop at most count times if count is > 0, or until done if
 *  count < 0.
 *
 *  Return TRUE for more work, FALSE for all clean.
 *********************************************************************/
static bool
ixgbe_rxeof(struct ix_queue *que, int count)
{
	struct adapter		*adapter = que->adapter;
	struct rx_ring		*rxr = que->rxr;
	struct ifnet		*ifp = adapter->ifp;
#ifdef LRO
	struct lro_ctrl		*lro = &rxr->lro;
	struct lro_entry	*queued;
#endif /* LRO */
	int			i, nextp, processed = 0;
	u32			staterr = 0;
	union ixgbe_adv_rx_desc	*cur;
	struct ixgbe_rx_buf	*rbuf, *nbuf;

	IXGBE_RX_LOCK(rxr);

	for (i = rxr->next_to_check; count != 0;) {
		struct mbuf	*sendmp, *mh, *mp;
		u32		rsc, ptype;
		u16		hlen, plen, hdr, vtag;
		bool		eop;
 
		/* Sync the ring. */
		ixgbe_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur = &rxr->rx_base[i];
		staterr = le32toh(cur->wb.upper.status_error);

		if ((staterr & IXGBE_RXD_STAT_DD) == 0)
			break;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			break;

		count--;
		sendmp = NULL;
		nbuf = NULL;
		rsc = 0;
		cur->wb.upper.status_error = 0;
		rbuf = &rxr->rx_buffers[i];
		mh = rbuf->m_head;
		mp = rbuf->m_pack;

		plen = le16toh(cur->wb.upper.length);
		ptype = le32toh(cur->wb.lower.lo_dword.data) &
		    IXGBE_RXDADV_PKTTYPE_MASK;
		hdr = le16toh(cur->wb.lower.lo_dword.hs_rss.hdr_info);
		vtag = le16toh(cur->wb.upper.vlan);
		eop = ((staterr & IXGBE_RXD_STAT_EOP) != 0);

		/* Make sure bad packets are discarded */
		if (((staterr & IXGBE_RXDADV_ERR_FRAME_ERR_MASK) != 0) ||
		    (rxr->discard)) {
			ifp->if_ierrors++;
			rxr->rx_discarded.ev_count++;
			if (eop)
				rxr->discard = FALSE;
			else
				rxr->discard = TRUE;
			ixgbe_rx_discard(rxr, i);
			goto next_desc;
		}

		/*
		** On 82599 which supports a hardware
		** LRO (called HW RSC), packets need
		** not be fragmented across sequential
		** descriptors, rather the next descriptor
		** is indicated in bits of the descriptor.
		** This also means that we might proceses
		** more than one packet at a time, something
		** that has never been true before, it
		** required eliminating global chain pointers
		** in favor of what we are doing here.  -jfv
		*/
		if (!eop) {
			/*
			** Figure out the next descriptor
			** of this frame.
			*/
			if (rxr->hw_rsc == TRUE) {
				rsc = ixgbe_rsc_count(cur);
				rxr->rsc_num += (rsc - 1);
			}
			if (rsc) { /* Get hardware index */
				nextp = ((staterr &
				    IXGBE_RXDADV_NEXTP_MASK) >>
				    IXGBE_RXDADV_NEXTP_SHIFT);
			} else { /* Just sequential */
				nextp = i + 1;
				if (nextp == adapter->num_rx_desc)
					nextp = 0;
			}
			nbuf = &rxr->rx_buffers[nextp];
			prefetch(nbuf);
		}
		/*
		** The header mbuf is ONLY used when header 
		** split is enabled, otherwise we get normal 
		** behavior, ie, both header and payload
		** are DMA'd into the payload buffer.
		**
		** Rather than using the fmp/lmp global pointers
		** we now keep the head of a packet chain in the
		** buffer struct and pass this along from one
		** descriptor to the next, until we get EOP.
		*/
		if (rxr->hdr_split && (rbuf->fmp == NULL)) {
			/* This must be an initial descriptor */
			hlen = (hdr & IXGBE_RXDADV_HDRBUFLEN_MASK) >>
			    IXGBE_RXDADV_HDRBUFLEN_SHIFT;
			if (hlen > IXGBE_RX_HDR)
				hlen = IXGBE_RX_HDR;
			mh->m_len = hlen;
			mh->m_flags |= M_PKTHDR;
			mh->m_next = NULL;
			mh->m_pkthdr.len = mh->m_len;
			/* Null buf pointer so it is refreshed */
			rbuf->m_head = NULL;
			/*
			** Check the payload length, this
			** could be zero if its a small
			** packet.
			*/
			if (plen > 0) {
				mp->m_len = plen;
				mp->m_next = NULL;
				mp->m_flags &= ~M_PKTHDR;
				mh->m_next = mp;
				mh->m_pkthdr.len += mp->m_len;
				/* Null buf pointer so it is refreshed */
				rbuf->m_pack = NULL;
				rxr->rx_split_packets.ev_count++;
			}
			/*
			** Now create the forward
			** chain so when complete 
			** we wont have to.
			*/
                        if (eop == 0) {
				/* stash the chain head */
                                nbuf->fmp = mh;
				/* Make forward chain */
                                if (plen)
                                        mp->m_next = nbuf->m_pack;
                                else
                                        mh->m_next = nbuf->m_pack;
                        } else {
				/* Singlet, prepare to send */
                                sendmp = mh;
                                if (VLAN_ATTACHED(&adapter->osdep.ec) &&
				  (staterr & IXGBE_RXD_STAT_VP)) {
					/* XXX Do something reasonable on
					 * error.
					 */
#if 0
					printf("%s.%d: VLAN_INPUT_TAG\n",
					    __func__, __LINE__);
					Debugger();
#endif
					VLAN_INPUT_TAG(ifp, sendmp, vtag,
					    printf("%s: could not apply VLAN "
					        "tag", __func__));
                                }
                        }
		} else {
			/*
			** Either no header split, or a
			** secondary piece of a fragmented
			** split packet.
			*/
			mp->m_len = plen;
			/*
			** See if there is a stored head
			** that determines what we are
			*/
			sendmp = rbuf->fmp;
			rbuf->m_pack = rbuf->fmp = NULL;

			if (sendmp != NULL) /* secondary frag */
				sendmp->m_pkthdr.len += mp->m_len;
			else {
				/* first desc of a non-ps chain */
				sendmp = mp;
				sendmp->m_flags |= M_PKTHDR;
				sendmp->m_pkthdr.len = mp->m_len;
				if (staterr & IXGBE_RXD_STAT_VP) {
					/* XXX Do something reasonable on
					 * error.
					 */
#if 0
					printf("%s.%d: VLAN_INPUT_TAG\n",
					    __func__, __LINE__);
					Debugger();
#endif
					VLAN_INPUT_TAG(ifp, sendmp, vtag,
					    printf("%s: could not apply VLAN "
					        "tag", __func__));
				}
                        }
			/* Pass the head pointer on */
			if (eop == 0) {
				nbuf->fmp = sendmp;
				sendmp = NULL;
				mp->m_next = nbuf->m_pack;
			}
		}
		++processed;
		/* Sending this frame? */
		if (eop) {
			sendmp->m_pkthdr.rcvif = ifp;
			ifp->if_ipackets++;
			rxr->rx_packets.ev_count++;
			/* capture data for AIM */
			rxr->bytes += sendmp->m_pkthdr.len;
			rxr->rx_bytes.ev_count += sendmp->m_pkthdr.len;
			if ((ifp->if_capenable & IFCAP_RXCSUM) != 0) {
				ixgbe_rx_checksum(staterr, sendmp, ptype,
				   &adapter->stats);
			}
#if __FreeBSD_version >= 800000
			sendmp->m_pkthdr.flowid = que->msix;
			sendmp->m_flags |= M_FLOWID;
#endif
		}
next_desc:
		ixgbe_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Advance our pointers to the next descriptor. */
		if (++i == adapter->num_rx_desc)
			i = 0;

		/* Now send to the stack or do LRO */
		if (sendmp != NULL) {
			rxr->next_to_check = i;
			ixgbe_rx_input(rxr, ifp, sendmp, ptype);
			i = rxr->next_to_check;
		}

               /* Every 8 descriptors we go to refresh mbufs */
		if (processed == 8) {
			ixgbe_refresh_mbufs(rxr, i);
			processed = 0;
		}
	}

	/* Refresh any remaining buf structs */
	if (ixgbe_rx_unrefreshed(rxr))
		ixgbe_refresh_mbufs(rxr, i);

	rxr->next_to_check = i;

#ifdef LRO
	/*
	 * Flush any outstanding LRO work
	 */
	while ((queued = SLIST_FIRST(&lro->lro_active)) != NULL) {
		SLIST_REMOVE_HEAD(&lro->lro_active, next);
		tcp_lro_flush(lro, queued);
	}
#endif /* LRO */

	IXGBE_RX_UNLOCK(rxr);

	/*
	** We still have cleaning to do?
	** Schedule another interrupt if so.
	*/
	if ((staterr & IXGBE_RXD_STAT_DD) != 0) {
		ixgbe_rearm_queues(adapter, (u64)(1ULL << que->msix));
		return true;
	}

	return false;
}


/*********************************************************************
 *
 *  Verify that the hardware indicated that the checksum is valid.
 *  Inform the stack about the status of checksum so that stack
 *  doesn't spend time verifying the checksum.
 *
 *********************************************************************/
static void
ixgbe_rx_checksum(u32 staterr, struct mbuf * mp, u32 ptype,
    struct ixgbe_hw_stats *stats)
{
	u16	status = (u16) staterr;
	u8	errors = (u8) (staterr >> 24);
#if 0
	bool	sctp = FALSE;

	if ((ptype & IXGBE_RXDADV_PKTTYPE_ETQF) == 0 &&
	    (ptype & IXGBE_RXDADV_PKTTYPE_SCTP) != 0)
		sctp = TRUE;
#endif

	if (status & IXGBE_RXD_STAT_IPCS) {
		stats->ipcs.ev_count++;
		if (!(errors & IXGBE_RXD_ERR_IPE)) {
			/* IP Checksum Good */
			mp->m_pkthdr.csum_flags = M_CSUM_IPv4;

		} else {
			stats->ipcs_bad.ev_count++;
			mp->m_pkthdr.csum_flags = M_CSUM_IPv4|M_CSUM_IPv4_BAD;
		}
	}
	if (status & IXGBE_RXD_STAT_L4CS) {
		stats->l4cs.ev_count++;
		u16 type = M_CSUM_TCPv4|M_CSUM_TCPv6|M_CSUM_UDPv4|M_CSUM_UDPv6;
		if (!(errors & IXGBE_RXD_ERR_TCPE)) {
			mp->m_pkthdr.csum_flags |= type;
		} else {
			stats->l4cs_bad.ev_count++;
			mp->m_pkthdr.csum_flags |= type | M_CSUM_TCP_UDP_BAD;
		}
	}
	return;
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
ixgbe_register_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct adapter	*adapter = ifp->if_softc;
	u16		index, bit;

	if (ifp->if_softc !=  arg)   /* Not our event */
		return;

	if ((vtag == 0) || (vtag > 4095))	/* Invalid */
		return;

	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	adapter->shadow_vfta[index] |= (1 << bit);
	ixgbe_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/*
** This routine is run via an vlan
** unconfig EVENT, remove our entry
** in the soft vfta.
*/
static void
ixgbe_unregister_vlan(void *arg, struct ifnet *ifp, u16 vtag)
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
	adapter->shadow_vfta[index] &= ~(1 << bit);
	/* Re-init to load the changes */
	ixgbe_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}
#endif

static void
ixgbe_setup_vlan_hw_support(struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		ctrl;

	/*
	** We get here thru init_locked, meaning
	** a soft reset, this has already cleared
	** the VFTA and other state, so if there
	** have been no vlan's registered do nothing.
	*/
	if (!VLAN_ATTACHED(&adapter->osdep.ec)) {
		return;
	}

	/*
	** A soft reset zero's out the VFTA, so
	** we need to repopulate it now.
	*/
	for (int i = 0; i < IXGBE_VFTA_SIZE; i++)
		if (adapter->shadow_vfta[i] != 0)
			IXGBE_WRITE_REG(hw, IXGBE_VFTA(i),
			    adapter->shadow_vfta[i]);

	ctrl = IXGBE_READ_REG(hw, IXGBE_VLNCTRL);
	/* Enable the Filter Table if enabled */
	if (ec->ec_capenable & ETHERCAP_VLAN_HWFILTER) {
		ctrl &= ~IXGBE_VLNCTRL_CFIEN;
		ctrl |= IXGBE_VLNCTRL_VFE;
	}
	if (hw->mac.type == ixgbe_mac_82598EB)
		ctrl |= IXGBE_VLNCTRL_VME;
	IXGBE_WRITE_REG(hw, IXGBE_VLNCTRL, ctrl);

	/* On 82599 the VLAN enable is per/queue in RXDCTL */
	if (hw->mac.type != ixgbe_mac_82598EB)
		for (int i = 0; i < adapter->num_queues; i++) {
			ctrl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(i));
				ctrl |= IXGBE_RXDCTL_VME;
			IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(i), ctrl);
		}
}

static void
ixgbe_enable_intr(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = adapter->queues;
	u32 mask = (IXGBE_EIMS_ENABLE_MASK & ~IXGBE_EIMS_RTX_QUEUE);


	/* Enable Fan Failure detection */
	if (hw->device_id == IXGBE_DEV_ID_82598AT)
		    mask |= IXGBE_EIMS_GPI_SDP1;
	else {
		    mask |= IXGBE_EIMS_ECC;
		    mask |= IXGBE_EIMS_GPI_SDP1;
		    mask |= IXGBE_EIMS_GPI_SDP2;
#ifdef IXGBE_FDIR
		    mask |= IXGBE_EIMS_FLOW_DIR;
#endif
	}

	IXGBE_WRITE_REG(hw, IXGBE_EIMS, mask);

	/* With RSS we use auto clear */
	if (adapter->msix_mem) {
		mask = IXGBE_EIMS_ENABLE_MASK;
		/* Don't autoclear Link */
		mask &= ~IXGBE_EIMS_OTHER;
		mask &= ~IXGBE_EIMS_LSC;
		IXGBE_WRITE_REG(hw, IXGBE_EIAC, mask);
	}

	/*
	** Now enable all queues, this is done separately to
	** allow for handling the extended (beyond 32) MSIX
	** vectors that can be used by 82599
	*/
        for (int i = 0; i < adapter->num_queues; i++, que++)
                ixgbe_enable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(hw);

	return;
}

static void
ixgbe_disable_intr(struct adapter *adapter)
{
	if (adapter->msix_mem)
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIAC, 0);
	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, ~0);
	} else {
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, 0xFFFF0000);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(0), ~0);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(1), ~0);
	}
	IXGBE_WRITE_FLUSH(&adapter->hw);
	return;
}

u16
ixgbe_read_pci_cfg(struct ixgbe_hw *hw, u32 reg)
{
	switch (reg % 4) {
	case 0:
		return pci_conf_read(hw->back->pc, hw->back->tag, reg) &
		    __BITS(15, 0);
	case 2:
		return __SHIFTOUT(pci_conf_read(hw->back->pc, hw->back->tag,
		    reg - 2), __BITS(31, 16));
	default:
		panic("%s: invalid register (%" PRIx32, __func__, reg); 
		break;
	}
}

void
ixgbe_write_pci_cfg(struct ixgbe_hw *hw, u32 reg, u16 value)
{
	pcireg_t old;

	switch (reg % 4) {
	case 0:
		old = pci_conf_read(hw->back->pc, hw->back->tag, reg) &
		    __BITS(31, 16);
		pci_conf_write(hw->back->pc, hw->back->tag, reg, value | old);
		break;
	case 2:
		old = pci_conf_read(hw->back->pc, hw->back->tag, reg - 2) &
		    __BITS(15, 0);
		pci_conf_write(hw->back->pc, hw->back->tag, reg - 2,
		    __SHIFTIN(value, __BITS(31, 16)) | old);
		break;
	default:
		panic("%s: invalid register (%" PRIx32, __func__, reg); 
		break;
	}

	return;
}

/*
** Setup the correct IVAR register for a particular MSIX interrupt
**   (yes this is all very magic and confusing :)
**  - entry is the register array entry
**  - vector is the MSIX vector for this queue
**  - type is RX/TX/MISC
*/
static void
ixgbe_set_ivar(struct adapter *adapter, u8 entry, u8 vector, s8 type)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 ivar, index;

	vector |= IXGBE_IVAR_ALLOC_VAL;

	switch (hw->mac.type) {

	case ixgbe_mac_82598EB:
		if (type == -1)
			entry = IXGBE_IVAR_OTHER_CAUSES_INDEX;
		else
			entry += (type * 64);
		index = (entry >> 2) & 0x1F;
		ivar = IXGBE_READ_REG(hw, IXGBE_IVAR(index));
		ivar &= ~(0xFF << (8 * (entry & 0x3)));
		ivar |= (vector << (8 * (entry & 0x3)));
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_IVAR(index), ivar);
		break;

	case ixgbe_mac_82599EB:
		if (type == -1) { /* MISC IVAR */
			index = (entry & 1) * 8;
			ivar = IXGBE_READ_REG(hw, IXGBE_IVAR_MISC);
			ivar &= ~(0xFF << index);
			ivar |= (vector << index);
			IXGBE_WRITE_REG(hw, IXGBE_IVAR_MISC, ivar);
		} else {	/* RX/TX IVARS */
			index = (16 * (entry & 1)) + (8 * type);
			ivar = IXGBE_READ_REG(hw, IXGBE_IVAR(entry >> 1));
			ivar &= ~(0xFF << index);
			ivar |= (vector << index);
			IXGBE_WRITE_REG(hw, IXGBE_IVAR(entry >> 1), ivar);
		}

	default:
		break;
	}
}

static void
ixgbe_configure_ivars(struct adapter *adapter)
{
	struct  ix_queue *que = adapter->queues;
	u32 newitr;

	if (ixgbe_max_interrupt_rate > 0)
		newitr = (8000000 / ixgbe_max_interrupt_rate) & 0x0FF8;
	else
		newitr = 0;

        for (int i = 0; i < adapter->num_queues; i++, que++) {
		/* First the RX queue entry */
                ixgbe_set_ivar(adapter, i, que->msix, 0);
		/* ... and the TX */
		ixgbe_set_ivar(adapter, i, que->msix, 1);
		/* Set an Initial EITR value */
                IXGBE_WRITE_REG(&adapter->hw,
                    IXGBE_EITR(que->msix), newitr);
	}

	/* For the Link interrupt */
        ixgbe_set_ivar(adapter, 1, adapter->linkvec, -1);
}

/*
** ixgbe_sfp_probe - called in the local timer to
** determine if a port had optics inserted.
*/  
static bool ixgbe_sfp_probe(struct adapter *adapter)
{
	struct ixgbe_hw	*hw = &adapter->hw;
	device_t	dev = adapter->dev;
	bool		result = FALSE;

	if ((hw->phy.type == ixgbe_phy_nl) &&
	    (hw->phy.sfp_type == ixgbe_sfp_type_not_present)) {
		s32 ret = hw->phy.ops.identify_sfp(hw);
		if (ret)
                        goto out;
		ret = hw->phy.ops.reset(hw);
		if (ret == IXGBE_ERR_SFP_NOT_SUPPORTED) {
			device_printf(dev,"Unsupported SFP+ module detected!");
			device_printf(dev, "Reload driver with supported module.\n");
			adapter->sfp_probe = FALSE;
                        goto out;
		} else
			device_printf(dev,"SFP+ module detected!\n");
		/* We now have supported optics */
		adapter->sfp_probe = FALSE;
		/* Set the optics type so system reports correctly */
		ixgbe_setup_optics(adapter);
		result = TRUE;
	}
out:
	return (result);
}

/*
** Tasklet handler for MSIX Link interrupts
**  - do outside interrupt since it might sleep
*/
static void
ixgbe_handle_link(void *context)
{
	struct adapter  *adapter = context;

	if (ixgbe_check_link(&adapter->hw,
	    &adapter->link_speed, &adapter->link_up, 0) == 0)
	    ixgbe_update_link_status(adapter);
}

/*
** Tasklet for handling SFP module interrupts
*/
static void
ixgbe_handle_mod(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	device_t	dev = adapter->dev;
	u32 err;

	err = hw->phy.ops.identify_sfp(hw);
	if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		device_printf(dev,
		    "Unsupported SFP+ module type was detected.\n");
		return;
	}
	err = hw->mac.ops.setup_sfp(hw);
	if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		device_printf(dev,
		    "Setup failure - unsupported SFP+ module type.\n");
		return;
	}
	softint_schedule(adapter->msf_si);
	return;
}


/*
** Tasklet for handling MSF (multispeed fiber) interrupts
*/
static void
ixgbe_handle_msf(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 autoneg;
	bool negotiate;

	autoneg = hw->phy.autoneg_advertised;
	if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
		hw->mac.ops.get_link_capabilities(hw, &autoneg, &negotiate);
	else
		negotiate = 0;
	if (hw->mac.ops.setup_link)
		hw->mac.ops.setup_link(hw, autoneg, negotiate, TRUE);
	return;
}

#ifdef IXGBE_FDIR
/*
** Tasklet for reinitializing the Flow Director filter table
*/
static void
ixgbe_reinit_fdir(void *context)
{
	struct adapter  *adapter = context;
	struct ifnet   *ifp = adapter->ifp;

	if (adapter->fdir_reinit != 1) /* Shouldn't happen */
		return;
	ixgbe_reinit_fdir_tables_82599(&adapter->hw);
	adapter->fdir_reinit = 0;
	/* Restart the interface */
	ifp->if_flags |= IFF_RUNNING;
	return;
}
#endif

/**********************************************************************
 *
 *  Update the board statistics counters.
 *
 **********************************************************************/
static void
ixgbe_update_stats_counters(struct adapter *adapter)
{
	struct ifnet   *ifp = adapter->ifp;
	struct ixgbe_hw *hw = &adapter->hw;
	u32  missed_rx = 0, bprc, lxon, lxoff, total;
	u64  total_missed_rx = 0;

	adapter->stats.crcerrs.ev_count += IXGBE_READ_REG(hw, IXGBE_CRCERRS);
	adapter->stats.illerrc.ev_count += IXGBE_READ_REG(hw, IXGBE_ILLERRC);
	adapter->stats.errbc.ev_count += IXGBE_READ_REG(hw, IXGBE_ERRBC);
	adapter->stats.mspdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MSPDC);

	for (int i = 0; i < __arraycount(adapter->stats.mpc); i++) {
		int j = i % adapter->num_queues;
		u32 mp;
		mp = IXGBE_READ_REG(hw, IXGBE_MPC(i));
		/* missed_rx tallies misses for the gprc workaround */
		missed_rx += mp;
		/* global total per queue */
        	adapter->stats.mpc[j].ev_count += mp;
		/* Running comprehensive total for stats display */
		total_missed_rx += adapter->stats.mpc[j].ev_count;
		if (hw->mac.type == ixgbe_mac_82598EB)
			adapter->stats.rnbc[j] +=
			    IXGBE_READ_REG(hw, IXGBE_RNBC(i));
		adapter->stats.pxontxc[j].ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_PXONTXC(i));
		adapter->stats.pxonrxc[j].ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_PXONRXC(i));
		adapter->stats.pxofftxc[j].ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_PXOFFTXC(i));
		adapter->stats.pxoffrxc[j].ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_PXOFFRXC(i));
		adapter->stats.pxon2offc[j].ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_PXON2OFFCNT(i));
	}
	for (int i = 0; i < __arraycount(adapter->stats.qprc); i++) {
		int j = i % adapter->num_queues;
		adapter->stats.qprc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRC(i));
		adapter->stats.qptc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPTC(i));
		adapter->stats.qbrc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QBRC(i));
		adapter->stats.qbrc[j].ev_count += 
		    ((u64)IXGBE_READ_REG(hw, IXGBE_QBRC(i)) << 32);
		adapter->stats.qbtc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QBTC(i));
		adapter->stats.qbtc[j].ev_count +=
		    ((u64)IXGBE_READ_REG(hw, IXGBE_QBTC(i)) << 32);
		adapter->stats.qprdc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
	}
	adapter->stats.mlfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MLFC);
	adapter->stats.mrfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MRFC);
	adapter->stats.rlec.ev_count += IXGBE_READ_REG(hw, IXGBE_RLEC);

	/* Hardware workaround, gprc counts missed packets */
	adapter->stats.gprc.ev_count += IXGBE_READ_REG(hw, IXGBE_GPRC) - missed_rx;

	lxon = IXGBE_READ_REG(hw, IXGBE_LXONTXC);
	adapter->stats.lxontxc.ev_count += lxon;
	lxoff = IXGBE_READ_REG(hw, IXGBE_LXOFFTXC);
	adapter->stats.lxofftxc.ev_count += lxoff;
	total = lxon + lxoff;

	if (hw->mac.type != ixgbe_mac_82598EB) {
		adapter->stats.gorc.ev_count += IXGBE_READ_REG(hw, IXGBE_GORCL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_GORCH) << 32);
		adapter->stats.gotc.ev_count += IXGBE_READ_REG(hw, IXGBE_GOTCL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_GOTCH) << 32) - total * ETHER_MIN_LEN;
		adapter->stats.tor.ev_count += IXGBE_READ_REG(hw, IXGBE_TORL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_TORH) << 32);
		adapter->stats.lxonrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXONRXCNT);
		adapter->stats.lxoffrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXOFFRXCNT);
	} else {
		adapter->stats.lxonrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXONRXC);
		adapter->stats.lxoffrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXOFFRXC);
		/* 82598 only has a counter in the high register */
		adapter->stats.gorc.ev_count += IXGBE_READ_REG(hw, IXGBE_GORCH);
		adapter->stats.gotc.ev_count += IXGBE_READ_REG(hw, IXGBE_GOTCH) - total * ETHER_MIN_LEN;
		adapter->stats.tor.ev_count += IXGBE_READ_REG(hw, IXGBE_TORH);
	}

	/*
	 * Workaround: mprc hardware is incorrectly counting
	 * broadcasts, so for now we subtract those.
	 */
	bprc = IXGBE_READ_REG(hw, IXGBE_BPRC);
	adapter->stats.bprc.ev_count += bprc;
	adapter->stats.mprc.ev_count += IXGBE_READ_REG(hw, IXGBE_MPRC) - ((hw->mac.type == ixgbe_mac_82598EB) ? bprc : 0);

	adapter->stats.prc64.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC64);
	adapter->stats.prc127.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC127);
	adapter->stats.prc255.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC255);
	adapter->stats.prc511.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC511);
	adapter->stats.prc1023.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC1023);
	adapter->stats.prc1522.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC1522);

	adapter->stats.gptc.ev_count += IXGBE_READ_REG(hw, IXGBE_GPTC) - total;
	adapter->stats.mptc.ev_count += IXGBE_READ_REG(hw, IXGBE_MPTC) - total;
	adapter->stats.ptc64.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC64) - total;

	adapter->stats.ruc.ev_count += IXGBE_READ_REG(hw, IXGBE_RUC);
	adapter->stats.rfc.ev_count += IXGBE_READ_REG(hw, IXGBE_RFC);
	adapter->stats.roc.ev_count += IXGBE_READ_REG(hw, IXGBE_ROC);
	adapter->stats.rjc.ev_count += IXGBE_READ_REG(hw, IXGBE_RJC);
	adapter->stats.mngprc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPRC);
	adapter->stats.mngpdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPDC);
	adapter->stats.mngptc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPTC);
	adapter->stats.tpr.ev_count += IXGBE_READ_REG(hw, IXGBE_TPR);
	adapter->stats.tpt.ev_count += IXGBE_READ_REG(hw, IXGBE_TPT);
	adapter->stats.ptc127.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC127);
	adapter->stats.ptc255.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC255);
	adapter->stats.ptc511.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC511);
	adapter->stats.ptc1023.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC1023);
	adapter->stats.ptc1522.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC1522);
	adapter->stats.bptc.ev_count += IXGBE_READ_REG(hw, IXGBE_BPTC);
	adapter->stats.xec.ev_count += IXGBE_READ_REG(hw, IXGBE_XEC);
	adapter->stats.fccrc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCCRC);
	adapter->stats.fclast.ev_count += IXGBE_READ_REG(hw, IXGBE_FCLAST);

	/* Only read FCOE on 82599 */
	if (hw->mac.type != ixgbe_mac_82598EB) {
		adapter->stats.fcoerpdc.ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_FCOERPDC);
		adapter->stats.fcoeprc.ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_FCOEPRC);
		adapter->stats.fcoeptc.ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_FCOEPTC);
		adapter->stats.fcoedwrc.ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_FCOEDWRC);
		adapter->stats.fcoedwtc.ev_count +=
		    IXGBE_READ_REG(hw, IXGBE_FCOEDWTC);
	}

	/* Fill out the OS statistics structure */
	ifp->if_ipackets = adapter->stats.gprc.ev_count;
	ifp->if_opackets = adapter->stats.gptc.ev_count;
	ifp->if_ibytes = adapter->stats.gorc.ev_count;
	ifp->if_obytes = adapter->stats.gotc.ev_count;
	ifp->if_imcasts = adapter->stats.mprc.ev_count;
	ifp->if_collisions = 0;

	/* Rx Errors */
	ifp->if_ierrors = total_missed_rx + adapter->stats.crcerrs.ev_count +
		adapter->stats.rlec.ev_count;
}

/** ixgbe_sysctl_tdh_handler - Handler function
 *  Retrieves the TDH value from the hardware
 */
static int 
ixgbe_sysctl_tdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t val;
	struct tx_ring *txr;

	node = *rnode;
	txr = (struct tx_ring *)node.sysctl_data;
	if (txr == NULL)
		return 0;
	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_TDH(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/** ixgbe_sysctl_tdt_handler - Handler function
 *  Retrieves the TDT value from the hardware
 */
static int 
ixgbe_sysctl_tdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t val;
	struct tx_ring *txr;

	node = *rnode;
	txr = (struct tx_ring *)node.sysctl_data;
	if (txr == NULL)
		return 0;
	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_TDT(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/** ixgbe_sysctl_rdh_handler - Handler function
 *  Retrieves the RDH value from the hardware
 */
static int 
ixgbe_sysctl_rdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t val;
	struct rx_ring *rxr;

	node = *rnode;
	rxr = (struct rx_ring *)node.sysctl_data;
	if (rxr == NULL)
		return 0;
	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_RDH(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/** ixgbe_sysctl_rdt_handler - Handler function
 *  Retrieves the RDT value from the hardware
 */
static int 
ixgbe_sysctl_rdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t val;
	struct rx_ring *rxr;

	node = *rnode;
	rxr = (struct rx_ring *)node.sysctl_data;
	if (rxr == NULL)
		return 0;
	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_RDT(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
ixgbe_sysctl_interrupt_rate_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct ix_queue *que;
	uint32_t reg, usec, rate;

	node = *rnode;
	que = (struct ix_queue *)node.sysctl_data;
	if (que == NULL)
		return 0;
	reg = IXGBE_READ_REG(&que->adapter->hw, IXGBE_EITR(que->msix));
	usec = ((reg & 0x0FF8) >> 3);
	if (usec > 0)
		rate = 1000000 / usec;
	else
		rate = 0;
	node.sysctl_data = &rate;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

const struct sysctlnode *
ixgbe_sysctl_instance(struct adapter *adapter)
{
	const char *dvname;
	struct sysctllog **log;
	int rc;
	const struct sysctlnode *rnode;

	log = &adapter->sysctllog;
	dvname = device_xname(adapter->dev);

	if ((rc = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, dvname,
	    SYSCTL_DESCR("ixgbe information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return rnode;
err:
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
	return NULL;
}

/*
 * Add sysctl variables, one per statistic, to the system.
 */
static void
ixgbe_add_hw_stats(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	const struct sysctlnode *rnode, *cnode;
	struct sysctllog **log = &adapter->sysctllog;
	struct tx_ring *txr = adapter->tx_rings;
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbe_hw	 *hw = &adapter->hw;

	struct ixgbe_hw_stats *stats = &adapter->stats;

	/* Driver Statistics */
#if 0
	/* These counters are not updated by the software */
	SYSCTL_ADD_ULONG(ctx, child, OID_AUTO, "dropped",
			CTLFLAG_RD, &adapter->dropped_pkts,
			"Driver dropped packets");
	SYSCTL_ADD_ULONG(ctx, child, OID_AUTO, "mbuf_header_failed",
			CTLFLAG_RD, &adapter->mbuf_header_failed,
			"???");
	SYSCTL_ADD_ULONG(ctx, child, OID_AUTO, "mbuf_packet_failed",
			CTLFLAG_RD, &adapter->mbuf_packet_failed,
			"???");
	SYSCTL_ADD_ULONG(ctx, child, OID_AUTO, "no_tx_map_avail",
			CTLFLAG_RD, &adapter->no_tx_map_avail,
			"???");
#endif
	evcnt_attach_dynamic(&adapter->handleq, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Handled queue in softint");
	evcnt_attach_dynamic(&adapter->req, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Requeued in softint");
	evcnt_attach_dynamic(&adapter->morerx, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Interrupt handler more rx");
	evcnt_attach_dynamic(&adapter->moretx, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Interrupt handler more tx");
	evcnt_attach_dynamic(&adapter->txloops, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Interrupt handler tx loops");
	evcnt_attach_dynamic(&adapter->efbig_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma soft fail EFBIG");
	evcnt_attach_dynamic(&adapter->m_defrag_failed, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "m_defrag() failed");
	evcnt_attach_dynamic(&adapter->efbig2_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma hard fail EFBIG");
	evcnt_attach_dynamic(&adapter->einval_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma hard fail EINVAL");
	evcnt_attach_dynamic(&adapter->other_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma hard fail other");
	evcnt_attach_dynamic(&adapter->eagain_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma soft fail EAGAIN");
	evcnt_attach_dynamic(&adapter->enomem_tx_dma_setup, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Driver tx dma soft fail ENOMEM");
	evcnt_attach_dynamic(&adapter->watchdog_events, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Watchdog timeouts");
	evcnt_attach_dynamic(&adapter->tso_err, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "TSO errors");
	evcnt_attach_dynamic(&adapter->tso_tx, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "TSO");
	evcnt_attach_dynamic(&adapter->link_irq, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Link MSIX IRQ Handled");

	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		snprintf(adapter->queues[i].evnamebuf,
		    sizeof(adapter->queues[i].evnamebuf), "%s queue%d",
		    device_xname(dev), i);
		snprintf(adapter->queues[i].namebuf,
		    sizeof(adapter->queues[i].namebuf), "queue%d", i);

		if ((rnode = ixgbe_sysctl_instance(adapter)) == NULL) {
			aprint_error_dev(dev, "could not create sysctl root\n");
			break;
		}

		if (sysctl_createv(log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE,
		    adapter->queues[i].namebuf, SYSCTL_DESCR("Queue Name"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0)
			break;

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "interrupt_rate", SYSCTL_DESCR("Interrupt Rate"),
		    ixgbe_sysctl_interrupt_rate_handler, 0,
		    (void *)&adapter->queues[i], 0, CTL_CREATE, CTL_EOL) != 0)
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

		evcnt_attach_dynamic(&txr->no_desc_avail, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "Queue No Descriptor Available");
		evcnt_attach_dynamic(&txr->total_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf,
		    "Queue Packets Transmitted");

#ifdef LRO
		struct lro_ctrl *lro = &rxr->lro;
#endif /* LRO */

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

		if (i < __arraycount(adapter->stats.mpc)) {
			evcnt_attach_dynamic(&adapter->stats.mpc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "Missed Packet Count");
		}
		if (i < __arraycount(adapter->stats.pxontxc)) {
			evcnt_attach_dynamic(&adapter->stats.pxontxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxontxc");
			evcnt_attach_dynamic(&adapter->stats.pxonrxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxonrxc");
			evcnt_attach_dynamic(&adapter->stats.pxofftxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxofftxc");
			evcnt_attach_dynamic(&adapter->stats.pxoffrxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxoffrxc");
			evcnt_attach_dynamic(&adapter->stats.pxon2offc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxon2offc");
		}
		if (i < __arraycount(adapter->stats.qprc)) {
			evcnt_attach_dynamic(&adapter->stats.qprc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qprc");
			evcnt_attach_dynamic(&adapter->stats.qptc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qptc");
			evcnt_attach_dynamic(&adapter->stats.qbrc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qbrc");
			evcnt_attach_dynamic(&adapter->stats.qbtc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qbtc");
			evcnt_attach_dynamic(&adapter->stats.qprdc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qprdc");
		}

		evcnt_attach_dynamic(&rxr->rx_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Packets Received");
		evcnt_attach_dynamic(&rxr->rx_bytes, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Queue Bytes Received");
		evcnt_attach_dynamic(&rxr->no_jmbuf, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx no jumbo mbuf");
		evcnt_attach_dynamic(&rxr->rx_discarded, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx discarded");
		evcnt_attach_dynamic(&rxr->rx_split_packets, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx split packets");
		evcnt_attach_dynamic(&rxr->rx_irq, EVCNT_TYPE_MISC,
		    NULL, adapter->queues[i].evnamebuf, "Rx interrupts");
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
	    sizeof(stats->namebuf), "%s MAC Statistics", device_xname(dev));

	evcnt_attach_dynamic(&stats->ipcs, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - IP");
	evcnt_attach_dynamic(&stats->l4cs, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - L4");
	evcnt_attach_dynamic(&stats->ipcs_bad, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - IP bad");
	evcnt_attach_dynamic(&stats->l4cs_bad, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "rx csum offload - L4 bad");
	evcnt_attach_dynamic(&stats->intzero, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Interrupt conditions zero");
	evcnt_attach_dynamic(&stats->legint, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Legacy interrupts");
	evcnt_attach_dynamic(&stats->crcerrs, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "CRC Errors");
	evcnt_attach_dynamic(&stats->illerrc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Illegal Byte Errors");
	evcnt_attach_dynamic(&stats->errbc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Byte Errors");
	evcnt_attach_dynamic(&stats->mspdc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "MAC Short Packets Discarded");
	evcnt_attach_dynamic(&stats->mlfc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "MAC Local Faults");
	evcnt_attach_dynamic(&stats->mrfc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "MAC Remote Faults");
	evcnt_attach_dynamic(&stats->rlec, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Receive Length Errors");
	evcnt_attach_dynamic(&stats->lxontxc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Link XON Transmitted");
	evcnt_attach_dynamic(&stats->lxonrxc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Link XON Received");
	evcnt_attach_dynamic(&stats->lxofftxc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Link XOFF Transmitted");
	evcnt_attach_dynamic(&stats->lxoffrxc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Link XOFF Received");

	/* Packet Reception Stats */
	evcnt_attach_dynamic(&stats->tor, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Total Octets Received");
	evcnt_attach_dynamic(&stats->gorc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Good Octets Received");
	evcnt_attach_dynamic(&stats->tpr, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Total Packets Received");
	evcnt_attach_dynamic(&stats->gprc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Good Packets Received");
	evcnt_attach_dynamic(&stats->mprc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Multicast Packets Received");
	evcnt_attach_dynamic(&stats->bprc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Broadcast Packets Received");
	evcnt_attach_dynamic(&stats->prc64, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "64 byte frames received ");
	evcnt_attach_dynamic(&stats->prc127, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "65-127 byte frames received");
	evcnt_attach_dynamic(&stats->prc255, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "128-255 byte frames received");
	evcnt_attach_dynamic(&stats->prc511, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "256-511 byte frames received");
	evcnt_attach_dynamic(&stats->prc1023, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "512-1023 byte frames received");
	evcnt_attach_dynamic(&stats->prc1522, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "1023-1522 byte frames received");
	evcnt_attach_dynamic(&stats->ruc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Receive Undersized");
	evcnt_attach_dynamic(&stats->rfc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Fragmented Packets Received ");
	evcnt_attach_dynamic(&stats->roc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Oversized Packets Received");
	evcnt_attach_dynamic(&stats->rjc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Received Jabber");
	evcnt_attach_dynamic(&stats->mngprc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Management Packets Received");
	evcnt_attach_dynamic(&stats->xec, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Checksum Errors");

	/* Packet Transmission Stats */
	evcnt_attach_dynamic(&stats->gotc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Good Octets Transmitted");
	evcnt_attach_dynamic(&stats->tpt, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Total Packets Transmitted");
	evcnt_attach_dynamic(&stats->gptc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Good Packets Transmitted");
	evcnt_attach_dynamic(&stats->bptc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Broadcast Packets Transmitted");
	evcnt_attach_dynamic(&stats->mptc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Multicast Packets Transmitted");
	evcnt_attach_dynamic(&stats->mngptc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Management Packets Transmitted");
	evcnt_attach_dynamic(&stats->ptc64, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "64 byte frames transmitted ");
	evcnt_attach_dynamic(&stats->ptc127, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "65-127 byte frames transmitted");
	evcnt_attach_dynamic(&stats->ptc255, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "128-255 byte frames transmitted");
	evcnt_attach_dynamic(&stats->ptc511, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "256-511 byte frames transmitted");
	evcnt_attach_dynamic(&stats->ptc1023, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "512-1023 byte frames transmitted");
	evcnt_attach_dynamic(&stats->ptc1522, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "1024-1522 byte frames transmitted");

	/* FC Stats */
	evcnt_attach_dynamic(&stats->fccrc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "FC CRC Errors");
	evcnt_attach_dynamic(&stats->fclast, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "FC Last Error");
	if (hw->mac.type != ixgbe_mac_82598EB) {
		evcnt_attach_dynamic(&stats->fcoerpdc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "FCoE Packets Dropped");
		evcnt_attach_dynamic(&stats->fcoeprc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "FCoE Packets Received");
		evcnt_attach_dynamic(&stats->fcoeptc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "FCoE Packets Transmitted");
		evcnt_attach_dynamic(&stats->fcoedwrc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "FCoE DWords Received");
		evcnt_attach_dynamic(&stats->fcoedwtc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "FCoE DWords Transmitted");
	}
}

/*
** Set flow control using sysctl:
** Flow control values:
** 	0 - off
**	1 - rx pause
**	2 - tx pause
**	3 - full
*/
static int
ixgbe_set_flowcntl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	int last = ixgbe_flow_control;
	struct adapter *adapter;

	node = *rnode;
	adapter = (struct adapter *)node.sysctl_data;
	node.sysctl_data = &ixgbe_flow_control;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	/* Don't bother if it's not changed */
	if (ixgbe_flow_control == last)
		return (0);

	switch (ixgbe_flow_control) {
		case ixgbe_fc_rx_pause:
		case ixgbe_fc_tx_pause:
		case ixgbe_fc_full:
			adapter->hw.fc.requested_mode = ixgbe_flow_control;
			break;
		case ixgbe_fc_none:
		default:
			adapter->hw.fc.requested_mode = ixgbe_fc_none;
	}

	ixgbe_fc_enable(&adapter->hw, 0);
	return 0;
}

static void
ixgbe_add_rx_process_limit(struct adapter *adapter, const char *name,
        const char *description, int *limit, int value)
{
	const struct sysctlnode *rnode, *cnode;
	struct sysctllog **log = &adapter->sysctllog;

        *limit = value;

	if ((rnode = ixgbe_sysctl_instance(adapter)) == NULL)
		aprint_error_dev(adapter->dev,
		    "could not create sysctl root\n");
	else if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT,
	    name, SYSCTL_DESCR(description),
	    NULL, 0, limit, 0,
	    CTL_CREATE, CTL_EOL) != 0) {
		aprint_error_dev(adapter->dev, "%s: could not create sysctl",
		    __func__);
	}
}

/*
** Control link advertise speed:
** 	0 - normal
**	1 - advertise only 1G
*/
static int
ixgbe_set_advertise(SYSCTLFN_ARGS)
{
	struct sysctlnode	node;
	int			t, error;
	struct adapter		*adapter;
	struct ixgbe_hw		*hw;
	ixgbe_link_speed	speed, last;

	node = *rnode;
	adapter = (struct adapter *)node.sysctl_data;
	t = adapter->advertise;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (t == -1)
		return 0;

	adapter->advertise = t;

	hw = &adapter->hw;
	last = hw->phy.autoneg_advertised;

	if (!((hw->phy.media_type == ixgbe_media_type_copper) ||
            (hw->phy.multispeed_fiber)))
		return 0;

	if (adapter->advertise == 1)
                speed = IXGBE_LINK_SPEED_1GB_FULL;
	else
                speed = IXGBE_LINK_SPEED_1GB_FULL |
			IXGBE_LINK_SPEED_10GB_FULL;

	if (speed == last) /* no change */
		return 0;

	hw->mac.autotry_restart = TRUE;
	hw->mac.ops.setup_link(hw, speed, TRUE, TRUE);

	return 0;
}
