/*	$NetBSD: pcmb.c,v 1.14 2006/10/12 01:30:43 christos Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pcmb.c,v 1.14 2006/10/12 01:30:43 christos Exp $");

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

int	pcmbmatch(struct device *, struct cfdata *, void *);
void	pcmbattach(struct device *, struct device *, void *);

CFATTACH_DECL(pcmb, sizeof(struct device),
    pcmbmatch, pcmbattach, NULL, NULL);

void	pcmb_callback(struct device *);

int
pcmbmatch(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
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
pcmbattach(struct device *parent __unused, struct device *self, void *aux)
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
	aprint_normal("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	config_defer(self, pcmb_callback);
}

void
pcmb_callback(struct device *self)
{
	struct mcabus_attach_args ma;

	/*
	 * Attach MCA bus behind this bridge.
	 */
	ma.mba_iot = X86_BUS_SPACE_IO;
	ma.mba_memt = X86_BUS_SPACE_MEM;
#if NMCA > 0
	ma.mba_dmat = &mca_bus_dma_tag;
#endif
	ma.mba_mc = NULL;
	ma.mba_bus = 0;
	config_found_ia(self, "mcabus", &ma, mcabusprint);
}
