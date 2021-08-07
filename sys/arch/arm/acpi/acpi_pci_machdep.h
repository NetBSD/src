/* $NetBSD: acpi_pci_machdep.h,v 1.8 2021/08/07 21:27:53 jmcneill Exp $ */

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
	int ap_maxbus;
	bus_space_tag_t ap_bst;
	bus_space_handle_t ap_conf_bsh;
	int (*ap_conf_read)(pci_chipset_tag_t, pcitag_t, int, pcireg_t *);
	int (*ap_conf_write)(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
	void *ap_conf_priv;
	int ap_pciflags_clear;
	u_int ap_flags;
#define	ACPI_PCI_FLAG_NO_MCFG		__BIT(0)	/* ignore MCFG table */
};

struct acpi_pci_quirk {
	const char			q_oemid[ACPI_OEM_ID_SIZE+1];
	const char			q_oemtableid[ACPI_OEM_TABLE_ID_SIZE+1];
	uint32_t			q_oemrevision;
	int				q_segment;
	void				(*q_init)(struct acpi_pci_context *);
};

const struct acpi_pci_quirk *	acpi_pci_md_find_quirk(int);

void	acpi_pci_smccc_init(struct acpi_pci_context *);
void	acpi_pci_graviton_init(struct acpi_pci_context *);
void	acpi_pci_layerscape_gen4_init(struct acpi_pci_context *);
void	acpi_pci_n1sdp_init(struct acpi_pci_context *);

#endif /* !_ARM_ACPI_PCI_MACHDEP_H */
