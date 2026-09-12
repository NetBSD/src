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

#include <dev/microcode/uwe5622/uwe5622_ini.h>


/*
 * ---- helpers ----
 */

int __unused
uwe5622_sdio_wifi_set_regdom_world(struct uwe5622_sdio_softc *sc, uint8_t ctx_id)
{
	struct uwe_regdom_rule {
		uint32_t start_freq_khz;
		uint32_t end_freq_khz;
		uint32_t max_bandwidth_khz;
		uint32_t max_antenna_gain;
		uint32_t max_eirp;
		uint32_t flags;
		uint32_t dfs_cac_ms;
	} __packed;
	struct {
		uint32_t n_reg_rules;
		char alpha2[2];
		char pad[2];
		struct uwe_regdom_rule rules[2];
	} __packed rd;
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len;
	uint8_t rctx;
	int8_t status;
	int err;

	memset(&rd, 0, sizeof(rd));
	rd.n_reg_rules = htole32(2);
	rd.alpha2[0] = 'D';
	rd.alpha2[1] = 'E';

	/* 2.4ghz rule */
	rd.rules[0].start_freq_khz = htole32(2402000);
	rd.rules[0].end_freq_khz = htole32(2482000);
	rd.rules[0].max_bandwidth_khz = htole32(40000);
	rd.rules[0].max_antenna_gain = htole32(6000);
	rd.rules[0].max_eirp = htole32(2000);
	rd.rules[0].flags = htole32(0);
	rd.rules[0].dfs_cac_ms = htole32(0);

	/* 5ghz rule, broad passive-scan-only range, no DFS handling */
	rd.rules[1].start_freq_khz = htole32(5150000);
	rd.rules[1].end_freq_khz = htole32(5850000);
	rd.rules[1].max_bandwidth_khz = htole32(80000);
	rd.rules[1].max_antenna_gain = htole32(6000);
	rd.rules[1].max_eirp = htole32(2000);
	rd.rules[1].flags = htole32(0);
	rd.rules[1].dfs_cac_ms = htole32(0);

	reply_len = sizeof(reply);
	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_SET_REGDOM,
	    ctx_id, &rd, sizeof(rd), true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_SET_REGDOM,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;
	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "SET_REGDOM failed status=%d\n", status);
		return EIO;
	}

	aprint_normal_dev(sc->sc_dev,
	    "SET_REGDOM ok ctx=%u\n", rctx);
	return 0;
}

int
uwe5622_sdio_wifi_download_ini(struct uwe5622_sdio_softc *sc, uint8_t ctx_id)
{
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len;
	uint8_t rctx;
	int8_t status;
	int err;
	static const struct {
		const uint8_t *data;
		unsigned int len;
		const char *name;
	} sections[] = {
		{ wifi_ini_sec1, wifi_ini_sec1_len, "sec1" },
		{ wifi_ini_sec2, wifi_ini_sec2_len, "sec2" },
		/*
		 * Tried skipping sec3 entirely as an experiment (theory:
		 * attempting-and-failing it leaves firmware's RSN-capabilities
		 * computation reading uninitialized state) - inconclusive.
		 * The SAME bssid (50:e6:36:1b:f0:6c, chan 11) gave
		 * assoc_resp_status=1 in an earlier attempt WITH sec3 sent,
		 * then assoc_resp_status=45 in this attempt WITHOUT it - the
		 * rejection reason isn't even stable per-AP across attempts,
		 * suggesting something varies attempt-to-attempt regardless
		 * of sec3. Reverted to sending it (no clear downside either
		 * way, and skipping it isn't obviously safer for anything
		 * else sec3 might configure).
		 */
		{ wifi_ini_sec3, wifi_ini_sec3_len, "sec3" },
	};
	size_t i;

	for (i = 0; i < __arraycount(sections); i++) {
		reply_len = sizeof(reply);
		memset(reply, 0, sizeof(reply));

		err = uwe5622_sdio_send_wifi_cmd(sc,
		    UWE_WIFI_CMD_DOWNLOAD_INI, ctx_id,
		    sections[i].data, sections[i].len, true);
		if (err)
			return err;

		err = uwe5622_sdio_wait_wifi_reply(sc,
		    UWE_WIFI_CMD_DOWNLOAD_INI,
		    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len,
		    &rctx, &status);
		if (err)
			return err;
		// if (status != 0) {
		// 	aprint_error_dev(sc->sc_dev,
		// 	    "DOWNLOAD_INI %s failed status=%d\n",
		// 	    sections[i].name, status);
		// 	return EIO;
		// }
		if (status != 0) {
			aprint_normal_dev(sc->sc_dev,
				"DOWNLOAD_INI %s rejected status=%d (continuing)\n",
				sections[i].name, status);
			continue;  // return EIO yerine continue
		}

		aprint_normal_dev(sc->sc_dev,
		    "DOWNLOAD_INI %s ok (%u bytes)\n",
		    sections[i].name, sections[i].len);
	}

	return 0;
}
 

uint8_t
uwe5622_sdio_common_build(uint8_t type, uint8_t reserv, uint8_t rsp, uint8_t ctx_id)
{
	return (uint8_t)((type & 0x7) |
	    ((reserv & 0x1) << 3) |
	    ((rsp & 0x1) << 4) |
	    ((ctx_id & 0x7) << 5));
}

uint8_t
uwe5622_sdio_common_type(uint8_t raw)
{
	return raw & 0x7;
}

uint8_t
uwe5622_sdio_common_rsp(uint8_t raw)
{
	return (raw >> 4) & 0x1;
}

uint8_t
uwe5622_sdio_common_ctx(uint8_t raw)
{
	return (raw >> 5) & 0x7;
}

void
uwe5622_sdio_dump_payload_ascii(const uint8_t *buf, size_t len)
{
	size_t i, n;

	n = len;
	if (n > 64)
		n = 64;

	printf(" ascii='");
	for (i = 0; i < n; i++) {
		uint8_t c;

		c = buf[i];
		if (c >= 0x20 && c <= 0x7e)
			printf("%c", c);
		else
			printf(".");
	}
	printf("'");
}

void
uwe5622_sdio_dump_bytes(struct uwe5622_sdio_softc *sc, const char *tag,
    const void *vbuf, size_t len)
{
	const uint8_t *buf = vbuf;
	size_t i, n;

	n = len;
	if (n > UWE_DUMP_MAX_BYTES)
		n = UWE_DUMP_MAX_BYTES;

	aprint_debug("%s: %s (%zu bytes):",
	    device_xname(sc->sc_dev), tag, len);
	for (i = 0; i < n; i++)
		aprint_debug(" %02x", buf[i]);
	if (len > n)
		aprint_debug(" ...");
	aprint_debug("\n");
}

void
uwe5622_sdio_log_printable_segments(struct uwe5622_sdio_softc *sc,
    const uint8_t *buf, size_t len)
{
	char seg[UWE_LOG_ASCII_EXTRACT_BUFSZ];
	size_t i, start, seglen, copylen;
	bool printed_any;

	printed_any = false;
	i = 0;

	while (i < len) {
		while (i < len) {
			uint8_t c = buf[i];
			if (c >= 0x20 && c <= 0x7e)
				break;
			i++;
		}

		if (i >= len)
			break;

		start = i;
		while (i < len) {
			uint8_t c = buf[i];
			if (!(c >= 0x20 && c <= 0x7e))
				break;
			i++;
		}

		seglen = i - start;

		if (seglen < 8)
			continue;

		if (seglen == 4 &&
		    buf[start + 0] == '~' &&
		    buf[start + 1] == '~' &&
		    buf[start + 2] == '~' &&
		    buf[start + 3] == '~')
			continue;

		copylen = seglen;
		if (copylen >= sizeof(seg))
			copylen = sizeof(seg) - 1;

		memcpy(seg, buf + start, copylen);
		seg[copylen] = '\0';

		aprint_debug_dev(sc->sc_dev, "diag/log text: %s\n", seg);
		printed_any = true;
	}

	if (!printed_any) {
		aprint_debug_dev(sc->sc_dev,
		    "diag/log packet observed len=%zu\n", len);
	}
}

/*
 * ---- pkt stats ----
 */

