/*	$NetBSD: pci_subr.c,v 1.8 1995/05/23 03:43:06 cgd Exp $	*/

/*
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI autoconfiguration support
 * XXX probably should be moved to pci_subr.c?
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

int
pciprint(aux, pci)
	void *aux;
	char *pci;
{
	register struct pci_attach_args *pa = aux;

	printf(" bus %d device %d", pa->pa_bus, pa->pa_device);
	return (UNCONF);
}

int
pcisubmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
	struct pci_attach_args *pa = aux;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != pa->pa_bus)
		return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != pa->pa_device)
		return 0;
	return ((*cf->cf_driver->cd_match)(parent, match, aux));
}

/*
 * Try to find and attach the PCI device at the give bus and device number.
 * Return 1 if successful, 0 if unsuccessful.
 */
int
pci_attach_subdev(pcidev, bus, device)
        struct device *pcidev;
        int bus, device;
{
	pcitag_t tag;
	pcireg_t id, class;
	struct pci_attach_args pa;
	struct cfdata *cf;

	tag = pci_make_tag(bus, device, 0);
	id = pci_conf_read(tag, PCI_ID_REG);
	if (id == 0 || id == 0xffffffff)
		return (0);
	class = pci_conf_read(tag, PCI_CLASS_REG);

	pa.pa_bus = bus;
	pa.pa_device = device;
	pa.pa_tag = tag;
	pa.pa_id = id;
	pa.pa_class = class;

	if ((cf = config_search(pcisubmatch, pcidev, &pa)) != NULL)
		config_attach(pcidev, cf, &pa, pciprint);
	else {
		printf("%s bus %d device %d: identifier %08x class %08x%s",
		    pcidev->dv_xname, bus, device, id, class,
		    " not configured\n");
		return (0);
	}

	return (1);
}
