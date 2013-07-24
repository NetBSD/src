/*	$NetBSD: pci.h,v 1.1.2.4 2013/07/24 03:01:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_PCI_H_
#define _LINUX_PCI_H_

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <linux/ioport.h>

struct pci_bus;
struct pci_device_id;

struct pci_dev {
	struct pci_bus *bus;
	struct pci_attach_args pd_pa;
};

#define	PCI_CAP_ID_AGP	PCI_CAP_AGP

static inline int
pci_find_capability(struct pci_dev *pdev, int cap)
{
	return pci_get_capability(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, cap,
	    NULL, NULL);
}

static inline void
pci_read_config_dword(struct pci_dev *pdev, int reg, uint32_t *valuep)
{
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg);
}

static inline void
pci_write_config_dword(struct pci_dev *pdev, int reg, uint32_t value)
{
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, value);
}

#define	PCIBIOS_MIN_MEM	0	/* XXX bogus x86 kludge bollocks */

static inline bus_addr_t
pcibios_align_resource(void *p, const struct resource *resource,
    bus_addr_t addr, bus_size_t size)
{
	panic("pcibios_align_resource has accessed unaligned neurons!");
}

static inline int
pci_bus_alloc_resource(struct pci_bus *bus, struct resource *resource,
    bus_size_t size, bus_size_t align, bus_addr_t start, int type __unused,
    bus_addr_t (*align_fn)(void *, const struct resource *, bus_addr_t,
	bus_size_t) __unused,
    struct pci_dev *pdev)
{
	const struct pci_attach_args *const pa = &pdev->pd_pa;
	bus_space_tag_t bst;
	int error;

	switch (resource->flags) {
	case IORESOURCE_MEM:
		bst = pa->pa_memt;
		break;

	case IORESOURCE_IO:
		bst = pa->pa_iot;
		break;

	default:
		panic("I don't know what kind of resource you want!");
	}

	resource->r_bst = bst;
	error = bus_space_alloc(bst, start, 0xffffffffffffffffULL /* XXX */,
	    size, align, 0, 0, &resource->start, &resource->r_bsh);
	if (error)
		return error;

	resource->size = size;
	return 0;
}

#endif  /* _LINUX_PCI_H_ */
