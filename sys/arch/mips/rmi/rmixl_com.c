/* $Id: rmixl_com.c,v 1.2 2009/12/14 00:46:07 matt Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: rmixl_com.c,v 1.2 2009/12/14 00:46:07 matt Exp $");

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
	int rate;
	extern int comspeed(long, long, int);

	com0addr = (uint32_t *)
		MIPS_PHYS_TO_KSEG1(io_pbase + RMIXL_IO_DEV_UART_1);

	if (CONSFREQ != -1) {
		rate = comspeed(CONSPEED, CONSFREQ, COM_TYPE_NORMAL);
		if (rate < 0)
			return;					/* XXX */

		com0addr[com_ier] = 0;
		com0addr[com_lctl] = htobe32(LCR_DLAB);
		com0addr[com_dlbl] = htobe32(rate & 0xff);
		com0addr[com_dlbh] = htobe32(rate >> 8);
		com0addr[com_lctl] = htobe32(LCR_8BITS);	/* XXX */
		com0addr[com_mcr]  = htobe32(MCR_DTR|MCR_RTS);
		com0addr[com_fifo] = htobe32(
			FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_1);
	}
}


void
rmixl_putchar(char c)
{
	int timo = 150000;

	while ((be32toh(com0addr[com_lsr]) & LSR_TXRDY) == 0)
		if (--timo == 0)
			break;

	com0addr[com_data] = htobe32((uint32_t)c);

	while ((be32toh(com0addr[com_lsr]) & LSR_TSRE) == 0)
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

static char hexc[] = "0123456789abcdef";

#define RMIXL_PUTHEX 						\
	u_int shift = sizeof(val) * 8;				\
	rmixl_putchar('0');					\
	rmixl_putchar('x');					\
	do {							\
		shift -= 4;					\
		rmixl_putchar(hexc[(val >> shift) & 0xf]);	\
	} while(shift != 0)

void
rmixl_puthex32(uint32_t val)
{
	RMIXL_PUTHEX;
}

void
rmixl_puthex64(uint64_t val)
{
	RMIXL_PUTHEX;
}

int
rmixl_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_tag_t bst;
	bus_addr_t addr;
	bus_size_t size;
	struct com_regs	regs;
	int rv;

	bst = obio->obio_bst;
	addr = obio->obio_addr;
	size = obio->obio_size;

	if (com_is_console(bst, addr, &regs.cr_ioh))
		return 1;

	if (bus_space_map(bst, addr, size, 0, &regs.cr_ioh))
		return 0;		/* FAIL */

	memset(&regs, 0, sizeof(regs));
	regs.cr_iot = bst;
	regs.cr_iobase = addr;
	regs.cr_nports = size;
	rmixl_com_initmap(&regs);

	rv = com_probe_subr(&regs);

	bus_space_unmap(bst, regs.cr_ioh, size);

	return rv;
}

void
rmixl_com_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_com_softc *rsc = device_private(self);
	struct com_softc *sc = &rsc->sc_com;
	struct obio_attach_args *obio = aux;
	bus_space_tag_t bst;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_frequency = CONSFREQ;

	bst = obio->obio_bst;
	addr = obio->obio_addr;
	size = obio->obio_size;

	if (!com_is_console(bst, addr, &ioh)
	&&  bus_space_map(bst, addr, size, 0, &ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	memset(&sc->sc_regs, 0, sizeof(sc->sc_regs));
	COM_INIT_REGS(sc->sc_regs, bst, ioh, addr);
	sc->sc_regs.cr_nports = size;
	rmixl_com_initmap(&sc->sc_regs);

	com_attach_subr(sc);

	rmixl_intr_establish(obio->obio_intr, IPL_SERIAL,
		RMIXL_INTR_LEVEL, RMIXL_INTR_HIGH, comintr, sc);

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
rmixl_com_cnattach(bus_addr_t addr, int speed, int freq,
	int type, tcflag_t mode)
{
	bus_space_tag_t bst;
	bus_size_t sz;
	struct com_regs	regs;

	bst = (bus_space_tag_t)&rmixl_configuration.rc_obio_memt;
	sz = COM_NPORTS * sizeof(uint32_t);	/* span of UART regs in bytes */

	memset(&regs, 0, sizeof(regs));
	rmixl_com_initmap(&regs);
	regs.cr_iot = bst;
	regs.cr_iobase = addr;
	regs.cr_nports = sz;

	comcnattach1(&regs, speed, freq, type, mode);
}