void
uwe5622_sdio_pkt_stat_note(struct uwe5622_sdio_softc *sc,
    uint8_t type, uint8_t subtype, uint16_t len)
{
	struct uwe_pkt_stat *slot;
	size_t i;

	sc->sc_rx_total_packets++;

	for (i = 0; i < UWE_PKT_STAT_SLOTS; i++) {
		slot = &sc->sc_pkt_stats[i];
		if (!slot->used)
			continue;
		if (slot->type == type && slot->subtype == subtype) {
			slot->count++;
			slot->last_len = len;
			return;
		}
	}

	for (i = 0; i < UWE_PKT_STAT_SLOTS; i++) {
		slot = &sc->sc_pkt_stats[i];
		if (slot->used)
			continue;

		slot->used = true;
		slot->type = type;
		slot->subtype = subtype;
		slot->count = 1;
		slot->last_len = len;

		aprint_normal_dev(sc->sc_dev,
		    "new RX packet kind: type=%u subtype=%u len=%u\n",
		    type, subtype, len);
		return;
	}
}

void
uwe5622_sdio_pkt_stat_dump(struct uwe5622_sdio_softc *sc)
{
	struct uwe_pkt_stat *slot;
	size_t i;

	aprint_normal_dev(sc->sc_dev,
	    "RX census: total=%u unhandled=%u\n",
	    sc->sc_rx_total_packets, sc->sc_rx_unhandled_packets);

	for (i = 0; i < UWE_PKT_STAT_SLOTS; i++) {
		slot = &sc->sc_pkt_stats[i];
		if (!slot->used)
			continue;

		aprint_normal_dev(sc->sc_dev,
		    "RX census[%zu]: type=%u subtype=%u count=%u last_len=%u\n",
		    i, slot->type, slot->subtype, slot->count, slot->last_len);
	}
}

/*
 * ---- name tables ----
 */

const char *
uwe5622_sdio_wifi_cmd_name(uint8_t cmd_id)
{
	switch (cmd_id) {
	case UWE_WIFI_CMD_GET_INFO:	return "GET_INFO";
	case UWE_WIFI_CMD_SET_REGDOM:	return "SET_REGDOM";
	case UWE_WIFI_CMD_OPEN:		return "OPEN";
	case UWE_WIFI_CMD_CLOSE:	return "CLOSE";
	case UWE_WIFI_CMD_POWER_SAVE:	return "POWER_SAVE";
	case UWE_WIFI_CMD_SET_PARAM:	return "SET_PARAM";
	case UWE_WIFI_CMD_SET_CHANNEL:	return "SET_CHANNEL";
	case UWE_WIFI_CMD_REQ_LTE_CONCUR: return "REQ_LTE_CONCUR";
	case UWE_WIFI_CMD_SYNC_VERSION:	return "SYNC_VERSION";
	case UWE_WIFI_CMD_CONNECT:	return "CONNECT";
	case UWE_WIFI_CMD_SCAN:		return "SCAN";
	case UWE_WIFI_CMD_SCHED_SCAN:	return "SCHED_SCAN";
	case UWE_WIFI_CMD_KEY:		return "KEY";
	case UWE_WIFI_CMD_SET_IE:	return "SET_IE";
	case UWE_WIFI_CMD_NOTIFY_IP_ACQUIRED: return "NOTIFY_IP_ACQUIRED";
	case UWE_WIFI_CMD_BA:		return "BA";
	case UWE_WIFI_CMD_DOWNLOAD_INI: return "DOWNLOAD_INI";
	default:			return "UNKNOWN_CMD";
	}
}

const char *
uwe5622_sdio_wifi_event_name(uint8_t event_id)
{
	switch (event_id) {
	case UWE_WIFI_EVENT_CONNECT:	return "CONNECT";
	case UWE_WIFI_EVENT_DISCONNECT:	return "DISCONNECT";
	case UWE_WIFI_EVENT_SCAN_DONE:	return "SCAN_DONE";
	case UWE_WIFI_EVENT_MGMT_FRAME:	return "MGMT_FRAME";
	case UWE_WIFI_EVENT_GSCAN_FRAME: return "GSCAN_FRAME";
	case UWE_WIFI_EVENT_SDIO_FLOWCON: return "SDIO_FLOWCON";
	case UWE_WIFI_EVENT_BA:		return "BA";
	case UWE_WIFI_EVENT_STA_LUT_INDEX: return "STA_LUT_INDEX";
	case UWE_WIFI_EVENT_FW_PWR_DOWN: return "FW_PWR_DOWN";
	default:			return "UNKNOWN_EVENT";
	}
}

/*
 * ---- rx handlers ----
 */

void
uwe5622_sdio_handle_at_payload(struct uwe5622_sdio_softc *sc,
    uint8_t subtype, const uint8_t *payload, size_t len)
{
	size_t copy_len;

	copy_len = len;
	if (copy_len >= sizeof(sc->sc_at_reply))
		copy_len = sizeof(sc->sc_at_reply) - 1;

	memcpy(sc->sc_at_reply, payload, copy_len);
	sc->sc_at_reply[copy_len] = '\0';
	sc->sc_at_reply_len = copy_len;
	sc->sc_at_reply_subtype = subtype;
	sc->sc_at_reply_ready = true;

	aprint_normal_dev(sc->sc_dev,
	    "AT/status reply received subtype=%u len=%zu\n",
	    subtype, copy_len);
	uwe5622_sdio_dump_bytes(sc, "AT/status payload", payload, len);
}

void
uwe5622_sdio_handle_log_payload(struct uwe5622_sdio_softc *sc,
    const uint8_t *payload, size_t len)
{
	size_t preview_len;

	preview_len = len;
	if (preview_len > sizeof(sc->sc_log_preview))
		preview_len = sizeof(sc->sc_log_preview);

	memcpy(sc->sc_log_preview, payload, preview_len);
	sc->sc_log_preview_len = preview_len;
	sc->sc_log_seen = true;

	uwe5622_sdio_log_printable_segments(sc, payload, len);
}

void
uwe5622_sdio_handle_wifi_cmd_payload(struct uwe5622_sdio_softc *sc,
    const uint8_t *payload, size_t len)
{
	const struct uwe_sprdwl_cmd_hdr *hdr;
	size_t plen, paylen;

	if (len < sizeof(*hdr)) {
		aprint_error_dev(sc->sc_dev,
		    "short wifi-cmd reply payload len=%zu\n", len);
		return;
	}

	hdr = (const struct uwe_sprdwl_cmd_hdr *)payload;
	plen = le16toh(hdr->plen_le);
	if (plen < sizeof(*hdr))
		paylen = 0;
	else
		paylen = plen - sizeof(*hdr);
	if (paylen > len - sizeof(*hdr))
		paylen = len - sizeof(*hdr);
	if (paylen > sizeof(sc->sc_wifi_reply))
		paylen = sizeof(sc->sc_wifi_reply);

	/*
	 * Only deliver this reply into the single-slot mailbox if it's the
	 * exact command uwe5622_sdio_wait_wifi_reply() is currently blocked
	 * on (sc_wifi_pending_cmd) - see that field's comment. A reply for
	 * any other cmd_id (e.g. firmware's own auto-ack for a
	 * fire-and-forget send) is still logged below but must not touch
	 * the mailbox, since there is no protection against it clobbering
	 * an already-arrived-but-not-yet-read matching reply otherwise.
	 */
	if (hdr->cmd_id == sc->sc_wifi_pending_cmd) {
		memcpy(sc->sc_wifi_reply, payload + sizeof(*hdr), paylen);
		sc->sc_wifi_reply_len = paylen;
		sc->sc_wifi_reply_cmd_id = hdr->cmd_id;
		sc->sc_wifi_reply_ctx_id = uwe5622_sdio_common_ctx(hdr->common.raw);
		sc->sc_wifi_reply_status = hdr->status;
		sc->sc_wifi_reply_ready = true;
	}

	if (hdr->cmd_id != UWE_WIFI_CMD_SCAN || sc->sc_verbose || hdr->status != 0)
		aprint_normal_dev(sc->sc_dev,
		    "wifi-cmd reply: cmd=%s(%u) ctx=%u status=%d rsp=%u "
		    "plen=%zu paylen=%zu\n",
		    uwe5622_sdio_wifi_cmd_name(hdr->cmd_id), hdr->cmd_id,
		    uwe5622_sdio_common_ctx(hdr->common.raw), hdr->status,
		    uwe5622_sdio_common_rsp(hdr->common.raw), plen, paylen);
	uwe5622_sdio_dump_bytes(sc, "wifi-cmd reply body", payload, len);
}

