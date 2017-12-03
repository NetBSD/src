/*	$NetBSD: if_iwm.c,v 1.76.2.2 2017/12/03 11:37:07 jdolecek Exp $	*/
/*	OpenBSD: if_iwm.c,v 1.148 2016/11/19 21:07:08 stsp Exp	*/
#define IEEE80211_NO_HT
/*
 * Copyright (c) 2014, 2016 genua gmbh <info@genua.de>
 *   Author: Stefan Sperling <stsp@openbsd.org>
 * Copyright (c) 2014 Fixup Software Ltd.
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

/*-
 * Based on BSD-licensed source modules in the Linux iwlwifi driver,
 * which were used as the reference documentation for this implementation.
 *
 ***********************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2008 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016        Intel Deutschland GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * Contact Information:
 *  Intel Linux Wireless <linuxwifi@intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 * BSD LICENSE
 *
 * Copyright(c) 2005 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016        Intel Deutschland GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2007-2010 Damien Bergamini <damien.bergamini@free.fr>
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
__KERNEL_RCSID(0, "$NetBSD: if_iwm.c,v 1.76.2.2 2017/12/03 11:37:07 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/workqueue.h>
#include <machine/endian.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/firmload.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

#define DEVNAME(_s)	device_xname((_s)->sc_dev)
#define IC2IFP(_ic_)	((_ic_)->ic_ifp)

#define le16_to_cpup(_a_) (le16toh(*(const uint16_t *)(_a_)))
#define le32_to_cpup(_a_) (le32toh(*(const uint32_t *)(_a_)))

#ifdef IWM_DEBUG
#define DPRINTF(x)	do { if (iwm_debug > 0) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (iwm_debug >= (n)) printf x; } while (0)
int iwm_debug = 0;
#else
#define DPRINTF(x)	do { ; } while (0)
#define DPRINTFN(n, x)	do { ; } while (0)
#endif

#include <dev/pci/if_iwmreg.h>
#include <dev/pci/if_iwmvar.h>

static const uint8_t iwm_nvm_channels[] = {
	/* 2.4 GHz */
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	/* 5 GHz */
	36, 40, 44, 48, 52, 56, 60, 64,
	100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
	149, 153, 157, 161, 165
};

static const uint8_t iwm_nvm_channels_8000[] = {
	/* 2.4 GHz */
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	/* 5 GHz */
	36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92,
	96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
	149, 153, 157, 161, 165, 169, 173, 177, 181
};

#define IWM_NUM_2GHZ_CHANNELS	14

static const struct iwm_rate {
	uint8_t rate;
	uint8_t plcp;
	uint8_t ht_plcp;
} iwm_rates[] = {
		/* Legacy */		/* HT */
	{   2,	IWM_RATE_1M_PLCP,	IWM_RATE_HT_SISO_MCS_INV_PLCP  },
	{   4,	IWM_RATE_2M_PLCP,	IWM_RATE_HT_SISO_MCS_INV_PLCP },
	{  11,	IWM_RATE_5M_PLCP,	IWM_RATE_HT_SISO_MCS_INV_PLCP  },
	{  22,	IWM_RATE_11M_PLCP,	IWM_RATE_HT_SISO_MCS_INV_PLCP },
	{  12,	IWM_RATE_6M_PLCP,	IWM_RATE_HT_SISO_MCS_0_PLCP },
	{  18,	IWM_RATE_9M_PLCP,	IWM_RATE_HT_SISO_MCS_INV_PLCP  },
	{  24,	IWM_RATE_12M_PLCP,	IWM_RATE_HT_SISO_MCS_1_PLCP },
	{  36,	IWM_RATE_18M_PLCP,	IWM_RATE_HT_SISO_MCS_2_PLCP },
	{  48,	IWM_RATE_24M_PLCP,	IWM_RATE_HT_SISO_MCS_3_PLCP },
	{  72,	IWM_RATE_36M_PLCP,	IWM_RATE_HT_SISO_MCS_4_PLCP },
	{  96,	IWM_RATE_48M_PLCP,	IWM_RATE_HT_SISO_MCS_5_PLCP },
	{ 108,	IWM_RATE_54M_PLCP,	IWM_RATE_HT_SISO_MCS_6_PLCP },
	{ 128,	IWM_RATE_INVM_PLCP,	IWM_RATE_HT_SISO_MCS_7_PLCP },
};
#define IWM_RIDX_CCK	0
#define IWM_RIDX_OFDM	4
#define IWM_RIDX_MAX	(__arraycount(iwm_rates)-1)
#define IWM_RIDX_IS_CCK(_i_) ((_i_) < IWM_RIDX_OFDM)
#define IWM_RIDX_IS_OFDM(_i_) ((_i_) >= IWM_RIDX_OFDM)

#ifndef IEEE80211_NO_HT
/* Convert an MCS index into an iwm_rates[] index. */
static const int iwm_mcs2ridx[] = {
	IWM_RATE_MCS_0_INDEX,
	IWM_RATE_MCS_1_INDEX,
	IWM_RATE_MCS_2_INDEX,
	IWM_RATE_MCS_3_INDEX,
	IWM_RATE_MCS_4_INDEX,
	IWM_RATE_MCS_5_INDEX,
	IWM_RATE_MCS_6_INDEX,
	IWM_RATE_MCS_7_INDEX,
};
#endif

struct iwm_nvm_section {
	uint16_t length;
	uint8_t *data;
};

struct iwm_newstate_state {
	struct work ns_wk;
	enum ieee80211_state ns_nstate;
	int ns_arg;
	int ns_generation;
};

static int	iwm_store_cscheme(struct iwm_softc *, uint8_t *, size_t);
static int	iwm_firmware_store_section(struct iwm_softc *,
		    enum iwm_ucode_type, uint8_t *, size_t);
static int	iwm_set_default_calib(struct iwm_softc *, const void *);
static int	iwm_read_firmware(struct iwm_softc *, enum iwm_ucode_type);
static uint32_t iwm_read_prph(struct iwm_softc *, uint32_t);
static void	iwm_write_prph(struct iwm_softc *, uint32_t, uint32_t);
#ifdef IWM_DEBUG
static int	iwm_read_mem(struct iwm_softc *, uint32_t, void *, int);
#endif
static int	iwm_write_mem(struct iwm_softc *, uint32_t, const void *, int);
static int	iwm_write_mem32(struct iwm_softc *, uint32_t, uint32_t);
static int	iwm_poll_bit(struct iwm_softc *, int, uint32_t, uint32_t, int);
static int	iwm_nic_lock(struct iwm_softc *);
static void	iwm_nic_unlock(struct iwm_softc *);
static void	iwm_set_bits_mask_prph(struct iwm_softc *, uint32_t, uint32_t,
		    uint32_t);
static void	iwm_set_bits_prph(struct iwm_softc *, uint32_t, uint32_t);
static void	iwm_clear_bits_prph(struct iwm_softc *, uint32_t, uint32_t);
static int	iwm_dma_contig_alloc(bus_dma_tag_t, struct iwm_dma_info *,
		    bus_size_t, bus_size_t);
static void	iwm_dma_contig_free(struct iwm_dma_info *);
static int	iwm_alloc_rx_ring(struct iwm_softc *, struct iwm_rx_ring *);
static void	iwm_disable_rx_dma(struct iwm_softc *);
static void	iwm_reset_rx_ring(struct iwm_softc *, struct iwm_rx_ring *);
static void	iwm_free_rx_ring(struct iwm_softc *, struct iwm_rx_ring *);
static int	iwm_alloc_tx_ring(struct iwm_softc *, struct iwm_tx_ring *,
		    int);
static void	iwm_reset_tx_ring(struct iwm_softc *, struct iwm_tx_ring *);
static void	iwm_free_tx_ring(struct iwm_softc *, struct iwm_tx_ring *);
static void	iwm_enable_rfkill_int(struct iwm_softc *);
static int	iwm_check_rfkill(struct iwm_softc *);
static void	iwm_enable_interrupts(struct iwm_softc *);
static void	iwm_restore_interrupts(struct iwm_softc *);
static void	iwm_disable_interrupts(struct iwm_softc *);
static void	iwm_ict_reset(struct iwm_softc *);
static int	iwm_set_hw_ready(struct iwm_softc *);
static int	iwm_prepare_card_hw(struct iwm_softc *);
static void	iwm_apm_config(struct iwm_softc *);
static int	iwm_apm_init(struct iwm_softc *);
static void	iwm_apm_stop(struct iwm_softc *);
static int	iwm_allow_mcast(struct iwm_softc *);
static int	iwm_start_hw(struct iwm_softc *);
static void	iwm_stop_device(struct iwm_softc *);
static void	iwm_nic_config(struct iwm_softc *);
static int	iwm_nic_rx_init(struct iwm_softc *);
static int	iwm_nic_tx_init(struct iwm_softc *);
static int	iwm_nic_init(struct iwm_softc *);
static int	iwm_enable_txq(struct iwm_softc *, int, int, int);
static int	iwm_post_alive(struct iwm_softc *);
static struct iwm_phy_db_entry *
		iwm_phy_db_get_section(struct iwm_softc *,
		    enum iwm_phy_db_section_type, uint16_t);
static int	iwm_phy_db_set_section(struct iwm_softc *,
		    struct iwm_calib_res_notif_phy_db *, uint16_t);
static int	iwm_is_valid_channel(uint16_t);
static uint8_t	iwm_ch_id_to_ch_index(uint16_t);
static uint16_t iwm_channel_id_to_papd(uint16_t);
static uint16_t iwm_channel_id_to_txp(struct iwm_softc *, uint16_t);
static int	iwm_phy_db_get_section_data(struct iwm_softc *, uint32_t,
		    uint8_t **, uint16_t *, uint16_t);
static int	iwm_send_phy_db_cmd(struct iwm_softc *, uint16_t, uint16_t,
		    void *);
static int	iwm_phy_db_send_all_channel_groups(struct iwm_softc *,
		    enum iwm_phy_db_section_type, uint8_t);
static int	iwm_send_phy_db_data(struct iwm_softc *);
static void	iwm_te_v2_to_v1(const struct iwm_time_event_cmd_v2 *,
		    struct iwm_time_event_cmd_v1 *);
static int	iwm_send_time_event_cmd(struct iwm_softc *,
		    const struct iwm_time_event_cmd_v2 *);
static void	iwm_protect_session(struct iwm_softc *, struct iwm_node *,
		    uint32_t, uint32_t);
static int	iwm_nvm_read_chunk(struct iwm_softc *, uint16_t, uint16_t,
		    uint16_t, uint8_t *, uint16_t *);
static int	iwm_nvm_read_section(struct iwm_softc *, uint16_t, uint8_t *,
		    uint16_t *, size_t);
static void	iwm_init_channel_map(struct iwm_softc *, const uint16_t * const,
		    const uint8_t *, size_t);
#ifndef IEEE80211_NO_HT
static void	iwm_setup_ht_rates(struct iwm_softc *);
static void	iwm_htprot_task(void *);
static void	iwm_update_htprot(struct ieee80211com *,
		    struct ieee80211_node *);
static int	iwm_ampdu_rx_start(struct ieee80211com *,
		    struct ieee80211_node *, uint8_t);
static void	iwm_ampdu_rx_stop(struct ieee80211com *,
		    struct ieee80211_node *, uint8_t);
static void	iwm_sta_rx_agg(struct iwm_softc *, struct ieee80211_node *,
		    uint8_t, uint16_t, int);
#ifdef notyet
static int	iwm_ampdu_tx_start(struct ieee80211com *,
		    struct ieee80211_node *, uint8_t);
static void	iwm_ampdu_tx_stop(struct ieee80211com *,
		    struct ieee80211_node *, uint8_t);
#endif
static void	iwm_ba_task(void *);
#endif

static int	iwm_parse_nvm_data(struct iwm_softc *, const uint16_t *,
		    const uint16_t *, const uint16_t *, const uint16_t *,
		    const uint16_t *, const uint16_t *);
static void	iwm_set_hw_address_8000(struct iwm_softc *,
		    struct iwm_nvm_data *, const uint16_t *, const uint16_t *);
static int	iwm_parse_nvm_sections(struct iwm_softc *,
		    struct iwm_nvm_section *);
static int	iwm_nvm_init(struct iwm_softc *);
static int	iwm_firmware_load_sect(struct iwm_softc *, uint32_t,
		    const uint8_t *, uint32_t);
static int	iwm_firmware_load_chunk(struct iwm_softc *, uint32_t,
		    const uint8_t *, uint32_t);
static int	iwm_load_cpu_sections_7000(struct iwm_softc *,
		    struct iwm_fw_sects *, int , int *);
static int	iwm_load_firmware_7000(struct iwm_softc *, enum iwm_ucode_type);
static int	iwm_load_cpu_sections_8000(struct iwm_softc *,
		    struct iwm_fw_sects *, int , int *);
static int	iwm_load_firmware_8000(struct iwm_softc *, enum iwm_ucode_type);
static int	iwm_load_firmware(struct iwm_softc *, enum iwm_ucode_type);
static int	iwm_start_fw(struct iwm_softc *, enum iwm_ucode_type);
static int	iwm_send_tx_ant_cfg(struct iwm_softc *, uint8_t);
static int	iwm_send_phy_cfg_cmd(struct iwm_softc *);
static int	iwm_load_ucode_wait_alive(struct iwm_softc *,
		    enum iwm_ucode_type);
static int	iwm_run_init_mvm_ucode(struct iwm_softc *, int);
static int	iwm_rx_addbuf(struct iwm_softc *, int, int);
static int	iwm_calc_rssi(struct iwm_softc *, struct iwm_rx_phy_info *);
static int	iwm_get_signal_strength(struct iwm_softc *,
		    struct iwm_rx_phy_info *);
static void	iwm_rx_rx_phy_cmd(struct iwm_softc *,
		    struct iwm_rx_packet *, struct iwm_rx_data *);
static int	iwm_get_noise(const struct iwm_statistics_rx_non_phy *);
static void	iwm_rx_rx_mpdu(struct iwm_softc *, struct iwm_rx_packet *,
		    struct iwm_rx_data *);
static void	iwm_rx_tx_cmd_single(struct iwm_softc *, struct iwm_rx_packet *,		    struct iwm_node *);
static void	iwm_rx_tx_cmd(struct iwm_softc *, struct iwm_rx_packet *,
		    struct iwm_rx_data *);
static int	iwm_binding_cmd(struct iwm_softc *, struct iwm_node *,
		    uint32_t);
#if 0
static int	iwm_binding_update(struct iwm_softc *, struct iwm_node *, int);
static int	iwm_binding_add_vif(struct iwm_softc *, struct iwm_node *);
#endif
static void	iwm_phy_ctxt_cmd_hdr(struct iwm_softc *, struct iwm_phy_ctxt *,
		    struct iwm_phy_context_cmd *, uint32_t, uint32_t);
static void	iwm_phy_ctxt_cmd_data(struct iwm_softc *,
		    struct iwm_phy_context_cmd *, struct ieee80211_channel *,
		    uint8_t, uint8_t);
static int	iwm_phy_ctxt_cmd(struct iwm_softc *, struct iwm_phy_ctxt *,
		    uint8_t, uint8_t, uint32_t, uint32_t);
static int	iwm_send_cmd(struct iwm_softc *, struct iwm_host_cmd *);
static int	iwm_send_cmd_pdu(struct iwm_softc *, uint32_t, uint32_t,
		    uint16_t, const void *);
static int	iwm_send_cmd_status(struct iwm_softc *, struct iwm_host_cmd *,
		    uint32_t *);
static int	iwm_send_cmd_pdu_status(struct iwm_softc *, uint32_t, uint16_t,
		    const void *, uint32_t *);
static void	iwm_free_resp(struct iwm_softc *, struct iwm_host_cmd *);
static void	iwm_cmd_done(struct iwm_softc *, int qid, int idx);
#if 0
static void	iwm_update_sched(struct iwm_softc *, int, int, uint8_t,
		    uint16_t);
#endif
static const struct iwm_rate *
		iwm_tx_fill_cmd(struct iwm_softc *, struct iwm_node *,
		    struct ieee80211_frame *, struct iwm_tx_cmd *);
static int	iwm_tx(struct iwm_softc *, struct mbuf *,
		    struct ieee80211_node *, int);
static void	iwm_led_enable(struct iwm_softc *);
static void	iwm_led_disable(struct iwm_softc *);
static int	iwm_led_is_enabled(struct iwm_softc *);
static void	iwm_led_blink_timeout(void *);
static void	iwm_led_blink_start(struct iwm_softc *);
static void	iwm_led_blink_stop(struct iwm_softc *);
static int	iwm_beacon_filter_send_cmd(struct iwm_softc *,
		    struct iwm_beacon_filter_cmd *);
static void	iwm_beacon_filter_set_cqm_params(struct iwm_softc *,
		    struct iwm_node *, struct iwm_beacon_filter_cmd *);
static int	iwm_update_beacon_abort(struct iwm_softc *, struct iwm_node *,
		    int);
static void	iwm_power_build_cmd(struct iwm_softc *, struct iwm_node *,
		    struct iwm_mac_power_cmd *);
static int	iwm_power_mac_update_mode(struct iwm_softc *,
		    struct iwm_node *);
static int	iwm_power_update_device(struct iwm_softc *);
#ifdef notyet
static int	iwm_enable_beacon_filter(struct iwm_softc *, struct iwm_node *);
#endif
static int	iwm_disable_beacon_filter(struct iwm_softc *);
static int	iwm_add_sta_cmd(struct iwm_softc *, struct iwm_node *, int);
static int	iwm_add_aux_sta(struct iwm_softc *);
static uint16_t iwm_scan_rx_chain(struct iwm_softc *);
static uint32_t iwm_scan_rate_n_flags(struct iwm_softc *, int, int);
#ifdef notyet
static uint16_t iwm_get_active_dwell(struct iwm_softc *, int, int);
static uint16_t iwm_get_passive_dwell(struct iwm_softc *, int);
#endif
static uint8_t	iwm_lmac_scan_fill_channels(struct iwm_softc *,
		    struct iwm_scan_channel_cfg_lmac *, int);
static int	iwm_fill_probe_req(struct iwm_softc *,
		    struct iwm_scan_probe_req *);
static int	iwm_lmac_scan(struct iwm_softc *);
static int	iwm_config_umac_scan(struct iwm_softc *);
static int	iwm_umac_scan(struct iwm_softc *);
static uint8_t	iwm_ridx2rate(struct ieee80211_rateset *, int);
static void	iwm_ack_rates(struct iwm_softc *, struct iwm_node *, int *,
		    int *);
static void	iwm_mac_ctxt_cmd_common(struct iwm_softc *, struct iwm_node *,
		    struct iwm_mac_ctx_cmd *, uint32_t, int);
static void	iwm_mac_ctxt_cmd_fill_sta(struct iwm_softc *, struct iwm_node *,
		    struct iwm_mac_data_sta *, int);
static int	iwm_mac_ctxt_cmd(struct iwm_softc *, struct iwm_node *,
		    uint32_t, int);
static int	iwm_update_quotas(struct iwm_softc *, struct iwm_node *);
static int	iwm_auth(struct iwm_softc *);
static int	iwm_assoc(struct iwm_softc *);
static void	iwm_calib_timeout(void *);
#ifndef IEEE80211_NO_HT
static void	iwm_setrates_task(void *);
static int	iwm_setrates(struct iwm_node *);
#endif
static int	iwm_media_change(struct ifnet *);
static int	iwm_do_newstate(struct ieee80211com *, enum ieee80211_state,
		    int);
static void	iwm_newstate_cb(struct work *, void *);
static int	iwm_newstate(struct ieee80211com *, enum ieee80211_state, int);
static void	iwm_endscan(struct iwm_softc *);
static void	iwm_fill_sf_command(struct iwm_softc *, struct iwm_sf_cfg_cmd *,
		    struct ieee80211_node *);
static int	iwm_sf_config(struct iwm_softc *, int);
static int	iwm_send_bt_init_conf(struct iwm_softc *);
static int	iwm_send_update_mcc_cmd(struct iwm_softc *, const char *);
static void	iwm_tt_tx_backoff(struct iwm_softc *, uint32_t);
static int	iwm_init_hw(struct iwm_softc *);
static int	iwm_init(struct ifnet *);
static void	iwm_start(struct ifnet *);
static void	iwm_stop(struct ifnet *, int);
static void	iwm_watchdog(struct ifnet *);
static int	iwm_ioctl(struct ifnet *, u_long, void *);
#ifdef IWM_DEBUG
static const char *iwm_desc_lookup(uint32_t);
static void	iwm_nic_error(struct iwm_softc *);
static void	iwm_nic_umac_error(struct iwm_softc *);
#endif
static void	iwm_notif_intr(struct iwm_softc *);
static int	iwm_intr(void *);
static void	iwm_softintr(void *);
static int	iwm_preinit(struct iwm_softc *);
static void	iwm_attach_hook(device_t);
static void	iwm_attach(device_t, device_t, void *);
#if 0
static void	iwm_init_task(void *);
static int	iwm_activate(device_t, enum devact);
static void	iwm_wakeup(struct iwm_softc *);
#endif
static void	iwm_radiotap_attach(struct iwm_softc *);
static int	iwm_sysctl_fw_loaded_handler(SYSCTLFN_PROTO);

static int iwm_sysctl_root_num;
static int iwm_lar_disable;

#ifndef	IWM_DEFAULT_MCC
#define	IWM_DEFAULT_MCC	"ZZ"
#endif
static char iwm_default_mcc[3] = IWM_DEFAULT_MCC;

static int
iwm_firmload(struct iwm_softc *sc)
{
	struct iwm_fw_info *fw = &sc->sc_fw;
	firmware_handle_t fwh;
	int err;

	if (ISSET(sc->sc_flags, IWM_FLAG_FW_LOADED))
		return 0;

	/* Open firmware image. */
	err = firmware_open("if_iwm", sc->sc_fwname, &fwh);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not get firmware handle %s\n", sc->sc_fwname);
		return err;
	}

	if (fw->fw_rawdata != NULL && fw->fw_rawsize > 0) {
		kmem_free(fw->fw_rawdata, fw->fw_rawsize);
		fw->fw_rawdata = NULL;
	}

	fw->fw_rawsize = firmware_get_size(fwh);
	/*
	 * Well, this is how the Linux driver checks it ....
	 */
	if (fw->fw_rawsize < sizeof(uint32_t)) {
		aprint_error_dev(sc->sc_dev,
		    "firmware too short: %zd bytes\n", fw->fw_rawsize);
		err = EINVAL;
		goto out;
	}

	/* Read the firmware. */
	fw->fw_rawdata = kmem_alloc(fw->fw_rawsize, KM_SLEEP);
	err = firmware_read(fwh, 0, fw->fw_rawdata, fw->fw_rawsize);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not read firmware %s\n", sc->sc_fwname);
		goto out;
	}

	SET(sc->sc_flags, IWM_FLAG_FW_LOADED);
 out:
	/* caller will release memory, if necessary */

	firmware_close(fwh);
	return err;
}

/*
 * just maintaining status quo.
 */
static void
iwm_fix_channel(struct iwm_softc *sc, struct mbuf *m)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_frame *wh;
	uint8_t subtype;

	wh = mtod(m, struct ieee80211_frame *);

	if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT)
		return;

	subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

	if (subtype != IEEE80211_FC0_SUBTYPE_BEACON &&
	    subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP)
		return;

	int chan = le32toh(sc->sc_last_phy_info.channel);
	if (chan < __arraycount(ic->ic_channels))
		ic->ic_curchan = &ic->ic_channels[chan];
}

static int
iwm_store_cscheme(struct iwm_softc *sc, uint8_t *data, size_t dlen)
{
	struct iwm_fw_cscheme_list *l = (struct iwm_fw_cscheme_list *)data;

	if (dlen < sizeof(*l) ||
	    dlen < sizeof(l->size) + l->size * sizeof(*l->cs))
		return EINVAL;

	/* we don't actually store anything for now, always use s/w crypto */

	return 0;
}

static int
iwm_firmware_store_section(struct iwm_softc *sc, enum iwm_ucode_type type,
    uint8_t *data, size_t dlen)
{
	struct iwm_fw_sects *fws;
	struct iwm_fw_onesect *fwone;

	if (type >= IWM_UCODE_TYPE_MAX)
		return EINVAL;
	if (dlen < sizeof(uint32_t))
		return EINVAL;

	fws = &sc->sc_fw.fw_sects[type];
	if (fws->fw_count >= IWM_UCODE_SECT_MAX)
		return EINVAL;

	fwone = &fws->fw_sect[fws->fw_count];

	/* first 32bit are device load offset */
	memcpy(&fwone->fws_devoff, data, sizeof(uint32_t));

	/* rest is data */
	fwone->fws_data = data + sizeof(uint32_t);
	fwone->fws_len = dlen - sizeof(uint32_t);

	/* for freeing the buffer during driver unload */
	fwone->fws_alloc = data;
	fwone->fws_allocsize = dlen;

	fws->fw_count++;
	fws->fw_totlen += fwone->fws_len;

	return 0;
}

struct iwm_tlv_calib_data {
	uint32_t ucode_type;
	struct iwm_tlv_calib_ctrl calib;
} __packed;

static int
iwm_set_default_calib(struct iwm_softc *sc, const void *data)
{
	const struct iwm_tlv_calib_data *def_calib = data;
	uint32_t ucode_type = le32toh(def_calib->ucode_type);

	if (ucode_type >= IWM_UCODE_TYPE_MAX) {
		DPRINTF(("%s: Wrong ucode_type %u for default calibration.\n",
		    DEVNAME(sc), ucode_type));
		return EINVAL;
	}

	sc->sc_default_calib[ucode_type].flow_trigger =
	    def_calib->calib.flow_trigger;
	sc->sc_default_calib[ucode_type].event_trigger =
	    def_calib->calib.event_trigger;

	return 0;
}

