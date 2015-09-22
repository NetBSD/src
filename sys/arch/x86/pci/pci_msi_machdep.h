/*	$NetBSD: pci_msi_machdep.h,v 1.2.2.3 2015/09/22 12:05:54 skrll Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan Inc.
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

#ifndef _X86_PCI_PCI_MSI_MACHDEP_H_
#define _X86_PCI_PCI_MSI_MACHDEP_H_

const char	*x86_pci_msi_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
void		x86_pci_msi_release(pci_chipset_tag_t, pci_intr_handle_t *,
		    int);
void		*x86_pci_msi_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *, const char *);
void		x86_pci_msi_disestablish(pci_chipset_tag_t, void *);

void		x86_pci_msix_release(pci_chipset_tag_t, pci_intr_handle_t *,
		    int);
void		*x86_pci_msix_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *, const char *xname);
void		x86_pci_msix_disestablish(pci_chipset_tag_t, void *);

#endif /* _X86_PCI_PCI_MSI_MACHDEP_H_ */
