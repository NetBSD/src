/* $NetBSD: smsh_fdt.c,v 1.1.10.2 2017/12/03 11:35:52 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: smsh_fdt.c,v 1.1.10.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/lan9118var.h>
#include <dev/ic/lan9118reg.h>

static int	smsh_fdt_match(device_t, cfdata_t, void *);
static void	smsh_fdt_attach(device_t, device_t, void *);

static const char * const compatible[] = { "smsc,lan9118", NULL };

CFATTACH_DECL_NEW(smsh_fdt, sizeof(struct lan9118_softc),
	smsh_fdt_match, smsh_fdt_attach, NULL, NULL);

static int
smsh_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
}

static void
smsh_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct lan9118_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh)) {
		aprint_error(": couldn't map device\n");
		return;
	}

	if (of_hasprop(phandle, "smsc,irq-active-high"))
		sc->sc_flags |= LAN9118_FLAGS_IRQ_ACTHI;
	if (of_hasprop(phandle, "smsc,irq-push-pull"))
		sc->sc_flags |= LAN9118_FLAGS_IRQ_PP;

	if (lan9118_attach(sc) != 0)
		goto unmap;

	ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0, lan9118_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		goto unmap;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	return;

unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
}
