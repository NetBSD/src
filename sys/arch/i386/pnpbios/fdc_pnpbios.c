/*	$NetBSD: fdc_pnpbios.c,v 1.14.14.1 2010/10/24 22:48:04 jym Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * PNPBIOS attachment for the PC Floppy Controller driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdc_pnpbios.c,v 1.14.14.1 2010/10/24 22:48:04 jym Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/queue.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/fdcvar.h>

#include <i386/pnpbios/pnpbiosvar.h>

int	fdc_pnpbios_match(device_t, cfdata_t, void *);
void	fdc_pnpbios_attach(device_t, device_t, void *);

struct fdc_pnpbios_softc {
        struct fdc_softc sc_fdc;        /* base fdc device */

        bus_space_handle_t sc_baseioh;  /* base I/O handle */
};


CFATTACH_DECL_NEW(fdc_pnpbios, sizeof(struct fdc_pnpbios_softc),
    fdc_pnpbios_match, fdc_pnpbios_attach, NULL, NULL);

int
fdc_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "PNP0700") == 0)
		return (1);

	return (0);
}

void
fdc_pnpbios_attach(device_t parent, device_t self, void *aux)
{
        struct fdc_pnpbios_softc *pdc = device_private(self);
	struct fdc_softc *fdc = &pdc->sc_fdc;
	struct pnpbiosdev_attach_args *aa = aux;
        int size, base;
        
	aprint_normal("\n");

	fdc->sc_dev = self;
	fdc->sc_ic = aa->ic;

	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &fdc->sc_iot,
            &pdc->sc_baseioh)) {
		aprint_error_dev(self, "unable to map I/O space\n");
		return;
	}

	/* 
         * Start accounting for "odd" pnpbios's. Some probe as 4 ports,
         * some as 6 and some don't give the ctl port back. 
         */

        if (pnpbios_getiosize(aa->pbt, aa->resc, 0, &size)) {
                aprint_error_dev(self, "can't get iobase size\n");
                return;
        }

        switch (size) {

        /* Easy case. This matches right up with what the fdc code expects. */
        case 4:
                fdc->sc_ioh = pdc->sc_baseioh;
                break;

        /* Map a subregion so this all lines up with the fdc code. */
        case 6:
                if (bus_space_subregion(fdc->sc_iot, pdc->sc_baseioh, 2, 4,
                    &fdc->sc_ioh)) {
                        aprint_error_dev(self,
			    "unable to subregion I/O space\n");
                        return;
                }
                break;
        default:
                aprint_error_dev(self, "unknown size: %d of io mapping\n", size);
                return;
        }
        
        /* 
         * XXX: This is bad. If the pnpbios claims only 1 I/O range then it's
         * omitting the controller I/O port. (One has to exist for there to
         * be a working fdc). Just try and force the mapping in. 
         */

        if (aa->resc->numio == 1) {

                if (pnpbios_getiobase(aa->pbt, aa->resc, 0, 0, &base)) {
                        aprint_error_dev(self, "can't get iobase\n");
                        return;
                }
                if (bus_space_map(fdc->sc_iot, base + size + 1, 1, 0,
                    &fdc->sc_fdctlioh)) {
                        aprint_error_dev(self,
			    "unable to force map CTL I/O space\n");
                        return;
                }
        } else if (pnpbios_io_map(aa->pbt, aa->resc, 1, &fdc->sc_iot,
            &fdc->sc_fdctlioh)) {
                aprint_error_dev(self, "unable to map CTL I/O space\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &fdc->sc_drq)) {
		aprint_error_dev(self, "unable to get DMA channel\n");
		return;
	}

	pnpbios_print_devres(self, aa);
        if (aa->resc->numio == 1)
                aprint_error_dev(self,
		    "ctl io %x didn't probe. Forced attach\n",
		    base + size + 1);

	fdc->sc_ih = pnpbios_intr_establish(aa->pbt, aa->resc, 0, IPL_BIO,
	    fdcintr, fdc);

	fdcattach(fdc);
}
