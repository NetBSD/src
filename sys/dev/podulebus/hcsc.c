/*	$NetBSD: hcsc.c,v 1.1 2001/05/26 23:01:19 bjh21 Exp $	*/

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
 * HCCS 8-bit SCSI driver using the generic NCR5380 driver
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: hcsc.c,v 1.1 2001/05/26 23:01:19 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/bootconfig.h>

#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>

void hcsc_attach (struct device *, struct device *, void *);
int  hcsc_match  (struct device *, struct cfdata *, void *);

/*
 * HCCS 8-bit SCSI softc structure.
 *
 * Contains the generic ncr5380 device node, podule information and
 * global information required by the driver.
 */

struct hcsc_softc {
	struct ncr5380_softc	sc_ncr5380;
	void		*sc_ih;
	struct evcnt	sc_intrcnt;
};

struct cfattach hcsc_ca = {
	sizeof(struct hcsc_softc), hcsc_match, hcsc_attach
};

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
hcsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	if (matchpodule(pa, MANUFACTURER_HCCS, PODULE_HCCS_IDESCSI, -1) == 0)
		return(0);

	return(1);
}

/*
 * Card attach function
 *
 */

void
hcsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct hcsc_softc *sc = (struct hcsc_softc *)self;
	struct podulebus_attach_args *pa = aux;
	u_char *iobase;
	char hi_option[sizeof(sc->sc_ncr5380.sc_dev.dv_xname) + 8];

	sc->sc_ncr5380.sc_min_dma_len = 0;
	sc->sc_ncr5380.sc_no_disconnect = 0xff;
	sc->sc_ncr5380.sc_parity_disable = 0xff;

	sc->sc_ncr5380.sc_dma_alloc = NULL;
	sc->sc_ncr5380.sc_dma_free = NULL;
	sc->sc_ncr5380.sc_dma_poll = NULL;
	sc->sc_ncr5380.sc_dma_setup = NULL;
	sc->sc_ncr5380.sc_dma_start = NULL;
	sc->sc_ncr5380.sc_dma_eop = NULL;
	sc->sc_ncr5380.sc_dma_stop = NULL;
	sc->sc_ncr5380.sc_intr_on = NULL;
	sc->sc_ncr5380.sc_intr_off = NULL;

#ifdef NCR5380_USE_BUS_SPACE
	sc->sc_ncr5380.sc_regt = pa->pa_fast_t;
	bus_space_map(sc->sc_ncr5380.sc_regt, pa->pa_fast_base + 0x2000, 8, 0,
	    &sc->sc_ncr5380.sc_regh);
	sc->sc_ncr5380.sci_r0 = 0;
	sc->sc_ncr5380.sci_r1 = 1;
	sc->sc_ncr5380.sci_r2 = 2;
	sc->sc_ncr5380.sci_r3 = 3;
	sc->sc_ncr5380.sci_r4 = 4;
	sc->sc_ncr5380.sci_r5 = 5;
	sc->sc_ncr5380.sci_r6 = 6;
	sc->sc_ncr5380.sci_r7 = 7;
#else
	iobase = (u_char *)pa->pa_fast_base + 0x2000;
	sc->sc_ncr5380.sci_r0 = iobase + 0;
	sc->sc_ncr5380.sci_r1 = iobase + 4;
	sc->sc_ncr5380.sci_r2 = iobase + 8;
	sc->sc_ncr5380.sci_r3 = iobase + 12;
	sc->sc_ncr5380.sci_r4 = iobase + 16;
	sc->sc_ncr5380.sci_r5 = iobase + 20;
	sc->sc_ncr5380.sci_r6 = iobase + 24;
	sc->sc_ncr5380.sci_r7 = iobase + 28;
#endif

	sc->sc_ncr5380.sc_rev = NCR_VARIANT_DP8490;

	sc->sc_ncr5380.sc_pio_in = ncr5380_pio_in;
	sc->sc_ncr5380.sc_pio_out = ncr5380_pio_out;

	/* Provide an override for the host id */
	sc->sc_ncr5380.sc_channel.chan_id = 7;
	sprintf(hi_option, "%s.hostid", sc->sc_ncr5380.sc_dev.dv_xname);
	(void)get_bootconf_option(boot_args, hi_option,
	    BOOTOPT_TYPE_INT, &sc->sc_ncr5380.sc_channel.chan_id);
	sc->sc_ncr5380.sc_adapter.adapt_minphys = minphys;

	printf(": host=%d, using 8 bit PIO\n",
	    sc->sc_ncr5380.sc_channel.chan_id);

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");
	sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO, ncr5380_intr,
	    sc, &sc->sc_intrcnt);

	ncr5380_attach(&sc->sc_ncr5380);
}
