/* $NetBSD: rtcsram.c,v 1.1.2.2 2024/02/03 11:47:04 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rtcsram.c,v 1.1.2.2 2024/02/03 11:47:04 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <dev/clock_subr.h>

#include <lib/libkern/libkern.h>

#include "exi.h"

#define	WII_RTCSRAM_ID		0xfffff308

#define	RTC_BASE		0x20000000
#define	SRAM_BASE		0x20000100

#define	WRITE_OFFSET		0x80000000

struct rtcsram_sram {
	uint16_t	checksum[2];
	uint16_t	ead[2];
	int32_t		counter_bias;
	int8_t		display_offset_h;
	uint8_t		ntd;
	uint8_t		language;
	uint8_t		flags;
	uint16_t	flash_id[12];
	uint32_t	wireless_keyboard_id;
	uint32_t	wireless_pad_id[2];
	uint8_t		last_dvd_errorcode;
	uint8_t		padding1;
	uint16_t	flash_id_checksum[2];
	uint16_t	padding2;
} __aligned(32);
CTASSERT(sizeof(struct rtcsram_sram) == 64);

struct rtcsram_softc {
	struct todr_chip_handle	sc_todr;

	uint8_t			sc_chan;
	uint8_t			sc_device;

	struct rtcsram_sram	sc_sram;
};

static int	rtcsram_match(device_t, cfdata_t, void *);
static void	rtcsram_attach(device_t, device_t, void *);

static uint32_t	rtcsram_read_4(struct rtcsram_softc *, uint32_t);
static void	rtcsram_write_4(struct rtcsram_softc *, uint32_t, uint32_t);
static void	rtcsram_read_buf(struct rtcsram_softc *, uint32_t, void *,
				 size_t);

static int	rtcsram_gettime(todr_chip_handle_t, struct timeval *);
static int	rtcsram_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(rtcsram, sizeof(struct rtcsram_softc),
	rtcsram_match, rtcsram_attach, NULL, NULL);

static int
rtcsram_match(device_t parent, cfdata_t cf, void *aux)
{
	struct exi_attach_args * const eaa = aux;

	return eaa->eaa_id == WII_RTCSRAM_ID;
}

static void
rtcsram_attach(device_t parent, device_t self, void *aux)
{
	struct rtcsram_softc * const sc = device_private(self);
	struct exi_attach_args * const eaa = aux;

	aprint_naive("\n");
	aprint_normal(": RTC/SRAM\n");

	sc->sc_chan = eaa->eaa_chan;
	sc->sc_device = eaa->eaa_device;

	/* Read RTC counter bias from SRAM. */
	rtcsram_read_buf(sc, SRAM_BASE, &sc->sc_sram, sizeof(sc->sc_sram));
	aprint_debug_dev(self, "counter bias %d\n", sc->sc_sram.counter_bias);
	hexdump(aprint_debug, device_xname(self), &sc->sc_sram,
	    sizeof(sc->sc_sram));

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = rtcsram_gettime;
	sc->sc_todr.todr_settime = rtcsram_settime;
	todr_attach(&sc->sc_todr);
}

static uint32_t
rtcsram_read_4(struct rtcsram_softc *sc, uint32_t offset)
{
	uint32_t val;

	exi_select(sc->sc_chan, sc->sc_device);
	exi_send_imm(sc->sc_chan, sc->sc_device, &offset, sizeof(offset));
	exi_recv_imm(sc->sc_chan, sc->sc_device, &val, sizeof(val));
	exi_unselect(sc->sc_chan);

	return val;
}

static void
rtcsram_write_4(struct rtcsram_softc *sc, uint32_t offset, uint32_t val)
{
	offset |= WRITE_OFFSET;

	exi_select(sc->sc_chan, sc->sc_device);
	exi_send_imm(sc->sc_chan, sc->sc_device, &offset, sizeof(offset));
	exi_send_imm(sc->sc_chan, sc->sc_device, &val, sizeof(val));
	exi_unselect(sc->sc_chan);
}

static void
rtcsram_read_buf(struct rtcsram_softc *sc, uint32_t offset, void *data,
    size_t datalen)
{
	exi_select(sc->sc_chan, sc->sc_device);
	exi_send_imm(sc->sc_chan, sc->sc_device, &offset, sizeof(offset));
	exi_recv_dma(sc->sc_chan, sc->sc_device, data, datalen);
	exi_unselect(sc->sc_chan);
}

static int
rtcsram_gettime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct rtcsram_softc * const sc = ch->cookie;
	uint32_t val;

	val = rtcsram_read_4(sc, RTC_BASE);
	tv->tv_sec = (uint64_t)val + sc->sc_sram.counter_bias;
	tv->tv_usec = 0;

	return 0;
}

static int
rtcsram_settime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct rtcsram_softc * const sc = ch->cookie;

	rtcsram_write_4(sc, RTC_BASE, tv->tv_sec - sc->sc_sram.counter_bias);

	return 0;
}
