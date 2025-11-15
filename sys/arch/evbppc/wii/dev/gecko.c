/* $NetBSD: gecko.c,v 1.1 2025/11/15 17:59:24 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Serial console support over USB Gecko.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: gecko.c,v 1.1 2025/11/15 17:59:24 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/bitops.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <machine/pio.h>
#include <machine/wii.h>

#include <dev/cons.h>

#include "exi.h"
#include "exireg.h"
#include "gecko.h"

#define USBGECKO_CSR					\
	(__SHIFTIN(1, EXI_CSR_CS) |			\
	 __SHIFTIN(EXI_FREQ_32MHZ, EXI_CSR_CLK))
#define USBGECKO_CR(len, rdwr)				\
	(__SHIFTIN((len) - 1, EXI_CR_TLEN) | 		\
	 (rdwr) | EXI_CR_TSTART)

#define USBGECKO_CMD_LED_OFF	0x7000
#define USBGECKO_CMD_LED_ON	0x8000
#define USBGECKO_CMD_ID		0x9000
#define USBGECKO_CMD_RECV_BYTE	0xa000
#define USBGECKO_CMD_SEND_BYTE	0xb000


#define RD4(reg)		in32(EXI_BASE + (reg))
#define WR4(reg, val)		out32(EXI_BASE + (reg), val)

static int	usbgecko_cngetc(dev_t);
static void	usbgecko_cnputc(dev_t, int);
static void	usbgecko_cnpollc(dev_t, int);

static struct cnm_state usbgecko_cnmstate;
static struct consdev usbgecko_consdev = {
	.cn_getc = usbgecko_cngetc,
	.cn_putc = usbgecko_cnputc,
	.cn_pollc = usbgecko_cnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL,
};
static uint32_t usbgecko_chan;
static bool usbgecko_found;

struct usbgecko_softc {
	device_t	sc_dev;
	uint8_t		sc_chan;
	uint8_t		sc_device;
};

static struct usbgecko_softc *usbgecko_sc;

static int	usbgecko_match(device_t, cfdata_t, void *);
static void	usbgecko_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gecko, sizeof(struct usbgecko_softc),
    usbgecko_match, usbgecko_attach, NULL, NULL);

static void
usbgecko_wait(void)
{
	int retry;

	for (retry = 0; retry < 1000; retry++) {
		if ((RD4(EXI_CR(usbgecko_chan)) & EXI_CR_TSTART) == 0) {
			return;
		}
	}
}

static uint16_t
usbgecko_command(uint16_t command)
{
	struct usbgecko_softc *sc = usbgecko_sc;
	uint32_t command32 = (uint32_t)command << 16;
	uint16_t value;

	if (sc != NULL) {
		exi_select(sc->sc_chan, sc->sc_device, EXI_FREQ_32MHZ);
		exi_sendrecv_imm(sc->sc_chan, sc->sc_device, &command32,
		    &value, sizeof(value));
		exi_unselect(sc->sc_chan);
	} else {
		WR4(EXI_CSR(usbgecko_chan), USBGECKO_CSR);
		WR4(EXI_DATA(usbgecko_chan), command32);
		WR4(EXI_CR(usbgecko_chan), USBGECKO_CR(2, EXI_CR_RW_READWRITE));
		usbgecko_wait();
		value = RD4(EXI_DATA(usbgecko_chan)) >> 16;
		WR4(EXI_CSR(usbgecko_chan), 0);
	}

	return value;
}

static uint32_t
usbgecko_id(void)
{
	uint32_t value;

	KASSERT(usbgecko_sc == NULL);

	WR4(EXI_CSR(usbgecko_chan), USBGECKO_CSR);
	WR4(EXI_DATA(usbgecko_chan), 0);
	WR4(EXI_CR(usbgecko_chan), USBGECKO_CR(2, EXI_CR_RW_READWRITE));
	usbgecko_wait();
	WR4(EXI_CR(usbgecko_chan), USBGECKO_CR(sizeof(value),
	    EXI_CR_RW_READWRITE));
	usbgecko_wait();
	value = RD4(EXI_DATA(usbgecko_chan));
	WR4(EXI_CSR(usbgecko_chan), 0);

	return value;
}

static int
usbgecko_cngetc(dev_t dev)
{
	uint16_t value;

	value = usbgecko_command(USBGECKO_CMD_RECV_BYTE);
	return (value & 0x0800) == 0 ? -1 : (value & 0xff);
}

static void
usbgecko_cnputc(dev_t dev, int c)
{
	c &= 0xff;
	usbgecko_command(USBGECKO_CMD_SEND_BYTE | (c << 4));
}

static void
usbgecko_cnpollc(dev_t dev, int on)
{
}

static bool
usbgecko_detect(void)
{
	return usbgecko_id() == 0 &&
	       usbgecko_command(USBGECKO_CMD_ID) == 0x0470;
}

void
usbgecko_consinit(void)
{
	WR4(EXI_CSR(0), 0);
	WR4(EXI_CSR(1), 0);
	WR4(EXI_CSR(2), 0);

	WR4(EXI_CSR(0), EXI_CSR_EXTINT | EXI_CSR_EXTINTMASK);
	WR4(EXI_CSR(1), EXI_CSR_EXTINT | EXI_CSR_EXTINTMASK);

	usbgecko_chan = 0;
	if (!usbgecko_detect()) {
		usbgecko_chan = 1;
		if (!usbgecko_detect()) {
			return;
		}
	}

	cn_tab = &usbgecko_consdev;
	cn_init_magic(&usbgecko_cnmstate);
	cn_set_magic("\047\001");

	usbgecko_command(USBGECKO_CMD_LED_ON);
	usbgecko_found = true;
}

static int
usbgecko_match(device_t parent, cfdata_t cf, void *aux)
{
	struct exi_attach_args * const eaa = aux;

	return usbgecko_found &&
	       eaa->eaa_id == 0 &&
	       eaa->eaa_chan == usbgecko_chan &&
	       eaa->eaa_device == 0;
}

static void
usbgecko_attach(device_t parent, device_t self, void *aux)
{
	struct usbgecko_softc *sc = device_private(self);
	struct exi_attach_args * const eaa = aux;

	aprint_naive("\n");
	aprint_normal(": USB Gecko\n");

	sc->sc_dev = self;
	sc->sc_chan = eaa->eaa_chan;
	sc->sc_device = eaa->eaa_device;

	usbgecko_sc = sc;
}