static int
iwm_read_firmware(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	struct iwm_fw_info *fw = &sc->sc_fw;
	struct iwm_tlv_ucode_header *uhdr;
	struct iwm_ucode_tlv tlv;
	enum iwm_ucode_tlv_type tlv_type;
	uint8_t *data;
	int err, status;
	size_t len;

	if (ucode_type != IWM_UCODE_TYPE_INIT &&
	    fw->fw_status == IWM_FW_STATUS_DONE)
		return 0;

	if (fw->fw_status == IWM_FW_STATUS_NONE) {
		fw->fw_status = IWM_FW_STATUS_INPROGRESS;
	} else {
		while (fw->fw_status == IWM_FW_STATUS_INPROGRESS)
			tsleep(&sc->sc_fw, 0, "iwmfwp", 0);
	}
	status = fw->fw_status;

	if (status == IWM_FW_STATUS_DONE)
		return 0;

	err = iwm_firmload(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not read firmware %s (error %d)\n",
		    sc->sc_fwname, err);
		goto out;
	}

	sc->sc_capaflags = 0;
	sc->sc_capa_n_scan_channels = IWM_MAX_NUM_SCAN_CHANNELS;
	memset(sc->sc_enabled_capa, 0, sizeof(sc->sc_enabled_capa));
	memset(sc->sc_fw_mcc, 0, sizeof(sc->sc_fw_mcc));

	uhdr = (void *)fw->fw_rawdata;
	if (*(uint32_t *)fw->fw_rawdata != 0
	    || le32toh(uhdr->magic) != IWM_TLV_UCODE_MAGIC) {
		aprint_error_dev(sc->sc_dev, "invalid firmware %s\n",
		    sc->sc_fwname);
		err = EINVAL;
		goto out;
	}

	snprintf(sc->sc_fwver, sizeof(sc->sc_fwver), "%d.%d (API ver %d)",
	    IWM_UCODE_MAJOR(le32toh(uhdr->ver)),
	    IWM_UCODE_MINOR(le32toh(uhdr->ver)),
	    IWM_UCODE_API(le32toh(uhdr->ver)));
	data = uhdr->data;
	len = fw->fw_rawsize - sizeof(*uhdr);

	while (len >= sizeof(tlv)) {
		size_t tlv_len;
		void *tlv_data;

		memcpy(&tlv, data, sizeof(tlv));
		tlv_len = le32toh(tlv.length);
		tlv_type = le32toh(tlv.type);

		len -= sizeof(tlv);
		data += sizeof(tlv);
		tlv_data = data;

		if (len < tlv_len) {
			aprint_error_dev(sc->sc_dev,
			    "firmware too short: %zu bytes\n", len);
			err = EINVAL;
			goto parse_out;
		}

		switch (tlv_type) {
		case IWM_UCODE_TLV_PROBE_MAX_LEN:
			if (tlv_len < sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			sc->sc_capa_max_probe_len
			    = le32toh(*(uint32_t *)tlv_data);
			/* limit it to something sensible */
			if (sc->sc_capa_max_probe_len >
			    IWM_SCAN_OFFLOAD_PROBE_REQ_SIZE) {
				err = EINVAL;
				goto parse_out;
			}
			break;
		case IWM_UCODE_TLV_PAN:
			if (tlv_len) {
				err = EINVAL;
				goto parse_out;
			}
			sc->sc_capaflags |= IWM_UCODE_TLV_FLAGS_PAN;
			break;
		case IWM_UCODE_TLV_FLAGS:
			if (tlv_len < sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			if (tlv_len % sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			/*
			 * Apparently there can be many flags, but Linux driver
			 * parses only the first one, and so do we.
			 *
			 * XXX: why does this override IWM_UCODE_TLV_PAN?
			 * Intentional or a bug?  Observations from
			 * current firmware file:
			 *  1) TLV_PAN is parsed first
			 *  2) TLV_FLAGS contains TLV_FLAGS_PAN
			 * ==> this resets TLV_PAN to itself... hnnnk
			 */
			sc->sc_capaflags = le32toh(*(uint32_t *)tlv_data);
			break;
		case IWM_UCODE_TLV_CSCHEME:
			err = iwm_store_cscheme(sc, tlv_data, tlv_len);
			if (err)
				goto parse_out;
			break;
		case IWM_UCODE_TLV_NUM_OF_CPU: {
			uint32_t num_cpu;
			if (tlv_len != sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			num_cpu = le32toh(*(uint32_t *)tlv_data);
			if (num_cpu == 2) {
				fw->fw_sects[IWM_UCODE_TYPE_REGULAR].is_dual_cpus =
				    true;
				fw->fw_sects[IWM_UCODE_TYPE_INIT].is_dual_cpus =
				    true;
				fw->fw_sects[IWM_UCODE_TYPE_WOW].is_dual_cpus =
				    true;
			} else if (num_cpu < 1 || num_cpu > 2) {
				err = EINVAL;
				goto parse_out;
			}
			break;
		}
		case IWM_UCODE_TLV_SEC_RT:
			err = iwm_firmware_store_section(sc,
			    IWM_UCODE_TYPE_REGULAR, tlv_data, tlv_len);
			if (err)
				goto parse_out;
			break;
		case IWM_UCODE_TLV_SEC_INIT:
			err = iwm_firmware_store_section(sc,
			    IWM_UCODE_TYPE_INIT, tlv_data, tlv_len);
			if (err)
				goto parse_out;
			break;
		case IWM_UCODE_TLV_SEC_WOWLAN:
			err = iwm_firmware_store_section(sc,
			    IWM_UCODE_TYPE_WOW, tlv_data, tlv_len);
			if (err)
				goto parse_out;
			break;
		case IWM_UCODE_TLV_DEF_CALIB:
			if (tlv_len != sizeof(struct iwm_tlv_calib_data)) {
				err = EINVAL;
				goto parse_out;
			}
			err = iwm_set_default_calib(sc, tlv_data);
			if (err)
				goto parse_out;
			break;
		case IWM_UCODE_TLV_PHY_SKU:
			if (tlv_len != sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			sc->sc_fw_phy_config = le32toh(*(uint32_t *)tlv_data);
			break;

		case IWM_UCODE_TLV_API_CHANGES_SET: {
			struct iwm_ucode_api *api;
			uint32_t idx, bits;
			int i;
			if (tlv_len != sizeof(*api)) {
				err = EINVAL;
				goto parse_out;
			}
			api = (struct iwm_ucode_api *)tlv_data;
			idx = le32toh(api->api_index);
			bits = le32toh(api->api_flags);
			if (idx >= howmany(IWM_NUM_UCODE_TLV_API, 32)) {
				err = EINVAL;
				goto parse_out;
			}
			for (i = 0; i < 32; i++) {
				if (!ISSET(bits, __BIT(i)))
					continue;
				setbit(sc->sc_ucode_api, i + (32 * idx));
			}
			break;
		}

		case IWM_UCODE_TLV_ENABLED_CAPABILITIES: {
			struct iwm_ucode_capa *capa;
			uint32_t idx, bits;
			int i;
			if (tlv_len != sizeof(*capa)) {
				err = EINVAL;
				goto parse_out;
			}
			capa = (struct iwm_ucode_capa *)tlv_data;
			idx = le32toh(capa->api_index);
			bits = le32toh(capa->api_capa);
			if (idx >= howmany(IWM_NUM_UCODE_TLV_CAPA, 32)) {
				err = EINVAL;
				goto parse_out;
			}
			for (i = 0; i < 32; i++) {
				if (!ISSET(bits, __BIT(i)))
					continue;
				setbit(sc->sc_enabled_capa, i + (32 * idx));
			}
			break;
		}

		case IWM_UCODE_TLV_FW_UNDOCUMENTED1:
		case IWM_UCODE_TLV_SDIO_ADMA_ADDR:
		case IWM_UCODE_TLV_FW_GSCAN_CAPA:
		case IWM_UCODE_TLV_FW_MEM_SEG:
			/* ignore, not used by current driver */
			break;

		case IWM_UCODE_TLV_SEC_RT_USNIFFER:
			err = iwm_firmware_store_section(sc,
			    IWM_UCODE_TYPE_REGULAR_USNIFFER, tlv_data,
			    tlv_len);
			if (err)
				goto parse_out;
			break;

		case IWM_UCODE_TLV_PAGING: {
			uint32_t paging_mem_size;
			if (tlv_len != sizeof(paging_mem_size)) {
				err = EINVAL;
				goto parse_out;
			}
			paging_mem_size = le32toh(*(uint32_t *)tlv_data);
			if (paging_mem_size > IWM_MAX_PAGING_IMAGE_SIZE) {
				err = EINVAL;
				goto parse_out;
			}
			if (paging_mem_size & (IWM_FW_PAGING_SIZE - 1)) {
				err = EINVAL;
				goto parse_out;
			}
			fw->fw_sects[IWM_UCODE_TYPE_REGULAR].paging_mem_size =
			    paging_mem_size;
			fw->fw_sects[IWM_UCODE_TYPE_REGULAR_USNIFFER].paging_mem_size =
			    paging_mem_size;
			break;
		}

		case IWM_UCODE_TLV_N_SCAN_CHANNELS:
			if (tlv_len != sizeof(uint32_t)) {
				err = EINVAL;
				goto parse_out;
			}
			sc->sc_capa_n_scan_channels =
			  le32toh(*(uint32_t *)tlv_data);
			break;

		case IWM_UCODE_TLV_FW_VERSION:
			if (tlv_len != sizeof(uint32_t) * 3) {
				err = EINVAL;
				goto parse_out;
			}
			snprintf(sc->sc_fwver, sizeof(sc->sc_fwver),
			    "%d.%d.%d",
			    le32toh(((uint32_t *)tlv_data)[0]),
			    le32toh(((uint32_t *)tlv_data)[1]),
			    le32toh(((uint32_t *)tlv_data)[2]));
			break;

		default:
			DPRINTF(("%s: unknown firmware section %d, abort\n",
			    DEVNAME(sc), tlv_type));
			err = EINVAL;
			goto parse_out;
		}

		len -= roundup(tlv_len, 4);
		data += roundup(tlv_len, 4);
	}

	KASSERT(err == 0);

 parse_out:
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "firmware parse error, section type %d\n", tlv_type);
	}

	if (!(sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_PM_CMD_SUPPORT)) {
		aprint_error_dev(sc->sc_dev,
		    "device uses unsupported power ops\n");
		err = ENOTSUP;
	}

 out:
	if (err)
		fw->fw_status = IWM_FW_STATUS_NONE;
	else
		fw->fw_status = IWM_FW_STATUS_DONE;
	wakeup(&sc->sc_fw);

	if (err && fw->fw_rawdata != NULL) {
		kmem_free(fw->fw_rawdata, fw->fw_rawsize);
		fw->fw_rawdata = NULL;
		CLR(sc->sc_flags, IWM_FLAG_FW_LOADED);
		/* don't touch fw->fw_status */
		memset(fw->fw_sects, 0, sizeof(fw->fw_sects));
	}
	return err;
}

static uint32_t
iwm_read_prph(struct iwm_softc *sc, uint32_t addr)
{
	IWM_WRITE(sc,
	    IWM_HBUS_TARG_PRPH_RADDR, ((addr & 0x000fffff) | (3 << 24)));
	IWM_BARRIER_READ_WRITE(sc);
	return IWM_READ(sc, IWM_HBUS_TARG_PRPH_RDAT);
}

static void
iwm_write_prph(struct iwm_softc *sc, uint32_t addr, uint32_t val)
{
	IWM_WRITE(sc,
	    IWM_HBUS_TARG_PRPH_WADDR, ((addr & 0x000fffff) | (3 << 24)));
	IWM_BARRIER_WRITE(sc);
	IWM_WRITE(sc, IWM_HBUS_TARG_PRPH_WDAT, val);
}

#ifdef IWM_DEBUG
static int
iwm_read_mem(struct iwm_softc *sc, uint32_t addr, void *buf, int dwords)
{
	int offs;
	uint32_t *vals = buf;

	if (iwm_nic_lock(sc)) {
		IWM_WRITE(sc, IWM_HBUS_TARG_MEM_RADDR, addr);
		for (offs = 0; offs < dwords; offs++)
			vals[offs] = IWM_READ(sc, IWM_HBUS_TARG_MEM_RDAT);
		iwm_nic_unlock(sc);
		return 0;
	}
	return EBUSY;
}
#endif

static int
iwm_write_mem(struct iwm_softc *sc, uint32_t addr, const void *buf, int dwords)
{
	int offs;
	const uint32_t *vals = buf;

	if (iwm_nic_lock(sc)) {
		IWM_WRITE(sc, IWM_HBUS_TARG_MEM_WADDR, addr);
		/* WADDR auto-increments */
		for (offs = 0; offs < dwords; offs++) {
			uint32_t val = vals ? vals[offs] : 0;
			IWM_WRITE(sc, IWM_HBUS_TARG_MEM_WDAT, val);
		}
		iwm_nic_unlock(sc);
		return 0;
	}
	return EBUSY;
}

static int
iwm_write_mem32(struct iwm_softc *sc, uint32_t addr, uint32_t val)
{
	return iwm_write_mem(sc, addr, &val, 1);
}

static int
iwm_poll_bit(struct iwm_softc *sc, int reg, uint32_t bits, uint32_t mask,
    int timo)
{
	for (;;) {
		if ((IWM_READ(sc, reg) & mask) == (bits & mask)) {
			return 1;
		}
		if (timo < 10) {
			return 0;
		}
		timo -= 10;
		DELAY(10);
	}
}

static int
iwm_nic_lock(struct iwm_softc *sc)
{
	int rv = 0;

	if (sc->sc_cmd_hold_nic_awake)
		return 1;

	IWM_SETBITS(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000)
		DELAY(2);

	if (iwm_poll_bit(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY
	     | IWM_CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP, 15000)) {
		rv = 1;
	} else {
		DPRINTF(("%s: resetting device via NMI\n", DEVNAME(sc)));
		IWM_WRITE(sc, IWM_CSR_RESET, IWM_CSR_RESET_REG_FLAG_FORCE_NMI);
	}

	return rv;
}

static void
iwm_nic_unlock(struct iwm_softc *sc)
{

	if (sc->sc_cmd_hold_nic_awake)
		return;

	IWM_CLRBITS(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
}

static void
iwm_set_bits_mask_prph(struct iwm_softc *sc, uint32_t reg, uint32_t bits,
    uint32_t mask)
{
	uint32_t val;

	/* XXX: no error path? */
	if (iwm_nic_lock(sc)) {
		val = iwm_read_prph(sc, reg) & mask;
		val |= bits;
		iwm_write_prph(sc, reg, val);
		iwm_nic_unlock(sc);
	}
}

static void
iwm_set_bits_prph(struct iwm_softc *sc, uint32_t reg, uint32_t bits)
{
	iwm_set_bits_mask_prph(sc, reg, bits, ~0);
}

static void
iwm_clear_bits_prph(struct iwm_softc *sc, uint32_t reg, uint32_t bits)
{
	iwm_set_bits_mask_prph(sc, reg, 0, ~bits);
}

static int
iwm_dma_contig_alloc(bus_dma_tag_t tag, struct iwm_dma_info *dma,
    bus_size_t size, bus_size_t alignment)
{
	int nsegs, err;
	void *va;

	dma->tag = tag;
	dma->size = size;

	err = bus_dmamap_create(tag, size, 1, size, 0, BUS_DMA_NOWAIT,
	    &dma->map);
	if (err)
		goto fail;

	err = bus_dmamem_alloc(tag, size, alignment, 0, &dma->seg, 1, &nsegs,
	    BUS_DMA_NOWAIT);
	if (err)
		goto fail;

	err = bus_dmamem_map(tag, &dma->seg, 1, size, &va, BUS_DMA_NOWAIT);
	if (err)
		goto fail;
	dma->vaddr = va;

	err = bus_dmamap_load(tag, dma->map, dma->vaddr, size, NULL,
	    BUS_DMA_NOWAIT);
	if (err)
		goto fail;

	memset(dma->vaddr, 0, size);
	bus_dmamap_sync(tag, dma->map, 0, size, BUS_DMASYNC_PREWRITE);
	dma->paddr = dma->map->dm_segs[0].ds_addr;

	return 0;

fail:	iwm_dma_contig_free(dma);
	return err;
}

static void
iwm_dma_contig_free(struct iwm_dma_info *dma)
{
	if (dma->map != NULL) {
		if (dma->vaddr != NULL) {
			bus_dmamap_sync(dma->tag, dma->map, 0, dma->size,
			    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
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
iwm_alloc_rx_ring(struct iwm_softc *sc, struct iwm_rx_ring *ring)
{
	bus_size_t size;
	int i, err;

	ring->cur = 0;

	/* Allocate RX descriptors (256-byte aligned). */
	size = IWM_RX_RING_COUNT * sizeof(uint32_t);
	err = iwm_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma, size, 256);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate RX ring DMA memory\n");
		goto fail;
	}
	ring->desc = ring->desc_dma.vaddr;

	/* Allocate RX status area (16-byte aligned). */
	err = iwm_dma_contig_alloc(sc->sc_dmat, &ring->stat_dma,
	    sizeof(*ring->stat), 16);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate RX status DMA memory\n");
		goto fail;
	}
	ring->stat = ring->stat_dma.vaddr;

	for (i = 0; i < IWM_RX_RING_COUNT; i++) {
		struct iwm_rx_data *data = &ring->data[i];

		memset(data, 0, sizeof(*data));
		err = bus_dmamap_create(sc->sc_dmat, IWM_RBUF_SIZE, 1,
		    IWM_RBUF_SIZE, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &data->map);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not create RX buf DMA map\n");
			goto fail;
		}

		err = iwm_rx_addbuf(sc, IWM_RBUF_SIZE, i);
		if (err)
			goto fail;
	}
	return 0;

fail:	iwm_free_rx_ring(sc, ring);
	return err;
}

static void
iwm_disable_rx_dma(struct iwm_softc *sc)
{
	int ntries;

	if (iwm_nic_lock(sc)) {
		IWM_WRITE(sc, IWM_FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
		for (ntries = 0; ntries < 1000; ntries++) {
			if (IWM_READ(sc, IWM_FH_MEM_RSSR_RX_STATUS_REG) &
			    IWM_FH_RSSR_CHNL0_RX_STATUS_CHNL_IDLE)
				break;
			DELAY(10);
		}
		iwm_nic_unlock(sc);
	}
}

void
iwm_reset_rx_ring(struct iwm_softc *sc, struct iwm_rx_ring *ring)
{
	ring->cur = 0;
	memset(ring->stat, 0, sizeof(*ring->stat));
	bus_dmamap_sync(sc->sc_dmat, ring->stat_dma.map, 0,
	    ring->stat_dma.size, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

static void
iwm_free_rx_ring(struct iwm_softc *sc, struct iwm_rx_ring *ring)
{
	int i;

	iwm_dma_contig_free(&ring->desc_dma);
	iwm_dma_contig_free(&ring->stat_dma);

	for (i = 0; i < IWM_RX_RING_COUNT; i++) {
		struct iwm_rx_data *data = &ring->data[i];

		if (data->m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(sc->sc_dmat, data->map);
			m_freem(data->m);
			data->m = NULL;
		}
		if (data->map != NULL) {
			bus_dmamap_destroy(sc->sc_dmat, data->map);
			data->map = NULL;
		}
	}
}

static int
iwm_alloc_tx_ring(struct iwm_softc *sc, struct iwm_tx_ring *ring, int qid)
{
	bus_addr_t paddr;
	bus_size_t size;
	int i, err, nsegs;

	ring->qid = qid;
	ring->queued = 0;
	ring->cur = 0;

	/* Allocate TX descriptors (256-byte aligned). */
	size = IWM_TX_RING_COUNT * sizeof (struct iwm_tfd);
	err = iwm_dma_contig_alloc(sc->sc_dmat, &ring->desc_dma, size, 256);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate TX ring DMA memory\n");
		goto fail;
	}
	ring->desc = ring->desc_dma.vaddr;

	/*
	 * We only use rings 0 through 9 (4 EDCA + cmd) so there is no need
	 * to allocate commands space for other rings.
	 */
	if (qid > IWM_CMD_QUEUE)
		return 0;

	size = IWM_TX_RING_COUNT * sizeof(struct iwm_device_cmd);
	err = iwm_dma_contig_alloc(sc->sc_dmat, &ring->cmd_dma, size, 4);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate TX cmd DMA memory\n");
		goto fail;
	}
	ring->cmd = ring->cmd_dma.vaddr;

	paddr = ring->cmd_dma.paddr;
	for (i = 0; i < IWM_TX_RING_COUNT; i++) {
		struct iwm_tx_data *data = &ring->data[i];
		size_t mapsize;

		data->cmd_paddr = paddr;
		data->scratch_paddr = paddr + sizeof(struct iwm_cmd_header)
		    + offsetof(struct iwm_tx_cmd, scratch);
		paddr += sizeof(struct iwm_device_cmd);

		/* FW commands may require more mapped space than packets. */
		if (qid == IWM_CMD_QUEUE) {
			mapsize = IWM_RBUF_SIZE;
			nsegs = 1;
		} else {
			mapsize = MCLBYTES;
			nsegs = IWM_NUM_OF_TBS - 2;
		}
		err = bus_dmamap_create(sc->sc_dmat, mapsize, nsegs, mapsize,
		    0, BUS_DMA_NOWAIT, &data->map);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not create TX buf DMA map\n");
			goto fail;
		}
	}
	KASSERT(paddr == ring->cmd_dma.paddr + size);
	return 0;

fail:	iwm_free_tx_ring(sc, ring);
	return err;
}

static void
iwm_clear_cmd_in_flight(struct iwm_softc *sc)
{

	if (!sc->apmg_wake_up_wa)
		return;

	if (!sc->sc_cmd_hold_nic_awake) {
		aprint_error_dev(sc->sc_dev,
		    "cmd_hold_nic_awake not set\n");
		return;
	}

	sc->sc_cmd_hold_nic_awake = 0;
	IWM_CLRBITS(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
}

static int
iwm_set_cmd_in_flight(struct iwm_softc *sc)
{
	int ret;

	/*
	 * wake up the NIC to make sure that the firmware will see the host
	 * command - we will let the NIC sleep once all the host commands
	 * returned. This needs to be done only on NICs that have
	 * apmg_wake_up_wa set.
	 */
	if (sc->apmg_wake_up_wa && !sc->sc_cmd_hold_nic_awake) {

		IWM_SETBITS(sc, IWM_CSR_GP_CNTRL,
		    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);

		ret = iwm_poll_bit(sc, IWM_CSR_GP_CNTRL,
		    IWM_CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
		    (IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
		     IWM_CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP),
		    15000);
		if (ret == 0) {
			IWM_CLRBITS(sc, IWM_CSR_GP_CNTRL,
			    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
			aprint_error_dev(sc->sc_dev,
			    "failed to wake NIC for hcmd\n");
			return EIO;
		}
		sc->sc_cmd_hold_nic_awake = 1;
	}

	return 0;
}
static void
iwm_reset_tx_ring(struct iwm_softc *sc, struct iwm_tx_ring *ring)
{
	int i;

	for (i = 0; i < IWM_TX_RING_COUNT; i++) {
		struct iwm_tx_data *data = &ring->data[i];

		if (data->m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, data->map);
			m_freem(data->m);
			data->m = NULL;
		}
	}
	/* Clear TX descriptors. */
	memset(ring->desc, 0, ring->desc_dma.size);
	bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map, 0,
	    ring->desc_dma.size, BUS_DMASYNC_PREWRITE);
	sc->qfullmsk &= ~(1 << ring->qid);
	ring->queued = 0;
	ring->cur = 0;

	if (ring->qid == IWM_CMD_QUEUE && sc->sc_cmd_hold_nic_awake)
		iwm_clear_cmd_in_flight(sc);
}

static void
iwm_free_tx_ring(struct iwm_softc *sc, struct iwm_tx_ring *ring)
{
	int i;

	iwm_dma_contig_free(&ring->desc_dma);
	iwm_dma_contig_free(&ring->cmd_dma);

	for (i = 0; i < IWM_TX_RING_COUNT; i++) {
		struct iwm_tx_data *data = &ring->data[i];

		if (data->m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, data->map);
			m_freem(data->m);
			data->m = NULL;
		}
		if (data->map != NULL) {
			bus_dmamap_destroy(sc->sc_dmat, data->map);
			data->map = NULL;
		}
	}
}

static void
iwm_enable_rfkill_int(struct iwm_softc *sc)
{
	sc->sc_intmask = IWM_CSR_INT_BIT_RF_KILL;
	IWM_WRITE(sc, IWM_CSR_INT_MASK, sc->sc_intmask);
}

static int
iwm_check_rfkill(struct iwm_softc *sc)
{
	uint32_t v;
	int s;
	int rv;

	s = splnet();

	/*
	 * "documentation" is not really helpful here:
	 *  27:	HW_RF_KILL_SW
	 *	Indicates state of (platform's) hardware RF-Kill switch
	 *
	 * But apparently when it's off, it's on ...
	 */
	v = IWM_READ(sc, IWM_CSR_GP_CNTRL);
	rv = (v & IWM_CSR_GP_CNTRL_REG_FLAG_HW_RF_KILL_SW) == 0;
	if (rv) {
		sc->sc_flags |= IWM_FLAG_RFKILL;
	} else {
		sc->sc_flags &= ~IWM_FLAG_RFKILL;
	}

	splx(s);
	return rv;
}

static void
iwm_enable_interrupts(struct iwm_softc *sc)
{
	sc->sc_intmask = IWM_CSR_INI_SET_MASK;
	IWM_WRITE(sc, IWM_CSR_INT_MASK, sc->sc_intmask);
}

static void
iwm_restore_interrupts(struct iwm_softc *sc)
{
	IWM_WRITE(sc, IWM_CSR_INT_MASK, sc->sc_intmask);
}

static void
iwm_disable_interrupts(struct iwm_softc *sc)
{
	int s = splnet();

	IWM_WRITE(sc, IWM_CSR_INT_MASK, 0);

	/* acknowledge all interrupts */
	IWM_WRITE(sc, IWM_CSR_INT, ~0);
	IWM_WRITE(sc, IWM_CSR_FH_INT_STATUS, ~0);

	splx(s);
}

static void
iwm_ict_reset(struct iwm_softc *sc)
{
	iwm_disable_interrupts(sc);

	memset(sc->ict_dma.vaddr, 0, IWM_ICT_SIZE);
	bus_dmamap_sync(sc->sc_dmat, sc->ict_dma.map, 0, IWM_ICT_SIZE,
	    BUS_DMASYNC_PREWRITE);
	sc->ict_cur = 0;

	/* Set physical address of ICT (4KB aligned). */
	IWM_WRITE(sc, IWM_CSR_DRAM_INT_TBL_REG,
	    IWM_CSR_DRAM_INT_TBL_ENABLE
	    | IWM_CSR_DRAM_INIT_TBL_WRAP_CHECK
	    | IWM_CSR_DRAM_INIT_TBL_WRITE_POINTER
	    | sc->ict_dma.paddr >> IWM_ICT_PADDR_SHIFT);

	/* Switch to ICT interrupt mode in driver. */
	sc->sc_flags |= IWM_FLAG_USE_ICT;

	IWM_WRITE(sc, IWM_CSR_INT, ~0);
	iwm_enable_interrupts(sc);
}

#define IWM_HW_READY_TIMEOUT 50
static int
iwm_set_hw_ready(struct iwm_softc *sc)
{
	int ready;

	IWM_SETBITS(sc, IWM_CSR_HW_IF_CONFIG_REG,
	    IWM_CSR_HW_IF_CONFIG_REG_BIT_NIC_READY);

	ready = iwm_poll_bit(sc, IWM_CSR_HW_IF_CONFIG_REG,
	    IWM_CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
	    IWM_CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
	    IWM_HW_READY_TIMEOUT);
	if (ready)
		IWM_SETBITS(sc, IWM_CSR_MBOX_SET_REG,
		    IWM_CSR_MBOX_SET_REG_OS_ALIVE);

	return ready;
}
#undef IWM_HW_READY_TIMEOUT

static int
iwm_prepare_card_hw(struct iwm_softc *sc)
{
	int t = 0;

	if (iwm_set_hw_ready(sc))
		return 0;

	DELAY(100);

	/* If HW is not ready, prepare the conditions to check again */
	IWM_SETBITS(sc, IWM_CSR_HW_IF_CONFIG_REG,
	    IWM_CSR_HW_IF_CONFIG_REG_PREPARE);

	do {
		if (iwm_set_hw_ready(sc))
			return 0;
		DELAY(200);
		t += 200;
	} while (t < 150000);

	return ETIMEDOUT;
}

static void
iwm_apm_config(struct iwm_softc *sc)
{
	pcireg_t reg;

	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag,
	    sc->sc_cap_off + PCIE_LCSR);
	if (reg & PCIE_LCSR_ASPM_L1) {
		/* Um the Linux driver prints "Disabling L0S for this one ... */
		IWM_SETBITS(sc, IWM_CSR_GIO_REG,
		    IWM_CSR_GIO_REG_VAL_L0S_ENABLED);
	} else {
		/* ... and "Enabling" here */
		IWM_CLRBITS(sc, IWM_CSR_GIO_REG,
		    IWM_CSR_GIO_REG_VAL_L0S_ENABLED);
	}
}

/*
 * Start up NIC's basic functionality after it has been reset
 * e.g. after platform boot or shutdown.
 * NOTE:  This does not load uCode nor start the embedded processor
 */
static int
iwm_apm_init(struct iwm_softc *sc)
{
	int err = 0;

	/* Disable L0S exit timer (platform NMI workaround) */
	if (sc->sc_device_family != IWM_DEVICE_FAMILY_8000) {
		IWM_SETBITS(sc, IWM_CSR_GIO_CHICKEN_BITS,
		    IWM_CSR_GIO_CHICKEN_BITS_REG_BIT_DIS_L0S_EXIT_TIMER);
	}

	/*
	 * Disable L0s without affecting L1;
	 *  don't wait for ICH L0s (ICH bug W/A)
	 */
	IWM_SETBITS(sc, IWM_CSR_GIO_CHICKEN_BITS,
	    IWM_CSR_GIO_CHICKEN_BITS_REG_BIT_L1A_NO_L0S_RX);

	/* Set FH wait threshold to maximum (HW error during stress W/A) */
	IWM_SETBITS(sc, IWM_CSR_DBG_HPET_MEM_REG, IWM_CSR_DBG_HPET_MEM_REG_VAL);

	/*
	 * Enable HAP INTA (interrupt from management bus) to
	 * wake device's PCI Express link L1a -> L0s
	 */
	IWM_SETBITS(sc, IWM_CSR_HW_IF_CONFIG_REG,
	    IWM_CSR_HW_IF_CONFIG_REG_BIT_HAP_WAKE_L1A);

	iwm_apm_config(sc);

#if 0 /* not for 7k/8k */
	/* Configure analog phase-lock-loop before activating to D0A */
	if (trans->cfg->base_params->pll_cfg_val)
		IWM_SETBITS(trans, IWM_CSR_ANA_PLL_CFG,
		    trans->cfg->base_params->pll_cfg_val);
#endif

	/*
	 * Set "initialization complete" bit to move adapter from
	 * D0U* --> D0A* (powered-up active) state.
	 */
	IWM_SETBITS(sc, IWM_CSR_GP_CNTRL, IWM_CSR_GP_CNTRL_REG_FLAG_INIT_DONE);

	/*
	 * Wait for clock stabilization; once stabilized, access to
	 * device-internal resources is supported, e.g. iwm_write_prph()
	 * and accesses to uCode SRAM.
	 */
	if (!iwm_poll_bit(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25000)) {
		aprint_error_dev(sc->sc_dev,
		    "timeout waiting for clock stabilization\n");
		err = ETIMEDOUT;
		goto out;
	}

	if (sc->host_interrupt_operation_mode) {
		/*
		 * This is a bit of an abuse - This is needed for 7260 / 3160
		 * only check host_interrupt_operation_mode even if this is
		 * not related to host_interrupt_operation_mode.
		 *
		 * Enable the oscillator to count wake up time for L1 exit. This
		 * consumes slightly more power (100uA) - but allows to be sure
		 * that we wake up from L1 on time.
		 *
		 * This looks weird: read twice the same register, discard the
		 * value, set a bit, and yet again, read that same register
		 * just to discard the value. But that's the way the hardware
		 * seems to like it.
		 */
		iwm_read_prph(sc, IWM_OSC_CLK);
		iwm_read_prph(sc, IWM_OSC_CLK);
		iwm_set_bits_prph(sc, IWM_OSC_CLK, IWM_OSC_CLK_FORCE_CONTROL);
		iwm_read_prph(sc, IWM_OSC_CLK);
		iwm_read_prph(sc, IWM_OSC_CLK);
	}

	/*
	 * Enable DMA clock and wait for it to stabilize.
	 *
	 * Write to "CLK_EN_REG"; "1" bits enable clocks, while "0" bits
	 * do not disable clocks.  This preserves any hardware bits already
	 * set by default in "CLK_CTRL_REG" after reset.
	 */
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		iwm_write_prph(sc, IWM_APMG_CLK_EN_REG,
		    IWM_APMG_CLK_VAL_DMA_CLK_RQT);
		DELAY(20);

		/* Disable L1-Active */
		iwm_set_bits_prph(sc, IWM_APMG_PCIDEV_STT_REG,
		    IWM_APMG_PCIDEV_STT_VAL_L1_ACT_DIS);

		/* Clear the interrupt in APMG if the NIC is in RFKILL */
		iwm_write_prph(sc, IWM_APMG_RTC_INT_STT_REG,
		    IWM_APMG_RTC_INT_STT_RFKILL);
	}
 out:
	if (err)
		aprint_error_dev(sc->sc_dev, "apm init error %d\n", err);
	return err;
}

static void
iwm_apm_stop(struct iwm_softc *sc)
{
	/* stop device's busmaster DMA activity */
	IWM_SETBITS(sc, IWM_CSR_RESET, IWM_CSR_RESET_REG_FLAG_STOP_MASTER);

	if (!iwm_poll_bit(sc, IWM_CSR_RESET,
	    IWM_CSR_RESET_REG_FLAG_MASTER_DISABLED,
	    IWM_CSR_RESET_REG_FLAG_MASTER_DISABLED, 100))
		aprint_error_dev(sc->sc_dev, "timeout waiting for master\n");
	DPRINTF(("iwm apm stop\n"));
}

static int
iwm_start_hw(struct iwm_softc *sc)
{
	int err;

	err = iwm_prepare_card_hw(sc);
	if (err)
		return err;

	/* Reset the entire device */
	IWM_WRITE(sc, IWM_CSR_RESET, IWM_CSR_RESET_REG_FLAG_SW_RESET);
	DELAY(10);

	err = iwm_apm_init(sc);
	if (err)
		return err;

	iwm_enable_rfkill_int(sc);
	iwm_check_rfkill(sc);

	return 0;
}

static void
iwm_stop_device(struct iwm_softc *sc)
{
	int chnl, ntries;
	int qid;

	iwm_disable_interrupts(sc);
	sc->sc_flags &= ~IWM_FLAG_USE_ICT;

	/* Deactivate TX scheduler. */
	iwm_write_prph(sc, IWM_SCD_TXFACT, 0);

	/* Stop all DMA channels. */
	if (iwm_nic_lock(sc)) {
		for (chnl = 0; chnl < IWM_FH_TCSR_CHNL_NUM; chnl++) {
			IWM_WRITE(sc,
			    IWM_FH_TCSR_CHNL_TX_CONFIG_REG(chnl), 0);
			for (ntries = 0; ntries < 200; ntries++) {
				uint32_t r;

				r = IWM_READ(sc, IWM_FH_TSSR_TX_STATUS_REG);
				if (r & IWM_FH_TSSR_TX_STATUS_REG_MSK_CHNL_IDLE(
				    chnl))
					break;
				DELAY(20);
			}
		}
		iwm_nic_unlock(sc);
	}
	iwm_disable_rx_dma(sc);

	iwm_reset_rx_ring(sc, &sc->rxq);

	for (qid = 0; qid < __arraycount(sc->txq); qid++)
		iwm_reset_tx_ring(sc, &sc->txq[qid]);

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		/* Power-down device's busmaster DMA clocks */
		if (iwm_nic_lock(sc)) {
			iwm_write_prph(sc, IWM_APMG_CLK_DIS_REG,
			    IWM_APMG_CLK_VAL_DMA_CLK_RQT);
			DELAY(5);
			iwm_nic_unlock(sc);
		}
	}

	/* Make sure (redundant) we've released our request to stay awake */
	IWM_CLRBITS(sc, IWM_CSR_GP_CNTRL,
	    IWM_CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);

	/* Stop the device, and put it in low power state */
	iwm_apm_stop(sc);

	/*
	 * Upon stop, the APM issues an interrupt if HW RF kill is set.
	 * Clean again the interrupt here
	 */
	iwm_disable_interrupts(sc);

	/* Reset the on-board processor. */
	IWM_WRITE(sc, IWM_CSR_RESET, IWM_CSR_RESET_REG_FLAG_SW_RESET);

	/* Even though we stop the HW we still want the RF kill interrupt. */
	iwm_enable_rfkill_int(sc);
	iwm_check_rfkill(sc);
}

static void
iwm_nic_config(struct iwm_softc *sc)
{
	uint8_t radio_cfg_type, radio_cfg_step, radio_cfg_dash;
	uint32_t reg_val = 0;

	radio_cfg_type = (sc->sc_fw_phy_config & IWM_FW_PHY_CFG_RADIO_TYPE) >>
	    IWM_FW_PHY_CFG_RADIO_TYPE_POS;
	radio_cfg_step = (sc->sc_fw_phy_config & IWM_FW_PHY_CFG_RADIO_STEP) >>
	    IWM_FW_PHY_CFG_RADIO_STEP_POS;
	radio_cfg_dash = (sc->sc_fw_phy_config & IWM_FW_PHY_CFG_RADIO_DASH) >>
	    IWM_FW_PHY_CFG_RADIO_DASH_POS;

	reg_val |= IWM_CSR_HW_REV_STEP(sc->sc_hw_rev) <<
	    IWM_CSR_HW_IF_CONFIG_REG_POS_MAC_STEP;
	reg_val |= IWM_CSR_HW_REV_DASH(sc->sc_hw_rev) <<
	    IWM_CSR_HW_IF_CONFIG_REG_POS_MAC_DASH;

	/* radio configuration */
	reg_val |= radio_cfg_type << IWM_CSR_HW_IF_CONFIG_REG_POS_PHY_TYPE;
	reg_val |= radio_cfg_step << IWM_CSR_HW_IF_CONFIG_REG_POS_PHY_STEP;
	reg_val |= radio_cfg_dash << IWM_CSR_HW_IF_CONFIG_REG_POS_PHY_DASH;

	IWM_WRITE(sc, IWM_CSR_HW_IF_CONFIG_REG, reg_val);

	DPRINTF(("Radio type=0x%x-0x%x-0x%x\n", radio_cfg_type,
	    radio_cfg_step, radio_cfg_dash));

	/*
	 * W/A : NIC is stuck in a reset state after Early PCIe power off
	 * (PCIe power is lost before PERST# is asserted), causing ME FW
	 * to lose ownership and not being able to obtain it back.
	 */
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		iwm_set_bits_mask_prph(sc, IWM_APMG_PS_CTRL_REG,
		    IWM_APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS,
		    ~IWM_APMG_PS_CTRL_EARLY_PWR_OFF_RESET_DIS);
	}
}

static int
iwm_nic_rx_init(struct iwm_softc *sc)
{
	if (!iwm_nic_lock(sc))
		return EBUSY;

	memset(sc->rxq.stat, 0, sizeof(*sc->rxq.stat));
	bus_dmamap_sync(sc->sc_dmat, sc->rxq.stat_dma.map,
	    0, sc->rxq.stat_dma.size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	iwm_disable_rx_dma(sc);
	IWM_WRITE(sc, IWM_FH_MEM_RCSR_CHNL0_RBDCB_WPTR, 0);
	IWM_WRITE(sc, IWM_FH_MEM_RCSR_CHNL0_FLUSH_RB_REQ, 0);
	IWM_WRITE(sc, IWM_FH_RSCSR_CHNL0_RDPTR, 0);
	IWM_WRITE(sc, IWM_FH_RSCSR_CHNL0_RBDCB_WPTR_REG, 0);

	/* Set physical address of RX ring (256-byte aligned). */
	IWM_WRITE(sc,
	    IWM_FH_RSCSR_CHNL0_RBDCB_BASE_REG, sc->rxq.desc_dma.paddr >> 8);

	/* Set physical address of RX status (16-byte aligned). */
	IWM_WRITE(sc,
	    IWM_FH_RSCSR_CHNL0_STTS_WPTR_REG, sc->rxq.stat_dma.paddr >> 4);

	/* Enable RX. */
	IWM_WRITE(sc, IWM_FH_MEM_RCSR_CHNL0_CONFIG_REG,
	    IWM_FH_RCSR_RX_CONFIG_CHNL_EN_ENABLE_VAL		|
	    IWM_FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY		|  /* HW bug */
	    IWM_FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_INT_HOST_VAL	|
	    IWM_FH_RCSR_CHNL0_RX_CONFIG_SINGLE_FRAME_MSK	|
	    IWM_FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K		|
	    (IWM_RX_RB_TIMEOUT << IWM_FH_RCSR_RX_CONFIG_REG_IRQ_RBTH_POS) |
	    IWM_RX_QUEUE_SIZE_LOG << IWM_FH_RCSR_RX_CONFIG_RBDCB_SIZE_POS);

	IWM_WRITE_1(sc, IWM_CSR_INT_COALESCING, IWM_HOST_INT_TIMEOUT_DEF);

	/* W/A for interrupt coalescing bug in 7260 and 3160 */
	if (sc->host_interrupt_operation_mode)
		IWM_SETBITS(sc, IWM_CSR_INT_COALESCING, IWM_HOST_INT_OPER_MODE);

	/*
	 * This value should initially be 0 (before preparing any RBs),
	 * and should be 8 after preparing the first 8 RBs (for example).
	 */
	IWM_WRITE(sc, IWM_FH_RSCSR_CHNL0_WPTR, 8);

	iwm_nic_unlock(sc);

	return 0;
}

static int
iwm_nic_tx_init(struct iwm_softc *sc)
{
	int qid;

	if (!iwm_nic_lock(sc))
		return EBUSY;

	/* Deactivate TX scheduler. */
	iwm_write_prph(sc, IWM_SCD_TXFACT, 0);

	/* Set physical address of "keep warm" page (16-byte aligned). */
	IWM_WRITE(sc, IWM_FH_KW_MEM_ADDR_REG, sc->kw_dma.paddr >> 4);

	for (qid = 0; qid < __arraycount(sc->txq); qid++) {
		struct iwm_tx_ring *txq = &sc->txq[qid];

		/* Set physical address of TX ring (256-byte aligned). */
		IWM_WRITE(sc, IWM_FH_MEM_CBBC_QUEUE(qid),
		    txq->desc_dma.paddr >> 8);
		DPRINTF(("loading ring %d descriptors (%p) at %"PRIxMAX"\n",
		    qid, txq->desc, (uintmax_t)(txq->desc_dma.paddr >> 8)));
	}

	iwm_write_prph(sc, IWM_SCD_GP_CTRL, IWM_SCD_GP_CTRL_AUTO_ACTIVE_MODE);

	iwm_nic_unlock(sc);

	return 0;
}

static int
iwm_nic_init(struct iwm_softc *sc)
{
	int err;

	iwm_apm_init(sc);
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		iwm_set_bits_mask_prph(sc, IWM_APMG_PS_CTRL_REG,
		    IWM_APMG_PS_CTRL_VAL_PWR_SRC_VMAIN,
		    ~IWM_APMG_PS_CTRL_MSK_PWR_SRC);
	}

	iwm_nic_config(sc);

	err = iwm_nic_rx_init(sc);
	if (err)
		return err;

	err = iwm_nic_tx_init(sc);
	if (err)
		return err;

	DPRINTF(("shadow registers enabled\n"));
	IWM_SETBITS(sc, IWM_CSR_MAC_SHADOW_REG_CTRL, 0x800fffff);

	return 0;
}

static const uint8_t iwm_ac_to_tx_fifo[] = {
	IWM_TX_FIFO_VO,
	IWM_TX_FIFO_VI,
	IWM_TX_FIFO_BE,
	IWM_TX_FIFO_BK,
};

static int
iwm_enable_txq(struct iwm_softc *sc, int sta_id, int qid, int fifo)
{
	if (!iwm_nic_lock(sc)) {
		DPRINTF(("%s: cannot enable txq %d\n", DEVNAME(sc), qid));
		return EBUSY;
	}

	IWM_WRITE(sc, IWM_HBUS_TARG_WRPTR, qid << 8 | 0);

	if (qid == IWM_CMD_QUEUE) {
		iwm_write_prph(sc, IWM_SCD_QUEUE_STATUS_BITS(qid),
		    (0 << IWM_SCD_QUEUE_STTS_REG_POS_ACTIVE)
		    | (1 << IWM_SCD_QUEUE_STTS_REG_POS_SCD_ACT_EN));

		iwm_nic_unlock(sc);

		iwm_clear_bits_prph(sc, IWM_SCD_AGGR_SEL, (1 << qid));

		if (!iwm_nic_lock(sc))
			return EBUSY;
		iwm_write_prph(sc, IWM_SCD_QUEUE_RDPTR(qid), 0);
		iwm_nic_unlock(sc);

		iwm_write_mem32(sc,
		    sc->sched_base + IWM_SCD_CONTEXT_QUEUE_OFFSET(qid), 0);

		/* Set scheduler window size and frame limit. */
		iwm_write_mem32(sc,
		    sc->sched_base + IWM_SCD_CONTEXT_QUEUE_OFFSET(qid) +
		    sizeof(uint32_t),
		    ((IWM_FRAME_LIMIT << IWM_SCD_QUEUE_CTX_REG2_WIN_SIZE_POS) &
		    IWM_SCD_QUEUE_CTX_REG2_WIN_SIZE_MSK) |
		    ((IWM_FRAME_LIMIT
		        << IWM_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_POS) &
		    IWM_SCD_QUEUE_CTX_REG2_FRAME_LIMIT_MSK));

		if (!iwm_nic_lock(sc))
			return EBUSY;
		iwm_write_prph(sc, IWM_SCD_QUEUE_STATUS_BITS(qid),
		    (1 << IWM_SCD_QUEUE_STTS_REG_POS_ACTIVE) |
		    (fifo << IWM_SCD_QUEUE_STTS_REG_POS_TXF) |
		    (1 << IWM_SCD_QUEUE_STTS_REG_POS_WSL) |
		    IWM_SCD_QUEUE_STTS_REG_MSK);
	} else {
		struct iwm_scd_txq_cfg_cmd cmd;
		int err;

		iwm_nic_unlock(sc);

		memset(&cmd, 0, sizeof(cmd));
		cmd.scd_queue = qid;
		cmd.enable = 1;
		cmd.sta_id = sta_id;
		cmd.tx_fifo = fifo;
		cmd.aggregate = 0;
		cmd.window = IWM_FRAME_LIMIT;

		err = iwm_send_cmd_pdu(sc, IWM_SCD_QUEUE_CFG, 0, sizeof(cmd),
		    &cmd);
		if (err)
			return err;

		if (!iwm_nic_lock(sc))
			return EBUSY;
	}

	iwm_write_prph(sc, IWM_SCD_EN_CTRL,
	    iwm_read_prph(sc, IWM_SCD_EN_CTRL) | qid);

	iwm_nic_unlock(sc);

	DPRINTF(("enabled txq %d FIFO %d\n", qid, fifo));

	return 0;
}

