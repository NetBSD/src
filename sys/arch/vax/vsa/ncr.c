/*	$NetBSD: ncr.c,v 1.22 1999/09/21 18:07:54 ragge Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, Gordon W. Ross, and Jens A. Nilsson.
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
 * This file contains the machine-dependent parts of the NCR-5380
 * controller. The machine-independent parts are in ncr5380sbc.c.
 *
 * Note: Only PIO transfers for now which implicates very bad
 * performance. DMA support will come soon.
 *
 * Jens A. Nilsson.
 *
 * Credits:
 * 
 * This code is based on arch/sun3/dev/si*
 * Written by David Jones, Gordon Ross, and Adam Glass.
 */

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

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/vsbus.h>
#include <machine/bus.h>
#include <machine/sid.h>

#include "ioconf.h"

#define MIN_DMA_LEN 128

#ifdef notyet
struct si_dma_handle {
	int	dh_flags;
#define SIDH_BUSY	1
#define SIDH_OUT	2
	u_char	*dh_addr;
};
#endif

struct si_softc {
	struct ncr5380_softc	ncr_sc;
};

/* This is copied from julian's bt driver */
/* "so we have a default dev struct for our link struct." */
static struct scsipi_device si_dev = {
	NULL,	/* Use default error handler. */
	NULL,	/* Use default start handler. */
	NULL,	/* Use default async handler. */
	NULL,	/* Use default "done" routine. */
};

static int si_match(struct device *, struct cfdata *, void *);
static void si_attach(struct device *, struct device *, void *);
static void si_minphys(struct buf *);
static void si_intr(int);
void dk_establish(void);

struct cfattach ncr_ca = {
	sizeof(struct si_softc), si_match, si_attach
};

void
dk_establish(void)
{
#if 0
	printf("faking dk_establish()...\n");
#endif
}

static int
si_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vsbus_attach_args *va = aux;
	volatile char *si_csr = (char *) va->va_addr;

	if (vax_boardtype == VAX_BTYP_49)
		return 0;
	/* This is the way Linux autoprobes the interrupt MK-990321 */
	si_csr[12] = 0;
	si_csr[16] = 0x80;
	si_csr[0] = 0x80;
	si_csr[4] = 5; /* 0xcf */
	DELAY(100000);
	va->va_ivec = si_intr;
	return 1;
}

static void
si_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct vsbus_attach_args *va = aux;
	struct si_softc *sc = (struct si_softc *) self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;

	printf("\n");
	/*
	 * MD function pointers used by the MI code.
	 */
	ncr_sc->sc_pio_out = ncr5380_pio_out;
	ncr_sc->sc_pio_in =  ncr5380_pio_in;

#ifdef notyet
	ncr_sc->sc_dma_alloc = si_dma_alloc;
	ncr_sc->sc_dma_free  = si_dma_free;
	ncr_sc->sc_dma_setup = si_dma_setup;
	ncr_sc->sc_dma_start = si_dma_start;
	ncr_sc->sc_dma_poll  = si_dma_poll;
	ncr_sc->sc_dma_eop   = si_dma_eop;
	ncr_sc->sc_dma_stop  = si_dma_stop;
#endif
	ncr_sc->sc_intr_on   = NULL /*si_intr_on*/;
	ncr_sc->sc_intr_off  = NULL /*si_intr_off*/;

	ncr_sc->sc_dma_alloc = NULL;
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Fill in the adapter.
	 */
	ncr_sc->sc_adapter.scsipi_cmd = ncr5380_scsi_cmd;
	ncr_sc->sc_adapter.scsipi_minphys = si_minphys;

	/*
	 * Fill in the prototype scsi_link.
	 */
	ncr_sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	ncr_sc->sc_link.adapter_softc = sc;
	ncr_sc->sc_link.scsipi_scsi.adapter_target = 7;
	ncr_sc->sc_link.adapter = &ncr_sc->sc_adapter;
	ncr_sc->sc_link.device = &si_dev;
	ncr_sc->sc_link.type = BUS_SCSI;
	
	/*
	 * Initialize fields used by the MI code.
	 */
/*	ncr_sc->sc_regt =  Unused on VAX */
	ncr_sc->sc_regh = vax_map_physmem(va->va_paddr, 1);

	/* Register offsets */
	ncr_sc->sci_r0 = 0;
	ncr_sc->sci_r1 = 4;
	ncr_sc->sci_r2 = 8;
	ncr_sc->sci_r3 = 12;
	ncr_sc->sci_r4 = 16;
	ncr_sc->sci_r5 = 20;
	ncr_sc->sci_r6 = 24;
	ncr_sc->sci_r7 = 28;

	ncr_sc->sc_no_disconnect = 0xff;
	/*
	 * Initialize si board itself.
	 */
	ncr5380_init(ncr_sc);
	ncr5380_reset_scsibus(ncr_sc);

	config_found(&(ncr_sc->sc_dev), &(ncr_sc->sc_link), scsiprint);
}

/*
 * Adjust the max transfer size. The DMA buffer is only 16k on VS2000.
 */
static void
si_minphys(struct buf *bp)
{
	if ((vax_boardtype == VAX_BTYP_410) && (bp->b_bcount > (16*1024)))
		bp->b_bcount = (16*1024);
	else if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}

static void
si_intr(int arg)
{
	ncr5380_intr(ncr_cd.cd_devs[arg]);
}
