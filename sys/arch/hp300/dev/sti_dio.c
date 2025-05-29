/*	$NetBSD: sti_dio.c,v 1.3 2025/05/26 12:25:12 tsutsui Exp $	*/
/*	$OpenBSD: sti_dio.c,v 1.1 2011/08/18 20:02:57 miod Exp $	*/

/*
 * Copyright (c) 2005, 2011, Miodrag Vallat
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
__KERNEL_RCSID(0, "$NetBSD: sti_dio.c,v 1.3 2025/05/26 12:25:12 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/autoconf.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/sti_diovar.h>
#include <hp300/dev/sti_machdep.h>

static int  sti_dio_match(device_t, cfdata_t, void *);
static void sti_dio_attach(device_t, device_t, void *);

static int sti_dio_probe(bus_space_tag_t, int);

CFATTACH_DECL_NEW(sti_dio, sizeof(struct sti_machdep_softc),
    sti_dio_match, sti_dio_attach, NULL, NULL);

static int
sti_dio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct dio_attach_args *da = aux;

	if (da->da_id != DIO_DEVICE_ID_FRAMEBUFFER ||
	    (da->da_secid != DIO_DEVICE_SECID_A1474MID &&
	     da->da_secid != DIO_DEVICE_SECID_A147xVGA))
		return 0;

	/*
	 * If we already probed it successfully as a console device, go ahead,
	 * since we will not be able to bus_space_map() again.
	 */
	if (da->da_scode == conscode ||
	    sti_dio_probe(da->da_bst, da->da_scode) != 0)
		return 10;	/* Beat old gendiofb(4) */

	return 0;
}

static void
sti_dio_attach(device_t parent, device_t self, void *aux)
{
	struct sti_machdep_softc *sc = device_private(self);
	struct sti_softc *ssc = &sc->sc_sti;
	struct dio_attach_args *da = aux;
	bus_addr_t base;
	bus_space_tag_t bst;
	bus_space_handle_t romh;
	u_int romend;
	int i;

	ssc->sc_dev = self;
	bst = da->da_bst;

	base = (paddr_t)dio_scodetopa(da->da_scode + STI_DIO_SCODE_OFFSET);
	sc->sc_base = base;

	/*
	 * If we already probed it successfully as a console device, go ahead,
	 * since we will not be able to bus_space_map() again.
	 */
	if (da->da_scode == conscode) {
		sti_machdep_attach_console(sc);
	} else {
		if (bus_space_map(bst, base, PAGE_SIZE, 0, &romh)) {
			aprint_error(": can't map frame buffer");
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
		for (i = 1; i < STI_REGION_MAX; i++)
			ssc->bases[i] = base;

		if (sti_attach_common(ssc, bst, bst, romh,
		    STI_CODEBASE_ALT) != 0)
			return;
	}

	sti_machdep_attach(sc);
}

static int
sti_dio_probe(bus_space_tag_t bst, int scode)
{
	bus_space_handle_t bsh;
	bus_addr_t addr, base;
	int devtype;
	u_int span;

	/*
	 * Sanity checks:
	 * these devices provide both a DIO and an STI ROM. We expect the
	 * DIO ROM to be a DIO-II ROM (i.e. to be at a DIO-II select code)
	 * and report the device as spanning at least four select codes.
	 */

	if (!DIO_ISDIOII(scode))
		return 0;

	addr = (bus_addr_t)dio_scodetopa(scode);
	if (bus_space_map(bst, addr, PAGE_SIZE, 0, &bsh))
		return 0;
	span = bus_space_read_1(bst, bsh, DIOII_SIZEOFF);
	bus_space_unmap(bst, bsh, PAGE_SIZE);

	if (span < STI_DIO_SIZE - 1)
		return 0;

	base = (bus_addr_t)dio_scodetopa(scode + STI_DIO_SCODE_OFFSET);
	if (bus_space_map(bst, base, PAGE_SIZE, 0, &bsh))
		return 0;
	devtype = bus_space_read_1(bst, bsh, 3);
	bus_space_unmap(bst, bsh, PAGE_SIZE);

	if (devtype != STI_DEVTYPE1 && devtype != STI_DEVTYPE4)
		return 0;

	return 1;
}

int
sti_dio_cnprobe(bus_space_tag_t bst, bus_addr_t addr, int scode)
{

	if (sti_dio_probe(bst, scode) == 0) {
		/* not found */
		return 1;
	}
	conscode = scode;

	return 0;
}

void
sti_dio_cnattach(bus_space_tag_t bst, int scode)
{
	paddr_t base;

	base = (paddr_t)dio_scodetopa(scode + STI_DIO_SCODE_OFFSET);
	sti_machdep_cnattach(bst, base);
}