void
uwe5622_sdio_handle_wifi_event_payload(struct uwe5622_sdio_softc *sc,
    const uint8_t *payload, size_t len)
{
	const struct uwe_sprdwl_cmd_hdr *hdr;
	uint8_t ctx;

	if (len < sizeof(*hdr)) {
		aprint_error_dev(sc->sc_dev,
		    "short wifi-event payload len=%zu\n", len);
		return;
	}

	hdr = (const struct uwe_sprdwl_cmd_hdr *)payload;
	ctx = uwe5622_sdio_common_ctx(hdr->common.raw);

	if ((hdr->cmd_id != UWE_WIFI_EVENT_MGMT_FRAME &&
	    hdr->cmd_id != UWE_WIFI_EVENT_SCAN_DONE &&
	    hdr->cmd_id != UWE_WIFI_EVENT_SDIO_FLOWCON) || sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "wifi-event: event=%s(0x%02x) ctx=%u status=%d len=%zu\n",
		    uwe5622_sdio_wifi_event_name(hdr->cmd_id), hdr->cmd_id,
		    ctx, hdr->status, len);
	uwe5622_sdio_dump_bytes(sc, "wifi-event body", payload, len);

	if (hdr->cmd_id == UWE_WIFI_EVENT_DISCONNECT) {
		sc->sc_sta_lut_valid = false;
		sc->sc_ipv4_notified = false;
		memset(sc->sc_notified_ipv4, 0, sizeof(sc->sc_notified_ipv4));
		if (sc->sc_net80211_attached) {
			if_link_state_change(&sc->sc_if, LINK_STATE_DOWN);
			if (sc->sc_ic.ic_state != IEEE80211_S_INIT)
				(void)ieee80211_new_state(&sc->sc_ic,
				    IEEE80211_S_INIT, -1);
		}
	}

	if (hdr->cmd_id == UWE_WIFI_EVENT_CONNECT)
		sc->sc_fw_tx_asserted = false;

	if (hdr->cmd_id == UWE_WIFI_EVENT_MGMT_FRAME)
		uwe5622_sdio_try_log_mgmt_frame(sc, payload, len);

	/* cmdevt.h: struct sprdwl_event_ba is exactly eight bytes. */
	if (hdr->cmd_id == UWE_WIFI_EVENT_BA &&
	    len >= sizeof(*hdr) + 8) {
		const uint8_t *ba = payload + sizeof(*hdr);
		uint16_t param0 = le16dec(ba + 4);
		uint16_t param1 = le16dec(ba + 6);

		atomic_inc_32(&sc->sc_ba_events);
		aprint_normal_dev(sc->sc_dev,
		    "BA event: type=%u tid=%u lut=%u reserved=%u "
		    "param0=%u param1=%u%s\n",
		    ba[0], ba[1], ba[2], ba[3], param0, param1,
		    ba[0] == 0 ? " (ADDBA_REQ: win_start/win_size)" : "");

		/*
		 * Vendor send_addba_rsp(): type, tid, peer DA, success.
		 *
		 * Fire-and-forget (no uwe5622_sdio_wait_wifi_reply() here
		 * anymore): this handler runs from WITHIN RX event dispatch,
		 * which itself can be invoked reentrantly from inside some
		 * OTHER, unrelated wait_wifi_reply() call's own
		 * rx_consume_once() (e.g. a WIFI_CMD_KEY sender's wait loop).
		 * The reply mechanism is a single shared slot with no
		 * queueing - a nested BA wait here would, on seeing a
		 * mismatched reply, unconditionally clear
		 * sc_wifi_reply_ready, silently discarding a real reply some
		 * OUTER caller (like the KEY sender) was actually waiting
		 * for. Confirmed on real hardware (2026-08-26): 8 ADDBA_REQ
		 * events arriving within ~150ms during WPA2 group-key
		 * install caused exactly this - the GTK's own KEY(14) reply
		 * got lost, timing out with "KEY add failed ... err=60" even
		 * though the firmware did reply. We don't need synchronous
		 * confirmation of the ADDBA response for correctness (worst
		 * case of a silent failure is degraded BA aggregation on
		 * that TID, not a connectivity break), so just send it and
		 * move on - this fully avoids ever nesting a
		 * wait_wifi_reply() call inside RX dispatch.
		 */
		if (ba[0] == 0 && sc->sc_sta_lut_valid) {
			uint8_t rsp[9];
			int error;

			memset(rsp, 0, sizeof(rsp));
			rsp[0] = 1; /* SPRDWL_ADDBA_RSP_CMD */
			rsp[1] = ba[1];
			memcpy(rsp + 2, sc->sc_sta_ra, UWE_WIFI_MAC_LEN);
			rsp[8] = 1; /* success */
			error = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_BA,
			    ctx, rsp, sizeof(rsp), false);
			if (error == 0) {
				atomic_inc_32(&sc->sc_ba_rsp_ok);
				if (sc->sc_verbose)
					aprint_normal_dev(sc->sc_dev,
					    "BA ADDBA response sent tid=%u lut=%u\n",
					    ba[1], ba[2]);
			} else {
				atomic_inc_32(&sc->sc_ba_rsp_error);
				aprint_error_dev(sc->sc_dev,
				    "BA ADDBA response send failed tid=%u err=%d\n",
				    ba[1], error);
			}
		}
	}

	/*
	 * struct sprdwl_sta_lut_ind { u8 ctx_id; u8 action; u8 sta_lut_index;
	 * u8 ra[6]; u8 is_ht_enable; u8 is_vht_enable; } __packed - 11 bytes,
	 * confirmed against vendor cmdevt.h (armbian/uwe5622 fork). We've
	 * received this event (logged as UNKNOWN_EVENT(0xf5), len=23 = our
	 * 12-byte generic header + this 11-byte payload) right after every
	 * successful CONNECT but never parsed it. sta_lut_index is an
	 * AP-assigned per-station table index some TX paths on this chip
	 * family need embedded in outgoing frame descriptors; unclear yet
	 * whether the simpler 8-byte sprdwl_data_hdr TX path this driver
	 * uses actually requires it too - stash it either way so it's
	 * available if a future TX fix needs it, and log it now to see
	 * whether it's ever actually a meaningful nonzero value.
	 */
	if (hdr->cmd_id == UWE_WIFI_EVENT_STA_LUT_INDEX &&
	    len >= sizeof(*hdr) + 11) {
		const uint8_t *lut = payload + sizeof(*hdr);
		uint8_t lut_ctx_id = lut[0];
		uint8_t lut_action = lut[1];
		uint8_t lut_index = lut[2];
		const uint8_t *ra = lut + 3;
		uint8_t is_ht = lut[9];
		uint8_t is_vht = lut[10];

		/* action: 0=DEL, 1=ADD, 2=UPD (inferred from observed values) */
		if (lut_action == 0) {
			sc->sc_sta_lut_valid = false;
		} else {
			sc->sc_sta_lut_index = lut_index;
			sc->sc_sta_lut_valid = true;
			memcpy(sc->sc_sta_ra, ra, UWE_WIFI_MAC_LEN);
		}

		aprint_normal_dev(sc->sc_dev,
		    "sta_lut_index event: ctx_id=%u action=%u lut_index=%u "
		    "ra=%s ht=%u vht=%u\n",
		    lut_ctx_id, lut_action, lut_index,
		    ether_sprintf(ra), is_ht, is_vht);
	}

	/*
	 * cmdevt.c's own event-dispatch switch is indeed a no-op
	 * (case WIFI_EVENT_SDIO_FLOWCON: break;) - but that's misleading:
	 * real credit accounting happens EARLIER, in tx_msg.c's
	 * sprdwl_sdio_process_credit(), called from intf_rx_handle() on
	 * every inbound SDIO buffer BEFORE dispatch ever reaches cmdevt.c.
	 * That function atomic_add()s each of the 4 payload bytes into a
	 * per-"color" credit pool (tx_msg->flow_ctrl[i].flow), consumed
	 * one-per-send in sprdwl_fc_get_send_num()/sprdwl_intf_fill_msdu_dscr()
	 * BEFORE writing to SDIO - not merely logged. All 4 bytes being 0 is
	 * a real, distinct "reset credit" signal from firmware, not just an
	 * empty update. This driver only ever uses color 0 (single STA-mode
	 * vif), so only sc_tx_credit[0] is actually drawn down in
	 * uwe5622_sdio_tx_data(); the others are tracked for completeness.
	 */
	if (hdr->cmd_id == UWE_WIFI_EVENT_SDIO_FLOWCON && len > sizeof(*hdr)) {
		size_t extra = len - sizeof(*hdr);
		size_t i;
		bool all_zero = true;

		if (sc->sc_verbose)
			aprint_normal_dev(sc->sc_dev,
			    "flowcon payload (%zu bytes):", extra);
		for (i = 0; i < extra; i++) {
			if (sc->sc_verbose)
				aprint_normal(" %02x", payload[sizeof(*hdr) + i]);
			if (payload[sizeof(*hdr) + i] != 0)
				all_zero = false;
		}
		if (sc->sc_verbose)
			aprint_normal("\n");

		if (all_zero) {
			sc->sc_tx_credit[0] = 0;
			sc->sc_tx_credit[1] = 0;
			sc->sc_tx_credit[2] = 0;
			sc->sc_tx_credit[3] = 0;
		} else {
			for (i = 0; i < extra && i < 4; i++) {
				uint8_t add = payload[sizeof(*hdr) + i];

				if (add != 0)
					atomic_add_32(&sc->sc_tx_credit[i], add);
			}
		}

		/* A packet held for lack of credit can now be retried. */
		if (sc->sc_tx_waiting_credit && sc->sc_tx_wq != NULL &&
		    (sc->sc_tx_credit[0] != 0 || sc->sc_tx_credit[1] != 0 ||
		     sc->sc_tx_credit[2] != 0 || sc->sc_tx_credit[3] != 0) &&
		    atomic_cas_uint(&sc->sc_tx_pending, 0, 1) == 0) {
			sc->sc_tx_waiting_credit = false;
			atomic_inc_32(&sc->sc_tx_schedule_claims);
			workqueue_enqueue(sc->sc_tx_wq, &sc->sc_tx_work, NULL);
		}
	}

 	if (hdr->cmd_id == UWE_WIFI_EVENT_SCAN_DONE) {
		uint8_t evtype;

		sc->sc_wifi_scan_done = true;
		sc->sc_wifi_scan_aborted = false;

		if (len > sizeof(*hdr)) {
			evtype = payload[sizeof(*hdr)];
			sc->sc_wifi_scan_aborted =
			    (evtype == UWE_SPRDWL_SCAN_ERROR);
			if (sc->sc_verbose || sc->sc_wifi_scan_aborted)
				aprint_normal_dev(sc->sc_dev,
				    "scan-done event: type=%u hdr_status=%d "
				    "aborted=%s\n", evtype, hdr->status,
				    sc->sc_wifi_scan_aborted ? "yes" : "no");
 		} else {
			aprint_normal_dev(sc->sc_dev,
			    "scan-done event without payload hdr_status=%d\n",
			    hdr->status);
		}
		uwe5622_sdio_finish_net80211_scan(sc);
	}

	if (hdr->cmd_id == UWE_WIFI_EVENT_CONNECT) {
		sc->sc_wifi_connect_done = true;

		/*
		 * hdr->status (from the generic cmd_hdr overlay) was always 0
		 * here and we trusted that as "success" - but vendor's real
		 * sprdwl_event_connect() does NOT use that struct for this
		 * event at all. It parses the event-specific payload fresh
		 * from its own byte 0: a 1-byte status (0=success, matching
		 * every other status enum in this codebase), and ONLY on
		 * failure, a real IEEE 802.11 association-response status
		 * code at byte offset 2 ("Assoc response status code by set
		 * in the 3 byte if failure" per vendor's comment). That
		 * event-specific payload is exactly the bytes after our
		 * 12-byte generic header. Confirmed on real hardware: got
		 * "01 00 2d" here once AKM/PSK/channel bugs were fixed enough
		 * to reach a real AP - 0x2d=45="Invalid RSN Information
		 * Element Capabilities" in the 802.11 status code table. The
		 * AP is genuinely receiving and rejecting our association.
		 */
		if (len > sizeof(*hdr)) {
			const uint8_t *evt = payload + sizeof(*hdr);
			size_t evt_len = len - sizeof(*hdr);

			if (evt[0] != 0) {
				uint8_t assoc_status =
				    evt_len > 2 ? evt[2] : 0;

				aprint_error_dev(sc->sc_dev,
				    "connect event: real failure, "
				    "conn_status=%u assoc_resp_status=%u "
				    "(802.11 status code - see IEEE 802.11 "
				    "Table 9-46)\n",
				    evt[0], assoc_status);
				sc->sc_sta_lut_valid = false;
				if (sc->sc_net80211_attached) {
					if_link_state_change(&sc->sc_if,
					    LINK_STATE_DOWN);
					if (sc->sc_ic.ic_state != IEEE80211_S_INIT)
						(void)ieee80211_new_state(
						    &sc->sc_ic,
						    IEEE80211_S_INIT, -1);
				}
				return;
			}
		}

		if (hdr->status != 0) {
			aprint_error_dev(sc->sc_dev,
			    "connect event: failed status=%d\n", hdr->status);
			return;
		}

		aprint_normal_dev(sc->sc_dev,
		    "connect event: success, notifying net80211\n");

		if (sc->sc_net80211_attached && sc->sc_ic.ic_bss != NULL) {
			struct ieee80211com *ic = &sc->sc_ic;
			struct ieee80211_node *ni = ic->ic_bss;
			int sl = ic->ic_des_esslen;

			if (sl > IEEE80211_NWID_LEN)
				sl = IEEE80211_NWID_LEN;
			if (sl > 0) {
				memcpy(ni->ni_essid, ic->ic_des_essid, sl);
				ni->ni_esslen = (uint8_t)sl;
			}
			/*
			 * This is a full-MAC association, so net80211 never parses an
			 * association response and cannot learn the current BSSID by
			 * itself.  Keep ic_bss/des_bssid synchronized with the target;
			 * driver_bsd queries this immediately after RTM_IEEE80211_ASSOC.
			 */
			if (sc->sc_wifi_connect_bssid[0] != 0 ||
			    sc->sc_wifi_connect_bssid[1] != 0 ||
			    sc->sc_wifi_connect_bssid[2] != 0 ||
			    sc->sc_wifi_connect_bssid[3] != 0 ||
			    sc->sc_wifi_connect_bssid[4] != 0 ||
			    sc->sc_wifi_connect_bssid[5] != 0) {
				IEEE80211_ADDR_COPY(ni->ni_bssid,
				    sc->sc_wifi_connect_bssid);
				IEEE80211_ADDR_COPY(ic->ic_des_bssid,
				    sc->sc_wifi_connect_bssid);
				ic->ic_flags |= IEEE80211_F_DESBSSID;
			}

			/*
			 * ic_bss never had its channel updated to the real
			 * one we connected on - ifconfig kept showing a
			 * stale/default channel regardless of what CONNECT
			 * actually used. sc_wifi_connect_chan was recorded
			 * in uwe5622_sdio_wifi_cmd_connect() right before
			 * sending the command.
			 */
			if (sc->sc_wifi_connect_chan > 0 &&
			    sc->sc_wifi_connect_chan <= IEEE80211_CHAN_MAX &&
			    ic->ic_channels[sc->sc_wifi_connect_chan].ic_freq != 0)
				ni->ni_chan =
				    &ic->ic_channels[sc->sc_wifi_connect_chan];

			/*
			 * ieee80211_newstate()'s RUN-state entry for the
			 * SCAN->RUN transition (our manual jump, bypassing
			 * the normal AUTH/ASSOC dance) asserts
			 * ni_txrate < ni_rates.rs_nrates. ic_bss is our
			 * driver's own placeholder node, never populated
			 * with a rate set (firmware negotiates rates
			 * internally, net80211 never sees the real
			 * association response) - rs_nrates was 0, so the
			 * assert always failed here, panicking on every real
			 * CONNECT success. A default rateset is enough to
			 * satisfy net80211's bookkeeping; it doesn't affect
			 * what firmware actually transmits at.
			 */
			if (ni->ni_rates.rs_nrates == 0)
				ni->ni_rates = ieee80211_std_rateset_11g;
			ni->ni_txrate = 0;

			/*
			 * sta_join leaves net80211 in AUTH.  AUTH -> RUN is an
			 * invalid shortcut and suppresses RTM_IEEE80211_ASSOC, so
			 * wpa_supplicant never starts processing the received EAPOL.
			 */
			if (ic->ic_state == IEEE80211_S_AUTH)
				(void)ieee80211_new_state(ic, IEEE80211_S_ASSOC, -1);

			/*
			 * AUTH -> ASSOC can replace ic_bss.  Do not rely on the node
			 * cached above: the replacement has an empty rate set and RUN
			 * asserts ni_txrate < rs_nrates.  Repopulate all full-MAC state
			 * on the node that RUN will actually consume.
			 */
			ni = ic->ic_bss;
			if (ni != NULL) {
				if (sl > 0) {
					memcpy(ni->ni_essid, ic->ic_des_essid, sl);
					ni->ni_esslen = (uint8_t)sl;
				}
				if (sc->sc_wifi_connect_chan > 0 &&
				    sc->sc_wifi_connect_chan <= IEEE80211_CHAN_MAX &&
				    ic->ic_channels[sc->sc_wifi_connect_chan].ic_freq != 0)
					ni->ni_chan = &ic->ic_channels[
					    sc->sc_wifi_connect_chan];
				if (sc->sc_wifi_connect_bssid[0] != 0 ||
				    sc->sc_wifi_connect_bssid[1] != 0 ||
				    sc->sc_wifi_connect_bssid[2] != 0 ||
				    sc->sc_wifi_connect_bssid[3] != 0 ||
				    sc->sc_wifi_connect_bssid[4] != 0 ||
				    sc->sc_wifi_connect_bssid[5] != 0)
					IEEE80211_ADDR_COPY(ni->ni_bssid,
					    sc->sc_wifi_connect_bssid);
				if (ni->ni_rates.rs_nrates == 0)
					ni->ni_rates = ieee80211_std_rateset_11g;
				ni->ni_txrate = 0;
			}
			(void)ieee80211_new_state(ic, IEEE80211_S_RUN,
			    IEEE80211_FC0_SUBTYPE_ASSOC_RESP);

			/*
			 * net80211's default newstate() does not touch
			 * if_link_state - without this, IPv4/IPv6 addresses
			 * assigned to this interface stay DETACHED (unusable
			 * for routing) even though ic_state/media report
			 * "active", since the ifnet-generic link-state the
			 * address-usability logic actually checks was never
			 * updated.
			 */
			if_link_state_change(&sc->sc_if, LINK_STATE_UP);
		}
	}
}

