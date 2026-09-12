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


void
uwe5622_sdio_if_start(struct ifnet *ifp)
{
	struct uwe5622_sdio_softc *sc;

	sc = ifp->if_softc;
	atomic_inc_32(&sc->sc_tx_start_calls);
	aprint_debug_dev(sc->sc_dev,
	    "if_start: qlen=%d flags=0x%x timer=%d\n",
	    IFQ_IS_EMPTY(&ifp->if_snd) ? 0 : 1, ifp->if_flags, ifp->if_timer);

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	if (!sc->sc_net80211_attached ||
	    sc->sc_ic.ic_state != IEEE80211_S_RUN) {
		/* Not associated yet: nothing to send it to. */
		IFQ_PURGE(&ifp->if_snd);
		return;
	}

	/*
	 * if_start() can run from softint context (e.g. ARP's DAD timer),
	 * but the actual SDIO transfer (sunxi_mmc_exec_command) blocks on
	 * a condvar waiting for hardware completion, which panics if
	 * attempted from softint. Defer the real work to a workqueue,
	 * which runs its callback in normal, sleepable kthread context.
	 * The atomic_cas here ensures only one work item is ever pending
	 * regardless of how many times if_start is called concurrently.
	 */
	if (sc->sc_tx_wq != NULL) {
		if (atomic_cas_uint(&sc->sc_tx_pending, 0, 1) == 0) {
			atomic_inc_32(&sc->sc_tx_schedule_claims);
			workqueue_enqueue(sc->sc_tx_wq, &sc->sc_tx_work, NULL);
		} else {
			atomic_inc_32(&sc->sc_tx_schedule_busy);
		}
	}
}

void
uwe5622_sdio_tx_work(struct work *wk, void *arg)
{
	struct uwe5622_sdio_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;
	bool queue_empty;
	int error;

	atomic_inc_32(&sc->sc_tx_worker_runs);

	for (;;) {
		while (true) {
			IFQ_DEQUEUE(&ifp->if_snd, m);
			if (m == NULL)
				break;

			atomic_inc_32(&sc->sc_tx_dequeued);
			bpf_mtap(ifp, m, BPF_D_OUT);

			error = uwe5622_sdio_tx_data(sc, m);
			if (error == EAGAIN) {
				/* Preserve ordering and ownership: firmware credit is a
				 * temporary backpressure condition, not a packet error. */
				IFQ_LOCK(&ifp->if_snd);
				IF_PREPEND(&ifp->if_snd, m);
				IFQ_UNLOCK(&ifp->if_snd);
				atomic_inc_32(&sc->sc_tx_no_credit);
				atomic_inc_32(&sc->sc_tx_credit_waits);
				sc->sc_tx_waiting_credit = true;
				atomic_swap_uint(&sc->sc_tx_pending, 0);
				/* Close the race with a FLOWCON arriving just before the
				 * pending flag was cleared. */
				if ((sc->sc_tx_credit[0] != 0 ||
				     sc->sc_tx_credit[1] != 0 ||
				     sc->sc_tx_credit[2] != 0 ||
				     sc->sc_tx_credit[3] != 0) &&
				    atomic_cas_uint(&sc->sc_tx_pending, 0, 1) == 0) {
					sc->sc_tx_waiting_credit = false;
					atomic_inc_32(&sc->sc_tx_schedule_claims);
					workqueue_enqueue(sc->sc_tx_wq,
					    &sc->sc_tx_work, NULL);
				}
				return;
			}
			if (error != 0) {
				atomic_inc_32(&sc->sc_tx_error);
				if (error == ENOTCONN)
					atomic_inc_32(&sc->sc_tx_no_lut);
				if_statinc(ifp, if_oerrors);
			} else {
				atomic_inc_32(&sc->sc_tx_ok);
				if_statinc(ifp, if_opackets);
			}
			m_freem(m);
		}

		/*
		 * Keep pending set for the entire drain.  Clearing it at worker
		 * entry allowed if_start() to enqueue this same work item while it
		 * was already executing.  A workqueue may coalesce that enqueue,
		 * leaving pending stuck at 1; every later packet then remained on
		 * if_snd forever.  Clear only after observing an empty queue and
		 * recheck it to close the producer/consumer wakeup race.
		 */
		atomic_swap_uint(&sc->sc_tx_pending, 0);
		IFQ_LOCK(&ifp->if_snd);
		queue_empty = IFQ_IS_EMPTY(&ifp->if_snd);
		IFQ_UNLOCK(&ifp->if_snd);
		if (queue_empty)
			break;
		atomic_inc_32(&sc->sc_tx_rechecks);

		/*
		 * If a concurrent if_start() already claimed and queued the work,
		 * let that invocation handle it.  Otherwise retain ownership and
		 * drain packets that arrived just before pending was cleared.
		 */
		if (atomic_cas_uint(&sc->sc_tx_pending, 0, 1) != 0) {
			atomic_inc_32(&sc->sc_tx_lost_wakeup_suspect);
			break;
		}
	}
}

