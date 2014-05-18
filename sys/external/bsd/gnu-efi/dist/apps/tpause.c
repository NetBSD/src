/*	$NetBSD: tpause.c,v 1.1.1.1.4.2 2014/05/18 17:46:02 rmind Exp $	*/

#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	Print(L"Press `q' to quit, any other key to continue:\n");
	
}
