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
/*$FreeBSD: head/sys/dev/ixgbe/if_ix.c 302384 2016-07-07 03:39:18Z sbruno $*/
/*$NetBSD: ixgbe.c,v 1.14.4.8 2017/02/05 13:40:45 skrll Exp $*/

#include "opt_inet.h"
#include "opt_inet6.h"

#include "ixgbe.h"
#include "vlan.h"

#include <sys/cprng.h>

/*********************************************************************
 *  Driver version
 *********************************************************************/
char ixgbe_driver_version[] = "3.1.13-k";


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
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP_SF2, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP_FCOE, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599EN_SFP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_SFP_SF_QP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_QSFP_SF_QP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X540T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X540T1, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550T1, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_KR, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_KX4, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_10G_T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_SFP, 0, 0, 0},
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
static bool	ixgbe_suspend(device_t, const pmf_qual_t *);
static bool	ixgbe_resume(device_t, const pmf_qual_t *);
static int      ixgbe_ioctl(struct ifnet *, u_long, void *);
static void	ixgbe_ifstop(struct ifnet *, int);
static int	ixgbe_init(struct ifnet *);
static void	ixgbe_init_locked(struct adapter *);
static void     ixgbe_stop(void *);
static void	ixgbe_add_media_types(struct adapter *);
static void     ixgbe_media_status(struct ifnet *, struct ifmediareq *);
static int      ixgbe_media_change(struct ifnet *);
static void     ixgbe_identify_hardware(struct adapter *);
static int      ixgbe_allocate_pci_resources(struct adapter *,
		    const struct pci_attach_args *);
static void	ixgbe_get_slot_info(struct adapter *);
static int      ixgbe_allocate_msix(struct adapter *,
		    const struct pci_attach_args *);
static int      ixgbe_allocate_legacy(struct adapter *,
		    const struct pci_attach_args *);
static int	ixgbe_setup_msix(struct adapter *);
static void	ixgbe_free_pci_resources(struct adapter *);
static void	ixgbe_local_timer(void *);
static void	ixgbe_local_timer1(void *);
static int	ixgbe_setup_interface(device_t, struct adapter *);
static void	ixgbe_config_gpie(struct adapter *);
static void	ixgbe_config_dmac(struct adapter *);
static void	ixgbe_config_delay_values(struct adapter *);
static void	ixgbe_config_link(struct adapter *);
static void	ixgbe_check_wol_support(struct adapter *);
static int	ixgbe_setup_low_power_mode(struct adapter *);
static void	ixgbe_rearm_queues(struct adapter *, u64);

static void     ixgbe_initialize_transmit_units(struct adapter *);
static void     ixgbe_initialize_receive_units(struct adapter *);
static void	ixgbe_enable_rx_drop(struct adapter *);
static void	ixgbe_disable_rx_drop(struct adapter *);
static void	ixgbe_initialize_rss_mapping(struct adapter *);

static void     ixgbe_enable_intr(struct adapter *);
static void     ixgbe_disable_intr(struct adapter *);
static void     ixgbe_update_stats_counters(struct adapter *);
static void     ixgbe_set_promisc(struct adapter *);
static void     ixgbe_set_multi(struct adapter *);
static void     ixgbe_update_link_status(struct adapter *);
static void	ixgbe_set_ivar(struct adapter *, u8, u8, s8);
static void	ixgbe_configure_ivars(struct adapter *);
static u8 *	ixgbe_mc_array_itr(struct ixgbe_hw *, u8 **, u32 *);

static void	ixgbe_setup_vlan_hw_support(struct adapter *);
#if 0
static void	ixgbe_register_vlan(void *, struct ifnet *, u16);
static void	ixgbe_unregister_vlan(void *, struct ifnet *, u16);
#endif

static void	ixgbe_add_device_sysctls(struct adapter *);
static void     ixgbe_add_hw_stats(struct adapter *);
static int	ixgbe_set_flowcntl(struct adapter *, int);
static int	ixgbe_set_advertise(struct adapter *, int);

/* Sysctl handlers */
static void	ixgbe_set_sysctl_value(struct adapter *, const char *,
		     const char *, int *, int);
