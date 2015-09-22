/*	$NetBSD: vrdsu.c,v 1.11.14.1 2015/09/22 12:05:43 skrll Exp $	*/

/*
 * Copyright (c) 1999 Shin Takemura All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vrdsu.c,v 1.11.14.1 2015/09/22 12:05:43 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/dsureg.h>
#include <hpcmips/vr/vrdsuvar.h>

struct vrdsu_softc {
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int vrdsumatch(device_t, cfdata_t, void *);
static void vrdsuattach(device_t, device_t, void *);

static void vrdsu_write(struct vrdsu_softc *, int, unsigned short);
static unsigned short vrdsu_read(struct vrdsu_softc *, int);

CFATTACH_DECL_NEW(vrdsu, sizeof(struct vrdsu_softc),
    vrdsumatch, vrdsuattach, NULL, NULL);

struct vrdsu_softc *the_dsu_sc = NULL;

static inline void
vrdsu_write(struct vrdsu_softc *sc, int port, unsigned short val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrdsu_read(struct vrdsu_softc *sc, int port)
{

	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, port));
}

static int
vrdsumatch(device_t parent, cfdata_t cf, void *aux)
{

	return (1);
}

static void
vrdsuattach(device_t parent, device_t self, void *aux)
{
	struct vrdsu_softc *sc = device_private(self);
	struct vrip_attach_args *va = aux;

	sc->sc_iot = va->va_iot;
	if (bus_space_map(va->va_iot, va->va_addr, va->va_size,
	    0, &sc->sc_ioh)) {
		printf(": can't map bus space\n");
		return;
	}
	printf("\n");
	the_dsu_sc = sc;
}

void
vrdsu_reset(void)
{

	if (the_dsu_sc) {
		splhigh();
		vrdsu_write(the_dsu_sc, DSUSET_REG_W, 1); /* 1 sec */
		vrdsu_write(the_dsu_sc, DSUCNT_REG_W, DSUCNT_DSWEN);
		/*
		 * wipe out all physical memory for clean WinCE boot.
		 */
		memset((void *)MIPS_PHYS_TO_KSEG1(0), 0, ptoa(physmem) - 0);
		while (1);
	} else {
		printf("%s(%d): There is no DSU.", __FILE__, __LINE__);
	}
}
