/*	$NetBSD: mq200_vrip.c,v 1.4.2.2 2002/02/11 20:08:12 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 Takemura Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/dev/mq200var.h>
#include "bivideo.h"
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>     
#endif


#include "locators.h"

struct mq200_vrip_softc {
	struct mq200_softc	sc_mq200;
};

static int	mq200_vrip_probe(struct device *, struct cfdata *, void *);
static void	mq200_vrip_attach(struct device *, struct device *, void *);

struct cfattach mqvideo_vrip_ca = {
	sizeof(struct mq200_vrip_softc), mq200_vrip_probe, mq200_vrip_attach
};

static int
mq200_vrip_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vrip_attach_args *va = aux;
	bus_space_handle_t ioh;
	int res;

	if (va->va_addr == VRIPIFCF_ADDR_DEFAULT)
		return (0);

#if NBIVIDEO > 0
	if (bivideo_dont_attach) /* already attach video driver */
		return 0;
#endif /* NBIVIDEO > 0 */

	if (bus_space_map(va->va_iot, va->va_addr, va->va_size, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return 0;
	}
	res = mq200_probe(va->va_iot, ioh);
	bus_space_unmap(va->va_iot, ioh, va->va_size);

	return (res);
}


static void
mq200_vrip_attach(struct device *parent, struct device *self, void *aux)
{
	struct mq200_vrip_softc *vsc = (void *)self;
	struct mq200_softc *sc = &vsc->sc_mq200;
	struct vrip_attach_args *va = aux;

	sc->sc_baseaddr = va->va_addr;
	sc->sc_iot = va->va_iot;
	if (bus_space_map(va->va_iot, va->va_addr, va->va_size, 0,
			  &sc->sc_ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	mq200_attach(sc);
}
