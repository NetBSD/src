/* $NetBSD$ */

/*-
 * Copyright (c) 2026 berte
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/stdint.h>
#include <sys/endian.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/bpf.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_proto.h>
#include <net80211/ieee80211_node.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/led.h>

#include "if_uwe5622_sdiovar.h"

#include <dev/microcode/uwe5622/uwe5622_fw.h>

CFATTACH_DECL_NEW(uwe5622_sdio, sizeof(struct uwe5622_sdio_softc),
    uwe5622_sdio_match, uwe5622_sdio_attach, uwe5622_sdio_detach, NULL);


int
uwe5622_sdio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sdmmc_attach_args *sa;
	struct sdmmc_function *sf;

	sa = aux;
	if (sa == NULL || sa->sf == NULL)
		return 0;

	sf = sa->sf;

	if (sf->number != 1)
		return 0;
	if (sa->interface != 0)
		return 0;
	if (sa->manufacturer == 0 && sa->product == 0)
		return 1;

	return 0;
}


void
uwe5622_sdio_attach(device_t parent, device_t self, void *aux)
{
	struct uwe5622_sdio_softc *sc;
	struct sdmmc_attach_args *sa;
	struct sdmmc_function *sf;
	uint32_t raw4, m3l_r4, m3e_r4, m3l_m, m3e_m, m3l_dt, m3e_dt;
	int error;
	uint8_t b[4];

	sc = device_private(self);
	sa = aux;

	sc->sc_dev = self;
	sc->sc_sf = sa->sf;

	mutex_init(&sc->sc_bus_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_rx_cv_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_rx_cv, "uweintr");
	sc->sc_sdio_ih = NULL;
	sc->sc_rx_lwp = NULL;
	sc->sc_rx_stop = false;

	callout_init(&sc->sc_led_co, 0);
	callout_setfunc(&sc->sc_led_co, uwe5622_sdio_led_timeout, sc);
	sc->sc_led_on = false;

	sc->sc_enabled = false;
	sc->sc_sync_cfg_written = false;
	sc->sc_transport_ready = false;
	sc->sc_intr_enabled = false;
	sc->sc_chipid = 0;
	sc->sc_chip_model = UWE_CHIP_INVALID;
	sc->sc_at_reply_ready = false;
	sc->sc_at_reply_subtype = 0xff;
	sc->sc_at_reply_len = 0;
	sc->sc_at_reply[0] = '\0';
	sc->sc_log_seen = false;
	sc->sc_log_preview_len = 0;
	memset(sc->sc_log_preview, 0, sizeof(sc->sc_log_preview));
	memset(sc->sc_pkt_stats, 0, sizeof(sc->sc_pkt_stats));
	sc->sc_rx_total_packets = 0;
 sc->sc_rx_unhandled_packets = 0;
	sc->sc_wifi_reply_ready = false;
	sc->sc_wifi_pending_cmd = 0;
	sc->sc_wifi_scan_done = false;
	sc->sc_wifi_scan_aborted = false;
	sc->sc_wifi_reply_cmd_id = 0;
	sc->sc_wifi_reply_ctx_id = UWE_WIFI_CTX_INVALID;
	sc->sc_wifi_reply_status = 0;
	sc->sc_wifi_reply_len = 0;
	sc->sc_wifi_ctx_id = UWE_WIFI_CTX_INVALID;
	sc->sc_fw_version = 0;
	sc->sc_fw_std = 0;
	sc->sc_fw_capa = 0;
	sc->sc_credit_capa = 0;
	sc->sc_ott_supt = 0xff;
	sc->sc_fw_tx_asserted = false;
	sc->sc_tx_credit[0] = 0;
	sc->sc_tx_credit[1] = 0;
	sc->sc_tx_credit[2] = 0;
	sc->sc_tx_credit[3] = 0;
	memset(sc->sc_fw_mac, 0, sizeof(sc->sc_fw_mac));
	sc->sc_wifi_opened = false;
	sc->sc_wifi_session_ready = false;
	sc->sc_scan_publish_active = false;
	sc->sc_net80211_attached = false;
	sc->sc_sta_lut_valid = false;
	sc->sc_sta_lut_index = 0;
	memset(&sc->sc_ic, 0, sizeof(sc->sc_ic));

	sf = sc->sc_sf;
	raw4 = m3l_r4 = m3e_r4 = m3l_m = m3e_m = m3l_dt = m3e_dt = 0;

	aprint_naive("\n");
	aprint_normal_dev(self,
	    "attach: fn=%d interface=%d vendor=0x%04x product=0x%04x\n",
	    sf->number, sa->interface, sa->manufacturer, sa->product);

	(void)sdmmc_io_function_disable(sf);
	error = sdmmc_io_function_enable(sf);
	if (error) {
		aprint_error_dev(self, "function enable failed: %d\n", error);
		return;
	}
	sc->sc_enabled = true;

	error = sdmmc_io_set_blocklen(sf, 512);
	if (error) {
		aprint_error_dev(self, "set blocklen failed: %d\n", error);
	} else {
		aprint_normal_dev(self, "blocklen set to 512\n");
	}

	raw4 = uwe5622_sdio_read_dt_port_read4(sc);
	m3l_r4 = uwe5622_sdio_readl_read4(sc, UWE_CHIPID_REG_M3_M3L);
	m3e_r4 = uwe5622_sdio_readl_read4(sc, UWE_CHIPID_REG_M3E);
	m3l_m = uwe5622_sdio_readl_multi(sc, UWE_CHIPID_REG_M3_M3L);
	m3e_m = uwe5622_sdio_readl_multi(sc, UWE_CHIPID_REG_M3E);

	if (uwe5622_sdio_dt_read(sc, UWE_CHIPID_REG_M3_M3L, b, sizeof(b)) == 0) {
		m3l_dt = ((uint32_t)b[0] << 0) |
		    ((uint32_t)b[1] << 8) |
		    ((uint32_t)b[2] << 16) |
		    ((uint32_t)b[3] << 24);
	}

	if (uwe5622_sdio_dt_read(sc, UWE_CHIPID_REG_M3E, b, sizeof(b)) == 0) {
		m3e_dt = ((uint32_t)b[0] << 0) |
		    ((uint32_t)b[1] << 8) |
		    ((uint32_t)b[2] << 16) |
		    ((uint32_t)b[3] << 24);
	}

	if (uwe5622_sdio_classify_chipid(m3l_dt) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3l_dt;
	else if (uwe5622_sdio_classify_chipid(m3e_dt) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3e_dt;
	else if (uwe5622_sdio_classify_chipid(m3l_m) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3l_m;
	else if (uwe5622_sdio_classify_chipid(m3e_m) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3e_m;
	else if (uwe5622_sdio_classify_chipid(m3l_r4) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3l_r4;
	else if (uwe5622_sdio_classify_chipid(m3e_r4) != UWE_CHIP_INVALID)
		sc->sc_chipid = m3e_r4;
	else
		sc->sc_chipid = raw4;

	sc->sc_chip_model = uwe5622_sdio_classify_chipid(sc->sc_chipid);

	aprint_normal_dev(self, "selected chip id = 0x%08x\n", sc->sc_chipid);

	if (sc->sc_chip_model == UWE_CHIP_INVALID) {
		aprint_error_dev(self,
		    "chip id not stable/recognized yet: 0x%08x\n",
		    sc->sc_chipid);
		return;
	}

	aprint_normal_dev(self, "identified chip model: %s\n",
	    uwe5622_sdio_chip_name(sc->sc_chip_model));

	if (sc->sc_chip_model == UWE_CHIP_MARLIN3L)
		uwe5622_sdio_dt_dump(sc, UWE_SYNC_ADDR_M3L, 16, "SYNC");

	uwe5622_fw_probe_blob(sc, wcnmodem_38222_bin, wcnmodem_38222_bin_len);

	if (!sc->sc_transport_ready) {
		aprint_error_dev(self,
		    "transport not ready after firmware bringup\n");
		return;
	}

	aprint_normal_dev(self,
	    "attach complete, transport ready for upper-layer commands\n");

	uwe5622_sdio_sysctl_attach(sc);

#if UWE_ENABLE_NET80211
	uwe5622_sdio_net80211_attach(sc);
	/*
	 * Discover and publish the permanent firmware MAC before userland can
	 * open BPF on this interface.  If wpa_supplicant opens its EAPOL pcap
	 * handle while the attach-time placeholder (02:55:...) is still active,
	 * its generated destination filter permanently excludes EAPOL frames
	 * addressed to the real 78:8a:... station, even though the kernel RX
	 * path receives them.  Session setup is synchronous and the RX thread
	 * required for command replies was created by net80211_attach above.
	 */
	if (uwe5622_sdio_ensure_wifi_session(sc) != 0)
		aprint_error_dev(self,
		    "attach-time WiFi session setup failed; will retry on use\n");
