/*	$NetBSD: pcmb.c,v 1.18.14.1 2010/10/24 22:48:04 jym Exp $	*/

/*-
 * Copyright (c) 1996, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Jaromir Dolecek.
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
 * "Driver" for PCI-MCA Bridges.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcmb.c,v 1.18.14.1 2010/10/24 22:48:04 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/mca/mcavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include "mca.h"

int	pcmbmatch(device_t, cfdata_t, void *);
void	pcmbattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pcmb, 0, pcmbmatch, pcmbattach, NULL, NULL);

void	pcmb_callback(device_t);

int
pcmbmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match anything which claims to be PCI-MCA bridge.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE
	    && PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_MC)
		return (1);

	return (0);
}

void
pcmbattach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	aprint_naive("\n");
	aprint_normal("\n");

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal_dev(self, "%s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	config_defer(self, pcmb_callback);
}

void
pcmb_callback(device_t self)
{
	struct mcabus_attach_args ma;

	/*
	 * Attach MCA bus behind this bridge.
	 */
	ma.mba_iot = x86_bus_space_io;
	ma.mba_memt = x86_bus_space_mem;
#if NMCA > 0
	ma.mba_dmat = &mca_bus_dma_tag;
#endif
	ma.mba_mc = NULL;
	ma.mba_bus = 0;
	config_found_ia(self, "mcabus", &ma, mcabusprint);
}
