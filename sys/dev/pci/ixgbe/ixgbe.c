/* $NetBSD: ixgbe.c,v 1.129 2018/03/07 03:29:10 msaitoh Exp $ */

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
/*$FreeBSD: head/sys/dev/ixgbe/if_ix.c 320916 2017-07-12 17:35:32Z sbruno $*/

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

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_net_mpsafe.h"
#endif

#include "ixgbe.h"
#include "vlan.h"

#include <sys/cprng.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

/************************************************************************
 * Driver version
 ************************************************************************/
char ixgbe_driver_version[] = "3.2.12-k";


/************************************************************************
 * PCI Device ID Table
 *
 *   Used by probe to select devices to load on
 *   Last field stores an index into ixgbe_strings
 *   Last entry must be all 0s
 *
 *   { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 ************************************************************************/
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
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_1G_T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_X_SFP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_KR, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_KR_L, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_SFP, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_SFP_N, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_SGMII, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_SGMII_L, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_10G_T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_1G_T, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X550EM_A_1G_T_L, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_X540_BYPASS, 0, 0, 0},
	{IXGBE_INTEL_VENDOR_ID, IXGBE_DEV_ID_82599_BYPASS, 0, 0, 0},
	/* required last entry */
	{0, 0, 0, 0, 0}
};

/************************************************************************
 * Table of branding strings
 ************************************************************************/
static const char    *ixgbe_strings[] = {
	"Intel(R) PRO/10GbE PCI-Express Network Driver"
};

/************************************************************************
 * Function prototypes
 ************************************************************************/
static int      ixgbe_probe(device_t, cfdata_t, void *);
static void     ixgbe_attach(device_t, device_t, void *);
static int      ixgbe_detach(device_t, int);
#if 0
static int      ixgbe_shutdown(device_t);
#endif
static bool	ixgbe_suspend(device_t, const pmf_qual_t *);
static bool	ixgbe_resume(device_t, const pmf_qual_t *);
static int	ixgbe_ifflags_cb(struct ethercom *);
static int      ixgbe_ioctl(struct ifnet *, u_long, void *);
static void	ixgbe_ifstop(struct ifnet *, int);
static int	ixgbe_init(struct ifnet *);
static void	ixgbe_init_locked(struct adapter *);
static void     ixgbe_stop(void *);
static void     ixgbe_init_device_features(struct adapter *);
static void     ixgbe_check_fan_failure(struct adapter *, u32, bool);
static void	ixgbe_add_media_types(struct adapter *);
static void     ixgbe_media_status(struct ifnet *, struct ifmediareq *);
static int      ixgbe_media_change(struct ifnet *);
static int      ixgbe_allocate_pci_resources(struct adapter *,
		    const struct pci_attach_args *);
static void      ixgbe_free_softint(struct adapter *);
static void	ixgbe_get_slot_info(struct adapter *);
static int      ixgbe_allocate_msix(struct adapter *,
		    const struct pci_attach_args *);
static int      ixgbe_allocate_legacy(struct adapter *,
		    const struct pci_attach_args *);
static int      ixgbe_configure_interrupts(struct adapter *);
static void	ixgbe_free_pciintr_resources(struct adapter *);
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
static void	ixgbe_eitr_write(struct ix_queue *, uint32_t);

static void	ixgbe_setup_vlan_hw_support(struct adapter *);
#if 0
static void	ixgbe_register_vlan(void *, struct ifnet *, u16);
static void	ixgbe_unregister_vlan(void *, struct ifnet *, u16);
#endif

static void	ixgbe_add_device_sysctls(struct adapter *);
static void     ixgbe_add_hw_stats(struct adapter *);
static void	ixgbe_clear_evcnt(struct adapter *);
static int	ixgbe_set_flowcntl(struct adapter *, int);
static int	ixgbe_set_advertise(struct adapter *, int);
static int      ixgbe_get_advertise(struct adapter *);

/* Sysctl handlers */
static void	ixgbe_set_sysctl_value(struct adapter *, const char *,
		     const char *, int *, int);
static int	ixgbe_sysctl_flowcntl(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_advertise(SYSCTLFN_PROTO);
static int      ixgbe_sysctl_interrupt_rate_handler(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_dmac(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_phy_temp(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_phy_overtemp_occurred(SYSCTLFN_PROTO);
#ifdef IXGBE_DEBUG
static int	ixgbe_sysctl_power_state(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_print_rss_config(SYSCTLFN_PROTO);
#endif
static int      ixgbe_sysctl_rdh_handler(SYSCTLFN_PROTO);
static int      ixgbe_sysctl_rdt_handler(SYSCTLFN_PROTO);
static int      ixgbe_sysctl_tdt_handler(SYSCTLFN_PROTO);
static int      ixgbe_sysctl_tdh_handler(SYSCTLFN_PROTO);
static int      ixgbe_sysctl_eee_state(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_wol_enable(SYSCTLFN_PROTO);
static int	ixgbe_sysctl_wufc(SYSCTLFN_PROTO);

/* Support for pluggable optic modules */
static bool	ixgbe_sfp_probe(struct adapter *);

/* Legacy (single vector) interrupt handler */
static int	ixgbe_legacy_irq(void *);

/* The MSI/MSI-X Interrupt handlers */
static int	ixgbe_msix_que(void *);
static int	ixgbe_msix_link(void *);

/* Software interrupts for deferred work */
static void	ixgbe_handle_que(void *);
static void	ixgbe_handle_link(void *);
static void	ixgbe_handle_msf(void *);
static void	ixgbe_handle_mod(void *);
static void	ixgbe_handle_phy(void *);

/* Workqueue handler for deferred work */
static void	ixgbe_handle_que_work(struct work *, void *);

static ixgbe_vendor_info_t *ixgbe_lookup(const struct pci_attach_args *);

/************************************************************************
 *  NetBSD Device Interface Entry Points
 ************************************************************************/
CFATTACH_DECL3_NEW(ixg, sizeof(struct adapter),
    ixgbe_probe, ixgbe_attach, ixgbe_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

#if 0
devclass_t ix_devclass;
DRIVER_MODULE(ix, pci, ix_driver, ix_devclass, 0, 0);

MODULE_DEPEND(ix, pci, 1, 1, 1);
MODULE_DEPEND(ix, ether, 1, 1, 1);
#ifdef DEV_NETMAP
MODULE_DEPEND(ix, netmap, 1, 1, 1);
#endif
#endif

/*
 * TUNEABLE PARAMETERS:
 */

/*
 * AIM: Adaptive Interrupt Moderation
 * which means that the interrupt rate
 * is varied over time based on the
 * traffic for that interrupt vector
 */
static bool ixgbe_enable_aim = true;
#define SYSCTL_INT(_a1, _a2, _a3, _a4, _a5, _a6, _a7)
SYSCTL_INT(_hw_ix, OID_AUTO, enable_aim, CTLFLAG_RDTUN, &ixgbe_enable_aim, 0,
    "Enable adaptive interrupt moderation");

static int ixgbe_max_interrupt_rate = (4000000 / IXGBE_LOW_LATENCY);
SYSCTL_INT(_hw_ix, OID_AUTO, max_interrupt_rate, CTLFLAG_RDTUN,
    &ixgbe_max_interrupt_rate, 0, "Maximum interrupts per second");

/* How many packets rxeof tries to clean at a time */
static int ixgbe_rx_process_limit = 256;
SYSCTL_INT(_hw_ix, OID_AUTO, rx_process_limit, CTLFLAG_RDTUN,
    &ixgbe_rx_process_limit, 0, "Maximum number of received packets to process at a time, -1 means unlimited");

/* How many packets txeof tries to clean at a time */
static int ixgbe_tx_process_limit = 256;
SYSCTL_INT(_hw_ix, OID_AUTO, tx_process_limit, CTLFLAG_RDTUN,
    &ixgbe_tx_process_limit, 0,
    "Maximum number of sent packets to process at a time, -1 means unlimited");

/* Flow control setting, default to full */
static int ixgbe_flow_control = ixgbe_fc_full;
SYSCTL_INT(_hw_ix, OID_AUTO, flow_control, CTLFLAG_RDTUN,
    &ixgbe_flow_control, 0, "Default flow control used for all adapters");

/* Which pakcet processing uses workqueue or softint */
static bool ixgbe_txrx_workqueue = false;

/*
 * Smart speed setting, default to on
 * this only works as a compile option
 * right now as its during attach, set
 * this to 'ixgbe_smart_speed_off' to
 * disable.
 */
static int ixgbe_smart_speed = ixgbe_smart_speed_on;

/*
 * MSI-X should be the default for best performance,
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
 * Number of TX descriptors per ring,
 * setting higher than RX as this seems
 * the better performing choice.
 */
static int ixgbe_txd = PERFORM_TXD;
SYSCTL_INT(_hw_ix, OID_AUTO, txd, CTLFLAG_RDTUN, &ixgbe_txd, 0,
    "Number of transmit descriptors per queue");

/* Number of RX descriptors per ring */
static int ixgbe_rxd = PERFORM_RXD;
SYSCTL_INT(_hw_ix, OID_AUTO, rxd, CTLFLAG_RDTUN, &ixgbe_rxd, 0,
    "Number of receive descriptors per queue");

/*
 * Defining this on will allow the use
 * of unsupported SFP+ modules, note that
 * doing so you are on your own :)
 */
static int allow_unsupported_sfp = false;
#define TUNABLE_INT(__x, __y)
TUNABLE_INT("hw.ix.unsupported_sfp", &allow_unsupported_sfp);

/*
 * Not sure if Flow Director is fully baked,
 * so we'll default to turning it off.
 */
static int ixgbe_enable_fdir = 0;
SYSCTL_INT(_hw_ix, OID_AUTO, enable_fdir, CTLFLAG_RDTUN, &ixgbe_enable_fdir, 0,
    "Enable Flow Director");

/* Legacy Transmit (single queue) */
static int ixgbe_enable_legacy_tx = 0;
SYSCTL_INT(_hw_ix, OID_AUTO, enable_legacy_tx, CTLFLAG_RDTUN,
    &ixgbe_enable_legacy_tx, 0, "Enable Legacy TX flow");

/* Receive-Side Scaling */
static int ixgbe_enable_rss = 1;
SYSCTL_INT(_hw_ix, OID_AUTO, enable_rss, CTLFLAG_RDTUN, &ixgbe_enable_rss, 0,
    "Enable Receive-Side Scaling (RSS)");

/* Keep running tab on them for sanity check */
static int ixgbe_total_ports;

#if 0
static int (*ixgbe_start_locked)(struct ifnet *, struct tx_ring *);
static int (*ixgbe_ring_empty)(struct ifnet *, pcq_t *);
#endif

#ifdef NET_MPSAFE
#define IXGBE_MPSAFE		1
#define IXGBE_CALLOUT_FLAGS	CALLOUT_MPSAFE
#define IXGBE_SOFTINFT_FLAGS	SOFTINT_MPSAFE
#define IXGBE_WORKQUEUE_FLAGS	WQ_PERCPU | WQ_MPSAFE
#else
#define IXGBE_CALLOUT_FLAGS	0
#define IXGBE_SOFTINFT_FLAGS	0
#define IXGBE_WORKQUEUE_FLAGS	WQ_PERCPU
#endif
#define IXGBE_WORKQUEUE_PRI PRI_SOFTNET

/************************************************************************
 * ixgbe_initialize_rss_mapping
 ************************************************************************/
static void
ixgbe_initialize_rss_mapping(struct adapter *adapter)
{
	struct ixgbe_hw	*hw = &adapter->hw;
	u32             reta = 0, mrqc, rss_key[10];
	int             queue_id, table_size, index_mult;
	int             i, j;
	u32             rss_hash_config;

	/* force use default RSS key. */
#ifdef __NetBSD__
	rss_getkey((uint8_t *) &rss_key);
#else
	if (adapter->feat_en & IXGBE_FEATURE_RSS) {
		/* Fetch the configured RSS key */
		rss_getkey((uint8_t *) &rss_key);
	} else {
		/* set up random bits */
		cprng_fast(&rss_key, sizeof(rss_key));
	}
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
	case ixgbe_mac_X550EM_a:
		table_size = 512;
		break;
	default:
		break;
	}

	/* Set up the redirection table */
	for (i = 0, j = 0; i < table_size; i++, j++) {
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
			queue_id = (j * index_mult);

		/*
		 * The low 8 bits are for hash value (n+0);
		 * The next 8 bits are for hash value (n+1), etc.
		 */
		reta = reta >> 8;
		reta = reta | (((uint32_t) queue_id) << 24);
		if ((i & 3) == 3) {
			if (i < 128)
				IXGBE_WRITE_REG(hw, IXGBE_RETA(i >> 2), reta);
			else
				IXGBE_WRITE_REG(hw, IXGBE_ERETA((i >> 2) - 32),
				    reta);
			reta = 0;
		}
	}

	/* Now fill our hash function seeds */
	for (i = 0; i < 10; i++)
		IXGBE_WRITE_REG(hw, IXGBE_RSSRK(i), rss_key[i]);

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
		                | RSS_HASHTYPE_RSS_TCP_IPV6
		                | RSS_HASHTYPE_RSS_IPV6_EX
		                | RSS_HASHTYPE_RSS_TCP_IPV6_EX;
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
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX;
	if (rss_hash_config & RSS_HASHTYPE_RSS_TCP_IPV6_EX)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX_TCP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV4)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_UDP;
	if (rss_hash_config & RSS_HASHTYPE_RSS_UDP_IPV6_EX)
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV6_EX_UDP;
	mrqc |= ixgbe_get_mrqc(adapter->iov_mode);
	IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);
} /* ixgbe_initialize_rss_mapping */

/************************************************************************
 * ixgbe_initialize_receive_units - Setup receive registers and features.
 ************************************************************************/
#define BSIZEPKT_ROUNDUP ((1<<IXGBE_SRRCTL_BSIZEPKT_SHIFT)-1)
	
static void
ixgbe_initialize_receive_units(struct adapter *adapter)
{
	struct	rx_ring	*rxr = adapter->rx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet    *ifp = adapter->ifp;
	int             i, j;
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
	/* CRC stripping is conditional in Netmap */
	if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) &&
	    (ifp->if_capenable & IFCAP_NETMAP) &&
	    !ix_crcstrip)
		hlreg &= ~IXGBE_HLREG0_RXCRCSTRP;
	else
#endif /* DEV_NETMAP */
		hlreg |= IXGBE_HLREG0_RXCRCSTRP;

	IXGBE_WRITE_REG(hw, IXGBE_HLREG0, hlreg);

	bufsz = (adapter->rx_mbuf_sz + BSIZEPKT_ROUNDUP) >>
	    IXGBE_SRRCTL_BSIZEPKT_SHIFT;

	for (i = 0; i < adapter->num_queues; i++, rxr++) {
		u64 rdba = rxr->rxdma.dma_paddr;
		u32 tqsmreg, reg;
		int regnum = i / 4;	/* 1 register per 4 queues */
		int regshift = i % 4;	/* 4 bits per 1 queue */
		j = rxr->me;

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

		/* Set RQSMR (Receive Queue Statistic Mapping) register */
		reg = IXGBE_READ_REG(hw, IXGBE_RQSMR(regnum));
		reg &= ~(0x000000ff << (regshift * 8));
		reg |= i << (regshift * 8);
		IXGBE_WRITE_REG(hw, IXGBE_RQSMR(regnum), reg);

		/*
		 * Set RQSMR (Receive Queue Statistic Mapping) register.
		 * Register location for queue 0...7 are different between
		 * 82598 and newer.
		 */
		if (adapter->hw.mac.type == ixgbe_mac_82598EB)
			tqsmreg = IXGBE_TQSMR(regnum);
		else
			tqsmreg = IXGBE_TQSM(regnum);
		reg = IXGBE_READ_REG(hw, tqsmreg);
		reg &= ~(0x000000ff << (regshift * 8));
		reg |= i << (regshift * 8);
		IXGBE_WRITE_REG(hw, tqsmreg, reg);

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
		u32 psrtype = IXGBE_PSRTYPE_TCPHDR
		            | IXGBE_PSRTYPE_UDPHDR
		            | IXGBE_PSRTYPE_IPV4HDR
		            | IXGBE_PSRTYPE_IPV6HDR;
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
} /* ixgbe_initialize_receive_units */

/************************************************************************
 * ixgbe_initialize_transmit_units - Enable transmit units.
 ************************************************************************/
static void
ixgbe_initialize_transmit_units(struct adapter *adapter)
{
	struct tx_ring  *txr = adapter->tx_rings;
	struct ixgbe_hw	*hw = &adapter->hw;

	/* Setup the Base and Length of the Tx Descriptor Ring */
	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		u64 tdba = txr->txdma.dma_paddr;
		u32 txctrl = 0;
		int j = txr->me;

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

		dmatxctl = IXGBE_READ_REG(hw, IXGBE_DMATXCTL);
		dmatxctl |= IXGBE_DMATXCTL_TE;
		IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL, dmatxctl);
		/* Disable arbiter to set MTQC */
		rttdcs = IXGBE_READ_REG(hw, IXGBE_RTTDCS);
		rttdcs |= IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
		IXGBE_WRITE_REG(hw, IXGBE_MTQC,
		    ixgbe_get_mtqc(adapter->iov_mode));
		rttdcs &= ~IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
	}

	return;
} /* ixgbe_initialize_transmit_units */

/************************************************************************
 * ixgbe_attach - Device initialization routine
 *
 *   Called when the driver is being loaded.
 *   Identifies the type of hardware, allocates all resources
 *   and initializes the hardware.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static void
ixgbe_attach(device_t parent, device_t dev, void *aux)
{
	struct adapter  *adapter;
	struct ixgbe_hw *hw;
	int             error = -1;
	u32		ctrl_ext;
	u16		high, low, nvmreg;
	pcireg_t	id, subid;
	ixgbe_vendor_info_t *ent;
	struct pci_attach_args *pa = aux;
	const char *str;
	char buf[256];

	INIT_DEBUGOUT("ixgbe_attach: begin");

	/* Allocate, clear, and link in our adapter structure */
	adapter = device_private(dev);
	adapter->hw.back = adapter;
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

	/* Core Lock Init*/
	IXGBE_CORE_LOCK_INIT(adapter, device_xname(dev));

	/* Set up the timer callout */
	callout_init(&adapter->timer, IXGBE_CALLOUT_FLAGS);

	/* Determine hardware revision */
	id = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_ID_REG);
	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	hw->vendor_id = PCI_VENDOR(id);
	hw->device_id = PCI_PRODUCT(id);
	hw->revision_id =
	    PCI_REVISION(pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG));
	hw->subsystem_vendor_id = PCI_SUBSYS_VENDOR(subid);
	hw->subsystem_device_id = PCI_SUBSYS_ID(subid);

	/*
	 * Make sure BUSMASTER is set
	 */
	ixgbe_pci_enable_busmaster(pa->pa_pc, pa->pa_tag);

	/* Do base PCI setup - map BAR0 */
	if (ixgbe_allocate_pci_resources(adapter, pa)) {
		aprint_error_dev(dev, "Allocation of PCI resources failed\n");
		error = ENXIO;
		goto err_out;
	}

	/* let hardware know driver is loaded */
	ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
	ctrl_ext |= IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);

	/*
	 * Initialize the shared code
	 */
	if (ixgbe_init_shared_code(hw)) {
		aprint_error_dev(dev, "Unable to initialize the shared code\n");
		error = ENXIO;
		goto err_out;
	}

	switch (hw->mac.type) {
	case ixgbe_mac_82598EB:
		str = "82598EB";
		break;
	case ixgbe_mac_82599EB:
		str = "82599EB";
		break;
	case ixgbe_mac_X540:
		str = "X540";
		break;
	case ixgbe_mac_X550:
		str = "X550";
		break;
	case ixgbe_mac_X550EM_x:
		str = "X550EM";
		break;
	case ixgbe_mac_X550EM_a:
		str = "X550EM A";
		break;
	default:
		str = "Unknown";
		break;
	}
	aprint_normal_dev(dev, "device %s\n", str);

	if (hw->mbx.ops.init_params)
		hw->mbx.ops.init_params(hw);

	hw->allow_unsupported_sfp = allow_unsupported_sfp;

	/* Pick up the 82599 settings */
	if (hw->mac.type != ixgbe_mac_82598EB) {
		hw->phy.smart_speed = ixgbe_smart_speed;
		adapter->num_segs = IXGBE_82599_SCATTER;
	} else
		adapter->num_segs = IXGBE_82598_SCATTER;

	hw->mac.ops.set_lan_id(hw);
	ixgbe_init_device_features(adapter);

	if (ixgbe_configure_interrupts(adapter)) {
		error = ENXIO;
		goto err_out;
	}

	/* Allocate multicast array memory. */
	adapter->mta = malloc(sizeof(*adapter->mta) *
	    MAX_NUM_MULTICAST_ADDRESSES, M_DEVBUF, M_NOWAIT);
	if (adapter->mta == NULL) {
		aprint_error_dev(dev, "Cannot allocate multicast setup array\n");
		error = ENOMEM;
		goto err_out;
	}

	/* Enable WoL (if supported) */
	ixgbe_check_wol_support(adapter);

	/* Verify adapter fan is still functional (if applicable) */
	if (adapter->feat_en & IXGBE_FEATURE_FAN_FAIL) {
		u32 esdp = IXGBE_READ_REG(hw, IXGBE_ESDP);
		ixgbe_check_fan_failure(adapter, esdp, FALSE);
	}

	/* Ensure SW/FW semaphore is free */
	ixgbe_init_swfw_semaphore(hw);

	/* Enable EEE power saving */
	if (adapter->feat_en & IXGBE_FEATURE_EEE)
		hw->mac.ops.setup_eee(hw, TRUE);

	/* Set an initial default flow control value */
	hw->fc.requested_mode = ixgbe_flow_control;

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
	 * With many RX rings it is easy to exceed the
	 * system mbuf allocation. Tuning nmbclusters
	 * can alleviate this.
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

	hw->phy.reset_if_overtemp = TRUE;
	error = ixgbe_reset_hw(hw);
	hw->phy.reset_if_overtemp = FALSE;
	if (error == IXGBE_ERR_SFP_NOT_PRESENT) {
		/*
		 * No optics in this port, set up
		 * so the timer routine will probe
		 * for later insertion.
		 */
		adapter->sfp_probe = TRUE;
		error = IXGBE_SUCCESS;
	} else if (error == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		aprint_error_dev(dev, "Unsupported SFP+ module detected!\n");
		error = EIO;
		goto err_late;
	} else if (error) {
		aprint_error_dev(dev, "Hardware initialization failed\n");
		error = EIO;
		goto err_late;
	}

	/* Make sure we have a good EEPROM before we read from it */
	if (ixgbe_validate_eeprom_checksum(&adapter->hw, NULL) < 0) {
		aprint_error_dev(dev, "The EEPROM Checksum Is Not Valid\n");
		error = EIO;
		goto err_late;
	}

	aprint_normal("%s:", device_xname(dev));
	/* NVM Image Version */
	switch (hw->mac.type) {
	case ixgbe_mac_X540:
	case ixgbe_mac_X550EM_a:
		hw->eeprom.ops.read(hw, IXGBE_NVM_IMAGE_VER, &nvmreg);
		if (nvmreg == 0xffff)
			break;
		high = (nvmreg >> 12) & 0x0f;
		low = (nvmreg >> 4) & 0xff;
		id = nvmreg & 0x0f;
		aprint_normal(" NVM Image Version %u.", high);
		if (hw->mac.type == ixgbe_mac_X540)
			str = "%x";
		else
			str = "%02x";
		aprint_normal(str, low);
		aprint_normal(" ID 0x%x,", id);
		break;
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550:
		hw->eeprom.ops.read(hw, IXGBE_NVM_IMAGE_VER, &nvmreg);
		if (nvmreg == 0xffff)
			break;
		high = (nvmreg >> 12) & 0x0f;
		low = nvmreg & 0xff;
		aprint_normal(" NVM Image Version %u.%02x,", high, low);
		break;
	default:
		break;
	}

	/* PHY firmware revision */
	switch (hw->mac.type) {
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
		hw->eeprom.ops.read(hw, IXGBE_PHYFW_REV, &nvmreg);
		if (nvmreg == 0xffff)
			break;
		high = (nvmreg >> 12) & 0x0f;
		low = (nvmreg >> 4) & 0xff;
		id = nvmreg & 0x000f;
		aprint_normal(" PHY FW Revision %u.", high);
		if (hw->mac.type == ixgbe_mac_X540)
			str = "%x";
		else
			str = "%02x";
		aprint_normal(str, low);
		aprint_normal(" ID 0x%x,", id);
		break;
	default:
		break;
	}

	/* NVM Map version & OEM NVM Image version */
	switch (hw->mac.type) {
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
		hw->eeprom.ops.read(hw, IXGBE_NVM_MAP_VER, &nvmreg);
		if (nvmreg != 0xffff) {
			high = (nvmreg >> 12) & 0x0f;
			low = nvmreg & 0x00ff;
			aprint_normal(" NVM Map version %u.%02x,", high, low);
		}
		hw->eeprom.ops.read(hw, IXGBE_OEM_NVM_IMAGE_VER, &nvmreg);
		if (nvmreg != 0xffff) {
			high = (nvmreg >> 12) & 0x0f;
			low = nvmreg & 0x00ff;
			aprint_verbose(" OEM NVM Image version %u.%02x,", high,
			    low);
		}
		break;
	default:
		break;
	}

	/* Print the ETrackID */
	hw->eeprom.ops.read(hw, IXGBE_ETRACKID_H, &high);
	hw->eeprom.ops.read(hw, IXGBE_ETRACKID_L, &low);
	aprint_normal(" ETrackID %08x\n", ((uint32_t)high << 16) | low);

	if (adapter->feat_en & IXGBE_FEATURE_MSIX) {
		error = ixgbe_allocate_msix(adapter, pa);
		if (error) {
			/* Free allocated queue structures first */
			ixgbe_free_transmit_structures(adapter);
			ixgbe_free_receive_structures(adapter);
			free(adapter->queues, M_DEVBUF);

			/* Fallback to legacy interrupt */
			adapter->feat_en &= ~IXGBE_FEATURE_MSIX;
			if (adapter->feat_cap & IXGBE_FEATURE_MSI)
				adapter->feat_en |= IXGBE_FEATURE_MSI;
			adapter->num_queues = 1;

			/* Allocate our TX/RX Queues again */
			if (ixgbe_allocate_queues(adapter)) {
				error = ENOMEM;
				goto err_out;
			}
		}
	}
	if ((adapter->feat_en & IXGBE_FEATURE_MSIX) == 0)
		error = ixgbe_allocate_legacy(adapter, pa);
	if (error) 
		goto err_late;

	/* Tasklets for Link, SFP, Multispeed Fiber and Flow Director */
	adapter->link_si = softint_establish(SOFTINT_NET |IXGBE_SOFTINFT_FLAGS,
	    ixgbe_handle_link, adapter);
	adapter->mod_si = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
	    ixgbe_handle_mod, adapter);
	adapter->msf_si = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
	    ixgbe_handle_msf, adapter);
	adapter->phy_si = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
	    ixgbe_handle_phy, adapter);
	if (adapter->feat_en & IXGBE_FEATURE_FDIR)
		adapter->fdir_si =
		    softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixgbe_reinit_fdir, adapter);
	if ((adapter->link_si == NULL) || (adapter->mod_si == NULL)
	    || (adapter->msf_si == NULL) || (adapter->phy_si == NULL)
	    || ((adapter->feat_en & IXGBE_FEATURE_FDIR)
		&& (adapter->fdir_si == NULL))) {
		aprint_error_dev(dev,
		    "could not establish software interrupts ()\n");
		goto err_out;
	}

	error = ixgbe_start_hw(hw);
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
		goto err_late;
	case IXGBE_ERR_SFP_NOT_PRESENT:
		aprint_error_dev(dev, "No SFP+ Module found\n");
		/* falls thru */
	default:
		break;
	}

	/* Setup OS specific network interface */
	if (ixgbe_setup_interface(dev, adapter) != 0)
		goto err_late;

	/*
	 *  Print PHY ID only for copper PHY. On device which has SFP(+) cage
	 * and a module is inserted, phy.id is not MII PHY id but SFF 8024 ID.
	 */
	if (hw->phy.media_type == ixgbe_media_type_copper) {
		uint16_t id1, id2;
		int oui, model, rev;
		const char *descr;

		id1 = hw->phy.id >> 16;
		id2 = hw->phy.id & 0xffff;
		oui = MII_OUI(id1, id2);
		model = MII_MODEL(id2);
		rev = MII_REV(id2);
		if ((descr = mii_get_descr(oui, model)) != NULL)
			aprint_normal_dev(dev,
			    "PHY: %s (OUI 0x%06x, model 0x%04x), rev. %d\n",
			    descr, oui, model, rev);
		else
			aprint_normal_dev(dev,
			    "PHY OUI 0x%06x, model 0x%04x, rev. %d\n",
			    oui, model, rev);
	}

	/* Enable the optics for 82599 SFP+ fiber */
	ixgbe_enable_tx_laser(hw);

	/* Enable power to the phy. */
	ixgbe_set_phy_power(hw, TRUE);

	/* Initialize statistics */
	ixgbe_update_stats_counters(adapter);

	/* Check PCIE slot type/speed/width */
	ixgbe_get_slot_info(adapter);

	/*
	 * Do time init and sysctl init here, but
	 * only on the first port of a bypass adapter.
	 */
	ixgbe_bypass_init(adapter);

	/* Set an initial dmac value */
	adapter->dmac = 0;
	/* Set initial advertised speeds (if applicable) */
	adapter->advertise = ixgbe_get_advertise(adapter);

	if (adapter->feat_cap & IXGBE_FEATURE_SRIOV)
		ixgbe_define_iov_schemas(dev, &error);

	/* Add sysctls */
	ixgbe_add_device_sysctls(adapter);
	ixgbe_add_hw_stats(adapter);

	/* For Netmap */
	adapter->init_locked = ixgbe_init_locked;
	adapter->stop_locked = ixgbe_stop;

	if (adapter->feat_en & IXGBE_FEATURE_NETMAP)
		ixgbe_netmap_attach(adapter);

	snprintb(buf, sizeof(buf), IXGBE_FEATURE_FLAGS, adapter->feat_cap);
	aprint_verbose_dev(dev, "feature cap %s\n", buf);
	snprintb(buf, sizeof(buf), IXGBE_FEATURE_FLAGS, adapter->feat_en);
	aprint_verbose_dev(dev, "feature ena %s\n", buf);

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
	free(adapter->queues, M_DEVBUF);
