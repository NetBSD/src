/* $Id: rmixl_com.c,v 1.1.2.2 2009/09/13 07:00:30 cliff Exp $ */
/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_com.c,v 1.1.2.2 2009/09/13 07:00:30 cliff Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <sys/types.h>

#include <machine/bus.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/cpuregs.h>

#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixl_comvar.h>
#include <mips/rmi/rmixlvar.h>

#include "opt_com.h"

struct rmixl_com_softc {
	struct com_softc sc_com;
};

static void rmixl_com_initmap(struct com_regs *);
static int rmixl_com_match(device_t, cfdata_t , void *);
static void rmixl_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rmixl_com, sizeof(struct rmixl_com_softc),
    rmixl_com_match, rmixl_com_attach, NULL, NULL);

#ifndef	COM_REGMAP
#error	COM_REGMAP not defined!
#endif

volatile uint32_t *com0addr = (uint32_t *)
	MIPS_PHYS_TO_KSEG1(RMIXL_IO_DEV_PBASE + RMIXL_IO_DEV_UART_1);

void
rmixl_putchar_init(uint64_t io_pbase)
{
	com0addr = (uint32_t *)MIPS_PHYS_TO_KSEG1(io_pbase + RMIXL_IO_DEV_UART_1);
}

void
rmixl_putchar(char c)
{
	int timo = 150000;

	while ((com0addr[5] & LSR_TXRDY) == 0)
		if (--timo == 0)
			break;

	com0addr[0] = c;

	while ((com0addr[5] & LSR_TSRE) == 0)
		if (--timo == 0)
			break;
}

void
rmixl_puts(const char *restrict s)
{
	char c;

	while ((c = *s++) != 0)
		rmixl_putchar(c);
}

void
rmixl_puthex32(uint32_t val)
{
	char hexc[] = "0123456789abcdef";

	rmixl_putchar('0');
	rmixl_putchar('x');
	rmixl_putchar(hexc[(val >> 28) & 0xf]);
	rmixl_putchar(hexc[(val >> 24) & 0xf]);
	rmixl_putchar(hexc[(val >> 20) & 0xf]);
	rmixl_putchar(hexc[(val >> 16) & 0xf]);
	rmixl_putchar(hexc[(val >> 12) & 0xf]);
	rmixl_putchar(hexc[(val >> 8) & 0xf]);
	rmixl_putchar(hexc[(val >> 4) & 0xf]);
	rmixl_putchar(hexc[(val >> 0) & 0xf]);
}

void
rmixl_puthex64(uint64_t val)
{
	char hexc[] = "0123456789abcdef";

	rmixl_putchar('0');
	rmixl_putchar('x');
	rmixl_putchar(hexc[(val >> 60) & 0xf]);
	rmixl_putchar(hexc[(val >> 56) & 0xf]);
	rmixl_putchar(hexc[(val >> 52) & 0xf]);
	rmixl_putchar(hexc[(val >> 48) & 0xf]);
	rmixl_putchar(hexc[(val >> 44) & 0xf]);
	rmixl_putchar(hexc[(val >> 40) & 0xf]);
	rmixl_putchar(hexc[(val >> 36) & 0xf]);
	rmixl_putchar(hexc[(val >> 32) & 0xf]);
	rmixl_putchar(hexc[(val >> 28) & 0xf]);
	rmixl_putchar(hexc[(val >> 24) & 0xf]);
	rmixl_putchar(hexc[(val >> 20) & 0xf]);
	rmixl_putchar(hexc[(val >> 16) & 0xf]);
	rmixl_putchar(hexc[(val >> 12) & 0xf]);
	rmixl_putchar(hexc[(val >> 8) & 0xf]);
	rmixl_putchar(hexc[(val >> 4) & 0xf]);
	rmixl_putchar(hexc[(val >> 0) & 0xf]);
}

int
rmixl_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	struct com_regs	regs;
	int rv;

	if (com_is_console(obio->obio_bst, obio->obio_addr, &regs.cr_ioh))
		return 1;

	if (bus_space_map(obio->obio_bst, obio->obio_addr, obio->obio_size,
		0, &regs.cr_ioh))
			return 0;

	regs.cr_iot = obio->obio_bst;
	regs.cr_iobase = obio->obio_addr;
	regs.cr_nports = obio->obio_size;
	rmixl_com_initmap(&regs);

	rv = com_probe_subr(&regs);

	bus_space_unmap(obio->obio_bst, regs.cr_ioh, obio->obio_size);

	return rv;
}

void
rmixl_com_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_com_softc *rsc = device_private(self);
	struct com_softc *sc = &rsc->sc_com;
	struct obio_attach_args *obio = aux;
#if 0
	prop_number_t prop;
#endif
	bus_space_handle_t ioh;

	sc->sc_dev = self;

#if 0
	prop = prop_dictionary_get(device_properties(sc->sc_dev),
	    "frequency");
	if (prop == NULL) {
		aprint_error(": unable to get frequency property\n");
		return;
	}
	KASSERT(prop_object_type(prop) == PROP_TYPE_NUMBER);

	sc->sc_frequency = (int)prop_number_integer_value(prop);
#else
	sc->sc_frequency = -1;	/* XXX */
#endif

	if (!com_is_console(obio->obio_bst, obio->obio_addr, &ioh) &&
	    bus_space_map(obio->obio_bst, obio->obio_addr, obio->obio_size, 0,
		&ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, obio->obio_bst, ioh, obio->obio_addr);
	sc->sc_regs.cr_nports = obio->obio_size;
	rmixl_com_initmap(&sc->sc_regs);

	com_attach_subr(sc);

#ifdef NOTYET
	rmixl_intr_establish(obio->obio_intr, comintr, sc);
#endif
}

void
rmixl_com_initmap(struct com_regs *regsp)
{
	int i;
	int sz;

	/*
	 * map the 4 byte register stride
	 */
	sz = sizeof(regsp->cr_map) / sizeof(regsp->cr_map[0]);
	for (i = 0; i < 16; i++)
		regsp->cr_map[i] = com_std_map[i] * 4;
}

void
rmixl_com_cnattach(bus_addr_t addr, int speed, int freq, int type, tcflag_t mode)
{
	bus_space_tag_t bst;
	bus_size_t sz;
	struct com_regs	regs;

	bst = rmixl_obio_get_bus_space_tag();
	sz = COM_NPORTS * sizeof(uint32_t);	/* span of UART regs in bytes */

	memset(&regs, 0, sizeof(regs));
	rmixl_com_initmap(&regs);
	regs.cr_iot = bst;
	regs.cr_iobase = addr;
	regs.cr_nports = sz;

	comcnattach1(&regs, speed, freq, type, mode);
}
