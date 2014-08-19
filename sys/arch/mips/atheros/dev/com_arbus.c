/* $NetBSD: com_arbus.c,v 1.10.12.1 2014/08/20 00:03:12 tls Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: com_arbus.c,v 1.10.12.1 2014/08/20 00:03:12 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <sys/types.h>

#include <sys/bus.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/cpuregs.h>
#include <mips/atheros/include/arbusvar.h>

#include "opt_com.h"

struct com_arbus_softc {
	struct com_softc sc_com;
};

static void com_arbus_initmap(struct com_regs *);
//static bus_space_tag_t com_arbus_get_bus_space_tag(void);
static int com_arbus_match(device_t, cfdata_t , void *);
static void com_arbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_arbus, sizeof(struct com_arbus_softc),
    com_arbus_match, com_arbus_attach, NULL, NULL);

#if 0
#ifdef	TTYDEF_SPEED
#define	COM_ARBUS_BAUD	TTYDEF_SPEED
#else
#define	COM_ARBUS_BAUD	115200
#endif
#endif

#ifndef COM_ARBUS_BAUD
#define	COM_ARBUS_BAUD	115200
#endif

int	com_arbus_baud = COM_ARBUS_BAUD;

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

#ifndef	COM_REGMAP
#error	COM_REGMAP not defined!
#endif

int
com_arbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct arbus_attach_args	*aa = aux;
	struct com_regs			regs;
	int				rv;

	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;

	if (com_is_console(aa->aa_bst, aa->aa_addr, &regs.cr_ioh))
		return 1;

	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size,
		0, &regs.cr_ioh))
		return 0;

	regs.cr_iot = aa->aa_bst;
	regs.cr_iobase = aa->aa_addr;
	regs.cr_nports = aa->aa_size;
	com_arbus_initmap(&regs);

	rv = com_probe_subr(&regs);

	bus_space_unmap(aa->aa_bst, regs.cr_ioh, aa->aa_size);

	return rv;
}

void
com_arbus_attach(device_t parent, device_t self, void *aux)
{
	struct com_arbus_softc *arsc = device_private(self);
	struct com_softc *sc = &arsc->sc_com;
	struct arbus_attach_args *aa = aux;
	prop_number_t prop;
	bus_space_handle_t ioh;

	sc->sc_dev = self;

	prop = prop_dictionary_get(device_properties(sc->sc_dev),
	    "frequency");
	if (prop == NULL) {
		aprint_error(": unable to get frequency property\n");
		return;
	}
	KASSERT(prop_object_type(prop) == PROP_TYPE_NUMBER);

	sc->sc_frequency = (int)prop_number_integer_value(prop);

	if (!com_is_console(aa->aa_bst, aa->aa_addr, &ioh)
	    && bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0,
		    &ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, aa->aa_bst, ioh, aa->aa_addr);
	sc->sc_regs.cr_nports = aa->aa_size;
	com_arbus_initmap(&sc->sc_regs);

	com_attach_subr(sc);

	arbus_intr_establish(aa->aa_cirq, aa->aa_mirq, comintr, sc);
}

void
com_arbus_initmap(struct com_regs *regsp)
{
#if _BYTE_ORDER == _BIG_ENDIAN
	int off = 3;
#else
	int off = 0;
#endif

	/* rewrite the map to shift for alignment */
	for (size_t i = 0; i < __arraycount(regsp->cr_map); i++) {
		regsp->cr_map[i] = (com_std_map[i] * 4) + off;
	}
}

void
com_arbus_cnattach(bus_addr_t addr, uint32_t freq)
{
	struct com_regs		regs;

	regs.cr_iot = arbus_get_bus_space_tag();
	regs.cr_iobase = addr;
	regs.cr_nports = 0x1000;
	com_arbus_initmap(&regs);

	if (bus_space_map(regs.cr_iot, regs.cr_iobase, regs.cr_nports, 0,
		&regs.cr_ioh))
		return;

	comcnattach1(&regs, com_arbus_baud, freq, COM_TYPE_NORMAL, CONMODE);
}

