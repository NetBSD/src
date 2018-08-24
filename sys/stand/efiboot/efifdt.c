/* $NetBSD: efifdt.c,v 1.1 2018/08/24 02:01:06 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "efiboot.h"
#include "efifdt.h"

#include <libfdt.h>

#define FDT_TABLE_GUID	\
	{ 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }
static EFI_GUID FdtTableGuid = FDT_TABLE_GUID;

#define	FDT_MEMORY_NODE_PATH	"/memory"
#define	FDT_MEMORY_NODE_NAME	"memory"
#define	FDT_CHOSEN_NODE_PATH	"/chosen"

#define	FDT_MEMORY_USABLE(_md)	\
	((_md)->Type == EfiLoaderCode || (_md)->Type == EfiLoaderData || \
	 (_md)->Type == EfiBootServicesCode || (_md)->Type == EfiBootServicesData || \
	 (_md)->Type == EfiConventionalMemory)

static void *fdt_data = NULL;

int
efi_fdt_probe(void)
{
	EFI_STATUS status;

	status = LibGetSystemConfigurationTable(&FdtTableGuid, &fdt_data);
	if (EFI_ERROR(status))
		return EIO;

	if (fdt_check_header(fdt_data) != 0) {
		fdt_data = NULL;
		return EINVAL;
	}

	return 0;
}

void *
efi_fdt_data(void)
{
	return fdt_data;
}

int
efi_fdt_size(void)
{
	return fdt_data == NULL ? 0 : fdt_totalsize(fdt_data);
}

void
efi_fdt_memory_map(void)
{
	UINTN nentries = 0, mapkey, descsize;
	EFI_MEMORY_DESCRIPTOR *md;
	UINT32 descver;
	int n, memory;

	memory = fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH);
	if (memory < 0)
		memory = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, "/"), FDT_MEMORY_NODE_NAME);
	if (memory < 0)
		panic("FDT: Failed to create " FDT_MEMORY_NODE_PATH " node");

	fdt_delprop(fdt_data, memory, "reg");

	const int address_cells = fdt_address_cells(fdt_data, fdt_path_offset(fdt_data, "/"));
	const int size_cells = fdt_size_cells(fdt_data, fdt_path_offset(fdt_data, "/"));

	md = LibMemoryMap(&nentries, &mapkey, &descsize, &descver);
	for (n = 0; n < nentries; n++, md = NextMemoryDescriptor(md, descsize)) {
		if ((md->Attribute & EFI_MEMORY_WB) == 0)
			continue;
		if (!FDT_MEMORY_USABLE(md))
			continue;

		if (address_cells == 1)
			fdt_appendprop_u32(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", (uint32_t)md->PhysicalStart);
		else
			fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", md->PhysicalStart);

		if (size_cells == 1)
			fdt_appendprop_u32(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", (uint32_t)md->NumberOfPages * EFI_PAGE_SIZE);
		else
			fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", (uint64_t)md->NumberOfPages * EFI_PAGE_SIZE);
	}
}

void
efi_fdt_bootargs(const char *bootargs)
{
	fdt_setprop_string(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "bootargs", bootargs);
}
