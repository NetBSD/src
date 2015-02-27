/* $NetBSD: amlogic_com.c,v 1.2 2015/02/27 17:35:08 jmcneill Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: amlogic_com.c,v 1.2 2015/02/27 17:35:08 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <dev/cons.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>
#include <arm/amlogic/amlogic_comreg.h>
#include <arm/amlogic/amlogic_comvar.h>

static int amlogic_com_match(device_t, cfdata_t, void *);
static void amlogic_com_attach(device_t, device_t, void *);

struct amlogic_com_softc {
	device_t sc_dev;
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;

	int sc_ospeed;
	tcflag_t sc_cflag;
};

static struct amlogic_com_softc amlogic_com_cnsc;

static struct cnm_state amlogic_com_cnm_state;

static int	amlogic_com_cngetc(dev_t);
static void	amlogic_com_cnputc(dev_t, int);
static void	amlogic_com_cnpollc(dev_t, int);

struct consdev amlogic_com_consdev = {
	.cn_getc = amlogic_com_cngetc,
	.cn_putc = amlogic_com_cnputc,
	.cn_pollc = amlogic_com_cnpollc,
};

CFATTACH_DECL_NEW(amlogic_com, sizeof(struct amlogic_com_softc),
	amlogic_com_match, amlogic_com_attach, NULL, NULL);

static int
amlogic_com_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_com_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_com_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	const bus_addr_t iobase = AMLOGIC_CORE_BASE + loc->loc_offset;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_bsh = aio->aio_bsh;

	aprint_naive("\n");
	if (amlogic_com_is_console(iobase)) {
		aprint_normal(": (console)\n");
	} else {
		aprint_normal("\n");
	}
}

static int
amlogic_com_cngetc(dev_t dev)
{
	bus_space_tag_t bst = amlogic_com_cnsc.sc_bst;
	bus_space_handle_t bsh = amlogic_com_cnsc.sc_bsh;
	uint32_t status;
	int s, c;

	s = splserial();

	status = bus_space_read_4(bst, bsh, UART_STATUS_REG);
	if (status & UART_STATUS_RX_EMPTY) {
		splx(s);
		return -1;
	}

	c = bus_space_read_4(bst, bsh, UART_RFIFO_REG);
#if defined(DDB)
	extern int db_active;
	if (!db_active)
#endif
	{
		int cn_trapped __unused = 0;
		cn_check_magic(dev, c, amlogic_com_cnm_state);
	}

	splx(s);

	return c & 0xff;
}

static void
amlogic_com_cnputc(dev_t dev, int c)
{
	bus_space_tag_t bst = amlogic_com_cnsc.sc_bst;
	bus_space_handle_t bsh = amlogic_com_cnsc.sc_bsh;
	int s;

	s = splserial();

	while ((bus_space_read_4(bst, bsh, UART_STATUS_REG) & UART_STATUS_TX_EMPTY) == 0)
		;

	bus_space_write_4(bst, bsh, UART_WFIFO_REG, c);

	splx(s);
}
	

static void
amlogic_com_cnpollc(dev_t dev, int on)
{
}

bool
amlogic_com_cnattach(bus_space_tag_t bst, bus_space_handle_t bsh,
    int ospeed, tcflag_t cflag)
{
	struct amlogic_com_softc *sc = &amlogic_com_cnsc;

	cn_tab = &amlogic_com_consdev;
	cn_init_magic(&amlogic_com_cnm_state);
	cn_set_magic("\047\001");

	sc->sc_bst = bst;
	sc->sc_bsh = bsh;
	sc->sc_ospeed = ospeed;
	sc->sc_cflag = cflag;

	return true;
}

bool
amlogic_com_is_console(bus_addr_t iobase)
{
	return iobase == CONSADDR;
}
