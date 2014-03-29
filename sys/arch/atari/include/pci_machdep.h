/*	$NetBSD: pci_machdep.h,v 1.16 2014/03/29 19:28:26 christos Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
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

#ifndef _ATARI_PCI_MACHDEP_H_
#define _ATARI_PCI_MACHDEP_H_

#include <machine/intr.h>

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */
#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * Types provided to machine-independent PCI code
 */
typedef void	*pci_chipset_tag_t;

typedef u_long	pcitag_t;
typedef int	pci_intr_handle_t;

typedef struct	{
	int		ipl;	/* ipl requested			*/
	int		imask;	/* bitmask for MFP-register		*/
	int		(*ifunc)(void *);	/* function to call	*/
	void		*iarg;	/* argument for 'ifunc'			*/
	struct intrhand	*ihand;	/* save this for disestablishing	*/
} pci_intr_info_t;

/*
 * Functions provided to machine-independent PCI code.
 */
void		pci_attach_hook(device_t, device_t,
			struct pcibus_attach_args *);
int		pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	pci_make_tag(pci_chipset_tag_t, int, int, int);
void		pci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *,
		    int *);
pcireg_t	pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
const char	*pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t, char *, size_t);
const struct evcnt *pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
void		*pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
			int, int (*)(void *), void *);
void		pci_intr_disestablish(pci_chipset_tag_t, void *);

#endif /* _ATARI_PCI_MACHDEP_H_ */
