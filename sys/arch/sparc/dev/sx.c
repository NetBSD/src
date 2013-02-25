/*	$NetBSD: sx.c,v 1.2.4.2 2013/02/25 00:28:57 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Lorenz.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sx.c,v 1.2.4.2 2013/02/25 00:28:57 tls Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <sparc/dev/sbusvar.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/dev/sxvar.h>
#include <sparc/dev/sxreg.h>

/* autoconfiguration driver */
static	int sx_match(device_t, struct cfdata *, void *);
static	void sx_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sx, sizeof(struct sx_softc),
    sx_match, sx_attach, NULL, NULL);

static int
sx_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("SUNW,sx", ma->ma_name) == 0);
}

static void
sx_attach(device_t parent, device_t self, void *aux)
{
	struct sx_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
    	int i;
#ifdef SX_DEBUG
	int j;
#endif

	printf("\n");

	sc->sc_dev = self;
	sc->sc_tag = ma->ma_bustag;
	sc->sc_uregs = ma->ma_paddr + 0x1000;

	if (bus_space_map(sc->sc_tag, ma->ma_paddr, 0x1000, 0, &sc->sc_regh)) {
		aprint_error_dev(self, "failed to map registers\n");
		return;
	}

	/* stop the processor */
	sx_write(sc, SX_CONTROL_STATUS, 0);
	/* initialize control registers, clear errors etc. */
	sx_write(sc, SX_R0_INIT, 0);
	sx_write(sc, SX_ERROR, 0);
	/* default, to be overriden once cgfourteen takes over */
	sx_write(sc, SX_PAGE_BOUND_LOWER, 0xfc000000);
	/* cg14 takes up the whole 64MB chunk */
	sx_write(sc, SX_PAGE_BOUND_UPPER, 0xffffffff);
	sx_write(sc, SX_DIAGNOSTICS, 0);
	sx_write(sc, SX_PLANEMASK, 0xffffffff);

	/*
	 * initialize all other registers
	 * use the direct port since the processor is stopped
	 */
	for (i = 4; i < 0x200; i += 4) 
		sx_write(sc, SX_DIRECT_R0 + i, 0);

	/* ... and start the processor again */
	sx_write(sc, SX_CONTROL_STATUS, SX_PB | SX_GO);

#ifdef SX_DEBUG
	sta(0xfc000000, ASI_SX, SX_LD(8, 31, 0));
	for (i = 1; i < 60; i++)
		sta(0xfc000000 + (i * 1280), ASI_SX, SX_ST(8, 31, 0));
	for (i = 900; i < 1000; i++)
		sta(0xfc000000 + (i * 1280) + 600, ASI_SX, SX_ST(0, 31, 0));

    	for (i = 0; i < 0x30; i+= 16) {
    		printf("%08x:", i);
    		for (j = 0; j < 16; j += 4) {
			if ((i + j) > 0x28) continue;
    			printf(" %08x",
    			    bus_space_read_4(sc->sc_tag, sc->sc_regh, i + j));
		}
		printf("\n");
	}
	printf("registers:\n");
    	for (i = 0x300; i < 0x500; i+= 16) {
    		printf("%08x:", i);
    		for (j = 0; j < 16; j += 4) {
    			printf(" %08x",
    			    bus_space_read_4(sc->sc_tag, sc->sc_regh,
    			        i + j));
		}
		printf("\n");
	}
#endif
}