err_out:
	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	ctrl_ext &= ~IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT, ctrl_ext);
	ixgbe_free_softint(adapter);
	ixgbe_free_pci_resources(adapter);
	if (adapter->mta != NULL)
		free(adapter->mta, M_DEVBUF);
	IXGBE_CORE_LOCK_DESTROY(adapter);

	return;
} /* ixgbe_attach */

/************************************************************************
 * ixgbe_check_wol_support
 *
 *   Checks whether the adapter's ports are capable of
 *   Wake On LAN by reading the adapter's NVM.
 *
 *   Sets each port's hw->wol_enabled value depending
 *   on the value read here.
 ************************************************************************/
static void
ixgbe_check_wol_support(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u16             dev_caps = 0;

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
} /* ixgbe_check_wol_support */

/************************************************************************
 * ixgbe_setup_interface
 *
 *   Setup networking device structure and register an interface.
 ************************************************************************/
static int
ixgbe_setup_interface(device_t dev, struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ifnet   *ifp;
	int rv;

	INIT_DEBUGOUT("ixgbe_setup_interface: begin");

	ifp = adapter->ifp = &ec->ec_if;
	strlcpy(ifp->if_xname, device_xname(dev), IFNAMSIZ);
	ifp->if_baudrate = IF_Gbps(10);
	ifp->if_init = ixgbe_init;
	ifp->if_stop = ixgbe_ifstop;
	ifp->if_softc = adapter;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef IXGBE_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_ioctl = ixgbe_ioctl;
#if __FreeBSD_version >= 1100045
	/* TSO parameters */
	ifp->if_hw_tsomax = 65518;
	ifp->if_hw_tsomaxsegcount = IXGBE_82599_SCATTER;
	ifp->if_hw_tsomaxsegsize = 2048;
#endif
	if (adapter->feat_en & IXGBE_FEATURE_LEGACY_TX) {
#if 0
		ixgbe_start_locked = ixgbe_legacy_start_locked;
#endif
	} else {
		ifp->if_transmit = ixgbe_mq_start;
#if 0
		ixgbe_start_locked = ixgbe_mq_start_locked;
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
	/*
	 * We use per TX queue softint, so if_deferred_start_init() isn't
	 * used.
	 */
	if_register(ifp);
	ether_set_ifflags_cb(ec, ixgbe_ifflags_cb);

	adapter->max_frame_size = ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

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
	 * Don't turn this on by default, if vlans are
	 * created on another pseudo device (eg. lagg)
	 * then vlan events are not passed thru, breaking
	 * operation, but with HW FILTER off it works. If
	 * using vlans directly on the ixgbe driver you can
	 * enable this and get full hardware tag filtering.
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
} /* ixgbe_setup_interface */

/************************************************************************
 * ixgbe_add_media_types
 ************************************************************************/
static void
ixgbe_add_media_types(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	device_t        dev = adapter->dev;
	u64             layer;

	layer = adapter->phy_layer;

#define	ADD(mm, dd)							\
	ifmedia_add(&adapter->media, IFM_ETHER | (mm), (dd), NULL);

	/* Media types with matching NetBSD media defines */
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_T) {
		ADD(IFM_10G_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_T) {
		ADD(IFM_1000_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_100BASE_TX) {
		ADD(IFM_100_TX | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10BASE_T) {
		ADD(IFM_10_T | IFM_FDX, 0);
	}

	if (layer & IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU ||
	    layer & IXGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA) {
		ADD(IFM_10G_TWINAX | IFM_FDX, 0);
	}

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_LR) {
		ADD(IFM_10G_LR | IFM_FDX, 0);
		if (hw->phy.multispeed_fiber) {
			ADD(IFM_1000_LX | IFM_FDX, 0);
		}
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_SR) {
		ADD(IFM_10G_SR | IFM_FDX, 0);
		if (hw->phy.multispeed_fiber) {
			ADD(IFM_1000_SX | IFM_FDX, 0);
		}
	} else if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_SX) {
		ADD(IFM_1000_SX | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_CX4) {
		ADD(IFM_10G_CX4 | IFM_FDX, 0);
	}

#ifdef IFM_ETH_XTYPE
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR) {
		ADD(IFM_10G_KR | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4) {
		ADD(AIFM_10G_KX4 | IFM_FDX, 0);
	}
#else
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR) {
		device_printf(dev, "Media supported: 10GbaseKR\n");
		device_printf(dev, "10GbaseKR mapped to 10GbaseSR\n");
		ADD(IFM_10G_SR | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4) {
		device_printf(dev, "Media supported: 10GbaseKX4\n");
		device_printf(dev, "10GbaseKX4 mapped to 10GbaseCX4\n");
		ADD(IFM_10G_CX4 | IFM_FDX, 0);
	}
#endif
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX) {
		ADD(IFM_1000_KX | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_2500BASE_KX) {
		ADD(IFM_2500_KX | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_2500BASE_T) {
		ADD(IFM_2500_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_5GBASE_T) {
		ADD(IFM_5000_T | IFM_FDX, 0);
	}
	if (layer & IXGBE_PHYSICAL_LAYER_1000BASE_BX)
		device_printf(dev, "Media supported: 1000baseBX\n");
	/* XXX no ifmedia_set? */
	
	ADD(IFM_AUTO, 0);

#undef ADD
} /* ixgbe_add_media_types */

/************************************************************************
 * ixgbe_is_sfp
 ************************************************************************/
static inline bool
ixgbe_is_sfp(struct ixgbe_hw *hw)
{
	switch (hw->mac.type) {
	case ixgbe_mac_82598EB:
		if (hw->phy.type == ixgbe_phy_nl)
			return TRUE;
		return FALSE;
	case ixgbe_mac_82599EB:
		switch (hw->mac.ops.get_media_type(hw)) {
		case ixgbe_media_type_fiber:
		case ixgbe_media_type_fiber_qsfp:
			return TRUE;
		default:
			return FALSE;
		}
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
		if (hw->mac.ops.get_media_type(hw) == ixgbe_media_type_fiber)
			return TRUE;
		return FALSE;
	default:
		return FALSE;
	}
} /* ixgbe_is_sfp */

/************************************************************************
 * ixgbe_config_link
 ************************************************************************/
static void
ixgbe_config_link(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32             autoneg, err = 0;
	bool            sfp, negotiate = false;

	sfp = ixgbe_is_sfp(hw);

	if (sfp) { 
		if (hw->phy.multispeed_fiber) {
			hw->mac.ops.setup_sfp(hw);
			ixgbe_enable_tx_laser(hw);
			kpreempt_disable();
			softint_schedule(adapter->msf_si);
			kpreempt_enable();
		} else {
			kpreempt_disable();
			softint_schedule(adapter->mod_si);
			kpreempt_enable();
		}
	} else {
		if (hw->mac.ops.check_link)
			err = ixgbe_check_link(hw, &adapter->link_speed,
			    &adapter->link_up, FALSE);
		if (err)
			goto out;
		autoneg = hw->phy.autoneg_advertised;
		if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
                	err = hw->mac.ops.get_link_capabilities(hw, &autoneg,
			    &negotiate);
		if (err)
			goto out;
		if (hw->mac.ops.setup_link)
                	err = hw->mac.ops.setup_link(hw, autoneg,
			    adapter->link_up);
	}
out:

	return;
} /* ixgbe_config_link */

/************************************************************************
 * ixgbe_update_stats_counters - Update board statistics counters.
 ************************************************************************/
static void
ixgbe_update_stats_counters(struct adapter *adapter)
{
	struct ifnet          *ifp = adapter->ifp;
	struct ixgbe_hw       *hw = &adapter->hw;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;
	u32                   missed_rx = 0, bprc, lxon, lxoff, total;
	u64                   total_missed_rx = 0;
	uint64_t              crcerrs, rlec;

	crcerrs = IXGBE_READ_REG(hw, IXGBE_CRCERRS);
	stats->crcerrs.ev_count += crcerrs;
	stats->illerrc.ev_count += IXGBE_READ_REG(hw, IXGBE_ILLERRC);
	stats->errbc.ev_count += IXGBE_READ_REG(hw, IXGBE_ERRBC);
	stats->mspdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MSPDC);
	if (hw->mac.type == ixgbe_mac_X550)
		stats->mbsdc.ev_count += IXGBE_READ_REG(hw, IXGBE_MBSDC);

	for (int i = 0; i < __arraycount(stats->qprc); i++) {
		int j = i % adapter->num_queues;
		stats->qprc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRC(i));
		stats->qptc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPTC(i));
		stats->qprdc[j].ev_count += IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
	}
	for (int i = 0; i < __arraycount(stats->mpc); i++) {
		uint32_t mp;
		int j = i % adapter->num_queues;

		mp = IXGBE_READ_REG(hw, IXGBE_MPC(i));
		/* global total per queue */
		stats->mpc[j].ev_count += mp;
		/* running comprehensive total for stats display */
		total_missed_rx += mp;

		if (hw->mac.type == ixgbe_mac_82598EB)
			stats->rnbc[j].ev_count
			    += IXGBE_READ_REG(hw, IXGBE_RNBC(i));
		
	}
	stats->mpctotal.ev_count += total_missed_rx;

	/* Document says M[LR]FC are valid when link is up and 10Gbps */
	if ((adapter->link_active == TRUE)
	    && (adapter->link_speed == IXGBE_LINK_SPEED_10GB_FULL)) {
		stats->mlfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MLFC);
		stats->mrfc.ev_count += IXGBE_READ_REG(hw, IXGBE_MRFC);
	}
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
	stats->mprc.ev_count += IXGBE_READ_REG(hw, IXGBE_MPRC)
	    - ((hw->mac.type == ixgbe_mac_82598EB) ? bprc : 0);

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
} /* ixgbe_update_stats_counters */