static int
iwm_post_alive(struct iwm_softc *sc)
{
	int nwords = (IWM_SCD_TRANS_TBL_MEM_UPPER_BOUND -
	    IWM_SCD_CONTEXT_MEM_LOWER_BOUND) / sizeof(uint32_t);
	int err, chnl;
	uint32_t base;

	if (!iwm_nic_lock(sc))
		return EBUSY;

	base = iwm_read_prph(sc, IWM_SCD_SRAM_BASE_ADDR);
	if (sc->sched_base != base) {
		DPRINTF(("%s: sched addr mismatch: 0x%08x != 0x%08x\n",
		    DEVNAME(sc), sc->sched_base, base));
		sc->sched_base = base;
	}

	iwm_nic_unlock(sc);

	iwm_ict_reset(sc);

	/* Clear TX scheduler state in SRAM. */
	err = iwm_write_mem(sc,
	    sc->sched_base + IWM_SCD_CONTEXT_MEM_LOWER_BOUND, NULL, nwords);
	if (err)
		return err;

	if (!iwm_nic_lock(sc))
		return EBUSY;

	/* Set physical address of TX scheduler rings (1KB aligned). */
	iwm_write_prph(sc, IWM_SCD_DRAM_BASE_ADDR, sc->sched_dma.paddr >> 10);

	iwm_write_prph(sc, IWM_SCD_CHAINEXT_EN, 0);

	iwm_nic_unlock(sc);

	/* enable command channel */
	err = iwm_enable_txq(sc, 0 /* unused */, IWM_CMD_QUEUE, 7);
	if (err)
		return err;

	if (!iwm_nic_lock(sc))
		return EBUSY;

	/* Activate TX scheduler. */
	iwm_write_prph(sc, IWM_SCD_TXFACT, 0xff);

	/* Enable DMA channels. */
	for (chnl = 0; chnl < IWM_FH_TCSR_CHNL_NUM; chnl++) {
		IWM_WRITE(sc, IWM_FH_TCSR_CHNL_TX_CONFIG_REG(chnl),
		    IWM_FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
		    IWM_FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_ENABLE);
	}

	IWM_SETBITS(sc, IWM_FH_TX_CHICKEN_BITS_REG,
	    IWM_FH_TX_CHICKEN_BITS_SCD_AUTO_RETRY_EN);

	/* Enable L1-Active */
	if (sc->sc_device_family != IWM_DEVICE_FAMILY_8000) {
		iwm_clear_bits_prph(sc, IWM_APMG_PCIDEV_STT_REG,
		    IWM_APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
	}

	iwm_nic_unlock(sc);

	return 0;
}

static struct iwm_phy_db_entry *
iwm_phy_db_get_section(struct iwm_softc *sc, enum iwm_phy_db_section_type type,
    uint16_t chg_id)
{
	struct iwm_phy_db *phy_db = &sc->sc_phy_db;

	if (type >= IWM_PHY_DB_MAX)
		return NULL;

	switch (type) {
	case IWM_PHY_DB_CFG:
		return &phy_db->cfg;
	case IWM_PHY_DB_CALIB_NCH:
		return &phy_db->calib_nch;
	case IWM_PHY_DB_CALIB_CHG_PAPD:
		if (chg_id >= IWM_NUM_PAPD_CH_GROUPS)
			return NULL;
		return &phy_db->calib_ch_group_papd[chg_id];
	case IWM_PHY_DB_CALIB_CHG_TXP:
		if (chg_id >= IWM_NUM_TXP_CH_GROUPS)
			return NULL;
		return &phy_db->calib_ch_group_txp[chg_id];
	default:
		return NULL;
	}
	return NULL;
}

static int
iwm_phy_db_set_section(struct iwm_softc *sc,
    struct iwm_calib_res_notif_phy_db *phy_db_notif, uint16_t size)
{
	struct iwm_phy_db_entry *entry;
	enum iwm_phy_db_section_type type = le16toh(phy_db_notif->type);
	uint16_t chg_id = 0;

	if (type == IWM_PHY_DB_CALIB_CHG_PAPD ||
	    type == IWM_PHY_DB_CALIB_CHG_TXP)
		chg_id = le16toh(*(uint16_t *)phy_db_notif->data);

	entry = iwm_phy_db_get_section(sc, type, chg_id);
	if (!entry)
		return EINVAL;

	if (entry->data)
		kmem_intr_free(entry->data, entry->size);
	entry->data = kmem_intr_alloc(size, KM_NOSLEEP);
	if (!entry->data) {
		entry->size = 0;
		return ENOMEM;
	}
	memcpy(entry->data, phy_db_notif->data, size);
	entry->size = size;

	DPRINTFN(10, ("%s(%d): [PHYDB]SET: Type %d, Size: %d, data: %p\n",
	    __func__, __LINE__, type, size, entry->data));

	return 0;
}

static int
iwm_is_valid_channel(uint16_t ch_id)
{
	if (ch_id <= 14 ||
	    (36 <= ch_id && ch_id <= 64 && ch_id % 4 == 0) ||
	    (100 <= ch_id && ch_id <= 140 && ch_id % 4 == 0) ||
	    (145 <= ch_id && ch_id <= 165 && ch_id % 4 == 1))
		return 1;
	return 0;
}

static uint8_t
iwm_ch_id_to_ch_index(uint16_t ch_id)
{
	if (!iwm_is_valid_channel(ch_id))
		return 0xff;

	if (ch_id <= 14)
		return ch_id - 1;
	if (ch_id <= 64)
		return (ch_id + 20) / 4;
	if (ch_id <= 140)
		return (ch_id - 12) / 4;
	return (ch_id - 13) / 4;
}


static uint16_t
iwm_channel_id_to_papd(uint16_t ch_id)
{
	if (!iwm_is_valid_channel(ch_id))
		return 0xff;

	if (1 <= ch_id && ch_id <= 14)
		return 0;
	if (36 <= ch_id && ch_id <= 64)
		return 1;
	if (100 <= ch_id && ch_id <= 140)
		return 2;
	return 3;
}

static uint16_t
iwm_channel_id_to_txp(struct iwm_softc *sc, uint16_t ch_id)
{
	struct iwm_phy_db *phy_db = &sc->sc_phy_db;
	struct iwm_phy_db_chg_txp *txp_chg;
	int i;
	uint8_t ch_index = iwm_ch_id_to_ch_index(ch_id);

	if (ch_index == 0xff)
		return 0xff;

	for (i = 0; i < IWM_NUM_TXP_CH_GROUPS; i++) {
		txp_chg = (void *)phy_db->calib_ch_group_txp[i].data;
		if (!txp_chg)
			return 0xff;
		/*
		 * Looking for the first channel group the max channel
		 * of which is higher than the requested channel.
		 */
		if (le16toh(txp_chg->max_channel_idx) >= ch_index)
			return i;
	}
	return 0xff;
}

static int
iwm_phy_db_get_section_data(struct iwm_softc *sc, uint32_t type, uint8_t **data,
    uint16_t *size, uint16_t ch_id)
{
	struct iwm_phy_db_entry *entry;
	uint16_t ch_group_id = 0;

	if (type == IWM_PHY_DB_CALIB_CHG_PAPD)
		ch_group_id = iwm_channel_id_to_papd(ch_id);
	else if (type == IWM_PHY_DB_CALIB_CHG_TXP)
		ch_group_id = iwm_channel_id_to_txp(sc, ch_id);

	entry = iwm_phy_db_get_section(sc, type, ch_group_id);
	if (!entry)
		return EINVAL;

	*data = entry->data;
	*size = entry->size;

	DPRINTFN(10, ("%s(%d): [PHYDB] GET: Type %d , Size: %d\n",
		       __func__, __LINE__, type, *size));

	return 0;
}

static int
iwm_send_phy_db_cmd(struct iwm_softc *sc, uint16_t type, uint16_t length,
    void *data)
{
	struct iwm_phy_db_cmd phy_db_cmd;
	struct iwm_host_cmd cmd = {
		.id = IWM_PHY_DB_CMD,
		.flags = IWM_CMD_ASYNC,
	};

	DPRINTFN(10, ("Sending PHY-DB hcmd of type %d, of length %d\n",
	    type, length));

	phy_db_cmd.type = le16toh(type);
	phy_db_cmd.length = le16toh(length);

	cmd.data[0] = &phy_db_cmd;
	cmd.len[0] = sizeof(struct iwm_phy_db_cmd);
	cmd.data[1] = data;
	cmd.len[1] = length;

	return iwm_send_cmd(sc, &cmd);
}

static int
iwm_phy_db_send_all_channel_groups(struct iwm_softc *sc,
    enum iwm_phy_db_section_type type, uint8_t max_ch_groups)
{
	uint16_t i;
	int err;
	struct iwm_phy_db_entry *entry;

	/* Send all the channel-specific groups to operational fw */
	for (i = 0; i < max_ch_groups; i++) {
		entry = iwm_phy_db_get_section(sc, type, i);
		if (!entry)
			return EINVAL;

		if (!entry->size)
			continue;

		err = iwm_send_phy_db_cmd(sc, type, entry->size, entry->data);
		if (err) {
			DPRINTF(("%s: Can't SEND phy_db section %d (%d), "
			    "err %d\n", DEVNAME(sc), type, i, err));
			return err;
		}

		DPRINTFN(10, ("%s: Sent PHY_DB HCMD, type = %d num = %d\n",
		    DEVNAME(sc), type, i));

		DELAY(1000);
	}

	return 0;
}

static int
iwm_send_phy_db_data(struct iwm_softc *sc)
{
	uint8_t *data = NULL;
	uint16_t size = 0;
	int err;

	err = iwm_phy_db_get_section_data(sc, IWM_PHY_DB_CFG, &data, &size, 0);
	if (err)
		return err;

	err = iwm_send_phy_db_cmd(sc, IWM_PHY_DB_CFG, size, data);
	if (err)
		return err;

	err = iwm_phy_db_get_section_data(sc, IWM_PHY_DB_CALIB_NCH,
	    &data, &size, 0);
	if (err)
		return err;

	err = iwm_send_phy_db_cmd(sc, IWM_PHY_DB_CALIB_NCH, size, data);
	if (err)
		return err;

	err = iwm_phy_db_send_all_channel_groups(sc,
	    IWM_PHY_DB_CALIB_CHG_PAPD, IWM_NUM_PAPD_CH_GROUPS);
	if (err)
		return err;

	err = iwm_phy_db_send_all_channel_groups(sc,
	    IWM_PHY_DB_CALIB_CHG_TXP, IWM_NUM_TXP_CH_GROUPS);
	if (err)
		return err;

	return 0;
}

/*
 * For the high priority TE use a time event type that has similar priority to
 * the FW's action scan priority.
 */
#define IWM_ROC_TE_TYPE_NORMAL IWM_TE_P2P_DEVICE_DISCOVERABLE
#define IWM_ROC_TE_TYPE_MGMT_TX IWM_TE_P2P_CLIENT_ASSOC

/* used to convert from time event API v2 to v1 */
#define IWM_TE_V2_DEP_POLICY_MSK (IWM_TE_V2_DEP_OTHER | IWM_TE_V2_DEP_TSF |\
			     IWM_TE_V2_EVENT_SOCIOPATHIC)
static inline uint16_t
iwm_te_v2_get_notify(uint16_t policy)
{
	return le16toh(policy) & IWM_TE_V2_NOTIF_MSK;
}

static inline uint16_t
iwm_te_v2_get_dep_policy(uint16_t policy)
{
	return (le16toh(policy) & IWM_TE_V2_DEP_POLICY_MSK) >>
		IWM_TE_V2_PLACEMENT_POS;
}

static inline uint16_t
iwm_te_v2_get_absence(uint16_t policy)
{
	return (le16toh(policy) & IWM_TE_V2_ABSENCE) >> IWM_TE_V2_ABSENCE_POS;
}

static void
iwm_te_v2_to_v1(const struct iwm_time_event_cmd_v2 *cmd_v2,
    struct iwm_time_event_cmd_v1 *cmd_v1)
{
	cmd_v1->id_and_color = cmd_v2->id_and_color;
	cmd_v1->action = cmd_v2->action;
	cmd_v1->id = cmd_v2->id;
	cmd_v1->apply_time = cmd_v2->apply_time;
	cmd_v1->max_delay = cmd_v2->max_delay;
	cmd_v1->depends_on = cmd_v2->depends_on;
	cmd_v1->interval = cmd_v2->interval;
	cmd_v1->duration = cmd_v2->duration;
	if (cmd_v2->repeat == IWM_TE_V2_REPEAT_ENDLESS)
		cmd_v1->repeat = htole32(IWM_TE_V1_REPEAT_ENDLESS);
	else
		cmd_v1->repeat = htole32(cmd_v2->repeat);
	cmd_v1->max_frags = htole32(cmd_v2->max_frags);
	cmd_v1->interval_reciprocal = 0; /* unused */

	cmd_v1->dep_policy = htole32(iwm_te_v2_get_dep_policy(cmd_v2->policy));
	cmd_v1->is_present = htole32(!iwm_te_v2_get_absence(cmd_v2->policy));
	cmd_v1->notify = htole32(iwm_te_v2_get_notify(cmd_v2->policy));
}

static int
iwm_send_time_event_cmd(struct iwm_softc *sc,
    const struct iwm_time_event_cmd_v2 *cmd)
{
	struct iwm_time_event_cmd_v1 cmd_v1;

	if (sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_TIME_EVENT_API_V2)
		return iwm_send_cmd_pdu(sc, IWM_TIME_EVENT_CMD, 0, sizeof(*cmd),
		    cmd);

	iwm_te_v2_to_v1(cmd, &cmd_v1);
	return iwm_send_cmd_pdu(sc, IWM_TIME_EVENT_CMD, 0, sizeof(cmd_v1),
	    &cmd_v1);
}

static void
iwm_protect_session(struct iwm_softc *sc, struct iwm_node *in,
    uint32_t duration, uint32_t max_delay)
{
	struct iwm_time_event_cmd_v2 time_cmd;

	memset(&time_cmd, 0, sizeof(time_cmd));

	time_cmd.action = htole32(IWM_FW_CTXT_ACTION_ADD);
	time_cmd.id_and_color =
	    htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id, in->in_color));
	time_cmd.id = htole32(IWM_TE_BSS_STA_AGGRESSIVE_ASSOC);

	time_cmd.apply_time = htole32(0);

	time_cmd.max_frags = IWM_TE_V2_FRAG_NONE;
	time_cmd.max_delay = htole32(max_delay);
	/* TODO: why do we need to interval = bi if it is not periodic? */
	time_cmd.interval = htole32(1);
	time_cmd.duration = htole32(duration);
	time_cmd.repeat = 1;
	time_cmd.policy
	    = htole16(IWM_TE_V2_NOTIF_HOST_EVENT_START |
	        IWM_TE_V2_NOTIF_HOST_EVENT_END |
		IWM_T2_V2_START_IMMEDIATELY);

	iwm_send_time_event_cmd(sc, &time_cmd);
}

/*
 * NVM read access and content parsing.  We do not support
 * external NVM or writing NVM.
 */

/* list of NVM sections we are allowed/need to read */
static const int iwm_nvm_to_read[] = {
	IWM_NVM_SECTION_TYPE_HW,
	IWM_NVM_SECTION_TYPE_SW,
	IWM_NVM_SECTION_TYPE_REGULATORY,
	IWM_NVM_SECTION_TYPE_CALIBRATION,
	IWM_NVM_SECTION_TYPE_PRODUCTION,
	IWM_NVM_SECTION_TYPE_HW_8000,
	IWM_NVM_SECTION_TYPE_MAC_OVERRIDE,
	IWM_NVM_SECTION_TYPE_PHY_SKU,
};

/* Default NVM size to read */
#define IWM_NVM_DEFAULT_CHUNK_SIZE	(2*1024)
#define IWM_MAX_NVM_SECTION_SIZE_7000	(16 * 512 * sizeof(uint16_t)) /*16 KB*/
#define IWM_MAX_NVM_SECTION_SIZE_8000	(32 * 512 * sizeof(uint16_t)) /*32 KB*/

#define IWM_NVM_WRITE_OPCODE 1
#define IWM_NVM_READ_OPCODE 0

static int
iwm_nvm_read_chunk(struct iwm_softc *sc, uint16_t section, uint16_t offset,
    uint16_t length, uint8_t *data, uint16_t *len)
{
	offset = 0;
	struct iwm_nvm_access_cmd nvm_access_cmd = {
		.offset = htole16(offset),
		.length = htole16(length),
		.type = htole16(section),
		.op_code = IWM_NVM_READ_OPCODE,
	};
	struct iwm_nvm_access_resp *nvm_resp;
	struct iwm_rx_packet *pkt;
	struct iwm_host_cmd cmd = {
		.id = IWM_NVM_ACCESS_CMD,
		.flags = (IWM_CMD_WANT_SKB | IWM_CMD_SEND_IN_RFKILL),
		.data = { &nvm_access_cmd, },
	};
	int err, offset_read;
	size_t bytes_read;
	uint8_t *resp_data;

	cmd.len[0] = sizeof(struct iwm_nvm_access_cmd);

	err = iwm_send_cmd(sc, &cmd);
	if (err) {
		DPRINTF(("%s: Could not send NVM_ACCESS command (error=%d)\n",
		    DEVNAME(sc), err));
		return err;
	}

	pkt = cmd.resp_pkt;
	if (pkt->hdr.flags & IWM_CMD_FAILED_MSK) {
		err = EIO;
		goto exit;
	}

	/* Extract NVM response */
	nvm_resp = (void *)pkt->data;

	err = le16toh(nvm_resp->status);
	bytes_read = le16toh(nvm_resp->length);
	offset_read = le16toh(nvm_resp->offset);
	resp_data = nvm_resp->data;
	if (err) {
		err = EINVAL;
		goto exit;
	}

	if (offset_read != offset) {
		err = EINVAL;
		goto exit;
	}
	if (bytes_read > length) {
		err = EINVAL;
		goto exit;
	}

	memcpy(data + offset, resp_data, bytes_read);
	*len = bytes_read;

 exit:
	iwm_free_resp(sc, &cmd);
	return err;
}

/*
 * Reads an NVM section completely.
 * NICs prior to 7000 family doesn't have a real NVM, but just read
 * section 0 which is the EEPROM. Because the EEPROM reading is unlimited
 * by uCode, we need to manually check in this case that we don't
 * overflow and try to read more than the EEPROM size.
 */
static int
iwm_nvm_read_section(struct iwm_softc *sc, uint16_t section, uint8_t *data,
    uint16_t *len, size_t max_len)
{
	uint16_t chunklen, seglen;
	int err;

	chunklen = seglen = IWM_NVM_DEFAULT_CHUNK_SIZE;
	*len = 0;

	/* Read NVM chunks until exhausted (reading less than requested) */
	while (seglen == chunklen && *len < max_len) {
		err = iwm_nvm_read_chunk(sc, section, *len, chunklen, data,
		    &seglen);
		if (err) {
			DPRINTF(("%s: Cannot read NVM from section %d "
			    "offset %d, length %d\n",
			    DEVNAME(sc), section, *len, chunklen));
			return err;
		}
		*len += seglen;
	}

	DPRINTFN(4, ("NVM section %d read completed\n", section));
	return 0;
}

static uint8_t
iwm_fw_valid_tx_ant(struct iwm_softc *sc)
{
	uint8_t tx_ant;

	tx_ant = ((sc->sc_fw_phy_config & IWM_FW_PHY_CFG_TX_CHAIN)
	    >> IWM_FW_PHY_CFG_TX_CHAIN_POS);

	if (sc->sc_nvm.valid_tx_ant)
		tx_ant &= sc->sc_nvm.valid_tx_ant;

	return tx_ant;
}

static uint8_t
iwm_fw_valid_rx_ant(struct iwm_softc *sc)
{
	uint8_t rx_ant;

	rx_ant = ((sc->sc_fw_phy_config & IWM_FW_PHY_CFG_RX_CHAIN)
	    >> IWM_FW_PHY_CFG_RX_CHAIN_POS);

	if (sc->sc_nvm.valid_rx_ant)
		rx_ant &= sc->sc_nvm.valid_rx_ant;

	return rx_ant;
}

static void
iwm_init_channel_map(struct iwm_softc *sc, const uint16_t * const nvm_ch_flags,
    const uint8_t *nvm_channels, size_t nchan)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_nvm_data *data = &sc->sc_nvm;
	int ch_idx;
	struct ieee80211_channel *channel;
	uint16_t ch_flags;
	int is_5ghz;
	int flags, hw_value;

	for (ch_idx = 0; ch_idx < nchan; ch_idx++) {
		ch_flags = le16_to_cpup(nvm_ch_flags + ch_idx);
		aprint_debug_dev(sc->sc_dev,
		    "Ch. %d: %svalid %cibss %s %cradar %cdfs"
		    " %cwide %c40MHz %c80MHz %c160MHz\n",
		    nvm_channels[ch_idx],
		    ch_flags & IWM_NVM_CHANNEL_VALID ? "" : "in",
		    ch_flags & IWM_NVM_CHANNEL_IBSS ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_ACTIVE ? "active" : "passive",
		    ch_flags & IWM_NVM_CHANNEL_RADAR ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_DFS ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_WIDE ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_40MHZ ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_80MHZ ? '+' : '-',
		    ch_flags & IWM_NVM_CHANNEL_160MHZ ? '+' : '-');

		if (ch_idx >= IWM_NUM_2GHZ_CHANNELS &&
		    !data->sku_cap_band_52GHz_enable)
			ch_flags &= ~IWM_NVM_CHANNEL_VALID;

		if (!(ch_flags & IWM_NVM_CHANNEL_VALID)) {
			DPRINTF(("Ch. %d Flags %x [%sGHz] - No traffic\n",
			    nvm_channels[ch_idx], ch_flags,
			    (ch_idx >= IWM_NUM_2GHZ_CHANNELS) ? "5" : "2.4"));
			continue;
		}

		hw_value = nvm_channels[ch_idx];
		channel = &ic->ic_channels[hw_value];

		is_5ghz = ch_idx >= IWM_NUM_2GHZ_CHANNELS;
		if (!is_5ghz) {
			flags = IEEE80211_CHAN_2GHZ;
			channel->ic_flags
			    = IEEE80211_CHAN_CCK
			    | IEEE80211_CHAN_OFDM
			    | IEEE80211_CHAN_DYN
			    | IEEE80211_CHAN_2GHZ;
		} else {
			flags = IEEE80211_CHAN_5GHZ;
			channel->ic_flags =
			    IEEE80211_CHAN_A;
		}
		channel->ic_freq = ieee80211_ieee2mhz(hw_value, flags);

		if (!(ch_flags & IWM_NVM_CHANNEL_ACTIVE))
			channel->ic_flags |= IEEE80211_CHAN_PASSIVE;

#ifndef IEEE80211_NO_HT
		if (data->sku_cap_11n_enable)
			channel->ic_flags |= IEEE80211_CHAN_HT;
#endif
	}
}

#ifndef IEEE80211_NO_HT
static void
iwm_setup_ht_rates(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;

	/* TX is supported with the same MCS as RX. */
	ic->ic_tx_mcs_set = IEEE80211_TX_MCS_SET_DEFINED;

	ic->ic_sup_mcs[0] = 0xff;		/* MCS 0-7 */

#ifdef notyet
	if (sc->sc_nvm.sku_cap_mimo_disable)
		return;

	if (iwm_fw_valid_rx_ant(sc) > 1)
		ic->ic_sup_mcs[1] = 0xff;	/* MCS 8-15 */
	if (iwm_fw_valid_rx_ant(sc) > 2)
		ic->ic_sup_mcs[2] = 0xff;	/* MCS 16-23 */
#endif
}

#define IWM_MAX_RX_BA_SESSIONS 16

static void
iwm_sta_rx_agg(struct iwm_softc *sc, struct ieee80211_node *ni, uint8_t tid,
    uint16_t ssn, int start)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_add_sta_cmd_v7 cmd;
	struct iwm_node *in = (struct iwm_node *)ni;
	int err, s;
	uint32_t status;

	if (start && sc->sc_rx_ba_sessions >= IWM_MAX_RX_BA_SESSIONS) {
		ieee80211_addba_req_refuse(ic, ni, tid);
		return;
	}

	memset(&cmd, 0, sizeof(cmd));

	cmd.sta_id = IWM_STATION_ID;
	cmd.mac_id_n_color
	    = htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id, in->in_color));
	cmd.add_modify = IWM_STA_MODE_MODIFY;

	if (start) {
		cmd.add_immediate_ba_tid = (uint8_t)tid;
		cmd.add_immediate_ba_ssn = ssn;
	} else {
		cmd.remove_immediate_ba_tid = (uint8_t)tid;
	}
	cmd.modify_mask = start ? IWM_STA_MODIFY_ADD_BA_TID :
	    IWM_STA_MODIFY_REMOVE_BA_TID;

	status = IWM_ADD_STA_SUCCESS;
	err = iwm_send_cmd_pdu_status(sc, IWM_ADD_STA, sizeof(cmd), &cmd,
	    &status);

	s = splnet();
	if (err == 0 && status == IWM_ADD_STA_SUCCESS) {
		if (start) {
			sc->sc_rx_ba_sessions++;
			ieee80211_addba_req_accept(ic, ni, tid);
		} else if (sc->sc_rx_ba_sessions > 0)
			sc->sc_rx_ba_sessions--;
	} else if (start)
		ieee80211_addba_req_refuse(ic, ni, tid);
	splx(s);
}

static void
iwm_htprot_task(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;
	int err;

	/* This call updates HT protection based on in->in_ni.ni_htop1. */
	err = iwm_mac_ctxt_cmd(sc, in, IWM_FW_CTXT_ACTION_MODIFY, 1);
	if (err)
		aprint_error_dev(sc->sc_dev,
		    "could not change HT protection: error %d\n", err);
}

/*
 * This function is called by upper layer when HT protection settings in
 * beacons have changed.
 */
static void
iwm_update_htprot(struct ieee80211com *ic, struct ieee80211_node *ni)
{
	struct iwm_softc *sc = ic->ic_softc;

	/* assumes that ni == ic->ic_bss */
	task_add(systq, &sc->htprot_task);
}

static void
iwm_ba_task(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;

	if (sc->ba_start)
		iwm_sta_rx_agg(sc, ni, sc->ba_tid, sc->ba_ssn, 1);
	else
		iwm_sta_rx_agg(sc, ni, sc->ba_tid, 0, 0);
}

/*
 * This function is called by upper layer when an ADDBA request is received
 * from another STA and before the ADDBA response is sent.
 */
static int
iwm_ampdu_rx_start(struct ieee80211com *ic, struct ieee80211_node *ni,
    uint8_t tid)
{
	struct ieee80211_rx_ba *ba = &ni->ni_rx_ba[tid];
	struct iwm_softc *sc = IC2IFP(ic)->if_softc;

	if (sc->sc_rx_ba_sessions >= IWM_MAX_RX_BA_SESSIONS)
		return ENOSPC;

	sc->ba_start = 1;
	sc->ba_tid = tid;
	sc->ba_ssn = htole16(ba->ba_winstart);
	task_add(systq, &sc->ba_task);

	return EBUSY;
}

/*
 * This function is called by upper layer on teardown of an HT-immediate
 * Block Ack agreement (eg. upon receipt of a DELBA frame).
 */
static void
iwm_ampdu_rx_stop(struct ieee80211com *ic, struct ieee80211_node *ni,
    uint8_t tid)
{
	struct iwm_softc *sc = IC2IFP(ic)->if_softc;

	sc->ba_start = 0;
	sc->ba_tid = tid;
	task_add(systq, &sc->ba_task);
}
#endif

static void
iwm_free_fw_paging(struct iwm_softc *sc)
{
	int i;

	if (sc->fw_paging_db[0].fw_paging_block.vaddr == NULL)
		return;

	for (i = 0; i < IWM_NUM_OF_FW_PAGING_BLOCKS; i++) {
		iwm_dma_contig_free(&sc->fw_paging_db[i].fw_paging_block);
	}

	memset(sc->fw_paging_db, 0, sizeof(sc->fw_paging_db));
}

static int
iwm_fill_paging_mem(struct iwm_softc *sc, const struct iwm_fw_sects *fws)
{
	int sec_idx, idx;
	uint32_t offset = 0;

	/*
	 * find where is the paging image start point:
	 * if CPU2 exist and it's in paging format, then the image looks like:
	 * CPU1 sections (2 or more)
	 * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between CPU1 to CPU2
	 * CPU2 sections (not paged)
	 * PAGING_SEPARATOR_SECTION delimiter - separate between CPU2
	 * non paged to CPU2 paging sec
	 * CPU2 paging CSS
	 * CPU2 paging image (including instruction and data)
	 */
	for (sec_idx = 0; sec_idx < IWM_UCODE_SECT_MAX; sec_idx++) {
		if (fws->fw_sect[sec_idx].fws_devoff ==
		    IWM_PAGING_SEPARATOR_SECTION) {
			sec_idx++;
			break;
		}
	}

	/*
	 * If paging is enabled there should be at least 2 more sections left
	 * (one for CSS and one for Paging data)
	 */
	if (sec_idx >= __arraycount(fws->fw_sect) - 1) {
		aprint_verbose_dev(sc->sc_dev,
		    "Paging: Missing CSS and/or paging sections\n");
		iwm_free_fw_paging(sc);
		return EINVAL;
	}

	/* copy the CSS block to the dram */
	DPRINTF(("%s: Paging: load paging CSS to FW, sec = %d\n", DEVNAME(sc),
	    sec_idx));

	memcpy(sc->fw_paging_db[0].fw_paging_block.vaddr,
	    fws->fw_sect[sec_idx].fws_data, sc->fw_paging_db[0].fw_paging_size);

	DPRINTF(("%s: Paging: copied %d CSS bytes to first block\n",
	    DEVNAME(sc), sc->fw_paging_db[0].fw_paging_size));

	sec_idx++;

	/*
	 * copy the paging blocks to the dram
	 * loop index start from 1 since that CSS block already copied to dram
	 * and CSS index is 0.
	 * loop stop at num_of_paging_blk since that last block is not full.
	 */
	for (idx = 1; idx < sc->num_of_paging_blk; idx++) {
		memcpy(sc->fw_paging_db[idx].fw_paging_block.vaddr,
		       (const char *)fws->fw_sect[sec_idx].fws_data + offset,
		       sc->fw_paging_db[idx].fw_paging_size);

		DPRINTF(("%s: Paging: copied %d paging bytes to block %d\n",
		    DEVNAME(sc), sc->fw_paging_db[idx].fw_paging_size, idx));

		offset += sc->fw_paging_db[idx].fw_paging_size;
	}

	/* copy the last paging block */
	if (sc->num_of_pages_in_last_blk > 0) {
		memcpy(sc->fw_paging_db[idx].fw_paging_block.vaddr,
		    (const char *)fws->fw_sect[sec_idx].fws_data + offset,
		    IWM_FW_PAGING_SIZE * sc->num_of_pages_in_last_blk);

		DPRINTF(("%s: Paging: copied %d pages in the last block %d\n",
		    DEVNAME(sc), sc->num_of_pages_in_last_blk, idx));
	}

	return 0;
}

static int
iwm_alloc_fw_paging_mem(struct iwm_softc *sc, const struct iwm_fw_sects *fws)
{
	int blk_idx = 0;
	int error, num_of_pages;
	bus_dmamap_t dmap;

	if (sc->fw_paging_db[0].fw_paging_block.vaddr != NULL) {
		int i;
		/* Device got reset, and we setup firmware paging again */
		for (i = 0; i < sc->num_of_paging_blk + 1; i++) {
			dmap = sc->fw_paging_db[i].fw_paging_block.map;
			bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
		}
		return 0;
	}

	/* ensure IWM_BLOCK_2_EXP_SIZE is power of 2 of IWM_PAGING_BLOCK_SIZE */
	CTASSERT(__BIT(IWM_BLOCK_2_EXP_SIZE) == IWM_PAGING_BLOCK_SIZE);

	num_of_pages = fws->paging_mem_size / IWM_FW_PAGING_SIZE;
	sc->num_of_paging_blk =
	    howmany(num_of_pages, IWM_NUM_OF_PAGE_PER_GROUP);
	sc->num_of_pages_in_last_blk = num_of_pages -
	    IWM_NUM_OF_PAGE_PER_GROUP * (sc->num_of_paging_blk - 1);

	DPRINTF(("%s: Paging: allocating mem for %d paging blocks, "
	    "each block holds 8 pages, last block holds %d pages\n",
	    DEVNAME(sc), sc->num_of_paging_blk, sc->num_of_pages_in_last_blk));

	/* allocate block of 4Kbytes for paging CSS */
	error = iwm_dma_contig_alloc(sc->sc_dmat,
	    &sc->fw_paging_db[blk_idx].fw_paging_block, IWM_FW_PAGING_SIZE,
	    4096);
	if (error) {
		/* free all the previous pages since we failed */
		iwm_free_fw_paging(sc);
		return ENOMEM;
	}

	sc->fw_paging_db[blk_idx].fw_paging_size = IWM_FW_PAGING_SIZE;

	DPRINTF(("%s: Paging: allocated 4K(CSS) bytes for firmware paging.\n",
	    DEVNAME(sc)));

	/*
	 * allocate blocks in dram.
	 * since that CSS allocated in fw_paging_db[0] loop start from index 1
	 */
	for (blk_idx = 1; blk_idx < sc->num_of_paging_blk + 1; blk_idx++) {
		/* allocate block of IWM_PAGING_BLOCK_SIZE (32K) */
		/* XXX Use iwm_dma_contig_alloc for allocating */
		error = iwm_dma_contig_alloc(sc->sc_dmat,
		    &sc->fw_paging_db[blk_idx].fw_paging_block,
		    IWM_PAGING_BLOCK_SIZE, 4096);
		if (error) {
			/* free all the previous pages since we failed */
			iwm_free_fw_paging(sc);
			return ENOMEM;
		}

		sc->fw_paging_db[blk_idx].fw_paging_size =
		    IWM_PAGING_BLOCK_SIZE;

		DPRINTF(("%s: Paging: allocated 32K bytes for firmware "
		    "paging.\n", DEVNAME(sc)));
	}

	return 0;
}

static int
iwm_save_fw_paging(struct iwm_softc *sc, const struct iwm_fw_sects *fws)
{
	int err;

	err = iwm_alloc_fw_paging_mem(sc, fws);
	if (err)
		return err;

	return iwm_fill_paging_mem(sc, fws);
}

static bool
iwm_has_new_tx_api(struct iwm_softc *sc)
{
	/* XXX */
	return false;
}

/* send paging cmd to FW in case CPU2 has paging image */
static int
iwm_send_paging_cmd(struct iwm_softc *sc, const struct iwm_fw_sects *fws)
{
	struct iwm_fw_paging_cmd fw_paging_cmd = {
		.flags = htole32(IWM_PAGING_CMD_IS_SECURED |
		                 IWM_PAGING_CMD_IS_ENABLED |
		                 (sc->num_of_pages_in_last_blk <<
		                  IWM_PAGING_CMD_NUM_OF_PAGES_IN_LAST_GRP_POS)),
		.block_size = htole32(IWM_BLOCK_2_EXP_SIZE),
		.block_num = htole32(sc->num_of_paging_blk),
	};
	size_t size = sizeof(fw_paging_cmd);
	int blk_idx;
	bus_dmamap_t dmap;

	if (!iwm_has_new_tx_api(sc))
		size -= (sizeof(uint64_t) - sizeof(uint32_t)) *
		    IWM_NUM_OF_FW_PAGING_BLOCKS;

	/* loop for for all paging blocks + CSS block */
	for (blk_idx = 0; blk_idx < sc->num_of_paging_blk + 1; blk_idx++) {
		bus_addr_t dev_phy_addr =
		    sc->fw_paging_db[blk_idx].fw_paging_block.paddr;
		if (iwm_has_new_tx_api(sc)) {
			fw_paging_cmd.device_phy_addr.addr64[blk_idx] =
			    htole64(dev_phy_addr);
		} else {
			dev_phy_addr = dev_phy_addr >> IWM_PAGE_2_EXP_SIZE;
			fw_paging_cmd.device_phy_addr.addr32[blk_idx] =
			    htole32(dev_phy_addr);
		}
		dmap = sc->fw_paging_db[blk_idx].fw_paging_block.map;
		bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}

	return iwm_send_cmd_pdu(sc,
	    iwm_cmd_id(IWM_FW_PAGING_BLOCK_CMD, IWM_ALWAYS_LONG_GROUP, 0),
	    0, size, &fw_paging_cmd);
}

