/*	$NetBSD: sc_mbmem.c,v 1.8 2003/07/15 03:36:12 lukem Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, Gordon W. Ross, and Matthew Fredette.
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

/*
 * This file contains only the machine-dependent parts of the
 * Sun-2 SCSI driver.  (Autoconfig stuff and DMA functions.)
 * The machine-independent parts are in sunscpal.c
 *
 * Supported hardware includes:
 * Sun SCSI-2 on Multibus (Sun2/120,Sun2/170)
 * Sun SCSI-2 on VME (Sun2/50,Sun3/160,Sun3/260,others?)
 *
 * Credits, history:
 *
 * Matthew Fredette took revision 1.15 of si_vme.c, and then heavily
 * modified it to support the Sun sc.  The remaining credits
 * are from si_vme.c:
 *
 * David Jones wrote the initial version of this module, which
 * included support for the VME adapter only. (no reselection).
 *
 * Gordon Ross added support for the OBIO adapter, and re-worked
 * both the VME and OBIO code to support disconnect/reselect.
 * (Required figuring out the hardware "features" noted above.)
 *
 * The autoconfiguration boilerplate came from Adam Glass.
 */

/*****************************************************************
 * Multibus functions for sc
 ****************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sc_mbmem.c,v 1.8 2003/07/15 03:36:12 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

/* #define DEBUG XXX */

#include <dev/ic/sunscpalreg.h>
#include <dev/ic/sunscpalvar.h>

/* Yes, we use a VME header file. */
#include <dev/vme/screg.h>

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

/*
 * New-style autoconfig attachment
 */

static int	sunsc_mbmem_match __P((struct device *, struct cfdata *, void *));
static void	sunsc_mbmem_attach __P((struct device *, struct device *, void *));
static int	sunsc_mbmem_intr __P((void *));

CFATTACH_DECL(sc_mbmem, sizeof(struct sunscpal_softc),
    sunsc_mbmem_match, sunsc_mbmem_attach, NULL, NULL);

/*
 * Options for parity, DMA, and interrupts.
 */
int sunsc_mbmem_options = 0x00;

static int
sunsc_mbmem_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mbmem_attach_args *mbma = aux;
	bus_space_handle_t bh;
	int matched;

	/* No default Multibus address. */
	if (mbma->mbma_paddr == -1)
		return (0);

	/* Make sure there is something there... */
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, SCREG_BANK_SZ,
			  0, &bh))
		return (0);
	matched = (bus_space_peek_2(mbma->mbma_bustag, bh, SCREG_ICR, NULL) == 0);
	bus_space_unmap(mbma->mbma_bustag, bh, SCREG_BANK_SZ);
	if (!matched)
		return (0);

	/* Default interrupt priority. */
	if (mbma->mbma_pri == -1)
		mbma->mbma_pri = 2;

	return (1);
}

static void
sunsc_mbmem_attach(parent, self, args)
	struct device	*parent, *self;
	void		*args;
{
	struct sunscpal_softc *sc = (void *) self;
	struct cfdata *cf = self->dv_cfdata;
	struct mbmem_attach_args *mbma = args;
	int i;

	/* Map in the device. */
	sc->sunscpal_regt = mbma->mbma_bustag;
	sc->sunscpal_dmat = mbma->mbma_dmatag;
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, SCREG_BANK_SZ,
			  0, &sc->sunscpal_regh))
		panic("sunsc_mbmem_attach: can't map");

	/* Device register offsets. */
	sc->sunscpal_data = SCREG_DATA;
	sc->sunscpal_cmd_stat = SCREG_CMD_STAT;
	sc->sunscpal_icr = SCREG_ICR;
	sc->sunscpal_dma_addr_h = SCREG_DMA_ADDR_H;
	sc->sunscpal_dma_addr_l = SCREG_DMA_ADDR_L;
	sc->sunscpal_dma_count = SCREG_DMA_COUNT;
	sc->sunscpal_intvec = SCREG_INTVEC;
	
	/* Allocate DMA handles. */
	i = SUNSCPAL_OPENINGS * sizeof(struct sunscpal_dma_handle);
	sc->sc_dma_handles = (sunscpal_dma_handle_t)
		malloc(i, M_DEVBUF, M_WAITOK);
	if (sc->sc_dma_handles == NULL)
		panic("sc: DMA handles malloc failed");
	for (i = 0; i < SUNSCPAL_OPENINGS; i++)
		if (bus_dmamap_create(sc->sunscpal_dmat, SUNSCPAL_MAX_DMA_LEN,
				      1, SUNSCPAL_MAX_DMA_LEN,
				      0, BUS_DMA_WAITOK, &sc->sc_dma_handles[i].
dh_dmamap) != 0)
			panic("sc: DMA map create failed");

	/* Miscellaneous. */
	sc->sc_min_dma_len = MIN_DMA_LEN;
	sc->sc_rev = SUNSCPAL_VARIANT_501_1006;

	/* Attach interrupt handler. */
	bus_intr_establish(mbma->mbma_bustag, mbma->mbma_pri, IPL_BIO, 0,
			   sunsc_mbmem_intr, sc);

	/* Do the common attach stuff. */
	sunscpal_attach(sc, (cf->cf_flags ? cf->cf_flags : sunsc_mbmem_options));
}

static int
sunsc_mbmem_intr(void *arg)
{
	struct sunscpal_softc *sc = arg;
	int claimed;

	claimed = sunscpal_intr(sc);
#ifdef	DEBUG
	if (!claimed) {
		printf("sunsc_intr: spurious from SBC\n");
	}
#endif
	/* Yes, we DID cause this interrupt. */
	claimed = 1;

	return (claimed);
}
