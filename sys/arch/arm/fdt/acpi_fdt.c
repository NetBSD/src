/* $NetBSD: acpi_fdt.c,v 1.5 2018/10/21 12:06:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_fdt.c,v 1.5 2018/10/21 12:06:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <dev/fdt/fdtvar.h>

#include <dev/acpi/acpivar.h>
#include <dev/pci/pcivar.h>

#include <arm/arm/psci.h>

#include <arm/acpi/acpi_pci_machdep.h>

static int	acpi_fdt_match(device_t, cfdata_t, void *);
static void	acpi_fdt_attach(device_t, device_t, void *);

static void	acpi_fdt_poweroff(device_t);

static struct acpi_pci_context acpi_fdt_pci_context;

static const char * const compatible[] = {
	"netbsd,acpi",
	NULL
};

static const struct fdtbus_power_controller_func acpi_fdt_power_funcs = {
	.poweroff = acpi_fdt_poweroff,
};

CFATTACH_DECL_NEW(acpi_fdt, 0, acpi_fdt_match, acpi_fdt_attach, NULL, NULL);

static int
acpi_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
}

static void
acpi_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	struct acpibus_attach_args aa;

	aprint_naive("\n");
	aprint_normal(": ACPI Platform support\n");

	fdtbus_register_power_controller(self, faa->faa_phandle,
	    &acpi_fdt_power_funcs);

	if (!acpi_probe())
		aprint_error_dev(self, "failed to probe ACPI\n");

	acpi_fdt_pci_context.ap_pc = arm_acpi_pci_chipset;
	acpi_fdt_pci_context.ap_pc.pc_conf_v = &acpi_fdt_pci_context;
	acpi_fdt_pci_context.ap_seg = 0;

	aa.aa_iot = 0;
	aa.aa_memt = faa->faa_bst;
	aa.aa_pc = &acpi_fdt_pci_context.ap_pc;
	aa.aa_pciflags =
	    /*PCI_FLAGS_IO_OKAY |*/ PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | 
	    PCI_FLAGS_MWI_OKAY;
#ifdef __HAVE_PCI_MSI_MSIX
	aa.aa_pciflags |= PCI_FLAGS_MSI_OKAY;
#endif
	aa.aa_ic = 0;
	aa.aa_dmat = faa->faa_dmat;
#ifdef _PCI_HAVE_DMA64
	aa.aa_dmat64 = faa->faa_dmat;
#endif
	config_found_ia(self, "acpibus", &aa, 0);
}

static void
acpi_fdt_poweroff(device_t dev)
{
	delay(500000);
	if (psci_available())
		psci_system_off();
}
