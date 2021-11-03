/* $NetBSD: virtio_mainbus.c,v 1.4 2021/11/03 07:53:56 skrll Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: virtio_mainbus.c,v 1.4 2021/11/03 07:53:56 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/device.h>
#include <evbmips/mipssim/autoconf.h>
#include <evbmips/mipssim/mipssimreg.h>

#define VIRTIO_PRIVATE
#include <dev/virtio/virtio_mmiovar.h>


static int	virtio_mainbus_match(device_t, cfdata_t, void *);
static void	virtio_mainbus_attach(device_t, device_t, void *);
static int	virtio_mainbus_rescan(device_t, const char *, const int *);
static int	virtio_mainbus_detach(device_t, int);

static int	virtio_mainbus_alloc_interrupts(struct virtio_mmio_softc *);
static void	virtio_mainbus_free_interrupts(struct virtio_mmio_softc *);

struct virtio_mainbus_softc {
	struct virtio_mmio_softc sc_msc;

	int		 sc_irq;
	void		*sc_ih;
};


CFATTACH_DECL3_NEW(virtio_mainbus, sizeof(struct virtio_mainbus_softc),
    virtio_mainbus_match, virtio_mainbus_attach,
    virtio_mainbus_detach, NULL,
    virtio_mainbus_rescan, (void *)voidop, DVF_DETACH_SHUTDOWN);


static int
virtio_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* TODO can we check here if its present? */
	if (strcmp(ma->ma_name, match->cf_name) == 0)
		return (1<<1);

	return (0);
}


void
virtio_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct virtio_mainbus_softc *sc = device_private(self);
	struct virtio_mmio_softc *msc = &sc->sc_msc;
	struct virtio_softc *vsc = &msc->sc_sc;
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t ioh;

	if (bus_space_map(ma->ma_iot, ma->ma_addr, VIRTIO_STRIDE, 0, &ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_irq = ma->ma_irq;

	msc->sc_iot = ma->ma_iot;
	msc->sc_ioh = ioh;
	msc->sc_iosize = VIRTIO_STRIDE;
	msc->sc_alloc_interrupts = virtio_mainbus_alloc_interrupts;
	msc->sc_free_interrupts  = virtio_mainbus_free_interrupts;

	vsc->sc_dev = self;
	vsc->sc_dmat = ma->ma_dmat;
	if (virtio_mmio_common_probe_present(msc))
		virtio_mmio_common_attach(msc);

	virtio_mainbus_rescan(self, NULL, NULL);
}


/* ARGSUSED */
static int
virtio_mainbus_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct virtio_mainbus_softc *sc = device_private(self);
	struct virtio_mmio_softc *msc = &sc->sc_msc;
	struct virtio_softc *vsc = &msc->sc_sc;
	struct virtio_attach_args va;

	if (vsc->sc_child)	/* child already attached? */
		return 0;

	memset(&va, 0, sizeof(va));
	va.sc_childdevid = vsc->sc_childdevid;

	config_found(self, &va, NULL, CFARGS_NONE);

	if (virtio_attach_failed(vsc))
		return 0;
	return 0;
}


static int
virtio_mainbus_detach(device_t self, int flags)
{
	struct virtio_mainbus_softc *sc = device_private(self);
	struct virtio_mmio_softc * const msc = &sc->sc_msc;

	return virtio_mmio_common_detach(msc, flags);
}


static int
virtio_mainbus_alloc_interrupts(struct virtio_mmio_softc *msc)
{
	struct virtio_mainbus_softc *sc = (struct virtio_mainbus_softc *) msc;
	struct virtio_softc * const vsc = &msc->sc_sc;

	msc->sc_ih = evbmips_intr_establish(sc->sc_irq, virtio_mmio_intr,
		msc);
	if (msc->sc_ih == NULL) {
		aprint_error_dev(vsc->sc_dev,
			"couldn't install interrupt handler\n");
		return -1;
	}

	aprint_normal_dev(vsc->sc_dev, "interrupting at irq %d\n", sc->sc_irq);

	return 0;
}


static void
virtio_mainbus_free_interrupts(struct virtio_mmio_softc *msc)
{
	panic("%s not implemented\n", __func__);
}
