/*	$NetBSD: t2.c,v 1.1.1.1.36.1 2019/06/10 22:08:34 christos Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

        InitializeLib(image, systab);
	conout = systab->ConOut;
	uefi_call_wrapper(conout->OutputString, 2, conout, L"Hello World!\n\r");

	return EFI_SUCCESS;
}
