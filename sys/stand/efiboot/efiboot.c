/* $NetBSD: efiboot.c,v 1.3 2018/08/24 23:21:56 jmcneill Exp $ */

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
#include "efifdt.h"

EFI_HANDLE efi_ih;
EFI_DEVICE_PATH *efi_bootdp;
EFI_LOADED_IMAGE *efi_li;

static EFI_PHYSICAL_ADDRESS heap_start;
static UINTN heap_size = 1 * 1024 * 1024;
static EFI_EVENT delay_ev = 0;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);

EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_STATUS status;
	u_int sz = EFI_SIZE_TO_PAGES(heap_size);

	efi_ih = imageHandle;

	InitializeLib(imageHandle, systemTable);

	(void)uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);

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

	efi_fdt_probe();
	efi_file_system_probe();

	boot();

	return EFI_SUCCESS;
}

void
efi_cleanup(void)
{
	EFI_STATUS status;
	UINTN nentries, mapkey, descsize;
	UINT32 descver;

	LibMemoryMap(&nentries, &mapkey, &descsize, &descver);

	status = uefi_call_wrapper(BS->ExitBootServices, 2, efi_ih, mapkey);
	if (EFI_ERROR(status))
		printf("WARNING: ExitBootServices failed\n");
}

void
efi_exit(void)
{
	EFI_STATUS status;

	status = uefi_call_wrapper(BS->Exit, 4, efi_ih, EFI_ABORTED, 0, NULL);
	if (EFI_ERROR(status))
		printf("WARNING: Exit failed\n");
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
