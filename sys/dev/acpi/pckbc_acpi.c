/*	$NetBSD: pckbc_acpi.c,v 1.16.6.1 2006/04/22 11:38:46 simonb Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * ACPI attachment for the PC Keyboard Controller driver.
 *
 * This is a little wonky.  The keyboard controller actually
 * has 2 ACPI nodes: one for the controller and the keyboard
 * interrupt, and one for the aux port (mouse) interrupt.
 *
 * For this reason, we actually attach *two* instances of this
 * driver.  After both of them have been found, then we attach
 * sub-devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pckbc_acpi.c,v 1.16.6.1 2006/04/22 11:38:46 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <dev/acpi/acpivar.h>

static int	pckbc_acpi_match(struct device *, struct cfdata *, void *);
static void	pckbc_acpi_attach(struct device *, struct device *, void *);

struct pckbc_acpi_softc {
	struct pckbc_softc sc_pckbc;

	isa_chipset_tag_t sc_ic;
	int sc_irq;
	int sc_ist;
	pckbc_slot_t sc_slot;
};

/* Save first port: */
static struct pckbc_acpi_softc *first;

extern struct cfdriver pckbc_cd;

CFATTACH_DECL(pckbc_acpi, sizeof(struct pckbc_acpi_softc),
    pckbc_acpi_match, pckbc_acpi_attach, NULL, NULL);

static void	pckbc_acpi_intr_establish(struct pckbc_softc *, pckbc_slot_t);

/*
 * Supported Device IDs
 */

static const char * const pckbc_acpi_ids_kbd[] = {
	"PNP03??",	/* Standard PC KBD port */
	NULL
};

static const char * const pckbc_acpi_ids_ms[] = {
	"PNP0F03",
	"PNP0F0E",
	"PNP0F12",
	"PNP0F13",
	"PNP0F19",
	"PNP0F1B",
	"PNP0F1C",
	NULL
};

/*
 * pckbc_acpi_match: autoconf(9) match routine
 */
static int
pckbc_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	int rv;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	rv = acpi_match_hid(aa->aa_node->ad_devinfo, pckbc_acpi_ids_kbd);
	if (rv)
		return rv;
	rv = acpi_match_hid(aa->aa_node->ad_devinfo, pckbc_acpi_ids_ms);
	if (rv)
		return rv;
	return 0;
}

static void
pckbc_acpi_attach(struct device *parent,
    struct device *self,
    void *aux)
{
	struct pckbc_acpi_softc *psc = (void *) self;
	struct pckbc_softc *sc = &psc->sc_pckbc;
	struct pckbc_internal *t;
	struct acpi_attach_args *aa = aux;
	bus_space_handle_t ioh_d, ioh_c;
	pckbc_slot_t peer;
	struct acpi_resources res;
	struct acpi_io *io0, *io1;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	psc->sc_ic = aa->aa_ic;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, pckbc_acpi_ids_kbd)) {
		psc->sc_slot = PCKBC_KBD_SLOT;
		peer = PCKBC_AUX_SLOT;
	} else if (acpi_match_hid(aa->aa_node->ad_devinfo, pckbc_acpi_ids_ms)) {
		psc->sc_slot = PCKBC_AUX_SLOT;
		peer = PCKBC_KBD_SLOT;
	} else {
		printf(": unknown port!\n");
		panic("pckbc_acpi_attach: impossible");
	}

	aprint_naive("\n");
	aprint_normal(": %s port\n", pckbc_slot_names[psc->sc_slot]);

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dv, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error("%s: unable to find irq resource\n", sc->sc_dv.dv_xname);
		goto out;
	}
	psc->sc_irq = irq->ar_irq;
	psc->sc_ist = (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL;

	if (psc->sc_slot == PCKBC_KBD_SLOT)
		first = psc;

	if ((!first || !first->sc_pckbc.id) &&
	    (psc->sc_slot == PCKBC_KBD_SLOT)) {

		io0 = acpi_res_io(&res, 0);
		if (io0 == NULL) {
			aprint_error("%s: unable to find i/o resources\n",
			    sc->sc_dv.dv_xname);
			goto out;
		}

		if (pckbc_is_console(aa->aa_iot, io0->ar_base)) {
			t = &pckbc_consdata;
			ioh_d = t->t_ioh_d;
			ioh_c = t->t_ioh_c;
			pckbc_console_attached = 1;
			/* t->t_cmdbyte was initialized by cnattach */
		} else {
			io1 = acpi_res_io(&res, 1);
			if (io1 == NULL) {
				aprint_error("%s: unable to find i/o resources\n",
				    sc->sc_dv.dv_xname);
				goto out;
			}
			if (bus_space_map(aa->aa_iot, io0->ar_base,
					  io0->ar_length, 0, &ioh_d) ||
			    bus_space_map(aa->aa_iot, io1->ar_base,
					  io1->ar_length, 0, &ioh_c))
				panic("pckbc_acpi_attach: couldn't map");

			t = malloc(sizeof(struct pckbc_internal),
			    M_DEVBUF, M_WAITOK);
			memset(t, 0, sizeof(*t));
			t->t_iot = aa->aa_iot;
			t->t_ioh_d = ioh_d;
			t->t_ioh_c = ioh_c;
			t->t_addr = io0->ar_base;
			t->t_cmdbyte = KC8_CPU;	/* Enable ports */
			callout_init(&t->t_cleanup);
		}

		t->t_sc = &first->sc_pckbc;
		first->sc_pckbc.id = t;

		first->sc_pckbc.intr_establish = pckbc_acpi_intr_establish;
		config_defer(&first->sc_pckbc.sc_dv,
			     (void(*)(struct device *))pckbc_attach);
	}
 out:
	acpi_resource_cleanup(&res);
}

static void
pckbc_acpi_intr_establish(struct pckbc_softc *sc,
    pckbc_slot_t slot)
{
	struct pckbc_acpi_softc *psc;
	isa_chipset_tag_t ic = NULL;
	void *rv = NULL;
	int irq = 0, ist = 0; /* XXX: gcc */
	int i;

	/*
	 * Note we're always called with sc == first.
	 */
	for (i = 0; i < pckbc_cd.cd_ndevs; i++) {
		psc = pckbc_cd.cd_devs[i];
		if (psc && psc->sc_slot == slot) {
			irq = psc->sc_irq;
			ist = psc->sc_ist;
			ic = psc->sc_ic;
			break;
		}
	}
	if (i < pckbc_cd.cd_ndevs)
		rv = isa_intr_establish(ic, irq, ist, IPL_TTY, pckbcintr, sc);
	if (rv == NULL) {
		aprint_error("%s: unable to establish interrupt for %s slot\n",
		    sc->sc_dv.dv_xname, pckbc_slot_names[slot]);
	} else {
		aprint_normal("%s: using irq %d for %s slot\n", sc->sc_dv.dv_xname,
		    irq, pckbc_slot_names[slot]);
	}
}
