/*	$NetBSD: pci_machdep.c,v 1.1 1995/06/28 01:25:56 cgd Exp $	*/

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
 * Machine-specific functions for PCI autoconfiguration.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/vm.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <alpha/pci/pci_chipset.h>

int pcimatch __P((struct device *, void *, void *));
void pciattach __P((struct device *, struct device *, void *));

struct cfdriver pcicd = {
	NULL, "pci", pcimatch, pciattach, DV_DULL, sizeof(struct device)
};

int
pcimatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{

	return 1;
}

void
pciattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	printf("\n");
	(*pci_cs_fcns->cs_setup)();
	(*pci_cfg_fcns->cfg_attach)(parent, self, aux);
}

pcitag_t
pci_make_tag(bus, device, function)
	int bus, device, function;
{

	return (*pci_cs_fcns->cs_make_tag)(bus, device, function);
}

pcireg_t
pci_conf_read(tag, offset)
	pcitag_t tag;
	int offset;					/* XXX */
{

	return (*pci_cs_fcns->cs_conf_read)(tag, offset);
}

void
pci_conf_write(tag, offset, data)
	pcitag_t tag;
	int offset;					/* XXX */
	pcireg_t data;
{

	(*pci_cs_fcns->cs_conf_write)(tag, offset, data);
}

int
pci_map_io(tag, reg, iobasep)
	pcitag_t tag;
	int reg;
	int *iobasep;
{

	/*
	 * XXX should be a chipset-dependent function, but...
	 * what would it do, and what would use it?
	 */
	panic("pci_map_io: not implemented");
}

int
pci_map_mem(tag, reg, vap, pap)
	pcitag_t tag;
	int reg;
	vm_offset_t *vap, *pap;
{

	return (*pci_cs_fcns->cs_map_mem)(tag, reg, vap, pap);
}

int
pcidma_map(addr, size, mappings)
	caddr_t addr;
	vm_size_t size;
	vm_offset_t *mappings;
{

	return (*pci_cs_fcns->cs_pcidma_map)(addr, size, mappings);
}

void
pcidma_unmap(addr, size, nmappings, mappings)
	caddr_t addr;
	vm_size_t size;
	int nmappings;
	vm_offset_t *mappings;
{

	(*pci_cs_fcns->cs_pcidma_unmap)(addr, size, nmappings, mappings);
}

void *
pci_map_int(tag, level, func, arg)
	pcitag_t tag;
	pci_intrlevel level;
	int (*func) __P((void *));
	void *arg;
{
	pcireg_t data;
	int pin;

	data = pci_conf_read(tag, PCI_INTERRUPT_REG);

	pin = PCI_INTERRUPT_PIN(data);

	if (pin == 0) {
		/* No IRQ used. */
		return 0;
	}

	if (pin > 4) {
		printf("pci_map_int: bad interrupt pin %d\n", pin);
		return NULL;
	}

	return (*pci_cfg_fcns->cfg_map_int)(tag, level, func, arg, pin);
}

isa_intrlevel
pcilevel_to_isa(level)
	pci_intrlevel level;
{

	switch (level) {
	case PCI_IPL_NONE:
		return (ISA_IPL_NONE);

	case PCI_IPL_BIO:
		return (ISA_IPL_BIO);

	case PCI_IPL_NET:
		return (ISA_IPL_NET);

	case PCI_IPL_TTY:
		return (ISA_IPL_TTY);

	case PCI_IPL_CLOCK:
		return (ISA_IPL_CLOCK);

	default:
		panic("pcilevel_to_isa: unknown level %d\n", level);
	}
}
