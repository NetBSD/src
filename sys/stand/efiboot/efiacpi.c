/* $NetBSD: efiacpi.c,v 1.2 2018/10/23 10:12:59 jmcneill Exp $ */

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

#include "efiboot.h"
#include "efiacpi.h"
#include "efifdt.h"

#include <libfdt.h>

#define	ACPI_FDT_SIZE	(64 * 1024)

static EFI_GUID Acpi20TableGuid = ACPI_20_TABLE_GUID;
static EFI_GUID Smbios3TableGuid = SMBIOS3_TABLE_GUID;

static void *acpi_root = NULL;
static void *smbios3_table = NULL;

int
efi_acpi_probe(void)
{
	EFI_STATUS status;

	status = LibGetSystemConfigurationTable(&Acpi20TableGuid, &acpi_root);
	if (EFI_ERROR(status))
		return EIO;

	status = LibGetSystemConfigurationTable(&Smbios3TableGuid, &smbios3_table);
	if (EFI_ERROR(status))
		smbios3_table = NULL;

	return 0;
}

int
efi_acpi_available(void)
{
	return acpi_root != NULL;
}

void
efi_acpi_show(void)
{
	if (!efi_acpi_available())
		return;

	printf("ACPI: RSDP %p", acpi_root);
	if (smbios3_table)
		printf(", SMBIOS %p", smbios3_table);
	printf("\n");
}

int
efi_acpi_create_fdt(void)
{
	int error;
	void *fdt;

	if (acpi_root == NULL)
		return EINVAL;

	fdt = AllocatePool(ACPI_FDT_SIZE);
	if (fdt == NULL)
		return ENOMEM;

	error = fdt_create_empty_tree(fdt, ACPI_FDT_SIZE);
	if (error)
		return EIO;

	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/"), "compatible", "netbsd,generic-acpi");
	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/"), "model", "ACPI");
	fdt_setprop_cell(fdt, fdt_path_offset(fdt, "/"), "#address-cells", 2);
	fdt_setprop_cell(fdt, fdt_path_offset(fdt, "/"), "#size-cells", 2);

	fdt_add_subnode(fdt, fdt_path_offset(fdt, "/"), "chosen");
	fdt_setprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,acpi-root-table", (uint64_t)(uintptr_t)acpi_root);
	if (smbios3_table)
		fdt_setprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,smbios-table", (uint64_t)(uintptr_t)smbios3_table);

	fdt_add_subnode(fdt, fdt_path_offset(fdt, "/"), "acpi");
	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/acpi"), "compatible", "netbsd,acpi");

	return efi_fdt_set_data(fdt);
}
