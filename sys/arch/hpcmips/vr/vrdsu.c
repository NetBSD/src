/*	$NetBSD: vrdsu.c,v 1.3 2001/09/16 05:32:21 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/dsureg.h>
#include <hpcmips/vr/vrdsuvar.h>

struct vrdsu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int vrdsumatch(struct device *, struct cfdata *, void *);
static void vrdsuattach(struct device *, struct device *, void *);

static void vrdsu_write(struct vrdsu_softc *, int, unsigned short);
static unsigned short vrdsu_read(struct vrdsu_softc *, int);

struct cfattach vrdsu_ca = {
	sizeof(struct vrdsu_softc), vrdsumatch, vrdsuattach
};

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
vrdsumatch(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
vrdsuattach(struct device *parent, struct device *self, void *aux)
{
	struct vrdsu_softc *sc = (struct vrdsu_softc *)self;
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
vrdsu_reset()
{

	if (the_dsu_sc) {
		splhigh();
		vrdsu_write(the_dsu_sc, DSUSET_REG_W, 1); /* 1 sec */
		vrdsu_write(the_dsu_sc, DSUCNT_REG_W, DSUCNT_DSWEN);
		while (1);
	} else {
		printf("%s(%d): There is no DSU.", __FILE__, __LINE__);
	}
}
