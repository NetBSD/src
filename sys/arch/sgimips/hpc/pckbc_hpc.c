/* $NetBSD: pckbc_hpc.c,v 1.9.68.1 2015/04/06 15:18:01 skrll Exp $	 */

/*
 * Copyright (c) 2003 Christopher SEKIYA
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pckbc_hpc.c,v 1.9.68.1 2015/04/06 15:18:01 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/bus.h>

#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/hpcvar.h>

struct pckbc_hpc_softc {
	struct pckbc_softc sc_pckbc;

	int	sc_irq;
	int	sc_hasintr;
};

static int      pckbc_hpc_match(device_t, cfdata_t, void *);
static void     pckbc_hpc_attach(device_t, device_t, void *);
static void     pckbc_hpc_intr_establish(struct pckbc_softc *, pckbc_slot_t);

CFATTACH_DECL_NEW(pckbc_hpc, sizeof(struct pckbc_hpc_softc),
	      pckbc_hpc_match, pckbc_hpc_attach, NULL, NULL);

static int
pckbc_hpc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (strcmp(ha->ha_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
pckbc_hpc_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_hpc_softc *msc = device_private(self);
	struct pckbc_softc *sc = &msc->sc_pckbc;
	struct hpc_attach_args *haa = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;

	sc->sc_dv = self;

	msc->sc_irq = haa->ha_irq;

	msc->sc_hasintr = 0;

	sc->intr_establish = pckbc_hpc_intr_establish;

	/* XXX Ugly hack & kludge XXX */
	if (pckbc_is_console(haa->ha_st, MIPS_KSEG1_TO_PHYS(haa->ha_sh +
	    haa->ha_devoff))) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
	} else {
		/* XXX should be bus_space_map() */
		if (bus_space_subregion(haa->ha_st, haa->ha_sh,
					haa->ha_devoff + KBDATAP, 1, &ioh_d) ||
		    bus_space_subregion(haa->ha_st, haa->ha_sh,
					haa->ha_devoff + KBCMDP, 1, &ioh_c))
			panic("pckbc_hpc_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF,
		    M_WAITOK | M_ZERO);
		t->t_iot = hpc_memt;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = haa->ha_sh;
		t->t_cmdbyte = KC8_CPU;	/* Enable ports */
		callout_init(&t->t_cleanup, 0);
	}

	t->t_sc = sc;
	sc->id = t;

	aprint_normal("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

static void
pckbc_hpc_intr_establish(struct pckbc_softc * sc, pckbc_slot_t slot)
{
	struct pckbc_hpc_softc *msc = (void *) sc;

	if (msc->sc_hasintr)
		return;

	if (cpu_intr_establish(msc->sc_irq, IPL_TTY, pckbcintr, sc) == NULL) {
		aprint_error_dev(sc->sc_dv,
		    "unable to establish interrupt for %s slot\n",
		    pckbc_slot_names[slot]);
	} else {
		msc->sc_hasintr = 1;
	}
}
