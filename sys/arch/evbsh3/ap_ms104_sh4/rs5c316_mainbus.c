/*	$NetBSD: rs5c316_mainbus.c,v 1.1.6.2 2010/08/11 22:51:56 yamt Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rs5c316_mainbus.c,v 1.1.6.2 2010/08/11 22:51:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/clock_subr.h>
#include <dev/ic/rs5c313var.h>

#include <machine/autoconf.h>

#include <sh3/devreg.h>
#include <sh3/pfcreg.h>

#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4reg.h>
#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4var.h>

/* chip access methods */
static void rtc_begin(struct rs5c313_softc *);
static void rtc_ce(struct rs5c313_softc *, int);
static void rtc_dir(struct rs5c313_softc *, int);
static void rtc_clk(struct rs5c313_softc *, int);
static int  rtc_read(struct rs5c313_softc *);
static void rtc_write(struct rs5c313_softc *, int);

static struct rs5c313_ops rs5c316_mainbus_ops = {
	.rs5c313_op_begin = rtc_begin,
	.rs5c313_op_ce    = rtc_ce,
	.rs5c313_op_clk   = rtc_clk,
	.rs5c313_op_dir   = rtc_dir,
	.rs5c313_op_read  = rtc_read,
	.rs5c313_op_write = rtc_write,
};

/* autoconf glue */
static int rs5c316_mainbus_match(device_t, cfdata_t, void *);
static void rs5c316_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rs5c313_mainbus, sizeof(struct rs5c313_softc),
    rs5c316_mainbus_match, rs5c316_mainbus_attach, NULL, NULL);

#define ndelay(x) delay(((x) + 999) / 1000)

static int
rs5c316_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = (struct mainbus_attach_args *)aux;

	if (strcmp(maa->ma_name, "rs5c313rtc") != 0)
		return 0;
	return 1;
}


static void
rs5c316_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct rs5c313_softc *sc = device_private(self);
	uint32_t reg;

	sc->sc_dev = self;
	sc->sc_model = MODEL_5C316;
	sc->sc_ops = &rs5c316_mainbus_ops;

	/* setup gpio pin */
	reg = _reg_read_4(SH4_PCTRA);
	reg &= ~(3 << (GPIO_PIN_RTC_CE * 2));
	reg |=  (1 << (GPIO_PIN_RTC_CE * 2));	/* output */
	reg &= ~(3 << (GPIO_PIN_RTC_SCLK * 2));
	reg |=  (1 << (GPIO_PIN_RTC_SCLK * 2));	/* output */
	reg &= ~(3 << (GPIO_PIN_RTC_SIO * 2));
	reg |=  (1 << (GPIO_PIN_RTC_SIO * 2));	/* output */
	_reg_write_4(SH4_PCTRA, reg);

	rs5c313_attach(sc);
}

static void
rtc_begin(struct rs5c313_softc *sc)
{

	/* nothing to do */
}

static void
rtc_ce(struct rs5c313_softc *sc, int onoff)
{
	uint16_t

	reg = _reg_read_2(SH4_PDTRA);
	if (onoff) {
		reg |= (1 << GPIO_PIN_RTC_CE);
	} else {
		reg &= ~(1 << GPIO_PIN_RTC_CE);
	}
	_reg_write_2(SH4_PDTRA, reg);
	ndelay(600);
}

static void
rtc_clk(struct rs5c313_softc *sc, int onoff)
{
	uint16_t reg;
	
	reg = _reg_read_2(SH4_PDTRA);
	if (onoff) {
		reg |= (1 << GPIO_PIN_RTC_SCLK);
	} else {
		reg &= ~(1 << GPIO_PIN_RTC_SCLK);
	}
	_reg_write_2(SH4_PDTRA, reg);
}

static void
rtc_dir(struct rs5c313_softc *sc, int output)
{
	uint32_t reg;

	reg = _reg_read_4(SH4_PCTRA);
	reg &= ~(3 << (GPIO_PIN_RTC_SIO * 2));		/* input */
	if (output) {
		reg |=  (1 << (GPIO_PIN_RTC_SIO * 2));	/* output */
	}
	_reg_write_4(SH4_PCTRA, reg);
}

static int
rtc_read(struct rs5c313_softc *sc)
{
	int bit;

	ndelay(300);

	bit = (_reg_read_2(SH4_PDTRA) & (1 << GPIO_PIN_RTC_SIO)) ? 1 : 0;

	rtc_clk(sc, 0);
	ndelay(300);
	rtc_clk(sc, 1);

	return bit;
}

static void
rtc_write(struct rs5c313_softc *sc, int bit)
{
	uint16_t reg;
	
	reg = _reg_read_2(SH4_PDTRA);
	if (bit)
		reg |= (1 << GPIO_PIN_RTC_SIO);
	else
		reg &= ~(1 << GPIO_PIN_RTC_SIO);
	_reg_write_2(SH4_PDTRA, reg);

	ndelay(300);

	rtc_clk(sc, 0);
	ndelay(300);
	rtc_clk(sc, 1);
}