static void
iwm_set_hw_address_8000(struct iwm_softc *sc, struct iwm_nvm_data *data,
    const uint16_t *mac_override, const uint16_t *nvm_hw)
{
	static const uint8_t reserved_mac[ETHER_ADDR_LEN] = {
		0x02, 0xcc, 0xaa, 0xff, 0xee, 0x00
	};
	static const u_int8_t etheranyaddr[ETHER_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	const uint8_t *hw_addr;

	if (mac_override) {
		hw_addr = (const uint8_t *)(mac_override +
		    IWM_MAC_ADDRESS_OVERRIDE_8000);

		/*
		 * Store the MAC address from MAO section.
		 * No byte swapping is required in MAO section
		 */
		memcpy(data->hw_addr, hw_addr, ETHER_ADDR_LEN);

		/*
		 * Force the use of the OTP MAC address in case of reserved MAC
		 * address in the NVM, or if address is given but invalid.
		 */
		if (memcmp(reserved_mac, hw_addr, ETHER_ADDR_LEN) != 0 &&
		    (memcmp(etherbroadcastaddr, data->hw_addr,
		    sizeof(etherbroadcastaddr)) != 0) &&
		    (memcmp(etheranyaddr, data->hw_addr,
		    sizeof(etheranyaddr)) != 0) &&
		    !ETHER_IS_MULTICAST(data->hw_addr))
			return;
	}

	if (nvm_hw) {
		/* Read the mac address from WFMP registers. */
		uint32_t mac_addr0 =
		    htole32(iwm_read_prph(sc, IWM_WFMP_MAC_ADDR_0));
		uint32_t mac_addr1 =
		    htole32(iwm_read_prph(sc, IWM_WFMP_MAC_ADDR_1));

		hw_addr = (const uint8_t *)&mac_addr0;
		data->hw_addr[0] = hw_addr[3];
		data->hw_addr[1] = hw_addr[2];
		data->hw_addr[2] = hw_addr[1];
		data->hw_addr[3] = hw_addr[0];

		hw_addr = (const uint8_t *)&mac_addr1;
		data->hw_addr[4] = hw_addr[1];
		data->hw_addr[5] = hw_addr[0];

		return;
	}

	aprint_error_dev(sc->sc_dev, "mac address not found\n");
	memset(data->hw_addr, 0, sizeof(data->hw_addr));
}

static int
iwm_parse_nvm_data(struct iwm_softc *sc, const uint16_t *nvm_hw,
    const uint16_t *nvm_sw, const uint16_t *nvm_calib,
    const uint16_t *mac_override, const uint16_t *phy_sku,
    const uint16_t *regulatory)
{
	struct iwm_nvm_data *data = &sc->sc_nvm;
	uint8_t hw_addr[ETHER_ADDR_LEN];
	uint32_t sku;

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		uint16_t radio_cfg = le16_to_cpup(nvm_sw + IWM_RADIO_CFG);
		data->radio_cfg_type = IWM_NVM_RF_CFG_TYPE_MSK(radio_cfg);
		data->radio_cfg_step = IWM_NVM_RF_CFG_STEP_MSK(radio_cfg);
		data->radio_cfg_dash = IWM_NVM_RF_CFG_DASH_MSK(radio_cfg);
		data->radio_cfg_pnum = IWM_NVM_RF_CFG_PNUM_MSK(radio_cfg);

		data->nvm_version = le16_to_cpup(nvm_sw + IWM_NVM_VERSION);
		sku = le16_to_cpup(nvm_sw + IWM_SKU);
	} else {
		uint32_t radio_cfg = le32_to_cpup(phy_sku + IWM_RADIO_CFG_8000);
		data->radio_cfg_type = IWM_NVM_RF_CFG_TYPE_MSK_8000(radio_cfg);
		data->radio_cfg_step = IWM_NVM_RF_CFG_STEP_MSK_8000(radio_cfg);
		data->radio_cfg_dash = IWM_NVM_RF_CFG_DASH_MSK_8000(radio_cfg);
		data->radio_cfg_pnum = IWM_NVM_RF_CFG_PNUM_MSK_8000(radio_cfg);
		data->valid_tx_ant = IWM_NVM_RF_CFG_TX_ANT_MSK_8000(radio_cfg);
		data->valid_rx_ant = IWM_NVM_RF_CFG_RX_ANT_MSK_8000(radio_cfg);

		data->nvm_version = le32_to_cpup(nvm_sw + IWM_NVM_VERSION_8000);
		sku = le32_to_cpup(phy_sku + IWM_SKU_8000);
	}

	data->sku_cap_band_24GHz_enable = sku & IWM_NVM_SKU_CAP_BAND_24GHZ;
	data->sku_cap_band_52GHz_enable = sku & IWM_NVM_SKU_CAP_BAND_52GHZ;
	data->sku_cap_11n_enable = sku & IWM_NVM_SKU_CAP_11N_ENABLE;
	data->sku_cap_mimo_disable = sku & IWM_NVM_SKU_CAP_MIMO_DISABLE;

	data->n_hw_addrs = le16_to_cpup(nvm_sw + IWM_N_HW_ADDRS);

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		memcpy(hw_addr, nvm_hw + IWM_HW_ADDR, ETHER_ADDR_LEN);
		data->hw_addr[0] = hw_addr[1];
		data->hw_addr[1] = hw_addr[0];
		data->hw_addr[2] = hw_addr[3];
		data->hw_addr[3] = hw_addr[2];
		data->hw_addr[4] = hw_addr[5];
		data->hw_addr[5] = hw_addr[4];
	} else
		iwm_set_hw_address_8000(sc, data, mac_override, nvm_hw);

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000) {
		uint16_t lar_offset, lar_config;
		lar_offset = data->nvm_version < 0xE39 ?
		    IWM_NVM_LAR_OFFSET_8000_OLD : IWM_NVM_LAR_OFFSET_8000;
		lar_config = le16_to_cpup(regulatory + lar_offset);
                data->lar_enabled = !!(lar_config & IWM_NVM_LAR_ENABLED_8000);
	}

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000)
		iwm_init_channel_map(sc, &nvm_sw[IWM_NVM_CHANNELS],
		    iwm_nvm_channels, __arraycount(iwm_nvm_channels));
	else
		iwm_init_channel_map(sc, &regulatory[IWM_NVM_CHANNELS_8000],
		    iwm_nvm_channels_8000, __arraycount(iwm_nvm_channels_8000));

	data->calib_version = 255;   /* TODO:
					this value will prevent some checks from
					failing, we need to check if this
					field is still needed, and if it does,
					where is it in the NVM */

	return 0;
}

static int
iwm_parse_nvm_sections(struct iwm_softc *sc, struct iwm_nvm_section *sections)
{
	const uint16_t *hw, *sw, *calib, *mac_override = NULL, *phy_sku = NULL;
	const uint16_t *regulatory = NULL;

	/* Checking for required sections */
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000) {
		if (!sections[IWM_NVM_SECTION_TYPE_SW].data ||
		    !sections[IWM_NVM_SECTION_TYPE_HW].data) {
			return ENOENT;
		}

		hw = (const uint16_t *) sections[IWM_NVM_SECTION_TYPE_HW].data;
	} else if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000) {
		/* SW and REGULATORY sections are mandatory */
		if (!sections[IWM_NVM_SECTION_TYPE_SW].data ||
		    !sections[IWM_NVM_SECTION_TYPE_REGULATORY].data) {
			return ENOENT;
		}
		/* MAC_OVERRIDE or at least HW section must exist */
		if (!sections[IWM_NVM_SECTION_TYPE_HW_8000].data &&
		    !sections[IWM_NVM_SECTION_TYPE_MAC_OVERRIDE].data) {
			return ENOENT;
		}

		/* PHY_SKU section is mandatory in B0 */
		if (!sections[IWM_NVM_SECTION_TYPE_PHY_SKU].data) {
			return ENOENT;
		}

		regulatory = (const uint16_t *)
		    sections[IWM_NVM_SECTION_TYPE_REGULATORY].data;
		hw = (const uint16_t *)
		    sections[IWM_NVM_SECTION_TYPE_HW_8000].data;
		mac_override =
			(const uint16_t *)
			sections[IWM_NVM_SECTION_TYPE_MAC_OVERRIDE].data;
		phy_sku = (const uint16_t *)
		    sections[IWM_NVM_SECTION_TYPE_PHY_SKU].data;
	} else {
		panic("unknown device family %d\n", sc->sc_device_family);
	}

	sw = (const uint16_t *)sections[IWM_NVM_SECTION_TYPE_SW].data;
	calib = (const uint16_t *)
	    sections[IWM_NVM_SECTION_TYPE_CALIBRATION].data;

	return iwm_parse_nvm_data(sc, hw, sw, calib, mac_override,
	    phy_sku, regulatory);
}

static int
iwm_nvm_init(struct iwm_softc *sc)
{
	struct iwm_nvm_section nvm_sections[IWM_NVM_NUM_OF_SECTIONS];
	int i, section, err;
	uint16_t len;
	uint8_t *buf;
	const size_t bufsz = (sc->sc_device_family == IWM_DEVICE_FAMILY_8000) ?
	    IWM_MAX_NVM_SECTION_SIZE_8000 : IWM_MAX_NVM_SECTION_SIZE_7000;

	/* Read From FW NVM */
	DPRINTF(("Read NVM\n"));

	memset(nvm_sections, 0, sizeof(nvm_sections));

	buf = kmem_alloc(bufsz, KM_SLEEP);

	for (i = 0; i < __arraycount(iwm_nvm_to_read); i++) {
		section = iwm_nvm_to_read[i];
		KASSERT(section <= IWM_NVM_NUM_OF_SECTIONS);

		err = iwm_nvm_read_section(sc, section, buf, &len, bufsz);
		if (err) {
			err = 0;
			continue;
		}
		nvm_sections[section].data = kmem_alloc(len, KM_SLEEP);
		memcpy(nvm_sections[section].data, buf, len);
		nvm_sections[section].length = len;
	}
	kmem_free(buf, bufsz);
	if (err == 0)
		err = iwm_parse_nvm_sections(sc, nvm_sections);

	for (i = 0; i < IWM_NVM_NUM_OF_SECTIONS; i++) {
		if (nvm_sections[i].data != NULL)
			kmem_free(nvm_sections[i].data, nvm_sections[i].length);
	}

	return err;
}

static int
iwm_firmware_load_sect(struct iwm_softc *sc, uint32_t dst_addr,
    const uint8_t *section, uint32_t byte_cnt)
{
	int err = EINVAL;
	uint32_t chunk_sz, offset;

	chunk_sz = MIN(IWM_FH_MEM_TB_MAX_LENGTH, byte_cnt);

	for (offset = 0; offset < byte_cnt; offset += chunk_sz) {
		uint32_t addr, len;
		const uint8_t *data;
		bool is_extended = false;

		addr = dst_addr + offset;
		len = MIN(chunk_sz, byte_cnt - offset);
		data = section + offset;

		if (addr >= IWM_FW_MEM_EXTENDED_START &&
		    addr <= IWM_FW_MEM_EXTENDED_END)
			is_extended = true;

		if (is_extended)
			iwm_set_bits_prph(sc, IWM_LMPM_CHICK,
			    IWM_LMPM_CHICK_EXTENDED_ADDR_SPACE);

		err = iwm_firmware_load_chunk(sc, addr, data, len);

		if (is_extended)
			iwm_clear_bits_prph(sc, IWM_LMPM_CHICK,
			    IWM_LMPM_CHICK_EXTENDED_ADDR_SPACE);

		if (err)
			break;
	}

	return err;
}

static int
iwm_firmware_load_chunk(struct iwm_softc *sc, uint32_t dst_addr,
    const uint8_t *section, uint32_t byte_cnt)
{
	struct iwm_dma_info *dma = &sc->fw_dma;
	int err;

	/* Copy firmware chunk into pre-allocated DMA-safe memory. */
	memcpy(dma->vaddr, section, byte_cnt);
	bus_dmamap_sync(sc->sc_dmat, dma->map, 0, byte_cnt,
	    BUS_DMASYNC_PREWRITE);

	sc->sc_fw_chunk_done = 0;

	if (!iwm_nic_lock(sc))
		return EBUSY;

	IWM_WRITE(sc, IWM_FH_TCSR_CHNL_TX_CONFIG_REG(IWM_FH_SRVC_CHNL),
	    IWM_FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE);
	IWM_WRITE(sc, IWM_FH_SRVC_CHNL_SRAM_ADDR_REG(IWM_FH_SRVC_CHNL),
	    dst_addr);
	IWM_WRITE(sc, IWM_FH_TFDIB_CTRL0_REG(IWM_FH_SRVC_CHNL),
	    dma->paddr & IWM_FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK);
	IWM_WRITE(sc, IWM_FH_TFDIB_CTRL1_REG(IWM_FH_SRVC_CHNL),
	    (iwm_get_dma_hi_addr(dma->paddr)
	      << IWM_FH_MEM_TFDIB_REG1_ADDR_BITSHIFT) | byte_cnt);
	IWM_WRITE(sc, IWM_FH_TCSR_CHNL_TX_BUF_STS_REG(IWM_FH_SRVC_CHNL),
	    1 << IWM_FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM |
	    1 << IWM_FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX |
	    IWM_FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID);
	IWM_WRITE(sc, IWM_FH_TCSR_CHNL_TX_CONFIG_REG(IWM_FH_SRVC_CHNL),
	    IWM_FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE    |
	    IWM_FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE |
	    IWM_FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD);

	iwm_nic_unlock(sc);

	/* Wait for this segment to load. */
	err = 0;
	while (!sc->sc_fw_chunk_done) {
		err = tsleep(&sc->sc_fw, 0, "iwmfw", mstohz(5000));
		if (err)
			break;
	}
	if (!sc->sc_fw_chunk_done) {
		DPRINTF(("%s: fw chunk addr 0x%x len %d failed to load\n",
		    DEVNAME(sc), dst_addr, byte_cnt));
	}

	return err;
}

static int
iwm_load_cpu_sections_7000(struct iwm_softc *sc, struct iwm_fw_sects *fws,
    int cpu, int *first_ucode_section)
{
	int i, err = 0;
	uint32_t last_read_idx = 0;
	void *data;
	uint32_t dlen;
	uint32_t offset;

	if (cpu == 1) {
		*first_ucode_section = 0;
	} else {
		(*first_ucode_section)++;
	}

	for (i = *first_ucode_section; i < IWM_UCODE_SECT_MAX; i++) {
		last_read_idx = i;
		data = fws->fw_sect[i].fws_data;
		dlen = fws->fw_sect[i].fws_len;
		offset = fws->fw_sect[i].fws_devoff;

		/*
		 * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
		 * CPU1 to CPU2.
		 * PAGING_SEPARATOR_SECTION delimiter - separate between
		 * CPU2 non paged to CPU2 paging sec.
		 */
		if (!data || offset == IWM_CPU1_CPU2_SEPARATOR_SECTION ||
		    offset == IWM_PAGING_SEPARATOR_SECTION)
			break;

		if (dlen > sc->sc_fwdmasegsz) {
			err = EFBIG;
		} else
			err = iwm_firmware_load_sect(sc, offset, data, dlen);
		if (err) {
			DPRINTF(("%s: could not load firmware chunk %d "
			    "(error %d)\n", DEVNAME(sc), i, err));
			return err;
		}
	}

	*first_ucode_section = last_read_idx;

	return 0;
}

static int
iwm_load_firmware_7000(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	struct iwm_fw_sects *fws;
	int err = 0;
	int first_ucode_section;

	fws = &sc->sc_fw.fw_sects[ucode_type];

	DPRINTF(("%s: working with %s CPU\n", DEVNAME(sc),
	    fws->is_dual_cpus ? "dual" : "single"));

	/* load to FW the binary Secured sections of CPU1 */
	err = iwm_load_cpu_sections_7000(sc, fws, 1, &first_ucode_section);
	if (err)
		return err;

	if (fws->is_dual_cpus) {
		/* set CPU2 header address */
		if (iwm_nic_lock(sc)) {
			iwm_write_prph(sc,
			    IWM_LMPM_SECURE_UCODE_LOAD_CPU2_HDR_ADDR,
			    IWM_LMPM_SECURE_CPU2_HDR_MEM_SPACE);
			iwm_nic_unlock(sc);
		}

		/* load to FW the binary sections of CPU2 */
		err = iwm_load_cpu_sections_7000(sc, fws, 2,
		    &first_ucode_section);
		if (err)
			return err;
	}

	/* release CPU reset */
	IWM_WRITE(sc, IWM_CSR_RESET, 0);

	return 0;
}

static int
iwm_load_cpu_sections_8000(struct iwm_softc *sc, struct iwm_fw_sects *fws,
    int cpu, int *first_ucode_section)
{
	int shift_param;
	int i, err = 0, sec_num = 0x1;
	uint32_t val, last_read_idx = 0;
	void *data;
	uint32_t dlen;
	uint32_t offset;

	if (cpu == 1) {
		shift_param = 0;
		*first_ucode_section = 0;
	} else {
		shift_param = 16;
		(*first_ucode_section)++;
	}

	for (i = *first_ucode_section; i < IWM_UCODE_SECT_MAX; i++) {
		last_read_idx = i;
		data = fws->fw_sect[i].fws_data;
		dlen = fws->fw_sect[i].fws_len;
		offset = fws->fw_sect[i].fws_devoff;

		/*
		 * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
		 * CPU1 to CPU2.
		 * PAGING_SEPARATOR_SECTION delimiter - separate between
		 * CPU2 non paged to CPU2 paging sec.
		 */
		if (!data || offset == IWM_CPU1_CPU2_SEPARATOR_SECTION ||
		    offset == IWM_PAGING_SEPARATOR_SECTION)
			break;

		if (dlen > sc->sc_fwdmasegsz) {
			err = EFBIG;
		} else
			err = iwm_firmware_load_sect(sc, offset, data, dlen);
		if (err) {
			DPRINTF(("%s: could not load firmware chunk %d "
			    "(error %d)\n", DEVNAME(sc), i, err));
			return err;
		}

		/* Notify the ucode of the loaded section number and status */
		if (iwm_nic_lock(sc)) {
			val = IWM_READ(sc, IWM_FH_UCODE_LOAD_STATUS);
			val = val | (sec_num << shift_param);
			IWM_WRITE(sc, IWM_FH_UCODE_LOAD_STATUS, val);
			sec_num = (sec_num << 1) | 0x1;
			iwm_nic_unlock(sc);

			/*
			 * The firmware won't load correctly without this delay.
			 */
			DELAY(8000);
		}
	}

	*first_ucode_section = last_read_idx;

	if (iwm_nic_lock(sc)) {
		if (cpu == 1)
			IWM_WRITE(sc, IWM_FH_UCODE_LOAD_STATUS, 0xFFFF);
		else
			IWM_WRITE(sc, IWM_FH_UCODE_LOAD_STATUS, 0xFFFFFFFF);
		iwm_nic_unlock(sc);
	}

	return 0;
}

static int
iwm_load_firmware_8000(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	struct iwm_fw_sects *fws;
	int err = 0;
	int first_ucode_section;

	fws = &sc->sc_fw.fw_sects[ucode_type];

	/* configure the ucode to be ready to get the secured image */
	/* release CPU reset */
	if (iwm_nic_lock(sc)) {
		iwm_write_prph(sc, IWM_RELEASE_CPU_RESET,
		    IWM_RELEASE_CPU_RESET_BIT);
		iwm_nic_unlock(sc);
	}

	/* load to FW the binary Secured sections of CPU1 */
	err = iwm_load_cpu_sections_8000(sc, fws, 1, &first_ucode_section);
	if (err)
		return err;

	/* load to FW the binary sections of CPU2 */
	return iwm_load_cpu_sections_8000(sc, fws, 2, &first_ucode_section);
}

static int
iwm_load_firmware(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	int err, w;

	sc->sc_uc.uc_intr = 0;

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000)
		err = iwm_load_firmware_8000(sc, ucode_type);
	else
		err = iwm_load_firmware_7000(sc, ucode_type);
	if (err)
		return err;

	/* wait for the firmware to load */
	for (w = 0; !sc->sc_uc.uc_intr && w < 10; w++)
		err = tsleep(&sc->sc_uc, 0, "iwmuc", mstohz(100));
	if (err || !sc->sc_uc.uc_ok) {
		aprint_error_dev(sc->sc_dev,
		    "could not load firmware (error %d, ok %d)\n",
		    err, sc->sc_uc.uc_ok);
		if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000) {
			aprint_error_dev(sc->sc_dev, "cpu1 status: 0x%x\n",
			    iwm_read_prph(sc, IWM_SB_CPU_1_STATUS));
			aprint_error_dev(sc->sc_dev, "cpu2 status: 0x%x\n",
			    iwm_read_prph(sc, IWM_SB_CPU_2_STATUS));
		}
	}

	return err;
}

static int
iwm_start_fw(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	int err;

	IWM_WRITE(sc, IWM_CSR_INT, ~0);

	err = iwm_nic_init(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "Unable to init nic\n");
		return err;
	}

	/* make sure rfkill handshake bits are cleared */
	IWM_WRITE(sc, IWM_CSR_UCODE_DRV_GP1_CLR, IWM_CSR_UCODE_SW_BIT_RFKILL);
	IWM_WRITE(sc, IWM_CSR_UCODE_DRV_GP1_CLR,
	    IWM_CSR_UCODE_DRV_GP1_BIT_CMD_BLOCKED);

	/* clear (again), then enable host interrupts */
	IWM_WRITE(sc, IWM_CSR_INT, ~0);
	iwm_enable_interrupts(sc);

	/* really make sure rfkill handshake bits are cleared */
	/* maybe we should write a few times more?  just to make sure */
	IWM_WRITE(sc, IWM_CSR_UCODE_DRV_GP1_CLR, IWM_CSR_UCODE_SW_BIT_RFKILL);
	IWM_WRITE(sc, IWM_CSR_UCODE_DRV_GP1_CLR, IWM_CSR_UCODE_SW_BIT_RFKILL);

	return iwm_load_firmware(sc, ucode_type);
}

static int
iwm_send_tx_ant_cfg(struct iwm_softc *sc, uint8_t valid_tx_ant)
{
	struct iwm_tx_ant_cfg_cmd tx_ant_cmd = {
		.valid = htole32(valid_tx_ant),
	};

	return iwm_send_cmd_pdu(sc, IWM_TX_ANT_CONFIGURATION_CMD, 0,
	    sizeof(tx_ant_cmd), &tx_ant_cmd);
}

static int
iwm_send_phy_cfg_cmd(struct iwm_softc *sc)
{
	struct iwm_phy_cfg_cmd phy_cfg_cmd;
	enum iwm_ucode_type ucode_type = sc->sc_uc_current;

	phy_cfg_cmd.phy_cfg = htole32(sc->sc_fw_phy_config);
	phy_cfg_cmd.calib_control.event_trigger =
	    sc->sc_default_calib[ucode_type].event_trigger;
	phy_cfg_cmd.calib_control.flow_trigger =
	    sc->sc_default_calib[ucode_type].flow_trigger;

	DPRINTFN(10, ("Sending Phy CFG command: 0x%x\n", phy_cfg_cmd.phy_cfg));
	return iwm_send_cmd_pdu(sc, IWM_PHY_CONFIGURATION_CMD, 0,
	    sizeof(phy_cfg_cmd), &phy_cfg_cmd);
}

static int
iwm_load_ucode_wait_alive(struct iwm_softc *sc, enum iwm_ucode_type ucode_type)
{
	struct iwm_fw_sects *fws;
	enum iwm_ucode_type old_type = sc->sc_uc_current;
	int err;

	err = iwm_read_firmware(sc, ucode_type);
	if (err)
		return err;

	sc->sc_uc_current = ucode_type;
	err = iwm_start_fw(sc, ucode_type);
	if (err) {
		sc->sc_uc_current = old_type;
		return err;
	}

	err = iwm_post_alive(sc);
	if (err)
		return err;

	fws = &sc->sc_fw.fw_sects[ucode_type];
	if (fws->paging_mem_size) {
		err = iwm_save_fw_paging(sc, fws);
		if (err)
			return err;

		err = iwm_send_paging_cmd(sc, fws);
		if (err) {
			iwm_free_fw_paging(sc);
			return err;
		}
	}

	return 0;
}

static int
iwm_run_init_mvm_ucode(struct iwm_softc *sc, int justnvm)
{
	int err;

	if ((sc->sc_flags & IWM_FLAG_RFKILL) && !justnvm) {
		aprint_error_dev(sc->sc_dev,
		    "radio is disabled by hardware switch\n");
		return EPERM;
	}

	sc->sc_init_complete = 0;
	err = iwm_load_ucode_wait_alive(sc, IWM_UCODE_TYPE_INIT);
	if (err) {
		DPRINTF(("%s: failed to load init firmware\n", DEVNAME(sc)));
		return err;
	}

	if (justnvm) {
		err = iwm_nvm_init(sc);
		if (err) {
			aprint_error_dev(sc->sc_dev, "failed to read nvm\n");
			return err;
		}

		memcpy(&sc->sc_ic.ic_myaddr, &sc->sc_nvm.hw_addr,
		    ETHER_ADDR_LEN);
		return 0;
	}

	err = iwm_send_bt_init_conf(sc);
	if (err)
		return err;

	err = iwm_sf_config(sc, IWM_SF_INIT_OFF);
	if (err)
		return err;

	err = iwm_send_tx_ant_cfg(sc, iwm_fw_valid_tx_ant(sc));
	if (err)
		return err;

	/*
	 * Send phy configurations command to init uCode
	 * to start the 16.0 uCode init image internal calibrations.
	 */
	err = iwm_send_phy_cfg_cmd(sc);
	if (err)
		return err;

	/*
	 * Nothing to do but wait for the init complete notification
	 * from the firmware
	 */
	while (!sc->sc_init_complete) {
		err = tsleep(&sc->sc_init_complete, 0, "iwminit", mstohz(2000));
		if (err)
			break;
	}

	return err;
}

static int
iwm_rx_addbuf(struct iwm_softc *sc, int size, int idx)
{
	struct iwm_rx_ring *ring = &sc->rxq;
	struct iwm_rx_data *data = &ring->data[idx];
	struct mbuf *m;
	int err;
	int fatal = 0;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;

	if (size <= MCLBYTES) {
		MCLGET(m, M_DONTWAIT);
	} else {
		MEXTMALLOC(m, IWM_RBUF_SIZE, M_DONTWAIT);
	}
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}

	if (data->m != NULL) {
		bus_dmamap_unload(sc->sc_dmat, data->map);
		fatal = 1;
	}

	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	err = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (err) {
		/* XXX */
		if (fatal)
			panic("iwm: could not load RX mbuf");
		m_freem(m);
		return err;
	}
	data->m = m;
	bus_dmamap_sync(sc->sc_dmat, data->map, 0, size, BUS_DMASYNC_PREREAD);

	/* Update RX descriptor. */
	ring->desc[idx] = htole32(data->map->dm_segs[0].ds_addr >> 8);
	bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map,
	    idx * sizeof(uint32_t), sizeof(uint32_t), BUS_DMASYNC_PREWRITE);

	return 0;
}

#define IWM_RSSI_OFFSET 50
static int
iwm_calc_rssi(struct iwm_softc *sc, struct iwm_rx_phy_info *phy_info)
{
	int rssi_a, rssi_b, rssi_a_dbm, rssi_b_dbm, max_rssi_dbm;
	uint32_t agc_a, agc_b;
	uint32_t val;

	val = le32toh(phy_info->non_cfg_phy[IWM_RX_INFO_AGC_IDX]);
	agc_a = (val & IWM_OFDM_AGC_A_MSK) >> IWM_OFDM_AGC_A_POS;
	agc_b = (val & IWM_OFDM_AGC_B_MSK) >> IWM_OFDM_AGC_B_POS;

	val = le32toh(phy_info->non_cfg_phy[IWM_RX_INFO_RSSI_AB_IDX]);
	rssi_a = (val & IWM_OFDM_RSSI_INBAND_A_MSK) >> IWM_OFDM_RSSI_A_POS;
	rssi_b = (val & IWM_OFDM_RSSI_INBAND_B_MSK) >> IWM_OFDM_RSSI_B_POS;

	/*
	 * dBm = rssi dB - agc dB - constant.
	 * Higher AGC (higher radio gain) means lower signal.
	 */
	rssi_a_dbm = rssi_a - IWM_RSSI_OFFSET - agc_a;
	rssi_b_dbm = rssi_b - IWM_RSSI_OFFSET - agc_b;
	max_rssi_dbm = MAX(rssi_a_dbm, rssi_b_dbm);

	DPRINTF(("Rssi In A %d B %d Max %d AGCA %d AGCB %d\n",
	    rssi_a_dbm, rssi_b_dbm, max_rssi_dbm, agc_a, agc_b));

	return max_rssi_dbm;
}

/*
 * RSSI values are reported by the FW as positive values - need to negate
 * to obtain their dBM.  Account for missing antennas by replacing 0
 * values by -256dBm: practically 0 power and a non-feasible 8 bit value.
 */
static int
iwm_get_signal_strength(struct iwm_softc *sc, struct iwm_rx_phy_info *phy_info)
{
	int energy_a, energy_b, energy_c, max_energy;
	uint32_t val;

	val = le32toh(phy_info->non_cfg_phy[IWM_RX_INFO_ENERGY_ANT_ABC_IDX]);
	energy_a = (val & IWM_RX_INFO_ENERGY_ANT_A_MSK) >>
	    IWM_RX_INFO_ENERGY_ANT_A_POS;
	energy_a = energy_a ? -energy_a : -256;
	energy_b = (val & IWM_RX_INFO_ENERGY_ANT_B_MSK) >>
	    IWM_RX_INFO_ENERGY_ANT_B_POS;
	energy_b = energy_b ? -energy_b : -256;
	energy_c = (val & IWM_RX_INFO_ENERGY_ANT_C_MSK) >>
	    IWM_RX_INFO_ENERGY_ANT_C_POS;
	energy_c = energy_c ? -energy_c : -256;
	max_energy = MAX(energy_a, energy_b);
	max_energy = MAX(max_energy, energy_c);

	DPRINTFN(12, ("energy In A %d B %d C %d, and max %d\n",
	    energy_a, energy_b, energy_c, max_energy));

	return max_energy;
}

static void
iwm_rx_rx_phy_cmd(struct iwm_softc *sc, struct iwm_rx_packet *pkt,
    struct iwm_rx_data *data)
{
	struct iwm_rx_phy_info *phy_info = (void *)pkt->data;

	DPRINTFN(20, ("received PHY stats\n"));
	bus_dmamap_sync(sc->sc_dmat, data->map, sizeof(*pkt),
	    sizeof(*phy_info), BUS_DMASYNC_POSTREAD);

	memcpy(&sc->sc_last_phy_info, phy_info, sizeof(sc->sc_last_phy_info));
}

/*
 * Retrieve the average noise (in dBm) among receivers.
 */
static int
iwm_get_noise(const struct iwm_statistics_rx_non_phy *stats)
{
	int i, total, nbant, noise;

	total = nbant = noise = 0;
	for (i = 0; i < 3; i++) {
		noise = le32toh(stats->beacon_silence_rssi[i]) & 0xff;
		if (noise) {
			total += noise;
			nbant++;
		}
	}

	/* There should be at least one antenna but check anyway. */
	return (nbant == 0) ? -127 : (total / nbant) - 107;
}

static void
iwm_rx_rx_mpdu(struct iwm_softc *sc, struct iwm_rx_packet *pkt,
    struct iwm_rx_data *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct ieee80211_channel *c = NULL;
	struct mbuf *m;
	struct iwm_rx_phy_info *phy_info;
	struct iwm_rx_mpdu_res_start *rx_res;
	int device_timestamp;
	uint32_t len;
	uint32_t rx_pkt_status;
	int rssi;
	int s;

	bus_dmamap_sync(sc->sc_dmat, data->map, 0, IWM_RBUF_SIZE,
	    BUS_DMASYNC_POSTREAD);

	phy_info = &sc->sc_last_phy_info;
	rx_res = (struct iwm_rx_mpdu_res_start *)pkt->data;
	wh = (struct ieee80211_frame *)(pkt->data + sizeof(*rx_res));
	len = le16toh(rx_res->byte_count);
	rx_pkt_status = le32toh(*(uint32_t *)(pkt->data +
	    sizeof(*rx_res) + len));

	m = data->m;
	m->m_data = pkt->data + sizeof(*rx_res);
	m->m_pkthdr.len = m->m_len = len;

	if (__predict_false(phy_info->cfg_phy_cnt > 20)) {
		DPRINTF(("dsp size out of range [0,20]: %d\n",
		    phy_info->cfg_phy_cnt));
		return;
	}

	if (!(rx_pkt_status & IWM_RX_MPDU_RES_STATUS_CRC_OK) ||
	    !(rx_pkt_status & IWM_RX_MPDU_RES_STATUS_OVERRUN_OK)) {
		DPRINTF(("Bad CRC or FIFO: 0x%08X.\n", rx_pkt_status));
		return; /* drop */
	}

	device_timestamp = le32toh(phy_info->system_timestamp);

	if (sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_RX_ENERGY_API) {
		rssi = iwm_get_signal_strength(sc, phy_info);
	} else {
		rssi = iwm_calc_rssi(sc, phy_info);
	}
	rssi = -rssi;

	if (ic->ic_state == IEEE80211_S_SCAN)
		iwm_fix_channel(sc, m);

	if (iwm_rx_addbuf(sc, IWM_RBUF_SIZE, sc->rxq.cur) != 0)
		return;

	m_set_rcvif(m, IC2IFP(ic));

	if (le32toh(phy_info->channel) < __arraycount(ic->ic_channels))
		c = &ic->ic_channels[le32toh(phy_info->channel)];

	s = splnet();

	ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)wh);
	if (c)
		ni->ni_chan = c;

	if (__predict_false(sc->sc_drvbpf != NULL)) {
		struct iwm_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		if (phy_info->phy_flags & htole16(IWM_PHY_INFO_FLAG_SHPREAMBLE))
			tap->wr_flags |= IEEE80211_RADIOTAP_F_SHORTPRE;
		tap->wr_chan_freq =
		    htole16(ic->ic_channels[phy_info->channel].ic_freq);
		tap->wr_chan_flags =
		    htole16(ic->ic_channels[phy_info->channel].ic_flags);
		tap->wr_dbm_antsignal = (int8_t)rssi;
		tap->wr_dbm_antnoise = (int8_t)sc->sc_noise;
		tap->wr_tsft = phy_info->system_timestamp;
		if (phy_info->phy_flags &
		    htole16(IWM_RX_RES_PHY_FLAGS_OFDM_HT)) {
			uint8_t mcs = (phy_info->rate_n_flags &
			    htole32(IWM_RATE_HT_MCS_RATE_CODE_MSK |
			      IWM_RATE_HT_MCS_NSS_MSK));
			tap->wr_rate = (0x80 | mcs);
		} else {
			uint8_t rate = (phy_info->rate_n_flags &
			    htole32(IWM_RATE_LEGACY_RATE_MSK));
			switch (rate) {
			/* CCK rates. */
			case  10: tap->wr_rate =   2; break;
			case  20: tap->wr_rate =   4; break;
			case  55: tap->wr_rate =  11; break;
			case 110: tap->wr_rate =  22; break;
			/* OFDM rates. */
			case 0xd: tap->wr_rate =  12; break;
			case 0xf: tap->wr_rate =  18; break;
			case 0x5: tap->wr_rate =  24; break;
			case 0x7: tap->wr_rate =  36; break;
			case 0x9: tap->wr_rate =  48; break;
			case 0xb: tap->wr_rate =  72; break;
			case 0x1: tap->wr_rate =  96; break;
			case 0x3: tap->wr_rate = 108; break;
			/* Unknown rate: should not happen. */
			default:  tap->wr_rate =   0;
			}
		}

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_rxtap_len, m);
	}
	ieee80211_input(ic, m, ni, rssi, device_timestamp);
	ieee80211_free_node(ni);

	splx(s);
}

static void
iwm_rx_tx_cmd_single(struct iwm_softc *sc, struct iwm_rx_packet *pkt,
    struct iwm_node *in)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = IC2IFP(ic);
	struct iwm_tx_resp *tx_resp = (void *)pkt->data;
	int status = le16toh(tx_resp->status.status) & IWM_TX_STATUS_MSK;
	int failack = tx_resp->failure_frame;

	KASSERT(tx_resp->frame_count == 1);

	/* Update rate control statistics. */
	in->in_amn.amn_txcnt++;
	if (failack > 0) {
		in->in_amn.amn_retrycnt++;
	}

	if (status != IWM_TX_STATUS_SUCCESS &&
	    status != IWM_TX_STATUS_DIRECT_DONE)
		ifp->if_oerrors++;
	else
		ifp->if_opackets++;
}

static void
iwm_rx_tx_cmd(struct iwm_softc *sc, struct iwm_rx_packet *pkt,
    struct iwm_rx_data *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = IC2IFP(ic);
	struct iwm_cmd_header *cmd_hdr = &pkt->hdr;
	int idx = cmd_hdr->idx;
	int qid = cmd_hdr->qid;
	struct iwm_tx_ring *ring = &sc->txq[qid];
	struct iwm_tx_data *txd = &ring->data[idx];
	struct iwm_node *in = txd->in;
	int s;

	s = splnet();

	if (txd->done) {
		DPRINTF(("%s: got tx interrupt that's already been handled!\n",
		    DEVNAME(sc)));
		splx(s);
		return;
	}

	bus_dmamap_sync(sc->sc_dmat, data->map, 0, IWM_RBUF_SIZE,
	    BUS_DMASYNC_POSTREAD);

	sc->sc_tx_timer = 0;

	iwm_rx_tx_cmd_single(sc, pkt, in);

	bus_dmamap_sync(sc->sc_dmat, txd->map, 0, txd->map->dm_mapsize,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, txd->map);
	m_freem(txd->m);

	DPRINTFN(8, ("free txd %p, in %p\n", txd, txd->in));
	KASSERT(txd->done == 0);
	txd->done = 1;
	KASSERT(txd->in);

	txd->m = NULL;
	txd->in = NULL;
	ieee80211_free_node(&in->in_ni);

	if (--ring->queued < IWM_TX_RING_LOMARK) {
		sc->qfullmsk &= ~(1 << qid);
		if (sc->qfullmsk == 0 && (ifp->if_flags & IFF_OACTIVE)) {
			ifp->if_flags &= ~IFF_OACTIVE;
			KASSERT(KERNEL_LOCKED_P());
			iwm_start(ifp);
		}
	}

	splx(s);
}

static int
iwm_binding_cmd(struct iwm_softc *sc, struct iwm_node *in, uint32_t action)
{
	struct iwm_binding_cmd cmd;
	struct iwm_phy_ctxt *phyctxt = in->in_phyctxt;
	int i, err;
	uint32_t status;

	memset(&cmd, 0, sizeof(cmd));

	cmd.id_and_color
	    = htole32(IWM_FW_CMD_ID_AND_COLOR(phyctxt->id, phyctxt->color));
	cmd.action = htole32(action);
	cmd.phy = htole32(IWM_FW_CMD_ID_AND_COLOR(phyctxt->id, phyctxt->color));

	cmd.macs[0] = htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id, in->in_color));
	for (i = 1; i < IWM_MAX_MACS_IN_BINDING; i++)
		cmd.macs[i] = htole32(IWM_FW_CTXT_INVALID);

	status = 0;
	err = iwm_send_cmd_pdu_status(sc, IWM_BINDING_CONTEXT_CMD,
	    sizeof(cmd), &cmd, &status);
	if (err == 0 && status != 0)
		err = EIO;

	return err;
}

static void
iwm_phy_ctxt_cmd_hdr(struct iwm_softc *sc, struct iwm_phy_ctxt *ctxt,
    struct iwm_phy_context_cmd *cmd, uint32_t action, uint32_t apply_time)
{
	memset(cmd, 0, sizeof(struct iwm_phy_context_cmd));

	cmd->id_and_color = htole32(IWM_FW_CMD_ID_AND_COLOR(ctxt->id,
	    ctxt->color));
	cmd->action = htole32(action);
	cmd->apply_time = htole32(apply_time);
}

