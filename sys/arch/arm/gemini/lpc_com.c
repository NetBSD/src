/*	$NetBSD: lpc_com.c,v 1.3.6.2 2009/01/19 13:15:58 skrll Exp $	*/

/* adapted from:
 *	NetBSD: gemini_com.c,v 1.1 2008/10/24 04:23:18 matt Exp
 */

/*
 * Based on arch/arm/xscale/pxa2x0_com.c
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpc_com.c,v 1.3.6.2 2009/01/19 13:15:58 skrll Exp $");

#include "opt_com.h"
#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/callout.h>
#include <sys/kernel.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/gemini/gemini_obiovar.h>
#include <arm/gemini/gemini_lpchcvar.h>
#include <arm/gemini/gemini_lpcvar.h>
#include <arm/gemini/gemini_reg.h>

#include <arm/gemini/lpc_com.h>

typedef struct lpc_com_softc {
	struct com_softc	sc_com;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	int			sc_intr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct callout		sc_callout;
} lpc_com_softc_t;

static int	lpc_com_match(device_t, cfdata_t , void *);
static void	lpc_com_attach(device_t, device_t, void *);
static int	lpc_com_intr(void *);
static void	lpc_com_time(void *);

CFATTACH_DECL_NEW(lpc_com, sizeof(struct lpc_com_softc),
    lpc_com_match, lpc_com_attach, NULL, NULL);

static int
lpc_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gemini_lpc_attach_args *lpc = aux;
	lpctag_t lpctag;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;
	int rv;

	if (lpc->lpc_addr == LPCCF_LDN_DEFAULT
	||  lpc->lpc_addr == LPCCF_ADDR_DEFAULT)
		panic("lpc_com must have ldn and addr"
			" in config.");

	if ((lpc->lpc_intr != LPCCF_INTR_DEFAULT) && (lpc->lpc_intr > 0xff))
		panic("lpc_com: bad intr %d", lpc->lpc_intr);

	if (lpc->lpc_size == LPCCF_SIZE_DEFAULT)
		lpc->lpc_size = IT8712F_UART_SIZE;

	iobase = lpc->lpc_base + lpc->lpc_addr;
	if (com_is_console(lpc->lpc_iot, iobase, NULL))
		return 1;

	lpctag = lpc->lpc_tag;

	lpc_pnp_enter(lpctag);

	/* Activate */
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0x30, 0x01);

	/* Set address */
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0x60,
		(lpc->lpc_addr % 0xff00) >> 8);
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0x61,
		(lpc->lpc_addr % 0x00ff) >> 0);

	/* Set Interrupt Level */
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0x70, lpc->lpc_intr);

	/* Set Special Configuration Regs */
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0xf0, 0x00);
#if 0
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0xf1, 0x50);
#else
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0xf1, 0x58);	/* LO */
#endif
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0xf2, 0x00);
	lpc_pnp_write(lpctag, lpc->lpc_ldn, 0xf3, 0x7f);

	lpc_pnp_exit(lpctag);

	iot = lpc->lpc_iot;
	if (bus_space_map(iot, iobase, lpc->lpc_size, 0, &ioh))
		return 0;

	rv = comprobe1(iot, ioh);

	bus_space_unmap(iot, ioh, lpc->lpc_size);

	return rv;
}

static void
lpc_com_attach(device_t parent, device_t self, void *aux)
{
	struct lpc_com_softc *sc = device_private(self);
	struct gemini_lpc_attach_args *lpc = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t iobase;

	sc->sc_com.sc_dev = self;
	iot = lpc->lpc_iot;
	iobase = lpc->lpc_base + lpc->lpc_addr;
	sc->sc_com.sc_frequency = IT8712F_COM_FREQ;
	sc->sc_com.sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, lpc->lpc_size, 0, &ioh)) {
		panic(": can't map registers\n");
		return;
	} 

	COM_INIT_REGS(sc->sc_com.sc_regs, iot, ioh, iobase);

	com_attach_subr(&sc->sc_com);
	aprint_naive("\n");

	if (lpc->lpc_intr == LPCCF_INTR_DEFAULT) {
		/* callout based polliung */
		callout_init(&sc->sc_callout, 0);
		callout_setfunc(&sc->sc_callout, lpc_com_time, sc);
		callout_schedule(&sc->sc_callout, hz/16);
		aprint_normal("%s: callout polling mode\n", device_xname(self));
	} else {
		/* interrupting */
#if 0
		lpc_intr_establish(lpc->lpc_tag, lpc->lpc_intr,
			IPL_SERIAL, IST_LEVEL_HIGH, comintr, &sc->sc_com);
#else
		lpc_intr_establish(lpc->lpc_tag, lpc->lpc_intr,
			IPL_SERIAL, IST_LEVEL_LOW, lpc_com_intr, &sc->sc_com);
#endif
	}
}

int
lpc_com_intr(void *arg)
{
	printf(".");
	return comintr(arg);
}

void
lpc_com_time(void *arg)
{
	lpc_com_softc_t *sc = arg;
	int s;

	s = splserial();
	(void)comintr(&sc->sc_com);
	callout_schedule(&sc->sc_callout, hz/16);
	splx(s);
}
