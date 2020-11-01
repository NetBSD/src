/*	$NetBSD: pckbc_js.c,v 1.19 2012/10/13 17:58:54 jdc Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pckbc_js.c,v 1.19 2012/10/13 17:58:54 jdc Exp $");

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

struct pckbc_js_softc {
	struct pckbc_softc jsc_pckbc;	/* real "pckbc" softc */

	/* kbd and mouse share interrupt in both mr.coffee and krups */
	uint32_t jsc_intr;
	int jsc_establised;
	void *jsc_int_cookie;
};


static int	pckbc_obio_match(device_t, cfdata_t, void *);
static void	pckbc_obio_attach(device_t, device_t, void *);

static int	pckbc_ebus_match(device_t, cfdata_t, void *);
static void	pckbc_ebus_attach(device_t, device_t, void *);

static void	pckbc_js_attach_common(struct pckbc_js_softc *,
				       bus_space_tag_t, bus_addr_t, int, int);
static void	pckbc_js_intr_establish(struct pckbc_softc *, pckbport_slot_t);
static int	jsc_pckbdintr(void *);

/* Mr.Coffee */
CFATTACH_DECL_NEW(pckbc_obio, sizeof(struct pckbc_js_softc),
    pckbc_obio_match, pckbc_obio_attach, NULL, NULL);

/* ms-IIep */
CFATTACH_DECL_NEW(pckbc_ebus, sizeof(struct pckbc_js_softc),
    pckbc_ebus_match, pckbc_ebus_attach, NULL, NULL);

#define PCKBC_PROM_DEVICE_NAME "8042"

static int
pckbc_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	return (strcmp(sa->sa_name, PCKBC_PROM_DEVICE_NAME) == 0);
}

static int
pckbc_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(ea->ea_name, PCKBC_PROM_DEVICE_NAME) == 0);
}


static void
pckbc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_js_softc *jsc = device_private(self);
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	bus_space_tag_t iot;
	bus_addr_t ioaddr;
	int intr, isconsole;

	jsc->jsc_pckbc.sc_dv = self;
	iot = sa->sa_bustag;
	ioaddr = BUS_ADDR(sa->sa_slot, sa->sa_offset);
	intr = sa->sa_nintr ? sa->sa_pri : /* level */ 13;

	/*
	 * TODO:
	 * . on OFW machines stdin is keyboard node, not 8042 node
	 * . on OBP machines 8042 node is faked by boot's prompatch
	 *   and PROM's stdin points to zs keyboard (add prom patch?)
	 * . we probably want the stdout node to control whether
	 *   console goes to serial
	 */
	isconsole = 1;

	pckbc_js_attach_common(jsc, iot, ioaddr, intr, isconsole);
}

static void
pckbc_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_js_softc *jsc = device_private(self);
	struct ebus_attach_args *ea = aux;
	bus_space_tag_t iot;
	bus_addr_t ioaddr;
	int intr;
	int stdin_node,	node;
	int isconsole;

	jsc->jsc_pckbc.sc_dv = self;
	iot = ea->ea_bustag;
	ioaddr = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);
	intr = ea->ea_nintr ? ea->ea_intr[0] : /* line */ 0;

	/* search children of "8042" node for stdin (keyboard) */
	stdin_node = prom_instance_to_package(prom_stdin());
	isconsole = 0;

	for (node = prom_firstchild(ea->ea_node);
	     node != 0; node = prom_nextsibling(node))
		if (node == stdin_node) {
			isconsole = 1;
			break;
		}

	pckbc_js_attach_common(jsc, iot, ioaddr, intr, isconsole);
}


static void
pckbc_js_attach_common(struct pckbc_js_softc *jsc,
		       bus_space_tag_t iot, bus_addr_t ioaddr, int intr,
		       int isconsole)
{
	struct pckbc_softc *sc = (struct pckbc_softc *)jsc;
	struct pckbc_internal *t;

	jsc->jsc_pckbc.intr_establish = pckbc_js_intr_establish;
	jsc->jsc_intr = intr;
	jsc->jsc_establised = 0;

	if (isconsole) {
		int status;

		status = pckbc_cnattach(iot, ioaddr, KBCMDP, PCKBC_KBD_SLOT, 0);
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
	t->t_sc = sc;
	sc->id = t;

	/* finish off the attach */
	aprint_normal("\n");
	pckbc_attach(sc);
}


/*
 * Keyboard and mouse share the interrupt
 * so don't install interrupt handler twice.
 */
static void
pckbc_js_intr_establish(struct pckbc_softc *sc, pckbport_slot_t slot)
{
	struct pckbc_js_softc *jsc = (struct pckbc_js_softc *)sc;
	void *res;

	if (jsc->jsc_establised) {
#ifdef DEBUG
		aprint_verbose_dev(sc->sc_dv,
		    "%s slot shares interrupt (already established)\n",
		    pckbc_slot_names[slot]);
#endif
		return;
	}

	/*
	 * We can not choose the devic class interruptlevel freely,
	 * so we debounce via a softinterrupt.
	 */
	jsc->jsc_int_cookie = softint_establish(SOFTINT_SERIAL,
	    pckbcintr_soft, &jsc->jsc_pckbc);
	if (jsc->jsc_int_cookie == NULL) {
		aprint_error_dev(sc->sc_dv,
		    "unable to establish %s soft interrupt\n",
		    pckbc_slot_names[slot]);
		return;
	}
	res = bus_intr_establish(sc->id->t_iot, jsc->jsc_intr,
				 IPL_SERIAL, jsc_pckbdintr, jsc);
	if (res == NULL)
		aprint_error_dev(sc->sc_dv,
		    "unable to establish %s slot interrupt\n",
		    pckbc_slot_names[slot]);
	else
		jsc->jsc_establised = 1;
}

static int
jsc_pckbdintr(void *vsc)
{
	struct pckbc_js_softc *jsc = vsc;

	softint_schedule(jsc->jsc_int_cookie);
	pckbcintr_hard(&jsc->jsc_pckbc);

	/*
	 * This interrupt is not shared on javastations, avoid "stray"
	 * warnings. XXX - why do "stray interrupt" warnings happen if
	 * we don't claim the interrupt always?
	 */
	return 1;
}

/*
 * MD hook for use without MI wscons.
 * Called by pckbc_cnattach().
 */
int
pckbport_machdep_cnattach(pckbport_tag_t constag, pckbport_slot_t slot)
{

	return (0);
}
