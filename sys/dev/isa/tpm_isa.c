/*	$NetBSD: tpm_isa.c,v 1.4.2.1 2019/10/16 09:52:38 martin Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * Copyright (c) 2008, 2009 Michael Shalayeff
 * Copyright (c) 2009, 2010 Hans-Joerg Hoexer
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tpm_isa.c,v 1.4.2.1 2019/10/16 09:52:38 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <dev/ic/tpmreg.h>
#include <dev/ic/tpmvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "ioconf.h"

static int	tpm_isa_match(device_t, cfdata_t, void *);
static void	tpm_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tpm_isa, sizeof(struct tpm_softc), tpm_isa_match,
    tpm_isa_attach, NULL, NULL);

static int
tpm_isa_match(device_t parent, cfdata_t match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t bt = ia->ia_memt;
	bus_space_handle_t bh;
	int rv;

	/* There can be only one. */
	if (tpm_cd.cd_devs && tpm_cd.cd_devs[0])
		return 0;

	if (ia->ia_iomem[0].ir_addr == ISA_UNKNOWN_IOMEM)
		return 0;

	/* XXX: integer locator sign extension */
	if (bus_space_map(bt, (unsigned int)ia->ia_iomem[0].ir_addr,
	    TPM_SPACE_SIZE, 0, &bh))
		return 0;

	if ((rv = (*tpm_intf_tis12.probe)(bt, bh)) == 0) {
		ia->ia_nio = 0;
		ia->ia_io[0].ir_size = 0;
		ia->ia_iomem[0].ir_size = TPM_SPACE_SIZE;
	}
	ia->ia_ndrq = 0;

	bus_space_unmap(bt, bh, TPM_SPACE_SIZE);
	return (rv == 0) ? 1 : 0;
}

static void
tpm_isa_attach(device_t parent, device_t self, void *aux)
{
	struct tpm_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	bus_addr_t base;
	bus_size_t size;
	int rv;

	base = (unsigned int)ia->ia_iomem[0].ir_addr;
	size = TPM_SPACE_SIZE;

	sc->sc_dev = self;
	sc->sc_ver = TPM_1_2;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_busy = false;
	sc->sc_intf = &tpm_intf_tis12;
	sc->sc_bt = ia->ia_memt;
	if (bus_space_map(sc->sc_bt, base, size, 0, &sc->sc_bh)) {
		aprint_error_dev(sc->sc_dev, "cannot map registers\n");
		return;
	}

	if ((rv = (*sc->sc_intf->init)(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot init device, rv=%d\n", rv);
		bus_space_unmap(sc->sc_bt, sc->sc_bh, size);
		return;
	}

	if (!pmf_device_register(self, tpm_suspend, tpm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}
