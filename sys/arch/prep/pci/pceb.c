/*	$NetBSD: pceb.c,v 1.1.40.1 2007/05/01 19:18:59 garbled Exp $	*/

/*-
 * Copyright (c) 1996, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pceb.c,v 1.1.40.1 2007/05/01 19:18:59 garbled Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/eisa/eisavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include "eisa.h"
#include "isa.h"

int	pcebmatch(struct device *, struct cfdata *, void *);
void	pcebattach(struct device *, struct device *, void *);

CFATTACH_DECL(pceb, sizeof(struct device),
    pcebmatch, pcebattach, NULL, NULL);

void	pceb_callback(struct device *);

union pceb_attach_args {
	const char *ea_name;			/* XXX should be common */
	struct isabus_attach_args ea_iba;
	struct eisabus_attach_args ea_eba;
};

int
pcebmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match anything which claims to be PCI-EISA bridge.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_EISA)
		return (1);

	/*
	 * Match some known PCI-EISA bridges explicitly.
	 * XXX this is probably not necessary, should be matched by above
	 * condition
	 */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_PCEB:
			return (1);
		}
		break;
	}

	return (0);
}

void
pcebattach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	int error;

	printf("\n");

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	prep_eisa_io_space_tag.pbs_extent = prep_io_space_tag.pbs_extent;
	error = bus_space_init(&prep_eisa_io_space_tag, "eisa-ioport", NULL, 0);
	if (error)
		panic("prep_bus_space_init: can't init eisa io tag");

	prep_eisa_mem_space_tag.pbs_extent = prep_mem_space_tag.pbs_extent;
	error = bus_space_init(&prep_eisa_mem_space_tag, "eisa-iomem", NULL, 0);
	if (error)
		panic("prep_bus_space_init: can't init eisa mem tag");

	config_defer(self, pceb_callback);
}

void
pceb_callback(struct device *self)
{
	union pceb_attach_args ea;

	/*
	 * Attach the EISA bus behind this bridge.
	 */
	memset(&ea, 0, sizeof(ea));
	ea.ea_eba.eba_iot = &prep_eisa_io_space_tag;
	ea.ea_eba.eba_memt = &prep_eisa_mem_space_tag;
#if NEISA > 0
	ea.ea_eba.eba_dmat = &eisa_bus_dma_tag;
#endif
	config_found_ia(self, "eisabus", &ea.ea_eba, eisabusprint);

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&ea, 0, sizeof(ea));
	ea.ea_iba.iba_iot = &genppc_isa_io_space_tag;
	ea.ea_iba.iba_memt = &genppc_isa_mem_space_tag;
#if NISA > 0
	ea.ea_iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found_ia(self, "isabus", &ea.ea_iba, isabusprint);
}