int
uwe5622_sdio_send_cmd(struct uwe5622_sdio_softc *sc, uint8_t subtype,
    const void *payload, size_t payload_len)
{
	uint8_t *buf;
	uint32_t hdr;
	size_t aligned_len, total_len, off;
	int err;

	if (payload_len == 0)
		return EINVAL;

	aligned_len = (payload_len + 3) & ~3U;
	total_len = 4 + aligned_len + 4;
	buf = kmem_zalloc(total_len, KM_SLEEP);

	hdr = uwe5622_sdio_pkt_build_raw(UWE_PKT_TYPE_CTRL, subtype, 0, aligned_len, 0);

	buf[0] = (hdr >> 0) & 0xff;
	buf[1] = (hdr >> 8) & 0xff;
	buf[2] = (hdr >> 16) & 0xff;
	buf[3] = (hdr >> 24) & 0xff;

	memcpy(buf + 4, payload, payload_len);

	off = 4 + aligned_len;
	buf[off + 0] = 0x00;
	buf[off + 1] = 0x00;
	buf[off + 2] = 0x80;
	buf[off + 3] = 0x00;

	err = uwe5622_sdio_pkt_write(sc, buf, total_len);
	kmem_free(buf, total_len);

	return err;
}

int
uwe5622_sdio_wait_wifi_reply(struct uwe5622_sdio_softc *sc, uint8_t expect_cmd,
    int timeout_ms, uint8_t *reply, size_t *reply_len, uint8_t *ctx_id,
    int8_t *status)
{
	int err, elapsed;
	size_t copy_len;

	for (elapsed = 0;
	     elapsed < timeout_ms;
	     elapsed += UWE_AT_POLL_DELAY_US / 1000) {
		err = uwe5622_sdio_rx_consume_once(sc, NULL);
		if (err) {
			sc->sc_wifi_pending_cmd = 0;
			return err;
		}
		if (!sc->sc_wifi_reply_ready) {
			delay(UWE_AT_POLL_DELAY_US);
			continue;
		}

		/*
		 * uwe5622_sdio_handle_wifi_cmd_payload() only ever sets
		 * sc_wifi_reply_ready for a reply whose cmd_id matched
		 * sc_wifi_pending_cmd (== expect_cmd, set by
		 * uwe5622_sdio_send_wifi_cmd() before this wait started), so
		 * a mismatch here is no longer possible - see that mailbox
		 * field's comment for why match-at-write (not match-at-read)
		 * is required now that the dedicated RX kthread and this
		 * function's own uwe5622_sdio_rx_consume_once() race to
		 * dispatch incoming packets.
		 */
		if (reply != NULL && reply_len != NULL) {
			copy_len = sc->sc_wifi_reply_len;
			if (copy_len > *reply_len)
				copy_len = *reply_len;
			memcpy(reply, sc->sc_wifi_reply, copy_len);
			*reply_len = copy_len;
		}
		if (ctx_id != NULL)
			*ctx_id = sc->sc_wifi_reply_ctx_id;
		if (status != NULL)
			*status = sc->sc_wifi_reply_status;
		sc->sc_wifi_pending_cmd = 0;
		return 0;
	}

	sc->sc_wifi_pending_cmd = 0;
	aprint_error_dev(sc->sc_dev,
	    "wifi-cmd timeout waiting for %s(%u)\n",
	    uwe5622_sdio_wifi_cmd_name(expect_cmd), expect_cmd);
	return ETIMEDOUT;
}