#else
	aprint_normal_dev(self,
	    "net80211 attach disabled by build flag; enable later after validation\n");
#endif

#if UWE_AUTORUN_SELFTEST
	uwe5622_sdio_selftest(sc);
#else
	aprint_normal_dev(self,
	    "selftest autorun disabled; use manual commands for testing\n");
#endif
}


int
uwe5622_sdio_detach(device_t self, int flags)
{
	struct uwe5622_sdio_softc *sc;

	sc = device_private(self);

	sysctl_teardown(&sc->sc_sysctllog);

	uwe5622_sdio_net80211_detach(sc);

	if (sc->sc_sdio_ih != NULL) {
		sdmmc_intr_disestablish(sc->sc_sdio_ih);
		sc->sc_sdio_ih = NULL;
	}

	if (sc->sc_enabled && sc->sc_sf != NULL) {
		sdmmc_io_function_disable(sc->sc_sf);
		sc->sc_enabled = false;
	}

	callout_halt(&sc->sc_led_co, NULL);
	callout_destroy(&sc->sc_led_co);
	led_set_by_name(UWE_LED_NAME, LED_STATE_OFF);

	cv_destroy(&sc->sc_rx_cv);
	mutex_destroy(&sc->sc_rx_cv_lock);
	mutex_destroy(&sc->sc_bus_lock);

	return 0;
}

