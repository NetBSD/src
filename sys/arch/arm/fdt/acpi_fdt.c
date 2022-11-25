/* $NetBSD: acpi_fdt.c,v 1.24 2022/11/25 22:17:20 mrg Exp $ */

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

#include "pci.h"
#include "opt_efi.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_fdt.c,v 1.24 2022/11/25 22:17:20 mrg Exp $");

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
#include <dev/smbiosvar.h>

#include <arm/arm/psci.h>

#include <arm/acpi/acpi_pci_machdep.h>

#ifdef EFI_RUNTIME
#include <arm/arm/efi_runtime.h>
#endif

static int	acpi_fdt_match(device_t, cfdata_t, void *);
static void	acpi_fdt_attach(device_t, device_t, void *);

static void	acpi_fdt_poweroff(device_t);

static void	acpi_fdt_smbios_init(device_t);

extern struct arm32_bus_dma_tag acpi_coherent_dma_tag;

static uint64_t smbios_table = 0;

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "netbsd,acpi" },
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_power_controller_func acpi_fdt_power_funcs = {
	.poweroff = acpi_fdt_poweroff,
};

CFATTACH_DECL_NEW(acpi_fdt, 0, acpi_fdt_match, acpi_fdt_attach, NULL, NULL);

static int
acpi_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
acpi_fdt_attach(device_t parent, device_t self, void *aux)
{
	extern void platform_init(void); /* XXX */
	struct fdt_attach_args * const faa = aux;
	struct acpibus_attach_args aa;

	aprint_naive("\n");
	aprint_normal("\n");

	acpi_fdt_smbios_init(self);
	platform_init();

	fdtbus_register_power_controller(self, faa->faa_phandle,
	    &acpi_fdt_power_funcs);

	if (!acpi_probe())
		panic("ACPI subsystem failed to initialize");

	platform_init();

	memset(&aa, 0, sizeof(aa));
#if NPCI > 0
	aa.aa_pciflags =
	    /*PCI_FLAGS_IO_OKAY |*/ PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
	    PCI_FLAGS_MWI_OKAY;
#ifdef __HAVE_PCI_MSI_MSIX
	aa.aa_pciflags |= PCI_FLAGS_MSI_OKAY | PCI_FLAGS_MSIX_OKAY;
#endif
#endif

	aa.aa_memt = faa->faa_bst;
	aa.aa_dmat = NULL;
	aa.aa_dmat64 = NULL;
	config_found(self, &aa, NULL, CFARGS_NONE);
}

static void
acpi_fdt_poweroff(device_t dev)
{
	delay(500000);
#ifdef EFI_RUNTIME
	if (arm_efirt_reset(EFI_RESET_SHUTDOWN) == 0)
		return;
#endif
	if (psci_available())
		psci_system_off();
}

static int
acpi_fdt_smbios_version(void)
{
	uint8_t *hdr;
	int smbver;

	if (smbios_table == 0) {
		return 0;
	}

	hdr = AcpiOsMapMemory(smbios_table, 24);
	if (hdr == NULL) {
		return 0;
	}
	if (smbios3_check_header(hdr)) {
		smbver = 3;
	} else if (smbios2_check_header(hdr)) {
		smbver = 2;
	} else {
		smbver = 0;
	}
	AcpiOsUnmapMemory(hdr, 24);
	return smbver;
}

static void
acpi_fdt_smbios_init(device_t dev)
{
	uint8_t *ptr;
	int smbver;

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0) {
		of_getprop_uint64(chosen, "netbsd,smbios-table", &smbios_table);
	}
	if (smbios_table == 0) {
		return;
	}

	smbios_entry.hdrphys = smbios_table;

	smbver = acpi_fdt_smbios_version();
	if (smbver == 3) {
		struct smb3hdr *sh = AcpiOsMapMemory(smbios_table, sizeof(*sh));
		if (sh == NULL) {
			return;
		}

		ptr = AcpiOsMapMemory(sh->addr, sh->size);
		if (ptr != NULL) {
			smbios_entry.tabphys = sh->addr;
			smbios_entry.addr = ptr;
			smbios_entry.len = sh->size;
			smbios_entry.rev = sh->eprev;
			smbios_entry.mjr = sh->majrev;
			smbios_entry.min = sh->minrev;
			smbios_entry.doc = sh->docrev;
			smbios_entry.count = UINT16_MAX;
		}

		aprint_normal_dev(dev, "SMBIOS rev. %d.%d.%d @ 0x%lx\n",
		    sh->majrev, sh->minrev, sh->docrev, (u_long)sh->addr);
		AcpiOsUnmapMemory(sh, sizeof(*sh));
	} else if (smbver == 2) {
		struct smbhdr *sh = AcpiOsMapMemory(smbios_table, sizeof(*sh));
		if (sh == NULL) {
			return;
		}

		ptr = AcpiOsMapMemory(sh->addr, sh->size);
		if (ptr != NULL) {
			smbios_entry.tabphys = sh->addr;
			smbios_entry.addr = ptr;
			smbios_entry.len = sh->size;
			smbios_entry.rev = 0;
			smbios_entry.mjr = sh->majrev;
			smbios_entry.min = sh->minrev;
			smbios_entry.doc = 0;
			smbios_entry.count = sh->count;
		}

		aprint_normal_dev(dev, "SMBIOS rev. %d.%d @ 0x%lx (%d entries)\n",
		    sh->majrev, sh->minrev, (u_long)sh->addr, sh->count);
		AcpiOsUnmapMemory(sh, sizeof(*sh));
	}
}
