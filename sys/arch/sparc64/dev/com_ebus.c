/*	$NetBSD: com_ebus.c,v 1.30.6.1 2011/06/06 09:06:49 jruoho Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_ebus.c,v 1.30.6.1 2011/06/06 09:06:49 jruoho Exp $");

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

int	com_ebus_match(device_t, cfdata_t , void *);
void	com_ebus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_ebus, sizeof(struct com_softc),
    com_ebus_match, com_ebus_attach, NULL, NULL);

static const char *com_names[] = {
	"su",
	"su_pnp",
	"rsc-console",
	NULL
};

int
com_ebus_match(device_t parent, cfdata_t match, void *aux)
{
	struct ebus_attach_args *ea = aux;
	int i;

	for (i = 0; com_names[i]; i++)
		if (strcmp(ea->ea_name, com_names[i]) == 0)
			return (1);

	if (strcmp(ea->ea_name, "serial") == 0) {
		char *compat;

		/* Could be anything. */
		compat = prom_getpropstring(ea->ea_node, "compatible");
		if (strcmp(compat, "su16550") == 0 ||
		    strcmp(compat, "su16552") == 0 ||
		    strcmp(compat, "su") == 0 ||
		    strcmp(compat, "FJSV,su") == 0) {
			return (1);
		}
	}
	return (0);
}

#define BAUD_BASE       (1846200)

void
com_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	struct kbd_ms_tty_attach_args kma;
#if (NKBD > 0) || (NMS > 0)
	int maj;
	extern const struct cdevsw com_cdevsw;
#endif
	int i;
	int com_is_input;
	int com_is_output;
	bus_space_handle_t ioh;
	bus_space_tag_t iot;
	bus_addr_t iobase;
	int node, port;
	char buf[32];

	sc->sc_dev = self;
	iot = ea->ea_bustag;
	iobase = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);

	/*
	 * Addresses that should be supplied by the prom:
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
		sparc_promaddr_to_handle(iot, ea->ea_vaddr[0],
			&ioh);
	else if (bus_space_map(iot, iobase,
		ea->ea_reg[0].size, 0, &ioh) != 0) {
		aprint_error(": can't map register space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	sc->sc_hwflags = 0;
	sc->sc_frequency = BAUD_BASE;

	for (i = 0; i < ea->ea_nintr; i++)
		bus_intr_establish(ea->ea_bustag, ea->ea_intr[i],
		    IPL_SERIAL, comintr, sc);

	kma.kmta_consdev = NULL;

	/*
	 * Figure out if we're the console.
	 *
	 * The Fujitsu SPARC Enterprise M4000/M5000/M8000/M9000 has a
	 * serial port on each I/O board and a pseudo console that is
	 * redirected to one of these serial ports.  The board number
	 * of the serial port in question is encoded in the "tty-port#"
	 * property of the pseudo console, so we figure out what our
	 * board number is by walking up the device tree, and check
	 * for a match.
	 */
	node = prom_instance_to_package(prom_stdin());
	com_is_input = (ea->ea_node == node);
	if (OF_getprop(node, "name", buf, sizeof(buf)) > 0 &&
	    strcmp(buf, "pseudo-console") == 0) {
		port = prom_getpropint(node, "tty-port#", -1);
		node = OF_parent(OF_parent(ea->ea_node));
		com_is_input = (prom_getpropint(node, "board#", -2) == port);
	}

	node = prom_instance_to_package(prom_stdout());
	com_is_output = (ea->ea_node == node);
	if (OF_getprop(node, "name", buf, sizeof(buf)) > 0 &&
	   strcmp(buf, "pseudo-console") == 0) {
		port = prom_getpropint(node, "tty-port#", -1);
		node = OF_parent(OF_parent(ea->ea_node));
		com_is_output = (prom_getpropint(node, "board#", -2) == port);
	}

	if (com_is_input || com_is_output) {
		struct consdev *cn_orig;

		/* Record some info to attach console. */
		if (strcmp(ea->ea_name, "rsc-console") == 0)
			kma.kmta_baud = 115200;
		else
			kma.kmta_baud = 9600;
		kma.kmta_cflag = (CREAD | CS8 | HUPCL);

		/* Attach com as the console. */
		cn_orig = cn_tab;
		if (comcnattach(iot, iobase, kma.kmta_baud,
			sc->sc_frequency, COM_TYPE_NORMAL, kma.kmta_cflag)) {
			aprint_error("Error: comcnattach failed\n");
		}
		if (com_is_input) {
			cn_orig->cn_dev = cn_tab->cn_dev;
			cn_orig->cn_probe = cn_tab->cn_probe;
			cn_orig->cn_init = cn_tab->cn_init;
			cn_orig->cn_getc = cn_tab->cn_getc;
			cn_orig->cn_pollc = cn_tab->cn_pollc;
		}
		if (com_is_output) {
			cn_orig->cn_putc = cn_tab->cn_putc;
		}
		cn_tab = cn_orig;
		kma.kmta_consdev = cn_tab;
	}

	/*
	 * Apparently shoving too much data down the TX FIFO on the
	 * Fujitsu SPARC Enterprise M4000/M5000 causes a hardware
	 * fault.  Avoid this issue by setting the FIFO depth to 1.
	 * This will effectively disable the TX FIFO, but will still
	 * enable the RX FIFO, which is what we really care about.
	 */
	if (OF_getprop(ea->ea_node, "compatible", buf, sizeof(buf)) > 0 &&
	    strcmp(buf, "FJSV,su") == 0)
		sc->sc_fifolen = 1;

	/* Now attach the driver */
	com_attach_subr(sc);

#if (NKBD > 0) || (NMS > 0)
	kma.kmta_tp = sc->sc_tty;
/* If we figure out we're the console we should point this to our consdev */

	/* locate the major number */
	maj = cdevsw_lookup_major(&com_cdevsw);

	kma.kmta_dev = makedev(maj, device_unit(sc->sc_dev));

/* Attach 'em if we got 'em. */
#if (NKBD > 0)
	kma.kmta_name = "keyboard";
	if (prom_getproplen(ea->ea_node, kma.kmta_name) == 0) {
		config_found(self, (void *)&kma, NULL);
	}
#endif
#if (NMS > 0)
	kma.kmta_name = "mouse";
	if (prom_getproplen(ea->ea_node, kma.kmta_name) == 0) {
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
