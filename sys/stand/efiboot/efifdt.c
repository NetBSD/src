/* $NetBSD: efifdt.c,v 1.19 2019/08/30 00:01:33 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
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
#include "efiblock.h"

#include <libfdt.h>

#define FDT_TABLE_GUID	\
	{ 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }
static EFI_GUID FdtTableGuid = FDT_TABLE_GUID;

#define	FDT_MEMORY_NODE_PATH	"/memory"
#define	FDT_MEMORY_NODE_NAME	"memory"
#define	FDT_CHOSEN_NODE_PATH	"/chosen"
#define	FDT_CHOSEN_NODE_NAME	"chosen"

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

int
efi_fdt_set_data(void *data)
{
	if (fdt_check_header(data) != 0)
		return EINVAL;

	fdt_data = data;
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

bool
efi_fdt_overlay_is_compatible(void *dtbo)
{
	const int system_root = fdt_path_offset(fdt_data, "/");
	const int overlay_root = fdt_path_offset(dtbo, "/");

	if (system_root < 0 || overlay_root < 0)
		return false;

	const int system_ncompat = fdt_stringlist_count(fdt_data, system_root,
	    "compatible");
	const int overlay_ncompat = fdt_stringlist_count(dtbo, overlay_root,
	    "compatible");

	if (system_ncompat <= 0 || overlay_ncompat <= 0)
		return false;

	const char *system_compatible, *overlay_compatible;
	int si, oi;

	for (si = 0; si < system_ncompat; si++) {
		system_compatible = fdt_stringlist_get(fdt_data,
		    system_root, "compatible", si, NULL);
		if (system_compatible == NULL)
			continue;
		for (oi = 0; oi < overlay_ncompat; oi++) {
			overlay_compatible = fdt_stringlist_get(dtbo,
			    overlay_root, "compatible", oi, NULL);
			if (overlay_compatible == NULL)
				continue;
			if (strcmp(system_compatible, overlay_compatible) == 0)
				return true;
		}
	}

	return false;
}

int
efi_fdt_overlay_apply(void *dtbo, int *fdterr)
{
	int err = fdt_overlay_apply(fdt_data, dtbo);
	if (fdterr)
		*fdterr = err;
	return err == 0 ? 0 : EIO;
}

void
efi_fdt_init(u_long addr, u_long len)
{
	int error;

	error = fdt_open_into(fdt_data, (void *)addr, len);
	if (error < 0)
		panic("fdt_open_into failed: %d", error);

	fdt_data = (void *)addr;
}

void
efi_fdt_fini(void)
{
	int error;

	error = fdt_pack(fdt_data);
	if (error < 0)
		panic("fdt_pack failed: %d", error);
}

void
efi_fdt_show(void)
{
	const char *model, *compat;
	int n, ncompat;

	if (fdt_data == NULL)
		return;

	model = fdt_getprop(fdt_data, fdt_path_offset(fdt_data, "/"), "model", NULL);
	if (model)
		printf("FDT: %s [", model);
	ncompat = fdt_stringlist_count(fdt_data, fdt_path_offset(fdt_data, "/"), "compatible");
	for (n = 0; n < ncompat; n++) {
		compat = fdt_stringlist_get(fdt_data, fdt_path_offset(fdt_data, "/"),
		    "compatible", n, NULL);
		printf("%s%s", n == 0 ? "" : ", ", compat);
	}
	printf("]\n");
}

void
efi_fdt_memory_map(void)
{
	UINTN nentries = 0, mapkey, descsize;
	EFI_MEMORY_DESCRIPTOR *md, *memmap;
	UINT32 descver;
	UINT64 phys_start, phys_size;
	int n, memory, chosen;

	memory = fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH);
	if (memory < 0)
		memory = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, "/"), FDT_MEMORY_NODE_NAME);
	if (memory < 0)
		panic("FDT: Failed to create " FDT_MEMORY_NODE_PATH " node");

	chosen = fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH);
	if (chosen < 0)
		chosen = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, "/"), FDT_CHOSEN_NODE_NAME);
	if (chosen < 0)
		panic("FDT: Failed to create " FDT_CHOSEN_NODE_PATH " node");

	fdt_delprop(fdt_data, memory, "reg");

	const int address_cells = fdt_address_cells(fdt_data, fdt_path_offset(fdt_data, "/"));
	const int size_cells = fdt_size_cells(fdt_data, fdt_path_offset(fdt_data, "/"));

	memmap = LibMemoryMap(&nentries, &mapkey, &descsize, &descver);
	for (n = 0, md = memmap; n < nentries; n++, md = NextMemoryDescriptor(md, descsize)) {
		fdt_appendprop_u32(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "netbsd,uefi-memmap", md->Type);
		fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "netbsd,uefi-memmap", md->PhysicalStart);
		fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "netbsd,uefi-memmap", md->NumberOfPages);
		fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "netbsd,uefi-memmap", md->Attribute);

		if ((md->Attribute & EFI_MEMORY_RUNTIME) != 0)
			continue;

		if ((md->Attribute & EFI_MEMORY_WB) == 0)
			continue;
		if (!FDT_MEMORY_USABLE(md))
			continue;
		if ((address_cells == 1 || size_cells == 1) && md->PhysicalStart + (md->NumberOfPages * EFI_PAGE_SIZE) > 0xffffffff)
			continue;
		if (md->NumberOfPages <= 1)
			continue;

		phys_start = md->PhysicalStart;
		phys_size = md->NumberOfPages * EFI_PAGE_SIZE;

		if (phys_start & EFI_PAGE_MASK) {
			/* UEFI spec says these should be 4KB aligned, but U-Boot doesn't always.. */
			phys_start = (phys_start + EFI_PAGE_SIZE) & ~EFI_PAGE_MASK;
			phys_size -= (EFI_PAGE_SIZE * 2);
			if (phys_size == 0)
				continue;
		}

		if (address_cells == 1)
			fdt_appendprop_u32(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", (uint32_t)phys_start);
		else
			fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", phys_start);

		if (size_cells == 1)
			fdt_appendprop_u32(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", (uint32_t)phys_size);
		else
			fdt_appendprop_u64(fdt_data, fdt_path_offset(fdt_data, FDT_MEMORY_NODE_PATH),
			    "reg", phys_size);
	}
}

