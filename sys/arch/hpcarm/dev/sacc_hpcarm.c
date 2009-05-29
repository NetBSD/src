/*      $NetBSD: sacc_hpcarm.c,v 1.11 2009/05/29 14:15:45 rjs Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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

/*
 * Platform dependent part for SA-11[01]1 companion chip on hpcarm.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sacc_hpcarm.c,v 1.11 2009/05/29 14:15:45 rjs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/uio.h>

#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>

static void	sacc_attach(device_t, device_t, void *);
static int	sacc_intr(void *);

struct platid_data sacc_platid_table[] = {
	{ &platid_mask_MACH_HP_JORNADA_7XX, (void *)1 },
	{ NULL, NULL }
};

CFATTACH_DECL_NEW(sacc, sizeof(struct sacc_softc),
    sacc_probe, sacc_attach, NULL, NULL);

#ifdef INTR_DEBUG
#define DPRINTF(arg)	aprint_normal arg
#else
#define DPRINTF(arg)
#endif

static void
sacc_attach(device_t parent, device_t self, void *aux)
{
	int i, gpiopin;
	uint32_t skid;
	struct sacc_softc *sc = device_private(self);
	struct sa11x0_softc *psc = device_private(parent);
	struct sa11x0_attach_args *sa = aux;
	struct platid_data *p;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;
	sc->sc_piot = psc->sc_iot;
	sc->sc_gpioh = psc->sc_gpioh;

	p = platid_search_data(&platid, sacc_platid_table);
	if (p == NULL)
		return;

	gpiopin = (int)p->data;
	sc->sc_gpiomask = 1 << gpiopin;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh)) {
		aprint_normal_dev(self, "unable to map registers\n");
		return;
	}

	skid = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCSBI_SKID);

	aprint_normal_dev(self, "SA-1111 rev %d.%d\n",
			  (skid & 0xf0) >> 4, skid & 0xf);

	for (i = 0; i < SACCIC_LEN; i++)
		sc->sc_intrhand[i] = NULL;

	/* initialize SA-1111 interrupt controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTTSTSEL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR0, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR1, 0xffffffff);

	/* connect to SA-1110's GPIO intr */
	sa11x0_intr_establish(0, gpiopin, 1, IPL_SERIAL, sacc_intr, sc);

	/* attach each devices */
	config_search_ia(sa1111_search, self, "sacc", NULL);
}

static int
sacc_intr(void *arg)
{
	int i;
	uint32_t mask;
	struct sacc_intrvec intstat;
	struct sacc_softc *sc = arg;
	struct sacc_intrhand *ih;

	intstat.lo =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR0);
	intstat.hi =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR1);
	DPRINTF(("sacc_intr: %x %x\n", intstat.lo, intstat.hi));

	/* clear SA-1110's GPIO intr status */
	bus_space_write_4(sc->sc_piot, sc->sc_gpioh,
			  SAGPIO_EDR, sc->sc_gpiomask);

	for (i = 0, mask = 1; i < 32; i++, mask <<= 1)
		if (intstat.lo & mask) {
			/*
			 * Clear intr status before calling intr handlers.
			 * This cause stray interrupts, but clearing
			 * after calling intr handlers cause intr lossage.
			 */
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR0, 1 << i);

			for (ih = sc->sc_intrhand[i]; ih; ih = ih->ih_next)
				softint_schedule(ih->ih_soft);
		}
	for (i = 0, mask = 1; i < SACCIC_LEN - 32; i++, mask <<= 1)
		if (intstat.hi & mask) {
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR1, 1 << i);
			for (ih = sc->sc_intrhand[i + 32]; ih; ih = ih->ih_next)
				softint_schedule(ih->ih_soft);
		}
	return 1;
}
