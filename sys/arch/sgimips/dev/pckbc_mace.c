/*	$NetBSD: pckbc_mace.c,v 1.7 2003/10/05 15:38:08 tsutsui Exp $	*/

/*
 * Copyright (c) 2003 Christopher SEKIYA
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          NetBSD Project.  See http://www.netbsd.org/ for
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
__KERNEL_RCSID(0, "$NetBSD: pckbc_mace.c,v 1.7 2003/10/05 15:38:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/dev/macevar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

struct pckbc_mace_softc {
	struct pckbc_softc sc_pckbc;

	int sc_irq[PCKBC_NSLOTS];
};

static int	pckbc_mace_match(struct device *, struct cfdata *, void *);
static void	pckbc_mace_attach(struct device *, struct device *, void *);
void pckbc_mace_intr_establish(struct pckbc_softc *, pckbc_slot_t);

CFATTACH_DECL(pckbc_mace, sizeof(struct pckbc_mace_softc),
    pckbc_mace_match, pckbc_mace_attach, NULL, NULL);

static int
pckbc_mace_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	if (mach_type == MACH_SGI_IP32)
		return (1);

	return (0);
}

static void
pckbc_mace_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pckbc_mace_softc *msc = (void *)self;
	struct pckbc_softc *sc = &msc->sc_pckbc;
	struct mace_attach_args *maa = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;

	msc->sc_irq[PCKBC_KBD_SLOT] =
	    msc->sc_irq[PCKBC_AUX_SLOT] = maa->maa_intr;

	sc->intr_establish = pckbc_mace_intr_establish;

	/* XXX should be bus_space_map() */
	if (bus_space_subregion(maa->maa_st, maa->maa_sh,
	    maa->maa_offset, 1, &ioh_d) ||
	    bus_space_subregion(maa->maa_st, maa->maa_sh,
	    maa->maa_offset + 0x20, 1, &ioh_c))
		panic("pckbc_attach: couldn't map");

	t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK|M_ZERO);
	t->t_iot = maa->maa_st;
	t->t_ioh_d = ioh_d;
	t->t_ioh_c = ioh_c;
	t->t_addr = maa->maa_sh;
	t->t_cmdbyte = KC8_CPU; /* Enable ports */
	callout_init(&t->t_cleanup);

	t->t_sc = sc;
	sc->id = t;

	printf("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

/* XXX */

/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{

	return (ENXIO);
}

void
pckbc_mace_intr_establish(sc, slot)
	struct pckbc_softc *sc;
	pckbc_slot_t slot;
{

	mace_intr_establish(5, 0, pckbcintr, sc);
}
