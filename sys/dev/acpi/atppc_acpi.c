/* $NetBSD: atppc_acpi.c,v 1.3 2004/04/11 09:38:19 kochi Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: atppc_acpi.c,v 1.3 2004/04/11 09:38:19 kochi Exp $");

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

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/ic/atppcvar.h>
#include <dev/isa/atppc_isadma.h>

static int	atppc_acpi_match(struct device *, struct cfdata *, void *);
static void	atppc_acpi_attach(struct device *, struct device *, void *);

struct atppc_acpi_softc {
	struct atppc_softc sc_atppc;

	isa_chipset_tag_t sc_ic;
	int sc_drq;
};

CFATTACH_DECL(atppc_acpi, sizeof(struct atppc_acpi_softc), atppc_acpi_match,
    atppc_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const atppc_acpi_ids[] = {
	"PNP04??",	/* Standard AT printer port */
	NULL
};

static int atppc_acpi_dma_start(struct atppc_softc *, void *, u_int,
	u_int8_t);
static int atppc_acpi_dma_finish(struct atppc_softc *);
static int atppc_acpi_dma_abort(struct atppc_softc *);
static int atppc_acpi_dma_malloc(struct device *, caddr_t *, bus_addr_t *,
	bus_size_t);
static void atppc_acpi_dma_free(struct device *, caddr_t *, bus_addr_t *,
	bus_size_t);
/*
 * atppc_acpi_match: autoconf(9) match routine
 */
static int
atppc_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, atppc_acpi_ids);
}

static void
atppc_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct atppc_softc *sc = (struct atppc_softc *) self;
	struct atppc_acpi_softc *asc = (struct atppc_acpi_softc *)self;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io;
	struct acpi_irq *irq;
	struct acpi_drq *drq;
	ACPI_STATUS rv;
	int nirq;

	sc->sc_dev_ok = ATPPC_NOATTACH;

	printf(": AT Parallel Port\n");

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
				 &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		printf("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	nirq = irq->ar_irq;

	/* find our DRQ */
	drq = acpi_res_drq(&res, 0);
	if (drq == NULL) {
		printf("%s: unable to find drq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	asc->sc_drq = drq->ar_drq;

	/* Attach */
	sc->sc_iot = aa->aa_iot;
	sc->sc_has = 0;
	asc->sc_ic = aa->aa_ic;

	sc->sc_dev_ok = ATPPC_ATTACHED;

	if (bus_space_map(sc->sc_iot, io->ar_base, io->ar_length, 0,
		&sc->sc_ioh) != 0) {
		printf("%s: attempt to map bus space failed, device not "
			"properly attached.\n", self->dv_xname);
		return;
	}

	sc->sc_ieh = isa_intr_establish(aa->aa_ic, nirq,
	    (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL,
	    IPL_TTY, atppcintr, sc);

	/* setup DMA hooks */
	if (atppc_isadma_setup(sc, asc->sc_ic, asc->sc_drq) == 0) {
		sc->sc_has |= ATPPC_HAS_DMA;
		sc->sc_dma_start = atppc_acpi_dma_start;
		sc->sc_dma_finish = atppc_acpi_dma_finish;
		sc->sc_dma_abort = atppc_acpi_dma_abort;
		sc->sc_dma_malloc = atppc_acpi_dma_malloc;
		sc->sc_dma_free = atppc_acpi_dma_free;
	}

	sc->sc_has |= ATPPC_HAS_INTR;

	/* Run soft configuration attach */
	atppc_sc_attach(sc);
}

/* Start DMA operation over ISA bus */
static int
atppc_acpi_dma_start(struct atppc_softc *lsc, void *buf, u_int nbytes,
	u_int8_t mode)
{
	struct atppc_acpi_softc * sc = (struct atppc_acpi_softc *) lsc;

	return atppc_isadma_start(sc->sc_ic, sc->sc_drq, buf, nbytes, mode);
}

/* Stop DMA operation over ISA bus */
static int
atppc_acpi_dma_finish(struct atppc_softc * lsc)
{
	struct atppc_acpi_softc * sc = (struct atppc_acpi_softc *) lsc;

	return atppc_isadma_finish(sc->sc_ic, sc->sc_drq);
}

/* Abort DMA operation over ISA bus */
int
atppc_acpi_dma_abort(struct atppc_softc * lsc)
{
	struct atppc_acpi_softc * sc = (struct atppc_acpi_softc *) lsc;

	return atppc_isadma_abort(sc->sc_ic, sc->sc_drq);
}

/* Allocate memory for DMA over ISA bus */
int
atppc_acpi_dma_malloc(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr,
	bus_size_t size)
{
	struct atppc_acpi_softc * sc = (struct atppc_acpi_softc *) dev;

	return atppc_isadma_malloc(sc->sc_ic, sc->sc_drq, buf, bus_addr, size);
}

/* Free memory allocated by atppc_isa_dma_malloc() */
void
atppc_acpi_dma_free(struct device * dev, caddr_t * buf, bus_addr_t * bus_addr,
	bus_size_t size)
{
	struct atppc_acpi_softc * sc = (struct atppc_acpi_softc *) dev;

	return atppc_isadma_free(sc->sc_ic, sc->sc_drq, buf, bus_addr, size);
}
