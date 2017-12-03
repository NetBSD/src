/*	$NetBSD: msipic.h,v 1.2.18.2 2017/12/03 11:36:50 jdolecek Exp $	*/

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

#ifndef _X86_PCI_MSIPIC_H_
#define _X86_PCI_MSIPIC_H_

#include <dev/pci/pcivar.h>

struct pic	*msipic_construct_msi_pic(const struct pci_attach_args *);
void		msipic_destruct_msi_pic(struct pic *);
struct pic	*msipic_construct_msix_pic(const struct pci_attach_args *);
void		msipic_destruct_msix_pic(struct pic *);
struct pic	*msipic_find_msi_pic(int);
int		msipic_set_msi_vectors(struct pic *, pci_intr_handle_t *, int);

bool		msipic_is_msi_pic(struct pic *);
int		msipic_get_devid(struct pic *);

void		msipic_init(void);

#endif /* _X86_PCI_MSIPIC_H_ */
