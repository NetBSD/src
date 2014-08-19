/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_wdc.c,v 1.2.12.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

static int awin_wdc_match(device_t, cfdata_t, void *);
static void awin_wdc_attach(device_t, device_t, void *);

struct awin_wdc_softc {
	struct wdc_softc asc_sc;
        struct ata_channel *asc_chanlist[1];
        struct ata_channel asc_channel;
        struct ata_queue asc_chqueue;
        struct wdc_regs asc_wdc_regs;
        void    *asc_ih;

};

CFATTACH_DECL_NEW(awin_wdc, sizeof(struct awin_wdc_softc),
	awin_wdc_match, awin_wdc_attach, NULL, NULL);

static int
awin_wdc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT)
		return 0;

	return 1;
}

static void
awin_wdc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_wdc_softc * const asc = device_private(self);
	struct wdc_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	struct wdc_regs * const regs = &asc->asc_wdc_regs;

        sc->sc_atac.atac_dev = self;
	sc->regs = regs;

	regs->cmd_iot = aio->aio_core_a4x_bst;
	regs->ctl_iot = aio->aio_core_a4x_bst;
	regs->data32iot = aio->aio_core_a4x_bst;

	bus_space_subregion(regs->cmd_iot, aio->aio_core_bsh,
	    loc->loc_offset / 4, loc->loc_size, &regs->cmd_baseioh);

	aprint_naive(": ATA controller\n");
	aprint_normal(": ATA controller\n");

        for (u_int i = 0; i < 8; i++) {
                if (bus_space_subregion(regs->cmd_iot, regs->cmd_baseioh,
                    i, 1, &regs->cmd_iohs[i]) != 0) {
                        aprint_error(": couldn't subregion registers\n");
                        return;
                }
        }

        if (bus_space_subregion(regs->ctl_iot, regs->cmd_baseioh,
            AWIN_PATA_CTL_REG, 1, &regs->ctl_ioh) != 0) {
                aprint_error(": couldn't subregion registers\n");
                return;
        }


	asc->asc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL,
	    wdcintr, sc);
	if (asc->asc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	return;

fail:
	if (asc->asc_ih) {
		intr_disestablish(asc->asc_ih);
		asc->asc_ih = NULL;
	}
}