static void
iwm_phy_ctxt_cmd_data(struct iwm_softc *sc, struct iwm_phy_context_cmd *cmd,
    struct ieee80211_channel *chan, uint8_t chains_static,
    uint8_t chains_dynamic)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t active_cnt, idle_cnt;

	cmd->ci.band = IEEE80211_IS_CHAN_2GHZ(chan) ?
	    IWM_PHY_BAND_24 : IWM_PHY_BAND_5;

	cmd->ci.channel = ieee80211_chan2ieee(ic, chan);
	cmd->ci.width = IWM_PHY_VHT_CHANNEL_MODE20;
	cmd->ci.ctrl_pos = IWM_PHY_VHT_CTRL_POS_1_BELOW;

	/* Set rx the chains */
	idle_cnt = chains_static;
	active_cnt = chains_dynamic;

	cmd->rxchain_info = htole32(iwm_fw_valid_rx_ant(sc) <<
	    IWM_PHY_RX_CHAIN_VALID_POS);
	cmd->rxchain_info |= htole32(idle_cnt << IWM_PHY_RX_CHAIN_CNT_POS);
	cmd->rxchain_info |= htole32(active_cnt <<
	    IWM_PHY_RX_CHAIN_MIMO_CNT_POS);

	cmd->txchain_info = htole32(iwm_fw_valid_tx_ant(sc));
}

static int
iwm_phy_ctxt_cmd(struct iwm_softc *sc, struct iwm_phy_ctxt *ctxt,
    uint8_t chains_static, uint8_t chains_dynamic, uint32_t action,
    uint32_t apply_time)
{
	struct iwm_phy_context_cmd cmd;

	iwm_phy_ctxt_cmd_hdr(sc, ctxt, &cmd, action, apply_time);

	iwm_phy_ctxt_cmd_data(sc, &cmd, ctxt->channel,
	    chains_static, chains_dynamic);

	return iwm_send_cmd_pdu(sc, IWM_PHY_CONTEXT_CMD, 0,
	    sizeof(struct iwm_phy_context_cmd), &cmd);
}

static int
iwm_send_cmd(struct iwm_softc *sc, struct iwm_host_cmd *hcmd)
{
	struct iwm_tx_ring *ring = &sc->txq[IWM_CMD_QUEUE];
	struct iwm_tfd *desc;
	struct iwm_tx_data *txdata;
	struct iwm_device_cmd *cmd;
	struct mbuf *m;
	bus_addr_t paddr;
	uint32_t addr_lo;
	int err = 0, i, paylen, off, s;
	int code;
	int async, wantresp;
	int group_id;
	size_t hdrlen, datasz;
	uint8_t *data;

	code = hcmd->id;
	async = hcmd->flags & IWM_CMD_ASYNC;
	wantresp = hcmd->flags & IWM_CMD_WANT_SKB;

	for (i = 0, paylen = 0; i < __arraycount(hcmd->len); i++) {
		paylen += hcmd->len[i];
	}

	/* if the command wants an answer, busy sc_cmd_resp */
	if (wantresp) {
		KASSERT(!async);
		while (sc->sc_wantresp != IWM_CMD_RESP_IDLE)
			tsleep(&sc->sc_wantresp, 0, "iwmcmdsl", 0);
		sc->sc_wantresp = ring->qid << 16 | ring->cur;
	}

	/*
	 * Is the hardware still available?  (after e.g. above wait).
	 */
	s = splnet();
	if (sc->sc_flags & IWM_FLAG_STOPPED) {
		err = ENXIO;
		goto out;
	}

	desc = &ring->desc[ring->cur];
	txdata = &ring->data[ring->cur];

	group_id = iwm_cmd_groupid(code);
	if (group_id != 0) {
		hdrlen = sizeof(cmd->hdr_wide);
		datasz = sizeof(cmd->data_wide);
	} else {
		hdrlen = sizeof(cmd->hdr);
		datasz = sizeof(cmd->data);
	}

	if (paylen > datasz) {
		/* Command is too large to fit in pre-allocated space. */
		size_t totlen = hdrlen + paylen;
		if (paylen > IWM_MAX_CMD_PAYLOAD_SIZE) {
			aprint_error_dev(sc->sc_dev,
			    "firmware command too long (%zd bytes)\n", totlen);
			err = EINVAL;
			goto out;
		}
		m = m_gethdr(M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			err = ENOMEM;
			goto out;
		}
		MEXTMALLOC(m, IWM_RBUF_SIZE, M_DONTWAIT);
		if (!(m->m_flags & M_EXT)) {
			aprint_error_dev(sc->sc_dev,
			    "could not get fw cmd mbuf (%zd bytes)\n", totlen);
			m_freem(m);
			err = ENOMEM;
			goto out;
		}
		cmd = mtod(m, struct iwm_device_cmd *);
		err = bus_dmamap_load(sc->sc_dmat, txdata->map, cmd,
		    totlen, NULL, BUS_DMA_NOWAIT | BUS_DMA_WRITE);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not load fw cmd mbuf (%zd bytes)\n", totlen);
			m_freem(m);
			goto out;
		}
		txdata->m = m;
		paddr = txdata->map->dm_segs[0].ds_addr;
	} else {
		cmd = &ring->cmd[ring->cur];
		paddr = txdata->cmd_paddr;
	}

	if (group_id != 0) {
		cmd->hdr_wide.opcode = iwm_cmd_opcode(code);
		cmd->hdr_wide.group_id = group_id;
		cmd->hdr_wide.qid = ring->qid;
		cmd->hdr_wide.idx = ring->cur;
		cmd->hdr_wide.length = htole16(paylen);
		cmd->hdr_wide.version = iwm_cmd_version(code);
		data = cmd->data_wide;
	} else {
		cmd->hdr.code = code;
		cmd->hdr.flags = 0;
		cmd->hdr.qid = ring->qid;
		cmd->hdr.idx = ring->cur;
		data = cmd->data;
	}

	for (i = 0, off = 0; i < __arraycount(hcmd->data); i++) {
		if (hcmd->len[i] == 0)
			continue;
		memcpy(data + off, hcmd->data[i], hcmd->len[i]);
		off += hcmd->len[i];
	}
	KASSERT(off == paylen);

	/* lo field is not aligned */
	addr_lo = htole32((uint32_t)paddr);
	memcpy(&desc->tbs[0].lo, &addr_lo, sizeof(uint32_t));
	desc->tbs[0].hi_n_len  = htole16(iwm_get_dma_hi_addr(paddr)
	    | ((hdrlen + paylen) << 4));
	desc->num_tbs = 1;

	DPRINTFN(8, ("iwm_send_cmd 0x%x size=%zu %s\n",
	    code, hdrlen + paylen, async ? " (async)" : ""));

	if (paylen > datasz) {
		bus_dmamap_sync(sc->sc_dmat, txdata->map, 0, hdrlen + paylen,
		    BUS_DMASYNC_PREWRITE);
	} else {
		bus_dmamap_sync(sc->sc_dmat, ring->cmd_dma.map,
		    (uint8_t *)cmd - (uint8_t *)ring->cmd, hdrlen + paylen,
		    BUS_DMASYNC_PREWRITE);
	}
	bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map,
	    (uint8_t *)desc - (uint8_t *)ring->desc, sizeof(*desc),
	    BUS_DMASYNC_PREWRITE);

	err = iwm_set_cmd_in_flight(sc);
	if (err)
		goto out;
	ring->queued++;

#if 0
	iwm_update_sched(sc, ring->qid, ring->cur, 0, 0);
#endif
	DPRINTF(("sending command 0x%x qid %d, idx %d\n",
	    code, ring->qid, ring->cur));

	/* Kick command ring. */
	ring->cur = (ring->cur + 1) % IWM_TX_RING_COUNT;
	IWM_WRITE(sc, IWM_HBUS_TARG_WRPTR, ring->qid << 8 | ring->cur);

	if (!async) {
		int generation = sc->sc_generation;
		err = tsleep(desc, PCATCH, "iwmcmd", mstohz(2000));
		if (err == 0) {
			/* if hardware is no longer up, return error */
			if (generation != sc->sc_generation) {
				err = ENXIO;
			} else {
				hcmd->resp_pkt = (void *)sc->sc_cmd_resp;
			}
		}
	}
 out:
	if (wantresp && err) {
		iwm_free_resp(sc, hcmd);
	}
	splx(s);

	return err;
}

static int
iwm_send_cmd_pdu(struct iwm_softc *sc, uint32_t id, uint32_t flags,
    uint16_t len, const void *data)
{
	struct iwm_host_cmd cmd = {
		.id = id,
		.len = { len, },
		.data = { data, },
		.flags = flags,
	};

	return iwm_send_cmd(sc, &cmd);
}

static int
iwm_send_cmd_status(struct iwm_softc *sc, struct iwm_host_cmd *cmd,
    uint32_t *status)
{
	struct iwm_rx_packet *pkt;
	struct iwm_cmd_response *resp;
	int err, resp_len;

	KASSERT((cmd->flags & IWM_CMD_WANT_SKB) == 0);
	cmd->flags |= IWM_CMD_WANT_SKB;

	err = iwm_send_cmd(sc, cmd);
	if (err)
		return err;
	pkt = cmd->resp_pkt;

	/* Can happen if RFKILL is asserted */
	if (!pkt) {
		err = 0;
		goto out_free_resp;
	}

	if (pkt->hdr.flags & IWM_CMD_FAILED_MSK) {
		err = EIO;
		goto out_free_resp;
	}

	resp_len = iwm_rx_packet_payload_len(pkt);
	if (resp_len != sizeof(*resp)) {
		err = EIO;
		goto out_free_resp;
	}

	resp = (void *)pkt->data;
	*status = le32toh(resp->status);
 out_free_resp:
	iwm_free_resp(sc, cmd);
	return err;
}

static int
iwm_send_cmd_pdu_status(struct iwm_softc *sc, uint32_t id, uint16_t len,
    const void *data, uint32_t *status)
{
	struct iwm_host_cmd cmd = {
		.id = id,
		.len = { len, },
		.data = { data, },
	};

	return iwm_send_cmd_status(sc, &cmd, status);
}

static void
iwm_free_resp(struct iwm_softc *sc, struct iwm_host_cmd *hcmd)
{
	KASSERT(sc->sc_wantresp != IWM_CMD_RESP_IDLE);
	KASSERT((hcmd->flags & IWM_CMD_WANT_SKB) == IWM_CMD_WANT_SKB);
	sc->sc_wantresp = IWM_CMD_RESP_IDLE;
	wakeup(&sc->sc_wantresp);
}

static void
iwm_cmd_done(struct iwm_softc *sc, int qid, int idx)
{
	struct iwm_tx_ring *ring = &sc->txq[IWM_CMD_QUEUE];
	struct iwm_tx_data *data;
	int s;

	if (qid != IWM_CMD_QUEUE) {
		return;	/* Not a command ack. */
	}

	s = splnet();

	data = &ring->data[idx];

	if (data->m != NULL) {
		bus_dmamap_sync(sc->sc_dmat, data->map, 0,
		    data->map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, data->map);
		m_freem(data->m);
		data->m = NULL;
	}
	wakeup(&ring->desc[idx]);

	if (((idx + ring->queued) % IWM_TX_RING_COUNT) != ring->cur) {
		aprint_error_dev(sc->sc_dev,
		    "Some HCMDs skipped?: idx=%d queued=%d cur=%d\n",
		    idx, ring->queued, ring->cur);
	}

	KASSERT(ring->queued > 0);
	if (--ring->queued == 0)
		iwm_clear_cmd_in_flight(sc);

	splx(s);
}

#if 0
/*
 * necessary only for block ack mode
 */
void
iwm_update_sched(struct iwm_softc *sc, int qid, int idx, uint8_t sta_id,
    uint16_t len)
{
	struct iwm_agn_scd_bc_tbl *scd_bc_tbl;
	uint16_t w_val;

	scd_bc_tbl = sc->sched_dma.vaddr;

	len += 8; /* magic numbers came naturally from paris */
	if (sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_DW_BC_TABLE)
		len = roundup(len, 4) / 4;

	w_val = htole16(sta_id << 12 | len);

	/* Update TX scheduler. */
	scd_bc_tbl[qid].tfd_offset[idx] = w_val;
	bus_dmamap_sync(sc->sc_dmat, sc->sched_dma.map,
	    (char *)(void *)w - (char *)(void *)sc->sched_dma.vaddr,
	    sizeof(uint16_t), BUS_DMASYNC_PREWRITE);

	/* I really wonder what this is ?!? */
	if (idx < IWM_TFD_QUEUE_SIZE_BC_DUP) {
		scd_bc_tbl[qid].tfd_offset[IWM_TFD_QUEUE_SIZE_MAX + idx] = w_val;
		bus_dmamap_sync(sc->sc_dmat, sc->sched_dma.map,
		    (char *)(void *)(w + IWM_TFD_QUEUE_SIZE_MAX) -
		    (char *)(void *)sc->sched_dma.vaddr,
		    sizeof (uint16_t), BUS_DMASYNC_PREWRITE);
	}
}
#endif

/*
 * Fill in various bit for management frames, and leave them
 * unfilled for data frames (firmware takes care of that).
 * Return the selected TX rate.
 */
static const struct iwm_rate *
iwm_tx_fill_cmd(struct iwm_softc *sc, struct iwm_node *in,
    struct ieee80211_frame *wh, struct iwm_tx_cmd *tx)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = &in->in_ni;
	const struct iwm_rate *rinfo;
	int type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
	int ridx, rate_flags, i, ind;
	int nrates = ni->ni_rates.rs_nrates;

	tx->rts_retry_limit = IWM_RTS_DFAULT_RETRY_LIMIT;
	tx->data_retry_limit = IWM_DEFAULT_TX_RETRY;

	if (IEEE80211_IS_MULTICAST(wh->i_addr1) ||
	    type != IEEE80211_FC0_TYPE_DATA) {
		/* for non-data, use the lowest supported rate */
		ridx = (IEEE80211_IS_CHAN_5GHZ(ni->ni_chan)) ?
		    IWM_RIDX_OFDM : IWM_RIDX_CCK;
		tx->data_retry_limit = IWM_MGMT_DFAULT_RETRY_LIMIT;
#ifndef IEEE80211_NO_HT
	} else if (ic->ic_fixed_mcs != -1) {
		ridx = sc->sc_fixed_ridx;
#endif
	} else if (ic->ic_fixed_rate != -1) {
		ridx = sc->sc_fixed_ridx;
	} else {
		/* for data frames, use RS table */
		tx->initial_rate_index = 0;
		tx->tx_flags |= htole32(IWM_TX_CMD_FLG_STA_RATE);
		DPRINTFN(12, ("start with txrate %d\n",
		    tx->initial_rate_index));
#ifndef IEEE80211_NO_HT
		if (ni->ni_flags & IEEE80211_NODE_HT) {
			ridx = iwm_mcs2ridx[ni->ni_txmcs];
			return &iwm_rates[ridx];
		}
#endif
		ridx = (IEEE80211_IS_CHAN_5GHZ(ni->ni_chan)) ?
		    IWM_RIDX_OFDM : IWM_RIDX_CCK;
		for (i = 0; i < nrates; i++) {
			if (iwm_rates[i].rate == (ni->ni_txrate &
			    IEEE80211_RATE_VAL)) {
				ridx = i;
				break;
			}
		}
		return &iwm_rates[ridx];
	}

	rinfo = &iwm_rates[ridx];
	for (i = 0, ind = sc->sc_mgmt_last_antenna;
	    i < IWM_RATE_MCS_ANT_NUM; i++) {
		ind = (ind + 1) % IWM_RATE_MCS_ANT_NUM;
		if (iwm_fw_valid_tx_ant(sc) & (1 << ind)) {
			sc->sc_mgmt_last_antenna = ind;
			break;
		}
	}
	rate_flags = (1 << sc->sc_mgmt_last_antenna) << IWM_RATE_MCS_ANT_POS;
	if (IWM_RIDX_IS_CCK(ridx))
		rate_flags |= IWM_RATE_MCS_CCK_MSK;
#ifndef IEEE80211_NO_HT
	if ((ni->ni_flags & IEEE80211_NODE_HT) &&
	    rinfo->ht_plcp != IWM_RATE_HT_SISO_MCS_INV_PLCP) {
		rate_flags |= IWM_RATE_MCS_HT_MSK;
		tx->rate_n_flags = htole32(rate_flags | rinfo->ht_plcp);
	} else
#endif
		tx->rate_n_flags = htole32(rate_flags | rinfo->plcp);

	return rinfo;
}

#define TB0_SIZE 16
static int
iwm_tx(struct iwm_softc *sc, struct mbuf *m, struct ieee80211_node *ni, int ac)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ni;
	struct iwm_tx_ring *ring;
	struct iwm_tx_data *data;
	struct iwm_tfd *desc;
	struct iwm_device_cmd *cmd;
	struct iwm_tx_cmd *tx;
	struct ieee80211_frame *wh;
	struct ieee80211_key *k = NULL;
	struct mbuf *m1;
	const struct iwm_rate *rinfo;
	uint32_t flags;
	u_int hdrlen;
	bus_dma_segment_t *seg;
	uint8_t tid, type;
	int i, totlen, err, pad;

	wh = mtod(m, struct ieee80211_frame *);
	hdrlen = ieee80211_anyhdrsize(wh);
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;

	tid = 0;

	ring = &sc->txq[ac];
	desc = &ring->desc[ring->cur];
	memset(desc, 0, sizeof(*desc));
	data = &ring->data[ring->cur];

	cmd = &ring->cmd[ring->cur];
	cmd->hdr.code = IWM_TX_CMD;
	cmd->hdr.flags = 0;
	cmd->hdr.qid = ring->qid;
	cmd->hdr.idx = ring->cur;

	tx = (void *)cmd->data;
	memset(tx, 0, sizeof(*tx));

	rinfo = iwm_tx_fill_cmd(sc, in, wh, tx);

	if (__predict_false(sc->sc_drvbpf != NULL)) {
		struct iwm_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		tap->wt_chan_freq = htole16(ni->ni_chan->ic_freq);
		tap->wt_chan_flags = htole16(ni->ni_chan->ic_flags);
#ifndef IEEE80211_NO_HT
		if ((ni->ni_flags & IEEE80211_NODE_HT) &&
		    !IEEE80211_IS_MULTICAST(wh->i_addr1) &&
		    type == IEEE80211_FC0_TYPE_DATA &&
		    rinfo->plcp == IWM_RATE_INVM_PLCP) {
			tap->wt_rate = (0x80 | rinfo->ht_plcp);
		} else
#endif
			tap->wt_rate = rinfo->rate;
		tap->wt_hwqueue = ac;
		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
			tap->wt_flags |= IEEE80211_RADIOTAP_F_WEP;

		bpf_mtap2(sc->sc_drvbpf, tap, sc->sc_txtap_len, m);
	}

	/* Encrypt the frame if need be. */
	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		k = ieee80211_crypto_encap(ic, ni, m);
		if (k == NULL) {
			m_freem(m);
			return ENOBUFS;
		}
		/* Packet header may have moved, reset our local pointer. */
		wh = mtod(m, struct ieee80211_frame *);
	}
	totlen = m->m_pkthdr.len;

	flags = 0;
	if (!IEEE80211_IS_MULTICAST(wh->i_addr1)) {
		flags |= IWM_TX_CMD_FLG_ACK;
	}

	if (type == IEEE80211_FC0_TYPE_DATA &&
	    !IEEE80211_IS_MULTICAST(wh->i_addr1) &&
	    (totlen + IEEE80211_CRC_LEN > ic->ic_rtsthreshold ||
	     (ic->ic_flags & IEEE80211_F_USEPROT)))
		flags |= IWM_TX_CMD_FLG_PROT_REQUIRE;

	if (IEEE80211_IS_MULTICAST(wh->i_addr1) ||
	    type != IEEE80211_FC0_TYPE_DATA)
		tx->sta_id = IWM_AUX_STA_ID;
	else
		tx->sta_id = IWM_STATION_ID;

	if (type == IEEE80211_FC0_TYPE_MGT) {
		uint8_t subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

		if (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ ||
		    subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ)
			tx->pm_frame_timeout = htole16(IWM_PM_FRAME_ASSOC);
		else
			tx->pm_frame_timeout = htole16(IWM_PM_FRAME_MGMT);
	} else {
		tx->pm_frame_timeout = htole16(IWM_PM_FRAME_NONE);
	}

	if (hdrlen & 3) {
		/* First segment length must be a multiple of 4. */
		flags |= IWM_TX_CMD_FLG_MH_PAD;
		pad = 4 - (hdrlen & 3);
	} else
		pad = 0;

	tx->driver_txop = 0;
	tx->next_frame_len = 0;

	tx->len = htole16(totlen);
	tx->tid_tspec = tid;
	tx->life_time = htole32(IWM_TX_CMD_LIFE_TIME_INFINITE);

	/* Set physical address of "scratch area". */
	tx->dram_lsb_ptr = htole32(data->scratch_paddr);
	tx->dram_msb_ptr = iwm_get_dma_hi_addr(data->scratch_paddr);

	/* Copy 802.11 header in TX command. */
	memcpy(tx + 1, wh, hdrlen);

	flags |= IWM_TX_CMD_FLG_BT_DIS | IWM_TX_CMD_FLG_SEQ_CTL;

	tx->sec_ctl = 0;
	tx->tx_flags |= htole32(flags);

	/* Trim 802.11 header. */
	m_adj(m, hdrlen);

	err = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m,
	    BUS_DMA_NOWAIT | BUS_DMA_WRITE);
	if (err) {
		if (err != EFBIG) {
			aprint_error_dev(sc->sc_dev,
			    "can't map mbuf (error %d)\n", err);
			m_freem(m);
			return err;
		}
		/* Too many DMA segments, linearize mbuf. */
		MGETHDR(m1, M_DONTWAIT, MT_DATA);
		if (m1 == NULL) {
			m_freem(m);
			return ENOBUFS;
		}
		if (m->m_pkthdr.len > MHLEN) {
			MCLGET(m1, M_DONTWAIT);
			if (!(m1->m_flags & M_EXT)) {
				m_freem(m);
				m_freem(m1);
				return ENOBUFS;
			}
		}
		m_copydata(m, 0, m->m_pkthdr.len, mtod(m1, void *));
		m1->m_pkthdr.len = m1->m_len = m->m_pkthdr.len;
		m_freem(m);
		m = m1;

		err = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m,
		    BUS_DMA_NOWAIT | BUS_DMA_WRITE);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "can't map mbuf (error %d)\n", err);
			m_freem(m);
			return err;
		}
	}
	data->m = m;
	data->in = in;
	data->done = 0;

	DPRINTFN(8, ("sending txd %p, in %p\n", data, data->in));
	KASSERT(data->in != NULL);

	DPRINTFN(8, ("sending data: qid=%d idx=%d len=%d nsegs=%d type=%d "
	    "subtype=%x tx_flags=%08x init_rateidx=%08x rate_n_flags=%08x\n",
	    ring->qid, ring->cur, totlen, data->map->dm_nsegs, type,
	    (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) >> 4,
	    le32toh(tx->tx_flags), le32toh(tx->initial_rate_index),
	    le32toh(tx->rate_n_flags)));

	/* Fill TX descriptor. */
	desc->num_tbs = 2 + data->map->dm_nsegs;

	desc->tbs[0].lo = htole32(data->cmd_paddr);
	desc->tbs[0].hi_n_len = htole16(iwm_get_dma_hi_addr(data->cmd_paddr)) |
	    (TB0_SIZE << 4);
	desc->tbs[1].lo = htole32(data->cmd_paddr + TB0_SIZE);
	desc->tbs[1].hi_n_len = htole16(iwm_get_dma_hi_addr(data->cmd_paddr)) |
	    ((sizeof(struct iwm_cmd_header) + sizeof(*tx)
	      + hdrlen + pad - TB0_SIZE) << 4);

	/* Other DMA segments are for data payload. */
	seg = data->map->dm_segs;
	for (i = 0; i < data->map->dm_nsegs; i++, seg++) {
		desc->tbs[i+2].lo = htole32(seg->ds_addr);
		desc->tbs[i+2].hi_n_len =
		    htole16(iwm_get_dma_hi_addr(seg->ds_addr))
		    | ((seg->ds_len) << 4);
	}

	bus_dmamap_sync(sc->sc_dmat, data->map, 0, data->map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, ring->cmd_dma.map,
	    (uint8_t *)cmd - (uint8_t *)ring->cmd, sizeof(*cmd),
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map,
	    (uint8_t *)desc - (uint8_t *)ring->desc, sizeof(*desc),
	    BUS_DMASYNC_PREWRITE);

#if 0
	iwm_update_sched(sc, ring->qid, ring->cur, tx->sta_id,
	    le16toh(tx->len));
#endif

	/* Kick TX ring. */
	ring->cur = (ring->cur + 1) % IWM_TX_RING_COUNT;
	IWM_WRITE(sc, IWM_HBUS_TARG_WRPTR, ring->qid << 8 | ring->cur);

	/* Mark TX ring as full if we reach a certain threshold. */
	if (++ring->queued > IWM_TX_RING_HIMARK) {
		sc->qfullmsk |= 1 << ring->qid;
	}

	return 0;
}

#if 0
/* not necessary? */
static int
iwm_flush_tx_path(struct iwm_softc *sc, int tfd_msk, int sync)
{
	struct iwm_tx_path_flush_cmd flush_cmd = {
		.queues_ctl = htole32(tfd_msk),
		.flush_ctl = htole16(IWM_DUMP_TX_FIFO_FLUSH),
	};
	int err;

	err = iwm_send_cmd_pdu(sc, IWM_TXPATH_FLUSH, sync ? 0 : IWM_CMD_ASYNC,
	    sizeof(flush_cmd), &flush_cmd);
	if (err)
		aprint_error_dev(sc->sc_dev, "Flushing tx queue failed: %d\n",
		    err);
	return err;
}
#endif

static void
iwm_led_enable(struct iwm_softc *sc)
{
	IWM_WRITE(sc, IWM_CSR_LED_REG, IWM_CSR_LED_REG_TURN_ON);
}

static void
iwm_led_disable(struct iwm_softc *sc)
{
	IWM_WRITE(sc, IWM_CSR_LED_REG, IWM_CSR_LED_REG_TURN_OFF);
}

static int
iwm_led_is_enabled(struct iwm_softc *sc)
{
	return (IWM_READ(sc, IWM_CSR_LED_REG) == IWM_CSR_LED_REG_TURN_ON);
}

static void
iwm_led_blink_timeout(void *arg)
{
	struct iwm_softc *sc = arg;

	if (iwm_led_is_enabled(sc))
		iwm_led_disable(sc);
	else
		iwm_led_enable(sc);

	callout_schedule(&sc->sc_led_blink_to, mstohz(200));
}

static void
iwm_led_blink_start(struct iwm_softc *sc)
{
	callout_schedule(&sc->sc_led_blink_to, mstohz(200));
}

static void
iwm_led_blink_stop(struct iwm_softc *sc)
{
	callout_stop(&sc->sc_led_blink_to);
	iwm_led_disable(sc);
}

#define IWM_POWER_KEEP_ALIVE_PERIOD_SEC    25

static int
iwm_beacon_filter_send_cmd(struct iwm_softc *sc,
    struct iwm_beacon_filter_cmd *cmd)
{
	return iwm_send_cmd_pdu(sc, IWM_REPLY_BEACON_FILTERING_CMD,
	    0, sizeof(struct iwm_beacon_filter_cmd), cmd);
}

static void
iwm_beacon_filter_set_cqm_params(struct iwm_softc *sc, struct iwm_node *in,
    struct iwm_beacon_filter_cmd *cmd)
{
	cmd->ba_enable_beacon_abort = htole32(sc->sc_bf.ba_enabled);
}

static int
iwm_update_beacon_abort(struct iwm_softc *sc, struct iwm_node *in, int enable)
{
	struct iwm_beacon_filter_cmd cmd = {
		IWM_BF_CMD_CONFIG_DEFAULTS,
		.bf_enable_beacon_filter = htole32(1),
		.ba_enable_beacon_abort = htole32(enable),
	};

	if (!sc->sc_bf.bf_enabled)
		return 0;

	sc->sc_bf.ba_enabled = enable;
	iwm_beacon_filter_set_cqm_params(sc, in, &cmd);
	return iwm_beacon_filter_send_cmd(sc, &cmd);
}

static void
iwm_power_build_cmd(struct iwm_softc *sc, struct iwm_node *in,
    struct iwm_mac_power_cmd *cmd)
{
	struct ieee80211_node *ni = &in->in_ni;
	int dtim_period, dtim_msec, keep_alive;

	cmd->id_and_color = htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id,
	    in->in_color));
	if (ni->ni_dtim_period)
		dtim_period = ni->ni_dtim_period;
	else
		dtim_period = 1;

	/*
	 * Regardless of power management state the driver must set
	 * keep alive period. FW will use it for sending keep alive NDPs
	 * immediately after association. Check that keep alive period
	 * is at least 3 * DTIM.
	 */
	dtim_msec = dtim_period * ni->ni_intval;
	keep_alive = MAX(3 * dtim_msec, 1000 * IWM_POWER_KEEP_ALIVE_PERIOD_SEC);
	keep_alive = roundup(keep_alive, 1000) / 1000;
	cmd->keep_alive_seconds = htole16(keep_alive);

#ifdef notyet
	cmd->flags = htole16(IWM_POWER_FLAGS_POWER_SAVE_ENA_MSK);
	cmd->rx_data_timeout = IWM_DEFAULT_PS_RX_DATA_TIMEOUT;
	cmd->tx_data_timeout = IWM_DEFAULT_PS_TX_DATA_TIMEOUT;
#endif
}

static int
iwm_power_mac_update_mode(struct iwm_softc *sc, struct iwm_node *in)
{
	int err;
	int ba_enable;
	struct iwm_mac_power_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));

	iwm_power_build_cmd(sc, in, &cmd);

	err = iwm_send_cmd_pdu(sc, IWM_MAC_PM_POWER_TABLE, 0,
	    sizeof(cmd), &cmd);
	if (err)
		return err;

	ba_enable = !!(cmd.flags &
	    htole16(IWM_POWER_FLAGS_POWER_MANAGEMENT_ENA_MSK));
	return iwm_update_beacon_abort(sc, in, ba_enable);
}

static int
iwm_power_update_device(struct iwm_softc *sc)
{
	struct iwm_device_power_cmd cmd = {
#ifdef notyet
		.flags = htole16(IWM_DEVICE_POWER_FLAGS_POWER_SAVE_ENA_MSK),
#else
		.flags = 0,
#endif
	};

	if (!(sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_DEVICE_PS_CMD))
		return 0;

	cmd.flags |= htole16(IWM_DEVICE_POWER_FLAGS_CAM_MSK);
	DPRINTF(("Sending device power command with flags = 0x%X\n",
	    cmd.flags));

	return iwm_send_cmd_pdu(sc, IWM_POWER_TABLE_CMD, 0, sizeof(cmd), &cmd);
}

#ifdef notyet
static int
iwm_enable_beacon_filter(struct iwm_softc *sc, struct iwm_node *in)
{
	struct iwm_beacon_filter_cmd cmd = {
		IWM_BF_CMD_CONFIG_DEFAULTS,
		.bf_enable_beacon_filter = htole32(1),
	};
	int err;

	iwm_beacon_filter_set_cqm_params(sc, in, &cmd);
	err = iwm_beacon_filter_send_cmd(sc, &cmd);

	if (err == 0)
		sc->sc_bf.bf_enabled = 1;

	return err;
}
#endif

static int
iwm_disable_beacon_filter(struct iwm_softc *sc)
{
	struct iwm_beacon_filter_cmd cmd;
	int err;

	memset(&cmd, 0, sizeof(cmd));
	if ((sc->sc_capaflags & IWM_UCODE_TLV_FLAGS_BF_UPDATED) == 0)
		return 0;

	err = iwm_beacon_filter_send_cmd(sc, &cmd);
	if (err == 0)
		sc->sc_bf.bf_enabled = 0;

	return err;
}

static int
iwm_add_sta_cmd(struct iwm_softc *sc, struct iwm_node *in, int update)
{
	struct iwm_add_sta_cmd_v7 add_sta_cmd;
	int err;
	uint32_t status;

	memset(&add_sta_cmd, 0, sizeof(add_sta_cmd));

	add_sta_cmd.sta_id = IWM_STATION_ID;
	add_sta_cmd.mac_id_n_color
	    = htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id, in->in_color));
	if (!update) {
		int ac;
		for (ac = 0; ac < WME_NUM_AC; ac++) {
			add_sta_cmd.tfd_queue_msk |=
			    htole32(__BIT(iwm_ac_to_tx_fifo[ac]));
		}
		IEEE80211_ADDR_COPY(&add_sta_cmd.addr, in->in_ni.ni_bssid);
	}
	add_sta_cmd.add_modify = update ? 1 : 0;
	add_sta_cmd.station_flags_msk
	    |= htole32(IWM_STA_FLG_FAT_EN_MSK | IWM_STA_FLG_MIMO_EN_MSK);
	add_sta_cmd.tid_disable_tx = htole16(0xffff);
	if (update)
		add_sta_cmd.modify_mask |= (IWM_STA_MODIFY_TID_DISABLE_TX);

#ifndef IEEE80211_NO_HT
	if (in->in_ni.ni_flags & IEEE80211_NODE_HT) {
		add_sta_cmd.station_flags_msk
		    |= htole32(IWM_STA_FLG_MAX_AGG_SIZE_MSK |
		    IWM_STA_FLG_AGG_MPDU_DENS_MSK);

		add_sta_cmd.station_flags
		    |= htole32(IWM_STA_FLG_MAX_AGG_SIZE_64K);
		switch (ic->ic_ampdu_params & IEEE80211_AMPDU_PARAM_SS) {
		case IEEE80211_AMPDU_PARAM_SS_2:
			add_sta_cmd.station_flags
			    |= htole32(IWM_STA_FLG_AGG_MPDU_DENS_2US);
			break;
		case IEEE80211_AMPDU_PARAM_SS_4:
			add_sta_cmd.station_flags
			    |= htole32(IWM_STA_FLG_AGG_MPDU_DENS_4US);
			break;
		case IEEE80211_AMPDU_PARAM_SS_8:
			add_sta_cmd.station_flags
			    |= htole32(IWM_STA_FLG_AGG_MPDU_DENS_8US);
			break;
		case IEEE80211_AMPDU_PARAM_SS_16:
			add_sta_cmd.station_flags
			    |= htole32(IWM_STA_FLG_AGG_MPDU_DENS_16US);
			break;
		default:
			break;
		}
	}
#endif

	status = IWM_ADD_STA_SUCCESS;
	err = iwm_send_cmd_pdu_status(sc, IWM_ADD_STA, sizeof(add_sta_cmd),
	    &add_sta_cmd, &status);
	if (err == 0 && status != IWM_ADD_STA_SUCCESS)
		err = EIO;

	return err;
}

static int
iwm_add_aux_sta(struct iwm_softc *sc)
{
	struct iwm_add_sta_cmd_v7 cmd;
	int err;
	uint32_t status;

	err = iwm_enable_txq(sc, 0, IWM_AUX_QUEUE, IWM_TX_FIFO_MCAST);
	if (err)
		return err;

	memset(&cmd, 0, sizeof(cmd));
	cmd.sta_id = IWM_AUX_STA_ID;
	cmd.mac_id_n_color =
	    htole32(IWM_FW_CMD_ID_AND_COLOR(IWM_MAC_INDEX_AUX, 0));
	cmd.tfd_queue_msk = htole32(1 << IWM_AUX_QUEUE);
	cmd.tid_disable_tx = htole16(0xffff);

	status = IWM_ADD_STA_SUCCESS;
	err = iwm_send_cmd_pdu_status(sc, IWM_ADD_STA, sizeof(cmd), &cmd,
	    &status);
	if (err == 0 && status != IWM_ADD_STA_SUCCESS)
		err = EIO;

	return err;
}

#define IWM_PLCP_QUIET_THRESH 1
#define IWM_ACTIVE_QUIET_TIME 10
#define LONG_OUT_TIME_PERIOD 600
#define SHORT_OUT_TIME_PERIOD 200
#define SUSPEND_TIME_PERIOD 100

static uint16_t
iwm_scan_rx_chain(struct iwm_softc *sc)
{
	uint16_t rx_chain;
	uint8_t rx_ant;

	rx_ant = iwm_fw_valid_rx_ant(sc);
	rx_chain = rx_ant << IWM_PHY_RX_CHAIN_VALID_POS;
	rx_chain |= rx_ant << IWM_PHY_RX_CHAIN_FORCE_MIMO_SEL_POS;
	rx_chain |= rx_ant << IWM_PHY_RX_CHAIN_FORCE_SEL_POS;
	rx_chain |= 0x1 << IWM_PHY_RX_CHAIN_DRIVER_FORCE_POS;
	return htole16(rx_chain);
}

static uint32_t
iwm_scan_rate_n_flags(struct iwm_softc *sc, int flags, int no_cck)
{
	uint32_t tx_ant;
	int i, ind;

	for (i = 0, ind = sc->sc_scan_last_antenna;
	    i < IWM_RATE_MCS_ANT_NUM; i++) {
		ind = (ind + 1) % IWM_RATE_MCS_ANT_NUM;
		if (iwm_fw_valid_tx_ant(sc) & (1 << ind)) {
			sc->sc_scan_last_antenna = ind;
			break;
		}
	}
	tx_ant = (1 << sc->sc_scan_last_antenna) << IWM_RATE_MCS_ANT_POS;

	if ((flags & IEEE80211_CHAN_2GHZ) && !no_cck)
		return htole32(IWM_RATE_1M_PLCP | IWM_RATE_MCS_CCK_MSK |
				   tx_ant);
	else
		return htole32(IWM_RATE_6M_PLCP | tx_ant);
}

#ifdef notyet
/*
 * If req->n_ssids > 0, it means we should do an active scan.
 * In case of active scan w/o directed scan, we receive a zero-length SSID
 * just to notify that this scan is active and not passive.
 * In order to notify the FW of the number of SSIDs we wish to scan (including
 * the zero-length one), we need to set the corresponding bits in chan->type,
 * one for each SSID, and set the active bit (first). If the first SSID is
 * already included in the probe template, so we need to set only
 * req->n_ssids - 1 bits in addition to the first bit.
 */
