/*	$NetBSD: drmfb_pci.c,v 1.3.20.2 2017/12/03 11:38:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * drmfb_pci: drmfb hooks for PCI devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drmfb_pci.c,v 1.3.20.2 2017/12/03 11:38:00 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/pci/pciio.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/wsdisplay_pci.h>

#if NVGA > 0
/*
 * XXX All we really need is vga_is_console from vgavar.h, but the
 * header files are missing their own dependencies, so we need to
 * explicitly drag in the other crap.
 */
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>

#include <drm/drmfb.h>
#include <drm/drmfb_pci.h>

/*
 * drmfb_pci_mmap: Implementation of drmfb_params::dp_mmap.  Don't use
 * this for dp_mmapfb -- how to get at the framebuffer is device-
 * specific.
 */
paddr_t
drmfb_pci_mmap(struct drmfb_softc *sc, off_t offset, int prot)
{
	struct drm_device *const dev = sc->sc_da.da_fb_helper->dev;
	const struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	unsigned i;

	for (i = 0; PCI_BAR(i) <= PCI_MAPREG_ROM; i++) {
		pcireg_t type;
		bus_addr_t addr;
		bus_size_t size;
		int flags;

		/* Interrogate the BAR.  */
		if (!pci_mapreg_probe(pa->pa_pc, pa->pa_tag, PCI_BAR(i),
			&type))
			continue;
		if (PCI_MAPREG_TYPE(type) != PCI_MAPREG_TYPE_MEM)
			continue;
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, PCI_BAR(i), type,
			&addr, &size, &flags))
			continue;

		/* Try to map it if it's in range.  */
		if ((addr <= offset) && (offset < (addr + size)))
			return bus_space_mmap(pa->pa_memt, addr,
			    (offset - addr), prot, flags);

		/* Skip a slot if this was a 64-bit BAR.  */
		if ((PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_MEM) &&
		    (PCI_MAPREG_MEM_TYPE(type) == PCI_MAPREG_MEM_TYPE_64BIT))
			i += 1;
	}

	/* Failure!  */
	return -1;
}

/*
 * drmfb_pci_ioctl: Implementation of drmfb_params::dp_ioctl.  Provides:
 *
 * - WSDISPLAY_GET_BUSID
 * - WSDISPLAY_GTYPE
 *
 * Additionally allows access to PCI registers.
 *
 * XXX Do we need to provide access to PCI registers?  I can't find
 * anything that uses this functionality, and as is it is very
 * dangerous.
 */
int
drmfb_pci_ioctl(struct drmfb_softc *sc, unsigned long cmd, void *data,
    int flag, struct lwp *l)
{
	struct drm_device *const dev = sc->sc_da.da_fb_helper->dev;
	struct pci_attach_args *const pa = &dev->pdev->pd_pa;

	switch (cmd) {
	/* PCI config read/write passthrough.  */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(pa->pa_pc, pa->pa_tag, cmd, data, flag, l);

	/* PCI-specific wsdisplay ioctls.  */
	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(dev->dev, pa->pa_pc, pa->pa_tag,
		    data);
	case WSDISPLAYIO_GTYPE:
		*(unsigned int *)data = WSDISPLAY_TYPE_PCIVGA;
		return 0;

	default:
		return EPASSTHROUGH;
	}
}

bool
drmfb_pci_is_vga_console(struct drm_device *dev)
{

#if NVGA > 0
	return vga_is_console(dev->pdev->pd_pa.pa_iot, -1);
#else
	return false;
#endif
}
