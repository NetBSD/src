/*	$NetBSD: sgc.c,v 1.2.6.2 2011/06/06 09:05:36 jruoho Exp $	*/
/*	$OpenBSD: sgc.c,v 1.6 2010/04/15 20:35:21 miod Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * SGC bus attachment and mapping glue.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>
#include <machine/hp300spu.h>

#include <hp300/dev/sgcreg.h>
#include <hp300/dev/sgcvar.h>

#ifdef SGC_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	do {} while (/* CONSTCOND */ 0)
#endif

struct sgc_softc {
	device_t sc_dev;
	struct bus_space_tag sc_tag;
};

static int	sgcmatch(device_t, cfdata_t, void *);
static void	sgcattach(device_t, device_t, void *);
static int	sgcprint(void *, const char *);

CFATTACH_DECL_NEW(sgc, sizeof(struct sgc_softc),
    sgcmatch, sgcattach, NULL, NULL);

int
sgcmatch(device_t parent, cfdata_t cf, void *aux)
{
	static int sgc_matched = 0;

	/* Allow only one instance. */
	if (sgc_matched)
		return 0;

	/*
	 * Leave out machines which can not have an SGC bus.
	 */

	switch (machineid) {
#if 0
	case HP_362:
	case HP_382:
#endif
	case HP_400:
	case HP_425:
	case HP_433:
		return sgc_matched = 1;
	default:
		return 0;
	}
}

void
sgcattach(device_t parent, device_t self, void *aux)
{
	struct sgc_softc *sc;
	struct sgc_attach_args saa;
	paddr_t pa;
	void *va;
	int slot, rv;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	sc = device_private(self);
	sc->sc_dev = self;
	aprint_normal("\n");

	bst = &sc->sc_tag;
	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_SGC;

	for (slot = 0; slot < SGC_NSLOTS; slot++) {
		pa = sgc_slottopa(slot);
		if (bus_space_map(bst, pa, PAGE_SIZE, 0, &bsh) != 0) {
			aprint_error_dev(self, "can't map slot %d\n", slot);
			continue;
		}
		va = bus_space_vaddr(bst, bsh);

		/* Check for hardware. */
		rv = badaddr(va);
		bus_space_unmap(bst, bsh, PAGE_SIZE);

		if (rv != 0) {
			DPRINTF(("%s: no valid device at slot %d\n",
			    device_xname(self), slot));
			continue;
		}

		memset(&saa, 0, sizeof(saa));
		saa.saa_iot = bst;
		saa.saa_slot = slot;

		/* Attach matching device. */
		config_found(self, &saa, sgcprint);
	}
}

int
sgcprint(void *aux, const char *pnp)
{
	struct sgc_attach_args *saa = aux;

	if (pnp)
		aprint_normal("unknown SGC card at %s", pnp);
	aprint_normal(" slot %d", saa->saa_slot);
	return UNCONF;
}

/*
 * Convert a slot number to a system physical address.
 * This is needed for bus_space.
 */
paddr_t
sgc_slottopa(int slot)
{
	paddr_t rval;

	if (slot < 0 || slot >= SGC_NSLOTS)
		rval = 0;
	else
		rval = SGC_BASE + (slot * SGC_DEVSIZE);

	return rval;
}
