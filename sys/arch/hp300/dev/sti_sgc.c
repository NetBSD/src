/*	$NetBSD: sti_sgc.c,v 1.1.2.2 2013/01/23 00:05:47 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.1.2.2 2013/01/23 00:05:47 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsdisplayvar.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

#include <hp300/dev/sgcvar.h>

static int sticonslot;

static int sti_sgc_match(device_t, struct cfdata *, void *);
static void sti_sgc_attach(device_t, device_t, void *);

static int sti_sgc_probe(bus_space_tag_t, int);
static void sti_sgc_end_attach(device_t);

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
	bus_space_tag_t iot = saa->saa_iot;
	bus_space_handle_t ioh, romh;
	bus_addr_t pa = (bus_addr_t)sgc_slottopa(saa->saa_slot);
	u_int romend;
	int i;

	/* XXX: temporalily map before obtain romend. */
#define STI_ROMSIZE_SAFE	(sizeof(struct sti_dd) * 4)
	if (bus_space_map(iot, pa, STI_ROMSIZE_SAFE, 0, &ioh)) {
		aprint_error(": can't map ROM");
		return;
	}
	romend = sti_rom_size(iot, ioh);
	bus_space_unmap(iot, ioh, STI_ROMSIZE_SAFE);

	sc->sc_dev = self;
	sc->sc_enable_rom = NULL;
	sc->sc_disable_rom = NULL;

	if (bus_space_map(iot, pa, romend, 0, &romh)) {
		aprint_error(": can't map ROM(2)");
		return;
	}
	sc->bases[0] = romh;
	for (i = 0; i < STI_REGION_MAX; i++)
		sc->bases[i] = pa;
	if (saa->saa_slot == sticonslot)
		sc->sc_flags |= STI_CONSOLE;
	if (sti_attach_common(sc, iot, iot, romh, STI_CODEBASE_ALT) == 0)
		config_interrupts(self, sti_sgc_end_attach);
}

static int
sti_sgc_probe(bus_space_tag_t iot, int slot)
{
	bus_space_handle_t ioh;
	bus_addr_t pa = (bus_addr_t)sgc_slottopa(slot);
	int devtype;

	if (bus_space_map(iot, pa, PAGE_SIZE, 0, &ioh))
		return 0;
	devtype = bus_space_read_1(iot, ioh, 3);
	bus_space_unmap(iot, ioh, PAGE_SIZE);

	/*
	 * This might not be reliable enough. On the other hand, non-STI
	 * SGC cards will apparently not initialize in an hp300, to the
	 * point of not even answering bus probes (checked with an
	 * Harmony/FDDI SGC card).
	 */
	if (devtype != STI_DEVTYPE1 &&
	    devtype != STI_DEVTYPE4)
		return 0;
	return 1;
}

static void
sti_sgc_end_attach(device_t self)
{
	struct sti_softc *sc = device_private(self);

	sti_end_attach(sc);
}
