/*	$NetBSD: fdc_jazzio.c,v 1.1 2001/05/22 03:22:50 soda Exp $	*/
/*	$OpenBSD: fd.c,v 1.6 1998/10/03 21:18:57 millert Exp $	*/
/*	NetBSD: fd.c,v 1.78 1995/07/04 07:23:09 mycroft Exp 	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)fd.c	7.4 (Berkeley) 5/25/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/queue.h>

#include <mips/locore.h>	/* for mips3_HitFlushDCache() */

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/fdreg.h>
#include <arc/jazz/fdcvar.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/dma.h>

#include "locators.h"

/* controller driver configuration */
int fdc_jazzio_probe(struct device *, struct cfdata *, void *);
void fdc_jazzio_attach(struct device *, struct device *, void *);

/* MD DMA hook functions */
void fdc_jazzio_dma_start(struct fdc_softc *, caddr_t, size_t, int);
void fdc_jazzio_dma_abort(struct fdc_softc *);
void fdc_jazzio_dma_done(struct fdc_softc *);

/* software state, per controller */
struct fdc_jazzio_softc {
	struct fdc_softc sc_fdc;	/* base fdc device */

	bus_space_handle_t sc_baseioh;	/* base I/O handle */
	struct dma_softc __dma;
	struct dma_softc *dma;
};

struct cfattach fdc_jazzio_ca = {
	sizeof(struct fdc_jazzio_softc), fdc_jazzio_probe, fdc_jazzio_attach
};

#define FDC_NPORT 6
#define FDC_OFFSET 2 /* Should we use bus_space_subregion() or not? */

int
fdc_jazzio_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot;
	bus_space_handle_t base_ioh, ioh;
	int rv;

	if (strcmp(ja->ja_name, "fdc") != 0)
		return 0;

	iot = ja->ja_bust;
	rv = 0;

	/* Map the I/O space. */
	if (bus_space_map(iot, ja->ja_addr,
	    FDC_OFFSET + FDC_NPORT, 0, &base_ioh))
		return 0;

	if (bus_space_subregion(iot, base_ioh, FDC_OFFSET, FDC_NPORT, &ioh))
		goto out;

	/* reset */
	bus_space_write_1(iot, ioh, FDOUT, 0);
	delay(100);
	bus_space_write_1(iot, ioh, FDOUT, FDO_FRST);

	/* see if it can handle a command */
	if (out_fdc(iot, ioh, NE7CMD_SPECIFY) < 0)
		goto out;
	out_fdc(iot, ioh, 0xdf); /* XXX */
	out_fdc(iot, ioh, 2); /* XXX */

	rv = 1;

 out:
	bus_space_unmap(iot, base_ioh, FDC_OFFSET + FDC_NPORT);
	return rv;
}

void
fdc_jazzio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_jazzio_softc *jsc = (struct fdc_jazzio_softc *)self;
	struct fdc_softc *fdc = &jsc->sc_fdc;
	struct jazzio_attach_args *ja = aux;

	fdc->sc_iot = ja->ja_bust;

	fdc->sc_maxiosize = 4096; /* XXX */
	fdc->sc_dma_start = fdc_jazzio_dma_start;
	fdc->sc_dma_abort = fdc_jazzio_dma_abort;
	fdc->sc_dma_done = fdc_jazzio_dma_done;

	jsc->dma = &jsc->__dma;
	fdc_dma_init(jsc->dma);

	if (bus_space_map(fdc->sc_iot, ja->ja_addr,
	    FDC_OFFSET + FDC_NPORT, 0, &jsc->sc_baseioh)) {
		printf(": unable to map I/O space\n");
		return;
	}

	if (bus_space_subregion(fdc->sc_iot, jsc->sc_baseioh,
	    FDC_OFFSET, FDC_NPORT, &fdc->sc_ioh)) {
		printf(": unable to subregion I/O space\n");
		bus_space_unmap(fdc->sc_iot, jsc->sc_baseioh,
		    FDC_OFFSET + FDC_NPORT);
		return;
	}

	printf("\n");

	jazzio_intr_establish(ja->ja_intr, fdcintr, fdc);

	fdcattach(fdc);
}

void
fdc_jazzio_dma_start(fdc, addr, size, datain)
	struct fdc_softc *fdc;
	caddr_t addr;
	size_t size;
	int datain;
{
	struct fdc_jazzio_softc *jsc = (void *)fdc;

	mips3_FlushDCache((vaddr_t)addr, (vsize_t)size);
	DMA_START(jsc->dma, addr, size, datain ? DMA_FROM_DEV : DMA_TO_DEV);
}

void
fdc_jazzio_dma_abort(fdc)
	struct fdc_softc *fdc;
{
	struct fdc_jazzio_softc *jsc = (void *)fdc;

	DMA_RESET(jsc->dma);
}

void
fdc_jazzio_dma_done(fdc)
	struct fdc_softc *fdc;
{
	struct fdc_jazzio_softc *jsc = (void *)fdc;

	DMA_END(jsc->dma);
}
