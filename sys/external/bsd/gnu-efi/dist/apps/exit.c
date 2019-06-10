/*	$NetBSD: exit.c,v 1.1.1.1.6.2 2019/06/10 22:08:34 christos Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	InitializeLib(image_handle, systab);

	Exit(EFI_SUCCESS, 0, NULL);

	return EFI_UNSUPPORTED;
}