/************************************************************************
 * ixgbe_add_hw_stats
 *
 *   Add sysctl variables, one per statistic, to the system.
 ************************************************************************/
static void
ixgbe_add_hw_stats(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	const struct sysctlnode *rnode, *cnode;
	struct sysctllog **log = &adapter->sysctllog;
	struct tx_ring *txr = adapter->tx_rings;
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;
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
			    "RX Missed Packet Count");
			if (hw->mac.type == ixgbe_mac_82598EB)
				evcnt_attach_dynamic(&stats->rnbc[i],
				    EVCNT_TYPE_MISC, NULL,
				    adapter->queues[i].evnamebuf,
				    "Receive No Buffers");
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
	if (hw->mac.type >= ixgbe_mac_X550)
		evcnt_attach_dynamic(&stats->mbsdc, EVCNT_TYPE_MISC, NULL,
		    stats->namebuf, "Bad SFD");
	evcnt_attach_dynamic(&stats->mpctotal, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Total Packets Missed");
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
	evcnt_attach_dynamic(&stats->mngpdc, EVCNT_TYPE_MISC, NULL,
	    stats->namebuf, "Management Packets Dropped");
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
} /* ixgbe_add_hw_stats */

static void
ixgbe_clear_evcnt(struct adapter *adapter)
{
	struct tx_ring *txr = adapter->tx_rings;
	struct rx_ring *rxr = adapter->rx_rings;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;

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

	txr = adapter->tx_rings;
	for (int i = 0; i < adapter->num_queues; i++, rxr++, txr++) {
		adapter->queues[i].irqs.ev_count = 0;
		adapter->queues[i].handleq.ev_count = 0;
		adapter->queues[i].req.ev_count = 0;
		txr->no_desc_avail.ev_count = 0;
		txr->total_packets.ev_count = 0;
		txr->tso_tx.ev_count = 0;
#ifndef IXGBE_LEGACY_TX
		txr->pcq_drops.ev_count = 0;
#endif

		if (i < __arraycount(stats->mpc)) {
			stats->mpc[i].ev_count = 0;
			if (hw->mac.type == ixgbe_mac_82598EB)
				stats->rnbc[i].ev_count = 0;
		}
		if (i < __arraycount(stats->pxontxc)) {
			stats->pxontxc[i].ev_count = 0;
			stats->pxonrxc[i].ev_count = 0;
			stats->pxofftxc[i].ev_count = 0;
			stats->pxoffrxc[i].ev_count = 0;
			stats->pxon2offc[i].ev_count = 0;
		}
		if (i < __arraycount(stats->qprc)) {
			stats->qprc[i].ev_count = 0;
			stats->qptc[i].ev_count = 0;
			stats->qbrc[i].ev_count = 0;
			stats->qbtc[i].ev_count = 0;
			stats->qprdc[i].ev_count = 0;
		}

		rxr->rx_packets.ev_count = 0;
		rxr->rx_bytes.ev_count = 0;
		rxr->rx_copies.ev_count = 0;
		rxr->no_jmbuf.ev_count = 0;
		rxr->rx_discarded.ev_count = 0;
	}
	stats->ipcs.ev_count = 0;
	stats->l4cs.ev_count = 0;
	stats->ipcs_bad.ev_count = 0;
	stats->l4cs_bad.ev_count = 0;
	stats->intzero.ev_count = 0;
	stats->legint.ev_count = 0;
	stats->crcerrs.ev_count = 0;
	stats->illerrc.ev_count = 0;
	stats->errbc.ev_count = 0;
	stats->mspdc.ev_count = 0;
	stats->mbsdc.ev_count = 0;
	stats->mpctotal.ev_count = 0;
	stats->mlfc.ev_count = 0;
	stats->mrfc.ev_count = 0;
	stats->rlec.ev_count = 0;
	stats->lxontxc.ev_count = 0;
	stats->lxonrxc.ev_count = 0;
	stats->lxofftxc.ev_count = 0;
	stats->lxoffrxc.ev_count = 0;

	/* Packet Reception Stats */
	stats->tor.ev_count = 0;
	stats->gorc.ev_count = 0;
	stats->tpr.ev_count = 0;
	stats->gprc.ev_count = 0;
	stats->mprc.ev_count = 0;
	stats->bprc.ev_count = 0;
	stats->prc64.ev_count = 0;
	stats->prc127.ev_count = 0;
	stats->prc255.ev_count = 0;
	stats->prc511.ev_count = 0;
	stats->prc1023.ev_count = 0;
	stats->prc1522.ev_count = 0;
	stats->ruc.ev_count = 0;
	stats->rfc.ev_count = 0;
	stats->roc.ev_count = 0;
	stats->rjc.ev_count = 0;
	stats->mngprc.ev_count = 0;
	stats->mngpdc.ev_count = 0;
	stats->xec.ev_count = 0;

	/* Packet Transmission Stats */
	stats->gotc.ev_count = 0;
	stats->tpt.ev_count = 0;
	stats->gptc.ev_count = 0;
	stats->bptc.ev_count = 0;
	stats->mptc.ev_count = 0;
	stats->mngptc.ev_count = 0;
	stats->ptc64.ev_count = 0;
	stats->ptc127.ev_count = 0;
	stats->ptc255.ev_count = 0;
	stats->ptc511.ev_count = 0;
	stats->ptc1023.ev_count = 0;
	stats->ptc1522.ev_count = 0;
}

/************************************************************************
 * ixgbe_sysctl_tdh_handler - Transmit Descriptor Head handler function
 *
 *   Retrieves the TDH value from the hardware
 ************************************************************************/
static int 
ixgbe_sysctl_tdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct tx_ring *txr = (struct tx_ring *)node.sysctl_data;
	uint32_t val;

	if (!txr)
		return (0);

	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_TDH(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixgbe_sysctl_tdh_handler */

/************************************************************************
 * ixgbe_sysctl_tdt_handler - Transmit Descriptor Tail handler function
 *
 *   Retrieves the TDT value from the hardware
 ************************************************************************/
static int 
ixgbe_sysctl_tdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct tx_ring *txr = (struct tx_ring *)node.sysctl_data;
	uint32_t val;

	if (!txr)
		return (0);

	val = IXGBE_READ_REG(&txr->adapter->hw, IXGBE_TDT(txr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixgbe_sysctl_tdt_handler */

/************************************************************************
 * ixgbe_sysctl_rdh_handler - Receive Descriptor Head handler function
 *
 *   Retrieves the RDH value from the hardware
 ************************************************************************/
static int 
ixgbe_sysctl_rdh_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	uint32_t val;

	if (!rxr)
		return (0);

	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_RDH(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixgbe_sysctl_rdh_handler */

/************************************************************************
 * ixgbe_sysctl_rdt_handler - Receive Descriptor Tail handler function
 *
 *   Retrieves the RDT value from the hardware
 ************************************************************************/
static int 
ixgbe_sysctl_rdt_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct rx_ring *rxr = (struct rx_ring *)node.sysctl_data;
	uint32_t val;

	if (!rxr)
		return (0);

	val = IXGBE_READ_REG(&rxr->adapter->hw, IXGBE_RDT(rxr->me));
	node.sysctl_data = &val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
} /* ixgbe_sysctl_rdt_handler */

#if 0	/* XXX Badly need to overhaul vlan(4) on NetBSD. */
/************************************************************************
 * ixgbe_register_vlan
 *
 *   Run via vlan config EVENT, it enables us to use the
 *   HW Filter table since we can get the vlan id. This
 *   just creates the entry in the soft version of the
 *   VFTA, init will repopulate the real table.
 ************************************************************************/
static void
ixgbe_register_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct adapter	*adapter = ifp->if_softc;
	u16		index, bit;

	if (ifp->if_softc != arg)   /* Not our event */
		return;

	if ((vtag == 0) || (vtag > 4095))	/* Invalid */
		return;

	IXGBE_CORE_LOCK(adapter);
	index = (vtag >> 5) & 0x7F;
	bit = vtag & 0x1F;
	adapter->shadow_vfta[index] |= (1 << bit);
	ixgbe_setup_vlan_hw_support(adapter);
	IXGBE_CORE_UNLOCK(adapter);
} /* ixgbe_register_vlan */

/************************************************************************
 * ixgbe_unregister_vlan
 *
 *   Run via vlan unconfig EVENT, remove our entry in the soft vfta.
 ************************************************************************/
static void
ixgbe_unregister_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct adapter	*adapter = ifp->if_softc;
	u16		index, bit;

	if (ifp->if_softc != arg)
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
} /* ixgbe_unregister_vlan */
#endif

static void
ixgbe_setup_vlan_hw_support(struct adapter *adapter)
{
	struct ethercom *ec = &adapter->osdep.ec;
	struct ixgbe_hw *hw = &adapter->hw;
	struct rx_ring	*rxr;
	int             i;
	u32		ctrl;


	/*
	 * We get here thru init_locked, meaning
	 * a soft reset, this has already cleared
	 * the VFTA and other state, so if there
	 * have been no vlan's registered do nothing.
	 */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		return;

	/* Setup the queues for vlans */
	if (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING) {
		for (i = 0; i < adapter->num_queues; i++) {
			rxr = &adapter->rx_rings[i];
			/* On 82599 the VLAN enable is per/queue in RXDCTL */
			if (hw->mac.type != ixgbe_mac_82598EB) {
				ctrl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(rxr->me));
				ctrl |= IXGBE_RXDCTL_VME;
				IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(rxr->me), ctrl);
			}
			rxr->vtag_strip = TRUE;
		}
	}

	if ((ec->ec_capenable & ETHERCAP_VLAN_HWFILTER) == 0)
		return;
	/*
	 * A soft reset zero's out the VFTA, so
	 * we need to repopulate it now.
	 */
	for (i = 0; i < IXGBE_VFTA_SIZE; i++)
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
} /* ixgbe_setup_vlan_hw_support */

/************************************************************************
 * ixgbe_get_slot_info
 *
 *   Get the width and transaction speed of
 *   the slot this adapter is plugged into.
 ************************************************************************/
static void
ixgbe_get_slot_info(struct adapter *adapter)
{
	device_t		dev = adapter->dev;
	struct ixgbe_hw		*hw = &adapter->hw;
	u32                   offset;
//	struct ixgbe_mac_info	*mac = &hw->mac;
	u16			link;
	int                   bus_info_valid = TRUE;

	/* Some devices are behind an internal bridge */
	switch (hw->device_id) {
	case IXGBE_DEV_ID_82599_SFP_SF_QP:
	case IXGBE_DEV_ID_82599_QSFP_SF_QP:
		goto get_parent_info;
	default:
		break;
	}

	ixgbe_get_bus_info(hw);

	/*
	 * Some devices don't use PCI-E, but there is no need
	 * to display "Unknown" for bus speed and width.
	 */
	switch (hw->mac.type) {
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
		return;
	default:
		goto display;
	}

get_parent_info:
	/*
	 * For the Quad port adapter we need to parse back
	 * up the PCI tree to find the speed of the expansion
	 * slot into which this adapter is plugged. A bit more work.
	 */
	dev = device_parent(device_parent(dev));
#if 0
#ifdef IXGBE_DEBUG
	device_printf(dev, "parent pcib = %x,%x,%x\n", pci_get_bus(dev),
	    pci_get_slot(dev), pci_get_function(dev));
#endif
	dev = device_parent(device_parent(dev));
#ifdef IXGBE_DEBUG
	device_printf(dev, "slot pcib = %x,%x,%x\n", pci_get_bus(dev),
	    pci_get_slot(dev), pci_get_function(dev));
#endif
#endif
	/* Now get the PCI Express Capabilities offset */
	if (pci_get_capability(adapter->osdep.pc, adapter->osdep.tag,
	    PCI_CAP_PCIEXPRESS, &offset, NULL)) {
		/*
		 * Hmm...can't get PCI-Express capabilities.
		 * Falling back to default method.
		 */
		bus_info_valid = FALSE;
		ixgbe_get_bus_info(hw);
		goto display;
	}
	/* ...and read the Link Status Register */
	link = pci_conf_read(adapter->osdep.pc, adapter->osdep.tag,
	    offset + PCIE_LCSR) >> 16;
	ixgbe_set_pci_config_data_generic(hw, link);

display:
	device_printf(dev, "PCI Express Bus: Speed %s Width %s\n",
	    ((hw->bus.speed == ixgbe_bus_speed_8000)    ? "8.0GT/s" :
	     (hw->bus.speed == ixgbe_bus_speed_5000)    ? "5.0GT/s" :
	     (hw->bus.speed == ixgbe_bus_speed_2500)    ? "2.5GT/s" :
	     "Unknown"),
	    ((hw->bus.width == ixgbe_bus_width_pcie_x8) ? "x8" :
	     (hw->bus.width == ixgbe_bus_width_pcie_x4) ? "x4" :
	     (hw->bus.width == ixgbe_bus_width_pcie_x1) ? "x1" :
	     "Unknown"));

	if (bus_info_valid) {
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
	} else
		device_printf(dev, "Unable to determine slot speed/width. The speed/width reported are that of the internal switch.\n");

	return;
} /* ixgbe_get_slot_info */

