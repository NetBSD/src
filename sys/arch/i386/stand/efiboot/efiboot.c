/*	$NetBSD: efiboot.c,v 1.4.16.2 2017/12/03 11:36:18 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
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

#include "bootinfo.h"
#include "devopen.h"

EFI_HANDLE IH;
EFI_DEVICE_PATH *efi_bootdp;
EFI_LOADED_IMAGE *efi_li;
uintptr_t efi_main_sp;
physaddr_t efi_loadaddr, efi_kernel_start;
u_long efi_kernel_size;
bool efi_cleanuped;

static EFI_PHYSICAL_ADDRESS heap_start = EFI_ALLOCATE_MAX_ADDRESS;
static UINTN heap_size = 1 * 1024 * 1024;			/* 1MB */
static struct btinfo_efi btinfo_efi;

static void efi_heap_init(void);

EFI_STATUS EFIAPI efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);
EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS status;
	EFI_DEVICE_PATH *dp0, *dp;
	extern char twiddle_toggle;

	IH = imageHandle;
	InitializeLib(IH, systemTable);

	efi_main_sp = (uintptr_t)&status;
	twiddle_toggle = 1;	/* no twiddling until we're ready */

	cninit();
	efi_heap_init();
	efi_md_init();

	status = uefi_call_wrapper(BS->HandleProtocol, 3, IH,
	    &LoadedImageProtocol, (void **)&efi_li);
	if (EFI_ERROR(status))
		Panic(L"HandleProtocol(LoadedImageProtocol): %r", status);
	status = uefi_call_wrapper(BS->HandleProtocol, 3, efi_li->DeviceHandle,
	    &DevicePathProtocol, (void **)&dp0);
	if (EFI_ERROR(status))
		Panic(L"HandleProtocol(DevicePathProtocol): %r", status);
	for (dp = dp0; !IsDevicePathEnd(dp); dp = NextDevicePathNode(dp)) {
		if (DevicePathType(dp) == MEDIA_DEVICE_PATH)
			continue;
		if (DevicePathSubType(dp) == MEDIA_HARDDRIVE_DP) {
			boot_biosdev = 0x80;
			efi_bootdp = dp;
			break;
		}
		break;
	}

	efi_disk_probe();

	status = uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
	if (EFI_ERROR(status))
		Panic(L"SetWatchdogTimer: %r", status);

	boot();

	return EFI_SUCCESS;
}

void
efi_cleanup(void)
{
	EFI_STATUS status;
	EFI_MEMORY_DESCRIPTOR *desc;
	UINTN NoEntries, MapKey, DescriptorSize;
	UINT32 DescriptorVersion;
	struct btinfo_efimemmap *bim;
	size_t allocsz;

	clearit();

	memset(&btinfo_efi, 0, sizeof(btinfo_efi));
	btinfo_efi.systblpa = (intptr_t)ST;
#ifdef	__i386__	/* bootia32.efi */
	btinfo_efi.flags |= BI_EFI_32BIT;
#endif
	BI_ADD(&btinfo_efi, BTINFO_EFI, sizeof(btinfo_efi));

	NoEntries = 0;
	desc = efi_memory_get_map(&NoEntries, &MapKey, &DescriptorSize,
	    &DescriptorVersion, true);
	status = uefi_call_wrapper(BS->ExitBootServices, 2, IH, MapKey);
	if (EFI_ERROR(status)) {
		FreePool(desc);
		desc = efi_memory_get_map(&NoEntries, &MapKey, &DescriptorSize,
		    &DescriptorVersion, true);
		status = uefi_call_wrapper(BS->ExitBootServices, 2, IH, MapKey);
		if (EFI_ERROR(status))
			Panic(L"ExitBootServices failed");
	}
	efi_cleanuped = true;

	allocsz = sizeof(struct btinfo_efimemmap) - 1
	    + NoEntries * DescriptorSize;
	bim = alloc(allocsz);
	bim->num = NoEntries;
	bim->version = DescriptorVersion;
	bim->size = DescriptorSize;
	memcpy(bim->memmap, desc, NoEntries * DescriptorSize);
	BI_ADD(bim, BTINFO_EFIMEMMAP, allocsz);
}

static void
efi_heap_init(void)
{
	EFI_STATUS status;
	u_int sz = EFI_SIZE_TO_PAGES(heap_size);

	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress,
	    EfiLoaderData, sz, &heap_start);
	if (EFI_ERROR(status))
		Panic(L"%a: AllocatePages() failed: %d page(s): %r",
		    __func__, sz, status);
	setheap((void *)(UINTN)heap_start,
	    (void *)(UINTN)(heap_start + heap_size));
}