/*
 * This driver has no SDIO interrupt handler; RX is purely polled. A tight
 * dedicated kthread is needed (rather than piggybacking a poll onto TX,
 * which was tried and is far too infrequent/bursty to catch fast-arriving
 * replies before firmware overwrites its RX buffer) so that inbound data
 * is drained continuously regardless of outgoing traffic. All actual SDIO
 * bus access is serialized via sc_bus_lock inside pkt_read()/pkt_write(),
 * since this now runs concurrently with the TX workqueue and the
 * synchronous command wait-loops.
 */
void
uwe5622_sdio_rx_thread(void *arg)
{
	struct uwe5622_sdio_softc *sc = arg;
	uint8_t *buf;
	unsigned int empty_polls;
	bool had_data;

	buf = kmem_zalloc(UWE_PKT_RX_BUFSZ, KM_SLEEP);
	empty_polls = 0;
	while (!sc->sc_rx_stop) {
		had_data = false;
		(void)uwe5622_sdio_rx_consume_buf(sc, buf, &had_data);
		if (had_data) {
			/* Drain a populated firmware queue without sleeping. */
			empty_polls = 0;
		} else {
			int wait_ticks;

			if (empty_polls < UWE_RX_IDLE_BACKOFF_POLLS)
				empty_polls++;
			/*
			 * cv_timedwait() instead of a fixed delay(): a real
			 * SDIO card interrupt (uwe5622_sdio_intr()) signals
			 * this cv and we wake immediately, draining right
			 * away instead of waiting out the rest of the poll
			 * interval. The timeout is kept as a fallback safety
			 * net (covers platforms where sdmmc_intr_establish()
			 * returned NULL, or a missed/lost wakeup) - not yet
			 * removed pending a hardware A/B comparison of idle
			 * CPU with interrupts active.
			 */
			wait_ticks = mstohz(
			    empty_polls >= UWE_RX_IDLE_BACKOFF_POLLS ?
			    UWE_RX_QUIET_POLL_DELAY_US / 1000 :
			    UWE_RX_IDLE_POLL_DELAY_US / 1000);
			if (wait_ticks < 1)
				wait_ticks = 1;
			mutex_enter(&sc->sc_rx_cv_lock);
			(void)cv_timedwait(&sc->sc_rx_cv, &sc->sc_rx_cv_lock,
			    wait_ticks);
			mutex_exit(&sc->sc_rx_cv_lock);
		}
	}
	kmem_free(buf, UWE_PKT_RX_BUFSZ);
	kthread_exit(0);
}

void
uwe5622_sdio_if_watchdog(struct ifnet *ifp)
{
	struct uwe5622_sdio_softc *sc;

	sc = ifp->if_softc;
	aprint_debug_dev(sc->sc_dev, "net80211 watchdog fired: if_timer=%d\n",
	    ifp->if_timer);
	ieee80211_watchdog(&sc->sc_ic);
}

/*
 * Look up this driver's own ifnet's currently-assigned IPv4 address.
 * uwe5622_sdio_tx_data()'s NOTIFY_IP_ACQUIRED logic used to read the
 * SOURCE address field out of whatever IPv4 packet happened to be
 * passing through it instead of asking the interface what its real
 * address is - on real hardware (2026-08-26) this caused firmware to
 * be notified alternating between the real WiFi address and a foreign
 * one, almost certainly a same-subnet packet from an established
 * connection/cached route still using a DIFFERENT interface's (emac0)
 * old address, re-routed onto this interface once that other
 * interface went down. Querying the ifnet's own address list directly
 * avoids trusting packet content for something that isn't actually
 * about packet content at all.
 */
bool
uwe5622_sdio_get_own_ipv4(struct uwe5622_sdio_softc *sc, uint8_t addr[4])
{
	struct ifaddr *ifa;

	IFADDR_FOREACH(ifa, &sc->sc_if) {
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		memcpy(addr, &satosin(ifa->ifa_addr)->sin_addr, 4);
		return true;
	}
	return false;
}

