/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_broadcom.h"
#include "locators.h"
#include "com.h"
#include "gpio.h"
#include "bcmcca.h"

#define	CCA_PRIVATE
#define CRU_PRIVATE
#define IDM_PRIVATE

#if NCOM == 0
#error no console configured
#endif

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_cca.c,v 1.5 2022/03/03 06:26:28 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/mainbus/mainbus.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmcca_mainbus_match(device_t, cfdata_t, void *);
static void bcmcca_mainbus_attach(device_t, device_t, void *);

struct bcmcca_softc;
static void bcmcca_uart_attach(struct bcmcca_softc *sc);
#if NGPIO > 0
static void bcmcca_gpio_attach(struct bcmcca_softc *sc);
#endif

struct bcmcca_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct com_softc *sc_com_softc[2];
	void *sc_ih;
	uint32_t sc_gpiopins;
};

struct bcmcca_attach_args {
	bus_space_tag_t ccaaa_bst;
	bus_space_handle_t ccaaa_bsh;
	bus_size_t ccaaa_offset;
	bus_size_t ccaaa_size;
	int ccaaa_channel;
};

static struct bcmcca_softc bcmcca_sc = {
	.sc_gpiopins = 0xffffff,	/* assume all 24 pins are available */
};

CFATTACH_DECL_NEW(bcmcca, 0,
	bcmcca_mainbus_match, bcmcca_mainbus_attach, NULL, NULL);

static int
bcmcca_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	if (bcmcca_sc.sc_dev != NULL)
		return 0;

	return 1;
}

static int
bcmcca_print(void *aux, const char *pnp)
{
	const struct bcmcca_attach_args * const ccaaa = aux;

	if (ccaaa->ccaaa_channel != BCMCCACF_CHANNEL_DEFAULT)
		aprint_normal(" channel %d", ccaaa->ccaaa_channel);

	return QUIET;
}