static int	ixgbe_sysctl_flowcntl(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_advertise(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_thermal_test(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_dmac(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_phy_temp(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_phy_overtemp_occurred(SYSCTLFN_PROTO);
#ifdef IXGBE_DEBUG
static int	ixgbe_sysctl_power_state(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_print_rss_config(SYSCTLFN_PROTO);
#endif
static int	ixgbe_sysctl_wol_enable(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_wufc(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_eee_enable(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_eee_negotiated(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_eee_rx_lpi_status(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_eee_tx_lpi_status(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_eee_tx_lpi_delay(SYSCTLFN_PROTO);

/* Support for pluggable optic modules */
static bool	ixgbe_sfp_probe(struct adapter *);
static void	ixgbe_setup_optics(struct adapter *);

/* Legacy (single vector interrupt handler */
static int	ixgbe_legacy_irq(void *);

/* The MSI/X Interrupt handlers */
static int	ixgbe_msix_que(void *);
static int	ixgbe_msix_link(void *);

/* Software interrupts for deferred work */
static void	ixgbe_handle_que(void *);
static void	ixgbe_handle_link(void *);
static void	ixgbe_handle_msf(void *);
static void	ixgbe_handle_mod(void *);
static void	ixgbe_handle_phy(void *);

const struct sysctlnode *ixgbe_sysctl_instance(struct adapter *);
static ixgbe_vendor_info_t *ixgbe_lookup(const struct pci_attach_args *);

#ifdef IXGBE_FDIR
static void	ixgbe_reinit_fdir(void *, int);
#endif

#ifdef PCI_IOV
static void	ixgbe_ping_all_vfs(struct adapter *);
static void	ixgbe_handle_mbx(void *, int);
static int	ixgbe_init_iov(device_t, u16, const nvlist_t *);
static void	ixgbe_uninit_iov(device_t);
static int	ixgbe_add_vf(device_t, u16, const nvlist_t *);
static void	ixgbe_initialize_iov(struct adapter *);
static void	ixgbe_recalculate_max_frame(struct adapter *);
static void	ixgbe_init_vf(struct adapter *, struct ixgbe_vf *);
#endif /* PCI_IOV */


/*********************************************************************
 *  FreeBSD Device Interface Entry Points
 *********************************************************************/

CFATTACH_DECL3_NEW(ixg, sizeof(struct adapter),
    ixgbe_probe, ixgbe_attach, ixgbe_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

#if 0
devclass_t ix_devclass;
DRIVER_MODULE(ix, pci, ix_driver, ix_devclass, 0, 0);

MODULE_DEPEND(ix, pci, 1, 1, 1);
MODULE_DEPEND(ix, ether, 1, 1, 1);
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
#define SYSCTL_INT(_a1, _a2, _a3, _a4, _a5, _a6, _a7)
SYSCTL_INT(_hw_ix, OID_AUTO, enable_aim, CTLFLAG_RWTUN, &ixgbe_enable_aim, 0,
    "Enable adaptive interrupt moderation");

static int ixgbe_max_interrupt_rate = (4000000 / IXGBE_LOW_LATENCY);
SYSCTL_INT(_hw_ix, OID_AUTO, max_interrupt_rate, CTLFLAG_RDTUN,
    &ixgbe_max_interrupt_rate, 0, "Maximum interrupts per second");

/* How many packets rxeof tries to clean at a time */
static int ixgbe_rx_process_limit = 256;
SYSCTL_INT(_hw_ix, OID_AUTO, rx_process_limit, CTLFLAG_RDTUN,
    &ixgbe_rx_process_limit, 0,
    "Maximum number of received packets to process at a time,"
    "-1 means unlimited");

/* How many packets txeof tries to clean at a time */
static int ixgbe_tx_process_limit = 256;
SYSCTL_INT(_hw_ix, OID_AUTO, tx_process_limit, CTLFLAG_RDTUN,
    &ixgbe_tx_process_limit, 0,
    "Maximum number of sent packets to process at a time,"
    "-1 means unlimited");

/* Flow control setting, default to full */
static int ixgbe_flow_control = ixgbe_fc_full;
SYSCTL_INT(_hw_ix, OID_AUTO, flow_control, CTLFLAG_RDTUN,
    &ixgbe_flow_control, 0, "Default flow control used for all adapters");

/* Advertise Speed, default to 0 (auto) */
static int ixgbe_advertise_speed = 0;
SYSCTL_INT(_hw_ix, OID_AUTO, advertise_speed, CTLFLAG_RDTUN,
    &ixgbe_advertise_speed, 0, "Default advertised speed for all adapters");

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
SYSCTL_INT(_hw_ix, OID_AUTO, enable_msix, CTLFLAG_RDTUN, &ixgbe_enable_msix, 0,
    "Enable MSI-X interrupts");

/*
 * Number of Queues, can be set to 0,
 * it then autoconfigures based on the
 * number of cpus with a max of 8. This
 * can be overriden manually here.
 */
static int ixgbe_num_queues = 0;
SYSCTL_INT(_hw_ix, OID_AUTO, num_queues, CTLFLAG_RDTUN, &ixgbe_num_queues, 0,
    "Number of queues to configure, 0 indicates autoconfigure");

/*
** Number of TX descriptors per ring,
** setting higher than RX as this seems
** the better performing choice.
*/
static int ixgbe_txd = PERFORM_TXD;
SYSCTL_INT(_hw_ix, OID_AUTO, txd, CTLFLAG_RDTUN, &ixgbe_txd, 0,
    "Number of transmit descriptors per queue");

/* Number of RX descriptors per ring */
static int ixgbe_rxd = PERFORM_RXD;
SYSCTL_INT(_hw_ix, OID_AUTO, rxd, CTLFLAG_RDTUN, &ixgbe_rxd, 0,
    "Number of receive descriptors per queue");

/*
** Defining this on will allow the use
** of unsupported SFP+ modules, note that
** doing so you are on your own :)
*/
static int allow_unsupported_sfp = false;
#define TUNABLE_INT(__x, __y)
TUNABLE_INT("hw.ix.unsupported_sfp", &allow_unsupported_sfp);

/* Keep running tab on them for sanity check */
static int ixgbe_total_ports;

#ifdef IXGBE_FDIR
/* 
** Flow Director actually 'steals'
** part of the packet buffer as its
** filter pool, this variable controls
** how much it uses:
**  0 = 64K, 1 = 128K, 2 = 256K
*/
static int fdir_pballoc = 1;
#endif

#ifdef DEV_NETMAP
/*
 * The #ifdef DEV_NETMAP / #endif blocks in this file are meant to
 * be a reference on how to implement netmap support in a driver.
 * Additional comments are in ixgbe_netmap.h .
 *
 * <dev/netmap/ixgbe_netmap.h> contains functions for netmap support
 * that extend the standard driver.
 */
#include <dev/netmap/ixgbe_netmap.h>
#endif /* DEV_NETMAP */

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

	INIT_DEBUGOUT("ixgbe_lookup: begin");

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
	int             error = -1;
	u16		csum;
	u32		ctrl_ext;
	ixgbe_vendor_info_t *ent;
	struct pci_attach_args *pa = aux;

	INIT_DEBUGOUT("ixgbe_attach: begin");

	/* Allocate, clear, and link in our adapter structure */
	adapter = device_private(dev);
	adapter->dev = dev;
	hw = &adapter->hw;
	adapter->osdep.pc = pa->pa_pc;
	adapter->osdep.tag = pa->pa_tag;
	if (pci_dma64_available(pa))
		adapter->osdep.dmat = pa->pa_dmat64;
	else
		adapter->osdep.dmat = pa->pa_dmat;
	adapter->osdep.attached = false;

	ent = ixgbe_lookup(pa);

	KASSERT(ent != NULL);

	aprint_normal(": %s, Version - %s\n",
	    ixgbe_strings[ent->index], ixgbe_driver_version);

#ifdef DEV_NETMAP
	adapter->init_locked = ixgbe_init_locked;
	adapter->stop_locked = ixgbe_stop;
#endif

	/* Core Lock Init*/
	IXGBE_CORE_LOCK_INIT(adapter, device_xname(dev));

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

	/* Sysctls for limiting the amount of work done in the taskqueues */
	ixgbe_set_sysctl_value(adapter, "rx_processing_limit",
	    "max number of rx packets to process",
	    &adapter->rx_process_limit, ixgbe_rx_process_limit);

	ixgbe_set_sysctl_value(adapter, "tx_processing_limit",
	    "max number of tx packets to process",
	&adapter->tx_process_limit, ixgbe_tx_process_limit);

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
	if (nmbclusters > 0) {
		int s;
		s = (ixgbe_rxd * adapter->num_queues) * ixgbe_total_ports;
		if (s > nmbclusters) {
			aprint_error_dev(dev, "RX Descriptors exceed "
			    "system mbuf max, using default instead!\n");
			ixgbe_rxd = DEFAULT_RXD;
		}
	}

	if (((ixgbe_rxd * sizeof(union ixgbe_adv_rx_desc)) % DBA_ALIGN) != 0 ||
	    ixgbe_rxd < MIN_RXD || ixgbe_rxd > MAX_RXD) {
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
	adapter->mta = malloc(sizeof(*adapter->mta) *
	    MAX_NUM_MULTICAST_ADDRESSES, M_DEVBUF, M_NOWAIT);
	if (adapter->mta == NULL) {
		aprint_error_dev(dev, "Cannot allocate multicast setup array\n");
		error = ENOMEM;
		goto err_late;
	}

	/* Initialize the shared code */
	hw->allow_unsupported_sfp = allow_unsupported_sfp;
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
		aprint_error_dev(dev, "Unsupported SFP+ module detected!\n");
		error = EIO;
		goto err_late;
	} else if (error) {
		aprint_error_dev(dev, "Unable to initialize the shared code\n");
		error = EIO;
		goto err_late;
	}

	/* Make sure we have a good EEPROM before we read from it */
	if (ixgbe_validate_eeprom_checksum(&adapter->hw, &csum) < 0) {
		aprint_error_dev(dev, "The EEPROM Checksum Is Not Valid\n");
		error = EIO;
		goto err_late;
	}

	error = ixgbe_init_hw(hw);
	switch (error) {
	case IXGBE_ERR_EEPROM_VERSION:
		aprint_error_dev(dev, "This device is a pre-production adapter/"
		    "LOM.  Please be aware there may be issues associated "
		    "with your hardware.\nIf you are experiencing problems "
		    "please contact your Intel or hardware representative "
		    "who provided you with this hardware.\n");
		break;
	case IXGBE_ERR_SFP_NOT_SUPPORTED:
		aprint_error_dev(dev, "Unsupported SFP+ Module\n");
		error = EIO;
		aprint_error_dev(dev, "Hardware Initialization Failure\n");
		goto err_late;
	case IXGBE_ERR_SFP_NOT_PRESENT:
		aprint_error_dev(dev, "No SFP+ Module found\n");
		/* falls thru */
	default:
		break;
	}

	/* hw.ix defaults init */
	ixgbe_set_advertise(adapter, ixgbe_advertise_speed);
	ixgbe_set_flowcntl(adapter, ixgbe_flow_control);
	adapter->enable_aim = ixgbe_enable_aim;

	error = -1;
	if ((adapter->msix > 1) && (ixgbe_enable_msix))
		error = ixgbe_allocate_msix(adapter, pa);
	if (error != 0)
		error = ixgbe_allocate_legacy(adapter, pa);
	if (error) 
		goto err_late;

	/* Enable the optics for 82599 SFP+ fiber */
	ixgbe_enable_tx_laser(hw);

	/* Enable power to the phy. */
	ixgbe_set_phy_power(hw, TRUE);

	/* Setup OS specific network interface */
	if (ixgbe_setup_interface(dev, adapter) != 0)
		goto err_late;

	/* Initialize statistics */
	ixgbe_update_stats_counters(adapter);

        /* Check PCIE slot type/speed/width */
	ixgbe_get_slot_info(adapter);

	/* Set an initial default flow control & dmac value */
	adapter->fc = ixgbe_fc_full;
	adapter->dmac = 0;
	adapter->eee_enabled = 0;

#ifdef PCI_IOV
	if ((hw->mac.type != ixgbe_mac_82598EB) && (adapter->msix > 1)) {
		nvlist_t *pf_schema, *vf_schema;

		hw->mbx.ops.init_params(hw);
		pf_schema = pci_iov_schema_alloc_node();
		vf_schema = pci_iov_schema_alloc_node();
		pci_iov_schema_add_unicast_mac(vf_schema, "mac-addr", 0, NULL);
		pci_iov_schema_add_bool(vf_schema, "mac-anti-spoof",
		    IOV_SCHEMA_HASDEFAULT, TRUE);
		pci_iov_schema_add_bool(vf_schema, "allow-set-mac",
		    IOV_SCHEMA_HASDEFAULT, FALSE);
		pci_iov_schema_add_bool(vf_schema, "allow-promisc",
		    IOV_SCHEMA_HASDEFAULT, FALSE);
		error = pci_iov_attach(dev, pf_schema, vf_schema);
		if (error != 0) {
			device_printf(dev,
			    "Error %d setting up SR-IOV\n", error);
		}
	}
#endif /* PCI_IOV */

	/* Check for certain supported features */
	ixgbe_check_wol_support(adapter);

	/* Add sysctls */
	ixgbe_add_device_sysctls(adapter);
	ixgbe_add_hw_stats(adapter);

	/* let hardware know driver is loaded */
	ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
	ctrl_ext |= IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);

#ifdef DEV_NETMAP
	ixgbe_netmap_attach(adapter);
#endif /* DEV_NETMAP */

	if (pmf_device_register(dev, ixgbe_suspend, ixgbe_resume))
		pmf_class_network_register(dev, adapter->ifp);
	else
		aprint_error_dev(dev, "couldn't establish power handler\n");

	INIT_DEBUGOUT("ixgbe_attach: end");
	adapter->osdep.attached = true;
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
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;
	struct ix_queue *que = adapter->queues;
	struct tx_ring *txr = adapter->tx_rings;
	u32	ctrl_ext;

	INIT_DEBUGOUT("ixgbe_detach: begin");
	if (adapter->osdep.attached == false)
		return 0;

#if NVLAN > 0
	/* Make sure VLANs are not using driver */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		;	/* nothing to do: no VLANs */ 
	else if ((flags & (DETACH_SHUTDOWN|DETACH_FORCE)) != 0)
		vlan_ifdetach(adapter->ifp);
	else {
		aprint_error_dev(dev, "VLANs in use\n");
		return EBUSY;
	}
#endif

#ifdef PCI_IOV
	if (pci_iov_detach(dev) != 0) {
		device_printf(dev, "SR-IOV in use; detach first.\n");
		return (EBUSY);
	}
#endif /* PCI_IOV */

	pmf_device_deregister(dev);

	ether_ifdetach(adapter->ifp);
	/* Stop the adapter */
	IXGBE_CORE_LOCK(adapter);
	ixgbe_setup_low_power_mode(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	for (int i = 0; i < adapter->num_queues; i++, que++, txr++) {
#ifndef IXGBE_LEGACY_TX
		softint_disestablish(txr->txr_si);
#endif
		softint_disestablish(que->que_si);
	}

	/* Drain the Link queue */
	softint_disestablish(adapter->link_si);
	softint_disestablish(adapter->mod_si);
	softint_disestablish(adapter->msf_si);
#ifdef PCI_IOV
	softint_disestablish(adapter->mbx_si);
#endif
	softint_disestablish(adapter->phy_si);
#ifdef IXGBE_FDIR
	softint_disestablish(adapter->fdir_si);
#endif

	/* let hardware know driver is unloading */
	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	ctrl_ext &= ~IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT, ctrl_ext);

	callout_halt(&adapter->timer, NULL);
#ifdef DEV_NETMAP
	netmap_detach(adapter->ifp);
#endif /* DEV_NETMAP */
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
	evcnt_detach(&adapter->link_irq);

	txr = adapter->tx_rings;
	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		evcnt_detach(&txr->no_desc_avail);
		evcnt_detach(&txr->total_packets);
		evcnt_detach(&txr->tso_tx);

		if (i < __arraycount(adapter->stats.pf.mpc)) {
			evcnt_detach(&adapter->stats.pf.mpc[i]);
		}
		if (i < __arraycount(adapter->stats.pf.pxontxc)) {
			evcnt_detach(&adapter->stats.pf.pxontxc[i]);
			evcnt_detach(&adapter->stats.pf.pxonrxc[i]);
			evcnt_detach(&adapter->stats.pf.pxofftxc[i]);
			evcnt_detach(&adapter->stats.pf.pxoffrxc[i]);
			evcnt_detach(&adapter->stats.pf.pxon2offc[i]);
		}
		if (i < __arraycount(adapter->stats.pf.qprc)) {
			evcnt_detach(&adapter->stats.pf.qprc[i]);
			evcnt_detach(&adapter->stats.pf.qptc[i]);
			evcnt_detach(&adapter->stats.pf.qbrc[i]);
			evcnt_detach(&adapter->stats.pf.qbtc[i]);
			evcnt_detach(&adapter->stats.pf.qprdc[i]);
		}

		evcnt_detach(&rxr->rx_packets);
		evcnt_detach(&rxr->rx_bytes);
		evcnt_detach(&rxr->rx_copies);
		evcnt_detach(&rxr->no_jmbuf);
		evcnt_detach(&rxr->rx_discarded);
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
	int error = 0;

	INIT_DEBUGOUT("ixgbe_shutdown: begin");

	IXGBE_CORE_LOCK(adapter);
	error = ixgbe_setup_low_power_mode(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	return (error);
}
#endif

/**
 * Methods for going from:
 * D0 -> D3: ixgbe_suspend
 * D3 -> D0: ixgbe_resume
 */
static bool
ixgbe_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct adapter *adapter = device_private(dev);
	int error = 0;

	INIT_DEBUGOUT("ixgbe_suspend: begin");

	IXGBE_CORE_LOCK(adapter);

	error = ixgbe_setup_low_power_mode(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return (error);
}

static bool
ixgbe_resume(device_t dev, const pmf_qual_t *qual)
{
	struct adapter *adapter = device_private(dev);
	struct ifnet *ifp = adapter->ifp;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 wus;

	INIT_DEBUGOUT("ixgbe_resume: begin");

	IXGBE_CORE_LOCK(adapter);

	/* Read & clear WUS register */
	wus = IXGBE_READ_REG(hw, IXGBE_WUS);
	if (wus)
		device_printf(dev, "Woken up by (WUS): %#010x\n",
		    IXGBE_READ_REG(hw, IXGBE_WUS));
	IXGBE_WRITE_REG(hw, IXGBE_WUS, 0xffffffff);
	/* And clear WUFC until next low-power transition */
	IXGBE_WRITE_REG(hw, IXGBE_WUFC, 0);

	/*
	 * Required after D3->D0 transition;
	 * will re-advertise all previous advertised speeds
	 */
	if (ifp->if_flags & IFF_UP)
		ixgbe_init_locked(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return true;
}

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

	/* Set up VLAN support and filter */
	ixgbe_setup_vlan_hw_support(adapter);

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
#ifdef __NetBSD__
	case SIOCINITIFADDR:
		IOCTL_DEBUGOUT("ioctl: SIOCINITIFADDR");
		break;
	case SIOCGIFFLAGS:
		IOCTL_DEBUGOUT("ioctl: SIOCGIFFLAGS");
		break;
	case SIOCGIFAFLAG_IN:
		IOCTL_DEBUGOUT("ioctl: SIOCGIFAFLAG_IN");
		break;
	case SIOCGIFADDR:
		IOCTL_DEBUGOUT("ioctl: SIOCGIFADDR");
		break;
	case SIOCGIFMTU:
		IOCTL_DEBUGOUT("ioctl: SIOCGIFMTU (Get Interface MTU)");
		break;
	case SIOCGIFCAP:
		IOCTL_DEBUGOUT("ioctl: SIOCGIFCAP (Get IF cap)");
		break;
	case SIOCGETHERCAP:
		IOCTL_DEBUGOUT("ioctl: SIOCGETHERCAP (Get ethercap)");
		break;
	case SIOCGLIFADDR:
		IOCTL_DEBUGOUT("ioctl: SIOCGLIFADDR (Get Interface addr)");
		break;
	case SIOCAIFADDR:
		IOCTL_DEBUGOUT("ioctl: SIOCAIFADDR (add/chg IF alias)");
		break;
#endif
	default:
		IOCTL_DEBUGOUT1("ioctl: UNKNOWN (0x%X)", (int)command);
		break;
	}

	switch (command) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		return ifmedia_ioctl(ifp, ifr, &adapter->media, command);
	case SIOCGI2C:
	{
		struct ixgbe_hw *hw = &adapter->hw;
		struct ixgbe_i2c_req	i2c;
		IOCTL_DEBUGOUT("ioctl: SIOCGI2C (Get I2C Data)");
		error = copyin(ifr->ifr_data, &i2c, sizeof(i2c));
		if (error != 0)
			break;
		if (i2c.dev_addr != 0xA0 && i2c.dev_addr != 0xA2) {
			error = EINVAL;
			break;
		}
		if (i2c.len > sizeof(i2c.data)) {
			error = EINVAL;
			break;
		}

		hw->phy.ops.read_i2c_byte(hw, i2c.offset,
		    i2c.dev_addr, i2c.data);
		error = copyout(&i2c, ifr->ifr_data, sizeof(i2c));
		break;
	}
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
#ifdef PCI_IOV
			ixgbe_recalculate_max_frame(adapter);
#endif
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

	return error;
}

/*
 * Set the various hardware offload abilities.
 *
 * This takes the ifnet's if_capenable flags (e.g. set by the user using
 * ifconfig) and indicates to the OS via the ifnet's if_hwassist field what
 * mbuf offload flags the driver will understand.
 */
static void
ixgbe_set_if_hwassist(struct adapter *adapter)
{
	/* XXX */
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
	struct tx_ring  *txr;
	struct rx_ring  *rxr;
	u32		txdctl, mhadd;
	u32		rxdctl, rxctrl;
	int err = 0;
#ifdef PCI_IOV
	enum ixgbe_iov_mode mode;
#endif

	/* XXX check IFF_UP and IFF_RUNNING, power-saving state! */

	KASSERT(mutex_owned(&adapter->core_mtx));
	INIT_DEBUGOUT("ixgbe_init_locked: begin");

	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
        callout_stop(&adapter->timer);

	/* XXX I moved this here from the SIOCSIFMTU case in ixgbe_ioctl(). */
	adapter->max_frame_size =
		ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

#ifdef PCI_IOV
	mode = ixgbe_get_iov_mode(adapter);
	adapter->pool = ixgbe_max_vfs(mode);
	/* Queue indices may change with IOV mode */
	for (int i = 0; i < adapter->num_queues; i++) {
		adapter->rx_rings[i].me = ixgbe_pf_que_index(mode, i);
		adapter->tx_rings[i].me = ixgbe_pf_que_index(mode, i);
	}
#endif
        /* reprogram the RAR[0] in case user changed it. */
	ixgbe_set_rar(hw, 0, hw->mac.addr, adapter->pool, IXGBE_RAH_AV);

	/* Get the latest mac address, User can use a LAA */
	memcpy(hw->mac.addr, CLLADDR(ifp->if_sadl),
	    IXGBE_ETH_LENGTH_OF_ADDRESS);
	ixgbe_set_rar(hw, 0, hw->mac.addr, adapter->pool, 1);
	hw->addr_ctrl.rar_used_count = 1;

	/* Set hardware offload abilities from ifnet flags */
	ixgbe_set_if_hwassist(adapter);

	/* Prepare transmit descriptors and buffers */
	if (ixgbe_setup_transmit_structures(adapter)) {
		device_printf(dev, "Could not setup transmit structures\n");
		ixgbe_stop(adapter);
		return;
	}

	ixgbe_init_hw(hw);
#ifdef PCI_IOV
	ixgbe_initialize_iov(adapter);
#endif
	ixgbe_initialize_transmit_units(adapter);

	/* Setup Multicast table */
	ixgbe_set_multi(adapter);

	/* Determine the correct mbuf pool, based on frame size */
	if (adapter->max_frame_size <= MCLBYTES)
		adapter->rx_mbuf_sz = MCLBYTES;
	else
		adapter->rx_mbuf_sz = MJUMPAGESIZE;

	/* Prepare receive descriptors and buffers */
	if (ixgbe_setup_receive_structures(adapter)) {
		device_printf(dev, "Could not setup receive structures\n");
		ixgbe_stop(adapter);
		return;
	}

	/* Configure RX settings */
	ixgbe_initialize_receive_units(adapter);

	/* Enable SDP & MSIX interrupts based on adapter */
	ixgbe_config_gpie(adapter);

	/* Set MTU size */
	if (ifp->if_mtu > ETHERMTU) {
		/* aka IXGBE_MAXFRS on 82599 and newer */
		mhadd = IXGBE_READ_REG(hw, IXGBE_MHADD);
		mhadd &= ~IXGBE_MHADD_MFS_MASK;
		mhadd |= adapter->max_frame_size << IXGBE_MHADD_MFS_SHIFT;
		IXGBE_WRITE_REG(hw, IXGBE_MHADD, mhadd);
	}
	
	/* Now enable all the queues */
	for (int i = 0; i < adapter->num_queues; i++) {
		txr = &adapter->tx_rings[i];
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(txr->me));
		txdctl |= IXGBE_TXDCTL_ENABLE;
		/* Set WTHRESH to 8, burst writeback */
		txdctl |= (8 << 16);
		/*
		 * When the internal queue falls below PTHRESH (32),
		 * start prefetching as long as there are at least
		 * HTHRESH (1) buffers ready. The values are taken
		 * from the Intel linux driver 3.8.21.
		 * Prefetching enables tx line rate even with 1 queue.
		 */
		txdctl |= (32 << 0) | (1 << 8);
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(txr->me), txdctl);
	}

	for (int i = 0, j = 0; i < adapter->num_queues; i++) {
		rxr = &adapter->rx_rings[i];
		rxdctl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(rxr->me));
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
		IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(rxr->me), rxdctl);
		for (; j < 10; j++) {
			if (IXGBE_READ_REG(hw, IXGBE_RXDCTL(rxr->me)) &
			    IXGBE_RXDCTL_ENABLE)
				break;
			else
				msec_delay(1);
		}
		wmb();
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

			IXGBE_WRITE_REG(hw, IXGBE_RDT(rxr->me), t);
		} else
#endif /* DEV_NETMAP */
		IXGBE_WRITE_REG(hw, IXGBE_RDT(rxr->me), adapter->num_rx_desc - 1);
	}

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
	if (hw->mac.type != ixgbe_mac_82598EB) {
		u32 hdrm = 32 << fdir_pballoc;

		hw->mac.ops.setup_rxpba(hw, 0, hdrm, PBA_STRATEGY_EQUAL);
		ixgbe_init_fdir_signature_82599(&adapter->hw, fdir_pballoc);
	}
#endif

	/*
	 * Check on any SFP devices that
	 * need to be kick-started
	 */
	if (hw->phy.type == ixgbe_phy_none) {
		err = hw->phy.ops.identify(hw);
		if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
                	device_printf(dev,
			    "Unsupported SFP+ module type was detected.\n");
			return;
        	}
	}

	/* Set moderation on the Link interrupt */
	IXGBE_WRITE_REG(hw, IXGBE_EITR(adapter->vector), IXGBE_LINK_ITR);

	/* Configure Energy Efficient Ethernet for supported devices */
	if (hw->mac.ops.setup_eee) {
		err = hw->mac.ops.setup_eee(hw, adapter->eee_enabled);
		if (err)
			device_printf(dev, "Error setting up EEE: %d\n", err);
	}

	/* Enable power to the phy. */
	ixgbe_set_phy_power(hw, TRUE);

	/* Config/Enable Link */
	ixgbe_config_link(adapter);

	/* Hardware Packet Buffer & Flow Control setup */
	ixgbe_config_delay_values(adapter);	

	/* Initialize the FC settings */
	ixgbe_start_hw(hw);

	/* Set up VLAN support and filter */
	ixgbe_setup_vlan_hw_support(adapter);

	/* Setup DMA Coalescing */
	ixgbe_config_dmac(adapter);

	/* And now turn on interrupts */
	ixgbe_enable_intr(adapter);

#ifdef PCI_IOV
	/* Enable the use of the MBX by the VF's */
	{
		u32 reg = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
		reg |= IXGBE_CTRL_EXT_PFRSTD;
		IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, reg);
	}
#endif

	/* Now inform the stack we're ready */
	ifp->if_flags |= IFF_RUNNING;

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

static void
ixgbe_config_gpie(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 gpie;

	gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);

	/* Fan Failure Interrupt */
	if (hw->device_id == IXGBE_DEV_ID_82598AT)
		gpie |= IXGBE_SDP1_GPIEN;

	/*
	 * Module detection (SDP2)
	 * Media ready (SDP1)
	 */
	if (hw->mac.type == ixgbe_mac_82599EB) {
		gpie |= IXGBE_SDP2_GPIEN;
		if (hw->device_id != IXGBE_DEV_ID_82599_QSFP_SF_QP)
			gpie |= IXGBE_SDP1_GPIEN;
	}

	/*
	 * Thermal Failure Detection (X540)
	 * Link Detection (X552 SFP+, X552/X557-AT)
	 */
	if (hw->mac.type == ixgbe_mac_X540 ||
	    hw->device_id == IXGBE_DEV_ID_X550EM_X_SFP ||
	    hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T)
		gpie |= IXGBE_SDP0_GPIEN_X540;

	if (adapter->msix > 1) {
		/* Enable Enhanced MSIX mode */
		gpie |= IXGBE_GPIE_MSIX_MODE;
		gpie |= IXGBE_GPIE_EIAME | IXGBE_GPIE_PBA_SUPPORT |
		    IXGBE_GPIE_OCD;
	}

	IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);
	return;
}

/*
 * Requires adapter->max_frame_size to be set.
 */
static void
ixgbe_config_delay_values(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 rxpb, frame, size, tmp;

	frame = adapter->max_frame_size;

	/* Calculate High Water */
	switch (hw->mac.type) {
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
		tmp = IXGBE_DV_X540(frame, frame);
		break;
	default:
		tmp = IXGBE_DV(frame, frame);
		break;
	}
	size = IXGBE_BT2KB(tmp);
	rxpb = IXGBE_READ_REG(hw, IXGBE_RXPBSIZE(0)) >> 10;
	hw->fc.high_water[0] = rxpb - size;

	/* Now calculate Low Water */
	switch (hw->mac.type) {
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
		tmp = IXGBE_LOW_DV_X540(frame);
		break;
	default:
		tmp = IXGBE_LOW_DV(frame);
		break;
	}
	hw->fc.low_water[0] = IXGBE_BT2KB(tmp);

	hw->fc.requested_mode = adapter->fc;
	hw->fc.pause_time = IXGBE_FC_PAUSE;
	hw->fc.send_xon = TRUE;
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

static void
ixgbe_handle_que(void *context)
{
	struct ix_queue *que = context;
	struct adapter  *adapter = que->adapter;
	struct tx_ring  *txr = que->txr;
	struct ifnet    *ifp = adapter->ifp;

	adapter->handleq.ev_count++;

	if (ifp->if_flags & IFF_RUNNING) {
		ixgbe_rxeof(que);
		IXGBE_TX_LOCK(txr);
		ixgbe_txeof(txr);
#ifndef IXGBE_LEGACY_TX
		if (pcq_peek(txr->txr_interq) != NULL)
			ixgbe_mq_start_locked(ifp, txr);
#else
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			ixgbe_start_locked(txr, ifp);
#endif
		IXGBE_TX_UNLOCK(txr);
	}

	/* Reenable this interrupt */
	if (que->res != NULL)
		ixgbe_enable_queue(adapter, que->msix);
	else
		ixgbe_enable_intr(adapter);
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
	struct ifnet    *ifp = adapter->ifp;
	struct 		tx_ring *txr = adapter->tx_rings;
	bool		more = false;
	u32       	reg_eicr;


	reg_eicr = IXGBE_READ_REG(hw, IXGBE_EICR);

	adapter->stats.pf.legint.ev_count++;
	++que->irqs.ev_count;
	if (reg_eicr == 0) {
		adapter->stats.pf.intzero.ev_count++;
		if ((ifp->if_flags & IFF_UP) != 0)
			ixgbe_enable_intr(adapter);
		return 0;
	}

	if ((ifp->if_flags & IFF_RUNNING) != 0) {
#ifdef __NetBSD__
		/* Don't run ixgbe_rxeof in interrupt context */
		more = true;
#else
		more = ixgbe_rxeof(que);
#endif

		IXGBE_TX_LOCK(txr);
		ixgbe_txeof(txr);
#ifdef IXGBE_LEGACY_TX
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			ixgbe_start_locked(txr, ifp);
#else
		if (pcq_peek(txr->txr_interq) != NULL)
			ixgbe_mq_start_locked(ifp, txr);
#endif
		IXGBE_TX_UNLOCK(txr);
	}

	/* Check for fan failure */
	if ((hw->device_id == IXGBE_DEV_ID_82598AT) &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP1)) {
                device_printf(adapter->dev, "\nCRITICAL: FAN FAILURE!! "
		    "REPLACE IMMEDIATELY!!\n");
		IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
	}

	/* Link status change */
	if (reg_eicr & IXGBE_EICR_LSC)
		softint_schedule(adapter->link_si);

	/* External PHY interrupt */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP0_X540))
		softint_schedule(adapter->phy_si);

	if (more)
#ifndef IXGBE_LEGACY_TX
		softint_schedule(txr->txr_si);
#else
		softint_schedule(que->que_si);
#endif
	else
		ixgbe_enable_intr(adapter);
	return 1;
}


/*********************************************************************
 *
 *  MSIX Queue Interrupt Service routine
 *
 **********************************************************************/
static int
ixgbe_msix_que(void *arg)
{
	struct ix_queue	*que = arg;
	struct adapter  *adapter = que->adapter;
	struct ifnet    *ifp = adapter->ifp;
	struct tx_ring	*txr = que->txr;
	struct rx_ring	*rxr = que->rxr;
	bool		more;
	u32		newitr = 0;

	/* Protect against spurious interrupts */
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return 0;

	ixgbe_disable_queue(adapter, que->msix);
	++que->irqs.ev_count;

#ifdef __NetBSD__
	/* Don't run ixgbe_rxeof in interrupt context */
	more = true;
#else
	more = ixgbe_rxeof(que);
#endif

	IXGBE_TX_LOCK(txr);
	ixgbe_txeof(txr);
#ifdef IXGBE_LEGACY_TX
	if (!IFQ_IS_EMPTY(&adapter->ifp->if_snd))
		ixgbe_start_locked(txr, ifp);
#else
	if (pcq_peek(txr->txr_interq) != NULL)
		ixgbe_mq_start_locked(ifp, txr);
#endif
	IXGBE_TX_UNLOCK(txr);

	/* Do AIM now? */

	if (adapter->enable_aim == FALSE)
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
	if (more)
		softint_schedule(que->que_si);
	else
		ixgbe_enable_queue(adapter, que->msix);
	return 1;
}


static int
ixgbe_msix_link(void *arg)
{
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		reg_eicr, mod_mask;

	++adapter->link_irq.ev_count;

	/* Pause other interrupts */
	IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_OTHER);

	/* First get the cause */
	reg_eicr = IXGBE_READ_REG(hw, IXGBE_EICS);
	/* Be sure the queue bits are not cleared */
	reg_eicr &= ~IXGBE_EICR_RTX_QUEUE;
	/* Clear interrupt with write */
	IXGBE_WRITE_REG(hw, IXGBE_EICR, reg_eicr);

	/* Link status change */
	if (reg_eicr & IXGBE_EICR_LSC) {
		IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_LSC);
		softint_schedule(adapter->link_si);
	}

	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