int
uwe5622_sdio_tx_data(struct uwe5622_sdio_softc *sc, struct mbuf *m)
{
	uint8_t *buf;
	uint8_t *dscr;
	uint32_t puh;
	uint16_t etype;
	uint8_t tx_ctrl;
	uint8_t color;
	size_t mlen, wire_len, aligned_len, total_len, ott_len;
	int err;

	mlen = m->m_pkthdr.len;
	if (mlen == 0 || mlen > UWE_PKT_RX_BUFSZ)
		return EINVAL;

	/*
	 * vendor sprdwl_intf_fill_msdu_dscr(): ARP/TDLS/PREAUTH frames get
	 * tx_ctrl.sw_rate=1 ("to make sure ARP/TDLS/preauth can be tx ASAP") -
	 * queued head-first (sprdwl_queue_data_msg_buf()) instead of
	 * tail-appended. Without it ARP requests may sit behind other
	 * traffic or be handled differently by firmware; since ping can
	 * never succeed without ARP resolving first, this matters even
	 * though it's a secondary field.
	 */
	tx_ctrl = 0;
	etype = 0;
	if (mlen >= 14)
		m_copydata(m, 12, 2, &etype);
	if (ntohs(etype) == ETHERTYPE_ARP)
		tx_ctrl |= (1 << 2); /* sw_rate */

	/*
	 * The vendor driver sends WIFI_CMD_NOTIFY_IP_ACQUIRED with the raw
	 * four-byte IPv4 address after NETDEV_UP.  Address-management ioctls do
	 * not reliably reach this driver's ioctl method on NetBSD, so use any
	 * outgoing IPv4 packet as a cheap trigger to re-check this ifnet's own
	 * assigned address.  This function runs in the sleepable TX worker,
	 * making the synchronous command safe.  A failure is retried on a
	 * later IPv4 packet.
	 *
	 * Uses uwe5622_sdio_get_own_ipv4() (this ifnet's own address list),
	 * NOT the packet's own IP source field - a same-subnet route can
	 * carry a DIFFERENT interface's already-bound source address through
	 * this interface (real hardware, 2026-08-26: emac0's old address kept
	 * appearing here after emac0 was taken down), and packet content was
	 * never really what "IP acquired" should mean anyway.
	 *
	 * Originally a one-shot latch on "first nonzero source address ever
	 * seen" - on real hardware (2026-08-26, real router/mesh test) the
	 * very first outgoing IPv4 packet was a 169.254.x.x (IPv4LL/link-
	 * local) address from dhcpcd's own pre-lease probing, which got
	 * latched permanently, so firmware was told the wrong address even
	 * after the real DHCP lease arrived afterward. Now: skip
	 * 169.254.0.0/16 entirely (never worth notifying), and re-notify
	 * whenever the real address actually changes (covers the original
	 * "notify once" case as change-from-zero, plus DHCP renew/rebind or
	 * a roam onto a different subnet).
	 */
	if (ntohs(etype) == ETHERTYPE_IP) {
		uint8_t own_ipv4[4];

		if (uwe5622_sdio_get_own_ipv4(sc, own_ipv4) &&
		    (own_ipv4[0] | own_ipv4[1] | own_ipv4[2] | own_ipv4[3]) != 0 &&
		    !(own_ipv4[0] == 169 && own_ipv4[1] == 254) &&
		    memcmp(own_ipv4, sc->sc_notified_ipv4, 4) != 0) {
			int8_t status = -1;
			size_t reply_len = 0;

			atomic_inc_32(&sc->sc_ip_notify_attempts);
			err = uwe5622_sdio_send_wifi_cmd(sc,
			    UWE_WIFI_CMD_NOTIFY_IP_ACQUIRED, sc->sc_wifi_ctx_id,
			    own_ipv4, 4, true);
			if (err == 0)
				err = uwe5622_sdio_wait_wifi_reply(sc,
				    UWE_WIFI_CMD_NOTIFY_IP_ACQUIRED, 1000,
				    NULL, &reply_len, NULL, &status);
			if (err == 0 && status == 0) {
				memcpy(sc->sc_notified_ipv4, own_ipv4, 4);
				sc->sc_ipv4_notified = true;
				atomic_inc_32(&sc->sc_ip_notify_ok);
				aprint_normal_dev(sc->sc_dev,
				    "firmware notified of IPv4 %u.%u.%u.%u\n",
				    own_ipv4[0], own_ipv4[1], own_ipv4[2], own_ipv4[3]);
			} else {
				atomic_inc_32(&sc->sc_ip_notify_error);
				aprint_error_dev(sc->sc_dev,
				    "IPv4 notify failed addr=%u.%u.%u.%u "
				    "err=%d status=%d\n",
				    own_ipv4[0], own_ipv4[1], own_ipv4[2], own_ipv4[3],
				    err, status);
			}
		}
	}

	/*
	 * Matches vendor sprdwl_intf_fill_msdu_dscr()'s own refusal
	 * ("sta disconn, no data tx!") when lut_index isn't valid yet -
	 * firmware assigns this via WIFI_EVENT_STA_LUT_INDEX right after a
	 * successful CONNECT (see uwe5622_sdio_handle_wifi_event_payload()).
	 */
	if (!sc->sc_sta_lut_valid)
		return ENOTCONN;

	if (sc->sc_fw_tx_asserted)
		return EIO;

	/*
	 * vendor sprdwl_fc_get_send_num(): host must not send more than
	 * firmware's advertised per-color credit budget before waiting for
	 * the next WIFI_EVENT_SDIO_FLOWCON top-up (see the FLOWCON handler
	 * in uwe5622_sdio_handle_wifi_event_payload()) - firmware's own
	 * SDIO host-interface ring can overflow otherwise. This driver only
	 * ever uses color 0. Decrement-if-positive via CAS loop since RX
	 * (kthread, replenishes) and TX (workqueue, consumes) run
	 * concurrently.
	 */
	/* Color 0 is the STA mode's exclusive pool; vendor code then borrows
	 * from every other pool as shared credit.  Descriptor color_bit must
	 * identify the pool actually consumed. */
	color = 4;
	for (uint8_t i = 0; i < 4 && color == 4; i++) {
		uint32_t cur;

		while ((cur = sc->sc_tx_credit[i]) != 0) {
			if (atomic_cas_32(&sc->sc_tx_credit[i], cur,
			    cur - 1) == cur) {
				color = i;
				break;
			}
		}
	}
	if (color == 4) {
		if (sc->sc_verbose)
			aprint_normal_dev(sc->sc_dev,
			    "data tx: waiting for firmware credit\n");
		return EAGAIN;
	}

	/*
	 * REPLACES the old 8-byte struct uwe_sprdwl_data_hdr + 2-byte
	 * SPRDWL_DATA_OFFSET scheme. Fetched sprdwl_intf_fill_msdu_dscr()
	 * (wl_intf.c, called unconditionally from sprdwl_send_data() for
	 * every data frame, not gated by any chip-variant #ifdef) - it
	 * builds an 11-byte "struct tx_msdu_dscr" (DSCR_LEN=11), not the
	 * simpler data_hdr this driver assumed. Also confirmed via wl_intf.h:
	 * SDIO_TX_DATA_PORT=10, distinct from SDIO_TX_CMD_PORT=8 (which
	 * exactly matches this driver's own already-working
	 * UWE_WIFI_CMD_TX_PUH_SUBTYPE=8) - outgoing DATA frames need PUH
	 * subtype=10, not 8. This likely explains the earlier SDIO bus fault
	 * when outer PUH type was changed to 0 while subtype stayed at 8:
	 * that combination (type=0/subtype=8) is literally the CMD-TX
	 * channel identifier, so firmware was handed a giant Ethernet frame
	 * and tried to parse it as a malformed command.
	 */
	/*
	 * vendor wl_intf.c/tx_msg.c consistently place the descriptor at
	 * tran_data + FOUR_BYTES_ALIGN_OFFSET (=3) whenever OTT_UWE is
	 * defined - and unisocwifi/Makefile defines it unconditionally
	 * (ccflags-y += -DOTT_UWE, not gated by any chip #ifdef) for the
	 * exact obj-$(CONFIG_WLAN_UWE5622) module this chip uses. HOWEVER
	 * that's a HOST driver compile-time flag; whether the actual
	 * firmware blob running on the WCN chip agrees is a separate,
	 * runtime capability (GET_INFO's GET_INFO_TLV_TP_OTT TLV,
	 * sc_ott_supt: 0=OTT_NO_SUPT, 1=OTT_SUPT). Adding this 3-byte gap
	 * (previously always-on) produced a byte-for-byte format matching
	 * vendor's own documented example, yet firmware still asserted
	 * ("WCN Assert in transmit.c line 142") on the very first frame -
	 * and cmdevt.c's sprdwl_get_fw_info() forces ott_supt=OTT_NO_SUPT
	 * (no TLV even sent) whenever the firmware negotiates as the older
	 * "VERSION_1" compat tier, which this firmware's GET_INFO reply
	 * looks consistent with (no OTT TLV found in the 89-byte reply).
	 * So: only add the prefix when firmware has explicitly confirmed
	 * OTT_SUPT via that TLV - default to NOT adding it otherwise, the
	 * reverse of the previous always-on assumption.
	 */
	ott_len = (sc->sc_ott_supt == 1) ? UWE_TX_OTT_PREFIX_LEN : 0;
	wire_len = ott_len + UWE_TX_MSDU_DSCR_LEN + mlen;
	aligned_len = (wire_len + 3) & ~3U;
	total_len = 4 + aligned_len + 4;
	if (total_len % 512 != 0)
		total_len = (total_len + 511) & ~511U;

	/*
	 * if_start() can be invoked from softint context (e.g. ARP's DAD
	 * timer transmitting via ether_output()). kmem_zalloc() KASSERTs
	 * !cpu_softintr_p() unconditionally, regardless of KM_SLEEP vs
	 * KM_NOSLEEP - the interrupt-safe allocator is a distinct function.
	 */
	buf = kmem_intr_zalloc(total_len, KM_NOSLEEP);
	if (buf == NULL) {
		atomic_inc_32(&sc->sc_tx_credit[color]);
		return ENOMEM;
	}

	/*
	 * PUH.len must be the RAW (unaligned) length, not the 4-byte-padded
	 * space it occupies. Confirmed via sdiohal_tx_fill_puh() (unisocwcn/
	 * sdio/sdiohal_common.c): "puh->len = mbuf_node->len;" - the 4-byte
	 * rounding (SDIOHAL_ALIGN_4BYTE) is applied only afterwards, purely
	 * for the transport layer's own used_len/next-packet-offset
	 * bookkeeping, never stored into the length field itself.
	 */
	puh = uwe5622_sdio_pkt_build_raw(UWE_PKT_TYPE_CTRL,
	    UWE_WIFI_DATA_TX_PUH_SUBTYPE, 0, wire_len, 0);
	buf[0] = (puh >> 0) & 0xff;
	buf[1] = (puh >> 8) & 0xff;
	buf[2] = (puh >> 16) & 0xff;
	buf[3] = (puh >> 24) & 0xff;

	/*
	 * struct tx_msdu_dscr (wl_intf.h), built manually byte-by-byte
	 * (matching this file's existing convention for the PUH header)
	 * rather than via C bitfields, to avoid any compiler-dependent
	 * bitfield packing surprises for a struct we can't directly verify
	 * on this target:
	 *   byte 0: common = type:3 | direction_ind:1<<3 | need_rsp:1<<4 |
	 *           interface(ctx_id):3<<5
	 *   byte 1: offset = descriptor length (11)
	 *   byte 2: tx_ctrl = checksum_offload:1 | checksum_type:1<<1 |
	 *           sw_rate:1<<2 | wds:1<<3 | swq_flag:1<<4 | rsvd:1<<5 |
	 *           next_buffer_type:1<<6 | pcie_mh_readcomp:1<<7 - sw_rate
	 *           set for ARP (vendor: "to make sure ARP/TDLS/preauth can
	 *           be tx ASAP")
	 *   bytes 3-4: pkt_len, LE - payload length only (vendor:
	 *              skb->len - DSCR_LEN), i.e. just mlen here
	 *   byte 5: buffer_info = msdu_tid:4 | mac_data_offset:4<<4 - 0
	 *   byte 6: sta_lut_index
	 *   bytes 7-8: color_bit:2 | rsvd:14<<2, LE - 0
	 *   bytes 9-10: tcp_udp_header_offset, LE - 0 (no checksum offload)
	 *
	 * Preceded by ott_len bytes (0 or UWE_TX_OTT_PREFIX_LEN, left zeroed
	 * by kmem_intr_zalloc when nonzero) - see wire_len comment above.
	 */
	dscr = buf + 4 + ott_len;
	dscr[0] = (UWE_SPRDWL_TYPE_DATA & 0x7) |
	    ((sc->sc_wifi_ctx_id & 0x7) << 5);
	dscr[1] = UWE_TX_MSDU_DSCR_LEN;
	dscr[2] = tx_ctrl;
	dscr[3] = mlen & 0xff;
	dscr[4] = (mlen >> 8) & 0xff;
	dscr[5] = 0;
	dscr[6] = sc->sc_sta_lut_index;
	dscr[7] = color & 0x03;
	dscr[8] = 0;
	dscr[9] = 0;
	dscr[10] = 0;

	m_copydata(m, 0, mlen, buf + 4 + ott_len + UWE_TX_MSDU_DSCR_LEN);

	buf[4 + aligned_len + 0] = 0x00;
	buf[4 + aligned_len + 1] = 0x00;
	buf[4 + aligned_len + 2] = 0x80;
	buf[4 + aligned_len + 3] = 0x00;

	err = uwe5622_sdio_pkt_write(sc, buf, total_len);
	if (err != 0)
		atomic_inc_32(&sc->sc_tx_credit[color]);
	else {
		atomic_inc_32(&sc->sc_tx_color_used[color]);
		uwe5622_sdio_led_touch(sc);
	}
	if (sc->sc_verbose) {
		aprint_normal_dev(sc->sc_dev,
		    "data tx: mlen=%zu wire_len=%zu ott_len=%zu total_len=%zu "
		    "lut=%u color=%u sw_rate=%u credit=%u/%u/%u/%u err=%d\n",
		    mlen, wire_len, ott_len, total_len, sc->sc_sta_lut_index,
		    color, (tx_ctrl >> 2) & 1, sc->sc_tx_credit[0],
		    sc->sc_tx_credit[1], sc->sc_tx_credit[2],
		    sc->sc_tx_credit[3], err);
		if (tx_ctrl & (1 << 2)) {
			size_t dlen, di, eth_off;

			dlen = 4 + wire_len;
			if (dlen > 80)
				dlen = 80;
			aprint_normal_dev(sc->sc_dev,
			    "data tx wire (ARP, %zu bytes):", dlen);
			for (di = 0; di < dlen; di++)
				aprint_normal(" %02x", buf[di]);
			aprint_normal("\n");

			/*
			 * Decode sender/target IP explicitly instead of
			 * requiring the reader to hand-count hex bytes (a
			 * repeated source of real mistakes this session,
			 * e.g. misreading the assert-message offsets) -
			 * standard ARP layout: eth[14] + hwtype(2)+ptype(2)+
			 * hlen(1)+plen(1)+op(2)+sha(6)+spa(4)+tha(6)+tpa(4).
			 */
			eth_off = 4 + ott_len + UWE_TX_MSDU_DSCR_LEN;
			if (eth_off + 38 + 4 <= total_len) {
				const uint8_t *spa = buf + eth_off + 28;
				const uint8_t *tpa = buf + eth_off + 38;

				aprint_normal_dev(sc->sc_dev,
				    "data tx ARP: sender_ip=%u.%u.%u.%u "
				    "target_ip=%u.%u.%u.%u\n",
				    spa[0], spa[1], spa[2], spa[3],
				    tpa[0], tpa[1], tpa[2], tpa[3]);
			}
		}
	}
	kmem_intr_free(buf, total_len);

	return err;
}

