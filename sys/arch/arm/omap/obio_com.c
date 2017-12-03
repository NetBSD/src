/*	$NetBSD: obio_com.c,v 1.4.12.2 2017/12/03 11:35:55 jdolecek Exp $	*/

/*
 * Based on arch/arm/omap/omap_com.c
 *
 * Copyright 2003 Wasabi Systems, Inc.
 * OMAP support Copyright (c) 2007 Microsoft
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
__KERNEL_RCSID(0, "$NetBSD: obio_com.c,v 1.4.12.2 2017/12/03 11:35:55 jdolecek Exp $");

#include "opt_omap.h"
#include "opt_com.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap_com.h>

#include <arm/omap/omap2_prcm.h>
#include <arm/omap/am335x_prcm.h>

#include "locators.h"

static int	obiouart_match(device_t, cfdata_t, void *);
static void	obiouart_attach(device_t, device_t, void *);
#if defined(TI_AM335X)
static int	uart_enable_am335x(struct obio_attach_args *);
#else
static int	uart_enable_omap(struct obio_attach_args *);
#endif
static int	uart_enable(struct obio_attach_args *, bus_space_handle_t);
static void	obiouart_callout(void *);

#if defined(TI_AM335X)
struct am335x_com {
	bus_addr_t as_base_addr;
	int as_intr;
	struct omap_module as_module;
};

static const struct am335x_com am335x_com[] = {
	{ 0x44e09000, 72, { AM335X_PRCM_CM_WKUP, 0xb4 } },
	{ 0x48022000, 73, { AM335X_PRCM_CM_PER, 0x6c } },
	{ 0x48024000, 74, { AM335X_PRCM_CM_PER, 0x70 } },
	{ 0x481a6000, 44, { AM335X_PRCM_CM_PER, 0x74 } },
	{ 0x481a8000, 45, { AM335X_PRCM_CM_PER, 0x78 } },
	{ 0x481aa000, 46, { AM335X_PRCM_CM_PER, 0x38 } },
};
#endif

struct com_obio_softc {
	struct com_softc sc_sc;
	struct callout sc_callout;
};

CFATTACH_DECL_NEW(obiouart, sizeof(struct com_obio_softc),
    obiouart_match, obiouart_attach, NULL, NULL);

static int
obiouart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_handle_t bh;
	int rv;

	if (obio->obio_addr == OBIOCF_ADDR_DEFAULT)
		panic("obiouart must have addr specified in config.");

#if 0
	/*
	 * XXX this should be ifdefed on a board-dependent switch
	 * We don't know what is the irq for com0 on the sdp2430
	 */
	if (obio->obio_intr == OBIOCF_INTR_DEFAULT)
		panic("obiouart must have intr specified in config.");
#endif

	if (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		obio->obio_size = OMAP_COM_SIZE;

	if (com_is_console(obio->obio_iot, obio->obio_addr, NULL))
		return 1;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
			  0, &bh) != 0)
		return 0;
	rv = 0;
	if (uart_enable(obio, bh) == 0)
		rv = comprobe1(obio->obio_iot, bh);

	bus_space_unmap(obio->obio_iot, bh, obio->obio_size);

	return rv;
}

