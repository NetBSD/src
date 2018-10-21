/* $NetBSD: pci_msi_machdep.h,v 1.1 2018/10/21 00:42:06 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#ifndef _ARM_PCI_MSI_MACHDEP_H
#define _ARM_PCI_MSI_MACHDEP_H

#include <sys/queue.h>
#include <arm/pic/picvar.h>

struct arm_pci_msi {
	device_t		msi_dev;
	uint8_t			msi_id;		/* software ID */

	void *			msi_priv;

	pci_intr_handle_t *	(*msi_alloc)(struct arm_pci_msi *, int *, const struct pci_attach_args *, bool);
	void *			(*msi_intr_establish)(struct arm_pci_msi *,
				    pci_intr_handle_t, int, int (*)(void *), void *);
	void			(*msi_intr_release)(struct arm_pci_msi *,
				    pci_intr_handle_t *, int);

	SIMPLEQ_ENTRY(arm_pci_msi) msi_link;
};

int	arm_pci_msi_add(struct arm_pci_msi *);
void *	arm_pci_msi_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
	    int, int (*)(void *), void *);

#endif /* !_ARM_PCI_MSI_MACHDEP_H */
