/* $NetBSD: acpi_pci_machdep.h,v 1.2.8.1 2019/10/15 19:37:58 martin Exp $ */

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

#ifndef _ARM_ACPI_PCI_MACHDEP_H
#define _ARM_ACPI_PCI_MACHDEP_H

extern struct arm32_pci_chipset arm_acpi_pci_chipset;

struct acpi_pci_context {
	struct arm32_pci_chipset ap_pc;
	device_t ap_dev;
	u_int ap_seg;
	int ap_bus;
	ACPI_HANDLE ap_handle;
	bus_space_tag_t ap_bst;
	bus_space_handle_t ap_conf_bsh;
	int (*ap_conf_read)(pci_chipset_tag_t, pcitag_t, int, pcireg_t *);
	int (*ap_conf_write)(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
	int (*ap_bus_maxdevs)(struct acpi_pci_context *, int);
};

#endif /* !_ARM_ACPI_PCI_MACHDEP_H */