void
efi_fdt_gop(void)
{
	EFI_STATUS status;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
	EFI_HANDLE *gop_handle;
	UINTN ngop_handle, n;
	char buf[48];
	int fb;

	status = LibLocateHandle(ByProtocol, &GraphicsOutputProtocol, NULL, &ngop_handle, &gop_handle);
	if (EFI_ERROR(status) || ngop_handle == 0)
		return;

	for (n = 0; n < ngop_handle; n++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, gop_handle[n], &GraphicsOutputProtocol, (void **)&gop);
		if (EFI_ERROR(status))
			continue;

		mode = gop->Mode;
		if (mode == NULL)
			continue;

#ifdef EFIBOOT_DEBUG
		printf("GOP: FB @ 0x%lx size 0x%lx\n", mode->FrameBufferBase, mode->FrameBufferSize);
		printf("GOP: Version %d\n", mode->Info->Version);
		printf("GOP: HRes %d VRes %d\n", mode->Info->HorizontalResolution, mode->Info->VerticalResolution);
		printf("GOP: PixelFormat %d\n", mode->Info->PixelFormat);
		printf("GOP: PixelBitmask R 0x%x G 0x%x B 0x%x Res 0x%x\n",
		    mode->Info->PixelInformation.RedMask,
		    mode->Info->PixelInformation.GreenMask,
		    mode->Info->PixelInformation.BlueMask,
		    mode->Info->PixelInformation.ReservedMask);
		printf("GOP: Pixels per scanline %d\n", mode->Info->PixelsPerScanLine);
#endif

		if (mode->Info->PixelFormat == PixelBltOnly) {
			printf("GOP: PixelBltOnly pixel format not supported\n");
			continue;
		}

		fdt_setprop_u32(fdt_data,
		    fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "#address-cells", 2);
		fdt_setprop_u32(fdt_data,
		    fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "#size-cells", 2);
		fdt_setprop_empty(fdt_data,
		    fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), "ranges");

		snprintf(buf, sizeof(buf), "framebuffer@%" PRIx64, mode->FrameBufferBase);
		fb = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH), buf);
		if (fb < 0)
			panic("FDT: Failed to create framebuffer node");

		fdt_appendprop_string(fdt_data, fb, "compatible", "simple-framebuffer");
		fdt_appendprop_string(fdt_data, fb, "status", "okay");
		fdt_appendprop_u64(fdt_data, fb, "reg", mode->FrameBufferBase);
		fdt_appendprop_u64(fdt_data, fb, "reg", mode->FrameBufferSize);
		fdt_appendprop_u32(fdt_data, fb, "width", mode->Info->HorizontalResolution);
		fdt_appendprop_u32(fdt_data, fb, "height", mode->Info->VerticalResolution);
		fdt_appendprop_u32(fdt_data, fb, "stride", mode->Info->PixelsPerScanLine * 4);	/* XXX */
		fdt_appendprop_string(fdt_data, fb, "format", "a8b8g8r8");

		snprintf(buf, sizeof(buf), "/chosen/framebuffer@%" PRIx64, mode->FrameBufferBase);
		fdt_setprop_string(fdt_data, fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH),
		    "stdout-path", buf);

		return;
	}
}

