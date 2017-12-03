/* $NetBSD: bcm2835_com.c,v 1.3.4.2 2017/12/03 11:35:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_com.c,v 1.3.4.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <dev/ic/comvar.h>

static int	bcm_com_match(device_t, cfdata_t, void *);
static void	bcm_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmcom, sizeof(struct com_softc),
	bcm_com_match, bcm_com_attach, NULL, NULL);

static int
bcm_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args *aaa = aux;

	return strcmp(aaa->aaa_name, "com") == 0;
}

static void
bcm_com_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc * const sc = device_private(self);
	struct amba_attach_args * const aaa = aux;
	const prop_dictionary_t dict = device_properties(self);
	bus_space_tag_t bst = &bcm2835_a4x_bs_tag;
	bus_space_handle_t bsh;
	void *ih;

	sc->sc_dev = self;
	sc->sc_type = COM_TYPE_BCMAUXUART;

	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_frequency);
	if (sc->sc_frequency == 0) {
		aprint_error(": couldn't get frequency\n");
		return;
	}
	sc->sc_frequency *= 2;

	if (com_is_console(bst, aaa->aaa_addr, &bsh) == 0 &&
	    bus_space_map(bst, aaa->aaa_addr, aaa->aaa_size, 0, &bsh) != 0) {
		aprint_error(": can't map device\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, bst, bsh, aaa->aaa_addr);

	com_attach_subr(sc);
	aprint_naive("\n");

	ih = intr_establish(aaa->aaa_intr, IPL_SERIAL, IST_LEVEL | IST_MPSAFE,
	    comintr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    aaa->aaa_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on intr %d\n", aaa->aaa_intr);
}
