/*	$NetBSD: mvsoc_sdhc.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: mvsoc_sdhc.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

#define MVSOC_SDHC_SIZE		0x2000
#define MVSOC_SDHC_REG_SIZE	SDHC_CAPABILITIES2

#define MVSOC_SDHC_MCR0		  0x48
#define MVSOC_SDHC_MCR1		  0x4a
#define MVSOC_SDHC_C1R		  0x6a
#define   SDHC_C1R_CRC16_CHK_EN		(1 << 2)

static int mvsoc_sdhc_match(device_t, cfdata_t, void *);
static void mvsoc_sdhc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mvsoc_sdhc, sizeof(struct sdhc_softc),
    mvsoc_sdhc_match, mvsoc_sdhc_attach, NULL, NULL);

static int
mvsoc_sdhc_match(device_t parent, cfdata_t match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	mva->mva_size = MVSOC_SDHC_SIZE;
	return 1;
}

static void
mvsoc_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct sdhc_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;
	bus_space_handle_t ioh;
	int error;

	sc->sc_dev = self;
	sc->sc_host = malloc(sizeof(*sc->sc_host), M_DEVBUF, M_WAITOK | M_ZERO);
	sc->sc_dmat = mva->mva_dmat;
	/* Must require the DMA.  This sdhc can't 32bit access to SDHC_DATA. */
	sc->sc_flags = SDHC_FLAG_USE_DMA;
	sc->sc_flags |= SDHC_FLAG_EXTDMA_DMAEN;
	sc->sc_flags |= SDHC_FLAG_NO_AUTO_STOP;
	sc->sc_flags |= SDHC_FLAG_NO_BUSY_INTR;

	aprint_naive("\n");
	aprint_normal(": SDHC controller\n");

	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
				mva->mva_offset, mva->mva_size, &ioh)) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	intr_establish(mva->mva_irq, IPL_VM, IST_LEVEL, sdhc_intr, sc);

	bus_space_write_2(mva->mva_iot, ioh, MVSOC_SDHC_C1R,
	    bus_space_read_2(mva->mva_iot, ioh, MVSOC_SDHC_C1R) |
	    SDHC_C1R_CRC16_CHK_EN);

	error = sdhc_host_found(sc, mva->mva_iot, ioh, MVSOC_SDHC_REG_SIZE);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error %d\n",
		    error);
		return;
	}
}
