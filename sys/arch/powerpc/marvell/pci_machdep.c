/*	$NetBSD: pci_machdep.c,v 1.4.12.1 2014/08/20 00:03:20 tls Exp $	*/
/*
 * Copyright (c) 2009 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.4.12.1 2014/08/20 00:03:20 tls Exp $");

#include "gtpci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#if NGTPCI > 0
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>
#endif
#include <dev/marvell/marvellvar.h>

#include <machine/pci_machdep.h>


#if NGTPCI > 0
void gtpci_md_conf_interrupt(void *, int, int, int, int, int *);
int gtpci_md_conf_hook(void *, int, int, int, pcireg_t);


struct genppc_pci_chipset genppc_gtpci0_chipset = {
	.pc_conf_v = NULL,
	.pc_attach_hook = gtpci_attach_hook,
	.pc_bus_maxdevs = gtpci_bus_maxdevs,
	.pc_make_tag = gtpci_make_tag,
	.pc_conf_read = gtpci_conf_read,
	.pc_conf_write = gtpci_conf_write,

	.pc_intr_v = &genppc_gtpci0_chipset,
	.pc_intr_map = genppc_pci_intr_map,
	.pc_intr_string = genppc_pci_intr_string,
	.pc_intr_evcnt = genppc_pci_intr_evcnt,
	.pc_intr_establish = genppc_pci_intr_establish,
	.pc_intr_disestablish = genppc_pci_intr_disestablish,
	.pc_intr_setattr = genppc_pci_intr_setattr,

	.pc_msi_v = &genppc_gtpci0_chipset,
	GENPPC_PCI_MSI_INITIALIZER,

	.pc_conf_interrupt = gtpci_md_conf_interrupt,
	.pc_decompose_tag = gtpci_decompose_tag,
	.pc_conf_hook = gtpci_md_conf_hook,
};
struct genppc_pci_chipset genppc_gtpci1_chipset = {
	.pc_conf_v = NULL,
	.pc_attach_hook = gtpci_attach_hook,
	.pc_bus_maxdevs = gtpci_bus_maxdevs,
	.pc_make_tag = gtpci_make_tag,
	.pc_conf_read = gtpci_conf_read,
	.pc_conf_write = gtpci_conf_write,

	.pc_intr_v = &genppc_gtpci1_chipset,
	.pc_intr_map = genppc_pci_intr_map,
	.pc_intr_string = genppc_pci_intr_string,
	.pc_intr_evcnt = genppc_pci_intr_evcnt,
	.pc_intr_establish = genppc_pci_intr_establish,
	.pc_intr_disestablish = genppc_pci_intr_disestablish,
	.pc_intr_setattr = genppc_pci_intr_setattr,

	.pc_msi_v = &genppc_gtpci1_chipset,
	GENPPC_PCI_MSI_INITIALIZER,

	.pc_conf_interrupt = gtpci_md_conf_interrupt,
	.pc_decompose_tag = gtpci_decompose_tag,
	.pc_conf_hook = gtpci_md_conf_hook,
};
#endif