#ifdef IXGBE_FDIR
		if (reg_eicr & IXGBE_EICR_FLOW_DIR) {
			/* This is probably overkill :) */
			if (!atomic_cmpset_int(&adapter->fdir_reinit, 0, 1))
				return 1;
                	/* Disable the interrupt */
			IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EICR_FLOW_DIR);
			softint_schedule(adapter->fdir_si);
		} else
#endif
		if (reg_eicr & IXGBE_EICR_ECC) {
			device_printf(adapter->dev, "CRITICAL: ECC ERROR!! "
			    "Please Reboot!!\n");
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_ECC);
		}

		/* Check for over temp condition */
		if (reg_eicr & IXGBE_EICR_TS) {
			device_printf(adapter->dev, "CRITICAL: OVER TEMP!! "
			    "PHY IS SHUT DOWN!!\n");
			device_printf(adapter->dev, "System shutdown required!\n");
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_TS);
		}
#ifdef PCI_IOV
		if (reg_eicr & IXGBE_EICR_MAILBOX)
			taskqueue_enqueue(adapter->tq, &adapter->mbx_task);
#endif
	}

	/* Pluggable optics-related interrupt */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_SFP)
		mod_mask = IXGBE_EICR_GPI_SDP0_X540;
	else
		mod_mask = IXGBE_EICR_GPI_SDP2_BY_MAC(hw);

	if (ixgbe_is_sfp(hw)) {
		if (reg_eicr & mod_mask) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR, mod_mask);
			softint_schedule(adapter->mod_si);
		} else if (reg_eicr & IXGBE_EICR_GPI_SDP1_BY_MAC(hw)) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
			softint_schedule(adapter->msf_si);
		}
	}

	/* Check for fan failure */
	if ((hw->device_id == IXGBE_DEV_ID_82598AT) &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP1)) {
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1);
                device_printf(adapter->dev, "\nCRITICAL: FAN FAILURE!! "
		    "REPLACE IMMEDIATELY!!\n");
	}

	/* External PHY interrupt */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T &&
	    (reg_eicr & IXGBE_EICR_GPI_SDP0_X540)) {
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP0_X540);
		softint_schedule(adapter->phy_si);
 	}

	/* Re-enable other interrupts */
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS, IXGBE_EIMS_OTHER);
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
ixgbe_media_status(struct ifnet * ifp, struct ifmediareq * ifmr)
{
	struct adapter *adapter = ifp->if_softc;
	struct ixgbe_hw *hw = &adapter->hw;
	int layer;

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
	layer = adapter->phy_layer;

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_1000BASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_100BASE_TX)
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
	if (layer & IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU ||
	    layer & IXGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_TWINAX | IFM_FDX;
			break;
		}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_LR)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_LR | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_LX | IFM_FDX;
			break;
		}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_LRM)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_LRM | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_LX | IFM_FDX;
			break;
		}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_SR ||
	    layer & IXGBE_PHYSICAL_LAYER_1000BASE_SX)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_SR | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_SX | IFM_FDX;
			break;
		}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_CX4)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_CX4 | IFM_FDX;
			break;
		}
	/*
	** XXX: These need to use the proper media types once
	** they're added.
	*/
#ifndef IFM_ETH_XTYPE
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_SR | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_SX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_CX | IFM_FDX;
			break;
		}
	else if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4
	    || layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_CX4 | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_SX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_CX | IFM_FDX;
			break;
		}
#else
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_KR | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_KX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_KX | IFM_FDX;
			break;
		}
	else if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4
	    || layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
			ifmr->ifm_active |= IFM_10G_KX4 | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_KX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_KX | IFM_FDX;
			break;
		}
#endif
	
	/* If nothing is recognized... */
#if 0
	if (IFM_SUBTYPE(ifmr->ifm_active) == 0)
		ifmr->ifm_active |= IFM_UNKNOWN;
#endif
	
	/* Display current flow control setting used on link */
	if (hw->fc.current_mode == ixgbe_fc_rx_pause ||
	    hw->fc.current_mode == ixgbe_fc_full)
		ifmr->ifm_active |= IFM_ETH_RXPAUSE;
	if (hw->fc.current_mode == ixgbe_fc_tx_pause ||
	    hw->fc.current_mode == ixgbe_fc_full)
		ifmr->ifm_active |= IFM_ETH_TXPAUSE;

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
	struct ixgbe_hw *hw = &adapter->hw;
	ixgbe_link_speed speed = 0;

	INIT_DEBUGOUT("ixgbe_media_change: begin");

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	if (hw->phy.media_type == ixgbe_media_type_backplane)
		return (ENODEV);

	/*
	** We don't actually need to check against the supported
	** media types of the adapter; ifmedia will take care of
	** that for us.
	*/
#ifndef IFM_ETH_XTYPE
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
		case IFM_AUTO:
		case IFM_10G_T:
			speed |= IXGBE_LINK_SPEED_100_FULL;
		case IFM_10G_LRM:
		case IFM_10G_SR: /* KR, too */
		case IFM_10G_LR:
		case IFM_10G_CX4: /* KX4 */
			speed |= IXGBE_LINK_SPEED_1GB_FULL;
		case IFM_10G_TWINAX:
			speed |= IXGBE_LINK_SPEED_10GB_FULL;
			break;
		case IFM_1000_T:
			speed |= IXGBE_LINK_SPEED_100_FULL;
		case IFM_1000_LX:
		case IFM_1000_SX:
		case IFM_1000_CX: /* KX */
			speed |= IXGBE_LINK_SPEED_1GB_FULL;
			break;
		case IFM_100_TX:
			speed |= IXGBE_LINK_SPEED_100_FULL;
			break;
		default:
			goto invalid;
	}
#else
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
		case IFM_AUTO:
		case IFM_10G_T:
			speed |= IXGBE_LINK_SPEED_100_FULL;
		case IFM_10G_LRM:
		case IFM_10G_KR:
		case IFM_10G_LR:
		case IFM_10G_KX4:
			speed |= IXGBE_LINK_SPEED_1GB_FULL;
		case IFM_10G_TWINAX:
			speed |= IXGBE_LINK_SPEED_10GB_FULL;
			break;
		case IFM_1000_T:
			speed |= IXGBE_LINK_SPEED_100_FULL;
		case IFM_1000_LX:
		case IFM_1000_SX:
		case IFM_1000_KX:
			speed |= IXGBE_LINK_SPEED_1GB_FULL;
			break;
		case IFM_100_TX:
			speed |= IXGBE_LINK_SPEED_100_FULL;
			break;
		default:
			goto invalid;
	}
#endif

	hw->mac.autotry_restart = TRUE;
	hw->mac.ops.setup_link(hw, speed, TRUE);
	if (IFM_SUBTYPE(ifm->ifm_media) == IFM_AUTO) {
		adapter->advertise = 0;
	} else {
		if ((speed & IXGBE_LINK_SPEED_10GB_FULL) != 0)
			adapter->advertise |= 1 << 2;
		if ((speed & IXGBE_LINK_SPEED_1GB_FULL) != 0)
			adapter->advertise |= 1 << 1;
		if ((speed & IXGBE_LINK_SPEED_100_FULL) != 0)
			adapter->advertise |= 1 << 0;
	}

	return (0);

invalid:
	device_printf(adapter->dev, "Invalid media type!\n");
	return (EINVAL);
}

static void
ixgbe_set_promisc(struct adapter *adapter)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int32_t       reg_rctl;
	struct ethercom *ec = &adapter->osdep.ec;
	struct ifnet   *ifp = adapter->ifp;
	int		mcnt = 0;

	reg_rctl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	reg_rctl &= (~IXGBE_FCTRL_UPE);
	if (ifp->if_flags & IFF_ALLMULTI)
		mcnt = MAX_NUM_MULTICAST_ADDRESSES;
	else {
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (mcnt == MAX_NUM_MULTICAST_ADDRESSES)
				break;
			mcnt++;
			ETHER_NEXT_MULTI(step, enm);
		}
	}
	if (mcnt < MAX_NUM_MULTICAST_ADDRESSES)
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
	u32			fctrl;
	u8			*update_ptr;
	struct ixgbe_mc_addr	*mta;
	int			mcnt = 0;
	struct ifnet		*ifp = adapter->ifp;
	struct ethercom		*ec = &adapter->osdep.ec;
	struct ether_multi	*enm;
	struct ether_multistep	step;

	IOCTL_DEBUGOUT("ixgbe_set_multi: begin");

	mta = adapter->mta;
	bzero(mta, sizeof(*mta) * MAX_NUM_MULTICAST_ADDRESSES);

	ifp->if_flags &= ~IFF_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if ((mcnt == MAX_NUM_MULTICAST_ADDRESSES) ||
		    (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			ETHER_ADDR_LEN) != 0)) {
			ifp->if_flags |= IFF_ALLMULTI;
			break;
		}
		bcopy(enm->enm_addrlo,
		    mta[mcnt].addr, IXGBE_ETH_LENGTH_OF_ADDRESS);
		mta[mcnt].vmdq = adapter->pool;
		mcnt++;
		ETHER_NEXT_MULTI(step, enm);
	}

	fctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	fctrl &= ~(IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
	if (ifp->if_flags & IFF_PROMISC)
		fctrl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
	else if (ifp->if_flags & IFF_ALLMULTI) {
		fctrl |= IXGBE_FCTRL_MPE;
	}
	
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, fctrl);

	if (mcnt < MAX_NUM_MULTICAST_ADDRESSES) {
		update_ptr = (u8 *)mta;
		ixgbe_update_mc_addr_list(&adapter->hw,
		    update_ptr, mcnt, ixgbe_mc_array_itr, TRUE);
	}

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
	struct ixgbe_mc_addr *mta;

	mta = (struct ixgbe_mc_addr *)*update_ptr;
	*vmdq = mta->vmdq;

	*update_ptr = (u8*)(mta + 1);
	return (mta->addr);
}


/*********************************************************************
 *  Timer routine
 *
 *  This routine checks for link status,updates statistics,
 *  and runs the watchdog check.
 *
 **********************************************************************/

static void
ixgbe_local_timer(void *arg)
{
	struct adapter *adapter = arg;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_local_timer1(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

static void
ixgbe_local_timer1(void *arg)
{
	struct adapter	*adapter = arg;
	device_t	dev = adapter->dev;
	struct ix_queue *que = adapter->queues;
	u64		queues = 0;
	int		hung = 0;

	KASSERT(mutex_owned(&adapter->core_mtx));

	/* Check for pluggable optics */
	if (adapter->sfp_probe)
		if (!ixgbe_sfp_probe(adapter))
			goto out; /* Nothing to do */

	ixgbe_update_link_status(adapter);
	ixgbe_update_stats_counters(adapter);

	/*
	** Check the TX queues status
	**	- mark hung queues so we don't schedule on them
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

	/* Only truely watchdog if all queues show hung */
	if (hung == adapter->num_queues)
		goto watchdog;
	else if (queues != 0) { /* Force an IRQ on queues with work */
		ixgbe_rearm_queues(adapter, queues);
	}

out:
	callout_reset(&adapter->timer, hz, ixgbe_local_timer, adapter);
	return;

watchdog:
	device_printf(adapter->dev, "Watchdog timeout -- resetting\n");
	adapter->ifp->if_flags &= ~IFF_RUNNING;
	adapter->watchdog_events.ev_count++;
	ixgbe_init_locked(adapter);
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
	device_t dev = adapter->dev;

	if (adapter->link_up){ 
		if (adapter->link_active == FALSE) {
			if (bootverbose)
				device_printf(dev,"Link is up %d Gbps %s \n",
				    ((adapter->link_speed == 128)? 10:1),
				    "Full Duplex");
			adapter->link_active = TRUE;
			/* Update any Flow Control changes */
			ixgbe_fc_enable(&adapter->hw);
			/* Update DMA coalescing config */
			ixgbe_config_dmac(adapter);
			if_link_state_change(ifp, LINK_STATE_UP);
#ifdef PCI_IOV
			ixgbe_ping_all_vfs(adapter);
#endif
		}
	} else { /* Link down */
		if (adapter->link_active == TRUE) {
			if (bootverbose)
				device_printf(dev,"Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			adapter->link_active = FALSE;
#ifdef PCI_IOV
			ixgbe_ping_all_vfs(adapter);
#endif
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
	callout_stop(&adapter->timer);

	/* Let the stack know...*/
	ifp->if_flags &= ~IFF_RUNNING;

	ixgbe_reset_hw(hw);
	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
	if (hw->mac.type == ixgbe_mac_82599EB)
		ixgbe_stop_mac_link_on_d3_82599(hw);
	/* Turn off the laser - noop with no optics */
	ixgbe_disable_tx_laser(hw);

	/* Update the stack */
	adapter->link_up = FALSE;
	ixgbe_update_link_status(adapter);

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

	/*
	** Make sure BUSMASTER is set
	*/
	ixgbe_pci_enable_busmaster(pc, tag);

	/* We need this here to set the num_segs below */
	ixgbe_set_mac_type(hw);

	/* Pick up the 82599 settings */
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

	layer = adapter->phy_layer = ixgbe_get_supported_physical_layer(hw);

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_T) {
		adapter->optics = IFM_10G_T;
		return;
	}

	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_T) {
		adapter->optics = IFM_1000_T;
		return;
	}

	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_SX) {
		adapter->optics = IFM_1000_SX;
		return;
	}

	if (layer & (IXGBE_PHYSICAL_LAYER_10GBASE_LR |
	    IXGBE_PHYSICAL_LAYER_10GBASE_LRM)) {
		adapter->optics = IFM_10G_LR;
		return;
	}

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_SR) {
		adapter->optics = IFM_10G_SR;
		return;
	}

	if (layer & IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU) {
		adapter->optics = IFM_10G_TWINAX;
		return;
	}

	if (layer & (IXGBE_PHYSICAL_LAYER_10GBASE_KX4 |
	    IXGBE_PHYSICAL_LAYER_10GBASE_CX4)) {
		adapter->optics = IFM_10G_CX4;
		return;
	}

	/* If we get here just set the default */
	adapter->optics = IFM_ETHER | IFM_AUTO;
	return;
}

/*********************************************************************
 *
 *  Setup the Legacy or MSI Interrupt handler
 *
 **********************************************************************/
static int
ixgbe_allocate_legacy(struct adapter *adapter,
    const struct pci_attach_args *pa)
{
	device_t	dev = adapter->dev;
	struct		ix_queue *que = adapter->queues;
#ifndef IXGBE_LEGACY_TX
	struct tx_ring		*txr = adapter->tx_rings;
#endif
	int		counts[PCI_INTR_TYPE_SIZE];
	pci_intr_type_t intr_type, max_type;
	char intrbuf[PCI_INTRSTR_LEN];
	const char	*intrstr = NULL;
 
	/* Allocation settings */
	max_type = PCI_INTR_TYPE_MSI;
	counts[PCI_INTR_TYPE_MSIX] = 0;
	counts[PCI_INTR_TYPE_MSI] = 1;
	counts[PCI_INTR_TYPE_INTX] = 1;

alloc_retry:
	if (pci_intr_alloc(pa, &adapter->osdep.intrs, counts, max_type) != 0) {
		aprint_error_dev(dev, "couldn't alloc interrupt\n");
		return ENXIO;
	}
	adapter->osdep.nintrs = 1;
	intrstr = pci_intr_string(adapter->osdep.pc, adapter->osdep.intrs[0],
	    intrbuf, sizeof(intrbuf));
	adapter->osdep.ihs[0] = pci_intr_establish_xname(adapter->osdep.pc,
	    adapter->osdep.intrs[0], IPL_NET, ixgbe_legacy_irq, que,
	    device_xname(dev));
	if (adapter->osdep.ihs[0] == NULL) {
		intr_type = pci_intr_type(adapter->osdep.pc,
		    adapter->osdep.intrs[0]);
		aprint_error_dev(dev,"unable to establish %s\n",
		    (intr_type == PCI_INTR_TYPE_MSI) ? "MSI" : "INTx");
		pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs, 1);
		switch (intr_type) {
		case PCI_INTR_TYPE_MSI:
			/* The next try is for INTx: Disable MSI */
			max_type = PCI_INTR_TYPE_INTX;
			counts[PCI_INTR_TYPE_INTX] = 1;
			goto alloc_retry;
		case PCI_INTR_TYPE_INTX:
		default:
			/* See below */
			break;
		}
	}
	if (adapter->osdep.ihs[0] == NULL) {
		aprint_error_dev(dev,
		    "couldn't establish interrupt%s%s\n",
		    intrstr ? " at " : "", intrstr ? intrstr : "");
		pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs, 1);
		return ENXIO;
	}
	aprint_normal_dev(dev, "interrupting at %s\n", intrstr);
	/*
	 * Try allocating a fast interrupt and the associated deferred
	 * processing contexts.
	 */
#ifndef IXGBE_LEGACY_TX
	txr->txr_si = softint_establish(SOFTINT_NET, ixgbe_deferred_mq_start,
	    txr);
#endif
	que->que_si = softint_establish(SOFTINT_NET, ixgbe_handle_que, que);

	/* Tasklets for Link, SFP and Multispeed Fiber */
	adapter->link_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_link, adapter);
	adapter->mod_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_mod, adapter);
	adapter->msf_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_msf, adapter);
	adapter->phy_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_phy, adapter);

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

	/* For simplicity in the handlers */
	adapter->active_queues = IXGBE_EIMS_ENABLE_MASK;

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
	device_t        dev = adapter->dev;
	struct 		ix_queue *que = adapter->queues;
	struct  	tx_ring *txr = adapter->tx_rings;
	pci_chipset_tag_t pc;
	char		intrbuf[PCI_INTRSTR_LEN];
	char		intr_xname[32];
	const char	*intrstr = NULL;
	int 		error, vector = 0;
	int		cpu_id = 0;
	kcpuset_t	*affinity;
