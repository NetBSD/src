/*	$NetBSD: csa.c,v 1.10.44.2 2014/08/20 00:02:40 tls Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
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

/*
 * Cumana SCSI 1 driver using the generic NCR5380 driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: csa.c,v 1.10.44.2 2014/08/20 00:02:40 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/io.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>

#include <acorn32/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/powerromreg.h>

#define CSA_NCR5380_OFFSET	0x2100
#define CSA_CTRL_OFFSET		0x2000 - 2308
#define  CSA_DIRWRITE		0x02
#define  CSA_DIRREAD		0x00
#define  CSA_16BITS		0x00
#define  CSA_8BITS		0x10
#define  CSA_XXX		0x40
#define CSA_DATA_OFFSET		0x2000
#define CSA_INTR_OFFSET		0x2000
#define  CSA_INTR_MASK		0x80
#define CSA_STAT_OFFSET		0x2004
#define  CSA_STAT_DRQ		0x40
#define  CSA_STAT_END		0x80

int  csa_match(device_t, cfdata_t, void *);
void csa_attach(device_t, device_t, void *);

/*
 * Cumana SCSI 1 softc structure.
 *
 * Contains the generic ncr5380 device node,
 * podule information and global information
 * required by the driver.
 */

struct csa_softc {
	struct ncr5380_softc	sc_ncr5380;
	void			*sc_ih;
	struct evcnt		sc_intrcnt;
	int			sc_podule_number;
	podule_t		*sc_podule;
	volatile uint8_t	*sc_irqstatus;
	uint8_t			sc_irqmask;
	volatile uint8_t	*sc_ctrl;
	volatile uint8_t	*sc_status;
	volatile uint8_t	*sc_data;
};

CFATTACH_DECL_NEW(csa, sizeof(struct csa_softc),
    csa_match, csa_attach, NULL, NULL);

int csa_intr(void *arg);

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
csa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = aux;

	if (pa->pa_product == PODULE_CUMANA_SCSI1)
		return 1;

	/* PowerROM */
        if (pa->pa_product == PODULE_ALSYSTEMS_SCSI &&
            podulebus_initloader(pa) == 0 &&
            (podloader_callloader(pa, 0, 0) == PRID_CUMANA_SCSI1_8 ||
	     podloader_callloader(pa, 0, 0) == PRID_CUMANA_SCSI1_16))
                return 1;

	return 0;
}

/*
 * Card attach function
 *
 */

void
csa_attach(device_t parent, device_t self, void *aux)
{
	struct csa_softc *sc = device_private(self);
	struct ncr5380_softc *ncr_sc = &sc->sc_ncr5380;
	struct podule_attach_args *pa = aux;
	uint8_t *iobase;
	char hi_option[sizeof(device_xname(self)) + 8];

	ncr_sc->sc_dev = self;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;
	ncr_sc->sc_min_dma_len = 0;
	ncr_sc->sc_no_disconnect = 0x00;
	ncr_sc->sc_parity_disable = 0x00;

	ncr_sc->sc_dma_alloc = NULL;
	ncr_sc->sc_dma_free = NULL;
	ncr_sc->sc_dma_poll = NULL;
	ncr_sc->sc_dma_setup = NULL;
	ncr_sc->sc_dma_start = NULL;
	ncr_sc->sc_dma_eop = NULL;
	ncr_sc->sc_dma_stop = NULL;
	ncr_sc->sc_intr_on = NULL;
	ncr_sc->sc_intr_off = NULL;

	iobase = (uint8_t *)pa->pa_podule->slow_base + CSA_NCR5380_OFFSET;
	ncr_sc->sci_r0 = iobase + 0;
	ncr_sc->sci_r1 = iobase + 4;
	ncr_sc->sci_r2 = iobase + 8;
	ncr_sc->sci_r3 = iobase + 12;
	ncr_sc->sci_r4 = iobase + 16;
	ncr_sc->sci_r5 = iobase + 20;
	ncr_sc->sci_r6 = iobase + 24;
	ncr_sc->sci_r7 = iobase + 28;

	ncr_sc->sc_rev = NCR_VARIANT_NCR5380;

	sc->sc_ctrl = (uint8_t *)pa->pa_podule->slow_base + CSA_CTRL_OFFSET;
	sc->sc_status = (uint8_t *)pa->pa_podule->slow_base + CSA_STAT_OFFSET;
	sc->sc_data = (uint8_t *)pa->pa_podule->slow_base + CSA_DATA_OFFSET;

	ncr_sc->sc_pio_in = ncr5380_pio_in;
	ncr_sc->sc_pio_out = ncr5380_pio_out;

	/* Provide an override for the host id */
	ncr_sc->sc_channel.chan_id = 7;
	snprintf(hi_option, sizeof(hi_option), "%s.hostid", device_xname(self));
	(void)get_bootconf_option(boot_args, hi_option,
	    BOOTOPT_TYPE_INT, &ncr_sc->sc_channel.chan_id);
	ncr_sc->sc_adapter.adapt_minphys = minphys;

	aprint_normal(": host=%d, using 8 bit PIO",
	    ncr_sc->sc_channel.chan_id);

	sc->sc_irqstatus =
	    (uint8_t *)pa->pa_podule->slow_base + CSA_INTR_OFFSET;
	sc->sc_irqmask = CSA_INTR_MASK;

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");
	sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO, csa_intr, sc,
	    &sc->sc_intrcnt);
	if (sc->sc_ih == NULL)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;

	if (ncr_sc->sc_flags & NCR5380_FORCE_POLLING)
		aprint_normal(", polling");
	aprint_normal("\n");
	*sc->sc_ctrl = 0;

	ncr5380_attach(ncr_sc);
}

int
csa_intr(void *arg)
{
	struct csa_softc *sc = arg;

	if ((*sc->sc_irqstatus) & sc->sc_irqmask)
		(void)ncr5380_intr(&sc->sc_ncr5380);

	return 0;
}