void
uwe5622_sdio_handle_data_payload(struct uwe5622_sdio_softc *sc,
    const uint8_t *payload, size_t len)
{
	struct mbuf *m;
	uint32_t desc0;
	uint8_t msdu_offset;
	uint16_t msdu_len;

	if (len < 4)
		return;

	desc0 = ((uint32_t)payload[0] << 0) | ((uint32_t)payload[1] << 8) |
	    ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
	msdu_offset = (desc0 >> 8) & 0xff;
	msdu_len = (desc0 >> 16) & 0xffff;

	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "data rx: pktlen=%zu msdu_offset=%u msdu_len=%u "
		    "ic_state=%d\n",
		    len, msdu_offset, msdu_len, sc->sc_ic.ic_state);

	if (msdu_len == 0 || (size_t)msdu_offset + msdu_len > len ||
	    msdu_len > MCLBYTES) {
		aprint_error_dev(sc->sc_dev,
		    "data rx: bad msdu desc offset=%u len=%u pktlen=%zu\n",
		    msdu_offset, msdu_len, len);
		return;
	}

	/* Make the WPA2 four-way handshake visible without noisy full dumps. */
	if (msdu_len >= 23 &&
	    payload[msdu_offset + 12] == 0x88 &&
	    payload[msdu_offset + 13] == 0x8e) {
		const uint8_t *e = payload + msdu_offset + 14;
		uint16_t body_len = be16dec(e + 2);
		uint16_t key_info = msdu_len >= 35 ? be16dec(e + 5) : 0;

		aprint_normal_dev(sc->sc_dev,
		    "EAPOL rx: src=%s dst=%s version=%u type=%u body_len=%u "
		    "desc=%u key_info=0x%04x\n",
		    ether_snprintf((char[18]){0}, 18,
		        payload + msdu_offset + 6),
		    ether_snprintf((char[18]){0}, 18,
		        payload + msdu_offset),
		    e[0], e[1], body_len, msdu_len >= 19 ? e[4] : 0,
		    key_info);
	}

	if (!sc->sc_net80211_attached ||
	    sc->sc_ic.ic_state != IEEE80211_S_RUN)
		return;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	if (msdu_len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return;
		}
	}
	memcpy(mtod(m, void *), payload + msdu_offset, msdu_len);
	m->m_len = m->m_pkthdr.len = msdu_len;
	m_set_rcvif(m, &sc->sc_if);

	uwe5622_sdio_led_touch(sc);
	if_percpuq_enqueue(sc->sc_if.if_percpuq, m);
}

