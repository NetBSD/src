/* $NetBSD: efiboot.c,v 1.12 2018/11/01 00:43:38 jmcneill Exp $ */

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
#include "efifile.h"
#include "efiblock.h"
#include "efifdt.h"
#include "efiacpi.h"

#include <sys/reboot.h>

#include <libfdt.h>

EFI_HANDLE IH;
EFI_DEVICE_PATH *efi_bootdp;
EFI_LOADED_IMAGE *efi_li;

int howto = 0;

static EFI_PHYSICAL_ADDRESS heap_start;
static UINTN heap_size = 8 * 1024 * 1024;
static EFI_EVENT delay_ev = 0;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);

EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS status;
	u_int sz = EFI_SIZE_TO_PAGES(heap_size);

	IH = imageHandle;

	InitializeLib(imageHandle, systemTable);

	(void)uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
	if (ST->ConOut->ClearScreen)
		(void)uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, sz, &heap_start);
	if (EFI_ERROR(status))
		return status;
	setheap((void *)heap_start, (void *)(heap_start + heap_size));

	status = uefi_call_wrapper(BS->HandleProtocol, 3, imageHandle, &LoadedImageProtocol, (void **)&efi_li);
	if (EFI_ERROR(status))
		return status;
	status = uefi_call_wrapper(BS->HandleProtocol, 3, efi_li->DeviceHandle, &DevicePathProtocol, (void **)&efi_bootdp);
	if (EFI_ERROR(status))
		efi_bootdp = NULL;

#ifdef EFIBOOT_DEBUG
	Print(L"Loaded image      : 0x%lX\n", efi_li);
	Print(L"FilePath          : 0x%lX\n", efi_li->FilePath);
	Print(L"ImageBase         : 0x%lX\n", efi_li->ImageBase);
	Print(L"ImageSize         : 0x%lX\n", efi_li->ImageSize);
	Print(L"Image file        : %s\n", DevicePathToStr(efi_li->FilePath));
#endif

	efi_acpi_probe();
	efi_fdt_probe();
	efi_pxe_probe();
	efi_net_probe();
	efi_file_system_probe();
	efi_block_probe();

	boot();

	return EFI_SUCCESS;
}

#ifdef EFIBOOT_RUNTIME_ADDRESS
static uint64_t
efi_runtime_alloc_va(uint64_t npages)
{
	static uint64_t va = EFIBOOT_RUNTIME_ADDRESS;
	static uint64_t sz = EFIBOOT_RUNTIME_SIZE;
	uint64_t nva;

	if (sz < (npages * EFI_PAGE_SIZE))
		panic("efi_acpi_alloc_va: couldn't allocate %" PRIu64 " pages", npages);

	nva = va;
	va += (npages * EFI_PAGE_SIZE);
	sz -= (npages * EFI_PAGE_SIZE);

	return nva;
}
#endif

#ifdef EFIBOOT_RUNTIME_ADDRESS
static void
efi_set_virtual_address_map(EFI_MEMORY_DESCRIPTOR *memmap, UINTN nentries, UINTN mapkey, UINTN descsize, UINT32 descver)
{
	EFI_MEMORY_DESCRIPTOR *md, *vmd, *vmemmap;
	EFI_STATUS status;
	int n, nrt;
	void *fdt;

	fdt = efi_fdt_data();

	vmemmap = alloc(nentries * descsize);
	if (vmemmap == NULL)
		panic("FATAL: couldn't allocate virtual memory map");

	for (n = 0, nrt = 0, vmd = vmemmap, md = memmap; n < nentries; n++, md = NextMemoryDescriptor(md, descsize)) {
		if ((md->Attribute & EFI_MEMORY_RUNTIME) == 0)
			continue;
		md->VirtualStart = efi_runtime_alloc_va(md->NumberOfPages);

		switch (md->Type) {
		case EfiRuntimeServicesCode:
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-code", md->PhysicalStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-code", md->VirtualStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-code", md->NumberOfPages * EFI_PAGE_SIZE);
			break;
		case EfiRuntimeServicesData:
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-data", md->PhysicalStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-data", md->VirtualStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-data", md->NumberOfPages * EFI_PAGE_SIZE);
			break;
		case EfiMemoryMappedIO:
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-mmio", md->PhysicalStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-mmio", md->VirtualStart);
			fdt_appendprop_u64(fdt, fdt_path_offset(fdt, "/chosen"), "netbsd,uefi-runtime-mmio", md->NumberOfPages * EFI_PAGE_SIZE);
			break;
		default:
			break;
		}

		*vmd = *md;
		vmd = NextMemoryDescriptor(vmd, descsize);
		++nrt;
	}

	status = uefi_call_wrapper(RT->SetVirtualAddressMap, 4, nrt * descsize, descsize, descver, vmemmap);
	if (EFI_ERROR(status)) {
		printf("WARNING: SetVirtualAddressMap failed\n");
		return;
	}
}
#endif

void
efi_cleanup(void)
{
	EFI_STATUS status;
	EFI_MEMORY_DESCRIPTOR *memmap;
	UINTN nentries, mapkey, descsize;
	UINT32 descver;

	memmap = LibMemoryMap(&nentries, &mapkey, &descsize, &descver);

	status = uefi_call_wrapper(BS->ExitBootServices, 2, IH, mapkey);
	if (EFI_ERROR(status)) {
		printf("WARNING: ExitBootServices failed\n");
		return;
	}

#ifdef EFIBOOT_RUNTIME_ADDRESS
	efi_set_virtual_address_map(memmap, nentries, mapkey, descsize, descver);
#endif
}

void
efi_exit(void)
{
	EFI_STATUS status;

	status = uefi_call_wrapper(BS->Exit, 4, IH, EFI_ABORTED, 0, NULL);
	if (EFI_ERROR(status))
		printf("WARNING: Exit failed\n");
}

void
efi_reboot(void)
{
	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetCold, EFI_SUCCESS, 0, NULL);

	printf("WARNING: Reset failed\n");
}

void
efi_delay(int us)
{
	EFI_STATUS status;
	UINTN val;

	if (delay_ev == 0) {
		status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_APPLICATION, 0, 0, &delay_ev);
		if (EFI_ERROR(status))
			return;
	}

	uefi_call_wrapper(BS->SetTimer, 3, delay_ev, TimerRelative, us * 10);
	uefi_call_wrapper(BS->WaitForEvent, 3, 1, &delay_ev, &val);
}

void
efi_progress(const char *fmt, ...)
{
	va_list ap;

	if ((howto & AB_SILENT) != 0)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}