static inline uint32_t
bcmcca_read_4(struct bcmcca_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmcca_write_4(struct bcmcca_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static int
bcmcca_intr(void *arg)
{
	struct bcmcca_softc * sc = arg;
	int rv = 0;

	uint32_t v = bcmcca_read_4(sc, MISC_INTSTATUS);
	if (v & INTSTATUS_UARTINT) {
		if (sc->sc_com_softc[0] != NULL)
			rv = comintr(sc->sc_com_softc[0]);
		if (sc->sc_com_softc[1] != NULL) {
			int rv0 = comintr(sc->sc_com_softc[1]);
			if (rv)
				rv = rv0;
		}
	}
	if (v & INTSTATUS_GPIOINT) {
		
	}
	return rv;
}

static void
bcmcca_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct bcmcca_softc * const sc = &bcmcca_sc;

	sc->sc_dev = self;
	device_set_private(self, sc);

	sc->sc_bst = bcm53xx_ioreg_bst;

	bus_space_subregion (sc->sc_bst, bcm53xx_ioreg_bsh,
	    CCA_MISC_BASE, CCA_MISC_SIZE, &sc->sc_bsh);

	uint32_t chipid = bcmcca_read_4(sc, MISC_CHIPID);

	aprint_naive("\n");
	aprint_normal(": BCM%u (Rev %c%u)\n",
	    (u_int)__SHIFTOUT(chipid, CHIPID_ID),
	    (u_int)('A' + (__SHIFTOUT(chipid, CHIPID_REV) >> 2)),
	    (u_int)(__SHIFTOUT(chipid, CHIPID_REV) & 3));

	sc->sc_ih = intr_establish(IRQ_CCA, IPL_TTY, IST_LEVEL, bcmcca_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish CCA intr\n");
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at irq %d\n", IRQ_CCA);

	bcmcca_uart_attach(sc);
#if NGPIO > 0
	bcmcca_gpio_attach(sc);
#endif
}

static void
bcmcca_uart_attach(struct bcmcca_softc *sc)
{
	struct bcmcca_attach_args ccaaa = {
		.ccaaa_bst = sc->sc_bst,
		.ccaaa_bsh = sc->sc_bsh,
		.ccaaa_offset = CCA_UART0_BASE,
		.ccaaa_size = COM_NPORTS,
		.ccaaa_channel = 0,
	};
	device_t dv;

#if 0
	/*
	 * Force the UART to use the BCM53xx reference clock.
	 */
	uint32_t v = bcmcca_read_4(sc, IDM_BASE + APBX_IDM_IO_CONTROL_DIRECT);
	if (v & IO_CONTROL_DIRECT_UARTCLKSEL) {
		v &= ~IO_CONTROL_DIRECT_UARTCLKSEL;
		bcmcca_write_4(sc, IDM_BASE + APBX_IDM_IO_CONTROL_DIRECT, v);
	}
	v = bcmcca_read_4(sc, MISC_CORECTL);
	if (v & CORECTL_UART_CLK_OVERRIDE) {
		v &= ~CORECTL_UART_CLK_OVERRIDE;
		bcmcca_write_4(sc, MISC_CORECTL, v);
	}
#endif

	bool children = false;

	dv = config_found(sc->sc_dev, &ccaaa, bcmcca_print, CFARGS_NONE);
	if (dv != NULL) {
		sc->sc_com_softc[0] = device_private(dv);
		children = true;
	}

	ccaaa.ccaaa_offset = CCA_UART1_BASE;
	ccaaa.ccaaa_channel = 1;

	dv = config_found(sc->sc_dev, &ccaaa, bcmcca_print, CFARGS_NONE);
	if (dv != NULL) {
		sc->sc_com_softc[1] = device_private(dv);
		children = true;
		/*
		 * UART1 uses the same pins as GPIO pins 15..12
		 */
		sc->sc_gpiopins &= ~__BITS(15,12);
	}

	if (children) {
		/*
		 * If we configured children, enable interrupts for the UART(s).
		 */
		uint32_t intmask = bcmcca_read_4(sc, MISC_INTMASK);
		intmask |= INTMASK_UARTINT;
		bcmcca_write_4(sc, MISC_INTMASK, intmask);
	}
}

static int com_cca_match(device_t, cfdata_t, void *);
static void com_cca_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_cca, sizeof(struct com_softc),
    com_cca_match, com_cca_attach, NULL, NULL);

static int
com_cca_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmcca_attach_args * const ccaaa = aux;
	const int channel = cf->cf_loc[BCMCCACF_CHANNEL];
	const bus_addr_t addr = BCM53XX_IOREG_PBASE + ccaaa->ccaaa_offset;
	bus_space_handle_t bsh;

	KASSERT(ccaaa->ccaaa_offset == CCA_UART0_BASE || ccaaa->ccaaa_offset == CCA_UART1_BASE);
	KASSERT(bcmcca_sc.sc_com_softc[ccaaa->ccaaa_channel] == NULL);

	if (channel != BCMCCACF_CHANNEL_DEFAULT && channel != ccaaa->ccaaa_channel)
		return 0;

	if (com_is_console(ccaaa->ccaaa_bst, addr, NULL))
		return 1;

	bus_space_subregion(ccaaa->ccaaa_bst, ccaaa->ccaaa_bsh,
	    ccaaa->ccaaa_offset, ccaaa->ccaaa_size, &bsh);

	return comprobe1(ccaaa->ccaaa_bst, bsh);
}

static void
com_cca_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc * const sc = device_private(self);
	struct bcmcca_attach_args * const ccaaa = aux;
	const bus_addr_t addr = BCM53XX_IOREG_PBASE + ccaaa->ccaaa_offset;
	bus_space_handle_t bsh;

	sc->sc_dev = self;
	sc->sc_frequency = BCM53XX_REF_CLK;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(ccaaa->ccaaa_bst, addr, &bsh) == 0 &&
	    bus_space_subregion(ccaaa->ccaaa_bst, ccaaa->ccaaa_bsh,
		ccaaa->ccaaa_offset, ccaaa->ccaaa_size, &bsh)) {
		panic(": can't map registers\n");
		return;
	}
	com_init_regs(&sc->sc_regs, ccaaa->ccaaa_bst, bsh, addr);

	com_attach_subr(sc);
}

#if NGPIO > 0
static void
bcmcca_gpio_attach(struct bcmcca_softc *sc)
{
	/*
	 * First see if there are any pins being used as GPIO pins...
	 */
	uint32_t v = bcmcca_read(sc, CRU_BASE + CRU_GPIO_SELECT);
	if (v == 0)
		return;
}
#endif