int
uwe5622_sdio_rx_consume_buf(struct uwe5622_sdio_softc *sc, uint8_t *buf,
    bool *had_data)
{
	uint32_t valid_len;
	int err;

	if (had_data != NULL)
		*had_data = false;

	err = uwe5622_sdio_pkt_read(sc, buf, UWE_PKT_RX_BUFSZ);
	if (err)
		return err;

	valid_len =
	    ((uint32_t)buf[UWE_PKT_RX_BUFSZ - 8] << 0) |
	    ((uint32_t)buf[UWE_PKT_RX_BUFSZ - 7] << 8) |
	    ((uint32_t)buf[UWE_PKT_RX_BUFSZ - 6] << 16) |
	    ((uint32_t)buf[UWE_PKT_RX_BUFSZ - 5] << 24);

	if (valid_len != 0 && sc->sc_verbose) {
		aprint_normal_dev(sc->sc_dev,
		    "poll: valid_len=%u bufsz=%d trailer=%02x %02x %02x %02x "
		    "%02x %02x %02x %02x\n",
		    valid_len, UWE_PKT_RX_BUFSZ,
		    buf[UWE_PKT_RX_BUFSZ - 8], buf[UWE_PKT_RX_BUFSZ - 7],
		    buf[UWE_PKT_RX_BUFSZ - 6], buf[UWE_PKT_RX_BUFSZ - 5],
		    buf[UWE_PKT_RX_BUFSZ - 4], buf[UWE_PKT_RX_BUFSZ - 3],
		    buf[UWE_PKT_RX_BUFSZ - 2], buf[UWE_PKT_RX_BUFSZ - 1]);
	}

	if (valid_len != 0 && valid_len <= (UWE_PKT_RX_BUFSZ - 8)) {
		if (had_data != NULL)
			*had_data = true;
		uwe5622_sdio_rx_parse_frame(sc, buf, UWE_PKT_RX_BUFSZ - 8,
		    valid_len);
	}

	return 0;
}

