/*	$NetBSD: t2.c,v 1.1.1.1.6.2 2014/05/22 11:40:57 yamt Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

	conout = systab->ConOut;
	uefi_call_wrapper(conout->OutputString, 2, conout, L"Hello World!\n\r");

	return EFI_SUCCESS;
}