int
uwe5622_sdio_wifi_cmd_open(struct uwe5622_sdio_softc *sc, uint8_t mode,
    const uint8_t *mac, uint8_t *ctx_id)
{
	struct uwe_sprdwl_cmd_open open_cmd;
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len;
	uint8_t rctx;
	int8_t status;
	int err;

	memset(&open_cmd, 0, sizeof(open_cmd));
	open_cmd.mode = mode;
	if (mac != NULL)
		memcpy(open_cmd.mac, mac, sizeof(open_cmd.mac));
	reply_len = sizeof(reply);
	memset(reply, 0, sizeof(reply));

	/*
	 * Reset credit tracking BEFORE sending OPEN, not after receiving
	 * its reply: real hardware logs show the initial FLOWCON (with the
	 * real "10 10 10 10" starting budget) can arrive and get processed
	 * BEFORE the OPEN command's own reply, when both land in the same
	 * RX poll batch. Resetting after the OPEN-success branch (the
	 * original, buggy placement) wiped out real credit that had
	 * already arrived, so every single send failed with "no credit"
	 * from the very first attempt.
	 */
	{
		int _ci;
		for (_ci = 0; _ci < 4; _ci++)
			sc->sc_tx_credit[_ci] = 0;
	}

	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_OPEN, 0,
	    &open_cmd, sizeof(open_cmd), true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_OPEN,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;
	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_OPEN failed status=%d\n", status);
		return EIO;
	}

	if (ctx_id != NULL)
		*ctx_id = rctx;
	sc->sc_wifi_ctx_id = rctx;
	aprint_normal_dev(sc->sc_dev,
	    "WIFI_CMD_OPEN ok mode=%u ctx_id=%u\n", mode, rctx);
	return 0;
}

int
uwe5622_sdio_wifi_cmd_close(struct uwe5622_sdio_softc *sc, uint8_t ctx_id,
    uint8_t mode)
{
	struct uwe_sprdwl_cmd_close close_cmd;
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len;
	uint8_t rctx;
	int8_t status;
	int err;

	memset(&close_cmd, 0, sizeof(close_cmd));
	close_cmd.mode = mode;
	reply_len = sizeof(reply);
	memset(reply, 0, sizeof(reply));

	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_CLOSE, ctx_id,
	    &close_cmd, sizeof(close_cmd), true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_CLOSE,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;
	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_CLOSE failed status=%d\n", status);
		return EIO;
	}

	if (sc->sc_wifi_ctx_id == ctx_id)
		sc->sc_wifi_ctx_id = UWE_WIFI_CTX_INVALID;
	aprint_normal_dev(sc->sc_dev,
	    "WIFI_CMD_CLOSE ok mode=%u ctx_id=%u\n", mode, ctx_id);
	return 0;
}

