/* $NetBSD: pci_machdep.h,v 1.1.4.3 2001/03/12 13:29:29 bouyer Exp $ */

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _MACHINE_PCI_MACHDEP_H_
#define _MACHINE_PCI_MACHDEP_H_

/*
 * We want to contro both device & function probe order.
 */
#define		__PCI_BUS_DEVORDER
#define		__PCI_DEV_FUNCORDER

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * define some bits used to glue into the common PCI code.
 */

typedef struct sparc_pci_chipset *pci_chipset_tag_t;
typedef u_int pcitag_t;
typedef u_int pci_intr_handle_t;

struct sparc_pci_chipset {
	void			*cookie;	/* psycho_pbm, but sssh! */
	int			node;		/* OFW node */
	int			busno;		/* PCI bus number */
};

void		pci_attach_hook(struct device *, struct device *,
				     struct pcibus_attach_args *);
#ifdef __PCI_BUS_DEVORDER
int		pci_bus_devorder(pci_chipset_tag_t, int, char *);
#endif
#ifdef __PCI_DEV_FUNCORDER
int		pci_dev_funcorder(pci_chipset_tag_t, int, int, char *);
#endif
int		pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	pci_make_tag(pci_chipset_tag_t, int, int, int);
pcireg_t	pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		pci_conf_write(pci_chipset_tag_t, pcitag_t, int,
				    pcireg_t);
int		pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char	*pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
const struct evcnt *pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
void		*pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
					 int, int (*)(void *), void *);
void		pci_intr_disestablish(pci_chipset_tag_t, void *);

#endif /* _MACHINE_PCI_MACHDEP_H_ */
