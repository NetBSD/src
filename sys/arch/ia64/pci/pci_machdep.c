/*	$NetBSD: pci_machdep.c,v 1.2.18.2 2017/12/03 11:36:20 jdolecek Exp $	*/
/*
 * Copyright (c) 2009, 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.2.18.2 2017/12/03 11:36:20 jdolecek Exp $");

#include <machine/bus.h>
#include <machine/sal.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>


void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{

	/* Nothing */
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	 return 32;	/* 32 device/bus */
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{

#if DIAGNOSTIC
	if (bus >= 256 || device >= 32 || function >= 8)
		panic("%s: bad request", __func__);
#endif

	return (bus << 16) | (device << 11) | (function << 8);
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct ia64_sal_result res;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return -1;

	res = ia64_sal_entry(SAL_PCI_CONFIG_READ,
	    tag | reg, sizeof(pcireg_t), 0, 0, 0, 0, 0);
	if (res.sal_status < 0)
		return -1;
	else
		return res.sal_result[0];
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	struct ia64_sal_result res;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	res = ia64_sal_entry(SAL_PCI_CONFIG_WRITE,
	    tag | reg, sizeof(pcireg_t), val, 0, 0, 0, 0);
	if (res.sal_status < 0)
		printf("pci configuration write failed\n");
}

int
pci_enumerate_bus(struct pci_softc *sc, const int *locators,
		  int (*match)(const struct pci_attach_args *), struct pci_attach_args *pap)
{
	/* XXX implement */
	panic("ia64 pci_enumerate_bus not implemented");

	return -1;
}