/************************************************************************
 * ixgbe_enable_queue - MSI-X Interrupt Handlers and Tasklets
 ************************************************************************/
static inline void
ixgbe_enable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = &adapter->queues[vector];
	u64             queue = (u64)(1ULL << vector);
	u32             mask;

	mutex_enter(&que->im_mtx);
	if (que->im_nest > 0 && --que->im_nest > 0)
		goto out;

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
out:
	mutex_exit(&que->im_mtx);
} /* ixgbe_enable_queue */

/************************************************************************
 * ixgbe_disable_queue
 ************************************************************************/
static inline void
ixgbe_disable_queue(struct adapter *adapter, u32 vector)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ix_queue *que = &adapter->queues[vector];
	u64             queue = (u64)(1ULL << vector);
	u32             mask;

	mutex_enter(&que->im_mtx);
	if (que->im_nest++ > 0)
		goto  out;

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
out:
	mutex_exit(&que->im_mtx);
} /* ixgbe_disable_queue */

/************************************************************************
 * ixgbe_msix_que - MSI-X Queue Interrupt Service routine
 ************************************************************************/
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
		ixgbe_eitr_write(que, que->eitr_setting);

	que->eitr_setting = 0;

	/* Idle, do nothing */
        if ((txr->bytes == 0) && (rxr->bytes == 0))
                goto no_calc;
                                
	if ((txr->bytes) && (txr->packets))
		newitr = txr->bytes/txr->packets;
	if ((rxr->bytes) && (rxr->packets))
		newitr = max(newitr, (rxr->bytes / rxr->packets));
	newitr += 24; /* account for hardware frame, crc */

	/* set an upper boundary */
	newitr = min(newitr, 3000);

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
	if (more) {
		que->req.ev_count++;
		if (adapter->txrx_use_workqueue) {
			/*
			 * adapter->que_wq is bound to each CPU instead of
			 * each NIC queue to reduce workqueue kthread. As we
			 * should consider about interrupt affinity in this
			 * function, the workqueue kthread must be WQ_PERCPU.
			 * If create WQ_PERCPU workqueue kthread for each NIC
			 * queue, that number of created workqueue kthread is
			 * (number of used NIC queue) * (number of CPUs) =
			 * (number of CPUs) ^ 2 most often.
			 *
			 * The same NIC queue's interrupts are avoided by
			 * masking the queue's interrupt. And different
			 * NIC queue's interrupts use different struct work
			 * (que->wq_cookie). So, "enqueued flag" to avoid
			 * twice workqueue_enqueue() is not required .
			 */
			workqueue_enqueue(adapter->que_wq, &que->wq_cookie,
			    curcpu());
		} else {
			softint_schedule(que->que_si);
		}
	} else
		ixgbe_enable_queue(adapter, que->msix);

	return 1;
} /* ixgbe_msix_que */

/************************************************************************
 * ixgbe_media_status - Media Ioctl callback
 *
 *   Called whenever the user queries the status of
 *   the interface using ifconfig.
 ************************************************************************/
static void
ixgbe_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
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
		ifmr->ifm_active |= IFM_NONE;
		IXGBE_CORE_UNLOCK(adapter);
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;
	layer = adapter->phy_layer;

	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_5GBASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_2500BASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_1000BASE_T ||
	    layer & IXGBE_PHYSICAL_LAYER_100BASE_TX ||
	    layer & IXGBE_PHYSICAL_LAYER_10BASE_T)
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
	 * XXX: These need to use the proper media types once
	 * they're added.
	 */
	if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KR)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
#ifndef IFM_ETH_XTYPE
			ifmr->ifm_active |= IFM_10G_SR | IFM_FDX;
#else
			ifmr->ifm_active |= IFM_10G_KR | IFM_FDX;
#endif
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_KX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_KX | IFM_FDX;
			break;
		}
	else if (layer & IXGBE_PHYSICAL_LAYER_10GBASE_KX4 ||
	    layer & IXGBE_PHYSICAL_LAYER_2500BASE_KX ||
	    layer & IXGBE_PHYSICAL_LAYER_1000BASE_KX)
		switch (adapter->link_speed) {
		case IXGBE_LINK_SPEED_10GB_FULL:
#ifndef IFM_ETH_XTYPE
			ifmr->ifm_active |= IFM_10G_CX4 | IFM_FDX;
#else
			ifmr->ifm_active |= IFM_10G_KX4 | IFM_FDX;
#endif
			break;
		case IXGBE_LINK_SPEED_2_5GB_FULL:
			ifmr->ifm_active |= IFM_2500_KX | IFM_FDX;
			break;
		case IXGBE_LINK_SPEED_1GB_FULL:
			ifmr->ifm_active |= IFM_1000_KX | IFM_FDX;
			break;
		}

	/* If nothing is recognized... */
#if 0
	if (IFM_SUBTYPE(ifmr->ifm_active) == 0)
		ifmr->ifm_active |= IFM_UNKNOWN;
#endif

	ifp->if_baudrate = ifmedia_baudrate(ifmr->ifm_active);

	/* Display current flow control setting used on link */
	if (hw->fc.current_mode == ixgbe_fc_rx_pause ||
	    hw->fc.current_mode == ixgbe_fc_full)
		ifmr->ifm_active |= IFM_ETH_RXPAUSE;
	if (hw->fc.current_mode == ixgbe_fc_tx_pause ||
	    hw->fc.current_mode == ixgbe_fc_full)
		ifmr->ifm_active |= IFM_ETH_TXPAUSE;

	IXGBE_CORE_UNLOCK(adapter);

	return;
} /* ixgbe_media_status */

/************************************************************************
 * ixgbe_media_change - Media Ioctl callback
 *
 *   Called when the user changes speed/duplex using
 *   media/mediopt option with ifconfig.
 ************************************************************************/
static int
ixgbe_media_change(struct ifnet *ifp)
{
	struct adapter   *adapter = ifp->if_softc;
	struct ifmedia   *ifm = &adapter->media;
	struct ixgbe_hw  *hw = &adapter->hw;
	ixgbe_link_speed speed = 0;
	ixgbe_link_speed link_caps = 0;
	bool negotiate = false;
	s32 err = IXGBE_NOT_IMPLEMENTED;

	INIT_DEBUGOUT("ixgbe_media_change: begin");

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	if (hw->phy.media_type == ixgbe_media_type_backplane)
		return (ENODEV);

	/*
	 * We don't actually need to check against the supported
	 * media types of the adapter; ifmedia will take care of
	 * that for us.
	 */
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		err = hw->mac.ops.get_link_capabilities(hw, &link_caps,
		    &negotiate);
		if (err != IXGBE_SUCCESS) {
			device_printf(adapter->dev, "Unable to determine "
			    "supported advertise speeds\n");
			return (ENODEV);
		}
		speed |= link_caps;
		break;
	case IFM_10G_T:
	case IFM_10G_LRM:
	case IFM_10G_LR:
	case IFM_10G_TWINAX:
#ifndef IFM_ETH_XTYPE
	case IFM_10G_SR: /* KR, too */
	case IFM_10G_CX4: /* KX4 */
#else
	case IFM_10G_KR:
	case IFM_10G_KX4:
#endif
		speed |= IXGBE_LINK_SPEED_10GB_FULL;
		break;
	case IFM_5000_T:
		speed |= IXGBE_LINK_SPEED_5GB_FULL;
		break;
	case IFM_2500_T:
	case IFM_2500_KX:
		speed |= IXGBE_LINK_SPEED_2_5GB_FULL;
		break;
	case IFM_1000_T:
	case IFM_1000_LX:
	case IFM_1000_SX:
	case IFM_1000_KX:
		speed |= IXGBE_LINK_SPEED_1GB_FULL;
		break;
	case IFM_100_TX:
		speed |= IXGBE_LINK_SPEED_100_FULL;
		break;
	case IFM_10_T:
		speed |= IXGBE_LINK_SPEED_10_FULL;
		break;
	default:
		goto invalid;
	}

	hw->mac.autotry_restart = TRUE;
	hw->mac.ops.setup_link(hw, speed, TRUE);
	adapter->advertise = 0;
	if (IFM_SUBTYPE(ifm->ifm_media) != IFM_AUTO) {
		if ((speed & IXGBE_LINK_SPEED_10GB_FULL) != 0)
			adapter->advertise |= 1 << 2;
		if ((speed & IXGBE_LINK_SPEED_1GB_FULL) != 0)
			adapter->advertise |= 1 << 1;
		if ((speed & IXGBE_LINK_SPEED_100_FULL) != 0)
			adapter->advertise |= 1 << 0;
		if ((speed & IXGBE_LINK_SPEED_10_FULL) != 0)
			adapter->advertise |= 1 << 3;
		if ((speed & IXGBE_LINK_SPEED_2_5GB_FULL) != 0)
			adapter->advertise |= 1 << 4;
		if ((speed & IXGBE_LINK_SPEED_5GB_FULL) != 0)
			adapter->advertise |= 1 << 5;
	}

	return (0);

invalid:
	device_printf(adapter->dev, "Invalid media type!\n");

	return (EINVAL);
} /* ixgbe_media_change */

/************************************************************************
 * ixgbe_set_promisc
 ************************************************************************/
static void
ixgbe_set_promisc(struct adapter *adapter)
{
	struct ifnet *ifp = adapter->ifp;
	int          mcnt = 0;
	u32          rctl;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ethercom *ec = &adapter->osdep.ec;

	KASSERT(mutex_owned(&adapter->core_mtx));
	rctl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	rctl &= (~IXGBE_FCTRL_UPE);
	if (ifp->if_flags & IFF_ALLMULTI)
		mcnt = MAX_NUM_MULTICAST_ADDRESSES;
	else {
		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (mcnt == MAX_NUM_MULTICAST_ADDRESSES)
				break;
			mcnt++;
			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
	}
	if (mcnt < MAX_NUM_MULTICAST_ADDRESSES)
		rctl &= (~IXGBE_FCTRL_MPE);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, rctl);

	if (ifp->if_flags & IFF_PROMISC) {
		rctl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, rctl);
	} else if (ifp->if_flags & IFF_ALLMULTI) {
		rctl |= IXGBE_FCTRL_MPE;
		rctl &= ~IXGBE_FCTRL_UPE;
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, rctl);
	}
} /* ixgbe_set_promisc */

/************************************************************************
 * ixgbe_msix_link - Link status change ISR (MSI/MSI-X)
 ************************************************************************/
static int
ixgbe_msix_link(void *arg)
{
	struct adapter	*adapter = arg;
	struct ixgbe_hw *hw = &adapter->hw;
	u32		eicr, eicr_mask;
	s32             retval;

	++adapter->link_irq.ev_count;

	/* Pause other interrupts */
	IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_OTHER);

	/* First get the cause */
	/*
	 * The specifications of 82598, 82599, X540 and X550 say EICS register
	 * is write only. However, Linux says it is a workaround for silicon
	 * errata to read EICS instead of EICR to get interrupt cause. It seems
	 * there is a problem about read clear mechanism for EICR register.
	 */
	eicr = IXGBE_READ_REG(hw, IXGBE_EICS);
	/* Be sure the queue bits are not cleared */
	eicr &= ~IXGBE_EICR_RTX_QUEUE;
	/* Clear interrupt with write */
	IXGBE_WRITE_REG(hw, IXGBE_EICR, eicr);

	/* Link status change */
	if (eicr & IXGBE_EICR_LSC) {
		IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_LSC);
		softint_schedule(adapter->link_si);
	}

	if (adapter->hw.mac.type != ixgbe_mac_82598EB) {
		if ((adapter->feat_en & IXGBE_FEATURE_FDIR) &&
		    (eicr & IXGBE_EICR_FLOW_DIR)) {
			/* This is probably overkill :) */
			if (!atomic_cas_uint(&adapter->fdir_reinit, 0, 1))
				return 1;
			/* Disable the interrupt */
			IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_FLOW_DIR);
			softint_schedule(adapter->fdir_si);
		}

		if (eicr & IXGBE_EICR_ECC) {
			device_printf(adapter->dev,
			    "CRITICAL: ECC ERROR!! Please Reboot!!\n");
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_ECC);
		}

		/* Check for over temp condition */
		if (adapter->feat_en & IXGBE_FEATURE_TEMP_SENSOR) {
			switch (adapter->hw.mac.type) {
			case ixgbe_mac_X550EM_a:
				if (!(eicr & IXGBE_EICR_GPI_SDP0_X550EM_a))
					break;
				IXGBE_WRITE_REG(hw, IXGBE_EIMC,
				    IXGBE_EICR_GPI_SDP0_X550EM_a);
				IXGBE_WRITE_REG(hw, IXGBE_EICR,
				    IXGBE_EICR_GPI_SDP0_X550EM_a);
				retval = hw->phy.ops.check_overtemp(hw);
				if (retval != IXGBE_ERR_OVERTEMP)
					break;
				device_printf(adapter->dev, "CRITICAL: OVER TEMP!! PHY IS SHUT DOWN!!\n");
				device_printf(adapter->dev, "System shutdown required!\n");
				break;
			default:
				if (!(eicr & IXGBE_EICR_TS))
					break;
				retval = hw->phy.ops.check_overtemp(hw);
				if (retval != IXGBE_ERR_OVERTEMP)
					break;
				device_printf(adapter->dev, "CRITICAL: OVER TEMP!! PHY IS SHUT DOWN!!\n");
				device_printf(adapter->dev, "System shutdown required!\n");
				IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_TS);
				break;
			}
		}

		/* Check for VF message */
		if ((adapter->feat_en & IXGBE_FEATURE_SRIOV) &&
		    (eicr & IXGBE_EICR_MAILBOX))
			softint_schedule(adapter->mbx_si);
	}

	if (ixgbe_is_sfp(hw)) {
		/* Pluggable optics-related interrupt */
		if (hw->mac.type >= ixgbe_mac_X540)
			eicr_mask = IXGBE_EICR_GPI_SDP0_X540;
		else
			eicr_mask = IXGBE_EICR_GPI_SDP2_BY_MAC(hw);

		if (eicr & eicr_mask) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR, eicr_mask);
			softint_schedule(adapter->mod_si);
		}

		if ((hw->mac.type == ixgbe_mac_82599EB) &&
		    (eicr & IXGBE_EICR_GPI_SDP1_BY_MAC(hw))) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR,
			    IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
			softint_schedule(adapter->msf_si);
		}
	}

	/* Check for fan failure */
	if (adapter->feat_en & IXGBE_FEATURE_FAN_FAIL) {
		ixgbe_check_fan_failure(adapter, eicr, TRUE);
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
	}

	/* External PHY interrupt */
	if ((hw->phy.type == ixgbe_phy_x550em_ext_t) &&
	    (eicr & IXGBE_EICR_GPI_SDP0_X540)) {
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP0_X540);
		softint_schedule(adapter->phy_si);
 	}

	/* Re-enable other interrupts */
	IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMS_OTHER);
	return 1;
} /* ixgbe_msix_link */

static void
ixgbe_eitr_write(struct ix_queue *que, uint32_t itr)
{
	struct adapter *adapter = que->adapter;
	
        if (adapter->hw.mac.type == ixgbe_mac_82598EB)
                itr |= itr << 16;
        else
                itr |= IXGBE_EITR_CNT_WDIS;

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EITR(que->msix),
	    itr);
}


/************************************************************************
 * ixgbe_sysctl_interrupt_rate_handler
 ************************************************************************/
static int
ixgbe_sysctl_interrupt_rate_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ix_queue *que = (struct ix_queue *)node.sysctl_data;
	struct adapter  *adapter = que->adapter;
	uint32_t reg, usec, rate;
	int error;

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
	if (error || newp == NULL)
		return error;
	reg &= ~0xfff; /* default, no limitation */
	if (rate > 0 && rate < 500000) {
		if (rate < 1000)
			rate = 1000;
		reg |= ((4000000/rate) & 0xff8);
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
		ixgbe_max_interrupt_rate = rate;
	} else
		ixgbe_max_interrupt_rate = 0;
	ixgbe_eitr_write(que, reg);

	return (0);
} /* ixgbe_sysctl_interrupt_rate_handler */

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

/************************************************************************
 * ixgbe_add_device_sysctls
 ************************************************************************/
