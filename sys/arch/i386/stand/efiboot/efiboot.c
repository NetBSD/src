/*	$NetBSD: efiboot.c,v 1.12 2020/02/09 12:13:39 jmcneill Exp $	*/

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
enum efi_boot_device_type efi_bootdp_type = BOOT_DEVICE_TYPE_HD;
EFI_LOADED_IMAGE *efi_li;
uintptr_t efi_main_sp;
physaddr_t efi_loadaddr, efi_kernel_start;
u_long efi_kernel_size;
bool efi_cleanuped;
struct btinfo_efimemmap *btinfo_efimemmap = NULL;

static EFI_PHYSICAL_ADDRESS heap_start = EFI_ALLOCATE_MAX_ADDRESS;
static UINTN heap_size = 1 * 1024 * 1024;			/* 1MB */
static struct btinfo_efi btinfo_efi;

static void efi_heap_init(void);

/*
 * Print an UUID in a human-readable manner.
 */
void
efi_aprintuuid(EFI_GUID *uuid)
{
	Print(L" %08x", uuid->Data1);
	Print(L"-%04x", uuid->Data2);
	Print(L"-%04x\n", uuid->Data3);
}

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
		panic("HandleProtocol(LoadedImageProtocol): %" PRIxMAX,
		    (uintmax_t)status);
	status = uefi_call_wrapper(BS->HandleProtocol, 3, efi_li->DeviceHandle,
	    &DevicePathProtocol, (void **)&dp0);
	if (EFI_ERROR(status))
		panic("HandleProtocol(DevicePathProtocol): %" PRIxMAX,
		    (uintmax_t)status);
	efi_bootdp = dp0;
	for (dp = dp0; !IsDevicePathEnd(dp); dp = NextDevicePathNode(dp)) {
		if (DevicePathType(dp) == MEDIA_DEVICE_PATH &&
		    DevicePathSubType(dp) == MEDIA_CDROM_DP) {
			efi_bootdp_type = BOOT_DEVICE_TYPE_CD;
			break;
		}
		if (DevicePathType(dp) == MESSAGING_DEVICE_PATH &&
		    DevicePathSubType(dp) == MSG_MAC_ADDR_DP) {
			efi_bootdp_type = BOOT_DEVICE_TYPE_NET;
			break;
		}
	}

	efi_memory_probe();
	efi_disk_probe();
	efi_pxe_probe();
	efi_net_probe();

	void *esrt_root = NULL;
	EFI_GUID EsrtGuid = {0xb122a263,0x3661,0x4f68, \
    {0x99,0x29,0x78,0xf8,0xb0,0xd6,0x21,0x80}};

	int found = 0;
	int esrt_index;

	for (int i = 0; i < systemTable->NumberOfTableEntries; ++i) {
		Print(L"%d ", i);
		efi_aprintuuid(&systemTable->ConfigurationTable[i].VendorGuid);
		if (!memcmp(&EsrtGuid, &(systemTable->ConfigurationTable[i].VendorGuid), sizeof(EFI_GUID))) {
			found = 1;
			esrt_index = i;
			Print(L"found ESRT!\n");
			esrt_root = systemTable->ConfigurationTable[i].VendorTable;
			break;
		}
	}
	if (!found) {
		panic("ESRT not found\n");
	}

	Print(L"Number of cfgtbl entries = %d\n", systemTable->NumberOfTableEntries);
	Print(L"ESRT adress = %" PRIx32 "\n", esrt_root);

	if (esrt_root == NULL) {
		panic("ESRT not available\n");
	}

	struct EFI_SYSTEM_RESOURCE_TABLE *esrt = (struct EFI_SYSTEM_RESOURCE_TABLE *) esrt_root;

	Print(L"Resource count = %d\n", esrt->FwResourceCount);
	Print(L"Max resource count = %d\n", esrt->FwResourceCountMax);
	Print(L"Resource version = %d\n", esrt->FwResourceVersion);

	int esrt_size = sizeof(struct EFI_SYSTEM_RESOURCE_TABLE) + 2 * sizeof(struct EFI_SYSTEM_RESOURCE_ENTRY);

	void *esrt_copy;

	status = uefi_call_wrapper(BS->AllocatePool,
									3,
									EfiRuntimeServicesData,
									esrt_size,
									(void**) &esrt_copy
									);
	if (EFI_ERROR(status)) {
		panic("BS->AllocatePool() error!\n");
		return EIO;
	}

	// esrt_copy = AllocateRuntimePool(esrt_size);
	// if (name == NULL) {
	// 	printf("memory allocation failed: %" PRIuMAX" bytes\n",
	// 	    (uintmax_t)esrt_size);
	// 	return;
	// }

	// uefi_call_wrapper(BS->CopyMem,
	// 						3,
	// 						esrt_copy,
	// 						esrt,
	// 						esrt_size
	// );

	CopyMem(esrt_copy, esrt, esrt_size);
	systemTable->ConfigurationTable[esrt_index].VendorTable = esrt_copy;

	Print(L"Copying table successful\n");

	// /*
	//  * Print memory byte by byte
	//  */
	// Print(L"ESRT 16 bytes of memory starting from ESRT pointer\n");
	// unsigned char *r = (unsigned char *)esrt;
	// for (int i = 0; i < 16; ++i) {
	// 	Print(L"%02x\n", r[i]);
	// }
	// Print(L"\n");

	// /*
	//  * Print memory byte by byte
	//  */
	// Print(L"ESRT copy 16 bytes of memory starting from ESRT pointer\n");
	// unsigned char *q = (unsigned char *)esrt_copy;
	// for (int i = 0; i < 16; ++i) {
	// 	Print(L"%02x\n", q[i]);
	// }
	// Print(L"\n");

	// status = uefi_call_wrapper(BS->InstallConfigurationTable,
	// 								2,
	// 								&EsrtGuid,
	// 								&esrt_root
	// );

	panic("Stopping for debugging purposes\n");
	return EIO;


	// struct EFI_SYSTEM_RESOURCE_TABLE *esrt;

	// int bufsize = sizeof (struct EFI_SYSTEM_RESOURCE_TABLE) + sizeof (struct EFI_SYSTEM_RESOURCE_ENTRY);

	// status = uefi_call_wrapper(BS->AllocatePool,
	// 								3,
	// 								EfiRuntimeServicesData,
	// 								bufsize,
	// 								(void**) &esrt
	// 								);
	// if (EFI_ERROR(status)) {
	// 	panic("BS->AllocatePool() error!\n");
	// 	return EIO;
	// }

	// esrt->FwResourceVersion  = 1;
  	// esrt->FwResourceCount    = 2;
  	// esrt->FwResourceCountMax = 2;

  	// struct EFI_SYSTEM_RESOURCE_ENTRY *Esre = (void *)&esrt[1];

  	// EFI_GUID FwClass = {
    // 	0x415f009f, 0xfb1d, 0x4cc3, {0x8a, 0x25, 0x57, 0x10, 0xa7, 0x70, 0x59, 0x18 }
  	// };
  	// CopyMem(&Esre[0].FwClass, &FwClass, sizeof (EFI_GUID));

  	// Esre[0].FwType = 1; // 1
  	// Esre[0].FwVersion = 5;
  	// Esre[0].LowestSupportedFwVersion = 1;
  	// Esre[0].CapsuleFlags = CAPSULE_FLAGS_INITIATE_RESET; // 0x00040000
  	// Esre[0].LastAttemptVersion = 0;
  	// Esre[0].LastAttemptStatus = 0; // 0

	// EFI_GUID gESRTGuid = ESRT_TABLE_GUID;

	// status = uefi_call_wrapper(BS->InstallConfigurationTable,
	// 								2,
	// 								&gESRTGuid,
	// 								&esrt
	// 								);

  	//efi_bs->bs_installconfigurationtable(&gESRTGuid, &esrt);

	status = uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
	if (EFI_ERROR(status))
		panic("SetWatchdogTimer: %" PRIxMAX, (uintmax_t)status);

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
	size_t allocsz;

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
			panic("ExitBootServices failed");
	}
	efi_cleanuped = true;

	efi_memory_compact_map(desc, &NoEntries, DescriptorSize);
	allocsz = sizeof(struct btinfo_efimemmap) - 1
	    + NoEntries * DescriptorSize;
	btinfo_efimemmap = alloc(allocsz);
	btinfo_efimemmap->num = NoEntries;
	btinfo_efimemmap->version = DescriptorVersion;
	btinfo_efimemmap->size = DescriptorSize;
	memcpy(btinfo_efimemmap->memmap, desc, NoEntries * DescriptorSize);
	BI_ADD(btinfo_efimemmap, BTINFO_EFIMEMMAP, allocsz);
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