static void
obiouart_attach(device_t parent, device_t self, void *aux)
{
	struct com_obio_softc *osc = device_private(self);
	struct com_softc *sc = &osc->sc_sc;
	struct obio_attach_args *obio = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh = 0;
	bus_addr_t iobase;

	sc->sc_dev = self;

	iot = obio->obio_iot;
	iobase = obio->obio_addr;
	sc->sc_frequency = OMAP_COM_FREQ;
	sc->sc_type = COM_TYPE_OMAP;

	if (com_is_console(iot, iobase, &ioh) == 0 &&
	    bus_space_map(iot, iobase, obio->obio_size, 0, &ioh)) {
		panic(": can't map registers\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	if (obio->obio_intr != OBIOCF_INTR_DEFAULT) {
		intr_establish(obio->obio_intr, IPL_SERIAL, IST_LEVEL,
			comintr, sc);
	} else {
		callout_init(&osc->sc_callout, 0);
		callout_reset(&osc->sc_callout, 1, obiouart_callout, osc);
	}
}

static void
obiouart_callout(void *arg)
{
	struct com_obio_softc * const osc = arg;
	int s;
	s = splserial();
	comintr(arg);
	splx(s);
	callout_schedule(&osc->sc_callout, 1);
}


#if defined(TI_AM335X)

static int
uart_enable_am335x(struct obio_attach_args *obio)
{
	int i;

	/* XXX Not really AM335X-specific.  */
	for (i = 0; i < __arraycount(am335x_com); i++)
		if ((obio->obio_addr == am335x_com[i].as_base_addr) &&
		    (obio->obio_intr == am335x_com[i].as_intr)) {
			prcm_module_enable(&am335x_com[i].as_module);
			break;
		}
	KASSERT(i < __arraycount(am335x_com));

	return 0;
}

#else

static int
uart_enable_omap(struct obio_attach_args *obio)
{
	bus_space_handle_t ioh;
	uint32_t r;
#if 0
	uint32_t clksel2;
#endif
	uint32_t fclken1;
	uint32_t iclken1;
	int err __diagused;
	int n=-1;

	KASSERT(obio != NULL);

	err = bus_space_map(obio->obio_iot, OMAP2_CM_BASE,
		OMAP2_CM_SIZE, 0, &ioh);
	KASSERT(err == 0);


	switch(obio->obio_addr) {
	case 0x4806a000:
		fclken1 = OMAP2_CM_FCLKEN1_CORE_EN_UART1;
		iclken1 = OMAP2_CM_ICLKEN1_CORE_EN_UART1;
		n = 1;
		break;
	case 0x4806c000:
		fclken1 = OMAP2_CM_FCLKEN1_CORE_EN_UART2;
		iclken1 = OMAP2_CM_ICLKEN1_CORE_EN_UART2;
		n = 2;
		break;
	case 0x4806e000:
		fclken1 = OMAP2_CM_FCLKEN2_CORE_EN_UART3;
		iclken1 = OMAP2_CM_ICLKEN2_CORE_EN_UART3;
		n = 3;
		break;
	default:
		goto err;
	}

printf("%s: UART#%d\n", __func__, n);

#if 0
	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE);
	r |= clksel2;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE, r);
#endif

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE);
	r |= fclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE, r);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE);
	r |= iclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE, r);

err:
//	bus_space_unmap(obio->obio_iot, ioh, OMAP2_CM_SIZE);

	return 0;
}

#endif

static int
uart_enable(struct obio_attach_args *obio, bus_space_handle_t bh)
{
	uint32_t v;

#if defined(TI_AM335X)
	if (uart_enable_am335x(obio) != 0)
		return -1;
#else
	if (uart_enable_omap(obio) != 0)
		return -1;
#endif

	v = bus_space_read_4(obio->obio_iot, bh, OMAP_COM_SYSC);
	v |= OMAP_COM_SYSC_SOFT_RESET;
	bus_space_write_4(obio->obio_iot, bh, OMAP_COM_SYSC, v);
	v = bus_space_read_4(obio->obio_iot, bh, OMAP_COM_SYSS);
	while (!(v & OMAP_COM_SYSS_RESET_DONE))
		v = bus_space_read_4(obio->obio_iot, bh, OMAP_COM_SYSS);

	/* Disable smart idle */
	v = bus_space_read_4(obio->obio_iot, bh, OMAP_COM_SYSC);
	v |= OMAP_COM_SYSC_NO_IDLE;
	bus_space_write_4(obio->obio_iot, bh, OMAP_COM_SYSC, v);

	return 0;
}