static void
ixgbe_add_device_sysctls(struct adapter *adapter)
{
	device_t               dev = adapter->dev;
	struct ixgbe_hw        *hw = &adapter->hw;
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
	if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
	    CTLTYPE_INT, "fc", SYSCTL_DESCR(IXGBE_SYSCTL_DESC_SET_FC),
	    ixgbe_sysctl_flowcntl, 0, (void *)adapter, 0, CTL_CREATE,
	    CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	adapter->enable_aim = ixgbe_enable_aim;
	if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "enable_aim", SYSCTL_DESCR("Interrupt Moderation"),
	    NULL, 0, &adapter->enable_aim, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "advertise_speed", SYSCTL_DESCR(IXGBE_SYSCTL_DESC_ADV_SPEED),
	    ixgbe_sysctl_advertise, 0, (void *)adapter, 0, CTL_CREATE,
	    CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	adapter->txrx_use_workqueue = ixgbe_txrx_workqueue;
	if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "txrx_workqueue", SYSCTL_DESCR("Use workqueue for packet processing"),
	    NULL, 0, &adapter->txrx_use_workqueue, 0, CTL_CREATE, CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

#ifdef IXGBE_DEBUG
	/* testing sysctls (for all devices) */
	if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
	    CTLTYPE_INT, "power_state", SYSCTL_DESCR("PCI Power State"),
	    ixgbe_sysctl_power_state, 0, (void *)adapter, 0, CTL_CREATE,
	    CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");

	if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READONLY,
	    CTLTYPE_STRING, "print_rss_config",
	    SYSCTL_DESCR("Prints RSS Configuration"),
	    ixgbe_sysctl_print_rss_config, 0, (void *)adapter, 0, CTL_CREATE,
	    CTL_EOL) != 0)
		aprint_error_dev(dev, "could not create sysctl\n");
#endif
	/* for X550 series devices */
	if (hw->mac.type >= ixgbe_mac_X550)
		if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
		    CTLTYPE_INT, "dmac", SYSCTL_DESCR("DMA Coalesce"),
		    ixgbe_sysctl_dmac, 0, (void *)adapter, 0, CTL_CREATE,
		    CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

	/* for WoL-capable devices */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T) {
		if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
		    CTLTYPE_BOOL, "wol_enable",
		    SYSCTL_DESCR("Enable/Disable Wake on LAN"),
		    ixgbe_sysctl_wol_enable, 0, (void *)adapter, 0, CTL_CREATE,
		    CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
		    CTLTYPE_INT, "wufc",
		    SYSCTL_DESCR("Enable/Disable Wake Up Filters"),
		    ixgbe_sysctl_wufc, 0, (void *)adapter, 0, CTL_CREATE,
		    CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}

	/* for X552/X557-AT devices */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T) {
		const struct sysctlnode *phy_node;

		if (sysctl_createv(log, 0, &rnode, &phy_node, 0, CTLTYPE_NODE,
		    "phy", SYSCTL_DESCR("External PHY sysctls"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0) {
			aprint_error_dev(dev, "could not create sysctl\n");
			return;
		}

		if (sysctl_createv(log, 0, &phy_node, &cnode, CTLFLAG_READONLY,
		    CTLTYPE_INT, "temp",
		    SYSCTL_DESCR("Current External PHY Temperature (Celsius)"),
		    ixgbe_sysctl_phy_temp, 0, (void *)adapter, 0, CTL_CREATE,
		    CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");

		if (sysctl_createv(log, 0, &phy_node, &cnode, CTLFLAG_READONLY,
		    CTLTYPE_INT, "overtemp_occurred",
		    SYSCTL_DESCR("External PHY High Temperature Event Occurred"),
		    ixgbe_sysctl_phy_overtemp_occurred, 0, (void *)adapter, 0,
		    CTL_CREATE, CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}

	if (adapter->feat_cap & IXGBE_FEATURE_EEE) {
		if (sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_READWRITE,
		    CTLTYPE_INT, "eee_state",
		    SYSCTL_DESCR("EEE Power Save State"),
		    ixgbe_sysctl_eee_state, 0, (void *)adapter, 0, CTL_CREATE,
		    CTL_EOL) != 0)
			aprint_error_dev(dev, "could not create sysctl\n");
	}
} /* ixgbe_add_device_sysctls */

/************************************************************************
 * ixgbe_allocate_pci_resources
 ************************************************************************/
static int
ixgbe_allocate_pci_resources(struct adapter *adapter,
    const struct pci_attach_args *pa)
{
	pcireg_t	memtype;
	device_t dev = adapter->dev;
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

	return (0);
} /* ixgbe_allocate_pci_resources */

static void
ixgbe_free_softint(struct adapter *adapter)
{
	struct ix_queue *que = adapter->queues;
	struct tx_ring *txr = adapter->tx_rings;
	int i;

	for (i = 0; i < adapter->num_queues; i++, que++, txr++) {
		if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX)) {
			if (txr->txr_si != NULL)
				softint_disestablish(txr->txr_si);
		}
		if (que->que_si != NULL)
			softint_disestablish(que->que_si);
	}
	if (adapter->txr_wq != NULL)
		workqueue_destroy(adapter->txr_wq);
	if (adapter->txr_wq_enqueued != NULL)
		percpu_free(adapter->txr_wq_enqueued, sizeof(u_int));
	if (adapter->que_wq != NULL)
		workqueue_destroy(adapter->que_wq);

	/* Drain the Link queue */
	if (adapter->link_si != NULL) {
		softint_disestablish(adapter->link_si);
		adapter->link_si = NULL;
	}
	if (adapter->mod_si != NULL) {
		softint_disestablish(adapter->mod_si);
		adapter->mod_si = NULL;
	}
	if (adapter->msf_si != NULL) {
		softint_disestablish(adapter->msf_si);
		adapter->msf_si = NULL;
	}
	if (adapter->phy_si != NULL) {
		softint_disestablish(adapter->phy_si);
		adapter->phy_si = NULL;
	}
	if (adapter->feat_en & IXGBE_FEATURE_FDIR) {
		if (adapter->fdir_si != NULL) {
			softint_disestablish(adapter->fdir_si);
			adapter->fdir_si = NULL;
		}
	}
	if (adapter->feat_cap & IXGBE_FEATURE_SRIOV) {
		if (adapter->mbx_si != NULL) {
			softint_disestablish(adapter->mbx_si);
			adapter->mbx_si = NULL;
		}
	}
} /* ixgbe_free_softint */

/************************************************************************
 * ixgbe_detach - Device removal routine
 *
 *   Called when the driver is being removed.
 *   Stops the adapter and deallocates all the resources
 *   that were allocated for driver operation.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static int
ixgbe_detach(device_t dev, int flags)
{
	struct adapter *adapter = device_private(dev);
	struct rx_ring *rxr = adapter->rx_rings;
	struct tx_ring *txr = adapter->tx_rings;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_hw_stats *stats = &adapter->stats.pf;
	u32	ctrl_ext;

	INIT_DEBUGOUT("ixgbe_detach: begin");
	if (adapter->osdep.attached == false)
		return 0;

	if (ixgbe_pci_iov_detach(dev) != 0) {
		device_printf(dev, "SR-IOV in use; detach first.\n");
		return (EBUSY);
	}

	/* Stop the interface. Callouts are stopped in it. */
	ixgbe_ifstop(adapter->ifp, 1);
#if NVLAN > 0
	/* Make sure VLANs are not using driver */
	if (!VLAN_ATTACHED(&adapter->osdep.ec))
		;	/* nothing to do: no VLANs */ 
	else if ((flags & (DETACH_SHUTDOWN|DETACH_FORCE)) != 0)
		vlan_ifdetach(adapter->ifp);
	else {
		aprint_error_dev(dev, "VLANs in use, detach first\n");
		return (EBUSY);
	}
#endif

	pmf_device_deregister(dev);

	ether_ifdetach(adapter->ifp);
	/* Stop the adapter */
	IXGBE_CORE_LOCK(adapter);
	ixgbe_setup_low_power_mode(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	ixgbe_free_softint(adapter);
	
	/* let hardware know driver is unloading */
	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	ctrl_ext &= ~IXGBE_CTRL_EXT_DRV_LOAD;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT, ctrl_ext);

	callout_halt(&adapter->timer, NULL);

	if (adapter->feat_en & IXGBE_FEATURE_NETMAP)
		netmap_detach(adapter->ifp);

	ixgbe_free_pci_resources(adapter);
#if 0	/* XXX the NetBSD port is probably missing something here */
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

		if (i < __arraycount(stats->mpc)) {
			evcnt_detach(&stats->mpc[i]);
			if (hw->mac.type == ixgbe_mac_82598EB)
				evcnt_detach(&stats->rnbc[i]);
		}
		if (i < __arraycount(stats->pxontxc)) {
			evcnt_detach(&stats->pxontxc[i]);
			evcnt_detach(&stats->pxonrxc[i]);
			evcnt_detach(&stats->pxofftxc[i]);
			evcnt_detach(&stats->pxoffrxc[i]);
			evcnt_detach(&stats->pxon2offc[i]);
		}
		if (i < __arraycount(stats->qprc)) {
			evcnt_detach(&stats->qprc[i]);
			evcnt_detach(&stats->qptc[i]);
			evcnt_detach(&stats->qbrc[i]);
			evcnt_detach(&stats->qbtc[i]);
			evcnt_detach(&stats->qprdc[i]);
		}

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
	evcnt_detach(&stats->intzero);
	evcnt_detach(&stats->legint);
	evcnt_detach(&stats->crcerrs);
	evcnt_detach(&stats->illerrc);
	evcnt_detach(&stats->errbc);
	evcnt_detach(&stats->mspdc);
	if (hw->mac.type >= ixgbe_mac_X550)
		evcnt_detach(&stats->mbsdc);
	evcnt_detach(&stats->mpctotal);
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
	evcnt_detach(&stats->mngpdc);
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
	for (int i = 0; i < adapter->num_queues; i++) {
		struct ix_queue * que = &adapter->queues[i];
		mutex_destroy(&que->im_mtx);
	}
	free(adapter->queues, M_DEVBUF);
	free(adapter->mta, M_DEVBUF);

	IXGBE_CORE_LOCK_DESTROY(adapter);

	return (0);
} /* ixgbe_detach */

/************************************************************************
 * ixgbe_setup_low_power_mode - LPLU/WoL preparation
 *
 *   Prepare the adapter/port for LPLU and/or WoL
 ************************************************************************/
static int
ixgbe_setup_low_power_mode(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	device_t        dev = adapter->dev;
	s32             error = 0;

	KASSERT(mutex_owned(&adapter->core_mtx));

	/* Limit power management flow to X550EM baseT */
	if (hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T &&
	    hw->phy.ops.enter_lplu) {
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
} /* ixgbe_setup_low_power_mode */

/************************************************************************
 * ixgbe_shutdown - Shutdown entry point
 ************************************************************************/
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
} /* ixgbe_shutdown */
#endif

/************************************************************************
 * ixgbe_suspend
 *
 *   From D0 to D3
 ************************************************************************/
static bool
ixgbe_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct adapter *adapter = device_private(dev);
	int            error = 0;

	INIT_DEBUGOUT("ixgbe_suspend: begin");

	IXGBE_CORE_LOCK(adapter);

	error = ixgbe_setup_low_power_mode(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return (error);
} /* ixgbe_suspend */

/************************************************************************
 * ixgbe_resume
 *
 *   From D3 to D0
 ************************************************************************/
static bool
ixgbe_resume(device_t dev, const pmf_qual_t *qual)
{
	struct adapter  *adapter = device_private(dev);
	struct ifnet    *ifp = adapter->ifp;
	struct ixgbe_hw *hw = &adapter->hw;
	u32             wus;

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
} /* ixgbe_resume */

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

/************************************************************************
 * ixgbe_init_locked - Init entry point
 *
 *   Used in two ways: It is used by the stack as an init
 *   entry point in network interface structure. It is also
 *   used by the driver as a hw/sw initialization routine to
 *   get to a consistent state.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
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
	u32             ctrl_ext;
	int             err = 0;

	/* XXX check IFF_UP and IFF_RUNNING, power-saving state! */

	KASSERT(mutex_owned(&adapter->core_mtx));
	INIT_DEBUGOUT("ixgbe_init_locked: begin");

	hw->adapter_stopped = FALSE;
	ixgbe_stop_adapter(hw);
        callout_stop(&adapter->timer);

	/* XXX I moved this here from the SIOCSIFMTU case in ixgbe_ioctl(). */
	adapter->max_frame_size =
		ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;

	/* Queue indices may change with IOV mode */
	ixgbe_align_all_queue_indices(adapter);

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
	ixgbe_initialize_iov(adapter);
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

	/* Enable SDP & MSI-X interrupts based on adapter */
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
			 * PTHRESH = 21
			 * HTHRESH = 4
			 * WTHRESH = 8
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
#ifdef DEV_NETMAP
		if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) &&
		    (ifp->if_capenable & IFCAP_NETMAP)) {
			struct netmap_adapter *na = NA(adapter->ifp);
			struct netmap_kring *kring = &na->rx_rings[i];
			int t = na->num_rx_desc - 1 - nm_kr_rxspace(kring);

			IXGBE_WRITE_REG(hw, IXGBE_RDT(rxr->me), t);
		} else
#endif /* DEV_NETMAP */
			IXGBE_WRITE_REG(hw, IXGBE_RDT(rxr->me),
			    adapter->num_rx_desc - 1);
	}

	/* Enable Receive engine */
	rxctrl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	if (hw->mac.type == ixgbe_mac_82598EB)
		rxctrl |= IXGBE_RXCTRL_DMBYPS;
	rxctrl |= IXGBE_RXCTRL_RXEN;
	ixgbe_enable_rx_dma(hw, rxctrl);

	callout_reset(&adapter->timer, hz, ixgbe_local_timer, adapter);

	/* Set up MSI-X routing */
	if (adapter->feat_en & IXGBE_FEATURE_MSIX) {
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

	ixgbe_init_fdir(adapter);

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

	/* Enable the use of the MBX by the VF's */
	if (adapter->feat_en & IXGBE_FEATURE_SRIOV) {
		ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
		ctrl_ext |= IXGBE_CTRL_EXT_PFRSTD;
		IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);
	}

	/* Update saved flags. See ixgbe_ifflags_cb() */
	adapter->if_flags = ifp->if_flags;

	/* Now inform the stack we're ready */
	ifp->if_flags |= IFF_RUNNING;

	return;
} /* ixgbe_init_locked */

/************************************************************************
 * ixgbe_init
 ************************************************************************/
static int
ixgbe_init(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_init_locked(adapter);
	IXGBE_CORE_UNLOCK(adapter);

	return 0;	/* XXX ixgbe_init_locked cannot fail?  really? */
} /* ixgbe_init */

/************************************************************************
 * ixgbe_set_ivar
 *
 *   Setup the correct IVAR register for a particular MSI-X interrupt
 *     (yes this is all very magic and confusing :)
 *    - entry is the register array entry
 *    - vector is the MSI-X vector for this queue
 *    - type is RX/TX/MISC
 ************************************************************************/
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
	case ixgbe_mac_X550EM_a:
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
} /* ixgbe_set_ivar */

/************************************************************************
 * ixgbe_configure_ivars
 ************************************************************************/
static void
ixgbe_configure_ivars(struct adapter *adapter)
{
	struct ix_queue *que = adapter->queues;
	u32             newitr;

	if (ixgbe_max_interrupt_rate > 0)
		newitr = (4000000 / ixgbe_max_interrupt_rate) & 0x0FF8;
	else {
		/*
		 * Disable DMA coalescing if interrupt moderation is
		 * disabled.
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
		ixgbe_eitr_write(que, newitr);
	}

	/* For the Link interrupt */
        ixgbe_set_ivar(adapter, 1, adapter->vector, -1);
} /* ixgbe_configure_ivars */

/************************************************************************
 * ixgbe_config_gpie
 ************************************************************************/
static void
ixgbe_config_gpie(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32             gpie;

	gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);

	if (adapter->feat_en & IXGBE_FEATURE_MSIX) {
		/* Enable Enhanced MSI-X mode */
		gpie |= IXGBE_GPIE_MSIX_MODE
		     |  IXGBE_GPIE_EIAME
		     |  IXGBE_GPIE_PBA_SUPPORT
		     |  IXGBE_GPIE_OCD;
	}

	/* Fan Failure Interrupt */
	if (adapter->feat_en & IXGBE_FEATURE_FAN_FAIL)
		gpie |= IXGBE_SDP1_GPIEN;

	/* Thermal Sensor Interrupt */
	if (adapter->feat_en & IXGBE_FEATURE_TEMP_SENSOR)
		gpie |= IXGBE_SDP0_GPIEN_X540;

	/* Link detection */
	switch (hw->mac.type) {
	case ixgbe_mac_82599EB:
		gpie |= IXGBE_SDP1_GPIEN | IXGBE_SDP2_GPIEN;
		break;
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
		gpie |= IXGBE_SDP0_GPIEN_X540;
		break;
	default:
		break;
	}

	IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);

	return;
} /* ixgbe_config_gpie */

/************************************************************************
 * ixgbe_config_delay_values
 *
 *   Requires adapter->max_frame_size to be set.
 ************************************************************************/
static void
ixgbe_config_delay_values(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32             rxpb, frame, size, tmp;

	frame = adapter->max_frame_size;

	/* Calculate High Water */
	switch (hw->mac.type) {
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
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
	case ixgbe_mac_X550EM_a:
		tmp = IXGBE_LOW_DV_X540(frame);
		break;
	default:
		tmp = IXGBE_LOW_DV(frame);
		break;
	}
	hw->fc.low_water[0] = IXGBE_BT2KB(tmp);

	hw->fc.pause_time = IXGBE_FC_PAUSE;
	hw->fc.send_xon = TRUE;
} /* ixgbe_config_delay_values */