static uint16_t
iwm_get_active_dwell(struct iwm_softc *sc, int flags, int n_ssids)
{
	if (flags & IEEE80211_CHAN_2GHZ)
		return 30  + 3 * (n_ssids + 1);
	return 20  + 2 * (n_ssids + 1);
}

static uint16_t
iwm_get_passive_dwell(struct iwm_softc *sc, int flags)
{
	return (flags & IEEE80211_CHAN_2GHZ) ? 100 + 20 : 100 + 10;
}
#endif

static uint8_t
iwm_lmac_scan_fill_channels(struct iwm_softc *sc,
    struct iwm_scan_channel_cfg_lmac *chan, int n_ssids)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_channel *c;
	uint8_t nchan;

	for (nchan = 0, c = &ic->ic_channels[1];
	    c <= &ic->ic_channels[IEEE80211_CHAN_MAX] &&
	    nchan < sc->sc_capa_n_scan_channels;
	    c++) {
		if (c->ic_flags == 0)
			continue;

		chan->channel_num = htole16(ieee80211_mhz2ieee(c->ic_freq, 0));
		chan->iter_count = htole16(1);
		chan->iter_interval = htole32(0);
		chan->flags = htole32(IWM_UNIFIED_SCAN_CHANNEL_PARTIAL);
		chan->flags |= htole32(IWM_SCAN_CHANNEL_NSSIDS(n_ssids));
		if (!IEEE80211_IS_CHAN_PASSIVE(c) && n_ssids != 0)
			chan->flags |= htole32(IWM_SCAN_CHANNEL_TYPE_ACTIVE);
		chan++;
		nchan++;
	}

	return nchan;
}

static uint8_t
iwm_umac_scan_fill_channels(struct iwm_softc *sc,
    struct iwm_scan_channel_cfg_umac *chan, int n_ssids)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_channel *c;
	uint8_t nchan;

	for (nchan = 0, c = &ic->ic_channels[1];
	    c <= &ic->ic_channels[IEEE80211_CHAN_MAX] &&
	    nchan < sc->sc_capa_n_scan_channels;
	    c++) {
		if (c->ic_flags == 0)
			continue;

		chan->channel_num = ieee80211_mhz2ieee(c->ic_freq, 0);
		chan->iter_count = 1;
		chan->iter_interval = htole16(0);
		chan->flags = htole32(IWM_SCAN_CHANNEL_UMAC_NSSIDS(n_ssids));
		chan++;
		nchan++;
	}

	return nchan;
}

static int
iwm_fill_probe_req(struct iwm_softc *sc, struct iwm_scan_probe_req *preq)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_frame *wh = (struct ieee80211_frame *)preq->buf;
	struct ieee80211_rateset *rs;
	size_t remain = sizeof(preq->buf);
	uint8_t *frm, *pos;

	memset(preq, 0, sizeof(*preq));

	if (remain < sizeof(*wh) + 2 + ic->ic_des_esslen)
		return ENOBUFS;

	/*
	 * Build a probe request frame.  Most of the following code is a
	 * copy & paste of what is done in net80211.
	 */
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT |
	    IEEE80211_FC0_SUBTYPE_PROBE_REQ;
	wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
	IEEE80211_ADDR_COPY(wh->i_addr1, etherbroadcastaddr);
	IEEE80211_ADDR_COPY(wh->i_addr2, ic->ic_myaddr);
	IEEE80211_ADDR_COPY(wh->i_addr3, etherbroadcastaddr);
	*(uint16_t *)&wh->i_dur[0] = 0;	/* filled by HW */
	*(uint16_t *)&wh->i_seq[0] = 0;	/* filled by HW */

	frm = (uint8_t *)(wh + 1);
	frm = ieee80211_add_ssid(frm, ic->ic_des_essid, ic->ic_des_esslen);

	/* Tell the firmware where the MAC header is. */
	preq->mac_header.offset = 0;
	preq->mac_header.len = htole16(frm - (uint8_t *)wh);
	remain -= frm - (uint8_t *)wh;

	/* Fill in 2GHz IEs and tell firmware where they are. */
	rs = &ic->ic_sup_rates[IEEE80211_MODE_11G];
	if (rs->rs_nrates > IEEE80211_RATE_SIZE) {
		if (remain < 4 + rs->rs_nrates)
			return ENOBUFS;
	} else if (remain < 2 + rs->rs_nrates)
		return ENOBUFS;
	preq->band_data[0].offset = htole16(frm - (uint8_t *)wh);
	pos = frm;
	frm = ieee80211_add_rates(frm, rs);
	if (rs->rs_nrates > IEEE80211_RATE_SIZE)
		frm = ieee80211_add_xrates(frm, rs);
	preq->band_data[0].len = htole16(frm - pos);
	remain -= frm - pos;

	if (isset(sc->sc_enabled_capa,
	    IWM_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT)) {
		if (remain < 3)
			return ENOBUFS;
		*frm++ = IEEE80211_ELEMID_DSPARMS;
		*frm++ = 1;
		*frm++ = 0;
		remain -= 3;
	}

	if (sc->sc_nvm.sku_cap_band_52GHz_enable) {
		/* Fill in 5GHz IEs. */
		rs = &ic->ic_sup_rates[IEEE80211_MODE_11A];
		if (rs->rs_nrates > IEEE80211_RATE_SIZE) {
			if (remain < 4 + rs->rs_nrates)
				return ENOBUFS;
		} else if (remain < 2 + rs->rs_nrates)
			return ENOBUFS;
		preq->band_data[1].offset = htole16(frm - (uint8_t *)wh);
		pos = frm;
		frm = ieee80211_add_rates(frm, rs);
		if (rs->rs_nrates > IEEE80211_RATE_SIZE)
			frm = ieee80211_add_xrates(frm, rs);
		preq->band_data[1].len = htole16(frm - pos);
		remain -= frm - pos;
	}

#ifndef IEEE80211_NO_HT
	/* Send 11n IEs on both 2GHz and 5GHz bands. */
	preq->common_data.offset = htole16(frm - (uint8_t *)wh);
	pos = frm;
	if (ic->ic_flags & IEEE80211_F_HTON) {
		if (remain < 28)
			return ENOBUFS;
		frm = ieee80211_add_htcaps(frm, ic);
		/* XXX add WME info? */
	}
#endif

	preq->common_data.len = htole16(frm - pos);

	return 0;
}

static int
iwm_lmac_scan(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_host_cmd hcmd = {
		.id = IWM_SCAN_OFFLOAD_REQUEST_CMD,
		.len = { 0, },
		.data = { NULL, },
		.flags = 0,
	};
	struct iwm_scan_req_lmac *req;
	size_t req_len;
	int err;

	DPRINTF(("%s: %s\n", DEVNAME(sc), __func__));

	req_len = sizeof(struct iwm_scan_req_lmac) +
	    (sizeof(struct iwm_scan_channel_cfg_lmac) *
	    sc->sc_capa_n_scan_channels) + sizeof(struct iwm_scan_probe_req);
	if (req_len > IWM_MAX_CMD_PAYLOAD_SIZE)
		return ENOMEM;
	req = kmem_zalloc(req_len, KM_SLEEP);
	hcmd.len[0] = (uint16_t)req_len;
	hcmd.data[0] = (void *)req;

	/* These timings correspond to iwlwifi's UNASSOC scan. */
	req->active_dwell = 10;
	req->passive_dwell = 110;
	req->fragmented_dwell = 44;
	req->extended_dwell = 90;
	req->max_out_time = 0;
	req->suspend_time = 0;

	req->scan_prio = htole32(IWM_SCAN_PRIORITY_HIGH);
	req->rx_chain_select = iwm_scan_rx_chain(sc);
	req->iter_num = htole32(1);
	req->delay = 0;

	req->scan_flags = htole32(IWM_LMAC_SCAN_FLAG_PASS_ALL |
	    IWM_LMAC_SCAN_FLAG_ITER_COMPLETE |
	    IWM_LMAC_SCAN_FLAG_EXTENDED_DWELL);
	if (ic->ic_des_esslen == 0)
		req->scan_flags |= htole32(IWM_LMAC_SCAN_FLAG_PASSIVE);
	else
		req->scan_flags |= htole32(IWM_LMAC_SCAN_FLAG_PRE_CONNECTION);
	if (isset(sc->sc_enabled_capa,
	    IWM_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT))
		req->scan_flags |= htole32(IWM_LMAC_SCAN_FLAGS_RRM_ENABLED);

	req->flags = htole32(IWM_PHY_BAND_24);
	if (sc->sc_nvm.sku_cap_band_52GHz_enable)
		req->flags |= htole32(IWM_PHY_BAND_5);
	req->filter_flags =
	    htole32(IWM_MAC_FILTER_ACCEPT_GRP | IWM_MAC_FILTER_IN_BEACON);

	/* Tx flags 2 GHz. */
	req->tx_cmd[0].tx_flags = htole32(IWM_TX_CMD_FLG_SEQ_CTL |
	    IWM_TX_CMD_FLG_BT_DIS);
	req->tx_cmd[0].rate_n_flags =
	    iwm_scan_rate_n_flags(sc, IEEE80211_CHAN_2GHZ, 1/*XXX*/);
	req->tx_cmd[0].sta_id = IWM_AUX_STA_ID;

	/* Tx flags 5 GHz. */
	req->tx_cmd[1].tx_flags = htole32(IWM_TX_CMD_FLG_SEQ_CTL |
	    IWM_TX_CMD_FLG_BT_DIS);
	req->tx_cmd[1].rate_n_flags =
	    iwm_scan_rate_n_flags(sc, IEEE80211_CHAN_5GHZ, 1/*XXX*/);
	req->tx_cmd[1].sta_id = IWM_AUX_STA_ID;

	/* Check if we're doing an active directed scan. */
	if (ic->ic_des_esslen != 0) {
		req->direct_scan[0].id = IEEE80211_ELEMID_SSID;
		req->direct_scan[0].len = ic->ic_des_esslen;
		memcpy(req->direct_scan[0].ssid, ic->ic_des_essid,
		    ic->ic_des_esslen);
	}

	req->n_channels = iwm_lmac_scan_fill_channels(sc,
	    (struct iwm_scan_channel_cfg_lmac *)req->data,
	    ic->ic_des_esslen != 0);

	err = iwm_fill_probe_req(sc,
	    (struct iwm_scan_probe_req *)(req->data +
	    (sizeof(struct iwm_scan_channel_cfg_lmac) *
	     sc->sc_capa_n_scan_channels)));
	if (err) {
		kmem_free(req, req_len);
		return err;
	}

	/* Specify the scan plan: We'll do one iteration. */
	req->schedule[0].iterations = 1;
	req->schedule[0].full_scan_mul = 1;

	/* Disable EBS. */
	req->channel_opt[0].non_ebs_ratio = 1;
	req->channel_opt[1].non_ebs_ratio = 1;

	err = iwm_send_cmd(sc, &hcmd);
	kmem_free(req, req_len);
	return err;
}

static int
iwm_config_umac_scan(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_scan_config *scan_config;
	int err, nchan;
	size_t cmd_size;
	struct ieee80211_channel *c;
	struct iwm_host_cmd hcmd = {
		.id = iwm_cmd_id(IWM_SCAN_CFG_CMD, IWM_ALWAYS_LONG_GROUP, 0),
		.flags = 0,
	};
	static const uint32_t rates = (IWM_SCAN_CONFIG_RATE_1M |
	    IWM_SCAN_CONFIG_RATE_2M | IWM_SCAN_CONFIG_RATE_5M |
	    IWM_SCAN_CONFIG_RATE_11M | IWM_SCAN_CONFIG_RATE_6M |
	    IWM_SCAN_CONFIG_RATE_9M | IWM_SCAN_CONFIG_RATE_12M |
	    IWM_SCAN_CONFIG_RATE_18M | IWM_SCAN_CONFIG_RATE_24M |
	    IWM_SCAN_CONFIG_RATE_36M | IWM_SCAN_CONFIG_RATE_48M |
	    IWM_SCAN_CONFIG_RATE_54M);

	cmd_size = sizeof(*scan_config) + sc->sc_capa_n_scan_channels;

	scan_config = kmem_zalloc(cmd_size, KM_SLEEP);
	scan_config->tx_chains = htole32(iwm_fw_valid_tx_ant(sc));
	scan_config->rx_chains = htole32(iwm_fw_valid_rx_ant(sc));
	scan_config->legacy_rates = htole32(rates |
	    IWM_SCAN_CONFIG_SUPPORTED_RATE(rates));

	/* These timings correspond to iwlwifi's UNASSOC scan. */
	scan_config->dwell_active = 10;
	scan_config->dwell_passive = 110;
	scan_config->dwell_fragmented = 44;
	scan_config->dwell_extended = 90;
	scan_config->out_of_channel_time = htole32(0);
	scan_config->suspend_time = htole32(0);

	IEEE80211_ADDR_COPY(scan_config->mac_addr, sc->sc_ic.ic_myaddr);

	scan_config->bcast_sta_id = IWM_AUX_STA_ID;
	scan_config->channel_flags = IWM_CHANNEL_FLAG_EBS |
	    IWM_CHANNEL_FLAG_ACCURATE_EBS | IWM_CHANNEL_FLAG_EBS_ADD |
	    IWM_CHANNEL_FLAG_PRE_SCAN_PASSIVE2ACTIVE;

	for (c = &ic->ic_channels[1], nchan = 0;
	    c <= &ic->ic_channels[IEEE80211_CHAN_MAX] &&
	    nchan < sc->sc_capa_n_scan_channels; c++) {
		if (c->ic_flags == 0)
			continue;
		scan_config->channel_array[nchan++] =
		    ieee80211_mhz2ieee(c->ic_freq, 0);
	}

	scan_config->flags = htole32(IWM_SCAN_CONFIG_FLAG_ACTIVATE |
	    IWM_SCAN_CONFIG_FLAG_ALLOW_CHUB_REQS |
	    IWM_SCAN_CONFIG_FLAG_SET_TX_CHAINS |
	    IWM_SCAN_CONFIG_FLAG_SET_RX_CHAINS |
	    IWM_SCAN_CONFIG_FLAG_SET_AUX_STA_ID |
	    IWM_SCAN_CONFIG_FLAG_SET_ALL_TIMES |
	    IWM_SCAN_CONFIG_FLAG_SET_LEGACY_RATES |
	    IWM_SCAN_CONFIG_FLAG_SET_MAC_ADDR |
	    IWM_SCAN_CONFIG_FLAG_SET_CHANNEL_FLAGS|
	    IWM_SCAN_CONFIG_N_CHANNELS(nchan) |
	    IWM_SCAN_CONFIG_FLAG_CLEAR_FRAGMENTED);

	hcmd.data[0] = scan_config;
	hcmd.len[0] = cmd_size;

	err = iwm_send_cmd(sc, &hcmd);
	kmem_free(scan_config, cmd_size);
	return err;
}

static int
iwm_umac_scan(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_host_cmd hcmd = {
		.id = iwm_cmd_id(IWM_SCAN_REQ_UMAC, IWM_ALWAYS_LONG_GROUP, 0),
		.len = { 0, },
		.data = { NULL, },
		.flags = 0,
	};
	struct iwm_scan_req_umac *req;
	struct iwm_scan_req_umac_tail *tail;
	size_t req_len;
	int err;

	DPRINTF(("%s: %s\n", DEVNAME(sc), __func__));

	req_len = sizeof(struct iwm_scan_req_umac) +
	    (sizeof(struct iwm_scan_channel_cfg_umac) *
	    sc->sc_capa_n_scan_channels) +
	    sizeof(struct iwm_scan_req_umac_tail);
	if (req_len > IWM_MAX_CMD_PAYLOAD_SIZE)
		return ENOMEM;
	req = kmem_zalloc(req_len, KM_SLEEP);

	hcmd.len[0] = (uint16_t)req_len;
	hcmd.data[0] = (void *)req;

	/* These timings correspond to iwlwifi's UNASSOC scan. */
	req->active_dwell = 10;
	req->passive_dwell = 110;
	req->fragmented_dwell = 44;
	req->extended_dwell = 90;
	req->max_out_time = 0;
	req->suspend_time = 0;

	req->scan_priority = htole32(IWM_SCAN_PRIORITY_HIGH);
	req->ooc_priority = htole32(IWM_SCAN_PRIORITY_HIGH);

	req->n_channels = iwm_umac_scan_fill_channels(sc,
	    (struct iwm_scan_channel_cfg_umac *)req->data,
	    ic->ic_des_esslen != 0);

	req->general_flags = htole32(IWM_UMAC_SCAN_GEN_FLAGS_PASS_ALL |
	    IWM_UMAC_SCAN_GEN_FLAGS_ITER_COMPLETE |
	    IWM_UMAC_SCAN_GEN_FLAGS_EXTENDED_DWELL);

	tail = (struct iwm_scan_req_umac_tail *)(req->data +
		sizeof(struct iwm_scan_channel_cfg_umac) *
			sc->sc_capa_n_scan_channels);

	/* Check if we're doing an active directed scan. */
	if (ic->ic_des_esslen != 0) {
		tail->direct_scan[0].id = IEEE80211_ELEMID_SSID;
		tail->direct_scan[0].len = ic->ic_des_esslen;
		memcpy(tail->direct_scan[0].ssid, ic->ic_des_essid,
		    ic->ic_des_esslen);
		req->general_flags |=
		    htole32(IWM_UMAC_SCAN_GEN_FLAGS_PRE_CONNECT);
	} else
		req->general_flags |= htole32(IWM_UMAC_SCAN_GEN_FLAGS_PASSIVE);

	if (isset(sc->sc_enabled_capa,
	    IWM_UCODE_TLV_CAPA_DS_PARAM_SET_IE_SUPPORT))
		req->general_flags |=
		    htole32(IWM_UMAC_SCAN_GEN_FLAGS_RRM_ENABLED);

	err = iwm_fill_probe_req(sc, &tail->preq);
	if (err) {
		kmem_free(req, req_len);
		return err;
	}

	/* Specify the scan plan: We'll do one iteration. */
	tail->schedule[0].interval = 0;
	tail->schedule[0].iter_count = 1;

	err = iwm_send_cmd(sc, &hcmd);
	kmem_free(req, req_len);
	return err;
}

static uint8_t
iwm_ridx2rate(struct ieee80211_rateset *rs, int ridx)
{
	int i;
	uint8_t rval;

	for (i = 0; i < rs->rs_nrates; i++) {
		rval = (rs->rs_rates[i] & IEEE80211_RATE_VAL);
		if (rval == iwm_rates[ridx].rate)
			return rs->rs_rates[i];
	}
	return 0;
}

static void
iwm_ack_rates(struct iwm_softc *sc, struct iwm_node *in, int *cck_rates,
    int *ofdm_rates)
{
	struct ieee80211_node *ni = &in->in_ni;
	struct ieee80211_rateset *rs = &ni->ni_rates;
	int lowest_present_ofdm = -1;
	int lowest_present_cck = -1;
	uint8_t cck = 0;
	uint8_t ofdm = 0;
	int i;

	if (ni->ni_chan == IEEE80211_CHAN_ANYC ||
	    IEEE80211_IS_CHAN_2GHZ(ni->ni_chan)) {
		for (i = IWM_FIRST_CCK_RATE; i < IWM_FIRST_OFDM_RATE; i++) {
			if ((iwm_ridx2rate(rs, i) & IEEE80211_RATE_BASIC) == 0)
				continue;
			cck |= (1 << i);
			if (lowest_present_cck == -1 || lowest_present_cck > i)
				lowest_present_cck = i;
		}
	}
	for (i = IWM_FIRST_OFDM_RATE; i <= IWM_LAST_NON_HT_RATE; i++) {
		if ((iwm_ridx2rate(rs, i) & IEEE80211_RATE_BASIC) == 0)
			continue;
		ofdm |= (1 << (i - IWM_FIRST_OFDM_RATE));
		if (lowest_present_ofdm == -1 || lowest_present_ofdm > i)
			lowest_present_ofdm = i;
	}

	/*
	 * Now we've got the basic rates as bitmaps in the ofdm and cck
	 * variables. This isn't sufficient though, as there might not
	 * be all the right rates in the bitmap. E.g. if the only basic
	 * rates are 5.5 Mbps and 11 Mbps, we still need to add 1 Mbps
	 * and 6 Mbps because the 802.11-2007 standard says in 9.6:
	 *
	 *    [...] a STA responding to a received frame shall transmit
	 *    its Control Response frame [...] at the highest rate in the
	 *    BSSBasicRateSet parameter that is less than or equal to the
	 *    rate of the immediately previous frame in the frame exchange
	 *    sequence ([...]) and that is of the same modulation class
	 *    ([...]) as the received frame. If no rate contained in the
	 *    BSSBasicRateSet parameter meets these conditions, then the
	 *    control frame sent in response to a received frame shall be
	 *    transmitted at the highest mandatory rate of the PHY that is
	 *    less than or equal to the rate of the received frame, and
	 *    that is of the same modulation class as the received frame.
	 *
	 * As a consequence, we need to add all mandatory rates that are
	 * lower than all of the basic rates to these bitmaps.
	 */

	if (IWM_RATE_24M_INDEX < lowest_present_ofdm)
		ofdm |= IWM_RATE_BIT_MSK(24) >> IWM_FIRST_OFDM_RATE;
	if (IWM_RATE_12M_INDEX < lowest_present_ofdm)
		ofdm |= IWM_RATE_BIT_MSK(12) >> IWM_FIRST_OFDM_RATE;
	/* 6M already there or needed so always add */
	ofdm |= IWM_RATE_BIT_MSK(6) >> IWM_FIRST_OFDM_RATE;

	/*
	 * CCK is a bit more complex with DSSS vs. HR/DSSS vs. ERP.
	 * Note, however:
	 *  - if no CCK rates are basic, it must be ERP since there must
	 *    be some basic rates at all, so they're OFDM => ERP PHY
	 *    (or we're in 5 GHz, and the cck bitmap will never be used)
	 *  - if 11M is a basic rate, it must be ERP as well, so add 5.5M
	 *  - if 5.5M is basic, 1M and 2M are mandatory
	 *  - if 2M is basic, 1M is mandatory
	 *  - if 1M is basic, that's the only valid ACK rate.
	 * As a consequence, it's not as complicated as it sounds, just add
	 * any lower rates to the ACK rate bitmap.
	 */
	if (IWM_RATE_11M_INDEX < lowest_present_cck)
		cck |= IWM_RATE_BIT_MSK(11) >> IWM_FIRST_CCK_RATE;
	if (IWM_RATE_5M_INDEX < lowest_present_cck)
		cck |= IWM_RATE_BIT_MSK(5) >> IWM_FIRST_CCK_RATE;
	if (IWM_RATE_2M_INDEX < lowest_present_cck)
		cck |= IWM_RATE_BIT_MSK(2) >> IWM_FIRST_CCK_RATE;
	/* 1M already there or needed so always add */
	cck |= IWM_RATE_BIT_MSK(1) >> IWM_FIRST_CCK_RATE;

	*cck_rates = cck;
	*ofdm_rates = ofdm;
}

static void
iwm_mac_ctxt_cmd_common(struct iwm_softc *sc, struct iwm_node *in,
    struct iwm_mac_ctx_cmd *cmd, uint32_t action, int assoc)
{
#define IWM_EXP2(x)	((1 << (x)) - 1)	/* CWmin = 2^ECWmin - 1 */
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	int cck_ack_rates, ofdm_ack_rates;
	int i;

	cmd->id_and_color = htole32(IWM_FW_CMD_ID_AND_COLOR(in->in_id,
	    in->in_color));
	cmd->action = htole32(action);

	cmd->mac_type = htole32(IWM_FW_MAC_TYPE_BSS_STA);
	cmd->tsf_id = htole32(IWM_TSF_ID_A);

	IEEE80211_ADDR_COPY(cmd->node_addr, ic->ic_myaddr);
	IEEE80211_ADDR_COPY(cmd->bssid_addr, ni->ni_bssid);

	iwm_ack_rates(sc, in, &cck_ack_rates, &ofdm_ack_rates);
	cmd->cck_rates = htole32(cck_ack_rates);
	cmd->ofdm_rates = htole32(ofdm_ack_rates);

	cmd->cck_short_preamble
	    = htole32((ic->ic_flags & IEEE80211_F_SHPREAMBLE)
	      ? IWM_MAC_FLG_SHORT_PREAMBLE : 0);
	cmd->short_slot
	    = htole32((ic->ic_flags & IEEE80211_F_SHSLOT)
	      ? IWM_MAC_FLG_SHORT_SLOT : 0);

	for (i = 0; i < WME_NUM_AC; i++) {
		struct wmeParams *wmep = &ic->ic_wme.wme_params[i];
		int txf = iwm_ac_to_tx_fifo[i];

		cmd->ac[txf].cw_min = htole16(IWM_EXP2(wmep->wmep_logcwmin));
		cmd->ac[txf].cw_max = htole16(IWM_EXP2(wmep->wmep_logcwmax));
		cmd->ac[txf].aifsn = wmep->wmep_aifsn;
		cmd->ac[txf].fifos_mask = (1 << txf);
		cmd->ac[txf].edca_txop = htole16(wmep->wmep_txopLimit * 32);
	}
	if (ni->ni_flags & IEEE80211_NODE_QOS)
		cmd->qos_flags |= htole32(IWM_MAC_QOS_FLG_UPDATE_EDCA);

#ifndef IEEE80211_NO_HT
	if (ni->ni_flags & IEEE80211_NODE_HT) {
		enum ieee80211_htprot htprot =
		    (ni->ni_htop1 & IEEE80211_HTOP1_PROT_MASK);
		switch (htprot) {
		case IEEE80211_HTPROT_NONE:
			break;
		case IEEE80211_HTPROT_NONMEMBER:
		case IEEE80211_HTPROT_NONHT_MIXED:
			cmd->protection_flags |=
			    htole32(IWM_MAC_PROT_FLG_HT_PROT);
		case IEEE80211_HTPROT_20MHZ:
			cmd->protection_flags |=
			    htole32(IWM_MAC_PROT_FLG_HT_PROT |
			    IWM_MAC_PROT_FLG_FAT_PROT);
			break;
		default:
			break;
		}

		cmd->qos_flags |= htole32(IWM_MAC_QOS_FLG_TGN);
	}
#endif

	if (ic->ic_flags & IEEE80211_F_USEPROT)
		cmd->protection_flags |= htole32(IWM_MAC_PROT_FLG_TGG_PROTECT);

	cmd->filter_flags = htole32(IWM_MAC_FILTER_ACCEPT_GRP);
#undef IWM_EXP2
}

static void
iwm_mac_ctxt_cmd_fill_sta(struct iwm_softc *sc, struct iwm_node *in,
    struct iwm_mac_data_sta *sta, int assoc)
{
	struct ieee80211_node *ni = &in->in_ni;
	uint32_t dtim_off;
	uint64_t tsf;

	dtim_off = ni->ni_dtim_count * ni->ni_intval * IEEE80211_DUR_TU;
	tsf = le64toh(ni->ni_tstamp.tsf);

	sta->is_assoc = htole32(assoc);
	sta->dtim_time = htole32(ni->ni_rstamp + dtim_off);
	sta->dtim_tsf = htole64(tsf + dtim_off);
	sta->bi = htole32(ni->ni_intval);
	sta->bi_reciprocal = htole32(iwm_reciprocal(ni->ni_intval));
	sta->dtim_interval = htole32(ni->ni_intval * ni->ni_dtim_period);
	sta->dtim_reciprocal = htole32(iwm_reciprocal(sta->dtim_interval));
	sta->listen_interval = htole32(10);
	sta->assoc_id = htole32(ni->ni_associd);
	sta->assoc_beacon_arrive_time = htole32(ni->ni_rstamp);
}

static int
iwm_mac_ctxt_cmd(struct iwm_softc *sc, struct iwm_node *in, uint32_t action,
    int assoc)
{
	struct ieee80211_node *ni = &in->in_ni;
	struct iwm_mac_ctx_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));

	iwm_mac_ctxt_cmd_common(sc, in, &cmd, action, assoc);

	/* Allow beacons to pass through as long as we are not associated or we
	 * do not have dtim period information */
	if (!assoc || !ni->ni_associd || !ni->ni_dtim_period)
		cmd.filter_flags |= htole32(IWM_MAC_FILTER_IN_BEACON);
	else
		iwm_mac_ctxt_cmd_fill_sta(sc, in, &cmd.sta, assoc);

	return iwm_send_cmd_pdu(sc, IWM_MAC_CONTEXT_CMD, 0, sizeof(cmd), &cmd);
}

#define IWM_MISSED_BEACONS_THRESHOLD 8

static void
iwm_rx_missed_beacons_notif(struct iwm_softc *sc,
	struct iwm_rx_packet *pkt, struct iwm_rx_data *data)
{
	struct iwm_missed_beacons_notif *mb = (void *)pkt->data;
	int s;

	DPRINTF(("missed bcn mac_id=%u, consecutive=%u (%u, %u, %u)\n",
	    le32toh(mb->mac_id),
	    le32toh(mb->consec_missed_beacons),
	    le32toh(mb->consec_missed_beacons_since_last_rx),
	    le32toh(mb->num_recvd_beacons),
	    le32toh(mb->num_expected_beacons)));

	/*
	 * TODO: the threshold should be adjusted based on latency conditions,
	 * and/or in case of a CS flow on one of the other AP vifs.
	 */
	if (le32toh(mb->consec_missed_beacons_since_last_rx) >
	    IWM_MISSED_BEACONS_THRESHOLD) {
		s = splnet();
		ieee80211_beacon_miss(&sc->sc_ic);
		splx(s);
	}
}

static int
iwm_update_quotas(struct iwm_softc *sc, struct iwm_node *in)
{
	struct iwm_time_quota_cmd cmd;
	int i, idx, num_active_macs, quota, quota_rem;
	int colors[IWM_MAX_BINDINGS] = { -1, -1, -1, -1, };
	int n_ifs[IWM_MAX_BINDINGS] = {0, };
	uint16_t id;

	memset(&cmd, 0, sizeof(cmd));

	/* currently, PHY ID == binding ID */
	if (in) {
		id = in->in_phyctxt->id;
		KASSERT(id < IWM_MAX_BINDINGS);
		colors[id] = in->in_phyctxt->color;

		if (1)
			n_ifs[id] = 1;
	}

	/*
	 * The FW's scheduling session consists of
	 * IWM_MAX_QUOTA fragments. Divide these fragments
	 * equally between all the bindings that require quota
	 */
	num_active_macs = 0;
	for (i = 0; i < IWM_MAX_BINDINGS; i++) {
		cmd.quotas[i].id_and_color = htole32(IWM_FW_CTXT_INVALID);
		num_active_macs += n_ifs[i];
	}

	quota = 0;
	quota_rem = 0;
	if (num_active_macs) {
		quota = IWM_MAX_QUOTA / num_active_macs;
		quota_rem = IWM_MAX_QUOTA % num_active_macs;
	}

	for (idx = 0, i = 0; i < IWM_MAX_BINDINGS; i++) {
		if (colors[i] < 0)
			continue;

		cmd.quotas[idx].id_and_color =
			htole32(IWM_FW_CMD_ID_AND_COLOR(i, colors[i]));

		if (n_ifs[i] <= 0) {
			cmd.quotas[idx].quota = htole32(0);
			cmd.quotas[idx].max_duration = htole32(0);
		} else {
			cmd.quotas[idx].quota = htole32(quota * n_ifs[i]);
			cmd.quotas[idx].max_duration = htole32(0);
		}
		idx++;
	}

	/* Give the remainder of the session to the first binding */
	cmd.quotas[0].quota = htole32(le32toh(cmd.quotas[0].quota) + quota_rem);

	return iwm_send_cmd_pdu(sc, IWM_TIME_QUOTA_CMD, 0, sizeof(cmd), &cmd);
}

static int
iwm_auth(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;
	uint32_t duration;
	int err;

	err = iwm_sf_config(sc, IWM_SF_FULL_ON);
	if (err)
		return err;

	err = iwm_allow_mcast(sc);
	if (err)
		return err;

	sc->sc_phyctxt[0].channel = in->in_ni.ni_chan;
	err = iwm_phy_ctxt_cmd(sc, &sc->sc_phyctxt[0], 1, 1,
	    IWM_FW_CTXT_ACTION_MODIFY, 0);
	if (err)
		return err;
	in->in_phyctxt = &sc->sc_phyctxt[0];

	err = iwm_mac_ctxt_cmd(sc, in, IWM_FW_CTXT_ACTION_ADD, 0);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not add MAC context (error %d)\n", err);
		return err;
	}

	err = iwm_binding_cmd(sc, in, IWM_FW_CTXT_ACTION_ADD);
	if (err)
		return err;

	err = iwm_add_sta_cmd(sc, in, 0);
	if (err)
		return err;

	err = iwm_mac_ctxt_cmd(sc, in, IWM_FW_CTXT_ACTION_MODIFY, 0);
	if (err) {
		aprint_error_dev(sc->sc_dev, "failed to update MAC\n");
		return err;
	}

	/*
	 * Prevent the FW from wandering off channel during association
	 * by "protecting" the session with a time event.
	 */
	if (in->in_ni.ni_intval)
		duration = in->in_ni.ni_intval * 2;
	else
		duration = IEEE80211_DUR_TU;
	iwm_protect_session(sc, in, duration, in->in_ni.ni_intval / 2);
	DELAY(100);

	return 0;
}

static int
iwm_assoc(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;
	int err;

	err = iwm_add_sta_cmd(sc, in, 1);
	if (err)
		return err;

	return 0;
}

static struct ieee80211_node *
iwm_node_alloc(struct ieee80211_node_table *nt)
{
	return malloc(sizeof(struct iwm_node), M_80211_NODE, M_NOWAIT | M_ZERO);
}

static void
iwm_calib_timeout(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;
#ifndef IEEE80211_NO_HT
	struct ieee80211_node *ni = &in->in_ni;
	int otxrate;
#endif
	int s;

	s = splnet();
	if ((ic->ic_fixed_rate == -1
#ifndef IEEE80211_NO_HT
	    || ic->ic_fixed_mcs == -1
#endif
	    ) &&
	    ic->ic_opmode == IEEE80211_M_STA && ic->ic_bss) {
#ifndef IEEE80211_NO_HT
		if (ni->ni_flags & IEEE80211_NODE_HT)
			otxrate = ni->ni_txmcs;
		else
			otxrate = ni->ni_txrate;
#endif
		ieee80211_amrr_choose(&sc->sc_amrr, &in->in_ni, &in->in_amn);

#ifndef IEEE80211_NO_HT
		/*
		 * If AMRR has chosen a new TX rate we must update
		 * the firwmare's LQ rate table from process context.
		 */
		if ((ni->ni_flags & IEEE80211_NODE_HT) &&
		    otxrate != ni->ni_txmcs)
			softint_schedule(sc->setrates_task);
		else if (otxrate != ni->ni_txrate)
			softint_schedule(sc->setrates_task);
#endif
	}
	splx(s);

	callout_schedule(&sc->sc_calib_to, mstohz(500));
}

#ifndef IEEE80211_NO_HT
static void
iwm_setrates_task(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;

	/* Update rates table based on new TX rate determined by AMRR. */
	iwm_setrates(in);
}

static int
iwm_setrates(struct iwm_node *in)
{
	struct ieee80211_node *ni = &in->in_ni;
	struct ieee80211com *ic = ni->ni_ic;
	struct iwm_softc *sc = IC2IFP(ic)->if_softc;
	struct iwm_lq_cmd *lq = &in->in_lq;
	struct ieee80211_rateset *rs = &ni->ni_rates;
	int i, j, ridx, ridx_min, tab = 0;
#ifndef IEEE80211_NO_HT
	int sgi_ok;
#endif
	struct iwm_host_cmd cmd = {
		.id = IWM_LQ_CMD,
		.len = { sizeof(in->in_lq), },
	};

	memset(lq, 0, sizeof(*lq));
	lq->sta_id = IWM_STATION_ID;

	if (ic->ic_flags & IEEE80211_F_USEPROT)
		lq->flags |= IWM_LQ_FLAG_USE_RTS_MSK;

#ifndef IEEE80211_NO_HT
	sgi_ok = ((ni->ni_flags & IEEE80211_NODE_HT) &&
	    (ni->ni_htcaps & IEEE80211_HTCAP_SGI20));
#endif


	/*
	 * Fill the LQ rate selection table with legacy and/or HT rates
	 * in descending order, i.e. with the node's current TX rate first.
	 * In cases where throughput of an HT rate corresponds to a legacy
	 * rate it makes no sense to add both. We rely on the fact that
	 * iwm_rates is laid out such that equivalent HT/legacy rates share
	 * the same IWM_RATE_*_INDEX value. Also, rates not applicable to
	 * legacy/HT are assumed to be marked with an 'invalid' PLCP value.
	 */
	j = 0;
	ridx_min = (IEEE80211_IS_CHAN_5GHZ(ni->ni_chan)) ?
	    IWM_RIDX_OFDM : IWM_RIDX_CCK;
	for (ridx = IWM_RIDX_MAX; ridx >= ridx_min; ridx--) {
		if (j >= __arraycount(lq->rs_table))
			break;
		tab = 0;
#ifndef IEEE80211_NO_HT
		if ((ni->ni_flags & IEEE80211_NODE_HT) &&
		    iwm_rates[ridx].ht_plcp != IWM_RATE_HT_SISO_MCS_INV_PLCP) {
			for (i = ni->ni_txmcs; i >= 0; i--) {
				if (isclr(ni->ni_rxmcs, i))
					continue;
				if (ridx == iwm_mcs2ridx[i]) {
					tab = iwm_rates[ridx].ht_plcp;
					tab |= IWM_RATE_MCS_HT_MSK;
					if (sgi_ok)
						tab |= IWM_RATE_MCS_SGI_MSK;
					break;
				}
			}
		}
#endif
		if (tab == 0 && iwm_rates[ridx].plcp != IWM_RATE_INVM_PLCP) {
			for (i = ni->ni_txrate; i >= 0; i--) {
				if (iwm_rates[ridx].rate == (rs->rs_rates[i] &
				    IEEE80211_RATE_VAL)) {
					tab = iwm_rates[ridx].plcp;
					break;
				}
			}
		}

		if (tab == 0)
			continue;

		tab |= 1 << IWM_RATE_MCS_ANT_POS;
		if (IWM_RIDX_IS_CCK(ridx))
			tab |= IWM_RATE_MCS_CCK_MSK;
		DPRINTFN(2, ("station rate %d %x\n", i, tab));
		lq->rs_table[j++] = htole32(tab);
	}

	/* Fill the rest with the lowest possible rate */
	i = j > 0 ? j - 1 : 0;
	while (j < __arraycount(lq->rs_table))
		lq->rs_table[j++] = lq->rs_table[i];

	lq->single_stream_ant_msk = IWM_ANT_A;
	lq->dual_stream_ant_msk = IWM_ANT_AB;

	lq->agg_time_limit = htole16(4000);	/* 4ms */
	lq->agg_disable_start_th = 3;
#ifdef notyet
	lq->agg_frame_cnt_limit = 0x3f;
#else
	lq->agg_frame_cnt_limit = 1; /* tx agg disabled */
#endif

	cmd.data[0] = &in->in_lq;
	return iwm_send_cmd(sc, &cmd);
}
#endif

