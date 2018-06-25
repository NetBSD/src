/* $NetBSD: virtio_mmio_fdt.c,v 1.2.2.2 2018/06/25 07:25:49 pgoyette Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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
__KERNEL_RCSID(0, "$NetBSD: virtio_mmio_fdt.c,v 1.2.2.2 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#define VIRTIO_PRIVATE
#include <dev/virtio/virtio_mmiovar.h>

static int	virtio_mmio_fdt_match(device_t, cfdata_t, void *);
static void	virtio_mmio_fdt_attach(device_t, device_t, void *);
static int	virtio_mmio_fdt_rescan(device_t, const char *, const int *);
static int	virtio_mmio_fdt_detach(device_t, int);

static int	virtio_mmio_fdt_setup_interrupts(struct virtio_mmio_softc *);
static void	virtio_mmio_fdt_free_interrupts(struct virtio_mmio_softc *);

struct virtio_mmio_fdt_softc {
	struct virtio_mmio_softc	sc_msc;
	int				sc_phandle;
};

CFATTACH_DECL3_NEW(virtio_mmio_fdt, sizeof(struct virtio_mmio_fdt_softc),
    virtio_mmio_fdt_match, virtio_mmio_fdt_attach, virtio_mmio_fdt_detach, NULL,
    virtio_mmio_fdt_rescan, (void *)voidop, DVF_DETACH_SHUTDOWN);

static const char * const compatible[] = {
	"virtio,mmio",
	NULL
};

static int
virtio_mmio_fdt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
virtio_mmio_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct virtio_mmio_fdt_softc * const fsc = device_private(self);
	struct virtio_mmio_softc * const msc = &fsc->sc_msc;
	struct virtio_softc * const vsc = &msc->sc_sc;
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	aprint_normal("\n");
	aprint_naive("\n");

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(self, "couldn't get registers\n");
		return;
	}

	fsc->sc_phandle = faa->faa_phandle;
	msc->sc_iot = faa->faa_bst;
	vsc->sc_dev = self;
	vsc->sc_dmat = faa->faa_dmat;

	error = bus_space_map(msc->sc_iot, addr, size, 0, &msc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map %#llx: %d",
		    (uint64_t)addr, error);
		return;
	}
	msc->sc_iosize = size;

	msc->sc_setup_interrupts = virtio_mmio_fdt_setup_interrupts;
	msc->sc_free_interrupts = virtio_mmio_fdt_free_interrupts;

	virtio_mmio_common_attach(msc);
	virtio_mmio_fdt_rescan(self, "virtio", NULL);
}

/* ARGSUSED */
static int
virtio_mmio_fdt_rescan(device_t self, const char *attr, const int *scan_flags)
{
	struct virtio_mmio_fdt_softc * const fsc = device_private(self);
	struct virtio_mmio_softc * const msc = &fsc->sc_msc;
	struct virtio_softc * const vsc = &msc->sc_sc;
	struct virtio_attach_args va;

	if (vsc->sc_child)	/* Child already attached? */
		return 0;
	memset(&va, 0, sizeof(va));
	va.sc_childdevid = vsc->sc_childdevid;

	config_found_ia(self, attr, &va, virtiobusprint);

	if (vsc->sc_child == NULL) {
		return 0;
	}

	if (vsc->sc_child == VIRTIO_CHILD_FAILED) {
		aprint_error_dev(self, "virtio configuration failed\n");
		return 0;
	}

	/*
	 * Make sure child drivers initialize interrupts via call
	 * to virtio_child_attach_finish().
	 */
	KASSERT(msc->sc_ih != NULL);

	return 0;
}

static int
virtio_mmio_fdt_detach(device_t self, int flags)
{
	struct virtio_mmio_fdt_softc * const fsc = device_private(self);
	struct virtio_mmio_softc * const msc = &fsc->sc_msc;

	return virtio_mmio_common_detach(msc, flags);
}

static int
virtio_mmio_fdt_setup_interrupts(struct virtio_mmio_softc *msc)
{
	struct virtio_mmio_fdt_softc * const fsc = (void *)msc;
	struct virtio_softc * const vsc = &msc->sc_sc;
	char intrstr[128];
	int flags = 0;

	if (!fdtbus_intr_str(fsc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(vsc->sc_dev, "failed to decode interrupt\n");
		return -1;
	}

	if (vsc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		flags |= FDT_INTR_MPSAFE;

	msc->sc_ih = fdtbus_intr_establish(fsc->sc_phandle, 0, vsc->sc_ipl,
	    flags, virtio_mmio_intr, msc);
	if (msc->sc_ih == NULL) {
		aprint_error_dev(vsc->sc_dev,
		    "failed to establish interrupt on %s\n", intrstr);
		return -1;
	}
	aprint_normal_dev(vsc->sc_dev, "interrupting on %s\n", intrstr);

	return 0;
}

static void
virtio_mmio_fdt_free_interrupts(struct virtio_mmio_softc *msc)
{
	struct virtio_mmio_fdt_softc * const fsc = (void *)msc;

	if (msc->sc_ih != NULL) {
		fdtbus_intr_disestablish(fsc->sc_phandle, msc->sc_ih);
		msc->sc_ih = NULL;
	}
}
