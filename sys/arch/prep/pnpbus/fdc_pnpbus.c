/*	$NetBSD: fdc_pnpbus.c,v 1.4.6.1 2017/12/03 11:36:38 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: fdc_pnpbus.c,v 1.4.6.1 2017/12/03 11:36:38 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/queue.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/isa_machdep.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/fdcvar.h>

#include <prep/pnpbus/pnpbusvar.h>

static int	fdc_pnpbus_match(device_t, cfdata_t, void *);
static void	fdc_pnpbus_attach(device_t, device_t, void *);

struct fdc_pnpbus_softc {
        struct fdc_softc sc_fdc;        /* base fdc device */

        bus_space_handle_t sc_baseioh;  /* base I/O handle */
};

CFATTACH_DECL2_NEW(fdc_pnpbus, sizeof(struct fdc_pnpbus_softc),
    fdc_pnpbus_match, fdc_pnpbus_attach, NULL, NULL, NULL, NULL);

static int
fdc_pnpbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	if (strcmp(pna->pna_devid, "PNP0700") == 0)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);

	return ret;
}

static void
fdc_pnpbus_attach(device_t parent, device_t self, void *aux)
{
        struct fdc_pnpbus_softc *pdc = device_private(self);
	struct fdc_softc *fdc = &pdc->sc_fdc;
	struct pnpbus_dev_attach_args *pna = aux;
        int size, base;
        
	fdc->sc_dev = self;
	fdc->sc_ic = pna->pna_ic;

	if (pnpbus_io_map(&pna->pna_res, 0, &fdc->sc_iot, &pdc->sc_baseioh)) {
		aprint_error_dev(self, "unable to map I/O space\n");
		return;
	}

	/* 
         * Start accounting for "odd" pnpbus's. Some probe as 4 ports,
         * some as 6 and some don't give the ctl port back. 
         */

        if (pnpbus_getioport(&pna->pna_res, 0, &base, &size)) {
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
         * XXX: This is bad. If the pnpbus claims only 1 I/O range then it's
         * omitting the controller I/O port. (One has to exist for there to
         * be a working fdc). Just try and force the mapping in. 
         */

        if (pna->pna_res.numio == 1) {
                if (bus_space_map(fdc->sc_iot, base + size + 1, 1, 0,
                    &fdc->sc_fdctlioh)) {
                        aprint_error_dev(self,
			    "unable to force map CTL I/O space\n");
                        return;
                }
        } else if (pnpbus_io_map(&pna->pna_res, 1, &fdc->sc_iot,
		   &fdc->sc_fdctlioh)) {
                aprint_error_dev(self, "unable to map CTL I/O space\n");
		return;
	}

	if (pnpbus_getdmachan(&pna->pna_res, 0, &fdc->sc_drq)) {
		aprint_error_dev(self, "unable to get DMA channel\n");
		return;
	}

        if (pna->pna_res.numio == 1)
                aprint_normal_dev(self,
		    "ctl io %x didn't probe. Forced attach\n", base + size + 1);

	aprint_normal("\n");

	/* The 7043-140 gets the type wrong, so overide to edge allways */
	fdc->sc_ih = pnpbus_intr_establish(0, IPL_BIO, IST_EDGE, fdcintr, fdc,
	    &pna->pna_res);

	fdcattach(fdc);
}