/************************************************************************
 * ixgbe_set_multi - Multicast Update
 *
 *   Called whenever multicast address list is updated.
 ************************************************************************/
static void
ixgbe_set_multi(struct adapter *adapter)
{
	struct ixgbe_mc_addr	*mta;
	struct ifnet		*ifp = adapter->ifp;
	u8			*update_ptr;
	int			mcnt = 0;
	u32			fctrl;
	struct ethercom		*ec = &adapter->osdep.ec;
	struct ether_multi	*enm;
	struct ether_multistep	step;

	KASSERT(mutex_owned(&adapter->core_mtx));
	IOCTL_DEBUGOUT("ixgbe_set_multi: begin");

	mta = adapter->mta;
	bzero(mta, sizeof(*mta) * MAX_NUM_MULTICAST_ADDRESSES);

	ifp->if_flags &= ~IFF_ALLMULTI;
	ETHER_LOCK(ec);
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
	ETHER_UNLOCK(ec);

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
		ixgbe_update_mc_addr_list(&adapter->hw, update_ptr, mcnt,
		    ixgbe_mc_array_itr, TRUE);
	}

	return;
} /* ixgbe_set_multi */

/************************************************************************
 * ixgbe_mc_array_itr
 *
 *   An iterator function needed by the multicast shared code.
 *   It feeds the shared code routine the addresses in the
 *   array of ixgbe_set_multi() one by one.
 ************************************************************************/
static u8 *
ixgbe_mc_array_itr(struct ixgbe_hw *hw, u8 **update_ptr, u32 *vmdq)
{
	struct ixgbe_mc_addr *mta;

	mta = (struct ixgbe_mc_addr *)*update_ptr;
	*vmdq = mta->vmdq;

	*update_ptr = (u8*)(mta + 1);

	return (mta->addr);
} /* ixgbe_mc_array_itr */

/************************************************************************
 * ixgbe_local_timer - Timer routine
 *
 *   Checks for link status, updates statistics,
 *   and runs the watchdog check.
 ************************************************************************/
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
	 * Check the TX queues status
	 *      - mark hung queues so we don't schedule on them
	 *      - watchdog only if all queues show hung
	 */
	for (int i = 0; i < adapter->num_queues; i++, que++) {
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
} /* ixgbe_local_timer */

/************************************************************************
 * ixgbe_sfp_probe
 *
 *   Determine if a port had optics inserted.
 ************************************************************************/
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
		adapter->sfp_probe = FALSE;
		if (ret == IXGBE_ERR_SFP_NOT_SUPPORTED) {
			device_printf(dev,"Unsupported SFP+ module detected!");
			device_printf(dev,
			    "Reload driver with supported module.\n");
                        goto out;
		} else
			device_printf(dev, "SFP+ module detected!\n");
		/* We now have supported optics */
		result = TRUE;
	}
out:

	return (result);
} /* ixgbe_sfp_probe */

/************************************************************************
 * ixgbe_handle_mod - Tasklet for SFP module interrupts
 ************************************************************************/
static void
ixgbe_handle_mod(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	device_t	dev = adapter->dev;
	u32             err, cage_full = 0;

	if (adapter->hw.need_crosstalk_fix) {
		switch (hw->mac.type) {
		case ixgbe_mac_82599EB:
			cage_full = IXGBE_READ_REG(hw, IXGBE_ESDP) &
			    IXGBE_ESDP_SDP2;
			break;
		case ixgbe_mac_X550EM_x:
		case ixgbe_mac_X550EM_a:
			cage_full = IXGBE_READ_REG(hw, IXGBE_ESDP) &
			    IXGBE_ESDP_SDP0;
			break;
		default:
			break;
		}

		if (!cage_full)
			return;
	}

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
} /* ixgbe_handle_mod */


/************************************************************************
 * ixgbe_handle_msf - Tasklet for MSF (multispeed fiber) interrupts
 ************************************************************************/
static void
ixgbe_handle_msf(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;
	u32             autoneg;
	bool            negotiate;

	/* get_supported_phy_layer will call hw->phy.ops.identify_sfp() */
	adapter->phy_layer = ixgbe_get_supported_physical_layer(hw);

	autoneg = hw->phy.autoneg_advertised;
	if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
		hw->mac.ops.get_link_capabilities(hw, &autoneg, &negotiate);
	else
		negotiate = 0;
	if (hw->mac.ops.setup_link)
		hw->mac.ops.setup_link(hw, autoneg, TRUE);

	/* Adjust media types shown in ifconfig */
	ifmedia_removeall(&adapter->media);
	ixgbe_add_media_types(adapter);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);
} /* ixgbe_handle_msf */

/************************************************************************
 * ixgbe_handle_phy - Tasklet for external PHY interrupts
 ************************************************************************/
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
		    "Error handling LASI interrupt: %d\n", error);
} /* ixgbe_handle_phy */

static void
ixgbe_ifstop(struct ifnet *ifp, int disable)
{
	struct adapter *adapter = ifp->if_softc;

	IXGBE_CORE_LOCK(adapter);
	ixgbe_stop(adapter);
	IXGBE_CORE_UNLOCK(adapter);
}

/************************************************************************
 * ixgbe_stop - Stop the hardware
 *
 *   Disables all traffic on the adapter by issuing a
 *   global reset on the MAC and deallocates TX/RX buffers.
 ************************************************************************/
static void
ixgbe_stop(void *arg)
{
	struct ifnet    *ifp;
	struct adapter  *adapter = arg;
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
} /* ixgbe_stop */

/************************************************************************
 * ixgbe_update_link_status - Update OS on link state
 *
 * Note: Only updates the OS on the cached link state.
 *       The real check of the hardware only happens with
 *       a link interrupt.
 ************************************************************************/
static void
ixgbe_update_link_status(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	device_t        dev = adapter->dev;
	struct ixgbe_hw *hw = &adapter->hw;

	if (adapter->link_up) {
		if (adapter->link_active == FALSE) {
			if (adapter->link_speed == IXGBE_LINK_SPEED_10GB_FULL){
				/*
				 *  Discard count for both MAC Local Fault and
				 * Remote Fault because those registers are
				 * valid only when the link speed is up and
				 * 10Gbps.
				 */
				IXGBE_READ_REG(hw, IXGBE_MLFC);
				IXGBE_READ_REG(hw, IXGBE_MRFC);
			}

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
			adapter->link_active = TRUE;
			/* Update any Flow Control changes */
			ixgbe_fc_enable(&adapter->hw);
			/* Update DMA coalescing config */
			ixgbe_config_dmac(adapter);
			if_link_state_change(ifp, LINK_STATE_UP);
			if (adapter->feat_en & IXGBE_FEATURE_SRIOV)
				ixgbe_ping_all_vfs(adapter);
		}
	} else { /* Link down */
		if (adapter->link_active == TRUE) {
			if (bootverbose)
				device_printf(dev, "Link is Down\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
			adapter->link_active = FALSE;
			if (adapter->feat_en & IXGBE_FEATURE_SRIOV)
				ixgbe_ping_all_vfs(adapter);
		}
	}

	return;
} /* ixgbe_update_link_status */

/************************************************************************
 * ixgbe_config_dmac - Configure DMA Coalescing
 ************************************************************************/
static void
ixgbe_config_dmac(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_dmac_config *dcfg = &hw->mac.dmac_config;

	if (hw->mac.type < ixgbe_mac_X550 || !hw->mac.ops.dmac_config)
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
} /* ixgbe_config_dmac */

/************************************************************************
 * ixgbe_enable_intr
 ************************************************************************/
static void
ixgbe_enable_intr(struct adapter *adapter)
{
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ix_queue	*que = adapter->queues;
	u32		mask, fwsm;

	mask = (IXGBE_EIMS_ENABLE_MASK & ~IXGBE_EIMS_RTX_QUEUE);

	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82599EB:
		mask |= IXGBE_EIMS_ECC;
		/* Temperature sensor on some adapters */
		mask |= IXGBE_EIMS_GPI_SDP0;
		/* SFP+ (RX_LOS_N & MOD_ABS_N) */
		mask |= IXGBE_EIMS_GPI_SDP1;
		mask |= IXGBE_EIMS_GPI_SDP2;
		break;
	case ixgbe_mac_X540:
		/* Detect if Thermal Sensor is enabled */
		fwsm = IXGBE_READ_REG(hw, IXGBE_FWSM);
		if (fwsm & IXGBE_FWSM_TS_ENABLED)
			mask |= IXGBE_EIMS_TS;
		mask |= IXGBE_EIMS_ECC;
		break;
	case ixgbe_mac_X550:
		/* MAC thermal sensor is automatically enabled */
		mask |= IXGBE_EIMS_TS;
		mask |= IXGBE_EIMS_ECC;
		break;
	case ixgbe_mac_X550EM_x:
	case ixgbe_mac_X550EM_a:
		/* Some devices use SDP0 for important information */
		if (hw->device_id == IXGBE_DEV_ID_X550EM_X_SFP ||
		    hw->device_id == IXGBE_DEV_ID_X550EM_A_SFP ||
		    hw->device_id == IXGBE_DEV_ID_X550EM_A_SFP_N ||
		    hw->device_id == IXGBE_DEV_ID_X550EM_X_10G_T)
			mask |= IXGBE_EIMS_GPI_SDP0_BY_MAC(hw);
		if (hw->phy.type == ixgbe_phy_x550em_ext_t)
			mask |= IXGBE_EICR_GPI_SDP0_X540;
		mask |= IXGBE_EIMS_ECC;
		break;
	default:
		break;
	}

	/* Enable Fan Failure detection */
	if (adapter->feat_en & IXGBE_FEATURE_FAN_FAIL)
		mask |= IXGBE_EIMS_GPI_SDP1;
	/* Enable SR-IOV */
	if (adapter->feat_en & IXGBE_FEATURE_SRIOV)
		mask |= IXGBE_EIMS_MAILBOX;
	/* Enable Flow Director */
	if (adapter->feat_en & IXGBE_FEATURE_FDIR)
		mask |= IXGBE_EIMS_FLOW_DIR;

	IXGBE_WRITE_REG(hw, IXGBE_EIMS, mask);

	/* With MSI-X we use auto clear */
	if (adapter->msix_mem) {
		mask = IXGBE_EIMS_ENABLE_MASK;
		/* Don't autoclear Link */
		mask &= ~IXGBE_EIMS_OTHER;
		mask &= ~IXGBE_EIMS_LSC;
		if (adapter->feat_cap & IXGBE_FEATURE_SRIOV)
			mask &= ~IXGBE_EIMS_MAILBOX;
		IXGBE_WRITE_REG(hw, IXGBE_EIAC, mask);
	}

	/*
	 * Now enable all queues, this is done separately to
	 * allow for handling the extended (beyond 32) MSI-X
	 * vectors that can be used by 82599
	 */
        for (int i = 0; i < adapter->num_queues; i++, que++)
                ixgbe_enable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(hw);

	return;
} /* ixgbe_enable_intr */

/************************************************************************
 * ixgbe_disable_intr
 ************************************************************************/
static void
ixgbe_disable_intr(struct adapter *adapter)
{
	struct ix_queue	*que = adapter->queues;

	/* disable interrupts other than queues */
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, ~IXGBE_EIMC_RTX_QUEUE);

	if (adapter->msix_mem)
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIAC, 0);

	for (int i = 0; i < adapter->num_queues; i++, que++)
		ixgbe_disable_queue(adapter, que->msix);

	IXGBE_WRITE_FLUSH(&adapter->hw);

	return;
} /* ixgbe_disable_intr */

/************************************************************************
 * ixgbe_legacy_irq - Legacy Interrupt Service routine
 ************************************************************************/
static int
ixgbe_legacy_irq(void *arg)
{
	struct ix_queue *que = arg;
	struct adapter	*adapter = que->adapter;
	struct ixgbe_hw	*hw = &adapter->hw;
	struct ifnet    *ifp = adapter->ifp;
	struct 		tx_ring *txr = adapter->tx_rings;
	bool		more = false;
	u32             eicr, eicr_mask;

	/* Silicon errata #26 on 82598 */
	IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_IRQ_CLEAR_MASK);

	eicr = IXGBE_READ_REG(hw, IXGBE_EICR);

	adapter->stats.pf.legint.ev_count++;
	++que->irqs.ev_count;
	if (eicr == 0) {
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
#ifdef notyet
		if (!ixgbe_ring_empty(ifp, txr->br))
			ixgbe_start_locked(ifp, txr);
#endif
		IXGBE_TX_UNLOCK(txr);
	}

	/* Check for fan failure */
	if (adapter->feat_en & IXGBE_FEATURE_FAN_FAIL) {
		ixgbe_check_fan_failure(adapter, eicr, true);
		IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
	}

	/* Link status change */
	if (eicr & IXGBE_EICR_LSC)
		softint_schedule(adapter->link_si);

	if (ixgbe_is_sfp(hw)) {
		/* Pluggable optics-related interrupt */
		if (hw->mac.type >= ixgbe_mac_X540)
			eicr_mask = IXGBE_EICR_GPI_SDP0_X540;
		else
			eicr_mask = IXGBE_EICR_GPI_SDP2_BY_MAC(hw);

		if (eicr & eicr_mask) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR, eicr_mask);
			softint_schedule(adapter->mod_si);
		}

		if ((hw->mac.type == ixgbe_mac_82599EB) &&
		    (eicr & IXGBE_EICR_GPI_SDP1_BY_MAC(hw))) {
			IXGBE_WRITE_REG(hw, IXGBE_EICR,
			    IXGBE_EICR_GPI_SDP1_BY_MAC(hw));
			softint_schedule(adapter->msf_si);
		}
	}

	/* External PHY interrupt */
	if ((hw->phy.type == ixgbe_phy_x550em_ext_t) &&
	    (eicr & IXGBE_EICR_GPI_SDP0_X540))
		softint_schedule(adapter->phy_si);

	if (more) {
		que->req.ev_count++;
		softint_schedule(que->que_si);
	} else
		ixgbe_enable_intr(adapter);

	return 1;
} /* ixgbe_legacy_irq */

/************************************************************************
 * ixgbe_free_pciintr_resources
 ************************************************************************/
static void
ixgbe_free_pciintr_resources(struct adapter *adapter)
{
	struct ix_queue *que = adapter->queues;
	int		rid;

	/*
	 * Release all msix queue resources:
	 */
	for (int i = 0; i < adapter->num_queues; i++, que++) {
		if (que->res != NULL) {
			pci_intr_disestablish(adapter->osdep.pc,
			    adapter->osdep.ihs[i]);
			adapter->osdep.ihs[i] = NULL;
		}
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

	if (adapter->osdep.intrs != NULL) {
		pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs,
		    adapter->osdep.nintrs);
		adapter->osdep.intrs = NULL;
	}

	return;
} /* ixgbe_free_pciintr_resources */

/************************************************************************
 * ixgbe_free_pci_resources
 ************************************************************************/
static void
ixgbe_free_pci_resources(struct adapter *adapter)
{

	ixgbe_free_pciintr_resources(adapter);

	if (adapter->osdep.mem_size != 0) {
		bus_space_unmap(adapter->osdep.mem_bus_space_tag,
		    adapter->osdep.mem_bus_space_handle,
		    adapter->osdep.mem_size);
	}

	return;
} /* ixgbe_free_pci_resources */

/************************************************************************
 * ixgbe_set_sysctl_value
 ************************************************************************/
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
} /* ixgbe_set_sysctl_value */

/************************************************************************
 * ixgbe_sysctl_flowcntl
 *
 *   SYSCTL wrapper around setting Flow Control
 ************************************************************************/
static int
ixgbe_sysctl_flowcntl(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	int error, fc;

	fc = adapter->hw.fc.current_mode;
	node.sysctl_data = &fc;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	/* Don't bother if it's not changed */
	if (fc == adapter->hw.fc.current_mode)
		return (0);

	return ixgbe_set_flowcntl(adapter, fc);
} /* ixgbe_sysctl_flowcntl */

/************************************************************************
 * ixgbe_set_flowcntl - Set flow control
 *
 *   Flow control values:
 *     0 - off
 *     1 - rx pause
 *     2 - tx pause
 *     3 - full
 ************************************************************************/
static int
ixgbe_set_flowcntl(struct adapter *adapter, int fc)
{
	switch (fc) {
		case ixgbe_fc_rx_pause:
		case ixgbe_fc_tx_pause:
		case ixgbe_fc_full:
			adapter->hw.fc.requested_mode = fc;
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

#if 0 /* XXX NetBSD */
	/* Don't autoneg if forcing a value */
	adapter->hw.fc.disable_fc_autoneg = TRUE;
#endif
	ixgbe_fc_enable(&adapter->hw);

	return (0);
} /* ixgbe_set_flowcntl */

/************************************************************************
 * ixgbe_enable_rx_drop
 *
 *   Enable the hardware to drop packets when the buffer is
 *   full. This is useful with multiqueue, so that no single
 *   queue being full stalls the entire RX engine. We only
 *   enable this when Multiqueue is enabled AND Flow Control
 *   is disabled.
 ************************************************************************/
static void
ixgbe_enable_rx_drop(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct rx_ring  *rxr;
	u32             srrctl;

	for (int i = 0; i < adapter->num_queues; i++) {
		rxr = &adapter->rx_rings[i];
		srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(rxr->me));
		srrctl |= IXGBE_SRRCTL_DROP_EN;
		IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(rxr->me), srrctl);
	}

	/* enable drop for each vf */
	for (int i = 0; i < adapter->num_vfs; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_QDE,
		    (IXGBE_QDE_WRITE | (i << IXGBE_QDE_IDX_SHIFT) |
		    IXGBE_QDE_ENABLE));
	}
} /* ixgbe_enable_rx_drop */

