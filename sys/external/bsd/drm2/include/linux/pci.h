/*	$NetBSD: pci.h,v 1.1.2.3 2013/07/24 03:00:34 riastradh Exp $	*/

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

#include <dev/pci/pcivar.h>

struct pci_device_id;

struct pci_dev {
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
pci_config_read_dword(struct pci_dev *pdev, int reg, uint32_t *valuep)
{
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg);
}

static inline void
pci_config_write_dword(struct pci_dev *pdev, int reg, uint32_t value)
{
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, value);
}

#endif  /* _LINUX_PCI_H_ */