int
uwe5622_sdio_rx_consume_once(struct uwe5622_sdio_softc *sc, bool *had_data)
{
	uint8_t *buf;
	int err;

	buf = kmem_zalloc(UWE_PKT_RX_BUFSZ, KM_SLEEP);
	err = uwe5622_sdio_rx_consume_buf(sc, buf, had_data);
	kmem_free(buf, UWE_PKT_RX_BUFSZ);
	return err;
}

void
uwe5622_sdio_rx_parse_frame(struct uwe5622_sdio_softc *sc,
    const uint8_t *buf, size_t buflen, uint32_t valid_len)
{
	size_t off;

	off = 0;
	while (off + 4 <= buflen && off < valid_len) {
		uint32_t raw;
		struct uwe_pkt_header ph;
		size_t payload_off, aligned_len, next_off;
		const uint8_t *payload;

		raw = ((uint32_t)buf[off + 0] << 0) |
		      ((uint32_t)buf[off + 1] << 8) |
		      ((uint32_t)buf[off + 2] << 16) |
		      ((uint32_t)buf[off + 3] << 24);

		uwe5622_sdio_pkt_decode(raw, &ph);

		if (ph.eof)
			break;
		if (ph.len == 0)
			break;

		payload_off = off + 4;
		aligned_len = (ph.len + 3) & ~3U;
		next_off = payload_off + aligned_len;
		if (next_off <= off || payload_off + ph.len > buflen)
			break;

		payload = buf + payload_off;

		uwe5622_sdio_pkt_stat_note(sc, ph.type, ph.subtype, ph.len);

		if (!(ph.type == UWE_PKT_TYPE_CTRL &&
		    ph.subtype == UWE_PKT_SUBTYPE_LOG)) {
			aprint_debug_dev(sc->sc_dev,
			    "rx trace: type=%u subtype=%u eof=%u len=%u\n",
			    ph.type, ph.subtype, ph.eof, ph.len);
		}

		if (ph.type == UWE_PKT_TYPE_CTRL &&
		    ph.subtype == UWE_PKT_SUBTYPE_LOG) {
			uwe5622_sdio_handle_log_payload(sc, payload, ph.len);
			goto next;
		}

		/* non-log packet: always dump for diagnostics */
		aprint_debug_dev(sc->sc_dev,
		    "RX non-log: puh=0x%08x type=%u subtype=%u len=%u\n",
		    raw, ph.type, ph.subtype, ph.len);
		uwe5622_sdio_dump_bytes(sc, "RX non-log payload", payload, ph.len);

		if (ph.type == UWE_PKT_TYPE_EVENT) {
			uwe5622_sdio_handle_wifi_event_payload(sc,
			    payload, ph.len);
			goto next;
		}

		if (ph.type == UWE_PKT_TYPE_DATA) {
			uwe5622_sdio_handle_data_payload(sc, payload, ph.len);
			goto next;
		}

		/*
		 * unisocwcn/sdio/sdiohal_common.c: RX channel = subtype + 12.
		 * wl_intf.c registers SDIO_RX_DATA_PORT=24 -> subtype 12, a
		 * DIFFERENT wire subtype than the CMD/EVENT wrapper (10) this
		 * driver has always assumed data would share. The "type"
		 * field doesn't participate in this channel routing at all
		 * per vendor source, so check subtype alone here regardless
		 * of ph.type.
		 */
		if (ph.subtype == UWE_WIFI_DATA_RX_PUH_SUBTYPE) {
			uwe5622_sdio_handle_data_payload(sc, payload, ph.len);
			goto next;
		}

		if (ph.type == 0 &&
			ph.subtype == UWE_WIFI_CMD_RX_PUH_SUBTYPE) {
			uint8_t inner_type = ph.len >= 1 ?
			    uwe5622_sdio_common_type(payload[0]) : 0xff;

			/*
			 * Every event/cmd-reply we've observed arrives with
			 * outer PUH type=0 (CTRL) - the real discrimination
			 * happens via the INNER common_hdr.type byte. Real
			 * inbound DATA frames very likely follow the exact
			 * same convention (never once seen an outer PUH
			 * type=UWE_PKT_TYPE_DATA packet on the wire despite
			 * a genuine over-the-air association completing) -
			 * check for that here too, alongside EVENT vs CMD
			 * reply, instead of only in the dead outer-type=2
			 * branch above.
			 */
			if (ph.len >= sizeof(struct uwe_sprdwl_data_hdr) &&
			    inner_type == UWE_SPRDWL_TYPE_DATA) {
				uwe5622_sdio_handle_data_payload(sc, payload, ph.len);
			} else if (ph.len >= sizeof(struct uwe_sprdwl_cmd_hdr) &&
				inner_type == UWE_SPRDWL_TYPE_EVENT) {
				uwe5622_sdio_handle_wifi_event_payload(sc, payload, ph.len);
			} else {
				uwe5622_sdio_handle_wifi_cmd_payload(sc, payload, ph.len);
			}
			goto next;
		}

		/*
		 * type=0 subtype=0 or subtype=1:
		 * shared between AT replies and sprdwl cmd responses.
		 *
		 * try sprdwl first: check embedded common header.
		 * log the heuristic decision so we can debug misrouting.
		 */
		if (ph.type == UWE_PKT_TYPE_CTRL &&
		    (ph.subtype == UWE_PKT_SUBTYPE_AT_REPLY ||
		     ph.subtype == UWE_PKT_SUBTYPE_AT_STATUS)) {
			bool is_sprdwl = false;

			if (ph.len >= sizeof(struct uwe_sprdwl_cmd_hdr)) {
				const struct uwe_sprdwl_cmd_hdr *wchdr;
				uint8_t ctype;
				uint16_t plen;

				wchdr = (const struct uwe_sprdwl_cmd_hdr *)payload;
				ctype = uwe5622_sdio_common_type(wchdr->common.raw);
				plen = le16toh(wchdr->plen_le);

				aprint_normal_dev(sc->sc_dev,
				    "heuristic: common.raw=0x%02x ctype=%u rsp=%u ctx=%u "
				    "cmd_id=%u plen=%u ph.len=%u\n",
				    wchdr->common.raw, ctype,
				    uwe5622_sdio_common_rsp(wchdr->common.raw),
				    uwe5622_sdio_common_ctx(wchdr->common.raw),
				    wchdr->cmd_id, plen, ph.len);

				if (ctype <= UWE_SPRDWL_TYPE_DATA &&
				    plen >= sizeof(*wchdr) &&
				    plen <= ph.len &&
				    (wchdr->cmd_id <= UWE_WIFI_CMD_SCHED_SCAN ||
				     wchdr->cmd_id >= UWE_WIFI_EVENT_CONNECT)) {
					is_sprdwl = true;
				}
			}

			if (is_sprdwl) {
				aprint_normal_dev(sc->sc_dev,
				    "heuristic: -> routed to sprdwl cmd handler\n");
				uwe5622_sdio_handle_wifi_cmd_payload(sc,
				    payload, ph.len);
			} else {
				aprint_normal_dev(sc->sc_dev,
				    "heuristic: -> routed to AT handler\n");
				uwe5622_sdio_handle_at_payload(sc,
				    ph.subtype, payload, ph.len);
			}
			goto next;
		}

		sc->sc_rx_unhandled_packets++;
		aprint_normal_dev(sc->sc_dev,
		    "unhandled RX packet type=%u subtype=%u len=%u\n",
		    ph.type, ph.subtype, ph.len);

		/*
		 * subtype=2 (logical SDIO channel 14, per
		 * sdiohal_hwtype_to_channel(): channel = subtype + 12) isn't
		 * any WiFi CMD/EVENT/DATA port we know of - real content
		 * turned out to be a WCN firmware debug/assert log string,
		 * readable at odd byte offsets (even offsets are noise/other
		 * framing, empirically confirmed - NOT an overlap with our
		 * own TX buffer, ruled out separately). Decode it plainly
		 * instead of just hex-dumping, and stop sending further DATA
		 * frames once a real assert is seen - the firmware is
		 * already wedged for this TX attempt and hammering it with
		 * more frames only pollutes the log.
		 */
		if (ph.type == 0 && ph.subtype == 2 && ph.len > 0) {
			char msg0[81], msg1[81], msg2[81];
			size_t dlen, di, mi, pi;

			/*
			 * Always dump raw bytes too (not just the decoded
			 * guess) - a prior decode of this packet turned out
			 * garbled on one test run with no way to cross-check
			 * it, since only the decoded string was logged then.
			 */
			dlen = ph.len;
			if (dlen > 160)
				dlen = 160;
			aprint_normal_dev(sc->sc_dev,
			    "subtype=2 raw (%zu bytes):", dlen);
			for (di = 0; di < dlen; di++)
				aprint_normal(" %02x", payload[di]);
			if (ph.len > dlen)
				aprint_normal(" ...");
			aprint_normal("\n");

			/*
			 * Framing of this debug-log packet is NOT consistent
			 * between messages: "Assert in transmit.c line 142"
			 * only read cleanly at stride 2 starting at byte 1,
			 * but a later "Assert in host_if_sdio.c line 58"
			 * read cleanly with NO skipping at all (plain
			 * sequential from byte 0) - stride2[0]/[1] both
			 * garbled it, so the assert-detection check silently
			 * missed it. Try all 3 candidate decodings and check
			 * "Assert" against each rather than assuming one
			 * fixed framing.
			 */
			mi = 0;
			for (pi = 0; pi < ph.len && mi < sizeof(msg0) - 1; pi += 2) {
				uint8_t c = payload[pi];
				msg0[mi++] = (c >= 0x20 && c < 0x7f) ? (char)c : '.';
			}
			msg0[mi] = '\0';
			mi = 0;
			for (pi = 1; pi < ph.len && mi < sizeof(msg1) - 1; pi += 2) {
				uint8_t c = payload[pi];
				msg1[mi++] = (c >= 0x20 && c < 0x7f) ? (char)c : '.';
			}
			msg1[mi] = '\0';
			mi = 0;
			for (pi = 0; pi < ph.len && mi < sizeof(msg2) - 1; pi++) {
				uint8_t c = payload[pi];
				msg2[mi++] = (c >= 0x20 && c < 0x7f) ? (char)c : '.';
			}
			msg2[mi] = '\0';
			aprint_normal_dev(sc->sc_dev,
			    "WCN firmware log stride2[0]: %s\n", msg0);
			aprint_normal_dev(sc->sc_dev,
			    "WCN firmware log stride2[1]: %s\n", msg1);
			aprint_normal_dev(sc->sc_dev,
			    "WCN firmware log stride1: %s\n", msg2);
			if ((strstr(msg0, "Assert") != NULL ||
			     strstr(msg1, "Assert") != NULL ||
			     strstr(msg2, "Assert") != NULL) &&
			    !sc->sc_fw_tx_asserted) {
				sc->sc_fw_tx_asserted = true;
				aprint_error_dev(sc->sc_dev,
				    "firmware asserted on TX - halting further "
				    "data tx until next connect\n");
			}
		} else if (sc->sc_verbose) {
			size_t dlen, di;

			dlen = ph.len;
			if (dlen > 160)
				dlen = 160;
			aprint_normal_dev(sc->sc_dev,
			    "unhandled RX payload (%zu bytes):", dlen);
			for (di = 0; di < dlen; di++)
				aprint_normal(" %02x", payload[di]);
			if (ph.len > dlen)
				aprint_normal(" ...");
			aprint_normal("\n");
		}

next:
		off = next_off;
	}
}
