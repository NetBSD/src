/*	$NetBSD: pceb.c,v 1.22.14.1 2010/10/24 22:48:03 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pceb.c,v 1.22.14.1 2010/10/24 22:48:03 jym Exp $");

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

int	pcebmatch(device_t , cfdata_t, void *);
void	pcebattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pceb, 0, pcebmatch, pcebattach, NULL, NULL);

void	pceb_callback(device_t);

union pceb_attach_args {
	const char *ea_name;			/* XXX should be common */
	struct isabus_attach_args ea_iba;
	struct eisabus_attach_args ea_eba;
};

int
pcebmatch(device_t parent, cfdata_t match, void *aux)
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
pcebattach(device_t parent, device_t self, void *aux)
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

	config_defer(self, pceb_callback);
}

void
pceb_callback(device_t self)
{
	union pceb_attach_args ea;

	/*
	 * Attach the EISA bus behind this bridge.
	 */
	memset(&ea, 0, sizeof(ea));
	ea.ea_eba.eba_iot = x86_bus_space_io;
	ea.ea_eba.eba_memt = x86_bus_space_mem;
#if NEISA > 0
	ea.ea_eba.eba_dmat = &eisa_bus_dma_tag;
#endif
	config_found_ia(self, "eisabus", &ea.ea_eba, eisabusprint);

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&ea, 0, sizeof(ea));
	ea.ea_iba.iba_iot = x86_bus_space_io;
	ea.ea_iba.iba_memt = x86_bus_space_mem;
#if NISA > 0
	ea.ea_iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found_ia(self, "isabus", &ea.ea_iba, isabusprint);
}
