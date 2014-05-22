/*	$NetBSD: tpause.c,v 1.1.1.1.6.2 2014/05/22 11:40:57 yamt Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	Print(L"Press `q' to quit, any other key to continue:\n");
	
}
