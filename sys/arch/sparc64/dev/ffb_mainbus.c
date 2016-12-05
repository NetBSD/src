/*	$NetBSD: ffb_mainbus.c,v 1.13.24.1 2016/12/05 10:54:58 skrll Exp $	*/
/*	$OpenBSD: creator_mainbus.c,v 1.4 2002/07/26 16:39:04 jason Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net),
 *  Federico G. Schwindt (fgsch@openbsd.org)
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
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffb_mainbus.c,v 1.13.24.1 2016/12/05 10:54:58 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <sparc64/dev/ffbreg.h>
#include <sparc64/dev/ffbvar.h>

extern int prom_stdout_node;

int	ffb_mainbus_match(device_t, cfdata_t, void *);
void	ffb_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ffb_mainbus, sizeof(struct ffb_softc),
    ffb_mainbus_match, ffb_mainbus_attach, NULL, NULL);

int
ffb_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "SUNW,ffb") == 0 ||
	    strcmp(ma->ma_name, "SUNW,afb") == 0)
		return 1;
	return 0;
}

void
ffb_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct ffb_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int i, nregs;

	sc->sc_dev = self;
	sc->sc_bt = ma->ma_bustag;

	nregs = min(ma->ma_nreg, FFB_NREGS);

	if (nregs < FFB_REG_DFB24) {
		printf(": no dfb24 regs found\n");
		goto fail1;
	}


	if (bus_space_map(sc->sc_bt, ma->ma_reg[FFB_REG_FBC].ur_paddr,
	    ma->ma_reg[FFB_REG_FBC].ur_len, 0, &sc->sc_fbc_h)) {
		printf(": failed to map fbc\n");
		goto fail1;
	}

	if (bus_space_map(sc->sc_bt, ma->ma_reg[FFB_REG_DAC].ur_paddr,
	    ma->ma_reg[FFB_REG_DAC].ur_len, 0, &sc->sc_dac_h)) {
		printf(": failed to map dac\n");
		goto unmap_fbc;
	}

	if (bus_space_map(sc->sc_bt, ma->ma_reg[FFB_REG_SFB32].ur_paddr,
	    ma->ma_reg[FFB_REG_SFB32].ur_len,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
	    &sc->sc_sfb32_h)) {
		panic(": failed to map SFB32\n");
		goto unmap_dac;
	}
	sc->sc_sfb32 = bus_space_vaddr(sc->sc_bt, sc->sc_sfb32_h);

	for (i = 0; i < nregs; i++) {
		sc->sc_addrs[i] = ma->ma_reg[i].ur_paddr;
		sc->sc_sizes[i] = ma->ma_reg[i].ur_len;
	}
	sc->sc_nreg = nregs;

	sc->sc_console = (prom_stdout_node == ma->ma_node);
	sc->sc_node = ma->ma_node;

	if (strcmp(ma->ma_name, "SUNW,afb") == 0)
		sc->sc_type = FFB_AFB;

	ffb_attach(self);

	return;

unmap_dac:
	bus_space_unmap(sc->sc_bt, sc->sc_dac_h,
		    ma->ma_reg[FFB_REG_DAC].ur_len);

unmap_fbc:
	bus_space_unmap(sc->sc_bt, sc->sc_fbc_h,
		    ma->ma_reg[FFB_REG_FBC].ur_len);
fail1:
	return;
}
