/*	$NetBSD: pci_machdep.h,v 1.2 2003/03/06 07:15:46 matt Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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

#ifndef __MACHINE_PCI_MACHDEP_H
#define	__MACHINE_PCI_MACHDEP_H

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */
#define	__PCI_BUS_DEVORDER 1
#define	__HAVE_PCI_CONF_HOOK 1

/*
 * be-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 *
 * Configuration tag; created from a {bus,device,function} triplet by
 * pci_make_tag(), and passed to pci_conf_read() and pci_conf_write().
 * We could instead always pass the {bus,device,function} triplet to
 * the read and write routines, but this would cause extra overhead.
 */

struct pci_attach_args;	/* Forward declaration */

/*
 * Types provided to machine-independent PCI code
 */
typedef struct pci_chipset *pci_chipset_tag_t;
typedef int pcitag_t;
typedef int pci_intr_handle_t;

extern struct powerpc_bus_dma_tag pci_bus_dma_tag;

struct pci_chipset_functions {
	void (*pcf_bus_attach_hook)(struct device *, struct device *,
		struct pcibus_attach_args *);
	int (*pcf_bus_maxdevs)(pci_chipset_tag_t, int);
	void (*pcf_bus_devorder)(pci_chipset_tag_t, int, char []);

	pcitag_t (*pcf_make_tag)(pci_chipset_tag_t, int, int, int);
	void (*pcf_decompose_tag)(pci_chipset_tag_t, pcitag_t, int *, int *,
		int *);

	pcireg_t (*pcf_conf_read)(pci_chipset_tag_t, pcitag_t, int);
	void (*pcf_conf_write)(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
	int (*pcf_conf_hook)(pci_chipset_tag_t, int, int, int, pcireg_t);
	void (*pcf_conf_interrupt)(pci_chipset_tag_t, int, int, int, int,
	    int *);

	int (*pcf_intr_map)(struct pci_attach_args *, pci_intr_handle_t *);
	const char *(*pcf_intr_string)(pci_chipset_tag_t, pci_intr_handle_t);
	const struct evcnt *(*pcf_intr_evcnt)(pci_chipset_tag_t,
	    pci_intr_handle_t);
	void *(*pcf_intr_establish)(pci_chipset_tag_t, pci_intr_handle_t,
	    int, int (*)(void *), void *);
	void (*pcf_intr_disestablish)(pci_chipset_tag_t, void *);
};

struct pci_chipset_md {
	int mdpc_busno;
	bus_space_handle_t mdpc_cfgaddr;
	bus_space_handle_t mdpc_cfgdata;
	bus_space_handle_t mdpc_syncreg;
	void *mdpc_gt;
	bus_space_tag_t mdpc_io_bs;
	bus_space_tag_t mdpc_mem_bs;
};

struct pci_chipset {
	struct device *pc_parent;
	const struct pci_chipset_functions *pc_funcs;
	struct pci_chipset_md pc_md;
};

/*
 * Functions provided to machine-independent PCI code.
 */

#define	pci_attach_hook(parent, self,  pba) \
	((*(pba)->pba_pc->pc_funcs->pcf_bus_attach_hook)((parent), (self), (pba)))
#define	pci_bus_maxdevs(pc, busno) \
	((*pc->pc_funcs->pcf_bus_maxdevs)((pc), (busno)))
#define	pci_enumerate_bus(sc, match, pa) \
	(pci_enumerate_bus_generic((sc), (match), (pa)))
#define	pci_make_tag(pc, bus, dev, func) \
	((*pc->pc_funcs->pcf_make_tag)((pc), (bus), (dev), (func)))
#define	pci_decompose_tag(pc, tag, bp, dp, fp) \
	((*pc->pc_funcs->pcf_decompose_tag)((pc), (tag), (bp), (dp), (fp)))
#define	pci_conf_read(pc, tag, reg) \
	((*pc->pc_funcs->pcf_conf_read)((pc), (tag), (reg)))
#define	pci_conf_write(pc, tag, reg, data) \
	((*pc->pc_funcs->pcf_conf_write)((pc), (tag), (reg), (data)))
#define	pci_conf_hook(pc, bus, dev, func, id) \
	((*pc->pc_funcs->pcf_conf_hook)((pc), (bus), (dev), (func), (id)))
#define	pci_conf_interrupt(pc, bus, dev, pin, swiz, iline) \
	((*pc->pc_funcs->pcf_conf_interrupt)((pc), (bus), (dev), (pin), (swiz), (iline)))
#define	pci_intr_map(pa, ihp) \
	((*pa->pa_pc->pc_funcs->pcf_intr_map)((pa), (ihp)))
#define	pci_intr_string(pc, ih) \
	((*(pc)->pc_funcs->pcf_intr_string)((pc), (ih)))
#define	pci_intr_evcnt(pc, ih) \
	((*(pc)->pc_funcs->pcf_intr_evcnt)((pc), (ih)))
#define	pci_intr_establish(pc, ih, ipl, func, arg) \
	((*(pc)->pc_funcs->pcf_intr_establish)((pc), (ih), (ipl), (func), (arg)))
#define	pci_intr_disestablish(pc, ih) \
	((*(pc)->pc_funcs->pcf_intr_evcnt)((pc), (ih)))
#define	pci_bus_devorder(pc, bus, devs) \
	((*(pc)->pc_funcs->pcf_bus_devorder)((pc), (bus), (devs)))

#endif /* __MACHINE_PCI_MACHDEP_H */