void
efi_fdt_bootargs(const char *bootargs)
{
	struct efi_block_part *bpart = efi_block_boot_part();
	uint8_t macaddr[6];
	int chosen;

	chosen = fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH);
	if (chosen < 0)
		chosen = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, "/"), FDT_CHOSEN_NODE_NAME);
	if (chosen < 0)
		panic("FDT: Failed to create " FDT_CHOSEN_NODE_PATH " node");

	if (*bootargs)
		fdt_setprop_string(fdt_data, chosen, "bootargs", bootargs);

	if (bpart) {
		switch (bpart->type) {
		case EFI_BLOCK_PART_DISKLABEL:
			fdt_setprop(fdt_data, chosen, "netbsd,mbr",
			    bpart->hash, sizeof(bpart->hash));
			fdt_setprop_u32(fdt_data, chosen, "netbsd,partition",
			    bpart->index);
			break;
		case EFI_BLOCK_PART_GPT:
			if (bpart->gpt.ent.ent_name[0] == 0x0000) {
				fdt_setprop(fdt_data, chosen, "netbsd,gpt-guid",
				    bpart->hash, sizeof(bpart->hash));
			} else {
				char *label = NULL;
				int rv = ucs2_to_utf8(bpart->gpt.ent.ent_name, &label);
				if (rv == 0) {
					fdt_setprop_string(fdt_data, chosen, "netbsd,gpt-label", label);
					FreePool(label);
				}
			}
			break;
		default:
			break;
		}
	} else if (efi_net_get_booted_macaddr(macaddr) == 0) {
		fdt_setprop(fdt_data, chosen, "netbsd,booted-mac-address", macaddr, sizeof(macaddr));
	}
}

void
efi_fdt_initrd(u_long initrd_addr, u_long initrd_size)
{
	int chosen;

	if (initrd_size == 0)
		return;

	chosen = fdt_path_offset(fdt_data, FDT_CHOSEN_NODE_PATH);
	if (chosen < 0)
		chosen = fdt_add_subnode(fdt_data, fdt_path_offset(fdt_data, "/"), FDT_CHOSEN_NODE_NAME);
	if (chosen < 0)
		panic("FDT: Failed to create " FDT_CHOSEN_NODE_PATH " node");

	fdt_setprop_u64(fdt_data, chosen, "linux,initrd-start", initrd_addr);
	fdt_setprop_u64(fdt_data, chosen, "linux,initrd-end", initrd_addr + initrd_size);
}
