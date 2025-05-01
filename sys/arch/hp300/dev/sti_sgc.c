/*	$NetBSD: sti_sgc.c,v 1.9 2025/05/01 06:11:21 tsutsui Exp $	*/
/*	$OpenBSD: sti_sgc.c,v 1.14 2007/05/26 00:36:03 krw Exp $	*/

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.9 2025/05/01 06:11:21 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <hp300/dev/sgcvar.h>
#include <hp300/dev/sti_sgcvar.h>
#include <hp300/dev/sti_machdep.h>

static int sticonslot = -1;

static int sti_sgc_match(device_t, struct cfdata *, void *);
static void sti_sgc_attach(device_t, device_t, void *);

static int sti_sgc_probe(bus_space_tag_t, int);

CFATTACH_DECL_NEW(sti_sgc, sizeof(struct sti_machdep_softc),
    sti_sgc_match, sti_sgc_attach, NULL, NULL);

static int
sti_sgc_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct sgc_attach_args *saa = aux;

	/*
	 * If we already probed it successfully as a console device, go ahead,
	 * since we will not be able to bus_space_map() again.
	 */
	if (saa->saa_slot == sticonslot)
		return 1;

	return sti_sgc_probe(saa->saa_iot, saa->saa_slot);
}

static void
sti_sgc_attach(device_t parent, device_t self, void *aux)
{
	struct sti_machdep_softc *sc = device_private(self);
	struct sti_softc *ssc = &sc->sc_sti;
	struct sgc_attach_args *saa = aux;
	bus_space_tag_t bst;
	bus_space_handle_t romh;
	bus_addr_t base;
	u_int romend;
	int i;

	ssc->sc_dev = self;
	bst = saa->saa_iot;
	base = (bus_addr_t)sgc_slottopa(saa->saa_slot);
	sc->sc_base = base;

	if (saa->saa_slot == sticonslot) {
		sti_machdep_attach_console(sc);
	} else {
		if (bus_space_map(bst, base, PAGE_SIZE, 0, &romh)) {
			aprint_error(": can't map ROM");
			return;
		}
		/*
		 * Compute real PROM size
		 */
		romend = sti_rom_size(bst, romh);

		bus_space_unmap(bst, romh, PAGE_SIZE);

		if (bus_space_map(bst, base, romend, 0, &romh)) {
			aprint_error(": can't map frame buffer");
			return;
		}

		ssc->bases[0] = romh;
		for (i = 0; i < STI_REGION_MAX; i++)
			ssc->bases[i] = base;

		if (sti_attach_common(ssc, bst, bst, romh,
		    STI_CODEBASE_ALT) != 0)
			return;
	}

	sti_machdep_attach(sc);
}

static int
sti_sgc_probe(bus_space_tag_t iot, int slot)
{
	bus_space_handle_t ioh;
	int devtype;

	if (bus_space_map(iot, (bus_addr_t)sgc_slottopa(slot),
	    PAGE_SIZE, 0, &ioh))
		return 0;

	devtype = bus_space_read_1(iot, ioh, 3);

	bus_space_unmap(iot, ioh, PAGE_SIZE);

	/*
	 * This might not be reliable enough. On the other hand, non-STI
	 * SGC cards will apparently not initialize in the hp300, to the
	 * point of not even answering bus probes (checked with an
	 * Harmony/FDDI SGC card).
	 */
	if (devtype != STI_DEVTYPE1 && devtype != STI_DEVTYPE4)
		return 0;

	return 1;
}

int
sti_sgc_cnprobe(bus_space_tag_t bst, int slot)
{
	void *va;
	bus_space_handle_t romh;
	bus_addr_t base;
	int devtype, rv = 0;

	base = sgc_slottopa(slot);
	if (bus_space_map(bst, base, PAGE_SIZE, 0, &romh))
		return 0;

	va = bus_space_vaddr(bst, romh);
	if (badaddr(va))
		goto out;

	devtype = bus_space_read_1(bst, romh, 3);
	if (devtype == STI_DEVTYPE1 || devtype == STI_DEVTYPE4)
		rv = 1;

 out:
	bus_space_unmap(bst, romh, PAGE_SIZE);
	return rv;
}

void
sti_sgc_cnattach(bus_space_tag_t bst, int slot)
{
	paddr_t base;

	base = sgc_slottopa(slot);

	sti_machdep_cnattach(bst, base);

	sticonslot = slot;
}
