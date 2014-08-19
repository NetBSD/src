/*	$NetBSD: sti_sgc.c,v 1.1.6.3 2014/08/20 00:03:00 tls Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.1.6.3 2014/08/20 00:03:00 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsdisplayvar.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

#include <hp300/dev/sgcvar.h>
#include <hp300/dev/sti_sgcvar.h>
#include <machine/autoconf.h>

static int sticonslot = -1;
static struct sti_rom sticn_rom;
static struct sti_screen sticn_scr;
static bus_addr_t sticn_bases[STI_REGION_MAX];

static int sti_sgc_match(device_t, struct cfdata *, void *);
static void sti_sgc_attach(device_t, device_t, void *);

static int sti_sgc_probe(bus_space_tag_t, int);

CFATTACH_DECL_NEW(sti_sgc, sizeof(struct sti_softc),
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
	struct sti_softc *sc = device_private(self);
	struct sgc_attach_args *saa = aux;
	bus_space_handle_t romh;
	bus_addr_t base;
	u_int romend;
	int i;

	sc->sc_dev = self;

	if (saa->saa_slot == sticonslot) {
		sc->sc_flags |= STI_CONSOLE | STI_ATTACHED;
		sc->sc_rom = &sticn_rom;
		sc->sc_scr = &sticn_scr;
		memcpy(sc->bases, sticn_bases, sizeof(sc->bases));

		sti_describe(sc);
	} else {
		base = (bus_addr_t)sgc_slottopa(saa->saa_slot);
		if (bus_space_map(saa->saa_iot, base, PAGE_SIZE, 0, &romh)) {
			aprint_error(": can't map ROM");
			return;
		}
		/*
		 * Compute real PROM size
		 */
		romend = sti_rom_size(saa->saa_iot, romh);

		bus_space_unmap(saa->saa_iot, romh, PAGE_SIZE);

		if (bus_space_map(saa->saa_iot, base, romend, 0, &romh)) {
			aprint_error(": can't map frame buffer");
			return;
		}

		sc->bases[0] = romh;
		for (i = 0; i < STI_REGION_MAX; i++)
			sc->bases[i] = base;

		if (sti_attach_common(sc, saa->saa_iot, saa->saa_iot, romh,
		    STI_CODEBASE_ALT) != 0)
			return;
	}

	/*
	 * Note on 425e sti(4) framebuffer bitmap memory can be accessed at
	 * (sgc_slottopa(saa->saa_slot) + 0x200000)
	 * but the mmap function to map bitmap display is not provided yet.
	 */

	sti_end_attach(sc);
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
	 * SGC cards will apparently not initialize in an hp300, to the
	 * point of not even answering bus probes (checked with an
	 * Harmony/FDDI SGC card).
	 */
	if (devtype != STI_DEVTYPE1 && devtype != STI_DEVTYPE4)
		return 0;

	return 1;
}

int
sti_sgc_cnprobe(bus_space_tag_t bst, bus_addr_t addr, int slot)
{
	void *va;
	bus_space_handle_t romh;
	int devtype, rv = 0;

	if (bus_space_map(bst, addr, PAGE_SIZE, 0, &romh))
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
sti_sgc_cnattach(bus_space_tag_t bst, bus_addr_t addr, int slot)
{
	int i;

	/* sticn_bases[0] will be fixed in sti_cnattach() */
	for (i = 0; i < STI_REGION_MAX; i++)
		sticn_bases[i] = addr;

	sti_cnattach(&sticn_rom, &sticn_scr, bst, sticn_bases,
	    STI_CODEBASE_ALT);

	sticonslot = slot;
}