static int
iwm_media_change(struct ifnet *ifp)
{
	struct iwm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t rate, ridx;
	int err;

	err = ieee80211_media_change(ifp);
	if (err != ENETRESET)
		return err;

#ifndef IEEE80211_NO_HT
	if (ic->ic_fixed_mcs != -1)
		sc->sc_fixed_ridx = iwm_mcs2ridx[ic->ic_fixed_mcs];
	else
#endif
	if (ic->ic_fixed_rate != -1) {
		rate = ic->ic_sup_rates[ic->ic_curmode].
		    rs_rates[ic->ic_fixed_rate] & IEEE80211_RATE_VAL;
		/* Map 802.11 rate to HW rate index. */
		for (ridx = 0; ridx <= IWM_RIDX_MAX; ridx++)
			if (iwm_rates[ridx].rate == rate)
				break;
		sc->sc_fixed_ridx = ridx;
	}

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
	    (IFF_UP | IFF_RUNNING)) {
		iwm_stop(ifp, 0);
		err = iwm_init(ifp);
	}
	return err;
}

static int
iwm_do_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet *ifp = IC2IFP(ic);
	struct iwm_softc *sc = ifp->if_softc;
	enum ieee80211_state ostate = ic->ic_state;
	struct iwm_node *in;
	int err;

	DPRINTF(("switching state %s->%s\n", ieee80211_state_name[ostate],
	    ieee80211_state_name[nstate]));

	if (ostate == IEEE80211_S_SCAN && nstate != ostate)
		iwm_led_blink_stop(sc);

	if (ostate == IEEE80211_S_RUN && nstate != ostate)
		iwm_disable_beacon_filter(sc);

	/* Reset the device if moving out of AUTH, ASSOC, or RUN. */
	/* XXX Is there a way to switch states without a full reset? */
	if (ostate > IEEE80211_S_SCAN && nstate < ostate) {
		/*
		 * Upon receiving a deauth frame from AP the net80211 stack
		 * puts the driver into AUTH state. This will fail with this
		 * driver so bring the FSM from RUN to SCAN in this case.
		 */
		if (nstate != IEEE80211_S_INIT) {
			DPRINTF(("Force transition to INIT; MGT=%d\n", arg));
			/* Always pass arg as -1 since we can't Tx right now. */
			sc->sc_newstate(ic, IEEE80211_S_INIT, -1);
			iwm_stop(ifp, 0);
			iwm_init(ifp);
			return 0;
		}

		iwm_stop_device(sc);
		iwm_init_hw(sc);
	}

	switch (nstate) {
	case IEEE80211_S_INIT:
		break;

	case IEEE80211_S_SCAN:
		if (ostate == nstate &&
		    ISSET(sc->sc_flags, IWM_FLAG_SCANNING))
			return 0;
		if (isset(sc->sc_enabled_capa, IWM_UCODE_TLV_CAPA_UMAC_SCAN))
			err = iwm_umac_scan(sc);
		else
			err = iwm_lmac_scan(sc);
		if (err) {
			DPRINTF(("%s: could not initiate scan: %d\n",
			    DEVNAME(sc), err));
			return err;
		}
		SET(sc->sc_flags, IWM_FLAG_SCANNING);
		ic->ic_state = nstate;
		iwm_led_blink_start(sc);
		return 0;

	case IEEE80211_S_AUTH:
		err = iwm_auth(sc);
		if (err) {
			DPRINTF(("%s: could not move to auth state: %d\n",
			    DEVNAME(sc), err));
			return err;
		}
		break;

	case IEEE80211_S_ASSOC:
		err = iwm_assoc(sc);
		if (err) {
			DPRINTF(("%s: failed to associate: %d\n", DEVNAME(sc),
			    err));
			return err;
		}
		break;

	case IEEE80211_S_RUN:
		in = (struct iwm_node *)ic->ic_bss;

		/* We have now been assigned an associd by the AP. */
		err = iwm_mac_ctxt_cmd(sc, in, IWM_FW_CTXT_ACTION_MODIFY, 1);
		if (err) {
			aprint_error_dev(sc->sc_dev, "failed to update MAC\n");
			return err;
		}

		err = iwm_power_update_device(sc);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could send power command (error %d)\n", err);
			return err;
		}
#ifdef notyet
		/*
		 * Disabled for now. Default beacon filter settings
		 * prevent net80211 from getting ERP and HT protection
		 * updates from beacons.
		 */
		err = iwm_enable_beacon_filter(sc, in);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not enable beacon filter\n");
			return err;
		}
#endif
		err = iwm_power_mac_update_mode(sc, in);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not update MAC power (error %d)\n", err);
			return err;
		}

		err = iwm_update_quotas(sc, in);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not update quotas (error %d)\n", err);
			return err;
		}

		ieee80211_amrr_node_init(&sc->sc_amrr, &in->in_amn);

		/* Start at lowest available bit-rate, AMRR will raise. */
		in->in_ni.ni_txrate = 0;
#ifndef IEEE80211_NO_HT
		in->in_ni.ni_txmcs = 0;
		iwm_setrates(in);
#endif

		callout_schedule(&sc->sc_calib_to, mstohz(500));
		iwm_led_enable(sc);
		break;

	default:
		break;
	}

	return sc->sc_newstate(ic, nstate, arg);
}

static void
iwm_newstate_cb(struct work *wk, void *v)
{
	struct iwm_softc *sc = v;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_newstate_state *iwmns = (struct iwm_newstate_state *)wk;
	enum ieee80211_state nstate = iwmns->ns_nstate;
	int generation = iwmns->ns_generation;
	int arg = iwmns->ns_arg;
	int s;

	kmem_intr_free(iwmns, sizeof(*iwmns));

	s = splnet();

	DPRINTF(("Prepare to switch state %d->%d\n", ic->ic_state, nstate));
	if (sc->sc_generation != generation) {
		DPRINTF(("newstate_cb: someone pulled the plug meanwhile\n"));
		if (nstate == IEEE80211_S_INIT) {
			DPRINTF(("newstate_cb: nstate == IEEE80211_S_INIT: "
			    "calling sc_newstate()\n"));
			(void) sc->sc_newstate(ic, nstate, arg);
		}
	} else
		(void) iwm_do_newstate(ic, nstate, arg);

	splx(s);
}

static int
iwm_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct iwm_newstate_state *iwmns;
	struct ifnet *ifp = IC2IFP(ic);
	struct iwm_softc *sc = ifp->if_softc;

	callout_stop(&sc->sc_calib_to);

	iwmns = kmem_intr_alloc(sizeof(*iwmns), KM_NOSLEEP);
	if (!iwmns) {
		DPRINTF(("%s: allocating state cb mem failed\n", DEVNAME(sc)));
		return ENOMEM;
	}

	iwmns->ns_nstate = nstate;
	iwmns->ns_arg = arg;
	iwmns->ns_generation = sc->sc_generation;

	workqueue_enqueue(sc->sc_nswq, &iwmns->ns_wk, NULL);

	return 0;
}

static void
iwm_endscan(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	DPRINTF(("%s: scan ended\n", DEVNAME(sc)));

	s = splnet();
	if (ic->ic_state == IEEE80211_S_SCAN)
		ieee80211_end_scan(ic);
	splx(s);
}

/*
 * Aging and idle timeouts for the different possible scenarios
 * in default configuration
 */
static const uint32_t
iwm_sf_full_timeout_def[IWM_SF_NUM_SCENARIO][IWM_SF_NUM_TIMEOUT_TYPES] = {
	{
		htole32(IWM_SF_SINGLE_UNICAST_AGING_TIMER_DEF),
		htole32(IWM_SF_SINGLE_UNICAST_IDLE_TIMER_DEF)
	},
	{
		htole32(IWM_SF_AGG_UNICAST_AGING_TIMER_DEF),
		htole32(IWM_SF_AGG_UNICAST_IDLE_TIMER_DEF)
	},
	{
		htole32(IWM_SF_MCAST_AGING_TIMER_DEF),
		htole32(IWM_SF_MCAST_IDLE_TIMER_DEF)
	},
	{
		htole32(IWM_SF_BA_AGING_TIMER_DEF),
		htole32(IWM_SF_BA_IDLE_TIMER_DEF)
	},
	{
		htole32(IWM_SF_TX_RE_AGING_TIMER_DEF),
		htole32(IWM_SF_TX_RE_IDLE_TIMER_DEF)
	},
};

/*
 * Aging and idle timeouts for the different possible scenarios
 * in single BSS MAC configuration.
 */
static const uint32_t
iwm_sf_full_timeout[IWM_SF_NUM_SCENARIO][IWM_SF_NUM_TIMEOUT_TYPES] = {
	{
		htole32(IWM_SF_SINGLE_UNICAST_AGING_TIMER),
		htole32(IWM_SF_SINGLE_UNICAST_IDLE_TIMER)
	},
	{
		htole32(IWM_SF_AGG_UNICAST_AGING_TIMER),
		htole32(IWM_SF_AGG_UNICAST_IDLE_TIMER)
	},
	{
		htole32(IWM_SF_MCAST_AGING_TIMER),
		htole32(IWM_SF_MCAST_IDLE_TIMER)
	},
	{
		htole32(IWM_SF_BA_AGING_TIMER),
		htole32(IWM_SF_BA_IDLE_TIMER)
	},
	{
		htole32(IWM_SF_TX_RE_AGING_TIMER),
		htole32(IWM_SF_TX_RE_IDLE_TIMER)
	},
};

static void
iwm_fill_sf_command(struct iwm_softc *sc, struct iwm_sf_cfg_cmd *sf_cmd,
    struct ieee80211_node *ni)
{
	int i, j, watermark;

	sf_cmd->watermark[IWM_SF_LONG_DELAY_ON] = htole32(IWM_SF_W_MARK_SCAN);

	/*
	 * If we are in association flow - check antenna configuration
	 * capabilities of the AP station, and choose the watermark accordingly.
	 */
	if (ni) {
#ifndef IEEE80211_NO_HT
		if (ni->ni_flags & IEEE80211_NODE_HT) {
#ifdef notyet
			if (ni->ni_rxmcs[2] != 0)
				watermark = IWM_SF_W_MARK_MIMO3;
			else if (ni->ni_rxmcs[1] != 0)
				watermark = IWM_SF_W_MARK_MIMO2;
			else
#endif
				watermark = IWM_SF_W_MARK_SISO;
		} else
#endif
			watermark = IWM_SF_W_MARK_LEGACY;
	/* default watermark value for unassociated mode. */
	} else {
		watermark = IWM_SF_W_MARK_MIMO2;
	}
	sf_cmd->watermark[IWM_SF_FULL_ON] = htole32(watermark);

	for (i = 0; i < IWM_SF_NUM_SCENARIO; i++) {
		for (j = 0; j < IWM_SF_NUM_TIMEOUT_TYPES; j++) {
			sf_cmd->long_delay_timeouts[i][j] =
					htole32(IWM_SF_LONG_DELAY_AGING_TIMER);
		}
	}

	if (ni) {
		memcpy(sf_cmd->full_on_timeouts, iwm_sf_full_timeout,
		       sizeof(iwm_sf_full_timeout));
	} else {
		memcpy(sf_cmd->full_on_timeouts, iwm_sf_full_timeout_def,
		       sizeof(iwm_sf_full_timeout_def));
	}
}

static int
iwm_sf_config(struct iwm_softc *sc, int new_state)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_sf_cfg_cmd sf_cmd = {
		.state = htole32(IWM_SF_FULL_ON),
	};

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000)
		sf_cmd.state |= htole32(IWM_SF_CFG_DUMMY_NOTIF_OFF);

	switch (new_state) {
	case IWM_SF_UNINIT:
	case IWM_SF_INIT_OFF:
		iwm_fill_sf_command(sc, &sf_cmd, NULL);
		break;
	case IWM_SF_FULL_ON:
		iwm_fill_sf_command(sc, &sf_cmd, ic->ic_bss);
		break;
	default:
		return EINVAL;
	}

	return iwm_send_cmd_pdu(sc, IWM_REPLY_SF_CFG_CMD, IWM_CMD_ASYNC,
	    sizeof(sf_cmd), &sf_cmd);
}

static int
iwm_send_bt_init_conf(struct iwm_softc *sc)
{
	struct iwm_bt_coex_cmd bt_cmd;

	bt_cmd.mode = htole32(IWM_BT_COEX_WIFI);
	bt_cmd.enabled_modules = htole32(IWM_BT_COEX_HIGH_BAND_RET);

	return iwm_send_cmd_pdu(sc, IWM_BT_CONFIG, 0, sizeof(bt_cmd), &bt_cmd);
}

static bool
iwm_is_lar_supported(struct iwm_softc *sc)
{
	bool nvm_lar = sc->sc_nvm.lar_enabled;
	bool tlv_lar = isset(sc->sc_enabled_capa,
	    IWM_UCODE_TLV_CAPA_LAR_SUPPORT);

	if (iwm_lar_disable)
		return false;

	/*
	 * Enable LAR only if it is supported by the FW (TLV) &&
	 * enabled in the NVM
	 */
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000)
		return nvm_lar && tlv_lar;
	else
		return tlv_lar;
}

static int
iwm_send_update_mcc_cmd(struct iwm_softc *sc, const char *alpha2)
{
	struct iwm_mcc_update_cmd mcc_cmd;
	struct iwm_host_cmd hcmd = {
		.id = IWM_MCC_UPDATE_CMD,
		.flags = IWM_CMD_WANT_SKB,
		.data = { &mcc_cmd },
	};
	int err;
	int resp_v2 = isset(sc->sc_enabled_capa,
	    IWM_UCODE_TLV_CAPA_LAR_SUPPORT_V2);

	if (!iwm_is_lar_supported(sc)) {
		DPRINTF(("%s: no LAR support\n", __func__));
		return 0;
	}

	memset(&mcc_cmd, 0, sizeof(mcc_cmd));
	mcc_cmd.mcc = htole16(alpha2[0] << 8 | alpha2[1]);
	if (isset(sc->sc_ucode_api, IWM_UCODE_TLV_API_WIFI_MCC_UPDATE) ||
	    isset(sc->sc_enabled_capa, IWM_UCODE_TLV_CAPA_LAR_MULTI_MCC))
		mcc_cmd.source_id = IWM_MCC_SOURCE_GET_CURRENT;
	else
		mcc_cmd.source_id = IWM_MCC_SOURCE_OLD_FW;

	if (resp_v2)
		hcmd.len[0] = sizeof(struct iwm_mcc_update_cmd);
	else
		hcmd.len[0] = sizeof(struct iwm_mcc_update_cmd_v1);

	err = iwm_send_cmd(sc, &hcmd);
	if (err)
		return err;

	iwm_free_resp(sc, &hcmd);

	return 0;
}

static void
iwm_tt_tx_backoff(struct iwm_softc *sc, uint32_t backoff)
{
	struct iwm_host_cmd cmd = {
		.id = IWM_REPLY_THERMAL_MNG_BACKOFF,
		.len = { sizeof(uint32_t), },
		.data = { &backoff, },
	};

	iwm_send_cmd(sc, &cmd);
}

static int
iwm_init_hw(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int err, i, ac;

	err = iwm_preinit(sc);
	if (err)
		return err;

	err = iwm_start_hw(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "could not initialize hardware\n");
		return err;
	}

	err = iwm_run_init_mvm_ucode(sc, 0);
	if (err)
		return err;

	/* Should stop and start HW since INIT image just loaded. */
	iwm_stop_device(sc);
	err = iwm_start_hw(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "could not initialize hardware\n");
		return err;
	}

	/* Restart, this time with the regular firmware */
	err = iwm_load_ucode_wait_alive(sc, IWM_UCODE_TYPE_REGULAR);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not load firmware (error %d)\n", err);
		goto err;
	}

	err = iwm_send_bt_init_conf(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not init bt coex (error %d)\n", err);
		goto err;
	}

	err = iwm_send_tx_ant_cfg(sc, iwm_fw_valid_tx_ant(sc));
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not init tx ant config (error %d)\n", err);
		goto err;
	}

	/* Send phy db control command and then phy db calibration*/
	err = iwm_send_phy_db_data(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not init phy db (error %d)\n", err);
		goto err;
	}

	err = iwm_send_phy_cfg_cmd(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not send phy config (error %d)\n", err);
		goto err;
	}

	/* Add auxiliary station for scanning */
	err = iwm_add_aux_sta(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not add aux station (error %d)\n", err);
		goto err;
	}

	for (i = 0; i < IWM_NUM_PHY_CTX; i++) {
		/*
		 * The channel used here isn't relevant as it's
		 * going to be overwritten in the other flows.
		 * For now use the first channel we have.
		 */
		sc->sc_phyctxt[i].channel = &ic->ic_channels[1];
		err = iwm_phy_ctxt_cmd(sc, &sc->sc_phyctxt[i], 1, 1,
		    IWM_FW_CTXT_ACTION_ADD, 0);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not add phy context %d (error %d)\n",
			    i, err);
			goto err;
		}
	}

	/* Initialize tx backoffs to the minimum. */
	if (sc->sc_device_family == IWM_DEVICE_FAMILY_7000)
		iwm_tt_tx_backoff(sc, 0);

	err = iwm_power_update_device(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could send power command (error %d)\n", err);
		goto err;
	}

	err = iwm_send_update_mcc_cmd(sc, iwm_default_mcc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not init LAR (error %d)\n", err);
		goto err;
	}

	if (isset(sc->sc_enabled_capa, IWM_UCODE_TLV_CAPA_UMAC_SCAN)) {
		err = iwm_config_umac_scan(sc);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not configure scan (error %d)\n", err);
			goto err;
		}
	}

	for (ac = 0; ac < WME_NUM_AC; ac++) {
		err = iwm_enable_txq(sc, IWM_STATION_ID, ac,
		    iwm_ac_to_tx_fifo[ac]);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not enable Tx queue %d (error %d)\n",
			    i, err);
			goto err;
		}
	}

	err = iwm_disable_beacon_filter(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not disable beacon filter (error %d)\n", err);
		goto err;
	}

	return 0;

 err:
	iwm_stop_device(sc);
	return err;
}

/* Allow multicast from our BSSID. */
static int
iwm_allow_mcast(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct iwm_mcast_filter_cmd *cmd;
	size_t size;
	int err;

	size = roundup(sizeof(*cmd), 4);
	cmd = kmem_intr_zalloc(size, KM_NOSLEEP);
	if (cmd == NULL)
		return ENOMEM;
	cmd->filter_own = 1;
	cmd->port_id = 0;
	cmd->count = 0;
	cmd->pass_all = 1;
	IEEE80211_ADDR_COPY(cmd->bssid, ni->ni_bssid);

	err = iwm_send_cmd_pdu(sc, IWM_MCAST_FILTER_CMD, 0, size, cmd);
	kmem_intr_free(cmd, size);
	return err;
}

static int
iwm_init(struct ifnet *ifp)
{
	struct iwm_softc *sc = ifp->if_softc;
	int err;

	if (ISSET(sc->sc_flags, IWM_FLAG_HW_INITED))
		return 0;

	sc->sc_generation++;
	sc->sc_flags &= ~IWM_FLAG_STOPPED;

	err = iwm_init_hw(sc);
	if (err) {
		iwm_stop(ifp, 1);
		return err;
	}

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	ieee80211_begin_scan(&sc->sc_ic, 0);
	SET(sc->sc_flags, IWM_FLAG_HW_INITED);

	return 0;
}

static void
iwm_start(struct ifnet *ifp)
{
	struct iwm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ether_header *eh;
	struct mbuf *m;
	int ac;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		/* why isn't this done per-queue? */
		if (sc->qfullmsk != 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/* need to send management frames even if we're not RUNning */
		IF_DEQUEUE(&ic->ic_mgtq, m);
		if (m) {
			ni = M_GETCTX(m, struct ieee80211_node *);
			M_CLEARCTX(m);
			ac = WME_AC_BE;
			goto sendit;
		}
		if (ic->ic_state != IEEE80211_S_RUN) {
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (m->m_len < sizeof (*eh) &&
		   (m = m_pullup(m, sizeof (*eh))) == NULL) {
			ifp->if_oerrors++;
			continue;
		}

		eh = mtod(m, struct ether_header *);
		ni = ieee80211_find_txnode(ic, eh->ether_dhost);
		if (ni == NULL) {
			m_freem(m);
			ifp->if_oerrors++;
			continue;
		}

		/* classify mbuf so we can find which tx ring to use */
		if (ieee80211_classify(ic, m, ni) != 0) {
			m_freem(m);
			ieee80211_free_node(ni);
			ifp->if_oerrors++;
			continue;
		}

		/* No QoS encapsulation for EAPOL frames. */
		ac = (eh->ether_type != htons(ETHERTYPE_PAE)) ?
		    M_WME_GETAC(m) : WME_AC_BE;

		bpf_mtap(ifp, m);

		if ((m = ieee80211_encap(ic, m, ni)) == NULL) {
			ieee80211_free_node(ni);
			ifp->if_oerrors++;
			continue;
		}

 sendit:
		bpf_mtap3(ic->ic_rawbpf, m);

		if (iwm_tx(sc, m, ni, ac) != 0) {
			ieee80211_free_node(ni);
			ifp->if_oerrors++;
			continue;
		}

		if (ifp->if_flags & IFF_UP) {
			sc->sc_tx_timer = 15;
			ifp->if_timer = 1;
		}
	}
}

static void
iwm_stop(struct ifnet *ifp, int disable)
{
	struct iwm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct iwm_node *in = (struct iwm_node *)ic->ic_bss;

	sc->sc_flags &= ~IWM_FLAG_HW_INITED;
	sc->sc_flags |= IWM_FLAG_STOPPED;
	sc->sc_generation++;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	if (in)
		in->in_phyctxt = NULL;

	if (ic->ic_state != IEEE80211_S_INIT)
		ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	callout_stop(&sc->sc_calib_to);
	iwm_led_blink_stop(sc);
	ifp->if_timer = sc->sc_tx_timer = 0;
	iwm_stop_device(sc);
}

static void
iwm_watchdog(struct ifnet *ifp)
{
	struct iwm_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;
	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			aprint_error_dev(sc->sc_dev, "device timeout\n");
#ifdef IWM_DEBUG
			iwm_nic_error(sc);
#endif
			ifp->if_flags &= ~IFF_UP;
			iwm_stop(ifp, 1);
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(&sc->sc_ic);
}

static int
iwm_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct iwm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	const struct sockaddr *sa;
	int s, err = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		err = ifioctl_common(ifp, cmd, data);
		if (err)
			break;
		if (ifp->if_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING)) {
				err = iwm_init(ifp);
				if (err)
					ifp->if_flags &= ~IFF_UP;
			}
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				iwm_stop(ifp, 1);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (!ISSET(sc->sc_flags, IWM_FLAG_ATTACHED)) {
			err = ENXIO;
			break;
		}
		sa = ifreq_getaddr(SIOCADDMULTI, (struct ifreq *)data);
		err = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(sa, &sc->sc_ec) :
		    ether_delmulti(sa, &sc->sc_ec);
		if (err == ENETRESET)
			err = 0;
		break;

	default:
		if (!ISSET(sc->sc_flags, IWM_FLAG_ATTACHED)) {
			err = ether_ioctl(ifp, cmd, data);
			break;
		}
		err = ieee80211_ioctl(ic, cmd, data);
		break;
	}

	if (err == ENETRESET) {
		err = 0;
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING)) {
			iwm_stop(ifp, 0);
			err = iwm_init(ifp);
		}
	}

	splx(s);
	return err;
}

/*
 * Note: This structure is read from the device with IO accesses,
 * and the reading already does the endian conversion. As it is
 * read with uint32_t-sized accesses, any members with a different size
 * need to be ordered correctly though!
 */
struct iwm_error_event_table {
	uint32_t valid;		/* (nonzero) valid, (0) log is empty */
	uint32_t error_id;		/* type of error */
	uint32_t trm_hw_status0;	/* TRM HW status */
	uint32_t trm_hw_status1;	/* TRM HW status */
	uint32_t blink2;		/* branch link */
	uint32_t ilink1;		/* interrupt link */
	uint32_t ilink2;		/* interrupt link */
	uint32_t data1;		/* error-specific data */
	uint32_t data2;		/* error-specific data */
	uint32_t data3;		/* error-specific data */
	uint32_t bcon_time;		/* beacon timer */
	uint32_t tsf_low;		/* network timestamp function timer */
	uint32_t tsf_hi;		/* network timestamp function timer */
	uint32_t gp1;		/* GP1 timer register */
	uint32_t gp2;		/* GP2 timer register */
	uint32_t fw_rev_type;	/* firmware revision type */
	uint32_t major;		/* uCode version major */
	uint32_t minor;		/* uCode version minor */
	uint32_t hw_ver;		/* HW Silicon version */
	uint32_t brd_ver;		/* HW board version */
	uint32_t log_pc;		/* log program counter */
	uint32_t frame_ptr;		/* frame pointer */
	uint32_t stack_ptr;		/* stack pointer */
	uint32_t hcmd;		/* last host command header */
	uint32_t isr0;		/* isr status register LMPM_NIC_ISR0:
				 * rxtx_flag */
	uint32_t isr1;		/* isr status register LMPM_NIC_ISR1:
				 * host_flag */
	uint32_t isr2;		/* isr status register LMPM_NIC_ISR2:
				 * enc_flag */
	uint32_t isr3;		/* isr status register LMPM_NIC_ISR3:
				 * time_flag */
	uint32_t isr4;		/* isr status register LMPM_NIC_ISR4:
				 * wico interrupt */
	uint32_t last_cmd_id;	/* last HCMD id handled by the firmware */
	uint32_t wait_event;		/* wait event() caller address */
	uint32_t l2p_control;	/* L2pControlField */
	uint32_t l2p_duration;	/* L2pDurationField */
	uint32_t l2p_mhvalid;	/* L2pMhValidBits */
	uint32_t l2p_addr_match;	/* L2pAddrMatchStat */
	uint32_t lmpm_pmg_sel;	/* indicate which clocks are turned on
				 * (LMPM_PMG_SEL) */
	uint32_t u_timestamp;	/* indicate when the date and time of the
				 * compilation */
	uint32_t flow_handler;	/* FH read/write pointers, RX credit */
} __packed /* LOG_ERROR_TABLE_API_S_VER_3 */;

/*
 * UMAC error struct - relevant starting from family 8000 chip.
 * Note: This structure is read from the device with IO accesses,
 * and the reading already does the endian conversion. As it is
 * read with u32-sized accesses, any members with a different size
 * need to be ordered correctly though!
 */
struct iwm_umac_error_event_table {
	uint32_t valid;		/* (nonzero) valid, (0) log is empty */
	uint32_t error_id;	/* type of error */
	uint32_t blink1;	/* branch link */
	uint32_t blink2;	/* branch link */
	uint32_t ilink1;	/* interrupt link */
	uint32_t ilink2;	/* interrupt link */
	uint32_t data1;		/* error-specific data */
	uint32_t data2;		/* error-specific data */
	uint32_t data3;		/* error-specific data */
	uint32_t umac_major;
	uint32_t umac_minor;
	uint32_t frame_pointer;	/* core register 27 */
	uint32_t stack_pointer;	/* core register 28 */
	uint32_t cmd_header;	/* latest host cmd sent to UMAC */
	uint32_t nic_isr_pref;	/* ISR status register */
} __packed;

#define ERROR_START_OFFSET  (1 * sizeof(uint32_t))
#define ERROR_ELEM_SIZE     (7 * sizeof(uint32_t))

#ifdef IWM_DEBUG
static const struct {
	const char *name;
	uint8_t num;
} advanced_lookup[] = {
	{ "NMI_INTERRUPT_WDG", 0x34 },
	{ "SYSASSERT", 0x35 },
	{ "UCODE_VERSION_MISMATCH", 0x37 },
	{ "BAD_COMMAND", 0x38 },
	{ "NMI_INTERRUPT_DATA_ACTION_PT", 0x3C },
	{ "FATAL_ERROR", 0x3D },
	{ "NMI_TRM_HW_ERR", 0x46 },
	{ "NMI_INTERRUPT_TRM", 0x4C },
	{ "NMI_INTERRUPT_BREAK_POINT", 0x54 },
	{ "NMI_INTERRUPT_WDG_RXF_FULL", 0x5C },
	{ "NMI_INTERRUPT_WDG_NO_RBD_RXF_FULL", 0x64 },
	{ "NMI_INTERRUPT_HOST", 0x66 },
	{ "NMI_INTERRUPT_ACTION_PT", 0x7C },
	{ "NMI_INTERRUPT_UNKNOWN", 0x84 },
	{ "NMI_INTERRUPT_INST_ACTION_PT", 0x86 },
	{ "ADVANCED_SYSASSERT", 0 },
};

static const char *
iwm_desc_lookup(uint32_t num)
{
	int i;

	for (i = 0; i < __arraycount(advanced_lookup) - 1; i++)
		if (advanced_lookup[i].num == num)
			return advanced_lookup[i].name;

	/* No entry matches 'num', so it is the last: ADVANCED_SYSASSERT */
	return advanced_lookup[i].name;
}

/*
 * Support for dumping the error log seemed like a good idea ...
 * but it's mostly hex junk and the only sensible thing is the
 * hw/ucode revision (which we know anyway).  Since it's here,
 * I'll just leave it in, just in case e.g. the Intel guys want to
 * help us decipher some "ADVANCED_SYSASSERT" later.
 */
static void
iwm_nic_error(struct iwm_softc *sc)
{
	struct iwm_error_event_table t;
	uint32_t base;

	aprint_error_dev(sc->sc_dev, "dumping device error log\n");
	base = sc->sc_uc.uc_error_event_table;
	if (base < 0x800000) {
		aprint_error_dev(sc->sc_dev,
		    "Invalid error log pointer 0x%08x\n", base);
		return;
	}

	if (iwm_read_mem(sc, base, &t, sizeof(t)/sizeof(uint32_t))) {
		aprint_error_dev(sc->sc_dev, "reading errlog failed\n");
		return;
	}

	if (!t.valid) {
		aprint_error_dev(sc->sc_dev, "errlog not found, skipping\n");
		return;
	}

	if (ERROR_START_OFFSET <= t.valid * ERROR_ELEM_SIZE) {
		aprint_error_dev(sc->sc_dev, "Start Error Log Dump:\n");
		aprint_error_dev(sc->sc_dev, "Status: 0x%x, count: %d\n",
		    sc->sc_flags, t.valid);
	}

	aprint_error_dev(sc->sc_dev, "%08X | %-28s\n", t.error_id,
	    iwm_desc_lookup(t.error_id));
	aprint_error_dev(sc->sc_dev, "%08X | trm_hw_status0\n",
	    t.trm_hw_status0);
	aprint_error_dev(sc->sc_dev, "%08X | trm_hw_status1\n",
	    t.trm_hw_status1);
	aprint_error_dev(sc->sc_dev, "%08X | branchlink2\n", t.blink2);
	aprint_error_dev(sc->sc_dev, "%08X | interruptlink1\n", t.ilink1);
	aprint_error_dev(sc->sc_dev, "%08X | interruptlink2\n", t.ilink2);
	aprint_error_dev(sc->sc_dev, "%08X | data1\n", t.data1);
	aprint_error_dev(sc->sc_dev, "%08X | data2\n", t.data2);
	aprint_error_dev(sc->sc_dev, "%08X | data3\n", t.data3);
	aprint_error_dev(sc->sc_dev, "%08X | beacon time\n", t.bcon_time);
	aprint_error_dev(sc->sc_dev, "%08X | tsf low\n", t.tsf_low);
	aprint_error_dev(sc->sc_dev, "%08X | tsf hi\n", t.tsf_hi);
	aprint_error_dev(sc->sc_dev, "%08X | time gp1\n", t.gp1);
	aprint_error_dev(sc->sc_dev, "%08X | time gp2\n", t.gp2);
	aprint_error_dev(sc->sc_dev, "%08X | uCode revision type\n",
	    t.fw_rev_type);
	aprint_error_dev(sc->sc_dev, "%08X | uCode version major\n",
	    t.major);
	aprint_error_dev(sc->sc_dev, "%08X | uCode version minor\n",
	    t.minor);
	aprint_error_dev(sc->sc_dev, "%08X | hw version\n", t.hw_ver);
	aprint_error_dev(sc->sc_dev, "%08X | board version\n", t.brd_ver);
	aprint_error_dev(sc->sc_dev, "%08X | hcmd\n", t.hcmd);
	aprint_error_dev(sc->sc_dev, "%08X | isr0\n", t.isr0);
	aprint_error_dev(sc->sc_dev, "%08X | isr1\n", t.isr1);
	aprint_error_dev(sc->sc_dev, "%08X | isr2\n", t.isr2);
	aprint_error_dev(sc->sc_dev, "%08X | isr3\n", t.isr3);
	aprint_error_dev(sc->sc_dev, "%08X | isr4\n", t.isr4);
	aprint_error_dev(sc->sc_dev, "%08X | last cmd Id\n", t.last_cmd_id);
	aprint_error_dev(sc->sc_dev, "%08X | wait_event\n", t.wait_event);
	aprint_error_dev(sc->sc_dev, "%08X | l2p_control\n", t.l2p_control);
	aprint_error_dev(sc->sc_dev, "%08X | l2p_duration\n", t.l2p_duration);
	aprint_error_dev(sc->sc_dev, "%08X | l2p_mhvalid\n", t.l2p_mhvalid);
	aprint_error_dev(sc->sc_dev, "%08X | l2p_addr_match\n",
	    t.l2p_addr_match);
	aprint_error_dev(sc->sc_dev, "%08X | lmpm_pmg_sel\n", t.lmpm_pmg_sel);
	aprint_error_dev(sc->sc_dev, "%08X | timestamp\n", t.u_timestamp);
	aprint_error_dev(sc->sc_dev, "%08X | flow_handler\n", t.flow_handler);

	if (sc->sc_uc.uc_umac_error_event_table)
		iwm_nic_umac_error(sc);
}

static void
iwm_nic_umac_error(struct iwm_softc *sc)
{
	struct iwm_umac_error_event_table t;
	uint32_t base;

	base = sc->sc_uc.uc_umac_error_event_table;

	if (base < 0x800000) {
		aprint_error_dev(sc->sc_dev,
		    "Invalid error log pointer 0x%08x\n", base);
		return;
	}

	if (iwm_read_mem(sc, base, &t, sizeof(t)/sizeof(uint32_t))) {
		aprint_error_dev(sc->sc_dev, "reading errlog failed\n");
		return;
	}

	if (ERROR_START_OFFSET <= t.valid * ERROR_ELEM_SIZE) {
		aprint_error_dev(sc->sc_dev, "Start UMAC Error Log Dump:\n");
		aprint_error_dev(sc->sc_dev, "Status: 0x%x, count: %d\n",
		    sc->sc_flags, t.valid);
	}

	aprint_error_dev(sc->sc_dev, "0x%08X | %s\n", t.error_id,
		iwm_desc_lookup(t.error_id));
	aprint_error_dev(sc->sc_dev, "0x%08X | umac branchlink1\n", t.blink1);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac branchlink2\n", t.blink2);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac interruptlink1\n",
	    t.ilink1);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac interruptlink2\n",
	    t.ilink2);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac data1\n", t.data1);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac data2\n", t.data2);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac data3\n", t.data3);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac major\n", t.umac_major);
	aprint_error_dev(sc->sc_dev, "0x%08X | umac minor\n", t.umac_minor);
	aprint_error_dev(sc->sc_dev, "0x%08X | frame pointer\n",
	    t.frame_pointer);
	aprint_error_dev(sc->sc_dev, "0x%08X | stack pointer\n",
	    t.stack_pointer);
	aprint_error_dev(sc->sc_dev, "0x%08X | last host cmd\n", t.cmd_header);
	aprint_error_dev(sc->sc_dev, "0x%08X | isr status reg\n",
	    t.nic_isr_pref);
}
#endif

