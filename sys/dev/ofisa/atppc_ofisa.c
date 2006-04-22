/* $NetBSD: atppc_ofisa.c,v 1.3.6.1 2006/04/22 11:39:11 simonb Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atppc_ofisa.c,v 1.3.6.1 2006/04/22 11:39:11 simonb Exp $");

#include "opt_atppc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/termios.h>

#include <machine/bus.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/atppcvar.h>
#include <dev/isa/atppc_isadma.h>

static int	atppc_ofisa_match(struct device *, struct cfdata *, void *);
static void	atppc_ofisa_attach(struct device *, struct device *, void *);

struct atppc_ofisa_softc {
	struct atppc_softc sc_atppc;

	isa_chipset_tag_t sc_ic;
	int sc_drq;
};

CFATTACH_DECL(atppc_ofisa, sizeof(struct atppc_ofisa_softc), atppc_ofisa_match,
    atppc_ofisa_attach, NULL, NULL);

static int atppc_ofisa_dma_start(struct atppc_softc *, void *, u_int,
	u_int8_t);
static int atppc_ofisa_dma_finish(struct atppc_softc *);
static int atppc_ofisa_dma_abort(struct atppc_softc *);
static int atppc_ofisa_dma_malloc(struct device *, caddr_t *, bus_addr_t *,
	bus_size_t);
static void atppc_ofisa_dma_free(struct device *, caddr_t *, bus_addr_t *,
	bus_size_t);
/*
 * atppc_ofisa_match: autoconf(9) match routine
 */
static int
atppc_ofisa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ofisa_attach_args *aa = aux;
	static const char *const compatible_strings[] = { "pnpPNP,401", NULL };
	int rv = 0;

	if (of_compatible(aa->oba.oba_phandle, compatible_strings) != -1)
		rv = 5;
#ifdef _LPT_OFISA_MD_MATCH
	if (!rv)
		rv = lpt_ofisa_md_match(parent, match, aux);
#endif
	return (rv);
}

static void
atppc_ofisa_attach(struct device *parent, struct device *self, void *aux)
{
	struct atppc_softc *sc = device_private(self);
	struct atppc_ofisa_softc *asc = device_private(self);
	struct ofisa_attach_args *aa = aux;
	struct ofisa_reg_desc reg;
	struct ofisa_intr_desc intr;
	struct ofisa_dma_desc dma;
	int n;

	sc->sc_dev_ok = ATPPC_NOATTACH;

	printf(": AT Parallel Port\n");

	/* find our I/O space */
	n = ofisa_reg_get(aa->oba.oba_phandle, &reg, 1);
#ifdef _LPT_OFISA_MD_REG_FIXUP
	n = lpt_ofisa_md_reg_fixup(parent, self, aux, &reg, 1, n);
#endif
	if (n != 1) {
		printf("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our IRQ */
	n = ofisa_intr_get(aa->oba.oba_phandle, &intr, 1);
#ifdef _LPT_OFISA_MD_INTR_FIXUP
	n = lpt_ofisa_md_intr_fixup(parent, self, aux, &intr, 1, n);
#endif
	if (n != 1) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our DRQ */
	if (ofisa_dma_get(aa->oba.oba_phandle, &dma, 1) != 1) {
		printf("%s: unable to find DMA data\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	asc->sc_drq = dma.drq;

	/* Attach */
	sc->sc_iot = (reg.type == OFISA_REG_TYPE_IO) ? aa->iot : aa->memt;
	sc->sc_has = 0;
	asc->sc_ic = aa->ic;

	sc->sc_dev_ok = ATPPC_ATTACHED;

	if (bus_space_map(sc->sc_iot, reg.addr, reg.len, 0,
		&sc->sc_ioh) != 0) {
		printf("%s: attempt to map bus space failed, device not "
			"properly attached.\n", self->dv_xname);
		return;
	}

	sc->sc_ieh = isa_intr_establish(aa->ic, intr.irq, intr.share,
	    IPL_TTY, atppcintr, sc);
	sc->sc_has |= ATPPC_HAS_INTR;

	/* setup DMA hooks */
	if (atppc_isadma_setup(sc, asc->sc_ic, asc->sc_drq) == 0) {
		sc->sc_has |= ATPPC_HAS_DMA;
		sc->sc_dma_start = atppc_ofisa_dma_start;
		sc->sc_dma_finish = atppc_ofisa_dma_finish;
		sc->sc_dma_abort = atppc_ofisa_dma_abort;
		sc->sc_dma_malloc = atppc_ofisa_dma_malloc;
		sc->sc_dma_free = atppc_ofisa_dma_free;
	}

	/* Run soft configuration attach */
	atppc_sc_attach(sc);
}

/* Start DMA operation over ISA bus */
static int
atppc_ofisa_dma_start(struct atppc_softc *lsc, void *buf, u_int nbytes,
	u_int8_t mode)
{
	struct atppc_ofisa_softc * sc = (struct atppc_ofisa_softc *) lsc;

	return atppc_isadma_start(sc->sc_ic, sc->sc_drq, buf, nbytes, mode);
}

/* Stop DMA operation over ISA bus */
static int
atppc_ofisa_dma_finish(struct atppc_softc * lsc)
{
	struct atppc_ofisa_softc * sc = (struct atppc_ofisa_softc *) lsc;

	return atppc_isadma_finish(sc->sc_ic, sc->sc_drq);
}

/* Abort DMA operation over ISA bus */
int
atppc_ofisa_dma_abort(struct atppc_softc * lsc)
{
	struct atppc_ofisa_softc * sc = (struct atppc_ofisa_softc *) lsc;

	return atppc_isadma_abort(sc->sc_ic, sc->sc_drq);
}

/* Allocate memory for DMA over ISA bus */
int
atppc_ofisa_dma_malloc(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr,
	bus_size_t size)
{
	struct atppc_ofisa_softc * sc = (struct atppc_ofisa_softc *) dev;

	return atppc_isadma_malloc(sc->sc_ic, sc->sc_drq, buf, bus_addr, size);
}

/* Free memory allocated by atppc_isa_dma_malloc() */
void
atppc_ofisa_dma_free(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr,
	bus_size_t size)
{
	struct atppc_ofisa_softc * sc = (struct atppc_ofisa_softc *) dev;

	return atppc_isadma_free(sc->sc_ic, sc->sc_drq, buf, bus_addr, size);
}