#ifdef	RSS
	cpuset_t	cpu_mask;
#endif

	pc = adapter->osdep.pc;
#ifdef	RSS
	/*
	 * If we're doing RSS, the number of queues needs to
	 * match the number of RSS buckets that are configured.
	 *
	 * + If there's more queues than RSS buckets, we'll end
	 *   up with queues that get no traffic.
	 *
	 * + If there's more RSS buckets than queues, we'll end
	 *   up having multiple RSS buckets map to the same queue,
	 *   so there'll be some contention.
	 */
	if (adapter->num_queues != rss_getnumbuckets()) {
		device_printf(dev,
		    "%s: number of queues (%d) != number of RSS buckets (%d)"
		    "; performance will be impacted.\n",
		    __func__,
		    adapter->num_queues,
		    rss_getnumbuckets());
	}
#endif

	adapter->osdep.nintrs = adapter->num_queues + 1;
	if (pci_msix_alloc_exact(pa, &adapter->osdep.intrs,
	    adapter->osdep.nintrs) != 0) {
		aprint_error_dev(dev,
		    "failed to allocate MSI-X interrupt\n");
		return (ENXIO);
	}

	kcpuset_create(&affinity, false);
	for (int i = 0; i < adapter->num_queues; i++, vector++, que++, txr++) {
		snprintf(intr_xname, sizeof(intr_xname), "%s TX/RX",
		    device_xname(dev));
		intrstr = pci_intr_string(pc, adapter->osdep.intrs[i], intrbuf,
		    sizeof(intrbuf));
#ifdef IXG_MPSAFE
		pci_intr_setattr(pc, adapter->osdep.intrs[i], PCI_INTR_MPSAFE,
		    true);
#endif
		/* Set the handler function */
		que->res = adapter->osdep.ihs[i] = pci_intr_establish_xname(pc,
		    adapter->osdep.intrs[i], IPL_NET, ixgbe_msix_que, que,
		    intr_xname);
		if (que->res == NULL) {
			pci_intr_release(pc, adapter->osdep.intrs,
			    adapter->osdep.nintrs);
			aprint_error_dev(dev,
			    "Failed to register QUE handler\n");
			kcpuset_destroy(affinity);
			return ENXIO;
		}
		que->msix = vector;
		adapter->active_queues |= (u64)(1 << que->msix);
#ifdef	RSS
		/*
		 * The queue ID is used as the RSS layer bucket ID.
		 * We look up the queue ID -> RSS CPU ID and select
		 * that.
		 */
		cpu_id = rss_getcpu(i % rss_getnumbuckets());
#else
		/*
		 * Bind the msix vector, and thus the
		 * rings to the corresponding cpu.
		 *
		 * This just happens to match the default RSS round-robin
		 * bucket -> queue -> CPU allocation.
		 */
		if (adapter->num_queues > 1)
			cpu_id = i;
#endif
		/* Round-robin affinity */
		kcpuset_zero(affinity);
		kcpuset_set(affinity, cpu_id % ncpu);
		error = interrupt_distribute(adapter->osdep.ihs[i], affinity,
		    NULL);
		aprint_normal_dev(dev, "for TX/RX, interrupting at %s",
		    intrstr);
		if (error == 0) {
#if 1 /* def IXGBE_DEBUG */
#ifdef	RSS
			aprintf_normal(
			    ", bound RSS bucket %d to CPU %d",
			    i, cpu_id);
#else
			aprint_normal(
			    ", bound queue %d to cpu %d",
			    i, cpu_id);
#endif
#endif /* IXGBE_DEBUG */
		}
		aprint_normal("\n");
#ifndef IXGBE_LEGACY_TX
		txr->txr_si = softint_establish(SOFTINT_NET,
		    ixgbe_deferred_mq_start, txr);
#endif
		que->que_si = softint_establish(SOFTINT_NET, ixgbe_handle_que,
		    que);
		if (que->que_si == NULL) {
			aprint_error_dev(dev,
			    "could not establish software interrupt\n"); 
		}
	}

	/* and Link */
	cpu_id++;
	snprintf(intr_xname, sizeof(intr_xname), "%s link", device_xname(dev));
	intrstr = pci_intr_string(pc, adapter->osdep.intrs[vector], intrbuf,
	    sizeof(intrbuf));
#ifdef IXG_MPSAFE
	pci_intr_setattr(pc, &adapter->osdep.intrs[vector], PCI_INTR_MPSAFE,
	    true);
#endif
	/* Set the link handler function */
	adapter->osdep.ihs[vector] = pci_intr_establish_xname(pc,
	    adapter->osdep.intrs[vector], IPL_NET, ixgbe_msix_link, adapter,
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
		aprint_normal(", affinity to cpu %d\n", cpu_id);
	else
		aprint_normal("\n");

	adapter->vector = vector;
	/* Tasklets for Link, SFP and Multispeed Fiber */
	adapter->link_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_link, adapter);
	adapter->mod_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_mod, adapter);
	adapter->msf_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_msf, adapter);
#ifdef PCI_IOV
	TASK_INIT(&adapter->mbx_task, 0, ixgbe_handle_mbx, adapter);
#endif
	adapter->phy_si =
	    softint_establish(SOFTINT_NET, ixgbe_handle_phy, adapter);
#ifdef IXGBE_FDIR
	adapter->fdir_si =
	    softint_establish(SOFTINT_NET, ixgbe_reinit_fdir, adapter);
#endif

	kcpuset_destroy(affinity);
	return (0);
}

/*
 * Setup Either MSI/X or MSI
 */
static int
ixgbe_setup_msix(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	int want, queues, msgs;

	/* Override by tuneable */
	if (ixgbe_enable_msix == 0)
		goto msi;

	/* First try MSI/X */
	msgs = pci_msix_count(adapter->osdep.pc, adapter->osdep.tag);
	msgs = MIN(msgs, IXG_MAX_NINTR);
	if (msgs < 2)
		goto msi;

	adapter->msix_mem = (void *)1; /* XXX */

	/* Figure out a reasonable auto config value */
	queues = (ncpu > (msgs - 1)) ? (msgs - 1) : ncpu;

#ifdef	RSS
	/* If we're doing RSS, clamp at the number of RSS buckets */
	if (queues > rss_getnumbuckets())
		queues = rss_getnumbuckets();
#endif

	if (ixgbe_num_queues != 0)
		queues = ixgbe_num_queues;
	/* Set max queues to 8 when autoconfiguring */
	else if ((ixgbe_num_queues == 0) && (queues > 8))
		queues = 8;

	/* reflect correct sysctl value */
	ixgbe_num_queues = queues;

	/*
	** Want one vector (RX/TX pair) per queue
	** plus an additional for Link.
	*/
	want = queues + 1;
	if (msgs >= want)
		msgs = want;
	else {
               	aprint_error_dev(dev,
		    "MSIX Configuration Problem, "
		    "%d vectors but %d queues wanted!\n",
		    msgs, want);
		goto msi;
	}
	device_printf(dev,
	    "Using MSIX interrupts with %d vectors\n", msgs);
	adapter->num_queues = queues;
	return (msgs);

	/*
	** If MSIX alloc failed or provided us with
	** less than needed, free and fall through to MSI
	*/
msi:
       	msgs = pci_msi_count(adapter->osdep.pc, adapter->osdep.tag);
	adapter->msix_mem = NULL; /* XXX */
       	msgs = 1;
	aprint_normal_dev(dev, "Using an MSI interrupt\n");
	return (msgs);
}


static int
ixgbe_allocate_pci_resources(struct adapter *adapter,
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

	/* Default to 1 queue if MSI-X setup fails */
	adapter->num_queues = 1;

	/*
	** Now setup MSI or MSI-X, should
	** return us the number of supported
	** vectors. (Will be 1 for MSI)
	*/
	adapter->msix = ixgbe_setup_msix(adapter);
	return (0);
}

static void
ixgbe_free_pci_resources(struct adapter * adapter)
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


	/* Clean the Legacy or Link interrupt last */
	if (adapter->vector) /* we are doing MSIX */
		rid = adapter->vector;
	else
		rid = 0;

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
ixgbe_setup_interface(device_t dev, struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ifnet   *ifp;

	INIT_DEBUGOUT("ixgbe_setup_interface: begin");

	ifp = adapter->ifp = &ec->ec_if;
	strlcpy(ifp->if_xname, device_xname(dev), IFNAMSIZ);
	ifp->if_baudrate = IF_Gbps(10);
	ifp->if_init = ixgbe_init;
	ifp->if_stop = ixgbe_ifstop;
	ifp->if_softc = adapter;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = ixgbe_ioctl;
#if __FreeBSD_version >= 1100045
	/* TSO parameters */
	ifp->if_hw_tsomax = 65518;
	ifp->if_hw_tsomaxsegcount = IXGBE_82599_SCATTER;
	ifp->if_hw_tsomaxsegsize = 2048;
#endif
#ifndef IXGBE_LEGACY_TX
	ifp->if_transmit = ixgbe_mq_start;
#else
	IFQ_SET_MAXLEN(&ifp->if_snd, adapter->num_tx_desc - 2);
#if 0
	ifp->if_snd.ifq_drv_maxlen = adapter->num_tx_desc - 2;
#endif
	IFQ_SET_READY(&ifp->if_snd);
#endif
	ifp->if_start = ixgbe_start;

	if_initialize(ifp);
	ether_ifattach(ifp, adapter->hw.mac.addr);
#ifndef IXGBE_LEGACY_TX
#if 0	/* We use per TX queue softint */
	if_deferred_start_init(ifp, ixgbe_deferred_mq_start);
#endif
#endif
	if_register(ifp);
	ether_set_ifflags_cb(ec, ixgbe_ifflags_cb);

	adapter->max_frame_size =
	    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);

	/* Set capability flags */
	ifp->if_capabilities |= IFCAP_RXCSUM
			     |  IFCAP_TXCSUM
			     |  IFCAP_TSOv4
			     |  IFCAP_TSOv6
			     |  IFCAP_LRO;
	ifp->if_capenable = 0;

	ec->ec_capabilities |= ETHERCAP_VLAN_HWTAGGING
	    		    |  ETHERCAP_VLAN_HWCSUM
	    		    |  ETHERCAP_JUMBO_MTU
	    		    |  ETHERCAP_VLAN_MTU;

	/* Enable the above capabilities by default */
	ec->ec_capenable = ec->ec_capabilities;

	/*
	** Don't turn this on by default, if vlans are
	** created on another pseudo device (eg. lagg)
	** then vlan events are not passed thru, breaking
	** operation, but with HW FILTER off it works. If
	** using vlans directly on the ixgbe driver you can
	** enable this and get full hardware tag filtering.
	*/
	ec->ec_capabilities |= ETHERCAP_VLAN_HWFILTER;

	/*
	 * Specify the media types supported by this adapter and register
	 * callbacks to update media and link information
	 */
	ifmedia_init(&adapter->media, IFM_IMASK, ixgbe_media_change,
		    ixgbe_media_status);

	adapter->phy_layer = ixgbe_get_supported_physical_layer(&adapter->hw);
	ixgbe_add_media_types(adapter);

	/* Set autoselect media by default */
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);

	return (0);
}

static void
ixgbe_add_media_types(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	device_t dev = adapter->dev;
	int layer;

	layer = adapter->phy_layer;

#define	ADD(mm, dd)							\
	ifmedia_add(&adapter->media, IFM_ETHER | (mm), (dd), NULL);

	/* Media types with matching NetBSD media defines */
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_T) {
		ADD(IFM_10G_T, 0);
		ADD(IFM_10G_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_T) {
		ADD(IFM_1000_T, 0);
		ADD(IFM_1000_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_100BASE_TX) {
		ADD(IFM_100_TX, 0);
		ADD(IFM_100_TX | IFM_FDX, 0);
	}

	if (layer & IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU ||
	    layer & IXGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA) {
		ADD(IFM_10G_TWINAX, 0);
		ADD(IFM_10G_TWINAX | IFM_FDX, 0);
	}

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_LR) {
		ADD(IFM_10G_LR, 0);
		ADD(IFM_10G_LR | IFM_FDX, 0);
		if (hw->phy.multispeed_fiber) {
			ADD(IFM_1000_LX, 0);
			ADD(IFM_1000_LX | IFM_FDX, 0);
		}
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_SR) {
		ADD(IFM_10G_SR, 0);
		ADD(IFM_10G_SR | IFM_FDX, 0);
		if (hw->phy.multispeed_fiber) {
			ADD(IFM_1000_SX, 0);
			ADD(IFM_1000_SX | IFM_FDX, 0);
		}
	} else if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_SX) {
		ADD(IFM_1000_SX, 0);
		ADD(IFM_1000_SX | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_CX4) {
		ADD(IFM_10G_CX4, 0);
		ADD(IFM_10G_CX4 | IFM_FDX, 0);
	}

#ifdef IFM_ETH_XTYPE
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR) {
		ADD(IFM_10G_KR, 0);
		ADD(IFM_10G_KR | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4) {
		ADD(AIFM_10G_KX4, 0);
		ADD(AIFM_10G_KX4 | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX) {
		ADD(IFM_1000_KX, 0);
		ADD(IFM_1000_KX | IFM_FDX, 0);
	}
#else
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR) {
		device_printf(dev, "Media supported: 10GbaseKR\n");
		device_printf(dev, "10GbaseKR mapped to 10GbaseSR\n");
		ADD(IFM_10G_SR, 0);
		ADD(IFM_10G_SR | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4) {
		device_printf(dev, "Media supported: 10GbaseKX4\n");
		device_printf(dev, "10GbaseKX4 mapped to 10GbaseCX4\n");
		ADD(IFM_10G_CX4, 0);
		ADD(IFM_10G_CX4 | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX) {
		device_printf(dev, "Media supported: 1000baseKX\n");
		device_printf(dev, "1000baseKX mapped to 1000baseCX\n");
		ADD(IFM_1000_CX, 0);
		ADD(IFM_1000_CX | IFM_FDX, 0);
	}
#endif
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_BX)
		device_printf(dev, "Media supported: 1000baseBX\n");
	/* XXX no ifmedia_set? */
	
	if (hw->device_id == IXGBE_DEV_ID_82598AT) {
		ADD(IFM_1000_T | IFM_FDX, 0);
		ADD(IFM_1000_T, 0);
	}

	ADD(IFM_AUTO, 0);

#undef ADD
}

static void
ixgbe_config_link(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32	autoneg, err = 0;
	bool	sfp, negotiate;

	sfp = ixgbe_is_sfp(hw);

	if (sfp) { 
		kpreempt_disable();
		softint_schedule(adapter->mod_si);
		kpreempt_enable();
	} else {
		if (hw->mac.ops.check_link)
			err = ixgbe_check_link(hw, &adapter->link_speed,
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
                	err = hw->mac.ops.setup_link(hw,
			    autoneg, adapter->link_up);
	}
out:
	return;
}


/*********************************************************************
 *
 *  Enable transmit units.
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
		u32	txctrl = 0;
		int	j = txr->me;

		IXGBE_WRITE_REG(hw, IXGBE_TDBAL(j),
		       (tdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_TDBAH(j), (tdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_TDLEN(j),
		    adapter->num_tx_desc * sizeof(union ixgbe_adv_tx_desc));

		/* Setup the HW Tx Head and Tail descriptor pointers */
		IXGBE_WRITE_REG(hw, IXGBE_TDH(j), 0);
		IXGBE_WRITE_REG(hw, IXGBE_TDT(j), 0);

		/* Cache the tail address */
		txr->tail = IXGBE_TDT(j);

		/* Disable Head Writeback */
		/*
		 * Note: for X550 series devices, these registers are actually
		 * prefixed with TPH_ isntead of DCA_, but the addresses and
		 * fields remain the same.
		 */
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL(j));
			break;
		default:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(j));
			break;
                }
		txctrl &= ~IXGBE_DCA_TXCTRL_DESC_WRO_EN;
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL(j), txctrl);
			break;
		default:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(j), txctrl);
			break;
		}

	}

	if (hw->mac.type != ixgbe_mac_82598EB) {
		u32 dmatxctl, rttdcs;
#ifdef PCI_IOV
		enum ixgbe_iov_mode mode = ixgbe_get_iov_mode(adapter);
#endif
		dmatxctl = IXGBE_READ_REG(hw, IXGBE_DMATXCTL);
		dmatxctl |= IXGBE_DMATXCTL_TE;
		IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL, dmatxctl);
		/* Disable arbiter to set MTQC */
		rttdcs = IXGBE_READ_REG(hw, IXGBE_RTTDCS);
		rttdcs |= IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
#ifdef PCI_IOV
		IXGBE_WRITE_REG(hw, IXGBE_MTQC, ixgbe_get_mtqc(mode));
#else
		IXGBE_WRITE_REG(hw, IXGBE_MTQC, IXGBE_MTQC_64Q_1PB);
#endif
		rttdcs &= ~IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
	}

	return;
}

static void
ixgbe_initialize_rss_mapping(struct adapter *adapter)
{
	struct ixgbe_hw	*hw = &adapter->hw;
	u32 reta = 0, mrqc, rss_key[10];
	int queue_id, table_size, index_mult;
#ifdef	RSS
	u32 rss_hash_config;
#endif
#ifdef PCI_IOV
	enum ixgbe_iov_mode mode;
#endif

#ifdef	RSS
	/* Fetch the configured RSS key */
	rss_getkey((uint8_t *) &rss_key);
#else
	/* set up random bits */
	cprng_fast(&rss_key, sizeof(rss_key));
#endif

	/* Set multiplier for RETA setup and table size based on MAC */
	index_mult = 0x1;
	table_size = 128;
	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82598EB:
		index_mult = 0x11;
		break;
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
		table_size = 512;
		break;
	default:
		break;
	}

	/* Set up the redirection table */
	for (int i = 0, j = 0; i < table_size; i++, j++) {
		if (j == adapter->num_queues) j = 0;
#ifdef	RSS
		/*
		 * Fetch the RSS bucket id for the given indirection entry.
		 * Cap it at the number of configured buckets (which is
		 * num_queues.)
		 */
		queue_id = rss_get_indirection_to_bucket(i);
		queue_id = queue_id % adapter->num_queues;
#else
		queue_id = (j * index_mult);
#endif
		/*
		 * The low 8 bits are for hash value (n+0);
		 * The next 8 bits are for hash value (n+1), etc.
		 */
		reta = reta >> 8;
		reta = reta | ( ((uint32_t) queue_id) << 24);
		if ((i & 3) == 3) {
			if (i < 128)
				IXGBE_WRITE_REG(hw, IXGBE_RETA(i >> 2), reta);
			else
				IXGBE_WRITE_REG(hw, IXGBE_ERETA((i >> 2) - 32), reta);
			reta = 0;
		}
	}

	/* Now fill our hash function seeds */
	for (int i = 0; i < 10; i++)
		IXGBE_WRITE_REG(hw, IXGBE_RSSRK(i), rss_key[i]);

	/* Perform hash on these packet types */
