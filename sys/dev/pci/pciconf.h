/*	$NetBSD: pciconf.h,v 1.16 2020/10/20 23:03:30 jmcneill Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_PCI_PCICONF_H_
#define	_DEV_PCI_PCICONF_H_

#include <sys/vmem.h>

struct pciconf_resources;

#define	PCICONF_RESOURCE_IO			0
#define	PCICONF_RESOURCE_MEM			1
#define	PCICONF_RESOURCE_PREFETCHABLE_MEM	2

struct pciconf_resources *
	pciconf_resource_init(void);
void	pciconf_resource_fini(struct pciconf_resources *);
int	pciconf_resource_add(struct pciconf_resources *, int,
	    bus_addr_t, bus_size_t);
void	pciconf_resource_reserve(int, bus_addr_t, bus_size_t,
	    void (*)(void *, uint64_t), void *);

/*
 * args: pci_chipset_tag_t, resources, firstbus, cacheline_size
 * If no prefetchable memory resources are available, plain memory resources
 * will be used for both.
 */
int	pci_configure_bus(pci_chipset_tag_t, struct pciconf_resources *,
	    int, int);

/* Defined in machdep code.  Returns the interrupt line to set */
/* args: chipset_tag, bus, dev, ipin, swiz, ptr to interrupt line */
#ifndef pci_conf_interrupt
void	pci_conf_interrupt(pci_chipset_tag_t, int, int, int, int, int *);
#endif

#define PCI_CONF_MAP_IO		0x0001
#define PCI_CONF_MAP_MEM	0x0002
#define PCI_CONF_MAP_ROM	0x0004
#define PCI_CONF_ENABLE_IO	0x0008
#define PCI_CONF_ENABLE_MEM	0x0010
#define PCI_CONF_ENABLE_BM	0x0020
#define PCI_CONF_ENABLE_PARITY	0x0040
#define PCI_CONF_ENABLE_SERR	0x0080
#define PCI_CONF_ENABLE_ROM	0x0100
#define PCI_CONF_DEFAULT	0x00ff
#define PCI_CONF_ALL		0x01ff

#endif /* _DEV_PCI_PCICONF_H_ */