int
uwe5622_sdio_wifi_cmd_get_info(struct uwe5622_sdio_softc *sc, uint8_t ctx_id)
{
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len;
	uint8_t rctx;
	int8_t status;
	const struct uwe_sprdwl_cmd_fw_info *info;
	int err;

	reply_len = sizeof(reply);
	memset(reply, 0, sizeof(reply));

	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_GET_INFO, ctx_id,
	    NULL, 0, true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_GET_INFO,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;
	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_GET_INFO failed status=%d\n", status);
		return EIO;
	}
	if (reply_len < sizeof(*info)) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_GET_INFO short reply len=%zu\n", reply_len);
		return EPROTO;
	}

	/*
	 * Unconditional (not gated by sc_verbose): GET_INFO runs once per
	 * bring-up, before the user typically has a chance to flip verbose
	 * on, and its exact TLV-boundary bytes matter for the OTT_UWE/
	 * credit_capa investigation - cheap to always show.
	 */
	{
		size_t di;

		aprint_normal_dev(sc->sc_dev,
		    "GET_INFO raw reply (%zu bytes):", reply_len);
		for (di = 0; di < reply_len; di++)
			aprint_normal(" %02x", reply[di]);
		aprint_normal("\n");
	}

	info = (const struct uwe_sprdwl_cmd_fw_info *)reply;
	sc->sc_fw_version = le32toh(info->fw_version_le);
	sc->sc_fw_std = le32toh(info->fw_std_le);
	sc->sc_fw_capa = le32toh(info->fw_capa_le);
	memcpy(sc->sc_fw_mac, info->mac_addr, sizeof(sc->sc_fw_mac));
	uwe5622_sdio_net80211_sync_mac(sc);

	/*
	 * vendor cmdevt.c sprdwl_get_fw_info(): credit_capa (0=TX_WITH_CREDIT,
	 * 1=TX_NO_CREDIT) is the field right after mac_addr - decides whether
	 * the TX descriptor's color_bit field participates in a real
	 * per-queue flow-control scheme (sprdwl_fc_set_clor_bit()) or is
	 * always 0. A GET_INFO_TLV_TP_OTT(=1) TLV may follow at
	 * sizeof(*info) reporting whether THIS firmware build actually
	 * expects the OTT_UWE 3-byte descriptor prefix at runtime, since
	 * that's a host driver compile-time flag, not necessarily proof the
	 * firmware blob agrees.
	 */
	sc->sc_credit_capa = info->credit_capa;
	sc->sc_ott_supt = 0xff; /* not reported / unknown */
	{
		/*
		 * The TLV section is 4-byte aligned after the fixed struct,
		 * not immediately adjacent - confirmed by hand-decoding a
		 * real captured 89-byte reply: byte 82=credit_capa,
		 * byte 83=0x00 padding, byte 84..87=TLV header
		 * (type=1,len=1 LE), byte 88=value(0=OTT_NO_SUPT). Reading
		 * from sizeof(*info)=83 directly (no rounding) misparses the
		 * padding byte as part of the TLV type field.
		 */
		size_t tlv_off = roundup2(sizeof(*info), 4);
		const uint8_t *tlv = reply + tlv_off;
		size_t tlv_room = (reply_len > tlv_off) ? reply_len - tlv_off : 0;

		while (tlv_room >= 4) {
			uint16_t ttype = tlv[0] | ((uint16_t)tlv[1] << 8);
			uint16_t tlen = tlv[2] | ((uint16_t)tlv[3] << 8);

			if (tlv_room < (size_t)4 + tlen)
				break;
			if (ttype == 1 && tlen == 1)
				sc->sc_ott_supt = tlv[4];
			tlv += 4 + tlen;
			tlv_room -= 4 + tlen;
		}
	}

	aprint_normal_dev(sc->sc_dev,
	    "WIFI_CMD_GET_INFO ok ctx=%u chip_model=0x%08x chip_version=0x%08x "
	    "credit_capa=%u(%s) ott_supt=%u reply_len=%zu\n",
	    rctx, le32toh(info->chip_model_le), le32toh(info->chip_version_le),
	    sc->sc_credit_capa, sc->sc_credit_capa == 0 ? "WITH_CREDIT" :
	    "NO_CREDIT", sc->sc_ott_supt, reply_len);
	uwe5622_sdio_dump_fw_info(sc);
	return 0;
}

int
uwe5622_sdio_wifi_cmd_scan(struct uwe5622_sdio_softc *sc, uint8_t ctx_id,
    uint32_t channels_mask)
{
	uint8_t payload[sizeof(struct uwe_sprdwl_cmd_scan) +
	    sizeof(struct uwe_sprdwl_5g_tail) +
	    __arraycount(uwe_5ghz_channels) * sizeof(uint16_t)];
	struct uwe_sprdwl_cmd_scan *scan;
	struct uwe_sprdwl_5g_tail *tail5g;
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len, i;
	uint8_t rctx;
	int8_t status;
	int err, elapsed;

	memset(payload, 0, sizeof(payload));
	scan = (struct uwe_sprdwl_cmd_scan *)payload;
	scan->channels_le = htole32(channels_mask);
	scan->reserved_le = htole32(0);
	scan->ssid_len_le = htole16(0);

	tail5g = (struct uwe_sprdwl_5g_tail *)(payload +
	    sizeof(struct uwe_sprdwl_cmd_scan));
	tail5g->n_5g_chn_le = htole16(__arraycount(uwe_5ghz_channels));
	for (i = 0; i < __arraycount(uwe_5ghz_channels); i++)
		tail5g->chns_le[i] = htole16(uwe_5ghz_channels[i]);

	sc->sc_wifi_scan_done = false;
	sc->sc_wifi_scan_aborted = false;
	reply_len = sizeof(reply);
	memset(reply, 0, sizeof(reply));

	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_SCAN, ctx_id,
	    payload, sizeof(payload), true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_SCAN,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;

	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_SCAN immediate reply status=%d\n", status);
		return EIO;
	}

	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "WIFI_CMD_SCAN accepted ctx=%u, waiting for scan-done event\n",
		    rctx);

	for (elapsed = 0;
	     elapsed < UWE_WIFI_SCAN_TIMEOUT_MS;
	     elapsed += UWE_AT_POLL_DELAY_US / 1000) {
		err = uwe5622_sdio_rx_consume_once(sc, NULL);
		if (err)
			return err;
		if (sc->sc_wifi_scan_done)
			break;
		delay(UWE_AT_POLL_DELAY_US);
	}

	if (!sc->sc_wifi_scan_done) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_SCAN no scan-done event within timeout\n");
		return ETIMEDOUT;
	}

	if (sc->sc_verbose || sc->sc_wifi_scan_aborted)
		aprint_normal_dev(sc->sc_dev,
		    "WIFI_CMD_SCAN completed aborted=%s\n",
		    sc->sc_wifi_scan_aborted ? "yes" : "no");

	return sc->sc_wifi_scan_aborted ? EIO : 0;
}

void
uwe5622_sdio_ssid_lookup_cb(void *arg, struct ieee80211_node *ni)
{
	struct uwe5622_ssid_lookup *lk = arg;
	struct ieee80211com *ic = &lk->sc->sc_ic;
	ptrdiff_t idx;

	if (ni->ni_esslen != lk->ssid_len)
		return;
	if (memcmp(ni->ni_essid, lk->ssid, lk->ssid_len) != 0)
		return;
	if (ni->ni_chan == NULL || ni->ni_chan == IEEE80211_CHAN_ANYC)
		return;

	idx = ni->ni_chan - ic->ic_channels;
	if (idx <= 0 || idx > IEEE80211_CHAN_MAX)
		return;

	/*
	 * Same SSID can be broadcast by several BSSIDs (mesh/repeater
	 * setups - "sincap" showed 5 distinct APs on real hardware). Track
	 * the strongest signal seen so far rather than just the first match.
	 */
	if (lk->found && ni->ni_rssi <= lk->best_rssi)
		return;

	IEEE80211_ADDR_COPY(lk->bssid, ni->ni_bssid);
	lk->chan = (uint8_t)idx;
	lk->best_rssi = ni->ni_rssi;
	lk->found = true;
}

