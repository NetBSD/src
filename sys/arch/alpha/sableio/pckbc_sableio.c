/* $NetBSD: pckbc_sableio.c,v 1.12 2014/03/29 19:28:25 christos Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pckbc_sableio.c,v 1.12 2014/03/29 19:28:25 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <dev/pci/pcivar.h>
#include <dev/isa/isavar.h>

#include <alpha/sableio/sableiovar.h>

struct pckbc_sableio_softc {
	struct	pckbc_softc sc_pckbc;	/* real "pckbc" softc */

	/* STDIO-specific goo. */
	void	*sc_ih[PCKBC_NSLOTS];	/* interrupt handlers */
	int	sc_irq[PCKBC_NSLOTS];	/* Sable IRQs to use */
	pci_chipset_tag_t sc_pc;	/* PCI chipset for registering intrs */
};

int	pckbc_sableio_match(device_t, cfdata_t, void *);
void	pckbc_sableio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pckbc_sableio, sizeof(struct pckbc_sableio_softc),
    pckbc_sableio_match, pckbc_sableio_attach, NULL, NULL);

void	pckbc_sableio_intr_establish(struct pckbc_softc *, pckbc_slot_t);

int
pckbc_sableio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sableio_attach_args *sa = aux;

	/* Always present. */
	if (strcmp(sa->sa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
pckbc_sableio_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_sableio_softc *ssc = device_private(self);
	struct pckbc_softc *sc = &ssc->sc_pckbc;
	struct sableio_attach_args *sa = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;

	sc->sc_dv = self;
	ssc->sc_pc = sa->sa_pc;

	/*
	 * Set up IRQs.
	 */
	ssc->sc_irq[PCKBC_KBD_SLOT] = sa->sa_sableirq[0];
	ssc->sc_irq[PCKBC_AUX_SLOT] = sa->sa_sableirq[1];

	sc->intr_establish = pckbc_sableio_intr_establish;

	if (pckbc_is_console(sa->sa_iot, sa->sa_ioaddr)) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		if (bus_space_map(sa->sa_iot, sa->sa_ioaddr + KBDATAP,
				  1, 0, &ioh_d) != 0 ||
		    bus_space_map(sa->sa_iot, sa->sa_ioaddr + KBCMDP,
				  1, 0, &ioh_c) != 0)
			panic("pckbc_sableio_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK);
		memset(t, 0, sizeof(struct pckbc_internal));
		t->t_iot = sa->sa_iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = sa->sa_ioaddr;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
	}

	t->t_sc = sc;
	sc->id = t;

	aprint_normal("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

void
pckbc_sableio_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_sableio_softc *ssc = (void *) sc;
	const char *intrstr;
	char buf[PCI_INTRSTR_LEN];

	intrstr = pci_intr_string(ssc->sc_pc, ssc->sc_irq[slot], buf,
	    sizeof(buf));
	ssc->sc_ih[slot] = pci_intr_establish(ssc->sc_pc, ssc->sc_irq[slot],
	    IPL_TTY, pckbcintr, sc);
	if (ssc->sc_ih[slot] == NULL) {
		aprint_error_dev(sc->sc_dv,
		    "unable to establish interrupt for %s slot",
		    pckbc_slot_names[slot]);
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(sc->sc_dv, "%s slot interrupting at %s\n",
	    pckbc_slot_names[slot], intrstr);
}
