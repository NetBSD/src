/*	$NetBSD: pci_subr.c,v 1.4 1994/11/03 22:15:25 mycroft Exp $	*/

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
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>

#include <i386/pci/pcivar.h>
#include <i386/pci/pcireg.h>

int pciprobe __P((struct device *, void *, void *));
void pciattach __P((struct device *, struct device *, void *));

struct cfdriver pcicd = {
	NULL, "pci", pciprobe, pciattach, DV_DULL, sizeof(struct device)
};

int
pciprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{

#ifdef i386
	if (pci_mode_detect() == 0)
		return 0;
#endif
	return 1;
}

int
pciprint(aux, pci)
	void *aux;
	char *pci;
{
	register struct pci_attach_args *pa = aux;

	printf("%s bus %d device %d", pci ? pci : "", pa->pa_bus,
	    pa->pa_device);
	if (pci)
		printf(": identifier %08x class %08x", pa->pa_id,
		    pa->pa_class);
	return UNCONF;
}

void
pciattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int bus, device;

#ifdef i386
	printf(": configuration mode %d\n", pci_mode);
#else
	printf("\n");
#endif

#if 0
	for (bus = 0; bus <= 255; bus++) {
#else
	/*
	 * XXX
	 * Some current chipsets do wacky things with bus numbers > 0.
	 * This seems like a violation of protocol, but the PCI BIOS does
	 * allow one to query the maximum bus number, and eventually we
	 * should do so.
	 */
	for (bus = 0; bus <= 0; bus++) {
#endif
#ifdef i386
		for (device = 0; device <= (pci_mode == 2 ? 15 : 31);
		    device++) {
#else
		for (device = 0; device <= 31; device++) {
#endif
			pcitag_t tag = pci_make_tag(bus, device, 0);
			pcireg_t id = pci_conf_read(tag, PCI_ID_REG),
				 class = pci_conf_read(tag, PCI_CLASS_REG);
			struct pci_attach_args pa;

			if (id == 0 || id == 0xffffffff)
				continue;

			pa.pa_bus = bus;
			pa.pa_device = device;
			pa.pa_tag = tag;
			pa.pa_id = id;
			pa.pa_class = class;

			(void)config_found(self, (void *)&pa, pciprint);
		}
	}
}

int
pci_targmatch(cf, pa)
	struct cfdata *cf;
	struct pci_attach_args *pa;
{

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != pa->pa_bus)
		return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != pa->pa_device)
		return 0;
	return 1;
}