#ifdef	RSS
	mrqc = IXGBE_MRQC_RSSEN;
	rss_hash_config = rss_gethashconfig();
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_IPV6_EX)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV6_EX)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV4_EX)
		device_printf(adapter->dev,
		    "%s: RSS_HASHTYPE_RSS_UDP_IPV4_EX defined, "
		    "but not supported\n", __func__);
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6_EX)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX_UDP;
#else
	/*
	 * Disable UDP - IP fragments aren't currently being handled
	 * and so we end up with a mix of 2-tuple and 4-tuple
	 * traffic.
	 */
	mrqc = IXGBE_MRQC_RSSEN
	     | IXGBE_MRQC_RSS_FIELD_IPV4
	     | IXGBE_MRQC_RSS_FIELD_IPV4_TCP
	     | IXGBE_MRQC_RSS_FIELD_IPV6_EX_TCP
	     | IXGBE_MRQC_RSS_FIELD_IPV6_EX
	     | IXGBE_MRQC_RSS_FIELD_IPV6
	     | IXGBE_MRQC_RSS_FIELD_IPV6_TCP
	;
#endif /* RSS */
#ifdef PCI_IOV
	mode = ixgbe_get_iov_mode(adapter);
	mrqc |= ixgbe_get_mrqc(mode);
#endif
	IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);
}


/*********************************************************************
 *
 *  Setup receive registers and features.
 *
 **********************************************************************/
#define IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT 2

#define BSIZEPKT_ROUNDUP ((1<<IXGBE_SRRCTL_BSIZEPKT_SHIFT)-1)
	
static void
ixgbe_initialize_receive_units(struct adapter *adapter)
{
	int i;
	struct	rx_ring	*rxr = adapter->rx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet   *ifp = adapter->ifp;
	u32		bufsz, fctrl, srrctl, rxcsum;
	u32		hlreg;

	/*
	 * Make sure receives are disabled while
	 * setting up the descriptor ring
	 */
	ixgbe_disable_rx(hw);

	/* Enable broadcasts */
	fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
	fctrl |= IXGBE_FCTRL_BAM;
	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		fctrl |= IXGBE_FCTRL_DPF;
		fctrl |= IXGBE_FCTRL_PMCF;
	}
	IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);

	/* Set for Jumbo Frames? */
	hlreg = IXGBE_READ_REG(hw, IXGBE_HLREG0);
	if (ifp->if_mtu > ETHERMTU)
		hlreg |= IXGBE_HLREG0_JUMBOEN;
	else
		hlreg &= ~IXGBE_HLREG0_JUMBOEN;
#ifdef DEV_NETMAP
	/* crcstrip is conditional in netmap (in RDRXCTL too ?) */
	if (ifp->if_capenable & IFCAP_NETMAP && !ix_crcstrip)
		hlreg &= ~IXGBE_HLREG0_RXCRCSTRP;
	else
		hlreg |= IXGBE_HLREG0_RXCRCSTRP;
#endif /* DEV_NETMAP */
	IXGBE_WRITE_REG(hw, IXGBE_HLREG0, hlreg);

	bufsz = (adapter->rx_mbuf_sz +
	    BSIZEPKT_ROUNDUP) >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;

	for (i = 0; i < adapter->num_queues; i++, rxr++) {
		u64 rdba = rxr->rxdma.dma_paddr;
		int j = rxr->me;

		/* Setup the Base and Length of the Rx Descriptor Ring */
		IXGBE_WRITE_REG(hw, IXGBE_RDBAL(j),
			       (rdba & 0x00000000ffffffffULL));
		IXGBE_WRITE_REG(hw, IXGBE_RDBAH(j), (rdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_RDLEN(j),
		    adapter->num_rx_desc * sizeof(union ixgbe_adv_rx_desc));

		/* Set up the SRRCTL register */
		srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(j));
		srrctl &= ~IXGBE_SRRCTL_BSIZEHDR_MASK;
		srrctl &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;
		srrctl |= bufsz;
		srrctl |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;

		/*
		 * Set DROP_EN iff we have no flow control and >1 queue.
		 * Note that srrctl was cleared shortly before during reset,
		 * so we do not need to clear the bit, but do it just in case
		 * this code is moved elsewhere.
		 */
		if (adapter->num_queues > 1 &&
		    adapter->hw.fc.requested_mode == ixgbe_fc_none) {
			srrctl |= IXGBE_SRRCTL_DROP_EN;
		} else {
			srrctl &= ~IXGBE_SRRCTL_DROP_EN;
		}

		IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(j), srrctl);

		/* Setup the HW Rx Head and Tail Descriptor Pointers */
		IXGBE_WRITE_REG(hw, IXGBE_RDH(j), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RDT(j), 0);

		/* Set the driver rx tail address */
		rxr->tail =  IXGBE_RDT(rxr->me);
	}

	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
		u32 psrtype = IXGBE_PSRTYPE_TCPHDR |
			      IXGBE_PSRTYPE_UDPHDR |
			      IXGBE_PSRTYPE_IPV4HDR |
			      IXGBE_PSRTYPE_IPV6HDR;
		IXGBE_WRITE_REG(hw, IXGBE_PSRTYPE(0), psrtype);
	}

	rxcsum = IXGBE_READ_REG(hw, IXGBE_RXCSUM);

	ixgbe_initialize_rss_mapping(adapter);

	if (adapter->num_queues > 1) {
		/* RSS and RX IPP Checksum are mutually exclusive */
		rxcsum |= IXGBE_RXCSUM_PCSD;
	}

	if (ifp->if_capenable & IFCAP_RXCSUM)
		rxcsum |= IXGBE_RXCSUM_PCSD;

	/* This is useful for calculating UDP/IP fragment checksums */
	if (!(rxcsum & IXGBE_RXCSUM_PCSD))
		rxcsum |= IXGBE_RXCSUM_IPPCSE;

	IXGBE_WRITE_REG(hw, IXGBE_RXCSUM, rxcsum);

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
	ixgbe_setup_vlan_hw_support(adapter);
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
	ixgbe_setup_vlan_hw_support(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}
#endif

static void
ixgbe_setup_vlan_hw_support(struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	struct rx_ring	*rxr;
	u32		ctrl;


	/*
	** We get here thru init_locked, meaning
	** a soft reset, this has already cleared
	** the VFTA and other state, so if there
	** have been no vlan's registered do nothing.
	*/
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		return;

	/* Setup the queues for vlans */
	for (int i = 0; i < adapter->num_queues; i++) {
		rxr = &adapter->rx_rings[i];
		/* On 82599 the VLAN enable is per/queue in RXDCTL */
		if (hw->mac.type != ixgbe_mac_82598EB) {
			ctrl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(rxr->me));
			ctrl |= IXGBE_RXDCTL_VME;
			IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(rxr->me), ctrl);
		}
		rxr->vtag_strip = TRUE;
	}

	if ((ec->ec_capenable & ETHERCAP_VLAN_HWFILTER) == 0)
		return;
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
}

static void
ixgbe_enable_intr(struct adapter *adapter)
{
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ix_queue	*que = adapter->queues;
	u32		mask, fwsm;

	mask = (IXGBE_EIMS_ENABLE_MASK & ~IXGBE_EIMS_RTX_QUEUE);
	/* Enable Fan Failure detection */
	if (hw->device_id == IXGBE_DEV_ID_82598AT)
		    mask |= IXGBE_EIMS_GPI_SDP1;

	switch (adapter->hw.mac.type) {
		case ixgbe_mac_82599EB:
			mask |= IXGBE_EIMS_ECC;
			/* Temperature sensor on some adapters */
			mask |= IXGBE_EIMS_GPI_SDP0;
			/* SFP+ (RX_LOS_N & MOD_ABS_N) */
			mask |= IXGBE_EIMS_GPI_SDP1;
			mask |= IXGBE_EIMS_GPI_SDP2;
#ifdef IXGBE_FDIR
			mask |= IXGBE_EIMS_FLOW_DIR;
#endif
#ifdef PCI_IOV
			mask |= IXGBE_EIMS_MAILBOX;
#endif
			break;
		case ixgbe_mac_X540:
			/* Detect if Thermal Sensor is enabled */
			fwsm = IXGBE_READ_REG(hw, IXGBE_FWSM);
			if (fwsm & IXGBE_FWSM_TS_ENABLED)
				mask |= IXGBE_EIMS_TS;
			mask |= IXGBE_EIMS_ECC;
#ifdef IXGBE_FDIR
			mask |= IXGBE_EIMS_FLOW_DIR;
#endif
			break;
		case ixgbe_mac_X550:
		case ixgbe_mac_X550EM_x:
			/* MAC thermal sensor is automatically enabled */
			mask |= IXGBE_EIMS_TS;
			/* Some devices use SDP0 for important information */
			if (hw->device_id == IXGBE_DEV_ID_X550EM_X_SFP ||
			    hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T)
				mask |= IXGBE_EIMS_GPI_SDP0_BY_MAC(hw);
			mask |= IXGBE_EIMS_ECC;
#ifdef IXGBE_FDIR
			mask |= IXGBE_EIMS_FLOW_DIR;
#endif
#ifdef PCI_IOV
			mask |= IXGBE_EIMS_MAILBOX;
#endif
		/* falls through */
		default:
			break;
	}

	IXGBE_WRITE_REG(hw, IXGBE_EIMS, mask);

	/* With MSI-X we use auto clear */
	if (adapter->msix_mem) {
		mask = IXGBE_EIMS_ENABLE_MASK;
		/* Don't autoclear Link */
		mask &= ~IXGBE_EIMS_OTHER;
		mask &= ~IXGBE_EIMS_LSC;
#ifdef PCI_IOV
		mask &= ~IXGBE_EIMS_MAILBOX;
#endif
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

/*
** Get the width and transaction speed of
** the slot this adapter is plugged into.
*/
static void
ixgbe_get_slot_info(struct adapter *adapter)
{
	device_t		dev = adapter->dev;
	struct ixgbe_hw		*hw = &adapter->hw;
	struct ixgbe_mac_info	*mac = &hw->mac;
	u16			link;

	/* For most devices simply call the shared code routine */
	if (hw->device_id != IXGBE_DEV_ID_82599_SFP_SF_QP) {
		ixgbe_get_bus_info(hw);
		/* These devices don't use PCI-E */
		switch (hw->mac.type) {
		case ixgbe_mac_X550EM_x:
			return;
		default:
			goto display;
		}
	}

	/*
	** For the Quad port adapter we need to parse back
	** up the PCI tree to find the speed of the expansion
	** slot into which this adapter is plugged. A bit more work.
	*/
	dev = device_parent(device_parent(dev));
#ifdef IXGBE_DEBUG
	device_printf(dev, "parent pcib = %x,%x,%x\n",
	    pci_get_bus(dev), pci_get_slot(dev), pci_get_function(dev));
#endif
	dev = device_parent(device_parent(dev));
#ifdef IXGBE_DEBUG
	device_printf(dev, "slot pcib = %x,%x,%x\n",
	    pci_get_bus(dev), pci_get_slot(dev), pci_get_function(dev));
#endif
	/* Now get the PCI Express Capabilities offset */
	/* ...and read the Link Status Register */
	link = IXGBE_READ_PCIE_WORD(hw, IXGBE_PCI_LINK_STATUS);
	switch (link & IXGBE_PCI_LINK_WIDTH) {
	case IXGBE_PCI_LINK_WIDTH_1:
		hw->bus.width = ixgbe_bus_width_pcie_x1;
		break;
	case IXGBE_PCI_LINK_WIDTH_2:
		hw->bus.width = ixgbe_bus_width_pcie_x2;
		break;
	case IXGBE_PCI_LINK_WIDTH_4:
		hw->bus.width = ixgbe_bus_width_pcie_x4;
		break;
	case IXGBE_PCI_LINK_WIDTH_8:
		hw->bus.width = ixgbe_bus_width_pcie_x8;
		break;
	default:
		hw->bus.width = ixgbe_bus_width_unknown;
		break;
	}

	switch (link & IXGBE_PCI_LINK_SPEED) {
	case IXGBE_PCI_LINK_SPEED_2500:
		hw->bus.speed = ixgbe_bus_speed_2500;
		break;
	case IXGBE_PCI_LINK_SPEED_5000:
		hw->bus.speed = ixgbe_bus_speed_5000;
		break;
	case IXGBE_PCI_LINK_SPEED_8000:
		hw->bus.speed = ixgbe_bus_speed_8000;
		break;
	default:
		hw->bus.speed = ixgbe_bus_speed_unknown;
		break;
	}

	mac->ops.set_lan_id(hw);

display:
	device_printf(dev,"PCI Express Bus: Speed %s Width %s\n",
	    ((hw->bus.speed == ixgbe_bus_speed_8000) ? "8.0GT/s":
	    (hw->bus.speed == ixgbe_bus_speed_5000) ? "5.0GT/s":
	    (hw->bus.speed == ixgbe_bus_speed_2500) ? "2.5GT/s":"Unknown"),
	    (hw->bus.width == ixgbe_bus_width_pcie_x8) ? "x8" :
	    (hw->bus.width == ixgbe_bus_width_pcie_x4) ? "x4" :
	    (hw->bus.width == ixgbe_bus_width_pcie_x1) ? "x1" :
	    ("Unknown"));

	if ((hw->device_id != IXGBE_DEV_ID_82599_SFP_SF_QP) &&
	    ((hw->bus.width <= ixgbe_bus_width_pcie_x4) &&
	    (hw->bus.speed == ixgbe_bus_speed_2500))) {
		device_printf(dev, "PCI-Express bandwidth available"
		    " for this card\n     is not sufficient for"
		    " optimal performance.\n");
		device_printf(dev, "For optimal performance a x8 "
		    "PCIE, or x4 PCIE Gen2 slot is required.\n");
        }
	if ((hw->device_id == IXGBE_DEV_ID_82599_SFP_SF_QP) &&
	    ((hw->bus.width <= ixgbe_bus_width_pcie_x8) &&
	    (hw->bus.speed < ixgbe_bus_speed_8000))) {
		device_printf(dev, "PCI-Express bandwidth available"
		    " for this card\n     is not sufficient for"
		    " optimal performance.\n");
		device_printf(dev, "For optimal performance a x8 "
		    "PCIE Gen3 slot is required.\n");
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
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
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
	struct  ix_queue	*que = adapter->queues;
	u32			newitr;

	if (ixgbe_max_interrupt_rate > 0)
		newitr = (4000000 / ixgbe_max_interrupt_rate) & 0x0FF8;
	else {
		/*
		** Disable DMA coalescing if interrupt moderation is
		** disabled.
		*/
		adapter->dmac = 0;
		newitr = 0;
	}

        for (int i = 0; i < adapter->num_queues; i++, que++) {
		struct rx_ring *rxr = &adapter->rx_rings[i];
		struct tx_ring *txr = &adapter->tx_rings[i];
		/* First the RX queue entry */
                ixgbe_set_ivar(adapter, rxr->me, que->msix, 0);
		/* ... and the TX */
		ixgbe_set_ivar(adapter, txr->me, que->msix, 1);
		/* Set an Initial EITR value */
                IXGBE_WRITE_REG(&adapter->hw,
                    IXGBE_EITR(que->msix), newitr);
	}

	/* For the Link interrupt */
        ixgbe_set_ivar(adapter, 1, adapter->vector, -1);
}

/*
** ixgbe_sfp_probe - called in the local timer to
** determine if a port had optics inserted.
*/  
static bool
ixgbe_sfp_probe(struct adapter *adapter)
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
			device_printf(dev, "SFP+ module detected!\n");
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
	struct ixgbe_hw *hw = &adapter->hw;

	ixgbe_check_link(hw,
	    &adapter->link_speed, &adapter->link_up, 0);
	ixgbe_update_link_status(adapter);

	/* Re-enable link interrupts */
	IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMS_LSC);
}

/*
** Tasklet for handling SFP module interrupts
*/
static void
ixgbe_handle_mod(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	enum ixgbe_phy_type orig_type = hw->phy.type;
	device_t	dev = adapter->dev;
	u32 err;

	IXGBE_CORE_LOCK(adapter);

	/* Check to see if the PHY type changed */
	if (hw->phy.ops.identify) {
		hw->phy.type = ixgbe_phy_unknown;
		hw->phy.ops.identify(hw);
	}

	if (hw->phy.type != orig_type) {
		device_printf(dev, "Detected phy_type %d\n", hw->phy.type);

		if (hw->phy.type == ixgbe_phy_none) {
			hw->phy.sfp_type = ixgbe_sfp_type_unknown;
			goto out;
		}

		/* Try to do the initialization that was skipped before */
		if (hw->phy.ops.init)
			hw->phy.ops.init(hw);
		if (hw->phy.ops.reset)
			hw->phy.ops.reset(hw);
	}

	err = hw->phy.ops.identify_sfp(hw);
	if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		device_printf(dev,
		    "Unsupported SFP+ module type was detected.\n");
		goto out;
	}

	err = hw->mac.ops.setup_sfp(hw);
	if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		device_printf(dev,
		    "Setup failure - unsupported SFP+ module type.\n");
		goto out;
	}
out:
	/* Update media type */
	switch (hw->mac.ops.get_media_type(hw)) {
		case ixgbe_media_type_fiber:
			adapter->optics = IFM_10G_SR;
			break;
		case ixgbe_media_type_copper:
			adapter->optics = IFM_10G_TWINAX;
			break;
		case ixgbe_media_type_cx4:
			adapter->optics = IFM_10G_CX4;
			break;
		default:
			adapter->optics = 0;
			break;
	}

	/* Adjust media types shown in ifconfig */
	ifmedia_removeall(&adapter->media);
	/* get_supported_phy_layer will call hw->phy.ops.identify_sfp() */
	adapter->phy_layer = ixgbe_get_supported_physical_layer(hw);
	ixgbe_add_media_types(adapter);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);

	softint_schedule(adapter->msf_si);
	IXGBE_CORE_UNLOCK(adapter);
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

	IXGBE_CORE_LOCK(adapter);

	autoneg = hw->phy.autoneg_advertised;
	if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
		hw->mac.ops.get_link_capabilities(hw, &autoneg, &negotiate);
	else
		negotiate = 0;
	if (hw->mac.ops.setup_link)
		hw->mac.ops.setup_link(hw, autoneg, TRUE);

	IXGBE_CORE_UNLOCK(adapter);
	return;
}

/*
** Tasklet for handling interrupts from an external PHY
*/
static void
ixgbe_handle_phy(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	int error;

	error = hw->phy.ops.handle_lasi(hw);
	if (error == IXGBE_ERR_OVERTEMP)
		device_printf(adapter->dev,
		    "CRITICAL: EXTERNAL PHY OVER TEMP!! "
		    " PHY will downshift to lower power state!\n");
	else if (error)
		device_printf(adapter->dev,
		    "Error handling LASI interrupt: %d\n",
		    error);
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
	/* re-enable flow director interrupts */
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS, IXGBE_EIMS_FLOW_DIR);
	/* Restart the interface */
	ifp->if_flags |= IFF_RUNNING;
	return;
}
#endif