/************************************************************************
 * ixgbe_disable_rx_drop
 ************************************************************************/
static void
ixgbe_disable_rx_drop(struct adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct rx_ring  *rxr;
	u32             srrctl;

	for (int i = 0; i < adapter->num_queues; i++) {
		rxr = &adapter->rx_rings[i];
        	srrctl = IXGBE_READ_REG(hw, IXGBE_SRRCTL(rxr->me));
        	srrctl &= ~IXGBE_SRRCTL_DROP_EN;
        	IXGBE_WRITE_REG(hw, IXGBE_SRRCTL(rxr->me), srrctl);
	}

	/* disable drop for each vf */
	for (int i = 0; i < adapter->num_vfs; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_QDE,
		    (IXGBE_QDE_WRITE | (i << IXGBE_QDE_IDX_SHIFT)));
	}
} /* ixgbe_disable_rx_drop */

/************************************************************************
 * ixgbe_sysctl_advertise
 *
 *   SYSCTL wrapper around setting advertised speed
 ************************************************************************/
static int
ixgbe_sysctl_advertise(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	int            error = 0, advertise;

	advertise = adapter->advertise;
	node.sysctl_data = &advertise;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	return ixgbe_set_advertise(adapter, advertise);
} /* ixgbe_sysctl_advertise */

/************************************************************************
 * ixgbe_set_advertise - Control advertised link speed
 *
 *   Flags:
 *     0x00 - Default (all capable link speed)
 *     0x01 - advertise 100 Mb
 *     0x02 - advertise 1G
 *     0x04 - advertise 10G
 *     0x08 - advertise 10 Mb
 *     0x10 - advertise 2.5G
 *     0x20 - advertise 5G
 ************************************************************************/
static int
ixgbe_set_advertise(struct adapter *adapter, int advertise)
{
	device_t         dev;
	struct ixgbe_hw  *hw;
	ixgbe_link_speed speed = 0;
	ixgbe_link_speed link_caps = 0;
	s32              err = IXGBE_NOT_IMPLEMENTED;
	bool             negotiate = FALSE;

	/* Checks to validate new value */
	if (adapter->advertise == advertise) /* no change */
		return (0);

	dev = adapter->dev;
	hw = &adapter->hw;

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

	if (advertise < 0x0 || advertise > 0x2f) {
		device_printf(dev,
		    "Invalid advertised speed; valid modes are 0x0 through 0x7\n");
		return (EINVAL);
	}

	if (hw->mac.ops.get_link_capabilities) {
		err = hw->mac.ops.get_link_capabilities(hw, &link_caps,
		    &negotiate);
		if (err != IXGBE_SUCCESS) {
			device_printf(dev, "Unable to determine supported advertise speeds\n");
			return (ENODEV);
		}
	}

	/* Set new value and report new advertised mode */
	if (advertise & 0x1) {
		if (!(link_caps & IXGBE_LINK_SPEED_100_FULL)) {
			device_printf(dev, "Interface does not support 100Mb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_100_FULL;
	}
	if (advertise & 0x2) {
		if (!(link_caps & IXGBE_LINK_SPEED_1GB_FULL)) {
			device_printf(dev, "Interface does not support 1Gb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_1GB_FULL;
	}
	if (advertise & 0x4) {
		if (!(link_caps & IXGBE_LINK_SPEED_10GB_FULL)) {
			device_printf(dev, "Interface does not support 10Gb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_10GB_FULL;
	}
	if (advertise & 0x8) {
		if (!(link_caps & IXGBE_LINK_SPEED_10_FULL)) {
			device_printf(dev, "Interface does not support 10Mb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_10_FULL;
	}
	if (advertise & 0x10) {
		if (!(link_caps & IXGBE_LINK_SPEED_2_5GB_FULL)) {
			device_printf(dev, "Interface does not support 2.5Gb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_2_5GB_FULL;
	}
	if (advertise & 0x20) {
		if (!(link_caps & IXGBE_LINK_SPEED_5GB_FULL)) {
			device_printf(dev, "Interface does not support 5Gb advertised speed\n");
			return (EINVAL);
		}
		speed |= IXGBE_LINK_SPEED_5GB_FULL;
	}
	if (advertise == 0)
		speed = link_caps; /* All capable link speed */

	hw->mac.autotry_restart = TRUE;
	hw->mac.ops.setup_link(hw, speed, TRUE);
	adapter->advertise = advertise;

	return (0);
} /* ixgbe_set_advertise */

/************************************************************************
 * ixgbe_get_advertise - Get current advertised speed settings
 *
 *   Formatted for sysctl usage.
 *   Flags:
 *     0x01 - advertise 100 Mb
 *     0x02 - advertise 1G
 *     0x04 - advertise 10G
 *     0x08 - advertise 10 Mb (yes, Mb)
 *     0x10 - advertise 2.5G
 *     0x20 - advertise 5G
 ************************************************************************/
static int
ixgbe_get_advertise(struct adapter *adapter)
{
	struct ixgbe_hw  *hw = &adapter->hw;
	int              speed;
	ixgbe_link_speed link_caps = 0;
	s32              err;
	bool             negotiate = FALSE;

	/*
	 * Advertised speed means nothing unless it's copper or
	 * multi-speed fiber
	 */
	if (!(hw->phy.media_type == ixgbe_media_type_copper) &&
	    !(hw->phy.multispeed_fiber))
		return (0);

	err = hw->mac.ops.get_link_capabilities(hw, &link_caps, &negotiate);
	if (err != IXGBE_SUCCESS)
		return (0);

	speed =
	    ((link_caps & IXGBE_LINK_SPEED_10GB_FULL)  ? 0x04 : 0) |
	    ((link_caps & IXGBE_LINK_SPEED_1GB_FULL)   ? 0x02 : 0) |
	    ((link_caps & IXGBE_LINK_SPEED_100_FULL)   ? 0x01 : 0) |
	    ((link_caps & IXGBE_LINK_SPEED_10_FULL)    ? 0x08 : 0) |
	    ((link_caps & IXGBE_LINK_SPEED_2_5GB_FULL) ? 0x10 : 0) |
	    ((link_caps & IXGBE_LINK_SPEED_5GB_FULL)   ? 0x20 : 0);

	return speed;
} /* ixgbe_get_advertise */

/************************************************************************
 * ixgbe_sysctl_dmac - Manage DMA Coalescing
 *
 *   Control values:
 *     0/1 - off / on (use default value of 1000)
 *
 *     Legal timer values are:
 *     50,100,250,500,1000,2000,5000,10000
 *
 *     Turning off interrupt moderation will also turn this off.
 ************************************************************************/
static int
ixgbe_sysctl_dmac(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ifnet   *ifp = adapter->ifp;
	int            error;
	int            newval;

	newval = adapter->dmac;
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
/************************************************************************
 * ixgbe_sysctl_power_state
 *
 *   Sysctl to test power states
 *   Values:
 *     0      - set device to D0
 *     3      - set device to D3
 *     (none) - get current device power state
 ************************************************************************/
static int
ixgbe_sysctl_power_state(SYSCTLFN_ARGS)
{
#ifdef notyet
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	device_t       dev =  adapter->dev;
	int            curr_ps, new_ps, error = 0;

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
} /* ixgbe_sysctl_power_state */
#endif

/************************************************************************
 * ixgbe_sysctl_wol_enable
 *
 *   Sysctl to enable/disable the WoL capability,
 *   if supported by the adapter.
 *
 *   Values:
 *     0 - disabled
 *     1 - enabled
 ************************************************************************/
static int
ixgbe_sysctl_wol_enable(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter  *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	bool            new_wol_enabled;
	int             error = 0;

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
} /* ixgbe_sysctl_wol_enable */

/************************************************************************
 * ixgbe_sysctl_wufc - Wake Up Filter Control
 *
 *   Sysctl to enable/disable the types of packets that the
 *   adapter will wake up on upon receipt.
 *   Flags:
 *     0x1  - Link Status Change
 *     0x2  - Magic Packet
 *     0x4  - Direct Exact
 *     0x8  - Directed Multicast
 *     0x10 - Broadcast
 *     0x20 - ARP/IPv4 Request Packet
 *     0x40 - Direct IPv4 Packet
 *     0x80 - Direct IPv6 Packet
 *
 *   Settings not listed above will cause the sysctl to return an error.
 ************************************************************************/
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

	new_wufc &= 0xff;
	new_wufc |= (0xffffff & adapter->wufc);
	adapter->wufc = new_wufc;

	return (0);
} /* ixgbe_sysctl_wufc */

#ifdef IXGBE_DEBUG
/************************************************************************
 * ixgbe_sysctl_print_rss_config
 ************************************************************************/
static int
ixgbe_sysctl_print_rss_config(SYSCTLFN_ARGS)
{
#ifdef notyet
	struct sysctlnode node = *rnode;
	struct adapter  *adapter = (struct adapter *)node.sysctl_data;
	struct ixgbe_hw *hw = &adapter->hw;
	device_t        dev = adapter->dev;
	struct sbuf     *buf;
	int             error = 0, reta_size;
	u32             reg;

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
	case ixgbe_mac_X550EM_a:
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
#endif
	return (0);
} /* ixgbe_sysctl_print_rss_config */
#endif /* IXGBE_DEBUG */

/************************************************************************
 * ixgbe_sysctl_phy_temp - Retrieve temperature of PHY
 *
 *   For X552/X557-AT devices using an external PHY
 ************************************************************************/
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
		IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE, &reg)) {
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
} /* ixgbe_sysctl_phy_temp */

/************************************************************************
 * ixgbe_sysctl_phy_overtemp_occurred
 *
 *   Reports (directly from the PHY) whether the current PHY
 *   temperature is over the overtemp threshold.
 ************************************************************************/
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
		IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE, &reg)) {
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
} /* ixgbe_sysctl_phy_overtemp_occurred */

/************************************************************************
 * ixgbe_sysctl_eee_state
 *
 *   Sysctl to set EEE power saving feature
 *   Values:
 *     0      - disable EEE
 *     1      - enable EEE
 *     (none) - get current device EEE state
 ************************************************************************/
static int
ixgbe_sysctl_eee_state(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adapter *adapter = (struct adapter *)node.sysctl_data;
	struct ifnet   *ifp = adapter->ifp;
	device_t       dev = adapter->dev;
	int            curr_eee, new_eee, error = 0;
	s32            retval;

	curr_eee = new_eee = !!(adapter->feat_en & IXGBE_FEATURE_EEE);
	node.sysctl_data = &new_eee;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if ((error) || (newp == NULL))
		return (error);

	/* Nothing to do */
	if (new_eee == curr_eee)
		return (0);

	/* Not supported */
	if (!(adapter->feat_cap & IXGBE_FEATURE_EEE))
		return (EINVAL);

	/* Bounds checking */
	if ((new_eee < 0) || (new_eee > 1))
		return (EINVAL);

	retval = adapter->hw.mac.ops.setup_eee(&adapter->hw, new_eee);
	if (retval) {
		device_printf(dev, "Error in EEE setup: 0x%08X\n", retval);
		return (EINVAL);
	}

	/* Restart auto-neg */
	ixgbe_init(ifp);

	device_printf(dev, "New EEE state: %d\n", new_eee);

	/* Cache new value */
	if (new_eee)
		adapter->feat_en |= IXGBE_FEATURE_EEE;
	else
		adapter->feat_en &= ~IXGBE_FEATURE_EEE;

	return (error);
} /* ixgbe_sysctl_eee_state */

/************************************************************************
 * ixgbe_init_device_features
 ************************************************************************/
static void
ixgbe_init_device_features(struct adapter *adapter)
{
	adapter->feat_cap = IXGBE_FEATURE_NETMAP
	                  | IXGBE_FEATURE_RSS
	                  | IXGBE_FEATURE_MSI
	                  | IXGBE_FEATURE_MSIX
	                  | IXGBE_FEATURE_LEGACY_IRQ
	                  | IXGBE_FEATURE_LEGACY_TX;

	/* Set capabilities first... */
	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82598EB:
		if (adapter->hw.device_id == IXGBE_DEV_ID_82598AT)
			adapter->feat_cap |= IXGBE_FEATURE_FAN_FAIL;
		break;
	case ixgbe_mac_X540:
		adapter->feat_cap |= IXGBE_FEATURE_SRIOV;
		adapter->feat_cap |= IXGBE_FEATURE_FDIR;
		if ((adapter->hw.device_id == IXGBE_DEV_ID_X540_BYPASS) &&
		    (adapter->hw.bus.func == 0))
			adapter->feat_cap |= IXGBE_FEATURE_BYPASS;
		break;
	case ixgbe_mac_X550:
		adapter->feat_cap |= IXGBE_FEATURE_TEMP_SENSOR;
		adapter->feat_cap |= IXGBE_FEATURE_SRIOV;
		adapter->feat_cap |= IXGBE_FEATURE_FDIR;
		break;
	case ixgbe_mac_X550EM_x:
		adapter->feat_cap |= IXGBE_FEATURE_SRIOV;
		adapter->feat_cap |= IXGBE_FEATURE_FDIR;
		if (adapter->hw.device_id == IXGBE_DEV_ID_X550EM_X_KR)
			adapter->feat_cap |= IXGBE_FEATURE_EEE;
		break;
	case ixgbe_mac_X550EM_a:
		adapter->feat_cap |= IXGBE_FEATURE_SRIOV;
		adapter->feat_cap |= IXGBE_FEATURE_FDIR;
		adapter->feat_cap &= ~IXGBE_FEATURE_LEGACY_IRQ;
		if ((adapter->hw.device_id == IXGBE_DEV_ID_X550EM_A_1G_T) ||
		    (adapter->hw.device_id == IXGBE_DEV_ID_X550EM_A_1G_T_L)) {
			adapter->feat_cap |= IXGBE_FEATURE_TEMP_SENSOR;
			adapter->feat_cap |= IXGBE_FEATURE_EEE;
		}
		break;
	case ixgbe_mac_82599EB:
		adapter->feat_cap |= IXGBE_FEATURE_SRIOV;
		adapter->feat_cap |= IXGBE_FEATURE_FDIR;
		if ((adapter->hw.device_id == IXGBE_DEV_ID_82599_BYPASS) &&
		    (adapter->hw.bus.func == 0))
			adapter->feat_cap |= IXGBE_FEATURE_BYPASS;
		if (adapter->hw.device_id == IXGBE_DEV_ID_82599_QSFP_SF_QP)
			adapter->feat_cap &= ~IXGBE_FEATURE_LEGACY_IRQ;
		break;
	default:
		break;
	}

	/* Enabled by default... */
	/* Fan failure detection */
	if (adapter->feat_cap & IXGBE_FEATURE_FAN_FAIL)
		adapter->feat_en |= IXGBE_FEATURE_FAN_FAIL;
	/* Netmap */
	if (adapter->feat_cap & IXGBE_FEATURE_NETMAP)
		adapter->feat_en |= IXGBE_FEATURE_NETMAP;
	/* EEE */
	if (adapter->feat_cap & IXGBE_FEATURE_EEE)
		adapter->feat_en |= IXGBE_FEATURE_EEE;
	/* Thermal Sensor */
	if (adapter->feat_cap & IXGBE_FEATURE_TEMP_SENSOR)
		adapter->feat_en |= IXGBE_FEATURE_TEMP_SENSOR;

	/* Enabled via global sysctl... */
	/* Flow Director */
	if (ixgbe_enable_fdir) {
		if (adapter->feat_cap & IXGBE_FEATURE_FDIR)
			adapter->feat_en |= IXGBE_FEATURE_FDIR;
		else
			device_printf(adapter->dev, "Device does not support Flow Director. Leaving disabled.");
	}
	/* Legacy (single queue) transmit */
	if ((adapter->feat_cap & IXGBE_FEATURE_LEGACY_TX) &&
	    ixgbe_enable_legacy_tx)
		adapter->feat_en |= IXGBE_FEATURE_LEGACY_TX;
	/*
	 * Message Signal Interrupts - Extended (MSI-X)
	 * Normal MSI is only enabled if MSI-X calls fail.
	 */
	if (!ixgbe_enable_msix)
		adapter->feat_cap &= ~IXGBE_FEATURE_MSIX;
	/* Receive-Side Scaling (RSS) */
	if ((adapter->feat_cap & IXGBE_FEATURE_RSS) && ixgbe_enable_rss)
		adapter->feat_en |= IXGBE_FEATURE_RSS;

	/* Disable features with unmet dependencies... */
	/* No MSI-X */
	if (!(adapter->feat_cap & IXGBE_FEATURE_MSIX)) {
		adapter->feat_cap &= ~IXGBE_FEATURE_RSS;
		adapter->feat_cap &= ~IXGBE_FEATURE_SRIOV;
		adapter->feat_en &= ~IXGBE_FEATURE_RSS;
		adapter->feat_en &= ~IXGBE_FEATURE_SRIOV;
	}
} /* ixgbe_init_device_features */

/************************************************************************
 * ixgbe_probe - Device identification routine
 *
 *   Determines if the driver should be loaded on
 *   adapter based on its PCI vendor/device ID.
 *
 *   return BUS_PROBE_DEFAULT on success, positive on failure
 ************************************************************************/
static int
ixgbe_probe(device_t dev, cfdata_t cf, void *aux)
{
	const struct pci_attach_args *pa = aux;

	return (ixgbe_lookup(pa) != NULL) ? 1 : 0;
}

static ixgbe_vendor_info_t *
ixgbe_lookup(const struct pci_attach_args *pa)
{
	ixgbe_vendor_info_t *ent;
	pcireg_t subid;

	INIT_DEBUGOUT("ixgbe_lookup: begin");

	if (PCI_VENDOR(pa->pa_id) != IXGBE_INTEL_VENDOR_ID)
		return NULL;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	for (ent = ixgbe_vendor_info_array; ent->vendor_id != 0; ent++) {
		if ((PCI_VENDOR(pa->pa_id) == ent->vendor_id) &&
		    (PCI_PRODUCT(pa->pa_id) == ent->device_id) &&
		    ((PCI_SUBSYS_VENDOR(subid) == ent->subvendor_id) ||
			(ent->subvendor_id == 0)) &&
		    ((PCI_SUBSYS_ID(subid) == ent->subdevice_id) ||
			(ent->subdevice_id == 0))) {
			++ixgbe_total_ports;
			return ent;
		}
	}
	return NULL;
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

	if ((change & ~(IFF_CANTCHANGE | IFF_DEBUG)) != 0)
		rc = ENETRESET;
	else if ((change & (IFF_PROMISC | IFF_ALLMULTI)) != 0)
		ixgbe_set_promisc(adapter);

	/* Set up VLAN support and filter */
	ixgbe_setup_vlan_hw_support(adapter);

	IXGBE_CORE_UNLOCK(adapter);

	return rc;
}

/************************************************************************
 * ixgbe_ioctl - Ioctl entry point
 *
 *   Called when the user wants to configure the interface.
 *
 *   return 0 on success, positive on failure
 ************************************************************************/
static int
ixgbe_ioctl(struct ifnet * ifp, u_long command, void *data)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ixgbe_hw *hw = &adapter->hw;
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
	case SIOCZIFDATA:
		IOCTL_DEBUGOUT("ioctl: SIOCZIFDATA (Zero counter)");
		hw->mac.ops.clear_hw_cntrs(hw);
		ixgbe_clear_evcnt(adapter);
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
			ixgbe_recalculate_max_frame(adapter);
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
} /* ixgbe_ioctl */

/************************************************************************
 * ixgbe_check_fan_failure
 ************************************************************************/
static void
ixgbe_check_fan_failure(struct adapter *adapter, u32 reg, bool in_interrupt)
{
	u32 mask;

	mask = (in_interrupt) ? IXGBE_EICR_GPI_SDP1_BY_MAC(&adapter->hw) :
	    IXGBE_ESDP_SDP1;

	if (reg & mask)
		device_printf(adapter->dev, "\nCRITICAL: FAN FAILURE!! REPLACE IMMEDIATELY!!\n");
} /* ixgbe_check_fan_failure */

/************************************************************************
 * ixgbe_handle_que
 ************************************************************************/
static void
ixgbe_handle_que(void *context)
{
	struct ix_queue *que = context;
	struct adapter  *adapter = que->adapter;
	struct tx_ring  *txr = que->txr;
	struct ifnet    *ifp = adapter->ifp;
	bool		more = false;

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
	}

	if (more) {
		que->req.ev_count++;
		if (adapter->txrx_use_workqueue) {
			/*
			 * "enqueued flag" is not required here.
			 * See ixgbe_msix_que().
			 */
			workqueue_enqueue(adapter->que_wq, &que->wq_cookie,
			    curcpu());
		} else {
			softint_schedule(que->que_si);
		}
	} else if (que->res != NULL) {
		/* Re-enable this interrupt */
		ixgbe_enable_queue(adapter, que->msix);
	} else
		ixgbe_enable_intr(adapter);

	return;
} /* ixgbe_handle_que */

/************************************************************************
 * ixgbe_handle_que_work
 ************************************************************************/
static void
ixgbe_handle_que_work(struct work *wk, void *context)
{
	struct ix_queue *que = container_of(wk, struct ix_queue, wq_cookie);

	/*
	 * "enqueued flag" is not required here.
	 * See ixgbe_msix_que().
	 */
	ixgbe_handle_que(que);
}

/************************************************************************
 * ixgbe_allocate_legacy - Setup the Legacy or MSI Interrupt handler
 ************************************************************************/
static int
ixgbe_allocate_legacy(struct adapter *adapter,
    const struct pci_attach_args *pa)
{
	device_t	dev = adapter->dev;
	struct ix_queue *que = adapter->queues;
	struct tx_ring  *txr = adapter->tx_rings;
	int		counts[PCI_INTR_TYPE_SIZE];
	pci_intr_type_t intr_type, max_type;
	char            intrbuf[PCI_INTRSTR_LEN];
	const char	*intrstr = NULL;
 
	/* We allocate a single interrupt resource */
	max_type = PCI_INTR_TYPE_MSI;
	counts[PCI_INTR_TYPE_MSIX] = 0;
	counts[PCI_INTR_TYPE_MSI] =
	    (adapter->feat_en & IXGBE_FEATURE_MSI) ? 1 : 0;
	/* Check not feat_en but feat_cap to fallback to INTx */
	counts[PCI_INTR_TYPE_INTX] =
	    (adapter->feat_cap & IXGBE_FEATURE_LEGACY_IRQ) ? 1 : 0;

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
	intr_type = pci_intr_type(adapter->osdep.pc, adapter->osdep.intrs[0]);
	if (adapter->osdep.ihs[0] == NULL) {
		aprint_error_dev(dev,"unable to establish %s\n",
		    (intr_type == PCI_INTR_TYPE_MSI) ? "MSI" : "INTx");
		pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs, 1);
		adapter->osdep.intrs = NULL;
		switch (intr_type) {
		case PCI_INTR_TYPE_MSI:
			/* The next try is for INTx: Disable MSI */
			max_type = PCI_INTR_TYPE_INTX;
			counts[PCI_INTR_TYPE_INTX] = 1;
			adapter->feat_en &= ~IXGBE_FEATURE_MSI;
			if (adapter->feat_cap & IXGBE_FEATURE_LEGACY_IRQ) {
				adapter->feat_en |= IXGBE_FEATURE_LEGACY_IRQ;
				goto alloc_retry;
			} else
				break;
		case PCI_INTR_TYPE_INTX:
		default:
			/* See below */
			break;
		}
	}
	if (intr_type == PCI_INTR_TYPE_INTX) {
		adapter->feat_en &= ~IXGBE_FEATURE_MSI;
		adapter->feat_en |= IXGBE_FEATURE_LEGACY_IRQ;
	}
	if (adapter->osdep.ihs[0] == NULL) {
		aprint_error_dev(dev,
		    "couldn't establish interrupt%s%s\n",
		    intrstr ? " at " : "", intrstr ? intrstr : "");
		pci_intr_release(adapter->osdep.pc, adapter->osdep.intrs, 1);
		adapter->osdep.intrs = NULL;
		return ENXIO;
	}
	aprint_normal_dev(dev, "interrupting at %s\n", intrstr);
	/*
	 * Try allocating a fast interrupt and the associated deferred
	 * processing contexts.
	 */
	if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX))
		txr->txr_si =
		    softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixgbe_deferred_mq_start, txr);
	que->que_si = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
	    ixgbe_handle_que, que);

	if ((!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX)
		& (txr->txr_si == NULL)) || (que->que_si == NULL)) {
		aprint_error_dev(dev,
		    "could not establish software interrupts\n"); 

		return ENXIO;
	}
	/* For simplicity in the handlers */
	adapter->active_queues = IXGBE_EIMS_ENABLE_MASK;

	return (0);
} /* ixgbe_allocate_legacy */

