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


/*
 * ---- puh encode/decode ----
 */

uint32_t
uwe5622_sdio_pkt_build_raw(uint8_t type, uint8_t subtype, uint8_t eof,
    size_t len, uint8_t csum)
{
	uint32_t raw;

	raw = 0;
	raw |= ((uint32_t)(type & 0x0f) << 28);
	raw |= ((uint32_t)(subtype & 0x0f) << 24);
	raw |= ((uint32_t)(eof & 0x01) << 23);
	raw |= ((uint32_t)(len & 0xffff) << 7);
	raw |= ((uint32_t)(csum & 0x01) << 6);

	return raw;
}

void
uwe5622_sdio_pkt_decode(uint32_t raw, struct uwe_pkt_header *ph)
{
	ph->pad = (raw >> 0) & 0x3f;
	ph->csum = (raw >> 6) & 0x1;
	ph->len = (raw >> 7) & 0xffff;
	ph->eof = (raw >> 23) & 0x1;
	ph->subtype = (raw >> 24) & 0x0f;
	ph->type = (raw >> 28) & 0x0f;
}

int
uwe5622_sdio_enable_func_intr(struct uwe5622_sdio_softc *sc)
{
	uint8_t reg;
	struct sdmmc_function *sf0;

	sf0 = sc->sc_sf->sc->sc_fn0;

	reg = sdmmc_io_read_1(sf0, UWE_SDIO_CCCR_INTEN);
	reg |= (1 << sc->sc_sf->number);
	reg |= 0x01;

	sdmmc_io_write_1(sf0, UWE_SDIO_CCCR_INTEN, reg);

	reg = sdmmc_io_read_1(sf0, UWE_SDIO_CCCR_INTEN);
	aprint_normal_dev(sc->sc_dev,
	    "function interrupt enabled INTEN=0x%02x\n", reg);

	sc->sc_intr_enabled = true;
	return 0;
}

/*
 * SDIO card-interrupt handler, called from sdmmc's interrupt task context
 * (itself invoked from sunxi_mmc's real hardware IRQ handler once the
 * SUNXI_MMC_INT_SDIO_INT bit is seen). Deliberately does no SDIO bus I/O
 * here - sdmmc_intr_task() runs handlers back-to-back under its own lock,
 * and any real read/write needs sc_bus_lock plus the possibility of
 * sleeping in sunxi_mmc's command-completion wait, neither of which
 * belongs in this context. Just wake the RX kthread; it does the actual
 * pkt_read() using its normal locking.
 */
int
uwe5622_sdio_intr(void *arg)
{
	struct uwe5622_sdio_softc *sc = arg;

	mutex_enter(&sc->sc_rx_cv_lock);
	cv_signal(&sc->sc_rx_cv);
	mutex_exit(&sc->sc_rx_cv_lock);

	return 1;
}

/*
 * Callout fired when the activity LED hasn't been touched for
 * UWE_LED_DECAY_MS - turns it back off.
 */
void
uwe5622_sdio_led_timeout(void *arg)
{
	struct uwe5622_sdio_softc *sc = arg;

	sc->sc_led_on = false;
	led_set_by_name(UWE_LED_NAME, LED_STATE_OFF);
}

/*
 * Called (from thread/kthread context only - never hard interrupt
 * context) on TX send and RX dispatch to indicate WiFi activity on the
 * board's "led-1" (green, STATUS) LED. Turns the LED on if it isn't
 * already (sc_led_on is just a benign-race hint to skip the redundant
 * led_set_by_name() call and its internal lock on every packet - a
 * missed or duplicate transition here is harmless) and always pushes
 * the off-callout back by UWE_LED_DECAY_MS, so the LED stays lit
 * continuously under sustained traffic and decays to off shortly after
 * traffic stops instead of blinking per-packet.
 */
void
uwe5622_sdio_led_touch(struct uwe5622_sdio_softc *sc)
{

	if (!sc->sc_led_on) {
		sc->sc_led_on = true;
		led_set_by_name(UWE_LED_NAME, LED_STATE_ON);
	}
	callout_schedule(&sc->sc_led_co, mstohz(UWE_LED_DECAY_MS));
}

int
uwe5622_sdio_pkt_read(struct uwe5622_sdio_softc *sc, void *buf, size_t len)
{
	int err;

	/*
	 * sdmmc_io_{read,write}_multi_1() explicitly don't lock (see their
	 * comments in sdmmc_io.c) - the SDIO bus is a single shared
	 * resource and callers must serialize their own access. With both
	 * the TX workqueue and the dedicated RX poll kthread now touching
	 * the bus concurrently (plus the synchronous command wait-loops),
	 * this lock is required to avoid corrupting the wire protocol.
	 */
	mutex_enter(&sc->sc_bus_lock);
	err = sdmmc_io_read_multi_1(sc->sc_sf, UWE_SDIO_PK_MODE_ADDR, buf, len);
	mutex_exit(&sc->sc_bus_lock);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "PK fifo read failed len=%zu err=%d\n", len, err);
		return err;
	}
	return 0;
}

int
uwe5622_sdio_pkt_write(struct uwe5622_sdio_softc *sc, const void *buf, size_t len)
{
	int err;

	if ((len & 3) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "PK fifo write length not 4-byte aligned: %zu\n", len);
		return EINVAL;
	}

	mutex_enter(&sc->sc_bus_lock);
	err = sdmmc_io_write_multi_1(sc->sc_sf, UWE_SDIO_PK_MODE_ADDR,
	    __UNCONST(buf), len);
	mutex_exit(&sc->sc_bus_lock);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "PK fifo write failed len=%zu err=%d\n", len, err);
		return err;
	}
	return 0;
}

void __unused
uwe5622_sdio_decode_puh(struct uwe5622_sdio_softc *sc, uint32_t v,
    const char *tag)
{
	struct uwe_pkt_header ph;

	uwe5622_sdio_pkt_decode(v, &ph);
	aprint_normal_dev(sc->sc_dev,
	    "%s decode: raw=0x%08x type=0x%x subtype=0x%x eof=%u len=%u csum=%u pad=0x%x\n",
	    tag, v, ph.type, ph.subtype, ph.eof, ph.len,
	    ph.csum, ph.pad);
}
