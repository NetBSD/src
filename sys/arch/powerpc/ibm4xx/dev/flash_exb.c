/*	$Id: flash_exb.c,v 1.1.2.1 2010/08/11 13:56:28 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Masao Uebayashi.
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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/flashvar.h>

#include <powerpc/ibm4xx/dev/exbvar.h>

static int flash_match(device_t, struct cfdata *, void *);
static void flash_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(flash_exb, sizeof(struct flash_softc),
    flash_match, flash_attach, NULL, NULL);

static int
flash_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct exb_conf *ec = aux;

	if (strcmp(ec->ec_name, "flash") == 0)
		return 1;

	return 0;
}

static void
flash_attach(device_t parent, device_t self, void *aux)
{
	struct flash_softc *sc = device_private(self);
	struct exb_conf *ec = aux;

	sc->sc_addr = ec->ec_addr;
	sc->sc_size = ec->ec_size;
	sc->sc_iot = ec->ec_bust;

	flash_init(sc);
}