int
uwe5622_sdio_wifi_cmd_connect(struct uwe5622_sdio_softc *sc, uint8_t ctx_id,
    const uint8_t *bssid, uint8_t channel, const char *ssid,
    const char *passphrase)
{
	static const uint8_t wpa2_psk_ccmp_rsn_ie[] = {
		0x30, 0x14,             /* RSN element, 20-byte body */
		0x01, 0x00,             /* RSN version 1 */
		0x00, 0x0f, 0xac, 0x04, /* group cipher: CCMP */
		0x01, 0x00,             /* one pairwise cipher */
		0x00, 0x0f, 0xac, 0x04, /* pairwise cipher: CCMP */
		0x01, 0x00,             /* one AKM suite */
		0x00, 0x0f, 0xac, 0x02, /* AKM: PSK */
		0x00, 0x00              /* RSN capabilities: no PMF */
	};
	struct uwe_sprdwl_cmd_connect conn;
	uint8_t set_ie_buf[sizeof(struct uwe_sprdwl_cmd_set_ie) +
	    sizeof(wpa2_psk_ccmp_rsn_ie)];
	struct uwe_sprdwl_cmd_set_ie *set_ie;
	uint8_t psk[32];
	uint8_t reply[UWE_WIFI_MAX_REPLY];
	size_t reply_len, ssid_len;
	uint8_t rctx;
	int8_t status;
	int err, elapsed;

	size_t psk_len;
	uint8_t resolved_bssid[UWE_WIFI_MAC_LEN];
	const uint8_t *use_bssid;
	uint8_t use_channel;
	bool protected;

	ssid_len = strlen(ssid);
	if (ssid_len > UWE_IEEE80211_MAX_SSID_LEN)
		ssid_len = UWE_IEEE80211_MAX_SSID_LEN;

	/*
	 * CONNECT with channel=0/bssid=any relies on firmware to pick a
	 * channel itself; observed behavior is it always lands on channel 1
	 * regardless of which SSID was requested, so if the real AP isn't on
	 * channel 1 the association frames go out on the wrong channel and
	 * the AP never sees them (matches "CONNECT event success" locally
	 * while the AP's client list stays empty). Resolve the target SSID's
	 * real channel/BSSID from a prior (or freshly triggered) scan instead
	 * of connecting blind.
	 */
	use_bssid = bssid;
	use_channel = channel;
	if (use_bssid == NULL && use_channel == 0 && sc->sc_net80211_attached) {
		struct uwe5622_ssid_lookup lk;

		memset(&lk, 0, sizeof(lk));
		lk.sc = sc;
		lk.ssid = ssid;
		lk.ssid_len = ssid_len;
		ieee80211_iterate_nodes(&sc->sc_ic.ic_scan,
		    uwe5622_sdio_ssid_lookup_cb, &lk);

		if (!lk.found) {
			aprint_normal_dev(sc->sc_dev,
			    "CONNECT: no scan result for ssid='%s' yet, "
			    "scanning first\n", ssid);
			(void)uwe5622_sdio_bridge_scan_request(sc);
			ieee80211_iterate_nodes(&sc->sc_ic.ic_scan,
			    uwe5622_sdio_ssid_lookup_cb, &lk);
		}

		if (lk.found) {
			memcpy(resolved_bssid, lk.bssid, UWE_WIFI_MAC_LEN);
			use_bssid = resolved_bssid;
			use_channel = lk.chan;
			aprint_normal_dev(sc->sc_dev,
			    "CONNECT: resolved ssid='%s' to bssid=%s "
			    "chan=%u from scan\n", ssid,
			    ether_sprintf(resolved_bssid), use_channel);
		} else {
			aprint_error_dev(sc->sc_dev,
			    "CONNECT: ssid='%s' not found even after scan, "
			    "connecting blind (chan=0/bssid=any)\n", ssid);
		}
	}

	/*
	 * cfg80211.c's sprdwl_cfg80211_connect() does NOT derive a PMK -
	 * it copies sme->key (the raw ASCII passphrase from wpa_supplicant,
	 * logged with "%s") straight into con.psk with psk_len=strlen. The
	 * PBKDF2-HMAC-SHA1 PMK derivation previously done here was based on
	 * a wrong assumption that firmware wants a pre-derived 256-bit PMK;
	 * firmware does the PSK->PMK conversion itself. Sending our derived
	 * PMK bytes instead of the passphrase made every 4-way handshake
	 * fail with a MIC mismatch (AP never lists the client), even though
	 * firmware's own CONNECT command flow "succeeded" locally.
	 */
	psk_len = strlen(passphrase);
	if (psk_len > sizeof(psk))
		psk_len = sizeof(psk);
	memset(psk, 0, sizeof(psk));
	memcpy(psk, passphrase, psk_len);
	protected = psk_len != 0 ||
	    (sc->sc_net80211_attached &&
	    (sc->sc_ic.ic_flags & IEEE80211_F_WPA2) != 0);

	sc->sc_wifi_connect_chan = use_channel;
	memset(sc->sc_wifi_connect_bssid, 0,
	    sizeof(sc->sc_wifi_connect_bssid));
	if (use_bssid != NULL)
		memcpy(sc->sc_wifi_connect_bssid, use_bssid,
		    sizeof(sc->sc_wifi_connect_bssid));

	memset(&conn, 0, sizeof(conn));
	conn.auth_type = UWE_SPRDWL_AUTH_OPEN;
	if (use_bssid != NULL)
		memcpy(conn.bssid, use_bssid, UWE_WIFI_MAC_LEN);
	conn.channel = use_channel;
	if (!protected) {
		/*
		 * DIAGNOSTIC: connecting to an unsecured (open) network -
		 * isolating whether the persistent "Invalid RSN Capabilities"
		 * / "Unspecified failure" CONNECT rejections are specific to
		 * WPA2/RSN negotiation, or a more fundamental problem with
		 * this driver's whole association flow that would fail
		 * against ANY AP. wpa_versions=0 and NONE ciphers/AKM (no
		 * RSN IE at all for a real open network) - VALID_CONFIG
		 * intentionally omitted since there's nothing to configure.
		 */
		conn.wpa_versions_le = 0;
		conn.pairwise_cipher = UWE_SPRDWL_CIPHER_NONE;
		conn.group_cipher = UWE_SPRDWL_CIPHER_NONE;
		conn.key_mgmt = UWE_SPRDWL_AKM_SUITE_NONE;
	} else {
		conn.wpa_versions_le = htole32(UWE_WPA_VERSION_2);
		conn.pairwise_cipher =
		    UWE_SPRDWL_CIPHER_CCMP | UWE_SPRDWL_VALID_CONFIG;
		conn.group_cipher =
		    UWE_SPRDWL_CIPHER_CCMP | UWE_SPRDWL_VALID_CONFIG;
		conn.key_mgmt =
		    UWE_SPRDWL_AKM_SUITE_PSK | UWE_SPRDWL_VALID_CONFIG;
	}
	/*
	 * Tried mfp_enable=1 as an experiment (theory: AP wants PMF) -
	 * still got assoc_resp_status=45 "Invalid RSN Capabilities" (on a
	 * different AP instance of the same SSID), so it doesn't move the
	 * needle. Reverted to 0, matching vendor's normal sme->mfp passthrough
	 * for a non-PMF-required network (confirmed this AP is plain WPA2,
	 * not WPA2+WPA3 mixed, via its Fritz!Box config page).
	 */
	conn.mfp_enable = 0;
	conn.psk_len = (uint8_t)psk_len;
	conn.ssid_len = (uint8_t)ssid_len;
	memcpy(conn.psk, psk, sizeof(psk));
	memcpy(conn.ssid, ssid, ssid_len);

	/*
	 * Vendor sprdwl_cfg80211_connect() sends sme->ie as the association
	 * request IE immediately before WIFI_CMD_CONNECT.  The crypto fields
	 * above do not replace this step: without an RSN IE the AP can reject
	 * association before a STA LUT is created.  Our sysctl path has no
	 * wpa_supplicant-generated IE, so provide the exact WPA2-PSK/CCMP IE
	 * matching the CONNECT fields.
	 */
	if (protected) {
		set_ie = (struct uwe_sprdwl_cmd_set_ie *)set_ie_buf;
		set_ie->type = UWE_SPRDWL_IE_ASSOC_REQ;
		set_ie->len_le = htole16(sizeof(wpa2_psk_ccmp_rsn_ie));
		memcpy(set_ie->data, wpa2_psk_ccmp_rsn_ie,
		    sizeof(wpa2_psk_ccmp_rsn_ie));

		reply_len = sizeof(reply);
		memset(reply, 0, sizeof(reply));
		err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_SET_IE,
		    ctx_id, set_ie_buf, sizeof(set_ie_buf), true);
		if (err)
			return err;
		err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_SET_IE,
		    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
		if (err)
			return err;
		if (status != 0) {
			aprint_error_dev(sc->sc_dev,
			    "WIFI_CMD_SET_IE immediate reply status=%d\n", status);
			return EIO;
		}
		aprint_normal_dev(sc->sc_dev,
		    "WIFI_CMD_SET_IE: WPA2-PSK/CCMP association RSN IE installed\n");
	}

	reply_len = sizeof(reply);
	memset(reply, 0, sizeof(reply));

	aprint_normal_dev(sc->sc_dev,
	    "WIFI_CMD_CONNECT: ssid='%s' chan=%u bssid=%s\n",
	    ssid, use_channel,
	    use_bssid != NULL ? ether_sprintf(use_bssid) : "any");

	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_CONNECT, ctx_id,
	    &conn, sizeof(conn), true);
	if (err)
		return err;

	err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_CONNECT,
	    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err)
		return err;

	if (status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "WIFI_CMD_CONNECT immediate reply status=%d\n", status);
		return EIO;
	}

	aprint_normal_dev(sc->sc_dev,
	    "WIFI_CMD_CONNECT accepted ctx=%u, waiting for connect event\n",
	    rctx);

	sc->sc_wifi_connect_done = false;
	for (elapsed = 0;
	     elapsed < UWE_WIFI_CONNECT_DRAIN_MS;
	     elapsed += UWE_AT_POLL_DELAY_US / 1000) {
		(void)uwe5622_sdio_rx_consume_once(sc, NULL);
		if (sc->sc_wifi_connect_done)
			break;
		delay(UWE_AT_POLL_DELAY_US);
	}

	return 0;
}