#define SYNC_RESP_STRUCT(_var_, _pkt_)					\
do {									\
	bus_dmamap_sync(sc->sc_dmat, data->map, sizeof(*(_pkt_)),	\
	    sizeof(*(_var_)), BUS_DMASYNC_POSTREAD);			\
	_var_ = (void *)((_pkt_)+1);					\
} while (/*CONSTCOND*/0)

#define SYNC_RESP_PTR(_ptr_, _len_, _pkt_)				\
do {									\
	bus_dmamap_sync(sc->sc_dmat, data->map, sizeof(*(_pkt_)),	\
	    sizeof(len), BUS_DMASYNC_POSTREAD);				\
	_ptr_ = (void *)((_pkt_)+1);					\
} while (/*CONSTCOND*/0)

#define ADVANCE_RXQ(sc) (sc->rxq.cur = (sc->rxq.cur + 1) % IWM_RX_RING_COUNT);

static void
iwm_notif_intr(struct iwm_softc *sc)
{
	uint16_t hw;

	bus_dmamap_sync(sc->sc_dmat, sc->rxq.stat_dma.map,
	    0, sc->rxq.stat_dma.size, BUS_DMASYNC_POSTREAD);

	hw = le16toh(sc->rxq.stat->closed_rb_num) & 0xfff;
	while (sc->rxq.cur != hw) {
		struct iwm_rx_data *data = &sc->rxq.data[sc->rxq.cur];
		struct iwm_rx_packet *pkt;
		struct iwm_cmd_response *cresp;
		int orig_qid, qid, idx, code;

		bus_dmamap_sync(sc->sc_dmat, data->map, 0, sizeof(*pkt),
		    BUS_DMASYNC_POSTREAD);
		pkt = mtod(data->m, struct iwm_rx_packet *);

		orig_qid = pkt->hdr.qid;
		qid = orig_qid & ~0x80;
		idx = pkt->hdr.idx;

		code = IWM_WIDE_ID(pkt->hdr.flags, pkt->hdr.code);

		/*
		 * randomly get these from the firmware, no idea why.
		 * they at least seem harmless, so just ignore them for now
		 */
		if (__predict_false((pkt->hdr.code == 0 && qid == 0 && idx == 0)
		    || pkt->len_n_flags == htole32(0x55550000))) {
			ADVANCE_RXQ(sc);
			continue;
		}

		switch (code) {
		case IWM_REPLY_RX_PHY_CMD:
			iwm_rx_rx_phy_cmd(sc, pkt, data);
			break;

		case IWM_REPLY_RX_MPDU_CMD:
			iwm_rx_rx_mpdu(sc, pkt, data);
			break;

		case IWM_TX_CMD:
			iwm_rx_tx_cmd(sc, pkt, data);
			break;

		case IWM_MISSED_BEACONS_NOTIFICATION:
			iwm_rx_missed_beacons_notif(sc, pkt, data);
			break;

		case IWM_MFUART_LOAD_NOTIFICATION:
			break;

		case IWM_ALIVE: {
			struct iwm_alive_resp_v1 *resp1;
			struct iwm_alive_resp_v2 *resp2;
			struct iwm_alive_resp_v3 *resp3;

			if (iwm_rx_packet_payload_len(pkt) == sizeof(*resp1)) {
				SYNC_RESP_STRUCT(resp1, pkt);
				sc->sc_uc.uc_error_event_table
				    = le32toh(resp1->error_event_table_ptr);
				sc->sc_uc.uc_log_event_table
				    = le32toh(resp1->log_event_table_ptr);
				sc->sched_base = le32toh(resp1->scd_base_ptr);
				if (resp1->status == IWM_ALIVE_STATUS_OK)
					sc->sc_uc.uc_ok = 1;
				else
					sc->sc_uc.uc_ok = 0;
			}
			if (iwm_rx_packet_payload_len(pkt) == sizeof(*resp2)) {
				SYNC_RESP_STRUCT(resp2, pkt);
				sc->sc_uc.uc_error_event_table
				    = le32toh(resp2->error_event_table_ptr);
				sc->sc_uc.uc_log_event_table
				    = le32toh(resp2->log_event_table_ptr);
				sc->sched_base = le32toh(resp2->scd_base_ptr);
				sc->sc_uc.uc_umac_error_event_table
				    = le32toh(resp2->error_info_addr);
				if (resp2->status == IWM_ALIVE_STATUS_OK)
					sc->sc_uc.uc_ok = 1;
				else
					sc->sc_uc.uc_ok = 0;
			}
			if (iwm_rx_packet_payload_len(pkt) == sizeof(*resp3)) {
				SYNC_RESP_STRUCT(resp3, pkt);
				sc->sc_uc.uc_error_event_table
				    = le32toh(resp3->error_event_table_ptr);
				sc->sc_uc.uc_log_event_table
				    = le32toh(resp3->log_event_table_ptr);
				sc->sched_base = le32toh(resp3->scd_base_ptr);
				sc->sc_uc.uc_umac_error_event_table
				    = le32toh(resp3->error_info_addr);
				if (resp3->status == IWM_ALIVE_STATUS_OK)
					sc->sc_uc.uc_ok = 1;
				else
					sc->sc_uc.uc_ok = 0;
			}

			sc->sc_uc.uc_intr = 1;
			wakeup(&sc->sc_uc);
			break;
		}

		case IWM_CALIB_RES_NOTIF_PHY_DB: {
			struct iwm_calib_res_notif_phy_db *phy_db_notif;
			SYNC_RESP_STRUCT(phy_db_notif, pkt);
			uint16_t size = le16toh(phy_db_notif->length);
			bus_dmamap_sync(sc->sc_dmat, data->map,
			    sizeof(*pkt) + sizeof(*phy_db_notif),
			    size, BUS_DMASYNC_POSTREAD);
			iwm_phy_db_set_section(sc, phy_db_notif, size);
			break;
		}

		case IWM_STATISTICS_NOTIFICATION: {
			struct iwm_notif_statistics *stats;
			SYNC_RESP_STRUCT(stats, pkt);
			memcpy(&sc->sc_stats, stats, sizeof(sc->sc_stats));
			sc->sc_noise = iwm_get_noise(&stats->rx.general);
			break;
		}

		case IWM_NVM_ACCESS_CMD:
		case IWM_MCC_UPDATE_CMD:
			if (sc->sc_wantresp == ((qid << 16) | idx)) {
				bus_dmamap_sync(sc->sc_dmat, data->map, 0,
				    sizeof(sc->sc_cmd_resp),
				    BUS_DMASYNC_POSTREAD);
				memcpy(sc->sc_cmd_resp,
				    pkt, sizeof(sc->sc_cmd_resp));
			}
			break;

		case IWM_MCC_CHUB_UPDATE_CMD: {
			struct iwm_mcc_chub_notif *notif;
			SYNC_RESP_STRUCT(notif, pkt);

			sc->sc_fw_mcc[0] = (notif->mcc & 0xff00) >> 8;
			sc->sc_fw_mcc[1] = notif->mcc & 0xff;
			sc->sc_fw_mcc[2] = '\0';
			break;
		}

		case IWM_DTS_MEASUREMENT_NOTIFICATION:
		case IWM_WIDE_ID(IWM_PHY_OPS_GROUP,
		    IWM_DTS_MEASUREMENT_NOTIF_WIDE): {
			struct iwm_dts_measurement_notif_v1 *notif1;
			struct iwm_dts_measurement_notif_v2 *notif2;

			if (iwm_rx_packet_payload_len(pkt) == sizeof(*notif1)) {
				SYNC_RESP_STRUCT(notif1, pkt);
				DPRINTF(("%s: DTS temp=%d \n",
				    DEVNAME(sc), notif1->temp));
				break;
			}
			if (iwm_rx_packet_payload_len(pkt) == sizeof(*notif2)) {
				SYNC_RESP_STRUCT(notif2, pkt);
				DPRINTF(("%s: DTS temp=%d \n",
				    DEVNAME(sc), notif2->temp));
				break;
			}
			break;
		}

		case IWM_PHY_CONFIGURATION_CMD:
		case IWM_TX_ANT_CONFIGURATION_CMD:
		case IWM_ADD_STA:
		case IWM_MAC_CONTEXT_CMD:
		case IWM_REPLY_SF_CFG_CMD:
		case IWM_POWER_TABLE_CMD:
		case IWM_PHY_CONTEXT_CMD:
		case IWM_BINDING_CONTEXT_CMD:
		case IWM_TIME_EVENT_CMD:
		case IWM_SCAN_REQUEST_CMD:
		case IWM_WIDE_ID(IWM_ALWAYS_LONG_GROUP, IWM_SCAN_CFG_CMD):
		case IWM_WIDE_ID(IWM_ALWAYS_LONG_GROUP, IWM_SCAN_REQ_UMAC):
		case IWM_WIDE_ID(IWM_ALWAYS_LONG_GROUP, IWM_SCAN_ABORT_UMAC):
		case IWM_SCAN_OFFLOAD_REQUEST_CMD:
		case IWM_SCAN_OFFLOAD_ABORT_CMD:
		case IWM_REPLY_BEACON_FILTERING_CMD:
		case IWM_MAC_PM_POWER_TABLE:
		case IWM_TIME_QUOTA_CMD:
		case IWM_REMOVE_STA:
		case IWM_TXPATH_FLUSH:
		case IWM_LQ_CMD:
		case IWM_WIDE_ID(IWM_ALWAYS_LONG_GROUP, IWM_FW_PAGING_BLOCK_CMD):
		case IWM_BT_CONFIG:
		case IWM_REPLY_THERMAL_MNG_BACKOFF:
			SYNC_RESP_STRUCT(cresp, pkt);
			if (sc->sc_wantresp == ((qid << 16) | idx)) {
				memcpy(sc->sc_cmd_resp,
				    pkt, sizeof(*pkt) + sizeof(*cresp));
			}
			break;

		/* ignore */
		case IWM_PHY_DB_CMD:
			break;

		case IWM_INIT_COMPLETE_NOTIF:
			sc->sc_init_complete = 1;
			wakeup(&sc->sc_init_complete);
			break;

		case IWM_SCAN_OFFLOAD_COMPLETE: {
			struct iwm_periodic_scan_complete *notif;
			SYNC_RESP_STRUCT(notif, pkt);
			break;
		}

		case IWM_SCAN_ITERATION_COMPLETE: {
			struct iwm_lmac_scan_complete_notif *notif;
			SYNC_RESP_STRUCT(notif, pkt);
			if (ISSET(sc->sc_flags, IWM_FLAG_SCANNING)) {
				CLR(sc->sc_flags, IWM_FLAG_SCANNING);
				iwm_endscan(sc);
			}
			break;
		}

		case IWM_SCAN_COMPLETE_UMAC: {
			struct iwm_umac_scan_complete *notif;
			SYNC_RESP_STRUCT(notif, pkt);
			if (ISSET(sc->sc_flags, IWM_FLAG_SCANNING)) {
				CLR(sc->sc_flags, IWM_FLAG_SCANNING);
				iwm_endscan(sc);
			}
			break;
		}

		case IWM_SCAN_ITERATION_COMPLETE_UMAC: {
			struct iwm_umac_scan_iter_complete_notif *notif;
			SYNC_RESP_STRUCT(notif, pkt);
			if (ISSET(sc->sc_flags, IWM_FLAG_SCANNING)) {
				CLR(sc->sc_flags, IWM_FLAG_SCANNING);
				iwm_endscan(sc);
			}
			break;
		}

		case IWM_REPLY_ERROR: {
			struct iwm_error_resp *resp;
			SYNC_RESP_STRUCT(resp, pkt);
			aprint_error_dev(sc->sc_dev,
			    "firmware error 0x%x, cmd 0x%x\n",
			    le32toh(resp->error_type), resp->cmd_id);
			break;
		}

		case IWM_TIME_EVENT_NOTIFICATION: {
			struct iwm_time_event_notif *notif;
			SYNC_RESP_STRUCT(notif, pkt);
			break;
		}

		case IWM_DEBUG_LOG_MSG:
			break;

		case IWM_MCAST_FILTER_CMD:
			break;

		case IWM_SCD_QUEUE_CFG: {
			struct iwm_scd_txq_cfg_rsp *rsp;
			SYNC_RESP_STRUCT(rsp, pkt);
			break;
		}

		default:
			aprint_error_dev(sc->sc_dev,
			    "unhandled firmware response 0x%x 0x%x/0x%x "
			    "rx ring %d[%d]\n",
			    code, pkt->hdr.code, pkt->len_n_flags, qid, idx);
			break;
		}

		/*
		 * uCode sets bit 0x80 when it originates the notification,
		 * i.e. when the notification is not a direct response to a
		 * command sent by the driver.
		 * For example, uCode issues IWM_REPLY_RX when it sends a
		 * received frame to the driver.
		 */
		if (!(orig_qid & (1 << 7))) {
			iwm_cmd_done(sc, qid, idx);
		}

		ADVANCE_RXQ(sc);
	}

	/*
	 * Seems like the hardware gets upset unless we align the write by 8??
	 */
	hw = (hw == 0) ? IWM_RX_RING_COUNT - 1 : hw - 1;
	IWM_WRITE(sc, IWM_FH_RSCSR_CHNL0_WPTR, hw & ~7);
}

static int
iwm_intr(void *arg)
{
	struct iwm_softc *sc = arg;

	/* Disable interrupts */
	IWM_WRITE(sc, IWM_CSR_INT_MASK, 0);

	softint_schedule(sc->sc_soft_ih);
	return 1;
}

static void
iwm_softintr(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ifnet *ifp = IC2IFP(&sc->sc_ic);
	uint32_t r1, r2;
	int isperiodic = 0, s;

	if (__predict_true(sc->sc_flags & IWM_FLAG_USE_ICT)) {
		uint32_t *ict = sc->ict_dma.vaddr;
		int tmp;

		bus_dmamap_sync(sc->sc_dmat, sc->ict_dma.map,
		    0, sc->ict_dma.size, BUS_DMASYNC_POSTREAD);
		tmp = htole32(ict[sc->ict_cur]);
		if (tmp == 0)
			goto out_ena;	/* Interrupt not for us. */

		/*
		 * ok, there was something.  keep plowing until we have all.
		 */
		r1 = r2 = 0;
		while (tmp) {
			r1 |= tmp;
			ict[sc->ict_cur] = 0;	/* Acknowledge. */
			sc->ict_cur = (sc->ict_cur + 1) % IWM_ICT_COUNT;
			tmp = htole32(ict[sc->ict_cur]);
		}

		bus_dmamap_sync(sc->sc_dmat, sc->ict_dma.map,
		    0, sc->ict_dma.size, BUS_DMASYNC_PREWRITE);

		/* this is where the fun begins.  don't ask */
		if (r1 == 0xffffffff)
			r1 = 0;

		/* i am not expected to understand this */
		if (r1 & 0xc0000)
			r1 |= 0x8000;
		r1 = (0xff & r1) | ((0xff00 & r1) << 16);
	} else {
		r1 = IWM_READ(sc, IWM_CSR_INT);
		if (r1 == 0xffffffff || (r1 & 0xfffffff0) == 0xa5a5a5a0)
			return;	/* Hardware gone! */
		r2 = IWM_READ(sc, IWM_CSR_FH_INT_STATUS);
	}
	if (r1 == 0 && r2 == 0) {
		goto out_ena;	/* Interrupt not for us. */
	}

	/* Acknowledge interrupts. */
	IWM_WRITE(sc, IWM_CSR_INT, r1 | ~sc->sc_intmask);
	if (__predict_false(!(sc->sc_flags & IWM_FLAG_USE_ICT)))
		IWM_WRITE(sc, IWM_CSR_FH_INT_STATUS, r2);

	if (r1 & IWM_CSR_INT_BIT_SW_ERR) {
#ifdef IWM_DEBUG
		int i;

		iwm_nic_error(sc);

		/* Dump driver status (TX and RX rings) while we're here. */
		DPRINTF(("driver status:\n"));
		for (i = 0; i < IWM_MAX_QUEUES; i++) {
			struct iwm_tx_ring *ring = &sc->txq[i];
			DPRINTF(("  tx ring %2d: qid=%-2d cur=%-3d "
			    "queued=%-3d\n",
			    i, ring->qid, ring->cur, ring->queued));
		}
		DPRINTF(("  rx ring: cur=%d\n", sc->rxq.cur));
		DPRINTF(("  802.11 state %s\n",
		    ieee80211_state_name[sc->sc_ic.ic_state]));
#endif

		aprint_error_dev(sc->sc_dev, "fatal firmware error\n");
 fatal:
		s = splnet();
		ifp->if_flags &= ~IFF_UP;
		iwm_stop(ifp, 1);
		splx(s);
		/* Don't restore interrupt mask */
		return;

	}

	if (r1 & IWM_CSR_INT_BIT_HW_ERR) {
		aprint_error_dev(sc->sc_dev,
		    "hardware error, stopping device\n");
		goto fatal;
	}

	/* firmware chunk loaded */
	if (r1 & IWM_CSR_INT_BIT_FH_TX) {
		IWM_WRITE(sc, IWM_CSR_FH_INT_STATUS, IWM_CSR_FH_INT_TX_MASK);
		sc->sc_fw_chunk_done = 1;
		wakeup(&sc->sc_fw);
	}

	if (r1 & IWM_CSR_INT_BIT_RF_KILL) {
		if (iwm_check_rfkill(sc) && (ifp->if_flags & IFF_UP))
			goto fatal;
	}

	if (r1 & IWM_CSR_INT_BIT_RX_PERIODIC) {
		IWM_WRITE(sc, IWM_CSR_INT, IWM_CSR_INT_BIT_RX_PERIODIC);
		if ((r1 & (IWM_CSR_INT_BIT_FH_RX | IWM_CSR_INT_BIT_SW_RX)) == 0)
			IWM_WRITE_1(sc,
			    IWM_CSR_INT_PERIODIC_REG, IWM_CSR_INT_PERIODIC_DIS);
		isperiodic = 1;
	}

	if ((r1 & (IWM_CSR_INT_BIT_FH_RX | IWM_CSR_INT_BIT_SW_RX)) ||
	    isperiodic) {
		IWM_WRITE(sc, IWM_CSR_FH_INT_STATUS, IWM_CSR_FH_INT_RX_MASK);

		iwm_notif_intr(sc);

		/* enable periodic interrupt, see above */
		if (r1 & (IWM_CSR_INT_BIT_FH_RX | IWM_CSR_INT_BIT_SW_RX) &&
		    !isperiodic)
			IWM_WRITE_1(sc, IWM_CSR_INT_PERIODIC_REG,
			    IWM_CSR_INT_PERIODIC_ENA);
	}

out_ena:
	iwm_restore_interrupts(sc);
}

/*
 * Autoconf glue-sniffing
 */

static const pci_product_id_t iwm_devices[] = {
	PCI_PRODUCT_INTEL_WIFI_LINK_7260_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_7260_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_3160_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_3160_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_7265_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_7265_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_3165_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_3165_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_8260_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_8260_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_4165_1,
	PCI_PRODUCT_INTEL_WIFI_LINK_4165_2,
	PCI_PRODUCT_INTEL_WIFI_LINK_8265,
};

static int
iwm_match(device_t parent, cfdata_t match __unused, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	for (size_t i = 0; i < __arraycount(iwm_devices); i++)
		if (PCI_PRODUCT(pa->pa_id) == iwm_devices[i])
			return 1;

	return 0;
}

static int
iwm_preinit(struct iwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int err;

	if (ISSET(sc->sc_flags, IWM_FLAG_ATTACHED))
		return 0;

	err = iwm_start_hw(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "could not initialize hardware\n");
		return err;
	}

	err = iwm_run_init_mvm_ucode(sc, 1);
	iwm_stop_device(sc);
	if (err)
		return err;

	sc->sc_flags |= IWM_FLAG_ATTACHED;

	aprint_normal_dev(sc->sc_dev, "hw rev 0x%x, fw ver %s, address %s\n",
	    sc->sc_hw_rev & IWM_CSR_HW_REV_TYPE_MSK, sc->sc_fwver,
	    ether_sprintf(sc->sc_nvm.hw_addr));

#ifndef IEEE80211_NO_HT
	if (sc->sc_nvm.sku_cap_11n_enable)
		iwm_setup_ht_rates(sc);
#endif

	/* not all hardware can do 5GHz band */
	if (sc->sc_nvm.sku_cap_band_52GHz_enable)
		ic->ic_sup_rates[IEEE80211_MODE_11A] = ieee80211_std_rateset_11a;

	ieee80211_ifattach(ic);

	ic->ic_node_alloc = iwm_node_alloc;

	/* Override 802.11 state transition machine. */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = iwm_newstate;
	ieee80211_media_init(ic, iwm_media_change, ieee80211_media_status);
	ieee80211_announce(ic);

	iwm_radiotap_attach(sc);

	return 0;
}

static void
iwm_attach_hook(device_t dev)
{
	struct iwm_softc *sc = device_private(dev);

	iwm_preinit(sc);
}

static void
iwm_attach(device_t parent, device_t self, void *aux)
{
	struct iwm_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	pcireg_t reg, memtype;
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr;
	int err;
	int txq_i;
	const struct sysctlnode *node;

	sc->sc_dev = self;
	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_pciid = pa->pa_id;

	pci_aprint_devinfo(pa, NULL);

	if (workqueue_create(&sc->sc_nswq, "iwmns",
	    iwm_newstate_cb, sc, PRI_NONE, IPL_NET, 0))
		panic("%s: could not create workqueue: newstate",
		    device_xname(self));
	sc->sc_soft_ih = softint_establish(SOFTINT_NET, iwm_softintr, sc);
	if (sc->sc_soft_ih == NULL)
		panic("%s: could not establish softint", device_xname(self));

	/*
	 * Get the offset of the PCI Express Capability Structure in PCI
	 * Configuration Space.
	 */
	err = pci_get_capability(sc->sc_pct, sc->sc_pcitag,
	    PCI_CAP_PCIEXPRESS, &sc->sc_cap_off, NULL);
	if (err == 0) {
		aprint_error_dev(self,
		    "PCIe capability structure not found!\n");
		return;
	}

	/* Clear device-specific "PCI retry timeout" register (41h). */
	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0x40);
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0x40, reg & ~0xff00);

	/* Enable bus-mastering */
	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, reg);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_MAPREG_START);
	err = pci_mapreg_map(pa, PCI_MAPREG_START, memtype, 0,
	    &sc->sc_st, &sc->sc_sh, NULL, &sc->sc_sz);
	if (err) {
		aprint_error_dev(self, "can't map mem space\n");
		return;
	}

	/* Install interrupt handler. */
	err = pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0);
	if (err) {
		aprint_error_dev(self, "can't allocate interrupt\n");
		return;
	}
	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	if (pci_intr_type(sc->sc_pct, sc->sc_pihp[0]) == PCI_INTR_TYPE_INTX)
		CLR(reg, PCI_COMMAND_INTERRUPT_DISABLE);
	else
		SET(reg, PCI_COMMAND_INTERRUPT_DISABLE);
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, reg);
	intrstr = pci_intr_string(sc->sc_pct, sc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(sc->sc_pct, sc->sc_pihp[0],
	    IPL_NET, iwm_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_wantresp = IWM_CMD_RESP_IDLE;

	sc->sc_hw_rev = IWM_READ(sc, IWM_CSR_HW_REV);
	switch (PCI_PRODUCT(sc->sc_pciid)) {
	case PCI_PRODUCT_INTEL_WIFI_LINK_3160_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_3160_2:
		sc->sc_fwname = "iwlwifi-3160-17.ucode";
		sc->host_interrupt_operation_mode = 1;
		sc->apmg_wake_up_wa = 1;
		sc->sc_device_family = IWM_DEVICE_FAMILY_7000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_3165_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_3165_2:
		sc->sc_fwname = "iwlwifi-7265D-22.ucode";
		sc->host_interrupt_operation_mode = 0;
		sc->apmg_wake_up_wa = 1;
		sc->sc_device_family = IWM_DEVICE_FAMILY_7000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_3168:
		sc->sc_fwname = "iwlwifi-3168-22.ucode";
		sc->host_interrupt_operation_mode = 0;
		sc->apmg_wake_up_wa = 1;
		sc->sc_device_family = IWM_DEVICE_FAMILY_7000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_7260_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_7260_2:
		sc->sc_fwname = "iwlwifi-7260-17.ucode";
		sc->host_interrupt_operation_mode = 1;
		sc->apmg_wake_up_wa = 1;
		sc->sc_device_family = IWM_DEVICE_FAMILY_7000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_7265_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_7265_2:
		sc->sc_fwname = (sc->sc_hw_rev & IWM_CSR_HW_REV_TYPE_MSK) ==
		    IWM_CSR_HW_REV_TYPE_7265D ?
		    "iwlwifi-7265D-22.ucode": "iwlwifi-7265-17.ucode";
		sc->host_interrupt_operation_mode = 0;
		sc->apmg_wake_up_wa = 1;
		sc->sc_device_family = IWM_DEVICE_FAMILY_7000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_8260_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_8260_2:
	case PCI_PRODUCT_INTEL_WIFI_LINK_4165_1:
	case PCI_PRODUCT_INTEL_WIFI_LINK_4165_2:
		sc->sc_fwname = "iwlwifi-8000C-22.ucode";
		sc->host_interrupt_operation_mode = 0;
		sc->apmg_wake_up_wa = 0;
		sc->sc_device_family = IWM_DEVICE_FAMILY_8000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ_8000;
		break;
	case PCI_PRODUCT_INTEL_WIFI_LINK_8265:
		sc->sc_fwname = "iwlwifi-8265-22.ucode";
		sc->host_interrupt_operation_mode = 0;
		sc->apmg_wake_up_wa = 0;
		sc->sc_device_family = IWM_DEVICE_FAMILY_8000;
		sc->sc_fwdmasegsz = IWM_FWDMASEGSZ_8000;
		break;
	default:
		aprint_error_dev(self, "unknown product %#x",
		    PCI_PRODUCT(sc->sc_pciid));
		return;
	}
	DPRINTF(("%s: firmware=%s\n", DEVNAME(sc), sc->sc_fwname));

	/*
	 * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
	 * changed, and now the revision step also includes bit 0-1 (no more
	 * "dash" value). To keep hw_rev backwards compatible - we'll store it
	 * in the old format.
	 */

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000)
		sc->sc_hw_rev = (sc->sc_hw_rev & 0xfff0) |
		    (IWM_CSR_HW_REV_STEP(sc->sc_hw_rev << 2) << 2);

	if (iwm_prepare_card_hw(sc) != 0) {
		aprint_error_dev(sc->sc_dev, "could not initialize hardware\n");
		return;
	}

	if (sc->sc_device_family == IWM_DEVICE_FAMILY_8000) {
		uint32_t hw_step;

		/*
		 * In order to recognize C step the driver should read the
		 * chip version id located at the AUX bus MISC address.
		 */
		IWM_SETBITS(sc, IWM_CSR_GP_CNTRL,
			    IWM_CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
		DELAY(2);

		err = iwm_poll_bit(sc, IWM_CSR_GP_CNTRL,
				   IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
				   IWM_CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
				   25000);
		if (!err) {
			aprint_error_dev(sc->sc_dev,
			    "failed to wake up the nic\n");
			return;
		}

		if (iwm_nic_lock(sc)) {
			hw_step = iwm_read_prph(sc, IWM_WFPM_CTRL_REG);
			hw_step |= IWM_ENABLE_WFPM;
			iwm_write_prph(sc, IWM_WFPM_CTRL_REG, hw_step);
			hw_step = iwm_read_prph(sc, IWM_AUX_MISC_REG);
			hw_step = (hw_step >> IWM_HW_STEP_LOCATION_BITS) & 0xF;
			if (hw_step == 0x3)
				sc->sc_hw_rev = (sc->sc_hw_rev & 0xFFFFFFF3) |
				    (IWM_SILICON_C_STEP << 2);
			iwm_nic_unlock(sc);
		} else {
			aprint_error_dev(sc->sc_dev,
			    "failed to lock the nic\n");
			return;
		}
	}

	/*
	 * Allocate DMA memory for firmware transfers.
	 * Must be aligned on a 16-byte boundary.
	 */
	err = iwm_dma_contig_alloc(sc->sc_dmat, &sc->fw_dma, sc->sc_fwdmasegsz,
	    16);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate memory for firmware\n");
		return;
	}

	/* Allocate "Keep Warm" page, used internally by the card. */
	err = iwm_dma_contig_alloc(sc->sc_dmat, &sc->kw_dma, 4096, 4096);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate keep warm page\n");
		goto fail1;
	}

	/* Allocate interrupt cause table (ICT).*/
	err = iwm_dma_contig_alloc(sc->sc_dmat, &sc->ict_dma, IWM_ICT_SIZE,
	    1 << IWM_ICT_PADDR_SHIFT);
	if (err) {
		aprint_error_dev(sc->sc_dev, "could not allocate ICT table\n");
		goto fail2;
	}

	/* TX scheduler rings must be aligned on a 1KB boundary. */
	err = iwm_dma_contig_alloc(sc->sc_dmat, &sc->sched_dma,
	    __arraycount(sc->txq) * sizeof(struct iwm_agn_scd_bc_tbl), 1024);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate TX scheduler rings\n");
		goto fail3;
	}

	for (txq_i = 0; txq_i < __arraycount(sc->txq); txq_i++) {
		err = iwm_alloc_tx_ring(sc, &sc->txq[txq_i], txq_i);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "could not allocate TX ring %d\n", txq_i);
			goto fail4;
		}
	}

	err = iwm_alloc_rx_ring(sc, &sc->rxq);
	if (err) {
		aprint_error_dev(sc->sc_dev, "could not allocate RX ring\n");
		goto fail5;
	}

	/* Clear pending interrupts. */
	IWM_WRITE(sc, IWM_CSR_INT, 0xffffffff);

	if ((err = sysctl_createv(&sc->sc_clog, 0, NULL, &node,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("iwm per-controller controls"),
	    NULL, 0, NULL, 0,
	    CTL_HW, iwm_sysctl_root_num, CTL_CREATE,
	    CTL_EOL)) != 0) {
		aprint_normal_dev(sc->sc_dev,
		    "couldn't create iwm per-controller sysctl node\n");
	}
	if (err == 0) {
		int iwm_nodenum = node->sysctl_num;

		/* Reload firmware sysctl node */
		if ((err = sysctl_createv(&sc->sc_clog, 0, NULL, &node,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "fw_loaded",
		    SYSCTL_DESCR("Reload firmware"),
		    iwm_sysctl_fw_loaded_handler, 0, (void *)sc, 0,
		    CTL_HW, iwm_sysctl_root_num, iwm_nodenum, CTL_CREATE,
		    CTL_EOL)) != 0) {
			aprint_normal_dev(sc->sc_dev,
			    "couldn't create load_fw sysctl node\n");
		}
	}

	/*
	 * Attach interface
	 */
	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;	/* default to BSS mode */
	ic->ic_state = IEEE80211_S_INIT;

	/* Set device capabilities. */
	ic->ic_caps =
	    IEEE80211_C_WEP |		/* WEP */
	    IEEE80211_C_WPA |		/* 802.11i */
#ifdef notyet
	    IEEE80211_C_SCANALL |	/* device scans all channels at once */
	    IEEE80211_C_SCANALLBAND |	/* device scans all bands at once */
#endif
	    IEEE80211_C_SHSLOT |	/* short slot time supported */
	    IEEE80211_C_SHPREAMBLE;	/* short preamble supported */

#ifndef IEEE80211_NO_HT
	ic->ic_htcaps = IEEE80211_HTCAP_SGI20;
	ic->ic_htxcaps = 0;
	ic->ic_txbfcaps = 0;
	ic->ic_aselcaps = 0;
	ic->ic_ampdu_params = (IEEE80211_AMPDU_PARAM_SS_4 | 0x3 /* 64k */);
#endif

	/* all hardware can do 2.4GHz band */
	ic->ic_sup_rates[IEEE80211_MODE_11B] = ieee80211_std_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = ieee80211_std_rateset_11g;

	for (int i = 0; i < __arraycount(sc->sc_phyctxt); i++) {
		sc->sc_phyctxt[i].id = i;
	}

	sc->sc_amrr.amrr_min_success_threshold =  1;
	sc->sc_amrr.amrr_max_success_threshold = 15;

	/* IBSS channel undefined for now. */
	ic->ic_ibss_chan = &ic->ic_channels[1];

#if 0
	ic->ic_max_rssi = IWM_MAX_DBM - IWM_MIN_DBM;
#endif

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = iwm_init;
	ifp->if_stop = iwm_stop;
	ifp->if_ioctl = iwm_ioctl;
	ifp->if_start = iwm_start;
	ifp->if_watchdog = iwm_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, DEVNAME(sc), IFNAMSIZ);

	err = if_initialize(ifp);
	if (err != 0) {
		aprint_error_dev(sc->sc_dev, "if_initialize failed(%d)\n",
		    err);
		goto fail6;
	}
#if 0
	ieee80211_ifattach(ic);
#else
	ether_ifattach(ifp, ic->ic_myaddr);	/* XXX */
#endif
	/* Use common softint-based if_input */
	ifp->if_percpuq = if_percpuq_create(ifp);
	if_register(ifp);

	callout_init(&sc->sc_calib_to, 0);
	callout_setfunc(&sc->sc_calib_to, iwm_calib_timeout, sc);
	callout_init(&sc->sc_led_blink_to, 0);
	callout_setfunc(&sc->sc_led_blink_to, iwm_led_blink_timeout, sc);
#ifndef IEEE80211_NO_HT
	if (workqueue_create(&sc->sc_setratewq, "iwmsr",
	    iwm_setrates_task, sc, PRI_NONE, IPL_NET, 0))
		panic("%s: could not create workqueue: setrates",
		    device_xname(self));
	if (workqueue_create(&sc->sc_bawq, "iwmba",
	    iwm_ba_task, sc, PRI_NONE, IPL_NET, 0))
		panic("%s: could not create workqueue: blockack",
		    device_xname(self));
	if (workqueue_create(&sc->sc_htprowq, "iwmhtpro",
	    iwm_htprot_task, sc, PRI_NONE, IPL_NET, 0))
		panic("%s: could not create workqueue: htprot",
		    device_xname(self));
#endif

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * We can't do normal attach before the file system is mounted
	 * because we cannot read the MAC address without loading the
	 * firmware from disk.  So we postpone until mountroot is done.
	 * Notably, this will require a full driver unload/load cycle
	 * (or reboot) in case the firmware is not present when the
	 * hook runs.
	 */
	config_mountroot(self, iwm_attach_hook);

	return;

fail6:	iwm_free_rx_ring(sc, &sc->rxq);
fail5:	while (--txq_i >= 0)
		iwm_free_tx_ring(sc, &sc->txq[txq_i]);
fail4:	iwm_dma_contig_free(&sc->sched_dma);
fail3:	if (sc->ict_dma.vaddr != NULL)
		iwm_dma_contig_free(&sc->ict_dma);
fail2:	iwm_dma_contig_free(&sc->kw_dma);
fail1:	iwm_dma_contig_free(&sc->fw_dma);
}

void
iwm_radiotap_attach(struct iwm_softc *sc)
{
	struct ifnet *ifp = IC2IFP(&sc->sc_ic);

	bpf_attach2(ifp, DLT_IEEE802_11_RADIO,
	    sizeof (struct ieee80211_frame) + IEEE80211_RADIOTAP_HDRLEN,
	    &sc->sc_drvbpf);

	sc->sc_rxtap_len = sizeof sc->sc_rxtapu;
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(IWM_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof sc->sc_txtapu;
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(IWM_TX_RADIOTAP_PRESENT);
}

#if 0
static void
iwm_init_task(void *arg)
{
	struct iwm_softc *sc = arg;
	struct ifnet *ifp = IC2IFP(&sc->sc_ic);
	int s;

	rw_enter_write(&sc->ioctl_rwl);
	s = splnet();

	iwm_stop(ifp, 0);
	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == IFF_UP)
		iwm_init(ifp);

	splx(s);
	rw_exit(&sc->ioctl_rwl);
}

static void
iwm_wakeup(struct iwm_softc *sc)
{
	pcireg_t reg;

	/* Clear device-specific "PCI retry timeout" register (41h). */
	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag, 0x40);
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, 0x40, reg & ~0xff00);

	iwm_init_task(sc);
}

static int
iwm_activate(device_t self, enum devact act)
{
	struct iwm_softc *sc = device_private(self);
	struct ifnet *ifp = IC2IFP(&sc->sc_ic);

	switch (act) {
	case DVACT_DEACTIVATE:
		if (ifp->if_flags & IFF_RUNNING)
			iwm_stop(ifp, 0);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}
#endif

CFATTACH_DECL_NEW(iwm, sizeof(struct iwm_softc), iwm_match, iwm_attach,
	NULL, NULL);

static int
iwm_sysctl_fw_loaded_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct iwm_softc *sc;
	int err, t;

	node = *rnode;
	sc = node.sysctl_data;
	t = ISSET(sc->sc_flags, IWM_FLAG_FW_LOADED) ? 1 : 0;
	node.sysctl_data = &t;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	if (t == 0)
		CLR(sc->sc_flags, IWM_FLAG_FW_LOADED);
	return 0;
}

SYSCTL_SETUP(sysctl_iwm, "sysctl iwm(4) subtree setup")
{
	const struct sysctlnode *rnode;
#ifdef IWM_DEBUG
	const struct sysctlnode *cnode;
#endif /* IWM_DEBUG */
	int rc;

	if ((rc = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "iwm",
	    SYSCTL_DESCR("iwm global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	iwm_sysctl_root_num = rnode->sysctl_num;

#ifdef IWM_DEBUG
	/* control debugging printfs */
	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &iwm_debug, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif /* IWM_DEBUG */

	return;

 err:
	aprint_error("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
