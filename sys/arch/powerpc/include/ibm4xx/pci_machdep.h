/*	$NetBSD: pci_machdep.h,v 1.7 2011/06/22 18:06:34 matt Exp $	*/

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

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

#define __HAVE_PCI_CONF_HOOK

#include <dev/pci/pcivar.h>
#include <powerpc/pci_machdep.h>

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * ibm4xx-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 *
 * Configuration tag; created from a {bus,device,function} triplet by
 * pci_make_tag(), and passed to pci_conf_read() and pci_conf_write().
 * We could instead always pass the {bus,device,function} triplet to
 * the read and write routines, but this would cause extra overhead.
 */

/*
 * Functions provided to machine-independent PCI code.
 */
void		ibm4xx_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		ibm4xx_pci_bus_maxdevs(void *, int);
pcitag_t	ibm4xx_pci_make_tag(void *, int, int, int);
void		ibm4xx_pci_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t	ibm4xx_pci_conf_read(void *, pcitag_t, int);
void		ibm4xx_pci_conf_write(void *, pcitag_t, int, pcireg_t);

int		ibm4xx_pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
int		ibm4xx_pci_intr_setattr(void *, pci_intr_handle_t *,
		    int, uint64_t);

void		ibm4xx_pci_conf_interrupt(void *, int, int, int, int, int *);
int		ibm4xx_pci_conf_hook(void *, int, int, int, pcireg_t);

void		ibm4xx_pci_machdep_init(void);
pci_chipset_tag_t
		ibm4xx_get_pci_chipset_tag(void);


/*
 * Internal functions.
 */
void		ibm4xx_pci_init(int);