/*********************************************************************
 *
 *  Configure DMA Coalescing
 *
 **********************************************************************/
static void
ixgbe_config_dmac(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_dmac_config *dcfg = &hw->mac.dmac_config;

	if (hw->mac.type < ixgbe_mac_X550 ||
	    !hw->mac.ops.dmac_config)
		return;

	if (dcfg->watchdog_timer ^ adapter->dmac ||
	    dcfg->link_speed ^ adapter->link_speed) {
		dcfg->watchdog_timer = adapter->dmac;
		dcfg->fcoe_en = false;
		dcfg->link_speed = adapter->link_speed;
		dcfg->num_tcs = 1;
		
		INIT_DEBUGOUT2("dmac settings: watchdog %d, link speed %d\n",
		    dcfg->watchdog_timer, dcfg->link_speed);

		hw->mac.ops.dmac_config(hw);
	}
}

/*
 * Checks whether the adapter's ports are capable of
 * Wake On LAN by reading the adapter's NVM.
 *
 * Sets each port's hw->wol_enabled value depending
 * on the value read here.
 */
static void
ixgbe_check_wol_support(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u16 dev_caps = 0;

	/* Find out WoL support for port */
	adapter->wol_support = hw->wol_enabled = 0;
	ixgbe_get_device_caps(hw, &dev_caps);
	if ((dev_caps & IXGBE_DEVICE_CAPS_WOL_PORT0_1) ||
	    ((dev_caps & IXGBE_DEVICE_CAPS_WOL_PORT0) &&
	      hw->bus.func == 0))
		adapter->wol_support = hw->wol_enabled = 1;

	/* Save initial wake up filter configuration */
	adapter->wufc = IXGBE_READ_REG(hw, IXGBE_WUFC);

	return;
}

/*
 * Prepare the adapter/port for LPLU and/or WoL
 */
static int
ixgbe_setup_low_power_mode(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	device_t dev = adapter->dev;
	s32 error = 0;

	KASSERT(mutex_owned(&adapter->core_mtx));

	/* Limit power management flow to X550EM baseT */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T
	    && hw->phy.ops.enter_lplu) {
		/* X550EM baseT adapters need a special LPLU flow */
		hw->phy.reset_disable = true;
		ixgbe_stop(adapter);
		error = hw->phy.ops.enter_lplu(hw);
		if (error)
			device_printf(dev,
			    "Error entering LPLU: %d\n", error);
		hw->phy.reset_disable = false;
	} else {
		/* Just stop for other adapters */
		ixgbe_stop(adapter);
	}

	if (!hw->wol_enabled) {
		ixgbe_set_phy_power(hw, FALSE);
		IXGBE_WRITE_REG(hw, IXGBE_WUFC, 0);
		IXGBE_WRITE_REG(hw, IXGBE_WUC, 0);
	} else {
		/* Turn off support for APM wakeup. (Using ACPI instead) */
		IXGBE_WRITE_REG(hw, IXGBE_GRC,
		    IXGBE_READ_REG(hw, IXGBE_GRC) & ~(u32)2);

		/*
		 * Clear Wake Up Status register to prevent any previous wakeup
		 * events from waking us up immediately after we suspend.
		 */
		IXGBE_WRITE_REG(hw, IXGBE_WUS, 0xffffffff);

		/*
		 * Program the Wakeup Filter Control register with user filter
		 * settings
		 */
		IXGBE_WRITE_REG(hw, IXGBE_WUFC, adapter->wufc);

		/* Enable wakeups and power management in Wakeup Control */
		IXGBE_WRITE_REG(hw, IXGBE_WUC,
		    IXGBE_WUC_WKEN | IXGBE_WUC_PME_EN);

	}

	return error;
}

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
	u32 missed_rx = 0, bprc, lxon, lxoff, total;
	u64 total_missed_rx = 0;
	uint64_t crcerrs, rlec;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;

	crcerrs = IXGBE_READ_REG(hw, IXGBE_CRCERRS);
	stats->crcerrs.ev_count += crcerrs;
	stats->illerrc.ev_count += IXGBE_READ_REG(hw, IXGBE_ILLERRC);
	stats->errbc.ev_count += IXGBE_READ_REG(hw, IXGBE_ERRBC);
	stats->mspdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MSPDC);

	for (int i = 0; i < __arraycount(stats->qprc); i++) {
		int j = i % adapter->num_queues;
		stats->qprc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRC(i));
		stats->qptc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPTC(i));
		stats->qprdc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
	}
	stats->mlfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MLFC);
	stats->mrfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MRFC);
	rlec = IXGBE_READ_REG(hw, IXGBE_RLEC);
	stats->rlec.ev_count += rlec;

	/* Hardware workaround, gprc counts missed packets */
	stats->gprc.ev_count += IXGBE_READ_REG(hw, IXGBE_GPRC) - missed_rx;

	lxon = IXGBE_READ_REG(hw, IXGBE_LXONTXC);
	stats->lxontxc.ev_count += lxon;
	lxoff = IXGBE_READ_REG(hw, IXGBE_LXOFFTXC);
	stats->lxofftxc.ev_count += lxoff;
	total = lxon + lxoff;

	if (hw->mac.type != ixgbe_mac_82598EB) {
		stats->gorc.ev_count += IXGBE_READ_REG(hw, IXGBE_GORCL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_GORCH) << 32);
		stats->gotc.ev_count += IXGBE_READ_REG(hw, IXGBE_GOTCL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_GOTCH) << 32) - total * ETHER_MIN_LEN;
		stats->tor.ev_count += IXGBE_READ_REG(hw, IXGBE_TORL) +
		    ((u64)IXGBE_READ_REG(hw, IXGBE_TORH) << 32);
		stats->lxonrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXONRXCNT);
		stats->lxoffrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXOFFRXCNT);
	} else {
		stats->lxonrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXONRXC);
		stats->lxoffrxc.ev_count += IXGBE_READ_REG(hw, IXGBE_LXOFFRXC);
		/* 82598 only has a counter in the high register */
		stats->gorc.ev_count += IXGBE_READ_REG(hw, IXGBE_GORCH);
		stats->gotc.ev_count += IXGBE_READ_REG(hw, IXGBE_GOTCH) - total * ETHER_MIN_LEN;
		stats->tor.ev_count += IXGBE_READ_REG(hw, IXGBE_TORH);
	}

	/*
	 * Workaround: mprc hardware is incorrectly counting
	 * broadcasts, so for now we subtract those.
	 */
	bprc = IXGBE_READ_REG(hw, IXGBE_BPRC);
	stats->bprc.ev_count += bprc;
	stats->mprc.ev_count += IXGBE_READ_REG(hw, IXGBE_MPRC) - ((hw->mac.type == ixgbe_mac_82598EB) ? bprc : 0);

	stats->prc64.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC64);
	stats->prc127.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC127);
	stats->prc255.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC255);
	stats->prc511.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC511);
	stats->prc1023.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC1023);
	stats->prc1522.ev_count += IXGBE_READ_REG(hw, IXGBE_PRC1522);

	stats->gptc.ev_count += IXGBE_READ_REG(hw, IXGBE_GPTC) - total;
	stats->mptc.ev_count += IXGBE_READ_REG(hw, IXGBE_MPTC) - total;
	stats->ptc64.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC64) - total;

	stats->ruc.ev_count += IXGBE_READ_REG(hw, IXGBE_RUC);
	stats->rfc.ev_count += IXGBE_READ_REG(hw, IXGBE_RFC);
	stats->roc.ev_count += IXGBE_READ_REG(hw, IXGBE_ROC);
	stats->rjc.ev_count += IXGBE_READ_REG(hw, IXGBE_RJC);
	stats->mngprc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPRC);
	stats->mngpdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPDC);
	stats->mngptc.ev_count += IXGBE_READ_REG(hw, IXGBE_MNGPTC);
	stats->tpr.ev_count += IXGBE_READ_REG(hw, IXGBE_TPR);
	stats->tpt.ev_count += IXGBE_READ_REG(hw, IXGBE_TPT);
	stats->ptc127.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC127);
	stats->ptc255.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC255);
	stats->ptc511.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC511);
	stats->ptc1023.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC1023);
	stats->ptc1522.ev_count += IXGBE_READ_REG(hw, IXGBE_PTC1522);
	stats->bptc.ev_count += IXGBE_READ_REG(hw, IXGBE_BPTC);
	stats->xec.ev_count += IXGBE_READ_REG(hw, IXGBE_XEC);
	stats->fccrc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCCRC);
	stats->fclast.ev_count += IXGBE_READ_REG(hw, IXGBE_FCLAST);
	/* Only read FCOE on 82599 */
	if (hw->mac.type != ixgbe_mac_82598EB) {
		stats->fcoerpdc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCOERPDC);
		stats->fcoeprc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCOEPRC);
		stats->fcoeptc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCOEPTC);
		stats->fcoedwrc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCOEDWRC);
		stats->fcoedwtc.ev_count += IXGBE_READ_REG(hw, IXGBE_FCOEDWTC);
	}

	/* Fill out the OS statistics structure */
	/*
	 * NetBSD: Don't override if_{i|o}{packets|bytes|mcasts} with
	 * adapter->stats counters. It's required to make ifconfig -z
	 * (SOICZIFDATA) work.
	 */
	ifp->if_collisions = 0;

	/* Rx Errors */
	ifp->if_iqdrops += total_missed_rx;
	ifp->if_ierrors += crcerrs + rlec;
}

/** ixgbe_sysctl_tdh_handler - Handler function
 *  Retrieves the TDH value from the hardware
 */
static int 
ixgbe_sysctl_tdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	uint32_t val;
	struct tx_ring *txr;

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
	struct sysctlnode node = *rnode;
	uint32_t val;
	struct tx_ring *txr;

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
	struct sysctlnode node = *rnode;
	uint32_t val;
	struct rx_ring *rxr;

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
	struct sysctlnode node = *rnode;
	uint32_t val;
	struct rx_ring *rxr;

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
	struct sysctlnode node = *rnode;
	struct ix_queue *que;
	uint32_t reg, usec, rate;
	int error;

	que = (struct ix_queue *)node.sysctl_data;
	if (que == NULL)
		return 0;
	reg = IXGBE_READ_REG(&que->adapter->hw, IXGBE_EITR(que->msix));
	usec = ((reg & 0x0FF8) >> 3);
	if (usec > 0)
		rate = 500000 / usec;
	else
		rate = 0;
	node.sysctl_data = &rate;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error)
		return error;
	reg &= ~0xfff; /* default, no limitation */
	ixgbe_max_interrupt_rate = 0;
	if (rate > 0 && rate < 500000) {
		if (rate < 1000)
			rate = 1000;
		ixgbe_max_interrupt_rate = rate;
		reg |= ((4000000/rate) & 0xff8 );
	}
	IXGBE_WRITE_REG(&que->adapter->hw, IXGBE_EITR(que->msix), reg);
	return 0;
}

