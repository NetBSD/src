/*	$NetBSD: com_ebus.c,v 1.11 2002/03/16 14:00:00 mrg Exp $	*/

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
 * NS Super I/O PC87332VLJ "com" to ebus attachment
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/cons.h>
#include <dev/ic/comvar.h>
#include <dev/sun/kbd_ms_ttyvar.h>

#include "kbd.h"
#include "ms.h"

cdev_decl(com); /* XXX this belongs elsewhere */

int	com_ebus_match __P((struct device *, struct cfdata *, void *));
void	com_ebus_attach __P((struct device *, struct device *, void *));

struct cfattach com_ebus_ca = {
	sizeof(struct com_softc), com_ebus_match, com_ebus_attach
};

static char *com_names[] = {
	"su",
	"su_pnp",
	NULL
};

int
com_ebus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ebus_attach_args *ea = aux;
	int i;

	for (i=0; com_names[i]; i++)
		if (strcmp(ea->ea_name, com_names[i]) == 0)
			return (1);

	if (strcmp(ea->ea_name, "serial") == 0) {
		char compat[80];

		/* Could be anything. */
		if ((i = OF_getproplen(ea->ea_node, "compatible")) &&
			OF_getprop(ea->ea_node, "compatible", compat,
				sizeof(compat)) == i) {
			if (strcmp(compat, "su16550") == 0 || 
				strcmp(compat, "su") == 0) {
				return (1);
			}
		}
	}
	return (0);
}

#define BAUD_BASE       (1846200)

void
com_ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct ebus_attach_args *ea = aux;
	struct kbd_ms_tty_attach_args kma;
#if (NKBD > 0) || (NMS > 0)
	int maj;
#endif
	int i;
	int com_is_input;
	int com_is_output;

	sc->sc_iot = ea->ea_bustag;
	sc->sc_iobase = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);
	/*
	 * Addresses that shoud be supplied by the prom:
	 *	- normal com registers
	 *	- ns873xx configuration registers
	 *	- DMA space
	 * The `com' driver does not use DMA accesses, so we can
	 * ignore that for now.  We should enable the com port in
	 * the ns873xx registers here. XXX
	 *
	 * Use the prom address if there.
	 */
	if (ea->ea_nvaddr)
		sc->sc_ioh = (bus_space_handle_t)ea->ea_vaddr[0];
	else if (bus_space_map(sc->sc_iot, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
		ea->ea_reg[0].size, 0, &sc->sc_ioh) != 0) {
		printf(": can't map register space\n");
		return;
	}
	sc->sc_hwflags = 0;
	sc->sc_frequency = BAUD_BASE;

	for (i = 0; i < ea->ea_nintr; i++)
		bus_intr_establish(ea->ea_bustag, ea->ea_intr[i],
		    IPL_SERIAL, 0, comintr, sc);

	kma.kmta_consdev = NULL;

	/* Figure out if we're the console. */
	com_is_input = (ea->ea_node == OF_instance_to_package(OF_stdin()));
	com_is_output = (ea->ea_node == OF_instance_to_package(OF_stdout()));

	if (com_is_input || com_is_output) {
		extern struct consdev comcons;
		struct consdev *cn_orig;

		/* Record some info to attach console. */
		kma.kmta_baud = 9600;
		kma.kmta_cflag = (CREAD | CS8 | HUPCL);

		/* Attach com as the console. */
		cn_orig = cn_tab;
		if (comcnattach(sc->sc_iot, sc->sc_iobase, kma.kmta_baud,
			sc->sc_frequency, kma.kmta_cflag)) {
			printf("Error: comcnattach failed\n");
		}
		cn_tab = cn_orig;
		if (com_is_input) {
			cn_tab->cn_dev = comcons.cn_dev;
			cn_tab->cn_probe = comcons.cn_probe;
			cn_tab->cn_init = comcons.cn_init;
			cn_tab->cn_getc = comcons.cn_getc;
			cn_tab->cn_pollc = comcons.cn_pollc;
		}
		if (com_is_output) {
			cn_tab->cn_putc = comcons.cn_putc;
		}
		kma.kmta_consdev = cn_tab;
	}
	/* Now attach the driver */
	com_attach_subr(sc);

#if (NKBD > 0) || (NMS > 0)
	kma.kmta_tp = sc->sc_tty;
/* If we figure out we're the console we should point this to our consdev */

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == comopen)
			break;

	kma.kmta_dev = makedev(maj, sc->sc_dev.dv_unit);

/* Attach 'em if we got 'em. */
#if (NKBD > 0)
	kma.kmta_name = "keyboard";
	if (OF_getproplen(ea->ea_node, kma.kmta_name) == 0) {
		config_found(self, (void *)&kma, NULL);
	}
#endif
#if (NMS > 0)
	kma.kmta_name = "mouse";
	if (OF_getproplen(ea->ea_node, kma.kmta_name) == 0) {
		config_found(self, (void *)&kma, NULL);
	}
#endif
#endif
	if (kma.kmta_consdev) {
		/*
		 * If we're the keyboard then we need the original
		 * cn_tab w/prom I/O, which sunkbd copied into kma.
		 * Otherwise we want conscom which we copied into
		 * kma.kmta_consdev.
		 */
		cn_tab = kma.kmta_consdev;
	}
}

