/*	$NetBSD: csa.c,v 1.12 2001/07/04 17:54:18 bjh21 Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Cumana SCSI 1 driver using the generic NCR5380 driver
 */

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
#include <machine/irqhandler.h>
#include <machine/bootconfig.h>

#include <arm32/podulebus/podulebus.h>
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

void csa_attach __P((struct device *, struct device *, void *));
int  csa_match  __P((struct device *, struct cfdata *, void *));

/*
 * Cumana SCSI 1 softc structure.
 *
 * Contains the generic ncr5380 device node, podule information and global information
 * required by the driver.
 */

struct csa_softc {
	struct ncr5380_softc	sc_ncr5380;
	void			*sc_ih;
	struct evcnt		sc_intrcnt;
	int			sc_podule_number;
	podule_t		*sc_podule;
	volatile u_char		*sc_irqstatus;
	u_char			sc_irqmask;
	volatile u_char		*sc_ctrl;
	volatile u_char		*sc_status;
	volatile u_char		*sc_data;
};

struct cfattach csa_ca = {
	sizeof(struct csa_softc), csa_match, csa_attach
};

int csa_intr		 __P((void *arg));

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
csa_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct podule_attach_args *pa = aux;

	if (matchpodule(pa, MANUFACTURER_CUMANA, PODULE_CUMANA_SCSI1, -1))
		return(1);

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
csa_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct csa_softc *sc = (struct csa_softc *)self;
	struct podule_attach_args *pa = aux;
	u_char *iobase;
	char hi_option[sizeof(sc->sc_ncr5380.sc_dev.dv_xname) + 8];

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sc->sc_ncr5380.sc_flags |= NCR5380_FORCE_POLLING;
	sc->sc_ncr5380.sc_min_dma_len = 0;
	sc->sc_ncr5380.sc_no_disconnect = 0x00;
	sc->sc_ncr5380.sc_parity_disable = 0x00;

	sc->sc_ncr5380.sc_dma_alloc = NULL;
	sc->sc_ncr5380.sc_dma_free = NULL;
	sc->sc_ncr5380.sc_dma_poll = NULL;
	sc->sc_ncr5380.sc_dma_setup = NULL;
	sc->sc_ncr5380.sc_dma_start = NULL;
	sc->sc_ncr5380.sc_dma_eop = NULL;
	sc->sc_ncr5380.sc_dma_stop = NULL;
	sc->sc_ncr5380.sc_intr_on = NULL;
	sc->sc_ncr5380.sc_intr_off = NULL;

	iobase = (u_char *)pa->pa_podule->slow_base + CSA_NCR5380_OFFSET;
	sc->sc_ncr5380.sci_r0 = iobase + 0;
	sc->sc_ncr5380.sci_r1 = iobase + 4;
	sc->sc_ncr5380.sci_r2 = iobase + 8;
	sc->sc_ncr5380.sci_r3 = iobase + 12;
	sc->sc_ncr5380.sci_r4 = iobase + 16;
	sc->sc_ncr5380.sci_r5 = iobase + 20;
	sc->sc_ncr5380.sci_r6 = iobase + 24;
	sc->sc_ncr5380.sci_r7 = iobase + 28;

	sc->sc_ncr5380.sc_rev = NCR_VARIANT_NCR5380;

	sc->sc_ctrl = (u_char *)pa->pa_podule->slow_base + CSA_CTRL_OFFSET;
	sc->sc_status = (u_char *)pa->pa_podule->slow_base + CSA_STAT_OFFSET;
	sc->sc_data = (u_char *)pa->pa_podule->slow_base + CSA_DATA_OFFSET;

	sc->sc_ncr5380.sc_pio_in = ncr5380_pio_in;
	sc->sc_ncr5380.sc_pio_out = ncr5380_pio_out;

	/* Provide an override for the host id */
	sc->sc_ncr5380.sc_channel.chan_id = 7;
	sprintf(hi_option, "%s.hostid", sc->sc_ncr5380.sc_dev.dv_xname);
	(void)get_bootconf_option(boot_args, hi_option,
	    BOOTOPT_TYPE_INT, &sc->sc_ncr5380.sc_channel.chan_id);
	sc->sc_ncr5380.sc_adapter.adapt_minphys = minphys;

	printf(": host=%d, using 8 bit PIO",
	    sc->sc_ncr5380.sc_channel.chan_id);

	sc->sc_irqstatus = (u_char *)pa->pa_podule->slow_base + CSA_INTR_OFFSET;
	sc->sc_irqmask = CSA_INTR_MASK;

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");
	sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO, csa_intr, sc,
	    &sc->sc_intrcnt);
	if (sc->sc_ih == NULL)
		sc->sc_ncr5380.sc_flags |= NCR5380_FORCE_POLLING;

	if (sc->sc_ncr5380.sc_flags & NCR5380_FORCE_POLLING)
		printf(", polling");
	printf("\n");
	*sc->sc_ctrl = 0;

	ncr5380_attach(&sc->sc_ncr5380);
}

int
csa_intr(arg)
	void *arg;
{
	struct csa_softc *sc = arg;

	if ((*sc->sc_irqstatus) & sc->sc_irqmask)
		(void)ncr5380_intr(&sc->sc_ncr5380);

	return(0);
}