const struct sysctlnode *
ixgbe_sysctl_instance(struct adapter *adapter)
{
	const char *dvname;
	struct sysctllog **log;
	int rc;
	const struct sysctlnode *rnode;

	if (adapter->sysctltop != NULL)
		return adapter->sysctltop;

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

static void
ixgbe_add_device_sysctls(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct ixgbe_hw *hw = &adapter->hw;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;

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

	/* Sysctls for all devices */
	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "fc", SYSCTL_DESCR(IXGBE_SYSCTL_DESC_SET_FC),
	    ixgbe_sysctl_flowcntl, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	/* XXX This is an *instance* sysctl controlling a *global* variable.
	 * XXX It's that way in the FreeBSD driver that this derives from.
	 */
	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "enable_aim", SYSCTL_DESCR("Interrupt Moderation"),
	    NULL, 0, &ixgbe_enable_aim, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "advertise_speed", SYSCTL_DESCR(IXGBE_SYSCTL_DESC_ADV_SPEED),
	    ixgbe_sysctl_advertise, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "ts", SYSCTL_DESCR("Thermal Test"),
	    ixgbe_sysctl_thermal_test, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

#ifdef IXGBE_DEBUG
	/* testing sysctls (for all devices) */
	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "power_state", SYSCTL_DESCR("PCI Power State"),
	    ixgbe_sysctl_power_state, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING,
	    "print_rss_config", SYSCTL_DESCR("Prints RSS Configuration"),
	    ixgbe_sysctl_print_rss_config, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
#endif
	/* for X550 series devices */
	if (hw->mac.type >= ixgbe_mac_X550)
		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "dmac", SYSCTL_DESCR("DMA Coalesce"),
		    ixgbe_sysctl_dmac, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

	/* for X552 backplane devices */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_KR) {
		const struct sysctlnode *eee_node;

		if (sysctl_createv(log, 0, &rnode, &eee_node,
		    0, CTLTYPE_NODE,
		    "eee", SYSCTL_DESCR("Energy Efficient Ethernet sysctls"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0) {
			aprint_error_dev(dev, "could not create sysctl\n");
			return;
		}

		if (sysctl_createv(log, 0, &eee_node, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "enable", SYSCTL_DESCR("Enable or Disable EEE"),
		    ixgbe_sysctl_eee_enable, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &eee_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_BOOL,
		    "negotiated", SYSCTL_DESCR("EEE negotiated on link"),
		    ixgbe_sysctl_eee_negotiated, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &eee_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_BOOL,
		    "tx_lpi_status", SYSCTL_DESCR("Whether or not TX link is in LPI state"),
		    ixgbe_sysctl_eee_tx_lpi_status, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &eee_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_BOOL,
		    "rx_lpi_status", SYSCTL_DESCR("Whether or not RX link is in LPI state"),
		    ixgbe_sysctl_eee_rx_lpi_status, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &eee_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_BOOL,
		    "tx_lpi_delay", SYSCTL_DESCR("TX LPI entry delay in microseconds"),
		    ixgbe_sysctl_eee_tx_lpi_delay, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}

	/* for WoL-capable devices */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T) {
		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL,
		    "wol_enable", SYSCTL_DESCR("Enable/Disable Wake on LAN"),
		    ixgbe_sysctl_wol_enable, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "wufc", SYSCTL_DESCR("Enable/Disable Wake Up Filters"),
		    ixgbe_sysctl_wufc, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}

	/* for X552/X557-AT devices */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T) {
		const struct sysctlnode *phy_node;

		if (sysctl_createv(log, 0, &rnode, &phy_node,
		    0, CTLTYPE_NODE,
		    "phy", SYSCTL_DESCR("External PHY sysctls"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0) {
			aprint_error_dev(dev, "could not create sysctl\n");
			return;
		}

		if (sysctl_createv(log, 0, &phy_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "temp", SYSCTL_DESCR("Current External PHY Temperature (Celsius)"),
		    ixgbe_sysctl_phy_temp, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &phy_node, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT,
		    "overtemp_occurred", SYSCTL_DESCR("External PHY High Temperature Event Occurred"),
		    ixgbe_sysctl_phy_overtemp_occurred, 0, (void *)adapter, 0, CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}
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
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;

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
	evcnt_attach_dynamic(&adapter->link_irq, EVCNT_TYPE_MISC,
	    NULL, device_xname(dev), "Link MSIX IRQ Handled");

	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		snprintf(adapter->queues[i].evnamebuf,
		    sizeof(adapter->queues[i].evnamebuf), "%s q%d",
		    device_xname(dev), i);
		snprintf(adapter->queues[i].namebuf,
		    sizeof(adapter->queues[i].namebuf), "q%d", i);

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
		    CTLFLAG_READWRITE, CTLTYPE_INT,
		    "interrupt_rate", SYSCTL_DESCR("Interrupt Rate"),
		    ixgbe_sysctl_interrupt_rate_handler, 0,
		    (void *)&adapter->queues[i], 0, CTL_CREATE, CTL_EOL) != 0)
			break;

#if 0 /* XXX msaitoh */
		if (sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_QUAD,
		    "irqs", SYSCTL_DESCR("irqs on this queue"),
			NULL, 0, &(adapter->queues[i].irqs),
		    0, CTL_CREATE, CTL_EOL) != 0)
			break;
#endif

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

		evcnt_attach_dynamic(&txr->tso_tx, EVCNT_TYPE_MISC,
		    NULL, device_xname(dev), "TSO");
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

		if (i < __arraycount(stats->mpc)) {
			evcnt_attach_dynamic(&stats->mpc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "Missed Packet Count");
		}
		if (i < __arraycount(stats->pxontxc)) {
			evcnt_attach_dynamic(&stats->pxontxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxontxc");
			evcnt_attach_dynamic(&stats->pxonrxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxonrxc");
			evcnt_attach_dynamic(&stats->pxofftxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxofftxc");
			evcnt_attach_dynamic(&stats->pxoffrxc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxoffrxc");
			evcnt_attach_dynamic(&stats->pxon2offc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "pxon2offc");
		}
		if (i < __arraycount(stats->qprc)) {
			evcnt_attach_dynamic(&stats->qprc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qprc");
			evcnt_attach_dynamic(&stats->qptc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qptc");
			evcnt_attach_dynamic(&stats->qbrc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qbrc");
			evcnt_attach_dynamic(&stats->qbtc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qbtc");
			evcnt_attach_dynamic(&stats->qprdc[i],
			    EVCNT_TYPE_MISC, NULL, adapter->queues[i].evnamebuf,
			    "qprdc");
		}

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
}

static void
ixgbe_set_sysctl_value(struct adapter *adapter, const char *name,
    const char *description, int *limit, int value)
{
	device_t dev =  adapter->dev;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;

	log = &adapter->sysctllog;
	if ((rnode = ixgbe_sysctl_instance(adapter)) == NULL) {
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

/*
** Set flow control using sysctl:
** Flow control values:
** 	0 - off
**	1 - rx pause
**	2 - tx pause
**	3 - full
*/
static int
ixgbe_sysctl_flowcntl(SYSCTLFN_ARGS)
{
	int error, fc;
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;

	fc = adapter->fc;
	node.sysctl_data = &fc;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	/* Don't bother if it's not changed */
	if (adapter->fc == fc)
		return (0);

	return ixgbe_set_flowcntl(adapter, fc);
}


static int
ixgbe_set_flowcntl(struct adapter *adapter, int fc)
{

	switch (fc) {
		case ixgbe_fc_rx_pause:
		case ixgbe_fc_tx_pause:
		case ixgbe_fc_full:
			adapter->hw.fc.requested_mode = adapter->fc;
			if (adapter->num_queues > 1)
				ixgbe_disable_rx_drop(adapter);
			break;
		case ixgbe_fc_none:
			adapter->hw.fc.requested_mode = ixgbe_fc_none;
			if (adapter->num_queues > 1)
				ixgbe_enable_rx_drop(adapter);
			break;
		default:
			return (EINVAL);
	}
	adapter->fc = fc;
#if 0 /* XXX NetBSD */
	/* Don't autoneg if forcing a value */
	adapter->hw.fc.disable_fc_autoneg = TRUE;
#endif
	ixgbe_fc_enable(&adapter->hw);
	return (0);
}

/*
** Control advertised link speed:
**	Flags:
**	0x0 - Default (all capable link speed)
**	0x1 - advertise 100 Mb
**	0x2 - advertise 1G
**	0x4 - advertise 10G
*/
static int
ixgbe_sysctl_advertise(SYSCTLFN_ARGS)
{
	struct sysctlnode	node = *rnode;
	int			error = 0, advertise;
	struct adapter		*adapter = (struct adapter *)node.sysctl_data;

	advertise = adapter->advertise;
	node.sysctl_data = &advertise;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	return ixgbe_set_advertise(adapter, advertise);
}

static int
ixgbe_set_advertise(struct adapter *adapter, int advertise)
{
	device_t		dev;
	struct ixgbe_hw		*hw;
	ixgbe_link_speed	speed;

	/* Checks to validate new value */
	if (adapter->advertise == advertise) /* no change */
		return (0);

	hw = &adapter->hw;
	dev = adapter->dev;

	/* No speed changes for backplane media */
	if (hw->phy.media_type == ixgbe_media_type_backplane)
		return (ENODEV);

	if (!((hw->phy.media_type == ixgbe_media_type_copper) ||
	    (hw->phy.multispeed_fiber))) {
		device_printf(dev,
		    "Advertised speed can only be set on copper or "
		    "multispeed fiber media types.\n");
		return (EINVAL);
	}

	if (advertise < 0x0 || advertise > 0x7) {
		device_printf(dev,
		    "Invalid advertised speed; valid modes are 0x0 through 0x7\n");
		return (EINVAL);
	}

	/* Set new value and report new advertised mode */
	speed = 0;
	if ((hw->mac.type != ixgbe_mac_X540)
	    && (hw->mac.type != ixgbe_mac_X550)) {
		if (advertise & 0x1) {
			device_printf(dev,
			    "Set Advertise: 100Mb on X540/X550 only\n");
			return (EINVAL);
		}
	} else if ((advertise & 0x1) || (advertise == 0))
		speed |= IXGBE_LINK_SPEED_100_FULL;
	if ((advertise & 0x2) || (advertise == 0))
		speed |= IXGBE_LINK_SPEED_1GB_FULL;
	if ((advertise & 0x4) || (advertise == 0))
		speed |= IXGBE_LINK_SPEED_10GB_FULL;
	adapter->advertise = advertise;

	hw->mac.autotry_restart = TRUE;
	hw->mac.ops.setup_link(hw, speed, TRUE);

	return 0;
}

/*
 * The following two sysctls are for X552/X557-AT devices;
 * they deal with the external PHY used in them.
 */
static int
ixgbe_sysctl_phy_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter	*adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	int val;
	u16 reg;
	int		error;

	if (hw->device_id != IXGBE_DEV_ID_X550EM_X_10G_T) {
		device_printf(adapter->dev,
		    "Device has no supported external thermal sensor.\n");
		return (ENODEV);
	}

	if (hw->phy.ops.read_reg(hw, IXGBE_PHY_CURRENT_TEMP,
				      IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
				      &reg)) {
		device_printf(adapter->dev,
		    "Error reading from PHY's current temperature register\n");
		return (EAGAIN);
	}

	node.sysctl_data = &val;

	/* Shift temp for output */
	val = reg >> 8;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);

	return (0);
}

/*
 * Reports whether the current PHY temperature is over
 * the overtemp threshold.
 *  - This is reported directly from the PHY
 */
static int
ixgbe_sysctl_phy_overtemp_occurred(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter	*adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	int val, error;
	u16 reg;

	if (hw->device_id != IXGBE_DEV_ID_X550EM_X_10G_T) {
		device_printf(adapter->dev,
		    "Device has no supported external thermal sensor.\n");
		return (ENODEV);
	}

	if (hw->phy.ops.read_reg(hw, IXGBE_PHY_OVERTEMP_STATUS,
				      IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
				      &reg)) {
		device_printf(adapter->dev,
		    "Error reading from PHY's temperature status register\n");
		return (EAGAIN);
	}

	node.sysctl_data = &val;

	/* Get occurrence bit */
	val = !!(reg & 0x4000);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);

	return (0);
}

/*
** Thermal Shutdown Trigger (internal MAC)
**   - Set this to 1 to cause an overtemp event to occur
*/
static int
ixgbe_sysctl_thermal_test(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter	*adapter = (struct adapter *)node.sysctl_data;
	int		error, fire = 0;
	struct ixgbe_hw *hw;

	hw = &adapter->hw;

	node.sysctl_data = &fire;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);

	if (fire) {
		u32 reg = IXGBE_READ_REG(hw, IXGBE_EICS);
		reg |= IXGBE_EICR_TS;
		IXGBE_WRITE_REG(hw, IXGBE_EICS, reg);
	}

	return (0);
}

/*
** Manage DMA Coalescing.
** Control values:
** 	0/1 - off / on (use default value of 1000)
**
**	Legal timer values are:
**	50,100,250,500,1000,2000,5000,10000
**
**	Turning off interrupt moderation will also turn this off.
*/
static int
ixgbe_sysctl_dmac(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ifnet *ifp = adapter->ifp;
	int		error;
	u16		oldval;
	int		newval;

	oldval = adapter->dmac;
	newval = oldval;
	node.sysctl_data = &newval;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);

	switch (newval) {
	case 0:
		/* Disabled */
		adapter->dmac = 0;
		break;
	case 1:
		/* Enable and use default */
		adapter->dmac = 1000;
		break;
	case 50:
	case 100:
	case 250:
	case 500:
	case 1000:
	case 2000:
	case 5000:
	case 10000:
		/* Legal values - allow */
		adapter->dmac = newval;
		break;
	default:
		/* Do nothing, illegal value */
		return (EINVAL);
	}

	/* Re-initialize hardware if it's already running */
	if (ifp->if_flags & IFF_RUNNING)
		ixgbe_init(ifp);

	return (0);
}

#ifdef IXGBE_DEBUG
/**
 * Sysctl to test power states
 * Values:
 *	0      - set device to D0
 *	3      - set device to D3
 *	(none) - get current device power state
 */
static int
ixgbe_sysctl_power_state(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	device_t dev =  adapter->dev;
	int curr_ps, new_ps, error = 0;

#if notyet
	curr_ps = new_ps = pci_get_powerstate(dev);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (req->newp == NULL))
		return (error);

	if (new_ps == curr_ps)
		return (0);

	if (new_ps == 3 && curr_ps == 0)
		error = DEVICE_SUSPEND(dev);
	else if (new_ps == 0 && curr_ps == 3)
		error = DEVICE_RESUME(dev);
	else
		return (EINVAL);

	device_printf(dev, "New state: %d\n", pci_get_powerstate(dev));

	return (error);
#else
	return 0;
#endif
}
#endif
/*
 * Sysctl to enable/disable the WoL capability, if supported by the adapter.
 * Values:
 *	0 - disabled
 *	1 - enabled
 */
static int
ixgbe_sysctl_wol_enable(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	bool new_wol_enabled;
	int error = 0;

	new_wol_enabled = hw->wol_enabled;
	node.sysctl_data = &new_wol_enabled;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);
	if (new_wol_enabled == hw->wol_enabled)
		return (0);

	if (new_wol_enabled && !adapter->wol_support)
		return (ENODEV);
	else
		hw->wol_enabled = new_wol_enabled;

	return (0);
}

/*
 * Sysctl to enable/disable the Energy Efficient Ethernet capability,
 * if supported by the adapter.
 * Values:
 *	0 - disabled
 *	1 - enabled
 */
static int
ixgbe_sysctl_eee_enable(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ifnet *ifp = adapter->ifp;
	int new_eee_enabled, error = 0;

	new_eee_enabled = adapter->eee_enabled;
	node.sysctl_data = &new_eee_enabled;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);
	new_eee_enabled = !!(new_eee_enabled);
	if (new_eee_enabled == adapter->eee_enabled)
		return (0);

	if (new_eee_enabled > 0 && !hw->mac.ops.setup_eee)
		return (ENODEV);
	else
		adapter->eee_enabled = new_eee_enabled;

	/* Re-initialize hardware if it's already running */
	if (ifp->if_flags & IFF_RUNNING)
		ixgbe_init(ifp);

	return (0);
}

/*
 * Read-only sysctl indicating whether EEE support was negotiated
 * on the link.
 */
static int
ixgbe_sysctl_eee_negotiated(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	bool status;

	status = !!(IXGBE_READ_REG(hw, IXGBE_EEE_STAT) & IXGBE_EEE_STAT_NEG);

	node.sysctl_data = &status;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Read-only sysctl indicating whether RX Link is in LPI state.
 */
static int
ixgbe_sysctl_eee_rx_lpi_status(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	bool status;

	status = !!(IXGBE_READ_REG(hw, IXGBE_EEE_STAT) &
	    IXGBE_EEE_RX_LPI_STATUS);

	node.sysctl_data = &status;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Read-only sysctl indicating whether TX Link is in LPI state.
 */
static int
ixgbe_sysctl_eee_tx_lpi_status(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	bool status;

	status = !!(IXGBE_READ_REG(hw, IXGBE_EEE_STAT) &
	    IXGBE_EEE_TX_LPI_STATUS);

	node.sysctl_data = &status;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Read-only sysctl indicating TX Link LPI delay
 */
static int
ixgbe_sysctl_eee_tx_lpi_delay(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 reg;

	reg = IXGBE_READ_REG(hw, IXGBE_EEE_SU);

	reg >>= 26;
	node.sysctl_data = &reg;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Sysctl to enable/disable the types of packets that the
 * adapter will wake up on upon receipt.
 * WUFC - Wake Up Filter Control
 * Flags:
 *	0x1  - Link Status Change
 *	0x2  - Magic Packet
 *	0x4  - Direct Exact
 *	0x8  - Directed Multicast
 *	0x10 - Broadcast
 *	0x20 - ARP/IPv4 Request Packet
 *	0x40 - Direct IPv4 Packet
 *	0x80 - Direct IPv6 Packet
 *
 * Setting another flag will cause the sysctl to return an
 * error.
 */
static int
ixgbe_sysctl_wufc(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	int error = 0;
	u32 new_wufc;

	new_wufc = adapter->wufc;
	node.sysctl_data = &new_wufc;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);
	if (new_wufc == adapter->wufc)
		return (0);

	if (new_wufc & 0xffffff00)
		return (EINVAL);
	else {
		new_wufc &= 0xff;
		new_wufc |= (0xffffff & adapter->wufc);
		adapter->wufc = new_wufc;
	}

	return (0);
}

#ifdef IXGBE_DEBUG
static int
ixgbe_sysctl_print_rss_config(SYSCTLFN_ARGS)
{
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	device_t dev = adapter->dev;
	int error = 0, reta_size;
	struct sbuf *buf;
	u32 reg;

	buf = sbuf_new_for_sysctl(NULL, NULL, 128, req);
	if (!buf) {
		device_printf(dev, "Could not allocate sbuf for output.\n");
		return (ENOMEM);
	}

	// TODO: use sbufs to make a string to print out
	/* Set multiplier for RETA setup and table size based on MAC */
	switch (adapter->hw.mac.type) {
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
		reta_size = 128;
		break;
	default:
		reta_size = 32;
		break;
	}

	/* Print out the redirection table */
	sbuf_cat(buf, "\n");
	for (int i = 0; i < reta_size; i++) {
		if (i < 32) {
			reg = IXGBE_READ_REG(hw, IXGBE_RETA(i));
			sbuf_printf(buf, "RETA(%2d): 0x%08x\n", i, reg);
		} else {
			reg = IXGBE_READ_REG(hw, IXGBE_ERETA(i - 32));
			sbuf_printf(buf, "ERETA(%2d): 0x%08x\n", i - 32, reg);
		}
	}

	// TODO: print more config

	error = sbuf_finish(buf);
	if (error)
		device_printf(dev, "Error finishing sbuf: %d\n", error);

	sbuf_delete(buf);
	return (0);
}
#endif /* IXGBE_DEBUG */

/*
** Enable the hardware to drop packets when the buffer is
** full. This is useful when multiqueue,so that no single
** queue being full stalls the entire RX engine. We only
** enable this when Multiqueue AND when Flow Control is 
** disabled.
*/
static void
ixgbe_enable_rx_drop(struct adapter *adapter)
{
        struct ixgbe_hw *hw = &adapter->hw;

	for (int i = 0; i < adapter->num_queues; i++) {
		struct rx_ring *rxr = &adapter->rx_rings[i];
        	u32 srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(rxr->me));
        	srrctl |= IXGBE_SRRCTL_DROP_EN;
        	IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(rxr->me), srrctl);
	}
#ifdef PCI_IOV
	/* enable drop for each vf */
	for (int i = 0; i < adapter->num_vfs; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_QDE,
		    (IXGBE_QDE_WRITE | (i << IXGBE_QDE_IDX_SHIFT) |
		    IXGBE_QDE_ENABLE));
	}
#endif
}

static void
ixgbe_disable_rx_drop(struct adapter *adapter)
{
        struct ixgbe_hw *hw = &adapter->hw;

	for (int i = 0; i < adapter->num_queues; i++) {
		struct rx_ring *rxr = &adapter->rx_rings[i];
        	u32 srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(rxr->me));
        	srrctl &= ~IXGBE_SRRCTL_DROP_EN;
        	IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(rxr->me), srrctl);
	}
#ifdef PCI_IOV
	/* disable drop for each vf */
	for (int i = 0; i < adapter->num_vfs; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_QDE,
		    (IXGBE_QDE_WRITE | (i << IXGBE_QDE_IDX_SHIFT)));
	}
#endif
}

static void
ixgbe_rearm_queues(struct adapter *adapter, u64 queues)
{
	u32 mask;

	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82598EB:
		mask = (IXGBE_EIMS_RTX_QUEUE & queues);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS, mask);
		break;
	case ixgbe_mac_82599EB:
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
		mask = (queues & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(0), mask);
		mask = (queues >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(1), mask);
		break;
	default:
		break;
	}
}

#ifdef PCI_IOV

/*
** Support functions for SRIOV/VF management
*/

static void
ixgbe_ping_all_vfs(struct adapter *adapter)
{
	struct ixgbe_vf *vf;

	for (int i = 0; i < adapter->num_vfs; i++) {
		vf = &adapter->vfs[i];
		if (vf->flags & IXGBE_VF_ACTIVE)
			ixgbe_send_vf_msg(adapter, vf, IXGBE_PF_CONTROL_MSG);
	}
}


static void
ixgbe_vf_set_default_vlan(struct adapter *adapter, struct ixgbe_vf *vf,
    uint16_t tag)
{
	struct ixgbe_hw *hw;
	uint32_t vmolr, vmvir;

	hw = &adapter->hw;

	vf->vlan_tag = tag;
	
	vmolr = IXGBE_READ_REG(hw, IXGBE_VMOLR(vf->pool));

	/* Do not receive packets that pass inexact filters. */
	vmolr &= ~(IXGBE_VMOLR_ROMPE | IXGBE_VMOLR_ROPE);

	/* Disable Multicast Promicuous Mode. */
	vmolr &= ~IXGBE_VMOLR_MPE;

	/* Accept broadcasts. */
	vmolr |= IXGBE_VMOLR_BAM;

	if (tag == 0) {
		/* Accept non-vlan tagged traffic. */
		//vmolr |= IXGBE_VMOLR_AUPE;

		/* Allow VM to tag outgoing traffic; no default tag. */
		vmvir = 0;
	} else {
		/* Require vlan-tagged traffic. */
		vmolr &= ~IXGBE_VMOLR_AUPE;

		/* Tag all traffic with provided vlan tag. */
		vmvir = (tag | IXGBE_VMVIR_VLANA_DEFAULT);
	}
	IXGBE_WRITE_REG(hw, IXGBE_VMOLR(vf->pool), vmolr);
	IXGBE_WRITE_REG(hw, IXGBE_VMVIR(vf->pool), vmvir);
}


static boolean_t
ixgbe_vf_frame_size_compatible(struct adapter *adapter, struct ixgbe_vf *vf)
{

	/*
	 * Frame size compatibility between PF and VF is only a problem on
	 * 82599-based cards.  X540 and later support any combination of jumbo
	 * frames on PFs and VFs.
	 */
	if (adapter->hw.mac.type != ixgbe_mac_82599EB)
		return (TRUE);

	switch (vf->api_ver) {
	case IXGBE_API_VER_1_0:
	case IXGBE_API_VER_UNKNOWN:
		/*
		 * On legacy (1.0 and older) VF versions, we don't support jumbo
		 * frames on either the PF or the VF.
		 */
		if (adapter->max_frame_size > ETHER_MAX_LEN ||
		    vf->max_frame_size > ETHER_MAX_LEN)
		    return (FALSE);

		return (TRUE);

		break;
	case IXGBE_API_VER_1_1:
	default:
		/*
		 * 1.1 or later VF versions always work if they aren't using
		 * jumbo frames.
		 */
		if (vf->max_frame_size <= ETHER_MAX_LEN)
			return (TRUE);

		/*
		 * Jumbo frames only work with VFs if the PF is also using jumbo
		 * frames.
		 */
		if (adapter->max_frame_size <= ETHER_MAX_LEN)
			return (TRUE);

		return (FALSE);
	
	}
}


static void
ixgbe_process_vf_reset(struct adapter *adapter, struct ixgbe_vf *vf)
{
	ixgbe_vf_set_default_vlan(adapter, vf, vf->default_vlan);

	// XXX clear multicast addresses

	ixgbe_clear_rar(&adapter->hw, vf->rar_index);

	vf->api_ver = IXGBE_API_VER_UNKNOWN;
}


static void
ixgbe_vf_enable_transmit(struct adapter *adapter, struct ixgbe_vf *vf)
{
	struct ixgbe_hw *hw;
	uint32_t vf_index, vfte;

	hw = &adapter->hw;

	vf_index = IXGBE_VF_INDEX(vf->pool);
	vfte = IXGBE_READ_REG(hw, IXGBE_VFTE(vf_index));
	vfte |= IXGBE_VF_BIT(vf->pool);
	IXGBE_WRITE_REG(hw, IXGBE_VFTE(vf_index), vfte);
}


static void
ixgbe_vf_enable_receive(struct adapter *adapter, struct ixgbe_vf *vf)
{
	struct ixgbe_hw *hw;
	uint32_t vf_index, vfre;

	hw = &adapter->hw;
	
	vf_index = IXGBE_VF_INDEX(vf->pool);
	vfre = IXGBE_READ_REG(hw, IXGBE_VFRE(vf_index));
	if (ixgbe_vf_frame_size_compatible(adapter, vf))
		vfre |= IXGBE_VF_BIT(vf->pool);
	else
		vfre &= ~IXGBE_VF_BIT(vf->pool);
	IXGBE_WRITE_REG(hw, IXGBE_VFRE(vf_index), vfre);
}


