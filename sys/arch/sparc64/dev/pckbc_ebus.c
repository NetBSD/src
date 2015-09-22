/*	$NetBSD: pckbc_ebus.c,v 1.1.18.1 2015/09/22 12:05:52 skrll Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pckbc_ebus.c,v 1.1.18.1 2015/09/22 12:05:52 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <machine/autoconf.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pckbport/pckbportvar.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

struct pckbc_ebus_softc {
	struct pckbc_softc psc_pckbc;	/* real "pckbc" softc */
	uint32_t psc_intr[PCKBC_NSLOTS];
};

static int	pckbc_ebus_match(device_t, cfdata_t, void *);
static void	pckbc_ebus_attach(device_t, device_t, void *);

static void	pckbc_ebus_intr_establish(struct pckbc_softc *, pckbport_slot_t);

#define PCKBC_PROM_DEVICE_NAME "8042"
#define PCKBC_PROM_DEVICE_NAME2 "kb_ps2"

CFATTACH_DECL_NEW(pckbc_ebus, sizeof(struct pckbc_ebus_softc),
    pckbc_ebus_match, pckbc_ebus_attach, NULL, NULL);


static int
pckbc_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	if (strcmp(ea->ea_name, PCKBC_PROM_DEVICE_NAME) == 0 ||
	    strcmp(ea->ea_name, PCKBC_PROM_DEVICE_NAME2) == 0)
		return 1;
	return 0;
}

static void
pckbc_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_ebus_softc *sc = device_private(self);
	struct pckbc_softc *psc = (struct pckbc_softc *) sc;
	struct ebus_attach_args *ea = aux;
	struct pckbc_internal *t;
	bus_space_tag_t iot;
	bus_addr_t ioaddr;
	int stdin_node, node;
	int isconsole, i;

	psc->sc_dv = self;
	iot = ea->ea_bustag;
	ioaddr = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);

	stdin_node = prom_instance_to_package(prom_stdin());
	isconsole = 0;
	for (node = prom_firstchild(ea->ea_node);
	     node != 0; node = prom_nextsibling(node))
		if (node == stdin_node) {
			isconsole = 1;
			break;
		}

	psc->intr_establish = pckbc_ebus_intr_establish;

	if (ea->ea_nintr < PCKBC_NSLOTS) {
		aprint_error(": no intr %d", ea->ea_nintr);

		/*
		 * XXX OpenBIOS doesn't provide interrupts for pckbc
		 * currently, so use the interrupt numbers described in
		 * QEMU's hw/sparc64/sun4u.c::isa_irq_handler.
		 */
		if (strcmp(machine_model, "OpenBiosTeam,OpenBIOS") == 0) {
			sc->psc_intr[PCKBC_KBD_SLOT] = 0x29;
			sc->psc_intr[PCKBC_AUX_SLOT] = 0x2a;
		} else {
			aprint_error("\n");
			return;
		}
	} else {
		for (i = 0; i < PCKBC_NSLOTS; i++)
			sc->psc_intr[i] = ea->ea_intr[i];
	}

	if (isconsole) {
		int status;

		status = pckbc_cnattach(iot, ioaddr, KBCMDP, 0,
		    PCKBC_NEED_AUXWRITE | PCKBC_CANT_TRANSLATE);
		if (status == 0)
			aprint_normal(": cnattach ok");
		else
			aprint_error(": cnattach %d", status);
	}

	if (pckbc_is_console(iot, ioaddr)) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
	} else {
		bus_space_handle_t ioh_d, ioh_c;

		if (bus_space_map(iot, ioaddr + KBDATAP, 1, 0, &ioh_d) != 0) {
			aprint_error(": unable to map data register\n");
			return;
		}

		if (bus_space_map(iot, ioaddr + KBCMDP,  1, 0, &ioh_c) != 0) {
			bus_space_unmap(iot, ioh_d, 1);
			aprint_error(": unable to map cmd register\n");
			return;
		}

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK);
		memset(t, 0, sizeof(struct pckbc_internal));
		t->t_iot = iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = ioaddr;
		t->t_cmdbyte = KC8_CPU; /* initial command: enable ports */
		callout_init(&t->t_cleanup, 0);

		(void) pckbc_poll_data1(t, PCKBC_KBD_SLOT); /* flush */

		if (pckbc_send_cmd(iot, ioh_c, KBC_SELFTEST) == 0)
			aprint_error(": unable to request self test");
		else {
			int response;

			response = pckbc_poll_data1(t, PCKBC_KBD_SLOT);
			if (response == 0x55)
				aprint_normal(": selftest ok");
			else
				aprint_error(": selftest failed (0x%02x)",
				    response);
		}
	}

	/* crosslink */
	t->t_sc = psc;
	psc->id = t;

	/* finish off the attach */
	aprint_normal("\n");
	pckbc_attach(psc);
}


static void
pckbc_ebus_intr_establish(struct pckbc_softc *sc, pckbport_slot_t slot)
{
	struct pckbc_ebus_softc *psc = (struct pckbc_ebus_softc *)sc;
	void *res;

	/* We assume that interrupt order is the same as slot order. */
	res = bus_intr_establish(sc->id->t_iot, psc->psc_intr[slot],
				 IPL_TTY, pckbcintr, sc);
	if (res == NULL)
		aprint_error_dev(sc->sc_dv,
		    "unable to establish %s slot interrupt\n",
		    pckbc_slot_names[slot]);

	return;
}
