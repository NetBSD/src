/*	$NetBSD: t7.c,v 1.1.1.1 2014/04/01 16:16:06 jakllsch Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_INPUT_KEY efi_input_key;
	EFI_STATUS efi_status;

	InitializeLib(image, systab);

	Print(L"HelloLib application started\n");

	Print(L"\n\n\nHit any key to exit this image\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);

	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, L"\n\n");

	efi_status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &efi_input_key);

	Print(L"ScanCode: %xh  UnicodeChar: %xh\n",
		efi_input_key.ScanCode, efi_input_key.UnicodeChar);

	return EFI_SUCCESS;
}
