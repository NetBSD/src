/*	$NetBSD: lpt_ebus.c,v 1.12 2002/03/21 01:17:08 eeh Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * NS Super I/O PC87332VLJ "lpt" to ebus attachment
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/ic/lptvar.h>

int	lpt_ebus_match __P((struct device *, struct cfdata *, void *));
void	lpt_ebus_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_ebus_ca = {
	sizeof(struct lpt_softc), lpt_ebus_match, lpt_ebus_attach
};

#define	ROM_LPT_NAME	"ecpp"

int
lpt_ebus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ebus_attach_args *ea = aux;

	if (strcmp(ea->ea_name, ROM_LPT_NAME) == 0)
		return (1);

	return (0);
}

void
lpt_ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lpt_softc *sc = (void *)self;
	struct ebus_attach_args *ea = aux;
	int i;

	sc->sc_iot = ea->ea_bustag;
	/*
	 * Addresses that shoud be supplied by the prom:
	 *	- normal lpt registers
	 *	- ns873xx configuration registers
	 *	- DMA space
	 * The `lpt' driver does not use DMA accesses, so we can
	 * ignore that for now.  We should enable the lpt port in
	 * the ns873xx registers here. XXX
	 *
	 * Use the prom address if there.
	 */
	if (ea->ea_nvaddr)
		sparc_promaddr_to_handle(sc->sc_iot, ea->ea_vaddr[0],
			&sc->sc_ioh);
	else if (bus_space_map(sc->sc_iot,
			      EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			      ea->ea_reg[0].size,
			      0,
			      &sc->sc_ioh) != 0) {
		printf(": can't map register space\n");
                return;
	}

	for (i = 0; i < ea->ea_nintr; i++)
		bus_intr_establish(ea->ea_bustag, ea->ea_intr[i],
				   IPL_SERIAL, 0, lptintr, sc);
	printf("\n");

	lpt_attach_subr(sc);
}
