/* $Id: rmixl_com.c,v 1.1.2.16 2011/12/24 01:57:54 matt Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: rmixl_com.c,v 1.1.2.16 2011/12/24 01:57:54 matt Exp $");

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

#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixl_comvar.h>

#include "opt_com.h"

/* span of UART regs in bytes */
#define RMIXL_IO_DEV_UART_SIZE	(COM_NPORTS * sizeof(uint32_t))

#define RMIXL_COM_INIT_REGS(regs, bst, ioh, addr) 		\
	do {							\
		memset(&regs, 0, sizeof(regs));			\
		COM_INIT_REGS(regs, bst, ioh, addr);		\
		regs.cr_nports = RMIXL_IO_DEV_UART_SIZE;	\
		rmixl_com_initmap(&regs);			\
	} while (0)


struct rmixl_com_softc {
	struct com_softc sc_com;
};

static void rmixl_com_initmap(struct com_regs *);
static int rmixl_com_match(device_t, cfdata_t , void *);
static void rmixl_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_rmixl, sizeof(struct rmixl_com_softc),
    rmixl_com_match, rmixl_com_attach, NULL, NULL);

#ifndef	COM_REGMAP
#error	COM_REGMAP not defined!
#endif

extern int comcnfreq;
extern int comcnspeed;

int
rmixl_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;
	bus_space_tag_t bst;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	struct com_regs	regs;
	int rv;

	bst = obio->obio_eb_bst;
	addr = obio->obio_addr;
	size = (bus_size_t)RMIXL_IO_DEV_UART_SIZE;

	if (com_is_console(bst, addr, &regs.cr_ioh))
		return 1;

	if (bus_space_map(bst, addr, size, 0, &ioh))
		return 0;		/* FAIL */

	RMIXL_COM_INIT_REGS(regs, bst, ioh, addr);

	rv = com_probe_subr(&regs);

	bus_space_unmap(bst, ioh, size);

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
	sc->sc_frequency = comcnfreq;

	bst = obio->obio_eb_bst;
	addr = obio->obio_addr;
	size = (bus_size_t)RMIXL_IO_DEV_UART_SIZE;

	if (!com_is_console(bst, addr, &ioh)
	&&  bus_space_map(bst, addr, size, 0, &ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	RMIXL_COM_INIT_REGS(sc->sc_regs, bst, ioh, addr);

	com_attach_subr(sc);

	rmixl_intr_establish(obio->obio_intr,
		IPL_VM, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
		comintr, sc, true);

}

void
rmixl_com_initmap(struct com_regs *regsp)
{
	/*
	 * map the 4 byte register stride
	 */
	for (int i = 0; i < __arraycount(regsp->cr_map); i++)
		regsp->cr_map[i] = com_std_map[i] * 4;
}

void
rmixl_com_cnattach(bus_addr_t addr, int speed, int freq,
	int type, tcflag_t mode)
{
	bus_space_tag_t bst;
	struct com_regs	regs;

	bst = (bus_space_tag_t)&rmixl_configuration.rc_obio_eb_memt;

	RMIXL_COM_INIT_REGS(regs, bst, 0, addr);

	comcnattach1(&regs, speed, freq, type, mode);
}
