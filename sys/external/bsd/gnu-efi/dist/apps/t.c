/*	$NetBSD: t.c,v 1.1.1.1.6.2 2014/05/22 11:40:57 yamt Exp $	*/

#include <efi.h>
#include <efilib.h>

static CHAR16 *
a2u (char *str)
{
	static CHAR16 mem[2048];
	int i;

	for (i = 0; str[i]; ++i)
		mem[i] = (CHAR16) str[i];
	mem[i] = 0;
	return mem;
}

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

	InitializeLib(image_handle, systab);
	conout = systab->ConOut;
	uefi_call_wrapper(conout->OutputString, 2, conout, (CHAR16 *)L"Hello World!\n\r");
	uefi_call_wrapper(conout->OutputString, 2, conout, a2u("Hello World!\n\r"));

	return EFI_SUCCESS;
}
