/* $NetBSD: efiacpi.c,v 1.10 2021/07/23 21:33:00 jmcneill Exp $ */

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
#include "smbios.h"

struct acpi_rdsp {
	char signature[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t revision;
	uint32_t rsdtphys;
	uint32_t length;
	uint64_t xsdtphys;
	uint8_t extcsum;
	uint8_t reserved[3];
};

#include <libfdt.h>

#define	ACPI_FDT_SIZE	(128 * 1024)

static EFI_GUID Acpi20TableGuid = ACPI_20_TABLE_GUID;
static EFI_GUID Smbios3TableGuid = SMBIOS3_TABLE_GUID;
static EFI_GUID SmbiosTableGuid = SMBIOS_TABLE_GUID;

static int acpi_enable = 1;
static void *acpi_root = NULL;
static void *smbios_table = NULL;

int
efi_acpi_probe(void)
{
	EFI_STATUS status;

	status = LibGetSystemConfigurationTable(&Acpi20TableGuid, &acpi_root);
	if (EFI_ERROR(status))
		return EIO;

	status = LibGetSystemConfigurationTable(&Smbios3TableGuid, &smbios_table);
	if (EFI_ERROR(status)) {
		status = LibGetSystemConfigurationTable(&SmbiosTableGuid, &smbios_table);
	}
	if (EFI_ERROR(status)) {
		smbios_table = NULL;
	}

	return 0;
}

int
efi_acpi_available(void)
{
	return acpi_root != NULL;
}

int
efi_acpi_enabled(void)
{
	return acpi_enable;
}

void
efi_acpi_enable(int enable)
{
	acpi_enable = enable;
}

static char model_buf[128];

static const char *
efi_acpi_get_model(void)
{
	struct smbtable smbios;
	struct smbios_sys *psys;
	const char *s;
	char *buf;

	memset(model_buf, 0, sizeof(model_buf));

	if (smbios_table != NULL) {
		smbios_init(smbios_table);

		buf = model_buf;
		smbios.cookie = 0;
		if (smbios_find_table(SMBIOS_TYPE_SYSTEM, &smbios)) {
			psys = smbios.tblhdr;
			if ((s = smbios_get_string(&smbios, psys->vendor, buf, 64)) != NULL) {
				buf += strlen(s);
				*buf++ = ' ';
			}
			smbios_get_string(&smbios, psys->product, buf, 64);
		}
	}

	if (model_buf[0] == '\0')
		strcpy(model_buf, "ACPI");

	return model_buf;
}

void
efi_acpi_show(void)
{
	struct acpi_rdsp *rsdp = acpi_root;

	if (!efi_acpi_available())
		return;

	printf("ACPI: v%02d %c%c%c%c%c%c\n", rsdp->revision,
	    rsdp->oemid[0], rsdp->oemid[1], rsdp->oemid[2],
	    rsdp->oemid[3], rsdp->oemid[4], rsdp->oemid[5]);

	if (smbios_table)
		printf("SMBIOS: %s\n", efi_acpi_get_model());
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

	const char *model = efi_acpi_get_model();

	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/"), "compatible", "netbsd,generic-acpi");
	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/"), "model", model);
	fdt_setprop_cell(fdt, fdt_path_offset(fdt, "/"), "#address-cells", 2);
	fdt_setprop_cell(fdt, fdt_path_offset(fdt, "/"), "#size-cells", 2);

	fdt_add_subnode(fdt, fdt_path_offset(fdt, "/"), "chosen");
	fdt_setprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,acpi-root-table", (uint64_t)(uintptr_t)acpi_root);
	if (smbios_table)
		fdt_setprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,smbios-table", (uint64_t)(uintptr_t)smbios_table);

	fdt_add_subnode(fdt, fdt_path_offset(fdt, "/"), "acpi");
	fdt_setprop_string(fdt, fdt_path_offset(fdt, "/acpi"), "compatible", "netbsd,acpi");

	return efi_fdt_set_data(fdt);
}
