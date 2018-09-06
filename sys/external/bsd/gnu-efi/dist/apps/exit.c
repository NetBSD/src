/*	$NetBSD: exit.c,v 1.1.1.1.2.2 2018/09/06 06:56:38 pgoyette Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	InitializeLib(image_handle, systab);

	Exit(EFI_SUCCESS, 0, NULL);

	return EFI_UNSUPPORTED;
}