static void
ixgbe_vf_reset_msg(struct adapter *adapter, struct ixgbe_vf *vf, uint32_t *msg)
{
	struct ixgbe_hw *hw;
	uint32_t ack;
	uint32_t resp[IXGBE_VF_PERMADDR_MSG_LEN];

	hw = &adapter->hw;

	ixgbe_process_vf_reset(adapter, vf);

	if (ixgbe_validate_mac_addr(vf->ether_addr) == 0) {
		ixgbe_set_rar(&adapter->hw, vf->rar_index,
		    vf->ether_addr, vf->pool, TRUE);
		ack = IXGBE_VT_MSGTYPE_ACK;
	} else
		ack = IXGBE_VT_MSGTYPE_NACK;

	ixgbe_vf_enable_transmit(adapter, vf);
	ixgbe_vf_enable_receive(adapter, vf);

	vf->flags |= IXGBE_VF_CTS;

	resp[0] = IXGBE_VF_RESET | ack | IXGBE_VT_MSGTYPE_CTS;
	bcopy(vf->ether_addr, &resp[1], ETHER_ADDR_LEN);
	resp[3] = hw->mac.mc_filter_type;
	ixgbe_write_mbx(hw, resp, IXGBE_VF_PERMADDR_MSG_LEN, vf->pool);
}


static void
ixgbe_vf_set_mac(struct adapter *adapter, struct ixgbe_vf *vf, uint32_t *msg)
{
	uint8_t *mac;

	mac = (uint8_t*)&msg[1];

	/* Check that the VF has permission to change the MAC address. */
	if (!(vf->flags & IXGBE_VF_CAP_MAC) && ixgbe_vf_mac_changed(vf, mac)) {
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}

	if (ixgbe_validate_mac_addr(mac) != 0) {
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}

	bcopy(mac, vf->ether_addr, ETHER_ADDR_LEN);

	ixgbe_set_rar(&adapter->hw, vf->rar_index, vf->ether_addr, 
	    vf->pool, TRUE);

	ixgbe_send_vf_ack(adapter, vf, msg[0]);
}


/*
** VF multicast addresses are set by using the appropriate bit in
** 1 of 128 32 bit addresses (4096 possible).
*/
static void
ixgbe_vf_set_mc_addr(struct adapter *adapter, struct ixgbe_vf *vf, u32 *msg)
{
	u16	*list = (u16*)&msg[1];
	int	entries;
	u32	vmolr, vec_bit, vec_reg, mta_reg;

	entries = (msg[0] & IXGBE_VT_MSGINFO_MASK) >> IXGBE_VT_MSGINFO_SHIFT;
	entries = min(entries, IXGBE_MAX_VF_MC);

	vmolr = IXGBE_READ_REG(&adapter->hw, IXGBE_VMOLR(vf->pool));

	vf->num_mc_hashes = entries;

	/* Set the appropriate MTA bit */
	for (int i = 0; i < entries; i++) {
		vf->mc_hash[i] = list[i];
		vec_reg = (vf->mc_hash[i] >> 5) & 0x7F;
                vec_bit = vf->mc_hash[i] & 0x1F;
                mta_reg = IXGBE_READ_REG(&adapter->hw, IXGBE_MTA(vec_reg));
                mta_reg |= (1 << vec_bit);
                IXGBE_WRITE_REG(&adapter->hw, IXGBE_MTA(vec_reg), mta_reg);
        }

	vmolr |= IXGBE_VMOLR_ROMPE;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_VMOLR(vf->pool), vmolr);
	ixgbe_send_vf_ack(adapter, vf, msg[0]);
	return;
}


static void
ixgbe_vf_set_vlan(struct adapter *adapter, struct ixgbe_vf *vf, uint32_t *msg)
{
	struct ixgbe_hw *hw;
	int enable;
	uint16_t tag;

	hw = &adapter->hw;
	enable = IXGBE_VT_MSGINFO(msg[0]);
	tag = msg[1] & IXGBE_VLVF_VLANID_MASK;

	if (!(vf->flags & IXGBE_VF_CAP_VLAN)) {
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}

	/* It is illegal to enable vlan tag 0. */
	if (tag == 0 && enable != 0){
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}
	
	ixgbe_set_vfta(hw, tag, vf->pool, enable);
	ixgbe_send_vf_ack(adapter, vf, msg[0]);
}


static void
ixgbe_vf_set_lpe(struct adapter *adapter, struct ixgbe_vf *vf, uint32_t *msg)
{
	struct ixgbe_hw *hw;
	uint32_t vf_max_size, pf_max_size, mhadd;

	hw = &adapter->hw;
	vf_max_size = msg[1];

	if (vf_max_size < ETHER_CRC_LEN) {
		/* We intentionally ACK invalid LPE requests. */
		ixgbe_send_vf_ack(adapter, vf, msg[0]);
		return;
	}

	vf_max_size -= ETHER_CRC_LEN;

	if (vf_max_size > IXGBE_MAX_FRAME_SIZE) {
		/* We intentionally ACK invalid LPE requests. */
		ixgbe_send_vf_ack(adapter, vf, msg[0]);
		return;
	}

	vf->max_frame_size = vf_max_size;
	ixgbe_update_max_frame(adapter, vf->max_frame_size);

	/*
	 * We might have to disable reception to this VF if the frame size is
	 * not compatible with the config on the PF.
	 */
	ixgbe_vf_enable_receive(adapter, vf);

	mhadd = IXGBE_READ_REG(hw, IXGBE_MHADD);
	pf_max_size = (mhadd & IXGBE_MHADD_MFS_MASK) >> IXGBE_MHADD_MFS_SHIFT;

	if (pf_max_size < adapter->max_frame_size) {
		mhadd &= ~IXGBE_MHADD_MFS_MASK;
		mhadd |= adapter->max_frame_size << IXGBE_MHADD_MFS_SHIFT;
		IXGBE_WRITE_REG(hw, IXGBE_MHADD, mhadd);
	}

	ixgbe_send_vf_ack(adapter, vf, msg[0]);
}


static void
ixgbe_vf_set_macvlan(struct adapter *adapter, struct ixgbe_vf *vf,
    uint32_t *msg)
{
	//XXX implement this
	ixgbe_send_vf_nack(adapter, vf, msg[0]);
}


static void
ixgbe_vf_api_negotiate(struct adapter *adapter, struct ixgbe_vf *vf,
    uint32_t *msg)
{

	switch (msg[1]) {
	case IXGBE_API_VER_1_0:
	case IXGBE_API_VER_1_1:
		vf->api_ver = msg[1];
		ixgbe_send_vf_ack(adapter, vf, msg[0]);
		break;
	default:
		vf->api_ver = IXGBE_API_VER_UNKNOWN;
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		break;
	}
}


static void
ixgbe_vf_get_queues(struct adapter *adapter, struct ixgbe_vf *vf,
    uint32_t *msg)
{
	struct ixgbe_hw *hw;
	uint32_t resp[IXGBE_VF_GET_QUEUES_RESP_LEN];
	int num_queues;

	hw = &adapter->hw;

	/* GET_QUEUES is not supported on pre-1.1 APIs. */
	switch (msg[0]) {
	case IXGBE_API_VER_1_0:
	case IXGBE_API_VER_UNKNOWN:
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}

	resp[0] = IXGBE_VF_GET_QUEUES | IXGBE_VT_MSGTYPE_ACK | 
	    IXGBE_VT_MSGTYPE_CTS;

	num_queues = ixgbe_vf_queues(ixgbe_get_iov_mode(adapter));
	resp[IXGBE_VF_TX_QUEUES] = num_queues;
	resp[IXGBE_VF_RX_QUEUES] = num_queues;
	resp[IXGBE_VF_TRANS_VLAN] = (vf->default_vlan != 0);
	resp[IXGBE_VF_DEF_QUEUE] = 0;

	ixgbe_write_mbx(hw, resp, IXGBE_VF_GET_QUEUES_RESP_LEN, vf->pool);
}


static void
ixgbe_process_vf_msg(struct adapter *adapter, struct ixgbe_vf *vf)
{
	struct ixgbe_hw *hw;
	uint32_t msg[IXGBE_VFMAILBOX_SIZE];
	int error;

	hw = &adapter->hw;

	error = ixgbe_read_mbx(hw, msg, IXGBE_VFMAILBOX_SIZE, vf->pool);

	if (error != 0)
		return;

	CTR3(KTR_MALLOC, "%s: received msg %x from %d",
	    adapter->ifp->if_xname, msg[0], vf->pool);
	if (msg[0] == IXGBE_VF_RESET) {
		ixgbe_vf_reset_msg(adapter, vf, msg);
		return;
	}

	if (!(vf->flags & IXGBE_VF_CTS)) {
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
		return;
	}

	switch (msg[0] & IXGBE_VT_MSG_MASK) {
	case IXGBE_VF_SET_MAC_ADDR:
		ixgbe_vf_set_mac(adapter, vf, msg);
		break;
	case IXGBE_VF_SET_MULTICAST:
		ixgbe_vf_set_mc_addr(adapter, vf, msg);
		break;
	case IXGBE_VF_SET_VLAN:
		ixgbe_vf_set_vlan(adapter, vf, msg);
		break;
	case IXGBE_VF_SET_LPE:
		ixgbe_vf_set_lpe(adapter, vf, msg);
		break;
	case IXGBE_VF_SET_MACVLAN:
		ixgbe_vf_set_macvlan(adapter, vf, msg);
		break;
	case IXGBE_VF_API_NEGOTIATE:
		ixgbe_vf_api_negotiate(adapter, vf, msg);
		break;
	case IXGBE_VF_GET_QUEUES:
		ixgbe_vf_get_queues(adapter, vf, msg);
		break;
	default:
		ixgbe_send_vf_nack(adapter, vf, msg[0]);
	}
}


/*
 * Tasklet for handling VF -> PF mailbox messages.
 */
static void
ixgbe_handle_mbx(void *context, int pending)
{
	struct adapter *adapter;
	struct ixgbe_hw *hw;
	struct ixgbe_vf *vf;
	int i;

	adapter = context;
	hw = &adapter->hw;

	IXGBE_CORE_LOCK(adapter);
	for (i = 0; i < adapter->num_vfs; i++) {
		vf = &adapter->vfs[i];

		if (vf->flags & IXGBE_VF_ACTIVE) {
			if (ixgbe_check_for_rst(hw, vf->pool) == 0)
				ixgbe_process_vf_reset(adapter, vf);

			if (ixgbe_check_for_msg(hw, vf->pool) == 0)
				ixgbe_process_vf_msg(adapter, vf);

			if (ixgbe_check_for_ack(hw, vf->pool) == 0)
				ixgbe_process_vf_ack(adapter, vf);
		}
	}
	IXGBE_CORE_UNLOCK(adapter);
}


static int
ixgbe_init_iov(device_t dev, u16 num_vfs, const nvlist_t *config)
{
	struct adapter *adapter;
	enum ixgbe_iov_mode mode;

	adapter = device_get_softc(dev);
	adapter->num_vfs = num_vfs;
	mode = ixgbe_get_iov_mode(adapter);

	if (num_vfs > ixgbe_max_vfs(mode)) {
		adapter->num_vfs = 0;
		return (ENOSPC);
	}

	IXGBE_CORE_LOCK(adapter);

	adapter->vfs = malloc(sizeof(*adapter->vfs) * num_vfs, M_IXGBE, 
	    M_NOWAIT | M_ZERO);

	if (adapter->vfs == NULL) {
		adapter->num_vfs = 0;
		IXGBE_CORE_UNLOCK(adapter);
		return (ENOMEM);
	}

	ixgbe_init_locked(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return (0);
}


static void
ixgbe_uninit_iov(device_t dev)
{
	struct ixgbe_hw *hw;
	struct adapter *adapter;
	uint32_t pf_reg, vf_reg;

	adapter = device_get_softc(dev);
	hw = &adapter->hw;

	IXGBE_CORE_LOCK(adapter);

	/* Enable rx/tx for the PF and disable it for all VFs. */
	pf_reg = IXGBE_VF_INDEX(adapter->pool);
	IXGBE_WRITE_REG(hw, IXGBE_VFRE(pf_reg),
	    IXGBE_VF_BIT(adapter->pool));
	IXGBE_WRITE_REG(hw, IXGBE_VFTE(pf_reg),
	    IXGBE_VF_BIT(adapter->pool));

	if (pf_reg == 0)
		vf_reg = 1;
	else
		vf_reg = 0;
	IXGBE_WRITE_REG(hw, IXGBE_VFRE(vf_reg), 0);
	IXGBE_WRITE_REG(hw, IXGBE_VFTE(vf_reg), 0);

	IXGBE_WRITE_REG(hw, IXGBE_VT_CTL, 0);

	free(adapter->vfs, M_IXGBE);
	adapter->vfs = NULL;
	adapter->num_vfs = 0;

	IXGBE_CORE_UNLOCK(adapter);
}


static void
ixgbe_initialize_iov(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	uint32_t mrqc, mtqc, vt_ctl, vf_reg, gcr_ext, gpie;
	enum ixgbe_iov_mode mode;
	int i;

	mode = ixgbe_get_iov_mode(adapter);
	if (mode == IXGBE_NO_VM)
		return;

	IXGBE_CORE_LOCK_ASSERT(adapter);

	mrqc = IXGBE_READ_REG(hw, IXGBE_MRQC);
	mrqc &= ~IXGBE_MRQC_MRQE_MASK;

	switch (mode) {
	case IXGBE_64_VM:
		mrqc |= IXGBE_MRQC_VMDQRSS64EN;
		break;
	case IXGBE_32_VM:
		mrqc |= IXGBE_MRQC_VMDQRSS32EN;
		break;
	default:
		panic("Unexpected SR-IOV mode %d", mode);
	}
	IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);

	mtqc = IXGBE_MTQC_VT_ENA;
	switch (mode) {
	case IXGBE_64_VM:
		mtqc |= IXGBE_MTQC_64VF;
		break;
	case IXGBE_32_VM:
		mtqc |= IXGBE_MTQC_32VF;
		break;
	default:
		panic("Unexpected SR-IOV mode %d", mode);
	}
	IXGBE_WRITE_REG(hw, IXGBE_MTQC, mtqc);
	

	gcr_ext = IXGBE_READ_REG(hw, IXGBE_GCR_EXT);
	gcr_ext |= IXGBE_GCR_EXT_MSIX_EN;
	gcr_ext &= ~IXGBE_GCR_EXT_VT_MODE_MASK;
	switch (mode) {
	case IXGBE_64_VM:
		gcr_ext |= IXGBE_GCR_EXT_VT_MODE_64;
		break;
	case IXGBE_32_VM:
		gcr_ext |= IXGBE_GCR_EXT_VT_MODE_32;
		break;
	default:
		panic("Unexpected SR-IOV mode %d", mode);
	}
	IXGBE_WRITE_REG(hw, IXGBE_GCR_EXT, gcr_ext);
	

	gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);
	gcr_ext &= ~IXGBE_GPIE_VTMODE_MASK;
	switch (mode) {
	case IXGBE_64_VM:
		gpie |= IXGBE_GPIE_VTMODE_64;
		break;
	case IXGBE_32_VM:
		gpie |= IXGBE_GPIE_VTMODE_32;
		break;
	default:
		panic("Unexpected SR-IOV mode %d", mode);
	}
	IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);

	/* Enable rx/tx for the PF. */
	vf_reg = IXGBE_VF_INDEX(adapter->pool);
	IXGBE_WRITE_REG(hw, IXGBE_VFRE(vf_reg), 
	    IXGBE_VF_BIT(adapter->pool));
	IXGBE_WRITE_REG(hw, IXGBE_VFTE(vf_reg), 
	    IXGBE_VF_BIT(adapter->pool));

	/* Allow VM-to-VM communication. */
	IXGBE_WRITE_REG(hw, IXGBE_PFDTXGSWC, IXGBE_PFDTXGSWC_VT_LBEN);

	vt_ctl = IXGBE_VT_CTL_VT_ENABLE | IXGBE_VT_CTL_REPLEN;
	vt_ctl |= (adapter->pool << IXGBE_VT_CTL_POOL_SHIFT);
	IXGBE_WRITE_REG(hw, IXGBE_VT_CTL, vt_ctl);

	for (i = 0; i < adapter->num_vfs; i++)
		ixgbe_init_vf(adapter, &adapter->vfs[i]);
}


/*
** Check the max frame setting of all active VF's
*/
static void
ixgbe_recalculate_max_frame(struct adapter *adapter)
{
	struct ixgbe_vf *vf;

	IXGBE_CORE_LOCK_ASSERT(adapter);

	for (int i = 0; i < adapter->num_vfs; i++) {
		vf = &adapter->vfs[i];
		if (vf->flags & IXGBE_VF_ACTIVE)
			ixgbe_update_max_frame(adapter, vf->max_frame_size);
	}
}


static void
ixgbe_init_vf(struct adapter *adapter, struct ixgbe_vf *vf)
{
	struct ixgbe_hw *hw;
	uint32_t vf_index, pfmbimr;

	IXGBE_CORE_LOCK_ASSERT(adapter);

	hw = &adapter->hw;

	if (!(vf->flags & IXGBE_VF_ACTIVE))
		return;

	vf_index = IXGBE_VF_INDEX(vf->pool);
	pfmbimr = IXGBE_READ_REG(hw, IXGBE_PFMBIMR(vf_index));
	pfmbimr |= IXGBE_VF_BIT(vf->pool);
	IXGBE_WRITE_REG(hw, IXGBE_PFMBIMR(vf_index), pfmbimr);

	ixgbe_vf_set_default_vlan(adapter, vf, vf->vlan_tag);

	// XXX multicast addresses

	if (ixgbe_validate_mac_addr(vf->ether_addr) == 0) {
		ixgbe_set_rar(&adapter->hw, vf->rar_index,
		    vf->ether_addr, vf->pool, TRUE);
	}

	ixgbe_vf_enable_transmit(adapter, vf);
	ixgbe_vf_enable_receive(adapter, vf);
	
	ixgbe_send_vf_msg(adapter, vf, IXGBE_PF_CONTROL_MSG);
}

static int
ixgbe_add_vf(device_t dev, u16 vfnum, const nvlist_t *config)
{
	struct adapter *adapter;
	struct ixgbe_vf *vf;
	const void *mac;

	adapter = device_get_softc(dev);

	KASSERT(vfnum < adapter->num_vfs, ("VF index %d is out of range %d",
	    vfnum, adapter->num_vfs));

	IXGBE_CORE_LOCK(adapter);
	vf = &adapter->vfs[vfnum];
	vf->pool= vfnum;

	/* RAR[0] is used by the PF so use vfnum + 1 for VF RAR. */
	vf->rar_index = vfnum + 1;
	vf->default_vlan = 0;
	vf->max_frame_size = ETHER_MAX_LEN;
	ixgbe_update_max_frame(adapter, vf->max_frame_size);

	if (nvlist_exists_binary(config, "mac-addr")) {
		mac = nvlist_get_binary(config, "mac-addr", NULL);
		bcopy(mac, vf->ether_addr, ETHER_ADDR_LEN);
		if (nvlist_get_bool(config, "allow-set-mac"))
			vf->flags |= IXGBE_VF_CAP_MAC;
	} else
		/*
		 * If the administrator has not specified a MAC address then
		 * we must allow the VF to choose one.
		 */
		vf->flags |= IXGBE_VF_CAP_MAC;

	vf->flags = IXGBE_VF_ACTIVE;

	ixgbe_init_vf(adapter, vf);
	IXGBE_CORE_UNLOCK(adapter);

	return (0);
}
#endif /* PCI_IOV */