/************************************************************************
 * ixgbe_allocate_msix - Setup MSI-X Interrupt resources and handlers
 ************************************************************************/
static int
ixgbe_allocate_msix(struct adapter *adapter, const struct pci_attach_args *pa)
{
	device_t        dev = adapter->dev;
	struct 		ix_queue *que = adapter->queues;
	struct  	tx_ring *txr = adapter->tx_rings;
	pci_chipset_tag_t pc;
	char		intrbuf[PCI_INTRSTR_LEN];
	char		intr_xname[32];
	char		wqname[MAXCOMLEN];
	const char	*intrstr = NULL;
	int 		error, vector = 0;
	int		cpu_id = 0;
	kcpuset_t	*affinity;
#ifdef RSS
	unsigned int    rss_buckets = 0;
	kcpuset_t	cpu_mask;
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
	rss_buckets = rss_getnumbuckets();
	if ((adapter->feat_en & IXGBE_FEATURE_RSS) &&
	    (adapter->num_queues != rss_buckets)) {
		device_printf(dev,
		    "%s: number of queues (%d) != number of RSS buckets (%d)"
		    "; performance will be impacted.\n",
		    __func__, adapter->num_queues, rss_buckets);
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
		    adapter->osdep.intrs[i], IPL_NET, ixgbe_msix_que, que,
		    intr_xname);
		if (que->res == NULL) {
			aprint_error_dev(dev,
			    "Failed to register QUE handler\n");
			error = ENXIO;
			goto err_out;
		}
		que->msix = vector;
		adapter->active_queues |= (u64)(1 << que->msix);

		if (adapter->feat_en & IXGBE_FEATURE_RSS) {
#ifdef	RSS
			/*
			 * The queue ID is used as the RSS layer bucket ID.
			 * We look up the queue ID -> RSS CPU ID and select
			 * that.
			 */
			cpu_id = rss_getcpu(i % rss_getnumbuckets());
			CPU_SETOF(cpu_id, &cpu_mask);
#endif
		} else {
			/*
			 * Bind the MSI-X vector, and thus the
			 * rings to the corresponding CPU.
			 *
			 * This just happens to match the default RSS
			 * round-robin bucket -> queue -> CPU allocation.
			 */
			if (adapter->num_queues > 1)
				cpu_id = i;
		}
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
			aprintf_normal(", bound RSS bucket %d to CPU %d", i,
			    cpu_id % ncpu);
#else
			aprint_normal(", bound queue %d to cpu %d", i,
			    cpu_id % ncpu);
#endif
#endif /* IXGBE_DEBUG */
		}
		aprint_normal("\n");

		if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX)) {
			txr->txr_si = softint_establish(
				SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
				ixgbe_deferred_mq_start, txr);
			if (txr->txr_si == NULL) {
				aprint_error_dev(dev,
				    "couldn't establish software interrupt\n");
				error = ENXIO;
				goto err_out;
			}
		}
		que->que_si
		    = softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixgbe_handle_que, que);
		if (que->que_si == NULL) {
			aprint_error_dev(dev,
			    "couldn't establish software interrupt\n"); 
			error = ENXIO;
			goto err_out;
		}
	}
	snprintf(wqname, sizeof(wqname), "%sdeferTx", device_xname(dev));
	error = workqueue_create(&adapter->txr_wq, wqname,
	    ixgbe_deferred_mq_start_work, adapter, IXGBE_WORKQUEUE_PRI, IPL_NET,
	    IXGBE_WORKQUEUE_FLAGS);
	if (error) {
		aprint_error_dev(dev, "couldn't create workqueue for deferred Tx\n");
		goto err_out;
	}
	adapter->txr_wq_enqueued = percpu_alloc(sizeof(u_int));

	snprintf(wqname, sizeof(wqname), "%sTxRx", device_xname(dev));
	error = workqueue_create(&adapter->que_wq, wqname,
	    ixgbe_handle_que_work, adapter, IXGBE_WORKQUEUE_PRI, IPL_NET,
	    IXGBE_WORKQUEUE_FLAGS);
	if (error) {
		aprint_error_dev(dev, "couldn't create workqueue for Tx/Rx\n");
		goto err_out;
	}

	/* and Link */
	cpu_id++;
	snprintf(intr_xname, sizeof(intr_xname), "%s link", device_xname(dev));
	adapter->vector = vector;
	intrstr = pci_intr_string(pc, adapter->osdep.intrs[vector], intrbuf,
	    sizeof(intrbuf));
#ifdef IXGBE_MPSAFE
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
		error = ENXIO;
		goto err_out;
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

	if (adapter->feat_cap & IXGBE_FEATURE_SRIOV) {
		adapter->mbx_si =
		    softint_establish(SOFTINT_NET | IXGBE_SOFTINFT_FLAGS,
			ixgbe_handle_mbx, adapter);
		if (adapter->mbx_si == NULL) {
			aprint_error_dev(dev,
			    "could not establish software interrupts\n"); 

			error = ENXIO;
			goto err_out;
		}
	}

	kcpuset_destroy(affinity);
	aprint_normal_dev(dev,
	    "Using MSI-X interrupts with %d vectors\n", vector + 1);

	return (0);

err_out:
	kcpuset_destroy(affinity);
	ixgbe_free_softint(adapter);
	ixgbe_free_pciintr_resources(adapter);
	return (error);
} /* ixgbe_allocate_msix */

/************************************************************************
 * ixgbe_configure_interrupts
 *
 *   Setup MSI-X, MSI, or legacy interrupts (in that order).
 *   This will also depend on user settings.
 ************************************************************************/
static int
ixgbe_configure_interrupts(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct ixgbe_mac_info *mac = &adapter->hw.mac;
	int want, queues, msgs;

	/* Default to 1 queue if MSI-X setup fails */
	adapter->num_queues = 1;

	/* Override by tuneable */
	if (!(adapter->feat_cap & IXGBE_FEATURE_MSIX))
		goto msi;

	/*
	 *  NetBSD only: Use single vector MSI when number of CPU is 1 to save
	 * interrupt slot.
	 */
	if (ncpu == 1)
		goto msi;
	
	/* First try MSI-X */
	msgs = pci_msix_count(adapter->osdep.pc, adapter->osdep.tag);
	msgs = MIN(msgs, IXG_MAX_NINTR);
	if (msgs < 2)
		goto msi;

	adapter->msix_mem = (void *)1; /* XXX */

	/* Figure out a reasonable auto config value */
	queues = (ncpu > (msgs - 1)) ? (msgs - 1) : ncpu;

#ifdef	RSS
	/* If we're doing RSS, clamp at the number of RSS buckets */
	if (adapter->feat_en & IXGBE_FEATURE_RSS)
		queues = min(queues, rss_getnumbuckets());
#endif
	if (ixgbe_num_queues > queues) {
		aprint_error_dev(adapter->dev, "ixgbe_num_queues (%d) is too large, using reduced amount (%d).\n", ixgbe_num_queues, queues);
		ixgbe_num_queues = queues;
	}

	if (ixgbe_num_queues != 0)
		queues = ixgbe_num_queues;
	else
		queues = min(queues,
		    min(mac->max_tx_queues, mac->max_rx_queues));

	/* reflect correct sysctl value */
	ixgbe_num_queues = queues;

	/*
	 * Want one vector (RX/TX pair) per queue
	 * plus an additional for Link.
	 */
	want = queues + 1;
	if (msgs >= want)
		msgs = want;
	else {
               	aprint_error_dev(dev, "MSI-X Configuration Problem, "
		    "%d vectors but %d queues wanted!\n",
		    msgs, want);
		goto msi;
	}
	adapter->num_queues = queues;
	adapter->feat_en |= IXGBE_FEATURE_MSIX;
	return (0);

	/*
	 * MSI-X allocation failed or provided us with
	 * less vectors than needed. Free MSI-X resources
	 * and we'll try enabling MSI.
	 */
msi:
	/* Without MSI-X, some features are no longer supported */
	adapter->feat_cap &= ~IXGBE_FEATURE_RSS;
	adapter->feat_en  &= ~IXGBE_FEATURE_RSS;
	adapter->feat_cap &= ~IXGBE_FEATURE_SRIOV;
	adapter->feat_en  &= ~IXGBE_FEATURE_SRIOV;

       	msgs = pci_msi_count(adapter->osdep.pc, adapter->osdep.tag);
	adapter->msix_mem = NULL; /* XXX */
	if (msgs > 1)
		msgs = 1;
	if (msgs != 0) {
		msgs = 1;
		adapter->feat_en |= IXGBE_FEATURE_MSI;
		return (0);
	}

	if (!(adapter->feat_cap & IXGBE_FEATURE_LEGACY_IRQ)) {
		aprint_error_dev(dev,
		    "Device does not support legacy interrupts.\n");
		return 1;
	}

	adapter->feat_en |= IXGBE_FEATURE_LEGACY_IRQ;

	return (0);
} /* ixgbe_configure_interrupts */


/************************************************************************
 * ixgbe_handle_link - Tasklet for MSI-X Link interrupts
 *
 *   Done outside of interrupt context since the driver might sleep
 ************************************************************************/
static void
ixgbe_handle_link(void *context)
{
	struct adapter  *adapter = context;
	struct ixgbe_hw *hw = &adapter->hw;

	ixgbe_check_link(hw, &adapter->link_speed, &adapter->link_up, 0);
	ixgbe_update_link_status(adapter);

	/* Re-enable link interrupts */
	IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMS_LSC);
} /* ixgbe_handle_link */

/************************************************************************
 * ixgbe_rearm_queues
 ************************************************************************/
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
	case ixgbe_mac_X550EM_a:
		mask = (queues & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(0), mask);
		mask = (queues >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(1), mask);
		break;
	default:
		break;
	}
} /* ixgbe_rearm_queues */
