/*	$NetBSD: t2.c,v 1.1.1.1.4.2 2014/05/18 17:46:02 rmind Exp $	*/

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
