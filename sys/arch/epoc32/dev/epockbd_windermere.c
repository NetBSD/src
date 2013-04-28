/*	$NetBSD: epockbd_windermere.c,v 1.1 2013/04/28 12:11:25 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: epockbd_windermere.c,v 1.1 2013/04/28 12:11:25 kiyohara Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <epoc32/windermere/windermerereg.h>
#include <epoc32/windermere/windermerevar.h>
#include <epoc32/dev/epockbdvar.h>

#include "locators.h"

static int epockbd_windermere_match(device_t, cfdata_t, void *);
static void epockbd_windermere_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(epockbd_windermere, sizeof(struct epockbd_softc),
    epockbd_windermere_match, epockbd_windermere_attach, NULL, NULL);

static int
epockbd_windermere_match(device_t parent, cfdata_t match, void *aux)
{
	struct windermere_attach_args *aa = aux;

        /* Wildcard not accept */
	if (aa->aa_offset == WINDERMERECF_OFFSET_DEFAULT)
		return 0;

	return 1;
}

static void
epockbd_windermere_attach(device_t parent, device_t self, void *aux)
{
	struct epockbd_softc *sc = device_private(self);
	struct windermere_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;
	bus_space_handle_t ioh = *aa->aa_ioh;
	bus_addr_t gpio_addr;

	sc->sc_dev = self;
	sc->sc_kbd_ncolumn = 8;
	sc->sc_kbd_nrow = 7;
	if (windermere_bus_space_subregion(iot, ioh, aa->aa_offset,
					sizeof(uint32_t), &sc->sc_scanh) != 0) {
		aprint_error_dev(self, "can't map scan registers\n");
		return;
	}
	gpio_addr = aa->aa_offset & ~0xff;	/* XXXX */
	if (windermere_bus_space_subregion(iot, ioh, gpio_addr,
					sizeof(uint32_t), &sc->sc_datah) != 0) {
		aprint_error_dev(self, "can't map data registers\n");
		return;
	}
	sc->sc_scant = iot;
	sc->sc_datat = iot;

	epockbd_attach(sc);
}