int
uwe5622_sdio_send_wifi_cmd(struct uwe5622_sdio_softc *sc, uint8_t cmd_id,
    uint8_t ctx_id, const void *payload, size_t payload_len, bool need_rsp)
{
	uint8_t *buf;
	struct uwe_sprdwl_cmd_hdr *hdr;
	uint32_t puh;
	size_t total_len, aligned_len;
	int err;

	aligned_len = sizeof(*hdr) + payload_len;
	aligned_len = (aligned_len + 3) & ~3U;
	total_len = 4 + aligned_len + 4;

	if (total_len % 512 != 0)
		total_len = (total_len + 511) & ~511U;

	buf = kmem_zalloc(total_len, KM_SLEEP);

	//puh = uwe5622_sdio_pkt_build_raw(UWE_PKT_TYPE_CMD, 0, 0, aligned_len, 0);
	puh = uwe5622_sdio_pkt_build_raw(0, UWE_WIFI_CMD_TX_PUH_SUBTYPE,
	    0, aligned_len, 0);
	buf[0] = (puh >> 0) & 0xff;
	buf[1] = (puh >> 8) & 0xff;
	buf[2] = (puh >> 16) & 0xff;
	buf[3] = (puh >> 24) & 0xff;

	hdr = (struct uwe_sprdwl_cmd_hdr *)(buf + 4);
	hdr->common.raw = uwe5622_sdio_common_build(UWE_SPRDWL_TYPE_CMD, 0,
	    need_rsp ? UWE_SPRDWL_HEAD_RSP : UWE_SPRDWL_HEAD_NORSP, ctx_id);
	hdr->cmd_id = cmd_id;
	hdr->plen_le = htole16(sizeof(*hdr) + payload_len);
	hdr->mstime_le = htole32(0);
	hdr->status = 0;
	hdr->rsp_cnt = 0;
	hdr->reserv[0] = 0;
	hdr->reserv[1] = 0;

	if (payload_len != 0)
		memcpy(buf + 4 + sizeof(*hdr), payload, payload_len);

	buf[4 + aligned_len + 0] = 0x00;
	buf[4 + aligned_len + 1] = 0x00;
	buf[4 + aligned_len + 2] = 0x80;
	buf[4 + aligned_len + 3] = 0x00;

	/*
	 * Register interest BEFORE the write, not in
	 * uwe5622_sdio_wait_wifi_reply() - closes the window where a very
	 * fast reply could arrive before that function is even called.
	 * Fire-and-forget (need_rsp=false) sends must NOT touch either
	 * field: doing so could stomp a different, already-in-flight
	 * synchronous wait's registration (real hardware, 2026-08-26: a
	 * fire-and-forget BA(68) response sent from within RX-event
	 * dispatch, while a KEY(14) wait was outstanding on another
	 * thread, did exactly this and lost the KEY reply).
	 */
	if (need_rsp) {
		sc->sc_wifi_pending_cmd = cmd_id;
		sc->sc_wifi_reply_ready = false;
	}

	if (cmd_id != UWE_WIFI_CMD_SCAN || sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "wifi-cmd tx: cmd=%s(%u) ctx=%u paylen=%zu need_rsp=%s "
		    "puh_subtype=%u\n",
		    uwe5622_sdio_wifi_cmd_name(cmd_id), cmd_id, ctx_id,
		    payload_len, need_rsp ? "yes" : "no",
		    UWE_WIFI_CMD_TX_PUH_SUBTYPE);
	uwe5622_sdio_dump_bytes(sc, "wifi-cmd TX wire", buf, total_len);

	err = uwe5622_sdio_pkt_write(sc, buf, total_len);
	kmem_free(buf, total_len);

	return err;
}

void
uwe5622_sdio_run_cmd(struct uwe5622_sdio_softc *sc, const char *cmd)
{
	aprint_normal_dev(sc->sc_dev,
	    "safe stub: AT selftest disabled cmd='%s'\n", cmd != NULL ? cmd : "(null)");
}

void
uwe5622_sdio_run_wifi_cmd_matrix(struct uwe5622_sdio_softc *sc)
{
	aprint_normal_dev(sc->sc_dev,
	    "safe stub: wifi command matrix disabled\n");
}

int
uwe5622_at_cmd(struct uwe5622_sdio_softc *sc, const char *cmd,
    char *replybuf, size_t replybuf_len, int timeout_ms)
{
	return uwe5622_at_cmd_ex(sc, cmd, replybuf, replybuf_len, NULL, timeout_ms);
}

int
uwe5622_at_cmd_ex(struct uwe5622_sdio_softc *sc, const char *cmd,
    char *replybuf, size_t replybuf_len, uint8_t *reply_subtype,
    int timeout_ms)
{
	if (replybuf != NULL && replybuf_len > 0)
		replybuf[0] = '\0';
	if (reply_subtype != NULL)
		*reply_subtype = 0xff;
	aprint_normal_dev(sc->sc_dev,
	    "safe stub: AT cmd disabled cmd='%s' timeout=%d\n",
	    cmd != NULL ? cmd : "(null)", timeout_ms);
	return ENOTSUP;
}

void
uwe5622_sdio_try_cmd(struct uwe5622_sdio_softc *sc, const char *cmd)
{
	aprint_normal_dev(sc->sc_dev,
	    "safe stub: manual command disabled cmd='%s'\n",
	    cmd != NULL ? cmd : "(null)");
}

void
uwe5622_sdio_dump_fw_info(struct uwe5622_sdio_softc *sc)
{
	aprint_normal_dev(sc->sc_dev,
	    "fw-info: fw_ver=%u fw_std=0x%08x fw_capa=0x%08x "
	    "mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	    sc->sc_fw_version, sc->sc_fw_std, sc->sc_fw_capa,
	    sc->sc_fw_mac[0], sc->sc_fw_mac[1], sc->sc_fw_mac[2],
	    sc->sc_fw_mac[3], sc->sc_fw_mac[4], sc->sc_fw_mac[5]);
}

void
uwe5622_sdio_selftest(struct uwe5622_sdio_softc *sc)
{
	aprint_normal_dev(sc->sc_dev,
	    "safe stub: selftest disabled in this build\n");
}
